/** sfr: requests a position update from unit owner for a simmover entity */
#include "MsgInc/RequestSimMoverPosition.h"
#include "MsgInc/SendSimMoverPosition.h"
#include "mesg.h"

// TODO discover what thread uses it...

RequestSimMoverPosition::RequestSimMoverPosition(SimMoverClass *mover, VU_ID target) : 
	FalconEvent(RequestSimMoverPositionMsg, FalconEvent::SimThread, mover->Id(), static_cast<VuTargetEntity*>(vuDatabase->Find(target)), FALSE)
{
	// this is always reliable and OOB
	RequestReliableTransmit();
	RequestOutOfBandTransmit();
}

RequestSimMoverPosition::RequestSimMoverPosition(VU_ID senderID, VU_ID targetID) :
	FalconEvent(RequestSimMoverPositionMsg, FalconEvent::SimThread, senderID, targetID) 
{
}

RequestSimMoverPosition::~RequestSimMoverPosition(){
}

int RequestSimMoverPosition::Size() const {
	return FalconEvent::Size();
}

int RequestSimMoverPosition::Decode (VU_BYTE **buf, long *rem){
	return FalconEvent::Decode(buf, rem);
}

int RequestSimMoverPosition::Encode (VU_BYTE **buf){
	return FalconEvent::Encode(buf);
}

int RequestSimMoverPosition::Process(uchar autodisp){
	if (autodisp){
		return 0;
	}

	SimMoverClass *mover = static_cast<SimMoverClass *>(this->Entity());
	if (mover == NULL){
		return 0;
	}

	VuTargetEntity *sender = static_cast<VuTargetEntity*>(vuDatabase->Find(sender_));
	if (sender == NULL){
		return 0;
	}

	FalconEvent *sup = new SendSimMoverPosition(mover, sender);
	FalconSendMessage(sup, true);
	return 0;
}
