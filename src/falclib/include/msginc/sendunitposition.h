#ifndef SEND_UNIT_POSITION
#define SEND_UNIT_POSITION

/** sfr: this message is a little different from regular position updates message.
* Its an answer to RequestUnitPosition message
* Its not interpreted by driver. Its used when client needs a unit position from its current owner
* before client owns it.
*/

#include "Falcmesg.h"
#include "Sim/Include/simmover.h"


class SendUnitPosition : public FalconEvent {
public:
	// sender constructor
	SendUnitPosition(SimMoverClass *mover, VuTargetEntity *target);
	// receiver constructor
	SendUnitPosition(VU_ID senderID, VU_ID target);
	// destructor
	~SendUnitPosition();

	// stream functions
	int Size (void);
	int Decode (VU_BYTE **buf, long *rem);
	int Encode (VU_BYTE **buf);

protected:
	int Process(uchar autodisp);

	// position info
	SM_SCALAR yaw_, pitch_, roll_;
	SM_SCALAR dyaw_, dpitch_, droll_;
	BIG_SCALAR x_, y_, z_;
	SM_SCALAR dx_, dy_, dz_;

};

#endif