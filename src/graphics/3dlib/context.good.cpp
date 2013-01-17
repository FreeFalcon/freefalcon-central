/***************************************************************************\
    Context.cpp
    Scott Randolph
	April 29, 1996

    //JAM 06Oct03 - Begin Major Rewrite
\***************************************************************************/
#include "stdafx.h"
#include "ImageBuf.h"
#include "context.h"
#include "polylib.h"
#include "StateStack.h"
#include "render3d.h"
#include "alloc.h"
#include "radix.h"
#include "graphics\include\texbank.h"
#include "graphics\include\fartex.h"
#include "graphics\include\terrtex.h"
#include "FalcLib\include\playerop.h"
#include "FalcLib\include\dispopts.h"

extern bool g_bSlowButSafe;
extern float g_fMipLodBias;

#define	INT3 _asm {int 3}

#ifdef _DEBUG

#define _CONTEXT_ENABLE_STATS
//#define _CONTEXT_RECORD_USED_STATES
//#define _CONTEXT_ENABLE_RENDERSTATE_HIGHLIGHT
//#define _CONTEXT_ENABLE_RENDERSTATE_HIGHLIGHT_REPLACE
//define _CONTEXT_FLUSH_EVERY_PRIMITIVE
//#define _CONTEXT_TRACE_ALL
#define _CONTEXT_USE_MANAGED_TEXTURES
//#define _VALIDATE_DEVICE

static int m_nInstCount = 0;

#ifdef _CONTEXT_RECORD_USED_STATES
#include <set>
static std::set<int> m_setStatesUsed;
#endif

#ifdef _CONTEXT_ENABLE_RENDERSTATE_HIGHLIGHT_REPLACE
static bool bEnableRenderStateHighlightReplace = false;
static int bRenderStateHighlightReplaceTargetState = 0;
#endif

#endif //_DEBUG

UInt ContextMPR::StateTable[MAXIMUM_MPR_STATE];
ContextMPR::State ContextMPR::StateTableInternal[MAXIMUM_MPR_STATE];
int ContextMPR::StateSetupCounter = 0;


// COBRA - RED - This function is to calculate some CXs used in Drawing functions
// They are calcualted on change o gZBias by 'ContextMPR::setGlobalZBias()'
inline void ContextMPR::ZCX_Calculate(void)
{
	zNear = ZNEAR + gZBias;
	szCX1=ZFAR/(ZFAR-zNear);
	szCX2=ZFAR*zNear/(zNear-ZFAR);
}

// Macro to use CXes
#define	SCALE_SZ(x)	(szCX1+szCX2/x)

// COBRA - RED - End



ContextMPR::ContextMPR()
{ 
	#ifdef _DEBUG
	m_nInstCount++;
	#endif

	m_pCtxDX = NULL;
	m_pDD = NULL;
	m_pD3DD = NULL;
	m_pVB = NULL;
	m_pVBH = NULL;
	m_pVBB = NULL;
	m_dwVBSize = 0;
	m_pIdx = NULL;
	m_dwNumVtx = 0;
	m_dwNumIdx = 0;
	m_dwStartVtx = 0;
	m_nCurPrimType = 0;
	m_pRenderTarget = NULL;
	m_bEnableScissors = false;
	m_pDDSP = NULL;
	m_bNoD3DStatsAvail = false;
	m_bUseSetStateInternal = false;
	m_pIB = NULL;
	m_nFrameDepth = 0;
	m_pTLVtx = NULL;
	m_bRenderTargetHasZBuffer = false;
	m_bViewportLocked = false;
	m_colFG = m_colBG = 0;
	m_colFG_Raw = m_colBG_Raw = 0;
	bZBuffering = false;
	gZBias = 0.f;
	ZFAR = 280000.f;
	ZNEAR = 1.f;
	NVGmode = 0;
	TVmode = 0;
	IRmode = 0;
	palID = 0;
	texID = 0;

	ZCX_Calculate();				// COBRA - RED - Drawing CXs update

	#ifdef _DEBUG
	m_pVtxEnd = NULL;
	#endif
}

ContextMPR::~ContextMPR()
{ 
	#ifdef _DEBUG
	m_nInstCount--;
	#endif
}

BOOL ContextMPR::Setup(ImageBuffer *pIB,DXContext *c)
{
	BOOL bRetval = FALSE;

	#ifdef _CONTEXT_TRACE_ALL
	MonoPrint("ContextMPR::Setup(0x%X,0x%X)\n",pIB,c);
	#endif

	try
	{
		m_pCtxDX = c;

		if(!m_pCtxDX)
		{
			ShiWarning("Failed to create device!");
			return FALSE;
		}

		ShiAssert(m_pTLVtx == NULL);

		ShiAssert(pIB);

		if(!pIB) return FALSE;

		m_pIB = pIB;
		IDirectDrawSurface7 *lpDDSBack = pIB->targetSurface();
		NewImageBuffer((UInt)lpDDSBack);

		m_pDD = m_pCtxDX->m_pDD;
		m_pD3DD = m_pCtxDX->m_pD3DD;

		// Setup the vertex buffer
		IDirect3DVertexBuffer7Ptr p;
		D3DVERTEXBUFFERDESC vbdesc;
		ZeroMemory(&vbdesc,sizeof(vbdesc));
		vbdesc.dwSize = sizeof(vbdesc);
		vbdesc.dwFVF = D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX2 | D3DFVF_SPECULAR;
		vbdesc.dwCaps = D3DVBCAPS_WRITEONLY | D3DVBCAPS_DONOTCLIP | D3DVBCAPS_SYSTEMMEMORY;

		m_dwVBSize = 1024;
		vbdesc.dwNumVertices = m_dwVBSize;

		CheckHR(m_pCtxDX->m_pD3D->CreateVertexBuffer(&vbdesc,&m_pVB,NULL));
		CheckHR(m_pCtxDX->m_pD3D->CreateVertexBuffer(&vbdesc,&m_pVBB,NULL));

		m_pIdx = new WORD[vbdesc.dwNumVertices * 3];
		if(!m_pIdx) throw _com_error(E_OUTOFMEMORY);

		// Setup our set of cached rendering states
		SetupMPRState(CHECK_PREVIOUS_STATE);

		// Initialise render states
		m_pD3DD->SetRenderState(D3DRENDERSTATE_COLORVERTEX,TRUE);
		m_pD3DD->SetRenderState(D3DRENDERSTATE_LIGHTING,FALSE);
		m_pD3DD->SetRenderState(D3DRENDERSTATE_CULLMODE,D3DCULL_NONE);
		m_pD3DD->SetRenderState(D3DRENDERSTATE_FOGENABLE,TRUE);
		m_pD3DD->SetRenderState(D3DRENDERSTATE_TEXTUREPERSPECTIVE,TRUE);
		m_pD3DD->SetRenderState(D3DRENDERSTATE_STIPPLEDALPHA,FALSE);
		m_pD3DD->SetRenderState(D3DRENDERSTATE_COLORKEYENABLE,FALSE);
		m_pD3DD->SetRenderState(D3DRENDERSTATE_STENCILENABLE,FALSE);
		m_pD3DD->SetRenderState(D3DRENDERSTATE_DITHERENABLE,FALSE);
		m_pD3DD->SetRenderState(D3DRENDERSTATE_CLIPPING,FALSE);
		m_pD3DD->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE,FALSE);
		m_pD3DD->SetRenderState(D3DRENDERSTATE_ZFUNC,D3DCMP_ALWAYS); 

		// Disable all stages
		for(int i = 0; i < 8; i++)
		{
			m_pD3DD->SetTextureStageState(i,D3DTSS_COLOROP,D3DTOP_DISABLE);
			m_pD3DD->SetTextureStageState(i,D3DTSS_ALPHAOP,D3DTOP_DISABLE);
		}

		// Setup stage 0
		m_pD3DD->SetTextureStageState(0,D3DTSS_COLORARG2,D3DTA_TEXTURE);
		m_pD3DD->SetTextureStageState(0,D3DTSS_COLORARG2,D3DTA_CURRENT);
		m_pD3DD->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE);
		m_pD3DD->SetTextureStageState(0,D3DTSS_ALPHAARG2,D3DTA_CURRENT);

		// Set Mipmap LOD Bias
		if(DisplayOptions.bMipmapping)
			m_pD3DD->SetTextureStageState(0,D3DTSS_MIPMAPLODBIAS,*((LPDWORD)(&g_fMipLodBias)));

		// Initialize our poly, mem, and radix buckets
		mIdx = 0;
		plainPolys = texturedPolys = translucentPolys = NULL;
		plainPolyVCnt = texturedPolyVCnt = translucentPolyVCnt = 0;
		currentState = lastState = currentTexture1 = currentTexture2 = lastTexture1 = lastTexture2 = -1;
		memPool = AllocInit();
		RadixReset();

		InvalidateState();
		RestoreState(STATE_SOLID);
		ZeroMemory(&m_rcVP,sizeof(m_rcVP));
		m_bViewportLocked = false;

		#ifdef _CONTEXT_ENABLE_STATS
		m_stats.Init();
		m_stats.StartBatch();
		#endif

		bRetval = TRUE;
	}

	catch(_com_error e)
	{
		MonoPrint("ContextMPR::Setup - Error 0x%X\n",e.Error());
	}

	return bRetval;
}

void ContextMPR::Cleanup()
{
	#ifdef _CONTEXT_TRACE_ALL
	MonoPrint("ContextMPR::Cleanup()\n");
	#endif

	#ifdef _DEBUG
	#ifdef _CONTEXT_ENABLE_STATS
	m_stats.Report();
	#endif
	#endif

	if(StateSetupCounter)
		CleanupMPRState(CHECK_PREVIOUS_STATE);

// Warning: The SIM code uses a shared DXContext which might be already toast when this function gets called!!
// Under no circumstances access m_pCtxDX here
// Btw: this was causing the infamous LGB CTD

	m_pCtxDX = NULL;

	if(m_pVB)
	{
		m_pVB->Release();
		m_pVB = NULL;
	}

	if(m_pVBB)
	{
		m_pVBB->Release();
		m_pVBB = NULL;
	}

	if(m_pIdx)
	{
		delete[] m_pIdx;
		m_pIdx = NULL;
	}

	m_pIdx = NULL;
	m_dwNumVtx = 0;
	m_dwNumIdx = 0;
	m_dwStartVtx = 0;
	m_nCurPrimType = 0;
	m_pIB = NULL;
	m_nFrameDepth = 0;
	m_pTLVtx = NULL;
	m_bRenderTargetHasZBuffer = false;

	#ifdef _DEBUG
	m_pVtxEnd = NULL;
	#endif

	#ifdef _CONTEXT_RECORD_USED_STATES
	MonoPrint("ContextMPR::Cleanup - Report of used states follows\n	");
	std::set<int>::iterator it;
	for(it = m_setStatesUsed.begin(); it != m_setStatesUsed.end(); it++)
		MonoPrint("%d,",*it);
	m_setStatesUsed.clear();
	MonoPrint("\nContextMPR::Cleanup - End of report\n	");
	#endif
}

void ContextMPR::NewImageBuffer( UInt lpDDSBack)
{
	#ifdef _CONTEXT_TRACE_ALL
	MonoPrint("ContextMPR::NewImageBuffer(0x%X)\n",lpDDSBack);
	#endif

	if(m_pRenderTarget)
		m_pRenderTarget = NULL;

	m_pRenderTarget = (IDirectDrawSurface7 *)lpDDSBack;

	// Some drivers (like the 3.68 detonators) implicitly create Z buffers
	if(m_pRenderTarget)
	{
		IDirectDrawSurface7Ptr pDDS;

		DDSCAPS2 ddscaps;
		ZeroMemory(&ddscaps,sizeof(ddscaps));
		ddscaps.dwCaps = DDSCAPS_ZBUFFER; 
		m_bRenderTargetHasZBuffer = SUCCEEDED(m_pRenderTarget->GetAttachedSurface(&ddscaps,&pDDS)); 
	}
}

void ContextMPR::ClearBuffers(WORD ClearInfo) 
{
	#ifdef _CONTEXT_TRACE_ALL
	MonoPrint("ContextMPR::ClearBuffers(0x%X)\n",ClearInfo);
	#endif

	DWORD dwClearFlags = 0;
	if(ClearInfo & MPR_CI_DRAW_BUFFER) dwClearFlags |= D3DCLEAR_TARGET;
	if(ClearInfo & MPR_CI_ZBUFFER) dwClearFlags |= D3DCLEAR_ZBUFFER;

	HRESULT hr = m_pD3DD->Clear(NULL,NULL,dwClearFlags,m_colBG,1.0f,NULL);
	ShiAssert(SUCCEEDED(hr));
}

void ContextMPR::StartFrame(void)
{
	#ifdef _CONTEXT_TRACE_ALL
	MonoPrint("ContextMPR::StartFrame()\n");
	#endif

	// Returns false if render target unchanged
	if(m_pCtxDX->SetRenderTarget(m_pRenderTarget))
		UpdateViewport();

	InvalidateState();

	currentState = lastState = currentTexture1 = currentTexture2 = lastTexture1 = lastTexture2 = -1;

	HRESULT hr;
	
	hr = m_pD3DD->Clear(NULL,NULL,D3DCLEAR_ZBUFFER,0,1.f,NULL);

	hr = m_pD3DD->BeginScene();

	if(FAILED(hr))
	{
		MonoPrint("ContextMPR::FinishFrame - BeginScene failed 0x%X\n",hr);

		if(hr == DDERR_SURFACELOST)
		{
			MonoPrint("ContextMPR::StartFrame - Restoring all surfaces\n",hr);

			TheTextureBank.RestoreAll();
			TheTerrTextures.RestoreAll();
			TheFarTextures.RestoreAll();

			hr = m_pD3DD->EndScene();

			if(FAILED(hr))
			{
				MonoPrint("ContextMPR::StartFrame - Retry for BeginScene failed 0x%X\n",hr);
				return;
			}
		}
	}

	#if defined _DEBUG && defined _CONTEXT_ENABLE_RENDERSTATE_HIGHLIGHT_REPLACE
	if(bEnableRenderStateHighlightReplace)
	{
		Sleep(1000);
		if(GetKeyState(VK_F4) & ~1)
			DebugBreak();
		bRenderStateHighlightReplaceTargetState++;
	}
	#endif
}

void ContextMPR::FinishFrame(void *lpFnPtr)
{
	#ifdef _CONTEXT_TRACE_ALL
	MonoPrint("ContextMPR::FinishFrame(0x%X)\n",lpFnPtr);
	#endif

	FlushVB();

	HRESULT hr = m_pD3DD->EndScene();
	if(FAILED(hr))
	{
		MonoPrint("ContextMPR::FinishFrame - EndScene failed 0x%X\n",hr);

		if(hr == DDERR_SURFACELOST)
		{
			MonoPrint("ContextMPR::FinishFrame - Restoring all surfaces\n",hr);

			TheTextureBank.RestoreAll();
			TheTerrTextures.RestoreAll();
			TheFarTextures.RestoreAll();

			hr = m_pD3DD->EndScene();

			if(FAILED(hr))
			{
				MonoPrint("ContextMPR::FinishFrame - Retry for EndScene failed 0x%X\n",hr);
				return;
			}
		}
	}

	#ifdef _CONTEXT_ENABLE_STATS
	m_stats.StartFrame();
	#endif

	ShiAssert(lpFnPtr == NULL);
	Stats();
}

