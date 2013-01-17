#include "stdhdr.h"
#include "digi.h"
#include "mesg.h"
#include "simveh.h"
#include "MsgInc\WingmanMsg.h"
#include "find.h"
#include "flight.h"
#include "wingorder.h"
#include "classtbl.h"
#include "Aircrft.h"

#include "stdhdr.h"
#include "simbase.h"
#include "simdrive.h"
#include "find.h"
#include "flight.h"
#include "falcsess.h"
#include "simmover.h"
#include "wingorder.h"
#include "object.h"
#include "find.h"
#include "otwdrive.h"
#include "cpmanager.h"
#include "icp.h"
#include "fsound.h"
#include "falcsnd\voicemanager.h"
#include "radar.h"
#include "airframe.h"
#include "PlayerOp.h"

extern int g_nChatterInterval; // FRB message interval time
// ---------------------------------------------
//
// AiMakeRadioCall
//
// ---------------------------------------------

void AiMakeRadioCall( SimBaseClass* p_sender,
							 int command,
							 int extent,
							 VU_ID targetid )
{
	FalconRadioChatterMessage*	p_radioMsgs = NULL;

	AiCreateRadioMsgs( p_sender, &p_radioMsgs ); // Create radio msgs for flight
	ShiAssert(p_radioMsgs);
	AiCustomizeRadioMsg( p_sender, extent, &p_radioMsgs, command, targetid );
	
	//if the message was set to -1 we don't want to play anything
	if(p_radioMsgs->dataBlock.message == -1)
	{
		delete p_radioMsgs;
		return;
	}
	else
	{
		ShiAssert(p_radioMsgs->dataBlock.message >= 0 && p_radioMsgs->dataBlock.message < LastComm);
		FalconSendMessage( p_radioMsgs, FALSE );
	}

}


// ---------------------------------------------
//
// AiMakeRadioResponse
//
// ---------------------------------------------

void AiMakeRadioResponse( SimBaseClass* p_sender,
							 int message,
							 short* p_edata )
{
	FalconRadioChatterMessage*		p_radioMsgs;

	AiCreateRadioMsgs( p_sender, &p_radioMsgs ); // Create radio msgs for flight

	p_radioMsgs->dataBlock.message	= message;

   switch (message)
   {
      case rcSAM:
      case rcINBOUND:
	      p_radioMsgs->dataBlock.time_to_play	= 0;
      break;

      default:
	  if (PlayerOptions.PlayerRadioVoice)
	      //p_radioMsgs->dataBlock.time_to_play	= g_nChatterInterval * CampaignSeconds;
	      p_radioMsgs->dataBlock.time_to_play	= 4000;
	  else
	      p_radioMsgs->dataBlock.time_to_play	= 500;
      break;
   }

	p_radioMsgs->dataBlock.edata[0]	= p_edata[0];
	p_radioMsgs->dataBlock.edata[1]	= p_edata[1];
	p_radioMsgs->dataBlock.edata[2]	= p_edata[2];
	p_radioMsgs->dataBlock.edata[3]	= p_edata[3];
	p_radioMsgs->dataBlock.edata[4]	= p_edata[4];
	p_radioMsgs->dataBlock.edata[5]	= p_edata[5];
	p_radioMsgs->dataBlock.edata[6]	= p_edata[6];
	p_radioMsgs->dataBlock.edata[7]	= p_edata[7];
	p_radioMsgs->dataBlock.edata[8]	= p_edata[8];
	p_radioMsgs->dataBlock.edata[9]	= p_edata[9];

	FalconSendMessage( p_radioMsgs, FALSE );
}

// --------------------------------------------------------------------
//
// AiCreateRadioMsgs
//
// --------------------------------------------------------------------

