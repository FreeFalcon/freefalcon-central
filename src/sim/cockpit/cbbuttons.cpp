#include "stdafx.h"
#include "stdhdr.h"
#include "sms.h"
#include "fcc.h"
#include "mfd.h"
#include "simdrive.h"
#include "radar.h"
#include "commands.h"
#include "aircrft.h"
#include "airframe.h"
#include "alr56.h"
#include "cpcb.h"
#include "button.h"
#include "icp.h"
#include "cpmisc.h"
#include "commands.h"
#include "hud.h"
#include "camp2sim.h"
#include "fack.h"
#include "fault.h"
#include "navsystem.h"
#include "cphsi.h"
#include "kneeboard.h"
#include "dofsnswitches.h"
#include "PilotInputs.h"	//MI
#include "SimIO.h" // MD

extern bool g_bRealisticAvionics;
extern bool g_bIFF;
extern bool g_bMLU;

void CBEKneeboardMap(void *, int) {
	KneeBoard *board = OTWDriver.pCockpitManager->mpKneeBoard;
	board->SetPage( KneeBoard::MAP );
}

void CBEKneeboardBrief(void *, int) {
	KneeBoard *board = OTWDriver.pCockpitManager->mpKneeBoard;
	board->SetPage( KneeBoard::BRIEF );
}

void CBEKneeboardStpt(void *, int) {
	KneeBoard *board = OTWDriver.pCockpitManager->mpKneeBoard;
	board->SetPage( KneeBoard::STEERPOINT );
}

// Auto Manual Chaff Flare stuff
void CBEAMChaffFlare(void *, int) {
	SimToggleDropPattern (0, KEY_DOWN, NULL);
}


void CBExAMChaffFlare(void * pButton, int) {

	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	if (playerAC->IsSetFlag(MOTION_OWNSHIP)){
		PlayerRwrClass* theRwr = (PlayerRwrClass*)FindSensor(playerAC, SensorClass::RWR);
		if(theRwr) {
			pCPButton->SetCurrentState(theRwr->AutoDrop());
		}
	}
}



void CBEHandoffB(void *, int) {
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	if ((playerAC->IsSetFlag(MOTION_OWNSHIP))) {
		PlayerRwrClass* theRwr = (PlayerRwrClass*)FindSensor(playerAC, SensorClass::RWR);
		if(theRwr) {
			theRwr->SelectNextEmitter();

// JB 010727 RP5 RWR
// 2001-02-15 ADDED BY S.G. SO WE HEAR THE SOUND ***RIGHT AWAY*** 
//            I WON'T PASS A targetList (although I could) SO NOT ALL OF THE ROUTINE IS DONE
//            IN 1.08i2, DoAudio PROCESSES THE WHOLE CONTACT LIST BY ITSELF AND 
//            NOT JUST THE PASSED CONTACT. SINCE 1.07 DOESN'T I'M STUCK AT DOING THIS :-(
//            THIS WON'T BE FPS INTENSIVE ANYWAY SINCE IT ONLY RUNS WHEN THE HANDOFF BUTTON IS PRESSED
//            LATER ON, I MIGHT MAKE THIS CODE 1.08i2 'COMPATIBLE'
			theRwr->Exec(NULL);
		}
	}
}


void CBEPriModeB(void *, int) {
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	if (playerAC->IsSetFlag(MOTION_OWNSHIP)) {
		PlayerRwrClass* theRwr = (PlayerRwrClass*)FindSensor(playerAC, SensorClass::RWR);
		if(theRwr) {
			theRwr->TogglePriority();
		}
	}
}


void CBEUnknownB(void *, int) {
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	if ((playerAC->IsSetFlag(MOTION_OWNSHIP))) {
		PlayerRwrClass* theRwr = (PlayerRwrClass*)FindSensor(playerAC, SensorClass::RWR);
		if(theRwr) {
			theRwr->ToggleUnknowns();
		}
	}
}

void CBENavalB(void *, int) {
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	if ((playerAC->IsSetFlag(MOTION_OWNSHIP))) {
		PlayerRwrClass* theRwr = (PlayerRwrClass*)FindSensor(playerAC, SensorClass::RWR);
		if(theRwr) {
			theRwr->ToggleNaval();
		}
	}
}

void CBETgtSepB(void *, int) {
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	if ((playerAC->IsSetFlag(MOTION_OWNSHIP))) {
		PlayerRwrClass* theRwr = (PlayerRwrClass*)FindSensor(playerAC, SensorClass::RWR);
		if(theRwr) {
			theRwr->ToggleTargetSep();
		}
	}
}


void CBEAuxWarnSearchB(void *, int) {
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	if ((playerAC->IsSetFlag(MOTION_OWNSHIP))) {
		PlayerRwrClass* theRwr = (PlayerRwrClass*)FindSensor(playerAC, SensorClass::RWR);
		if(theRwr) {
			theRwr->ToggleSearch();
		}
	}
}

void CBEAuxWarnAltB(void *, int) {
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	if ((playerAC->IsSetFlag(MOTION_OWNSHIP))) {
		PlayerRwrClass* theRwr = (PlayerRwrClass*)FindSensor(playerAC, SensorClass::RWR);
		if(theRwr) {
			theRwr->ToggleLowAltPriority();
		}
	}
}

void CBEAuxWarnPwrB(void *, int) {
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	if ((playerAC->IsSetFlag(MOTION_OWNSHIP))) {
		PlayerRwrClass* theRwr = (PlayerRwrClass*)FindSensor(playerAC, SensorClass::RWR);
		if(theRwr) {
			theRwr->SetPower(!theRwr->IsOn());
		}
	}
}



//Landing Gear Handle
void CBELandGearSelect(void *, int) {

	AFGearToggle(0, KEY_DOWN, NULL);
}

void CBEMasterCaution(void *, int) {

	ExtinguishMasterCaution(0, KEY_DOWN, NULL);
}

void CBEAutoPilot(void *, int) {
	
	SimToggleAutopilot(0, KEY_DOWN, NULL);
}

void CBExAutoPilot(void * pButton, int) {

	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

	if ( (playerAC == NULL) || !playerAC->IsSetFlag(MOTION_OWNSHIP) ){
		return;
	}

	if (playerAC->autopilotType == AircraftClass::APOff) {
		pCPButton->SetCurrentState(0);
	}
	else {
		pCPButton->SetCurrentState(1);
	}
}

void CBECourseSelect(void *, int event) {

	if(event == CP_MOUSE_BUTTON0) {
		SimHsiCourseInc (0, KEY_DOWN, NULL);
	}
	else if(event == CP_MOUSE_BUTTON1) {		
		SimHsiCourseDec (0, KEY_DOWN, NULL);
	}
}

void CBExCourseSelect(void * pButton, int) {

	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	pCPButton->SetCurrentState(OTWDriver.pCockpitManager->mpHsi->GetState(CPHsi::HSI_STA_CRS_STATE));
}

void CBEHeadingSelect(void *, int event) {

	if(event == CP_MOUSE_BUTTON0) {
		SimHsiHeadingInc (0, KEY_DOWN, NULL);
	}
	else if(event == CP_MOUSE_BUTTON1) {		
		SimHsiHeadingDec (0, KEY_DOWN, NULL);
	}
}

void CBExHeadingSelect(void * pButton, int) {

	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;

	pCPButton->SetCurrentState(OTWDriver.pCockpitManager->mpHsi->GetState(CPHsi::HSI_STA_HDG_STATE));
}


void CBEChaffDispense(void *, int) {

	SimDropChaff (0, KEY_DOWN, NULL);
}

void CBExChaffDispense(void * pButton, int event) {

	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	pCPButton->SetCurrentState(event == CP_CHECK_EVENT ? 0 : 1); // JPO
}

void CBEFlareDispense(void *, int) {

	SimDropFlare (0, KEY_DOWN, NULL);
}

void CBExFlareDispense(void * pButton, int event) {

	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	pCPButton->SetCurrentState(event == CP_CHECK_EVENT ? 0 : 1); // JPO
}

void CBEStoresJettison(void *, int) {
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}
	
	SimEmergencyJettison(0, KEY_DOWN, NULL);
	if(playerAC->Sms != NULL && playerAC->Sms->DidEmergencyJettison()){
		SimEmergencyJettison(0, 0, NULL);
	}
}


void CBExStoresJettison(void * pButton, int) {

	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
//	pCPButton->SetCurrentState(1);
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL || playerAC->Sms == NULL){
		return;
	}

	if (playerAC->Sms->DidEmergencyJettison()){
		pCPButton->SetCurrentState(1);
	}
	else {
		pCPButton->SetCurrentState(0);
	}
}



void CBEAVTRControl(void *, int) {

	SimAVTRToggle(0, KEY_DOWN, NULL);
}

void CBExAVTRControl(void * pButton, int) {

	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;

	if(SimDriver.AVTROn() == TRUE) {
		pCPButton->SetCurrentState(1);
	}
	else {
		pCPButton->SetCurrentState(0);
	}
}


void CBExModeSelect(void * pButton, int event) {

	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	
	switch (gNavigationSys->GetInstrumentMode()) {
	case NavigationSystem::NAV:
	    pCPButton->SetCurrentState(0);
	break;

	case NavigationSystem::ILS_NAV:
	    pCPButton->SetCurrentState(1);
	break;

	case NavigationSystem::ILS_TACAN:
	    pCPButton->SetCurrentState(2);
	break;

	case NavigationSystem::TACAN:
	    pCPButton->SetCurrentState(3);
	break;

	default:
		pCPButton->SetCurrentState(0);
	break;
	}
}

void CBEModeSelect(void *, int event) {
	if (event == CP_MOUSE_BUTTON0)
	    SimStepHSIMode(0, KEY_DOWN, NULL);
	else {
	    SimStepHSIMode(0, KEY_DOWN, NULL); // JPO - surely one of the grossest hacks you ever did see
	    SimStepHSIMode(0, KEY_DOWN, NULL);
	    SimStepHSIMode(0, KEY_DOWN, NULL);
	}
}

// =============================================//
// Callback Function CBEMPO, Manual Pitch Override
// =============================================//
void CBExMPO(void * pButton, int) {
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	if(playerAC->af->IsSet(AirframeClass::MPOverride)){
		pCPButton->SetCurrentState(1);
	}
	else {
		pCPButton->SetCurrentState(0);
	}
}

void CBEMPO(void *, int) {

	SimMPOToggle(0, KEY_DOWN, NULL);
}

void CBExHornSilencer(void * pButton, int event) {
	
	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;

	pCPButton->SetCurrentState(event == CP_CHECK_EVENT ? 0 : 1); // JPO
}

void CBEHornSilencer(void *, int) {

	SimSilenceHorn(0, KEY_DOWN, NULL);
}

void CBExHUDColor (void* pButton, int event) {
	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	pCPButton->SetCurrentState(event == CP_CHECK_EVENT ? 0 : 1); // JPO
}

void CBExMFDButton(void * pButton, int event) {
	
	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	//pCPButton->SetCurrentState(1);
	pCPButton->SetCurrentState(event == CP_CHECK_EVENT ? 0 : 1); // JPO
}

void CBEOSB_1L(void *, int) {

	SimCBEOSB_1L(0, KEY_DOWN, NULL);
}

void CBEOSB_2L(void *, int) {

	SimCBEOSB_2L(0, KEY_DOWN, NULL);
}

void CBEOSB_3L(void *, int) {

	SimCBEOSB_3L(0, KEY_DOWN, NULL);
}

void CBEOSB_4L(void *, int) {

	SimCBEOSB_4L(0, KEY_DOWN, NULL);
}

