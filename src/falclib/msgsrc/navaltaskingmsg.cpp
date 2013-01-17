/*
 * Machine Generated source file for message "Naval Tasking Message".
 * NOTE: The functions here must be completed by hand.
 * Generated on 30-October-1996 at 21:23:46
 * Generated from file EVENTS.XLS by KEVINK
 */

#include "MsgInc/NavalTaskingMsg.h"
#include "mesg.h"
#include "falclib.h"
#include "falcmesg.h"
#include "falcgame.h"
#include "falcsess.h"
#include "InvalidBufferException.h"


FalconNavalTaskingMessage::FalconNavalTaskingMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) : FalconEvent (NavalTaskingMsg, FalconEvent::CampaignThread, entityId, target, loopback)
	{
   // Your Code Goes Here
	}

FalconNavalTaskingMessage::FalconNavalTaskingMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target) : FalconEvent (NavalTaskingMsg, FalconEvent::CampaignThread, senderid, target)
	{
   // Your Code Goes Here
	type;
	}

FalconNavalTaskingMessage::~FalconNavalTaskingMessage(void)
	{
   // Your Code Goes Here
	}

int FalconNavalTaskingMessage::Process(uchar autodisp)
	{
	// Nothing here for now
	return 0;
	autodisp;
	}

