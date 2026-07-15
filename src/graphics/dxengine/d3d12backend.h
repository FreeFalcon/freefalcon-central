//-----------------------------------------------------------------------------
// D3D12Backend.h
//
// Artscout - 2026: #DX12 Phase 1 of the D3D11 -> D3D12 render port (VR single-pass
// target). Self-contained Direct3D 12 device + DXGI flip swap chain that lives
// ALONGSIDE the D3D11 path (which itself lives alongside the legacy DDraw/D3D7).
// Selected at runtime by g_bUseD3D12. When the flag is off this object is never
// created and the D3D11/D3D7 paths are completely untouched.
//
// Phase 1 milestone (mirrors the original D3D11 Phase 1): the device comes up,
// clears a colored frame, and Presents through a flip-model swap chain with a
// per-frame fence. No scene geometry yet -- the FFEmu shader pipeline, the
// object/terrain/RTT passes, the OpenXR-on-D3D12 binding, and finally view-
// instanced single-pass stereo are later phases. See the DX12 plan in memory.
//
// VR note: like D3D11Backend, this owns only the device + the window swap chain.
// Per-eye/per-view targets and the multiview (SV_ViewID) matrix array belong to
// the render pass (Phase 5), not here -- keeping device creation independent of
// any single projection is the VR-relevant invariant held from the start.
//-----------------------------------------------------------------------------
#ifndef _D3D12BACKEND_H_
#define _D3D12BACKEND_H_

#include <windows.h>
#include "IRenderBackend.h"   // Artscout - 2026: #DX12 -- API-neutral backend interface

// Forward declarations keep d3d12.h out of the widely-included headers (same
// discipline as d3d11backend.h with d3d11.h).
struct ID3D12Device;
struct ID3D12CommandQueue;
struct ID3D12DescriptorHeap;
struct ID3D12Resource;
struct ID3D12CommandAllocator;
struct ID3D12GraphicsCommandList;
struct ID3D12Fence;
struct ID3D12RootSignature;   // Phase 2 (2D UI quad)
struct ID3D12PipelineState;
struct IDXGISwapChain3;
struct D3D12Texture;          // #DX12 п.5 A1: in-scene VR menu RTT (from D3D12TextureManager)

class D3D12Backend : public IRenderBackend   // Artscout - 2026: #DX12 -- implements the neutral backend surface
{
public:
	D3D12Backend();
	~D3D12Backend();

	// Creates the device + flip swap chain bound to hWnd. Returns false (and logs)
	// on any failure so the caller can fall back to D3D11/D3D7.
	bool Init(HWND hWnd, int nWidth, int nHeight, int nDepth, bool bFullscreen);

	// Releases everything (waits for the GPU to idle first). Safe to call twice.
	void Release();

	// Recreate the swap-chain back buffers at a new size (waits for GPU idle).
	bool Resize(int nWidth, int nHeight);

	bool IsValid() const { return m_pDevice != 0; }

	// --- Frame loop -----------------------------------------------------------
	// Begin recording this frame: wait for the target back buffer's fence, reset
	// the allocator + command list, transition the back buffer to RENDER_TARGET,
	// bind it, and clear to argb (0xAARRGGBB).
	void BeginFrame(unsigned long argbClearColor = 0xFF203040);

	// Transition the back buffer to PRESENT, close+execute the command list,
	// Present through the flip swap chain, and signal the fence for this frame.
	void Present(bool bVSync = true);

	// --- Neutral backend surface (Phase 1: stubs where the ported pass isn't up yet) ---
	void BindBackBuffer(bool bClearDepth);   // Phase 1: same as BeginFrame(clear) -- one RT, no depth yet
	void ClearDepth();                        // Phase 1: no depth buffer yet -> no-op
	void SetViewportRect(int x, int y, int w, int h);
	void ResolveMsaaToBackBuffer();           // Phase 1: no MSAA yet -> no-op
	void BlitBitmap565(const void* pSrc565, int srcW, int srcH);   // opaque fullscreen blit (menu frame)
	// #DX12 п.2: composite the RGB565 UI surface OVER the already-rendered 3D scene. Black (0x0000) -> alpha 0
	// (transparent, 3D shows through); everything else opaque. Alpha-blended fullscreen quad. Used on a GPU frame.
	void CompositeBitmap565(const void* pSrc565, int srcW, int srcH);
	void SetGScreenSize(int w, int h);        // Phase 2 (cbViewport) -> stub for now
	void ClearCurrentRTV(float r, float g, float b, float a);
	void FlushContext();                      // D3D12 uses explicit submit -> no-op

