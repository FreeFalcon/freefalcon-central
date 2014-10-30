#ifndef OBJECTIVE_H
#define OBJECTIVE_H

#include "CmpGlobl.h"
#include "FalcLib.h"
#include "CampList.h"
#include "CampBase.h"
#include "SIM/include/atcBrain.h"
#include "CmpRadar.h"
#include "MsgInc/ObjectiveMsg.h"

// =======================
// Forward declarations
// =======================


//sfr: we dont need these hacks anymore
//class RadarRangeClass;
//class ATCBrain;

// =======================
// Transmitable flags
// =======================

#define O_FRONTLINE 0x1
#define O_SECONDLINE 0x2
#define O_THIRDLINE 0x4
#define O_B3 0x8
#define O_JAMMED 0x10
#define O_BEACH 0x20
#define O_B1 0x40
#define O_B2 0x80
#define O_MANUAL_SET 0x100
#define O_MOUNTAIN_SITE 0x200
#define O_SAM_SITE 0x400
#define O_ARTILLERY_SITE 0x800
#define O_AMBUSHCAP_SITE 0x1000
#define O_BORDER_SITE 0x2000
#define O_COMMANDO_SITE 0x4000
#define O_FLAT_SITE 0x8000
#define O_RADAR_SITE 0x10000
#define O_NEED_REPAIR 0x20000
#define O_EMPTY1 0x40000
#define O_EMPTY2 0x80000
#define O_ABANDONED 0x100000
// 2002-02-13 ADDED BY MN for Sylvain's new Identify
#define O_HAS_NCTR 0x200000
#define O_IS_GCI 0x400000

// =======================
// Random externals
// =======================

extern Objective FindObjective(VU_ID id);

// =======================
// Campaign Objectives ADT
// =======================

struct CampObjectiveTransmitDataType
{
    CampaignTime last_repair; // Last time this objective got something repaired
    short aiscore; // Used for scoring junque
    ulong obj_flags; // Transmitable flags
    uchar supply; // Amount of supply going through here
    uchar fuel; // Amount of fuel going through here
    uchar losses; // Amount of supply/fuel losses (in percentage)
    uchar status; // % operational
    uchar priority; // Target's general priority
    uchar* fstatus; // Array of feature statuses (was [((FEATURES_PER_OBJ*2)+7)/8])
};

struct CampObjectiveStaticDataType
{
    short nameid; // Index into name table
    short local_data; // Local AI data dump
    VU_ID parent; // ID of parent SO or PO
    Control first_owner; // Origional objective owner
    uchar links; // Number of links
    RadarRangeClass* radar_data; // Data on what a radar stationed here can see
    ObjClassDataType* class_data; // Pointer to class data
};

struct CampObjectiveLinkDataType
{
    uchar costs[MOVEMENT_TYPES]; // Cost to go here, depending on movement type
    VU_ID id;
};

class ObjectiveClass : public CampBaseClass
{
#ifdef USE_SH_POOLS
public:
    // Overload new/delete to use a SmartHeap fixed size pool
    void *operator new(size_t size)
    {
        ShiAssert(size == sizeof(ObjectiveClass));
        return MemAllocFS(pool);
    };
    void operator delete(void *mem)
    {
        if (mem) MemFreeFS(mem);
    };
    static void InitializeStorage()
    {
        pool = MemPoolInitFS(sizeof(ObjectiveClass), 2500, 0);
    };
    static void ReleaseStorage()
    {
        MemPoolFree(pool);
    };
    static MEM_POOL pool;
#endif
private:
    CampObjectiveTransmitDataType obj_data;
    int dirty_objective;

public:
    CampObjectiveStaticDataType static_data;
    CampObjectiveLinkDataType* link_data; // The actual link data (was [OBJ_MAX_NEIGHBORS])
    ATCBrain* brain;
public:
    // access functions
    ulong GetObjFlags(void);
    void ClearObjFlags(ulong flags)
    {
        obj_data.obj_flags and_eq compl (flags);
    }
    void SetObjFlags(ulong flags)
    {
        obj_data.obj_flags or_eq (flags);
    }

