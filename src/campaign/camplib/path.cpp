#include <stddef.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <conio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "campmap.h"
#include "CmpGlobl.h"
#include "CampCell.h"
#include "CampTerr.h"
#include "Listadt.h"
#include "Objectiv.h"
#include "ASearch.h"
#include "Unit.h"
#include "Find.h"
#include "Campaign.h"
#include "path.h"
#include "team.h"
#include "classtbl.h"
#include "Debuggr.h"
#include "uiwin.h"
#include "GndUnit.h"
#include "AIInput.h"
#include "GTM.h"
#include "MsgInc/GndTaskingMsg.h"

void debugprintf(LPSTR dbgFormat, ...)
{
    char  dbgBuffer[256];
    va_list ap;

    va_start(ap, dbgFormat);

    wvsprintf(dbgBuffer, dbgFormat, ap);
    OutputDebugString(dbgBuffer);
#ifdef DEBUGSTDOUTALSO
    printf(dbgBuffer);
#endif
    va_end(ap);
}

//#define MonoPrint  debugprintf

// =====================================
// Path.c Globals and defines
// =====================================

#define EUNIT_WEIGHT    2
#define FUNIT_WEIGHT    5
#define OBJ_WEIGHT      2
#define THREAT_STEP 8

#define PATH_DIAGONAL 0x8000 // set if a diagonal move

// Costs are: 99=No move, 1-x= Easy to hard
float CostTable[COVER_TYPES][MOVEMENT_TYPES] =
{
    { 99.0F, 99.0F, 99.0F, 99.0F,  1.0F,  1.0F,   1.0F,  99.0F }, // Water
    { 99.0F,  3.0F,  8.0F,  6.0F,  1.0F,  1.0F,  99.0F,  99.0F }, // Bog/Swamp
    { 99.0F,  1.5F,  4.0F,  3.0F,  1.0F,  1.0F,  99.0F,  99.0F }, // Barren/Desert
    { 99.0F,  1.0F,  3.0F,  2.0F,  1.0F,  1.0F,  99.0F,  99.0F }, // Plains/Farmland
    { 99.0F,  1.0F,  3.0F,  2.0F,  1.0F,  1.0F,  99.0F,  99.0F }, // Grass/Brush
    { 99.0F,  2.0F,  6.0F,  4.0F,  1.0F,  1.0F,  99.0F,  99.0F }, // LightForest
    { 99.0F,  2.0F,  6.0F,  4.0F,  1.0F,  1.0F,  99.0F,  99.0F }, // HvyForest/Jungle
    { 99.0F,  2.0F,  3.0F,  3.0F,  1.0F,  1.0F,  99.0F,  99.0F }
};// Urban

float ReliefCost[RELIEF_TYPES] = { 1.0F, 1.41F, 1.73F, 2.0F };
float CoverValues[COVER_TYPES] = { 0.0F, 2.0F, 1.0F, 1.5F, 2.0F, 3.0F, 4.0F, 4.0F };

int QuickSearch;
int moveTeam, moveType, moveFlags, moveAlt, maxSearch = MAX_SEARCH;
costtype maxCost = 0;

extern GridIndex dxo[], dyo[];

// =========================
// Local Function Prototypes
// =========================

void GetNeighborCoord(AS_DataClass* asd, void* o, void* t);

void GetNeighborObject(AS_DataClass* asd, void* o, void* t);

// ==========================
// Public functions
// ==========================

#ifdef CAMPTOOL

extern signed short ShowPath(Path path, GridIndex X, GridIndex Y);
extern void ShowWhere(MapData md, GridIndex x, GridIndex y, int color);
extern void ShowWP(MapData md, GridIndex X, GridIndex Y, int color);
extern void ShowLink(MapData md, Objective o, Objective n, int color);
extern unsigned char ShowSearch;
int DColor;

#endif CAMPTOOL


