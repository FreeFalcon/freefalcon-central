//-----------------------------------------------------------------------------
// D3D12Renderer.h -- Artscout - 2026: #DX12 Phase 3.
//
// The D3D12 implementation of IRenderer (the API-neutral scene-renderer surface).
// Mirrors D3D11Renderer but on D3D12: PSO cache (one PSO per state/pass combo,
// since D3D12 bakes blend/depth/raster/shaders/RTV into a single object), per-frame
// upload constant buffers (b0..b4 of FFEmu.hlsl), a shader-visible descriptor heap
// for the two textures (t0,t1) + static samplers, and a dynamic VB/IB. Draw calls
// record into the D3D12Backend's CURRENT command list (opened by BeginFrame).
//
// PHASE 3 (this increment): the draw machinery is implemented -- PSO cache, per-frame
// UPLOAD rings for constants / vertices / indices, and the screen / object / terrain /
// dynamic-2D passes recording into the backend command list. Textures (SetTexture),
// the RTT composite, and the object-VB path (aircraft/cockpit BSP) are later increments;
// until then geometry draws flat vertex colour against a 1x1 white default (t0,t1).
// NOTE: the render call-sites still target g_pD3D11Renderer (gated by g_bUseD3D11), so
// nothing calls this yet -- wiring (call-site migration + frame bracketing) is the next step.
//-----------------------------------------------------------------------------
#ifndef _D3D12RENDERER_H_
#define _D3D12RENDERER_H_

#include <windows.h>
#include "Graphics/DXEngine/d3d11/IRenderer.h"   // the neutral interface + D3D11_TLVERTEX (shared POD)

struct ID3D12Device;
struct ID3D12GraphicsCommandList;
struct ID3D12RootSignature;
struct ID3D12PipelineState;
struct ID3D12DescriptorHeap;
struct ID3D12Resource;
// NOTE: ID3DBlob is a TYPEDEF (d3dcommon.h) -> cannot be forward-declared as a struct. The compiled
// shader blobs are held as void* here and cast to ID3DBlob* in the .cpp.

// Artscout - 2026: #VFX Phase 1 -- one GPU-instanced billboard particle. The BYTE LAYOUT here MUST
// match the particle per-instance input layout (slot 1, particleIL) in D3D12Renderer.cpp AND
// VSInParticleInst in FFEmu.hlsl. `color` is a packed D3DCOLOR (0xAARRGGBB) read as B8G8R8A8_UNORM
// (BGRA in memory), i.e. the same convention the engine's vertex colours use -- so a caller can pass a
// Falcon colour dword directly. Callers fill an array of these and hand it to DrawParticlesInstanced.
struct D3D12ParticleInstance
{
	float    center[3];   // world position                          (offset  0)
	float    size[2];     // world width,height of the billboard      (offset 12)
	float    rot;         // billboard-plane rotation (radians)       (offset 20)
	unsigned color;       // packed D3DCOLOR ARGB (BGRA bytes)         (offset 24)
	float    uvRect[4];   // xy = atlas uv offset, zw = atlas uv scale (offset 28)
};                        // total 44 bytes

class D3D12Renderer : public IRenderer
{
public:
	D3D12Renderer();
	~D3D12Renderer();

	// --- IRenderer ---
	bool Init(const char* shaderDir);
	void Release();
	bool IsValid() const { return m_pDevice != 0 && m_pRootSig != 0; }

	void SetViewportSize(int w, int h);
	int  ScreenW() const { return m_screenW; }   // Artscout - 2026: current gScreenSize (DIAG/RTT), VS_Screen divisor
	int  ScreenH() const { return m_screenH; }
	// #DX12: the backend calls this when it (un)binds a depth-stencil. When no DSV is bound (the RTT display
	// atlas), depth is forced OFF in every PSO so a null DSV is legal (else #615 drops the draw -> RTT blank).
	void SetDepthTargetBound(bool b) { m_depthTargetBound = b; }
	// #DX12: the texture manager calls this (via D3D12Renderer_NotifyTextureFreed) right before it deletes a
	// D3D12Texture. m_pTex0/m_pTex1 hold the CURRENTLY bound D3D12Texture* -- if it's the one being freed (e.g.
	// on 3D-exit texture cleanup), drop the pointer so the next FlushConstants doesn't read a freed struct's
	// srvCpuPtr and feed a dangling descriptor into CopyDescriptorsSimple (debug-layer break / crash on re-entry).
	void OnTextureFreed(const void* tex) { if (m_pTex0 == tex) { m_pTex0 = 0; m_tableDirty = true; } if (m_pTex1 == tex) { m_pTex1 = 0; m_tableDirty = true; } }
	void SetView(const float* m);
	void SetProj(const float* m);
	void SetWorld(const float* m);
	void SetCameraPos(float x, float y, float z);
	void SetState(int legacyState);

