/** @file Entity.cpp
* Routines for loading an manipulating the entity class table and class data
*/
#include "falclib.h"
#include "entity.h"
#include "ClassTbl.h"
#include "feature.h"
#include "vehicle.h"
#include "f4find.h"
#include "WeapList.h"
#include "rdrackdata.h"
#include "sim/include/visualData.h"
#include "sim/include/irstData.h"
#include "sim/include/rwrData.h"
#include "sim/include/radarData.h"
#include "falclib/include/alist.h"
#include "falclib/include/token.h"


extern int F4SessionUpdateTime;
extern int F4SessionAliveTimeout;
extern BOOL LoadMissionData();
extern bool g_bDisplayTrees;
extern bool g_bFFDBC;
extern bool g_bCheckFeatureIndex;

extern int g_nSessionTimeout;
extern int g_nSessionUpdateRate;
extern int g_nMiniDump;//Cobra

//
// Globals
//
// VP_changes the segment is important for creation og ClassTable
UnitClassDataType* UnitDataTable = NULL;
ObjClassDataType* ObjDataTable = NULL;
FeatureEntry* FeatureEntryDataTable = NULL;
WeaponClassDataType* WeaponDataTable = NULL;
FeatureClassDataType* FeatureDataTable = NULL;
VehicleClassDataType* VehicleDataTable = NULL;
WeaponListDataType* WeaponListDataTable = NULL;
SquadronStoresDataType* SquadronStoresDataTable = NULL;
PtHeaderDataType* PtHeaderDataTable = NULL;
PtDataType* PtDataTable = NULL;
SimWeaponDataType* SimWeaponDataTable = NULL;
SimACDefType*           SimACDefTable;
RocketClassDataType* RocketDataTable = NULL; // 2001-11-05 Added by M.N.
DirtyDataClassType* DDP = NULL; // 2002-04-20 Added by M.N.

RackGroup *RackGroupTable ;
RackObject *RackObjectTable;

short NumUnitEntries;
short NumObjectiveEntries;
short NumObjFeatEntries;
short NumVehicleEntries;
short NumFeatureEntries;
short NumSquadTypes;
short NumObjectiveTypes;
short NumWeaponTypes;
short NumPtHeaders;
short NumPts;
short NumSimWeaponEntries;
short NumACDefEntries;
short NumRocketTypes; // 2001-11-05 Added by M.N.
short NumDirtyDataPriorities;   // 2002-04-20 Added by M.N.

bool fedtree;

int MaxRackObjects = 0;
int MaxRackGroups = 0;
// A description index for our generic entity classes
int SFXType;
int F4SessionType;
int F4GroupType;
int F4GameType;
int F4FlyingEyeType;
int F4GenericTruckType;
int F4GenericUSTruckType;

// Rack id's for each rack type
short gRackId_Single_Rack;
short gRackId_Triple_Rack;
short gRackId_Quad_Rack;
short gRackId_Six_Rack;
short gRackId_Two_Rack;
short gRackId_Single_AA_Rack;
short gRackId_Mav_Rack;

// Id of our generic rocket.
short gRocketId;

// =====================
// Prototypes
// =====================

void UpdateVehicleCombatStatistics(void);
void UpdateFeatureCombatStatistics(void);
void UpdateUnitCombatStatistics(void);
void UpdateObjectiveCombatStatistics(void);
int LoadFeatureEntryData(char *filename);
int LoadACDefData(char*);
int LoadSquadronStoresData(char *filename);
extern int FileVerify(void);

extern FILE* OpenCampFile(char *filename, char *ext, char *mode);
void WriteClassTable();
void ReadClassTable();
void LoadVisIdMap();

//
// Functions
//

void WriteClassTable(void)
{
    /*
       if ( not VirtualProtect (Falcon4ClassTable, NumEntities * sizeof (Falcon4EntityClassType), PAGE_READWRITE, NULL))
       {
       ShiAssert ( not "Cannot Read/Write Protect ClassTable\n");
       }
     */
}

void ReadClassTable(void)
{
    /*
       if ( not VirtualProtect (Falcon4ClassTable, NumEntities * sizeof (Falcon4EntityClassType), PAGE_READONLY, NULL))
       {
       ShiAssert ( not "Cannot ReadOnly Protect ClassTable\n");
       }
     */
}

FILE *ErrorFH; // MLR 5/15/2004 -

