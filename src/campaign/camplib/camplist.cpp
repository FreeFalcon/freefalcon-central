/******************************************************************************
/*
/* CdbList managing routines
/*
/*****************************************************************************/

#include "cmpglobl.h"
#include "F4Vu.h"
#include "objectiv.h"
#include "listadt.h"
#include "CampList.h"
#include "gtmobj.h"
#include "team.h"
#include "falcgame.h"
#include "CampBase.h"
#include "Campaign.h"
#include "Find.h"
#include "AIInput.h"
#include "CmpClass.h"
#include "CampMap.h"
#include "classtbl.h"
#include "FalcSess.h"

using namespace std;

// ==================================
// externals
// ==================================

extern void MarkObjectives(void);
//TJL 11/14/03 Variable moved here from the RebuildFrontList function to allow lastRequest to stay set
static CampaignTime lastRequest = 0;

// ==================================
// Unit specific filters
// ==================================

UnitFilter::UnitFilter(uchar p, uchar r, ushort h, uchar a)
{
    parent = p;
    real = r;
    host = h;
    inactive = a;
}

VU_BOOL UnitFilter::Test(VuEntity *e)
{
    if ( not (e->EntityType())->classInfo_[VU_DOMAIN] or (e->EntityType())->classInfo_[VU_CLASS] not_eq CLASS_UNIT)
        return FALSE;

    if (parent and not ((Unit)e)->Parent())
        return FALSE;

    if (real and not Real((e->EntityType())->classInfo_[VU_TYPE]))
        return FALSE;

    if (host and not e->IsLocal())
        return FALSE;

    if ( not inactive and ((Unit)e)->Inactive())
        return FALSE;
    else if (inactive and not ((Unit)e)->Inactive())
        return FALSE;

    return TRUE;
}

VU_BOOL UnitFilter::RemoveTest(VuEntity *e)
{
    if ( not (e->EntityType())->classInfo_[VU_DOMAIN] or (e->EntityType())->classInfo_[VU_CLASS] not_eq CLASS_UNIT)
        return FALSE;

    if (parent and not ((Unit)e)->Parent())
        return FALSE;

    if (real and not Real((e->EntityType())->classInfo_[VU_TYPE]))
        return FALSE;

    if (host and not e->IsLocal())
        return FALSE;

    if ( not inactive and ((Unit)e)->Inactive())
        return FALSE;
    else if (inactive and not ((Unit)e)->Inactive())
        return FALSE;

    return TRUE;
}

AirUnitFilter::AirUnitFilter(uchar p, uchar r, ushort h)
{
    parent = p;
    real = r;
    host = h;
}

VU_BOOL AirUnitFilter::Test(VuEntity *e)
{
    if ( not (e->EntityType())->classInfo_[VU_DOMAIN] or (e->EntityType())->classInfo_[VU_CLASS] not_eq CLASS_UNIT)
        return FALSE;

    if (((Unit)e)->GetDomain() not_eq DOMAIN_AIR)
        return FALSE;

    if (parent and not ((Unit)e)->Parent())
        return FALSE;

    if (real and not Real((e->EntityType())->classInfo_[VU_TYPE]))
        return FALSE;

    if (host and not e->IsLocal())
        return FALSE;

    if (((Unit)e)->Inactive())
        return FALSE;

    return TRUE;
}

VU_BOOL AirUnitFilter::RemoveTest(VuEntity *e)
{
    if ( not (e->EntityType())->classInfo_[VU_DOMAIN] or (e->EntityType())->classInfo_[VU_CLASS] not_eq CLASS_UNIT)
        return FALSE;

    if (((Unit)e)->GetDomain() not_eq DOMAIN_AIR)
        return FALSE;

    if (parent and not ((Unit)e)->Parent())
        return FALSE;

    if (real and not Real((e->EntityType())->classInfo_[VU_TYPE]))
        return FALSE;

    if (host and not e->IsLocal())
        return FALSE;

    if (((Unit)e)->Inactive())
        return FALSE;

    return TRUE;
}

GroundUnitFilter::GroundUnitFilter(uchar p, uchar r, ushort h)
{
    parent = p;
    real = r;
    host = h;
}

VU_BOOL GroundUnitFilter::Test(VuEntity *e)
{
    if ( not (e->EntityType())->classInfo_[VU_DOMAIN] or (e->EntityType())->classInfo_[VU_CLASS] not_eq CLASS_UNIT)
        return FALSE;

    if (((Unit)e)->GetDomain() not_eq DOMAIN_LAND)
        return FALSE;

    if (parent and not ((Unit)e)->Parent())
        return FALSE;

    if (real and not Real((e->EntityType())->classInfo_[VU_TYPE]))
        return FALSE;

    if (host and not e->IsLocal())
        return FALSE;

    if (((Unit)e)->Inactive())
        return FALSE;

    return TRUE;
}

