/***************************************************************************\
    Context.h
    Scott Randolph
	April 29, 1996

    //JAM 06Oct03 - Begin Major Rewrite
\***************************************************************************/
#ifndef _3DEJ_CONTEXT_H_
#define _3DEJ_CONTEXT_H_

#include "define.h"

#if !defined(MPR_INTERNAL)
#include <windows.h>
#endif

#include <vector>
#include <string>
#include <ddraw.h>
#include <d3dtypes.h>
#include "alloc.h"

#ifdef  __cplusplus
extern  "C"{
#endif

#ifndef _SHI__INT_H_
typedef unsigned int        UInt;
typedef unsigned char       UInt8;
typedef unsigned short      UInt16;
typedef unsigned long       UInt32;

typedef signed   int        Int;
typedef signed   char       Int8;
typedef signed   short      Int16;
typedef signed   long       Int32;
#endif

typedef UInt  (*DWProc_t)();
typedef UInt  MPRHandle_t;
typedef float Float_t;

//NOTE: Care is needed when mucking with these structures. Some of the asm code ASSumes groupings and alignments.
//START OF SPECIAL CARE SECTION
typedef struct { 
  Float_t x, y;             
} MPRVtx_t;

typedef struct { 
  Float_t x, y; 
  Float_t r, g;
  Float_t b, a;
} MPRVtxClr_t;

typedef struct { 
  Float_t x, y; 
  Float_t r, g;
  Float_t b, a;   
  Float_t u, v;
  Float_t u1, v1;
  Float_t s;
  Float_t q;
} MPRVtxTexClr_t;
//END OF SPECIAL CARE SECTION

typedef enum MPRSurfaceType
{
	SystemMem, VideoMem,	// Valid for front or back buffer specifier
	Primary, 				// Valid for front buffer specifier only
	Flip,					// Valid for back buffer specifier only
	None,
	LocalVideoMem,			// True local video memory
	LocalVideoMem3D,		// True local video memory
} MPRSurfaceType;

typedef struct MPRSurfaceDescription {
  UInt height;
  UInt width;
  UInt pitch;
  UInt RGBBitCount; 
  UInt RBitMask; 
  UInt GBitMask; 
  UInt BBitMask; 
} MPRSurfaceDescription;

typedef enum {
  HOOK_CREATESURFACE = 0,   
  HOOK_RELEASESURFACE,             
  HOOK_GETSURFACEDESRIPTION,       
  HOOK_LOCK,           
  HOOK_UNLOCK,          
  HOOK_BLT,        
  HOOK_BLTCHROMA,        
  HOOK_SETCHROMA,               
  HOOK_SWAPSURFACE,
  HOOK_SETWINDOWRECT,
  HOOK_RESTORESURFACE,
  HOOK_DEACTIVATESURFACE,
  
  MAXSURFACEHOOKS,
} mprSurfaceHookNames;

#define MPR_MAX_DEVICES 8

 typedef enum {
  MPR_PKT_POINTS = 1,           /* MUST BE SAME AS HOOK_POINTS		   */
  MPR_PKT_LINES,                /* MUST BE SAME AS HOOK_LINES          */
  MPR_PKT_POLYLINE,             /* MUST BE SAME AS HOOK_POLYLINE       */
  MPR_PKT_TRIANGLES,            /* MUST BE SAME AS HOOK_TRIANGLES      */
  MPR_PKT_TRISTRIP,             /* MUST BE SAME AS HOOK_TRISTRIP       */
  MPR_PKT_TRIFAN,               /* MUST BE SAME AS HOOK_TRIFAN         */

  MPR_PKT_ID_COUNT,             /* MUST BE LAST */
} MPRPacketID;

#define MPR_PRM_POINTS			MPR_PKT_POINTS
#define MPR_PRM_LINES           MPR_PKT_LINES
#define MPR_PRM_POLYLINE        MPR_PKT_POLYLINE 
#define MPR_PRM_TRIANGLES       MPR_PKT_TRIANGLES 
#define MPR_PRM_TRISTRIP        MPR_PKT_TRISTRIP
#define MPR_PRM_TRIFAN          MPR_PKT_TRIFAN

enum
{
	MPR_STA_NONE,
	MPR_STA_ENABLES,				// MPR_SE
	MPR_STA_DISABLES,				// MPR_SE

	MPR_STA_SRC_BLEND_FUNCTION,		// MPR_BF
	MPR_STA_DST_BLEND_FUNCTION,		// MPR_BF
	MPR_STA_COLOR_OP_FUNCTION,		// MPR_TO
	MPR_STA_COLOR_ARG1_FUNCTION,	// MPR_TO
	MPR_STA_COLOR_ARG2_FUNCTION,	// MPR_TO
	MPR_STA_ALPHA_OP_FUNCTION,		// MPR_TO
	MPR_STA_TEXTURE_FACTOR,			// DWORD RGBA

	MPR_STA_TEX_FILTER,				// MPR_TX

	MPR_STA_FOG_COLOR,
	MPR_STA_FG_COLOR,				// Long, RGBA or index
	MPR_STA_BG_COLOR,				// Long, RGBA or index

	MPR_STA_TEX_ID,					// Handle

	MPR_STA_SCISSOR_LEFT,  
	MPR_STA_SCISSOR_TOP,  
	MPR_STA_SCISSOR_RIGHT,			// Right, bottom, not inclusive
	MPR_STA_SCISSOR_BOTTOM,			// Validity check done here
};

typedef enum {
  MPR_MSG_NOP,
  MPR_MSG_CREATE_RC,
  MPR_MSG_DELETE_RC,
  MPR_MSG_GET_STATE,
  MPR_MSG_NEW_TEXTURE,
  MPR_MSG_LOAD_TEXTURE,
  MPR_MSG_FREE_TEXTURE,
  MPR_MSG_PACKET,
  MPR_MSG_BITMAP,
  MPR_MSG_NEW_TEXTURE_PAL,
  MPR_MSG_LOAD_TEXTURE_PAL,
  MPR_MSG_FREE_TEXTURE_PAL,
  MPR_MSG_ATTACH_TEXTURE_PAL,
  MPR_MSG_STORE_STATEID,
  MPR_MSG_GET_STATEID,
  MPR_MSG_FREE_STATEID,
  MPR_MSG_ATTACH_SURFACE,
  MPR_MSG_OPEN_DEVICE,
  MPR_MSG_CLOSE_DEVICE
} mprMsgID;

// Possible values for: MPR_STA_ENABLES & MPR_STA_DISABLES 
#define MPR_SE_SHADING							0x00000001L
#define MPR_SE_TEXTURING						0x00000002L
#define MPR_SE_MODULATION						0x00000004L
#define MPR_SE_Z_COMPARE						0x00000008L
#define MPR_SE_Z_MASKING						0x00000010L
#define MPR_SE_SCISSORING						0x00000020L
#define	MPR_SE_ALPHA							0x00000040L
#define MPR_SE_CHROMA							0x00000080L
#define MPR_SE_FILTERING						0x00000100L
#define MPR_SE_Z_WRITE							0x00000200L
#define MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE  0x00000400L

// Possible values for: MPR_STA_SRC_BLEND_FUNCTION & MPR_STA_DST_BLEND_FUNCTION
enum
{
	MPR_BF_ZERO=1,
	MPR_BF_ONE,
	MPR_BF_SRC,                    // Only MPR_STA_DST_BLEND_FUNCTION
	MPR_BF_SRC_INV,                // Only MPR_STA_DST_BLEND_FUNCTION
	MPR_BF_SRC_ALPHA,
	MPR_BF_SRC_ALPHA_INV,
	MPR_BF_DST_ALPHA,
	MPR_BF_DST_ALPHA_INV,
	MPR_BF_DST,                    // Only MPR_STA_SRC_BLEND_FUNCTION
	MPR_BF_DST_INV,                // Only MPR_STA_SRC_BLEND_FUNCTION
};

// Possible values for: MPR_STA_ALPHA_OP_FUNCTION && MPR_STA_TEXTURE_OP_FUNCTION
enum
{
	MPR_TO_DISABLE = 1,  
    MPR_TO_SELECTARG1,  
    MPR_TO_SELECTARG2,  
    MPR_TO_MODULATE,  
    MPR_TO_MODULATE2X,  
    MPR_TO_MODULATE4X,  
    MPR_TO_ADD,  
    MPR_TO_ADDSIGNED,  
    MPR_TO_ADDSIGNED2X,  
    MPR_TO_SUBTRACT,  
    MPR_TO_ADDSMOOTH,  
    MPR_TO_BLENDDIFFUSEALPHA,  
    MPR_TO_BLENDTEXTUREALPHA,  
    MPR_TO_BLENDFACTORALPHA,  
    MPR_TO_BLENDTEXTUREALPHAPM,  
    MPR_TO_BLENDCURRENTALPHA,  
    MPR_TO_PREMODULATE,  
    MPR_TO_MODULATEALPHA_ADDCOLOR,  
    MPR_TO_MODULATECOLOR_ADDALPHA,  
    MPR_TO_MODULATEINVALPHA_ADDCOLOR,  
    MPR_TO_MODULATEINVCOLOR_ADDALPHA,  
    MPR_TO_BUMPENVMAP, 
    MPR_TO_BUMPENVMAPLUMINANCE, 
    MPR_TO_DOTPRODUCT3,
    MPR_TO_FORCE_DWORD = 0x7fffffff,  
};

// Possible values for: MPR_STA_COLOR_ARG1_FUNCTION && MPR_STA_COLOR_ARG2_FUNCTION
enum
{
	MPR_TA_DIFFUSE,
	MPR_TA_D3DTA_CURRENT,
	MPR_TA_D3DTA_TEXTURE,
	MPR_TA_D3DTA_TFACTOR,
	MPR_TA_D3DTA_SPECULAR,
};

// Possible values for: MPR_STA_TEX_FILTER
enum {
  MPR_TX_NONE,
  MPR_TX_BILINEAR,              // interpolate 4 pixels
  MPR_TX_DITHER,                // Dither the colors
  MPR_TX_MIPMAP_NEAREST = 10,   // nearest mipmap
  MPR_TX_MIPMAP_LINEAR,         // interpolate between mipmaps
  MPR_TX_MIPMAP_BILINEAR,       // interpolate 4x within mipmap
  MPR_TX_MIPMAP_TRILINEAR,      // interpolate mipmaps,4 pixels
  MPR_TX_BILINEAR_NOCLAMP,      // interpolate 4 pixels, dont set clamp mode
};

// Possible values for : VtxInfo in the MPRPrimitive() prototype below
#define MPR_VI_COLOR                0x0002
#define MPR_VI_TEXTURE              0x0004  
    
// Possible values for: ClearInfo in the MPRClearBuffers() prototype below
#define MPR_CI_DRAW_BUFFER          0x0001
#define MPR_CI_ZBUFFER              0x0004  

// Possible values for: TexInfo in the MPRNewTexture() prototype below.
#define MPR_TI_DEFAULT              0x000000
#define MPR_TI_MIPMAP               0x000001
#define MPR_TI_CHROMAKEY            0x000020
#define MPR_TI_ALPHA	            0x000040
#define MPR_TI_PALETTE              0x000080
#define MPR_TI_DDS					0x000100
#define MPR_TI_DXT1					0x000200
#define MPR_TI_DXT3					0x000400
#define MPR_TI_DXT5					0x000800
#define MPR_TI_16					0x001000
#define MPR_TI_32					0x002000
#define MPR_TI_64					0x004000
#define MPR_TI_128					0x008000
#define MPR_TI_256					0x010000
#define MPR_TI_512					0x020000
#define MPR_TI_1024					0x040000
#define MPR_TI_2048					0x080000
#define MPR_TI_TERRAIN				0x100000
#define MPR_TI_RGB24				0x200000
#define MPR_TI_ARGB32				0x400000
#define MPR_TI_INVALID				0x800000

struct IDirectDraw7;
struct IDirect3D7;
struct IDirect3DDevice7;
struct IDirectDrawSurface7;
struct _D3DDeviceDesc7;
struct tagDDDEVICEIDENTIFIER2;
struct _DDCAPS_DX7;
struct _DDPIXELFORMAT;
struct IDirectDrawPalette;

class DXContext
{
public:
	DXContext();
	~DXContext();

	IDirectDraw7 *m_pDD;
	IDirect3D7 *m_pD3D;
	IDirect3DDevice7 *m_pD3DD;
	bool m_bFullscreen;
	HWND m_hWnd;
	int m_nWidth, m_nHeight;
	GUID m_guidDD;
	GUID m_guidD3D;

	enum D3DDeviceCategory
	{
		D3DDeviceCategory_Unknown,
		D3DDeviceCategory_Software,
		D3DDeviceCategory_Hardware,
		D3DDeviceCategory_Hardware_TNL
	} m_eDeviceCategory;

	struct _DDCAPS_DX7 *m_pcapsDD;
	struct _D3DDeviceDesc7 *m_pD3DHWDeviceDesc;
	struct tagDDDEVICEIDENTIFIER2 *m_pDevID;

public:
	bool Init(HWND hWnd,int nWidth,int nHeight,int nDepth,bool bFullscreen);
	bool SetRenderTarget(IDirectDrawSurface7 *pRenderTarget);
	void Shutdown();
	bool ValidateD3DDevice();
	DWORD TestCooperativeLevel();

	// JPO - use simple reference counting
	int AddRef()
	{
		return ++refcount;
	};

	int Release()
	{
		int rc = --refcount;
		if (refcount <= 0) delete this;
		return rc;
	};

protected:
	void EnumZBufferFormats(void *parr);
	static HRESULT _stdcall CALLBACK EnumZBufferFormatsCallback(struct _DDPIXELFORMAT *lpDDPixFmt, LPVOID lpContext);
	void AttachDepthBuffer(IDirectDrawSurface7 *p);
	void CheckCaps();
	int refcount;
};

class PaletteHandle;

class TextureHandle
{
public:
	TextureHandle();
	~TextureHandle();

	IDirectDrawSurface7 *m_pDDS;
	enum _D3DX_SURFACEFORMAT m_eSurfFmt;
	int m_nWidth;
	int m_nHeight;
	int m_nActualWidth;
	int m_nActualHeight;
	DWORD m_dwFlags;
	DWORD m_dwChromaKey;
	PaletteHandle *m_pPalAttach;
	BYTE *m_pImageData;					// Copy if palettized src image data if the device doesnt not support palettized textures
	bool m_bImageDataOwned;				// self allocated or not
	int m_nImageDataStride;

	enum _TextureHandleFlags
	{
		FLAG_HINT_DYNAMIC	= 0x1,
		FLAG_HINT_STATIC	= 0x2,
		FLAG_NOTMANAGED		= 0x4,		// dont use the texture manager
		FLAG_INLOCALVIDMEM	= 0x8,		// put it in videomemory
		FLAG_RENDERTARGET	= 0x10,		// can be used as 3d render target
		FLAG_MATCHPRIMARY	= 0x20,		// use same pixel format as primary surface 
	};

protected:
	enum _TEX_CAT
	{
		TEX_CAT_DEFAULT,
		TEX_CAT_CHROMA,
		TEX_CAT_ALPHA,
		TEX_CAT_CHROMA_ALPHA,
		TEX_CAT_RGB24,
		TEX_CAT_ARGB32,
		TEX_CAT_MAX
	};

	struct TEXTURESEARCHINFO
	{
		DWORD dwDesiredBPP;
		DWORD dwDesiredAlphaBPP;
		BOOL bUsePalette;
		BOOL bFoundGoodFormat;

		struct _DDPIXELFORMAT* pddpf;
	};

	static _DDPIXELFORMAT m_arrPF[TEX_CAT_MAX];

	// Warning: Not addref'd
	static IDirect3DDevice7 *m_pD3DD;

	static struct _D3DDeviceDesc7 *m_pD3DHWDeviceDesc;

#ifdef _DEBUG
public:
	static DWORD m_dwNumHandles;			// Number of instances
	static DWORD m_dwBitmapBytes;			// Bytes allocated for bitmap copies
	static DWORD m_dwTotalBytes;			// Total number of bytes allocated (including bitmap copies and object size)

protected:
	std::string m_strName;
#endif

public:
	bool Create(char *strName, UInt32 info, UInt16 bits, UInt16 width, UInt16 height, DWORD dwFlags = 0);
	bool Load(UInt16 mip, UInt chroma, UInt8 *TexBuffer, bool bDoNotLoadBits = false, bool bDoNotCopyBits = false, int nImageDataStride = - 1);
	bool Reload();
	IDirectDrawSurface7 *GetDDSurface() { return m_pDDS; }
	void PaletteAttach(PaletteHandle *p);
	void PaletteDetach(PaletteHandle *p);
	bool SetPriority(DWORD dwPrio);
	void Clear();
	void PreLoad();
	void RestoreAll();

protected:
	void ReportTextureLoadError(HRESULT hr, bool bDuringLoad = false);
	void ReportTextureLoadError(char *strReason);
	static HRESULT CALLBACK TextureSearchCallback(struct _DDPIXELFORMAT* pddpf, VOID* param);

#ifdef _DEBUG
public:
	static void MemoryUsageReport();
#endif

public:
	static void StaticInit(IDirect3DDevice7 *pD3DD);
	static void StaticCleanup();
};

class PaletteHandle
{
public:
	PaletteHandle(IDirectDraw7 *pDD, UInt16 PalBitsPerEntry, UInt16 PalNumEntries);
	~PaletteHandle();

	IDirectDrawPalette *m_pIDDP;
	std::vector<TextureHandle *> m_arrAttachedTextures;
	short m_nNumEntries;
	DWORD *m_pPalData;

#ifdef _DEBUG
public:
	static DWORD m_dwNumHandles;			// Number of instances
	static DWORD m_dwTotalBytes;			// Total number of bytes allocated (including bitmap copies and object size)
#endif

	void Load(UInt16 info, UInt16 PalBitsPerEntry, UInt16 index, UInt16 entries,UInt8 *PalBuffer);
	void AttachToTexture(TextureHandle *pTex);
	void DetachFromTexture(TextureHandle *pTex);

protected:
	std::vector<TextureHandle *>::iterator GetAttachedTextureIndex(TextureHandle *pTex);

#ifdef _DEBUG
public:
	static void MemoryUsageReport();
#endif
};

struct IDirect3DDevice7;
struct IDirect3D7;
struct IDirectDraw7;
struct IDirectDrawSurface7;
struct IDirect3DDevice7;
struct IDirect3DVertexBuffer7;
struct _DDSURFACEDESC2;

class ImageBuffer;
class DXContext;
struct Poly;
struct Ptexcoord;
struct Tpoint;

enum PRIM_COLOR_OP
{
	PRIM_COLOP_NONE			= 0,
	PRIM_COLOP_COLOR		= 1,
	PRIM_COLOP_INTENSITY	= 2,
	PRIM_COLOP_TEXTURE		= 4,
};

// values for SetupMPRState flag argument
#define	CHECK_PREVIOUS_STATE 0x01

enum {
	STATE_SOLID = 0,
	STATE_LIT,
    STATE_GOURAUD,

    STATE_TEXTURE,
    STATE_TEXTURE_PERSPECTIVE,
	STATE_TEXTURE_LIT,
    STATE_TEXTURE_LIT_PERSPECTIVE,
	STATE_TEXTURE_SMOOTH,
	STATE_TEXTURE_SMOOTH_PERSPECTIVE,
	STATE_TEXTURE_GOURAUD,
    STATE_TEXTURE_GOURAUD_PERSPECTIVE,

	STATE_TEXTURE_NOFILTER,
	STATE_TEXTURE_NOFILTER_PERSPECTIVE,

	STATE_TEXTURE_TEXT,

	STATE_ALPHA_SOLID,
	STATE_ALPHA_LIT,
    STATE_ALPHA_GOURAUD,
 
	STATE_CHROMA_TEXTURE,
	STATE_CHROMA_TEXTURE_PERSPECTIVE,
	STATE_CHROMA_TEXTURE_LIT,
	STATE_CHROMA_TEXTURE_LIT_PERSPECTIVE,
	STATE_CHROMA_TEXTURE_GOURAUD,
	STATE_CHROMA_TEXTURE_GOURAUD_PERSPECTIVE,

	STATE_ALPHA_TEXTURE,
    STATE_ALPHA_TEXTURE_PERSPECTIVE,
	STATE_ALPHA_TEXTURE_LIT,
    STATE_ALPHA_TEXTURE_LIT_PERSPECTIVE,
	STATE_ALPHA_TEXTURE_SMOOTH,
    STATE_ALPHA_TEXTURE_SMOOTH_PERSPECTIVE,
	STATE_ALPHA_TEXTURE_GOURAUD,
    STATE_ALPHA_TEXTURE_GOURAUD_PERSPECTIVE,

	STATE_ALPHA_TEXTURE_NOFILTER,
    STATE_ALPHA_TEXTURE_NOFILTER_PERSPECTIVE,

	STATE_ALPHA_TEXTURE_PERSPECTIVE_CLAMP,

	STATE_TEXTURE_TERRAIN,
	STATE_TEXTURE_TERRAIN_NIGHT,

	MAXIMUM_MPR_STATE = 36					
};


#if (MAXIMUM_MPR_STATE >= 64)
#error "Can have at most 64 prestored states (#define'd inside MPR) & we need one free"
#endif

struct TLVERTEX
{
	float		sx; 
	float		sy; 
	float		sz; 
	float		rhw; 
	DWORD		color;
	DWORD		specular;
	float		tu0;
	float		tv0;
	float		tu1;
	float		tv1;
}; 

class BVertex
{
public:
	float		sx;
	float		sy;
	float		sz;
	float		rhw;
	DWORD		color;
	DWORD		specular;
	float		tu0;
	float		tv0;
	float		tu1;
	float		tv1;

	BVertex		*pNext;
};

class BPolygon
{
public:
	BPolygon	*pNext;
	BVertex		*pVertexList;

	DWORD		numVertices;
	int			renderState;
	GLint		textureID0;
	GLint		textureID1;
	DWORD		zBuffer;

public:
	void CalcPolyZ();
	TLVERTEX* CopyToVertexBuffer(TLVERTEX *bufferPos);
};

class ContextMPR
{
public:
	ContextMPR();
	virtual ~ContextMPR();

	BOOL Setup(ImageBuffer *pIB, DXContext *c);
	void Cleanup( void );
	void NewImageBuffer(UInt lpDDSBack);

	void SetView(LPD3DMATRIX l_pMV);
	void SetWorld(LPD3DMATRIX l_pMW);
	void SetProjection(LPD3DMATRIX l_pMP);

	void Wipe(DWORD Color);
//	void SetState(WORD State,DWORD Value);
	void SetState(WORD State,DWORD Value,short state=0);
	void SetStateInternal(WORD State,DWORD Value);
	void ClearBuffers(WORD ClearInfo);
	void SwapBuffers(WORD SwapInfo);
	void StartFrame(void);
	void FinishFrame(void *lpFnPtr);
	void SetColorCorrection(DWORD color,float percent);
	void SetupMPRState(GLint flag=0);
	void SetFog(DWORD color);
	void SelectForegroundColor (GLint color);
	void SelectBackgroundColor (GLint color);
	void SelectTexture1(GLint texID);
	void SelectTexture2(GLint texID);
	void SetTexture1(GLint texID);
	void SetTexture2(GLint texID);
	void RestoreState(GLint state);
	void ApplyStateBlock(GLint state);
	void UpdateSpecularFog(DWORD specular);
	void SetZBuffering(BOOL state);
	void SetNVGmode(BOOL state);
	void SetTVmode(BOOL state);
	void SetIRmode(BOOL state);
	void SetPalID(int id);
	void SetTexID(int id);
	void ToggleZBuffering();
	void setGlobalZBias(float zBias);
	void InvalidateState () { currentTexture1 = currentTexture2 = -1; m_colFG_Raw = 0x00ffffff; currentState = -1; };
	int CurrentForegroundColor(void) {return m_colFG_Raw;};
	void Render2DBitmap( int sX, int sY, int dX, int dY, int w, int h, int totalWidth, DWORD *source );

public:
	static GLint StateSetupCounter;

	class State
	{
	public:
		State() { ZeroMemory(this, sizeof(*this)); }

		bool SE_SHADING;
		bool SE_TEXTURING;
		bool SE_MODULATION;
		bool SE_Z_BUFFERING;
		bool SE_Z_WRITE;
		bool SE_PIXEL_MASKING;
		bool SE_Z_MASKING;
		bool SE_PATTERNING;
		bool SE_SCISSORING;
		bool SE_CLIPPING;
		bool SE_BLENDING;
		bool SE_FILTERING;
		bool SE_ALPHA;
		bool SE_NON_PERSPECTIVE_CORRECTION_MODE;
		bool SE_NON_SUBPIXEL_PRECISION_MODE;
		bool SE_HARDWARE_OFF;
		bool SE_PALETTIZED_TEXTURES;
		bool SE_DECAL_TEXTURES;
		bool SE_ANTIALIASING;
		bool SE_DITHERING;
	};

	static UInt StateTable[MAXIMUM_MPR_STATE];
	static State StateTableInternal[MAXIMUM_MPR_STATE];
	bool m_bUseSetStateInternal;

	GLint m_colFG_Raw;
	GLint m_colBG_Raw;
	GLint currentState;
	GLint lastState;
	GLint currentTexture1;
	GLint currentTexture2;
	GLint lastTexture1;
	GLint lastTexture2;
	BOOL NVGmode;
	BOOL TVmode;
	BOOL IRmode;
	int palID;
	int texID;

	D3DMATRIX mV;
	D3DMATRIX mP;
	D3DMATRIX mW[4096];

	float ZFAR;
	float ZNEAR;
	float gZBias;
	BOOL bZBuffering;

protected:
	inline void SetStateTable (GLint state, GLint flag);
	inline void ClearStateTable (GLint state);
	void SetCurrentState (GLint state, GLint flag);
	void SetCurrentStateInternal(GLint state, GLint flag);
	void CleanupMPRState (GLint flag=0);
	inline void SetPrimitiveType(int nType);
	inline void AllocatePolygon(BPolygon *&curPoly, const DWORD numVertices);
	inline void AddPolygon(BPolygon *&polyList, BPolygon *&curPoly);
	void RenderPolyList(BPolygon *&pHead);

public:
	DXContext *m_pCtxDX;

protected:
	int m_nFrameDepth;						// EndScene is called when this value reaches zero
	ImageBuffer *m_pIB;
	IDirectDraw7 *m_pDD;
	IDirect3DDevice7 *m_pD3DD;
	IDirect3DVertexBuffer7 *m_pVB;
	IDirect3DVertexBuffer7 *m_pVBH;
	IDirect3DVertexBuffer7 *m_pVBB;
	IDirectDrawSurface7 *m_pRenderTarget;
	DWORD m_spec;
	DWORD m_colFG;
	DWORD m_colBG;
	DWORD m_colFOG;
	RECT m_rcVP;
	bool m_bEnableScissors;
	DWORD m_dwVBSize;
	WORD *m_pIdx;
	DWORD m_dwNumVtx;
	DWORD m_dwNumIdx;
	DWORD m_dwStartVtx;
	DWORD m_dwStartIdx;
	short m_nCurPrimType;
	TLVERTEX *m_pTLVtx;
	int mIdx;
	DWORD terrainPolyVCnt;
	DWORD translucentPolyVCnt;
	BPolygon *terrainPolys;
	BPolygon *translucentPolys;
	alloc_handle_t *memPool;

#ifdef _DEBUG
	BYTE *m_pVtxEnd;
#endif

	IDirectDrawSurface7 *m_pDDSP;
	bool m_bNoD3DStatsAvail;				// D3D Texture management stats (dx debug runtime only)
	bool m_bRenderTargetHasZBuffer;
	bool m_bViewportLocked;

	class Stats
	{
	public:
		Stats();

	public:
		DWORD dwLastFPS;
		DWORD dwMinFPS;
		DWORD dwMaxFPS;
		DWORD dwAverageFPS;
		DWORD dwTotalPrimitives;
		DWORD arrPrimitives[6];
		DWORD dwMaxVtxBatchSize;
		DWORD dwAvgVtxBatchSize;
		DWORD dwMaxPrimBatchSize;
		DWORD dwAvgPrimBatchSize;
		DWORD dwMaxVtxCountPerSecond;
		DWORD dwAvgVtxCountPerSecond;
		DWORD dwMaxPrimCountPerSecond;
		DWORD dwAvgPrimCountPerSecond;
		DWORD dwPutTextureTotal;
		DWORD dwPutTextureCached;

	protected:
		DWORD dwTicks;
		DWORD dwTotalSeconds;
		DWORD dwCurrentFPS;
		DWORD dwTotalFPS;
		__int64 dwTotalVtxCount;
		__int64 dwTotalPrimCount;
		__int64 dwTotalVtxBatchSize;
		__int64 dwTotalPrimBatchSize;
		DWORD dwTotalBatches;
		DWORD dwCurVtxBatchSize;
		DWORD dwCurPrimBatchSize;
		DWORD dwCurVtxCountPerSecond;
		DWORD dwCurPrimCountPerSecond;

	protected:
		void Check();

	public:
		void Init();
		void StartFrame();
		void StartBatch();
		void Primitive(DWORD dwType, DWORD dwNumVtx);
		void PutTexture(bool bCached);
		void Report();
	};

	Stats m_stats;

	inline DWORD MPRColor2D3DRGBA(GLint color);
	inline void UpdateViewport();
	inline bool LockVB(int nVtxCount, void **p);
	inline void UnlockVB();
	inline void FlushVB();
	static HRESULT WINAPI EnumSurfacesCB2(IDirectDrawSurface7 *lpDDSurface, struct _DDSURFACEDESC2 *lpDDSurfaceDesc, LPVOID lpContext);
	void Stats();

public:
	void DrawPoly(DWORD opFlag, Poly *poly, int *xyzIdxPtr, int *rgbaIdxPtr, int *IIdxPtr, Ptexcoord *uv, bool bUseFGColor = false);
	void Draw2DPoint(Tpoint *v0);
	void Draw2DPoint(float x, float y);
	void Draw2DLine(Tpoint *v0, Tpoint *v1);
	void Draw2DLine(float x0, float y0, float x1, float y1);
	void DrawPrimitive2D(int type, int nVerts, int *xyzIdxPtr);
	void DrawPrimitive(int type, WORD VtxInfo, WORD Count, MPRVtx_t *data, WORD Stride);
	void DrawPrimitive(int type, WORD VtxInfo, WORD Count, MPRVtxTexClr_t *data, WORD Stride);
//	void DrawPrimitive(int type, WORD VtxInfo, WORD Count, MPRVtxTexClr_t **data);
	void DrawPrimitive(int type, WORD VtxInfo, WORD Count, MPRVtxTexClr_t **data, bool terrain=false);
	void TextOut(short x, short y, DWORD col, LPSTR str);
	void SetViewportAbs(int nLeft, int nTop, int nRight, int nBottom);
	void LockViewport();
	void UnlockViewport();
	void GetViewport(RECT *prc);
	void FlushPolyLists();
};

#ifdef  __cplusplus
};
#endif

#endif // _3DEJ_CONTEXT_H_
