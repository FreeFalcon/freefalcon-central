//-----------------------------------------------------------------------------
// OpenXRBackend.cpp -- see header. VR bring-up over the D3D11 device.
//
// Milestone 1: full OpenXR plumbing (instance/system/session/swapchains + the
// xrWaitFrame/Begin/End loop). RunFrame(NULL) clears both eyes and submits the
// projection layer -> validates the headset path end-to-end. Per-eye view/proj
// are computed and handed to the render callback for milestone 2.
//-----------------------------------------------------------------------------
#include <windows.h> // Windows: real; Linux (#108): the win32shim (CRITICAL_SECTION/HANDLE/HWND/POINT/RECT/OutputDebugString)
#include "common/wincursorcopy.h" // #VR: the live desktop cursor for the menu panel
#include <chrono> // #107 PERF: xrWaitFrame pacing timer
#include <thread> // #107 turbo: background xrWaitFrame gate
#include <mutex>
#include <condition_variable>
#ifdef _WIN32
#include <d3d11.h>
#include <d3d11_4.h> // ID3D11Multithread (context multithread protection)
#endif
#include <vector>
#include <math.h>
#include <stdio.h>
#include <stdarg.h>

// Artscout - 2026: TEMP diag -> OutputDebugString so it lands in the VS output log
// (MonoPrint goes to the mono card, not the debugger log). Remove with the diags.
static void XrDbg(const char* fmt, ...)
{
    char buf[256];
    va_list a;
    va_start(a, fmt);
    _vsnprintf(buf, sizeof(buf) - 1, fmt, a);
    va_end(a);
    buf[sizeof(buf) - 1] = 0;
    OutputDebugStringA(buf);
    // Artscout - 2026: also append to a file in the game cwd so OpenXR init/hand-tracking diagnostics are
    // visible WITHOUT a debugger (OutputDebugString needs DebugView). Same pattern as the vrray_diag dumps.
    FILE* f = fopen("openxr_diag.txt", "a");
    if (f)
    {
        fputs(buf, f);
        fclose(f);
    }
}

// OpenXR graphics bindings. On Windows all three (D3D11/D3D12/Vulkan) compile; on Linux (#108) only the Vulkan
// binding is active -- the D3D bindings and the Win32 platform define are Windows-only (the XrSwapchainImageVulkanKHR
// path is platform-neutral, so no XR_USE_PLATFORM_* is needed for the Vulkan session on Linux).
#ifdef _WIN32
#define XR_USE_PLATFORM_WIN32
#define XR_USE_GRAPHICS_API_D3D11
#define XR_USE_GRAPHICS_API_D3D12 // #DX12 п.5: dual-path VR (session bound to D3D11 or D3D12 by g_bUseD3D12)
#endif
#define XR_USE_GRAPHICS_API_VULKAN // #107: Vulkan VR path -- session bound to the Vulkan device (g_bUseVulkan). Always
// active: it is the ONLY graphics binding on the Linux build (#108).
#ifdef _WIN32
#include <d3d12.h>
#endif
#include <vulkan/vulkan.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#include "openxrbackend.h"
#ifdef _WIN32
#include "d3d12backend.h" // #DX12 п.5: g_pD3D12Backend (device + queue for the XR binding)
#endif
#include "../vulkan/vulkanbackend.h" // #107: g_pVulkanBackend (VkInstance/PhysicalDevice/Device for the XR binding)
#ifdef _WIN32
#include "d3d12/d3d12renderer.h" // #DX12 п.5: g_pD3D12Renderer (ViewInstancingAvailable for the VI decision)
#include "d3d12/d3d12texturemanager.h" // #DX12 п.5 A1: D3D12Texture (in-scene menu RTT copy source)
#endif
#include "../../sim/include/ivibedata.h" // g_intellivibeData.In3D (menu vs 3D world)

#ifndef _WIN32
// #108: on Linux only the Vulkan graphics binding compiles, so the D3D extension-name tokens (defined by
// <openxr_platform.h> only under XR_USE_GRAPHICS_API_D3D11/D3D12) are absent. useD3D11/useD3D12 are forced false at
// runtime here, so every D3D branch is dead -- but the tokens must still exist for those dead branches to COMPILE.
// Provide harmless placeholders; they are never passed to the runtime (the Vulkan branch is always taken).
#ifndef XR_KHR_D3D11_ENABLE_EXTENSION_NAME
#define XR_KHR_D3D11_ENABLE_EXTENSION_NAME "XR_KHR_D3D11_enable"
#endif
#ifndef XR_KHR_D3D12_ENABLE_EXTENSION_NAME
#define XR_KHR_D3D12_ENABLE_EXTENSION_NAME "XR_KHR_D3D12_enable"
#endif
#endif


OpenXRBackend* g_pOpenXRBackend = NULL;

// Menu UI surface cache (set by ImageBuffer::SwapBuffers, read by OpenXR_PumpFrame).
const void* g_pXrMenuSurface565 = NULL;
int g_xrMenuW = 0;
int g_xrMenuH = 0;

// Artscout - 2026 (VR menu): eliminate the cross-thread race on the menu 565 surface. The OLD design cached
// g_pXrMenuSurface565 = an ImageBuffer's m_pSysMem and let the pump (other thread) read it -> the buffer
// could be freed/resized mid-read (0xC0000005). Now the PRODUCER (PresentGpu, where m_pSysMem is valid)
// COPIES the surface into a stable buffer under a lock, and the pump reads a snapshot of THAT. No race.
static CRITICAL_SECTION s_xrMenuCS;
static bool s_xrMenuCSReady = false;
static unsigned char* s_xrMenuStable = NULL; // producer-filled, lock-protected
static long s_xrMenuStableCap = 0;
static unsigned char* s_xrMenuSnap =
    NULL; // pump-local snapshot (the convert reads this, no lock)
static long s_xrMenuSnapCap = 0;

static void XrMenuLockInit()
{
    if (!s_xrMenuCSReady)
    {
        InitializeCriticalSection(&s_xrMenuCS);
        s_xrMenuCSReady = true;
    }
}

// Producer (PresentGpu thread): copy the (valid-here) 565 surface into the stable buffer under the lock.
void OpenXR_CacheMenuSurface(const void* src565, int w, int h)
{
    if (!src565 || w <= 0 || h <= 0)
        return;
    XrMenuLockInit();
    const long need = (long)w * h * 2;
    EnterCriticalSection(&s_xrMenuCS);
    if (need > s_xrMenuStableCap)
    {
        unsigned char* n = (unsigned char*)realloc(s_xrMenuStable, need);
        if (n)
        {
            s_xrMenuStable = n;
            s_xrMenuStableCap = need;
        }
    }
    if (s_xrMenuStable && need <= s_xrMenuStableCap)
    {
        memcpy(s_xrMenuStable, src565,
               need); // src == this thread's own m_pSysMem -> safe to read here
        g_xrMenuW = w;
        g_xrMenuH = h;
        g_pXrMenuSurface565 =
            s_xrMenuStable; // non-null -> the pump has a menu surface to present
    }
    LeaveCriticalSection(&s_xrMenuCS);
}

// Consumer (pump): snapshot the stable buffer into a pump-local buffer (fast memcpy under the lock). The
// returned buffer is safe to read WITHOUT the lock (the producer never touches the snapshot buffer).
static const unsigned char* XrMenuSnapshot(int w, int h)
{
    if (!s_xrMenuCSReady || w <= 0 || h <= 0)
        return NULL;
    const long need = (long)w * h * 2;
    const unsigned char* out = NULL;
    EnterCriticalSection(&s_xrMenuCS);
    if (s_xrMenuStable && need > 0 && need <= s_xrMenuStableCap)
    {
        if (need > s_xrMenuSnapCap)
        {
            unsigned char* n = (unsigned char*)realloc(s_xrMenuSnap, need);
            if (n)
            {
                s_xrMenuSnap = n;
                s_xrMenuSnapCap = need;
            }
        }
        if (s_xrMenuSnap && need <= s_xrMenuSnapCap)
        {
            memcpy(s_xrMenuSnap, s_xrMenuStable, need);
            out = s_xrMenuSnap;
        }
    }
    LeaveCriticalSection(&s_xrMenuCS);
    return out;
}

//-----------------------------------------------------------------------------
// Error-check helper: log + bail. XR_FAILED is the canonical failure test.
//-----------------------------------------------------------------------------
#define XR_BAIL(call, what)                                                    \
    do                                                                         \
    {                                                                          \
        XrResult _r = (call);                                                  \
        if (XR_FAILED(_r))                                                     \
        {                                                                      \
            XrDbg("OpenXR: %s failed (XrResult %d)\n", what, (int)_r);         \
            return false;                                                      \
        }                                                                      \
    } while (0)

//-----------------------------------------------------------------------------
// Private state (PIMPL): all OpenXR + D3D11 types live here, out of the header.
//-----------------------------------------------------------------------------
struct OpenXRBackend::Impl
{
    XrInstance instance;
    XrSystemId systemId;
    XrSession session;
    XrSpace appSpace; // LOCAL reference space (app world origin, recentered)
    XrSpace
        localRef; // Artscout - 2026 (#67): PRISTINE LOCAL space, never recentered -- the fixed
    // measurement reference for recenter (so each recenter is absolute, not relative
    // to the already-shifted appSpace, which made the 2nd press toggle back).
    XrSpace viewSpace; // VIEW reference space (head), for the camera feed

    XrSessionState sessionState;
    bool sessionRunning;

    // #107 TURBO MODE (g_bXrTurboMode): a background thread absorbs the xrWaitFrame pacing BLOCK so the render
    // thread never stalls on the compositor gate (~2.2ms/frame measured). The spec allows wait(N+1) as soon as
    // begin(N) was called, so BeginStereoFrame requests the NEXT frame's wait right after xrBeginFrame and the
    // block overlaps the WHOLE frame (render + sim). ALL xrWaitFrame paths consume through XrWaitGated so the
    // wait<->begin 1:1 pairing survives path switches (3D loop <-> menu frames).
    std::thread xrWaitThread;
    std::mutex xrWaitMtx;
    std::condition_variable xrWaitCv;
    bool xrWaitPending = false; // the thread should run one xrWaitFrame
    bool xrWaitInFlight =
        false; // the thread is INSIDE xrWaitFrame right now (pending already cleared)
    bool xrWaitReady =
        false; // a completed pre-wait is available for consumption
    bool xrWaitExit = false;
    XrResult xrWaitRes = XR_SUCCESS;
    XrFrameState xrWaitState = {XR_TYPE_FRAME_STATE};
    int xrTurboWarmup =
        0; // successful stereo frames since (re)entry; pre-waits start after 5 --
    // the PVR compositor raced the very first turbo frames (xrBeginFrame
    // XR_ERROR_CALL_ORDER_INVALID at startup), so warm up synchronously first

    XrViewConfigurationType viewConfigType;
    std::vector<XrViewConfigurationView>
        configViews; // per-eye recommended size
    std::vector<XrView> views; // per-eye located pose+fov

    struct Swapchain
    {
        XrSwapchain handle;
        int32_t width;
        int32_t height;
#ifdef _WIN32
        std::vector<XrSwapchainImageD3D11KHR> images;
        std::vector<ID3D11RenderTargetView*> rtvs;
#endif
        // #107: parallel Vulkan swapchain images (VkImage). VR-Vulkan is always the per-view COPY model: the scene is
        // rendered once into VulkanBackend's N-layer multiview array, then each layer is blitted into these per-view
        // images (arraySize 1). No views/framebuffers needed here -- the blit targets the VkImage directly.
        std::vector<XrSwapchainImageVulkanKHR> imagesVk;
#ifdef _WIN32
        // #DX12 п.5: parallel D3D12 swapchain images + their RTV CPU handles (in Impl::rtvHeap12).
        std::vector<XrSwapchainImageD3D12KHR> images12;
#endif
        std::vector<unsigned __int64>
            rtvs12; // per-eye: TEXTURE2D RTV; VI: TEXTURE2DARRAY (all slices)
        // #DX12 п.5 view-instancing: when this is the single ARRAY swapchain (arraySize=N, 2 stereo / 4 quad),
        // rtvs12 above is the array RTV (all slices) for the single VI geometry pass; sliceRtvs12[v] are the
        // per-slice single-slice RTVs used by the per-view 2D overlay tail.
        std::vector<unsigned __int64> sliceRtvs12[4];
    };
    std::vector<Swapchain>
        swapchains; // one per eye (VI: a single 2-slice array swapchain in [0])

    int64_t swapchainFormat; // DXGI_FORMAT chosen for the eye color images

    // #DX12 п.5 view-instancing: single-pass stereo is active (STEREO + tier + DXC shaders). When true the
    // color swapchain is ONE 2-slice array (swapchains[0]) rendered in a single VI pass instead of N per-eye.
    bool viActive;
    bool viDiagFlag, viDiagStereo, viDiagTier,
        viDiagSh; // decision breakdown (for the runtime diag)
    bool viAcquired
        [2]; // per-group: swapchain image actually acquired this frame (release guard)

    // #DX12 п.5: D3D12 VR path (session bound to the D3D12 device+queue instead of D3D11). Selected by g_bUseD3D12.
    // Windows-only (#108): the D3D12 device/queue/command objects have no Linux equivalent -- the Vulkan path is used.
    bool useD3D12;
#ifdef _WIN32
    ID3D12Device* d3d12Device;
    ID3D12CommandQueue* d3d12Queue;
    ID3D12DescriptorHeap* rtvHeap12; // RTVs for all eye + UI swapchain images
    unsigned rtvInc12;
    unsigned rtvHead12;
    ID3D12CommandAllocator* alloc12; // for the eye clear / composite work
    ID3D12GraphicsCommandList* list12;
    ID3D12Fence* fence12;
    unsigned __int64 fenceVal12;
    HANDLE fenceEvt12;
#endif

    // Artscout - 2026 (#107): Vulkan VR path (session bound to the Vulkan device).
    // #109 enable2: the OpenXR runtime now WRAPS the creation of this VkInstance/VkDevice (xrCreateVulkanInstance/
    // DeviceKHR) and NAMES the physical device (xrGetVulkanGraphicsDevice2KHR) -- so the runtime injects its own
    // interop extensions and picks the HMD adapter. VulkanBackend still owns/destroys the handles; it just creates
    // them THROUGH the hooks below (installed in PreInitVulkan). These fields are copied from g_pVulkanBackend at Init.
    bool useVulkan;
    VkInstance vkInstance;
    VkPhysicalDevice vkPhysDevice;
    VkDevice vkDevice;
    uint32_t vkQueueFamily;
    uint32_t vkQueueIndex;
    // #109 enable2 entry points (resolved by name from p->instance in PreInitVulkan; extension functions).
    PFN_xrCreateVulkanInstanceKHR pfnCreateVulkanInstance;
    PFN_xrCreateVulkanDeviceKHR pfnCreateVulkanDevice;
    PFN_xrGetVulkanGraphicsDevice2KHR pfnGetVulkanGraphicsDevice2;
    PFN_xrGetVulkanGraphicsRequirements2KHR pfnGetVulkanGraphicsRequirements2;

    // Menu mode: a single quad-layer swapchain showing the flat 2D UI, plus a
    // CPU-writable staging texture for the 565->RGBA upload.
    XrSwapchain uiSwapchain;
    int uiW, uiH;
    int64_t uiFormat; // R8G8B8A8 (UNORM or _SRGB)
#ifdef _WIN32
    std::vector<XrSwapchainImageD3D11KHR> uiImages;
    ID3D11Texture2D* uiStaging;
    // #DX12 п.5: D3D12 UI swapchain images + an UPLOAD buffer for the 565->RGBA copy into them.
    std::vector<XrSwapchainImageD3D12KHR> uiImages12;
#endif
    std::vector<XrSwapchainImageVulkanKHR>
        uiImagesVk; // #107 VR-Vulkan: menu quad swapchain VkImages
#ifdef _WIN32
    ID3D12Resource* uiUpload12;
    unsigned uiRowPitch12;
    void*
        menuTexD3D12; // #DX12 п.5 A1: D3D12Texture* staged by SubmitInSceneMenuQuad, copied in EndStereoFrame
#endif

    // Artscout - 2026: #DX12 п.5 -- dedicated head-locked FPS quad (a small independent copy of the menu-quad path).
    XrSwapchain fpsSwapchain;
    int fpsW, fpsH;
    std::vector<XrSwapchainImageVulkanKHR>
        fpsImagesVk; // #107 VR-Vulkan: FPS quad swapchain VkImages (eager blit)
#ifdef _WIN32
    std::vector<XrSwapchainImageD3D12KHR> fpsImages12;
    void*
        fpsTexD3D12; // D3D12Texture* staged by SubmitFpsQuad, copied in EndStereoFrame
#endif
    bool fpsQuadPending;
    XrCompositionLayerQuad fpsQuad;
    // #59 subtitle quad (per Albert): clone of the FPS-quad plumbing (Vulkan path; D3D12 MV can adopt later)
    XrSwapchain subSwapchain = XR_NULL_HANDLE;
    std::vector<XrSwapchainImageVulkanKHR> subImagesVk;
    int subW = 0, subH = 0;
    XrCompositionLayerQuad subQuad;
    bool subQuadPending = false;

#ifdef _WIN32
    ID3D11Device* device;
    ID3D11DeviceContext* ctx;
#endif

    float nearZ, farZ;

    XrPosef lastHeadPose;
    bool haveHeadPose;

    float lastYaw, lastPitch,
        lastRoll; // HMD head angles (rad) for the cockpit camera
    bool haveHeadAngles;

    // Per-eye stereo frame state.
    bool inStereoFrame;
    XrFrameState stereoFrameState;
    int currentEye;
    std::vector<uint32_t> eyeImgIndex; // acquired swapchain image per eye
    std::vector<bool>
        eyeAcquired; // eye image acquired this frame (deferred release)
    std::vector<XrCompositionLayerProjectionView>
        projViews; // per-eye projection-layer views
    float eyeLatFeet[8]; // signed lateral offset per eye (feet)
    XrFovf submitFov; // engine fov submitted in the layer
    bool haveSubmitFov;

    // Artscout - 2026 (#59 VR menu): head-locked menu quad staged for THIS stereo frame (added by EndStereoFrame).
    bool menuQuadPending;
    XrCompositionLayerQuad menuQuad;

    // Artscout - 2026 (#67): recenter requested (any thread); applied by the render thread in BeginStereoFrame.
    bool recenterPending;

    // Artscout - 2026 (VR controllers, Phase 1): action-based input. Actions are declared once and bound per
    // interaction profile (touch/index/wmr/simple) -- the runtime maps them to whatever controller is present,
    // so we never enumerate per-vendor buttons. aim/grip are TRACKING POSES (not the squeeze button).
    XrActionSet actionSet;
    // aim/grip = tracking POSES (not buttons). squeeze = the grip BUTTON, used ONLY to pick the active hand
    // (whoever squeezes owns the ray); no grab mechanic. trigger = click, thumb = switches/knobs, a/b = zoom/recenter.
    XrAction aimAction, gripAction, squeezeAction, triggerAction, thumbAction,
        aBtnAction, bBtnAction;
    XrPath handPath[2]; // 0 = /user/hand/left, 1 = /user/hand/right
    XrSpace aimSpace[2], gripSpace[2];
    bool inputReady;
    int activeHand; // 0=left, 1=right; default right, switched by whoever squeezes grip
    struct HandInput
    {
        bool aimValid;
        XrPosef aimPose; // laser origin/direction (angled like a pointer)
        bool gripValid;
        XrPosef
            gripPose; // where the hand holds it (for the controller model, Phase 4)
        float trigger;
        bool triggerDown;
        float squeeze;
        bool squeezeDown; // grip button -> active-hand switch (rising edge)
        float thumbX, thumbY;
        bool aBtn, bBtn;
        bool
            prevBBtn; // Artscout - 2026: edge-detect for the ALWAYS-ON recenter (see SyncControllers)
    } hand[2];

    // Artscout - 2026 (VR hands): XR_EXT_hand_tracking. Index/knuckles synthesise a hand skeleton from the
    // controller's capacitive finger sensors (no camera module needed) -- the runtime returns 26 joint poses
    // per hand. Located every frame; if the runtime reports them not-active we fall back to the wireframe
    // controller. Extension entry points are resolved via xrGetInstanceProcAddr after the instance is created.
    bool handTrackingEnabled; // ext enabled on the instance
    PFN_xrCreateHandTrackerEXT pfnCreateHandTracker;
    PFN_xrDestroyHandTrackerEXT pfnDestroyHandTracker;
    PFN_xrLocateHandJointsEXT pfnLocateHandJoints;
    XrHandTrackerEXT handTracker[2];
    bool handJointsValid[2];
    XrHandJointLocationEXT handJoints[2][XR_HAND_JOINT_COUNT_EXT];
    // Artscout - 2026 (VR hand-tracking interaction): edge-detect for the PINCH click when a hand drives the ray
    // (Quest 3, no controllers). Pinch = thumb-tip to index-tip below a threshold; it stands in for the trigger.
    bool prevPinch[2];
    int pinchRun[2]; // consecutive frames at press strength (debounce -- see the pinch block)

    Impl()
        : instance(XR_NULL_HANDLE), systemId(XR_NULL_SYSTEM_ID),
          session(XR_NULL_HANDLE), appSpace(XR_NULL_HANDLE),
          localRef(XR_NULL_HANDLE), viewSpace(XR_NULL_HANDLE),
          sessionState(XR_SESSION_STATE_UNKNOWN), sessionRunning(false),
          viewConfigType(XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO),
          swapchainFormat(0), uiSwapchain(XR_NULL_HANDLE), uiW(0), uiH(0),
          uiFormat(0),
#ifdef _WIN32
          uiStaging(NULL),
#endif
          fpsSwapchain(XR_NULL_HANDLE), fpsW(0), fpsH(0),
#ifdef _WIN32
          fpsTexD3D12(NULL),
#endif
          fpsQuadPending(false),
#ifdef _WIN32
          device(NULL), ctx(NULL),
#endif
          nearZ(1.0f), farZ(80000.0f), haveHeadPose(false), lastYaw(0.0f),
          lastPitch(0.0f), lastRoll(0.0f), haveHeadAngles(false),
          inStereoFrame(false), currentEye(-1), haveSubmitFov(false),
          menuQuadPending(false), recenterPending(false),
          actionSet(XR_NULL_HANDLE), aimAction(XR_NULL_HANDLE),
          gripAction(XR_NULL_HANDLE), squeezeAction(XR_NULL_HANDLE),
          triggerAction(XR_NULL_HANDLE), thumbAction(XR_NULL_HANDLE),
          aBtnAction(XR_NULL_HANDLE), bBtnAction(XR_NULL_HANDLE),
          inputReady(false), activeHand(1), handTrackingEnabled(false),
          pfnCreateHandTracker(NULL), pfnDestroyHandTracker(NULL),
          pfnLocateHandJoints(NULL),
          // #DX12 п.5: D3D12 members must be zero-initialized (raw pointers/handles) -- otherwise
          // EnsureUiSwapchain's `if(p->uiUpload12) Release()` derefs garbage (0xFFFF... read AV).
          viActive(false), viDiagFlag(false), viDiagStereo(false),
          viDiagTier(false), viDiagSh(false), viAcquired(), useD3D12(false),
#ifdef _WIN32
          d3d12Device(NULL), d3d12Queue(NULL), rtvHeap12(NULL), rtvInc12(0),
          rtvHead12(0), alloc12(NULL), list12(NULL), fence12(NULL),
          fenceVal12(0), fenceEvt12(NULL),
#endif
          useVulkan(false), vkInstance(VK_NULL_HANDLE),
          vkPhysDevice(VK_NULL_HANDLE), vkDevice(VK_NULL_HANDLE),
          vkQueueFamily(0), vkQueueIndex(0), pfnCreateVulkanInstance(NULL),
          pfnCreateVulkanDevice(NULL), pfnGetVulkanGraphicsDevice2(NULL),
          pfnGetVulkanGraphicsRequirements2(NULL)
#ifdef _WIN32
          ,
          uiUpload12(NULL), uiRowPitch12(0), menuTexD3D12(NULL)
#endif
    {
        lastHeadPose.orientation.x = lastHeadPose.orientation.y =
            lastHeadPose.orientation.z = 0.0f;
        lastHeadPose.orientation.w = 1.0f;
        lastHeadPose.position.x = lastHeadPose.position.y =
            lastHeadPose.position.z = 0.0f;
        handPath[0] = handPath[1] = XR_NULL_PATH;
        aimSpace[0] = aimSpace[1] = gripSpace[0] = gripSpace[1] =
            XR_NULL_HANDLE;
        handTracker[0] = handTracker[1] = XR_NULL_HANDLE;
        handJointsValid[0] = handJointsValid[1] = false;
        prevPinch[0] = prevPinch[1] = false;
        pinchRun[0] = pinchRun[1] = 0;
        memset(hand, 0, sizeof(hand));
    }
};

//=============================================================================
// Matrix helpers. Output is 4x4 ROW-MAJOR in D3DXMATRIX layout (row-vector
// convention, v' = v * M), matching the engine's matrices.
//
// NOTE (milestone 2): OpenXR is right-handed (+Y up, -Z forward); the engine is
// left-handed (it flips RH->LH via Flip.m02 in render3d.cpp). The projection
// below is a D3D LH off-center frustum with z in [0,1]; the view is the rigid
// inverse of the eye pose. The RH->LH reconciliation must be verified against the
// live scene when the per-eye render is wired -- for milestone 1 (clear only)
// these are computed but not visually used.
//=============================================================================
static void XrProjectionToD3D(const XrFovf& fov, float zn, float zf,
                              float* m /*[16]*/)
{
    const float tanL = tanf(fov.angleLeft);
    const float tanR = tanf(fov.angleRight);
    const float tanU = tanf(fov.angleUp);
    const float tanD = tanf(fov.angleDown);
    const float w = tanR - tanL;
    const float h = tanU - tanD;

    for (int i = 0; i < 16; ++i)
        m[i] = 0.0f;
    m[0] = 2.0f / w; // m00
    m[5] = 2.0f / h; // m11
    m[8] = (tanR + tanL) / w; // m20 (x shear)
    m[9] = (tanU + tanD) / h; // m21 (y shear)
    m[10] = zf / (zf - zn); // m22
    m[11] = 1.0f; // m23
    m[14] = (zn * zf) / (zn - zf); // m32
}

// Quaternion (x,y,z,w) -> row-major 3x3 stored into the upper-left of m[16].
static void XrQuatToMat(const XrQuaternionf& q, float* m /*[16]*/)
{
    const float xx = q.x * q.x, yy = q.y * q.y, zz = q.z * q.z;
    const float xy = q.x * q.y, xz = q.x * q.z, yz = q.y * q.z;
    const float wx = q.w * q.x, wy = q.w * q.y, wz = q.w * q.z;

    m[0] = 1.0f - 2.0f * (yy + zz);
    m[1] = 2.0f * (xy + wz);
    m[2] = 2.0f * (xz - wy);
    m[3] = 0.0f;
    m[4] = 2.0f * (xy - wz);
    m[5] = 1.0f - 2.0f * (xx + zz);
    m[6] = 2.0f * (yz + wx);
    m[7] = 0.0f;
    m[8] = 2.0f * (xz + wy);
    m[9] = 2.0f * (yz - wx);
    m[10] = 1.0f - 2.0f * (xx + yy);
    m[11] = 0.0f;
    m[12] = 0.0f;
    m[13] = 0.0f;
    m[14] = 0.0f;
    m[15] = 1.0f;
}

// View matrix = inverse of the eye's rigid world transform (pose). Row-major.
static void XrPoseToView(const XrPosef& pose, float* m /*[16]*/)
{
    float R[16];
    XrQuatToMat(pose.orientation, R);

    // Inverse rotation is the transpose; inverse translation is -t * R^T.
    const float tx = pose.position.x, ty = pose.position.y,
                tz = pose.position.z;

    m[0] = R[0];
    m[1] = R[4];
    m[2] = R[8];
    m[3] = 0.0f;
    m[4] = R[1];
    m[5] = R[5];
    m[6] = R[9];
    m[7] = 0.0f;
    m[8] = R[2];
    m[9] = R[6];
    m[10] = R[10];
    m[11] = 0.0f;
    // -t * R^T  (R^T rows are R columns)
    m[12] = -(tx * R[0] + ty * R[1] + tz * R[2]);
    m[13] = -(tx * R[4] + ty * R[5] + tz * R[6]);
    m[14] = -(tx * R[8] + ty * R[9] + tz * R[10]);
    m[15] = 1.0f;
}

// HMD orientation (OpenXR quaternion, RH: +X right, +Y up, -Z forward) -> Falcon head
// look angles yaw/pitch/roll (radians). Derived from the rotated forward/up vectors to
// avoid Euler-order ambiguity. SIGN_* let us flip an axis without touching the math if a
// direction comes out inverted on the headset.
static void XrQuatToYawPitchRoll(const XrQuaternionf& q, float& yaw,
                                 float& pitch, float& roll)
{
    const float x = q.x, y = q.y, z = q.z, w = q.w;
    // forward = R * (0,0,-1)
    const float fx = -2.0f * (x * z + w * y);
    const float fy = 2.0f * (w * x - y * z);
    const float fz = 2.0f * (x * x + y * y) - 1.0f;
    // up = R * (0,1,0)
    const float ux = 2.0f * (x * y - w * z);
    const float uy = 1.0f - 2.0f * (x * x + z * z);

    float p = fy;
    if (p < -1.0f)
        p = -1.0f;
    else if (p > 1.0f)
        p = 1.0f;

    const float SIGN_YAW = 1.0f, SIGN_PITCH = -1.0f,
                SIGN_ROLL = -1.0f; // flip if inverted in HMD
    yaw = SIGN_YAW * atan2f(fx, -fz); // look right (+X) -> +yaw
    pitch = SIGN_PITCH * asinf(p); // look up (+Y)    -> +pitch
    roll = SIGN_ROLL * atan2f(ux, uy); // head tilt
}

//=============================================================================
// Artscout - 2026 (VR controllers, Phase 1): action-based input.
// Actions are semantic (aim/grip pose, squeeze, trigger, thumbstick, A, B); we suggest bindings for each
// interaction profile and the runtime maps them to the connected controller. No per-vendor enumeration.
//=============================================================================
static XrPath XrStr2Path(XrInstance inst, const char* s)
{
    XrPath path = XR_NULL_PATH;
    xrStringToPath(inst, s, &path);
    return path;
}

