#ifndef _GRAPHICSTEXTDISPLAYMSG_H
#define _GRAPHICSTEXTDISPLAYMSG_H

#include "F4vu.h"
#include "mission.h"
#include "FalcMesg.h"
#include "InvalidBufferException.h"

#pragma pack (1)

/*
 * Message Type Graphics Text Display
 */
class GraphicsTextDisplay : public FalconEvent
{
   public:
      GraphicsTextDisplay(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback=TRUE);
      GraphicsTextDisplay(VU_MSG_TYPE type, VU_ID senderid, VU_ID target);
      ~GraphicsTextDisplay(void);
      virtual int Size() const { return sizeof(dataBlock) + dataBlock.len + 1 + FalconEvent::Size();};
	  int Decode (VU_BYTE **buf, long *rem) {
		  long init = *rem;

		  FalconEvent::Decode (buf, rem);
		  memcpychk(&dataBlock, buf, sizeof (dataBlock), rem);

		  //sfr: we have to do this manually because datablock is a pointer...
		  //we should change this someday
		  //dataBlock.msg = (char*)(*buf);
		  dataBlock.msg = new char[dataBlock.len+1];
		  memcpychk(dataBlock.msg, buf, dataBlock.len+1, rem);
			
		  return init - *rem;
	  };
      int Encode (VU_BYTE **buf)
         {
         int size;

            size = FalconEvent::Encode (buf);
            memcpy (*buf, &dataBlock, sizeof (dataBlock));
            *buf += sizeof (dataBlock);
            size += sizeof (dataBlock);
            memcpy (*buf, dataBlock.msg, dataBlock.len);
            *buf += dataBlock.len;
            size += dataBlock.len;
            **buf = 0;
            *buf += 1;
            size += 1;
            return size;
         };
      class DATA_BLOCK
      {
         public:
            VU_ID target;
            int len;
            char* msg;
      } dataBlock;

   protected:
      int Process(uchar autodisp);
};

#pragma pack ()

#endif
