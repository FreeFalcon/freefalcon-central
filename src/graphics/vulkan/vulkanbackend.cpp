//-----------------------------------------------------------------------------
// VulkanBackend.cpp -- Artscout - 2026 (#104, Linux port -- subsystem 2).
// A self-contained Vulkan bring-up implementing IRenderBackend: instance, device, SDL surface, swapchain, depth,
// render pass, per-frame command buffers + sync, the clear/present frame loop, a fullscreen RGB565 menu blit, and
// swapchain resize. The heavier draw surface (IRenderer: pipelines/passes/shaders) is VulkanRenderer, built on top
// of the device/queue/swapchain this class owns.
//-----------------------------------------------------------------------------
#include <ciso646> // not/and/or tokens under MSVC
#include "vulkanbackend.h"
#include "vulkanvma.h"
#include "vulkanrenderer.h" // g_pVulkanRenderer -- SetGScreenSize forwards the 2D pixel->NDC divisor here
#include "../shaders/ffshaderblobs.h" // ffxrblit.hlsl: the sRGB-decoding XR blit

#include <vulkan/vulkan.h>

// Surface source is platform-split: Windows uses VK_KHR_win32_surface straight from the real HWND (so the Vulkan
// backend is selectable at runtime next to D3D12 with the existing window, no SDL needed for the surface); Linux
// uses SDL. A future SDL window on Windows still hands back an HWND, so this stays compatible.
#ifdef _WIN32
#include <vulkan/vulkan_win32.h>
#else
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include "../../platform/ffplatform/ff_window.h" // Init(HWND) receives the Window* token on Linux
#endif

#include <cstdio>
#include <cstring>
#include <cstdint>
#include <cstdarg>
#include <vector>
#include <algorithm>
#include <chrono> // #107 PERF: frame profiler timing
#include <map>
#include <mutex>
#include <string>

// Artscout - 2026 (#104): external synchronization for the single VkQueue. Vulkan requires that vkQueueSubmit /
// vkQueuePresentKHR / vkQueueWaitIdle on one queue never run concurrently from different threads. The engine uploads
// vertex buffers and textures from background loader threads (VulkanVbManager / VulkanTextureManager) onto the SAME
// queue the render thread submits + presents on. Every queue operation -- here and in those managers -- must hold this
// lock. (The old per-frame vkQueueWaitIdle accidentally serialized everything and hid the race; removing it exposed a
// hard driver deadlock: a loader thread's vkQueueWaitIdle colliding with the render thread's submit/present.)
std::mutex g_vkQueueMutex;

// ---------------------------------------------------------------------------------------- #107 PERF: frame profiler
// Lightweight CPU-side profiler: accumulate per-phase wall-clock (mainly the fence WAITS -- the signal that tells us
// CPU-bound vs GPU-bound / bad overlap) and dump an average every ~2s to OutputDebugString. Enabled by g_bVulkanProfile
// (FFViper.cfg "VulkanProfile 1"). Zero cost when off.
bool g_bVulkanProfile = true; // default ON -- the user runs with the profiler telemetry
namespace
{
enum VkProfCat
{
    VP_SCENE_WAIT,
    VP_TAIL_REUSE_WAIT,
    VP_TAIL_SCENE_WAIT,
    VP_BLIT_SCENE_WAIT,
    VP_BLIT_TAILS_WAIT,
    VP_BLIT_FENCE_WAIT,
    VP_RTT_REUSE_WAIT,
    VP_RTT_CONSUME_WAIT,
    VP_MENU_WAIT,
    VP_DRAWSCENE,
    VP_BLITGROUP,
    VP_TERRAIN_GPU,
    VP_TERRAIN,
    VP_OBJECTS,
    VP_TERR_ACC,
    VP_TERR_FLUSH,
    VP_GPU_SCENE,
    VP_GPU_BLIT, // #107 PERF: GPU-side ms (timestamp queries), not CPU waits
    VP_RENDERVR,
    VP_TAIL_CPU,
    VP_VCOCK, // #107 PERF stage 2: decompose the uncovered CPU_FRAME
    VP_PREVR,
    VP_POSTVR, // #107 PERF stage 2b: RenderFrame outside the VR body
    VP_XRWAITFRAME, // #107 PERF stage 2c: xrWaitFrame pacing gate (inside PreVR)
    VP_COUNT
};
const char* kVkProfNames[VP_COUNT] = { // order MUST match VkProfCat
    "sceneFence(BeginScene)",
    "tailFence(reuse)",
    "sceneFence(BeginTail)",
    "sceneFence(blit)",
    "tails(blit)",
    "vrBlitFence",
    "rttFence(reuse)",
    "rttFence(consume)",
    "menuFence",
    "DrawScene(CPU)",
    "BlitGroup(xrWait)",
    "TerrainGpu(draws)",
    "TerrainSpan(CPU)",
    "Objects(CPU)",
    "TerrAcc(CPU-build)",
    "TerrFlush(draws)",
    "GPU_scene",
    "GPU_blit",
    "RenderVR(CPU)",
    "TailEye(CPU)",
    "VCock(CPU)",
    "PreVR(CPU)",
    "PostVR(CPU)",
    "XrWaitFrame(pace)"};
using ProfClock = std::chrono::steady_clock;
struct VkProf
{
    double accum[VP_COUNT] = {};
    int frames = 0;
    double accumTotalCpu = 0.0;
    long long drawCalls = 0;
    long long terrDraws =
        0; // #107 PERF: obj draws + terrain bucket draws/frame
    ProfClock::time_point frameStart;
    bool haveStart = false;
    // #107 PERF worst-frame: snapshot the accumulators at FrameStart; at EndFrame the deltas are THIS
    // frame's per-phase costs. Keep the worst frame of the window + its breakdown -- the averages hide
    // the rare 90->83 dip frames; this line shows exactly which phase blew the 11.1ms budget.
    double frameBase[VP_COUNT] = {};
    double worstTotal = 0.0;
    double worstPhase[VP_COUNT] = {};
} g_vkProf;
inline double ProfMs(ProfClock::time_point a, ProfClock::time_point b)
{
    return std::chrono::duration<double, std::milli>(b - a).count();
}
}
// Time `stmt` into category `cat` when profiling is on (else just run it).
#define PROF_WAIT(cat, stmt)                                                   \
    do                                                                         \
    {                                                                          \
        if (g_bVulkanProfile)                                                  \
        {                                                                      \
            auto _pa = ProfClock::now();                                       \
            stmt;                                                              \
            g_vkProf.accum[cat] += ProfMs(_pa, ProfClock::now());              \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            stmt;                                                              \
        }                                                                      \
    } while (0)

// windows.h (via irenderbackend.h) defines min/max as macros for the game code; this clean Vulkan TU wants the
// std:: templates, so drop the macros here (we never use the game's MAXIMUM/MINIMUM in this file).
#undef min
#undef max

// Artscout - 2026 (#104): log to stderr AND (on Windows) OutputDebugStringA, so the failure reason is visible in
// the VS Output window of the GUI app (stderr is not).
#ifdef _WIN32
static void VkbLog(const char* fmt, ...)
{
    // 4 KB, not 512: validation-layer messages routinely run 1-2 KB (they quote the spec + the object chain).
    char buf[4096];
    va_list ap;
    va_start(ap, fmt);
    _vsnprintf(buf, sizeof(buf) - 1, fmt, ap);
    va_end(ap);
    buf[sizeof(buf) - 1] = 0;
    OutputDebugStringA(buf);
    fputs(buf, stderr);
}
#define VKB_LOG(...) VkbLog("[Vulkan] " __VA_ARGS__)
#else
#define VKB_LOG(...)                                                           \
    do                                                                         \
    {                                                                          \
        fprintf(stderr, "[Vulkan] " __VA_ARGS__);                              \
    } while (0)
#endif
#define VK_CHECK(expr)                                                         \
    do                                                                         \
    {                                                                          \
        VkResult _r = (expr);                                                  \
        if (_r != VK_SUCCESS)                                                  \
        {                                                                      \
            VKB_LOG("%s failed: VkResult=%d (%s:%d)\n", #expr, (int)_r,        \
                    __FILE__, __LINE__);                                       \
            return false;                                                      \
        }                                                                      \
    } while (0)

namespace
{
constexpr int kMaxFramesInFlight = 2;
}

// Runtime backend selection (see header). Default: Windows starts on D3D12 (option flips to Vulkan); the Linux
// startup sets g_bUseVulkan = true since Vulkan is the only backend there.
#ifdef _WIN32
bool g_bUseVulkan = false;
#else
bool g_bUseVulkan = true;
#endif
// Artscout - 2026 (#104): gate the flat 3D scene path (lazy scene open + PresentScene). OFF by default while the
// scene frame is debugged headless -- with it OFF, entering 3D is stable (no scene draws -> no GPU hang; the menu
// 565 path presents), matching the pre-scene behaviour. Set to 1 in cfg to exercise the 3D scene.
bool g_bVulkanScene =
    true; // #104: re-enabled for scene-present debugging (per-frame acquire/present logs)
// #104: the GPU-terrain path (screen-space CPU LOD -> GPU, #78). This was OFF because the terrain came out as garbage
// -- textures flickering/swimming. That diagnosis ("vertex layout / world-transform mismatch") was right about the
// transform: BeginTerrainPass was literally `BeginObjectPass()`, so the terrain drew with the OBJECT world matrix
// while its posts are camera-relative, lit when it is already pre-lit in the vertex colour, without fog, and without
// the #78 depth bias. All of that is ported now (see VulkanRenderer::BeginTerrainPass), so it is worth a look --
// turned ON to be evaluated against a frame rather than by reading the code.
bool g_bVulkanTerrain = true;
// #104 DIAGNOSTIC: when true, PresentScene ignores the scene and clears the swapchain to a per-frame CYCLING color.
// If the screen visibly cycles -> presentation works (the freeze is scene/sim); if it stays frozen -> presentation
// itself is broken (swapchain/window). Set false for normal rendering.
bool g_bVulkanPresentTest = false;
VulkanBackend* g_pVulkanBackend = nullptr;

struct VulkanBackend::Impl
{
    void* sdlWindow =
        nullptr; // SDL_Window* on Linux (opaque here; Windows uses the HWND directly)
    HWND hwndToken = nullptr;
    int width = 0, height = 0;
    bool vsync = true;
    unsigned long clearArgb = 0xFF203040;

    // Artscout - 2026 (#107 VR-Vulkan): extra instance/device extensions the OpenXR runtime requires so the session
    // can share swapchain images with the compositor (VK_KHR_external_memory/semaphore/... on Win32). Populated by
    // SetExtraVulkanExtensions BEFORE Init (from OpenXRBackend::QueryVulkanExtensions); appended in Init. Empty = flat.
    std::vector<std::string> vrInstExts;
    std::vector<std::string> vrDevExts;

    // #109 enable2: VR bring-up hooks (installed by OpenXRBackend before Init; NULL on the flat/non-VR/Linux path).
    // In VR mode Init calls these so the OpenXR runtime wraps vkCreateInstance/vkCreateDevice and picks the GPU.
    VulkanBackend::VkbXrCreateInstanceHook hookCreateInstance = nullptr;
    VulkanBackend::VkbXrPickPhysicalHook hookPickPhysical = nullptr;
    VulkanBackend::VkbXrCreateDeviceHook hookCreateDevice = nullptr;
    void* hookUser = nullptr;

    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger =
        VK_NULL_HANDLE; // #104: validation output (see VkbSetupValidation)
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkPhysicalDevice phys = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    uint32_t gfxFamily = 0;
    VkQueue queue = VK_NULL_HANDLE;
    VkQueue xrQueue =
        VK_NULL_HANDLE; // #107 VR-Vulkan: dedicated queue handed to the OpenXR runtime
    uint32_t xrQueueIndex =
        0; // index within gfxFamily (1 if available, else 0)

    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkFormat scFormat = VK_FORMAT_B8G8R8A8_UNORM;
    VkExtent2D scExtent = {0, 0};
    std::vector<VkImage> scImages;
    std::vector<VkImageView> scViews;
    std::vector<VkFramebuffer> framebuffers;

    // Artscout - 2026 (#104): D3D12 runs D32_FLOAT_S8X24 -- reversed-Z depth PLUS a stencil plane, which #76 (the HUD
    // aperture clip) needs. The Vulkan side had plain D32_SFLOAT, so there was no stencil to mark or test at all.
    // Picked for real against the device in PickDepthFormat; depthHasStencil gates the stencil paths if we fall back.
    VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
    bool depthHasStencil = false;
    VkImage depthImage = VK_NULL_HANDLE;
    VkDeviceMemory depthMem = VK_NULL_HANDLE;
    VkImageView depthView = VK_NULL_HANDLE;

    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkCommandPool cmdPool = VK_NULL_HANDLE;

    // per-frame-in-flight sync + command buffer
    VkCommandBuffer cmd[kMaxFramesInFlight] = {};
    VkSemaphore semImageAvail[kMaxFramesInFlight] = {};
    VkFence fenceInFlight[kMaxFramesInFlight] = {};
    // Artscout - 2026 (#104): the render-done (present-wait) semaphore is PER SWAPCHAIN IMAGE, not per frame-in-flight.
    // A present that waits on semRenderDone[i] holds it until the presentation engine is done; that completion is NOT
    // gated by fenceInFlight (which only covers the queue submit). Indexing by the in-flight slot re-signals a semaphore
    // still owned by a pending present -> intermittent frozen picture on NVIDIA. We return to image i only after it is
    // re-acquired, which proves its previous present finished, so a per-image semaphore is always free to reuse.
    std::vector<VkSemaphore>
        semRenderDone; // one per scImages[] entry (rebuilt with the swapchain)
    uint32_t frameSlot = 0; // 0..kMaxFramesInFlight-1
    uint32_t imageIndex = 0; // acquired swapchain image
    bool resizeRequested =
        false; // #104: OUT_OF_DATE/SUBOPTIMAL sets this; the recreate happens at the
    // START of the next present (EnsureSwapchainReady), never mid-frame
    bool recording = false;
    bool frameOk = false; // acquire succeeded this frame
    bool headless =
        false; // no surface/swapchain: offscreen color target stands in (server/tests)
    VkDeviceMemory offscreenColorMem =
        VK_NULL_HANDLE; // headless: memory of the offscreen color image (in scImages[0])

    // ---- multiview / quad-views scene target ----
    bool multiviewOk = false; // device supports + enabled VK_KHR_multiview
    bool bindlessOk =
        false; // #107 PERF: device supports + enabled descriptor-indexing (bindless terrain)
    // Artscout - 2026 (#78): VK_EXT_mesh_shader for the GPU-driven terrain. Its
    // entry points are device-level, so they must be resolved by hand.
    bool meshShaderOk = false;
    PFN_vkCmdDrawMeshTasksEXT cmdDrawMeshTasks = nullptr;
    int sceneViews = 0; // N layers (2 stereo / 4 quad)
    int sceneW = 0, sceneH = 0;
    // #107 VR-Vulkan: the scene target is allocated at the LARGEST per-view size and REUSED for every eye (quad's
    // periphery+focus differ, but reallocating it per eye churned allocations -> hit the driver's max-allocation cap
    // and crashed). Each eye renders into the top-left sceneRenderW x sceneRenderH sub-rect; the blit copies only that.
    int sceneRenderW = 0, sceneRenderH = 0;
    VkFormat sceneFormat = VK_FORMAT_R8G8B8A8_UNORM;
    VkImage sceneColor = VK_NULL_HANDLE;
    VkDeviceMemory sceneColorMem = VK_NULL_HANDLE;
    VkImageView sceneColorView = VK_NULL_HANDLE;
    VkImage sceneDepth = VK_NULL_HANDLE;
    VkDeviceMemory sceneDepthMem = VK_NULL_HANDLE;
    VkImageView sceneDepthView = VK_NULL_HANDLE;
    VkRenderPass sceneRenderPass = VK_NULL_HANDLE;
    VkFramebuffer sceneFbo = VK_NULL_HANDLE;
    VkCommandBuffer sceneCmd = VK_NULL_HANDLE;
    // #107 PERF: GPU-side timing. A 4-slot timestamp pool (scene begin/end = 0/1, blit begin/end = 2/3), reused per
    // group; each group reads its own slots before the next overwrites (groups run serially). Created lazily on first
    // profiled frame. tsPeriodNs = ns per tick (device limit). 0 => timestamps unsupported, GPU_scene/GPU_blit stay 0.
    VkQueryPool tsPool = VK_NULL_HANDLE;
    double tsPeriodNs = 0.0;
    VkFence sceneFence = VK_NULL_HANDLE;
    bool sceneRecording = false;

    // Artscout - 2026: an sRGB XR image encodes on write and the scene bytes
    // are already encoded, so those copies run through ffxrblit.hlsl, which
    // decodes first. A plain blit cannot -- see SetXrColorFormat.
    VkFormat xrColorFormat = VK_FORMAT_UNDEFINED;
    VkDescriptorSetLayout xrBlitSetLayout = VK_NULL_HANDLE;
    VkPipelineLayout xrBlitPipeLayout = VK_NULL_HANDLE;
    VkSampler xrBlitSampler = VK_NULL_HANDLE;
    VkShaderModule xrBlitVs = VK_NULL_HANDLE, xrBlitPs = VK_NULL_HANDLE;
    VkDescriptorPool xrBlitDescPool = VK_NULL_HANDLE;
    VkRenderPass xrBlitPass = VK_NULL_HANDLE;
    VkPipeline xrBlitPipe = VK_NULL_HANDLE;
    VkImageView xrBlitSrcView[4] = {VK_NULL_HANDLE, VK_NULL_HANDLE,
                                    VK_NULL_HANDLE, VK_NULL_HANDLE};
    VkImage xrBlitSrcOwner = VK_NULL_HANDLE; // sceneColor the views came from
    // One framebuffer per XR image, kept for the life of the swapchain.
    struct XrBlitTarget
    {
        VkImage img;
        VkImageView view;
        VkFramebuffer fb;
        int w, h;
    };
    std::vector<XrBlitTarget> xrBlitTargets;
    // A ring of sets rewritten per use: caching by source view would dangle
    // when an RTT is recreated. Each XR blit waits its own fence, and one
    // submit records at most four views, so nothing in flight is rewritten.
    enum
    {
        kXrBlitSets = 8
    };
    VkDescriptorSet xrBlitSet[kXrBlitSets] = {};
    unsigned int xrBlitSetNext = 0;

    // #107 GROUPED multiview, Option 2 -- per-eye TAIL pass. After the 2-view world+cockpit pass leaves the scene
    // color array in SHADER_READ, the RTT/2D tail (VCock_Exec/DrawRttQuad, screen-space via the per-eye camera) is
    // drawn PER LAYER through a SEPARATE single-view render pass (loadOp=LOAD color so world+cockpit survive, CLEAR
    // depth so the displays draw on top) targeting ONE array layer via a single-layer framebuffer. A DISTINCT pass
    // handle (not sceneRenderPass) so PurgePipelinesIfScenePassChanged never fires (the 2-view scene pass is stable)
    // and the tail's screen pipelines (keyed by (topo,pass)) coexist with the scene pipelines. Synchronous submit.
    VkRenderPass tailRenderPass = VK_NULL_HANDLE;
    VkImageView tailColorView[4] = {VK_NULL_HANDLE, VK_NULL_HANDLE,
                                    VK_NULL_HANDLE, VK_NULL_HANDLE};
    VkImageView tailDepthView[4] = {VK_NULL_HANDLE, VK_NULL_HANDLE,
                                    VK_NULL_HANDLE, VK_NULL_HANDLE};
    VkFramebuffer tailFbo[4] = {VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE,
                                VK_NULL_HANDLE};
    VkCommandBuffer tailCmd =
        VK_NULL_HANDLE; // ALIAS -> current ring slot (tailRing[tailCurSlot]) while recording
    VkFence tailFence =
        VK_NULL_HANDLE; // (legacy single fence -- unused once the ring is active; kept for init)
    // #107 PERF: per-eye TAIL command-buffer RING + per-slot fence. The old single tailCmd forced a synchronous
    // vkWaitForFences in EndTailView on EVERY eye (quad = 4 CPU->GPU stalls/frame), fully serializing CPU and GPU per
    // eye. The ring lets EndTailView submit WITHOUT waiting (CPU records the next eye while this eye's tail runs on the
    // GPU); the tail-writes-into-scene-layer -> blit-reads-it hazard is covered by ONE batched fence-wait in
    // BlitSceneToXrImages (WaitTailsConsumed), which waits every tail submitted since the last blit. kTailRing=4 =
    // quad's 4 eyes, so no intra-frame reuse; the next frame's BeginTailView waits the slot's fence (frame boundary).
    static const int kTailRing = 4;
    VkCommandBuffer tailRing[kTailRing] = {};
    VkFence tailRingFence[kTailRing] = {};
    int tailRingIdx = 0;
    int tailCurSlot = -1;
    int tailPending[kTailRing] =
        {}; // ring slots submitted since the last blit consume
    int tailPendingN = 0;
    bool tailRecording = false;
    bool tailActive = false;

    // #107 VR: one-shot command buffer + fence to blit the multiview scene layers into the per-view XR swapchain
    // images (VR-Vulkan present). Lazily created on first blit. Its OWN command pool -- a VkCommandPool is NOT thread-
    // safe, and the VR blit records on the SIM thread while the flat present records m->cmd[] on the UI thread; sharing
    // m->cmdPool corrupted the pool (crash inside the driver while recording a barrier). Dedicated pool = no race.
    VkCommandPool vrBlitPool = VK_NULL_HANDLE;
    VkCommandBuffer vrBlitCmd = VK_NULL_HANDLE;
    VkFence vrBlitFence = VK_NULL_HANDLE;

    // ---- RTT (HUD/MFD/DED render-target textures) -- see VulkanBackend::BindSceneRtt ----
    // RTT renders into a SEPARATE one-time command buffer (not interrupting the swapchain pass): BindSceneRtt begins
    // it + an RTT pass, the displays draw (FlatCommandBuffer() returns rttCmd while active), UnbindSceneRtt ends +
    // submits it. The render pass is compatible with the swapchain pass (same scFormat color + depth) so the
    // renderer's screen/dynamic2D pipelines are valid inside it.
    VkRenderPass rttRenderPass =
        VK_NULL_HANDLE; // color=scFormat CLEAR->STORE finalLayout SHADER_READ + depth
    // GM radar / mid-batch atlas re-binds: same pass but color loadOp=LOAD (initialLayout SHADER_READ), so a
    // re-bind PRESERVES what the texture already holds. Render-pass-compatible with rttRenderPass (only the
    // load/store ops differ), so the same framebuffers and screen pipelines are valid in both.
    VkRenderPass rttRenderPassLoad = VK_NULL_HANDLE;
    VkImage rttDepthImage =
        VK_NULL_HANDLE; // shared scratch depth for RTT (grown on demand)
    VkDeviceMemory rttDepthMem = VK_NULL_HANDLE;
    VkImageView rttDepthView = VK_NULL_HANDLE;
    int rttDepthW = 0, rttDepthH = 0;
    std::map<VkImageView, VkFramebuffer>
        rttFbCache; // one framebuffer per RTT color view (+ shared depth)
    VkCommandBuffer rttCmd =
        VK_NULL_HANDLE; // ALIAS -> the currently-recording ring slot (rttRing[rttCurSlot])
    // #107 PERF: RTT command-buffer RING + per-slot fence. The old single persistent rttCmd forced a full vkQueueWaitIdle
    // in UnbindSceneRtt on EVERY RTT (atlas + each MFD sensor sub-render, per eye = ~16 queue-drains/quad-frame) so the
    // one buffer was idle before the next bind reset it AND the panel could sample the finished texture. The ring lets
    // each RTT submit WITHOUT a wait (next bind takes a free slot); the sample/reuse hazard is covered by a single
    // fence-wait per CONSUMER batch (WaitRttConsumed, called before the tail/scene/blit submit that reads the RTTs).
    static const int kRttRing =
        16; // >= max RTTs recorded before one consumer batch (atlas + sensors + GM sweep re-binds)
    VkCommandBuffer rttRing[kRttRing] = {};
    VkFence rttRingFence[kRttRing] = {};
    int rttRingIdx = 0; // next free ring slot to hand out
    int rttCurSlot =
        -1; // ring slot of the CURRENTLY-open RTT (rttCmd aliases it)
    int rttLastSub =
        -1; // ring slot of the last SUBMITTED RTT (consumer waits its fence)
    bool rttNeedConsume =
        false; // an RTT was submitted and not yet consumed by a reader
    // #107 VR-Vulkan: the RTT command buffer is ALLOCATED + FREED per RTT bind, on the SIM thread. Those pool ops need
    // external synchronization; sharing m->cmdPool with the UI thread's flat-present record corrupted the pool (crash
    // in the driver while recording a later 2D draw -- e.g. the MFD radar). Its own pool removes that race.
    VkCommandPool rttPool = VK_NULL_HANDLE;
    bool rttActive = false; // an RTT pass is currently open
    void* rttBoundHandle =
        nullptr; // VulkanTexture* the open RTT pass targets (stale-unbind guard)
    int rttW = 0,
        rttH = 0; // the ACTIVE RTT's extent -- the render area a clear must
    // stay inside (the shared depth is only grown, so rttDepthW/H
    // is a high-water mark and cannot answer this)
    // Sensor-display zone rect (ConfineObjectViewportToZone -> SetViewportRect while an RTT is bound). The recorded
    // vkCmdSetViewport is fire-and-forget: any mid-batch BindSceneRtt (GM sweep sub-render, another display's
    // re-bind) opens a NEW ring command buffer whose viewport is reset to the full atlas -- and the sensor OBJECT
    // flush then draws at atlas scale (512/zoneW ~ 3.4x, centred) instead of the zone: "objects move faster than
    // the cursor". So the zone is remembered here and the RENDERER re-asserts it per path at record time: the
    // object path applies the zone, the 2D path applies the full atlas (its vertices are already atlas-zoned --
    // zoning the viewport too would double-transform them, the round-3 mistake). Valid only while the handle it
    // was confined against stays bound; cleared by the final UnbindSceneRtt.
    int rttZone[4] = {0, 0, 0, 0}; // x, y, w, h
    void* rttZoneHandle = nullptr; // atlas handle the zone belongs to
    unsigned rttVpSerial =
        0; // bumped whenever the recorded viewport state changes under the renderer (bind or SetViewportRect) --
    // the renderer's per-path viewport assert caches against it (ring buffers recycle VkCommandBuffer
    // pointers, so the pointer alone cannot key the cache)

    // #107 VR-Vulkan in-3D menu target (owned; the display RTTs' textures are owned by the texture manager, but the menu
    // has no draw-item, so the backend owns this one, mirroring D3D12Backend::m_pMenuRtt). Unlike the display RTTs this
    // is SCENE-FORMAT with its OWN single-view render pass: the exit dialog is a 3D BSP that draws through the object
    // pipelines, which are pass-locked to a scene-format pass -- an scFormat RTT pass is incompatible, so those draws
    // leaked into the eye (the empty quad the menu-in-focus experiments kept hitting). Its own command buffer + fence
    // because the eye scene owns sceneCmd at the same time. menuRttHandle mirrors {image,memory,view,0} so MenuRttTex /
    // BlitTexToXrImage read [0]=image, [2]=view.
    VkImage menuColor = VK_NULL_HANDLE;
    VkDeviceMemory menuColorMem = VK_NULL_HANDLE;
    VkImageView menuColorView = VK_NULL_HANDLE;
    VkImage menuDepth = VK_NULL_HANDLE;
    VkDeviceMemory menuDepthMem = VK_NULL_HANDLE;
    VkImageView menuDepthView = VK_NULL_HANDLE;
    VkRenderPass menuPass =
        VK_NULL_HANDLE; // single-view, sceneFormat color + depth -- object-pipeline compatible
    VkFramebuffer menuFbo = VK_NULL_HANDLE;
    VkCommandBuffer menuCmd =
        VK_NULL_HANDLE; // menu-pass one-time command buffer (own; sceneCmd is the eye's)
    VkFence menuFence = VK_NULL_HANDLE;
    bool menuActive =
        false; // a menu pass is open -> object/screen draws route to menuCmd
    uint64_t menuRttHandle[4] = {0, 0, 0, 0};
    int menuRttW = 0, menuRttH = 0;

    // #107 VR-Vulkan FPS quad: an owned scFormat RTT for the head-locked "FPS N" counter. Unlike the menu it is 2D text
    // only (no 3D dialog), so it rides the display-RTT path (BindSceneRtt -> rttCmd, scFormat), not the menu pass.
    VkImage fpsRttImage = VK_NULL_HANDLE;
    VkDeviceMemory fpsRttMem = VK_NULL_HANDLE;
    VkImageView fpsRttView = VK_NULL_HANDLE;
    uint64_t fpsRttHandle[4] = {0, 0, 0, 0};
    int fpsRttW = 0, fpsRttH = 0;
    // #59: subtitle-quad panel RTT (clone of the FPS one)
    VkImage subRttImage = VK_NULL_HANDLE;
    VkDeviceMemory subRttMem = VK_NULL_HANDLE;
    VkImageView subRttView = VK_NULL_HANDLE;
    uint64_t subRttHandle[4] = {0, 0, 0, 0};
    int subRttW = 0, subRttH = 0;

    uint32_t FindMemoryType(uint32_t typeBits,
                            VkMemoryPropertyFlags props) const;
};

// Artscout - 2026 (#104): defined below, used by the depth/render-pass creation above their definition point.
static void PickDepthFormat(VulkanBackend::Impl* m);
static VkImageAspectFlags DepthAspect(const VulkanBackend::Impl* m);

// ---------------------------------------------------------------------------------------------- helpers
uint32_t VulkanBackend::Impl::FindMemoryType(uint32_t typeBits,
                                             VkMemoryPropertyFlags props) const
{
    VkPhysicalDeviceMemoryProperties mp;
    vkGetPhysicalDeviceMemoryProperties(phys, &mp);
    for (uint32_t i = 0; i < mp.memoryTypeCount; ++i)
        if ((typeBits & (1u << i)) &&
            (mp.memoryTypes[i].propertyFlags & props) == props)
            return i;
    return 0;
}

static uint32_t ArgbToVkClear(unsigned long argb, VkClearColorValue& out)
{
    out.float32[0] = ((argb >> 16) & 0xff) / 255.0f; // R
    out.float32[1] = ((argb >> 8) & 0xff) / 255.0f; // G
    out.float32[2] = ((argb) & 0xff) / 255.0f; // B
    out.float32[3] = ((argb >> 24) & 0xff) / 255.0f; // A
    return 0;
}

