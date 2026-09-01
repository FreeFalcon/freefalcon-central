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
#include <vector> // #91: per-SRV terrain batch accumulation
#include <chrono> // #107 PERF: terrain accumulate/flush split timing
#include "graphics/include/frameprof.h" // #107 PERF: FrameProf_* (backend-neutral)
#include <map>

#include "ttypes.h" // POST macros, FeetPerPost
#include "tmap.h" // TheMap.LastNearTexLOD()
#include "tpost.h" // Tpost { z, texID, u,v,d, ... }
#include "terrtex.h" // TheTerrTextures.GetTileSRV()
#include "rviewpnt.h" // RViewPoint
#include "tod.h" // Artscout - 2026: #97 -- TheTimeOfDay (moon elevation/phase drives the night ground floor)
#include "graphics/dxengine/dxengine.h" // CDXEngine::GetObjProjection/View/CameraPos
#include "graphics/dxengine/common/irenderer.h" // g_pRenderer
#include "terraingpu.h"
#include "terrainclipmap.h" // #78: the mesh-shader path's data side

extern bool g_bGpuTerrain;

// Artscout - 2026: day/night brightness for the GPU terrain (the vertex colours carry no lighting). 1 = full day,
// floored so a moonlit night isn't pitch black. Set per-frame in TerrainGpu_Render from the sun's diffuse level.
static float s_terrainDayNight = 1.0f;

// Object-layout vertex: must match ObjV / D3D11VertexEx (40 bytes) exactly.
struct TGpuVert
{
    float p[3];
    float n[3];
    unsigned int
        col; // 32-bit D3DCOLOR -- MUST be int, not long. long is 8 bytes on LP64 (Linux), which shifts
    unsigned int
        spec; // tu/tv and the stride off the layout the vertex pipeline/shader expect -> untextured,
    // wrong-coloured terrain (worked on Windows where long is 4 bytes). LP64 GPU-struct bug.
    float tu, tv;
};
// LP64 guard: the vertex pipeline reads a fixed byte layout -- lock the size so a stray `long` (8 bytes on Linux)
// breaks the build on BOTH platforms instead of silently shifting the layout (the bug that untextured the terrain).
static_assert(
    sizeof(TGpuVert) == 40,
    "TGpuVert must stay 40 bytes (ObjV layout) -- check for a 64-bit member");

// #107 PERF bindless terrain vertex: ObjV (40 bytes) + a uint tile slot at offset 40 -> 44 bytes. Must match the
// bindless object pipeline's vertex input (VulkanRenderer GetObjPipe, bindless=true) and ffobject.vert location 5.
// When the renderer offers the bindless path, EVERY terrain quad -- regardless of tile -- goes into ONE stream and
// carries its own slot, so the whole ground draws in 1-2 calls instead of one per distinct tile texture.
struct TGpuVertBindless
{
    float p[3];
    float n[3];
    unsigned int
        col; // MUST be int, not long: long is 8 bytes on LP64 (Linux) -> shifts tu/tv/texIndex and the
    unsigned int
        spec; // stride off the 44-byte layout the bindless vertex pipeline expects. Same LP64 fix as TGpuVert.
    float tu, tv;
    unsigned int texIndex;
};
static_assert(
    sizeof(TGpuVertBindless) == 44,
    "TGpuVertBindless must stay 44 bytes (bindless pipeline vertex input)");

static const int CHUNK = 32;
static const int SIDE = CHUNK + 1;
static const int GW =
    SIDE + 2; // grid width WITH a 1-post border (for geomorph neighbours)
static const int MAX_RADIUS = 96;

