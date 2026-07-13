//-----------------------------------------------------------------------------
// TerrainGpu.cpp -- Artscout - 2026: #78 Phase 2 + Phase 4 (see TerrainGpu.h)
//
// GPU world-space terrain through the object path. Posts carry ABSOLUTE world
// coords; the object view matrix is rotation-only, so vertices are camera-relative
// (postWorld - camPos), world = identity.
//
// Textured (Phase 2b): each quad uses ONE tile (texID + UVs from its (r,c) corner
// post). Vertices are per-quad; quads are grouped by tile SRV. Coarse/unloaded tiles
// fall back to flat elevation colour.
//
// Geomorphing (Phase 4): near a LOD's outer edge each vertex's HEIGHT is morphed
// toward the coarse (LOD+1) representation -- coarseZ = interpolation of the EVEN
// neighbour posts (the ones that survive the 2x decimation into LOD+1). At the outer
// edge (alpha=1) the fine edge lands exactly on the coarse surface -> no T-junction
// crack, no height pop as the camera moves. Computed from the fine grid alone (even
// posts == the coarse LOD's posts). Only Z morphs; X/Y stay on the fine grid.
//-----------------------------------------------------------------------------
#include <windows.h>
#include <math.h>
#include <vector>   // #91: per-SRV terrain batch accumulation
#include <map>

#include "Ttypes.h"     // POST macros, FeetPerPost
#include "TMap.h"       // TheMap.LastNearTexLOD()
#include "Tpost.h"      // Tpost { z, texID, u,v,d, ... }
#include "TerrTex.h"    // TheTerrTextures.GetTileSRV()
#include "RViewPnt.h"   // RViewPoint
#include "Graphics/DXEngine/DXEngine.h"            // CDXEngine::GetObjProjection/View/CameraPos
#include "Graphics/DXEngine/common/IRenderer.h" // g_pRenderer
#include "TerrainGpu.h"

extern bool g_bGpuTerrain;

// Object-layout vertex: must match ObjV / D3D11VertexEx (40 bytes) exactly.
struct TGpuVert
{
    float p[3];
    float n[3];
    unsigned long col;
    unsigned long spec;
    float tu, tv;
};

static const int CHUNK = 32;
static const int SIDE  = CHUNK + 1;
static const int GW    = SIDE + 2;   // grid width WITH a 1-post border (for geomorph neighbours)
static const int MAX_RADIUS = 96;

// Artscout - 2026: #DX12 A5 -- per-render terrain radius cap. The GPU terrain draws one DrawTerrainMesh per
// distinct tile texture per 32x32 chunk out to MAX_RADIUS=96 posts -> ~thousands of small draws per view. That
// is fine (barely) for ONE view, but the TGP/Maverick/FLIR sensor renders a SECOND full-radius view into the
// atlas every frame -> ~9000 draws/frame -> the GPU misses the TDR deadline (DEVICE_HUNG), worst while slewing.
// A zoomed narrow-FOV sensor does not need the full 96-post radius; the sensor pass sets a small cap so it draws
// far fewer chunks. 0 = no cap (the main world view). Set via FF_SetTerrainRadiusCap around the sensor DrawScene.
int g_terrainRadiusCap = 0;
void FF_SetTerrainRadiusCap(int r) { g_terrainRadiusCap = r; }

// cached post grid cell (camera-relative). za = ABSOLUTE post z (for the morph math);
// z = camera-relative MORPHED z (filled after caching).
struct GCell
{
    float za;
    float x, y, z;
    unsigned long tex;
    float u, v, d;
    unsigned long col;
    bool ok;
};

// #91 Stage 1: global per-SRV terrain batch. The GPU terrain was texture-bound -- one DrawTerrainMesh per
// distinct tile texture PER 32x32 chunk -> ~thousands of tiny draws (the #65 fps cost + the sensor's TDR when
// it renders a second view). Accumulate every quad of a given tile texture across ALL chunks/LODs into one
// bucket, then emit ONE DrawTerrainMesh per texture at the end. Same textures/quads -> identical pixels, far
// fewer draws. (Stage 2 = a Texture2DArray/bindless index -> the whole terrain in 1-2 draws.)
namespace {
    struct TBucket { void* srv; std::vector<TGpuVert> v; std::vector<unsigned short> i; };
    static std::vector<TBucket*> s_buckets;      // heap-stable buckets (reused across frames; contents cleared)
    static std::map<void*, int>  s_srvToBucket;