void AiCreateRadioMsgs( SimBaseClass* p_sender,
							  FalconRadioChatterMessage** pp_radioMsgs )
{
	int	filter;

	*pp_radioMsgs								= new FalconRadioChatterMessage( p_sender->GetCampaignObject()->Id(), FalconLocalGame );	// Create new message


	(*pp_radioMsgs)->dataBlock.voice_id	= ((FlightClass*) p_sender->GetCampaignObject())->GetPilotVoiceID(p_sender->GetCampaignObject()->GetComponentIndex(p_sender));
	(*pp_radioMsgs)->dataBlock.from		= p_sender->Id();


	if(p_sender == SimDriver.GetPlayerEntity()) {
		if(OTWDriver.pCockpitManager->mpIcp->GetICPTertiaryMode() == COMM1_MODE) {
			filter = VM->GetRadioFreq(0);
		}
		else {
			filter = VM->GetRadioFreq(1);
		}

		switch(filter) {
			case rcfOff:
				(*pp_radioMsgs)->dataBlock.to	= MESSAGE_FOR_AIRCRAFT;
				break;

			case rcfFlight5:
			case rcfFlight1:
			case rcfFlight2:
			case rcfFlight3:
			case rcfFlight4:
				(*pp_radioMsgs)->dataBlock.to	= MESSAGE_FOR_FLIGHT;
				break;

			case rcfPackage5:
			case rcfPackage1:
			case rcfPackage2:
			case rcfPackage3:
			case rcfPackage4:
			
			case rcfFromPackage:
				(*pp_radioMsgs)->dataBlock.to	= MESSAGE_FOR_PACKAGE;
				break;

			case rcfProx:
				(*pp_radioMsgs)->dataBlock.to	= MESSAGE_FOR_PACKAGE;
				break;

			case rcfTeam:
				(*pp_radioMsgs)->dataBlock.to	= MESSAGE_FOR_TEAM;
				break;

			case rcfAll:
				(*pp_radioMsgs)->dataBlock.to	= MESSAGE_FOR_WORLD;
				break;

			case rcfTower:
				(*pp_radioMsgs)->dataBlock.to	= MESSAGE_FOR_TEAM;
				break;
		} 
	}
	else {
		(*pp_radioMsgs)->dataBlock.to		= MESSAGE_FOR_FLIGHT;	// Digi's send message to the flight
	}	
}


// --------------------------------------------------------------------
//
// AiFillCallsign
//
// --------------------------------------------------------------------


void AiFillCallsign(SimBaseClass* p_sender, int extent, FalconRadioChatterMessage** pp_radioMsgs, BOOL fillCallName)
{
	
	int flightIdx;

	flightIdx	= p_sender->GetCampaignObject()->GetComponentIndex(p_sender);

	if(flightIdx == AiFlightLead) {
		if(extent == AiWingman) {
			if(fillCallName) {
				(*pp_radioMsgs)->dataBlock.edata[0] = ((FlightClass*)p_sender->GetCampaignObject())->callsign_id;
			}
			else {
				(*pp_radioMsgs)->dataBlock.edata[0] = -1;
			}
			(*pp_radioMsgs)->dataBlock.edata[1] = 4 * (((FlightClass*)p_sender->GetCampaignObject())->callsign_num - 1) + 2;
		}
		else if(extent == AiElement) {
			if(fillCallName) {
				(*pp_radioMsgs)->dataBlock.edata[0] = ((FlightClass*)p_sender->GetCampaignObject())->callsign_id;
			}
			else {
				(*pp_radioMsgs)->dataBlock.edata[0] = -1;
			}
			(*pp_radioMsgs)->dataBlock.edata[1] = 4 * (((FlightClass*)p_sender->GetCampaignObject())->callsign_num - 1) + 3;
		}
		else {
			(*pp_radioMsgs)->dataBlock.edata[0] = ((FlightClass*)p_sender->GetCampaignObject())->callsign_id;
			(*pp_radioMsgs)->dataBlock.edata[1] = ((FlightClass*)p_sender->GetCampaignObject())->callsign_num + CALLSIGN_NUM_OFFSET;
		}
	}
	else if(flightIdx == AiElementLead) {
		if(fillCallName) {
			(*pp_radioMsgs)->dataBlock.edata[0] = ((FlightClass*)p_sender->GetCampaignObject())->callsign_id;
		}
		else {
			(*pp_radioMsgs)->dataBlock.edata[0] = -1;
		}
		(*pp_radioMsgs)->dataBlock.edata[1] = 4 * (((FlightClass*)p_sender->GetCampaignObject())->callsign_num - 1) + 4;
	}
}



// ---------------------------------------------
//
// AiDispatchRadioMsgs
//
// ---------------------------------------------

void AiDispatchRadioMsgs( FalconRadioChatterMessage*** ppp_radioMsgs,
								  int numRadioRecipients )
{
	int i;

	if( numRadioRecipients ) {
		
		for ( i = 0; i < numRadioRecipients; i++ ) {							// Send the commmand to all recipients.
			FalconSendMessage( (*ppp_radioMsgs)[i], FALSE );
		}

		delete [] *ppp_radioMsgs;
	}
}