	void SetTexture(unsigned slot, ID3D11ShaderResourceView* srv);
	void SetChromaKey(unsigned long argb, float tolerance);
	void SetFog(unsigned long argb, float start, float end);
	void SetAlphaRef(float ref01);
	void SetMaterialColor(float r, float g, float b, float a);
	void SetMaterialSpecular(float r, float g, float b, float power);
	// Artscout - 2026: #DX12 -- D3D12-only (not on IRenderer): fills the cbLights shadow for a future
	// D3D12 object path. Unused by the current terrain/dynamic2D passes (flat vertex colour, no lighting).
	void SetLights(const float ambient[4], int numLights, const void* lights, int lightStride = 64);
	void SetObjectAlphaBlend(bool on);
	void SetObjectAdditiveBlend(bool on);
	void SetAlphaTestEnabled(bool on);
	void SetEmissive(bool on);
	void SetAfterburner(bool on);
	void SetCockpitPass(bool on);
	void SetIRGrey(bool on);   // #DX12 A5: sensor pass -> grey (luma in PS)
	void SetTexColorDiffuse(bool on);
	void SetForcePerSample(bool on);
	void SetStencil(int mode, unsigned ref);
	void SetHudStencil(int mode);

	void BeginScreenPass();
	void BeginObjectPass();
	void BeginTerrainPass();
	void SetTerrainRasterForLod(int level);
	void RebuildTerrainRasters();

	void DrawTL(int primType, const D3D11_TLVERTEX* verts, int count);
	void DrawTLIndexed(int primType, const D3D11_TLVERTEX* verts, int vcount,
	                   const unsigned short* indices, int icount);
	void DrawColorTrisScreen(const D3D11_TLVERTEX* verts, int count,
	                         ID3D11ShaderResourceView* tex, int opaque, int cull);
	void DrawBitmap2D(int dX, int dY, int w, int h, int totalWidth, int sX, int sY,
	                  const unsigned* pSrc, bool fit, int screenW, int screenH);
	void DrawTerrainMesh(const void* verts, int vcount, const unsigned short* indices, int icount);
	void BeginDynamic2D(bool additive);
	void UploadDynamic2D(const void* dynVerts, int vcount);
	void DrawDynamic2D(const void* dynVerts, int vcount, ID3D11ShaderResourceView* srv, int primType);
	void DrawDynamic2DIndexed(const unsigned short* indices, int icount, ID3D11ShaderResourceView* srv, int primType);

	void CompositeUISurface(const void* src565, int w, int h);
	ID3D11ShaderResourceView* LoadTextureFile(const char* path);

	// #DX12 п.4: object/BSP path. vbHandle = ID3D12Resource* (per-model VB from the VB manager).
	void DrawObjectIndexed(int primType, void* vbHandle, int stride, int baseVertex,
	                       const unsigned short* indices, int indexCount);
	void DrawObjectStrip(int primType, void* vbHandle, int stride, int startVertex, int vertexCount);

	// Artscout - 2026: #VFX Phase 1 -- GPU-instanced billboard particles. `inst` is an array of
	// `count` D3D12ParticleInstance (const void* on IRenderer to keep the D3D12 struct out of the
	// neutral header). `atlasSrv` is the opaque texture handle (D3D12Texture*) for gTex0. blendMode:
	// 0 = additive (glow), 1 = alpha. One DrawIndexedInstanced(6, count) draws every particle.
	void DrawParticlesInstanced(const void* inst, int count, void* atlasSrv, int blendMode);

private:
	enum { kFrames = 2 };   // matches D3D12Backend::kFrameCount

