#include "MsgInc/RegenerationMsg.h"
#include "mesg.h"
#include "falclib.h"
#include "falcmesg.h"
#include "falcgame.h"
#include "falcsess.h"
#include "simbase.h"
#include "dogfight.h"

//sfr: added here for checks
#include "InvalidBufferException.h"

extern int g_nDFRegenerateFix;

FalconRegenerationMessage::FalconRegenerationMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) : FalconEvent (RegenerationMsg, FalconEvent::SimThread, entityId, target, loopback)
{
	// Your Code Goes Here
	RequestReliableTransmit ();
	RequestOutOfBandTransmit ();
}

FalconRegenerationMessage::FalconRegenerationMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target) : FalconEvent (RegenerationMsg, FalconEvent::SimThread, senderid, target){
}

FalconRegenerationMessage::~FalconRegenerationMessage(void)
{
   // Your Code Goes Here
}

int FalconRegenerationMessage::Process(uchar autodisp)
{
	if (autodisp)
		return 0;

	SimBaseClass		*theObject = (SimBaseClass*) Entity();

	if (theObject){

		// 2002-04-10 MN Why do we wait until something else sets he entity to dead 
		// if we want to regenerate it ??? Crap! 
		// Let's just make the not yet dead object dead and regenerate it again
		// This fixes the infamous respawning bug in Dogfights.
		if (!(g_nDFRegenerateFix & 0x01)){
			if (!theObject->IsDead ()){
				MonoPrint ("Delaying Regeneration Message\n");
				// sfr: flag bug, setting all but loopback :/
				//this->flags_ |= ~VU_LOOPBACK_MSG_FLAG;
				this->flags_ &= ~VU_LOOPBACK_MSG_FLAG;
				VuTimerEvent *timer = new VuTimerEvent(0, vuxRealTime + 1000, VU_DELAY_TIMER, this);
				VuMessageQueue::PostVuMessage(timer);
			
				return 0;
			}

		}
		else {	
			if (!theObject->IsDead()){
				theObject->SetDead(true);
			}
		}

		// Regenerate the object
		if (theObject){
			theObject->Regenerate(dataBlock.newx, dataBlock.newy, dataBlock.newz, dataBlock.newyaw);
		}

		// Reset our fly state if we've been regenerated and we are in Match play
		if (
			theObject && 
			theObject == FalconLocalSession->GetPlayerEntity() && 
			SimDogfight.GetGameType() == dog_TeamMatchplay
		){
			FalconLocalSession->SetFlyState(FLYSTATE_WAITING);
		}
	}

	return 0;
}

