#include "stdhdr.h"
#include "guns.h"
#include "gunsfunc.h"
#include "camp2sim.h"
#include "initdata.h"
#include "Entity.h"

SimWeaponClass** InitGun (SimBaseClass* parent, ushort id, int num, int side)
{
SimWeaponClass** gunPtr;

   gunPtr = new SimWeaponClass*[1];
   gunPtr[0] = new GunClass (WeaponDataTable[id].Index + VU_LAST_ENTITY_TYPE);
   gunPtr[0]->SetParent(parent);
	gunPtr[0]->SetCountry(side);
   gunPtr[0]->SetFlag(MOTION_BMB_AI);
	((GunClass*)(gunPtr[0]))->Init(3000.0F, num);

	return (gunPtr);
}

SimWeaponClass* InitAGun (SimBaseClass* parent, ushort id, int rounds)
{
GunClass* gunPtr;

	gunPtr = new GunClass (WeaponDataTable[id].Index + VU_LAST_ENTITY_TYPE);
	gunPtr->SetParent(parent);
	gunPtr->SetCountry(parent->GetCountry());
	gunPtr->SetFlag(MOTION_BMB_AI);
	gunPtr->Init(3000.0F, rounds);
	return gunPtr;
}
