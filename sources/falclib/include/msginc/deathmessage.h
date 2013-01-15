/*
 * Machine Generated include file for message "Death Message".
 * NOTE: This file is read only. DO NOT ATTEMPT TO MODIFY IT BY HAND.
 * Generated on 24-July-1997 at 18:20:03
 * Generated from file EVENTS.XLS by Kevin Klemmick
 */

#ifndef _DEATHMESSAGE_H
#define _DEATHMESSAGE_H

/*
 * Required Include Files
 */
#include "F4vu.h"
#include "mission.h"
#include "FalcMesg.h"

//sfr: for chks
#include "InvalidBufferException.h"

#pragma pack (1)

/*
 * Message Type Death Message
 */
class FalconDeathMessage : public FalconEvent
{
   public:
      FalconDeathMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback=TRUE);
      FalconDeathMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target);
      ~FalconDeathMessage(void);
      virtual int Size() const { return sizeof(dataBlock) + FalconEvent::Size();};
	  //sfr: changed to ong *
	  int Decode (VU_BYTE **buf, long *rem)
	  {
		  long init = *rem;

		  FalconEvent::Decode (buf, rem);
		  memcpychk(&dataBlock, buf, sizeof (dataBlock), rem);
		  return init - *rem;
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
            unsigned int damageType;
			float deathPctStrength;
            VU_ID dEntityID;
            ushort dCampID;
            uchar dPilotID;
            ushort dIndex;
            uchar dSide;
            VU_ID fEntityID;
            ushort fCampID;
            uchar fPilotID;
            ushort fIndex;
            uchar fSide;
            ushort fWeaponID;
            VU_ID fWeaponUID;
      } dataBlock;

   protected:
      int Process(uchar autodisp);
};

#pragma pack ()

#endif