int LoadClassTable(char *filename)
{
    int i;
    char *objSet;
    char *newstr;

    ErrorFH = 0L;

    if (g_bCheckFeatureIndex)
        ErrorFH = fopen("C:\\Objective-Errors.txt", "w"); // MLR 5/15/2004 -

#if 0 // not required JPO?
    HKEY theKey;
    DWORD size, type;
    // Get the path data from the registry
    RegOpenKeyEx(HKEY_LOCAL_MACHINE, FALCON_REGISTRY_KEY, 0, KEY_ALL_ACCESS, &theKey);

    size = sizeof(FalconObjectDataDir);
    RegQueryValueEx(theKey, "objectdir", 0, &type, (LPBYTE)FalconObjectDataDir, &size);
    RegCloseKey(theKey);
#endif
    objSet = newstr = strchr(FalconObjectDataDir, '\\');

    while (newstr)
    {
        objSet = newstr + 1;
        newstr = strchr(objSet, '\\');
    }

    // Check file integrity
    // FileVerify();

    InitClassTableAndData(filename, objSet);
    ReadClassTable();
#ifndef MISSILE_TEST_PROG
#ifndef ACMI
#ifndef IACONVERT

    if ( not LoadUnitData(filename)) ShiError("Failed to load unit data");

    if ( not LoadFeatureEntryData(filename)) ShiError("Failed to load feature entries");

    if ( not LoadObjectiveData(filename)) ShiError("Failed to load objective data");

    if ( not LoadWeaponData(filename)) ShiError("Failed to load weapon data");

    if ( not LoadFeatureData(filename)) ShiError("Failed to load feature data");

    if ( not LoadVehicleData(filename)) ShiError("Failed to load vehicle data");

    if ( not LoadWeaponListData(filename)) ShiError("Failed to load weapon list");

    if ( not LoadPtHeaderData(filename)) ShiError("Failed to load point headers");

    if ( not LoadPtData(filename)) ShiError("Failed to load point data");

    if ( not LoadRadarData(filename)) ShiError("Failed to load radar data");

    if ( not LoadIRSTData(filename)) ShiError("Failed to load IRST data");

    if ( not LoadRwrData(filename)) ShiError("Failed to load Rwr data");

    if ( not LoadVisualData(filename)) ShiError("Failed to load Visual data");

    if ( not LoadSimWeaponData(filename)) ShiError("Failed to load SimWeapon data");

    if ( not LoadACDefData(filename)) ShiError("Failed to load AC Definition data");

    if ( not LoadSquadronStoresData(filename)) ShiError("Failed to load Squadron stores data");

    if ( not LoadRocketData(filename)) ShiError("Failed to load Rocket data"); // added by M.N.

    if ( not LoadDirtyData(filename)) ShiError("Failed to load Dirty data priorities");   // added by M.N.

    LoadMissionData();
    LoadVisIdMap();
    LoadRackTables();
    RDLoadRackData();

    F4Assert(Falcon4ClassTable);
    WriteClassTable();

    // Build ptr data
    for (i = 0; i < NumEntities; i++)
    {
        if (Falcon4ClassTable[i].dataPtr not_eq NULL)
        {
            if (Falcon4ClassTable[i].dataType == DTYPE_UNIT)
            {
                // if (Falcon4ClassTable[i].vuClassData.classInfo_[VU_DOMAIN] == DOMAIN_AIR and Falcon4ClassTable[i].vuClassData.classInfo_[VU_TYPE] == TYPE_SQUADRON)
                // NumSquadTypes++;
                ShiAssert((int)Falcon4ClassTable[i].dataPtr < NumUnitEntries);
                UnitDataTable[(int)Falcon4ClassTable[i].dataPtr].Index = i;
                Falcon4ClassTable[i].dataPtr = (void*) &UnitDataTable[(int)Falcon4ClassTable[i].dataPtr];
            }
            else if (Falcon4ClassTable[i].dataType == DTYPE_OBJECTIVE)
            {
                if (Falcon4ClassTable[i].vuClassData.classInfo_[VU_TYPE] >= NumObjectiveTypes)
                    NumObjectiveTypes = Falcon4ClassTable[i].vuClassData.classInfo_[VU_TYPE];

                ShiAssert((int)Falcon4ClassTable[i].dataPtr < NumObjectiveEntries);
                ObjDataTable[(int)Falcon4ClassTable[i].dataPtr].Index = i;
                Falcon4ClassTable[i].dataPtr = (void*) &ObjDataTable[(int)Falcon4ClassTable[i].dataPtr];
            }
            else if (Falcon4ClassTable[i].dataType == DTYPE_WEAPON)
            {
                ShiAssert((int)Falcon4ClassTable[i].dataPtr < NumWeaponTypes);
                WeaponDataTable[(int)Falcon4ClassTable[i].dataPtr].Index = i;
                Falcon4ClassTable[i].dataPtr = (void*) &WeaponDataTable[(int)Falcon4ClassTable[i].dataPtr];
            }
            else if (Falcon4ClassTable[i].dataType == DTYPE_FEATURE)
            {
                ShiAssert((int)Falcon4ClassTable[i].dataPtr < NumFeatureEntries);
                FeatureDataTable[(int)Falcon4ClassTable[i].dataPtr].Index = i;
                Falcon4ClassTable[i].dataPtr = (void*) &FeatureDataTable[(int)Falcon4ClassTable[i].dataPtr];
            }
            else if (Falcon4ClassTable[i].dataType == DTYPE_VEHICLE)
            {
                ShiAssert((int)Falcon4ClassTable[i].dataPtr < NumVehicleEntries);
                VehicleDataTable[(int)Falcon4ClassTable[i].dataPtr].Index = i;
                Falcon4ClassTable[i].dataPtr = (void*) &VehicleDataTable[(int)Falcon4ClassTable[i].dataPtr];
            }
            else
                Falcon4ClassTable[i].dataPtr = NULL;
        }
    }

    ReadClassTable();
    // Update some precalculated statistics
    // KCK: I do these precalculations in the classtable builder now..
    // UpdateVehicleCombatStatistics();
    // UpdateFeatureCombatStatistics();
    // UpdateUnitCombatStatistics();
    // UpdateObjectiveCombatStatistics();
    // Set our special indices;
    SFXType = GetClassID(DOMAIN_ABSTRACT, CLASS_SFX, TYPE_ANY, STYPE_ANY, SPTYPE_ANY, VU_ANY, VU_ANY, VU_ANY);
    F4SessionType = GetClassID(DOMAIN_ABSTRACT, CLASS_SESSION, TYPE_ANY, STYPE_ANY, SPTYPE_ANY, VU_ANY, VU_ANY, VU_ANY);
    F4GroupType = GetClassID(DOMAIN_ABSTRACT, CLASS_GROUP, TYPE_ANY, STYPE_ANY, SPTYPE_ANY, VU_ANY, VU_ANY, VU_ANY);
    F4GameType = GetClassID(DOMAIN_ABSTRACT, CLASS_GAME, TYPE_ANY, STYPE_ANY, SPTYPE_ANY, VU_ANY, VU_ANY, VU_ANY);
    F4FlyingEyeType = GetClassID(DOMAIN_ABSTRACT, CLASS_ABSTRACT, TYPE_FLYING_EYE, STYPE_ANY, SPTYPE_ANY, VU_ANY, VU_ANY, VU_ANY);
    F4GenericTruckType = GetClassID(DOMAIN_LAND, CLASS_VEHICLE, TYPE_WHEELED, STYPE_WHEELED_TRANSPORT, SPTYPE_KrAz255B, VU_ANY, VU_ANY, VU_ANY);
    // KCK: Temporary until the classtable gets rebuilt - then replace with the commented out line
    // F4GenericUSTruckType = GetClassID(DOMAIN_LAND,CLASS_VEHICLE,TYPE_WHEELED,STYPE_WHEELED_TRANSPORT,SPTYPE_HUMMVCARGO,VU_ANY,VU_ANY,VU_ANY);
    F4GenericUSTruckType = 534;
    // F4GenericCrewType = GetClassID(DOMAIN_LAND,CLASS_VEHICLE,TYPE_FOOT,STYPE_FOOT_SQUAD,SPTYPE_DPRKARTSQD,VU_ANY,VU_ANY,VU_ANY);
    // Set our special rack Ids
    i = GetClassID(DOMAIN_ABSTRACT, CLASS_WEAPON, TYPE_RACK, STYPE_RACK, SPTYPE_SINGLE, VU_ANY, VU_ANY, VU_ANY);
    gRackId_Single_Rack = (short)(((int)Falcon4ClassTable[i].dataPtr - (int)WeaponDataTable) / sizeof(WeaponClassDataType));
    i = GetClassID(DOMAIN_ABSTRACT, CLASS_WEAPON, TYPE_RACK, STYPE_RACK, SPTYPE_TRIPLE, VU_ANY, VU_ANY, VU_ANY);
    gRackId_Triple_Rack = (short)(((int)Falcon4ClassTable[i].dataPtr - (int)WeaponDataTable) / sizeof(WeaponClassDataType));
    i = GetClassID(DOMAIN_ABSTRACT, CLASS_WEAPON, TYPE_RACK, STYPE_RACK, SPTYPE_QUAD, VU_ANY, VU_ANY, VU_ANY);
    gRackId_Quad_Rack = (short)(((int)Falcon4ClassTable[i].dataPtr - (int)WeaponDataTable) / sizeof(WeaponClassDataType));
    i = GetClassID(DOMAIN_ABSTRACT, CLASS_WEAPON, TYPE_RACK, STYPE_RACK, SPTYPE_SIX, VU_ANY, VU_ANY, VU_ANY);
    gRackId_Six_Rack = (short)(((int)Falcon4ClassTable[i].dataPtr - (int)WeaponDataTable) / sizeof(WeaponClassDataType));
    i = GetClassID(DOMAIN_ABSTRACT, CLASS_WEAPON, TYPE_RACK, STYPE_RACK, SPTYPE_2RAIL, VU_ANY, VU_ANY, VU_ANY);
    gRackId_Two_Rack = (short)(((int)Falcon4ClassTable[i].dataPtr - (int)WeaponDataTable) / sizeof(WeaponClassDataType));
    i = GetClassID(DOMAIN_ABSTRACT, CLASS_WEAPON, TYPE_RACK, STYPE_RACK, SPTYPE_SINGLE_AA, VU_ANY, VU_ANY, VU_ANY);
    gRackId_Single_AA_Rack = (short)(((int)Falcon4ClassTable[i].dataPtr - (int)WeaponDataTable) / sizeof(WeaponClassDataType));
    i = GetClassID(DOMAIN_ABSTRACT, CLASS_WEAPON, TYPE_RACK, STYPE_RACK, SPTYPE_MAVRACK, VU_ANY, VU_ANY, VU_ANY);
    gRackId_Mav_Rack = (short)(((int)Falcon4ClassTable[i].dataPtr - (int)WeaponDataTable) / sizeof(WeaponClassDataType));
    // Find our "Rocket Type". That is, the rocket all aircraft rocket pods will fire.
    i = GetClassID(DOMAIN_AIR, CLASS_VEHICLE, TYPE_ROCKET, STYPE_ROCKET, SPTYPE_2_75mm, VU_ANY, VU_ANY, VU_ANY);
    gRocketId = (short)(((int)Falcon4ClassTable[i].dataPtr - (int)WeaponDataTable) / sizeof(WeaponClassDataType));
    // Special hardcoded class data for sessions/groups/games
    WriteClassTable();
    Falcon4ClassTable[F4SessionType].vuClassData.managementDomain_ = VU_GLOBAL_DOMAIN;
    Falcon4ClassTable[F4SessionType].vuClassData.global_ = TRUE;
    Falcon4ClassTable[F4SessionType].vuClassData.persistent_ = TRUE;

    // KCK: High disconnect time for comms debugging
    if (F4SessionAliveTimeout)
    {
        Falcon4ClassTable[F4SessionType].vuClassData.updateTolerance_ = F4SessionAliveTimeout * 1000;
    }
    else
    {
#ifdef DEBUG
        Falcon4ClassTable[F4SessionType].vuClassData.updateTolerance_ = 30000; // MS before a session times out
#else
        Falcon4ClassTable[F4SessionType].vuClassData.updateTolerance_ = g_nSessionTimeout * 1000; // MS before a session times out
#endif
    }

    if (F4SessionUpdateTime)
    {
        Falcon4ClassTable[F4SessionType].vuClassData.updateRate_ = F4SessionUpdateTime * 1000;
    }
    else
    {
        Falcon4ClassTable[F4SessionType].vuClassData.updateRate_ = g_nSessionUpdateRate * 1000; // MS before a session update
    }

    Falcon4ClassTable[F4GroupType].vuClassData.updateRate_ = Falcon4ClassTable[F4SessionType].vuClassData.updateRate_;

    Falcon4ClassTable[F4GroupType].vuClassData.managementDomain_ = VU_GLOBAL_DOMAIN;
    Falcon4ClassTable[F4GroupType].vuClassData.global_ = TRUE;
    Falcon4ClassTable[F4GroupType].vuClassData.persistent_ = FALSE;
    Falcon4ClassTable[F4GameType].vuClassData.managementDomain_ = VU_GLOBAL_DOMAIN;
    Falcon4ClassTable[F4GameType].vuClassData.global_ = TRUE;
    Falcon4ClassTable[F4GameType].vuClassData.persistent_ = FALSE;

    Falcon4ClassTable[F4FlyingEyeType].vuClassData.fineUpdateMultiplier_ = 0.2F;
#endif
#endif
#endif
    ReadClassTable();

    if (ErrorFH)
        fclose(ErrorFH); // MLR 5/15/2004 -

    return 1;
}

int UnloadClassTable(void)
{
    delete [] UnitDataTable;
    delete [] ObjDataTable;
    delete [] WeaponDataTable;
    delete [] FeatureDataTable;
    delete [] VehicleDataTable;
    delete [] WeaponListDataTable;
    delete [] SquadronStoresDataTable;
    delete [] Falcon4ClassTable;
    delete [] PtHeaderDataTable;
    delete [] PtDataTable;
    delete [] FeatureEntryDataTable;
    delete [] RadarDataTable;
    delete [] IRSTDataTable;
    delete [] RwrDataTable;
    delete [] VisualDataTable;
    delete [] SimWeaponDataTable;
    delete [] SimACDefTable;
    delete [] RocketDataTable; // Added by M.N.
    delete [] DDP; // Added by M.N.
    delete [] RackGroupTable;
    delete [] RackObjectTable;
    MaxRackObjects = 0;
    MaxRackGroups = 0;
    RDUnloadRackData();
    return (TRUE);
}

#ifndef MISSILE_TEST_PROG
#ifndef ACMI
#ifndef IACONVERT
int LoadUnitData(char *filename)
{
    FILE* fp;
    short entries;

    if ((fp = OpenCampFile(filename, "UCD", "rb")) == NULL)
        return 0;

    fseek(fp, 0, SEEK_END); // JPO - work out if the file looks the right size.
    unsigned int size = ftell(fp);

    // FF - DB Control
    //fseek(fp, 0, SEEK_SET);
    //fread(&entries,sizeof(short),1,fp);
    //fseek(fp, 0, SEEK_SET);
    //if (entries == 0)
    // g_bFFDBC = true;
    if (g_bFFDBC)
    {
        // FF - get real count of entries
        short iknt = 0;
        fseek(fp, 0, SEEK_END);
        fseek(fp, -2, SEEK_CUR);
        fread(&iknt, sizeof(short), 1, fp);
        fseek(fp, 0, SEEK_SET);
        // Move pointer past the 0 entries
        fread(&entries, sizeof(short), 1, fp);

        if (entries > 0)
            g_bFFDBC = false;
        else
            entries = iknt;
    }
    else
    {
        fseek(fp, 0, SEEK_SET);

        if (fread(&entries, sizeof(short), 1, fp) < 1)
            return 0;

        if (size not_eq sizeof(UnitClassDataType) * entries + 2)
            return 0;
    }

    UnitDataTable = new UnitClassDataType[entries];
    fread(UnitDataTable, sizeof(UnitClassDataType), entries, fp);
    fclose(fp);
    NumUnitEntries = entries;

    return 1;
}

