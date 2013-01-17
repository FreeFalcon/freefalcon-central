/*
 * Machine Generated source file for message "Add SFX Message".
 * NOTE: The functions here must be completed by hand.
 * Generated on 29-July-1997 at 17:49:00
 * Generated from file EVENTS.XLS by Kevin Klemmick
 */

#include "MsgInc/AddSFXMessage.h"
#include "mesg.h"
#include "Sim/Include/SFX.h"
#include "falclib.h"
#include "falcmesg.h"
#include "falcgame.h"
#include "falcsess.h"
#include "InvalidBufferException.h"

extern void NewTimedPersistantObject (int vistype, CampaignTime removalTime, float x, float y);

extern void NewLinkedPersistantObject (int vistype, VU_ID campObjID, int campIdx, float x, float y);

FalconAddSFXMessage::FalconAddSFXMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) : FalconEvent (AddSFXMessage, FalconEvent::SimThread, entityId, target, loopback)
{
   // Your Code Goes Here
}

FalconAddSFXMessage::FalconAddSFXMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target) : FalconEvent (AddSFXMessage, FalconEvent::SimThread, senderid, target)
{
   // Your Code Goes Here
	type;
}

FalconAddSFXMessage::~FalconAddSFXMessage(void)
{
   // Your Code Goes Here
}

int FalconAddSFXMessage::Process(uchar autodisp)
	{
	if (autodisp)
		return 0;

	switch(dataBlock.type)
		{
		case SFX_TIMED_PERSISTANT:
			NewTimedPersistantObject (dataBlock.visType, dataBlock.time, dataBlock.xLoc, dataBlock.yLoc);
			break;
		case SFX_LINKED_PERSISTANT:
			NewLinkedPersistantObject (dataBlock.visType, EntityId(), dataBlock.time, dataBlock.xLoc, dataBlock.yLoc);
			break;
		default:
			break;
		}
	return 0;
	}

