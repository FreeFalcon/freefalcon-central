#include "stdafx.h"
#include "cpdigits.h"

#include "Graphics\Include\grinline.h"	//Wombat778 3-22-04

//MI
extern bool g_bRealisticAvionics;
extern bool g_bFilter2DPit;		//Wombat778 3-30-04


CPDigits::CPDigits(ObjectInitStr *pobjectInitStr, DigitsInitStr* pdigitsInitStr) : CPObject(pobjectInitStr)
{
	int i;

	mDestDigits		= pdigitsInitStr->numDestDigits;
	mpSrcRects		= pdigitsInitStr->psrcRects;
	mpDestRects		= pdigitsInitStr->pdestRects;

	for(i = 0; i < mDestDigits; i++) {
		mpDestRects[i].top		= (LONG)(mVScale * mpDestRects[i].top);
		mpDestRects[i].left		= (LONG)(mHScale * mpDestRects[i].left);
		mpDestRects[i].bottom	= (LONG)(mVScale * mpDestRects[i].bottom);
		mpDestRects[i].right	= (LONG)(mHScale * mpDestRects[i].right);	
	}

	mValue			= 0;

	#ifdef USE_SH_POOLS
	mpValues = (int *)MemAllocPtr(gCockMemPool,sizeof(int)*mDestDigits,FALSE);
	mpDestString = (char *)MemAllocPtr(gCockMemPool,sizeof(char)*(mDestDigits+1),FALSE);
	#else
	mpValues			= new int[mDestDigits];
	mpDestString	= new char[mDestDigits + 1];
	#endif

	memset(mpValues, 0, mDestDigits * sizeof(int));

	//MI
	active = TRUE;

	//Wombat778 3-22-04  Added following for rendered (rather than blitted digits)
	if (DisplayOptions.bRender2DCockpit)
	{
		mpSourceBuffer = pdigitsInitStr->sourcedigits;		

		for( i = 0; i < 10; i++)
		{	
			mpSourceBuffer[i].mWidth		= pdigitsInitStr->psrcRects[i].right - pdigitsInitStr->psrcRects[i].left;
			mpSourceBuffer[i].mHeight		= pdigitsInitStr->psrcRects[i].bottom - pdigitsInitStr->psrcRects[i].top;
		}
	}
	//Wombat778 end

}

CPDigits::~CPDigits()
{
	delete [] mpSrcRects;
	delete [] mpDestRects;
	delete [] mpValues;
	delete [] mpDestString;

	//Wombat778 3-22-04 clean up buffers
	if (DisplayOptions.bRender2DCockpit)
	{		
		for(int i = 0; i < 10; i++)		//Assume 10 digits, which is how big the number of source rectangles is
			glReleaseMemory((char*) mpSourceBuffer[i].digit);		
		delete [] mpSourceBuffer;
	}

}

void CPDigits::Exec(SimBaseClass* pOwnship) {

	mpOwnship = pOwnship;

	//get the value from ac model
	if(mExecCallback) {
		mExecCallback(this);
	}

	SetDirtyFlag(); //VWF FOR NOW
}

void CPDigits::DisplayBlit()
{
	int	i;
	mDirtyFlag = TRUE;

	if(!mDirtyFlag)
		return;

	if (DisplayOptions.bRender2DCockpit)			//Wombat778 3-22-04 Handle drawing in DisplayBlit3D
		return;
	
	//MI
	if(!g_bRealisticAvionics)
	{
		for(i = 0; i < mDestDigits; i++)
			mpOTWImage->Compose(mpTemplate, &mpSrcRects[mpValues[i]], &mpDestRects[i]);
	}
	else
	{
		for(i = 0; i < mDestDigits; i++)
		{
			if(active)
				mpOTWImage->Compose(mpTemplate, &mpSrcRects[mpValues[i]], &mpDestRects[i]);
		}
	}

	mDirtyFlag = FALSE;
}