int GetGridPath(Path p, GridIndex x, GridIndex y, GridIndex xx, GridIndex yy, int type, int who, int flags)
{
    void*       o;
    void* t;
    int retval;

    moveTeam = who;
    moveType = type;
    moveFlags = flags;

#ifdef CAMPTOOL

    if (ShowSearch)
        DColor = (DColor + 1) % 16;

    //DColor = Blue;
#endif

    // Should be able to drop this. Assume not true in future.
    if (x < 0 or y < 0 or xx < 0 or yy < 0 or x >= Map_Max_X or y >= Map_Max_Y or xx >= Map_Max_X or yy >= Map_Max_Y)
    {
        p->ClearPath();
        return -1;
    }

    if (GetMovementCost(xx, yy, (MoveType)moveType, moveFlags, Here) > MAX_COST)
    {
        p->ClearPath();
        return -1;
    }

    o = PackXY(x, y);
    t = PackXY(xx, yy);
    // Debug stuff
    //ulong time,newtime;
    //time = GetTickCount();
    retval = ASD->ASSearch(p, o, t, GetNeighborCoord, RETURN_PARTIAL_ON_FAIL bitor RETURN_PARTIAL_ON_MAX, maxSearch, maxCost);
    //newtime = GetTickCount();
    //MonoPrint("Finding Grid Path: %d,%d -> %d,%d   Time to find: %d (Result: %d)\n",x,y,xx,yy,newtime-time,retval);
#ifdef CAMPTOOL

    if (ShowSearch and MOVE_AIR(moveType))
        Sleep(5000);

#endif
    return retval;
}

costtype GetPathCost(GridIndex x, GridIndex y, Path path, MoveType mt, int flags)
{
    costtype cost = 0.0F;
    int d, i = 0;

    d = path->GetDirection(i);

    while (d not_eq Here)
    {
        x += dx[d];
        y += dy[d];
        cost += GetMovementCost(x, y, mt, flags, (CampaignHeading)d);
        i++;
        d = path->GetDirection(i);
    }

    return cost;
}

costtype GetPathCost(Objective o, Path path, MoveType mt, int flags)
{
    costtype    cost = 0.0F;
    int d, i = 0;

    d = path->GetDirection(i);

    while (d not_eq Here)
    {
        cost += o->GetNeighborCost(d, mt);
        o = o->GetNeighbor(d);
        i++;
        d = path->GetDirection(i);
    }

    return cost;
    flags;
}

int GetObjectivePath(Path p, Objective o, Objective t, int type, int who, int flags)
{
    int retval;
    moveTeam = who;
    moveType = type;
    moveFlags = flags;

    if (type == NoMove and o not_eq t)
        return -1;

#ifdef CAMPTOOL

    if (ShowSearch)
        // DColor = (DColor+1)%16;
        DColor = Blue;

#endif

    // Debug stuff
    //ulong time,newtime;
    //GridIndex x,y,xx,yy;
    //time = GetTickCount();
    retval = ASD->ASSearch(p, o, t, GetNeighborObject, RETURN_PARTIAL_ON_FAIL bitor RETURN_PARTIAL_ON_MAX, maxSearch, maxCost);
    //newtime = GetTickCount();
    //o->GetLocation(&x,&y);
    //t->GetLocation(&xx,&yy);
    //MonoPrint("Finding Obj Path: %d,%d -> %d,%d   Time: %d (Result: %d, cost: %f)\n",x,y,xx,yy,newtime-time,retval,p->GetCost());
    return retval;
}

int GetObjectivePath(Path p, GridIndex x, GridIndex y, GridIndex xx, GridIndex yy, int type, int who, int flags)
{
    Objective   o, t;

    o = FindNearestObjective(x, y, NULL);
    t = FindNearestObjective(xx, yy, NULL);
    return GetObjectivePath(p, o, t, type, who, flags);
}

// This should only be called for linking objectives
int FindLinkPath(Path p, Objective O1, Objective O2, MoveType mt)
{
    void*       o;
    void* t;
    GridIndex   ox, oy, tx, ty;

    O1->GetLocation(&ox, &oy);
    O2->GetLocation(&tx, &ty);
    o = PackXY(ox, oy);
    t = PackXY(tx, ty);

    if (mt == NoMove)
    {
        moveType = Wheeled;
        moveFlags = PATH_ROADOK bitor PATH_BASIC;
    }
    else
    {
        moveType = mt;
        moveFlags = PATH_BASIC;
    }

    moveTeam = 0;

    if (GetMovementCost(ox, oy, (MoveType)moveType, moveFlags, Here) > MAX_COST or
        GetMovementCost(tx, ty, (MoveType)moveType, moveFlags, Here) > MAX_COST)
    {
        p->ClearPath();
        return 0;
    }

    return ASD->ASSearch(p, o, t, GetNeighborCoord, 0, maxSearch, maxCost);
}