	int  Width()  const { return m_nWidth; }
	int  Height() const { return m_nHeight; }
	HWND Hwnd()   const { return m_hWnd; }

	// For later phases (renderer / OpenXR-D3D12 binding).
	ID3D12Device*       GetDevice() const { return m_pDevice; }
	ID3D12CommandQueue* GetQueue()  const { return m_pQueue; }

	// Artscout - 2026: #DX12 Phase 3 -- the renderer records scene geometry into the SAME command list
	// the backend opens between BeginFrame() and Present(). Expose it + the recording flag so
	// D3D12Renderer can bracket its draws, and a helper to lazily open the frame on the first draw
	// (mirrors how the D3D11 path binds the back buffer at frame start).
	ID3D12GraphicsCommandList* GetCommandList() const { return m_pList; }
	bool  IsRecording() const { return m_bRecording; }
	unsigned FrameIndex() const { return m_frameIndex; }   // which of the kFrameCount ring slots is live
	void  EnsureFrameStarted(unsigned long argbClear = 0xFF000000) { if (!m_bRecording) BeginFrame(argbClear); }
	// The scene depth-stencil (D32) CPU handle, for the renderer's OMSetRenderTargets. ptr==0 if none.
	unsigned __int64 DepthDsvPtr() const;

	// #DX12 п.3 RTT: bind an external render-target texture (D3D12Texture*, from the texture manager) as the
	// current target -- displays draw their symbology into it. Transitions it to RENDER_TARGET. UnbindSceneRtt
	// transitions it back to PIXEL_SHADER_RESOURCE (sampled by DrawRttQuad) and rebinds the back buffer.
	void BindSceneRtt(void* d3d12TexHandle, int w, int h, bool clear);
	void UnbindSceneRtt(void* d3d12TexHandle);
	void BindBackBufferRTV();   // rebind the swap-chain back buffer RTV + depth + full viewport (no clear)

	// #DX12 A5: off-screen RTT readback/copy for the sensor displays (TGP/FLIR + Munitions 3D-viewer) and the
	// GM radar snapshot. rttHandle/srcHandle/dstHandle are D3D12Texture* (render-target textures from the
	// texture manager). ReadbackRttTo565 is DEFERRED + 1-frame latent: it converts the PREVIOUS frame's copy
	// into the 565 dst buffer, then records THIS frame's copy on the main list -- no mid-frame GPU stall.
	// CopyRtt is a GPU copy (src RTT -> dst texture) recorded on the main list (GM sweep -> persistent panel).
	// ReleaseReadbackFor drops the readback buffer bound to a destroyed RTT.
	void ReadbackRttTo565(void* rttHandle, unsigned short* dst, int dstStridePix, int dstHeightPix,
	                      int x, int y, int w, int h);
	void CopyRtt(void* srcHandle, void* dstHandle);
	void ReleaseReadbackFor(void* rttHandle);

	// #DX12 п.5 (VR per-eye): render the scene INTO an XR eye swapchain image. BeginEyeFrame opens a command
	// list bound to the eye RTV + a VR depth buffer (clears both), the engine draws via g_pRenderer, EndEyeFrame
	// transitions the image to COMMON, executes the list, and fences (so the image is filled before the runtime
	// releases it). eyeImg = ID3D12Resource* of the eye image (from OpenXRBackend), eyeRtvPtr = its RTV handle.
	void BeginEyeFrame(void* eyeImg, unsigned __int64 eyeRtvPtr, int w, int h);
	void EndEyeFrame(void* eyeImg);