VU_BOOL GroundUnitFilter::RemoveTest(VuEntity *e)
{
    if ( not (e->EntityType())->classInfo_[VU_DOMAIN] or (e->EntityType())->classInfo_[VU_CLASS] not_eq CLASS_UNIT)
        return FALSE;

    if (((Unit)e)->GetDomain() not_eq DOMAIN_LAND)
        return FALSE;

    if (parent and not ((Unit)e)->Parent())
        return FALSE;

    if (real and not Real((e->EntityType())->classInfo_[VU_TYPE]))
        return FALSE;

    if (host and not e->IsLocal())
        return FALSE;

    if (((Unit)e)->Inactive())
        return FALSE;

    return TRUE;
}

NavalUnitFilter::NavalUnitFilter(uchar p, uchar r, ushort h)
{
    parent = p;
    real = r;
    host = h;
}

VU_BOOL NavalUnitFilter::Test(VuEntity *e)
{
    if ( not (e->EntityType())->classInfo_[VU_DOMAIN] or (e->EntityType())->classInfo_[VU_CLASS] not_eq CLASS_UNIT)
        return FALSE;

    if (((Unit)e)->GetDomain() not_eq DOMAIN_SEA)
        return FALSE;

    if (parent and not ((Unit)e)->Parent())
        return FALSE;

    if (real and not Real((e->EntityType())->classInfo_[VU_TYPE]))
        return FALSE;

    if (host and not e->IsLocal())
        return FALSE;

    if (((Unit)e)->Inactive())
        return FALSE;

    return TRUE;
}

VU_BOOL NavalUnitFilter::RemoveTest(VuEntity *e)
{
    if ( not (e->EntityType())->classInfo_[VU_DOMAIN] or (e->EntityType())->classInfo_[VU_CLASS] not_eq CLASS_UNIT)
        return FALSE;

    if (((Unit)e)->GetDomain() not_eq DOMAIN_SEA)
        return FALSE;

    if (parent and not ((Unit)e)->Parent())
        return FALSE;

    if (real and not Real((e->EntityType())->classInfo_[VU_TYPE]))
        return FALSE;

    if (host and not e->IsLocal())
        return FALSE;

    if (((Unit)e)->Inactive())
        return FALSE;

    return TRUE;
}

UnitFilter AllUnitFilter(0, 0, 0, 0);
AirUnitFilter AllAirFilter(0, 0, 0);
GroundUnitFilter AllGroundFilter(0, 0, 0);
NavalUnitFilter AllNavalFilter(0, 0, 0);
UnitFilter AllParentFilter(TRUE, 0, 0, 0);
UnitFilter AllRealFilter(0, TRUE, 0, 0);
UnitFilter InactiveFilter(0, 0, 0, 1);
VuOpaqueFilter AllOpaqueFilter;

#if GRID_CORRECTION
#else
UnitProxFilter::UnitProxFilter(int r) : VuBiKeyFilter()
{
    // KCK NOTE: Using this max will cause errors, since we'll
    // have more percision than the sim coordinates we're deriving from
    // VU_KEY max = compl 0;
    VU_KEY max = 0xFFFF;
    step = (float)(max / (GRID_SIZE_FT * (Map_Max_Y + 1)));
    real = (uchar) r;
}

UnitProxFilter::UnitProxFilter(const UnitProxFilter *other, int r) : VuBiKeyFilter(other)
{
    // KCK NOTE: Using this max will cause errors, since we'll
    // have more percision than the sim coordinates we're deriving from
    // VU_KEY max = compl 0;
    VU_KEY max = 0xFFFF;
    step = (float)(max / (GRID_SIZE_FT * (Map_Max_Y + 1)));
    real = (uchar) r;
}
#endif


VU_BOOL UnitProxFilter::Test(VuEntity *ent)
{
    if ( not ent->EntityType()->classInfo_[VU_DOMAIN] or ent->EntityType()->classInfo_[VU_CLASS] not_eq CLASS_UNIT)
    {
        return FALSE;
    }

    if ( not Real((ent->EntityType())->classInfo_[VU_TYPE]))
    {
        return FALSE;
    }

    if (((Unit)ent)->Inactive())
    {
        return FALSE;
    }

    return TRUE;
}

VU_BOOL UnitProxFilter::RemoveTest(VuEntity *ent)
{
    if ( not ent->EntityType()->classInfo_[VU_DOMAIN] or ent->EntityType()->classInfo_[VU_CLASS] not_eq CLASS_UNIT)
    {
        return FALSE;
    }

    if ( not Real((ent->EntityType())->classInfo_[VU_TYPE]))
    {
        return FALSE;
    }

    return TRUE;
}

UnitProxFilter* AllUnitProxFilter = NULL;
UnitProxFilter* RealUnitProxFilter = NULL;

// ==============================
// Objective specific filters
// ==============================

// Standard Objective Filter
ObjFilter::ObjFilter(ushort h)
{
    host = h;
}

VU_BOOL ObjFilter::Test(VuEntity *e)
{
    if ( not (e->EntityType())->classInfo_[VU_DOMAIN] or (e->EntityType())->classInfo_[VU_CLASS] not_eq CLASS_OBJECTIVE)
        return FALSE;

    if (host and not e->IsLocal())
        return FALSE;

    return TRUE;
}

