//-----------------------------------------------------------------------------
// D3D11Renderer.cpp  -- see D3D11Renderer.h. PHASE 2 of the D3D7->D3D11 port.
//-----------------------------------------------------------------------------
#include "stdafx.h"
#include "D3D11Renderer.h"
#include "Graphics\DXEngine\D3D11Backend.h"

#include <d3d11.h>
#include <d3dcompiler.h>
#include <string>

#pragma comment(lib, "d3dcompiler.lib")

extern "C" void MonoPrint(char *fmt, ...);

D3D11Renderer* g_pD3D11Renderer = NULL;

// PHASE 4: set true by any GPU draw in the frame. Present skips the CPU blit (BlitBitmap565)
// when the frame was GPU-drawn (3D scene), otherwise it blits the 2D UI (menus).
bool g_bD3D11GPUDraw = false;

//================================ cbuffer mirrors =============================
// Layouts must match FFEmu.hlsl exactly (16-byte aligned).

struct CBViewport { float screenW, screenH, pad0, pad1; };
struct CBView     { float view[16]; float proj[16]; float camPos[4]; };
struct CBObject   { float world[16]; };
struct CBRender
{
	unsigned int flags;
	float        alphaRef;
	float        fogStart;
	float        fogEnd;
	float        fogColor[4];
	float        chromaKey[4];
	float        materialColor[4];
	float        specular[4];   // rgb=specular color, w=power
	float        waterParams[4]; // #12: x=time(sec), rest reserved
};

static inline void ArgbToRgba(unsigned long argb, float* out, float tol = -1.0f)
{
	out[0] = ((argb >> 16) & 0xFF) / 255.0f;
	out[1] = ((argb >>  8) & 0xFF) / 255.0f;
	out[2] = ((argb      ) & 0xFF) / 255.0f;
	out[3] = (tol >= 0.0f) ? tol : (((argb >> 24) & 0xFF) / 255.0f);
}

static inline void SafeRel(IUnknown* p) { if (p) p->Release(); }

//================================ ctor/dtor ==================================

D3D11Renderer::D3D11Renderer()
{
	ZeroMemory(this, sizeof(*this));
	m_alphaRef   = 0.5f;
	m_fogStart   = 0.0f;
	m_fogEnd     = 1.0e9f;	// #29: default = "no fog" (if SetFog isn't called in the frame,
	                        // FF_FOG won't hide objects; the real haze is set by otw.cpp SetFog).
	m_fogColor   = 0xFF808080;
	m_chromaKey  = 0xFF000000;
	m_chromaTol  = 0.02f;
	m_materialColor[0] = m_materialColor[1] = m_materialColor[2] = m_materialColor[3] = 1.0f;
	m_specular[0] = m_specular[1] = m_specular[2] = m_specular[3] = 0.0f;	// no specular
	m_camPos[0] = m_camPos[1] = m_camPos[2] = m_camPos[3] = 0.0f;
	m_texColorDiffuse = false;
	m_hasTex0 = false;
	m_renderCBDirty = true;
}

D3D11Renderer::~D3D11Renderer() { Release(); }

//================================ Init =======================================

bool D3D11Renderer::Init(const char* shaderDir)
{
	if (!g_pD3D11Backend || !g_pD3D11Backend->IsValid())
	{
		MonoPrint("D3D11Renderer::Init - backend not ready\n");
		return false;
	}
	m_pDev = g_pD3D11Backend->GetDevice();
	m_pCtx = g_pD3D11Backend->GetContext();

	if (!CompileShaders(shaderDir)) return false;
	if (!CreateStateObjects())      return false;
	if (!CreateBuffers())           return false;

	SetViewportSize(g_pD3D11Backend->Width(), g_pD3D11Backend->Height());
	MonoPrint("D3D11Renderer::Init succeeded\n");
	return true;
}

static ID3DBlob* CompileOne(const wchar_t* path, const char* entry, const char* target)
{
	ID3DBlob* code = NULL;
	ID3DBlob* err  = NULL;
	UINT flags = D3DCOMPILE_OPTIMIZATION_LEVEL3;
#ifdef _DEBUG
	flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
	HRESULT hr = D3DCompileFromFile(path, NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE,
	                                entry, target, flags, 0, &code, &err);
	if (FAILED(hr))
	{
		MonoPrint("D3D11Renderer: shader %s/%s compile failed: %s\n",
		          entry, target, err ? (const char*)err->GetBufferPointer() : "?");
		SafeRel(err);
		return NULL;
	}
	SafeRel(err);
	return code;
}

