#include "stdafx.h"
#include "cpcb.h"
#include "cpmanager.h"
#include "button.h"
#include "soundfx.h"
#include "fsound.h"

//#include "trainingscript.h"

#include "Graphics/Include/grinline.h"	//Wombat778 3-22-04
extern bool g_bFilter2DPit;		//Wombat778 3-30-04

//===================================
// Function Bodies for CPButtonObject
//===================================

//------------------------------------------------------------------
// CPButtonObject::CPButtonObject
//------------------------------------------------------------------

CPButtonObject::CPButtonObject(ButtonObjectInitStr *pInitStr) {

	mIdNum				= pInitStr->objectId;
	mCallbackSlot		= pInitStr->callbackSlot;
	mTotalStates		= pInitStr->totalStates;
	mNormalState		= pInitStr->normalState;

#ifndef _CPBUTTON_USE_STL_CONTAINERS
	mTotalViews			= pInitStr->totalViews;
	mViewSlot			= 0;
#endif

	mCursorIndex		= pInitStr->cursorIndex;
	mDelay				= pInitStr->delay;

	mDelayInc			= -1;
	mCurrentState		= mNormalState;

	mSound1				= pInitStr->sound1;
	mSound2				= pInitStr->sound2;

	// JPO - lets be careful out there
	if(mCallbackSlot >= 0 && mCallbackSlot < TOTAL_BUTTONCALLBACK_SLOTS) {

		mTransStateToAero	= ButtonCallbackArray[mCallbackSlot].TransStateToAero;
		mTransAeroToState	= ButtonCallbackArray[mCallbackSlot].TransAeroToState;
	}
	else {

		mTransStateToAero = NULL;
		mTransAeroToState = NULL;
	}


#ifndef _CPBUTTON_USE_STL_CONTAINERS
	#ifdef USE_SH_POOLS
	mpButtonView = (CPButtonView **)MemAllocPtr(gCockMemPool,sizeof(CPButtonView *)*mTotalViews,FALSE);
	#else
	mpButtonView		= new CPButtonView*[mTotalViews];
	#endif
	
	memset(mpButtonView, 0, mTotalViews * sizeof(CPButtonView*));
#endif
}


//------------------------------------------------------------------
// CPButtonObject::HandleEvent
//------------------------------------------------------------------

void CPButtonObject::HandleEvent(int event) {

	if(mTransAeroToState) {
		mTransAeroToState(this, event);			// translate aero to button click		
	}					

	mDelayInc = mDelay;

	if(mSound1 >= 0) {
		F4SoundFXSetDist(mSound1, FALSE, 0.0f, 1.0f);
	}

	NotifyViews();
}

//------------------------------------------------------------------
// CPButtonObject::HandleMouseEvent
//------------------------------------------------------------------

void CPButtonObject::HandleMouseEvent(int event) {

	if(event == CP_MOUSE_BUTTON0 || 
		event == CP_MOUSE_BUTTON1) {	

		//Wombat778 3-09-04 Check if this function is being blocked by the training script
		//if( mTransStateToAero && !TrainingScript->IsBlocked(NULL,mCallbackSlot) )
		if(mTransStateToAero){
			mTransStateToAero(this, event);     //translate button click to aero
			//Wombat778 3-06-04 If the scripting object is capturing, add the callback
			//if (TrainingScript->IsCapturing())	
			//	TrainingScript->CaptureCommand(NULL, mCallbackSlot);
		}
		HandleEvent(event);
	}
}

int CPButtonObject::GetSound(int which) const {

	int returnVal = -1;

	F4Assert(which == 1 || which == 2);	//values for which can only be 1 or 2;

	if(which == 1) {
		returnVal = mSound1;
	}
	else if (which == 2) {
		returnVal = mSound2;
	}

	return returnVal;
}


void CPButtonObject::SetSound(int which, int index) {

	if(which == 1) {
		mSound1 = index;
	}
	else if (which == 2) {
		mSound2 = index;
	}
	else {
		ShiWarning("Which can only be 1 or 2");	//values for which can only be 1 or 2;
	}
}

//------------------------------------------------------------------
// CPButtonObject::DecrementDelay
//------------------------------------------------------------------

void CPButtonObject::DecrementDelay(void) {

	if(mDelayInc < 0) {
		return;
	}
	else if(mDelayInc == 0) {
		mDelayInc--;
		mCurrentState = mNormalState;
		NotifyViews();
		if(mSound2 >= 0) {
			F4SoundFXSetDist(mSound2, FALSE, 0.0f, 1.0f);
		}
	}
	else if(mDelayInc > 0) {
		mDelayInc--;
	}
}


