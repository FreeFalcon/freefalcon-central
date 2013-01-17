#include "Graphics\Include\drawbsp.h"
#include "stdhdr.h"
#include "missile.h"
#include "misslist.h"
#include "camp2sim.h"
#include "initdata.h"
#include "Entity.h"

SimWeaponClass** InitMissile (FalconEntity* parent, ushort id, int num, int side)
{
int i;
SimWeaponClass** missilePtr = NULL;
int numSlots=1;

   if (num)
   {
      if (num > 6)
         numSlots = num;
      else if (num > 3)
         numSlots = 6;
      else if (num > 1)
         numSlots = 3;
      else  if (num > 0)
         numSlots = 1;
      
      missilePtr = new SimWeaponClass*[numSlots];

	   for (i=0; i<num; i++)
	   {
		   /*------------------------*/
		   /* Add it to the database */
		   /*------------------------*/
         missilePtr[i] = new MissileClass (WeaponDataTable[id].Index + VU_LAST_ENTITY_TYPE);
		   missilePtr[i]->SetCountry(side);
         missilePtr[i]->SetFlag (MOTION_MSL_AI);
         missilePtr[i]->SetParent(parent);
		 missilePtr[i]->Init();
	   }

      for (; i<numSlots; i++)
      {
         missilePtr[i] = NULL;
      }
   }

	return (missilePtr);
}

SimWeaponClass* InitAMissile (FalconEntity* parent, ushort type, int slot)
{
MissileClass* missilePtr;

	missilePtr = new MissileClass (WeaponDataTable[type].Index + VU_LAST_ENTITY_TYPE);
	missilePtr->SetCountry(parent->GetCountry());
	missilePtr->SetFlag (MOTION_MSL_AI);
	missilePtr->SetParent(parent);
	missilePtr->SetRackSlot(slot);
	missilePtr->Init();
	return (missilePtr);
}

void FreeFlyingMissile (SimBaseClass* flier)
{
   vuDatabase->Remove (flier);
}

void FreeRailMissile (SimBaseClass* railer)
{
   railer->SetDead(TRUE);
   railer->SetExploding(TRUE);
   vuDatabase->Remove (railer);
}
