#include "stdafx.h"
#include "cpcb.h"
#include "stdhdr.h"
#include "airframe.h"
#include "alr56.h"
#include "flightData.h"
#include "cbackproto.h"
#include "aircrft.h"

#include "cplight.h"
#include "fack.h"
#include "simdrive.h"
#include "camp2sim.h"
#include "radar.h"
#include "cpmisc.h"
#include "cpmanager.h"
#include "dofsnswitches.h"
#include "Sensclas.h"//TJL 01/24/04
#include "Object.h"
#include "radardoppler.h"
#include "fcc.h"
#include "hud.h"
#include "drawbsp.h" // sfr: for 3d pit stuff

extern bool shootCue = 0;

extern bool g_bRealisticAvionics;
extern bool g_bTO_LDG_LightFix;

// ECM Stuff
void CBEECMPwrLight(void * pObject) {
	CPLight *pCPLight	= (CPLight*) pObject;
	
	if(SimDriver.GetPlayerEntity() && SimDriver.GetPlayerEntity()->HasSPJamming()) {
		pCPLight->mState = SimDriver.GetPlayerEntity()->IsSetFlag(ECM_ON) != FALSE;
	}
	else {
		pCPLight->mState = FALSE;
	}
	//MI data extraction
	// MD -- 20031011: moved to cautions.cpp so the bits are set even if the player isn't
	// looking at the cockpit view.
	//if (pCPLight->mState) 
	//{
	//	cockpitFlightData.SetLightBit2(FlightData::EcmPwr);
	//}
	//else 
	//{
	//	cockpitFlightData.ClearLightBit2(FlightData::EcmPwr);
	//}
}

void CBEECMFailLight(void * pObject) {
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	CPLight *pCPLight	= (CPLight*) pObject;
	if (playerAC->HasSPJamming()) {
		if (
			playerAC->mFaults->GetFault(FaultClass::epod_fault) || 
			playerAC->mFaults->GetFault(FaultClass::blkr_fault)
		){ 			
			pCPLight->mState = TRUE;
		}
		else {
			pCPLight->mState = FALSE;
		}
	}
	else {
		pCPLight->mState = FALSE;
	}

	//MI data extraction
	// MD -- 20031011: moved to cautions.cpp so the bits are set even if the player isn't
	// looking at the cockpit view.
	//if (pCPLight->mState) 
	//{
	//	cockpitFlightData.SetLightBit2(FlightData::EcmFail);
	//}
	//else 
	//{
	//	cockpitFlightData.ClearLightBit2(FlightData::EcmFail);
	//}
}


void CBEAuxWarnActL(void * pObject) {
	CPLight*	pCPLight = (CPLight*) pObject;
   if (pCPLight && (SimDriver.GetPlayerEntity()) && (SimDriver.GetPlayerEntity()->IsSetFlag(MOTION_OWNSHIP))) {
	   PlayerRwrClass* theRwr = (PlayerRwrClass*)FindSensor(SimDriver.GetPlayerEntity(), SensorClass::RWR);
		if(theRwr) {
			pCPLight->mState = (theRwr->HasActivity() != FALSE);
		}
		//MI extracting Data
		// MD -- 20031011: moved to cautions.cpp so the bits are set even if the player isn't
		// looking at the cockpit view.
		//if (pCPLight->mState) 
		//{
		//	cockpitFlightData.SetLightBit2(FlightData::AuxAct);
		//}
		//else 
		//{
		//	cockpitFlightData.ClearLightBit2(FlightData::AuxAct);
		//}
	}
}

void CBELaunchL(void * pObject) {
	CPLight*	pCPLight = (CPLight*) pObject;
   if (pCPLight && (SimDriver.GetPlayerEntity()) && (SimDriver.GetPlayerEntity()->IsSetFlag(MOTION_OWNSHIP))) {
	   PlayerRwrClass* theRwr = (PlayerRwrClass*)FindSensor(SimDriver.GetPlayerEntity(), SensorClass::RWR);
		if(theRwr) {
			pCPLight->mState = (theRwr->LaunchIndication() != FALSE);
		}
		//MI extracting Data
		// MD -- 20031011: moved to cautions.cpp so the bits are set even if the player isn't
		// looking at the cockpit view.
		//if (pCPLight->mState) 
		//{
		//	cockpitFlightData.SetLightBit2(FlightData::Launch);
		//}
		//else 
		//{
		//	cockpitFlightData.ClearLightBit2(FlightData::Launch);
		//}
	}
}


void CBEHandoffL(void * pObject) {
	CPLight*	pCPLight = (CPLight*) pObject;
   if (pCPLight && (SimDriver.GetPlayerEntity()) && (SimDriver.GetPlayerEntity()->IsSetFlag(MOTION_OWNSHIP))) {
	   PlayerRwrClass* theRwr = (PlayerRwrClass*)FindSensor(SimDriver.GetPlayerEntity(), SensorClass::RWR);
		if(theRwr) {
			pCPLight->mState = (theRwr->ManualSelect() != FALSE);
		}
		//MI extracting Data
		// MD -- 20031011: moved to cautions.cpp so the bits are set even if the player isn't
		// looking at the cockpit view.
		//if (pCPLight->mState) 
		//{
		//	cockpitFlightData.SetLightBit2(FlightData::HandOff);
		//}
		//else 
		//{
		//	cockpitFlightData.ClearLightBit2(FlightData::HandOff);
		//}
	}
}


void CBEPriModeL(void * pObject) {
	CPLight*	pCPLight = (CPLight*) pObject;
   if (pCPLight && (SimDriver.GetPlayerEntity()) && (SimDriver.GetPlayerEntity()->IsSetFlag(MOTION_OWNSHIP))) {
	   PlayerRwrClass* theRwr = (PlayerRwrClass*)FindSensor(SimDriver.GetPlayerEntity(), SensorClass::RWR);
		if(theRwr) {
			pCPLight->mState = (theRwr->IsPriority() != FALSE);
		}
		//MI extracting DATA
		// MD -- 20031011: moved to cautions.cpp so the bits are set even if the player isn't
		// looking at the cockpit view.
		//if (pCPLight->mState) 
		//{
		//	cockpitFlightData.SetLightBit2(FlightData::PriMode);
		//}
		//else 
		//{
		//	cockpitFlightData.ClearLightBit2(FlightData::PriMode);
		//}
	}
}


void CBEUnknownL(void * pObject) {
	CPLight*	pCPLight = (CPLight*) pObject;
   if (pCPLight && (SimDriver.GetPlayerEntity()) && (SimDriver.GetPlayerEntity()->IsSetFlag(MOTION_OWNSHIP))) {
	   PlayerRwrClass* theRwr = (PlayerRwrClass*)FindSensor(SimDriver.GetPlayerEntity(), SensorClass::RWR);
		if(theRwr) {
			pCPLight->mState = (theRwr->LightUnknowns() != FALSE);
		}
		//MI extracting Data
		// MD -- 20031011: moved to cautions.cpp so the bits are set even if the player isn't
		// looking at the cockpit view.
		//if (pCPLight->mState) 
		//{
		//	cockpitFlightData.SetLightBit2(FlightData::Unk);
		//}
		//else 
		//{
		//	cockpitFlightData.ClearLightBit2(FlightData::Unk);
		//}
	}
}

void CBENavalL(void * pObject) {
	CPLight*	pCPLight = (CPLight*) pObject;
   if (pCPLight && (SimDriver.GetPlayerEntity()) && (SimDriver.GetPlayerEntity()->IsSetFlag(MOTION_OWNSHIP))) {
	   PlayerRwrClass* theRwr = (PlayerRwrClass*)FindSensor(SimDriver.GetPlayerEntity(), SensorClass::RWR);
		if(theRwr) {
			pCPLight->mState = (theRwr->ShowNaval() != FALSE);
		}
		//MI extracting Data
		// MD -- 20031011: moved to cautions.cpp so the bits are set even if the player isn't
		// looking at the cockpit view.
		//if (pCPLight->mState)
		//{
		//	cockpitFlightData.SetLightBit2(FlightData::Naval);
		//}
		//else 
		//{
		//	cockpitFlightData.ClearLightBit2(FlightData::Naval);
		//}
	}
}

