//
// ATM.cpp
//
// Air Tasking Manager routines
//
// Kevin Klemmick
// ===============================================================

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>
#include "campmap.h"
#include "CmpGlobl.h"
#include "ListADT.h"
#include "vutypes.h"
#include "Objectiv.h"
#include "Find.h"
#include "F4Vu.h"
#include "strategy.h"
#include "Path.h"
#include "ASearch.h"
#include "Campaign.h"
#include "Update.h"
#include "CampList.h"
#include "mesg.h"
#include "falcmesg.h"
#include "mission.h"
#include "atm.h"
#include "Package.h"
#include "team.h"
#include "AIInput.h"
#include "F4Find.h"
#include "MsgInc/AirTaskingMsg.h"
#include "MsgInc/DivertMsg.h"
#include "MsgInc/RadioChatterMsg.h"
#include "ThreadMgr.h"
#include "CmpClass.h"
#include "FalcSess.h"
#include "classtbl.h"

//sfr: added for buffer chks
#include "InvalidBufferException.h"

#include "Debuggr.h"

//#define TEST_SCRAMBLE 1

//#define DEBUG_TIMING 1

//#define REQHELP_DEBUG 1

#ifdef DEBUG_TIMING
extern _TCHAR MissStr[AMIS_OTHER][16];
#endif

#ifdef CAMPTOOL
// Renaming tool stuff
extern VU_ID_NUMBER RenameTable[65536];
extern int gRenameIds;
#endif

extern int FriendlyTerritory(GridIndex x, GridIndex y, int team);
extern int GetAction(CampaignTime tot, int who);

// =================================================================
// ATM (Air Tasking Manager) defines and locals
// =================================================================

#define REQUESTS_TO_SAVE 10
#define MAX_SCRAMBLE_DISTANCE 60

extern int PackRadius(int type);
extern int PackInserted;
extern int gCampDataVersion;

extern  int g_nMaxInterceptDistance;

//#ifdef KEV_ADEBUG
extern char MissStr[AMIS_OTHER][16];
//#endif

// int SortieCycle[6] = { 0x1F, 0x1B, 0x15, 0x0A, 0x04, 0x00 };

// ================
// Prototypes
// ================

void FindBestLocation(MissionRequest mis, List list);
void ChillPackage(Package *pc);
void RecalculateWaypoint(WayPointClass *w, CampaignTime newDeparture);

// ==========================
// ATM Airbase Class
// ==========================

ATMAirbaseClass::ATMAirbaseClass(void)
{
    id = FalconNullId;
    memset(schedule, 0, sizeof(uchar)*ATM_MAX_CYCLES);
    usage = 0;
    next = NULL;
}

ATMAirbaseClass::ATMAirbaseClass(CampEntity ent)
{
    id = ent->Id();
    memset(schedule, 0, sizeof(uchar)*ATM_MAX_CYCLES);
    usage = 0;
    next = NULL;
}

ATMAirbaseClass::ATMAirbaseClass(VU_BYTE **stream, long *rem)
{
    memcpychk(&id, stream, sizeof(VU_ID), rem);
#ifdef DEBUG
    id.num_ and_eq 0xffff;
#endif
    memcpychk(schedule, stream, ATM_MAX_CYCLES, rem);
    usage = 0;
    next = NULL;
}

ATMAirbaseClass::ATMAirbaseClass(FILE *file)
{
    fread(&id, sizeof(VU_ID), 1, file);
#ifdef DEBUG
    id.num_ and_eq 0xffff;
#endif
    fread(schedule, sizeof(uchar), ATM_MAX_CYCLES, file);
    usage = 0;
    next = NULL;

    // Or in sortie masks - This is part of the hack for Dave
    // for (int i=0; i<ATM_MAX_CYCLES; i++)
    // schedule[i] or_eq SortieCycle[DEFAULT_SORTIE_RATE];
}

ATMAirbaseClass::~ATMAirbaseClass()
{
}

int ATMAirbaseClass::Save(VU_BYTE **stream)
{
#ifdef CAMPTOOL

    if (gRenameIds)
        id.num_ = RenameTable[id.num_];

#endif
    memcpy(*stream, &id, sizeof(VU_ID));
    *stream += sizeof(VU_ID);
    memcpy(*stream, schedule, ATM_MAX_CYCLES);
    *stream += ATM_MAX_CYCLES;
    return Size();
}

int ATMAirbaseClass::Save(FILE *file)
{
#ifdef CAMPTOOL

    if (gRenameIds)
        id.num_ = RenameTable[id.num_];

#endif
    fwrite(&id, sizeof(VU_ID), 1, file);
    fwrite(schedule, sizeof(uchar), ATM_MAX_CYCLES, file);
    return Size();
}

// ==========================
// ATM Class
// ==========================

// constructors
AirTaskingManagerClass::AirTaskingManagerClass(ushort type, Team t) : CampManagerClass(type, t)
{
    flags = 0;
    squadrons = 0;
    awacsList = new ListClass;
    tankerList = new ListClass;
    ecmList = new ListClass;
    requestList = new ListClass(LADT_FREE_USER_DATA bitor LADT_SORTED_LIST);
    delayedList = new ListClass(LADT_FREE_USER_DATA bitor LADT_SORTED_LIST);
    squadronList = NULL;
    packageList = NULL;
    airbaseList = NULL;
    supplyBase = 0;
    cycle = 0;
    averageCAMissions = 500;
    averageCAStrength = 50;
    currentCAMissions = 0;
    sampleCycles = 0;
}

AirTaskingManagerClass::AirTaskingManagerClass(VU_BYTE **stream, long *rem) : CampManagerClass(stream, rem)
{
    uchar num;
    ATMAirbaseClass *cur, *last = NULL;

    // Zero other data (will be rebuilt)
    squadrons = 0;
    awacsList = new ListClass;
    tankerList = new ListClass;
    ecmList = new ListClass;
    requestList = new ListClass(LADT_FREE_USER_DATA bitor LADT_SORTED_LIST);
    delayedList = new ListClass(LADT_FREE_USER_DATA bitor LADT_SORTED_LIST);
    squadronList = NULL;
    packageList = NULL;
    supplyBase = 0;

    memcpychk(&flags, stream, sizeof(short), rem);
    memcpychk(&averageCAStrength, stream, sizeof(short), rem);
    memcpychk(&averageCAMissions, stream, sizeof(short), rem);
    memcpychk(&sampleCycles, stream, sizeof(uchar), rem);
    currentCAMissions = 0;
    memcpychk(&num, stream, sizeof(uchar), rem);
    airbaseList = NULL;

    while (num)
    {
        cur = new ATMAirbaseClass(stream, rem);

        if ( not airbaseList)
            airbaseList = cur;
        else
            last->next = cur;

        last = cur;
        num --;
    }

    memcpychk(&cycle, stream, sizeof(uchar), rem);
}

AirTaskingManagerClass::AirTaskingManagerClass(FILE *file) : CampManagerClass(file)
{
    short nreq;
    MissionRequest mis;
    uchar num;
    ATMAirbaseClass *cur, *last = NULL;

    // Zero other data (will be rebuilt)
    squadrons = 0;
    awacsList = new ListClass;
    tankerList = new ListClass;
    ecmList = new ListClass;
    requestList = new ListClass(LADT_FREE_USER_DATA bitor LADT_SORTED_LIST);
    delayedList = new ListClass(LADT_FREE_USER_DATA bitor LADT_SORTED_LIST);
    squadronList = NULL;
    packageList = NULL;
    supplyBase = 0;

    fread(&flags, sizeof(short), 1, file);

    if (gCampDataVersion >= 28)
    {
        if (gCampDataVersion >= 63)
            fread(&averageCAStrength, sizeof(short), 1, file);

        fread(&averageCAMissions, sizeof(short), 1, file);
        fread(&sampleCycles, sizeof(uchar), 1, file);
        currentCAMissions = 0;
    }

    if (gCampDataVersion < 63)
    {
        averageCAMissions = 500;
        averageCAStrength = 50;
        sampleCycles = 10;
        currentCAMissions = 0;
    }

    fread(&num, sizeof(uchar), 1, file);
    airbaseList = NULL;

    while (num)
    {
        cur = new ATMAirbaseClass(file);

        if ( not airbaseList)
            airbaseList = cur;
        else
            last->next = cur;

        num--;
        last = cur;
    }

    fread(&cycle, sizeof(uchar), 1, file);

    // Read list size
    fread(&nreq, sizeof(short), 1, file);

    // Read list entries
    if (gCampDataVersion < 35)
    {
        // Zero these - they're invalid
        int size = 64;
        mis = new MissionRequestClass();

        while (nreq > 0)
        {
            fread(mis, size, 1, file);
            nreq--;
        }

        delete mis;
    }
    else
    {
        while (nreq > 0)
        {
            mis = new MissionRequestClass();
            fread(mis, sizeof(MissionRequestClass), 1, file);

            if (gCampDataVersion >= 22)
                requestList->InsertNewElement(mis->priority, mis, LADT_FREE_USER_DATA);
            else
                delete mis;

            nreq--;
        }
    }
}

AirTaskingManagerClass::~AirTaskingManagerClass()
{
    ATMAirbaseClass *next;

    delete requestList;
    delete delayedList;
    delete tankerList;
    delete ecmList;
    delete awacsList;

    while (airbaseList)
    {
        next = airbaseList->next;
        delete airbaseList;
        airbaseList = next;
    }

    if (squadronList)
    {
        squadronList->Unregister();
        delete squadronList;
        squadronList = NULL;
    }

    if (packageList)
    {
        packageList->Unregister();
        delete packageList;
        packageList = NULL;
    }

    squadrons = 0;
}

int AirTaskingManagerClass::SaveSize(void)
{
    int size = 0;
    ATMAirbaseClass *cur;

    // Count airbase elements
    cur = airbaseList;

    while (cur)
    {
        size += cur->Size();
        cur = cur->next;
    }

    size += CampManagerClass::SaveSize()
            + sizeof(short)
            + sizeof(short)
            + sizeof(short)
            + sizeof(uchar)
            + sizeof(uchar)
            + sizeof(uchar);

    return size;
}

int AirTaskingManagerClass::Save(VU_BYTE **stream)
{
    uchar num = 0;
    ATMAirbaseClass *cur;

    CampManagerClass::Save(stream);
    memcpy(*stream, &flags, sizeof(short));
    *stream += sizeof(short);
    memcpy(*stream, &averageCAStrength, sizeof(short));
    *stream += sizeof(short);
    memcpy(*stream, &averageCAMissions, sizeof(short));
    *stream += sizeof(short);
    memcpy(*stream, &sampleCycles, sizeof(uchar));
    *stream += sizeof(uchar);

    // Count airbase elements
    cur = airbaseList;

    while (cur)
    {
        num++;
        cur = cur->next;
    }

    memcpy(*stream, &num, sizeof(uchar));
    *stream += sizeof(uchar);

    cur = airbaseList;

    while (cur)
    {
        cur->Save(stream);
        cur = cur->next;
    }

    memcpy(*stream, &cycle, sizeof(uchar));
    *stream += sizeof(uchar);

#ifdef KEV_DEBUG
    MonoPrint("Sending team %d air tasking manager.\n", owner);
#endif

    return AirTaskingManagerClass::SaveSize();
}

int AirTaskingManagerClass::Save(FILE *file)
{
    short nreq = 0, retval = 0;
    MissionRequest mis;
    ListNode lp;
    uchar num = 0;
    ATMAirbaseClass *cur;

    if ( not file)
        return 0;

    retval += CampManagerClass::Save(file);
    retval += fwrite(&flags, sizeof(short), 1, file);
    retval += fwrite(&averageCAStrength, sizeof(short), 1, file);
    retval += fwrite(&averageCAMissions, sizeof(short), 1, file);
    retval += fwrite(&sampleCycles, sizeof(uchar), 1, file);

    // Count airbase elements
    cur = airbaseList;

    while (cur)
    {
        num++;
        cur = cur->next;
    }

    retval += fwrite(&num, sizeof(uchar), 1, file);
    cur = airbaseList;

    while (cur)
    {
        cur->Save(file);
        cur = cur->next;
    }

    retval += fwrite(&cycle, sizeof(uchar), 1, file);

    // Count list elements
    lp = requestList->GetFirstElement();

    while (lp and nreq < REQUESTS_TO_SAVE)
    {
        if (lp->GetUserData())
            nreq++;

        lp = lp->GetNext();
    }

    lp = delayedList->GetFirstElement();

    while (lp and nreq < REQUESTS_TO_SAVE)
    {
        if (lp->GetUserData())
            nreq++;

        lp = lp->GetNext();
    }

    retval += fwrite(&nreq, sizeof(short), 1, file);

    // Write list entries
    lp = requestList->GetFirstElement();

    while (lp and nreq)
    {
        mis = (MissionRequest) lp->GetUserData();

        if (mis)
        {
            retval += fwrite(mis, sizeof(MissionRequestClass), 1, file);
            nreq--;
        }

        lp = lp->GetNext();
    }

    lp = delayedList->GetFirstElement();

    while (lp and nreq)
    {
        mis = (MissionRequest) lp->GetUserData();

        if (mis)
        {
            retval += fwrite(mis, sizeof(MissionRequestClass), 1, file);
            nreq--;
        }

        lp = lp->GetNext();
    }

    return retval;
}

