/*
 * Machine Generated include file for message "Campaign Weap Fire".
 * NOTE: This file is read only. DO NOT ATTEMPT TO MODIFY IT BY HAND.
 * Generated on 18-March-1998 at 09:20:55
 * Generated from file EVENTS.XLS by MicroProse
 */

#ifndef _CAMPWEAPONFIREMSG_H
#define _CAMPWEAPONFIREMSG_H

/*
 * Required Include Files
 */
#include "F4vu.h"
#include "FalcMesg.h"
#include "mission.h"

// =============================================================================
// KCK: This message is used to broadcast damage data to the entire game for
// Campaign Entities. This is called only from a Campaign Entity's ApplyDamage()
// function, but is called regardless of wether damage was applied in order to
// do the distant visual effects.
//
// It contains a list of all weapons being fired and the quantity, as well as
// decodable damage data. The weapon list is used for visual effects on anymore.
// =============================================================================

#pragma pack (1)

#define MAX_TYPES_PER_CAMP_FIRE_MESSAGE		8

class SimBaseClass;

/*
 * Message Type Campaign Weap Fire
 */
class FalconCampWeaponsFire : public FalconEvent
{
   public:
      FalconCampWeaponsFire(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback=TRUE);
      FalconCampWeaponsFire(VU_MSG_TYPE type, VU_ID senderid, VU_ID target);
      ~FalconCampWeaponsFire(void);
      virtual int Size (void) const;
	  //sfr: changed to long *
      virtual int Decode (VU_BYTE **buf, long *rem);
      virtual int Encode (VU_BYTE **buf);
      class DATA_BLOCK
      {
         public:

            VU_ID shooterID;
            VU_ID fWeaponUID;
            short weapon[MAX_TYPES_PER_CAMP_FIRE_MESSAGE];
            uchar shots[MAX_TYPES_PER_CAMP_FIRE_MESSAGE];
			uchar fPilotId;
			uchar dPilotId;
			ushort size;
			uchar *data;
      } dataBlock;

   protected:
      int Process(uchar autodisp);
};

extern uchar gDamageStatusBuffer[256];
extern uchar *gDamageStatusPtr;

// Functions to fire on sim entities from campaign units
extern SimBaseClass* GetSimTarget(CampEntity target, uchar targetId);
extern void FireOnSimEntity(CampEntity shooter, CampEntity campTarg, short weapon[], uchar shots[], uchar targetId=255);
extern void FireOnSimEntity(CampEntity shooter, SimBaseClass *simTarg, short weaponId);
extern void SendSimDamageMessage( CampEntity shooter, SimBaseClass *target, float rangeSq, int damageType,	int weapId );

#pragma pack ()

#endif
