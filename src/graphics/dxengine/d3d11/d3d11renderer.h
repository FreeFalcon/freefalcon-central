//-----------------------------------------------------------------------------
// D3D11Renderer.h  -- the D3D11 replacement for the D3D7 fixed-function draw
// path. PHASE 2 of the port. Self-contained: depends only on D3D11Backend
// (device/context), FFStateMap, and the FFEmu.hlsl shaders.
//
// The legacy context.cpp funnels almost everything into a TLVERTEX (XYZRHW)
// vertex buffer (screen-space, pre-lit). This class owns that path: a dynamic
// vertex buffer, the input layout, constant buffers, the shaders, and the
// bucketed D3D11 state objects. context.cpp's DrawPrimitive() bodies become
// thin wrappers that fill a TLVERTEX array and call DrawTL() here.
//
// The object/BSP path (VS_Object, real world/view/proj + lighting) is driven
// through SetWorld/SetView/SetProj; view+proj are per-pass for VR stereo.
//-----------------------------------------------------------------------------
#ifndef _D3D11RENDERER_H_
#define _D3D11RENDERER_H_

#include <windows.h>
#include "FFStateMap.h"

struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11VertexShader;
struct ID3D11PixelShader;
struct ID3D11InputLayout;
struct ID3D11Buffer;
struct ID3D11BlendState;
struct ID3D11DepthStencilState;
struct ID3D11RasterizerState;
struct ID3D11SamplerState;
struct ID3D11ShaderResourceView;

// Mirror of the legacy TLVERTEX (context.h) for the VB layout.
struct D3D11_TLVERTEX
{
	float sx, sy, sz, rhw;     // screen pos + 1/w
	unsigned long color;       // D3DCOLOR ARGB
	unsigned long specular;    // D3DCOLOR ARGB (fog)
	float tu0, tv0;
	float tu1, tv1;
};

class D3D11Renderer
{
public:
	D3D11Renderer();
	~D3D11Renderer();

	// Compiles shaders, builds layouts / cbuffers / state objects. shaderDir is
	// where FFEmu.hlsl lives (so it can be reloaded). Returns false on failure.
	bool Init(const char* shaderDir);
	void Release();
	bool IsValid() const { return m_pVSscreen != 0; }

	// Per-frame: backbuffer size, used by the XYZRHW screen path.
	void SetViewportSize(int w, int h);
	int  ScreenW() const { return m_screenW; }	// Artscout - 2026: current gScreenSize (DIAG/RTT)
	int  ScreenH() const { return m_screenH; }

	// Per-pass transforms (object path). view/proj are per-eye in VR.
	void SetView(const float* m4x4_rowmajor);
	void SetProj(const float* m4x4_rowmajor);
	void SetWorld(const float* m4x4_rowmajor);

	// Select a legacy STATE_* (translated via FFMapState). Binds blend/depth/
	// rasterizer/sampler and updates the shader feature flags.
	void SetState(int legacyState);

	// One light for the object path (mirrors GpuLight in FFEmu.hlsl).
	struct GpuLightCPU
	{
		float Position[4];   // xyz, w
		float Direction[4];  // xyz normalized
		float Color[4];      // rgb
		float Params[4];     // x=range, y=type(0=dir,1=point)
	};
	// Upload the light set + ambient into cbLights (object-path lighting).
	void SetLights(const float ambient[4], int numLights, const GpuLightCPU* lights);

	// Render parameters that used to be D3D7 render states / engine globals.
	void SetTexture(unsigned slot, ID3D11ShaderResourceView* srv);
	void SetChromaKey(unsigned long argb, float tolerance = 0.02f);
	void SetFog(unsigned long argb, float start, float end);
	void SetAlphaRef(float ref01);
	// Artscout - 2026: gMaterialColor: global color modulator (object path). Default (1,1,1,1).
	// Radar blips use green. Reset to (1,1,1,1) after the radar.
	void SetMaterialColor(float r, float g, float b, float a);
	// Artscout - 2026: Surface specular (Blinn-Phong from light 0). power<=0 -> no highlight. Set
	// per-surface (like D3D7 TheMaterial.power/dcvSpecular).
	void SetMaterialSpecular(float r, float g, float b, float power);
	// Artscout - 2026: World camera position (for specular). Set once per frame (FlushBuffers).
	void SetCameraPos(float x, float y, float z);

	// Draw a TLVERTEX array (screen/XYZRHW path). primType is MPR_PKT_*.
	// Handles the TRIFAN case (no native D3D11 topology) via index emulation.
	void DrawTL(int primType, const D3D11_TLVERTEX* verts, int count);

	// Indexed screen-path draw. The legacy context.cpp converts multi-fan/
	// multi-linestrip batches to an index buffer (TRIANGLELIST/LINELIST).
	// primType here is the *list* type (4=TRIANGLES, 2=LINES).
	void DrawTLIndexed(int primType, const D3D11_TLVERTEX* verts, int vcount,
	                   const unsigned short* indices, int icount);

