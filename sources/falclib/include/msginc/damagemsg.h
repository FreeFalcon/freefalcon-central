/*
 * Machine Generated include file for message "Damage Message".
 * NOTE: This file is read only. DO NOT ATTEMPT TO MODIFY IT BY HAND.
 * Generated on 24-July-1997 at 18:20:00
 * Generated from file EVENTS.XLS by Kevin Klemmick
 */

#ifndef _DAMAGEMSG_H
#define _DAMAGEMSG_H

/*
 * Required Include Files
 */
#include "F4vu.h"
#include "Falcmesg.h"
#include "FalcDmg.h"
#include "mission.h"
#include "falcsess.h"
#include "sim/include/simveh.h"
#include "InvalidBufferException.h"

#pragma pack (1)

/*
 * How close is close enough?
 */
#define POINT_BLANK		400				// 20 ft^2

/*
 * Message Type Damage Message
 */
class FalconDamageMessage : public FalconEvent
{
   public:
      FalconDamageMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback=TRUE);
      FalconDamageMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target);
      ~FalconDamageMessage(void);
      virtual int Size() const { return sizeof(dataBlock) + FalconEvent::Size();};
	  //sfr: changed to long *
	  int Decode (VU_BYTE **buf, long *rem) {
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
            float damageStrength;
			float damageRandomFact;
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

FalconDamageMessage *CreateGroundCollisionMessage(SimVehicleClass* vehicle, int damage, VuTargetEntity *target = FalconLocalGame);

#endif