// ---------------------------------------------------------------------------------------------- ctor/dtor
VulkanBackend::VulkanBackend() : m(new Impl())
{
}
VulkanBackend::~VulkanBackend()
{
    Release();
    delete m;
    m = nullptr;
}

// ---------------------------------------------------------------------------------------------- swapchain build/teardown
static void DestroySwapchainObjects(VulkanBackend::Impl* m)
{
    if (m->device == VK_NULL_HANDLE)
        return;
    vkDeviceWaitIdle(m->device);
    for (VkFramebuffer fb : m->framebuffers)
        if (fb)
            vkDestroyFramebuffer(m->device, fb, nullptr);
    m->framebuffers.clear();
    if (m->depthView)
    {
        vkDestroyImageView(m->device, m->depthView, nullptr);
        m->depthView = VK_NULL_HANDLE;
    }
    if (m->depthImage)
    {
        vkDestroyImage(m->device, m->depthImage, nullptr);
        m->depthImage = VK_NULL_HANDLE;
    }
    if (m->depthMem)
    {
        FF_VmaFree((void*)m->depthMem);
        m->depthMem = VK_NULL_HANDLE;
    }
    for (VkImageView v : m->scViews)
        if (v)
            vkDestroyImageView(m->device, v, nullptr);
    m->scViews.clear();
    for (VkSemaphore s : m->semRenderDone)
        if (s)
            vkDestroySemaphore(m->device, s, nullptr);
    m->semRenderDone.clear();
    // headless: scImages[0] is OUR offscreen color image (the swapchain owns its images, so only destroy in headless)
    if (m->headless)
    {
        for (VkImage img : m->scImages)
            if (img)
                vkDestroyImage(m->device, img, nullptr);
        if (m->offscreenColorMem)
        {
            FF_VmaFree((void*)m->offscreenColorMem);
            m->offscreenColorMem = VK_NULL_HANDLE;
        }
    }
    m->scImages.clear();
    if (m->swapchain)
    {
        vkDestroySwapchainKHR(m->device, m->swapchain, nullptr);
        m->swapchain = VK_NULL_HANDLE;
    }
}

