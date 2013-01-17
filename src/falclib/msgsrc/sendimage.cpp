#include "MsgInc/SendImage.h"
#include "mesg.h"
#include "falclib.h"
#include "falcmesg.h"
#include "falcgame.h"
#include "falcsess.h"
#include "InvalidBufferException.h"

UI_SendImage::UI_SendImage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) : FalconEvent (SendImage, FalconEvent::SimThread, entityId, target, loopback)
{
   // Your Code Goes Here
}

UI_SendImage::UI_SendImage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target) : FalconEvent (SendImage, FalconEvent::SimThread, senderid, target)
{
   // Your Code Goes Here
	type;
}

UI_SendImage::~UI_SendImage(void)
{
   // Your Code Goes Here
	delete dataBlock.data;
}

int UI_SendImage::Process(uchar autodisp)
{
   // Your Code Goes Here
   return 0;
   autodisp;
}

