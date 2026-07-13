//-----------------------------------------------------------------------------
// IRenderer.h -- Artscout - 2026: #DX12 renderer abstraction.
//
// The API-NEUTRAL draw/state surface of the scene renderer. Both D3D11Renderer
// and (later) D3D12Renderer implement it; the engine talks to whichever is active
// through g_pRenderer, so the pervasive `g_pD3D11Renderer->...` calls become
// backend-agnostic.
//
// Pragmatic typing (matches the D3D11 signatures 1:1 so D3D11Renderer inherits
// with ZERO call-site churn): the texture handle stays typed as
// ID3D11ShaderResourceView* but is treated as an OPAQUE handle -- each backend
// interprets it as its own texture object. ScreenVertex is a plain POD vertex
// (layout-neutral) shared by both backends. Real neutral handle types can come
// later; keeping signatures identical is what makes this a safe, inert first step.
//
// DELIBERATELY NOT here (stay concrete on D3D11Renderer, reimplemented per-API in
// the object phase): SetLights (nested GpuLightCPU), DrawObjectIndexed/Strip
// (take an ID3D11Buffer* from the D3D11 VB manager).
//-----------------------------------------------------------------------------
#ifndef _IRENDERER_H_
#define _IRENDERER_H_

struct ID3D11ShaderResourceView;   // opaque texture handle (each backend interprets)

// Artscout - 2026 (D3D11 purge): these types were defined on/inside D3D11Renderer(.h); moved here to the
// neutral header (and de-D3D11'd) when the D3D11 backend was retired -- the layouts are backend-agnostic.
// POD transformed+lit screen vertex (XYZRHW), shared by both backends. (Renamed from D3D11_TLVERTEX.)
struct ScreenVertex
{
	float sx, sy, sz, rhw;     // screen pos + 1/w
	unsigned long color;       // D3DCOLOR ARGB
	unsigned long specular;    // D3DCOLOR ARGB (fog)
	float tu0, tv0;
	float tu1, tv1;
};

// One light for the object path (mirrors GpuLight in FFEmu.hlsl). Was GpuLightCPU.
struct GpuLightCPU
{
	float Position[4];   // xyz, w
	float Direction[4];  // xyz normalized
	float Color[4];      // rgb
	float Params[4];     // x=range, y=type(0=dir,1=point)
};

// HUD aperture-stencil mode for SetHudStencil(). Was the D3D11Renderer::{HUD_STENCIL_*} enum.
enum { HUD_STENCIL_OFF = 0, HUD_STENCIL_MARK = 1, HUD_STENCIL_TEST = 2 };

// VR controller/hand model triangle vertex (screen-space, pre-projected). Was D3D11Backend::VrTriVtx;
// vcock builds these then converts to ScreenVertex for g_pRenderer->DrawColorTrisScreen.
struct VrTriVtx { float x, y; unsigned color; float u, v; };

class IRenderer
{
public:
	virtual ~IRenderer() {}

	virtual bool Init(const char* shaderDir) = 0;
	virtual void Release() = 0;
	virtual bool IsValid() const = 0;

	// Per-frame / per-pass state
	virtual void SetViewportSize(int w, int h) = 0;
	virtual void SetView(const float* m4x4_rowmajor) = 0;
	virtual void SetProj(const float* m4x4_rowmajor) = 0;
	virtual void SetWorld(const float* m4x4_rowmajor) = 0;
	virtual void SetCameraPos(float x, float y, float z) = 0;
	virtual void SetState(int legacyState) = 0;

	// Render parameters
	virtual void SetTexture(unsigned slot, ID3D11ShaderResourceView* srv) = 0;
	virtual void SetChromaKey(unsigned long argb, float tolerance) = 0;
	virtual void SetFog(unsigned long argb, float start, float end) = 0;
	virtual void SetAlphaRef(float ref01) = 0;
	virtual void SetMaterialColor(float r, float g, float b, float a) = 0;
	virtual void SetMaterialSpecular(float r, float g, float b, float power) = 0;
	virtual void SetObjectAlphaBlend(bool on) = 0;
	virtual void SetObjectAdditiveBlend(bool on) = 0;
	virtual void SetAlphaTestEnabled(bool on) = 0;
	virtual void SetEmissive(bool on) = 0;
	virtual void SetAfterburner(bool on) = 0;
	virtual void SetCockpitPass(bool on) = 0;
	virtual void SetIRGrey(bool on) = 0;   // #DX12 A5: sensor pass -> monochrome grey (TGP/TV, Maverick/FLIR/IR)
	virtual void SetTexColorDiffuse(bool on) = 0;
	virtual void SetForcePerSample(bool on) = 0;
	virtual void SetStencil(int mode, unsigned ref) = 0;
	virtual void SetHudStencil(int mode) = 0;