static bool CreateSwapchain(VulkanBackend::Impl* m,
                            VkSwapchainKHR oldSc = VK_NULL_HANDLE)
{
    VkSurfaceCapabilitiesKHR caps;
    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m->phys, m->surface, &caps) !=
        VK_SUCCESS)
        return false;

    // surface format: prefer B8G8R8A8_UNORM
    uint32_t nfmt = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m->phys, m->surface, &nfmt, nullptr);
    std::vector<VkSurfaceFormatKHR> fmts(nfmt);
    vkGetPhysicalDeviceSurfaceFormatsKHR(m->phys, m->surface, &nfmt,
                                         fmts.data());
    VkSurfaceFormatKHR chosen =
        fmts.empty() ? VkSurfaceFormatKHR{VK_FORMAT_B8G8R8A8_UNORM,
                                          VK_COLORSPACE_SRGB_NONLINEAR_KHR} :
                       fmts[0];
    for (const auto& f : fmts)
        if (f.format == VK_FORMAT_B8G8R8A8_UNORM &&
            f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            chosen = f;
            break;
        }
    m->scFormat = chosen.format;

    // present mode: FIFO always available (vsync); MAILBOX if no-vsync requested and available
    VkPresentModeKHR present = VK_PRESENT_MODE_FIFO_KHR;
    if (!m->vsync)
    {
        uint32_t npm = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(m->phys, m->surface, &npm,
                                                  nullptr);
        std::vector<VkPresentModeKHR> pms(npm);
        vkGetPhysicalDeviceSurfacePresentModesKHR(m->phys, m->surface, &npm,
                                                  pms.data());
        for (auto p : pms)
            if (p == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                present = p;
                break;
            }
    }

    VkExtent2D ext = caps.currentExtent;
    if (ext.width == 0xFFFFFFFF)
    {
        ext.width =
            std::max(caps.minImageExtent.width,
                     std::min(caps.maxImageExtent.width, (uint32_t)m->width));
        ext.height =
            std::max(caps.minImageExtent.height,
                     std::min(caps.maxImageExtent.height, (uint32_t)m->height));
    }
    if (ext.width == 0 || ext.height == 0)
        return false; // minimized
    m->scExtent = ext;

    uint32_t imgCount = caps.minImageCount + 1;
    if (caps.maxImageCount > 0 && imgCount > caps.maxImageCount)
        imgCount = caps.maxImageCount;

    VkSwapchainCreateInfoKHR ci{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    ci.surface = m->surface;
    ci.minImageCount = imgCount;
    ci.imageFormat = m->scFormat;
    ci.imageColorSpace = chosen.colorSpace;
    ci.imageExtent = ext;
    ci.imageArrayLayers = 1;
    ci.imageUsage =
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    ci.preTransform = caps.currentTransform;
    ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    ci.presentMode = present;
    ci.clipped = VK_TRUE;
    ci.oldSwapchain =
        oldSc; // #104: hand the retiring swapchain to the driver for a clean resize transition
    if (vkCreateSwapchainKHR(m->device, &ci, nullptr, &m->swapchain) !=
        VK_SUCCESS)
        return false;

    uint32_t n = 0;
    vkGetSwapchainImagesKHR(m->device, m->swapchain, &n, nullptr);
    m->scImages.resize(n);
    vkGetSwapchainImagesKHR(m->device, m->swapchain, &n, m->scImages.data());

    m->scViews.resize(n);
    for (uint32_t i = 0; i < n; ++i)
    {
        VkImageViewCreateInfo iv{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        iv.image = m->scImages[i];
        iv.viewType = VK_IMAGE_VIEW_TYPE_2D;
        iv.format = m->scFormat;
        iv.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        if (vkCreateImageView(m->device, &iv, nullptr, &m->scViews[i]) !=
            VK_SUCCESS)
            return false;
    }

    // Artscout - 2026 (#104): one render-done semaphore per swapchain image (indexed by acquired imgIdx at present).
    m->semRenderDone.resize(n, VK_NULL_HANDLE);
    {
        VkSemaphoreCreateInfo sci{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        for (uint32_t i = 0; i < n; ++i)
            if (vkCreateSemaphore(m->device, &sci, nullptr,
                                  &m->semRenderDone[i]) != VK_SUCCESS)
                return false;
    }

    // depth
    VkImageCreateInfo di{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    di.imageType = VK_IMAGE_TYPE_2D;
    di.format = m->depthFormat;
    di.extent = {ext.width, ext.height, 1};
    di.mipLevels = 1;
    di.arrayLayers = 1;
    di.samples = VK_SAMPLE_COUNT_1_BIT;
    di.tiling = VK_IMAGE_TILING_OPTIMAL;
    di.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    di.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    if (vkCreateImage(m->device, &di, nullptr, &m->depthImage) != VK_SUCCESS)
        return false;
    if (not FF_VmaAllocImageMemory(m->depthImage, (void**)&m->depthMem))
        return false;
    VkImageViewCreateInfo dv{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    dv.image = m->depthImage;
    dv.viewType = VK_IMAGE_VIEW_TYPE_2D;
    dv.format = m->depthFormat;
    dv.subresourceRange = {DepthAspect(m), 0, 1, 0, 1};
    if (vkCreateImageView(m->device, &dv, nullptr, &m->depthView) != VK_SUCCESS)
        return false;

    // framebuffers
    m->framebuffers.resize(n);
    for (uint32_t i = 0; i < n; ++i)
    {
        VkImageView atts[2] = {m->scViews[i], m->depthView};
        VkFramebufferCreateInfo fb{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        fb.renderPass = m->renderPass;
        fb.attachmentCount = 2;
        fb.pAttachments = atts;
        fb.width = ext.width;
        fb.height = ext.height;
        fb.layers = 1;
        if (vkCreateFramebuffer(m->device, &fb, nullptr, &m->framebuffers[i]) !=
            VK_SUCCESS)
            return false;
    }
    return true;
}

static bool CreateRenderPass(VulkanBackend::Impl* m)
{
    VkAttachmentDescription color{};
    color.format = m->scFormat;
    color.samples = VK_SAMPLE_COUNT_1_BIT;
    color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depth{};
    depth.format = m->depthFormat;
    depth.samples = VK_SAMPLE_COUNT_1_BIT;
    depth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    // #76: the stencil plane is real state within the pass (the glass marks it, the symbology tests it), so it must
    // be cleared with the depth and kept -- DONT_CARE would leave the aperture mask undefined.
    depth.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
    depth.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorRef{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkAttachmentReference depthRef{
        1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
    VkSubpassDescription sub{};
    sub.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    sub.colorAttachmentCount = 1;
    sub.pColorAttachments = &colorRef;
    sub.pDepthStencilAttachment = &depthRef;

    VkSubpassDependency dep{};
    dep.srcSubpass = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass = 0;
    dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                       VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                       VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dep.srcAccessMask = 0;
    dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkAttachmentDescription atts[2] = {color, depth};
    VkRenderPassCreateInfo rp{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    rp.attachmentCount = 2;
    rp.pAttachments = atts;
    rp.subpassCount = 1;
    rp.pSubpasses = &sub;
    rp.dependencyCount = 1;
    rp.pDependencies = &dep;
    return vkCreateRenderPass(m->device, &rp, nullptr, &m->renderPass) ==
           VK_SUCCESS;
}

// ------------------------------------------------------------------------------------- validation layers (#104)
// Artscout - 2026 (#104): Vulkan has no runtime error checking of its own -- a bad barrier, a stale descriptor or a
// missing sync just produces a wrong picture, a driver deadlock or DEVICE_LOST with nothing in the log. The validation
// layer is the debugger for exactly that class of bug, so it is wired here and routed into VKB_LOG.
//   g_bVulkanValidation     -- the KHRONOS validation layer (object lifetimes, descriptor/state, API misuse).
//   g_bVulkanSyncValidation -- SYNCHRONIZATION validation on top: names the exact missing barrier / read-after-write
//                              hazard between submits. This is the one that finds hangs, and it is the expensive one.
// Both default ON while the #104 freeze is being chased; turn them OFF here for a normal/perf run. Needs the layer
// installed (Vulkan SDK on Windows, vulkan-validationlayers on Linux) -- if absent we log once and run without it.
// Artscout - 2026 (#104): OFF now that the log is clean. They are not free -- SYNCHRONIZATION validation in
// particular tracks every access to every resource across submits and cost roughly an order of magnitude here
// (the frame rate fell to ~8 fps with both on). Flip either back on to chase a new hang or corruption; that is
// what they are for, and they earned their keep: the queue/pool races, the RTT format mismatch and the
// descriptor-reset hazard were all found by them, not by reading code.
bool g_bVulkanValidation =
    false; // OFF: the hangs/regressions are closed. Flip on to chase a new hazard.
bool g_bVulkanSyncValidation =
    false; // sync validation costs ~an order of magnitude -- on only when hunting a stall

namespace
{
// Per-message-id log budget: validation happily repeats the same complaint every frame, which buries the log and
// tanks the framerate to the point the original timing bug stops reproducing. Log each id a few times, then hush.
constexpr unsigned kVkDbgMaxPerId = 8;
std::mutex s_vkDbgMutex;
std::map<int32_t, unsigned> s_vkDbgSeen;

// Everything vkCreateInstance's pNext chain points at must outlive the call, so it lives in one struct the caller
// keeps on its stack. Also carries the messenger create-info we reuse for the persistent (instance-lifetime) one.
struct ValidationSetup
{
    std::vector<const char*> layers;
    VkValidationFeatureEnableEXT enables[2] = {};
    VkValidationFeaturesEXT features{VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT};
    VkDebugUtilsMessengerCreateInfoEXT dbgCi{
        VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
    bool debugUtils = false;
};
}

static VKAPI_ATTR VkBool32 VKAPI_CALL
VkbDebugCb(VkDebugUtilsMessageSeverityFlagBitsEXT sev,
           VkDebugUtilsMessageTypeFlagsEXT types,
           const VkDebugUtilsMessengerCallbackDataEXT* data, void*)
{
    if (!data)
        return VK_FALSE;
    { // callbacks arrive on whatever thread made the call (render + loader threads) -- keep the counter sane
        std::lock_guard<std::mutex> lk(s_vkDbgMutex);
        unsigned& n = s_vkDbgSeen[data->messageIdNumber];
        if (++n > kVkDbgMaxPerId)
            return VK_FALSE;
        if (n == kVkDbgMaxPerId)
        {
            VKB_LOG("VALIDATION: id %s repeated %u times -- further "
                    "occurrences suppressed\n",
                    data->pMessageIdName ? data->pMessageIdName : "?",
                    kVkDbgMaxPerId);
            return VK_FALSE;
        }
    }
    const char* tag =
        (sev & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)   ? "ERROR" :
        (sev & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) ? "WARN" :
                                                                  "INFO";
    const char* kind =
        (types & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)  ? "VALID" :
        (types & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) ? "PERF" :
                                                                    "GEN";
    VKB_LOG("VALIDATION %s/%s [%s]: %s\n", tag, kind,
            data->pMessageIdName ? data->pMessageIdName : "?",
            data->pMessage ? data->pMessage : "");
    return VK_FALSE; // never abort the offending call -- we want the frame to continue so the bug still reproduces
}

static bool VkbHasLayer(const char* name)
{
    uint32_t n = 0;
    vkEnumerateInstanceLayerProperties(&n, nullptr);
    std::vector<VkLayerProperties> p(n);
    if (n)
        vkEnumerateInstanceLayerProperties(&n, p.data());
    for (const auto& l : p)
        if (!strcmp(l.layerName, name))
            return true;
    return false;
}
static bool VkbHasInstanceExt(const char* name)
{
    uint32_t n = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &n, nullptr);
    std::vector<VkExtensionProperties> p(n);
    if (n)
        vkEnumerateInstanceExtensionProperties(nullptr, &n, p.data());
    for (const auto& e : p)
        if (!strcmp(e.extensionName, name))
            return true;
    return false;
}

// Artscout - 2026 (#78): device-level twin of the above -- mesh shaders are an
// extension, not core, so presence must be checked before the feature query.
static bool VkbHasDeviceExt(VkPhysicalDevice phys, const char* name)
{
    if (phys == VK_NULL_HANDLE)
        return false;
    uint32_t n = 0;
    vkEnumerateDeviceExtensionProperties(phys, nullptr, &n, nullptr);
    std::vector<VkExtensionProperties> p(n);
    if (n)
        vkEnumerateDeviceExtensionProperties(phys, nullptr, &n, p.data());
    for (const auto& e : p)
        if (!strcmp(e.extensionName, name))
            return true;
    return false;
}

// Appends the layer/extension/pNext bits to `ici` + `exts`. Never fails the init: if validation is off or the layer
// is not installed, `ici` is left alone and the instance comes up exactly as before.
static void VkbSetupValidation(ValidationSetup& v,
                               std::vector<const char*>& exts,
                               VkInstanceCreateInfo& ici)
{
    if (!g_bVulkanValidation)
        return;
    if (!VkbHasLayer("VK_LAYER_KHRONOS_validation"))
    {
        VKB_LOG("validation requested but VK_LAYER_KHRONOS_validation is NOT "
                "installed -- running without it "
                "(install the Vulkan SDK on Windows / vulkan-validationlayers "
                "on Linux)\n");
        return;
    }
    v.layers.push_back("VK_LAYER_KHRONOS_validation");
    ici.enabledLayerCount = (uint32_t)v.layers.size();
    ici.ppEnabledLayerNames = v.layers.data();

    if (VkbHasInstanceExt(VK_EXT_DEBUG_UTILS_EXTENSION_NAME))
    {
        exts.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        v.debugUtils = true;
        v.dbgCi.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        v.dbgCi.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                              VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                              VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        v.dbgCi.pfnUserCallback = VkbDebugCb;
        ici.pNext =
            &v.dbgCi; // chained here too, so create/destroy-instance messages are not lost
    }
    if (g_bVulkanSyncValidation)
    {
        v.enables[0] =
            VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT;
        v.features.enabledValidationFeatureCount = 1;
        v.features.pEnabledValidationFeatures = v.enables;
        v.features.pNext =
            ici.pNext; // keep the messenger node in the chain behind us
        ici.pNext = &v.features;
    }
    VKB_LOG("validation ENABLED (sync=%d, debugUtils=%d) -- expect lower fps; "
            "set g_bVulkanValidation=false for a "
            "normal run\n",
            (int)g_bVulkanSyncValidation, (int)v.debugUtils);
}

static void VkbCreateDebugMessenger(VulkanBackend::Impl* m,
                                    const ValidationSetup& v)
{
    if (!v.debugUtils || m->instance == VK_NULL_HANDLE)
        return;
    auto fn = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        m->instance, "vkCreateDebugUtilsMessengerEXT");
    if (!fn)
        return;
    if (fn(m->instance, &v.dbgCi, nullptr, &m->debugMessenger) != VK_SUCCESS)
        VKB_LOG("vkCreateDebugUtilsMessengerEXT failed -- messages limited to "
                "instance create/destroy\n");
}
static void VkbDestroyDebugMessenger(VulkanBackend::Impl* m)
{
    if (!m->debugMessenger || m->instance == VK_NULL_HANDLE)
        return;
    auto fn = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        m->instance, "vkDestroyDebugUtilsMessengerEXT");
    if (fn)
        fn(m->instance, m->debugMessenger, nullptr);
    m->debugMessenger = VK_NULL_HANDLE;
}

// Artscout - 2026 (#104): choose the depth format once the physical device is known. Prefer D32_SFLOAT_S8_UINT --
// same shape as D3D12's D32_FLOAT_S8X24: full float depth for reversed-Z plus the stencil plane #76 needs. Vulkan
// guarantees at least one of D32_SFLOAT_S8_UINT / D24_UNORM_S8_UINT, so the stencil-less fallback should never be
// reached on real hardware; if it is, depthHasStencil stays false and the stencil paths disable themselves rather
// than the device failing to come up.
static void PickDepthFormat(VulkanBackend::Impl* m)
{
    const VkFormat wanted[] = {VK_FORMAT_D32_SFLOAT_S8_UINT,
                               VK_FORMAT_D24_UNORM_S8_UINT};
    for (VkFormat f : wanted)
    {
        VkFormatProperties fp{};
        vkGetPhysicalDeviceFormatProperties(m->phys, f, &fp);
        if (fp.optimalTilingFeatures &
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
        {
            m->depthFormat = f;
            m->depthHasStencil = true;
            return;
        }
    }
    m->depthFormat = VK_FORMAT_D32_SFLOAT;
    m->depthHasStencil = false;
    VKB_LOG("no depth+stencil format available -- HUD aperture stencil (#76) "
            "disabled\n");
}

// The depth aspect(s) to view/clear: stencil is only legal when the chosen format actually has that plane.
static VkImageAspectFlags DepthAspect(const VulkanBackend::Impl* m)
{
    return VK_IMAGE_ASPECT_DEPTH_BIT |
           (m->depthHasStencil ? VK_IMAGE_ASPECT_STENCIL_BIT : 0);
}

// ---------------------------------------------------------------------------------------------- Init
static bool RecreateSwapchain(
    VulkanBackend::Impl*
        m); // the one swapchain-recreate (oldSwapchain handoff); defined below

bool VulkanBackend::Init(HWND hWnd, int nWidth, int nHeight, int /*nDepth*/,
                         bool /*bFullscreen*/)
{
#ifndef _WIN32
    // #104 3D resolution: on Linux the swapchain follows the surface's currentExtent (i.e. the real SDL window
    // size), so a requested render resolution only takes effect if the window itself is resized. Do it here --
    // before any swapchain (re)create -- so both the first init and the menu->3D re-init below land at the target
    // size. (On Windows the DX path sizes the HWND separately; there the token is a real HWND, not a Window*.)
    if (ffplatform::Window* win = reinterpret_cast<ffplatform::Window*>(hWnd))
        win->SetSize(nWidth, nHeight);
#endif
    // Artscout - 2026 (#104): Init is called again on 3D entry (menu res -> 3D res). If the device is already up,
    // do NOT recreate the instance/device/surface -- the renderer + resource managers borrowed the device handle,
    // and a second surface/swapchain on the same HWND fails with VK_ERROR_NATIVE_WINDOW_IN_USE_KHR ("swapchain
    // failed"). Just rebuild the swapchain at the new size on the existing device/surface.
    if (m->device != VK_NULL_HANDLE)
    {
        // 3D entry / exit-to-menu: keep the device/surface, rebuild the swapchain at the new size through the ONE
        // recreate path (oldSwapchain handoff + semaphore reset), same as a runtime Resize.
        m->width = nWidth;
        m->height = nHeight;
        if (!RecreateSwapchain(m))
        {
            VKB_LOG("re-init swapchain failed (%dx%d)\n", nWidth, nHeight);
            return false;
        }
        VKB_LOG("re-init OK: %dx%d, %zu images\n", nWidth, nHeight,
                m->scImages.size());
        return true;
    }

    m->hwndToken = hWnd;
    m->width = nWidth;
    m->height = nHeight;

    // ---- instance extensions (surface) ----
    std::vector<const char*> exts;
#ifdef _WIN32
    exts.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
    exts.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#else
    // Linux: the HWND token IS the ffplatform::Window*; SDL enumerates the surface extensions.
    ffplatform::Window* win = reinterpret_cast<ffplatform::Window*>(hWnd);
    m->sdlWindow = win ? win->GetSdlWindow() : nullptr;
    if (!m->sdlWindow)
    {
        VKB_LOG("Init: no SDL window from HWND token %p\n", (void*)hWnd);
        return false;
    }
    Uint32 nExt = 0;
    const char* const* sdlExt = SDL_Vulkan_GetInstanceExtensions(&nExt);
    exts.assign(sdlExt, sdlExt + nExt);
#endif

    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.pApplicationName = "FreeFalcon";
    app.apiVersion = VK_API_VERSION_1_2;
    VkInstanceCreateInfo ici{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    ici.pApplicationInfo = &app;
    ValidationSetup
        vsetup; // must outlive vkCreateInstance -- ici.pNext points into it
    VkbSetupValidation(
        vsetup, exts,
        ici); // may append to exts, so do this BEFORE handing exts to ici
    // #107: append the OpenXR runtime's required instance extensions (VR session interop), skipping dups. The strings
    // live in m->vrInstExts (Impl), so their c_str() stays valid through vkCreateInstance.
    for (const std::string& e : m->vrInstExts)
    {
        bool dup = false;
        for (const char* x : exts)
            if (e == x)
            {
                dup = true;
                break;
            }
        if (!dup)
            exts.push_back(e.c_str());
    }
    ici.enabledExtensionCount = (uint32_t)exts.size();
    ici.ppEnabledExtensionNames = exts.data();
    // #109 enable2: in VR mode the OpenXR runtime wraps vkCreateInstance (adding its interop extensions). It still
    // consumes OUR ici (surface + validation), so the surface/validation setup above is unchanged. Flat path = plain call.
    if (m->hookCreateInstance)
    {
        VkResult _vr =
            (VkResult)m->hookCreateInstance(&ici, &m->instance, m->hookUser);
        if (_vr != VK_SUCCESS)
        {
            VKB_LOG(
                "Init: OpenXR CreateVulkanInstance hook failed (VkResult %d)\n",
                (int)_vr);
            return false;
        }
    }
    else
    {
        VK_CHECK(vkCreateInstance(&ici, nullptr, &m->instance));
    }
    VkbCreateDebugMessenger(m, vsetup);

    // ---- surface ----
#ifdef _WIN32
    VkWin32SurfaceCreateInfoKHR wsci{
        VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
    wsci.hinstance = GetModuleHandle(nullptr);
    wsci.hwnd = (HWND)hWnd; // the real Win32 window
    VK_CHECK(vkCreateWin32SurfaceKHR(m->instance, &wsci, nullptr, &m->surface));
#else
    if (!SDL_Vulkan_CreateSurface((SDL_Window*)m->sdlWindow, m->instance,
                                  nullptr, &m->surface))
    {
        VKB_LOG("SDL_Vulkan_CreateSurface failed: %s\n", SDL_GetError());
        return false;
    }
#endif

    // ---- physical device + graphics/present queue family ----
    // #109 enable2: in VR mode the OpenXR runtime NAMES the GPU it needs (xrGetVulkanGraphicsDevice2KHR -- must be the
    // HMD's adapter). Restrict the search to that device; otherwise enumerate all. Either way the queue-family scan below
    // is identical (still needs a graphics+present family on the chosen device).
    std::vector<VkPhysicalDevice> devs;
    VkPhysicalDevice _xrPhys = VK_NULL_HANDLE;
    if (m->hookPickPhysical &&
        (VkResult)m->hookPickPhysical((void*)m->instance, &_xrPhys,
                                      m->hookUser) == VK_SUCCESS &&
        _xrPhys != VK_NULL_HANDLE)
    {
        devs.push_back(_xrPhys);
    }
    else
    {
        uint32_t nphys = 0;
        vkEnumeratePhysicalDevices(m->instance, &nphys, nullptr);
        if (!nphys)
        {
            VKB_LOG("no Vulkan physical devices\n");
            return false;
        }
        devs.resize(nphys);
        vkEnumeratePhysicalDevices(m->instance, &nphys, devs.data());
    }
    bool found = false;
    for (VkPhysicalDevice pd : devs)
    {
        uint32_t nq = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(pd, &nq, nullptr);
        std::vector<VkQueueFamilyProperties> qf(nq);
        vkGetPhysicalDeviceQueueFamilyProperties(pd, &nq, qf.data());
        for (uint32_t i = 0; i < nq; ++i)
        {
            VkBool32 present = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(pd, i, m->surface, &present);
            if ((qf[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && present)
            {
                m->phys = pd;
                m->gfxFamily = i;
                found = true;
                break;
            }
        }
        if (found)
            break;
    }
    if (!found)
    {
        VKB_LOG("no graphics+present queue family\n");
        return false;
    }
    PickDepthFormat(
        m); // #104: needs the physical device; must precede any depth image / render pass

    // ---- query multiview support (VK_KHR_multiview -- core since 1.1; we request 1.2) ----
    VkPhysicalDeviceMultiviewFeatures mvFeat{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES};
    // #107 PERF bindless: query descriptor-indexing (VK_EXT_descriptor_indexing / core 1.2) -- an unbounded texture
    // array sampled by a per-primitive index lets the whole terrain draw with ONE bind (D3D12-style), killing the
    // ~13k per-tile draws + per-draw descriptor churn.
    VkPhysicalDeviceDescriptorIndexingFeatures diFeat{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES};
    mvFeat.pNext = &diFeat;
    // Artscout - 2026 (#78): mesh/task shaders. Unlike descriptor-indexing this
    // is NOT core in 1.2, so the extension must be present before we query it.
    const bool meshExtPresent =
        VkbHasDeviceExt(m->phys, VK_EXT_MESH_SHADER_EXTENSION_NAME);
    VkPhysicalDeviceMeshShaderFeaturesEXT msFeat{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT};
    if (meshExtPresent)
        diFeat.pNext = &msFeat;
    VkPhysicalDeviceFeatures2 feat2{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
    feat2.pNext = &mvFeat;
    vkGetPhysicalDeviceFeatures2(m->phys, &feat2);
    m->multiviewOk = (mvFeat.multiview == VK_TRUE);
    m->bindlessOk =
        (diFeat.runtimeDescriptorArray == VK_TRUE) &&
        (diFeat.shaderSampledImageArrayNonUniformIndexing == VK_TRUE) &&
        (diFeat.descriptorBindingPartiallyBound == VK_TRUE) &&
        (diFeat.descriptorBindingSampledImageUpdateAfterBind == VK_TRUE);
    // Both stages are required: the terrain culls in the task shader.
    m->meshShaderOk = meshExtPresent && (msFeat.meshShader == VK_TRUE) &&
                      (msFeat.taskShader == VK_TRUE);
    VKB_LOG("Init: mesh shaders %s\n",
            m->meshShaderOk ? "supported" :
                              (meshExtPresent ? "extension present, features off" :
                                                "unsupported"));

    // ---- logical device + queue (chain multiview feature enablement) ----
    // #107 VR-Vulkan: request a SECOND graphics queue for the OpenXR runtime. KHR_vulkan_enable shares the app's
    // VkQueue with the runtime, and the runtime submits/compositing there during xrEndFrame -- concurrently with the
    // app's own submits (flat present on the UI thread, blits) -> unsynchronized vkQueueSubmit on ONE VkQueue corrupts
    // it (DEVICE_LOST, seen as the runtime's vkGetFenceStatus -4). Giving the runtime queueIndex 1 (a separate VkQueue
    // in the same family) removes the shared-queue race. Falls back to index 0 if the family exposes only one queue.
    uint32_t famQueueCount = 1;
    {
        uint32_t nq = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(m->phys, &nq, nullptr);
        std::vector<VkQueueFamilyProperties> qfp(nq);
        vkGetPhysicalDeviceQueueFamilyProperties(m->phys, &nq, qfp.data());
        if (m->gfxFamily < nq)
            famQueueCount = qfp[m->gfxFamily].queueCount;
    }
    const uint32_t wantQueues = (famQueueCount >= 2) ? 2u : 1u;
    m->xrQueueIndex = (wantQueues >= 2) ? 1u : 0u;
    float prio[2] = {1.0f, 1.0f};
    VkDeviceQueueCreateInfo qci{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    qci.queueFamilyIndex = m->gfxFamily;
    qci.queueCount = wantQueues;
    qci.pQueuePriorities = prio;
    std::vector<const char*> devExts;
    devExts.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    if (m->bindlessOk)
        devExts.push_back(
            VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME); // #107 PERF: bindless terrain
    if (m->meshShaderOk)
        devExts.push_back(
            VK_EXT_MESH_SHADER_EXTENSION_NAME); // #78: GPU-driven terrain
    // #107: append the OpenXR runtime's required device extensions (VR compositor image sharing), skipping dups.
    for (const std::string& e : m->vrDevExts)
    {
        bool dup = false;
        for (const char* x : devExts)
            if (e == x)
            {
                dup = true;
                break;
            }
        if (!dup)
            devExts.push_back(e.c_str());
    }
    VkPhysicalDeviceMultiviewFeatures mvEnable{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES};
    mvEnable.multiview = m->multiviewOk ? VK_TRUE : VK_FALSE;
    // #107 PERF bindless: enable the descriptor-indexing features we checked above; chain after multiview.
    VkPhysicalDeviceDescriptorIndexingFeatures diEnable{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES};
    if (m->bindlessOk)
    {
        diEnable.runtimeDescriptorArray = VK_TRUE;
        diEnable.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
        diEnable.descriptorBindingPartiallyBound = VK_TRUE;
        diEnable.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
        diEnable.descriptorBindingUpdateUnusedWhilePending = VK_TRUE;
        mvEnable.pNext = &diEnable;
    }
    // #78: chain mesh/task enablement after whichever struct is last.
    VkPhysicalDeviceMeshShaderFeaturesEXT msEnable{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT};
    if (m->meshShaderOk)
    {
        msEnable.meshShader = VK_TRUE;
        msEnable.taskShader = VK_TRUE;
        // VR renders the scene through multiview; a mesh shader may only run in
        // such a pass when the device also offers multiviewMeshShader.
        msEnable.multiviewMeshShader =
            (m->multiviewOk && msFeat.multiviewMeshShader == VK_TRUE) ? VK_TRUE :
                                                                        VK_FALSE;
        if (m->bindlessOk)
            diEnable.pNext = &msEnable;
        else
            mvEnable.pNext = &msEnable;
    }
    VkPhysicalDeviceFeatures avail{};
    vkGetPhysicalDeviceFeatures(m->phys, &avail);
    VkPhysicalDeviceFeatures feats{};
    feats.textureCompressionBC =
        avail
            .textureCompressionBC; // native DXT/BC sampling (on-disk texture format)
    feats.samplerAnisotropy = avail.samplerAnisotropy;
    VkDeviceCreateInfo dci{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    dci.pNext = &mvEnable;
    dci.queueCreateInfoCount = 1;
    dci.pQueueCreateInfos = &qci;
    dci.enabledExtensionCount = (uint32_t)devExts.size();
    dci.ppEnabledExtensionNames = devExts.data();
    dci.pEnabledFeatures = &feats;
    // #109 enable2: in VR mode the OpenXR runtime wraps vkCreateDevice (adding its interop device extensions). It
    // consumes OUR dci (swapchain ext + 2 queues + multiview + BC/aniso features), so the queue/feature setup is
    // unchanged. Flat path = plain call.
    if (m->hookCreateDevice)
    {
        VkResult _vr = (VkResult)m->hookCreateDevice((void*)m->phys, &dci,
                                                     &m->device, m->hookUser);
        if (_vr != VK_SUCCESS)
        {
            VKB_LOG(
                "Init: OpenXR CreateVulkanDevice hook failed (VkResult %d)\n",
                (int)_vr);
            return false;
        }
    }
    else
    {
        VK_CHECK(vkCreateDevice(m->phys, &dci, nullptr, &m->device));
    }
    vkGetDeviceQueue(m->device, m->gfxFamily, 0, &m->queue);
    // #78: mesh-shader entry points are device-level; resolve or drop the path.
    if (m->meshShaderOk)
    {
        m->cmdDrawMeshTasks = (PFN_vkCmdDrawMeshTasksEXT)vkGetDeviceProcAddr(
            m->device, "vkCmdDrawMeshTasksEXT");
        if (!m->cmdDrawMeshTasks)
        {
            m->meshShaderOk = false;
            VKB_LOG("Init: vkCmdDrawMeshTasksEXT missing -> mesh path off\n");
        }
    }
    // Artscout - 2026: central GPU allocator (AMD VMA) -- every image/buffer below allocates through it.
    if (not FF_VmaInit(m->instance, m->phys, m->device, VK_API_VERSION_1_2))
        return false;
    // #107: the OpenXR runtime's queue (index 1 when available, else the shared index 0).
    vkGetDeviceQueue(m->device, m->gfxFamily, m->xrQueueIndex, &m->xrQueue);
    VKB_LOG("multiview=%d textureBC=%d (queues=%u, xrQueueIndex=%u)\n",
            (int)m->multiviewOk, (int)avail.textureCompressionBC, wantQueues,
            m->xrQueueIndex);

    // ---- command pool + per-frame command buffers + sync ----
    VkCommandPoolCreateInfo pci{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    pci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pci.queueFamilyIndex = m->gfxFamily;
    VK_CHECK(vkCreateCommandPool(m->device, &pci, nullptr, &m->cmdPool));
    // #107 VR-Vulkan: a SEPARATE pool for the VR blits -- they record on the SIM thread while the flat present records
    // m->cmd[] on the UI thread, and a VkCommandPool is not thread-safe (concurrent recording from one pool corrupts it
    // -> a crash inside the NVIDIA driver while recording a barrier). Its own pool removes the cross-thread race.
    VK_CHECK(vkCreateCommandPool(m->device, &pci, nullptr, &m->vrBlitPool));
    // #107: dedicated pool for the RTT command buffer (alloc/free per bind on the sim thread; must not share m->cmdPool
    // with the UI thread's present record -- concurrent pool ops corrupt it -> driver crash in a later 2D draw).
    VK_CHECK(vkCreateCommandPool(m->device, &pci, nullptr, &m->rttPool));

    VkCommandBufferAllocateInfo cbi{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    cbi.commandPool = m->cmdPool;
    cbi.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbi.commandBufferCount = kMaxFramesInFlight;
    VK_CHECK(vkAllocateCommandBuffers(m->device, &cbi, m->cmd));

    VkSemaphoreCreateInfo sci{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    VkFenceCreateInfo fci{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fci.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    for (int i = 0; i < kMaxFramesInFlight; ++i)
    {
        VK_CHECK(
            vkCreateSemaphore(m->device, &sci, nullptr, &m->semImageAvail[i]));
        VK_CHECK(vkCreateFence(m->device, &fci, nullptr, &m->fenceInFlight[i]));
    }
    // semRenderDone[] is per-swapchain-image; created inside CreateSwapchain(), destroyed in DestroySwapchainObjects().

    // a dedicated command buffer + fence for the multiview scene pass (VR/quad path)
    VkCommandBufferAllocateInfo scbi{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    scbi.commandPool = m->cmdPool;
    scbi.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    scbi.commandBufferCount = 1;
    VK_CHECK(vkAllocateCommandBuffers(m->device, &scbi, &m->sceneCmd));
    VK_CHECK(vkCreateFence(m->device, &fci, nullptr, &m->sceneFence));

    // #107 PERF: RING of command buffers + per-slot fences for the per-eye tail pass (submitted without a per-eye wait;
    // the blit batches one fence-wait over all pending tails). Fences created SIGNALED so the first BeginTailView reuse-
    // wait passes.
    for (int i = 0; i < Impl::kTailRing; ++i)
    {
        VK_CHECK(vkAllocateCommandBuffers(m->device, &scbi, &m->tailRing[i]));
        VK_CHECK(vkCreateFence(m->device, &fci, nullptr, &m->tailRingFence[i]));
    }
    m->tailCmd = m->tailRing[0]; // valid default alias

    if (!CreateRenderPass(m))
    {
        VKB_LOG("render pass failed\n");
        return false;
    }
    if (!CreateSwapchain(m))
    {
        VKB_LOG("swapchain failed\n");
        return false;
    }

    VKB_LOG("Init OK: %dx%d, %zu swapchain images\n", m->width, m->height,
            m->scImages.size());
    return true;
}

// ---------------------------------------------------------------------------------------------- InitHeadless
// Build an offscreen color+depth target into the sc* vectors so the flat frame loop drives it with no swapchain.
static bool CreateHeadlessTarget(VulkanBackend::Impl* m)
{
    m->scFormat = VK_FORMAT_B8G8R8A8_UNORM;
    m->scExtent = {(uint32_t)m->width, (uint32_t)m->height};

    // offscreen color image (COLOR_ATTACHMENT to draw into, TRANSFER_SRC to read back)
    VkImage color = VK_NULL_HANDLE;
    VkDeviceMemory colorMem = VK_NULL_HANDLE;
    VkImageView colorView = VK_NULL_HANDLE;
    VkImageCreateInfo ci{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    ci.imageType = VK_IMAGE_TYPE_2D;
    ci.format = m->scFormat;
    ci.extent = {m->scExtent.width, m->scExtent.height, 1};
    ci.mipLevels = 1;
    ci.arrayLayers = 1;
    ci.samples = VK_SAMPLE_COUNT_1_BIT;
    ci.tiling = VK_IMAGE_TILING_OPTIMAL;
    ci.usage =
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    if (vkCreateImage(m->device, &ci, nullptr, &color) != VK_SUCCESS)
        return false;
    if (not FF_VmaAllocImageMemory(color, (void**)&colorMem))
        return false;
    VkImageViewCreateInfo iv{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    iv.image = color;
    iv.viewType = VK_IMAGE_VIEW_TYPE_2D;
    iv.format = m->scFormat;
    iv.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    if (vkCreateImageView(m->device, &iv, nullptr, &colorView) != VK_SUCCESS)
        return false;
    m->scImages.push_back(color);
    m->scViews.push_back(colorView);
    // stash the color memory in depthMem's sibling slot? no -- track via a dedicated field reuse: we free it in
    // DestroySwapchainObjects by walking scImages; but scImages frees images only. Keep colorMem in a static-free
    // list: simplest is to bind it to the depth allocation cleanup below by storing in m->depthMem AFTER depth.
    // To avoid leaking, we free colorMem explicitly in Release (headless) via the offscreenColorMem field.
    m->offscreenColorMem = colorMem;

    // depth
    VkImageCreateInfo di{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    di.imageType = VK_IMAGE_TYPE_2D;
    di.format = m->depthFormat;
    di.extent = {m->scExtent.width, m->scExtent.height, 1};
    di.mipLevels = 1;
    di.arrayLayers = 1;
    di.samples = VK_SAMPLE_COUNT_1_BIT;
    di.tiling = VK_IMAGE_TILING_OPTIMAL;
    di.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    di.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    if (vkCreateImage(m->device, &di, nullptr, &m->depthImage) != VK_SUCCESS)
        return false;
    if (not FF_VmaAllocImageMemory(m->depthImage, (void**)&m->depthMem))
        return false;
    VkImageViewCreateInfo dv{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    dv.image = m->depthImage;
    dv.viewType = VK_IMAGE_VIEW_TYPE_2D;
    dv.format = m->depthFormat;
    dv.subresourceRange = {DepthAspect(m), 0, 1, 0, 1};
    if (vkCreateImageView(m->device, &dv, nullptr, &m->depthView) != VK_SUCCESS)
        return false;

    VkImageView atts[2] = {colorView, m->depthView};
    VkFramebufferCreateInfo fb{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    fb.renderPass = m->renderPass;
    fb.attachmentCount = 2;
    fb.pAttachments = atts;
    fb.width = m->scExtent.width;
    fb.height = m->scExtent.height;
    fb.layers = 1;
    VkFramebuffer f = VK_NULL_HANDLE;
    if (vkCreateFramebuffer(m->device, &fb, nullptr, &f) != VK_SUCCESS)
        return false;
    m->framebuffers.push_back(f);
    return true;
}

bool VulkanBackend::InitHeadless(int nWidth, int nHeight)
{
    m->headless = true;
    m->width = nWidth;
    m->height = nHeight;

    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.pApplicationName = "FreeFalcon-headless";
    app.apiVersion = VK_API_VERSION_1_2;
    VkInstanceCreateInfo ici{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    ici.pApplicationInfo = &app; // no surface extensions
    std::vector<const char*> exts; // empty unless validation adds debug-utils
    ValidationSetup
        vsetup; // must outlive vkCreateInstance -- ici.pNext points into it
    VkbSetupValidation(vsetup, exts, ici);
    ici.enabledExtensionCount = (uint32_t)exts.size();
    ici.ppEnabledExtensionNames = exts.empty() ? nullptr : exts.data();
    VK_CHECK(vkCreateInstance(&ici, nullptr, &m->instance));
    VkbCreateDebugMessenger(m, vsetup);

    uint32_t nphys = 0;
    vkEnumeratePhysicalDevices(m->instance, &nphys, nullptr);
    if (!nphys)
    {
        VKB_LOG("headless: no Vulkan physical devices\n");
        return false;
    }
    std::vector<VkPhysicalDevice> devs(nphys);
    vkEnumeratePhysicalDevices(m->instance, &nphys, devs.data());
    bool found = false;
    for (VkPhysicalDevice pd : devs)
    {
        uint32_t nq = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(pd, &nq, nullptr);
        std::vector<VkQueueFamilyProperties> qf(nq);
        vkGetPhysicalDeviceQueueFamilyProperties(pd, &nq, qf.data());
        for (uint32_t i = 0; i < nq; ++i)
            if (qf[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                m->phys = pd;
                m->gfxFamily = i;
                found = true;
                break;
            }
        if (found)
            break;
    }
    if (!found)
    {
        VKB_LOG("headless: no graphics queue family\n");
        return false;
    }
    PickDepthFormat(
        m); // #104: needs the physical device; must precede any depth image / render pass

    VkPhysicalDeviceMultiviewFeatures mvFeat{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES};
    VkPhysicalDeviceFeatures2 feat2{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
    feat2.pNext = &mvFeat;
    vkGetPhysicalDeviceFeatures2(m->phys, &feat2);
    m->multiviewOk = (mvFeat.multiview == VK_TRUE);

    float prio = 1.0f;
    VkDeviceQueueCreateInfo qci{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    qci.queueFamilyIndex = m->gfxFamily;
    qci.queueCount = 1;
    qci.pQueuePriorities = &prio;
    VkPhysicalDeviceMultiviewFeatures mvEnable{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES};
    mvEnable.multiview = m->multiviewOk ? VK_TRUE : VK_FALSE;
    VkPhysicalDeviceFeatures availH{};
    vkGetPhysicalDeviceFeatures(m->phys, &availH);
    VkPhysicalDeviceFeatures featsH{};
    featsH.textureCompressionBC = availH.textureCompressionBC;
    VkDeviceCreateInfo dci{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    dci.pNext = &mvEnable;
    dci.queueCreateInfoCount = 1;
    dci.pQueueCreateInfos = &qci; // no swapchain ext
    dci.pEnabledFeatures = &featsH;
    VK_CHECK(vkCreateDevice(m->phys, &dci, nullptr, &m->device));
    vkGetDeviceQueue(m->device, m->gfxFamily, 0, &m->queue);
    if (not FF_VmaInit(m->instance, m->phys, m->device, VK_API_VERSION_1_2))
        return false;

    VkCommandPoolCreateInfo pci{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    pci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pci.queueFamilyIndex = m->gfxFamily;
    VK_CHECK(vkCreateCommandPool(m->device, &pci, nullptr, &m->cmdPool));
    VkCommandBufferAllocateInfo cbi{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    cbi.commandPool = m->cmdPool;
    cbi.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbi.commandBufferCount = kMaxFramesInFlight;
    VK_CHECK(vkAllocateCommandBuffers(m->device, &cbi, m->cmd));
    VkSemaphoreCreateInfo sci{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    VkFenceCreateInfo fci{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fci.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    for (int i = 0; i < kMaxFramesInFlight; ++i)
    {
        VK_CHECK(
            vkCreateSemaphore(m->device, &sci, nullptr, &m->semImageAvail[i]));
        VK_CHECK(vkCreateFence(m->device, &fci, nullptr, &m->fenceInFlight[i]));
    }
    // semRenderDone[] is per-swapchain-image; created inside CreateSwapchain(), destroyed in DestroySwapchainObjects().
    VkCommandBufferAllocateInfo scbi{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    scbi.commandPool = m->cmdPool;
    scbi.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    scbi.commandBufferCount = 1;
    VK_CHECK(vkAllocateCommandBuffers(m->device, &scbi, &m->sceneCmd));
    VK_CHECK(vkCreateFence(m->device, &fci, nullptr, &m->sceneFence));

    if (!CreateRenderPass(m))
    {
        VKB_LOG("headless: render pass failed\n");
        return false;
    }
    if (!CreateHeadlessTarget(m))
    {
        VKB_LOG("headless: offscreen target failed\n");
        return false;
    }
    VKB_LOG("InitHeadless OK: %dx%d (multiview=%d)\n", m->width, m->height,
            (int)m->multiviewOk);
    return true;
}

void VulkanBackend::ReadbackColor(void* dstRgba)
{
    if (!IsValid() || !m->headless || m->scImages.empty() || !dstRgba)
        return;
    const VkDeviceSize bytes = (VkDeviceSize)m->width * m->height * 4;
    // host-visible staging buffer
    VkBuffer buf = VK_NULL_HANDLE;
    VkDeviceMemory bufMem = VK_NULL_HANDLE;
    VkBufferCreateInfo bi{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bi.size = bytes;
    bi.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    bi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vkCreateBuffer(m->device, &bi, nullptr, &buf);
    void* bufMap = nullptr;
    if (not FF_VmaAllocReadbackMemory(buf, (void**)&bufMem, &bufMap))
    {
        vkDestroyBuffer(m->device, buf, nullptr);
        return;
    }

    VkCommandBuffer c = m->cmd[0];
    vkResetCommandBuffer(c, 0);
    VkCommandBufferBeginInfo cbi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    cbi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(c, &cbi);
    // color left PRESENT_SRC by the render pass finalLayout; transition to TRANSFER_SRC for the copy
    VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    b.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    b.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    b.srcQueueFamilyIndex = b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.image = m->scImages[0];
    b.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    b.srcAccessMask = 0;
    b.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    vkCmdPipelineBarrier(c, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                         nullptr, 1, &b);
    VkBufferImageCopy cp{};
    cp.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    cp.imageExtent = {(uint32_t)m->width, (uint32_t)m->height, 1};
    vkCmdCopyImageToBuffer(c, m->scImages[0],
                           VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buf, 1, &cp);
    vkEndCommandBuffer(c);
    VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    si.commandBufferCount = 1;
    si.pCommandBuffers = &c;
    {
        std::lock_guard<std::mutex> _qlk(g_vkQueueMutex);
        vkQueueSubmit(m->queue, 1, &si, VK_NULL_HANDLE);
        vkQueueWaitIdle(m->queue);
    }
    // swapchain color is BGRA; swizzle to RGBA for the caller
    const uint8_t* src = (const uint8_t*)bufMap;
    uint8_t* dst = (uint8_t*)dstRgba;
    for (VkDeviceSize i = 0; i < bytes; i += 4)
    {
        dst[i + 0] = src[i + 2];
        dst[i + 1] = src[i + 1];
        dst[i + 2] = src[i + 0];
        dst[i + 3] = src[i + 3];
    }
    vkDestroyBuffer(m->device, buf, nullptr);
    FF_VmaFree((void*)bufMem);
}

bool VulkanBackend::DepthHasStencil() const
{
    return m && m->depthHasStencil;
}
unsigned VulkanBackend::FrameSlot() const
{
    return m ? (unsigned)m->frameSlot : 0u;
}

// Artscout - 2026: the sRGB-decoding XR blit owns views into the scene target,
// so both are torn down wherever that target is (see SetXrColorFormat).
static void DestroyXrBlit(VulkanBackend::Impl* m);
static void DestroyXrBlitSrcViews(VulkanBackend::Impl* m);

// ---------------------------------------------------------------------------------------------- Release
void VulkanBackend::Release()
{
    if (!m || m->device == VK_NULL_HANDLE)
    {
        if (m && m->instance)
        {
            VkbDestroyDebugMessenger(m);
            if (m->surface)
                vkDestroySurfaceKHR(m->instance, m->surface, nullptr);
            vkDestroyInstance(m->instance, nullptr);
            m->surface = VK_NULL_HANDLE;
            m->instance = VK_NULL_HANDLE;
        }
        return;
    }
    vkDeviceWaitIdle(m->device);
    DestroyXrBlit(m); // holds views into the scene target -- free it first
    // scene multiview target
    if (m->sceneFbo)
        vkDestroyFramebuffer(m->device, m->sceneFbo, nullptr);
    if (m->sceneColorView)
        vkDestroyImageView(m->device, m->sceneColorView, nullptr);
    if (m->sceneColor)
        vkDestroyImage(m->device, m->sceneColor, nullptr);
    if (m->sceneColorMem)
        FF_VmaFree((void*)m->sceneColorMem);
    if (m->sceneDepthView)
        vkDestroyImageView(m->device, m->sceneDepthView, nullptr);
    if (m->sceneDepth)
        vkDestroyImage(m->device, m->sceneDepth, nullptr);
    if (m->sceneDepthMem)
        FF_VmaFree((void*)m->sceneDepthMem);
    if (m->sceneRenderPass)
        vkDestroyRenderPass(m->device, m->sceneRenderPass, nullptr);
    if (m->sceneFence)
        vkDestroyFence(m->device, m->sceneFence, nullptr);
    if (m->tailFence)
        vkDestroyFence(m->device, m->tailFence,
                       nullptr); // legacy single (may be unused)
    for (int i = 0; i < Impl::kTailRing;
         ++i) // #107 PERF: tail ring fences (cmd buffers freed with cmdPool)
        if (m->tailRingFence[i])
        {
            vkDestroyFence(m->device, m->tailRingFence[i], nullptr);
            m->tailRingFence[i] = VK_NULL_HANDLE;
        }
    m->tailRingIdx = 0;
    m->tailCurSlot = -1;
    m->tailPendingN = 0;
    m->sceneFbo = VK_NULL_HANDLE;
    m->sceneColorView = VK_NULL_HANDLE;
    m->sceneColor = VK_NULL_HANDLE;
    m->sceneColorMem = VK_NULL_HANDLE;
    m->sceneDepthView = VK_NULL_HANDLE;
    m->sceneDepth = VK_NULL_HANDLE;
    m->sceneDepthMem = VK_NULL_HANDLE;
    m->sceneRenderPass = VK_NULL_HANDLE;
    m->sceneFence = VK_NULL_HANDLE;
    // RTT (HUD/MFD/DED) resources
    for (auto& kv : m->rttFbCache)
        if (kv.second)
            vkDestroyFramebuffer(m->device, kv.second, nullptr);
    m->rttFbCache.clear();
    if (m->rttDepthView)
        vkDestroyImageView(m->device, m->rttDepthView, nullptr);
    if (m->rttDepthImage)
        vkDestroyImage(m->device, m->rttDepthImage, nullptr);
    if (m->rttDepthMem)
        FF_VmaFree((void*)m->rttDepthMem);
    if (m->rttRenderPass)
        vkDestroyRenderPass(m->device, m->rttRenderPass, nullptr);
    if (m->rttRenderPassLoad)
        vkDestroyRenderPass(m->device, m->rttRenderPassLoad, nullptr);
    m->rttDepthView = VK_NULL_HANDLE;
    m->rttDepthImage = VK_NULL_HANDLE;
    m->rttDepthMem = VK_NULL_HANDLE;
    m->rttRenderPass = VK_NULL_HANDLE;
    m->rttRenderPassLoad = VK_NULL_HANDLE;
    m->rttDepthW = m->rttDepthH = 0;

    // #107 VR-Vulkan in-3D menu target (owned: sceneFormat color + depth + own pass/fbo/cmd/fence)
    if (m->menuFence)
    {
        vkDestroyFence(m->device, m->menuFence, nullptr);
        m->menuFence = VK_NULL_HANDLE;
    }
    if (m->menuFbo)
        vkDestroyFramebuffer(m->device, m->menuFbo, nullptr);
    if (m->menuPass)
        vkDestroyRenderPass(m->device, m->menuPass, nullptr);
    if (m->menuColorView)
        vkDestroyImageView(m->device, m->menuColorView, nullptr);
    if (m->menuColor)
        vkDestroyImage(m->device, m->menuColor, nullptr);
    if (m->menuColorMem)
        FF_VmaFree((void*)m->menuColorMem);
    if (m->menuDepthView)
        vkDestroyImageView(m->device, m->menuDepthView, nullptr);
    if (m->menuDepth)
        vkDestroyImage(m->device, m->menuDepth, nullptr);
    if (m->menuDepthMem)
        FF_VmaFree((void*)m->menuDepthMem);
    m->menuFbo = VK_NULL_HANDLE;
    m->menuPass = VK_NULL_HANDLE;
    m->menuColorView = VK_NULL_HANDLE;
    m->menuColor = VK_NULL_HANDLE;
    m->menuColorMem = VK_NULL_HANDLE;
    m->menuDepthView = VK_NULL_HANDLE;
    m->menuDepth = VK_NULL_HANDLE;
    m->menuDepthMem = VK_NULL_HANDLE;
    m->menuActive = false;
    m->menuRttHandle[0] = m->menuRttHandle[1] = m->menuRttHandle[2] = 0;
    m->menuRttW = m->menuRttH = 0;
    // menuCmd is freed with rttPool below (allocated from it)

    // #107 VR-Vulkan FPS quad RTT (owned scFormat)
    if (m->fpsRttView)
        vkDestroyImageView(m->device, m->fpsRttView, nullptr);
    if (m->fpsRttImage)
        vkDestroyImage(m->device, m->fpsRttImage, nullptr);
    if (m->fpsRttMem)
        FF_VmaFree((void*)m->fpsRttMem);
    if (m->subRttView)
        vkDestroyImageView(m->device, m->subRttView,
                           nullptr); // #59 subtitle RTT
    if (m->subRttImage)
        vkDestroyImage(m->device, m->subRttImage, nullptr);
    if (m->subRttMem)
        FF_VmaFree((void*)m->subRttMem);
    m->fpsRttView = VK_NULL_HANDLE;
    m->fpsRttImage = VK_NULL_HANDLE;
    m->fpsRttMem = VK_NULL_HANDLE;
    m->fpsRttHandle[0] = m->fpsRttHandle[1] = m->fpsRttHandle[2] = 0;
    m->fpsRttW = m->fpsRttH = 0;

    DestroySwapchainObjects(m); // also destroys the per-image semRenderDone[]
    for (int i = 0; i < kMaxFramesInFlight; ++i)
    {
        if (m->semImageAvail[i])
            vkDestroySemaphore(m->device, m->semImageAvail[i], nullptr);
        if (m->fenceInFlight[i])
            vkDestroyFence(m->device, m->fenceInFlight[i], nullptr);
        m->semImageAvail[i] = VK_NULL_HANDLE;
        m->fenceInFlight[i] = VK_NULL_HANDLE;
    }
    if (m->vrBlitFence)
    {
        vkDestroyFence(m->device, m->vrBlitFence, nullptr);
        m->vrBlitFence = VK_NULL_HANDLE;
    } // #107 VR blit
    if (m->vrBlitPool)
    {
        vkDestroyCommandPool(m->device, m->vrBlitPool, nullptr);
        m->vrBlitPool = VK_NULL_HANDLE;
        m->vrBlitCmd = VK_NULL_HANDLE;
    }
    // #107 PERF: RTT ring -- the command buffers are freed with rttPool below; destroy their fences explicitly.
    for (int i = 0; i < Impl::kRttRing; ++i)
    {
        if (m->rttRingFence[i])
        {
            vkDestroyFence(m->device, m->rttRingFence[i], nullptr);
            m->rttRingFence[i] = VK_NULL_HANDLE;
        }
        m->rttRing[i] = VK_NULL_HANDLE;
    }
    m->rttRingIdx = 0;
    m->rttCurSlot = -1;
    m->rttLastSub = -1;
    m->rttNeedConsume = false;
    if (m->rttPool)
    {
        vkDestroyCommandPool(m->device, m->rttPool, nullptr);
        m->rttPool = VK_NULL_HANDLE;
        m->rttCmd = VK_NULL_HANDLE;
    } // #107 RTT
    if (m->cmdPool)
    {
        vkDestroyCommandPool(m->device, m->cmdPool, nullptr);
        m->cmdPool = VK_NULL_HANDLE;
    }
    if (m->tsPool)
    {
        vkDestroyQueryPool(m->device, m->tsPool, nullptr);
        m->tsPool = VK_NULL_HANDLE;
        m->tsPeriodNs = 0.0;
    } // #107 PERF
    if (m->renderPass)
    {
        vkDestroyRenderPass(m->device, m->renderPass, nullptr);
        m->renderPass = VK_NULL_HANDLE;
    }
    FF_VmaShutdown(); // all VMA-backed resources must already be destroyed above
    vkDestroyDevice(m->device, nullptr);
    m->device = VK_NULL_HANDLE;
    VkbDestroyDebugMessenger(m); // before the instance it was created from
    if (m->surface)
    {
        vkDestroySurfaceKHR(m->instance, m->surface, nullptr);
        m->surface = VK_NULL_HANDLE;
    }
    if (m->instance)
    {
        vkDestroyInstance(m->instance, nullptr);
        m->instance = VK_NULL_HANDLE;
    }
}

// Artscout - 2026 (#104): the ONE swapchain-recreate used by every resize trigger (present OUT_OF_DATE/SUBOPTIMAL,
// 3D entry, exit-to-menu). Steps that matter for a clean, hang-free transition on NVIDIA:
//   1. vkDeviceWaitIdle -- no in-flight work references the objects we're about to destroy.
//   2. Keep the RETIRING swapchain handle; null m->swapchain so DestroySwapchainObjects tears down only the dependent
//      objects (image views, depth, framebuffers, per-image semRenderDone) and leaves the old swapchain alive.
//   3. Recreate the image-available semaphores -- an aborted acquire (the OUT_OF_DATE that triggered us) can leave one
//      SIGNALED-but-unwaited; reusing it silently wedges presentation on NVIDIA (present SUCCESS, image frozen).
//   4. Create the new swapchain with oldSwapchain = the retiring one (driver hands surface ownership over cleanly --
//      NOT doing this leaked the old swapchain and left resize/exit intermittently stuck).
//   5. Destroy the retiring swapchain AFTER the new one is built.
// CreateSwapchain reads the surface's CURRENT extent, so the new swapchain always matches the real window size.
static bool RecreateSwapchain(VulkanBackend::Impl* m)
{
    if (m->device == VK_NULL_HANDLE)
        return false;
    // Hold the queue lock across the whole recreate: vkDeviceWaitIdle waits on ALL queues and, with the swapchain
    // teardown, must not race a loader thread's upload submit. None of the callees re-lock, so this can't self-deadlock.
    std::lock_guard<std::mutex> _qlk(g_vkQueueMutex);
    vkDeviceWaitIdle(m->device);
    VkSwapchainKHR old = m->swapchain;
    m->swapchain =
        VK_NULL_HANDLE; // so DestroySwapchainObjects keeps `old` alive for the handoff
    DestroySwapchainObjects(m);
    VkSemaphoreCreateInfo sci{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    for (int i = 0; i < kMaxFramesInFlight; ++i)
        if (m->semImageAvail[i])
        {
            vkDestroySemaphore(m->device, m->semImageAvail[i], nullptr);
            vkCreateSemaphore(m->device, &sci, nullptr, &m->semImageAvail[i]);
        }
    bool ok = CreateSwapchain(m, old);
    if (old)
        vkDestroySwapchainKHR(m->device, old, nullptr);
    if (!ok)
        VKB_LOG("RecreateSwapchain FAILED (%dx%d) -- swapchain left NULL, will "
                "retry next present\n",
                m->width, m->height);
    return ok;
}

bool VulkanBackend::Resize(int nWidth, int nHeight)
{
    if (!m || m->device == VK_NULL_HANDLE)
        return false;
    m->width = nWidth;
    m->height = nHeight;
    return RecreateSwapchain(m);
}

// Artscout - 2026 (#104): the DEFERRED-resize gate, run at the START of every present path (before acquire). This is
// the vkguide pattern: a present/acquire that returns OUT_OF_DATE/SUBOPTIMAL does NOT recreate the swapchain inline
// (recreating mid-frame -- amid command recording / a pending present -- is what left exit/resize intermittently
// hung); it just sets m->resizeRequested and bails. Here, at a clean frame boundary with nothing in flight, we do the
// actual RecreateSwapchain. Also covers a swapchain left NULL by a failed recreate (window mid-transition, extent 0):
// we retry every frame until the window settles. Returns false to skip this frame (present nothing) rather than stall.
static bool EnsureSwapchainReady(VulkanBackend* /*self*/,
                                 VulkanBackend::Impl* m)
{
    if (m->resizeRequested || m->swapchain == VK_NULL_HANDLE)
    {
        m->resizeRequested = false;
        if (!RecreateSwapchain(m) || m->swapchain == VK_NULL_HANDLE)
            return false;
    }
    return true;
}

bool VulkanBackend::IsValid() const
{
    return m && m->device != VK_NULL_HANDLE;
}

// ---------------------------------------------------------------------------------------------- frame loop
void VulkanBackend::BeginFrame(unsigned long argbClearColor)
{
    if (!IsValid() || m->recording)
        return;
    if (!m->headless && !EnsureSwapchainReady(this, m))
        return; // swapchain lost + window not ready -> skip frame
    m->clearArgb = argbClearColor;
    m->frameSlot = (m->frameSlot + 1) % kMaxFramesInFlight;

    vkWaitForFences(m->device, 1, &m->fenceInFlight[m->frameSlot], VK_TRUE,
                    UINT64_MAX);

    if (m->headless)
    {
        m->imageIndex = 0; // the single offscreen target; no acquire
    }
    else
    {
        VkResult acq = vkAcquireNextImageKHR(
            m->device, m->swapchain, UINT64_MAX, m->semImageAvail[m->frameSlot],
            VK_NULL_HANDLE, &m->imageIndex);
        if (acq == VK_ERROR_OUT_OF_DATE_KHR)
        {
            m->resizeRequested = true;
            m->frameOk = false;
            return;
        }
        if (acq != VK_SUCCESS && acq != VK_SUBOPTIMAL_KHR)
        {
            m->frameOk = false;
            return;
        }
    }

    vkResetFences(m->device, 1, &m->fenceInFlight[m->frameSlot]);
    VkCommandBuffer c = m->cmd[m->frameSlot];
    vkResetCommandBuffer(c, 0);
    VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    vkBeginCommandBuffer(c, &bi);

    VkClearValue clears[2];
    ArgbToVkClear(argbClearColor, clears[0].color);
    clears[1].depthStencil = {
        0.0f,
        0}; // reversed-Z friendly clear (far = 0); renderer picks compare op

    VkRenderPassBeginInfo rpb{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    rpb.renderPass = m->renderPass;
    rpb.framebuffer = m->framebuffers[m->imageIndex];
    rpb.renderArea.extent = m->scExtent;
    rpb.clearValueCount = 2;
    rpb.pClearValues = clears;
    vkCmdBeginRenderPass(c, &rpb, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport vp{
        0, 0, (float)m->scExtent.width, (float)m->scExtent.height, 0.0f, 1.0f};
    VkRect2D sc{{0, 0}, m->scExtent};
    vkCmdSetViewport(c, 0, 1, &vp);
    vkCmdSetScissor(c, 0, 1, &sc);

    m->recording = true;
    m->frameOk = true;
}

void VulkanBackend::BindBackBuffer(bool /*bClearDepth*/)
{
    if (!m->recording)
        BeginFrame(
            m->clearArgb); // one RT: same as BeginFrame (parity with D3D12Backend phase 1)
}

void VulkanBackend::ClearDepth()
{
    // Clear the depth of whatever pass is actually open, in priority RTT -> scene -> flat. The exit dialog
    // (otwloop.cpp) clears depth so its 3D BSP draws ON TOP of the cockpit -- and the cockpit is in the SCENE pass,
    // so clearing the flat buffer's depth (as this used to, unconditionally) left the dialog buried. Same shape as
    // the SetViewportRect / ClearCurrentRTV fixes: the scene is usually the recording target, not the flat frame.
    VkCommandBuffer c = VK_NULL_HANDLE;
    VkExtent2D ext{};
    if (m->rttActive)
    {
        c = m->rttCmd;
        ext = {(uint32_t)m->rttW, (uint32_t)m->rttH};
    }
    else if (m->sceneRecording)
    {
        c = m->sceneCmd;
        // #107 VR-Vulkan: clear only THIS eye's render sub-rect of the (reused, max-sized) target, not the whole thing.
        const int rW = (m->sceneRenderW > 0 && m->sceneRenderW <= m->sceneW) ?
                           m->sceneRenderW :
                           m->sceneW;
        const int rH = (m->sceneRenderH > 0 && m->sceneRenderH <= m->sceneH) ?
                           m->sceneRenderH :
                           m->sceneH;
        ext = {(uint32_t)rW, (uint32_t)rH};
    }
    else if (m->recording && m->frameOk)
    {
        c = m->cmd[m->frameSlot];
        ext = m->scExtent;
    }
    if (!c || !ext.width || !ext.height)
        return;
    VkClearAttachment ca{};
    ca.aspectMask = DepthAspect(m);
    ca.clearValue.depthStencil = {0.0f, 0};
    VkClearRect cr{{{0, 0}, ext}, 0, 1};
    vkCmdClearAttachments(c, 1, &ca, 1, &cr);
}

// Artscout - 2026 (#104): narrow the viewport to a sub-rect -- VirtualDisplay::ConfineObjectViewportToZone uses it to
// keep a sensor display's 3D objects inside its own atlas zone. Two things were wrong here, both the same shape as the
// ClearCurrentRTV/ClearDepth bug: it recorded into the flat swapchain buffer whatever pass was actually open (so the
// narrowing missed the RTT entirely AND left the swapchain viewport clamped to an MFD zone), and it set no scissor, so
// nothing was actually clipped -- D3D12Backend::SetViewportRect sets both.
void VulkanBackend::SetViewportRect(int x, int y, int w, int h)
{
    if (w <= 0 || h <= 0)
        return;
    VkCommandBuffer c = VK_NULL_HANDLE;
    bool sceneTarget = false;
    if (m->rttActive)
    {
        c = m->rttCmd;
        // Remember the zone so the object path can re-assert it after any mid-batch re-bind reset the command
        // buffer's viewport to the full atlas (see rttZone in Impl). The only caller while an RTT is bound is
        // ConfineObjectViewportToZone.
        m->rttZone[0] = x;
        m->rttZone[1] = y;
        m->rttZone[2] = w;
        m->rttZone[3] = h;
        m->rttZoneHandle = m->rttBoundHandle;
        ++m->rttVpSerial;
    }
    else if (m->sceneRecording)
    {
        c = m->sceneCmd;
        sceneTarget = true;
    }
    else if (m->recording && m->frameOk)
        c = m->cmd[m->frameSlot];
    if (!c)
        return;
    // The scene pass runs a NEGATIVE-height viewport (its matrices are D3D-convention); match that sign or a sub-rect
    // set during the scene would silently flip the 3D inside it. The RTT/swapchain passes are positive.
    VkViewport vp =
        sceneTarget ?
            VkViewport{(float)x,  (float)(y + h), (float)w,
                       -(float)h, 0.0f,           1.0f} :
            VkViewport{(float)x, (float)y, (float)w, (float)h, 0.0f, 1.0f};
    VkRect2D sc{{x, y}, {(uint32_t)w, (uint32_t)h}};
    vkCmdSetViewport(c, 0, 1, &vp);
    vkCmdSetScissor(c, 0, 1, &sc);
}

void VulkanBackend::ResolveMsaaToBackBuffer()
{ /* no MSAA yet (parity with D3D12Backend) */
}

void VulkanBackend::Present(bool bVSync)
{
    {
        static int _n = 0;
        if (_n++ % 60 == 0)
            fprintf(stderr,
                    "[Vulkan] Present() call #%d recording=%d frameOk=%d\n", _n,
                    (int)m->recording, (int)m->frameOk);
    }
    if (!IsValid())
        return;
    if (m->vsync != bVSync)
    {
        m->vsync = bVSync;
    } // applied on next swapchain (re)create
    if (!m->recording || !m->frameOk)
    {
        m->recording = false;
        return;
    }

    VkCommandBuffer c = m->cmd[m->frameSlot];
    vkCmdEndRenderPass(c);
    vkEndCommandBuffer(c);

    if (m->headless)
    {
        // no swapchain semaphores / present: just submit + fence (ReadbackColor waits on the queue)
        VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        si.commandBufferCount = 1;
        si.pCommandBuffers = &c;
        {
            std::lock_guard<std::mutex> _qlk(g_vkQueueMutex);
            vkQueueSubmit(m->queue, 1, &si, m->fenceInFlight[m->frameSlot]);
        }
        m->recording = false;
        m->frameOk = false;
        return;
    }

    VkPipelineStageFlags waitStage =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    si.waitSemaphoreCount = 1;
    si.pWaitSemaphores = &m->semImageAvail[m->frameSlot];
    si.pWaitDstStageMask = &waitStage;
    si.commandBufferCount = 1;
    si.pCommandBuffers = &c;
    si.signalSemaphoreCount = 1;
    si.pSignalSemaphores =
        &m->semRenderDone
             [m->imageIndex]; // per-image (not per-slot): see semRenderDone decl

    VkPresentInfoKHR pi{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    pi.waitSemaphoreCount = 1;
    pi.pWaitSemaphores = &m->semRenderDone[m->imageIndex];
    pi.swapchainCount = 1;
    pi.pSwapchains = &m->swapchain;
    pi.pImageIndices = &m->imageIndex;
    VkResult pr;
    {
        std::lock_guard<std::mutex> _qlk(
            g_vkQueueMutex); // submit+present as one critical section on the shared queue
        vkQueueSubmit(m->queue, 1, &si, m->fenceInFlight[m->frameSlot]);
        pr = vkQueuePresentKHR(m->queue, &pi);
    }
    if (pr == VK_ERROR_OUT_OF_DATE_KHR || pr == VK_SUBOPTIMAL_KHR)
        m->resizeRequested = true; // deferred recreate next frame
    // no vkQueueWaitIdle: the full drain collapsed the swapchain to one reused image and wedged FIFO present (see PresentScene)

    m->recording = false;
    m->frameOk = false;
}

// ---------------------------------------------------------------------------------------------- 565 menu blit
// A full standalone frame: acquire -> upload the RGB565 bitmap into a staging image -> blit (scaled) onto the
// swapchain image -> present. Independent of BeginFrame/Present (the menu path calls this directly).
void VulkanBackend::BlitBitmap565(const void* pSrc565, int srcW, int srcH)
{
    if (!IsValid() || !pSrc565 || srcW <= 0 || srcH <= 0)
        return;
    if (!EnsureSwapchainReady(this, m))
        return; // swapchain lost + window not ready -> skip frame

    uint32_t slot = (m->frameSlot + 1) % kMaxFramesInFlight;
    m->frameSlot = slot;
    vkWaitForFences(m->device, 1, &m->fenceInFlight[slot], VK_TRUE, UINT64_MAX);

    uint32_t imgIdx = 0;
    VkResult acq =
        vkAcquireNextImageKHR(m->device, m->swapchain, UINT64_MAX,
                              m->semImageAvail[slot], VK_NULL_HANDLE, &imgIdx);
    if (acq == VK_ERROR_OUT_OF_DATE_KHR)
    {
        m->resizeRequested = true;
        return;
    } // deferred recreate next frame
    if (acq != VK_SUCCESS && acq != VK_SUBOPTIMAL_KHR)
        return;

    // Artscout - 2026: an acquired image MUST go back, or the swapchain runs dry
    // and the next acquire blocks forever. Every early exit below presents it
    // untouched instead of returning; the fence is reset only at the submit.
    auto giveBackImage = [&]()
    {
        VkPresentInfoKHR pib{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
        pib.waitSemaphoreCount = 1;
        pib.pWaitSemaphores = &m->semImageAvail[slot];
        pib.swapchainCount = 1;
        pib.pSwapchains = &m->swapchain;
        pib.pImageIndices = &imgIdx;
        std::lock_guard<std::mutex> _qlk(g_vkQueueMutex);
        vkQueuePresentKHR(m->queue, &pib);
    };

    // staging image in R5G6B5 matching the source bytes
    VkImage stage = VK_NULL_HANDLE;
    VkDeviceMemory stageMem = VK_NULL_HANDLE;
    VkImageCreateInfo ci{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    ci.imageType = VK_IMAGE_TYPE_2D;
    ci.format = VK_FORMAT_R5G6B5_UNORM_PACK16;
    ci.extent = {(uint32_t)srcW, (uint32_t)srcH, 1};
    ci.mipLevels = 1;
    ci.arrayLayers = 1;
    ci.samples = VK_SAMPLE_COUNT_1_BIT;
    ci.tiling = VK_IMAGE_TILING_LINEAR;
    ci.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    ci.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
    if (vkCreateImage(m->device, &ci, nullptr, &stage) != VK_SUCCESS)
    {
        giveBackImage();
        return;
    }
    void* stageMap = nullptr;
    if (not FF_VmaAllocImageMemoryHost(stage, (void**)&stageMem, &stageMap) or
        not stageMap)
    {
        vkDestroyImage(m->device, stage, nullptr);
        stage = VK_NULL_HANDLE;
    }

    // copy rows honoring the image's row pitch
    if (not stage)
    {
        giveBackImage();
        return;
    }
    VkImageSubresource sub{VK_IMAGE_ASPECT_COLOR_BIT, 0, 0};
    VkSubresourceLayout sl;
    vkGetImageSubresourceLayout(m->device, stage, &sub, &sl);
    // stageMap: persistent VMA map
    const uint8_t* src = (const uint8_t*)pSrc565;
    for (int y = 0; y < srcH; ++y)
        memcpy((uint8_t*)stageMap + sl.offset + (VkDeviceSize)y * sl.rowPitch,
               src + (size_t)y * srcW * 2, (size_t)srcW * 2);

    VkCommandBuffer c = m->cmd[slot];
    vkResetCommandBuffer(c, 0);
    VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(c, &bi);

    auto barrier = [&](VkImage img, VkImageLayout oldL, VkImageLayout newL,
                       VkAccessFlags srcA, VkAccessFlags dstA,
                       VkPipelineStageFlags srcS, VkPipelineStageFlags dstS)
    {
        VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        b.oldLayout = oldL;
        b.newLayout = newL;
        b.srcQueueFamilyIndex = b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        b.image = img;
        b.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        b.srcAccessMask = srcA;
        b.dstAccessMask = dstA;
        vkCmdPipelineBarrier(c, srcS, dstS, 0, 0, nullptr, 0, nullptr, 1, &b);
    };

    barrier(stage, VK_IMAGE_LAYOUT_PREINITIALIZED,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_HOST_WRITE_BIT,
            VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_HOST_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT);
    // Artscout - 2026 (#104): srcStage = TRANSFER, NOT TOP_OF_PIPE. This barrier WRITES the swapchain image (a layout
    // transition), and the only thing ordering it after vkAcquireNextImageKHR is the semImageAvail wait -- which is
    // declared at TRANSFER (pWaitDstStageMask below). A semaphore wait blocks its wait stage and later ones, so a
    // barrier whose source scope is TOP_OF_PIPE sits BEFORE the wait and may run while the acquire is still reading:
    // sync validation reports it as SYNC-HAZARD-WRITE-AFTER-READ. Matching the wait stage puts it inside the gate.
    barrier(m->scImages[imgIdx], VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0,
            VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT);

    VkImageBlit blit{};
    blit.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    blit.srcOffsets[1] = {srcW, srcH, 1};
    blit.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    blit.dstOffsets[1] = {(int)m->scExtent.width, (int)m->scExtent.height, 1};
    vkCmdBlitImage(c, stage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   m->scImages[imgIdx], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                   &blit, VK_FILTER_LINEAR);

    barrier(m->scImages[imgIdx], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_ACCESS_TRANSFER_WRITE_BIT, 0,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
    vkEndCommandBuffer(c);

    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    si.waitSemaphoreCount = 1;
    si.pWaitSemaphores = &m->semImageAvail[slot];
    si.pWaitDstStageMask = &waitStage;
    si.commandBufferCount = 1;
    si.pCommandBuffers = &c;
    si.signalSemaphoreCount = 1;
    si.pSignalSemaphores = &m->semRenderDone[imgIdx]; // per-image

    VkPresentInfoKHR pi{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    pi.waitSemaphoreCount = 1;
    pi.pWaitSemaphores = &m->semRenderDone[imgIdx];
    pi.swapchainCount = 1;
    pi.pSwapchains = &m->swapchain;
    pi.pImageIndices = &imgIdx;
    {
        std::lock_guard<std::mutex> _qlk(g_vkQueueMutex);
        // Reset here, not right after the acquire: an early exit above must
        // leave the fence signalled, or the next frame waits on it forever.
        vkResetFences(m->device, 1, &m->fenceInFlight[slot]);
        vkQueueSubmit(m->queue, 1, &si, m->fenceInFlight[slot]);
        vkQueuePresentKHR(m->queue, &pi);
    }

    // the staging image is transient -- wait this frame out, then free it (bring-up simplicity; a pool comes later)
    vkWaitForFences(m->device, 1, &m->fenceInFlight[slot], VK_TRUE, UINT64_MAX);
    vkDestroyImage(m->device, stage, nullptr);
    FF_VmaFree((void*)stageMem);
}

// Artscout - 2026 (#107 VR-Vulkan menu): blit a 565 UI surface into each per-view XR swapchain image (full-eye
// background) for the menu / splash headset frame. Peer of BlitBitmap565 (565 -> host-visible staging) + the
// per-view barriers of BlitSceneToXrImages (each dst left in COLOR_ATTACHMENT_OPTIMAL, own fence, synchronous).
bool VulkanBackend::Blit565ToXrImages(int nViews, void* const* dstImages,
                                      const int* dstW, const int* dstH,
                                      const void* src565, int srcW, int srcH)
{
    if (!IsValid() || !dstImages || !src565 || srcW <= 0 || srcH <= 0 ||
        nViews < 1)
        return false;
    if (nViews > 4)
        nViews = 4;

    // 565 staging (host-visible, LINEAR), exactly as BlitBitmap565.
    VkImage stage = VK_NULL_HANDLE;
    VkDeviceMemory stageMem = VK_NULL_HANDLE;
    VkImageCreateInfo ci{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    ci.imageType = VK_IMAGE_TYPE_2D;
    ci.format = VK_FORMAT_R5G6B5_UNORM_PACK16;
    ci.extent = {(uint32_t)srcW, (uint32_t)srcH, 1};
    ci.mipLevels = 1;
    ci.arrayLayers = 1;
    ci.samples = VK_SAMPLE_COUNT_1_BIT;
    ci.tiling = VK_IMAGE_TILING_LINEAR;
    ci.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    ci.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
    if (vkCreateImage(m->device, &ci, nullptr, &stage) != VK_SUCCESS)
        return false;
    void* stageMap = nullptr;
    if (not FF_VmaAllocImageMemoryHost(stage, (void**)&stageMem, &stageMap) or
        not stageMap)
    {
        vkDestroyImage(m->device, stage, nullptr);
        stage = VK_NULL_HANDLE;
    }
    if (not stage)
        return false;
    VkImageSubresource ssub{VK_IMAGE_ASPECT_COLOR_BIT, 0, 0};
    VkSubresourceLayout sl;
    vkGetImageSubresourceLayout(m->device, stage, &ssub, &sl);
    // stageMap: persistent VMA map
    const uint8_t* src = (const uint8_t*)src565;
    for (int y = 0; y < srcH; ++y)
        memcpy((uint8_t*)stageMap + sl.offset + (VkDeviceSize)y * sl.rowPitch,
               src + (size_t)y * srcW * 2, (size_t)srcW * 2);

    if (m->vrBlitCmd == VK_NULL_HANDLE)
    {
        VkCommandBufferAllocateInfo cai{
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        cai.commandPool = m->vrBlitPool;
        cai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cai.commandBufferCount = 1;
        if (vkAllocateCommandBuffers(m->device, &cai, &m->vrBlitCmd) !=
            VK_SUCCESS)
        {
            vkDestroyImage(m->device, stage, nullptr);
            FF_VmaFree((void*)stageMem);
            return false;
        }
        VkFenceCreateInfo fi{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        vkCreateFence(m->device, &fi, nullptr, &m->vrBlitFence);
    }
    vkResetFences(m->device, 1, &m->vrBlitFence);
    VkCommandBuffer c = m->vrBlitCmd;
    vkResetCommandBuffer(c, 0);
    VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(c, &bi);
    auto barrier = [&](VkImage img, VkImageLayout oldL, VkImageLayout newL,
                       VkAccessFlags sa, VkAccessFlags da,
                       VkPipelineStageFlags ss, VkPipelineStageFlags ds)
    {
        VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        b.oldLayout = oldL;
        b.newLayout = newL;
        b.srcQueueFamilyIndex = b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        b.image = img;
        b.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        b.srcAccessMask = sa;
        b.dstAccessMask = da;
        vkCmdPipelineBarrier(c, ss, ds, 0, 0, nullptr, 0, nullptr, 1, &b);
    };
    barrier(stage, VK_IMAGE_LAYOUT_PREINITIALIZED,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_HOST_WRITE_BIT,
            VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_HOST_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT);
    for (int e = 0; e < nViews; ++e)
    {
        VkImage dst = (VkImage)dstImages[e];
        if (!dst)
            continue;
        const int dw = dstW ? dstW[e] : srcW, dh = dstH ? dstH[e] : srcH;
        barrier(dst, VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0,
                VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT);
        VkImageBlit blit{};
        blit.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        blit.srcOffsets[1] = {srcW, srcH, 1};
        blit.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        blit.dstOffsets[1] = {dw, dh, 1};
        vkCmdBlitImage(c, stage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst,
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit,
                       VK_FILTER_LINEAR);
        barrier(dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_ACCESS_TRANSFER_WRITE_BIT,
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    }
    vkEndCommandBuffer(c);
    VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    si.commandBufferCount = 1;
    si.pCommandBuffers = &c;
    {
        std::lock_guard<std::mutex> _qlk(g_vkQueueMutex);
        vkQueueSubmit(m->queue, 1, &si, m->vrBlitFence);
    }
    vkWaitForFences(m->device, 1, &m->vrBlitFence, VK_TRUE, 1000000000ull);
    vkDestroyImage(m->device, stage, nullptr);
    FF_VmaFree((void*)stageMem);
    return true;
}

// Artscout - 2026 (#107 VR-Vulkan menu quad): upload an RGBA8 buffer into an XR menu-swapchain VkImage (peer of the
// D3D12 XrCopyRgbaToUiImageD3D12). Staging buffer -> vkCmdCopyBufferToImage; dst left in COLOR_ATTACHMENT_OPTIMAL
// (the layout the runtime expects on release). Own fence, synchronous.
bool VulkanBackend::UploadRgbaToXrImage(void* dstImage, int w, int h,
                                        const void* rgba, int rowPitchBytes)
{
    if (!IsValid() || !dstImage || !rgba || w <= 0 || h <= 0)
        return false;
    VkImage dst = (VkImage)dstImage;
    const VkDeviceSize sz = (VkDeviceSize)w * h * 4;

    VkBuffer buf = VK_NULL_HANDLE;
    VkDeviceMemory bufMem = VK_NULL_HANDLE;
    VkBufferCreateInfo bci{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bci.size = sz;
    bci.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(m->device, &bci, nullptr, &buf) != VK_SUCCESS)
        return false;
    void* mapped = nullptr;
    if (not FF_VmaAllocBufferMemory(buf, true, (void**)&bufMem, &mapped) or
        not mapped)
    {
        vkDestroyBuffer(m->device, buf, nullptr);
        return false;
    }
    const uint8_t* s = (const uint8_t*)rgba;
    for (int y = 0; y < h; ++y)
        memcpy((uint8_t*)mapped + (size_t)y * w * 4,
               s + (size_t)y * rowPitchBytes, (size_t)w * 4);

    if (m->vrBlitCmd == VK_NULL_HANDLE)
    {
        VkCommandBufferAllocateInfo cai{
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        cai.commandPool = m->vrBlitPool;
        cai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cai.commandBufferCount = 1;
        if (vkAllocateCommandBuffers(m->device, &cai, &m->vrBlitCmd) !=
            VK_SUCCESS)
        {
            vkDestroyBuffer(m->device, buf, nullptr);
            FF_VmaFree((void*)bufMem);
            return false;
        }
        VkFenceCreateInfo fi{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        vkCreateFence(m->device, &fi, nullptr, &m->vrBlitFence);
    }
        vkWaitForFences(m->device, 1, &m->vrBlitFence, VK_TRUE,
                    1000000000ull); // CopyRtt submits async -- cmd may still be in flight
    vkResetFences(m->device, 1, &m->vrBlitFence);
    VkCommandBuffer c = m->vrBlitCmd;
    vkResetCommandBuffer(c, 0);
    VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(c, &bi);
    auto barrier = [&](VkImageLayout oldL, VkImageLayout newL, VkAccessFlags sa,
                       VkAccessFlags da, VkPipelineStageFlags ss,
                       VkPipelineStageFlags ds)
    {
        VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        b.oldLayout = oldL;
        b.newLayout = newL;
        b.srcQueueFamilyIndex = b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        b.image = dst;
        b.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        b.srcAccessMask = sa;
        b.dstAccessMask = da;
        vkCmdPipelineBarrier(c, ss, ds, 0, 0, nullptr, 0, nullptr, 1, &b);
    };
    barrier(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0,
            VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT);
    VkBufferImageCopy region{};
    region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    region.imageExtent = {(uint32_t)w, (uint32_t)h, 1};
    vkCmdCopyBufferToImage(c, buf, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                           &region);
    barrier(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    vkEndCommandBuffer(c);
    VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    si.commandBufferCount = 1;
    si.pCommandBuffers = &c;
    {
        std::lock_guard<std::mutex> _qlk(g_vkQueueMutex);
        vkQueueSubmit(m->queue, 1, &si, m->vrBlitFence);
    }
    vkWaitForFences(m->device, 1, &m->vrBlitFence, VK_TRUE, 1000000000ull);
    vkDestroyBuffer(m->device, buf, nullptr);
    FF_VmaFree((void*)bufMem);
    return true;
}

// Artscout - 2026 (#107): the 2D pixel->NDC divisor (gScreenSize) lives in VulkanRenderer's viewport cbuffer, so
// forward it. VR_GSCREEN was a no-op on Vulkan, which is why AWACS/chat/menu 2D drew against the eye's full-target
// size and slid into a corner. D3D12's SetGScreenSize wires this the same way.
void VulkanBackend::SetGScreenSize(int w, int h)
{
    if (g_pVulkanRenderer)
        g_pVulkanRenderer->SetViewportSize(w, h);
}

// Artscout - 2026 (#104): clear the attachment of the pass that is CURRENTLY open on the flat side -- which is the
// RTT pass whenever one is bound, NOT the swapchain pass. Both of these wrote straight into m->cmd[frameSlot] with a
// swapchain-sized rect, and that is wrong twice over once an RTT is active: BindSceneRtt ENDED the swapchain pass on
// that buffer and opened its own on rttCmd, so the clear was recorded outside any render pass (which makes the whole
// command buffer invalid), and the rect was the window's, far outside a 512-pixel atlas. Harmless only while nothing
// called them under Vulkan; enabling the GM radar's clear surfaced it as the cockpit and models disappearing.
void VulkanBackend::ClearCurrentRTV(float r, float g, float b, float a)
{
    // Artscout - 2026 (GM radar): an open RTT pass is self-contained (own cmd + submit) and does not need the
    // flat frame to be recording -- in a VR frame there IS no flat frame, yet the GM sweep clear must still land.
    if (!m->rttActive && (!m->recording || !m->frameOk))
        return;
    VkCommandBuffer c = m->rttActive ? m->rttCmd : m->cmd[m->frameSlot];
    if (!c)
        return;
    VkExtent2D ext = m->rttActive ?
                         VkExtent2D{(uint32_t)m->rttW, (uint32_t)m->rttH} :
                         m->scExtent;
    if (!ext.width || !ext.height)
        return;
    VkClearAttachment ca{};
    ca.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    ca.colorAttachment = 0;
    ca.clearValue.color = {{r, g, b, a}};
    VkClearRect cr{{{0, 0}, ext}, 0, 1};
    vkCmdClearAttachments(c, 1, &ca, 1, &cr);
}

void VulkanBackend::FlushContext()
{ /* explicit submit model -> no-op (parity with D3D12) */
}

int VulkanBackend::Width() const
{
    return m ? m->width : 0;
}
int VulkanBackend::Height() const
{
    return m ? m->height : 0;
}
HWND VulkanBackend::Hwnd() const
{
    return m ? m->hwndToken : nullptr;
}

// Artscout - 2026 (#107 VR-Vulkan): split a runtime-provided extension string (space- and/or NUL-separated -- the
// OpenXR spec allows either) into individual names.
static void VkbSplitExtString(const char* s, std::vector<std::string>& out)
{
    if (!s)
        return;
    std::string cur;
    for (const char* p = s;; ++p)
    {
        char c = *p;
        if (c == ' ' || c == '\0')
        {
            if (!cur.empty())
            {
                out.push_back(cur);
                cur.clear();
            }
            if (c == '\0')
                break;
        }
        else
            cur.push_back(c);
    }
}

void VulkanBackend::SetExtraVulkanExtensions(const char* instExts,
                                             const char* devExts)
{
    if (!m)
        return;
    VkbSplitExtString(instExts, m->vrInstExts);
    VkbSplitExtString(devExts, m->vrDevExts);
    VKB_LOG(
        "SetExtraVulkanExtensions: +%zu instance, +%zu device (VR interop)\n",
        m->vrInstExts.size(), m->vrDevExts.size());
}

// #109 enable2: install the VR bring-up hooks. When set, Init routes vkCreateInstance/pick-physical/vkCreateDevice
// through the OpenXR runtime (which adds its interop extensions + names the HMD's GPU). Supersedes the manual
// SetExtraVulkanExtensions injection (the runtime now does that itself). NULL hooks -> the plain flat path.
void VulkanBackend::SetVulkanCreationHooks(VkbXrCreateInstanceHook ci,
                                           VkbXrPickPhysicalHook pp,
                                           VkbXrCreateDeviceHook cd, void* user)
{
    if (!m)
        return;
    m->hookCreateInstance = ci;
    m->hookPickPhysical = pp;
    m->hookCreateDevice = cd;
    m->hookUser = user;
    VKB_LOG("SetVulkanCreationHooks: VR bring-up routes instance/device "
            "creation through the OpenXR runtime (enable2)\n");
}

// ---------------------------------------------------------------------------------------------- device sharing
void* VulkanBackend::VkInstanceHandle() const
{
    return m ? (void*)m->instance : nullptr;
} // #107 VR: XrGraphicsBindingVulkan2KHR
void* VulkanBackend::VkDeviceHandle() const
{
    return m ? (void*)m->device : nullptr;
}
void* VulkanBackend::VkPhysicalDeviceHandle() const
{
    return m ? (void*)m->phys : nullptr;
}
void* VulkanBackend::VkQueueHandle() const
{
    return m ? (void*)m->queue : nullptr;
}
void* VulkanBackend::VkCommandPoolHandle() const
{
    return m ? (void*)m->cmdPool : nullptr;
}
unsigned VulkanBackend::VkGraphicsFamily() const
{
    return m ? m->gfxFamily : 0;
}
unsigned VulkanBackend::VkXrQueueIndex() const
{
    return m ? m->xrQueueIndex : 0;
} // #107: queue index for the OpenXR binding
unsigned long long VulkanBackend::SwapchainRenderPass() const
{
    return m ? (unsigned long long)m->renderPass : 0ull;
}
void* VulkanBackend::FlatCommandBuffer() const
{
    if (!m)
        return nullptr;
    if (m->rttActive)
        return (void*)m
            ->rttCmd; // RTT active -> display draws go to the RTT command buffer
    return m->recording ? (void*)m->cmd[m->frameSlot] : nullptr;
}
unsigned VulkanBackend::SwapchainColorFormatVk() const
{
    return m ? (unsigned)m->scFormat : (unsigned)VK_FORMAT_B8G8R8A8_UNORM;
}
void* VulkanBackend::SceneCommandBuffer() const
{
    return m ? (void*)m->sceneCmd : nullptr;
}
bool VulkanBackend::IsFlatRecording() const
{
    return m && m->recording;
}
bool VulkanBackend::IsSceneRecording() const
{
    return m && m->sceneRecording;
}
// Header-free probe for sim-side code: is a display RTT currently open? (-1 = not the Vulkan backend.)
// mavdisp gates PostSceneCloudOcclusion on it (that overlay is hard-wired to the eye scene cmd).
extern "C" int FF_VkRttActive(void)
{
    // (globals referenced through the file-scope declarations -- a block-scope extern inside an
    // extern "C" body inherits C linkage and clashes with their C++ one)
    if (!g_bUseVulkan || !g_pVulkanBackend)
        return -1;
    return g_pVulkanBackend->IsRttActive() ? 1 : 0;
}

bool VulkanBackend::IsRttActive() const
{
    return m && m->rttActive;
}

// The sensor zone rect remembered by SetViewportRect, if it still belongs to the currently bound RTT. The renderer
// re-asserts it on the object path at record time (and the full atlas on the 2D path) -- see rttZone in Impl.
bool VulkanBackend::GetRttZone(int* x, int* y, int* w, int* h) const
{
    if (!m || !m->rttActive || !m->rttZoneHandle ||
        m->rttZoneHandle != m->rttBoundHandle)
        return false;
    *x = m->rttZone[0];
    *y = m->rttZone[1];
    *w = m->rttZone[2];
    *h = m->rttZone[3];
    return true;
}

void VulkanBackend::GetRttExtent(int* w, int* h) const
{
    *w = m ? m->rttW : 0;
    *h = m ? m->rttH : 0;
}

unsigned VulkanBackend::RttViewportSerial() const
{
    return m ? m->rttVpSerial : 0;
}
// #104: fall back to the window size before the scene target exists -- same contract as D3D12Backend. Callers use
// this to set gScreenSize, and a zero there would map every CPU-projected 2D vertex to infinity.
// #107 VR-Vulkan: when a per-eye render sub-rect is active (VR path renders into the top-left sceneRenderW x
// sceneRenderH corner of a fixed max-sized target), report THAT sub-rect, not the whole target. gScreenSize is the
// pixel->NDC divisor for CPU-projected 2D (VS_Screen: HUD text, subtitles, chat, exit menu); the pixel came from
// VR_SetRes(eyeW,eyeH) == the sub-rect, so a full-target divisor here pushes every 2D overlay into the top-left
// corner (chat "stuck in a corner"). Same guard as the blit source rect (see lines ~1068/1606/1800). Flat path
// resets sceneRenderW/H to 0, so it falls back to the full target unchanged.
int VulkanBackend::SceneW() const
{
    if (!m)
        return 0;
    const int r = (m->sceneRenderW > 0 && m->sceneRenderW <= m->sceneW) ?
                      m->sceneRenderW :
                      m->sceneW;
    return r > 0 ? r : m->width;
}
int VulkanBackend::SceneH() const
{
    if (!m)
        return 0;
    const int r = (m->sceneRenderH > 0 && m->sceneRenderH <= m->sceneH) ?
                      m->sceneRenderH :
                      m->sceneH;
    return r > 0 ? r : m->height;
}
unsigned VulkanBackend::FindMemoryTypeIndex(unsigned typeBits,
                                            unsigned propFlags) const
{
    return m ? m->FindMemoryType(typeBits, (VkMemoryPropertyFlags)propFlags) :
               0;
}

// ---------------------------------------------------------------------------------------------- multiview / quad-views
bool VulkanBackend::MultiviewSupported() const
{
    return m && m->multiviewOk;
}
bool VulkanBackend::BindlessSupported() const
{
    return m && m->bindlessOk;
}
bool VulkanBackend::MeshShaderSupported() const
{
    return m && m->meshShaderOk && m->cmdDrawMeshTasks;
}
void VulkanBackend::CmdDrawMeshTasks(void* cmd, unsigned groupsX) const
{
    if (!m || !m->cmdDrawMeshTasks || !cmd || !groupsX)
        return;
    m->cmdDrawMeshTasks((VkCommandBuffer)cmd, groupsX, 1, 1);
}
bool VulkanBackend::IsSoftwareRasterizer() const
{
    if (!m || m->phys == VK_NULL_HANDLE)
        return false;
    VkPhysicalDeviceProperties p;
    vkGetPhysicalDeviceProperties(m->phys, &p);
    return p.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU;
}
int VulkanBackend::SceneViews() const
{
    return m ? m->sceneViews : 0;
}
void* VulkanBackend::GetSceneArrayImage() const
{
    return m ? (void*)m->sceneColor : nullptr;
}
unsigned long long VulkanBackend::GetSceneRenderPass() const
{
    return m ? (unsigned long long)m->sceneRenderPass : 0ull;
}
// #104: the RTT pass, for pipelines that draw into a bound RTT. It is built swapchain-COMPATIBLE on purpose (same
// attachments and dependencies), so the screen pipeline variant keyed to the swapchain pass is bindable inside it.
unsigned long long VulkanBackend::RttRenderPass() const
{
    return m ? (unsigned long long)m->rttRenderPass : 0ull;
}

static void DestroyTailTarget(VulkanBackend::Impl* m); // #107 Option 2 fwd decl

static void DestroySceneTarget(VulkanBackend::Impl* m)
{
    if (m->device == VK_NULL_HANDLE)
        return;
    DestroyTailTarget(
        m); // #107 Option 2: tail views reference sceneColor/sceneDepth -> free them FIRST
    DestroyXrBlitSrcViews(m); // the XR blit's per-layer views alias it too
    if (m->sceneFbo)
    {
        vkDestroyFramebuffer(m->device, m->sceneFbo, nullptr);
        m->sceneFbo = VK_NULL_HANDLE;
    }
    if (m->sceneColorView)
    {
        vkDestroyImageView(m->device, m->sceneColorView, nullptr);
        m->sceneColorView = VK_NULL_HANDLE;
    }
    if (m->sceneColor)
    {
        vkDestroyImage(m->device, m->sceneColor, nullptr);
        m->sceneColor = VK_NULL_HANDLE;
    }
    if (m->sceneColorMem)
    {
        FF_VmaFree((void*)m->sceneColorMem);
        m->sceneColorMem = VK_NULL_HANDLE;
    }
    if (m->sceneDepthView)
    {
        vkDestroyImageView(m->device, m->sceneDepthView, nullptr);
        m->sceneDepthView = VK_NULL_HANDLE;
    }
    if (m->sceneDepth)
    {
        vkDestroyImage(m->device, m->sceneDepth, nullptr);
        m->sceneDepth = VK_NULL_HANDLE;
    }
    if (m->sceneDepthMem)
    {
        FF_VmaFree((void*)m->sceneDepthMem);
        m->sceneDepthMem = VK_NULL_HANDLE;
    }
    if (m->sceneRenderPass)
    {
        vkDestroyRenderPass(m->device, m->sceneRenderPass, nullptr);
        m->sceneRenderPass = VK_NULL_HANDLE;
    }
}

static bool MakeArrayImage(VulkanBackend::Impl* m, VkFormat fmt, int w, int h,
                           int layers, VkImageUsageFlags usage,
                           VkImageAspectFlags aspect, VkImage& img,
                           VkDeviceMemory& mem, VkImageView& view)
{
    VkImageCreateInfo ci{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    ci.imageType = VK_IMAGE_TYPE_2D;
    ci.format = fmt;
    ci.extent = {(uint32_t)w, (uint32_t)h, 1};
    ci.mipLevels = 1;
    ci.arrayLayers = (uint32_t)layers;
    ci.samples = VK_SAMPLE_COUNT_1_BIT;
    ci.tiling = VK_IMAGE_TILING_OPTIMAL;
    ci.usage = usage;
    ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    if (vkCreateImage(m->device, &ci, nullptr, &img) != VK_SUCCESS)
        return false;
    if (not FF_VmaAllocImageMemory(img, (void**)&mem))
        return false;
    VkImageViewCreateInfo iv{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    iv.image = img;
    iv.viewType =
        VK_IMAGE_VIEW_TYPE_2D_ARRAY; // array view: multiview render pass writes all layers at once
    iv.format = fmt;
    iv.subresourceRange = {aspect, 0, 1, 0, (uint32_t)layers};
    return vkCreateImageView(m->device, &iv, nullptr, &view) == VK_SUCCESS;
}

// #107 Option 2: destroy the per-eye tail pass + single-layer views/framebuffers (rebuilt with the scene target).
static void DestroyTailTarget(VulkanBackend::Impl* m)
{
    if (m->device == VK_NULL_HANDLE)
        return;
    for (int i = 0; i < 4; ++i)
    {
        if (m->tailFbo[i])
        {
            vkDestroyFramebuffer(m->device, m->tailFbo[i], nullptr);
            m->tailFbo[i] = VK_NULL_HANDLE;
        }
        if (m->tailColorView[i])
        {
            vkDestroyImageView(m->device, m->tailColorView[i], nullptr);
            m->tailColorView[i] = VK_NULL_HANDLE;
        }
        if (m->tailDepthView[i])
        {
            vkDestroyImageView(m->device, m->tailDepthView[i], nullptr);
            m->tailDepthView[i] = VK_NULL_HANDLE;
        }
    }
    if (m->tailRenderPass)
    {
        vkDestroyRenderPass(m->device, m->tailRenderPass, nullptr);
        m->tailRenderPass = VK_NULL_HANDLE;
    }
}

// #107 Option 2: build the tail pass (single-view; LOAD color from the SHADER_READ scene array so world+cockpit
// survive, CLEAR depth so displays draw on top) + one single-layer color/depth view + framebuffer per array layer.
static bool BuildTailTarget(VulkanBackend::Impl* m, int w, int h, int nViews,
                            VkImageAspectFlags depthAspect)
{
    DestroyTailTarget(m);
    if (!m->multiviewOk)
        return true; // no multiview device -> grouped multiview path never runs; tail unused

    VkAttachmentDescription color{};
    color.format = m->sceneFormat;
    color.samples = VK_SAMPLE_COUNT_1_BIT;
    color.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color.initialLayout =
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; // left there by the multiview scene pass
    color.finalLayout =
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; // so BlitSceneToXrImages reads it as before
    VkAttachmentDescription depth{};
    depth.format = m->depthFormat;
    depth.samples = VK_SAMPLE_COUNT_1_BIT;
    depth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    VkAttachmentReference colorRef{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkAttachmentReference depthRef{
        1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
    VkSubpassDescription sub{};
    sub.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    sub.colorAttachmentCount = 1;
    sub.pColorAttachments = &colorRef;
    sub.pDepthStencilAttachment = &depthRef;
    VkAttachmentDescription atts[2] = {color, depth};
    VkRenderPassCreateInfo rp{
        VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO}; // viewMask 0 -> single-view (NOT multiview)
    rp.attachmentCount = 2;
    rp.pAttachments = atts;
    rp.subpassCount = 1;
    rp.pSubpasses = &sub;
    if (vkCreateRenderPass(m->device, &rp, nullptr, &m->tailRenderPass) !=
        VK_SUCCESS)
        return false;

    for (int i = 0; i < nViews && i < 4; ++i)
    {
        VkImageViewCreateInfo cv{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        cv.image = m->sceneColor;
        cv.viewType = VK_IMAGE_VIEW_TYPE_2D;
        cv.format = m->sceneFormat;
        cv.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, (uint32_t)i,
                               1}; // ONE layer
        if (vkCreateImageView(m->device, &cv, nullptr, &m->tailColorView[i]) !=
            VK_SUCCESS)
            return false;
        VkImageViewCreateInfo dv{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        dv.image = m->sceneDepth;
        dv.viewType = VK_IMAGE_VIEW_TYPE_2D;
        dv.format = m->depthFormat;
        dv.subresourceRange = {depthAspect, 0, 1, (uint32_t)i, 1};
        if (vkCreateImageView(m->device, &dv, nullptr, &m->tailDepthView[i]) !=
            VK_SUCCESS)
            return false;
        VkImageView fbAtts[2] = {m->tailColorView[i], m->tailDepthView[i]};
        VkFramebufferCreateInfo fb{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        fb.renderPass = m->tailRenderPass;
        fb.attachmentCount = 2;
        fb.pAttachments = fbAtts;
        fb.width = (uint32_t)w;
        fb.height = (uint32_t)h;
        fb.layers = 1;
        if (vkCreateFramebuffer(m->device, &fb, nullptr, &m->tailFbo[i]) !=
            VK_SUCCESS)
            return false;
    }
    return true;
}

bool VulkanBackend::EnsureSceneTarget(int perViewW, int perViewH, int nViews)
{
    if (!IsValid() || perViewW <= 0 || perViewH <= 0)
        return false;
    if (nViews < 1)
        nViews = 1;
    if (nViews > 4)
        nViews = 4;
    // #107 PERF: keep a MAX-sized, reused scene target and render into its top-left [0,0,perViewW,perViewH] sub-rect
    // (SetSceneRenderSize -> BeginSceneMultiview/BeginTailView sub-rect viewport -> the sub-rect blit in
    // BlitSceneToXrImages, all already sub-rect-aware). Quad alternates periphery<->focus size every group; the old
    // exact-size test recreated the target (with a full vkDeviceWaitIdle) twice per frame. Reuse whenever the existing
    // target is BIG ENOUGH for this view (same nViews); grow only for a LARGER view, and never shrink.
    if (m->sceneColor && m->sceneViews == nViews && m->sceneW >= perViewW &&
        m->sceneH >= perViewH)
        return true; // big enough -> render the sub-rect, no recreate (no device drain)

    // Growing: allocate the MAX of requested and current so alternating sizes never ping-pong the recreate (after the
    // largest view is seen once, every smaller view reuses it). Only within the SAME nViews (mode) -- a nViews change
    // (flat<->VR) is a real mode switch and must take the exact requested size, not inherit the other mode's max.
    if (m->sceneColor && m->sceneViews == nViews)
    {
        if (perViewW < m->sceneW)
            perViewW = m->sceneW;
        if (perViewH < m->sceneH)
            perViewH = m->sceneH;
    }

    vkDeviceWaitIdle(m->device);
    // #107: the render pass depends only on format + view mask (nViews), NOT size. KEEP it across pure size changes so
    // the pipelines built against it survive -- in quad the per-eye periphery<->focus size alternation would otherwise
    // recreate the pass every eye, and PurgePipelinesIfScenePassChanged destroyed+rebuilt EVERY pipeline twice a frame
    // (and crashed inside the purge). Recreate the pass ONLY when nViews changes; images+framebuffer track the size.
    const bool keepPass =
        (m->sceneViews == nViews) && (m->sceneRenderPass != VK_NULL_HANDLE);
    DestroyTailTarget(
        m); // #107 Option 2: tail views alias sceneColor/sceneDepth -> free before recreating them
    DestroyXrBlitSrcViews(m); // the XR blit's per-layer views alias it too
    if (m->sceneFbo)
    {
        vkDestroyFramebuffer(m->device, m->sceneFbo, nullptr);
        m->sceneFbo = VK_NULL_HANDLE;
    }
    if (m->sceneColorView)
    {
        vkDestroyImageView(m->device, m->sceneColorView, nullptr);
        m->sceneColorView = VK_NULL_HANDLE;
    }
    if (m->sceneColor)
    {
        vkDestroyImage(m->device, m->sceneColor, nullptr);
        m->sceneColor = VK_NULL_HANDLE;
    }
    if (m->sceneColorMem)
    {
        FF_VmaFree((void*)m->sceneColorMem);
        m->sceneColorMem = VK_NULL_HANDLE;
    }
    if (m->sceneDepthView)
    {
        vkDestroyImageView(m->device, m->sceneDepthView, nullptr);
        m->sceneDepthView = VK_NULL_HANDLE;
    }
    if (m->sceneDepth)
    {
        vkDestroyImage(m->device, m->sceneDepth, nullptr);
        m->sceneDepth = VK_NULL_HANDLE;
    }
    if (m->sceneDepthMem)
    {
        FF_VmaFree((void*)m->sceneDepthMem);
        m->sceneDepthMem = VK_NULL_HANDLE;
    }
    if (!keepPass && m->sceneRenderPass)
    {
        vkDestroyRenderPass(m->device, m->sceneRenderPass, nullptr);
        m->sceneRenderPass = VK_NULL_HANDLE;
    }
    m->sceneW = perViewW;
    m->sceneH = perViewH;
    m->sceneViews = nViews;

    if (!MakeArrayImage(m, m->sceneFormat, perViewW, perViewH, nViews,
                        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                            VK_IMAGE_USAGE_SAMPLED_BIT |
                            VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                        VK_IMAGE_ASPECT_COLOR_BIT, m->sceneColor,
                        m->sceneColorMem, m->sceneColorView))
        return false;
    if (!MakeArrayImage(m, m->depthFormat, perViewW, perViewH, nViews,
                        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                        DepthAspect(m), m->sceneDepth, m->sceneDepthMem,
                        m->sceneDepthView))
        return false;

    // multiview render pass: one draw -> all N layers, shader selects per-view matrix by gl_ViewIndex. Built only when
    // it does not already exist for this nViews (kept across pure size changes -- see keepPass above).
    if (!keepPass)
    {
        VkAttachmentDescription color{};
        color.format = m->sceneFormat;
        color.samples = VK_SAMPLE_COUNT_1_BIT;
        color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        color.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        VkAttachmentDescription depth{};
        depth.format = m->depthFormat;
        depth.samples = VK_SAMPLE_COUNT_1_BIT;
        depth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depth.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depth.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE; // #76
        depth.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depth.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        VkAttachmentReference colorRef{
            0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        VkAttachmentReference depthRef{
            1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
        VkSubpassDescription sub{};
        sub.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        sub.colorAttachmentCount = 1;
        sub.pColorAttachments = &colorRef;
        sub.pDepthStencilAttachment = &depthRef;

        const uint32_t viewMask =
            (nViews >= 32) ? 0xFFFFFFFFu : ((1u << nViews) - 1u);
        VkRenderPassMultiviewCreateInfo mv{
            VK_STRUCTURE_TYPE_RENDER_PASS_MULTIVIEW_CREATE_INFO};
        mv.subpassCount = 1;
        mv.pViewMasks = &viewMask;
        mv.correlationMaskCount = 1;
        mv.pCorrelationMasks = &viewMask;

        VkAttachmentDescription atts[2] = {color, depth};
        VkRenderPassCreateInfo rp{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
        rp.pNext =
            m->multiviewOk ?
                &mv :
                nullptr; // without multiview support, a 1-view pass still works
        rp.attachmentCount = 2;
        rp.pAttachments = atts;
        rp.subpassCount = 1;
        rp.pSubpasses = &sub;
        if (vkCreateRenderPass(m->device, &rp, nullptr, &m->sceneRenderPass) !=
            VK_SUCCESS)
            return false;
    }

    VkImageView fbAtts[2] = {m->sceneColorView, m->sceneDepthView};
    VkFramebufferCreateInfo fb{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    fb.renderPass = m->sceneRenderPass;
    fb.attachmentCount = 2;
    fb.pAttachments = fbAtts;
    fb.width = (uint32_t)perViewW;
    fb.height = (uint32_t)perViewH;
    fb.layers =
        1; // multiview: layers driven by the view mask, framebuffer layerCount MUST be 1
    if (vkCreateFramebuffer(m->device, &fb, nullptr, &m->sceneFbo) !=
        VK_SUCCESS)
        return false;

    // #107 Option 2: (re)build the per-eye tail pass + single-layer framebuffers to match this scene target.
    if (!BuildTailTarget(m, perViewW, perViewH, nViews, DepthAspect(m)))
    {
        VKB_LOG("tail target build FAILED\n");
        DestroyTailTarget(m);
    }

    VKB_LOG("scene target: %dx%d x %d views (multiview=%d)\n", perViewW,
            perViewH, nViews, (int)m->multiviewOk);
    return true;
}

// #107 VR-Vulkan: set THIS eye's render sub-rect within the (reused, max-sized) scene target. BeginSceneMultiview
// renders into [0,0,w,h] and the eye blit copies only that region. 0,0 -> whole target (flat path).
void VulkanBackend::SetSceneRenderSize(int w, int h)
{
    if (m)
    {
        m->sceneRenderW = w;
        m->sceneRenderH = h;
    }
}

void VulkanBackend::BeginSceneMultiview(unsigned long argbClear)
{
    if (!IsValid() || !m->sceneRenderPass || m->sceneRecording)
        return;
    PROF_WAIT(VP_SCENE_WAIT, vkWaitForFences(m->device, 1, &m->sceneFence,
                                             VK_TRUE, UINT64_MAX));
    vkResetFences(m->device, 1, &m->sceneFence);

    VkCommandBuffer c = m->sceneCmd;
    vkResetCommandBuffer(c, 0);
    VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(c, &bi);

    // #107 PERF: GPU timing -- lazily create the 4-slot timestamp pool, then reset+stamp the scene begin (outside the
    // render pass, as vkCmdResetQueryPool requires). Reads happen in BlitSceneToXrImages after the guarding fences.
    if (g_bVulkanProfile)
    {
        if (m->tsPool == VK_NULL_HANDLE && m->tsPeriodNs == 0.0)
        {
            VkPhysicalDeviceProperties pp;
            vkGetPhysicalDeviceProperties(m->phys, &pp);
            if (pp.limits.timestampPeriod > 0.0f)
            {
                VkQueryPoolCreateInfo qi{
                    VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
                qi.queryType = VK_QUERY_TYPE_TIMESTAMP;
                qi.queryCount = 4;
                if (vkCreateQueryPool(m->device, &qi, nullptr, &m->tsPool) ==
                    VK_SUCCESS)
                    m->tsPeriodNs = (double)pp.limits.timestampPeriod;
            }
            if (m->tsPeriodNs == 0.0)
                m->tsPeriodNs = -1.0; // unsupported -> don't retry every frame
        }
        if (m->tsPool)
        {
            vkCmdResetQueryPool(c, m->tsPool, 0, 2);
            vkCmdWriteTimestamp(c, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, m->tsPool,
                                0);
        }
    }

    VkClearValue clears[2];
    ArgbToVkClear(argbClear, clears[0].color);
    clears[1].depthStencil = {0.0f, 0}; // reversed-Z clear (far = 0)

    // #107 VR-Vulkan: render into the top-left rW x rH sub-rect of the (possibly larger, reused) target -- rW/rH is
    // THIS eye's size when set, else the whole target (flat path). The blit later copies only [0,0,rW,rH].
    const int rW = (m->sceneRenderW > 0 && m->sceneRenderW <= m->sceneW) ?
                       m->sceneRenderW :
                       m->sceneW;
    const int rH = (m->sceneRenderH > 0 && m->sceneRenderH <= m->sceneH) ?
                       m->sceneRenderH :
                       m->sceneH;

    // Artscout - 2026 (#78): the terrain clipmap's copies are transfers, illegal
    // inside a render pass -- drain them here, while none is open. Hand over THIS
    // buffer: the renderer's own curCmd is a different one and not recording.
    if (g_pRenderer)
    {
        VulkanRenderer* vr = static_cast<VulkanRenderer*>(g_pRenderer);
        vr->FlushTerrainClipmapUploads((void*)c);
    }

    VkRenderPassBeginInfo rpb{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    rpb.renderPass = m->sceneRenderPass;
    rpb.framebuffer = m->sceneFbo;
    rpb.renderArea.extent = {(uint32_t)rW, (uint32_t)rH};
    rpb.clearValueCount = 2;
    rpb.pClearValues = clears;
    vkCmdBeginRenderPass(c, &rpb, VK_SUBPASS_CONTENTS_INLINE);

    // Artscout - 2026 (#104): NEGATIVE-height viewport (y=H, height=-H) flips clip-space Y so the D3D-convention
    // matrices (NDC Y up) render right-side-up in Vulkan (NDC Y down). Core since Vulkan 1.1 (multiview device);
    // the object pipeline is cull=NONE so the winding flip is harmless.
    VkViewport vp{0, (float)rH, (float)rW, -(float)rH, 0.0f, 1.0f};
    VkRect2D sc{{0, 0}, {(uint32_t)rW, (uint32_t)rH}};
    vkCmdSetViewport(c, 0, 1, &vp);
    vkCmdSetScissor(c, 0, 1, &sc);
    m->sceneRecording = true;
}

void VulkanBackend::EndSceneMultiview()
{
    if (!m->sceneRecording)
        return;
    VkCommandBuffer c = m->sceneCmd;
    vkCmdEndRenderPass(
        c); // render pass finalLayout leaves color in SHADER_READ_ONLY_OPTIMAL
    if (g_bVulkanProfile &&
        m->tsPool) // #107 PERF: stamp scene end (slot 1) after the render pass, before submit
        vkCmdWriteTimestamp(c, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, m->tsPool,
                            1);
    vkEndCommandBuffer(c);
    WaitRttConsumed(); // #107 PERF: FLAT path composites the RTT displays into sceneCmd -> ensure batched RTT submits
    // finish before scene runs. In the VR multiview path RTT happens later (tail), so this no-ops.
    VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    si.commandBufferCount = 1;
    si.pCommandBuffers = &c;
    {
        std::lock_guard<std::mutex> _qlk(g_vkQueueMutex);
        vkQueueSubmit(m->queue, 1, &si, m->sceneFence);
    }
    m->sceneRecording = false;
}

// #107 Option 2: open the per-eye TAIL pass on scene-array LAYER `layer`. LOAD color (world+cockpit from the
// multiview pass survive), CLEAR depth (displays draw on top). The renderer routes 2D/RTT draws here while active.
// Requires the scene multiview pass to have ENDED (color left in SHADER_READ) -- serial after EndSceneMultiview.
void VulkanBackend::BeginTailView(int layer)
{
    if (!IsValid() || !m->tailRenderPass || m->tailRecording)
        return;
    if (layer < 0 || layer >= 4 || m->tailFbo[layer] == VK_NULL_HANDLE)
        return;
    // The tail LOADs the color layer the multiview scene pass just wrote -> the world submit MUST have finished
    // (correct LOAD + no dynamic-VB reuse race). EndSceneMultiview signalled sceneFence; wait it before recording.
    PROF_WAIT(VP_TAIL_SCENE_WAIT, vkWaitForFences(m->device, 1, &m->sceneFence,
                                                  VK_TRUE, UINT64_MAX));
    // #107 PERF: take the next tail RING slot. Wait ONLY this slot's fence (its previous tail is done -> safe to reset);
    // deep enough (kTailRing=4 = quad's eyes) that within a frame this never blocks. No per-eye synchronous wait anymore.
    const int slot = m->tailRingIdx;
    PROF_WAIT(VP_TAIL_REUSE_WAIT,
              vkWaitForFences(m->device, 1, &m->tailRingFence[slot], VK_TRUE,
                              UINT64_MAX));
    vkResetFences(m->device, 1, &m->tailRingFence[slot]);
    m->tailCurSlot = slot;
    m->tailCmd =
        m->tailRing
            [slot]; // alias: TailCommandBuffer()/TailViewport2D/renderer use the current slot
    VkCommandBuffer c = m->tailCmd;
    vkResetCommandBuffer(c, 0);
    VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(c, &bi);

    VkClearValue clears[2];
    clears[0].color = {{0, 0, 0, 0}}; // color loadOp is LOAD -> this is ignored
    clears[1].depthStencil = {0.0f, 0}; // reversed-Z far

    const int rW = (m->sceneRenderW > 0 && m->sceneRenderW <= m->sceneW) ?
                       m->sceneRenderW :
                       m->sceneW;
    const int rH = (m->sceneRenderH > 0 && m->sceneRenderH <= m->sceneH) ?
                       m->sceneRenderH :
                       m->sceneH;
    VkRenderPassBeginInfo rpb{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    rpb.renderPass = m->tailRenderPass;
    rpb.framebuffer = m->tailFbo[layer];
    rpb.renderArea.extent = {(uint32_t)rW, (uint32_t)rH};
    rpb.clearValueCount = 2;
    rpb.pClearValues = clears;
    vkCmdBeginRenderPass(c, &rpb, VK_SUBPASS_CONTENTS_INLINE);
    // #107 Option 2: NEGATIVE-height viewport, IDENTICAL to BeginSceneMultiview -- the tail now draws the 3D cockpit BSP
    // (object path, D3D-convention matrices need the Y-flip) as well as the 2D RTT, exactly as the per-eye scene pass
    // does both correctly. The blit is a straight copy (no Y-flip), so tail orientation must match the world's.
    VkViewport vp{0, (float)rH, (float)rW, -(float)rH, 0.0f, 1.0f};
    VkRect2D sc{{0, 0}, {(uint32_t)rW, (uint32_t)rH}};
    vkCmdSetViewport(c, 0, 1, &vp);
    vkCmdSetScissor(c, 0, 1, &sc);
    m->tailRecording = true;
    m->tailActive = true;
}

// #107 Option 2: close the tail pass for the current layer and submit SYNCHRONOUSLY (wait the fence) so the layer is
// finished before the next view's tail or the BlitSceneToXrImages that reads it.
void VulkanBackend::EndTailView()
{
    if (!m->tailRecording)
        return;
    VkCommandBuffer c = m->tailCmd;
    vkCmdEndRenderPass(c);
    vkEndCommandBuffer(c);
    WaitRttConsumed(); // #107 PERF: the tail's DrawRttQuad composites sample the RTT display textures -> ensure the
    // batched RTT submits are finished before this tail (which reads them) runs on the GPU.
    const int slot = m->tailCurSlot;
    VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    si.commandBufferCount = 1;
    si.pCommandBuffers = &c;
    // #107 PERF: submit WITHOUT a wait -- record this slot as pending. BlitSceneToXrImages (which reads the scene layer
    // this tail drew into) calls WaitTailsConsumed() to wait every pending tail once. The next eye's tail records while
    // this one runs on the GPU (no per-eye CPU->GPU stall).
    {
        std::lock_guard<std::mutex> _qlk(g_vkQueueMutex);
        vkQueueSubmit(m->queue, 1, &si, m->tailRingFence[slot]);
    }
    if (m->tailPendingN < Impl::kTailRing)
        m->tailPending[m->tailPendingN++] = slot;
    m->tailRingIdx = (slot + 1) % Impl::kTailRing;
    m->tailCurSlot = -1;
    m->tailRecording = false;
    m->tailActive = false;
}

// #107 PERF: block until every tail submitted since the last consume is finished, so a reader (BlitSceneToXrImages) can
// safely read the scene layers the tails drew into. Waits each pending tail fence (they draw into DIFFERENT layers, so
// no single "last" fence covers all -- unlike the RTT ring). No-op when nothing is pending.
void VulkanBackend::WaitTailsConsumed()
{
    if (!m || m->tailPendingN <= 0)
        return;
    for (int i = 0; i < m->tailPendingN; ++i)
        if (m->tailRingFence[m->tailPending[i]])
            PROF_WAIT(VP_BLIT_TAILS_WAIT,
                      vkWaitForFences(m->device, 1,
                                      &m->tailRingFence[m->tailPending[i]],
                                      VK_TRUE, UINT64_MAX));
    m->tailPendingN = 0;
}

// #107 PERF: call once per VR frame. Every ~120 frames, dump the average per-phase CPU WAIT time (ms/frame) and the
// TOTAL wait. Read it against the frame budget (1000/fps ms): TOTAL_WAIT close to the frame time => the CPU spends the
// frame BLOCKED on the GPU (GPU-bound / no overlap -> frame-in-flight helps); TOTAL_WAIT small => the CPU is busy
// recording/submitting (CPU-bound -> bindless / fewer draws helps). Enable with FFViper.cfg "VulkanProfile 1".
// #107 PERF: backend-NEUTRAL free functions (declared in graphics/include/frameprof.h). The shared renderer (otw.cpp /
// TerrainGpu.cpp) and BOTH VR drivers call these directly, so the profiler runs identically on Vulkan AND D3D12 -- the
// two backends' frame cost can be compared with the same [VKPROF] line (same shared DrawScene/TerrainGpu code).
void FrameProf_EndFrame()
{
    static bool s_diagOnce = false;
    if (!s_diagOnce)
    {
        s_diagOnce = true;
        VKB_LOG("PROF: FrameProf_EndFrame reached, g_bVulkanProfile=%d\n",
                (int)g_bVulkanProfile);
    }
    if (!g_bVulkanProfile)
        return;
    if (g_vkProf.haveStart)
    {
        double frameMs = ProfMs(g_vkProf.frameStart, ProfClock::now());
        g_vkProf.accumTotalCpu += frameMs;
        g_vkProf.haveStart = false;
        if (frameMs > g_vkProf.worstTotal)
        { // keep this window's worst frame + its per-phase breakdown
            g_vkProf.worstTotal = frameMs;
            for (int i = 0; i < VP_COUNT; ++i)
                g_vkProf.worstPhase[i] =
                    g_vkProf.accum[i] - g_vkProf.frameBase[i];
        }
    }
    g_vkProf.frames++;
    if (g_vkProf.frames < 120)
        return;
    char b[640];
    int o = 0;
    o += snprintf(b + o, sizeof(b) - o,
                  "[VKPROF] avg/frame over %d frames (ms):", g_vkProf.frames);
    double totalWait = 0.0;
    for (int i = 0; i < VP_COUNT; ++i)
    {
        double avg = g_vkProf.accum[i] / (double)g_vkProf.frames;
        totalWait += avg;
        if (avg > 0.01)
            o += snprintf(b + o, sizeof(b) - o, " %s=%.2f", kVkProfNames[i],
                          avg);
    }
    snprintf(b + o, sizeof(b) - o,
             " | TOTAL_WAIT=%.2f CPU_FRAME=%.2f DRAWS=%lld TERRDRAWS=%lld",
             totalWait, g_vkProf.accumTotalCpu / (double)g_vkProf.frames,
             g_vkProf.drawCalls / (long long)g_vkProf.frames,
             g_vkProf.terrDraws / (long long)g_vkProf.frames);
    VKB_LOG("%s\n", b);
    o = 0;
    o += snprintf(b + o, sizeof(b) - o,
                  "[VKPROF-WORST] frame=%.2fms:", g_vkProf.worstTotal);
    for (int i = 0; i < VP_COUNT; ++i)
        if (g_vkProf.worstPhase[i] > 0.30)
            o += snprintf(b + o, sizeof(b) - o, " %s=%.2f", kVkProfNames[i],
                          g_vkProf.worstPhase[i]);
    VKB_LOG("%s\n", b);
    g_vkProf.worstTotal = 0.0;
    for (int i = 0; i < VP_COUNT; ++i)
        g_vkProf.accum[i] = 0.0;
    g_vkProf.accumTotalCpu = 0.0;
    g_vkProf.drawCalls = 0;
    g_vkProf.terrDraws = 0;
    g_vkProf.frames = 0;
}
void FrameProf_FrameStart()
{
    if (!g_bVulkanProfile)
        return;
    g_vkProf.frameStart = ProfClock::now();
    g_vkProf.haveStart = true;
    for (int i = 0; i < VP_COUNT; ++i)
        g_vkProf.frameBase[i] = g_vkProf.accum[i]; // worst-frame delta base
}
void FrameProf_DrawScene(double ms)
{
    if (g_bVulkanProfile)
        g_vkProf.accum[VP_DRAWSCENE] += ms;
}
void FrameProf_BlitGroup(double ms)
{
    if (g_bVulkanProfile)
        g_vkProf.accum[VP_BLITGROUP] += ms;
}
void FrameProf_TerrainGpu(double ms)
{
    if (g_bVulkanProfile)
        g_vkProf.accum[VP_TERRAIN_GPU] += ms;
}
void FrameProf_TerrainSpan(double ms)
{
    if (g_bVulkanProfile)
        g_vkProf.accum[VP_TERRAIN] += ms;
}
void FrameProf_TerrAcc(double ms)
{
    if (g_bVulkanProfile)
        g_vkProf.accum[VP_TERR_ACC] += ms;
}
void FrameProf_TerrFlush(double ms)
{
    if (g_bVulkanProfile)
        g_vkProf.accum[VP_TERR_FLUSH] += ms;
}
void FrameProf_CountTerrainDraw()
{
    if (g_bVulkanProfile)
        g_vkProf.terrDraws++;
}
void FrameProf_Objects(double ms)
{
    if (g_bVulkanProfile)
        g_vkProf.accum[VP_OBJECTS] += ms;
}
void FrameProf_CountDraw()
{
    if (g_bVulkanProfile)
        g_vkProf.drawCalls++;
}
void FrameProf_RenderVR(double ms)
{
    if (g_bVulkanProfile)
        g_vkProf.accum[VP_RENDERVR] += ms;
}
void FrameProf_TailEye(double ms)
{
    if (g_bVulkanProfile)
        g_vkProf.accum[VP_TAIL_CPU] += ms;
}
void FrameProf_VCock(double ms)
{
    if (g_bVulkanProfile)
        g_vkProf.accum[VP_VCOCK] += ms;
}
void FrameProf_PreVR(double ms)
{
    if (g_bVulkanProfile)
        g_vkProf.accum[VP_PREVR] += ms;
}
void FrameProf_PostVR(double ms)
{
    if (g_bVulkanProfile)
        g_vkProf.accum[VP_POSTVR] += ms;
}
void FrameProf_XrWaitFrame(double ms)
{
    if (g_bVulkanProfile)
        g_vkProf.accum[VP_XRWAITFRAME] += ms;
}

// #107 Option 2: mid-tail-pass, switch the GPU viewport to POSITIVE height for the 2D RTT composite. The 3D cockpit BSP
// (object path, D3D-convention matrices) needs the NEGATIVE-height Y-flip (BeginTailView's default, same as the scene
// pass); the 2D screen path (VS_Screen) comes out framebuffer-Y-down and needs positive. Call after the cockpit,
// before VCock_Exec. (In the per-eye scene pass both share negative-height; isolating them in the tail exposed this.)
void VulkanBackend::TailViewport2D()
{
    if (!m || !m->tailRecording)
        return;
    const int rW = (m->sceneRenderW > 0 && m->sceneRenderW <= m->sceneW) ?
                       m->sceneRenderW :
                       m->sceneW;
    const int rH = (m->sceneRenderH > 0 && m->sceneRenderH <= m->sceneH) ?
                       m->sceneRenderH :
                       m->sceneH;
    VkViewport vp{0, 0.0f, (float)rW, (float)rH, 0.0f, 1.0f}; // positive height
    vkCmdSetViewport(m->tailCmd, 0, 1, &vp);
}

bool VulkanBackend::IsTailActive() const
{
    return m && m->tailActive;
}
void* VulkanBackend::TailCommandBuffer() const
{
    return m ? (void*)m->tailCmd : nullptr;
}
unsigned long long VulkanBackend::TailRenderPass() const
{
    return m ? (unsigned long long)m->tailRenderPass : 0ull;
}

// ---------------------------------------------------------------------------------------------- flat 3D frame
void VulkanBackend::EnsureSceneStarted(unsigned long argbSkyClear)
{
    if (!IsValid() || m->sceneRecording)
        return;
    if (!EnsureSceneTarget(m->width, m->height, 1))
    {
        static bool w = false;
        if (!w)
        {
            w = true;
            VKB_LOG("EnsureSceneStarted: EnsureSceneTarget FAILED %dx%d\n",
                    m->width, m->height);
        }
        return;
    }
    static bool once = false;
    if (!once)
    {
        once = true;
        VKB_LOG("EnsureSceneStarted: scene %dx%d opened (first 3D frame)\n",
                m->width, m->height);
    }
    m->sceneRenderW = 0;
    m->sceneRenderH =
        0; // #107: flat path renders the WHOLE target (clears any leftover VR eye sub-rect)
    BeginSceneMultiview(argbSkyClear);
}

bool VulkanBackend::SceneStarted() const
{
    return m && m->sceneRecording;
}

// End the scene pass and blit layer 0 of the scene color array onto the swapchain, then present. This is the flat
// (desktop) counterpart of the VR path (which submits the array to OpenXR). No UI composite yet -- terrain/objects
// first; the 2D HUD overlay follows.
void VulkanBackend::PresentScene()
{
    {
        static int _n = 0;
        if (_n++ % 60 == 0)
            fprintf(stderr, "[Vulkan] PresentScene() call #%d headless=%d\n",
                    _n, (int)m->headless);
    }
    if (!IsValid() || m->headless)
        return;
    if (m->sceneRecording)
        EndSceneMultiview(); // submits the scene; color -> SHADER_READ, signals sceneFence
    if (!EnsureSwapchainReady(this, m))
        return; // swapchain lost + window not ready -> skip this present, retry next
    if (!m->sceneColor)
    {
        static bool w = false;
        if (!w)
        {
            w = true;
            VKB_LOG("PresentScene: no scene color (scene never opened?)\n");
        }
        return;
    }
    static unsigned s_pf = 0;
    ++s_pf;
    // #104: finite timeouts (was UINT64_MAX) as a safety net -- no vkQueueWaitIdle in the loop, so a GPU/sync stall
    // degrades to a skipped frame + a throttled log naming the stuck op, instead of a hard hang. 1s >> any real frame.
    const uint64_t kWaitNs = 1000000000ull;
    if (vkWaitForFences(m->device, 1, &m->sceneFence, VK_TRUE, kWaitNs) !=
        VK_SUCCESS)
    {
        static unsigned n = 0;
        if ((n++ % 64) == 0)
            VKB_LOG("PresentScene #%u: sceneFence WAIT TIMEOUT -> skip frame "
                    "(n=%u)\n",
                    s_pf, n);
        return;
    }

    uint32_t slot = (m->frameSlot + 1) % kMaxFramesInFlight;
    m->frameSlot = slot;
    if (vkWaitForFences(m->device, 1, &m->fenceInFlight[slot], VK_TRUE,
                        kWaitNs) != VK_SUCCESS)
    {
        static unsigned n = 0;
        if ((n++ % 64) == 0)
            VKB_LOG("PresentScene #%u: fenceInFlight[%u] WAIT TIMEOUT -> skip "
                    "frame (n=%u)\n",
                    s_pf, slot, n);
        return;
    }

    uint32_t imgIdx = 0;
    VkResult acq =
        vkAcquireNextImageKHR(m->device, m->swapchain, kWaitNs,
                              m->semImageAvail[slot], VK_NULL_HANDLE, &imgIdx);
    if (acq == VK_TIMEOUT || acq == VK_NOT_READY)
    {
        static unsigned n = 0;
        if ((n++ % 64) == 0)
            VKB_LOG("PresentScene #%u: acquire TIMEOUT -> skip frame (n=%u)\n",
                    s_pf, n);
        return;
    }
    if (acq != VK_SUCCESS)
        VKB_LOG("PresentScene frame #%u: acquire=%d\n", s_pf,
                (int)acq); // any non-SUCCESS
    if (acq == VK_ERROR_OUT_OF_DATE_KHR)
    {
        m->resizeRequested = true;
        return;
    } // deferred recreate next frame
    if (acq != VK_SUCCESS && acq != VK_SUBOPTIMAL_KHR)
        return;
    vkResetFences(m->device, 1, &m->fenceInFlight[slot]);

    VkCommandBuffer c = m->cmd[slot];
    vkResetCommandBuffer(c, 0);
    VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(c, &bi);

    auto barrier = [&](VkImage img, uint32_t layer, VkImageLayout oldL,
                       VkImageLayout newL, VkAccessFlags sa, VkAccessFlags da,
                       VkPipelineStageFlags ss, VkPipelineStageFlags ds)
    {
        VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        b.oldLayout = oldL;
        b.newLayout = newL;
        b.srcQueueFamilyIndex = b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        b.image = img;
        b.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, layer, 1};
        b.srcAccessMask = sa;
        b.dstAccessMask = da;
        vkCmdPipelineBarrier(c, ss, ds, 0, 0, nullptr, 0, nullptr, 1, &b);
    };
    extern bool g_bVulkanPresentTest;
    if (g_bVulkanPresentTest)
    {
        // DIAGNOSTIC: ignore the scene, clear the swapchain image to a per-frame cycling colour.
        barrier(m->scImages[imgIdx], 0, VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0,
                VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT);
        VkClearColorValue cc{};
        unsigned k =
            (s_pf / 30u) % 3u; // ~0.5s per colour at 60fps: R -> G -> B
        cc.float32[k] = 1.0f;
        VkImageSubresourceRange rng{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        vkCmdClearColorImage(c, m->scImages[imgIdx],
                             VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &cc, 1,
                             &rng);
        barrier(m->scImages[imgIdx], 0, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_ACCESS_TRANSFER_WRITE_BIT,
                0, VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
    }
    else
    {
        barrier(m->sceneColor, 0, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_SHADER_READ_BIT,
                VK_ACCESS_TRANSFER_READ_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT);
        // srcStage = TRANSFER, matching pWaitDstStageMask below: this write must sit INSIDE the semImageAvail gate, or it
        // races the acquire's read of the same image (SYNC-HAZARD-WRITE-AFTER-READ). See the note in BlitBitmap565.
        barrier(m->scImages[imgIdx], 0, VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0,
                VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT);

        VkImageBlit blit{};
        blit.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0,
                               1}; // scene layer 0 (the mono/left view)
        blit.srcOffsets[1] = {m->sceneW, m->sceneH, 1};
        blit.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        blit.dstOffsets[1] = {(int)m->scExtent.width, (int)m->scExtent.height,
                              1};
        vkCmdBlitImage(c, m->sceneColor, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       m->scImages[imgIdx],
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit,
                       VK_FILTER_LINEAR);

        barrier(m->scImages[imgIdx], 0, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_ACCESS_TRANSFER_WRITE_BIT,
                0, VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
        barrier(m->sceneColor, 0, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    }
    vkEndCommandBuffer(c);

    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    si.waitSemaphoreCount = 1;
    si.pWaitSemaphores = &m->semImageAvail[slot];
    si.pWaitDstStageMask = &waitStage;
    si.commandBufferCount = 1;
    si.pCommandBuffers = &c;
    si.signalSemaphoreCount = 1;
    si.pSignalSemaphores = &m->semRenderDone[imgIdx]; // per-image

    VkPresentInfoKHR pi{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    pi.waitSemaphoreCount = 1;
    pi.pWaitSemaphores = &m->semRenderDone[imgIdx];
    pi.swapchainCount = 1;
    pi.pSwapchains = &m->swapchain;
    pi.pImageIndices = &imgIdx;
    VkResult pr;
    {
        std::lock_guard<std::mutex> _qlk(
            g_vkQueueMutex); // submit+present as one critical section on the shared queue
        vkQueueSubmit(m->queue, 1, &si, m->fenceInFlight[slot]);
        pr = vkQueuePresentKHR(m->queue, &pi);
    }
    if (pr == VK_ERROR_OUT_OF_DATE_KHR || pr == VK_SUBOPTIMAL_KHR)
    {
        VKB_LOG("PresentScene frame #%u: present=%d -> resize requested "
                "(deferred to next frame)\n",
                s_pf, (int)pr);
        m->resizeRequested =
            true; // recreate at the START of the next present, not mid-frame (vkguide pattern)
        return;
    }
    else if (pr != VK_SUCCESS)
        VKB_LOG("PresentScene frame #%u: present=%d\n", s_pf, (int)pr);
    // Artscout - 2026 (#104): NO vkQueueWaitIdle here. The full-drain "serialization" (added earlier as a bring-up
    // crutch) forced exactly one frame in flight, which collapsed the swapchain to a single reused image -- acquire
    // handed back the SAME imgIdx every frame and FIFO present intermittently stopped flipping (frozen picture while
    // the loop kept running). The per-image render-done semaphore + per-frame-slot fence already make the multi-frame
    // loop correct, so let the swapchain cycle its images normally.
}

// Artscout - 2026: the runtime picks an sRGB XR format, so a copy INTO it
// encodes gamma -- and the scene bytes are already encoded. These build the
// one-triangle pass of ffxrblit.hlsl, which decodes and cancels that encode.
static bool XrFormatIsSrgb(VkFormat f)
{
    return f == VK_FORMAT_R8G8B8A8_SRGB || f == VK_FORMAT_B8G8R8A8_SRGB;
}

static VkShaderModule MakeShaderModule(VulkanBackend::Impl* m, FFShaderId id)
{
    unsigned int n = 0;
    const void* p = FFGetShaderBlob(id, FFSHADER_SPIRV, &n);
    if (!p || n < 4)
        return VK_NULL_HANDLE;

    VkShaderModuleCreateInfo ci{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    ci.codeSize = (size_t)n;
    ci.pCode = (const uint32_t*)p;
    VkShaderModule mod = VK_NULL_HANDLE;
    if (vkCreateShaderModule(m->device, &ci, nullptr, &mod) != VK_SUCCESS)
        return VK_NULL_HANDLE;
    return mod;
}

static bool EnsureXrBlitPipeline(VulkanBackend::Impl* m)
{
    if (m->xrBlitPipe != VK_NULL_HANDLE)
        return true;
    if (m->device == VK_NULL_HANDLE || !XrFormatIsSrgb(m->xrColorFormat))
        return false;

    if (m->xrBlitSampler == VK_NULL_HANDLE)
    {
        VkSamplerCreateInfo si{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        si.magFilter = si.minFilter = VK_FILTER_LINEAR;
        si.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        si.addressModeU = si.addressModeV = si.addressModeW =
            VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        si.maxLod = 0.0f;
        si.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
        if (vkCreateSampler(m->device, &si, nullptr, &m->xrBlitSampler) !=
            VK_SUCCESS)
            return false;
    }

    if (m->xrBlitSetLayout == VK_NULL_HANDLE)
    {
        VkDescriptorSetLayoutBinding b{};
        b.binding = 0;
        b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        b.descriptorCount = 1;
        b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        VkDescriptorSetLayoutCreateInfo li{
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        li.bindingCount = 1;
        li.pBindings = &b;
        if (vkCreateDescriptorSetLayout(m->device, &li, nullptr,
                                        &m->xrBlitSetLayout) != VK_SUCCESS)
            return false;
    }

    if (m->xrBlitPipeLayout == VK_NULL_HANDLE)
    {
        VkPushConstantRange pc{};
        pc.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        pc.offset = 0;
        pc.size = 16; // float2 uvScale + pad, matching XrBlitPush
        VkPipelineLayoutCreateInfo pl{
            VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        pl.setLayoutCount = 1;
        pl.pSetLayouts = &m->xrBlitSetLayout;
        pl.pushConstantRangeCount = 1;
        pl.pPushConstantRanges = &pc;
        if (vkCreatePipelineLayout(m->device, &pl, nullptr,
                                   &m->xrBlitPipeLayout) != VK_SUCCESS)
            return false;
    }

    if (m->xrBlitDescPool == VK_NULL_HANDLE)
    {
        VkDescriptorPoolSize ps{};
        ps.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        ps.descriptorCount = 16;
        VkDescriptorPoolCreateInfo pi{
            VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        pi.maxSets = 16;
        pi.poolSizeCount = 1;
        pi.pPoolSizes = &ps;
        if (vkCreateDescriptorPool(m->device, &pi, nullptr,
                                   &m->xrBlitDescPool) != VK_SUCCESS)
            return false;
    }

    if (m->xrBlitVs == VK_NULL_HANDLE)
        m->xrBlitVs = MakeShaderModule(m, FFSHADER_XRBLIT_VS);
    if (m->xrBlitPs == VK_NULL_HANDLE)
        m->xrBlitPs = MakeShaderModule(m, FFSHADER_XRBLIT_PS);
    if (m->xrBlitVs == VK_NULL_HANDLE || m->xrBlitPs == VK_NULL_HANDLE)
    {
        VKB_LOG("XrBlit: ffxrblit SPIR-V missing -- falling back to the blit\n");
        return false;
    }

    if (m->xrBlitPass == VK_NULL_HANDLE)
    {
        VkAttachmentDescription att{};
        att.format = m->xrColorFormat;
        att.samples = VK_SAMPLE_COUNT_1_BIT;
        att.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        att.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        att.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        att.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        att.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        // The layout the OpenXR runtime expects on release.
        att.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        VkAttachmentReference ref{0,
                                  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        VkSubpassDescription sub{};
        sub.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        sub.colorAttachmentCount = 1;
        sub.pColorAttachments = &ref;
        VkRenderPassCreateInfo rp{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
        rp.attachmentCount = 1;
        rp.pAttachments = &att;
        rp.subpassCount = 1;
        rp.pSubpasses = &sub;
        if (vkCreateRenderPass(m->device, &rp, nullptr, &m->xrBlitPass) !=
            VK_SUCCESS)
            return false;
    }

    VkPipelineShaderStageCreateInfo st[2] = {};
    st[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    st[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    st[0].module = m->xrBlitVs;
    st[0].pName = "VS_XrBlit";
    st[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    st[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    st[1].module = m->xrBlitPs;
    st[1].pName = "PS_XrBlit";

    VkPipelineVertexInputStateCreateInfo vi{
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    VkPipelineInputAssemblyStateCreateInfo ia{
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkPipelineViewportStateCreateInfo vp{
        VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    vp.viewportCount = vp.scissorCount = 1;
    VkPipelineRasterizationStateCreateInfo rs{
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rs.polygonMode = VK_POLYGON_MODE_FILL;
    rs.cullMode = VK_CULL_MODE_NONE;
    rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rs.lineWidth = 1.0f;
    VkPipelineMultisampleStateCreateInfo ms{
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    VkPipelineDepthStencilStateCreateInfo ds{
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    VkPipelineColorBlendAttachmentState cba{};
    cba.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                         VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    VkPipelineColorBlendStateCreateInfo cb{
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    cb.attachmentCount = 1;
    cb.pAttachments = &cba;
    const VkDynamicState dyn[2] = {VK_DYNAMIC_STATE_VIEWPORT,
                                   VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dsi{
        VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dsi.dynamicStateCount = 2;
    dsi.pDynamicStates = dyn;

    VkGraphicsPipelineCreateInfo gp{
        VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    gp.stageCount = 2;
    gp.pStages = st;
    gp.pVertexInputState = &vi;
    gp.pInputAssemblyState = &ia;
    gp.pViewportState = &vp;
    gp.pRasterizationState = &rs;
    gp.pMultisampleState = &ms;
    gp.pDepthStencilState = &ds;
    gp.pColorBlendState = &cb;
    gp.pDynamicState = &dsi;
    gp.layout = m->xrBlitPipeLayout;
    gp.renderPass = m->xrBlitPass;
    if (vkCreateGraphicsPipelines(m->device, VK_NULL_HANDLE, 1, &gp, nullptr,
                                  &m->xrBlitPipe) != VK_SUCCESS)
    {
        m->xrBlitPipe = VK_NULL_HANDLE;
        VKB_LOG("XrBlit: pipeline creation failed\n");
        return false;
    }
    VKB_LOG("XrBlit: sRGB-decoding pass ready (fmt=%d)\n",
            (int)m->xrColorFormat);
    return true;
}

// The framebuffer for one XR image, kept for the life of the swapchain: the
// runtime hands back the same images every frame.
static VkFramebuffer XrBlitTargetFbo(VulkanBackend::Impl* m, VkImage img, int w,
                                     int h)
{
    for (size_t i = 0; i < m->xrBlitTargets.size(); ++i)
        if (m->xrBlitTargets[i].img == img && m->xrBlitTargets[i].w == w &&
            m->xrBlitTargets[i].h == h)
            return m->xrBlitTargets[i].fb;

    VulkanBackend::Impl::XrBlitTarget t;
    t.img = img;
    t.view = VK_NULL_HANDLE;
    t.fb = VK_NULL_HANDLE;
    t.w = w;
    t.h = h;

    VkImageViewCreateInfo iv{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    iv.image = img;
    iv.viewType = VK_IMAGE_VIEW_TYPE_2D;
    iv.format = m->xrColorFormat;
    iv.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    if (vkCreateImageView(m->device, &iv, nullptr, &t.view) != VK_SUCCESS)
        return VK_NULL_HANDLE;

    VkFramebufferCreateInfo fb{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    fb.renderPass = m->xrBlitPass;
    fb.attachmentCount = 1;
    fb.pAttachments = &t.view;
    fb.width = (uint32_t)w;
    fb.height = (uint32_t)h;
    fb.layers = 1;
    if (vkCreateFramebuffer(m->device, &fb, nullptr, &t.fb) != VK_SUCCESS)
    {
        vkDestroyImageView(m->device, t.view, nullptr);
        return VK_NULL_HANDLE;
    }
    m->xrBlitTargets.push_back(t);
    return t.fb;
}

// Next set in the ring, pointed at `view`. See kXrBlitSets on why rewriting
// rather than caching is the safe order here.
static VkDescriptorSet XrBlitSourceSet(VulkanBackend::Impl* m, VkImageView view)
{
    const unsigned int i =
        m->xrBlitSetNext++ % (unsigned int)VulkanBackend::Impl::kXrBlitSets;
    VkDescriptorSet set = m->xrBlitSet[i];

    if (set == VK_NULL_HANDLE)
    {
        VkDescriptorSetAllocateInfo ai{
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        ai.descriptorPool = m->xrBlitDescPool;
        ai.descriptorSetCount = 1;
        ai.pSetLayouts = &m->xrBlitSetLayout;
        if (vkAllocateDescriptorSets(m->device, &ai, &set) != VK_SUCCESS)
            return VK_NULL_HANDLE;
        m->xrBlitSet[i] = set;
    }

    VkDescriptorImageInfo ii{};
    ii.sampler = m->xrBlitSampler;
    ii.imageView = view;
    ii.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    VkWriteDescriptorSet w{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    w.dstSet = set;
    w.dstBinding = 0;
    w.descriptorCount = 1;
    w.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    w.pImageInfo = &ii;
    vkUpdateDescriptorSets(m->device, 1, &w, 0, nullptr);
    return set;
}

// Record the decoding copy of `srcView` into `dst`. srcU/srcV are the fraction
// of the source actually rendered (the eye's sub-rect of the reused target).
static bool RecordXrBlit(VulkanBackend::Impl* m, VkCommandBuffer c,
                         VkImageView srcView, VkImage dst, int dw, int dh,
                         float srcU, float srcV)
{
    if (!EnsureXrBlitPipeline(m))
        return false;

    const VkFramebuffer fbo = XrBlitTargetFbo(m, dst, dw, dh);
    const VkDescriptorSet set = XrBlitSourceSet(m, srcView);
    if (fbo == VK_NULL_HANDLE || set == VK_NULL_HANDLE)
        return false;

    VkRenderPassBeginInfo rb{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    rb.renderPass = m->xrBlitPass;
    rb.framebuffer = fbo;
    rb.renderArea.extent = {(uint32_t)dw, (uint32_t)dh};
    vkCmdBeginRenderPass(c, &rb, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport vp{0.0f, 0.0f, (float)dw, (float)dh, 0.0f, 1.0f};
    VkRect2D sc{{0, 0}, {(uint32_t)dw, (uint32_t)dh}};
    vkCmdSetViewport(c, 0, 1, &vp);
    vkCmdSetScissor(c, 0, 1, &sc);
    vkCmdBindPipeline(c, VK_PIPELINE_BIND_POINT_GRAPHICS, m->xrBlitPipe);
    vkCmdBindDescriptorSets(c, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m->xrBlitPipeLayout, 0, 1, &set, 0, nullptr);
    const float push[4] = {srcU, srcV, 0.0f, 0.0f};
    vkCmdPushConstants(c, m->xrBlitPipeLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                       sizeof(push), push);
    vkCmdDraw(c, 3, 1, 0, 0);
    vkCmdEndRenderPass(c);
    return true;
}

// Single-layer views of the scene array, the shader path's source. Rebuilt
// whenever the scene target is.
static VkImageView XrBlitSceneView(VulkanBackend::Impl* m, int layer)
{
    if (layer < 0 || layer >= 4 || !m->sceneColor)
        return VK_NULL_HANDLE;
    if (m->xrBlitSrcOwner != m->sceneColor)
        return VK_NULL_HANDLE; // caller refreshes first
    if (m->xrBlitSrcView[layer] != VK_NULL_HANDLE)
        return m->xrBlitSrcView[layer];

    VkImageViewCreateInfo iv{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    iv.image = m->sceneColor;
    iv.viewType = VK_IMAGE_VIEW_TYPE_2D;
    iv.format = m->sceneFormat;
    iv.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, (uint32_t)layer, 1};
    if (vkCreateImageView(m->device, &iv, nullptr,
                          &m->xrBlitSrcView[layer]) != VK_SUCCESS)
        m->xrBlitSrcView[layer] = VK_NULL_HANDLE;
    return m->xrBlitSrcView[layer];
}

// Sets referencing the old views must go with them, so the pool is reset.
static void DestroyXrBlitSrcViews(VulkanBackend::Impl* m)
{
    if (m->device == VK_NULL_HANDLE)
        return;
    for (int i = 0; i < 4; ++i)
        if (m->xrBlitSrcView[i] != VK_NULL_HANDLE)
        {
            vkDestroyImageView(m->device, m->xrBlitSrcView[i], nullptr);
            m->xrBlitSrcView[i] = VK_NULL_HANDLE;
        }
    m->xrBlitSrcOwner = VK_NULL_HANDLE;
}

static void DestroyXrBlit(VulkanBackend::Impl* m)
{
    if (m->device == VK_NULL_HANDLE)
        return;
    DestroyXrBlitSrcViews(m);
    for (size_t i = 0; i < m->xrBlitTargets.size(); ++i)
    {
        if (m->xrBlitTargets[i].fb)
            vkDestroyFramebuffer(m->device, m->xrBlitTargets[i].fb, nullptr);
        if (m->xrBlitTargets[i].view)
            vkDestroyImageView(m->device, m->xrBlitTargets[i].view, nullptr);
    }
    m->xrBlitTargets.clear();
    if (m->xrBlitPipe)
        vkDestroyPipeline(m->device, m->xrBlitPipe, nullptr);
    if (m->xrBlitPass)
        vkDestroyRenderPass(m->device, m->xrBlitPass, nullptr);
    if (m->xrBlitPipeLayout)
        vkDestroyPipelineLayout(m->device, m->xrBlitPipeLayout, nullptr);
    if (m->xrBlitSetLayout)
        vkDestroyDescriptorSetLayout(m->device, m->xrBlitSetLayout, nullptr);
    if (m->xrBlitDescPool)
        vkDestroyDescriptorPool(m->device, m->xrBlitDescPool, nullptr);
    for (int i = 0; i < VulkanBackend::Impl::kXrBlitSets; ++i)
        m->xrBlitSet[i] = VK_NULL_HANDLE; // freed with the pool
    m->xrBlitSetNext = 0;
    if (m->xrBlitSampler)
        vkDestroySampler(m->device, m->xrBlitSampler, nullptr);
    if (m->xrBlitVs)
        vkDestroyShaderModule(m->device, m->xrBlitVs, nullptr);
    if (m->xrBlitPs)
        vkDestroyShaderModule(m->device, m->xrBlitPs, nullptr);
    m->xrBlitPipe = VK_NULL_HANDLE;
    m->xrBlitPass = VK_NULL_HANDLE;
    m->xrBlitPipeLayout = VK_NULL_HANDLE;
    m->xrBlitSetLayout = VK_NULL_HANDLE;
    m->xrBlitDescPool = VK_NULL_HANDLE;
    m->xrBlitSampler = VK_NULL_HANDLE;
    m->xrBlitVs = m->xrBlitPs = VK_NULL_HANDLE;
}

// Artscout - 2026: the XR color format, from the runtime's own choice. sRGB
// means every copy into an XR image must decode first -- see RecordXrBlit.
void VulkanBackend::SetXrColorFormat(unsigned int vkFormat)
{
    if (!m || m->xrColorFormat == (VkFormat)vkFormat)
        return;
    if (m->device != VK_NULL_HANDLE && m->xrBlitPipe != VK_NULL_HANDLE)
    {
        vkDeviceWaitIdle(m->device);
        DestroyXrBlit(m);
    }
    m->xrColorFormat = (VkFormat)vkFormat;
    VKB_LOG("SetXrColorFormat: %u (%s)\n", vkFormat,
            XrFormatIsSrgb((VkFormat)vkFormat) ? "sRGB -- decoding blit" :
                                                 "UNORM -- plain blit");
}

// Artscout - 2026 (#107 VR-Vulkan): blit each layer of the multiview scene array into the matching per-view XR
// swapchain image. The scene must already be ended (EndSceneMultiview -> color in SHADER_READ, sceneFence signalled).
// dstImages[e] is the VkImage acquired from XR swapchain e (arraySize 1); the blit scales the scene per-view size to
// dstW[e] x dstH[e]. Each dst is left in COLOR_ATTACHMENT_OPTIMAL, the layout the OpenXR runtime expects on release.
// Synchronous (waits its own fence) so the caller may xrReleaseSwapchainImage immediately. Peer of PresentScene's blit.
bool VulkanBackend::BlitSceneToXrImages(int nViews, void* const* dstImages,
                                        const int* dstW, const int* dstH)
{
    if (!IsValid() || !m->sceneColor || !dstImages || nViews < 1)
        return false;
    if (nViews > m->sceneViews)
        nViews = m->sceneViews;

    const uint64_t kWaitNs = 1000000000ull;
    // The scene was submitted with sceneFence; wait it so the layers are fully rendered before we read them.
    if (m->sceneFence && vkWaitForFences(m->device, 1, &m->sceneFence, VK_TRUE,
                                         kWaitNs) != VK_SUCCESS)
    {
        static unsigned n = 0;
        if ((n++ % 64) == 0)
            VKB_LOG("BlitSceneToXrImages: sceneFence TIMEOUT -> skip\n");
        return false;
    }
    WaitTailsConsumed(); // #107 PERF: the tails drew cockpit+RTT INTO the scene layers this blit reads -> wait them
    // (batched over the group's eyes) before the blit. No-op on paths without a tail.

    // #107 PERF: scene GPU time -- the scene submit is complete now (sceneFence waited above), so slots 0/1 are ready.
    if (g_bVulkanProfile && m->tsPool && m->tsPeriodNs > 0.0)
    {
        uint64_t ts[2] = {};
        if (vkGetQueryPoolResults(m->device, m->tsPool, 0, 2, sizeof(ts), ts,
                                  sizeof(uint64_t),
                                  VK_QUERY_RESULT_64_BIT) == VK_SUCCESS &&
            ts[1] > ts[0])
            g_vkProf.accum[VP_GPU_SCENE] +=
                (double)(ts[1] - ts[0]) * m->tsPeriodNs / 1e6;
    }

    if (m->vrBlitCmd == VK_NULL_HANDLE)
    {
        VkCommandBufferAllocateInfo ai{
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        ai.commandPool = m->vrBlitPool;
        ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        ai.commandBufferCount = 1;
        if (vkAllocateCommandBuffers(m->device, &ai, &m->vrBlitCmd) !=
            VK_SUCCESS)
        {
            VKB_LOG("BlitSceneToXrImages: cmd alloc failed\n");
            return false;
        }
        VkFenceCreateInfo fi{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        vkCreateFence(m->device, &fi, nullptr, &m->vrBlitFence);
    }
    vkResetFences(m->device, 1, &m->vrBlitFence);
    VkCommandBuffer c = m->vrBlitCmd;
    vkResetCommandBuffer(c, 0);
    VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(c, &bi);
    if (g_bVulkanProfile && m->tsPool)
    { // #107 PERF: stamp blit begin (slot 2) -- reset first (outside any render pass)
        vkCmdResetQueryPool(c, m->tsPool, 2, 2);
        vkCmdWriteTimestamp(c, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, m->tsPool, 2);
    }

    auto barrier = [&](VkImage img, uint32_t layer, VkImageLayout oldL,
                       VkImageLayout newL, VkAccessFlags sa, VkAccessFlags da,
                       VkPipelineStageFlags ss, VkPipelineStageFlags ds)
    {
        VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        b.oldLayout = oldL;
        b.newLayout = newL;
        b.srcQueueFamilyIndex = b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        b.image = img;
        b.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, layer, 1};
        b.srcAccessMask = sa;
        b.dstAccessMask = da;
        vkCmdPipelineBarrier(c, ss, ds, 0, 0, nullptr, 0, nullptr, 1, &b);
    };

    for (int e = 0; e < nViews; ++e)
    {
        VkImage dst = (VkImage)dstImages[e];
        if (!dst)
            continue;
        const int dw = dstW ? dstW[e] : m->sceneW;
        const int dh = dstH ? dstH[e] : m->sceneH;
        // #107: the eye rendered into the top-left [0,0,rW,rH] sub-rect of the (reused, max-sized) target -- copy only
        // that region, not the whole target. rW/rH = this eye's render size (== dst size); fallback to the full target.
        const int rW = (m->sceneRenderW > 0 && m->sceneRenderW <= m->sceneW) ?
                           m->sceneRenderW :
                           m->sceneW;
        const int rH = (m->sceneRenderH > 0 && m->sceneRenderH <= m->sceneH) ?
                           m->sceneRenderH :
                           m->sceneH;

        // Artscout - 2026: an sRGB XR image encodes on write, so the copy runs
        // through the decoding shader; the scene layer stays in SHADER_READ.
        if (XrFormatIsSrgb(m->xrColorFormat))
        {
            if (m->xrBlitSrcOwner != m->sceneColor)
            {
                DestroyXrBlitSrcViews(m);
                m->xrBlitSrcOwner = m->sceneColor;
            }
            const VkImageView sv = XrBlitSceneView(m, e);
            if (sv != VK_NULL_HANDLE &&
                RecordXrBlit(m, c, sv, dst, dw, dh, (float)rW / (float)m->sceneW,
                             (float)rH / (float)m->sceneH))
                continue;
            // Falling through leaves the picture bright rather than absent.
        }

        // scene layer e: SHADER_READ -> TRANSFER_SRC ; dst (whole image, content discarded): UNDEFINED -> TRANSFER_DST
        barrier(m->sceneColor, (uint32_t)e,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_SHADER_READ_BIT,
                VK_ACCESS_TRANSFER_READ_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT);
        barrier(dst, 0, VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0,
                VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT);

        VkImageBlit blit{};
        blit.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, (uint32_t)e,
                               1}; // scene array layer e
        blit.srcOffsets[1] = {rW, rH, 1};
        blit.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0,
                               1}; // XR image is arraySize 1
        blit.dstOffsets[1] = {dw, dh, 1};
        vkCmdBlitImage(c, m->sceneColor, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit,
                       VK_FILTER_LINEAR);

        // dst -> COLOR_ATTACHMENT_OPTIMAL (XR release layout) ; scene layer e -> SHADER_READ (restore for next frame)
        barrier(dst, 0, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_ACCESS_TRANSFER_WRITE_BIT,
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
        barrier(m->sceneColor, (uint32_t)e,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    }
    if (g_bVulkanProfile && m->tsPool) // #107 PERF: stamp blit end (slot 3)
        vkCmdWriteTimestamp(c, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, m->tsPool,
                            3);
    vkEndCommandBuffer(c);

    VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    si.commandBufferCount = 1;
    si.pCommandBuffers = &c;
    {
        std::lock_guard<std::mutex> _qlk(g_vkQueueMutex);
        vkQueueSubmit(m->queue, 1, &si, m->vrBlitFence);
    }
    VkResult _blitWr;
    PROF_WAIT(VP_BLIT_FENCE_WAIT,
              _blitWr = vkWaitForFences(m->device, 1, &m->vrBlitFence, VK_TRUE,
                                        kWaitNs));
    if (_blitWr != VK_SUCCESS)
    {
        static unsigned n = 0;
        if ((n++ % 64) == 0)
            VKB_LOG("BlitSceneToXrImages: vrBlitFence TIMEOUT\n");
        return false;
    }
    // #107 PERF: blit GPU time -- the blit submit is complete now (vrBlitFence waited), so slots 2/3 are ready.
    if (g_bVulkanProfile && m->tsPool && m->tsPeriodNs > 0.0)
    {
        uint64_t ts[2] = {};
        if (vkGetQueryPoolResults(m->device, m->tsPool, 2, 2, sizeof(ts), ts,
                                  sizeof(uint64_t),
                                  VK_QUERY_RESULT_64_BIT) == VK_SUCCESS &&
            ts[1] > ts[0])
            g_vkProf.accum[VP_GPU_BLIT] +=
                (double)(ts[1] - ts[0]) * m->tsPeriodNs / 1e6;
    }
    return true;
}

// ---------------------------------------------------------------------------------------------- RTT (HUD/MFD/DED)
// Ensure the shared RTT render pass (color = swapchain format, CLEAR->STORE, finalLayout SHADER_READ + a depth
// attachment). Render-pass-compatible with the swapchain pass so the renderer's screen/dynamic2D pipelines are valid.
static bool EnsureRttRenderPass(VulkanBackend::Impl* m)
{
    if (m->rttRenderPass && m->rttRenderPassLoad)
        return true;
    VkAttachmentDescription color{};
    // Artscout - 2026 (#104): scFormat, and it is NOT free to change. The RTT symbology is drawn through the SCREEN
    // path, whose pipelines are built against the swapchain render pass -- and a pipeline may only be bound inside a
    // render pass COMPATIBLE with the one it was created for, which for colour means the same format. So the whole
    // chain has to agree on the swapchain format: this pass, the screen pipelines, and the RTT image itself
    // (VulkanTextureManager::CreateRenderTarget, which is where the mismatch actually was).
    color.format = m->scFormat;
    color.samples = VK_SAMPLE_COUNT_1_BIT;
    color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    VkAttachmentDescription depth{};
    depth.format = m->depthFormat;
    depth.samples = VK_SAMPLE_COUNT_1_BIT;
    depth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE; // #76
    depth.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    VkAttachmentReference colorRef{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkAttachmentReference depthRef{
        1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
    VkSubpassDescription sub{};
    sub.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    sub.colorAttachmentCount = 1;
    sub.pColorAttachments = &colorRef;
    sub.pDepthStencilAttachment = &depthRef;
    // Artscout - 2026 (#104): the SAME external dependency the swapchain pass declares -- and it is not optional
    // housekeeping. The RTT symbology is drawn with the screen pipelines, which are built against the swapchain pass,
    // and a pipeline may only be bound inside a COMPATIBLE render pass. Compatibility means the two passes are
    // identical apart from layouts and load/store ops -- dependencies included. With this pass declaring none, every
    // display draw was rejected (validation: "dependencyCount is incompatible") and the MFDs/HUD stayed blank.
    VkSubpassDependency dep{};
    dep.srcSubpass = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass = 0;
    dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                       VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                       VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dep.srcAccessMask = 0;
    dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    VkAttachmentDescription atts[2] = {color, depth};
    VkRenderPassCreateInfo rp{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    rp.attachmentCount = 2;
    rp.pAttachments = atts;
    rp.subpassCount = 1;
    rp.pSubpasses = &sub;
    rp.dependencyCount = 1;
    rp.pDependencies = &dep;
    if (!m->rttRenderPass &&
        vkCreateRenderPass(m->device, &rp, nullptr, &m->rttRenderPass) !=
            VK_SUCCESS)
        return false;
    // LOAD twin: preserve the texture's existing contents on a re-bind. The GM radar accumulates its sweep across
    // frames, and the display batch re-binds the atlas mid-frame (after a GM/beam sub-render) -- a CLEAR pass at
    // either point wipes finished panels. initialLayout must match what UnbindSceneRtt leaves (SHADER_READ), so a
    // LOAD bind is only legal for a texture that has been through a CLEAR pass at least once (ImageBuffer forces
    // clear=true on the first bind of a virgin RTT for exactly that reason).
    if (!m->rttRenderPassLoad)
    {
        atts[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        atts[0].initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        // srcAccessMask: the prior pass's finalLayout transition must be visible before we load from it.
        dep.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        if (vkCreateRenderPass(m->device, &rp, nullptr,
                               &m->rttRenderPassLoad) != VK_SUCCESS)
            return false;
    }
    return true;
}

// Ensure the shared scratch depth image is at least w x h (RTT displays are small; one grown buffer suffices).
static bool EnsureRttDepth(VulkanBackend::Impl* m, int w, int h)
{
    if (m->rttDepthImage && m->rttDepthW >= w && m->rttDepthH >= h)
        return true;
    if (m->rttDepthView)
    {
        vkDestroyImageView(m->device, m->rttDepthView, nullptr);
        m->rttDepthView = VK_NULL_HANDLE;
    }
    if (m->rttDepthImage)
    {
        vkDestroyImage(m->device, m->rttDepthImage, nullptr);
        m->rttDepthImage = VK_NULL_HANDLE;
    }
    if (m->rttDepthMem)
    {
        FF_VmaFree((void*)m->rttDepthMem);
        m->rttDepthMem = VK_NULL_HANDLE;
    }
    m->rttFbCache
        .clear(); // framebuffers referenced the old depth view -> stale
    m->rttDepthW = w;
    m->rttDepthH = h;
    VkImageCreateInfo ii{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    ii.imageType = VK_IMAGE_TYPE_2D;
    ii.format = m->depthFormat;
    ii.extent = {(uint32_t)w, (uint32_t)h, 1};
    ii.mipLevels = 1;
    ii.arrayLayers = 1;
    ii.samples = VK_SAMPLE_COUNT_1_BIT;
    ii.tiling = VK_IMAGE_TILING_OPTIMAL;
    ii.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    ii.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    if (vkCreateImage(m->device, &ii, nullptr, &m->rttDepthImage) != VK_SUCCESS)
        return false;
    FF_VmaAllocImageMemory(m->rttDepthImage, (void**)&m->rttDepthMem);
    VkImageViewCreateInfo vi{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    vi.image = m->rttDepthImage;
    vi.viewType = VK_IMAGE_VIEW_TYPE_2D;
    vi.format = m->depthFormat;
    vi.subresourceRange = {DepthAspect(m), 0, 1, 0, 1};
    return vkCreateImageView(m->device, &vi, nullptr, &m->rttDepthView) ==
           VK_SUCCESS;
}

// End the open RTT pass and submit its command buffer (the shared tail of UnbindSceneRtt and a bind-over-bind
// switch). Ring bookkeeping identical to the original UnbindSceneRtt body.
static void EndRttPass(VulkanBackend::Impl* m)
{
    if (!m->rttActive || !m->rttCmd)
        return;
    vkCmdEndRenderPass(
        m->rttCmd); // finalLayout -> SHADER_READ so the panel quad can sample the texture
    vkEndCommandBuffer(m->rttCmd);
    const int slot = m->rttCurSlot;
    VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    si.commandBufferCount = 1;
    si.pCommandBuffers = &m->rttCmd;
    {
        std::lock_guard<std::mutex> _qlk(g_vkQueueMutex);
        vkResetFences(m->device, 1, &m->rttRingFence[slot]);
        vkQueueSubmit(m->queue, 1, &si, m->rttRingFence[slot]);
    }
    m->rttLastSub = slot;
    m->rttNeedConsume = true;
    m->rttRingIdx = (slot + 1) % VulkanBackend::Impl::kRttRing;
    m->rttActive = false;
    m->rttCurSlot = -1;
    m->rttW = m->rttH = 0;
    m->rttBoundHandle = nullptr;
}

void VulkanBackend::BindSceneRtt(void* vulkanTexHandle, int w, int h,
                                 bool clear)
{
    if (!IsValid() || !vulkanTexHandle || w <= 0 || h <= 0)
        return;
    // Artscout - 2026 (GM radar): a bind while another RTT pass is open is a TARGET SWITCH, not an error. The GM
    // sweep sub-render binds its private 128x128 buffer in the middle of the atlas batch (and the atlas is then
    // re-bound after). End-and-submit the open pass, then open the new one. (D3D12 just swaps the RTV here.)
    if (m->rttActive)
        EndRttPass(m);
    // The handle is a VulkanTexture* (from CreateRenderTarget). Its layout is {uint64 image, memory, view,
    // descriptor; int w,h} -- read the color VkImageView (the 3rd uint64) by offset, so this TU does not need the
    // VulkanTexture type visible (avoids an MSVC include-resolution issue with VulkanTextureManager.h here).
    VkImageView colorView =
        (VkImageView)(((const uint64_t*)vulkanTexHandle)[2]);
    if (!colorView)
        return;
    if (!EnsureRttRenderPass(m) || !EnsureRttDepth(m, w, h))
        return;

    VkFramebuffer fbo = VK_NULL_HANDLE;
    auto it = m->rttFbCache.find(colorView);
    if (it != m->rttFbCache.end())
        fbo = it->second;
    else
    {
        VkImageView fbAtts[2] = {colorView, m->rttDepthView};
        VkFramebufferCreateInfo fb{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        fb.renderPass = m->rttRenderPass;
        fb.attachmentCount = 2;
        fb.pAttachments = fbAtts;
        fb.width = (uint32_t)w;
        fb.height = (uint32_t)h;
        fb.layers = 1;
        if (vkCreateFramebuffer(m->device, &fb, nullptr, &fbo) != VK_SUCCESS)
            return;
        m->rttFbCache[colorView] = fbo;
    }

    // #107 PERF: take the next RING slot. Alloc lazily (fence created SIGNALED so the first reuse-wait passes). A slot is
    // reset only after its fence signals -> the buffer is idle; no per-RTT vkQueueWaitIdle. The ring is deep enough that
    // by the time we wrap back the slot's RTT has long been consumed, so this wait almost never blocks.
    {
        int i = m->rttRingIdx;
        if (m->rttRing[i] == VK_NULL_HANDLE)
        {
            VkCommandBufferAllocateInfo cai{
                VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
            cai.commandPool = m->rttPool;
            cai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            cai.commandBufferCount = 1;
            if (vkAllocateCommandBuffers(m->device, &cai, &m->rttRing[i]) !=
                VK_SUCCESS)
            {
                m->rttRing[i] = VK_NULL_HANDLE;
                return;
            }
            VkFenceCreateInfo fci{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
            fci.flags = VK_FENCE_CREATE_SIGNALED_BIT;
            if (vkCreateFence(m->device, &fci, nullptr, &m->rttRingFence[i]) !=
                VK_SUCCESS)
            {
                m->rttRingFence[i] = VK_NULL_HANDLE;
                return;
            }
        }
        else
        {
            vkWaitForFences(
                m->device, 1, &m->rttRingFence[i], VK_TRUE,
                UINT64_MAX); // slot's prior RTT done -> safe to reset
            vkResetCommandBuffer(m->rttRing[i], 0);
        }
        m->rttCurSlot = i;
        m->rttCmd =
            m->rttRing
                [i]; // alias: all rttCmd references (draws, viewport, FlatCommandBuffer) use the current slot
    }
    VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(m->rttCmd, &bi);
    VkClearValue clears[2];
    clears[0].color = {
        {0.0f, 0.0f, 0.0f,
         0.0f}}; // transparent black -> the panel composites the symbology
    clears[1].depthStencil = {0.0f, 0};
    VkRenderPassBeginInfo rpb{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    // Honor `clear` at last: CLEAR pass wipes (first bind of a batch), LOAD pass preserves (GM sweep accumulation,
    // mid-batch atlas re-binds). The clear values are ignored by LOAD attachments, so passing them is harmless.
    rpb.renderPass = clear ? m->rttRenderPass : m->rttRenderPassLoad;
    rpb.framebuffer = fbo;
    rpb.renderArea.extent = {(uint32_t)w, (uint32_t)h};
    rpb.clearValueCount = 2;
    rpb.pClearValues = clears;
    vkCmdBeginRenderPass(m->rttCmd, &rpb, VK_SUBPASS_CONTENTS_INLINE);
    VkViewport vp{0, 0, (float)w, (float)h, 0.0f, 1.0f};
    VkRect2D sc{{0, 0}, {(uint32_t)w, (uint32_t)h}};
    vkCmdSetViewport(m->rttCmd, 0, 1, &vp);
    vkCmdSetScissor(m->rttCmd, 0, 1, &sc);
    m->rttActive = true;
    m->rttW = w;
    m->rttH = h; // the render area ClearCurrentRTV/ClearDepth must stay inside
    m->rttBoundHandle =
        vulkanTexHandle; // so a stale Unbind(otherHandle) can be recognised and ignored
    ++m->rttVpSerial; // fresh command buffer -> its viewport is the full extent again
    // The confined zone dies on ANY bind (including a same-handle re-bind): it scopes exactly the sensor bracket's
    // Confine -> flush window. Surviving past the bracket's closing ReBindRttTarget would leave the zone SCISSOR
    // armed for the 2D path, clipping away the symbology of every display drawn after the sensor page.
    m->rttZoneHandle = nullptr;
}

void VulkanBackend::UnbindSceneRtt(void* vulkanTexHandle)
{
    // #107 VR-Vulkan: the in-3D menu is its OWN pass/command buffer (BindMenuRtt), not the display-RTT rttCmd. otwloop
    // finalizes it with UnbindSceneRtt(menuTex), so recognise the menu handle and end the menu pass instead.
    if (m->menuActive && vulkanTexHandle == (void*)m->menuRttHandle)
    {
        EndMenuRtt();
        return;
    }
    // Artscout - 2026 (GM radar): an unbind naming a handle that is NOT the currently-bound RTT is stale (its
    // pass was already ended by a bind-over-bind switch) -- ignore it, do not close somebody else's pass.
    // NULL keeps the legacy meaning "whatever is current" (BindBackBufferRTV).
    if (vulkanTexHandle && m->rttActive && vulkanTexHandle != m->rttBoundHandle)
        return;
    // The zone rect dies with the batch it was confined for (the next display confines its own before its flush).
    {
        void* h = vulkanTexHandle ? vulkanTexHandle : m->rttBoundHandle;
        if (h && h == m->rttZoneHandle)
            m->rttZoneHandle = nullptr;
    }
    // #107 PERF: EndRttPass submits WITHOUT a wait -- it signals the slot's fence and records it as the last pending
    // RTT. The reader (tail/scene/blit) calls WaitRttConsumed() once before its own submit, which waits that fence:
    // because submits on the single queue execute in order, that one wait guarantees THIS and every earlier RTT this
    // batch is finished, so the panel samples valid textures. The ring lets the next bind proceed immediately.
    EndRttPass(m);
}

// #107 PERF: block until every RTT submitted since the last consume is finished (so a reader can safely sample the RTT
// textures). Waits only the LAST pending RTT fence -- single-queue in-order execution means all earlier RTTs of the
// batch are done too. Called before the tail/scene/blit submit that reads the RTTs. No-op when nothing is pending.
void VulkanBackend::WaitRttConsumed()
{
    if (!m || !m->rttNeedConsume || m->rttLastSub < 0)
        return;
    if (m->rttRingFence[m->rttLastSub])
        vkWaitForFences(m->device, 1, &m->rttRingFence[m->rttLastSub], VK_TRUE,
                        UINT64_MAX);
    m->rttNeedConsume = false;
}

// ------------------------------------------------------------------------------------------- in-3D menu target (#107)
// Owned SCENE-FORMAT color+depth target for the in-scene VR menu (2D comms/AWACS text + the 3D exit-dialog BSP), with
// its OWN single-view render pass. This is the fix for "the menu draws head-locked into the eye, the quad is empty":
// the exit dialog is a 3D BSP that draws through the object pipelines, and those pipelines are pass-locked to a
// scene-FORMAT pass. The old menu RTT was scFormat (a display-RTT pass) -- incompatible -- so BeginObjectPass routed
// the dialog to the eye's sceneCmd instead, and only the 2D text (if anything) reached the quad. Here menuPass is
// sceneFormat and single-view; VulkanRenderer builds a menu pipeline variant against it and routes BOTH object and
// screen draws to menuCmd while IsMenuActive(). SubmitInSceneMenuQuad then copies menuColor into the XR quad image.
void VulkanBackend::EnsureMenuRtt(int w, int h)
{
    if (!IsValid() || w <= 0 || h <= 0)
        return;
    if (m->menuColor && m->menuRttW == w && m->menuRttH == h)
        return; // already at size
    // size changed -> tear the size-dependent objects down (the pass is size-independent -> created once, kept:
    // VulkanRenderer's menu pipelines are built against it and a rebuild would strand them).
    if (m->menuFbo)
    {
        vkDestroyFramebuffer(m->device, m->menuFbo, nullptr);
        m->menuFbo = VK_NULL_HANDLE;
    }
    if (m->menuColorView)
    {
        vkDestroyImageView(m->device, m->menuColorView, nullptr);
        m->menuColorView = VK_NULL_HANDLE;
    }
    if (m->menuColor)
    {
        vkDestroyImage(m->device, m->menuColor, nullptr);
        m->menuColor = VK_NULL_HANDLE;
    }
    if (m->menuColorMem)
    {
        FF_VmaFree((void*)m->menuColorMem);
        m->menuColorMem = VK_NULL_HANDLE;
    }
    if (m->menuDepthView)
    {
        vkDestroyImageView(m->device, m->menuDepthView, nullptr);
        m->menuDepthView = VK_NULL_HANDLE;
    }
    if (m->menuDepth)
    {
        vkDestroyImage(m->device, m->menuDepth, nullptr);
        m->menuDepth = VK_NULL_HANDLE;
    }
    if (m->menuDepthMem)
    {
        FF_VmaFree((void*)m->menuDepthMem);
        m->menuDepthMem = VK_NULL_HANDLE;
    }
    m->menuRttHandle[0] = m->menuRttHandle[1] = m->menuRttHandle[2] = 0;
    m->menuRttW = m->menuRttH = 0;

    auto makeImg = [&](VkFormat fmt, VkImageUsageFlags usage,
                       VkImageAspectFlags aspect, VkImage& img,
                       VkDeviceMemory& mem, VkImageView& view) -> bool
    {
        VkImageCreateInfo ii{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        ii.imageType = VK_IMAGE_TYPE_2D;
        ii.format = fmt;
        ii.extent = {(uint32_t)w, (uint32_t)h, 1};
        ii.mipLevels = 1;
        ii.arrayLayers = 1;
        ii.samples = VK_SAMPLE_COUNT_1_BIT;
        ii.tiling = VK_IMAGE_TILING_OPTIMAL;
        ii.usage = usage;
        ii.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        if (vkCreateImage(m->device, &ii, nullptr, &img) != VK_SUCCESS)
        {
            img = VK_NULL_HANDLE;
            return false;
        }
        if (not FF_VmaAllocImageMemory(img, (void**)&mem))
        {
            vkDestroyImage(m->device, img, nullptr);
            img = VK_NULL_HANDLE;
            return false;
        }
        VkImageViewCreateInfo vi{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        vi.image = img;
        vi.viewType = VK_IMAGE_VIEW_TYPE_2D;
        vi.format = fmt;
        vi.subresourceRange = {aspect, 0, 1, 0, 1};
        if (vkCreateImageView(m->device, &vi, nullptr, &view) != VK_SUCCESS)
        {
            view = VK_NULL_HANDLE;
            return false;
        }
        return true;
    };
    if (!makeImg(m->sceneFormat,
                 VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                     VK_IMAGE_USAGE_SAMPLED_BIT |
                     VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                 VK_IMAGE_ASPECT_COLOR_BIT, m->menuColor, m->menuColorMem,
                 m->menuColorView))
    {
        VKB_LOG("EnsureMenuRtt: color image failed\n");
        return;
    }
    if (!makeImg(m->depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                 DepthAspect(m), m->menuDepth, m->menuDepthMem,
                 m->menuDepthView))
    {
        VKB_LOG("EnsureMenuRtt: depth image failed\n");
        return;
    }

    // The pass mirrors the scene pass's attachment ops (so the object pipelines' depth/stencil expectations match) but
    // is SINGLE-view: no VkRenderPassMultiviewCreateInfo. Color finalLayout SHADER_READ so BlitTexToXrImage can sample.
    if (!m->menuPass)
    {
        VkAttachmentDescription color{};
        color.format = m->sceneFormat;
        color.samples = VK_SAMPLE_COUNT_1_BIT;
        color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        color.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        VkAttachmentDescription depth{};
        depth.format = m->depthFormat;
        depth.samples = VK_SAMPLE_COUNT_1_BIT;
        depth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depth.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depth.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE; // #76
        depth.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depth.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        VkAttachmentReference colorRef{
            0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        VkAttachmentReference depthRef{
            1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
        VkSubpassDescription sub{};
        sub.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        sub.colorAttachmentCount = 1;
        sub.pColorAttachments = &colorRef;
        sub.pDepthStencilAttachment = &depthRef;
        VkAttachmentDescription atts[2] = {color, depth};
        VkRenderPassCreateInfo rp{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
        rp.attachmentCount = 2;
        rp.pAttachments = atts;
        rp.subpassCount = 1;
        rp.pSubpasses = &sub;
        if (vkCreateRenderPass(m->device, &rp, nullptr, &m->menuPass) !=
            VK_SUCCESS)
        {
            m->menuPass = VK_NULL_HANDLE;
            VKB_LOG("EnsureMenuRtt: pass failed\n");
            return;
        }
    }

    VkImageView fbAtts[2] = {m->menuColorView, m->menuDepthView};
    VkFramebufferCreateInfo fb{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    fb.renderPass = m->menuPass;
    fb.attachmentCount = 2;
    fb.pAttachments = fbAtts;
    fb.width = (uint32_t)w;
    fb.height = (uint32_t)h;
    fb.layers = 1;
    if (vkCreateFramebuffer(m->device, &fb, nullptr, &m->menuFbo) != VK_SUCCESS)
    {
        m->menuFbo = VK_NULL_HANDLE;
        VKB_LOG("EnsureMenuRtt: fbo failed\n");
        return;
    }

    if (m->menuCmd == VK_NULL_HANDLE)
    { // allocated from rttPool (freed with it); persists across binds
        VkCommandBufferAllocateInfo cai{
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        cai.commandPool = m->rttPool;
        cai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cai.commandBufferCount = 1;
        if (vkAllocateCommandBuffers(m->device, &cai, &m->menuCmd) !=
            VK_SUCCESS)
        {
            m->menuCmd = VK_NULL_HANDLE;
            VKB_LOG("EnsureMenuRtt: cmd alloc failed\n");
            return;
        }
    }
    if (m->menuFence == VK_NULL_HANDLE)
    {
        // SIGNALED: BindMenuRtt waits this fence BEFORE the first submit (peer of sceneFence). Created unsignaled it
        // would deadlock on the very first menu -- exactly "hangs the moment any menu is opened".
        VkFenceCreateInfo fi{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        fi.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        vkCreateFence(m->device, &fi, nullptr, &m->menuFence);
    }
    m->menuRttHandle[0] =
        (uint64_t)m
            ->menuColor; // [0]=image, [2]=view -- MenuRttTex / BlitTexToXrImage layout
    m->menuRttHandle[1] = (uint64_t)m->menuColorMem;
    m->menuRttHandle[2] = (uint64_t)m->menuColorView;
    m->menuRttHandle[3] = 0;
    m->menuRttW = w;
    m->menuRttH = h;
}

// Begin the menu pass on menuCmd. Object + screen draws route here while IsMenuActive() (see VulkanRenderer). NEGATIVE-
// height viewport like the scene, so the object path's D3D-convention matrices come out right-side up (the 2D screen
// path's flipY term keys on the menu pass to match). clear ignored: always cleared to transparent black (the quad
// blends over the world via BLEND_TEXTURE_SOURCE_ALPHA).
void VulkanBackend::BindMenuRtt(bool /*clear*/)
{
    if (!m->menuColor || !m->menuFbo || m->menuActive)
        return;
    vkWaitForFences(m->device, 1, &m->menuFence, VK_TRUE, UINT64_MAX);
    vkResetFences(m->device, 1, &m->menuFence);
    VkCommandBuffer c = m->menuCmd;
    vkResetCommandBuffer(c, 0);
    VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(c, &bi);
    VkClearValue clears[2];
    clears[0].color = {
        {0.0f, 0.0f, 0.0f,
         0.0f}}; // transparent black -> quad shows only what the menu draws
    clears[1].depthStencil = {0.0f, 0}; // reversed-Z (far = 0)
    VkRenderPassBeginInfo rpb{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    rpb.renderPass = m->menuPass;
    rpb.framebuffer = m->menuFbo;
    rpb.renderArea.extent = {(uint32_t)m->menuRttW, (uint32_t)m->menuRttH};
    rpb.clearValueCount = 2;
    rpb.pClearValues = clears;
    vkCmdBeginRenderPass(c, &rpb, VK_SUBPASS_CONTENTS_INLINE);
    VkViewport vp{
        0,   (float)m->menuRttH, (float)m->menuRttW, -(float)m->menuRttH, 0.0f,
        1.0f}; // negative height
    VkRect2D sc{{0, 0}, {(uint32_t)m->menuRttW, (uint32_t)m->menuRttH}};
    vkCmdSetViewport(c, 0, 1, &vp);
    vkCmdSetScissor(c, 0, 1, &sc);
    m->menuActive = true;
}

// End + submit the menu pass, wait it idle (the copy into the XR image is synchronous right after). Color left in
// SHADER_READ (pass finalLayout). Called via UnbindSceneRtt(menuTex) from otwloop -- see that override.
void VulkanBackend::EndMenuRtt()
{
    if (!m->menuActive)
        return;
    VkCommandBuffer c = m->menuCmd;
    vkCmdEndRenderPass(c);
    vkEndCommandBuffer(c);
    VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    si.commandBufferCount = 1;
    si.pCommandBuffers = &c;
    {
        std::lock_guard<std::mutex> _qlk(g_vkQueueMutex);
        vkQueueSubmit(m->queue, 1, &si, m->menuFence);
    }
    vkWaitForFences(m->device, 1, &m->menuFence, VK_TRUE, 1000000000ull);
    m->menuActive = false;
}

bool VulkanBackend::IsMenuActive() const
{
    return m && m->menuActive;
}
unsigned long long VulkanBackend::MenuRenderPass() const
{
    return m ? (unsigned long long)m->menuPass : 0ull;
}
void* VulkanBackend::MenuCommandBuffer() const
{
    return m ? (void*)m->menuCmd : nullptr;
}

void* VulkanBackend::MenuRttTex()
{
    return m->menuColor ? (void*)m->menuRttHandle : nullptr;
}

// ------------------------------------------------------------------------------------------- in-3D FPS quad RTT (#107)
// Owned scFormat color RTT for the head-locked FPS counter (2D green text on a transparent canvas). 2D only -> it uses
// the display-RTT path (BindSceneRtt/rttCmd), not the menu pass. Peer of D3D12Backend EnsureFpsRtt/BindFpsRtt/FpsRttTex.
void VulkanBackend::EnsureFpsRtt(int w, int h)
{
    if (!IsValid() || w <= 0 || h <= 0)
        return;
    if (m->fpsRttImage && m->fpsRttW == w && m->fpsRttH == h)
        return;
    if (m->fpsRttView)
    {
        auto it = m->rttFbCache.find(m->fpsRttView);
        if (it != m->rttFbCache.end())
        {
            vkDestroyFramebuffer(m->device, it->second, nullptr);
            m->rttFbCache.erase(it);
        }
        vkDestroyImageView(m->device, m->fpsRttView, nullptr);
        m->fpsRttView = VK_NULL_HANDLE;
    }
    if (m->fpsRttImage)
    {
        vkDestroyImage(m->device, m->fpsRttImage, nullptr);
        m->fpsRttImage = VK_NULL_HANDLE;
    }
    if (m->fpsRttMem)
    {
        FF_VmaFree((void*)m->fpsRttMem);
        m->fpsRttMem = VK_NULL_HANDLE;
    }
    m->fpsRttHandle[0] = m->fpsRttHandle[1] = m->fpsRttHandle[2] = 0;
    m->fpsRttW = m->fpsRttH = 0;

    VkImageCreateInfo ii{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    ii.imageType = VK_IMAGE_TYPE_2D;
    ii.format = m->scFormat;
    ii.extent = {(uint32_t)w, (uint32_t)h, 1};
    ii.mipLevels = 1;
    ii.arrayLayers = 1;
    ii.samples = VK_SAMPLE_COUNT_1_BIT;
    ii.tiling = VK_IMAGE_TILING_OPTIMAL;
    ii.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
               VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    ii.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    if (vkCreateImage(m->device, &ii, nullptr, &m->fpsRttImage) != VK_SUCCESS)
    {
        m->fpsRttImage = VK_NULL_HANDLE;
        return;
    }
    if (not FF_VmaAllocImageMemory(m->fpsRttImage, (void**)&m->fpsRttMem))
    {
        vkDestroyImage(m->device, m->fpsRttImage, nullptr);
        m->fpsRttImage = VK_NULL_HANDLE;
        return;
    }
    VkImageViewCreateInfo vi{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    vi.image = m->fpsRttImage;
    vi.viewType = VK_IMAGE_VIEW_TYPE_2D;
    vi.format = m->scFormat;
    vi.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    if (vkCreateImageView(m->device, &vi, nullptr, &m->fpsRttView) !=
        VK_SUCCESS)
    {
        FF_VmaFree((void*)m->fpsRttMem);
        m->fpsRttMem = VK_NULL_HANDLE;
        vkDestroyImage(m->device, m->fpsRttImage, nullptr);
        m->fpsRttImage = VK_NULL_HANDLE;
        return;
    }
    m->fpsRttHandle[0] = (uint64_t)m->fpsRttImage;
    m->fpsRttHandle[1] = (uint64_t)m->fpsRttMem;
    m->fpsRttHandle[2] = (uint64_t)m->fpsRttView;
    m->fpsRttHandle[3] = 0;
    m->fpsRttW = w;
    m->fpsRttH = h;
}

void VulkanBackend::BindFpsRtt(bool clear)
{
    if (!m->fpsRttImage)
        return;
    BindSceneRtt((void*)m->fpsRttHandle, m->fpsRttW, m->fpsRttH,
                 clear); // 2D text draws route through rttCmd
}

void* VulkanBackend::FpsRttTex()
{
    return m->fpsRttImage ? (void*)m->fpsRttHandle : nullptr;
}

// #59: subtitle-quad panel RTT -- clone of the FPS RTT trio above (owned scFormat color image, 2D text on a
// transparent canvas, drawn through the display-RTT path).
void VulkanBackend::EnsureSubRtt(int w, int h)
{
    if (!IsValid() || w <= 0 || h <= 0)
        return;
    if (m->subRttImage && m->subRttW == w && m->subRttH == h)
        return;
    if (m->subRttView)
    {
        auto it = m->rttFbCache.find(m->subRttView);
        if (it != m->rttFbCache.end())
        {
            vkDestroyFramebuffer(m->device, it->second, nullptr);
            m->rttFbCache.erase(it);
        }
        vkDestroyImageView(m->device, m->subRttView, nullptr);
        m->subRttView = VK_NULL_HANDLE;
    }
    if (m->subRttImage)
    {
        vkDestroyImage(m->device, m->subRttImage, nullptr);
        m->subRttImage = VK_NULL_HANDLE;
    }
    if (m->subRttMem)
    {
        FF_VmaFree((void*)m->subRttMem);
        m->subRttMem = VK_NULL_HANDLE;
    }
    m->subRttHandle[0] = m->subRttHandle[1] = m->subRttHandle[2] = 0;
    m->subRttW = m->subRttH = 0;

    VkImageCreateInfo ii{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    ii.imageType = VK_IMAGE_TYPE_2D;
    ii.format = m->scFormat;
    ii.extent = {(uint32_t)w, (uint32_t)h, 1};
    ii.mipLevels = 1;
    ii.arrayLayers = 1;
    ii.samples = VK_SAMPLE_COUNT_1_BIT;
    ii.tiling = VK_IMAGE_TILING_OPTIMAL;
    ii.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
               VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    ii.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    if (vkCreateImage(m->device, &ii, nullptr, &m->subRttImage) != VK_SUCCESS)
    {
        m->subRttImage = VK_NULL_HANDLE;
        return;
    }
    if (not FF_VmaAllocImageMemory(m->subRttImage, (void**)&m->subRttMem))
    {
        vkDestroyImage(m->device, m->subRttImage, nullptr);
        m->subRttImage = VK_NULL_HANDLE;
        return;
    }
    VkImageViewCreateInfo vi{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    vi.image = m->subRttImage;
    vi.viewType = VK_IMAGE_VIEW_TYPE_2D;
    vi.format = m->scFormat;
    vi.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    if (vkCreateImageView(m->device, &vi, nullptr, &m->subRttView) !=
        VK_SUCCESS)
    {
        FF_VmaFree((void*)m->subRttMem);
        m->subRttMem = VK_NULL_HANDLE;
        vkDestroyImage(m->device, m->subRttImage, nullptr);
        m->subRttImage = VK_NULL_HANDLE;
        return;
    }
    m->subRttHandle[0] = (uint64_t)m->subRttImage;
    m->subRttHandle[1] = (uint64_t)m->subRttMem;
    m->subRttHandle[2] = (uint64_t)m->subRttView;
    m->subRttHandle[3] = 0;
    m->subRttW = w;
    m->subRttH = h;
}

void VulkanBackend::BindSubRtt(bool clear)
{
    if (!m->subRttImage)
        return;
    BindSceneRtt((void*)m->subRttHandle, m->subRttW, m->subRttH, clear);
}

void* VulkanBackend::SubRttTex()
{
    return m->subRttImage ? (void*)m->subRttHandle : nullptr;
}

// Blit an owned RTT color image (SHADER_READ after UnbindSceneRtt) into an XR UI-swapchain VkImage. Single-image peer
// of BlitSceneToXrImages; the XR image is left in COLOR_ATTACHMENT_OPTIMAL (its release layout), own fence, synchronous.
bool VulkanBackend::BlitTexToXrImage(void* dstImage, void* srcTexHandle, int w,
                                     int h)
{
    if (!IsValid() || !dstImage || !srcTexHandle || w <= 0 || h <= 0)
        return false;
    VkImage src = (VkImage)((
        (const uint64_t*)
            srcTexHandle)[0]); // [0] = VkImage (see MenuRttTex layout)
    const VkImageView srcView =
        (VkImageView)(((const uint64_t*)srcTexHandle)[2]); // [2] = VkImageView
    VkImage dst = (VkImage)dstImage;
    if (!src)
        return false;
    const uint64_t kWaitNs = 1000000000ull;
    WaitRttConsumed(); // #107 PERF: the FPS quad's source is an RTT (BindFpsRtt/UnbindSceneRtt, batched) -> ensure it
    // finished before this blit reads it. (Menu source uses menuFence, not the RTT ring -> no-op.)

    if (m->vrBlitCmd == VK_NULL_HANDLE)
    {
        VkCommandBufferAllocateInfo ai{
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        ai.commandPool = m->vrBlitPool;
        ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        ai.commandBufferCount = 1;
        if (vkAllocateCommandBuffers(m->device, &ai, &m->vrBlitCmd) !=
            VK_SUCCESS)
        {
            VKB_LOG("BlitTexToXrImage: cmd alloc failed\n");
            return false;
        }
        VkFenceCreateInfo fi{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        vkCreateFence(m->device, &fi, nullptr, &m->vrBlitFence);
    }
        vkWaitForFences(m->device, 1, &m->vrBlitFence, VK_TRUE,
                    1000000000ull); // CopyRtt submits async -- cmd may still be in flight
    vkResetFences(m->device, 1, &m->vrBlitFence);
    VkCommandBuffer c = m->vrBlitCmd;
    vkResetCommandBuffer(c, 0);
    VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(c, &bi);

    auto barrier = [&](VkImage img, VkImageLayout oldL, VkImageLayout newL,
                       VkAccessFlags sa, VkAccessFlags da,
                       VkPipelineStageFlags ss, VkPipelineStageFlags ds)
    {
        VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        b.oldLayout = oldL;
        b.newLayout = newL;
        b.srcQueueFamilyIndex = b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        b.image = img;
        b.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        b.srcAccessMask = sa;
        b.dstAccessMask = da;
        vkCmdPipelineBarrier(c, ss, ds, 0, 0, nullptr, 0, nullptr, 1, &b);
    };

    // Artscout - 2026: same sRGB story as the eye path -- decode in the shader
    // so the XR image's own encode does not lift the panel a second time.
    bool viaShader = false;
    if (XrFormatIsSrgb(m->xrColorFormat) && srcView != VK_NULL_HANDLE)
        viaShader = RecordXrBlit(m, c, srcView, dst, w, h, 1.0f, 1.0f);

    if (!viaShader)
    {
        barrier(src, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_READ_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT);
        barrier(dst, VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0,
                VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT);

        VkImageBlit blit{};
        blit.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        blit.srcOffsets[1] = {w, h, 1};
        blit.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        blit.dstOffsets[1] = {w, h, 1};
        vkCmdBlitImage(c, src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst,
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit,
                       VK_FILTER_LINEAR);

        barrier(dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_ACCESS_TRANSFER_WRITE_BIT,
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
        barrier(src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    }
    vkEndCommandBuffer(c);

    VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    si.commandBufferCount = 1;
    si.pCommandBuffers = &c;
    {
        std::lock_guard<std::mutex> _qlk(g_vkQueueMutex);
        vkQueueSubmit(m->queue, 1, &si, m->vrBlitFence);
    }
    if (vkWaitForFences(m->device, 1, &m->vrBlitFence, VK_TRUE, kWaitNs) !=
        VK_SUCCESS)
    {
        static unsigned n = 0;
        if ((n++ % 64) == 0)
            VKB_LOG("BlitTexToXrImage: vrBlitFence TIMEOUT\n");
        return false;
    }
    return true;
}

// Artscout - 2026 (GM radar under Vulkan): GPU-copy one render-target texture into another (the Vulkan peer of
// D3D12Backend::CopyRtt / D3D11 CopyResource). The GM radar snapshots a completed sweep (the private 128x128 RTT)
// into a persistent panel texture on each beam reversal; DrawComposite then samples the panel textures. Both
// handles are VulkanTexture* ({image, memory, view, descriptor} -- read by offset like BindSceneRtt does). A blit
// (not a copy) so a size mismatch stretches instead of failing; formats are identical (both CreateRenderTarget).
// Synchronous one-shot submit, same as BlitTexToXrImage: this fires once per sweep reversal, not per frame.
bool VulkanBackend::CopyRtt(void* srcTexHandle, void* dstTexHandle, int srcW,
                            int srcH, int dstW, int dstH)
{
    if (!IsValid() || !srcTexHandle || !dstTexHandle || srcW <= 0 ||
        srcH <= 0 || dstW <= 0 || dstH <= 0)
        return false;
    VkImage src = (VkImage)(((const uint64_t*)srcTexHandle)[0]);
    VkImage dst = (VkImage)(((const uint64_t*)dstTexHandle)[0]);
    if (!src || !dst || src == dst)
        return false;
    const uint64_t kWaitNs = 1000000000ull;
    WaitRttConsumed(); // the sweep's last RTT pass must have finished writing the source

    if (m->vrBlitCmd == VK_NULL_HANDLE)
    {
        VkCommandBufferAllocateInfo ai{
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        ai.commandPool = m->vrBlitPool;
        ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        ai.commandBufferCount = 1;
        if (vkAllocateCommandBuffers(m->device, &ai, &m->vrBlitCmd) !=
            VK_SUCCESS)
        {
            VKB_LOG("CopyRtt: cmd alloc failed\n");
            return false;
        }
        VkFenceCreateInfo fi{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        fi.flags = VK_FENCE_CREATE_SIGNALED_BIT; // first top-wait must pass
        vkCreateFence(m->device, &fi, nullptr, &m->vrBlitFence);
    }
    // #107 PERF: wait the PREVIOUS snapshot's fence here (before reusing the cmd), NOT our own submit
    // below. The old submit+wait stalled the CPU for the whole queue depth on every sweep reversal --
    // the periodic 100-213ms VCock spikes (perceived 65fps). In-order queue: the blit lands before any
    // later pass that samples dst, so no wait is needed for correctness.
    vkWaitForFences(m->device, 1, &m->vrBlitFence, VK_TRUE, kWaitNs);
    vkResetFences(m->device, 1, &m->vrBlitFence);
    VkCommandBuffer c = m->vrBlitCmd;
    vkResetCommandBuffer(c, 0);
    VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(c, &bi);

    auto barrier = [&](VkImage img, VkImageLayout oldL, VkImageLayout newL,
                       VkAccessFlags sa, VkAccessFlags da,
                       VkPipelineStageFlags ss, VkPipelineStageFlags ds)
    {
        VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        b.oldLayout = oldL;
        b.newLayout = newL;
        b.srcQueueFamilyIndex = b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        b.image = img;
        b.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        b.srcAccessMask = sa;
        b.dstAccessMask = da;
        vkCmdPipelineBarrier(c, ss, ds, 0, 0, nullptr, 0, nullptr, 1, &b);
    };

    // src: SHADER_READ (UnbindSceneRtt's finalLayout) -> TRANSFER_SRC. dst: UNDEFINED -> TRANSFER_DST -- the copy
    // overwrites the whole image, so discarding whatever layout/content it had is correct even on first use.
    barrier(src, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_SHADER_READ_BIT,
            VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT);
    barrier(dst, VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0,
            VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT);

    VkImageBlit blit{};
    blit.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    blit.srcOffsets[1] = {srcW, srcH, 1};
    blit.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    blit.dstOffsets[1] = {dstW, dstH, 1};
    vkCmdBlitImage(c, src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit,
                   VK_FILTER_NEAREST);

    // dst is sampled by DrawComposite (descriptor path) -> SHADER_READ, not COLOR_ATTACHMENT.
    barrier(dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    barrier(src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    vkEndCommandBuffer(c);

    VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    si.commandBufferCount = 1;
    si.pCommandBuffers = &c;
    {
        std::lock_guard<std::mutex> _qlk(g_vkQueueMutex);
        vkQueueSubmit(m->queue, 1, &si, m->vrBlitFence);
    }
    // async: the fence is waited at the TOP of the next CopyRtt (cmd reuse guard), not here.
    return true;
}

void VulkanBackend::BindBackBufferRTV()
{
    if (m->rttActive)
    {
        UnbindSceneRtt(nullptr);
        return;
    } // leaving an RTT -> back to the backbuffer
    EnsureFrameStarted(m->clearArgb);
}

void VulkanBackend::EnsureFrameStarted(unsigned long argbClear)
{
    if (!m->recording)
        BeginFrame(argbClear);
}

bool VulkanBackend::IsRecording() const
{
    return m && m->recording;
}

void VulkanBackend::CompositeBitmap565(const void* pSrc565, int srcW, int srcH)
{
    // Artscout - 2026 (#104): UI 565 over the 3D frame. Parity with BlitBitmap565 for now (full replace); the
    // black=transparent composite over the live 3D is a later refinement (needs a blend blit pipeline).
    BlitBitmap565(pSrc565, srcW, srcH);
}

void VulkanBackend::ReadbackScene(int layer, void* dstRgba)
{
    if (!IsValid() || !m->sceneColor || !dstRgba || layer < 0 ||
        layer >= m->sceneViews)
        return;
    vkWaitForFences(m->device, 1, &m->sceneFence, VK_TRUE,
                    UINT64_MAX); // scene submit done
    const VkDeviceSize bytes = (VkDeviceSize)m->sceneW * m->sceneH * 4;
    VkBuffer buf = VK_NULL_HANDLE;
    VkDeviceMemory bufMem = VK_NULL_HANDLE;
    VkBufferCreateInfo bi{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bi.size = bytes;
    bi.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    bi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vkCreateBuffer(m->device, &bi, nullptr, &buf);
    void* bufMap = nullptr;
    if (not FF_VmaAllocReadbackMemory(buf, (void**)&bufMem, &bufMap))
    {
        vkDestroyBuffer(m->device, buf, nullptr);
        return;
    }

    VkCommandBuffer c = m->cmd[0];
    vkResetCommandBuffer(c, 0);
    VkCommandBufferBeginInfo cbi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    cbi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(c, &cbi);
    VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    b.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    b.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    b.srcQueueFamilyIndex = b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.image = m->sceneColor;
    b.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, (uint32_t)layer, 1};
    b.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    b.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    vkCmdPipelineBarrier(c, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                         nullptr, 1, &b);
    VkBufferImageCopy cp{};
    cp.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, (uint32_t)layer, 1};
    cp.imageExtent = {(uint32_t)m->sceneW, (uint32_t)m->sceneH, 1};
    vkCmdCopyImageToBuffer(c, m->sceneColor,
                           VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buf, 1, &cp);
    vkEndCommandBuffer(c);
    VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    si.commandBufferCount = 1;
    si.pCommandBuffers = &c;
    {
        std::lock_guard<std::mutex> _qlk(g_vkQueueMutex);
        vkQueueSubmit(m->queue, 1, &si, VK_NULL_HANDLE);
        vkQueueWaitIdle(m->queue);
    }
    memcpy(dstRgba, bufMap,
           (size_t)bytes); // scene color is R8G8B8A8 -> already RGBA
    vkDestroyBuffer(m->device, buf, nullptr);
    FF_VmaFree((void*)bufMem);
}

// Artscout - 2026 (#104): standalone GPU-name enumeration for the graphics-options "video card" selector on
// Linux, where DXGI does not exist. Creates a throwaway VkInstance (the options UI runs before the render
// backend is up), lists physical devices, and copies each deviceName. Mirrors the Windows DXGI adapter list
// (devmgr.cpp) so the selector index maps 1:1 to a real GPU. Returns the count written (<= maxN).
int VulkanEnumerateGpuNames(char (*out)[256], int maxN)
{
    if (!out || maxN <= 0)
        return 0;
    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.apiVersion = VK_API_VERSION_1_1;
    VkInstanceCreateInfo ici{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    ici.pApplicationInfo = &app;
    VkInstance inst = VK_NULL_HANDLE;
    if (vkCreateInstance(&ici, nullptr, &inst) != VK_SUCCESS || !inst)
        return 0;
    uint32_t n = 0;
    vkEnumeratePhysicalDevices(inst, &n, nullptr);
    if (n > (uint32_t)maxN)
        n = (uint32_t)maxN;
    std::vector<VkPhysicalDevice> devs(n);
    if (n)
        vkEnumeratePhysicalDevices(inst, &n, devs.data());
    int count = 0;
    for (uint32_t i = 0; i < n; ++i)
    {
        VkPhysicalDeviceProperties props{};
        vkGetPhysicalDeviceProperties(devs[i], &props);
        std::snprintf(out[count], 256, "%s", props.deviceName);
        out[count][255] = 0;
        ++count;
    }
    vkDestroyInstance(inst, nullptr);
    return count;
}