void CBEOSB_5L(void *, int) {

	SimCBEOSB_5L(0, KEY_DOWN, NULL);
}

void CBEOSB_6L(void *, int) {

	SimCBEOSB_6L(0, KEY_DOWN, NULL);
}

void CBEOSB_7L(void *, int) {

	SimCBEOSB_7L(0, KEY_DOWN, NULL);
}

void CBEOSB_8L(void *, int) {

	SimCBEOSB_8L(0, KEY_DOWN, NULL);
}

void CBEOSB_9L(void *, int) {

	SimCBEOSB_9L(0, KEY_DOWN, NULL);
}

void CBEOSB_10L(void *, int) {

	SimCBEOSB_10L(0, KEY_DOWN, NULL);
}

void CBEOSB_11L(void *, int) {

	SimCBEOSB_11L(0, KEY_DOWN, NULL);
}

void CBEOSB_12L(void *, int) {

	SimCBEOSB_12L(0, KEY_DOWN, NULL);
}

void CBEOSB_13L(void *, int) {

	SimCBEOSB_13L(0, KEY_DOWN, NULL);
}

void CBEOSB_14L(void *, int) {

	SimCBEOSB_14L(0, KEY_DOWN, NULL);
}

void CBEOSB_15L(void *, int) {

	SimCBEOSB_15L(0, KEY_DOWN, NULL);
}

void CBEOSB_16L(void *, int) {

	SimCBEOSB_16L(0, KEY_DOWN, NULL);
}

void CBEOSB_17L(void *, int) {

	SimCBEOSB_17L(0, KEY_DOWN, NULL);
}

void CBEOSB_18L(void *, int) {

	SimCBEOSB_18L(0, KEY_DOWN, NULL);
}

void CBEOSB_19L(void *, int) {

	SimCBEOSB_19L(0, KEY_DOWN, NULL);
}

void CBEOSB_20L(void *, int) {

	SimCBEOSB_20L(0, KEY_DOWN, NULL);
}


void CBEOSB_1R(void *, int) {

	SimCBEOSB_1R(0, KEY_DOWN, NULL);
}

void CBEOSB_2R(void *, int) {

	SimCBEOSB_2R(0, KEY_DOWN, NULL);
}

void CBEOSB_3R(void *, int) {

	SimCBEOSB_3R(0, KEY_DOWN, NULL);
}

void CBEOSB_4R(void *, int) {

	SimCBEOSB_4R(0, KEY_DOWN, NULL);
}

void CBEOSB_5R(void *, int) {

	SimCBEOSB_5R(0, KEY_DOWN, NULL);
}

void CBEOSB_6R(void *, int) {

	SimCBEOSB_6R(0, KEY_DOWN, NULL);
}

void CBEOSB_7R(void *, int) {

	SimCBEOSB_7R(0, KEY_DOWN, NULL);
}

void CBEOSB_8R(void *, int) {

	SimCBEOSB_8R(0, KEY_DOWN, NULL);
}

void CBEOSB_9R(void *, int) {

	SimCBEOSB_9R(0, KEY_DOWN, NULL);
}

void CBEOSB_10R(void *, int) {

	SimCBEOSB_10R(0, KEY_DOWN, NULL);
}

void CBEOSB_11R(void *, int) {

	SimCBEOSB_11R(0, KEY_DOWN, NULL);
}

void CBEOSB_12R(void *, int) {

	SimCBEOSB_12R(0, KEY_DOWN, NULL);
}

void CBEOSB_13R(void *, int) {

	SimCBEOSB_13R(0, KEY_DOWN, NULL);
}

void CBEOSB_14R(void *, int) {

	SimCBEOSB_14R(0, KEY_DOWN, NULL);
}

void CBEOSB_15R(void *, int) {

	SimCBEOSB_15R(0, KEY_DOWN, NULL);
}

void CBEOSB_16R(void *, int) {

	SimCBEOSB_16R(0, KEY_DOWN, NULL);
}

void CBEOSB_17R(void *, int) {

	SimCBEOSB_17R(0, KEY_DOWN, NULL);
}

void CBEOSB_18R(void *, int) {

	SimCBEOSB_18R(0, KEY_DOWN, NULL);
}

void CBEOSB_19R(void *, int) {

	SimCBEOSB_19R(0, KEY_DOWN, NULL);
}

void CBEOSB_20R(void *, int) {

	SimCBEOSB_20R(0, KEY_DOWN, NULL);
}


//Wombat778 4-12-04  Callback Functions for new MFDs
void CBEOSB_1T(void *, int) {

	SimCBEOSB_1T(0, KEY_DOWN, NULL);
}

void CBEOSB_2T(void *, int) {

	SimCBEOSB_2T(0, KEY_DOWN, NULL);
}

void CBEOSB_3T(void *, int) {

	SimCBEOSB_3T(0, KEY_DOWN, NULL);
}

void CBEOSB_4T(void *, int) {

	SimCBEOSB_4T(0, KEY_DOWN, NULL);
}

void CBEOSB_5T(void *, int) {

	SimCBEOSB_5T(0, KEY_DOWN, NULL);
}

void CBEOSB_6T(void *, int) {

	SimCBEOSB_6T(0, KEY_DOWN, NULL);
}

void CBEOSB_7T(void *, int) {

	SimCBEOSB_7T(0, KEY_DOWN, NULL);
}

void CBEOSB_8T(void *, int) {

	SimCBEOSB_8T(0, KEY_DOWN, NULL);
}

void CBEOSB_9T(void *, int) {

	SimCBEOSB_9T(0, KEY_DOWN, NULL);
}

void CBEOSB_10T(void *, int) {

	SimCBEOSB_10T(0, KEY_DOWN, NULL);
}

void CBEOSB_11T(void *, int) {

	SimCBEOSB_11T(0, KEY_DOWN, NULL);
}

void CBEOSB_12T(void *, int) {

	SimCBEOSB_12T(0, KEY_DOWN, NULL);
}

void CBEOSB_13T(void *, int) {

	SimCBEOSB_13T(0, KEY_DOWN, NULL);
}

void CBEOSB_14T(void *, int) {

	SimCBEOSB_14T(0, KEY_DOWN, NULL);
}

void CBEOSB_15T(void *, int) {

	SimCBEOSB_15T(0, KEY_DOWN, NULL);
}

void CBEOSB_16T(void *, int) {

	SimCBEOSB_16T(0, KEY_DOWN, NULL);
}

void CBEOSB_17T(void *, int) {

	SimCBEOSB_17T(0, KEY_DOWN, NULL);
}

void CBEOSB_18T(void *, int) {

	SimCBEOSB_18T(0, KEY_DOWN, NULL);
}

void CBEOSB_19T(void *, int) {

	SimCBEOSB_19T(0, KEY_DOWN, NULL);
}

void CBEOSB_20T(void *, int) {

	SimCBEOSB_20T(0, KEY_DOWN, NULL);
}


void CBEOSB_1F(void *, int) {

	SimCBEOSB_1F(0, KEY_DOWN, NULL);
}

void CBEOSB_2F(void *, int) {

	SimCBEOSB_2F(0, KEY_DOWN, NULL);
}

void CBEOSB_3F(void *, int) {

	SimCBEOSB_3F(0, KEY_DOWN, NULL);
}

void CBEOSB_4F(void *, int) {

	SimCBEOSB_4F(0, KEY_DOWN, NULL);
}

void CBEOSB_5F(void *, int) {

	SimCBEOSB_5F(0, KEY_DOWN, NULL);
}

void CBEOSB_6F(void *, int) {

	SimCBEOSB_6F(0, KEY_DOWN, NULL);
}

void CBEOSB_7F(void *, int) {

	SimCBEOSB_7F(0, KEY_DOWN, NULL);
}

void CBEOSB_8F(void *, int) {

	SimCBEOSB_8F(0, KEY_DOWN, NULL);
}

void CBEOSB_9F(void *, int) {

	SimCBEOSB_9F(0, KEY_DOWN, NULL);
}

void CBEOSB_10F(void *, int) {

	SimCBEOSB_10F(0, KEY_DOWN, NULL);
}

void CBEOSB_11F(void *, int) {

	SimCBEOSB_11F(0, KEY_DOWN, NULL);
}

void CBEOSB_12F(void *, int) {

	SimCBEOSB_12F(0, KEY_DOWN, NULL);
}

void CBEOSB_13F(void *, int) {

	SimCBEOSB_13F(0, KEY_DOWN, NULL);
}

void CBEOSB_14F(void *, int) {

	SimCBEOSB_14F(0, KEY_DOWN, NULL);
}

void CBEOSB_15F(void *, int) {

	SimCBEOSB_15F(0, KEY_DOWN, NULL);
}

void CBEOSB_16F(void *, int) {

	SimCBEOSB_16F(0, KEY_DOWN, NULL);
}

void CBEOSB_17F(void *, int) {

	SimCBEOSB_17F(0, KEY_DOWN, NULL);
}

void CBEOSB_18F(void *, int) {

	SimCBEOSB_18F(0, KEY_DOWN, NULL);
}

void CBEOSB_19F(void *, int) {

	SimCBEOSB_19F(0, KEY_DOWN, NULL);
}

void CBEOSB_20F(void *, int) {

	SimCBEOSB_20F(0, KEY_DOWN, NULL);
}

void CBEThreeGainUp(void *, int) {
	SimCBEOSB_GAINUP_T (0, KEY_DOWN, NULL);
}

void CBEThreeGainDown(void *, int) {
	SimCBEOSB_GAINDOWN_T (0, KEY_DOWN, NULL);
}

void CBEFourGainDown(void *, int) {
	SimCBEOSB_GAINDOWN_F (0, KEY_DOWN, NULL);
}

void CBEFourGainUp(void *, int) {
	SimCBEOSB_GAINUP_F (0, KEY_DOWN, NULL);
}


//Wombat778 End


// =============================================//
// Callback Function CBEICPTILS
// =============================================//

void CBEICPTILS(void *pButton, int) {

	SimICPTILS(0, KEY_DOWN, pButton);
}

// =============================================//
// Callback Function CBEICPALOW
// =============================================//

void CBEICPALOW(void *pButton, int) {

	SimICPALOW(0, KEY_DOWN, pButton);
}



// =============================================//
// Callback Function CBEICPFAck
// =============================================//

void CBEICPFAck(void *pButton, int) {

	SimICPFAck(0, KEY_DOWN, pButton);
}

// =============================================//
// Callback Function CBEICPPrevious
// =============================================//

void CBEICPPrevious(void *pButton, int) {

	SimICPPrevious(0, KEY_DOWN, pButton);
}

void CBExICPPrevious(void * pButton, int event) {
	
	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	//pCPButton->SetCurrentState(1);
	pCPButton->SetCurrentState(event == CP_CHECK_EVENT ? 0 : 1); // JPO

}


// =============================================//
// Callback Function CBEICPNext
// =============================================//

void CBEICPNext(void * pButton, int) {

	SimICPNext(0, KEY_DOWN, pButton);
}

void CBExICPNext(void * pButton, int event) {
	
	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	//pCPButton->SetCurrentState(1);
	pCPButton->SetCurrentState(event == CP_CHECK_EVENT ? 0 : 1); // JPO

}
// =============================================//
// Callback Function CBEICPLink
// =============================================//

void CBEICPLink(void * pButton, int) {

	SimICPLink(0, KEY_DOWN, pButton);
}

// =============================================//
// Callback Function CBEICPCrus
// =============================================//

void CBEICPCrus(void * pButton, int) {

	SimICPCrus(0, KEY_DOWN, pButton);
}