int AirTaskingManagerClass::Task(void)
{
    ListNode lp, pp;
    MissionRequest mis;
    Package pc;
    int res;
    DWORD task_time = GetTickCount();

    // Don't do this if we're not active, or not owned by this machine
    if ( not (TeamInfo[owner]->flags bitand TEAM_ACTIVE) or not IsLocal())
        return 0;

    // If we don't have any squadrons, don't bother
    if ( not squadrons)
        return 0;

    // Check need requests on current non-final packages
    {
        VuListIterator packit(packageList);
        pc = (Package) packit.GetFirst();

        while (pc)
        {
            if ( not pc->Final())
                pc->CheckNeedRequests();

            pc = (Package) packit.GetNext();
        }
    }
    pc = NULL;

    // Don't bother looking unless we've gotten a new request or more airplanes
    if ( not (flags bitand ATM_NEW_REQUESTS) and not (flags bitand ATM_NEW_PLANES))
        return 0;

    // Clear flags
    flags and_eq compl ATM_NEW_PLANES;
    flags or_eq ATM_NEW_REQUESTS;

    // Now traverse my request list
    lp = requestList->GetLastElement();

    while (lp)
    {
        mis = (MissionRequest) lp->GetUserData();
        pp = lp;
        lp = pp->GetPrev();

        if ( not mis)
        {
            // This request has been invalidated while we were looping
            CampEnterCriticalSection();
            requestList->Remove(pp);
            CampLeaveCriticalSection();
            continue;
        }

        if (missionsFilled >= missionsToFill and not mis->action_type and not (MissionData[mis->mission].flags bitand AMIS_FLYALWAYS))
            continue;

        // REMOVE ASAP
        //Cobra someone didn't remove this and thus we had NO DCA missions
        /*if (mis->mission == AMIS_BARCAP)
          continue;*/
        // END REMOVE

        if (GetTickCount() - task_time > 200)
            return 0;

#ifdef DEBUG_TIMING
        DWORD time = GetTickCount();
#endif

        if (mis->flags bitand AMIS_IMMEDIATE)
            res = BuildDivert(mis);
        else
            res = BuildPackage(&pc, mis);

        // RV - Biker - Limit strenght for give mission
        mis->aircraft = min(mis->aircraft, MissionData[mis->mission].str);

#ifdef DEBUG_TIMING
        MonoPrint("Team %d %s Pack @ %d,%d: %d ticks (%s)\n", owner, MissStr[mis->mission], mis->tx, mis->ty, GetTickCount() - time, (res == PRET_SUCCESS) ? "Success" : "Failure");
#endif
        CampEnterCriticalSection();

        switch (res)
        {
            case PRET_SUCCESS:

                // Tally any new Counter Air missions
                if (MissionData[mis->mission].skill == ARO_CA)
                {
                    if (pc)
                        averageCAStrength = (averageCAStrength * 4 + pc->GetAAStrength()) / 5;

                    currentCAMissions++;
                }

                // Insert this package into our active package list
                if (pc and not (mis->flags bitand AMIS_IMMEDIATE))
                {
                    packageList->ForcedInsert(pc);
                    pc = NULL;
                }

                // JB 000811 - Move the Remove and NULL around since references to mis could otherwise cause a crash (occured once)
                //requestList->Remove(pp);//-
                //pp = NULL;//-
                if ( not mis->action_type and not (MissionData[mis->mission].flags bitand AMIS_FLYALWAYS))
                    missionsFilled++;

                requestList->Remove(pp);//+
                pp = NULL;//+
                // JB 000811
                break;

            case PRET_DELAYED:
                // Put this element on the delayed list
                requestList->Detach(pp);
                delayedList->Insert(pp);
                pp = NULL;
                break;

            default:
                // Delete this request
                requestList->Remove(pp);
                pp = NULL;
                break;
        }

        CampLeaveCriticalSection();

        // Clean up aborted packages (otherwise we can reuse the non-inserted entity)
        if (pc)
        {
            if (PackInserted)
            {
                ChillPackage(&pc);
            }
            else // JPO - brought into the loop - so it frees it here.
            {
                // Let vu clean up any packages which havn't been inserted
                VuReferenceEntity(pc);
                VuDeReferenceEntity(pc);
                pc = NULL;
            }
        }
    }

    // we actually finished traversing the whole list.
    // Mark us as not having new requests and move the delayed list to the request list
    flags and_eq compl ATM_NEW_REQUESTS;
    lp = delayedList->GetFirstElement();

    while (lp)
    {
        delayedList->Detach(lp);
        requestList->Insert(lp);
        lp = delayedList->GetFirstElement();
    }

    return 1;
}

// This will update needed info such as available aircraft, tanker tracks, AWACS location,
// JSTAR location, and available squadrons
void AirTaskingManagerClass::DoCalculations(void)
{
    Squadron sq;
    int j, total_airbases = 0, avg_rate, sortie_rate;
    VU_ID oid;
    UnitClassDataType* uc;
    CampEntity airbase;
    GridIndex x = 0, y = 0;
    MissionRequestClass mis;
    ATMAirbaseClass *cur, *last;

    // Don't do this if we're not active, or not owned by this machine
    if ( not (TeamInfo[owner]->flags bitand TEAM_ACTIVE) or not IsLocal())
        return;

    if ( not squadrons)
        return;

    // Zero usage
    cur = airbaseList;

    while (cur)
    {
        cur->usage = 0;
        cur = cur->next;
    }

    // Statistics manipulation
    avg_rate = 60 / MIN_PLAN_AIR;
    averageCAMissions = ((averageCAMissions * sampleCycles) + currentCAMissions * 10 * (avg_rate / 2)) / (sampleCycles + avg_rate / 2);
    sampleCycles++;

    if (sampleCycles >= avg_rate)
        sampleCycles = avg_rate;

    currentCAMissions = 0;

    // Update table statistics, and add new entries
    {
        VuListIterator squadit(squadronList);
        sq = (Squadron) squadit.GetFirst();

        while (sq)
        {
            // Check out the airbase, add to our table.
            if ( not (airbase = sq->GetUnitAirbase()))
            {
                // Check for objective:
                sq->GetLocation(&x, &y);
                airbase = GetObjectiveByXY(x, y);

                if ( not airbase or (airbase->GetType() not_eq TYPE_AIRBASE and airbase->GetType() not_eq TYPE_ARMYBASE))
                {
                    // Check for carrier unit
                    airbase = FindUnitByXY(AllRealList, x, y, DOMAIN_SEA);

                    if ( not airbase or airbase->GetSType() not_eq STYPE_UNIT_CARRIER)
                    {
                        // Check for a fixed flag (we're our own airbase)
                        if (sq->DontPlan())
                        {
                            airbase = sq;
                        }
                    }
                }
            }

#ifdef DEBUG

            if (airbase == sq and not sq->DontPlan())
            {
                GridIndex x, y;
                sq->GetLocation(&x, &y);
                MonoPrint("Error: Squadron @ %d,%d not on airbase. Contact Dave Power.\n", x, y);
                airbase = NULL;
            }

#endif

            if (airbase)
            {
                AddToAirbaseList(airbase);
                sq->SetUnitAirbase(airbase->Id());
            }
            else
            {
#ifdef DEBUG
                MonoPrint("Couldn't find airbase for unit %d at %d,%d- Contact Kevin.\n", sq->GetCampID(), x, y);
#endif
                sq->SetUnitAirbase(FalconNullId);
            }

            // Shift usage schedule over by one
            sq->ShiftSchedule();
            // Modify ratings
            uc = sq->GetUnitClassData();

            for (j = 0; j < ARO_OTHER; j++)
            {
                sq->SetRating(j, (sq->GetRating(j) * 2 + uc->Scores[j]) / 3);
                ShiAssert(sq->GetRating(j) == 0 or uc->Scores[j] > 0);
            }

            sq = (Squadron) squadit.GetNext();
        }
    }
    flags or_eq ATM_NEW_PLANES;

    // Clean up airbase list and set new sortie rates
    cur = airbaseList;
    last = NULL;

    while (cur)
    {
        airbase = (CampEntity) vuDatabase->Find(cur->id);

        if ( not cur->usage or not airbase)
        {
            if ( not last)
                airbaseList = cur->next;
            else
                last->next = cur->next;

            delete cur;
        }
        else
        {
            total_airbases++;
            last = cur;

            // Shift current sortie values
            for (j = 0; j < ATM_MAX_CYCLES - 1; j++)
                cur->schedule[j] = cur->schedule[j + 1];

            if (airbase->IsObjective())
                sortie_rate = ((Objective)airbase)->GetAdjustedDataRate() * MIN_PLAN_AIR;
            else if (airbase->IsUnit())
                sortie_rate = 2 * MIN_PLAN_AIR;
            else
                sortie_rate = 0;

            if (sortie_rate)
            {
                if (airbase->IsObjective())
                {
                    // Request enemy strike vs this airbase
                    mis.requesterID = FalconNullId;
                    airbase->GetLocation(&mis.tx, &mis.ty);
                    mis.vs = airbase->GetTeam();
                    mis.who = GetEnemyTeam(mis.vs);
                    j = 30 + (rand() + airbase->GetCampID()) % 60;
                    mis.tot = Camp_GetCurrentTime() + j * CampaignMinutes;
                    mis.tot_type = TYPE_NE;
                    mis.targetID = airbase->Id();
                    mis.target_num = 255;
                    mis.mission = AMIS_OCASTRIKE;
                    mis.roe_check = ROE_AIR_ATTACK;
                    mis.context = enemyAirPowerAirbase;
                    mis.priority = cur->usage * 30;
                    mis.RequestMission();
                }
                else if (airbase->GetDomain() == DOMAIN_SEA)
                {
                    // Request enemy strike vs this carrier task force
                    mis.requesterID = FalconNullId;
                    airbase->GetLocation(&mis.tx, &mis.ty);
                    mis.vs = airbase->GetTeam();
                    mis.who = GetEnemyTeam(mis.vs);
                    j = 30 + (rand() + airbase->GetCampID()) % 60;
                    mis.tot = Camp_GetCurrentTime() + j * CampaignMinutes;
                    mis.tot_type = TYPE_NE;
                    mis.targetID = airbase->Id();
                    mis.target_num = 255;
                    mis.mission = AMIS_ASHIP;
                    mis.roe_check = ROE_NAVAL_FIRE;
                    mis.context = enemyAirPowerAirbase;
                    mis.RequestMission();
                }
            }

            if (supplyBase == total_airbases and airbase->IsObjective() and airbase->GetType() == TYPE_AIRBASE)
            {
                // Request an airlift mission into this airbase
                mis.requesterID = FalconNullId;
                airbase->GetLocation(&mis.tx, &mis.ty);
                mis.who = owner;
                mis.vs = 0;
                mis.roe_check = ROE_AIR_OVERFLY;
                mis.tot = Camp_GetCurrentTime() + 90 * CampaignMinutes;
                mis.tot_type = TYPE_NE;
                mis.targetID = airbase->Id();
                mis.target_num = 0;
                mis.mission = AMIS_AIRLIFT;
                mis.context = friendlySuppliesIncomingAir;

                mis.RequestMission();
            }

            // Clear the new block
            cur->schedule[ATM_MAX_CYCLES - 1] = 0;
        }

        if (last)
            cur = last->next;
        else
            cur = airbaseList;
    }

    supplyBase++;

    if (supplyBase >= total_airbases)
        supplyBase = 0;

    // KCK WARNING: This won't get sent over the net.
    TeamInfo[owner]->SetCurrentStats()->airbases = total_airbases;

    // Now update our AWACS, JSTAR, Tanker and ECM locations
    // AWACS/JSTAR ranges (from FLOT)
    FillDistanceList(awacsList, owner, MINIMUM_AWACS_DISTANCE, MAXIMUM_AWACS_DISTANCE);
    // Tanker ranges (from FLOT)
    FillDistanceList(tankerList, owner, MINIMUM_TANKER_DISTANCE, MAXIMUM_TANKER_DISTANCE);
    // ECM ranges (from FLOT)
    FillDistanceList(ecmList, owner, MINIMUM_ECM_DISTANCE, MAXIMUM_ECM_DISTANCE);
}