VU_BOOL ObjFilter::RemoveTest(VuEntity *e)
{
    if ( not (e->EntityType())->classInfo_[VU_DOMAIN] or (e->EntityType())->classInfo_[VU_CLASS] not_eq CLASS_OBJECTIVE)
        return FALSE;

    if (host and not e->IsLocal())
        return FALSE;

    return TRUE;
}

ObjFilter AllObjFilter(0);

// Proximity Objective Filter
#if GRID_CORRECTION
#else
ObjProxFilter::ObjProxFilter(void) : VuBiKeyFilter()
{
    // KCK NOTE: Using this max will cause errors, since we'll
    // have more percision than the sim coordinates we're deriving from
    // VU_KEY max = compl 0;
    VU_KEY max = 0xFFFF;
    xStep = (float)(max / (GRID_SIZE_FT * (Map_Max_Y + 1)));
    yStep = (float)(max / (GRID_SIZE_FT * (Map_Max_X + 1)));
}

ObjProxFilter::ObjProxFilter(const ObjProxFilter *other) : VuBiKeyFilter(other)
{
    // KCK NOTE: Using this max will cause errors, since we'll
    // have more percision than the sim coordinates we're deriving from
    // VU_KEY max = compl 0;
    VU_KEY max = 0xFFFF;
    xStep = (float)(max / (GRID_SIZE_FT * (Map_Max_Y + 1)));
    yStep = (float)(max / (GRID_SIZE_FT * (Map_Max_X + 1)));
}
#endif

VU_BOOL ObjProxFilter::Test(VuEntity *ent)
{
    if ((ent->EntityType())->classInfo_[VU_DOMAIN] and (ent->EntityType())->classInfo_[VU_CLASS] not_eq CLASS_OBJECTIVE)
    {
        return FALSE;
    }

    return TRUE;
}

VU_BOOL ObjProxFilter::RemoveTest(VuEntity *ent)
{
    if ((ent->EntityType())->classInfo_[VU_DOMAIN] and (ent->EntityType())->classInfo_[VU_CLASS] not_eq CLASS_OBJECTIVE)
    {
        return FALSE;
    }

    return TRUE;
}

ObjProxFilter* AllObjProxFilter = NULL;

// ==============================
// General Filters
// ==============================

VU_BOOL CampBaseFilter::Test(VuEntity *e)
{
    if ((e->EntityType())->classInfo_[VU_DOMAIN] and 
        ((e->EntityType())->classInfo_[VU_CLASS] == CLASS_UNIT or
         (e->EntityType())->classInfo_[VU_CLASS] == CLASS_OBJECTIVE))
        return TRUE;

    return FALSE;
}

VU_BOOL CampBaseFilter::RemoveTest(VuEntity *e)
{
    if ((e->EntityType())->classInfo_[VU_DOMAIN] and 
        ((e->EntityType())->classInfo_[VU_CLASS] == CLASS_UNIT or
         (e->EntityType())->classInfo_[VU_CLASS] == CLASS_OBJECTIVE))
        return TRUE;

    return FALSE;
}

CampBaseFilter CampFilter;

// ==============================
// Registered Collections
// ==============================

#if VU_ALL_FILTERED
VuLinkedList* AllUnitList = NULL; // All units
VuLinkedList* AllAirList = NULL; // All air units
VuLinkedList* AllParentList = NULL; // All parent units
VuLinkedList* AllRealList = NULL; // All real units
VuLinkedList* AllObjList = NULL; // All objectives
VuLinkedList* AllCampList = NULL; // All campaign entities
VuLinkedList* InactiveList = NULL; // Inactive units (reinforcements)
#else
VuFilteredList* AllUnitList = NULL; // All units
VuFilteredList* AllAirList = NULL; // All air units
VuFilteredList* AllParentList = NULL; // All parent units
VuFilteredList* AllRealList = NULL; // All real units
VuFilteredList* AllObjList = NULL; // All objectives
VuFilteredList* AllCampList = NULL; // All campaign entities
VuFilteredList* InactiveList = NULL; // Inactive units (reinforcements)
#endif

// ==============================
// Maintained Collections
// ==============================

#if not USE_VU_COLL_FOR_CAMPAIGN
CampBaseMap::CampBaseMap(const string &name) : mutex(F4CreateCriticalSection(name.c_str())) {}
CampBaseMap::~CampBaseMap()
{
    F4DestroyCriticalSection(mutex);
}
void CampBaseMap::insert(CampBaseBin cb)
{
    F4ScopeLock l(mutex);
    StdCampBaseMap::insert(make_pair(cb->Id(), cb));
}
void CampBaseMap::remove(const VU_ID &id)
{
    F4ScopeLock l(mutex);
    erase(id);
}
const CampBaseMap::iterator CampBaseMap::begin()
{
    return StdCampBaseMap::begin();
}
const CampBaseMap::iterator CampBaseMap::end()
{
    return StdCampBaseMap::end();
}
#endif

