#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include "CmpGlobl.h"
#include "ListADT.h"
#include "F4Vu.h"
#include "vutypes.h"
#include "Objectiv.h"
#include "Strategy.h"
#include "Unit.h"
#include "Find.h"
#include "Path.h"
#include "ASearch.h"
#include "Campaign.h"
#include "Update.h"
#include "f4vu.h"
#include "CampList.h"
#include "gtm.h"
#include "team.h"
#include "gndunit.h"
#include "gtmobj.h"
#include "Manager.h"
#include "MsgInc/GndTaskingMsg.h"
#include "MsgInc/CampDataMsg.h"
#include "AIInput.h"
#include "Division.h"
#include "CmpClass.h"
#include "ThreadMgr.h"
#include "classtbl.h"
#include "FalcSess.h"
#include "brief.h"
#include "CampStr.h"

//sfr: added buffer chk
#include "InvalidBufferException.h"

#include "Debuggr.h"

// =====================
// Debug Mode
// =====================

#ifdef DEBUG
//#define KEV_GDEBUG 0
#endif

extern void debugprintf(LPSTR dbgFormat, ...);

//#define MonoPrint  debugprintf

// ======================================================================
// Externals
// ======================================================================

extern char* UnitTypeStr(Unit u, int stype, char buffer[]);
extern int FindUnitSupportRole(Unit u);
extern costtype CostToArrive(Unit u, int orders, GridIndex x, GridIndex y, Objective t);

// ======================================================================
// GTM (Ground Tasking Manager) Build ground tasking orders for each team
// ======================================================================

// Assignedment modes
#define GTM_MODE_BEST 0 // Find the best unit for these orders, regardless of distance
#define GTM_MODE_FASTEST 1 // Find a reasonable unit who can get there the fastest.

#define MAX_GTMU_SCORE 32000
#define MIN_ALLOWABLE_ROLE_SCORE 10

#define COLLECTABLE_HP_OBJECTIVES 5

#define COLLECT_RESERVE 0x01
#define COLLECT_CAPTURE 0x02
#define COLLECT_SECURE 0x04
#define COLLECT_ASSAULT 0x08
#define COLLECT_AIRBORNE 0x10
#define COLLECT_COMMANDO 0x20
#define COLLECT_DEFEND 0x40
#define COLLECT_SUPPORT 0x80
#define COLLECT_REPAIR 0x100
#define COLLECT_AIRDEFENSE 0x200
#define COLLECT_RECON 0x400
#define COLLECT_RADAR 0x800

float DM, IM;
int Assigned;
int AssignmentDistance;

int sOffensiveAssigned, sOffensiveDesired;

extern char OrderStr[GORD_LAST][15];

#ifdef KEV_GDEBUG
int Priority, ReSearch, LastDiv;
int ObjCount[GORD_LAST], UnitCount[GORD_LAST], AssignedCount[GORD_LAST];
char GTMBuf[40];
ulong Time[GORD_LAST], ScoreTime, PickTime, ListBuildTime;
#endif

int ScoreObjectiveOffensive(float uod, float odd, float ofd, float ufd, float im, float sm, int basescore, int priority);
int ScoreObjectiveDefensive(float uod, float odd, float ofd, float ufd, float im, float sm, int basescore, int priority);
int GetTopPriorityObjectives(int team, _TCHAR* buffers[COLLECTABLE_HP_OBJECTIVES]);
void CleanupUnitlist(VuLinkedList* unitList);

// =====================
// Class Functions
// =====================

// constructors
GroundTaskingManagerClass::GroundTaskingManagerClass(ushort type, Team t) : CampManagerClass(type, t)
{
    flags = 0;
    topPriority = 0;
    memset(canidateList, 0, sizeof(void*)*GORD_LAST);
    memset(objList, 0, sizeof(void*)*GORD_LAST);
}

GroundTaskingManagerClass::GroundTaskingManagerClass(VU_BYTE **stream, long *rem) : CampManagerClass(stream, rem)
{
    memcpychk(&flags, stream, sizeof(short), rem);
    topPriority = 0;
    memset(canidateList, 0, sizeof(void*)*GORD_LAST);
    memset(objList, 0, sizeof(void*)*GORD_LAST);
}

GroundTaskingManagerClass::GroundTaskingManagerClass(FILE *file) : CampManagerClass(file)
{
    fread(&flags, sizeof(short), 1, file);
    topPriority = 0;
    memset(canidateList, 0, sizeof(void*)*GORD_LAST);
    memset(objList, 0, sizeof(void*)*GORD_LAST);
}

GroundTaskingManagerClass::~GroundTaskingManagerClass()
{
}

int GroundTaskingManagerClass::SaveSize(void)
{
    return CampManagerClass::SaveSize()
           + sizeof(short);
}

int GroundTaskingManagerClass::Save(VU_BYTE **stream)
{
    CampManagerClass::Save(stream);
    memcpy(*stream, &flags, sizeof(short));
    *stream += sizeof(short);
    return GroundTaskingManagerClass::SaveSize();
}

int GroundTaskingManagerClass::Save(FILE *file)
{
    int retval = 0;

    if ( not file)
        return 0;

    retval += CampManagerClass::Save(file);
    retval += fwrite(&flags, sizeof(short), 1, file);
    return retval;
}