//------------------------------------------------------------------
// CPButtonObject::~CPButtonObject
//------------------------------------------------------------------

CPButtonObject::~CPButtonObject()
{
#ifndef _CPBUTTON_USE_STL_CONTAINERS
	delete [] mpButtonView;
#endif
}

//------------------------------------------------------------------
// CPButtonObject::AddView
//------------------------------------------------------------------

void CPButtonObject::AddView(CPButtonView* pCPButtonView)
{
#ifndef _CPBUTTON_USE_STL_CONTAINERS
	mpButtonView[mViewSlot++] = pCPButtonView;
#else
	mpButtonView.push_back(pCPButtonView);
#endif

	pCPButtonView->SetParentButtonPointer(this);
}


//------------------------------------------------------------------
// CPButtonObject::DoBlit
//------------------------------------------------------------------

BOOL CPButtonObject::DoBlit(void) {

	return(mCurrentState != mNormalState);
}

//------------------------------------------------------------------
// CPButtonObject::NotifyViews
//------------------------------------------------------------------

void CPButtonObject::NotifyViews() {

	int		i;

#ifndef _CPBUTTON_USE_STL_CONTAINERS
	for(i = 0; i < mTotalViews; i++) {
#else
	for(i = 0; i < (int)mpButtonView.size(); i++) {
#endif
		mpButtonView[i]->SetDirtyFlag();
	}
}

//------------------------------------------------------------------
// CPButtonObject::GetCurrentState
//------------------------------------------------------------------
/*
int CPButtonObject::	GetCurrentState() {

	return mCurrentState;
}
*/

//------------------------------------------------------------------
// CPButtonObject::GetNormalState
//------------------------------------------------------------------
/*	
int CPButtonObject::GetNormalState() {

	return mNormalState;
}
*/
//------------------------------------------------------------------
// CPButtonObject::GetId
//------------------------------------------------------------------
/*
int CPButtonObject::GetId() {

	return mIdNum;
}
*/
//------------------------------------------------------------------
// CPButtonObject::GetCursorIndex
//------------------------------------------------------------------
/*	
int CPButtonObject::GetCursorIndex() {

	return mCursorIndex;
}
*/
//------------------------------------------------------------------
// CPButtonObject::SetCurrentState
//------------------------------------------------------------------

void CPButtonObject::SetCurrentState(int newState) {
	// sfr: @todo should we change the state if its bad? wouldnt it be safer to keep the old one?
	if(newState < 0 || newState >= mTotalStates) {	
		// If getting passed a bad state
		mCurrentState = mNormalState;
	}
	else {
		mCurrentState = newState;
	}
}

void CPButtonObject::IncrementState(){
	if (mCurrentState < (mTotalStates -1)){
		++mCurrentState;
	}
}
void CPButtonObject::DecrementState(){
	if (mCurrentState > 0){
		--mCurrentState;
	}
}


// call the update function if we have one.
void CPButtonObject::UpdateStatus(){
    if(mTransAeroToState) { 
		// JPO special call so we can distinguish between action and check - if we care
		mTransAeroToState(this, CP_CHECK_EVENT); // translate aero to button click
    }					
}


//===================================
// Function Bodies for CPButtonView
//===================================

//------------------------------------------------------------------
// CPButtonView::CPButtonView
//------------------------------------------------------------------

CPButtonView::CPButtonView(ButtonViewInitStr* pButtonViewInitStr) {

	mIdNum				= pButtonViewInitStr->objectId;
	mParentButton		= pButtonViewInitStr->parentButton;
	mTransparencyType	= pButtonViewInitStr->transparencyType;
	mDestRect			= pButtonViewInitStr->destRect;
	mpSrcRect			= pButtonViewInitStr->pSrcRect;	// List of Rects
	mpOTWImage			= pButtonViewInitStr->pOTWImage;
	mpTemplate			= pButtonViewInitStr->pTemplate;
	mPersistant			= pButtonViewInitStr->persistant;
	mStates				= pButtonViewInitStr->states;
	mHScale				= pButtonViewInitStr->hScale;
	mVScale				= pButtonViewInitStr->vScale;

	mDestRect.top		= (LONG)(mDestRect.top * mVScale);
	mDestRect.left		= (LONG)(mDestRect.left * mHScale);
	mDestRect.bottom	= (LONG)(mDestRect.bottom * mVScale);
	mDestRect.right	= (LONG)(mDestRect.right * mHScale);

	//Wombat778 3-22-04  Added following for rendered (rather than blitted buttonviews)
	if (DisplayOptions.bRender2DCockpit)
	{
		mpSourceBuffer = pButtonViewInitStr->sourcebuttonview;		

		for( int i = 0; i < mStates; i++)
		{	
			mpSourceBuffer[i].mWidth		= mpSrcRect[i].right - mpSrcRect[i].left;
			mpSourceBuffer[i].mHeight		= mpSrcRect[i].bottom - mpSrcRect[i].top;
		}
	}	
	//Wombat778 end

	m_pPalette = NULL;			//Wombat778 3-23-04 added to initialize new variable
}

//------------------------------------------------------------------
// CPButtonView::~CPButtonView
//------------------------------------------------------------------

CPButtonView::~CPButtonView() {

	if(mpSrcRect) {
		delete [] mpSrcRect;
	}

	//Wombat778 3-22-04 clean up buffers
	if (DisplayOptions.bRender2DCockpit)
	{		
		for(int i = 0; i < mStates; i++){		
			glReleaseMemory((char*) mpSourceBuffer[i].buttonview);		
		}
		delete [] mpSourceBuffer;
	}
}

//------------------------------------------------------------------
// CPButtonView::Display
//------------------------------------------------------------------

void CPButtonView::DisplayBlit(void) {
	
#if _ALWAYS_DIRTY
	mDirtyFlag = TRUE;
#endif

	if(!mDirtyFlag) {
		return;
	}

	if (DisplayOptions.bRender2DCockpit)			//Handle these in displayblit3d
		return;	

	if(mpButtonObject->DoBlit() && mStates) {
		if(mTransparencyType == CPTRANSPARENT) {
			mpOTWImage->ComposeTransparent(mpTemplate, &mpSrcRect[mpButtonObject->GetCurrentState()], &mDestRect);
		}
		else {
			mpOTWImage->Compose(mpTemplate, &mpSrcRect[mpButtonObject->GetCurrentState()], &mDestRect);
		}
	}

	mpButtonObject->DecrementDelay();
	mDirtyFlag = FALSE;
}

void RenderButtonViewPoly(SourceButtonViewType *sb, tagRECT *destrect, GLint alpha)		//Wombat778 3-22-04 helper function to keep the displayblit3d tidy. 
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
	pVtx[0].x = (float)destrect->left; pVtx[0].y = (float)destrect->top; pVtx[0].u = fStartU; pVtx[0].v = fStartV;
	pVtx[0].r = pVtx[0].g = pVtx[0].b = pVtx[0].a = 1.0f;
	pVtx[1].x = (float)destrect->right; pVtx[1].y = (float)destrect->top; pVtx[1].u = fMaxU; pVtx[1].v = fStartV;
	pVtx[1].r = pVtx[1].g = pVtx[1].b = pVtx[1].a = 1.0f;
	pVtx[2].x = (float)destrect->right; pVtx[2].y = (float)destrect->bottom; pVtx[2].u = fMaxU; pVtx[2].v = fMaxV;
	pVtx[2].r = pVtx[2].g = pVtx[2].b = pVtx[2].a = 1.0f;
	pVtx[3].x = (float)destrect->left; pVtx[3].y = (float)destrect->bottom; pVtx[3].u = fStartU; pVtx[3].v = fMaxV;
	pVtx[3].r = pVtx[3].g = pVtx[3].b = pVtx[3].a = 1.0f;	

	OTWDriver.pCockpitManager->AddTurbulence(pVtx);
	OTWDriver.renderer->context.RestoreState(alpha);
	OTWDriver.renderer->context.SelectTexture1((GLint) pTex);
	OTWDriver.renderer->context.DrawPrimitive(MPR_PRM_TRIFAN,MPR_VI_COLOR|MPR_VI_TEXTURE,4,pVtx,sizeof(pVtx[0]));

}