void CBETgtSepL(void * pObject) {
	CPLight*	pCPLight = (CPLight*) pObject;
   if (pCPLight && (SimDriver.GetPlayerEntity()) && (SimDriver.GetPlayerEntity()->IsSetFlag(MOTION_OWNSHIP))) {
	   PlayerRwrClass* theRwr = (PlayerRwrClass*)FindSensor(SimDriver.GetPlayerEntity(), SensorClass::RWR);
		if(theRwr) {
			pCPLight->mState = (theRwr->TargetSep() != FALSE);
		}
		//MI extracting Data
		// MD -- 20031011: moved to cautions.cpp so the bits are set even if the player isn't
		// looking at the cockpit view.
		//if (pCPLight->mState) 
		//{
		//	cockpitFlightData.SetLightBit2(FlightData::TgtSep);
		//}
		//else 
		//{
		//	cockpitFlightData.ClearLightBit2(FlightData::TgtSep);
		//}
	}
}


void CBEAuxWarnSearchL(void * pObject) {
	CPLight*	pCPLight = (CPLight*) pObject;
   if (pCPLight && (SimDriver.GetPlayerEntity()) && (SimDriver.GetPlayerEntity()->IsSetFlag(MOTION_OWNSHIP))) {
	   PlayerRwrClass* theRwr = (PlayerRwrClass*)FindSensor(SimDriver.GetPlayerEntity(), SensorClass::RWR);
		if(theRwr) {
			pCPLight->mState = (theRwr->LightSearch() != FALSE);
		}
		//MI extracting Data
		// MD -- 20031011: moved to cautions.cpp so the bits are set even if the player isn't
		// looking at the cockpit view.
		//if (pCPLight->mState) 
		//{
		//	cockpitFlightData.SetLightBit2(FlightData::AuxSrch);
		//}
		//else 
		//{
		//	cockpitFlightData.ClearLightBit2(FlightData::AuxSrch);
		//}
	}
}


void CBEAuxWarnAltL(void * pObject) {
	CPLight*	pCPLight = (CPLight*) pObject;
   if (pCPLight && (SimDriver.GetPlayerEntity()) && (SimDriver.GetPlayerEntity()->IsSetFlag(MOTION_OWNSHIP))) {
	   PlayerRwrClass* theRwr = (PlayerRwrClass*)FindSensor(SimDriver.GetPlayerEntity(), SensorClass::RWR);
		if(theRwr) {
			pCPLight->mState = (theRwr->LowAltPriority() != FALSE);
		}
		//MI extracting Data
		// MD -- 20031011: moved to cautions.cpp so the bits are set even if the player isn't
		// looking at the cockpit view.
		//if (pCPLight->mState) 
		//{
		//	cockpitFlightData.SetLightBit2(FlightData::AuxLow);
		//}
		//else 
		//{
		//	cockpitFlightData.ClearLightBit2(FlightData::AuxLow);
		//}
	}
}


void CBEAuxWarnPwrL(void * pObject) {
	CPLight*	pCPLight = (CPLight*) pObject;
   if (pCPLight && (SimDriver.GetPlayerEntity()) && (SimDriver.GetPlayerEntity()->IsSetFlag(MOTION_OWNSHIP))) {
	   PlayerRwrClass* theRwr = (PlayerRwrClass*)FindSensor(SimDriver.GetPlayerEntity(), SensorClass::RWR);
		if(theRwr) {
			pCPLight->mState = (theRwr->IsOn() != FALSE);
		}
		//MI extracting Data
		// MD -- 20031011: moved to cautions.cpp so the bits are set even if the player isn't
		// looking at the cockpit view.
		//if (pCPLight->mState) 
		//{
		//	cockpitFlightData.SetLightBit2(FlightData::AuxPwr);
		//}
		//else 
		//{
		//	cockpitFlightData.ClearLightBit2(FlightData::AuxPwr);
		//}
	}
}








#if OLD_STUFF

// Aux Warn Stuff
void CBEAuxWarnSearch(void * pObject) {
	CPLight *pCPLight	= (CPLight*) pObject;

	if ((SimDriver.GetPlayerEntity()) && (SimDriver.GetPlayerEntity()->IsSetFlag(MOTION_OWNSHIP))) {

	   ALR56Class* theRwr = (ALR56Class*)FindSensor(SimDriver.GetPlayerEntity(), SensorClass::RWR);

		if(theRwr) {
			pCPLight->mState = theRwr->LightSearch();
		}
	}
}


void CBEAuxWarnActPwr(void * pObject) {
	CPLight *pCPLight	= (CPLight*) pObject;

	if ((SimDriver.GetPlayerEntity()) && (SimDriver.GetPlayerEntity()->IsSetFlag(MOTION_OWNSHIP))) {

	   ALR56Class* theRwr = (ALR56Class*)FindSensor(SimDriver.GetPlayerEntity(), SensorClass::RWR);

		if(theRwr) {
         pCPLight->mState = (theRwr->CurSpike() ? TRUE : FALSE);
		}
	}
}


void CBEAuxWarnAlt(void * pObject) {
	CPLight *pCPLight	= (CPLight*) pObject;

	if ((SimDriver.GetPlayerEntity()) && (SimDriver.GetPlayerEntity()->IsSetFlag(MOTION_OWNSHIP))) {

	   ALR56Class* theRwr = (ALR56Class*)FindSensor(SimDriver.GetPlayerEntity(), SensorClass::RWR);

		if(theRwr) {
			pCPLight->mState = theRwr->LowAltPriority();
      }
	}
}


void CBEAuxWarnPwr(void * pObject) {
	CPLight *pCPLight	= (CPLight*) pObject;

	pCPLight->mState = 1;
}
#endif

void CBECheckMasterCaution(void * pObject) {

	CPLight *pCPLight	= (CPLight*) pObject;

	OTWDriver.pCockpitManager->mMiscStates.StepMasterCautionLight();
	pCPLight->mState = OTWDriver.pCockpitManager->mMiscStates.GetMasterCautionLight();

	// sfr: testing this
#if 0
	OTWDriver.GetVirtualCockpit()->SetSwitchMask( 2, pCPLight->mState);
#endif
}

void CheckLandingGearHandle(void * pObject) {

	CPLight *pCPLight	= (CPLight*) pObject;
	int	currentState;
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL || !playerAC->IsSetFlag(MOTION_OWNSHIP)){
		return;
	}

	if (playerAC->af->gearHandle <= 0.0F){ 
		if(playerAC->af->gearPos == 0.0F && !playerAC->mFaults->GetFault(FaultClass::gear_fault)){
			currentState = 0;				// handle up & wheels locked				
		}
		else {
			currentState = 1;				// handle up & wheels moving
		}
	}
	else {
		if (playerAC->af->gearPos == 1.0F && !playerAC->mFaults->GetFault(FaultClass::gear_fault)){
			currentState = 2;				// handle down & wheels locked
		}
		else {
			currentState = 3;				// handle down & wheels moving
		}
	}
	pCPLight->mState = currentState;
}