void GroundTaskingManagerClass::DoCalculations(void)
{
    Objective o;
    int score, fs, es, i;
    float d;
    Team t;
    GridIndex x, y;
    POData pd;

    // Don't do this if we're not active, or not owned by this machine
    if ( not (TeamInfo[owner]->flags bitand TEAM_ACTIVE) or not IsLocal())
        return;

    topPriority = 0;
    VuListIterator poit(POList);
    o = GetFirstObjective(&poit);

    while (o not_eq NULL)
    {
        // Get score for proximity to front
        o->GetLocation(&x, &y);
        d = DistanceToFront(x, y);
        fs = FloatToInt32((200.0F - d) * 0.2F);

        t = o->GetTeam();
        pd = GetPOData(o);
        es = 0;

        // Get score for enemy strength
        if (d < 100.0F)
        {
            for (i = 1; i < NUM_TEAMS; i++)
            {
                if (GetRoE(owner, i, ROE_GROUND_FIRE))
                    es += pd->ground_assigned[i] / 50; // 1 assignment pt = 1 vehicle, so 1 enemy strength pt per 50 vehs..
            }

            if (es > 30)
                es = 30; // Cap enemy strength after 1500 vehicles

            if (owner not_eq t)
                es = -es + (rand() % 5) - 2;
        }

        score = fs + es + (rand() % 5);

        if (o->GetObjectivePriority() > 95)
            score += 50;

        if (o->GetObjectivePriority() > 90)
            score += 20;
        else
            score += o->GetObjectivePriority() - 80;

        // os = (o->GetObjectivePriority()-80)*3;
        // score = os + fs + es + (rand()%5);

        if (score < 0)
            score = 0;

        if (score > 100)
            score = 100;

        // Minimum of 1 priority if it's owned by us.
        if ( not score and t == owner)
            score = 1;

        // KCK: AI's air and ground priorities are identical for now
        if ( not (pd->flags bitand GTMOBJ_SCRIPTED_PRIORITY))
        {
            pd->ground_priority[owner] = score;
            pd->air_priority[owner] = score;
            // KCK: player_priority only used now if it's >= 0
            // if ( not (pd->flags bitand GTMOBJ_PLAYER_SET_PRIORITY))
            // pd->player_priority[owner] = pd->air_priority[owner];
        }

        if ( not GetRoE(owner, t, ROE_GROUND_CAPTURE) and owner not_eq t)
            pd->ground_priority[owner] = 0;

        if ( not GetRoE(owner, t, ROE_AIR_ATTACK) and owner not_eq t)
            pd->air_priority[owner] = 0;

        if (score > topPriority)
        {
            topPriority = score;
            priorityObj = o->GetCampID();
        }

        o = GetNextObjective(&poit);
    }
}

int GroundTaskingManagerClass::Task(void)
{
    int done = 0;
    int count = 0, collect;
    int action;

    // Don't do this if we're not active, or not owned by this machine
    if ( not (TeamInfo[owner]->flags bitand TEAM_ACTIVE) or not IsLocal())
        return 0;

    action = TeamInfo[owner]->GetGroundActionType();

    // Check for offensive grinding to a halt
    if (action == GACTION_OFFENSIVE and TeamInfo[owner]->GetGroundAction()->actionPoints == 0)
    {
        TeamInfo[owner]->SelectGroundAction();
        action = TeamInfo[owner]->GetGroundActionType();
    }

#ifdef DEBUG
    ulong time;//,newtime;
    time = GetTickCount();
#endif

    Cleanup();

    // Choose types of orders we can give
    collect = COLLECT_AIRDEFENSE bitor COLLECT_SUPPORT bitor COLLECT_REPAIR bitor COLLECT_RESERVE bitor COLLECT_DEFEND bitor COLLECT_RADAR;

    if (action == GACTION_OFFENSIVE)
        collect or_eq COLLECT_CAPTURE bitor COLLECT_ASSAULT bitor COLLECT_AIRBORNE bitor COLLECT_COMMANDO bitor COLLECT_SECURE;
    else if (action == GACTION_MINOROFFENSIVE)
        collect or_eq COLLECT_SECURE;
    else if (action == GACTION_CONSOLIDATE)
        collect or_eq COLLECT_SECURE;

#ifdef KEV_GDEBUG
    ulong ltime;
    ltime = GetTickCount();
#endif

    if (CollectGroundAssets(collect))
    {
#ifdef KEV_GDEBUG
        ListBuildTime = GetTickCount() - ltime;
#endif

        // Give orders based on action type
        switch (action)
        {
            case GACTION_OFFENSIVE:
                // Full offensive - priorities are offensive, securing, then defense
                AssignUnits(GORD_CAPTURE, GTM_MODE_FASTEST);
                // if (NavalSuperiority(owner) >= STATE_CONTESTED)
                AssignUnits(GORD_ASSAULT, GTM_MODE_BEST);
                // if (AirSuperiority(owner) >= STATE_CONTESTED)
                {
                    AssignUnits(GORD_AIRBORNE, GTM_MODE_BEST);
                    AssignUnits(GORD_COMMANDO, GTM_MODE_BEST);
                }
                AssignUnits(GORD_SECURE, GTM_MODE_FASTEST);
                AssignUnits(GORD_DEFEND, GTM_MODE_BEST);
                break;

            case GACTION_MINOROFFENSIVE:
                // Consolidation/Counterattack phase - priorities are securing objectives then defense
                AssignUnits(GORD_SECURE, GTM_MODE_BEST);
                AssignUnits(GORD_DEFEND, GTM_MODE_BEST);

            case GACTION_CONSOLIDATE:
                // Cautious consolidation phase - priorities are defense, then securing objectives
                AssignUnits(GORD_DEFEND, GTM_MODE_FASTEST);
                AssignUnits(GORD_SECURE, GTM_MODE_BEST);
                break;

            case GACTION_DEFENSIVE:
            default:
                // Defensive posture - priorities are defense only
                AssignUnits(GORD_DEFEND, GTM_MODE_FASTEST);
                break;
        }

        // Now do the things we do all the time:
        AssignUnits(GORD_AIRDEFENSE, GTM_MODE_FASTEST);
        AssignUnits(GORD_SUPPORT, GTM_MODE_FASTEST);
        AssignUnits(GORD_REPAIR, GTM_MODE_FASTEST);
        AssignUnits(GORD_RADAR, GTM_MODE_FASTEST);
        AssignUnits(GORD_RESERVE, GTM_MODE_BEST);
    }

    // Check if our tasking failed to meet at least 50 of our offensive requests
    if (sOffensiveDesired and sOffensiveAssigned < sOffensiveDesired / 2)
        TeamInfo[owner]->GetGroundAction()->actionPoints = 0;

#ifdef KEV_GDEBUG
    int i;
    MonoPrint("Assigned:     ");

    for (i = 0; i < GORD_LAST; i++)
        MonoPrint("%3d  ", AssignedCount[i]);

    MonoPrint("%3d\n", Assigned);
    MonoPrint("Time (s):     ");

    for (i = 0; i < GORD_LAST; i++)
        MonoPrint("%3.1f  ", (float)(Time[i] / 1000.0F));

    MonoPrint("\n");
#endif

    Cleanup();

#ifdef KEV_GDEBUG
    newtime = GetTickCount();
    MonoPrint("Ground tasking for team %d: %d ms\n", owner, newtime - time);
#endif

    return Assigned;
}

