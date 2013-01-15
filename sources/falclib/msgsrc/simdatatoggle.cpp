#include "MsgInc/SimDataToggle.h"
#include "mesg.h"
#include "falclib.h"
#include "falcmesg.h"
#include "falcgame.h"
#include "falcsess.h"
#include "simmover.h"
#include "InvalidBufferException.h"

FalconSimDataToggle::FalconSimDataToggle(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) : FalconEvent (SimDataToggle, FalconEvent::SimThread, entityId, target, loopback)
{
	// Your Code Goes Here
	RequestReliableTransmit ();
	RequestOutOfBandTransmit ();
}

FalconSimDataToggle::FalconSimDataToggle(VU_MSG_TYPE type, VU_ID senderid, VU_ID target) : FalconEvent (SimDataToggle, FalconEvent::SimThread, senderid, target)
{
	// Your Code Goes Here
	type;
}

FalconSimDataToggle::~FalconSimDataToggle(void)
{
	// Your Code Goes Here
}

int FalconSimDataToggle::Process(uchar autodisp)
{
	SimMoverClass* theEntity;

	if (autodisp)
		return 0;

	theEntity = (SimMoverClass*)(vuDatabase->Find (dataBlock.entityID));
	if (theEntity && IsLocal())
	{
		theEntity->AddDataRequest(dataBlock.flag);
	}

	return TRUE;
}

