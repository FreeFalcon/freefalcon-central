#include "stdafx.h"
#include "stdhdr.h"
#include "aircrft.h"
#include "simdrive.h"

#include "cpmanager.h"
#include "cphsi.h"
#include "cpmisc.h"
#include "camp2sim.h"
#include "fack.h"

//====================================================//
// CPMisc::CPMisc
//====================================================//

CPMisc::CPMisc() {

	mChaffFlareMode		= none;
	mChaffFlareControl	= manual;

	mMasterCautionLightState	= 0;
	mMasterCautionEvent		= FALSE;
	mEjectState				= FALSE;

	mRefuelState			= 0;
	mRefuelTimer 			= 0;

	mUHFPosition			= 0;

	memset(MFDButtonArray, 0, sizeof(int) * MFD_BUTTONS * 4);		//Wombat778 4-12-04 changed from 2 to 4
}


void CPMisc::StepUHFPostion(void)
{	
	mUHFPosition++;
	mUHFPosition %= 8;
}

void CPMisc::DecUHFPosition(void)
{
	if (mUHFPosition == 0)  // wrap
		mUHFPosition = 7;
	else
		mUHFPosition--;
}

void CPMisc::SetRefuelState(int state)
{
	mRefuelState = state;
	mRefuelTimer = vuxGameTime;
}


void CPMisc::SetEjectButtonState(BOOL newState) {

	mEjectState = newState;
}

BOOL CPMisc::GetEjectButtonState(void) {

	return mEjectState;
}

int CPMisc::GetMasterCautionLight(void) {
	
	return mMasterCautionLightState;
}

void CPMisc::SetMasterCautionEvent(void) {
		mMasterCautionEvent = TRUE;
}

void CPMisc::StepMasterCautionLight(void) {

	BOOL	masterCautionState;
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL || !playerAC->IsSetFlag(MOTION_OWNSHIP)){
		return;
	}

	masterCautionState	= playerAC->mFaults->MasterCaution();
		
	if(mMasterCautionLightState == 0 && masterCautionState == TRUE) {
		mMasterCautionLightState  = 1;
	}
	else if(mMasterCautionLightState == 1 && masterCautionState == TRUE) {
		mMasterCautionLightState  = 1;
	}
	else if(mMasterCautionLightState == 1 && masterCautionState == FALSE) {
		mMasterCautionLightState  = 2;
	}
	else if(mMasterCautionLightState == 2) {
		mMasterCautionLightState  = 3;
	}
	else if(mMasterCautionLightState == 0 && masterCautionState == FALSE && mMasterCautionEvent) {
		mMasterCautionLightState  = 3;
		mMasterCautionEvent = FALSE;
	}
	else {
		mMasterCautionLightState  = 0;
	}
}






int CPMisc::GetMFDButtonState(int side, int button) {

	return MFDButtonArray[button - 1][side];
}


void CPMisc::SetMFDButtonState(int side, int button, int value) {

	if(SimDriver.GetPlayerEntity() && SimDriver.GetPlayerEntity()->IsSetFlag(MOTION_OWNSHIP)) {
		MFDButtonArray[button - 1][side] = value;
	}
}