void CheckThreatWarn(void * pObject, type_TWSubSystem subSystem)
{
CPLight* pCPLight = (CPLight*) pObject;;
PlayerRwrClass* theRwr = (PlayerRwrClass*)FindSensor ((AircraftClass*)pCPLight->mpOwnship, SensorClass::RWR);
int val = FALSE;

   switch (subSystem)
   {
      case handoff:
         val = theRwr->ManualSelect();
      break;

      case missile_launch:
         // If we have a launch warning and we're in the "ON" part of the blink cycle
         val = theRwr->LaunchIndication() && (vuxRealTime & 0x200);
      break;

      case pri_mode:
         val = theRwr->IsPriority();
      break;

      case sys_test:
         val = theRwr->ShowNaval();
      break;

      case tgt_t:
         val = theRwr->TargetSep();
      break;

      case unk:
         val = theRwr->LightUnknowns();
      break;

      case search:
         val = theRwr->LightSearch();
      break;

      case activate_power:
         val = theRwr->HasActivity();
      break;

      case low_altitude:
         val = theRwr->LowAltPriority();
      break;

      case system_power:
         val = theRwr->IsOn();
      break;
   }

	if(val)
   {
		pCPLight->mState = CPLIGHT_ON;
	}
	else
   {
		pCPLight->mState = CPLIGHT_OFF;
	}
}

void CheckCaution2(void				 *pObject,
						 type_CSubSystem subsystem1,
						 type_CSubSystem subsystem2) {

	CPLight*			pCPLight;
	FackClass*		faultSys;
	BOOL				stateSysA, stateSysB;

	pCPLight	= (CPLight*) pObject;
	faultSys	= ((AircraftClass*) pCPLight->mpOwnship)->mFaults;

#if 0
	//VWF Hack for testing
		if(pCPLight->mState == CPLIGHT_ON) {
			pCPLight->mState = CPLIGHT_OFF;
		}
		else {
			pCPLight->mState = CPLIGHT_ON;
		}
#endif

//	if(faultSys->IsFlagSet()) {

		stateSysB = faultSys->GetFault(subsystem1);
		stateSysA = faultSys->GetFault(subsystem2);
		
		pCPLight->mState = stateSysB * 2 + stateSysA;
//	}
}

void CheckCaution1(void	* pObject, type_CSubSystem subsystem) {

	CPLight*			pCPLight;
	FackClass*		faultSys;

	pCPLight	= (CPLight*) pObject;
	faultSys	= ((AircraftClass*) pCPLight->mpOwnship)->mFaults;

#if 0
//VWF Hack for testing
		if(pCPLight->mState == CPLIGHT_ON) {
			pCPLight->mState = CPLIGHT_OFF;
		}
		else {
			pCPLight->mState = CPLIGHT_ON;
		}
#endif

//	if(faultSys->IsFlagSet()) {

		if(faultSys->GetFault(subsystem)) {
			pCPLight->mState	= CPLIGHT_ON;
		}
		else{
			pCPLight->mState	= CPLIGHT_OFF;
		} 
}

void CBEAOAIndLight(void * pObject){
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	CPLight*		pCPLight;
	float			currentAOAVal;

	pCPLight	= (CPLight*) pObject;
	currentAOAVal = cockpitFlightData.alpha;

	//MI in realistic these only work with gear down  //JPG 2 Jan 04 - Nosir, -1 says AoA indexer is operational regardless of gear position
/*	if(g_bRealisticAvionics && SimDriver.GetPlayerEntity() && 
		SimDriver.GetPlayerEntity()->af->gearPos < 0.8F)
	{
		pCPLight->mState = CPLIGHT_AOA_OFF;
		return;
	}*/
	//TJL 03/08/04 F18

	switch (playerAC->af->GetTypeAC()){
		case 8:
		case 9:
		case 10:
			if (currentAOAVal >= 9.3F){
				pCPLight->mState = CPLIGHT_AOA_SLOW;
			}
			else if((currentAOAVal < 9.3F) && (currentAOAVal >= 6.9F)) {
				pCPLight->mState = CPLIGHT_AOA_ON;
			}
			else if(currentAOAVal < 6.8F) {
				pCPLight->mState = CPLIGHT_AOA_FAST;
			}
		break;
		case 6:
		case 7:
			if(currentAOAVal >= 15.5F) {
				pCPLight->mState = CPLIGHT_AOA_SLOW;
			}
			else if((currentAOAVal < 15.5F) && (currentAOAVal >= 14.5F)) {
				pCPLight->mState = CPLIGHT_AOA_ON;
			}
			else if(currentAOAVal < 14.5F) {
				pCPLight->mState = CPLIGHT_AOA_FAST;
			}
		break;
		default:
			if(currentAOAVal >= 14.0F) {
				pCPLight->mState = CPLIGHT_AOA_SLOW;
			}
			else if((currentAOAVal < 14.0F) && (currentAOAVal >= 11.5F)) {
				pCPLight->mState = CPLIGHT_AOA_ON;
			}
			else if(currentAOAVal < 11.5F) {
				pCPLight->mState = CPLIGHT_AOA_FAST;
			}
	}
}

extern void CBEAOAFastLight(void * pObject){
	
	CPLight*		pCPLight;
	float			currentAOAVal;

	pCPLight	= (CPLight*) pObject;
	currentAOAVal = cockpitFlightData.alpha;

	//MI in realistic these only work with gear down //JPG 2 Jan 04 - Nosir, -1 says AoA indexer is operational regardless of gear position
/*	if(g_bRealisticAvionics && SimDriver.GetPlayerEntity() && 
		SimDriver.GetPlayerEntity()->af->gearPos < 0.8F)
	{
		pCPLight->mState = CPLIGHT_AOA_OFF;
		return;
	}*/

	if(currentAOAVal < 11.5F) {
		pCPLight->mState = 1;
	}
	else {
		pCPLight->mState = 0;
	}
}

void CBERefuelLight(void * pObject){
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}	
	CPLight*		pCPLight;

	pCPLight				= (CPLight*) pObject;

// MD -- 20031215: moving this to cautions.cpp to ensure that the state is set independent of whether the lamp is being
// drawn in the OTW view currently or not,
//	if(OTWDriver.pCockpitManager->mMiscStates.mRefuelState == 3 && (vuxGameTime > (OTWDriver.pCockpitManager->mMiscStates.mRefuelTimer + 3000))) {
//		OTWDriver.pCockpitManager->mMiscStates.SetRefuelState(0);
//	}

	

	if (playerAC->af->IsSet(AirframeClass::NoseSteerOn)) {
		pCPLight->mState	= CPLIGHT_AR_NWS_ON;
	}

	if (playerAC->af->IsEngineFlag(AirframeClass::FuelDoorOpen)){
		pCPLight->mState	= CPLIGHT_AR_NWS_RDY;
		if (OTWDriver.pCockpitManager->mMiscStates.mRefuelState >CPLIGHT_AR_NWS_RDY){
			pCPLight->mState	= OTWDriver.pCockpitManager->mMiscStates.mRefuelState;
		}
	}
	//MI NWS light fix
	//else if (!SimDriver.GetPlayerEntity()->af->IsEngineFlag(AirframeClass::FuelDoorOpen) && pCPLight->mState > CPLIGHT_AR_NWS_RDY )
	else if (
		!playerAC->af->IsEngineFlag(AirframeClass::FuelDoorOpen) && 
		!playerAC->af->IsSet(AirframeClass::NoseSteerOn)
	){
		pCPLight->mState = CPLIGHT_AR_NWS_OFF;
	}
}

void CBEDiscLight(void * pObject) {

	CPLight*		pCPLight;

	pCPLight	= (CPLight*) pObject;

	if(OTWDriver.pCockpitManager->mMiscStates.mRefuelState == CPLIGHT_AR_NWS_DISC)
	{
		pCPLight->mState	= TRUE;
	}
	else {
		pCPLight->mState	= FALSE;
	}
}



void CBEThreatWarn7(void * pObject) {

	CheckThreatWarn(pObject, search);
}

void CBEThreatWarn8(void * pObject) {

	CheckThreatWarn(pObject, activate_power);
}

void CBEThreatWarn9(void * pObject) {

	CheckThreatWarn(pObject, low_altitude);
}