int LoadObjectiveData(char *filename)
{
    FILE* fp;
    // int i,j,fid;
    short entries;
    //char fname[64];



    //strcpy (fname, filename); // M.N. switch between objectives with and without trees
    //if (g_bDisplayTrees)
    // strcat(fname,"tree");

    //if ((fp = OpenCampFile(fname, "OCD", "rb")) == NULL)
    //{
    // if (g_bDisplayTrees and fedtree) // we've loaded a fedtree previously, so we also need a ocdtree version
    // {
    // ShiError( "Failed to load objective data" );
    // return 0;
    // }
    if ((fp = OpenCampFile(filename, "OCD", "rb")) == NULL) // if we have no "tree" version, just load the standard one
        return 0;

    //}
    fseek(fp, 0, SEEK_END); // JPO - work out if the file looks the right size.
    unsigned int size = ftell(fp);

    // FF - DB Control
    if (g_bFFDBC)
    {
        // FF - get real count of entries
        short iknt = 0;
        fseek(fp, 0, SEEK_END);
        fseek(fp, -2, SEEK_CUR);
        fread(&iknt, sizeof(short), 1, fp);
        fseek(fp, 0, SEEK_SET);
        // Move pointer past the 0 entries
        fread(&entries, sizeof(short), 1, fp);

        if (entries == 0)
            entries = iknt;
    }
    else
    {
        fseek(fp, 0, SEEK_SET);

        if (fread(&entries, sizeof(short), 1, fp) < 1)
            return 0;

        if (size not_eq sizeof(ObjClassDataType) * entries + 2)
            return 0;
    }

    //fseek(fp, 0, SEEK_SET);
    //if (fread(&entries,sizeof(short),1,fp) < 1)
    // return 0;
    //if (size not_eq sizeof(ObjClassDataType) * entries + 2)
    // return 0;
    ObjDataTable = new ObjClassDataType[entries];
    fread(ObjDataTable, sizeof(ObjClassDataType), entries, fp);
    fclose(fp);
    /*
    // Build feature ids
    for (i=0; i<entries; i++)
    {
    fid = ObjDataTable[i].FirstFeature;
    for (j=0; j<ObjDataTable[i].Features; j++)
    {
    FeatureEntryDataTable[fid].Index = GetClassID(FeatureEntryDataTable[fid].eClass[0],
    FeatureEntryDataTable[fid].eClass[1],FeatureEntryDataTable[fid].eClass[2],
    FeatureEntryDataTable[fid].eClass[3],FeatureEntryDataTable[fid].eClass[4],
    FeatureEntryDataTable[fid].eClass[5],FeatureEntryDataTable[fid].eClass[6],
    FeatureEntryDataTable[fid].eClass[7]);
    fid++;
    }
    }
     */
    NumObjectiveEntries = entries;
    return 1;
}

int LoadWeaponData(char *filename)
{
    FILE* fp;
    short entries;

    if ((fp = OpenCampFile(filename, "WCD", "rb")) == NULL)
        return 0;

    fseek(fp, 0, SEEK_END); // JPO - work out if the file looks the right size.
    unsigned int size = ftell(fp);

    // FF - DB Control
    if (g_bFFDBC)
    {
        // FF - get real count of entries
        short iknt = 0;
        fseek(fp, 0, SEEK_END);
        fseek(fp, -2, SEEK_CUR);
        fread(&iknt, sizeof(short), 1, fp);
        fseek(fp, 0, SEEK_SET);
        // Move pointer past the 0 entries
        fread(&entries, sizeof(short), 1, fp);

        if (entries == 0)
            entries = iknt;
    }
    else
    {
        fseek(fp, 0, SEEK_SET);

        if (fread(&entries, sizeof(short), 1, fp) < 1)
            return 0;

        if (size not_eq sizeof(WeaponClassDataType) * entries + 2)
            return 0;
    }

    //fseek(fp, 0, SEEK_SET);
    //if (fread(&entries,sizeof(short),1,fp) < 1)
    // return 0;
    //if (size not_eq sizeof(WeaponClassDataType) * entries + 2)
    // return 0;
    WeaponDataTable = new WeaponClassDataType[entries];
    fread(WeaponDataTable, sizeof(WeaponClassDataType), entries, fp);
    fclose(fp);
    NumWeaponTypes = entries;
    return 1;
}

// 2001-11-05 ADDED by M.N. Forget the rocket hacking in SMS.cpp, use a datafile instead

int LoadRocketData(char *filename)
{
    FILE* fp;
    short entries;

    if ((fp = OpenCampFile(filename, "RKT", "rb")) == NULL)
        return 0;

    fseek(fp, 0, SEEK_END); // JPO - work out if the file looks the right size.
    unsigned int size = ftell(fp);

    // FF - DB Control
    if (g_bFFDBC)
    {
        // FF - get real count of entries
        short iknt = 0;
        fseek(fp, 0, SEEK_END);
        fseek(fp, -2, SEEK_CUR);
        fread(&iknt, sizeof(short), 1, fp);
        fseek(fp, 0, SEEK_SET);
        // Move pointer past the 0 entries
        fread(&entries, sizeof(short), 1, fp);

        if (entries == 0)
            entries = iknt;
    }
    else
    {
        fseek(fp, 0, SEEK_SET);

        if (fread(&entries, sizeof(short), 1, fp) < 1)
            return 0;

        if (size not_eq sizeof(RocketClassDataType) * entries + 2)
            return 0;
    }

    //fseek(fp, 0, SEEK_SET);
    //if (fread(&entries,sizeof(short),1,fp) < 1)
    // return 0;
    //if (size not_eq sizeof(RocketClassDataType) * entries + 2)
    // return 0;
    RocketDataTable = new RocketClassDataType[entries];
    fread(RocketDataTable, sizeof(RocketClassDataType), entries, fp);
    fclose(fp);
    NumRocketTypes = entries;
    return 1;
}

// END of added section

// 2002-04-20 ADDED by M.N. Read in dirty data message priorities

int LoadDirtyData(char *filename)
{
    FILE* fp;
    short entries;

    if ((fp = OpenCampFile(filename, "DDP", "rb")) == NULL)
        return 0;

    fseek(fp, 0, SEEK_END); // JPO - work out if the file looks the right size.
    unsigned int size = ftell(fp);

    // FF - DB Control
    if (g_bFFDBC)
    {
        // FF - get real count of entries
        short iknt = 0;
        fseek(fp, 0, SEEK_END);
        fseek(fp, -2, SEEK_CUR);
        fread(&iknt, sizeof(short), 1, fp);
        fseek(fp, 0, SEEK_SET);
        // Move pointer past the 0 entries
        fread(&entries, sizeof(short), 1, fp);

        if (entries == 0)
            entries = iknt;
    }
    else
    {
        fseek(fp, 0, SEEK_SET);

        if (fread(&entries, sizeof(short), 1, fp) < 1)
            return 0;

        if (size not_eq sizeof(DirtyDataClassType) * entries + 2)
            return 0;
    }

    //fseek(fp, 0, SEEK_SET);
    //if (fread(&entries,sizeof(short),1,fp) < 1)
    // return 0;
    //if (size not_eq sizeof(DirtyDataClassType) * entries + 2)
    // return 0;
    DDP = new DirtyDataClassType[entries];
    fread(DDP, sizeof(DirtyDataClassType), entries, fp);
    fclose(fp);
    NumDirtyDataPriorities = entries;
    return 1;
}

// END of added section




int LoadFeatureData(char *filename)
{
    FILE* fp;
    short entries;

    if ((fp = OpenCampFile(filename, "FCD", "rb")) == NULL)
        return 0;

    fseek(fp, 0, SEEK_END); // JPO - work out if the file looks the right size.
    unsigned int size = ftell(fp);

    // FF - DB Control
    if (g_bFFDBC)
    {
        // FF - get real count of entries
        short iknt = 0;
        fseek(fp, 0, SEEK_END);
        fseek(fp, -2, SEEK_CUR);
        fread(&iknt, sizeof(short), 1, fp);
        fseek(fp, 0, SEEK_SET);
        // Move pointer past the 0 entries
        fread(&entries, sizeof(short), 1, fp);

        if (entries == 0)
            entries = iknt;
    }
    else
    {
        fseek(fp, 0, SEEK_SET);

        if (fread(&entries, sizeof(short), 1, fp) < 1)
            return 0;

        if (size not_eq sizeof(FeatureClassDataType) * entries + 2)
            return 0;
    }

    //fseek(fp, 0, SEEK_SET);
    //if (fread(&entries,sizeof(short),1,fp) < 1)
    // return 0;
    //if (size not_eq sizeof(FeatureClassDataType) * entries + 2)
    // return 0;
    FeatureDataTable = new FeatureClassDataType[entries];
    fread(FeatureDataTable, sizeof(FeatureClassDataType), entries, fp);
    fclose(fp);
    NumFeatureEntries = entries;
    return 1;
}

int LoadVehicleData(char *filename)
{
    FILE* fp;
    short entries;

    if ((fp = OpenCampFile(filename, "VCD", "rb")) == NULL)
        return 0;

    fseek(fp, 0, SEEK_END); // JPO - work out if the file looks the right size.
    unsigned int size = ftell(fp);

    // FF - DB Control
    if (g_bFFDBC)
    {
        // FF - get real count of entries
        short iknt = 0;
        fseek(fp, 0, SEEK_END);
        fseek(fp, -2, SEEK_CUR);
        fread(&iknt, sizeof(short), 1, fp);
        fseek(fp, 0, SEEK_SET);
        // Move pointer past the 0 entries
        fread(&entries, sizeof(short), 1, fp);

        if (entries == 0)
            entries = iknt;
    }
    else
    {
        fseek(fp, 0, SEEK_SET);

        if (fread(&entries, sizeof(short), 1, fp) < 1)
            return 0;

        if (size not_eq sizeof(VehicleClassDataType) * entries + 2)
            return 0;
    }

    //fseek(fp, 0, SEEK_SET);
    //if (fread(&entries,sizeof(short),1,fp) < 1)
    // return 0;
    //if (size not_eq sizeof(VehicleClassDataType) * entries + 2)
    // return 0;
    VehicleDataTable = new VehicleClassDataType[entries];
    fread(VehicleDataTable, sizeof(VehicleClassDataType), entries, fp);
    fclose(fp);
    NumVehicleEntries = entries;
    return 1;
}