// Used only by planning algorythm
costtype CostToArrive(Unit u, int orders, GridIndex x, GridIndex y, Objective t)
{
    Objective o;
    PathClass path;
    int flags = 0;

    // Movement options
    if (GetGroundRole(orders) == GRO_ATTACK)
        flags or_eq PATH_ENEMYOK; // bitor PATH_ENEMYCOST;

    if (u->GetSType() == STYPE_UNIT_AIRMOBILE)
        flags or_eq PATH_AIRBORNE;

    if (u->GetSType() == STYPE_UNIT_MARINE)
        flags or_eq PATH_MARINE;

    o = FindNearestObjective(x, y, NULL);

    if ( not o or not t or o == t)
        return 0;

    moveTeam = u->GetTeam();
    moveType = u->GetMovementType();
    moveFlags = flags;

    if (moveType == NoMove)
        return -1;

#ifdef CAMPTOOL

    if (ShowSearch)
        // DColor = (DColor+1)%16;
        DColor = Blue;

#endif

    if (ASD->ASSearch(&path, o, t, GetNeighborObject, RETURN_EMPTY_ON_FAIL, OBJ_GROUND_PATH_MAX_SEARCH, (costtype) OBJ_GROUND_PATH_MAX_COST) < 1)
        return OBJ_GROUND_PATH_MAX_COST;

    return path.GetCost() * 10.0f / u->GetMaxSpeed();
}

// Movement cost is the 'virtual cost'- How many times longer it'll take us to go one
// map sector than if we went there directly via road. This should take into account
// lower speeds through terrain, and path curvyness.
float GetMovementCost(GridIndex x, GridIndex y, MoveType move, int flags, CampaignHeading h)
{
    float          cost;
    Objective o;

    cost = CostTable[GetCover(x, y)][move];

    switch (move)
    {
        case Foot:
        case Wheeled:
        case Tracked:
            if (GetRoad(x, y) and not (h bitand 0x01))
            {
                // It's a bridge or port, check if intact
                if (cost > MAX_COST)
                {
                    o = FindNearestObjective(x, y, NULL);

                    // RV - Biker - Loop through ground units to find engineer battalion assigned for repair
                    VuListIterator uit(AllUnitList);
                    Unit u = GetFirstUnit(&uit);
                    GridIndex ux = 0, uy = 0;
                    bool assignedEng = false;

                    while (u)
                    {
                        if (u->IsBrigade() or u->GetDomain() not_eq DOMAIN_LAND or u->GetTeam() not_eq o->GetTeam())
                        {
                            u = GetNextUnit(&uit);
                            continue;
                        }

                        u->GetLocation(&ux, &uy);
                        float dx = float(ux - x);
                        float dy = float(uy - y);

                        float dist = sqrt(dx * dx + dy * dy);

                        // RV - Biker - Check for engineer type maybe we need some more check
                        if (u->GetSType() == STYPE_UNIT_ENGINEER or u->GetSType() == STYPE_WHEELED_ENGINEER)
                        {
                            if (dist <= 1.0f and u->GetTeam() == o->GetTeam())
                            {
                                assignedEng = true;
                                break;
                            }
                        }

                        u = GetNextUnit(&uit);
                    }

                    if (o and (o->GetType() == TYPE_PORT or (o->GetType() == TYPE_BRIDGE and (o->GetObjectiveStatus() > 0 or flags bitand PATH_ENGINEER))))
                        cost = 0.5F;

                    if (o and o->GetType() == TYPE_BRIDGE and o->GetObjectiveStatus() < 30 and assignedEng)
                        cost = 5.0F;
                }
                else if (flags bitand PATH_ROADOK and GetRoad((GridIndex)(x - dx[h]), (GridIndex)(y - dy[h])))
                    cost = 0.5F; // Use roads when we're allowed to
                else
                    cost *= 0.5F; // Otherwise, lesser bonus
            }

            cost *= ReliefCost[GetRelief(x, y)];
            break;

        case LowAir:
            cost *= ReliefCost[GetRelief(x, y)]; // This only makes since for helecopters
            break;

        case Rail:
            if (GetRail(x, y))
            {
                if (cost > MAX_COST) // It's a bridge, check if intact
                {
                    o = FindNearestObjective(x, y, NULL);

                    if (o->GetType() == TYPE_PORT or (o->GetType() == TYPE_BRIDGE and (o->GetObjectiveStatus() > 0 or flags bitand PATH_ENGINEER)))
                        cost = 0.5F;
                }
                else if (flags bitand PATH_RAILOK and GetRail((GridIndex)(x - dx[h]), (GridIndex)(y - dy[h]))) // Use rails when we're allowed to
                    cost = 0.5F;
            }

            break;

        case Naval:
            if (cost > MAX_COST)
            {
                o = FindNearestObjective(x, y, NULL);

                if (o and (o->GetType() == TYPE_PORT or o->GetType() == TYPE_BEACH) and o->GetObjectiveStatus() > 0)
                    cost = 1.0F;
            }

            break;

        case Air:
        default:
            break;
    }

    if (h bitand 0x01)
        cost *= 1.41F;

    return cost;
}