int AirTaskingManagerClass::BuildPackage(Package *pc, MissionRequest mis)
{
    int timeleft, j;
    Squadron sq;

    // Check time frame
    if (mis->tot < TheCampaign.CurrentTime)
        timeleft = (-1 * (TheCampaign.CurrentTime - mis->tot)) / CampaignMinutes;
    else
        timeleft = (mis->tot - TheCampaign.CurrentTime) / CampaignMinutes;

    if (timeleft < MissionData[mis->mission].min_time)
    {
        if (timeleft < 0 or mis->tot_type == TYPE_EQ or mis->tot_type == TYPE_LT  or (mis->delayed > 8))
            return PRET_TIMEOUT; // timed out, get rid of it

        // delay it's tot by a half hour
        mis->tot += 30 * CampaignMinutes;
        mis->delayed++;
        return PRET_DELAYED;
    }

    // if (mis->tot_type == TYPE_EQ)
    // mis->flags or_eq REQF_USERESERVES;

    // Reset assigned stats for this package (KCK NOTE: this is a minor gain, can probably axe this if need be)
    {
        VuListIterator squadit(squadronList);
        sq = (Squadron) squadit.GetFirst();

        while (sq)
        {
            sq->SetAssigned(0);
            sq = (Squadron) squadit.GetNext();
        }
    }

    // Get rid of unwanted mission requests
    if ( not *pc)
    {
        // Special case of new unit - We don't want to insert it.
        j = GetClassID(DOMAIN_AIR, CLASS_UNIT, TYPE_PACKAGE, 0, 0, 0, 0, 0) + VU_LAST_ENTITY_TYPE;
        *pc = NewPackage(j);
        (*pc)->BuildElements();
        (*pc)->SetOwner(owner);
        PackInserted = 0;
    }

    if ( not *pc)
        return PRET_NO_ASSETS; // This would be a VERY bad thing. Probably out of memory

    // MonoPrint("Building Mission Group %d (%d) at %d, %d\n",*pc->GetUnitID(),mis->mission,mis->tx,mis->ty);
    j = (*pc)->BuildPackage(mis, NULL);

    if (j == PRET_SUCCESS)
    {
        CampEntity e;
        mis->flags or_eq REQF_MET;

        // Return receipt
        if (mis->flags bitand REQF_NEEDRESPONSE)
        {
            e = (CampEntity) vuDatabase->Find(mis->requesterID);
#ifdef KEV_ADEBUG

            if ( not e)
                MonoPrint("Unknown Requester\n");

#endif

            if (e and e->IsUnit())
                ((Unit)e)->SendUnitMessage((*pc)->GetMainFlightID(), FalconUnitMessage::unitRequestMet, mis->mission, mis->who, PRET_SUCCESS);
        }

        // Now send a full update on the package
        VuEvent *event = new VuFullUpdateEvent(*pc, FalconLocalGame);
        event->RequestReliableTransmit();
        VuMessageQueue::PostVuMessage(event);
        return PRET_SUCCESS;
    }
    else // Failed to meet request
    {
        if (j == PRET_ABORTED)
            return PRET_ABORTED;
        else if (mis->flags bitand REQF_ONETRY)
            return PRET_CANCELED;
        else if (j == PRET_DELAYED or j == PRET_NO_ASSETS)
        {
            // We've either got no aircraft capible of flying this, or the block is out of takeoff range
            if (mis->max_to < 0) // Nothing can make it here in time
            {
                if (mis->tot_type == TYPE_EQ or mis->tot_type == TYPE_LT  or (mis->delayed > 8))
                    return PRET_TIMEOUT; // timed out, get rid of it
                else
                {
                    // delay it's tot by a half hour
                    mis->tot += 30 * CampaignMinutes;
                    mis->delayed++;
                    return PRET_DELAYED;
                }
            }
            else if (mis->min_to > 5) // To early for this mission, wait a while
                return PRET_DELAYED;
        }
        else
            return PRET_CANCELED;
    }

    return PRET_CANCELED;
}

int AirTaskingManagerClass::BuildDivert(MissionRequest mis)
{
    int time, ls, hs, tr;
    long misflags = MissionData[mis->mission].flags;
    Flight flight;
    // Package pack;
    FalconDivertMessage *divert;

    // Check time frame (NOTE: tot is request time for immediate missions, so we just want to check vs timeout)
    time = (Camp_GetCurrentTime() - mis->tot) / CampaignMinutes;

    if (time > MissionData[mis->mission].max_time)
        return PRET_TIMEOUT;

    // Check target viability (SAM coverage at target)
    ls = ScoreThreatFast(mis->tx, mis->ty, GetAltitudeLevel(MissionData[mis->mission].minalt * 100), mis->who);
    hs = ScoreThreatFast(mis->tx, mis->ty, GetAltitudeLevel(MissionData[mis->mission].maxalt * 100), mis->who);

    if (hs > ls)
        hs = ls;

    if (misflags bitand AMIS_HIGHTHREAT)
        tr = MAX_FLYMISSION_HIGHTHREAT;
    else if (misflags bitand AMIS_NOTHREAT)
        tr = MAX_FLYMISSION_NOTHREAT;
    else
        tr = MAX_FLYMISSION_THREAT;

    if (hs > tr)
        return PRET_CANCELED;

    // Attempt to find an available flight
    mis->caps or_eq MissionData[mis->mission].caps;
    mis->target_num = 255;

    if ( not mis->aircraft)
        mis->aircraft = MissionData[mis->mission].str;

    flight = TeamInfo[mis->who]->atm->FindBestAirFlight(mis); // We divert a current flight

    if ( not flight)
    {
        CampEntity target = (CampEntity)vuDatabase->Find(mis->targetID);

        if (target and (target->IsPackage() or target->IsBrigade()))
            target = ((Unit)target)->GetFirstUnitElement();

        if ( not target or (target->IsUnit() and ((Unit)target)->Broken()))
            return PRET_CANCELED;

        return PRET_DELAYED; // Check again later
    }

    flight->SetDiverted(1); // Keep us from getting picked again

    // Update the package statistics
    // pack = (Package)flight->GetUnitParent();
    // pack->GetMissionRequest()->tot = mis->tot;
    // pack->GetMissionRequest()->mission = mis->mission;
    // pack->GetMissionRequest()->targetID = mis->targetID;
    // pack->SetUnitDestination(mis->tx,mis->ty);

    // Send the divert message to everyone (for radio messages and scramble warning)
    divert = new FalconDivertMessage(flight->Id(), FalconLocalGame);
    divert->dataBlock.tot = mis->tot;
    divert->dataBlock.flags = mis->flags;
    divert->dataBlock.targetID = mis->targetID;
    divert->dataBlock.requesterID = mis->requesterID;
    divert->dataBlock.tx = mis->tx;
    divert->dataBlock.ty = mis->ty;
    divert->dataBlock.mission = mis->mission;
    divert->dataBlock.priority = (uchar)mis->priority;
    FalconSendMessage(divert, TRUE);

    if (mis->mission == AMIS_INTERCEPT)
    {
        // If we've not got enough strength for this intercept, we'll allow it to stay on
        // our request queue and task more aircraft versus it.
        int aa = GetUnitScore(flight, Air);

        if (aa * 2 < mis->match_strength)
        {
            mis->match_strength -= aa;
            return PRET_NO_ASSETS; // we're faking failure so it'll stay on the queue.
        }
    }

    return PRET_SUCCESS;
}

// This finds the "best" mission to divert the passed flight to.
int AirTaskingManagerClass::BuildSpecificDivert(Flight flight)
{
    ListNode lp, pp, bp = NULL;
    MissionRequest mis, bm = NULL;
    int minutes_past_tot, score, bs = 99999, ls, hs, tr;
    float d, speed;
    CampaignTime t;
    GridIndex x, y;
    FalconDivertMessage *divert;
    VuTargetEntity *vuTarget;

    // Don't do this if we're not active, or not owned by this machine
    if ( not (TeamInfo[owner]->flags bitand TEAM_ACTIVE) or not IsLocal())
        return 0;

    flight->GetLocation(&x, &y);
    speed = (float)flight->GetCombatSpeed();

    // Now traverse my request list
    lp = requestList->GetLastElement();

    while (lp)
    {
        mis = (MissionRequest) lp->GetUserData();
        pp = lp;
        lp = pp->GetPrev();

        if ( not mis)
            continue;

        // Check vs roll
        if (flight->GetUnitCurrentRole() not_eq MissionData[mis->mission].skill)
            continue;

        // Check viability of this target
        d = Distance(x, y, mis->tx, mis->ty);
        t = TimeToArrive(d, speed);
        minutes_past_tot = (int)(((Camp_GetCurrentTime() + t) - mis->tot) / CampaignMinutes);

        if (minutes_past_tot > 0 and (mis->tot_type == TYPE_EQ or mis->tot_type == TYPE_LT))
            continue;

        ls = ScoreThreatFast(mis->tx, mis->ty, GetAltitudeLevel(MissionData[mis->mission].minalt * 100), mis->who);
        hs = ScoreThreatFast(mis->tx, mis->ty, GetAltitudeLevel(MissionData[mis->mission].maxalt * 100), mis->who);

        if (hs > ls)
            hs = ls;

        if (MissionData[mis->mission].flags bitand AMIS_HIGHTHREAT)
            tr = MAX_FLYMISSION_HIGHTHREAT;
        else if (MissionData[mis->mission].flags bitand AMIS_NOTHREAT)
            tr = MAX_FLYMISSION_NOTHREAT;
        else
            tr = MAX_FLYMISSION_THREAT;

        if (hs > tr)
            continue;

        score = FloatToInt32(d) + abs(minutes_past_tot);

        if (score < bs)
        {
            bs = score;
            bm = mis;
            bp = pp;
        }
    }

    if (bm or flight->IsSetFalcFlag(FEC_HASPLAYERS))
    {
        // Send the divert message (or divert denied if not bm and flight-Player())
        if (flight->IsSetFalcFlag(FEC_HASPLAYERS))
            vuTarget = FalconLocalGame;
        else
            vuTarget = (VuTargetEntity*) vuDatabase->Find(flight->OwnerId());

        divert = new FalconDivertMessage(flight->Id(), vuTarget);

        if (bm)
        {
            // Update the package statistics
            Package pack = (Package)flight->GetUnitParent();
            pack->GetMissionRequest()->tot = bm->tot;
            pack->SetUnitDestination(bm->tx, bm->ty);

            divert->dataBlock.tot = bm->tot;
            divert->dataBlock.flags = bm->flags;
            divert->dataBlock.targetID = bm->targetID;
            divert->dataBlock.requesterID = bm->requesterID;
            divert->dataBlock.tx = bm->tx;
            divert->dataBlock.ty = bm->ty;
            divert->dataBlock.mission = bm->mission;
            divert->dataBlock.priority = (uchar)bm->priority;
        }

        FalconSendMessage(divert, TRUE);

        // Get rid of this request
        CampEnterCriticalSection();
        requestList->Remove(bp);
        CampLeaveCriticalSection();
        return PRET_SUCCESS;
    }

    return PRET_CANCELED;
}

