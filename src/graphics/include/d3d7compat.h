// Artscout - 2026: [DX7-PURGE] d3d7compat.h
//
// Non-COM, non-runtime replacement for the DirectX 7 <ddraw.h>/<d3dtypes.h>/<d3d.h>
// DATA vocabulary that the engine still legitimately uses:
//   * the D3D primitive/scalar/vector/matrix/viewport types the D3D11/D3D12
//     renderer keeps as its internal representation (D3DPT_*, D3DVALUE, D3DVECTOR,
//     D3DMATRIX, D3DVIEWPORT7, D3DCOLOR);
//   * the DirectDraw surface-description structs used purely as texture metadata
//     carriers (DDSURFACEDESC2 + DDPIXELFORMAT + DDSCAPS2 + DDCOLORKEY) and the
//     DDS_* / DDPF_* / DDSCAPS_* flags the on-disk DDS header parser reads.
//
// It deliberately does NOT declare the COM interfaces (IDirectDraw7 & friends are
// left as opaque struct tags -- pointers work, method calls do not), the DirectDraw
// runtime entry points (DirectDrawCreateEx), or the runtime blit/lock/error
// constants (DDLOCK_*, DDBLT_*, DDERR_*). Any code that still needs those is dead
// DDraw runtime and must be removed -- the resulting compile error is the checklist.
//
// Struct layouts are byte-identical to the DX7 SDK (DDSURFACEDESC2 == 124 bytes on
// x86, 128 on x64 via LPVOID lpSurface) so existing sizeof()/fread() texture-loading
// paths behave exactly as before.

#ifndef _D3D7COMPAT_H_
#define _D3D7COMPAT_H_

#include <windows.h>   // DWORD, LONG, WORD, LPVOID, HRESULT, S_OK

// ---------------------------------------------------------------------------
// Direct3D scalar / colour / vector / matrix / viewport / primitive vocabulary
// (from <d3dtypes.h>; kept because the modern renderer uses them internally).
// ---------------------------------------------------------------------------
// These four are "shared" DirectX types that <dsound.h>, <dxgitype.h> and the
// D3D9/11 headers ALSO define, each guarded by the same *_DEFINED macro. We must
// use the identical guards so the shim coexists with whichever header wins.
#ifndef D3DVALUE_DEFINED
typedef float D3DVALUE;
#define D3DVALUE_DEFINED
#endif
typedef D3DVALUE *LPD3DVALUE;

#ifndef D3DCOLOR_DEFINED
typedef DWORD D3DCOLOR;
#define D3DCOLOR_DEFINED
#endif
#ifndef LPD3DCOLOR_DEFINED
typedef DWORD *LPD3DCOLOR;
#define LPD3DCOLOR_DEFINED
#endif

#ifndef D3DVECTOR_DEFINED
typedef struct _D3DVECTOR
{
    float x;
    float y;
    float z;
} D3DVECTOR;
#define D3DVECTOR_DEFINED
#endif
#ifndef LPD3DVECTOR_DEFINED
typedef D3DVECTOR *LPD3DVECTOR;
#define LPD3DVECTOR_DEFINED
#endif

#ifndef D3DMATRIX_DEFINED
typedef struct _D3DMATRIX
{
    union {
        struct {
            float _11, _12, _13, _14;
            float _21, _22, _23, _24;
            float _31, _32, _33, _34;
            float _41, _42, _43, _44;
        };
        float m[4][4];
    };
} D3DMATRIX;
#define D3DMATRIX_DEFINED
#endif
typedef D3DMATRIX *LPD3DMATRIX;

typedef struct _D3DVIEWPORT7
{
    DWORD    dwX;
    DWORD    dwY;
    DWORD    dwWidth;
    DWORD    dwHeight;
    D3DVALUE dvMinZ;
    D3DVALUE dvMaxZ;
} D3DVIEWPORT7, *LPD3DVIEWPORT7;