void GroundTaskingManagerClass::Setup(void)
{
    for (int i = 0; i < GORD_LAST; i++)
    {
        canidateList[i] = NULL;
        objList[i] = NULL;
    }

#ifdef KEV_GDEBUG
    ScoreTime = PickTime = ListBuildTime;
#endif
    Assigned = 0;
}

void GroundTaskingManagerClass::Cleanup(void)
{
    for (int i = 0; i < GORD_LAST; i++)
    {
        if (canidateList[i])
            canidateList[i] = canidateList[i]->Purge();

        if (objList[i])
            objList[i] = objList[i]->Purge();

#ifdef KEV_GDEBUG
        ObjCount[i] = 0;
        UnitCount[i] = 0;
        AssignedCount[i] = 0;
        Time[i] = 0;
#endif
    }

    sOffensiveAssigned = sOffensiveDesired = 0;
    Assigned = 0;
}

// Determine if this objective can accept the passed orders
int GroundTaskingManagerClass::IsValidObjective(int orders, Objective o)
{
    if ( not o)
        return 0;

    switch (orders)
    {
        case GORD_CAPTURE:
            if (o->IsSecondary() and o->IsNearfront() and GetRoE(owner, o->GetTeam(), ROE_GROUND_CAPTURE) == ROE_ALLOWED)
                return 1;

            break;

        case GORD_SECURE:
            if (o->IsSecondary() and owner == o->GetTeam() and (o->IsFrontline() or o->IsSecondline()))
                return 1;

            break;

        case GORD_ASSAULT:
            if (o->IsSecondary() and not o->IsNearfront() and o->IsBeach() and GetRoE(owner, o->GetTeam(), ROE_GROUND_CAPTURE) == ROE_ALLOWED)
                return 1;

            break;

        case GORD_AIRBORNE:
            if (o->IsSecondary() and not o->IsNearfront() and GetRoE(owner, o->GetTeam(), ROE_GROUND_CAPTURE) == ROE_ALLOWED) // and not defended)
                return 1;

            break;

        case GORD_COMMANDO:
            if (o->CommandoSite() and GetRoE(owner, o->GetTeam(), ROE_GROUND_CAPTURE) == ROE_ALLOWED)
                return 1;

            break;

        case GORD_DEFEND:
            if (o->IsSecondary() and o->IsNearfront() and owner == o->GetTeam() and not o->Abandoned())
                return 1;

            break;

        case GORD_SUPPORT:
            if (owner == o->GetTeam() and o->ArtillerySite())
                return 1;

            break;

        case GORD_REPAIR:
            if (owner == o->GetTeam() and o->NeedRepair() and o->GetObjectiveStatus() < 51)
                return 1;

            break;

        case GORD_AIRDEFENSE:
            if (owner == o->GetTeam() and o->SamSite())
                return 1;

            break;

        case GORD_RECON:
            return 0;
            break;

        case GORD_RADAR:
            if (owner == o->GetTeam() and o->RadarSite())
                return 1;

            break;

        case GORD_RESERVE:
        default:
            if (o->IsSecondary() and owner == o->GetTeam() and not o->IsNearfront())
                return 1;

            break;
    }

    return 0;
}