void AirTaskingManagerClass::ProcessRequest(MissionRequest request)
{
    ListNode lp, pp;
    MissionRequest mis, pmis;
    int timeleft;
    CampEntity e;
    Package pack = NULL;
    GridIndex px, py;

    if (request->flags bitand AMIS_IMMEDIATE)
    {
        // Immediately try to task this, or continue if we can't
        if (BuildDivert(request) == PRET_SUCCESS)
            return;
    }

    // Adjust target depending on mission type
    if (request->mission == AMIS_AWACS)
        FindBestLocation(request, awacsList);
    else if (request->mission == AMIS_JSTAR)
        FindBestLocation(request, awacsList);
    else if (request->mission == AMIS_ECM)
        FindBestLocation(request, ecmList);
    else if (request->mission == AMIS_TANKER)
        FindBestLocation(request, tankerList);

    if ( not request->tx or not request->ty)
        return;

    // Adjust priority and times depending on it's inclusion in an action
    if (request->flags bitand REQF_PART_OF_ACTION)
    {
        // Add to our priority
        request->priority += 100;

        if (request->priority > 255)
            request->priority = 255;
    }
    else
    {
        // Reduce our priority
        request->priority -= 25;

        if (request->priority < 0)
            request->priority = 0;
    }

    // Convert mission type for unspotted objectives/units
    e = (CampEntity) vuDatabase->Find(request->targetID);

    if ( not e)
    {
        // No target.. Just a location
    }
    else if (e->IsObjective())
    {
        Objective o = (Objective)e;

        if (o and not o->GetSpotted(request->who)) // and o->GetObjectiveStatus() < 100)
        {
            // 20% chance of recon mission instead
            if ( not (rand() % 5) and o->HasDelta()) // Only recon if something's happened to it.
            {
                request->mission = AMIS_RECON;
                request->context = targetReconNeeded;
                request->tot_type = TYPE_NE;
                request->priority /= 4;
            }

            // 20% chance of ignoring it
            if ( not (rand() % 5))
                return;
        }
    }
    else if (e->IsUnit() and (e->IsBattalion() or e->IsBrigade()))
    {
        Unit u = (Unit)e;

        if (u and not u->GetSpotted(request->who) and not FriendlyTerritory(request->tx, request->ty, request->who))
        {
            // 33% chance of SAD mission instead (Except for SEAD Strike missions)
            if (request->mission not_eq AMIS_SEADSTRIKE and not (rand() % 3))
            {
                request->mission = AMIS_SAD;
                request->targetID = FalconNullId;
                request->context = enemyForcesPresent;
                //request->priority /= 4; Cobra removed // Very low priority mission
            }

            // 33% chance of ignoring it //Cobra removed this
            //if ( not (rand()%3))
            //return;
        }
    }
    else if (e->IsUnit() and e->IsTaskForce())
    {
        Unit u = (Unit)e;

        if (u and not u->GetSpotted(request->who))
        {
            // 66% chance of PATROL mission instead
            if ((rand() % 3))
            {
                request->mission = AMIS_PATROL;
                request->targetID = FalconNullId;
                // KCK WARNING: We'll need a new context here
                request->context = enemyForcesPresent;
                request->priority /= 4; // Very low priority mission
            }
        }
    }

    // Check to see if a similar mission is already in progress, and cancel if so.
    if (packageList) // and not request->action_type)
    {
        VuListIterator packit(packageList);
        pack = (Package) GetFirstUnit(&packit);

        while (pack)
        {
            pmis = pack->GetMissionRequest();

            if (request->action_type == pmis->action_type and 
                pmis->mission == request->mission and not pack->Aborted()) // KCK: Probably want ' and not mission finished' too
            {
                // We want to compare to in-progress missions differently for IMMEDIATE and non IMMEDIATE requests
                if (request->flags bitand AMIS_IMMEDIATE)
                {
                    // Check if it's our initial request (requested at the same time and same target)
                    // Dump only additional requests - the only time our initial request would still
                    // be here is if we need additional interceptors.
                    if (request->targetID == pmis->targetID and pmis->tot not_eq request->tot)
                        return;
                }
                else
                {
                    // Check if this planned/in-progress mission is similar to the one requested
                    pack->GetUnitDestination(&px, &py);

                    if (abs((int)pmis->tot - (int)request->tot) < MissionData[request->mission].mintime * CampaignMinutes
                       and Distance(request->tx, request->ty, px, py) < MissionData[request->mission].mindistance)
                    {
                        // We're going to cancel - but send a positive response if one's needed
                        // KCK NOTE: this may cause one BARCAP mission to be forced to fend off several attacking packages -
                        // all with escorts. Sending a negative response would cause no warning to appear in the briefing.
                        if (request->flags bitand REQF_NEEDRESPONSE)
                        {
                            e = (CampEntity) vuDatabase->Find(request->requesterID);

                            if (e->IsUnit())
                                ((Unit)e)->SendUnitMessage(pack->GetMainFlightID(), FalconUnitMessage::unitRequestMet, request->mission, request->who, 0);
                        }

                        return;
                    }
                }
            }

            pack = (Package) GetNextUnit(&packit);
        }
    }

    // Now check to see if a similar mission request is already on the queue
    lp = requestList->GetFirstElement();

    while (lp)
    {
        pmis = (MissionRequest) lp->GetUserData();
        pp = lp;
        lp = pp->GetNext();

        if ( not pmis)
            continue;

        // Check for timeout
        timeleft = (int)((pmis->tot - Camp_GetCurrentTime()) / CampaignMinutes);

        if (pmis->mission not_eq request->mission)
            continue;

        // Check for action vs non-action missions
        if (request->action_type and not pmis->action_type)
        {
            CampEnterCriticalSection();
            requestList->Remove(pp);
            CampLeaveCriticalSection();
            continue;
        }

        if ( not request->action_type and pmis->action_type)
            return;

        if ((pmis->tot_type == TYPE_LT or pmis->tot_type == TYPE_EQ) and timeleft < LONGRANGE_MIN_TIME)
        {
            CampEnterCriticalSection();
            requestList->Remove(pp);
            CampLeaveCriticalSection();
            continue;
        }

        // We want to compare to in-progress missions differently for IMMEDIATE and non IMMEDIATE requests
        if (request->flags bitand AMIS_IMMEDIATE)
        {
            // Dump earlier requests
            if (request->targetID == pmis->targetID and pmis->tot > request->tot)
            {
                if (pmis->tot > request->tot)
                    return;
                else
                {
                    CampEnterCriticalSection();
                    requestList->Remove(pp);
                    CampLeaveCriticalSection();
                }
            }
        }
        else
        {
            // Check if this planned/in-progress mission is similar to the one requested
            if (abs((int)pmis->tot - (int)request->tot) < MissionData[request->mission].mintime * CampaignMinutes
               and Distance(request->tx, request->ty, pmis->tx, pmis->ty) < MissionData[request->mission].mindistance)
            {
                // Take the earlier request
                if (pmis->tot <= request->tot)
                    return;

                CampEnterCriticalSection();
                requestList->Remove(pp);
                CampLeaveCriticalSection();
            }
        }
    }

    // Add the request to the request list
    mis = new MissionRequestClass();
    *mis = *request;

    if ( not mis)
        return;

    requestList->InsertNewElement(mis->priority, mis, LADT_FREE_USER_DATA);

    // KCK: Increase the number of missions we're allowed to plan if this is a valid type
    if ( not mis->action_type and not (MissionData[mis->mission].flags bitand AMIS_FLYALWAYS) and rand() % 100 < TeamInfo[mis->who]->GetGroundAction()->actionTempo)
        missionsToFill++;

#ifdef KEV_ADEBUG
    MonoPrint("* Accepted %s request for team %d at %d,%d - tot %f - pri %d\n", MissStr[mis->mission], mis->who, mis->tx, mis->ty, mis->tot, mis->priority);
#endif
    flags or_eq ATM_NEW_REQUESTS;
}

// This finds the best squadron to assign to a given mission.
Squadron AirTaskingManagerClass::FindBestAir(MissionRequest mis, GridIndex bx, GridIndex by)
{
    Squadron sq, ns, bs = NULL;
    int score, best = 0, bq = 0, av, sb, fb, role, na, sc = 0, lowestScore;
    uchar slots[4];
    float d, speed;
    short stats, caps, service;
    GridIndex X, Y;
    Unit cargo = NULL;
    CampaignTime t, to, tf, land, quickest = INFINITE_TIME;
    UnitClassDataType* uc;
    ATMAirbaseClass *airbase;
    Objective po;

    if ( not squadrons)
        return NULL;

    if (mis->flags bitand REQF_USE_REQ_SQUAD)
    {
        // We don't want to look for a squadron, we want to use this one.
        sq = (Squadron) FindUnit(mis->requesterID);
        memset(mis->slots, 255, 4);
        mis->start_block = mis->final_block = ATM_MAX_CYCLES - 1;
        av = sq->FindAvailableAircraft(mis);

        if (av)
            return sq;
        else
            return NULL;
    }

    // po = FindNearestObjective (POList, mis->tx, mis->ty, NULL);
    po = (Objective) vuDatabase->Find(mis->pakID);
    tf = CampaignMinutes * MIN_PLAN_AIR;
    role = MissionData[mis->mission].skill;

    if (role == ARO_TACTRANS)
        cargo = (Unit) vuDatabase->Find(mis->requesterID);
    else if (role == ARO_CA)
        sc = SQUADRON_SPECIALTY_AA; // Air to Air role
    else if (role == ARO_GA or role == ARO_S or role == ARO_SB or role == ARO_REC)
        sc = SQUADRON_SPECIALTY_AG; // Air to Ground role

    // Check for night missions
    if (TimeOfDayGeneral(mis->tot) == TOD_NIGHT)
        mis->caps or_eq VEH_NIGHT;

    caps = mis->caps bitand VEH_CAPIBILITY_MASK;
    service = mis->caps bitand VEH_SERVICE_MASK;
    lowestScore = (255 - mis->priority) / 25;

    {
        VuListIterator squadit(squadronList);
        ns = (Squadron) squadit.GetFirst();

        while (ns)
        {
            sq = ns;
            ns = (Squadron) squadit.GetNext();

            // 2001-07-05 ADDED BY S.G. DON'T USE IF THE RELOCATION TIMER HASN'T EXPIRED
            if (sq->squadronRetaskAt > Camp_GetCurrentTime())
                continue;

            // END OF ADDED SECTION

            // Determine base score (strat bombers use strike missions)
            score = (sq->GetRating(role) + 4) / 5;

            if (role == ARO_S and sq->GetRating(ARO_SB) > sq->GetRating(role))
                score = (sq->GetRating(ARO_SB) + 4) / 5;

            if (sq->GetUnitSpecialty())
            {
                if (sc == sq->GetUnitSpecialty())
                    score += 5; // Bonus for speciality
                else
                    score -= 5; // Penalty for non-specialty
            }

            // A.S. Assume a minimum score of 1, they should be able to do something at least...
            // M.N. bring back to original state, this change gives us AWACS flying Sweep missions...:-)
            // if ( max(score,1) <= lowestScore)
            if (score <= lowestScore)
                continue;

            // KCK HACK TO FORCE ONLY ALERT MISSIONS (TO TRACK DOWN THE SCRAMBLE STUFF)
#ifdef TEST_SCRAMBLE

            if (mis->mission not_eq AMIS_ALERT and sq->Id() == FalconLocalSession->GetPlayerSquadronID())
                continue;

#endif

            // Check for required plane capibilities
            uc = sq->GetUnitClassData();
            stats = uc->Flags;

            // 2001-04-28 ADDED BY S.G. GOOD MORAL SQUADRON WILL TASK NON NIGHT PLANES AT NIGHT (THEY'LL USE GCI ANYWAY)...
            if ( not sq->Broken())
                stats or_eq VEH_NIGHT;

            // END OF ADDED SECTION

            if ((caps bitand stats) not_eq caps or (service and not (service bitand stats)))
                continue;

            // 2001-04-26 ADDED BY S.G. SO STEALTH AIRCRAFT ARE NOT TASKED DURING DAYTIME. ONLY AT NIGHT...
            if (TimeOfDayGeneral(mis->tot) not_eq TOD_NIGHT and (stats bitand VEH_STEALTH))
                continue;

            // END OF ADDED SECTION


            if ((MissionData[mis->mission].flags bitand AMIS_NPC_ONLY) and TheCampaign.IsValidAircraftType(sq))
                continue;

            // Check range
            speed = (float)sq->GetCruiseSpeed();
            sq->GetLocation(&X, &Y);
            d = Distance(X, Y, mis->tx, mis->ty);

            if ( not (MissionData[mis->mission].flags bitand AMIS_FUDGE_RANGE))
            {
                float db = 0.0F;

                // Add loiter time to the estimated distance.
                if (MissionData[mis->mission].loitertime > 0)
                    db = (MissionData[mis->mission].loitertime * speed) / 60.0F;

                // RV - Biker - Reduce loiter time for short range AC
                if (d + db > sq->GetUnitRange())
                {
                    MissionData[mis->mission].loitertime = min(45, MissionData[mis->mission].loitertime);
                    db = (MissionData[mis->mission].loitertime * speed) / 60.0F;

                    if (d + db > sq->GetUnitRange())
                        continue;
                }

                // Airlift missions must come from another airbase
                if (mis->mission == AMIS_AIRLIFT and d < 40)
                    continue;
            }

            // Check speed vs required
            if (mis->speed and (mis->flags bitand AMIS_MATCHSPEED or MissionData[mis->mission].flags bitand AMIS_MATCHSPEED))
            {
                if (sq->GetMaxSpeed() < mis->speed or speed > mis->speed * 1.2F)
                    continue;

                speed = (float)mis->speed;
            }

            // Estimate takeoff time
            t = TimeToArrive(d, speed);
            to = mis->tot - t;
            land = mis->tot + t;

            if (to < scheduleTime)
            {
                if (mis->tot_type not_eq TYPE_NE and mis->tot_type <= TYPE_EQ) // Not going to be here in time
                    continue;

                // Otherwise, shift our estimate
                land += scheduleTime - to;
                to = scheduleTime;
            }

            mis->start_block = (int)((to - scheduleTime) / tf);
            mis->final_block = (int)((land - scheduleTime) / tf) + 1;

            if (mis->start_block < 1)
                mis->start_block = 0;
            else
                mis->start_block--; // Give us some breathing room

            if (mis->final_block > ATM_MAX_CYCLES)
                mis->final_block = ATM_MAX_CYCLES;

            // Determine # of aircraft needed
            if (mis->match_strength and role == ARO_CA)
            {
                mis->aircraft = 4;

                // if ((uc->Scores[role] * AirExperienceAdjustment(mis->who))/2 > mis->match_strength)
                if ((uc->HitChance[Air] * 3) / 2 > mis->match_strength)
                    mis->aircraft = 2;
            }

            // Check for aircraft availability
            memset(mis->slots, 255, 4);

            if (MissionData[mis->mission].flags bitand AMIS_DONT_USE_AC)
                av = mis->aircraft;
            else
                av = sq->FindAvailableAircraft(mis);

            if ((av < mis->aircraft - 1 and not (mis->flags bitand REQF_USERESERVES)) or av < 1)
                continue;

            // Check against airbase schedule for this block and previous block
            airbase = FindATMAirbase(sq->GetUnitAirbaseID());

            if ( not airbase or (airbase->schedule[mis->start_block] == ATM_CYCLE_FULL and ( not mis->start_block or airbase->schedule[mis->start_block - 1] == ATM_CYCLE_FULL)))
                continue;

            // Calculate it's score
            if (TheCampaign.IsValidAircraftType(sq)) // Task player squadrons with important missions
            {
                int bonus = 1;

                if (sq->IsSetFalcFlag(FEC_HASPLAYERS)) // Give a player unit some bonuses
                    bonus = 2;

                if (mis->priority > 200) // For high priority missions
                    score += mis->priority * bonus / 20;
                else if (mis->priority > 100)
                    score += mis->priority * bonus / 40;
                else if (mis->priority < 50)
                    score -= (50 - mis->priority) * bonus / 10;

                if (mis->action_type or not (MissionData[mis->mission].flags bitand AMIS_FLYALWAYS))
                    score += 5 * bonus; // Bonus for special or "hotspot" missions
                else
                    score -= 5 * bonus;
            }

            if (sq->GetAssigned()) // Bonus for using the same squadron
                score += 3;

            if (X == bx and Y == by) // Bonus for same airbase
                score += 2;

            if (d < sq->GetUnitRange() / 2) // Bonus if within 1/2 range
                score += 2;

            if (cargo and cargo->GetOwner() == sq->GetOwner()) // Bonus for carriers owned by same country
                score += 5;

            if (av < mis->aircraft) // Penalty for not enough aircraft
                score -= 5;

            if (t < quickest) // Bonus for being there soonest
            {
                score += 2;
                quickest = t;

                if (bq)
                {
                    best--;
                    bq = 0;
                }
            }

            if (score <= best)
                continue;

            best = score;
            bs = sq;

            // We can use this data later
            sb = mis->start_block;
            fb = mis->final_block;
            na = mis->aircraft;
            memcpy(slots, mis->slots, 4);

            if (t == quickest)
                bq = 1;
        }

    }

    if ( not bs)
        return NULL;

    // Record service of the selected aircraft and other info
    if ( not service)
        mis->caps or_eq (bs->class_data->Flags bitand VEH_SERVICE_MASK);

    // Set stealth mission, if applicable
    if (bs->class_data->Flags bitand VEH_STEALTH)
        mis->caps or_eq VEH_STEALTH;

    mis->start_block = sb;
    mis->final_block = fb;
    mis->aircraft = na;
    memcpy(mis->slots, slots, 4);

    return bs;
}

