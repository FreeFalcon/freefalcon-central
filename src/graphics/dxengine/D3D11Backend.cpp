//-----------------------------------------------------------------------------
// D3D11Backend.cpp   -- see D3D11Backend.h for the Phase 1 contract.
//-----------------------------------------------------------------------------
#include "stdafx.h"
#include "D3D11Backend.h"

#include <d3d11.h>
#include <dxgi.h>

// MonoPrint is the project-wide debug logger (used throughout DDstuff).
extern "C" void MonoPrint(char *fmt, ...);

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

D3D11Backend* g_pD3D11Backend = NULL;
bool g_rttBatchActive = false;	// Artscout - 2026: DIAG (RTT): see D3D11Backend.h

// Local soft check: unlike the project's CheckHR (which throws _com_error),
// this logs and lets Init() return false so the caller falls back to D3D7.
#define D3D11_CHECK(hr, what)                                            \
	do {                                                                \
		if (FAILED(hr)) {                                               \
			MonoPrint("D3D11Backend: %s failed, hr=0x%08X\n",          \
			          (what), (unsigned)(hr));                          \
			return false;                                              \
		}                                                               \
	} while (0)

static inline void SafeRelease(IUnknown* p)
{
	if (p) p->Release();
}

D3D11Backend::D3D11Backend()
	: m_pDevice(NULL)
	, m_pContext(NULL)
	, m_pSwapChain(NULL)
	, m_pBackBufferRTV(NULL)
	, m_pDepthTex(NULL)
	, m_pDepthDSV(NULL)
	, m_pBlitStaging(NULL)
	, m_blitW(0)
	, m_blitH(0)
	, m_pMsaaColorTex(NULL)
	, m_pMsaaRTV(NULL)
	, m_pMsaaDepthTex(NULL)
	, m_pMsaaDSV(NULL)
	, m_msaaSamples(0)
	, m_pRttMsaaTex(NULL)
	, m_pRttMsaaRTV(NULL)
	, m_rttMsaaW(0)
	, m_rttMsaaH(0)
	, m_hWnd(NULL)
	, m_nWidth(0)
	, m_nHeight(0)
	, m_nDepth(24)
	, m_bFullscreen(false)
{
}

D3D11Backend::~D3D11Backend()
{
	Release();
}