void ContextMPR::SetState(WORD State, DWORD Value)
{
	if(m_bUseSetStateInternal)
	{
		SetStateInternal(State,Value);
		return;
	}

	#ifdef _CONTEXT_TRACE_ALL
	MonoPrint("ContextMPR::SetState(%d,0x%X)\n",State,Value);
	#endif
	ShiAssert(FALSE == F4IsBadReadPtr(m_pD3DD,sizeof *m_pD3DD));

	if (!m_pD3DD)
		return;

	switch(State)
	{
		case MPR_STA_ENABLES:
		{
			if(Value & MPR_SE_MODULATION)
			{
				FlushVB();

				m_pD3DD->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_MODULATE);
			}

			if(Value & MPR_SE_ALPHA)
			{
				FlushVB();

				if(m_pCtxDX->m_pD3DHWDeviceDesc->dpcTriCaps.dwAlphaCmpCaps & D3DCMP_GREATEREQUAL)
				{
					m_pD3DD->SetRenderState(D3DRENDERSTATE_ALPHATESTENABLE,TRUE);
					m_pD3DD->SetRenderState(D3DRENDERSTATE_ALPHAREF,(DWORD)1);
					m_pD3DD->SetRenderState(D3DRENDERSTATE_ALPHAFUNC,D3DCMP_GREATEREQUAL);
				}

				m_pD3DD->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE,TRUE);
				m_pD3DD->SetRenderState(D3DRENDERSTATE_SRCBLEND,D3DBLEND_SRCALPHA);
				m_pD3DD->SetRenderState(D3DRENDERSTATE_DESTBLEND,D3DBLEND_INVSRCALPHA);
				m_pD3DD->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_MODULATE);
			}

			if(Value & MPR_SE_CHROMA)
			{
				FlushVB();

				if(m_pCtxDX->m_pD3DHWDeviceDesc->dpcTriCaps.dwAlphaCmpCaps & D3DCMP_GREATEREQUAL)
				{
					m_pD3DD->SetRenderState(D3DRENDERSTATE_ALPHATESTENABLE,TRUE);
					m_pD3DD->SetRenderState(D3DRENDERSTATE_ALPHAREF,(DWORD)1);
					m_pD3DD->SetRenderState(D3DRENDERSTATE_ALPHAFUNC,D3DCMP_GREATEREQUAL);
				}

				m_pD3DD->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE,TRUE);
				m_pD3DD->SetRenderState(D3DRENDERSTATE_SRCBLEND,D3DBLEND_SRCALPHA);
				m_pD3DD->SetRenderState(D3DRENDERSTATE_DESTBLEND,D3DBLEND_INVSRCALPHA);
				m_pD3DD->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);
			}

			if(Value & MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE)
			{
				FlushVB();

				m_pD3DD->SetRenderState(D3DRENDERSTATE_TEXTUREPERSPECTIVE,FALSE);
			}

			if(Value & MPR_SE_SHADING)
			{
				FlushVB();

				m_pD3DD->SetRenderState(D3DRENDERSTATE_SHADEMODE,D3DSHADE_GOURAUD);
			}

			if((Value & MPR_SE_SCISSORING) && !m_bEnableScissors)
			{
				FlushVB();

				m_bEnableScissors = true;
				UpdateViewport();
			}

			if(Value & MPR_SE_Z_BUFFERING)
			{
				FlushVB();

				m_pD3DD->SetRenderState(D3DRENDERSTATE_ZFUNC,D3DCMP_LESSEQUAL); 
			}

			if(Value & MPR_SE_Z_WRITE)
			{
				FlushVB();

				m_pD3DD->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE,TRUE);
			}

			break;
		}

		case MPR_STA_DISABLES:
		{
			if(Value & MPR_SE_MODULATION)
			{
				FlushVB();

				m_pD3DD->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1);
			}

			if(Value & MPR_SE_TEXTURING)
			{
				FlushVB();

				m_pD3DD->SetTexture(0,NULL);
			}

			if(Value & MPR_SE_SHADING)
			{
				FlushVB();

				m_pD3DD->SetRenderState(D3DRENDERSTATE_SHADEMODE,D3DSHADE_FLAT);
			}

			if(Value & MPR_SE_FILTERING)
			{
				FlushVB();

				m_pD3DD->SetTextureStageState(0,D3DTSS_MAGFILTER,D3DTFG_POINT);
				m_pD3DD->SetTextureStageState(0,D3DTSS_MINFILTER,D3DTFN_POINT);
			}

			if(Value & MPR_SE_ALPHA)
			{
				FlushVB();

				m_pD3DD->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE,FALSE);
				m_pD3DD->SetRenderState(D3DRENDERSTATE_ALPHATESTENABLE,FALSE);
				m_pD3DD->SetRenderState(D3DRENDERSTATE_ALPHAFUNC,D3DCMP_ALWAYS);
				m_pD3DD->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);
			}

			if(Value & MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE)
			{
				FlushVB();

				m_pD3DD->SetRenderState(D3DRENDERSTATE_TEXTUREPERSPECTIVE,TRUE);
			}

			if((Value & MPR_SE_SCISSORING) && m_bEnableScissors)
			{
				FlushVB();

				m_bEnableScissors = false;
				UpdateViewport();
			}

			if(Value & MPR_SE_Z_BUFFERING)
			{
				FlushVB();

				m_pD3DD->SetRenderState(D3DRENDERSTATE_ZFUNC,D3DCMP_ALWAYS);
			}

			if(Value & MPR_SE_Z_WRITE)
			{
				FlushVB();

				m_pD3DD->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE,FALSE);
			}

			break;
		}

		case MPR_STA_SRC_BLEND_FUNCTION:
		{
			FlushVB();

			m_pD3DD->SetRenderState(D3DRENDERSTATE_SRCBLEND,Value);

			break;
		}

		case MPR_STA_DST_BLEND_FUNCTION:
		{
			FlushVB();

			m_pD3DD->SetRenderState(D3DRENDERSTATE_DESTBLEND,Value);

			break;
		}

		case MPR_STA_ALPHA_OP_FUNCTION:
		{
			FlushVB();

			m_pD3DD->SetTextureStageState(0,D3DTSS_ALPHAOP,Value);

			break;
		}

		case MPR_STA_COLOR_OP_FUNCTION:
		{
			FlushVB();

			m_pD3DD->SetTextureStageState(0,D3DTSS_COLOROP,Value);

			break;
		}

		case MPR_STA_TEXTURE_FACTOR:
		{
			FlushVB();

			m_pD3DD->SetRenderState(D3DRENDERSTATE_TEXTUREFACTOR,Value);

			break;
		}
		
		case MPR_STA_TEX_FILTER:
		{
			FlushVB();

			switch(Value)
			{
				case MPR_TX_NONE:
				{
					m_pD3DD->SetTextureStageState(0,D3DTSS_MAGFILTER,D3DTFG_POINT);
					m_pD3DD->SetTextureStageState(0,D3DTSS_MINFILTER,D3DTFN_POINT);
					break;
				}

				case MPR_TX_BILINEAR:
				case MPR_TX_BILINEAR_NOCLAMP:
				{
					if(DisplayOptions.bAnisotropicFiltering)
					{
						m_pD3DD->SetTextureStageState(0,D3DTSS_MAGFILTER,D3DTFG_ANISOTROPIC);
						m_pD3DD->SetTextureStageState(0,D3DTSS_MINFILTER,D3DTFN_ANISOTROPIC);
						m_pD3DD->SetTextureStageState(0,D3DTSS_MAXANISOTROPY,m_pCtxDX->m_pD3DHWDeviceDesc->dwMaxAnisotropy);
					}

					else
					{
						m_pD3DD->SetTextureStageState(0,D3DTSS_MAGFILTER,D3DTFG_LINEAR);
						m_pD3DD->SetTextureStageState(0,D3DTSS_MINFILTER,D3DTFN_LINEAR);
					}

					if(Value == MPR_TX_BILINEAR)
					{
						m_pD3DD->SetTextureStageState(0,D3DTSS_ADDRESS,D3DTADDRESS_CLAMP);
					}

					break;
				}

				case MPR_TX_MIPMAP_NEAREST:
				{
					m_pD3DD->SetTextureStageState(0,D3DTSS_MIPFILTER,D3DTFP_NONE);
					break;
				}

				case MPR_TX_MIPMAP_LINEAR:
				{
					if(DisplayOptions.bLinearMipFiltering)
						m_pD3DD->SetTextureStageState(0,D3DTSS_MIPFILTER,D3DTFP_LINEAR);

					break;
				}
			}

			break;
		}

		case MPR_STA_FG_COLOR:
		{
			SelectForegroundColor(Value);
			break;
		}

		case MPR_STA_BG_COLOR:
		{
			SelectBackgroundColor(Value);
			break;
		}

		case MPR_STA_FOG_COLOR:
		{
			FlushVB();

			m_pD3DD->SetRenderState(D3DRENDERSTATE_FOGCOLOR,MPRColor2D3DRGBA(Value));
			break;
		}

		case MPR_STA_SCISSOR_LEFT:  
		{
			if(Value != m_rcVP.left)
			{
				FlushVB();

				m_rcVP.left = Value;
				UpdateViewport();
			}
			break;
		}

		case MPR_STA_SCISSOR_TOP:  
		{
			if(Value != m_rcVP.top)
			{
				FlushVB();

				m_rcVP.top = Value;
				UpdateViewport();
			}

			break;
		}

		case MPR_STA_SCISSOR_RIGHT:
		{
			if(Value != m_rcVP.right)
			{
				FlushVB();

				m_rcVP.right = Value;
				UpdateViewport();
			}

			break;
		}

		case MPR_STA_SCISSOR_BOTTOM:
		{
			if(Value != m_rcVP.bottom)
			{
				FlushVB();

				m_rcVP.bottom = Value;
				UpdateViewport();
			}

			break;
		}

		case MPR_STA_NONE:
		{
			break;
		}
	}
}

void ContextMPR::SetStateInternal(WORD State, DWORD Value)
{
	switch(State)
	{
		case MPR_STA_NONE:
		{
			break;
		}

		case MPR_STA_DISABLES:
		case MPR_STA_ENABLES:
		{
			bool bNewVal = (State == MPR_STA_ENABLES) ? true : false;

			if(Value & MPR_SE_SCISSORING)
				StateTableInternal[currentState].SE_SCISSORING = bNewVal;

			if(Value & MPR_SE_MODULATION)
				StateTableInternal[currentState].SE_MODULATION = bNewVal;

			if(Value & MPR_SE_TEXTURING)
				StateTableInternal[currentState].SE_TEXTURING = bNewVal;

			if(Value & MPR_SE_SHADING)
				StateTableInternal[currentState].SE_SHADING = bNewVal;

			if(Value & MPR_SE_Z_BUFFERING)
				StateTableInternal[currentState].SE_Z_BUFFERING = bNewVal;

			if(Value & MPR_SE_Z_WRITE)
				StateTableInternal[currentState].SE_Z_WRITE = bNewVal;

			if(Value & MPR_SE_FILTERING)
				StateTableInternal[currentState].SE_FILTERING = bNewVal;

			if(Value & MPR_SE_ALPHA)
				StateTableInternal[currentState].SE_ALPHA = bNewVal;

			if(Value & MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE)
				StateTableInternal[currentState].SE_NON_PERSPECTIVE_CORRECTION_MODE = bNewVal;

			break;
		}
	}
}