int LoadWeaponListData(char *filename)
{
    FILE* fp;
    short entries;

    if ((fp = OpenCampFile(filename, "WLD", "rb")) == NULL)
        return 0;

    fseek(fp, 0, SEEK_END); // JPO - work out if the file looks the right size.
    unsigned int size = ftell(fp);

    // FF - DB Control
    if (g_bFFDBC)
    {
        // FF - get real count of entries
        short iknt = 0;
        fseek(fp, 0, SEEK_END);
        fseek(fp, -2, SEEK_CUR);
        fread(&iknt, sizeof(short), 1, fp);
        fseek(fp, 0, SEEK_SET);
        // Move pointer past the 0 entries
        fread(&entries, sizeof(short), 1, fp);

        if (entries == 0)
            entries = iknt;
    }
    else
    {
        fseek(fp, 0, SEEK_SET);

        if (fread(&entries, sizeof(short), 1, fp) < 1)
            return 0;

        if (size not_eq sizeof(WeaponListDataType) * entries + 2)
            return 0;
    }

    //fseek(fp, 0, SEEK_SET);
    //if (fread(&entries,sizeof(short),1,fp) < 1)
    // return 0;
    //if (size not_eq sizeof(WeaponListDataType) * entries + 2)
    // return 0;
    WeaponListDataTable = new WeaponListDataType[entries];
    fread(WeaponListDataTable, sizeof(WeaponListDataType), entries, fp);
    fclose(fp);
    return 1;
}

int LoadPtHeaderData(char *filename)
{
    FILE* fp;

    if ((fp = OpenCampFile(filename, "PHD", "rb")) == NULL)
    {
        return 0;
    }

    fseek(fp, 0, SEEK_END); // JPO - work out if the file looks the right size.
    unsigned int size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (fread(&NumPtHeaders, sizeof(short), 1, fp) < 1)
    {
        return 0;
    }

    if (size not_eq sizeof(PtHeaderDataType) * NumPtHeaders + 2)
    {
        return 0;
    }

    PtHeaderDataTable = new PtHeaderDataType[NumPtHeaders];
    fread(PtHeaderDataTable, sizeof(PtHeaderDataType), NumPtHeaders, fp);
    fclose(fp);
    PtHeaderDataTable[0].cosHeading = 1.0F;

    if (g_bCheckFeatureIndex)
    {
        int l, t; // MLR 5/15/2004 - Sanity check

        for (l = 0; l < NumPtHeaders; l++)
        {
            if (PtHeaderDataTable[l].objID < NumObjectiveEntries)
            {
                int featureCount = ObjDataTable[PtHeaderDataTable[l].objID].Features;

                for (t = 0; t < MAX_FEAT_DEPEND; t++)
                {
                    if (PtHeaderDataTable[l].features[t] not_eq 255 and PtHeaderDataTable[l].features[t] >= featureCount)
                    {
                        if (ErrorFH)
                            fprintf(ErrorFH, "PtHeaderDataTable[%d].features[%d]=%d >= Objective[%d]'s Features %d\n",
                                    l, t, PtHeaderDataTable[l].features[t], PtHeaderDataTable[l].objID, featureCount);

                        PtHeaderDataTable[l].features[t] = 255;
                    }
                }
            }
            else
            {
                if (ErrorFH)
                    fprintf(ErrorFH, "PtHeaderDataTable[%d].objId=%d >= NumObjectiveEntries=%d\n",
                            l, PtHeaderDataTable[l].objID, NumObjectiveEntries);
            }
        }
    }

    // org int l,t; // FRB - Sanity check
    int l; // FRB - Sanity check

    for (l = 0; l < NumObjectiveEntries; l++)
    {
        if ((ObjDataTable[l].PtDataIndex >= NumPtHeaders) or (ObjDataTable[l].PtDataIndex < 0))
        {
            if (ErrorFH)
                fprintf(ErrorFH, "ObjDataTable[%d].PtDataIndex >= NumPtHeaders = %d or < 0\n",
                        l, ObjDataTable[l].PtDataIndex, NumPtHeaders);

            ObjDataTable[l].PtDataIndex = 0;
        }
    }

    return 1;
}

int LoadPtData(char *filename)
{
    FILE* fp;

    if ((fp = OpenCampFile(filename, "PD", "rb")) == NULL)
    {
        return 0;
    }

    fseek(fp, 0, SEEK_END); // JPO - work out if the file looks the right size.
    unsigned int size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (fread(&NumPts, sizeof(short), 1, fp) < 1)
    {
        return 0;
    }

    if (size not_eq sizeof(PtDataType) * NumPts + 2)
    {
        return 0;
    }

    PtDataTable = new PtDataType[NumPts];
    fread(PtDataTable, sizeof(PtDataType), NumPts, fp);
    fclose(fp);
    return 1;
}

int LoadFeatureEntryData(char *filename)
{
    FILE* fp;
    char fname[64];

    fedtree = false;
    strcpy(fname, filename); // M.N. switch between objectives with and without trees

    if (g_bDisplayTrees)
    {
        strcat(fname, "tree");
        fedtree = true;
    }

    if ((fp = OpenCampFile(fname, "FED", "rb")) == NULL)
    {
        fedtree = false;

        if ((fp = OpenCampFile(filename, "FED", "rb")) == NULL) // if we have no "tree" version, just load the standard one
            return 0;
    }

    fseek(fp, 0, SEEK_END); // JPO - work out if the file looks the right size.
    unsigned int size = ftell(fp);

    // FF - DB Control
    if (g_bFFDBC)
    {
        // FF - get real count of entries
        short iknt = 0;
        fseek(fp, 0, SEEK_END);
        fseek(fp, -2, SEEK_CUR);
        fread(&iknt, sizeof(short), 1, fp);
        fseek(fp, 0, SEEK_SET);
        // Move pointer past the 0 entries
        fread(&NumObjFeatEntries, sizeof(short), 1, fp);

        if (NumObjFeatEntries == 0)
            NumObjFeatEntries = iknt;
    }
    else
    {
        fseek(fp, 0, SEEK_SET);

        if (fread(&NumObjFeatEntries, sizeof(short), 1, fp) < 1)
            return 0;

        if (size not_eq sizeof(FeatureEntry) * NumObjFeatEntries + 2)
            return 0;
    }

    //fseek(fp, 0, SEEK_SET);
    //if (fread(&NumObjFeatEntries,sizeof(short),1,fp) < 1)
    // return 0;
    //if (size not_eq sizeof(FeatureEntry) * NumObjFeatEntries + 2)
    // return 0;
    FeatureEntryDataTable = new FeatureEntry[NumObjFeatEntries];
    fread(FeatureEntryDataTable, sizeof(FeatureEntry), NumObjFeatEntries, fp);
    fclose(fp);
    return 1;
}

int LoadRadarData(char *filename)
{
    FILE* fp;

    if ((fp = OpenCampFile(filename, "RCD", "rb")) == NULL)
        return 0;

    fseek(fp, 0, SEEK_END); // JPO - work out if the file looks the right size.
    unsigned int size = ftell(fp);

    // FF - DB Control
    if (g_bFFDBC)
    {
        // FF - get real count of entries
        short iknt = 0, entries;
        fseek(fp, 0, SEEK_END);
        fseek(fp, -2, SEEK_CUR);
        fread(&iknt, sizeof(short), 1, fp);
        fseek(fp, 0, SEEK_SET);
        // Move pointer past the 0 entries
        fread(&entries, sizeof(short), 1, fp);

        if (NumRadarEntries == 0)
            NumRadarEntries = iknt;
    }
    else
    {
        fseek(fp, 0, SEEK_SET);

        if (fread(&NumRadarEntries, sizeof(short), 1, fp) < 1)
            return 0;

        if (size not_eq sizeof(RadarDataType) * NumRadarEntries + 2)
            return 0;
    }

    //fseek(fp, 0, SEEK_SET);
    //if (fread(&NumRadarEntries,sizeof(short),1,fp) < 1)
    // return 0;
    //if (size not_eq sizeof(RadarDataType) * NumRadarEntries + 2)
    // return 0;
    RadarDataTable = new RadarDataType[NumRadarEntries];
    ShiAssert(RadarDataTable);
    fread(RadarDataTable, sizeof(RadarDataType), NumRadarEntries, fp);
    fclose(fp);
    return 1;
}

int LoadIRSTData(char *filename)
{
    FILE* fp;

    if ((fp = OpenCampFile(filename, "ICD", "rb")) == NULL)
        return 0;

    fseek(fp, 0, SEEK_END); // JPO - work out if the file looks the right size.
    unsigned int size = ftell(fp);

    // FF - DB Control
    if (g_bFFDBC)
    {
        // FF - get real count of entries
        short iknt = 0, entries;
        fseek(fp, 0, SEEK_END);
        fseek(fp, -2, SEEK_CUR);
        fread(&iknt, sizeof(short), 1, fp);
        fseek(fp, 0, SEEK_SET);
        // Move pointer past the 0 entries
        fread(&entries, sizeof(short), 1, fp);

        if (NumIRSTEntries == 0)
            NumIRSTEntries = iknt;
    }
    else
    {
        fseek(fp, 0, SEEK_SET);

        if (fread(&NumIRSTEntries, sizeof(short), 1, fp) < 1)
            return 0;

        if (size not_eq sizeof(IRSTDataType) * NumIRSTEntries + 2)
            return 0;
    }

    //fseek(fp, 0, SEEK_SET);
    //if (fread(&NumIRSTEntries,sizeof(short),1,fp) < 1)
    // return 0;
    //if (size not_eq sizeof(IRSTDataType) * NumIRSTEntries + 2)
    // return 0;
    IRSTDataTable = new IRSTDataType[NumIRSTEntries];
    ShiAssert(IRSTDataTable);
    fread(IRSTDataTable, sizeof(IRSTDataType), NumIRSTEntries, fp);
    fclose(fp);
    return 1;
}

