/*
 * Machine Generated source file for message "Camp Event Data".
 * NOTE: The functions here must be completed by hand.
 * Generated on 25-November-1998 at 11:57:47
 * Generated from file EVENTS.XLS by MicroProse
 */

#include "MsgInc\CampEventDataMsg.h"
#include "mesg.h"
#include "brief.h"
#include "CmpEvent.h"

#include "Cmpclass.h"
#include "falclib.h"
#include "falcmesg.h"
#include "falcgame.h"
#include "falcsess.h"
#include "InvalidBufferException.h"

extern EventClass**	CampEvents;

extern void UI_AddMovieToList(long ID,long timestamp,_TCHAR *Description);

CampEventDataMessage::CampEventDataMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) : FalconEvent (CampEventDataMsg, FalconEvent::CampaignThread, entityId, target, loopback)
{
	dataBlock.event = 0;
	dataBlock.status = 0;
}

CampEventDataMessage::CampEventDataMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target) : FalconEvent (CampEventDataMsg, FalconEvent::CampaignThread, senderid, target)
{
	dataBlock.event = 0;
	dataBlock.status = 0;
	type;
}

CampEventDataMessage::~CampEventDataMessage(void)
{
}

int CampEventDataMessage::Process(uchar autodisp)
{
	switch (dataBlock.message)
	{
			case eventMessage:
					if (dataBlock.status)
						CampEvents[dataBlock.event]->flags |= CE_FIRED;
					else
						CampEvents[dataBlock.event]->flags &= ~CE_FIRED;
					break;
			case victoryConditionMessage:
					break;
			case playMovie:
					_TCHAR		str[128]={0};
					AddIndexedStringToBuffer(1160+dataBlock.event-100,str);
					UI_AddMovieToList(dataBlock.event,TheCampaign.CurrentTime,str); // Must be a "localized" string...
					break;
	}

	return 0;
	autodisp;
}

