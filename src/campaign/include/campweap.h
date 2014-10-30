//
// Campweap.h
//
// Deals with weapon information
//
// Kevin Klemmick, 1996

#ifndef CAMPWEAP_H
#define CAMPWEAP_H

// ===================================
// Guidance Types
// ===================================

#define WEAP_VISUALONLY 0x0 // Default state unless one of these flags are set
#define WEAP_ANTIRADATION 0x1
#define WEAP_HEATSEEKER 0x2
#define WEAP_RADAR 0x4
#define WEAP_LASER 0x8
#define WEAP_TV 0x10
#define WEAP_REAR_ASPECT 0x20 // Really only applies to heatseakers
#define WEAP_FRONT_ASPECT 0x40 // Really only applies to SAMs

#define WEAP_DUMB_ONLY 0x1000 // ONLY load non-guided or GPS weapons

#define WEAP_GUIDED_MASK 0x1F

// ===================================
// Flags
// ===================================

#define WEAP_RECON 0x01 // Used for recon
#define WEAP_FUEL 0x02 // Fuel tank
#define WEAP_ECM 0x04 // Used for ECM
#define WEAP_AREA 0x08 // Can damage multiple vehicles
#define WEAP_CLUSTER 0x10 // CBU -- cluster bomb
#define WEAP_TRACER 0x20 // Use tracers when drawing weapon fire
#define WEAP_ALWAYSRACK 0x40 // this weapon has no rack
#define WEAP_LGB_3RD_GEN 0x80 // 3rd generation LGB's
#define WEAP_BOMBWARHEAD 0x100 // this is for example a missile with a bomb war head. MissileEnd when ground impact, not when lethalradius reached
#define WEAP_NO_TRAIL 0x200 // do not display any weapon trails or engine glow
#define WEAP_BOMBDROPSOUND 0x400 // play the bomb drop sound instead of missile launch
#define WEAP_BOMBGPS 0x800 // we use this for JDAM "missile" bombs to have them always CanDetectObject and CanSeeObject true

// RV - Biker - Make the rocket marker flag usable
#define WEAP_ROCKET_MARKER 0x1000

#define WEAP_FORCE_ON_ONE 0x2000 // Put all requested weapons on one/two hardpoints
#define WEAP_GUN 0x4000 // Used by LoadWeapons only - to specify guns only
#define WEAP_ONETENTH 0x8000 // # listed is actually 1/10th the # of shots

#define WEAP_BAI_LOADOUT 0x10000 // special loadout for BAI type missions:
// only Mavericks and dumb bombs, GBU only
// GBU-12 (wid 68) and GBU-22 (wid 310)
// this flag is NOT used in the WeaponClassDataType,
// only for loading specific weapons only

// RV - Biker - Some more loadout flags
#define WEAP_DEAD_LOADOUT 0x20000 // Give long range weapons higher prio
#define WEAP_LASER_POD 0x40000 // Used to load laser pods on HPs
#define WEAP_FAC_LOADOUT 0x80000 // Load Willy Pete rockets only
#define WEAP_CHAFF_POD 0x100000 // Load Chaff Flare dispenser pods

#define WEAP_INFINITE_MASK 0x07 // Things which meet this mask never run out.

#define HARDPOINT_MAX 16 // Number of hardpoints in the weapon table

// ===================================
// Damage Types
// ===================================

typedef enum { NoDamage,
                PenetrationDam, // Hardened structures, tanks, ships
                HighExplosiveDam, // Soft targets, area targets
                HeaveDam, // Runways
                IncendairyDam, // Burn baby, burn
                ProximityDam, // AA missiles, etc.
                KineticDam, // Guns, small arms fire
                HydrostaticDam, // Submarines
                ChemicalDam,
                NuclearDam,
                OtherDam
             } DamageDataType;

typedef DamageDataType DamType;

// ===================================
// Functions
// ===================================

extern int LoadWeaponTable(char *filename);

extern int GetWeaponStrength(int w);

extern int GetWeaponRange(int w, int mt);

extern int GetWeaponFireRate(int w);

extern int GetWeaponHitChance(int w, int mt);

extern int GetWeaponHitChance(int w, int mt, int range);

extern int GetWeaponHitChance(int w, int mt, int range, int wrange);

extern int GetWeaponScore(int w, int mt, int range);

extern int GetWeaponScore(int w, int mt, int range, int wrange);

extern int GetWeaponScore(int w, uchar* dam, int m, int range);

extern int GetWeaponScore(int w, uchar* dam, int mt, int range, int wrange);

extern int GetWeaponDamageType(int w);

extern int GetWeaponDescriptionIndex(int w);

extern int GetWeaponIdFromDescriptionIndex(int index);

extern int GetWeaponFlags(int w);

#endif