bool D3D11Backend::Init(HWND hWnd, int nWidth, int nHeight, int nDepth, bool bFullscreen)
{
	MonoPrint("D3D11Backend::Init(0x%X, %d, %d, %d, %d)\n",
	          (unsigned)hWnd, nWidth, nHeight, nDepth, bFullscreen);

	// Artscout - 2026: Init may be called again on a mode change (entering 3D).
	// Do NOT recreate the device -- otherwise the renderer/VB-manager/staging hold the old
	// device while the context becomes new => D3D11 ValidateSameDevice crash.
	if (m_pDevice)
	{
		m_hWnd = hWnd;
		if (nWidth != m_nWidth || nHeight != m_nHeight)
			Resize(nWidth, nHeight);
		MonoPrint("D3D11Backend::Init - already initialized, reusing the device\n");
		return true;
	}

	m_hWnd        = hWnd;
	m_nWidth      = nWidth;
	m_nHeight     = nHeight;
	m_nDepth      = nDepth;
	m_bFullscreen = bFullscreen;

	DXGI_SWAP_CHAIN_DESC scd;
	ZeroMemory(&scd, sizeof(scd));
	scd.BufferCount        = 1;
	scd.BufferDesc.Width   = nWidth;
	scd.BufferDesc.Height  = nHeight;
	scd.BufferDesc.Format  = DXGI_FORMAT_R8G8B8A8_UNORM;
	scd.BufferDesc.RefreshRate.Numerator   = 0;   // let DXGI pick
	scd.BufferDesc.RefreshRate.Denominator = 1;
	scd.BufferUsage        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scd.OutputWindow       = hWnd;
	scd.SampleDesc.Count   = 1;                   // no MSAA in Phase 1
	scd.SampleDesc.Quality = 0;
	// Windowed for bring-up; DXGI exclusive fullscreen is deferred (Phase 5).
	scd.Windowed           = TRUE;
	scd.SwapEffect         = DXGI_SWAP_EFFECT_DISCARD;

	UINT createFlags = 0;
#ifdef _DEBUG
	createFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	const D3D_FEATURE_LEVEL wanted[] = {
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
	};
	D3D_FEATURE_LEVEL gotLevel = D3D_FEATURE_LEVEL_10_0;

	HRESULT hr = D3D11CreateDeviceAndSwapChain(
		NULL,                       // default adapter
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		createFlags,
		wanted, _countof(wanted),
		D3D11_SDK_VERSION,
		&scd,
		&m_pSwapChain,
		&m_pDevice,
		&gotLevel,
		&m_pContext);

	if (FAILED(hr)) {
		// Retry once with the debug layer dropped, in case the SDK layers
		// are not installed on the machine (common outside dev boxes).
		if (createFlags & D3D11_CREATE_DEVICE_DEBUG) {
			createFlags &= ~D3D11_CREATE_DEVICE_DEBUG;
			hr = D3D11CreateDeviceAndSwapChain(
				NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createFlags,
				wanted, _countof(wanted), D3D11_SDK_VERSION,
				&scd, &m_pSwapChain, &m_pDevice, &gotLevel, &m_pContext);
		}
	}
	D3D11_CHECK(hr, "D3D11CreateDeviceAndSwapChain");

	MonoPrint("D3D11Backend: device created, feature level 0x%X\n",
	          (unsigned)gotLevel);

	if (!CreateBackBufferViews())            return false;
	if (!CreateDepthBuffer(nWidth, nHeight, nDepth)) return false;
	CreateMsaaTargets(nWidth, nHeight);  // Artscout - 2026: #7 MSAA -- best-effort; on failure we run without MSAA

	// Bind the whole back buffer as the viewport for Phase 1.
	D3D11_VIEWPORT vp;
	vp.TopLeftX = 0.0f;
	vp.TopLeftY = 0.0f;
	vp.Width    = (FLOAT)nWidth;
	vp.Height   = (FLOAT)nHeight;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	m_pContext->RSSetViewports(1, &vp);

	MonoPrint("D3D11Backend::Init succeeded (%dx%d)\n", nWidth, nHeight);
	return true;
}

bool D3D11Backend::CreateBackBufferViews()
{
	ID3D11Texture2D* pBackBuffer = NULL;
	HRESULT hr = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D),
	                                     (void**)&pBackBuffer);
	D3D11_CHECK(hr, "SwapChain::GetBuffer");

	hr = m_pDevice->CreateRenderTargetView(pBackBuffer, NULL, &m_pBackBufferRTV);
	SafeRelease(pBackBuffer);
	D3D11_CHECK(hr, "CreateRenderTargetView");
	return true;
}

bool D3D11Backend::CreateDepthBuffer(int nWidth, int nHeight, int nDepth)
{
	// Artscout - 2026: Always with stencil (D24S8) -- needed for the 3D-pit mask (SetStencilMode).
	// D32_FLOAT has no stencil component, which let the sky/world cover the pit.
	DXGI_FORMAT fmt = DXGI_FORMAT_D24_UNORM_S8_UINT;
	(void)nDepth;

	D3D11_TEXTURE2D_DESC td;
	ZeroMemory(&td, sizeof(td));
	td.Width            = nWidth;
	td.Height           = nHeight;
	td.MipLevels        = 1;
	td.ArraySize        = 1;
	td.Format           = fmt;
	td.SampleDesc.Count = 1;
	td.Usage            = D3D11_USAGE_DEFAULT;
	td.BindFlags        = D3D11_BIND_DEPTH_STENCIL;

	HRESULT hr = m_pDevice->CreateTexture2D(&td, NULL, &m_pDepthTex);
	D3D11_CHECK(hr, "CreateTexture2D(depth)");

	D3D11_DEPTH_STENCIL_VIEW_DESC dsvd;
	ZeroMemory(&dsvd, sizeof(dsvd));
	dsvd.Format        = fmt;
	dsvd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvd.Texture2D.MipSlice = 0;

	hr = m_pDevice->CreateDepthStencilView(m_pDepthTex, &dsvd, &m_pDepthDSV);
	D3D11_CHECK(hr, "CreateDepthStencilView");
	return true;
}

