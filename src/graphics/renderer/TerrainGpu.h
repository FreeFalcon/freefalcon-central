//-----------------------------------------------------------------------------
// TerrainGpu.h -- Artscout - 2026: #78 Phase 1
//
// Draw the (near) terrain as GPU WORLD-SPACE geometry through the existing object
// path (VS_Object, real gView/gProj, real depth buffer) instead of the legacy CPU
// screen-space rasterization. Gated by g_bGpuTerrain (OFF by default; the CPU path
// stays the shipping default until parity is confirmed).
//
// Phase 1 goal: the ground appears in roughly the right place and lines up with
// objects/cockpit (coordinate parity). Flat per-vertex color, no lighting/texture,
// finest LOD only, capped radius. Texturing, normals/lighting, geomorphing, water
// and full multi-LOD coverage come in later phases.
//-----------------------------------------------------------------------------
#ifndef _TERRAINGPU_H_
#define _TERRAINGPU_H_

class RViewPoint;

// Draw the GPU terrain patch for this viewpoint. Sets up the object-path camera
// (CDXEngine's per-eye matrices) itself, so it is safe to call from the terrain
// draw hook in RenderOTW::DrawScene. No-op unless g_bGpuTerrain and D3D11 are live.
void TerrainGpu_Render(RViewPoint* vp);

#endif // _TERRAINGPU_H_