// KCK: This is an admittidly hackish way of determining which lists to add the objective to,
// but it's either this or call IsValidObjective() for each order type which would require many
// more checks.. HOWEVER, we need to keep this and IsValidObjective() in sync (they must agree)
int GroundTaskingManagerClass::GetAddBits(Objective o, int to_collect)
{
    int add_now = to_collect;

    if ( not o)
        return 0;

    if ( not o->IsSecondary())
        add_now and_eq compl (COLLECT_RESERVE bitor COLLECT_CAPTURE bitor COLLECT_SECURE bitor COLLECT_ASSAULT bitor COLLECT_AIRBORNE bitor COLLECT_DEFEND);

    if (o->IsNearfront())
        add_now and_eq compl (COLLECT_RESERVE bitor COLLECT_ASSAULT bitor COLLECT_AIRBORNE);
    else
        add_now and_eq compl (COLLECT_CAPTURE bitor COLLECT_DEFEND);

    if (owner not_eq o->GetTeam())
        add_now and_eq compl (COLLECT_RESERVE bitor COLLECT_SECURE bitor COLLECT_DEFEND bitor COLLECT_SUPPORT bitor COLLECT_REPAIR bitor COLLECT_AIRDEFENSE bitor COLLECT_RADAR);

    if (GetRoE(owner, o->GetTeam(), ROE_GROUND_CAPTURE) not_eq ROE_ALLOWED)
        add_now and_eq compl (COLLECT_CAPTURE bitor COLLECT_ASSAULT bitor COLLECT_AIRBORNE bitor COLLECT_COMMANDO);

    if (o->Abandoned())
        add_now and_eq compl COLLECT_DEFEND;

    if ( not o->IsFrontline() and not o->IsSecondline())
        add_now and_eq compl (COLLECT_SECURE);

    if ( not o->IsBeach())
        add_now and_eq compl (COLLECT_ASSAULT);

    if (1) // defended
        add_now and_eq compl (COLLECT_AIRBORNE);

    if ( not o->CommandoSite())
        add_now and_eq compl (COLLECT_COMMANDO);

    if ( not o->ArtillerySite())
        add_now and_eq compl (COLLECT_SUPPORT);

    if ( not o->NeedRepair() or o->GetObjectiveStatus() > 50)
        add_now and_eq compl (COLLECT_REPAIR);

    if ( not o->SamSite())
        add_now and_eq compl (COLLECT_AIRDEFENSE);

    if ( not o->RadarSite())
        add_now and_eq compl (COLLECT_RADAR);

    return add_now;
}

int ScoreObj(int orders, int os, int ss, int ps, int pps, int fs)
{
    switch (orders)
    {
        case GORD_CAPTURE:
        case GORD_SECURE:
        case GORD_ASSAULT:
        case GORD_AIRBORNE:
        case GORD_DEFEND:
        case GORD_RECON:
            return os + ss + ps - fs;
            break;

        case GORD_SUPPORT:
            return os - fs;
            break;

        case GORD_COMMANDO:
        case GORD_RADAR:
            return os;
            break;

        case GORD_REPAIR:
            return os + ss + ps;
            break;

        case GORD_AIRDEFENSE:
            return os + ss + ps;
            break;

        case GORD_RESERVE:
        default:

            // KCK EXPERIMENTAL: Only assign reserve objectives which are near the front
            if (fs > 60 or fs < 20)
                return 0;

            // END EXPERIMENTAL
            return ss + ps * 2 - fs * 4;
            break;
    }

    return 0;
}

int GroundTaskingManagerClass::BuildObjectiveLists(int to_collect)
{
    Objective o = NULL, so = NULL, po = NULL;
    int add_now = 0, os = 0, ss = 0, ps = 0, pps = 0, fs = 0;

    // Create the objective lists
    VuListIterator objit(AllObjList);
    o = GetFirstObjective(&objit);

    while (o)
    {
        add_now = GetAddBits(o, to_collect);

        if (add_now)
        {
            so = po = o;

            if ( not so->IsSecondary())
                so = o->GetObjectiveParent();

            if ( not so)
            {
                po = NULL;
                ps = os = ss = 0;
            }
            else if ( not po->IsPrimary())
                po = so->GetObjectiveParent();

            os = o->GetObjectivePriority();

            if (so)
                ss = so->GetObjectivePriority();

            if (po)
            {
                POData pd = GetPOData(po);
                pps = pd->ground_priority[owner];
                ps = po->GetObjectivePriority();
            }

            if (add_now bitand (COLLECT_RESERVE bitor COLLECT_CAPTURE bitor COLLECT_SECURE bitor COLLECT_ASSAULT bitor COLLECT_AIRBORNE bitor COLLECT_DEFEND bitor GORD_SUPPORT))
            {
                GridIndex ox, oy;
                o->GetLocation(&ox, &oy);
                fs = FloatToInt32(DistanceToFront(ox, oy));
            }

            // Now insert it in the proper lists
            for (int i = 0; i < GORD_LAST; i++)
            {
                if ( not (add_now bitand (0x01 << i)))
                    continue;

                if (i == GORD_CAPTURE)
                    sOffensiveDesired++;

#ifdef KEV_GDEBUG
                ObjCount[i]++;
#endif
                GODNode new_node = new GndObjDataType();
                new_node->obj = o;
                new_node->priority_score = ScoreObj(i, os, ss, ps, pps, fs);

                if ( not objList[i])
                    objList[i] = new_node;

                objList[i] = objList[i]->Insert(new_node, GODN_SORT_BY_PRIORITY);

                // KCK EXPERIMENTAL: Try adding certain objectives twice
                if (i == GORD_CAPTURE and TeamInfo[owner]->GetGroundActionType() == GACTION_OFFENSIVE and TeamInfo[owner]->GetGroundAction()->actionObjective == o->GetObjectivePrimary()->Id())
                {
                    new_node = new GndObjDataType();
                    new_node->obj = o;
                    new_node->priority_score = ScoreObj(i, os, ss, ps, pps, fs);
                    objList[i] = objList[i]->Insert(new_node, GODN_SORT_BY_PRIORITY);
                }

                // END EXPERIMENTAL
            }
        }

        o = GetNextObjective(&objit);
    }

    return 1;
}

