/** sfr: requests a position update from unit owner for a simmover entity */
#include "MsgInc/RequestUnitPosition.h"
#include "MsgInc/SendUnitPosition.h"
#include "mesg.h"

// TODO discover what thread uses it...

RequestUnitPosition::RequestUnitPosition(SimMoverClass *mover, VU_ID target) : 
	FalconEvent(RequestUnitPositionMsg, FalconEvent::SimThread, mover->Id(), static_cast<VuTargetEntity*>(vuDatabase->Find(target)), FALSE)
{
	// this is always reliable
	this->RequestReliableTransmit();
}

RequestUnitPosition::RequestUnitPosition(VU_ID senderID, VU_ID targetID) :
	FalconEvent(RequestUnitPositionMsg, FalconEvent::SimThread, senderID, targetID) 
{

}

RequestUnitPosition::~RequestUnitPosition(){

}

int RequestUnitPosition::Size (void){
	return FalconEvent::Size();
}

int RequestUnitPosition::Decode (VU_BYTE **buf, long *rem){
	return FalconEvent::Decode(buf, rem);
}

int RequestUnitPosition::Encode (VU_BYTE **buf){
	return FalconEvent::Encode(buf);
}

int RequestUnitPosition::Process(uchar autodisp){
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

	FalconEvent *sup = new SendUnitPosition(mover, sender);
	sup->Send();
	return 0;
}