// Artscout - 2026: #7 MSAA: create the multisample 3D-scene target (color+depth). Color format = swapchain
// format (R8G8B8A8) for ResolveSubresource. If unsupported -> false (fallback without MSAA).
bool D3D11Backend::CreateMsaaTargets(int nWidth, int nHeight)
{
	const DXGI_FORMAT colFmt = DXGI_FORMAT_R8G8B8A8_UNORM;	// Artscout - 2026: = swapchain format
	const DXGI_FORMAT depFmt = DXGI_FORMAT_D24_UNORM_S8_UINT;
	const UINT samples = 4;	// 4x MSAA

	UINT quality = 0;
	if (FAILED(m_pDevice->CheckMultisampleQualityLevels(colFmt, samples, &quality)) || quality == 0)
	{
		MonoPrint("D3D11Backend: MSAA %ux not supported -> without MSAA\n", samples);
		return false;
	}
	const UINT qLevel = quality - 1;
	bool ok = false;

	do
	{
		// Artscout - 2026: --- color (MSAA) ---
		D3D11_TEXTURE2D_DESC td; ZeroMemory(&td, sizeof(td));
		td.Width = nWidth; td.Height = nHeight; td.MipLevels = 1; td.ArraySize = 1;
		td.Format = colFmt;
		td.SampleDesc.Count = samples; td.SampleDesc.Quality = qLevel;
		td.Usage = D3D11_USAGE_DEFAULT; td.BindFlags = D3D11_BIND_RENDER_TARGET;
		if (FAILED(m_pDevice->CreateTexture2D(&td, NULL, &m_pMsaaColorTex))) break;

		D3D11_RENDER_TARGET_VIEW_DESC rtvd; ZeroMemory(&rtvd, sizeof(rtvd));
		rtvd.Format = colFmt; rtvd.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
		if (FAILED(m_pDevice->CreateRenderTargetView(m_pMsaaColorTex, &rtvd, &m_pMsaaRTV))) break;

		// Artscout - 2026: --- depth (MSAA, same sample count) ---
		D3D11_TEXTURE2D_DESC dd; ZeroMemory(&dd, sizeof(dd));
		dd.Width = nWidth; dd.Height = nHeight; dd.MipLevels = 1; dd.ArraySize = 1;
		dd.Format = depFmt;
		dd.SampleDesc.Count = samples; dd.SampleDesc.Quality = qLevel;
		dd.Usage = D3D11_USAGE_DEFAULT; dd.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		if (FAILED(m_pDevice->CreateTexture2D(&dd, NULL, &m_pMsaaDepthTex))) break;

		D3D11_DEPTH_STENCIL_VIEW_DESC dsvd; ZeroMemory(&dsvd, sizeof(dsvd));
		dsvd.Format = depFmt; dsvd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
		if (FAILED(m_pDevice->CreateDepthStencilView(m_pMsaaDepthTex, &dsvd, &m_pMsaaDSV))) break;

		ok = true;
	} while (0);

	if (!ok)
	{
		SafeRelease(m_pMsaaDSV);      m_pMsaaDSV = NULL;
		SafeRelease(m_pMsaaDepthTex); m_pMsaaDepthTex = NULL;
		SafeRelease(m_pMsaaRTV);      m_pMsaaRTV = NULL;
		SafeRelease(m_pMsaaColorTex); m_pMsaaColorTex = NULL;
		m_msaaSamples = 0;
		return false;
	}

	m_msaaSamples = samples;
	MonoPrint("D3D11Backend: MSAA %ux enabled (quality levels %u)\n", samples, quality);
	return true;
}