void CPButtonView::DisplayBlit3D(void) {
	
#if _ALWAYS_DIRTY
	mDirtyFlag = TRUE;
#endif

	if(!mDirtyFlag) {
		return;
	}

	if (!DisplayOptions.bRender2DCockpit)			//Handle these in displayblit
		return;	

	if(mpButtonObject->DoBlit() && mStates) {

		if(mTransparencyType == CPTRANSPARENT) {

			if (g_bFilter2DPit)		//Wombat778 3-30-04 Add option to filter
				RenderButtonViewPoly(&mpSourceBuffer[mpButtonObject->GetCurrentState()],&mDestRect,STATE_CHROMA_TEXTURE);				
			else
				RenderButtonViewPoly(&mpSourceBuffer[mpButtonObject->GetCurrentState()],&mDestRect,STATE_ALPHA_TEXTURE_NOFILTER);				
		}
		else {
			
			if (g_bFilter2DPit)		//Wombat778 3-30-04 Add option to filter
				RenderButtonViewPoly(&mpSourceBuffer[mpButtonObject->GetCurrentState()],&mDestRect,STATE_TEXTURE);						
			else
				RenderButtonViewPoly(&mpSourceBuffer[mpButtonObject->GetCurrentState()],&mDestRect,STATE_TEXTURE_NOFILTER);						
		}
	}

	mpButtonObject->DecrementDelay();

	mDirtyFlag = FALSE;
}

