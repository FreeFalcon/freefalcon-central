/*
 * Machine Generated source file for message "Camp Tasking Message".
 * NOTE: The functions here must be completed by hand.
 * Generated on 24-October-1996 at 13:37:00
 * Generated from file EVENTS.XLS by KEVINK
 */

#include "MsgInc/CampTaskingMsg.h"
#include "mesg.h"
#include "falclib.h"
#include "falcmesg.h"
#include "falcgame.h"
#include "falcsess.h"
#include "InvalidBufferException.h"


FalconCampTaskingMessage::FalconCampTaskingMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) : FalconEvent (CampTaskingMsg, FalconEvent::CampaignThread, entityId, target, loopback)
{
	// Your Code Goes Here
}

FalconCampTaskingMessage::FalconCampTaskingMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target) : FalconEvent (CampTaskingMsg, FalconEvent::CampaignThread, senderid, target)
{
	// Your Code Goes Here
	type;
}

FalconCampTaskingMessage::~FalconCampTaskingMessage(void)
{
	// Your Code Goes Here
}

int FalconCampTaskingMessage::Process(uchar autodisp)
{
	// Not implemented currently
	return 0;
	autodisp;
}

