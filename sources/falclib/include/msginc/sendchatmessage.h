/*
 * Machine Generated include file for message "Send Chat Message".
 * NOTE: This file is read only. DO NOT ATTEMPT TO MODIFY IT BY HAND.
 * Generated on 07-August-1997 at 02:38:16
 * Generated from file EVENTS.XLS by MIS
 */

#ifndef _SENDCHATMESSAGE_H
#define _SENDCHATMESSAGE_H

/*
 * Required Include Files
 */

#include "F4vu.h"
#include "falcmesg.h"
//#include "mission.h"
#pragma pack (1)

/*
 * Message Type Send Chat Message
 */
class UI_SendChatMessage : public FalconEvent
{
   public:
      UI_SendChatMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback=TRUE);
      UI_SendChatMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target);
      ~UI_SendChatMessage(void);
      virtual int Size() const { return sizeof(VU_ID) + sizeof(short) + dataBlock.size + FalconEvent::Size();};
		//sfr: changed to long *
	  int Decode (VU_BYTE **buf, long *rem);
      int Encode (VU_BYTE **buf);

      class DATA_BLOCK
      {
         public:

            VU_ID from;
            short size;
            void* message;
      } dataBlock;

   protected:
      int Process(uchar autodisp);
};
#pragma pack ()

#endif
