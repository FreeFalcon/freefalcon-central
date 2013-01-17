#ifndef _CAMPEVENTDATAMSG_H
#define _CAMPEVENTDATAMSG_H

#include "F4vu.h"
#include "FalcMesg.h"
#include "mission.h"
#include "InvalidBufferException.h"

#pragma pack (1)

/*
 * Message Type Camp Event Data
 */
class CampEventDataMessage : public FalconEvent
{
   public:
      enum eventMsgType {
         eventMessage,
         victoryConditionMessage,
		 playMovie };

      CampEventDataMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback=TRUE);
      CampEventDataMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target);
      ~CampEventDataMessage(void);
      virtual int Size() const { return sizeof(dataBlock) + FalconEvent::Size();};
	  //sfr: long *
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
            unsigned int message;
            short event;
            uchar status;
      } dataBlock;

   protected:
      int Process(uchar autodisp);
};
#pragma pack ()

#endif
