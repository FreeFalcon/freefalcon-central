//-----------------------------------------------------------------------------
// IRenderBackend.h -- Artscout - 2026: #DX12 render-backend abstraction.
//
// The API-NEUTRAL surface of the window render backend (device + swap chain +
// present + the 2D/present plumbing that is meaningful for ANY GPU API). Both
// D3D11Backend and D3D12Backend implement it, and the app talks to whichever is
// active through the single global g_pRenderBackend -- so the pervasive
// `if (g_bUseD3D11) ...` device/present branches become one polymorphic call.
//
// DELIBERATELY NOT here: the D3D11-specific plumbing (GetDevice/GetContext ->
// ID3D11* types, the RTT-MSAA / VR per-eye / menu-RTT targets, MirrorEyeToBackBuffer).
// Those pass D3D11 interface pointers and are tied to the D3D11 RTT/OpenXR path;
// they stay concrete on D3D11Backend and get a from-scratch D3D12 implementation
// in the later RTT (Phase 4) and OpenXR-D3D12 (Phase 5) phases -- not shared here.
//-----------------------------------------------------------------------------
#ifndef _IRENDERBACKEND_H_
#define _IRENDERBACKEND_H_

#include <windows.h>

class IRenderBackend
{
public:
	virtual ~IRenderBackend() {}

	virtual bool Init(HWND hWnd, int nWidth, int nHeight, int nDepth, bool bFullscreen) = 0;
	virtual void Release() = 0;
	virtual bool Resize(int nWidth, int nHeight) = 0;
	virtual bool IsValid() const = 0;

	// Frame loop
	virtual void BeginFrame(unsigned long argbClearColor) = 0;
	virtual void BindBackBuffer(bool bClearDepth) = 0;
	virtual void ClearDepth() = 0;
	virtual void SetViewportRect(int x, int y, int w, int h) = 0;
	virtual void ResolveMsaaToBackBuffer() = 0;
	virtual void Present(bool bVSync) = 0;

	// 2D UI blit (RGB565 CPU bitmap -> back buffer) + the cbViewport screen size.
	virtual void BlitBitmap565(const void* pSrc565, int srcW, int srcH) = 0;
	virtual void SetGScreenSize(int w, int h) = 0;
	virtual void ClearCurrentRTV(float r, float g, float b, float a) = 0;
	virtual void FlushContext() = 0;

	// Geometry
	virtual int  Width()  const = 0;
	virtual int  Height() const = 0;
	virtual HWND Hwnd()   const = 0;
};

// The active backend (D3D11Backend or D3D12Backend), set in DXContext::Init. NULL
// under the legacy D3D7/DDraw path. Use this for API-neutral device/present calls.
extern IRenderBackend* g_pRenderBackend;

#endif // _IRENDERBACKEND_H_
