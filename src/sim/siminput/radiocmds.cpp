#include "stdhdr.h"
#include "commands.h"

// OW FIXME: this had to be added after installing the DX8 Beta 1 SDK
//Retro_dead 15Jan2004	#define DIRECTINPUT_VERSION 0x0700
//Retro_dead 15Jan2004	#include "dinput.h"

#include "otwdrive.h"
#include "simdrive.h"
#include "aircrft.h"
#include "falcsess.h"
#include "msginc\awacsmsg.h"
#include "msginc\facmsg.h"
#include "msginc\atcmsg.h"
#include "msginc\tankermsg.h"
#include "wingorder.h"
#include "mesg.h"
#include "airunit.h"
#include "tacan.h"
#include "navsystem.h"
#include "popmenu.h"
#include "fsound.h"
#include "falcsnd\voicemanager.h"
#include "cpmanager.h"
#include "airframe.h"
#include "drawbsp.h" // sfr: for 3d pit stuff

extern bool g_bRealisticAvionics;

void OTWRadioMenuStep (unsigned long, int state, void*)
{
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered)
	{
		OTWDriver.pMenuManager->StepNextPage(state);
	}
}

void OTWRadioMenuStepBack (unsigned long, int state, void*)
{
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered)
	{
		OTWDriver.pMenuManager->StepPrevPage(state);
	}
}


void RadioMessageSend (unsigned long val, int state, void*)
{
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered)
	{
		OTWDriver.pMenuManager->ProcessInput(val, state, 0);
	}
}

// MD -- 20031122: Making adjustment to the following function because the control should
// only ever change the UHF channel.  [if the ICP is broken, there is no way to change VHF
// channel!].  But it should only change the channel when the CNI switch is in "backup".
// of course the encoder knob physically moves either way.
// So sayeth the dash one.
// Leave it as it was for folks using the simplified avionics.

void SimCycleRadioChannel(unsigned long, int state, void*) {
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered)
	{
		if(!g_bRealisticAvionics) 
		{
			if(OTWDriver.pCockpitManager->mpIcp->GetICPTertiaryMode() == COMM1_MODE) 
			{
				VM->ForwardCycleFreq(0);
			}
			else 
			{
				VM->ForwardCycleFreq(1);
			}
			OTWDriver.pCockpitManager->mpIcp->SetICPUpdateFlag(CNI_UPDATE);
			OTWDriver.pCockpitManager->mMiscStates.StepUHFPostion();
		}
		else
			if (gNavigationSys->GetUHFSrc() == NavigationSystem::UHF_BACKUP) 
			{
				VM->ForwardCycleFreq(0);

				if(OTWDriver.pCockpitManager->mpIcp->IsICPSet(ICPClass::MODE_CNI) &&
					OTWDriver.pCockpitManager->mpIcp->GetICPSecondaryMode() == NONE_MODE)
				{
					OTWDriver.pCockpitManager->mpIcp->ClearStrings();
				}

				OTWDriver.pCockpitManager->mpIcp->SetICPUpdateFlag(CNI_UPDATE);
				OTWDriver.pCockpitManager->mMiscStates.StepUHFPostion();

				//int val = OTWDriver.pCockpitManager->mMiscStates.mUHFPosition +1;
				int val = 1<< OTWDriver.pCockpitManager->mMiscStates.mUHFPosition;
				if (OTWDriver.GetVirtualCockpit())
					OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_UHF_CH, val);
			}
	}
}

// MD -- 20031024: Adding a decrement function so that cockpit builders
// who have physical knobs, or more likely encoders, can have the radio
// channel go up and down depending on direction of control rotation.
// 20031122: and now make sure that the switch only changes the UHF channel
// following the operation of the real jet (see above SimCycleRadioChannel).

void SimDecRadioChannel(unsigned long, int state, void*) {
	if(!g_bRealisticAvionics)
		return;

	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered)
	{
		if (gNavigationSys->GetUHFSrc() == NavigationSystem::UHF_BACKUP) 
		{
			VM->BackwardCycleFreq(0);

			if(OTWDriver.pCockpitManager->mpIcp->IsICPSet(ICPClass::MODE_CNI) &&
				OTWDriver.pCockpitManager->mpIcp->GetICPSecondaryMode() == NONE_MODE)
			{
				OTWDriver.pCockpitManager->mpIcp->ClearStrings();
			}
			OTWDriver.pCockpitManager->mpIcp->SetICPUpdateFlag(CNI_UPDATE);
			OTWDriver.pCockpitManager->mMiscStates.DecUHFPosition();

			//int val = OTWDriver.pCockpitManager->mMiscStates.mUHFPosition +1;
			int val = 1<< OTWDriver.pCockpitManager->mMiscStates.mUHFPosition;
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_UHF_CH, val);
		}
	}
}