void CBEThreatWarn10(void * pObject) {

	CheckThreatWarn(pObject, system_power);
}



//Caution flt_cont_fault
void CBECaution1(void * pObject) {

	CPLight*			pCPLight;
	FackClass*		faultSys;

	pCPLight	= (CPLight*) pObject;
	faultSys	= ((AircraftClass*) pCPLight->mpOwnship)->mFaults;

	pCPLight->mState = faultSys->GetFault(FaultClass::flcs_fault) != 0;
}

//Caution le_flaps_fault
void CBECaution2(void * pObject) {

	CPLight*			pCPLight;
	FackClass*		faultSys;

	pCPLight	= (CPLight*) pObject;
	faultSys	= ((AircraftClass*) pCPLight->mpOwnship)->mFaults;

	// JPO
	pCPLight->mState = faultSys->GetFault(le_flaps_fault) != 0;
}

//Caution overheat_fault ???
void CBECaution3(void * pObject) {
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	CPLight*			pCPLight;
	FackClass*		faultSys;

	pCPLight	= (CPLight*) pObject;
	faultSys	= ((AircraftClass*) pCPLight->mpOwnship)->mFaults;
	pCPLight->mState = (faultSys->GetFault(FaultClass::eng_fault) != 0) && 
	    playerAC->af->rpm <= 0.75 &&
	    playerAC->af->FuelFlow() > 0.0f
	; // JPO
}

//Caution fuel_low_fault
void CBECaution4(void * pObject) {
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	CPLight*			pCPLight;
	FackClass*		faultSys;

	pCPLight	= (CPLight*) pObject;
	faultSys	= ((AircraftClass*) pCPLight->mpOwnship)->mFaults;

	pCPLight->mState = (playerAC->af->Fuel() < 750.0F) || faultSys->GetFault(FaultClass::fms_fault);
}

//Caution avionics_fault
void CBECaution5(void * pObject) {

	CPLight*		pCPLight;
	FackClass*		faultSys;

	pCPLight	= (CPLight*) pObject;
	faultSys	= ((AircraftClass*) pCPLight->mpOwnship)->mFaults;

	// JPO fix
	if(!g_bRealisticAvionics){
		pCPLight->mState = faultSys->GetFault(FaultClass::amux_fault) != 0;
	}
	else{
		pCPLight->mState = SimDriver.GetPlayerAircraft()->mFaults->NeedAckAvioncFault != 0;
	}
}

//Caution radar_alt_fault
void CBECaution6(void * pObject) {
	CPLight*			pCPLight;
	FackClass*		faultSys;

	pCPLight	= (CPLight*) pObject;
	faultSys	= ((AircraftClass*) pCPLight->mpOwnship)->mFaults;

	// JPO fix
	pCPLight->mState = faultSys->GetFault(FaultClass::ralt_fault) != 0;
}

//Caution iff_fault
void CBECaution7(void * pObject) {
	CPLight*			pCPLight;
	FackClass*		faultSys;

	pCPLight	= (CPLight*) pObject;
	faultSys	= ((AircraftClass*) pCPLight->mpOwnship)->mFaults;

	// IFF - JPO fix
	pCPLight->mState = faultSys->GetFault(FaultClass::iff_fault) != 0;
}

//Caution ecm_fault
void CBECaution8(void * pObject) {
	CPLight*			pCPLight;
	FackClass*		faultSys;

	pCPLight	= (CPLight*) pObject;
	faultSys	= ((AircraftClass*) pCPLight->mpOwnship)->mFaults;

	// JPO Fix
	pCPLight->mState = faultSys->GetFault(FaultClass::rwr_fault) != 0;
}

//Caution hook_fault - JPO Fix
void CBECaution9(void *pObject) {
	CPLight*			pCPLight;
	FackClass*		faultSys;

	pCPLight	= (CPLight*) pObject;
	faultSys	= ((AircraftClass*) pCPLight->mpOwnship)->mFaults;

	// use the hook caution to set it!
	pCPLight->mState = faultSys->GetFault(hook_fault) != 0;
}

//Caution nws_fault
void CBECaution10(void * pObject) {
  CheckCaution1(pObject, nws_fault); // JPO - lets use the routine...
}

//Caution cabin_press_fault
void CBECaution11(void * pObject) {
  CheckCaution1(pObject, cabin_press_fault); // ... and again
}

//Caution engine
void CBECaution12(void * pObject) {
	CPLight*			pCPLight;
	FackClass*		faultSys;

	pCPLight	= (CPLight*) pObject;
	faultSys	= ((AircraftClass*) pCPLight->mpOwnship)->mFaults;

	pCPLight->mState = (faultSys->GetFault(FaultClass::eng_fault) != 0) ;
}

// Caution TO/LDG Config		
void CBECaution13(void * pObject)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	CPLight*			pCPLight;
	pCPLight	= (CPLight*) pObject;
	
	if (playerAC->OnGround()){
		//MI set it to off here
		pCPLight->mState = CPLIGHT_OFF;
	}
	else {
		if(g_bTO_LDG_LightFix) {
			if (playerAC->mFaults && playerAC->mFaults->GetFault(to_ldg_config)){
				pCPLight->mState = TRUE;
			}
			else{
				pCPLight->mState = FALSE;
			}
		}
		else {
			if (
				//RV - I-Hawk - changed altitude value from 10000 to 5000 according to Dannycoh
				(playerAC->ZPos() > -5000.0F) &&
				(playerAC->GetKias() < 190.0F) && 
				(playerAC->ZDelta() * 60.0F >= 250.0F) && 
				(playerAC->af->gearPos != 1.0F)
			){
				pCPLight->mState = TRUE;
			}
			else {
				pCPLight->mState = FALSE;
			}
		}
	}
	//(In air and Altitude < 10,000ft and speed < 190 kts and descent >= 250FPM and LGs not down or TEF's not down) or (on ground and TEF's not fullydown) or (
}

// Caution DUAL FC / CANOPY	 	
void CBECaution14(void * pObject) {
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	CPLight*			pCPLight;
	FackClass*		faultSys;

	pCPLight	= (CPLight*) pObject;
	faultSys	= ((AircraftClass*) pCPLight->mpOwnship)->mFaults;

	//pCPLight->mState = (2 * faultSys->GetFault(FaultClass::fcc_fault) || faultSys->GetFault(FaultClass::dmux_fault)) + faultSys->GetFault(FaultClass::hud_fault);
	pCPLight->mState = 0;
// copied canopy fault code
	int canopyopen;
	if (playerAC->IsComplex()){
	    canopyopen = playerAC->GetDOFValue(COMP_CANOPY_DOF) > 0;
	}
	else {
	    canopyopen = playerAC->GetDOFValue(SIMP_CANOPY_DOF) > 0;
	}

	if (canopyopen || (faultSys && faultSys->GetFault(canopy))){
		pCPLight->mState = 1;
	}

	if (faultSys->GetFault(FaultClass::fcc_fault)){
		pCPLight->mState += 2;
	}

}

// Caution HYD/OIL PRESS
void CBECaution15(void * pObject) {
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	CPLight*			pCPLight;
	FackClass*		faultSys;

	pCPLight	= (CPLight*) pObject;
	faultSys	= ((AircraftClass*) pCPLight->mpOwnship)->mFaults;

	pCPLight->mState = (playerAC->af->rpm * 37.0F) < 15.0F ||
	    faultSys->GetFault(FaultClass::eng_fault) != 0 ||
	    !playerAC->af->HydraulicOK();
}
/////////


