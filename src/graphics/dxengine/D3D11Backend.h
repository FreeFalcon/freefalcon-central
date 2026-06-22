//-----------------------------------------------------------------------------
// D3D11Backend.h
//
// PHASE 1 of the D3D7 -> D3D11 render port (see project memory).
//
// Self-contained Direct3D 11 device + DXGI swap chain that lives ALONGSIDE the
// legacy DirectDraw7 / Direct3D7 path. Selected at runtime by g_bUseD3D11.
// When the flag is off this object is never created and the legacy path is
// completely untouched.
//
// Phase 1 milestone: come up, Clear() a colored frame, Present(). No scene
// geometry yet -- the fixed-function pipeline is ported in Phase 2.
//
// VR / OpenXR note (see openxr-feasibility memory): this object owns only the
// device + the window swap chain. Per-eye view/projection and per-eye render
// targets are deliberately NOT here -- they belong to the render pass (Phase 2)
// and to the OpenXR swap-chain images (Phase 6+). Keeping the device creation
// separate from any single projection is the one VR-relevant invariant we hold
// from the start.
//-----------------------------------------------------------------------------
#ifndef _D3D11BACKEND_H_
#define _D3D11BACKEND_H_

#include <windows.h>

// Forward declarations keep d3d11.h out of the widely-included headers, the
// same way context.h forward-declares the D3D7 interfaces.
struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGISwapChain;
struct ID3D11RenderTargetView;
struct ID3D11Texture2D;
struct ID3D11DepthStencilView;

class D3D11Backend
{
public:
	D3D11Backend();
	~D3D11Backend();

	// Creates the device + swap chain bound to hWnd. Returns false (and logs
	// via MonoPrint) on any failure so the caller can fall back to D3D7.
	// bFullscreen currently creates a windowed swap chain sized to nWidth x
	// nHeight; exclusive fullscreen via DXGI is deferred (Phase 5 / display
	// management). nDepth selects the depth-buffer format.
	bool Init(HWND hWnd, int nWidth, int nHeight, int nDepth, bool bFullscreen);

	// Releases everything. Safe to call multiple times.
	void Release();

	bool IsValid() const { return m_pDevice != NULL; }

	// --- Frame loop (Phase 1 uses only these) --------------------------------

	// Binds the back-buffer RTV + depth DSV and clears them. Color is 0xAARRGGBB.
	void BeginFrame(unsigned long argbClearColor = 0xFF000000);

	// PHASE 4: binds the back-buffer RTV + depth DSV + viewport WITHOUT clearing
	// (so the ContextMPR 3D pass has a render target without erasing 2D). bClearDepth --
	// clear only Z (for the 3D scene).
	void BindBackBuffer(bool bClearDepth = false);

	// Artscout - 2026: TEMP RTT diagnostic -- print the currently-bound OM render target and which
	// known target it is (MSAA-3D/back buffer/off-screen RTT). Used to locate where the TGP/GM-radar
	// sensor objects actually land. Remove once the RTT sensor path is fixed.

	// PHASE 5: clear ONLY depth/stencil (leave color) -- so the 3D cockpit
	// (object path) draws OVER the already-rendered world/sky (screen path).
	void ClearDepth();

	// PHASE 5: set the viewport rectangle (for per-display scissor/viewport of MFD/HUD).
	void SetViewportRect(int x, int y, int w, int h);

	// PHASE 5 (RTT): bind an arbitrary RTV (MFD/HUD panel textures) as render target
	// + viewport over the whole texture, optionally clear. rtv = ID3D11RenderTargetView* (void* to
	// avoid pulling d3d11.h into the calling code).
	// unbindSRV: drop PS SRV inputs before binding the RTV (RTT hazard: renderTexture
	// as both input AND output). true only in StartRtt; see .cpp.
	void BindRenderTargetView(void* rtv, int w, int h, bool clear, bool unbindSRV = false);

	// Presents the swap chain. bVSync maps to the DXGI sync interval.
	void Present(bool bVSync = true);

	// PHASE 2 (2D UI): copies a 16-bit RGB565 CPU bitmap (UI95 composite) into the back buffer
	// (565->RGBA8 conversion + CopyResource). Called before Present for the screen buffer.
	void BlitBitmap565(const void* pSrc565, int srcW, int srcH);

