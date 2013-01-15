#ifndef _LANDINGMESSAGE_H
#define _LANDINGMESSAGE_H

#include "F4vu.h"
#include "mission.h"
#include "FalcMesg.h"


//sfr: for chks
#include "InvalidBufferException.h"

#pragma pack (1)

/*
 * Message Type Landing Message
 */
class FalconLandingMessage : public FalconEvent
{
   public:
      FalconLandingMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback=TRUE);
      FalconLandingMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target);
      ~FalconLandingMessage(void);
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
					  ushort campID;
					  uchar pilotID;
	  } dataBlock;

   protected:
      int Process(uchar autodisp);
};

#pragma pack ()

#endif