// Caution ENG FIRE/ENGINE 
void CBECaution16(void * pObject) {
	CPLight*			pCPLight;
	FackClass*		faultSys;

	pCPLight	= (CPLight*) pObject;
	faultSys	= ((AircraftClass*) pCPLight->mpOwnship)->mFaults;

#if 0
	// These are not faults, they are cautions
	switch (faultSys->GetFault(FaultClass::eng_fault)) {
	case eng_fire: // JPO just an engine fire 
	    pCPLight->mState = 3;
	    break;
	case 0: // nothing
	    pCPLight->mState = 0;
	    break;
	default: // either just something else or both
	    if (faultSys->GetFault(FaultClass::eng_fault) & eng_fire)
		pCPLight->mState = 2;
	    else
		pCPLight->mState = 1;
	    break;
	}

#endif

	// MD -- 20031221: re-wrote this so that the ENGINE half of this split face works like
	// the dash one says it does.
	// NB!!!!  I changed the state table swapping the last two over so any cockpit art that
	// has four state support needs to flip the last two srcloc lines in the dat file for
	// this lamp now.

	int engine = 0, fire = 0;

	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	if (
		(
			(playerAC->af->rpm <= 0.6F) &&
			(playerAC->MainPower() == AircraftClass::MainPowerMain)
		) || 
		(cockpitFlightData.ftit > 1100.0F)
	){
		engine = 1;
	}

	if (faultSys->GetFault(FaultClass::eng_fault) & eng_fire){
		fire = 2;
	}

	pCPLight->mState = engine + fire; // 0 == off, 1 == ENGINE, 2 == ENG FIRE, 3 == both

}

//TJL 01/25/04 multi-engine Engine Fire #2 copy/paste of above
// Caution ENG FIRE/ENGINE 
void CBEEng2WarningLight(void * pObject) {
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	CPLight*			pCPLight;
	FackClass*		faultSys;

	pCPLight	= (CPLight*) pObject;
	faultSys	= ((AircraftClass*) pCPLight->mpOwnship)->mFaults;


	// MD -- 20031221: re-wrote this so that the ENGINE half of this split face works like
	// the dash one says it does.
	// NB!!!!  I changed the state table swapping the last two over so any cockpit art that
	// has four state support needs to flip the last two srcloc lines in the dat file for
	// this lamp now.

	int engine = 0, fire = 0;

	if (
		((playerAC->af->rpm2 <= 0.6F) &&
		(playerAC->MainPower() == AircraftClass::MainPowerMain)) || 
		(cockpitFlightData.ftit2 > 1100.0F)
	){
		engine = 1;
	}

	if (faultSys->GetFault(FaultClass::eng2_fault) & eng_fire){
		fire = 2;
	}

	pCPLight->mState = engine + fire; // 0 == off, 1 == ENGINE, 2 == ENG FIRE, 3 == both

}

//ALT LOW
void CBECaution17(void * pObject) {
	CheckCaution1(pObject, alt_low);
}

//Front Landing Gear light
void CBEFrontLandGearLight(void * pObject) {

	CPLight	*pCPLight;

	pCPLight	= (CPLight*) pObject;

	if( ((AircraftClass*) pCPLight->mpOwnship)->af->gear[0].flags & GearData::GearProblem)
   {
 	   pCPLight->mState	= 2;  // Damaged
   }
   else
   {
	   if(((AircraftClass*) pCPLight->mpOwnship)->GetDOFValue(ComplexGearDOF[0]/*COMP_NOS_GEAR*/) ==  // MLR 2/22/2004 - 
		   ((AircraftClass*) pCPLight->mpOwnship)->af->GetAeroData(AeroDataSet::NosGearRng)*DTR)
		{
		   pCPLight->mState	= 1; // Down
	   }
	   else if (((AircraftClass*) pCPLight->mpOwnship)->GetDOFValue(ComplexGearDOF[0]/*COMP_NOS_GEAR)*/)) // MLR 2/22/2004 - 
		{
	 	   pCPLight->mState	= 2; // In Transit
		}
		else
		{
		   pCPLight->mState	= 0; // Up
	   }
   }
}

//Left Landing Gear light
void CBELeftLandGearLight(void * pObject) {

	CPLight	*pCPLight;

	pCPLight	= (CPLight*) pObject;

	int gear = min(1, ((AircraftClass*) pCPLight->mpOwnship)->af->NumGear());
	if(((AircraftClass*) pCPLight->mpOwnship)->af->gear[gear].flags & GearData::GearProblem)
	{
 	   pCPLight->mState	= 2;  // Damaged
	}
	else
	{
		if(((AircraftClass*) pCPLight->mpOwnship)->GetDOFValue(ComplexGearDOF[gear]/*COMP_NOS_GEAR+gear*/) ==  // MLR 2/22/2004 - 
			((AircraftClass*) pCPLight->mpOwnship)->af->GetAeroData(AeroDataSet::NosGearRng+gear*4)*DTR)
		{
		   pCPLight->mState	= 1; // Down
		}
		else if (((AircraftClass*) pCPLight->mpOwnship)->GetDOFValue(ComplexGearDOF[gear]/*COMP_NOS_GEAR+gear*/)) // MLR 2/22/2004 - 
		{
	 	   pCPLight->mState	= 2; // In Transit
		}
		else
		{
		   pCPLight->mState	= 0; // Up
		}
	}
}

//Right Landing Gear light
void CBERightLandGearLight(void * pObject) {

	CPLight	*pCPLight;

	pCPLight	= (CPLight*) pObject;

	int gear = min(2, ((AircraftClass*) pCPLight->mpOwnship)->af->NumGear());
	if(((AircraftClass*) pCPLight->mpOwnship)->af->gear[gear].flags & GearData::GearProblem)
	{
 	   pCPLight->mState	= 2;  // Damaged
	}
	else
	{
		if(((AircraftClass*) pCPLight->mpOwnship)->GetDOFValue(ComplexGearDOF[gear]/*COMP_NOS_GEAR+gear*/) ==  // MLR 2/22/2004 - 
			((AircraftClass*) pCPLight->mpOwnship)->af->GetAeroData(AeroDataSet::NosGearRng+gear*4)*DTR)
		{
		   pCPLight->mState	= 1; // Down
		}
		else if (((AircraftClass*) pCPLight->mpOwnship)->GetDOFValue(ComplexGearDOF[gear]/*COMP_NOS_GEAR+gear*/)) // MLR 2/22/2004 - 
		{
	 	   pCPLight->mState	= 2; // In Transit
		}
		else
		{
		   pCPLight->mState	= 0; // Up
		}
	}
}

void CBEHydPressA(void * pObject) {

	CPLight	*pCPLight;

	pCPLight	= (CPLight*) pObject;
    
	// JPO A circuit failure
	if(((AircraftClass*) pCPLight->mpOwnship)->af->HydraulicA()) {
		pCPLight->mState	= CPLIGHT_ON;
	}
	else{
		pCPLight->mState	= CPLIGHT_OFF;
	}
}


void CBEHydPressB(void * pObject) {

	CPLight	*pCPLight;

	pCPLight	= (CPLight*) pObject;
	
	// JPO B circuit failure
	if(((AircraftClass*) pCPLight->mpOwnship)->af->HydraulicB()) {
	    pCPLight->mState	= CPLIGHT_ON;
	}
	else{
	    pCPLight->mState	= CPLIGHT_OFF;
	}
}

// Jfs Run Light
void CBEJfsRun(void * pObject) {

	CPLight	*pCPLight;

	pCPLight	= (CPLight*) pObject;
	
	// JPO Is JFS running.
	if(((AircraftClass*) pCPLight->mpOwnship)->af->IsSet(AirframeClass::JfsStart)) {
	    pCPLight->mState	= CPLIGHT_ON;
	}
	else{
	    pCPLight->mState	= CPLIGHT_OFF;
	}
}

// EPU Run Light
void CBEEpuRun(void * pObject) {

	CPLight	*pCPLight;

	pCPLight	= (CPLight*) pObject;
	
	// JPO Is EPU running.
	if(((AircraftClass*) pCPLight->mpOwnship)->af->GeneratorRunning(AirframeClass::GenEpu)) {
	    pCPLight->mState	= CPLIGHT_ON;
	}
	else{
	    pCPLight->mState	= CPLIGHT_OFF;
	}
}

