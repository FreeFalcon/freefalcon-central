#ifndef ENTITY_H
#define ENTITY_H

#include <tchar.h>
#include "camplib.h"
#include "campweap.h"
#include "dirtybits.h"

// ==================================
// Class table entry structure
// ==================================

#pragma pack (1)

typedef struct
{
    VuEntityType vuClassData;
    short          visType[7];
    short          vehicleDataIndex;
    uchar          dataType;
    void*          dataPtr;
} Falcon4EntityClassType;

#pragma pack ()

// ==================================
// Private data data structures
// ==================================

struct UnitClassDataType
{
    short Index; // descriptionIndex pointing here
    int NumElements[VEHICLE_GROUPS_PER_UNIT];
    short VehicleType[VEHICLE_GROUPS_PER_UNIT]; // Class table description index
    uchar VehicleClass[VEHICLE_GROUPS_PER_UNIT][8]; // 9 byte class description array
    ushort Flags; // Unit capibility flags (see VEH_ flags in vehicle.h)
    _TCHAR Name[20]; // Unit name 'Infantry', 'Armor'
    MoveType MovementType;
    short MovementSpeed;
    short MaxRange; // Movement/flight range with full supply
    long Fuel; // Fuel (internal)
    short Rate; // Fuel usage- in lbs per minute (cruise speed)
    short PtDataIndex; // Index into pt header data table
    uchar Scores[MAXIMUM_ROLES]; // Score for each type of mission or role
    uchar Role; // What sort of mission role is standard
    uchar HitChance[MOVEMENT_TYPES]; // Unit hit chances (best hitchance)
    uchar Strength[MOVEMENT_TYPES]; // Unit strengths (full strength only)
    uchar Range[MOVEMENT_TYPES]; // Firing ranges (maximum range)
    uchar Detection[MOVEMENT_TYPES]; // Electronic detection ranges at full strength
    uchar DamageMod[OtherDam + 1]; // How much each type will hurt me (% of strength applied)
    uchar RadarVehicle; // ID of the radar vehicle for this unit
    short SpecialIndex; // Index into yet another table (max stores for squadrons)
    short IconIndex; // Index to this unit's icon type
};

struct FeatureEntry
{
    short Index; // Entity class index of feature
    ushort Flags;
    uchar eClass[8]; // Entity class array of feature
    uchar Value; // % loss in operational status for destruction
    vector Offset;
    Int16 Facing;
};

struct ObjClassDataType
{
    short Index; // descriptionIndex pointing here
    _TCHAR Name[20];
    short DataRate; // Sorte Rate and other cool data
    short DeagDistance; // Distance to deaggregate at.
    short PtDataIndex; // Index into pt header data table
    uchar Detection[MOVEMENT_TYPES]; // Detection ranges
    uchar DamageMod[OtherDam + 1]; // How much each type will hurt me (% of strength applied)
    short IconIndex; // Index to this objective's icon type
    uchar Features; // Number of features in this objective
    uchar RadarFeature; // ID of the radar feature for this objective
    short FirstFeature; // Index of first feature entry
};

struct WeaponClassDataType
{
    short Index; // descriptionIndex pointing here
    // 2002-02-08 MN changed from short to ushort
    ushort Strength; // How much damage it'll do.
    DamType DamageType; // What type of damage it does.
    short Range; // Range, in km.
    ushort Flags;
    _TCHAR Name[20];
    uchar HitChance[MOVEMENT_TYPES];
    uchar FireRate; // # of shots fired per barrage
    uchar Rariety; // % of full supply which is actually provided
    ushort GuidanceFlags;
    uchar Collective;
    short SimweapIndex; // Index into sim's weapon data tables
    ushort Weight; // Weight in lbs.
    short DragIndex;
    ushort BlastRadius; // radius in ft.
    short RadarType; // Index into RadarDataTable
    short SimDataIdx; // Index int SimWeaponDataTable
    char MaxAlt; // Maximum altitude it can hit in thousands of feet
};