int LoadRwrData(char *filename)
{
    FILE* fp;

    if ((fp = OpenCampFile(filename, "rwd", "rb")) == NULL)
        return 0;

    fseek(fp, 0, SEEK_END); // JPO - work out if the file looks the right size.
    unsigned int size = ftell(fp);

    // FF - DB Control
    if (g_bFFDBC)
    {
        // FF - get real count of entries
        short iknt = 0, entries;
        fseek(fp, 0, SEEK_END);
        fseek(fp, -2, SEEK_CUR);
        fread(&iknt, sizeof(short), 1, fp);
        fseek(fp, 0, SEEK_SET);
        // Move pointer past the 0 entries
        fread(&entries, sizeof(short), 1, fp);

        if (NumRwrEntries == 0)
            NumRwrEntries = iknt;
    }
    else
    {
        fseek(fp, 0, SEEK_SET);

        if (fread(&NumRwrEntries, sizeof(short), 1, fp) < 1)
            return 0;

        if (size not_eq sizeof(RwrDataType) * NumRwrEntries + 2)
            return 0;
    }

    //fseek(fp, 0, SEEK_SET);
    //if (fread(&NumRwrEntries,sizeof(short),1,fp) < 1)
    // return 0;
    //if (size not_eq sizeof(RwrDataType) * NumRwrEntries + 2)
    // return 0;
    RwrDataTable = new RwrDataType[NumRwrEntries];
    ShiAssert(RwrDataTable);
    fread(RwrDataTable, sizeof(RwrDataType), NumRwrEntries, fp);
    fclose(fp);
    return 1;
}

int LoadVisualData(char *filename)
{
    FILE* fp;

    if ((fp = OpenCampFile(filename, "vsd", "rb")) == NULL)
        return 0;

    fseek(fp, 0, SEEK_END); // JPO - work out if the file looks the right size.
    unsigned int size = ftell(fp);

    // FF - DB Control
    if (g_bFFDBC)
    {
        // FF - get real count of entries
        short iknt = 0, entries;
        fseek(fp, 0, SEEK_END);
        fseek(fp, -2, SEEK_CUR);
        fread(&iknt, sizeof(short), 1, fp);
        fseek(fp, 0, SEEK_SET);
        // Move pointer past the 0 entries
        fread(&entries, sizeof(short), 1, fp);

        if (NumVisualEntries == 0)
            NumVisualEntries = iknt;
    }
    else
    {
        fseek(fp, 0, SEEK_SET);

        if (fread(&NumVisualEntries, sizeof(short), 1, fp) < 1)
            return 0;

        if (size not_eq sizeof(VisualDataType) * NumVisualEntries + 2)
            return 0;
    }

    //fseek(fp, 0, SEEK_SET);
    //if (fread(&NumVisualEntries,sizeof(short),1,fp) < 1)
    // return 0;
    //if (size not_eq sizeof(VisualDataType) * NumVisualEntries + 2)
    // return 0;
    VisualDataTable = new VisualDataType[NumVisualEntries];
    ShiAssert(VisualDataTable);
    fread(VisualDataTable, sizeof(VisualDataType), NumVisualEntries, fp);
    fclose(fp);
    return 1;
}

int LoadSimWeaponData(char *filename)
{
    FILE* fp;

    if ((fp = OpenCampFile(filename, "SWD", "rb")) == NULL)
        return 0;

    fseek(fp, 0, SEEK_END); // JPO - work out if the file looks the right size.
    unsigned int size = ftell(fp);

    // FF - DB Control
    if (g_bFFDBC)
    {
        // FF - get real count of entries
        short iknt = 0, entries;
        fseek(fp, 0, SEEK_END);
        fseek(fp, -2, SEEK_CUR);
        fread(&iknt, sizeof(short), 1, fp);
        fseek(fp, 0, SEEK_SET);
        // Move pointer past the 0 entries
        fread(&entries, sizeof(short), 1, fp);

        if (NumSimWeaponEntries == 0)
            NumSimWeaponEntries = iknt;
    }
    else
    {
        fseek(fp, 0, SEEK_SET);

        if (fread(&NumSimWeaponEntries, sizeof(short), 1, fp) < 1)
            return 0;

        if (size not_eq sizeof(SimWeaponDataType) * NumSimWeaponEntries + 2)
            return 0;
    }

    //fseek(fp, 0, SEEK_SET);
    //if (fread(&NumSimWeaponEntries,sizeof(short),1,fp) < 1)
    // return 0;
    //if (size not_eq sizeof(SimWeaponDataType) * NumSimWeaponEntries + 2)
    // return 0;
    SimWeaponDataTable = new SimWeaponDataType[NumSimWeaponEntries];
    ShiAssert(SimWeaponDataTable);
    fread(SimWeaponDataTable, sizeof(SimWeaponDataType), NumSimWeaponEntries, fp);
    fclose(fp);
    return 1;
}

int LoadACDefData(char *filename)
{
    FILE* fp;

    if ((fp = OpenCampFile(filename, "ACD", "rb")) == NULL)
        return 0;

    fseek(fp, 0, SEEK_END); // JPO - work out if the file looks the right size.
    unsigned int size = ftell(fp);

    // FF - DB Control
    if (g_bFFDBC)
    {
        // FF - get real count of entries
        short iknt = 0, entries;
        fseek(fp, 0, SEEK_END);
        fseek(fp, -2, SEEK_CUR);
        fread(&iknt, sizeof(short), 1, fp);
        fseek(fp, 0, SEEK_SET);
        // Move pointer past the 0 entries
        fread(&entries, sizeof(short), 1, fp);

        if (NumACDefEntries == 0)
            NumACDefEntries = iknt;
    }
    else
    {
        fseek(fp, 0, SEEK_SET);

        if (fread(&NumACDefEntries, sizeof(short), 1, fp) < 1)
            return 0;

        if (size not_eq sizeof(SimACDefType) * NumACDefEntries + 2)
            return 0;
    }

    //fseek(fp, 0, SEEK_SET);
    //if (fread(&NumACDefEntries,sizeof(short),1,fp) < 1)
    // return 0;
    //if (size not_eq sizeof(SimACDefType) * NumACDefEntries + 2)
    // return 0;
    SimACDefTable = new SimACDefType[NumACDefEntries];
    ShiAssert(SimACDefTable);
    fread(SimACDefTable, sizeof(SimACDefType), NumACDefEntries, fp);
    fclose(fp);
    return 1;
}

int LoadSquadronStoresData(char *filename)
{
    FILE *fp;

    if ((fp = OpenCampFile(filename, "SSD", "rb")) == NULL)
        return 0;

    fseek(fp, 0, SEEK_END); // JPO - work out if the file looks the right size.
    unsigned int size = ftell(fp);

    // FF - DB Control
    if (g_bFFDBC)
    {
        short iknt = 0, entries = 0;
        // FF - get real count of entries
        fseek(fp, 0, SEEK_END);
        fseek(fp, -2, SEEK_CUR);
        fread(&iknt, sizeof(short), 1, fp);
        fseek(fp, 0, SEEK_SET);
        // Move pointer past the 0 entries
        fread(&entries, sizeof(short), 1, fp);

        if (NumSquadTypes == 0)
            NumSquadTypes = iknt;
    }
    else
    {
        fseek(fp, 0, SEEK_SET);
        fread(&NumSquadTypes, sizeof(short), 1, fp);

        if (NumSquadTypes < 1)
            return 0;

        if (size not_eq sizeof(SquadronStoresDataType) * NumSquadTypes + 2)
            // MAXIMUM_WEAPTYPES = 600;
            return 0;
    }

    // Check for FF new record size
    //if ((size == sizeof(SquadronStoresDataType) * NumSquadTypes + 2)
    // and (sizeof(SquadronStoresDataType)-3 == FF_MAXIMUM_WEAPTYPES))
    // MAXIMUM_WEAPTYPES = FF_MAXIMUM_WEAPTYPES;
    //else
    // MAXIMUM_WEAPTYPES = SP_MAXIMUM_WEAPTYPES;

    SquadronStoresDataTable = new SquadronStoresDataType[NumSquadTypes];
    ShiAssert(SquadronStoresDataTable);
    fread(SquadronStoresDataTable, sizeof(SquadronStoresDataType), NumSquadTypes, fp);
    fclose(fp);
    return 1;
}

// =====================================================
// Routines to calulcate aprox values for quick searches
// =====================================================

#if 0
void UpdateVehicleCombatStatistics(void)
{
    int id, mt;

    for (id = 1; id < NumVehicleEntries; id++)
    {
        for (mt = 0; mt < MOVEMENT_TYPES; mt++)
        {
            // KCK: HitChance is now the inherent hitchance only
            // Units will use the calculated hitchance
            // VehicleDataTable[id].HitChance[mt] += CalculateVehicleHitChance(VehicleDataTable[id].Index,mt);
            VehicleDataTable[id].Range[mt] = CalculateVehicleRange(VehicleDataTable[id].Index, mt);
            VehicleDataTable[id].Strength[mt] = CalculateVehicleCombatStrength(VehicleDataTable[id].Index, mt);
        }
    }
}

void UpdateFeatureCombatStatistics(void)
{
    /* int id,mt;

     for (id=1; id < NumFeatureEntries; id++)
     {
     for (mt=0; mt<MOVEMENT_TYPES; mt++)
     {
     FeatureDataTable[id].HitChance[mt] = CalculateFeatureHitChance(FeatureDataTable[id].Index,mt);
     FeatureDataTable[id].Range[mt] = CalculateFeatureRange(FeatureDataTable[id].Index,mt);
     FeatureDataTable[id].Strength[mt] = CalculateFeatureCombatStrength(FeatureDataTable[id].Index,mt);
     }
     }
     */
}