// Artscout - 2026: #7 AA-RTT: create the display MSAA atlas at the RTT size (best-effort). Color only (2D content).
void D3D11Backend::SetupRttMsaa(int w, int h)
{
	if (!m_pDevice || w <= 0 || h <= 0) return;
	if (m_pRttMsaaRTV && m_rttMsaaW == w && m_rttMsaaH == h) return; // Artscout - 2026: already exists at the needed size

	SafeRelease(m_pRttMsaaRTV); m_pRttMsaaRTV = NULL;
	SafeRelease(m_pRttMsaaTex); m_pRttMsaaTex = NULL;
	m_rttMsaaW = m_rttMsaaH = 0;

	const DXGI_FORMAT fmt = DXGI_FORMAT_R8G8B8A8_UNORM; // Artscout - 2026: = renderTexture format (for ResolveSubresource)
	const UINT samples = 4;
	UINT quality = 0;
	if (FAILED(m_pDevice->CheckMultisampleQualityLevels(fmt, samples, &quality)) || quality == 0)
	{
		MonoPrint("D3D11Backend: RTT MSAA %ux not supported -> displays without AA\n", samples);
		return;
	}

	D3D11_TEXTURE2D_DESC td; ZeroMemory(&td, sizeof(td));
	td.Width = w; td.Height = h; td.MipLevels = 1; td.ArraySize = 1;
	td.Format = fmt;
	td.SampleDesc.Count = samples; td.SampleDesc.Quality = quality - 1;
	td.Usage = D3D11_USAGE_DEFAULT; td.BindFlags = D3D11_BIND_RENDER_TARGET;
	if (FAILED(m_pDevice->CreateTexture2D(&td, NULL, &m_pRttMsaaTex)) || !m_pRttMsaaTex) { m_pRttMsaaTex = NULL; return; }

	D3D11_RENDER_TARGET_VIEW_DESC rtvd; ZeroMemory(&rtvd, sizeof(rtvd));
	rtvd.Format = fmt; rtvd.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
	if (FAILED(m_pDevice->CreateRenderTargetView(m_pRttMsaaTex, &rtvd, &m_pRttMsaaRTV)) || !m_pRttMsaaRTV)
	{
		SafeRelease(m_pRttMsaaTex); m_pRttMsaaTex = NULL; m_pRttMsaaRTV = NULL; return;
	}

	m_rttMsaaW = w; m_rttMsaaH = h;
	MonoPrint("D3D11Backend: RTT MSAA %ux enabled %dx%d\n", samples, w, h);
}

// Artscout - 2026: #7 AA-RTT: bind the MSAA atlas as render target (displays draw into it).
void D3D11Backend::BindRttMsaaRTV(int w, int h, bool clear, bool unbindSRV)
{
	if (!m_pContext || !m_pRttMsaaRTV) return;
	if (unbindSRV)
	{
		ID3D11ShaderResourceView* nullSRV[2] = { NULL, NULL };
		m_pContext->PSSetShaderResources(0, 2, nullSRV);
	}
	m_pContext->OMSetRenderTargets(1, &m_pRttMsaaRTV, NULL); // Artscout - 2026: without depth (2D content MFD/HUD)
	D3D11_VIEWPORT vp;
	vp.TopLeftX = 0; vp.TopLeftY = 0; vp.Width = (FLOAT)w; vp.Height = (FLOAT)h;
	vp.MinDepth = 0.0f; vp.MaxDepth = 1.0f;
	m_pContext->RSSetViewports(1, &vp);
	if (clear) { const FLOAT z[4] = { 0.0f, 0.0f, 0.0f, 0.0f }; m_pContext->ClearRenderTargetView(m_pRttMsaaRTV, z); }
}

// Artscout - 2026: #7 AA-RTT: resolve the MSAA atlas -> the normal renderTexture (destTex = its m_pD3D11Tex). After
// this the panel samples the resolved atlas via DrawRttQuad.
void D3D11Backend::ResolveRttMsaa(void* destTex)
{
	if (!m_pContext || !m_pRttMsaaTex || !destTex) return;
	m_pContext->OMSetRenderTargets(0, NULL, NULL); // Artscout - 2026: unbind the RTV (MSAA target) before resolve
	m_pContext->ResolveSubresource((ID3D11Texture2D*)destTex, 0, m_pRttMsaaTex, 0, DXGI_FORMAT_R8G8B8A8_UNORM);
}

