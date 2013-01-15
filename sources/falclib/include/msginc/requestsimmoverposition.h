#ifndef REQ_POS_UPDATE
#define REQ_POS_UPDATE

/** sfr: message to request a position update from unit owner */

#include "Falcmesg.h"
#include "Sim/Include/simmover.h"

// byte alignment
#pragma pack (1)


class RequestSimMoverPosition : public FalconEvent {
public:
	// sender constructor
	RequestSimMoverPosition(SimMoverClass *mover, VU_ID target);
	// receiver constructor
	RequestSimMoverPosition(VU_ID senderID, VU_ID target);
	// destructor
	~RequestSimMoverPosition();

	// stream functions
	virtual int Size(void) const;
	virtual int Decode (VU_BYTE **buf, long *rem);
	virtual int Encode (VU_BYTE **buf);

protected:
	int Process(uchar autodisp);
};
#pragma pack ()

#endif