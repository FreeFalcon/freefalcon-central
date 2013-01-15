#ifndef _TIMINGMSG_H
#define _TIMINGMSG_H

/*
 * Required Include Files
 */
#include "F4vu.h"
#include "mission.h"
#include "FalcMesg.h"

#pragma pack (1)

/*
 * Message Type Timing Message
 */
class FalconTimingMessage : public FalconEvent
{
public:
      FalconTimingMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback=TRUE);
	  FalconTimingMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target);
      ~FalconTimingMessage(void);
      virtual int Size() const { return sizeof(dataBlock) + FalconEvent::Size();};
      int Decode (VU_BYTE **buf, long *rem);
      int Encode (VU_BYTE **buf);

	class DATA_BLOCK {
         public:
			ulong targetTime;
			char  compressionRatio;
      } dataBlock;

protected:
      int Process(uchar autodisp);
};
#pragma pack ()

#endif