//MI Stores config light
void CBEConfigLight(void *pObject)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	CPLight	*pCPLight;

	pCPLight	= (CPLight*) pObject;

	//Check if our lightbit is set
	if (playerAC->mFaults->GetFault(stores_config_fault)){
		pCPLight->mState	= CPLIGHT_ON;
	}
	else {
		pCPLight->mState	= CPLIGHT_OFF;
	}
}

//JPO  Fwd Fuel Low caution
void CBECautionFwdFuel(void *pObject)
{
    CheckCaution1(pObject, fwd_fuel_low_fault);
}

//JPO  Aft Fuel Low caution
void CBECautionAftFuel(void *pObject)
{
    CheckCaution1(pObject, aft_fuel_low_fault);
}

//JPO  Sec engine caution
void CBECautionSec(void *pObject)
{
    CheckCaution1(pObject, sec_fault);
}

//JPO  oxy low caution
void CBECautionOxyLow(void *pObject)
{
    CheckCaution1(pObject, oxy_low_fault);
}

//JPO  probe heat caution
void CBECautionProbeHeat(void *pObject)
{
    CheckCaution1(pObject, probeheat_fault);
}

//JPO seat not armed caution
void CBECautionSeatNotArmed(void *pObject)
{
    CheckCaution1(pObject, seat_notarmed_fault);
}

//JPO  BUC caution
void CBECautionBUC(void *pObject)
{
    CheckCaution1(pObject, buc_fault);
}

//JPO  Fuel Oil too hot  caution
void CBECautionFuelOilHot(void *pObject)
{
    CheckCaution1(pObject, fueloil_hot_fault);
}

//JPO  Anti Skid caution
void CBECautionAntiSkid(void *pObject)
{
    CheckCaution1(pObject, anti_skid_fault);
}

// JPO Electrical subsytem fault
void CBECautionElectric(void *pObject)
{
    CheckCaution1(pObject, elec_fault);
}

//JPO  Main generator caution
void CBECautionMainGen(void *pObject)
{
    CPLight	*pCPLight;
    
    pCPLight	= (CPLight*) pObject;
    if(((AircraftClass*) pCPLight->mpOwnship)->af->GeneratorRunning(AirframeClass::GenMain))
	pCPLight->mState	= CPLIGHT_OFF;
    else
	pCPLight->mState	= CPLIGHT_ON;
}

//JPO  Stby Generator caution
void CBECautionStbyGen(void *pObject)
{
    CPLight	*pCPLight;
    
    pCPLight	= (CPLight*) pObject;
    if(((AircraftClass*) pCPLight->mpOwnship)->af->GeneratorRunning(AirframeClass::GenStdby))
	pCPLight->mState	= CPLIGHT_OFF;
    else
	pCPLight->mState	= CPLIGHT_ON;
}

//JPO Interior lights
void CBEInteriorLight(void *pObject)
{
    CPLight	*pCPLight;
    
    pCPLight	= (CPLight*) pObject;
    
    if(!((AircraftClass*) pCPLight->mpOwnship)->HasPower(AircraftClass::InteriorLightPower))
	pCPLight->mState	= CPLIGHT_OFF;
    else {
	switch(((AircraftClass*) pCPLight->mpOwnship)->GetInteriorLight()) {
	case AircraftClass::LT_OFF:
	    pCPLight->mState =CPLIGHT_OFF;
	    break;
	case AircraftClass::LT_LOW:
	    pCPLight->mState = 1;
	    break;
	case AircraftClass::LT_NORMAL:
	    pCPLight->mState = 2;
	    break;
	}
    }
}

//JPO Instrument lights
void CBEInstrumentLight(void *pObject)
{
    CPLight	*pCPLight;
    
    pCPLight	= (CPLight*) pObject;
    
    if(!((AircraftClass*) pCPLight->mpOwnship)->HasPower(AircraftClass::InstrumentLightPower))
	pCPLight->mState	= CPLIGHT_OFF;
    else {
	switch(((AircraftClass*) pCPLight->mpOwnship)->GetInstrumentLight()) {
	case AircraftClass::LT_OFF:
	    pCPLight->mState =CPLIGHT_OFF;
	    break;
	case AircraftClass::LT_LOW:
	    pCPLight->mState = 1;
	    break;
	case AircraftClass::LT_NORMAL:
	    pCPLight->mState = 2;
	    break;
	}
    }
}

//MI Spot lights
void CBESpotLight(void *pObject)
{
    CPLight	*pCPLight;
    
    pCPLight	= (CPLight*) pObject;
    
    if(!((AircraftClass*) pCPLight->mpOwnship)->HasPower(AircraftClass::SpotLightPower))
	pCPLight->mState	= CPLIGHT_OFF;
    else {
	switch(((AircraftClass*) pCPLight->mpOwnship)->GetSpotLight()) {
	case AircraftClass::LT_OFF:
	    pCPLight->mState =CPLIGHT_OFF;
	    break;
	case AircraftClass::LT_LOW:
	    pCPLight->mState = 1;
	    break;
	case AircraftClass::LT_NORMAL:
	    pCPLight->mState = 2;
	    break;
	}
    }
}

// whole slew of eletrical lights...
void CBEFlcsPMG(void *pObject)
{
    CPLight	*pCPLight;
    
    pCPLight	= (CPLight*) pObject;
    
    if(((AircraftClass*) pCPLight->mpOwnship)->ElecIsSet(AircraftClass::ElecFlcsPmg))
	pCPLight->mState	= CPLIGHT_ON;
    else
	pCPLight->mState	= CPLIGHT_OFF;
}

void CBEEpuGen(void *pObject)
{
    CPLight	*pCPLight;
    
    pCPLight	= (CPLight*) pObject;
    
    if(((AircraftClass*) pCPLight->mpOwnship)->ElecIsSet(AircraftClass::ElecEpuGen))
	pCPLight->mState	= CPLIGHT_ON;
    else
	pCPLight->mState	= CPLIGHT_OFF;
}

void CBEEpuPmg(void *pObject)
{
    CPLight	*pCPLight;
    
    pCPLight	= (CPLight*) pObject;
    
    if(((AircraftClass*) pCPLight->mpOwnship)->ElecIsSet(AircraftClass::ElecEpuPmg))
	pCPLight->mState	= CPLIGHT_ON;
    else
	pCPLight->mState	= CPLIGHT_OFF;
}

void CBEToFlcs(void *pObject)
{
    CPLight	*pCPLight;
    
    pCPLight	= (CPLight*) pObject;
    
    if(((AircraftClass*) pCPLight->mpOwnship)->ElecIsSet(AircraftClass::ElecToFlcs))
	pCPLight->mState	= CPLIGHT_ON;
    else
	pCPLight->mState	= CPLIGHT_OFF;
}

void CBEFlcsRly(void *pObject)
{
    CPLight	*pCPLight;
    
    pCPLight	= (CPLight*) pObject;
    
    if(((AircraftClass*) pCPLight->mpOwnship)->ElecIsSet(AircraftClass::ElecFlcsRly))
	pCPLight->mState	= CPLIGHT_ON;
    else
	pCPLight->mState	= CPLIGHT_OFF;
}

void CBEBatteryFail(void *pObject)
{
    CPLight	*pCPLight;
    
    pCPLight	= (CPLight*) pObject;
    
    if(((AircraftClass*) pCPLight->mpOwnship)->ElecIsSet(AircraftClass::ElecBatteryFail))
	pCPLight->mState	= CPLIGHT_ON;
    else
	pCPLight->mState	= CPLIGHT_OFF;
}

