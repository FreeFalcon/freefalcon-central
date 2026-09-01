//-----------------------------------------------------------------------------
// VulkanBackend.h -- Artscout - 2026 (#104, Linux port -- subsystem 2: renderer backend).
//
// The Linux implementation of IRenderBackend (device + swapchain + present + the 2D blit), the peer of the
// Windows D3D12Backend. The engine talks to whichever is active through the single global g_pRenderBackend, so
// there is NO call-site churn and no #ifdef at the call sites -- this is a proper backend split, selected once
// where g_pRenderBackend is created (devmgr.cpp). The Windows D3D12 path is untouched.
//
// Init(HWND) reuses the neutral signature: on Linux the "HWND" carries the ffplatform::Window* token (that is what
// dispcfg sets appWin to on Linux), from which we build the Vulkan surface via SDL_Vulkan_CreateSurface.
//
// All Vulkan/SDL detail is hidden behind a pimpl so this header stays free of <vulkan/vulkan.h> and SDL.
//-----------------------------------------------------------------------------
#ifndef FF_VULKAN_BACKEND_H
#define FF_VULKAN_BACKEND_H

#include "../dxengine/irenderbackend.h" // the neutral surface (pulls <windows.h> for HWND on both platforms)

class VulkanBackend : public IRenderBackend
{
public:
    VulkanBackend();
    ~VulkanBackend() override;

    // Headless bring-up: a full device + an offscreen color+depth target standing in for the swapchain (no SDL
    // surface, no present). Enables server/offscreen rendering AND GPU tests under a software ICD (lavapipe). The
    // flat frame loop (BeginFrame/Present) drives the offscreen target; ReadbackColor pulls the pixels back.
    bool InitHeadless(int nWidth, int nHeight);
    // Copy the offscreen (headless) color target into a host RGBA8 buffer (dst = w*h*4 bytes). No-op unless headless.
    void ReadbackColor(void* dstRgba);

    // ---- IRenderBackend ----
    bool Init(HWND hWnd, int nWidth, int nHeight, int nDepth,
              bool bFullscreen) override;
    void Release() override;
    bool Resize(int nWidth, int nHeight) override;
    bool IsValid() const override;

    void BeginFrame(unsigned long argbClearColor) override;
    void BindBackBuffer(bool bClearDepth) override;
    void ClearDepth() override;
    void SetViewportRect(int x, int y, int w, int h) override;
    // Artscout - 2026 (#104): no-op -- Vulkan has no MSAA yet. NOTE this is NOT parity: D3D12Backend does implement it
    // (MsaaActive/CurrentSampleCount + a real resolve), so the Vulkan scene is aliased next to the Windows one. The
    // multisample target + resolve attachment + per-sample-count pipelines belong to the #107 VR/MSAA phase.
    void ResolveMsaaToBackBuffer() override;
    void Present(bool bVSync) override;

    void BlitBitmap565(const void* pSrc565, int srcW, int srcH) override;
    void SetGScreenSize(int w, int h) override;
    void ClearCurrentRTV(float r, float g, float b, float a) override;
    void FlushContext() override;

    int Width() const override;
    int Height() const override;
    HWND Hwnd() const override;

