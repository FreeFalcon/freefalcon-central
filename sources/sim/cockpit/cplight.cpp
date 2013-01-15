#include "stdafx.h"
#include "cpmanager.h"
#include "cplight.h"
//MI
#include "aircrft.h"
#include "simdrive.h"

#include "Graphics/Include/grinline.h"	//Wombat778 3-22-04
extern bool g_bFilter2DPit;		//Wombat778 3-30-04


//====================================================//
// CPLight::CPLight
//====================================================//

CPLight::CPLight(ObjectInitStr *pobjectInitStr, LightButtonInitStr *plightInitStr) : CPObject(pobjectInitStr) {
	
	mStates		= plightInitStr->states;
	mpSrcRect	= plightInitStr->psrcRect;
	mState		= plightInitStr->initialState;
	//MI
	WasPersistant = FALSE;

	//Wombat778 3-22-04  Added following for rendered (rather than blitted lights)
	if (DisplayOptions.bRender2DCockpit)
	{
		mpSourceBuffer = plightInitStr->sourcelights;		

		for( int i = 0; i < mStates; i++)
		{	
			mpSourceBuffer[i].mWidth		= mpSrcRect[i].right - mpSrcRect[i].left;
			mpSourceBuffer[i].mHeight		= mpSrcRect[i].bottom - mpSrcRect[i].top;
		}
	}
	//Wombat778 end

}

//====================================================//
// CPLight::~CPLight
//====================================================//

CPLight::~CPLight() {

	delete [] mpSrcRect;

	//Wombat778 3-22-04 clean up buffers
	if (DisplayOptions.bRender2DCockpit)
	{		
		for(int i = 0; i < mStates; i++)		
			glReleaseMemory((char*) mpSourceBuffer[i].light);		
		delete [] mpSourceBuffer;
	}
}

//====================================================//
// CPLight::Exec
//====================================================//

void CPLight::Exec(SimBaseClass* pOwnship) { 

	mpOwnship = pOwnship;
	if(mExecCallback) {
		mExecCallback(this);
	}
	SetDirtyFlag(); //VWF FOR NOW
}

//====================================================//	
// CPLight::Display
//====================================================//

void CPLight::DisplayBlit(void) {

	mDirtyFlag = TRUE;

	if(!mDirtyFlag || !SimDriver.GetPlayerEntity()) {
		return;
	}

	if(mState >= mStates) {
		mState = 0;
	}

	if (DisplayOptions.bRender2DCockpit)			//Handle these in displayblit3d
		return;	

//F4Assert(mState < mStates);
	// Non-Persistant and If the state is not off, 
	// i.e. not CPLIGHT_OFF, CPLIGHT_AOA_OFF or 
	// CPLIGHT_AR_NWS_OFF

	//MI check for electrics
	if((((AircraftClass*)(SimDriver.GetPlayerEntity()))->MainPower() == AircraftClass::MainPowerOff)
		&& mPersistant == 3)
	{
		//restore our original state
		if(mState)
		{ 
			if(mTransparencyType == CPTRANSPARENT) 
			{
				mpOTWImage->ComposeTransparent(mpTemplate, &mpSrcRect[mState], &mDestRect);
			}
			else
			{
				mpOTWImage->Compose(mpTemplate, &mpSrcRect[mState], &mDestRect);
			}
		} 
		return;
	}
	else if((((AircraftClass*)(SimDriver.GetPlayerEntity()))->MainPower() == AircraftClass::MainPowerOff)
		&& mPersistant != 3)
	{
		//restore our original state
		if(WasPersistant)
		{
			mPersistant = 3;
			WasPersistant = FALSE;
		}
		return;
	}
	else if((((AircraftClass*)(SimDriver.GetPlayerEntity()))->MainPower() == AircraftClass::MainPowerBatt))
	{
		if(mState)
		{ 
			if(mTransparencyType == CPTRANSPARENT) 
			{
				mpOTWImage->ComposeTransparent(mpTemplate, &mpSrcRect[mState], &mDestRect);
			}
			else
			{
				mpOTWImage->Compose(mpTemplate, &mpSrcRect[mState], &mDestRect);
			}
		} 
		return;
	}
	else if((((AircraftClass*)(SimDriver.GetPlayerEntity()))->MainPower() == AircraftClass::MainPowerMain)
		&& mPersistant == 3)
	{
		//make them go away
		mPersistant = 0;
		//remember which one it was
		WasPersistant = TRUE;
	}

	if(((AircraftClass*)(SimDriver.GetPlayerEntity()))->TestLights && mPersistant !=3 && !WasPersistant)
	{
		mState = TRUE;
	}
	
	if(mState || mPersistant)
	{
		if(mTransparencyType == CPTRANSPARENT) 
		{
			mpOTWImage->ComposeTransparent(mpTemplate, &mpSrcRect[mState], &mDestRect);
		}
		else
		{
			mpOTWImage->Compose(mpTemplate, &mpSrcRect[mState], &mDestRect);
		}
	}

	mDirtyFlag = FALSE;
}