// flag & 0x01  --> skip StateSetupCount checking --> reset/set state
void ContextMPR::SetCurrentState(GLint state, GLint flag)
{
	UInt32	i = 0;

	#ifdef _CONTEXT_TRACE_ALL
	MonoPrint("ContextMPR::SetCurrentState (%d,0x%X)\n",state,flag);
	#endif

	ShiAssert(FALSE == F4IsBadReadPtr(m_pD3DD,sizeof *m_pD3DD));

	//Disable secondary stage
	m_pD3DD->SetTextureStageState(1,D3DTSS_ALPHAOP,D3DTOP_DISABLE);
	m_pD3DD->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_DISABLE);

	m_pD3DD->SetTextureStageState(0,D3DTSS_ADDRESS,D3DTADDRESS_WRAP);

	switch(state)
	{
		case STATE_SOLID:
			SetState(MPR_STA_DISABLES,
				MPR_SE_FILTERING |
				MPR_SE_ALPHA |
				MPR_SE_TEXTURING | 
				MPR_SE_MODULATION | 
				MPR_SE_SHADING);

			break;

		case STATE_LIT:
			SetState(MPR_STA_DISABLES,
				MPR_SE_FILTERING |
				MPR_SE_ALPHA |
				MPR_SE_TEXTURING | 
				MPR_SE_SHADING);

			SetState(MPR_STA_ENABLES,
				MPR_SE_MODULATION);

			break;

		case STATE_GOURAUD:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_FILTERING |
				MPR_SE_ALPHA |
				MPR_SE_TEXTURING |
				MPR_SE_MODULATION);

			SetState(MPR_STA_ENABLES,
				MPR_SE_SHADING);

			break;

		case STATE_TEXTURE:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_FILTERING |
				MPR_SE_ALPHA |
				MPR_SE_MODULATION | 
				MPR_SE_SHADING);

			SetState(MPR_STA_ENABLES,
				MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | 
				MPR_SE_TEXTURING);

			SetState(MPR_STA_TEX_FILTER,MPR_TX_MIPMAP_NEAREST);
			if(PlayerOptions.FilteringOn())
			{
				SetState(MPR_STA_TEX_FILTER,MPR_TX_BILINEAR_NOCLAMP);
				SetState(MPR_STA_TEX_FILTER,MPR_TX_MIPMAP_LINEAR);
			}

			break;

		case STATE_TEXTURE_LIT:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_FILTERING |
				MPR_SE_ALPHA |
				MPR_SE_SHADING);

			SetState(MPR_STA_ENABLES,
				MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE |
				MPR_SE_TEXTURING | 
				MPR_SE_MODULATION);

			SetState(MPR_STA_TEX_FILTER,MPR_TX_MIPMAP_NEAREST);
			if(PlayerOptions.FilteringOn())
			{
				SetState(MPR_STA_TEX_FILTER,MPR_TX_BILINEAR_NOCLAMP);
				SetState(MPR_STA_TEX_FILTER,MPR_TX_MIPMAP_LINEAR);
			}

			break;

		case STATE_TEXTURE_LIT_PERSPECTIVE:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_FILTERING |
				MPR_SE_ALPHA |
				MPR_SE_SHADING |
				MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE);

			SetState(MPR_STA_ENABLES,
				MPR_SE_TEXTURING |
				MPR_SE_MODULATION);

			SetState(MPR_STA_TEX_FILTER,MPR_TX_MIPMAP_NEAREST);
			if(PlayerOptions.FilteringOn())
			{
				SetState(MPR_STA_TEX_FILTER,MPR_TX_BILINEAR_NOCLAMP);
				SetState(MPR_STA_TEX_FILTER,MPR_TX_MIPMAP_LINEAR);
			}

			break;

 		case STATE_TEXTURE_SMOOTH:
 			SetState(MPR_STA_DISABLES, 
   				MPR_SE_FILTERING |
 				MPR_SE_MODULATION | 
 				MPR_SE_ALPHA);
 
 			SetState(MPR_STA_ENABLES,
 				MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | 
  				MPR_SE_TEXTURING | 
 				MPR_SE_SHADING);
 
 			SetState(MPR_STA_TEX_FILTER,MPR_TX_MIPMAP_NEAREST);
 			if(PlayerOptions.FilteringOn())
 			{
				SetState(MPR_STA_TEX_FILTER,MPR_TX_BILINEAR_NOCLAMP);
 				SetState(MPR_STA_TEX_FILTER,MPR_TX_MIPMAP_LINEAR);
 			}
 
 			break;

		case STATE_TEXTURE_GOURAUD:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_FILTERING |
				MPR_SE_ALPHA);

			SetState(MPR_STA_ENABLES,
				MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE |
				MPR_SE_TEXTURING | 
				MPR_SE_SHADING | 
				MPR_SE_MODULATION);

			SetState(MPR_STA_TEX_FILTER,MPR_TX_MIPMAP_NEAREST);
			if(PlayerOptions.FilteringOn())
			{
				SetState(MPR_STA_TEX_FILTER,MPR_TX_BILINEAR_NOCLAMP);
				SetState(MPR_STA_TEX_FILTER,MPR_TX_MIPMAP_LINEAR);
			}

			break;

		case STATE_TEXTURE_PERSPECTIVE:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_FILTERING |
				MPR_SE_ALPHA |
				MPR_SE_MODULATION | 
				MPR_SE_SHADING |
				MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE);

			SetState(MPR_STA_ENABLES,
				MPR_SE_TEXTURING);

			SetState(MPR_STA_TEX_FILTER,MPR_TX_MIPMAP_NEAREST);
			if(PlayerOptions.FilteringOn())
			{
				SetState(MPR_STA_TEX_FILTER,MPR_TX_BILINEAR_NOCLAMP);
				SetState(MPR_STA_TEX_FILTER,MPR_TX_MIPMAP_LINEAR);
			}

			break;

 		case STATE_TEXTURE_SMOOTH_PERSPECTIVE:
 			SetState(MPR_STA_DISABLES, 
   				MPR_SE_FILTERING |
 				MPR_SE_MODULATION | 
				MPR_SE_ALPHA |
 				MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE);
 
 			SetState(MPR_STA_ENABLES,
  				MPR_SE_TEXTURING | 
 				MPR_SE_SHADING);
 
 			SetState(MPR_STA_TEX_FILTER,MPR_TX_MIPMAP_NEAREST);
 			if(PlayerOptions.FilteringOn())
 			{
				SetState(MPR_STA_TEX_FILTER,MPR_TX_BILINEAR_NOCLAMP);
 				SetState(MPR_STA_TEX_FILTER,MPR_TX_MIPMAP_LINEAR);
 			}
 
 			break;

		case STATE_TEXTURE_GOURAUD_PERSPECTIVE:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_FILTERING |
				MPR_SE_ALPHA |
				MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE);

			SetState(MPR_STA_ENABLES,
				MPR_SE_TEXTURING | 
				MPR_SE_SHADING | 
				MPR_SE_MODULATION);

			SetState(MPR_STA_TEX_FILTER,MPR_TX_MIPMAP_NEAREST);
			if(PlayerOptions.FilteringOn())
			{
				SetState(MPR_STA_TEX_FILTER,MPR_TX_BILINEAR_NOCLAMP);
				SetState(MPR_STA_TEX_FILTER,MPR_TX_MIPMAP_LINEAR);
			}

			break;

		case STATE_ALPHA_SOLID:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_FILTERING |
				MPR_SE_TEXTURING | 
				MPR_SE_MODULATION | 
				MPR_SE_SHADING);

			SetState(MPR_STA_ENABLES,
				MPR_SE_ALPHA);

			break;

		case STATE_ALPHA_LIT:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_FILTERING |
				MPR_SE_TEXTURING | 
				MPR_SE_SHADING);

			SetState(MPR_STA_ENABLES,
				MPR_SE_MODULATION | 
				MPR_SE_ALPHA);

			break;

		case STATE_ALPHA_GOURAUD:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_FILTERING |
				MPR_SE_TEXTURING);

			SetState(MPR_STA_ENABLES,
				MPR_SE_SHADING |
				MPR_SE_MODULATION |
				MPR_SE_ALPHA);

			break;

		case STATE_CHROMA_TEXTURE:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_FILTERING |
				MPR_SE_MODULATION | 
				MPR_SE_SHADING);

			SetState(MPR_STA_ENABLES,
				MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE |
				MPR_SE_TEXTURING |
				MPR_SE_CHROMA);

			SetState(MPR_STA_TEX_FILTER,MPR_TX_MIPMAP_NEAREST);
			if(PlayerOptions.FilteringOn() && DisplayOptions.m_texMode == DisplayOptionsClass::TEX_MODE_DDS)
			{
				SetState(MPR_STA_TEX_FILTER,MPR_TX_BILINEAR_NOCLAMP);
				SetState(MPR_STA_TEX_FILTER,MPR_TX_MIPMAP_LINEAR);
			}

			break;

		case STATE_ALPHA_TEXTURE:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_FILTERING |
				MPR_SE_MODULATION | 
				MPR_SE_SHADING);

			SetState(MPR_STA_ENABLES,
				MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE |
				MPR_SE_TEXTURING |
				MPR_SE_ALPHA);

			SetState(MPR_STA_TEX_FILTER,MPR_TX_MIPMAP_NEAREST);
			if(PlayerOptions.FilteringOn())
			{
				SetState(MPR_STA_TEX_FILTER,MPR_TX_BILINEAR_NOCLAMP);
				SetState(MPR_STA_TEX_FILTER,MPR_TX_MIPMAP_LINEAR);
			}

			break;

		case STATE_CHROMA_TEXTURE_LIT:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_FILTERING |
				MPR_SE_SHADING);

			SetState(MPR_STA_ENABLES,
				MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE |
				MPR_SE_TEXTURING | 
				MPR_SE_MODULATION |
				MPR_SE_CHROMA);

			SetState(MPR_STA_TEX_FILTER,MPR_TX_MIPMAP_NEAREST);
			if(PlayerOptions.FilteringOn() && DisplayOptions.m_texMode == DisplayOptionsClass::TEX_MODE_DDS)
			{
				SetState(MPR_STA_TEX_FILTER,MPR_TX_BILINEAR_NOCLAMP);
				SetState(MPR_STA_TEX_FILTER,MPR_TX_MIPMAP_LINEAR);
			}

			break;

		case STATE_ALPHA_TEXTURE_LIT:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_FILTERING |
				MPR_SE_SHADING);

			SetState(MPR_STA_ENABLES,
				MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE |
				MPR_SE_TEXTURING | 
				MPR_SE_MODULATION |
				MPR_SE_ALPHA);

			SetState(MPR_STA_TEX_FILTER,MPR_TX_MIPMAP_NEAREST);
			if(PlayerOptions.FilteringOn())
			{
				SetState(MPR_STA_TEX_FILTER,MPR_TX_BILINEAR_NOCLAMP);
				SetState(MPR_STA_TEX_FILTER,MPR_TX_MIPMAP_LINEAR);
			}

			break;

		case STATE_CHROMA_TEXTURE_LIT_PERSPECTIVE:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_FILTERING |
				MPR_SE_SHADING |
				MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE);

			SetState(MPR_STA_ENABLES,
				MPR_SE_TEXTURING | 
				MPR_SE_MODULATION |
				MPR_SE_CHROMA);

			SetState(MPR_STA_TEX_FILTER,MPR_TX_MIPMAP_NEAREST);
			if(PlayerOptions.FilteringOn() && DisplayOptions.m_texMode == DisplayOptionsClass::TEX_MODE_DDS)
			{
				SetState(MPR_STA_TEX_FILTER,MPR_TX_BILINEAR_NOCLAMP);
				SetState(MPR_STA_TEX_FILTER,MPR_TX_MIPMAP_LINEAR);
			}

			break;

		case STATE_ALPHA_TEXTURE_LIT_PERSPECTIVE:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_FILTERING |
				MPR_SE_SHADING |
				MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE);

			SetState(MPR_STA_ENABLES,
				MPR_SE_TEXTURING | 
				MPR_SE_MODULATION |
				MPR_SE_ALPHA);

			SetState(MPR_STA_TEX_FILTER,MPR_TX_MIPMAP_NEAREST);
			if(PlayerOptions.FilteringOn())
			{
				SetState(MPR_STA_TEX_FILTER,MPR_TX_BILINEAR_NOCLAMP);
				SetState(MPR_STA_TEX_FILTER,MPR_TX_MIPMAP_LINEAR);
			}

			break;

		case STATE_ALPHA_TEXTURE_SMOOTH:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_FILTERING |
				MPR_SE_MODULATION);

			SetState(MPR_STA_ENABLES,
				MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE |
				MPR_SE_TEXTURING | 
				MPR_SE_SHADING |
				MPR_SE_ALPHA);

			SetState(MPR_STA_TEX_FILTER,MPR_TX_MIPMAP_NEAREST);
			if(PlayerOptions.FilteringOn())
			{
				SetState(MPR_STA_TEX_FILTER,MPR_TX_BILINEAR_NOCLAMP);
				SetState(MPR_STA_TEX_FILTER,MPR_TX_MIPMAP_LINEAR);
			}

			break;

		case STATE_ALPHA_TEXTURE_SMOOTH_PERSPECTIVE:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_FILTERING |
				MPR_SE_MODULATION | 
				MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE);

			SetState(MPR_STA_ENABLES,
				MPR_SE_TEXTURING | 
				MPR_SE_SHADING |
				MPR_SE_ALPHA);

			SetState(MPR_STA_TEX_FILTER,MPR_TX_MIPMAP_NEAREST);
			if(PlayerOptions.FilteringOn())
			{
				SetState(MPR_STA_TEX_FILTER,MPR_TX_BILINEAR_NOCLAMP);
				SetState(MPR_STA_TEX_FILTER,MPR_TX_MIPMAP_LINEAR);
			}

			break;

		case STATE_CHROMA_TEXTURE_GOURAUD:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_FILTERING);

			SetState(MPR_STA_ENABLES,
				MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | 
				MPR_SE_TEXTURING | 
				MPR_SE_SHADING | 
				MPR_SE_MODULATION |
				MPR_SE_CHROMA);

			SetState(MPR_STA_TEX_FILTER,MPR_TX_MIPMAP_NEAREST);
			if(PlayerOptions.FilteringOn() && DisplayOptions.m_texMode == DisplayOptionsClass::TEX_MODE_DDS)
			{
				SetState(MPR_STA_TEX_FILTER,MPR_TX_BILINEAR_NOCLAMP);
				SetState(MPR_STA_TEX_FILTER,MPR_TX_MIPMAP_LINEAR);
			}

			break;

		case STATE_ALPHA_TEXTURE_GOURAUD:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_FILTERING);

			SetState(MPR_STA_ENABLES,
				MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | 
				MPR_SE_TEXTURING | 
				MPR_SE_SHADING | 
				MPR_SE_MODULATION |
				MPR_SE_ALPHA);

			SetState(MPR_STA_TEX_FILTER,MPR_TX_MIPMAP_NEAREST);
			if(PlayerOptions.FilteringOn())
			{
				SetState(MPR_STA_TEX_FILTER,MPR_TX_BILINEAR_NOCLAMP);
				SetState(MPR_STA_TEX_FILTER,MPR_TX_MIPMAP_LINEAR);
			}

			break;

		case STATE_CHROMA_TEXTURE_PERSPECTIVE:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_FILTERING |
				MPR_SE_MODULATION | 
				MPR_SE_SHADING |
				MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE);

			SetState(MPR_STA_ENABLES,
				MPR_SE_TEXTURING |
				MPR_SE_CHROMA);

			SetState(MPR_STA_TEX_FILTER,MPR_TX_MIPMAP_NEAREST);
			if(PlayerOptions.FilteringOn() && DisplayOptions.m_texMode == DisplayOptionsClass::TEX_MODE_DDS)
			{
				SetState(MPR_STA_TEX_FILTER,MPR_TX_BILINEAR_NOCLAMP);
				SetState(MPR_STA_TEX_FILTER,MPR_TX_MIPMAP_LINEAR);
			}

			break;

		case STATE_ALPHA_TEXTURE_PERSPECTIVE:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_FILTERING |
				MPR_SE_MODULATION | 
				MPR_SE_SHADING |
				MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE);

			SetState(MPR_STA_ENABLES,
				MPR_SE_TEXTURING |
				MPR_SE_ALPHA);

			SetState(MPR_STA_TEX_FILTER,MPR_TX_MIPMAP_NEAREST);
			if(PlayerOptions.FilteringOn())
			{
				SetState(MPR_STA_TEX_FILTER,MPR_TX_BILINEAR_NOCLAMP);
				SetState(MPR_STA_TEX_FILTER,MPR_TX_MIPMAP_LINEAR);
			}

			break;

		case STATE_CHROMA_TEXTURE_GOURAUD_PERSPECTIVE:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_FILTERING |
				MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE);

			SetState(MPR_STA_ENABLES,
				MPR_SE_TEXTURING | 
				MPR_SE_SHADING | 
				MPR_SE_MODULATION |
				MPR_SE_CHROMA);

			SetState(MPR_STA_TEX_FILTER,MPR_TX_MIPMAP_NEAREST);
			if(PlayerOptions.FilteringOn() && DisplayOptions.m_texMode == DisplayOptionsClass::TEX_MODE_DDS)
			{
				SetState(MPR_STA_TEX_FILTER,MPR_TX_BILINEAR_NOCLAMP);
				SetState(MPR_STA_TEX_FILTER,MPR_TX_MIPMAP_LINEAR);
			}

			break;

		case STATE_ALPHA_TEXTURE_GOURAUD_PERSPECTIVE:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_FILTERING |
				MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE);

			SetState(MPR_STA_ENABLES,
				MPR_SE_TEXTURING | 
				MPR_SE_SHADING | 
				MPR_SE_MODULATION |
				MPR_SE_ALPHA);

			SetState(MPR_STA_TEX_FILTER,MPR_TX_MIPMAP_NEAREST);
			if(PlayerOptions.FilteringOn())
			{
				SetState(MPR_STA_TEX_FILTER,MPR_TX_BILINEAR_NOCLAMP);
				SetState(MPR_STA_TEX_FILTER,MPR_TX_MIPMAP_LINEAR);
			}

			break;

		case STATE_TEXTURE_NOFILTER:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_FILTERING |
				MPR_SE_ALPHA |
				MPR_SE_SHADING);

			SetState(MPR_STA_ENABLES,
				MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | 
				MPR_SE_TEXTURING |
				MPR_SE_MODULATION);

			SetState(MPR_STA_TEX_FILTER,MPR_TX_MIPMAP_NEAREST);

			break;

		case STATE_TEXTURE_NOFILTER_PERSPECTIVE:
			SetState(MPR_STA_DISABLES,
				MPR_SE_FILTERING |
				MPR_SE_ALPHA |
				MPR_SE_SHADING |
				MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE);

			SetState(MPR_STA_ENABLES,
				MPR_SE_TEXTURING |
				MPR_SE_MODULATION);

			SetState(MPR_STA_TEX_FILTER,MPR_TX_MIPMAP_NEAREST);

			break;

		case STATE_ALPHA_TEXTURE_NOFILTER:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_FILTERING |
				MPR_SE_MODULATION |
				MPR_SE_SHADING);

			SetState(MPR_STA_ENABLES,
				MPR_SE_TEXTURING |
				MPR_SE_ALPHA |
				MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE);

			SetState(MPR_STA_TEX_FILTER,MPR_TX_MIPMAP_NEAREST);

			break;

		case STATE_ALPHA_TEXTURE_NOFILTER_PERSPECTIVE:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_FILTERING |
				MPR_SE_MODULATION | 
				MPR_SE_SHADING |
				MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE);

			SetState(MPR_STA_ENABLES,
				MPR_SE_TEXTURING |
				MPR_SE_ALPHA);

			SetState(MPR_STA_TEX_FILTER,MPR_TX_MIPMAP_NEAREST);

			break;

		case STATE_LANDSCAPE_LIT:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_FILTERING |
				MPR_SE_ALPHA |
				MPR_SE_SHADING |
				MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE);

			SetState(MPR_STA_ENABLES,
				MPR_SE_TEXTURING |
				MPR_SE_MODULATION);

			SetState(MPR_STA_TEX_FILTER,MPR_TX_MIPMAP_NEAREST);
			if(PlayerOptions.FilteringOn())
				SetState(MPR_STA_TEX_FILTER,MPR_TX_BILINEAR);

			if(DisplayOptions.m_texMode != DisplayOptionsClass::TEX_MODE_DDS)
				SetState(MPR_STA_DISABLES,MPR_SE_MODULATION);

			break;

		case STATE_LANDSCAPE_GOURAUD:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_FILTERING |
				MPR_SE_ALPHA |
				MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE);

			SetState(MPR_STA_ENABLES,
				MPR_SE_TEXTURING | 
				MPR_SE_SHADING | 
				MPR_SE_MODULATION);

			SetState(MPR_STA_TEX_FILTER,MPR_TX_MIPMAP_NEAREST);
			if(PlayerOptions.FilteringOn())
				SetState(MPR_STA_TEX_FILTER,MPR_TX_BILINEAR);

			if(DisplayOptions.m_texMode != DisplayOptionsClass::TEX_MODE_DDS)
				SetState(MPR_STA_DISABLES,MPR_SE_MODULATION);

			break;

		case STATE_MULTITEXTURE:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_FILTERING |
				MPR_SE_ALPHA |
				MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE);

			SetState(MPR_STA_ENABLES,
				MPR_SE_TEXTURING | 
				MPR_SE_SHADING | 
				MPR_SE_MODULATION);

			SetState(MPR_STA_TEX_FILTER,MPR_TX_MIPMAP_NEAREST);
			if(PlayerOptions.FilteringOn())
				SetState(MPR_STA_TEX_FILTER,MPR_TX_BILINEAR);

			//TEXTURESTAGE1
			m_pD3DD->SetTextureStageState(1,D3DTSS_COLORARG2,D3DTA_TEXTURE);
			m_pD3DD->SetTextureStageState(1,D3DTSS_COLORARG1,D3DTA_CURRENT);
			m_pD3DD->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_ADD);
			m_pD3DD->SetTextureStageState(1,D3DTSS_ALPHAOP,D3DTOP_DISABLE);

			m_pD3DD->SetTextureStageState(1,D3DTSS_MINFILTER,D3DTFN_POINT);
			m_pD3DD->SetTextureStageState(1,D3DTSS_MAGFILTER,D3DTFG_POINT);
			m_pD3DD->SetTextureStageState(1,D3DTSS_MIPFILTER,D3DTFP_NONE);

			if(PlayerOptions.FilteringOn())
			{
				if(DisplayOptions.bAnisotropicFiltering)
				{
					m_pD3DD->SetTextureStageState(1,D3DTSS_MAGFILTER,D3DTFG_ANISOTROPIC);
					m_pD3DD->SetTextureStageState(1,D3DTSS_MINFILTER,D3DTFN_ANISOTROPIC);
					m_pD3DD->SetTextureStageState(1,D3DTSS_MAXANISOTROPY,m_pCtxDX->m_pD3DHWDeviceDesc->dwMaxAnisotropy);
				}
				else
				{
					m_pD3DD->SetTextureStageState(1,D3DTSS_MAGFILTER,D3DTFG_LINEAR);
					m_pD3DD->SetTextureStageState(1,D3DTSS_MINFILTER,D3DTFN_LINEAR);
				}
			}

			break;

		case STATE_MULTITEXTURE_ALPHA:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_FILTERING |
				MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE);

			SetState(MPR_STA_ENABLES,
				MPR_SE_TEXTURING | 
				MPR_SE_ALPHA |
				MPR_SE_SHADING | 
				MPR_SE_MODULATION);

			SetState(MPR_STA_TEX_FILTER,MPR_TX_MIPMAP_NEAREST);
			if(PlayerOptions.FilteringOn())
				SetState(MPR_STA_TEX_FILTER,MPR_TX_BILINEAR);

			//TEXTURESTAGE1
			m_pD3DD->SetTextureStageState(1,D3DTSS_COLORARG1,D3DTA_CURRENT);
			m_pD3DD->SetTextureStageState(1,D3DTSS_COLORARG2,D3DTA_TEXTURE);
			m_pD3DD->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_ADD);

			m_pD3DD->SetTextureStageState(1,D3DTSS_ALPHAARG1,D3DTA_CURRENT);
			m_pD3DD->SetTextureStageState(1,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);

			m_pD3DD->SetTextureStageState(1,D3DTSS_MINFILTER,D3DTFN_POINT);
			m_pD3DD->SetTextureStageState(1,D3DTSS_MAGFILTER,D3DTFG_POINT);
			m_pD3DD->SetTextureStageState(1,D3DTSS_MIPFILTER,D3DTFP_NONE);

			if(PlayerOptions.FilteringOn())
			{
				if(DisplayOptions.bAnisotropicFiltering)
				{
					m_pD3DD->SetTextureStageState(1,D3DTSS_MAGFILTER,D3DTFG_ANISOTROPIC);
					m_pD3DD->SetTextureStageState(1,D3DTSS_MINFILTER,D3DTFN_ANISOTROPIC);
					m_pD3DD->SetTextureStageState(1,D3DTSS_MAXANISOTROPY,m_pCtxDX->m_pD3DHWDeviceDesc->dwMaxAnisotropy);
				}
				else
				{
					m_pD3DD->SetTextureStageState(1,D3DTSS_MAGFILTER,D3DTFG_LINEAR);
					m_pD3DD->SetTextureStageState(1,D3DTSS_MINFILTER,D3DTFN_LINEAR);
				}
			}

			break;

		case STATE_TEXTURE_TEXT:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_SHADING | 
				MPR_SE_FILTERING);

			SetState(MPR_STA_ENABLES,
				MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | 
				MPR_SE_TEXTURING | 
				MPR_SE_MODULATION |
				MPR_SE_ALPHA);

			SetState(MPR_STA_TEX_FILTER,MPR_TX_MIPMAP_NEAREST);

			break;

		case STATE_ALPHA_TEXTURE_PERSPECTIVE_CLAMP:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_FILTERING |
				MPR_SE_SHADING |
				MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE);

			SetState(MPR_STA_ENABLES,
				MPR_SE_TEXTURING |
				MPR_SE_MODULATION | 
				MPR_SE_ALPHA);

			SetState(MPR_STA_TEX_FILTER,MPR_TX_MIPMAP_NEAREST);
			if(PlayerOptions.FilteringOn())
			{
				SetState(MPR_STA_TEX_FILTER,MPR_TX_BILINEAR);
				SetState(MPR_STA_TEX_FILTER,MPR_TX_MIPMAP_LINEAR);
			}

			break;

		default:
			ShiWarning("BAD OR MISSING CONTEXT STATE");
	}
}