F4PFList FrontList = NULL; // Frontline objectives
F4POList POList = NULL; // Primary objective list
F4PFList SOList = NULL; // Secondary objective list
F4PFList AirDefenseList = NULL; // All air defenses
F4PFList EmitterList = NULL; // All emitters
//F4PFList DeaggregateList = NULL; // All deaggregated units
#if USE_VU_COLL_FOR_CAMPAIGN
#if VU_ALL_FILTERED
VuHashTable *deaggregatedEntities = NULL;
#else
VuFilteredHashTable *deaggregatedEntities = NULL;
#endif
#else
CampBaseMap *deaggregatedMap = NULL;
#endif

#if USE_VU_COLL_FOR_DIRTY
TailInsertList *campDirtyBuckets[MAX_DIRTY_BUCKETS];
TailInsertList *simDirtyBuckets[MAX_DIRTY_BUCKETS];
#else
FalconEntityList *campDirtyBuckets[MAX_DIRTY_BUCKETS];
FalconEntityList *simDirtyBuckets[MAX_DIRTY_BUCKETS];
#endif

F4CSECTIONHANDLE *campDirtyMutexes[MAX_DIRTY_BUCKETS];
F4CSECTIONHANDLE *simDirtyMutexes[MAX_DIRTY_BUCKETS];

// ==============================
// Objective data lists
// ==============================

List PODataList = NULL;
List FLOTList = NULL; // A List of PackXY points defining the Forward Line Of Troops.

// ==============================
// Proximity Lists
// ==============================

VuGridTree* ObjProxList = NULL; // Proximity list of all objectives
VuGridTree* RealUnitProxList = NULL; // Proximity list of all units

// ==============================
// List maintenance routines
// ==============================

// All versions of campaign use these lists
void InitBaseLists(void)
{
#if VU_ALL_FILTERED
    AllCampList = new VuLinkedList(&CampFilter);
#else
    AllCampList = new VuFilteredList(&CampFilter);
#endif
    AllCampList->Register();
    //deaggregateList = new FalconPrivateList (&CampFilter);
    //DeaggregateList->Init();
    /* sfr: these are initialized with campaign now
    for (int loop = 0; loop < MAX_DIRTY_BUCKETS; loop ++){
     // sfr: new dirty buckets
     //DirtyBucket[loop] = new TailInsertList (&AllOpaqueFilter);
     //DirtyBucket[loop]->Init();
     campDirtyBuckets[loop] = new list<FalconEntityBin>;
     campDirtyMutexes[loop] = F4CreateCriticalSection("camp dirty mutex");
     simDirtyBuckets[loop] = new list<FalconEntityBin>;
     simDirtyMutexes[loop] = F4CreateCriticalSection("sim dirty mutex");
    }
    */
    EmitterList = new FalconPrivateList(&CampFilter);
    EmitterList->Register();
}

void InitCampaignLists(void)
{
#if USE_VU_COLL_FOR_CAMPAIGN
#if VU_ALL_FILTERED
    deaggregatedEntities = new VuHashTable(&FalconNothingFilter, 101);
#else
    deaggregatedEntities = new VuFilteredHashTable(&FalconNothingFilter, 101);
#endif
    deaggregatedEntities->Register();
#else
    deaggregatedMap = new CampBaseMap("deaggregated campbase");
#endif

    /* sfr: these are initialized with campaign now */
    for (int loop = 0; loop < MAX_DIRTY_BUCKETS; loop ++)
    {
        // sfr: new dirty buckets
        //DirtyBucket[loop] = new TailInsertList (&AllOpaqueFilter);
        //DirtyBucket[loop]->Init();
#if USE_VU_COLL_FOR_DIRTY
        campDirtyBuckets[loop] = new TailInsertList(&AllOpaqueFilter);
        campDirtyBuckets[loop]->Register();
        simDirtyBuckets[loop] = new TailInsertList(&AllOpaqueFilter);
        simDirtyBuckets[loop]->Register();
#else
        campDirtyBuckets[loop] = new list<FalconEntityBin>;
        simDirtyBuckets[loop] = new list<FalconEntityBin>;
#endif
        campDirtyMutexes[loop] = F4CreateCriticalSection("camp dirty mutex");
        simDirtyMutexes[loop] = F4CreateCriticalSection("sim dirty mutex");
    }


#if VU_ALL_FILTERED
    AllUnitList = new VuLinkedList(&AllUnitFilter);
    AllAirList = new VuLinkedList(&AllAirFilter);
    AllParentList = new VuLinkedList(&AllParentFilter);
    AllRealList = new VuLinkedList(&AllRealFilter);
    AllObjList = new VuLinkedList(&AllObjFilter);
    InactiveList = new VuLinkedList(&InactiveFilter);
#else
    AllUnitList = new VuFilteredList(&AllUnitFilter);
    AllAirList = new VuFilteredList(&AllAirFilter);
    AllParentList = new VuFilteredList(&AllParentFilter);
    AllRealList = new VuFilteredList(&AllRealFilter);
    AllObjList = new VuFilteredList(&AllObjFilter);
    InactiveList = new VuFilteredList(&InactiveFilter);
#endif
    AllUnitList->Register();
    AllAirList->Register();
    AllParentList->Register();
    AllRealList->Register();
    AllObjList->Register();
    InactiveList->Register();

    FrontList = new FalconPrivateList(&AllObjFilter);
    FrontList->Register();
    AirDefenseList = new FalconPrivateList(&CampFilter);
    AirDefenseList->Register();
    POList = new FalconPrivateOrderedList(&AllObjFilter);
    POList->Register();
    SOList = new FalconPrivateList(&AllObjFilter);
    SOList->Register();

    if (PODataList == NULL)
    {
        PODataList = new ListClass();
    }
    else
    {
        PODataList->Purge();
    }

    if (FLOTList == NULL)
    {
        FLOTList = new ListClass(LADT_SORTED_LIST);
    }
    else
    {
        FLOTList->Purge();
    }

#if GRID_CORRECTION
    RealUnitProxFilter = new UnitProxFilter(TREE_RES, (GRID_SIZE_FT * (Map_Max_Y + 1)));
#else
    RealUnitProxFilter = new UnitProxFilter(1);
#endif
#ifdef VU_GRID_TREE_Y_MAJOR
    RealUnitProxList = new VuGridTree(RealUnitProxFilter, TREE_RES);
#else
    RealUnitProxList = new VuGridTree(RealUnitProxFilter, TREE_RES);
#endif
    RealUnitProxList->Register();
}

