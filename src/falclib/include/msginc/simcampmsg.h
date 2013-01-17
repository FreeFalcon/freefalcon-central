/*
 * Machine Generated include file for message "Sim Camp Message".
 * NOTE: This file is read only. DO NOT ATTEMPT TO MODIFY IT BY HAND.
 * Generated on 03-August-1998 at 13:53:14
 * Generated from file EVENTS.XLS by MicroProse
 */

#ifndef _SIMCAMPMSG_H
#define _SIMCAMPMSG_H

/*
 * Required Include Files
 */
#include "F4vu.h"
#include "FalcMesg.h"
#include "mission.h"
#pragma pack (1)

/*
 * Message Type Sim Camp Message
 */
class FalconSimCampMessage : public FalconEvent
{
   public:
      enum SimCampMsgType {
         simcampReaggregate,
         simcampDeaggregate,
         simcampChangeOwner,
         simcampReaggregateFromData,
         simcampDeaggregateFromData,
         simcampChangeOwnerFromData,
		 simcampRequestDeagData,
	     simcampRequestAllDeagData
	  };

      FalconSimCampMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback=TRUE);
      FalconSimCampMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target);
      ~FalconSimCampMessage(void);
      virtual int Size() const;
	  //sfr: changed to long *
      virtual int Decode (VU_BYTE **buf, long *rem);
      virtual int Encode (VU_BYTE **buf);

      class DATA_BLOCK
      {
         public:

            VU_ID from;
            unsigned int message;
            ushort size;
            uchar* data;
      } dataBlock;

   protected:
      int Process(uchar autodisp);
};
#pragma pack ()

#endif