void UpdateUnitCombatStatistics(void)
{
    int mt, i, j, id, vid, rng, mr, str, det, mdet, hc, mhc, dv, mspeed;
    int dam[OtherDam], tv, squads = 0;
    VehicleClassDataType* vc;
    uchar *vhc = new uchar[NumVehicleEntries * MOVEMENT_TYPES];

    SquadronStoresDataTable = new SquadronStoresDataType[NumSquadTypes];
    memset(SquadronStoresDataTable, 0, sizeof(SquadronStoresDataType)*NumSquadTypes);

    // Calculate vehicle hit chances - to copy into unit slots.
    memset(vhc, 0, MOVEMENT_TYPES); // Set 0th entry to zeros

    for (id = 1; id < NumVehicleEntries; id++)
    {
        for (mt = 0; mt < MOVEMENT_TYPES; mt++)
            vhc[id * MOVEMENT_TYPES + mt] = VehicleDataTable[id].HitChance[mt] + CalculateVehicleHitChance(VehicleDataTable[id].Index, mt);
    }

    for (id = 1; id < NumUnitEntries; id++)
    {
        dv = 255;
        mspeed = 999;

        for (mt = 0; mt < MOVEMENT_TYPES; mt++)
        {
            mr = str = mdet = mhc = 0;

            for (i = 0; i < VEHICLE_GROUPS_PER_UNIT; i++)
            {
                if (UnitDataTable[id].VehicleType[i])
                {
                    // HitChance is Inherent HC + best Weapon HC
                    vid = ((int)Falcon4ClassTable[UnitDataTable[id].VehicleType[i]].dataPtr - (int)VehicleDataTable) / sizeof(VehicleClassDataType);
                    hc = vhc[vid * MOVEMENT_TYPES + mt];

                    if (hc > mhc)
                        mhc = hc;

                    rng = GetAproxVehicleRange(UnitDataTable[id].VehicleType[i], mt);

                    if (rng > mr)
                        mr = rng;

                    det = GetVehicleDetectionRange(UnitDataTable[id].VehicleType[i], mt);

                    if (det > mdet)
                    {
                        mdet = det;
                        dv = i;
                    }

                    str += UnitDataTable[id].NumElements[i] * GetAproxVehicleCombatStrength(UnitDataTable[id].VehicleType[i], mt, 0);
                }
            }

            UnitDataTable[id].Range[mt] = mr;
            UnitDataTable[id].HitChance[mt] = mhc;
            UnitDataTable[id].Strength[mt] = str;
            UnitDataTable[id].Detection[mt] = mdet;
            UnitDataTable[id].RadarVehicle = dv;
        }

        tv = 1;
        memset(dam, 0, sizeof(int)*OtherDam);

        for (i = 0; i < VEHICLE_GROUPS_PER_UNIT; i++)
        {
            if (UnitDataTable[id].VehicleType[i])
            {
                vc = GetVehicleClassData(UnitDataTable[id].VehicleType[i]);

                for (j = 0; j < OtherDam and vc; j++)
                    dam[j] += vc->DamageMod[j];

                UnitDataTable[id].Flags or_eq vc->Flags bitand 0xFF;

                if (vc->MaxSpeed < mspeed)
                    mspeed = vc->MaxSpeed;

                tv += UnitDataTable[id].NumElements[i];
            }
        }

        for (j = 0; j < OtherDam; j++)
            UnitDataTable[id].DamageMod[j] = dam[j] / tv;

        // Determine max speed
        UnitDataTable[id].MovementSpeed = mspeed;

        if (mspeed == 0)
            UnitDataTable[id].MovementType = NoMove;

        if (Falcon4ClassTable[UnitDataTable[id].Index].vuClassData.classInfo_[0] == DOMAIN_AIR and 
            Falcon4ClassTable[UnitDataTable[id].Index].vuClassData.classInfo_[2] == TYPE_SQUADRON)
        {
            // Calculate squadron's max stores, by weapons it can load
            vc = GetVehicleClassData(UnitDataTable[id].VehicleType[0]);

            if (vc and squads < NumSquadTypes)
            {
                uchar baaweap = 0, bagweap = 0;
                short score, baascore = 9999, bagscore = 9999;

                // Total weapon usage
                for (i = 0; i < HARDPOINT_MAX; i++)
                {
                    if (vc->Weapons[i] == 255)
                    {
                        for (j = 0, mr = -1; j < MAX_WEAPONS_IN_LIST and mr; j++)
                        {
                            mr = GetListEntryWeapon(vc->Weapon[i], j);

                            if (mr > 0 and mr < 255)
                            {
                                SquadronStoresDataTable[squads].Stores[mr]++;

                                if ( not baaweap and GetWeaponScore(mr, Air, 0))
                                    baaweap = mr;

                                if ( not bagweap and GetWeaponScore(mr, NoMove, 0))
                                    bagweap = mr;
                            }
                        }
                    }
                    else if (vc->Weapon[i])
                        SquadronStoresDataTable[squads].Stores[vc->Weapon[i]]++;
                }

                // Pick worst AG and AA weapon
                for (i = 0; i < NumWeaponTypes; i++)
                {
                    if (SquadronStoresDataTable[squads].Stores[i])
                    {
                        score = GetWeaponScore(i, NoMove, 0);

                        if (score and score < bagscore and WeaponDataTable[i].DamageType == HighExplosiveDam)
                        {
                            bagscore = score;
                            bagweap = i;
                        }

                        score = GetWeaponScore(i, Air, 0);

                        if (score and score < baascore and i not_eq vc->Weapon[0])
                        {
                            baascore = score;
                            baaweap = i;
                        }
                    }
                }

                SquadronStoresDataTable[squads].infiniteAG = bagweap;
                SquadronStoresDataTable[squads].infiniteAA = baaweap;
                SquadronStoresDataTable[squads].infiniteGun = unsigned char(vc->Weapon[0]);

                // Adjust stockpile ratings
                for (i = 0; i < NumWeaponTypes; i++)
                {
                    j = SquadronStoresDataTable[squads].Stores[i] * 20;

                    if (j > 255 or WeaponDataTable[i].Flags bitand WEAP_ONETENTH)
                        j = 255;

                    SquadronStoresDataTable[squads].Stores[i] = (uchar)j;
                }

                UnitDataTable[id].SpecialIndex = squads;
                squads++;
            }
        }
    }

    delete vhc;
}

void UpdateObjectiveCombatStatistics(void)
{
    int mt, id, str, i, mr, det, mdet, mhc, df; // rng,hc;
    int dam[OtherDam], tf, j, fid;
    FeatureClassDataType* fc;

    for (id = 1; id < NumObjectiveEntries; id++)
    {
        df = 255;

        for (mt = 0; mt < MOVEMENT_TYPES; mt++)
        {
            mr = str = mdet = mhc = 0;
            fid = ObjDataTable[id].FirstFeature;

            for (i = 0; i < ObjDataTable[id].Features; i++)
            {
                det = GetFeatureDetectionRange(FeatureEntryDataTable[fid].Index, mt);

                if (det > mdet)
                {
                    mdet = det;
                    df = i;
                }

                fid++;
            }

            ObjDataTable[id].Detection[mt] = mdet;
            ObjDataTable[id].RadarFeature = df;
        }

        tf = 1;
        memset(dam, 0, sizeof(int)*OtherDam);
        fid = ObjDataTable[id].FirstFeature;

        for (i = 0; i < ObjDataTable[id].Features; i++)
        {
            fc = GetFeatureClassData(FeatureEntryDataTable[fid + i].Index);

            if (fc)
            {
                for (j = 0; j < OtherDam and fc; j++)
                    dam[j] += fc->DamageMod[j];

                tf++;
            }
        }

        for (j = 0; j < OtherDam; j++)
            ObjDataTable[id].DamageMod[j] = dam[j] / tf;
    }
}
#endif
#endif
#endif
#endif

int CheckClassEntry(int id, uchar filter[CLASS_NUM_BYTES])
{
    int
    i;

    /* compare class bytes */
    for (i = 0; i < CLASS_NUM_BYTES; i++)
    {
        if (filter[i] == VU_FILTERSTOP)
        {
            break;
        }
        else
        {
            if (filter[i] not_eq VU_FILTERANY)
            {
                if (filter[i] not_eq Falcon4ClassTable[id].vuClassData.classInfo_[i])
                {
                    return 0;
                }
            }
        }
    }

    return 1;
}

int GetClassID(uchar domain, uchar eclass, uchar type, uchar stype, uchar sp, uchar owner, uchar c6, uchar c7)
{
    int id;
    uchar filter[CLASS_NUM_BYTES];

    filter[0] = domain;
    filter[1] = eclass;
    filter[2] = type;
    filter[3] = stype;
    filter[4] = sp;
    filter[5] = owner;
    filter[6] = c6;
    filter[7] = c7;


    // This should eventually be smart enough to return the 'next best thing'
    for (id = 0; id < NumEntities; id++)
    {
        if (CheckClassEntry(id, filter))
            return id;
    }

    return 0;
}

char *GetClassName(int ID)
{
    int
    type;

    union
    {
        void *ptr;
        FeatureClassDataType *fc;
        ObjClassDataType *oc;
        UnitClassDataType *uc;
        VehicleClassDataType *vc;
        WeaponClassDataType *wc;
    }
    ptr;

    type = Falcon4ClassTable[ID].dataType;

    ptr.ptr = Falcon4ClassTable[ID].dataPtr;

    if (type == DTYPE_FEATURE)
    {
        return ptr.fc->Name;
    }
    else if (type == DTYPE_OBJECTIVE)
    {
        return ptr.oc->Name;
    }
    else if (type == DTYPE_UNIT)
    {
        return ptr.uc->Name;
    }
    else if (type == DTYPE_VEHICLE)
    {
        return ptr.vc->Name;
    }
    else if (type == DTYPE_WEAPON)
    {
        return ptr.wc->Name;
    }

    return NULL;
}

#define MAXMAPID 1400
static DWORD idmap[MAXMAPID]; // well simple is easiest

void LoadVisIdMap()
{
    FILE *fp;
    char buffer[128];
    int id1, id2;

    for (int i = 0; i < MAXMAPID; i++)
        idmap[i] = i; // identity map

    if ((fp = OpenCampFile("visid", "map", "rt")) == NULL)
        return;

    while (fgets(buffer, sizeof buffer, fp))
    {
        if (buffer[0] == '/' or buffer[0] == '\n' or buffer[0] == '\r')
            continue;

        if (sscanf(buffer, "%d => %d", &id1, &id2) == 2 and 
            id1 >= 0 and id1 < MAXMAPID)
            idmap[id1] = id2;
    }

    fclose(fp);
}