void CBEEpuHydrazine(void *pObject)
{
    CPLight	*pCPLight;
    
    pCPLight	= (CPLight*) pObject;
    
    if(((AircraftClass*) pCPLight->mpOwnship)->af->EpuIsHydrazine())
	pCPLight->mState	= CPLIGHT_ON;
    else
	pCPLight->mState	= CPLIGHT_OFF;
}

void CBEEpuAir(void *pObject)
{
    CPLight	*pCPLight;
    
    pCPLight	= (CPLight*) pObject;
    
    if(((AircraftClass*) pCPLight->mpOwnship)->af->EpuIsAir())
	pCPLight->mState	= CPLIGHT_ON;
    else
	pCPLight->mState	= CPLIGHT_OFF;
}
//MI TF Fail light
void CBETFFail(void *pObject)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	CPLight	*pCPLight;
    
    pCPLight	= (CPLight*) pObject;
    
	if (playerAC->RFState == 2){
		pCPLight->mState	= CPLIGHT_ON;
	}
	else {
		pCPLight->mState	= CPLIGHT_OFF;
	}
}
//for EWS panel displays
void CBEEwsPanelPower(void *pObject)
{
	//Pseudo light for the digital displays of the EWS panel. If we don't have power, nothing 
	//should be displayed
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	CPLight *pCPLight;

	pCPLight	= (CPLight*) pObject;

	if (playerAC->HasPower(AircraftClass::ChaffFlareCount)){
		pCPLight->mState	= CPLIGHT_ON;
	}
	else {
		pCPLight->mState	= CPLIGHT_OFF;
	}
}
void CBECanopyLight(void *pObject)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	CPLight*			pCPLight;
	FackClass*		faultSys;

	pCPLight	= (CPLight*) pObject;
	faultSys	= ((AircraftClass*) pCPLight->mpOwnship)->mFaults;
	int canopyopen;
	if (playerAC->IsComplex()){
	    canopyopen = playerAC->GetDOFValue(COMP_CANOPY_DOF) > 0;
	}
	else {
	    canopyopen = playerAC->GetDOFValue(SIMP_CANOPY_DOF) > 0;
	}

	if(canopyopen || (faultSys && faultSys->GetFault(canopy))){
		pCPLight->mState = CPLIGHT_ON;
	}
	else{
		pCPLight->mState = CPLIGHT_OFF;
	}
}
//MI TFR ACTIVE light
void CBETFRLight(void *pObject)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	CPLight*			pCPLight;
	
	pCPLight	= (CPLight*) pObject;
	if (playerAC->AutopilotType() == AircraftClass::LantirnAP){
		pCPLight->mState = CPLIGHT_ON;
	}
	else {
		pCPLight->mState = CPLIGHT_OFF;
	}

	// MD -- 20031011: moved to cautions.cpp so the bits are set even if the player isn't
	// looking at the cockpit view.
	//if (pCPLight->mState) 
	//	cockpitFlightData.SetLightBit2(FlightData::TFR_ENGAGED);
	//else 
	//	cockpitFlightData.ClearLightBit2(FlightData::TFR_ENGAGED);
}