    // constructors
    ObjectiveClass(int type);
    ObjectiveClass(VU_BYTE **stream, long *rem);
    virtual ~ObjectiveClass();
    virtual int SaveSize(void);
    virtual int SaveSize(int toDisk);
    virtual int Save(VU_BYTE **stream);
    virtual int Save(VU_BYTE **stream, int toDisk);
    void UpdateFromData(VU_BYTE **stream, long *rem);

    // pure virtual implementation
    virtual float GetVt() const
    {
        return 0;
    }
    virtual float GetKias() const
    {
        return 0;
    }


    // event Handlers
    virtual VU_ERRCODE Handle(VuFullUpdateEvent *event);

    // Required pure virtuals handled by objective.h
    virtual void SendDeaggregateData(VuTargetEntity *);
    virtual int RecordCurrentState(FalconSessionEntity*, int);
    virtual int Deaggregate(FalconSessionEntity* session);
    virtual int Reaggregate(FalconSessionEntity* session);
    virtual int TransferOwnership(FalconSessionEntity* session);
    virtual int Wake(void);
    virtual int Sleep(void);
    virtual void InsertInSimLists(float cameraX, float cameraY);
    virtual void RemoveFromSimLists(void);
    //sfr: changed this function prototype
    //virtual void DeaggregateFromData (int size, uchar* data);
    virtual void DeaggregateFromData(VU_BYTE* data, long rem);
    //virtual void ReaggregateFromData (int size, uchar* data);
    virtual void ReaggregateFromData(VU_BYTE* data, long rem);
    //virtual void TransferOwnershipFromData (int size, uchar* data);
    virtual void TransferOwnershipFromData(VU_BYTE *data, long rem);
    virtual MoveType GetMovementType(void)
    {
        return NoMove;
    }
    virtual int ApplyDamage(FalconCampWeaponsFire *cwfm, uchar);
    virtual int ApplyDamage(DamType d, int *str, int where, short flags);
    virtual int DecodeDamageData(uchar *data, Unit shooter, FalconDeathMessage *dtm);
    virtual uchar* GetDamageModifiers(void);
    virtual _TCHAR* GetName(_TCHAR* buffer, int size, int object);
    virtual _TCHAR* GetFullName(_TCHAR* buffer, int size, int object);
    virtual int GetHitChance(int mt, int range);
    virtual int GetAproxHitChance(int mt, int range);
    virtual int GetCombatStrength(int mt, int range);
    virtual int GetAproxCombatStrength(int mt, int range);
    virtual int GetWeaponRange(int mt, FalconEntity *target = NULL);  // 2008-03-08 ADDED SECOND DEFAULT PARM
    virtual int GetAproxWeaponRange(int mt);
    virtual int GetDetectionRange(int mt); // Takes into account emitter status
    virtual int GetElectronicDetectionRange(int mt); // Max Electronic detection range, even if turned off
    virtual int CanDetect(FalconEntity* ent); // Nonzero if this entity can see ent
    virtual int OnGround(void)
    {
        return TRUE;
    }
    virtual int GetRadarMode(void);
    virtual int GetRadarType(void);

    // These are only really relevant for sam/airdefense/radar entities
    virtual int GetNumberOfArcs(void);
    virtual float GetArcRatio(int anum);
    virtual float GetArcRange(int anum);
    virtual void GetArcAngle(int anum, float* a1, float *a2);
    int SiteCanDetect(FalconEntity* ent);
    float GetSiteRange(FalconEntity* ent);

    // core functions
    void SendObjMessage(VU_ID from, short mes, short d1, short d2, short d3);
    void DisposeObjective(void);
    void DamageObjective(int loss);
    void AddObjectiveNeighbor(Objective o, uchar c[MOVEMENT_TYPES]);
    void RemoveObjectiveNeighbor(int n);
    void SetNeighborCosts(int num, uchar c[MOVEMENT_TYPES]);
    virtual int IsObjective(void)
    {
        return TRUE;
    }
    int IsFrontline(void)
    {
        return (int)(O_FRONTLINE bitand obj_data.obj_flags);
    }
    int IsSecondline(void)
    {
        return (int)(O_SECONDLINE bitand obj_data.obj_flags);
    }
    int IsThirdline(void)
    {
        return (int)(O_THIRDLINE bitand obj_data.obj_flags);
    }
    int IsNearfront(void)
    {
        return (int)((O_THIRDLINE bitor O_SECONDLINE bitor O_FRONTLINE) bitand obj_data.obj_flags);
    }
    int IsBeach(void)
    {
        return (int)(O_BEACH bitand obj_data.obj_flags);
    }
    int IsPrimary(void);
    int IsSecondary(void);
    int IsSupplySource(void);
    int IsGCI(void)
    {
        return (int)(O_IS_GCI bitand obj_data.obj_flags);    // 2002-02-13 ADDED BY S.G.
    }
    int HasNCTR(void)
    {
        return (int)(O_HAS_NCTR bitand obj_data.obj_flags);    // 2002-02-13 ADDED BY S.G.
    }
    int HasRadarRanges(void);
    void UpdateObjectiveLists(void);
    void ResetLinks(void);
    void Dump(void);
    void Repair(void);