// =============================================//
// Callback Function CBEICPStpt
// =============================================//

void CBEICPStpt(void * pButton, int) {

	SimICPStpt(0, KEY_DOWN, pButton);
}

// =============================================//
// Callback Function CBEICPMark
// =============================================//

void CBEICPMark(void * pButton, int) {

	SimICPMark(0, KEY_DOWN, pButton);
}

// =============================================//
// Callback Function CBEICPEnter
// =============================================//

void CBEICPEnter(void * pButton, int) {

	SimICPEnter(0, KEY_DOWN, pButton);
}

void CBExICPEnter(void * pButton, int event) {
	
	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	//pCPButton->SetCurrentState(1);
	pCPButton->SetCurrentState(event == CP_CHECK_EVENT ? 0 : 1); // JPO

}

// =============================================//
// Callback Function CBEICPCom1
// =============================================//

void CBEICPCom1(void * pButton, int) {
	
	SimICPCom1(0, KEY_DOWN, pButton);
}


// =============================================//
// Callback Function CBEICPCom2
// =============================================//

void CBEICPCom2(void * pButton, int) {
	
	SimICPCom2(0, KEY_DOWN, pButton);
}

// =============================================//
// Callback Function CBEICPNav
// =============================================//

void CBEICPNav(void * pButton, int) {
	
	SimICPNav(0, KEY_DOWN, pButton);
}

// =============================================//
// Callback Function CBEICPAA
// =============================================//

void CBEICPAA(void * pButton, int) {
	
	SimICPAA(0, KEY_DOWN, pButton);
}

// =============================================//
// Callback Function CBEICPAG
// =============================================//

void CBEICPAG(void * pButton, int) {
	
	SimICPAG(0, KEY_DOWN, pButton);
}


////
void CBExICPPrimaryExclusive(void * pButton, int event) {

	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;

	if(!g_bRealisticAvionics)
	{
		//MI original code
		if(OTWDriver.pCockpitManager->mpIcp->GetPrimaryExclusiveButton() == pButton)
		{
			pCPButton->SetCurrentState(1);
		}
		else 
		{
			pCPButton->SetCurrentState(0);
		}
	}
	else
	{
		//MI modified code
		//pCPButton->SetCurrentState(1);
	    pCPButton->SetCurrentState(event == CP_CHECK_EVENT ? 0 : 1); // JPO
	}
}

void CBExICPSecondaryExclusive(void * pButton, int event) {
	
	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;

	if(!g_bRealisticAvionics)
	{
		//MI original code
		if(OTWDriver.pCockpitManager->mpIcp->GetSecondaryExclusiveButton() == pButton){
			pCPButton->SetCurrentState(1);
		}
		else {
			pCPButton->SetCurrentState(0);
		}
	}
	else
	{
		//MI modified code
		//pCPButton->SetCurrentState(1);
	    pCPButton->SetCurrentState(event == CP_CHECK_EVENT ? 0 : 1); // JPO
	}
}


void CBExICPTertiaryExclusive(void * pButton, int event) {

	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;

	if(!g_bRealisticAvionics)
	{
		//MI original code
		if(OTWDriver.pCockpitManager->mpIcp->GetTertiaryExclusiveButton() == pButton){
			pCPButton->SetCurrentState(1);
		}
		else {
			pCPButton->SetCurrentState(0);
		}
	}
	else
	{
		//MI modified code
		//pCPButton->SetCurrentState(0);
	    pCPButton->SetCurrentState(event == CP_CHECK_EVENT ? 0 : 1); // JPO
	}
}

// =============================================//
// Callback Function 
// =============================================//
void CBExHUDScales(void * pButton, int) {
	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;

	switch (TheHud->GetScalesSwitch()) {
	case HudClass::VAH:
		pCPButton->SetCurrentState(0);
	break;

	case HudClass::VV_VAH:
		pCPButton->SetCurrentState(1);
	break;

	case HudClass::SS_OFF:
		pCPButton->SetCurrentState(2);
	break;

	default:
		pCPButton->SetCurrentState(0);
	break;
	}	
}

void CBEHUDScales(void *, int) {

	SimHUDScales(0, KEY_DOWN, NULL);
}

// =============================================//
// Callback Function 
// =============================================//

void CBExHUDFPM(void * pButton, int) {
	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;

	switch (TheHud->GetFPMSwitch()) {
	case HudClass::FPM:
		pCPButton->SetCurrentState(0);
	break;

	case HudClass::ATT_FPM:
		pCPButton->SetCurrentState(1);
	break;

	case HudClass::FPM_OFF:
		pCPButton->SetCurrentState(2);
	break;

	default:
		pCPButton->SetCurrentState(0);
	break;
	}	
}

void CBEHUDFPM(void *, int) {

	SimHUDFPM(0, KEY_DOWN, NULL);
}

// =============================================//
// Callback Function 
// =============================================//

void CBExHUDDED(void * pButton, int) {
	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;

	switch (TheHud->GetDEDSwitch()) {
	case HudClass::DED_OFF:
		pCPButton->SetCurrentState(0);
	break;

	case HudClass::DED_DATA:
		pCPButton->SetCurrentState(1);
	break;

	case HudClass::PFL_DATA:
		pCPButton->SetCurrentState(2);
	break;

	default:
		pCPButton->SetCurrentState(0);
	break;
	}	
}

void CBEHUDDED(void *, int) {

	SimHUDDED(0, KEY_DOWN, NULL);
}

// 2000-11-10 FUNCTIONS ADDED BY S.G. FOR THE Drift C/O switch

// =============================================//
// Callback Function 
// =============================================//

void CBExHUDDriftCO(void * pButton, int) {
	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;

	switch (TheHud->GetDriftCOSwitch()) {
	case HudClass::DRIFT_CO_OFF:
		pCPButton->SetCurrentState(0);
	break;

	case HudClass::DRIFT_CO_ON:
		pCPButton->SetCurrentState(1);
	break;

	default:
		pCPButton->SetCurrentState(0);
	break;
	}	
}

void CBEHUDDriftCO(void *, int) {

	SimDriftCO(0, KEY_DOWN, NULL);
}

// END OF ADDED SECTION

// 2000-11-17 FUNCTIONS ADDED BY S.G. FOR THE Cat I/III switch

// =============================================//
// Callback Function 
// =============================================//

void CBExHUDVelocity(void * pButton, int) {
	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;

	switch (TheHud->GetVelocitySwitch()) {
	case HudClass::CAS:
		pCPButton->SetCurrentState(0);
	break;

	case HudClass::TAS:
		pCPButton->SetCurrentState(1);
	break;

	case HudClass::GND_SPD:
		pCPButton->SetCurrentState(2);
	break;

	default:
		pCPButton->SetCurrentState(0);
	break;
	}
}

void CBEHUDVelocity(void *, int) {

	SimHUDVelocity(0, KEY_DOWN, NULL);
}

// =============================================//
// Callback Function 
// =============================================//

void CBExHUDRadar(void * pButton, int) {
	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;

	switch (TheHud->GetRadarSwitch()) {
	case HudClass::ALT_RADAR:
		pCPButton->SetCurrentState(0);
	break;

	case HudClass::BARO:
		pCPButton->SetCurrentState(1);
	break;

	case HudClass::RADAR_AUTO:
		pCPButton->SetCurrentState(2);
	break;

	default:
		pCPButton->SetCurrentState(0);
	break;
	}
}

void CBEHUDRadar(void *, int) {

	SimHUDRadar(0, KEY_DOWN, NULL);
}

// =============================================//
// Callback Function 
// =============================================//

void CBExHUDBrightness(void * pButton, int) {
	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;

	switch (TheHud->GetBrightnessSwitch()) 
	{
	case HudClass::DAY:
		pCPButton->SetCurrentState(1);
	break;

	case HudClass::BRIGHT_AUTO:
		pCPButton->SetCurrentState(3);
	break;

	case HudClass::NIGHT:
		pCPButton->SetCurrentState(2);
	break;

	default:
		pCPButton->SetCurrentState(1);
	break;
	}
}

void CBEHUDBrightness(void *, int event) 
{
	SimHUDBrightness(0, KEY_DOWN, NULL);
}

// =============================================//
// Callback Function 
// =============================================//

void CBExECMSwitch(void * pButton, int) {
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;

	if (playerAC->IsSetFlag(ECM_ON)){
		pCPButton->SetCurrentState(0);
	}
	else {
		pCPButton->SetCurrentState(1);
	}
}

void CBEECMSwitch(void *, int) {

	SimECMOn(0, KEY_DOWN, NULL);
}

// =============================================//
// Callback Function 
// =============================================//

void CBExUHFSwitch(void * pButton, int) {

	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	pCPButton->SetCurrentState(OTWDriver.pCockpitManager->mMiscStates.mUHFPosition);
}

void CBEUHFSwitch(void *, int) {

	if(gNavigationSys->GetUHFSrc() == NavigationSystem::UHF_BACKUP) {
		SimCycleRadioChannel(0, KEY_DOWN, NULL);
	}
	else {
		OTWDriver.pCockpitManager->mMiscStates.StepUHFPostion();
	}
}

// =============================================//
// Callback Function 
// =============================================//

void CBExAuxCommLeft(void * pButton, int) {
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}
	
	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	pCPButton->SetCurrentState(gNavigationSys->GetTacanChannel(NavigationSystem::AUXCOMM, 2));
}

void CBEAuxCommLeft(void *, int event) {
	if(event == CP_MOUSE_BUTTON0){
		// JPO allow right key decrement
		SimCycleLeftAuxComDigit(0, KEY_DOWN, NULL);
	}
	else if(event == CP_MOUSE_BUTTON1){
		SimDecLeftAuxComDigit(0, KEY_DOWN, NULL);
	}
}

void CBExAuxCommCenter(void * pButton, int) {
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}
	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	pCPButton->SetCurrentState(gNavigationSys->GetTacanChannel(NavigationSystem::AUXCOMM, 1));
}

void CBEAuxCommCenter(void *, int event) {
	if(event == CP_MOUSE_BUTTON0){
		// allow right key decrement
	    SimCycleCenterAuxComDigit(0, KEY_DOWN, NULL);
	}
	else if(event == CP_MOUSE_BUTTON1){
	    SimDecCenterAuxComDigit(0, KEY_DOWN, NULL);
	}
}

void CBExAuxCommRight(void * pButton, int) {
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}
	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	pCPButton->SetCurrentState(gNavigationSys->GetTacanChannel(NavigationSystem::AUXCOMM, 0));
}

void CBEAuxCommRight(void *, int event) {
	if(event == CP_MOUSE_BUTTON0){ 
		// allow right key decrement
		SimCycleRightAuxComDigit(0, KEY_DOWN, NULL);
	}
	else if (event == CP_MOUSE_BUTTON1){
		SimDecRightAuxComDigit(0, KEY_DOWN, NULL);
	}
}

void CBExAuxCommBand(void * pButton, int) {
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}
	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	if(gNavigationSys->GetTacanBand(NavigationSystem::AUXCOMM) == TacanList::X) {
		pCPButton->SetCurrentState(0);
	}
	else {
		pCPButton->SetCurrentState(1);
	}
}

void CBEAuxCommBand(void *, int) {

	SimCycleBandAuxComDigit(0, KEY_DOWN, NULL);
}

// =============================================//
// Callback Function 
// =============================================//
void CBExEject(void * pButton, int) {

	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	pCPButton->SetCurrentState(OTWDriver.pCockpitManager->mMiscStates.GetEjectButtonState());
}

