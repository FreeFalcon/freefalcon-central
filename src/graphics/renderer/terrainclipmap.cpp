// terrainclipmap.cpp -- Artscout - 2026: #78 mesh-shader terrain, data side.
// Only the posts the camera just uncovered are uploaded; the mesh shader builds
// every vertex from them (see ffterrain.hlsl).
#include <windows.h>
#include <math.h>
#include <vector>

#include "ttypes.h"   // FeetPerPost, LEVEL_POST_TO_WORLD
#include "tmap.h"     // TheMap.LastNearTexLOD()
#include "tpost.h"    // Tpost
#include "terrtex.h"  // TheTerrTextures / TheFarTextures
#include "rviewpnt.h" // RViewPoint
#include "dispopts.h" // DisplayOptions -- screen size for the NVG vignette
#include "graphics/dxengine/dxengine.h"
#include "graphics/dxengine/common/irenderer.h"
#include <stdio.h>
#include <stdarg.h>
#include "terrainclipmap.h"

// Same radius cap the legacy path uses, so both cover the same ground.
static const int TCLIP_MAX_RADIUS = 96;
static const int TCLIP_MORPH_POSTS = 10;

// Artscout - 2026: the debugger stream, same one R12Log uses -- MonoPrint in FF
// goes somewhere else entirely and never reaches the log.
static void TClipLog(const char* fmt, ...)
{
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    _vsnprintf(buf, sizeof(buf) - 1, fmt, ap);
    va_end(ap);
    buf[sizeof(buf) - 1] = 0;
    OutputDebugStringA(buf);
}

// Post info bits -- must match the PI_* defines in ffterrain.hlsl.
static const unsigned int PI_VALID = (1u << 16);
static const unsigned int PI_HASTILE = (1u << 17);
static const unsigned int PI_SLOT_MASK = 0xFFFFu;

// Terrain feature flags -- must match the TF_* defines in ffterrain.hlsl.
static const unsigned int TF_TEXTURED = (1u << 0);
static const unsigned int TF_LIGHTING = (1u << 1);
static const unsigned int TF_FOG = (1u << 2);
static const unsigned int TF_WIREOVERLAY = (1u << 3);
static const unsigned int TF_IRGREY = (1u << 4);
static const unsigned int TF_NVG = (1u << 5);

// Declared OUT here on purpose: inside the anonymous namespace below an extern
// picks up internal linkage and never resolves to its real definition.
extern bool g_bTerrainMeshShader;
extern bool g_bTerrainMeshDebugTint; // flat per-LOD tint, for bring-up
extern int g_terrainRadiusCap; // #DX12 A5: sensor pass shrinks its rings

