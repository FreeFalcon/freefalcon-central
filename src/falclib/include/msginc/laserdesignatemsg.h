#ifndef _LASERDESIGNATEMSG_H
#define _LASERDESIGNATEMSG_H

#include "F4vu.h"
#include "FalcMesg.h"
#include "mission.h"
#include "InvalidBufferException.h"

#pragma pack (1)

/*
 * Message Type Laser Designate Msg
 */
class FalconLaserDesignateMsg : public FalconEvent
{
   public:
      FalconLaserDesignateMsg(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback=TRUE);
      FalconLaserDesignateMsg(VU_MSG_TYPE type, VU_ID senderid, VU_ID target);
      ~FalconLaserDesignateMsg(void);
      virtual int Size() const { return sizeof(dataBlock) + FalconEvent::Size();};
	  //sfr: changed to long *
	  int Decode (VU_BYTE **buf, long *rem)
	  {
		  long int init = *rem;

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
            VU_ID source;
            VU_ID target;
            uchar state;
      } dataBlock;

   protected:
      int Process(uchar autodisp);
};
#pragma pack ()

#endif