typedef enum _D3DPRIMITIVETYPE
{
    D3DPT_POINTLIST     = 1,
    D3DPT_LINELIST      = 2,
    D3DPT_LINESTRIP     = 3,
    D3DPT_TRIANGLELIST  = 4,
    D3DPT_TRIANGLESTRIP = 5,
    D3DPT_TRIANGLEFAN   = 6,
    D3DPT_FORCE_DWORD   = 0x7fffffff
} D3DPRIMITIVETYPE;

#ifndef D3D_OK
#define D3D_OK ((HRESULT)0)   // == S_OK
#endif
#define D3DENUMRET_OK      1
#define D3DENUMRET_CANCEL  0

// ---- FourCC + D3D colour helper macros (from <ddraw.h>/<d3dtypes.h>) ----
#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3)                                  \
    ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) |                   \
    ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24))
#endif

#define RGBA_GETALPHA(rgb)      ((rgb) >> 24)
#define RGBA_GETRED(rgb)        (((rgb) >> 16) & 0xff)
#define RGBA_GETGREEN(rgb)      (((rgb) >> 8) & 0xff)
#define RGBA_GETBLUE(rgb)       ((rgb) & 0xff)
#define RGBA_MAKE(r, g, b, a)   ((D3DCOLOR)(((a) << 24) | ((r) << 16) | ((g) << 8) | (b)))
#define D3DRGB(r, g, b)         \
    (0xff000000L | (((long)((r) * 255)) << 16) | (((long)((g) * 255)) << 8) | (long)((b) * 255))
#define D3DRGBA(r, g, b, a)     \
    ((((long)((a) * 255)) << 24) | (((long)((r) * 255)) << 16) | (((long)((g) * 255)) << 8) | (long)((b) * 255))

// ---------------------------------------------------------------------------
// DirectDraw surface-description data structs (texture metadata carriers only).
// Byte-identical layout to the DX7 SDK.
// ---------------------------------------------------------------------------
typedef struct _DDCOLORKEY
{
    DWORD dwColorSpaceLowValue;
    DWORD dwColorSpaceHighValue;
} DDCOLORKEY;

typedef struct _DDSCAPS2
{
    DWORD dwCaps;
    DWORD dwCaps2;
    DWORD dwCaps3;
    DWORD dwCaps4;
} DDSCAPS2;

typedef struct _DDPIXELFORMAT
{
    DWORD dwSize;
    DWORD dwFlags;
    DWORD dwFourCC;
    DWORD dwRGBBitCount;        // union in SDK (YUV/Z/alpha/luminance bit counts)
    DWORD dwRBitMask;           // union in SDK
    DWORD dwGBitMask;           // union in SDK
    DWORD dwBBitMask;           // union in SDK
    DWORD dwRGBAlphaBitMask;    // union in SDK
} DDPIXELFORMAT;

typedef struct _DDSURFACEDESC2
{
    DWORD dwSize;
    DWORD dwFlags;
    DWORD dwHeight;
    DWORD dwWidth;
    union { LONG lPitch; DWORD dwLinearSize; };
    union { DWORD dwBackBufferCount; DWORD dwDepth; };
    union { DWORD dwMipMapCount; DWORD dwRefreshRate; DWORD dwSrcVBHandle; };
    DWORD dwAlphaBitDepth;
    DWORD dwReserved;
    LPVOID lpSurface;
    union { DDCOLORKEY ddckCKDestOverlay; DWORD dwEmptyFaceColor; };
    DDCOLORKEY ddckCKDestBlt;
    DDCOLORKEY ddckCKSrcOverlay;
    DDCOLORKEY ddckCKSrcBlt;
    union { DDPIXELFORMAT ddpfPixelFormat; DWORD dwFVF; };
    DDSCAPS2 ddsCaps;
    DWORD dwTextureStage;
} DDSURFACEDESC2, *LPDDSURFACEDESC2;

// ---- DDSURFACEDESC2.dwFlags ----
#define DDSD_CAPS               0x00000001l
#define DDSD_HEIGHT             0x00000002l
#define DDSD_WIDTH              0x00000004l
#define DDSD_PITCH              0x00000008l
#define DDSD_PIXELFORMAT        0x00001000l
#define DDSD_MIPMAPCOUNT        0x00020000l
#define DDSD_LINEARSIZE         0x00080000l
#define DDSD_TEXTURESTAGE       0x00100000l