void GroundTaskingManagerClass::AddToList(Unit u, int orders)
{
    USNode curu;

    if (orders == GORD_CAPTURE)
        sOffensiveAssigned++;

#ifdef KEV_GDEBUG
    UnitCount[orders]++;
#endif
    curu = new UnitScoreNode;
    curu->unit = u;
    curu->score = u->GetUnitRoleScore(GetGroundRole(orders), CALC_MAX, USE_VEH_COUNT bitor IGNORE_BROKEN);

    if ( not canidateList[orders])
        canidateList[orders] = curu;

    canidateList[orders] = canidateList[orders]->Insert(curu, USN_SORT_BY_SCORE);
}

void GroundTaskingManagerClass::AddToLists(Unit u, int to_collect)
{
    int i, role;

    // Units with valid orders are not reassigned
    if (u->GetUnitOrders() not_eq GRO_RESERVE)
    {
        int orders = u->GetUnitOrders();

        if ((to_collect bitand (0x01 << orders)) and IsValidObjective(orders, u->GetUnitObjective()))
        {
            Objective o = u->GetUnitObjective();
            GODNode curo = objList[orders];

            while (curo)
            {
                if (curo->obj == o)
                {
                    if (orders == GORD_CAPTURE)
                        sOffensiveAssigned++;

#ifdef KEV_GDEBUG
                    UnitCount[orders]++;
#endif
                    AssignUnit(u, orders, o, 999);

                    // Their objective is removed from the satisfy list
                    if (objList[orders])
                        objList[orders] = objList[orders]->Remove(o);

                    return;
                }

                curo = curo->next;
            }
        }
    }

    // Immobile units just do what they do best..
    if (u->GetMovementType() == NoMove)
    {
        GridIndex x, y;
        Objective o;
        float d = -1.0F;
        u->GetLocation(&x, &y);
        o = FindNearestObjective(x, y, &d);

        if ( not o or d > 2.0F or GetRoE(o->GetTeam(), owner, ROE_GROUND_FIRE) == ROE_ALLOWED)
        {
            // Overrun
            u->KillUnit();
            return;
        }

        i = GetGroundOrders(u->GetUnitNormalRole());
#ifdef KEV_GDEBUG
        UnitCount[i]++;
#endif
        AssignUnit(u, i, o, 999);

        if (objList[i])
            objList[i] = objList[i]->Remove(o);

        return;
    }

    // Broken/unsupplied units get tasked as reserve only
    if (u->Broken() or u->GetUnitSupply() < 50)
    {
        AddToList(u, GORD_RESERVE);
        return;
    }

    u->SetUnitOrders(GORD_RESERVE);
    u->SetAssigned(0);

    // Check for one role units
    role = u->GetUnitNormalRole();

    if (role == GRO_FIRESUPPORT or role == GRO_AIRDEFENSE or role == GRO_ENGINEER) // KCK: Radar units here?
    {
        AddToList(u, GetGroundOrders(role));
        AddToList(u, GORD_RESERVE);
        return;
    }

    // Add it to a list for each type of orders it's capible of performing
    for (i = 0; i < GORD_LAST; i++)
    {
        if ( not (to_collect bitand (0x01 << i)))
            continue;

        if (i == GORD_ASSAULT and u->GetUnitNormalRole() not_eq GRO_ASSAULT)
            continue;

        if (i == GORD_COMMANDO and not u->Commando())
            continue;

        if (i == GORD_AIRBORNE and u->GetUnitNormalRole() not_eq GRO_AIRBORNE)
            continue;

        if (i == GORD_SUPPORT or i == GORD_REPAIR or i == GORD_AIRDEFENSE)
            continue;

        if (i == GORD_RADAR and u->GetUnitNormalRole() not_eq GRO_RECON)
            continue;

        if ( not i or u->GetUnitRoleScore(GetGroundRole(i), CALC_MAX, 0) > MIN_ALLOWABLE_ROLE_SCORE)
        {
            // Add to canidate list
            AddToList(u, i);
        }
    }
}

// Gather up all units for this team
int GroundTaskingManagerClass::CollectGroundAssets(int to_collect)
{
    Unit u, pu;
    int count = 0, objListBuilt = 0;

    // Create the unit lists
    VuListIterator myit(AllParentList);
    u = GetFirstUnit(&myit);

    while (u)
    {
        if (u->GetTeam() == owner and u->GetDomain() == DOMAIN_LAND and not u->Scripted())
        {
            // We've got at least one unit to assign - build our objective lists
            if ( not objListBuilt)
            {
                Setup();
                BuildObjectiveLists(to_collect);
                objListBuilt = 1;
            }

            u->RallyUnit(MIN_TASK_GROUND);
            u->UpdateParentStatistics();

            // We want to order support battalions individually.
            if ( not u->Real() and FindUnitSupportRole(u))
            {
                pu = u;
                u = pu->GetFirstUnitElement();

                while (u)
                {
                    AddToLists(u, to_collect);
                    count++;
                    u = pu->GetNextUnitElement();
                }
            }
            else
            {
                AddToLists(u, to_collect);
                count++;
            }
        }

        u = GetNextUnit(&myit);
    }

    return count;
}

