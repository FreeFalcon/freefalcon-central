#ifndef _MISSIONREQUESTMSG_H
#define _MISSIONREQUESTMSG_H

#include "F4vu.h"
#include "mission.h"
#include "FalcMesg.h"
#include "InvalidBufferException.h"

#pragma pack (push, pack1, 1)

/*
 * Message Type Mission Request
 */
class FalconMissionRequestMessage : public FalconEvent
{
   public:
      FalconMissionRequestMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback=TRUE);
      FalconMissionRequestMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target);
      ~FalconMissionRequestMessage(void);
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
            ushort team;
            MissionRequestClass request;
      } dataBlock;

   protected:
      int Process(uchar autodisp);
};
#pragma pack (pop, pack1)

#endif