void ContextMPR::Render2DBitmap( int sX, int sY, int dX, int dY, int w, int h, int totalWidth, DWORD *pSrc)
{
	DWORD *pDst = NULL;

	#ifdef _CONTEXT_TRACE_ALL
	MonoPrint("ContextMPR::Render2DBitmap(%d,%d,%d,%d,%d,%d,%d,0x%X)\n",sX,sY,dX,dY,w,h,totalWidth,pSrc);
	#endif
	ShiAssert(FALSE == F4IsBadReadPtr(m_pD3DD,sizeof *m_pD3DD));
	try
	{
		// Convert from ABGR to ARGB ;(
		pSrc = (DWORD *)(((BYTE *)pSrc) + (sY * (totalWidth << 2)) + sX);
		pDst = new DWORD[w * h];
		if(!pDst) throw _com_error(E_OUTOFMEMORY);

		for(int y=0;y<h;y++)
		{
			for(int x=0;x<w;x++)
				pDst[y * w + x] = RGBA_MAKE(RGBA_GETBLUE(pSrc[x]),RGBA_GETGREEN(pSrc[x]),
					RGBA_GETRED(pSrc[x]),RGBA_GETALPHA(pSrc[x]));

			pSrc = (DWORD *)(((BYTE *)pSrc) + (totalWidth << 2));
		}

		// Create tmp texture
		DWORD dwFlags = D3DX_TEXTURE_NOMIPMAP;
		DWORD dwActualWidth = w;
		DWORD dwActualHeight = h;
		D3DX_SURFACEFORMAT fmt = D3DX_SF_A8R8G8B8;
		DWORD dwNumMipMaps = 0;

		IDirectDrawSurface7Ptr pDDSTex;
		CheckHR(D3DXCreateTexture(m_pD3DD,&dwFlags,&dwActualWidth,&dwActualHeight,
			&fmt,NULL,&pDDSTex,&dwNumMipMaps));

		ShiAssert(FALSE==F4IsBadReadPtr(pDDSTex,sizeof *pDDSTex));
		CheckHR(D3DXLoadTextureFromMemory(m_pD3DD,pDDSTex,0,pDst,NULL,
			D3DX_SF_A8R8G8B8,w << 2,NULL,D3DX_FT_LINEAR));

		// Setup vertices
		TwoDVertex pVtx[4];
		ZeroMemory(pVtx,sizeof(pVtx));
		pVtx[0].x = dX; pVtx[0].y = dY; pVtx[0].u = 0.0f; pVtx[0].v = 0.0f;
		pVtx[1].x = dX + w; pVtx[1].y = dY; pVtx[1].u = 1.0f; pVtx[1].v = 0.0f;
		pVtx[2].x = dX + w; pVtx[2].y = dY + h; pVtx[2].u = 1.0f; pVtx[2].v = 1.0f;
		pVtx[3].x = dX; pVtx[3].y = dY + h; pVtx[3].u = 0.0f; pVtx[3].v = 1.0f;

		// Setup state
		RestoreState(STATE_TEXTURE);

		CheckHR(m_pD3DD->SetTexture(0,pDDSTex));

		// Render it (finally)
		DrawPrimitive(MPR_PRM_TRIFAN,MPR_VI_COLOR | MPR_VI_TEXTURE,4,pVtx,sizeof(pVtx[0]));

		FlushVB();
		InvalidateState();
	}

	catch(_com_error e)
	{
		MonoPrint("ContextMPR::Render2DBitmap - Error 0x%X\n",e.Error());
	}

	if(pDst) delete[] pDst;
}

inline void ContextMPR::SetStateTable(GLint state, GLint flag)
{
	if (!m_pD3DD)
		return;
		
	// Record a stateblock
	HRESULT hr = m_pD3DD->BeginStateBlock();
	ShiAssert(SUCCEEDED(hr));

	SetCurrentState(state,flag);

	hr = m_pD3DD->EndStateBlock((DWORD *)&StateTable[state]);
	ShiAssert(SUCCEEDED(hr) && StateTable[state]);

	// Record internal state
	m_bUseSetStateInternal = true;
	SetCurrentState(state,flag);
	m_bUseSetStateInternal = false;
}

inline void ContextMPR::ClearStateTable(GLint state)
{
	HRESULT hr = m_pD3DD->DeleteStateBlock(StateTable[state]);
	ShiAssert(SUCCEEDED(hr));

	StateTable[state] = 0;
}


void ContextMPR::SetupMPRState(GLint flag)
{
	if(flag & CHECK_PREVIOUS_STATE)
	{
		StateSetupCounter++;
		if(StateSetupCounter > 1)
			return;
	}
	else if(StateSetupCounter)
		CleanupMPRState();

	// Record one stateblock per poly type
	MonoPrint("ContextMPR - Setting up state table\n");

	for(currentState = STATE_SOLID; currentState < MAXIMUM_MPR_STATE; currentState++)
		SetStateTable(currentState,flag);

	InvalidateState();
}

void ContextMPR::CleanupMPRState (GLint flag)
{
	if(!StateSetupCounter)
	{
		ShiWarning("MPR not initialized!");
		return;
	}

	if(flag & CHECK_PREVIOUS_STATE)
	{
		StateSetupCounter--;
		if(StateSetupCounter > 0)
			return;
	}

	MonoPrint("ContextMPR - Clearing state table\n");
	for(int i=STATE_SOLID; i<MAXIMUM_MPR_STATE; i++)
		ClearStateTable(i);
}

void ContextMPR::SetTexture1(GLint texID)
{
	if(texID != lastTexture1)
	{
		HRESULT hr;
		
		lastTexture1 = texID;

		if(texID == -1)
			hr = m_pD3DD->SetTexture(0,NULL);
		else
			hr = m_pD3DD->SetTexture(0,(IDirectDrawSurface7 *)texID);

		if(!SUCCEEDED(hr)) INT3;

		m_pD3DD->SetTexture(1,NULL);
	}
}

void ContextMPR::SetTexture2(GLint texID)
{
	if(texID != lastTexture2)
	{
		HRESULT hr;
		
		lastTexture2 = texID;

		if(texID == -1)
			hr = m_pD3DD->SetTexture(1,NULL);
		else
			hr = m_pD3DD->SetTexture(1,(IDirectDrawSurface7 *)texID);

		if(!SUCCEEDED(hr)) INT3;
	}
}

void ContextMPR::SelectTexture1(GLint texID)
{
#ifdef _CONTEXT_TRACE_ALL
	MonoPrint("ContextMPR::ApplyTexture1(0x%X)\n",texID);
#endif

	if(texID)
		texID = (GLint)((TextureHandle *)texID)->m_pDDS;

	if(texID != currentTexture1)
	{
		currentTexture1 = texID;

	#ifdef _CONTEXT_ENABLE_STATS
		m_stats.PutTexture(false);
	#endif

		if(!bZBuffering)
		{
			// JB 010326 CTD (too much CPU)
			if(g_bSlowButSafe && F4IsBadReadPtr((TextureHandle *)texID,sizeof(TextureHandle)))
				return;

			FlushVB();

			HRESULT hr = m_pD3DD->SetTexture(0,(IDirectDrawSurface7 *)texID);
			ShiAssert(SUCCEEDED(hr));

			m_pD3DD->SetTexture(1,NULL);
		}
	}	
#ifdef _CONTEXT_ENABLE_STATS
	else m_stats.PutTexture(true);
#endif

	currentTexture2 = -1;
}

void ContextMPR::SelectTexture2(GLint texID)
{
#ifdef _CONTEXT_TRACE_ALL
	MonoPrint("ContextMPR::ApplyTexture2(0x%X)\n",texID);
#endif

	if(texID)
		texID = (GLint)((TextureHandle *)texID)->m_pDDS;

	if(texID != currentTexture2)
	{
		currentTexture2 = texID;

	#ifdef _CONTEXT_ENABLE_STATS
		m_stats.PutTexture(false);
	#endif

		if(!bZBuffering)
		{
			// JB 010326 CTD (too much CPU)
			if(g_bSlowButSafe && F4IsBadReadPtr((TextureHandle *)texID,sizeof(TextureHandle)))
				return;

			FlushVB();

			HRESULT hr = m_pD3DD->SetTexture(1,(IDirectDrawSurface7 *)texID);
			ShiAssert(SUCCEEDED(hr));
		}
	}	
#ifdef _CONTEXT_ENABLE_STATS
	else m_stats.PutTexture(true);
#endif
}

void ContextMPR::SelectForegroundColor(GLint color)
{
	if(color != m_colFG_Raw)
	{
		m_colFG_Raw = color;
		m_colFG = MPRColor2D3DRGBA(color);
	}
}

void ContextMPR::SelectBackgroundColor(GLint color)
{
	if(color != m_colBG_Raw)
	{
		m_colBG_Raw = color;
		m_colBG = MPRColor2D3DRGBA(color);
	}
}

void ContextMPR::ApplyStateBlock(GLint state)
{
	if(state == -1) return;
	ShiAssert(state >= 0 && state < MAXIMUM_MPR_STATE);

	if(state != lastState)
	{
		lastState = state;

		HRESULT hr = m_pD3DD->ApplyStateBlock(StateTable[state]);
		if(!SUCCEEDED(hr)) INT3;
	}
}

void ContextMPR::RestoreState(GLint state)
{
	ShiAssert(state != -1);
	ShiAssert(state >= 0 && state < MAXIMUM_MPR_STATE);

#if defined _DEBUG && defined _CONTEXT_ENABLE_RENDERSTATE_HIGHLIGHT_REPLACE
	if(GetKeyState(VK_F4) & ~1)
	{
		if(!bEnableRenderStateHighlightReplace)
			bEnableRenderStateHighlightReplace = true;
	}
	
	if(bEnableRenderStateHighlightReplace && (state == bRenderStateHighlightReplaceTargetState))
	{
		state = STATE_SOLID;
		m_colFG = 0xffff0000;
		currentState = -1;
	}
#endif

	if(state != currentState)
	{
	#ifdef _CONTEXT_TRACE_ALL
		MonoPrint("ContextMPR::RestoreState(%d)\n",state);
	#endif

	#ifdef _CONTEXT_RECORD_USED_STATES
		m_setStatesUsed.insert(state);
	#endif

		if(currentState == -1 || (StateTableInternal[currentState].SE_TEXTURING && !StateTableInternal[state].SE_TEXTURING))
			currentTexture1 = -1;

		currentState = state;

		if(!bZBuffering)
		{
			FlushVB();

			HRESULT hr = m_pD3DD->ApplyStateBlock(StateTable[currentState]);
			ShiAssert(SUCCEEDED(hr));
		}
	}
}