	// Artscout - 2026: #DX12 п.5 view-instanced single-pass stereo. ViewInstancingSupported() queries
	// D3D12_FEATURE_D3D12_OPTIONS3.ViewInstancingTier once (cached). BeginStereoInstancedFrame opens ONE command
	// list bound to the 2-slice ARRAY eye RTV + a 2-slice array depth, clears both slices, and sets the view
	// instance mask 0b11 so both views rasterize. The world scene is then drawn ONCE (VI PSOs). BindEyeSlice
	// re-binds a SINGLE array slice (RTV + that slice's depth) with view instance mask 1 for the per-eye 2D
	// overlay tail. EndStereoInstancedFrame closes+executes+fences (like EndEyeFrame). arrayRtvPtr / sliceRtvPtr
	// come from OpenXRBackend (it owns the array swapchain image + its RTVs).
	bool ViewInstancingSupported();
	void BeginStereoInstancedFrame(void* arrayImg, unsigned __int64 arrayRtvPtr, int w, int h, int nViews);
	void BindEyeSlice(int view, unsigned __int64 sliceRtvPtr, int w, int h);
	void EndStereoInstancedFrame(void* arrayImg);
	// #DX12 п.5 QUAD copy path (foveated layer rejects array swapchains): VI-render a group into a PRIVATE 2-slice
	// array target, then copy its slices into the 2 per-view swapchain images. See m_viColor below.
	void BeginViCopyGroup(int w, int h, int fmt, void** sliceRtvsOut /*[4]*/);
	void EndViCopyGroup(void* dstImg0, void* dstImg1);
	// A monotonic render-frame counter: bumped by BeginFrame AND BeginEyeFrame. The renderer resets its per-frame
	// rings when this changes (each VR eye is a separate render epoch even though the swap-chain index doesn't move).
	unsigned RenderEpoch() const { return m_renderEpoch; }
	// #DX12 п.5: the current SCENE target size -- the eye in a VR per-eye pass (set by BeginEyeFrame), else the
	// back buffer (BeginFrame). ContextMPR::StartFrame sets gScreenSize to THIS so the CPU-projected 2D sky/
	// terrain (VR_SetRes -> eye-sized pixels) map to NDC correctly instead of by the back-buffer size.
	int SceneW() const { return m_sceneW > 0 ? m_sceneW : m_nWidth; }
	int SceneH() const { return m_sceneH > 0 ? m_sceneH : m_nHeight; }

	// #DX12 п.5 A1 (in-scene VR menu): off-screen RGBA8 color RTT (+ its own depth for the exit-dialog BSP) into
	// which the comms/exit menu is drawn on the eye command list, then copied into the XR UI swapchain image and
	// submitted as a head-locked quad by OpenXRBackend::SubmitInSceneMenuQuad. Mirrors D3D11Backend's menu RTT.
	void  EnsureMenuRtt(int w, int h);   // (re)create at this size, best-effort (cached; recreated on size change)
	void  BindMenuRtt(bool clear);       // bind the menu color RTV + its depth, set viewport, optionally clear
	void* MenuRttTex();                  // D3D12Texture* (copy source for the XR UI swapchain); NULL if not up
	// Artscout - 2026: #DX12 п.5 -- a SMALL colour-only RTT for the head-locked VR FPS quad (2D text; no depth). Same
	// pattern as the menu RTT but separate so FPS + a menu don't thrash one RTT's size. See OpenXRBackend::SubmitFpsQuad.
	void  EnsureFpsRtt(int w, int h);
	void  BindFpsRtt(bool clear);
	void* FpsRttTex();
	// Fixed formats the renderer bakes into its PSOs (must match the swap chain / depth buffer).
	static int BackBufferFormat();   // DXGI_FORMAT_R8G8B8A8_UNORM
	static int DepthFormat();        // DXGI_FORMAT_D32_FLOAT_S8X24_UINT (reversed-Z float depth + stencil)