DWORD MapVisId(DWORD visId)
{
    if (visId >= 0 and visId < MAXMAPID)
        return idmap[visId];

    return visId;
}

void LoadRackTables()
{
    FILE *fp;
    int ngrp;
    char buffer[MAX_PATH];

    if ((fp = OpenCampFile("Rack", "dat", "rt")) == NULL)
    {
        sprintf(buffer, "%s\\%s", FalconObjectDataDir, "Rack.dat");

        if ((fp = fopen(buffer, "rt")) == NULL) return;
    }

    while (fgets(buffer, sizeof buffer, fp))
    {
        if (buffer[0] == '#' or buffer[0] == '\n')
            continue;

        if (sscanf(buffer, "NumGroups %d", &ngrp) not_eq 1)
            continue;

        ShiAssert(ngrp >= 0 and ngrp < 1000); // arbitrary 1000
        break;
    }

    if (feof(fp))
    {
        fclose(fp);
        return;
    }

    // read in the groups
    MaxRackGroups = ngrp;
    RackGroupTable = new RackGroup[MaxRackGroups];
    int rg = 0;

    while (rg < MaxRackGroups and fgets(buffer, sizeof buffer, fp))
    {
        if (buffer[0] == '#' or buffer[0] == '\n')
            continue;

        int grp;

        char *cp = buffer;

        if (sscanf(cp, "Group%d", &grp) not_eq 1)
            continue;

        cp += 5;
        ShiAssert(grp < MaxRackGroups); // well it should be

        while (isdigit(*cp)) cp++;

        while (isspace(*cp)) cp++;

        ngrp = atoi(cp);
        ShiAssert(ngrp >= 0 and ngrp < 1000); // arbitrary 1000
        RackGroupTable[grp].nentries = ngrp;
        RackGroupTable[grp].entries = new int [ngrp];

        while (isdigit(*cp)) cp++;

        while (isspace(*cp)) cp++;

        for (int i = 0; *cp and i < ngrp; i++)
        {
            RackGroupTable[grp].entries[i] = atoi(cp);

            while (isdigit(*cp)) cp++;

            while (isspace(*cp)) cp++;
        }

        rg ++;
    }

    // now read in the entries
    while (fgets(buffer, sizeof buffer, fp))
    {
        if (buffer[0] == '#' or buffer[0] == '\n')
            continue;

        if (sscanf(buffer, "NumRacks %d", &ngrp) not_eq 1)
            continue;

        ShiAssert(ngrp >= 0 and ngrp < 1000); // arbitrary 1000
        break;
    }

    if (feof(fp))
    {
        fclose(fp);
        return;
    }

    MaxRackObjects = ngrp;
    RackObjectTable = new RackObject[MaxRackObjects];
    memset(RackObjectTable, 0, sizeof(*RackObjectTable) * MaxRackObjects);
    int rack = 0;

    while (fgets(buffer, sizeof buffer, fp))
    {
        if (buffer[0] == '#' or buffer[0] == '\n')
            continue;

        int ctid, occ;

        if (sscanf(buffer, "Rack%d %d %d", &rack, &ctid, &occ) not_eq 3)
            continue;

        ShiAssert(rack < MaxRackObjects);
        ShiAssert(ctid >= 0 and ctid < NumEntities);
        RackObjectTable[rack].ctind = ctid;
        RackObjectTable[rack].maxoccupancy = occ;
    }

    fclose(fp);
}

int FindBestRackID(int rackgroup, int count)
{
    if (rackgroup < 0 or rackgroup >= MaxRackGroups)
        return -1;

    for (int i = 0; i < RackGroupTable[rackgroup].nentries; i++)
    {
        int rack = RackGroupTable[rackgroup].entries[i];

        if (rack > 0 and rack < MaxRackObjects and 
            RackObjectTable[rack].maxoccupancy >= count)
            return rack;
    }

    return -1; // not possible
}

int FindBestRackIDByPlaneAndWeapon(int planerg, int weaponrg, int count)
{
    if (planerg < 0 or planerg >= MaxRackGroups or
        weaponrg < 0 or weaponrg >= MaxRackGroups)
        return -1;

    // first find a rackgroup in common
    for (int i = 0; i < RackGroupTable[planerg].nentries; i++)
    {
        for (int j = 0; j < RackGroupTable[weaponrg].nentries; j++)
        {
            int rack = RackGroupTable[planerg].entries[i];

            if (rack == RackGroupTable[weaponrg].entries[j] and 
                rack > 0 and rack < MaxRackObjects and 
                RackObjectTable[rack].maxoccupancy >= count)
                return rack;
        }
    }

    return -1;
}


// MLR 2/12/2004 - replacing rack.dat with something more managable and more expandable

class RDRackNode : public ANode
{
public:
    RDRackNode();
    ~RDRackNode();

    char rackName[32];
    char mnemonic[12];
    int rackCT; // could be 0
    int stations;
    int swdCount;
    int *swd;
    int wIdCount;
    int *wId;
    int wClassCount;
    int *wClass;
    int any; // anything goes here
    int flags;
    AList loadOrder;
};

class RDLoadOrderNode : public ANode
{
public:
    RDLoadOrderNode(int Count);
    ~RDLoadOrderNode();
    int count;
    int *loadOrder;
};

class RDRackNameNode : public ANode
{
public:
    char rackName[32];
};

class RDPylonNode : public ANode
{
public:
    RDPylonNode();
    ~RDPylonNode();
    char mnemonic[12];
    int pylonCT; // could be 0
    int flags; // RDF_
    AList rackNameList;
};


class RDHardpointNode : public ANode
{
public:
    ~RDHardpointNode();
    int groupId;
    AList pylonList;
};

/*************************************/

RDRackNode::RDRackNode()
{
    rackName[0] = 0;
    mnemonic[0] = 0;

    rackCT = 0;
    stations = 0;
    swdCount = 0;
    swd = 0;
    wIdCount = 0;
    wId = 0;
    any = 0;
    wClassCount = 0;
    wClass = 0;
    flags = RDF_EMERGENCY_JETT_RACK   bitor RDF_SELECTIVE_JETT_RACK  |
            RDF_EMERGENCY_JETT_WEAPON bitor RDF_SELECTIVE_JETT_WEAPON;
}


RDRackNode::~RDRackNode()
{
    if (swd)
        free(swd);

    if (wId)
        free(wId);

    RDLoadOrderNode *ron;

    while (ron = (RDLoadOrderNode*)loadOrder.RemHead())
    {
        delete ron;
    }
}

RDLoadOrderNode::RDLoadOrderNode(int Count)
{
    count = Count;
    loadOrder = (int *)malloc(sizeof(int) * count);
}

RDLoadOrderNode::~RDLoadOrderNode()
{
    if (loadOrder)
        free(loadOrder);
}


/*-------------------------------------*/

RDPylonNode::RDPylonNode()
{
    mnemonic[0] = 0;
    pylonCT = 0;
    flags = 0;
}

RDPylonNode::~RDPylonNode()
{
    RDRackNameNode *rnn;

    while (rnn = (RDRackNameNode *)rackNameList.RemHead())
    {
        delete rnn;
    }
}

/*-------------------------------------*/

RDHardpointNode::~RDHardpointNode()
{
    RDPylonNode *pn;

    while (pn = (RDPylonNode *)pylonList.RemHead())
    {
        delete pn;
    }
}

/*************************************/

AList RDRackList;
AList RDHardpointList;

