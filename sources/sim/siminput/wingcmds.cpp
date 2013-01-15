#include "stdhdr.h"
#include "aircrft.h"
#include "simdrive.h"
#include "falcsnd\conv.h"
#include "commands.h"
#include "wingorder.h"
#include "radar.h"
#include "laserpod.h"
#include "harmpod.h"
#include "object.h"
#include "sms.h"
#include "simweapn.h"
#include "missile.h"
#include "misldisp.h"
//Cobra
#include "harmpod.h"
#include "simveh.h"
#include "fcc.h"

/* S.G. SO WE CAN TARGET PADLOCK OBJECT */ #include "otwdrive.h"

VU_ID FindAircraftTarget (AircraftClass* theVehicle);
extern SensorClass* FindLaserPod (SimMoverClass* theObject);

//-----------------------------------
// Formation Commands
//-----------------------------------

// Wedge

void WingmanWedge (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMWedge, AiWingman );
	}
}

void ElementWedge (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMWedge, AiElement );
	}
}

void FlightWedge (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMWedge, AiFlight );
	}
}

// Arrow

void WingmanArrow(unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMArrowHead, AiWingman );
	}
}

void ElementArrow (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMArrowHead, AiElement );
	}
}

void FlightArrow (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMArrowHead, AiFlight );
	}
}

// Box

void WingmanBox(unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMBox, AiWingman );
	}
}

void ElementBox (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMBox, AiElement );
	}
}

void FlightBox (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMBox, AiFlight );
	}
}


// Res Cell

void WingmanResCell (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMResCell, AiWingman );
	}
}

void ElementResCell (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMResCell, AiElement );
	}
}

void FlightResCell (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMResCell, AiFlight );
	}
}

// Trail

void WingmanTrail (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMTrail, AiWingman );
	}
}

void ElementTrail (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMTrail, AiElement );
	}
}

void FlightTrail (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMTrail, AiFlight );
	}
}



// Spread

void WingmanSpread (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMSpread, AiWingman );
	}
}

void ElementSpread (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMSpread, AiElement );
	}
}

void FlightSpread (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMSpread, AiFlight );
	}
}

// Fluid 4
void WingmanFluid (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMFluidFour, AiWingman );
	}
}

void ElementFluid (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMFluidFour, AiElement );
	}
}

void FlightFluid (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMFluidFour, AiFlight );
	}
}


// Ladder
void WingmanLadder (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMLadder, AiWingman );
	}
}

void ElementLadder (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMLadder, AiElement );
	}
}

void FlightLadder (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMLadder, AiFlight );
	}
}

// Stack

void WingmanStack (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMStack, AiWingman );
	}
}

void ElementStack (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMStack, AiElement );
	}
}

void FlightStack (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMStack, AiFlight );
	}
}


// Vic

void WingmanVic (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMVic, AiWingman );
	}
}

void ElementVic (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMVic, AiElement );
	}
}

void FlightVic (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMVic, AiFlight );
	}
}


// Finger 4

void WingmanFinger4 (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMFinger4, AiWingman );
	}
}

void ElementFinger4 (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMFinger4, AiElement );
	}
}

void FlightFinger4 (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMFinger4, AiFlight );
	}
}


// Echelon

void WingmanEchelon (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMEchelon, AiWingman);
	}
}

void ElementEchelon (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMEchelon, AiElement );
	}
}

void FlightEchelon (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMEchelon, AiFlight );
	}
}

// Form1 placeholder

void WingmanForm1 (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMForm1, AiWingman );
	}
}

void ElementForm1 (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMForm1, AiElement );
	}
}

void FlightForm1 (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMForm1, AiFlight );
	}
}

// Form2 placeholder

void WingmanForm2 (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMForm2, AiWingman );
	}
}

void ElementForm2 (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMForm2, AiElement );
	}
}

void FlightForm2 (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMForm2, AiFlight );
	}
}

// Form3 placeholder

void WingmanForm3 (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMForm3, AiWingman );
	}
}

void ElementForm3 (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMForm3, AiElement );
	}
}

void FlightForm3 (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMForm3, AiFlight );
	}
}

// Form4 placeholder

void WingmanForm4 (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMForm4, AiWingman );
	}
}

