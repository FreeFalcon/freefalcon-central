/*
 * Machine Generated source file for message "Request Campaign data".
 * NOTE: The functions here must be completed by hand.
 * Generated on 01-April-1997 at 14:44:37
 * Generated from file EVENTS.XLS by Kevin Klemmick
 */
/*

//sfr: this is not used, and does not compile 
#include "MsgInc\RequestCampaignMsg.h"
#include "MsgInc\SendCampaignMsg.h"
#include "mesg.h"
#include "falclib.h"
#include "falcmesg.h"
#include "falcgame.h"
#include "falcsess.h"

//sfr: added here for checks
#include "InvalidBufferException.h"
using std::memcpychk;


FalconRequestCampaign::FalconRequestCampaign(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) :
		FalconEvent (RequestCampaignMsg, FalconEvent::CampaignThread, entityId, target, loopback)
{
	// Your Code Goes Here
}

FalconRequestCampaign::FalconRequestCampaign(VU_MSG_TYPE type, VU_ID senderid, VU_ID target) : FalconEvent (RequestCampaignMsg, FalconEvent::CampaignThread, senderid, target)
{
	// Your Code Goes Here
}

FalconRequestCampaign::~FalconRequestCampaign(void)
{
	// Your Code Goes Here
}

int FalconRequestCampaign::Process(void)
{
	// We got a request for campaign info, we need to send a reply
	FalconSendCampaign*	msg = new FalconSendCampaign(
		dataBlock.requester_session,dataBlock.requester_session,VU_SPECIFIED_TARGET);
	msg->dataBlock.campTime = Camp_GetCurrentTime();
	msg->dataBlock.from = vuLocalSessionEntity->Id();
	FalconSendMessage(msg,TRUE);
	return 0;
}

*/