#ifndef _VOICEDATAMSG_H
#define _VOICEDATAMSG_H

#include "F4vu.h"
#include "mission.h"
#include "FalcMesg.h"
#include "InvalidBufferException.h"

#pragma pack (1)

/*
 * Message Type Voice Data Message
 */
class FalconVoiceDataMessage : public FalconEvent
{
   public:
      FalconVoiceDataMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback=TRUE);
      FalconVoiceDataMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target);
      ~FalconVoiceDataMessage(void);
      virtual int Size() const { return sizeof(dataBlock) + FalconEvent::Size();};

	  //sfr: changed long *
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

					  ushort id;
					  ushort size;
					  uchar* data;
	  } dataBlock;

   protected:
	  int Process(uchar autodisp);
};
#pragma pack ()

#endif
