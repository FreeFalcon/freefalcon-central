/*
 * Machine Generated source file for message "Landing Message".
 * NOTE: The functions here must be completed by hand.
 * Generated on 25-February-1997 at 09:47:38
 * Generated from file EVENTS.XLS by Leon Rosenshein
 */

#include "MsgInc/LandingMessage.h"
#include "mesg.h"
#include "falclib.h"
#include "falcmesg.h"
#include "falcgame.h"
#include "falcsess.h"
#include "pilot.h"
#include "Cmpclass.h"
#include "MissEval.h"

#include "InvalidBufferException.h"

FalconLandingMessage::FalconLandingMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) : FalconEvent (LandingMessage, FalconEvent::SimThread, entityId, target, loopback)
{
   // Your Code Goes Here
}

FalconLandingMessage::FalconLandingMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target) : FalconEvent (LandingMessage, FalconEvent::SimThread, senderid, target)
{
   // Your Code Goes Here
	type;
}

FalconLandingMessage::~FalconLandingMessage(void)
{
   // Your Code Goes Here
}

int FalconLandingMessage::Process(uchar autodisp)
{
	GridIndex	x,y;
	int			status;

	if (!Entity() || autodisp)
		return -1;

	// Check for friendly territory
	x = SimToGrid(Entity()->YPos());
	y = SimToGrid(Entity()->XPos());
#if 0	//MI Marco's landing at relocated AB fix
	if (GetRoE(GetOwner(TheCampaign.CampMapData,x,y),((FalconEntity*)Entity())->GetTeam(),ROE_AIR_USE_BASES) == ROE_ALLOWED)
		status = PILOT_AVAILABLE;
	else
		{
		status = PILOT_MIA;
//		ShiAssert(!"Please show Kevin K this message!");
		}
#else
	status = PILOT_AVAILABLE;
#endif

	TheCampaign.MissionEvaluator->RegisterLanding(this,status);
	return 0;
}
