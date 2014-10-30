// ===================================================
// Package.cpp
// ===================================================

#include <cISO646>
#include <stdio.h>
#include <time.h>
#include "CmpGlobl.h"
#include "ListADT.h"
#include "vutypes.h"
#include "Objectiv.h"
#include "Find.h"
#include "Path.h"
#include "ASearch.h"
#include "Campaign.h"
#include "CampList.h"
#include "mission.h"
#include "Package.h"
#include "update.h"
#include "team.h"
#include "atm.h"
#include "AIInput.h"
#include "MsgInc/AirTaskingMsg.h"
#include "Feature.h"
#include "classtbl.h"
//sfr: included InvalidBuffer
#include "InvalidBufferException.h"

#include "Debuggr.h"

#define AMIS_SUPPORT_MASK AMIS_ADDAWACS bitor AMIS_ADDJSTAR bitor AMIS_ADDECM bitor AMIS_ADDTANKER

#define MIN_FORCE_ESCORT 12

// ============================================
// Externals
// ============================================

#ifdef KEV_ADEBUG
extern char MissStr[AMIS_OTHER][16];
#endif

#ifdef CAMPTOOL
// Renaming tool stuff
extern VU_ID_NUMBER RenameTable[65536];
extern int gRenameIds;
#endif

extern FILE
*save_log,
*load_log;

extern int
start_save_stream,
start_load_stream;

#ifdef DEBUG
int gSEADrequested = 0;
int gSEADgot = 0;
int gFACrequested = 0;
int gFACgot = 0;
int gBARCAPrequested = 0;
int gBARCAPgot = 0;
int gSWEEPrequested = 0;
int gSWEEPgot = 0;
int gESCORTrequested = 0;
int gESCORTgot = 0;

extern int gCheckConstructFunction;
#endif

int PackInserted = 0;

#define ATM_HIGH_PRIORITY 150
#pragma warning (disable : 4786) // debug info truncation

extern int gCampDataVersion;
extern bool g_bTankerWaypoints;
extern bool g_bLargeStrike;

#ifdef DEBUG
//#define DEBUG_COUNT // this can cause crashes when doing long debug runs 
// JPO some simple package tracking code, keeps track of whats been lost.
#include <map>

int gPackageCount = 0;
class PackageListCounter
{
    typedef std::map<int, PackageClass *> ID2PACK;
    int maxoccupancy;

    ID2PACK packlist;
public:
    PackageListCounter() : maxoccupancy(0) { };
    void Report()
    {
        MonoPrint("%d (%d) packages of %d left\n", gPackageCount, packlist.size(), maxoccupancy);
        ID2PACK::iterator it;

        for (it = packlist.begin(); it not_eq packlist.end(); it++)
        {
            PackageClass *ent = (*it).second;
            MonoPrint("Package left %d campid %d ref %d\n", (*it).first,
                      ent->GetCampId(), ent->RefCount());
        }

        packlist.clear();
    }
    void AddObj(int id, PackageClass *entry)
    {
        packlist[id] = entry;

        if (packlist.size() > (WORD) maxoccupancy)
            maxoccupancy = packlist.size();
    };
    void DelObj(int id)
    {
        ID2PACK::iterator it;
        it = packlist.find(id);
        packlist.erase(it);
    }
};
static PackageListCounter mypacklist;
void PackageReport(void)
{
    mypacklist.Report();
}
#endif

// ============================================
// Prototypes
// ============================================

void FinalizeFlight(Unit flight, int flights);

int CallInOCAStrikes(MissionRequest mis);

// =================================
// Smart heap pool stuff
// =================================

#ifdef USE_SH_POOLS
MEM_POOL PackageClass::pool;
#endif

// ============================================
// PackageClass Functions
// ============================================

// KCK: ALL PACKAGE CONSTRUCTION SHOULD USE THIS FUNCTION
PackageClass* NewPackage(int type)
{
    PackageClass *new_package;
    /*VuEnterCriticalSection();
    lastVolatileId = vuAssignmentId;
    vuAssignmentId = lastPackageId;
    // vuAssignmentId = lastLowVolitileId;
    vuLowWrapNumber = FIRST_LOW_VOLATILE_VU_ID_NUMBER;
    vuHighWrapNumber = (FIRST_LOW_VOLATILE_VU_ID_NUMBER + LAST_LOW_VOLATILE_VU_ID_NUMBER)/2;*/
    new_package = new PackageClass(type);
    /*lastPackageId = vuAssignmentId;
    // lastLowVolitileId = vuAssignmentId;
    vuAssignmentId = lastVolatileId;
    vuLowWrapNumber = FIRST_VOLATILE_VU_ID_NUMBER;
    vuHighWrapNumber = LAST_VOLATILE_VU_ID_NUMBER;
    VuExitCriticalSection();*/
    return new_package;
}

PackageClass::PackageClass(ushort type) : AirUnitClass(type, GetIdFromNamespace(PackageNS))
{
    elements = 0;
    c_element = 0;
    memset(element, 0, sizeof(VU_ID)*MAX_UNIT_CHILDREN);
    interceptor = FalconNullId;
    wait_cycles = 0;
    flights = 0;
    wait_for = 0;
    iax = iay = 0;
    eax = eay = 0;
    bpx = bpy = 0;
    tpx = tpy = 0;
    takeoff = 0;
    tp_time = 0;
    caps = 0;
    package_flags = 0;
    requests = 0;
    responses = 0;
    aa_strength = 0;
    ingress = NULL;
    egress = NULL;
    dirty_package = 0;
    SetParent(1);
#ifdef DEBUG_COUNT
    gPackageCount++;
    mypacklist.AddObj(Id().num_, this);
#endif
}

PackageClass::PackageClass(VU_BYTE **stream, long *rem) : AirUnitClass(stream, rem)
{
    if (load_log)
    {
        fprintf(load_log, "%08x PackageClass ", *stream - start_load_stream);
        fflush(load_log);
    }

    if (share_.id_.creator_ == vuLocalSession.creator_)
    {
        PackageNS.UseId(share_.id_.num_);
        // and share_.id_.num_ > lastPackageId){
        //printf("does this happen?");
        // lastPackageId = share_.id_.num_;
    }

    memset(element, 0, sizeof(VU_ID)*MAX_UNIT_CHILDREN);
    memcpychk(&elements, stream, sizeof(uchar), rem);
    memcpychk(element, stream, sizeof(VU_ID)*elements, rem);
#ifdef DEBUG

    for (int i = 0; i < elements; i++)
        element[i].num_ and_eq 0xffff;

#endif
    memcpychk(&interceptor, stream, sizeof(VU_ID), rem);

    if (gCampDataVersion >= 7)
    {
        memcpychk(&awacs, stream, sizeof(VU_ID), rem);
        memcpychk(&jstar, stream, sizeof(VU_ID), rem);
        memcpychk(&ecm, stream, sizeof(VU_ID), rem);
        memcpychk(&tanker, stream, sizeof(VU_ID), rem);
    }

#ifdef DEBUG
    awacs.num_ and_eq 0x0000ffff;
    jstar.num_ and_eq 0x0000ffff;
    ecm.num_ and_eq 0x0000ffff;
    tanker.num_ and_eq 0x0000ffff;
#endif
    memcpychk(&wait_cycles, stream, sizeof(uchar), rem);

    // If this package has already been planned, we can save some room
    if (Final() and not wait_cycles)
    {

        memcpychk(&requests, stream, sizeof(short), rem);

        if (gCampDataVersion < 35)
        {
            short threat_stats;
            memcpychk(&threat_stats, stream, sizeof(short), rem);
        }

        memcpychk(&responses, stream, sizeof(short), rem);
        memcpychk(&mis_request.mission, stream, sizeof(short), rem);
        memcpychk(&mis_request.context, stream, sizeof(short), rem);
        memcpychk(&mis_request.requesterID, stream, sizeof(VU_ID), rem);
        memcpychk(&mis_request.targetID, stream, sizeof(VU_ID), rem);

#ifdef DEBUG
        mis_request.requesterID.num_ and_eq 0x0000ffff;
        mis_request.targetID.num_ and_eq 0x0000ffff;
#endif

        if (gCampDataVersion >= 26)
        {
            memcpychk(&mis_request.tot, stream, sizeof(CampaignTime), rem);
        }
        else if (gCampDataVersion >= 16)
        {
            memcpychk(&mis_request.tot, stream, sizeof(CampaignTime), rem);
        }

        if (gCampDataVersion >= 35)
        {
            memcpychk(&mis_request.action_type, stream, sizeof(uchar), rem);
        }
        else
        {
            mis_request.action_type = 0;
        }

        if (gCampDataVersion >= 41)
        {
            memcpychk(&mis_request.priority, stream, sizeof(short), rem);
        }
        else
        {
            mis_request.priority = 1;
        }

        ingress = NULL;
        egress = NULL;
        package_flags = 0;
    }

    // Otherwise, we'd better save everything
    else
    {
        uchar wps;
        WayPoint w, lw;

        memcpychk(&flights, stream, sizeof(uchar), rem);
        memcpychk(&wait_for, stream, sizeof(short), rem);
        memcpychk(&iax, stream, sizeof(GridIndex), rem);
        memcpychk(&iay, stream, sizeof(GridIndex), rem);
        memcpychk(&eax, stream, sizeof(GridIndex), rem);
        memcpychk(&eay, stream, sizeof(GridIndex), rem);
        memcpychk(&bpx, stream, sizeof(GridIndex), rem);
        memcpychk(&bpy, stream, sizeof(GridIndex), rem);
        memcpychk(&tpx, stream, sizeof(GridIndex), rem);
        memcpychk(&tpy, stream, sizeof(GridIndex), rem);
        memcpychk(&takeoff, stream, sizeof(CampaignTime), rem);
        memcpychk(&tp_time, stream, sizeof(CampaignTime), rem);
        memcpychk(&package_flags, stream, sizeof(ulong), rem);
        memcpychk(&caps, stream, sizeof(short), rem);
        memcpychk(&requests, stream, sizeof(short), rem);

        if (gCampDataVersion < 35)
        {
            short threat_stats;
            memcpychk(&threat_stats, stream, sizeof(short), rem);
        }

        memcpychk(&responses, stream, sizeof(short), rem);
        // Read in routes
        memcpychk(&wps, stream, sizeof(uchar), rem);
        ingress = lw = NULL;

        while (wps)
        {
            w = new WayPointClass(stream, rem);

            if ( not lw)
                ingress = lw = w;
            else
                lw->InsertWP(w);

            lw = w;
            wps--;
        }

        memcpychk(&wps, stream, sizeof(uchar), rem);

        egress = lw = NULL;

        while (wps)
        {
            w = new WayPointClass(stream, rem);

            if ( not lw)
                egress = lw = w;
            else
                lw->InsertWP(w);

            lw = w;
            wps--;
        }

        if (gCampDataVersion < 35)
        {
            // This isn't valid any more
            memcpychk(&mis_request, stream, 64, rem);
        }
        else
        {
            // MLR Had a CTD here while exiting Takeoff TE,
            // called by LoadCampaign()->LoadUnit()->DecodeUnitData()->NewUnit()
            // the stream isn't long enough, trying to copy 76 bytes.  Reason unknown?
            memcpychk(&mis_request, stream, sizeof(MissionRequestClass), rem);
        }
    }

    dirty_package = 0;
#ifdef DEBUG_COUNT
    gPackageCount++;
    mypacklist.AddObj(Id().num_, this);
#endif
}

