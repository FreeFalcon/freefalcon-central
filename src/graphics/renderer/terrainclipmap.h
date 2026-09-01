// terrainclipmap.h -- Artscout - 2026: #78 mesh-shader terrain, data side.
// Streams the theater posts into a toroidal GPU clipmap (one array slice per
// LOD) and publishes the constants ffterrain.hlsl reads. No vertices.
#ifndef _TERRAINCLIPMAP_H_
#define _TERRAINCLIPMAP_H_

class RViewPoint;

#define TCLIP_MAX_LODS 8  // must match MAX_CLIP_LODS in ffterrain.hlsl
#define TCLIP_TEXELS 256  // posts per clipmap slice edge
#define TCLIP_CHUNK 8     // quads per chunk edge (CHUNK_QUADS in the shader)
#define TCLIP_TILES (TCLIP_TEXELS / TCLIP_CHUNK) // bounds tiles per slice edge

// One clipmap level, mirroring ClipLevel in ffterrain.hlsl field for field.
struct TerrainClipLevelGpu
{
    int originPost[4]; // xy = window origin (row,col) in level posts
    int ringOuter[4];  // rLo, rHi, cLo, cHi
    int ringInner[4];  // finer LOD's box; w < 0 = none
    int chunkSpan[4];  // x = first chunk id, y = per row, z = count, w = per col
};

// Mirrors cbTerrain. One cbuffer, one C++ mirror -- keep the order identical.
struct TerrainClipConstants
{
    // Per VIEW (flat = 1, stereo = 2, quad-views = 4), indexed by SV_ViewID.
    // The neutral code fills every slot with the base matrix; a backend that
    // draws view-instanced overwrites them per eye.
    float view[4][16];
    float proj[4][16];
    float camPos[4];
    float params[4];      // FeetPerPost, day/night, morph posts, clip texels
    unsigned int flags[4]; // TF_* bits, clip LOD count, base LOD, chunk count
    float fog[4];
    float fogColor[4];
    float sunDir[4];     // xyz = ray direction (away from the sun)
    float sunColor[4];   // rgb = sun diffuse
    float sunAmbient[4]; // rgb = scene ambient
    float misc[4];       // xy = screen size (px), z = time (sec)
    float frustum[6][4];
    TerrainClipLevelGpu clip[TCLIP_MAX_LODS];
};

// Per-frame: stream in whatever the camera uncovered, rebuild the rings and the
// constants. False = nothing to draw this frame (no device support, no data yet).
bool TerrainClipmap_Update(RViewPoint* vp, const float camPos[3],
                           float dayNight);

const TerrainClipConstants& TerrainClipmap_Constants();
int TerrainClipmap_ChunkCount();

// Drop every window (theater change / teleport): the next Update refills.
void TerrainClipmap_Invalidate();

#endif // _TERRAINCLIPMAP_H_