void CBEEject(void *pButton, int) {
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	//MI
	if(g_bRealisticAvionics){
		if (playerAC->SeatArmed){
			OTWDriver.pCockpitManager->mMiscStates.SetEjectButtonState(TRUE);
			SimEject(0, KEY_DOWN, pButton);
		}
	}
	else {
		OTWDriver.pCockpitManager->mMiscStates.SetEjectButtonState(TRUE);
		SimEject(0, KEY_DOWN, pButton);
	}
}

// =============================================//
// Callback Function 
// =============================================//

void CBExAuxCommMaster(void * pButton, int) {
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	int	state = 0;

	if(gNavigationSys->GetControlSrc() == NavigationSystem::AUXCOMM) {
		state = 1;
	}
	pCPButton->SetCurrentState(state);
}

void CBEAuxCommMaster(void *, int) {
	SimToggleAuxComMaster(0, KEY_DOWN, NULL);
}

// =============================================//
// Callback Function 
// =============================================//

void CBExAuxCommAATR(void * pButton, int) {
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}
	
	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;

	if(gNavigationSys->GetDomain(NavigationSystem::AUXCOMM) == TacanList::AA) {
		pCPButton->SetCurrentState(0);
	}
	else {
		pCPButton->SetCurrentState(1);
	}
}

void CBEAuxCommAATR(void *, int) {
	SimToggleAuxComAATR(0, KEY_DOWN, NULL);
}

// =============================================//
// Callback Function 
// =============================================//

// MD -- 20031122: change this to just do nothing for now when clicked
// since the CNI switch now does what this one used to do match the real
// function.  If the UHF function knob is modeled properly at some point,
// this will likely need to be changed again.

void CBExUHFMaster(void * pButton, int) {

#if 0
	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;

	if (playerAC) {
		if(gNavigationSys->GetUHFSrc() == NavigationSystem::UHF_NORM) {
			pCPButton->SetCurrentState(0);
		}
		else {
			pCPButton->SetCurrentState(1);
		}
	}
#endif
}

void CBEUHFMaster(void *, int) {
	SimToggleUHFMaster(0, KEY_DOWN, NULL);
}


// =============================================//
// Callback Function 
// =============================================//

void CBExExteriorLite(void * pButton, int) {

	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

	if (playerAC == NULL) {
		return;
	}

	if (playerAC->af->IsSet(AirframeClass::HasComplexGear) && playerAC->GetSwitch(COMP_NAV_LIGHTS)) {
		pCPButton->SetCurrentState(1);
	}
	else {
		pCPButton->SetCurrentState(0);
	}
}

void CBEExteriorLite(void *, int) {
	SimToggleExtLights(0, KEY_DOWN, NULL);
}

void CBEMasterArm(void *, int event) {
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL) {
		return;
	}

    if(event == CP_MOUSE_BUTTON0){
		switch (playerAC->Sms->MasterArm()) {
			case SMSBaseClass::Sim:
				playerAC->Sms->SetMasterArm(SMSBaseClass::Safe);
			break;
			case SMSBaseClass::Safe:
				playerAC->Sms->SetMasterArm(SMSBaseClass::Arm);
			break;
		}
    }
	else {
		switch (playerAC->Sms->MasterArm()) {
			case SMSBaseClass::Arm:
				playerAC->Sms->SetMasterArm(SMSBaseClass::Safe);
			break;
			case SMSBaseClass::Safe:
				playerAC->Sms->SetMasterArm(SMSBaseClass::Sim);
			break;
		}
    }
}

// OW CAT III cockpit switch extension
void CBECatIII(void *, int) {
//   OTWTimeOfDayStep (0, KEY_DOWN, NULL);
// 2000-11-23 S.G. NOW USING ITS OWN ENTRY
	SimCATSwitch(0, KEY_DOWN, NULL);
}

void CBERightGainUp(void *, int) {
	SimCBEOSB_GAINUP_R (0, KEY_DOWN, NULL);
}

void CBERightGainDown(void *, int) {
	SimCBEOSB_GAINDOWN_R (0, KEY_DOWN, NULL);
}

void CBELeftGainDown(void *, int) {
	SimCBEOSB_GAINDOWN_L (0, KEY_DOWN, NULL);
}

void CBELeftGainUp(void *, int) {
	SimCBEOSB_GAINUP_L (0, KEY_DOWN, NULL);
}

void CBEHUDColor(void *, int event) {
//	OTWStepHudColor (0, KEY_DOWN, NULL);
	if(event == CP_MOUSE_BUTTON0)
		OTWStepHudContrastUp(0, KEY_DOWN, NULL);
	else
		OTWStepHudContrastDn(0, KEY_DOWN, NULL);	
}

void CBExMasterArm(void * pButton, int)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL) {
		return;
	}

	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;

	// sfr: TODO this MUST go away!!!!
	if (
		!F4IsBadReadPtr(playerAC, sizeof(AircraftClass*)) && 
		!F4IsBadReadPtr(playerAC->Sms, sizeof(SMSClass))
	){ // JB 010326 CTD
		switch (playerAC->Sms->MasterArm())
		{
			case SMSBaseClass::Safe:
   			pCPButton->SetCurrentState(2);
			break;
			case SMSBaseClass::Sim:
   			pCPButton->SetCurrentState(1);
			break;
			case SMSBaseClass::Arm:
   			pCPButton->SetCurrentState(0);
			break;
		}
	}
}

// OW CAT III cockpit switch extension
void CBExCatIII(void * pButton, int)
{
	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if ((playerAC == NULL) || (playerAC->af == NULL)){
		return;
	}
	pCPButton->SetCurrentState(playerAC->af->IsSet(AirframeClass::CATLimiterIII) ? 1 : 0);
}


// JPO - new callbacks for jfs and epu switches
void CBEJfs(void *, int) {
    SimJfsStart(0, KEY_DOWN, NULL);
}

void CBExJfs(void * pButton, int)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
    
	if (playerAC->af->IsSet(AirframeClass::JfsStart)) {
	    pCPButton->SetCurrentState(1);
	}
	else {
	    pCPButton->SetCurrentState(0);
	}
}

void CBEEpu(void *, int) {
    SimEpuToggle(0, KEY_DOWN, NULL);
}

void CBExEpu(void * pButton, int)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
    
	switch (playerAC->af->GetEpuSwitch()) {
		case AirframeClass::EpuState::OFF:
			pCPButton->SetCurrentState(0);
		break;
		case AirframeClass::EpuState::AUTO:
			pCPButton->SetCurrentState(1);
		break;
		case AirframeClass::EpuState::ON:
   			pCPButton->SetCurrentState(2);
		break;
	}
}

void CBEAltLGear(void *, int) {
    AFAlternateGear(0, KEY_DOWN, NULL);
}

void CBEAltLGearReset(void *, int) {
    AFAlternateGearReset(0, KEY_DOWN, NULL);
}

void CBExAltLGear(void * pButton, int)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}
    CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	pCPButton->SetCurrentState(playerAC->af->altGearDeployed ? 1 : 0);
}

//MI ICP Stuff
// =============================================//
// Callback Function CBEICPIFF, IFF Override button
// =============================================//
void CBEICPIFF(void * pButton, int) 
{
	SimICPIFF(0, KEY_DOWN, pButton);
}

// =============================================//
// Callback Function CBEICPLIST, LIST Override button
// =============================================//
void CBEICPLIST(void * pButton, int) 
{
	SimICPLIST(0, KEY_DOWN, pButton);
}

// =============================================//
// Callback Function CBEThreeButton, ICP 3
// =============================================//
void CBETHREEButton(void * pButton, int) 
{
	SimICPTHREE(0, KEY_DOWN, pButton);
}
// =============================================//
// Callback Function CBESixButton, ICP 6
// =============================================//
void CBESIXButton(void * pButton, int) 
{
	SimICPSIX(0, KEY_DOWN, pButton);
}
// =============================================//
// Callback Function CBEEightButton, ICP 8
// =============================================//
void CBEEIGHTButton(void * pButton, int) 
{
	SimICPEIGHT(0, KEY_DOWN, pButton);
}
// =============================================//
// Callback Function CBENineButton, ICP 9
// =============================================//
void CBENINEButton(void * pButton, int) 
{
	SimICPNINE(0, KEY_DOWN, pButton);
}
// =============================================//
// Callback Function CBEZeroButton, ICP 0
// =============================================//
void CBEZEROButton(void * pButton, int) 
{
	SimICPZERO(0, KEY_DOWN, pButton);
}
// =============================================//
// Callback Function CBEResetDEDPage, ICP 0
// =============================================//
void CBEResetDEDPage(void * pButton, int) 
{
	SimICPResetDED(0, KEY_DOWN, pButton);
}
// =============================================//
// Callback Function CBEDEDUP, ICP UP
// =============================================//
void CBEICPDEDUP(void * pButton, int) 
{
	SimICPDEDUP(0, KEY_DOWN, pButton);
}
// =============================================//
// Callback Function CBEDEDDOWN, ICP DOWN
// =============================================//
void CBEICPDEDDOWN(void * pButton, int) 
{
	SimICPDEDDOWN(0, KEY_DOWN, pButton);
}
// =============================================//
// Callback Function CBEDEDSeq, ICP SEQUENCE
// =============================================//
void CBEICPDEDSEQ(void * pButton, int) 
{
	SimICPDEDSEQ(0, KEY_DOWN, pButton);
}
// =============================================//
// Callback Function CBEICPCLEAR, ICP DOWN
// =============================================//
void CBEICPCLEAR(void * pButton, int) 
{
	SimICPCLEAR(0, KEY_DOWN, pButton);
}

// =============================================//
// Callback Function CBExRALT, RALT
// =============================================//
void CBExRALTSwitch(void * pButton, int)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	//MI reported CTD check
	switch (playerAC->RALTStatus) 
	{
		case AircraftClass::RaltStatus::ROFF:
			pCPButton->SetCurrentState(0);
		break;
		case AircraftClass::RaltStatus::RSTANDBY:
			pCPButton->SetCurrentState(1);
		break;
		case AircraftClass::RaltStatus::RON:
			pCPButton->SetCurrentState(2);
		break;
		default:
			ShiWarning("Inconsistant ralt state");
		break;
	}
}	

// =============================================//
// Callback Function CBERALTSTDBY, RALT Standby
// =============================================//
void CBERALTSTDBY(void * pButton, int)
{
	SimRALTSTDBY(0, KEY_DOWN, pButton);
}

// =============================================//
// Callback Function CBERALTON, RALT On
// =============================================//
void CBERALTON(void * pButton, int)
{
	SimRALTON(0, KEY_DOWN, pButton);
}

// =============================================//
// Callback Function CBERALTOFF, RALT Off
// =============================================//
void CBERALTOFF(void * pButton, int)
{
	SimRALTOFF(0, KEY_DOWN, pButton);
}

// =============================================//
// Callback Function CBERALTOFF, RALT Switch
// =============================================//
void CBERALTSwitch(void * pButton, int event)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

    AircraftClass::RaltStatus rs = playerAC->RALTStatus;
    if(event == CP_MOUSE_BUTTON0) 
	{
		if (rs == AircraftClass::RaltStatus::ROFF){
			SimRALTSTDBY(0, KEY_DOWN, pButton);
		}
		else if (rs == AircraftClass::RaltStatus::RSTANDBY){
			SimRALTON(0, KEY_DOWN, pButton);
		}
    }
    else {
		if (rs == AircraftClass::RaltStatus::RON){
			SimRALTSTDBY(0, KEY_DOWN, pButton);
		}
		else if (rs == AircraftClass::RaltStatus::RSTANDBY){
			SimRALTOFF(0, KEY_DOWN, pButton);
		}
    }
}

