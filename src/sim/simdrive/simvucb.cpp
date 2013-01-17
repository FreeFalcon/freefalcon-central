#include "stdhdr.h"
#include "simvucb.h"
#include "simvudrv.h"
#include "aircrft.h"
#include "missile.h"
#include "ground.h"
#include "simfeat.h"
#include "bomb.h"
#include "chaff.h" // JB 010220
#include "flare.h" // JB 010220
#include "otwdrive.h"
#include "classtbl.h"
#include "simdrive.h"
#include "helo.h"
#include "CampBase.h"
#include "CampList.h"
#include "simeject.h"

//VuEntity* SimVUCreateVehicle (ushort type, ushort size, VU_BYTE* data)
VuEntity* SimVUCreateVehicle (ushort type, ushort sizeShort, VU_BYTE* data)
{
	long size = sizeShort;
	Falcon4EntityClassType* classPtr = (Falcon4EntityClassType*)VuxType(type);
	SimBaseClass* theVehicle = NULL;

	if (classPtr->vuClassData.classInfo_[VU_DOMAIN] == DOMAIN_AIR)
	{
		if (classPtr->vuClassData.classInfo_[VU_TYPE] == TYPE_AIRPLANE)
		{
			ShiWarning ("Should never make an A/C this way");
			//			theVehicle = new AircraftClass(TRUE, &data);
		}
		else if (classPtr->vuClassData.classInfo_[VU_TYPE] == TYPE_MISSILE)
		{
			theVehicle = new MissileClass(&data, &size);
			if (((SimWeaponClass*)theVehicle)->parent == NULL)
			{
				delete theVehicle;
				theVehicle = NULL;
				return NULL;
			}
		}
		else if (classPtr->vuClassData.classInfo_[VU_TYPE] == TYPE_ROCKET)
		{
			theVehicle = new MissileClass(&data, &size);
			if (((SimWeaponClass*)theVehicle)->parent == NULL)
			{
				delete theVehicle;
				theVehicle = NULL;
				return NULL;
			}
		}
		else if (classPtr->vuClassData.classInfo_[VU_TYPE] == TYPE_BOMB)
		{
			// JB 010220
			if (classPtr->vuClassData.classInfo_[VU_STYPE] == STYPE_CHAFF &&
							classPtr->vuClassData.classInfo_[VU_SPTYPE] == SPTYPE_CHAFF1)
			{
				theVehicle = new ChaffClass(&data, &size);
				if (((SimWeaponClass*)theVehicle)->parent == NULL)
				{
					delete theVehicle;
					theVehicle = NULL;
					return NULL;
				}
			}
			else if (classPtr->vuClassData.classInfo_[VU_STYPE] == STYPE_FLARE1 &&
							classPtr->vuClassData.classInfo_[VU_SPTYPE] == SPTYPE_CHAFF1 + 1)
			{
				theVehicle = new FlareClass(&data, &size);
				if (((SimWeaponClass*)theVehicle)->parent == NULL)
				{
					delete theVehicle;
					theVehicle = NULL;
					return NULL;
				}
			}
			else
			{
				// JB 010220
				theVehicle = new BombClass(&data, (long*)&size);
				if (((SimWeaponClass*)theVehicle)->parent == NULL)
				{
					delete theVehicle;
					theVehicle = NULL;
					return NULL;
				}
			}
		}
		else if (classPtr->vuClassData.classInfo_[VU_TYPE] == TYPE_HELICOPTER)
		{
			ShiWarning ("Should never make an Helo this way");
			//			theVehicle = new HelicopterClass(&data);
		}
		else if (classPtr->vuClassData.classInfo_[VU_TYPE] == TYPE_EJECT)
		{
			// TODO: remote creation for ejected pilots!!!
			theVehicle = new EjectedPilotClass(&data, &size);
		}
	}
	else if (classPtr->vuClassData.classInfo_[VU_DOMAIN] == DOMAIN_LAND)
	{
		ShiWarning ("Should never make an Truck this way");
		//		theVehicle = new GroundClass (&data);
	}
	else if (classPtr->vuClassData.classInfo_[VU_DOMAIN] == DOMAIN_SEA)
	{
		ShiWarning ("Should never make an Ship this way");
		//		theVehicle = new GroundClass (&data);
	}

	F4Assert (theVehicle);
	if (theVehicle)
	{
		// Zero out velocities until ready to run
		//theVehicle->SetDelta(0.0F, 0.0F, 0.0F);
		//theVehicle->SetYPRDelta(0.0F, 0.0F, 0.0F);
		theVehicle->SetDriver(NULL);
		//		MonoPrint("Got remote creation event! Inserting object %08x\n", theVehicle);

		theVehicle->Init(NULL);
	}

	return theVehicle;
}

//VuEntity* SimVUCreateFeature (ushort type, ushort size, VU_BYTE* data)
VuEntity* SimVUCreateFeature (ushort, ushort sizeShort, VU_BYTE* data, long rem)
{
	long size = rem;
	SimFeatureClass* theFeature;

	theFeature = new SimFeatureClass (&data, &size);

	((SimBaseClass*)theFeature)->Init(NULL);

	return theFeature;
}