//Wombat778 3-22-04 Additional functions for rendering the image

void CPButtonView::CreateLit(void)
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
				if((int)dwMaxTextureWidth >= mpSourceBuffer[i].mWidth && (int)dwMaxTextureHeight >= mpSourceBuffer[i].mHeight) 
				{
					TextureHandle *pTex = new TextureHandle;
					if(!pTex)
						throw _com_error(E_OUTOFMEMORY);
					m_pPalette->AttachToTexture(pTex);
					if(!pTex->Create("CPButtonView", MPR_TI_PALETTE | MPR_TI_CHROMAKEY, 8, mpSourceBuffer[i].mWidth, mpSourceBuffer[i].mHeight))
						throw _com_error(E_FAIL);
					if(!pTex->Load(0, 0xFFFF0000, (BYTE*) mpSourceBuffer[i].buttonview, true, true))	// soon to be re-loaded by CPSurface::Translate3D
						throw _com_error(E_FAIL);
					mpSourceBuffer[i].m_arrTex.push_back(pTex);
				}
			}

		}
		catch(_com_error e)
		{
			MonoPrint("CPButtonView::CreateLit - Error 0x%X (%s)\n", e.Error(), e.ErrorMessage());
			DiscardLit();
		}
	}
}

void CPButtonView::DiscardLit(void)
{	
	if (DisplayOptions.bRender2DCockpit)
	{
		for(int i2 = 0; i2 < mStates; i2++)
		{										
			for(int i=0;i<(int)mpSourceBuffer[i2].m_arrTex.size();i++)	//delete the textures for each buttonview
				delete mpSourceBuffer[i2].m_arrTex[i];
			mpSourceBuffer[i2].m_arrTex.clear();	
		}
	}	

	if(m_pPalette)
	{
	  delete m_pPalette; // JPO - memory leak fix
	  m_pPalette = NULL;
	}
}

//buttonviews arent objects, so this needs to be added to get a proper palette
void CPButtonView::Translate3D(DWORD* palette32){
	if(m_pPalette){
		m_pPalette->Load(MPR_TI_PALETTE, 32, 0, 256, (BYTE*) palette32);
	}
}

//Wombat778 end

//------------------------------------------------------------------
// CPButtonView::GetId
//------------------------------------------------------------------

int CPButtonView::GetId() {

	return mIdNum;
}

//------------------------------------------------------------------
// CPButtonView::GetTransparencyType
//------------------------------------------------------------------

int CPButtonView::GetTransparencyType() {

	return mTransparencyType;
}

//------------------------------------------------------------------
// CPButtonView::GetParentButton
//------------------------------------------------------------------

int CPButtonView::GetParentButton(void) {

	return mParentButton;
}

//------------------------------------------------------------------
// CPButtonView::GetParentButton
//------------------------------------------------------------------

void CPButtonView::SetParentButtonPointer(CPButtonObject* pButtonObject) {

	mpButtonObject = pButtonObject;
}



//------------------------------------------------------------------
// CPButtonView::HandleEvent
//------------------------------------------------------------------

BOOL CPButtonView::HandleEvent(int* cursorIndex, int event, int xpos, int ypos) {


	BOOL isTarget = FALSE;

	if(xpos >= mDestRect.left && 
		xpos <= mDestRect.right && 
		ypos >= mDestRect.top &&
		ypos <= mDestRect.bottom) {

		isTarget = TRUE;
		*cursorIndex = mpButtonObject->GetCursorIndex();

		mpButtonObject->HandleMouseEvent(event);
	}

	return isTarget;
}

void CPButtonView::UpdateView()
{
    mpButtonObject->UpdateStatus();
}

//Wombat778 3-09-04 Function to return a buttonview's callback id and x/y position. Used for the trainingscript.
int CPButtonView::GetCallBackAndXY(int *x, int *y)			
{ 
	*x = (int)(mDestRect.left+((mDestRect.right-mDestRect.left)/2.0f));
	*y = (int)(mDestRect.top+((mDestRect.bottom-mDestRect.top)/2.0f));
	return mpButtonObject->GetCallbackId(); 
}
