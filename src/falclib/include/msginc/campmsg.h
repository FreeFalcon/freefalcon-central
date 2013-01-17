/*
 * Machine Generated include file for message "Camp Message".
 * NOTE: This file is read only. DO NOT ATTEMPT TO MODIFY IT BY HAND.
 * Generated on 01-April-1997 at 18:56:59
 * Generated from file EVENTS.XLS by Kevin Klemmick
 */

#ifndef _CAMPMSG_H
#define _CAMPMSG_H

/*
 * Required Include Files
 */
#include "FalcMesg.h"
#include "mission.h"

#include "Falcmesg.h"

//sfr: added for checks
#include "InvalidBufferException.h"

#pragma pack (1)

/*
 * Message Type Camp Message
 */
class FalconCampMessage : public FalconEvent
{
   public:
      enum CampMsgType {
		 campAttackWarning,
         campFiredOn,
         campSpotted,
         campRepair,
         campReaggregate,
         campDeaggregate };

      FalconCampMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback=TRUE);
      FalconCampMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target);
      ~FalconCampMessage(void);
      virtual int Size() const { return sizeof(dataBlock) + FalconEvent::Size();};
	  int Decode (VU_BYTE **buf, long *rem) {
		  long init = *rem;

		  FalconEvent::Decode (buf, rem);
		  memcpychk (&dataBlock, buf, sizeof (dataBlock), rem);
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
            VU_ID from;
            unsigned int message;
            short data1;
            short data2;
            short data3;
            short data4;
      } dataBlock;

   protected:
      int Process(uchar autodisp);
};

#pragma pack ()

#endif
