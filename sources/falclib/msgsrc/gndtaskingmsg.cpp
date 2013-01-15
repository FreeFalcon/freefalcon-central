#include "MsgInc/GndTaskingMsg.h"
#include "mesg.h"
#include "GTM.h"
#include "falclib.h"
#include "falcmesg.h"
#include "falcgame.h"
#include "falcsess.h"
#include "InvalidBufferException.h"

FalconGndTaskingMessage::FalconGndTaskingMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) : FalconEvent (GndTaskingMsg, FalconEvent::CampaignThread, entityId, target, loopback)
{	
		// Your Code Goes Here
}

FalconGndTaskingMessage::FalconGndTaskingMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target) : FalconEvent (GndTaskingMsg, FalconEvent::CampaignThread, senderid, target)
{
	// Your Code Goes Here
}

FalconGndTaskingMessage::~FalconGndTaskingMessage(void)
{
	// Your Code Goes Here
}

int FalconGndTaskingMessage::Process(uchar autodisp)
{
	if (autodisp)
		return 0;

	switch(dataBlock.messageType)
	{
		case gtmSupportRequest:
//			TeamInfo[dataBlock.team]->gtm->RequestSupport(dataBlock.enemy,dataBlock.data1);
			break;
		case gtmEngineerRequest:
//			TeamInfo[dataBlock.team]->gtm->RequestEngineer(dataBlock.data1,dataBlock.data2,dataBlock.enemy.num_);
			break;
		case gtmAirDefenseRequest:
//			TeamInfo[dataBlock.team]->gtm->RequestAirDefense(dataBlock.data1,dataBlock.data2,dataBlock.enemy.num_);
			break;
		default:
			break;
	}
	return 0;
}

