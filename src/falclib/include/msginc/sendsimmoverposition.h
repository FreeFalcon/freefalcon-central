#ifndef SEND_UNIT_POSITION
#define SEND_UNIT_POSITION

/** sfr: this message is a little different from regular position updates message.
* Its an answer to RequestSimMoverPosition message
* Its not interpreted by driver. Its used when client needs a unit position from its current owner
* before client owns it.
*/

#include "Falcmesg.h"
#include "Sim/Include/simmover.h"

// byte alignment
#pragma pack (1)

class SendSimMoverPosition : public FalconEvent {
public:
	// sender constructor
	SendSimMoverPosition(SimMoverClass *mover, VuTargetEntity *target);
	// receiver constructor
	SendSimMoverPosition(VU_ID senderID, VU_ID target);
	// destructor
	~SendSimMoverPosition();

	// stream functions
	virtual int Size() const;
	virtual int Decode(VU_BYTE **buf, long *rem);
	virtual int Encode(VU_BYTE **buf);

protected:
	int Process(uchar autodisp);

	// position info
	SM_SCALAR yaw_, pitch_, roll_;
	SM_SCALAR dyaw_, dpitch_, droll_;
	BIG_SCALAR x_, y_, z_;
	SM_SCALAR dx_, dy_, dz_;

};
#pragma pack ()

#endif