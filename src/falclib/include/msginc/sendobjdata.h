/*
 * Machine Generated include file for message "Send Objective Data".
 * NOTE: This file is read only. DO NOT ATTEMPT TO MODIFY IT BY HAND.
 * Generated on 19-September-1997 at 17:15:07
 * Generated from file EVENTS.XLS by MicroProse
 */

#ifndef _SENDOBJDATA_H
#define _SENDOBJDATA_H

/*
 * Required Include Files
 */
#include "F4vu.h"
#include "mission.h"
#include "FalcMesg.h"

#pragma pack (1)

/*
 * Message Type Send Objective Data
 */
class FalconSendObjData : public FalconEvent
{
   public:
      FalconSendObjData(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback=TRUE);
      FalconSendObjData(VU_MSG_TYPE type, VU_ID senderid, VU_ID target);
      ~FalconSendObjData(void);
      virtual int Size (void) const;
		//sfr: changed to long *
	  virtual int Decode (VU_BYTE **buf, long *rem);
      virtual int Encode (VU_BYTE **buf);

      class DATA_BLOCK
      {
         public:

			VU_ID owner;
			short set;
            short size;
			ulong totalSize;
            uchar block;
            uchar totalBlocks;
            void* objData;
      } dataBlock;

   protected:
      int Process(uchar autodisp);
};

void SendObjectiveDeltas (FalconSessionEntity *session, VuTargetEntity *target, uchar *blocksNeeded);

#pragma pack ()

#endif