// ---- DDPIXELFORMAT.dwFlags ----
#define DDPF_ALPHAPIXELS        0x00000001l
#define DDPF_ALPHA              0x00000002l
#define DDPF_FOURCC             0x00000004l
#define DDPF_RGB                0x00000040l
#define DDPF_LUMINANCE          0x00020000l

// ---- DDSCAPS2.dwCaps (texture metadata subset) ----
#define DDSCAPS_COMPLEX         0x00000008l
#define DDSCAPS_TEXTURE         0x00001000l
#define DDSCAPS_MIPMAP          0x00400000l

// ---- DDSCAPS2.dwCaps2 ----
#define DDSCAPS2_CUBEMAP            0x00000200l
#define DDSCAPS2_CUBEMAP_POSITIVEX  0x00000400l
#define DDSCAPS2_CUBEMAP_NEGATIVEX  0x00000800l
#define DDSCAPS2_CUBEMAP_POSITIVEY  0x00001000l
#define DDSCAPS2_CUBEMAP_NEGATIVEY  0x00002000l
#define DDSCAPS2_CUBEMAP_POSITIVEZ  0x00004000l
#define DDSCAPS2_CUBEMAP_NEGATIVEZ  0x00008000l

#ifndef DD_OK
#define DD_OK ((HRESULT)0)   // == S_OK
#endif

// ---------------------------------------------------------------------------
// Direct3D 7 light / material / vertex-buffer descriptors + enums the engine
// still uses as its internal data representation (filled on the CPU side).
// ---------------------------------------------------------------------------
typedef enum _D3DLIGHTTYPE
{
    D3DLIGHT_POINT         = 1,
    D3DLIGHT_SPOT          = 2,
    D3DLIGHT_DIRECTIONAL   = 3,
    D3DLIGHT_PARALLELPOINT = 4,
    D3DLIGHT_FORCE_DWORD   = 0x7fffffff
} D3DLIGHTTYPE;

typedef enum _D3DSHADEMODE
{
    D3DSHADE_FLAT        = 1,
    D3DSHADE_GOURAUD     = 2,
    D3DSHADE_PHONG       = 3,
    D3DSHADE_FORCE_DWORD = 0x7fffffff
} D3DSHADEMODE;

#ifndef D3DCOLORVALUE_DEFINED
typedef struct _D3DCOLORVALUE
{
    float r;
    float g;
    float b;
    float a;
} D3DCOLORVALUE;
#define D3DCOLORVALUE_DEFINED
#endif

typedef struct _D3DLIGHT7
{
    D3DLIGHTTYPE  dltType;
    D3DCOLORVALUE dcvDiffuse;
    D3DCOLORVALUE dcvSpecular;
    D3DCOLORVALUE dcvAmbient;
    D3DVECTOR     dvPosition;
    D3DVECTOR     dvDirection;
    D3DVALUE      dvRange;
    D3DVALUE      dvFalloff;
    D3DVALUE      dvAttenuation0;
    D3DVALUE      dvAttenuation1;
    D3DVALUE      dvAttenuation2;
    D3DVALUE      dvTheta;
    D3DVALUE      dvPhi;
} D3DLIGHT7, *LPD3DLIGHT7;

typedef struct _D3DMATERIAL7
{
    union { D3DCOLORVALUE diffuse;  D3DCOLORVALUE dcvDiffuse; };
    union { D3DCOLORVALUE ambient;  D3DCOLORVALUE dcvAmbient; };
    union { D3DCOLORVALUE specular; D3DCOLORVALUE dcvSpecular; };
    union { D3DCOLORVALUE emissive; D3DCOLORVALUE dcvEmissive; };
    union { D3DVALUE      power;    D3DVALUE dvPower; };
} D3DMATERIAL7, *LPD3DMATERIAL7;

