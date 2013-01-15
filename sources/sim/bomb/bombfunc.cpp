#include "stdhdr.h"
#include "bomb.h"
#include "bombfunc.h"
#include "camp2sim.h"
#include "initdata.h"
#include "Entity.h"

SimWeaponClass** InitBomb (FalconEntity* parent, ushort type, int num, int side)
{
int i;
SimWeaponClass** bombPtr = NULL;
int numSlots = 1;
	ShiAssert(num > 0);
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

      bombPtr = new SimWeaponClass*[numSlots];

	   for (i=0; i<num; i++)
	   {
		   /*------------------------*/
		   /* Add it to the database */
		   /*------------------------*/
         bombPtr[i] = new BombClass (WeaponDataTable[type].Index + VU_LAST_ENTITY_TYPE);
         bombPtr[i]->SetParent(parent);
         bombPtr[i]->SetCountry(side);
         bombPtr[i]->SetFlag (MOTION_BMB_AI);
         bombPtr[i]->Init();
	   }

      for (; i<numSlots; i++)
      {
         bombPtr[i] = NULL;
      }
   }

	return (bombPtr);
}

SimWeaponClass* InitABomb (FalconEntity* parent, ushort type, int slot)
{
	BombClass* bombPtr;

	bombPtr = new BombClass (WeaponDataTable[type].Index + VU_LAST_ENTITY_TYPE);
	bombPtr->SetCountry(parent->GetCountry());
	bombPtr->SetFlag(MOTION_BMB_AI);
	bombPtr->SetParent(parent);
	bombPtr->SetRackSlot(slot);
	bombPtr->Init();
	return (bombPtr);
}

void FreeRackBomb (SimBaseClass *railer)
{
   vuDatabase->Remove (railer);
}