PackageClass::~PackageClass(void)
{
    if (IsAwake())
        Sleep();

    WayPoint w, nw;

    w = egress;

    while (w)
    {
        nw = w->GetNextWP();
        delete w;
        w = nw;
    }

    egress = NULL;
    w = ingress;

    while (w)
    {
        nw = w->GetNextWP();
        delete w;
        w = nw;
    }

    ingress = NULL;

    if (wp_list)
        DisposeWayPoints();

    wp_list = NULL;
#ifdef DEBUG_COUNT
    gPackageCount--;
    mypacklist.DelObj(Id().num_);
#endif
}

int PackageClass::SaveSize(void)
{
    WayPoint w;
    int iw = 0, ew = 0, size;

    size = AirUnitClass::SaveSize()
           + sizeof(uchar)
           // + sizeof(VU_ID)*MAX_UNIT_CHILDREN
           + sizeof(VU_ID) * elements
           + sizeof(VU_ID)
           + sizeof(VU_ID)
           + sizeof(VU_ID)
           + sizeof(VU_ID)
           + sizeof(VU_ID)
           + sizeof(uchar);

    // If this package has already been planned, we can save some room
    if (Final() and not wait_cycles)
    {
        size += sizeof(short)
                + sizeof(short)
                + sizeof(short)
                + sizeof(short)
                + sizeof(VU_ID)
                + sizeof(VU_ID)
                + sizeof(CampaignTime)
                + sizeof(uchar)
                + sizeof(short);
    }
    else
    {
        // Count route lengths
        w = ingress;

        while (w)
        {
            iw++;
            w = w->GetNextWP();
        }

        w = egress;

        while (w)
        {
            ew++;
            w = w->GetNextWP();
        }

        size += sizeof(uchar)
                + sizeof(short)
                + sizeof(GridIndex)
                + sizeof(GridIndex)
                + sizeof(GridIndex)
                + sizeof(GridIndex)
                + sizeof(GridIndex)
                + sizeof(GridIndex)
                + sizeof(GridIndex)
                + sizeof(GridIndex)
                + sizeof(CampaignTime)
                + sizeof(CampaignTime)
                + sizeof(ulong)
                + sizeof(short)
                + sizeof(short)
                + sizeof(short)
                + sizeof(uchar)
                + sizeof(uchar)
                + sizeof(MissionRequestClass);
        w = ingress;

        while (w)
        {
            size += w->SaveSize();
            w = w->GetNextWP();
        }

        w = egress;

        while (w)
        {
            size += w->SaveSize();
            w = w->GetNextWP();
        }
    }

    return size;
}

int PackageClass::Save(VU_BYTE **stream)
{
    WayPoint w;
    uchar iw = 0, ew = 0;

    AirUnitClass::Save(stream);

    if (save_log)
    {
        fprintf(save_log, "%08x PackageClass ", *stream - start_save_stream);
        fflush(save_log);
    }

    memcpy(*stream, &elements, sizeof(uchar));
    *stream += sizeof(uchar);
#ifdef CAMPTOOL

    if (gRenameIds)
    {
        for (int i = 0; i < elements; i++)
            element[i].num_ = RenameTable[element[i].num_];

        interceptor.num_ = RenameTable[interceptor.num_];
        awacs.num_ = RenameTable[awacs.num_];
        jstar.num_ = RenameTable[jstar.num_];
        ecm.num_ = RenameTable[ecm.num_];
        tanker.num_ = RenameTable[tanker.num_];
        mis_request.requesterID.num_ = RenameTable[mis_request.requesterID.num_];
        mis_request.targetID.num_ = RenameTable[mis_request.targetID.num_];
    }

#endif
    memcpy(*stream, element, sizeof(VU_ID)*elements);
    *stream += sizeof(VU_ID) * elements;
    memcpy(*stream, &interceptor, sizeof(VU_ID));
    *stream += sizeof(VU_ID);
    memcpy(*stream, &awacs, sizeof(VU_ID));
    *stream += sizeof(VU_ID);
    memcpy(*stream, &jstar, sizeof(VU_ID));
    *stream += sizeof(VU_ID);
    memcpy(*stream, &ecm, sizeof(VU_ID));
    *stream += sizeof(VU_ID);
    memcpy(*stream, &tanker, sizeof(VU_ID));
    *stream += sizeof(VU_ID);
    memcpy(*stream, &wait_cycles, sizeof(uchar));
    *stream += sizeof(uchar);

    // If this package has already been planned, we can save some room
    if (Final() and not wait_cycles)
    {
        memcpy(*stream, &requests, sizeof(short));
        *stream += sizeof(short);
        memcpy(*stream, &responses, sizeof(short));
        *stream += sizeof(short);
        memcpy(*stream, &mis_request.mission, sizeof(short));
        *stream += sizeof(short);
        memcpy(*stream, &mis_request.context, sizeof(short));
        *stream += sizeof(short);
        memcpy(*stream, &mis_request.requesterID, sizeof(VU_ID));
        *stream += sizeof(VU_ID);
        memcpy(*stream, &mis_request.targetID, sizeof(VU_ID));
        *stream += sizeof(VU_ID);
        memcpy(*stream, &mis_request.tot, sizeof(CampaignTime));
        *stream += sizeof(CampaignTime);
        memcpy(*stream, &mis_request.action_type, sizeof(uchar));
        *stream += sizeof(uchar);
        memcpy(*stream, &mis_request.priority, sizeof(short));
        *stream += sizeof(short);
    }

    // Otherwise, we'd better save everything
    else
    {
        // Count route lengths
        w = ingress;

        while (w)
        {
            iw++;
            w = w->GetNextWP();
        }

        w = egress;

        while (w)
        {
            ew++;
            w = w->GetNextWP();
        }

        memcpy(*stream, &flights, sizeof(uchar));
        *stream += sizeof(uchar);
        memcpy(*stream, &wait_for, sizeof(short));
        *stream += sizeof(short);
        memcpy(*stream, &iax, sizeof(GridIndex));
        *stream += sizeof(GridIndex);
        memcpy(*stream, &iay, sizeof(GridIndex));
        *stream += sizeof(GridIndex);
        memcpy(*stream, &eax, sizeof(GridIndex));
        *stream += sizeof(GridIndex);
        memcpy(*stream, &eay, sizeof(GridIndex));
        *stream += sizeof(GridIndex);
        memcpy(*stream, &bpx, sizeof(GridIndex));
        *stream += sizeof(GridIndex);
        memcpy(*stream, &bpy, sizeof(GridIndex));
        *stream += sizeof(GridIndex);
        memcpy(*stream, &tpx, sizeof(GridIndex));
        *stream += sizeof(GridIndex);
        memcpy(*stream, &tpy, sizeof(GridIndex));
        *stream += sizeof(GridIndex);
        memcpy(*stream, &takeoff, sizeof(CampaignTime));
        *stream += sizeof(CampaignTime);
        memcpy(*stream, &tp_time, sizeof(CampaignTime));
        *stream += sizeof(CampaignTime);
        // package flags need to be saved always... MOVE IN FUTURE
        memcpy(*stream, &package_flags, sizeof(ulong));
        *stream += sizeof(ulong);
        memcpy(*stream, &caps, sizeof(short));
        *stream += sizeof(short);
        memcpy(*stream, &requests, sizeof(short));
        *stream += sizeof(short);
        memcpy(*stream, &responses, sizeof(short));
        *stream += sizeof(short);
        // save routes
        memcpy(*stream, &iw, sizeof(uchar));
        *stream += sizeof(uchar);
        w = ingress;

        while (w)
        {
            w->Save(stream);
            w = w->GetNextWP();
        }

        memcpy(*stream, &ew, sizeof(uchar));
        *stream += sizeof(uchar);
        w = egress;

        while (w)
        {
            w->Save(stream);
            w = w->GetNextWP();
        }

        memcpy(*stream, &mis_request, sizeof(MissionRequestClass));
        *stream += sizeof(MissionRequestClass);
    }

    return PackageClass::SaveSize();
}

