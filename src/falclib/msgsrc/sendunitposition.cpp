#include "MsgInc/SendUnitPosition.h"
#include "mesg.h"
#include "InvalidBufferException.h"

using namespace std;

// TODO discover what thread uses it...

SendUnitPosition::SendUnitPosition(SimMoverClass *mover, VuTargetEntity *target) : 
	FalconEvent(SendUnitPositionMsg, FalconEvent::SimThread, mover->Id(), target, FALSE)
{
	// this is always reliable
	this->RequestReliableTransmit();
	x_ = mover->XPos();
	y_ = mover->YPos();
	z_ = mover->ZPos();
	dx_ = mover->XDelta();
	dy_ = mover->YDelta();
	dz_ = mover->ZDelta();
	yaw_ = mover->Yaw();
	pitch_ = mover->Pitch();
	roll_ = mover->Roll();
	dyaw_ = mover->YawDelta();
	dpitch_ = mover->PitchDelta();
	droll_ = mover->RollDelta();

}

SendUnitPosition::SendUnitPosition(VU_ID senderID, VU_ID targetID) :
	FalconEvent(SendUnitPositionMsg, FalconEvent::SimThread, senderID, targetID) 
{

}

SendUnitPosition::~SendUnitPosition(){

}

int SendUnitPosition::Size (void){
	return FalconEvent::Size() + sizeof(SM_SCALAR)*9 + sizeof(BIG_SCALAR)*3;
}

int SendUnitPosition::Decode (VU_BYTE **buf, long *rem){
	FalconEvent::Decode(buf, rem);

	memcpychk(&yaw_, buf, sizeof(yaw_), rem);
	memcpychk(&pitch_, buf, sizeof(pitch_), rem);
	memcpychk(&roll_, buf, sizeof(roll_), rem);
	memcpychk(&dyaw_, buf, sizeof(dyaw_), rem);
	memcpychk(&dpitch_, buf, sizeof(dpitch_), rem);
	memcpychk(&droll_, buf, sizeof(droll_), rem);
	memcpychk(&x_, buf, sizeof(x_), rem);
	memcpychk(&y_, buf, sizeof(y_), rem);
	memcpychk(&z_, buf, sizeof(z_), rem);
	memcpychk(&dx_, buf, sizeof(dx_), rem);
	memcpychk(&dy_, buf, sizeof(dy_), rem);
	memcpychk(&dz_, buf, sizeof(dz_), rem);

	return Size();
}

int SendUnitPosition::Encode (VU_BYTE **buf){
	FalconEvent::Encode(buf);

	memcpy(*buf, &yaw_, sizeof(yaw_));      *buf += sizeof(yaw_);
	memcpy(*buf, &pitch_, sizeof(pitch_));  *buf += sizeof(pitch_);
	memcpy(*buf, &roll_, sizeof(roll_));    *buf += sizeof(roll_);
	memcpy(*buf, &dyaw_, sizeof(yaw_));     *buf += sizeof(dyaw_);
	memcpy(*buf, &dpitch_, sizeof(pitch_)); *buf += sizeof(dpitch_);
	memcpy(*buf, &droll_, sizeof(roll_));   *buf += sizeof(droll_);
	memcpy(*buf, &x_, sizeof(x_));          *buf += sizeof(x_);
	memcpy(*buf, &y_, sizeof(y_));          *buf += sizeof(y_);
	memcpy(*buf, &z_, sizeof(z_));          *buf += sizeof(z_);
	memcpy(*buf, &dx_, sizeof(dx_));        *buf += sizeof(dx_);
	memcpy(*buf, &dy_, sizeof(dy_));        *buf += sizeof(dy_);
	memcpy(*buf, &dz_, sizeof(dz_));        *buf += sizeof(dz_);

	return Size();
}

int SendUnitPosition::Process(uchar autodisp){
	if (autodisp){
		return 0;
	}

	SimMoverClass *mover = static_cast<SimMoverClass *>(this->Entity());
	if (mover == NULL){
		return 0;
	}

	mover->SetPosition(x_, y_, z_);
	mover->SetDelta(dx_, dy_, dz_);
	mover->SetYPR(yaw_, pitch_, roll_);
	mover->SetYPRDelta(dyaw_, dpitch_, droll_);

	// were done...
	mover->PositionUpdateDone();

	return 0;
}