void InitTheaterLists(void)
{
#if GRID_CORRECTION
    AllObjProxFilter = new ObjProxFilter(TREE_RES, (GRID_SIZE_FT * (Map_Max_Y + 1)));
#else
    AllObjProxFilter = new ObjProxFilter();
#endif
#ifdef VU_GRID_TREE_Y_MAJOR
    ObjProxList = new VuGridTree(AllObjProxFilter, TREE_RES);
#else
    ObjProxList = new VuGridTree(AllObjProxFilter, TREE_RES);
#endif
    ObjProxList->Register();
}

void DisposeBaseLists(void)
{
    AllCampList->Unregister();
    delete AllCampList;
    AllCampList = NULL;
    //DeaggregateList->DeInit();
    //delete DeaggregateList;
    /* sfr: finalized with campaign now
    for (int loop = 0; loop < MAX_DIRTY_BUCKETS; loop ++){
     // sfr: new dirty bucket
     //DirtyBucket[loop]->DeInit();
     //delete DirtyBucket[loop];
     //DirtyBucket[loop] = NULL;
     delete simDirtyBuckets[loop];
     F4DestroyCriticalSection(simDirtyMutexes[loop]);
     delete campDirtyBuckets[loop];
     F4DestroyCriticalSection(campDirtyMutexes[loop]);
    }*/
    EmitterList->Unregister();
    delete EmitterList;
    EmitterList = NULL;
}

// Called on campaign end
void DisposeCampaignLists(void)
{
    /* sfr: finalized with campaign now */
    for (int loop = 0; loop < MAX_DIRTY_BUCKETS; loop ++)
    {
        // sfr: new dirty bucket
        //DirtyBucket[loop]->DeInit();
        //delete DirtyBucket[loop];
        //DirtyBucket[loop] = NULL;
#if USE_VU_COLL_FOR_DIRTY
        simDirtyBuckets[loop]->Unregister();
#endif
        delete simDirtyBuckets[loop];
        F4DestroyCriticalSection(simDirtyMutexes[loop]);
        simDirtyBuckets[loop] = NULL;
        simDirtyMutexes[loop] = NULL;

#if USE_VU_COLL_FOR_DIRTY
        campDirtyBuckets[loop]->Unregister();
#endif
        delete campDirtyBuckets[loop];
        F4DestroyCriticalSection(campDirtyMutexes[loop]);
        campDirtyBuckets[loop] = NULL;
        campDirtyMutexes[loop] = NULL;
    }

#if USE_VU_COLL_FOR_CAMPAIGN
    deaggregatedEntities->Unregister();
    delete deaggregatedEntities;
    deaggregatedEntities = NULL;
#else
    delete deaggregatedMap;
    deaggregatedMap = NULL;
#endif


    DisposeObjList();

    if (AllUnitList)
    {
        AllUnitList->Unregister();
        delete AllUnitList;
        AllUnitList = NULL;
    }

    if (AllAirList)
    {
        AllAirList->Unregister();
        delete AllAirList;
        AllAirList = NULL;
    }

    if (AllParentList)
    {
        AllParentList->Unregister();
        delete AllParentList;
        AllParentList = NULL;
    }

    if (AllRealList)
    {
        AllRealList->Unregister();
        delete AllRealList;
        AllRealList = NULL;
    }

    if (AllObjList)
    {
        AllObjList->Unregister();
        delete AllObjList;
        AllObjList = NULL;
    }

    if (InactiveList)
    {
        InactiveList->Unregister();
        delete InactiveList;
        InactiveList = NULL;
    }

    if (FrontList)
    {
        FrontList->Unregister();
        delete FrontList;
        FrontList = NULL;
    }

    if (AirDefenseList)
    {
        AirDefenseList->Unregister();
        delete AirDefenseList;
        AirDefenseList = NULL;
    }

    if (POList)
    {
        POList->Unregister();
        delete POList;
        POList = NULL;
    }

    if (SOList)
    {
        SOList->Unregister();
        delete SOList;
        SOList = NULL;
    }

    if (RealUnitProxList)
    {
        RealUnitProxList->Unregister();
        delete RealUnitProxList;
        RealUnitProxList = NULL;
    }

    if (RealUnitProxFilter)
    {
        delete RealUnitProxFilter;
        RealUnitProxFilter = NULL;
    }
}