// event handlers
int PackageClass::Handle(VuFullUpdateEvent *event)
{
    return (AirUnitClass::Handle(event));
}

#define AIRBASE_ATTACK_WARNING_RANGE 25

int PackageClass::MoveUnit(CampaignTime time)
{
    Unit e;
    e = GetFirstUnitElement();

    if ( not e)
    {
        // This package is gone
        KillUnit();
        return 0;
    }

    return 1;
}

int PackageClass::CheckNeedRequests(void)
{
    if (Final() or Aborted())
        return -1;

    Unit e;

    // Check if we're waiting on planning elements
    if (wait_cycles)
        wait_cycles--;

    if ( not wait_cycles or Camp_GetCurrentTime() > takeoff + (AMIS_TAKEOFF_DELAY - 1) * CampaignMinutes)
    {
        // 2001-09-16 REMOVED by M.N. We don't need this anymore - is done by AddTankerWaypoints result
        // -> If no tanker, packageelement->KillUnit();
        /* if (wait_for bitand AMIS_NEEDTANKER)
         {
        // Cancel package if no tanker planned
        wait_for = 0;
        KillUnit();
        return 0;
        } */
        if ((wait_for bitand AMIS_BARCAP) or (wait_for bitand AMIS_SWEEP))
        {
            // if we're still waiting on these, they arn't planned yet and probably won't be.
            wait_for or_eq AMIS_BARCAP bitor AMIS_SWEEP;
            wait_for xor_eq AMIS_BARCAP bitor AMIS_SWEEP;
        }

        if ((package_flags bitand AMIS_ADDBDA) and (mis_request.priority > MINIMUM_BDA_PRIORITY))
        {
            MissionRequestClass mis;
            Objective target;

            // Add BDA with any spare aircraft
            GetUnitDestination(&mis.tx, &mis.ty);
            target = GetObjectiveByXY(mis.tx, mis.ty);

            if (target)
            {
                mis.mission = AMIS_BDA;
                mis.who = GetTeam();
                mis.vs = target->GetTeam();
                mis.targetID = target->Id();
                mis.target_num = 255;
                mis.aircraft = MissionData[mis.mission].str;
                mis.tot = mis_request.tot + (CampaignTime)(MissionData[mis.mission].separation) * CampaignSeconds;
                mis.tot_type = TYPE_EQ;
                mis.roe_check = ROE_AIR_ENGAGE;
                mis.caps = caps bitor MissionData[mis.mission].caps;
                mis.speed = mis_request.speed;
                mis.priority = GetPriority(&mis);
                e = AttachFlight(&mis, this);

                if (e)
                {
                    if (e->BuildMission(&mis) == PRET_SUCCESS)
                    {
                        RecordFlightAddition((Flight)e, &mis, 0);
                    }
                    else
                    {
                        CancelFlight((Flight)e);
                    }
                }
            }
        }

        // 2001-09-11 ADDED BY M.N. if the packags wants a tanker AND the package
        // really *needs* a tanker, add refueling waypoints

        if (g_bTankerWaypoints)
        {
            if ((wait_for bitand AMIS_ADDTANKER) and (package_flags bitand AMIS_NEEDTANKER))
            {
                e = GetFirstUnitElement();
                Flight f = (Flight)e;

                while (e)
                {
                    // add to all package elements that really need to refuel
                    if (e->GetUnitMission() not_eq AMIS_TANKER and f->refuel not_eq 0)
                    {
                        if ( not AddTankerWayPoint(f, f->refuel))
                        {
                            // M.N. remove the kill unit call - the 2D fuelneeded calculation has a too high
                            // variance so that we shouldn't delete the package here
                            // e->KillUnit();
                            break;
                        }
                    }

                    e = GetNextUnitElement();
                    f = (Flight)e;
                }
            }
        }

        // END OF ADDED SECTION

        // Now finalize our component flights
        e = GetFirstUnitElement();

        while (e)
        {
            FinalizeFlight(e, flights);
            e = GetNextUnitElement();
        }

        if (flights < 2)
        {
            // This package isn't really needed, only parent to one flight.
            // Can we just promote the flight to parent status and remove the package?
        }

        SetFinal(1);
        package_flags = 0;
    }

    return 0;
}

