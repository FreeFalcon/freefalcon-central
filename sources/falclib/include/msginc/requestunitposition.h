#ifndef REQ_POS_UPDATE
#define REQ_POS_UPDATE

/** sfr: message to request a position update from unit owner */

#include "Falcmesg.h"
#include "Sim/Include/simmover.h"

class RequestUnitPosition : public FalconEvent {
public:
	// sender constructor
	RequestUnitPosition(SimMoverClass *mover, VU_ID target);
	// receiver constructor
	RequestUnitPosition(VU_ID senderID, VU_ID target);
	// destructor
	~RequestUnitPosition();

	// stream functions
	int Size (void);
	int Decode (VU_BYTE **buf, long *rem);
	int Encode (VU_BYTE **buf);

protected:
	int Process(uchar autodisp);

};


#endif