// This finds the best in-flight Flight to assign to a given mission.
Flight AirTaskingManagerClass::FindBestAirFlight(MissionRequest mis)
{
    Flight bf = NULL;
    Unit nu, cf;
    int score, best = -999, role, aa;
    float d, speed;
    short stats, caps, service;
    GridIndex X, Y;
    CampaignTime t;
    UnitClassDataType* uc;

    if ( not squadrons)
        return NULL;

    role = MissionData[mis->mission].skill;
    caps = mis->caps bitand VEH_CAPIBILITY_MASK;
    service = mis->caps bitand VEH_SERVICE_MASK;
    VuListIterator myit(AllAirList);
    nu = (Unit) myit.GetFirst();

    while (nu)
    {
        cf = nu;
        nu = (Unit) myit.GetNext();

        if (cf->GetType() not_eq TYPE_FLIGHT or not cf->Final() or cf->IsDead())
            continue;

#ifdef REQHELP_DEBUG

        if ( not (mis->flags bitand AMIS_HELP_REQUEST))
            continue;

#endif

        // Check for valid role
        if (MissionData[cf->GetUnitMission()].skill not_eq role)
            continue;

        // Check for team
        if (cf->GetTeam() not_eq mis->who)
            continue;

        // Check if it's busy (Flights should reduce their priority to 0 when they're done with their current task)
        if (mis->flags bitand AMIS_HELP_REQUEST)
        {
            if (cf->GetUnitPriority() > mis->priority + 50)
                continue;
        }
        else
        {
            if (cf->GetUnitPriority() * 2 > mis->priority)
                continue;
        }

        // Verify it's not aborting or diverted (if diverted, reevaluate if help request)
        if (cf->Aborted() or cf->Diverted() and not (mis->flags bitand AMIS_HELP_REQUEST))
            continue;

        // Check to make sure it's taken off (unless it's an alert mission)
        if (cf->GetUnitMission() not_eq AMIS_ALERT and ( not cf->GetCurrentUnitWP() or cf->GetCurrentUnitWP()->GetWPAction() == WP_TAKEOFF))
            continue;

        // Check for required plane capibilities
        uc = cf->GetUnitClassData();
        stats = uc->Flags;
        score = (uc->Scores[role] + 4) / 5;

        if (score <= 0 or (caps bitand stats) not_eq caps or (service and not (service bitand stats)))
            continue;

        // Check for aircraft and priority
        // 2001-10-27 MODIFIED BY S.G. Doesn't matter how many vehicle if it's a help request. Hopefully, the one requesting help will assist us
        // 2001-12-18 M.N. give a help request mission priority some more points..
        // if (cf->GetTotalVehicles() < mis->aircraft or cf->GetUnitPriority() >= mis->priority)
        if (( not (mis->flags bitand AMIS_HELP_REQUEST) and cf->GetTotalVehicles() < mis->aircraft) or ( not (mis->flags bitand AMIS_HELP_REQUEST) and cf->GetUnitPriority() >= mis->priority) or (mis->flags bitand AMIS_HELP_REQUEST) and cf->GetUnitPriority() >= mis->priority + 20)
            continue;

        // Check speed vs required
        speed = (float)cf->GetCombatSpeed();

        if (mis->speed and (mis->flags bitand AMIS_MATCHSPEED or MissionData[mis->mission].flags bitand AMIS_MATCHSPEED))
        {
            if (cf->GetMaxSpeed() < mis->speed or speed > mis->speed * 1.2F)
                continue;

            speed = (float)mis->speed;
        }

        // M.N. commented out - since when is a heading interesting for finding a best air flight ? can't they turn around ?
        // Check vs heading
        cf->GetLocation(&X, &Y);
        // if (cf->Moving() and mis->min_to >= 0)
        // {
        // int hd = abs(mis->min_to - DirectionTo(X,Y,mis->tx,mis->ty));
        // if (hd < 3 or hd > 5)
        // continue;
        // }

        // Calculate it's score
        if (cf->IsSetFalcFlag(FEC_HASPLAYERS)) // Give a player unit a bonus for important missions
        {
            // 2001-10-27 ADDED BY S.G. If it's a help request, don't count players' flight. Let the players deal with themself.
            if (mis->flags bitand AMIS_HELP_REQUEST)
                continue;

            // END OF ADDED SECTION 2001-10-27
            score += mis->priority / 50;
        }

        d = Distance(X, Y, mis->tx, mis->ty);

        // 2002-04-14 MN when the target is more than this distance away, skip the tested interceptor flight
        // when player calls for help - ignore distance
        //Cobra TEST remove first part
        //if (d > g_nMaxInterceptDistance and not (mis->flags bitand AMIS_HELP_REQUEST))
        //continue;
        if (cf->GetUnitMission() == AMIS_ALERT and d > 250.0f/*MAX_SCRAMBLE_DISTANCE*/)
            continue;

        if (d < cf->GetUnitRange() / 4) // Bonus if within 1/4 range
            score ++;

        if (cf->GetUnitMission() == mis->mission)
            score += 2; // Bonus for same mission



        // KCK: Experimental - this is to keep from tasking flights which will simply ignore
        // their orders because they have better things to do.
        if (cf->Engaged() and cf->GetTargetID() not_eq FalconNullId)
        {
            FalconEntity *oldtarget = cf->GetTarget();
            FalconEntity *newtarget = (FalconEntity*) vuDatabase->Find(mis->targetID);
            int oldreact = 0, newreact = 0, combat, spot, tstr;
            float d;

            if (oldtarget)
            {
                if (oldtarget->IsAirplane())
                    oldreact = ((Flight)cf)->DetectVs((AircraftClass*)oldtarget, &d, &combat, &spot, &tstr);
                else if (oldtarget->IsCampaign())
                    oldreact = ((Flight)cf)->DetectVs((CampEntity)oldtarget, &d, &combat, &spot, &tstr);
            }

            if (newtarget)
            {
                if (newtarget->IsAirplane())
                    newreact = ((Flight)cf)->DetectVs((AircraftClass*)newtarget, &d, &combat, &spot, &tstr);
                else if (newtarget->IsCampaign())
                    newreact = ((Flight)cf)->DetectVs((CampEntity)newtarget, &d, &combat, &spot, &tstr);
            }

            if (oldreact + 2 > newreact)
                continue;
        }

        // Adjust for current priority
        score -= cf->GetUnitPriority() / 30;

        // 2001-10-27 ADDED BY S.G. Adjust for the number of planes in both flights if it's a request for help
        if ((mis->flags bitand AMIS_HELP_REQUEST) and cf->GetTotalVehicles() < mis->aircraft)
            score -=  mis->aircraft - cf->GetTotalVehicles();

        // END OF ADDED SECTION 2001-10-27

        // Modify by strength for counter-air missions
        if (role == ARO_CA)
        {
            if ( not mis->match_strength)
                mis->match_strength = 1; // Some strength - for ratioing

            aa = GetUnitScore(cf, Air);

            if (aa > mis->match_strength * 2)
                score -= max(5, aa / mis->match_strength); // We're overkill
            else if (aa * 2 < mis->match_strength)
                score -= max(5, mis->match_strength / aa); // We're outmatched

            if (cf->GetUnitMission() == AMIS_BARCAP2)
                score += 5; // Bonus for BARCAP2 missions
        }

        // Estimate arrival time
        t = TimeToArrive(d, speed);
        score -= t / (2 * CampaignMinutes);
        /*
           if (t < quickest and score+3 > best-3) // Bonus for being there soonest
           {
           score += 3;
           best -= 3;
           quickest = t;
           }
         */

        if (score <= best)
            continue;

        best = score;
        bf = (Flight)cf;

        // if (t == quickest)
        // bq = 1;
    }

    if ( not bf)
        return NULL;

    // Record service of the selected aircraft and other info
    if ( not service)
        mis->caps or_eq (bf->class_data->Flags bitand VEH_SERVICE_MASK);

    return bf;
}

//void AirTaskingManagerClass::SendATMMessage(VU_ID from, Team to, short msg, short d1, short d2, void* d3, int flags)
void AirTaskingManagerClass::SendATMMessage(VU_ID from, Team to, short msg, short d1, short d2, void* d3, int)
{
    FalconAirTaskingMessage *message;
    VuTargetEntity *target = (VuTargetEntity*) vuDatabase->Find(OwnerId());

    message = new FalconAirTaskingMessage(Id(), target);
    message->dataBlock.from = from;
    message->dataBlock.team = to;
    message->dataBlock.messageType = msg;
    message->dataBlock.data1 = d1;
    message->dataBlock.data2 = d2;
    message->dataBlock.data3 = d3;
    FalconSendMessage(message, TRUE);
}