// ---------------------------------------------
//
// AiCheckExtent
//
// ---------------------------------------------
BOOL	AiIsFullResponse(int flightIdx, int extent) 
{


	// If I'm the leader's wingman
	if(flightIdx == AiFirstWing) {

		if(extent == AiFlight) {
			
			// If the command is intended for the whole flight
			return TRUE;
		}
		else if(extent == AiWingman) {

			// If the command is intended for just me
			return TRUE;
		}
	}
	else if(flightIdx == AiElementLead) {
		// If I'm the element lead

		if(extent == AiElement) {

			// If the command was intended for the element
			return TRUE;
		}
		else {

			// Otherwise just respond with short call
			return FALSE;
		}
	}
	else if(flightIdx == AiSecondWing) {
		
		if(extent == AiWingman) {
			// If the command is intended for just me
			return TRUE;
		}
		else {
			// If I'm the element's wingman
			return FALSE;
		}
	}

	return FALSE;
}


// ---------------------------------------------
//
// DigitalBrain::RespondLongCallSign
//
// ---------------------------------------------

void AiRespondLongCallSign( AircraftClass* p_aircraft )
{
	FlightClass*	p_flightObj;
	int				flightIndex;
	short				edata[10];

	p_flightObj	= (FlightClass*) p_aircraft->GetCampaignObject();
	flightIndex = p_flightObj->GetComponentIndex(p_aircraft);
	
	edata[0]		= p_flightObj->callsign_id;
	edata[1]		= (p_flightObj->callsign_num - 1) * 4 + flightIndex + 1;
	edata[2]		= -1;
	edata[3]		= -1;
	edata[4]		= -1;
	edata[5]		= -1;
	edata[6]		= -1;
	edata[7]		= -1;
	edata[8]		= -1;
	edata[9]		= -1;

  	AiMakeRadioResponse( p_aircraft, rcENEMYCRASH, edata );
}

// ---------------------------------------------
//
// RespondShortCallSign
//
// ---------------------------------------------

void AiRespondShortCallSign( AircraftClass* p_aircraft )
{
	short edata[10];

	edata[0] = ((FlightClass*) p_aircraft->GetCampaignObject())->GetComponentIndex(p_aircraft);
	edata[1]	= -1;
	edata[2]	= -1;
	edata[3]	= -1;
	edata[4]	= -1;
	edata[5]	= -1;
	edata[6]	= -1;
	edata[7]	= -1;
	edata[8]	= -1;
	edata[9]	= -1;

  	AiMakeRadioResponse( p_aircraft, rcFORMRESPONSEB, edata );
}


// --------------------------------------------------------------------
//
// AiCustomizeRadioMsg
//
// --------------------------------------------------------------------

