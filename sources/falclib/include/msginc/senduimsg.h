#ifndef _SENDUIMSG_H
#define _SENDUIMSG_H

#include "F4vu.h"
#include "FalcMesg.h"
#include "mission.h"
#include "InvalidBufferException.h"


#pragma pack (1)

/*
 * Message Type Send UI Message
 */
class UISendMsg : public FalconEvent
{
   public:

		enum
		{
			UnknownType=0,
			VC_Update,
			VC_GameOver,
			UpdateIter,
		};

      UISendMsg(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback=TRUE);
      UISendMsg(VU_MSG_TYPE type, VU_ID senderid, VU_ID target);
      ~UISendMsg(void);
      virtual int Size() const { return sizeof(dataBlock) + FalconEvent::Size();};
	  //sfr: changed to long*
	  int Decode (VU_BYTE **buf, long *rem)
	  {
		  long init = *rem;

		  FalconEvent::Decode (buf, rem);
		  memcpychk(&dataBlock, buf, sizeof (dataBlock), rem);
		  return init  - *rem;
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
            uchar msgType;
            long number;
            long value;
      } dataBlock;

   protected:
      int Process(uchar autodisp);
};
#pragma pack ()

#endif
