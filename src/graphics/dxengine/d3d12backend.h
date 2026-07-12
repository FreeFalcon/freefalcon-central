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
	// Fixed formats the renderer bakes into its PSOs (must match the swap chain / depth buffer).
	static int BackBufferFormat();   // DXGI_FORMAT_R8G8B8A8_UNORM
	static int DepthFormat();        // DXGI_FORMAT_D32_FLOAT_S8X24_UINT (reversed-Z float depth + stencil)

	// Artscout - 2026: MSAA (D3D12). The scene renders into an MSAA color+depth target, then resolves into
	// the single-sample backbuffer (flat) / eye image (VR). Sample count from g_bMsaaEnable/g_nMsaaSamples,
	// snapped to a supported level. CurrentSampleCount() = the sample count of the CURRENTLY-bound target
	// (MSAA scene vs single-sample RTT/menu), so the renderer builds PSOs whose SampleDesc matches the RT.
	int  CurrentSampleCount() const { return m_curSampleCount > 0 ? m_curSampleCount : 1; }
	bool MsaaActive() const { return m_msaaSamples > 1 && m_pMsaaColorTex != 0; }

private:
	bool CreateBackBufferViews();
	void ReleaseBackBufferViews();
	bool CreateDepthBuffer();          // Artscout - 2026: #DX12 Phase 3 -- D32 depth-stencil for the scene
	void ReleaseDepthBuffer();
	void WaitForGpu();          // block until the GPU has finished ALL submitted work
	void MoveToNextFrame();     // signal current frame's fence, advance, wait if the next is in flight

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
	unsigned __int64          m_fenceValue[kFrameCount];
	unsigned __int64          m_fenceCounter;
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