typedef struct _D3DVERTEXBUFFERDESC
{
    DWORD dwSize;
    DWORD dwCaps;
    DWORD dwFVF;
    DWORD dwNumVertices;
} D3DVERTEXBUFFERDESC, *LPD3DVERTEXBUFFERDESC;

// ---- D3DRENDERSTATETYPE values used by the engine ----
#define D3DRENDERSTATE_SHADEMODE                9
#define D3DRENDERSTATE_FOGENABLE                28
#define D3DRENDERSTATE_ZBIAS                    47
#define D3DRENDERSTATE_CLIPPING                 136
#define D3DRENDERSTATE_DIFFUSEMATERIALSOURCE    145
#define D3DRENDERSTATE_SPECULARMATERIALSOURCE   146
#define D3DRENDERSTATE_AMBIENTMATERIALSOURCE    147
#define D3DRENDERSTATE_EMISSIVEMATERIALSOURCE   148

// ---- D3DFVF flags (standard subset; engine's DYNAMIC/MANAGED/SIMPLE live elsewhere) ----
#define D3DFVF_XYZ      0x002
#define D3DFVF_NORMAL   0x010
#define D3DFVF_DIFFUSE  0x040
#define D3DFVF_SPECULAR 0x080
#define D3DFVF_TEX1     0x100

// ---------------------------------------------------------------------------
// DirectDraw runtime flags (lock / blit / caps / cooperative-level). Present as
// plain #defines so legacy (gated-off, non-GPU) code parses; the actual DDraw
// surface method calls it wraps still fail to compile against the opaque tags.
// ---------------------------------------------------------------------------
#define DDPF_PALETTEINDEXED8    0x00000020l
#define DDPF_ZBUFFER            0x00000400l
#define DDPF_BUMPLUMINANCE      0x00040000l
#define DDPF_BUMPDUDV           0x00080000l

#define DDSCAPS_BACKBUFFER      0x00000004l
#define DDSCAPS_FLIP            0x00000010l
#define DDSCAPS_OFFSCREENPLAIN  0x00000040l
#define DDSCAPS_PRIMARYSURFACE  0x00000200l
#define DDSCAPS_SYSTEMMEMORY    0x00000800l
#define DDSCAPS_3DDEVICE        0x00002000l
#define DDSCAPS_VIDEOMEMORY     0x00004000l
#define DDSCAPS_ZBUFFER         0x00020000l
#define DDSCAPS_LOCALVIDMEM     0x10000000l

#define DDSCAPS2_HINTDYNAMIC    0x00000004l
#define DDSCAPS2_HINTSTATIC     0x00000008l
#define DDSCAPS2_TEXTUREMANAGE  0x00000010l
#define DDSCAPS2_DONOTPERSIST   0x00040000l

#define DDLOCK_SURFACEMEMORYPTR 0x00000000L
#define DDLOCK_WAIT             0x00000001L
#define DDLOCK_WRITEONLY        0x00000020L
#define DDLOCK_NOSYSLOCK        0x00000800L
#define DDLOCK_NOOVERWRITE      0x00001000L
#define DDLOCK_DISCARDCONTENTS  0x00002000L
#define DDLOCK_DONOTWAIT        0x00004000L

#define DDBLT_COLORFILL         0x00000400l
#define DDBLT_KEYSRC            0x00008000l
#define DDBLT_WAIT              0x01000000l
#define DDBLTFAST_NOCOLORKEY    0x00000000
#define DDBLTFAST_SRCCOLORKEY   0x00000001
#define DDBLTFAST_WAIT          0x00000010

#define DDCKEY_SRCBLT           0x00000008l
#define DDGBS_ISBLTDONE         0x00000002l
#define DDENUMRET_OK            1
#define DDENUMRET_CANCEL        0

#define DDSCL_FULLSCREEN        0x00000001l
#define DDSCL_ALLOWREBOOT       0x00000002l
#define DDSCL_NORMAL            0x00000008l
#define DDSCL_EXCLUSIVE         0x00000010l
#define DDSCL_MULTITHREADED     0x00000400l
#define DDSCL_FPUPRESERVE       0x00001000l