	// Passes
	virtual void BeginScreenPass() = 0;
	virtual void BeginObjectPass() = 0;
	virtual void BeginTerrainPass() = 0;
	virtual void SetTerrainRasterForLod(int level) = 0;
	virtual void RebuildTerrainRasters() = 0;

	// Draws (screen / color-tri / terrain / dynamic-2D)
	virtual void DrawTL(int primType, const ScreenVertex* verts, int count) = 0;
	virtual void DrawTLIndexed(int primType, const ScreenVertex* verts, int vcount,
	                           const unsigned short* indices, int icount) = 0;
	virtual void DrawColorTrisScreen(const ScreenVertex* verts, int count,
	                                 ID3D11ShaderResourceView* tex, int opaque, int cull) = 0;
	virtual void DrawBitmap2D(int dX, int dY, int w, int h, int totalWidth, int sX, int sY,
	                          const unsigned* pSrc, bool fit, int screenW, int screenH) = 0;
	// Artscout - 2026: G-force / end-flight vignette (blackout/redout) fullscreen post-process (FF_GLOC).
	virtual void DrawGlocOverlay(float intensity, float innerR, float outerR, float tintR, float tintG, float tintB) = 0;
	virtual void DrawTerrainMesh(const void* verts, int vcount, const unsigned short* indices, int icount) = 0;
	virtual void BeginDynamic2D(bool additive) = 0;
	virtual void UploadDynamic2D(const void* dynVerts, int vcount) = 0;
	virtual void DrawDynamic2D(const void* dynVerts, int vcount, ID3D11ShaderResourceView* srv, int primType) = 0;
	virtual void DrawDynamic2DIndexed(const unsigned short* indices, int icount,
	                                  ID3D11ShaderResourceView* srv, int primType) = 0;

	// UI + textures
	virtual void CompositeUISurface(const void* src565, int w, int h) = 0;
	virtual ID3D11ShaderResourceView* LoadTextureFile(const char* path) = 0;

	// #DX12 п.4: object/BSP path (aircraft/cockpit models). vbHandle is an OPAQUE per-model vertex buffer
	// (ID3D11Buffer* under D3D11, ID3D12Resource* under D3D12 -- from the VB manager). lights is a packed
	// array of 64-byte GpuLight structs (matches cbLights). These were D3D11-only; now neutral so D3D12 draws them.
	virtual void SetLights(const float ambient[4], int numLights, const void* lights, int lightStride) = 0;
	virtual void DrawObjectIndexed(int primType, void* vbHandle, int stride, int baseVertex,
	                               const unsigned short* indices, int indexCount) = 0;
	virtual void DrawObjectStrip(int primType, void* vbHandle, int stride, int startVertex, int vertexCount) = 0;

	// Artscout - 2026: #VFX Phase 1 -- GPU-instanced billboard particles. `inst` is an array of `count`
	// per-instance records; the concrete POD is the backend's own particle-instance struct (D3D12:
	// D3D12ParticleInstance in D3D12Renderer.h) -- kept as const void* here so no backend struct leaks into
	// this neutral header. The documented layout (each record, tightly packed, little-endian) is:
	//   float center[3];  // world position                    (offset  0)
	//   float size[2];    // world width,height                (offset 12)
	//   float rot;        // billboard-plane rotation (radians)(offset 20)
	//   unsigned color;   // packed D3DCOLOR ARGB (BGRA bytes) (offset 24)
	//   float uvRect[4];  // xy=atlas uv offset, zw=uv scale   (offset 28)   -> 44 bytes/record
	// `atlasSrv` is the opaque sprite-atlas texture handle bound to gTex0. blendMode: 0 additive, 1 alpha.
	virtual void DrawParticlesInstanced(const void* inst, int count, void* atlasSrv, int blendMode) = 0;
};

// The active renderer (D3D11Renderer or, later, D3D12Renderer). Set in DXContext::Init.
// NULL under the legacy D3D7/DDraw path (and, for now, under D3D12 until its renderer lands).
extern IRenderer* g_pRenderer;

#endif // _IRENDERER_H_