void RDLoadRackData(void)
{
    FILE *fp;
    char buffer[1024];
    RDHardpointNode *hpn = 0;
    RDPylonNode *pn = 0;
    RDRackNode *rn = 0;

    RDUnloadRackData(); // just incase

    if ((fp = OpenCampFile("BMSRack", "dat", "rt")) == NULL)
    {
        sprintf(buffer, "%s\\%s", FalconObjectDataDir, "BMSRack.dat");

        if ((fp = fopen(buffer, "rt")) == NULL)
            return;
    }

    while (fgets(buffer, sizeof buffer, fp))
    {
        char *com, *arg;

        if (buffer[0] == '#' or buffer[0] == ';' or buffer[0] == '\n')
            continue;

        com = strtok(buffer, " \t\n");
        arg = strtok(0, "\n\0");


        /* kludge so that arg is the current string being parsed */
        SetTokenString(arg);

        if ( not com)
            continue;

#define On(s) if(stricmp(com,s)==0)

        On("definerack")
        {
            char *n = TokenStr(0);

            if (n)
            {
                if (rn = new RDRackNode)
                {
                    RDRackList.AddTail(rn);
                    strncpy(rn->rackName, n, 32);
                    rn->any = 0;
                    //strncpy(rn->mnemonic,TokenStr("-------"),11);
                }
            }
        }

        if (rn)
        {
            On("rackct")
            {
                rn->rackCT = TokenI(0);
            }

            On("rackstations")
            {
                rn->stations = TokenI(0);
            }

            On("rackjettmodes")
            {
                int i;
                rn->flags and_eq compl (RDF_EMERGENCY_JETT_RACK bitor RDF_SELECTIVE_JETT_RACK); // clear flags
                char *enums[] = {"emergency", "selective", 0};

                while (-1 not_eq (i = TokenEnum(enums, -1)))
                {
                    switch (i)
                    {
                        case 0:
                            rn->flags or_eq RDF_EMERGENCY_JETT_RACK;
                            break;

                        case 1:
                            rn->flags or_eq RDF_SELECTIVE_JETT_RACK;
                            break;
                    }
                }
            }

            On("weapjettmodes")
            {
                int i;
                rn->flags and_eq compl (RDF_EMERGENCY_JETT_WEAPON bitor RDF_SELECTIVE_JETT_WEAPON); // clear flags
                char *enums[] = {"emergency", "selective", 0};

                while (-1 not_eq (i = TokenEnum(enums, -1)))
                {
                    switch (i)
                    {
                        case 0:
                            rn->flags or_eq RDF_EMERGENCY_JETT_WEAPON;
                            break;

                        case 1:
                            rn->flags or_eq RDF_SELECTIVE_JETT_WEAPON;
                            break;
                    }
                }
            }


            On("addswd")
            {
                int l = 0;
                int i[100];
                int ok = 1;

                while (ok and l < 100)
                {
                    i[l] = TokenI(-1);

                    if (i[l] == -1)
                        ok = 0;

                    l++;
                }

                rn->swd = (int *)malloc(sizeof(int) * l);
                rn->swdCount = l;

                for (l = 0; l < rn->swdCount; l++)
                {
                    rn->swd[l] = i[l];
                }
            }

            On("addwid")
            {
                int l = 0;
                int i[100];
                int ok = 1;

                while (ok and l < 100)
                {
                    i[l] = TokenI(-1);

                    if (i[l] == -1)
                        ok = 0;

                    l++;
                }

                rn->wId = (int *)malloc(sizeof(int) * l);
                rn->wIdCount = l;

                for (l = 0; l < rn->wIdCount; l++)
                {
                    rn->wId[l] = i[l];
                }
            }

            On("racksmsname")
            {
                strncpy(rn->mnemonic, TokenStr("-------"), 11);
            }

            On("addloadorder")
            {
                int l = 0;
                int i[100];
                int ok = 1;

                while (ok)
                {
                    i[l] = TokenI(-1);

                    if (i[l] == -1)
                        ok = 0;
                    else
                        l++;
                }

                RDLoadOrderNode *lon = new RDLoadOrderNode(l);

                if (lon)
                {
                    for (l = 0; i[l] not_eq -1; l++)
                    {
                        lon->loadOrder[l] = i[l];
                    }

                    rn->loadOrder.AddTail((ANode *)lon);
                }
            }


            On("addwclass")
            {
                char *enums[] = { "aim", "rocket", "bomb", "gun", "ecm", "tank", "agm", "harm", "sam", "gbu", "camera", 0 };
                int l = 0;
                int i[100];
                int ok = 1;

                while (ok and l < 100)
                {
                    i[l] = TokenEnum(enums, -1);

                    if (i[l] == -1)
                        ok = 0;

                    l++;
                }

                rn->wClass = (int *)malloc(sizeof(int) * l);
                rn->wClassCount = l;

                for (l = 0; l < rn->wClassCount; l++)
                {
                    rn->wClass[l] = i[l];
                }
            }

            On("addany")
            {
                rn->any = 1;
            }
        }

        On("definegroup")
        {
            if (hpn = new RDHardpointNode)
            {
                hpn->groupId = TokenI(0);
                RDHardpointList.AddTail(hpn);
                pn = 0;
            }
        }

        if (hpn)
        {
            On("addpylon")
            {
                if (pn = new RDPylonNode)
                {
                    //strncpy(pn->mnemonic,TokenStr("-------"),11);
                    hpn->pylonList.AddTail(pn);
                }
            }

            if (pn)
            {
                On("addrack")
                {
                    char *n;

                    while (n = TokenStr(0))
                    {
                        RDRackNameNode *rnn;

                        if (rnn = new RDRackNameNode)
                        {
                            strncpy(rnn->rackName, n, 32);
                            pn->rackNameList.AddTail(rnn);
                        }
                    }
                }

                On("pylonct")
                {
                    pn->pylonCT = TokenI(0);
                }

                On("pylonsmsname")
                {
                    strncpy(pn->mnemonic, TokenStr("-------"), 12);
                }

                On("pylonjettmodes")
                {
                    int i;
                    pn->flags and_eq compl (RDF_EMERGENCY_JETT_PYLON bitor RDF_SELECTIVE_JETT_PYLON); // clear flags
                    char *enums[] = {"emergency", "selective", 0};

                    while (-1 not_eq (i = TokenEnum(enums, -1)))
                    {
                        switch (i)
                        {
                            case 0:
                                pn->flags or_eq RDF_EMERGENCY_JETT_PYLON;
                                break;

                            case 1:
                                pn->flags or_eq RDF_SELECTIVE_JETT_PYLON;
                                break;
                        }
                    }
                }
            }
        }
    }

    fclose(fp);
}

void RDUnloadRackData(void)
{
    RDHardpointNode *hpn;

    while (hpn = (RDHardpointNode *)RDHardpointList.RemHead())
    {
        delete hpn;
    }

}

int RDFindBestRackWClass(int GroupId, int wClass, int WeaponCount, struct RDRackData *rd);


int RDFindBestRack(int GroupId, int WeaponId, int WeaponCount, struct RDRackData *rd)
{
    if (WeaponId)
    {
        if (RDFindBestRackWID(GroupId, WeaponId, WeaponCount, rd))
            return 1;

        int wclass = SimWeaponDataTable[Falcon4ClassTable[WeaponDataTable[WeaponId].Index].vehicleDataIndex].weaponClass;

        if (RDFindBestRackWClass(GroupId, wclass, WeaponCount, rd))
            return 1;

        if (RDFindBestRackSWD(GroupId, WeaponDataTable[WeaponId].SimweapIndex, WeaponCount, rd))
            return 1;
    }

    return 0;
}

void RDCopyRackData(int count, RDPylonNode *pn, RDRackNode *rn, RDRackData *rd)
{
    rd->rackCT   = rn->rackCT;
    rd->pylonCT   = pn->pylonCT;
    rd->rackStations  = rn->stations;
    rd->flags         = pn->flags bitor rn->flags bitor RDF_BMSDEFINITION;
    rd->pylonmnemonic = pn->mnemonic;
    rd->rackmnemonic  = rn->mnemonic;
    rd->count         = count;

    RDLoadOrderNode *lon;

    lon = (RDLoadOrderNode *)rn->loadOrder.GetHead();

    while (lon)
    {
        if (count <= lon->count)
        {
            // loan the array to the HP
            rd->loadOrder = lon->loadOrder;
            lon = NULL;
        }
        else
            lon = (RDLoadOrderNode *)lon->GetSucc();
    }

}


int RDFindBestRackWID(int GroupId, int WeaponId, int WeaponCount, struct RDRackData *rd)
{
    RDHardpointNode *hpn;
    RDPylonNode *pn;
    RDRackNameNode *rnn;
    RDRackNode *rn;


    hpn = (RDHardpointNode *)RDHardpointList.GetHead();

    while (hpn)
    {
        if (hpn->groupId == GroupId)
        {
            pn = (RDPylonNode *)hpn->pylonList.GetHead();

            while (pn)
            {
                rnn = (RDRackNameNode *)pn->rackNameList.GetHead();

                while (rnn)
                {
                    rn = (RDRackNode *)RDRackList.GetHead();

                    while (rn)
                    {
                        if (stricmp(rn->rackName, rnn->rackName) == 0 and 
                            rn->stations >= WeaponCount)
                        {
                            int l;

                            for (l = 0; l < rn->wIdCount; l++)
                            {
                                if (rn->wId[l] == WeaponId)
                                {
                                    // a match
                                    RDCopyRackData(WeaponCount, pn, rn, rd);
                                    return 1;
                                }
                            }

                        }

                        rn = (RDRackNode *)rn->GetSucc();
                    }

                    rnn = (RDRackNameNode *)rnn->GetSucc();
                }

                pn = (RDPylonNode *)pn->GetSucc();
            }
        }

        hpn = (RDHardpointNode *)hpn->GetSucc();
    }

    rd->pylonCT = rd->rackCT = 0;
    return 0;
}

int RDFindBestRackWClass(int GroupId, int wClass, int WeaponCount, struct RDRackData *rd)
{
    RDHardpointNode *hpn;
    RDPylonNode *pn;
    RDRackNameNode *rnn;
    RDRackNode *rn;

    hpn = (RDHardpointNode *)RDHardpointList.GetHead();

    while (hpn)
    {
        if (hpn->groupId == GroupId)
        {
            pn = (RDPylonNode *)hpn->pylonList.GetHead();

            while (pn)
            {
                rnn = (RDRackNameNode *)pn->rackNameList.GetHead();

                while (rnn)
                {
                    rn = (RDRackNode *)RDRackList.GetHead();

                    while (rn)
                    {
                        if (stricmp(rn->rackName, rnn->rackName) == 0 and 
                            rn->stations >= WeaponCount)
                        {
                            int l;

                            for (l = 0; l < rn->wClassCount; l++)
                            {
                                if (rn->wClass[l] == wClass)
                                {
                                    // a match
                                    RDCopyRackData(WeaponCount, pn, rn, rd);
                                    return 1;
                                }
                            }

                        }

                        rn = (RDRackNode *)rn->GetSucc();
                    }

                    rnn = (RDRackNameNode *)rnn->GetSucc();
                }

                pn = (RDPylonNode *)pn->GetSucc();
            }
        }

        hpn = (RDHardpointNode *)hpn->GetSucc();

    }

    rd->pylonCT = rd->rackCT = 0;
    return 0;
}




int RDFindBestRackSWD(int GroupId, int SWD, int WeaponCount, struct RDRackData *rd)
{
    RDHardpointNode *hpn;
    RDPylonNode *pn;
    RDRackNameNode *rnn;
    RDRackNode *rn;

    hpn = (RDHardpointNode *)RDHardpointList.GetHead();

    while (hpn)
    {
        if (hpn->groupId == GroupId)
        {
            pn = (RDPylonNode *)hpn->pylonList.GetHead();

            while (pn)
            {
                rnn = (RDRackNameNode *)pn->rackNameList.GetHead();

                while (rnn)
                {
                    rn = (RDRackNode *)RDRackList.GetHead();

                    while (rn)
                    {
                        if (stricmp(rn->rackName, rnn->rackName) == 0 and 
                            rn->stations >= WeaponCount)
                        {
                            int l;

                            for (l = 0; l < rn->swdCount; l++)
                            {
                                if (rn->any)
                                {
                                    RDCopyRackData(WeaponCount, pn, rn, rd);
                                    return 1;
                                }

                                if (rn->swd[l] == SWD)
                                {
                                    // a match
                                    RDCopyRackData(WeaponCount, pn, rn, rd);
                                    return 1;
                                }
                            }

                        }

                        rn = (RDRackNode *)rn->GetSucc();
                    }

                    rnn = (RDRackNameNode *)rnn->GetSucc();
                }

                pn = (RDPylonNode *)pn->GetSucc();
            }
        }

        hpn = (RDHardpointNode *)hpn->GetSucc();

    }

    rd->pylonCT = rd->rackCT = 0;
    return 0;
}


