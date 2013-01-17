/*
 * Machine Generated source file for message "Eject Message".
 * NOTE: The functions here must be completed by hand.
 * Generated on 24-September-1997 at 14:37:55
 * Generated from file EVENTS.XLS by MicroProse
 */

#include "MsgInc/EjectMsg.h"
#include "mesg.h"
#include "falclib.h"
#include "falcmesg.h"
#include "falcgame.h"
#include "falcsess.h"
#include "squadron.h"
#include "campmap.h"
#include "MissEval.h"

//sfr: added here for checks
#include "InvalidBufferException.h"

FalconEjectMessage::FalconEjectMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) : FalconEvent (EjectMsg, FalconEvent::SimThread, entityId, target, loopback)
{
	RequestOutOfBandTransmit ();
}

FalconEjectMessage::FalconEjectMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target) : FalconEvent (EjectMsg, FalconEvent::SimThread, senderid, target)
{
	RequestOutOfBandTransmit ();
	type;
}

FalconEjectMessage::~FalconEjectMessage(void)
{
}

int FalconEjectMessage::Process(uchar autodisp)
{
	FalconEntity*   falcEnt;
	Flight			flight;
	Squadron		sq;
	GridIndex		x,y;
	int				squadron_pilot = 255,ps=PILOT_MIA;
	PilotClass		*pc;

	if (autodisp){
		return 0;
	}

	// Determine success of this ejection and adjust squadron/pilot statistics appropriately
	falcEnt = (FalconEntity*)vuDatabase->Find(dataBlock.eFlightID);
	if (falcEnt && falcEnt->IsFlight())
	{
		flight = (Flight)falcEnt;
		flight->GetLocation(&x,&y);

		// KCK: Determanistic rescue right now.. might want to check for chopper actually
		// arriving at some point in the far, far future.
		if (
			GetOwner(TheCampaign.CampMapData,x,y) == flight->GetTeam() ||
			!((flight->GetCampID()+dataBlock.ePilotID)%3))
		{
			ps = PILOT_RESCUED;
		}

		// Record the pilot in the squadron records
		sq = (Squadron) flight->GetUnitSquadron();
		if (sq && dataBlock.ePilotID < PILOTS_PER_FLIGHT){
			squadron_pilot = flight->pilots[dataBlock.ePilotID];
			if (squadron_pilot < PILOTS_PER_SQUADRON){
				pc = sq->GetPilotData(squadron_pilot);
				if (pc){
					pc->pilot_status = (uchar)ps;
				}
			}
		}

		if (TheCampaign.MissionEvaluator){
			TheCampaign.MissionEvaluator->RegisterEjection(this, ps);
		}
	}

	return 0;
}
