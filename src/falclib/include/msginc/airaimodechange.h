#ifndef _AIRAIMODECHANGE_H
#define _AIRAIMODECHANGE_H

#include "F4vu.h"
#include "mission.h"
#include "InvalidBufferException.h"
#include "Falcmesg.h"

#pragma pack (1)

/*
 * Message Type Air AI Mode Change
 */
class AirAIModeMsg : public FalconEvent
{
   public:
      AirAIModeMsg(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback=TRUE);
      AirAIModeMsg(VU_MSG_TYPE type, VU_ID senderid, VU_ID target);
      ~AirAIModeMsg(void);
      virtual int Size() const { return sizeof(dataBlock) + FalconEvent::Size();}
	  int Decode (VU_BYTE **buf, long *rem){
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
            return size;
         };
      class DATA_BLOCK
      {
         public:

            VU_ID whoDidIt;
            int newMode;
      } dataBlock;

   protected:
      int Process(uchar autodisp);
};

#pragma pack ()

#endif