    // Flag setting stuff
    void SetManual(int s);
    int ManualSet(void)
    {
        return obj_data.obj_flags bitand O_MANUAL_SET;
    }
    void SetJammed(int j);
    int Jammed(void)
    {
        return obj_data.obj_flags bitand O_JAMMED;
    }
    void SetSamSite(int j);
    int SamSite(void)
    {
        return obj_data.obj_flags bitand O_SAM_SITE;
    }
    void SetArtillerySite(int j);
    int ArtillerySite(void)
    {
        return obj_data.obj_flags bitand O_ARTILLERY_SITE;
    }
    void SetAmbushCAPSite(int j);
    int AmbushCAPSite(void)
    {
        return obj_data.obj_flags bitand O_AMBUSHCAP_SITE;
    }
    void SetBorderSite(int j);
    int BorderSite(void)
    {
        return obj_data.obj_flags bitand O_BORDER_SITE;
    }
    void SetMountainSite(int j);
    int MountainSite(void)
    {
        return obj_data.obj_flags bitand O_MOUNTAIN_SITE;
    }
    void SetCommandoSite(int j);
    int CommandoSite(void)
    {
        return obj_data.obj_flags bitand O_COMMANDO_SITE;
    }
    void SetFlatSite(int j);
    int FlatSite(void)
    {
        return obj_data.obj_flags bitand O_FLAT_SITE;
    }
    void SetRadarSite(int j);
    int RadarSite(void)
    {
        return obj_data.obj_flags bitand O_RADAR_SITE;
    }
    void SetAbandoned(int t);
    int Abandoned(void)
    {
        return obj_data.obj_flags bitand O_ABANDONED;
    }
    void SetNeedRepair(int t);
    int NeedRepair(void)
    {
        return obj_data.obj_flags bitand O_NEED_REPAIR;
    }

    // Dirty Functions
    void MakeObjectiveDirty(Dirty_Objective bits, Dirtyness score);
    void WriteDirty(unsigned char **stream);
    //sfr: changed prototype
    //void ReadDirty (unsigned char **stream);
    void ReadDirty(unsigned char **stream, long *rem);

    // Objective data stuff
    virtual void SetOwner(Control c)
    {
        CampBaseClass::SetOwner(c);
        SetDelta(1);
    }
    void SetObjectiveOldown(Control c)
    {
        static_data.first_owner = c;
    }
    void SetObjectiveParent(VU_ID p)
    {
        static_data.parent = p;
    }
    void SetObjectiveNameID(short n)
    {
        static_data.nameid = n;
    }
    void SetObjectiveName(char* name);
    void SetObjectivePriority(PriorityLevel p)
    {
        obj_data.priority = p;
    }
    void SetObjectiveScore(short score)
    {
        obj_data.aiscore = score;
    }
    void SetObjectiveRepairTime(CampaignTime t)
    {
        obj_data.last_repair = t;
    }
    // JB 000811
    // Set the last repair time to now if some damage has been taken
    //void SetObjectiveStatus (uchar s) { obj_data.status = s; MakeObjectiveDirty (DIRTY_STATUS, SEND_NOW); }
    void SetObjectiveStatus(uchar s)
    {
        if (obj_data.status > s) obj_data.last_repair = Camp_GetCurrentTime();

        obj_data.status = s;
        MakeObjectiveDirty(DIRTY_STATUS, DDP[180].priority);
    }
    //void SetObjectiveStatus (uchar s) { if (obj_data.status > s) obj_data.last_repair = Camp_GetCurrentTime(); obj_data.status = s; MakeObjectiveDirty (DIRTY_STATUS, SEND_NOW); }
    // JB 000811
    void SetObjectiveSupply(uchar s)
    {
        obj_data.supply = s;
    }
    void SetObjectiveFuel(uchar f)
    {
        obj_data.fuel = f;
    }
    void SetObjectiveSupplyLosses(uchar l)
    {
        obj_data.losses = l;
    }
    void SetObjectiveType(ObjectiveType t);
    void SetObjectiveSType(uchar s);
    void SetObjectiveClass(int dindex);
    void SetFeatureStatus(int f, int n);
    void SetFeatureStatus(int f, int n, int from);