// Function returns the time delta between our requested takeoff time and
// the assigned takeoff time.
// returns 0xFFFFFFFF on error
int AirTaskingManagerClass::FindTakeoffSlot(VU_ID abid, WayPoint w)
{
    int block, slot, mins, count = 0;
    CampaignTime requestedtakeoff;
    ATMAirbaseClass *airbase;

    airbase = FindATMAirbase(abid);

    if ( not airbase)
        return 0xFFFFFFFF;

    // Find minutes to takeoff
    requestedtakeoff = w->GetWPArrivalTime();
    mins = (int)((requestedtakeoff - scheduleTime) / CampaignMinutes);
    // Find block and slot
    block = mins / MIN_PLAN_AIR;
    slot = mins % MIN_PLAN_AIR;

    if (block >= ATM_MAX_CYCLES or mins <= 0)
        return 0xFFFFFFFF;

    // Try to take off at this exact slot, on main runway
    if ( not (airbase->schedule[block] bitand (0x01 << slot)))
    {
        // Runway is free, snap time to the minute and recalc speed
        RecalculateWaypoint(w, scheduleTime + mins * CampaignMinutes);
        return -1 * (requestedtakeoff - w->GetWPArrivalTime());
    }

    // Otherwise, try the next two slots (one and two minutes later)
    mins++;
    block = mins / MIN_PLAN_AIR;
    slot = mins % MIN_PLAN_AIR;

    if (block < ATM_MAX_CYCLES and not (airbase->schedule[block] bitand (0x01 << slot)))
    {
        // Runway is free, snap time to the minute and recalc speed
        RecalculateWaypoint(w, scheduleTime + mins * CampaignMinutes);
        return w->GetWPArrivalTime() - requestedtakeoff;
    }

    mins++;
    block = mins / MIN_PLAN_AIR;
    slot = mins % MIN_PLAN_AIR;

    if (block < ATM_MAX_CYCLES and not (airbase->schedule[block] bitand (0x01 << slot)))
    {
        // Runway is free, snap time to the minute and recalc speed
        RecalculateWaypoint(w, scheduleTime + mins * CampaignMinutes);
        return w->GetWPArrivalTime() - requestedtakeoff;
    }

    // Otherwise, find the next earliest slot and add a timing leg (if necessary)
    mins -= 3;

    while (mins > 0 and count < 10)
    {
        block = mins / MIN_PLAN_AIR;
        slot = mins % MIN_PLAN_AIR;

        if ( not (airbase->schedule[block] bitand (0x01 << slot)))
        {
            // Runway is free, snap time to the minute and recalc speed
            RecalculateWaypoint(w, scheduleTime + mins * CampaignMinutes);
            return -1 * (requestedtakeoff - w->GetWPArrivalTime());
        }

        count++;
        mins--;
    }

    return 0xFFFFFFFF;
}

void AirTaskingManagerClass::ScheduleAircraft(VU_ID abid, WayPoint w, int aircraft)
{
    int block, slot, mins, sortie_rate = 2;
    ATMAirbaseClass *airbase;
    CampEntity abe;
    WayPoint lw;

    airbase = FindATMAirbase(abid);

    if ( not airbase)
        return;

    // Find minutes to takeoff
    mins = (int)((w->GetWPArrivalTime() - scheduleTime) / CampaignMinutes);

    // Find block and slot
    block = mins / MIN_PLAN_AIR;
    slot = mins % MIN_PLAN_AIR;

    if (block >= ATM_MAX_CYCLES or mins <= 0)
        return;

    // Now fill the takeoff slot
    airbase->schedule[block] or_eq (0x01 << slot);

    // Fill the same slot in the next block - to allow for fudge time
    if (block + 1 < ATM_MAX_CYCLES)
        airbase->schedule[block + 1] or_eq (0x01 << slot);

    // If we're a large flight, fill the next slot
    if (aircraft > 2)
    {
        mins++;
        block = mins / MIN_PLAN_AIR;
        slot = mins % MIN_PLAN_AIR;

        if (block < ATM_MAX_CYCLES)
        {
            airbase->schedule[block] or_eq (0x01 << slot);

            // Fill the same slot in the next block - to allow for fudge time
            if (block + 1 < ATM_MAX_CYCLES)
                airbase->schedule[block + 1] or_eq (0x01 << slot);
        }
    }

    // If this is a one runway airbase, find the landing waypoint and
    // fill in the landing time - otherwise, assume landing aircraft will
    // use the other runway.
    abe = (CampEntity) vuDatabase->Find(airbase->id);

    if (abe->IsObjective())
    {
        sortie_rate = ((Objective)abe)->GetAdjustedDataRate();
        ShiAssert(sortie_rate <= 2);
    }
    else if (abe->IsUnit())
        sortie_rate = 1;

    if (sortie_rate < 2)
    {
        lw = w->GetNextWP();

        while (lw and (lw->GetWPAction() not_eq WP_LAND or lw->GetWPTargetID() not_eq w->GetWPTargetID()))
            lw = lw->GetNextWP();

        if (lw)
        {
            mins = (int)((w->GetWPArrivalTime() - scheduleTime) / CampaignMinutes);
            block = mins / MIN_PLAN_AIR;
            slot = mins % MIN_PLAN_AIR;

            // Fill one slot for each landing aircraft - keep
            // looking until we find enough slots
            while (aircraft and block < ATM_MAX_CYCLES)
            {
                if ( not (airbase->schedule[block] bitand (0x01 << slot)))
                {
                    airbase->schedule[block] or_eq (0x01 << slot);
                    aircraft--;
                }

                mins++;
                block = mins / MIN_PLAN_AIR;
                slot = mins % MIN_PLAN_AIR;
            }
        }
    }
}

// Zap the schedule for the passed airbase and cancle any flights planned during the damaged period
void AirTaskingManagerClass::ZapAirbase(VU_ID abid)
{
    int tilblock;
    ATMAirbaseClass *airbase;
    CampEntity the_airbase;

    airbase = FindATMAirbase(abid);

    if ( not airbase)
        // KCK Note: airbase no longer in use. Still, it would be nice
        // to eliminate flights.. But it'd require a few minutes work..
        return;

    the_airbase = (CampEntity) vuDatabase->Find(abid);

    if (the_airbase->IsUnit())
    {
        // This is probably an aircraft carrier which was sunk.
        // Techinically, we should kill all squadrons based here and cancle all flights..
        // But for now, we do nothing.
    }
    else if ((((Objective)the_airbase)->static_data.class_data) and ((Objective)the_airbase)->static_data.class_data->DataRate == 1)
    {
        // Find out how long til we get our runway active
        tilblock = ((Objective)the_airbase)->GetRepairTime(1) * (60 / MIN_PLAN_AIR);

        if (tilblock > 0)
            ZapSchedule(0, airbase, tilblock);
    }
    else
    {
        // Find out how long til we get one runway active
        tilblock = ((Objective)the_airbase)->GetRepairTime(1) * (60 / MIN_PLAN_AIR);

        if (tilblock > 0)
            ZapSchedule(0, airbase, tilblock);

        // Find out how long til we get our second runway active
        tilblock = ((Objective)the_airbase)->GetRepairTime(51) * (60 / MIN_PLAN_AIR);

        if (tilblock > 0)
            ZapSchedule(1, airbase, tilblock);
    }
}

void AirTaskingManagerClass::ZapSchedule(int rw, ATMAirbaseClass *airbase, int tilblock)
{
    int i;
    Unit u;

    if (tilblock >= ATM_MAX_CYCLES)
        tilblock = ATM_MAX_CYCLES - 1;

    if ( not rw)
    {
        // We have no runway's operating currently - zap the entire schedule
        for (i = 0; i <= tilblock; i++)
            airbase->schedule[i] = ATM_CYCLE_FULL;
    }
    else if (rw == 1)
    {
        // We have one runway working - zap half our blocks
        for (i = 0; i <= tilblock; i++)
        {
            if (i % 2)
                airbase->schedule[i] = ATM_CYCLE_FULL;
        }
    }
    else
        // We have more than one runway working
        return;

    // Now cancel flights in the offending blocks
    VuListIterator myit(AllAirList);
    u = GetFirstUnit(&myit);

    while (u)
    {
        if (u->GetTeam() == owner and u->GetType() == TYPE_FLIGHT)
        {
            WayPoint w = u->GetCurrentUnitWP();

            if (w and w->GetWPAction() == WP_TAKEOFF and w->GetWPTargetID() == airbase->id)
            {
                int block, mins_til_takeoff;
                mins_til_takeoff = (w->GetWPDepartureTime() - TheCampaign.lastAirPlan) / CampaignMinutes;
                block = mins_til_takeoff / 5;

                if (block < tilblock and ( not rw or block % 2))
                {
                    CancelFlight((Flight)u);
                }
            }
        }

        u = GetNextUnit(&myit);
    }
}

ATMAirbaseClass* AirTaskingManagerClass::FindATMAirbase(VU_ID abid)
{
    ATMAirbaseClass *cur;

    cur = airbaseList;

    while (cur)
    {
        if (cur->id == abid)
            return cur;

        cur = cur->next;
    }

    return NULL;
}

ATMAirbaseClass* AirTaskingManagerClass::AddToAirbaseList(CampEntity ent)
{
    ATMAirbaseClass *cur;

    cur = airbaseList;

    while (cur)
    {
        if (cur->id == ent->Id())
        {
            cur->usage++;
            return cur;
        }

        cur = cur->next;
    }

    cur = new ATMAirbaseClass(ent);
    cur->next = airbaseList;
    cur->usage++;
    airbaseList = cur;
    return cur;
}

int AirTaskingManagerClass::FindNearestActiveTanker(GridIndex *x, GridIndex *y, CampaignTime *time)
{
    Int32 d, bd = 9999;

    if (packageList)
    {
        GridIndex bx = 0, by = 0;
        CampaignTime bt = 0;
        Package p;

        VuListIterator myit(packageList);
        p = (Package) myit.GetFirst();

        while (p)
        {
            if (p->GetMissionRequest()->mission == AMIS_TANKER and p->GetMissionRequest()->tot < *time and p->GetMissionRequest()->tot + (MissionData[AMIS_TANKER].loitertime * CampaignMinutes) > *time)
            {
                // It's a tanker mission and it's planning to be around when we need it
                d = FloatToInt32(Distance(*x, *y, p->GetMissionRequest()->tx, p->GetMissionRequest()->ty));

                if (d < bd)
                {
                    bd = d;
                    bx = p->GetMissionRequest()->tx;
                    by = p->GetMissionRequest()->ty;
                    bt = p->GetMissionRequest()->tot + (MissionData[AMIS_TANKER].loitertime * CampaignMinutes);
                }
            }

            p = (Package) myit.GetNext();
        }

        if (bt > 1)
        {
            *x = bx;
            *y = by;
            *time = bt;
        }
    }

    return bd;
}

int AirTaskingManagerClass::FindNearestActiveJammer(GridIndex *x, GridIndex *y, CampaignTime *time)
{
    return 0;
}

int AirTaskingManagerClass::Handle(VuFullUpdateEvent *event)
{
    AirTaskingManagerClass *tmpATM = (AirTaskingManagerClass*)(event->expandedData_.get());
    ATMAirbaseClass *cur, *next, *last = NULL;

    // Copy in new data
    memcpy(&flags, &tmpATM->flags, sizeof(short));

    CampEnterCriticalSection();

    // Really the only data we care about is the scheduling data.
    cur = tmpATM->airbaseList;
    airbaseList = NULL;

    while (cur)
    {
        next = new ATMAirbaseClass();
        next->id = cur->id;
        memcpy(next->schedule, cur->schedule, sizeof(uchar)*ATM_MAX_CYCLES);

        if ( not airbaseList)
            airbaseList = next;
        else
            last->next = next;

        last = next;
        cur = cur->next;
    }

    cycle = tmpATM->cycle;

    // Zap old list
    requestList->Purge();

    CampLeaveCriticalSection();
    return (VuEntity::Handle(event));
}

// =====================================
// Support functions
// =====================================

void RebuildATMLists(void)
{
    Unit u;
    int t, cycle;

    // Reset the threat search array, so we only target threats once per planning cycle.
    // memset(ThreatSearch,0,sizeof(uchar)*MAX_CAMP_ENTITIES);

    // Increment our cycle
    // sfr: @TODO remove JB check
    if (TeamInfo[0] and TeamInfo[0]->atm and not F4IsBadReadPtr(TeamInfo[0], sizeof(TeamClass)) and not F4IsBadReadPtr(TeamInfo[0]->atm, sizeof(AirTaskingManagerClass))) // JB 010220 CTD
        cycle = (TeamInfo[0]->atm->cycle + 1) % ATM_MAX_CYCLES;

    for (t = 0; t < NUM_TEAMS; t++)
    {
        // sfr: added mem check
        if (TeamInfo[t] and TeamInfo[t]->VuState() == VU_MEM_ACTIVE)
        {
            TeamInfo[t]->atm->squadrons = 0;
            TeamInfo[t]->atm->cycle = cycle;
            // Truncate schedule time to minute.
            TeamInfo[t]->atm->scheduleTime = TheCampaign.CurrentTime / CampaignMinutes;
            TeamInfo[t]->atm->scheduleTime *= CampaignMinutes;

            if (TeamInfo[t]->atm->squadronList)
            {
                TeamInfo[t]->atm->squadronList->Purge();
                TeamInfo[t]->atm->packageList->Purge();
            }
        }
    }

    // Rebuild list of available assets
    VuListIterator myit(AllAirList);
    u = GetFirstUnit(&myit);

    while (u)
    {
        t = u->GetTeam();

        // Since we know we have a squadron/package now, initialize the lists
        // if we don't have them
        if (TeamInfo[t])
        {
            if ( not TeamInfo[t]->atm->squadronList)
            {
                TeamInfo[t]->atm->squadronList = new FalconPrivateList(&AllAirFilter);
                TeamInfo[t]->atm->squadronList->Register();
                TeamInfo[t]->atm->packageList = new FalconPrivateList(&AllAirFilter);
                TeamInfo[t]->atm->packageList->Register();
            }

            // Add to the correct list;
            if (u->GetType() == TYPE_SQUADRON and not u->Scripted())
            {
                TeamInfo[t]->atm->squadronList->ForcedInsert(u);
                TeamInfo[t]->atm->squadrons++;
            }
            else if (u->GetType() == TYPE_PACKAGE)
            {
                TeamInfo[t]->atm->packageList->ForcedInsert(u);
            }
        }

        u = GetNextUnit(&myit);
    }
}