	// Artscout - 2026: MSAA (D3D12). The scene renders into an MSAA color+depth target, then resolves into
	// the single-sample backbuffer (flat) / eye image (VR). Sample count from g_bMsaaEnable/g_nMsaaSamples,
	// snapped to a supported level. CurrentSampleCount() = the sample count of the CURRENTLY-bound target
	// (MSAA scene vs single-sample RTT/menu), so the renderer builds PSOs whose SampleDesc matches the RT.
	int  CurrentSampleCount() const { return m_curSampleCount > 0 ? m_curSampleCount : 1; }
	bool MsaaActive() const { return m_msaaSamples > 1 && m_pMsaaColorTex != 0; }

	// Artscout - 2026: #13 volumetric clouds -- the scene depth, readable as t2 so the cloud raymarch can clamp
	// itself against the world instead of leaning on the rasterizer's depth test (which cannot work for a volume
	// you fly through). Returns a CPU descriptor for a Texture2DArray<float> view of the CURRENTLY-bound scene
	// depth, or 0 when there is none / it cannot be viewed that way.
	//   ONE view type suffices because both VR paths are single-sample (BeginEyeFrame / BeginStereoInstancedFrame
	// both set m_curSampleCount = 1 -- "MSAA-in-VR = a later increment"), so VR depth is a plain 2-or-4 slice
	// array, and a non-array Texture2D is viewable as an array of 1. Only the FLAT MSAA path has a multisampled
	// depth, which a Texture2DArray cannot view -> 0 there (see SceneDepthSrvCpu).
	unsigned __int64 SceneDepthSrvCpu();
	// Transition the scene depth between DEPTH_WRITE and DEPTH_READ|PIXEL_SHADER_RESOURCE around the cloud draw.
	// Legal only because the cloud pass does not write depth.
	void SetSceneDepthReadable(bool readable);

private:
	struct ID3D12Resource*     m_pSceneDepthRes;   // #13: the depth resource the CURRENT scene pass bound (not owned)
	int                        m_sceneDepthSlices; // #13: its array slice count (1 flat, 2 stereo, 2/4 quad groups)
	bool                       m_sceneDepthMs;     // #13: true = multisampled (flat MSAA) -> no array SRV
	bool                       m_sceneDepthReadable;
	struct ID3D12DescriptorHeap* m_pDepthSrvHeap;  // #13: tiny CPU heap holding the depth SRV
	struct ID3D12Resource*     m_depthSrvFor;      // #13: which resource m_pDepthSrvHeap's descriptor describes

	bool CreateBackBufferViews();
	void ReleaseBackBufferViews();
	bool CreateDepthBuffer();          // Artscout - 2026: #DX12 Phase 3 -- D32 depth-stencil for the scene
	void ReleaseDepthBuffer();
	void WaitForGpu();          // block until the GPU has finished ALL submitted work
	void MoveToNextFrame();     // signal this frame's allocator fence, then advance to the next back buffer

	// Artscout - 2026: #DX12 -- command-allocator lifetime. An allocator may only be Reset once the GPU has
	// retired EVERY command list recorded from it, so each allocator now carries the fence value signalled right
	// after its work was submitted, and BeginCommandList() waits that value out before recycling it.
	// The previous scheme instead leaned on an implicit "MoveToNextFrame already waited for this index"
	// invariant: correct for the flat BeginFrame->Present loop, but the VR entry points
	// (BeginStereoInstancedFrame / BeginEyeFrame) reset the allocator themselves and key off the SWAPCHAIN
	// index -- which their frames never present -- so the invariant did not hold there and the debug layer
	// flagged it (ERROR #552: allocator reset while its executions are still in flight -> CPU stomps command
	// memory the GPU is reading -> non-deterministic garbage geometry). Tying the wait to the allocator itself
	// makes every path (flat / stereo / eye / quad-copy / mid-frame WaitForGpu / early return) correct by
	// construction rather than by convention.
	unsigned __int64 SignalQueue();                    // post the next strictly-increasing value on m_pQueue
	void             WaitForFence(unsigned __int64 v); // block until the fence reaches v
	void             BeginCommandList();               // wait out m_pAlloc[m_frameIndex], then reset alloc + list
	unsigned __int64 NextSignalValue() const { return m_fenceCounter + 1; }   // value the NEXT SignalQueue() posts

