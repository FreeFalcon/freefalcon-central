#include <algorithm>

#include <stddef.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <conio.h>
#include <math.h>
#include <string.h>
#include <time.h>

#include "campaign.h"
#include "CampLib.h"
#include "ListADT.h"
#include "F4Thread.h"
#include "CmpClass.h"
#include "ErrorLog.h"
#include "EventLog.h"
#include "CampCell.h"
#include "CampTerr.h"
#include "ASearch.h"
#include "Objectiv.h"
#include "Unit.h"
#include "airunit.h"
#include "gndunit.h"
#include "navunit.h"
#include "Path.h"
#include "GTM.h"
#include "Update.h"
#include "Find.h"
#include "Debuggr.h"
#include "f4thread.h"
#include "f4find.h"
#include "f4vu.h"
#include "falcmesg.h"
#include "ATM.h"
#include "CampList.h"
#include "cmpevent.h"
#include "team.h"
#include "feature.h"
#include "loadout.h"
#include "name.h"
#include "gtmobj.h"
#include "Weather.h"
#include "Supply.h"
#include "AIInput.h"
#include "process.h"
#include "CUIEvent.h"
#include "ThreadMgr.h"
#include "Division.h"
#include "otwdrive.h"
#include "SimDrive.h"
#include "Persist.h"
#include "FalcSess.h"
#include "classtbl.h"
#include "sfx.h"
#include "ui95/CHandler.h"
#include "uicomms.h"
#include "iaction.h"
#include "MsgInc/SimCampMsg.h"
#include "MsgInc/DivertMsg.h"
#include "PlayerOp.h"
#include "atcbrain.h"
#include "Falcuser.h"
#include "Dispcfg.h"
#include "ehandler.h"
#include "ptdata.h"
#include "GameMgr.h"//me123
#include "TimerThread.h"
#ifdef CAMPTOOL
#include "resource.h"
#endif
#include "aircrft.h"
#include "MissEval.h"
//#define KEV_DEBUG 1
//#define _TIMEDEBUG

extern bool g_bFloatingBullseye; // JB/Codec 010115
extern bool g_bServer;
extern bool g_bServerHostAll;
extern int g_nDeagTimer;

// JB 0203111 Restrict takeoff/ramp options.
extern bool g_bMPStartRestricted;
// JB 0203111 MP takeoff time if g_bMPStartRestricted enabled.
extern int g_nMPStartTime;
// bullseye fix
extern int g_nChooseBullseyeFix;
// Booster 2004/10/12 Taxi takeoff time option
extern int g_nTaxiLaunchTime;


// =====================================
// Campaign - Globals and Defines
// =====================================
Float32 f;

GridIndex   dx[17] = {0, 1, 1, 1, 0, -1, -1, -1, 0, 0, 2, 2, 2, 0, -2, -2, -2}; // dx per direction
GridIndex   dy[17] = {1, 1, 0, -1, -1, -1, 0, 1, 0, 2, 2, 0, -2, -2, -2, 0, 2}; // dy per direction

int VisualDetectionRange[OtherDam] = { 12, 4, 5, 5, 8, 16, 10, 5 };
//2001-03-24 MODIFIED BY S.G. SO LOW AIR AND AIR MOVEMENT TYPE ARE DOWN TO 1 MINUTE FROM 2 MINUTES
//CampaignTime ReconLossTime[MOVEMENT_TYPES] =
//{ CampaignHours, 20*CampaignMinutes, 10*CampaignMinutes, 10*CampaignMinutes,
//2*CampaignMinutes, 2*CampaignMinutes, 10*CampaignMinutes, 10*CampaignMinutes };
CampaignTime ReconLossTime[MOVEMENT_TYPES] =
{
    CampaignHours, 20 * CampaignMinutes, 10 * CampaignMinutes,
    10 * CampaignMinutes, 1 * CampaignMinutes, 1 * CampaignMinutes, 10 * CampaignMinutes, 10 * CampaignMinutes
};
uchar DefaultDamageMods[OtherDam + 1] = { 0, 100, 100, 0, 0, 100, 100, 0, 0, 0, 0 };

// distance squared, in feet of fatherest visual effect
#define MAX_DISTANT_EFFECT_DIST_SQ 45000000000
// The max distance (ft) at which we'll deaggregate persistant objects
#define PERSIST_BUBBLE_MAX 10*GRID_SIZE_FT

#define CAMP_NORMAL_MODE 0
#define CAMP_STARTING_UP 1

#define CAMPAIGN_STAGE_TIME_MINUTES 5

#define STAGE_1 1
#define STAGE_2 2
#define STAGE_3 3
#define STAGE_4 4
#define STAGE_5 5
#define STAGE_6 6
#define STAGE_7 7
#define STAGE_8 8
#define STAGE_9 9
#define STAGE_10 10
#define STAGE_11 11
#define STAGE_12 12

// Campaign Locals
static int sCampaignSleepRequested = 0;
static int sCampaignStartingUp = CAMP_NORMAL_MODE;
static int sTakeoffTime = 0;
static int FirstflightsTakeoffTime = 0;


// Campaign's critical section
F4CSECTIONHANDLE* campCritical;

volatile int gLeftToDeaggregate = 0;


// =========================
// Externals
// =========================

extern int doUI;

extern C_Handler *gMainHandler;

// Special time setting stuff
extern void SetTemporaryCompression(int newComp);
extern int targetCompressionRatio;
extern void set_spinner2(int);

// Cheat Stuff;
extern void CheckForCheatFlight(ulong time);

#ifdef CAMPTOOL
extern void ShowTime(CampaignTime t);
extern void RedrawPlayerBubble(void);
extern boolean PBubble;
extern HWND hMainWnd;
#endif

// =========================
// Local Function Prototypes
// =========================

void ResumeTacticalEngagement(void);
void DoTacticalLoop(int startup);
void DoCampaignLoop(int startup);
void CheckForVictory(void);
void UpdateParentUnits(CampaignTime deltatime);
void UpdateRealUnits(CampaignTime deltatime);
void CheckNewDay(void);
void PlanGroundAndNavalUnits(int *planCount);
void OrderGroundAndNavalUnits(void);
void RallyUnits(int minutes);

// ============================
// External function Prototypes
// ============================

extern int LoadTheater(void);
extern void SendRequestedData(void);
extern void CleanupCampaignUI();
extern void CleanupTacticalEngagementUI();
extern int check_victory_conditions(void);
extern void RebuildATMLists(void);
extern void ReadClassTable(void);
extern void WriteClassTable(void);
extern void TriggerTacEndGame(void);
extern void UI_HandleFlightCancel(void);
extern void UI_HandleAircraftDestroyed(void);
extern void UI_HandleAirbaseDestroyed(void);
extern void UI_HandleFlightScrub(void);
extern bool g_bSleepAll;
extern bool g_brebuildbobbleFix;
// ================================
// Support Functions
// ================================

//
// Theater routines
//

int LoadTheater(char *theater)
{
    // This assumes the Class Table was loaded elsewhere
    CampEnterCriticalSection();

    if ( not LoadTheaterTerrain(theater))
    {
        MonoPrint("Failed to open theater: %s, using default theater.\n", theater);
        Map_Max_X = Map_Max_Y = 500;
        InitTheaterTerrain();
        CampLeaveCriticalSection();
        return 0;
    }

    LoadNames(theater);
    CampLeaveCriticalSection();
    return 1;
}

int SaveTheater(char *theater)
{
    CampEnterCriticalSection();
    SaveTheaterTerrain(theater);
    SaveNames(theater);
    CampLeaveCriticalSection();
    return 1;
}

//
// Routines for Objectives
//

Objective AddObjectiveToCampaign(GridIndex x, GridIndex y)
{
    Objective o;

    o = NewObjective();
    o->SetLocation(x, y);
    o->UpdateObjectiveLists();
    RebuildObjectiveLists();
    RebuildParentsList();
    return o;
}

int LinkCampaignObjectives(Path path, Objective O1, Objective O2)
{
    int         i = 0, cost = 0, found = 0;
    uchar costs[MOVEMENT_TYPES] = {254};
    GridIndex   x = 0, y = 0;

    for (i = 0; i < MOVEMENT_TYPES; i++)
    {
        if (FindLinkPath(path, O1, O2, (MoveType)i) < 1)
        {
            if (FindLinkPath(path, O2, O1, (MoveType)i) < 1)
                costs[i] = 255;
            else
            {
                O2->GetLocation(&x, &y);
                cost = (FloatToInt32(path->GetCost()) + cost) / 2;

                if (cost > 254)
                    cost = 254; // If it's possible to move, but very expensive, mark it as our max movable cost (254)

                costs[i] = (uchar)cost;
                found = 1;
            }
        }
        else
        {
            O1->GetLocation(&x, &y);
            cost = FloatToInt32(path->GetCost());

            if (cost > 254)
                cost = 254; // If it's possible to move, but very expensive, mark it as our max movable cost (254)

            costs[i] = (uchar)cost;
            found = 1;
        }

        // KCK Hack: If we can get there by road, fool it into thinking it's a 254 cost link for
        // any otherwise unfound paths.
        if (costs[i] == 255 and (i == Foot or i == Wheeled or i == Tracked) and costs[NoMove] < 255)
            costs[i] = 254;
    }

    if (found)
    {
        O1->AddObjectiveNeighbor(O2, costs);
        O2->AddObjectiveNeighbor(O1, costs);
        return 1;
    }
    else
        return 0;
}

int UnLinkCampaignObjectives(Objective O1, Objective O2)
{
    int        i, unlinked = 0;

    for (i = 0; i < O1->NumLinks(); i++)
    {
        if (O1->GetNeighborId(i) == O2->Id())
        {
            O1->RemoveObjectiveNeighbor(i);
            unlinked = 1;
        }
    }

    for (i = 0; i < O2->NumLinks(); i++)
    {
        if (O2->GetNeighborId(i) == O1->Id())
        {
            O2->RemoveObjectiveNeighbor(i);
            unlinked = 1;
        }
    }

    return unlinked;
}

int RecalculateLinks(Objective o)
{
    PathClass   path;
    Objective n;
    char        nn;

    for (nn = 0; nn < o->NumLinks(); nn++)
    {
        n = o->GetNeighbor(nn);

        if (n)
            LinkCampaignObjectives(&path, o, n);
    }

    return 1;
}

//
// Unit Routines
//

Unit AddUnit(GridIndex x, GridIndex y, char Side)
{
    Unit        nu;

    nu = NewUnit(DOMAIN_LAND, TYPE_BRIGADE, STYPE_UNIT_ARMOR, 1, NULL);
    nu->SetOwner(Side);
    nu->ResetLocations(x, y);
    nu->ResetDestinations(x, y);
    nu->SetUnitOrders(GORD_DEFEND);
    return nu;
}

void RemoveUnit(Unit u)
{
    u->Remove();
}

//
// Campaign wide statics
//