int GroundTaskingManagerClass::AssignUnit(Unit u, int orders, Objective o, int score)
{
    Objective so, po;
    POData pod;
    // SOData sod;

    if ( not u or not o)
        return 0;

#ifdef KEV_GDEBUG
    AssignedCount[orders]++;
#endif

    Assigned++;

    // Set local data right now...
    u->SetAssigned(1);
    u->SetOrdered(1);
    u->SetUnitOrders(orders, o->Id());

    // Now collect the SO and PO from this objective, if we don't already have them
    po = so = o;

    if ( not so->IsSecondary() and o->GetObjectiveParent())
        po = so = o->GetObjectiveParent();

    if ( not po->IsPrimary() and so->GetObjectiveParent())
        po = so->GetObjectiveParent();

    u->SetUnitObjective(o->Id());
    u->SetUnitSecondaryObj(so->Id());
    u->SetUnitPrimaryObj(po->Id());

    // Increment unit count for this primary
    pod = GetPOData(po);

    if (pod)
        pod->ground_assigned[owner] += u->GetTotalVehicles();

    /* if (so not_eq po)
     {
     sod = GetSOData(so);
     if (sod)
     sod->assigned[owner] += u->GetTotalVehicles();
     }
     */

#ifdef KEV_GDEBUG
    // char name1[128],name2[128];
    // GridIndex x,y,ux,uy;
    // u->GetName(name1,127);
    // o->GetName(name2,127);
    // u->GetLocation(&ux,&uy);
    // o->GetLocation(&x,&y);
    // MonoPrint("%s (%d) %s -> %s (%d) @ %d,%d - d:%d, s:%d\n",name1,u->GetCampID(),OrderStr[orders],name2,o->GetCampID(),x,y,(int)Distance(ux,uy,x,y),score);
#endif
    return 1;
}

int GroundTaskingManagerClass::AssignUnits(int orders, int mode)
{
    GODNode curo, nexto;
    USNode curu, nextu;
    int ucnt = 0, ocnt = 0, sortBy = GODN_SORT_BY_PRIORITY;

#ifdef KEV_GDEBUG
    ulong time, newtime;
    time = GetTickCount();
#endif

    if ( not objList[orders] or not canidateList[orders])
        return 0;

    // Special case for reserve orders -
    // We're only going to reorder the unit farthest from our primary objective
    if (orders == GORD_RESERVE)
    {
        GridIndex x, y, px = 512, py = 512;
        float ds, bestds = FLT_MAX;
        USNode bestn = NULL;
        Objective po = (Objective) vuDatabase->Find(TeamInfo[owner]->GetGroundAction()->actionObjective);

        if (po)
            po->GetLocation(&px, &py);

        nextu = canidateList[orders];

        while (nextu)
        {
            curu = nextu;
            nextu = curu->next;
            curu->unit->GetLocation(&x, &y);
            ds = (float) DistSqu(x, y, px, py);

            if (ds < bestds)
            {
                bestds = ds;

                if (bestn)
                    canidateList[orders] = canidateList[orders]->Remove(curu);

                bestn = curu;
            }
            else
                canidateList[orders] = canidateList[orders]->Remove(curu);
        }
    }

    // Maintain canidate list
    nextu = canidateList[orders];

    while (nextu)
    {
        curu = nextu;
        nextu = curu->next;

        if (curu->unit->Assigned())
            canidateList[orders] = canidateList[orders]->Remove(curu);
        else
            ucnt++;
    }

    // Set up our lists
    nexto = objList[orders];

    while (nexto)
    {
        curo = nexto;
        nexto = curo->next;

        if (orders == GORD_REPAIR)
        {
            // RV - Biker - Only repair ABs if they are used by some squadron
            if (curo->obj->GetType() == TYPE_AIRBASE or curo->obj->GetType() == TYPE_AIRSTRIP)
            {
                ATMAirbaseClass* atmbase = TeamInfo[curo->obj->GetTeam()]->atm->FindATMAirbase(curo->obj->Id());

                //Biker - That does not work???
                if (atmbase and atmbase->usage < 1)
                {
                    continue;
                }
            }

            GridIndex ox = 0, oy = 0;
            curo->obj->GetLocation(&ox, &oy);
            float dist = DistanceToFront(ox, oy);

            // RV - Biker - Do not repair object near front only if it's a bridge
            if (curo->obj->Type() not_eq TYPE_BRIDGE and (curo->obj->IsFrontline() or curo->obj->IsSecondline() or curo->obj->IsThirdline() or dist < 15.0f))
            {
                continue;
            }
        }

        if (ocnt <= ucnt)
        {
            // Only take one objective per available unit (highest priority first)
            curu = canidateList[orders];

            while (curu)
            {
                curo->InsertUnit(curu->unit, curu->score, ScoreUnitFast(curu, curo, orders, mode));
                curu = curu->next;
            }

            if (curo->unit_options > 0)
                ocnt++;
            else
                objList[orders] = objList[orders]->Remove(curo);
        }
        else
            objList[orders] = objList[orders]->Remove(curo);
    }

    // Assign each objective
    while (objList[orders]) // and canidateList[orders])
    {
        curo = objList[orders] = objList[orders]->Sort(GODN_SORT_BY_OPTIONS);

        if (curo)
            AssignObjective(curo, orders, mode);
    }

    if (canidateList[orders])
        canidateList[orders] = canidateList[orders]->Purge();

#ifdef KEV_GDEBUG
    newtime = GetTickCount();
    Time[orders] = newtime - time;
#endif

    return 1;
}

int checks = 0, runs = 0;

int GroundTaskingManagerClass::AssignObjective(GODNode curo, int orders, int mode)
{
    USNode curu, bestu;
    int bests, score, retval = 0;

#ifdef KEV_GDEBUG
    ulong time;
    time = GetTickCount();
    runs++;
#endif

    curu = curo->unit_list;
    bests = -30000;
    score = 0;
    bestu = NULL;

    while (curu and curu->distance > bests)
    {
#ifdef KEV_GDEBUG
        checks++;
#endif
        score = ScoreUnit(curu, curo, orders, mode);

        if (score > bests)
        {
            bestu = curu;
            bests = score;
        }

        curu = curu->next;
    }

    if (bestu)
    {
        // Assign this unit this this objective
        retval = AssignUnit(bestu->unit, orders, curo->obj, bestu->score);
        // Remove unit from the lists
        objList[orders]->RemoveUnitFromAll(bestu->unit);
        // canidateList[orders] = canidateList[orders]->Remove(bestu);
    }

    objList[orders] = objList[orders]->Remove(curo);
#ifdef KEV_GDEBUG
    PickTime += GetTickCount() - time;
#endif
    return retval;
}

