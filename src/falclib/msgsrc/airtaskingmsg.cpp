/*
 * Machine Generated source file for message "Air Tasking Message".
 * NOTE: The functions here must be completed by hand.
 * Generated on 23-October-1996 at 13:34:00
 * Generated from file EVENTS.XLS by KEVINK
 */

#include "MsgInc/AirTaskingMsg.h"
#include "mesg.h"
#include "Cmpclass.h"
#include "Team.h"
#include "ATM.h"
#include "falclib.h"
#include "falcmesg.h"
#include "falcgame.h"
#include "falcsess.h"

//sfr: added here for checks
#include "InvalidBufferException.h"

FalconAirTaskingMessage::FalconAirTaskingMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback)
		: FalconEvent (AirTaskingMsg, FalconEvent::CampaignThread, entityId, target, loopback)
{

			// Your Code Goes Here
}

FalconAirTaskingMessage::FalconAirTaskingMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target)
		:FalconEvent (AirTaskingMsg, FalconEvent::CampaignThread, senderid, target)
{
	// Your Code Goes Here
	type;
}

FalconAirTaskingMessage::~FalconAirTaskingMessage(void) {
	// Your Code Goes Here
}

int FalconAirTaskingMessage::Process(uchar autodisp) {
	// I shouldn't really be getting messages if we're not loaded!?!
	if (autodisp || !TheCampaign.IsLoaded()){
		return -1;
	}

	if (!TeamInfo[dataBlock.team]->atm || !TeamInfo[dataBlock.team]->atm->IsLocal()){
		return -1;
	}

	switch (dataBlock.messageType) {
			case atmAircraftRequest:
			case atmAssignMission:
					// These are for player built missions. Not supported right now
					break;
			case atmAssignDivert:
					{
						// Player requested divert. Ask our ATM to build a divert specifically for this flight
						Flight	flight = (Flight)FindUnit(dataBlock.from);
						if (flight)
							TeamInfo[dataBlock.team]->atm->BuildSpecificDivert(flight);
					}
					break;
			case atmNewACAvail:
					TeamInfo[dataBlock.team]->atm->flags |= ATM_NEW_PLANES;
					break;
			case atmCompleteMission:
					//			TeamInfo[dataBlock.team]->atm->RemoveRequest((ushort)dataBlock.data1);
					break;
			case atmScheduleAircraft:
					ShiWarning("This should no longer be used");
					//			TeamInfo[dataBlock.team]->atm->ScheduleAircraft(dataBlock.from, dataBlock.data1, dataBlock.data2);
					break;
			case atmZapAirbase:
					TeamInfo[dataBlock.team]->atm->ZapAirbase(dataBlock.from);
					break;
			default:
				break;
	}
	return 0;
}