void SimToggleRadioVolume(unsigned long, int state, void*) {
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered) 
	{
		//		SimDriver.GetPlayerAircraft()->Radio->ToggleVolume();
	}
}

void RadioTankerCommand (unsigned long val, int state, void*)
{
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered) 
	{
		OTWDriver.pMenuManager->ProcessInput(val, state, TankerMsg);
	}
}

void RadioTowerCommand (unsigned long val, int state, void*)
{
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered) {
		OTWDriver.pMenuManager->ProcessInput(val, state, ATCMsg);
	}
}

void RadioAWACSCommand (unsigned long val, int state, void*)
{
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered) {
		OTWDriver.pMenuManager->ProcessInput(val, state, AWACSMsg);
	}
}


void RadioWingCommand (unsigned long val, int state, void*)
{
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered) {
		OTWDriver.pMenuManager->ProcessInput(val, state, WingmanMsg, AiWingman);
	}
}


void RadioElementCommand (unsigned long val, int state, void*)
{
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered) {
		OTWDriver.pMenuManager->ProcessInput(val, state, WingmanMsg, AiElement);
	}
}


void RadioFlightCommand (unsigned long val, int state, void*)
{
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered) {
		OTWDriver.pMenuManager->ProcessInput(val, state, WingmanMsg, AiFlight);
	}
}

// Direct access to ATC commands
void ATCRequestClearance (unsigned long, int state, void*)
{
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered)
   {
      MenuSendAtc (FalconATCMessage::RequestClearance);
   }
}

void ATCRequestEmergencyClearance (unsigned long, int state, void*)
{
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered)
   {
      MenuSendAtc (FalconATCMessage::RequestEmerClearance);
   }
}

void ATCRequestTakeoff (unsigned long, int state, void*)
{
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered)
   {
      MenuSendAtc (FalconATCMessage::RequestTakeoff);
   }
}

void ATCRequestTaxi (unsigned long, int state, void*)
{
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered)
   {
      MenuSendAtc (FalconATCMessage::RequestTaxi);
   }
}

void ATCTaxiing (unsigned long, int state, void*)
{
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered)
   {
      //MenuSendAtc (FalconATCMessage::Taxiing + 1);
   }
}

void ATCReadyToGo (unsigned long, int state, void*)
{
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered)
   {
      //MenuSendAtc (FalconATCMessage::ReadyToGo + 1);
   }
}

void ATCRotate (unsigned long, int state, void*)
{
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered)
   {
      //MenuSendAtc (FalconATCMessage::Rotate + 1);
   }
}

void ATCGearUp (unsigned long, int state, void*)
{
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered)
   {
      //MenuSendAtc (FalconATCMessage::GearUp + 1);
   }
}

void ATCGearDown (unsigned long, int state, void*)
{
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered)
   {
      //MenuSendAtc (FalconATCMessage::GearDown + 1);
   }
}

void ATCBrake (unsigned long, int state, void*)
{
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered)
   {
      //MenuSendAtc (FalconATCMessage::Brake + 1);
   }
}

// M.N. 2001-12-20
void ATCAbortApproach (unsigned long, int state, void*)
{
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered)
   {
      MenuSendAtc (FalconATCMessage::AbortApproach);
   }
}



// Direct access to FAC commands
void FACCheckIn (unsigned long, int state, void*)
{
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered)
   {
//      SendRadioMenuMsg (FalconFACMessage::CheckIn + 1, FACMsg, 0);
   }
}

void FACWilco (unsigned long, int state, void*)
{
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered)
   {
//      SendRadioMenuMsg (FalconFACMessage::CheckIn + 1, FACMsg, 0);
   }
}