    static TBucket& TerrainBucket(void* srv)
    {
        std::map<void*, int>::iterator it = s_srvToBucket.find(srv);
        if (it != s_srvToBucket.end()) return *s_buckets[it->second];
        int idx = (int)s_buckets.size();
        s_srvToBucket[srv] = idx;
        s_buckets.push_back(new TBucket());
        s_buckets[idx]->srv = srv;
        return *s_buckets[idx];
    }
    static void FlushTerrainBucket(TBucket& b)
    {
        if (!b.v.empty() && !b.i.empty())
            g_pRenderer->SetTexture(0, (struct ID3D11ShaderResourceView*)b.srv),
            g_pRenderer->DrawTerrainMesh(&b.v[0], (int)b.v.size(), &b.i[0], (int)b.i.size());
        b.v.clear(); b.i.clear();
    }
    static void TerrainBatchBegin()   { for (size_t k = 0; k < s_buckets.size(); ++k) { s_buckets[k]->v.clear(); s_buckets[k]->i.clear(); } }
    static void TerrainBatchFlushAll(){ for (size_t k = 0; k < s_buckets.size(); ++k) FlushTerrainBucket(*s_buckets[k]); }
}

// Artscout - 2026: #78 terrain -- sample the COARSE (LOD+1) surface at the exact world position of the
// FINE post (r,c,LOD), so the geomorph target is the REAL adjacent-LOD height instead of a reconstruction
// from the fine grid. The theater LODs are NOT a pure decimation (the coarse DEM is independently
// filtered), so rebuilding the coarse height from the fine even-neighbours misses the true LOD+1 surface
// -> the morphed fine outer edge doesn't land on the LOD+1 patch it abuts -> cracks + "floating" heights
// (worst at the near/far texture boundary, where the texture change makes the gap obvious). A fine EVEN
// post coincides with a coarse post (r>>1,c>>1); odd posts linearly interpolate the two/four bracketing
// coarse posts. cR/cC/as = LOD+1 centre indices + available half-range (GetPost is UNSAFE out of range,
// tviewpnt.h:123). Returns false if any needed coarse post is unavailable -> caller falls back.
static bool TgpuCoarseZ(RViewPoint* vp, int r, int c, int LOD1, int cR, int cC, int as, float& out)
{
    #define TGPU_FETCH(R, C, Z) do { \
        const int _dR = ((R) >= cR) ? ((R) - cR) : (cR - (R)); \
        const int _dC = ((C) >= cC) ? ((C) - cC) : (cC - (C)); \
        if (_dR > as || _dC > as) return false; \
        Tpost* _p = vp->GetPost((R), (C), LOD1); if (!_p) return false; (Z) = _p->z; } while (0)
    const int ro = r & 1, co = c & 1;
    if (!ro && !co) { float z; TGPU_FETCH(r >> 1, c >> 1, z); out = z; return true; }
    if (ro && !co)  { const int R0 = (r - 1) >> 1; float z0, z1; TGPU_FETCH(R0, c >> 1, z0); TGPU_FETCH(R0 + 1, c >> 1, z1); out = 0.5f * (z0 + z1); return true; }
    if (!ro && co)  { const int C0 = (c - 1) >> 1; float z0, z1; TGPU_FETCH(r >> 1, C0, z0); TGPU_FETCH(r >> 1, C0 + 1, z1); out = 0.5f * (z0 + z1); return true; }
    { const int R0 = (r - 1) >> 1, C0 = (c - 1) >> 1; float z0, z1, z2, z3;
      TGPU_FETCH(R0, C0, z0); TGPU_FETCH(R0 + 1, C0, z1); TGPU_FETCH(R0, C0 + 1, z2); TGPU_FETCH(R0 + 1, C0 + 1, z3);
      out = 0.25f * (z0 + z1 + z2 + z3); return true; }
    #undef TGPU_FETCH
}

