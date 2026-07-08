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

#include "Ttypes.h"     // POST macros, FeetPerPost
#include "TMap.h"       // TheMap.LastNearTexLOD()
#include "Tpost.h"      // Tpost { z, texID, u,v,d, ... }
#include "TerrTex.h"    // TheTerrTextures.GetTileSRV()
#include "RViewPnt.h"   // RViewPoint
#include "Graphics/DXEngine/DXEngine.h"            // CDXEngine::GetObjProjection/View/CameraPos
#include "Graphics/DXEngine/d3d11/D3D11Renderer.h" // g_pD3D11Renderer
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

static float DrawLodPatch(RViewPoint* vp, int LOD, int loLOD, const D3DVECTOR& cp, float innerWorldR)
{
    const int avail = vp->GetAvailablePostRange(LOD);
    if (avail <= 2) return innerWorldR;
    const int availSafe = avail - 1;
    int range = availSafe;
    if (range > MAX_RADIUS) range = MAX_RADIUS;

    const float ftPerPost   = FeetPerPost * (float)(1 << LOD);
    const float outerWorldR = (float)range * ftPerPost;
    const bool  useTex      = (LOD <= TheMap.LastNearTexLOD());
    const bool  doMorph     = (LOD < loLOD);                 // coarsest LOD has nothing coarser to morph to
    const float morphLo     = outerWorldR * 0.70f;           // start morphing at 70% of this LOD's reach
    const float morphSpan   = outerWorldR * 0.30f;           // ... fully coarse (alpha=1) at the outer edge

    const int centerRow = (int)floorf(cp.x / FeetPerPost) >> LOD;
    const int centerCol = (int)floorf(cp.y / FeetPerPost) >> LOD;

    static GCell          grid[GW * GW];
    static void*          qsrv[CHUNK * CHUNK];
    static short          qi[CHUNK * CHUNK];
    static short          qj[CHUNK * CHUNK];
    static bool           qdone[CHUNK * CHUNK];
    static TGpuVert       cverts[CHUNK * CHUNK * 4];
    static unsigned short cidx[CHUNK * CHUNK * 6];

    const int rLo = centerRow - range, rHi = centerRow + range;
    const int cLo = centerCol - range, cHi = centerCol + range;

    for (int br = rLo; br < rHi; br += CHUNK)
    {
        for (int bc = cLo; bc < cHi; bc += CHUNK)
        {
            // per-chunk ring cull (farthest corner inside a finer LOD -> skip whole chunk)
            {
                float maxd = 0.0f;
                const int rr[2]  = { br, br + CHUNK };
                const int cc2[2] = { bc, bc + CHUNK };
                for (int ii = 0; ii < 2; ++ii)
                    for (int jj = 0; jj < 2; ++jj)
                    {
                        const float wx = fabsf((float)LEVEL_POST_TO_WORLD(rr[ii],  LOD) - cp.x);
                        const float wy = fabsf((float)LEVEL_POST_TO_WORLD(cc2[jj], LOD) - cp.y);
                        const float ch = (wx > wy) ? wx : wy;
                        if (ch > maxd) maxd = ch;
                    }
                if (maxd < innerWorldR) continue;
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
                    float cz = g.za;
                    if (doMorph)
                    {
                        const int r = br - 1 + gi, c = bc - 1 + gj;
                        const int ro = r & 1, co = c & 1;
                        // even neighbour posts survive into LOD+1; interpolate them for the coarse height.
                        #define ZAN(GI,GJ) (grid[(GI)*GW+(GJ)].ok ? grid[(GI)*GW+(GJ)].za : g.za)
                        if (ro && !co)      cz = 0.5f  * (ZAN(gi-1,gj)   + ZAN(gi+1,gj));
                        else if (!ro && co) cz = 0.5f  * (ZAN(gi,gj-1)   + ZAN(gi,gj+1));
                        else if (ro && co)  cz = 0.25f * (ZAN(gi-1,gj-1) + ZAN(gi+1,gj-1) + ZAN(gi-1,gj+1) + ZAN(gi+1,gj+1));
                        // (even,even) survives -> cz = g.za (already)
                        #undef ZAN
                        float dist = (fabsf(g.x) > fabsf(g.y)) ? fabsf(g.x) : fabsf(g.y);
                        float a = (morphSpan > 0.0f) ? (dist - morphLo) / morphSpan : 0.0f;
                        if (a < 0.0f) a = 0.0f; if (a > 1.0f) a = 1.0f;
                        cz = g.za + (cz - g.za) * a;
                    }
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
                    // ring cull PER QUAD: skip quads fully inside the finer coverage (farthest corner
                    // < innerWorldR) so adjacent LODs overlap by AT MOST one quad.
                    {
                        float md = 0.0f, t;
                        t = (fabsf(a.x) > fabsf(a.y)) ? fabsf(a.x) : fabsf(a.y); if (t > md) md = t;
                        t = (fabsf(b.x) > fabsf(b.y)) ? fabsf(b.x) : fabsf(b.y); if (t > md) md = t;
                        t = (fabsf(d.x) > fabsf(d.y)) ? fabsf(d.x) : fabsf(d.y); if (t > md) md = t;
                        t = (fabsf(e.x) > fabsf(e.y)) ? fabsf(e.x) : fabsf(e.y); if (t > md) md = t;
                        if (md < innerWorldR) continue;
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

            // --- draw quads grouped by SRV; build compact verts per group (each vert uploaded once) ---
            for (int q0 = 0; q0 < nq; ++q0)
            {
                if (qdone[q0]) continue;
                void* srv = qsrv[q0];
                int nv = 0, ni = 0;
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
                    const int base = nv;
                    cverts[nv].p[0]=a.x; cverts[nv].p[1]=a.y; cverts[nv].p[2]=a.z; cverts[nv].tu=a.u;      cverts[nv].tv=a.v;      ++nv;
                    cverts[nv].p[0]=b.x; cverts[nv].p[1]=b.y; cverts[nv].p[2]=b.z; cverts[nv].tu=a.u+a.d;  cverts[nv].tv=a.v;      ++nv;
                    cverts[nv].p[0]=d.x; cverts[nv].p[1]=d.y; cverts[nv].p[2]=d.z; cverts[nv].tu=a.u;      cverts[nv].tv=a.v-a.d;  ++nv;
                    cverts[nv].p[0]=e.x; cverts[nv].p[1]=e.y; cverts[nv].p[2]=e.z; cverts[nv].tu=a.u+a.d;  cverts[nv].tv=a.v-a.d;  ++nv;
                    for (int k = 0; k < 4; ++k) { cverts[base+k].n[0]=0.0f; cverts[base+k].n[1]=0.0f; cverts[base+k].n[2]=-1.0f; cverts[base+k].col=c0; cverts[base+k].spec=0; }
                    cidx[ni++]=(unsigned short)(base+0); cidx[ni++]=(unsigned short)(base+1); cidx[ni++]=(unsigned short)(base+3);
                    cidx[ni++]=(unsigned short)(base+0); cidx[ni++]=(unsigned short)(base+3); cidx[ni++]=(unsigned short)(base+2);
                }
                g_pD3D11Renderer->SetTexture(0, (struct ID3D11ShaderResourceView*)srv);
                g_pD3D11Renderer->DrawTerrainMesh(cverts, nv, cidx, ni);
            }
        }
    }

    return (outerWorldR > innerWorldR) ? outerWorldR : innerWorldR;
}

void TerrainGpu_Render(RViewPoint* vp)
{
    if (!g_bGpuTerrain || !vp || !vp->IsReady()) return;
    if (!g_pD3D11Renderer || !g_pD3D11Renderer->IsValid()) return;

    Tpoint pos;
    vp->GetPos(&pos);
    D3DVECTOR cp; cp.x = pos.x; cp.y = pos.y; cp.z = pos.z;

    g_pD3D11Renderer->BeginTerrainPass();
    g_pD3D11Renderer->SetProj((const float*)&CDXEngine::GetObjProjection());
    g_pD3D11Renderer->SetView((const float*)&CDXEngine::GetObjView());
    g_pD3D11Renderer->SetCameraPos(cp.x, cp.y, cp.z);

    const int hiLOD = vp->GetHighLOD();
    const int loLOD = vp->GetLowLOD();

    float innerWorldR = 0.0f;
    for (int LOD = hiLOD; LOD <= loLOD; ++LOD)
    {
        g_pD3D11Renderer->SetTerrainRasterForLod(LOD - hiLOD);   // #78 per-LOD depth bias (0 = finest)
        innerWorldR = DrawLodPatch(vp, LOD, loLOD, cp, innerWorldR);
    }
    // #78 leave slot 0 empty so later sky/sun billboards don't inherit a leftover ground tile (a road
    // texture was showing inside the sun's quad). The sun billboard's own binding is a separate bug.
    g_pD3D11Renderer->SetTexture(0, 0);
}
