/*
 * Machine Generated include file for message "Send Unit Data".
 * NOTE: This file is read only. DO NOT ATTEMPT TO MODIFY IT BY HAND.
 * Generated on 24-November-1997 at 21:03:10
 * Generated from file EVENTS.XLS by MicroProse
 */

#ifndef _SENDUNITDATA_H
#define _SENDUNITDATA_H

/*
 * Required Include Files
 */
#include "F4vu.h"
#include "mission.h"
#include "FalcMesg.h"

#pragma pack (1)

/*
 * Message Type Send Unit Data
 */
class FalconSendUnitData : public FalconEvent
{
   public:
      FalconSendUnitData(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback=TRUE);
      FalconSendUnitData(VU_MSG_TYPE type, VU_ID senderid, VU_ID target);
      ~FalconSendUnitData(void);
      virtual int Size() const;
		//sfr: changed to long *
	  virtual int Decode(VU_BYTE **buf, long *rem);
      virtual int Encode(VU_BYTE **buf);

      class DATA_BLOCK
      {
         public:

			VU_ID owner;
			short set;
            short size;
            ulong totalSize;
            uchar block;
            uchar totalBlocks;
            void* unitData;
      } dataBlock;

   protected:
      int Process(uchar autodisp);
};

#pragma pack ()

void SendCampaignUnitData (FalconSessionEntity *session, VuTargetEntity *target, uchar *blocksNeeded);

#endif