	// Artscout - 2026: #30 (ContextMPR::Render2DBitmap): a CPU bitmap (splash/cursor/mirror) as a textured quad
	// over the current RTV. pSrc is ABGR DWORD, sub-region (sX,sY,w,h) with stride totalWidth.
	void DrawBitmap2D(int dX, int dY, int w, int h, int totalWidth, int sX, int sY,
	                  const unsigned* pSrc, bool fit, int screenW, int screenH);

	// Artscout - 2026: PHASE 5: composite the 2D-UI CPU surface (RGB565) OVER the 3D scene in the RTV.
	// Black (0x0000) is treated as transparent (alpha-blend) -- 3D shows through it,
	// while 2D overlays (text/cursor/dialogs) draw on top. Called on the 3D frame instead of
	// BlitBitmap565 (which via CopyResource would overwrite the 3D).
	void CompositeUISurface(const void* src565, int w, int h);

	// Bind the screen (XYZRHW) program; call before a batch of DrawTL.
	void BeginScreenPass();
	// Bind the object (transform+lighting) program.
	void BeginObjectPass();
	// Toggle translucent object surfaces (canopy glass etc.): on => SRCALPHA blend,
	// Artscout - 2026: depth-test without write, alpha-test cutout removed (translucent pixels kept);
	// off => restore the opaque object-pass state.
	void SetObjectAlphaBlend(bool on);

	// Artscout - 2026: Per-surface alpha-test (chroma cutout). D3D7 enables alpha-test ONLY for
	// chroma surfaces; opaque surfaces (e.g. cockpit panels with alpha=0
	// in the texture) must draw without cutout. on => FF_ALPHATEST, off => clear it.
	void SetAlphaTestEnabled(bool on);

	// Artscout - 2026: #49 per-surface self-illumination (D3D7 SwEmissive: afterburner cone,
	// nav/formation lights). on => FF_EMISSIVE (object shader skips scene-light darkening so the
	// surface glows at any time of day); off => clear it. Set every DrawSurface in the object path.
	void SetEmissive(bool on);

	// Artscout - 2026: #49 afterburner cone (COMP_AB/COMP_AB2). on => FF_AFTERBURNER (shader recolors
	// the plume to a warm white-hot-core -> orange gradient). Implies emissive; set every DrawSurface.
	void SetAfterburner(bool on);

	// Artscout - 2026: #49 within the alpha pass, flip the afterburner cone to pure additive
	// (bright glowing flame) and back to alpha-blend for the surrounding translucent surfaces.
	void SetObjectAdditiveBlend(bool on);

	// Artscout - 2026: D3D7 TexColorDiffuse: text color from the vertex, the font texture is only a mask.
	void SetTexColorDiffuse(bool on);

	// Artscout - 2026: #7 display-panel SSAA: while on, screen draws (DrawTL/DrawTLIndexed) go through the
	// per-sample PS -> 4x supersampling on the MSAA target (for DrawRttQuad: stable HUD/MFD/DED text).
	void SetForcePerSample(bool on) { m_forcePerSample = on; }

	// Artscout - 2026: PHASE 5: 3D-pit mask (emulates D3D7 SetStencilMode). mode: 0=OFF, 2=WRITE
	// (writes ref into stencil where the pit draws), 3=CHECK (draws only where ref>stencil,
	// i.e. OUTSIDE the pit) -- mirrors STENCIL_OFF/WRITE/CHECK. ref is the current m_StencilRef.
	void SetStencil(int mode, unsigned ref);

	// Artscout - 2026 (VR HUD 3D glass): stencil clip for the collimated HUD. The glass plate writes a
	// dedicated bit (MARK), the symbology draws only where that bit is set (TEST) -> the HUD is clipped to
	// the combiner aperture shape. Depth is OFF in both (the screen/RTT path carries no per-vertex depth).
	// An OVERRIDE flag so SetState (re-bound by every RestoreState) keeps the stencil DSS during the draw.
	enum { HUD_STENCIL_OFF = 0, HUD_STENCIL_MARK = 1, HUD_STENCIL_TEST = 2 };
	void SetHudStencil(int mode);

	// Object path: draw from a persistent device VB owned by the VB manager
	// (CDXVbManager mirror of the legacy D3D7 VB). stride = sizeof(D3DVERTEXEX)=40.
	// primType is D3DPT_* (4=LIST, 5=STRIP, 6=FAN). Indices are 0-based into the
	// model's vertex block; baseVertex (=BaseOffset) is added by the GPU.
	void DrawObjectIndexed(int primType, struct ID3D11Buffer* vb, int stride, int baseVertex,
	                       const unsigned short* indices, int indexCount);
	// Non-indexed object draw (POINTLIST surfaces).
	void DrawObjectStrip(int primType, struct ID3D11Buffer* vb, int stride,
	                     int startVertex, int vertexCount);