void CPDigits::DisplayBlit3D()	//Wombat778 3-22-04 Add support for rendered digits.  Much faster than blitting.
{
	int	i;
	mDirtyFlag = TRUE;

	if(!mDirtyFlag)
		return;
	
	if (!DisplayOptions.bRender2DCockpit)		//Handle drawing in DisplayBlit
		return;
	
	//Wombat778 new rendering code. Taken from cpsurface.cpp
	if ( !g_bRealisticAvionics || (g_bRealisticAvionics && active))
	{
		OTWDriver.renderer->CenterOriginInViewport();
		OTWDriver.renderer->SetViewport(-1.0F, 1.0F, 1.0F, -1.0F);
		for(i = 0; i < mDestDigits; i++)
		{
			if(mpSourceBuffer[mpValues[i]].m_arrTex.size() == 1)
			{
				TextureHandle *pTex = mpSourceBuffer[mpValues[i]].m_arrTex[0];
				// Setup vertices
				float fStartU = 0;
				float fMaxU = (float) pTex->m_nWidth / (float) pTex->m_nActualWidth;
				fMaxU -= fStartU;
				float fStartV = 0;
				float fMaxV = (float) pTex->m_nHeight / (float) pTex->m_nActualHeight;
				fMaxV -= fStartV;
			
				TwoDVertex pVtx[4];
				ZeroMemory(pVtx, sizeof(pVtx));
					
				pVtx[0].x = (float)mpDestRects[i].left; pVtx[0].y = (float)mpDestRects[i].top; pVtx[0].u = fStartU; pVtx[0].v = fStartV;
				pVtx[0].r = pVtx[0].g = pVtx[0].b = pVtx[0].a = 1.0f;
				pVtx[1].x = (float)mpDestRects[i].right; pVtx[1].y = (float)mpDestRects[i].top; pVtx[1].u = fMaxU; pVtx[1].v = fStartV;
				pVtx[1].r = pVtx[1].g = pVtx[1].b = pVtx[1].a = 1.0f;
				pVtx[2].x = (float)mpDestRects[i].right; pVtx[2].y = (float)mpDestRects[i].bottom; pVtx[2].u = fMaxU; pVtx[2].v = fMaxV;
				pVtx[2].r = pVtx[2].g = pVtx[2].b = pVtx[2].a = 1.0f;
				pVtx[3].x = (float)mpDestRects[i].left; pVtx[3].y = (float)mpDestRects[i].bottom; pVtx[3].u = fStartU; pVtx[3].v = fMaxV;
				pVtx[3].r = pVtx[3].g = pVtx[3].b = pVtx[3].a = 1.0f;

				// COBRA - RED - Pit Vibrations
				OTWDriver.pCockpitManager->AddTurbulence(pVtx);

				if (g_bFilter2DPit)			//Wombat778 3-30-04 Add option to filter
					OTWDriver.renderer->context.RestoreState(STATE_TEXTURE);				
				else
					OTWDriver.renderer->context.RestoreState(STATE_TEXTURE_NOFILTER);				
				OTWDriver.renderer->context.SelectTexture1((GLint) pTex);
				OTWDriver.renderer->context.DrawPrimitive(MPR_PRM_TRIFAN,MPR_VI_COLOR|MPR_VI_TEXTURE,4,pVtx,sizeof(pVtx[0]));
			}
		}
	}
	mDirtyFlag = FALSE;
}


void CPDigits::SetDigitValues(long value)
{
	int	i, j;
	int	fieldlen;

	if(mValue != value) {
		mValue = value;
		char tbuf[20]; // temporary copy - as it might be bigger
		fieldlen = sprintf(tbuf, "%ld", mValue);
		if (fieldlen > mDestDigits) { // fix up oversized values
		    strncpy (mpDestString, &tbuf[fieldlen-mDestDigits], mDestDigits);
		    fieldlen = mDestDigits;
		}
		else 
		    strncpy (mpDestString, tbuf, mDestDigits);

		memset(mpValues, 0, mDestDigits * sizeof(int));

		if(value > 0)
		{
		    for(i = mDestDigits - fieldlen, j = 0; i < mDestDigits; i++, j++) {
			ShiAssert(i >=0 && i < mDestDigits);
			mpValues[i] = mpDestString[j] - 0x30;
		    }
		}
	}
}


//Wombat778 3-22-04 Additional functions for rendering the image

void CPDigits::CreateLit(void)
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

			for(int i = 0; i < 10; i++)
			{										
				// Check if we can use a single texture
				if((int)dwMaxTextureWidth >= mpSourceBuffer[i].mWidth && (int)dwMaxTextureHeight >= mpSourceBuffer[i].mHeight) 
				{
					TextureHandle *pTex = new TextureHandle;
					if(!pTex)
						throw _com_error(E_OUTOFMEMORY);
					m_pPalette->AttachToTexture(pTex);
					if(!pTex->Create("CPDigit", MPR_TI_PALETTE | MPR_TI_CHROMAKEY, 8, mpSourceBuffer[i].mWidth, mpSourceBuffer[i].mHeight))
						throw _com_error(E_FAIL);
					if(!pTex->Load(0, 0xFFFF0000, (BYTE*) mpSourceBuffer[i].digit, true, true))	// soon to be re-loaded by CPSurface::Translate3D
						throw _com_error(E_FAIL);
					mpSourceBuffer[i].m_arrTex.push_back(pTex);
				}
			}

		}
		catch(_com_error e)
		{
			MonoPrint("CPDigits::CreateLit - Error 0x%X (%s)\n", e.Error(), e.ErrorMessage());
			DiscardLit();
		}
	}
}

void CPDigits::DiscardLit(void)
{	
	if (DisplayOptions.bRender2DCockpit)
	{
		for(int i2 = 0; i2 < 10; i2++)
		{										
			for(int i=0;i<(int)mpSourceBuffer[i2].m_arrTex.size();i++)	//delete the textures for each digit
				delete mpSourceBuffer[i2].m_arrTex[i];
			mpSourceBuffer[i2].m_arrTex.clear();	
		}
	}

	for(int i=0;i<(int)m_arrTex.size();i++) delete m_arrTex[i];	//delete the local textures
	m_arrTex.clear();									
	

	if(m_pPalette)
	{
	  delete m_pPalette; // JPO - memory leak fix
	  m_pPalette = NULL;
	}
}
//Wombat778 end