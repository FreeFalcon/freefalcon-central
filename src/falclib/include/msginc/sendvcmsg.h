/*
 * Machine Generated include file for message "Send VC data".
 * NOTE: This file is read only. DO NOT ATTEMPT TO MODIFY IT BY HAND.
 * Generated on 28-November-1998 at 16:52:58
 * Generated from file EVENTS.XLS by Robin Heydon
 */

#ifndef _SENDVCMSG_H
#define _SENDVCMSG_H

/*
 * Required Include Files
 */
#include "F4vu.h"
#include "FalcMesg.h"
#include "mission.h"
#pragma pack (1)

/*
 * Message Type Send VC data
 */
class FalconSendVC : public FalconEvent
{
   public:
      FalconSendVC(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback=TRUE);
      FalconSendVC(VU_MSG_TYPE type, VU_ID senderid, VU_ID target);
      ~FalconSendVC(void);
      virtual int Size() const;
		//sfr long*
	  virtual int Decode (VU_BYTE **buf, long *rem);
      virtual int Encode (VU_BYTE **buf);
      class DATA_BLOCK
      {
         public:

            long size;
            uchar* data;
      } dataBlock;

   protected:
      int Process(uchar autodisp);
};
#pragma pack ()

#endif
