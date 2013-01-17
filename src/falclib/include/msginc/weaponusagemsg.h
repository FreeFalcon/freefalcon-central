/*
 * Machine Generated include file for message "Weapon Use Message".
 * NOTE: This file is read only. DO NOT ATTEMPT TO MODIFY IT BY HAND.
 * Generated on 28-August-1997 at 17:24:22
 * Generated from file EVENTS.XLS by MicroProse
 */

#ifndef _WEAPONUSAGEMSG_H
#define _WEAPONUSAGEMSG_H

/*
 * Required Include Files
 */
#include "F4vu.h"
#include "mission.h"
#include "FalcMesg.h"

#pragma pack (1)


/*
 * Message Type Weapon Use Message
 */

/* Unused.. Remove
class FalconWeaponUsageMessage : public FalconEvent
{
   public:
      FalconWeaponUsageMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback=TRUE);
      FalconWeaponUsageMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target);
      ~FalconWeaponUsageMessage(void);
      int Size (void) { return sizeof(dataBlock) + FalconEvent::Size();};
      int Decode (VU_BYTE **buf, int length)
         {
         int size;

            size = FalconEvent::Decode (buf, length);
            memcpy (&dataBlock, *buf, sizeof (dataBlock));
            *buf += sizeof (dataBlock);
            size += sizeof (dataBlock);
            return size;
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
			short fuel;
            uchar ids[HARDPOINT_MAX];
            uchar num[HARDPOINT_MAX];
			uchar vehicles;
      } dataBlock;

   protected:
      int Process(uchar autodisp);
};
*/

#pragma pack ()

#endif
