#ifndef _SENDLOGBOOK_H
#define _SENDLOGBOOK_H

#include "vutypes.h"
#include "logbook.h"
#include "FalcMesg.h"
#include "InvalidBufferException.h"

#pragma pack (1)

/*
 * Message Type Send Logbook
 */
class UI_SendLogbook : public FalconEvent
{
   public:
      UI_SendLogbook(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback=TRUE);
      UI_SendLogbook(VU_MSG_TYPE type, VU_ID senderid, VU_ID target);
      ~UI_SendLogbook(void);
      virtual int Size() const { return sizeof(dataBlock) + FalconEvent::Size();};
		//sfr: changed to long *
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
			VU_ID fromID;
            LB_PILOT Pilot;
            long Flags;
      } dataBlock;

   protected:
      int Process(uchar autodisp);
};
#pragma pack ()

#endif