//=============================================//
// Callback Function CBESmS
//=============================================//

void CBESmsPower (void *pButton, int)
{
    SimSMSPower(0, KEY_DOWN, pButton);
}

void CBExSmsPower(void * pButton, int)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

    CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	if(playerAC->PowerSwitchOn(AircraftClass::SMSPower)) {
	    pCPButton->SetCurrentState(1);
	}
	else {
	    pCPButton->SetCurrentState(0);
	}
}

//=============================================//
// Callback Function CBEFCC
//=============================================//

void CBEFCCPower (void *pButton, int)
{
    SimFCCPower(0, KEY_DOWN, pButton);
}

void CBExFCCPower(void * pButton, int)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

    CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	if (playerAC->PowerSwitchOn(AircraftClass::FCCPower)) {
	    pCPButton->SetCurrentState(1);
	}
	else {
	    pCPButton->SetCurrentState(0);
	}
}


//=============================================//
// Callback Function CBEMFD
//=============================================//

void CBEMFDPower (void *pButton, int)
{
    SimMFDPower(0, KEY_DOWN, pButton);
}

void CBExMFDPower(void * pButton, int)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

    CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	if (playerAC->PowerSwitchOn(AircraftClass::MFDPower)) {
	    pCPButton->SetCurrentState(1);
	}
	else {
	    pCPButton->SetCurrentState(0);
    }
}


//=============================================//
// Callback Function CBEUFC
//=============================================//

void CBEUFCPower (void *pButton, int)
{
    SimUFCPower(0, KEY_DOWN, pButton);
}

void CBExUFCPower(void * pButton, int)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

    CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	if(playerAC->PowerSwitchOn(AircraftClass::UFCPower)) {
	    pCPButton->SetCurrentState(1);
	}
	else {
	    pCPButton->SetCurrentState(0);
    }
}


//=============================================//
// Callback Function CBEGPS
//=============================================//

void CBEGPSPower (void *pButton, int)
{
    SimGPSPower(0, KEY_DOWN, pButton);
}

void CBExGPSPower(void * pButton, int)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	if(playerAC->PowerSwitchOn(AircraftClass::GPSPower)) {
	    pCPButton->SetCurrentState(1);
	}
	else {
	    pCPButton->SetCurrentState(0);
    }
}


//=============================================//
// Callback Function CBEDL
//=============================================//

void CBEDLPower (void *pButton, int)
{
    SimDLPower(0, KEY_DOWN, pButton);
}

void CBExDLPower(void * pButton, int)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

    CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	if (playerAC->PowerSwitchOn(AircraftClass::DLPower)) {
	    pCPButton->SetCurrentState(1);
	}
	else {
	    pCPButton->SetCurrentState(0);
    }
}


//=============================================//
// Callback Function CBEMAP
//=============================================//

void CBEMAPPower (void *pButton, int)
{
    SimMAPPower(0, KEY_DOWN, pButton);
}

void CBExMAPPower(void * pButton, int)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	if(playerAC->PowerSwitchOn(AircraftClass::MAPPower)) {
	    pCPButton->SetCurrentState(1);
	}
	else {
	    pCPButton->SetCurrentState(0);
    }
}

//=============================================//
// Callback Function Left Hpt Power
//=============================================//

void CBELEFTHPTPower (void *pButton, int)
{
    SimLeftHptPower(0, KEY_DOWN, pButton);
}

void CBExLEFTHPTPower(void * pButton, int)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	if(playerAC->PowerSwitchOn(AircraftClass::LeftHptPower)) {
	    pCPButton->SetCurrentState(1);
	}
	else {
	    pCPButton->SetCurrentState(0);
    }
}

//=============================================//
// Callback Function Right Hpt Power
//=============================================//

void CBERIGHTHPTPower (void *pButton, int)
{
    SimRightHptPower(0, KEY_DOWN, pButton);
}

void CBExRIGHTHPTPower(void * pButton, int)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	if (playerAC->PowerSwitchOn(AircraftClass::RightHptPower)) {
	    pCPButton->SetCurrentState(1);
	}
	else {
	    pCPButton->SetCurrentState(0);
	}
}

//=============================================//
// Callback Function TISL CBETISLPower
//=============================================//

void CBETISLPower (void *pButton, int)
{
    SimTISLPower(0, KEY_DOWN, pButton);
}

void CBExTISLPower(void * pButton, int)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

    CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	if(playerAC->PowerSwitchOn(AircraftClass::TISLPower)) {
	    pCPButton->SetCurrentState(1);
	}
	else {
	    pCPButton->SetCurrentState(0);
	}
}

//=============================================//
// Callback Function TISL CBETISLPower
//=============================================//

void CBEFCRPower (void *pButton, int)
{
    SimFCRPower(0, KEY_DOWN, pButton);
}

void CBExFCRPower(void * pButton, int)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	if(playerAC->PowerSwitchOn(AircraftClass::FCRPower)) {
	    pCPButton->SetCurrentState(1);
	}
	else {
	    pCPButton->SetCurrentState(0);
    }
}
//=============================================//
// Callback Function TISL CBETISLPower
//=============================================//

void CBEHUDPower (void *pButton, int)
{
    SimHUDPower(0, KEY_DOWN, pButton);
}

void CBExHUDPower(void * pButton, int)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	if(playerAC->PowerSwitchOn(AircraftClass::HUDPower)) {
	    pCPButton->SetCurrentState(1);
	}
	else {
	    pCPButton->SetCurrentState(0);
    }
}

//=============================================//
// Callback Function Fuel Gauge Display
//=============================================//
void CBEFuelSwitch(void * pButton, int event)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	if(event == CP_MOUSE_BUTTON0) {
	    playerAC->af->IncFuelSwitch();
	}
	else {
	    playerAC->af->DecFuelSwitch();
	}
}

void CBExFuelDisplay(void * pButton, int)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	pCPButton->SetCurrentState((int)playerAC->af->GetFuelSwitch());
}
//=============================================//
// Callback Function Fuel Valve Display
//=============================================//
void CBEFuelPump(void * pButton, int event)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	if(event == CP_MOUSE_BUTTON0) {
	    playerAC->af->IncFuelPump();
	}
	else {
	    playerAC->af->DecFuelPump();
	}
}

void CBExFuelPump(void * pButton, int)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	pCPButton->SetCurrentState((int)playerAC->af->GetFuelPump());
}

//=============================================//
// Callback Function Fuel Cock Valve
//=============================================//
void CBEFuelCock(void * pButton, int event)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

    playerAC->af->ToggleEngineFlag(AirframeClass::MasterFuelOff);
}

void CBExFuelCock(void * pButton, int)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	if(playerAC->af->IsEngineFlag(AirframeClass::MasterFuelOff)) {
	    pCPButton->SetCurrentState(1);
	}
	else {
	    pCPButton->SetCurrentState(0);
    }
}

//=============================================//
// Callback Function Fuel External Transfer Switch
//=============================================//
void CBEFuelExtTrans(void * pButton, int event)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}
    playerAC->af->ToggleEngineFlag(AirframeClass::WingFirst);
}

void CBExFuelExtTrans(void * pButton, int)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

    CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	if(playerAC->af->IsEngineFlag(AirframeClass::WingFirst)) {
	    pCPButton->SetCurrentState(1);
	}
	else {
	    pCPButton->SetCurrentState(0);
    }
}
//=============================================//
// Callback Function Air Source Display
//=============================================//
void CBEAirSource(void * pButton, int event)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	if(event == CP_MOUSE_BUTTON0) {
	    playerAC->af->IncAirSource();
	}
	else {
	    playerAC->af->DecAirSource();
	}
}

void CBExAirSource(void * pButton, int)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	pCPButton->SetCurrentState((int)playerAC->af->GetAirSource());
}
//=============================================//
// Callback Function LandingLight switch
//=============================================//
void CBELandingLightToggle(void * pButton, int event)
{
    SimLandingLightToggle(0, KEY_DOWN, pButton);
}

void CBExLandingLightToggle(void * pButton, int)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	if (playerAC->IsAcStatusBitsSet(AircraftClass::ACSTATUS_EXT_LANDINGLIGHT)) {
		pCPButton->SetCurrentState(1);
	}
	else {
		pCPButton->SetCurrentState(0);
	}
}
//=============================================//
// Callback Function Parkingbrake
//=============================================//
void CBEParkingBrakeToggle(void * pButton, int event)
{
    SimParkingBrakeToggle(0, KEY_DOWN, pButton);
}

void CBExParkingBrakeToggle(void * pButton, int)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

    CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	if(playerAC->af->PBON) {
		pCPButton->SetCurrentState(1);
	}
	else {
		pCPButton->SetCurrentState(0);
	}
}

// JB carrier start
//=============================================//
// Callback Function Hook
//=============================================//
void CBEHookToggle(void * pButton, int event)
{
    SimHookToggle(0, KEY_DOWN, pButton);
}

void CBExHookToggle(void * pButton, int)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	//MI if the hook is not up, we get the caution
	// MD -- 20031006: true but that should be moved to ToggleHook() in Airframe.cpp
	// to ensure that the fault is set regardless of whether we are looking at the cockpit
	//FackClass*		faultSys;
	//faultSys	= playerAC->mFaults;

	if(playerAC->af->IsSet(AirframeClass::Hook)){
		pCPButton->SetCurrentState(1);
		//MI
		//if(faultSys && playerAC->af->platform->IsF16())
		//	faultSys->SetCaution(hook_fault);
	}
	else {
		pCPButton->SetCurrentState(0);
		//MI
		//if(faultSys && faultSys->GetFault(hook_fault) && playerAC->af->platform->IsF16())
		//	faultSys->ClearFault(hook_fault);
	}
}
// JB carrier end

//=============================================//
// Callback Function LaserArm
//=============================================//
void CBELaserArmToggle(void * pButton, int event)
{
    SimLaserArmToggle(0, KEY_DOWN, pButton);
}

void CBExLaserArmToggle(void * pButton, int)
{
    CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	if(playerAC->FCC->LaserArm) {
		pCPButton->SetCurrentState(1);
	}
	else{
		pCPButton->SetCurrentState(0);
	}
}
//=============================================//
// Callback Function FuelDoor
//=============================================//
void CBEFuelDoorToggle(void * pButton, int event)
{
    SimFuelDoorToggle(0, KEY_DOWN, pButton);
}

void CBExFuelDoorToggle(void * pButton, int)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

    CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	if(playerAC->af->IsEngineFlag(AirframeClass::FuelDoorOpen)) {
	    pCPButton->SetCurrentState(1);
	}
	else {
	    pCPButton->SetCurrentState(0);
    }
}
//=============================================//
// Callback Function RightAPSwitch
//=============================================//
void CBERightAPSwitch(void * pButton, int event)
{
    SimRightAPSwitch(0, KEY_DOWN, pButton);
}

void CBExRightAPSwitch(void * pButton, int)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	if(playerAC->IsOn(AircraftClass::AttHold)){
		pCPButton->SetCurrentState(2);	//down
	}
	else if(playerAC->IsOn(AircraftClass::AltHold)) {
		pCPButton->SetCurrentState(1);	//up
	}
	else{
		pCPButton->SetCurrentState(0);	//middle
	}
}
//=============================================//
// Callback Function LeftAPSwitch
//=============================================//
void CBELeftAPSwitch(void * pButton, int event)
{
    SimLeftAPSwitch(0, KEY_DOWN, pButton);
}