    // ---- multiview / quad-views (VR) -----------------------------------------------------------------------
    // The Vulkan peer of the D3D12 view-instancing path (SV_ViewID): VK_KHR_multiview renders the scene ONCE into
    // an N-layer array target, the vertex shader picking a per-view matrix by gl_ViewIndex. N = 2 (stereo) or 4
    // (quad-views: focus + periphery per eye). Enabled on the device at Init; VulkanRenderer builds its scene
    // pipelines against GetSceneRenderPass() so every scene pipeline is multiview-capable from day one. The array
    // layers are then handed to OpenXR (VR) or copied to the window (desktop mirror) -- wired in the OpenXR phase.
    bool MultiviewSupported() const;
    bool BindlessSupported()
        const; // #107 PERF: descriptor-indexing enabled (bindless terrain)
    bool MeshShaderSupported()
        const; // #78: VK_EXT_mesh_shader enabled (GPU-driven terrain)
    // Records a mesh-task draw into `cmd` (the entry point is device-level).
    void CmdDrawMeshTasks(void* cmd, unsigned groupsX) const;
    bool IsSoftwareRasterizer()
        const; // VkPhysicalDeviceType == CPU (e.g. lavapipe) -- some formats (BC) unreliable
    // (Re)create the N-view scene array target at this per-view size. nViews in [1..4].
    bool EnsureSceneTarget(int perViewW, int perViewH, int nViews);
    // #107 VR-Vulkan: this eye's render sub-rect within the (reused, max-sized) scene target. The scene renders into
    // [0,0,w,h] and the per-eye blit copies only that. 0,0 -> whole target (flat path). Avoids per-eye reallocation.
    void SetSceneRenderSize(int w, int h);
    void BeginSceneMultiview(
        unsigned long
            argbClear); // begin the multiview render pass on the array target
    void EndSceneMultiview(); // end it (leaves the array in SHADER_READ layout)
    // #107 Option 2: per-eye tail pass -- draw the RTT/2D tail (screen-space, per-eye camera) into ONE scene-array
    // layer via a single-view LOAD-color pass, so the composite gets correct per-eye parallax. Call between
    // EndSceneMultiview and the per-group blit; synchronous (EndTailView waits its fence).
    void BeginTailView(int layer);
    void
    TailViewport2D(); // switch to POSITIVE-height viewport for the 2D RTT composite (cockpit BSP keeps negative)
    void EndTailView();
    void
    WaitTailsConsumed(); // #107 PERF: block until pending per-eye tail submits finish (call before reading their scene layers)
    // #107 PERF: the profiler is now backend-neutral free functions -- see graphics/include/frameprof.h.
    bool IsTailActive()
        const; // renderer routes 2D/RTT draws to the tail pass while true
    void* TailCommandBuffer() const; // VkCommandBuffer of the tail pass
    unsigned long long TailRenderPass()
        const; // VkRenderPass of the tail pass (single-view, LOAD color)
    void ReadbackScene(
        int layer,
        void*
            dstRgba); // copy one array layer of the scene color to a host RGBA buffer
    void*
    GetSceneArrayImage() const; // VkImage of the N-layer scene color (as void*)
    // #107 VR-Vulkan present: blit each scene-array layer into the matching per-view XR swapchain VkImage (dstImages[e],
    // sized dstW[e]xdstH[e]). Scene must be ended (color in SHADER_READ). Leaves each dst in COLOR_ATTACHMENT_OPTIMAL
    // and waits its own fence, so the caller may xrReleaseSwapchainImage right after. nViews clamped to the scene layers.
    bool BlitSceneToXrImages(int nViews, void* const* dstImages,
                             const int* dstW, const int* dstH);
    // Artscout - 2026: the VkFormat the runtime gave the XR color swapchains.
    // An sRGB one encodes on write, so those copies take a decoding shader.
    void SetXrColorFormat(unsigned int vkFormat);
    // #107 VR-Vulkan menu: blit a 565 UI surface (full-eye background) into each per-view XR image. Same layout/fence
    // contract as BlitSceneToXrImages. Used by the menu / splash headset frame (OpenXR_PumpFrame under Vulkan).
    bool Blit565ToXrImages(int nViews, void* const* dstImages, const int* dstW,
                           const int* dstH, const void* src565, int srcW,
                           int srcH);
    // #107 VR-Vulkan menu quad: upload an RGBA8 buffer (rowPitchBytes stride) into an XR menu-swapchain VkImage
    // (peer of D3D12 XrCopyRgbaToUiImageD3D12). Left in COLOR_ATTACHMENT_OPTIMAL, own fence, synchronous.
    bool UploadRgbaToXrImage(void* dstImage, int w, int h, const void* rgba,
                             int rowPitchBytes);
    // #107 VR-Vulkan in-3D menu (ESC / comms / exit dialog): Vulkan peer of D3D12Backend EnsureMenuRtt/BindMenuRtt/
    // MenuRttTex. The in-scene menu (2D comms text + the 3D exit dialog BSP) is drawn into an owned color+depth RTT
    // (BindMenuRtt routes draws through the RTT command buffer, same machinery as the display RTTs), then copied into
    // the XR UI swapchain image by SubmitInSceneMenuQuad. Without this the whole in-3D menu path was D3D12-only, so
    // DrawExitMenu never ran under Vulkan -> the exit-menu state machine never advanced -> ESC/keyboard felt dead.
    void EnsureMenuRtt(
        int w,
        int h); // (re)create the owned menu target (sceneFormat color + depth) at w x h
    void BindMenuRtt(
        bool
            clear); // begin the menu pass on menuCmd (color+depth cleared); menu draws route here
    void
    EndMenuRtt(); // end + submit menuCmd; menu color -> SHADER_READ for the copy
    bool IsMenuActive()
        const; // menu pass open -> object/screen draws route to menuCmd + MenuRenderPass()
    unsigned long long MenuRenderPass()
        const; // VkRenderPass (u64) the menu pipeline variant builds against
    void*
    MenuCommandBuffer() const; // VkCommandBuffer of the in-progress menu pass
    void*
    MenuRttTex(); // opaque handle {VkImage,VkDeviceMemory,VkImageView,0} or nullptr
    // #107 VR-Vulkan FPS quad: owned scFormat RTT for the head-locked FPS counter (2D text only -> display-RTT path).
    void EnsureFpsRtt(
        int w,
        int h); // (re)create the owned FPS RTT color image at w x h (scFormat)
    void EnsureSubRtt(int w,
                      int h); // #59: same, for the subtitle/chat quad panel
    void BindSubRtt(bool clear);
    void* SubRttTex();
    void BindFpsRtt(
        bool
            clear); // BindSceneRtt targeting the FPS image; UnbindSceneRtt(FpsRttTex) finalizes
    void*
    FpsRttTex(); // opaque handle {VkImage,VkDeviceMemory,VkImageView,0} or nullptr
    // #107 VR-Vulkan: blit an owned render-target's color image (srcTexHandle, layout SHADER_READ after UnbindSceneRtt)
    // into an XR UI-swapchain VkImage. Peer of the D3D11 CopyResource + the scene BlitSceneToXrImages, but from an
    // arbitrary RTT texture. Leaves dst in COLOR_ATTACHMENT_OPTIMAL, own fence, synchronous.
    bool BlitTexToXrImage(void* dstImage, void* srcTexHandle, int w, int h);
    unsigned long long GetSceneRenderPass()
        const; // VkRenderPass (as u64) for VulkanRenderer pipelines
    unsigned long long RttRenderPass()
        const; // VkRenderPass of the RTT pass (swapchain-compatible)
    int SceneViews() const;
    // #104: IRenderBackend overrides -- the engine asks the ACTIVE backend for its scene size (see the note there).
    int SceneW() const override;
    int SceneH() const override;

