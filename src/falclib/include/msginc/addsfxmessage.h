#ifndef _ADDSFXMESSAGE_H
#define _ADDSFXMESSAGE_H

#include "F4vu.h"
#include "falcmesg.h"
#include "mission.h"
#include "InvalidBufferException.h"

#pragma pack (1)

/*
 * Message Type Add SFX Message
 */
class FalconAddSFXMessage : public FalconEvent
{
   public:
      FalconAddSFXMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback=TRUE);
      FalconAddSFXMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target);
      ~FalconAddSFXMessage(void);
      virtual int Size() const { return sizeof(dataBlock) + FalconEvent::Size();}

	  int Decode (VU_BYTE **buf, long *rem)
	  {
		  int init = *rem;

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

            short type;
            short visType;
            short flags;
            CampaignTime time;
            float xLoc;
            float yLoc;
            float zLoc;
            float xVel;
            float yVel;
            float zVel;
      } dataBlock;

   protected:
      int Process(uchar autodisp);
};

#pragma pack ()

#endif
