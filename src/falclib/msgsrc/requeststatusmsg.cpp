#if 0

/*
 * Machine Generated source file for message "Request Status".
 * NOTE: The functions here must be completed by hand.
 * Generated on 05-November-1996 at 17:39:12
 * Generated from file EVENTS.XLS by Leon Rosenshein
 */

/*
#include "MsgInc\RequestStatusMsg.h"
#include "MsgInc\SendStatusMsg.h"
#include "mesg.h"
#include "falcmesg.h"
#include "falclib.h"
#include "falcmesg.h"
#include "falcgame.h"
#include "falcsess.h"
#include "InvalidBufferException.h"

FalconRequestStatus::FalconRequestStatus(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) : FalconEvent (RequestStatusMsg, FalconEvent::SimThread, entityId, target, loopback)
{
   // Your Code Goes Here
}

FalconRequestStatus::FalconRequestStatus(VU_MSG_TYPE type, VU_ID senderid, VU_ID target) : FalconEvent (RequestStatusMsg, FalconEvent::SimThread, senderid, target)
{
   // Your Code Goes Here
}

FalconRequestStatus::~FalconRequestStatus(void)
{
   // Your Code Goes Here
}

int FalconRequestStatus::Process(uchar autodisp)
{
   if (vuLocalSessionEntity->Group()->Id().creator_ == vuLocalSessionEntity->Id().creator_)
   {
   FalconSendStatus* newStatus = new FalconSendStatus(FalconNullId, VU_GROUP_TARGET);

      newStatus->dataBlock.gameTime = vuxGameTime;
      newStatus->dataBlock.whoDidIt = vuLocalSessionEntity->Id();
      newStatus->dataBlock.elapsedTime = vuxGameTime;
      newStatus->dataBlock.timeOfDay = 0.0F;

      FalconSendMessage (newStatus,TRUE);
   }

   return TRUE;
}
*/
#endif