// 2001-11-05 Added by M.N.

struct RocketClassDataType
{
    short weaponId; // Weapon ID
    short nweaponId; // new Weapon ID (if type of munition changes)
    short weaponCount; // number of rockets to fire
};

// 2002-04-20 Added by M.N. externalised DD priorities

struct DirtyDataClassType
{
    Dirtyness priority; // Priorities of DirtyData messages
};

// END of added section

struct FeatureClassDataType
{
    short Index; // descriptionIndex pointing here
    short RepairTime; // How long it takes to repair
    uchar Priority; // Display priority
    ushort Flags; // See FEAT_ flags in feature.h
    _TCHAR Name[20]; // 'Control Tower'
    short HitPoints; // Damage this thing can take
    short Height; // Height of vehicle ramp, if any
    float Angle; // Angle of vehicle ramp, if any
    short RadarType; // Index into RadarDataTable
    uchar Detection[MOVEMENT_TYPES]; // Electronic detection ranges
    uchar DamageMod[OtherDam + 1]; // How much each type will hurt me (% of strength applied)
};

struct VehicleClassDataType
{
    short Index; // descriptionIndex pointing here
    short HitPoints; // Damage this thing can take
    unsigned Flags; // see VEH_ flags in vehicle.h
    _TCHAR Name[15];
    _TCHAR NCTR[5];
    float RCSfactor; // log2( 1 + RCS relative to an F16 )
    long MaxWt; // Max loaded weight in lbs.
    long EmptyWt; // Empty weight in lbs.
    long FuelWt; // Weight of max fuel in lbs.
    short FuelEcon; // Fuel usage in lbs./min.
    short EngineSound; // SoundFX sample index of corresponding engine sound
    short HighAlt; // in hundreds of feet
    short LowAlt; // in hundreds of feet
    short CruiseAlt; // in hundreds of feet
    short MaxSpeed; // Maximum vehicle speed, in kph
    short RadarType; // Index into RadarDataTable
    short NumberOfPilots; // # of pilots (for eject)
    ushort RackFlags; //0x01 means hardpoint 0 needs a rack, 0x02 -> hdpt 1, etc
    ushort VisibleFlags; //0x01 means hardpoint 0 is visible, 0x02 -> hdpt 1, etc
    uchar CallsignIndex;
    uchar CallsignSlots;
    uchar HitChance[MOVEMENT_TYPES]; // Vehicle hit chances (best hitchance bitand bonus)
    uchar Strength[MOVEMENT_TYPES]; // Combat strengths (full strength only) (calculated)
    uchar Range[MOVEMENT_TYPES]; // Firing ranges (full strength only) (calculated)
    uchar Detection[MOVEMENT_TYPES]; // Electronic detection ranges
    short Weapon[HARDPOINT_MAX]; // Weapon id of weapons (or weapon list)
    uchar Weapons[HARDPOINT_MAX]; // Number of shots each (fully supplied)
    uchar DamageMod[OtherDam + 1]; // How much each type will hurt me (% of strength applied)
};

struct SquadronStoresDataType
{
    uchar Stores[MAXIMUM_WEAPTYPES]; // Weapon stores (only has meaning for squadrons)
    uchar infiniteAG; // One AG weapon we've chosen to always have available
    uchar infiniteAA; // One AA weapon we've chosen to always have available
    uchar infiniteGun; // Our main gun weapon, which we will always have available
};

struct PtHeaderDataType
{
    short objID; // ID of the objective this belongs to
    uchar type; // The type of pt data this contains
    uchar count; // Number of points
    uchar features[MAX_FEAT_DEPEND]; // Features this list depends on (# in objective's feature list)
    short data; // Other data (runway heading, for example)
    float sinHeading;
    float cosHeading;
    short first; // Index of first point
    short texIdx; // texture to apply to this runway
    char runwayNum; // -1 if not a runway, indicates which runway this list applies to
    char ltrt; // put base pt to rt or left
    short nextHeader; // Index of next header, if any
};