// Artscout - 2026: #7 MSAA: resolve the multisample 3D-scene color into the swapchain backbuffer and bind the backbuffer
// as the current RTV (UI composites over it in CompositeUISurface).
void D3D11Backend::ResolveMsaaToBackBuffer()
{
	if (!m_pContext || !MsaaActive() || !m_pSwapChain) return;

	// Artscout - 2026: Unbind the RTVs (MSAA target), otherwise ResolveSubresource into a bound texture = conflict.
	m_pContext->OMSetRenderTargets(0, NULL, NULL);

	ID3D11Texture2D* pBackBuf = NULL;
	if (SUCCEEDED(m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuf)) && pBackBuf)
	{
		m_pContext->ResolveSubresource(pBackBuf, 0, m_pMsaaColorTex, 0, DXGI_FORMAT_R8G8B8A8_UNORM);
		pBackBuf->Release();
	}

	// Artscout - 2026: Backbuffer = current RTV (no depth) -> CompositeUISurface puts the 2D-UI over the 3D.
	m_pContext->OMSetRenderTargets(1, &m_pBackBufferRTV, NULL);
	D3D11_VIEWPORT vp;
	vp.TopLeftX = 0; vp.TopLeftY = 0;
	vp.Width = (FLOAT)m_nWidth; vp.Height = (FLOAT)m_nHeight;
	vp.MinDepth = 0.0f; vp.MaxDepth = 1.0f;
	m_pContext->RSSetViewports(1, &vp);
}

