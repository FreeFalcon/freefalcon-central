#if 0
/*
 * Machine Generated source file for message "Weapon Use Message".
 * NOTE: The functions here must be completed by hand.
 * Generated on 18-August-1997 at 15:21:10
 * Generated from file EVENTS.XLS by MicroProse
 */

#include "MsgInc/WeaponUsageMsg.h"
#include "mesg.h"
#include "falclib.h"
#include "falcmesg.h"
#include "falcgame.h"
#include "falcsess.h"
#include "InvalidBufferException.h"

/* Unused... REMOVE

FalconWeaponUsageMessage::FalconWeaponUsageMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) : FalconEvent (WeaponUsageMsg, FalconEvent::CampaignThread, entityId, target, loopback)
{
   // Your Code Goes Here
}

FalconWeaponUsageMessage::FalconWeaponUsageMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target) : FalconEvent (WeaponUsageMsg, FalconEvent::CampaignThread, senderid, target)
{
   // Your Code Goes Here
}

FalconWeaponUsageMessage::~FalconWeaponUsageMessage(void)
{
   // Your Code Goes Here
}

int FalconWeaponUsageMessage::Process(uchar autodisp)
	{
	Squadron	sq = (Squadron)FindUnit(EntityId());
	int			i,n;
	long		f;

	if (autodisp || !sq)
		return 0;
	
	for (i=0; i<HARDPOINT_MAX; i++)
		{
		n = sq->GetStores(dataBlock.ids[i]) - dataBlock.num[i]*dataBlock.vehicles;
		if (n < 0)
			n = 0;
		sq->SetStores(dataBlock.ids[i], n);
		}
	f = sq->GetFuel() - ((dataBlock.fuel * dataBlock.vehicles) / 100);
	if (f < 0)
		f = 0;
	sq->SetFuel (f);
	return 1;
	}

*/
#endif