int PackageClass::BuildPackage(MissionRequest mis, F4PFList assemblyList)
{
    int targets = 0, tar = 0, retval = PRET_SUCCESS, result;
    int targetf[5], targetd = 0, hs, ls, bs = 0;
    uchar targeted[128];
    Flight flight = NULL, add_at_end = NULL;
    CampEntity target = NULL;
    MissionRequestClass newmis;

    SetUnitDestination(mis->tx, mis->ty);
    SetLocation(0, 0);
    mis_request = *mis;
    interceptor = FalconNullId;
    package_flags = MissionData[mis->mission].flags;
    escort_type = MissionData[mis->mission].escorttype; // 2001-09-19 M.N.

    aa_strength = 0;
    flights = 0;
    SetFinal(0);

    mis->min_to = 127;
    mis->max_to = -127;

    // RV - Biker - Reduce possibility for large airlift flights
    if (mis->mission == AMIS_AIRLIFT and mis->aircraft > 2)
    {
        mis->aircraft = (rand() % (mis->aircraft)) + 1;
    }

    // RV - Biker - We have a problem with converted missions (4 ship where in MissionData only 1 is allowed)
    mis->aircraft = min(mis->aircraft, MissionData[mis->mission].str);

    // 1. Analyse target, determine # of strikes (and type?) needed.
    memset(targeted, 0, FEATURES_PER_OBJ);

    if (mis->targetID)
        target = FindEntity(mis->targetID);

    if (target and target->IsObjective())
    {
        if (mis->target_num < FEATURES_PER_OBJ)
        {
            targets = 1;
            targetf[tar] = mis->target_num;
            targeted[mis->target_num] = 1;
        }
        else
        {
            // Plan enough strikes to get the target to 0% efficiency (assuming one target per flight)
            int stat, f, j, count = 0;
            short fid, fid2;

            stat = ((Objective)target)->GetObjectiveStatus();

            while (stat > 0 and tar < 5 and targets < 4 and count < 10)
            {
                count++;
                f = BestTargetFeature((Objective)target, targeted);

                if (f >= FEATURES_PER_OBJ or not ((Objective)target)->GetFeatureValue(f))
                {
                    count = 10;
                    continue;
                }

                targeted[f] = 1;

                for (j = 0; j < targets; j++)
                {
                    // Check to make sure the types are different - otherwise it looks weird.
                    fid = ((Objective)target)->GetFeatureID(targetf[j]);
                    fid2 = ((Objective)target)->GetFeatureID(f);

                    if (Falcon4ClassTable[fid].vuClassData.classInfo_[VU_TYPE] == Falcon4ClassTable[fid2].vuClassData.classInfo_[VU_TYPE])
                        j = 100;
                }

                if (j > 50)
                    continue;

                stat -= ((Objective)target)->GetFeatureValue(f);

                if (MissionData[mis->mission].str > 2)

                    // TJL 10/24/03 Allow for Large strike packages in campaign
                    if (g_bLargeStrike)
                    {
                        stat = 100; //TJL 10/24/03 Multiple 4 ships
                    }
                    else
                    {
                        stat = 0; // Never fly multiple 4 ship flights
                    }

                targetf[tar] = f;
                tar++;
                targets++;

                // Force a second strike for high priority packages (the ATM_HIGH_PRIORITY define
                // is a hack - probably should read in from AIInputs)
                if (mis->priority > ATM_HIGH_PRIORITY and targets < 2)
                    stat = 100;
            }

            if (targets < 1)
                return PRET_NOTARGET;
        }

        if (targets > 1)
        {
            // Reduce number of flights while keeping 2 aircraft per target
            mis->aircraft = targets * 2;
            targets = 1;

            // Limit number of aircraft assigned to low priority targets
            if (mis->priority < ATM_HIGH_PRIORITY)
                mis->aircraft = 4;

            if (mis->aircraft > 4)
            {
                mis->aircraft = 4;
                targets = 2;
            }
        }
    }
    else if (target and target->IsUnit())
    {
        targets = 1;
        targetf[tar] = 255;

        if (mis->mission == AMIS_SEADSTRIKE)
        {
            UnitClassDataType *uc = ((Unit)target)->GetUnitClassData();

            if (uc->RadarVehicle < 255 and ((Unit)target)->GetNumVehicles(uc->RadarVehicle))
                targetf[tar] = uc->RadarVehicle;
        }

        if (mis->mission == AMIS_ONCALLCAS)
        {
            targets = ONCALL_CAS_FLIGHTS_PER_REQUEST;
            memset(targetf, 255, 5);
        }
    }
    else // Location
    {
        targets = 1;
        targetf[tar] = 255;
    }

    // Check target viability (SAM coverage at target)
    ls = ScoreThreatFast(mis->tx, mis->ty, GetAltitudeLevel(MissionData[mis->mission].minalt * 100), mis->who);
    hs = ScoreThreatFast(mis->tx, mis->ty, GetAltitudeLevel(MissionData[mis->mission].maxalt * 100), mis->who);

    if (ls > MIN_SEADESCORT_THREAT or hs > MIN_SEADESCORT_THREAT)
        targetd or_eq NEED_SEAD;

    if (mis->priority > ATM_HIGH_PRIORITY and ls and hs)
        targetd or_eq NEED_SEAD;

    if (package_flags bitand AMIS_HIGHTHREAT)
        tar = MAX_FLYMISSION_HIGHTHREAT;
    else if (package_flags bitand AMIS_NOTHREAT)
        tar = MAX_FLYMISSION_NOTHREAT;
    else
        tar = MAX_FLYMISSION_THREAT;

    if (ls > tar and hs > tar)
    {
        // If the threat is so high even SEAD can't get in, just cancel now.
        if (ls > MAX_FLYMISSION_HIGHTHREAT and hs > MAX_FLYMISSION_HIGHTHREAT)
            return PRET_CANCELED;

        // Trigger an OCA strike mission and cancel this mission
        CallInOCAStrikes(mis);
        return PRET_CANCELED;
    }

    tar = 0;

    while (targets)
    {
        // 2. Create and build these strikes
        mis->target_num = targetf[tar];
        mis->caps or_eq MissionData[mis->mission].caps;

        if ( not mis->aircraft)
            mis->aircraft = MissionData[mis->mission].str;

        flight = AttachFlight(mis, this);

        if (flight)
        {
            mis->tx = mis_request.tx;
            mis->ty = mis_request.ty;

            if (MissionData[mis->mission].loitertime)
                mis->tot = mis_request.tot + MissionData[mis->mission].loitertime * flights * CampaignMinutes;
            else
                mis->tot = mis_request.tot + (CampaignTime)((flights) * 10 * CampaignSeconds);

            result = flight->BuildMission(mis);

            if (result == PRET_SUCCESS)
            {
                targetd = RecordFlightAddition(flight, mis, targetd);
                flight->SetTarget(0);
            }
            else
            {
                CancelFlight((Flight)flight);

                if (result == PRET_ABORTED and not flights and (ls or hs))
                {
                    // This flight couldn't get to target
                    return PRET_ABORTED;
                }

                if (result == PRET_CANCELED and not flights)
                {
                    // This flight probably could arm itself or get to the target -
                    // If we can disable us from finding the squadron next time through,
                    // that'd be ideal, otherwise we need to cancel
                    return PRET_CANCELED;
                }
            }
        }

        // If we're gonna add a FAC, do it now
        // This is kinda annoying, because we have to fuck with the flights counter, etc..
        // But if I need to convert my ONCALLCAS to a PRPLANCAS I only want to have to
        // convert one.
        if (flights == 1 and (package_flags bitand AMIS_ADDFAC))
        {
            // Add FAC directly
            newmis = *mis;
            newmis.mission = AMIS_FAC;
            newmis.target_num = 255;
            newmis.targetID = FalconNullId;
            newmis.tot = mis_request.tot - 5 * CampaignMinutes;
            newmis.caps or_eq MissionData[newmis.mission].caps;
            newmis.aircraft = MissionData[newmis.mission].str;
            newmis.flags = REQF_USERESERVES;
            newmis.priority = 0;
            newmis.priority = GetPriority(&newmis);
            add_at_end = AttachFlight(&newmis, this);

            if (add_at_end)
            {
                if (add_at_end->BuildMission(&newmis) not_eq PRET_SUCCESS)
                {
                    CancelFlight((Flight)add_at_end);
                    add_at_end = NULL;
                }
                else
                {
                    RemoveChild(add_at_end->Id()); // Play games to get the ordering right
                }
            }
            else
            {
                // No FAC available - we need to convert our ONCALLCAS to PREPLANCAS or cancel
                flight = (Flight) GetFirstUnitElement();
                mis->tx = mis_request.tx;
                mis->ty = mis_request.ty;
                mis_request.mission = mis->mission = AMIS_PRPLANCAS;

                if (flight->BuildMission(mis) not_eq PRET_SUCCESS)
                {
                    CancelFlight((Flight)flight);
                    DeleteWPList(ingress);
                    ingress = NULL;
                    DeleteWPList(egress);
                    egress = NULL;
                    DisposeWayPoints();
                    return PRET_CANCELED;
                }

                targets = 1;
            }
        }

        targets--;
        tar++;
    }

    // Correct for any weirded out target locations (ie: ONCALLCAS missions)
    mis->tx = mis_request.tx;
    mis->ty = mis_request.ty;

    if (add_at_end)
    {
        AddUnitChild(add_at_end);
        flights++;
    }

    if ( not flights or not flight)
        return PRET_NO_ASSETS;

#ifdef KEV_ADEBUG
    MonoPrint("Building team #%d %s mission (%d) at %d,%d\n", mis->who, MissStr[mis_request.mission], mis_request.mission, mis->tx, mis->ty);
#endif
    //TJL 11/13/03 Scene of a repeated CTD
    /*
       if (flight->IsHelicopter())
       {
    // Helocopter flights don't make any requests or ask for support
    return PRET_SUCCESS;
    }
     */


    // 3. Trigger required mission requests
    if ((package_flags bitand AMIS_ADDSEAD) and (targetd bitand NEED_SEAD))
    {
        // Add SEAD directly
        newmis = *mis;
        newmis.mission = AMIS_SEADESCORT;
        newmis.targetID = element[0];
        newmis.target_num = 255;
        newmis.tot = mis_request.tot;
        newmis.caps or_eq MissionData[newmis.mission].caps;
        // RV - Biker - Limit size of escort to size of main flight
        //newmis.aircraft = MissionData[newmis.mission].str;
        newmis.aircraft = min(MissionData[newmis.mission].str, mis->aircraft);
        newmis.priority = 0;
        newmis.priority = GetPriority(&newmis);
        // newmis.flags = REQF_USERESERVES;
        flight = AttachFlight(&newmis, this);
#ifdef DEBUG
        gSEADrequested++;

        if (flight)
            gSEADgot++;

#endif

        if (flight)
        {
            newmis.tot = mis_request.tot + (CampaignTime)(MissionData[newmis.mission].separation) * CampaignSeconds;

            if (flight->BuildMission(&newmis) == PRET_SUCCESS)
            {
                targetd = RecordFlightAddition(flight, mis, targetd);
                flight->SetUnitMissionTarget(GetMainFlightID());
            }
            else
            {
                CancelFlight((Flight)flight);
            }
        }

#ifdef KEV_ADEBUG
        else
            MonoPrint("Failed to find SEAD Escort\n");

#endif
    }

    // Marco edit - We want Escort regardless of threat
    if ((package_flags bitand AMIS_ADDESCORT))  // and mis->vs and TeamInfo[mis->vs]->atm->averageCAStrength > aa_strength)
    {
        // Add ESCORT directly
        newmis = *mis;

        // RV - Biker - If we have no mission type give 50% sead escort 50% fighter escort
        if (escort_type not_eq AMIS_NONE)
            newmis.mission = escort_type; // 2001-11-10 Modified by M.N. use requested escort type
        else
        {
            if (rand() bitand 1)
            {
                newmis.mission = AMIS_SEADESCORT;
            }
            else
            {
                newmis.mission = AMIS_ESCORT;
            }
        }

        newmis.targetID = element[0];
        newmis.target_num = 255;
        newmis.tot = mis_request.tot;
        newmis.caps or_eq MissionData[newmis.mission].caps;
        newmis.aircraft = MissionData[newmis.mission].str;
        newmis.priority = GetPriority(&newmis);
        // newmis.flags = REQF_USERESERVES;
        flight = AttachFlight(&newmis, this);

        if (flight)
        {
            newmis.tot = mis_request.tot + (CampaignTime)(MissionData[newmis.mission].separation) * CampaignSeconds;

            if (flight->BuildMission(&newmis) == PRET_SUCCESS)
            {
                targetd = RecordFlightAddition(flight, mis, targetd);
                flight->SetUnitMissionTarget(GetMainFlightID());
            }
            else
            {
                CancelFlight((Flight)flight);
            }
        }
    }


    if (package_flags bitand AMIS_ADDBARCAP)
    {
        // Request and enemy BARCAP mission
        newmis.requesterID = Id();
        newmis.targetID = GetMainFlightID();
        newmis.vs = GetTeam();
        newmis.tot = mis_request.tot - 240 + (rand() % 120 * CampaignSeconds);
        newmis.tot_type = TYPE_EQ;
        newmis.tx = mis->tx;
        newmis.ty = mis->ty;
        newmis.match_strength = aa_strength;
        newmis.mission = AMIS_BARCAP;
        newmis.roe_check = ROE_AIR_ENGAGE;
        newmis.priority = mis->priority / 5;

        if (MissionData[mis_request.mission].skill == ARO_GA)
            newmis.context = enemyCASAircraftPresent;
        else
            newmis.context = enemyStrikesExpected;

        newmis.flags = REQF_NEEDRESPONSE bitor REQF_ONETRY;

        if (mis->flags bitand REQF_PART_OF_ACTION)
            newmis.flags or_eq REQF_PART_OF_ACTION;

        newmis.action_type = mis->action_type;
        newmis.aircraft = 2 * flights;

        if (newmis.aircraft > 4)
            newmis.aircraft = 4;

        newmis.RequestEnemyMission();
        wait_for or_eq AMIS_ADDBARCAP;
        wait_cycles = (uchar)PACKAGE_CYCLES_TO_WAIT;
#ifdef DEBUG
        gBARCAPrequested++;
#endif
    }

    if (package_flags bitand AMIS_ADDSWEEP)
    {
        // Request an enemy Sweep mission
        newmis.requesterID = Id();
        newmis.targetID = GetMainFlightID();
        newmis.vs = GetTeam();
        newmis.tot = mis_request.tot - 60 + (rand() % 120 * CampaignSeconds);
        newmis.tot_type = TYPE_LE;
        newmis.tx = mis->tx;
        newmis.ty = mis->ty;
        newmis.match_strength = aa_strength;
        newmis.mission = AMIS_SWEEP;
        newmis.priority = 0;
        newmis.roe_check = ROE_AIR_ENGAGE;

        if (MissionData[mis_request.mission].skill == ARO_GA or MissionData[mis_request.mission].skill == ARO_REC)
            newmis.context = enemyCASAircraftPresent;
        else if (MissionData[mis_request.mission].skill == ARO_ASW)
            newmis.context = enemyAircraftPresent;
        else
            newmis.context = enemySupportAircraftPresent;

        if (mis->flags bitand REQF_PART_OF_ACTION)
            newmis.flags or_eq REQF_PART_OF_ACTION;

        newmis.action_type = mis->action_type;
        newmis.flags = REQF_NEEDRESPONSE bitor REQF_ONETRY;
        newmis.aircraft = 2 * flights;

        if (newmis.aircraft > 4)
            newmis.aircraft = 4;

        newmis.RequestEnemyMission();
        wait_for or_eq AMIS_ADDSWEEP;
        wait_cycles = (uchar)PACKAGE_CYCLES_TO_WAIT;
#ifdef DEBUG
        gSWEEPrequested++;
#endif
    }


    FindSupportFlights(mis, targetd);

#ifdef DEBUG

    if (mis->mission == AMIS_BARCAP)
        gBARCAPgot++;

    if (mis->mission == AMIS_SWEEP)
        gSWEEPgot++;

#endif
    return PRET_SUCCESS;
}