bool D3D11Renderer::CompileShaders(const char* shaderDir)
{
	std::wstring dir;
	{
		std::string s = shaderDir ? shaderDir : ".";
		dir.assign(s.begin(), s.end());
		if (!dir.empty() && dir.back() != L'\\' && dir.back() != L'/') dir += L'\\';
	}
	std::wstring path = dir + L"FFEmu.hlsl";

	ID3DBlob* vsScreen = CompileOne(path.c_str(), "VS_Screen", "vs_5_0");
	ID3DBlob* vsObject = CompileOne(path.c_str(), "VS_Object", "vs_5_0");
	ID3DBlob* ps       = CompileOne(path.c_str(), "PS_Main",   "ps_5_0");
	if (!vsScreen || !vsObject || !ps) { SafeRel(vsScreen); SafeRel(vsObject); SafeRel(ps); return false; }

	m_pDev->CreateVertexShader(vsScreen->GetBufferPointer(), vsScreen->GetBufferSize(), NULL, &m_pVSscreen);
	m_pDev->CreateVertexShader(vsObject->GetBufferPointer(), vsObject->GetBufferSize(), NULL, &m_pVSobject);
	m_pDev->CreatePixelShader (ps->GetBufferPointer(),       ps->GetBufferSize(),       NULL, &m_pPS);

	// #7 panel SSAA: per-sample PS variant (optional - if it fails to compile we run without SSAA).
	ID3DBlob* psPS = CompileOne(path.c_str(), "PS_PerSample", "ps_5_0");
	if (psPS)
	{
		m_pDev->CreatePixelShader(psPS->GetBufferPointer(), psPS->GetBufferSize(), NULL, &m_pPSPerSample);
		SafeRel(psPS);
	}

	// Input layout for TLVERTEX (XYZRHW screen path).
	const D3D11_INPUT_ELEMENT_DESC screenIL[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_B8G8R8A8_UNORM,     0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    1, DXGI_FORMAT_B8G8R8A8_UNORM,     0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT,       0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	m_pDev->CreateInputLayout(screenIL, _countof(screenIL),
		vsScreen->GetBufferPointer(), vsScreen->GetBufferSize(), &m_pILscreen);

	// Input layout for the object path -- must match D3DVERTEXEX exactly
	// (DXVbManager.h): vx,vy,vz, nx,ny,nz, dwColour, dwSpecular, tu,tv = 40 bytes.
	// COLOR0=dwColour (diffuse), COLOR1=dwSpecular (emissive, D3D7 D3DMCS_COLOR2).
	const D3D11_INPUT_ELEMENT_DESC objectIL[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_B8G8R8A8_UNORM,  0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    1, DXGI_FORMAT_B8G8R8A8_UNORM,  0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	m_pDev->CreateInputLayout(objectIL, _countof(objectIL),
		vsObject->GetBufferPointer(), vsObject->GetBufferSize(), &m_pILobject);

	SafeRel(vsScreen); SafeRel(vsObject); SafeRel(ps);
	return m_pVSscreen && m_pVSobject && m_pPS && m_pILscreen && m_pILobject;
}

bool D3D11Renderer::CreateStateObjects()
{
	// ---- blend states ----
	for (int b = 0; b < 3; ++b)
	{
		D3D11_BLEND_DESC bd; ZeroMemory(&bd, sizeof(bd));
		D3D11_RENDER_TARGET_BLEND_DESC& rt = bd.RenderTarget[0];
		rt.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		if (b == BLEND_OPAQUE) {
			rt.BlendEnable = FALSE;
		} else {
			rt.BlendEnable    = TRUE;
			// Artscout - 2026: BLEND_ADDITIVE (tracers/sparks only) is PURE additive (Src=ONE):
			// the emissive color is added regardless of vertex alpha, matching the D3D7 emissive
			// (D3DMCS_COLOR2) look. With Src=SRC_ALPHA the glow scaled by alpha (often <1) and came
			// out faint - and the optimized Release shader made it fainter still vs Debug. ONE makes
			// tracer brightness alpha-independent and identical across builds.
			rt.SrcBlend       = (b == BLEND_ADDITIVE) ? D3D11_BLEND_ONE : D3D11_BLEND_SRC_ALPHA;
			rt.DestBlend      = (b == BLEND_ADDITIVE) ? D3D11_BLEND_ONE : D3D11_BLEND_INV_SRC_ALPHA;
			rt.BlendOp        = D3D11_BLEND_OP_ADD;
			rt.SrcBlendAlpha  = D3D11_BLEND_ONE;
			rt.DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
			rt.BlendOpAlpha   = D3D11_BLEND_OP_ADD;
		}
		m_pDev->CreateBlendState(&bd, &m_pBlend[b]);
	}

	// ---- depth-stencil states [write][test] ----
	for (int w = 0; w < 2; ++w)
	for (int t = 0; t < 2; ++t)
	{
		D3D11_DEPTH_STENCIL_DESC dd; ZeroMemory(&dd, sizeof(dd));
		dd.DepthEnable    = t ? TRUE : FALSE;
		dd.DepthWriteMask = w ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
		dd.DepthFunc      = D3D11_COMPARISON_LESS_EQUAL;
		m_pDev->CreateDepthStencilState(&dd, &m_pDepth[(w << 1) | t]);
	}

	// ---- PHASE 5: stencil states for the 3D cockpit mask (depth test+write ON) ----
	{
		D3D11_DEPTH_STENCIL_DESC sd; ZeroMemory(&sd, sizeof(sd));
		sd.DepthEnable    = TRUE;
		sd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		sd.DepthFunc      = D3D11_COMPARISON_LESS_EQUAL;
		sd.StencilEnable  = TRUE;
		sd.StencilReadMask  = 0xFF;
		sd.StencilWriteMask = 0xFF;
		// WRITE: the pit writes ref (FUNC=ALWAYS, PASS=REPLACE)
		sd.FrontFace.StencilFunc        = D3D11_COMPARISON_ALWAYS;
		sd.FrontFace.StencilPassOp      = D3D11_STENCIL_OP_REPLACE;
		sd.FrontFace.StencilFailOp      = D3D11_STENCIL_OP_KEEP;
		sd.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
		sd.BackFace = sd.FrontFace;
		m_pDev->CreateDepthStencilState(&sd, &m_pDSStencilWrite);

		// CHECK: the world draws where ref > stencil (FUNC=GREATER, PASS=KEEP, write off)
		sd.StencilWriteMask = 0x00;
		sd.FrontFace.StencilFunc   = D3D11_COMPARISON_GREATER;
		sd.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		sd.BackFace = sd.FrontFace;
		m_pDev->CreateDepthStencilState(&sd, &m_pDSStencilCheck);
	}

	// ---- Artscout - 2026 (VR HUD 3D glass): stencil clip for the collimated HUD ----
	// MARK: the glass plate writes the aperture bit (0x40); TEST: the symbology draws only where that bit
	// is set. DEPTH OFF in both (the RTT/screen path carries no per-vertex depth -> a depth test there is
	// meaningless). Dedicated bit 0x40 + masks so the cockpit-mask stencil bits are untouched.
	{
		D3D11_DEPTH_STENCIL_DESC hd; ZeroMemory(&hd, sizeof(hd));
		hd.DepthEnable    = FALSE;
		hd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
		hd.DepthFunc      = D3D11_COMPARISON_ALWAYS;
		hd.StencilEnable  = TRUE;
		hd.StencilReadMask  = 0x40;
		hd.StencilWriteMask = 0x40;
		hd.FrontFace.StencilFunc        = D3D11_COMPARISON_ALWAYS;   // MARK: always pass, replace bit
		hd.FrontFace.StencilPassOp      = D3D11_STENCIL_OP_REPLACE;
		hd.FrontFace.StencilFailOp      = D3D11_STENCIL_OP_KEEP;
		hd.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
		hd.BackFace = hd.FrontFace;
		m_pDev->CreateDepthStencilState(&hd, &m_pDSHudMark);

		hd.StencilWriteMask = 0x00;                                 // TEST: don't modify stencil
		hd.FrontFace.StencilFunc   = D3D11_COMPARISON_EQUAL;        // draw where (ref & 0x40) == (stencil & 0x40)
		hd.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		hd.BackFace = hd.FrontFace;
		m_pDev->CreateDepthStencilState(&hd, &m_pDSHudTest);
	}

	// ---- rasterizer (no backface cull -- legacy engine winds inconsistently) ----
	{
		D3D11_RASTERIZER_DESC rd; ZeroMemory(&rd, sizeof(rd));
		rd.FillMode = D3D11_FILL_SOLID;
		rd.CullMode = D3D11_CULL_NONE;
		rd.DepthClipEnable = TRUE;
		rd.MultisampleEnable = TRUE;  // #7 MSAA: edge AA of geometry on the MSAA target (ignored on a 1-sample RT)
		m_pDev->CreateRasterizerState(&rd, &m_pRaster);

		// #16/#48: object pass keeps a small negative depth-bias so ground objects that sit
		// coplanar with the terrain (buildings/vehicles) lift just above it and do not
		// z-fight. The old value (-8000) was effectively INERT while the world pass cleared
		// the depth buffer before the object flush (#48) -- nothing to bias against. Now that
		// terrain depth is preserved, the bias is LIVE. DepthBias is in depth-buffer ULPs
		// (1/2^24 for D24_UNORM), and because NearZ=0.2/ZFAR=280000 compress depth near 1.0,
		// a constant bias maps to a world-space lift that grows ~z^2; -8000 would shove
		// distant objects clean through hills. Keep the constant tiny and rely mostly on the
		// slope-scaled term (targets coplanar terrain-aligned polygons). Tuning knob for #48:
		// if ground objects z-fight -> raise magnitude; if distant objects show through
		// terrain -> lower it. (Real fix is reversed-Z float depth -- deferred.)
		rd.DepthBias            = -16;
		rd.SlopeScaledDepthBias = -2.0f;
		rd.DepthBiasClamp       = 0.0f;
		m_pDev->CreateRasterizerState(&rd, &m_pRasterObj);
	}

	// ---- samplers [filter][addr] ----
	for (int f = 0; f < 2; ++f)
	for (int a = 0; a < 2; ++a)
	{
		D3D11_SAMPLER_DESC sd; ZeroMemory(&sd, sizeof(sd));
		// #7 SHARPNESS: ANISOTROPIC 16x ONLY for linear-WRAP (world/terrain - tiled textures at a
		// grazing angle, aniso is sharper than bilinear). Keep linear-CLAMP as plain LINEAR: it's
		// used by the RTT display composite, where aniso widened a line's gradient edge (from the
		// SSAA downscale) along the angle -> bright halo = "3D lines". Display sharpness already comes
		// from SSAA=3. Leave POINT alone (1:1 glyph blit into the atlas).
		if (f)
		{
			sd.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
		}
		else if (a == 0) // linear-wrap = world/terrain
		{
			sd.Filter = D3D11_FILTER_ANISOTROPIC;
			sd.MaxAnisotropy = 16;
		}
		else // linear-clamp = display composite (no halo)
		{
			sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		}
		D3D11_TEXTURE_ADDRESS_MODE am = a ? D3D11_TEXTURE_ADDRESS_CLAMP : D3D11_TEXTURE_ADDRESS_WRAP;
		sd.AddressU = sd.AddressV = sd.AddressW = am;
		sd.MaxLOD = D3D11_FLOAT32_MAX;
		m_pDev->CreateSamplerState(&sd, &m_pSamp[(f << 1) | a]);
	}
	return true;
}

template <class T>
static ID3D11Buffer* MakeCB(ID3D11Device* dev)
{
	D3D11_BUFFER_DESC bd; ZeroMemory(&bd, sizeof(bd));
	bd.ByteWidth      = (sizeof(T) + 15) & ~15;
	bd.Usage          = D3D11_USAGE_DEFAULT;
	bd.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
	ID3D11Buffer* b = NULL;
	dev->CreateBuffer(&bd, NULL, &b);
	return b;
}

bool D3D11Renderer::CreateBuffers()
{
	m_pCBViewport = MakeCB<CBViewport>(m_pDev);
	m_pCBView     = MakeCB<CBView>(m_pDev);
	m_pCBObject   = MakeCB<CBObject>(m_pDev);
	m_pCBRender   = MakeCB<CBRender>(m_pDev);
	// cbLights: ambient(16) + numLights(16) + 8*(64) = 544 bytes.
	{
		D3D11_BUFFER_DESC bd; ZeroMemory(&bd, sizeof(bd));
		bd.ByteWidth = 16 + 16 + 8 * 64;
		bd.Usage     = D3D11_USAGE_DEFAULT;
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		m_pDev->CreateBuffer(&bd, NULL, &m_pCBLights);
	}
	return m_pCBViewport && m_pCBView && m_pCBObject && m_pCBRender && m_pCBLights;
}

//================================ dynamic VB/IB ==============================

bool D3D11Renderer::EnsureVB(int bytes)
{
	if (m_pVB && m_nVBBytes >= bytes) return true;
	if (bytes <= 0 || bytes > (1 << 28)) return false; // guard: bad/huge size -> bail, don't spin
	SafeRel(m_pVB); m_pVB = NULL;
	int want = 1; while (want < bytes && want > 0) want <<= 1;
	if (want <= 0) return false;                        // overflow guard (int wrap -> hang)
	D3D11_BUFFER_DESC bd; ZeroMemory(&bd, sizeof(bd));
	bd.ByteWidth      = want;
	bd.Usage          = D3D11_USAGE_DYNAMIC;
	bd.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	if (FAILED(m_pDev->CreateBuffer(&bd, NULL, &m_pVB))) return false;
	m_nVBBytes = want;
	return true;
}

bool D3D11Renderer::EnsureIB(int indices)
{
	if (m_pIB && m_nIBCount >= indices) return true;
	if (indices <= 0 || indices > (1 << 27)) return false; // guard: bad/huge count -> bail
	SafeRel(m_pIB); m_pIB = NULL;
	int want = 64; while (want < indices && want > 0) want <<= 1;
	if (want <= 0) return false;                           // overflow guard
	D3D11_BUFFER_DESC bd; ZeroMemory(&bd, sizeof(bd));
	bd.ByteWidth      = want * sizeof(unsigned short);
	bd.Usage          = D3D11_USAGE_DYNAMIC;
	bd.BindFlags      = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	if (FAILED(m_pDev->CreateBuffer(&bd, NULL, &m_pIB))) return false;
	m_nIBCount = want;
	return true;
}

//================================ setters ====================================

void D3D11Renderer::SetViewportSize(int w, int h)
{
	if (!m_pCtx) return;
	m_screenW = w; m_screenH = h;	// keep the current gScreenSize (used by RTT)
	CBViewport cb = { (float)w, (float)h, 0, 0 };
	m_pCtx->UpdateSubresource(m_pCBViewport, 0, NULL, &cb, 0, 0);
}

// cbView = { float view[16]; float proj[16]; } -- upload both together.
void D3D11Renderer::UploadView()
{
	if (!m_pCtx || !m_viewDirty) return;
	CBView cb;
	memcpy(cb.view, m_view, sizeof(cb.view));
	memcpy(cb.proj, m_proj, sizeof(cb.proj));
	cb.camPos[0] = m_camPos[0]; cb.camPos[1] = m_camPos[1];
	cb.camPos[2] = m_camPos[2]; cb.camPos[3] = 1.0f;
	m_pCtx->UpdateSubresource(m_pCBView, 0, NULL, &cb, 0, 0);
	m_viewDirty = false;
}
void D3D11Renderer::SetView(const float* m)  { memcpy(m_view, m, sizeof(m_view)); m_viewDirty = true; UploadView(); }
void D3D11Renderer::SetProj(const float* m)  { memcpy(m_proj, m, sizeof(m_proj)); m_viewDirty = true; UploadView(); }
void D3D11Renderer::SetWorld(const float* m) { if (m_pCtx) m_pCtx->UpdateSubresource(m_pCBObject, 0, NULL, m, 0, 0); }

void D3D11Renderer::SetChromaKey(unsigned long argb, float tol) { m_chromaKey = argb; m_chromaTol = tol; m_renderCBDirty = true; }
void D3D11Renderer::SetFog(unsigned long argb, float s, float e) { m_fogColor = argb; m_fogStart = s; m_fogEnd = e; m_renderCBDirty = true; }
void D3D11Renderer::SetAlphaRef(float r) { m_alphaRef = r; m_renderCBDirty = true; }
void D3D11Renderer::SetMaterialColor(float r, float g, float b, float a)
{ m_materialColor[0]=r; m_materialColor[1]=g; m_materialColor[2]=b; m_materialColor[3]=a; m_renderCBDirty = true; }
void D3D11Renderer::SetMaterialSpecular(float r, float g, float b, float power)
{ m_specular[0]=r; m_specular[1]=g; m_specular[2]=b; m_specular[3]=power; m_renderCBDirty = true; }
void D3D11Renderer::SetCameraPos(float x, float y, float z)
{ m_camPos[0]=x; m_camPos[1]=y; m_camPos[2]=z; m_viewDirty = true; UploadView(); }

void D3D11Renderer::SetTexture(unsigned slot, ID3D11ShaderResourceView* srv)
{
	// PHASE 3: stage0 drives the FF_TEXTURE0 flag (1<<0): SRV present -> sample it,
	// absent -> draw by vertex color (untextured surfaces aren't black).
	if (slot == 0)
	{
		unsigned nf = srv ? (m_flags | 1u) : (m_flags & ~1u);
		if (nf != m_flags) { m_flags = nf; m_renderCBDirty = true; }
		m_hasTex0 = (srv != 0);   // whether a texture is actually bound to slot 0 (sticky for SetState)
	}
	if (m_pCtx) m_pCtx->PSSetShaderResources(slot, 1, &srv);
}

// cbLights = { float4 ambient; uint num; float3 pad; GpuLight[8]; } -> 544 bytes.
void D3D11Renderer::SetLights(const float ambient[4], int numLights, const GpuLightCPU* lights)
{
	if (!m_pCtx) return;
	const int MAXL = 8;
	if (numLights < 0) numLights = 0;
	if (numLights > MAXL) numLights = MAXL;

	unsigned char buf[16 + 16 + MAXL * 64];
	ZeroMemory(buf, sizeof(buf));
	memcpy(buf, ambient, 16);
	*(unsigned int*)(buf + 16) = (unsigned int)numLights;
	if (numLights > 0)
		memcpy(buf + 32, lights, numLights * 64);   // GpuLightCPU == 64 bytes

	m_pCtx->UpdateSubresource(m_pCBLights, 0, NULL, buf, 0, 0);
}

void D3D11Renderer::UpdateRenderCB()
{
	if (!m_renderCBDirty || !m_pCtx) return;
	CBRender cb; ZeroMemory(&cb, sizeof(cb));
	cb.flags    = m_flags;
	cb.alphaRef = m_alphaRef;
	cb.fogStart = m_fogStart;
	cb.fogEnd   = m_fogEnd;
	ArgbToRgba(m_fogColor, cb.fogColor);
	ArgbToRgba(m_chromaKey, cb.chromaKey, m_chromaTol);
	cb.materialColor[0] = m_materialColor[0]; cb.materialColor[1] = m_materialColor[1];
	cb.materialColor[2] = m_materialColor[2]; cb.materialColor[3] = m_materialColor[3];
	cb.specular[0] = m_specular[0]; cb.specular[1] = m_specular[1];
	cb.specular[2] = m_specular[2]; cb.specular[3] = m_specular[3];
	// #12: animated time for the water shader (seconds, wraps ~every 1000s to keep precision).
	cb.waterParams[0] = (float)(GetTickCount() % 1000000) * 0.001f;
	m_pCtx->UpdateSubresource(m_pCBRender, 0, NULL, &cb, 0, 0);
	m_renderCBDirty = false;
}

void D3D11Renderer::SetState(int legacyState)
{
	FFStateDesc d;
	FFMapState(legacyState, d);   // false -> SOLID fallback already filled

	// TexColorDiffuse (HUD/DED text) - sticky: SetState is called on EVERY FlushVB and would
	// overwrite the bit. Apply it once (on the glyph-batch flush) and reset, so text color comes
	// from the vertex (not colored glyph x color = black).
	unsigned nf = d.flags;
	// Artscout - 2026: FF_TEXCOLORDIFFUSE is sticky (set by TexColorDiffuse() for the next glyph batch).
	// Apply it ONLY to the dedicated text states. Otherwise it leaks onto the next textured draw that
	// happens to flush while it is set -- notably the GM radar GROUND composite (STATE_TEXTURE): with
	// FF_TEXCOLORDIFFUSE the shader skips `c *= t0` and takes color from the (white) vertex, so the green
	// ground texture is ignored -> the panel fills WHITE. Consume the flag regardless so it can never
	// carry past one SetState.
	if (m_texColorDiffuse)
	{
		if (legacyState == STATE_TEXTURE_TEXT || legacyState == STATE_CHROMA_TEXTURE_GOURAUD2)
			nf |= (1u << 8);
		m_texColorDiffuse = false;
	}
	// No real texture in slot 0 -> clear FF_TEXTURE0. Otherwise texture+chroma states (GOURAUD2:
	// HUD lines via ForceAlpha) sample an empty gTex0=(0,0,0,0), chroma cuts out black -> ALL HUD
	// symbology vanishes. D3D7: an unbound stage = white. Here = vertex color.
	if (!m_hasTex0) nf &= ~1u;

	if (nf != m_flags) { m_flags = nf; m_renderCBDirty = true; }

	const float blendFactor[4] = { 0, 0, 0, 0 };
	m_pCtx->OMSetBlendState(m_pBlend[d.blend], blendFactor, 0xFFFFFFFF);
	// Artscout - 2026 (VR HUD glass): keep the HUD stencil DSS through RestoreState while the clip is armed
	// (else this would clobber it with the plain depth state). 0x40 = the aperture bit (see SetHudStencil).
	if (m_hudStencil == HUD_STENCIL_MARK)
		m_pCtx->OMSetDepthStencilState(m_pDSHudMark, 0x40);
	else if (m_hudStencil == HUD_STENCIL_TEST)
		m_pCtx->OMSetDepthStencilState(m_pDSHudTest, 0x40);
	else
		m_pCtx->OMSetDepthStencilState(m_pDepth[((d.depthWrite ? 1 : 0) << 1) | (d.depthTest ? 1 : 0)], 0);

	ID3D11SamplerState* samp = m_pSamp[((d.filter == FILTER_POINT ? 1 : 0) << 1) | (d.addr == ADDR_CLAMP ? 1 : 0)];
	ID3D11SamplerState* samps[2] = { samp, samp };
	m_pCtx->PSSetSamplers(0, 2, samps);
}

//================================ passes / draw ==============================

void D3D11Renderer::BeginScreenPass()
{
	if (!m_pCtx) return;
	m_pCtx->VSSetShader(m_pVSscreen, NULL, 0);
	m_pCtx->PSSetShader(m_pPS, NULL, 0);
	m_pCtx->IASetInputLayout(m_pILscreen);
	m_pCtx->RSSetState(m_pRaster);
	ID3D11Buffer* vscb[] = { m_pCBViewport, m_pCBView };
	m_pCtx->VSSetConstantBuffers(0, 2, vscb);
	ID3D11Buffer* pscb[] = { m_pCBRender, m_pCBLights };
	m_pCtx->PSSetConstantBuffers(3, 2, pscb);
}

void D3D11Renderer::BeginObjectPass()
{
	if (!m_pCtx) return;
	m_pCtx->VSSetShader(m_pVSobject, NULL, 0);
	m_pCtx->PSSetShader(m_pPS, NULL, 0);
	m_pCtx->IASetInputLayout(m_pILobject);
	m_pCtx->RSSetState(m_pRasterObj ? m_pRasterObj : m_pRaster);	// #16: depth-bias -> on top of terrain
	// cbView=b1, cbObject=b2; cbLights belongs to b4 (b3=cbRender sits between them).
	// cbLights used to be bound to b3 by mistake -> VS lighting read garbage.
	ID3D11Buffer* vscb[] = { m_pCBView, m_pCBObject };
	m_pCtx->VSSetConstantBuffers(1, 2, vscb);
	// b3=cbRender in VS: VS_Object reads gFlags (FF_LIGHTING!) and gSpecular/gCameraPos. b3 used
	// NOT to be bound in the VS -> gFlags=0 -> FF_LIGHTING always FALSE -> object lighting in the
	// VS was DISABLED (flat vertex color, no sun/specular). Objects are now lit.
	m_pCtx->VSSetConstantBuffers(3, 1, &m_pCBRender);
	m_pCtx->VSSetConstantBuffers(4, 1, &m_pCBLights);
	ID3D11Buffer* pscb[] = { m_pCBRender, m_pCBLights };
	m_pCtx->PSSetConstantBuffers(3, 2, pscb);

	// Default 3D state: depth test+write on, opaque blend, linear-wrap sampler.
	const float bf[4] = { 0, 0, 0, 0 };
	m_pCtx->OMSetBlendState(m_pBlend[BLEND_OPAQUE], bf, 0xFFFFFFFF);
	m_pCtx->OMSetDepthStencilState(m_pDepth[(1 << 1) | 1], 0);	// [write][test]
	ID3D11SamplerState* samps[2] = { m_pSamp[0], m_pSamp[0] };	// linear-wrap
	m_pCtx->PSSetSamplers(0, 2, samps);

	// PHASE 4 bring-up: vertex color + per-vertex lighting + alpha-test cutout.
	// FF_VERTEXCOLOR=1<<2, FF_LIGHTING=1<<3, FF_ALPHATEST=1<<5.
	// Alpha-test (threshold 0.5) gives the chroma cutout: the texture loader bakes alpha=0 for
	// chroma pixels, so they are discarded. Opaque pixels (a=1) pass.
	// #29: object FF_FOG NOT enabled yet. Per FF7: object fog there is PER-OBJECT LOD fade
	// (m_FogLevel per GetDrawItem, FOGEND from LODRange - reference dxengine.cpp:1444/1779), NOT a
	// global haze. A global FF_FOG = regression. Do it per-object (SetFog with m_FogLevel before
	// each object) once objects (#16) are live, against the reference. The distance axis in
	// VS_Object is already fixed to clip.w (correct groundwork).
	m_flags = (1u << 2) | (1u << 3) | (1u << 5);
	// PHASE 5: alpha-test threshold is LOW - discard only chroma (baked alpha=0), don't cut
	// cockpit textures with partial alpha (specular-in-alpha etc. -> used to be "black").
	m_alphaRef = 0.06f;
	m_renderCBDirty = true;
}

void D3D11Renderer::SetStencil(int mode, unsigned ref)
{
	if (!m_pCtx) return;
	// mode: STENCIL_OFF=0, STENCIL_WRITE=2, STENCIL_CHECK=3 (see CDXEngine)
	switch (mode)
	{
	case 2:	// WRITE - the pit writes ref
		m_pCtx->OMSetDepthStencilState(m_pDSStencilWrite, ref & 0xFF);
		break;
	case 3:	// CHECK - the world draws OUTSIDE the pit (ref > stencil)
		m_pCtx->OMSetDepthStencilState(m_pDSStencilCheck, ref & 0xFF);
		break;
	default:	// OFF - normal depth test+write without stencil
		m_pCtx->OMSetDepthStencilState(m_pDepth[(1 << 1) | 1], 0);
		break;
	}
}

// Artscout - 2026 (VR HUD 3D glass): arm the HUD aperture stencil clip. MARK (glass plate writes bit
// 0x40), TEST (symbology draws only where the bit is set), OFF (restore depth-off composite). Sets the
// override flag (so SetState keeps it through RestoreState) AND binds immediately (in case no SetState
// runs before the draw). Depth stays off -- the RTT/screen path has no real per-vertex depth.
void D3D11Renderer::SetHudStencil(int mode)
{
	m_hudStencil = mode;
	if (!m_pCtx) return;
	if (mode == HUD_STENCIL_MARK)
		m_pCtx->OMSetDepthStencilState(m_pDSHudMark, 0x40);
	else if (mode == HUD_STENCIL_TEST)
		m_pCtx->OMSetDepthStencilState(m_pDSHudTest, 0x40);
	else
		m_pCtx->OMSetDepthStencilState(m_pDepth[0], 0);   // OFF: no depth test/write (RTT composite default)
}

void D3D11Renderer::SetAlphaTestEnabled(bool on)
{
	unsigned nf = on ? (m_flags | (1u << 5)) : (m_flags & ~(1u << 5));
	if (nf != m_flags) { m_flags = nf; m_renderCBDirty = true; }
}

// Artscout - 2026: #49 FF_EMISSIVE (bit 11) -- self-illuminated surface (afterburner cone, lights).
void D3D11Renderer::SetEmissive(bool on)
{
	unsigned nf = on ? (m_flags | (1u << 11)) : (m_flags & ~(1u << 11));
	if (nf != m_flags) { m_flags = nf; m_renderCBDirty = true; }
}

// Artscout - 2026: #49 FF_AFTERBURNER (bit 12) -- warm flame recolor for the afterburner cone.
void D3D11Renderer::SetAfterburner(bool on)
{
	unsigned nf = on ? (m_flags | (1u << 12)) : (m_flags & ~(1u << 12));
	if (nf != m_flags) { m_flags = nf; m_renderCBDirty = true; }
}

// D3D7 TexColorDiffuse (HUD/DED text): color from the vertex, the font texture is just a mask.
// Sets a sticky flag; actually applied in SetState on the next FlushVB (glyph batch), then
// reset. Can't write m_flags directly - SetState on the flush would overwrite it.
void D3D11Renderer::SetTexColorDiffuse(bool on)
{
	m_texColorDiffuse = on;
}

void D3D11Renderer::SetObjectAlphaBlend(bool on)
{
	if (!m_pCtx) return;
	const float bf[4] = { 0, 0, 0, 0 };
	if (on)
	{
		m_pCtx->OMSetBlendState(m_pBlend[BLEND_ALPHA], bf, 0xFFFFFFFF);
		m_pCtx->OMSetDepthStencilState(m_pDepth[(0 << 1) | 1], 0);	// test, no write
		unsigned nf = m_flags & ~(1u << 5);	// clear FF_ALPHATEST -> translucency is kept
		if (nf != m_flags) { m_flags = nf; m_renderCBDirty = true; }
	}
	else
	{
		m_pCtx->OMSetBlendState(m_pBlend[BLEND_OPAQUE], bf, 0xFFFFFFFF);
		m_pCtx->OMSetDepthStencilState(m_pDepth[(1 << 1) | 1], 0);	// test+write
		unsigned nf = m_flags | (1u << 5);	// restore FF_ALPHATEST (chroma cutout)
		if (nf != m_flags) { m_flags = nf; m_renderCBDirty = true; }
	}
}

// Artscout - 2026: #49 switch the afterburner cone to PURE ADDITIVE within the alpha pass, then
// back to alpha-blend. The stock cone is alpha-blended (translucent -> dull, see-through). Additive
// (Src=ONE,Dst=ONE) makes it ADD light: overlapping layers + bright color saturate the core to
// white -> it reads as a bright, opaque-looking glowing flame (reference real_af.png). Depth: test,
// no write (same as the alpha pass) so the plume doesn't z-fight / occlude wrongly.
void D3D11Renderer::SetObjectAdditiveBlend(bool on)
{
	if (!m_pCtx) return;
	const float bf[4] = { 0, 0, 0, 0 };
	m_pCtx->OMSetBlendState(m_pBlend[on ? BLEND_ADDITIVE : BLEND_ALPHA], bf, 0xFFFFFFFF);
	m_pCtx->OMSetDepthStencilState(m_pDepth[(0 << 1) | 1], 0);   // test, no write
}

// PHASE 5: composite the CPU 2D UI (RGB565) over the 3D scene into the RTV. Black -> transparent.
void D3D11Renderer::CompositeUISurface(const void* src565, int w, int h)
{
	if (!m_pCtx || !m_pDev || !src565 || w <= 0 || h <= 0) return;

	// (re)create a DYNAMIC texture sized to the UI surface
	if (!m_pUITex || m_uiW != w || m_uiH != h)
	{
		SafeRel(m_pUISRV); m_pUISRV = NULL;
		SafeRel(m_pUITex); m_pUITex = NULL;
		D3D11_TEXTURE2D_DESC td; ZeroMemory(&td, sizeof(td));
		td.Width = w; td.Height = h; td.MipLevels = 1; td.ArraySize = 1;
		td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		td.SampleDesc.Count = 1;
		td.Usage = D3D11_USAGE_DYNAMIC;
		td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		td.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		if (FAILED(m_pDev->CreateTexture2D(&td, NULL, &m_pUITex))) return;
		if (FAILED(m_pDev->CreateShaderResourceView(m_pUITex, NULL, &m_pUISRV)))
		{
			SafeRel(m_pUITex); m_pUITex = NULL; return;
		}
		m_uiW = w; m_uiH = h;
	}

	// 565 -> RGBA8: black (0x0000) -> alpha 0 (3D shows through), otherwise alpha 255.
	D3D11_MAPPED_SUBRESOURCE mr;
	if (FAILED(m_pCtx->Map(m_pUITex, 0, D3D11_MAP_WRITE_DISCARD, 0, &mr))) return;
	const unsigned short* src = (const unsigned short*)src565;
	for (int y = 0; y < h; ++y)
	{
		unsigned char* dst = (unsigned char*)mr.pData + (size_t)y * mr.RowPitch;
		const unsigned short* srow = src + (size_t)y * w;
		for (int x = 0; x < w; ++x)
		{
			unsigned short p = srow[x];
			unsigned char* d = dst + (size_t)x * 4;
			if (p == 0) { d[0] = d[1] = d[2] = d[3] = 0; continue; }   // transparent
			unsigned r = (p >> 11) & 0x1F, g = (p >> 5) & 0x3F, b = p & 0x1F;
			d[0] = (unsigned char)((r * 255 + 15) / 31);
			d[1] = (unsigned char)((g * 255 + 31) / 63);
			d[2] = (unsigned char)((b * 255 + 15) / 31);
			d[3] = 255;
		}
	}
	m_pCtx->Unmap(m_pUITex, 0);

	// fullscreen quad over the current RTV: alpha-blend, no depth, point-clamp.
	BeginScreenPass();
	const float bf[4] = { 0, 0, 0, 0 };
	m_pCtx->OMSetBlendState(m_pBlend[BLEND_ALPHA], bf, 0xFFFFFFFF);
	m_pCtx->OMSetDepthStencilState(m_pDepth[0], 0);	// no write/test
	ID3D11SamplerState* samps[2] = { m_pSamp[3], m_pSamp[3] };	// point-clamp (1:1)
	m_pCtx->PSSetSamplers(0, 2, samps);

	m_flags = 1u;	// FF_TEXTURE0 only (no alpha-test/lighting/fog)
	m_renderCBDirty = true;
	SetTexture(0, m_pUISRV);

	const float fw = (float)w, fh = (float)h;
	D3D11_TLVERTEX v[4];
	ZeroMemory(v, sizeof(v));
	v[0].sx = 0;  v[0].sy = 0;  v[0].tu0 = 0; v[0].tv0 = 0;	// TL
	v[1].sx = fw; v[1].sy = 0;  v[1].tu0 = 1; v[1].tv0 = 0;	// TR
	v[2].sx = 0;  v[2].sy = fh; v[2].tu0 = 0; v[2].tv0 = 1;	// BL
	v[3].sx = fw; v[3].sy = fh; v[3].tu0 = 1; v[3].tv0 = 1;	// BR
	for (int i = 0; i < 4; ++i)
	{
		v[i].sz = 0.0f; v[i].rhw = 1.0f;
		v[i].color = 0xFFFFFFFF; v[i].specular = 0;
		v[i].tu1 = v[i].tu0; v[i].tv1 = v[i].tv0;
	}
	DrawTL(5, v, 4);	// TRISTRIP
}

// #30 (Render2DBitmap): draw a CPU bitmap (splash/cursor/mirror) as a textured quad.
// pSrc - pixels in ABGR-DWORD (= R8G8B8A8 byte order), sub-region (sX,sY,w,h) with stride
// totalWidth. fit -> stretch to (screenW,screenH). Temporary one-draw texture.
void D3D11Renderer::DrawBitmap2D(int dX, int dY, int w, int h, int totalWidth, int sX, int sY,
                                 const unsigned* pSrc, bool fit, int screenW, int screenH)
{
	if (!m_pCtx || !m_pDev || !pSrc || w <= 0 || h <= 0) return;

	D3D11_TEXTURE2D_DESC td; ZeroMemory(&td, sizeof(td));
	td.Width = w; td.Height = h; td.MipLevels = 1; td.ArraySize = 1;
	td.Format = DXGI_FORMAT_R8G8B8A8_UNORM; td.SampleDesc.Count = 1;
	td.Usage = D3D11_USAGE_IMMUTABLE; td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	D3D11_SUBRESOURCE_DATA srd; ZeroMemory(&srd, sizeof(srd));
	srd.pSysMem = pSrc + (size_t)sY * totalWidth + sX;   // sub-region
	srd.SysMemPitch = totalWidth * 4;
	ID3D11Texture2D* tex = NULL; ID3D11ShaderResourceView* srv = NULL;
	if (FAILED(m_pDev->CreateTexture2D(&td, &srd, &tex)) || !tex) return;
	if (FAILED(m_pDev->CreateShaderResourceView(tex, NULL, &srv)) || !srv) { tex->Release(); return; }

	BeginScreenPass();
	const float bf[4] = { 0, 0, 0, 0 };
	m_pCtx->OMSetBlendState(m_pBlend[BLEND_ALPHA], bf, 0xFFFFFFFF);
	m_pCtx->OMSetDepthStencilState(m_pDepth[0], 0);   // no write/test
	ID3D11SamplerState* samps[2] = { m_pSamp[1], m_pSamp[1] };   // linear-clamp
	m_pCtx->PSSetSamplers(0, 2, samps);
	m_flags = 1u; m_renderCBDirty = true;             // FF_TEXTURE0 only
	SetTexture(0, srv);

	const float x0 = (float)dX, y0 = (float)dY;
	const float dw = fit ? (float)screenW : (float)w;
	const float dh = fit ? (float)screenH : (float)h;
	D3D11_TLVERTEX v[4]; ZeroMemory(v, sizeof(v));
	v[0].sx = x0;      v[0].sy = y0;      v[0].tu0 = 0; v[0].tv0 = 0;	// TL
	v[1].sx = x0 + dw; v[1].sy = y0;      v[1].tu0 = 1; v[1].tv0 = 0;	// TR
	v[2].sx = x0;      v[2].sy = y0 + dh; v[2].tu0 = 0; v[2].tv0 = 1;	// BL
	v[3].sx = x0 + dw; v[3].sy = y0 + dh; v[3].tu0 = 1; v[3].tv0 = 1;	// BR
	for (int i = 0; i < 4; ++i)
	{
		v[i].sz = 0.0f; v[i].rhw = 1.0f;
		v[i].color = 0xFFFFFFFF; v[i].specular = 0;
		v[i].tu1 = v[i].tu0; v[i].tv1 = v[i].tv0;
	}
	DrawTL(5, v, 4);	// TRISTRIP

	srv->Release(); tex->Release();
}

// MPR_PKT_* (context.h): 1=POINTS 2=LINES 3=POLYLINE 4=TRIANGLES 5=TRISTRIP 6=TRIFAN
void D3D11Renderer::DrawTL(int primType, const D3D11_TLVERTEX* verts, int count)
{
	if (!m_pCtx || count <= 0) return;
	g_bD3D11GPUDraw = true;

	// Lines aren't textured: clear FF_TEXTURE0, otherwise (if a font texture is still bound from
	// text) the line samples a foreign gTex0 and chroma cuts it out. primType 2/3 = lines.
	if ((primType == 2 || primType == 3) && (m_flags & 1u)) { m_flags &= ~1u; m_renderCBDirty = true; }

	UpdateRenderCB();

	const int stride = sizeof(D3D11_TLVERTEX);
	if (!EnsureVB(count * stride)) return;

	// Upload vertices.
	D3D11_MAPPED_SUBRESOURCE ms;
	if (FAILED(m_pCtx->Map(m_pVB, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms))) return;
	memcpy(ms.pData, verts, count * stride);
	m_pCtx->Unmap(m_pVB, 0);

	UINT off = 0, str = stride;
	m_pCtx->IASetVertexBuffers(0, 1, &m_pVB, &str, &off);

	// #7 panel SSAA: during DrawRttQuad (m_forcePerSample) the PS runs per-sample -> 4x SSAA.
	if (m_forcePerSample && m_pPSPerSample) m_pCtx->PSSetShader(m_pPSPerSample, NULL, 0);

	switch (primType)
	{
	case 1: // POINTS
		m_pCtx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
		m_pCtx->Draw(count, 0);
		break;
	case 2: // LINES
		m_pCtx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
		m_pCtx->Draw(count, 0);
		break;
	case 3: // POLYLINE
		m_pCtx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);
		m_pCtx->Draw(count, 0);
		break;
	case 4: // TRIANGLES
		m_pCtx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		m_pCtx->Draw(count, 0);
		break;
	case 5: // TRISTRIP
		m_pCtx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		m_pCtx->Draw(count, 0);
		break;
	case 6: // TRIFAN -- no native D3D11 topology; emulate with an index buffer.
	{
		int tris = count - 2;
		if (tris <= 0) break;
		int nIdx = tris * 3;
		if (!EnsureIB(nIdx)) break;
		D3D11_MAPPED_SUBRESOURCE im;
		if (FAILED(m_pCtx->Map(m_pIB, 0, D3D11_MAP_WRITE_DISCARD, 0, &im))) break;
		unsigned short* idx = (unsigned short*)im.pData;
		for (int i = 0; i < tris; ++i)
		{
			idx[i*3+0] = 0;
			idx[i*3+1] = (unsigned short)(i + 1);
			idx[i*3+2] = (unsigned short)(i + 2);
		}
		m_pCtx->Unmap(m_pIB, 0);
		m_pCtx->IASetIndexBuffer(m_pIB, DXGI_FORMAT_R16_UINT, 0);
		m_pCtx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		m_pCtx->DrawIndexed(nIdx, 0, 0);
		break;
	}
	default:
		break;
	}

	// #7 panel SSAA: restore the normal per-pixel PS after the per-sample draw.
	if (m_forcePerSample && m_pPSPerSample) m_pCtx->PSSetShader(m_pPS, NULL, 0);
}

// Indexed list draw (multi-fan / multi-linestrip batches from context.cpp).
void D3D11Renderer::DrawTLIndexed(int primType, const D3D11_TLVERTEX* verts, int vcount,
                                  const unsigned short* indices, int icount)
{
	if (!m_pCtx || vcount <= 0 || icount <= 0) return;
	g_bD3D11GPUDraw = true;

	// Lines aren't textured (see DrawTL): clear FF_TEXTURE0 for line primitives.
	if (primType == 2 && (m_flags & 1u)) { m_flags &= ~1u; m_renderCBDirty = true; }

	UpdateRenderCB();

	const int stride = sizeof(D3D11_TLVERTEX);
	if (!EnsureVB(vcount * stride)) return;
	if (!EnsureIB(icount))          return;

	D3D11_MAPPED_SUBRESOURCE ms;
	if (FAILED(m_pCtx->Map(m_pVB, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms))) return;
	memcpy(ms.pData, verts, vcount * stride);
	m_pCtx->Unmap(m_pVB, 0);

	D3D11_MAPPED_SUBRESOURCE im;
	if (FAILED(m_pCtx->Map(m_pIB, 0, D3D11_MAP_WRITE_DISCARD, 0, &im))) return;
	memcpy(im.pData, indices, icount * sizeof(unsigned short));
	m_pCtx->Unmap(m_pIB, 0);

	UINT off = 0, str = stride;
	m_pCtx->IASetVertexBuffers(0, 1, &m_pVB, &str, &off);
	m_pCtx->IASetIndexBuffer(m_pIB, DXGI_FORMAT_R16_UINT, 0);
	m_pCtx->IASetPrimitiveTopology(primType == 2
		? D3D11_PRIMITIVE_TOPOLOGY_LINELIST
		: D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	// #7 panel SSAA: per-sample PS during DrawRttQuad.
	if (m_forcePerSample && m_pPSPerSample) m_pCtx->PSSetShader(m_pPSPerSample, NULL, 0);
	m_pCtx->DrawIndexed(icount, 0, 0);
	if (m_forcePerSample && m_pPSPerSample) m_pCtx->PSSetShader(m_pPS, NULL, 0);
}

// D3DPT_* -> D3D11 topology (4=LIST, 5=STRIP, 3=LINESTRIP, 2=LINELIST, 1=POINTLIST).
static D3D11_PRIMITIVE_TOPOLOGY MapTopo(int primType)
{
	switch (primType)
	{
	case 1:  return D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
	case 2:  return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
	case 3:  return D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
	case 5:  return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
	default: return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;	// 4, and fan after expansion
	}
}

void D3D11Renderer::DrawObjectIndexed(int primType, ID3D11Buffer* vb, int stride, int baseVertex,
                                      const unsigned short* indices, int indexCount)
{
	if (!m_pCtx || !vb || indexCount <= 0) return;
	g_bD3D11GPUDraw = true;
	UpdateRenderCB();

	UINT off = 0, str = (UINT)stride;
	m_pCtx->IASetVertexBuffers(0, 1, &vb, &str, &off);

	// TRIANGLEFAN (6) doesn't exist in D3D11 - expand the indices into a TRIANGLELIST.
	if (primType == 6)
	{
		int tris = indexCount - 2;
		if (tris <= 0) return;
		int nIdx = tris * 3;
		if (!EnsureIB(nIdx)) return;
		D3D11_MAPPED_SUBRESOURCE im;
		if (FAILED(m_pCtx->Map(m_pIB, 0, D3D11_MAP_WRITE_DISCARD, 0, &im))) return;
		unsigned short* d = (unsigned short*)im.pData;
		for (int i = 0; i < tris; ++i)
		{
			d[i*3+0] = indices[0];
			d[i*3+1] = indices[i + 1];
			d[i*3+2] = indices[i + 2];
		}
		m_pCtx->Unmap(m_pIB, 0);
		m_pCtx->IASetIndexBuffer(m_pIB, DXGI_FORMAT_R16_UINT, 0);
		m_pCtx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		m_pCtx->DrawIndexed(nIdx, 0, baseVertex);
		return;
	}

	if (!EnsureIB(indexCount)) return;
	D3D11_MAPPED_SUBRESOURCE im;
	if (FAILED(m_pCtx->Map(m_pIB, 0, D3D11_MAP_WRITE_DISCARD, 0, &im))) return;
	memcpy(im.pData, indices, indexCount * sizeof(unsigned short));
	m_pCtx->Unmap(m_pIB, 0);
	m_pCtx->IASetIndexBuffer(m_pIB, DXGI_FORMAT_R16_UINT, 0);
	m_pCtx->IASetPrimitiveTopology(MapTopo(primType));
	m_pCtx->DrawIndexed(indexCount, 0, baseVertex);
}

void D3D11Renderer::DrawObjectStrip(int primType, ID3D11Buffer* vb, int stride,
                                    int startVertex, int vertexCount)
{
	if (!m_pCtx || !vb || vertexCount <= 0) return;
	g_bD3D11GPUDraw = true;
	UpdateRenderCB();
	UINT off = 0, str = (UINT)stride;
	m_pCtx->IASetVertexBuffers(0, 1, &vb, &str, &off);
	m_pCtx->IASetPrimitiveTopology(MapTopo(primType));
	m_pCtx->Draw(vertexCount, startVertex);
}

//=========================== Dynamic 2D-in-3D ================================
// DX2D particles/tracers/blips: billboards in WORLD space. Convert D3DDYNVERTEX(28b) ->
// D3DVERTEXEX(40b) (dummy normal) and draw with the object VS, world=identity, no lighting,
// alpha-blend, depth WITHOUT write (like D3D7 ZWRITE=FALSE).
namespace {
	#pragma pack(push,1)
	struct DynV  { float p[3]; unsigned col, spec; float tu, tv; };          // 28 bytes
	struct ObjV  { float p[3]; float n[3]; unsigned col, spec; float tu, tv; }; // 40 bytes
	#pragma pack(pop)
}

void D3D11Renderer::BeginDynamic2D(bool additive)
{
	if (!m_pCtx) return;
	m_pCtx->VSSetShader(m_pVSobject, NULL, 0);
	m_pCtx->PSSetShader(m_pPS, NULL, 0);
	m_pCtx->IASetInputLayout(m_pILobject);
	m_pCtx->RSSetState(m_pRaster);                 // no depth-bias (this isn't world geometry)
	const float bf[4] = { 0, 0, 0, 0 };
	// #31 tracers/sparks - additive (parity with D3D7 specular COLOR2 emission, otherwise alpha~0.2
	// is nearly invisible); 2D particles/chroma sprites - normal alpha-blend.
	m_pCtx->OMSetBlendState(m_pBlend[additive ? BLEND_ADDITIVE : BLEND_ALPHA], bf, 0xFFFFFFFF);
	m_pCtx->OMSetDepthStencilState(m_pDepth[(0 << 1) | 1], 0); // test, NO write
	// world = identity: vertices are already in world space (Add* added XMMPos).
	static const float I[16] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
	SetWorld(I);
	// vertex-color + alpha-test(chroma), NO lighting. FF_VERTEXCOLOR=1<<2, FF_ALPHATEST=1<<5.
	m_flags = (1u << 2) | (1u << 5);
	m_alphaRef = 0.02f;
	m_renderCBDirty = true;
}

// Convert + upload a batch of D3DDYNVERTEX into m_pVB ONCE per flush; the VB stays bound, then
// multiple DrawDynamic2DIndexed calls follow with different indices/textures.
void D3D11Renderer::UploadDynamic2D(const void* dynVerts, int vcount)
{
	if (!m_pCtx || !dynVerts || vcount <= 0) return;
	const int stride = (int)sizeof(ObjV);
	if (!EnsureVB(vcount * stride)) return;
	D3D11_MAPPED_SUBRESOURCE ms;
	if (FAILED(m_pCtx->Map(m_pVB, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms))) return;
	const DynV* src = (const DynV*)dynVerts;
	ObjV* dst = (ObjV*)ms.pData;
	for (int i = 0; i < vcount; ++i)
	{
		dst[i].p[0] = src[i].p[0]; dst[i].p[1] = src[i].p[1]; dst[i].p[2] = src[i].p[2];
		dst[i].n[0] = 0.0f; dst[i].n[1] = 0.0f; dst[i].n[2] = 1.0f;   // dummy normal
		dst[i].col  = src[i].col;  dst[i].spec = src[i].spec;
		dst[i].tu   = src[i].tu;   dst[i].tv   = src[i].tv;
	}
	m_pCtx->Unmap(m_pVB, 0);
	UINT off = 0, str = (UINT)stride;
	m_pCtx->IASetVertexBuffers(0, 1, &m_pVB, &str, &off);
}

// Indexed draw over the already-uploaded UploadDynamic2D buffer (one batch = one Draws2D item).
void D3D11Renderer::DrawDynamic2DIndexed(const unsigned short* indices, int icount,
                                         ID3D11ShaderResourceView* srv, int primType)
{
	if (!m_pCtx || !indices || icount <= 0 || !m_pVB) return;
	g_bD3D11GPUDraw = true;
	SetTexture(0, srv);   // sets/clears FF_TEXTURE0
	UpdateRenderCB();
	// Rebind the VB: a DrawSortedAlpha (object VBs) may have slipped in between 2D items.
	UINT off = 0, str = (UINT)sizeof(ObjV);
	m_pCtx->IASetVertexBuffers(0, 1, &m_pVB, &str, &off);
	if (!EnsureIB(icount)) return;
	D3D11_MAPPED_SUBRESOURCE im;
	if (FAILED(m_pCtx->Map(m_pIB, 0, D3D11_MAP_WRITE_DISCARD, 0, &im))) return;
	memcpy(im.pData, indices, icount * sizeof(unsigned short));
	m_pCtx->Unmap(m_pIB, 0);
	m_pCtx->IASetIndexBuffer(m_pIB, DXGI_FORMAT_R16_UINT, 0);
	m_pCtx->IASetPrimitiveTopology(MapTopo(primType));
	m_pCtx->DrawIndexed(icount, 0, 0);
}

// Self-contained non-indexed draw of a small batch (points/lines: Draw3DPoint/Line,
// SimpleBuffer). Converts+uploads+draws without indices.
void D3D11Renderer::DrawDynamic2D(const void* dynVerts, int vcount,
                                  ID3D11ShaderResourceView* srv, int primType)
{
	if (!m_pCtx || !dynVerts || vcount <= 0) return;
	g_bD3D11GPUDraw = true;
	SetTexture(0, srv);
	UpdateRenderCB();
	UploadDynamic2D(dynVerts, vcount);   // uploads and binds m_pVB
	m_pCtx->IASetPrimitiveTopology(MapTopo(primType));
	m_pCtx->Draw(vcount, 0);
}

//================================ teardown ===================================

void D3D11Renderer::Release()
{
	SafeRel(m_pVSscreen); SafeRel(m_pVSobject); SafeRel(m_pPS); SafeRel(m_pPSPerSample);
	SafeRel(m_pILscreen); SafeRel(m_pILobject);
	SafeRel(m_pCBViewport); SafeRel(m_pCBView); SafeRel(m_pCBObject);
	SafeRel(m_pCBRender); SafeRel(m_pCBLights);
	SafeRel(m_pVB); SafeRel(m_pIB);
	SafeRel(m_pUISRV); SafeRel(m_pUITex);
	for (int i = 0; i < 3; ++i) SafeRel(m_pBlend[i]);
	for (int i = 0; i < 4; ++i) SafeRel(m_pDepth[i]);
	SafeRel(m_pDSStencilWrite); SafeRel(m_pDSStencilCheck);
	SafeRel(m_pRaster); SafeRel(m_pRasterObj);
	for (int i = 0; i < 4; ++i) SafeRel(m_pSamp[i]);
	ZeroMemory(this, sizeof(*this));
}