void FACUnable (unsigned long, int state, void*)
{
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered)
   {
//      SendRadioMenuMsg (FalconFACMessage::CheckIn + 1, FACMsg, 0);
   }
}

void FACReady (unsigned long, int state, void*)
{
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered)
   {
//      SendRadioMenuMsg (FalconFACMessage::CheckIn + 1, FACMsg, 0);
   }
}

void FACIn (unsigned long, int state, void*)
{
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered)
   {
///      SendRadioMenuMsg (FalconFACMessage::In + 1, FACMsg, 0);
   }
}

void FACOut (unsigned long, int state, void*)
{
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered)
   {
///      SendRadioMenuMsg (FalconFACMessage::Out + 1, FACMsg, 0);
   }
}

void FACRequestMark (unsigned long, int state, void*)
{
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered)
   {
//      SendRadioMenuMsg (FalconFACMessage::RequestMark + 1, FACMsg, 0);
   }
}

void FACRequestTarget (unsigned long, int state, void*)
{
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered)
   {
//      SendRadioMenuMsg (FalconFACMessage::RequestTarget + 1, FACMsg, 0);
   }
}

void FACRequestBDA (unsigned long, int state, void*)
{
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered)
   {
//      SendRadioMenuMsg (FalconFACMessage::RequestBDA + 1, FACMsg, 0);
   }
}

void FACRequestLocation (unsigned long, int state, void*)
{
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered)
   {
//      SendRadioMenuMsg (FalconFACMessage::RequestLocation + 1, FACMsg, 0);
   }
}

void FACRequestTACAN (unsigned long, int state, void*)
{
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered)
   {
//      SendRadioMenuMsg (FalconFACMessage::RequestTACAN + 1, FACMsg, 0);
   }
}


// Direct access to Tanker commands
void TankerRequestFuel (unsigned long, int state, void*)
{
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered)
   {
      MenuSendTanker (FalconTankerMessage::RequestFuel);
   }
}

void TankerReadyForGas (unsigned long, int state, void*)
{
    if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered)
    {
	// JPO - requires the door to be opened.
	if (g_bRealisticAvionics &&
	    !SimDriver.GetPlayerAircraft()->af->IsEngineFlag(AirframeClass::FuelDoorOpen))
	    return;
	
	MenuSendTanker (FalconTankerMessage::ReadyForGas);
    }
}

void TankerDoneRefueling (unsigned long, int state, void*)
{
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered)
   {
      MenuSendTanker (FalconTankerMessage::DoneRefueling);
   }
}

void TankerBreakaway (unsigned long, int state, void*)
{
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered)
   {
      MenuSendTanker (FalconTankerMessage::Breakaway);
   }
}


// Direct access to AWACS commands
void AWACSRequestPicture (unsigned long, int state, void*)
{
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered)
   {
      MenuSendAwacs (FalconAWACSMessage::RequestPicture, SimDriver.GetPlayerAircraft()->Id());
   }
}

void AWACSRequestTanker (unsigned long, int state, void*)
{
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered)
   {
      MenuSendAwacs (FalconAWACSMessage::VectorToTanker, SimDriver.GetPlayerAircraft()->Id());
   }
}

// MN Carrier
void AWACSRequestCarrier (unsigned long, int state, void*)
{
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered)
   {
      MenuSendAwacs (FalconAWACSMessage::VectorToCarrier, SimDriver.GetPlayerAircraft()->Id());
   }
}


void AWACSWilco (unsigned long, int state, void*)
{
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered)
   {
      MenuSendAwacs (FalconAWACSMessage::Wilco, SimDriver.GetPlayerAircraft()->Id());
   }
}

void AWACSUnable (unsigned long, int state, void*)
{
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered)
   {
      MenuSendAwacs (FalconAWACSMessage::Unable, SimDriver.GetPlayerAircraft()->Id());
   }
}

void AWACSRequestHelp (unsigned long, int state, void*)
{
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered)
   {
      MenuSendAwacs (FalconAWACSMessage::RequestHelp, SimDriver.GetPlayerAircraft()->Id());
   }
}

void AWACSRequestRelief (unsigned long, int state, void*)
{
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered)
   {
      MenuSendAwacs (FalconAWACSMessage::RequestRelief, SimDriver.GetPlayerAircraft()->Id());
   }
}