// This records important data from the flight passed into the package's structures
int PackageClass::RecordFlightAddition(Flight flight, MissionRequest mis, int targetd)
{
    flights++;
    aa_strength += GetUnitScore(flight, Air);

    if (flights == 1)
    {
        WayPoint w, aw = NULL, bw = NULL, tw = NULL, eaw = NULL, lw = NULL;
        int intarget = 0;

        // Do some stuff for the first flight only
        mis->speed = flight->GetCombatSpeed();
        mis_request.speed = mis->speed;
        mis_request.tot = flight->GetUnitTOT();

        // If we can add SEAD, and havn't already decided to, check if we need to
        if ((package_flags bitand AMIS_ADDSEAD) and not (targetd bitand NEED_SEAD))
            targetd or_eq CheckPathThreats(flight);
        // If we can add ECM, check if we need to
        else if (package_flags bitand AMIS_ADDECM)
            targetd or_eq CheckPathThreats(flight);

        if ( not (MissionData[mis->mission].flags bitand AMIS_TARGET_ONLY))
        {
            // Find assembly points
            w = flight->GetFirstUnitWP();

            while (w)
            {
                if (w->GetWPFlags() bitand WPF_ASSEMBLE and not aw)
                    aw = w;

                if (w->GetWPFlags() bitand WPF_BREAKPOINT)
                    bw = w;

                if (w->GetWPFlags() bitand WPF_TARGET or w->GetWPFlags() bitand WPF_IP or w->GetWPFlags() bitand WPF_CP or /* w->GetWPFlags() bitand WPF_TURNPOINT or */ w->GetWPFlags() bitand WPF_REPEAT)
                {
                    if ( not bw)
                        bw = lw;

                    if (lw->GetWPFlags() bitand WPF_TARGET or lw->GetWPFlags() bitand WPF_CP)
                        tw = w; // Just passed the target waypoint

                    intarget = 1;
                }
                else if (intarget) // and not tw)
                {
                    tw = w; // Just passed the target waypoint
                    intarget = 0;
                }
                else
                    intarget = 0;

                if (w->GetWPFlags() bitand WPF_ASSEMBLE)
                    eaw = w;

                lw = w;
                w = w->GetNextWP();
            }

            if ( not bw and MissionData[mis->mission].flags bitand AMIS_NO_BREAKPT)
                bw = tw;

            ShiAssert(bw and tw and aw and eaw)
            // copy ingress and egress paths to package
            w = bw->GetNextWP();
            ingress =  CloneWPToList(aw, w);
            w = eaw->GetNextWP();
            egress =  CloneWPToList(tw, w);
        }
    }

    return targetd;
}