// Artscout - 2026: #78 -- connector/single-layer LOD tiling. Each LOD occupies an axis-aligned post RING whose
// OUTER edges are snapped to EVEN absolute posts (== the coarse LOD+1 grid: coarse post k == fine post 2k). The
// outer edges (rLo,rHi,cLo,cHi in ABSOLUTE level-LOD posts) are returned in outB[4]; the caller feeds outB>>1 as
// the next (coarser) LOD's inner box, so the two LODs meet on the EXACT same world lines on ALL FOUR sides,
// regardless of the (fractional) camera position -- no per-LOD Chebyshev parity drift, so no seam gaps. The fine
// outer edge (odd posts have no coarse twin) is decimated onto the coarse segment by the geomorph -> watertight.
// hasInner=false for the finest LOD (draws solid to the centre). Returns via outB; also *pOuterPost for legacy.
static void DrawLodPatch(RViewPoint* vp, int LOD, int loLOD, const D3DVECTOR& cp,
                         const int inB[4], bool hasInner, int outB[4], int* pOuterPost)
{
    const int avail = vp->GetAvailablePostRange(LOD);
    if (avail <= 2)   // nothing to draw; pass the inner box straight through as the (degenerate) outer box
    {
        if (hasInner) { outB[0]=inB[0]; outB[1]=inB[1]; outB[2]=inB[2]; outB[3]=inB[3]; }
        else          { outB[0]=outB[1]=outB[2]=outB[3]=0; }
        *pOuterPost = 0;
        return;
    }
    const int availSafe = avail - 1;
    int range = availSafe;
    if (range > MAX_RADIUS) range = MAX_RADIUS;
    // #DX12 A5: the SENSOR pass caps the per-LOD post radius (fewer 32x32 chunks -> far fewer texture-grouped
    // DrawTerrainMesh calls). The MAIN world view is left at MAX_RADIUS (unchanged). 0 = no cap.
    if (g_terrainRadiusCap > 0 && range > g_terrainRadiusCap) range = g_terrainRadiusCap;

    range &= ~1;                     // even reach so the snapped-even outer box is symmetric around the centre
    const int   outerPost   = range;
    *pOuterPost = outerPost;

    const bool  useTex      = (LOD <= TheMap.LastNearTexLOD());
    static const bool s_diagNoCrackFix = false;
    const bool  doMorph     = (LOD < loLOD) and not s_diagNoCrackFix;   // coarsest LOD has nothing coarser to morph to

    // Geomorph over the outer MORPH_POSTS-wide ring keyed on posts-to-nearest-OUTER-box-edge: alpha=1 EXACTLY on
    // the box edge (dOut==0) so the fine outer edge is fully decimated onto the coarse posts (watertight seam --
    // grid-locked, guaranteed for every edge post). A WIDER ring makes the per-jump alpha step smaller (the box
    // edge steps 2 posts as the camera moves), so the residual height pop is gentler. (True DX7-style pop-free
    // morphing would need a camera-relative toroidal boundary; that reintroduced seam gaps -- deferred. The
    // native Falcon terrain pops too, so this is acceptable parity.)
    const int   MORPH_POSTS = 10;

    const int centerRow = (int)floorf(cp.x / FeetPerPost) >> LOD;
    const int centerCol = (int)floorf(cp.y / FeetPerPost) >> LOD;

    // --- OUTER box in ABSOLUTE level-LOD posts, snapped to EVEN posts (the coarse LOD+1 grid) on every side.
    // rLo/cLo snap UP to even, rHi/cHi snap DOWN to even, so ALL four edges land on coarse-grid lines. The next
    // coarser LOD's inner box = these >>1, so the seams coincide in world space on all sides (no parity gap).
    int rHi = (centerRow + range) & ~1;
    int rLo = (centerRow - range); if (rLo & 1) rLo++;
    int cHi = (centerCol + range) & ~1;
    int cLo = (centerCol - range); if (cLo & 1) cLo++;
    outB[0] = rLo; outB[1] = rHi; outB[2] = cLo; outB[3] = cHi;
    // --- INNER box (the finer LOD's outer box, halved into this LOD's posts). Quads fully inside it belong to
    // the finer LOD and are skipped here. Finest LOD has no inner box.
    const int irLo = hasInner ? inB[0] : 0, irHi = hasInner ? inB[1] : 0;
    const int icLo = hasInner ? inB[2] : 0, icHi = hasInner ? inB[3] : 0;

    // Artscout - 2026: #78 -- LOD+1 (coarse) indexing for the geomorph target fetch (see TgpuCoarseZ).
    // doMorph guarantees LOD+1 <= loLOD is a rendered/available LOD, so blockLists[LOD+1] is valid.
    const int availC     = doMorph ? (vp->GetAvailablePostRange(LOD + 1) - 1) : 0;
    const int cRow1      = (int)floorf(cp.x / FeetPerPost) >> (LOD + 1);
    const int cCol1      = (int)floorf(cp.y / FeetPerPost) >> (LOD + 1);

    static GCell          grid[GW * GW];
    static void*          qsrv[CHUNK * CHUNK];
    static short          qi[CHUNK * CHUNK];
    static short          qj[CHUNK * CHUNK];
    static bool           qdone[CHUNK * CHUNK];
    static TGpuVert       cverts[CHUNK * CHUNK * 4];
    static unsigned short cidx[CHUNK * CHUNK * 6];

    // iterate chunks across the OUTER box (the box cull below skips the inner-box / out-of-box chunks)
    for (int br = rLo; br < rHi; br += CHUNK)
    {
        for (int bc = cLo; bc < cHi; bc += CHUNK)
        {
            // per-chunk box cull: skip a chunk that either misses the OUTER box or lies wholly INSIDE the inner box
            {
                const int cr0 = br, cr1 = br + CHUNK, cc0 = bc, cc1 = bc + CHUNK;
                if (cr1 <= rLo || cr0 >= rHi || cc1 <= cLo || cc0 >= cHi) continue;             // outside outer box
                if (hasInner && cr0 >= irLo && cr1 <= irHi && cc0 >= icLo && cc1 <= icHi) continue; // wholly in finer LOD
            }

            // --- cache the post grid WITH a 1-post border (cell (gi,gj) -> post (br-1+gi, bc-1+gj)) ---
            for (int gi = 0; gi < GW; ++gi)
            {
                for (int gj = 0; gj < GW; ++gj)
                {
                    const int r = br - 1 + gi, c = bc - 1 + gj;
                    GCell& g = grid[gi * GW + gj];
                    g.x = (float)LEVEL_POST_TO_WORLD(r, LOD) - cp.x;
                    g.y = (float)LEVEL_POST_TO_WORLD(c, LOD) - cp.y;

                    const int dr = (r >= centerRow) ? (r - centerRow) : (centerRow - r);
                    const int dc = (c >= centerCol) ? (c - centerCol) : (centerCol - c);
                    Tpost* p = ((dr > availSafe) || (dc > availSafe)) ? (Tpost*)0 : vp->GetPost(r, c, LOD);
                    if (p)
                    {
                        g.za = p->z;
                        g.tex = (unsigned long)p->texID;
                        g.u = p->u; g.v = p->v; g.d = p->d;
                        float elev = -p->z;
                        float s = 0.45f + elev / 14000.0f;
                        if (s < 0.30f) s = 0.30f; if (s > 1.00f) s = 1.00f;
                        unsigned rr2 = (unsigned)(s * 155.0f), gg = (unsigned)(s * 135.0f), bb = (unsigned)(s * 100.0f);
                        g.col = 0xFF000000u | (rr2 << 16) | (gg << 8) | bb;
                        g.ok = true;
                    }
                    else { g.za = 0.0f; g.tex = 0; g.u = g.v = g.d = 0.0f; g.col = 0xFF3A3228u; g.ok = false; }
                    g.z = g.za - cp.z;   // provisional (no morph); real cells overwrite below
                }
            }

            // --- geomorph: morph each REAL cell's height toward the coarse (even-neighbour) surface ---
            for (int gi = 1; gi <= SIDE; ++gi)
            {
                for (int gj = 1; gj <= SIDE; ++gj)
                {
                    GCell& g = grid[gi * GW + gj];
                    if (!g.ok) continue;
                    const int r = br - 1 + gi, c = bc - 1 + gj;
                    float cz = g.za;
                    if (doMorph)
                    {
                        // posts to the nearest OUTER box edge (0 ON the edge). Only the outer MORPH_POSTS ring
                        // morphs; the inner edge (large dOut) stays full-res to match the finer LOD's decimated edge.
                        int dOut = rHi - r; { int t;
                            t = r - rLo; if (t < dOut) dOut = t;
                            t = cHi - c; if (t < dOut) dOut = t;
                            t = c - cLo; if (t < dOut) dOut = t; }
                        float a = (float)(MORPH_POSTS - dOut) / (float)MORPH_POSTS;   // 1 at edge, 0 at >=MORPH_POSTS inward
                        if (a < 0.0f) a = 0.0f; if (a > 1.0f) a = 1.0f;
                        if (a > 0.0f)   // only morphing cells need the (costlier) coarse target
                        {
                            float czReal;
                            if (TgpuCoarseZ(vp, r, c, LOD + 1, cRow1, cCol1, availC, czReal))
                            {
                                cz = czReal;   // REAL LOD+1 surface -> fine edge lands exactly on the coarse patch
                            }
                            else
                            {
                                // fallback: LOD+1 data not resident here -> reconstruct from fine even-neighbours.
                                const int ro = r & 1, co = c & 1;
                                #define ZAN(GI,GJ) (grid[(GI)*GW+(GJ)].ok ? grid[(GI)*GW+(GJ)].za : g.za)
                                if (ro && !co)      cz = 0.5f  * (ZAN(gi-1,gj)   + ZAN(gi+1,gj));
                                else if (!ro && co) cz = 0.5f  * (ZAN(gi,gj-1)   + ZAN(gi,gj+1));
                                else if (ro && co)  cz = 0.25f * (ZAN(gi-1,gj-1) + ZAN(gi+1,gj-1) + ZAN(gi-1,gj+1) + ZAN(gi+1,gj+1));
                                #undef ZAN
                            }
                            cz = g.za + (cz - g.za) * a;
                        }
                    }
                    // Single-layer box tiling: no LOD overlap, so no depth-push needed (nothing coplanar to fight).
                    g.z = cz - cp.z;
                }
            }

            // --- resolve each valid quad's tile SRV (record its grid cell) ---
            int nq = 0;
            for (int i = 0; i < CHUNK; ++i)
            {
                for (int j = 0; j < CHUNK; ++j)
                {
                    const GCell& a = grid[(i + 1) * GW + (j + 1)];
                    const GCell& b = grid[(i + 1) * GW + (j + 2)];
                    const GCell& d = grid[(i + 2) * GW + (j + 1)];
                    const GCell& e = grid[(i + 2) * GW + (j + 2)];
                    if (!a.ok || !b.ok || !d.ok || !e.ok) continue;
                    // box membership: keep the quad iff it lies fully inside the OUTER box and NOT fully inside the
                    // inner box (the finer LOD draws that). The 4 corner posts are (br+i..br+i+1, bc+j..bc+j+1).
                    {
                        const int R0 = br + i, C0 = bc + j;
                        if (R0 < rLo || R0 + 1 > rHi || C0 < cLo || C0 + 1 > cHi) continue;   // outside outer box
                        if (hasInner && R0 >= irLo && R0 + 1 <= irHi && C0 >= icLo && C0 + 1 <= icHi) continue; // finer LOD's
                    }
                    // Near tiles use the high-res set; far tiles the low-res set. If a NEAR tile isn't resident
                    // yet (the GPU patch reaches past the streamed high-res area), fall back to the low-res FAR
                    // texture of the SAME tile (theater-wide, usually loaded) with the same post UVs -- textured
                    // ground instead of the flat brown fallback that read as a "wall" past the runway. #78.
                    void* srv = useTex ? TheTerrTextures.GetTileSRV((TextureID)a.tex)
                                       : TheFarTextures.GetTileSRV((TextureID)a.tex);
                    if (srv == 0)
                        srv = TheFarTextures.GetTileSRV((TextureID)a.tex);
                    qsrv[nq]  = srv;
                    qi[nq]    = (short)i;
                    qj[nq]    = (short)j;
                    qdone[nq] = false;
                    ++nq;
                }
            }
            if (nq == 0) continue;

            // #91 Stage 1: accumulate quads into the GLOBAL per-SRV bucket instead of drawing per chunk. The
            // whole terrain then emits ONE DrawTerrainMesh per distinct tile texture (flushed in TerrainGpu_Render).
            for (int q0 = 0; q0 < nq; ++q0)
            {
                if (qdone[q0]) continue;
                void* srv = qsrv[q0];
                TBucket& bk = TerrainBucket(srv);
                for (int q = q0; q < nq; ++q)
                {
                    if (qdone[q] || qsrv[q] != srv) continue;
                    qdone[q] = true;
                    const int i = qi[q], j = qj[q];
                    const GCell& a = grid[(i + 1) * GW + (j + 1)];   // (r,c)   tile origin
                    const GCell& b = grid[(i + 1) * GW + (j + 2)];   // (r,c+1) east  (+u)
                    const GCell& d = grid[(i + 2) * GW + (j + 1)];   // (r+1,c) north (-v)
                    const GCell& e = grid[(i + 2) * GW + (j + 2)];
                    const unsigned long c0 = srv ? 0xFFFFFFFFu : a.col;
                    if (bk.v.size() + 4 > 65000) FlushTerrainBucket(bk);   // keep base within the u16 index range
                    const unsigned short base = (unsigned short)bk.v.size();
                    TGpuVert v4[4];
                    v4[0].p[0]=a.x; v4[0].p[1]=a.y; v4[0].p[2]=a.z; v4[0].tu=a.u;     v4[0].tv=a.v;
                    v4[1].p[0]=b.x; v4[1].p[1]=b.y; v4[1].p[2]=b.z; v4[1].tu=a.u+a.d; v4[1].tv=a.v;
                    v4[2].p[0]=d.x; v4[2].p[1]=d.y; v4[2].p[2]=d.z; v4[2].tu=a.u;     v4[2].tv=a.v-a.d;
                    v4[3].p[0]=e.x; v4[3].p[1]=e.y; v4[3].p[2]=e.z; v4[3].tu=a.u+a.d; v4[3].tv=a.v-a.d;
                    for (int k = 0; k < 4; ++k) { v4[k].n[0]=0.0f; v4[k].n[1]=0.0f; v4[k].n[2]=-1.0f; v4[k].col=c0; v4[k].spec=0; bk.v.push_back(v4[k]); }
                    bk.i.push_back((unsigned short)(base+0)); bk.i.push_back((unsigned short)(base+1)); bk.i.push_back((unsigned short)(base+3));
                    bk.i.push_back((unsigned short)(base+0)); bk.i.push_back((unsigned short)(base+3)); bk.i.push_back((unsigned short)(base+2));
                }
            }
        }
    }
    // outB (this LOD's outer box, abs level-LOD posts) and *pOuterPost were filled above.
}

