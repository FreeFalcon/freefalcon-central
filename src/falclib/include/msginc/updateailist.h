/*
 * Machine Generated include file for message "Update DF Ai list".
 * NOTE: This file is read only. DO NOT ATTEMPT TO MODIFY IT BY HAND.
 * Generated on 20-January-1998 at 22:41:31
 * Generated from file EVENTS.XLS by Peter
 */

#ifndef _UPDATEAILIST_H
#define _UPDATEAILIST_H

/*
 * Required Include Files
 */
#include "F4vu.h"
#include "mission.h"
#include "FalcMesg.h"

#pragma pack (1)

/*
 * Message Type Update DF Ai list
 */
class UI_UpdateAIList : public FalconEvent
{
   public:
      UI_UpdateAIList(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback=TRUE);
      UI_UpdateAIList(VU_MSG_TYPE type, VU_ID senderid, VU_ID target);
      ~UI_UpdateAIList(void);
      int Size (void);
		//sfr: long *
	  int Decode (VU_BYTE **buf, long *rem);
      int Encode (VU_BYTE **buf);
      class DATA_BLOCK
      {
         public:

            VU_ID gameID;
            ushort count;
            ushort size;
            uchar* data;
      } dataBlock;

   protected:
      int Process(uchar autodisp);
};
#pragma pack ()

#endif
