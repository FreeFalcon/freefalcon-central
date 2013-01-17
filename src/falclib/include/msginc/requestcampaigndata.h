/*
 * Machine Generated include file for message "Request Campaign Data".
 * NOTE: This file is read only. DO NOT ATTEMPT TO MODIFY IT BY HAND.
 * Generated on 30-July-1997 at 10:45:40
 * Generated from file EVENTS.XLS by Kevin Klemmick
 */

#ifndef _REQUESTCAMPAIGNDATA_H
#define _REQUESTCAMPAIGNDATA_H

/*
 * Required Include Files
 */
#include "F4vu.h"
#include "mission.h"
#include "FalcMesg.h"

#pragma pack (1)

/*
 * Message Type Request Campaign Data
 */
class FalconRequestCampaignData : public FalconEvent
{
   public:
      FalconRequestCampaignData(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback=TRUE);
      FalconRequestCampaignData(VU_MSG_TYPE type, VU_ID senderid, VU_ID target);
      ~FalconRequestCampaignData(void);
      virtual int Size() const;
		//sfr: long *
      virtual int Decode (VU_BYTE **buf, long *rem);
      virtual int Encode (VU_BYTE **buf);

      class DATA_BLOCK
      {
         public:

            VU_ID who;
            ulong dataNeeded;
			uchar size;
			uchar *data;
      } dataBlock;

	  FalconRequestCampaignData *nextRequest;

   protected:
      int Process(uchar autodisp);
};
#pragma pack ()

#endif