void CBExLeftAPSwitch(void * pButton, int)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

    CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	if(playerAC->IsOn(AircraftClass::HDGSel)){
		pCPButton->SetCurrentState(1);	//up
	}
	else if(playerAC->IsOn(AircraftClass::StrgSel)) {
		pCPButton->SetCurrentState(2);	//down
	}
	else{
		pCPButton->SetCurrentState(0);	//middle*/
	}
}
//=============================================//
// Callback Function APOverride
//=============================================//
void CBEAPOverride(void * pButton, int event)
{
    SimAPOverride(0, KEY_DOWN, pButton);
}

//=============================================//
// Callback Function WarnReset
//=============================================//
void CBEWarnReset(void * pButton, int event)
{
    SimWarnReset(0, KEY_DOWN, pButton);
}
//=============================================//
// Callback Function WarnReset
//=============================================//
void CBExWarnReset(void * pButton, int event)
{
    CPButtonObject *pCPButton	= (CPButtonObject*) pButton;

	if(g_bRealisticAvionics)
	{
	    pCPButton->SetCurrentState(event == CP_CHECK_EVENT ? 0 : 1); // JPO
	}
}
//=============================================//
// Callback Function ReticleSwitch
//=============================================//
void CBEReticleSwitch(void * pButton, int event)
{
    SimReticleSwitch(0, KEY_DOWN, pButton);
}

void CBExReticleSwitch(void * pButton, int)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

    CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	if(TheHud->WhichMode == 1){
		//normal
		pCPButton->SetCurrentState(1);
	}
	else if (TheHud->WhichMode == 2){
		//backup
		pCPButton->SetCurrentState(2);
	}
	else{
		pCPButton->SetCurrentState(0);	//off
	}
}

//=============================================//
// Callback Function Panel Lights
//=============================================//

void CBEInteriorLightSwitch(void * pButton, int event)
{
    SimInteriorLight(0, KEY_DOWN, pButton);
}

void CBExInteriorLightSwitch(void * pButton, int event)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

    CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	switch (playerAC->GetInteriorLight()) {
		case AircraftClass::LT_OFF:
		    pCPButton->SetCurrentState(0);
	    break;
		case AircraftClass::LT_LOW:
		    pCPButton->SetCurrentState(1);
	    break;
		case AircraftClass::LT_NORMAL:
			pCPButton->SetCurrentState(2);
	    break;
    }
//	if (OTWDriver.pCockpitManager != NULL){
//		OTWDriver.pCockpitManager->UpdatePalette();
//	}

}

//=============================================//
// Callback Function Panel Lights
//=============================================//

void CBEInstrumentLightSwitch(void * pButton, int event)
{
    SimInstrumentLight(0, KEY_DOWN, pButton);
}

void CBExInstrumentLightSwitch(void * pButton, int event)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

    CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	switch (playerAC->GetInstrumentLight()) {
		case AircraftClass::LT_OFF:
		    pCPButton->SetCurrentState(0);
	    break;
		case AircraftClass::LT_LOW:
		    pCPButton->SetCurrentState(1);
	    break;
		case AircraftClass::LT_NORMAL:
		    pCPButton->SetCurrentState(2);
	    break;
    }
//	if (OTWDriver.pCockpitManager != NULL){
//		OTWDriver.pCockpitManager->UpdatePalette();
//	}
}

//=============================================//
// Callback Function Panel Lights
//=============================================//

void CBESpotLightSwitch(void * pButton, int event)
{
    SimSpotLight(0, KEY_DOWN, pButton);
}

void CBExSpotLightSwitch(void * pButton, int event)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	switch (playerAC->GetSpotLight()) {
		case AircraftClass::LT_OFF:
		    pCPButton->SetCurrentState(0);
	    break;
		case AircraftClass::LT_LOW:
		    pCPButton->SetCurrentState(1);
	    break;
		case AircraftClass::LT_NORMAL:
		    pCPButton->SetCurrentState(2);
	    break;
    }
}
//=============================================//
// Callback Function Seat Arm
//=============================================//

void CBESeatArmSwitch(void * pButton, int event)
{
    SimSeatArm(0, KEY_DOWN, pButton);
}

void CBExSeatArmSwitch(void * pButton, int event)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	if(playerAC->SeatArmed){
		pCPButton->SetCurrentState(1);
	}
	else{
		pCPButton->SetCurrentState(0);
    }
}

//MI EWS Stuff
void CBExEWSRWRPowerSwitch(void * pButton, int event)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;

	if(g_bRealisticAvionics){
		//MI we need this
		if(playerAC->PowerSwitchOn(AircraftClass::EWSRWRPower)){
			pCPButton->SetCurrentState(1);
		}
		else{
			pCPButton->SetCurrentState(0);
		}
	    //pCPButton->SetCurrentState(event == CP_CHECK_EVENT ? 0 : 1); // JPO
	}
}

void CBEEWSRWRPower(void * pButton, int event)
{
    SimEWSRWRPower(0, KEY_DOWN, pButton);
}

void CBExEWSJammerPowerSwitch(void * pButton, int event)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

    CPButtonObject *pCPButton	= (CPButtonObject*) pButton;

	if(g_bRealisticAvionics)
	{
		//MI we need this
		if(playerAC->PowerSwitchOn(AircraftClass::EWSJammerPower)){
			pCPButton->SetCurrentState(1);
		}
		else{
			pCPButton->SetCurrentState(0);
		}
		//pCPButton->SetCurrentState(event == CP_CHECK_EVENT ? 0 : 1); // JPO
	}
}

void CBEEWSJammerPower(void * pButton, int event)
{
    SimEWSJammerPower(0, KEY_DOWN, pButton);
}

void CBExEWSChaffPowerSwitch(void * pButton, int event)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

    CPButtonObject *pCPButton	= (CPButtonObject*) pButton;

	if(g_bRealisticAvionics)
	{
		//MI we need this
		if(playerAC->PowerSwitchOn(AircraftClass::EWSChaffPower)){
			pCPButton->SetCurrentState(1);
		}
		else{
			pCPButton->SetCurrentState(0);
		}
		//pCPButton->SetCurrentState(event == CP_CHECK_EVENT ? 0 : 1); // JPO
	}
}

void CBEEWSChaffPower(void * pButton, int event)
{
    SimEWSChaffPower(0, KEY_DOWN, pButton);
}

void CBExEWSFlarePowerSwitch(void * pButton, int event)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;

	if(g_bRealisticAvionics)
	{
		//MI we need this
		if(playerAC->PowerSwitchOn(AircraftClass::EWSFlarePower)){
			pCPButton->SetCurrentState(1);
		}
		else{
			pCPButton->SetCurrentState(0);
		}
		//pCPButton->SetCurrentState(event == CP_CHECK_EVENT ? 0 : 1); // JPO
	}
}

void CBEEWSFlarePower(void * pButton, int event)
{
    SimEWSFlarePower(0, KEY_DOWN, pButton);
}

void CBExEWSPGMButton(void * pButton, int event)
{
    CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
    if (playerAC == NULL){
		return;
	}

	switch (playerAC->EWSPGM()){
		case AircraftClass::EWSPGMSwitch::Off:
			pCPButton->SetCurrentState(0);
		break;
		case AircraftClass::EWSPGMSwitch::Stby:
			pCPButton->SetCurrentState(1);
		break;
		case AircraftClass::EWSPGMSwitch::Man:
			pCPButton->SetCurrentState(2);
		break;
		case AircraftClass::EWSPGMSwitch::Semi:
			pCPButton->SetCurrentState(3);
		break;
		case AircraftClass::EWSPGMSwitch::Auto:
			pCPButton->SetCurrentState(4);
		break;
		default:
			ShiWarning("Unknown EWS PGM state");
		break;
	}
}

void CBEEWSPGMButton(void * pButton, int event)
{
	if (event == CP_MOUSE_BUTTON0){
		SimEWSPGMInc (0, KEY_DOWN, NULL);
	}
	else {
		SimEWSPGMDec(0, KEY_DOWN, NULL);
	}
}

//Program select button
void CBExEWSProgButton(void * pButton, int event)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
    if (playerAC == NULL){
		return;
	}

    CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	switch(playerAC->EWSProgNum){
		case 0:
			pCPButton->SetCurrentState(0);
		break;
		case 1:
			pCPButton->SetCurrentState(1);
		break;
		case 2:
			pCPButton->SetCurrentState(2);
		break;
		case 3:
			pCPButton->SetCurrentState(3);
		break;
		default:
			ShiWarning("Unknown EWS Program state");
		break;
	}

}

void CBEEWSProgButton(void * pButton, int event)
{
	if(event == CP_MOUSE_BUTTON0){
		SimEWSProgInc (0, KEY_DOWN, NULL);
	}
	else {
		SimEWSProgDec(0, KEY_DOWN, NULL);
	}
}

// JPO main power button
void CBEMainPower(void *pButton, int event)
{
	if(event == CP_MOUSE_BUTTON0){
		SimMainPowerInc (0, KEY_DOWN, NULL);
	}
	else {
		SimMainPowerDec(0, KEY_DOWN, NULL);
	}
}

void CBExMainPower(void * pButton, int event)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
    if (playerAC == NULL){
		return;
	}

    CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	switch(playerAC->MainPower())
	{
		case AircraftClass::MainPowerOff:
			pCPButton->SetCurrentState(0);
			break;
		case AircraftClass::MainPowerBatt:
			pCPButton->SetCurrentState(1);
			break;
		case AircraftClass::MainPowerMain:
			pCPButton->SetCurrentState(2);
			break;
		default:
			ShiWarning("Unknown main power state");
			break;
	}

}


//=============================================//
// Callback Function NWS
//=============================================//
void CBENwsToggle(void * pButton, int event)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
    if (playerAC == NULL){
		return;
	}

	if (!playerAC->af->IsSet(AirframeClass::NoseSteerOn)){
		playerAC->af->SetFlag(AirframeClass::NoseSteerOn);
	}
	else {
		playerAC->af->ClearFlag(AirframeClass::NoseSteerOn);
	}
}

void CBExNwsToggle(void * pButton, int)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
    if (playerAC == NULL){
		return;
	}

    CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	if(playerAC->af->IsSet(AirframeClass::NoseSteerOn)) {
	    pCPButton->SetCurrentState(1);
	}
	else {
	    pCPButton->SetCurrentState(0);
	}
}

//=============================================//
// Callback Function Idle Detent
//=============================================//
void CBEIdleDetent(void * pButton, int event)
{
    SimThrottleIdleDetent(0, KEY_DOWN, pButton);
}

void CBExIdleDetent(void * pButton, int event)
{
    CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
    pCPButton->SetCurrentState(event == CP_CHECK_EVENT ? 0 : 1); // JPO
}
//=============================================//
// Callback Function Inhibit VMS
//=============================================//
void CBEInhibitVMS(void * pButton, int event)
{
    SimInhibitVMS(0, KEY_DOWN, pButton);
}

void CBExInhibitVMS(void * pButton, int event)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
    if (playerAC == NULL){
		return;
	}
    CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	if(!playerAC->playBetty){
		pCPButton->SetCurrentState(1); // Betty Off
	}
	else{
		pCPButton->SetCurrentState(0);
	}

}
//=============================================//
// Callback Function RF Switch
//=============================================//
void CBERFSwitch(void * pButton, int event)
{
    SimRFSwitch(0, KEY_DOWN, pButton);
}

void CBExRFSwitch(void * pButton, int event)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
    if (playerAC == NULL){
		return;
	}

    CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	switch (playerAC->RFState){
		case 0:
			//NORM
			pCPButton->SetCurrentState(0);
		break;
		case 1:
			//QUIET
			pCPButton->SetCurrentState(1);
		break;
		case 2:
			//SILENT
			pCPButton->SetCurrentState(2);
		break;
		default:
			;
	}
}