int GroundTaskingManagerClass::ScoreUnit(USNode curu, GODNode curo, int orders, int mode)
{
    int score = -32000;
    GridIndex ux, uy, ox, oy;
    costtype cost;

#ifdef KEV_GDEBUG
    ulong time;
    time = GetTickCount();
#endif

    curu->unit->GetLocation(&ux, &uy);
    cost = CostToArrive(curu->unit, orders, ux, uy, curo->obj);

    if (cost >= OBJ_GROUND_PATH_MAX_COST)
    {
#ifdef KEV_GDEBUG
        ScoreTime += GetTickCount() - time;
#endif
        return score;
    }

    if (mode == GTM_MODE_BEST)
    {
        // Adjust by distance to objective
        curo->obj->GetLocation(&ox, &oy);
        score = curu->score + 100 - FloatToInt32(Distance(ox, oy, ux, uy)) / 5;
        // Adjust by distance from division
        Division div;
        float d;
        curo->obj->GetLocation(&ox, &oy);

        if (div = GetDivisionByUnit(curu->unit))
        {
            div->GetLocation(&ux, &uy);
            d = (float) DistSqu(ux, uy, ox, oy);

            if (d > 900.0F)
                score -= FloatToInt32((d - 900.0F) / 50.0F);
        }
    }
    else if (mode == GTM_MODE_FASTEST)
    {
        // Adjust by cost to objective
        score = FloatToInt32(50 + curu->score / 2.0F - cost);
    }

#ifdef KEV_GDEBUG
    ScoreTime += GetTickCount() - time;
#endif
    return score;
}

int GroundTaskingManagerClass::ScoreUnitFast(USNode curu, GODNode curo, int orders, int mode)
{
    int score = -32000;
    GridIndex ox, oy, ux, uy;

    curo->obj->GetLocation(&ox, &oy);
    curu->unit->GetLocation(&ux, &uy);

    if (mode == GTM_MODE_BEST)
    {
        score = curu->score + 100 - FloatToInt32(Distance(ox, oy, ux, uy)) / 5;
    }
    else if (mode == GTM_MODE_FASTEST)
    {
        score = 50 + curu->score / 2 - FloatToInt32(Distance(ox, oy, ux, uy)) / 2;
    }

    return score;
}

void GroundTaskingManagerClass::FinalizeOrders(void)
{
}

// Sends a message to the GTM
void GroundTaskingManagerClass::SendGTMMessage(VU_ID from, short message, short data1, short data2, VU_ID data3)
{
    VuTargetEntity *target = (VuTargetEntity*) vuDatabase->Find(OwnerId());
    FalconGndTaskingMessage* togtm = new FalconGndTaskingMessage(Id(), target);

    if (this)
    {
        togtm->dataBlock.from = from;
        togtm->dataBlock.team = owner;
        togtm->dataBlock.messageType = message;
        togtm->dataBlock.data1 = data1;
        togtm->dataBlock.data2 = data2;
        togtm->dataBlock.enemy = data3;
        FalconSendMessage(togtm, TRUE);
    }
}

void GroundTaskingManagerClass::RequestEngineer(Objective o, int division)
{
    Unit u;
    o->SetNeedRepair(1);

    VuListIterator myit(AllParentList);
    u = GetFirstUnit(&myit);

    while (u)
    {
        if (u->GetTeam() == owner and u->GetDomain() == DOMAIN_LAND and 
            u->GetUnitNormalRole() == GRO_ENGINEER and u->GetUnitDivision() == division and u->GetUnitOrders() not_eq GORD_REPAIR)
        {
            u->SetUnitOrders(GORD_REPAIR, o->Id());
            return;
        }

        u = GetNextUnit(&myit);
    }

    // Find the best _free_ engineer to send to this location
}

void GroundTaskingManagerClass::RequestAirDefense(Objective o, int division)
{
}

int GroundTaskingManagerClass::Handle(VuFullUpdateEvent *event)
{
    GroundTaskingManagerClass* tmpGTM = (GroundTaskingManagerClass*)(event->expandedData_.get());

    // Copy in new data
    memcpy(&flags, &tmpGTM->flags, sizeof(short));
    return (VuEntity::Handle(event));
}

// ==================
// Global functions
// ==================

