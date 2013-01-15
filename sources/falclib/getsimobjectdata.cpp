// Weapon Stuff
#include "GetSimObjectData.h"

SimWeaponDataType *GetSWD(int WeaponId)
{
	if(WeaponId)
	{
		WeaponClassDataType* wc;
		Falcon4EntityClassType* classPtr;

		wc            = &WeaponDataTable[WeaponId];
		classPtr      = &(Falcon4ClassTable[wc->Index]);
		return &SimWeaponDataTable[classPtr->vehicleDataIndex];
	}
	return NULL;
}

Falcon4EntityClassType *GetWeaponF4CT(int WeaponId)
{
	if(WeaponId)
	{
		WeaponClassDataType* wc;

		wc            = &WeaponDataTable[WeaponId];
		return &(Falcon4ClassTable[wc->Index]);
	}
	return NULL;
}

WeaponClassDataType *GetWCD(int WeaponId)
{
	if(WeaponId)
	{
		return &WeaponDataTable[WeaponId];
	}
	return NULL;
}




