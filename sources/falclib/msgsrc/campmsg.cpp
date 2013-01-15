/*
 * Machine Generated source file for message "Camp Messages".
 * NOTE: The functions here must be completed by hand.
 * Generated on 05-November-1996 at 17:39:12
 * Generated from file EVENTS.XLS by Leon Rosenshein
 */

#include "MsgInc\CampMsg.h"
#include "mesg.h"
#include "Find.h"
#include "CampBase.h"
#include "CampWeap.h"
#include "Unit.h"
#include "Objectiv.h"
#include "Squadron.h"
#include "CmpClass.h"
#include "MissEval.h"
#include "Update.h"
#include "SimDrive.h"
#include "AIInput.h"
#include "MsgInc/RadioChatterMsg.h"
#include "classtbl.h"
#include "FalcUser.h"
#include "Dispcfg.h"
#include "falclib.h"
#include "falcmesg.h"
#include "falcgame.h"
#include "falcsess.h"
#include "InvalidBufferException.h"

void DeaggregateOwnershipCheck (CampEntity the_entity, FalconSessionEntity *session, int deag_request);

FalconCampMessage::FalconCampMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) : FalconEvent (CampMsg, FalconEvent::CampaignThread, entityId, target, loopback)
{
	RequestReliableTransmit ();
	// Your Code Goes Here
}

FalconCampMessage::FalconCampMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target) : FalconEvent (CampMsg, FalconEvent::CampaignThread, senderid, target)
{
	// Your Code Goes Here
	type;
}

FalconCampMessage::~FalconCampMessage(void)
{
	// Your Code Goes Here
}

int FalconCampMessage::Process(uchar autodisp)
{
	CampEntity			e;

	if (autodisp)
		return 0;

	e = FindEntity(EntityId());
	if (!e)
		return 0;
	switch (dataBlock.message)
	{
			case campAttackWarning:
					break;
			case campFiredOn:
					break;
			case campSpotted:
					e->SetSpotted((Team)dataBlock.data1,TheCampaign.CurrentTime);
					break;
			case campRepair:
					if (e->IsUnit())
						((Unit)e)->ChangeVehicles(dataBlock.data1);
					else if (e->IsObjective())
						((Objective)e)->Repair();
					break;
			default:
					break;
	}
	return 0;
}

// ==================================================
// Local functions
// ==================================================