int GetTopPriorityObjectives(int team, _TCHAR* buffers[COLLECTABLE_HP_OBJECTIVES])
{
    Objective o;
    _TCHAR tmp[80];
    int i;

    for (i = 0; i < COLLECTABLE_HP_OBJECTIVES; i++)
        buffers[i][0] = 0;

    // JB 010121
    if ( not TeamInfo[team])
        return 0;

    o = (Objective) vuDatabase->Find(TeamInfo[team]->GetDefensiveAirAction()->actionObjective);

    if (o and TeamInfo[team]->GetDefensiveAirAction()->actionType == AACTION_DCA)
        o->GetFullName(tmp, 79, FALSE);
    else
        ReadIndexedString(300, tmp, 10);

    AddStringToBuffer(tmp, buffers[0]);

    o = (Objective) vuDatabase->Find(TeamInfo[team]->GetOffensiveAirAction()->actionObjective);

    if (o and TeamInfo[team]->GetOffensiveAirAction()->actionType)
        o->GetFullName(tmp, 79, FALSE);
    else
        ReadIndexedString(300, tmp, 10);

    AddStringToBuffer(tmp, buffers[1]);

    o = (Objective) vuDatabase->Find(TeamInfo[team]->GetGroundAction()->actionObjective);

    if (o and TeamInfo[team]->GetGroundAction()->actionType < GACTION_MINOROFFENSIVE)
        o->GetFullName(tmp, 79, FALSE);
    else
        ReadIndexedString(300, tmp, 10);

    AddStringToBuffer(tmp, buffers[2]);

    o = (Objective) vuDatabase->Find(TeamInfo[team]->GetGroundAction()->actionObjective);

    if (o and TeamInfo[team]->GetGroundAction()->actionType >= GACTION_MINOROFFENSIVE)
        o->GetFullName(tmp, 79, FALSE);
    else
        ReadIndexedString(300, tmp, 10);

    AddStringToBuffer(tmp, buffers[3]);

    return 4;
}

// This should encode the current priority of all primary objectives
short EncodePrimaryObjectiveList(uchar teammask, uchar **buffer)
{
    uchar *data, *datahead;
    ListNode lp;
    POData pod;
    short size, count = 0, team, teams = 0;

    // 'teammask' determines which team data we're sending. If no such
    // team exists, there's no reason to send the data. Check for this
    for (team = 0; team < NUM_TEAMS; team++)
    {
        if ( not TeamInfo[team])
            teammask and_eq compl (1 << team);
        else if (teammask bitand (1 << team))
            teams++;
    }

    // Determine size
    size = sizeof(uchar) + sizeof(short);
    lp = PODataList->GetFirstElement();

    while (lp)
    {
        count++;
        lp = lp->GetNext();
    }

    size += count * (sizeof(VU_ID) + sizeof(short) * teams);

    // Write the data
    data = datahead = new uchar[size];
    memcpy(data, &teammask, sizeof(uchar));
    data += sizeof(uchar);
    memcpy(data, &count, sizeof(short));
    data += sizeof(short);
    lp = PODataList->GetFirstElement();

    while (lp)
    {
        pod = (POData) lp->GetUserData();
        memcpy(data, &pod->objective, sizeof(VU_ID));
        data += sizeof(VU_ID);

        for (team = 0; team < NUM_TEAMS; team++)
        {
            if (teammask bitand (1 << team))
            {
                memcpy(data, &pod->player_priority[team], sizeof(short));
                data += sizeof(short);
            }
        }

        lp = lp->GetNext();
    }

    *buffer = datahead;

    return size;
}

void DecodePrimaryObjectiveList(uchar *datahead, FalconEntity *fe)
{
    short count, priority;
    uchar team, teammask, *data = datahead;
    POData pod = NULL;
    Objective po;
    VU_ID id;

    ShiAssert(PODataList and PODataList->GetFirstElement())

    memcpy(&teammask, data, sizeof(uchar));
    data += sizeof(uchar);
    memcpy(&count, data, sizeof(short));
    data += sizeof(short);

    while (count)
    {
        memcpy(&id, data, sizeof(VU_ID));
        data += sizeof(VU_ID);
        po = (Objective) vuDatabase->Find(id);

        if (po)
            pod = GetPOData(po);
        else
        {
            // KCK: Remote machines should have built these thingys.
            ShiAssert(0);
        }

        for (team = 0; team < NUM_TEAMS; team++)
        {
            if (teammask bitand (1 << team))
            {
                memcpy(&priority, data, sizeof(short));
                data += sizeof(short);

                if (pod)
                    pod->player_priority[team] = priority;
            }
        }

        count--;
    }

    TheCampaign.Flags and_eq compl CAMP_NEED_PRIORITIES;
}

// This should send the current priority of all primary objectives
void SendPrimaryObjectiveList(uchar teammask)
{
    FalconCampDataMessage *msg;
    int team;

    // If we're not specifying a team, send them all
    if ( not teammask)
    {
        for (team = 0; team < NUM_TEAMS; team++)
        {
            if (TeamInfo[team])
                teammask or_eq (1 << team);
        }
    }

#ifdef DEBUG

    for (team = 0; team < NUM_TEAMS; team++)
    {
        if (teammask bitand (1 << team))
            ShiAssert(TeamInfo[team]);
    }

#endif

    msg = new FalconCampDataMessage(FalconNullId, FalconLocalGame, FALSE);

    msg->dataBlock.type = FalconCampDataMessage::campPriorityData;
    msg->dataBlock.size = EncodePrimaryObjectiveList(teammask, &msg->dataBlock.data);

    FalconSendMessage(msg, TRUE);
}

// This should save the current priority of all primary objectives to the passes file pointer
void SavePrimaryObjectiveList(char* scenario)
{
    short size, team;
    uchar *data, teammask = 0;
    FILE *fp;

    if ((fp = OpenCampFile(scenario, "pol", "wb")) == NULL)
        return;

    for (team = 0; team < NUM_TEAMS; team++)
    {
        if (TeamInfo[team])
            teammask or_eq (1 << team);
    }

    size = EncodePrimaryObjectiveList(teammask, &data);
    fwrite(data, size, 1, fp);
    CloseCampFile(fp);
    delete data;
}

int LoadPrimaryObjectiveList(char* scenario)
{
    // uchar *data;

    CampaignData  cd = ReadCampFile(scenario, "pol");

    if (cd.dataSize == -1)
    {
        return 0;
    }

    DecodePrimaryObjectiveList((uchar*)cd.data, NULL);
    delete cd.data;
    return 1;
}