struct PtDataType
{
    float xOffset, yOffset; // X and Y offsets of this point (from center of objective tile)
    uchar type; // The type of point this is
    uchar flags;
};

typedef struct SimWeaponDataType
{
    int  flags;                            // Flags for the SMS
    float cd;                              // Drag coefficient
    float weight;                          // Weight
    float area;                            // sirface area for drag calc
    float xEjection;                       // Body X axis ejection velocity
    float yEjection;                       // Body Y axis ejection velocity
    float zEjection;                       // Body Z axis ejection velocity
    char  mnemonic[8];                     // SMS Mnemonic
    int   weaponClass;                     // SMS Weapon Class
    int   domain;                          // SMS Weapon Domain
    int   weaponType;                      // SMS Weapon Type
    int   dataIdx;                         // Aditional characteristics data file
} SimWEaponDataType;

typedef struct SimACDefType
{
    int  combatClass;                      // What type of combat does it do?
    int  airframeIdx;                      // Index into airframe tables
    int  signatureIdx;                     // Index into signature tables (IR only for now)
    int  sensorType[5];                    // Sensor Types
    int  sensorIdx[5];                     // Index into sensor data tables
} SimACDefType;

typedef struct RackGroup
{
    int nentries;
    int *entries;
} RackGroup;

typedef struct RackObject
{
    int ctind;
    int maxoccupancy;
} RackObject;
// ===============================================
// Externals
// ===============================================

extern UnitClassDataType* UnitDataTable;
extern ObjClassDataType* ObjDataTable;
extern FeatureEntry* FeatureEntryDataTable;
extern WeaponClassDataType* WeaponDataTable;
extern FeatureClassDataType* FeatureDataTable;
extern VehicleClassDataType* VehicleDataTable;
extern SquadronStoresDataType* SquadronStoresDataTable;
extern PtHeaderDataType* PtHeaderDataTable;
extern PtDataType* PtDataTable;
extern SimWeaponDataType* SimWeaponDataTable;
extern SimACDefType*           SimACDefTable;
extern RocketClassDataType* RocketDataTable; // Added by M.N.
extern DirtyDataClassType* DDP;

extern Falcon4EntityClassType* Falcon4ClassTable;

extern short NumObjectiveTypes;
extern int F4GenericTruckType;
extern int F4GenericUSTruckType;

extern RackGroup *RackGroupTable;
extern int MaxRackGroups;
extern RackObject *RackObjectTable;
extern int MaxRackObjects;
// ===============================================
// Functions
// ===============================================

extern int LoadClassTable(char *filename);

extern int UnloadClassTable(void);

extern int LoadUnitData(char *filename);

extern int LoadObjectiveData(char *filename);

extern int LoadWeaponData(char *filename);

extern int LoadRocketData(char *filename); // 2001-11-05 Added by M.N.

extern int LoadDirtyData(char *filename); // 2002-04-20 Added by M.N.

extern int LoadFeatureData(char *filename);

extern int LoadVehicleData(char *filename);

extern int LoadWeaponListData(char *filename);

extern int LoadPtHeaderData(char *filename);

extern int LoadPtData(char *filename);

extern int LoadFeatureEntryData(char *filename);

extern int LoadRadarData(char *filename);

extern int LoadIRSTData(char *filename);
extern int LoadRwrData(char *filename);
extern int LoadVisualData(char *filename);

extern int LoadSimWeaponData(char *filename);

extern void InitEntityClasses(void);

extern int GetClassID(uchar domain, uchar eclass, uchar type, uchar stype, uchar sp, uchar owner, uchar c6, uchar c7);

extern char *GetClassName(int ID);
extern DWORD MapVisId(DWORD ID);
void LoadRackTables(void); // JPO
int FindBestRackID(int rackgroup, int count);
int FindBestRackIDByPlaneAndWeapon(int planerg, int weaponrg, int count);

#endif


