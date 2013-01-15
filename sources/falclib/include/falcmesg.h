#ifndef _FALCLIB_H
#define _FALCLIB_H

#include "F4Vu.h"

#pragma pack(1)

typedef struct
{
   ushort size;
   ushort type;
} EventIdData;

#pragma pack ()

extern FILE* F4EventFile;

// ==================================
// Falcon 4 Event stuff
// ==================================

class FalconEvent : public VuMessage{
public:
	enum HandlingThread {
		NoThread                     = 0x0,				// This would be rather pointless
		SimThread                    = 0x1,
		CampaignThread               = 0x2,
		UIThread					 = 0x4,
		VuThread					 = 0x8,			// Realtime thread! carefull with what you send here
		AllThreads                   = 0xff
	};
	
	HandlingThread handlingThread;

	virtual int Size() const;
	virtual int Decode(VU_BYTE **buf, long *rem);
	virtual int Encode(VU_BYTE **buf);

protected:
	FalconEvent(
		VU_MSG_TYPE type, HandlingThread threadID, VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback=TRUE
	);
	FalconEvent(
		VU_MSG_TYPE type, HandlingThread threadID, VU_ID senderid, VU_ID target
	);
	virtual ~FalconEvent (void);
	virtual int Activate(VuEntity *ent);
	virtual int Process(uchar autodisp) = 0;

private:
	virtual int LocalSize() const;
};

// ==================================
// Falcon 4 Message filter
// ==================================
// sfr: why must all threads process Deletes??
#define MF_DONT_PROCESS_DELETE 1
class FalconMessageFilter : public VuMessageFilter {
public:
#if VU_USE_ENUM_FOR_TYPES
	/** which thread should get these messages and shall it process vu messages? */
	FalconMessageFilter(FalconEvent::HandlingThread theThread, bool processVu = false);
#else
	FalconMessageFilter(FalconEvent::HandlingThread theThread, ulong vuMessageBits);
#endif
	virtual ~FalconMessageFilter();
	virtual VU_BOOL Test(VuMessage *event) const;
	virtual VuMessageFilter *Copy() const;

private:
	FalconEvent::HandlingThread filterThread;
#if VU_USE_ENUM_FOR_TYPES
	bool processVu;
#else
	ulong vuFilterBits;
#endif
};

// ==================================
// Functions
// ==================================

void FalconSendMessage (VuMessage* theEvent, BOOL reliableTransmit = FALSE);

#endif