	// Recreates the back-buffer / depth views for a new client size.
	bool Resize(int nWidth, int nHeight);

	// --- Accessors for later phases ------------------------------------------
	// Phase 2 (FF emulation) binds shaders/PSO-equivalents and draws through
	// these; Phase 3/4 create textures and vertex/index buffers from the device.
	ID3D11Device*        GetDevice()  const { return m_pDevice; }
	ID3D11DeviceContext* GetContext() const { return m_pContext; }

	int  Width()  const { return m_nWidth; }
	int  Height() const { return m_nHeight; }

	// Clear the currently bound RTV (for the RTT atlas: cleared once at the start of the batch
	// via ClearDraw->ClearBuffers, since StartRtt no longer clears).
	void ClearCurrentRTV(float r, float g, float b, float a);

	// #7 MSAA: resolve the 3D-scene MSAA target (resolve into the backbuffer at present). If MSAA
	// is active -- 3D renders into m_pMsaaRTV/m_pMsaaDSV, then ResolveMsaaToBackBuffer().
	bool MsaaActive() const { return m_pMsaaRTV != NULL && m_pMsaaDSV != NULL; }
	void ResolveMsaaToBackBuffer();

	// #7 AA-RTT: MSAA for the displays atlas (HUD/DED/MFD/RWR draw into the RTT atlas, which used to
	// be 1-sample -> jagged HUD lines/circle). Render into the MSAA atlas, before DrawRttQuad
	// resolve -> the normal renderTexture (its SRV is sampled by the panel). The 3D scene already has MSAA separately.
	void SetupRttMsaa(int w, int h);                 // create the MSAA atlas (sized to RTT), best-effort
	bool RttMsaaActive() const { return m_pRttMsaaRTV != NULL; }
	void BindRttMsaaRTV(int w, int h, bool clear, bool unbindSRV = false); // bind the MSAA atlas as RTV
	void ResolveRttMsaa(void* destTex);              // resolve the MSAA atlas -> destTex (renderTexture->m_pD3D11Tex)

private:
	bool CreateBackBufferViews();
	bool CreateDepthBuffer(int nWidth, int nHeight, int nDepth);
	bool CreateMsaaTargets(int nWidth, int nHeight); // #7 MSAA
	void ReleaseSizeDependent();

	ID3D11Device*           m_pDevice;
	ID3D11DeviceContext*    m_pContext;
	IDXGISwapChain*         m_pSwapChain;
	ID3D11RenderTargetView* m_pBackBufferRTV;
	ID3D11Texture2D*        m_pDepthTex;
	ID3D11DepthStencilView* m_pDepthDSV;
	ID3D11Texture2D*        m_pBlitStaging;	// staging texture for BlitBitmap565 (2D UI)
	int                     m_blitW, m_blitH;

	// #7 MSAA: the 3D-scene multisample target + its depth (resolve -> swapchain backbuffer).
	ID3D11Texture2D*        m_pMsaaColorTex;
	ID3D11RenderTargetView* m_pMsaaRTV;
	ID3D11Texture2D*        m_pMsaaDepthTex;
	ID3D11DepthStencilView* m_pMsaaDSV;
	unsigned int            m_msaaSamples;

	// #7 AA-RTT: the displays MSAA atlas (resolve -> renderTexture before DrawRttQuad).
	ID3D11Texture2D*        m_pRttMsaaTex;
	ID3D11RenderTargetView* m_pRttMsaaRTV;
	int                     m_rttMsaaW, m_rttMsaaH;

	HWND m_hWnd;
	int  m_nWidth;
	int  m_nHeight;
	int  m_nDepth;
	bool m_bFullscreen;
};

// Single global backend instance, created in DXContext::Init when g_bUseD3D11
// is set. NULL when the legacy D3D7 path is active.
extern D3D11Backend* g_pD3D11Backend;

// DIAG (RTT): true between StartRtt and FinishRtt -- so DrawTL logs the actually
// bound RTV during display rendering (renderTexture vs backbuffer).
extern bool g_rttBatchActive;

#endif // _D3D11BACKEND_H_
