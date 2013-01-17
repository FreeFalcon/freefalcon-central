#include "MsgInc/RequestObject.h"
#include "mesg.h"
#include "falclib.h"
#include "falcmesg.h"
#include "falcgame.h"
#include "falcsess.h"
#include "InvalidBufferException.h"


FalconRequestObject::FalconRequestObject(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) : FalconEvent (RequestObject, FalconEvent::SimThread, entityId, target, loopback)
{
   // Your Code Goes Here
}

FalconRequestObject::FalconRequestObject(VU_MSG_TYPE type, VU_ID senderid, VU_ID target) : FalconEvent (RequestObject, FalconEvent::SimThread, senderid, target)
{
   // Your Code Goes Here
	type;
}

FalconRequestObject::~FalconRequestObject(void)
{
   // Your Code Goes Here
}

int FalconRequestObject::Process(uchar autodisp)
{
	if (autodisp)
		return 0;

FalconSessionEntity* fromEnt = (FalconSessionEntity*)vuDatabase->Find (dataBlock.fromID);

   if (fromEnt && Entity() && Entity()->IsLocal())
   {
      VuMessage *resp;
      resp = new VuFullUpdateEvent(Entity(), fromEnt);
	   VuMessageQueue::PostVuMessage(resp);
   }
   return TRUE;
}