void CBEGearHandleLight(void *pObject)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL || !SimDriver.GetPlayerEntity()->IsSetFlag(MOTION_OWNSHIP)){
		return;
	}

	CPLight *pCPLight	= (CPLight*) pObject;
	//MI according to the -1, the light only lights up
	//if the gear is moving
	
	if(playerAC->af->gearHandle <= 0.0F){
		//handle in the up position. Check if our gear is locked
		//or if we have a T/O LDG config warning
		//MI check for electrics
		if(playerAC->MainPower() == AircraftClass::MainPowerOff){
			pCPLight->mState = CPLIGHT_OFF;
			return;
		}
        
		if (playerAC->af->gearPos == 0.0F) //0 = gear up
		{
			if(!playerAC->mFaults->GetFault(FaultClass::gear_fault) &&			
				!playerAC->mFaults->GetFault(to_ldg_config))
			{ 
				pCPLight->mState = CPLIGHT_OFF; //Light off
			}
			else {
				pCPLight->mState = CPLIGHT_ON; //handle up and something causing our light to go on
			}
		} 
		else {
			pCPLight->mState = CPLIGHT_ON; //handle up and something causing our light to go on
		}
	}
	else if (playerAC->af->gearHandle > 0.0F) {
		//MI check for electrics
		if(playerAC->MainPower() == AircraftClass::MainPowerOff){
			pCPLight->mState = 2;
			return;
		}
		//handle down. Here it's only on if our gear isn't locked
		if (playerAC->af->gearPos == 1.0F)	//1 = gear down
		{
			if(!playerAC->mFaults->GetFault(FaultClass::gear_fault)){
					pCPLight->mState = 2; //Light off
			}
			else {
				pCPLight->mState = 3; //handle down and something causing our light to go on
			}
		}
		else {
			pCPLight->mState = 3; //handle down and something causing our light to go on
		}
	}

		// MD -- 20031011: moved to cautions.cpp so the bits are set even if the player isn't
		// looking at the cockpit view.
		//if (pCPLight->mState) 
		//	cockpitFlightData.SetLightBit2(FlightData::GEARHANDLE);
		//else 
		//	cockpitFlightData.ClearLightBit2(FlightData::GEARHANDLE);

}
//MI ADI INS Flags
void CBEADIOff(void *pObject)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	CPLight*			pCPLight;
	pCPLight	= (CPLight*) pObject;
	if(!g_bRealisticAvionics)
	{
		pCPLight->mState = CPLIGHT_OFF;
		return;
	}

	if(playerAC->INSState(AircraftClass::INS_ADI_OFF_IN)){
		pCPLight->mState = CPLIGHT_OFF;	//Flag in
		// MD -- 20031011: Move to otwloop.cpp
		//cockpitFlightData.ClearHsiBit(FlightData::ADI_OFF);
	}
	else {
		pCPLight->mState = CPLIGHT_ON;	//Flag out
		// MD -- 20031011: Move to otwloop.cpp
		//cockpitFlightData.SetHsiBit(FlightData::ADI_OFF);
	}
}
void CBEADIAux(void *pObject)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	CPLight*			pCPLight;
	pCPLight	= (CPLight*) pObject;
	if(!g_bRealisticAvionics)
	{
		pCPLight->mState = CPLIGHT_OFF;
		return;
	}

	if(playerAC->INSState(AircraftClass::INS_ADI_AUX_IN)){
		pCPLight->mState = CPLIGHT_OFF;	//Flag in
		// MD -- 20031011: Move to otwloop.cpp
		//cockpitFlightData.ClearHsiBit(FlightData::ADI_AUX);
	}
	else {
		pCPLight->mState = CPLIGHT_ON;	//Flag out
		// MD -- 20031011: Move to otwloop.cpp
		//cockpitFlightData.SetHsiBit(FlightData::ADI_AUX);
	}

}
void CBEHSIOff(void *pObject)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	CPLight*			pCPLight;
	pCPLight	= (CPLight*) pObject;
	if(!g_bRealisticAvionics)
	{
		pCPLight->mState = CPLIGHT_OFF;
		return;
	}

	if (playerAC->INSState(AircraftClass::INS_HSI_OFF_IN)){
		pCPLight->mState = CPLIGHT_OFF;	//Flag in
		// MD -- 20031011: Move to otwloop.cpp
		//cockpitFlightData.ClearHsiBit(FlightData::HSI_OFF);
	}
	else {
		pCPLight->mState = CPLIGHT_ON;	//Flag out
		// MD -- 20031011: Move to otwloop.cpp
		//cockpitFlightData.SetHsiBit(FlightData::HSI_OFF);
	}
}
//MI LE Flaps caution light
void CBELEFLight(void *pObject)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL || playerAC->mFaults == NULL){
		return;
	}

	CPLight*			pCPLight;
	pCPLight	= (CPLight*) pObject;
	if (playerAC->mFaults->GetFault(lef_fault)){
		pCPLight->mState = CPLIGHT_ON;
	}
	else{
		pCPLight->mState = CPLIGHT_OFF;
	}
}
//MI Canopy damage
void CBECanopyDamage(void *pObject)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	CPLight*			pCPLight;
	pCPLight	= (CPLight*) pObject;
	if (playerAC->CanopyDamaged){
		pCPLight->mState = CPLIGHT_ON;
	}
	else {
		pCPLight->mState = CPLIGHT_OFF;
	}
}
//MI BUP ADI Off Flag
void CBEBUPADIFlag(void *pObject)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL || playerAC->mFaults == NULL){
		return;
	}

	CPLight*			pCPLight;
	pCPLight	= (CPLight*) pObject;
	if(!g_bRealisticAvionics)
	{
		pCPLight->mState = CPLIGHT_OFF;
		return;
	}

	if (playerAC->INSState(AircraftClass::BUP_ADI_OFF_IN)){
		pCPLight->mState = CPLIGHT_OFF;	//Flag in
		// MD -- 20031011: Move to otwloop.cpp
		//cockpitFlightData.ClearHsiBit(FlightData::BUP_ADI_OFF);
	}
	else {
		pCPLight->mState = CPLIGHT_ON;	//Flag out
		// MD -- 20031011: Move to otwloop.cpp
		//cockpitFlightData.SetHsiBit(FlightData::BUP_ADI_OFF);
	}
}
//MI AVTR Run Light
void CBEAVTRRunLight(void *pObject)
{
	CPLight*			pCPLight;
	pCPLight	= (CPLight*) pObject;
	
	if(SimDriver.AVTROn())
	{
		pCPLight->mState = CPLIGHT_ON;
		// MD -- 20031011: Move to otwloop.cpp
		//cockpitFlightData.SetHsiBit(FlightData::AVTR);
	}
	else
	{
		pCPLight->mState = CPLIGHT_OFF;
		// MD -- 20031011: Move to otwloop.cpp
		//cockpitFlightData.ClearHsiBit(FlightData::AVTR);
	}
}
//MI ADI GS Flag
void CBEGSFlag(void *pObject)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	CPLight*			pCPLight;
	pCPLight	= (CPLight*) pObject;
	if(!g_bRealisticAvionics)
	{
		pCPLight->mState = CPLIGHT_OFF;
		return;
	}
	
	if (playerAC->GSValid == FALSE || playerAC->currentPower == AircraftClass::PowerNone){
		pCPLight->mState = CPLIGHT_ON;	//Flag visible
	// MD -- 20031011: Move to otwloop.cpp
	//	cockpitFlightData.SetHsiBit(FlightData::ADI_GS);
	}
	else {
		pCPLight->mState = CPLIGHT_OFF;	//Flag invisible
	// MD -- 20031011: Move to otwloop.cpp
	//	cockpitFlightData.ClearHsiBit(FlightData::ADI_GS);
	}
}
//MI ADI LOC Flag
void CBELOCFlag(void *pObject)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	CPLight*			pCPLight;
	pCPLight	= (CPLight*) pObject;
	if(!g_bRealisticAvionics)
	{
		pCPLight->mState = CPLIGHT_OFF;
		return;
	}
	
	if (playerAC->LOCValid == FALSE || playerAC->currentPower == AircraftClass::PowerNone){
		pCPLight->mState = CPLIGHT_ON;	//Flag visible
		// MD -- 20031011: Move to otwloop.cpp
		//	cockpitFlightData.SetHsiBit(FlightData::ADI_LOC);
	}
	else {
		pCPLight->mState = CPLIGHT_OFF;	//Flag invisible
		// MD -- 20031011: Move to otwloop.cpp
		//	cockpitFlightData.ClearHsiBit(FlightData::ADI_LOC);
	}

}
//MI VVI Off Flag
void CBEVVIOFF(void *pObject)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	CPLight*			pCPLight;
	pCPLight	= (CPLight*) pObject;
	
	//Off when emergency power is gone
	if (playerAC->currentPower < AircraftClass::PowerEmergencyBus){
		pCPLight->mState = CPLIGHT_ON;	//Flag visible
	// MD -- 20031011: Move to otwloop.cpp
	//	cockpitFlightData.SetHsiBit(FlightData::VVI);
	//	cockpitFlightData.SetHsiBit(FlightData::AOA);
	}
	else {
		pCPLight->mState = CPLIGHT_OFF;	//Flag invisible
	// MD -- 20031011: Move to otwloop.cpp
	//	cockpitFlightData.ClearHsiBit(FlightData::VVI);
	//	cockpitFlightData.ClearHsiBit(FlightData::AOA);
	}
}
void CBECockpitFeatures(void *pObject)
{
	CPLight*			pCPLight;
	pCPLight	= (CPLight*) pObject;

	//always on
	pCPLight->mState = CPLIGHT_ON;
}
void CBECkptWingLight(void *pObject)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	CPLight*			pCPLight;
	pCPLight	= (CPLight*) pObject;

	if (playerAC->CockpitWingLight && PlayerOptions.ObjDetailLevel > 1.0F){
		pCPLight->mState = CPLIGHT_ON;
	}
	else{
		pCPLight->mState = CPLIGHT_OFF;
	}
}
// new code for flashing winglights
void CBECkptWingLightFlash(void *pObject)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	CPLight*			pCPLight;
	pCPLight	= (CPLight*) pObject;

	if(playerAC->CockpitWingLightFlash){
		pCPLight->mState = CPLIGHT_ON;
	}
	else {
		pCPLight->mState = CPLIGHT_OFF;
	}
}
void CBECkptStrobeLight(void *pObject)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC == NULL){
		return;
	}

	CPLight*			pCPLight;
	pCPLight	= (CPLight*) pObject;

	if(playerAC->CockpitStrobeLight){
		pCPLight->mState = CPLIGHT_ON;
	}
	else{
		pCPLight->mState = CPLIGHT_OFF;
	}
}

//TJL 01/25/04
void CBELockLight(void *pObject)
{
	CPLight*			pCPLight;
	pCPLight	= (CPLight*) pObject;
	Falcon4EntityClassType*		pclassPtr=NULL;

	RadarDopplerClass*			rdc = NULL;

	rdc = (RadarDopplerClass*)FindSensor(SimDriver.GetPlayerEntity(), SensorClass::Radar);
	if(rdc)
	{
		if (rdc->CurrentTarget())
			pCPLight->mState = CPLIGHT_ON;
		else
			pCPLight->mState = CPLIGHT_OFF;
	}
}

//TJL 01/25/04
void CBEShootLight(void *pObject)
{
	
	CPLight*			pCPLight;
	pCPLight	= (CPLight*) pObject;


	if(SimDriver.GetPlayerEntity() )
	{
		if(shootCue == 1)
			pCPLight->mState = CPLIGHT_ON;
		else
			pCPLight->mState = CPLIGHT_OFF;
	}
}
//Cobra 10/31/04 TJL
// MD -- 20041008: adding a callback for the MRK BCN lamp on the center console
// which lights up different colors depending which ILS marker beacon you happen
// to be overflying.  Not totally sure about the colors but we support three
// states for now -- outer marker, middle marker and off.

void CBEMarkerBeacon(void *pObject)
{
	/*
	CPLight*			pCPLight;
	pCPLight	= (CPLight*) pObject;


	if(SimDriver.GetPlayerEntity() )
	{
		if (cockpitFlightData.IsSetHsi(FlightData::OuterMarker))
			pCPLight->mState = 1;  // blue
		else if (cockpitFlightData.IsSetHsi(FlightData::MiddleMarker))
			pCPLight->mState = 2;  // orange
		else
			pCPLight->mState = CPLIGHT_OFF;
	}*/
}