    VU_ID GetNeighborId(int n)
    {
        return link_data[n].id;
    }
    Objective GetNeighbor(int n);
    float GetNeighborCost(int n, MoveType t)
    {
        return link_data[n].costs[t];
    }
    Control GetObjectiveOldown(void)
    {
        return static_data.first_owner;
    }
    Objective GetObjectiveParent(void)
    {
        return FindObjective(static_data.parent);
    }
    Objective GetObjectiveSecondary(void);
    Objective GetObjectivePrimary(void);
    VU_ID GetObjectiveParentID(void)
    {
        return static_data.parent;
    }
    int GetObjectiveNameID(void)
    {
        return static_data.nameid;
    }
    int NumLinks(void)
    {
        return static_data.links;
    }
    short GetObjectivePriority(void)
    {
        return obj_data.priority;
    }
    uchar GetObjectiveStatus(void)
    {
        return obj_data.status;
    }
    int GetObjectiveScore(void)
    {
        return obj_data.aiscore;
    }
    CampaignTime GetObjectiveRepairTime(void)
    {
        return obj_data.last_repair;
    }
    short GetObjectiveSupply(void)
    {
        return obj_data.supply;
    }
    short GetObjectiveFuel(void)
    {
        return obj_data.fuel;
    }
    short GetObjectiveSupplyLosses(void)
    {
        return obj_data.losses;
    }
    short GetObjectiveDataRate(void);
    short GetAdjustedDataRate(void);
    short GetTotalFeatures(void)
    {
        return static_data.class_data->Features;
    }
    int GetFeatureStatus(int f);
    int GetFeatureValue(int f);
    int GetFeatureRepairTime(int f);
    int GetFeatureID(int f);
    int GetFeatureOffset(int f, float* x, float* y, float* z);
    ObjClassDataType* GetObjectiveClassData(void);
    char* GetObjectiveClassName(void);
    // int RoE (VuEntity* e, int type);

    uchar GetExpectedStatus(int hours);
    int GetRepairTime(int status);
    uchar GetBestTarget();
    void ResetObjectiveStatus(void);
    void RepairFeature(int f);
    void RecalculateParent(void);
};

// ================================
// Objective Externals
// ================================

// ================================
// Inline functions
// ================================

// ---------------------------------------
// External Function Declarations
// ---------------------------------------

extern Objective NewObjective(void);

//sfr: added red to both
extern Objective NewObjective(short tid, VU_BYTE **stream, long *rem);

extern Objective NewObjective(short tid, VU_BYTE **stream, long *rem, int fromDisk);

extern int LoadBaseObjectives(char* scenario);

extern int LoadObjectiveDeltas(char* savefile);

extern void SaveBaseObjectives(char* scenario);

extern void SaveObjectiveDeltas(char* savefile);

extern Objective GetObjectiveByID(int ID);

extern int BestRepairFeature(Objective o, int *hours);

extern int BestTargetFeature(Objective o, uchar targeted[]);

extern void RepairObjectives(void);

extern DamageDataType GetDamageType(Objective o, int f);

extern F4PFList GetChildObjectives(Objective o, int maxdist, int flags);

extern Objective GetFirstObjective(F4LIt l);

extern Objective GetNextObjective(F4LIt l);

extern Objective GetFirstObjective(VuGridIterator* l);

extern Objective GetNextObjective(VuGridIterator* l);

extern void CaptureObjective(Objective co, Control who, Unit u = NULL);

extern int EncodeObjectiveDeltas(VU_BYTE **stream, FalconSessionEntity *owner);

//sfr: added rem
extern int DecodeObjectiveDeltas(VU_BYTE **stream, long *rem, FalconSessionEntity *owner);

#endif
