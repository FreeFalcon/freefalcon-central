//-----------------------------------------------------------------------------
// D3D11TextureManager.h  -- D3D11 replacement for the DirectDraw-surface texture
// path (TextureHandle::m_pDDS). PHASE 3 of the D3D7 -> D3D11 port.
//
// The legacy TextureHandle wraps an IDirectDrawSurface7 created via
// D3DXCreateTexture + D3DXLoadTextureFromMemory. This manager creates the
// equivalent ID3D11Texture2D + ID3D11ShaderResourceView, keeping the BCn
// (DXT1/3/5) data byte-identical -- DXT1==BC1, DXT3==BC2, DXT5==BC3, same bits.
//
// Most runtime textures arrive ALREADY compressed (DXT1) from .dds, so the core
// job is "given BCn or raw bytes, make a D3D11 SRV". Re-compression (the old
// nvtt::Compressor path in Tex.cpp / DumpImageToFile) is provided separately via
// CompressBCn(), backed by modern NVTT 3.x (guard REDVIPER_USE_NVTT3).
//-----------------------------------------------------------------------------
#ifndef _D3D11TEXTUREMANAGER_H_
#define _D3D11TEXTUREMANAGER_H_

#include <windows.h>
#include <vector>

struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11Texture2D;
struct ID3D11ShaderResourceView;

// A created texture. The engine's TextureHandle will hold one of these instead
// of an IDirectDrawSurface7.
struct D3D11Texture
{
	ID3D11Texture2D*          tex;
	ID3D11ShaderResourceView* srv;
	int  width, height, mips;
	int  dxgiFormat;          // DXGI_FORMAT as int (avoid pulling dxgi.h here)

	D3D11Texture() : tex(0), srv(0), width(0), height(0), mips(0), dxgiFormat(0) {}
};

// One mip level of source data.
struct TexMipData
{
	const void* data;
	int         rowPitch;     // bytes per row (or per block-row for BCn)
};

class D3D11TextureManager
{
public:
	D3D11TextureManager();
	~D3D11TextureManager();

	bool Init();              // grabs device from g_pD3D11Backend
	void Release();
	bool IsValid() const { return m_pDev != 0; }

	// Map a legacy MPR_TI_* flag set to a DXGI_FORMAT (returned as int).
	static int  DxgiFormatFromMPR(unsigned long mprTexInfoFlags);
	static bool IsBlockCompressed(int dxgiFormat);
	static int  BlockBytes(int dxgiFormat);    // 8 (BC1) or 16 (BC2/3)

	// Create an immutable texture from one-or-more mip levels already in the
	// target format (BCn bytes, or raw RGBA/565/etc). Fills out. Returns false
	// on failure.
	bool Create(D3D11Texture& out, int width, int height, int dxgiFormat,
	            const TexMipData* mips, int mipCount);

	// Convenience: single-mip BCn (DXT) blob with auto row pitch.
	bool CreateBCn(D3D11Texture& out, int width, int height, int dxgiFormat,
	               const void* blockData, int blockDataBytes);

	// Update one mip of an existing (default-usage) texture.
	void UpdateMip(D3D11Texture& t, int mip, const void* data, int rowPitch);

	void Destroy(D3D11Texture& t);

#ifdef REDVIPER_USE_NVTT3
	// Compress BGRA8 image to BCn (BC1/2/3) using NVTT 3.x. Output appended to
	// 'out' (single mip). format: same DXGI_FORMAT ints as elsewhere.
	bool CompressBCn(std::vector<unsigned char>& out, int dxgiFormat,
	                 const void* bgra, int width, int height);
#endif

private:
	ID3D11Device*        m_pDev;
	ID3D11DeviceContext* m_pCtx;
};

extern D3D11TextureManager* g_pD3D11TextureManager;

#endif // _D3D11TEXTUREMANAGER_H_
