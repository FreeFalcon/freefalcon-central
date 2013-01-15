/*
 * Machine Generated source file for message "Graphics Text Display".
 * NOTE: The functions here must be completed by hand.
 * Generated on 03-April-1997 at 17:25:17
 * Generated from file EVENTS.XLS by Leon Rosenshein
 */

#include "MsgInc/GraphicsTextDisplayMsg.h"
#include "mesg.h"
#include "otwdrive.h"
#include "falclib.h"
#include "falcmesg.h"
#include "falcgame.h"
#include "falcsess.h"
#include "InvalidBufferException.h"

GraphicsTextDisplay::GraphicsTextDisplay(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) : FalconEvent (GraphicsTextDisplayMsg, FalconEvent::SimThread, entityId, target, loopback)
{
   // Your Code Goes Here
}

GraphicsTextDisplay::GraphicsTextDisplay(VU_MSG_TYPE type, VU_ID senderid, VU_ID target) : FalconEvent (GraphicsTextDisplayMsg, FalconEvent::SimThread, senderid, target)
{
   // Your Code Goes Here
	type;
}

GraphicsTextDisplay::~GraphicsTextDisplay(void)
{
   // Your Code Goes Here
}

int GraphicsTextDisplay::Process(uchar autodisp)
{
	if (autodisp)
		return 0;

   OTWDriver.ShowMessage (dataBlock.msg);

   return 1;
}