int TimeOfDayGeneral(CampaignTime time)
{
    // 2001-04-10 MODIFIED BY S.G. SO IT USES MILISECOND AND NOT MINUTES
    //I COULD CHANGE THE .H FILE BUT IT WOULD TAKE TOO LONG TO RECOMPILE :-(
    /* if (time < TOD_SUNUP)
     return TOD_NIGHT;
     else if (time < TOD_SUNUP + 120*CampaignMinutes)
     return TOD_DAWNDUSK;
     else if (time < TOD_SUNDOWN - 120*CampaignMinutes)
     return TOD_DAY;
     else if (time < TOD_SUNDOWN)
     return TOD_DAWNDUSK;
     else
     return TOD_NIGHT;
     */
    // time can be over one day so I need to keep just the time in the current day...

    //Cobra We get called a lot and we are expensive.  Nothing needs this immediately
    static SIM_ULONG timer = 0;
    static long tod = 0;

    if ((timer == 0) or (SimLibElapsedTime > (SIM_ULONG)timer))
    {
        tod = time % CampaignDay;
        timer = SimLibElapsedTime + 900000;//15 minutes
    }

    //time = time % CampaignDay;
    //Cobra changed time to tod
    if (tod < TOD_SUNUP * CampaignMinutes)
        return TOD_NIGHT;
    else if (tod < TOD_SUNUP * CampaignMinutes + 120 * CampaignMinutes)
        return TOD_DAWNDUSK;
    else if (tod < TOD_SUNDOWN * CampaignMinutes - 120 * CampaignMinutes)
        return TOD_DAY;
    else if (tod < TOD_SUNDOWN * CampaignMinutes)
        return TOD_DAWNDUSK;
    else
        return TOD_NIGHT;
}

int TimeOfDayGeneral(void)
{
    return TimeOfDayGeneral(TheCampaign.TimeOfDay);
}

