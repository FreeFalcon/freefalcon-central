/*
 * Machine Generated include file for message "Campaign Event Message".
 * NOTE: This file is read only. DO NOT ATTEMPT TO MODIFY IT BY HAND.
 * Generated on 28-August-1997 at 17:24:22
 * Generated from file EVENTS.XLS by MicroProse
 */

#ifndef _CAMPEVENTMSG_H
#define _CAMPEVENTMSG_H

/*
 * Required Include Files
 */
#include "F4vu.h"
#include "mission.h"
#include "FalcMesg.h"

#include "InvalidBufferException.h"

class EventDataClass
	{
	public:
		EventDataClass(void);

	public:
		short formatId;
		short xLoc;
		short yLoc;
		VU_ID vuIds[CUI_ME];
		short owners[CUI_MD];
		short textIds[CUI_MS];
	};

#pragma pack (1)

/*
 * Message Type Campaign Event Message
 */
class FalconCampEventMessage : public FalconEvent
{
   public:
      enum CampEventType {
         campGroundAttack,
         campStrike,
         campAirCombat,
         campCombat,
         campLosses,
         unitWithdrawing,
         unitDestroyed,
         unitReinforcement,
         objectiveCaptured,
         relationsChange,
         triggeredEvent };

      FalconCampEventMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback=TRUE);
      FalconCampEventMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target);
      ~FalconCampEventMessage(void);
      virtual int Size() const { return sizeof(dataBlock) + FalconEvent::Size();};
		//sfr: changed to long *
		int Decode (VU_BYTE **buf,  long *rem)
		{
			long init = *rem;
			FalconEvent::Decode (buf, rem);
			memcpychk(&dataBlock, buf, sizeof (dataBlock), rem);
			return init  - *rem;
		};
      int Encode (VU_BYTE **buf)
         {
         int size;

            size = FalconEvent::Encode (buf);
            memcpy (*buf, &dataBlock, sizeof (dataBlock));
            *buf += sizeof (dataBlock);
            size += sizeof (dataBlock);
            return size;
         };
      class DATA_BLOCK
      {
         public:

            unsigned int eventType;
            uchar flags;
            uchar team;
            short size;
			EventDataClass data;
      } dataBlock;

   protected:
      int Process(uchar autodisp);
};

#pragma pack ()

#endif
