//-----------------------------------------------------------------------------
// D3D11TextureManager.cpp -- see header. PHASE 3 of the D3D7->D3D11 port.
//-----------------------------------------------------------------------------
#include "stdafx.h"
#include "D3D11TextureManager.h"
#include "Graphics\DXEngine\D3D11Backend.h"
#include "context.h"        // MPR_TI_* flags

#include <d3d11.h>

extern "C" void MonoPrint(char *fmt, ...);

D3D11TextureManager* g_pD3D11TextureManager = NULL;

static inline void SafeRel(IUnknown* p) { if (p) p->Release(); }

D3D11TextureManager::D3D11TextureManager() : m_pDev(0), m_pCtx(0) {}
D3D11TextureManager::~D3D11TextureManager() { Release(); }

bool D3D11TextureManager::Init()
{
	if (!g_pD3D11Backend || !g_pD3D11Backend->IsValid())
	{
		MonoPrint("D3D11TextureManager::Init - backend not ready\n");
		return false;
	}
	m_pDev = g_pD3D11Backend->GetDevice();
	m_pCtx = g_pD3D11Backend->GetContext();
	return true;
}

void D3D11TextureManager::Release() { m_pDev = 0; m_pCtx = 0; }

//================================ format mapping =============================

int D3D11TextureManager::DxgiFormatFromMPR(unsigned long f)
{
	if (f & MPR_TI_DXT1)   return DXGI_FORMAT_BC1_UNORM;   // == DXT1
	if (f & MPR_TI_DXT3)   return DXGI_FORMAT_BC2_UNORM;   // == DXT3
	if (f & MPR_TI_DXT5)   return DXGI_FORMAT_BC3_UNORM;   // == DXT5
	if (f & MPR_TI_ARGB32) return DXGI_FORMAT_B8G8R8A8_UNORM;
	if (f & MPR_TI_RGB24)  return DXGI_FORMAT_B8G8R8A8_UNORM; // 24->32 expand
	if (f & MPR_TI_RGB16)  return DXGI_FORMAT_B5G6R5_UNORM;
	// Palettized sources are expanded to BGRA before upload.
	return DXGI_FORMAT_B8G8R8A8_UNORM;
}

bool D3D11TextureManager::IsBlockCompressed(int fmt)
{
	return fmt == DXGI_FORMAT_BC1_UNORM
	    || fmt == DXGI_FORMAT_BC2_UNORM
	    || fmt == DXGI_FORMAT_BC3_UNORM;
}

int D3D11TextureManager::BlockBytes(int fmt)
{
	return (fmt == DXGI_FORMAT_BC1_UNORM) ? 8 : 16;
}

//================================ creation ==================================