    // ---- RTT (render-target textures for HUD/MFD/DED displays) -- Vulkan peer of D3D12Backend BindSceneRtt ------
    // A display renders its symbology into a texture (VulkanTexture* from CreateRenderTarget), then the panel quad
    // samples it. In Vulkan you cannot swap a framebuffer mid-render-pass, so BindSceneRtt ends the open swapchain
    // pass on the flat command buffer and begins an RTT pass targeting the texture; UnbindSceneRtt ends the RTT pass
    // (finalLayout -> SHADER_READ) and reopens the swapchain pass with loadOp=LOAD so the backbuffer is preserved.
    void BindSceneRtt(void* vulkanTexHandle, int w, int h, bool clear);
    void UnbindSceneRtt(void* vulkanTexHandle);
    // Artscout - 2026 (GM radar): GPU-copy (blit) one RTT texture into another -- Vulkan peer of D3D12 CopyRtt.
    // Both handles are VulkanTexture*; src must be SHADER_READ (after UnbindSceneRtt), dst ends SHADER_READ.
    bool CopyRtt(void* srcTexHandle, void* dstTexHandle, int srcW, int srcH,
                 int dstW, int dstH);
    void
    WaitRttConsumed(); // #107 PERF: block until pending RTT submits finish (call before a reader's submit)
    void
    BindBackBufferRTV(); // reopen the backbuffer (ends an active RTT pass, else ensures a frame)
    void EnsureFrameStarted(unsigned long argbClear = 0xFF000000);
    bool IsRecording() const; // flat frame in progress
    // Artscout - 2026 (#104): an RTT pass is open, so display draws belong to it -- not to the scene, even though the
    // scene is usually recording at the same time (the displays render mid-cockpit). FlatCommandBuffer() already
    // returns rttCmd while this is true; the renderer needs the fact itself to pick the render pass to match.
    bool IsRttActive() const;
    // Sensor zone viewport (ConfineObjectViewportToZone) remembered across mid-batch re-binds; false when no zone
    // is pending for the currently bound RTT. GetRttExtent = the bound RTT's full size (the 2D path's viewport).
    bool GetRttZone(int* x, int* y, int* w, int* h) const;
    void GetRttExtent(int* w, int* h) const;
    unsigned RttViewportSerial()
        const; // changes whenever the recorded RTT viewport state may have changed (cache key for the renderer)