void DisposeTheaterLists(void)
{
    if (AllObjProxFilter)
    {
        delete AllObjProxFilter;
    }

    if (ObjProxList)
    {
        ObjProxList->Unregister();
        delete ObjProxList;
    }

    AllObjProxFilter = NULL;
    ObjProxList = NULL;
}

// ============================================
// List rebuilders (Called at various intervals
// ============================================

int RebuildFrontList(int do_barcaps, int incremental)
{
    MissionRequestClass mis;
    Objective o, n;
    ulong fseed;
    //static CampaignTime lastRequest=0;
    //CampaignTime testLast;
    int i, front, isolated, bok = 0, dirty = 0;

    // HACK to allow furball to work
    if (FalconLocalGame and FalconLocalGame->GetGameType() == game_Dogfight)
    {
        return 0;
    }

    //TJL 11/14/03 Barcap timer and request interval were severely broken
    //lastRequest is now subtracted from CurrentTime.
    //Barcap Request also needed to be converted to milliseconds.
    //if (do_barcaps and lastRequest - Camp_GetCurrentTime() > (unsigned int)BARCAP_REQUEST_INTERVAL)
    if (
        do_barcaps and 
        (Camp_GetCurrentTime() - lastRequest > ((unsigned int)BARCAP_REQUEST_INTERVAL * CampaignMinutes))
    )
    {
        lastRequest = Camp_GetCurrentTime();
        bok = 1;
    }

    if ( not incremental)
    {
        FrontList->Purge();
    }

    {
        VuListIterator myit(AllObjList);
        o = GetFirstObjective(&myit);

        while (o)
        {
            front = 0;
            fseed = 0;
            isolated = 1;

            for (i = 0, n = o; i < o->static_data.links and n and ( not front or isolated); i++)
            {
                n = o->GetNeighbor(i);

                if (n)
                {
                    if (GetTTRelations(o->GetTeam(), n->GetTeam()) > Neutral)
                    {
                        front = n->GetOwner();
                    }
                    else if (isolated and not GetRoE(n->GetTeam(), o->GetTeam(), ROE_GROUND_CAPTURE))
                    {
                        isolated = 0;
                    }

                    if (bok and front and GetRoE(o->GetTeam(), n->GetTeam(), ROE_AIR_ENGAGE))
                    {
                        fseed = o->Id() + n->Id();
                        mis.vs = n->GetTeam();
                    }
                }
            }

            if (front and isolated and not o->IsSecondary())
            {
                // This objective has been cut off
                Unit u;
                GridIndex x, y;
                o->GetLocation(&x, &y);
                u = FindNearestRealUnit(x, y, NULL, 5);

                if (u and GetRoE(u->GetTeam(), o->GetTeam(), ROE_GROUND_CAPTURE))
                {
                    front = u->GetOwner();
                }

                if ( not u or (u and u->GetTeam() not_eq o->GetTeam()))
                {
                    // Enemy units are in control, send a captured message
                    CaptureObjective(o, (Control)front, NULL);
                }
            }

            if ( not front)
            {
                if (o->IsFrontline())
                {
                    dirty = 1;

                    if (incremental)
                    {
                        FrontList->Remove(o);
                    }

                    o->ClearObjFlags(O_FRONTLINE bitor O_SECONDLINE bitor O_THIRDLINE);
                }

                o->SetAbandoned(0);
                fseed = 0;
            }
            else if (front)
            {
                if ( not o->IsFrontline())
                {
                    dirty = 1;
                    o->SetObjFlags(O_FRONTLINE);
                    o->ClearObjFlags(O_SECONDLINE bitor O_THIRDLINE);
                }

                if ( not incremental or not o->IsFrontline())
                {
                    FrontList->ForcedInsert(o);
                }
            }

            //TJL 11/14/03 This is the BARCAP generator for the campaign.
            if (fseed and do_barcaps)
            {
                int barsweep = rand() % 3;
                // Request a low priority BARCAP mission (each side should take care of their own)
                mis.requesterID = o->Id();
                o->GetLocation(&mis.tx, &mis.ty);
                mis.who = o->GetTeam();
                mis.vs = 0;
                // Try to base TOT on the combined ID of the two frontline objectives -
                // to try and get both teams here at same time
                mis.tot = Camp_GetCurrentTime() + (60 + fseed % 60) * CampaignMinutes;
                mis.tot_type = TYPE_EQ;

                //Cobra This allows a variety of CAP missions
                if (barsweep == 1)
                {
                    mis.mission = AMIS_SWEEP;
                }
                else if (barsweep == 2)
                {
                    mis.mission = AMIS_BARCAP;
                }
                else
                {
                    mis.mission = AMIS_BARCAP2;
                }

                mis.context = hostileAircraftPresent;
                mis.roe_check = ROE_AIR_ENGAGE;
                mis.flags = 0;
                mis.priority = 0;
                mis.RequestMission();
            }

            o = GetNextObjective(&myit);
        }
    }

    if (dirty)
    {
        MarkObjectives();
        RebuildParentsList();
    }

    return dirty;
}