namespace
{
// One clipmap texel: elevation plus the tile coordinates of this post.
struct ClipPost
{
    float z, u, v, d;
};

struct Level
{
    bool ready;   // filled at least once since the last invalidate
    int lod;      // theater LOD this slice holds
    int originRow; // absolute level post mapped to texel row 0
    int originCol;
    int refreshRow; // rolling re-scan cursor (tiles stream in late)
    // The band actually drawn (ring + margin), in absolute level posts. Only
    // this is streamed: the ring is a fraction of the window, and every post
    // outside it would burn a GetPost plus a tile activation for nothing.
    int actR0, actR1, actC0, actC1;
    std::vector<float> shadowZ; // CPU copy, for the per-chunk bounds
    std::vector<unsigned char> shadowOk;
};

Level s_level[TCLIP_MAX_LODS];
int s_levels = 0;
int s_baseLod = 0;
bool s_created = false;
int s_chunkCount = 0;
TerrainClipConstants s_cb;

// Scratch upload buffers, kept across frames (the rects are small).
std::vector<ClipPost> s_post;
std::vector<unsigned int> s_info;
std::vector<float> s_bounds; // min/max per tile, all levels

// Status line every few seconds: without it a black ground gives no clue which
// step stalled (no support / no data / no chunks).
unsigned long s_statTick = 0;
unsigned int s_statPosts = 0;
unsigned int s_statRegions = 0;
unsigned int s_statTiled = 0; // posts that resolved to a real tile
// Per level, so it is visible WHICH posts miss: outside the theater's range
// (invalid), or valid but with no tile resident.
unsigned int s_statLvPosts[TCLIP_MAX_LODS] = {};
unsigned int s_statLvValid[TCLIP_MAX_LODS] = {};
unsigned int s_statLvTiled[TCLIP_MAX_LODS] = {};
unsigned int s_statLvNoSrv[TCLIP_MAX_LODS] = {}; // valid post, GetTileSRV = 0
// The sensor pass runs its own Update (cap > 0) and overwrites the shared
// constants, so its numbers have to be tracked apart from the main view's.
unsigned int s_statSensorUpdates = 0;
unsigned int s_statSensorPosts = 0;
unsigned int s_statSensorTiled = 0;
unsigned int s_statSensorFlags = 0;
int s_statSensorChunks = 0;
bool s_statSensorSunOk = false;
float s_statSensorSun[10] = {}; // dir3, color3, ambient3, dayNight

inline int WrapTexel(int v)
{
    const int m = v % TCLIP_TEXELS;
    return (m < 0) ? m + TCLIP_TEXELS : m;
}

inline int FloorDiv(int a, int b)
{
    return (a >= 0) ? (a / b) : -(((-a) + b - 1) / b);
}

// The ring's reach for this LOD, in level posts. 0 = nothing to draw.
// applyCap belongs to DRAWING only: the sensor pass shrinks its rings, but the
// streamed band must stay full-size or the main view loses the posts it needs.
int RingRange(RViewPoint* vp, int lod, bool applyCap)
{
    const int avail = vp->GetAvailablePostRange(lod);
    const int availSafe = (avail > 1) ? avail - 1 : 0;
    if (availSafe <= 2)
        return 0;

    int range = availSafe;
    if (range > TCLIP_MAX_RADIUS)
        range = TCLIP_MAX_RADIUS;

    // #DX12 A5: the TGP/Maverick sensor renders a SECOND full view into its RTT
    // every frame; it caps the radius so that pass stays cheap.
    if (applyCap && g_terrainRadiusCap > 0 && range > g_terrainRadiusCap)
        range = g_terrainRadiusCap;
    // Keep the ring inside the window it is streamed into.
    if (range > TCLIP_TEXELS / 2 - TCLIP_CHUNK)
        range = TCLIP_TEXELS / 2 - TCLIP_CHUNK;
    return range & ~1; // even reach -> the snapped box stays symmetric
}

// Fill one post from the theater. Out-of-range posts stay invalid: GetPost is
// unsafe past the available range (tviewpnt.h), it does not merely return null.
void ReadPost(RViewPoint* vp, int lod, int row, int col, int centerRow,
              int centerCol, int availSafe, bool useTex, ClipPost& outPost,
              unsigned int& outInfo)
{
    outPost.z = 0.0f;
    outPost.u = outPost.v = outPost.d = 0.0f;
    outInfo = 0;

    if (abs(row - centerRow) > availSafe || abs(col - centerCol) > availSafe)
        return;

    Tpost* p = vp->GetPost(row, col, lod);
    if (!p)
        return;

    outPost.z = p->z;
    outPost.u = p->u;
    outPost.v = p->v;
    outPost.d = p->d;
    outInfo = PI_VALID;

    // Near tiles come from the high-res set; fall back to the theater-wide far
    // texture of the same tile when the near one has not streamed in yet.
    void* srv = useTex ? TheTerrTextures.GetTileSRV((TextureID)p->texID) :
                         TheFarTextures.GetTileSRV((TextureID)p->texID);
    if (!srv)
        srv = TheFarTextures.GetTileSRV((TextureID)p->texID);
    if (!srv)
        return;

    const unsigned int slot = g_pRenderer->BindlessTexIndex(
        (struct ID3D11ShaderResourceView*)srv);
    if (slot <= PI_SLOT_MASK)
        outInfo |= PI_HASTILE | slot;
}

// Upload one seam-free rect, in ABSOLUTE level posts, and refresh the shadow.
void UploadRect(RViewPoint* vp, Level& lv, int level, int r0, int r1, int c0,
                int c1, int centerRow, int centerCol, int availSafe,
                bool useTex)
{
    const int h = r1 - r0, w = c1 - c0;
    if (h <= 0 || w <= 0)
        return;

    s_post.resize((size_t)h * w);
    s_info.resize((size_t)h * w);

    for (int r = 0; r < h; ++r)
    {
        for (int c = 0; c < w; ++c)
        {
            ClipPost p;
            unsigned int info;
            ReadPost(vp, lv.lod, r0 + r, c0 + c, centerRow, centerCol,
                     availSafe, useTex, p, info);
            s_post[(size_t)r * w + c] = p;
            s_info[(size_t)r * w + c] = info;

            if (level >= 0 && level < TCLIP_MAX_LODS)
            {
                ++s_statLvPosts[level];

                if (info & PI_VALID)
                    ++s_statLvValid[level];

                if (info & PI_HASTILE)
                {
                    ++s_statTiled;
                    ++s_statLvTiled[level];
                }
                else if (info & PI_VALID)
                {
                    ++s_statLvNoSrv[level]; // post exists, tile did not resolve
                }
            }

            const int tr = WrapTexel(r0 + r), tc = WrapTexel(c0 + c);
            lv.shadowZ[(size_t)tr * TCLIP_TEXELS + tc] = p.z;
            lv.shadowOk[(size_t)tr * TCLIP_TEXELS + tc] =
                (info & PI_VALID) ? 1 : 0;
        }
    }

    g_pRenderer->UpdateTerrainClipmap(level, WrapTexel(c0), WrapTexel(r0), w, h,
                                      &s_post[0], &s_info[0]);
    s_statPosts += (unsigned int)(w * h);
    ++s_statRegions;
}

// Clip to the drawn band, split on the toroidal seam, then upload the pieces.
void FillRect(RViewPoint* vp, Level& lv, int level, int r0, int r1, int c0,
              int c1, int centerRow, int centerCol, int availSafe, bool useTex)
{
    // Outside the band nothing is ever drawn, and every post there would cost a
    // GetPost plus a slice of the tile-activation budget.
    if (r0 < lv.actR0)
        r0 = lv.actR0;
    if (r1 > lv.actR1)
        r1 = lv.actR1;
    if (c0 < lv.actC0)
        c0 = lv.actC0;
    if (c1 > lv.actC1)
        c1 = lv.actC1;

    if (r1 <= r0 || c1 <= c0)
        return;

    int rowSplit[3], colSplit[3];
    int nRow = 0, nCol = 0;

    rowSplit[nRow++] = r0;
    if (WrapTexel(r0) + (r1 - r0) > TCLIP_TEXELS)
        rowSplit[nRow++] = r0 + (TCLIP_TEXELS - WrapTexel(r0));
    rowSplit[nRow++] = r1;

    colSplit[nCol++] = c0;
    if (WrapTexel(c0) + (c1 - c0) > TCLIP_TEXELS)
        colSplit[nCol++] = c0 + (TCLIP_TEXELS - WrapTexel(c0));
    colSplit[nCol++] = c1;

    for (int i = 0; i + 1 < nRow; ++i)
    {
        for (int j = 0; j + 1 < nCol; ++j)
        {
            UploadRect(vp, lv, level, rowSplit[i], rowSplit[i + 1],
                       colSplit[j], colSplit[j + 1], centerRow, centerCol,
                       availSafe, useTex);
        }
    }
}

// Per-tile elevation bounds straight off the shadow copy, so the amplification
// shader can cull a chunk without reading the post texture.
void RecomputeBounds(const Level& lv, int level)
{
    float* dst = &s_bounds[(size_t)level * TCLIP_TILES * TCLIP_TILES * 2];

    for (int tr = 0; tr < TCLIP_TILES; ++tr)
    {
        for (int tc = 0; tc < TCLIP_TILES; ++tc)
        {
            float lo = 1e30f, hi = -1e30f;
            // +1 post so a chunk's far edge (shared with the next chunk) counts.
            for (int r = 0; r <= TCLIP_CHUNK; ++r)
            {
                for (int c = 0; c <= TCLIP_CHUNK; ++c)
                {
                    const int pr = WrapTexel(tr * TCLIP_CHUNK + r);
                    const int pc = WrapTexel(tc * TCLIP_CHUNK + c);
                    const size_t k = (size_t)pr * TCLIP_TEXELS + pc;
                    if (!lv.shadowOk[k])
                        continue;
                    const float z = lv.shadowZ[k];
                    if (z < lo)
                        lo = z;
                    if (z > hi)
                        hi = z;
                }
            }
            if (lo > hi) // no valid post in this tile
            {
                lo = 0.0f;
                hi = 0.0f;
            }
            const size_t t = (size_t)tr * TCLIP_TILES + tc;
            dst[t * 2 + 0] = lo;
            dst[t * 2 + 1] = hi;
        }
    }
}
} // namespace