void PackageClass::FindSupportFlights(MissionRequest mis, int targetd)
{
    MissionRequestClass newmis;

    if ( not (package_flags bitand AMIS_SUPPORT_MASK))
        return;

    if (TeamInfo[GetTeam()] and TeamInfo[GetTeam()]->atm and TeamInfo[GetTeam()]->atm->packageList)
    {
        // Look for any existing missions which can help us out.
        Package pack;
        Flight flight;
        MissionRequest pmis;
        GridIndex px, py;
        float dist;
        float bestAWACSDist = (float)Map_Max_X * Map_Max_Y; // Reasonable maximum distance squared
        float bestJSTARDist = (float)Map_Max_X * Map_Max_Y; // Reasonable maximum distance squared
        float bestTANKDist = (float)Map_Max_X * Map_Max_Y; // Reasonable maximum distance squared
        float bestECMDist = (float)Map_Max_X * Map_Max_Y; // Reasonable maximum distance squared
        CampaignTime startTime, endTime;

        startTime = mis->tot - 10 * CampaignMinutes;
        endTime = mis->tot + (MissionData[mis->mission].loitertime + 10) * CampaignMinutes;
        VuListIterator packit(TeamInfo[GetTeam()]->atm->packageList);
        pack = (Package) GetFirstUnit(&packit);

        while (pack)
        {
            pmis = pack->GetMissionRequest();
            flight = (FlightClass*) pack->GetFirstUnitElement();

            if (flight and not flight->Aborted() and pmis->tot < startTime)
            {
                if ((package_flags bitand AMIS_ADDAWACS) and flight->GetUnitMission() == AMIS_AWACS)
                {
                    pack->GetUnitDestination(&px, &py);
                    dist = (float)DistSqu(mis->tx, mis->ty, px, py);

                    if (pmis->tot + MissionData[AMIS_AWACS].loitertime * CampaignMinutes > endTime and dist < bestAWACSDist)
                    {
                        awacs = flight->Id();
                        bestAWACSDist = dist;
                    }
                }

                if ((package_flags bitand AMIS_ADDJSTAR) and flight->GetUnitMission() == AMIS_JSTAR)
                {
                    pack->GetUnitDestination(&px, &py);
                    dist = (float)DistSqu(mis->tx, mis->ty, px, py);

                    if (pmis->tot + MissionData[AMIS_JSTAR].loitertime * CampaignMinutes > endTime and dist < bestJSTARDist)
                    {
                        jstar = flight->Id();
                        bestJSTARDist = dist;
                    }
                }

                if ((package_flags bitand AMIS_ADDECM) and (targetd bitand NEED_ECM) and flight->GetUnitMission() == AMIS_ECM)
                {
                    pack->GetUnitDestination(&px, &py);
                    dist = (float)DistSqu(mis->tx, mis->ty, px, py);

                    if (pmis->tot + MissionData[AMIS_ECM].loitertime * CampaignMinutes > endTime and dist < bestECMDist)
                    {
                        ecm = flight->Id();
                        bestECMDist = dist;
                    }
                }

                if ((package_flags bitand AMIS_ADDTANKER) and flight->GetUnitMission() == AMIS_TANKER)
                {
                    GridIndex x, y;
                    pack->GetUnitDestination(&px, &py);
                    GetUnitAssemblyPoint(0, &x, &y);

                    if (x and y)
                        dist = (float)DistSqu(x, y, px, py);
                    else
                        dist = (float)DistSqu(mis->tx, mis->ty, px, py);

                    if (pmis->tot + MissionData[AMIS_TANKER].loitertime * CampaignMinutes > endTime and dist < bestTANKDist)
                    {
                        tanker = flight->Id();
                        bestTANKDist = dist;
                    }
                }
            }

            pack = (Package) GetNextUnit(&packit);
        }
    }

    // Now request anything we couldn't find
    if ((package_flags bitand AMIS_ADDAWACS) and awacs == FalconNullId)
    {
        newmis.requesterID = Id();
        newmis.targetID = FalconNullId;
        newmis.who = GetTeam();
        newmis.vs = 0;
        newmis.tot = mis_request.tot;
        newmis.tot_type = TYPE_LE;
        newmis.tx = mis->tx;
        newmis.ty = mis->ty;
        newmis.mission = AMIS_AWACS;
        newmis.roe_check = ROE_AIR_OVERFLY;
        newmis.context = friendlyAWACSNeeded;
        newmis.aircraft = 0;
        newmis.priority = 0;
        newmis.flags = REQF_NEEDRESPONSE;
        newmis.RequestMission();
    }

    if ((package_flags bitand AMIS_ADDJSTAR) and jstar == FalconNullId)
    {
        newmis.requesterID = Id();
        newmis.targetID = FalconNullId;
        newmis.who = GetTeam();
        newmis.vs = 0;
        newmis.tot = mis_request.tot;
        newmis.tot_type = TYPE_LE;
        newmis.tx = mis->tx;
        newmis.ty = mis->ty;
        newmis.mission = AMIS_JSTAR;
        newmis.roe_check = ROE_AIR_OVERFLY;
        newmis.context = enemyGroundForcesPresent;
        newmis.aircraft = 0;
        newmis.priority = 0;
        newmis.flags = REQF_NEEDRESPONSE;
        newmis.RequestMission();
    }

    // 4. Trigger conditional mission requests.
    if ((package_flags bitand AMIS_ADDECM) and (targetd bitand NEED_ECM) and ecm == FalconNullId)
    {
        newmis.requesterID = Id();
        newmis.targetID = FalconNullId;
        newmis.who = GetTeam();
        newmis.vs = 0;
        newmis.tot = mis_request.tot;
        newmis.tot_type = TYPE_EQ;
        newmis.tx = mis->tx;
        newmis.ty = mis->ty;
        newmis.mission = AMIS_ECM;
        newmis.roe_check = ROE_AIR_OVERFLY;
        newmis.context = enemyRadarPresent;
        newmis.aircraft = 0;
        newmis.priority = 0;
        newmis.flags = REQF_NEEDRESPONSE;
        newmis.RequestMission();
    }

    if ((package_flags bitand AMIS_ADDTANKER) and tanker == FalconNullId)
    {
        GridIndex x, y;
        // Make sure some tankers are going to be here - later only ask for it if we'll be
        // short on fuel.
        newmis.requesterID = Id();
        newmis.targetID = FalconNullId;
        newmis.who = GetTeam();
        newmis.vs = 0;
        newmis.tot = mis_request.tot;
        newmis.tot_type = TYPE_LE;
        newmis.tx = mis->tx;
        newmis.ty = mis->ty;
        GetUnitAssemblyPoint(0, &x, &y);

        if (x and y)
        {
            newmis.tx = x;
            newmis.ty = y;
        }

        newmis.mission = AMIS_TANKER;
        newmis.roe_check = ROE_AIR_OVERFLY;
        newmis.context = friendlyAssetsRefueling;
        newmis.priority = 50; // We REALLY need this tanker
        newmis.aircraft = 0;
        newmis.flags = REQF_NEEDRESPONSE bitor REQF_ONETRY;
        newmis.RequestMission();
        wait_for or_eq AMIS_ADDTANKER;
        wait_cycles = (uchar)PACKAGE_CYCLES_TO_WAIT;
    }
}

void PackageClass::HandleRequestReceipt(int type, int them, VU_ID triggered_flight)
{
    MissionRequestClass mis;
    Unit flight, enemy;

    if ( not wait_cycles)
        return; // Error of sorts - response mission was planned after our wait period timed out

    switch (type)
    {
        case AMIS_BARCAP:
        case AMIS_SWEEP:
            wait_for or_eq AMIS_BARCAP bitor AMIS_SWEEP;
            wait_for xor_eq AMIS_BARCAP bitor AMIS_SWEEP;
            responses or_eq PRESPONSE_CA;
            interceptor = triggered_flight;
            enemy = FindUnit(triggered_flight);

            if ( not enemy)
                return;

            // RV - Biker - If we have no mission type return
            if (MissionData[mis_request.mission].escorttype == AMIS_NONE)
                return;
            else
                mis.mission = MissionData[mis_request.mission].escorttype;

            mis.who = GetTeam();
            mis.vs = them;
            mis.target_num = 255;
            mis.aircraft = MissionData[mis.mission].str;
            // skip this if our current package has enough air strength
            mis.match_strength = GetUnitScore(enemy, Air);

            // mis.match_strength = enemy->GetUnitRoleScore(ARO_CA, CALC_TOTAL, USE_EXP bitor USE_VEH_COUNT);
            if (aa_strength > mis.match_strength)
                return;

            mis.match_strength -= aa_strength; // we only want to match the excess
            mis.tot = mis_request.tot + MissionData[mis.mission].separation * CampaignSeconds;
            mis.tot_type = TYPE_EQ;
            mis.roe_check = ROE_AIR_ENGAGE;
            mis.priority = (mis.match_strength) / 2;

            if (mis_request.flags bitand REQF_PART_OF_ACTION)
            {
                mis.flags = REQF_PART_OF_ACTION;
                mis.action_type = mis_request.action_type;
            }

            mis.targetID = triggered_flight;

            // Mission location is assembly point initially (if we have one)
            // So we find the closest squadron to the assembly point
            if (iax > 0 and iay > 0)
            {
                mis.tx = iax;
                mis.ty = iay;
            }
            else
                GetUnitDestination(&mis.tx, &mis.ty);

            if (mis.mission == AMIS_ESCORT or mis.mission == AMIS_HAVCAP)
            {
                // Add a new flight to this package
                mis.caps = caps bitor MissionData[mis.mission].caps;
                mis.speed = mis_request.speed;
                mis.targetID = element[0];
                mis.priority = GetPriority(&mis);
                flight = AttachFlight(&mis, this);
#ifdef DEBUG
                gESCORTrequested++;

                if (flight)
                    gESCORTgot++;

#endif

                if (flight)
                {
                    // Now set the mission destination correctly
                    GetUnitDestination(&mis.tx, &mis.ty);

                    if (flight->BuildMission(&mis) == PRET_SUCCESS)
                    {
                        RecordFlightAddition((Flight)flight, &mis, 0);
                        flight->SetUnitMissionTarget(GetMainFlightID());
                    }
                    else
                    {
                        CancelFlight((Flight)flight);
                    }
                }

#ifdef KEV_ADEBUG
                else
                    MonoPrint("Failed to find CA Escort\n");

#endif
            }
            else if (mis.mission > 0)
            {
                // Request a new package (Sweep or RESCAP mostly)
                if (mis.mission == AMIS_SWEEP and mis_request.mission >= AMIS_FAC and mis_request.mission <= AMIS_CAS)
                    mis.context = friendlyCASExpected;
                else if (mis.mission == AMIS_RESCAP)
                    mis.context = friendlyRescueExpected;
                else
                    mis.context = friendlyAssetsExpected;

                mis.requesterID = Id();
                mis.RequestMission();
            }

            break;

        case AMIS_TANKER:
            responses or_eq PRESPONSE_TANKER;
            tanker = triggered_flight;
            break;

        case AMIS_AWACS:
            responses or_eq PRESPONSE_AWACS;
            awacs = triggered_flight;
            break;

        case AMIS_JSTAR:
            responses or_eq PRESPONSE_JSTAR;
            jstar = triggered_flight;
            break;

        case AMIS_ECM:
            responses or_eq PRESPONSE_ECM;
            ecm = triggered_flight;
            break;

        default:
            break;
    }
}