void ElementForm4 (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMForm4, AiElement );
	}
}

void FlightForm4 (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMForm4, AiFlight );
	}
}

//-----------------------------------
// Formation Adjustments
//-----------------------------------


void WingmanKickout (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMKickout, AiWingman );
	}
}

void ElementKickout (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMKickout, AiElement );
	}
}

void FlightKickout (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMKickout, AiFlight );
	}
}

//--

void WingmanCloseup (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMCloseup, AiWingman );
	}
}

void ElementCloseup (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMCloseup, AiElement );
	}
}

void FlightCloseup (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMCloseup, AiFlight );
	}
}

//--
void WingmanToggleSide (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMToggleSide, AiWingman );
	}
}

void ElementToggleSide (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMToggleSide, AiElement );
	}
}

void FlightToggleSide (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMToggleSide, AiFlight );
	}
}

//--

void WingmanIncreaseRelAlt (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMIncreaseRelAlt, AiWingman );
	}
}

void ElementIncreaseRelAlt (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMIncreaseRelAlt, AiElement );
	}
}

void FlightIncreaseRelAlt (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMIncreaseRelAlt, AiFlight );
	}
}

//--

void WingmanDecreaseRelAlt (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMDecreaseRelAlt, AiWingman );
	}
}

void ElementDecreaseRelAlt (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMDecreaseRelAlt, AiElement );
	}
}

void FlightDecreaseRelAlt (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMDecreaseRelAlt, AiFlight );
	}
}

//--

//-----------------------------------
// Action commands
//-----------------------------------

void WingmanDesignateTarget (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
      VU_ID tgtId = FalconNullId;

      tgtId = FindAircraftTarget(SimDriver.GetPlayerAircraft());

		AiSendPlayerCommand( FalconWingmanMsg::WMAssignTarget, AiWingman, tgtId );
	}
}

void ElementDesignateTarget (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
      VU_ID tgtId = FalconNullId;

      tgtId = FindAircraftTarget(SimDriver.GetPlayerAircraft());

		AiSendPlayerCommand( FalconWingmanMsg::WMAssignTarget, AiElement, tgtId );
	}
}

void FlightDesignateTarget (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
      VU_ID tgtId = FalconNullId;

      tgtId = FindAircraftTarget(SimDriver.GetPlayerAircraft());

		AiSendPlayerCommand( FalconWingmanMsg::WMAssignTarget, AiFlight, tgtId );
	}
}


void WingmanDesignateGroup (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
      VU_ID tgtId = FalconNullId;

      tgtId = FindAircraftTarget(SimDriver.GetPlayerAircraft());

		AiSendPlayerCommand( FalconWingmanMsg::WMAssignGroup, AiWingman, tgtId );
	}
}

void ElementDesignateGroup (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
      VU_ID tgtId = FalconNullId;

      tgtId = FindAircraftTarget(SimDriver.GetPlayerAircraft());

		AiSendPlayerCommand( FalconWingmanMsg::WMAssignGroup, AiElement, tgtId );
	}
}

void FlightDesignateGroup (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
      VU_ID tgtId = FalconNullId;

      tgtId = FindAircraftTarget(SimDriver.GetPlayerAircraft());

		AiSendPlayerCommand( FalconWingmanMsg::WMAssignGroup, AiFlight, tgtId );
	}
}