// Artscout - 2026: #DX12 A5 -- per-render terrain radius cap. The GPU terrain draws one DrawTerrainMesh per
// distinct tile texture per 32x32 chunk out to MAX_RADIUS=96 posts -> ~thousands of small draws per view. That
// is fine (barely) for ONE view, but the TGP/Maverick/FLIR sensor renders a SECOND full-radius view into the
// atlas every frame -> ~9000 draws/frame -> the GPU misses the TDR deadline (DEVICE_HUNG), worst while slewing.
// A zoomed narrow-FOV sensor does not need the full 96-post radius; the sensor pass sets a small cap so it draws
// far fewer chunks. 0 = no cap (the main world view). Set via FF_SetTerrainRadiusCap around the sensor DrawScene.
int g_terrainRadiusCap = 0;
void FF_SetTerrainRadiusCap(int r)
{
    g_terrainRadiusCap = r;
}

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
namespace
{
struct TBucket
{
    void* srv;
    std::vector<TGpuVert> v;
    std::vector<unsigned short> i;
};
static std::vector<TBucket*>
    s_buckets; // heap-stable buckets (reused across frames; contents cleared)
static std::map<void*, int> s_srvToBucket;

static TBucket& TerrainBucket(void* srv)
{
    std::map<void*, int>::iterator it = s_srvToBucket.find(srv);
    if (it != s_srvToBucket.end())
        return *s_buckets[it->second];
    int idx = (int)s_buckets.size();
    s_srvToBucket[srv] = idx;
    s_buckets.push_back(new TBucket());
    s_buckets[idx]->srv = srv;
    return *s_buckets[idx];
}
static void FlushTerrainBucket(TBucket& b)
{
    if (!b.v.empty() && !b.i.empty())
    {
        g_pRenderer->SetTexture(0, (struct ID3D11ShaderResourceView*)b.srv),
            g_pRenderer->DrawTerrainMesh(&b.v[0], (int)b.v.size(), &b.i[0],
                                         (int)b.i.size());
        FrameProf_CountTerrainDraw(); // #107 PERF: count terrain bucket draws
    }
    b.v.clear();
    b.i.clear();
}
static void TerrainBatchBegin()
{
    for (size_t k = 0; k < s_buckets.size(); ++k)
    {
        s_buckets[k]->v.clear();
        s_buckets[k]->i.clear();
    }
}
static void TerrainBatchFlushAll()
{
    for (size_t k = 0; k < s_buckets.size(); ++k)
        FlushTerrainBucket(*s_buckets[k]);
}

// #107 PERF: the bindless path. One global stream tagged with per-vertex tile slots -> one DrawTerrainMeshBindless
// (flushed early only when the u16 index range would overflow). s_bindless mirrors g_pRenderer->BindlessTerrainActive()
// for the frame so the hot emission loop below is a single branch, not a virtual call per quad.
static bool s_bindless = false;
static std::vector<TGpuVertBindless> s_biVerts;
static std::vector<unsigned short> s_biIdx;
static void BindlessFlush()
{
    if (!s_biVerts.empty() && !s_biIdx.empty())
    {
        g_pRenderer->DrawTerrainMeshBindless(&s_biVerts[0],
                                             (int)s_biVerts.size(), &s_biIdx[0],
                                             (int)s_biIdx.size());
        FrameProf_CountTerrainDraw();
    }
    s_biVerts.clear();
    s_biIdx.clear();
}
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
static bool TgpuCoarseZ(RViewPoint* vp, int r, int c, int LOD1, int cR, int cC,
                        int as, float& out)
{
#define TGPU_FETCH(R, C, Z)                                                    \
    do                                                                         \
    {                                                                          \
        const int _dR = ((R) >= cR) ? ((R) - cR) : (cR - (R));                 \
        const int _dC = ((C) >= cC) ? ((C) - cC) : (cC - (C));                 \
        if (_dR > as || _dC > as)                                              \
            return false;                                                      \
        Tpost* _p = vp->GetPost((R), (C), LOD1);                               \
        if (!_p)                                                               \
            return false;                                                      \
        (Z) = _p->z;                                                           \
    } while (0)
    const int ro = r & 1, co = c & 1;
    if (!ro && !co)
    {
        float z;
        TGPU_FETCH(r >> 1, c >> 1, z);
        out = z;
        return true;
    }
    if (ro && !co)
    {
        const int R0 = (r - 1) >> 1;
        float z0, z1;
        TGPU_FETCH(R0, c >> 1, z0);
        TGPU_FETCH(R0 + 1, c >> 1, z1);
        out = 0.5f * (z0 + z1);
        return true;
    }
    if (!ro && co)
    {
        const int C0 = (c - 1) >> 1;
        float z0, z1;
        TGPU_FETCH(r >> 1, C0, z0);
        TGPU_FETCH(r >> 1, C0 + 1, z1);
        out = 0.5f * (z0 + z1);
        return true;
    }
    {
        const int R0 = (r - 1) >> 1, C0 = (c - 1) >> 1;
        float z0, z1, z2, z3;
        TGPU_FETCH(R0, C0, z0);
        TGPU_FETCH(R0 + 1, C0, z1);
        TGPU_FETCH(R0, C0 + 1, z2);
        TGPU_FETCH(R0 + 1, C0 + 1, z3);
        out = 0.25f * (z0 + z1 + z2 + z3);
        return true;
    }
#undef TGPU_FETCH
}

// Artscout - 2026: #78 -- connector/single-layer LOD tiling. Each LOD occupies an axis-aligned post RING whose
// OUTER edges are snapped to EVEN absolute posts (== the coarse LOD+1 grid: coarse post k == fine post 2k). The
// outer edges (rLo,rHi,cLo,cHi in ABSOLUTE level-LOD posts) are returned in outB[4]; the caller feeds outB>>1 as
// the next (coarser) LOD's inner box, so the two LODs meet on the EXACT same world lines on ALL FOUR sides,
// regardless of the (fractional) camera position -- no per-LOD Chebyshev parity drift, so no seam gaps. The fine
// outer edge (odd posts have no coarse twin) is decimated onto the coarse segment by the geomorph -> watertight.
// hasInner=false for the finest LOD (draws solid to the centre). Returns via outB; also *pOuterPost for legacy.
static void DrawLodPatch(RViewPoint* vp, int LOD, int loLOD,
                         const D3DVECTOR& cp, const int inB[4], bool hasInner,
                         int outB[4], int* pOuterPost)
{
    const int avail = vp->GetAvailablePostRange(LOD);
    if (avail <=
        2) // nothing to draw; pass the inner box straight through as the (degenerate) outer box
    {
        if (hasInner)
        {
            outB[0] = inB[0];
            outB[1] = inB[1];
            outB[2] = inB[2];
            outB[3] = inB[3];
        }
        else
        {
            outB[0] = outB[1] = outB[2] = outB[3] = 0;
        }
        *pOuterPost = 0;
        return;
    }
    const int availSafe = avail - 1;
    int range = availSafe;
    if (range > MAX_RADIUS)
        range = MAX_RADIUS;
    // #DX12 A5: the SENSOR pass caps the per-LOD post radius (fewer 32x32 chunks -> far fewer texture-grouped
    // DrawTerrainMesh calls). The MAIN world view is left at MAX_RADIUS (unchanged). 0 = no cap.
    if (g_terrainRadiusCap > 0 && range > g_terrainRadiusCap)
        range = g_terrainRadiusCap;

    range &=
        ~1; // even reach so the snapped-even outer box is symmetric around the centre
    const int outerPost = range;
    *pOuterPost = outerPost;

    const bool useTex = (LOD <= TheMap.LastNearTexLOD());
    static const bool s_diagNoCrackFix = false;
    const bool doMorph =
        (LOD < loLOD) and
        not s_diagNoCrackFix; // coarsest LOD has nothing coarser to morph to

    // Geomorph over the outer MORPH_POSTS-wide ring keyed on posts-to-nearest-OUTER-box-edge: alpha=1 EXACTLY on
    // the box edge (dOut==0) so the fine outer edge is fully decimated onto the coarse posts (watertight seam --
    // grid-locked, guaranteed for every edge post). A WIDER ring makes the per-jump alpha step smaller (the box
    // edge steps 2 posts as the camera moves), so the residual height pop is gentler. (True DX7-style pop-free
    // morphing would need a camera-relative toroidal boundary; that reintroduced seam gaps -- deferred. The
    // native Falcon terrain pops too, so this is acceptable parity.)
    const int MORPH_POSTS = 10;

    const int centerRow = (int)floorf(cp.x / FeetPerPost) >> LOD;
    const int centerCol = (int)floorf(cp.y / FeetPerPost) >> LOD;

    // --- OUTER box in ABSOLUTE level-LOD posts, snapped to EVEN posts (the coarse LOD+1 grid) on every side.
    // rLo/cLo snap UP to even, rHi/cHi snap DOWN to even, so ALL four edges land on coarse-grid lines. The next
    // coarser LOD's inner box = these >>1, so the seams coincide in world space on all sides (no parity gap).
    int rHi = (centerRow + range) & ~1;
    int rLo = (centerRow - range);
    if (rLo & 1)
        rLo++;
    int cHi = (centerCol + range) & ~1;
    int cLo = (centerCol - range);
    if (cLo & 1)
        cLo++;
    outB[0] = rLo;
    outB[1] = rHi;
    outB[2] = cLo;
    outB[3] = cHi;
    // --- INNER box (the finer LOD's outer box, halved into this LOD's posts). Quads fully inside it belong to
    // the finer LOD and are skipped here. Finest LOD has no inner box.
    const int irLo = hasInner ? inB[0] : 0, irHi = hasInner ? inB[1] : 0;
    const int icLo = hasInner ? inB[2] : 0, icHi = hasInner ? inB[3] : 0;

    // Artscout - 2026: #78 -- LOD+1 (coarse) indexing for the geomorph target fetch (see TgpuCoarseZ).
    // doMorph guarantees LOD+1 <= loLOD is a rendered/available LOD, so blockLists[LOD+1] is valid.
    const int availC = doMorph ? (vp->GetAvailablePostRange(LOD + 1) - 1) : 0;
    const int cRow1 = (int)floorf(cp.x / FeetPerPost) >> (LOD + 1);
    const int cCol1 = (int)floorf(cp.y / FeetPerPost) >> (LOD + 1);

    static GCell grid[GW * GW];
    static void* qsrv[CHUNK * CHUNK];
    static short qi[CHUNK * CHUNK];
    static short qj[CHUNK * CHUNK];
    static bool qdone[CHUNK * CHUNK];
    static TGpuVert cverts[CHUNK * CHUNK * 4];
    static unsigned short cidx[CHUNK * CHUNK * 6];

    // iterate chunks across the OUTER box (the box cull below skips the inner-box / out-of-box chunks)
    for (int br = rLo; br < rHi; br += CHUNK)
    {
        for (int bc = cLo; bc < cHi; bc += CHUNK)
        {
            // per-chunk box cull: skip a chunk that either misses the OUTER box or lies wholly INSIDE the inner box
            {
                const int cr0 = br, cr1 = br + CHUNK, cc0 = bc,
                          cc1 = bc + CHUNK;
                if (cr1 <= rLo || cr0 >= rHi || cc1 <= cLo || cc0 >= cHi)
                    continue; // outside outer box
                if (hasInner && cr0 >= irLo && cr1 <= irHi && cc0 >= icLo &&
                    cc1 <= icHi)
                    continue; // wholly in finer LOD
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

                    const int dr =
                        (r >= centerRow) ? (r - centerRow) : (centerRow - r);
                    const int dc =
                        (c >= centerCol) ? (c - centerCol) : (centerCol - c);
                    Tpost* p = ((dr > availSafe) || (dc > availSafe)) ?
                                   (Tpost*)0 :
                                   vp->GetPost(r, c, LOD);
                    if (p)
                    {
                        g.za = p->z;
                        g.tex = (unsigned long)p->texID;
                        g.u = p->u;
                        g.v = p->v;
                        g.d = p->d;
                        float elev = -p->z;
                        float s = 0.45f + elev / 14000.0f;
                        if (s < 0.30f)
                            s = 0.30f;
                        if (s > 1.00f)
                            s = 1.00f;
                        // Artscout - 2026: day/night -- the GPU terrain colours vertices by elevation with NO lighting,
                        // so it stayed full-bright at midnight. Modulate by the sun's diffuse level (s_terrainDayNight,
                        // floored so moonlit ground isn't pure black).
                        s *= s_terrainDayNight;
                        unsigned rr2 = (unsigned)(s * 155.0f),
                                 gg = (unsigned)(s * 135.0f),
                                 bb = (unsigned)(s * 100.0f);
                        g.col = 0xFF000000u | (rr2 << 16) | (gg << 8) | bb;
                        g.ok = true;
                    }
                    else
                    {
                        g.za = 0.0f;
                        g.tex = 0;
                        g.u = g.v = g.d = 0.0f;
                        g.col = 0xFF3A3228u;
                        g.ok = false;
                    }
                    g.z =
                        g.za -
                        cp.z; // provisional (no morph); real cells overwrite below
                }
            }

            // --- geomorph: morph each REAL cell's height toward the coarse (even-neighbour) surface ---
            for (int gi = 1; gi <= SIDE; ++gi)
            {
                for (int gj = 1; gj <= SIDE; ++gj)
                {
                    GCell& g = grid[gi * GW + gj];
                    if (!g.ok)
                        continue;
                    const int r = br - 1 + gi, c = bc - 1 + gj;
                    float cz = g.za;
                    if (doMorph)
                    {
                        // posts to the nearest OUTER box edge (0 ON the edge). Only the outer MORPH_POSTS ring
                        // morphs; the inner edge (large dOut) stays full-res to match the finer LOD's decimated edge.
                        int dOut = rHi - r;
                        {
                            int t;
                            t = r - rLo;
                            if (t < dOut)
                                dOut = t;
                            t = cHi - c;
                            if (t < dOut)
                                dOut = t;
                            t = c - cLo;
                            if (t < dOut)
                                dOut = t;
                        }
                        float a =
                            (float)(MORPH_POSTS - dOut) /
                            (float)
                                MORPH_POSTS; // 1 at edge, 0 at >=MORPH_POSTS inward
                        if (a < 0.0f)
                            a = 0.0f;
                        if (a > 1.0f)
                            a = 1.0f;
                        if (a >
                            0.0f) // only morphing cells need the (costlier) coarse target
                        {
                            float czReal;
                            if (TgpuCoarseZ(vp, r, c, LOD + 1, cRow1, cCol1,
                                            availC, czReal))
                            {
                                cz =
                                    czReal; // REAL LOD+1 surface -> fine edge lands exactly on the coarse patch
                            }
                            else
                            {
                                // fallback: LOD+1 data not resident here -> reconstruct from fine even-neighbours.
                                const int ro = r & 1, co = c & 1;
#define ZAN(GI, GJ)                                                            \
    (grid[(GI) * GW + (GJ)].ok ? grid[(GI) * GW + (GJ)].za : g.za)
                                if (ro && !co)
                                    cz = 0.5f *
                                         (ZAN(gi - 1, gj) + ZAN(gi + 1, gj));
                                else if (!ro && co)
                                    cz = 0.5f *
                                         (ZAN(gi, gj - 1) + ZAN(gi, gj + 1));
                                else if (ro && co)
                                    cz = 0.25f * (ZAN(gi - 1, gj - 1) +
                                                  ZAN(gi + 1, gj - 1) +
                                                  ZAN(gi - 1, gj + 1) +
                                                  ZAN(gi + 1, gj + 1));
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
                    if (!a.ok || !b.ok || !d.ok || !e.ok)
                        continue;
                    // box membership: keep the quad iff it lies fully inside the OUTER box and NOT fully inside the
                    // inner box (the finer LOD draws that). The 4 corner posts are (br+i..br+i+1, bc+j..bc+j+1).
                    {
                        const int R0 = br + i, C0 = bc + j;
                        if (R0 < rLo || R0 + 1 > rHi || C0 < cLo ||
                            C0 + 1 > cHi)
                            continue; // outside outer box
                        if (hasInner && R0 >= irLo && R0 + 1 <= irHi &&
                            C0 >= icLo && C0 + 1 <= icHi)
                            continue; // finer LOD's
                    }
                    // Near tiles use the high-res set; far tiles the low-res set. If a NEAR tile isn't resident
                    // yet (the GPU patch reaches past the streamed high-res area), fall back to the low-res FAR
                    // texture of the SAME tile (theater-wide, usually loaded) with the same post UVs -- textured
                    // ground instead of the flat brown fallback that read as a "wall" past the runway. #78.
                    void* srv =
                        useTex ? TheTerrTextures.GetTileSRV((TextureID)a.tex) :
                                 TheFarTextures.GetTileSRV((TextureID)a.tex);
                    if (srv == 0)
                        srv = TheFarTextures.GetTileSRV((TextureID)a.tex);
                    qsrv[nq] = srv;
                    qi[nq] = (short)i;
                    qj[nq] = (short)j;
                    qdone[nq] = false;
                    ++nq;
                }
            }
            if (nq == 0)
                continue;

            // #107 PERF: bindless path. No per-SRV grouping -- every quad goes into ONE stream with a per-vertex tile
            // slot, so the whole ground draws in 1-2 calls (vs one per distinct texture). Geometry is identical to the
            // grouped path below; only the destination (one stream + texIndex) differs.
            if (s_bindless)
            {
                for (int q = 0; q < nq; ++q)
                {
                    const int i = qi[q], j = qj[q];
                    void* srv = qsrv[q];
                    const unsigned int ti = g_pRenderer->BindlessTexIndex(
                        (struct ID3D11ShaderResourceView*)srv);
                    const GCell& a = grid[(i + 1) * GW + (j + 1)];
                    const GCell& b = grid[(i + 1) * GW + (j + 2)];
                    const GCell& d = grid[(i + 2) * GW + (j + 1)];
                    const GCell& e = grid[(i + 2) * GW + (j + 2)];
                    // texIndex 0xFFFFFFFF (tile not resident in the array / bindless full) -> the frag falls through to
                    // gTex0; SetTexture(0) isn't called on this path, so it samples white -> the flat elevation colour
                    // still reads via the vertex colour. Keep the white-modulate/flat split identical to the grouped path.
                    const unsigned dnB = (unsigned)(s_terrainDayNight * 255.0f);
                    const unsigned long c0 =
                        srv ? (0xFF000000u | (dnB << 16) | (dnB << 8) | dnB) :
                              a.col;
                    if (s_biVerts.size() + 4 > 65000)
                        BindlessFlush(); // keep base within the u16 index range
                    const unsigned short base =
                        (unsigned short)s_biVerts.size();
                    TGpuVertBindless v4[4];
                    v4[0].p[0] = a.x;
                    v4[0].p[1] = a.y;
                    v4[0].p[2] = a.z;
                    v4[0].tu = a.u;
                    v4[0].tv = a.v;
                    v4[1].p[0] = b.x;
                    v4[1].p[1] = b.y;
                    v4[1].p[2] = b.z;
                    v4[1].tu = a.u + a.d;
                    v4[1].tv = a.v;
                    v4[2].p[0] = d.x;
                    v4[2].p[1] = d.y;
                    v4[2].p[2] = d.z;
                    v4[2].tu = a.u;
                    v4[2].tv = a.v - a.d;
                    v4[3].p[0] = e.x;
                    v4[3].p[1] = e.y;
                    v4[3].p[2] = e.z;
                    v4[3].tu = a.u + a.d;
                    v4[3].tv = a.v - a.d;
                    for (int k = 0; k < 4; ++k)
                    {
                        v4[k].n[0] = 0.0f;
                        v4[k].n[1] = 0.0f;
                        v4[k].n[2] = -1.0f;
                        v4[k].col = c0;
                        v4[k].spec = 0;
                        v4[k].texIndex = ti;
                        s_biVerts.push_back(v4[k]);
                    }
                    s_biIdx.push_back((unsigned short)(base + 0));
                    s_biIdx.push_back((unsigned short)(base + 1));
                    s_biIdx.push_back((unsigned short)(base + 3));
                    s_biIdx.push_back((unsigned short)(base + 0));
                    s_biIdx.push_back((unsigned short)(base + 3));
                    s_biIdx.push_back((unsigned short)(base + 2));
                }
                continue; // next chunk; skip the per-SRV grouped accumulation
            }

            // #91 Stage 1: accumulate quads into the GLOBAL per-SRV bucket instead of drawing per chunk. The
            // whole terrain then emits ONE DrawTerrainMesh per distinct tile texture (flushed in TerrainGpu_Render).
            for (int q0 = 0; q0 < nq; ++q0)
            {
                if (qdone[q0])
                    continue;
                void* srv = qsrv[q0];
                TBucket& bk = TerrainBucket(srv);
                for (int q = q0; q < nq; ++q)
                {
                    if (qdone[q] || qsrv[q] != srv)
                        continue;
                    qdone[q] = true;
                    const int i = qi[q], j = qj[q];
                    const GCell& a =
                        grid[(i + 1) * GW + (j + 1)]; // (r,c)   tile origin
                    const GCell& b =
                        grid[(i + 1) * GW + (j + 2)]; // (r,c+1) east  (+u)
                    const GCell& d =
                        grid[(i + 2) * GW + (j + 1)]; // (r+1,c) north (-v)
                    const GCell& e = grid[(i + 2) * GW + (j + 2)];
                    // Textured tiles: white modulate = texture as-is; scale white by day/night so the textured ground
                    // also darkens at night (texture * grey). Flat tiles already baked the factor into a.col.
                    const unsigned dnB = (unsigned)(s_terrainDayNight * 255.0f);
                    const unsigned long c0 =
                        srv ? (0xFF000000u | (dnB << 16) | (dnB << 8) | dnB) :
                              a.col;
                    if (bk.v.size() + 4 > 65000)
                        FlushTerrainBucket(
                            bk); // keep base within the u16 index range
                    const unsigned short base = (unsigned short)bk.v.size();
                    TGpuVert v4[4];
                    v4[0].p[0] = a.x;
                    v4[0].p[1] = a.y;
                    v4[0].p[2] = a.z;
                    v4[0].tu = a.u;
                    v4[0].tv = a.v;
                    v4[1].p[0] = b.x;
                    v4[1].p[1] = b.y;
                    v4[1].p[2] = b.z;
                    v4[1].tu = a.u + a.d;
                    v4[1].tv = a.v;
                    v4[2].p[0] = d.x;
                    v4[2].p[1] = d.y;
                    v4[2].p[2] = d.z;
                    v4[2].tu = a.u;
                    v4[2].tv = a.v - a.d;
                    v4[3].p[0] = e.x;
                    v4[3].p[1] = e.y;
                    v4[3].p[2] = e.z;
                    v4[3].tu = a.u + a.d;
                    v4[3].tv = a.v - a.d;
                    for (int k = 0; k < 4; ++k)
                    {
                        v4[k].n[0] = 0.0f;
                        v4[k].n[1] = 0.0f;
                        v4[k].n[2] = -1.0f;
                        v4[k].col = c0;
                        v4[k].spec = 0;
                        bk.v.push_back(v4[k]);
                    }
                    bk.i.push_back((unsigned short)(base + 0));
                    bk.i.push_back((unsigned short)(base + 1));
                    bk.i.push_back((unsigned short)(base + 3));
                    bk.i.push_back((unsigned short)(base + 0));
                    bk.i.push_back((unsigned short)(base + 3));
                    bk.i.push_back((unsigned short)(base + 2));
                }
            }
        }
    }
    // outB (this LOD's outer box, abs level-LOD posts) and *pOuterPost were filled above.
}

void TerrainGpu_Render(RViewPoint* vp)
{
    if (!g_bGpuTerrain || !vp || !vp->IsReady())
        return;
    if (!g_pRenderer || !g_pRenderer->IsValid())
        return;
    // Artscout - 2026 (#104): the Vulkan freeze breaks in this GPU-terrain path; gate it off under Vulkan (flag
    // default OFF) to isolate it and get a usable cockpit/objects/sky 3D. Re-enable via g_bVulkanTerrain once fixed.
    {
        extern bool g_bUseVulkan, g_bVulkanTerrain;
        if (g_bUseVulkan && !g_bVulkanTerrain)
            return;
    }

    Tpoint pos;
    vp->GetPos(&pos);
    D3DVECTOR cp;
    cp.x = pos.x;
    cp.y = pos.y;
    cp.z = pos.z;

    // Day/night: the sun's diffuse level (updated per-frame from the time of day). Luminance -> [floor..1]. The
    // terrain vertex colours below multiply by this.
    // Artscout - 2026: #97 -- the night FLOOR is now driven by the ACTUAL moon, not a fixed constant. Old code
    // clamped to a flat 0.10 so every night looked "moonlit" even with no moon up. Now: moon light =
    // elevation(0..1) * phase(0 new .. 1 full). A moonless / below-horizon / new-moon night falls to a faint
    // starlight floor (near black); a full moon high in the sky lifts the ground to a soft silver wash. This is
    // the slight ground illumination the moon should cast (the moon disc itself is drawn self-lit in otwsky).
    // Artscout - 2026: the sensor pass swaps the scene light for the TV/IR lamp (diffuse 0), so recomputing
    // there read a black sun and dimmed the sensor ground to the night floor. Keep the main view's level.
    if (!g_pRenderer->IsIRGrey())
    {
        const D3DCOLORVALUE& sd = CDXEngine::TheSun.dcvDiffuse;
        float lum = (sd.r + sd.g + sd.b) * (1.0f / 3.0f);

        float moonLight = 0.0f;
        if (TheTimeOfDay.ThereIsAMoon())
        {
            Tpoint md;
            TheTimeOfDay.CalculateSunMoonPos(&md, TRUE);
            float up = -md.z; // world z = down -> up component of the moon dir
            if (up < 0.0f)
                up = 0.0f;
            if (up > 1.0f)
                up = 1.0f; // 0 at/under the horizon .. 1 at the zenith
            float phase = (float)abs(TheTimeOfDay.CalculateMoonPercent() -
                                     NEW_MOON_PHASE) /
                          (float)NEW_MOON_PHASE;
            moonLight =
                up * phase; // 0 (new moon / down) .. 1 (full moon, overhead)
        }
        const float STAR_FLOOR =
            0.012f; // starlight -- darker moonless night (was 0.02; user wanted deeper night)
        const float MOON_MAX =
            0.16f; // full moon high overhead -> soft silver ground
        float night = STAR_FLOOR + moonLight * (MOON_MAX - STAR_FLOOR);

        if (lum < night)
            lum = night;
        if (lum > 1.0f)
            lum = 1.0f;

        // Artscout - 2026: #97 NVG -- image intensifiers amplify ambient light hugely. Without lifting here the
        // night-darkened terrain (~0.012) reads as near-black even after the PS greens it. Force it BRIGHT under
        // NVG so the goggle image shows a lit green landscape (the FF_NVG PS supplies the green tint + gain curve).
        extern bool bNVGmode;
        if (bNVGmode and lum < 0.75f)
            lum = 0.75f;

        s_terrainDayNight = lum;
    }

    // Artscout - 2026 (#78): the mesh-shader path replaces this function's whole
    // CPU grid build -- the clipmap streams posts, one DispatchMesh draws them.
    {
        extern bool g_bTerrainMeshShader;
        if (g_bTerrainMeshShader and g_pRenderer->MeshTerrainAvailable())
        {
            // Arm the tile-activation budget BEFORE streaming posts: GetTileSRV
            // activates a tile only while it lasts, and a post that misses out
            // keeps "no tile" until the next re-scan -- the brown underlay.
            {
                extern int g_nTileActivatePerFrame,
                    g_nTileActivateMeshPerFrame, g_nTileActivateBudget;
                // The larger of the two: a raised legacy knob must not be
                // silently ignored just because this path has its own.
                int budget = (g_nTileActivateMeshPerFrame > 0) ?
                                 g_nTileActivateMeshPerFrame :
                                 0;
                if (g_nTileActivatePerFrame > budget)
                    budget = g_nTileActivatePerFrame;
                g_nTileActivateBudget = (budget > 0) ? budget : 0x7FFFFFFF;
            }
            const float cam[3] = {cp.x, cp.y, cp.z};
            if (TerrainClipmap_Update(vp, cam, s_terrainDayNight))
            {
                const TerrainClipConstants& cb = TerrainClipmap_Constants();
                g_pRenderer->DrawTerrainMeshShader(
                    &cb, (int)sizeof(cb), TerrainClipmap_ChunkCount());
            }
            return;
        }
    }

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
    extern bool g_bVulkanProfile;
    const bool _tprof = g_bVulkanProfile;
    std::chrono::steady_clock::time_point _tp0;
    if (_tprof)
        _tp0 = std::chrono::steady_clock::now();
    // #107 PERF: pick the bindless path once per frame (Vulkan with descriptor-indexing). D3D12 and Vulkan without the
    // extension report false -> the untouched per-SRV bucket path runs. Cheaper to cache the bool than to virtual-call
    // BindlessTerrainActive() per quad.
    s_bindless = g_pRenderer->BindlessTerrainActive();
    // #107 PERF spike fix: arm the per-render tile-Activate budget (see g_nTileActivatePerFrame in f4config).
    {
        extern int g_nTileActivatePerFrame, g_nTileActivateBudget;
        g_nTileActivateBudget = (g_nTileActivatePerFrame > 0) ?
                                    g_nTileActivatePerFrame :
                                    0x7FFFFFFF;
    }
    TerrainBatchBegin();
    if (s_bindless)
    {
        s_biVerts.clear();
        s_biIdx.clear();
    }
    // Artscout - 2026: #78 -- thread the LOD seam as a per-side BOX in absolute posts. DrawLodPatch returns its
    // outer box (rLo,rHi,cLo,cHi, abs level-LOD posts, snapped to even = coarse-grid lines); the next (coarser,
    // half-res) LOD's inner box is exactly that >> 1, so the seams coincide in WORLD space on all four sides for
    // ANY camera position (no Chebyshev-parity gap). hasInner=false for the finest LOD (draws to the centre).
    int inB[4] = {0, 0, 0, 0};
    bool hasInner = false;
    for (int LOD = hiLOD; LOD <= loLOD; ++LOD)
    {
        g_pRenderer->SetTerrainRasterForLod(
            LOD - hiLOD); // #78 per-LOD depth bias (0 = finest)
        int outB[4], outerPost;
        DrawLodPatch(vp, LOD, loLOD, cp, inB, hasInner, outB, &outerPost);
        // coarser LOD's posts are 2x wider -> its inner box is this outer box halved (even edges -> exact)
        inB[0] = outB[0] >> 1;
        inB[1] = outB[1] >> 1;
        inB[2] = outB[2] >> 1;
        inB[3] = outB[3] >> 1;
        hasInner = true;
    }
    if (_tprof)
    {
        auto _n = std::chrono::steady_clock::
            now(); // #107 PERF: accumulate (CPU grid build) done
        FrameProf_TerrAcc(
            std::chrono::duration<double, std::milli>(_n - _tp0).count());
        _tp0 = _n;
    }
    if (s_bindless)
        BindlessFlush(); // #107: emit the whole ground in 1-2 bindless draws
    else
        TerrainBatchFlushAll(); // #91: emit one DrawTerrainMesh per distinct tile texture (was ~1 per chunk)
    if (_tprof)
        FrameProf_TerrFlush(std::chrono::duration<double, std::milli>(
                                std::chrono::steady_clock::now() - _tp0)
                                .count());
    // #78 leave slot 0 empty so later sky/sun billboards don't inherit a leftover ground tile (a road
    // texture was showing inside the sun's quad). The sun billboard's own binding is a separate bug.
    g_pRenderer->SetTexture(0, 0);
}