void TerrainGpu_Render(RViewPoint* vp)
{
    if (!g_bGpuTerrain || !vp || !vp->IsReady()) return;
    if (!g_pRenderer || !g_pRenderer->IsValid()) return;

    Tpoint pos;
    vp->GetPos(&pos);
    D3DVECTOR cp; cp.x = pos.x; cp.y = pos.y; cp.z = pos.z;

    g_pRenderer->BeginTerrainPass();
    g_pRenderer->SetProj((const float*)&CDXEngine::GetObjProjection());
    g_pRenderer->SetView((const float*)&CDXEngine::GetObjView());
    g_pRenderer->SetCameraPos(cp.x, cp.y, cp.z);

    const int hiLOD = vp->GetHighLOD();
    const int loLOD = vp->GetLowLOD();

    // #91 Stage 1: DrawLodPatch now ACCUMULATES quads per tile-texture instead of drawing per chunk. Empty the
    // buckets, run all LODs (fills buckets), then flush ONE DrawTerrainMesh per texture. (SetTerrainRasterForLod
    // is currently a no-op, so a single render state across all LODs is fine; per-LOD bias would need per-LOD
    // buckets -- a Stage-2 concern.)
    TerrainBatchBegin();
    // Artscout - 2026: #78 -- thread the LOD seam as a per-side BOX in absolute posts. DrawLodPatch returns its
    // outer box (rLo,rHi,cLo,cHi, abs level-LOD posts, snapped to even = coarse-grid lines); the next (coarser,
    // half-res) LOD's inner box is exactly that >> 1, so the seams coincide in WORLD space on all four sides for
    // ANY camera position (no Chebyshev-parity gap). hasInner=false for the finest LOD (draws to the centre).
    int inB[4] = { 0, 0, 0, 0 };
    bool hasInner = false;
    for (int LOD = hiLOD; LOD <= loLOD; ++LOD)
    {
        g_pRenderer->SetTerrainRasterForLod(LOD - hiLOD);   // #78 per-LOD depth bias (0 = finest)
        int outB[4], outerPost;
        DrawLodPatch(vp, LOD, loLOD, cp, inB, hasInner, outB, &outerPost);
        // coarser LOD's posts are 2x wider -> its inner box is this outer box halved (even edges -> exact)
        inB[0] = outB[0] >> 1; inB[1] = outB[1] >> 1; inB[2] = outB[2] >> 1; inB[3] = outB[3] >> 1;
        hasInner = true;
    }
    TerrainBatchFlushAll();   // #91: emit one DrawTerrainMesh per distinct tile texture (was ~1 per chunk)
    // #78 leave slot 0 empty so later sky/sun billboards don't inherit a leftover ground tile (a road
    // texture was showing inside the sun's quad). The sun billboard's own binding is a separate bug.
    g_pRenderer->SetTexture(0, 0);
}
