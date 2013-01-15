/*
 * Machine Generated include file for message "TankerMsg".
 * NOTE: This file is read only. DO NOT ATTEMPT TO MODIFY IT BY HAND.
 * Generated on 01-November-1998 at 21:58:35
 * Generated from file EVENTS.XLS by MicroProse
 */

#ifndef _TANKERMSG_H
#define _TANKERMSG_H

/*
 * Required Include Files
 */
#include "F4vu.h"
#include "FalcMesg.h"
#include "mission.h"
#include "InvalidBufferException.h"

#pragma pack (1)

/*
 * Message Type TankerMsg
 */
class FalconTankerMessage : public FalconEvent
{
   public:
      enum TankerMsgCode {
         RequestFuel,
         ReadyForGas,
         DoneRefueling,
         Contact,
         Breakaway,
         PreContact,
         ClearToContact,
         Stabalize,
         BoomCommand,
         Disconnect,
         TankerTurn,
         PositionUpdate};

      FalconTankerMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback=TRUE);
      FalconTankerMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target);
      ~FalconTankerMessage(void);
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

            VU_ID caller;
            unsigned int type;
            float data1;
            float data2;
      } dataBlock;

   protected:
      int Process(uchar autodisp);
};
#pragma pack ()

#endif
