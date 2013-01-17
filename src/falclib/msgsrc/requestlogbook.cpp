#include "MsgInc/RequestLogbook.h"
#include "mesg.h"
#include "ui/include/uicomms.h"
#include "falclib.h"
#include "falcmesg.h"
#include "falcgame.h"
#include "falcsess.h"
#include "InvalidBufferException.h"

UI_RequestLogbook::UI_RequestLogbook(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) : FalconEvent (RequestLogbook, FalconEvent::SimThread, entityId, target, loopback)
{
   // Your Code Goes Here
}

UI_RequestLogbook::UI_RequestLogbook(VU_MSG_TYPE type, VU_ID senderid, VU_ID target) : FalconEvent (RequestLogbook, FalconEvent::SimThread, senderid, target)
{
   // Your Code Goes Here
}

UI_RequestLogbook::~UI_RequestLogbook(void)
{
   // Your Code Goes Here
}

int UI_RequestLogbook::Process(uchar autodisp)
{
	if(gCommsMgr && dataBlock.fromID != FalconLocalSessionId){
		gCommsMgr->SendLogbook(dataBlock.fromID);
	}
	return 0;
}