    // ---- flat 3D frame (desktop, non-VR) ------------------------------------------------------------------------
    // The object/terrain/sky passes render into the multiview scene target (1 view for desktop). The renderer opens
    // it lazily on the first such pass via EnsureSceneStarted(); PresentScene() ends it and blits scene layer 0 to
    // the swapchain. (VR submits the array to OpenXR instead -- separate path.)
    void EnsureSceneStarted(
        unsigned long argbSkyClear =
            0xFF203040); // EnsureSceneTarget(w,h,1) + BeginSceneMultiview
    bool SceneStarted() const; // a scene pass is currently recording
    void PresentScene(); // end the scene + blit layer 0 -> swapchain + present
    void CompositeBitmap565(
        const void* pSrc565, int srcW,
        int srcH); // UI 565 over the 3D frame (black=transparent)
    unsigned SwapchainColorFormatVk()
        const; // (VkFormat) the swapchain color format -- RTT textures match it

    // Artscout - 2026 (#107 VR-Vulkan): inject the instance/device extensions the OpenXR runtime requires (space- or
    // NUL-separated names from xrGetVulkan{Instance,Device}ExtensionsKHR) so the VkInstance/VkDevice built in Init can
    // share swapchain images with the VR compositor. MUST be called AFTER construction but BEFORE Init. Keeps this
    // backend OpenXR-agnostic (the caller -- devmgr on Windows -- does the query); no-op with empty strings.
    void SetExtraVulkanExtensions(const char* instExts, const char* devExts);

    // #109 XR_KHR_vulkan_enable2: in VR mode the OpenXR runtime WRAPS vkCreateInstance/vkCreateDevice (injecting its
    // own interop extensions) and CHOOSES the physical device (xrGetVulkanGraphicsDevice2KHR) -- so Init must route
    // those three steps through the runtime instead of calling vkCreate* directly. VulkanBackend stays OpenXR-agnostic
    // (ffvulkan builds on Linux) by calling these hooks, which OpenXRBackend installs before Init. Vulkan handles pass
    // as void* to keep this header free of <vulkan/vulkan.h> (see the handle note below); both sides cast. Each returns
    // a VkResult as int (0 == VK_SUCCESS). Unset (flat / non-VR / Linux) -> the plain vkCreate* path runs. Arg shapes:
    //   create-instance: (const VkInstanceCreateInfo*, VkInstance* out, user)
    //   pick-physical:   (VkInstance,                  VkPhysicalDevice* out, user)
    //   create-device:   (VkPhysicalDevice, const VkDeviceCreateInfo*, VkDevice* out, user)
    typedef int (*VkbXrCreateInstanceHook)(const void* ci, void* outInstance,
                                           void* user);
    typedef int (*VkbXrPickPhysicalHook)(void* instance, void* outPhysical,
                                         void* user);
    typedef int (*VkbXrCreateDeviceHook)(void* physical, const void* ci,
                                         void* outDevice, void* user);
    void SetVulkanCreationHooks(VkbXrCreateInstanceHook ci,
                                VkbXrPickPhysicalHook pp,
                                VkbXrCreateDeviceHook cd, void* user);