void D3D11Backend::BeginFrame(unsigned long argb)
{
	if (!m_pContext) return;

	// Artscout - 2026: #7 MSAA: the 3D scene renders into the multisample target (resolved to backbuffer at present).
	ID3D11RenderTargetView* rtv = MsaaActive() ? m_pMsaaRTV   : m_pBackBufferRTV;
	ID3D11DepthStencilView* dsv = MsaaActive() ? m_pMsaaDSV   : m_pDepthDSV;

	m_pContext->OMSetRenderTargets(1, &rtv, dsv);

	FLOAT rgba[4];
	rgba[0] = ((argb >> 16) & 0xFF) / 255.0f; // R
	rgba[1] = ((argb >>  8) & 0xFF) / 255.0f; // G
	rgba[2] = ((argb      ) & 0xFF) / 255.0f; // B
	rgba[3] = ((argb >> 24) & 0xFF) / 255.0f; // A

	m_pContext->ClearRenderTargetView(rtv, rgba);
	m_pContext->ClearDepthStencilView(dsv,
		D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

void D3D11Backend::ClearDepth()
{
	ID3D11DepthStencilView* dsv = MsaaActive() ? m_pMsaaDSV : m_pDepthDSV; // #7 MSAA
	if (m_pContext && dsv)
		m_pContext->ClearDepthStencilView(dsv,
			D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

void D3D11Backend::BindRenderTargetView(void* rtv, int w, int h, bool clear, bool unbindSRV)
{
	if (!m_pContext || !rtv) return;
	ID3D11RenderTargetView* pRTV = (ID3D11RenderTargetView*)rtv;

	// Artscout - 2026: RTT HAZARD (cause of "clear shows, content does not"): renderTexture may still be
	// bound as an SRV INPUT (from last frame's DrawRttQuad, currentTexture1 cache).
	// D3D11 forbids one texture as input and output at once -> it auto-unbinds the
	// RTV, and all display draws go nowhere (while ClearRenderTargetView works
	// directly, bypassing the binding -> which is why the red clear was visible). We unbind SRV inputs
	// once in StartRtt (unbindSRV=true). Not needed in AdjustRttViewport -- there the slot
	// already holds the display font, not renderTexture, so there is no hazard (and unbinding would break the texture cache).
	if (unbindSRV)
	{
		ID3D11ShaderResourceView* nullSRV[2] = { NULL, NULL };
		m_pContext->PSSetShaderResources(0, 2, nullSRV);
	}

	m_pContext->OMSetRenderTargets(1, &pRTV, NULL);	// Artscout - 2026: without depth (2D content MFD/HUD)

	D3D11_VIEWPORT vp;
	vp.TopLeftX = 0; vp.TopLeftY = 0;
	vp.Width = (FLOAT)w; vp.Height = (FLOAT)h;
	vp.MinDepth = 0.0f; vp.MaxDepth = 1.0f;
	m_pContext->RSSetViewports(1, &vp);

	// Artscout - 2026: RTT: clear the renderTexture to transparent black (chroma/alpha removes empty areas).
	if (clear) { const FLOAT z[4] = { 0.0f, 0.0f, 0.0f, 0.0f }; m_pContext->ClearRenderTargetView(pRTV, z); }
}

void D3D11Backend::SetViewportRect(int x, int y, int w, int h)
{
	if (!m_pContext) return;
	if (w < 0) w = 0;
	if (h < 0) h = 0;
	D3D11_VIEWPORT vp;
	vp.TopLeftX = (FLOAT)x; vp.TopLeftY = (FLOAT)y;
	vp.Width = (FLOAT)w;    vp.Height = (FLOAT)h;
	vp.MinDepth = 0.0f;     vp.MaxDepth = 1.0f;
	m_pContext->RSSetViewports(1, &vp);
}

void D3D11Backend::ClearCurrentRTV(float r, float g, float b, float a)
{
	if (!m_pContext) return;
	ID3D11RenderTargetView* rtv = NULL;
	m_pContext->OMGetRenderTargets(1, &rtv, NULL);
	if (rtv)
	{
		const FLOAT c[4] = { r, g, b, a };
		m_pContext->ClearRenderTargetView(rtv, c);
		rtv->Release();
	}
}

void D3D11Backend::BindBackBuffer(bool bClearDepth)
{
	if (!m_pContext) return;

	// Artscout - 2026: #7 MSAA: returning to the 3D target = the multisample target (when MSAA is active), else the backbuffer.
	ID3D11RenderTargetView* rtv = MsaaActive() ? m_pMsaaRTV : m_pBackBufferRTV;
	ID3D11DepthStencilView* dsv = MsaaActive() ? m_pMsaaDSV : m_pDepthDSV;

	m_pContext->OMSetRenderTargets(1, &rtv, dsv);

	D3D11_VIEWPORT vp;
	vp.TopLeftX = 0; vp.TopLeftY = 0;
	vp.Width    = (FLOAT)m_nWidth;
	vp.Height   = (FLOAT)m_nHeight;
	vp.MinDepth = 0.0f; vp.MaxDepth = 1.0f;
	m_pContext->RSSetViewports(1, &vp);

	if (bClearDepth && dsv)
	{
		m_pContext->ClearDepthStencilView(dsv,
			D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
		// Artscout - 2026: 3D-frame base color (dark blue).
		const FLOAT bg[4] = { 0.05f, 0.10f, 0.20f, 1.0f };
		if (rtv) m_pContext->ClearRenderTargetView(rtv, bg);
	}
}

void D3D11Backend::Present(bool bVSync)
{
	if (m_pSwapChain)
		m_pSwapChain->Present(bVSync ? 1 : 0, 0);
}

// Artscout - 2026: Copies a 16-bit RGB565 CPU bitmap into the back buffer (R8G8B8A8): a staging texture of the
// needed size -> Map -> 565->RGBA8 conversion -> Unmap -> CopyResource into the back buffer.
// No shader. The source size is clipped to the swapchain size.
void D3D11Backend::BlitBitmap565(const void* pSrc565, int srcW, int srcH)
{
	if (!m_pDevice || !m_pContext || !m_pSwapChain || !pSrc565 || srcW <= 0 || srcH <= 0)
		return;

	// Artscout - 2026: Lazily (re)create the staging texture for the backbuffer size.
	if (!m_pBlitStaging || m_blitW != m_nWidth || m_blitH != m_nHeight)
	{
		SafeRelease(m_pBlitStaging); m_pBlitStaging = NULL;

		D3D11_TEXTURE2D_DESC td;
		ZeroMemory(&td, sizeof(td));
		td.Width            = m_nWidth;
		td.Height           = m_nHeight;
		td.MipLevels        = 1;
		td.ArraySize        = 1;
		td.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;	// Artscout - 2026: = swapchain format
		td.SampleDesc.Count = 1;
		td.Usage            = D3D11_USAGE_STAGING;
		td.CPUAccessFlags   = D3D11_CPU_ACCESS_WRITE;
		if (FAILED(m_pDevice->CreateTexture2D(&td, NULL, &m_pBlitStaging)))
			return;
		m_blitW = m_nWidth;
		m_blitH = m_nHeight;
	}

	D3D11_MAPPED_SUBRESOURCE mr;
	if (FAILED(m_pContext->Map(m_pBlitStaging, 0, D3D11_MAP_WRITE, 0, &mr)))
		return;

	const int copyW = (srcW < m_nWidth)  ? srcW : m_nWidth;
	const int copyH = (srcH < m_nHeight) ? srcH : m_nHeight;
	const unsigned short* src = (const unsigned short*)pSrc565;

	for (int y = 0; y < copyH; ++y)
	{
		unsigned char*        dst = (unsigned char*)mr.pData + (size_t)y * mr.RowPitch;
		const unsigned short* srow = src + (size_t)y * srcW;
		for (int x = 0; x < copyW; ++x)
		{
			unsigned short p = srow[x];
			unsigned r = (p >> 11) & 0x1F;
			unsigned g = (p >> 5)  & 0x3F;
			unsigned b =  p        & 0x1F;
			unsigned char* d = dst + (size_t)x * 4;
			d[0] = (unsigned char)((r * 255 + 15) / 31);	// R
			d[1] = (unsigned char)((g * 255 + 31) / 63);	// G
			d[2] = (unsigned char)((b * 255 + 15) / 31);	// B
			d[3] = 255;										// A
		}
	}

	m_pContext->Unmap(m_pBlitStaging, 0);

	ID3D11Texture2D* pBackBuf = NULL;
	if (SUCCEEDED(m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuf)) && pBackBuf)
	{
		m_pContext->CopyResource(pBackBuf, m_pBlitStaging);
		pBackBuf->Release();
	}
}

void D3D11Backend::ReleaseSizeDependent()
{
	if (m_pContext) m_pContext->OMSetRenderTargets(0, NULL, NULL);
	// #7 MSAA
	SafeRelease(m_pMsaaDSV);          m_pMsaaDSV = NULL;
	SafeRelease(m_pMsaaDepthTex);     m_pMsaaDepthTex = NULL;
	SafeRelease(m_pMsaaRTV);          m_pMsaaRTV = NULL;
	SafeRelease(m_pMsaaColorTex);     m_pMsaaColorTex = NULL;
	m_msaaSamples = 0;
	SafeRelease(m_pDepthDSV);        m_pDepthDSV = NULL;
	SafeRelease(m_pDepthTex);        m_pDepthTex = NULL;
	SafeRelease(m_pBackBufferRTV);   m_pBackBufferRTV = NULL;
}

bool D3D11Backend::Resize(int nWidth, int nHeight)
{
	if (!m_pSwapChain) return false;
	if (nWidth <= 0 || nHeight <= 0) return false;

	ReleaseSizeDependent();

	HRESULT hr = m_pSwapChain->ResizeBuffers(0, nWidth, nHeight,
		DXGI_FORMAT_UNKNOWN, 0);
	D3D11_CHECK(hr, "SwapChain::ResizeBuffers");

	m_nWidth  = nWidth;
	m_nHeight = nHeight;

	if (!CreateBackBufferViews())                    return false;
	if (!CreateDepthBuffer(nWidth, nHeight, m_nDepth)) return false;
	CreateMsaaTargets(nWidth, nHeight);  // Artscout - 2026: #7 MSAA -- recreate for the new size

	D3D11_VIEWPORT vp;
	vp.TopLeftX = 0.0f; vp.TopLeftY = 0.0f;
	vp.Width    = (FLOAT)nWidth; vp.Height = (FLOAT)nHeight;
	vp.MinDepth = 0.0f; vp.MaxDepth = 1.0f;
	m_pContext->RSSetViewports(1, &vp);
	return true;
}

void D3D11Backend::Release()
{
	ReleaseSizeDependent();

	// Artscout - 2026: #7 AA-RTT: display MSAA atlas (independent of screen size -> release here, not in ReleaseSizeDependent).
	SafeRelease(m_pRttMsaaRTV); m_pRttMsaaRTV = NULL;
	SafeRelease(m_pRttMsaaTex); m_pRttMsaaTex = NULL;
	m_rttMsaaW = m_rttMsaaH = 0;

	SafeRelease(m_pBlitStaging); m_pBlitStaging = NULL;
	m_blitW = m_blitH = 0;

	if (m_pSwapChain) {
		// DXGI forbids releasing a swap chain in exclusive fullscreen.
		m_pSwapChain->SetFullscreenState(FALSE, NULL);
	}
	SafeRelease(m_pSwapChain); m_pSwapChain = NULL;
	SafeRelease(m_pContext);   m_pContext   = NULL;
	SafeRelease(m_pDevice);    m_pDevice    = NULL;
	m_hWnd = NULL;
}