	// Phase 2 (2D UI): lazily build the fullscreen-quad pipeline + the (resizable) UI texture.
	bool EnsureQuadPipeline();
	bool EnsureQuadTexture(int w, int h);

	enum { kFrameCount = 2 };

	HWND                      m_hWnd;
	int                       m_nWidth;
	int                       m_nHeight;
	bool                      m_bFullscreen;
	bool                      m_bRecording;

	ID3D12Device*             m_pDevice;
	ID3D12CommandQueue*       m_pQueue;
	IDXGISwapChain3*          m_pSwapChain;
	ID3D12DescriptorHeap*     m_pRtvHeap;
	unsigned                  m_rtvDescSize;
	ID3D12DescriptorHeap*     m_pDsvHeap;      // Artscout - 2026: #DX12 Phase 3 -- 1 DSV
	ID3D12Resource*           m_pDepthTex;     // Artscout - 2026: #DX12 Phase 3 -- D32 depth-stencil
	unsigned                  m_renderEpoch;   // #DX12 п.5 -- bumped per BeginFrame / BeginEyeFrame (ring reset key)
	ID3D12Resource*           m_pEyeDepthTex;  // #DX12 п.5 -- VR per-eye depth (sized to the eye)
	ID3D12DescriptorHeap*     m_pEyeDsvHeap;
	int                       m_eyeDepthW, m_eyeDepthH;
	bool EnsureEyeDepth(int w, int h);
	// Artscout - 2026: #DX12 п.5 view-instancing. 2-slice ARRAY depth per VI GROUP (a view pair). Each slot's DSV
	// heap holds 3 views: [0] = whole 2-slice array (VI geometry pass), [1] = slice 0, [2] = slice 1 (per-eye 2D
	// overlay tail). TWO slots so quad's periphery + focus resolutions coexist WITHOUT a huge per-frame depth
	// realloc (stereo uses one). EnsureEyeDepthArray picks/builds the slot matching (w,h) and sets m_eyeDepthCur;
	// Begin/BindEyeSlice use that slot. m_viTier caches D3D12_FEATURE_D3D12_OPTIONS3 (-1 = not queried, 0 = none,
	// >=1 = tier). m_pList1 = SetViewInstanceMask (ID3D12GraphicsCommandList1).
	struct EyeDepthSlot { ID3D12Resource* tex; ID3D12DescriptorHeap* dsvHeap; int w, h, n; unsigned lru; };
	EyeDepthSlot              m_eyeDepth[2];    // one per VI group (resolution)
	int                       m_eyeDepthCur;    // slot bound by the last EnsureEyeDepthArray
	int                       m_viTier;         // -1 unknown, 0 none, >=1 supported
	struct ID3D12GraphicsCommandList1* m_pList1;
	bool EnsureEyeDepthArray(int w, int h, int nViews);
	// Artscout - 2026: #DX12 п.5 QUAD copy path. The PVR quad_views_foveated layer rejects ANY array swapchain
	// (arraySize 2 or 4 -> xrEndFrame HANDLE_INVALID); it wants 4 separate arraySize=1 per-view swapchains. So for
	// quad we VI-render each group into a PRIVATE 2-slice array color target (typeless RGBA, matched to the
	// swapchain family), then CopyTextureRegion each slice into the matching per-view swapchain image. Two slots so
	// periphery + focus resolutions coexist (LRU, like m_eyeDepth). BeginViCopyGroup reuses BeginStereoInstancedFrame
	// on the private array; EndViCopyGroup copies the 2 slices to the 2 swapchain images then closes+fences.
	struct ViColorSlot { ID3D12Resource* tex; ID3D12DescriptorHeap* rtvHeap; unsigned __int64 arrayRtv; unsigned __int64 sliceRtv[2]; int w, h; int fmt; unsigned lru; };
	ViColorSlot               m_viColor[2];
	int                       m_viColorCur;
	bool EnsureViColorArray(int w, int h, int fmt);
	// Artscout - 2026: MSAA scene targets (bound instead of the backbuffer/eye when active; resolved down in
	// ResolveMsaaToBackBuffer (flat) / EndEyeFrame (VR)).
	ID3D12Resource*       m_pMsaaColorTex;
	ID3D12DescriptorHeap* m_pMsaaRtvHeap;
	ID3D12Resource*       m_pMsaaDepthTex;
	ID3D12DescriptorHeap* m_pMsaaDsvHeap;
	int   m_msaaSamples;     // 1 = off; else the snapped sample count
	int   m_msaaW, m_msaaH;
	int   m_curSampleCount;  // sample count of the currently-bound target (PSO matching)
	void* m_pEyeResolveImg;  // XR eye image to resolve MSAA into (saved in BeginEyeFrame)
	int   ResolveMsaaSamples();            // read cfg + snap to a supported level (1 = off)
	bool  CreateMsaaTargets(int w, int h); // (re)create the MSAA color+depth at this size
	void  ReleaseMsaaTargets();
	void  BindMsaaScene(unsigned long argb, int w, int h);   // bind+clear the MSAA scene target (from Begin*Frame)
	D3D12Texture*             m_pMenuRtt;       // #DX12 п.5 A1 -- in-scene VR menu color RTT (owned)
	ID3D12Resource*           m_pMenuDepthTex;  // its depth (exit dialog is a 3D BSP -> needs Z)
	ID3D12DescriptorHeap*     m_pMenuDsvHeap;
	int                       m_menuRttW, m_menuRttH;
	D3D12Texture*             m_pFpsRtt;        // #DX12 п.5 -- small VR FPS quad color RTT (owned; no depth)
	int                       m_fpsRttW, m_fpsRttH;
	ID3D12Resource*           m_pBackBuffer[kFrameCount];
	ID3D12CommandAllocator*   m_pAlloc[kFrameCount];
	ID3D12GraphicsCommandList* m_pList;