void TerrainClipmap_Invalidate()
{
    for (int i = 0; i < TCLIP_MAX_LODS; ++i)
        s_level[i].ready = false;
    s_chunkCount = 0;
    // Force a fresh CreateTerrainClipmap: leaving 3D releases the backend's
    // images, and re-using them would draw through dead descriptors.
    s_created = false;
}

const TerrainClipConstants& TerrainClipmap_Constants()
{
    return s_cb;
}

int TerrainClipmap_ChunkCount()
{
    return s_chunkCount;
}

bool TerrainClipmap_Update(RViewPoint* vp, const float camPos[3],
                           float dayNight)
{
    if (!g_bTerrainMeshShader || !vp || !camPos)
        return false;
    if (!g_pRenderer || !g_pRenderer->MeshTerrainAvailable())
        return false;

    // Sensor pass (cap > 0) shares every static here with the main view, so
    // its share of the counters is taken as a delta over this call.
    const unsigned int postsEnter = s_statPosts;
    const unsigned int tiledEnter = s_statTiled;

    const int hiLOD = vp->GetHighLOD();
    const int loLOD = vp->GetLowLOD();
    int levels = loLOD - hiLOD + 1;
    if (levels < 1)
        return false;
    if (levels > TCLIP_MAX_LODS)
        levels = TCLIP_MAX_LODS;

    if (!s_created || levels != s_levels || hiLOD != s_baseLod)
    {
        if (!g_pRenderer->CreateTerrainClipmap(TCLIP_TEXELS, levels,
                                               TCLIP_TILES))
            return false;
        s_created = true;
        s_levels = levels;
        s_baseLod = hiLOD;
        s_bounds.assign((size_t)levels * TCLIP_TILES * TCLIP_TILES * 2, 0.0f);
        for (int i = 0; i < levels; ++i)
        {
            s_level[i].ready = false;
            s_level[i].lod = hiLOD + i;
            s_level[i].refreshRow = 0;
            s_level[i].shadowZ.assign(TCLIP_TEXELS * TCLIP_TEXELS, 0.0f);
            s_level[i].shadowOk.assign(TCLIP_TEXELS * TCLIP_TEXELS, 0);
        }
        // NOT TerrainClipmap_Invalidate() here: it clears s_created, which would
        // re-create the clipmap every frame and never let a level past L0.
        s_chunkCount = 0;
    }

    // One full refill per frame at most -- a fresh slice is 65k posts.
    bool refilledThisFrame = false;
    bool boundsDirty = false;
    int readyLevels = 0;

    for (int i = 0; i < s_levels; ++i)
    {
        Level& lv = s_level[i];
        const int lod = lv.lod;
        const float step = FeetPerPost * (float)(1 << lod);
        const int centerRow = (int)floorf(camPos[0] / FeetPerPost) >> lod;
        const int centerCol = (int)floorf(camPos[1] / FeetPerPost) >> lod;
        const bool useTex = (lod <= TheMap.LastNearTexLOD());

        int avail = vp->GetAvailablePostRange(lod);
        const int availSafe = (avail > 1) ? avail - 1 : 0;
        if (availSafe <= 2)
            continue; // nothing streamed here yet

        // The band this level streams: the ring plus a margin for the border
        // posts the normals and the geomorph read, and for camera drift.
        const int range = RingRange(vp, lod, false);
        if (range <= 0)
            continue;
        // Clamped to what the theater actually has: past availSafe every post
        // comes back invalid, and reading them was over half the work.
        int reach = range + TCLIP_CHUNK * 2;
        if (reach > availSafe)
            reach = availSafe;
        lv.actR0 = centerRow - reach;
        lv.actR1 = centerRow + reach + 1;
        lv.actC0 = centerCol - reach;
        lv.actC1 = centerCol + reach + 1;

        // Window origin: the camera sits in the middle of the slice.
        const int wantRow = centerRow - TCLIP_TEXELS / 2;
        const int wantCol = centerCol - TCLIP_TEXELS / 2;

        if (!lv.ready)
        {
            if (refilledThisFrame)
                continue; // next frame; the ring below skips this level
            lv.originRow = wantRow;
            lv.originCol = wantCol;
            FillRect(vp, lv, i, wantRow, wantRow + TCLIP_TEXELS, wantCol,
                     wantCol + TCLIP_TEXELS, centerRow, centerCol, availSafe,
                     useTex);
            lv.ready = true;
            refilledThisFrame = true;
            boundsDirty = true;
            RecomputeBounds(lv, i);
        }
        else
        {
            const int dRow = wantRow - lv.originRow;
            const int dCol = wantCol - lv.originCol;

            if (abs(dRow) >= TCLIP_TEXELS || abs(dCol) >= TCLIP_TEXELS)
            {
                if (refilledThisFrame)
                    continue;
                lv.originRow = wantRow;
                lv.originCol = wantCol;
                FillRect(vp, lv, i, wantRow, wantRow + TCLIP_TEXELS, wantCol,
                         wantCol + TCLIP_TEXELS, centerRow, centerCol,
                         availSafe, useTex);
                refilledThisFrame = true;
                boundsDirty = true;
                RecomputeBounds(lv, i);
            }
            else if (dRow || dCol)
            {
                // Only the strips the window just crossed. Rows first over the
                // NEW column span, then columns over the rows that stay.
                const int newR0 = (dRow > 0) ? lv.originRow + TCLIP_TEXELS :
                                               wantRow;
                const int newR1 = (dRow > 0) ? wantRow + TCLIP_TEXELS :
                                               lv.originRow;
                const int newC0 = (dCol > 0) ? lv.originCol + TCLIP_TEXELS :
                                               wantCol;
                const int newC1 = (dCol > 0) ? wantCol + TCLIP_TEXELS :
                                               lv.originCol;

                if (dRow)
                {
                    FillRect(vp, lv, i, newR0, newR1, wantCol,
                             wantCol + TCLIP_TEXELS, centerRow, centerCol,
                             availSafe, useTex);
                }
                if (dCol)
                {
                    const int keepR0 = (dRow > 0) ? wantRow : newR1;
                    const int keepR1 = (dRow > 0) ? newR0 : wantRow +
                                                                TCLIP_TEXELS;
                    FillRect(vp, lv, i, keepR0, keepR1, newC0, newC1,
                             centerRow, centerCol, availSafe, useTex);
                }

                lv.originRow = wantRow;
                lv.originCol = wantCol;
                boundsDirty = true;
                RecomputeBounds(lv, i);
            }

            // Rolling re-scan over the BAND: a tile activates long after its
            // post was uploaded (the budget is a few per frame), so the band is
            // re-read a few rows at a time until every post has found its tile.
            const int REFRESH_ROWS = 8;
            const int bandRows = lv.actR1 - lv.actR0;
            if (bandRows > 0)
            {
                if (lv.refreshRow >= bandRows)
                    lv.refreshRow = 0;
                const int rr = lv.actR0 + lv.refreshRow;
                FillRect(vp, lv, i, rr, rr + REFRESH_ROWS, lv.actC0, lv.actC1,
                         centerRow, centerCol, availSafe, useTex);
                lv.refreshRow += REFRESH_ROWS;
                if (lv.refreshRow >= bandRows)
                {
                    lv.refreshRow = 0;
                    boundsDirty = true;
                    RecomputeBounds(lv, i);
                }
            }
        }

        if (lv.ready)
            ++readyLevels;
        (void)step;
    }

    if (!readyLevels)
        return false;

    if (boundsDirty)
    {
        g_pRenderer->UpdateTerrainChunkBounds(
            &s_bounds[0], s_levels * TCLIP_TILES * TCLIP_TILES);
    }

    // ---- rings + chunk spans, tiled exactly like the legacy DrawLodPatch ----
    int chunkId = 0;
    int inB[4] = {0, 0, 0, 0};
    bool hasInner = false;

    for (int i = 0; i < s_levels; ++i)
    {
        Level& lv = s_level[i];
        TerrainClipLevelGpu& g = s_cb.clip[i];
        const int lod = lv.lod;

        g.originPost[0] = lv.originRow;
        g.originPost[1] = lv.originCol;
        g.chunkSpan[0] = chunkId;
        g.chunkSpan[1] = g.chunkSpan[2] = g.chunkSpan[3] = 0;
        g.ringInner[0] = g.ringInner[1] = g.ringInner[2] = 0;
        g.ringInner[3] = -1;

        int avail = vp->GetAvailablePostRange(lod);
        const int availSafe = (avail > 1) ? avail - 1 : 0;
        if (!lv.ready || availSafe <= 2)
        {
            g.ringOuter[0] = g.ringOuter[1] = 0;
            g.ringOuter[2] = g.ringOuter[3] = 0;
            g.originPost[2] = g.originPost[3] = 0;
            continue;
        }

        const int centerRow = (int)floorf(camPos[0] / FeetPerPost) >> lod;
        const int centerCol = (int)floorf(camPos[1] / FeetPerPost) >> lod;

        // Same reach the streaming band used -- one source, no drift.
        const int range = RingRange(vp, lod, true);

        // Outer edges snap to EVEN posts, i.e. the coarse LOD's grid lines.
        int rHi = (centerRow + range) & ~1;
        int rLo = centerRow - range;
        if (rLo & 1)
            ++rLo;
        int cHi = (centerCol + range) & ~1;
        int cLo = centerCol - range;
        if (cLo & 1)
            ++cLo;

        g.ringOuter[0] = rLo;
        g.ringOuter[1] = rHi;
        g.ringOuter[2] = cLo;
        g.ringOuter[3] = cHi;

        if (hasInner)
        {
            g.ringInner[0] = inB[0];
            g.ringInner[1] = inB[1];
            g.ringInner[2] = inB[2];
            g.ringInner[3] = inB[3];
        }

        // Chunks align to absolute posts that are a multiple of TCLIP_CHUNK, so
        // one chunk maps onto exactly one bounds tile.
        const int chunkR0 = FloorDiv(rLo, TCLIP_CHUNK);
        const int chunkR1 = FloorDiv(rHi - 1, TCLIP_CHUNK);
        const int chunkC0 = FloorDiv(cLo, TCLIP_CHUNK);
        const int chunkC1 = FloorDiv(cHi - 1, TCLIP_CHUNK);
        const int perCol = chunkR1 - chunkR0 + 1;
        const int perRow = chunkC1 - chunkC0 + 1;

        g.originPost[2] = chunkR0 * TCLIP_CHUNK;
        g.originPost[3] = chunkC0 * TCLIP_CHUNK;
        g.chunkSpan[1] = perRow;
        g.chunkSpan[2] = perRow * perCol;
        g.chunkSpan[3] = perCol;
        chunkId += g.chunkSpan[2];

        // The coarser LOD's inner box is this outer box halved (even edges).
        inB[0] = rLo >> 1;
        inB[1] = rHi >> 1;
        inB[2] = cLo >> 1;
        inB[3] = cHi >> 1;
        hasInner = true;
    }

    s_chunkCount = chunkId;

    // Every view slot gets the base matrix, then the backend overwrites them per
    // eye if this pass is view-instanced / multiview (stereo or quad-views).
    for (int v = 0; v < 4; ++v)
    {
        memcpy(s_cb.view[v], (const float*)&CDXEngine::GetObjView(),
               sizeof(s_cb.view[v]));
        memcpy(s_cb.proj[v], (const float*)&CDXEngine::GetObjProjection(),
               sizeof(s_cb.proj[v]));
    }
    const int viewsUsed = g_pRenderer->GetPerViewMatrices(s_cb.view, s_cb.proj);
    s_cb.camPos[0] = camPos[0];
    s_cb.camPos[1] = camPos[1];
    s_cb.camPos[2] = camPos[2];
    s_cb.camPos[3] = 0.0f;
    s_cb.params[0] = FeetPerPost;
    s_cb.params[1] = dayNight;
    s_cb.params[2] = (float)TCLIP_MORPH_POSTS;
    s_cb.params[3] = (float)TCLIP_TEXELS;
    // The sun as SetLights delivered it -- the very lamp that lights the
    // cockpit. CDXEngine::TheSun is a leftover D3D7 light and stays a fallback.
    bool sunOk = false;
    {
        float sd[3] = {0.0f, 0.0f, 1.0f};
        float sc[3] = {1.0f, 1.0f, 1.0f};
        float sa[3] = {0.0f, 0.0f, 0.0f};

        if (g_pRenderer->GetSunLight(sd, sc, sa))
        {
            float len = sqrtf(sd[0] * sd[0] + sd[1] * sd[1] + sd[2] * sd[2]);
            if (len < 1e-6f)
                len = 1.0f;
            s_cb.sunDir[0] = sd[0] / len;
            s_cb.sunDir[1] = sd[1] / len;
            s_cb.sunDir[2] = sd[2] / len;
            s_cb.sunDir[3] = 0.0f;
            s_cb.sunColor[0] = sc[0];
            s_cb.sunColor[1] = sc[1];
            s_cb.sunColor[2] = sc[2];
            s_cb.sunColor[3] = 0.0f;
            // Floor the ambient at the day/night level so a lit slope brightens
            // without the shaded side collapsing to black.
            const float floorAmb = dayNight * 0.5f;
            s_cb.sunAmbient[0] = (sa[0] > floorAmb) ? sa[0] : floorAmb;
            s_cb.sunAmbient[1] = (sa[1] > floorAmb) ? sa[1] : floorAmb;
            s_cb.sunAmbient[2] = (sa[2] > floorAmb) ? sa[2] : floorAmb;
            s_cb.sunAmbient[3] = 0.0f;
            sunOk = true;
        }
    }

    if (!sunOk)
    {
        const D3DVECTOR& d = CDXEngine::TheSun.dvDirection;
        float len = sqrtf(d.x * d.x + d.y * d.y + d.z * d.z);
        if (len < 1e-6f)
            len = 1.0f;
        s_cb.sunDir[0] = d.x / len;
        s_cb.sunDir[1] = d.y / len;
        s_cb.sunDir[2] = d.z / len;
        s_cb.sunDir[3] = 0.0f;

        const D3DCOLORVALUE& sd = CDXEngine::TheSun.dcvDiffuse;
        const D3DCOLORVALUE& sa = CDXEngine::TheSun.dcvAmbient;
        s_cb.sunColor[0] = sd.r;
        s_cb.sunColor[1] = sd.g;
        s_cb.sunColor[2] = sd.b;
        s_cb.sunColor[3] = 0.0f;
        // Floor the ambient at the day/night level so a lit slope brightens
        // without the shaded side collapsing to black.
        s_cb.sunAmbient[0] = (sa.r > dayNight * 0.5f) ? sa.r : dayNight * 0.5f;
        s_cb.sunAmbient[1] = (sa.g > dayNight * 0.5f) ? sa.g : dayNight * 0.5f;
        s_cb.sunAmbient[2] = (sa.b > dayNight * 0.5f) ? sa.b : dayNight * 0.5f;
        s_cb.sunAmbient[3] = 0.0f;
    }

    // #A5/#97: the sensor pass is monochrome and NVG greens the world; the mesh
    // terrain has its own shader, so it must be told what the others are doing.
    s_cb.flags[0] = TF_TEXTURED | TF_LIGHTING;

    if (g_bTerrainMeshDebugTint)
        s_cb.flags[0] |= TF_WIREOVERLAY;

    if (g_pRenderer->IsIRGrey())
        s_cb.flags[0] |= TF_IRGREY;

    if (g_pRenderer->IsNvgMode())
        s_cb.flags[0] |= TF_NVG;

    // Distance haze, as the legacy terrain pass had (FF_FOG): it dissolves the
    // far ground into the horizon and hides the residual LOD-seam shimmer.
    {
        float fs = 0.0f, fe = 0.0f, frgb[3] = {0.0f, 0.0f, 0.0f};

        if (g_pRenderer->GetFogParams(fs, fe, frgb))
        {
            s_cb.flags[0] |= TF_FOG;
            s_cb.fog[0] = fs;
            s_cb.fog[1] = fe;
            s_cb.fogColor[0] = frgb[0];
            s_cb.fogColor[1] = frgb[1];
            s_cb.fogColor[2] = frgb[2];
            s_cb.fogColor[3] = 1.0f;
        }
    }

    s_cb.misc[0] = (float)DisplayOptions.DispWidth;
    s_cb.misc[1] = (float)DisplayOptions.DispHeight;
    s_cb.misc[2] = (float)(GetTickCount() % 100000u) * 0.001f;
    s_cb.misc[3] = 0.0f;
    s_cb.flags[1] = (unsigned int)s_levels;
    s_cb.flags[2] = (unsigned int)s_baseLod;
    s_cb.flags[3] = (unsigned int)s_chunkCount;

    // Frustum planes for the amplification shader's cull, in the same
    // camera-relative world space the chunk AABBs use (the object view matrix
    // is rotation-only, so view*proj applies directly).
    {
        // One AS cull serves every view, so the planes must cover ALL of them:
        // per view, keep the smallest offset, which is the widest volume. Eyes
        // look almost the same way, so the normals stay near-parallel.
        static const int col[6] = {0, 0, 1, 1, 2, 2};
        static const float sign[6] = {1.0f, -1.0f, 1.0f, -1.0f, 0.0f, -1.0f};
        const int viewCount = (viewsUsed > 0) ? viewsUsed : 1;

        for (int p = 0; p < 6; ++p)
            s_cb.frustum[p][0] = s_cb.frustum[p][1] = s_cb.frustum[p][2] =
                s_cb.frustum[p][3] = 0.0f;

        for (int v = 0; v < viewCount; ++v)
        {
            float mvp[16];
            for (int r = 0; r < 4; ++r)
            {
                for (int c = 0; c < 4; ++c)
                {
                    float sum = 0.0f;
                    for (int k = 0; k < 4; ++k)
                        sum += s_cb.view[v][r * 4 + k] * s_cb.proj[v][k * 4 + c];
                    mvp[r * 4 + c] = sum;
                }
            }

            // Row-vector convention (clip = v * mvp): a plane is a column of
            // mvp combined with the w column. Inside = dot(n, p) + d >= 0.
            for (int p = 0; p < 6; ++p)
            {
                float pl[4];
                for (int i = 0; i < 4; ++i)
                {
                    // p == 4 is the near side (plain z column, reversed-Z too).
                    pl[i] = (p == 4) ? mvp[i * 4 + col[p]] :
                                       mvp[i * 4 + 3] +
                                           sign[p] * mvp[i * 4 + col[p]];
                }
                const float len =
                    sqrtf(pl[0] * pl[0] + pl[1] * pl[1] + pl[2] * pl[2]);
                if (len <= 1e-6f)
                {
                    // Degenerate matrix -> leave the plane zero, which rejects
                    // nothing. Never cull on a plane we could not build.
                    s_cb.frustum[p][0] = s_cb.frustum[p][1] =
                        s_cb.frustum[p][2] = s_cb.frustum[p][3] = 0.0f;
                    continue;
                }
                const float inv = 1.0f / len;
                if (v == 0 || pl[3] * inv < s_cb.frustum[p][3])
                {
                    for (int i = 0; i < 4; ++i)
                        s_cb.frustum[p][i] = pl[i] * inv;
                }
            }
        }
    }

    // The sensor pass overwrites every shared static, so keep its own view of
    // the last update apart from the main view's.
    if (g_terrainRadiusCap > 0)
    {
        ++s_statSensorUpdates;
        s_statSensorPosts += s_statPosts - postsEnter;
        s_statSensorTiled += s_statTiled - tiledEnter;
        s_statSensorFlags = s_cb.flags[0];
        s_statSensorChunks = s_chunkCount;
        s_statSensorSunOk = sunOk;
        for (int k = 0; k < 3; ++k)
        {
            s_statSensorSun[k] = s_cb.sunDir[k];
            s_statSensorSun[3 + k] = s_cb.sunColor[k];
            s_statSensorSun[6 + k] = s_cb.sunAmbient[k];
        }
        s_statSensorSun[9] = s_cb.params[1];
    }

    // ---- status line, on a clock (a per-frame line drowns the console) ----
    {
        const unsigned long now = GetTickCount();
        if (!s_statTick)
            s_statTick = now;
        if (now - s_statTick >= 5000)
        {
            TClipLog("[terr-mesh] levels=%d base=%d chunks=%d views=%d | 5s: "
                     "posts=%u regions=%u tiled=%u\n",
                     s_levels, s_baseLod, s_chunkCount, viewsUsed, s_statPosts,
                     s_statRegions, s_statTiled);
            TClipLog("[terr-mesh]  flags=0x%X (tex=%d lit=%d fog=%d ir=%d "
                     "nvg=%d) cap=%d\n",
                     s_cb.flags[0], (int)((s_cb.flags[0] & TF_TEXTURED) != 0),
                     (int)((s_cb.flags[0] & TF_LIGHTING) != 0),
                     (int)((s_cb.flags[0] & TF_FOG) != 0),
                     (int)((s_cb.flags[0] & TF_IRGREY) != 0),
                     (int)((s_cb.flags[0] & TF_NVG) != 0), g_terrainRadiusCap);
            TClipLog("[terr-mesh]  sensor: updates=%u posts=%u tiled=%u "
                     "chunks=%d flags=0x%X (tex=%d lit=%d ir=%d nvg=%d)\n",
                     s_statSensorUpdates, s_statSensorPosts, s_statSensorTiled,
                     s_statSensorChunks, s_statSensorFlags,
                     (int)((s_statSensorFlags & TF_TEXTURED) != 0),
                     (int)((s_statSensorFlags & TF_LIGHTING) != 0),
                     (int)((s_statSensorFlags & TF_IRGREY) != 0),
                     (int)((s_statSensorFlags & TF_NVG) != 0));
            TClipLog("[terr-mesh]  sensor sun src=%s dir=(%.3f %.3f %.3f) "
                     "diff=(%.2f %.2f %.2f) amb=(%.2f %.2f %.2f) "
                     "dayNight=%.2f\n",
                     s_statSensorSunOk ? "lights" : "FALLBACK(TheSun)",
                     s_statSensorSun[0], s_statSensorSun[1], s_statSensorSun[2],
                     s_statSensorSun[3], s_statSensorSun[4], s_statSensorSun[5],
                     s_statSensorSun[6], s_statSensorSun[7], s_statSensorSun[8],
                     s_statSensorSun[9]);
            TClipLog("[terr-mesh]  sun src=%s dir=(%.3f %.3f %.3f) diff=(%.2f "
                     "%.2f %.2f) amb=(%.2f %.2f %.2f) dayNight=%.2f fog=%d "
                     "(%.0f..%.0f)\n",
                     sunOk ? "lights" : "FALLBACK(TheSun)",
                     s_cb.sunDir[0], s_cb.sunDir[1], s_cb.sunDir[2],
                     s_cb.sunColor[0], s_cb.sunColor[1], s_cb.sunColor[2],
                     s_cb.sunAmbient[0], s_cb.sunAmbient[1], s_cb.sunAmbient[2],
                     s_cb.params[1], (int)((s_cb.flags[0] & TF_FOG) != 0),
                     s_cb.fog[0], s_cb.fog[1]);
            for (int i = 0; i < s_levels; ++i)
            {
                const Level& lv = s_level[i];
                const TerrainClipLevelGpu& g = s_cb.clip[i];
                TClipLog("[terr-mesh]  L%d lod=%d ready=%d ring=(%d..%d, "
                         "%d..%d) chunks=%d tex=%d | posts=%u valid=%u "
                         "tiled=%u noSrv=%u\n",
                         i, lv.lod, (int)lv.ready, g.ringOuter[0],
                         g.ringOuter[1], g.ringOuter[2], g.ringOuter[3],
                         g.chunkSpan[2],
                         (int)(lv.lod <= TheMap.LastNearTexLOD()),
                         s_statLvPosts[i], s_statLvValid[i], s_statLvTiled[i],
                         s_statLvNoSrv[i]);
            }
            s_statTick = now;
            s_statPosts = s_statRegions = s_statTiled = 0;
            s_statSensorUpdates = s_statSensorPosts = s_statSensorTiled = 0;
            for (int k = 0; k < TCLIP_MAX_LODS; ++k)
            {
                s_statLvPosts[k] = s_statLvValid[k] = 0;
                s_statLvTiled[k] = s_statLvNoSrv[k] = 0;
            }
        }
    }

    return s_chunkCount > 0;
}
