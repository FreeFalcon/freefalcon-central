#include "MsgInc/VoiceDataMsg.h"
#include "mesg.h"
#include "falclib.h"
#include "falcmesg.h"
#include "falcgame.h"
#include "falcsess.h"
#include "InvalidBufferException.h"

FalconVoiceDataMessage::FalconVoiceDataMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) : FalconEvent (VoiceDataMsg, FalconEvent::VuThread, entityId, target, loopback)
{
   // Your Code Goes Here
}

FalconVoiceDataMessage::FalconVoiceDataMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target) : FalconEvent (VoiceDataMsg, FalconEvent::VuThread, senderid, target)
{
   // Your Code Goes Here
	type;
}

FalconVoiceDataMessage::~FalconVoiceDataMessage(void)
{
   // Your Code Goes Here
}

int FalconVoiceDataMessage::Process(uchar autodisp)
{
   // Your Code Goes Here
   return 0;
   autodisp;
}