	// --- Dynamic 2D-in-3D (DX2D: particles/tracers/blips) ---------------------
	// Artscout - 2026: These primitives are billboards/polygons in WORLD space (D3DDYNVERTEX:
	// pos+diffuse+specular+uv, no normal). Drawn through the object VS (world*view*
	// proj) with world=identity, no lighting, with alpha-blend and optional alpha-test (chroma).
	// BeginDynamic2D sets the state once; DrawDynamic2D draws a batch.
	// additive=true -> additive blend (tracer/spark glow, parity with D3D7 emissive);
	// false -> normal alpha-blend (chroma sprites / 2D particles).
	void BeginDynamic2D(bool additive = false);
	// Artscout - 2026: Upload a batch of D3DDYNVERTEX (28-byte stride) into the VB ONCE per flush (converted to
	// the object vertex); the VB stays bound.
	void UploadDynamic2D(const void* dynVerts, int vcount);
	// Artscout - 2026: Indexed draw over the already-uploaded buffer (one Draws2D item). primType: D3DPT_*.
	void DrawDynamic2DIndexed(const unsigned short* indices, int icount,
	                          struct ID3D11ShaderResourceView* srv, int primType);
	// Artscout - 2026: Indirect draw of a small batch (points/lines). Convert + upload + draw.
	void DrawDynamic2D(const void* dynVerts, int vcount,
	                   struct ID3D11ShaderResourceView* srv, int primType);

private:
	bool CompileShaders(const char* shaderDir);
	bool CreateStateObjects();
	bool CreateBuffers();
	void UpdateRenderCB();

	bool EnsureVB(int bytes);
	bool EnsureIB(int indices);

	ID3D11Device*        m_pDev;
	ID3D11DeviceContext* m_pCtx;

	ID3D11VertexShader*  m_pVSscreen;
	ID3D11VertexShader*  m_pVSobject;
	ID3D11PixelShader*   m_pPS;
	ID3D11PixelShader*   m_pPSPerSample;	// Artscout - 2026: #7 panel SSAA (per-sample shading); NULL -> SSAA off
	bool                 m_forcePerSample;	// Artscout - 2026: #7 enabled around DrawRttQuad
	ID3D11InputLayout*   m_pILscreen;
	ID3D11InputLayout*   m_pILobject;

	ID3D11Buffer* m_pCBViewport;
	ID3D11Buffer* m_pCBView;
	ID3D11Buffer* m_pCBObject;
	ID3D11Buffer* m_pCBRender;
	ID3D11Buffer* m_pCBLights;

	ID3D11Buffer* m_pVB;        // dynamic
	int           m_nVBBytes;
	ID3D11Buffer* m_pIB;        // dynamic (trifan emulation)
	int           m_nIBCount;
	float         m_materialColor[4];   // Artscout - 2026: gMaterialColor (1,1,1,1 default; radar blips=green)
	float         m_specular[4];        // Artscout - 2026: gSpecular (rgb=highlight color, w=power; 0=no specular)
	float         m_camPos[4];          // Artscout - 2026: world camera position (specular half-vector)

	// Bucketed state objects.
	ID3D11BlendState*        m_pBlend[3];     // FFBlendMode
	ID3D11DepthStencilState* m_pDepth[4];     // [write][test]
	ID3D11DepthStencilState* m_pDSStencilWrite; // Artscout - 2026: PHASE 5: pit -- writes ref into stencil
	ID3D11DepthStencilState* m_pDSStencilCheck; // Artscout - 2026: PHASE 5: world -- draws where ref>stencil
	ID3D11DepthStencilState* m_pDSHudMark = nullptr; // Artscout - 2026 (VR HUD glass): write the aperture bit, depth off
	ID3D11DepthStencilState* m_pDSHudTest = nullptr; // Artscout - 2026 (VR HUD glass): draw where the bit is set, depth off
	int                      m_hudStencil = 0;       // HUD_STENCIL_OFF/MARK/TEST override for SetState
	ID3D11RasterizerState*   m_pRaster;
	ID3D11RasterizerState*   m_pRasterObj;	// Artscout - 2026: #16: depth-bias -> objects over terrain (equiv. D3D7 ZBIAS)
	ID3D11SamplerState*      m_pSamp[4];      // [filter][addr]

	// Shadow copies of cbView (view+proj uploaded together) and cbRender.
	float         m_view[16];
	float         m_proj[16];
	bool          m_viewDirty;
	void          UploadView();

	unsigned int  m_flags;
	float         m_alphaRef;
	float         m_fogStart, m_fogEnd;
	unsigned long m_fogColor;
	unsigned long m_chromaKey;
	float         m_chromaTol;
	bool          m_texColorDiffuse;	// Artscout - 2026: D3D7 TexColorDiffuse: text color from the vertex (sticky)
	bool          m_hasTex0;			// Artscout - 2026: texture actually bound in slot 0 (else clear FF_TEXTURE0)
	bool          m_renderCBDirty;
	int           m_screenW = 0, m_screenH = 0;	// Artscout - 2026: current gScreenSize (DIAG/RTT)

	// Artscout - 2026: PHASE 5: texture for compositing the 2D-UI over the 3D (in-sim overlays).
	struct ID3D11Texture2D*          m_pUITex;
	struct ID3D11ShaderResourceView* m_pUISRV;
	int                              m_uiW, m_uiH;
};

extern D3D11Renderer* g_pD3D11Renderer;

#endif // _D3D11RENDERER_H_
