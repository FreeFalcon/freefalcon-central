//-----------------------------------------------------------------------------
// d3dxcompat.cpp  -- reimplementation of the few legacy DX7 D3DX *functions*
// the engine still calls. The DX7 D3DX utility library is not part of the modern
// Windows SDK; the DDraw/D3D7 *types* still are (ddraw.h / d3d.h). See d3dxcore.h.
//
//  * D3DXMakeSurfaceFormat       -- DDPIXELFORMAT -> D3DX_SURFACEFORMAT (real).
//  * D3DXInitialize/Uninitialize -- legacy global init; no-op now (D3D11 backend
//                                   has its own init).
//  * D3DXCreateTexture / D3DXLoadTextureFromMemory -- legacy 2D-bitmap helper
//                                   (ContextMPR::Render2DBitmap). Stubbed
//                                   (E_NOTIMPL, caught by the call-site
//                                   try/catch) until the D3D11 2D path lands.
//-----------------------------------------------------------------------------
#include <windows.h>
#include "d3d7compat.h"
#include "d3dxcore.h"

//-----------------------------------------------------------------------------
// DDPIXELFORMAT -> D3DX_SURFACEFORMAT. Mirrors what the old D3DX did for the
// formats this engine actually uses (FourCC DXTn, 32/16-bit RGB, palette8).
//-----------------------------------------------------------------------------
D3DX_SURFACEFORMAT D3DXMakeSurfaceFormat(struct _DDPIXELFORMAT* pddpfIn)
{
	const DDPIXELFORMAT* p = reinterpret_cast<const DDPIXELFORMAT*>(pddpfIn);
	if (!p)
		return D3DX_SF_UNKNOWN;

	// Block-compressed (FourCC)
	if (p->dwFlags & DDPF_FOURCC)
	{
		if (p->dwFourCC == MAKEFOURCC('D','X','T','1')) return D3DX_SF_DXT1;
		if (p->dwFourCC == MAKEFOURCC('D','X','T','3')) return D3DX_SF_DXT3;
		if (p->dwFourCC == MAKEFOURCC('D','X','T','5')) return D3DX_SF_DXT5;
		return D3DX_SF_UNKNOWN;
	}

	// 8-bit palettized
	if (p->dwFlags & DDPF_PALETTEINDEXED8)
		return D3DX_SF_PALETTE8;

	// Uncompressed RGB(A)
	if (p->dwFlags & DDPF_RGB)
	{
		const bool hasAlpha = (p->dwFlags & DDPF_ALPHAPIXELS) != 0;

		switch (p->dwRGBBitCount)
		{
		case 32:
		case 24:
			return hasAlpha ? D3DX_SF_A8R8G8B8 : D3DX_SF_X8R8G8B8;

		case 16:
			// 5-6-5 has a 6-bit green channel (mask 0x07E0).
			if (p->dwGBitMask == 0x07E0)
				return D3DX_SF_R5G6B5;
			// 4-4-4-4 has a 4-bit red channel (mask 0x0F00).
			if (p->dwRBitMask == 0x0F00)
				return D3DX_SF_A4R4G4B4;
			// 5-5-5 with/without a 1-bit alpha.
			return hasAlpha ? D3DX_SF_A1R5G5B5 : D3DX_SF_R5G5B5;
		}
	}

	return D3DX_SF_UNKNOWN;
}

//-----------------------------------------------------------------------------
// Legacy global init -- no longer needed (the D3D11 backend initialises itself).
//-----------------------------------------------------------------------------
long D3DXInitialize(void)   { return S_OK; }
long D3DXUninitialize(void) { return S_OK; }

//-----------------------------------------------------------------------------
// Legacy 2D-bitmap texture helper. TODO[Phase 3]: replace with the D3D11 2D
// path (ContextMPR::Render2DBitmap). Return E_NOTIMPL -- the only caller wraps
// these in CheckHR inside a try/catch, so the bitmap is simply skipped.
//-----------------------------------------------------------------------------
long D3DXCreateTexture(
	struct IDirect3DDevice7*      /*pDevice*/,
	unsigned long*                /*pFlags*/,
	unsigned long*                /*pWidth*/,
	unsigned long*                /*pHeight*/,
	D3DX_SURFACEFORMAT*           /*pFormat*/,
	struct IDirectDrawPalette*    /*pPalette*/,
	struct IDirectDrawSurface7**  ppTexture,
	unsigned long*                /*pNumMipMaps*/)
{
	if (ppTexture) *ppTexture = 0;
	return E_NOTIMPL;
}

long D3DXLoadTextureFromMemory(
	struct IDirect3DDevice7*      /*pDevice*/,
	struct IDirectDrawSurface7*   /*pTexture*/,
	unsigned long                 /*dwMipLevel*/,
	void*                         /*pSrc*/,
	void*                         /*pSrcRect*/,
	D3DX_SURFACEFORMAT            /*srcFormat*/,
	unsigned long                 /*srcPitch*/,
	struct IDirectDrawPalette*    /*srcPalette*/,
	unsigned long                 /*dwFilter*/)
{
	return E_NOTIMPL;
}
