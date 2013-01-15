/*
 * Machine Generated include file for message "Send Persistant List".
 * NOTE: This file is read only. DO NOT ATTEMPT TO MODIFY IT BY HAND.
 * Generated on 29-July-1997 at 17:49:00
 * Generated from file EVENTS.XLS by Kevin Klemmick
 */

#ifndef _SENDPERSISTANTLIST_H
#define _SENDPERSISTANTLIST_H

/*
 * Required Include Files
 */
#include "F4vu.h"
#include "mission.h"
#include "FalcMesg.h"

#pragma pack (1)

/*
 * Message Type Send Persistant List
 */
class FalconSendPersistantList : public FalconEvent
{
   public:
      FalconSendPersistantList(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback=TRUE);
      FalconSendPersistantList(VU_MSG_TYPE type, VU_ID senderid, VU_ID target);
      ~FalconSendPersistantList(void);
      virtual int Size() const;
		//sfr: changed to long *
	  virtual int Decode (VU_BYTE **buf, long *rem);
      virtual int Encode (VU_BYTE **buf);

      class DATA_BLOCK
      {
         public:
            short size;
            void* data;
      } dataBlock;

   protected:
      int Process(uchar autodisp);
};
#pragma pack ()

#endif
