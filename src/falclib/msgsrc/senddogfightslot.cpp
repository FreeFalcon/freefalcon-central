#if 0

/*
 * Machine Generated source file for message "Send Dogfight Slot".
 * NOTE: The functions here must be completed by hand.
 * Generated on 06-April-1997 at 01:24:16
 * Generated from file EVENTS.XLS by PeterW
 */

#include "falcsess.h"
//#include "MsgInc\SendDogfightSlot.h"
#include "mesg.h"
#include "uicomms.h"
#include "falclib.h"
#include "falcmesg.h"
#include "falcgame.h"
#include "falcsess.h"
#include "InvalidBufferException.h"

/*
UI_SendDogfightSlot::UI_SendDogfightSlot(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) : FalconEvent (SendDogfightSlot, FalconEvent::UIThread, entityId, target, loopback)
{
	RequestOutOfBandTransmit();
}

UI_SendDogfightSlot::UI_SendDogfightSlot(VU_MSG_TYPE type, VU_ID senderid, VU_ID target) : FalconEvent (SendDogfightSlot, FalconEvent::UIThread, senderid, target)
{
   // Your Code Goes Here
}

UI_SendDogfightSlot::~UI_SendDogfightSlot(void)
{
   // Your Code Goes Here
}

int UI_SendDogfightSlot::Process(uchar autodisp)
{
	// Abort if this is autodispatching (the UI is probably shutdown)
	if (autodisp)
		return 0;

	if(gCommsMgr != NULL)
	{
		VuGameEntity	*game = (VuGameEntity*) vuDatabase->Find(dataBlock.game);

		if (game != FalconLocalGame)
			return FALSE;

		gCommsMgr->LookAtGame(game);

		if(dataBlock.status)
		{
			if(FalconLocalGame != game)
				gCommsMgr->JoinGame(dataBlock.game);

			FalconLocalSession->SetSide(dataBlock.teamid);
			FalconLocalSession->SetPilotSlot(dataBlock.planeid);
			FalconLocalSession->DoFullUpdate();

		}
		else
		{
			gCommsMgr->LeaveGame();

			FalconLocalSession->SetSide(255);
			FalconLocalSession->SetPilotSlot(255);
			FalconLocalSession->DoFullUpdate();
		}
	}
	return TRUE;
}

*/
#endif