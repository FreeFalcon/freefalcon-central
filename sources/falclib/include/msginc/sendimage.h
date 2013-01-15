#ifndef _SENDIMAGE_H
#define _SENDIMAGE_H

#include "F4vu.h"
#include "FalcMesg.h"
#include "mission.h"

#include "InvalidBufferException.h"

#pragma pack (1)

/*
 * Message Type Send Image
 */
class UI_SendImage : public FalconEvent
{
   public:
      UI_SendImage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback=TRUE);
      UI_SendImage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target);
      ~UI_SendImage(void);
      int Size (void)
		{
			int size=FalconEvent::Size();
			size += sizeof(VU_ID) +
					sizeof(uchar) +
					sizeof(short) +
					sizeof(short) +
					sizeof(long) +
					sizeof(long) +
					dataBlock.blockSize;
			return(size);
		}

		//sfr: changed to long *
	  int Decode (VU_BYTE **buf, long *rem)
	  {
		  long init = *rem;

		  FalconEvent::Decode (buf, rem);
		  memcpychk (&dataBlock.fromID, buf, sizeof (VU_ID), rem);
		  memcpychk (&dataBlock.typeID, buf, sizeof (uchar), rem);
		  memcpychk (&dataBlock.blockNo, buf, sizeof (short), rem);
		  memcpychk (&dataBlock.blockSize, buf, sizeof (short), rem);
		  memcpychk (&dataBlock.offset, buf, sizeof (long), rem);
		  memcpychk (&dataBlock.size, buf, sizeof (long), rem);
		  dataBlock.data=new uchar[dataBlock.blockSize];
		  memcpychk (dataBlock.data, buf, dataBlock.blockSize, rem);
		  return init - *rem;
	  };
      int Encode (VU_BYTE **buf)
         {
         int size;

            size = FalconEvent::Encode (buf);
            memcpy (*buf, &dataBlock.fromID, sizeof (VU_ID));	*buf += sizeof (VU_ID);		size += sizeof (VU_ID);
            memcpy (*buf, &dataBlock.typeID, sizeof (uchar));	*buf += sizeof (uchar);		size += sizeof (uchar);
            memcpy (*buf, &dataBlock.blockNo, sizeof (short));	*buf += sizeof (short);		size += sizeof (short);
            memcpy (*buf, &dataBlock.blockSize, sizeof (short));*buf += sizeof (short);		size += sizeof (short);
            memcpy (*buf, &dataBlock.offset, sizeof (long));	*buf += sizeof (long);		size += sizeof (long);
            memcpy (*buf, &dataBlock.size, sizeof (long));		*buf += sizeof (long);		size += sizeof (long);
            memcpy (*buf, dataBlock.data, dataBlock.blockSize);	*buf += dataBlock.blockSize;size += dataBlock.blockSize;
            return size;
         };
      class DATA_BLOCK
      {
         public:
			VU_ID fromID;
            uchar typeID;
            short blockNo;
			short blockSize;
			long  offset;
            long  size;
			uchar *data;
      } dataBlock;

   protected:
      int Process(uchar autodisp);
};
#pragma pack ()

#endif