// ---------------------------------------------------------------------------
// Opaque handle tags for the retired DX7 COM interfaces. Surviving code keeps
// these only as opaque pointers (stored, compared to NULL, reinterpreted as a
// D3D11/D3D12 handle). Any *.method() call fails to compile -> dead runtime.
// ---------------------------------------------------------------------------
struct IDirectDraw7;
struct IDirect3D7;
struct IDirect3DDevice7;
struct IDirectDrawSurface7;
struct IDirectDrawPalette;
struct IDirect3DVertexBuffer7;

typedef struct IDirectDraw7           *LPDIRECTDRAW7;
typedef struct IDirect3D7             *LPDIRECT3D7;
typedef struct IDirect3DDevice7       *LPDIRECT3DDEVICE7;
typedef struct IDirectDrawSurface7    *LPDIRECTDRAWSURFACE7;
typedef struct IDirectDrawPalette     *LPDIRECTDRAWPALETTE;
typedef struct IDirect3DVertexBuffer7 *LPDIRECT3DVERTEXBUFFER7;

// ---------------------------------------------------------------------------
// Legacy DirectDraw/Direct3D7 device-enumeration + blit descriptor structs.
// Referenced only by the (dead under D3D11/D3D12) DDraw driver/mode enumeration
// kept alive for the graphics-settings UI. Provided as inert data structs; the
// actual DDraw runtime calls that fill them still fail to compile (opaque tags).
// ---------------------------------------------------------------------------
#define _FACDD  0x876
#ifndef MAKE_DDHRESULT
#define MAKE_DDHRESULT(code)  MAKE_HRESULT(1, _FACDD, code)
#endif
#define DDERR_SURFACELOST               MAKE_DDHRESULT(450)
#define DDERR_OUTOFVIDEOMEMORY          MAKE_DDHRESULT(380)
#define DDERR_WASSTILLDRAWING           MAKE_DDHRESULT(540)

#define DDSD_BACKBUFFERCOUNT            0x00000020l
#define DDBD_32                         0x00000100l
#define DDPCAPS_8BIT                    0x00000004l
#define DDPCAPS_ALLOW256                0x00000040l
#define DDENUM_ATTACHEDSECONDARYDEVICES 0x00000001L
#define DDENUM_NONDISPLAYDEVICES        0x00000004L
#define DD_ROP_SPACE                    (256 / 32)
#define MAX_DDDEVICEID_STRING           512

#define D3DSTATUS_ZNOTVISIBLE           0x00000001L
#define D3DSTATUS_CLIPINTERSECTIONALL   0x001FE000L
#define D3DSTATUS_DEFAULT               (D3DSTATUS_CLIPINTERSECTIONALL | D3DSTATUS_ZNOTVISIBLE)
#define D3DVBCAPS_DONOTCLIP             0x00000001l
#define D3DVBCAPS_WRITEONLY             0x00010000l
#define D3DDEVCAPS_HWTRANSFORMANDLIGHT  0x00010000L
#define D3DDEVCAPS_HWRASTERIZATION      0x00080000L
#define D3DPTEXTURECAPS_POW2            0x00000002L
#define D3DPTEXTURECAPS_SQUAREONLY      0x00000020L

#ifndef DUMMYUNIONNAMEN
#define DUMMYUNIONNAMEN(n)
#endif

// Non-'7' opaque interface tags (used by legacy DDraw signatures).
struct IDirectDraw;
struct IDirect3D;
struct IDirectDrawSurface;
struct IDirectDrawClipper;
struct IDirectDrawGammaControl;
typedef struct IDirectDraw        *LPDIRECTDRAW;
typedef struct IDirectDrawSurface *LPDIRECTDRAWSURFACE;
typedef struct IDirectDrawClipper *LPDIRECTDRAWCLIPPER;

typedef struct _DDSCAPS { DWORD dwCaps; } DDSCAPS;

