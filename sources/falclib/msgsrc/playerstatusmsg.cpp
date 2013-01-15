#include "MsgInc/PlayerStatusMsg.h"
#include "MissEval.h"
#include "GameMgr.h"
#include "mesg.h"
#include "falclib.h"
#include "falcmesg.h"
#include "falcgame.h"
#include "falcsess.h"
#include "simmover.h"
#include "campbase.h"
#include "Unit.h"
#include "simdrive.h"

//sfr: added here for checks
#include "InvalidBufferException.h"


FalconPlayerStatusMessage::FalconPlayerStatusMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) : FalconEvent (PlayerStatusMsg, FalconEvent::SimThread, entityId, target, loopback)
{
	RequestOutOfBandTransmit();
	RequestReliableTransmit();//me123
}

FalconPlayerStatusMessage::FalconPlayerStatusMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target) : 
	FalconEvent(PlayerStatusMsg, FalconEvent::SimThread, senderid, target)
{
	RequestOutOfBandTransmit();
	RequestReliableTransmit();//me123
}

FalconPlayerStatusMessage::~FalconPlayerStatusMessage(){
}

int FalconPlayerStatusMessage::Process(uchar autodisp){
	FalconSessionEntity		*session = (FalconSessionEntity*) Entity();
	SimMoverClass			*mover = (SimMoverClass*) vuDatabase->Find(dataBlock.playerID);
	SimBaseClass			*theObject;
	if (autodisp){
		return 0;
	}

	if (!session){
		return 0;
	}

	if (!mover){
		FalconPlayerStatusMessage	*msg = new FalconPlayerStatusMessage(session->Id(), FalconLocalSession);
		_tcscpy(msg->dataBlock.callsign, dataBlock.callsign);
		msg->dataBlock.playerID         = dataBlock.playerID;
		msg->dataBlock.campID           = dataBlock.campID;
		msg->dataBlock.side             = dataBlock.side;
		msg->dataBlock.pilotID          = dataBlock.pilotID;
		msg->dataBlock.vehicleID		= dataBlock.vehicleID;
		msg->dataBlock.state            = dataBlock.state;

		VuTimerEvent *timer = new VuTimerEvent(0, vuxRealTime + 1000, VU_DELAY_TIMER, msg);
		VuMessageQueue::PostVuMessage(timer);

		return 0;
	}

	//	MonoPrint ("Player Status Message\n");
	//	MonoPrint ("  session %08x\n", session->Id().creator_);
	//	MonoPrint ("  mover %08x\n", mover);
	//	MonoPrint ("  pilotID %08x\n", dataBlock.pilotID);

	// Update this player's state and the state of the vehicle they entered/exited
	if (dataBlock.state == PSM_STATE_ENTERED_SIM){
		//MonoPrint ("  State Entered Sim\n");
		// Register this event
		if (TheCampaign.MissionEvaluator){
			TheCampaign.MissionEvaluator->RegisterPlayerJoin(this);
		}
		if (!mover){
			return 0;
		}
		//  Attach the session to this vehicle
		GameManager.AttachPlayerToVehicle(session, mover, dataBlock.pilotID);
		// Wake the vehicle's drawable, if it's a player only vehicle
		if (mover->IsSetFalcFlag(FEC_PLAYERONLY) && !mover->IsAwake() && mover->GetCampaignObject()->IsAwake()){
			VuListIterator flit(mover->GetCampaignObject()->GetComponents());
			theObject = (SimBaseClass*) flit.GetFirst();
			mover->Wake();
			while (theObject){
				if ((!theObject->IsAwake()) && (!theObject->IsSetFalcFlag(FEC_HASPLAYERS))){
					theObject->Wake ();
				}
				theObject = (SimBaseClass*)flit.GetNext();
			}
		}
	}
	else if (dataBlock.state == PSM_STATE_LEFT_SIM){
		//MonoPrint ("  State Left Sim\n");
		// Register the event
		if (TheCampaign.MissionEvaluator){
			TheCampaign.MissionEvaluator->RegisterPlayerJoin(this);
			TheCampaign.MissionEvaluator->ServerFileLog(this);
		}

		if (!mover){
			return 0;
		}

		// Sleep this vehicle if it's a player only vehicle
		if (mover->IsSetFalcFlag(FEC_PLAYERONLY) && mover->IsAwake()){
			mover->Sleep();
		}

		// Detach the session from the vehicle
		GameManager.DetachPlayerFromVehicle(session, mover);

		// Sleep the flight if this is the only plane in the flight and it's player only
		if (mover->IsSetFalcFlag(FEC_PLAYERONLY)){
			Unit campObject = (UnitClass*) mover->GetCampaignObject();
			if (campObject->NumberOfComponents() < 2){
				campObject->Sleep();
				campObject->SetFinal(0);
			}
		}
	}
	else if (dataBlock.state == PSM_STATE_TRANSFERED){
		//MonoPrint ("  State Transfered\n");
		if (mover && session){
			SimMoverClass	*oldMover = (SimMoverClass*) vuDatabase->Find(dataBlock.oldID);
			uchar			oldpslot = 255;
			//MonoPrint ("  old %08x\n", oldMover);
			if (oldMover){
				oldpslot = oldMover->pilotSlot; 
			}
			// Reassign player to new vehicle
			GameManager.ReassignPlayerVehicle(session, oldMover, mover);
			// Keep player accountable for the ejected ac
			if (oldMover){
	   			oldMover->pilotSlot = oldpslot;					
			}
		}
	}

	return 0;
}