/*
void ResetThreats (void)
 {
 Objective o;

 o = (Objective) CdbGetFirstInPointerList(ThreatList);
 while (o)
 {
 o->UnsetChecked();
 o = (Objective) CdbGetNextInPointerList(ThreatList);
 }
 }

/*
// Checks path between two waypoints at a given altitude
int CountLegThreats(WayPoint w1, WayPoint w2, Team who, AltitudeType alt)
   {
   int         i,step,found=0;
   GridIndex   x,y,fx,fy;
   float       d;
 double theta;

   w1->GetWPLocation(&fx,&fy);
   w2->GetWPLocation(&x,&y);
   d = Distance(x,y,fx,fy);
 step = THREAT_STEP;
 theta = asin((y-fy)/d);
 for (i=0; i<d; i+=step)
 {
 x = fx + (GridIndex)(cos(theta)*i);
 y = fy + (GridIndex)(sin(theta)*i);
 found += CountThreat(x,y,alt,who,0);
 }
   return found;
   }

// This counts the number of threats to a given type along a particular waypoint path
int CheckPathThreats (WayPoint w, Team who, AltitudeType alt)
 {
 WayPoint nw;
 int threats=0;

 ResetThreats();
 if ( not w)
 return 0;
 nw = w->GetNextWP();
 while (nw)
 {
 threats += CountLegThreats(w,nw,who,alt);
 w = nw;
 nw = w->GetNextWP();
 }
 return threats;
 }
*/

// ==========================
// Private Functions
// ==========================

char  AvoidTable[] = {20, 5, 1};    // Minimum distance from engaged units