void ContextMPR::UpdateSpecularFog(DWORD specular)
{
	if(specular != m_colFOG) m_colFOG = specular;
}

void ContextMPR::SetZBuffering(BOOL state)
{
	if(!bZBuffering && state)
	{
		FlushVB();
		bZBuffering = state;
	}
	else if(bZBuffering && !state)
	{
		bZBuffering = state;
	}
}

void ContextMPR::SetNVGmode(BOOL state)
{
	NVGmode = state;
}

void ContextMPR::SetTVmode(BOOL state)
{
	TVmode = state;
}

void ContextMPR::SetIRmode(BOOL state)
{
	IRmode = state;
}

void ContextMPR::SetPalID(int id)
{
	if(id != palID) palID = id;
}

void ContextMPR::SetTexID(int id)
{
	if(id != texID) texID = id;
}

DWORD ContextMPR::MPRColor2D3DRGBA(GLint color)
{
	return RGBA_MAKE(RGBA_GETBLUE(color),RGBA_GETGREEN(color),RGBA_GETRED(color),RGBA_GETALPHA(color));
}

HRESULT WINAPI ContextMPR::EnumSurfacesCB2(IDirectDrawSurface7 *lpDDSurface, struct _DDSURFACEDESC2 *lpDDSurfaceDesc, LPVOID lpContext)
{
	ContextMPR *pThis = (ContextMPR *)lpContext;
	ShiAssert(FALSE == F4IsBadReadPtr(pThis,sizeof *pThis));
	ShiAssert(FALSE == F4IsBadReadPtr(lpDDSurfaceDesc,sizeof *lpDDSurfaceDesc));

	if(lpDDSurfaceDesc->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
	{
		pThis->m_pDDSP = lpDDSurface;
		return DDENUMRET_CANCEL;
	}

	return DDENUMRET_OK;
}

void ContextMPR::UpdateViewport()
{
	#ifdef _CONTEXT_TRACE_ALL
	MonoPrint("ContextMPR::UpdateViewport()\n");
	#endif

	if(m_bViewportLocked || !m_pD3DD)
		return;

	// get current viewport
	D3DVIEWPORT7 vp;
	HRESULT hr = m_pD3DD->GetViewport(&vp);
	ShiAssert(SUCCEEDED(hr));
	if(FAILED(hr)) return;	

	if(m_bEnableScissors)
	{
		// Set the viewport to the specified dimensions
		vp.dwX = m_rcVP.left;
		vp.dwY = m_rcVP.top;
		vp.dwWidth = m_rcVP.right - m_rcVP.left;
		vp.dwHeight = m_rcVP.bottom - m_rcVP.top;

		if(!vp.dwWidth || !vp.dwHeight)
			return;
	}
	else
	{
		// Set the viewport to the full target surface dimensions
		DDSURFACEDESC2 ddsd;
		ZeroMemory(&ddsd,sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);
		hr = m_pRenderTarget->GetSurfaceDesc(&ddsd);
		ShiAssert(SUCCEEDED(hr));
		if(FAILED(hr)) return;	

		vp.dwX = 0;
		vp.dwY = 0;
		vp.dwWidth = ddsd.dwWidth;
		vp.dwHeight = ddsd.dwHeight;
	}	

	hr = m_pD3DD->SetViewport(&vp);
	ShiAssert(SUCCEEDED(hr));
}

void ContextMPR::SetViewportAbs(int nLeft, int nTop, int nRight, int nBottom)
{
	if(m_bViewportLocked)
		return;

	m_rcVP.left = nLeft;
	m_rcVP.right = nRight;
	m_rcVP.top = nTop;
	m_rcVP.bottom = nBottom;

	UpdateViewport();
}

void ContextMPR::LockViewport()
{
	m_bViewportLocked = true;
}

void ContextMPR::UnlockViewport()
{
	m_bViewportLocked = false;
}

void ContextMPR::GetViewport(RECT *prc)
{
    ShiAssert(FALSE == F4IsBadWritePtr(prc,sizeof *prc));
	*prc = m_rcVP;
}

void ContextMPR::Stats()
{
	#ifdef _DEBUG
	if(m_bNoD3DStatsAvail)
		return;

	HRESULT hr;
	#ifdef _CONTEXT_USE_MANAGED_TEXTURES
	D3DDEVINFO_TEXTUREMANAGER ditexman;
	hr = m_pD3DD->GetInfo(D3DDEVINFOID_TEXTUREMANAGER,&ditexman,sizeof(ditexman));
	m_bNoD3DStatsAvail = FAILED(hr) || hr == S_FALSE;
	#endif

	D3DDEVINFO_TEXTURING ditex;
	hr = m_pD3DD->GetInfo(D3DDEVINFOID_TEXTURING,&ditex,sizeof(ditex));
	m_bNoD3DStatsAvail = FAILED(hr) || hr == S_FALSE;
	#endif
}

void ContextMPR::TextOut(short x, short y, DWORD col, LPSTR str)
{
	#ifdef _CONTEXT_TRACE_ALL
	MonoPrint("ContextMPR::TextOut(%d,%d,0x%X,%s)\n",x,y,col,str);
	#endif

	if(!str) return;

	try
	{
		HDC hdc;

		// Get GDI Device context for Surface
		CheckHR(m_pRenderTarget->GetDC(&hdc));

		if(hdc)
		{
			::SetBkMode(hdc,TRANSPARENT);
			::SetTextColor(hdc,col);
			::MoveToEx(hdc,x,y,NULL);

			::DrawText(hdc,str,strlen(str),&m_rcVP,DT_LEFT);

			CheckHR(m_pRenderTarget->ReleaseDC(hdc));
		}
	}

	catch(_com_error e)
	{
	}
}

bool ContextMPR::LockVB(int nVtxCount, void **p)
{
#ifdef _CONTEXT_TRACE_ALL
	MonoPrint("ContextMPR::LockVB(%d,0x%X) (m_dwStartVtx = %d,m_dwNumVtx = %d)\n",nVtxCount,p,m_dwStartVtx,m_dwNumVtx);
#endif

	HRESULT hr;
	DWORD dwSize = 0;

	ShiAssert(FALSE == F4IsBadReadPtr (m_pVB,sizeof *m_pVB));

	// Check for VB overflow
	if((m_dwStartVtx + m_dwNumVtx + nVtxCount) >= m_dwVBSize)
	{
		// would overflow
		FlushVB();
		m_dwStartVtx = 0;

		// we are done with this VB, hint driver that he can use a another memory block to prevent breaking DMA activity
		hr = m_pVB->Lock(DDLOCK_SURFACEMEMORYPTR|DDLOCK_WRITEONLY|DDLOCK_WAIT|DDLOCK_DISCARDCONTENTS,p,&dwSize);
	}

	else if(m_pTLVtx)
	{
		// already locked, excellent
		return true;
	}

	else
	{
		// we will only append data, dont interrupt DMA
		if(m_dwStartVtx)
			hr = m_pVB->Lock(DDLOCK_SURFACEMEMORYPTR|DDLOCK_WRITEONLY|DDLOCK_WAIT|DDLOCK_NOOVERWRITE,p,&dwSize);
		// ok this is the first lock
		else
			hr = m_pVB->Lock(DDLOCK_SURFACEMEMORYPTR|DDLOCK_WRITEONLY|DDLOCK_WAIT|DDLOCK_DISCARDCONTENTS,p,&dwSize);
	}

	ShiAssert(SUCCEEDED(hr));

#ifdef _DEBUG
	if(SUCCEEDED(hr)) m_pVtxEnd = (BYTE *)*p + dwSize;
	else m_pVtxEnd = NULL;
#endif

	return SUCCEEDED(hr);
}

void ContextMPR::UnlockVB()
{
#ifdef _CONTEXT_TRACE_ALL
	MonoPrint("ContextMPR::UnlockVB()\n");
#endif

	// Unlock VB
	HRESULT hr = m_pVB->Unlock();
	ShiAssert(SUCCEEDED(hr));
	m_pTLVtx = NULL;
}

void ContextMPR::FlushPolyLists()
{
	SetState(MPR_STA_ENABLES,MPR_SE_Z_WRITE);
	SetState(MPR_STA_ENABLES,MPR_SE_Z_BUFFERING);

	if(plainPolys != NULL) RenderPolyList(plainPolys);
	if(texturedPolys != NULL) RenderPolyList(texturedPolys);

	SetState(MPR_STA_DISABLES,MPR_SE_Z_WRITE);

	if(translucentPolys != NULL) RenderPolyList(translucentPolys);

	mIdx = 0;
	plainPolys = texturedPolys = translucentPolys = NULL;
	plainPolyVCnt = texturedPolyVCnt = translucentPolyVCnt = 0;

	AllocResetPool();
	SetZBuffering(FALSE);
	SetState(MPR_STA_DISABLES,MPR_SE_Z_BUFFERING);
}

void ContextMPR::FlushVB()
{
	if(!m_dwNumVtx) return;
	ShiAssert(m_nCurPrimType != 0);

#ifdef _CONTEXT_TRACE_ALL
	MonoPrint("ContextMPR::FlushVB()\n");
#endif

	int nPrimType = m_nCurPrimType;

	// Convert triangle fans to triangle lists to make them batchable
	if(nPrimType == D3DPT_TRIANGLEFAN)
	{
		nPrimType = D3DPT_TRIANGLELIST;
		ShiAssert(m_dwNumIdx);
	}

	// Convert line strips to line lists to make them batchable
	else if(nPrimType == D3DPT_LINESTRIP)
	{
		nPrimType = D3DPT_LINELIST;
		ShiAssert(m_dwNumIdx);
	}

	HRESULT hr;

	UnlockVB();

#ifdef _VALIDATE_DEVICE
	if(!m_pCtxDX->ValidateD3DDevice())
		MonoPrint("ContextMPR::FlushVB() - Validate Device failed - currentState=%d,currentTexture=0x%\n",currentState,currentTexture);
#endif

#ifdef _CONTEXT_ENABLE_RENDERSTATE_HIGHLIGHT
	hr = m_pD3DD->ApplyStateBlock(StateTable[STATE_SOLID]);
	ShiAssert(SUCCEEDED(hr));
#endif

	if(m_dwNumIdx)
		hr = m_pD3DD->DrawIndexedPrimitiveVB((D3DPRIMITIVETYPE)nPrimType,m_pVB,m_dwStartVtx,m_dwNumVtx,m_pIdx,m_dwNumIdx,NULL);
	else
		hr = m_pD3DD->DrawPrimitiveVB((D3DPRIMITIVETYPE) nPrimType,m_pVB,m_dwStartVtx,m_dwNumVtx,NULL);

	ShiAssert(SUCCEEDED(hr));

#ifdef _CONTEXT_ENABLE_STATS
	m_stats.StartBatch();
#endif

	m_dwStartVtx += m_dwNumVtx;
	m_dwNumVtx = 0;
	m_dwNumIdx = 0;
}

void ContextMPR::SetPrimitiveType(int nType)
{
#ifdef _CONTEXT_TRACE_ALL
	MonoPrint("ContextMPR::SetPrimitiveType(%d)\n",nType);
#endif

	if(m_nCurPrimType != nType)
	{
		// Flush on changed primitive type
		FlushVB();
		m_nCurPrimType = nType;
	}
}

void ContextMPR::SetView(LPD3DMATRIX l_pMV)
{
	memcpy(&mV,l_pMV,sizeof(D3DMATRIX));
}

void ContextMPR::SetWorld(LPD3DMATRIX l_pMW)
{
	ShiAssert(mIdx < 4096);

	memcpy(&mW[mIdx++],l_pMW,sizeof(D3DMATRIX));
}

void ContextMPR::SetProjection(LPD3DMATRIX l_pMP)
{
	memcpy(&mP,l_pMP,sizeof(D3DMATRIX));
}

void ContextMPR::setGlobalZBias(float zBias)
{
	if(gZBias != zBias) gZBias = zBias;
	ZCX_Calculate();							// COBRA - RED - Drawing CXs update
}

inline TLVERTEX* SPolygon::CopyToVertexBuffer(TLVERTEX *bufferPos)
{
	SVertex *curVert = pVertexList;

	for(int i = numVertices; i > 0; i--)
	{
		memcpy(bufferPos,curVert,sizeof(TLVERTEX)); 

		bufferPos++;			
		curVert = curVert->pNext;
	}

	return bufferPos;
}


// COBRA - RED - PolyZ is caclulated step by step on each Vertex stuff, saving time, so the average is calculated
// on the passed value
inline void SPolygon::CalcPolyZ(float Avg)
{
	Avg /= float(numVertices);
	zBuffer = FloatToInt32(Avg * 16777215.f);
}

inline void ContextMPR::AllocatePolygon(SPolygon *&curPoly, const DWORD numVertices)
{
	curPoly = (SPolygon *)Alloc(sizeof(SPolygon)+numVertices*sizeof(SVertex));
	curPoly->numVertices = numVertices;
	curPoly->pVertexList = (SVertex *)(DWORD(curPoly)+sizeof(SPolygon));

/*	SVertex *curVert = curPoly->pVertexList;

	for(int i = numVertices-1; i > 0; i--,curVert++)
	{
		curVert->pNext = (curVert+1);
	}

	curVert->pNext = curPoly->pVertexList;*/
}

inline void ContextMPR::AddPolygon(SPolygon *&polyList, SPolygon *&curPoly)
{
	curPoly->pNext = polyList;
	polyList = curPoly;
}

void ContextMPR::RenderPolyList(SPolygon *&pHead)
{
	TLVERTEX *pIns;
	SPolygon *pStart,*pEnd,*pCur;
	DWORD offset,vertcnt = 0,verttot = 0;

	if(pHead->renderState >= STATE_ALPHA_SOLID && pHead->renderState <= STATE_ALPHA_TEXTURE_PERSPECTIVE_CLAMP)
	{
		offset = DWORD(&pHead->zBuffer)-DWORD(pHead);
		pHead = (SPolygon *)RadixSortDescending((radix_sort_t *)pHead,offset);
	}

	pStart = pEnd = pCur = pHead;
	while(pEnd != NULL)
	{
		vertcnt += pEnd->numVertices;
		pEnd = pEnd->pNext;

		if(pEnd == NULL || vertcnt + pEnd->numVertices > 1024)
		{
			m_pVBB->Lock(DDLOCK_WRITEONLY|DDLOCK_SURFACEMEMORYPTR|DDLOCK_DISCARDCONTENTS,(LPVOID *)&pIns,NULL);

			while(pEnd != pCur)
			{
				pIns = pCur->CopyToVertexBuffer(pIns);
				pCur = pCur->pNext;
			}

			m_pVBB->Unlock();
	
			vertcnt = 0;
			pCur = pStart;

			while(pEnd != pCur)
			{
				ApplyStateBlock(pCur->renderState);

				if((pCur->renderState > STATE_GOURAUD && pCur->renderState < STATE_ALPHA_SOLID) || pCur->renderState > STATE_ALPHA_GOURAUD)
					SetTexture1(pCur->textureID0);
				else
					SetTexture1(-1);

				if(pCur->renderState >=	STATE_MULTITEXTURE)
					SetTexture2(pCur->textureID1);
	
				verttot = pCur->numVertices;
	 			m_pD3DD->DrawPrimitiveVB(D3DPT_TRIANGLEFAN,m_pVBB,vertcnt,verttot,0);
		
				vertcnt += verttot;
				pCur = pCur->pNext;
			}

			pCur = pStart = pEnd;
			vertcnt = 0;
		}
	}
}

void ContextMPR::DrawPoly(DWORD opFlag, Poly *poly, int *xyzIdxPtr, int *rgbaIdxPtr, int *IIdxPtr, Ptexcoord *uv, bool bUseFGColor)
{
	float *I;
	Spoint *xyz;
	Pcolor *rgba;
	TLVERTEX *pVtx;
	SVertex	*sVertex;
	SPolygon *sPolygon;
	float	PolyZAvg=0;
	
	// Incoming type is always MPR_PRM_TRIFAN
	ShiAssert(FALSE == F4IsBadReadPtr(poly,sizeof *poly));
	ShiAssert(poly->nVerts >= 3);
	ShiAssert(xyzIdxPtr);
	ShiAssert(!bUseFGColor || (bUseFGColor && rgbaIdxPtr == NULL));

#ifdef _CONTEXT_TRACE_ALL
	MonoPrint("ContextMPR::DrawPoly(0x%X,0x%X,0x%X,0x%X,0x%X,0x%X,%s)\n",
		opFlag,poly,xyzIdxPtr,rgbaIdxPtr,IIdxPtr,uv,bUseFGColor ? "true" : "false");
#endif

#ifdef _CONTEXT_ENABLE_STATS
	m_stats.Primitive(D3DPT_TRIANGLEFAN,poly->nVerts);
#endif

	if(!bZBuffering)
	{
		// Lock VB
		if(!LockVB(poly->nVerts,(void **)&m_pTLVtx))
		{
			m_colFOG = 0xFFFFFFFF;
			return;
		}

		ShiAssert(FALSE == F4IsBadWritePtr(m_pTLVtx,sizeof *m_pTLVtx));	
		ShiAssert(m_dwStartVtx < m_dwVBSize);
		pVtx = &m_pTLVtx[m_dwStartVtx + m_dwNumVtx];
		ShiAssert(FALSE == F4IsBadWritePtr(pVtx,poly->nVerts * sizeof *pVtx));

		// JB 011124 CTD
		if(!pVtx)
		{
			m_colFOG = 0xFFFFFFFF;
			return;
		}

		SetPrimitiveType(D3DPT_TRIANGLEFAN);
	}
	else
	{
		AllocatePolygon(sPolygon,poly->nVerts);
		sPolygon->renderState = currentState;
		sPolygon->textureID0 = currentTexture1;
		sPolygon->pNext = NULL;
		sVertex = sPolygon->pVertexList;
	}

	// Iterate for each vertex
	if(!bZBuffering)
	{
		for(int i = 0; i < poly->nVerts; i++)
		{
			// Check for overrun
			ShiAssert((BYTE *)pVtx < m_pVtxEnd);
	
			xyz  = &TheStateStack.XformedPosPool[*xyzIdxPtr++];
	
						
			if (DisplayOptions.bScreenCoordinateBiasFix)		//Wombat778 4-01-04
			{
				pVtx->sx = xyz->x-0.5f;
				pVtx->sy = xyz->y-0.5f;
			}
			else
			{
				pVtx->sx = xyz->x;
				pVtx->sy = xyz->y;
			}

			// NOTE: HACK!!
			if(xyz->z > 5)
				pVtx->sz = SCALE_SZ(xyz->z);					// COBRA - RED - Using precomputed CXs
			else
				pVtx->sz = 0.f;

			pVtx->rhw = 1.f/xyz->z;
			pVtx->specular = m_colFOG;

			// End Mission box
			if(texID > 25 && texID < 32)
				pVtx->color = 0xFFFFFFFF;
			else
				pVtx->color = TheColorBank.TODcolor;

			if(opFlag&PRIM_COLOP_COLOR)
			{
				ShiAssert(rgbaIdxPtr);
				rgba = &TheColorBank.ColorPool[*rgbaIdxPtr++];
				
				ShiAssert(rgba);
				if(rgba)
				{
					if(opFlag & PRIM_COLOP_INTENSITY)
					{
						ShiAssert(IIdxPtr);
						I = &TheStateStack.IntensityPool[*IIdxPtr++];
						pVtx->color = D3DRGBA(rgba->r * *I,rgba->g * *I,rgba->b * *I,rgba->a);
					}

					else
					{
						pVtx->color = D3DRGBA(rgba->r,rgba->g,rgba->b,rgba->a);
					}
				}
			}
			else if(opFlag&PRIM_COLOP_INTENSITY)
			{
				ShiAssert(IIdxPtr);
	
				I = &TheStateStack.IntensityPool[*IIdxPtr++];
				pVtx->color = D3DRGBA(*I,*I,*I,1.f);
			}
			else if(bUseFGColor)
				pVtx->color = m_colFG;
			else
			{
				// Set the light level for the "special building lights"
	 			if(palID == 3)
	 				pVtx->color = TheColorBank.TODcolor;
					// Set the light level with "special cockpit reflection alpha"
				else if(palID == 2)
					pVtx->color = TheColorBank.TODcolor&0x26FFFFFF;
			}

			if(opFlag&PRIM_COLOP_TEXTURE)
			{
				// NVG_LIGHT_LEVEL = 0.703125f
				if(NVGmode || TVmode || IRmode)
				{
					pVtx->color &= 0xFF00FF00;
					pVtx->color |= 0x0000B400;
				}

				ShiAssert(uv);
	
				pVtx->tu0 = pVtx->tu1 = uv->u;
				pVtx->tv0 = pVtx->tv1 = uv->v;

				uv++;
			}
			else
			{
				pVtx->tu0 = 0;
				pVtx->tv0 = 0;
			}
	
			pVtx++;
		}
	}
	else
	{	
		int	i=poly->nVerts;
		//for(int i = 0; i < poly->nVerts; i++)
		while(i--)
		{
			xyz  = &TheStateStack.XformedPosPool[*xyzIdxPtr++];
	
			sVertex->sx = xyz->x;
			sVertex->sy = xyz->y;
	
			// NOTE: HACK!!
			if(xyz->z > 5)
				sVertex->sz = SCALE_SZ(xyz->z);					// COBRA - RED - Using precomputed CXs;
			else
				sVertex->sz = 0.f;

			sVertex->rhw = 1.f/xyz->z;
			sVertex->specular = m_colFOG;

			// End Mission box
			if(texID > 25 && texID < 32)
				sVertex->color = 0xFFFFFFFF;
			else
				sVertex->color = TheColorBank.TODcolor;

			if(opFlag&PRIM_COLOP_COLOR)
			{
				ShiAssert(rgbaIdxPtr);
				rgba = &TheColorBank.ColorPool[*rgbaIdxPtr++];
				
				ShiAssert(rgba);
				if(rgba)
				{
					if(opFlag & PRIM_COLOP_INTENSITY)
					{
						ShiAssert(IIdxPtr);
						I = &TheStateStack.IntensityPool[*IIdxPtr++];
						sVertex->color = D3DRGBA(rgba->r * *I,rgba->g * *I,rgba->b * *I,rgba->a);
					}

					else
					{
						sVertex->color = D3DRGBA(rgba->r,rgba->g,rgba->b,rgba->a);
					}
				}
			}
			else if(opFlag&PRIM_COLOP_INTENSITY)
			{
				ShiAssert(IIdxPtr);
	
				I = &TheStateStack.IntensityPool[*IIdxPtr++];
				sVertex->color = D3DRGBA(*I,*I,*I,1.f);
			}
			else if(bUseFGColor)
				sVertex->color = m_colFG;
			else
			{
				// Set the light level for the "special building lights"
	 			if(palID == 3)
	 				sVertex->color = TheColorBank.TODcolor;
				// Set the light level with "special cockpit reflection alpha"
				else if(palID == 2)
					sVertex->color = TheColorBank.TODcolor&0x26FFFFFF;
			}

			if(opFlag&PRIM_COLOP_TEXTURE)
			{
				// NVG_LIGHT_LEVEL = 0.703125f
				if(NVGmode || TVmode || IRmode)
				{
					sVertex->color &= 0xFF00FF00;
					sVertex->color |= 0x0000B400;
				}

				ShiAssert(uv);
	
				sVertex->tu0 = sVertex->tu1 = uv->u;
				sVertex->tv0 = sVertex->tv1 = uv->v;

				uv++;
			}
			else
			{
				sVertex->tu0 = 0;
				sVertex->tv0 = 0;
			}
			PolyZAvg+=sVertex->sz;			// COBRA - RED - Poly Z Sum is calculated onthe fly

			// COBRA - RED - Linking of vertexes is done on the fly 
			if(i) sVertex = sVertex->pNext = sVertex+1;
			else sVertex->pNext= sPolygon->pVertexList;
		}
	}

	// Generate Indices
	if(!bZBuffering)
	{
		WORD *pIdx = &m_pIdx[m_dwNumIdx];

		for(int x = 0; x < poly->nVerts-2; x++)
		{
			pIdx[0] = m_dwNumVtx;
			pIdx[1] = m_dwNumVtx + x + 1;
			pIdx[2] = m_dwNumVtx + x + 2;
			pIdx += 3;
		}

		m_dwNumIdx += pIdx - &m_pIdx[m_dwNumIdx];
		m_dwNumVtx += poly->nVerts;

	#ifdef _CONTEXT_FLUSH_EVERY_PRIMITIVE
		FlushVB();
	#endif
	}
	else
	{
		sPolygon->CalcPolyZ(PolyZAvg);

		// Double-textured
		if(sPolygon->renderState >= STATE_MULTITEXTURE)
		{
			texturedPolyVCnt += poly->nVerts;
			AddPolygon(texturedPolys,sPolygon);
		}
		// Translucent
		else if(sPolygon->renderState >= STATE_ALPHA_SOLID)
		{
			translucentPolyVCnt += poly->nVerts;
			AddPolygon(translucentPolys,sPolygon);
		}
		// Textured
		else if(sPolygon->renderState >= STATE_TEXTURE)
		{
			texturedPolyVCnt += poly->nVerts;
			AddPolygon(texturedPolys,sPolygon);
		}
		// Plain
		else if(sPolygon->renderState >= STATE_SOLID)
		{
			plainPolyVCnt += poly->nVerts;
			AddPolygon(plainPolys,sPolygon);
		}
		else
			INT3;
	}

	m_colFOG = 0xFFFFFFFF;
}

void ContextMPR::Draw2DPoint(Tpoint *v0)
{
	ShiAssert(v0);

	#ifdef _CONTEXT_TRACE_ALL
	MonoPrint("ContextMPR::Draw2DPoint(0x%X)\n",v0);
	#endif

	SetPrimitiveType(D3DPT_POINTLIST);

	#ifdef _CONTEXT_ENABLE_STATS
	m_stats.Primitive(m_nCurPrimType,1);
	#endif

	// Lock VB
	TLVERTEX *pVtx;
	if(!LockVB(1,(void **)&m_pTLVtx))
	{
		m_colFOG = 0xFFFFFFFF;
		return;
	}

	ShiAssert(FALSE == F4IsBadWritePtr(m_pTLVtx,sizeof *m_pTLVtx));	
	ShiAssert(m_dwStartVtx < m_dwVBSize);
	pVtx = &m_pTLVtx[m_dwStartVtx + m_dwNumVtx];
	ShiAssert(FALSE == F4IsBadWritePtr(pVtx,sizeof *pVtx));

	// Check for overrun
	ShiAssert((BYTE *)pVtx < m_pVtxEnd);

	if (DisplayOptions.bScreenCoordinateBiasFix)		//Wombat778 4-01-04
	{
		pVtx->sx = v0->x-0.5f;
		pVtx->sy = v0->y-0.5f;
	}
	else
	{
		pVtx->sx = v0->x;
		pVtx->sy = v0->y;
	}

	if(v0->z)
		pVtx->sz = SCALE_SZ(v0->z);					// COBRA - RED - Using precomputed CXs
	else
		pVtx->sz = 0.f;

	pVtx->rhw = 1.0f;
	pVtx->color = m_colFG;
	pVtx->specular = m_colFOG;
	pVtx->tu0 = 0;
	pVtx->tv0 = 0;

	#ifdef _CONTEXT_ENABLE_RENDERSTATE_HIGHLIGHT
	pVtx->color = currentState != -1 ? RGBA_MAKE((currentState << 1) + 50,(currentState << 1) + 50,(currentState << 1) + 50,(currentState << 1) + 50) : D3DRGBA(1.0f,1.0f,1.0f,1.0f);
	#endif

	m_dwNumVtx++;

	#ifdef _CONTEXT_FLUSH_EVERY_PRIMITIVE
	FlushVB();
	#endif

	m_colFOG = 0xFFFFFFFF;
}

void ContextMPR::Draw2DPoint(float x, float y)
{
	#ifdef _CONTEXT_TRACE_ALL
	MonoPrint("ContextMPR::Draw2DPoint(%f,%f)\n",x,y);
	#endif

	SetPrimitiveType(D3DPT_POINTLIST);

	#ifdef _CONTEXT_ENABLE_STATS
	m_stats.Primitive(m_nCurPrimType,1);
	#endif

	// Lock VB
	TLVERTEX *pVtx;
	if(!LockVB(1,(void **)&m_pTLVtx))
	{
		m_colFOG = 0xFFFFFFFF;
		return;
	}

	ShiAssert(FALSE == F4IsBadWritePtr(m_pTLVtx,sizeof *m_pTLVtx));	
	ShiAssert(m_dwStartVtx < m_dwVBSize);
	pVtx = &m_pTLVtx[m_dwStartVtx + m_dwNumVtx];

	// Check for overrun
	ShiAssert((BYTE *)pVtx < m_pVtxEnd);	
	ShiAssert(FALSE == F4IsBadWritePtr(pVtx,sizeof *pVtx));

	if(DisplayOptions.bScreenCoordinateBiasFix)		//Wombat778 4-01-04
	{
		pVtx->sx = x-0.5f;
		pVtx->sy = y-0.5f;
	}
	else
	{
		pVtx->sx = x;
		pVtx->sy = y;
	}
	pVtx->sz = 0.0f;
	pVtx->rhw = 1.0f;
	pVtx->color = m_colFG;
	pVtx->specular = m_colFOG;
	pVtx->tu0 = 0;
	pVtx->tv0 = 0;

	#ifdef _CONTEXT_ENABLE_RENDERSTATE_HIGHLIGHT
	pVtx->color = currentState != -1 ? RGBA_MAKE((currentState << 1) + 50,(currentState << 1) + 50,(currentState << 1) + 50,(currentState << 1) + 50) : D3DRGBA(1.0f,1.0f,1.0f,1.0f);
	#endif

	m_dwNumVtx++;

	#ifdef _CONTEXT_FLUSH_EVERY_PRIMITIVE
	FlushVB();
	#endif

	m_colFOG = 0xFFFFFFFF;
}

void ContextMPR::Draw2DLine(Tpoint *v0, Tpoint *v1)
{
	ShiAssert(v0 && v1);

	#ifdef _CONTEXT_TRACE_ALL
	MonoPrint("ContextMPR::Draw2DLine(0x%X,0x%X)\n",v0,v1);
	#endif

	SetPrimitiveType(D3DPT_LINESTRIP);

	#ifdef _CONTEXT_ENABLE_STATS
	m_stats.Primitive(m_nCurPrimType,2);
	#endif

	// Lock VB
	TLVERTEX *pVtx;
	if(!LockVB(2,(void **)&m_pTLVtx))
	{
		m_colFOG = 0xFFFFFFFF;
		return;
	}

	ShiAssert(FALSE == F4IsBadWritePtr(m_pTLVtx,sizeof *m_pTLVtx));	
	ShiAssert(m_dwStartVtx < m_dwVBSize);
	pVtx = &m_pTLVtx[m_dwStartVtx + m_dwNumVtx];

	// Check for overrun
	ShiAssert((BYTE *)pVtx < m_pVtxEnd);	
	ShiAssert(FALSE == F4IsBadWritePtr(pVtx,2*sizeof *pVtx));

	if(DisplayOptions.bScreenCoordinateBiasFix)		//Wombat778 4-01-04
	{
		pVtx->sx = v0->x-0.5f;
		pVtx->sy = v0->y-0.5f;
	}
	else
	{
		pVtx->sx = v0->x;
		pVtx->sy = v0->y;
	}

	if(v0->z)
		pVtx->sz = SCALE_SZ(v0->z);					// COBRA - RED - Using precomputed CXs
	else
		pVtx->sz = 0.f;

	// JB 010220 CTD
	if (v0->z)
		pVtx->rhw = 1.0f / v0->z;

	pVtx->color = m_colFG;
	pVtx->specular = m_colFOG;
	pVtx->tu0 = 0;
	pVtx->tv0 = 0;
	pVtx++;

	#ifdef _CONTEXT_ENABLE_RENDERSTATE_HIGHLIGHT
	pVtx->color = currentState != -1 ? RGBA_MAKE((currentState << 1) + 50,(currentState << 1) + 50,(currentState << 1) + 50,(currentState << 1) + 50) : D3DRGBA(1.0f,1.0f,1.0f,1.0f);
	#endif

	if(DisplayOptions.bScreenCoordinateBiasFix)		//Wombat778 4-01-04
	{
		pVtx->sx = v1->x-0.5f;
		pVtx->sy = v1->y-0.5f;
	}
	else
	{
		pVtx->sx = v1->x;
		pVtx->sy = v1->y;
	}

	if(v1->z)
		pVtx->sz = (ZFAR/(ZFAR-ZNEAR))+(ZFAR*ZNEAR/(ZNEAR-ZFAR))/v1->z;
	else
		pVtx->sz = 0.f;

	pVtx->rhw = 1.0f / v1->z;
	pVtx->color = m_colFG;
	pVtx->specular = m_colFOG;
	pVtx->tu0 = 0;
	pVtx->tv0 = 0;

	#ifdef _CONTEXT_ENABLE_RENDERSTATE_HIGHLIGHT
	pVtx->color = currentState != -1 ? RGBA_MAKE((currentState << 1) + 50,(currentState << 1) + 50,(currentState << 1) + 50,(currentState << 1) + 50) : D3DRGBA(1.0f,1.0f,1.0f,1.0f);
	#endif

	WORD *pIdx = &m_pIdx[m_dwNumIdx];
	*pIdx++ = m_dwNumVtx;
	*pIdx++ = m_dwNumVtx + 1;

	m_dwNumIdx += 2;
	m_dwNumVtx += 2;

	#ifdef _CONTEXT_FLUSH_EVERY_PRIMITIVE
	FlushVB();
	#endif

	m_colFOG = 0xFFFFFFFF;
}


void ContextMPR::Draw2DLine(float x0, float y0, float x1, float y1)
{
	#ifdef _CONTEXT_TRACE_ALL
	MonoPrint("ContextMPR::Draw2DLine(0x%X,0x%X)\n",x0,y0);
	#endif

	SetPrimitiveType(D3DPT_LINESTRIP);

	#ifdef _CONTEXT_ENABLE_STATS
	m_stats.Primitive(m_nCurPrimType,2);
	#endif

	// Lock VB
	TLVERTEX *pVtx;
	if(!LockVB(2,(void **)&m_pTLVtx))
	{
		m_colFOG = 0xFFFFFFFF;
		return;
	}

	ShiAssert(FALSE == F4IsBadWritePtr(m_pTLVtx,sizeof *m_pTLVtx));	
	ShiAssert(m_dwStartVtx < m_dwVBSize);
	pVtx = &m_pTLVtx[m_dwStartVtx + m_dwNumVtx];
	ShiAssert(FALSE == F4IsBadWritePtr(pVtx,2* sizeof *pVtx));

	// Check for overrun
	ShiAssert((BYTE *)pVtx < m_pVtxEnd);

	if(DisplayOptions.bScreenCoordinateBiasFix)		//Wombat778 4-01-04
	{
		pVtx->sx = x0-0.5f;
		pVtx->sy = y0-0.5f;
	}
	else
	{
		pVtx->sx = x0;
		pVtx->sy = y0;
	}
	pVtx->sz = 0.0f;
	pVtx->rhw = 1.0f;
	pVtx->color = m_colFG;
	pVtx->specular = m_colFOG;
	pVtx->tu0 = 0;
	pVtx->tv0 = 0;
	pVtx++;

	#ifdef _CONTEXT_ENABLE_RENDERSTATE_HIGHLIGHT
	pVtx->color = currentState != -1 ? RGBA_MAKE((currentState << 1) + 50,(currentState << 1) + 50,(currentState << 1) + 50,(currentState << 1) + 50) : D3DRGBA(1.0f,1.0f,1.0f,1.0f);
	#endif

	if(DisplayOptions.bScreenCoordinateBiasFix)		//Wombat778 4-01-04
	{
		pVtx->sx = x1-0.5f;
		pVtx->sy = y1-0.5f;
	}
	else
	{
		pVtx->sx = x1;
		pVtx->sy = y1;
	}
	pVtx->sz = 0.0f;
	pVtx->rhw = 1.0f;
	pVtx->color = m_colFG;
	pVtx->specular = m_colFOG;
	pVtx->tu0 = 0;
	pVtx->tv0 = 0;

	#ifdef _CONTEXT_ENABLE_RENDERSTATE_HIGHLIGHT
	pVtx->color = currentState != -1 ? RGBA_MAKE((currentState << 1) + 50,(currentState << 1) + 50,(currentState << 1) + 50,(currentState << 1) + 50) : D3DRGBA(1.0f,1.0f,1.0f,1.0f);
	#endif

	WORD *pIdx = &m_pIdx[m_dwNumIdx];
	*pIdx++ = m_dwNumVtx;
	*pIdx++ = m_dwNumVtx + 1;

	m_dwNumIdx += 2;
	m_dwNumVtx += 2;

	#ifdef _CONTEXT_FLUSH_EVERY_PRIMITIVE
	FlushVB();
	#endif

	m_colFOG = 0xFFFFFFFF;
}

void ContextMPR::DrawPrimitive2D(int type, int nVerts, int *xyzIdxPtr)
{
	ShiAssert(xyzIdxPtr);

	#ifdef _CONTEXT_TRACE_ALL
	MonoPrint("ContextMPR::DrawPrimitive2D(%d,%d,0x%X)\n",type,nVerts,xyzIdxPtr);
	#endif

	SetPrimitiveType(type == LineF ? D3DPT_LINESTRIP : D3DPT_POINTLIST);

	#ifdef _CONTEXT_ENABLE_STATS
	m_stats.Primitive(m_nCurPrimType,nVerts);
	#endif

	// Lock VB
	TLVERTEX *pVtx;
	if(!LockVB(nVerts,(void **)&m_pTLVtx))
	{
		m_colFOG = 0xFFFFFFFF;
		return;
	}

	ShiAssert(FALSE == F4IsBadWritePtr(m_pTLVtx,sizeof *m_pTLVtx));	
	ShiAssert(m_dwStartVtx < m_dwVBSize);
	pVtx = &m_pTLVtx[m_dwStartVtx + m_dwNumVtx];
	ShiAssert(FALSE == F4IsBadWritePtr(pVtx,nVerts * sizeof *pVtx));

	Ppoint *xyz;

	// Iterate for each vertex
	for(int i=0;i<nVerts;i++)
	{
	    ShiAssert(*xyzIdxPtr < MAX_VERT_POOL_SIZE);
		xyz = &TheStateStack.XformedPosPool[*xyzIdxPtr++];

		// Check for overrun
		ShiAssert((BYTE *)pVtx < m_pVtxEnd);

		if(DisplayOptions.bScreenCoordinateBiasFix)		//Wombat778 4-01-04
		{
			pVtx->sx = xyz->x-0.5f;
			pVtx->sy = xyz->y-0.5f;
		}
		else
		{
			pVtx->sx = xyz->x;
			pVtx->sy = xyz->y;
		}

		if(xyz->z)
			pVtx->sz = SCALE_SZ(xyz->z);					// COBRA - RED - Using precomputed CXs;
		else
			pVtx->sz = 0.f;

		// JB 010305
		if (xyz->z)
			pVtx->rhw = 1.0f / xyz->z;

		pVtx->color = m_colFG;
		pVtx->specular = m_colFOG;
		pVtx->tu0 = 0;
		pVtx->tv0 = 0;

		#ifdef _CONTEXT_ENABLE_RENDERSTATE_HIGHLIGHT
		pVtx->color = currentState != -1 ? RGBA_MAKE((currentState << 1) + 50,(currentState << 1) + 50,(currentState << 1) + 50,(currentState << 1) + 50) : D3DRGBA(1.0f,1.0f,1.0f,1.0f);
		#endif

		pVtx++;
	}

	// Generate Indices
	if(m_nCurPrimType == D3DPT_LINESTRIP)
	{
		WORD *pIdx = &m_pIdx[m_dwNumIdx];

		for(int x=0;x<nVerts;x++)
			*pIdx++ = m_dwNumVtx + x;

		m_dwNumIdx += nVerts;
	}

	m_dwNumVtx += nVerts;

	#ifdef _CONTEXT_FLUSH_EVERY_PRIMITIVE
	FlushVB();
	#endif

	m_colFOG = 0xFFFFFFFF;
}

void ContextMPR::DrawPrimitive(int nPrimType, WORD VtxInfo, WORD nVerts, MPRVtx_t *pData, WORD Stride)
{
	// Impossible
	ShiAssert(!(VtxInfo & MPR_VI_COLOR));

	// Ensure no degenerate nPrimTypeitives
	ShiAssert((nVerts >=3) || (nPrimType==MPR_PRM_POINTS && nVerts >=1) || (nPrimType<=MPR_PRM_POLYLINE && nVerts >=2));

	#ifdef _CONTEXT_TRACE_ALL
	MonoPrint("ContextMPR::DrawPrimitive(%d,0x%X,%d,0x%X,%d)\n",nPrimType,VtxInfo,nVerts,pData,Stride);
	#endif

	SetPrimitiveType(nPrimType);

	#ifdef _CONTEXT_ENABLE_STATS
	m_stats.Primitive(m_nCurPrimType,nVerts);
	#endif

	// Lock VB
	TLVERTEX *pVtx;
	if(!LockVB(nVerts,(void **)&m_pTLVtx))
	{ 
		m_colFOG = 0xFFFFFFFF;
		return;
	}

	ShiAssert(FALSE == F4IsBadWritePtr(m_pTLVtx,sizeof *m_pTLVtx));	
	ShiAssert(m_dwStartVtx < m_dwVBSize);
	pVtx = &m_pTLVtx[m_dwStartVtx + m_dwNumVtx];
	ShiAssert(FALSE == F4IsBadWritePtr(pVtx,nVerts * sizeof *pVtx));

	// Iterate for each vertex
	for(int i=0;i<nVerts;i++)
	{
		// Check for overrun
		ShiAssert((BYTE *)pVtx < m_pVtxEnd);

		if(DisplayOptions.bScreenCoordinateBiasFix)		//Wombat778 4-01-04
		{

			pVtx->sx = pData->x-0.5f;
			pVtx->sy = pData->y-0.5f;
		}
		else
		{
			pVtx->sx = pData->x;
			pVtx->sy = pData->y;
		}
		pVtx->sz = 0.f;
		pVtx->rhw = 1.0f;
		pVtx->color = m_colFG;
		pVtx->specular = m_colFOG;
		pVtx->tu0 = 0;
		pVtx->tv0 = 0;

		#ifdef _CONTEXT_ENABLE_RENDERSTATE_HIGHLIGHT
		pVtx->color = currentState != -1 ? RGBA_MAKE((currentState << 1) + 50,(currentState << 1) + 50,(currentState << 1) + 50,(currentState << 1) + 50) : D3DRGBA(1.0f,1.0f,1.0f,1.0f);
		#endif

		pVtx++;
		pData = (MPRVtx_t *)((BYTE *)pData + Stride);
	}

	// Generate Indices
	if(m_nCurPrimType == D3DPT_TRIANGLEFAN)
	{
		WORD *pIdx = &m_pIdx[m_dwNumIdx];

		for(int x=0;x<nVerts-2;x++)
		{
			pIdx[0] = m_dwNumVtx;
			pIdx[1] = m_dwNumVtx + x + 1;
			pIdx[2] = m_dwNumVtx + x + 2;
			pIdx += 3;
		}

		m_dwNumIdx += pIdx - &m_pIdx[m_dwNumIdx];
	}

	else if(m_nCurPrimType == D3DPT_LINESTRIP)
	{
		WORD *pIdx = &m_pIdx[m_dwNumIdx];

		for(int x=0;x<nVerts;x++)
			*pIdx++ = m_dwNumVtx + x;

		m_dwNumIdx += nVerts;
	}

	m_dwNumVtx += nVerts;

	#ifdef _CONTEXT_FLUSH_EVERY_PRIMITIVE
	FlushVB();
	#endif

	m_colFOG = 0xFFFFFFFF;
}

void ContextMPR::DrawPrimitive(int nPrimType, WORD VtxInfo, WORD nVerts, MPRVtxTexClr_t *pData, WORD Stride)
{
	TLVERTEX *pVtx;
	
	// Ensure no degenerate nPrimTypeitives
	ShiAssert((nVerts >=3) || (nPrimType==MPR_PRM_POINTS && nVerts >=1) || (nPrimType<=MPR_PRM_POLYLINE && nVerts >=2));

#ifdef _CONTEXT_TRACE_ALL
	MonoPrint("ContextMPR::DrawPrimitive2(%d,0x%X,%d,0x%X,%d)\n",nPrimType,VtxInfo,nVerts,pData,Stride);
#endif

#ifdef _CONTEXT_ENABLE_STATS
	m_stats.Primitive(m_nCurPrimType,nVerts);
#endif

	// Lock VB
	if(!LockVB(nVerts,(void **)&m_pTLVtx))
	{
		m_colFOG = 0xFFFFFFFF;
		return;
	}

	ShiAssert(FALSE == F4IsBadWritePtr(m_pTLVtx,sizeof *m_pTLVtx));	
	ShiAssert(m_dwStartVtx < m_dwVBSize);
	pVtx = &m_pTLVtx[m_dwStartVtx + m_dwNumVtx];

	ShiAssert(FALSE == F4IsBadWritePtr(pVtx,nVerts * sizeof *pVtx));

	// JB 011124 CTD
	if (!pVtx) 
	{
		m_colFOG = 0xFFFFFFFF;
		return;
	}

	SetPrimitiveType(nPrimType);

	// Iterate for each vertex
	for(int i = 0; i < nVerts; i++)
	{
		// Check for overrun
		ShiAssert((BYTE *)pVtx < m_pVtxEnd);
	
		if(DisplayOptions.bScreenCoordinateBiasFix)		//Wombat778 4-01-04
		{
			pVtx->sx = pData->x-0.5f;
			pVtx->sy = pData->y-0.5f;
		}
		else
		{
			pVtx->sx = pData->x;
			pVtx->sy = pData->y;
		}
		pVtx->sz = 0.f;

		// OW FIXME: this should be 1.0f / pData->z
		pVtx->rhw = 1.0f;

		if(VtxInfo == (MPR_VI_COLOR | MPR_VI_TEXTURE))
		{
			pVtx->color = D3DRGBA(pData->r,pData->g,pData->b,pData->a);
			pVtx->specular = m_colFOG;

			pVtx->tu0 = pData->u;
			pVtx->tv0 = pData->v;
		}
		else if(VtxInfo == MPR_VI_COLOR)
		{
			pVtx->color = D3DRGBA(pData->r,pData->g,pData->b,pData->a);
			pVtx->specular = m_colFOG;
		}
		else
		{
			pVtx->color = m_colFG;
			pVtx->specular = m_colFOG;
		}

	#ifdef _CONTEXT_ENABLE_RENDERSTATE_HIGHLIGHT
		pVtx->color = currentState != -1 ? RGBA_MAKE((currentState << 1) + 50,(currentState << 1) + 50,,(currentState << 1) + 50,(currentState << 1) + 50) : D3DRGBA(1.0f,1.0f,1.0f,1.0f);
	#endif

		pVtx++;
		pData = (MPRVtxTexClr_t *)((BYTE *)pData + Stride);
	}

	// Generate Indices (in advance)
	if(m_nCurPrimType == D3DPT_TRIANGLEFAN)
	{
		WORD *pIdx = &m_pIdx[m_dwNumIdx];

		for(int x=0;x<nVerts-2;x++)
		{
			pIdx[0] = m_dwNumVtx;
			pIdx[1] = m_dwNumVtx + x + 1;
			pIdx[2] = m_dwNumVtx + x + 2;
			pIdx += 3;
		}

		m_dwNumIdx += pIdx - &m_pIdx[m_dwNumIdx];
	}
	else if(m_nCurPrimType == D3DPT_LINESTRIP)
	{
		WORD *pIdx = &m_pIdx[m_dwNumIdx];

		for(int x=0;x<nVerts;x++)
			*pIdx++ = m_dwNumVtx + x;

		m_dwNumIdx += nVerts;
	}

	m_dwNumVtx += nVerts;

#ifdef _CONTEXT_FLUSH_EVERY_PRIMITIVE
	FlushVB();
#endif

	m_colFOG = 0xFFFFFFFF;
}

void ContextMPR::DrawPrimitive(int nPrimType, WORD VtxInfo, WORD nVerts, MPRVtxTexClr_t **pData, bool terrain)
{
	TLVERTEX *pVtx;
	SVertex *sVertex;
	SPolygon *sPolygon;
	float	PolyZAvg=0;

	// Ensure no degenerate nPrimTypeitives
	ShiAssert((nVerts >=3) || (nPrimType==MPR_PRM_POINTS && nVerts >=1) || (nPrimType<=MPR_PRM_POLYLINE && nVerts >=2));

#ifdef _CONTEXT_TRACE_ALL
	MonoPrint("ContextMPR::DrawPrimitive3(%d,0x%X,%d,0x%X)\n",nPrimType,VtxInfo,nVerts,pData);
#endif

#ifdef _CONTEXT_ENABLE_STATS
	m_stats.Primitive(m_nCurPrimType,nVerts);
#endif

	if(!bZBuffering)
	{
		// Lock VB
		if(!LockVB(nVerts,(void **)&m_pTLVtx))
		{
			m_colFOG = 0xFFFFFFFF;
			return;
		}

		ShiAssert(FALSE == F4IsBadWritePtr(m_pTLVtx,sizeof *m_pTLVtx));	
		ShiAssert(m_dwStartVtx < m_dwVBSize);
		pVtx = &m_pTLVtx[m_dwStartVtx + m_dwNumVtx];

		ShiAssert(FALSE == F4IsBadWritePtr(pVtx,nVerts * sizeof *pVtx));

		// JB 011124 CTD
		if (!pVtx)
		{
			m_colFOG = 0xFFFFFFFF;
			return;
		}

		SetPrimitiveType(nPrimType);
	}
	else
	{
		AllocatePolygon(sPolygon,nVerts);
		sPolygon->renderState = currentState;
		sPolygon->textureID0 = currentTexture1;

		if(currentState >= STATE_MULTITEXTURE)
			sPolygon->textureID1 = currentTexture2;
		else
			sPolygon->textureID1 = -1;

		sPolygon->pNext = NULL;
		sVertex = sPolygon->pVertexList;
	}

	if(!bZBuffering)
	{
		// Iterate for each vertex
		for(int i = 0; i < nVerts; i++)
		{
			// Check for overrun
			ShiAssert((BYTE *)pVtx < m_pVtxEnd);
	
			// JB 010712 CTD second try
			if(!pData[i]) break;
	
			if(DisplayOptions.bScreenCoordinateBiasFix)
			{
				pVtx->sx = pData[i]->x-0.5f;
				pVtx->sy = pData[i]->y-0.5f;
			}
			else
			{
				pVtx->sx = pData[i]->x;
				pVtx->sy = pData[i]->y;
			}

			// NOTE: HACK!!
			pVtx->sz = 1.0f;
			pVtx->rhw = pData[i]->q > 0.0f ? 1.0f / (pData[i]->q / Q_SCALE) : 1.0f;

			if(terrain)
			{
				if(VtxInfo & MPR_VI_COLOR)
					pVtx->color = D3DRGBA(pData[i]->r,pData[i]->g,pData[i]->b,1.f);

				pVtx->specular = (min(255,FloatToInt32(pData[i]->a * 255.f)) << 24) + 0xFFFFFF;
			}
			else
			{
				if(VtxInfo & MPR_VI_COLOR)
					pVtx->color = D3DRGBA(pData[i]->r,pData[i]->g,pData[i]->b,pData[i]->a);

				pVtx->specular = m_colFOG;
			}
	
			if(VtxInfo & MPR_VI_TEXTURE)
			{
				if(terrain)
				{
					if(DisplayOptions.m_texMode == DisplayOptionsClass::TEX_MODE_DDS)
					{
						// Tex coords for night texture
						pVtx->tu1 = pData[i]->u;
						pVtx->tv1 = pData[i]->v;
					}
				}

				pVtx->tu0 = pData[i]->u;
				pVtx->tv0 = pData[i]->v;
			}

		#ifdef _CONTEXT_ENABLE_RENDERSTATE_HIGHLIGHT
			pVtx->color = currentState != -1 ? RGBA_MAKE((currentState << 1) + 50,(currentState << 1) + 50,(currentState << 1) + 50,(currentState << 1) + 50) : D3DRGBA(1.0f,1.0f,1.0f,1.0f);
		#endif
	
			pVtx++;
		}
	}
	else
	{
		// COBRA - RED - These are to be calculated ONCE for poly, not for Vertex
		float gzNear,gCX1,gCX2;

		if(terrain)
			gzNear = ZNEAR-.02f;
		else
			gzNear = ZNEAR;

		gCX1=(ZFAR/(ZFAR-gzNear));
		gCX2=(ZFAR*gzNear/(gzNear-ZFAR));
		// COBRA - RED -End2


		// Iterate for each vertex
		int	i=nVerts;
		//for(int i = 0; i < poly->nVerts; i++)
		while(i--)
		{
			// JB 010712 CTD
			if(!pData[i]) break;
	
			sVertex->sx = pData[i]->x;
			sVertex->sy = pData[i]->y;

			// NOTE: HACK!!
			if(pData[i]->q)
				sVertex->sz = gCX1+gCX2/(pData[i]->q/Q_SCALE);
			else
				sVertex->sz = 0.f;

			sVertex->rhw = pData[i]->q > 0.0f ? 1.0f / (pData[i]->q / Q_SCALE) : 1.0f;

			if(terrain)
			{
				if(VtxInfo & MPR_VI_COLOR)
					sVertex->color = D3DRGBA(pData[i]->r,pData[i]->g,pData[i]->b,1.f);

				sVertex->specular = (min(255,FloatToInt32(pData[i]->a * 255.f)) << 24) + 0xFFFFFF;
			}
			else
			{
				if(VtxInfo & MPR_VI_COLOR)
					sVertex->color = D3DRGBA(pData[i]->r,pData[i]->g,pData[i]->b,pData[i]->a);

				sVertex->specular = m_colFOG;
			}

			if(VtxInfo & MPR_VI_TEXTURE)
			{
				if(terrain)
				{
					if(DisplayOptions.m_texMode == DisplayOptionsClass::TEX_MODE_DDS)
					{
						sVertex->tu1 = pData[i]->u;
						sVertex->tv1 = pData[i]->v;
					}
				}

				sVertex->tu0 = pData[i]->u;				
				sVertex->tv0 = pData[i]->v;
			}

		#ifdef _CONTEXT_ENABLE_RENDERSTATE_HIGHLIGHT
			sVertex->color = currentState != -1 ? RGBA_MAKE((currentState << 1) + 50,(currentState << 1) + 50,(currentState << 1) + 50,(currentState << 1) + 50) : D3DRGBA(1.0f,1.0f,1.0f,1.0f);
		#endif
			PolyZAvg+=sVertex->sz;							// COBRA - RED - Poly Z Sum is calculated onthe fly

			// COBRA - RED - Linking of vertexes is done on the fly
			if(i) sVertex = sVertex->pNext = sVertex+1;
			else sVertex->pNext= sPolygon->pVertexList;
		}
	}

	// Generate Indices
	if(!bZBuffering)
	{
		if(m_nCurPrimType == D3DPT_TRIANGLEFAN)
		{
			WORD *pIdx = &m_pIdx[m_dwNumIdx];

			for(int x=0;x<nVerts-2;x++)
			{
				pIdx[0] = m_dwNumVtx;
				pIdx[1] = m_dwNumVtx + x + 1;
				pIdx[2] = m_dwNumVtx + x + 2;
				pIdx += 3;
			}

			m_dwNumIdx += pIdx - &m_pIdx[m_dwNumIdx];
		}
		else if(m_nCurPrimType == D3DPT_LINESTRIP)
		{
			WORD *pIdx = &m_pIdx[m_dwNumIdx];

			for(int x=0;x<nVerts;x++)
				*pIdx++ = m_dwNumVtx + x;

			m_dwNumIdx += nVerts;
		}

		m_dwNumVtx += nVerts;

	#ifdef _CONTEXT_FLUSH_EVERY_PRIMITIVE
		FlushVB();
	#endif
	}
	else
	{
		sPolygon->CalcPolyZ(PolyZAvg);

		// Double-textured
		if(sPolygon->renderState >= STATE_MULTITEXTURE)
		{
			texturedPolyVCnt += nVerts;
			AddPolygon(texturedPolys,sPolygon);
		}
		// Translucent
		else if(sPolygon->renderState >= STATE_ALPHA_SOLID)
		{
			translucentPolyVCnt += nVerts;
			AddPolygon(translucentPolys,sPolygon);
		}
		// Textured
		else if(sPolygon->renderState >= STATE_TEXTURE)
		{
			texturedPolyVCnt += nVerts;
			AddPolygon(texturedPolys,sPolygon);
		}
		// Plain
		else if(sPolygon->renderState >= STATE_SOLID)
		{
			plainPolyVCnt += nVerts;
			AddPolygon(plainPolys,sPolygon);
		}
		else
			INT3;
	}

	m_colFOG = 0xFFFFFFFF;
}

ContextMPR::Stats::Stats()
{
	#ifdef _CONTEXT_ENABLE_STATS
	Init();
	#endif
}

#ifdef _CONTEXT_ENABLE_STATS
void ContextMPR::Stats::Check()
{
	DWORD Ticks = ::GetTickCount();

	if(Ticks - dwTicks > 1000)
	{
		dwTicks = Ticks;
		dwLastFPS = dwCurrentFPS;
		dwCurrentFPS = 0;

		if(dwCurPrimCountPerSecond > dwMaxPrimCountPerSecond)
			dwMaxPrimCountPerSecond = dwCurPrimCountPerSecond;

		if(dwCurVtxCountPerSecond > dwMaxVtxCountPerSecond)
			dwMaxVtxCountPerSecond = dwCurVtxCountPerSecond;

		if(dwTotalSeconds)
		{
			dwAvgVtxCountPerSecond = dwTotalVtxCount / dwTotalSeconds;
			dwAvgPrimCountPerSecond = dwTotalPrimCount / dwTotalSeconds;
		}

		if(dwTotalBatches)
		{
			dwAvgVtxBatchSize = dwTotalVtxBatchSize / dwTotalBatches;
			dwAvgPrimBatchSize = dwTotalPrimBatchSize / dwTotalBatches;
		}

		if(dwLastFPS < dwMinFPS)
			dwMinFPS = dwLastFPS;
		else if(dwLastFPS > dwMaxFPS)
			dwMaxFPS = dwLastFPS;

		dwTotalFPS += dwLastFPS;
		dwTotalSeconds++;
		dwAverageFPS = dwTotalFPS / dwTotalSeconds;
	}
}

void ContextMPR::Stats::Init()
{
	ZeroMemory(this,sizeof(*this));
}

void ContextMPR::Stats::StartFrame()
{
	dwCurrentFPS++;
	Check();
}

void ContextMPR::Stats::StartBatch()
{
	dwTotalBatches++;

	if(dwCurVtxBatchSize > dwMaxVtxBatchSize)
		dwMaxVtxBatchSize = dwCurVtxBatchSize;
	dwTotalVtxBatchSize += dwCurVtxBatchSize;
	dwCurVtxBatchSize = 0;

	if(dwCurPrimBatchSize > dwMaxPrimBatchSize)
		dwMaxPrimBatchSize = dwCurPrimBatchSize;
	dwTotalPrimBatchSize += dwCurPrimBatchSize;
	dwCurPrimBatchSize = 0;
}

void ContextMPR::Stats::Primitive(DWORD dwType, DWORD dwNumVtx)
{
	arrPrimitives[dwType - 1]++;
	dwTotalPrimitives++;
	dwCurPrimCountPerSecond++;
	dwCurVtxCountPerSecond += dwNumVtx;
	dwCurVtxBatchSize += dwNumVtx;
	dwCurPrimBatchSize++;
	dwTotalPrimCount++;
	dwTotalVtxCount += dwNumVtx;
}

void ContextMPR::Stats::PutTexture(bool bCached)
{
	dwPutTextureTotal++;
	if(bCached) dwPutTextureCached++;
}

void ContextMPR::Stats::Report()
{
	MonoPrint("Stats report follows\n");

	float fT = dwTotalPrimitives / 100.0f;

	MonoPrint("	MinFPS: %d\n",dwMinFPS);
	MonoPrint("	MaxFPS: %d\n",dwMaxFPS);
	MonoPrint("	AverageFPS: %d\n",dwAverageFPS);
	MonoPrint("	TotalPrimitives: %d\n",dwTotalPrimitives);
	MonoPrint("	Triangle Lists: %d (%.2f %%)\n",arrPrimitives[3],arrPrimitives[3] / fT);
	MonoPrint("	Triangle Strips: %d (%.2f %%)\n",arrPrimitives[4],arrPrimitives[4] / fT);
	MonoPrint("	Triangle Fans: %d (%.2f %%)\n",arrPrimitives[5],arrPrimitives[5] / fT);
	MonoPrint("	Point Lists: %d (%.2f %%)\n",arrPrimitives[0],arrPrimitives[0] / fT);
	MonoPrint("	Line Lists: %d (%.2f %%)\n",arrPrimitives[1],arrPrimitives[1] / fT);
	MonoPrint("	Line Strips: %d (%.2f %%)\n",arrPrimitives[2],arrPrimitives[2] / fT);
	MonoPrint("	AvgVtxBatchSize: %d\n",dwAvgVtxBatchSize);
	MonoPrint("	MaxVtxBatchSize: %d\n",dwMaxVtxBatchSize);
	MonoPrint("	AvgPrimBatchSize: %d\n",dwAvgPrimBatchSize);
	MonoPrint("	MaxPrimBatchSize: %d\n",dwMaxPrimBatchSize);
	MonoPrint("	AvgVtxCountPerSecond: %d\n",dwAvgVtxCountPerSecond);
	MonoPrint("	MaxVtxCountPerSecond: %d\n",dwMaxVtxCountPerSecond);
	MonoPrint("	AvgPrimCountPerSecond: %d\n",dwAvgPrimCountPerSecond);
	MonoPrint("	MaxPrimCountPerSecond: %d\n",dwMaxPrimCountPerSecond);
	MonoPrint("	TextureChangesTotal: %d\n",dwPutTextureTotal);
	MonoPrint("	TextureChangesCached: %d (%.2f %%)\n",dwPutTextureCached,(float) dwPutTextureCached / (dwPutTextureTotal / 100.0f));

	MonoPrint("End of stats report\n");
}

#endif