//=============================================//
// Callback Function Gear Handle
//=============================================//
void CBEGearHandle(void * pButton, int event)
{
    AFGearToggle(0, KEY_DOWN, NULL);
}

void CBExGearHandle(void * pButton, int event)
{
	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	if (playerAC->af->gearHandle <= 0.0F){
		pCPButton->SetCurrentState(0);	//Gear up
	}
	else if (playerAC->af->gearHandle > 0.0F){
        pCPButton->SetCurrentState(1);	//Gear Down	
	}
}
//=============================================//
// Callback Function PinkySwitch
//=============================================//
void CBEPinkySwitch(void * pButton, int event)
{
    SimPinkySwitch(0, KEY_DOWN, pButton);
}
//=============================================//
// Callback Function Ground Jett Enable
//=============================================//
void CBEGndJettEnable(void * pButton, int event)
{
    SimGndJettEnable(0, KEY_DOWN, NULL);
}

void CBExGndJettEnable(void * pButton, int event)
{
	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL || playerAC->Sms == NULL){
		return;
	}

	if (playerAC->Sms->GndJett){
		pCPButton->SetCurrentState(1);
	}
	else {
		pCPButton->SetCurrentState(0);
	}
}
//=============================================//
// Callback Function Exterior Lights Power
//=============================================//
void CBEExtlPower(void * pButton, int event)
{
    SimExtlPower(0, KEY_DOWN, NULL);
}

void CBExExtlPower(void * pButton, int event)
{
	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	if (playerAC->ExtlState(AircraftClass::ExtlLightFlags::Extl_Main_Power)){
		pCPButton->SetCurrentState(1);	//up
	}
	else {
		pCPButton->SetCurrentState(0);	//down
	}
}
//=============================================//
// Callback Function Exterior Lights Anti Collision
//=============================================//
void CBEExtlAntiColl(void * pButton, int event)
{
    SimExtlAntiColl(0, KEY_DOWN, NULL);
}

void CBExExtlAntiColl(void * pButton, int event)
{
	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	if (playerAC->ExtlState(AircraftClass::ExtlLightFlags::Extl_Anti_Coll)){
		pCPButton->SetCurrentState(1);	//up
	}
	else {
		pCPButton->SetCurrentState(0);	//down
	}
}
//=============================================//
// Callback Function Exterior Lights Steady/Flash
//=============================================//
void CBEExtlSteady(void * pButton, int event)
{
    SimExtlSteady(0, KEY_DOWN, NULL);
}

void CBExExtlSteady(void * pButton, int event)
{
	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}
	if (playerAC->ExtlState(AircraftClass::ExtlLightFlags::Extl_Flash)){
		pCPButton->SetCurrentState(1);	//up
	}
	else{
		pCPButton->SetCurrentState(0);	//down
	}
}
//=============================================//
// Callback Function Exterior Lights Wing/Tail
//=============================================//
void CBEExtlWing(void * pButton, int event)
{
    SimExtlWing(0, KEY_DOWN, NULL);
}

void CBExExtlWing(void * pButton, int event)
{
	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	if (playerAC->ExtlState(AircraftClass::ExtlLightFlags::Extl_Wing_Tail)){
		pCPButton->SetCurrentState(1);	//up
	}
	else {
		pCPButton->SetCurrentState(0);	//down
	}
}
//=============================================//
// Callback Function AVTR Switch
//=============================================//
void CBEAVTRSwitch(void * pButton, int event)
{
    SimAVTRSwitch(0, KEY_DOWN, NULL);
}

void CBExAVTRSwitch(void * pButton, int event)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	if(playerAC->AVTRState(AircraftClass::AVTRStateFlags::AVTR_ON)){
		pCPButton->SetCurrentState(1);	//up
	}
	else if(playerAC->AVTRState(AircraftClass::AVTRStateFlags::AVTR_AUTO)){
		pCPButton->SetCurrentState(2);	//middle
	}
	else{
		pCPButton->SetCurrentState(0);	//down
	}
}
//=============================================//
// Callback Function IFF Power Switch
//=============================================//
void CBEIFFPower(void * pButton, int event)
{
    SimIFFPower(0, KEY_DOWN, NULL);
}

void CBExIFFPower(void * pButton, int event)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	if(g_bIFF || g_bMLU){
		if(playerAC->PowerSwitchOn(AircraftClass::IFFPower)){
			pCPButton->SetCurrentState(1);
		}
		else {
			pCPButton->SetCurrentState(0);
		}
	}
	else{
		pCPButton->SetCurrentState(0);
	}
}
//=============================================//
// Callback Function INS Switch
//=============================================//
void CBEINSSwitch(void * pButton, int event)
{
	if (event == CP_MOUSE_BUTTON0){
		SimINSInc(0, KEY_DOWN, NULL);
	}
	else {
		SimINSDec(0, KEY_DOWN, NULL);
	}
}

void CBExINSSwitch(void * pButton, int event)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;

	if(playerAC->INSState(AircraftClass::INS_PowerOff)){
		pCPButton->SetCurrentState(0);
	}
	else if(playerAC->INSState(AircraftClass::INS_AlignNorm)){
		pCPButton->SetCurrentState(1);
	}
	else if(playerAC->INSState(AircraftClass::INS_Nav)){
		pCPButton->SetCurrentState(2);
	}
	else if(playerAC->INSState(AircraftClass::INS_AlignFlight)){
		pCPButton->SetCurrentState(3);
	}
}
//=============================================//
// Callback Function LEF Lock Switch
//=============================================//
void CBELEFLockSwitch(void * pButton, int event)
{
    SimLEFLockSwitch(0, KEY_DOWN, NULL);
}

void CBExLEFLockSwitch(void * pButton, int event)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}
	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;

	if(playerAC->LEFLocked){
		pCPButton->SetCurrentState(1);
	}
	else{
		pCPButton->SetCurrentState(0);
	}
}
//=============================================//
// Callback Function Digital Backup Switch
//=============================================//
void CBEDigitalBUP(void * pButton, int event)
{
    SimDigitalBUP(0, KEY_DOWN, NULL);
}

void CBExDigitalBUP(void * pButton, int event)
{
	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
}
//=============================================//
// Callback Function Alt Flaps Switch
//=============================================//
void CBEAltFlaps(void * pButton, int event){
    SimAltFlaps(0, KEY_DOWN, NULL);
}

void CBExAltFlaps(void * pButton, int event){
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;

	if (playerAC->TEFExtend){
		pCPButton->SetCurrentState(1);
	}
	else{
		pCPButton->SetCurrentState(0);
	}
}
//=============================================//
// Callback Function Manual Flyup Switch
//=============================================//
void CBEManualFlyup(void * pButton, int event)
{
    SimManualFlyup(0, KEY_DOWN, NULL);
}

void CBExManualFlyup(void * pButton, int event)
{
	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
}
//=============================================//
// Callback Function FLCS Reset Switch
//=============================================//
void CBEFLCSReset(void * pButton, int event)
{
    SimFLCSReset(0, KEY_DOWN, NULL);
}

void CBExFLCSReset(void * pButton, int event)
{
	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
}			
//=============================================//
// Callback Function BIT switch Switch
//=============================================//
void CBEFLTBIT(void * pButton, int event)
{
    SimFLTBIT(0, KEY_DOWN, NULL);
}

void CBExFLTBIT(void * pButton, int event)
{
	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
}
//=============================================//
// Callback Function OBOGS Bit Switch
//=============================================//
void CBEOBOGSBit(void * pButton, int event)
{
    SimOBOGSBit(0, KEY_DOWN, NULL);
}

void CBExOBOGSBit(void * pButton, int event)
{
	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
}
//=============================================//
// Callback Function Mal + Ind lights Switch
//=============================================//
void CBEMalIndLights(void * pButton, int event)
{
	if(event == CP_MOUSE_BUTTON0)
		SimMalIndLights(0, KEY_DOWN, NULL);
	else
		SimMalIndLights(0, 0, NULL);
}

void CBExMalIndLights(void * pButton, int event)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;

	if(playerAC->TestLights){
		pCPButton->SetCurrentState(1);
	}
	else{
		pCPButton->SetCurrentState(0);
	}
}
//=============================================//
// Callback Function Probeheat Switch
//=============================================//
void CBEProbeHeat(void * pButton, int event)
{
    SimProbeHeat(0, KEY_DOWN, NULL);
}

void CBExProbeHeat(void * pButton, int event)
{
	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
}
//=============================================//
// Callback Function EPU GEN Switch
//=============================================//
void CBEEPUGEN(void * pButton, int event)
{
    SimEPUGEN(0, KEY_DOWN, NULL);
}

void CBExEPUGEN(void * pButton, int event)
{
	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
}
//=============================================//
// Callback Function Test Switch
//=============================================//
void CBETestSwitch(void * pButton, int event)
{
    SimTestSwitch(0, KEY_DOWN, NULL);
}

void CBExTestSwitch(void * pButton, int event)
{
	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
}
//=============================================//
// Callback Function Overheat Switch
//=============================================//
void CBEOverHeat(void * pButton, int event)
{
	SimOverHeat(0, KEY_DOWN, NULL);
}

void CBExOverHeat(void * pButton, int event)
{
	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
}		
//=============================================//
// Callback Function Trim/AP Disc Switch
//=============================================//
void CBETrimAPDisc(void * pButton, int event)
{
    SimTrimAPDisc(0, KEY_DOWN, NULL);
}

void CBExTrimAPDisc(void * pButton, int event)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;

	if(playerAC->TrimAPDisc){
		pCPButton->SetCurrentState(1);
	}
	else{
		pCPButton->SetCurrentState(0);
	}
}
//=============================================//
// Callback Function Max Power Switch
//=============================================//
void CBEMaxPower(void * pButton, int event)
{
    SimMaxPower(0, KEY_DOWN, NULL);
}

void CBExMaxPower(void * pButton, int event)
{
	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
}
//=============================================//
// Callback Function AB Reset Switch
//=============================================//
void CBEABReset(void * pButton, int event)
{
    SimABReset(0, KEY_DOWN, NULL);
}

void CBExABReset(void * pButton, int event)
{
	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
}
//=============================================//
// Callback Function Trim NoseUp
//=============================================//
void CBETrimNose(void * pButton, int event)
{
	if(event == CP_MOUSE_BUTTON0){
		SimTrimNoseUp(0, KEY_DOWN, NULL);
	}
	else{
		SimTrimNoseDown(0, KEY_DOWN, NULL);
	}
}
//=============================================//
// Callback Function Trim YawLeft
//=============================================//
void CBETrimYaw(void * pButton, int event)
{
	if(event == CP_MOUSE_BUTTON0){
		SimTrimYawLeft(0, KEY_DOWN, NULL);
	}
	else{
		SimTrimYawRight(0, KEY_DOWN, NULL);
	}
}

