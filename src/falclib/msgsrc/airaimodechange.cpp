/*
 * Machine Generated source file for message "Air AI Mode Change".
 * NOTE: The functions here must be completed by hand.
 * Generated on 05-November-1996 at 17:39:13
 * Generated from file EVENTS.XLS by Leon Rosenshein
 */

#include "MsgInc/AirAIModeChange.h"
#include "mesg.h"
#include "falclib.h"
#include "falcmesg.h"
#include "falcgame.h"
#include "falcsess.h"
#include "InvalidBufferException.h"


AirAIModeMsg::AirAIModeMsg(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) : FalconEvent (AirAIModeChange, FalconEvent::SimThread, entityId, target, loopback)
{
   // Your Code Goes Here
}

AirAIModeMsg::AirAIModeMsg(VU_MSG_TYPE type, VU_ID senderid, VU_ID target) : FalconEvent (AirAIModeChange, FalconEvent::SimThread, senderid, target)
{
   // Your Code Goes Here
	type;
}

AirAIModeMsg::~AirAIModeMsg(void)
{
   // Your Code Goes Here
}

int AirAIModeMsg::Process(uchar autodisp)
{
   // Your Code Goes Here
   return 0;
   autodisp;
}