	unsigned __int64          m_curRtvPtr;   // #DX12: currently bound RTV (backbuffer OR the RTT atlas) -- ClearCurrentRTV clears THIS
	// #DX12 п.5: the current SCENE target (back buffer on desktop, the eye image in VR). RTT StartRtt/FinishRtt
	// switch away to the display atlas and back -- FinishRtt must rebind THIS, not always the back buffer.
	unsigned __int64          m_sceneRtvPtr;
	unsigned __int64          m_sceneDsvPtr;
	int                       m_sceneW, m_sceneH;
	ID3D12Fence*              m_pFence;
	unsigned __int64          m_allocFence[kFrameCount];   // value signalled after m_pAlloc[i]'s work was submitted
	unsigned __int64          m_fenceCounter;              // last value posted on m_pQueue (strictly increasing)
	HANDLE                    m_fenceEvent;
	unsigned                  m_frameIndex;

	// Phase 2 (2D UI): fullscreen textured quad presenting the RGB565 UI surface.
	ID3D12RootSignature*      m_pQuadRS;
	ID3D12PipelineState*      m_pQuadPSO;
	ID3D12PipelineState*      m_pQuadPSOBlend;   // #DX12 п.2: alpha-blend variant (UI-over-3D composite)
	ID3D12DescriptorHeap*     m_pSrvHeap;      // shader-visible CBV/SRV heap (1 SRV = the UI texture)
	ID3D12Resource*           m_pQuadTex;      // DEFAULT-heap RGBA8 texture (sampled by the quad)
	ID3D12Resource*           m_pQuadUpload;   // UPLOAD-heap staging buffer (565->RGBA8 each frame)
	int                       m_quadTexW, m_quadTexH;
	unsigned                  m_quadRowPitch;  // 256-aligned upload row pitch
	unsigned                  m_quadTexState;  // current D3D12_RESOURCE_STATES of m_pQuadTex
};

extern D3D12Backend* g_pD3D12Backend;   // NULL unless g_bUseD3D12 brought it up
extern bool          g_bUseD3D12;       // runtime selector, parallel to g_bUseD3D11

#endif // _D3D12BACKEND_H_
