#include "MsgInc/SendLogbook.h"
#include "mesg.h"
#include "ui/include/uicomms.h"
#include "falclib.h"
#include "falcmesg.h"
#include "falcgame.h"
#include "falcsess.h"
#include "InvalidBufferException.h"

UI_SendLogbook::UI_SendLogbook(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) : FalconEvent (SendLogbook, FalconEvent::SimThread, entityId, target, loopback)
{
	// Your Code Goes Here
}

UI_SendLogbook::UI_SendLogbook(VU_MSG_TYPE type, VU_ID senderid, VU_ID target) : FalconEvent (SendLogbook, FalconEvent::SimThread, senderid, target)
{
	// Your Code Goes Here
}

UI_SendLogbook::~UI_SendLogbook(void)
{
	// Your Code Goes Here
}

int UI_SendLogbook::Process(uchar autodisp)
{
	if(gCommsMgr){
		gCommsMgr->ReceiveLogbook(this->dataBlock.fromID,&this->dataBlock.Pilot);
	}
	return 0;
	autodisp;
}