// This finds the predetermined location closest to the request mission location
void FindBestLocation(MissionRequest mis, List list)
{
    ListNode lp;
    void *loc;
    GridIndex x, y, bx = 0, by = 0;
    Int32 d, bd = 9999;

    lp = list->GetFirstElement();

    while (lp)
    {
        loc = (void*) lp->GetUserData();
        UnpackXY(loc, &x, &y);
        d = FloatToInt32(Distance(x, y, mis->tx, mis->ty));

        if (d < bd)
        {
            bx = x;
            by = y;
            bd = d;
        }

        lp = lp->GetNext();
    }

    mis->tx = bx;
    mis->ty = by;
}

void ShowMissionLists(void)
{
    int i;
    MissionRequest mis;
    ListNode lp;
    Package pack;

    for (i = 0; i < NUM_TEAMS; i++)
    {
        if (TeamInfo[i]->atm)
        {
            lp = TeamInfo[i]->atm->requestList->GetLastElement();

            while (lp)
            {
                mis = (MissionRequest) lp->GetUserData();
                MonoPrint("Team #%d (requ), Mission: %d at %d,%d - priority: %d, tot: %f\n", i, mis->mission, mis->tx, mis->ty, mis->priority, mis->tot);
                lp = lp->GetPrev();
            }

            Sleep(1000);

            if (TeamInfo[i]->atm->packageList)
            {
                VuListIterator packit(TeamInfo[i]->atm->packageList);
                MonoPrint("In progress:\n");
                pack = (Package) GetFirstUnit(&packit);

                while (pack)
                {
                    mis = pack->GetMissionRequest();
                    MonoPrint("Team #%d (plan), Mission: %d at %d,%d - priority: %d, tot: %f\n", i, mis->mission, mis->tx, mis->ty, mis->priority, mis->tot);
                    pack = (Package) GetNextUnit(&packit);
                }
            }

            Sleep(1000);
        }
    }
}

// Cleans up any packages which are a no-go -
// ie, eliminates children, sets as dead, removes from database
void ChillPackage(Package *pc)
{
    // CampEnterCriticalSection();
    (*pc)->SetDead(1);
    (*pc)->KillUnit();
    (*pc)->Remove();
    *pc = NULL;
    PackInserted = 0;
    // CampLeaveCriticalSection();
}

void RequestPlayerDivert(void)
{
    FalconAirTaskingMessage *msg;
    Flight pflight;

    pflight = FalconLocalSession->GetPlayerFlight();

    if (pflight)
    {
        // AirTaskingManagerClass::SendATMMessage(pflight->Id(), TheCampaign.PlayerTeam, FalconAirTaskingMessage::atmAssignDivert, 0, 0, NULL, 0);
        VuTargetEntity *target = (VuTargetEntity*) vuDatabase->Find(TeamInfo[FalconLocalSession->GetTeam()]->atm->OwnerId());
        msg = new FalconAirTaskingMessage(TeamInfo[FalconLocalSession->GetTeam()]->atm->Id(), target);
        msg->dataBlock.from = pflight->Id();
        msg->dataBlock.messageType = FalconAirTaskingMessage::atmAssignDivert;
        msg->dataBlock.team = FalconLocalSession->GetTeam();
        FalconSendMessage(msg, TRUE);
    }
}

int AlreadyPlanned(MissionRequestClass *mis, int who)
{
    // Check for repeat requests/missions
    // KCK NOTE: This assumes RequestIntercept will always be called by host machine
    if (TeamInfo[who] and TeamInfo[who]->atm)
    {
        MissionRequest pmis;

        // Check to see if a similar mission is already in progress, and cancel if so.
        if (TeamInfo[who]->atm->packageList)
        {
            VuListIterator packit(TeamInfo[who]->atm->packageList);
            Package pack = (Package) GetFirstUnit(&packit);

            while (pack)
            {
                pmis = pack->GetMissionRequest();

                if (pmis->mission == mis->mission and pmis->targetID == mis->targetID and not pack->Aborted())
                    return 1;

                pack = (Package) GetNextUnit(&packit);
            }
        }

        // Now check to see if a similar mission request is already on the queue
        if (TeamInfo[who]->atm->requestList)
        {
            ListNode lp = TeamInfo[who]->atm->requestList->GetFirstElement();

            while (lp)
            {
                pmis = (MissionRequest) lp->GetUserData();

                if (pmis->mission == mis->mission and pmis->targetID == mis->targetID)
                {
                    // Dump earlier requests
                    if (pmis->tot > mis->tot)
                        return 1;

                    ListNode nlp = lp->GetNext();
                    CampEnterCriticalSection();
                    TeamInfo[who]->atm->requestList->Remove(lp);
                    CampLeaveCriticalSection();
                    lp = nlp;
                }
                else
                    lp = lp->GetNext();
            }
        }
    }

    return 0;
}

int RequestSARMission(FlightClass* flight)
{
    MissionRequestClass mis;
    int rel;

    mis.requesterID = flight->Id();
    mis.who = flight->GetTeam();
    mis.tot = Camp_GetCurrentTime();
    mis.tot_type = TYPE_NE;
    mis.mission = AMIS_SAR;
    mis.context = friendlyRescueExpected;
    flight->GetLocation(&mis.tx, &mis.ty);

    // Check if it's already planned
    if (AlreadyPlanned(&mis, mis.who))
        return 1;

    // Check if we're to far into enemy territory
    rel = GetTTRelations(GetOwner(TheCampaign.CampMapData, mis.tx, mis.ty), mis.who);

    if (rel not_eq Friendly and rel not_eq Allied and DistanceToFront(mis.tx, mis.ty) > MAX_SAR_DIST)
        return 0;

    mis.RequestMission();
    return 1;
}

void RequestIntercept(FlightClass* enemy, int who, RequIntHint hint)
{
    MissionRequestClass mis;
    Package enemyPackage;
    Flight main;
    GridIndex x, y, nx, ny;
    WayPoint w;

    //TJL 11/16/03 The Help Request will now target BARCAPSs
    // 2001-10-27 MODIFIED BY S.G. Do bother with BARCAP if yelling for help
    //if (enemy->GetUnitMission() == AMIS_BARCAP or enemy->GetUnitMission() == AMIS_BARCAP2 or enemy->GetUnitMission() == AMIS_FAC)
    // 2001-12-17 M.N. changed back, don't think this is right at all...
    // if (hint == RI_HELP or (enemy->GetUnitMission() == AMIS_BARCAP or enemy->GetUnitMission() == AMIS_BARCAP2 or enemy->GetUnitMission() == AMIS_FAC))
    //return;
    /*
    // Don't bother intercepting FAC
    if (enemy->GetUnitMission() == AMIS_FAC)
    return;

    // Don't bother intercepting helecopter flights (this is admittidly hackish)
    if (enemy->IsHelicopter())
    return;*/

    //Cobra Cheat script
    if (hint not_eq RI_HELP)
    {
        int test = enemy->GetUnitMission();

        if (test not_eq AMIS_OCASTRIKE and 
            test not_eq AMIS_INTSTRIKE and 
            test not_eq AMIS_STRIKE and 
            test not_eq AMIS_DEEPSTRIKE and 
            test not_eq AMIS_STSTRIKE and 
            test not_eq AMIS_STRATBOMB and 
            test not_eq AMIS_SEADSTRIKE)
            return;
    }

    if (enemy->IsHelicopter())
        return;

    // We're tasking vs the entire incoming package, not just this flight.
    enemyPackage = (Package)enemy->GetUnitParent();

    if ( not enemyPackage)
        return;

    main = (Flight) enemyPackage->GetFirstUnitElement();

    if ( not main)
        main = enemy;

    // Don't intercept flights/packages which are aborting
    if (main->Aborted())
        return;

    main->GetLocation(&mis.tx, &mis.ty);
    mis.requesterID = enemy->Id();
    // Target vs package
    mis.targetID = enemyPackage->Id();
    mis.vs = enemy->GetTeam();
    mis.who = who;
    mis.tot = Camp_GetCurrentTime();
    mis.tot_type = TYPE_NE;
    mis.mission = AMIS_INTERCEPT;
    mis.roe_check = ROE_AIR_ATTACK;
    mis.context = interceptEnemyAircraft;
    mis.match_strength = GetUnitScore(enemyPackage, Air); // Take entire package's score
    mis.flags = AMIS_IMMEDIATE;

    // 2001-10-27 ADDED BY S.G. Flag the intercept has been an help request
    if (hint == RI_HELP)
        mis.flags or_eq AMIS_HELP_REQUEST;

    // END OF ADDED SECTION 2001-10-27

    // Check for repeat requests/missions
    // KCK NOTE: This assumes RequestIntercept will always be called by host machine
    // Experimental Try no checking for repeat intercepts
    // if (AlreadyPlanned(&mis, mis.who))
    // return;

    // We're going to store our heading in the min_to variable
    main->GetLocation(&x, &y);
    w = main->GetCurrentUnitWP();

    if ( not w)
    {
        // The main flight hasn't taken off yet, get it's first non-takeoff wp
        w = main->GetFirstUnitWP();

        if (w)
            w = w->GetNextWP();
    }

    ShiAssert(w);
    w->GetWPLocation(&nx, &ny);
    mis.min_to = DirectionTo(x, y, nx, ny);

    mis.RequestMission();
}