void GetNeighborCoord(AS_DataClass* asd, void* o, void* t)
{
    int         d, step = 1;
    GridIndex   ox, oy, tx, ty, x, y;
    costtype    cost, left;
    void*       n;
    float leftmod = 1.0f;

    UnpackXY(o, &ox, &oy);
    UnpackXY(t, &tx, &ty);

    if (QuickSearch)
        step = QuickSearch;

    if (MOVE_AIR(moveType))
        leftmod = 4.0f;
    else if (MOVE_GROUND(moveType) and moveFlags bitand PATH_ROADOK)
        leftmod = 0.75f;
    else
        leftmod = 3.0f;

#ifdef CAMPTOOL

    if (ShowSearch and MOVE_AIR(moveType))
        ShowWP(NULL, ox, oy, DColor);

#endif

    for (d = 0; d < Here; d++)
    {
        x = (GridIndex)(ox + step * dx[d]);
        y = (GridIndex)(oy + step * dy[d]);

        if (x < 0 or x >= Map_Max_X or y < 0 or y >= Map_Max_Y)
        {
            asd->ASFillNode(d, &cost, &cost, -1, NULL);
            continue;
        }

        cost = GetMovementCost(x, y, (MoveType)moveType, moveFlags, (CampaignHeading)d);

        if (MOVE_AIR(moveType) and not (moveFlags bitand PATH_BASIC))
        {
            // Add cost for air threats
            // Essentially, tcost is 0-100. This translates into # of km out of our
            // way we're willing to go to avoid this threat.
            costtype hcost = 0.0F, tcost;
#ifdef DEBUG
            static float max_dif = -100.0F, min_dif = 100.0F;
#endif

            tcost = (float)ScoreThreatFast(x, y, moveAlt, (Team)moveTeam);

            if (tcost > hcost)
                hcost = tcost;

            tcost = (float)ScoreThreatFast((GridIndex)(x - MAP_RATIO), y, moveAlt, (Team)moveTeam);

            if (tcost > hcost)
                hcost = tcost;

            tcost = (float)ScoreThreatFast(x, (GridIndex)(y - MAP_RATIO), moveAlt, (Team)moveTeam);

            if (tcost > hcost)
                hcost = tcost;

            tcost = (float)ScoreThreatFast((GridIndex)(x + MAP_RATIO), y, moveAlt, (Team)moveTeam);

            if (tcost > hcost)
                hcost = tcost;

            tcost = (float)ScoreThreatFast(x, (GridIndex)(y + MAP_RATIO), moveAlt, (Team)moveTeam);

            if (tcost > hcost)
                hcost = tcost;

#ifdef DEBUG
            tcost = (float)ScoreThreatFast(x, y, moveAlt, moveTeam)
                    + (float)ScoreThreatFast((GridIndex)(x - MAP_RATIO), y, moveAlt, (Team)moveTeam)
                    + (float)ScoreThreatFast(x, (GridIndex)(y - MAP_RATIO), moveAlt, (Team)moveTeam)
                    + (float)ScoreThreatFast((GridIndex)(x + MAP_RATIO), y, moveAlt, (Team)moveTeam)
                    + (float)ScoreThreatFast(x, (GridIndex)(y + MAP_RATIO), moveAlt, (Team)moveTeam);
            tcost /= 5.0F;

            if (hcost - tcost > max_dif)
                max_dif = hcost - tcost;

            if (hcost - tcost < min_dif)
                min_dif = hcost - tcost;

#endif

            if (hcost > 120.0F)
            {
                // Illegal Move
                asd->ASFillNode(d, &cost, &cost, -1, NULL);
                continue;
            }

            cost = cost * step + hcost / 2.0F;
        }
        else
        {
            // Just check for illegal moves
            ShiAssert(step == 1);

            if (cost > MAX_COST)
            {
                asd->ASFillNode(d, &cost, &cost, -1, NULL);
                continue;
            }
        }

        left = Distance(x, y, tx, ty);

        if (left < QuickSearch)
        {
            // This is close enough
            x = tx;
            y = ty;
        }

        n = PackXY(x, y);
        left *= leftmod;
        asd->ASFillNode(d, &cost, &left, (char)d, n);
    }

    for (; d < MAX_NEIGHBORS; d++)
    {
        asd->ASFillNode(d, &cost, &cost, -1, NULL);
        continue;
    }
}