	bool CompileShaders(const char* shaderDir);
	bool CreateRootSignature();
	bool CreateConstantBuffers();      // per-frame constant ring
	bool CreateWhiteTexture();         // 1x1 white default SRV (t0,t1) -- data uploaded lazily on 1st frame

	// Draw helpers.
	struct ID3D12GraphicsCommandList* Cmd();           // the backend's open command list (NULL if not recording)
	void  BeginFrameStateIfNeeded();                   // detect a new frame -> reset rings, re-bind root/heaps/CBVs
	void  FlushConstants();                            // snapshot dirty CBs into the ring + rebind changed root CBVs
	void  EnsureWhiteTexture();                        // record the 1x1 white upload once (needs a recording list)
	struct ID3D12PipelineState* GetPSO(int pass, int blend, bool dWrite, bool dTest, int topoType, int cull, int bias);
	// Artscout - 2026: #VFX Phase 1 -- dedicated PSO for the instanced particle program (VS_Particle/
	// PS_Particle + particleIL). blendMode: 0 additive, 1 alpha. Depth: test yes / write no (glows sort
	// behind opaque geometry without occluding each other). Cached in the shared PSO map on a disjoint key.
	struct ID3D12PipelineState* GetParticlePSO(int blendMode);
	bool  EnsureParticleStatics();   // lazily create the static unit-quad VB/IB + one-time IB state barrier
	unsigned __int64 AllocCB(const void* src, unsigned bytes);                       // -> GPU VA of the copied region
	bool  AllocVB(const void* src, unsigned bytes, unsigned stride, void* outVbv);   // fill D3D12_VERTEX_BUFFER_VIEW
	bool  AllocInstanceVB(const void* data, int bytes, void* outVbv);                // #VFX per-instance stream (VB ring)
	bool  AllocIB(const unsigned short* src, unsigned icount, void* outIbv);         // fill D3D12_INDEX_BUFFER_VIEW
	void  MarkRenderDirty() { m_dRender = true; }

	// Artscout - 2026: #VFX -- the DynV vertex buffer staged by UploadDynamic2D for the following
	// per-item DrawDynamic2DIndexed calls in DX2D_Flush2DObjects. Revives ALL legacy DX2D 2D-in-3D
	// effects under D3D12 (smoke trails/tapes -> missile & flare smoke, blips) that the old stub dropped.
	const void*                m_dyn2DVerts;   // borrowed CPU pointer, valid across one flush
	int                        m_dyn2DVcount;  // vertices staged (index bound-check)

	ID3D12Device*              m_pDevice;    // borrowed from g_pD3D12Backend
	ID3D12RootSignature*       m_pRootSig;

	void*                      m_pVSScreen;  // ID3DBlob* -- FFEmu VS_Screen (DXBC), kept for PSO creation
	void*                      m_pVSObject;  // ID3DBlob* -- FFEmu VS_Object
	void*                      m_pPSMain;    // ID3DBlob* -- FFEmu PS_Main
	void*                      m_pVSParticle; // Artscout - 2026: #VFX ID3DBlob* -- FFEmu VS_Particle
	void*                      m_pPSParticle; // Artscout - 2026: #VFX ID3DBlob* -- FFEmu PS_Particle

	// Artscout - 2026: #VFX Phase 1 -- static geometry shared by every instanced particle draw: a unit quad
	// (4 verts, slot 0) + a 6-index buffer. Created once via the texture manager (DEFAULT heap); the IB is
	// transitioned VERTEX_AND_CONSTANT_BUFFER -> INDEX_BUFFER once on the first draw (needs an open list).
	ID3D12Resource*            m_pQuadVB;
	ID3D12Resource*            m_pQuadIB;
	bool                       m_particleIbReady;

	void*                      m_pPsoCache;  // std::map<unsigned, ID3D12PipelineState*>* (opaque here)