void RenderLightPoly(SourceLightType *sb, tagRECT *destrect, GLint alpha)		//Wombat778 3-22-04 helper function to keep the displayblit3d tidy. 
{

	OTWDriver.renderer->CenterOriginInViewport();
	OTWDriver.renderer->SetViewport(-1.0F, 1.0F, 1.0F, -1.0F);	
	TextureHandle *pTex = sb->m_arrTex[0];
	// Setup vertices
	float fStartU = 0;
	float fMaxU = (float) pTex->m_nWidth / (float) pTex->m_nActualWidth;
	fMaxU -= fStartU;
	float fStartV = 0;
	float fMaxV = (float) pTex->m_nHeight / (float) pTex->m_nActualHeight;
	fMaxV -= fStartV;
		
	TwoDVertex pVtx[4];	
	ZeroMemory(pVtx, sizeof(pVtx));

	

	pVtx[0].x += (Float_t)destrect->left; pVtx[0].y += (Float_t)destrect->top; pVtx[0].u = (Float_t)fStartU; pVtx[0].v = (Float_t)fStartV;
	pVtx[0].r = pVtx[0].g = pVtx[0].b = pVtx[0].a = 1.0f;
	pVtx[1].x += (Float_t)destrect->right; pVtx[1].y += (Float_t)destrect->top; pVtx[1].u = (Float_t)fMaxU; pVtx[1].v = (Float_t)fStartV;
	pVtx[1].r = pVtx[1].g = pVtx[1].b = pVtx[1].a = 1.0f;
	pVtx[2].x += (Float_t)destrect->right; pVtx[2].y += (Float_t)destrect->bottom; pVtx[2].u = (Float_t)fMaxU; pVtx[2].v = (Float_t)fMaxV;
	pVtx[2].r = pVtx[2].g = pVtx[2].b = pVtx[2].a = 1.0f;
	pVtx[3].x += (Float_t)destrect->left; pVtx[3].y += (Float_t)destrect->bottom; pVtx[3].u = (Float_t)fStartU; pVtx[3].v = (Float_t)fMaxV;
	pVtx[3].r = pVtx[3].g = pVtx[3].b = pVtx[3].a = 1.0f;	

	// COBRA - RED - Pit Vibrations
	OTWDriver.pCockpitManager->AddTurbulence(pVtx);

	OTWDriver.renderer->context.RestoreState(alpha);
	OTWDriver.renderer->context.SelectTexture1((GLint) pTex);
	OTWDriver.renderer->context.DrawPrimitive(MPR_PRM_TRIFAN,MPR_VI_COLOR|MPR_VI_TEXTURE,4,pVtx,sizeof(pVtx[0]));

}


void CPLight::DisplayBlit3D()	//Wombat778 3-22-04 Add support for rendered lights.  Much faster than blitting.
{
	mDirtyFlag = TRUE;

	if(!mDirtyFlag || !SimDriver.GetPlayerEntity()) {
		return;
	}

	if(mState >= mStates) {
		mState = 0;
	}

	if (!DisplayOptions.bRender2DCockpit)			//Handle these in displayblit
		return;	


	if(mpSourceBuffer[mState].m_arrTex.size() != 1)
		return;

	if((((AircraftClass*)(SimDriver.GetPlayerEntity()))->MainPower() == AircraftClass::MainPowerOff)
		&& mPersistant == 3)
	{
		//restore our original state
		if(mState)
		{ 
			if(mTransparencyType == CPTRANSPARENT) 
			{				
				if (g_bFilter2DPit)		//Wombat778 3-30-04 Added option to filter
					RenderLightPoly(&mpSourceBuffer[mState],&mDestRect,STATE_CHROMA_TEXTURE);				
				else
					RenderLightPoly(&mpSourceBuffer[mState],&mDestRect,STATE_ALPHA_TEXTURE_NOFILTER);				
			}
			else
			{			
				if (g_bFilter2DPit)		//Wombat778 3-30-04 Added option to filter
					RenderLightPoly(&mpSourceBuffer[mState],&mDestRect,STATE_TEXTURE);								
				else
					RenderLightPoly(&mpSourceBuffer[mState],&mDestRect,STATE_TEXTURE_NOFILTER);								
			}
		} 
		return;
	}
	else if((((AircraftClass*)(SimDriver.GetPlayerEntity()))->MainPower() == AircraftClass::MainPowerOff)
		&& mPersistant != 3)
	{
		//restore our original state
		if(WasPersistant)
		{
			mPersistant = 3;
			WasPersistant = FALSE;
		}
		return;
	}
	else if((((AircraftClass*)(SimDriver.GetPlayerEntity()))->MainPower() == AircraftClass::MainPowerBatt))
	{
		if(mState)
		{ 
			if(mTransparencyType == CPTRANSPARENT) 
			{			
				if (g_bFilter2DPit)		//Wombat778 3-30-04 Added option to filter
					RenderLightPoly(&mpSourceBuffer[mState],&mDestRect,STATE_CHROMA_TEXTURE);				
				else
					RenderLightPoly(&mpSourceBuffer[mState],&mDestRect,STATE_ALPHA_TEXTURE_NOFILTER);				
			}
			else
			{
				if (g_bFilter2DPit)		//Wombat778 3-30-04 Added option to filter
					RenderLightPoly(&mpSourceBuffer[mState],&mDestRect,STATE_TEXTURE);								
				else
					RenderLightPoly(&mpSourceBuffer[mState],&mDestRect,STATE_TEXTURE_NOFILTER);								
			}
		} 
		return;
	}
	else if((((AircraftClass*)(SimDriver.GetPlayerEntity()))->MainPower() == AircraftClass::MainPowerMain)
		&& mPersistant == 3)
	{
		//make them go away
		mPersistant = 0;
		//remember which one it was
		WasPersistant = TRUE;
	}

	if(((AircraftClass*)(SimDriver.GetPlayerEntity()))->TestLights && mPersistant !=3 && !WasPersistant)
	{
		mState = TRUE;
	}
	
	if(mState || mPersistant)
	{
		if(mTransparencyType == CPTRANSPARENT) 
		{			
			if (g_bFilter2DPit)		//Wombat778 3-30-04 Added option to filter
				RenderLightPoly(&mpSourceBuffer[mState],&mDestRect,STATE_CHROMA_TEXTURE);				
			else
				RenderLightPoly(&mpSourceBuffer[mState],&mDestRect,STATE_ALPHA_TEXTURE_NOFILTER);				
		}
		else
		{
			if (g_bFilter2DPit)		//Wombat778 3-30-04 Added option to filter
				RenderLightPoly(&mpSourceBuffer[mState],&mDestRect,STATE_TEXTURE);								
			else
				RenderLightPoly(&mpSourceBuffer[mState],&mDestRect,STATE_TEXTURE_NOFILTER);								
		}
	}

	mDirtyFlag = FALSE;



}



