#if 0
/*
 * Machine Generated source file for message "Request Dogfight Slot".
 * NOTE: The functions here must be completed by hand.
 * Generated on 03-April-1997 at 02:36:21
 * Generated from file EVENTS.XLS by PeterW
 */

#include "mesg.h"
#include "falcmesg.h"
#include "falclib.h"
#include "falcmesg.h"
#include "falcgame.h"
#include "falcsess.h"
#include "InvalidBufferException.h"

// KCK: This file is obsolete


/*

UI_RequestDogfightSlot::UI_RequestDogfightSlot(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) : FalconEvent (RequestDogfightSlot, FalconEvent::UIThread, entityId, target, loopback)
{
	RequestOutOfBandTransmit();
}

UI_RequestDogfightSlot::UI_RequestDogfightSlot(VU_MSG_TYPE type, VU_ID senderid, VU_ID target) : FalconEvent (RequestDogfightSlot, FalconEvent::UIThread, senderid, target)
{
   // Your Code Goes Here
}

UI_RequestDogfightSlot::~UI_RequestDogfightSlot(void)
{
	// Your Code Goes Here
}

int UI_RequestDogfightSlot::Process(uchar autodisp)
{
	if(gCommsMgr != NULL)
	{
		if (vuLocalSessionEntity->Game()->OwnerId() == vuLocalSessionEntity->Id())
		{
			FalconSessionEntity* requester = (FalconSessionEntity*)vuDatabase->Find(dataBlock.requester_id);
			if (!requester)
				return FALSE;

			UI_SendDogfightSlot *slot=new UI_SendDogfightSlot(FalconNullId,requester);

			slot->dataBlock.from=FalconLocalSession->Id();
			slot->dataBlock.game=FalconLocalGame->Id();
			if(gCommsMgr->SlotOpen(FalconLocalGame,requester,dataBlock.teamid,dataBlock.planeid))
			{
				slot->dataBlock.teamid=dataBlock.teamid;
				slot->dataBlock.planeid=dataBlock.planeid;
				slot->dataBlock.status=1;
				// KCK: We can try and force a redraw now - but if the data get's overwritten
				// between now and when we get the updated session event, we'll see a glitch.
				// It may be better to just not do this and wait...
//				((FalconSessionEntity*)requester)->SetSide(dataBlock.teamid);
//				((FalconSessionEntity*)requester)->SetPilotSlot(dataBlock.planeid);
//				((FalconSessionEntity*)requester)->DoFullUpdate();
				}
			else
			{
				slot->dataBlock.status=0;
			}
			FalconSendMessage(slot,TRUE);
		}
	}

	return TRUE;
}

*/
#endif