void AiCustomizeRadioMsg( SimBaseClass* p_sender,
								  int extent,
								  FalconRadioChatterMessage** pp_radioMsgs,
								  int command,
								  VU_ID targetid)
{
RadarClass* theRadar;
float xPos, yPos;
VuEntity* theTarget;
int	flightIdx;

// 2000-10-09 ADDED BY S.G. SO WE DON'T CRASH IF THERE IS NO TEXT ATTACHED TO THE MESSAGE (-1 IS TO FLAG 'STAY QUIET')
	(*pp_radioMsgs)->dataBlock.message		= -1;
// END OF ADDED SECTION

	switch(command) {

	case FalconWingmanMsg::WMSpread:
		(*pp_radioMsgs)->dataBlock.message		= rcGOFORMATION;
		AiFillCallsign( p_sender, extent, pp_radioMsgs, TRUE );
		(*pp_radioMsgs)->dataBlock.edata[2]		= 1;
	break;

	case FalconWingmanMsg::WMWedge:
		(*pp_radioMsgs)->dataBlock.message		= rcGOFORMATION;
		AiFillCallsign( p_sender, extent, pp_radioMsgs, TRUE );
		(*pp_radioMsgs)->dataBlock.edata[2]		= 2;
	break;

	case FalconWingmanMsg::WMTrail:
		(*pp_radioMsgs)->dataBlock.message		= rcGOFORMATION;
		AiFillCallsign( p_sender, extent, pp_radioMsgs, TRUE );
		(*pp_radioMsgs)->dataBlock.edata[2]		= 3;
	break;

	case FalconWingmanMsg::WMLadder:
		(*pp_radioMsgs)->dataBlock.message		= rcGOFORMATION;
		AiFillCallsign( p_sender, extent, pp_radioMsgs, TRUE );
		(*pp_radioMsgs)->dataBlock.edata[2]		= 4;
	break;

	case FalconWingmanMsg::WMStack:
		(*pp_radioMsgs)->dataBlock.message		= rcGOFORMATION;
		AiFillCallsign( p_sender, extent, pp_radioMsgs, TRUE );
		(*pp_radioMsgs)->dataBlock.edata[2]		= 5;
	break;

	case FalconWingmanMsg::WMResCell:
		(*pp_radioMsgs)->dataBlock.message		= rcGOFORMATION;
		AiFillCallsign( p_sender, extent, pp_radioMsgs, TRUE );
		(*pp_radioMsgs)->dataBlock.edata[2]		= 6;
	break;

	case FalconWingmanMsg::WMBox:
		(*pp_radioMsgs)->dataBlock.message		= rcGOFORMATION;
		AiFillCallsign( p_sender, extent, pp_radioMsgs, TRUE );
		(*pp_radioMsgs)->dataBlock.edata[2]		= 7;
	break;

	case FalconWingmanMsg::WMArrowHead:
		(*pp_radioMsgs)->dataBlock.message		= rcGOFORMATION;
		AiFillCallsign( p_sender, extent, pp_radioMsgs, TRUE );
		(*pp_radioMsgs)->dataBlock.edata[2]		= 8;
	break;

	case FalconWingmanMsg::WMFluidFour:
		(*pp_radioMsgs)->dataBlock.message		= rcGOFORMATION;
		AiFillCallsign( p_sender, extent, pp_radioMsgs, TRUE );
		(*pp_radioMsgs)->dataBlock.edata[2]		= 14;
	break;

	case FalconWingmanMsg::WMKickout:
		(*pp_radioMsgs)->dataBlock.message		= rcKICKOUT;
		AiFillCallsign( p_sender, extent, pp_radioMsgs, TRUE );
	break;

	case FalconWingmanMsg::WMCloseup:
		(*pp_radioMsgs)->dataBlock.message		= rcCLOSEUP;
		AiFillCallsign( p_sender, extent, pp_radioMsgs, TRUE );
	break;
	case FalconWingmanMsg::WMGiveBra:
		(*pp_radioMsgs)->dataBlock.message		= rcWHEREAREYOU;
		AiFillCallsign( p_sender, extent, pp_radioMsgs, TRUE );

		flightIdx		= p_sender->GetCampaignObject()->GetComponentIndex(p_sender);
		
		(*pp_radioMsgs)->dataBlock.edata[2] = ((FlightClass*)p_sender->GetCampaignObject())->callsign_id;
		(*pp_radioMsgs)->dataBlock.edata[3] = (((FlightClass*)p_sender->GetCampaignObject())->callsign_num - 1) * 4 + flightIdx + 1;
	break;

	case FalconWingmanMsg::WMGiveStatus:
		(*pp_radioMsgs)->dataBlock.message		= rcSTATUS;
		AiFillCallsign( p_sender, extent, pp_radioMsgs, TRUE );
	break;

	case FalconWingmanMsg::WMGiveDamageReport:
		(*pp_radioMsgs)->dataBlock.message		= rcSENDDAMAGEREPORT;
		AiFillCallsign( p_sender, extent, pp_radioMsgs, TRUE );
	break;

	case FalconWingmanMsg::WMGiveFuelState:
		(*pp_radioMsgs)->dataBlock.message		= rcFUELCHECK;
		AiFillCallsign( p_sender, extent, pp_radioMsgs, TRUE );
	break;

	case FalconWingmanMsg::WMGiveWeaponsCheck:
		(*pp_radioMsgs)->dataBlock.message		= rcWEAPONSCHECK;
		AiFillCallsign( p_sender, extent, pp_radioMsgs, TRUE );

		flightIdx		= p_sender->GetCampaignObject()->GetComponentIndex(p_sender);
		
		(*pp_radioMsgs)->dataBlock.edata[2] = ((FlightClass*)p_sender->GetCampaignObject())->callsign_id;
		(*pp_radioMsgs)->dataBlock.edata[3] = (((FlightClass*)p_sender->GetCampaignObject())->callsign_num - 1) * 4 + flightIdx + 1;
	break;

	case FalconWingmanMsg::WMWeaponsHold:
		(*pp_radioMsgs)->dataBlock.message		= rcHOLDFIRE;
		AiFillCallsign( p_sender, extent, pp_radioMsgs, TRUE );
	break;

	case FalconWingmanMsg::WMWeaponsFree:
		(*pp_radioMsgs)->dataBlock.message		= rcWEAPONSFREE;
		AiFillCallsign( p_sender, extent, pp_radioMsgs, TRUE );
	break;

	case FalconWingmanMsg::WMRejoin:
		(*pp_radioMsgs)->dataBlock.message		= rcREJOIN;
		AiFillCallsign( p_sender, extent, pp_radioMsgs, TRUE );
	break;

	case FalconWingmanMsg::WMSSOffset:
		(*pp_radioMsgs)->dataBlock.message		= rcEXECUTE;
		AiFillCallsign( p_sender, extent, pp_radioMsgs, TRUE );
		(*pp_radioMsgs)->dataBlock.edata[2] = 4;		// was together with Pince, now correct eval setup 
	break;

	case FalconWingmanMsg::WMPince:
		(*pp_radioMsgs)->dataBlock.message		= rcEXECUTE;
		AiFillCallsign( p_sender, extent, pp_radioMsgs, TRUE );
		(*pp_radioMsgs)->dataBlock.edata[2] = 0;
	break;

	case FalconWingmanMsg::WMChainsaw:
		(*pp_radioMsgs)->dataBlock.message		= rcEXECUTE;
		AiFillCallsign( p_sender, extent, pp_radioMsgs, TRUE );
		(*pp_radioMsgs)->dataBlock.edata[2] = 2;
	break;

	case FalconWingmanMsg::WMPosthole:
		(*pp_radioMsgs)->dataBlock.message		= rcEXECUTE;
		AiFillCallsign( p_sender, extent, pp_radioMsgs, TRUE );
		(*pp_radioMsgs)->dataBlock.edata[2] = 1;
	break;

	case FalconWingmanMsg::WMClearSix:
		(*pp_radioMsgs)->dataBlock.message		= rcCLEARSIX;
		AiFillCallsign( p_sender, extent, pp_radioMsgs, TRUE );
	break;

	case FalconWingmanMsg::WMCheckSix:
		(*pp_radioMsgs)->dataBlock.message		= rcCHECKSIX;
		AiFillCallsign( p_sender, extent, pp_radioMsgs, TRUE );
	break;

	case FalconWingmanMsg::WMResumeNormal:
		(*pp_radioMsgs)->dataBlock.message		= rcRESUME;
		AiFillCallsign( p_sender, extent, pp_radioMsgs, TRUE );
		(*pp_radioMsgs)->dataBlock.edata[2] = -1;
		(*pp_radioMsgs)->dataBlock.edata[3] = -1;
	break;

	case FalconWingmanMsg::WMRTB:
		(*pp_radioMsgs)->dataBlock.message		= rcRTB;
		AiFillCallsign( p_sender, extent, pp_radioMsgs, TRUE );
	break;

	case FalconWingmanMsg::WMRadarStby:
		(*pp_radioMsgs)->dataBlock.message		= rcRADAROFF;
		AiFillCallsign( p_sender, extent, pp_radioMsgs, TRUE );
	break;

	case FalconWingmanMsg::WMRadarOn:
		(*pp_radioMsgs)->dataBlock.message		= rcRADARON;
		AiFillCallsign( p_sender, extent, pp_radioMsgs, TRUE );
	break;
	
	case FalconWingmanMsg::WMBuddySpike:
		(*pp_radioMsgs)->dataBlock.message		= rcBUDDYSPIKE;
		AiFillCallsign( p_sender, extent, pp_radioMsgs, TRUE );
	break;

	case FalconWingmanMsg::WMRaygun:
      theRadar = (RadarClass*)FindSensor ((SimMoverClass*)p_sender, SensorClass::Radar);
      theRadar->GetCursorPosition (&xPos, &yPos);
		(*pp_radioMsgs)->dataBlock.message		= rcRAYGUN;
      if (theRadar->TargetUnderCursor() != FalconNullId)
      {
         theTarget = vuDatabase->Find(theRadar->TargetUnderCursor());
		   (*pp_radioMsgs)->dataBlock.edata[2] = SimToGrid(theTarget->YPos());
		   (*pp_radioMsgs)->dataBlock.edata[3] = SimToGrid(theTarget->XPos());
		   (*pp_radioMsgs)->dataBlock.edata[4] = -1*FloatToInt32(theTarget->ZPos());
      }
      else
      {
		   (*pp_radioMsgs)->dataBlock.edata[2] = SimToGrid(p_sender->YPos() + yPos * NM_TO_FT);
		   (*pp_radioMsgs)->dataBlock.edata[3] = SimToGrid(p_sender->XPos() + xPos * NM_TO_FT);
		   (*pp_radioMsgs)->dataBlock.edata[4] = -1*FloatToInt32(p_sender->ZPos());
      }
		AiFillCallsign( p_sender, extent, pp_radioMsgs, TRUE );
	break;

	case FalconWingmanMsg::WMSmokeOn:
		(*pp_radioMsgs)->dataBlock.message		= rcEXECUTE;
		(*pp_radioMsgs)->dataBlock.edata[2]		= 6;
		AiFillCallsign( p_sender, extent, pp_radioMsgs, TRUE );
	break;

	case FalconWingmanMsg::WMSmokeOff:
		(*pp_radioMsgs)->dataBlock.message		= rcEXECUTE;
		(*pp_radioMsgs)->dataBlock.edata[2]		= 7;
		AiFillCallsign( p_sender, extent, pp_radioMsgs, TRUE );
	break;

	case FalconWingmanMsg::WMAssignTarget:
// 2000-10-09 ADDED BY S.G. SO 'Assign Targets' SAYS AT LEAST SOMETHING USEFULL
	case FalconWingmanMsg::WMAssignGroup:
// END OF ADDED SECTION
		FalconEntity* pentity;
		
		pentity = (FalconEntity*) vuDatabase->Find(targetid);

		if(!pentity) {
			return;
		}

		int com;
		if (command == FalconWingmanMsg::WMAssignTarget)
			com = 0;
		else com = 1;
		
		if(pentity->GetDomain() == DOMAIN_LAND || pentity->GetDomain() == DOMAIN_SEA) {
			(*pp_radioMsgs)->dataBlock.message		= rcENGAGEGNDTARGET;
			AiFillCallsign( p_sender, extent, pp_radioMsgs, TRUE );
			(*pp_radioMsgs)->dataBlock.edata[2] = com;
			(*pp_radioMsgs)->dataBlock.edata[3] = SimToGrid(pentity->YPos());
			(*pp_radioMsgs)->dataBlock.edata[4] = SimToGrid(pentity->XPos());
		}
		else if(pentity->IsFlight() || pentity->IsAirplane() || pentity->IsHelicopter() || pentity->IsSquadron() || pentity->IsPackage()) {
			// if air target and shooter mode
			(*pp_radioMsgs)->dataBlock.message		= rcENGAGEDIRECTIVE;
			AiFillCallsign( p_sender, extent, pp_radioMsgs, TRUE );

			flightIdx		= p_sender->GetCampaignObject()->GetComponentIndex(p_sender);
		
			(*pp_radioMsgs)->dataBlock.edata[2] = ((FlightClass*)p_sender->GetCampaignObject())->callsign_id;
			(*pp_radioMsgs)->dataBlock.edata[3] = (((FlightClass*)p_sender->GetCampaignObject())->callsign_num - 1) * 4 + flightIdx + 1;
	
			if(pentity->IsFlight() || pentity->IsSquadron() || pentity->IsPackage()) {
				(*pp_radioMsgs)->dataBlock.edata[4] = ((Unit)pentity)->GetUnitClassData()->VehicleType[0] * 2 + 1;
//				(*pp_radioMsgs)->dataBlock.edata[5] = ((Unit)pentity)->GetTotalVehicles();
				(*pp_radioMsgs)->dataBlock.edata[5] = -1;
			}
			else if(pentity->IsAirplane() || pentity->IsHelicopter()) {
				(*pp_radioMsgs)->dataBlock.edata[4] = 2 * (pentity->Type() - VU_LAST_ENTITY_TYPE);
				(*pp_radioMsgs)->dataBlock.edata[5] = -1;
//				(*pp_radioMsgs)->dataBlock.edata[5] = 0;
			}
			(*pp_radioMsgs)->dataBlock.edata[6] = SimToGrid(pentity->YPos());
			(*pp_radioMsgs)->dataBlock.edata[7] = SimToGrid(pentity->XPos());
			(*pp_radioMsgs)->dataBlock.edata[8] = -1*FloatToInt32(pentity->ZPos());
		}

	break;

	case FalconWingmanMsg::WMShooterMode://me123 addet
		(*pp_radioMsgs)->dataBlock.message		= rcSHOOTER;
		AiFillCallsign( p_sender, extent, pp_radioMsgs, TRUE );
	break;

	case FalconWingmanMsg::WMCoverMode://me123 addet
		//this indicates I want to not play anything
		//Vince: if you feel these should say something, put the code here. DSP
		(*pp_radioMsgs)->dataBlock.message		= rcCOVER;
		AiFillCallsign( p_sender, extent, pp_radioMsgs, TRUE );
	break;

	case FalconWingmanMsg::WMJokerFuel:
		(*pp_radioMsgs)->dataBlock.edata[0]		= ((AircraftClass*)p_sender)->vehicleInUnit;
		(*pp_radioMsgs)->dataBlock.message		= rcFUELCRITICAL;
	break;

	case FalconWingmanMsg::WMFlameout:
	case FalconWingmanMsg::WMBingoFuel:
	case FalconWingmanMsg::WMFumes:
		(*pp_radioMsgs)->dataBlock.edata[0]		= ((AircraftClass*)p_sender)->vehicleInUnit;
		(*pp_radioMsgs)->dataBlock.edata[1]		= (FloatToInt32(((AircraftClass*)p_sender)->af->Fuel() + ((AircraftClass*)p_sender)->af->ExternalFuel()));
		(*pp_radioMsgs)->dataBlock.message		= rcFUELCHECKRSP;
	break;

	
	// Setup for later commfile additions

	case FalconWingmanMsg::WMBreakRight:
		(*pp_radioMsgs)->dataBlock.message		= rcBREAKRL;
		(*pp_radioMsgs)->dataBlock.edata[2]     =   3;
		AiFillCallsign( p_sender, extent, pp_radioMsgs, TRUE );
	break;

	case FalconWingmanMsg::WMBreakLeft:
		(*pp_radioMsgs)->dataBlock.message		= rcBREAKRL;
		(*pp_radioMsgs)->dataBlock.edata[2]     =   0;
		AiFillCallsign( p_sender, extent, pp_radioMsgs, TRUE );
	break;

	case FalconWingmanMsg::WMIncreaseRelAlt:
		(*pp_radioMsgs)->dataBlock.message		= rcINCREASEDECREASERELALT;
		(*pp_radioMsgs)->dataBlock.edata[2]     =   0;
		AiFillCallsign( p_sender, extent, pp_radioMsgs, TRUE );
	break;

	case FalconWingmanMsg::WMDecreaseRelAlt:
		(*pp_radioMsgs)->dataBlock.message		= rcINCREASEDECREASERELALT;
		(*pp_radioMsgs)->dataBlock.edata[2]     =   1;
		AiFillCallsign( p_sender, extent, pp_radioMsgs, TRUE );
	break;

	case FalconWingmanMsg::WMToggleSide:
		(*pp_radioMsgs)->dataBlock.message		= rcTOGGLESIDE;
		AiFillCallsign( p_sender, extent, pp_radioMsgs, TRUE );
	break;

	case FalconWingmanMsg::WMFlex:
		(*pp_radioMsgs)->dataBlock.message		= rcEXECUTE;
		AiFillCallsign( p_sender, extent, pp_radioMsgs, TRUE );
		(*pp_radioMsgs)->dataBlock.edata[2] = 3;

	break;

	case FalconWingmanMsg::WMPromote:
		//Take lead
		(*pp_radioMsgs)->dataBlock.message		= rcEXECUTE;
		AiFillCallsign( p_sender, extent, pp_radioMsgs, TRUE );
		(*pp_radioMsgs)->dataBlock.edata[2]		= 9;
	break;

	case FalconWingmanMsg::WMDropStores:
		(*pp_radioMsgs)->dataBlock.message		= rcDROPSTORES;
		AiFillCallsign( p_sender, extent, pp_radioMsgs, TRUE );
		break;

	case FalconWingmanMsg::WMVic:
		(*pp_radioMsgs)->dataBlock.message		= rcGOFORMATION;
		AiFillCallsign( p_sender, extent, pp_radioMsgs, TRUE );
		(*pp_radioMsgs)->dataBlock.edata[2]		= 11;
		break;

	case FalconWingmanMsg::WMFinger4:
		(*pp_radioMsgs)->dataBlock.message		= rcGOFORMATION;
		AiFillCallsign( p_sender, extent, pp_radioMsgs, TRUE );
		(*pp_radioMsgs)->dataBlock.edata[2]		= 15;
		break;

	case FalconWingmanMsg::WMEchelon:
		(*pp_radioMsgs)->dataBlock.message		= rcGOFORMATION;
		AiFillCallsign( p_sender, extent, pp_radioMsgs, TRUE );
		(*pp_radioMsgs)->dataBlock.edata[2]		= 12;
		break;

	case FalconWingmanMsg::WMForm1:
		(*pp_radioMsgs)->dataBlock.message		= rcGOFORMATION;
		AiFillCallsign( p_sender, extent, pp_radioMsgs, TRUE );
		(*pp_radioMsgs)->dataBlock.edata[2]		= 16;
		break;

	case FalconWingmanMsg::WMForm2:
		(*pp_radioMsgs)->dataBlock.message		= rcGOFORMATION;
		AiFillCallsign( p_sender, extent, pp_radioMsgs, TRUE );
		(*pp_radioMsgs)->dataBlock.edata[2]		= 17;
		break;

	case FalconWingmanMsg::WMForm3:
		(*pp_radioMsgs)->dataBlock.message		= rcGOFORMATION;
		AiFillCallsign( p_sender, extent, pp_radioMsgs, TRUE );
		(*pp_radioMsgs)->dataBlock.edata[2]		= 18;
		break;

	case FalconWingmanMsg::WMForm4:
		(*pp_radioMsgs)->dataBlock.message		= rcGOFORMATION;
		AiFillCallsign( p_sender, extent, pp_radioMsgs, TRUE );
		(*pp_radioMsgs)->dataBlock.edata[2]		= 19;
		break;

	case FalconWingmanMsg::WMGlue:
		(*pp_radioMsgs)->dataBlock.message		= rcEXECUTE;
		AiFillCallsign( p_sender, extent, pp_radioMsgs, TRUE );
		(*pp_radioMsgs)->dataBlock.edata[2] = 11;
		break;
	case FalconWingmanMsg::WMSplit:
		(*pp_radioMsgs)->dataBlock.message		= rcEXECUTE;
		AiFillCallsign( p_sender, extent, pp_radioMsgs, TRUE );
		(*pp_radioMsgs)->dataBlock.edata[2] = 10;
		break;
	case FalconWingmanMsg::WMECMOn:
		(*pp_radioMsgs)->dataBlock.message		= rcORDERECM;
		(*pp_radioMsgs)->dataBlock.edata[2]		= 0;	// Music on
		AiFillCallsign( p_sender, extent, pp_radioMsgs, TRUE );
	break;

	case FalconWingmanMsg::WMECMOff:
		(*pp_radioMsgs)->dataBlock.message		= rcORDERECM;
		(*pp_radioMsgs)->dataBlock.edata[2]		= 1;
		AiFillCallsign( p_sender, extent, pp_radioMsgs, TRUE );
	break;
	// 2002-03-15 ADDED BY S.G. BVR profiles that the player can send to his element
	case FalconWingmanMsg::WMPlevel1a:
	case FalconWingmanMsg::WMPlevel2a:
	case FalconWingmanMsg::WMPlevel3a:
	case FalconWingmanMsg::WMPlevel1b:
	case FalconWingmanMsg::WMPlevel2b:
	case FalconWingmanMsg::WMPlevel3b:
	case FalconWingmanMsg::WMPlevel1c:
	case FalconWingmanMsg::WMPlevel2c:
	case FalconWingmanMsg::WMPlevel3c:
	case FalconWingmanMsg::WMPbeamdeploy:
	case FalconWingmanMsg::WMPbeambeam:
	case FalconWingmanMsg::WMPwall:
	case FalconWingmanMsg::WMPgrinder:
	case FalconWingmanMsg::WMPwideazimuth:
	case FalconWingmanMsg::WMPshortazimuth:
	case FalconWingmanMsg::WMPwideLT:
	case FalconWingmanMsg::WMPShortLT:
	case FalconWingmanMsg::WMPDefensive:
		(*pp_radioMsgs)->dataBlock.message		= -1; // Nothing for now...
		break;
	// END OF ADDED SECTION 2002-03-15
	default:
		(*pp_radioMsgs)->dataBlock.message		= rcROGER;
		(*pp_radioMsgs)->dataBlock.edata[0]		= -1;
		(*pp_radioMsgs)->dataBlock.edata[1]		= -1;
		(*pp_radioMsgs)->dataBlock.edata[2]		= -1;
		(*pp_radioMsgs)->dataBlock.edata[3]		= -1;
		(*pp_radioMsgs)->dataBlock.edata[4]		= -1;
		(*pp_radioMsgs)->dataBlock.edata[5]		= -1;
		(*pp_radioMsgs)->dataBlock.edata[6]		= -1;
		(*pp_radioMsgs)->dataBlock.edata[7]		= -1;
		(*pp_radioMsgs)->dataBlock.edata[8]		= -1;
		(*pp_radioMsgs)->dataBlock.edata[9]		= -1;
	break;

	}
}