void CBExTrimYaw(void * pButton, int event)
{
	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	int state = -(int)(UserStickInputs.ytrim * 100);
	if(state <= -50)
		pCPButton->SetCurrentState(1);
	else if(state <= -40 && state > -50)
		pCPButton->SetCurrentState(2);
	else if(state <= -30 && state > -40)
		pCPButton->SetCurrentState(3);
	else if(state <= -20 && state > -30)
		pCPButton->SetCurrentState(4);
	else if(state <= -10 && state > -20)
		pCPButton->SetCurrentState(5);
	else if(state <= 9 && state > -10)
		pCPButton->SetCurrentState(6);
	else if(state >= 10 && state < 20)
		pCPButton->SetCurrentState(7);
	else if(state >= 20 && state < 30)
		pCPButton->SetCurrentState(8);
	else if(state >= 30 && state < 40)
		pCPButton->SetCurrentState(9);
	else if(state >= 40 && state < 50)
		pCPButton->SetCurrentState(10);
	else if(state >= 50)
		pCPButton->SetCurrentState(11);
}
//=============================================//
// Callback Function Trim RollLeft
//=============================================//
void CBETrimRoll(void * pButton, int event)
{
	if (event == CP_MOUSE_BUTTON0){
		SimTrimRollLeft(0, KEY_DOWN, NULL);
	}
	else {
		SimTrimRollRight(0, KEY_DOWN, NULL);
	}
}

//=============================================//
// Callback Function Missile Volume
//=============================================//
void CBEMissileVol(void * pButton, int event)
{
	if (event == CP_MOUSE_BUTTON0){
		SimStepMissileVolumeUp(0, KEY_DOWN, NULL);
	}
	else {
		SimStepMissileVolumeDown(0, KEY_DOWN, NULL);
	}
}
void CBExMissileVol(void * pButton, int event)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	int pos = playerAC->MissileVolume;
	switch(pos)
	{
		case 0:	//max vol
			pCPButton->SetCurrentState(8);
			break;
		case 1:
			pCPButton->SetCurrentState(7);
			break;
		case 2:
			pCPButton->SetCurrentState(6);
			break;
		case 3:
			pCPButton->SetCurrentState(5);
			break;
		case 4:
			pCPButton->SetCurrentState(4);
			break;
		case 5:
			pCPButton->SetCurrentState(3);
			break;
		case 6:
			pCPButton->SetCurrentState(2);
			break;
		case 7:
			pCPButton->SetCurrentState(1);
			break;
		case 8:
			pCPButton->SetCurrentState(0);
			break;
		default:
			break;
	}
}
//=============================================//
// Callback Function Missile Volume
//=============================================//
void CBEThreatVol(void * pButton, int event)
{
	if(event == CP_MOUSE_BUTTON0){
		SimStepThreatVolumeUp(0, KEY_DOWN, NULL);
	}
	else{
		SimStepThreatVolumeDown(0, KEY_DOWN, NULL);
	}
}
void CBExThreatVol(void * pButton, int event)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	int pos = playerAC->ThreatVolume;
	switch(pos)
	{
		case 0:	//max vol
			pCPButton->SetCurrentState(8);
			break;
		case 1:
			pCPButton->SetCurrentState(7);
			break;
		case 2:
			pCPButton->SetCurrentState(6);
			break;
		case 3:
			pCPButton->SetCurrentState(5);
			break;
		case 4:
			pCPButton->SetCurrentState(4);
			break;
		case 5:
			pCPButton->SetCurrentState(3);
			break;
		case 6:
			pCPButton->SetCurrentState(2);
			break;
		case 7:
			pCPButton->SetCurrentState(1);
			break;
		case 8:
			pCPButton->SetCurrentState(0);
			break;
		default:
			break;
	}
}
//=============================================//
// Callback Function DEPR RET
//=============================================//
void CBEDeprRet(void * pButton, int event)
{
	if(event == CP_MOUSE_BUTTON0){
		SimRetUp(0, KEY_DOWN, NULL);
	}
	else{
		SimRetDn(0, KEY_DOWN, NULL);
	}
}
void CBExDeprRet(void * pButton, int event)
{
	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	if(TheHud)
	{
		int Pos = -TheHud->ReticlePosition;
		pCPButton->SetCurrentState(Pos);
	}
}
//=============================================//
// Callback Function TFR Button
//=============================================//
void CBETFRButton(void * pButton, int event)
{
	SimToggleTFR(0, KEY_DOWN, NULL);
}

//=============================================//
// Callback Function Thrust Reverser Button
//=============================================//
void CBEThrRevButton(void * pButton, int event)
{
	SimReverseThrusterToggle(0, KEY_DOWN, NULL);
}

//=============================================//
// Callback Function FLAP Switch
//=============================================//

void CBEFlap(void * pButton, int event){
	CPButtonObject *pCPButton	= static_cast<CPButtonObject*>(pButton);
	if(event == CP_MOUSE_BUTTON0){
	    AFIncFlap(0, KEY_DOWN, NULL);
		pCPButton->SetCurrentState(1);
		//if (pCPButton != NULL){ pCPButton->IncrementState(); }
	}
	else{
	    AFDecFlap(0, KEY_DOWN, NULL);
		pCPButton->SetCurrentState(2);
		//if (pCPButton != NULL){ pCPButton->DecrementState(); }
	}
}

void CBExFlap(void * pButton, int event){
	return;
	CPButtonObject *pCPButton	= static_cast<CPButtonObject*>(pButton);
	if (pCPButton == NULL){ return; }

	if(event == CP_MOUSE_BUTTON0){
		pCPButton->SetCurrentState(1);
	}
	else if (event == CP_MOUSE_BUTTON1){
		pCPButton->SetCurrentState(2);
	}
}


//=============================================//
// Callback Function TEF Switch
//=============================================//

void CBELef(void * pButton, int event){
	CPButtonObject *pCPButton	= static_cast<CPButtonObject*>(pButton);
	if (event == CP_MOUSE_BUTTON0){
	    AFIncLEF(0, KEY_DOWN, NULL);
		//if (pCPButton != NULL){ pCPButton->IncrementState(); }
	}
	else {
	    AFDecLEF(0, KEY_DOWN, NULL);
		//if (pCPButton != NULL){ pCPButton->DecrementState(); }
	}
}

//=============================================//
// Callback Function DragChute
//=============================================//
void CBEDragChute(void * pButton, int event)
{
    AFDragChute(0, KEY_DOWN, NULL);
}

void CBExDragChute(void * pButton, int event)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	if(playerAC->af->dragChute != AirframeClass::DRAGC_STOWED){
	    pCPButton->SetCurrentState(1);
	}
	else{
	    pCPButton->SetCurrentState(0);
	}
}

//=============================================//
// Callback Function Canopy
//=============================================//
void CBECanopy(void * pButton, int event)
{
    AFCanopyToggle(0, KEY_DOWN, NULL);
}

void CBExCanopy(void * pButton, int event)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	if(playerAC->af->canopyState)
	    pCPButton->SetCurrentState(1);
	else
	    pCPButton->SetCurrentState(0);
}
//=============================================//
// Callback Function Comm1 Volume
//=============================================//
void CBEComm1Vol(void * pButton, int event)
{
	if(event == CP_MOUSE_BUTTON0)
		SimStepComm1VolumeUp(0, KEY_DOWN, NULL);
	else
		SimStepComm1VolumeDown(0, KEY_DOWN, NULL);
}
void CBExComm1Vol(void * pButton, int event)
{
	int pos = 0;
	if(OTWDriver.pCockpitManager && OTWDriver.pCockpitManager->mpIcp){
		pos = OTWDriver.pCockpitManager->mpIcp->Comm1Volume;
	}

	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	switch(pos)
	{
		case 0:	//max vol
			pCPButton->SetCurrentState(8);
			break;
		case 1:
			pCPButton->SetCurrentState(7);
			break;
		case 2:
			pCPButton->SetCurrentState(6);
			break;
		case 3:
			pCPButton->SetCurrentState(5);
			break;
		case 4:
			pCPButton->SetCurrentState(4);
			break;
		case 5:
			pCPButton->SetCurrentState(3);
			break;
		case 6:
			pCPButton->SetCurrentState(2);
			break;
		case 7:
			pCPButton->SetCurrentState(1);
			break;
		case 8:
			pCPButton->SetCurrentState(0);
			break;
		default:
			break;
	}
}
//=============================================//
// Callback Function Comm2 Volume
//=============================================//
void CBEComm2Vol(void * pButton, int event)
{
	if(event == CP_MOUSE_BUTTON0)
		SimStepComm2VolumeUp(0, KEY_DOWN, NULL);
	else
		SimStepComm2VolumeDown(0, KEY_DOWN, NULL);
}
void CBExComm2Vol(void * pButton, int event)
{
	int pos = 0;
	if(OTWDriver.pCockpitManager && OTWDriver.pCockpitManager->mpIcp)
		pos = OTWDriver.pCockpitManager->mpIcp->Comm2Volume;

	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}
	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	switch(pos)
	{
		case 0:	//max vol
			pCPButton->SetCurrentState(8);
			break;
		case 1:
			pCPButton->SetCurrentState(7);
			break;
		case 2:
			pCPButton->SetCurrentState(6);
			break;
		case 3:
			pCPButton->SetCurrentState(5);
			break;
		case 4:
			pCPButton->SetCurrentState(4);
			break;
		case 5:
			pCPButton->SetCurrentState(3);
			break;
		case 6:
			pCPButton->SetCurrentState(2);
			break;
		case 7:
			pCPButton->SetCurrentState(1);
			break;
		case 8:
			pCPButton->SetCurrentState(0);
			break;
		default:
			break;
	}
}
void CBESymWheel(void * pButton, int event)
{
	// MD -- 20040703: if analog control is mapped, then change this
	// mouse behavior to be a power on/off toggle instead of changing
	// both power on/off and brightness.
	if (IO.AnalogIsUsed(AXIS_HUD_BRIGHTNESS) == true) {
		if(event == CP_MOUSE_BUTTON0)
			SimHUDOn(0, KEY_DOWN, NULL);
		else
			SimHUDOff(0, KEY_DOWN, NULL);
	}
	else {
		if(event == CP_MOUSE_BUTTON0)
			SimSymWheelUp(0, KEY_DOWN, NULL);
		else
			SimSymWheelDn(0, KEY_DOWN, NULL);	
	}
}
void CBExSymWheel(void * pButton, int event)
{
	int pos = 0;
	if(TheHud)
		pos = (int)(TheHud->SymWheelPos * 10);

	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}
	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	switch(pos)
	{
		case 10:	//max vol
			pCPButton->SetCurrentState(5);
			break;
		case 9:
			pCPButton->SetCurrentState(4);
			break;
		case 8:
			pCPButton->SetCurrentState(3);
			break;
		case 7:
			pCPButton->SetCurrentState(2);
			break;
		case 6:
			pCPButton->SetCurrentState(1);
			break;
		case 5:
			pCPButton->SetCurrentState(0);
			break;
		default:
//			ShiWarning("No good state!");
		break;
	}
}
//MI for Jags
void CBESetNightPanel(void * pButton, int event)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	if(OTWDriver.pCockpitManager)
	{
		CPPanel* curPanel = OTWDriver.pCockpitManager->GetActivePanel();

		if(curPanel->mIdNum == 94700)
		{
			OTWDriver.pCockpitManager->SetActivePanel(4700);
			playerAC->NightLight = FALSE;
		}
		else
		{
			OTWDriver.pCockpitManager->SetActivePanel(94700);
			playerAC->NightLight = TRUE;
		}
	}
}
/*
void CBExSetNightPanel(void * pButton, int event)
{
	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	if(playerAC)
	{
		if(playerAC->NightLight){
			pCPButton->SetCurrentState(1);
		}
		else{
			pCPButton->SetCurrentState(0);
		}
	}
}*/

void CBEDummyCallback(void *pButton, int event){
	CPButtonObject *pCPButton	= (CPButtonObject*) pButton;
	if (event == CP_MOUSE_BUTTON0){
		pCPButton->IncrementState();
	}
	else if (event == CP_MOUSE_BUTTON1){
		pCPButton->DecrementState();
	}
}
