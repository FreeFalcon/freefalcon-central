/*
 * Machine Generated include file for message "Send Campaign data".
 * NOTE: This file is read only. DO NOT ATTEMPT TO MODIFY IT BY HAND.
 * Generated on 01-April-1997 at 18:57:00
 * Generated from file EVENTS.XLS by Kevin Klemmick
 */

#ifndef _SENDCAMPAIGNMSG_H
#define _SENDCAMPAIGNMSG_H

/*
 * Required Include Files
 */
#include "FalcMesg.h"

#include "F4vu.h"
#include "mission.h"
#pragma pack (1)

/*
 * Message Type Send Campaign data
 */
class FalconSendCampaign : public FalconEvent
{
   public:
      FalconSendCampaign(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback=TRUE);
      FalconSendCampaign(VU_MSG_TYPE type, VU_ID senderid, VU_ID target);
      ~FalconSendCampaign(void);
      virtual int Size (void) const;
	  //sfr: changed to long *
      virtual int Decode (VU_BYTE **buf, long  *rem);
      virtual int Encode (VU_BYTE **buf);

      class DATA_BLOCK
      {
         public:

            ulong campTime;
            VU_ID from;
			short dataSize;
            uchar* campInfo;
      } dataBlock;

   protected:
      int Process(uchar autodisp);
};
#pragma pack ()

#endif
