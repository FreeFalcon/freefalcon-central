#include "stdafx.h"

#include "cpobject.h"

//====================================================//
// CPObject::CPObject
//====================================================//

CPObject::CPObject(const ObjectInitStr* pobjectInitStr){

	mIdNum				= pobjectInitStr->idNum;
	mHScale				= pobjectInitStr->hScale;
	mVScale				= pobjectInitStr->vScale;
	mPersistant			= pobjectInitStr->persistant;
	mBSurface			= pobjectInitStr->bsurface;
	mBSrcRect			= pobjectInitStr->bsrcRect;
	mBDestRect			= pobjectInitStr->bdestRect;
	mpBackgroundSurface = NULL; //VWF May be able to remove later

	mDestRect.top		= pobjectInitStr->destRect.top;
	mDestRect.left		= pobjectInitStr->destRect.left;
	mDestRect.bottom	= (pobjectInitStr->destRect.bottom + 1);
	mDestRect.right	= (pobjectInitStr->destRect.right + 1);

	mTransparencyType	= pobjectInitStr->transparencyType;

	mWidth				= mDestRect.right - mDestRect.left; 
	mHeight				= mDestRect.bottom - mDestRect.top;

	mDestRect.top		= (long)(mVScale * mDestRect.top);
	mDestRect.left		= (long)(mHScale * mDestRect.left);
	mDestRect.bottom	= (long)(mVScale * mDestRect.bottom);
	mDestRect.right		= (long)(mHScale * mDestRect.right);

	mpOTWImage			= pobjectInitStr->pOTWImage;
	mpTemplate			= pobjectInitStr->pTemplate;
	mpCPManager			= pobjectInitStr->pCPManager;

	mCycleBits			= pobjectInitStr->cycleBits;
	mDirtyFlag			= TRUE;
	mCallbackSlot		= pobjectInitStr->callbackSlot;

	// JPO - test the limits.
	if(mCallbackSlot < 0 || mCallbackSlot >= TOTAL_CPCALLBACK_SLOTS){
		mExecCallback		= NULL;
		mDisplayCallback	= NULL;
		mEventCallback		= NULL;
	}
	else {
		mExecCallback		= CPCallbackArray[mCallbackSlot].ExecCallback;
		mDisplayCallback	= CPCallbackArray[mCallbackSlot].DisplayCallback;
		mEventCallback		= CPCallbackArray[mCallbackSlot].EventCallback;
	}

	// OW
	m_pPalette = NULL;
}

CPObject::~CPObject()
{
	// OW
	for(int i=0;i<(int)m_arrTex.size();i++){
		delete m_arrTex[i];
	}
	m_arrTex.clear();

	if (m_pPalette){
		delete m_pPalette;
	}
}

void CPObject::DiscardLit()
{
	for(int i=0;i<(int)m_arrTex.size();i++) delete m_arrTex[i];
	m_arrTex.clear();

	if(m_pPalette)
	{
	  // JPO - memory leak fix
		delete m_pPalette;
		m_pPalette = NULL;
	}
}

void CPObject::Translate3D(DWORD* palette32){
	if (m_pPalette){
		m_pPalette->Load(MPR_TI_PALETTE, 32, 0, 256, (BYTE*) palette32);
	}
}