// Suggest one profile's bindings from parallel {action, path-string} arrays. Non-fatal per profile
// (a runtime may reject a profile it doesn't know; other profiles still apply).
static void SuggestProfile(XrInstance inst, const char* profile,
                           const XrAction* acts, const char* const* paths,
                           int count)
{
    std::vector<XrActionSuggestedBinding> binds;
    for (int i = 0; i < count; ++i)
    {
        XrPath bp = XrStr2Path(inst, paths[i]);
        if (bp == XR_NULL_PATH || acts[i] == XR_NULL_HANDLE)
            continue;
        XrActionSuggestedBinding b;
        b.action = acts[i];
        b.binding = bp;
        binds.push_back(b);
    }
    if (binds.empty())
        return;
    XrInteractionProfileSuggestedBinding sb = {
        XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
    sb.interactionProfile = XrStr2Path(inst, profile);
    sb.countSuggestedBindings = (uint32_t)binds.size();
    sb.suggestedBindings = binds.data();
    xrSuggestInteractionProfileBindings(inst, &sb);
}

bool OpenXRBackend::CreateInputActions()
{
    Impl* p = m_impl;
    p->inputReady = false;
    p->handPath[0] = XrStr2Path(p->instance, "/user/hand/left");
    p->handPath[1] = XrStr2Path(p->instance, "/user/hand/right");

    XrActionSetCreateInfo asci = {XR_TYPE_ACTION_SET_CREATE_INFO};
    strcpy(asci.actionSetName, "gameplay");
    strcpy(asci.localizedActionSetName, "Gameplay");
    asci.priority = 0;
    if (XR_FAILED(xrCreateActionSet(p->instance, &asci, &p->actionSet)))
        return false;

    struct ADef
    {
        XrAction* a;
        const char* name;
        const char* loc;
        XrActionType type;
    };
    ADef defs[] = {
        {&p->aimAction, "aim_pose", "Aim Pose", XR_ACTION_TYPE_POSE_INPUT},
        {&p->gripAction, "grip_pose", "Grip Pose", XR_ACTION_TYPE_POSE_INPUT},
        {&p->squeezeAction, "squeeze", "Grip Button",
         XR_ACTION_TYPE_FLOAT_INPUT},
        {&p->triggerAction, "trigger", "Trigger", XR_ACTION_TYPE_FLOAT_INPUT},
        {&p->thumbAction, "thumb", "Thumbstick", XR_ACTION_TYPE_VECTOR2F_INPUT},
        {&p->aBtnAction, "btn_a", "Button A", XR_ACTION_TYPE_BOOLEAN_INPUT},
        {&p->bBtnAction, "btn_b", "Button B", XR_ACTION_TYPE_BOOLEAN_INPUT},
    };
    for (int i = 0; i < (int)(sizeof(defs) / sizeof(defs[0])); ++i)
    {
        XrActionCreateInfo aci = {XR_TYPE_ACTION_CREATE_INFO};
        strcpy(aci.actionName, defs[i].name);
        strcpy(aci.localizedActionName, defs[i].loc);
        aci.actionType = defs[i].type;
        aci.countSubactionPaths = 2;
        aci.subactionPaths = p->handPath;
        if (XR_FAILED(xrCreateAction(p->actionSet, &aci, defs[i].a)))
            return false;
    }

    // Full-feature profiles share the same action order (aim,grip,squeeze,trigger,thumb,A,B x L/R).
    const XrAction A14[] = {
        p->aimAction,     p->aimAction,     p->gripAction,    p->gripAction,
        p->squeezeAction, p->squeezeAction, p->triggerAction, p->triggerAction,
        p->thumbAction,   p->thumbAction,   p->aBtnAction,    p->aBtnAction,
        p->bBtnAction,    p->bBtnAction};

    // Oculus Touch (Quest/Rift): A/B on the right hand, X/Y on the left.
    const char* const touch[] = {"/user/hand/left/input/aim/pose",
                                 "/user/hand/right/input/aim/pose",
                                 "/user/hand/left/input/grip/pose",
                                 "/user/hand/right/input/grip/pose",
                                 "/user/hand/left/input/squeeze/value",
                                 "/user/hand/right/input/squeeze/value",
                                 "/user/hand/left/input/trigger/value",
                                 "/user/hand/right/input/trigger/value",
                                 "/user/hand/left/input/thumbstick",
                                 "/user/hand/right/input/thumbstick",
                                 "/user/hand/left/input/x/click",
                                 "/user/hand/right/input/a/click",
                                 "/user/hand/left/input/y/click",
                                 "/user/hand/right/input/b/click"};
    SuggestProfile(p->instance, "/interaction_profiles/oculus/touch_controller",
                   A14, touch, 14);

    // Valve Index: a/b on both hands, analog squeeze force.
    const char* const index[] = {"/user/hand/left/input/aim/pose",
                                 "/user/hand/right/input/aim/pose",
                                 "/user/hand/left/input/grip/pose",
                                 "/user/hand/right/input/grip/pose",
                                 "/user/hand/left/input/squeeze/force",
                                 "/user/hand/right/input/squeeze/force",
                                 "/user/hand/left/input/trigger/value",
                                 "/user/hand/right/input/trigger/value",
                                 "/user/hand/left/input/thumbstick",
                                 "/user/hand/right/input/thumbstick",
                                 "/user/hand/left/input/a/click",
                                 "/user/hand/right/input/a/click",
                                 "/user/hand/left/input/b/click",
                                 "/user/hand/right/input/b/click"};
    SuggestProfile(p->instance, "/interaction_profiles/valve/index_controller",
                   A14, index, 14);

    // Microsoft WMR motion controller: no A/B face buttons -> A=menu, B=trackpad click; squeeze is a click.
    const char* const wmr[] = {"/user/hand/left/input/aim/pose",
                               "/user/hand/right/input/aim/pose",
                               "/user/hand/left/input/grip/pose",
                               "/user/hand/right/input/grip/pose",
                               "/user/hand/left/input/squeeze/click",
                               "/user/hand/right/input/squeeze/click",
                               "/user/hand/left/input/trigger/value",
                               "/user/hand/right/input/trigger/value",
                               "/user/hand/left/input/thumbstick",
                               "/user/hand/right/input/thumbstick",
                               "/user/hand/left/input/menu/click",
                               "/user/hand/right/input/menu/click",
                               "/user/hand/left/input/trackpad/click",
                               "/user/hand/right/input/trackpad/click"};
    SuggestProfile(p->instance,
                   "/interaction_profiles/microsoft/motion_controller", A14,
                   wmr, 14);

    // Khronos simple controller (fallback): only select + menu, no thumbstick/squeeze.
    const XrAction A8[] = {p->aimAction,  p->aimAction,     p->gripAction,
                           p->gripAction, p->triggerAction, p->triggerAction,
                           p->aBtnAction, p->aBtnAction};
    const char* const simple[] = {"/user/hand/left/input/aim/pose",
                                  "/user/hand/right/input/aim/pose",
                                  "/user/hand/left/input/grip/pose",
                                  "/user/hand/right/input/grip/pose",
                                  "/user/hand/left/input/select/click",
                                  "/user/hand/right/input/select/click",
                                  "/user/hand/left/input/menu/click",
                                  "/user/hand/right/input/menu/click"};
    SuggestProfile(p->instance, "/interaction_profiles/khr/simple_controller",
                   A8, simple, 8);

    // Action spaces (aim + grip per hand) -- need the session.
    for (int h = 0; h < 2; ++h)
    {
        XrActionSpaceCreateInfo sci = {XR_TYPE_ACTION_SPACE_CREATE_INFO};
        sci.poseInActionSpace.orientation.w = 1.0f;
        sci.subactionPath = p->handPath[h];
        sci.action = p->aimAction;
        xrCreateActionSpace(p->session, &sci, &p->aimSpace[h]);
        sci.action = p->gripAction;
        xrCreateActionSpace(p->session, &sci, &p->gripSpace[h]);
    }

    XrSessionActionSetsAttachInfo ai = {
        XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};
    ai.countActionSets = 1;
    ai.actionSets = &p->actionSet;
    if (XR_FAILED(xrAttachSessionActionSets(p->session, &ai)))
        return false;

    p->inputReady = true;
    return true;
}

// Artscout - 2026 (VR hand-tracking interaction): a quaternion that points OpenXR's aim -Z axis down `f` (a unit
// world direction), with world-up as the roll reference. Used to synthesise an aim pose from the hand skeleton so
// bare-hand pointing feeds the exact same ray the controller aim pose does. Standard look-rotation (right-handed).
static XrQuaternionf QuatFromForward(float fx, float fy, float fz)
{
    // forward = -Z, so the basis' -Z column is (fx,fy,fz) i.e. the +Z column is -f.
    float zx = -fx, zy = -fy, zz = -fz;
    // right = normalize(up x z); up hint = world up (0,1,0), fall back to (0,0,1) when nearly parallel.
    float ux = 0.0f, uy = 1.0f, uz = 0.0f;
    if (fabsf(fy) > 0.99f)
    {
        ux = 0.0f;
        uy = 0.0f;
        uz = 1.0f;
    }
    float rx = uy * zz - uz * zy, ry = uz * zx - ux * zz,
          rz = ux * zy - uy * zx;
    float rl = sqrtf(rx * rx + ry * ry + rz * rz);
    if (rl < 1e-5f)
        rl = 1.0f;
    rx /= rl;
    ry /= rl;
    rz /= rl;
    // up = z x right (re-orthogonalised)
    float vx = zy * rz - zz * ry, vy = zz * rx - zx * rz,
          vz = zx * ry - zy * rx;
    // rotation matrix columns (right, up, z) -> quaternion
    float m00 = rx, m01 = vx, m02 = zx, m10 = ry, m11 = vy, m12 = zy, m20 = rz,
          m21 = vz, m22 = zz;
    XrQuaternionf q;
    float tr = m00 + m11 + m22;
    if (tr > 0.0f)
    {
        float s = sqrtf(tr + 1.0f) * 2.0f;
        q.w = 0.25f * s;
        q.x = (m21 - m12) / s;
        q.y = (m02 - m20) / s;
        q.z = (m10 - m01) / s;
    }
    else if (m00 > m11 && m00 > m22)
    {
        float s = sqrtf(1.0f + m00 - m11 - m22) * 2.0f;
        q.w = (m21 - m12) / s;
        q.x = 0.25f * s;
        q.y = (m01 + m10) / s;
        q.z = (m02 + m20) / s;
    }
    else if (m11 > m22)
    {
        float s = sqrtf(1.0f + m11 - m00 - m22) * 2.0f;
        q.w = (m02 - m20) / s;
        q.x = (m01 + m10) / s;
        q.y = 0.25f * s;
        q.z = (m12 + m21) / s;
    }
    else
    {
        float s = sqrtf(1.0f + m22 - m00 - m11) * 2.0f;
        q.w = (m10 - m01) / s;
        q.x = (m02 + m20) / s;
        q.y = (m12 + m21) / s;
        q.z = 0.25f * s;
    }
    return q;
}

// Per-frame: sync actions, locate aim/grip poses, read states. Called from BeginStereoFrame after xrBeginFrame.
void OpenXRBackend::SyncControllers()
{
    Impl* p = m_impl;
    if (!p->inputReady || p->session == XR_NULL_HANDLE)
        return;
    XrActiveActionSet aas;
    aas.actionSet = p->actionSet;
    aas.subactionPath = XR_NULL_PATH;
    XrActionsSyncInfo si = {XR_TYPE_ACTIONS_SYNC_INFO};
    si.countActiveActionSets = 1;
    si.activeActionSets = &aas;
    if (XR_FAILED(xrSyncActions(p->session, &si)))
        return; // e.g. session not focused yet

    const XrTime t = p->stereoFrameState.predictedDisplayTime;
    for (int h = 0; h < 2; ++h)
    {
        OpenXRBackend::Impl::HandInput& hi = p->hand[h];

        XrSpaceLocation loc = {XR_TYPE_SPACE_LOCATION};
        hi.aimValid =
            (p->aimSpace[h] != XR_NULL_HANDLE &&
             XR_SUCCEEDED(
                 xrLocateSpace(p->aimSpace[h], p->appSpace, t, &loc)) &&
             (loc.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) &&
             (loc.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT));
        if (hi.aimValid)
            hi.aimPose = loc.pose;

        XrSpaceLocation gloc = {XR_TYPE_SPACE_LOCATION};
        hi.gripValid =
            (p->gripSpace[h] != XR_NULL_HANDLE &&
             XR_SUCCEEDED(
                 xrLocateSpace(p->gripSpace[h], p->appSpace, t, &gloc)) &&
             (gloc.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) &&
             (gloc.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT));
        if (hi.gripValid)
            hi.gripPose = gloc.pose;

        XrActionStateGetInfo gi = {XR_TYPE_ACTION_STATE_GET_INFO};
        gi.subactionPath = p->handPath[h];

        XrActionStateFloat sf = {XR_TYPE_ACTION_STATE_FLOAT};
        gi.action = p->triggerAction;
        hi.trigger =
            (XR_SUCCEEDED(xrGetActionStateFloat(p->session, &gi, &sf)) &&
             sf.isActive) ?
                sf.currentState :
                0.0f;
        hi.triggerDown = hi.trigger > 0.6f;

        gi.action = p->squeezeAction;
        float sq = (XR_SUCCEEDED(xrGetActionStateFloat(p->session, &gi, &sf)) &&
                    sf.isActive) ?
                       sf.currentState :
                       0.0f;
        bool sqDown = sq > 0.6f;
        if (sqDown && !hi.squeezeDown)
            p->activeHand = h; // rising edge -> this hand owns the ray
        hi.squeeze = sq;
        hi.squeezeDown = sqDown;

        XrActionStateVector2f sv = {XR_TYPE_ACTION_STATE_VECTOR2F};
        gi.action = p->thumbAction;
        if (XR_SUCCEEDED(xrGetActionStateVector2f(p->session, &gi, &sv)) &&
            sv.isActive)
        {
            hi.thumbX = sv.currentState.x;
            hi.thumbY = sv.currentState.y;
        }
        else
        {
            hi.thumbX = hi.thumbY = 0.0f;
        }

        XrActionStateBoolean sb = {XR_TYPE_ACTION_STATE_BOOLEAN};
        gi.action = p->aBtnAction;
        hi.aBtn =
            (XR_SUCCEEDED(xrGetActionStateBoolean(p->session, &gi, &sb)) &&
             sb.isActive) ?
                (sb.currentState != XR_FALSE) :
                false;
        gi.action = p->bBtnAction;
        hi.bBtn =
            (XR_SUCCEEDED(xrGetActionStateBoolean(p->session, &gi, &sb)) &&
             sb.isActive) ?
                (sb.currentState != XR_FALSE) :
                false;

        // Artscout - 2026: RECENTER on the B/Y rising edge -- here, and for BOTH hands, precisely because this
        // runs every VR frame regardless of what the hands are doing. It used to live inside vcock's ray/pick
        // block, which only executes while a hand is ACTIVE (grip held, or the Index capacitive toggle armed),
        // so you had to wake a hand up before you could re-centre. Recentering is a VIEW action, not a cockpit
        // interaction: it has no business being gated on the cursor/hand at all.
        //   Recenter() only raises recenterPending; the render thread applies it (it owns appSpace), so calling
        // it from here is safe and at worst costs one frame. Repeated calls are idempotent.
        if (hi.bBtn && !hi.prevBBtn)
            Recenter();
        hi.prevBBtn = hi.bBtn;
    }

    // Artscout - 2026 (VR hands): locate the 26 hand joints for each hand in the app (LOCAL) space, this
    // frame's predicted time. handJointsValid[h] gates the skeleton render; when false the caller falls back
    // to the wireframe controller. Index/knuckles feed this from the grip's capacitive finger sensors.
    if (p->handTrackingEnabled && p->pfnLocateHandJoints)
    {
        for (int h = 0; h < 2; ++h)
        {
            p->handJointsValid[h] = false;
            if (p->handTracker[h] == XR_NULL_HANDLE)
                continue;
            XrHandJointsLocateInfoEXT li = {
                XR_TYPE_HAND_JOINTS_LOCATE_INFO_EXT};
            li.baseSpace = p->appSpace;
            li.time = t;
            XrHandJointLocationsEXT locs = {XR_TYPE_HAND_JOINT_LOCATIONS_EXT};
            locs.jointCount = XR_HAND_JOINT_COUNT_EXT;
            locs.jointLocations = p->handJoints[h];
            XrResult lr = p->pfnLocateHandJoints(p->handTracker[h], &li, &locs);
            if (XR_SUCCEEDED(lr) && locs.isActive)
                p->handJointsValid[h] = true;
            // TEMP DIAG: once every ~2s per hand, report why hands may be falling back to the wireframe.
            {
                static int s_hn[2] = {0, 0};
                if ((s_hn[h]++ % 180) == 0)
                    XrDbg("OpenXR: locateHandJoints hand %d -> result=%d "
                          "isActive=%d valid=%d\n",
                          h, (int)lr, (int)locs.isActive,
                          (int)p->handJointsValid[h]);
            }
        }
    }

    // Artscout - 2026 (VR hand-tracking interaction, Quest 3): drive the SAME aim ray + trigger from the hand
    // skeleton when a hand has no controller aim but its joints are tracked. Downstream (VrRay, cockpit hit-test,
    // click) reads hi.aimPose / hi.trigger / activeHand and does not care whether a controller or a bare hand filled
    // them -- so bare-hand pointing and pinch-to-click just work with zero changes there.
    //   Ray    = origin at the index knuckle, direction wrist->index-tip (the natural pointing gesture).
    //   Click  = PINCH: thumb-tip to index-tip distance; strength ramps 1..0 across 2cm..4cm; >0.6 == down.
    //   Active = whichever hand pinches (rising edge), mirroring the controller's grip-owns-the-ray rule.
    for (int h = 0; h < 2; ++h)
    {
        OpenXRBackend::Impl::HandInput& hi = p->hand[h];

        // TEMP [HANDDUMP] (g_bVrHandDump): dump BEFORE the controller-wins gate below, so it fires even
        // when the runtime also gives the hand an aim pose (SteamVR/Quest often emulate hand-as-controller
        // -> aimValid true -> the whole skeletal block would otherwise be skipped and print nothing). Shows
        // per-finger extension (tip-to-proximal; straight ~7-9cm, curled collapses) + the pinch distances +
        // aimValid, so we can tell a runtime-inferred clench from our math AND see if the controller path
        // is stealing the click.
        extern bool g_bVrHandDump;
        if (g_bVrHandDump && p->handJointsValid[h])
        {
            const XrHandJointLocationEXT* JD = p->handJoints[h];
            const unsigned pB = XR_SPACE_LOCATION_POSITION_VALID_BIT;
            auto dist = [&](int a, int b) -> float
            {
                if (!((JD[a].locationFlags & pB) && (JD[b].locationFlags & pB)))
                    return -1.0f;
                const XrVector3f pa = JD[a].pose.position;
                const XrVector3f pb = JD[b].pose.position;
                float ex = pa.x - pb.x, ey = pa.y - pb.y, ez = pa.z - pb.z;
                return sqrtf(ex * ex + ey * ey + ez * ez) * 100.0f; // cm
            };
            static unsigned s_hd = 0;
            if ((s_hd++ % 120u) < 2u) // ~every 2s, both hands
            {
                char b[256];
                snprintf(
                    b, sizeof(b),
                    "[HANDDUMP] %s aim=%d ext(cm) idx=%.1f mid=%.1f rng=%.1f "
                    "lit=%.1f thb=%.1f | thumb->idx=%.1f thumb->mid=%.1f\n",
                    h == 0 ? "L" : "R", hi.aimValid ? 1 : 0,
                    dist(XR_HAND_JOINT_INDEX_TIP_EXT,
                         XR_HAND_JOINT_INDEX_PROXIMAL_EXT),
                    dist(XR_HAND_JOINT_MIDDLE_TIP_EXT,
                         XR_HAND_JOINT_MIDDLE_PROXIMAL_EXT),
                    dist(XR_HAND_JOINT_RING_TIP_EXT,
                         XR_HAND_JOINT_RING_PROXIMAL_EXT),
                    dist(XR_HAND_JOINT_LITTLE_TIP_EXT,
                         XR_HAND_JOINT_LITTLE_PROXIMAL_EXT),
                    dist(XR_HAND_JOINT_THUMB_TIP_EXT,
                         XR_HAND_JOINT_THUMB_PROXIMAL_EXT),
                    dist(XR_HAND_JOINT_THUMB_TIP_EXT,
                         XR_HAND_JOINT_INDEX_TIP_EXT),
                    dist(XR_HAND_JOINT_THUMB_TIP_EXT,
                         XR_HAND_JOINT_MIDDLE_TIP_EXT));
                OutputDebugStringA(b);
                fprintf(stderr, "%s", b);
            }
        }

        // Run OUR validated pinch for every hand-tracked hand, even when the runtime also gives an aim
        // pose. The [HANDDUMP] data proved aimValid is ALWAYS 1 on Quest 3 bare-hand tracking -- so the
        // old `if (hi.aimValid) continue` left the CLICK to the runtime's system-pinch gesture (the one
        // that false-fires) and our pose-gated pinch was dead code. Now: keep the runtime's RAY when it
        // has one (better filtered), but OVERRIDE the click below with our thumb->index pinch + pose gate.
        // Only a hand with no tracked joints falls through to the controller path.
        if (!p->handJointsValid[h])
        {
            p->prevPinch[h] = false;
            continue;
        }
        const XrHandJointLocationEXT* J = p->handJoints[h];
        const unsigned posBit = XR_SPACE_LOCATION_POSITION_VALID_BIT;
        if (!(J[XR_HAND_JOINT_INDEX_TIP_EXT].locationFlags & posBit) ||
            !(J[XR_HAND_JOINT_WRIST_EXT].locationFlags & posBit) ||
            !(J[XR_HAND_JOINT_THUMB_TIP_EXT].locationFlags & posBit))
        {
            p->prevPinch[h] = false;
            continue;
        }

        const XrVector3f tip = J[XR_HAND_JOINT_INDEX_TIP_EXT].pose.position;
        const XrVector3f wr = J[XR_HAND_JOINT_WRIST_EXT].pose.position;
        const XrVector3f base =
            (J[XR_HAND_JOINT_INDEX_PROXIMAL_EXT].locationFlags & posBit) ?
                J[XR_HAND_JOINT_INDEX_PROXIMAL_EXT].pose.position :
                wr; // ray origin near the knuckle
        float dx = tip.x - wr.x, dy = tip.y - wr.y, dz = tip.z - wr.z;
        float dl = sqrtf(dx * dx + dy * dy + dz * dz);
        if (dl < 1e-4f)
        {
            p->prevPinch[h] = false;
            continue;
        }
        dx /= dl;
        dy /= dl;
        dz /= dl;

        // Only synthesize a ray if the runtime did NOT give one this frame; otherwise keep its aimPose.
        if (!hi.aimValid)
        {
            hi.aimPose.position = base;
            hi.aimPose.orientation = QuatFromForward(dx, dy, dz);
            hi.aimValid = true;
        }

        // PINCH -> trigger. thumb-tip to index-tip distance, CALIBRATED from [HANDDUMP]: a deliberate
        // pinch touches at ~2.9cm (fingertip thickness -> the runtime reports tip CENTRES ~3cm apart even
        // when the pads meet), a relaxed hand sits at 7-12cm. Ramp 1.0 at 2.5cm, 0.0 at 5.0cm. (The old
        // 1.2cm was unreachable -> no clicks.)
        const XrVector3f th = J[XR_HAND_JOINT_THUMB_TIP_EXT].pose.position;
        float px = tip.x - th.x, py = tip.y - th.y, pz = tip.z - th.z;
        float pd = sqrtf(px * px + py * py + pz * pz);
        float pinch = 1.0f - (pd - 0.025f) / 0.025f;
        if (pinch < 0.0f)
            pinch = 0.0f;
        else if (pinch > 1.0f)
            pinch = 1.0f;
        hi.trigger = pinch;

        // Pose gate: a REAL pinch touches thumb+index SPECIFICALLY while the middle finger stays out; an
        // inferred clench / fist brings the thumb near ALL the fingertips at once. So require the thumb to
        // be clearly FARTHER from the middle tip than from the index tip. This is what tells a deliberate
        // two-finger pinch from the runtime's occlusion-clench guess, which hysteresis alone cannot.
        bool poseOk = true;
        if (J[XR_HAND_JOINT_MIDDLE_TIP_EXT].locationFlags & posBit)
        {
            const XrVector3f mt = J[XR_HAND_JOINT_MIDDLE_TIP_EXT].pose.position;
            float mx = mt.x - th.x, my = mt.y - th.y, mz = mt.z - th.z;
            float md = sqrtf(mx * mx + my * my + mz * mz);
            poseOk = (md > pd * 1.6f); // middle clearly out -> not a fist
        }

        // Hysteresis + debounce on top of the pose gate. Press only after 3 consecutive frames with
        // thumb-index <= ~3.5cm (pinch >= 0.6) AND the pose gate satisfied; release at >= ~4.5cm
        // (pinch <= 0.2). The gap + the run swallow jitter; the pose gate rejects the inferred clench.
        bool down = p->prevPinch[h];
        if (!down)
        {
            if (pinch >= 0.6f && poseOk)
            {
                if (++p->pinchRun[h] >= 3)
                    down = true;
            }
            else
                p->pinchRun[h] = 0;
        }
        else
        {
            if (pinch <= 0.2f)
                down = false;
            p->pinchRun[h] = 0;
        }
        hi.triggerDown = down;
        if (hi.triggerDown && !p->prevPinch[h])
            p->activeHand = h; // pinch owns the ray (rising edge)
        p->prevPinch[h] = hi.triggerDown;
    }
}

//=============================================================================
// Construction
//=============================================================================
OpenXRBackend::OpenXRBackend() : m_impl(new Impl())
{
}
OpenXRBackend::~OpenXRBackend()
{
    Shutdown();
    delete m_impl;
    m_impl = NULL;
}

bool OpenXRBackend::IsInitialized() const
{
    return m_impl && m_impl->instance != XR_NULL_HANDLE;
}
// #107 VR-Vulkan: the session exists (full Init done). Distinct from IsInitialized (= instance exists), which is
// already true after PreInitVulkan -- devmgr gates the one-time session bring-up on THIS so a re-init (menu->3D,
// same device) does not create a second session (the runtime allows only one -> XR_ERROR_LIMIT_REACHED).
bool OpenXRBackend::IsSessionCreated() const
{
    return m_impl && m_impl->session != XR_NULL_HANDLE;
}
bool OpenXRBackend::IsSessionRunning() const
{
    return m_impl && m_impl->sessionRunning;
}
bool OpenXRBackend::IsQuadViews() const
{
    return m_impl && m_impl->viewConfigType ==
                         XR_VIEW_CONFIGURATION_TYPE_PRIMARY_QUAD_VARJO;
}
void OpenXRBackend::SetClipPlanes(float nearZ, float farZ)
{
    m_impl->nearZ = nearZ;
    m_impl->farZ = farZ;
}
// #107 VR-Vulkan: per-view XR swapchain render size (the multiview array is sized to it).
bool OpenXRBackend::GetEyeRenderSize(int e, int* w, int* h) const
{
    Impl* p = m_impl;
    if (!p || e < 0 || e >= (int)p->swapchains.size())
        return false;
    if (w)
        *w = p->swapchains[e].width;
    if (h)
        *h = p->swapchains[e].height;
    return true;
}

//=============================================================================
// PreInitVulkan -- #107 VR-Vulkan Phase 1.5
// Create the SINGLE XrInstance + system (the full extension set, so hand tracking /
// render models survive), then query which Vulkan instance/device extensions the app's
// VkInstance/VkDevice must carry (compositor image sharing). Runs BEFORE the Vulkan
// device is created; Init later REUSES this instance (the SteamVR loader forbids two
// simultaneous instances -- a throwaway query instance tripped XR_ERROR_LIMIT_REACHED).
//=============================================================================
// #109 enable2 hooks -- installed on VulkanBackend in PreInitVulkan. VulkanBackend::Init calls these (as void*-typed
// function pointers, keeping ffvulkan free of OpenXR headers) so the OpenXR runtime WRAPS vkCreateInstance/vkCreateDevice
// (adding its interop extensions) and NAMES the physical device. Each casts the void* args back to Vulkan types and
// forwards to the resolved xr* entry point; user = OpenXRBackend::Impl*. Returns a VkResult as int (0 == VK_SUCCESS).
int OpenXRBackend::XrVkCreateInstanceHook(const void* ci, void* outInstance,
                                          void* user)
{
    OpenXRBackend::Impl* p = (OpenXRBackend::Impl*)user;
    if (!p || !p->pfnCreateVulkanInstance)
        return VK_ERROR_INITIALIZATION_FAILED;
    XrVulkanInstanceCreateInfoKHR xci = {
        XR_TYPE_VULKAN_INSTANCE_CREATE_INFO_KHR};
    xci.systemId = p->systemId;
    xci.pfnGetInstanceProcAddr = vkGetInstanceProcAddr;
    xci.vulkanCreateInfo = (const VkInstanceCreateInfo*)ci;
    VkResult vr = VK_ERROR_INITIALIZATION_FAILED;
    XrResult xr = p->pfnCreateVulkanInstance(p->instance, &xci,
                                             (VkInstance*)outInstance, &vr);
    if (XR_FAILED(xr))
    {
        XrDbg("OpenXR: xrCreateVulkanInstanceKHR failed (%d)\n", (int)xr);
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    return (int)vr;
}

int OpenXRBackend::XrVkPickPhysicalHook(void* instance, void* outPhysical,
                                        void* user)
{
    OpenXRBackend::Impl* p = (OpenXRBackend::Impl*)user;
    if (!p || !p->pfnGetVulkanGraphicsDevice2)
        return VK_ERROR_INITIALIZATION_FAILED;
    XrVulkanGraphicsDeviceGetInfoKHR gi = {
        XR_TYPE_VULKAN_GRAPHICS_DEVICE_GET_INFO_KHR};
    gi.systemId = p->systemId;
    gi.vulkanInstance = (VkInstance)instance;
    XrResult xr = p->pfnGetVulkanGraphicsDevice2(
        p->instance, &gi, (VkPhysicalDevice*)outPhysical);
    if (XR_FAILED(xr))
    {
        XrDbg("OpenXR: xrGetVulkanGraphicsDevice2KHR failed (%d)\n", (int)xr);
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    return (int)VK_SUCCESS;
}

int OpenXRBackend::XrVkCreateDeviceHook(void* physical, const void* ci,
                                        void* outDevice, void* user)
{
    OpenXRBackend::Impl* p = (OpenXRBackend::Impl*)user;
    if (!p || !p->pfnCreateVulkanDevice)
        return VK_ERROR_INITIALIZATION_FAILED;
    XrVulkanDeviceCreateInfoKHR xci = {XR_TYPE_VULKAN_DEVICE_CREATE_INFO_KHR};
    xci.systemId = p->systemId;
    xci.pfnGetInstanceProcAddr = vkGetInstanceProcAddr;
    xci.vulkanPhysicalDevice = (VkPhysicalDevice)physical;
    xci.vulkanCreateInfo = (const VkDeviceCreateInfo*)ci;
    VkResult vr = VK_ERROR_INITIALIZATION_FAILED;
    XrResult xr =
        p->pfnCreateVulkanDevice(p->instance, &xci, (VkDevice*)outDevice, &vr);
    if (XR_FAILED(xr))
    {
        XrDbg("OpenXR: xrCreateVulkanDeviceKHR failed (%d)\n", (int)xr);
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    return (int)vr;
}

bool OpenXRBackend::PreInitVulkan(std::string& instExtsOut,
                                  std::string& devExtsOut)
{
    Impl* p = m_impl;
    // #109 enable2: the runtime injects its own interop extensions when it wraps vkCreate* -- no manual ext list needed
    // (this supersedes the v1 xrGetVulkan{Instance,Device}ExtensionsKHR query). The out params stay empty.
    instExtsOut.clear();
    devExtsOut.clear();

    p->useVulkan = true;
    p->useD3D12 = false;
    if (p->instance == XR_NULL_HANDLE && !CreateInstanceAndSystem())
    {
        XrDbg("OpenXR: PreInitVulkan - CreateInstanceAndSystem failed\n");
        return false;
    }

    // Resolve the enable2 entry points (extension functions -> xrGetInstanceProcAddr).
    xrGetInstanceProcAddr(p->instance, "xrCreateVulkanInstanceKHR",
                          (PFN_xrVoidFunction*)&p->pfnCreateVulkanInstance);
    xrGetInstanceProcAddr(p->instance, "xrCreateVulkanDeviceKHR",
                          (PFN_xrVoidFunction*)&p->pfnCreateVulkanDevice);
    xrGetInstanceProcAddr(p->instance, "xrGetVulkanGraphicsDevice2KHR",
                          (PFN_xrVoidFunction*)&p->pfnGetVulkanGraphicsDevice2);
    xrGetInstanceProcAddr(
        p->instance, "xrGetVulkanGraphicsRequirements2KHR",
        (PFN_xrVoidFunction*)&p->pfnGetVulkanGraphicsRequirements2);
    if (!p->pfnCreateVulkanInstance || !p->pfnCreateVulkanDevice ||
        !p->pfnGetVulkanGraphicsDevice2 ||
        !p->pfnGetVulkanGraphicsRequirements2)
    {
        XrDbg("OpenXR: PreInitVulkan - enable2 entry points unavailable "
              "(inst=%p dev=%p gpu2=%p req2=%p)\n",
              (void*)p->pfnCreateVulkanInstance,
              (void*)p->pfnCreateVulkanDevice,
              (void*)p->pfnGetVulkanGraphicsDevice2,
              (void*)p->pfnGetVulkanGraphicsRequirements2);
        return false;
    }

    // The spec requires xrGetVulkanGraphicsRequirements2KHR to be called BEFORE xrCreateVulkanInstanceKHR (i.e. before
    // the hooks run in VulkanBackend::Init).
    XrGraphicsRequirementsVulkanKHR reqs = {
        XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN_KHR};
    XrResult rr =
        p->pfnGetVulkanGraphicsRequirements2(p->instance, p->systemId, &reqs);
    if (XR_FAILED(rr))
    {
        XrDbg("OpenXR: xrGetVulkanGraphicsRequirements2KHR failed (%d)\n",
              (int)rr);
        return false;
    }
    XrDbg("OpenXR: Vulkan API range min=0x%llx max=0x%llx\n",
          (unsigned long long)reqs.minApiVersionSupported,
          (unsigned long long)reqs.maxApiVersionSupported);

    // Install the bring-up hooks: VulkanBackend::Init now creates the VkInstance/VkDevice THROUGH the runtime and lets
    // it name the GPU. VulkanBackend still owns/destroys those handles.
    if (!g_pVulkanBackend)
    {
        XrDbg("OpenXR: PreInitVulkan - g_pVulkanBackend NULL, cannot install "
              "enable2 hooks\n");
        return false;
    }
    g_pVulkanBackend->SetVulkanCreationHooks(
        XrVkCreateInstanceHook, XrVkPickPhysicalHook, XrVkCreateDeviceHook, p);
    return true;
}

//=============================================================================
// CreateInstanceAndSystem -- #107: instance + system + view-config (steps 1-3b of Init). Extracted so the
// Vulkan pre-init (PreInitVulkan) can create the SINGLE XrInstance BEFORE the Vulkan device (the SteamVR
// loader forbids two simultaneous instances). Init reuses it when it already exists; otherwise it creates it
// (the D3D paths). Uses p->useVulkan/useD3D12 (set by the caller before this).
//=============================================================================
bool OpenXRBackend::CreateInstanceAndSystem()
{
    Impl* p = m_impl;
    // --- 1. Require the D3D11 graphics extension -----------------------------
    uint32_t extCount = 0;
    if (XR_FAILED(
            xrEnumerateInstanceExtensionProperties(NULL, 0, &extCount, NULL)))
    {
        XrDbg("OpenXR: no runtime / xrEnumerateInstanceExtensionProperties "
              "failed\n");
        return false;
    }
    std::vector<XrExtensionProperties> exts(extCount);
    for (uint32_t i = 0; i < extCount; ++i)
    {
        exts[i].type = XR_TYPE_EXTENSION_PROPERTIES;
        exts[i].next = NULL;
    }
    xrEnumerateInstanceExtensionProperties(NULL, extCount, &extCount,
                                           exts.empty() ? NULL : exts.data());

    bool haveD3D11 = false, haveD3D12 = false, haveVulkan = false,
         haveQuadViews = false, haveEyeGaze = false, haveHandTracking = false;
    bool haveCtrlModelMSFT = false, haveRenderModelEXT = false,
         haveInteractionRenderModelEXT = false, haveUuid = false;
    for (uint32_t i = 0; i < extCount; ++i)
    {
        const char* n = exts[i].extensionName;
        if (strcmp(n, XR_KHR_VULKAN_ENABLE2_EXTENSION_NAME) == 0)
            haveVulkan = true; // #107/#109 enable2
        else if (strcmp(n, XR_KHR_D3D12_ENABLE_EXTENSION_NAME) == 0)
            haveD3D12 = true; // #DX12 п.5
        else if (strcmp(n, XR_KHR_D3D11_ENABLE_EXTENSION_NAME) == 0)
            haveD3D11 = true;
        else if (strcmp(n, XR_VARJO_QUAD_VIEWS_EXTENSION_NAME) == 0)
            haveQuadViews = true;
        else if (strcmp(n, XR_EXT_EYE_GAZE_INTERACTION_EXTENSION_NAME) == 0)
            haveEyeGaze = true;
        else if (strcmp(n, XR_EXT_HAND_TRACKING_EXTENSION_NAME) == 0)
            haveHandTracking = true; // real hands
        // Artscout - 2026 (VR controllers): probe for runtime-provided controller glTF model extensions. String
        // literals (not SDK macros) so it builds on older openxr headers. Any of these lets us fetch the REAL
        // controller mesh (glTF); EXT_render_model is the cross-vendor successor SteamVR/Valve is likelier to
        // expose than the MSFT one. interaction_render_model pairs models with our action-based bindings.
        else if (strcmp(n, "XR_MSFT_controller_model") == 0)
            haveCtrlModelMSFT = true;
        else if (strcmp(n, "XR_EXT_render_model") == 0)
            haveRenderModelEXT = true;
        else if (strcmp(n, "XR_EXT_interaction_render_model") == 0)
            haveInteractionRenderModelEXT = true;
        else if (strcmp(n, "XR_EXT_uuid") == 0)
            haveUuid = true; // #107: render_model DEPENDS on it
    }
    // Dump every advertised extension once so we can see exactly what THIS runtime (SteamVR/PVR/WMR) offers.
    XrDbg("OpenXR: %u instance extensions advertised by the runtime:\n",
          extCount);
    for (uint32_t i = 0; i < extCount; ++i)
        XrDbg("OpenXR:   ext[%u] = %s\n", i, exts[i].extensionName);
    XrDbg("OpenXR: controller-model extensions:  MSFT_controller_model=%d  "
          "EXT_render_model=%d  EXT_interaction_render_model=%d\n",
          (int)haveCtrlModelMSFT, (int)haveRenderModelEXT,
          (int)haveInteractionRenderModelEXT);
    // The active backend's graphics extension is mandatory (#DX12 п.5 / #107).
    const bool haveActiveGfxExt =
        p->useVulkan ? haveVulkan : (p->useD3D12 ? haveD3D12 : haveD3D11);
    if (!haveActiveGfxExt)
    {
        XrDbg("OpenXR: runtime lacks %s -- VR unavailable\n",
              p->useVulkan ?
                  XR_KHR_VULKAN_ENABLE2_EXTENSION_NAME :
                  (p->useD3D12 ? XR_KHR_D3D12_ENABLE_EXTENSION_NAME :
                                 XR_KHR_D3D11_ENABLE_EXTENSION_NAME));
        return false;
    }

    // --- 2. Create instance (D3D11 + optional quad-views + eye-gaze) ---------
    // Quad views (4 viewports: wide context + narrow focus per eye) + eye gaze let the
    // foveated-rendering API layer (e.g. Quad-Views-Foveated) do gaze-driven foveation.
    // The whole view/swapchain/render path below is N-view generic, so 2 or 4 just works.
    std::vector<const char*> enabledExts;
    enabledExts.push_back(
        p->useVulkan ?
            XR_KHR_VULKAN_ENABLE2_EXTENSION_NAME // #107/#109 enable2
            :
            (p->useD3D12 ? XR_KHR_D3D12_ENABLE_EXTENSION_NAME // #DX12 п.5
                           :
                           XR_KHR_D3D11_ENABLE_EXTENSION_NAME));
    if (haveQuadViews)
        enabledExts.push_back(XR_VARJO_QUAD_VIEWS_EXTENSION_NAME);
    if (haveEyeGaze)
        enabledExts.push_back(XR_EXT_EYE_GAZE_INTERACTION_EXTENSION_NAME);
    // Artscout - 2026 (VR controllers): also ENABLE the controller render-model extension(s) so we can call
    // their functions later (loading the real controller glTF). Enumeration above only reports SUPPORT; using
    // the functions requires the extension to be in enabledExtensionNames here. Guarded by availability, and
    // with a fallback retry below -- if enabling them makes xrCreateInstance fail (unmet dependency on some
    // runtime), we drop them and retry so VR still comes up on the wireframe. The count split lets us peel
    // them back off precisely.
    const size_t coreExtCount = enabledExts.size();
    if (haveHandTracking)
        enabledExts.push_back(XR_EXT_HAND_TRACKING_EXTENSION_NAME);
    // #107: XR_EXT_render_model DEPENDS on XR_EXT_uuid -- enabling render_model without it makes xrCreateInstance
    // fail (seen on the SteamVR/Monado runtime: "attempted to enable XR_EXT_RENDER_MODEL without XR_EXT_UUID").
    // Push uuid FIRST so the dependency is satisfied and the first xrCreateInstance succeeds WITH hand tracking
    // retained (else the fallback retry below peels hand tracking off together with the render-model exts).
    // Artscout - 2026 (#11): the runtime controller render-models are OFF by default -- we draw our own hand/
    // controller OBJ meshes, so the runtime glTF models are unused and enabling them only risks xrCreateInstance
    // dependency failures. Gate all four pushes behind the flag (VrControllerRenderModels) so they stay peelable.
    extern bool g_bVrControllerRenderModels;
    if (g_bVrControllerRenderModels)
    {
        if ((haveRenderModelEXT || haveInteractionRenderModelEXT) && haveUuid)
            enabledExts.push_back("XR_EXT_uuid");
        if (haveRenderModelEXT)
            enabledExts.push_back("XR_EXT_render_model");
        if (haveInteractionRenderModelEXT)
            enabledExts.push_back("XR_EXT_interaction_render_model");
        if (haveCtrlModelMSFT)
            enabledExts.push_back("XR_MSFT_controller_model");
    }
    XrDbg("OpenXR: ext quadViews=%d eyeGaze=%d uuid=%d | enabling "
          "handTracking=%d renderModelEXT=%d interactionRenderModelEXT=%d "
          "ctrlModelMSFT=%d\n",
          (int)haveQuadViews, (int)haveEyeGaze, (int)haveUuid,
          (int)haveHandTracking, (int)haveRenderModelEXT,
          (int)haveInteractionRenderModelEXT, (int)haveCtrlModelMSFT);

    XrInstanceCreateInfo ici = {XR_TYPE_INSTANCE_CREATE_INFO};
    strcpy(ici.applicationInfo.applicationName, "FreeFalcon");
    ici.applicationInfo.applicationVersion = 1;
    strcpy(ici.applicationInfo.engineName, "FFViper");
    ici.applicationInfo.engineVersion = 1;
    // #109: XR_CURRENT_API_VERSION (now 1.1.61 with the 1.1 headers). Targets the current spec -- our runtimes (Pimax,
    // Quest 3) are 1.1. enable2 is used through the current-version path (xrCreateVulkanInstance/DeviceKHR).
    ici.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
    ici.enabledExtensionCount = (uint32_t)enabledExts.size();
    ici.enabledExtensionNames = enabledExts.data();
    const std::vector<const char*> fullExts =
        enabledExts; // full list incl. optional exts, for the 1.0 retry
    XrResult icr = xrCreateInstance(&ici, &p->instance);
    if (XR_FAILED(icr) && enabledExts.size() > coreExtCount)
    {
        // Retry without the optional render-model extensions so a missing dependency can't kill VR.
        XrDbg("OpenXR: xrCreateInstance failed (XrResult %d) WITH render-model "
              "exts; retrying without them\n",
              (int)icr);
        enabledExts.resize(coreExtCount);
        ici.enabledExtensionCount = (uint32_t)enabledExts.size();
        ici.enabledExtensionNames = enabledExts.data();
        icr = xrCreateInstance(&ici, &p->instance);
    }
    if (icr == XR_ERROR_API_VERSION_UNSUPPORTED)
    {
        // The runtime rejected XR_CURRENT_API_VERSION (our 1.1 headers request 1.1.61; an installed loader/runtime
        // may be older, e.g. SteamVR's 1.1.47). Retry at successively lower baselines -- 1.1 first (keeps 1.1
        // features), then 1.0 -- with the full ext set restored (the failure was the version, not the extensions).
        const XrVersion candidates[] = {XR_MAKE_VERSION(1, 1, 0),
                                        XR_MAKE_VERSION(1, 0, 0)};
        for (XrVersion ver : candidates)
        {
            XrDbg("OpenXR: retrying xrCreateInstance at API %u.%u\n",
                  (unsigned)XR_VERSION_MAJOR(ver),
                  (unsigned)XR_VERSION_MINOR(ver));
            ici.applicationInfo.apiVersion = ver;
            enabledExts = fullExts;
            ici.enabledExtensionCount = (uint32_t)enabledExts.size();
            ici.enabledExtensionNames = enabledExts.data();
            icr = xrCreateInstance(&ici, &p->instance);
            if (XR_FAILED(icr) && enabledExts.size() > coreExtCount)
            {
                // version accepted range but an optional ext is unhappy -> peel the render-model exts off.
                enabledExts.resize(coreExtCount);
                ici.enabledExtensionCount = (uint32_t)enabledExts.size();
                ici.enabledExtensionNames = enabledExts.data();
                icr = xrCreateInstance(&ici, &p->instance);
            }
            if (XR_SUCCEEDED(icr))
                break;
        }
    }
    if (XR_FAILED(icr))
    {
        XrDbg("OpenXR: xrCreateInstance failed (XrResult %d)\n", (int)icr);
        return false;
    }
    // Hand tracking is usable only if it survived on the instance (the fallback retry peels ALL optional
    // exts off together, so if it fired, hand tracking is gone too). Entry points are resolved after this.
    p->handTrackingEnabled =
        haveHandTracking && (enabledExts.size() > coreExtCount);

    // --- 3. System (HMD) -----------------------------------------------------
    XrSystemGetInfo sgi = {XR_TYPE_SYSTEM_GET_INFO};
    sgi.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    if (XR_FAILED(xrGetSystem(p->instance, &sgi, &p->systemId)))
    {
        XrDbg("OpenXR: no HMD system available (headset off?)\n");
        Shutdown();
        return false;
    }

    // --- 3b. Prefer the quad-views (foveated) config if the runtime exposes it. ---
    // Everything downstream (configViews/views/swapchains/render loop/projection layer)
    // sizes off the view count, so this turns on 4-view rendering with no other change.
    p->viewConfigType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
    // v1 per-eye stereo uses the engine's symmetric projection + IPD position offset, which is
    // correct for STEREO (parallel) but NOT for QUAD's off-center (gaze-tracked) focus views. The
    // whole downstream path is N-view generic, so 4 views "just render"; with fixed foveation (no
    // eye tracking) the focus views are centered and the symmetric render stays internally consistent
    // (render fov == submit fov). Gated behind FFViper.cfg "UseQuadViews 1" -- EXPERIMENTAL, default
    // OFF so the proven 2-view stereo path stays the norm. (g_bUseQuadViews lives in f4config.cpp.)
    extern bool g_bUseQuadViews;
    if (g_bUseQuadViews && haveQuadViews)
    {
        uint32_t vcCount = 0;
        if (XR_SUCCEEDED(xrEnumerateViewConfigurations(p->instance, p->systemId,
                                                       0, &vcCount, NULL)) &&
            vcCount)
        {
            std::vector<XrViewConfigurationType> vcs(vcCount);
            if (XR_SUCCEEDED(xrEnumerateViewConfigurations(
                    p->instance, p->systemId, vcCount, &vcCount, vcs.data())))
                for (uint32_t i = 0; i < vcCount; ++i)
                    if (vcs[i] == XR_VIEW_CONFIGURATION_TYPE_PRIMARY_QUAD_VARJO)
                    {
                        p->viewConfigType =
                            XR_VIEW_CONFIGURATION_TYPE_PRIMARY_QUAD_VARJO;
                        break;
                    }
        }
    }
    XrDbg("OpenXR: view config = %s\n",
          p->viewConfigType == XR_VIEW_CONFIGURATION_TYPE_PRIMARY_QUAD_VARJO ?
              "QUAD_VARJO (4 views)" :
              "STEREO (2 views)");
    return true;
}

//=============================================================================
// Init
//=============================================================================
bool OpenXRBackend::Init(ID3D11Device* device)
{
    Impl* p = m_impl;

    // #DX12 п.5: bind the session to the D3D12 device/queue (from g_pD3D12Backend) instead of D3D11.
    // The 'device' param is NULL under D3D12 -- the D3D12 devmgr branch calls Init(NULL).
    extern bool g_bUseD3D12;
    extern bool
        g_bUseVulkan; // #107: Vulkan VR path -- takes priority over D3D12 when the Vulkan backend is active
    p->useVulkan = g_bUseVulkan;
    p->useD3D12 = g_bUseD3D12 && !g_bUseVulkan;
    if (p->useVulkan)
    {
        // #107 Phase 1: bind the session to the Vulkan device VulkanBackend already created (device ownership stays
        // with the renderer -- OpenXR only references the handles). Copy them here; the graphics binding + xrCreateSession
        // happen after the instance/system are up (step 4/5). The 'device' param is NULL under Vulkan (devmgr calls Init(NULL)).
        if (!g_pVulkanBackend || !g_pVulkanBackend->IsValid())
        {
            XrDbg("OpenXR: Init - no Vulkan backend\n");
            return false;
        }
        p->vkInstance = (VkInstance)g_pVulkanBackend->VkInstanceHandle();
        p->vkPhysDevice =
            (VkPhysicalDevice)g_pVulkanBackend->VkPhysicalDeviceHandle();
        p->vkDevice = (VkDevice)g_pVulkanBackend->VkDeviceHandle();
        p->vkQueueFamily = g_pVulkanBackend->VkGraphicsFamily();
        p->vkQueueIndex =
            g_pVulkanBackend
                ->VkXrQueueIndex(); // #107: dedicated queue (index 1) so the runtime doesn't race the app's queue 0
        if (!p->vkInstance || !p->vkDevice || !p->vkPhysDevice)
        {
            XrDbg("OpenXR: Init - null Vulkan handles\n");
            return false;
        }
        XrDbg("OpenXR: Vulkan handles adopted (inst=%p phys=%p dev=%p "
              "gfxFamily=%u)\n",
              (void*)p->vkInstance, (void*)p->vkPhysDevice, (void*)p->vkDevice,
              p->vkQueueFamily);
    }
#ifdef _WIN32
    else if (p->useD3D12)
    {
        if (!g_pD3D12Backend || !g_pD3D12Backend->IsValid())
        {
            XrDbg("OpenXR: Init - no D3D12 backend\n");
            return false;
        }
        p->d3d12Device = g_pD3D12Backend->GetDevice();
        p->d3d12Queue = g_pD3D12Backend->GetQueue();
        if (!p->d3d12Device || !p->d3d12Queue)
        {
            XrDbg("OpenXR: Init - null D3D12 device/queue\n");
            return false;
        }
        // RTV heap + a command list/allocator/fence for the eye clear/composite work (D3D12 has no clear-by-view).
        D3D12_DESCRIPTOR_HEAP_DESC hd;
        ZeroMemory(&hd, sizeof(hd));
        hd.NumDescriptors = 64;
        hd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        hd.Flags =
            D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // #DX12 п.5: quad VI = 1 array + 4 slice RTVs per image
        if (FAILED(p->d3d12Device->CreateDescriptorHeap(
                &hd, IID_PPV_ARGS(&p->rtvHeap12))))
        {
            XrDbg("OpenXR: RTV heap failed\n");
            return false;
        }
        p->rtvInc12 = p->d3d12Device->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        p->rtvHead12 = 0;
        p->d3d12Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                               IID_PPV_ARGS(&p->alloc12));
        p->d3d12Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                                          p->alloc12, NULL,
                                          IID_PPV_ARGS(&p->list12));
        if (p->list12)
            p->list12->Close();
        p->d3d12Device->CreateFence(0, D3D12_FENCE_FLAG_NONE,
                                    IID_PPV_ARGS(&p->fence12));
        p->fenceVal12 = 0;
        p->fenceEvt12 = CreateEvent(NULL, FALSE, FALSE, NULL);
    }
    else
    {
        if (!device)
        {
            XrDbg("OpenXR: Init - null device\n");
            return false;
        }
        p->device = device;
        device->GetImmediateContext(&p->ctx);

        // The engine drives D3D11 from several threads (sim render, ui95 OutputLoop) and
        // the XR pump adds another; the immediate context is NOT thread-safe by default
        // (-> "CORRUPTED_MULTITHREADING" + crash). Turn on the runtime's internal locking
        // so concurrent context calls are serialized. Required once VR is on.
        ID3D11Multithread* mt = NULL;
        if (SUCCEEDED(p->ctx->QueryInterface(__uuidof(ID3D11Multithread),
                                             (void**)&mt)) &&
            mt)
        {
            mt->SetMultithreadProtected(TRUE);
            mt->Release();
            XrDbg("OpenXR: D3D11 multithread protection enabled\n");
        }
        else
            XrDbg("OpenXR: WARNING could not enable D3D11 multithread "
                  "protection\n");
    }
#else
    // #108: Linux builds only the Vulkan graphics binding. If the Vulkan path was not taken (useVulkan false), there is
    // no D3D fallback -- fail so the caller keeps the flat path.
    else
    {
        XrDbg("OpenXR: Init - non-Vulkan graphics path is Windows-only "
              "(Linux)\n");
        return false;
    }
#endif

    // #107: create the instance/system here only if the Vulkan pre-init did not already do it (single-instance rule).
    if (p->instance == XR_NULL_HANDLE && !CreateInstanceAndSystem())
        return false;

    // --- 4/5. graphics requirements + session (bound to the active backend's device) --------
    if (p->useVulkan) // #107/#109 enable2: Vulkan graphics binding.
    {
        // The runtime CREATED this VkInstance/VkDevice (via the hooks in VulkanBackend::Init -> xrCreateVulkan*KHR) and
        // NAMED p->vkPhysDevice (xrGetVulkanGraphicsDevice2KHR), so it already injected its interop extensions and chose
        // the HMD adapter -- no v1 extension query, no device-mismatch warn. Requirements were checked in PreInitVulkan.
        // The Vulkan2 graphics binding struct is a typedef of the Vulkan KHR one (same fields).
        XrGraphicsBindingVulkan2KHR gb = {XR_TYPE_GRAPHICS_BINDING_VULKAN2_KHR};
        gb.instance = p->vkInstance;
        gb.physicalDevice = p->vkPhysDevice;
        gb.device = p->vkDevice;
        gb.queueFamilyIndex = p->vkQueueFamily;
        gb.queueIndex = p->vkQueueIndex;
        XrSessionCreateInfo sci = {XR_TYPE_SESSION_CREATE_INFO};
        sci.next = &gb;
        sci.systemId = p->systemId;
        XR_BAIL(xrCreateSession(p->instance, &sci, &p->session),
                "xrCreateSession(Vulkan2)");
        XrDbg("OpenXR: Vulkan session created (enable2, session=%p)\n",
              (void*)p->session);
    }
#ifdef _WIN32
    else if (
        p->useD3D12) // #DX12 п.5: D3D12 graphics binding (device + command queue)
    {
        PFN_xrGetD3D12GraphicsRequirementsKHR pfnReqs = NULL;
        xrGetInstanceProcAddr(p->instance, "xrGetD3D12GraphicsRequirementsKHR",
                              (PFN_xrVoidFunction*)&pfnReqs);
        if (!pfnReqs)
        {
            XrDbg("OpenXR: xrGetD3D12GraphicsRequirementsKHR unavailable\n");
            Shutdown();
            return false;
        }
        XrGraphicsRequirementsD3D12KHR reqs = {
            XR_TYPE_GRAPHICS_REQUIREMENTS_D3D12_KHR};
        XR_BAIL(pfnReqs(p->instance, p->systemId, &reqs),
                "xrGetD3D12GraphicsRequirementsKHR");

        XrGraphicsBindingD3D12KHR gb = {XR_TYPE_GRAPHICS_BINDING_D3D12_KHR};
        gb.device = p->d3d12Device;
        gb.queue = p->d3d12Queue;
        XrSessionCreateInfo sci = {XR_TYPE_SESSION_CREATE_INFO};
        sci.next = &gb;
        sci.systemId = p->systemId;
        XR_BAIL(xrCreateSession(p->instance, &sci, &p->session),
                "xrCreateSession(D3D12)");
    }
    else
    {
        PFN_xrGetD3D11GraphicsRequirementsKHR pfnReqs = NULL;
        xrGetInstanceProcAddr(p->instance, "xrGetD3D11GraphicsRequirementsKHR",
                              (PFN_xrVoidFunction*)&pfnReqs);
        if (!pfnReqs)
        {
            XrDbg("OpenXR: xrGetD3D11GraphicsRequirementsKHR unavailable\n");
            Shutdown();
            return false;
        }
        XrGraphicsRequirementsD3D11KHR reqs = {
            XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR};
        XR_BAIL(pfnReqs(p->instance, p->systemId, &reqs),
                "xrGetD3D11GraphicsRequirementsKHR");

        // Warn (do not fail) if our existing device is on a different adapter than the
        // runtime wants -- on a single-GPU PC they match.
        IDXGIDevice* dxgiDev = NULL;
        if (SUCCEEDED(device->QueryInterface(__uuidof(IDXGIDevice),
                                             (void**)&dxgiDev)) &&
            dxgiDev)
        {
            IDXGIAdapter* adapter = NULL;
            if (SUCCEEDED(dxgiDev->GetAdapter(&adapter)) && adapter)
            {
                DXGI_ADAPTER_DESC desc;
                if (SUCCEEDED(adapter->GetDesc(&desc)))
                {
                    if (memcmp(&desc.AdapterLuid, &reqs.adapterLuid,
                               sizeof(LUID)) != 0)
                        XrDbg("OpenXR: WARNING device adapter LUID != XR "
                              "adapter LUID (multi-GPU?)\n");
                }
                adapter->Release();
            }
            dxgiDev->Release();
        }

        XrGraphicsBindingD3D11KHR gb = {XR_TYPE_GRAPHICS_BINDING_D3D11_KHR};
        gb.device = device;
        XrSessionCreateInfo sci = {XR_TYPE_SESSION_CREATE_INFO};
        sci.next = &gb;
        sci.systemId = p->systemId;
        XR_BAIL(xrCreateSession(p->instance, &sci, &p->session),
                "xrCreateSession");
    }
#endif // _WIN32 (D3D12/D3D11 graphics bindings)

    // --- 6. Reference spaces (app=LOCAL, head=VIEW) --------------------------
    XrReferenceSpaceCreateInfo rsci = {XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
    rsci.poseInReferenceSpace.orientation.w = 1.0f;
    rsci.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
    XR_BAIL(xrCreateReferenceSpace(p->session, &rsci, &p->appSpace),
            "xrCreateReferenceSpace(LOCAL)");
    // Artscout - 2026 (#67): a second, identity LOCAL space kept PRISTINE as the recenter measurement frame.
    XR_BAIL(xrCreateReferenceSpace(p->session, &rsci, &p->localRef),
            "xrCreateReferenceSpace(LOCAL ref)");
    rsci.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
    XR_BAIL(xrCreateReferenceSpace(p->session, &rsci, &p->viewSpace),
            "xrCreateReferenceSpace(VIEW)");

    // Artscout - 2026 (VR controllers, Phase 1): action-based input. Non-fatal -- VR still runs without it
    // (the cockpit falls back to the VR mouse when no controller is tracked).
    if (!CreateInputActions())
        XrDbg("OpenXR: controller input unavailable (continuing without "
              "controllers)\n");

    // Artscout - 2026 (VR hands): resolve the hand-tracking entry points and create a tracker per hand.
    // Non-fatal: any failure just leaves handTrackingEnabled effectively off and the wireframe is used.
    XrDbg("OpenXR: handTrackingEnabled(instance)=%d\n",
          (int)p->handTrackingEnabled);
    if (p->handTrackingEnabled)
    {
        // Authoritative check: the runtime may ADVERTISE the extension yet report the SYSTEM can't do hand
        // tracking (common when there's no camera and the controller driver doesn't synthesize a skeleton).
        XrSystemHandTrackingPropertiesEXT htp = {
            XR_TYPE_SYSTEM_HAND_TRACKING_PROPERTIES_EXT};
        XrSystemProperties sp = {XR_TYPE_SYSTEM_PROPERTIES};
        sp.next = &htp;
        XrResult spr = xrGetSystemProperties(p->instance, p->systemId, &sp);
        XrDbg("OpenXR: system supportsHandTracking=%d (xrGetSystemProperties "
              "%d)\n",
              (int)htp.supportsHandTracking, (int)spr);

        xrGetInstanceProcAddr(p->instance, "xrCreateHandTrackerEXT",
                              (PFN_xrVoidFunction*)&p->pfnCreateHandTracker);
        xrGetInstanceProcAddr(p->instance, "xrDestroyHandTrackerEXT",
                              (PFN_xrVoidFunction*)&p->pfnDestroyHandTracker);
        xrGetInstanceProcAddr(p->instance, "xrLocateHandJointsEXT",
                              (PFN_xrVoidFunction*)&p->pfnLocateHandJoints);
        XrDbg("OpenXR: hand PFNs create=%p locate=%p\n",
              (void*)p->pfnCreateHandTracker, (void*)p->pfnLocateHandJoints);
        if (p->pfnCreateHandTracker && p->pfnLocateHandJoints)
        {
            for (int h = 0; h < 2; ++h)
            {
                XrHandTrackerCreateInfoEXT hci = {
                    XR_TYPE_HAND_TRACKER_CREATE_INFO_EXT};
                hci.hand = (h == 0) ? XR_HAND_LEFT_EXT : XR_HAND_RIGHT_EXT;
                hci.handJointSet = XR_HAND_JOINT_SET_DEFAULT_EXT;
                XrResult hr = p->pfnCreateHandTracker(p->session, &hci,
                                                      &p->handTracker[h]);
                if (XR_FAILED(hr))
                {
                    p->handTracker[h] = XR_NULL_HANDLE;
                    XrDbg(
                        "OpenXR: xrCreateHandTrackerEXT(hand %d) failed (%d)\n",
                        h, (int)hr);
                }
            }
            XrDbg("OpenXR: hand tracking initialised (L=%d R=%d)\n",
                  (int)(p->handTracker[0] != XR_NULL_HANDLE),
                  (int)(p->handTracker[1] != XR_NULL_HANDLE));
        }
        else
        {
            p->handTrackingEnabled = false;
            XrDbg("OpenXR: hand-tracking entry points missing -- disabled\n");
        }
    }

    // --- 7. Per-eye view configuration ---------------------------------------
    uint32_t viewCount = 0;
    XR_BAIL(xrEnumerateViewConfigurationViews(p->instance, p->systemId,
                                              p->viewConfigType, 0, &viewCount,
                                              NULL),
            "xrEnumerateViewConfigurationViews(count)");
    p->configViews.resize(viewCount);
    for (uint32_t i = 0; i < viewCount; ++i)
    {
        p->configViews[i].type = XR_TYPE_VIEW_CONFIGURATION_VIEW;
        p->configViews[i].next = NULL;
    }
    XR_BAIL(xrEnumerateViewConfigurationViews(
                p->instance, p->systemId, p->viewConfigType, viewCount,
                &viewCount, p->configViews.data()),
            "xrEnumerateViewConfigurationViews");
    p->views.resize(viewCount);
    for (uint32_t i = 0; i < viewCount; ++i)
    {
        p->views[i].type = XR_TYPE_VIEW;
        p->views[i].next = NULL;
    }

    // Artscout - 2026 (#107 Phase 1 boundary): the Vulkan swapchain + render + submit paths are Phase 2+. Everything
    // above (session, reference spaces, input actions, hand trackers, view config) is backend-agnostic and has now
    // exercised the Vulkan session, so its logs are meaningful. Stop here with the session confirmed -- a build can
    // verify the Vulkan session comes up (openxr_diag.txt) -- and return false to keep the flat path until the
    // --- 8. Pick a color swapchain format ------------------------------------
    // The format list is API-specific: DXGI_FORMAT ints for a D3D session, VkFormat ints for a Vulkan session.
    uint32_t fmtCount = 0;
    XR_BAIL(xrEnumerateSwapchainFormats(p->session, 0, &fmtCount, NULL),
            "xrEnumerateSwapchainFormats(count)");
    std::vector<int64_t> fmts(fmtCount);
    XR_BAIL(xrEnumerateSwapchainFormats(p->session, fmtCount, &fmtCount,
                                        fmts.data()),
            "xrEnumerateSwapchainFormats");
#ifdef _WIN32
    const int64_t preferredD3D[] = {
        DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
        DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_B8G8R8A8_UNORM};
#endif
    // #107: VkFormat preferences (SRGB variants first -- the runtime blends/scans out assuming sRGB, matching the
    // desktop swapchain). The blit source (VulkanBackend scene array) is UNORM; a UNORM->SRGB blit re-encodes gamma,
    // which is what the compositor expects, so an SRGB eye image is correct.
    const int64_t preferredVk[] = {
        VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_B8G8R8A8_SRGB,
        VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8A8_UNORM};
    const int64_t* preferred =
        preferredVk; // #108: Vulkan is the only graphics path on Linux
#ifdef _WIN32
    if (!p->useVulkan)
        preferred = preferredD3D;
#endif
    p->swapchainFormat = 0;
    for (int pf = 0; pf < 4 && p->swapchainFormat == 0; ++pf)
        for (uint32_t i = 0; i < fmtCount; ++i)
            if (fmts[i] == preferred[pf])
            {
                p->swapchainFormat = preferred[pf];
                break;
            }
    if (p->swapchainFormat == 0 && fmtCount > 0)
        p->swapchainFormat = fmts[0];
    XrDbg("OpenXR: chosen swapchain color format = %lld (%s)\n",
          (long long)p->swapchainFormat, p->useVulkan ? "VkFormat" : "DXGI");
    // Artscout - 2026: Vulkan copies into the eye image with a blit, which
    // ENCODES gamma into an sRGB target the D3D12 path writes raw. Hand the
    // backend the format so it can decode first -- see SetXrColorFormat.
    if (p->useVulkan && g_pVulkanBackend)
        g_pVulkanBackend->SetXrColorFormat((unsigned int)p->swapchainFormat);

    // Menu quad swapchain must be an R8G8B8A8 variant: the 565->RGBA staging upload
    // is R8G8B8A8_UNORM and CopyResource needs the same format family. Prefer _SRGB
    // (the UI bytes are sRGB-encoded, like the desktop swapchain), else plain UNORM.
    // #108: on Linux the menu quad swapchain is Vulkan (RunVulkanMenuFrame / uiImagesVk); it picks its own VkFormat
    // in EnsureUiSwapchain. The DXGI uiFormat pick below is only for the D3D UI staging path (Windows).
#ifdef _WIN32
    const int64_t uiPref[] = {DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
                              DXGI_FORMAT_R8G8B8A8_UNORM};
    p->uiFormat = 0;
    for (int pf = 0; pf < 2 && p->uiFormat == 0; ++pf)
        for (uint32_t i = 0; i < fmtCount; ++i)
            if (fmts[i] == uiPref[pf])
            {
                p->uiFormat = uiPref[pf];
                break;
            }
#endif

    // --- 8b. #DX12 п.5 -- decide single-pass view instancing. Requires: the cfg flag, D3D12, a 2-view STEREO
    // session (NOT quad), the device's ViewInstancing tier, AND the renderer's DXC/SM6.1 VI shaders. Any miss ->
    // the proven per-eye path. (Renderer inits before OpenXR, so ViewInstancingAvailable() is valid here.)
    {
        extern bool g_bVrViewInstancing;
        // VI supports 2-view STEREO (1 group) and 4-view QUAD (2 groups: periphery pair + focus pair). QUAD is
        // done as TWO 2-view VI passes, each into its OWN arraySize=2 swapchain at that pair's resolution -- this
        // keeps foveated per-pair resolution AND avoids the arraySize=4 swapchain the quad_views_foveated layer
        // rejects (xrEndFrame HANDLE_INVALID). Each group is a standard 2-view VI (reuses the stereo machinery).
        bool viewsOk = (viewCount == 2 || viewCount == 4);
        bool want = g_bVrViewInstancing && p->useD3D12 && viewsOk;
#ifdef _WIN32
        bool tierOk =
            g_pD3D12Backend && g_pD3D12Backend->ViewInstancingSupported();
        bool shOk =
            g_pD3D12Renderer && g_pD3D12Renderer->ViewInstancingAvailable();
#else
        // #108: view instancing is a D3D12 feature (D3D12Backend/Renderer). Not available on the Vulkan/Linux path.
        bool tierOk = false;
        bool shOk = false;
#endif
        p->viActive = want && tierOk && shOk;
        p->viDiagFlag = g_bVrViewInstancing;
        p->viDiagStereo = viewsOk;
        p->viDiagTier = tierOk;
        p->viDiagSh = shOk;
        XrDbg("OpenXR: view instancing %s (flag=%d viewsOk=%d d3d12=%d tier=%d "
              "shaders=%d viewCount=%d)\n",
              p->viActive ?
                  (viewCount == 4 ?
                       "ACTIVE (2-pass QUAD foveated: periphery+focus)" :
                       "ACTIVE (single-pass stereo)") :
                  "off (per-eye)",
              (int)g_bVrViewInstancing, (int)viewsOk, (int)p->useD3D12,
              (int)tierOk, (int)shOk, (int)viewCount);
    }

    // --- 9. Swapchains + RTVs. THREE layouts:
    //  * non-VI      : N per-eye swapchains, arraySize 1, rendered directly per eye.
    //  * VI DIRECT   : stereo (2 views) -> ONE arraySize=2 array swapchain, single VI pass (no foveation layer).
    //  * VI COPY     : quad (4 views) -> 4 per-view arraySize=1 swapchains (the quad_views_foveated layer rejects
    //                  ANY array swapchain -> HANDLE_INVALID); the VI pass renders into a PRIVATE array target and
    //                  the slices are COPIED into these per-view images. Per-view resolution = configViews[e] (foveated).
    const bool viDirect = p->viActive && (viewCount == 2);
    const bool viCopy = p->viActive && (viewCount == 4);
    const uint32_t scCount =
        viDirect ?
            1u :
            (uint32_t)viewCount; // viCopy + non-VI = one swapchain per view
    p->swapchains.resize(scCount);
    for (uint32_t e = 0; e < scCount; ++e)
    {
        Impl::Swapchain& sc = p->swapchains[e];
        // Resolution: viDirect group 0 = configViews[0]; viCopy/non-VI per view = configViews[e] (foveated per view).
        const uint32_t cvi = viDirect ? 0u : e;
        sc.width = (int32_t)p->configViews[cvi].recommendedImageRectWidth;
        sc.height = (int32_t)p->configViews[cvi].recommendedImageRectHeight;

        // Artscout - 2026: optional per-eye resolution scale (Advanced page "OpenXR Resolution Scale", 70..100%).
        // Trades sharpness for GPU headroom. 100 = the runtime's recommended size (unchanged). The whole engine
        // derives the per-eye viewport/projection from sc.width/height, so scaling here flows everywhere.
        {
            extern int g_nVrResolutionScale;
            int scl = g_nVrResolutionScale;
            if (scl < 50)
                scl = 50;
            if (scl > 100)
                scl = 100;
            if (scl < 100)
            {
                sc.width = (sc.width * scl) / 100;
                sc.height = (sc.height * scl) / 100;
                if (sc.width < 256)
                    sc.width = 256;
                if (sc.height < 256)
                    sc.height = 256;
            }
        }

        XrSwapchainCreateInfo scci = {XR_TYPE_SWAPCHAIN_CREATE_INFO};
        // Artscout - 2026: keep usage IDENTICAL to the proven non-VI quad path (COLOR_ATTACHMENT | SAMPLED). Do NOT
        // add TRANSFER_DST_BIT for viCopy: the quad_views_foveated layer appears to reject it (xrEndFrame ->
        // HANDLE_INVALID). D3D12 lets us CopyTextureRegion into the image regardless (COPY_DEST is a state, not a
        // creation flag), so the XR transfer hint is unnecessary.
        scci.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT |
                          XR_SWAPCHAIN_USAGE_SAMPLED_BIT;
        // #107: the Vulkan BLIT copy model blits the multiview scene layer INTO this image, so it must be a transfer dst.
        // Unlike D3D12 (COPY_DEST is a runtime state), Vulkan bakes usage at creation -- vkCmdBlitImage as dst is invalid
        // without it.
        if (p->useVulkan)
            scci.usageFlags |= XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT;
        scci.format = p->swapchainFormat;
        scci.sampleCount = 1;
        scci.width = sc.width;
        scci.height = sc.height;
        scci.faceCount = 1;
        scci.arraySize =
            viDirect ?
                2 :
                1; // #DX12 п.5: stereo VI = one 2-slice array; quad VI (copy) + non-VI = per-view arraySize 1
        scci.mipCount = 1;
        XR_BAIL(xrCreateSwapchain(p->session, &scci, &sc.handle),
                "xrCreateSwapchain");
        XrDbg("OpenXR: swapchain[%u] %dx%d arr=%u handle=%p (viDirect=%d "
              "viCopy=%d)\n",
              e, (int)sc.width, (int)sc.height, (unsigned)scci.arraySize,
              (void*)sc.handle, (int)viDirect, (int)viCopy);

        uint32_t imgCount = 0;
        XR_BAIL(xrEnumerateSwapchainImages(sc.handle, 0, &imgCount, NULL),
                "xrEnumerateSwapchainImages(count)");

        if (p->useVulkan) // #107: Vulkan swapchain images -> VkImages (blit targets; no views/RTVs needed)
        {
            sc.imagesVk.resize(imgCount);
            for (uint32_t i = 0; i < imgCount; ++i)
            {
                sc.imagesVk[i].type = XR_TYPE_SWAPCHAIN_IMAGE_VULKAN_KHR;
                sc.imagesVk[i].next = NULL;
            }
            XR_BAIL(xrEnumerateSwapchainImages(
                        sc.handle, imgCount, &imgCount,
                        (XrSwapchainImageBaseHeader*)sc.imagesVk.data()),
                    "xrEnumerateSwapchainImages(Vulkan)");
            XrDbg("OpenXR: swapchain[%u] Vulkan images=%u (img0=%p)\n", e,
                  imgCount, imgCount ? (void*)sc.imagesVk[0].image : NULL);
        }
#ifdef _WIN32
        else if (
            p->useD3D12) // #DX12 п.5: D3D12 swapchain images -> RTVs in the shared rtvHeap12
        {
            sc.images12.resize(imgCount);
            for (uint32_t i = 0; i < imgCount; ++i)
            {
                sc.images12[i].type = XR_TYPE_SWAPCHAIN_IMAGE_D3D12_KHR;
                sc.images12[i].next = NULL;
            }
            XR_BAIL(xrEnumerateSwapchainImages(
                        sc.handle, imgCount, &imgCount,
                        (XrSwapchainImageBaseHeader*)sc.images12.data()),
                    "xrEnumerateSwapchainImages(D3D12)");
            sc.rtvs12.resize(imgCount, 0);
            if (viDirect)
            {
                sc.sliceRtvs12[0].resize(imgCount, 0);
                sc.sliceRtvs12[1].resize(imgCount, 0);
            } // stereo: 2 slice RTVs
            // #DX12 п.5: the runtime picks an _SRGB swapchain format (fmt=29 R8G8B8A8_UNORM_SRGB), but the
            // renderer's PSOs bake R8G8B8A8_UNORM -> #613 RENDER_TARGET_FORMAT_MISMATCH and dropped draws.
            // Create the RTV with the UNORM cast of the same family (legal, same memory layout) so the eye
            // image accepts the scene draws (no gamma re-encode -- matches how the D3D11 eye path writes).
            DXGI_FORMAT rtvFmt = (DXGI_FORMAT)p->swapchainFormat;
            if (rtvFmt == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB)
                rtvFmt = DXGI_FORMAT_R8G8B8A8_UNORM;
            else if (rtvFmt == DXGI_FORMAT_B8G8R8A8_UNORM_SRGB)
                rtvFmt = DXGI_FORMAT_B8G8R8A8_UNORM;
            for (uint32_t i = 0; i < imgCount; ++i)
            {
                if (!viDirect) // viCopy + non-VI: per-view 2D RTV (viCopy's is unused -- slices are copied in)
                {
                    D3D12_CPU_DESCRIPTOR_HANDLE h =
                        p->rtvHeap12->GetCPUDescriptorHandleForHeapStart();
                    h.ptr += (SIZE_T)p->rtvHead12 * p->rtvInc12;
                    p->rtvHead12++;
                    D3D12_RENDER_TARGET_VIEW_DESC rd;
                    ZeroMemory(&rd, sizeof(rd));
                    rd.Format = rtvFmt;
                    rd.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
                    p->d3d12Device->CreateRenderTargetView(
                        sc.images12[i].texture, &rd, h);
                    sc.rtvs12[i] = (unsigned __int64)h.ptr;
                }
                else
                {
                    // Each VI-group swapchain has 2 slices (a 2-view pass). One array RTV covering both slices (the
                    // VI geometry pass) + one RTV per slice (the per-view 2D overlay tail). Entry 0 = array
                    // (FirstSlice 0, ArraySize 2); entries 1,2 = individual slices.
                    for (int v = 0; v <= 2; ++v)
                    {
                        D3D12_CPU_DESCRIPTOR_HANDLE h =
                            p->rtvHeap12->GetCPUDescriptorHandleForHeapStart();
                        h.ptr += (SIZE_T)p->rtvHead12 * p->rtvInc12;
                        p->rtvHead12++;
                        D3D12_RENDER_TARGET_VIEW_DESC rd;
                        ZeroMemory(&rd, sizeof(rd));
                        rd.Format = rtvFmt;
                        rd.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
                        rd.Texture2DArray.FirstArraySlice =
                            (v == 0) ? 0 : (UINT)(v - 1);
                        rd.Texture2DArray.ArraySize = (v == 0) ? 2u : 1u;
                        p->d3d12Device->CreateRenderTargetView(
                            sc.images12[i].texture, &rd, h);
                        if (v == 0)
                            sc.rtvs12[i] = (unsigned __int64)h.ptr;
                        else
                            sc.sliceRtvs12[v - 1][i] = (unsigned __int64)h.ptr;
                    }
                }
            }
        }
        else
        {
            sc.images.resize(imgCount);
            for (uint32_t i = 0; i < imgCount; ++i)
            {
                sc.images[i].type = XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR;
                sc.images[i].next = NULL;
            }
            XR_BAIL(xrEnumerateSwapchainImages(
                        sc.handle, imgCount, &imgCount,
                        (XrSwapchainImageBaseHeader*)sc.images.data()),
                    "xrEnumerateSwapchainImages");

            sc.rtvs.resize(imgCount, NULL);
            for (uint32_t i = 0; i < imgCount; ++i)
            {
                D3D11_RENDER_TARGET_VIEW_DESC rtvd;
                ZeroMemory(&rtvd, sizeof(rtvd));
                rtvd.Format = (DXGI_FORMAT)p->swapchainFormat;
                rtvd.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
                if (FAILED(device->CreateRenderTargetView(sc.images[i].texture,
                                                          &rtvd, &sc.rtvs[i])))
                {
                    XrDbg("OpenXR: CreateRenderTargetView failed (eye %u img "
                          "%u)\n",
                          e, i);
                    Shutdown();
                    return false;
                }
            }
        }
#endif // _WIN32 (D3D12 / D3D11 swapchain images)
    }

    if (p->useVulkan)
        XrDbg("OpenXR: [PHASE 2b] Vulkan VR up -- %u views, %dx%d, multiview "
              "render+blit+present wired\n",
              viewCount, p->swapchains[0].width, p->swapchains[0].height);
    XrDbg("OpenXR: up -- %u views, %dx%d, fmt=%lld\n", viewCount,
          p->swapchains[0].width, p->swapchains[0].height,
          (long long)p->swapchainFormat);
    return true;
}

//=============================================================================
// Event pump -- drives the session state machine. Shared by RunFrame/RunMenuFrame.
//=============================================================================
void OpenXRBackend::PollEvents()
{
    Impl* p = m_impl;
    XrEventDataBuffer ev;
    for (;;)
    {
        ev.type = XR_TYPE_EVENT_DATA_BUFFER;
        ev.next = NULL;
        if (xrPollEvent(p->instance, &ev) != XR_SUCCESS)
            break; // XR_EVENT_UNAVAILABLE -> done

        if (ev.type == XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED)
        {
            const XrEventDataSessionStateChanged& ssc =
                *(XrEventDataSessionStateChanged*)&ev;
            p->sessionState = ssc.state;
            switch (ssc.state)
            {
            case XR_SESSION_STATE_READY:
            {
                XrSessionBeginInfo bi = {XR_TYPE_SESSION_BEGIN_INFO};
                bi.primaryViewConfigurationType = p->viewConfigType;
                if (XR_SUCCEEDED(xrBeginSession(p->session, &bi)))
                {
                    p->sessionRunning = true;
                    XrDbg("OpenXR: session begun (running)\n");
                }
                break;
            }
            case XR_SESSION_STATE_STOPPING:
                xrEndSession(p->session);
                p->sessionRunning = false;
                XrDbg("OpenXR: session stopped\n");
                break;
            case XR_SESSION_STATE_EXITING:
            case XR_SESSION_STATE_LOSS_PENDING:
                p->sessionRunning = false;
                break;
            default:
                break;
            }
        }
        else if (ev.type == XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING)
        {
            p->sessionRunning = false;
        }
    }
}

#ifdef _WIN32
// #DX12 п.5: clear an acquired XR D3D12 swapchain image (color) via the dedicated queue. XR D3D12 images
// start/rest in COMMON; we round-trip COMMON->RENDER_TARGET->COMMON so the runtime always gets a valid state.
// Synchronous (fence-wait) -- the runtime's xrReleaseSwapchainImage assumes our GPU work is done. Params are
// raw D3D12 interfaces (no access to OpenXRBackend::Impl's private type from a free function).
static void XrClearRtvD3D12(ID3D12CommandAllocator* alloc,
                            ID3D12GraphicsCommandList* list,
                            ID3D12CommandQueue* queue, ID3D12Fence* fence,
                            HANDLE evt, unsigned __int64* fenceVal,
                            ID3D12Resource* img, unsigned __int64 rtvPtr,
                            const float clr[4])
{
    if (!alloc || !list || !queue || !fence || !img || !rtvPtr)
        return;
    alloc->Reset();
    list->Reset(alloc, NULL);
    D3D12_RESOURCE_BARRIER b;
    ZeroMemory(&b, sizeof(b));
    b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    b.Transition.pResource = img;
    b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    b.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
    b.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    list->ResourceBarrier(1, &b);
    D3D12_CPU_DESCRIPTOR_HANDLE rtv;
    rtv.ptr = (SIZE_T)rtvPtr;
    list->OMSetRenderTargets(1, &rtv, FALSE, NULL);
    list->ClearRenderTargetView(rtv, clr, 0, NULL);
    D3D12_RESOURCE_BARRIER b2 = b;
    b2.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    b2.Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
    list->ResourceBarrier(1, &b2);
    list->Close();
    ID3D12CommandList* lists[] = {(ID3D12CommandList*)list};
    queue->ExecuteCommandLists(1, lists);
    (*fenceVal)++;
    queue->Signal(fence, *fenceVal);
    if (fence->GetCompletedValue() < *fenceVal)
    {
        fence->SetEventOnCompletion(*fenceVal, evt);
        WaitForSingleObject(evt, INFINITE);
    }
}
#endif // _WIN32 (XrClearRtvD3D12)

//=============================================================================
// RunFrame
//=============================================================================
// ---------------------------------------------------------------- #107 TURBO MODE: the xrWaitFrame gate
extern bool g_bXrTurboMode;
// #VR: draw a copy of the live desktop cursor on the menu panel (else crosshair).
extern bool g_bVrWindowsCursor;
static void XrWaitThreadFn(OpenXRBackend::Impl* p)
{
    std::unique_lock<std::mutex> lk(p->xrWaitMtx);
    for (;;)
    {
        p->xrWaitCv.wait(lk, [p] { return p->xrWaitPending || p->xrWaitExit; });
        if (p->xrWaitExit)
            return;
        p->xrWaitPending = false;
        p->xrWaitInFlight =
            true; // CLOSE THE RACE WINDOW: pending is cleared but the wait hasn't completed yet --
        // without this a consumer arriving mid-wait saw an "empty" gate and issued a
        // SECOND concurrent xrWaitFrame (two waits per one begin -> the PVR runtime wedges
        // the session -> the TE-entry hang on campaign_wait_for_sim).
        XrFrameWaitInfo fwi = {XR_TYPE_FRAME_WAIT_INFO};
        XrFrameState fs = {XR_TYPE_FRAME_STATE};
        lk.unlock();
        XrResult r = xrWaitFrame(
            p->session, &fwi,
            &fs); // the pacing block -- overlapped with the app's frame
        lk.lock();
        p->xrWaitRes = r;
        p->xrWaitState = fs;
        p->xrWaitInFlight = false;
        p->xrWaitReady = true;
        p->xrWaitCv.notify_all();
    }
}
// Consume the gate: hand out the pre-waited state if one is pending/ready (waiting only for the thread to finish
// if it is mid-wait), else fall through to a plain synchronous xrWaitFrame. EVERY xrWaitFrame call site uses this,
// so a pre-wait requested in the 3D loop is correctly consumed by whichever path begins the next frame.
static XrResult XrWaitGated(OpenXRBackend::Impl* p, XrFrameState* out)
{
    {
        std::unique_lock<std::mutex> lk(p->xrWaitMtx);
        if (p->xrWaitReady || p->xrWaitPending || p->xrWaitInFlight)
        {
            p->xrWaitCv.wait(lk, [p] { return p->xrWaitReady; });
            p->xrWaitReady = false;
            *out = p->xrWaitState;
            return p->xrWaitRes;
        }
    }
    XrFrameWaitInfo fwi = {XR_TYPE_FRAME_WAIT_INFO};
    return xrWaitFrame(p->session, &fwi, out);
}
// Request the NEXT frame's xrWaitFrame on the background thread (turbo only; at most one in flight). Legal right
// after xrBeginFrame: the spec permits wait(N+1) once begin(N) has been called.
static void XrRequestPreWait(OpenXRBackend::Impl* p)
{
    if (!g_bXrTurboMode)
        return;
    std::lock_guard<std::mutex> lk(p->xrWaitMtx);
    if (p->xrWaitPending || p->xrWaitReady || p->xrWaitInFlight)
        return; // at most ONE wait per begin, ever
    if (!p->xrWaitThread.joinable())
        p->xrWaitThread = std::thread(XrWaitThreadFn, p);
    p->xrWaitPending = true;
    p->xrWaitCv.notify_all();
}
static void XrWaitGateStop(OpenXRBackend::Impl* p)
{
    if (!p->xrWaitThread.joinable())
        return;
    {
        std::lock_guard<std::mutex> lk(p->xrWaitMtx);
        p->xrWaitExit = true;
        p->xrWaitCv.notify_all();
    }
    p->xrWaitThread.join();
    p->xrWaitExit = false;
    p->xrWaitPending = false;
    p->xrWaitReady = false;
    p->xrWaitInFlight = false;
}

bool OpenXRBackend::RunFrame(XrEyeRenderFn render, void* user)
{
    Impl* p = m_impl;
    if (!p->instance || p->session == XR_NULL_HANDLE)
        return false;

    PollEvents();
    if (!p->sessionRunning)
        return false;

    XrFrameWaitInfo fwi = {XR_TYPE_FRAME_WAIT_INFO};
    XrFrameState fs = {XR_TYPE_FRAME_STATE};
    if (XR_FAILED(XrWaitGated(p, &fs)))
        return false; // #107 turbo-gated

    XrFrameBeginInfo fbi = {XR_TYPE_FRAME_BEGIN_INFO};
    if (XR_FAILED(xrBeginFrame(p->session, &fbi)))
        return false;

    std::vector<XrCompositionLayerProjectionView> projViews;
    XrCompositionLayerProjection layer = {XR_TYPE_COMPOSITION_LAYER_PROJECTION};
    bool haveLayer = false;

    if (fs.shouldRender)
    {
        // Locate the per-eye views in app space at the predicted display time.
        XrViewState vs = {XR_TYPE_VIEW_STATE};
        XrViewLocateInfo vli = {XR_TYPE_VIEW_LOCATE_INFO};
        vli.viewConfigurationType = p->viewConfigType;
        vli.displayTime = fs.predictedDisplayTime;
        vli.space = p->appSpace;
        uint32_t viewCountOut = 0;
        XrResult lr =
            xrLocateViews(p->session, &vli, &vs, (uint32_t)p->views.size(),
                          &viewCountOut, p->views.data());

        const bool posesValid =
            XR_SUCCEEDED(lr) &&
            (vs.viewStateFlags & XR_VIEW_STATE_ORIENTATION_VALID_BIT) &&
            (vs.viewStateFlags & XR_VIEW_STATE_POSITION_VALID_BIT);

        // Head pose (VIEW space relative to app space) for the cockpit camera feed.
        {
            XrSpaceLocation loc = {XR_TYPE_SPACE_LOCATION};
            if (XR_SUCCEEDED(xrLocateSpace(p->viewSpace, p->appSpace,
                                           fs.predictedDisplayTime, &loc)) &&
                (loc.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) &&
                (loc.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT))
            {
                p->lastHeadPose = loc.pose;
                p->haveHeadPose = true;
            }
        }

        if (posesValid)
        {
            projViews.resize(viewCountOut);
            for (uint32_t e = 0; e < viewCountOut; ++e)
            {
                Impl::Swapchain& sc = p->swapchains[e];

                uint32_t imgIndex = 0;
                XrSwapchainImageAcquireInfo ai = {
                    XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
                if (XR_FAILED(
                        xrAcquireSwapchainImage(sc.handle, &ai, &imgIndex)))
                    continue;

                XrSwapchainImageWaitInfo wi = {
                    XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
                wi.timeout = XR_INFINITE_DURATION;
                if (XR_FAILED(xrWaitSwapchainImage(sc.handle, &wi)))
                {
                    XrSwapchainImageReleaseInfo ri = {
                        XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
                    xrReleaseSwapchainImage(sc.handle, &ri);
                    continue;
                }

                const float clear[4] = {0.05f, 0.10f, 0.18f, 1.0f};
#ifdef _WIN32
                if (p->useD3D12)
                {
                    // #DX12 п.5 Milestone 1: clear the eye image on the D3D12 queue (per-eye scene render = later increment).
                    if (imgIndex < sc.images12.size())
                        XrClearRtvD3D12(p->alloc12, p->list12, p->d3d12Queue,
                                        p->fence12, p->fenceEvt12,
                                        &p->fenceVal12,
                                        sc.images12[imgIndex].texture,
                                        sc.rtvs12[imgIndex], clear);
                }
                else
                {
                    ID3D11RenderTargetView* rtv = sc.rtvs[imgIndex];

                    // Per-eye matrices (milestone 2 uses these; see handedness note above).
                    float proj[16], view[16];
                    XrProjectionToD3D(p->views[e].fov, p->nearZ, p->farZ, proj);
                    XrPoseToView(p->views[e].pose, view);

                    if (render)
                        render(user, (int)e, rtv, sc.width, sc.height, view,
                               proj);
                    else
                        p->ctx->ClearRenderTargetView(
                            rtv, clear); // Milestone 1: just clear
                }
#else
                // #108: RunFrame is the legacy D3D per-eye clear/render path; the Linux Vulkan path drives frames via
                // BeginStereoFrame/PresentMultiviewVulkan instead. Acquire/release with no draw keeps the frame paced.
                (void)clear;
                (void)render;
                (void)user;
#endif

                XrSwapchainImageReleaseInfo ri = {
                    XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
                xrReleaseSwapchainImage(sc.handle, &ri);

                XrCompositionLayerProjectionView& pv = projViews[e];
                pv.type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
                pv.next = NULL;
                pv.pose = p->views[e].pose;
                pv.fov = p->views[e].fov;
                pv.subImage.swapchain = sc.handle;
                pv.subImage.imageRect.offset.x = 0;
                pv.subImage.imageRect.offset.y = 0;
                pv.subImage.imageRect.extent.width = sc.width;
                pv.subImage.imageRect.extent.height = sc.height;
                pv.subImage.imageArrayIndex = 0;
            }

            layer.space = p->appSpace;
            layer.viewCount = (uint32_t)projViews.size();
            layer.views = projViews.data();
            haveLayer = true;
        }
    }

    XrCompositionLayerBaseHeader* layers[1] = {
        (XrCompositionLayerBaseHeader*)&layer};
    XrFrameEndInfo fei = {XR_TYPE_FRAME_END_INFO};
    fei.displayTime = fs.predictedDisplayTime;
    fei.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    fei.layerCount = haveLayer ? 1 : 0;
    fei.layers = haveLayer ? layers : NULL;
    xrEndFrame(p->session, &fei);
    return true;
}

//=============================================================================
// Menu quad swapchain: (re)create at the UI surface size.
//=============================================================================
bool OpenXRBackend::EnsureUiSwapchain(int w, int h)
{
    Impl* p = m_impl;
    // #107 VR-Vulkan: the UI format prefs (uiPref) are DXGI ints; under Vulkan uiFormat stays 0, so reuse the eye
    // swapchain's VkFormat (RGBA -- the 565->RGBA upload matches). D3D paths keep their chosen uiFormat.
    if (p->useVulkan && p->uiFormat == 0)
        p->uiFormat = p->swapchainFormat;
    if (w <= 0 || h <= 0 || p->uiFormat == 0)
        return false;
    if (p->uiSwapchain != XR_NULL_HANDLE && p->uiW == w && p->uiH == h)
        return true;

    if (p->uiSwapchain != XR_NULL_HANDLE)
    {
        xrDestroySwapchain(p->uiSwapchain);
        p->uiSwapchain = XR_NULL_HANDLE;
    }
#ifdef _WIN32
    p->uiImages.clear();
    if (p->uiStaging)
    {
        p->uiStaging->Release();
        p->uiStaging = NULL;
    }
#endif

    XrSwapchainCreateInfo ci = {XR_TYPE_SWAPCHAIN_CREATE_INFO};
    ci.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT |
                    XR_SWAPCHAIN_USAGE_SAMPLED_BIT |
                    XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT;
    ci.format = p->uiFormat;
    ci.sampleCount = 1;
    ci.width = w;
    ci.height = h;
    ci.faceCount = 1;
    ci.arraySize = 1;
    ci.mipCount = 1;
    if (XR_FAILED(xrCreateSwapchain(p->session, &ci, &p->uiSwapchain)))
    {
        XrDbg("OpenXR: UI xrCreateSwapchain failed (%dx%d)\n", w, h);
        return false;
    }

    uint32_t imgCount = 0;
    xrEnumerateSwapchainImages(p->uiSwapchain, 0, &imgCount, NULL);

    if (p->useVulkan) // #107 VR-Vulkan: menu quad swapchain VkImages (RGBA upload target)
    {
        p->uiImagesVk.resize(imgCount);
        for (uint32_t i = 0; i < imgCount; ++i)
        {
            p->uiImagesVk[i].type = XR_TYPE_SWAPCHAIN_IMAGE_VULKAN_KHR;
            p->uiImagesVk[i].next = NULL;
        }
        xrEnumerateSwapchainImages(
            p->uiSwapchain, imgCount, &imgCount,
            (XrSwapchainImageBaseHeader*)p->uiImagesVk.data());
    }
#ifdef _WIN32
    else if (
        p->useD3D12) // #DX12 п.5: D3D12 UI images + an UPLOAD buffer (256-aligned rows) for the 565->RGBA copy
    {
        p->uiImages12.resize(imgCount);
        for (uint32_t i = 0; i < imgCount; ++i)
        {
            p->uiImages12[i].type = XR_TYPE_SWAPCHAIN_IMAGE_D3D12_KHR;
            p->uiImages12[i].next = NULL;
        }
        xrEnumerateSwapchainImages(
            p->uiSwapchain, imgCount, &imgCount,
            (XrSwapchainImageBaseHeader*)p->uiImages12.data());

        if (p->uiUpload12)
        {
            p->uiUpload12->Release();
            p->uiUpload12 = NULL;
        }
        p->uiRowPitch12 = (unsigned)(((w * 4) + 255) & ~255);
        D3D12_HEAP_PROPERTIES hpUp;
        ZeroMemory(&hpUp, sizeof(hpUp));
        hpUp.Type = D3D12_HEAP_TYPE_UPLOAD;
        D3D12_RESOURCE_DESC bd;
        ZeroMemory(&bd, sizeof(bd));
        bd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        bd.Width = (UINT64)p->uiRowPitch12 * h;
        bd.Height = 1;
        bd.DepthOrArraySize = 1;
        bd.MipLevels = 1;
        bd.Format = DXGI_FORMAT_UNKNOWN;
        bd.SampleDesc.Count = 1;
        bd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        if (FAILED(p->d3d12Device->CreateCommittedResource(
                &hpUp, D3D12_HEAP_FLAG_NONE, &bd,
                D3D12_RESOURCE_STATE_GENERIC_READ, NULL,
                IID_PPV_ARGS(&p->uiUpload12))))
        {
            XrDbg("OpenXR: UI upload buffer create failed\n");
            return false;
        }
    }
    else
    {
        p->uiImages.resize(imgCount);
        for (uint32_t i = 0; i < imgCount; ++i)
        {
            p->uiImages[i].type = XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR;
            p->uiImages[i].next = NULL;
        }
        xrEnumerateSwapchainImages(
            p->uiSwapchain, imgCount, &imgCount,
            (XrSwapchainImageBaseHeader*)p->uiImages.data());

        D3D11_TEXTURE2D_DESC td;
        ZeroMemory(&td, sizeof(td));
        td.Width = w;
        td.Height = h;
        td.MipLevels = 1;
        td.ArraySize = 1;
        td.Format =
            DXGI_FORMAT_R8G8B8A8_UNORM; // staging is UNORM; copy-compatible with the _SRGB swapchain
        td.SampleDesc.Count = 1;
        td.Usage = D3D11_USAGE_STAGING;
        td.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        if (FAILED(p->device->CreateTexture2D(&td, NULL, &p->uiStaging)))
        {
            XrDbg("OpenXR: UI staging texture create failed\n");
            return false;
        }
    }
#endif // _WIN32 (D3D12 / D3D11 UI swapchain images)

    p->uiW = w;
    p->uiH = h;
    XrDbg("OpenXR: menu quad swapchain %dx%d\n", w, h);
    return true;
}

#ifdef _WIN32
// #DX12 п.5: copy a CPU RGBA8 image (w*h, tightly packed OR the UI's row pitch) into an acquired D3D12 UI
// swapchain image: fill the UPLOAD buffer (256-aligned rows), CopyTextureRegion (COMMON->COPY_DEST->COMMON),
// execute + fence. Returns after the copy is complete (the runtime releases the image right after).
static void XrCopyRgbaToUiImageD3D12(
    ID3D12Resource* upload, unsigned rowPitch, ID3D12CommandAllocator* alloc,
    ID3D12GraphicsCommandList* list, ID3D12CommandQueue* queue,
    ID3D12Fence* fence, HANDLE evt, unsigned __int64* fenceVal,
    ID3D12Resource* dstImg, const void* rgbaRows, unsigned srcRowPitch, int w,
    int h)
{
    if (!upload || !dstImg || !list)
        return;
    unsigned char* mapped = 0;
    D3D12_RANGE noRead;
    noRead.Begin = 0;
    noRead.End = 0;
    if (FAILED(upload->Map(0, &noRead, (void**)&mapped)) || !mapped)
        return;
    unsigned copyBytes = (unsigned)w * 4;
    for (int y = 0; y < h; ++y)
        memcpy(mapped + (size_t)y * rowPitch,
               (const unsigned char*)rgbaRows + (size_t)y * srcRowPitch,
               copyBytes);
    upload->Unmap(0, NULL);

    alloc->Reset();
    list->Reset(alloc, NULL);
    D3D12_RESOURCE_BARRIER b;
    ZeroMemory(&b, sizeof(b));
    b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    b.Transition.pResource = dstImg;
    b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    b.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
    b.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
    list->ResourceBarrier(1, &b);
    D3D12_TEXTURE_COPY_LOCATION dl;
    ZeroMemory(&dl, sizeof(dl));
    dl.pResource = dstImg;
    dl.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dl.SubresourceIndex = 0;
    D3D12_TEXTURE_COPY_LOCATION sl;
    ZeroMemory(&sl, sizeof(sl));
    sl.pResource = upload;
    sl.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    sl.PlacedFootprint.Offset = 0;
    sl.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sl.PlacedFootprint.Footprint.Width = (UINT)w;
    sl.PlacedFootprint.Footprint.Height = (UINT)h;
    sl.PlacedFootprint.Footprint.Depth = 1;
    sl.PlacedFootprint.Footprint.RowPitch = rowPitch;
    list->CopyTextureRegion(&dl, 0, 0, 0, &sl, NULL);
    D3D12_RESOURCE_BARRIER b2 = b;
    b2.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    b2.Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
    list->ResourceBarrier(1, &b2);
    list->Close();
    ID3D12CommandList* lists[] = {(ID3D12CommandList*)list};
    queue->ExecuteCommandLists(1, lists);
    (*fenceVal)++;
    queue->Signal(fence, *fenceVal);
    if (fence->GetCompletedValue() < *fenceVal)
    {
        fence->SetEventOnCompletion(*fenceVal, evt);
        WaitForSingleObject(evt, INFINITE);
    }
}

// #DX12 п.5 A1: copy a same-size D3D12 texture (the in-scene menu RTT, already rendered on the eye list which has
// executed) straight into an acquired D3D12 UI swapchain image, then execute + fence. srcState is the menu RTT's
// current state (RENDER_TARGET after BindMenuRtt) -- transitioned to COPY_SOURCE and back; the UI image is COMMON
// on acquire (COMMON->COPY_DEST->COMMON, same contract as XrCopyRgbaToUiImageD3D12). Formats are RGBA8 (UNORM vs
// _SRGB are copy-compatible in the same family), sizes match (menu RTT and UI swapchain both DispWidth x DispHeight).
static void XrCopyD3D12TexToUiImage(ID3D12CommandAllocator* alloc,
                                    ID3D12GraphicsCommandList* list,
                                    ID3D12CommandQueue* queue,
                                    ID3D12Fence* fence, HANDLE evt,
                                    unsigned __int64* fenceVal,
                                    ID3D12Resource* srcRes, unsigned srcState,
                                    ID3D12Resource* dstImg)
{
    if (!alloc || !list || !queue || !srcRes || !dstImg)
        return;
    alloc->Reset();
    list->Reset(alloc, NULL);
    D3D12_RESOURCE_BARRIER b[2];
    ZeroMemory(b, sizeof(b));
    b[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    b[0].Transition.pResource = srcRes;
    b[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    b[0].Transition.StateBefore = (D3D12_RESOURCE_STATES)srcState;
    b[0].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
    b[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    b[1].Transition.pResource = dstImg;
    b[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    b[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
    b[1].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
    list->ResourceBarrier(2, b);
    list->CopyResource(dstImg, srcRes);
    b[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
    b[0].Transition.StateAfter = (D3D12_RESOURCE_STATES)srcState;
    b[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    b[1].Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
    list->ResourceBarrier(2, b);
    list->Close();
    ID3D12CommandList* lists[] = {(ID3D12CommandList*)list};
    queue->ExecuteCommandLists(1, lists);
    (*fenceVal)++;
    queue->Signal(fence, *fenceVal);
    if (fence->GetCompletedValue() < *fenceVal)
    {
        fence->SetEventOnCompletion(*fenceVal, evt);
        WaitForSingleObject(evt, INFINITE);
    }
}
#endif // _WIN32 (D3D12 UI copy helpers)

//=============================================================================
// Artscout - 2026 (VR menu): the 565 source (g_pXrMenuSurface565 == an ImageBuffer's m_pSysMem) can be
// freed/resized by the UI/sim thread WHILE this main-thread pump reads it -- F4IsBadReadPtr only samples
// one instant, so a race still dangles the pointer MID-LOOP (0xC0000005 in the convert loop). Do the read
// under SEH so a stray access aborts to a clear frame instead of killing the process. POD-only locals ->
// no C++ object unwinding, so __try/__except is legal in this standalone helper.
//=============================================================================
static bool XrConvertMenu565ToRGBA(const void* src565, int srcW, int srcH,
                                   void* dstBase, unsigned rowPitch)
{
#ifdef _WIN32
    __try
    {
#endif
        const unsigned short* s = (const unsigned short*)src565;
        for (int y = 0; y < srcH; ++y)
        {
            unsigned char* d = (unsigned char*)dstBase + (size_t)y * rowPitch;
            const unsigned short* srow = s + (size_t)y * srcW;
            for (int x = 0; x < srcW; ++x)
            {
                unsigned short px = srow[x];
                unsigned r = (px >> 11) & 0x1F, g = (px >> 5) & 0x3F,
                         b = px & 0x1F;
                unsigned char* o = d + (size_t)x * 4;
                o[0] = (unsigned char)((r * 255 + 15) / 31);
                o[1] = (unsigned char)((g * 255 + 31) / 63);
                o[2] = (unsigned char)((b * 255 + 15) / 31);
                o[3] = 255;
            }
        }
        return true;
#ifdef _WIN32
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false; // source went away mid-read -> caller drops the panel for this frame
    }
#else
    // #108: Linux/clang has no SEH. The cross-thread dangling-pointer guard is Windows-only; do the plain conversion.
#endif
}

//=============================================================================
// RunMenuFrame -- present the flat 2D UI as a head-locked quad panel.
//=============================================================================
bool OpenXRBackend::RunMenuFrame(const void* src565, int srcW, int srcH)
{
#ifndef _WIN32
    // #108: RunMenuFrame is the D3D (D3D11/D3D12) menu-quad path. On Linux the Vulkan peer RunVulkanMenuFrame is used
    // (called by OpenXR_PumpFrame), so this is a no-op here.
    (void)src565;
    (void)srcW;
    (void)srcH;
    return false;
#else
    Impl* p = m_impl;
    if (!p->instance || p->session == XR_NULL_HANDLE)
        return false;

    // #107 VR-Vulkan (Windows): the D3D12/D3D11 menu paths below assume a D3D context (p->ctx / p->uiStaging), which is
    // NULL on the Vulkan backend -> the D3D11 fallback crashed (p->ctx->Map on NULL). Redirect to the Vulkan menu peer
    // (mirrors the Linux build). Covers EVERY caller (handle_WinMain / imagebuf) so a Vulkan session never hits the D3D path.
    if (p->useVulkan)
        return RunVulkanMenuFrame(src565, srcW, srcH);

    // #DX12 п.5: D3D12 VR menu panel -- convert the 565 UI into an RGBA buffer (+ cursor crosshair), copy it
    // into the D3D12 UI swapchain image, and submit it as a world-fixed quad. Self-contained (the D3D11 path
    // below is untouched). Same panel placement / cursor as D3D11.
    if (p->useD3D12)
    {
        PollEvents();
        if (!p->sessionRunning)
            return false;
        if (!EnsureUiSwapchain(srcW, srcH))
            return RunFrame(NULL, NULL);

        XrFrameWaitInfo fwi = {XR_TYPE_FRAME_WAIT_INFO};
        XrFrameState fs = {XR_TYPE_FRAME_STATE};
        if (XR_FAILED(XrWaitGated(p, &fs)))
            return false; // #107 turbo-gated
        XrFrameBeginInfo fbi = {XR_TYPE_FRAME_BEGIN_INFO};
        if (XR_FAILED(xrBeginFrame(p->session, &fbi)))
            return false;

        XrCompositionLayerQuad quad = {XR_TYPE_COMPOSITION_LAYER_QUAD};
        bool haveLayer = false;
        (void)src565;
        const unsigned char* srcSnap = XrMenuSnapshot(srcW, srcH);
        if (fs.shouldRender && srcSnap && srcW > 0 && srcH > 0)
        {
            unsigned char* rgba =
                (unsigned char*)malloc((size_t)srcW * srcH * 4);
            if (rgba && XrConvertMenu565ToRGBA(srcSnap, srcW, srcH, rgba,
                                               (unsigned)srcW * 4))
            {
                // VR cursor crosshair at the real pointer, mapped window-client -> panel pixels.
                HWND hwnd = g_pD3D12Backend ? g_pD3D12Backend->Hwnd() : NULL;
                POINT pt;
                RECT rc;
                if (hwnd && GetCursorPos(&pt) && GetClientRect(hwnd, &rc))
                {
                    ScreenToClient(hwnd, &pt);
                    const int cw = rc.right - rc.left, ch = rc.bottom - rc.top;
                    if (cw > 0 && ch > 0)
                    {
                        const int cx = (int)((long long)pt.x * srcW / cw),
                                  cy = (int)((long long)pt.y * srcH / ch),
                                  arm = 9;
#define XR12_SETPX(X, Y, V)                                                    \
    do                                                                         \
    {                                                                          \
        int _x = (X), _y = (Y);                                                \
        if (_x >= 0 && _x < srcW && _y >= 0 && _y < srcH)                      \
        {                                                                      \
            unsigned char* _o = rgba + ((size_t)_y * srcW + _x) * 4;           \
            _o[0] = _o[1] = _o[2] = (unsigned char)(V);                        \
            _o[3] = 255;                                                       \
        }                                                                      \
    } while (0)
                        // Artscout - 2026: draw the REAL desktop cursor; the
                        // crosshair stays as the fallback.
                        if (!g_bVrWindowsCursor ||
                            !FF_BlitWinCursorRGBA(rgba, srcW, srcH, cx, cy))
                        {
                            for (int d = -arm; d <= arm; ++d)
                            {
                                XR12_SETPX(cx + d, cy - 1, 0);
                                XR12_SETPX(cx + d, cy + 1, 0);
                                XR12_SETPX(cx - 1, cy + d, 0);
                                XR12_SETPX(cx + 1, cy + d, 0);
                            }
                            for (int d = -arm; d <= arm; ++d)
                            {
                                XR12_SETPX(cx + d, cy, 255);
                                XR12_SETPX(cx, cy + d, 255);
                            }
                        }
#undef XR12_SETPX
                    }
                }

                uint32_t idx = 0;
                XrSwapchainImageAcquireInfo ai = {
                    XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
                if (XR_SUCCEEDED(
                        xrAcquireSwapchainImage(p->uiSwapchain, &ai, &idx)))
                {
                    XrSwapchainImageWaitInfo wi = {
                        XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
                    wi.timeout = XR_INFINITE_DURATION;
                    if (XR_SUCCEEDED(
                            xrWaitSwapchainImage(p->uiSwapchain, &wi)) &&
                        idx < p->uiImages12.size())
                        XrCopyRgbaToUiImageD3D12(
                            p->uiUpload12, p->uiRowPitch12, p->alloc12,
                            p->list12, p->d3d12Queue, p->fence12, p->fenceEvt12,
                            &p->fenceVal12, p->uiImages12[idx].texture, rgba,
                            (unsigned)srcW * 4, srcW, srcH);
                    XrSwapchainImageReleaseInfo ri = {
                        XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
                    xrReleaseSwapchainImage(p->uiSwapchain, &ri);

                    const float aspect = (float)srcW / (float)srcH,
                                heightM = 1.3f, distM = 2.1f;
                    quad.layerFlags = 0;
                    quad.space = p->appSpace;
                    quad.eyeVisibility = XR_EYE_VISIBILITY_BOTH;
                    quad.subImage.swapchain = p->uiSwapchain;
                    quad.subImage.imageRect.offset.x = 0;
                    quad.subImage.imageRect.offset.y = 0;
                    quad.subImage.imageRect.extent.width = srcW;
                    quad.subImage.imageRect.extent.height = srcH;
                    quad.subImage.imageArrayIndex = 0;
                    quad.pose.orientation.x = quad.pose.orientation.y =
                        quad.pose.orientation.z = 0.0f;
                    quad.pose.orientation.w = 1.0f;
                    quad.pose.position.x = 0.0f;
                    quad.pose.position.y = 0.0f;
                    quad.pose.position.z = -distM;
                    quad.size.width = heightM * aspect;
                    quad.size.height = heightM;
                    haveLayer = true;
                }
            }
            if (rgba)
                free(rgba);
        }

        XrCompositionLayerBaseHeader* layers[1] = {
            (XrCompositionLayerBaseHeader*)&quad};
        XrFrameEndInfo fei = {XR_TYPE_FRAME_END_INFO};
        fei.displayTime = fs.predictedDisplayTime;
        fei.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
        fei.layerCount = haveLayer ? 1 : 0;
        fei.layers = haveLayer ? layers : NULL;
        xrEndFrame(p->session, &fei);
        return true;
    }

    PollEvents();
    if (!p->sessionRunning)
        return false;

    // If a panel can't be created (no R8G8B8A8 format), keep timing alive with a clear.
    if (!EnsureUiSwapchain(srcW, srcH))
    {
        return RunFrame(NULL, NULL);
    }

    XrFrameWaitInfo fwi = {XR_TYPE_FRAME_WAIT_INFO};
    XrFrameState fs = {XR_TYPE_FRAME_STATE};
    if (XR_FAILED(XrWaitGated(p, &fs)))
        return false; // #107 turbo-gated
    XrFrameBeginInfo fbi = {XR_TYPE_FRAME_BEGIN_INFO};
    if (XR_FAILED(xrBeginFrame(p->session, &fbi)))
        return false;

    XrCompositionLayerQuad quad = {XR_TYPE_COMPOSITION_LAYER_QUAD};
    bool haveLayer = false;

    // Artscout - 2026: read a STABLE pump-local snapshot of the menu surface (the producer copied it under
    // the lock in OpenXR_CacheMenuSurface), NOT the live ImageBuffer m_pSysMem -- so the convert can never
    // touch a freed/resized buffer (was the 0xC0000005 race). src565 is ignored in favour of the snapshot.
    (void)src565;
    const unsigned char* srcSnap = XrMenuSnapshot(srcW, srcH);
    const bool srcReadable = (srcSnap != NULL);

    if (fs.shouldRender && srcReadable)
    {
        // 565 -> RGBA8 into the staging texture (same conversion as BlitBitmap565).
        D3D11_MAPPED_SUBRESOURCE mr;
        if (SUCCEEDED(p->ctx->Map(p->uiStaging, 0, D3D11_MAP_WRITE, 0, &mr)))
        {
            // SEH-guarded 565->RGBA8 read (the source can be freed by another thread mid-loop).
            const bool convOk = XrConvertMenu565ToRGBA(srcSnap, srcW, srcH,
                                                       mr.pData, mr.RowPitch);
            if (!convOk)
            {
                p->ctx->Unmap(p->uiStaging, 0);
            }
            else
            {

                // VR cursor: the flat UI uses the OS hardware cursor (not in m_pSysMem), so
                // it would be invisible on the panel. Draw a crosshair (white core + black
                // outline) at the real pointer position, mapped from window-client pixels to
                // panel pixels. Mouse clicks still land via the desktop window at that spot.
                if (g_pD3D12Backend &&
                    g_pD3D12Backend
                        ->Hwnd()) // Artscout - 2026 (D3D11 purge): HWND from the D3D12 backend
                {
                    POINT pt;
                    HWND hwnd = g_pD3D12Backend->Hwnd();
                    RECT rc;
                    if (GetCursorPos(&pt) && GetClientRect(hwnd, &rc))
                    {
                        ScreenToClient(hwnd, &pt);
                        const int cw = rc.right - rc.left,
                                  ch = rc.bottom - rc.top;
                        if (cw > 0 && ch > 0)
                        {
                            const int cx = (int)((long long)pt.x * srcW / cw);
                            const int cy = (int)((long long)pt.y * srcH / ch);
                            unsigned char* base = (unsigned char*)mr.pData;
                            const int arm = 9;
#define XR_SETPX(X, Y, V)                                                      \
    do                                                                         \
    {                                                                          \
        int _x = (X), _y = (Y);                                                \
        if (_x >= 0 && _x < srcW && _y >= 0 && _y < srcH)                      \
        {                                                                      \
            unsigned char* _o =                                                \
                base + (size_t)_y * mr.RowPitch + (size_t)_x * 4;              \
            _o[0] = _o[1] = _o[2] = (unsigned char)(V);                        \
            _o[3] = 255;                                                       \
        }                                                                      \
    } while (0)
                            // Artscout - 2026: the REAL desktop cursor; the
                            // crosshair stays as the fallback.
                            if (!g_bVrWindowsCursor ||
                                !FF_BlitWinCursorRGBA(base, srcW, srcH, cx, cy,
                                                      (int)mr.RowPitch))
                            {
                                for (int d = -arm; d <= arm;
                                     ++d) // black outline first
                                {
                                    XR_SETPX(cx + d, cy - 1, 0);
                                    XR_SETPX(cx + d, cy + 1, 0);
                                    XR_SETPX(cx - 1, cy + d, 0);
                                    XR_SETPX(cx + 1, cy + d, 0);
                                }
                                for (int d = -arm; d <= arm;
                                     ++d) // white core on top
                                {
                                    XR_SETPX(cx + d, cy, 255);
                                    XR_SETPX(cx, cy + d, 255);
                                }
                            }
#undef XR_SETPX
                        }
                    }
                }

                p->ctx->Unmap(p->uiStaging, 0);

                uint32_t idx = 0;
                XrSwapchainImageAcquireInfo ai = {
                    XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
                if (XR_SUCCEEDED(
                        xrAcquireSwapchainImage(p->uiSwapchain, &ai, &idx)))
                {
                    XrSwapchainImageWaitInfo wi = {
                        XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
                    wi.timeout = XR_INFINITE_DURATION;
                    if (XR_SUCCEEDED(xrWaitSwapchainImage(p->uiSwapchain, &wi)))
                        p->ctx->CopyResource(p->uiImages[idx].texture,
                                             p->uiStaging);
                    XrSwapchainImageReleaseInfo ri = {
                        XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
                    xrReleaseSwapchainImage(p->uiSwapchain, &ri);

                    // World-fixed panel: placed in the app (LOCAL/recenter) space in front of
                    // the headset's recenter origin, so it stays put when you look around
                    // (does NOT follow the gaze, like the DCS 2D menu in VR).
                    const float aspect = (float)srcW / (float)srcH;
                    const float heightM = 1.3f; // panel height in meters
                    const float distM =
                        2.1f; // distance forward (-Z); ~30cm further than before
                    quad.layerFlags = 0;
                    quad.space = p->appSpace;
                    quad.eyeVisibility = XR_EYE_VISIBILITY_BOTH;
                    quad.subImage.swapchain = p->uiSwapchain;
                    quad.subImage.imageRect.offset.x = 0;
                    quad.subImage.imageRect.offset.y = 0;
                    quad.subImage.imageRect.extent.width = srcW;
                    quad.subImage.imageRect.extent.height = srcH;
                    quad.subImage.imageArrayIndex = 0;
                    quad.pose.orientation.x = quad.pose.orientation.y =
                        quad.pose.orientation.z = 0.0f;
                    quad.pose.orientation.w = 1.0f;
                    quad.pose.position.x = 0.0f;
                    quad.pose.position.y = 0.0f;
                    quad.pose.position.z = -distM;
                    quad.size.width = heightM * aspect;
                    quad.size.height = heightM;
                    haveLayer = true;
                }
            } // Artscout - 2026: close the convOk 'else' (SEH-guarded 565 read succeeded)
        }
    }


    XrCompositionLayerBaseHeader* layers[1] = {
        (XrCompositionLayerBaseHeader*)&quad};
    XrFrameEndInfo fei = {XR_TYPE_FRAME_END_INFO};
    fei.displayTime = fs.predictedDisplayTime;
    fei.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    fei.layerCount = haveLayer ? 1 : 0;
    fei.layers = haveLayer ? layers : NULL;
    xrEndFrame(p->session, &fei);
    return true;
#endif // _WIN32
}

//=============================================================================
// Per-eye stereo frame (the 3D scene is rendered once per eye)
//=============================================================================
bool OpenXRBackend::StereoActive() const
{
    return m_impl->inStereoFrame;
}
void OpenXRBackend::SetCurrentEye(int eye)
{
    m_impl->currentEye = eye;
}
int OpenXRBackend::CurrentEye() const
{
    return m_impl->currentEye;
}
int OpenXRBackend::CurEyeW() const
{
    int e = m_impl->currentEye;
    return (e >= 0 && e < (int)m_impl->swapchains.size()) ?
               m_impl->swapchains[e].width :
               0;
}
int OpenXRBackend::CurEyeH() const
{
    int e = m_impl->currentEye;
    return (e >= 0 && e < (int)m_impl->swapchains.size()) ?
               m_impl->swapchains[e].height :
               0;
}
float OpenXRBackend::GetEyeLateralOffsetFeet(int eye) const
{
    if (eye < 0 || eye >= 8)
        return 0.0f;
    return m_impl->eyeLatFeet[eye];
}

// Artscout - 2026: per-eye fov half-angles (radians) from the runtime's located views. Used to set
// the engine's render FOV to the headset's actual per-eye FOV (so the world fills the lenses and the
// rendered image matches the submitted projection -> no double vision).
bool OpenXRBackend::GetEyeFovAngles(int eye, float* l, float* r, float* u,
                                    float* d) const
{
    Impl* p = m_impl;
    if (eye < 0 || eye >= (int)p->views.size())
        return false;
    if (l)
        *l = p->views[eye].fov.angleLeft;
    if (r)
        *r = p->views[eye].fov.angleRight;
    if (u)
        *u = p->views[eye].fov.angleUp;
    if (d)
        *d = p->views[eye].fov.angleDown;
    return true;
}

void OpenXRBackend::SetSubmitFov(float h, float v)
{
    m_impl->submitFov.angleLeft = -h * 0.5f;
    m_impl->submitFov.angleRight = h * 0.5f;
    m_impl->submitFov.angleUp = v * 0.5f;
    m_impl->submitFov.angleDown = -v * 0.5f;
    m_impl->haveSubmitFov = true;
}

void OpenXRBackend::ClearSubmitFov()
{
    m_impl->haveSubmitFov = false;
}

// Returns: -1 = session not running (don't drive XR; render mono);
//           0 = frame begun but shouldRender false (render nothing; EndStereoFrame still required);
//           n = render n eyes.
int OpenXRBackend::BeginStereoFrame()
{
    Impl* p = m_impl;
    if (!p->instance || p->session == XR_NULL_HANDLE)
        return -1;
    PollEvents();
    if (!p->sessionRunning)
        return -1;

    XrFrameWaitInfo fwi = {XR_TYPE_FRAME_WAIT_INFO};
    p->stereoFrameState.type = XR_TYPE_FRAME_STATE;
    p->stereoFrameState.next = NULL;
    // #107 PERF stage 2c: xrWaitFrame is the runtime's FRAME PACING gate -- it BLOCKS until the compositor wants the
    // next frame. It sits inside the PreVR(CPU) window, so time it separately: if it accounts for most of PreVR, the
    // frame is pacing-limited (locked to the HMD interval), not CPU-bound there, and the honest CPU cost is the rest.
    {
        extern bool g_bVulkanProfile;
        extern void FrameProf_XrWaitFrame(double ms);
        std::chrono::steady_clock::time_point _xw0;
        if (g_bVulkanProfile)
            _xw0 = std::chrono::steady_clock::now();
        if (XR_FAILED(XrWaitGated(p, &p->stereoFrameState)))
            return -1; // #107 turbo-gated (consume pre-wait)
        if (g_bVulkanProfile)
            FrameProf_XrWaitFrame(std::chrono::duration<double, std::milli>(
                                      std::chrono::steady_clock::now() - _xw0)
                                      .count());
    }
    XrFrameBeginInfo fbi = {XR_TYPE_FRAME_BEGIN_INFO};
    if (XR_FAILED(xrBeginFrame(p->session, &fbi)))
    {
        p->xrTurboWarmup = 0;
        return -1;
    } // failed begin -> re-warm turbo
    // #107 turbo: absorb the NEXT frame's pacing block on the gate thread, overlapped with this frame. Warm up with
    // synchronous waits for the first frames after (re)entry -- the PVR runtime raced the earliest turbo frames.
    if (++p->xrTurboWarmup > 5)
        XrRequestPreWait(p);
    p->inStereoFrame = true;
    p->projViews.clear();
    p->menuQuadPending =
        false; // Artscout - 2026 (#59): no stale menu quad carried into a new frame
    p->fpsQuadPending = false;
    p->subQuadPending = false; // #DX12 п.5: same for the FPS quad
    p->subQuadPending = false; // #59: same for the subtitle quad

    // Artscout - 2026 (#67): apply a pending recenter here -- the render thread owns appSpace and we have a
    // fresh predicted time, so this is safe (no destroy-while-in-use). Rebuild the LOCAL app space at the
    // current head yaw + position; pitch/roll dropped (level horizon). Done BEFORE the view locate below so
    // this very frame renders recentered.
    if (p->recenterPending)
    {
        p->recenterPending = false;
        XrSpaceLocation rloc = {XR_TYPE_SPACE_LOCATION};
        // Locate the head in the PRISTINE LOCAL frame (localRef), NOT the current appSpace. The new space is
        // built as LOCAL + off, so off must be the head pose in LOCAL. Measuring against the already-recentered
        // appSpace made off ~0 on the 2nd press -> newSpace ~= LOCAL -> the view snapped back to the un-recentered
        // origin (the "second press undoes it" toggle). With localRef every press is an absolute recenter
        // (height + depth + yaw) to wherever the head currently is.
        if (XR_SUCCEEDED(xrLocateSpace(p->viewSpace, p->localRef,
                                       p->stereoFrameState.predictedDisplayTime,
                                       &rloc)) &&
            (rloc.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) &&
            (rloc.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT))
        {
            XrPosef off;
            off.position =
                rloc.pose
                    .position; // origin under the head -> centers + resets eye height
            float qy = rloc.pose.orientation.y, qw = rloc.pose.orientation.w;
            float nrm =
                sqrtf(qy * qy +
                      qw * qw); // yaw-only (swing-twist about Y, OpenXR Y-up)
            if (nrm < 1e-6f)
            {
                off.orientation.x = off.orientation.y = off.orientation.z =
                    0.0f;
                off.orientation.w = 1.0f;
            }
            else
            {
                off.orientation.x = 0.0f;
                off.orientation.y = qy / nrm;
                off.orientation.z = 0.0f;
                off.orientation.w = qw / nrm;
            }

            XrReferenceSpaceCreateInfo rsci = {
                XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
            rsci.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
            rsci.poseInReferenceSpace = off;
            XrSpace newSpace = XR_NULL_HANDLE;
            if (XR_SUCCEEDED(
                    xrCreateReferenceSpace(p->session, &rsci, &newSpace)) &&
                newSpace != XR_NULL_HANDLE)
            {
                xrDestroySpace(p->appSpace);
                p->appSpace = newSpace;
                XrDbg("OpenXR: recenter applied (pos %.2f %.2f %.2f)\n",
                      off.position.x, off.position.y, off.position.z);
            }
        }
    }

    // Artscout - 2026 (VR controllers, Phase 1): sync controller input for this frame (poses + buttons).
    SyncControllers();

    if (!p->stereoFrameState.shouldRender)
        return 0;

    XrViewState vs = {XR_TYPE_VIEW_STATE};
    XrViewLocateInfo vli = {XR_TYPE_VIEW_LOCATE_INFO};
    vli.viewConfigurationType = p->viewConfigType;
    vli.displayTime = p->stereoFrameState.predictedDisplayTime;
    vli.space = p->appSpace;
    uint32_t out = 0;
    if (XR_FAILED(xrLocateViews(p->session, &vli, &vs,
                                (uint32_t)p->views.size(), &out,
                                p->views.data())) ||
        !(vs.viewStateFlags & XR_VIEW_STATE_ORIENTATION_VALID_BIT))
        return 0;

    const int n = (int)out;

    // Head pose -> look angles for the cockpit camera.
    XrSpaceLocation loc = {XR_TYPE_SPACE_LOCATION};
    if (XR_SUCCEEDED(xrLocateSpace(p->viewSpace, p->appSpace,
                                   p->stereoFrameState.predictedDisplayTime,
                                   &loc)) &&
        (loc.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT))
    {
        p->lastHeadPose = loc.pose;
        p->haveHeadPose = true;
        XrQuatToYawPitchRoll(loc.pose.orientation, p->lastYaw, p->lastPitch,
                             p->lastRoll);
        p->haveHeadAngles = true;
    }

    // Per-eye lateral offset (IPD), meters -> feet, relative to the head centre.
    // Artscout - 2026: appSpace X decays as cos(yaw) and CHANGES SIGN past 90
    // degrees (eyes swap); the head-frame value below is the knob's answer.
    float cx = 0.0f, cy = 0.0f, cz = 0.0f;
    for (int e = 0; e < n; ++e)
    {
        cx += p->views[e].pose.position.x;
        cy += p->views[e].pose.position.y;
        cz += p->views[e].pose.position.z;
    }
    if (n)
    {
        cx /= (float)n;
        cy /= (float)n;
        cz /= (float)n;
    }

    // Conjugate of the head orientation: appSpace delta -> head frame.
    XrQuaternionf hq = {0.0f, 0.0f, 0.0f, 1.0f};
    if (p->haveHeadPose)
        hq = p->lastHeadPose.orientation;
    const float hcx = -hq.x, hcy = -hq.y, hcz = -hq.z, hcw = hq.w;

    // Artscout - 2026: knob, because the two render paths disagree on the axis
    // they spend it along (multiview = seat, per-eye = head).
    extern bool g_bVrEyeLatHeadFrame;
    float latHead[8] = {0.0f};

    for (int e = 0; e < n && e < 8; ++e)
    {
        const float dx = p->views[e].pose.position.x - cx;
        const float dy = p->views[e].pose.position.y - cy;
        const float dz = p->views[e].pose.position.z - cz;

        // v' = v + w*t + qv x t, with t = 2*(qv x v). Only x is wanted.
        const float tx = 2.0f * (hcy * dz - hcz * dy);
        const float ty = 2.0f * (hcz * dx - hcx * dz);
        const float tz = 2.0f * (hcx * dy - hcy * dx);

        latHead[e] = (dx + hcw * tx + (hcy * tz - hcz * ty)) * 3.28084f;
        p->eyeLatFeet[e] = g_bVrEyeLatHeadFrame ? latHead[e] : (dx * 3.28084f);
    }

    // Artscout - 2026: eye geometry ON A CLOCK -- head-frame offset next to the
    // raw appSpace one, so a swap reads straight off the pair.
    {
        static std::chrono::steady_clock::time_point s_last;
        static bool s_first = true;
        const std::chrono::steady_clock::time_point now =
            std::chrono::steady_clock::now();

        if (s_first ||
            std::chrono::duration_cast<std::chrono::milliseconds>(now - s_last)
                    .count() >= 3000)
        {
            s_first = false;
            s_last = now;

            const float rtd = 57.29578f;
            XrDbg("OpenXR: [eyegeom] views=%d head yaw/pit/rol %.1f %.1f %.1f\n",
                  n, p->lastYaw * rtd, p->lastPitch * rtd, p->lastRoll * rtd);

            for (int e = 0; e < n && e < 8; ++e)
            {
                const XrFovf& f = p->views[e].fov;
                const XrVector3f& pos = p->views[e].pose.position;
                const XrQuaternionf& q = p->views[e].pose.orientation;

                XrDbg("OpenXR: [eyegeom] v%d fov L%.2f R%.2f U%.2f D%.2f deg\n",
                      e, f.angleLeft * rtd, f.angleRight * rtd,
                      f.angleUp * rtd, f.angleDown * rtd);
                XrDbg("OpenXR: [eyegeom] v%d pos %.4f %.4f %.4f m\n", e, pos.x,
                      pos.y, pos.z);
                XrDbg("OpenXR: [eyegeom] v%d quat %.4f %.4f %.4f %.4f\n", e,
                      q.x, q.y, q.z, q.w);
                XrDbg("OpenXR: [eyegeom] v%d lat head %.4f appX %.4f used "
                      "%.4f ft\n",
                      e, latHead[e],
                      (p->views[e].pose.position.x - cx) * 3.28084f,
                      p->eyeLatFeet[e]);
            }

            if (n >= 2)
            {
                const float ix =
                    p->views[1].pose.position.x - p->views[0].pose.position.x;
                const float iy =
                    p->views[1].pose.position.y - p->views[0].pose.position.y;
                const float iz =
                    p->views[1].pose.position.z - p->views[0].pose.position.z;

                XrDbg("OpenXR: [eyegeom] ipd %.4f m submitFov=%d quad=%d\n",
                      sqrtf(ix * ix + iy * iy + iz * iz),
                      (int)p->haveSubmitFov, (int)(n > 2));

                if (p->haveSubmitFov)
                {
                    const XrFovf& s = p->submitFov;
                    XrDbg("OpenXR: [eyegeom] submit L%.2f R%.2f U%.2f D%.2f "
                          "deg\n",
                          s.angleLeft * rtd, s.angleRight * rtd,
                          s.angleUp * rtd, s.angleDown * rtd);
                }
            }
        }
    }

    p->projViews.resize(n);
    p->eyeAcquired.assign(n,
                          false); // deferred release: track per-frame acquires
    return n;
}

bool OpenXRBackend::BeginEye(int eye, void** outRtv, int* outW, int* outH)
{
    Impl* p = m_impl;
    if (eye < 0 || eye >= (int)p->swapchains.size())
        return false;
    Impl::Swapchain& sc = p->swapchains[eye];

    uint32_t idx = 0;
    XrSwapchainImageAcquireInfo ai = {XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
    XrResult ar = xrAcquireSwapchainImage(sc.handle, &ai, &idx);
    if (XR_FAILED(ar))
        return false;
    XrSwapchainImageWaitInfo wi = {XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
    wi.timeout = XR_INFINITE_DURATION;
    if (XR_FAILED(xrWaitSwapchainImage(sc.handle, &wi)))
    {
        XrSwapchainImageReleaseInfo ri = {XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
        xrReleaseSwapchainImage(sc.handle, &ri);
        return false;
    }

    if ((int)p->eyeImgIndex.size() < (int)p->swapchains.size())
        p->eyeImgIndex.resize(p->swapchains.size(), 0);
    p->eyeImgIndex[eye] = idx;
    if ((int)p->eyeAcquired.size() <= eye)
        p->eyeAcquired.resize(eye + 1, false);
    p->eyeAcquired[eye] =
        true; // deferred release: released by ReleaseEyes() after both eyes render

    if (p->useVulkan)
    {
        // #107 VR-Vulkan (full per-eye path, DX12 parity): render this eye's FULL scene (world + cockpit + HUD) into
        // VulkanBackend's 1-view scene target; EndEye ends it and blits it into THIS eye's XR image. The engine draws
        // through g_pRenderer into the scene target exactly as the flat path -> cockpit/HUD/offsets all reused.
        // The scene target is sized to the LARGEST per-view size and REUSED for every eye (quad periphery+focus differ,
        // but reallocating per eye churned allocations -> hit the driver's max-allocation cap and crashed). Each eye
        // renders into its top-left sc.width x sc.height sub-rect (SetSceneRenderSize), and the blit copies only that.
        if (!g_pVulkanBackend)
            return false;
        int maxW = 0, maxH = 0;
        for (size_t i = 0; i < p->swapchains.size(); ++i)
        {
            if (p->swapchains[i].width > maxW)
                maxW = p->swapchains[i].width;
            if (p->swapchains[i].height > maxH)
                maxH = p->swapchains[i].height;
        }
        if (!g_pVulkanBackend->EnsureSceneTarget(maxW, maxH, 1))
        {
            // Target unavailable (e.g. allocation failed) -> release this eye's image and skip it, never render into a
            // broken framebuffer (that was the vkCmdBeginRenderPass crash).
            XrSwapchainImageReleaseInfo ri = {
                XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
            xrReleaseSwapchainImage(sc.handle, &ri);
            if (eye < (int)p->eyeAcquired.size())
                p->eyeAcquired[eye] = false;
            XrDbg("OpenXR: BeginEye(Vulkan) - EnsureSceneTarget(%dx%d) failed, "
                  "eye %d skipped\n",
                  maxW, maxH, eye);
            return false;
        }
        // #107: render into the private multiview scene target's sub-rect for this eye; EndEye blits layer 0 into the
        // XR image. (A direct-render-into-the-XR-image experiment was tried and reverted: it never fixed anything the
        // blit path did not, and the blit path is the proven one under the Pimax foveation layer.)
        g_pVulkanBackend->SetSceneRenderSize(sc.width, sc.height);
        g_pVulkanBackend->BeginSceneMultiview(0xFF000000);
        if (outRtv)
            *outRtv =
                (void*)(SIZE_T)1; // non-NULL sentinel so the loop treats the eye as bound
    }
#ifdef _WIN32
    else if (p->useD3D12)
    {
        // #DX12 п.5: open a D3D12 command list rendering INTO this eye's image (bind eye RTV + VR depth, clear).
        // The engine then draws the scene per eye via g_pRenderer straight into the eye image. EndEye executes it.
        if (idx < sc.images12.size())
            g_pD3D12Backend->BeginEyeFrame(sc.images12[idx].texture,
                                           sc.rtvs12[idx], sc.width, sc.height);
        if (outRtv)
            *outRtv = (void*)(SIZE_T)sc.rtvs12[idx];
    }
    else
    {
        if (outRtv)
            *outRtv = sc.rtvs[idx];
    }
#endif
    if (outW)
        *outW = sc.width;
    if (outH)
        *outH = sc.height;
    return true;
}

void OpenXRBackend::EndEye(int eye)
{
    Impl* p = m_impl;
    if (eye < 0 || eye >= (int)p->swapchains.size())
        return;
    Impl::Swapchain& sc = p->swapchains[eye];

    // #DX12 п.5: execute the eye's command list (fill + flush the image) BEFORE the runtime releases it.
    if (p->useVulkan)
    {
        // #107: end the private scene pass, then blit layer 0 into this eye's XR image.
        int idx =
            (eye < (int)p->eyeImgIndex.size()) ? (int)p->eyeImgIndex[eye] : -1;
        if (g_pVulkanBackend)
        {
            g_pVulkanBackend->EndSceneMultiview();
            if (idx >= 0 && idx < (int)sc.imagesVk.size())
            {
                void* dst = (void*)sc.imagesVk[idx].image;
                int w = sc.width, h = sc.height;
                g_pVulkanBackend->BlitSceneToXrImages(1, &dst, &w, &h);
            }
        }
    }
#ifdef _WIN32
    else if (p->useD3D12)
    {
        int idx =
            (eye < (int)p->eyeImgIndex.size()) ? (int)p->eyeImgIndex[eye] : -1;
        if (idx >= 0 && idx < (int)sc.images12.size())
            g_pD3D12Backend->EndEyeFrame(sc.images12[idx].texture);
    }
#endif

    // Artscout - 2026: release THIS eye's image immediately (matches the proven-working clear-only
    // path DiagClearEyesAndEnd, which releases each eye before acquiring the next). Deferred release
    // (ReleaseEyes after both eyes) is now a no-op since eyeAcquired is cleared here.
    {
        XrSwapchainImageReleaseInfo ri = {XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
        xrReleaseSwapchainImage(sc.handle, &ri);
        if (eye < (int)p->eyeAcquired.size())
            p->eyeAcquired[eye] = false;
    }

    if (eye < (int)p->projViews.size())
    {
        XrCompositionLayerProjectionView& pv = p->projViews[eye];
        pv.type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
        pv.next = NULL;
        pv.pose = p->views[eye].pose;
        // Artscout - 2026: submit the fov the engine actually rendered with. STEREO renders symmetric
        // (SetFOV) and submits the symmetric submitFov so the two eyes fuse. QUAD-VIEWS renders each view
        // with its TRUE off-axis (gaze) fov (SetVRFrustum(views[eye].fov)) -> it MUST submit that same
        // per-view fov so the foveated compositor places the focus inset where it was drawn. haveSubmitFov
        // is set only by the stereo path and is NEVER reset, so a stereo/menu frame before quad engaged
        // would leave it true and (wrongly) submit the focus view SYMMETRIC while it was rendered off-axis
        // -> the compositor shifts the focus content = reads as symbology "drift". So gate on the ACTUAL
        // view config, not just the stale flag.
        const bool quadCfg = (p->views.size() > 2);
        pv.fov =
            (p->haveSubmitFov && !quadCfg) ? p->submitFov : p->views[eye].fov;
        pv.subImage.swapchain = sc.handle;
        pv.subImage.imageRect.offset.x = 0;
        pv.subImage.imageRect.offset.y = 0;
        pv.subImage.imageRect.extent.width = sc.width;
        pv.subImage.imageRect.extent.height = sc.height;
        pv.subImage.imageArrayIndex = 0;
    }
}

// Artscout - 2026: #DX12 п.5 -- is single-pass view-instanced stereo the active path this session?
bool OpenXRBackend::ViewInstancingActive() const
{
    return m_impl && m_impl->viActive;
}
int OpenXRBackend::ViewInstancingGroupCount() const
{
    return (m_impl && m_impl->viActive) ?
               ((int)m_impl->configViews.size() / 2) :
               0;
} // pairs: stereo=1, quad=2

// Artscout - 2026: #DX12 п.5 -- decision breakdown (for the one-shot runtime diag in otwloop). Any NULL is skipped.
void OpenXRBackend::GetViewInstancingDiag(bool* active, bool* flag,
                                          bool* stereo, bool* tier,
                                          bool* shaders) const
{
    if (!m_impl)
        return;
    if (active)
        *active = m_impl->viActive;
    if (flag)
        *flag = m_impl->viDiagFlag;
    if (stereo)
        *stereo = m_impl->viDiagStereo;
    if (tier)
        *tier = m_impl->viDiagTier;
    if (shaders)
        *shaders = m_impl->viDiagSh;
}

// Artscout - 2026: #DX12 п.5 -- open ONE group's VI pass. A group is a view pair: stereo = group 0 (views 0,1);
// quad = group 0 (periphery 0,1) + group 1 (focus 2,3). TWO layouts:
//  * viDirect (stereo): render directly into the group's 2-slice ARRAY swapchain (no foveation layer).
//  * viCopy (quad): render into a PRIVATE 2-slice array target (the foveated layer rejects array swapchains),
//    acquiring the group's TWO per-view arraySize=1 swapchains now so EndStereoInstanced can copy the slices in.
// Fills sliceRtvsOut[0..1] with the 2 per-slice RTVs (the per-view 2D overlay tail), outCount=2, render size.
bool OpenXRBackend::BeginStereoInstanced(int group, void** sliceRtvsOut,
                                         int* outCount, int* outW, int* outH)
{
#ifndef _WIN32
    // #108: single-pass view instancing is a D3D12 feature (D3D12Backend). viActive is always false on Linux/Vulkan.
    (void)group;
    (void)sliceRtvsOut;
    (void)outCount;
    (void)outW;
    (void)outH;
    return false;
#else
    Impl* p = m_impl;
    if (!p->viActive || group < 0)
        return false;
    const bool viCopy = (p->configViews.size() == 4);
    if ((int)p->eyeImgIndex.size() < (int)p->swapchains.size())
        p->eyeImgIndex.resize(p->swapchains.size(), 0);

    if (viCopy)
    {
        // Quad: this group's two per-view swapchains (periphery = 0,1; focus = 2,3).
        const int sc0 = 2 * group, sc1 = 2 * group + 1;
        if (sc0 < 0 || sc1 >= (int)p->swapchains.size())
            return false;
        Impl::Swapchain& a = p->swapchains[sc0];
        Impl::Swapchain& b = p->swapchains[sc1];
        uint32_t i0 = 0, i1 = 0;
        XrSwapchainImageAcquireInfo ai = {XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
        XrSwapchainImageWaitInfo wi = {XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
        wi.timeout = XR_INFINITE_DURATION;
        XrSwapchainImageReleaseInfo ri = {XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
        if (XR_FAILED(xrAcquireSwapchainImage(a.handle, &ai, &i0)))
            return false;
        if (XR_FAILED(xrWaitSwapchainImage(a.handle, &wi)))
        {
            xrReleaseSwapchainImage(a.handle, &ri);
            return false;
        }
        if (XR_FAILED(xrAcquireSwapchainImage(b.handle, &ai, &i1)))
        {
            xrReleaseSwapchainImage(a.handle, &ri);
            return false;
        }
        if (XR_FAILED(xrWaitSwapchainImage(b.handle, &wi)))
        {
            xrReleaseSwapchainImage(a.handle, &ri);
            xrReleaseSwapchainImage(b.handle, &ri);
            return false;
        }
        p->eyeImgIndex[sc0] = i0;
        p->eyeImgIndex[sc1] = i1;
        if (group < 2)
            p->viAcquired[group] = true;
        // Render into the PRIVATE array target at this group's foveated resolution; copy happens in End.
        g_pD3D12Backend->BeginViCopyGroup(
            (int)a.width, (int)a.height, (int)p->swapchainFormat, sliceRtvsOut);
        if (outCount)
            *outCount = 2;
        if (outW)
            *outW = a.width;
        if (outH)
            *outH = a.height;
        return true;
    }

    // viDirect (stereo): one 2-slice array swapchain, rendered directly.
    if (group >= (int)p->swapchains.size())
        return false;
    Impl::Swapchain& sc = p->swapchains[group];
    uint32_t idx = 0;
    XrSwapchainImageAcquireInfo ai = {XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
    if (XR_FAILED(xrAcquireSwapchainImage(sc.handle, &ai, &idx)))
        return false;
    XrSwapchainImageWaitInfo wi = {XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
    wi.timeout = XR_INFINITE_DURATION;
    if (XR_FAILED(xrWaitSwapchainImage(sc.handle, &wi)))
    {
        XrSwapchainImageReleaseInfo ri = {XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
        xrReleaseSwapchainImage(sc.handle, &ri);
        return false;
    }
    p->eyeImgIndex[group] = idx;
    if (group < 2)
        p->viAcquired[group] = true;
    if (idx < sc.images12.size())
        g_pD3D12Backend->BeginStereoInstancedFrame(
            sc.images12[idx].texture, sc.rtvs12[idx], sc.width, sc.height, 2);
    for (int v = 0; v < 4; ++v)
        if (sliceRtvsOut)
            sliceRtvsOut[v] =
                (void*)(SIZE_T)((v < 2 && idx < sc.sliceRtvs12[v].size()) ?
                                    sc.sliceRtvs12[v][idx] :
                                    0);
    if (outCount)
        *outCount = 2;
    if (outW)
        *outW = sc.width;
    if (outH)
        *outH = sc.height;
    return true;
#endif // _WIN32
}

// Artscout - 2026: #DX12 п.5 -- close+execute ONE group's list, release its array image, and fill the group's 2
// projection views (its swapchain, imageArrayIndex 0/1). Global view index = 2*group + local. QUAD submits each
// view's RAW per-view fov (off-center gaze -- the foveated compositor needs it); STEREO submits the symmetric
// submitFov the engine rendered with. Pose is the runtime's per-view located pose. Called per group; mirrors EndEye.
void OpenXRBackend::EndStereoInstanced(int group)
{
#ifndef _WIN32
    // #108: single-pass view instancing is a D3D12 feature. No-op on Linux/Vulkan (viActive is always false).
    (void)group;
    return;
#else
    Impl* p = m_impl;
    if (!p->viActive || group < 0)
        return;
    if (group < 2 && !p->viAcquired[group])
        return; // never acquired this frame (acquire failed) -> nothing to release
    const bool viCopy = (p->configViews.size() == 4);
    XrSwapchainImageReleaseInfo ri = {XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
    const int nTot = (int)p->projViews.size();
    const bool quad = (nTot > 2);

    if (viCopy)
    {
        // Quad: copy the private array's 2 slices into this group's TWO per-view swapchain images, then release
        // both. Each per-view swapchain is its own arraySize=1 image (imageArrayIndex 0) at its foveated res.
        const int sc0 = 2 * group, sc1 = 2 * group + 1;
        if (sc0 < 0 || sc1 >= (int)p->swapchains.size())
            return;
        Impl::Swapchain& a = p->swapchains[sc0];
        Impl::Swapchain& b = p->swapchains[sc1];
        int i0 =
            (sc0 < (int)p->eyeImgIndex.size()) ? (int)p->eyeImgIndex[sc0] : -1;
        int i1 =
            (sc1 < (int)p->eyeImgIndex.size()) ? (int)p->eyeImgIndex[sc1] : -1;
        void* dst0 = (i0 >= 0 && i0 < (int)a.images12.size()) ?
                         (void*)a.images12[i0].texture :
                         NULL;
        void* dst1 = (i1 >= 0 && i1 < (int)b.images12.size()) ?
                         (void*)b.images12[i1].texture :
                         NULL;
        g_pD3D12Backend->EndViCopyGroup(dst0, dst1);
        xrReleaseSwapchainImage(a.handle, &ri);
        xrReleaseSwapchainImage(b.handle, &ri);
        if (group < 2)
            p->viAcquired[group] = false;

        for (int j = 0; j < 2; ++j)
        {
            const int e = 2 * group + j; // global view index
            if (e >= nTot)
                break;
            Impl::Swapchain& s = p->swapchains[e];
            XrCompositionLayerProjectionView& pv = p->projViews[e];
            pv.type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
            pv.next = NULL;
            pv.pose = p->views[e].pose;
            pv.fov =
                p->views[e].fov; // quad: raw per-view fov (foveated compositor)
            pv.subImage.swapchain = s.handle;
            pv.subImage.imageRect.offset.x = 0;
            pv.subImage.imageRect.offset.y = 0;
            pv.subImage.imageRect.extent.width = s.width;
            pv.subImage.imageRect.extent.height = s.height;
            pv.subImage.imageArrayIndex =
                0; // per-view swapchain -> single slice
        }
        return;
    }

    // viDirect (stereo): one 2-slice array swapchain rendered directly.
    if (group >= (int)p->swapchains.size())
        return;
    Impl::Swapchain& sc = p->swapchains[group];
    int idx =
        (group < (int)p->eyeImgIndex.size()) ? (int)p->eyeImgIndex[group] : -1;
    if (idx >= 0 && idx < (int)sc.images12.size())
        g_pD3D12Backend->EndStereoInstancedFrame(sc.images12[idx].texture);
    xrReleaseSwapchainImage(sc.handle, &ri);
    if (group < 2)
        p->viAcquired[group] = false;

    for (int j = 0; j < 2; ++j)
    {
        const int e = 2 * group + j; // global view index
        if (e >= nTot)
            break;
        XrCompositionLayerProjectionView& pv = p->projViews[e];
        pv.type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
        pv.next = NULL;
        pv.pose = p->views[e].pose;
        pv.fov = (p->haveSubmitFov && !quad) ? p->submitFov : p->views[e].fov;
        pv.subImage.swapchain = sc.handle;
        pv.subImage.imageRect.offset.x = 0;
        pv.subImage.imageRect.offset.y = 0;
        pv.subImage.imageRect.extent.width = sc.width;
        pv.subImage.imageRect.extent.height = sc.height;
        pv.subImage.imageArrayIndex =
            j; // local slice within the group's swapchain
    }
#endif // _WIN32
}

// Artscout - 2026: TEMP DIAG -- replicate the M1 clear path INSIDE an already-begun stereo frame
// (sim thread): for each eye acquire/wait/clear(distinct color)/release, fill projViews, xrEndFrame.
// No engine rendering between eyes. If both eyes show their color -> two separate swapchains compose
// fine on this runtime/thread and the 2nd-eye-black is caused by the engine render between eyes.
// If the 2nd eye is black -> the second swapchain genuinely isn't composited (-> texture-array path).
void OpenXRBackend::DiagClearEyesAndEnd()
{
    Impl* p = m_impl;
    if (!p->inStereoFrame)
        return;
    const int n = (int)p->projViews.size();
    for (int e = 0; e < n && e < (int)p->swapchains.size(); ++e)
    {
        Impl::Swapchain& sc = p->swapchains[e];
        uint32_t idx = 0;
        XrSwapchainImageAcquireInfo ai = {XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
        if (XR_FAILED(xrAcquireSwapchainImage(sc.handle, &ai, &idx)))
            continue;
        XrSwapchainImageWaitInfo wi = {XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
        wi.timeout = XR_INFINITE_DURATION;
        if (XR_FAILED(xrWaitSwapchainImage(sc.handle, &wi)))
        {
            XrSwapchainImageReleaseInfo ri = {
                XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
            xrReleaseSwapchainImage(sc.handle, &ri);
            continue;
        }
        const float red[4] = {0.8f, 0.0f, 0.0f, 1.0f};
        const float green[4] = {0.0f, 0.8f, 0.0f, 1.0f};
#ifdef _WIN32
        if (p->useD3D12)
        {
            if (idx < sc.images12.size())
                XrClearRtvD3D12(p->alloc12, p->list12, p->d3d12Queue,
                                p->fence12, p->fenceEvt12, &p->fenceVal12,
                                sc.images12[idx].texture, sc.rtvs12[idx],
                                (e == 0) ? red : green);
        }
        else
        {
            p->ctx->ClearRenderTargetView(sc.rtvs[idx], (e == 0) ? red : green);
            p->ctx->Flush();
        }
#else
        // #108: the D3D clear path is Windows-only; this TEMP diag is not driven on the Linux/Vulkan path.
        (void)red;
        (void)green;
#endif
        XrSwapchainImageReleaseInfo ri = {XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
        xrReleaseSwapchainImage(sc.handle, &ri);

        XrCompositionLayerProjectionView& pv = p->projViews[e];
        pv.type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
        pv.next = NULL;
        pv.pose = p->views[e].pose;
        pv.fov = p->views[e].fov;
        pv.subImage.swapchain = sc.handle;
        pv.subImage.imageRect.offset.x = 0;
        pv.subImage.imageRect.offset.y = 0;
        pv.subImage.imageRect.extent.width = sc.width;
        pv.subImage.imageRect.extent.height = sc.height;
        pv.subImage.imageArrayIndex = 0;
    }

    XrCompositionLayerProjection layer = {XR_TYPE_COMPOSITION_LAYER_PROJECTION};
    layer.space = p->appSpace;
    layer.viewCount = (uint32_t)p->projViews.size();
    layer.views = p->projViews.data();
    XrCompositionLayerBaseHeader* layers[1] = {
        (XrCompositionLayerBaseHeader*)&layer};
    XrFrameEndInfo fei = {XR_TYPE_FRAME_END_INFO};
    fei.displayTime = p->stereoFrameState.predictedDisplayTime;
    fei.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    fei.layerCount = 1;
    fei.layers = layers;
    xrEndFrame(p->session, &fei);
    p->inStereoFrame = false;
    p->currentEye = -1;
}

// Artscout - 2026 (#107 VR-Vulkan): present the multiview scene array. The engine has rendered ONE multiview frame
// into VulkanBackend's N-layer scene array (world via gl_ViewIndex + the per-eye cockpit/HUD tail). Here: acquire each
// per-view XR swapchain image, blit scene layer e into it (VulkanBackend::BlitSceneToXrImages, one submit), release,
// fill the projection views, and xrEndFrame. Peer of the D3D12 EndStereoInstanced/EndStereoFrame tail. Returns false
// (still ends the frame) if the backend blit is unavailable, so the runtime always gets its xrEndFrame.
bool OpenXRBackend::PresentMultiviewVulkan()
{
    Impl* p = m_impl;
    if (!p->inStereoFrame)
        return false;
    const int n = (int)p->views.size();
    if (n < 1 || (int)p->swapchains.size() < n)
    {
        EndStereoFrame();
        return false;
    }

    // Acquire every per-view image first, collecting the VkImage + per-view size for a single blit submit.
    void* dstImages[8] = {0};
    int dstW[8] = {0}, dstH[8] = {0};
    uint32_t imgIdx[8] = {0};
    bool acquired[8] = {false};
    int nAcq = (n > 8) ? 8 : n;
    for (int e = 0; e < nAcq; ++e)
    {
        Impl::Swapchain& sc = p->swapchains[e];
        XrSwapchainImageAcquireInfo ai = {XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
        if (XR_FAILED(xrAcquireSwapchainImage(sc.handle, &ai, &imgIdx[e])))
            continue;
        XrSwapchainImageWaitInfo wi = {XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
        wi.timeout = XR_INFINITE_DURATION;
        if (XR_FAILED(xrWaitSwapchainImage(sc.handle, &wi)))
        {
            XrSwapchainImageReleaseInfo ri = {
                XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
            xrReleaseSwapchainImage(sc.handle, &ri);
            continue;
        }
        acquired[e] = true;
        dstImages[e] = (imgIdx[e] < sc.imagesVk.size()) ?
                           (void*)sc.imagesVk[imgIdx[e]].image :
                           NULL;
        dstW[e] = sc.width;
        dstH[e] = sc.height;
    }

    // One blit submit copies all N scene layers into the N acquired images (the backend waits its own fence, so the
    // images are safe to release immediately after).
    bool blitOk = false;
    bool sceneRec = g_pVulkanBackend && g_pVulkanBackend->IsSceneRecording();
    if (g_pVulkanBackend)
        blitOk =
            g_pVulkanBackend->BlitSceneToXrImages(nAcq, dstImages, dstW, dstH);
    // VR-black diag: sceneRec=0 -> the multiview 3D scene was never recorded (nothing to blit -> black eyes);
    // blit=0 -> the copy into the XR swapchain failed. dst0 NULL -> swapchain image acquire failed.
    {
        static int s_n = 0;
        if (s_n < 8)
        {
            s_n++;
            XrDbg("OpenXR: PresentMV nAcq=%d dst0=%p dstW0=%d sceneRec=%d "
                  "blit=%d\n",
                  nAcq, dstImages[0], dstW[0], (int)sceneRec, (int)blitOk);
        }
    }

    p->projViews.resize(n);
    for (int e = 0; e < nAcq; ++e)
    {
        if (acquired[e])
        {
            XrSwapchainImageReleaseInfo ri = {
                XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
            xrReleaseSwapchainImage(p->swapchains[e].handle, &ri);
        }
        XrCompositionLayerProjectionView& pv = p->projViews[e];
        pv.type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
        pv.next = NULL;
        pv.pose = p->views[e].pose;
        pv.fov = p->views[e].fov;
        pv.subImage.swapchain = p->swapchains[e].handle;
        pv.subImage.imageRect.offset.x = 0;
        pv.subImage.imageRect.offset.y = 0;
        pv.subImage.imageRect.extent.width = p->swapchains[e].width;
        pv.subImage.imageRect.extent.height = p->swapchains[e].height;
        pv.subImage.imageArrayIndex = 0;
    }

    XrCompositionLayerProjection layer = {XR_TYPE_COMPOSITION_LAYER_PROJECTION};
    layer.space = p->appSpace;
    layer.viewCount = (uint32_t)p->projViews.size();
    layer.views = p->projViews.data();
    XrCompositionLayerBaseHeader* layers[1] = {
        (XrCompositionLayerBaseHeader*)&layer};
    XrFrameEndInfo fei = {XR_TYPE_FRAME_END_INFO};
    fei.displayTime = p->stereoFrameState.predictedDisplayTime;
    fei.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    fei.layerCount = 1;
    fei.layers = layers;
    XrResult endr = xrEndFrame(p->session, &fei);
    {
        static int s_e = 0;
        if (s_e < 8)
        {
            s_e++;
            XrDbg("OpenXR: PresentMV xrEndFrame=%d\n", (int)endr);
        }
    }
    p->inStereoFrame = false;
    p->currentEye = -1;
    return true;
}

// Artscout - 2026 (#107 GROUPED multiview): blit group g's freshly-rendered 2-layer scene target into that group's two
// XR swapchains (views 2g, 2g+1). The scene target is REUSED per group (both groups render into layers 0,1), so this
// must run after the group's EndSceneMultiview and BEFORE the next group's BeginSceneMultiview overwrites it.
bool OpenXRBackend::BlitMultiviewGroupVulkan(int group)
{
    Impl* p = m_impl;
    if (!p->inStereoFrame || !g_pVulkanBackend)
        return false;
    const int n = (int)p->views.size();
    const int vv[2] = {group * 2, group * 2 + 1};
    if (vv[1] >= n || vv[1] >= (int)p->swapchains.size())
        return false;

    void* dst[2] = {0, 0};
    int dw[2] = {0, 0}, dh[2] = {0, 0};
    uint32_t idx[2] = {0, 0};
    bool acq[2] = {false, false};
    for (int j = 0; j < 2; ++j)
    {
        Impl::Swapchain& sc = p->swapchains[vv[j]];
        XrSwapchainImageAcquireInfo ai = {XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
        if (XR_FAILED(xrAcquireSwapchainImage(sc.handle, &ai, &idx[j])))
            continue;
        XrSwapchainImageWaitInfo wi = {XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
        wi.timeout = XR_INFINITE_DURATION;
        if (XR_FAILED(xrWaitSwapchainImage(sc.handle, &wi)))
        {
            XrSwapchainImageReleaseInfo ri = {
                XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
            xrReleaseSwapchainImage(sc.handle, &ri);
            continue;
        }
        acq[j] = true;
        dst[j] = (idx[j] < sc.imagesVk.size()) ?
                     (void*)sc.imagesVk[idx[j]].image :
                     NULL;
        dw[j] = sc.width;
        dh[j] = sc.height;
    }

    // One blit submit copies scene layers 0,1 into the two acquired images (backend waits its own fence).
    bool blitOk = g_pVulkanBackend->BlitSceneToXrImages(2, dst, dw, dh);
    {
        static int s_n = 0;
        if (s_n < 8)
        {
            s_n++;
            XrDbg("OpenXR: BlitMVGroup g=%d v=%d,%d dst0=%p %dx%d blit=%d\n",
                  group, vv[0], vv[1], dst[0], dw[0], dh[0], (int)blitOk);
        }
    }

    if ((int)p->projViews.size() < n)
        p->projViews.resize(n);
    for (int j = 0; j < 2; ++j)
    {
        if (acq[j])
        {
            XrSwapchainImageReleaseInfo ri = {
                XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
            xrReleaseSwapchainImage(p->swapchains[vv[j]].handle, &ri);
        }
        XrCompositionLayerProjectionView& pv = p->projViews[vv[j]];
        pv.type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
        pv.next = NULL;
        pv.pose = p->views[vv[j]].pose;
        // Submit the fov the engine ACTUALLY rendered with (mirror of EndEye): STEREO renders symmetric (SetFOV) and
        // must submit the symmetric submitFov so the eyes fuse; QUAD renders each view off-axis (SetVRFrustum) and must
        // submit that raw per-view fov. Submitting the raw runtime fov for a symmetric-rendered stereo eye makes the
        // compositor skew the image -> "too rotated" stereo.
        const bool quadCfg = (n > 2);
        pv.fov =
            (p->haveSubmitFov && !quadCfg) ? p->submitFov : p->views[vv[j]].fov;
        pv.subImage.swapchain = p->swapchains[vv[j]].handle;
        pv.subImage.imageRect.offset.x = 0;
        pv.subImage.imageRect.offset.y = 0;
        pv.subImage.imageRect.extent.width = p->swapchains[vv[j]].width;
        pv.subImage.imageRect.extent.height = p->swapchains[vv[j]].height;
        pv.subImage.imageArrayIndex = 0;
    }
    return true;
}

// Artscout - 2026 (#107 GROUPED multiview): submit ONE projection layer covering all views + xrEndFrame. Called once
// after the last group's BlitMultiviewGroupVulkan. (Stage 2 will add the menu/FPS/cursor quads here.)
bool OpenXRBackend::SubmitMultiviewVulkan()
{
    Impl* p = m_impl;
    if (!p->inStereoFrame)
        return false;
    const int n = (int)p->views.size();
    if ((int)p->projViews.size() < n)
    {
        EndStereoFrame();
        return false;
    }

    XrCompositionLayerProjection layer = {XR_TYPE_COMPOSITION_LAYER_PROJECTION};
    layer.space = p->appSpace;
    layer.viewCount = (uint32_t)n;
    layer.views = p->projViews.data();
    XrCompositionLayerBaseHeader* layers[1] = {
        (XrCompositionLayerBaseHeader*)&layer};
    XrFrameEndInfo fei = {XR_TYPE_FRAME_END_INFO};
    fei.displayTime = p->stereoFrameState.predictedDisplayTime;
    fei.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    fei.layerCount = 1;
    fei.layers = layers;
    XrResult endr = xrEndFrame(p->session, &fei);
    {
        static int s_e = 0;
        if (s_e < 8)
        {
            s_e++;
            XrDbg("OpenXR: SubmitMV views=%d xrEndFrame=%d\n", n, (int)endr);
        }
    }
    p->inStereoFrame = false;
    p->currentEye = -1;
    return true;
}

// Artscout - 2026: release all eye swapchain images acquired this frame (deferred release).
// Call after BOTH eyes have rendered, before EndStereoFrame()/xrEndFrame.
void OpenXRBackend::ReleaseEyes()
{
    Impl* p = m_impl;
    for (int e = 0;
         e < (int)p->eyeAcquired.size() && e < (int)p->swapchains.size(); ++e)
    {
        if (!p->eyeAcquired[e])
            continue;
        XrSwapchainImageReleaseInfo ri = {XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
        xrReleaseSwapchainImage(p->swapchains[e].handle, &ri);
        p->eyeAcquired[e] = false;
    }
}

// Artscout - 2026 (#59 VR menu): copy the drawn menu texture into the UI swapchain and stage a head-locked
// quad (VIEW space) to be composited on top of the projection layer by EndStereoFrame. Called once per stereo
// frame (sim thread) only while a comms/exit menu is up. Same machinery as RunMenuFrame's pre-3D quad, but in
// VIEW space (follows the head) and with source-alpha blend so only the menu pixels show.
bool OpenXRBackend::SubmitInSceneMenuQuad(void* menuTex, int w, int h)
{
    Impl* p = m_impl;
    if (!p->inStereoFrame || !menuTex || w <= 0 || h <= 0)
        return false;
    if (p->viewSpace == XR_NULL_HANDLE)
        return false;
    if (!EnsureUiSwapchain(w, h))
        return false;

    extern float g_fVrMenuDist, g_fVrMenuHeight;

    // #DX12 п.5 A1: D3D12 in-scene menu quad. menuTex is a D3D12Texture* (the backend's menu RTT). It is drawn on
    // the EYE command list, which has NOT executed yet at this point -- so we CANNOT copy it into the UI image now.
    // Stage it + the quad here; EndStereoFrame does the actual UI-image copy AFTER the eye frame(s) execute+fence.
#ifdef _WIN32
    if (p->useD3D12)
    {
        p->menuTexD3D12 = menuTex;

        const float aspect = (float)w / (float)h;
        const float heightM = (g_fVrMenuHeight > 0.1f) ? g_fVrMenuHeight : 1.4f;
        const float distM = (g_fVrMenuDist > 0.1f) ? g_fVrMenuDist : 1.8f;
        XrCompositionLayerQuad& q = p->menuQuad;
        memset(&q, 0, sizeof(q));
        q.type = XR_TYPE_COMPOSITION_LAYER_QUAD;
        q.layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
        q.space =
            p->viewSpace; // head-locked (in front of the head, independent of gaze/quad-views)
        q.eyeVisibility = XR_EYE_VISIBILITY_BOTH;
        q.subImage.swapchain = p->uiSwapchain;
        q.subImage.imageRect.extent.width = w;
        q.subImage.imageRect.extent.height = h;
        q.subImage.imageArrayIndex = 0;
        q.pose.orientation.w = 1.0f;
        q.pose.position.z = -distM;
        q.size.width = heightM * aspect;
        q.size.height = heightM;
        p->menuQuadPending = true;
        return true;
    }
#endif // _WIN32 (D3D12 in-scene menu quad)

    // #107 VR-Vulkan in-3D menu quad. menuTex is the Vulkan menu-RTT handle (VulkanBackend::MenuRttTex). Unlike D3D12,
    // the Vulkan RTT was rendered SYNCHRONOUSLY (the otwloop block UnbindSceneRtt'd it -- vkQueueWaitIdle -- before this
    // call), so we copy it into the UI swapchain image NOW (eager, like the D3D11 branch below) instead of deferring to
    // EndStereoFrame. Then stage the head-locked quad; EndStereoFrame's backend-agnostic layer assembly submits it.
    if (p->useVulkan)
    {
        if (!g_pVulkanBackend)
            return false;
        uint32_t idx = 0;
        XrSwapchainImageAcquireInfo ai = {XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
        if (XR_FAILED(xrAcquireSwapchainImage(p->uiSwapchain, &ai, &idx)))
            return false;
        XrSwapchainImageWaitInfo wi = {XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
        wi.timeout = XR_INFINITE_DURATION;
        if (XR_SUCCEEDED(xrWaitSwapchainImage(p->uiSwapchain, &wi)) &&
            idx < p->uiImagesVk.size())
            g_pVulkanBackend->BlitTexToXrImage((void*)p->uiImagesVk[idx].image,
                                               menuTex, w, h);
        XrSwapchainImageReleaseInfo ri = {XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
        xrReleaseSwapchainImage(p->uiSwapchain, &ri);

        const float aspect = (float)w / (float)h;
        const float heightM = (g_fVrMenuHeight > 0.1f) ? g_fVrMenuHeight : 1.4f;
        const float distM = (g_fVrMenuDist > 0.1f) ? g_fVrMenuDist : 1.8f;
        XrCompositionLayerQuad& q = p->menuQuad;
        memset(&q, 0, sizeof(q));
        q.type = XR_TYPE_COMPOSITION_LAYER_QUAD;
        q.layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
        q.space =
            p->viewSpace; // head-locked (in front of the head, independent of gaze/quad-views)
        q.eyeVisibility = XR_EYE_VISIBILITY_BOTH;
        q.subImage.swapchain = p->uiSwapchain;
        q.subImage.imageRect.extent.width = w;
        q.subImage.imageRect.extent.height = h;
        q.subImage.imageArrayIndex = 0;
        q.pose.orientation.w = 1.0f;
        q.pose.position.z = -distM;
        q.size.width = heightM * aspect;
        q.size.height = heightM;
        p->menuQuadPending = true;
        return true;
    }

#ifdef _WIN32
    ID3D11Texture2D* src = (ID3D11Texture2D*)menuTex;

    uint32_t idx = 0;
    XrSwapchainImageAcquireInfo ai = {XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
    if (XR_FAILED(xrAcquireSwapchainImage(p->uiSwapchain, &ai, &idx)))
        return false;
    XrSwapchainImageWaitInfo wi = {XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
    wi.timeout = XR_INFINITE_DURATION;
    if (XR_SUCCEEDED(xrWaitSwapchainImage(p->uiSwapchain, &wi)))
        p->ctx->CopyResource(p->uiImages[idx].texture, src);
    XrSwapchainImageReleaseInfo ri = {XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
    xrReleaseSwapchainImage(p->uiSwapchain, &ri);

    extern float g_fVrMenuDist, g_fVrMenuHeight;
    const float aspect = (float)w / (float)h;
    const float heightM = (g_fVrMenuHeight > 0.1f) ? g_fVrMenuHeight : 1.4f;
    const float distM = (g_fVrMenuDist > 0.1f) ? g_fVrMenuDist : 1.8f;

    XrCompositionLayerQuad& q = p->menuQuad;
    memset(&q, 0, sizeof(q));
    q.type = XR_TYPE_COMPOSITION_LAYER_QUAD;
    q.layerFlags =
        XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT; // menu alpha keys out the empty canvas
    q.space =
        p->viewSpace; // HEAD-LOCKED: stays in front of the head, independent of gaze/quad-views
    q.eyeVisibility = XR_EYE_VISIBILITY_BOTH;
    q.subImage.swapchain = p->uiSwapchain;
    q.subImage.imageRect.extent.width = w;
    q.subImage.imageRect.extent.height = h;
    q.subImage.imageArrayIndex = 0;
    q.pose.orientation.w = 1.0f;
    q.pose.position.z = -distM;
    q.size.width = heightM * aspect;
    q.size.height = heightM;

    p->menuQuadPending = true;
    return true;
#else
    // #108: the D3D11 in-scene menu path is unreachable on Linux (useVulkan is always true above -> returned).
    return false;
#endif
}

// Artscout - 2026: #DX12 п.5 -- small head-locked FPS quad swapchain (D3D12 only; the FPS RTT is GPU-copied into it
// in EndStereoFrame, no upload buffer needed). Mirrors EnsureUiSwapchain, minus the D3D11 565->RGBA staging.
bool OpenXRBackend::EnsureFpsSwapchain(int w, int h)
{
    Impl* p = m_impl;
    // #107 VR-Vulkan: reuse the eye VkFormat when uiFormat is 0 (same as EnsureUiSwapchain), so the Vulkan FPS quad
    // gets a valid format. D3D paths keep their chosen uiFormat.
    if (p->useVulkan && p->uiFormat == 0)
        p->uiFormat = p->swapchainFormat;
    if (w <= 0 || h <= 0 || p->uiFormat == 0)
        return false;
    if (p->fpsSwapchain != XR_NULL_HANDLE && p->fpsW == w && p->fpsH == h)
        return true;
    if (p->fpsSwapchain != XR_NULL_HANDLE)
    {
        xrDestroySwapchain(p->fpsSwapchain);
        p->fpsSwapchain = XR_NULL_HANDLE;
    }
    p->fpsImagesVk.clear();
#ifdef _WIN32
    p->fpsImages12.clear();
#endif

    XrSwapchainCreateInfo ci = {XR_TYPE_SWAPCHAIN_CREATE_INFO};
    ci.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT |
                    XR_SWAPCHAIN_USAGE_SAMPLED_BIT |
                    XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT;
    ci.format = p->uiFormat;
    ci.sampleCount = 1;
    ci.width = w;
    ci.height = h;
    ci.faceCount = 1;
    ci.arraySize = 1;
    ci.mipCount = 1;
    if (XR_FAILED(xrCreateSwapchain(p->session, &ci, &p->fpsSwapchain)))
    {
        XrDbg("OpenXR: FPS xrCreateSwapchain failed (%dx%d)\n", w, h);
        return false;
    }

    uint32_t imgCount = 0;
    xrEnumerateSwapchainImages(p->fpsSwapchain, 0, &imgCount, NULL);
    if (p->useVulkan) // #107 VR-Vulkan: FPS quad VkImages (BlitTexToXrImage target, eager)
    {
        p->fpsImagesVk.resize(imgCount);
        for (uint32_t i = 0; i < imgCount; ++i)
        {
            p->fpsImagesVk[i].type = XR_TYPE_SWAPCHAIN_IMAGE_VULKAN_KHR;
            p->fpsImagesVk[i].next = NULL;
        }
        xrEnumerateSwapchainImages(
            p->fpsSwapchain, imgCount, &imgCount,
            (XrSwapchainImageBaseHeader*)p->fpsImagesVk.data());
    }
#ifdef _WIN32
    else if (p->useD3D12)
    {
        p->fpsImages12.resize(imgCount);
        for (uint32_t i = 0; i < imgCount; ++i)
        {
            p->fpsImages12[i].type = XR_TYPE_SWAPCHAIN_IMAGE_D3D12_KHR;
            p->fpsImages12[i].next = NULL;
        }
        xrEnumerateSwapchainImages(
            p->fpsSwapchain, imgCount, &imgCount,
            (XrSwapchainImageBaseHeader*)p->fpsImages12.data());
    }
#endif
    p->fpsW = w;
    p->fpsH = h;
    return true;
}

// #59: subtitle-quad swapchain -- clone of EnsureFpsSwapchain (Vulkan images only; the block is Vulkan-path scoped).
bool OpenXRBackend::EnsureSubQuadSwapchain(int w, int h)
{
    Impl* p = m_impl;
    if (p->useVulkan && p->uiFormat == 0)
        p->uiFormat = p->swapchainFormat;
    if (w <= 0 || h <= 0 || p->uiFormat == 0)
        return false;
    if (p->subSwapchain != XR_NULL_HANDLE && p->subW == w && p->subH == h)
        return true;
    if (p->subSwapchain != XR_NULL_HANDLE)
    {
        xrDestroySwapchain(p->subSwapchain);
        p->subSwapchain = XR_NULL_HANDLE;
    }
    p->subImagesVk.clear();

    XrSwapchainCreateInfo ci = {XR_TYPE_SWAPCHAIN_CREATE_INFO};
    ci.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT |
                    XR_SWAPCHAIN_USAGE_SAMPLED_BIT |
                    XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT;
    ci.format = p->uiFormat;
    ci.sampleCount = 1;
    ci.width = w;
    ci.height = h;
    ci.faceCount = 1;
    ci.arraySize = 1;
    ci.mipCount = 1;
    if (XR_FAILED(xrCreateSwapchain(p->session, &ci, &p->subSwapchain)))
    {
        XrDbg("OpenXR: subtitle xrCreateSwapchain failed (%dx%d)\n", w, h);
        return false;
    }

    uint32_t imgCount = 0;
    xrEnumerateSwapchainImages(p->subSwapchain, 0, &imgCount, NULL);
    if (p->useVulkan)
    {
        p->subImagesVk.resize(imgCount);
        for (uint32_t i = 0; i < imgCount; ++i)
        {
            p->subImagesVk[i].type = XR_TYPE_SWAPCHAIN_IMAGE_VULKAN_KHR;
            p->subImagesVk[i].next = NULL;
        }
        xrEnumerateSwapchainImages(
            p->subSwapchain, imgCount, &imgCount,
            (XrSwapchainImageBaseHeader*)p->subImagesVk.data());
    }
    p->subW = w;
    p->subH = h;
    return true;
}

// #59: submit the subtitle/chat quad (per Albert -- its own head-locked panel where subtitles normally sit: upper
// area, like the flat path's TextLeft(-0.95, 0.84) top placement). Vulkan eager-copy path, mirror of SubmitFpsQuad.
bool OpenXRBackend::SubmitSubtitleQuad(void* tex, int w, int h)
{
    Impl* p = m_impl;
    if (!p->inStereoFrame || !tex || w <= 0 || h <= 0)
        return false;
    if (p->viewSpace == XR_NULL_HANDLE)
        return false;
    if (!p->useVulkan || !g_pVulkanBackend)
        return false; // D3D12 MV adoption later (needs the staged-copy machinery)
    if (!EnsureSubQuadSwapchain(w, h))
        return false;

    uint32_t idx = 0;
    XrSwapchainImageAcquireInfo ai = {XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
    if (XR_FAILED(xrAcquireSwapchainImage(p->subSwapchain, &ai, &idx)))
        return false;
    XrSwapchainImageWaitInfo wi = {XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
    wi.timeout = XR_INFINITE_DURATION;
    if (XR_SUCCEEDED(xrWaitSwapchainImage(p->subSwapchain, &wi)) &&
        idx < p->subImagesVk.size())
        g_pVulkanBackend->BlitTexToXrImage((void*)p->subImagesVk[idx].image,
                                           tex, w, h);
    XrSwapchainImageReleaseInfo ri = {XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
    xrReleaseSwapchainImage(p->subSwapchain, &ri);

    const float aspect = (float)w / (float)h;
    const float heightM = 0.34f; // readable multi-line panel
    const float distM =
        1.4f; // same plane as the FPS quad (no convergence fight)
    XrCompositionLayerQuad& q = p->subQuad;
    memset(&q, 0, sizeof(q));
    q.type = XR_TYPE_COMPOSITION_LAYER_QUAD;
    q.layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
    q.space = p->viewSpace;
    q.eyeVisibility = XR_EYE_VISIBILITY_BOTH;
    q.subImage.swapchain = p->subSwapchain;
    q.subImage.imageRect.extent.width = w;
    q.subImage.imageRect.extent.height = h;
    q.subImage.imageArrayIndex = 0;
    q.pose.orientation.w = 1.0f;
    {
        extern float g_fVrSubQuadX,
            g_fVrSubQuadY; // #59: cfg-tunable panel position (VrSubQuadX/VrSubQuadY)
        q.pose.position.x = g_fVrSubQuadX;
        q.pose.position.y = g_fVrSubQuadY;
    }
    q.pose.position.z = -distM;
    q.size.width = heightM * aspect;
    q.size.height = heightM;
    p->subQuadPending = true;
    return true;
}

// Artscout - 2026: #DX12 п.5 -- stage the FPS RTT (a D3D12Texture*) as a small HEAD-LOCKED quad (upper-centre, in
// front of the head, independent of gaze/foveation). Copy into the FPS swapchain happens in EndStereoFrame (the RTT
// was drawn on the not-yet-executed eye list). Mirrors SubmitInSceneMenuQuad but small + positioned up.
bool OpenXRBackend::SubmitFpsQuad(void* tex, int w, int h)
{
    Impl* p = m_impl;
    if (!p->inStereoFrame || !tex || w <= 0 || h <= 0)
        return false;
    if (p->viewSpace == XR_NULL_HANDLE)
        return false;
    if (!EnsureFpsSwapchain(w, h))
        return false;

#ifdef _WIN32
    if (p->useD3D12)
        p->fpsTexD3D12 =
            tex; // D3D12: staged, copied in EndStereoFrame (eye list not yet executed)
#endif
    // #107 VR-Vulkan: the Vulkan RTT is already finalized (UnbindSceneRtt), so copy it into the FPS image NOW (eager),
    // exactly like SubmitInSceneMenuQuad. EndStereoFrame just submits the pending layer.
    if (p->useVulkan && g_pVulkanBackend)
    {
        uint32_t idx = 0;
        XrSwapchainImageAcquireInfo ai = {XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
        if (XR_FAILED(xrAcquireSwapchainImage(p->fpsSwapchain, &ai, &idx)))
            return false;
        XrSwapchainImageWaitInfo wi = {XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
        wi.timeout = XR_INFINITE_DURATION;
        if (XR_SUCCEEDED(xrWaitSwapchainImage(p->fpsSwapchain, &wi)) &&
            idx < p->fpsImagesVk.size())
            g_pVulkanBackend->BlitTexToXrImage((void*)p->fpsImagesVk[idx].image,
                                               tex, w, h);
        XrSwapchainImageReleaseInfo ri = {XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
        xrReleaseSwapchainImage(p->fpsSwapchain, &ri);
    }

    const float aspect = (float)w / (float)h;
    // Artscout - 2026: 0.11 -> 0.33 m, 3x, per Albert. The quad is head-locked at a fixed 1.4 m, so this is really
    // an ANGULAR size: 0.11 m subtended ~4.5 deg, legible in the headset but only a handful of pixels once a
    // capture is downscaled -- and a counter you cannot read off the recording cannot answer "what did that cost",
    // which is the only reason it exists. Distance stays put deliberately: pulling the quad closer would resize it
    // too, but it would also change the eyes' convergence on a panel you look at while flying.
    const float heightM = 0.33f;
    const float distM = 1.4f; // close, head-locked
    XrCompositionLayerQuad& q = p->fpsQuad;
    memset(&q, 0, sizeof(q));
    q.type = XR_TYPE_COMPOSITION_LAYER_QUAD;
    q.layerFlags =
        XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT; // transparent canvas alpha-keys out
    q.space = p->viewSpace; // head-locked
    q.eyeVisibility = XR_EYE_VISIBILITY_BOTH;
    q.subImage.swapchain = p->fpsSwapchain;
    q.subImage.imageRect.extent.width = w;
    q.subImage.imageRect.extent.height = h;
    q.subImage.imageArrayIndex = 0;
    q.pose.orientation.w = 1.0f;
    q.pose.position.y = 0.30f; // upper area of the view
    q.pose.position.z = -distM;
    q.size.width = heightM * aspect;
    q.size.height = heightM;
    p->fpsQuadPending = true;
    return true;
}

void OpenXRBackend::EndStereoFrame()
{
    Impl* p = m_impl;
    if (!p->inStereoFrame)
        return;

    // #DX12 п.5 A1: now that the eye command list(s) have executed + fenced (EndEyeFrame), the in-scene menu RTT is
    // fully rendered -> copy it into a UI swapchain image and finalize the head-locked quad staged by
    // SubmitInSceneMenuQuad. Deferred to here precisely because the RTT was drawn on the (then-unexecuted) eye list.
#ifdef _WIN32
    if (p->useD3D12 && p->menuQuadPending && p->menuTexD3D12 &&
        p->uiSwapchain != XR_NULL_HANDLE)
    {
        D3D12Texture* mt = (D3D12Texture*)p->menuTexD3D12;
        if (mt && mt->tex)
        {
            uint32_t idx = 0;
            XrSwapchainImageAcquireInfo ai = {
                XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
            if (XR_SUCCEEDED(
                    xrAcquireSwapchainImage(p->uiSwapchain, &ai, &idx)))
            {
                XrSwapchainImageWaitInfo wi = {
                    XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
                wi.timeout = XR_INFINITE_DURATION;
                if (XR_SUCCEEDED(xrWaitSwapchainImage(p->uiSwapchain, &wi)) &&
                    idx < p->uiImages12.size())
                    XrCopyD3D12TexToUiImage(
                        p->alloc12, p->list12, p->d3d12Queue, p->fence12,
                        p->fenceEvt12, &p->fenceVal12, mt->tex, mt->rtState,
                        p->uiImages12[idx].texture);
                XrSwapchainImageReleaseInfo ri = {
                    XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
                xrReleaseSwapchainImage(p->uiSwapchain, &ri);
            }
        }
        p->menuTexD3D12 = NULL;
    }

    // Artscout - 2026: #DX12 п.5 -- same deferred copy for the FPS quad: the FPS RTT was drawn on the eye list (now
    // executed+fenced), so copy it into the FPS swapchain image here.
    if (p->useD3D12 && p->fpsQuadPending && p->fpsTexD3D12 &&
        p->fpsSwapchain != XR_NULL_HANDLE)
    {
        D3D12Texture* ft = (D3D12Texture*)p->fpsTexD3D12;
        if (ft && ft->tex)
        {
            uint32_t idx = 0;
            XrSwapchainImageAcquireInfo ai = {
                XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
            if (XR_SUCCEEDED(
                    xrAcquireSwapchainImage(p->fpsSwapchain, &ai, &idx)))
            {
                XrSwapchainImageWaitInfo wi = {
                    XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
                wi.timeout = XR_INFINITE_DURATION;
                if (XR_SUCCEEDED(xrWaitSwapchainImage(p->fpsSwapchain, &wi)) &&
                    idx < p->fpsImages12.size())
                    XrCopyD3D12TexToUiImage(
                        p->alloc12, p->list12, p->d3d12Queue, p->fence12,
                        p->fenceEvt12, &p->fenceVal12, ft->tex, ft->rtState,
                        p->fpsImages12[idx].texture);
                XrSwapchainImageReleaseInfo ri = {
                    XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
                xrReleaseSwapchainImage(p->fpsSwapchain, &ri);
            }
        }
        p->fpsTexD3D12 = NULL;
    }
#endif // _WIN32 (D3D12 deferred menu/FPS quad copies)

    XrCompositionLayerProjection layer = {XR_TYPE_COMPOSITION_LAYER_PROJECTION};
    const bool haveLayer = !p->projViews.empty();
    if (haveLayer)
    {
        layer.space = p->appSpace;
        layer.viewCount = (uint32_t)p->projViews.size();
        layer.views = p->projViews.data();
    }
    // Artscout - 2026 (#59 VR menu): projection layer first, then head-locked quads ON TOP (menu, then FPS).
    XrCompositionLayerBaseHeader* layers[4];
    uint32_t nLayers = 0;
    if (haveLayer)
        layers[nLayers++] = (XrCompositionLayerBaseHeader*)&layer;
    if (p->menuQuadPending)
        layers[nLayers++] = (XrCompositionLayerBaseHeader*)&p->menuQuad;
    if (p->fpsQuadPending)
        layers[nLayers++] = (XrCompositionLayerBaseHeader*)&p->fpsQuad;
    if (p->subQuadPending)
        layers[nLayers++] =
            (XrCompositionLayerBaseHeader*)&p->subQuad; // #59 subtitles

    // Artscout - 2026: #DX12 п.5 -- one-shot dump of the submitted projection views (which swapchain handle + array
    // index each references). If xrEndFrame returns HANDLE_INVALID, this pinpoints the bad handle/layout.
    {
        static bool s_once = false;
        if (!s_once)
        {
            s_once = true;
            XrDbg("OpenXR: SUBMIT nLayers=%u projViews=%u haveLayer=%d\n",
                  nLayers, (unsigned)p->projViews.size(), (int)haveLayer);
            for (uint32_t e = 0; e < (uint32_t)p->projViews.size(); ++e)
                XrDbg("  projView[%u] swapchain=%p arrayIdx=%d rect=%dx%d\n", e,
                      (void*)p->projViews[e].subImage.swapchain,
                      (int)p->projViews[e].subImage.imageArrayIndex,
                      (int)p->projViews[e].subImage.imageRect.extent.width,
                      (int)p->projViews[e].subImage.imageRect.extent.height);
        }
    }

    XrFrameEndInfo fei = {XR_TYPE_FRAME_END_INFO};
    fei.displayTime = p->stereoFrameState.predictedDisplayTime;
    fei.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    fei.layerCount = nLayers;
    fei.layers = nLayers ? layers : NULL;
    XrResult efr = xrEndFrame(p->session, &fei);
    if (XR_FAILED(efr))
    {
        static int s_e = 0;
        if (s_e < 3)
        {
            s_e++;
            XrDbg("OpenXR: EndStereoFrame xrEndFrame -> %d (nLayers=%u)\n",
                  (int)efr, nLayers);
        }
    }

    p->inStereoFrame = false;
    p->currentEye = -1;
    p->menuQuadPending = false; // consumed this frame
    p->fpsQuadPending = false;
}

//=============================================================================
// Head pose accessor
//=============================================================================
bool OpenXRBackend::GetHeadPose(float outPos[3], float outQuat[4]) const
{
    if (!m_impl->haveHeadPose)
        return false;
    const XrPosef& h = m_impl->lastHeadPose;
    outPos[0] = h.position.x;
    outPos[1] = h.position.y;
    outPos[2] = h.position.z;
    outQuat[0] = h.orientation.x;
    outQuat[1] = h.orientation.y;
    outQuat[2] = h.orientation.z;
    outQuat[3] = h.orientation.w;
    return true;
}

bool OpenXRBackend::GetHeadYawPitchRoll(float* yaw, float* pitch,
                                        float* roll) const
{
    if (!m_impl->haveHeadAngles)
        return false;
    if (yaw)
        *yaw = m_impl->lastYaw;
    if (pitch)
        *pitch = m_impl->lastPitch;
    if (roll)
        *roll = m_impl->lastRoll;
    return true;
}

//----------------------------------------------------------------------------
// Artscout - 2026 (VR controllers, Phase 1): expose the latest per-hand input snapshot.
//----------------------------------------------------------------------------
int OpenXRBackend::GetActiveHand() const
{
    return m_impl ? m_impl->activeHand : 1;
}

bool OpenXRBackend::GetControllerState(int hand, ControllerState* out) const
{
    if (!m_impl || !out || hand < 0 || hand > 1)
        return false;
    const Impl::HandInput& hi = m_impl->hand[hand];
    out->aimValid = hi.aimValid;
    out->aimPos[0] = hi.aimPose.position.x;
    out->aimPos[1] = hi.aimPose.position.y;
    out->aimPos[2] = hi.aimPose.position.z;
    out->aimQuat[0] = hi.aimPose.orientation.x;
    out->aimQuat[1] = hi.aimPose.orientation.y;
    out->aimQuat[2] = hi.aimPose.orientation.z;
    out->aimQuat[3] = hi.aimPose.orientation.w;
    out->gripValid = hi.gripValid;
    out->gripPos[0] = hi.gripPose.position.x;
    out->gripPos[1] = hi.gripPose.position.y;
    out->gripPos[2] = hi.gripPose.position.z;
    out->gripQuat[0] = hi.gripPose.orientation.x;
    out->gripQuat[1] = hi.gripPose.orientation.y;
    out->gripQuat[2] = hi.gripPose.orientation.z;
    out->gripQuat[3] = hi.gripPose.orientation.w;
    out->trigger = hi.trigger;
    out->triggerDown = hi.triggerDown;
    out->squeeze = hi.squeeze;
    out->squeezeDown = hi.squeezeDown;
    out->thumbX = hi.thumbX;
    out->thumbY = hi.thumbY;
    out->buttonA = hi.aBtn;
    out->buttonB = hi.bBtn;
    return true;
}

bool OpenXRBackend::ControllerActive() const
{
    return m_impl && m_impl->inputReady &&
           m_impl->hand[m_impl->activeHand].aimValid;
}

bool OpenXRBackend::GetControllerAimBody(int hand, float origin[3],
                                         float dir[3]) const
{
    if (!m_impl || hand < 0 || hand > 1)
        return false;
    const Impl::HandInput& hi = m_impl->hand[hand];
    if (!hi.aimValid)
        return false;
    const float M2FT = 3.28084f;
    const XrVector3f& pos = hi.aimPose.position;
    const XrQuaternionf& q = hi.aimPose.orientation;
    // TransformCameraCentricPoint (the cockpit projection) wants a vector FROM THE CAMERA (head) to the
    // point -- so the ray origin must be the controller RELATIVE TO THE HEAD, not absolute in appSpace.
    // Subtract the head position (same appSpace), then map OpenXR (RH,+Y up) -> Falcon body (x=fwd=-z,
    // y=right=+x, z=down=-y), in feet.
    const XrVector3f& hp = m_impl->lastHeadPose.position;
    float rx = pos.x - hp.x, ry = pos.y - hp.y, rz = pos.z - hp.z;
    // Empirically matched to the cockpit BUTTON frame (loc): a front-console button is (x<0, y>0, z<0),
    // while the naive GetHeadPosFeet map gave the ray (x>0, y>0, z>0). y matches; x & z are inverted (a
    // 180deg turn about the vertical). So x=+(-z_xr->flip)=+rz, y=+rx, z=+ry (feet).
    origin[0] = rz * M2FT;
    origin[1] = rx * M2FT;
    origin[2] = ry * M2FT;
    // Forward = rotate the quaternion by (0,0,-1) [OpenXR forward], i.e. -(third column of R):
    float fx = -2.0f * (q.x * q.z + q.w * q.y);
    float fy = -2.0f * (q.y * q.z - q.w * q.x);
    float fz = -(1.0f - 2.0f * (q.x * q.x + q.y * q.y));
    // Same (flipped x/z) body axis map for the direction (no metre scale; normalize).
    float bx = fz, by = fx, bz = fy;
    float len = sqrtf(bx * bx + by * by + bz * bz);
    if (len < 1e-6f)
        return false;
    dir[0] = bx / len;
    dir[1] = by / len;
    dir[2] = bz / len;
    return true;
}

// Grip pose position (where the hand holds the controller) in the Falcon BODY frame, relative to the head
// -- same flipped axis map as GetControllerAimBody. For drawing a controller marker. False if no grip pose.
bool OpenXRBackend::GetControllerGripBody(int hand, float origin[3]) const
{
    if (!m_impl || hand < 0 || hand > 1)
        return false;
    const Impl::HandInput& hi = m_impl->hand[hand];
    if (!hi.gripValid)
        return false;
    const float M2FT = 3.28084f;
    const XrVector3f& pos = hi.gripPose.position;
    const XrVector3f& hp = m_impl->lastHeadPose.position;
    float rx = pos.x - hp.x, ry = pos.y - hp.y, rz = pos.z - hp.z;
    origin[0] = rz * M2FT;
    origin[1] = rx * M2FT;
    origin[2] = ry * M2FT;
    return true;
}

// Artscout - 2026 (VR controller model): the runtime's CURRENT interaction profile path for a hand
// (e.g. "/interaction_profiles/valve/index_controller"), so the caller can pick which controller mesh to
// draw (Index vs Touch/other). Empty string if unknown. Uses xrGetCurrentInteractionProfile + xrPathToString.
bool OpenXRBackend::GetInteractionProfile(int hand, char* out, int cap) const
{
    if (!m_impl || !out || cap < 1 || hand < 0 || hand > 1)
        return false;
    out[0] = 0;
    if (m_impl->session == XR_NULL_HANDLE ||
        m_impl->handPath[hand] == XR_NULL_PATH)
        return false;
    XrInteractionProfileState ips = {XR_TYPE_INTERACTION_PROFILE_STATE};
    if (XR_FAILED(xrGetCurrentInteractionProfile(m_impl->session,
                                                 m_impl->handPath[hand], &ips)))
        return false;
    if (ips.interactionProfile == XR_NULL_PATH)
        return false;
    uint32_t len = 0;
    if (XR_FAILED(xrPathToString(m_impl->instance, ips.interactionProfile,
                                 (uint32_t)cap, &len, out)))
    {
        out[0] = 0;
        return false;
    }
    return out[0] != 0;
}

// Artscout - 2026 (VR controller model): grip ORIENTATION as a body-frame basis (fwd/right/up, unit) so a
// mesh can be oriented like the real controller. Same axis map as GetControllerAimBody's direction
// (body = (z,x,y) of the OpenXR vector). The caller applies the same VrRayFlipH/V it uses for the ray.
bool OpenXRBackend::GetControllerGripBasis(int hand, float fwd[3],
                                           float right[3], float up[3]) const
{
    if (!m_impl || hand < 0 || hand > 1)
        return false;
    const Impl::HandInput& hi = m_impl->hand[hand];
    if (!hi.gripValid)
        return false;
    const XrQuaternionf& q = hi.gripPose.orientation;
    // Column vectors of the rotation matrix in OpenXR axes.
    float rx[3] = {1.0f - 2.0f * (q.y * q.y + q.z * q.z),
                   2.0f * (q.x * q.y + q.w * q.z),
                   2.0f * (q.x * q.z - q.w * q.y)}; // R*(1,0,0) = right
    float uy[3] = {2.0f * (q.x * q.y - q.w * q.z),
                   1.0f - 2.0f * (q.x * q.x + q.z * q.z),
                   2.0f * (q.y * q.z + q.w * q.x)}; // R*(0,1,0) = up
    float fz[3] = {
        -2.0f * (q.x * q.z + q.w * q.y), -2.0f * (q.y * q.z - q.w * q.x),
        -(1.0f - 2.0f * (q.x * q.x + q.y * q.y))}; // R*(0,0,-1) = forward
    // Map OpenXR (vx,vy,vz) -> Falcon body (vz, vx, vy), same as GetControllerAimBody.
    fwd[0] = fz[2];
    fwd[1] = fz[0];
    fwd[2] = fz[1];
    right[0] = rx[2];
    right[1] = rx[0];
    right[2] = rx[1];
    up[0] = uy[2];
    up[1] = uy[0];
    up[2] = uy[1];
    return true;
}

// Artscout - 2026 (VR hands): true if the runtime returned a valid hand skeleton for THIS hand this frame.
// The caller (vcock) draws the skeleton when true, else falls back to the wireframe controller.
bool OpenXRBackend::HandJointsValid(int hand) const
{
    return m_impl && hand >= 0 && hand < 2 && m_impl->handTrackingEnabled &&
           m_impl->handJointsValid[hand];
}

// Artscout - 2026 (VR hands): the 26 hand joints in the Falcon BODY frame (feet, relative to the head) --
// the SAME axis map as GetControllerGripBody (x=fwd=+z_xr, y=right=+x_xr, z=down=+y_xr). out must hold
// XR_HAND_JOINT_COUNT_EXT (26) xyz triples. Only joints with a valid position are written; validOut[j]
// flags which. Returns false if no valid skeleton this frame.
bool OpenXRBackend::GetHandJointsBody(int hand, float out[][3],
                                      bool validOut[]) const
{
    if (!HandJointsValid(hand))
        return false;
    const float M2FT = 3.28084f;
    const XrVector3f& hp = m_impl->lastHeadPose.position;
    const XrHandJointLocationEXT* j = m_impl->handJoints[hand];
    for (int i = 0; i < XR_HAND_JOINT_COUNT_EXT; ++i)
    {
        bool ok =
            (j[i].locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0;
        validOut[i] = ok;
        if (!ok)
        {
            out[i][0] = out[i][1] = out[i][2] = 0.0f;
            continue;
        }
        const XrVector3f& pos = j[i].pose.position;
        float rx = pos.x - hp.x, ry = pos.y - hp.y, rz = pos.z - hp.z;
        out[i][0] = rz * M2FT;
        out[i][1] = rx * M2FT;
        out[i][2] = ry * M2FT;
    }
    return true;
}

bool OpenXRBackend::GetHandJointsBodyPose(int hand, float outMat[][12],
                                          bool validOut[]) const
{
    if (!HandJointsValid(hand))
        return false;
    const float M2FT = 3.28084f;
    const XrVector3f& hp = m_impl->lastHeadPose.position;
    const XrHandJointLocationEXT* j = m_impl->handJoints[hand];
    // Body frame is the XR frame with axes permuted: body=(xr.z, xr.x, xr.y) -- the SAME map
    // GetHandJointsBody applies to positions (out=(rz,rx,ry)). As a rotation P it is a pure cyclic
    // permutation (det +1, no reflection), so a joint's body rotation is Rbody = P*Rxr*P^T, which is just
    // Rbody[i][k] = Rxr[perm[i]][perm[k]] with perm = {2,0,1} (body axis -> xr axis). Position is rel. head.
    const int perm[3] = {2, 0, 1};
    for (int i = 0; i < XR_HAND_JOINT_COUNT_EXT; ++i)
    {
        const unsigned need = XR_SPACE_LOCATION_POSITION_VALID_BIT |
                              XR_SPACE_LOCATION_ORIENTATION_VALID_BIT;
        bool ok = (j[i].locationFlags & need) == need;
        validOut[i] = ok;
        if (!ok)
        {
            for (int k = 0; k < 12; ++k)
                outMat[i][k] = (k == 0 || k == 5 || k == 10) ? 1.0f : 0.0f;
            continue;
        }
        // XR quaternion -> 3x3 rotation (columns are the rotated basis).
        const XrQuaternionf& q = j[i].pose.orientation;
        const float x = q.x, y = q.y, z = q.z, w = q.w;
        float Rxr[3][3] = {
            {1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w)},
            {2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w)},
            {2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y)}};
        const XrVector3f& pos = j[i].pose.position;
        float rel[3] = {pos.x - hp.x, pos.y - hp.y, pos.z - hp.z};
        for (int r = 0; r < 3; ++r)
        {
            outMat[i][r * 4 + 0] = Rxr[perm[r]][perm[0]];
            outMat[i][r * 4 + 1] = Rxr[perm[r]][perm[1]];
            outMat[i][r * 4 + 2] = Rxr[perm[r]][perm[2]];
            outMat[i][r * 4 + 3] = rel[perm[r]] * M2FT; // body pos = (rz,rx,ry)*M2FT
        }
    }
    return true;
}

// Artscout - 2026 (#67 VR recenter): rebuild the app reference space so the user's CURRENT head pose
// (yaw + position) becomes the origin -- fixes the view drifting off (and "flying into the ground" when
// the runtime recenters under us). Standard seated recenter: locate the head in the current appSpace, take
// position + the YAW-ONLY part of the orientation (swing-twist about Y; pitch/roll dropped so the horizon
// stays level), and make a NEW LOCAL space at that offset. The head, located in the new space, lands at the
// origin facing forward -> the cockpit camera (which reads GetHead*) recenters. Drive from the sim thread.
// Public: request a recenter from ANY thread. The actual appSpace swap is done by the render thread in
// BeginStereoFrame (it owns appSpace + has a fresh predicted display time), avoiding a destroy-while-in-use race.
bool OpenXRBackend::Recenter()
{
    if (!m_impl->session)
        return false;
    m_impl->recenterPending = true;
    return true;
}


// Artscout - 2026: HMD head orientation as a Falcon-body basis (forward/right/up), gimbal-free.
// Replaces the Euler (yaw/pitch/roll -> BuildHeadMatrix) path which divided by zero / flipped at
// pitch = +-90 (look straight down at your feet) and wandered near the poles. OpenXR is RH
// (x=right, y=up, z=back); Falcon body is x=fwd, y=right, z=down -> map (vx,vy,vz)->(-vz,vx,-vy).
// The engine's head matrix columns are [at(forward), rt(right), up], so we hand those back directly.
bool OpenXRBackend::GetHeadBasis(float at[3], float right[3], float up[3]) const
{
    if (!m_impl->haveHeadPose)
        return false;
    const XrQuaternionf& q = m_impl->lastHeadPose.orientation;
    const float x = q.x, y = q.y, z = q.z, w = q.w;
    // forward = R*(0,0,-1), up = R*(0,1,0)  (OpenXR frame)
    const float fx = -2.0f * (x * z + w * y), fy = 2.0f * (w * x - y * z),
                fz = 2.0f * (x * x + y * y) - 1.0f;
    const float ux = 2.0f * (x * y - w * z), uy = 1.0f - 2.0f * (x * x + z * z),
                uz = 2.0f * (y * z + w * x);
    // Convert to Falcon body. 'at' matches the proven Euler forward (-fz,fx,-fy); 'upc' is the
    // engine's third-column convention (body-down at identity), so head roll comes through naturally.
    float at_[3] = {-fz, fx, -fy};
    float upc[3] = {uz, -ux, uy};
    auto norm3 = [](float v[3])
    {
        float s = sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
        if (s > 1e-6f)
        {
            v[0] /= s;
            v[1] /= s;
            v[2] /= s;
        }
    };
    norm3(at_);
    // rt = upc x at  ; up = at x rt  (re-orthonormalize). at & up are perpendicular -> no singularity.
    float rt_[3] = {upc[1] * at_[2] - upc[2] * at_[1],
                    upc[2] * at_[0] - upc[0] * at_[2],
                    upc[0] * at_[1] - upc[1] * at_[0]};
    norm3(rt_);
    float up_[3] = {at_[1] * rt_[2] - at_[2] * rt_[1],
                    at_[2] * rt_[0] - at_[0] * rt_[2],
                    at_[0] * rt_[1] - at_[1] * rt_[0]};
    norm3(up_);
    if (at)
    {
        at[0] = at_[0];
        at[1] = at_[1];
        at[2] = at_[2];
    }
    if (right)
    {
        right[0] = rt_[0];
        right[1] = rt_[1];
        right[2] = rt_[2];
    }
    if (up)
    {
        up[0] = up_[0];
        up[1] = up_[1];
        up[2] = up_[2];
    }
    return true;
}

// Artscout - 2026 (VR quad-views): per-VIEW orientation basis (Falcon body frame), from this frame's
// located view pose. Same conversion as GetHeadBasis but for views[eye].pose -- needed because the
// quad-views FOCUS views (2,3) are GAZE-tracked: their pose looks where the eyes look, not straight
// ahead. Rendering them from the head orientation (GetHeadBasis) while the compositor expects the gaze
// pose makes the focus inset double + the cockpit/displays "follow the gaze". Use this so render
// orientation == submitted view pose. Falls back to head basis logic if the eye index is invalid.
bool OpenXRBackend::GetEyeBasis(int eye, float at[3], float right[3],
                                float up[3]) const
{
    Impl* p = m_impl;
    if (eye < 0 || eye >= (int)p->views.size())
        return false;
    const XrQuaternionf& q = p->views[eye].pose.orientation;
    const float x = q.x, y = q.y, z = q.z, w = q.w;
    const float fx = -2.0f * (x * z + w * y), fy = 2.0f * (w * x - y * z),
                fz = 2.0f * (x * x + y * y) - 1.0f;
    const float ux = 2.0f * (x * y - w * z), uy = 1.0f - 2.0f * (x * x + z * z),
                uz = 2.0f * (y * z + w * x);
    float at_[3] = {-fz, fx, -fy};
    float upc[3] = {uz, -ux, uy};
    auto norm3 = [](float v[3])
    {
        float s = sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
        if (s > 1e-6f)
        {
            v[0] /= s;
            v[1] /= s;
            v[2] /= s;
        }
    };
    norm3(at_);
    float rt_[3] = {upc[1] * at_[2] - upc[2] * at_[1],
                    upc[2] * at_[0] - upc[0] * at_[2],
                    upc[0] * at_[1] - upc[1] * at_[0]};
    norm3(rt_);
    float up_[3] = {at_[1] * rt_[2] - at_[2] * rt_[1],
                    at_[2] * rt_[0] - at_[0] * rt_[2],
                    at_[0] * rt_[1] - at_[1] * rt_[0]};
    norm3(up_);
    if (at)
    {
        at[0] = at_[0];
        at[1] = at_[1];
        at[2] = at_[2];
    }
    if (right)
    {
        right[0] = rt_[0];
        right[1] = rt_[1];
        right[2] = rt_[2];
    }
    if (up)
    {
        up[0] = up_[0];
        up[1] = up_[1];
        up[2] = up_[2];
    }
    return true;
}

// Artscout - 2026: HMD head position (relative to the recentered LOCAL origin), converted to the
// Falcon aircraft body frame in FEET, for 6DOF positional tracking (lean in/out/sideways). OpenXR
// is RH: x=right, y=up, z=back. Falcon body: x=forward, y=right, z=down.
bool OpenXRBackend::GetHeadPosFeet(float* fwd, float* right, float* down) const
{
    if (!m_impl->haveHeadPose)
        return false;
    const XrVector3f& p = m_impl->lastHeadPose.position;
    const float M2FT = 3.28084f;
    if (fwd)
        *fwd = -p.z * M2FT; // OpenXR -z = forward
    if (right)
        *right = p.x * M2FT; // OpenXR +x = right
    if (down)
        *down = -p.y * M2FT; // OpenXR +y = up -> body +z is down
    return true;
}

//=============================================================================
// Shutdown
//=============================================================================
void OpenXRBackend::Shutdown()
{
    Impl* p = m_impl;
    if (!p)
        return;

    for (size_t e = 0; e < p->swapchains.size(); ++e)
    {
#ifdef _WIN32
        for (size_t i = 0; i < p->swapchains[e].rtvs.size(); ++i)
            if (p->swapchains[e].rtvs[i])
                p->swapchains[e]
                    .rtvs[i]
                    ->Release(); // D3D11 RTVs (D3D12 RTVs live in rtvHeap12)
#endif
        if (p->swapchains[e].handle != XR_NULL_HANDLE)
            xrDestroySwapchain(p->swapchains[e].handle);
    }
    p->swapchains.clear();

#ifdef _WIN32
    // #DX12 п.5: release the D3D12 VR resources (borrowed device/queue are NOT released).
    if (p->fenceEvt12)
    {
        CloseHandle(p->fenceEvt12);
        p->fenceEvt12 = NULL;
    }
    if (p->list12)
    {
        p->list12->Release();
        p->list12 = NULL;
    }
    if (p->alloc12)
    {
        p->alloc12->Release();
        p->alloc12 = NULL;
    }
    if (p->fence12)
    {
        p->fence12->Release();
        p->fence12 = NULL;
    }
    if (p->rtvHeap12)
    {
        p->rtvHeap12->Release();
        p->rtvHeap12 = NULL;
    }

    if (p->uiStaging)
    {
        p->uiStaging->Release();
        p->uiStaging = NULL;
    }
    p->uiImages.clear();
    if (p->uiUpload12)
    {
        p->uiUpload12->Release();
        p->uiUpload12 = NULL;
    } // #DX12 п.5
    p->uiImages12.clear();
#endif // _WIN32
    p->uiImagesVk.clear(); // #107 VR-Vulkan
    if (p->uiSwapchain != XR_NULL_HANDLE)
    {
        xrDestroySwapchain(p->uiSwapchain);
        p->uiSwapchain = XR_NULL_HANDLE;
    }
    if (p->fpsSwapchain != XR_NULL_HANDLE)
    {
        xrDestroySwapchain(p->fpsSwapchain);
        p->fpsSwapchain = XR_NULL_HANDLE;
    }

    // Artscout - 2026 (VR hands): destroy the hand trackers.
    if (p->pfnDestroyHandTracker)
        for (int h = 0; h < 2; ++h)
            if (p->handTracker[h] != XR_NULL_HANDLE)
            {
                p->pfnDestroyHandTracker(p->handTracker[h]);
                p->handTracker[h] = XR_NULL_HANDLE;
            }

    // Artscout - 2026 (VR controllers): tear down input action spaces + set.
    for (int h = 0; h < 2; ++h)
    {
        if (p->aimSpace[h] != XR_NULL_HANDLE)
        {
            xrDestroySpace(p->aimSpace[h]);
            p->aimSpace[h] = XR_NULL_HANDLE;
        }
        if (p->gripSpace[h] != XR_NULL_HANDLE)
        {
            xrDestroySpace(p->gripSpace[h]);
            p->gripSpace[h] = XR_NULL_HANDLE;
        }
    }
    if (p->actionSet != XR_NULL_HANDLE)
    {
        xrDestroyActionSet(p->actionSet);
        p->actionSet = XR_NULL_HANDLE;
    }
    p->inputReady = false;

    if (p->viewSpace != XR_NULL_HANDLE)
    {
        xrDestroySpace(p->viewSpace);
        p->viewSpace = XR_NULL_HANDLE;
    }
    if (p->appSpace != XR_NULL_HANDLE)
    {
        xrDestroySpace(p->appSpace);
        p->appSpace = XR_NULL_HANDLE;
    }
    if (p->localRef != XR_NULL_HANDLE)
    {
        xrDestroySpace(p->localRef);
        p->localRef = XR_NULL_HANDLE;
    }
    XrWaitGateStop(
        p); // #107 turbo: join the wait thread before the session it waits on is destroyed
    if (p->session != XR_NULL_HANDLE)
    {
        xrDestroySession(p->session);
        p->session = XR_NULL_HANDLE;
    }
    if (p->instance != XR_NULL_HANDLE)
    {
        xrDestroyInstance(p->instance);
        p->instance = XR_NULL_HANDLE;
    }

#ifdef _WIN32
    if (p->ctx)
    {
        p->ctx->Release();
        p->ctx = NULL;
    }
    p->device = NULL;
#endif
    p->sessionRunning = false;
}

//=============================================================================
// RunVulkanMenuFrame -- #107 VR-Vulkan menu/splash headset frame (main thread). Exact peer of the D3D12 RunMenuFrame:
// convert the cached 565 UI to RGBA (+ a cursor crosshair), upload it into the Vulkan menu-quad swapchain image, and
// submit it as a WORLD-FIXED head-height quad (2.1 m forward, 1.3 m tall) -- the "cinema screen", not a full-eye blit.
#ifndef _WIN32
// #108 Linux VR menu cursor: maps the SDL window mouse position into panel pixels (impl in ffplatform/ff_events.cpp).
extern "C" bool FF_GetMenuCursorPx(int panelW, int panelH, int* outX,
                                   int* outY);
#endif

bool OpenXRBackend::RunVulkanMenuFrame(const void* src565, int srcW, int srcH)
{
    Impl* p = m_impl;
    if (!p->instance || p->session == XR_NULL_HANDLE)
        return false;
    PollEvents();
    if (!p->sessionRunning)
        return false;
    if (!EnsureUiSwapchain(srcW, srcH))
        return false; // no panel yet -> nothing to submit this frame

    XrFrameWaitInfo fwi = {XR_TYPE_FRAME_WAIT_INFO};
    XrFrameState fs = {XR_TYPE_FRAME_STATE};
    if (XR_FAILED(XrWaitGated(p, &fs)))
        return false; // #107 turbo-gated
    XrFrameBeginInfo fbi = {XR_TYPE_FRAME_BEGIN_INFO};
    if (XR_FAILED(xrBeginFrame(p->session, &fbi)))
        return false;

    XrCompositionLayerQuad quad = {XR_TYPE_COMPOSITION_LAYER_QUAD};
    bool haveLayer = false;
    (void)src565;
    const unsigned char* srcSnap = XrMenuSnapshot(srcW, srcH);
    if (fs.shouldRender && srcSnap && srcW > 0 && srcH > 0)
    {
        unsigned char* rgba = (unsigned char*)malloc((size_t)srcW * srcH * 4);
        if (rgba && XrConvertMenu565ToRGBA(srcSnap, srcW, srcH, rgba,
                                           (unsigned)srcW * 4))
        {
            // VR cursor crosshair at the real pointer, mapped window-client -> panel pixels (mirror of D3D12).
#ifdef _WIN32
            HWND hwnd =
                g_pVulkanBackend ? (HWND)g_pVulkanBackend->Hwnd() : NULL;
            POINT pt;
            RECT rc;
            if (hwnd && GetCursorPos(&pt) && GetClientRect(hwnd, &rc))
            {
                ScreenToClient(hwnd, &pt);
                const int cw = rc.right - rc.left, ch = rc.bottom - rc.top;
                if (cw > 0 && ch > 0)
                {
                    const int cx = (int)((long long)pt.x * srcW / cw),
                              cy = (int)((long long)pt.y * srcH / ch), arm = 9;
#define XRVK_SETPX(X, Y, V)                                                    \
    do                                                                         \
    {                                                                          \
        int _x = (X), _y = (Y);                                                \
        if (_x >= 0 && _x < srcW && _y >= 0 && _y < srcH)                      \
        {                                                                      \
            unsigned char* _o = rgba + ((size_t)_y * srcW + _x) * 4;           \
            _o[0] = _o[1] = _o[2] = (unsigned char)(V);                        \
            _o[3] = 255;                                                       \
        }                                                                      \
    } while (0)
                    // Artscout - 2026: the REAL desktop cursor on Windows; the
                    // crosshair stays for Linux and as the fallback.
                    bool drawnWin = false;
#ifdef _WIN32

                    if (g_bVrWindowsCursor)
                        drawnWin = FF_BlitWinCursorRGBA(rgba, srcW, srcH, cx, cy);
#endif
                    if (!drawnWin)
                    {
                        for (int d = -arm; d <= arm; ++d)
                        {
                            XRVK_SETPX(cx + d, cy - 1, 0);
                            XRVK_SETPX(cx + d, cy + 1, 0);
                            XRVK_SETPX(cx - 1, cy + d, 0);
                            XRVK_SETPX(cx + 1, cy + d, 0);
                        }
                        for (int d = -arm; d <= arm; ++d)
                        {
                            XRVK_SETPX(cx + d, cy, 255);
                            XRVK_SETPX(cx, cy + d, 255);
                        }
                    }
#undef XRVK_SETPX
                }
            }
#else
            // #108 Linux: paint the crosshair from the SDL cursor (FF_GetMenuCursorPx maps window->panel px).
            // Do NOT use GetCursorPos/GetClientRect here -- GetClientRect returns 0x0 on the Linux shim.
            int cx = 0, cy = 0;
            if (FF_GetMenuCursorPx(srcW, srcH, &cx, &cy))
            {
                const int arm = 9;
#define XRVK_SETPX(X, Y, V)                                                    \
    do                                                                         \
    {                                                                          \
        int _x = (X), _y = (Y);                                                \
        if (_x >= 0 && _x < srcW && _y >= 0 && _y < srcH)                      \
        {                                                                      \
            unsigned char* _o = rgba + ((size_t)_y * srcW + _x) * 4;           \
            _o[0] = _o[1] = _o[2] = (unsigned char)(V);                        \
            _o[3] = 255;                                                       \
        }                                                                      \
    } while (0)
                for (int d = -arm; d <= arm; ++d)
                {
                    XRVK_SETPX(cx + d, cy - 1, 0);
                    XRVK_SETPX(cx + d, cy + 1, 0);
                    XRVK_SETPX(cx - 1, cy + d, 0);
                    XRVK_SETPX(cx + 1, cy + d, 0);
                }
                for (int d = -arm; d <= arm; ++d)
                {
                    XRVK_SETPX(cx + d, cy, 255);
                    XRVK_SETPX(cx, cy + d, 255);
                }
#undef XRVK_SETPX
            }
#endif

            uint32_t idx = 0;
            XrSwapchainImageAcquireInfo ai = {
                XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
            if (XR_SUCCEEDED(
                    xrAcquireSwapchainImage(p->uiSwapchain, &ai, &idx)))
            {
                XrSwapchainImageWaitInfo wi = {
                    XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
                wi.timeout = XR_INFINITE_DURATION;
                if (XR_SUCCEEDED(xrWaitSwapchainImage(p->uiSwapchain, &wi)) &&
                    idx < p->uiImagesVk.size() && g_pVulkanBackend)
                    g_pVulkanBackend->UploadRgbaToXrImage(
                        (void*)p->uiImagesVk[idx].image, srcW, srcH, rgba,
                        srcW * 4);
                XrSwapchainImageReleaseInfo ri = {
                    XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
                xrReleaseSwapchainImage(p->uiSwapchain, &ri);

                const float aspect = (float)srcW / (float)srcH, heightM = 1.3f,
                            distM = 2.1f;
                quad.layerFlags = 0;
                quad.space = p->appSpace;
                quad.eyeVisibility = XR_EYE_VISIBILITY_BOTH;
                quad.subImage.swapchain = p->uiSwapchain;
                quad.subImage.imageRect.offset.x = 0;
                quad.subImage.imageRect.offset.y = 0;
                quad.subImage.imageRect.extent.width = srcW;
                quad.subImage.imageRect.extent.height = srcH;
                quad.subImage.imageArrayIndex = 0;
                quad.pose.orientation.x = quad.pose.orientation.y =
                    quad.pose.orientation.z = 0.0f;
                quad.pose.orientation.w = 1.0f;
                quad.pose.position.x = 0.0f;
                quad.pose.position.y = 0.0f;
                quad.pose.position.z = -distM;
                quad.size.width = heightM * aspect;
                quad.size.height = heightM;
                haveLayer = true;
            }
        }
        if (rgba)
            free(rgba);
    }

    XrCompositionLayerBaseHeader* layers[1] = {
        (XrCompositionLayerBaseHeader*)&quad};
    XrFrameEndInfo fei = {XR_TYPE_FRAME_END_INFO};
    fei.displayTime = fs.predictedDisplayTime;
    fei.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    fei.layerCount = haveLayer ? 1 : 0;
    fei.layers = haveLayer ? layers : NULL;
    xrEndFrame(p->session, &fei);
    return true;
}

//=============================================================================
// OpenXR_PumpFrame -- single-threaded XR frame driver, called from the main
// message loop so the headset gets continuous frames even though the 2D UI only
// repaints on change. In the 3D world: eye layer (M1 clear). In menus: the cached
// flat UI as a quad panel. xrWaitFrame inside paces this to the headset rate.
//=============================================================================
bool OpenXR_PumpFrame()
{
    if (!g_bUseOpenXR || g_pOpenXRBackend == NULL)
        return false;

    // Sole owner of the XR frame loop (main thread). RunFrame/RunMenuFrame poll events first (so the session starts
    // on READY), then submit a frame.
    if (g_intellivibeData.In3D)
    {
        // In 3D the scene is rendered on the SIM thread; driving the XR frame from this (main) thread too contends on
        // the device and starves the sim -> hangs. So the main pump does NOTHING in 3D (the sim path drives the eye
        // frames). Also drop the menu cache (the menu ImageBuffer is freed/recreated across 3D entry/exit -> dangling).
        g_pXrMenuSurface565 = NULL;
        return false;
    }

    // #107 VR-Vulkan: the D3D menu pump (RunFrame / RunMenuFrame) is D3D-only. Under Vulkan the menu/splash headset
    // frame is its own driver (RunVulkanMenuFrame): blit the cached 565 UI into both eyes as a full-eye background +
    // submit the projection layer (a NULL surface submits an empty frame -> keeps the session paced without content).
    {
        extern bool g_bUseVulkan;
        if (g_bUseVulkan)
            return g_pOpenXRBackend->RunVulkanMenuFrame(g_pXrMenuSurface565,
                                                        g_xrMenuW, g_xrMenuH);
    }

    if (g_pXrMenuSurface565)
        return g_pOpenXRBackend->RunMenuFrame(g_pXrMenuSurface565, g_xrMenuW,
                                              g_xrMenuH);
    return g_pOpenXRBackend->RunFrame(
        NULL, NULL); // menu, no UI cached yet: poll + clear
}
