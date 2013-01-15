#ifndef _AIRTASKINGMSG_H
#define _AIRTASKINGMSG_H

#include "F4vu.h"
#include "mission.h"
#include "Falcmesg.h"
#include "InvalidBufferException.h"

#pragma pack (1)

/*
 * Message Type Air Tasking Message
 */
class FalconAirTaskingMessage : public FalconEvent
{
   public:
      enum ATMMsgType {
         atmNewACAvail,
         atmAircraftRequest,
         atmAssignMission,
         atmAssignDivert,
         atmCompleteMission,
         atmScheduleAircraft,
         atmZapAirbase};

      FalconAirTaskingMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback=TRUE);
      FalconAirTaskingMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target);
      ~FalconAirTaskingMessage(void);
      virtual int Size() const { return sizeof(dataBlock) + FalconEvent::Size();};
	  int Decode (VU_BYTE **buf, long *rem) {
		  long init = *rem;

		  FalconEvent::Decode (buf, rem);
		  memcpychk(&dataBlock, buf, sizeof (dataBlock), rem);
		  return init - *rem;
	  };
	  int Encode (VU_BYTE **buf)
         {
         int size;

            size = FalconEvent::Encode (buf);
            memcpy (*buf, &dataBlock, sizeof (dataBlock));
            *buf += sizeof (dataBlock);
            size += sizeof (dataBlock);
            return size;
         };
      class DATA_BLOCK
      {
         public:

            VU_ID from;
            VU_ID to;
            uchar team;
            unsigned int messageType;
            short data1;
            short data2;
            void* data3;
      } dataBlock;

   protected:
      int Process(uchar autodisp);
};

#pragma pack ()

#endif
