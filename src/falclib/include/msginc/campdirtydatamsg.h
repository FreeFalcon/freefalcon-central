/*
 * Machine Generated include file for message "Camp Dirty Data".
 * NOTE: This file is read only. DO NOT ATTEMPT TO MODIFY IT BY HAND.
 * Generated on 17-November-1998 at 20:52:31
 * Generated from file EVENTS.XLS by Robin Heydon
 */

#ifndef _CAMPDIRTYDATA_H
#define _CAMPDIRTYDATA_H

/*
 * Required Include Files
 */
#include "F4vu.h"
#include "FalcMesg.h"
#include "mission.h"
#pragma pack (1)

/*
 * Message Type Camp Dirty Data
 */
class CampDirtyData : public FalconEvent
{
   public:
      CampDirtyData(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback=TRUE);
      CampDirtyData(VU_MSG_TYPE type, VU_ID senderid, VU_ID target);
      ~CampDirtyData(void);
      virtual int Size (void) const;
	  //sfr: changed to long *
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