void PackageClass::SetUnitAssemblyPoint(int type, GridIndex x, GridIndex y)
{
    switch (type)
    {
        case 0:
            iax = x;
            iay = y;
            break;

        case 1:
            eax = x;
            eay = y;
            break;

        case 2:
            bpx = x;
            bpy = y;
            break;

        case 3:
            tpx = x;
            tpy = y;
            break;
    }
}

void PackageClass::GetUnitAssemblyPoint(int type, GridIndex *x, GridIndex *y)
{
    switch (type)
    {
        case 0:
            *x = iax;
            *y = iay;
            break;

        case 1:
            *x = eay;
            *y = eay;
            break;

        case 2:
            *x = bpy;
            *y = bpy;
            break;

        case 3:
            *x = tpx;
            *y = tpy;
            break;
    }
}

void PackageClass::CancelFlight(Flight flight)
{
    flight->SetDead(1);
    RemoveChild(flight->Id());
    flight->Remove();
    /* sfr: flight will be removed anyway, even if not in DB, since the remove call
    will ref unref it.
    // JPO - another memory leak - lets assume if its not in the database then it s not been added
    if(VuState() < VU_MEM_ACTIVE or flight->Remove() == VU_NO_OP)
    {
     // Never inserted - let vu do the deletion.
     VuReferenceEntity(flight);
     VuDeReferenceEntity(flight);
    }
    */
}

Unit PackageClass::GetFirstUnitElement(void)
{
    c_element = 0;
    return FindUnit(element[c_element]);
}

Unit PackageClass::GetNextUnitElement(void)
{
    c_element++;

    while (c_element < elements)
    {
        if (element[c_element])
            return FindUnit(element[c_element]);

        c_element++;
    }

    c_element = 0;
    return NULL;
}

Unit PackageClass::GetUnitElement(int en)
{
    if (en < elements and element[en])
        return (Unit)vuDatabase->Find(element[en]);

    return NULL;
}

Unit PackageClass::GetUnitElementByID(int eid)
{
    Unit e = NULL;

    if (eid < elements)
        e = (Unit)vuDatabase->Find(element[eid]);

    return e;
}

/* int i=0;

 while (i < elements)
 {
 if (element[i])
 {
 e = (Unit)vuDatabase->Find(element[i]);
 if (e and e->GetUnitElement() == eid)
 return  e;
 }
 i++;
 }
 return NULL;
 }
 */

void PackageClass::AddUnitChild(Unit e)
{
    int i = 0;

    while (element[i] and i < MAX_UNIT_CHILDREN)
    {
        if (element[i] == e->Id())
            return;

        i++;
    }

    if (i < MAX_UNIT_CHILDREN)
        element[i] = e->Id();

    if (i >= elements)
        elements = i + 1;
}

void PackageClass::DisposeChildren(void)
{
    Unit e;
    int i = elements - 1;

    while (i >= 0)
    {
        if (element[i])
        {
            e = (Unit)vuDatabase->Find(element[i]);

            if (e)
                e->KillUnit();

            element[i] = FalconNullId;
            elements--;
        }

        i--;
    }
}

void PackageClass::RemoveChild(VU_ID eid)
{
    int i = 0, j;

    while (i < elements)
    {
        if (element[i] == eid)
        {
            for (j = i; j < MAX_UNIT_CHILDREN - 1; j++)
                element[j] = element[j + 1];

            element[j] = FalconNullId;
            elements--;
        }
        else
            i++;
    }
}

VU_ID PackageClass::GetMainFlightID(void)
{
    Unit e;

    e = GetFirstUnitElement();

    if (e and e->IsFlight() and e->GetUnitMissionID() == 0)
        return e->Id();

    return FalconNullId;
}

Flight PackageClass::GetMainFlight(void)
{
    Unit e;

    e = GetFirstUnitElement();

    if (e and e->IsFlight() and e->GetUnitMissionID() == 0)
        return (Flight)e;

    return NULL;
}

// ================================
// Support Functions
// ================================

// Transfer a flight from it's current package to the one passed
void TransferFlight(Flight flight, Package pack)
{
    Package oldp;

    oldp = (Package)flight->GetUnitParent();

    if (oldp)
        oldp->RemoveChild(flight->Id());

    flight->SetUnitParent(pack);
}

// Creates a new flight, adds aircraft and attaches to the package.
Flight AttachFlight(MissionRequest mis, Package pack)
{
    Squadron squadron;
    Flight flight;
    short tid;
    GridIndex bx, by, x, y;

    if ( not TeamInfo[mis->who]->atm)
    {
#ifdef KEV_DEBUG
        MonoPrint("Error, I don't own the ATM\n");
#endif
        return NULL;
    }

    if ( not pack)
        return NULL;

    pack->GetLocation(&bx, &by);

    if (mis->flags bitand AMIS_IMMEDIATE)
    {
        flight = TeamInfo[mis->who]->atm->FindBestAirFlight(mis); // We steal the aircraft from a current flight (all the aircraft)

        if ( not flight)
            return NULL;

        mis->aircraft = flight->GetTotalVehicles();

        // Ok, this package is a go - add it locally
        if ( not PackInserted)
        {
            // sfr: only say its inserted if its really inserted
            pack->SetSendCreate(VuEntity::VU_SC_DONT_SEND);
            PackInserted = (vuDatabase->/*Silent*/Insert(pack) == VU_SUCCESS);
        }

        // Transfer the flight to the new parent
        TransferFlight((Flight)flight, pack);
    }
    else
    {
        squadron = TeamInfo[mis->who]->atm->FindBestAir(mis, bx, by);

        if ( not squadron)
            return NULL;

        if ( not bx and not by) // Call this our home base for this package set
        {
            squadron->GetLocation(&bx, &by);
            pack->SetLocation(bx, by);
        }

        // Ok, this package is a go - add it locally
        if ( not PackInserted)
        {
            // sfr: only say its inserted if its really inserted
            pack->SetSendCreate(VuEntity::VU_SC_DONT_SEND);
            PackInserted = (vuDatabase->/*Silent*/Insert(pack) == VU_SUCCESS);
        }

        tid = GetClassID(DOMAIN_AIR, CLASS_UNIT, TYPE_FLIGHT, squadron->GetSType(), squadron->GetSPType(), 0, 0, 0);
#ifdef DEBUG
        ShiAssert(tid);

        if ( not tid)
            return NULL;

#endif
        tid += VU_LAST_ENTITY_TYPE;
        flight = NewFlight(tid, pack, squadron);

        if ( not flight)
            return NULL;

        flight->SetOwner(squadron->GetOwner());
        squadron->GetLocation(&x, &y);
        flight->SetLocation(x, y);
        pack->GetUnitDestination(&x, &y);
        flight->SetUnitDestination(x, y);
        flight->SetFinal(0);

        // If we've got a bomber assigned to a lead strike role, switch the mission type to strat
        // bomb so that everything will be planned correctly
        if ( not pack->GetFlights() and 
            MissionData[mis->mission].skill == ARO_S and 
            squadron->GetRating(ARO_SB) > squadron->GetRating(ARO_S) and 
            // RV - Biker - Don't convert all to STRATBOMB
            rand() % 100 > 25)
        {
            mis->mission = AMIS_STRATBOMB;
            pack->GetMissionRequest()->mission = AMIS_STRATBOMB;
        }
    }

    flight->SetUnitMissionID(pack->GetFlights());
#ifdef KEV_ADEBUG
    MonoPrint("Attaching flight %d to package %d\n", flight->GetCampID(), pack->GetCampID());
#endif
    return flight;
}