// This will target all sites of an appropriate type in the passed Primary Objective
int TargetAllSites(Objective po, int action, int team, CampaignTime startTime)
{
    Objective otarget, o;
    Unit utarget;
    MissionRequestClass mis;
    int requests = 0;

    if ( not action)
        return 0;

    // Standard shit
    mis.who = team;
    mis.tot_type = TYPE_EQ;
    mis.roe_check = ROE_AIR_ATTACK;
    mis.flags = REQF_PART_OF_ACTION;
    mis.action_type = action;

    // Target any appropriate objective targets
    {
        VuListIterator oit(AllObjList);
        otarget = (Objective) oit.GetFirst();

        while (otarget)
        {
            if (otarget->GetObjectiveStatus() > 30 and GetRoE(team, otarget->GetTeam(), ROE_AIR_ATTACK) == ROE_ALLOWED)
            {
                otarget->GetLocation(&mis.tx, &mis.ty);
                o = FindNearestObjective(POList, mis.tx, mis.ty, NULL);

                if (o == po)
                {
                    mis.requesterID = otarget->Id();
                    mis.targetID = otarget->Id();
                    mis.vs = otarget->GetTeam();
                    mis.priority = 0;
                    mis.target_num = 255;

                    if (
                        otarget->GetType() == TYPE_AIRBASE or
                        otarget->GetType() == TYPE_AIRSTRIP or
                        otarget->GetType() == TYPE_ARMYBASE
                    )
                    {
                        // Request enemy strike vs this airbase
                        mis.tot = startTime + 15 * CampaignMinutes;
                        mis.mission = AMIS_OCASTRIKE;

                        if (action == AACTION_OCA)
                            mis.context = AirActionOCA;
                        else
                            mis.context = AirActionPrepAB;

                        mis.RequestMission();
                        requests++;
                    }
                    else if (otarget->GetType() == TYPE_COM_CONTROL)
                    {
                        // Request enemy strike vs this C3 facility
                        mis.tot = startTime + 15 * CampaignMinutes;
                        mis.mission = AMIS_STRIKE;

                        if (action == AACTION_OCA)
                            mis.context = AirActionOCA;
                        else
                            mis.context = AirActionPrepAB;

                        mis.RequestMission();
                        requests++;
                    }
                    else if (otarget->GetType() == TYPE_RADAR)
                    {
                        // Generate a mission to take out the radar
                        ObjClassDataType *oc = otarget->GetObjectiveClassData();
                        mis.target_num = oc->RadarFeature;
                        mis.tot = startTime + 15 * CampaignMinutes;
                        mis.mission = AMIS_OCASTRIKE;

                        if (action == AACTION_OCA)
                            mis.context = AirActionOCA;
                        else
                            mis.context = AirActionPrepAB;

                        mis.RequestMission();
                        requests++;
                    }
                    //TJL 01/02/04 Remove INTERDICT mode condition.  That mode
                    //is not called correctly.  These missions types should be called
                    //like the rest.  The player should use the sliders to determine
                    //what should/should not be targeted.
                    //This will also fix the supply code that calls for attacks on the
                    //powergrid.
                    //if (action == AACTION_INTERDICT)
                    else if (otarget->GetType() == TYPE_FACTORY or
                             otarget->GetType() == TYPE_CHEMICAL or
                             otarget->GetType() == TYPE_DEPOT or
                             otarget->GetType() == TYPE_REFINERY or
                             otarget->GetType() == TYPE_PORT or
                             otarget->GetType() == TYPE_NUCLEAR or
                             otarget->GetType() == TYPE_POWERPLANT)
                    {
                        // Generate a mission to strike this facility
                        mis.tot = startTime + 25 * CampaignMinutes;
                        mis.mission = AMIS_INTSTRIKE;
                        mis.context = AirActionInterdiction;
                        mis.RequestMission();
                        requests++;
                    }
                    else if (otarget->GetType() == TYPE_BRIDGE and 
                             TeamInfo[team]->GetGroundAction()->actionObjective not_eq po->Id())
                    {
                        // Generate a mission to take out this bridge (Shouldn't happen
                        // if this primary is a target of a ground offensive)
                        mis.tot = startTime + 25 * CampaignMinutes;
                        mis.mission = AMIS_INTSTRIKE;
                        mis.context = AirActionInterdiction;
                        mis.RequestMission();
                        requests++;
                    }
                    // TJL 01/02/04 Remove the AACTION_CAS.  This is never called.
                    //if (action == AACTION_CAS)
                    else if (otarget->GetType() == TYPE_FORTIFICATION)
                    {
                        // Generate a mission to strike this facility
                        mis.tot = startTime + 25 * CampaignMinutes;
                        mis.mission = AMIS_INTSTRIKE;
                        mis.context = AirActionCAS;
                        mis.RequestMission();
                        requests++;
                    }
                }
            }

            otarget = (Objective) oit.GetNext();
        }
    }

    // Target Air defense assets
    {
        VuListIterator ait(AirDefenseList);
        utarget = (Unit) ait.GetFirst();

        while (utarget)
        {
            if (
                GetRoE(team, utarget->GetTeam(), ROE_AIR_ATTACK) == ROE_ALLOWED and 
                utarget->GetElectronicDetectionRange(Air) > 0
            )
            {
                utarget->GetLocation(&mis.tx, &mis.ty);
                o = FindNearestObjective(POList, mis.tx, mis.ty, NULL);

                if (o == po)
                {
                    // Request enemy strike vs this air defense asset
                    mis.requesterID = utarget->Id();
                    mis.targetID = utarget->Id();
                    mis.vs = utarget->GetTeam();
                    mis.tot = startTime + 5 * CampaignMinutes;
                    mis.mission = AMIS_SEADSTRIKE;
                    mis.priority = 0;
                    mis.context = AirActionPrepAD;
                    mis.RequestMission();
                    requests++;
                }
            }

            utarget = (Unit) ait.GetNext();
        }
    }

    // Set up a few sweep missions
    mis.requesterID = FalconNullId;
    mis.targetID = FalconNullId;
    mis.vs = po->GetTeam();
    mis.mission = AMIS_SWEEP;
    mis.context = AirActionPrepAir;
    mis.tot = startTime + 5 * CampaignMinutes;
    mis.priority = 0;
    po->GetLocation(&mis.tx, &mis.ty);
    mis.RequestMission();
    requests++;
    mis.tot = startTime + 15 * CampaignMinutes;
    mis.priority = 0;
    mis.RequestMission();
    requests++;
    mis.tot = startTime + 25 * CampaignMinutes;
    mis.priority = 0;
    mis.RequestMission();
    requests++;

    // KCK TODO?: CAS actions -> BAI Interdiction?
    return requests;
}

extern void CheckForClimb(WayPoint w);
void recalculate_waypoint_list(WayPointClass *wp, int minSpeed, int maxSpeed);

void RecalculateWaypoint(WayPointClass *w, CampaignTime newDeparture)
{
    GridIndex x, y, nx, ny, tx, ty;
    float dist, newDist, newSpeed, plannedSpeed;
    int time, legDist;
    WayPoint nw, tw, cw;
    CampaignHeading h;

    // Let the waypoint recalculation routine do all the work
    nw = w->GetNextWP();
    plannedSpeed = nw->GetWPSpeed();
    w->SetWPTimes(newDeparture);
    recalculate_waypoint_list(w, 0, FloatToInt32(plannedSpeed * 1.2F));

    // Check for errors
    newSpeed = nw->GetWPSpeed();

    if (newSpeed < plannedSpeed * 0.5F)
    {
        // Find the assembly point (if one exists) and distance
        cw = w;
        nw = w->GetNextWP();
        dist = cw->DistanceTo(nw);

        while (nw and not (nw->GetWPFlags() bitand WPF_ASSEMBLE))
        {
            dist += cw->DistanceTo(nw);
            cw = nw;
            nw = nw->GetNextWP();
        }

        if ( not nw)
        {
            nw = w->GetNextWP();
            dist += cw->DistanceTo(nw);
        }

        time = nw->GetWPArrivalTime() - newDeparture;
        newDist = (plannedSpeed * 0.6F * time) / CampaignHours;
        legDist = FloatToInt32(((newDist - dist) + 1) / 2);

        if (legDist > 2)
        {
            // We're going to add two timing waypoints in front of the assembly point.
            // First, add one legDist km away from the assembly point in the
            // general direction of the previous waypoint.
            nw->GetWPLocation(&nx, &ny);
            cw = nw->GetPrevWP();
            cw->GetWPLocation(&x, &y);
            h = DirectionTo(nx, ny, x, y);
            tx = nx + legDist * dx[h];
            ty = ny + legDist * dy[h];
            // Add the first waypoint
            tw = new WayPointClass(tx, ty, nw->GetWPAltitude(), FloatToInt32(plannedSpeed * 0.6F), 0, 0, WP_TIMING, 0);
            tw->SetWPRouteAction(nw->GetWPRouteAction());
            cw->InsertWP(tw);
            // Add the second waypoint (at assembly point)
            tw = new WayPointClass(nx, ny, nw->GetWPAltitude(), FloatToInt32(plannedSpeed * 0.6F), 0, 0, WP_TIMING, 0);
            tw->SetWPRouteAction(nw->GetWPRouteAction());
            cw->InsertWP(tw);
            CheckForClimb(tw);
            CheckForClimb(tw->GetNextWP());
            recalculate_waypoint_list(w, FloatToInt32(plannedSpeed * 0.5F), FloatToInt32(plannedSpeed * 1.2F));
        }
    }
    else if (newSpeed > plannedSpeed * 1.2F)
    {
        // Find the assembly point (if one exists), clear the timing lock, and recalculate
        nw = w->GetNextWP();
        plannedSpeed = nw->GetWPSpeed();

        while (nw and not (nw->GetWPFlags() bitand WPF_ASSEMBLE))
            nw = nw->GetNextWP();

        if ( not nw)
            nw = w->GetNextWP();

        nw->UnSetWPFlag(WPF_TIME_LOCKED);
        recalculate_waypoint_list(w, FloatToInt32(plannedSpeed * 0.8F), FloatToInt32(plannedSpeed * 1.2F));
    }
}


/*


//
w->GetWPLocation(&x,&y);
nw->GetWPLocation(&nx,&ny);
dist = Distance(x,y,nx,ny);
time = nw->GetWPArrivalTime() - newDeparture;
plannedSpeed = (dist*CampaignHours)/(nw->GetWPArrivalTime() - w->GetWPDepartureTime());
newSpeed = (dist*CampaignHours)/(time);
w->SetWPTimes(newDeparture);
// If recalculating caused speed to be less than our min cruise speed, add a timing leg
if (newSpeed < plannedSpeed*0.9F)
{
// Determine distance we need to travel, and add a timing waypoint if it's far enough
newDist = (plannedSpeed*time)/CampaignHours;
legDist = FloatToInt32(((newDist-dist)+1)/2);
if (legDist > 2)
{
// We're going to add two timing waypoints in front of the assembly point.
// First, add the one which will legDist km away from the assembly point in the
// general direction of the previous waypoint
arr = w->GetWPDepartureTime() + FloatToInt32((legDist*CampaignHours)/plannedSpeed);
tw = new WayPointClass (x+legDist, y, nw->GetWPAltitude(), plannedSpeed, arr, 0, WP_NOTHING, 0);
tw->SetWPRouteAction(nw->GetWPRouteAction());
w->InsertWP(tw);
CheckForClimb(tw);
cw = tw;
arr = w->GetWPDepartureTime() + FloatToInt32((2.0F*legDist*CampaignHours)/plannedSpeed);
tw = new WayPointClass (x, y, nw->GetWPAltitude(), plannedSpeed, arr, 0, WP_NOTHING, 0);
tw->SetWPRouteAction(nw->GetWPRouteAction());
cw->InsertWP(tw);
CheckForClimb(tw);
newSpeed = (dist*CampaignHours)/(nw->GetWPArrivalTime()-arr);
}
nw->SetWPSpeed(newSpeed);
}
else if (newSpeed > plannedSpeed*1.1F)
{
// Try to redistribute the time through the entire route
// by calling the recalculate_waypoint_list function after
// clearing any possible timing locks on the next waypoint
nw->UnSetWPFlag(WPF_TIME_LOCKED);
recalculate_waypoint_list(w, FloatToInt32(plannedSpeed*0.8F), FloatToInt32(plannedSpeed*1.2F));
}
else
{
// recalculate_waypoint_list(w, FloatToInt32(plannedSpeed*0.8F), FloatToInt32(plannedSpeed*1.2F));
nw->SetWPSpeed(newSpeed);
}
}



nw = w->GetNextWP();
w->GetWPLocation(&x,&y);
nw->GetWPLocation(&nx,&ny);
dist = Distance(x,y,nx,ny);
time = nw->GetWPArrivalTime() - newDeparture;
plannedSpeed = (dist*CampaignHours)/(nw->GetWPArrivalTime() - w->GetWPDepartureTime());
newSpeed = (dist*CampaignHours)/(time);
w->SetWPTimes(newDeparture);
// If recalculating caused speed to be less than our min cruise speed, add a timing leg
if (newSpeed < plannedSpeed*0.9F)
{
// Determine distance we need to travel, and add a timing waypoint if it's far enough
newDist = (plannedSpeed*time)/CampaignHours;
legDist = FloatToInt32(((newDist-dist)+1)/2);
if (legDist > 2)
{
// We're going to add two timing waypoints in front of the assembly point.
// First, add the one which will legDist km away from the assembly point in the
// general direction of the previous waypoint
arr = w->GetWPDepartureTime() + FloatToInt32((legDist*CampaignHours)/plannedSpeed);
tw = new WayPointClass (x+legDist, y, nw->GetWPAltitude(), plannedSpeed, arr, 0, WP_NOTHING, 0);
tw->SetWPRouteAction(nw->GetWPRouteAction());
w->InsertWP(tw);
CheckForClimb(tw);
cw = tw;
arr = w->GetWPDepartureTime() + FloatToInt32((2.0F*legDist*CampaignHours)/plannedSpeed);
tw = new WayPointClass (x, y, nw->GetWPAltitude(), plannedSpeed, arr, 0, WP_NOTHING, 0);
tw->SetWPRouteAction(nw->GetWPRouteAction());
cw->InsertWP(tw);
CheckForClimb(tw);
newSpeed = (dist*CampaignHours)/(nw->GetWPArrivalTime()-arr);
}
nw->SetWPSpeed(newSpeed);
}
else if (newSpeed > plannedSpeed*1.1F)
{
 // Try to redistribute the time through the entire route
 // by calling the recalculate_waypoint_list function after
 // clearing any possible timing locks on the next waypoint
 nw->UnSetWPFlag(WPF_TIME_LOCKED);
 recalculate_waypoint_list(w, FloatToInt32(plannedSpeed*0.8F), FloatToInt32(plannedSpeed*1.2F));
}
else
{
 // recalculate_waypoint_list(w, FloatToInt32(plannedSpeed*0.8F), FloatToInt32(plannedSpeed*1.2F));
 nw->SetWPSpeed(newSpeed);
}
}
*/
