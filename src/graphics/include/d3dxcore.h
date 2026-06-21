//-----------------------------------------------------------------------------
// d3dxcore.h  -- modern replacement for the legacy DirectX 7 D3DX core header.
//
// PHASE 1.5 / PHASE 3 of the D3D7 -> D3D11 port.
//
// The original <d3dxcore.h> (old DirectX 7 SDK) provided two things:
//   1. The D3DX math types/functions  -> now in our replacement <d3dxmath.h>.
//   2. DirectDraw-surface texture helpers + the D3DX_SURFACEFORMAT enum.
//
// Group (2): the D3DX_SURFACEFORMAT enum is the format *vocabulary* the texture
// subsystem (Graphics/Texture/Tex.cpp) is built on -- it is kept verbatim (the
// exact DX7 ordering, because Tex.cpp indexes arrSurfFmt2String[] by it and
// stores it in TextureHandle::m_eSurfFmt). The helper FUNCTIONS are reimplemented
// in d3dxcompat.cpp:
//   * D3DXMakeSurfaceFormat  -- real DDPIXELFORMAT -> enum mapping.
//   * D3DXInitialize/Uninitialize -- no-ops (legacy DDraw/D3DX global init; the
//     D3D11 backend has its own init).
//   * D3DXCreateTexture / D3DXLoadTextureFromMemory -- legacy 2D-bitmap helper
//     (ContextMPR::Render2DBitmap). Stubbed for now (return E_NOTIMPL, caught by
//     the call-site try/catch); to be replaced by the D3D11 2D path in Phase 3.
//-----------------------------------------------------------------------------
#ifndef _REDVIPER_D3DXCORE_H_
#define _REDVIPER_D3DXCORE_H_

#include "d3dxmath.h"

//=============================================================================
// D3DX surface-format enum (verbatim DX7 ordering -- used as array index).
//=============================================================================
enum _D3DX_SURFACEFORMAT
{
	D3DX_SF_UNKNOWN    = 0,
	D3DX_SF_R8G8B8     = 1,
	D3DX_SF_A8R8G8B8   = 2,
	D3DX_SF_X8R8G8B8   = 3,
	D3DX_SF_R5G6B5     = 4,
	D3DX_SF_R5G5B5     = 5,		// X1R5G5B5
	D3DX_SF_PALETTE4   = 6,
	D3DX_SF_PALETTE8   = 7,
	D3DX_SF_A1R5G5B5   = 8,
	D3DX_SF_X4R4G4B4   = 9,
	D3DX_SF_A4R4G4B4   = 10,
	D3DX_SF_L8         = 11,
	D3DX_SF_A8L8       = 12,
	D3DX_SF_U8V8       = 13,
	D3DX_SF_U5V5L6     = 14,
	D3DX_SF_U8V8L8     = 15,
	D3DX_SF_UYVY       = 16,
	D3DX_SF_YUY2       = 17,
	D3DX_SF_DXT1       = 18,
	D3DX_SF_DXT3       = 19,
	D3DX_SF_DXT5       = 20,
	D3DX_SF_R3G3B2     = 21,
	D3DX_SF_A8         = 22,
	D3DX_SF_TEXTUREMAX = 23
};

typedef enum _D3DX_SURFACEFORMAT D3DX_SURFACEFORMAT;

// D3DX flag/filter constants used by the engine.
#ifndef D3DX_DEFAULT
#define D3DX_DEFAULT          (0xFFFFFFFFUL)
#endif
#define D3DX_TEXTURE_NOMIPMAP (0x00000100UL)	// don't generate mip chain
#define D3DX_FT_LINEAR        (0x00000003UL)	// linear filter (load/resample)

//=============================================================================
// Legacy D3DX texture helper functions (reimplemented in d3dxcompat.cpp).
// Forward-declare the DDraw/D3D7 interfaces so this header stays light.
//=============================================================================
struct _DDPIXELFORMAT;
struct IDirect3DDevice7;
struct IDirectDrawSurface7;
struct IDirectDrawPalette;

D3DX_SURFACEFORMAT D3DXMakeSurfaceFormat(struct _DDPIXELFORMAT* pddpf);

long /*HRESULT*/ D3DXInitialize(void);
long /*HRESULT*/ D3DXUninitialize(void);

long /*HRESULT*/ D3DXCreateTexture(
	struct IDirect3DDevice7*      pDevice,
	unsigned long*                pFlags,
	unsigned long*                pWidth,
	unsigned long*                pHeight,
	D3DX_SURFACEFORMAT*           pFormat,
	struct IDirectDrawPalette*    pPalette,
	struct IDirectDrawSurface7**  ppTexture,
	unsigned long*                pNumMipMaps);

long /*HRESULT*/ D3DXLoadTextureFromMemory(
	struct IDirect3DDevice7*      pDevice,
	struct IDirectDrawSurface7*   pTexture,
	unsigned long                 dwMipLevel,
	void*                         pSrc,
	void*                         pSrcRect,		// RECT* (NULL = whole surface)
	D3DX_SURFACEFORMAT            srcFormat,
	unsigned long                 srcPitch,
	struct IDirectDrawPalette*    srcPalette,
	unsigned long                 dwFilter);

#endif // _REDVIPER_D3DXCORE_H_
