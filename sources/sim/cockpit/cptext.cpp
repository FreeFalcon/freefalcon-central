#include "stdafx.h"
#include "simdrive.h"
#include "cpmanager.h"
#include "cptext.h"
#include "dispopts.h"
#include "Graphics\Include/renderow.h"
#include "otwdrive.h"

//====================================================//
// CPText::CPText
//====================================================//


CPText::CPText(ObjectInitStr *pobjectInitStr, int numStrings) : CPObject(pobjectInitStr) {
	float		halfWidth;
	float		halfHeight;

	int i;

	mNumStrings = numStrings;

	#ifdef USE_SH_POOLS
	mpString = (char **)MemAllocPtr(gCockMemPool,sizeof(char *)*mNumStrings,FALSE);
	#else
	mpString = new char*[mNumStrings];
	#endif

	for(i = 0; i < mNumStrings; i++) {
		#ifdef USE_SH_POOLS
		mpString[i] = (char *)MemAllocPtr(gCockMemPool,sizeof(char)*20,FALSE);
		#else
		mpString[i] = new char[20];
		#endif
	}

	halfWidth				= (float) DisplayOptions.DispWidth * 0.5F;
	halfHeight				= (float) DisplayOptions.DispHeight * 0.5F;

	mLeft		= (mDestRect.left - halfWidth) / halfWidth;
   mRight	= (mDestRect.right - halfWidth) / halfWidth;
   mTop		= -(mDestRect.top - halfHeight) / halfHeight;
   mBottom	= -(mDestRect.bottom - halfHeight) / halfHeight;
}


CPText::~CPText() {
	int i;

	for(i = 0; i < mNumStrings; i++) {
		delete mpString[i];
	}

	delete mpString;
}


void CPText::DisplayDraw(void)
{
	int oldFont = VirtualDisplay::CurFont();
	int	i;
	int	start;
	int	step;

	step = 11;
	start = mDestRect.top + 6;

	OTWDriver.renderer->SetViewport(mLeft, mTop, mRight, mBottom);
	OTWDriver.renderer->SetBackground(0x00000000);
	OTWDriver.renderer->ClearDraw();

	VirtualDisplay::SetFont(mpCPManager->SABoxFont());
	OTWDriver.renderer->SetColor(0xFF00A200);
	for(i = 0; i < mNumStrings; i++) {
		OTWDriver.renderer->ScreenText((float)mDestRect.left + 5.0F, (float)start, mpString[i]);
		start += step;
	}
	VirtualDisplay::SetFont(oldFont);
}


void CPText::Exec(SimBaseClass* pOwnship){
	
	mpOwnship = pOwnship;

	if(mExecCallback) {
		mExecCallback(this);
	}
}
