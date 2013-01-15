#include "stdhdr.h"
#include "bomb.h"
#include "bombdata.h"
#include "falcent.h"
#include "entity.h"
#include "hardpnt.h"

extern short NumRocketTypes;		// Added by M.N.
extern short gRocketId;



void BombClass::LauInit(void)
{
	Falcon4EntityClassType* classPtr;
	WeaponClassDataType* wc;
	SimWeaponDataType* wpnDefinition;
	int lauId;

	lauFireCount = 0;

	classPtr = (Falcon4EntityClassType*)EntityType();
	wc = (WeaponClassDataType*)classPtr->dataPtr;
	wpnDefinition = &SimWeaponDataTable[classPtr->vehicleDataIndex];
	lauId = (short)(((int)Falcon4ClassTable[wc->Index].dataPtr - (int)WeaponDataTable) / sizeof(WeaponClassDataType));

	if(wpnDefinition->weaponClass == wcRocketWpn)
	{

		if(auxData && auxData->lauWeaponId)
		{
			lauRounds    = lauMaxRounds = auxData->lauRounds;
			lauWeaponId  = auxData->lauWeaponId;
			lauSalvoSize = auxData->lauSalvoSize;
		}
		else
		{
			int j;
			int entryfound = 0;
			// read the rack dat file
			for (j=0; j<NumRocketTypes; j++)
			{
				if (lauId == RocketDataTable[j].weaponId)
				{
					if (RocketDataTable[j].nweaponId) // 0 = don't change weapon ID
						lauWeaponId  = RocketDataTable[j].nweaponId;
					lauMaxRounds = RocketDataTable[j].weaponCount; 
					lauRounds    = lauMaxRounds;
					lauSalvoSize = lauRounds;


					/*
					WeaponClass weapClass = (WeaponClass)SimWeaponDataTable[Falcon4ClassTable[WeaponDataTable[lauWeaponId].Index].vehicleDataIndex].weaponClass;  // MLR 1/28/2004 - hardPoint[i]->GetWeaponClass() is not correct

					if(weapClass==wcGunWpn)
					{
						hardPoint[i]->weaponPointer = InitAGun (newOwnship, hardPoint[i]->weaponId, hardPoint[i]->weaponCount);
					}
					else
					{
						hardPoint[i]->weaponPointer = InitWeaponList (newOwnship, hardPoint[i]->weaponId,
							hardPoint[i]->GetWeaponClass(), hardPoint[i]->weaponCount, InitAMissile);
					}
					*/

					entryfound = 1;
					break;
				}
			}
			/*
			if (!entryfound)	// use generic 2.75mm rocket
			{
				lauWeaponId  = gRocketId;
				lauMaxRounds = 19; 
				lauRounds    = lauMaxRounds;
				lauSalvoSize = lauRounds;
			}*/
		}
	}
	else
	{
		lauWeaponId=0;
	}
}

int BombClass::IsLauncher(void)
{
	if(lauWeaponId)
		return 1;
	return 0;
}

int BombClass::LauGetWeaponId(void)
{
	return(lauWeaponId);
}

void BombClass::LauSetRoundsRemaining(int r)
{
	if(r < lauMaxRounds)
		lauRounds = r;
	else
		lauRounds = lauMaxRounds;
}

void BombClass::LauAddRounds(int count)
{
	lauRounds += count;
	if(lauRounds > lauMaxRounds)
		lauRounds = lauMaxRounds;
	else
		if(lauRounds < 0)
			lauRounds = 0;
}

void BombClass::LauRemFiredRound( void )
{
	lauRounds --;
	lauFireCount --;

	if(lauRounds < 0)
		lauRounds = 0;
	if(lauFireCount < 0)
		lauFireCount = 0;
}



int BombClass::LauGetRoundsRemaining(void)
{
	return lauRounds - lauFireCount;
}

int BombClass::LauGetMaxRounds(void)
{
	return lauMaxRounds;
}

int BombClass::LauGetSalvoSize(void)
{
	if(auxData)
	{
		return auxData->lauSalvoSize;
	}
	return -1;
}

void BombClass::LauGetAttitude(float &elevation, float &azimuth)
{
	if(auxData)
	{
		elevation = auxData->lauElevation;
		azimuth   = auxData->lauAzimuth;
	}
	else
	{
		elevation = 0;
		azimuth   = 0;
	}
}

int BombClass::LauCheckTimer(void)
{
	if(lauTimer <= SimLibElapsedTime)
	{
		lauTimer=SimLibElapsedTime + auxData->lauRippleTime; 
		return 1;
	}
	return 0;
}

void BombClass::LauFireSalvo(void)
{
	if(auxData->lauSalvoSize>0)
	{
		lauFireCount += auxData->lauSalvoSize;
		if(lauFireCount > lauRounds)
			lauFireCount = lauRounds;
	}
	else
		lauFireCount = lauRounds;
}

int BombClass::LauIsFiring(void)
{
	return( lauRounds>0 && lauFireCount>0 );
}

