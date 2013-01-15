#include "stdafx.h"
#include "cpindicator.h"

#include "Graphics\Include\grinline.h"	//Wombat778 3-22-04
extern bool g_bFilter2DPit;		//Wombat778 3-30-04

CPIndicator::CPIndicator(ObjectInitStr *pobjectInitStr, IndicatorInitStr* pindicatorInitStr) : CPObject(pobjectInitStr) {
	int	i;

	mNumTapes			= pindicatorInitStr->numTapes;
	mMinVal				= pindicatorInitStr->minVal;
	mMaxVal				= pindicatorInitStr->maxVal;
	mOrientation		= pindicatorInitStr->orientation;
	mCalibrationVal	= pindicatorInitStr->calibrationVal;
							
	#ifdef USE_SH_POOLS
	mpSrcLocs = (RECT *)MemAllocPtr(gCockMemPool,sizeof(RECT)*mNumTapes,FALSE);
	mpDestLocs = (RECT *)MemAllocPtr(gCockMemPool,sizeof(RECT)*mNumTapes,FALSE);
	mpDestRects = (RECT *)MemAllocPtr(gCockMemPool,sizeof(RECT)*mNumTapes,FALSE);
	mpSrcRects = (RECT *)MemAllocPtr(gCockMemPool,sizeof(RECT)*mNumTapes,FALSE);
	mpTapeValues = (float *)MemAllocPtr(gCockMemPool,sizeof(float)*mNumTapes,FALSE);
	mPixelSlope = (float *)MemAllocPtr(gCockMemPool,sizeof(float)*mNumTapes,FALSE);
	mPixelIntercept = (float *)MemAllocPtr(gCockMemPool,sizeof(float)*mNumTapes,FALSE);
	mHeightTapeRect = (int *)MemAllocPtr(gCockMemPool,sizeof(int)*mNumTapes,FALSE);
	mWidthTapeRect = (int *)MemAllocPtr(gCockMemPool,sizeof(int)*mNumTapes,FALSE);
	#else
	mpSrcLocs			= new RECT[mNumTapes];
	mpDestLocs			= new RECT[mNumTapes];
	mpDestRects			= new RECT[mNumTapes];
	mpSrcRects			= new RECT[mNumTapes];
	mpTapeValues		= new float[mNumTapes];
	mPixelSlope       = new float[mNumTapes];
	mPixelIntercept   = new float[mNumTapes];
	mHeightTapeRect   = new int[mNumTapes];
	mWidthTapeRect    = new int[mNumTapes];
	#endif

	for(i = 0; i < mNumTapes; i++) {
		mpSrcLocs[i]	= pindicatorInitStr->psrcRect[i];
		mpSrcRects[i]	= pindicatorInitStr->psrcRect[i];
		mpDestRects[i] = pindicatorInitStr->pdestRect[i];

		// Determine how many pixels represent a unit, (units/pixel)
		mPixelSlope[i]			= (float)(pindicatorInitStr->maxPos[i] - pindicatorInitStr->minPos[i])/ (float)(mMaxVal - mMinVal);
		mPixelIntercept[i]	= pindicatorInitStr->maxPos[i] - (mPixelSlope[i] * mMaxVal);

		if(mOrientation == IND_VERTICAL) {
			mHeightTapeRect[i]	= mpDestRects[i].bottom - mpDestRects[i].top + 1;
		}
		else {
			mWidthTapeRect[i]		= mpDestRects[i].right - mpDestRects[i].left + 1;
		}
	}

	//Wombat778 3-22-04  Added following for rendered (rather than blitted indicators)
	if (DisplayOptions.bRender2DCockpit)
	{
		mpSourceBuffer = pindicatorInitStr->sourceindicator;		

		for( i = 0; i < mNumTapes; i++)
		{	
			mpSourceBuffer[i].mWidth		= mpSrcRects[i].right - mpSrcRects[i].left;
			mpSourceBuffer[i].mHeight		= mpSrcRects[i].bottom - mpSrcRects[i].top;
		}
	}
	//Wombat778 end
}

CPIndicator::~CPIndicator() {

	delete [] mpTapeValues;
	delete [] mpSrcLocs;
	delete [] mpSrcRects;
	delete [] mpDestLocs;
	delete [] mpDestRects;
	delete [] mPixelSlope;
	delete [] mPixelIntercept;
	delete [] mHeightTapeRect;
	delete [] mWidthTapeRect;

	//Wombat778 3-22-04 clean up buffers
	if (DisplayOptions.bRender2DCockpit)
	{		
		for(int i = 0; i < mNumTapes; i++)		
			glReleaseMemory((char*) mpSourceBuffer[i].indicator);		
		delete [] mpSourceBuffer;
	}
}