typedef struct _DDBLTFX
{
    DWORD dwSize;
    DWORD dwDDFX;
    DWORD dwROP;
    DWORD dwDDROP;
    DWORD dwRotationAngle;
    DWORD dwZBufferOpCode;
    DWORD dwZBufferLow;
    DWORD dwZBufferHigh;
    DWORD dwZBufferBaseDest;
    DWORD dwZDestConstBitDepth;
    union { DWORD dwZDestConst; LPDIRECTDRAWSURFACE lpDDSZBufferDest; };
    DWORD dwZSrcConstBitDepth;
    union { DWORD dwZSrcConst; LPDIRECTDRAWSURFACE lpDDSZBufferSrc; };
    DWORD dwAlphaEdgeBlendBitDepth;
    DWORD dwAlphaEdgeBlend;
    DWORD dwReserved;
    DWORD dwAlphaDestConstBitDepth;
    union { DWORD dwAlphaDestConst; LPDIRECTDRAWSURFACE lpDDSAlphaDest; };
    DWORD dwAlphaSrcConstBitDepth;
    union { DWORD dwAlphaSrcConst; LPDIRECTDRAWSURFACE lpDDSAlphaSrc; };
    union { DWORD dwFillColor; DWORD dwFillDepth; DWORD dwFillPixel; LPDIRECTDRAWSURFACE lpDDSPattern; };
    DDCOLORKEY ddckDestColorkey;
    DDCOLORKEY ddckSrcColorkey;
} DDBLTFX;

typedef struct _DDCAPS_DX7
{
    DWORD   dwSize;
    DWORD   dwCaps;
    DWORD   dwCaps2;
    DWORD   dwCKeyCaps;
    DWORD   dwFXCaps;
    DWORD   dwFXAlphaCaps;
    DWORD   dwPalCaps;
    DWORD   dwSVCaps;
    DWORD   dwAlphaBltConstBitDepths;
    DWORD   dwAlphaBltPixelBitDepths;
    DWORD   dwAlphaBltSurfaceBitDepths;
    DWORD   dwAlphaOverlayConstBitDepths;
    DWORD   dwAlphaOverlayPixelBitDepths;
    DWORD   dwAlphaOverlaySurfaceBitDepths;
    DWORD   dwZBufferBitDepths;
    DWORD   dwVidMemTotal;
    DWORD   dwVidMemFree;
    DWORD   dwMaxVisibleOverlays;
    DWORD   dwCurrVisibleOverlays;
    DWORD   dwNumFourCCCodes;
    DWORD   dwAlignBoundarySrc;
    DWORD   dwAlignSizeSrc;
    DWORD   dwAlignBoundaryDest;
    DWORD   dwAlignSizeDest;
    DWORD   dwAlignStrideAlign;
    DWORD   dwRops[DD_ROP_SPACE];
    DDSCAPS ddsOldCaps;
    DWORD   dwMinOverlayStretch;
    DWORD   dwMaxOverlayStretch;
    DWORD   dwMinLiveVideoStretch;
    DWORD   dwMaxLiveVideoStretch;
    DWORD   dwMinHwCodecStretch;
    DWORD   dwMaxHwCodecStretch;
    DWORD   dwReserved1;
    DWORD   dwReserved2;
    DWORD   dwReserved3;
    DWORD   dwSVBCaps;
    DWORD   dwSVBCKeyCaps;
    DWORD   dwSVBFXCaps;
    DWORD   dwSVBRops[DD_ROP_SPACE];
    DWORD   dwVSBCaps;
    DWORD   dwVSBCKeyCaps;
    DWORD   dwVSBFXCaps;
    DWORD   dwVSBRops[DD_ROP_SPACE];
    DWORD   dwSSBCaps;
    DWORD   dwSSBCKeyCaps;
    DWORD   dwSSBFXCaps;
    DWORD   dwSSBRops[DD_ROP_SPACE];
    DWORD   dwMaxVideoPorts;
    DWORD   dwCurrVideoPorts;
    DWORD   dwSVBCaps2;
    DWORD   dwNLVBCaps;
    DWORD   dwNLVBCaps2;
    DWORD   dwNLVBCKeyCaps;
    DWORD   dwNLVBFXCaps;
    DWORD   dwNLVBRops[DD_ROP_SPACE];
    DDSCAPS2 ddsCaps;
} DDCAPS_DX7;
typedef DDCAPS_DX7 DDCAPS;