    // ---- device sharing (for VulkanRenderer) ------------------------------------------------------------------
    // Handles exposed as void*/u64 so this header stays free of <vulkan/vulkan.h>; VulkanRenderer casts them back.
    void* VkInstanceHandle()
        const; // VkInstance (#107 VR: OpenXR graphics binding needs it too)
    void* VkDeviceHandle() const; // VkDevice
    void* VkPhysicalDeviceHandle() const; // VkPhysicalDevice
    void* VkQueueHandle() const; // VkQueue (graphics+present)
    void* VkCommandPoolHandle() const; // VkCommandPool
    unsigned VkGraphicsFamily() const;
    unsigned VkXrQueueIndex()
        const; // #107 VR: dedicated queue index for the OpenXR graphics binding (1 if available)
    // Artscout - 2026 (#104): the in-flight frame slot, advanced once per BeginFrame. This is the ONLY real frame
    // boundary; VulkanRenderer keys its per-frame rings off it instead of counting passes (see SyncFrame).
    unsigned FrameSlot() const;
    unsigned long long SwapchainRenderPass()
        const; // VkRenderPass of the flat swapchain pass (u64)
    // Artscout - 2026 (#104): true when the depth buffer carries a stencil plane (D32_SFLOAT_S8_UINT, the peer of
    // D3D12's D32_FLOAT_S8X24). The #76 HUD aperture stencil needs it; false disables those paths instead of failing.
    bool DepthHasStencil() const;
    void* FlatCommandBuffer()
        const; // VkCommandBuffer of the in-progress flat frame (null if not recording)
    void*
    SceneCommandBuffer() const; // VkCommandBuffer of the multiview scene pass
    bool IsFlatRecording() const;
    bool IsSceneRecording() const;
    unsigned
    FindMemoryTypeIndex(unsigned typeBits,
                        unsigned propFlags) const; // VkMemoryPropertyFlags

    // Pimpl: forward-declared here (public so the .cpp's file-local swapchain helpers can name it); defined in
    // VulkanBackend.cpp. Callers never touch it.
    struct Impl;

private:
    Impl* m; // all Vulkan + SDL state
};

// Runtime backend selection. On Windows the graphics-options driver selector flips g_bUseVulkan (DX12 vs Vulkan);
// on Linux it is forced true at startup (Vulkan is the only backend). g_pVulkanBackend is the active instance,
// mirrored into the neutral g_pRenderBackend by DXContext::Init.
extern bool g_bUseVulkan;
extern bool
    g_bVulkanScene; // #104: gate the flat 3D scene path (OFF by default; debugged headless)
extern bool
    g_bVulkanTerrain; // #104: gate the GPU-terrain path under Vulkan (OFF while its freeze is fixed)
// #104: the KHRONOS validation layer, plus synchronization validation on top of it, routed into the Vulkan log. Both
// default ON while the freeze is chased; flip them off in VulkanBackend.cpp for a full-speed run. Read once at
// vkCreateInstance -- changing them afterwards has no effect.
extern bool g_bVulkanValidation;
extern bool g_bVulkanSyncValidation;
extern VulkanBackend* g_pVulkanBackend;

#endif // FF_VULKAN_BACKEND_H
