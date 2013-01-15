/*
 * Machine Generated source file for message "Control Surface Msg".
 * NOTE: The functions here must be completed by hand.
 * Generated on 20-March-1997 at 14:17:33
 * Generated from file EVENTS.XLS by Leon Rosenshein
 */

#include "MsgInc/ControlSurfaceMsg.h"
#include "mesg.h"
#include "MsgInc/RequestObject.h"
#include "falclib.h"
#include "falcmesg.h"
#include "falcgame.h"
#include "falcsess.h"
#include "InvalidBufferException.h"
#include "OTWDrive.h"
#include "simdrive.h"

FalconControlSurfaceMsg::FalconControlSurfaceMsg(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) : FalconEvent (ControlSurfaceMsg, FalconEvent::SimThread, entityId, target, loopback)
{
   // Your Code Goes Here
}

FalconControlSurfaceMsg::FalconControlSurfaceMsg(VU_MSG_TYPE type, VU_ID senderid, VU_ID target) : FalconEvent (ControlSurfaceMsg, FalconEvent::SimThread, senderid, target)
{
   // Your Code Goes Here
	type;
}

FalconControlSurfaceMsg::~FalconControlSurfaceMsg(void)
{
   // Your Code Goes Here
}

int FalconControlSurfaceMsg::Process(uchar autodisp)
{
SimMoverClass* theEntity;

	if (autodisp)
		return 0;

   theEntity = (SimMoverClass*)(vuDatabase->Find (dataBlock.entityID));
   if (theEntity)
   {
      if (!theEntity->IsLocal())
      {
         theEntity->nonLocalData->timer3 = (float)SimLibElapsedTime / SEC_TO_MSEC;
      }
   }
   else
   {
      if (OTWDriver.IsActive() && SimDriver.GetPlayerEntity())
      {
         MonoPrint ("Control Surfaces for an entity I don't have\n");
      }
   }
   return TRUE;
}