	// #DX12 п.1 textures: per-draw the two SRVs (t0,t1) are COPIED into a per-frame shader-visible ring
	// (CopyDescriptorsSimple) and the root table points at that 2-slot region. Sources are the texture
	// manager's staging SRVs (D3D12Texture::srvCpuPtr) or the 1x1 white default (untextured slots).
	ID3D12DescriptorHeap*      m_pSrvRing[kFrames];   // shader-visible; per-frame descriptor ring
	unsigned                   m_srvRingOff[kFrames]; // bump offset (descriptors)
	unsigned                   m_srvRingCount;        // capacity (descriptors per ring)
	unsigned                   m_srvInc;              // CBV_SRV_UAV descriptor increment
	ID3D12DescriptorHeap*      m_pWhiteStaging;       // CPU heap: 1 white SRV (copy source for untextured slots)
	unsigned __int64           m_whiteSrvCpu;         // white SRV CPU handle .ptr
	ID3D12Resource*            m_pWhiteTex;
	ID3D12Resource*            m_pWhiteUpload;
	bool                       m_whiteUploaded;
	void*                      m_pTex0;               // D3D12Texture* bound to t0 (NULL -> white)
	void*                      m_pTex1;               // D3D12Texture* bound to t1
	bool                       m_tableDirty;          // re-copy + re-bind the SRV table before next draw

	// Per-frame upload rings (double-buffered so an in-flight frame is never overwritten).
	ID3D12Resource*            m_pCbRing[kFrames]; unsigned char* m_pCbCpu[kFrames]; unsigned m_cbSize[kFrames]; unsigned m_cbOff[kFrames];
	ID3D12Resource*            m_pVbRing[kFrames]; unsigned char* m_pVbCpu[kFrames]; unsigned m_vbSize[kFrames]; unsigned m_vbOff[kFrames];
	ID3D12Resource*            m_pIbRing[kFrames]; unsigned char* m_pIbCpu[kFrames]; unsigned m_ibSize[kFrames]; unsigned m_ibOff[kFrames];
	unsigned                   m_curFrame;    // backend frame index = which ring slot (0/1) is live
	unsigned                   m_curEpoch;    // #DX12 п.5: backend RenderEpoch we last synced (VR eyes bump it)
	bool                       m_frameRebind; // list was reset -> re-set root sig/heaps and all CBVs

	// Shadow of the CB contents (updated by Set*; flushed before a draw).
	float                      m_view[16], m_proj[16], m_world[16];
	float                      m_camPos[4];
	int                        m_screenW, m_screenH;
	unsigned                   m_flags;
	int                        m_curState;
	bool                       m_valid;

	// Shadow render-state (baked into the PSO / cbRender at draw time -- mirrors D3D11Renderer).
	int                        m_pass;        // 0 = screen, 1 = object
	int                        m_blend;       // FFBlendMode
	bool                       m_depthWrite, m_depthTest;
	bool                       m_depthTargetBound;   // false when the bound target has NO DSV (RTT atlas) -> force depth-off PSOs (#615)
	int                        m_hudStencil;         // #76 HUD aperture stencil: 0 OFF / 1 MARK (write 0x80) / 2 TEST (draw where (s&0xC0)==0x80)
	int                        m_cull;        // 0 none, 1 back, 2 front
	int                        m_bias;        // 0 none, 1 object (#16 depth-bias toward camera)
	bool                       m_forcePerSample;

	float                      m_alphaRef, m_fogStart, m_fogEnd;
	unsigned long              m_fogColor, m_chromaKey;
	float                      m_chromaTol;
	float                      m_materialColor[4], m_specular[4];
	bool                       m_texColorDiffuse, m_cockpitPass, m_hasTex0;
	bool                       m_irGrey;   // #DX12 A5: sensor grey pass (sticky, like m_cockpitPass)
	unsigned char              m_lightsBuf[16 + 16 + 8 * 64];   // cbLights shadow (ambient, num, pad, 8 lights)

	// Dirty flags: which CBs changed since last bind (per-slot root CBV rebind).
	bool                       m_dViewport, m_dView, m_dObject, m_dRender, m_dLights;
};

extern D3D12Renderer* g_pD3D12Renderer;   // concrete instance; g_pRenderer aliases it under D3D12

#endif // _D3D12RENDERER_H_
