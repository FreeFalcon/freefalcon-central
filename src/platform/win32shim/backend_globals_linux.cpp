//-----------------------------------------------------------------------------
// backend_globals_linux.cpp -- Artscout - 2026 (#104, Linux port).
//
// The backend-neutral render globals (g_bUseGpu, g_bUseD3D12, g_pRenderBackend, g_pRenderer, g_vrClearColor) and
// the concrete D3D12 pointers historically live in the Windows-only D3D12 sources (d3d12backend.cpp,
// D3D12Renderer.cpp, D3D12TextureManager.cpp). Those TUs are NOT compiled on Linux (there is no D3D12), yet
// shared engine code still *references* the symbols -- the neutral ones because Vulkan uses them, and the D3D12
// pointers because Tex.cpp/dxvbmanager name them inside `if (g_bUseD3D12) { ... }` branches that never execute on
// Linux but must still link. This TU provides those definitions for the Linux build ONLY.
//
// On Windows this file is a no-op (guarded out) -- the real definitions stay in the D3D12 sources, untouched.
//-----------------------------------------------------------------------------
#ifndef _WIN32

class IRenderBackend;
class IRenderer;
class D3D12Backend;
class D3D12Renderer;
class D3D12TextureManager;

// ---- backend-neutral selection/state (peers of the Windows definitions in d3d12backend.cpp / D3D12Renderer.cpp) --
bool g_bUseGpu =
    true; // a GPU backend is always used (Vulkan on Linux); never set false
bool g_bUseD3D12 =
    false; // no D3D12 on Linux -> the D3D12-concrete branches are dead here
IRenderBackend* g_pRenderBackend =
    0; // set to the VulkanBackend at bring-up (devmgr)
IRenderer* g_pRenderer = 0; // set to the VulkanRenderer at bring-up (devmgr)
float g_vrClearColor[3] = {0.0f, 0.0f, 0.0f};
bool g_bGpuDraw =
    false; // set per-frame: the renderer drew 3D -> present blits the scene
// g_bVulkanScene is DEFINED in VulkanBackend.cpp (compiled on Linux too), so it is not redefined here.

// ---- #78 reversed-Z terrain depth biases. Their real home is ui/src/f4config.cpp (platform-neutral), which is
// NOT yet part of the Linux build, so VulkanRenderer.cpp's `extern float g_fGpuTerrain*Bias` references are
// unresolved. Provide WEAK definitions: once ui/src (f4config.cpp) joins the Linux link, its strong definitions
// win with no duplicate-symbol clash; until then these supply the defaults (see f4config.cpp comments).
__attribute__((weak)) float g_fGpuTerrainSlopeBias = 4.0f;
__attribute__((weak)) float g_fGpuTerrainDepthBias = 0.0f;

// ---- backend-neutral RTT-batch flag: its strong definition lives in the Windows-only D3D12Renderer.cpp, but
// shared code (render2d/display/context) reads it. Weak default here; a real backend-neutral home can override.
__attribute__((weak)) bool g_rttBatchActive = false;

// ---- D3D12-concrete pointers: referenced by shared code under (never-taken) g_bUseD3D12 branches -> link stubs ----
D3D12Backend* g_pD3D12Backend = 0;
D3D12Renderer* g_pD3D12Renderer = 0;
D3D12TextureManager* g_pD3D12TextureManager = 0;

#endif // !_WIN32