bool D3D11TextureManager::Create(D3D11Texture& out, int width, int height,
                                 int dxgiFormat, const TexMipData* mips, int mipCount)
{
	if (!m_pDev || mipCount <= 0) return false;

	D3D11_TEXTURE2D_DESC td;
	ZeroMemory(&td, sizeof(td));
	td.Width            = width;
	td.Height           = height;
	td.MipLevels        = mipCount;
	td.ArraySize        = 1;
	td.Format           = (DXGI_FORMAT)dxgiFormat;
	td.SampleDesc.Count = 1;
	td.Usage            = D3D11_USAGE_DEFAULT;
	td.BindFlags        = D3D11_BIND_SHADER_RESOURCE;

	// Initial data per mip.
	std::vector<D3D11_SUBRESOURCE_DATA> srd(mipCount);
	for (int i = 0; i < mipCount; ++i)
	{
		srd[i].pSysMem          = mips[i].data;
		srd[i].SysMemPitch      = mips[i].rowPitch;
		srd[i].SysMemSlicePitch = 0;
	}

	ID3D11Texture2D* tex = NULL;
	HRESULT hr = m_pDev->CreateTexture2D(&td, &srd[0], &tex);
	if (FAILED(hr))
	{
		MonoPrint("D3D11TextureManager::Create - CreateTexture2D failed hr=0x%08X (%dx%d fmt=%d)\n",
		          (unsigned)hr, width, height, dxgiFormat);
		return false;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.Format                    = (DXGI_FORMAT)dxgiFormat;
	sd.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
	sd.Texture2D.MipLevels       = mipCount;
	sd.Texture2D.MostDetailedMip = 0;

	ID3D11ShaderResourceView* srv = NULL;
	hr = m_pDev->CreateShaderResourceView(tex, &sd, &srv);
	if (FAILED(hr))
	{
		MonoPrint("D3D11TextureManager::Create - CreateSRV failed hr=0x%08X\n", (unsigned)hr);
		SafeRel(tex);
		return false;
	}

	out.tex        = tex;
	out.srv        = srv;
	out.width      = width;
	out.height     = height;
	out.mips       = mipCount;
	out.dxgiFormat = dxgiFormat;
	return true;
}

bool D3D11TextureManager::CreateBCn(D3D11Texture& out, int width, int height,
                                    int dxgiFormat, const void* blockData, int /*bytes*/)
{
	// BCn row pitch = blocks-per-row * block bytes. 4x4 blocks.
	int blocksW   = (width + 3) / 4;
	int rowPitch  = blocksW * BlockBytes(dxgiFormat);

	TexMipData mip;
	mip.data     = blockData;
	mip.rowPitch = rowPitch;
	return Create(out, width, height, dxgiFormat, &mip, 1);
}

void D3D11TextureManager::UpdateMip(D3D11Texture& t, int mip, const void* data, int rowPitch)
{
	if (!m_pCtx || !t.tex) return;
	m_pCtx->UpdateSubresource(t.tex, mip, NULL, data, rowPitch, 0);
}

void D3D11TextureManager::Destroy(D3D11Texture& t)
{
	SafeRel(t.srv); t.srv = 0;
	SafeRel(t.tex); t.tex = 0;
	t.width = t.height = t.mips = 0;
}

//================================ NVTT 3 compression ========================
// NVTT 3.x is x64-only here, so the real compressor compiles only under _WIN64
// (or when REDVIPER_USE_NVTT3 is forced). On Win32 the authoring entry points
// degrade to no-ops -- the runtime never needs them (it loads ready .dds).
#if defined(_WIN64) || defined(REDVIPER_USE_NVTT3)
#include <nvtt/nvtt.h>

// NOTE: nvtt30205.lib is linked via the Falcon4 project's x64 AdditionalDependencies,
// NOT a #pragma comment(lib) here -- the final link uses /NODEFAULTLIB
// (IgnoreAllDefaultLibraries), which suppresses defaultlib pragmas. Drop
// nvtt30205.dll next to the executable at runtime.

namespace {
// NVTT 3 writes through an output handler; collect the BCn bytes into a vector.
struct VecOutput : public nvtt::OutputHandler
{
	std::vector<unsigned char>* out;
	virtual void beginImage(int, int, int, int, int, int) {}
	virtual void endImage() {}
	virtual bool writeData(const void* data, int size)
	{
		const unsigned char* p = (const unsigned char*)data;
		out->insert(out->end(), p, p + size);
		return true;
	}
};

// Pick the NVTT block format from the legacy MPR_TI_* texture-info flags, exactly
// as the old nvDXTcompress path did: alpha -> DXT3/BC2, chroma-key -> DXT1a/BC1a,
// otherwise plain DXT1/BC1.
nvtt::Format NvttFormatFromMPR(unsigned long f)
{
	if (f & MPR_TI_ALPHA)    return nvtt::Format_BC2;
	if (f & MPR_TI_CHROMAKEY) return nvtt::Format_BC1a;
	return nvtt::Format_BC1;
}
} // namespace

bool D3D11TextureManager::CompressBCn(std::vector<unsigned char>& out, int dxgiFormat,
                                      const void* bgra, int width, int height)
{
	nvtt::Format fmt;
	switch (dxgiFormat)
	{
	case DXGI_FORMAT_BC1_UNORM: fmt = nvtt::Format_BC1; break;
	case DXGI_FORMAT_BC2_UNORM: fmt = nvtt::Format_BC2; break;
	case DXGI_FORMAT_BC3_UNORM: fmt = nvtt::Format_BC3; break;
	default: return false;
	}

	nvtt::Surface image;
	// Legacy data is BGRA8 (D3DCOLOR order); NVTT takes it as BGRA input.
	image.setImage(nvtt::InputFormat_BGRA_8UB, width, height, 1, bgra);

	nvtt::CompressionOptions co;
	co.setFormat(fmt);

	nvtt::OutputOptions oo;
	VecOutput sink; sink.out = &out;
	oo.setOutputHandler(&sink);
	oo.setOutputHeader(false);

	nvtt::Context ctx(true);
	return ctx.compress(image, 0, 0, co, oo);
}

bool D3D11TextureManager::SaveBCnDDS(const char* fileName, unsigned long mprTexInfoFlags,
                                     const void* bgra, int width, int height)
{
	if (!fileName || !bgra || width <= 0 || height <= 0)
		return false;

	nvtt::Surface image;
	// Legacy DumpImageToFile produces tightly-packed BGRA8 (B,G,R,A bytes).
	if (!image.setImage(nvtt::InputFormat_BGRA_8UB, width, height, 1, bgra))
		return false;

	nvtt::CompressionOptions co;
	co.setFormat(NvttFormatFromMPR(mprTexInfoFlags));

	nvtt::OutputOptions oo;
	oo.setFileName(fileName);
	oo.setContainer(nvtt::Container_DDS);   // legacy DX9 .dds header (no DX10 ext)
	oo.setOutputHeader(true);

	nvtt::Context ctx(true);                 // CUDA if available, else CPU fallback
	// Single mip (the old path used dNoMipMaps): header first, then the data.
	if (!ctx.outputHeader(image, 1, co, oo))
		return false;
	return ctx.compress(image, 0, 0, co, oo);
}

#else  // !(_WIN64 || REDVIPER_USE_NVTT3)

// Win32 / NVTT-unavailable fallback: offline DDS authoring is not supported.
bool D3D11TextureManager::SaveBCnDDS(const char*, unsigned long, const void*, int, int)
{
	return false;
}

#endif // _WIN64 || REDVIPER_USE_NVTT3
