/*
 * Machine Generated source file for message "Mission Request".
 * NOTE: The functions here must be completed by hand.
 * Generated on 21-October-1996 at 19:43:18
 * Generated from file EVENTS.XLS by KEVINK
 */

#include "MsgInc/MissionRequestMsg.h"
#include "mesg.h"
#include "ATM.h"
#include "F4Thread.h"
#include "falclib.h"
#include "falcmesg.h"
#include "falcgame.h"
#include "falcsess.h"
#include "InvalidBufferException.h"


extern F4CSECTIONHANDLE* vuCritical;

FalconMissionRequestMessage::FalconMissionRequestMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) : FalconEvent (MissionRequestMsg, FalconEvent::CampaignThread, entityId, target, loopback)
{	
	// Your Code Goes Here
}

FalconMissionRequestMessage::FalconMissionRequestMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target) : FalconEvent (MissionRequestMsg, FalconEvent::CampaignThread, senderid, target)
{
	// Your Code Goes Here
}

FalconMissionRequestMessage::~FalconMissionRequestMessage(void)
{
	// Your Code Goes Here
}

int FalconMissionRequestMessage::Process(uchar autodisp)
{
//	F4Assert(vuCritical->count == 0);

	if (autodisp)
		return -1;

	if (TeamInfo[dataBlock.team] && TeamInfo[dataBlock.team]->atm && TeamInfo[dataBlock.team]->atm->IsLocal())
		TeamInfo[dataBlock.team]->atm->ProcessRequest(&(dataBlock.request));
	return 0;
}