VU_ID FindAircraftTarget (AircraftClass* theAC)
{
VU_ID tgtId = FalconNullId;
MissileClass* theMissile = NULL;
SensorClass* mslDisplay = NULL;
SensorClass* tPodDisplay = tPodDisplay = FindLaserPod (theAC);;
RadarClass* theRadar = (RadarClass*) FindSensor (theAC, SensorClass::Radar);

// 2000-11-15 ADDED BY S.G. SO PADLOCKED OBJECT CAN BE TARGETED FIRST
	if (OTWDriver.mpPadlockPriorityObject) {
		if (!OTWDriver.mpPadlockPriorityObject->IsMissile()) {
			tgtId = OTWDriver.mpPadlockPriorityObject->Id();
			if (tgtId != FalconNullId) {
				return (tgtId);
			}
		}
	}
// END OF ADDED SECTION

	if (theAC->Sms)
		theMissile = (MissileClass*)theAC->Sms->GetCurrentWeapon();

   if (theMissile && theMissile->IsMissile())
   {
      mslDisplay = (SensorClass*)theMissile->display;
   }

   if (tPodDisplay && tPodDisplay->IsSOI())
   {
      tgtId = tPodDisplay->TargetUnderCursor();
      if (tgtId == FalconNullId)
      {
         if (tPodDisplay->CurrentTarget())
            tgtId = tPodDisplay->CurrentTarget()->BaseData()->Id();
      }
   }

   //Cobra need to separate out HTS so it still targets after all 88's fired
   HarmTargetingPod *theHtS = (HarmTargetingPod*)FindSensor(SimDriver.GetPlayerAircraft(), SensorClass::HTS);
	   if (!mslDisplay && theHtS)
		   {
			tgtId = theHtS->FindIDUnderCursor();
			if (tgtId == FalconNullId && theHtS->CurrentTarget())
				{
				tgtId = theHtS->CurrentTarget()->BaseData()->Id();
				}
		   }

   if (tgtId == FalconNullId && mslDisplay && mslDisplay->IsSOI())
   {
      if (mslDisplay->Type() == SensorClass::HTS)
      {
         tgtId = ((HarmTargetingPod*)mslDisplay)->FindIDUnderCursor();
         if (tgtId == FalconNullId)
         {
            if (mslDisplay->CurrentTarget())
               tgtId = mslDisplay->CurrentTarget()->BaseData()->Id();
         }
      }
      else
      {
         if (((MissileDisplayClass*)mslDisplay)->DisplayType() == MissileDisplayClass::AGM65_IR ||
            ((MissileDisplayClass*)mslDisplay)->DisplayType() == MissileDisplayClass::AGM65_TV)
         {
            if (theMissile->targetPtr)
               tgtId = theMissile->targetPtr->BaseData()->Id();
         }
      }
   }

   if (tgtId == FalconNullId && theRadar)
   {
      tgtId = theRadar->TargetUnderCursor();
      if (tgtId == FalconNullId)
      {
         if (theRadar->CurrentTarget())
            tgtId = theRadar->CurrentTarget()->BaseData()->Id();
      }
   }

   return tgtId;
}

//--

void WingmanPince (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMPince, AiWingman );
	}
}

void ElementPince (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMPince, AiElement );
	}
}

void FlightPince (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMPince, AiFlight );
	}
}

//--

void FlightBreakRight (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMBreakRight, AiFlight );
	}
}


void ElementBreakRight (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMBreakRight, AiElement );
	}
}

void WingmanBreakRight (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMBreakRight, AiWingman );
	}
}

//--

void FlightBreakLeft (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMBreakLeft, AiFlight );
	}
}

void ElementBreakLeft (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMBreakLeft, AiElement );
	}
}

void WingmanBreakLeft (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMBreakLeft, AiWingman );
	}
}

//--

void WingmanPosthole (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMPosthole, AiWingman );
	}
}

void ElementPosthole (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMPosthole, AiElement );
	}
}

void FlightPosthole (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMPosthole, AiFlight );
	}
}

//--

void WingmanChainsaw (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMChainsaw, AiWingman );
	}
}

void ElementChainsaw (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMChainsaw, AiElement );
	}
}

void FlightChainsaw (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMChainsaw, AiFlight );
	}
}

//--

void WingmanFlex (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMFlex, AiWingman );
	}
}

void ElementFlex (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMFlex, AiElement );
	}
}

void FlightFlex (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMFlex, AiFlight );
	}
}

//--

void WingmanClearSix (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMClearSix, AiWingman );
	}
}

void ElementClearSix (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMClearSix, AiElement );
	}
}

void FlightClearSix (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMClearSix, AiFlight );
	}
}


void WingmanCheckSix (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMCheckSix, AiWingman );
	}
}

void ElementCheckSix (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMCheckSix, AiElement );
	}
}

void FlightCheckSix (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMCheckSix, AiFlight );
	}
}


//----------------------------
// Other Commands
//----------------------------

void WingmanRejoin (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMRejoin, AiWingman );
	}
}

void ElementRejoin (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMRejoin, AiElement );
	}
}

