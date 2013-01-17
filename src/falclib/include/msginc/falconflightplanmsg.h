/*
 * Machine Generated include file for message "Flight Plan Message".
 * NOTE: This file is read only. DO NOT ATTEMPT TO MODIFY IT BY HAND.
 * Generated on 10-November-1998 at 16:18:21
 * Generated from file EVENTS.XLS by .
 */

#ifndef _FALCONFLIGHTPLANMSG_H
#define _FALCONFLIGHTPLANMSG_H

/*
 * Required Include Files
 */
#include "F4vu.h"
#include "FalcMesg.h"
#include "mission.h"
#pragma pack (1)

/*
 * Message Type Flight Plan Message
 */
class FalconFlightPlanMessage : public FalconEvent
{
   public:
      enum { waypointData, loadoutData, squadronStores };

      FalconFlightPlanMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback=TRUE);
      FalconFlightPlanMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target);
      ~FalconFlightPlanMessage(void);
      virtual int Size() const;
	  //sfr: changed to long *length
      //int Decode (VU_BYTE **buf, int length);
	  virtual int Decode (VU_BYTE **buf, long *rem);
      virtual int Encode (VU_BYTE **buf);

      class DATA_BLOCK
      {
         public:
			uchar type;
            long size;
            uchar* data;
      } dataBlock;

   protected:
      int Process(uchar autodisp);
};
#pragma pack ()

#endif