void RebuildFLOTList(void)
{
    Objective o, n;
    int i, found;
    ListElementClass *lp;
    void *data;
    GridIndex ox, oy, nx, ny, fx, fy, x, y;

    // KCK WARNING: This list sorts for WEST TO EAST. This will look very bad in some situations.
    // I need to think of an algorythm to sort based on the relative geometry between frontline objectives.
    FLOTList->Purge();
    VuListIterator frontit(FrontList);
    o = (Objective) frontit.GetFirst();

    while (o)
    {
        o->GetLocation(&ox, &oy);

        for (i = 0; i < o->NumLinks(); i++)
        {
            n = o->GetNeighbor(i);

            if (n and n->IsFrontline() and o->GetTeam() not_eq n->GetTeam())
            {
                n->GetLocation(&nx, &ny);
                fx = (short)((ox + nx) / 2);
                fy = (short)((oy + ny) / 2);
                lp = FLOTList->GetFirstElement();
                data = PackXY(fx, fy);
                found = 0;

                while (lp and not found)
                {
                    UnpackXY(lp->GetUserData(), &x, &y);

                    if (DistSqu(x, y, fx, fy) < 900.0F) // Min 30 km between points
                        found = 1;

                    lp = lp->GetNext();
                }

                if ( not found)
                {
                    if (FLOTSortDirection)
                        FLOTList->InsertNewElement(fy, data, 0);
                    else
                        FLOTList->InsertNewElement(fx, data, 0);
                }
            }
        }

        o = (Objective) frontit.GetNext();
    }
}

// This will set flags for secondline and thirdline objectives, using the FrontList
void MarkObjectives(void)
{
    FalconPrivateList secondlist(&AllObjFilter);

    Objective o, n;
    int i;

    // KCK: It seems like this should be able to be avoided - but I can't
    // think of another way to clear out old secondline and thirdline flags
    {
        VuListIterator myit(AllObjList);
        o = GetFirstObjective(&myit);

        while (o not_eq NULL)
        {
            o->ClearObjFlags(O_SECONDLINE bitor O_THIRDLINE);
            o = GetNextObjective(&myit);
        }
    }

    {
        VuListIterator frontit(FrontList);
        o = GetFirstObjective(&frontit);

        while (o not_eq NULL)
        {
            for (i = 0; i < o->NumLinks(); i++)
            {
                n = o->GetNeighbor(i);

                if (n)
                {
                    if ( not n->IsFrontline() and o->GetTeam() == n->GetTeam())
                    {
                        n->SetObjFlags(O_SECONDLINE);
                        secondlist.ForcedInsert(n);
                    }
                }
            }

            o = GetNextObjective(&frontit);
        }
    }

    {
        VuListIterator secondit(&secondlist);
        o = GetFirstObjective(&secondit);

        while (o not_eq NULL)
        {
            for (i = 0; i < o->NumLinks(); i++)
            {
                n = o->GetNeighbor(i);

                if (n)
                {
                    if ( not n->IsFrontline() and not n->IsSecondline())
                    {
                        n->SetObjFlags(O_THIRDLINE);
                    }
                }
            }

            o = GetNextObjective(&secondit);
        }
    }
}

// This only needs to be called at the start of the campaign (after objs loaded)
// or if any new objectives have been added
int RebuildObjectiveLists(void)
{
    Objective o;

    POList->Purge();
    SOList->Purge();

    {
        // destroy iterator here
        VuListIterator myit(AllObjList);
        o = GetFirstObjective(&myit);

        while (o not_eq NULL)
        {
            if (o->IsPrimary())
                POList->ForcedInsert(o);

            if (o->IsSecondary())
                SOList->ForcedInsert(o);

            o = GetNextObjective(&myit);
        }
    }
    CleanupObjList();
    return 1;
}

// This recalculates which secondary objectives belong to which primaries
// Assumes RebuildObjectiveLists has been called
int RebuildParentsList(void)
{
    Objective o;
    VuListIterator myit(AllObjList);
    o = GetFirstObjective(&myit);

    while (o)
    {
        o->RecalculateParent();
        o = GetNextObjective(&myit);
    }

    return 1;
}