CampaignTime TimeOfDay(void)
{
    return TheCampaign.TimeOfDay;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// Player bubble stuff

/*
   float BubbleSize(uchar domain)
   {
   return DefBubbleSize[domain] * ((FalconSessionEntity*)vuLocalSessionEntity)->bubbleRatio;
   }
 */

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// This changes in order to try to keep a reasonable # of vehicles in bubble

void ResizeBubble(int moversInBubble)
{
    float br, new_ratio;

    br = (float)(PLAYER_BUBBLE_MOVERS - moversInBubble);
    //new_ratio = ((FalconSessionEntity*)vuLocalSessionEntity.get())->GetBubbleRatio() * 1.0F + (br * 0.01F);
    new_ratio = FalconLocalSession->GetBubbleRatio() * 1.0F + (br * 0.01F);

    if (new_ratio > 1.2F)
    {
        new_ratio = 1.2F;
    }

    if (new_ratio < 0.25F)
    {
        new_ratio = 0.25F;
    }

    //((FalconSessionEntity*)vuLocalSessionEntity)->SetBubbleRatio (new_ratio);
    FalconLocalSession->SetBubbleRatio(new_ratio);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
extern bool g_bLogEvents;
/* Here we do the following:
 *
 * a) Set Interested() state on entity.
 * Interested() means it should be deaggregated by the end of rebuild bubble
 * b) Wake or Sleep the entity locally if it's deaggregated and in/out of our bubble
 * c) Post a Deaggregate message if it's in someone's bubble and we're the host
 * d) Check if it's within our local local sim area and insert
 * or remove it from the sim lists accordingly
 */
int DeaggregationCheck(CampEntity e, FalconSessionEntity *session)
{
    int want_deaggregate = 0, want_in_sim_list = 0;
    bool didsimlistcrap = FALSE;
    int inbobble = (session->InSessionBubble(e, REAGREGATION_RATIO));

    if (e->IsAggregate())
    {
        if (session->InSessionBubble(e, 1.0F) and not g_bSleepAll)
        {
            // It's in our bubble, post deaggregate message if host
            if (e->IsLocal())
            {
                VuTargetEntity* target = (VuTargetEntity*)
                                         vuDatabase->Find(vuLocalSessionEntity->Game()->OwnerId());
                FalconSimCampMessage *msg = new FalconSimCampMessage(e->Id(), target);

                //me123 let the host own all the planes
                // sfr: host will own battallions too, since battallion 3d movement depends on 2d
                if ((e->IsBattalion()) or e->IsFlight() or (g_bServer and g_bServerHostAll))
                {
                    msg->dataBlock.from = vuLocalSessionEntity->Game()->OwnerId();

                    if (g_bLogEvents and e->IsSetFalcFlag(FEC_HASPLAYERS))
                    {
                        TheCampaign.MissionEvaluator->PreEvalFlight((Flight)e, NULL);
                    }
                }
                else
                {
                    msg->dataBlock.from = session->Id();
                }

                msg->dataBlock.message = FalconSimCampMessage::simcampDeaggregate;
                msg->RequestReliableTransmit();
                FalconSendMessage(msg);
            }

            e->SetInterest();
            want_deaggregate = 1;
            want_in_sim_list = 1;
        }
    }
    else
    {
        if (e == session->GetPlayerFlight() or inbobble)
        {
            // Mark this player entity to prevent reaggregation
            e->SetInterest();
            want_deaggregate = 1;
            want_in_sim_list = 1;
        }

        if (session == FalconLocalSession)
        {
            // Update local sleep/wake state
            // me123 this handles local wake/sleep
            // for the host dont' handle airplanes and helicopters
            if (e->IsUnit() and not g_bSleepAll)
            {
                want_in_sim_list = 1;

                if ( not e->IsAwake())
                {
                    e->Wake();
                }
            }
            else
            {
                // Update local sleep/wake state
                if (e->IsAwake() and ( not want_deaggregate or g_bSleepAll))
                {
                    if ( not vuLocalSessionEntity->Game()->IsLocal() or g_bSleepAll)
                    {
                        e->Sleep();
                    }

                    if (vuLocalSessionEntity->Game()->IsLocal())
                    {
                        if ( not e->IsAirplane() and not e->IsHelicopter())
                        {
                            e->Sleep();
                        }
                        else if (
 not e->IsObjective() or
                            (
                                e->IsObjective() and (e->GetType() not_eq TYPE_AIRBASE) and (e->GetType() not_eq TYPE_AIRSTRIP)
                            )
                        )
                            e->Sleep();
                    }
                }
                else if ( not e->IsAwake() and inbobble and not g_bSleepAll)
                {
                    e->Wake();
                }
            }

        }

        // host wake/sleep all deaged flights
        if (
            vuLocalSessionEntity->Game()->IsLocal() and 
            ( //me123 host wake/sleep stuff
                //handle airplanes helicopters and airbases
                g_bSleepAll or
                e->IsAirplane() or e->IsHelicopter()  or
                (e->IsObjective() and ((e->GetType() == TYPE_AIRBASE) or (e->GetType() == TYPE_AIRSTRIP))) or
                (g_bServer and g_bServerHostAll and e->IsUnit())
                //and airbases
            )
        )  //dedicated host wake all ents
        {
            didsimlistcrap = TRUE;

            if (g_bSleepAll) // host sleep all and get rdy to enter 3d
            {
                // this is the only case where a host wanna sleep ents
                // normaly the aggregate funktion wildo it
                if (e->IsAwake())
                {
                    e->Sleep();
                }

                e->SetChecked();
            }
            else if ( not g_bSleepAll and not e->IsSetFalcFlag(FEC_PLAYER_ENTERING) and // we are not in sleep all mode
                     (
 not e->IsSetFalcFlag(FEC_PLAYERONLY) or// not a human
                         (
                             //human but he's attached
                             e->IsSetFalcFlag(FEC_PLAYERONLY) and e->IsSetFalcFlag(FEC_HASPLAYERS)
                         )
                     )
                    )
            {
                if ((FalconLocalSession->GetFlyState() == FLYSTATE_IN_UI or
                     FalconLocalSession->GetFlyState() == FLYSTATE_FLYING or
                     FalconLocalSession->GetFlyState() == FLYSTATE_WAITING))
                    // only wake if the server isn't transitign to/from ui
                {
                    if ( not e->IsAwake())
                    {
                        e->Wake();
                    }

                    want_deaggregate = 1;
                    e->RemoveFromSimLists();
                }

            }
        }

    }

    if (session == FalconLocalSession and not didsimlistcrap)
    {
        // Update the entity's Sim List state
        if (want_in_sim_list)//me123 oldmp and InSimLists() )
        {
            e->RemoveFromSimLists();
        }
        else
        {
            VuEntity *player = session->GetCameraEntity(0);

            if (player)
            {
                e->InsertInSimLists(player->XPos(), player->YPos());
            }

            e->SetChecked();
        }
    }

    return want_deaggregate;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void ChooseNewSession(CampEntity ent)
{
    FalconSessionEntity *session = NULL, *best_session = NULL;
    VuSessionsIterator sit;
    VuEntity *player = NULL;

    float x = 0.0F, y = 0.0F, dx = 0.0F, dy = 0.0F, dist = 0.0F, best_dist = 0.0F;

    session = (FalconSessionEntity *) sit.GetFirst();
    best_session = NULL;

    while (session)
    {
        player = session->GetCameraEntity(0);

        if (player)
        {
            x = player->XPos();
            y = player->YPos();
            dx = x - ent->XPos();
            dy = y - ent->YPos();
            dist = dx * dx + dy * dy;

            if (( not best_session) or (dist < best_dist))
            {
                best_dist = dist;
                best_session = session;
            }
        }

        session = (FalconSessionEntity *)sit.GetNext();
    }

    if (best_session)
    {
        //VuTargetEntity* target = (VuTargetEntity*) vuDatabase->Find(FalconLocalGame->OwnerId());
        FalconSimCampMessage *msg = new FalconSimCampMessage(ent->Id(), FalconLocalGame);
        msg->dataBlock.from = best_session->Id();
        msg->dataBlock.message = FalconSimCampMessage::simcampChangeOwner;
        FalconSendMessage(msg);
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int AwakeCampaignEntities = 0;
int gGameType = -1;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void RebuildBubble(int forced)
{
    float sx, sy;
    // float sx2,sy2;
    VuGridIterator *myit;
    Objective o;
    Unit u;
    //CampEntity c;
    VuEntity *player;
    // VuEntity *camera2;
    // int airm = 0,gndm = 0,moversInBubble;
    int deag_ents = 0, reag_ents = 0, deag;
    int ok;
    VuSessionsIterator *sit;
    FalconSessionEntity  *session;

    CampEnterCriticalSection();


    if (forced and FalconLocalGame and g_brebuildbobbleFix)
    {
        // host wake missiles and bombs
        //MonoPrint ("FORCED SHORT REBUILD");
        if (FalconLocalGame->IsLocal())
        {
            // wake all missiles bitand bombs
            if (FalconLocalGame->IsLocal())
            {
                // host wake missiles and bombs so we can drive them
                SimBaseClass *object;
                VuListIterator dit(SimDriver.ObjsWithNoCampaignParentList);
                object = (SimBaseClass*)dit.GetFirst();
                int numberofobjects = 0;
                int wokenthiscycle = 0;
                static int wokentotal = 0;

                while (object)
                {
                    if ( not object->IsAwake() and 
 not object->IsDead() and 
                        (object->IsBomb() or object->IsMissile()))
                    {
                        object->Wake();
                        wokenthiscycle++;
                        wokentotal++;
                    }

                    object = (SimBaseClass*)dit.GetNext();
                    numberofobjects++;

                }
            }
        }
        else// just wake the missiles and bombs in our bobble
        {
            player = FalconLocalSession->GetCameraEntity(0);

            if (player)
            {
                sx = player->XPos();
                sy = player->YPos();
                UpdateNoCampaignParentObjectsWakeState(sx, sy, 100000);
            }
        }


        CampLeaveCriticalSection();
        return;
    }




    gGameType = -1;

#if USE_VU_COLL_FOR_CAMPAIGN

    if (( not FalconLocalGame) or ( not deaggregatedEntities))
    {
        CampLeaveCriticalSection();
        return;
    }

#else

    if (( not FalconLocalGame) or ( not deaggregatedMap))
    {
        CampLeaveCriticalSection();
        return;
    }

#endif

    // Check for dogfight game (special case in which flights always deaggregate)
    gGameType = FalconLocalGame->GetGameType();

    // sfr: What is this? SetInterest and UnsetInterest right after??
    // this is wrong, although Im not sure what the correct behavior is
#if USE_VU_COLL_FOR_CAMPAIGN
    VuHashIterator deagIt(deaggregatedEntities);

    for (
        CampEntity c = static_cast<CampEntity>(deagIt.GetFirst());
        c not_eq NULL;
        c = static_cast<CampEntity>(deagIt.GetNext())
    )
    {
        //if (c->IsFlight()){
        // c->SetInterest();
        //}
        c->UnsetInterest();
        reag_ents++;
    }

#else
    // Mark all currently deaggregated campaign entities as uninteresting
    {
        F4ScopeLock l(deaggregatedMap->getMutex());

        for (
            CampBaseMap::iterator it = deaggregatedMap->begin();
            it not_eq deaggregatedMap->end();
            ++it
        )
        {
            CampBaseBin cb = it->second;
            //if (cb->IsFlight()){
            // cb->SetInterest();
            //}
            cb->UnsetInterest();
            reag_ents++;
        }
    }
#endif

    // Mark all entities in sim lists as unchecked
    if (SimDriver.campUnitList)
    {
        VuListIterator cit(SimDriver.campUnitList);
        u = (Unit) cit.GetFirst();

        while (u)
        {
            u->UnsetChecked();
            u = (Unit) cit.GetNext();
        }
    }

    if (SimDriver.campObjList)
    {
        VuListIterator cit(SimDriver.campObjList);
        o = (Objective) cit.GetFirst();

        while (o)
        {
            o->UnsetChecked();
            o = (Objective) cit.GetNext();
        }
    }

    if ((FalconLocalGame) and (FalconLocalGame->IsLocal()))
    {
        // Check all cameras
        sit = new VuSessionsIterator(FalconLocalGame);
        session = (FalconSessionEntity*) sit->GetFirst();
    }
    else
    {
        // Check our camera only
        sit = NULL;
        session = FalconLocalSession;
    }

    while (session)
    {
        // Find the first camera
        player = session->GetCameraEntity(0);

        /* if ( not player and FalconLocalGame->IsLocal() and gCommsMgr->Online())
         player = FalconLocalSession->GetPlayerFlight();
         */
        if (player and (player not_eq FalconLocalSession or not sCampaignSleepRequested))
        {
            u = session->GetPlayerFlight();

            if (u)
            {
                u->SetInterest();
            }

            // We're in the cockpit
#ifdef CAMPTOOL

            if (PBubble and hMainWnd)
            {
                PostMessage(hMainWnd, WM_COMMAND, ID_CAMP_REFRESHPB, ID_CAMP_REFRESHPB);
            }

#endif

            if (player)
            {
                sx = player->XPos();
                sy = player->YPos();
            }

            /*
               if (FalconLocalSession == session)
               {
               MonoPrint ("Player Bubble %08x: %f,%f\n", player, sx, sy);
               }
               else
               {
               MonoPrint ("Remote Bubble %08x: %f,%f\n", player, sx, sy);
               }.

               camera2 = session->Camera(1);

               if (camera2)
               {
               sx2 = camera2->XPos ();
               sy2 = camera2->YPos ();
               if (FalconLocalSession == session)
               {
               MonoPrint ("Player Bubble %08x: %f,%f\n", camera2, sx2, sy2);
               }
               else
               {
               MonoPrint ("Remote Bubble %08x: %f,%f\n", camera2, sx2, sy2);
               }
               }
             */

            // Deaggregate persistant objects (This just creates/destroys a local drawable object)
            if (session == FalconLocalSession)
            {
                if (FalconLocalGame->IsLocal())
                {
                    // host wake missiles and bombs so we can drive them
                    VuListIterator dit(SimDriver.ObjsWithNoCampaignParentList);
                    SimBaseClass *object;
                    object = (SimBaseClass*)dit.GetFirst();
                    int numberofobjects = 0;
                    int wokenthiscycle = 0;
                    static int wokentotal = 0;

                    while (object)
                    {
                        if (
 not object->IsAwake() and not object->IsDead() and 
                            (object->IsBomb() or object->IsMissile())
                        )
                        {
                            object->Wake();
                            wokenthiscycle++;
                            wokentotal++;
                        }

                        object = (SimBaseClass*)dit.GetNext();
                        numberofobjects++;
                    }
                }

                UpdateNoCampaignParentObjectsWakeState(sx, sy, 100000);
                UpdatePersistantObjectsWakeState(sx, sy, PERSIST_BUBBLE_MAX, Camp_GetCurrentTime());
            }

            // Let's do objectives (max dist = reasonable max deaggreation distance plus some leeway)
            if (ObjProxList)
            {
#ifdef VU_GRID_TREE_Y_MAJOR
                myit = new VuGridIterator(ObjProxList, sy, sx, SIM_BUBBLE_SIZE);
#else
                myit = new VuGridIterator(ObjProxList, sx, sy, SIM_BUBBLE_SIZE);
#endif
                o = (Objective) myit->GetFirst();

                while (o)
                {
                    if (DeaggregationCheck(o, session) > 0)
                    {
                        if (o->IsAggregate())
                            deag_ents ++;
                    }

                    o = (Objective) myit->GetNext();
                }

                delete myit;
            }

            // Now units (max dist = reasonable max deaggreation distance plus some leeway)
            if (RealUnitProxList)
            {
#ifdef VU_GRID_TREE_Y_MAJOR
                myit = new VuGridIterator(RealUnitProxList, sy, sx, SIM_BUBBLE_SIZE);
#else
                myit = new VuGridIterator(RealUnitProxList, sx, sy, SIM_BUBBLE_SIZE);
#endif
                u = (Unit) myit->GetFirst();

                while (u)
                {
                    if (u->IsFlight() and (u->Final() or u == player))
                    {
                        if ( not u->Final())
                        {
                            u->SetFinal(1);
                        }

                        WayPoint w = u->GetCurrentUnitWP();

                        if ( not w)
                        {
                            deag = 0;
                        }
                        else if (w->GetWPAction() not_eq WP_TAKEOFF)
                        {
                            deag = 1;
                        }
                        else if ( not u->IsAggregate())
                        {
                            deag = 1;
                        }
                        else
                        {
                            CampaignTime minDeagTime;
                            Objective airbase = (Objective)u->GetUnitAirbase();

                            // We'll deaggregate a few minutes before takeoff for objective airbases
                            if (airbase and airbase->IsObjective() and airbase->brain)
                            {
                                if (GetTTRelations(airbase->GetTeam(), u->GetTeam()) <= Neutral)
                                {
                                    runwayQueueStruct *info = airbase->brain->InList(u->Id());

                                    if (info)
                                    {
                                        minDeagTime = 0;
                                    }
                                    else
                                    {
                                        int rwindex = airbase->brain->FindBestTakeoffRunway(TRUE);
                                        ulong nextTOTime = airbase->brain->FindFlightTakeoffTime(
                                                               (Flight)u, GetQueue(rwindex)
                                                           );

                                        minDeagTime = w->GetWPArrivalTime() - airbase->brain->MinDeagTime();

                                        if (gCommsMgr and gCommsMgr->Online())
                                        {
                                            //me123
                                            if ( not u->IsSetFalcFlag(FEC_PLAYER_ENTERING bitor FEC_HASPLAYERS))
                                            {
                                                minDeagTime = max(
                                                                  minDeagTime, nextTOTime - airbase->brain->MinDeagTime()
                                                              );
                                            }
                                            else if (
                                                u->IsFlight() and 
                                                (((Flight)u)->GetEvalFlags() bitand FEVAL_START_COLD)
                                            )
                                            {
                                                minDeagTime -= CampaignMinutes * PlayerOptionsClass::RAMP_MINUTES;
                                            }
                                        }
                                        else if (
                                            u->IsSetFalcFlag(FEC_PLAYER_ENTERING bitor FEC_HASPLAYERS) and 
                                            u->IsFlight()
                                        )
                                        {
                                            //JPO we need some extra time before takeoff if starting cold.
                                            if (((Flight)u)->GetEvalFlags() bitand FEVAL_START_COLD)
                                            {
                                                minDeagTime -= CampaignMinutes * PlayerOptionsClass::RAMP_MINUTES;
                                            }
                                        }
                                        else
                                        {
                                            minDeagTime -= CampaignMinutes * g_nDeagTimer;
                                        }

                                        if (TheCampaign.CurrentTime >= minDeagTime)
                                        {
                                            //if we will be deagg'ed we need to make sure the slot is filled so
                                            //another flight won't also think it's available
                                            int numVeh = u->GetTotalVehicles();

                                            for (int i = 0; i < numVeh; i++)
                                            {
                                                airbase->brain->AddTraffic(u->Id(), noATC, rwindex, nextTOTime);
                                                nextTOTime += TAKEOFF_TIME_DELTA / 2;
                                                // 27JAN04 - FRB - Bunch flight TO's closer together
                                            }
                                        }
                                    }
                                }
                                else
                                {
                                    //if we are trying to take off from a hostile airbase, cancel the flight
                                    minDeagTime = TheCampaign.CurrentTime + 1;
                                    CancelFlight((Flight)u);
                                }
                            }
                            else
                            {
                                minDeagTime = w->GetWPArrivalTime(); // Carrier takeoff

                                // JB carrier start
                                if (u->IsSetFalcFlag(FEC_PLAYER_ENTERING bitor FEC_HASPLAYERS) and u->IsFlight())
                                {
                                    //JPO we need some extra time before takeoff if starting cold.
                                    if (((Flight)u)->GetEvalFlags() bitand FEVAL_START_COLD)
                                    {
                                        minDeagTime -= CampaignMinutes * PlayerOptionsClass::RAMP_MINUTES;
                                    }
                                    else if (PlayerOptions.GetStartFlag() == PlayerOptionsClass::START_TAXI)
                                    {
                                        minDeagTime -= CampaignMinutes * g_nTaxiLaunchTime;
                                    }

                                    // Booster 2004/10/12 Taxi takeoff time
                                }

                                // JB carrier end
                            }

                            if (TheCampaign.CurrentTime >= minDeagTime)
                            {
                                deag = 1;
                            }
                            else
                            {
                                deag = 0;
                            }
                        }

                        if (deag)
                        {
                            if ( not u->IsDead() and DeaggregationCheck(u, session) > 0)
                            {
                                if (u->IsAggregate())
                                {
                                    deag_ents ++;
                                }
                            }
                        }
                    }
                    else if ((u->IsBattalion() or u->IsTaskForce()) and not u->Inactive())
                    {
                        if ( not u->IsDead() and not u->Inactive() and DeaggregationCheck(u, session) > 0)
                        {
                            if (u->IsAggregate())
                            {
                                deag_ents ++;
                            }
                        }
                    }

                    u = (Unit)myit->GetNext();
                }

                delete myit;
            }
        }

        if (sit)
        {
            session = (FalconSessionEntity*) sit->GetNext();
        }
        else
        {
            session = NULL;
        }
    }

    if (sit)
    {
        delete sit;
    }

    ///////////////////////////////////////////////////////////////////////////////
    // This is how many Campaign entities we're going to deaggregate this cycle.
    gLeftToDeaggregate = deag_ents;

    if (FalconLocalGame->IsLocal())
    {
        sit = new VuSessionsIterator(FalconLocalGame);
        session = (FalconSessionEntity *) sit->GetFirst();

        while (session)
        {
            if (session not_eq FalconLocalSession)
            {
#if USE_VU_COLL_FOR_CAMPAIGN
                VuHashIterator deagIt(deaggregatedEntities);

                for (
                    CampEntity c = static_cast<CampEntity>(deagIt.GetFirst());
                    c not_eq NULL;
                    c = static_cast<CampEntity>(deagIt.GetNext())
                )
                {
                    if ((c->IsUnit()) and (c->GetDeagOwner() == session->Id()))
                    {
                        if ( not session->GetCameraEntity(0))
                        {
                            ChooseNewSession(c);
                        }
                    }
                }

#else
                //me123 let the host own the ents even when he's in the ui
                F4ScopeLock l(deaggregatedMap->getMutex());

                for (
                    CampBaseMap::iterator it = deaggregatedMap->begin();
                    it not_eq deaggregatedMap->end();
                    ++it
                )
                {
                    CampBaseBin cb = it->second;

                    if ((cb->IsUnit()) and (cb->GetDeagOwner() == session->Id()))
                    {
                        if ( not session->GetCameraEntity(0))
                        {
                            ChooseNewSession(cb.get());
                        }
                    }
                }

#endif
            }

            session = (FalconSessionEntity *) sit->GetNext();
        }

        delete sit; // JPO memory leak fix
    }

    if (FalconLocalGame->IsLocal())
    {
        // Now Reaggregate any of our previous CampEntities which are no longer interesting
        if (reag_ents)
        {
#if USE_VU_COLL_FOR_CAMPAIGN
            VuHashIterator deagIt(deaggregatedEntities);

            for (
                CampEntity c = static_cast<CampEntity>(deagIt.GetFirst());
                c not_eq NULL;
                c = static_cast<CampEntity>(deagIt.GetNext())
            )
            {
                if ( not c->IsInterested())
                {
                    ok = TRUE;

                    if (c->IsFlight())
                    {
                        VuSessionsIterator sit(FalconLocalGame);

                        for (
                            session = static_cast<FalconSessionEntity*>(sit.GetFirst());
                            session not_eq NULL;
                            session = static_cast<FalconSessionEntity*>(sit.GetNext())
                        )
                        {
                            if (session->GetPlayerFlight() == c)
                            {
                                if (session->GetCameraEntity(0))
                                {
                                    ok = FALSE;
                                }
                            }
                        }
                    }

                    if (ok)
                    {
                        VuTargetEntity* target = (VuTargetEntity*) vuDatabase->Find(FalconLocalGame->OwnerId());
                        FalconSimCampMessage *msg = new FalconSimCampMessage(c->Id(), target);
                        msg->dataBlock.from = FalconLocalSessionId;
                        msg->dataBlock.message = FalconSimCampMessage::simcampReaggregate;
                        FalconSendMessage(msg);
                    }
                }
            }

#else
            F4ScopeLock l(deaggregatedMap->getMutex());

            for (
                CampBaseMap::iterator it = deaggregatedMap->begin();
                it not_eq deaggregatedMap->end();
                ++it
            )
            {
                CampBaseBin cb = it->second;

                if ( not cb->IsInterested())
                {
                    ok = TRUE;

                    if (cb->IsFlight())
                    {
                        sit = new VuSessionsIterator(FalconLocalGame);
                        session = (FalconSessionEntity*) sit->GetFirst();

                        while (session)
                        {
                            if (session->GetPlayerFlight() == cb)
                            {
                                if (session->GetCameraEntity(0))
                                {
                                    ok = FALSE;
                                }
                            }

                            session = (FalconSessionEntity *) sit->GetNext();
                        }

                        delete sit; // JPO memory leak fix
                    }

                    if (ok)
                    {
                        VuTargetEntity* target = (VuTargetEntity*) vuDatabase->Find(FalconLocalGame->OwnerId());
                        FalconSimCampMessage *msg = new FalconSimCampMessage(cb->Id(), target);
                        msg->dataBlock.from = FalconLocalSessionId;
                        msg->dataBlock.message = FalconSimCampMessage::simcampReaggregate;
                        FalconSendMessage(msg);
                    }
                }
            }

#endif
        }
    }
    else
    {
        // Now Reaggregate any of our previous CampEntities which are no longer interesting
#if 0
        if (reag_ents)
        {
            c = (CampEntity) deag_it.GetFirst();

            while (c)
            {
                if ( not c->IsInterested())
                {
                    ok = TRUE;

                    if (c->IsFlight())
                    {
                        sit = new VuSessionsIterator(FalconLocalGame);
                        session = (FalconSessionEntity*) sit->GetFirst();

                        while (session)
                        {
                            if (session->GetPlayerFlight() == c)
                            {
                                ok = FALSE;
                            }

                            session = (FalconSessionEntity *) sit->GetNext();
                        }

                        delete sit; //me123 memory leak

                        if ( not ok)
                        {
                            // MonoPrint ("Reaggregating Flight that is a player flight %08x\n", c);
                        }
                    }

                    if (ok)
                    {
                        //float
                        // dist = 0.0;

                        //MonoPrint ("I WANT TO REAGGREGATE %08x %f\n", c, dist);
                    }
                }

                c = (CampEntity) deag_it.GetNext();
            }
        }

#endif
    }

    if (sCampaignSleepRequested)
    {
        // Chill all persistant object draw pointers
        UpdatePersistantObjectsWakeState(0, 0, 0, TheCampaign.CurrentTime);
        UpdateNoCampaignParentObjectsWakeState(0, 0, 0);
#if USE_VU_COLL_FOR_CAMPAIGN
        VuHashIterator deagIt(deaggregatedEntities);

        for (
            CampEntity c = static_cast<CampEntity>(deagIt.GetFirst());
            c not_eq NULL;
            c = static_cast<CampEntity>(deagIt.GetNext())
        )
        {
            if (c->IsAwake())
            {
                c->Sleep();
                c->UnsetChecked();
            }
        }

#else
        // Sleep all deaggregated campaign entities
        F4ScopeLock l(deaggregatedMap->getMutex());
        for_each(deaggregatedMap->begin(), deaggregatedMap->end(), CampBaseClass::SleepAndUnsetCheckedOp());
#endif
    }

    // Now remove any CampEntities which we are no longer interested in
    if (SimDriver.campUnitList)
    {
        VuListIterator cit(SimDriver.campUnitList);
        u = (Unit) cit.GetFirst();

        while (u)
        {
            Unit next = static_cast<Unit>(cit.GetNext());

            if ( not u->IsChecked())
            {
                u->RemoveFromSimLists();
            }

            u = next;
        }
    }

    if (SimDriver.campObjList)
    {
        VuListIterator cit(SimDriver.campObjList);
        o = (Objective) cit.GetFirst();

        while (o)
        {
            Objective next = static_cast<Objective>(cit.GetNext());

            if ( not o->IsChecked())
            {
                o->RemoveFromSimLists();
            }

            o = next;
        }
    }

    sCampaignSleepRequested = 0;
    CampLeaveCriticalSection();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void CampaignRequestSleep(void)
{
    // This critical section wouldn't be necessary if it
    // wasn't for the simloop shutdown thread.
    CampEnterCriticalSection();
    sCampaignSleepRequested = 1;
    CampLeaveCriticalSection();
}

int CampaignAllAsleep(void)
{
    return 1 - sCampaignSleepRequested;
}

// Check proximity of an effect to the player
int InterestingSFX(float x, float y)
{
    float d, xd, yd;
    VuEntity *player;

    player = FalconLocalSession->GetCameraEntity(0);

    if ( not player)        // or not in the cockpit
        return 0;

    xd = player->XPos() - x;
    yd = player->YPos() - y;
    d = xd * xd + yd * yd;

    if (d < MAX_DISTANT_EFFECT_DIST_SQ)
    {
#ifndef NDEBUG
        // MonoPrint("Firing distant visual effect at %f,%f.\n",x,y);
#endif
        return 1;
    }

    return 0;
}

// =======================================
// File IO stuff
// =======================================

int CreateCampFile(char *filename, char* path)
{
    char fullname[MAX_PATH];
    FILE* fp;

    // This filename doesn't exist yet (At least res manager doesn't think so)
    // Create it, so that the manager can find it -
    sprintf(fullname, "%s\\%s", path, filename);
    fp = fopen(fullname, "wb");
    fclose(fp);
    // Now add the current save directory path, if we still can't find this file
    // if ( not ResExistFile(filename))
    // ResAddPath(path, FALSE);
    return 1;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// WCH files are constantly opened and closed, since they hold string
// data... These will now only be closed if another "different" wch file
// is requested.

// We allow 4 WCH files - we only currently use two, but given future products
// I'm setting it to 4 so that they don't not work.

#define MAX_WCH_FILES 4

static int
next_wch_file = 0;

static FILE
*wch_fp[MAX_WCH_FILES] = { 0 };

static char
wch_filename[MAX_WCH_FILES][MAX_PATH];

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// Campressed files ".CAM" bitand ".TAC" files information is stored in these
// variables. These are all static, and adjusted by the functions below.

static int
writing_campressed_file = FALSE,
reading_campressed_file = FALSE;

static FILE
*camp_fp;

static char
camp_names[32][255],
           camp_file_name[MAX_PATH];

static int
camp_num_files,
camp_offset[32],
camp_size[32];

static FalconGameType
camp_game_type;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static void GetCampFilePath(FalconGameType type, char *filename, char* path)
{
    FILE
    *fp;

    if (type == game_TacticalEngagement)
    {
        sprintf(path, "%s\\%s.tac", FalconCampUserSaveDirectory, filename);

        fp = fopen(path, "rb");

        if (fp)
        {
            fclose(fp);
        }
        else
        {
            sprintf(path, "%s\\%s.trn", FalconCampUserSaveDirectory, filename);
        }
    }
    else
    {
        sprintf(path, "%s\\%s.cam", FalconCampUserSaveDirectory, filename);
    }
}

static int IsCampFile(FalconGameType type, char *filename)
{
    char
    path[MAX_PATH];
    FILE
    *fp;

    GetCampFilePath(type, filename, path);

    fp = fopen(path, "rb");

    if ( not fp)
        return 0;

    fclose(fp);
    return 1;
}

int string_compare_extensions(char *one, char *two)
{
    char
    *one_ext,
    *two_ext;

    one_ext = one;
    two_ext = two;

    while (*one)
    {
        if (*one == '.')
        {
            one_ext = one;
        }

        one ++;
    }

    while (*two)
    {
        if (*two == '.')
        {
            two_ext = two;
        }

        two ++;
    }

    return stricmp(one_ext, two_ext);
}

void StartReadCampFile(FalconGameType type, char *filename)
{
    int
    index,
    str_len,
    offset;

    char
    path[MAX_PATH];

    if (reading_campressed_file)
    {
        MonoPrint("Already Reading Campressed File\n");
        ShiAssert( not reading_campressed_file);
        return;
    }

    reading_campressed_file = TRUE;

    GetCampFilePath(type, filename, path);

    camp_game_type = type;
    strcpy(camp_file_name, filename);

    camp_fp = fopen(path, "rb");

    if (camp_fp)
    {
        // MonoPrint ("Opening Campressed File %s\n", path);

        fread(&offset, 4, 1, camp_fp);

        fseek(camp_fp, offset, 0);

        fread(&camp_num_files, 4, 1, camp_fp);

        for (index = 0; index < camp_num_files; index ++)
        {
            fread(&str_len, 1, 1, camp_fp);

            str_len and_eq 0xff;

            fread(camp_names[index], str_len, 1, camp_fp);

            camp_names[index][str_len] = 0;

            fread(&camp_offset[index], 4, 1, camp_fp);

            fread(&camp_size[index], 4, 1, camp_fp);

            // MonoPrint ("  %s %d %d\n", camp_names[index], camp_offset[index], camp_size[index]);
        }
    }
    else
    {
        MonoPrint("Cannot Open %s\n", path);

        ShiWarning("Cannot Open Campressed File\n");
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
CampaignData ReadCampFile(char *filename, char *ext)
{
    int size, index;

    CampaignData cd = { -1, NULL};
    char /* *data,*/ buffer[MAX_PATH];

    FILE *fp;

    if (reading_campressed_file)
    {
        if ((strcmp(filename, camp_file_name) not_eq 0) and IsCampFile(camp_game_type, filename))
        {
            EndReadCampFile();
            StartReadCampFile(camp_game_type, filename);
            ShiAssert(reading_campressed_file);
        }

        strcpy(buffer, filename);
        strcat(buffer, ".");
        strcat(buffer, ext);

        for (index = 0; index < camp_num_files; index ++)
        {
            if (string_compare_extensions(buffer, camp_names[index]) == 0)
            {
                fseek(camp_fp, camp_offset[index], 0);
                cd.dataSize = camp_size[index] + 1;
                cd.data = new char [cd.dataSize];
                fread(cd.data, camp_size[index], 1, camp_fp);
                cd.data[camp_size[index]] = 0;
                return cd;
            }
        }

        reading_campressed_file = FALSE;
        fp = OpenCampFile(filename, ext, "rb");
        reading_campressed_file = TRUE;
    }
    else
    {
        fp = OpenCampFile(filename, ext, "rb");
    }

    if (fp)
    {
        fseek(fp, 0, 2);
        size = ftell(fp);
        fseek(fp, 0, 0);
        cd.dataSize = size + 1;
        cd.data = new char [cd.dataSize];
        fread(cd.data, size, 1, fp);
        cd.data[size] = 0;
        fclose(fp);
        return cd;
    }

    return cd;
    // return NULL;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void EndReadCampFile(void)
{
    if (camp_fp)
    {
        fclose(camp_fp);

        camp_fp = NULL;

        camp_num_files = 0;
    }

    reading_campressed_file = FALSE;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void StartWriteCampFile(FalconGameType type, char *filename)
{
    char
    path[MAX_PATH];

    writing_campressed_file = TRUE;

    if (type == game_TacticalEngagement)
    {
        sprintf(path, "%s\\%s.tac", FalconCampUserSaveDirectory, filename);
    }
    else
    {
        sprintf(path, "%s\\%s.cam", FalconCampUserSaveDirectory, filename);
    }

    camp_fp = fopen(path, "wb");

    if (camp_fp)
    {
        camp_num_files = 0;

        fwrite(&camp_num_files, sizeof(int), 1, camp_fp);
    }
    else
    {
        MonoPrint("Cannot Open %s\n", path);

        ShiWarning("Cannot Open Campressed File\n");
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void EndWriteCampFile(void)
{
    int
    str_len,
    index,
    offset;

    if (camp_fp)
    {
        // MonoPrint ("Writing Campressed File\n");

        fseek(camp_fp, 0, 2);

        offset = ftell(camp_fp);

        fseek(camp_fp, 0, 0);

        fwrite(&offset, sizeof(int), 1, camp_fp);

        fseek(camp_fp, offset, 0);

        fwrite(&camp_num_files, 4, 1, camp_fp);

        for (index = 0; index < camp_num_files; index ++)
        {
            str_len = strlen(camp_names[index]);

            fwrite(&str_len, 1, 1, camp_fp);

            fwrite(camp_names[index], str_len, 1, camp_fp);

            fwrite(&camp_offset[index], 4, 1, camp_fp);

            fwrite(&camp_size[index], 4, 1, camp_fp);

            // MonoPrint ("  %s %d %d\n", camp_names[index], camp_offset[index], camp_size[index]);
        }

        fclose(camp_fp);

        camp_fp = NULL;
    }

    writing_campressed_file = FALSE;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FILE* OpenCampFile(char *filename, char *ext, char *mode)
{
    char fullname[MAX_PATH], path[MAX_PATH];
    int index;

    char
    buffer[MAX_PATH];

    FILE
    *fp;

    if (strcmp(ext, "wch") == 0)
    {
        for (index = 0; index < MAX_WCH_FILES; index ++)
        {
            if ((wch_fp[index]) and (strcmp(wch_filename[index], filename) == 0))
            {
                fseek(wch_fp[index], 0, 0);

                return wch_fp[index];
            }
        }
    }

    sprintf(buffer, "OpenCampFile %s.%s %s\n", filename, ext, mode);
    // MonoPrint (buffer);
    // OutputDebugString (buffer);

    // 2002-03-25 MN added check for not being WCH file - otherwise can crash sometimes
    // especially after theater switching situations 
    if ((reading_campressed_file) and (mode[0] == 'r') and strcmp(ext, "wch") not_eq 0)
    {
        if (strcmp(filename, camp_file_name) not_eq 0 and IsCampFile(camp_game_type, filename))
        {
            EndReadCampFile();
            StartReadCampFile(camp_game_type, filename);
            ShiAssert(reading_campressed_file);
        }

        strcpy(fullname, filename);
        strcat(fullname, ".");
        strcat(fullname, ext);

        for (index = 0; index < camp_num_files; index ++)
        {
            if (string_compare_extensions(fullname, camp_names[index]) == 0)
            {
                fseek(camp_fp, camp_offset[index], 0);

                return camp_fp;
            }
        }

        // MonoPrint ("Cannot OpenCampFile while we are reading a campressed file\n");
    }

    if ((writing_campressed_file) and (mode[0] == 'w'))
    {
        strcpy(fullname, filename);
        strcat(fullname, ".");
        strcat(fullname, ext);

        fseek(camp_fp, 0, 2);

        strcpy(camp_names[camp_num_files], fullname);

        camp_offset[camp_num_files] = ftell(camp_fp);

        camp_size[camp_num_files] = 0;

        return camp_fp;
    }

    if (strcmp(ext, "cmp") == 0)
        sprintf(path, FalconCampUserSaveDirectory);
    else if (stricmp(ext, "obj") == 0)
        sprintf(path, FalconCampUserSaveDirectory);
    else if (stricmp(ext, "obd") == 0)
        sprintf(path, FalconCampUserSaveDirectory);
    else if (stricmp(ext, "uni") == 0)
        sprintf(path, FalconCampUserSaveDirectory);
    else if (stricmp(ext, "tea") == 0)
        sprintf(path, FalconCampUserSaveDirectory);
    else if (stricmp(ext, "wth") == 0)
        sprintf(path, FalconCampUserSaveDirectory);
    else if (stricmp(ext, "plt") == 0)
        sprintf(path, FalconCampUserSaveDirectory);
    else if (stricmp(ext, "mil") == 0)
        sprintf(path, FalconCampUserSaveDirectory);
    else if (stricmp(ext, "tri") == 0)
        sprintf(path, FalconCampUserSaveDirectory);
    else if (stricmp(ext, "evl") == 0)
        sprintf(path, FalconCampUserSaveDirectory);
    else if (stricmp(ext, "smd") == 0)
        sprintf(path, FalconCampUserSaveDirectory);
    else if (stricmp(ext, "sqd") == 0)
        sprintf(path, FalconCampUserSaveDirectory);
    else if (stricmp(ext, "pol") == 0)
        sprintf(path, FalconCampUserSaveDirectory);
    else if (stricmp(ext, "ct") == 0)
        sprintf(path, FalconObjectDataDir);
    else if (stricmp(ext, "ini") == 0)
        sprintf(path, FalconObjectDataDir);
    else if (stricmp(ext, "ucd") == 0)
        sprintf(path, FalconObjectDataDir);
    else if (stricmp(ext, "ocd") == 0)
        sprintf(path, FalconObjectDataDir);
    else if (stricmp(ext, "fcd") == 0)
        sprintf(path, FalconObjectDataDir);
    else if (stricmp(ext, "vcd") == 0)
        sprintf(path, FalconObjectDataDir);
    else if (stricmp(ext, "wcd") == 0)
        sprintf(path, FalconObjectDataDir);
    else if (stricmp(ext, "rcd") == 0)
        sprintf(path, FalconObjectDataDir);
    else if (stricmp(ext, "icd") == 0)
        sprintf(path, FalconObjectDataDir);
    else if (stricmp(ext, "rwd") == 0)
        sprintf(path, FalconObjectDataDir);
    else if (stricmp(ext, "vsd") == 0)
        sprintf(path, FalconObjectDataDir);
    else if (stricmp(ext, "swd") == 0)
        sprintf(path, FalconObjectDataDir);
    else if (stricmp(ext, "acd") == 0)
        sprintf(path, FalconObjectDataDir);
    else if (stricmp(ext, "wld") == 0)
        sprintf(path, FalconObjectDataDir);
    else if (stricmp(ext, "phd") == 0)
        sprintf(path, FalconObjectDataDir);
    else if (stricmp(ext, "pd") == 0)
        sprintf(path, FalconObjectDataDir);
    else if (stricmp(ext, "fed") == 0)
        sprintf(path, FalconObjectDataDir);
    else if (stricmp(ext, "ssd") == 0)
        sprintf(path, FalconObjectDataDir);
    else if (stricmp(ext, "rkt") == 0) // 2001-11-05 Added by M.N.
        sprintf(path, FalconObjectDataDir);
    else if (stricmp(ext, "ddp") == 0) // 2002-04-20 Added by M.N.
        sprintf(path, FalconObjectDataDir);
    else
        sprintf(path, FalconCampaignSaveDirectory);

    // Outdated by resmgr:
    // if ( not ResExistFile(filename))
    // ResAddPath(path, FALSE);

    sprintf(fullname, "%s\\%s.%s", path, filename, ext);
    fp = fopen(fullname, mode);

    if ((fp) and (strcmp(ext, "wch") == 0))
    {
        index = next_wch_file ++;

        F4Assert(next_wch_file <= MAX_WCH_FILES)

        wch_fp[index] = fp;
        strcpy(wch_filename[index], filename);
    }

    return fp;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void CloseCampFile(FILE *fp)
{
    int
    index;

    for (index = 0; index < MAX_WCH_FILES; index ++)
    {
        if (fp == wch_fp[index])
        {
            // do nothing - we want to keep this file open...
            // MonoPrint ("Keeping WCH File %s.wch\n", wch_filename);
            return;
        }
    }

    if (fp == camp_fp)
    {
        if (writing_campressed_file)
        {
            fseek(camp_fp, 0, 2);

            camp_size[camp_num_files] = ftell(camp_fp) - camp_offset[camp_num_files];

            camp_num_files ++;
        }
        else if (reading_campressed_file)
        {
        }
    }
    else
    {
        if (fp)
        {
            fclose(fp);
        }
    }
}

// JPO - forcibly shut the WCH files
void ClearCampCache()
{
    for (int index = 0; index < MAX_WCH_FILES; index ++)
    {
        if (wch_fp[index])
        {
            fclose(wch_fp[index]);
            wch_fp[index] = NULL;
            strcpy(wch_filename[index], "");
            next_wch_file--;
        }
    }
}

// =======================================
// Local Functions
// =======================================

void ChooseBullseye(void)
{
    if ( not g_bFloatingBullseye) // JB/Codec 010115
        return; // JB/Codec 010115

    // 2000-11-27 REMOVED BY S.G. SORRY KEVIN, NO MORE HARDCODED BULLSEYE :-)
    // KCK: They want a fixed Bullseye.. So.. here it is:
    // TheCampaign.SetBullseye(1,390,464);
    // END OF REMOVED SECTION
    // This code will pick the best frontline objective as a bullseye
    //reenabled by me123
    Objective o;
    GridIndex cx, cy, x, y, bestx = -1, besty = -1;
    float d, bestd = 999.9F;

    if (g_nChooseBullseyeFix bitand 0x01)
    {
        cx = TheaterXPosition; // from falcon4.aii - theater dependant
        cy = TheaterYPosition;
    }
    else
    {
        cx = Map_Max_X / 2;
        cy = Map_Max_Y / 2;
    }

    // Choose the best frontline objective as a bullseye
    {
        VuListIterator frontit(FrontList);
        o = (Objective) frontit.GetFirst();

        while (o)
        {
            o->GetLocation(&x, &y);
            d = Distance(x, y, cx, cy);

            if (d < bestd)
            {
                bestx = x;
                besty = y;
                bestd = d;
            }

            o = (Objective) frontit.GetNext();
        }
    }

    if (bestx >= 0)
        TheCampaign.SetBullseye(1, bestx, besty);
    else
        TheCampaign.SetBullseye(1, 390, 464); //me123 from 0,0,0

}

// =======================================
// Mission launch stuff
// =======================================

CampaignTime gCompressTillTime = 0;
CampaignTime gLaunchTime = 0;
CampaignTime gOldCompressTillTime = 0;
CampaignTime gOldCompressionRatio = -1;

void SetEntryTime(Flight flight)
{


    static VU_TIME timer = NULL;
    static VU_TIME  startvuxGameTime = NULL;
    static int starttimer = FALSE;

    if (timer)
    {
        timer = vuxGameTime - startvuxGameTime;

        if ( not timer) timer = 1;
    }
    else if (starttimer)
    {
        startvuxGameTime = vuxGameTime;
        timer = 1;
    }

    if (timer > 3 * CampaignSeconds)
    {
        timer = NULL;
        starttimer = FALSE;
    }

    static int lastchoice = 0;

    if (g_bMPStartRestricted and gCommsMgr and gCommsMgr->Online())
    {
        gCompressTillTime = sTakeoffTime - g_nMPStartTime * CampaignMinutes;
    }


    else if ( not gCompressTillTime or lastchoice not_eq PlayerOptions.GetStartFlag())
    {
        // JPO - decide where we start
        switch (PlayerOptions.GetStartFlag())
        {
            case PlayerOptionsClass::START_RAMP:
                gCompressTillTime = sTakeoffTime - PlayerOptionsClass::RAMP_MINUTES * CampaignMinutes;
                flight->SetEvalFlag(FEVAL_START_COLD);
                break;

            case PlayerOptionsClass::START_TAXI:
                // Booster 2004/10/12 Taxi takeoff time option
                gCompressTillTime = sTakeoffTime - g_nTaxiLaunchTime * CampaignMinutes;
                break;

            case PlayerOptionsClass::START_RUNWAY:
                if (gCommsMgr and gCommsMgr->Online())
                    gCompressTillTime = sTakeoffTime - CampaignMinutes;
                else
                    gCompressTillTime = sTakeoffTime;

                break;

            default:
                ShiWarning("Undefined Start time");
        }

        starttimer = TRUE;
    }

    lastchoice = PlayerOptions.GetStartFlag();


    //me123 give the player a few sec to make a change.
    if ((PlayerOptions.GetStartFlag() not_eq PlayerOptionsClass::START_RUNWAY) and timer and 
        (timer < (VU_TIME)(3 * CampaignSeconds)) and 
        (gLaunchTime < (CampaignTime)(vuxGameTime + 3 * CampaignSeconds)))
    {

        gLaunchTime = min(((VU_TIME)sTakeoffTime) , (vuxGameTime + 3 * CampaignSeconds));
    }

}

// This will cause the campaign to compress until the current mission's takeoff time
int CompressCampaignUntilTakeoff(Flight flight)
{
    WayPoint w;

    ShiAssert(flight);
    /////////////////me123 check all clients takeoff time if we are host
    int firstentrytime = 0;

    if (FalconLocalGame and FalconLocalGame->IsLocal())
    {
        VuSessionsIterator sessionWalker(FalconLocalGame);
        FalconSessionEntity *sess;
        sess = (FalconSessionEntity*)sessionWalker.GetFirst();

        while (sess)
        {

            Flight flt = sess->GetPlayerFlight();

            if (flt)
            {
                if (flt->GetCurrentUnitWP() and 
                    (flt->GetCurrentUnitWP()->GetWPAction() == WP_TAKEOFF))
                {

                    if ( not firstentrytime)
                    {
                        firstentrytime = flt->GetCurrentUnitWP()->GetWPArrivalTime();
                    }
                    else if (flt->GetCurrentUnitWP()->GetWPArrivalTime() < (CampaignTime)firstentrytime)
                    {
                        firstentrytime = flt->GetCurrentUnitWP()->GetWPArrivalTime();
                    }
                }
            }

            sess = (FalconSessionEntity*)sessionWalker.GetNext();
        }

    }
    else
    {
        // we are a client

    }

    /////////////////me123
    if (TheCampaign.IsSuspended())
        TheCampaign.Resume();

    w = flight->GetCurrentUnitWP();

    if (w and w->GetWPAction() == WP_TAKEOFF)
    {
        // Tell the flight to hold short if we're coming into the sim.
        flight->SetFalcFlag(FEC_HOLDSHORT);

        if (FalconLocalGame->IsLocal())  sTakeoffTime = firstentrytime;
        else  sTakeoffTime = w->GetWPArrivalTime();

        gLaunchTime = w->GetWPArrivalTime() ;

        switch (PlayerOptions.GetStartFlag())
        {
            case PlayerOptionsClass::START_RAMP:
                gLaunchTime -= PlayerOptionsClass::RAMP_MINUTES * CampaignMinutes;
                break;

            case PlayerOptionsClass::START_TAXI:
                gLaunchTime -= g_nTaxiLaunchTime * CampaignMinutes; // Booster 2004/10/12 Taxi takeoff time option
                break;

            case PlayerOptionsClass::START_RUNWAY:
                if (gCommsMgr and gCommsMgr->Online())
                    gLaunchTime -= CampaignMinutes;

                break;
        }


        SetEntryTime(flight);
        return -1;
    }
    else
    {
        sTakeoffTime = 0;
        gCompressTillTime = Camp_GetCurrentTime();
        // 2002-04-06 MN also set gLaunchTime to current time, or we'll use the launch time of
        // the mission we've run previously, if our current waypoint is not the takeoff waypoint
        // So - let us into the sim immediately.
        gLaunchTime = Camp_GetCurrentTime();
        return 1;
    }
}

void CancelCampaignCompression(void)
{
    gCompressTillTime = 0;
    gOldCompressTillTime = 0;
    SetTimeCompression(1);
}

void DoCompressionLoop(void)
{
    if (gCompressTillTime > 0)
    {
        Flight pf = FalconLocalSession->GetPlayerFlight();

        // Reset gCompressTillTime, in case player has changed options
        if (sTakeoffTime)
            CompressCampaignUntilTakeoff(pf);

        // Validate that the flight/ac combination the player is entering is still valid
        if ( not pf or pf->IsDead() or pf->Aborted())
        {
            gCompressTillTime = 0;
            UI_HandleFlightCancel();
            return;
        }

        if (pf->plane_stats[FalconLocalSession->GetAircraftNum()] not_eq AIRCRAFT_AVAILABLE)
        {
            gCompressTillTime = 0;
            UI_HandleAircraftDestroyed();
            return;
        }

        /* if ()
         {
         gCompressTillTime = 0;
         UI_HandleAirbaseDestroyed();
         return;
         }
         */

        // Check for sim entry
        if (gLaunchTime <= vuxGameTime and gCompressTillTime <= vuxGameTime)
        {
            // Now stop moving time forward and resync the campaign
            SetTimeCompression(0);

            if (Camp_GetCurrentTime() >= vuxGameTime)
            {
                // We've reached our takeoff time, now takeoff
                // Minimize bubble ratio for our session
                SimLibElapsedTime = vuxGameTime;
                UPDATE_SIM_ELAPSED_SECONDS; // COBRA - RED - Scale Elapsed Seconds
                gCompressTillTime = 0;

                // OW FIXME: sometimes gets called when mainhandler is already freed
                // if (gMainHandler->GetAppWnd ())
                if (gMainHandler and gMainHandler->GetAppWnd())
                {
                    switch (FalconLocalGame->GetGameType())
                    {
                        case game_InstantAction:
                        case game_Dogfight:
                            gMainHandler->SetDrawFlag(0); // Hack to keep the UI from drawing
                            PostMessage(gMainHandler->GetAppWnd(), FM_START_DOGFIGHT, 0, 0);
                            break;

                        case game_TacticalEngagement:
                            if (pf->GetFirstUnitWP() == pf->GetCurrentUnitWP())
                            {
                                if ((pf->GetPilotCount() < pf->GetACCount()) or not pf->GetACCount())
                                {
                                    gCompressTillTime = 0;
                                    UI_HandleFlightScrub();
                                    return;
                                }
                            }

                            gMainHandler->SetDrawFlag(0); // Hack to keep the UI from drawing
                            CleanupTacticalEngagementUI();
                            PostMessage(gMainHandler->GetAppWnd(), FM_START_TACTICAL, 0, 0);
                            ResumeTacticalEngagement();
                            break;

                        case game_Campaign:

                            // Check if player is only one in the flight... if so... abort (assuming there is supposed to be
                            // more than 1 pilot... also only do this check if flight is taking off
                            if (pf->GetFirstUnitWP() == pf->GetCurrentUnitWP())
                            {
                                if ((pf->GetPilotCount() < pf->GetACCount()) or not pf->GetACCount())
                                {
                                    gCompressTillTime = 0;
                                    UI_HandleFlightScrub();
                                    return;
                                }
                            }

                            gMainHandler->SetDrawFlag(0); // Hack to keep the UI from drawing
                            CleanupCampaignUI();
                            PostMessage(gMainHandler->GetAppWnd(), FM_START_CAMPAIGN, 0, 0);
                            break;
                    }
                }
            }
        }
        else
        {
            int
            diff;

            diff = gCompressTillTime - vuxGameTime;

            diff = diff / 2500; // two 1/2 minutes away - start reducing compression ratio

            if (diff > 64) // Are we a few minutes away
            {
                diff = 64;
            }
            else if (diff <= 0) // Don't just stop however
            {
                diff = 1;
            }

            SetTimeCompression(diff);
        }
    }
}

void SetCampaignStartupMode(void)
{
    sCampaignStartingUp = CAMP_STARTING_UP;
    ThreadManager::fast_campaign();
}

int gCampTime = 0;
int gAveCampTime = 0;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// sfr: removing locks here seem to be problematic, I wonder why
void UpdatePlayerSessions()
{
#define NEW_UPDATE_PLAYER_SESSION 1
#if NO_CAMP_LOCK and NEW_UPDATE_PLAYER_SESSION
#else
    CampEnterCriticalSection();
#endif
#if NO_VU_LOCK and NEW_UPDATE_PLAYER_SESSION
#else
    VuEnterCriticalSection();
#endif

    VuSessionsIterator sit(FalconLocalGame);
    FalconSessionEntity *session, *nextSession;

    for (
        session = (FalconSessionEntity*)sit.GetFirst();
        session not_eq NULL;
        session = nextSession
    )
    {
        nextSession = static_cast<FalconSessionEntity*>(sit.GetNext());
        session->UpdatePlayer();
    }

#if NO_VU_LOCK and NEW_UPDATE_PLAYER_SESSION
#else
    VuExitCriticalSection();
#endif
#if NO_CAMP_LOCK and NEW_UPDATE_PLAYER_SESSION
#else
    CampLeaveCriticalSection();
#endif
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// ==============================================================
//
// Main Campaign Loop
//
// ==============================================================

unsigned int __stdcall HandleCampaignThread(void)
{
    CampaignTime deltatime;
    int sleepTic;
    int startup = 0;

#if defined(_MSC_VER)
    // Set the FPU to Truncate
    _controlfp(_RC_CHOP, MCW_RC);

    // Set the FPU to 24 bit precision
    _controlfp(_PC_24, MCW_PC);
#endif

    TheCampaign.Flags or_eq CAMP_RUNNING;

    while (ThreadManager::campaign_active())
    {
        sleepTic = GetTickCount();

        //START_PROFILE("CA VU UPD");
        CampEnterCriticalSection();
#if CAP_DISPATCH
        // sfr: theres a duplicate call at the end. I think this one should be removed
        // 20 ms at most
        TheCampaign.vuThread->Update(20);
#else
        TheCampaign.vuThread->Update();
#endif
        CampLeaveCriticalSection();
        //STOP_PROFILE("CA VU UPD");

        if (TheCampaign.Flags bitand CAMP_SUSPEND_REQUEST)
        {
            // Someone's asked us to suspend
            TheCampaign.Flags or_eq CAMP_SUSPENDED;
            TheCampaign.Flags xor_eq CAMP_SUSPEND_REQUEST;
        }

        //START_PROFILE("CA SENDDATA");
        // Send any data that's been requested of us.
        SendRequestedData();
        //STOP_PROFILE("CA SENDDATA");

        // sfr: if we do this here, we risk processing it while
        // the UI is closing. This is wrong, passing to after the suspended stuff
#define NEW_UPDATE_HANDLE_CAMPAIGN 1
#if not NEW_UPDATE_HANDLE_CAMPAIGN
        UpdatePlayerSessions();
        // Send Dirty Campaign Objects
        FalconEntity::DoCampaignDirtyData(vuxRealTime);
#endif

        if ((TheCampaign.Flags bitand CAMP_SUSPENDED) or (TheCampaign.Flags bitand CAMP_TACTICAL_PAUSE))
        {
            // sfr: placed this one inside cs like the others
            CampEnterCriticalSection();
#if CAP_DISPATCH
            TheCampaign.vuThread->Update(-1);
#else
            TheCampaign.vuThread->Update();
#endif
            CampLeaveCriticalSection();
            gCampTime = GetTickCount() - sleepTic;
            gAveCampTime = (gAveCampTime * 7 + gCampTime) / 8;
            sleepTic = 100 * 100 / max(gAveCampTime, 10);
            ThreadManager::campaign_signal_sim();
#if NEW_SYNC
            ThreadManager::campaign_wait_for_sim(INFINITE);
#else
            ThreadManager::campaign_wait_for_sim(max(sleepTic, 300));
#endif
            continue;
        }

#if NEW_UPDATE_HANDLE_CAMPAIGN
        UpdatePlayerSessions();
        // Send Dirty Campaign Objects
        FalconEntity::DoCampaignDirtyData(vuxRealTime);
#endif

        // Do any time compression stuff (i.e: For takeoff) and launch the sim, if necessary
        DoCompressionLoop();

        ShiAssert(TheCampaign.CurrentTime <= vuxGameTime);

        // Update our time (1 minute max delta)
        deltatime = vuxGameTime - TheCampaign.CurrentTime;

        if (deltatime > CampaignMinutes)
        {
            // 1 minute maximum time delta
            deltatime = CampaignMinutes;

            if (gameCompressionRatio > 0 /*1*/)
            {
                SetTemporaryCompression(gameCompressionRatio / 2); // Slow things down
            }
        }
        else if (gameCompressionRatio not_eq targetCompressionRatio)
        {
            SetTemporaryCompression(targetCompressionRatio); // Back to full speed
        }

        TheCampaign.CurrentTime += deltatime;
        TheCampaign.TimeOfDay = TheCampaign.CurrentTime % CampaignDay;

        if (deltatime > 0 and (TheCampaign.Flags bitand CAMP_LOADED))
        {
            if (sCampaignStartingUp == CAMP_STARTING_UP)
            {
                startup = 1;
            }

            // Check for instant action's next wave time
            if (FalconLocalGame->GetGameType() == game_InstantAction)
            {
                instant_action::check_next_wave();
            }

            //START_PROFILE("CAMPLOOP");
            if ( not (TheCampaign.Flags bitand CAMP_LIGHT) and TheCampaign.IsMaster())
            {
                if (TheCampaign.Flags bitand CAMP_TACTICAL)
                {
                    DoTacticalLoop(startup);
                }
                else
                {
                    DoCampaignLoop(startup);
                }
            }

            //STOP_PROFILE("CAMPLOOP");

            // sfr: this is duplicated from above.
            // maybe we should leave only one
            // Process any events which we've collected, and do vu cleanup
            CampEnterCriticalSection();
#if CAP_DISPATCH
            // 20 ms at most
            TheCampaign.vuThread->Update(20);
#else
            TheCampaign.vuThread->Update();
#endif
            CampLeaveCriticalSection();

            //START_PROFILE("CA UPDATE ALL REAL");
            if (TheCampaign.IsMaster())
            {
                // Move our units
                UpdateRealUnits(deltatime);
            }

            //STOP_PROFILE("CA UPDATE ALL REAL");
        }

        if (startup)
        {
            ThreadManager::fast_campaign();
            sCampaignStartingUp = CAMP_NORMAL_MODE;
            startup = 0;
        }

        if ( not doUI)
        {
            gCampTime = GetTickCount() - sleepTic;
            gAveCampTime = (gAveCampTime * 7 + gCampTime) / 8;
            sleepTic = 100 * 100 / max(gAveCampTime, 10);
#if not NEW_SYNC
            ThreadManager::campaign_signal_sim();
            ThreadManager::campaign_wait_for_sim(max(sleepTic, 300));
#endif
        }
        else
        {
            // sfr: this thread needs sleep, otherwise it will suck CPU
            // I dont know what is the ideal time here though. This is just a test value
            Sleep(1);
        }

#if NEW_SYNC
        ThreadManager::campaign_signal_sim();
        ThreadManager::campaign_wait_for_sim(INFINITE);
#endif

    }

    TheCampaign.Flags xor_eq CAMP_RUNNING;
    return (0);
}

//
// Structured exception handler for campaign thread
//
unsigned int __stdcall CampaignThread(void)
{
    int Result = 0;

    __try
    {
        Result = HandleCampaignThread();
    }
    __except (RecordExceptionInfo(GetExceptionInformation(), "Campaign Thread"))
    {
        // Do nothing here - RecordExceptionInfo() has already done
        // everything that is needed. Actually this code won't even
        // get called unless you return EXCEPTION_EXECUTE_HANDLER from
        // the __except clause.
    }

    return Result;
}

// ==============================================================
//
// Tactical engagement main loop
//
// ==============================================================

void DoTacticalLoop(int startup)
{
    static int stage, lastStage;
    static CampaignTime lastCheck;
    Team t;

    // Calculate our current stage
    stage = (TheCampaign.CurrentTime % CampaignHours) / (CAMPAIGN_STAGE_TIME_MINUTES * CampaignMinutes);

    // Keep from skipping stages
    if (stage > lastStage + 1)
        stage = lastStage + 1;

    if (stage > STAGE_12)
        stage = STAGE_1;

    if (startup)
    {
        // Do all items
        CheckNewDay();
        UpdateTeamStatistics();
        RepairObjectives();
        StandardRebuild();
        lastStage = stage;
        lastCheck = 0;
    }

    // Victory check
    if ((TheCampaign.CurrentTime - lastCheck) > static_cast<CampaignTime>(VICTORY_CHECK_TIME * CampaignSeconds))
    {
        CheckForVictory();
        lastCheck = TheCampaign.CurrentTime;
    }

    // Do any stage actions on change
    if (stage not_eq lastStage)
    {
        switch (stage)
        {
            case STAGE_1:
                // Check for new day
                CheckNewDay();
                // calculate statistics
                UpdateTeamStatistics();
                break;

            case STAGE_10:
                // Repair objectives
                RepairObjectives();
                break;

            default:
                break;
        }

        // Rebuild lists
        StandardRebuild();

        RebuildATMLists();

        for (t = 0; t < NUM_TEAMS; t++)
        {
            // sfr: check vu state...
            if (TeamInfo[t] and TeamInfo[t]->VuState() == VU_MEM_ACTIVE)
            {
                TeamInfo[t]->atm->DoCalculations();
            }
        }

        // Rally units (2 times as fast as normal = 4 * 5 = 20 minutes)
        RallyUnits(REGAIN_RATE_MULTIPLIER_FOR_TE * CAMPAIGN_STAGE_TIME_MINUTES);

        lastStage = stage;
    }

    // Update weather when in UI
    if ( not SimDriver.InSim())
        ((WeatherClass*)realWeather)->UpdateWeather(); // Sim calls this otherwise
}

// ==============================================================
//
// Campaign main loop
//
// ==============================================================

void DoCampaignLoop(int startup)
{
    static int stage, lastStage, planCount;
    Team t;

    // Calculate our current stage
    stage = (TheCampaign.CurrentTime % CampaignHours) / (CAMPAIGN_STAGE_TIME_MINUTES * CampaignMinutes);

    // Keep from skipping stages
    if (stage > lastStage + 1)
        stage = lastStage + 1;

    if (stage > STAGE_12)
        stage = STAGE_1;

    if (startup)
    {
        // Do all items
        CheckNewDay();

        for (t = 0; t < NUM_TEAMS; t++)
            AddReinforcements(t, 1);

        UpdateTeamStatistics();
        planCount = 100;
        PlanGroundAndNavalUnits(&planCount);
        OrderGroundAndNavalUnits();
        StandardRebuild();
        CheckTriggers(TheCampaign.Scenario);
        RebuildATMLists();

        for (t = 0; t < NUM_TEAMS; t++)
            TeamInfo[t]->atm->DoCalculations();

        for (t = 1; t < NUM_TEAMS; t++)
            TeamInfo[t]->SelectAirActions();

        lastStage = stage;
    }

    // Do any stage actions on change
    if (stage not_eq lastStage)
    {
        switch (stage)
        {
            case STAGE_1:
                // Check for new day
                CheckNewDay();

                // Add new reinforcements
                for (t = 0; t < NUM_TEAMS; t++)
                    AddReinforcements(t, 1);

                // calculate statistics
                UpdateTeamStatistics();

                if (doUI)
                    SendMessage(FalconDisplay.appWin, FM_AUTOSAVE_CAMPAIGN, 0, game_Campaign);

                break;

            case STAGE_4:

                // Supply and repair units
                for (t = 0; t < NUM_TEAMS; t++)
                    SupplyUnits(t, CampaignHours);

                // Plan ground and naval units
                PlanGroundAndNavalUnits(&planCount);
                break;

            case STAGE_7:
                // Order ground and naval units
                OrderGroundAndNavalUnits();
                break;

            case STAGE_10:
                // Repair objectives
                RepairObjectives();
                // Produce supplies
                ProduceSupplies(CampaignHours);

                // Supply and repair units
                for (t = 0; t < NUM_TEAMS; t++)
                    SupplyUnits(t, CampaignHours);

                break;

            default:
                break;
        }

        // Rebuild lists
        StandardRebuild();

        // Check Triggers
        CheckTriggers(TheCampaign.Scenario);

        // Plan Air
        RebuildATMLists();

        for (t = 0; t < NUM_TEAMS; t++)
        {
            if (TeamInfo[t])
            {
                TeamInfo[t]->atm->DoCalculations();
            }
        }

        for (t = 1; t < NUM_TEAMS; t++)
        {
            if (TeamInfo[t])
            {
                TeamInfo[t]->SelectAirActions();
            }
        }

        // Order parent units
        UpdateParentUnits(CAMPAIGN_STAGE_TIME_MINUTES * CampaignMinutes);

        lastStage = stage;
    }

    // Check any pending divert missions
    CheckDivertStatus(DIVERT_NO_DIVERT);

    // Update weather when in UI
    if ( not SimDriver.InSim())
    {
        ((WeatherClass*)realWeather)->UpdateWeather(); // Sim calls this otherwise
    }

    // Task air
    for (t = 0; t < NUM_TEAMS; t++)
    {
        if (TeamInfo[t] and TeamInfo[t]->atm)
        {
            TeamInfo[t]->atm->Task();
        }
    }

#ifdef DEBUG
    CheckForCheatFlight(TheCampaign.CurrentTime);
#endif
}

// ==============================================================
//
// These are the workhorse functions
//
// ==============================================================

void UpdateParentUnits(CampaignTime deltatime)
{
    // Update parent units
    CampEnterCriticalSection();
    VuListIterator pit(AllParentList);
    Unit u, next;

    for (u = (Unit) pit.GetFirst(); u not_eq NULL; u = next)
    {
        next = (Unit) pit.GetNext();

        if (u->Father())
        {
            if (u->Inactive())
            {
                InactivateUnit(u);
            }
            else if (u->IsDead())
            {
                // wait a bit before removing
#define PARENT_DEATH_TIMEOUT_MS 240000/*7200000*/
                if (TheCampaign.CurrentTime - u->GetLastCheck() > PARENT_DEATH_TIMEOUT_MS)
                {
                    vuDatabase->Remove(u);
                }

#undef PARENT_DEATH_TIMEOUT_MS
            }
            else
            {
                UpdateUnit(u, deltatime);
            }
        }
        else
        {
            printf("does this happen?");
        }
    }

    CampLeaveCriticalSection();
}

void UpdateRealUnits(CampaignTime deltatime)
{
    // Update all real units
    //START_PROFILE("CA UPD REAL LOCK");
    CampEnterCriticalSection();
    //STOP_PROFILE("CA UPD REAL LOCK");

    VuListIterator rit(AllRealList);

    for (UnitClass *u = (UnitClass*)rit.GetFirst(), *next = NULL; u not_eq NULL; u = next)
    {
        next = static_cast<UnitClass*>(rit.GetNext());

        if (u and u->Inactive())
        {
            //START_PROFILE("CA UPD REAL INACTIVATE");
            InactivateUnit(u);
            //STOP_PROFILE("CA UPD REAL INACTIVATE");
        }
        else if (u and u->IsDead())
        {
            //START_PROFILE("CA UPD REAL DEAD");
            // wait a bit before removing
            const unsigned int REAL_DEATH_TIMEOUT_MS = 240000/*7200000*/;

            if (TheCampaign.CurrentTime - u->GetLastCheck() > REAL_DEATH_TIMEOUT_MS)
            {
                vuDatabase->Remove(u);
            }

            //STOP_PROFILE("CA UPD REAL DEAD");
        }
        else
        {
            //START_PROFILE("CA UPD REAL UNIT");
            if (u)
                UpdateUnit(u, deltatime);

            //STOP_PROFILE("CA UPD REAL UNIT");
        }
    }

    CampLeaveCriticalSection();
}

void CheckNewDay(void)
{
    if (TheCampaign.CurrentTime > static_cast<CampaignTime>(CampaignDay * (TheCampaign.CurrentDay + 1)))
    {
        MonoPrint("Entering Campaign Day: %d\n", TheCampaign.CurrentDay);

        // 2002-04-02 MNLOOK does this make problems in MP ?
        // Now that we've a floating bullseye, perhaps it should be put back in
        // Also, what happens to SMS if Bullseye changes while in-flight ?
        if (g_nChooseBullseyeFix bitand 0x02)
        {
            ChooseBullseye();
        }

        TheCampaign.GetCurrentDay();
    }
}

void PlanGroundAndNavalUnits(int *planCount)
{
    Team t;

    (*planCount)++;

    if (*planCount >= (MIN_PLAN_GROUND / 60))
    {
        for (t = 0; t < NUM_TEAMS; t++)
        {
            if (TeamInfo[t])
            {
                TeamInfo[t]->gtm->DoCalculations();
                TeamInfo[t]->ntm->DoCalculations();
            }
        }

        *planCount = 0;
    }

    for (t = 0; t < NUM_TEAMS; t++)
    {
        if (TeamInfo[t])
            TeamInfo[t]->SelectGroundAction();
    }
}

void OrderGroundAndNavalUnits(void)
{
    Team t;

    ResetObjectiveAssignmentScores();
    BuildDivisionData();

    for (t = 0; t < NUM_TEAMS; t++)
    {
        if (TeamInfo[t])
        {
            TeamInfo[t]->gtm->Task();
            TeamInfo[t]->ntm->Task();
        }
    }
}

// Rally units (Tactical engagement only) VP_chnges it should be checked
void RallyUnits(int minutes)
{
    Unit u;
    VuListIterator myit(AllParentList);

    // Create the unit lists
    u = GetFirstUnit(&myit);

    while (u)
    {
        if (u->GetDomain() == DOMAIN_LAND)
        {
            if ( not u->Scripted() and not u->Engaged() and u->GetUnitOrders() == GORD_RESERVE)
                u->RallyUnit(minutes);

            u->UpdateParentStatistics();
        }

        u = GetNextUnit(&myit);
    }
}

// ==============================================================
//
// Flag setting crapola
//
// ==============================================================

void MakeTacticalEdit(void)
{
    TheCampaign.Flags or_eq CAMP_TACTICAL_EDIT;
}

void RemoveTacticalEdit(void)
{
    TheCampaign.Flags and_eq compl CAMP_TACTICAL_EDIT;
}

void PauseTacticalEngagement(void)
{
    TheCampaign.Flags or_eq CAMP_TACTICAL_PAUSE;
}

void ResumeTacticalEngagement(void)
{
    TheCampaign.Flags and_eq compl CAMP_TACTICAL_PAUSE;
}

void OutputRef(VuEntity *ent, int refs)
{
    MonoPrint("Refer %08x %d %d\n", ent, ent->Type(), refs + 1);
}

void OutputDeref(VuEntity *ent, int refs)
{
    MonoPrint("DeRef %08x %d %d\n", ent, ent->Type(), refs - 1);
}