typedef struct tagDDDEVICEIDENTIFIER2
{
    char          szDriver[MAX_DDDEVICEID_STRING];
    char          szDescription[MAX_DDDEVICEID_STRING];
    LARGE_INTEGER liDriverVersion;
    DWORD         dwVendorId;
    DWORD         dwDeviceId;
    DWORD         dwSubSysId;
    DWORD         dwRevision;
    GUID          guidDeviceIdentifier;
    DWORD         dwWHQLLevel;
} DDDEVICEIDENTIFIER2, *LPDDDEVICEIDENTIFIER2;

typedef struct _D3DPRIMCAPS
{
    DWORD dwSize;
    DWORD dwMiscCaps;
    DWORD dwRasterCaps;
    DWORD dwZCmpCaps;
    DWORD dwSrcBlendCaps;
    DWORD dwDestBlendCaps;
    DWORD dwAlphaCmpCaps;
    DWORD dwShadeCaps;
    DWORD dwTextureCaps;
    DWORD dwTextureFilterCaps;
    DWORD dwTextureBlendCaps;
    DWORD dwTextureAddressCaps;
    DWORD dwStippleWidth;
    DWORD dwStippleHeight;
} D3DPRIMCAPS, *LPD3DPRIMCAPS;

typedef struct _D3DDeviceDesc7
{
    DWORD       dwDevCaps;
    D3DPRIMCAPS dpcLineCaps;
    D3DPRIMCAPS dpcTriCaps;
    DWORD       dwDeviceRenderBitDepth;
    DWORD       dwDeviceZBufferBitDepth;
    DWORD       dwMinTextureWidth, dwMinTextureHeight;
    DWORD       dwMaxTextureWidth, dwMaxTextureHeight;
    DWORD       dwMaxTextureRepeat;
    DWORD       dwMaxTextureAspectRatio;
    DWORD       dwMaxAnisotropy;
    D3DVALUE    dvGuardBandLeft;
    D3DVALUE    dvGuardBandTop;
    D3DVALUE    dvGuardBandRight;
    D3DVALUE    dvGuardBandBottom;
    D3DVALUE    dvExtentsAdjust;
    DWORD       dwStencilCaps;
    DWORD       dwFVFCaps;
    DWORD       dwTextureOpCaps;
    WORD        wMaxTextureBlendStages;
    WORD        wMaxSimultaneousTextures;
    DWORD       dwMaxActiveLights;
    D3DVALUE    dvMaxVertexW;
    GUID        deviceGUID;
    WORD        wMaxUserClipPlanes;
    WORD        wMaxVertexBlendMatrices;
    DWORD       dwVertexProcessingCaps;
    DWORD       dwReserved1;
    DWORD       dwReserved2;
    DWORD       dwReserved3;
    DWORD       dwReserved4;
} D3DDEVICEDESC7, *LPD3DDEVICEDESC7;

// ---- Legacy (pre-7) D3D material + misc typedefs ----
typedef DWORD D3DTEXTUREHANDLE;
typedef DDPIXELFORMAT *LPDDPIXELFORMAT;

typedef struct _D3DMATERIAL
{
    DWORD dwSize;
    union { D3DCOLORVALUE diffuse;  D3DCOLORVALUE dcvDiffuse; };
    union { D3DCOLORVALUE ambient;  D3DCOLORVALUE dcvAmbient; };
    union { D3DCOLORVALUE specular; D3DCOLORVALUE dcvSpecular; };
    union { D3DCOLORVALUE emissive; D3DCOLORVALUE dcvEmissive; };
    union { D3DVALUE      power;    D3DVALUE dvPower; };
    D3DTEXTUREHANDLE hTexture;
    DWORD dwRampSize;
} D3DMATERIAL, *LPD3DMATERIAL;

#endif // _D3D7COMPAT_H_