// Sets up Emitting flag for entities with detection capibilities
int RebuildEmitterList()
{
    CampEntity e, a;
    int range, d, r, emit, team, rl, change = 0;
    MoveType mt;
    GridIndex x, y, ex, ey;

    EmitterList->Purge();
    AirDefenseList->Purge();

    VuListIterator campit(AllCampList);
    e = (CampEntity)campit.GetFirst();

    while (e)
    {
        if (e->GetDomain() == DOMAIN_LAND and ( not e->IsUnit() or ( not ((Unit)e)->Inactive() and ((Unit)e)->Real())))
        {
            rl = e->GetElectronicDetectionRange(LowAir);
            range = e->GetElectronicDetectionRange(Air);

            if (rl > range)
            {
                range = rl;
                mt = LowAir;
            }
            else
            {
                mt = Air;
            }

            if (range)
            {
                // It's part of the IADS, check if we're needed
                emit = 1;
                e->GetLocation(&x, &y);
                team = e->GetTeam();

                if ( not e->IsObjective())
                {
                    VuListIterator airit(EmitterList);
                    a = (CampEntity)airit.GetFirst();

                    while (a and emit)
                    {
                        CampEntity next = (CampEntity)airit.GetNext();

                        if (a->GetTeam() == team and not a->IsObjective())
                        {
                            a->GetLocation(&ex, &ey);
                            d =  FloatToInt32(Distance(x, y, ex, ey));
                            r = a->GetElectronicDetectionRange(mt);

                            if (r > d + range)
                            {
                                emit = 0;
                            }
                            else if ((range > d + r) and (a->GetRadarMode() < FEC_RADAR_SEARCH_1))
                            {
                                a->SetEmitting(0);
                            }
                        }

                        a = next;
                    }
                }

                if (e->IsBattalion() and e->GetSType() == STYPE_UNIT_AIR_DEFENSE)
                    // if (e->GetAproxHitChance(LowAir,0) > 0)
                    AirDefenseList->ForcedInsert(e);

                if ( not change and ((emit and not e->IsEmitting()) or ( not emit and e->IsEmitting())))
                    change = 1;

                e->SetEmitting(emit);

                if (emit and e->GetRadarMode() < FEC_RADAR_AQUIRE)
                    e->SetSearchMode(FEC_RADAR_SEARCH_1);//me123 + rand()%3);
                else if ( not emit)
                    e->SetSearchMode(FEC_RADAR_OFF); // Our "search mode" is off
            }
        }

        e = (CampEntity)campit.GetNext();
    }

    return change;
}

// This will rebuild all changing lists
void StandardRebuild(void)
{
    static int build = 0;
    RebuildFrontList(TRUE, FALSE);
    RebuildEmitterList();

    if ( not build or not TheCampaign.SamMapData)
    {
        TheCampaign.MakeCampMap(MAP_SAMCOVERAGE);
    }
    else if (build == 1 or not TheCampaign.RadarMapData)
    {
        TheCampaign.MakeCampMap(MAP_RADARCOVERAGE);
    }
    else
    {
        build = -1;
    }

    if ( not TheCampaign.CampMapData)
    {
        TheCampaign.MakeCampMap(MAP_OWNERSHIP);
    }

    build++;
}



// JPO - some debug stuff - to check if lists are ok
int CheckObjProxyOK(int X, int Y)
{
    if (ObjProxList == NULL) return 1;

#ifdef VU_GRID_TREE_Y_MAJOR
    VuGridIterator myit(ObjProxList, (BIG_SCALAR)GridToSim(X), (BIG_SCALAR)GridToSim(Y), (BIG_SCALAR)GridToSim(100));
#else
    VuGridIterator myit(ObjProxList, (BIG_SCALAR)GridToSim(Y), (BIG_SCALAR)GridToSim(X), (BIG_SCALAR)GridToSim(100));
#endif
    Objective o;

    for (
        o = (Objective) myit.GetFirst();
        o not_eq NULL;
        o = (Objective) myit.GetNext()
    )
    {
        if (F4IsBadReadPtr(o, sizeof * o))
        {
            return 0;
        }
    }

    return 1;
}

int CheckUnitProxyOK(int X, int Y)
{
    if (RealUnitProxList == NULL) return 1;

#ifdef VU_GRID_TREE_Y_MAJOR
    VuGridIterator myit(RealUnitProxList, (BIG_SCALAR)GridToSim(X), (BIG_SCALAR)GridToSim(Y), (BIG_SCALAR)GridToSim(100));
#else
    VuGridIterator myit(RealUnitProxList, (BIG_SCALAR)GridToSim(Y), (BIG_SCALAR)GridToSim(X), (BIG_SCALAR)GridToSim(100));
#endif
    Unit u;

    for (
        u = (Unit) myit.GetFirst();
        u not_eq NULL;
        u = (Unit) myit.GetNext()
    )
    {
        if (F4IsBadReadPtr(u, sizeof * u))
        {
            return 0;
        }
    }

    return 1;
}

// sfr: called when a Unit is inactivated in AllRealList
void InactivateUnit(UnitClass *unit)
{
    // avoid unit deletion when removing from lists
    VuBin<UnitClass> safe(unit);
    // remove from lists
    AllUnitList->Remove(unit);
    AllParentList->Remove(unit);
    AllRealList->Remove(unit);
    RealUnitProxList->Remove(unit);
    // insert into inactive
    InactiveList->Insert(unit);
}