void FlightRejoin (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMRejoin, AiFlight );
	}
}

//--

void WingmanGiveBra (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMGiveBra, AiWingman );
	}
}

void ElementGiveBra (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMGiveBra, AiElement );
	}
}

void FlightGiveBra (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMGiveBra, AiFlight );
	}
}

//--

void WingmanRTB (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMRTB, AiWingman );
	}
}

void ElementRTB (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMRTB, AiElement );
	}
}

void FlightRTB (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMRTB, AiFlight );
	}
}

//--

void WingmanGiveStatus (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMGiveStatus, AiWingman );
	}
}

void ElementGiveStatus (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMGiveStatus, AiElement );
	}
}

void FlightGiveStatus (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMGiveStatus, AiFlight );
	}
}

//--

void WingmanGiveDamageReport (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMGiveDamageReport, AiWingman );
	}
}

void ElementGiveDamageReport (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMGiveDamageReport, AiElement );
	}
}

void FlightGiveDamageReport (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMGiveDamageReport, AiFlight );
	}
}

//--

void WingmanGiveFuelState (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMGiveFuelState, AiWingman );
	}
}

void ElementGiveFuelState (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMGiveFuelState, AiElement );
	}
}
void FlightGiveFuelState (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMGiveFuelState, AiFlight );
	}
}

//--

void WingmanGiveWeaponsCheck (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMGiveWeaponsCheck, AiWingman );
	}
}

void ElementGiveWeaponsCheck (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMGiveWeaponsCheck, AiElement );
	}
}

void FlightGiveWeaponsCheck (unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMGiveWeaponsCheck, AiFlight );
	}
}

// M.N.
void WingmanDropStores (unsigned long, int state, void*)
{
	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft()))
	{
		AiSendPlayerCommand( FalconWingmanMsg::WMDropStores, AiWingman );
	}
}

void ElementDropStores (unsigned long, int state, void*)
{
	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft()))
	{
		AiSendPlayerCommand( FalconWingmanMsg::WMDropStores, AiElement );
	}
}

void FlightDropStores (unsigned long, int state, void*)
{
	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft()))
	{
		AiSendPlayerCommand( FalconWingmanMsg::WMDropStores, AiFlight );
	}
}

//------------------------
// Flight Duty Commands
//------------------------

void WingmanGoCoverMode(unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMCoverMode, AiWingman );
	}
}

void ElementGoCoverMode(unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMCoverMode, AiElement );
	}
}

void FlightGoCoverMode(unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMCoverMode, AiFlight );
	}
}



void WingmanGoShooterMode(unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMShooterMode, AiWingman );
	}
}

void ElementGoShooterMode(unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMShooterMode, AiElement );
	}
}

void FlightGoShooterMode(unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMShooterMode, AiFlight );
	}
}



//--

void WingmanWeaponsHold(unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMWeaponsHold, AiWingman );
	}
}

void ElementWeaponsHold(unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMWeaponsHold, AiElement );
	}
}

void FlightWeaponsHold(unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMWeaponsHold, AiFlight );
	}
}


void WingmanWeaponsFree(unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMWeaponsFree, AiWingman );
	}
}

void ElementWeaponsFree(unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMWeaponsFree, AiElement );
	}
}

void FlightWeaponsFree(unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMWeaponsFree, AiFlight );
	}
}


//--

void WingmanSearchAir(unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMSearchAir, AiWingman );
	}
}

void ElementSearchAir(unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMSearchAir, AiElement );
	}
}

void FlightSearchAir(unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMSearchAir, AiFlight );
	}
}


void WingmanSearchGround(unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMSearchGround, AiWingman );
	}
}

void ElementSearchGround(unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMSearchGround, AiElement );
	}
}

void FlightSearchGround(unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMSearchGround, AiFlight );
	}
}

//--

void WingmanResumeNormal(unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMResumeNormal, AiWingman );
	}
}

void ElementResumeNormal(unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMResumeNormal, AiElement );
	}
}

void FlightResumeNormal(unsigned long, int state, void*) {

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft())){
		AiSendPlayerCommand( FalconWingmanMsg::WMResumeNormal, AiFlight );
	}
}


