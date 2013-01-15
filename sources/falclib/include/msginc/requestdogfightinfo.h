#ifndef _REQUESTDOGFIGHTINFO_H
#define _REQUESTDOGFIGHTINFO_H

#include "F4vu.h"
#include "mission.h"
#include "uicomms.h"
#include "FalcMesg.h"
#include "InvalidBufferException.h"

#pragma pack (1)

/*
 * Message Type Request Dogfight Info
 */
class UI_RequestDogfightInfo : public FalconEvent
{
   public:
      UI_RequestDogfightInfo(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback=TRUE);
      UI_RequestDogfightInfo(VU_MSG_TYPE type, VU_ID senderid, VU_ID target);
      ~UI_RequestDogfightInfo(void);
      virtual int Size() const { return sizeof(dataBlock) + FalconEvent::Size();};
	  int Decode (VU_BYTE **buf, long *rem)
	  {
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

            VU_ID requester_id;
      } dataBlock;

   protected:
      int Process(uchar autodisp);
};
#pragma pack ()

#endif