void CPIndicator::Exec(SimBaseClass* pOwnship) {

	float		tapePosition;
	int		i;

	mpOwnship = pOwnship;


	//get the value from ac model
	if(mExecCallback) {
		mExecCallback(this);
	}

	for(i = 0; i < mNumTapes; i++) {
		// Limit the current value
		if(mpTapeValues[i] > mMaxVal){
			mpTapeValues[i] = mMaxVal;
		}
		else if(mpTapeValues[i] < mMinVal){
			mpTapeValues[i] = mMinVal;
		}

		// Find the tape position
		tapePosition			= (mPixelSlope[i] * 	mpTapeValues[i]) + mPixelIntercept[i] + mCalibrationVal;

		if(mOrientation == IND_HORIZONTAL) {

			mpSrcLocs[i].left		= (int) tapePosition - (mWidthTapeRect[i]/2);
			mpSrcLocs[i].right	= (int) tapePosition + (mWidthTapeRect[i]/2);

			mpDestLocs[i]			= mpDestRects[i];

			if(mpSrcLocs[i].left < mpSrcRects[i].left) {

				mpSrcLocs[i].left = mpSrcRects[i].left;
				mpDestLocs[i].left = mpDestRects[i].left + (mpSrcRects[i].left - mpSrcLocs[i].left);
			}
			else if(mpSrcLocs[i].right > mpSrcRects[i].right) {

				mpSrcLocs[i].right = mpSrcRects[i].right;
				mpDestLocs[i].right = mpDestRects[i].right - (mpSrcLocs[i].right - mpSrcRects[i].right);
			}		
		}
		else {

			mpSrcLocs[i].top			= (int) tapePosition - (mHeightTapeRect[i]/2);
			mpSrcLocs[i].bottom		= (int) tapePosition + (mHeightTapeRect[i]/2);
			mpDestLocs[i]				= mpDestRects[i];

			if(mpSrcLocs[i].top	< mpSrcRects[i].top) {

				mpSrcLocs[i].top		= mpSrcRects[i].top;
				mpDestLocs[i].top		= mpDestRects[i].top + (mpSrcRects[i].top - mpSrcLocs[i].top);
			}
			else if(mpSrcLocs[i].bottom > mpSrcRects[i].bottom) {

				mpSrcLocs[i].bottom	= mpSrcRects[i].bottom;
				mpDestLocs[i].bottom = mpDestRects[i].bottom - (mpSrcLocs[i].bottom - mpSrcRects[i].bottom);
			}		
		}
	}

	SetDirtyFlag(); //VWF FOR NOW
}


void RenderIndicatorPoly(SourceIndicatorType *sb, tagRECT *srcrect,tagRECT *srcloc, tagRECT *destrect, GLint alpha)		//Wombat778 3-22-04 helper function to keep the displayblit3d tidy. 
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

	//select just a piece of the tapes by scaling the UV coordinates.

	fStartU = ((float)(srcloc->left-srcrect->left)/(float)(srcrect->right-srcrect->left))*fMaxU;
	fMaxU = ((float)(srcloc->right-srcrect->left)/(float)(srcrect->right-srcrect->left))*fMaxU;	 

	fStartV = ((float)(srcloc->top-srcrect->top)/(float)(srcrect->bottom-srcrect->top))*fMaxV;
	fMaxV = ((float)(srcloc->bottom-srcrect->top)/(float)(srcrect->bottom-srcrect->top))*fMaxV;	 

		
	TwoDVertex pVtx[4];	
	ZeroMemory(pVtx, sizeof(pVtx));
				
	pVtx[0].x = (float)destrect->left; pVtx[0].y = (float)destrect->top; pVtx[0].u = fStartU; pVtx[0].v = fStartV;
	pVtx[0].r = pVtx[0].g = pVtx[0].b = pVtx[0].a = 1.0f;
	pVtx[1].x = (float)destrect->right; pVtx[1].y = (float)destrect->top; pVtx[1].u = fMaxU; pVtx[1].v = fStartV;
	pVtx[1].r = pVtx[1].g = pVtx[1].b = pVtx[1].a = 1.0f;
	pVtx[2].x = (float)destrect->right; pVtx[2].y = (float)destrect->bottom; pVtx[2].u = fMaxU; pVtx[2].v = fMaxV;
	pVtx[2].r = pVtx[2].g = pVtx[2].b = pVtx[2].a = 1.0f;
	pVtx[3].x = (float)destrect->left; pVtx[3].y = (float)destrect->bottom; pVtx[3].u = fStartU; pVtx[3].v = fMaxV;
	pVtx[3].r = pVtx[3].g = pVtx[3].b = pVtx[3].a = 1.0f;	

	// COBRA - RED - Pit Vibrations
	OTWDriver.pCockpitManager->AddTurbulence(pVtx);

	OTWDriver.renderer->context.RestoreState(alpha);
	OTWDriver.renderer->context.SelectTexture1((GLint) pTex);
	OTWDriver.renderer->context.DrawPrimitive(MPR_PRM_TRIFAN,MPR_VI_COLOR|MPR_VI_TEXTURE,4,pVtx,sizeof(pVtx[0]));

}


