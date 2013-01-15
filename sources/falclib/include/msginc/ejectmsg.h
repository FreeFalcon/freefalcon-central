#ifndef _EJECTMSG_H
#define _EJECTMSG_H

#include "F4vu.h"
#include "mission.h"
#include "FalcMesg.h"
#include "InvalidBufferException.h"

#pragma pack (1)

/*
 * Message Type Eject Message
 */
class FalconEjectMessage : public FalconEvent
{
   public:
      FalconEjectMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback=TRUE);
      FalconEjectMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target);
      ~FalconEjectMessage(void);
      virtual int Size() const { return sizeof(dataBlock) + FalconEvent::Size();};
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
            size += sizeof (dataBlock);
            return size;
         };
      class DATA_BLOCK
      {
         public:
			 
			 VU_ID	ePlaneID;
//			 VU_ID	eEjectID;
			 VU_ID   eFlightID;
			 ushort	eCampID;
//          ushort	eIndex;
			 uchar	ePilotID;
//			 uchar	eSide;
			 uchar	hadLastShooter;
     } dataBlock;

   protected:
      int Process(uchar autodisp);
};

#pragma pack ()

#endif
