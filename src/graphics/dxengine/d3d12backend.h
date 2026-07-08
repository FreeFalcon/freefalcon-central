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

// Forward declarations keep d3d12.h out of the widely-included headers (same
// discipline as d3d11backend.h with d3d11.h).
struct ID3D12Device;
struct ID3D12CommandQueue;
struct ID3D12DescriptorHeap;
struct ID3D12Resource;
struct ID3D12CommandAllocator;
struct ID3D12GraphicsCommandList;
struct ID3D12Fence;
struct IDXGISwapChain3;

class D3D12Backend
{
public:
	D3D12Backend();
	~D3D12Backend();

	// Creates the device + flip swap chain bound to hWnd. Returns false (and logs
	// via MonoPrint) on any failure so the caller can fall back to D3D11/D3D7.
	// Windowed for bring-up; DXGI exclusive fullscreen is deferred.
	bool Init(HWND hWnd, int nWidth, int nHeight, int nDepth, bool bFullscreen);

	// Releases everything (waits for the GPU to idle first). Safe to call twice.
	void Release();

	bool IsValid() const { return m_pDevice != 0; }

	// --- Frame loop (Phase 1 uses only these) --------------------------------

	// Begin recording this frame: wait for the target back buffer's fence, reset
	// the allocator + command list, transition the back buffer to RENDER_TARGET,
	// bind it, and clear to argb (0xAARRGGBB).
	void BeginFrame(unsigned long argbClearColor = 0xFF203040);

	// Transition the back buffer to PRESENT, close+execute the command list,
	// Present through the flip swap chain, and signal the fence for this frame.
	void Present(bool bVSync = true);

	// Recreate the swap-chain back buffers at a new size (waits for GPU idle).
	void Resize(int nWidth, int nHeight);

	// For later phases (renderer / OpenXR-D3D12 binding).
	ID3D12Device*       GetDevice() const { return m_pDevice; }
	ID3D12CommandQueue* GetQueue()  const { return m_pQueue; }

private:
	bool CreateBackBufferViews();
	void ReleaseBackBufferViews();
	void WaitForGpu();          // block until the GPU has finished ALL submitted work
	void MoveToNextFrame();     // signal current frame's fence, advance, wait if the next is in flight

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
	ID3D12Resource*           m_pBackBuffer[kFrameCount];
	ID3D12CommandAllocator*   m_pAlloc[kFrameCount];
	ID3D12GraphicsCommandList* m_pList;

	ID3D12Fence*              m_pFence;
	unsigned __int64          m_fenceValue[kFrameCount];
	unsigned __int64          m_fenceCounter;
	HANDLE                    m_fenceEvent;
	unsigned                  m_frameIndex;
};

extern D3D12Backend* g_pD3D12Backend;   // NULL unless g_bUseD3D12 brought it up
extern bool          g_bUseD3D12;       // runtime selector, parallel to g_bUseD3D11

#endif // _D3D12BACKEND_H_