// Strip or rename unneeded waypoints.
void FinalizeFlight(Unit flight, int flights)
{
    int assem;
    WayPoint w;

#ifdef KEV_ADEBUG
    Unit pack;
    pack = flight->GetUnitParent();
    MonoPrint("Finalizing flight %d, package %d\n", flight->GetCampID(), pack->GetCampID());
#endif

#if 0
#ifdef DEBUG

    if (flight->GetTotalVehicles() <= PILOTS_PER_FLIGHT) // and (MissionData[mis->mission].flags bitand AMIS_DONT_USE_AC))
    {
        // Verify a few things are correct for this flight
        int acp, acr = flight->GetTotalVehicles();
        acp = 0;

        for (int i = 0; i < PILOTS_PER_FLIGHT; i++)
        {
            if (((FlightClass*)flight)->plane_stats[i] == AIRCRAFT_AVAILABLE)
                acp++;
        }

        ShiAssert(acp == acr);
    }

#endif
#endif

    if (flights > 1 and not (MissionData[flight->GetUnitMission()].flags bitand AMIS_DONT_COORD) and not (MissionData[flight->GetUnitMission()].flags bitand AMIS_TARGET_ONLY))
        assem = 1;
    else
        assem = 0;

    w = flight->GetFirstUnitWP();

    while (w)
    {
        if (w->GetWPFlags() bitand WPF_BREAKPOINT and not assem)
            w->SetWPFlags(w->GetWPFlags() xor WPF_BREAKPOINT); // Clear our flag

        if (w->GetWPFlags() bitand WPF_ASSEMBLE and not assem)
        {
            w->SetWPAction(WP_NOTHING); // Clear our action
            w->SetWPRouteAction(MissionData[flight->GetUnitMission()].routewp);
            w->SetWPFlags(w->GetWPFlags() xor WPF_ASSEMBLE); // Clear our flag
        }

        if (w->GetWPAction() == WP_NOTHING and not (w->GetWPFlags() bitand 0x4FF))
        {
            // This is an unused waypoint (Nothing action and no flags set)
            // Check if in line or co-existant with other wps, if so, remove
            // KCK NOTE: I'll leave it if it'll cut a large leg in half.
            WayPoint pw, nw;
            GridIndex x, y, px, py, nx, ny;
            float d1, d2, d3;

            pw = w->GetPrevWP();
            nw = w->GetNextWP();

            if (pw and nw)
            {
                w->GetWPLocation(&x, &y);
                pw->GetWPLocation(&px, &py);
                nw->GetWPLocation(&nx, &ny);

                // Check if waypoint is inline with other waypoints
                if (Distance(px, py, nx, ny) < 50)
                {
                    d1 = AngleTo(px, py, x, y);
                    d2 = AngleTo(x, y, nx, ny);
                    d3 = d2 - d1;

                    if (d3 < 0.1F and d3 > -0.1F)
                    {
                        w->DeleteWP();
                        w = pw;
                    }
                }

                // Check if waypoint is co-located with adjacent waypoints
                if (w not_eq pw and ((px == x and py == y) or (x == nx and y == ny)))
                {
                    w->DeleteWP();
                    w = pw;
                }
            }
        }

        w = w->GetNextWP();
    }

    flight->SetFinal(1);
}

// This sends a message to the ATM to remove the package's mission from it's planned list.
// Also - calculate player debrief stuff for player packages.
/*
   void CompleteMission(Package pack)
   {
   if (pack->requestID >= 0)
   TeamInfo[pack->GetTeam()]->atm->SendATMMessage(pack->Id(), pack->GetTeam(), FalconAirTaskingMessage::atmCompleteMission, pack->requestID, 0, NULL, 0);
   }
 */

int CallInOCAStrikes(MissionRequest mis)
{
    FalconPrivateList list(&CampFilter);
    short ts = 0;

    if (mis->mission == AMIS_SEADSTRIKE) // Don't generate recursive SEAD missions
        return 0;

    // Trigger OCA missions
    CollectThreatsFast(mis->tx, mis->ty, 1, mis->who, FIND_NOAIR bitor FIND_NOMOVERS bitor FIND_FINDUNSPOTTED bitor FIND_NODETECT, &list);
    CollectThreatsFast(mis->tx, mis->ty, 99, mis->who, FIND_NOAIR bitor FIND_NOMOVERS bitor FIND_FINDUNSPOTTED bitor FIND_NODETECT, &list);
    TargetThreats(mis->who, mis->priority, &list, LowAir, mis->tot, AMIS_ADDOCASTRIKE, &ts);
    return ts;
}

Flight PackageClass::GetFACFlight(void)
{
    Flight fac;

    if (MissionData[mis_request.mission].skill not_eq ARO_GA)
        return NULL;

    fac = (Flight) GetFirstUnitElement();

    while (fac)
    {
        if (fac->GetUnitMission() == AMIS_FAC)
            return fac;

        fac = (Flight) GetNextUnitElement();
    }

    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void PackageClass::SetTanker(VU_ID t)
{
    tanker = t;

    MakePackageDirty(DIRTY_TANKER, DDP[116].priority);
    // MakePackageDirty (DIRTY_TANKER, SEND_EVENTUALLY);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void PackageClass::SetPackageFlags(ulong f)
{
    package_flags or_eq f;

    MakePackageDirty(DIRTY_PACKAGE_FLAGS, DDP[117].priority);
    // MakePackageDirty (DIRTY_PACKAGE_FLAGS, SEND_EVENTUALLY);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void PackageClass::SetTakeoff(CampaignTime t)
{
    takeoff = t;

    MakePackageDirty(DIRTY_TAKEOFF, DDP[118].priority);
    // MakePackageDirty (DIRTY_TAKEOFF, SEND_EVENTUALLY);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void PackageClass::SetTPTime(CampaignTime t)
{
    tp_time = t;

    MakePackageDirty(DIRTY_TP_TIME, DDP[119].priority);
    // MakePackageDirty (DIRTY_TP_TIME, SEND_EVENTUALLY);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void PackageClass::MakePackageDirty(Dirty_Package bits, Dirtyness score)
{
    if (( not IsLocal()) or (VuState() not_eq VU_MEM_ACTIVE))
    {
        return;
    }

    if ( not IsAggregate() and (score not_eq SEND_RELIABLEANDOOB))
    {
        score = static_cast<Dirtyness>(score << 4);
    }

    dirty_package or_eq bits;

    MakeDirty(DIRTY_PACKAGE, score);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void PackageClass::WriteDirty(unsigned char **stream)
{
    //MonoPrint ("  PC %08x", dirty_package);

    *(uchar *)*stream = dirty_package;
    *stream += sizeof(uchar);

    if (dirty_package bitand DIRTY_TANKER)
    {
        *(VU_ID*)*stream = tanker;
        *stream += sizeof(VU_ID);
    }

    if (dirty_package bitand DIRTY_PACKAGE_FLAGS)
    {
        *(ulong*)*stream = package_flags;
        *stream += sizeof(ulong);
    }

    if (dirty_package bitand DIRTY_TAKEOFF)
    {
        *(CampaignTime*)*stream = takeoff;
        *stream += sizeof(CampaignTime);
    }

    if (dirty_package bitand DIRTY_TP_TIME)
    {
        *(CampaignTime*)*stream = tp_time;
        *stream += sizeof(CampaignTime);
    }

    dirty_package = 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//sfr: added rem and checks
void PackageClass::ReadDirty(VU_BYTE **stream, long *rem)
{
    uchar bits;

    //read bitfield
    memcpychk(&bits, stream, sizeof(uchar), rem);

    if (bits bitand DIRTY_TANKER)
    {
        memcpychk(&tanker, stream, sizeof(VU_ID), rem);
    }

    if (bits bitand DIRTY_PACKAGE_FLAGS)
    {
        memcpychk(&package_flags, stream, sizeof(ulong), rem);
    }

    if (bits bitand DIRTY_TAKEOFF)
    {
        memcpychk(&takeoff, stream, sizeof(CampaignTime), rem);
    }

    if (bits bitand DIRTY_TP_TIME)
    {
        memcpychk(&tp_time, stream, sizeof(CampaignTime), rem);
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