//Wombat778 3-22-04 Additional functions for rendering the image

void CPLight::CreateLit(void)
{
	if (DisplayOptions.bRender2DCockpit)
	{

		try
		{
			const DWORD dwMaxTextureWidth = mpOTWImage->GetDisplayDevice()->GetDefaultRC()->m_pD3DHWDeviceDesc->dwMaxTextureWidth;
			const DWORD dwMaxTextureHeight = mpOTWImage->GetDisplayDevice()->GetDefaultRC()->m_pD3DHWDeviceDesc->dwMaxTextureHeight;
			m_pPalette = new PaletteHandle(mpOTWImage->GetDisplayDevice()->GetDefaultRC()->m_pDD, 32, 256);

			if(!m_pPalette)
				throw _com_error(E_OUTOFMEMORY);

			for(int i = 0; i < mStates; i++)
			{										
				// Check if we can use a single texture
				if(((int)(dwMaxTextureWidth) >= mpSourceBuffer[i].mWidth) && 
					((int)(dwMaxTextureHeight) >= mpSourceBuffer[i].mHeight)) 
				{
					TextureHandle *pTex = new TextureHandle;
					if(!pTex)
						throw _com_error(E_OUTOFMEMORY);
					m_pPalette->AttachToTexture(pTex);
					if(!pTex->Create("CPLight", MPR_TI_PALETTE | MPR_TI_CHROMAKEY, 8, mpSourceBuffer[i].mWidth, mpSourceBuffer[i].mHeight))
						throw _com_error(E_FAIL);
					if(!pTex->Load(0, 0xFFFF0000, (BYTE*) mpSourceBuffer[i].light, true, true))	// soon to be re-loaded by CPSurface::Translate3D
						throw _com_error(E_FAIL);
					mpSourceBuffer[i].m_arrTex.push_back(pTex);
				}
			}

		}
		catch(_com_error e)
		{
			MonoPrint("CPLight::CreateLit - Error 0x%X (%s)\n", e.Error(), e.ErrorMessage());
			DiscardLit();
		}
	}
}

void CPLight::DiscardLit(void)
{	
	if (DisplayOptions.bRender2DCockpit)
	{
		for(int i2 = 0; i2 < mStates; i2++)
		{										
			for(unsigned int i=0;i<mpSourceBuffer[i2].m_arrTex.size();i++)	//delete the textures for each light
				delete mpSourceBuffer[i2].m_arrTex[i];
			mpSourceBuffer[i2].m_arrTex.clear();	
		}
	}

	for(unsigned int i=0;i<m_arrTex.size();i++) delete m_arrTex[i];	//delete the local textures
	m_arrTex.clear();									
	

	if(m_pPalette)
	{
	  delete m_pPalette; // JPO - memory leak fix
	  m_pPalette = NULL;
	}
}
//Wombat778 end