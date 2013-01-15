// ======================================
// Vehicle related stuff
// ======================================

#ifndef VEHICLE_H
#define VEHICLE_H

#include "Entity.h"

// ===================
// Flags
// ===================

// Aircraft special capibilities and service types
#define VEH_AIRFORCE				0x01
#define VEH_NAVY					0x02
#define VEH_MARINES					0x04
#define VEH_ARMY					0x08
#define VEH_ALLWEATHER				0x10
#define VEH_STEALTH					0x20
#define VEH_NIGHT					0x40
#define VEH_VTOL					0x80

// General flags
#define VEH_FLAT_CONTAINER			0x100			// Vehicles typically sit on this
#define VEH_ELEV_CONTAINER			0x200

#define VEH_CAN_EXPLODE				0x400
#define VEH_CAN_BURN				0x800
#define VEH_CAN_SMOKE				0x1000
#define VEH_CAN_COLAPSE				0x2000

#define VEH_HAS_CREW				0x4000
#define VEH_IS_TOWED				0x8000
#define VEH_HAS_JAMMER				0x10000			// Has built in self protection jamming
#define VEH_IS_AIR_DEFENSE			0x20000			// This ground thing prefers to shoot air things

// 2002-02-13 ADDED BY S.G. Used to flag the unit/vehicle has having NCTR or EXACT RWR capabilities
#define VEH_HAS_NCTR				0x40000
#define VEH_HAS_EXACT_RWR			0x80000

// RV - Biker - The two above only are used for AC so why not use for ground vehicle too
#define VEH_IS_ARTILLERY			0x40000
#define VEH_IS_ENGINEER				0x80000

#define VEH_USES_UNIT_RADAR			0x100000 // 2002-02-28 ADDED BY S.G.

// Service/capibility masks
#define VEH_SERVICE_MASK			0x0F
#define VEH_CAPIBILITY_MASK			0xF0

// ===================
// Other stuff
// ===================

typedef  uchar VehicleSP;
typedef  short VehicleID;

class UnitClass;
typedef UnitClass* Unit;

// ===================
// Global Functions
// ===================

extern char* GetVehicleName (VehicleID vid);

extern VehicleClassDataType* GetVehicleClassData (int index);

// extern int GetVehicleHitChance (Unit u, int id, int mt, int range, int hitflags);

extern int GetAproxVehicleHitChance (int id, int mt, int range);

extern int CalculateVehicleHitChance (int id, int mt);

// extern int GetVehicleCombatStrength (Unit u, int id, int mt, int range);

extern int GetAproxVehicleCombatStrength (int id, int mt, int range);

extern int CalculateVehicleCombatStrength (int id, int mt);

extern int GetAproxVehicleRange (int id, int mt);

// extern int GetVehicleRange (Unit u, int id, int mt);

extern int CalculateVehicleRange (int id, int mt);

extern int GetVehicleDetectionRange (int id, int mt);

extern int GetBestVehicleWeapon (int id, uchar* dam, MoveType m, int range, int *hard_point);

extern void CalculateVehicleStatistics (int id);

extern int GetVehicleWeapon (int vid, int hp);

extern int GetVehicleWeapons (int vid, int hp);

#endif