void	CPIndicator::DisplayBlit(void) {
	int	i;
   RECT temp;

	mDirtyFlag = TRUE;

	if(!mDirtyFlag) {
		return;
	}

	if (DisplayOptions.bRender2DCockpit)			//Handle these in displayblit3d
		return;	

	for(i = 0; i < mNumTapes; i++) {
		temp = mpDestLocs[i];

		temp.top		= (LONG)(temp.top * mVScale);
		temp.left	= (LONG)(temp.left * mHScale);
		temp.bottom	= (LONG)(temp.top + mVScale * (mpSrcLocs[i].bottom - mpSrcLocs[i].top));
		temp.right	= (LONG)(temp.left + mHScale * (mpSrcLocs[i].right - mpSrcLocs[i].left));
		mpOTWImage->Compose(mpTemplate, &mpSrcLocs[i], &temp);
	}

	mDirtyFlag = FALSE;
}


void	CPIndicator::DisplayBlit3D(void) {
	int	i;
   RECT temp;

	mDirtyFlag = TRUE;

	if(!mDirtyFlag) {
		return;
	}

	if (!DisplayOptions.bRender2DCockpit)			//Handle these in displayblit
		return;	


	for(i = 0; i < mNumTapes; i++) {
		temp = mpDestLocs[i];

		temp.top	= FloatToInt32(temp.top * mVScale);
		temp.left	= FloatToInt32(temp.left * mHScale);
		temp.bottom	= FloatToInt32(temp.top + mVScale * (mpSrcLocs[i].bottom - mpSrcLocs[i].top));
		temp.right	= FloatToInt32(temp.left + mHScale * (mpSrcLocs[i].right - mpSrcLocs[i].left));		
		
		if (g_bFilter2DPit){
			//Wombat778 3-30-04 Added option to filter
			RenderIndicatorPoly(&mpSourceBuffer[i],&mpSrcRects[i],&mpSrcLocs[i],&temp,STATE_TEXTURE);				
		}
		else{
			RenderIndicatorPoly(&mpSourceBuffer[i],&mpSrcRects[i],&mpSrcLocs[i],&temp,STATE_TEXTURE_NOFILTER);		
		}
	}

	mDirtyFlag = FALSE;
}


//Wombat778 3-22-04 Additional functions for rendering the image
void CPIndicator::CreateLit(void)
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

			for(int i = 0; i < mNumTapes; i++)
			{										
				// Check if we can use a single texture
				if((int)dwMaxTextureWidth >= mpSourceBuffer[i].mWidth && (int)dwMaxTextureHeight >= mpSourceBuffer[i].mHeight) 
				{
					TextureHandle *pTex = new TextureHandle;
					if(!pTex)
						throw _com_error(E_OUTOFMEMORY);
					m_pPalette->AttachToTexture(pTex);
					if(!pTex->Create("CPIndicator", MPR_TI_PALETTE | MPR_TI_CHROMAKEY, 8, mpSourceBuffer[i].mWidth, mpSourceBuffer[i].mHeight))
						throw _com_error(E_FAIL);
					if(!pTex->Load(0, 0xFFFF0000, (BYTE*) mpSourceBuffer[i].indicator, true, true))	// soon to be re-loaded by CPSurface::Translate3D
						throw _com_error(E_FAIL);
					mpSourceBuffer[i].m_arrTex.push_back(pTex);
				}
			}

		}
		catch(_com_error e)
		{
			MonoPrint("CPIndicator::CreateLit - Error 0x%X (%s)\n", e.Error(), e.ErrorMessage());
			DiscardLit();
		}
	}
}

void CPIndicator::DiscardLit(void)
{	
	if (DisplayOptions.bRender2DCockpit)
	{
		for(int i2 = 0; i2 < mNumTapes; i2++)
		{										
			for(int i=0;i<(int)mpSourceBuffer[i2].m_arrTex.size();i++)	//delete the textures for each tape
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