costtype GetObjectiveMovementCost(Objective o, Objective t, int neighbor, MoveType type, Team team, int flags)
{
    Objective n, p;
    costtype cost, opt, mult = 1.0F;
    Team owner;

    n = o->GetNeighbor(neighbor);

    if (n)
    {
        owner = (Team)n->GetTeam();

        // Check for allowable movement
        if ( not GetRoE(team, owner, ROE_GROUND_MOVE))
            return 255.0F;

        // Check for enemy movement
        if ( not (flags bitand PATH_BASIC) and GetRoE(team, owner, ROE_GROUND_FIRE))
        {
            if (flags bitand PATH_ENEMYOK)
            {
                // Double cost if ENEMYCOST is set
                mult = 2.0F;

                // Not allowed under certain circumstances:
                if (n == t)
                    ; // This is ok.
                else if (n->IsSecondary())
                    // return 255.0F;
                    mult = 4.0F;
                else if ( not t)
                    return 255.0F;
                else if (n->GetObjectiveParentID() not_eq t->Id())
                {
                    p = n->GetObjectiveParent();

                    if (p and p->GetTeam() not_eq team)
                        // return 255.0F;
                        mult = 4.0F;
                }
            }
            else
                return 255.0F;
        }

        // Check if road movement is allowed
        if (flags bitand PATH_ROADOK)
            type = NoMove; // KCK: I'm using the no-move slot for road movement costs

        if (flags bitand PATH_RAILOK)
            type = Rail;

        cost = o->GetNeighborCost(neighbor, type);

        if (flags bitand PATH_ENEMYCOST and cost < 255.0F)
        {
            cost *= mult;

            if (cost > 254.0F)
                cost = 254.0F;
        }

        // RV - Biker - Search for engineers then build pontoon bridge
        if ((MOVE_GROUND(type) or type == NoMove) and n->GetType() == TYPE_BRIDGE and not n->GetObjectiveStatus() and not (flags bitand PATH_ENGINEER))
        {
            // Bridge is broke, can't go here.
            // But let's send engineers, if we havn't already
            //GridIndex ox,oy;
            //o->GetLocation(&ox,&oy);

            // RV - Biker - Loop through ground units to find engineer battalion assigned for repair
            VuListIterator uit(AllUnitList);
            Unit u = GetFirstUnit(&uit);
            GridIndex ux = 0, uy = 0;
            GridIndex nx = 0, ny = 0;
            bool assignedEng = false;

            n->GetLocation(&nx, &ny);

            while (u)
            {
                if (u->IsBrigade() or u->GetDomain() not_eq DOMAIN_LAND or u->GetTeam() not_eq o->GetTeam())
                {
                    u = GetNextUnit(&uit);
                    continue;
                }

                u->GetLocation(&ux, &uy);
                float dx = float(ux - nx);
                float dy = float(uy - ny);

                float dist = sqrt(dx * dx + dy * dy);

                // RV - Biker - Check for engineer type maybe we need some more check
                if (u->GetSType() == STYPE_UNIT_ENGINEER or u->GetSType() == STYPE_WHEELED_ENGINEER)
                {
                    if (dist <= 1.0f and u->GetTeam() == n->GetTeam())
                    {
                        assignedEng = true;
                        break;
                    }
                }

                u = GetNextUnit(&uit);
            }

            // RV - Biker - We already have engineers here no need to send message
            if (assignedEng)
            {
                return 5.0f;
            }
            else
            {
                //TeamInfo[moveTeam]->gtm->SendGTMMessage(o->Id(),FalconGndTaskingMessage::gtmEngineerRequest,ox,oy,o->Id());
                TeamInfo[moveTeam]->gtm->SendGTMMessage(n->Id(), FalconGndTaskingMessage::gtmEngineerRequest, nx, ny, n->Id());
                return 255.0F;
            }
        }
        else
        {
            // Check for ground movement options
            if (flags bitand PATH_ROADOK)
            {
                opt = o->GetNeighborCost(neighbor, NoMove) * mult;

                if (opt < cost)
                    cost = opt;
            }

            if (flags bitand PATH_RAILOK)
            {
                opt = o->GetNeighborCost(neighbor, Rail) * mult;

                if (opt < cost)
                    cost = opt;
            }
        }

        if (flags bitand PATH_AIRBORNE)
        {
            opt = o->GetNeighborCost(neighbor, LowAir) * mult;

            if (opt < cost)
                cost = opt;
        }

        if (flags bitand PATH_MARINE)
        {
            opt = o->GetNeighborCost(neighbor, Naval) * mult;

            if (opt < cost)
                cost = opt;
        }

        return cost;
    }

    return (costtype)(255);
}

void GetNeighborObject(AS_DataClass* asd, void* ov, void* tv)
{
    costtype    cost, left;
    int         c = 0;
    Objective   n, o, t;
    GridIndex   nx, ny, tx, ty;

    o = (Objective)ov;
    t = (Objective)tv;
    t->GetLocation(&tx, &ty);

    while (c < o->static_data.links)
    {
        cost = GetObjectiveMovementCost(o, t, c, (MoveType)moveType, (Team)moveTeam, moveFlags);

        if (cost < 255)
        {
            n = o->GetNeighbor(c);
            n->GetLocation(&nx, &ny);
#ifdef CAMPTOOL

            if (ShowSearch)
                ShowLink(NULL, o, n, DColor);

#endif
            left = Distance(nx, ny, tx, ty);
            asd->ASFillNode(c, &cost, &left, (char)c, n);
        }
        else
        {
            asd->ASFillNode(c, &cost, &cost, -1, NULL);
        }

        c++;
    }

    for (; c < MAX_NEIGHBORS; c++)
    {
        asd->ASFillNode(c, &cost, &cost, -1, NULL);
        continue;
    }
}


