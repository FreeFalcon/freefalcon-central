#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <tchar.h>

#include "MissEval.h"
#include "Debuggr.h"
#include "CampStr.h"
#include "CampLib.h"
#include "Find.h"
#include "Flight.h"
#include "Name.h"
#include "Package.h"
#include "feature.h"
#include "Team.h"
#include "OwnResult.h"
#include "EvtParse.h"
#include "Mesg.h"
#include "MsgInc/DamageMsg.h"
#include "MsgInc/WeaponFireMsg.h"
#include "MsgInc/PlayerStatusMsg.h"
#include "MsgInc/DeathMessage.h"
#include "MsgInc/EjectMsg.h"
#include "MsgInc/LandingMessage.h"
#include "MsgInc/SendEvalMsg.h"
#include "MsgInc/SendChatMessage.h"
#include "falcsess.h"
#include "Campaign.h"
#include "AIInput.h"
#include "Pilot.h"
#include "CampMap.h"
#include "CmpClass.h"
#include "uicomms.h"
#include "classtbl.h"
#include "SimMover.h"
#include "logbook.h"
#include "brief.h"
#include "F4Version.h"
#include "otwdrive.h"
#include "ui/include/tac_class.h"
#include "ui/include/te_defs.h"
extern bool g_bLogEvents;
#ifdef USE_SH_POOLS
MEM_POOL EventElement::pool;
#endif

#ifdef DEBUG
//#define FUNKY_KEVIN_DEBUG_STUFF 1
#endif

extern short NumWeaponTypes;
extern short NumRocketTypes;
extern short gRocketId;

#ifdef DEBUG
int inMission = 0;
#endif

extern bool g_bNoAAAEventRecords;

// ======================================
// Various defines
// ======================================

#define STATION_TIME_LENIENCY 3*CampaignMinutes // In minutes
#define STATION_DIST_HITS_LENIENCY 35 // In km // JB 010215
//TJL 11/19/03 Now bumped to 50 miles, 93 KM from 60,
#define STATION_DIST_LENIENCY 93 // In km // JB 010215 from 35 to 60
#define TARGET_DIST_LENIENCY 5 // In km
#define RELATED_EVENT_RANGE_SQ 900 // In km squared

// ======================================
// Event types we have format strings for
// ======================================

enum MISS_EVAL_EVENT_TYPE
{
    FET_PILOT_DOWNED_BY_PACKMATE,
    FET_PILOT_DOWNED_BY_VEHICLE,
    FET_PILOT_KILLED_PACKMATE,
    FET_PILOT_KILLED_VEHICLE,
    FET_PILOT_DESTROYED_VEHICLE,
    FET_COLLIDED_WITH_PILOT,
    FET_COLLIDED_WITH_VEHICLE,
    FET_COLLIDED_WITH_FEATURE,
    FET_PILOT_CRASHED,
    FET_PILOT_KILLED_BY_DEBREE,
    FET_PILOT_KILLED_BY_BOMB,
    FET_PILOT_KILLED_BY_OTHER,
    FET_PILOT_EJECTED,
    FET_PILOT_JOINED,
    FET_PILOT_EXITED,
    FET_PILOT_LANDED,
    FET_FIRED_MISSED,
    FET_RELEASED_MISSED,
    FET_DAMAGED,
    FET_DESTROYED,
    FET_PHOTO_TAKEN_MISSED,
    FET_PHOTO_TAKEN_HIT,
    FET_LAST_EVENT_TYPE
};

// ===================================
// Format Table and access function
// ===================================

int FormatTable[FET_LAST_EVENT_TYPE][5] =
{
    { 0, 1754, 1754, 1704, 1704 }, // FET_PILOT_DOWNED_BY_PACKMATE
    { 0, 1753, 1753, 1703, 1703 }, // FET_PILOT_DOWNED_BY_VEHICLE
    { 0, 1755, 1755, 1705, 1705 }, // FET_PILOT_KILLED_PACKMATE
    { 0, 1750, 1750, 1700, 1700 }, // FET_PILOT_KILLED_VEHICLE
    { 0, 1751, 1751, 1701, 1701 }, // FET_PILOT_DESTROYED_VEHICLE
    { 0, 1757, 1757, 1707, 1707 }, // FET_COLLIDED_WITH_PACKMATE
    { 0, 1758, 1758, 1708, 1708 }, // FET_COLLIDED_WITH_VEHICLE
    { 0, 1759, 1759, 1709, 1709 }, // FET_COLLIDED_WITH_FEATURE
    { 0, 1752, 1752, 1702, 1702 }, // FET_PILOT_CRASHED
    { 0, 1760, 1760, 1710, 1710 }, // FET_PILOT_KILLED_BY_DEBREE
    { 0, 1761, 1761, 1711, 1711 }, // FET_PILOT_KILLED_BY_BOMB
    { 0, 1762, 1762, 1712, 1712 }, // FET_PILOT_KILLED_OTHER
    { 0, 1765, 1765, 1715, 1715 }, // FET_PILOT_EJECTED
    { 0, 1763, 1763, 1713, 1713 }, // FET_PILOT_JOINED
    { 0, 1764, 1764, 1714, 1714 }, // FET_PILOT_EXITED
    { 0, 1766, 1766, 1716, 1716 }, // FET_PILOT_LANDED
    { 0, 1770, 1770, 1720, 1720 }, // FET_FIRED_MISSED
    { 0, 1771, 1771, 1721, 1721 }, // FET_RELEASED_MISSED
    { 0, 1772, 1772, 1722, 1722 }, // FET_DAMAGED
    { 0, 1773, 1773, 1723, 1723 }, // FET_DESTROYED
    { 0, 1724, 1724, 1724, 1724 }, // FET_PHOTO_TAKEN_MISSED
    { 0, 1725, 1725, 1725, 1725 }, // FET_PHOTO_TAKEN_HIT
};

void GetFormatString(int eventType, _TCHAR *format)
{
    ReadIndexedString(FormatTable[eventType][FalconLocalGame->GetGameType()], format, 79);
}

// =================================
// A few globals
// =================================

// 2002-02-16 MN added another 0 for AWACSAbort
short ScoreAdjustment[6] = { -10, -5, 6, 12, 0, 0}; // Scores for failure - Success

// =================================
// Prototypes
// =================================

enum score_type
{
    SCORE_FIRE_WEAPON,
    SCORE_PILOT_LOST,
    SCORE_PILOT_FOUND,
    SCORE_PILOT_EJECTED,
    SCORE_PILOT_EJECTED_KILL,
    SCORE_PILOT_LANDED,
    SCORE_HIT_ENEMY,
    SCORE_HIT_FRIENDLY,
    SCORE_KILL_ENEMY,
    SCORE_KILL_FRIENDLY,
    SCORE_KILL_ENEMY_FEATURE,
    SCORE_KILL_FRIENDLY_FEATURE,
    SCORE_KILL_ENEMY_NAVAL,
    SCORE_KILL_FRIENDLY_NAVAL,
    SCORE_KILL_ENEMY_GROUND,
    SCORE_KILL_FRIENDLY_GROUND,
    SCORE_DIRECT_KILL_GROUND,
    SCORE_HIT_OUR_TARGET,
    SCORE_GROUND_COLLISION,
    SCORE_GROUND_COLLISION_KILL,
    SCORE_FEATURE_COLLISION,
    SCORE_VEHICLE_COLLISION,
    SCORE_DEBREE_KILL,
    SCORE_BOMB_KILL,
    SCORE_OTHER_KILL,
    SCORE_VEHICLE_KILL,
};

Objective FindAlternateLandingStrip(Flight flight);
static int CalcScore(score_type type, int index);
extern void RatePilot(Flight flight, int pilotSlot, int newRating);
extern long gRefreshScoresList;

int AddWeaponToUsageList(int index);
int AddAircraftToKillsList(int index);
int AddObjectToKillsList(int index);
int score_player_ejected(void);
void UpdateEvaluators(FlightDataClass *flight_data, PilotDataClass *pilot_data);
class C_Handler;
extern C_Handler *gMainHandler;
extern void AddMessageToChatWindow(VU_ID from, _TCHAR *message);

#ifdef DEBUG
extern int testDebrief;
#endif

#ifdef DEBUG
int gPlayerPilotLock = 0;
extern int doUI;
#endif

extern int ConvertTeamToStringIndex(int team, int gender = 0, int usage = 0, int plural = 0);

// =================================
// Data class constructors
// =================================

WeaponDataClass::WeaponDataClass(void)
{
    starting_load = 0;
    fired = 0;
    missed = 0;
    hit = 0;
    events = 0;
    root_event = NULL;
    weapon_id = 0;
}

WeaponDataClass::~WeaponDataClass()
{
    DisposeEventList(root_event);
}

PilotDataClass::PilotDataClass(void)
{
    aircraft_slot = 0;
    pilot_slot = 0;
    pilot_id = 0;
    pilot_flags = 0;
    pilot_status = 0;
    aircraft_status = 0;
    aa_kills = ag_kills = as_kills = an_kills = 0;
    donefiledebrief = FALSE;
    player_kills = 0;
    // memset(kills,0,MAX_DOGFIGHT_TEAMS*VS_EITHER*sizeof(short));
    memset(deaths, 0, VS_EITHER * sizeof(short));
    shot_at = 0;
    score = 0;
    rating = 0;
    weapon_types = 0;
    next_pilot = NULL;
    pilot_callsign[0] = 0;
    pilot_name[0] = 0;
}

PilotDataClass::~PilotDataClass()
{
    // ShiAssert (FalconLocalSession->GetFlyState() == FLYSTATE_IN_UI); //NOTE Hit while discarding mission
}

FlightDataClass::FlightDataClass(void)
{
    camp_id = 0;
    flight_id = target_id = FalconNullId;
    requester_id = FalconNullId;
    start_aircraft = finish_aircraft = 0;
    flight_team = 0;
    mission = 0;
    target_camp_id = 0;
    target_building = 0;
    target_x = target_y = 0;
    status_flags = 0;
    mission_success = 0;
    mission_context = 0;
    failure_code = 0;
    failure_data = 0;
    num_pilots = 0;
    pilot_list = NULL;
    events = 0;
    root_event = NULL;
    next_flight = NULL;
    memset(target_features, 0, MAX_TARGET_FEATURES);
    target_status = 0;
    strcpy(context_entity_name, "");
}

FlightDataClass::~FlightDataClass()
{
    CampEnterCriticalSection();

    PilotDataClass *pilot_data = pilot_list;
    PilotDataClass *next;

    while (pilot_data)
    {
        next = pilot_data->next_pilot;
        delete pilot_data;
        pilot_data = next;
    }

    pilot_list = NULL;
    DisposeEventList(root_event);

    CampLeaveCriticalSection();
}

// =================================
// Mission Evaluation functions
// =================================

MissionEvaluationClass::MissionEvaluationClass(void)
{
#ifdef FUNKY_KEVIN_DEBUG_STUFF
    ShiAssert( not inMission);
#endif

    memset(this, 0, sizeof(MissionEvaluationClass));
    player_pilot = NULL;
    package_element = NULL;
    player_element = NULL;
    flags = 0;
    memset(rounds_won, 0, MAX_DOGFIGHT_TEAMS * sizeof(uchar));

    for (int i = 0; i < MAX_RELATED_EVENTS; i++)
        related_events[i] = NULL;
}

MissionEvaluationClass::~MissionEvaluationClass(void)
{
    CleanupFlightData();
}

void MissionEvaluationClass::CleanupFlightData(void)
{
    FlightDataClass *flight_ptr = flight_data, *next_ptr;

    while (flight_ptr)
    {
        next_ptr = flight_ptr->next_flight;
        delete flight_ptr;
        flight_ptr = next_ptr;
    }

    flight_data = NULL;
}

void MissionEvaluationClass::CleanupPilotData(void)
{
    int k;
    PilotDataClass *pilot_data, *soon_to_die;
    FlightDataClass *flight_ptr = flight_data;

    while (flight_ptr)
    {
        pilot_data = flight_ptr->pilot_list;

        while (pilot_data)
        {
            soon_to_die = pilot_data;

            for (k = 0; k < (HARDPOINT_MAX / 2) + 2; k++)
            {
                DisposeEventList(pilot_data->weapon_data[k].root_event);
                pilot_data->weapon_data[k].root_event = NULL;
                pilot_data->weapon_data[k].events = 0;
            }

            pilot_data = pilot_data->next_pilot;
            delete soon_to_die;
        }

        flight_ptr->pilot_list = NULL;
        flight_ptr = flight_ptr->next_flight;
    }
}

extern int doUI;

void MissionEvaluationClass::PreDogfightEval(void)
{
    // Called only upon entering/resetting a dogfight game
    CampEnterCriticalSection();

    CleanupFlightData();
    ClearPackageData();

    for (int i = 0; i < MAX_RELATED_EVENTS; i++)
    {
        if (related_events[i])
            delete related_events[i];

        related_events[i] = NULL;
    }

    last_related_event = 0;

    contact_score = 0;
    pack_success = 0;
    package_element = NULL;
    player_element = NULL;
    player_aircraft_slot = 255;
    friendly_losses = 0;
    actual_tot = 0;
    patrol_time = 0;
    package_mission = AMIS_SWEEP;
    package_context = 0;
    responses = 0;
    requesting_ent = FalconNullId;
    intercepting_ent = FalconNullId;
    awacs_id = FalconNullId;
    jstar_id = FalconNullId;
    ecm_id = FalconNullId;
    tanker_id = FalconNullId;
    action_type = 0;
    flags = 0;
    team = 0;
    alternate_strip_id = FalconNullId;
    abx = aby = -1;
    player_start_time = vuxGameTime;
    player_pilot = NULL;
    curr_data = 0;
    ClearPotentialTargets();
    // Analyse all flights
    {
        VuListIterator flit(AllAirList);
        Unit uelement;
        uelement = (Unit) flit.GetFirst();

        while (uelement)
        {
            if (uelement->IsFlight())
            {
                PreEvalFlight((Flight)uelement, NULL);
            }

            uelement->SetInPackage(1);
            uelement = (Unit) flit.GetNext();
        }
    }

    memset(rounds_won, 0, MAX_DOGFIGHT_TEAMS * sizeof(uchar));

    friendly_losses = friendly_aa_losses = friendly_ga_losses = 0;
    logbook_data.KilledByHuman = 0;
    logbook_data.KilledBySelf = 0;
    logbook_data.FriendlyFireKills = 0;
    logbook_data.Flags = 0;

    CampLeaveCriticalSection();
}

int MissionEvaluationClass::PreMissionEval(Flight flight, uchar aircraft_slot)
{
    Package package;
    Flight element;
    Objective aas;

#ifdef DEBUG

    if (testDebrief)
        return PostMissionEval();

#endif

#ifdef FUNKY_KEVIN_DEBUG_STUFF
    ShiAssert( not inMission);
#endif

#ifdef DEBUG

    while (inMission and ( not g_bLogEvents))
    {
        Sleep(100);
    }

#endif

    ShiAssert(doUI or (g_bLogEvents));

    CampEnterCriticalSection();

    ClearPackageData();
    CleanupFlightData();

    for (int i = 0; i < MAX_RELATED_EVENTS; i++)
    {
        if (related_events[i])
            delete related_events[i];

        related_events[i] = NULL;
    }

    last_related_event = 0;

    contact_score = 0;
    package = (Package)flight->GetUnitParent();
    pack_success = 0;
    package_element = NULL;
    player_element = NULL;
    player_aircraft_slot = 255;
    friendly_losses = 0;
    assigned_tot = flight->GetUnitTOT();
    actual_tot = 0;
    patrol_time = 0;

    if (package)
    {
        package->FindSupportFlights(package->GetMissionRequest(), 0);
        package_mission = package->GetMissionRequest()->mission;
        package_context = package->GetMissionRequest()->context;
        responses = package->GetResponses();
        requesting_ent = package->GetMissionRequest()->requesterID;
        intercepting_ent = package->GetInterceptor();
        awacs_id = package->GetAwacs(); // FindAwacs(flight, &awx, &awy);
        jstar_id = package->GetJStar(); // FindJStar(flight, &jsx, &jsy);
        ecm_id = package->GetECM();
        tanker_id = package->GetTanker(); // FindTanker(flight, &tankx, &tanky);
        package->GetUnitDestination(&tx, &ty);
        action_type = package->GetMissionRequest()->action_type;
    }
    else
    {
        package_mission = AMIS_SWEEP;
        package_context = 0;
        responses = 0;
        requesting_ent = FalconNullId;
        intercepting_ent = FalconNullId;
        awacs_id = FalconNullId;
        jstar_id = FalconNullId;
        ecm_id = FalconNullId;
        tanker_id = FalconNullId;
        flight->GetLocation(&tx, &ty);
        action_type = 0;
    }

    flags = 0;
    team = flight->GetTeam();
    aas = FindAlternateLandingStrip(flight);

    if (aas)
    {
        alternate_strip_id = aas->Id();
        aas->GetLocation(&abx, &aby);
    }
    else
    {
        alternate_strip_id = FalconNullId;
        abx = aby = -1;
    }

    player_start_time = vuxGameTime;
    player_pilot = NULL;
    curr_data = 0;
    FindPotentialTargets();

    if (package)
    {
        // Analyse child flights
        element = (Flight) package->GetFirstUnitElement();

        while (element)
        {
            PreEvalFlight(element, flight);
            element = (Flight) package->GetNextUnitElement();
        }
    }
    else
    {
        // Analyse all flights
        VuListIterator flit(AllAirList);
        Unit uelement;
        uelement = (Unit) flit.GetFirst();

        while (uelement)
        {
            if (uelement->IsFlight())
            {
                PreEvalFlight((Flight)uelement, flight);
            }

            uelement->SetInPackage(1);
            uelement = (Unit) flit.GetNext();
        }
    }

    friendly_losses = friendly_aa_losses = friendly_ga_losses = 0;

    if (player_pilot)
        player_pilot->shot_at = 0;

    logbook_data.KilledByHuman = 0;
    logbook_data.KilledBySelf = 0;
    logbook_data.FriendlyFireKills = 0;
    logbook_data.Flags = 0;

    if (aircraft_slot < 255)
        SetPackageData();

    ShiAssert(package_element);

    CampLeaveCriticalSection();

    return 0;
}

void MissionEvaluationClass::PreEvalFlight(Flight element, Flight flight)
{
    VehicleClassDataType *vc;
    WayPoint tw, w;
    FlightDataClass *flight_ptr, *tmp_ptr;
    CampEntity target = NULL;

    // ShiAssert (FalconLocalSession->GetFlyState() == FLYSTATE_IN_UI); - not in dogfight at least

    if (element)
    {
        flight_ptr = new FlightDataClass();

        if (flight and element->Id() == flight->Id())
            player_element = flight_ptr;

        vc = GetVehicleClassData(element->GetVehicleID(0));

        if (vc) // JB 010113
            _stprintf(flight_ptr->aircraft_name, vc->Name);
        else
            _stprintf(flight_ptr->aircraft_name, "");

        _stprintf(flight_ptr->name, flight_ptr->aircraft_name);
        GetCallsign(element->callsign_id, element->callsign_num, flight_ptr->name);
        flight_ptr->camp_id = element->GetCampID();
        flight_ptr->flight_id = element->Id();
        flight_ptr->start_aircraft = element->GetTotalVehicles();
        flight_ptr->finish_aircraft = flight_ptr->start_aircraft;

        if (element->GetEvalFlags() bitand FEVAL_MISSION_STARTED)
            flight_ptr->status_flags = MISEVAL_FLIGHT_STARTED_LATE;
        else
            flight_ptr->status_flags = 0;

        flight_ptr->old_mission = element->GetOriginalMission();
        flight_ptr->mission = element->GetUnitMission();
        flight_ptr->mission_context = element->GetMissionContext();
        flight_ptr->requester_id = element->GetRequesterID();
        flight_ptr->flight_team = element->GetTeam();
        flight_ptr->target_x = tx;
        flight_ptr->target_y = ty;
        tw = element->GetFirstUnitWP();

        while (tw and not (tw->GetWPFlags() bitand WPF_TARGET))
            tw = tw->GetNextWP();

        if ( not tw or not tw->GetWPTarget())
            tw = element->GetOverrideWP();

        if (tw)
        {
            flight_ptr->target_id = tw->GetWPTargetID();
            flight_ptr->target_building = tw->GetWPTargetBuilding();
            target = (CampEntity) FindEntity(flight_ptr->target_id);
        }
        else
        {
            flight_ptr->target_id = FalconNullId;
            flight_ptr->target_building = 255;
        }

        if (flight_ptr->mission == AMIS_INTERCEPT or flight_ptr->mission == AMIS_CAS)
        {
            // Assign target to immediate target if it's intercepting/cas
            if (flight)
            {
                FalconEntity* tmp_target = flight->GetTarget();

                if (tmp_target and tmp_target->IsCampaign())
                    target = (CampEntity) tmp_target;
            }
        }

        if (target)
        {
            if (target->IsUnit())
            {
                Unit target_parent = ((Unit)target)->GetUnitParent();

                if (target_parent)
                {
                    target = target_parent;
                    flight_ptr->target_id = target_parent->Id();
                }
            }

            flight_ptr->target_camp_id = target->GetCampID();
        }

        if ( not package_element)
        {
            package_element = flight_ptr;
            // package_mission = element->GetUnitMission();
            package_target_id = flight_ptr->target_id;
            RecordTargetStatus(flight_ptr, target);
            // Traverse our waypoints and collect data
            tw = 0;
            w = element->GetFirstUnitWP();

            while (w)
            {
                CollectThreats(element, w);

                if (w->GetWPFlags() bitand WPF_REPEAT)
                    tw = w;

                w = w->GetNextWP();
            }

            if (tw)
            {
                WayPoint pw = tw->GetPrevWP();
                patrol_time = tw->GetWPDepartureTime() - pw->GetWPArrivalTime();
            }
        }

        flight_ptr->failure_code = 0;
        flight_ptr->failure_data = 0;
        flight_ptr->failure_id = FalconNullId;
        flight_ptr->mission_success = 0;
        flight_ptr->num_pilots = 0;
        // Add to end of list.
        tmp_ptr = flight_data;

        while (tmp_ptr and tmp_ptr->next_flight)
            tmp_ptr = tmp_ptr->next_flight;

        if (tmp_ptr)
            tmp_ptr->next_flight = flight_ptr;
        else
            flight_data = flight_ptr;

        SetupPilots(flight_ptr, element);
    }
}

void MissionEvaluationClass::RecordTargetStatus(FlightDataClass *flight_ptr, CampBaseClass *target)
{
    if ( not target)
        return;

    memset(flight_ptr->target_features, 255, MAX_TARGET_FEATURES * sizeof(uchar));

    if (target and target->IsObjective())
    {
        // Find top targets
        Objective to = (Objective)target;
        ObjClassDataType* oc = to->GetObjectiveClassData();
        uchar targeted[128] = { 0 };
        int i, f, tf = 0;

        for (i = 0; i < 20 and tf < MAX_TARGET_FEATURES; i++)
        {
            f = BestTargetFeature(to, targeted);

            if (f < 128 and to->GetFeatureValue(f))
            {
                targeted[f] = 1;
                flight_ptr->target_features[tf] = f;
                tf++;
            }
        }

        // Record target status
        flight_ptr->target_status = to->GetObjectiveStatus();
    }
    else if (target and target->IsUnit())
    {
        target->GetName(flight_ptr->context_entity_name, 39, FALSE);
        flight_ptr->target_status = ((Unit)target)->GetTotalVehicles();
    }

    // Special case related unit data
    if (flight_ptr->mission_context == enemyUnitAdvanceBridge or flight_ptr->mission_context == enemyUnitMoveBridge or flight_ptr->mission_context == friendlyUnitAirborneMovement)
    {
        CampEntity relEnt = (CampEntity) vuDatabase->Find(flight_ptr->requester_id);

        if (relEnt)
            relEnt->GetName(flight_ptr->context_entity_name, 39, FALSE);
    }
}


void MissionEvaluationClass::ServerFileLog(FalconPlayerStatusMessage *fpsm)
{
    if ( not g_bLogEvents) return;

    char *filename;
    char logstring [250];
    filename = "debrief.txt";
    FILE *fp = fopen(filename, "a");
    ///////////////////////////////////////////////////////////
    PilotDataClass *pilot_data;
    FlightDataClass *flight_ptr;

    CampEnterCriticalSection();

    player_end_time = vuxGameTime;
    logbook_data.AircraftInPackage = 0;
    friendly_losses = 0;
    pack_success = MissionSuccess(package_element);

    flight_ptr = flight_data;

    while (flight_ptr)
    {
        if (flight_ptr->camp_id)
        {
            // This flight was active at the mission start
            flight_ptr->mission_success = MissionSuccess(flight_ptr);
            pilot_data = flight_ptr->pilot_list;
            flight_ptr->finish_aircraft = flight_ptr->start_aircraft;

            // find out if we are the only human in this flight
            int humansinflight = 0;
            int humansexitedflight = 0;
            int pilotsinflight = 0;

            while (pilot_data)
            {
                pilotsinflight++;

                if (pilot_data->pilot_flags bitand PFLAG_PLAYER_CONTROLLED)
                {
                    if (fpsm->dataBlock.pilotID == pilot_data->pilot_slot)
                        pilot_data->donefiledebrief = TRUE;

                    humansinflight++;

                    if (pilot_data->donefiledebrief)
                        humansexitedflight++;
                }

                pilot_data = pilot_data->next_pilot;
            }

            // goto next flight there's either no humans in it or theres still humans flying
            if ( not humansinflight or humansinflight > humansexitedflight)
            {
                flight_ptr = flight_ptr->next_flight;
                continue;
            }

            // this is the last human player in the flight...do the whole flight output.
            bool HasDoneFLightData = FALSE;
            pilot_data = flight_ptr->pilot_list;
            int pilotnumber = 0;

            while (pilot_data)
            {
                if (fpsm->dataBlock.campID and fpsm->dataBlock.campID == flight_ptr->camp_id and 
                    (fpsm->dataBlock.pilotID == pilot_data->pilot_slot or pilot_data->donefiledebrief))
                {
                    if ( not HasDoneFLightData)
                    {
                        HasDoneFLightData = TRUE;
                        sprintf(logstring, "\n\n\n--------------------------------------------------------\n");
                        fputs(logstring, fp);

                        FILETIME CurrentTime;
                        GetSystemTimeAsFileTime(&CurrentTime);
                        char Buffer[100];

                        WORD Date, Time;

                        if (FileTimeToLocalFileTime(&CurrentTime, &CurrentTime) and 
                            FileTimeToDosDateTime(&CurrentTime, &Date, &Time))
                        {
                            wsprintf(Buffer, "%d/%d/%d %02d:%02d:%02d",
                                     (Date / 32) bitand 15, Date bitand 31, (Date / 512) + 1980,
                                     (Time / 2048), (Time / 32) bitand 63, (Time bitand 31) * 2);
                        }

                        sprintf(logstring, "RECORD BEGIN TIMESTAMP %s.\n", Buffer);
                        fputs(logstring, fp);

                        if (vuLocalSessionEntity and FalconLocalGame)
                        {
                            char *gtype;

                            switch (FalconLocalGame->GetGameType())
                            {
                                case  game_InstantAction:
                                    gtype = "Instant Action";
                                    break;

                                case  game_Dogfight:
                                    gtype = "DogFight";
                                    break;

                                case game_TacticalEngagement:
                                    gtype = "Tactical Engagement";
                                    break;

                                case game_Campaign:
                                    gtype = "Campaign";
                                    break;

                                default:
                                    gtype = "<unknown>";
                                    break;
                            }

                            sprintf(logstring, "Game is %s type %s\r\n", gtype,
                                    gCommsMgr and gCommsMgr->Online() ? "Networked" : "Local");
                            fputs(logstring, fp);
                        }

                        if (current_tactical_mission)
                        {
                            sprintf(logstring, "Mission name: %s\n", current_tactical_mission->get_title());
                            fputs(logstring, fp);
                        }

                        _TCHAR Bufferb[40] = "";
                        AddIndexedStringToBuffer(300 + flight_ptr->mission, Bufferb);
                        sprintf(logstring, "\nMission Type: %s \n", Bufferb);
                        fputs(logstring, fp);

                        sprintf(logstring, "Flight Unique Id: %d\n", flight_ptr->name);
                        fputs(logstring, fp);

                        sprintf(logstring, "%d Ship Flight\n", pilotsinflight - humansinflight);
                        fputs(logstring, fp);

                        sprintf(logstring, "Ac type: %s \n", flight_data->aircraft_name);
                        fputs(logstring, fp);

                        _TCHAR tmp2[30];

                        if (gLangIDNum == F4LANG_GERMAN)
                            ReadIndexedString(ConvertTeamToStringIndex(fpsm->dataBlock.side, F4LANG_FEMININE), tmp2, 29);
                        else
                            ReadIndexedString(ConvertTeamToStringIndex(fpsm->dataBlock.side), tmp2, 29);

                        sprintf(logstring, "Contry: %s \n", tmp2);
                        fputs(logstring, fp);
                        ///flight events
                        sprintf(logstring, "\nFLIGHT EVENTS\n");
                        fputs(logstring, fp);

                        EventElement *event = flight_ptr->root_event;

                        for (int j = 0; j < flight_ptr->events; j++)
                        {
                            sprintf(logstring, "Event %s \n", event->eventString);
                            fputs(logstring, fp);
                            event = event->next;
                        }

                    }

                    /// player data
                    sprintf(logstring, "\n-------------\n");
                    fputs(logstring, fp);
                    sprintf(logstring, "PILOT SLOT %d:\n\n", pilot_data->aircraft_slot + 1);
                    fputs(logstring, fp);

                    if (pilot_data->pilot_flags bitand PFLAG_PLAYER_CONTROLLED)
                        sprintf(logstring, "Human Player: %s \n", pilot_data->pilot_name);

                    else sprintf(logstring, "AI Player: %s \n", pilot_data->pilot_name);

                    fputs(logstring, fp);

                    sprintf(logstring, "Callsign: %s \n", pilot_data->pilot_callsign);
                    fputs(logstring, fp);

                    char* status = NULL;

                    if (pilot_data->pilot_status == PILOT_AVAILABLE) status = " - OK";

                    if (pilot_data->pilot_status == PILOT_KIA) status = " - KIA";

                    if (pilot_data->pilot_status == PILOT_MIA) status = " - MIA";

                    if (pilot_data->pilot_status == PILOT_RESCUED) status = " - RESCUED";

                    if (pilot_data->pilot_status == PILOT_IN_USE) status = " - OK";

                    sprintf(logstring, "Pilot status %s \n", status);
                    fputs(logstring, fp);

                    if (pilot_data->aircraft_status == VIS_NORMAL) status = " - OK";

                    if (pilot_data->aircraft_status == VIS_REPAIRED) status = " - REPAIRED";

                    if (pilot_data->aircraft_status == VIS_DAMAGED) status = " - DAMAGED";

                    if (pilot_data->aircraft_status == VIS_DESTROYED) status = " - DESTROYED";

                    if (pilot_data->aircraft_status == VIS_LEFT_DEST) status = " - DAMAGED Left side ";

                    if (pilot_data->aircraft_status == VIS_RIGHT_DEST) status = " - DAMAGED Right side";

                    if (pilot_data->aircraft_status == VIS_BOTH_DEST) status = " - DAMAGED Both sides";

                    sprintf(logstring, "Aircraft status %s \n", status);
                    fputs(logstring, fp);

                    sprintf(logstring, "AA Kills %d \n", pilot_data->aa_kills);
                    fputs(logstring, fp);

                    sprintf(logstring, "AG Kills %d \n", pilot_data->ag_kills);
                    fputs(logstring, fp);

                    sprintf(logstring, "AS Kills %d \n", pilot_data->as_kills);
                    fputs(logstring, fp);

                    sprintf(logstring, "AN Kills %d \n", pilot_data->an_kills);
                    fputs(logstring, fp);

                    sprintf(logstring, "Shoot At %d \n", pilot_data->shot_at);
                    fputs(logstring, fp);

                    sprintf(logstring, "Other Player Kills %d \n", pilot_data->player_kills);
                    fputs(logstring, fp);

                    sprintf(logstring, "\nWEAPON DATA \n");
                    fputs(logstring, fp);

                    for (int j = 0; j < pilot_data->weapon_types; j++)
                    {
                        //weapon statistics
                        sprintf(logstring, "\nLOADOUT %d: %s \n", j, pilot_data->weapon_data[j].weapon_name);
                        fputs(logstring, fp);

                        sprintf(logstring, "Starting Load %d \n", pilot_data->weapon_data[j].starting_load);
                        fputs(logstring, fp);
                        sprintf(logstring, "Fired %d \n", pilot_data->weapon_data[j].fired);
                        fputs(logstring, fp);
                        sprintf(logstring, "Missed %d \n", pilot_data->weapon_data[j].missed);
                        fputs(logstring, fp);
                        sprintf(logstring, "Hit %d \n", pilot_data->weapon_data[j].hit);
                        fputs(logstring, fp);
                        // weapon events
                        EventElement *tmpevent = pilot_data->weapon_data[j].root_event;

                        for (int i = 0; i < pilot_data->weapon_data[j].events; i++)
                        {
                            sprintf(logstring, "Event %s \n", tmpevent->eventString);
                            fputs(logstring, fp);
                            tmpevent = tmpevent->next;
                        }

                    }
                }

                pilot_data = pilot_data->next_pilot;
            }
        }

        flight_ptr = flight_ptr->next_flight;
    }


    CampLeaveCriticalSection();


    //////////////////////////////////////////////////////////////
    sprintf(logstring, "\n");
    fputs(logstring, fp);




    fclose(fp);
}


int MissionEvaluationClass::PostMissionEval(void)
{
    int i;
    PilotDataClass *pilot_data;
    FlightDataClass *flight_ptr;

    CampEnterCriticalSection();
#ifdef DEBUG
    gPlayerPilotLock = 0;
#endif
    player_end_time = vuxGameTime;
    logbook_data.AircraftInPackage = 0;
    friendly_losses = 0;
    pack_success = MissionSuccess(package_element);

    flight_ptr = flight_data;

    while (flight_ptr)
    {
        if (flight_ptr->camp_id)
        {
            // This flight was active at the mission start
            flight_ptr->mission_success = MissionSuccess(flight_ptr);
            pilot_data = flight_ptr->pilot_list;
            flight_ptr->finish_aircraft = flight_ptr->start_aircraft;

            while (pilot_data)
            {
                if (FalconLocalGame->GetGameType() not_eq game_Dogfight)
                {
                    pilot_data->score += ScoreAdjustment[flight_ptr->mission_success];
                    pilot_data->score += ScoreAdjustment[pack_success] / 2;
                }

                switch (pilot_data->pilot_status)
                {
                    case PILOT_KIA:
                    case PILOT_MIA:
                        pilot_data->score += CalcScore(SCORE_PILOT_LOST, 0);
                        flight_ptr->finish_aircraft--;
                        break;

                    case PILOT_RESCUED:
                        pilot_data->score += CalcScore(SCORE_PILOT_FOUND, 0);
                        flight_ptr->finish_aircraft--;
                        break;

                    default:
                        break;
                }

                if (pilot_data->score > 16)
                    pilot_data->rating = Excellent;
                else if (pilot_data->score > 6)
                    pilot_data->rating = Good;
                else if (pilot_data->score > -6)
                    pilot_data->rating = Average;
                else if (pilot_data->score > -16)
                    pilot_data->rating = Poor;
                else
                    pilot_data->rating = Horrible;

                // KCK HACK: Per Gilman - cap success to PartSuccess if player didn't land
                if (pilot_data->pilot_slot >= PILOTS_PER_FLIGHT and not (logbook_data.Flags bitand LANDED_AIRCRAFT) and flight_ptr->mission_success == Success and ( not gCommsMgr or not gCommsMgr->Online()))
                    flight_ptr->mission_success = PartSuccess;

                // END HACK

                // Record the rating (for records sake)
                // KCK:We probably don't have the flight at this point though...
                // 2002-02-16 MN Only rate a pilot if we didn't have to abort mission
                if ( not (flight_ptr->mission_success == AWACSAbort))
                    RatePilot((Flight)vuDatabase->Find(flight_ptr->flight_id), pilot_data->aircraft_slot, pilot_data->rating);

                pilot_data = pilot_data->next_pilot;
            }

            friendly_losses += flight_ptr->start_aircraft - flight_ptr->finish_aircraft;
            logbook_data.AircraftInPackage += flight_ptr->start_aircraft;
        }

        // On Call CAS package succeed if any of the components succeeded
        if (package_mission == AMIS_ONCALLCAS and flight_ptr->mission not_eq AMIS_FAC and flight_ptr->mission_success > pack_success)
            pack_success = flight_ptr->mission_success;

        flight_ptr = flight_ptr->next_flight;
    }

    // WARNING: Weird place for this, but for now this will work
    if (player_aircraft_slot < 255)
    {
        Objective o;
        // KCK: Techinally, we should probably use the parent PO if there is one, but whatever...
        o = FindNearestObjective(POList, tx, ty, NULL);

        if (o and player_pilot)
            ApplyPlayerInput(team, o->Id(), player_pilot->score);
    }

    // Update logbook data
    ShiAssert(player_pilot);

    if (player_pilot)
    {
        logbook_data.WeaponsExpended = 0;

        for (i = 0; i < player_pilot->weapon_types; i++)
        {
            // Only count guns once - regardless of number of bursts
            if (player_pilot->weapon_data[i].weapon_id >= 0) // sanity check
            {
                if ( not (WeaponDataTable[player_pilot->weapon_data[i].weapon_id].Flags bitand WEAP_ONETENTH))
                    logbook_data.WeaponsExpended += player_pilot->weapon_data[i].fired;
                else if (player_pilot->weapon_data[i].fired)
                    logbook_data.WeaponsExpended++;
            }
        }

        logbook_data.ShotsAtPlayer = player_pilot->shot_at;
        logbook_data.Score = player_pilot->rating + 1;
        logbook_data.Kills = player_pilot->aa_kills;
        logbook_data.HumanKills = player_pilot->player_kills;

        // Check for player survival
        if (player_pilot->pilot_status == PILOT_KIA)
            logbook_data.Killed = 1;
        else
            logbook_data.Killed = 0;

        // KCK QUICK HACK: Fix very large flight hours problem. The real fix should be done later.
        if (player_end_time < player_start_time)
            player_end_time = player_start_time + CampaignMinutes;

        logbook_data.FlightHours = (float)((float)(player_end_time - player_start_time) / CampaignHours);
        logbook_data.GroundUnitsKilled = player_pilot->ag_kills;
        logbook_data.FeaturesDestroyed = player_pilot->as_kills;
        logbook_data.NavalUnitsKilled = player_pilot->an_kills;
        logbook_data.WingmenLost = friendly_losses;

        switch (FalconLocalGame->GetGameType())
        {
            case game_Dogfight:
            {
                int won = 0, vsHuman = 0;

                if (flags bitand MISEVAL_GAME_COMPLETED)
                {
                    if (player_pilot->pilot_flags bitand PFLAG_WON_GAME)
                        won = 1;
                    else
                        won = -1;
                }

                if (flags bitand MISEVAL_ONLINE_GAME)
                    vsHuman = 1;

                LogBook.SetAceFactor(FalconLocalSession->GetAceFactor());
                LogBook.UpdateDogfight(won, logbook_data.FlightHours, vsHuman, player_pilot->aa_kills, player_pilot->deaths[VS_AI] + player_pilot->deaths[VS_HUMAN], player_pilot->player_kills, player_pilot->deaths[VS_HUMAN]);
            }
            break;

            case game_Campaign:
                LogBook.SetAceFactor(FalconLocalSession->GetAceFactor());

                // 2002-02-13 MN added AWACSAbort condition for "don't score mission"
                if ( not logbook_data.Killed and (player_element->mission_success == Incomplete or player_element->mission_success == AWACSAbort))
                {
                    logbook_data.Flags or_eq DONT_SCORE_MISSION;
                    LogBook.UpdateCampaign(&logbook_data);
                }
                else
                {
                    LogBook.UpdateCampaign(&logbook_data);

                    if (FalconLocalSession->GetMissions())
                    {
                        int missions, rating;
                        missions = FalconLocalSession->GetMissions();
                        rating = (FalconLocalSession->GetRating() * missions + (player_pilot->rating * 25)) / (missions + 1);
                        FalconLocalSession->SetRating(rating);
                        FalconLocalSession->SetMissions(missions + 1);
                    }
                    else
                    {
                        FalconLocalSession->SetRating((player_pilot->rating * 25));
                        FalconLocalSession->SetMissions(1);
                    }
                }

                FalconLocalSession->SetKill(FalconSessionEntity::_AIR_KILLS_, FalconLocalSession->GetKill(FalconSessionEntity::_AIR_KILLS_) + logbook_data.Kills + logbook_data.HumanKills);
                FalconLocalSession->SetKill(FalconSessionEntity::_GROUND_KILLS_, FalconLocalSession->GetKill(FalconSessionEntity::_GROUND_KILLS_) + logbook_data.GroundUnitsKilled);
                FalconLocalSession->SetKill(FalconSessionEntity::_NAVAL_KILLS_, FalconLocalSession->GetKill(FalconSessionEntity::_NAVAL_KILLS_) + logbook_data.NavalUnitsKilled);
                FalconLocalSession->SetKill(FalconSessionEntity::_STATIC_KILLS_, FalconLocalSession->GetKill(FalconSessionEntity::_STATIC_KILLS_) + logbook_data.FeaturesDestroyed);
                break;

            case game_TacticalEngagement:

                // 2002-02-13 MN added AWACSAbort condition
                if (player_element == 0)
                {
                    int a = 0; // so I can set a breakpoint
                }

                if (player_element) // MLR 3/25/2004 - CTD fix, cause is unknown. //Cobra 10/31/04 TJL
                {
                    if (logbook_data.Killed or (player_element->mission_success not_eq Incomplete) and (player_element->mission_success not_eq AWACSAbort))
                    {
                        if (FalconLocalSession->GetMissions())
                        {
                            int missions, rating;
                            missions = FalconLocalSession->GetMissions();
                            rating = (FalconLocalSession->GetRating() * missions + (player_pilot->rating * 25)) / (missions + 1);
                            FalconLocalSession->SetRating(rating);
                            FalconLocalSession->SetMissions(missions + 1);
                        }
                        else
                        {
                            FalconLocalSession->SetRating((player_pilot->rating * 25));
                            FalconLocalSession->SetMissions(1);
                        }
                    }
                }

                LogBook.UpdateFlightHours(logbook_data.FlightHours);
                LogBook.SetAceFactor(FalconLocalSession->GetAceFactor());
                LogBook.SaveData();
                FalconLocalSession->SetKill(FalconSessionEntity::_AIR_KILLS_, FalconLocalSession->GetKill(FalconSessionEntity::_AIR_KILLS_) + logbook_data.Kills + logbook_data.HumanKills);
                FalconLocalSession->SetKill(FalconSessionEntity::_GROUND_KILLS_, FalconLocalSession->GetKill(FalconSessionEntity::_GROUND_KILLS_) + logbook_data.GroundUnitsKilled);
                FalconLocalSession->SetKill(FalconSessionEntity::_NAVAL_KILLS_, FalconLocalSession->GetKill(FalconSessionEntity::_NAVAL_KILLS_) + logbook_data.NavalUnitsKilled);
                FalconLocalSession->SetKill(FalconSessionEntity::_STATIC_KILLS_, FalconLocalSession->GetKill(FalconSessionEntity::_STATIC_KILLS_) + logbook_data.FeaturesDestroyed);
                break;

            case game_InstantAction:
                LogBook.UpdateFlightHours(logbook_data.FlightHours);
                LogBook.SaveData();
                break;

            default:
                break;
        }
    }

    ClearPackageData();
    flags = 0;
    CampLeaveCriticalSection();

    return 0;
}

int MissionEvaluationClass::MissionSuccess(FlightDataClass *flight_ptr)
{
    int retval = Failed, losses;

    if ( not flight_ptr)
        return retval;

    // 2002-02-13 MN Check if we got an Abort from AWACS and the target has not been engaged
    // If it has been engaged, mission failed
    if (flight_ptr->status_flags bitand MISEVAL_FLIGHT_ABORT_BY_AWACS)
        if ( not (flight_ptr->status_flags bitand MISEVAL_FLIGHT_TARGET_HIT))
        {
            flight_ptr->failure_code = 1;
            return AWACSAbort;
        }
        else
            // We hit the target, but got AWACS order to abort before..
        {
            int statloss = 0;
            CampEntity e = FindEntity(flight_ptr->target_id);

            if (e and e->IsObjective())
                statloss = flight_ptr->target_status - ((Objective)e)->GetObjectiveStatus();

            if (flight_ptr->status_flags bitand MISEVAL_FLIGHT_HIT_HIGH_VAL or statloss > 35)
            {
                flight_ptr->failure_code = 4;
            }
            else if (statloss > 10)
            {
                flight_ptr->failure_code = 3;
            }
            else
            {
                flight_ptr->failure_code = 2;
            }

            return retval;
        }

    // Check for in progress
    if ( not (flight_ptr->status_flags bitand MISEVAL_FLIGHT_GOT_TO_TARGET) and 
 not (flight_ptr->status_flags bitand MISEVAL_FLIGHT_STATION_OVER) and 
 not (flight_ptr->status_flags bitand MISEVAL_FLIGHT_OFF_STATION) and 
 not (flight_ptr->status_flags bitand MISEVAL_FLIGHT_DESTROYED) and 
 not (flight_ptr->status_flags bitand MISEVAL_FLIGHT_ABORTED))
    {
        flight_ptr->failure_code = 0;
        return Incomplete;
    }

    switch (flight_ptr->mission)
    {
        case AMIS_BARCAP:
        case AMIS_BARCAP2:
        case AMIS_TARCAP:
        case AMIS_RESCAP:
        case AMIS_AMBUSHCAP:

            // Determine if we stayed in the area or not
            if ( not (flight_ptr->status_flags bitand MISEVAL_FLIGHT_OFF_STATION))
            {
                // Determine if our vol period is over or not
                if (flight_ptr->status_flags bitand MISEVAL_FLIGHT_STATION_OVER)
                {
                    // We completed the voll - simply check for damage or not
                    if (flight_ptr->status_flags bitand MISEVAL_FLIGHT_AREA_HIT)
                    {
                        retval = PartFailed;
                        flight_ptr->failure_code = 40;
                    }
                    else
                    {
                        retval = Success;
                        flight_ptr->failure_code = 41;
                    }
                }
                // Check if we had permission to leave early or not
                else if (flight_ptr->status_flags bitand MISEVAL_FLIGHT_RELIEVED)
                {
                    if (flight_ptr->status_flags bitand MISEVAL_FLIGHT_AREA_HIT)
                    {
                        retval = PartFailed;
                        flight_ptr->failure_code = 40;
                    }
                    else if (flight_ptr->status_flags bitand MISEVAL_FLIGHT_GOT_AKILL)
                    {
                        retval = Success;
                        flight_ptr->failure_code = 42;
                    }
                    else
                    {
                        retval = PartSuccess;
                        flight_ptr->failure_code = 43;
                    }
                }
                else
                    flight_ptr->failure_code = 57;
            }
            else
                flight_ptr->failure_code = 58;

            break;

        case AMIS_HAVCAP:

            // Check if our target was killed
            if (flight_ptr->status_flags bitand MISEVAL_FLIGHT_F_TARGET_KILLED)
                flight_ptr->failure_code = 48;
            // Check if our target was hit
            else if (flight_ptr->status_flags bitand MISEVAL_FLIGHT_F_TARGET_HIT)
            {
                retval = PartFailed;
                flight_ptr->failure_code = 49;
            }
            // Check if our target aborted
            else if (flight_ptr->status_flags bitand MISEVAL_FLIGHT_F_TARGET_ABORTED)
            {
                retval = PartFailed;
                flight_ptr->failure_code = 50;
            }
            else
            {
                // Check if we completed our time
                if (flight_ptr->status_flags bitand MISEVAL_FLIGHT_STATION_OVER)
                {
                    retval = Success;
                    flight_ptr->failure_code = 46;
                }
                // Check if we were relieved
                else if (flight_ptr->status_flags bitand MISEVAL_FLIGHT_RELIEVED)
                {
                    retval = PartSuccess;
                    flight_ptr->failure_code = 47;
                }
                else
                {
                    retval = PartFailed;
                    flight_ptr->failure_code = 57;
                }
            }

            break;

        case AMIS_INTERCEPT:
            if (flight_ptr->status_flags bitand MISEVAL_FLIGHT_TARGET_KILLED)
            {
                retval = Success;
                flight_ptr->failure_code = 35;
            }
            else if (flight_ptr->status_flags bitand MISEVAL_FLIGHT_TARGET_ABORTED)
            {
                retval = PartSuccess;
                flight_ptr->failure_code = 36;
            }
            else if (flight_ptr->status_flags bitand MISEVAL_FLIGHT_TARGET_HIT)
            {
                retval = PartFailed;
                flight_ptr->failure_code = 37;
            }
            else
                flight_ptr->failure_code = 38;

            break;

        case AMIS_SWEEP:
            if (flight_ptr->status_flags bitand MISEVAL_FLIGHT_DESTROYED)
                flight_ptr->failure_code = 30;
            else if ((flight_ptr->status_flags bitand MISEVAL_FLIGHT_LOSSES) or not (flight_ptr->status_flags bitand MISEVAL_FLIGHT_GOT_TO_TARGET))
            {
                if (flight_ptr->status_flags bitand MISEVAL_FLIGHT_GOT_AKILL)
                {
                    retval = PartFailed;
                    flight_ptr->failure_code = 32;
                }
                else
                {
                    retval = Failed;
                    flight_ptr->failure_code = 33;
                }
            }
            else
            {
                if (flight_ptr->status_flags bitand MISEVAL_FLIGHT_GOT_AKILL)
                {
                    retval = Success;
                    flight_ptr->failure_code = 31;
                }
                else
                {
                    retval = PartSuccess;
                    flight_ptr->failure_code = 34;
                }
            }

            break;

        case AMIS_ESCORT:
        case AMIS_SEADESCORT:
            if (flight_ptr->mission == AMIS_ESCORT)
                losses = friendly_aa_losses;
            else
                losses = friendly_ga_losses;

            if (package_element->status_flags bitand MISEVAL_FLIGHT_GOT_TO_TARGET or
                package_element->status_flags bitand MISEVAL_FLIGHT_ABORT_BY_AWACS)
            {
                if (package_element->status_flags bitand MISEVAL_FLIGHT_HIT_BY_GROUND and losses)
                {
                    retval = PartSuccess;

                    if (losses == 1)
                        flight_ptr->failure_code = 25;
                    else
                        flight_ptr->failure_code = 26;
                }
                else
                {
                    retval = Success;
                    flight_ptr->failure_code = 27;
                }
            }
            else if (package_element->status_flags bitand MISEVAL_FLIGHT_ABORTED)
            {
                if (package_element->status_flags bitand MISEVAL_FLIGHT_HIT_BY_GROUND and losses)
                {
                    retval = PartFailed;

                    if (losses == 1)
                        flight_ptr->failure_code = 25;
                    else
                        flight_ptr->failure_code = 26;
                }
                else
                {
                    retval = PartSuccess;
                    flight_ptr->failure_code = 28;
                }
            }
            else
            {
                retval = Failed;

                if (package_element->status_flags bitand MISEVAL_FLIGHT_STATION_OVER or package_element->status_flags bitand MISEVAL_FLIGHT_DESTROYED)
                    flight_ptr->failure_code = 25;
                else
                    flight_ptr->failure_code = 24;
            }

            break;

        case AMIS_OCASTRIKE:
        case AMIS_INTSTRIKE:
        case AMIS_STRIKE:
        case AMIS_DEEPSTRIKE:
        case AMIS_STSTRIKE:
        case AMIS_STRATBOMB:
            if (flight_ptr->status_flags bitand MISEVAL_FLIGHT_TARGET_HIT)
            {
                int statloss = 0;
                CampEntity e = FindEntity(flight_ptr->target_id);

                if (e and e->IsObjective())
                    statloss = flight_ptr->target_status - ((Objective)e)->GetObjectiveStatus();

                if (flight_ptr->status_flags bitand MISEVAL_FLIGHT_HIT_HIGH_VAL or statloss > 35)
                {
                    retval = Success;
                    flight_ptr->failure_code = 4;
                }
                else if (statloss > 10)
                {
                    retval = Success;
                    flight_ptr->failure_code = 3;
                }
                else
                {
                    retval = PartSuccess;
                    flight_ptr->failure_code = 2;
                }
            }
            else
                flight_ptr->failure_code = 1;

            break;

        case AMIS_SEADSTRIKE:
        case AMIS_PRPLANCAS:
        case AMIS_CAS:
        case AMIS_ASW:
        case AMIS_ASHIP:
            if (flight_ptr->status_flags bitand MISEVAL_FLIGHT_TARGET_HIT)
            {
                int statloss = 0;
                CampEntity e;
                e = FindEntity(flight_ptr->target_id);

                if (e and e->IsUnit())
                    statloss = FloatToInt32((float)(flight_ptr->target_status - ((Unit)e)->GetTotalVehicles()) * 100.0F / flight_ptr->target_status);

                if (flight_ptr->status_flags bitand MISEVAL_FLIGHT_HIT_HIGH_VAL or statloss > 8)
                {
                    retval = Success;
                    flight_ptr->failure_code = 8;
                }
                else if (statloss > 2)
                {
                    retval = Success;
                    flight_ptr->failure_code = 7;
                }
                else
                {
                    retval = PartSuccess;
                    flight_ptr->failure_code = 6;
                }
            }
            else
                flight_ptr->failure_code = 5;

            break;

        case AMIS_SAD:
        case AMIS_INT:
        case AMIS_BAI:
        case AMIS_PATROL:
            if (flight_ptr->status_flags bitand MISEVAL_FLIGHT_GOT_AKILL or
                flight_ptr->status_flags bitand MISEVAL_FLIGHT_GOT_GKILL or
                flight_ptr->status_flags bitand MISEVAL_FLIGHT_GOT_NKILL)
            {
                // Calculate hit ratio bitand kills
                int hit = 0, fired = 0, kills = 0, i;
                PilotDataClass *pilot_data;
                pilot_data = flight_ptr->pilot_list;

                while (pilot_data)
                {
                    for (i = 0; i < pilot_data->weapon_types; i++)
                    {
                        hit += pilot_data->weapon_data[i].hit;

                        if ( not (WeaponDataTable[pilot_data->weapon_data[i].weapon_id].GuidanceFlags bitand WEAP_GUIDED_MASK))
                            hit += pilot_data->weapon_data[i].hit; // double credit for unguided hits

                        if ( not (WeaponDataTable[pilot_data->weapon_data[i].weapon_id].Flags bitand WEAP_ONETENTH))
                            fired += pilot_data->weapon_data[i].fired; // only count nongun weapons as fired
                    }

                    kills += pilot_data->ag_kills + pilot_data->an_kills * 4;
                    pilot_data = pilot_data->next_pilot;
                }

                if (kills > 4)
                {
                    retval = Success;
                    flight_ptr->failure_code = 13;
                }

                if (kills > 0 and ((fired > 0 and ((hit * 100) / fired) > 49) or (hit > 0 and not fired)))
                {
                    retval = Success;
                    flight_ptr->failure_code = 11;
                }
                else
                {
                    retval = PartSuccess;
                    flight_ptr->failure_code = 12;
                }
            }
            else
                flight_ptr->failure_code = 10;

            break;

        case AMIS_ONCALLCAS:

            // Check to see that contact was made with FAC
            if (flight_ptr->status_flags bitand MISEVAL_FLIGHT_GOT_TO_TARGET)
            {
                if (flight_ptr->status_flags bitand MISEVAL_FLIGHT_OFF_STATION)
                {
                    retval = Failed;
                    flight_ptr->failure_code = 58;
                }
                else
                {
                    retval = PartSuccess;
                    flight_ptr->failure_code = 52;
                }
            }
            else
                flight_ptr->failure_code = 51;

            break;

        case AMIS_RECON:
        case AMIS_BDA:
        case AMIS_RECONPATROL:
            if (flight_ptr->status_flags bitand MISEVAL_FLIGHT_TARGET_HIT)
            {
                flight_ptr->failure_code = 15;
                retval = Success;
            }
            else
                flight_ptr->failure_code = 16;

            break;

        case AMIS_AWACS:
        case AMIS_JSTAR:
        case AMIS_TANKER:
        case AMIS_ECM:
        case AMIS_FAC:
            if (flight_ptr->status_flags bitand MISEVAL_FLIGHT_OFF_STATION)
            {
                retval = Failed;
                flight_ptr->failure_code = 58;
            }
            else if (flight_ptr->status_flags bitand MISEVAL_FLIGHT_RELIEVED)
            {
                retval = PartSuccess;
                flight_ptr->failure_code = 56;
            }
            else if (flight_ptr->status_flags bitand MISEVAL_FLIGHT_STATION_OVER)
            {
                retval = Success;
                flight_ptr->failure_code = 55;
            }
            else
                flight_ptr->failure_code = 57;

            break;

        case AMIS_SAR:
        case AMIS_AIRCAV:
        case AMIS_AIRLIFT:
            if (flight_ptr->status_flags bitand MISEVAL_FLIGHT_DESTROYED)
                flight_ptr->failure_code = 30;
            else if (flight_ptr->status_flags bitand MISEVAL_FLIGHT_GOT_TO_TARGET)
            {
                flight_ptr->failure_code = 60;
                retval = Success;
            }
            else
                flight_ptr->failure_code = 61;

            break;

        default:
            flight_ptr->failure_code = 99;
            break;
    }

    // If we didn't fly it all the way home, knock off one success level
    if (retval < Failed and not (flight_ptr->status_flags bitand MISEVAL_FLIGHT_GOT_HOME))
        retval -= 1;

    // If we didn't fly it from the start, cap success at Partial.
    if (retval == Success and (flight_ptr->status_flags bitand MISEVAL_FLIGHT_STARTED_LATE))
        retval = PartSuccess;

    return retval;
}

void MissionEvaluationClass::SetPackageData(void)
{
    // Set the package pointer for everything associated with the local player's package
    CampEntity ent;
    FlightDataClass *flight_ptr;

    flight_ptr = flight_data;

    while (flight_ptr)
    {
        ent = (CampEntity) vuDatabase->Find(flight_ptr->flight_id);

        if (ent)
            ent->SetInPackage(1);

        flight_ptr = flight_ptr->next_flight;
    }
}

void MissionEvaluationClass::ClearPackageData(void)
{
    // Clear the package pointer for everything associated with the local player's package
    CampEntity ent;
    FlightDataClass *flight_ptr;

    flight_ptr = flight_data;

    while (flight_ptr)
    {
        ent = (CampEntity) vuDatabase->Find(flight_ptr->flight_id);

        if (ent)
            ent->SetInPackage(0);

        flight_ptr = flight_ptr->next_flight;
    }
}

void MissionEvaluationClass::ClearPotentialTargets(void)
{
    for (int i = 0; i < MAX_POTENTIAL_TARGETS; i++)
        potential_targets[i] = FalconNullId;
}

void MissionEvaluationClass::FindPotentialTargets(void)
{
    int i, j;
    Objective o;

    ClearPotentialTargets();

    if (package_mission == AMIS_BARCAP or package_mission == AMIS_BARCAP2)
    {
#ifdef VU_GRID_TREE_Y_MAJOR
        VuGridIterator* myit = new VuGridIterator(ObjProxList, (BIG_SCALAR)GridToSim(tx), (BIG_SCALAR)GridToSim(ty), (BIG_SCALAR)GridToSim(MissionData[package_mission].mindistance));
#else
        VuGridIterator* myit = new VuGridIterator(ObjProxList, (BIG_SCALAR)GridToSim(ty), (BIG_SCALAR)GridToSim(tx), (BIG_SCALAR)GridToSim(MissionData[package_mission].mindistance));
#endif
        float d, wd, dists[MAX_POTENTIAL_TARGETS];
        GridIndex x, y;

        for (i = 0; i < MAX_POTENTIAL_TARGETS; i++)
            dists[i] = 999.9F;

        o = (Objective) myit->GetFirst();

        while (o)
        {
            if (o->GetTeam() == team and o->GetObjectiveStatus() > 30 and (o->GetType() == TYPE_BRIDGE or o->GetType() == TYPE_AIRBASE or o->GetType() == TYPE_DEPOT or o->GetType() == TYPE_ARMYBASE or o->GetType() == TYPE_FACTORY or o->GetType() == TYPE_RADAR or o->GetType() == TYPE_PORT or o->GetType() == TYPE_REFINERY or o->GetType() == TYPE_POWERPLANT))
            {
                o->GetLocation(&x, &y);
                d = Distance(x, y, tx, ty);

                // find the best distance to replace
                for (i = 0, j = -1, wd = 0.0F; i < MAX_POTENTIAL_TARGETS; i++)
                {
                    if (dists[i] > wd and d < dists[i])
                    {
                        j = i;
                        wd = dists[i];
                    }
                }

                if (j >= 0)
                {
                    dists[j] = d;
                    potential_targets[j] = o->Id();
                }
            }

            o = (Objective) myit->GetNext();
        }
    }
}

void MissionEvaluationClass::CollectThreats(Flight flight, WayPoint tw)
{
    WayPoint w, nw;
    int step, i;
    GridIndex x, y, fx, fy, nx, ny;
    float xd, yd, d;
    int dist, dists[MAX_COLLECTED_THREATS];

    memset(threat_ids, 0, sizeof(short)*MAX_COLLECTED_THREATS);

    for (i = 0; i < MAX_COLLECTED_THREATS; i++)
    {
        dists[i] = 999;
        threat_x[i] = threat_y[i] = 0;
    }

    // Collect threats at target first;
    if (tw)
    {
        tw->GetWPLocation(&x, &y);
        CollectThreats(x, y, tw->GetWPAltitude(), FIND_NOAIR bitor FIND_FINDUNSPOTTED, dists);
    }

    // Collect threats along route
    w = flight->GetFirstUnitWP();
    nw = w->GetNextWP();

    while (w and nw and w not_eq tw)
    {
        w->GetWPLocation(&fx, &fy);
        nw->GetWPLocation(&nx, &ny);
        d = Distance(fx, fy, nx, ny);
        dist = FloatToInt32(d);

        // JB 010614 CTD
        if (d not_eq 0.0)
        {
            xd = (float)(nx - fx) / d;
            yd = (float)(ny - fy) / d;
        }
        else
        {
            /////?????????????????????????????????
            xd = 0.0F;
            yd = 0.0F;
        }

        for (step = 0; step <= dist; step += 5)
        {
            x = fx + (GridIndex)(xd * step + 0.5F);
            y = fy + (GridIndex)(yd * step + 0.5F);
            CollectThreats(x, y, tw->GetWPAltitude(), FIND_NOAIR bitor FIND_FINDUNSPOTTED, dists);
        }

        w = nw;
        nw = w->GetNextWP();
    }
}

void MissionEvaluationClass::CollectThreats(GridIndex X, GridIndex Y, int Z, int flags, int *dists)
{
    int d, hc, alt = 0, i, j, k, wd, wid;
    MoveType mt;
    GridIndex x, y;
    Unit e;
    uchar tteam[NUM_TEAMS];
#ifdef VU_GRID_TREE_Y_MAJOR
    VuGridIterator myit(RealUnitProxList, (BIG_SCALAR)GridToSim(X), (BIG_SCALAR)GridToSim(Y), (BIG_SCALAR)GridToSim(MAX_AIR_SEARCH));
#else
    VuGridIterator myit(RealUnitProxList, (BIG_SCALAR)GridToSim(Y), (BIG_SCALAR)GridToSim(X), (BIG_SCALAR)GridToSim(MAX_AIR_SEARCH));
#endif

    alt = 3 * FloatToInt32(Z * 0.000303F); // Convert feet to km, then adjust heavy (1 km alt = 3 km range)
    alt = alt * alt; // pre square, to save calculations

    if (Z > LOW_ALTITUDE_CUTOFF)
        mt = Air;
    else
        mt = LowAir;

    // Set up roe checks
    for (d = 0; d < NUM_TEAMS and TeamInfo[d]; d++)
        tteam[d] = GetRoE(d, team, ROE_AIR_ENGAGE);

    // Tranverse our list
    e = (Unit) myit.GetFirst();

    while (e and not threat_ids[MAX_COLLECTED_THREATS - 1])
    {
        if (tteam[e->GetTeam()])
        {
            if ( not (flags bitand FIND_NOMOVERS and e->Moving()) and 
 not (flags bitand FIND_NOAIR and e->GetDomain() == DOMAIN_AIR) and 
                (flags bitand FIND_FINDUNSPOTTED or e->GetSpotted(team)))
            {
                // Find the distance
                e->GetLocation(&x, &y);
                d = FloatToInt32(Distance(X, Y, x, y));

                // Find hitchance
                hc = e->GetAproxHitChance(mt, d);

                if (hc and mt == Air) // Do a reasonable altitude adjusted guess
                    hc = e->GetAproxHitChance(mt, FloatToInt32((float)sqrt((float)(alt + d * d))));

                if (hc > 0)
                {
                    j = -1;

                    // Check if we've already found it before
                    for (i = 0; i < MAX_COLLECTED_THREATS; i++)
                    {
                        if (threat_x[i] == x and threat_y[i] == y)
                            j = 0;
                    }

                    // Now find which weapon it was which can hit us
                    for (i = 0; i < VEHICLE_GROUPS_PER_UNIT and j < 0; i++)
                    {
                        wid = e->GetBestVehicleWeapon(i, DefaultDamageMods, mt, d, &k);

                        if (wid and GetWeaponHitChance(wid, mt) > MINIMUM_VIABLE_THREAT)
                        {
                            // find the best distance to replace
                            for (k = 0, wd = 0; k < MAX_COLLECTED_THREATS; k++)
                            {
                                if (dists[k] > wd and d < dists[k])
                                {
                                    j = k;
                                    wd = dists[k];
                                }
                            }

                            if (j >= 0)
                            {
                                dists[j] = d;
                                threat_ids[j] = e->class_data->VehicleType[i];
                                threat_x[j] = x;
                                threat_y[j] = y;
                            }
                        }
                    }
                }
            }
        }

        e = (Unit) myit.GetNext();
    }
}

void MissionEvaluationClass::GetTeamKills(short *kills)
{
    FlightDataClass *flight_ptr;
    PilotDataClass *pilot_data;
    int t;

    CampEnterCriticalSection();

    for (t = 0; t < MAX_DOGFIGHT_TEAMS; t++)
        kills[t] = 0;

    // Check if someone in the package fired this.
    flight_ptr = flight_data;

    while (flight_ptr)
    {
        pilot_data = flight_ptr->pilot_list;

        while (pilot_data)
        {
            kills[flight_ptr->flight_team] += pilot_data->aa_kills;
            pilot_data = pilot_data->next_pilot;
        }

        flight_ptr = flight_ptr->next_flight;
    }

    CampLeaveCriticalSection();
}

/*
void MissionEvaluationClass::GetTeamKills (short *kills, int type)
 {
 FlightDataClass *flight_ptr;
 PilotDataClass *pilot_data;
 int t;

 CampEnterCriticalSection();

 for (t=0; t<MAX_DOGFIGHT_TEAMS; t++)
 kills[t] = 0;

 // Check if someone in the package fired this.
 flight_ptr = flight_data;
 while (flight_ptr)
 {
 pilot_data = flight_ptr->pilot_list;
 while (pilot_data)
 {
 for (t=0; t<MAX_DOGFIGHT_TEAMS; t++)
 {
 if (t not_eq flight_ptr->flight_team)
 {
 if (type == VS_EITHER)
 kills[flight_ptr->flight_team] += pilot_data->kills[t][VS_HUMAN] + pilot_data->kills[t][VS_AI];
 else if (type == VS_HUMAN)
 kills[flight_ptr->flight_team] += pilot_data->kills[t][VS_HUMAN];
 else
 kills[flight_ptr->flight_team] += pilot_data->kills[t][VS_AI];
 }
 }
 pilot_data = pilot_data->next_pilot;
 }
 flight_ptr = flight_ptr->next_flight;
 }
 CampLeaveCriticalSection();
 }
*/

void MissionEvaluationClass::GetTeamDeaths(short *deaths)
{
    FlightDataClass *flight_ptr;
    PilotDataClass *pilot_data;
    int t;

    CampEnterCriticalSection();

    for (t = 0; t < MAX_DOGFIGHT_TEAMS; t++)
        deaths[t] = 0;

    // Check if someone in the package fired this.
    flight_ptr = flight_data;

    while (flight_ptr)
    {
        pilot_data = flight_ptr->pilot_list;

        while (pilot_data)
        {
            deaths[flight_ptr->flight_team] += pilot_data->deaths[VS_HUMAN] + pilot_data->deaths[VS_AI];
            pilot_data = pilot_data->next_pilot;
        }

        flight_ptr = flight_ptr->next_flight;
    }

    CampLeaveCriticalSection();
}

/*
void MissionEvaluationClass::GetTeamDeaths (short *deaths, int type)
 {
 FlightDataClass *flight_ptr;
 PilotDataClass *pilot_data;
 int t;

 CampEnterCriticalSection();

 for (t=0; t<MAX_DOGFIGHT_TEAMS; t++)
 deaths[t] = 0;

 // Check if someone in the package fired this.
 flight_ptr = flight_data;
 while (flight_ptr)
 {
 pilot_data = flight_ptr->pilot_list;
 while (pilot_data)
 {
 if (type == VS_EITHER)
 deaths[flight_ptr->flight_team] += pilot_data->deaths[VS_HUMAN] + pilot_data->deaths[VS_AI];
 else if (type == VS_HUMAN)
 deaths[flight_ptr->flight_team] += pilot_data->deaths[VS_HUMAN];
 else
 deaths[flight_ptr->flight_team] += pilot_data->deaths[VS_AI];
 pilot_data = pilot_data->next_pilot;
 }
 flight_ptr = flight_ptr->next_flight;
 }

 CampLeaveCriticalSection();
 }
*/

void MissionEvaluationClass::GetTeamScore(short *score)
{
    FlightDataClass *flight_ptr;
    PilotDataClass *pilot_data;
    int t;

    CampEnterCriticalSection();

    // KCK: If we're in match play, we need to gather our 'score' from rounds won
    if (SimDogfight.GetGameType() == dog_TeamMatchplay)
    {
        for (t = 0; t < MAX_DOGFIGHT_TEAMS; t++)
            score[t] = rounds_won[t];
    }
    else
    {
        for (t = 0; t < MAX_DOGFIGHT_TEAMS; t++)
            score[t] = 0;

        // Check if someone in the package fired this.
        flight_ptr = flight_data;

        while (flight_ptr)
        {
            pilot_data = flight_ptr->pilot_list;

            while (pilot_data)
            {
                score[flight_ptr->flight_team] += pilot_data->score;
                pilot_data = pilot_data->next_pilot;
            }

            flight_ptr = flight_ptr->next_flight;
        }
    }

    CampLeaveCriticalSection();
}

int MissionEvaluationClass::GetKills(FalconSessionEntity *player)
{
    FlightDataClass *flight_ptr;
    PilotDataClass *pilot_data;

    CampEnterCriticalSection();

    // Check if someone in the package fired this.
    flight_ptr = flight_data;

    while (flight_ptr)
    {
        pilot_data = flight_ptr->pilot_list;

        while (pilot_data)
        {
            if (flight_ptr->flight_id == player->GetPlayerFlightID() and pilot_data->pilot_slot == player->GetPilotSlot())
            {
                CampLeaveCriticalSection();
                return pilot_data->aa_kills;
            }

            pilot_data = pilot_data->next_pilot;
        }

        flight_ptr = flight_ptr->next_flight;
    }

    CampLeaveCriticalSection();
    return 0;
}

int MissionEvaluationClass::GetMaxKills(void)
{
    FlightDataClass *flight_ptr;
    PilotDataClass *pilot_data;
    int best = 0;

    CampEnterCriticalSection();

    // Check if someone in the package fired this.
    flight_ptr = flight_data;

    while (flight_ptr)
    {
        pilot_data = flight_ptr->pilot_list;

        while (pilot_data)
        {
            if (pilot_data->aa_kills > best)
                best = pilot_data->aa_kills;

            pilot_data = pilot_data->next_pilot;
        }

        flight_ptr = flight_ptr->next_flight;
    }

    CampLeaveCriticalSection();
    return best;
}

int MissionEvaluationClass::GetMaxScore(void)
{
    FlightDataClass *flight_ptr;
    PilotDataClass *pilot_data;
    int best = 0;

    CampEnterCriticalSection();

    // Check if someone in the package fired this.
    flight_ptr = flight_data;

    while (flight_ptr)
    {
        pilot_data = flight_ptr->pilot_list;

        while (pilot_data)
        {
            if (pilot_data->score > best)
                best = pilot_data->score;

            pilot_data = pilot_data->next_pilot;
        }

        flight_ptr = flight_ptr->next_flight;
    }

    CampLeaveCriticalSection();
    return best;
}

void MissionEvaluationClass::RegisterShotAtPlayer(FalconWeaponsFire *wfm, unsigned short CampID, uchar fPilotID)
{
    EventElement *theEvent;
    _TCHAR time_str[20], format[80], pnum[5];

    // 2002-04-07 MN don't record gun shots at us or when someone takes a picture from us ;-)....
    if (g_bNoAAAEventRecords and (wfm->dataBlock.weaponType == FalconWeaponsFire::GUN or wfm->dataBlock.weaponType == FalconWeaponsFire::Recon))
        return;

    CampEnterCriticalSection();

    // Check if the target is also someone we're tracking messages for
    PilotDataClass *target_data = NULL;
    FlightDataClass *target_flight = flight_data;
    PilotDataClass *shooter_data = NULL;
    FlightDataClass *shooter_flight = flight_data;

    //find the target
    while (target_flight and not target_data)
    {
        if (target_flight->camp_id == CampID)
        {
            target_data = FindPilotData(target_flight, fPilotID);
            break;
        }

        target_flight = target_flight->next_flight;
    }

    if ( not target_data)
    {
        CampLeaveCriticalSection();
        return;
    }

    // find the shooter
    while (shooter_flight and not shooter_data)
    {
        if (shooter_flight->camp_id == wfm->dataBlock.fCampID)
        {
            shooter_data = FindPilotData(shooter_flight, wfm->dataBlock.fPilotID);
            break;
        }

        shooter_flight = shooter_flight->next_flight;
    }

    // Record a event
    theEvent = new EventElement;
    ParseTime(vuxGameTime, time_str);
    ReadIndexedString(target_data->aircraft_slot + 1, pnum, 4);
    //int weaponindx = WeaponDataTable[GetWeaponIdFromDescriptionIndex(wfm->dataBlock.fWeaponID-VU_LAST_ENTITY_TYPE)].Index;
    int weaponindx;
    int loop = 0;

    for (loop = 0; loop < NumWeaponTypes; loop++)
    {
        weaponindx = WeaponDataTable[loop].Index + VU_LAST_ENTITY_TYPE;

        if (wfm->dataBlock.fWeaponID == weaponindx) break;
    }

    if (shooter_data)
        sprintf(format, "%s %s launched %s at %s %s", shooter_flight->aircraft_name, shooter_data->pilot_callsign, WeaponDataTable[loop].Name, target_data->pilot_callsign , time_str);
    else
        sprintf(format, "%s launched at %s %s", WeaponDataTable[loop].Name, target_data->pilot_callsign , time_str);

    ConstructOrderedSentence(MAX_EVENT_STRING_LEN, theEvent->eventString, format, target_data->pilot_callsign, target_flight->name, pnum, time_str);
    theEvent->eventTime = vuxGameTime;
    AddEventToList(theEvent, target_flight, 0, 0);

    ShiAssert(player_pilot);

    if (target_data)
        target_data->shot_at++;

    CampLeaveCriticalSection();
}

void MissionEvaluationClass::RegisterShot(FalconWeaponsFire *wfm)
{
    _TCHAR time_str[20], format[80], target_name[20] = {0};
    PilotDataClass *pilot_data;
    EventElement *theEvent;
    int wn, windex;
    FlightDataClass *flight_ptr;

    CampEnterCriticalSection();

    // Check if someone in the package fired this.
    flight_ptr = flight_data;

    while (flight_ptr)
    {
        if ( not wfm->dataBlock.fCampID or wfm->dataBlock.fCampID not_eq flight_ptr->camp_id)
        {
            flight_ptr = flight_ptr->next_flight;
            continue;
        }

        pilot_data = flight_ptr->pilot_list;

        while (pilot_data)
        {
            if (wfm->dataBlock.fPilotID == pilot_data->pilot_slot)
            {

                for (wn = 0; wn < pilot_data->weapon_types; wn++)
                {
                    windex = WeaponDataTable[pilot_data->weapon_data[wn].weapon_id].Index + VU_LAST_ENTITY_TYPE;

                    if (wfm->dataBlock.fWeaponID == windex) // and wfm->dataBlock.fireOnOff == 1)
                    {
                        // We're interested
                        theEvent = new EventElement;

                        // Only subtract from our score for non guns and non-photos
                        if ( not (WeaponDataTable[pilot_data->weapon_data[wn].weapon_id].Flags bitand WEAP_ONETENTH) and wfm->dataBlock.weaponType not_eq FalconWeaponsFire::Recon)
                            pilot_data->score += CalcScore(SCORE_FIRE_WEAPON, windex - VU_LAST_ENTITY_TYPE);
                        else if (TheCampaign.Flags bitand CAMP_LIGHT)
                        {
                            // Don't record gun shots in IA and dogfight
                            CampLeaveCriticalSection();
                            return;
                        }

                        // Chalk up a weapon fired mark (assume a miss):
                        pilot_data->weapon_data[wn].fired++;
                        ParseTime(TheCampaign.CurrentTime, time_str);
#ifdef DEBUG
                        // Try and find a bug where weapon shots are being doubled
                        ShiAssert(wfm->dataBlock.fWeaponUID.num_ not_eq 0);
                        EventElement *tmpevent = pilot_data->weapon_data[wn].root_event;

                        while (tmpevent)
                        {
                            ShiAssert(tmpevent->vuIdData1 not_eq wfm->dataBlock.fWeaponUID);
                            tmpevent = tmpevent->next;
                        }

#endif

                        // In case of camera "shots", evaluate hit immediately
                        if (wfm->dataBlock.weaponType == FalconWeaponsFire::Recon)
                        {
                            FalconEntity *entity = (FalconEntity*) vuDatabase->Find(wfm->dataBlock.targetId);

                            if (entity and entity->IsSim())
                            {
                                if (((SimBaseClass*)entity)->GetCampaignObject()->Id() == flight_ptr->target_id)
                                    flight_ptr->status_flags or_eq MISEVAL_FLIGHT_TARGET_HIT;

                                if (Falcon4ClassTable[entity->Type() - VU_LAST_ENTITY_TYPE].dataType == DTYPE_VEHICLE)
                                    _stprintf(target_name, GetVehicleClassData(entity->Type() - VU_LAST_ENTITY_TYPE)->Name);
                                else if (Falcon4ClassTable[entity->Type() - VU_LAST_ENTITY_TYPE].dataType == DTYPE_FEATURE)
                                    _stprintf(target_name, GetFeatureClassData(entity->Type() - VU_LAST_ENTITY_TYPE)->Name);

                                pilot_data->weapon_data[wn].hit++;
                                GetFormatString(FET_PHOTO_TAKEN_HIT, format);
                            }
                            else
                            {
                                pilot_data->weapon_data[wn].missed++;
                                GetFormatString(FET_PHOTO_TAKEN_MISSED, format);
                            }
                        }
                        else
                        {
                            pilot_data->weapon_data[wn].missed++;

                            if (Falcon4ClassTable[wfm->dataBlock.fWeaponID - VU_LAST_ENTITY_TYPE].vuClassData.classInfo_[VU_TYPE] == TYPE_GUN)
                                GetFormatString(FET_FIRED_MISSED, format);
                            else
                                GetFormatString(FET_RELEASED_MISSED, format);
                        }

                        ConstructOrderedSentence(MAX_EVENT_STRING_LEN, theEvent->eventString, format, pilot_data->weapon_data[wn].weapon_name, time_str, pilot_data->pilot_callsign, target_name);
                        theEvent->vuIdData1 = wfm->dataBlock.fWeaponUID;
                        theEvent->eventTime = TheCampaign.CurrentTime;
                        AddEventToList(theEvent, flight_ptr, pilot_data, wn);
                        CampLeaveCriticalSection();
                        return;
                    }
                }
            }

            pilot_data = pilot_data->next_pilot;
        }

        flight_ptr = flight_ptr->next_flight;
    }

    CampLeaveCriticalSection();
}

void MissionEvaluationClass::RegisterHit(FalconDamageMessage *dmm)
{
    _TCHAR time_str[128], format[80], tmp[20];
    PilotDataClass *pilot_data;
    int wn, windex;
    VehicleClassDataType *vc;
    FeatureClassDataType *fc;
    FlightDataClass *flight_ptr;
    EventElement *tmpevent, *baseevent = NULL;

    // 2002-02-08 MN don't evaluate if FEAT_NO_HITEVAL (like trees...)
    if (Falcon4ClassTable[dmm->dataBlock.dIndex - VU_LAST_ENTITY_TYPE].dataType == DTYPE_FEATURE)
    {
        // get classtbl entry for feature
        fc = GetFeatureClassData(dmm->dataBlock.dIndex - VU_LAST_ENTITY_TYPE);

        if (fc and (fc->Flags bitand FEAT_NO_HITEVAL))
            return;
    }

    CampEnterCriticalSection();

    flight_ptr = flight_data;

    while (flight_ptr)
    {
        // Did we cause this damage?
        if (dmm->dataBlock.fCampID and dmm->dataBlock.fCampID == flight_ptr->camp_id)
        {
            pilot_data = flight_ptr->pilot_list;

            while (pilot_data)
            {
                if (dmm->dataBlock.fPilotID == pilot_data->pilot_slot)
                {
                    for (wn = 0; wn < pilot_data->weapon_types; wn++)
                    {
                        windex = WeaponDataTable[pilot_data->weapon_data[wn].weapon_id].Index + VU_LAST_ENTITY_TYPE;

                        // Check if fired by this flight
                        if (dmm->dataBlock.fWeaponID == windex)
                        {
                            int foundEvent = FALSE;
                            // We hit with one of these weapons, match to a weapon fire event
                            tmpevent = pilot_data->weapon_data[wn].root_event;

                            while (tmpevent and not foundEvent)
                            {
                                if (tmpevent->vuIdData1 == dmm->dataBlock.fWeaponUID)
                                {
                                    _TCHAR *sptr;
                                    // Several options can have happened here:
                                    // 1) We haven't recorded the weapon hitting anything yet -
                                    //    so we want to replace the 'miss' message with a 'hit' message
                                    // 3) The weapon hit something else and either damaged or destroyed
                                    //    it, and now another entity has been hit - so we want to make
                                    //    a new message with the result here.
                                    //
                                    // Check for a miss event first (this is the easy case)
                                    ReadIndexedString(1799, tmp, 79);
                                    sptr = _tcsstr(tmpevent->eventString, tmp);

                                    if (sptr)
                                    {
                                        // The first time we hit something with a weapon, we need to turn
                                        // a miss into a hit.
                                        if (GetRoE(GetTeam(dmm->dataBlock.fSide), GetTeam(dmm->dataBlock.dSide), ROE_AIR_FIRE) == ROE_ALLOWED)
                                            pilot_data->score += CalcScore(SCORE_HIT_ENEMY, 0); // Hit enemy
                                        else
                                            pilot_data->score += CalcScore(SCORE_HIT_FRIENDLY, 0); // Hit friendly/neutral

                                        pilot_data->weapon_data[wn].hit++;
                                        pilot_data->weapon_data[wn].missed--;
                                        foundEvent = TRUE;
                                        *sptr = 0;
                                    }
                                    else
                                    {
                                        // Check for a hit event next
                                        ReadIndexedString(1798, tmp, 79);
                                        sptr = _tcsstr(tmpevent->eventString, tmp);
                                        // We'd better have one or something is very wrong
                                        ShiAssert(sptr);

                                        // Now determine if it's the same target or not
                                        if (sptr and tmpevent->vuIdData2 == dmm->dataBlock.dEntityID)
                                        {
                                            foundEvent = TRUE;
                                            *sptr = 0;
                                        }
                                    }

                                    // If we haven't found the first event for this weapon yet, record
                                    // it now.
                                    if ( not baseevent)
                                        baseevent = tmpevent;
                                }

                                if ( not foundEvent)
                                    tmpevent = tmpevent->next;
                            }

                            // Make a new event if we need to
                            if ( not tmpevent)
                            {
                                // In IA or dogfight, don't list multiple hits
                                if (TheCampaign.Flags bitand CAMP_LIGHT)
                                {
                                    CampLeaveCriticalSection();
                                    return;
                                }

                                tmpevent = new EventElement;
                                tmpevent->vuIdData1 = dmm->dataBlock.fWeaponUID;
                                tmpevent->eventTime = TheCampaign.CurrentTime;
                                ReadIndexedString(1726, tmpevent->eventString, MAX_EVENT_STRING_LEN);
                                InsertEventToList(tmpevent, baseevent);
                            }

                            tmpevent->vuIdData2 = dmm->dataBlock.dEntityID;
                            ParseTime(TheCampaign.CurrentTime, time_str);

                            if (Falcon4ClassTable[dmm->dataBlock.dIndex - VU_LAST_ENTITY_TYPE].dataType == DTYPE_VEHICLE)
                            {
                                vc = GetVehicleClassData(dmm->dataBlock.dIndex - VU_LAST_ENTITY_TYPE);

                                if (vc) // JB 010113
                                    _stprintf(tmp, vc->Name);
                                else
                                    _stprintf(tmp, "");
                            }
                            else if (Falcon4ClassTable[dmm->dataBlock.dIndex - VU_LAST_ENTITY_TYPE].dataType == DTYPE_FEATURE)
                            {
                                fc = GetFeatureClassData(dmm->dataBlock.dIndex - VU_LAST_ENTITY_TYPE);

                                if (fc) // JB 010113
                                    _stprintf(tmp, fc->Name);
                                else
                                    _stprintf(tmp, "");
                            }

                            // Add on the damaged message
                            GetFormatString(FET_DAMAGED, format);
                            ConstructOrderedSentence(128, time_str, format, tmpevent->eventString, tmp);
                            _sntprintf(tmpevent->eventString, MAX_EVENT_STRING_LEN, time_str);
                            CampLeaveCriticalSection();
                            return;
                        }
                    }
                }

                pilot_data = pilot_data->next_pilot;
            }
        }
        // Did we take this damage?
        else if (dmm->dataBlock.dCampID and dmm->dataBlock.dCampID == flight_ptr->camp_id)
        {
            pilot_data = flight_ptr->pilot_list;

            while (pilot_data)
            {
                if (dmm->dataBlock.dPilotID == pilot_data->pilot_slot)
                {
                    // Just mark us as damaged
                    if (pilot_data->aircraft_status not_eq VIS_DESTROYED)
                        pilot_data->aircraft_status = VIS_DAMAGED;
                }

                pilot_data = pilot_data->next_pilot;
            }
        }

        flight_ptr = flight_ptr->next_flight;
    }

    CampLeaveCriticalSection();
}

void MissionEvaluationClass::RegisterKill(FalconDeathMessage *dtm, int type, int pilot_status)
{
    _TCHAR time_str[128], format[80], tmp[30], tmp2[30], pnum[5];
    VehicleClassDataType *vc;
    FeatureClassDataType *fc;
    PilotDataClass *pilot_data, *shooter_data = NULL, *target_data = NULL;
    EventElement *theEvent;
    int wn, windex, drop = 0, posted = 0, nokill = 0;
    FlightDataClass *flight_ptr, *shooter_flight = NULL, *target_flight = NULL;

    // 2002-02-08 MN don't evaluate if FEAT_NO_HITEVAL (like trees...)
    if (Falcon4ClassTable[dtm->dataBlock.dIndex - VU_LAST_ENTITY_TYPE].dataType == DTYPE_FEATURE)
    {
        // get classtbl entry for feature
        fc = GetFeatureClassData(dtm->dataBlock.dIndex - VU_LAST_ENTITY_TYPE);

        if (fc and (fc->Flags bitand FEAT_NO_HITEVAL))
            return;
    }

    CampEnterCriticalSection();

    flight_ptr = flight_data;

    while (flight_ptr)
    {
        // Did we get killed?
        if (dtm->dataBlock.dCampID and dtm->dataBlock.dCampID == flight_ptr->camp_id) // hit plane in this flight
        {
            // edg: It has been observed that pilot_data will be NULL when
            // feature is the entity.  Do check for this and don't exec the next
            // block of code if NULL.  I'm not sure this entirely correct
            pilot_data = FindPilotData(flight_ptr, dtm->dataBlock.dPilotID);

            // Don't do this if the pilot is already toast (or ejected)
            if (pilot_data and pilot_data->pilot_status == PILOT_IN_USE)
            {
                // Only record pilot status in non-dogfight games
                if (FalconLocalGame->GetGameType() not_eq game_Dogfight)
                    pilot_data->pilot_status = pilot_status;

                theEvent = new EventElement;
                ParseTime(TheCampaign.CurrentTime, time_str);

                pilot_data->aircraft_status = VIS_DESTROYED;
                ReadIndexedString(pilot_data->aircraft_slot + 1, pnum, 4);

                // Find shooter flight and pilot data, if they're also in our package
                shooter_data = NULL;
                shooter_flight = flight_data;

                while (shooter_flight and not shooter_data)
                {
                    if (shooter_flight->camp_id == dtm->dataBlock.fCampID)
                    {
                        shooter_data = FindPilotData(shooter_flight, dtm->dataBlock.fPilotID);

                        if ( not shooter_data)
                        {
                            break;
                        }
                    }
                    else
                    {
                        shooter_flight = shooter_flight->next_flight;
                    }
                }

                // Crash or kill?
                if ( not dtm->dataBlock.fIndex or dtm->dataBlock.damageType == FalconDamageType::GroundCollisionDamage)
                {
                    // Ground collision
                    GetFormatString(FET_PILOT_CRASHED, format);
                    ConstructOrderedSentence(MAX_EVENT_STRING_LEN, theEvent->eventString, format, pilot_data->pilot_callsign, time_str);

                    if (pilot_data == shooter_data)
                    {
                        // We killed ourselves
                        pilot_data->score += CalcScore(SCORE_GROUND_COLLISION, 0);
                        logbook_data.KilledBySelf = 1;

                        if (pilot_data == player_pilot)
                        {
                            logbook_data.Flags or_eq CRASH_UNDAMAGED;
                        }
                    }
                    else
                    {
                        // Crashed after taking damage
                        pilot_data->score += CalcScore(SCORE_GROUND_COLLISION_KILL, 0);
                    }

                    nokill = 1;
                }
                else if (dtm->dataBlock.damageType == FalconDamageType::FeatureCollisionDamage)
                {
                    // Hit feature
                    fc = GetFeatureClassData(dtm->dataBlock.fIndex - VU_LAST_ENTITY_TYPE);
                    GetFormatString(FET_COLLIDED_WITH_FEATURE, format);

                    if (fc) // JB 010113
                        ConstructOrderedSentence(MAX_EVENT_STRING_LEN, theEvent->eventString, format, pilot_data->pilot_callsign, fc->Name, time_str, time_str);
                    else
                        ConstructOrderedSentence(MAX_EVENT_STRING_LEN, theEvent->eventString, format, pilot_data->pilot_callsign, "", time_str, time_str);

                    if (pilot_data and pilot_data == player_pilot and shooter_data == player_pilot)
                    {
                        logbook_data.KilledBySelf = 1;
                    }

                    pilot_data->score += CalcScore(SCORE_FEATURE_COLLISION, 0);
                }
                else if (dtm->dataBlock.damageType == FalconDamageType::ObjectCollisionDamage or dtm->dataBlock.damageType == FalconDamageType::CollisionDamage)
                {
                    // Hit vehicle
                    if (shooter_flight and shooter_data)
                    {
                        // determine other guy's callsign
                        _stprintf(tmp, shooter_flight->name);
                        ReadIndexedString(shooter_data->aircraft_slot + 1, tmp2, 4);
                        GetFormatString(FET_COLLIDED_WITH_PILOT, format);
                        ConstructOrderedSentence(MAX_EVENT_STRING_LEN, theEvent->eventString, format, pilot_data->pilot_callsign, shooter_data->pilot_callsign, time_str);
                    }
                    else
                    {
                        // determine vehicle name and country
                        vc = GetVehicleClassData(dtm->dataBlock.fIndex - VU_LAST_ENTITY_TYPE);

                        if (gLangIDNum == F4LANG_GERMAN)
                            ReadIndexedString(ConvertTeamToStringIndex(dtm->dataBlock.fSide, F4LANG_FEMININE), tmp2, 29);
                        // ReadIndexedString(3821+20*dtm->dataBlock.fSide,tmp2,29);
                        else
                            ReadIndexedString(ConvertTeamToStringIndex(dtm->dataBlock.fSide), tmp2, 29);

                        // ReadIndexedString(3820+20*dtm->dataBlock.fSide,tmp2,29);
                        GetFormatString(FET_COLLIDED_WITH_VEHICLE, format);

                        if (vc) // JB 010113 Death due to ejected pilot collision was causing a CTD
                            ConstructOrderedSentence(MAX_EVENT_STRING_LEN, theEvent->eventString, format, pilot_data->pilot_callsign, tmp2, vc->Name, time_str);
                        else
                            ConstructOrderedSentence(MAX_EVENT_STRING_LEN, theEvent->eventString, format, pilot_data->pilot_callsign, tmp2, "", time_str);
                    }

                    nokill = 1;
                    pilot_data->score += CalcScore(SCORE_VEHICLE_COLLISION, 0);
                }
                else if (dtm->dataBlock.damageType == FalconDamageType::DebrisDamage or dtm->dataBlock.damageType == FalconDamageType::ProximityDamage)
                {
                    // Killed by debris
                    GetFormatString(FET_PILOT_KILLED_BY_DEBREE, format);
                    ConstructOrderedSentence(MAX_EVENT_STRING_LEN, theEvent->eventString, format, pilot_data->pilot_callsign, time_str);
                    pilot_data->score += CalcScore(SCORE_DEBREE_KILL, 0);
                }
                else if (dtm->dataBlock.damageType == FalconDamageType::BombDamage)
                {
                    // hit by bomb
                    GetFormatString(FET_PILOT_KILLED_BY_BOMB, format);
                    ConstructOrderedSentence(MAX_EVENT_STRING_LEN, theEvent->eventString, format, pilot_data->pilot_callsign, time_str);
                    pilot_data->score += CalcScore(SCORE_BOMB_KILL, 0);
                }
                else if (dtm->dataBlock.damageType == FalconDamageType::FODDamage)
                {
                    // Act of god
                    GetFormatString(FET_PILOT_KILLED_BY_OTHER, format);
                    ConstructOrderedSentence(MAX_EVENT_STRING_LEN, theEvent->eventString, format, pilot_data->pilot_callsign, time_str);
                    pilot_data->score += CalcScore(SCORE_OTHER_KILL, 0);
                }
                else if (dtm->dataBlock.damageType == FalconDamageType::OtherDamage)
                {
                    // Ignore this death
                    drop = 1;
                    shooter_flight = flight_ptr;
                }
                else
                {
                    if (shooter_flight and shooter_data)
                    {
                        GetFormatString(FET_PILOT_DOWNED_BY_PACKMATE, format);
                        ConstructOrderedSentence(MAX_EVENT_STRING_LEN, theEvent->eventString, format, pilot_data->pilot_callsign, shooter_data->pilot_callsign, time_str);
                    }
                    else
                    {
                        // Determine shooter type
                        if (Falcon4ClassTable[dtm->dataBlock.fIndex - VU_LAST_ENTITY_TYPE].dataType == DTYPE_VEHICLE)
                            vc = GetVehicleClassData(dtm->dataBlock.fIndex - VU_LAST_ENTITY_TYPE);
                        else
                        {
                            UnitClassDataType *uc;
                            ShiAssert(Falcon4ClassTable[dtm->dataBlock.fIndex - VU_LAST_ENTITY_TYPE].dataType == DTYPE_UNIT);
                            uc = (UnitClassDataType*)Falcon4ClassTable[dtm->dataBlock.fIndex - VU_LAST_ENTITY_TYPE].dataPtr;
                            vc = GetVehicleClassData(uc->VehicleType[0]);
                        }

                        if (vc) // JB 010113
                            _stprintf(tmp, vc->Name);
                        else
                            _stprintf(tmp, "");

                        if (gLangIDNum == F4LANG_GERMAN)
                            ReadIndexedString(ConvertTeamToStringIndex(dtm->dataBlock.fSide, F4LANG_FEMININE), tmp2, 29);
                        // ReadIndexedString(3821+20*dtm->dataBlock.fSide,tmp2,29);
                        else
                            ReadIndexedString(ConvertTeamToStringIndex(dtm->dataBlock.fSide), tmp2, 29);

                        // ReadIndexedString(3820+20*dtm->dataBlock.fSide,tmp2,29);
                        GetFormatString(FET_PILOT_DOWNED_BY_VEHICLE, format);

                        if (vc) // JB 010113
                            ConstructOrderedSentence(MAX_EVENT_STRING_LEN, theEvent->eventString, format, pilot_data->pilot_callsign, tmp2, vc->Name, time_str);
                        else
                            ConstructOrderedSentence(MAX_EVENT_STRING_LEN, theEvent->eventString, format, pilot_data->pilot_callsign, tmp2, "", time_str);
                    }

                    if (Falcon4ClassTable[dtm->dataBlock.fIndex - VU_LAST_ENTITY_TYPE].vuClassData.classInfo_[VU_DOMAIN] == DOMAIN_AIR)
                        friendly_aa_losses++;
                    else
                        friendly_ga_losses++;

                    pilot_data->score += CalcScore(SCORE_VEHICLE_KILL, 0);
                    drop = 1; // Don't post this if someone in our package caused this death
                }

                // Ace factor and death scoring
                if (shooter_data and shooter_data->pilot_slot > PILOTS_PER_FLIGHT)
                {
                    // Player killed us.
                    FalconSessionEntity *ssession = gCommsMgr->FindCampaignPlayer(shooter_flight->flight_id, shooter_data->aircraft_slot);
                    FalconSessionEntity *dsession = gCommsMgr->FindCampaignPlayer(flight_ptr->flight_id, pilot_data->aircraft_slot);

                    if (ssession and dsession and ssession not_eq dsession and GetCCRelations(dtm->dataBlock.fSide, dtm->dataBlock.dSide) not_eq Allied)
                        dsession->SetAceFactorDeath(ssession->GetInitAceFactor());

                    pilot_data->deaths[VS_HUMAN]++;
                }
                else
                {
                    pilot_data->deaths[VS_AI]++;
                }

                // Update remote scores for this guy
                // UpdateEvaluators(flight_ptr, pilot_data);

                // Need to determine how to tell when we're killed by a player
                if (dtm->dataBlock.fPilotID > PILOTS_PER_FLIGHT and not logbook_data.KilledBySelf)
                    logbook_data.KilledByHuman = 1;

                theEvent->eventTime = TheCampaign.CurrentTime;

                // Only post this event if it's not also going to be posted as a kill
                // i.e. If both shooter and dead guy are in same flight and it's one of several
                // messages which show in both catagories, drop the message
                if (drop and (shooter_flight == flight_ptr or FalconLocalGame->GetGameType() == game_Dogfight))
                    delete theEvent;
                else
                {
                    AddEventToList(theEvent, flight_ptr, NULL, 0);
                    posted = 1;
                }
            }
        }

        // Did we do the killing?
        if (dtm->dataBlock.fCampID and dtm->dataBlock.fCampID == flight_ptr->camp_id)
        {
            // edg: It has been observed that pilot_data will be NULL when
            // feature is the entity.  Do check for this and don't exec the next
            // block of code if NULL.  I'm not sure this entirely correct
            pilot_data = FindPilotData(flight_ptr, dtm->dataBlock.fPilotID);

            if (pilot_data)
            {
                if (dtm->dataBlock.fCampID and dtm->dataBlock.fCampID == flight_ptr->camp_id and 
                    (dtm->dataBlock.fPilotID not_eq dtm->dataBlock.dPilotID or dtm->dataBlock.fCampID not_eq dtm->dataBlock.dCampID))
                {
                    theEvent = new EventElement;
                    ParseTime(TheCampaign.CurrentTime, time_str);

                    // Check if the target is also someone we're tracking messages for
                    target_data = NULL;
                    target_flight = flight_data;

                    while (target_flight and not target_data)
                    {
                        if (target_flight->camp_id == dtm->dataBlock.dCampID)
                        {
                            target_data = FindPilotData(target_flight, dtm->dataBlock.dPilotID);

                            if ( not target_data)
                                break;
                        }
                        else
                            target_flight = target_flight->next_flight;
                    }

                    // We're interested - and log this as a kill
                    if (Falcon4ClassTable[dtm->dataBlock.dIndex - VU_LAST_ENTITY_TYPE].dataType == DTYPE_VEHICLE)
                    {
                        vc = GetVehicleClassData(dtm->dataBlock.dIndex - VU_LAST_ENTITY_TYPE);

                        if (vc) // JB 010113
                            _stprintf(tmp, vc->Name);
                        else
                            _stprintf(tmp, "");
                    }
                    else if (Falcon4ClassTable[dtm->dataBlock.dIndex - VU_LAST_ENTITY_TYPE].dataType == DTYPE_FEATURE)
                    {
                        // get classtbl entry for feature
                        fc = GetFeatureClassData(dtm->dataBlock.dIndex - VU_LAST_ENTITY_TYPE);

                        if (fc) // JB 010113
                            _stprintf(tmp, fc->Name);
                        else
                            _stprintf(tmp, "");
                    }

                    if (Falcon4ClassTable[dtm->dataBlock.dIndex - VU_LAST_ENTITY_TYPE].vuClassData.classInfo_[VU_DOMAIN] == DOMAIN_AIR)
                    {
                        // Air to air kill
                        if (GetRoE(GetTeam(dtm->dataBlock.fSide), GetTeam(dtm->dataBlock.dSide), ROE_AIR_FIRE) == ROE_ALLOWED)
                        {
                            pilot_data->score += CalcScore(SCORE_KILL_ENEMY, dtm->dataBlock.dIndex - VU_LAST_ENTITY_TYPE); // Hit enemy
                            pilot_data->aa_kills++;
                        }
                        else
                            pilot_data->score += CalcScore(SCORE_KILL_FRIENDLY, dtm->dataBlock.dIndex - VU_LAST_ENTITY_TYPE); // Hit friendly/neutral

                        if (target_flight and target_data)
                            GetFormatString(FET_PILOT_KILLED_PACKMATE, format);
                        else
                            GetFormatString(FET_PILOT_KILLED_VEHICLE, format);
                    }
                    else if (Falcon4ClassTable[dtm->dataBlock.dIndex - VU_LAST_ENTITY_TYPE].vuClassData.classInfo_[VU_CLASS] == CLASS_FEATURE)
                    {
                        // Feature kill
                        if ( not GetRoE(GetTeam(dtm->dataBlock.fSide), GetTeam(dtm->dataBlock.dSide), ROE_AIR_ATTACK) == ROE_ALLOWED)
                            pilot_data->score += CalcScore(SCORE_KILL_FRIENDLY_FEATURE, 0); // Hit friendly/neutral
                        else
                        {
                            // Find it's relative value // Hit enemy
                            Objective o = (Objective)GetEntityByCampID(dtm->dataBlock.dCampID);
                            int fid, f, value = 0, classID;
                            fid = o->static_data.class_data->FirstFeature;

                            for (f = 0; f < o->static_data.class_data->Features and not value; f++, fid++)
                            {
                                classID = o->GetFeatureID(f);

                                if (classID == dtm->dataBlock.dIndex)
                                    value = o->GetFeatureValue(f);
                            }

                            pilot_data->score += CalcScore(SCORE_KILL_ENEMY_FEATURE, value);
                            pilot_data->as_kills++;
                        }

                        nokill = 0;
                        GetFormatString(FET_PILOT_DESTROYED_VEHICLE, format);
                    }
                    else if (Falcon4ClassTable[dtm->dataBlock.dIndex - VU_LAST_ENTITY_TYPE].vuClassData.classInfo_[VU_DOMAIN] == DOMAIN_SEA)
                    {
                        // Naval Vehicle kill
                        int bonus = 0;

                        if ( not dtm->dataBlock.dPilotID)
                            bonus += 3; // Capitol ship

                        if (GetRoE(GetTeam(dtm->dataBlock.fSide), GetTeam(dtm->dataBlock.dSide), ROE_AIR_ATTACK) == ROE_ALLOWED)
                        {
                            pilot_data->score += CalcScore(SCORE_KILL_ENEMY_NAVAL, bonus); // Hit enemy
                            pilot_data->an_kills++;
                        }
                        else
                            pilot_data->score -= CalcScore(SCORE_KILL_FRIENDLY_NAVAL, bonus); // Hit friendly/neutral

                        nokill = 0;
                        GetFormatString(FET_PILOT_DESTROYED_VEHICLE, format);
                    }
                    else if (Falcon4ClassTable[dtm->dataBlock.dIndex - VU_LAST_ENTITY_TYPE].vuClassData.classInfo_[VU_CLASS] == CLASS_VEHICLE)
                    {
                        // Ground Vehicle kill
                        if (GetRoE(GetTeam(dtm->dataBlock.fSide), GetTeam(dtm->dataBlock.dSide), ROE_AIR_ATTACK) == ROE_ALLOWED)
                        {
                            pilot_data->score += CalcScore(SCORE_KILL_ENEMY_GROUND, dtm->dataBlock.dIndex - VU_LAST_ENTITY_TYPE); // Hit enemy
                            pilot_data->ag_kills++;
                        }
                        else
                            pilot_data->score += CalcScore(SCORE_KILL_FRIENDLY_GROUND, dtm->dataBlock.dIndex - VU_LAST_ENTITY_TYPE); // Hit friendly/neutral

                        nokill = 0;
                        GetFormatString(FET_PILOT_DESTROYED_VEHICLE, format);
                    }

                    if (package_element and dtm->dataBlock.dCampID == package_element->target_camp_id)
                    {
                        pilot_data->score += CalcScore(SCORE_DIRECT_KILL_GROUND, 0); // Hit actual target, so get a bonus

                        if (pilot_data == player_pilot)
                            logbook_data.Flags or_eq DESTROYED_PRIMARY;
                    }

                    if (target_data and target_data->pilot_slot > PILOTS_PER_FLIGHT)
                    {
                        // Player Kill
                        FalconSessionEntity *dsession = gCommsMgr->FindCampaignPlayer(target_flight->flight_id, target_data->aircraft_slot);
                        FalconSessionEntity *ssession = gCommsMgr->FindCampaignPlayer(flight_ptr->flight_id, pilot_data->aircraft_slot);
                        pilot_data->player_kills++;

                        if (pilot_data == player_pilot and GetCCRelations(dtm->dataBlock.fSide, dtm->dataBlock.dSide) == Allied)
                        {
                            logbook_data.Flags or_eq FR_HUMAN_KILLED;
                        }

                        if (ssession and dsession and ssession not_eq dsession and GetCCRelations(dtm->dataBlock.fSide, dtm->dataBlock.dSide) not_eq Allied)
                            ssession->SetAceFactorKill(dsession->GetInitAceFactor());
                    }

                    if (pilot_data == player_pilot and pilot_data not_eq target_data)
                    {
                        if (GetTTRelations(GetTeam(dtm->dataBlock.fSide), GetTeam(dtm->dataBlock.dSide)) == Allied)
                        {
                            logbook_data.FriendlyFireKills++;
                        }
                    }

                    if (gLangIDNum == F4LANG_GERMAN)
                        ReadIndexedString(ConvertTeamToStringIndex(dtm->dataBlock.dSide, F4LANG_FEMININE), tmp2, 29);
                    // ReadIndexedString(3821+20*dtm->dataBlock.dSide,tmp2,29);
                    else
                        ReadIndexedString(ConvertTeamToStringIndex(dtm->dataBlock.dSide), tmp2, 29);

                    // ReadIndexedString(3820+20*dtm->dataBlock.dSide,tmp2,29);
                    if (target_data)
                        ConstructOrderedSentence(MAX_EVENT_STRING_LEN, theEvent->eventString, format, target_data->pilot_callsign, tmp2, tmp, pilot_data->pilot_callsign, time_str);
                    else
                        ConstructOrderedSentence(MAX_EVENT_STRING_LEN, theEvent->eventString, format, tmp, tmp2, tmp, pilot_data->pilot_callsign, time_str);

                    theEvent->eventTime = TheCampaign.CurrentTime;

                    // If we've already handeled this event in a different format, drop it
                    // (i.e. We collided with someone in this flight)
                    if (nokill or (posted and target_flight == flight_ptr))
                        delete theEvent;
                    else
                        AddEventToList(theEvent, flight_ptr, NULL, 0);
                }

                // Adjust the fire event to reflect a kill
                pilot_data = flight_ptr->pilot_list;

                while (pilot_data)
                {
                    for (wn = 0; wn < pilot_data->weapon_types; wn++)
                    {
                        windex = WeaponDataTable[pilot_data->weapon_data[wn].weapon_id].Index + VU_LAST_ENTITY_TYPE;

                        if (dtm->dataBlock.fPilotID == pilot_data->pilot_slot and dtm->dataBlock.fWeaponID == windex)
                        {
                            EventElement *tmpevent, *baseevent = NULL;
                            int foundEvent = FALSE;
                            ParseTime(TheCampaign.CurrentTime, time_str);

                            // Check if this belonged to our target, and flag a hit if necessary
                            if (dtm->dataBlock.dCampID == flight_ptr->target_camp_id)
                                pilot_data->score += CalcScore(SCORE_HIT_OUR_TARGET, 0);

                            // We're interested, need to show this as a kill
                            // Now we want to find the weapon fire event and replace our 'damage' message with a 'destroyed'
                            tmpevent = pilot_data->weapon_data[wn].root_event;

                            while (tmpevent and not foundEvent)
                            {
                                if (tmpevent->vuIdData1 == dtm->dataBlock.fWeaponUID)
                                {
                                    _TCHAR *sptr;
                                    // Several options can have happened here:
                                    // 1) We haven't recorded the weapon hitting anything yet -
                                    //    so we want to replace the 'miss' message with a 'hit' message
                                    // 2) The weapon hit something and damaged it, and now we're getting
                                    //    a death message for the same object - so we want to replace
                                    //    the 'hit - damaged' with 'hit - destroyed'.
                                    // 3) The weapon hit something else and either damaged or destroyed
                                    //    it, and now something else has been hit - so we want to make
                                    //    a new message with the result here.
                                    //
                                    // Check for a miss event first (this is the easy case)
                                    ReadIndexedString(1799, tmp, 79);
                                    sptr = _tcsstr(tmpevent->eventString, tmp);

                                    if (sptr)
                                    {
                                        // The first time we hit something with a weapon, we need to turn
                                        // a miss into a hit.
                                        if (GetRoE(GetTeam(dtm->dataBlock.fSide), GetTeam(dtm->dataBlock.dSide), ROE_AIR_FIRE) == ROE_ALLOWED)
                                            pilot_data->score += CalcScore(SCORE_HIT_ENEMY, 0); // Hit enemy
                                        else
                                            pilot_data->score += CalcScore(SCORE_HIT_FRIENDLY, 0); // Hit friendly/neutral

                                        pilot_data->weapon_data[wn].hit++;
                                        pilot_data->weapon_data[wn].missed--;
                                        foundEvent = TRUE;
                                        *sptr = 0;
                                    }
                                    else
                                    {
                                        // Check for a hit event next
                                        ReadIndexedString(1798, tmp, 79);
                                        sptr = _tcsstr(tmpevent->eventString, tmp);
                                        // We'd better have one or something is very wrong
                                        ShiAssert(sptr);

                                        // Now determine if it's the same target or not
                                        if (sptr)
                                        {
                                            if (tmpevent->vuIdData2 == dtm->dataBlock.dEntityID)
                                            {
                                                foundEvent = TRUE;
                                                *sptr = 0;
                                            }

                                            // In IA or dogfight, take it regardless of the target -
                                            // we're just going to list the last thing killed in this
                                            // section (the rest of the kills are listed in a different
                                            // format
                                            if (TheCampaign.Flags bitand CAMP_LIGHT)
                                            {
                                                foundEvent = TRUE;
                                                *sptr = 0;
                                            }
                                        }
                                    }

                                    // If we haven't found the first event for this weapon yet, record
                                    // it now.
                                    if ( not baseevent)
                                        baseevent = tmpevent;
                                }

                                if ( not foundEvent)
                                    tmpevent = tmpevent->next;
                            }

                            // In IA or dogfight, don't list multiple kills or cannon kills - we have no
                            // fire event to match to, so it'd look lame. Besides, the kills are listed
                            // in another format anyway.
                            if (tmpevent or not (TheCampaign.Flags bitand CAMP_LIGHT))
                            {
                                // Make a new event if we need to
                                if ( not tmpevent)
                                {
                                    tmpevent = new EventElement;
                                    tmpevent->vuIdData1 = dtm->dataBlock.fWeaponUID;
                                    tmpevent->eventTime = TheCampaign.CurrentTime;
                                    ReadIndexedString(1726, tmpevent->eventString, MAX_EVENT_STRING_LEN);
                                    InsertEventToList(tmpevent, baseevent);
                                }

                                tmpevent->vuIdData2 = dtm->dataBlock.dEntityID;

                                if (Falcon4ClassTable[dtm->dataBlock.dIndex - VU_LAST_ENTITY_TYPE].dataType == DTYPE_VEHICLE)
                                {
                                    vc = GetVehicleClassData(dtm->dataBlock.dIndex - VU_LAST_ENTITY_TYPE);

                                    if (vc)
                                        _stprintf(tmp, vc->Name);
                                    else
                                        _stprintf(tmp, "");
                                }
                                else if (Falcon4ClassTable[dtm->dataBlock.dIndex - VU_LAST_ENTITY_TYPE].dataType == DTYPE_FEATURE)
                                {
                                    fc = GetFeatureClassData(dtm->dataBlock.dIndex - VU_LAST_ENTITY_TYPE);

                                    if (fc) // JB 010113
                                        _stprintf(tmp, fc->Name);
                                    else
                                        _stprintf(tmp, "");
                                }

#ifdef DEBUG
                                else
                                    ShiAssert(0);

#endif
                                // Add on the destroyed message
                                GetFormatString(FET_DESTROYED, format);
                                ConstructOrderedSentence(128, time_str, format, tmpevent->eventString, tmp);
                                _sntprintf(tmpevent->eventString, MAX_EVENT_STRING_LEN, time_str);
                            }
                        }
                    }

                    pilot_data = pilot_data->next_pilot;
                }
            }
        }

        flight_ptr = flight_ptr->next_flight;
    }

    CampLeaveCriticalSection();
}

void MissionEvaluationClass::RegisterPlayerJoin(FalconPlayerStatusMessage *fpsm)
{
    _TCHAR time_str[20], format[80], pnum[5];
    PilotDataClass *pilot_data;
    EventElement *theEvent;
    FlightDataClass *flight_ptr;

    CampEnterCriticalSection();

    // Check if someone in the package fired this.
    flight_ptr = flight_data;

    while (flight_ptr)
    {
        if (flight_ptr->flight_id == FalconLocalSession->GetPlayerFlightID())
            flags or_eq MISEVAL_MISSION_IN_PROGRESS;

        pilot_data = flight_ptr->pilot_list;

        while (pilot_data)
        {
            if (fpsm->dataBlock.campID and fpsm->dataBlock.campID == flight_ptr->camp_id and 
                fpsm->dataBlock.pilotID == pilot_data->pilot_slot)
            {
                // We're interested
                theEvent = new EventElement;
                ParseTime(vuxGameTime, time_str);
                ReadIndexedString(pilot_data->aircraft_slot + 1, pnum, 4);

                if (fpsm->dataBlock.state == PSM_STATE_ENTERED_SIM)
                    GetFormatString(FET_PILOT_JOINED, format);
                else if (fpsm->dataBlock.state == PSM_STATE_LEFT_SIM)
                    GetFormatString(FET_PILOT_EXITED, format);

                ConstructOrderedSentence(MAX_EVENT_STRING_LEN, theEvent->eventString, format, fpsm->dataBlock.callsign, flight_ptr->name, pnum, time_str);
                theEvent->eventTime = vuxGameTime;
                AddEventToList(theEvent, flight_ptr, 0, 0);

                {
                    UI_SendChatMessage *chat;

                    chat = new UI_SendChatMessage(FalconNullId, FalconLocalSession);

                    chat->dataBlock.from = FalconNullId;
                    chat->dataBlock.size = (strlen(theEvent->eventString) + 1) * sizeof(char);
                    chat->dataBlock.message = new char [strlen(theEvent->eventString) + 1];
                    memcpy(chat->dataBlock.message, theEvent->eventString, chat->dataBlock.size);
                    FalconSendMessage(chat, TRUE);
                }

                CampLeaveCriticalSection();
                return;
            }

            pilot_data = pilot_data->next_pilot;
        }

        flight_ptr = flight_ptr->next_flight;
    }

    CampLeaveCriticalSection();
}

void MissionEvaluationClass::RegisterEjection(FalconEjectMessage *em, int pilot_status)
{
    _TCHAR time_str[20], format[80];
    PilotDataClass *pilot_data;
    EventElement *theEvent;
    FlightDataClass *flight_ptr;

    CampEnterCriticalSection();

    // Check if someone in the package fired this.
    flight_ptr = flight_data;

    while (flight_ptr)
    {
        pilot_data = flight_ptr->pilot_list;

        while (pilot_data)
        {
            if (em->dataBlock.eCampID and em->dataBlock.eCampID == flight_ptr->camp_id and 
                em->dataBlock.ePilotID == pilot_data->pilot_slot)
            {
                // We're interested
                theEvent = new EventElement;
                ParseTime(TheCampaign.CurrentTime, time_str);
                GetFormatString(FET_PILOT_EJECTED, format);

                // Only record pilot status in non-dogfight games
                if (pilot_data->pilot_status == PILOT_IN_USE and FalconLocalGame->GetGameType() not_eq game_Dogfight)
                {
                    pilot_data->pilot_status = pilot_status;
                }

                pilot_data->aircraft_status = VIS_DESTROYED;

                if (em->dataBlock.hadLastShooter) // Ejected after taking damage
                {
                    pilot_data->score += CalcScore(SCORE_VEHICLE_KILL, 0); // Score vehicle death
                    pilot_data->score += CalcScore(SCORE_PILOT_EJECTED_KILL, 0); // Score the ejection
                }
                else // Ejected undamaged
                {
                    pilot_data->score += CalcScore(SCORE_GROUND_COLLISION, 0); // Score ground collistion
                    pilot_data->score += CalcScore(SCORE_PILOT_EJECTED, 0); // Score the ejection

                    if (pilot_data == player_pilot)
                    {
                        logbook_data.Flags or_eq EJECT_UNDAMAGED;
                    }
                }

                ConstructOrderedSentence(
                    MAX_EVENT_STRING_LEN, theEvent->eventString, format, pilot_data->pilot_callsign, time_str
                );
                theEvent->eventTime = TheCampaign.CurrentTime;
                AddEventToList(theEvent, flight_ptr, 0, 0);
                CampLeaveCriticalSection();
                return;
            }

            pilot_data = pilot_data->next_pilot;
        }

        flight_ptr = flight_ptr->next_flight;
    }

    CampLeaveCriticalSection();
}

void MissionEvaluationClass::RegisterLanding(FalconLandingMessage *lm, int pilot_status)
{
    _TCHAR time_str[20], format[80];
    PilotDataClass *pilot_data;
    EventElement *theEvent;
    FlightDataClass *flight_ptr;

    CampEnterCriticalSection();

    // Check if someone in the package fired this.
    flight_ptr = flight_data;

    while (flight_ptr)
    {
        pilot_data = flight_ptr->pilot_list;

        while (pilot_data)
        {
            if (lm->dataBlock.campID and lm->dataBlock.campID == flight_ptr->camp_id and 
                lm->dataBlock.pilotID == pilot_data->pilot_slot)
            {
                // We're interested
                theEvent = new EventElement;
                ParseTime(TheCampaign.CurrentTime, time_str);
                GetFormatString(FET_PILOT_LANDED, format);

                if (pilot_data->pilot_status == PILOT_IN_USE and FalconLocalGame->GetGameType() not_eq game_Dogfight)
                    pilot_data->pilot_status = pilot_status;

                // pilot_data->aircraft_status = VIS_DESTROYED;
                pilot_data->score += CalcScore(SCORE_PILOT_LANDED, 0);
                //if (pilot_data == player_pilot) Cobra it always fails this check
                //we now allow it to flag the player landing :)
                logbook_data.Flags or_eq LANDED_AIRCRAFT;
                ConstructOrderedSentence(MAX_EVENT_STRING_LEN, theEvent->eventString, format, pilot_data->pilot_callsign, time_str);
                theEvent->eventTime = TheCampaign.CurrentTime;
                AddEventToList(theEvent, flight_ptr, 0, 0);
                CampLeaveCriticalSection();
                return;
            }

            pilot_data = pilot_data->next_pilot;
        }

        flight_ptr = flight_ptr->next_flight;
    }

    CampLeaveCriticalSection();
}

void MissionEvaluationClass::RegisterContact(Unit contact)
{
    ulong score;
    // KCK NOTE: This assumes that Register Contact is only called once per contact

    // KCK: This only scores threats currently.
    // Possibly, the score should vary by mission type
    // i.e: SEAD missions score Air Defenses highest
    // ESCORT score Air to Air threats highest
    // BARCAP score enemy attack/bomber aircraft highest
    score = contact->GetHitChance(LowAir, 0);

    if (contact->IsFlight())
        score *= contact->GetTotalVehicles();

    if (contact->Broken())
        score /= 4;

    contact_score += score;
}

void MissionEvaluationClass::ParseTime(CampaignTime time, char* time_str)
{
    GetTimeString(time, time_str);
}

void MissionEvaluationClass::ParseTime(double time, char* time_str)
{
    GetTimeString(FloatToInt32((float)time * VU_TICS_PER_SECOND), time_str);
}

void MissionEvaluationClass::AddEventToList(EventElement* theEvent, FlightDataClass *flight_ptr, PilotDataClass* pilot_data, int wn)
{
    EventElement* curEvent = NULL;

    theEvent->next = NULL;

    if (pilot_data)
    {
        pilot_data->weapon_data[wn].events++;

        if ( not pilot_data->weapon_data[wn].root_event)
            pilot_data->weapon_data[wn].root_event = theEvent;
        else
            curEvent = pilot_data->weapon_data[wn].root_event;
    }
    else
    {
        flight_ptr->events++;

        if ( not flight_ptr->root_event)
            flight_ptr->root_event = theEvent;
        else
            curEvent = flight_ptr->root_event;
    }

    if ( not curEvent) // Added directly to root, we're done
        return;

    while (curEvent->next and curEvent->next not_eq curEvent)
        curEvent = curEvent->next;

    ShiAssert(curEvent not_eq theEvent);

    curEvent->next = theEvent;
}

/*
void MissionEvaluationClass::SetMissionComplete (short campId)
 {
 FlightDataClass *flight_ptr;

 flight_ptr = flight_data;
 while (flight_ptr)
 {
 if (flight_ptr->camp_id == campId)
 flight_ptr->status_flags or_eq MISEVAL_FLIGHT_GOT_HOME;
 flight_ptr = flight_ptr->next_flight;
 }
 }
*/

// ===================================
// Trigger register functions
// ===================================

void MissionEvaluationClass::RegisterKill(FalconEntity *shooter, FalconEntity *target, int targetEl)
{
    int sid, tid, i;
    CampEntity campTarget, campShooter;
    FlightDataClass *flight_ptr;

    if ( not (flags bitand MISEVAL_MISSION_IN_PROGRESS))
        return;

    CampEnterCriticalSection();

    // Get their indexes
    tid = target->Type() - VU_LAST_ENTITY_TYPE;
    sid = shooter->Type() - VU_LAST_ENTITY_TYPE;

    // Get campaign objects
    if (target->IsCampaign())
        campTarget = (CampBaseClass*)target;
    else
    {
        campTarget = ((SimBaseClass*)target)->GetCampaignObject();
        targetEl = ((SimBaseClass*)target)->GetSlot();
    }

    if (shooter->IsCampaign())
        campShooter = (CampBaseClass*)shooter;
    else
        campShooter = ((SimBaseClass*)shooter)->GetCampaignObject();

    // Check for hits by us
    if (campShooter->IsFlight() and campShooter->InPackage())
    {
        flight_ptr = FindFlightData((FlightClass*)campShooter);

        if (flight_ptr)
        {
            if (Falcon4ClassTable[tid].vuClassData.classInfo_[VU_DOMAIN] == DOMAIN_AIR and GetRoE(campShooter->GetTeam(), campTarget->GetTeam(), ROE_AIR_FIRE))
                flight_ptr->status_flags or_eq MISEVAL_FLIGHT_GOT_AKILL;
            else if (Falcon4ClassTable[tid].vuClassData.classInfo_[VU_DOMAIN] == DOMAIN_LAND and GetRoE(campShooter->GetTeam(), campTarget->GetTeam(), ROE_GROUND_FIRE))
            {
                if (Falcon4ClassTable[tid].dataType == DTYPE_OBJECTIVE or Falcon4ClassTable[tid].dataType == DTYPE_FEATURE)
                {
                    flight_ptr->status_flags or_eq MISEVAL_FLIGHT_GOT_SKILL;

                    // High value for hitting target feature
                    for (i = 0; i < MAX_TARGET_FEATURES; i++)
                    {
                        if (flight_ptr->target_features[i] < 255 and flight_ptr->target_features[i] == targetEl)
                            flight_ptr->status_flags or_eq MISEVAL_FLIGHT_HIT_HIGH_VAL;
                    }
                }
                else if (Falcon4ClassTable[tid].dataType == DTYPE_UNIT or Falcon4ClassTable[tid].dataType == DTYPE_VEHICLE)
                {
                    flight_ptr->status_flags or_eq MISEVAL_FLIGHT_GOT_GKILL;

                    // High value for killing radar vehicle
                    if (targetEl == ((UnitClass*)campTarget)->GetUnitClassData()->RadarVehicle)
                        flight_ptr->status_flags or_eq MISEVAL_FLIGHT_HIT_HIGH_VAL;

                    // For most air to ground missions, we're ok as long as we've hit the correct brigade
                    if (flight_ptr->target_id and (flight_ptr->target_id == ((UnitClass*)campTarget)->GetUnitParentID() or flight_ptr->target_id == campTarget->Id()))
                        flight_ptr->status_flags or_eq MISEVAL_FLIGHT_TARGET_HIT;
                }
            }
            else if (Falcon4ClassTable[tid].vuClassData.classInfo_[VU_DOMAIN] == DOMAIN_SEA and GetRoE(campShooter->GetTeam(), campTarget->GetTeam(), ROE_NAVAL_FIRE))
            {
                flight_ptr->status_flags or_eq MISEVAL_FLIGHT_GOT_NKILL;

                // High value for hitting capital ship
                if ( not targetEl)
                    flight_ptr->status_flags or_eq MISEVAL_FLIGHT_HIT_HIGH_VAL;
            }

            if (campTarget->Id() == flight_ptr->target_id or campTarget->GetCampID() == flight_ptr->target_camp_id)
                flight_ptr->status_flags or_eq MISEVAL_FLIGHT_TARGET_HIT;

            if (campTarget->IsUnit() and ((UnitClass*)campTarget)->IsDead())
                flight_ptr->status_flags or_eq MISEVAL_FLIGHT_TARGET_KILLED;
        }
    }

    // Check for hits against us
    if (campTarget->IsFlight() and campTarget->InPackage())
    {
        flight_ptr = FindFlightData((FlightClass*)campTarget);

        if (flight_ptr)
        {
            flight_ptr->status_flags or_eq MISEVAL_FLIGHT_LOSSES;

            if (campTarget->IsUnit() and ((UnitClass*)campTarget)->IsDead())
                flight_ptr->status_flags or_eq MISEVAL_FLIGHT_DESTROYED;

            if (Falcon4ClassTable[sid].vuClassData.classInfo_[VU_DOMAIN] == DOMAIN_AIR)
                flight_ptr->status_flags or_eq MISEVAL_FLIGHT_HIT_BY_AIR;
            else if (Falcon4ClassTable[sid].vuClassData.classInfo_[VU_DOMAIN] == DOMAIN_LAND)
                flight_ptr->status_flags or_eq MISEVAL_FLIGHT_HIT_BY_GROUND;
            else if (Falcon4ClassTable[sid].vuClassData.classInfo_[VU_DOMAIN] == DOMAIN_SEA)
                flight_ptr->status_flags or_eq MISEVAL_FLIGHT_HIT_BY_NAVAL;

            if (package_element and campTarget->Id() == package_element->flight_id)
            {
                // Package element hit - set flags on all elements
                FlightDataClass *tmp_ptr;
                int copyFlags;

                copyFlags = MISEVAL_FLIGHT_F_TARGET_HIT;

                if (flight_ptr->status_flags bitand MISEVAL_FLIGHT_DESTROYED)
                    copyFlags or_eq MISEVAL_FLIGHT_F_TARGET_KILLED;

                tmp_ptr = flight_data;

                while (tmp_ptr)
                {
                    tmp_ptr->status_flags or_eq copyFlags;
                    tmp_ptr = tmp_ptr->next_flight;
                }
            }
        }
    }

    // Check for area hits
    if (flags bitand MISEVAL_EVALUATE_HITS_IN_AREA)
    {
        GridIndex cx, cy;
        campTarget->GetLocation(&cx, &cy);

        //if (DistSqu(tx,ty,cx,cy) < 0.5F * STATION_DIST_LENIENCY*STATION_DIST_LENIENCY) // JB 010215
        if (DistSqu(tx, ty, cx, cy) < 0.5F * STATION_DIST_HITS_LENIENCY * STATION_DIST_HITS_LENIENCY) // JB 010215
        {
            // Area hit - set flags on all elements which were covering VOL
            FlightDataClass *tmp_ptr;
            Flight flight;
            tmp_ptr = flight_data;

            while (tmp_ptr)
            {
                flight = (Flight)vuDatabase->Find(tmp_ptr->flight_id);

                if (flight->GetEvalFlags() bitand FEVAL_ON_STATION)
                    tmp_ptr->status_flags or_eq MISEVAL_FLIGHT_F_AREA_HIT;

                tmp_ptr = tmp_ptr->next_flight;
            }
        }
    }

    CampLeaveCriticalSection();
}

// Set any flags associated with being someplace in space or time
void MissionEvaluationClass::RegisterMove(Flight flight)
{
    WayPoint w, nw;
    GridIndex fx, fy, wx, wy;
    float ds, cds = 1000000.0F;
    int feflags, meflags = 0;
    CampaignTime now = TheCampaign.CurrentTime;
    FlightDataClass *flight_ptr = NULL;

    ShiAssert(flight);

    if ( not flight)
        return;

    CampEnterCriticalSection();

    // Determine stuff about the flight
    flight->GetLocation(&fx, &fy);
    feflags = flight->GetEvalFlags();

    if ((flags bitand MISEVAL_MISSION_IN_PROGRESS) and flight->InPackage())
    {
        // Detailed check for flights in our package
        flight_ptr = FindFlightData(flight);

        if (flight_ptr)
            meflags = flight_ptr->status_flags;
        else
            flight->SetInPackage(0);
    }

    // Check for friendly territory
    if (GetOwner(TheCampaign.CampMapData, fx, fy) == flight->GetTeam())
    {
        // Check for getting home if we've been at our target, and either our mission time is over, or we've been relieved
        if (meflags bitand MISEVAL_FLIGHT_GOT_TO_TARGET and (meflags bitand MISEVAL_FLIGHT_STATION_OVER or meflags bitand MISEVAL_FLIGHT_RELIEVED))
            meflags or_eq MISEVAL_FLIGHT_GOT_HOME;
    }
    else
    {
        // Set mission started if we're over enemy territory
        feflags or_eq FEVAL_MISSION_STARTED;
    }

    // Check waypoint related stuff
    w = flight->GetFirstUnitWP();

    while (w)
    {
        w->GetWPLocation(&wx, &wy);

        if (w->GetWPFlags() bitand WPF_TARGET)
        {
            // 2002-02-13 MN check if target got occupied by us and has not been engaged yet - for 2D flights
            // 2002-03-03 MN fix - only for strike missions
            if (w->GetWPTarget() and w->GetWPTarget()->GetTeam() == flight->GetTeam() and 
                (flight->GetUnitMission() > AMIS_SEADESCORT and flight->GetUnitMission() < AMIS_FAC))
                meflags or_eq MISEVAL_FLIGHT_ABORT_BY_AWACS;

            // Determine station times
            CampaignTime tont, tofft;
            ds = (float)DistSqu(fx, fy, wx, wy);
            tont = w->GetWPArrivalTime();
            tofft = w->GetWPDepartureTime();
            nw = w->GetNextWP();

            if (nw and nw->GetWPFlags() bitand WPF_TARGET)
                tofft = nw->GetWPDepartureTime();

            // Determine if we're in our VOL or not
            if (now > tont and now < tofft)
                feflags or_eq FEVAL_ON_STATION;
            // Check if our VOL time is over
            else if (now > tofft)
            {
                feflags and_eq compl FEVAL_ON_STATION;
                meflags or_eq MISEVAL_FLIGHT_STATION_OVER;
            }

            // Determine if we're close enough for government work
            if (now > tont + STATION_TIME_LENIENCY and now < tofft - STATION_TIME_LENIENCY and ds > STATION_DIST_LENIENCY * STATION_DIST_LENIENCY)
                meflags or_eq MISEVAL_FLIGHT_OFF_STATION;

            // Check if we got to our target
            if (ds < TARGET_DIST_LENIENCY * TARGET_DIST_LENIENCY)
            {
                if (flight_ptr and flight_ptr == player_element and not (meflags bitand MISEVAL_FLIGHT_GOT_TO_TARGET))
                    actual_tot = TheCampaign.CurrentTime;

                feflags or_eq FEVAL_GOT_TO_TARGET;
                meflags or_eq MISEVAL_FLIGHT_GOT_TO_TARGET;
                // Set mission started
                feflags or_eq FEVAL_MISSION_STARTED;
            }
        }

        w = w->GetNextWP();
    }

    // copy the flags back in
    flight->SetEvalFlag(feflags, 1); // 2002-02-19 MODIFIED BY S.G. Added the 1 at then end of the function to specify feflags contains all the flags.

    if (flight_ptr)
        flight_ptr->status_flags = meflags;

    CampLeaveCriticalSection();
}

// MN Helper function for 3D AWACS aborts (i.e. our target got occupied by our troops)
void MissionEvaluationClass::Register3DAWACSabort(Flight flight)
{
    WayPoint w;
    int meflags = 0;
    FlightDataClass *flight_ptr = NULL;

    CampEnterCriticalSection();

    flight_ptr = FindFlightData(flight);

    if (flight_ptr)
    {
        meflags = flight_ptr->status_flags;
        w = flight->GetFirstUnitWP();

        while (w)
        {
            if (w->GetWPFlags() bitand WPF_TARGET)
            {
                if (w->GetWPTarget() and w->GetWPTarget()->GetTeam() == flight->GetTeam())
                    meflags or_eq MISEVAL_FLIGHT_ABORT_BY_AWACS;

                break;
            }

            w = w->GetNextWP();
        }

        flight_ptr->status_flags = meflags;
    }

    CampLeaveCriticalSection();
}




void MissionEvaluationClass::RegisterAbort(Flight flight)
{
    FlightDataClass *flight_ptr;
    int copyFlags = 0;

    if ( not (flags bitand MISEVAL_MISSION_IN_PROGRESS))
        return;

    if (package_element and package_element->flight_id == flight->Id())
        copyFlags = MISEVAL_FLIGHT_F_TARGET_ABORTED;

    CampEnterCriticalSection();
    flight_ptr = flight_data;

    while (flight_ptr)
    {
        if (flight_ptr->flight_id == flight->Id())
            flight_ptr->status_flags or_eq MISEVAL_FLIGHT_ABORTED;

        if (flight_ptr->target_id == flight->Id())
            flight_ptr->status_flags or_eq MISEVAL_FLIGHT_TARGET_ABORTED;

        flight_ptr->status_flags or_eq copyFlags;
        flight_ptr = flight_ptr->next_flight;
    }

    CampLeaveCriticalSection();
}

void MissionEvaluationClass::RegisterRelief(Flight flight)
{
    FlightDataClass *flight_ptr;

    if ( not (flags bitand MISEVAL_MISSION_IN_PROGRESS))
        return;

    CampEnterCriticalSection();
    flight_ptr = FindFlightData(flight);

    if (flight_ptr)
        flight_ptr->status_flags or_eq MISEVAL_FLIGHT_RELIEVED;

    CampLeaveCriticalSection();
}

void MissionEvaluationClass::RegisterDivert(Flight flight, MissionRequestClass *mis)
{
    FlightDataClass *flight_ptr;
    CampEntity target = NULL;

    if ( not (flags bitand MISEVAL_MISSION_IN_PROGRESS))
        return;

    CampEnterCriticalSection();
    flight_ptr = FindFlightData(flight);

    if (flight_ptr)
    {
        // KCK TODO: Convert this flight's mission to the divert mission
        flight_ptr->mission = mis->mission;

        if (flight_ptr == package_element)
        {
            // Trying to track down a potential bug here.. It's hard enough to
            // get diverts I figure I'll let QA do the testing..
            // ShiAssert ( not "Show this to Kevin K."); - Not any more - RH

            flight_ptr->mission_context = flight->GetMissionContext();
            flight_ptr->requester_id = flight->GetRequesterID();
            flight_ptr->target_id = flight->GetUnitMissionTargetID();
            ShiAssert(flight_ptr->target_id == mis->targetID);

            if (flight_ptr->target_id not_eq FalconNullId)
                target = (CampEntity) vuDatabase->Find(flight_ptr->target_id);

            RecordTargetStatus(flight_ptr, target);
        }
    }

    CampLeaveCriticalSection();
}

void MissionEvaluationClass::RegisterRoundWon(int team)
{
    rounds_won[team]++;
}

void MissionEvaluationClass::RegisterWin(int team)
{
    // This team won (-1 means player with highest score won)
    FlightDataClass *flight_ptr;
    PilotDataClass *pilot_data;
    int best = GetMaxScore();

    CampEnterCriticalSection();
    flags or_eq MISEVAL_GAME_COMPLETED;
    flight_ptr = flight_data;

    while (flight_ptr)
    {
        if (team == -1 or flight_ptr->flight_team == team)
        {
            pilot_data = flight_ptr->pilot_list;

            while (pilot_data)
            {
                if (team not_eq -1)
                    pilot_data->pilot_flags or_eq PFLAG_WON_GAME;
                else if (pilot_data->score >= best)
                    pilot_data->pilot_flags or_eq PFLAG_WON_GAME;

                if (pilot_data->player_kills)
                    flags or_eq MISEVAL_ONLINE_GAME;

                pilot_data = pilot_data->next_pilot;
            }
        }

        flight_ptr = flight_ptr->next_flight;
    }

    CampLeaveCriticalSection();
}

#ifdef USE_SH_POOLS
extern MEM_POOL gTextMemPool;
#endif

void MissionEvaluationClass::RegisterEvent(GridIndex x, GridIndex y, int eteam, int type, _TCHAR *event)
{
    int role;

    if ( not (flags bitand MISEVAL_MISSION_IN_PROGRESS))
        return;

    if (team not_eq eteam)
        return;

    // Filter out types by role
    role = MissionData[package_mission].skill;

    if (type not_eq FalconCampEventMessage::campStrike)
    {
        if (role == ARO_CA and type not_eq FalconCampEventMessage::campAirCombat)
            return;
        else if (role == ARO_GA and type not_eq FalconCampEventMessage::campGroundAttack)
            return;
    }

    if (DistSqu(x, y, tx, ty) < RELATED_EVENT_RANGE_SQ)
    {
        // Check if already in our list
        for (int i = 0; i < MAX_RELATED_EVENTS; i++)
        {
            if (related_events[i] and _tcscmp(related_events[i], event) == 0)
                return;
        }

        if (related_events[last_related_event])
            delete related_events[last_related_event];

#ifdef USE_SH_POOLS
        related_events[last_related_event] = (_TCHAR *)MemAllocPtr(gTextMemPool, sizeof(_TCHAR) * (_tcslen(event) + 1), FALSE);
#else
        related_events[last_related_event] = new _TCHAR[_tcslen(event) + 1];
#endif
        strcpy(related_events[last_related_event], event);
        last_related_event++;

        if (last_related_event >= MAX_RELATED_EVENTS)
            last_related_event = 0;
    }
}

// ==============================================
// Other stuff
// ==============================================

void MissionEvaluationClass::SetFinalAircraft(Flight flight)
{
    short campId, numAC, i;
    FlightDataClass *flight_ptr;

    CampEnterCriticalSection();

    campId = flight->GetCampID();
    numAC = 0;
    flight_ptr = flight_data;

    while (flight_ptr)
    {
        if (flight_ptr->camp_id == campId)
        {
            // Count remaining aircraft
            for (i = 0; i < PILOTS_PER_FLIGHT; i++)
            {
                if (flight->plane_stats[i] == AIRCRAFT_AVAILABLE)
                    numAC++;
            }

            // Record remaining aircraft
            flight_ptr->finish_aircraft = static_cast<uchar>(numAC);
        }

        flight_ptr = flight_ptr->next_flight;
    }

    CampLeaveCriticalSection();
}

int MissionEvaluationClass::GetPilotName(int pilot_num, TCHAR* buffer)
{
    PilotDataClass *pilot_ptr = NULL;
    int retval = 0;

    CampEnterCriticalSection();

    if (player_element)
    {
        pilot_ptr = player_element->pilot_list;

        if (pilot_ptr)
        {
            while (pilot_ptr and pilot_ptr->pilot_slot not_eq pilot_num)
                pilot_ptr = pilot_ptr->next_pilot;
        }
    }

    if (pilot_ptr and pilot_ptr->pilot_id not_eq NO_PILOT)
    {
        _tcscpy(buffer, pilot_ptr->pilot_name);
        retval = 1;
    }
    else
        _tcscpy(buffer, "");

    CampLeaveCriticalSection();

    return retval;
}

int MissionEvaluationClass::GetFlightName(TCHAR* buffer)
{
    int retval = 0;

    CampEnterCriticalSection();

    if (player_element)
    {
        _tcscpy(buffer, player_element->name);
        retval = 1;
    }
    else
        _tcscpy(buffer, "");

    CampLeaveCriticalSection();

    return retval;
}

PilotDataClass* MissionEvaluationClass::AddNewPlayerPilot(FlightDataClass *flight_ptr, int ac_num, Flight flight, FalconSessionEntity *player)
{
    if ( not player)
        return NULL;

#ifdef FUNKY_KEVIN_DEBUG_STUFF
    ShiAssert( not inMission or player not_eq FalconLocalSession);
#endif

    // Tack on a new slot
    PilotDataClass *pilot_data = AddNewPilot(flight_ptr, player->GetPilotSlot(), ac_num, flight);
    sprintf(pilot_data->pilot_name, player->GetPlayerName());
    sprintf(pilot_data->pilot_callsign, player->GetPlayerCallsign());
    pilot_data->pilot_flags or_eq PFLAG_PLAYER_CONTROLLED;

    if (player == FalconLocalSession)
    {
        player_pilot = pilot_data;
        player_aircraft_slot = ac_num;
    }

    return pilot_data;
}

PilotDataClass* MissionEvaluationClass::AddNewPilot(FlightDataClass *flight_ptr, int pilot_num, int ac_num, Flight flight)
{
    PilotDataClass *pilot_data = new PilotDataClass;
    PilotDataClass *prev_pilot = NULL;
    int k = 0, w = 0, wid = 0, wi = 0, nw = 0, new_weap = 0, load = 0;
    LoadoutStruct *loadout = NULL;

    // Assign a new pilot_slot
    pilot_data->pilot_slot = pilot_num;
    flight_ptr->num_pilots++;

    // Insert it into the list properly
    prev_pilot = flight_ptr->pilot_list;

    while (prev_pilot and prev_pilot->next_pilot and prev_pilot->next_pilot->pilot_slot <= pilot_data->pilot_slot)
        prev_pilot = prev_pilot->next_pilot;

    if (prev_pilot)
    {
        pilot_data->next_pilot = prev_pilot->next_pilot;
        prev_pilot->next_pilot = pilot_data;
    }
    else
    {
        flight_ptr->pilot_list = pilot_data;
        pilot_data->next_pilot = NULL;
    }

    pilot_data->aircraft_slot = ac_num;

    if (ac_num == pilot_num)
        pilot_data->pilot_id = flight->GetPilotID(ac_num);
    else
        pilot_data->pilot_id = -1; // Player Pilot

    pilot_data->pilot_status = PILOT_IN_USE;

    for (k = 0; k < (HARDPOINT_MAX / 2) + 2; k++)
    {
        pilot_data->weapon_data[k].root_event = NULL;
        pilot_data->weapon_data[k].events = 0;
    }

    // Check for player defined loadouts..
    loadout = flight->GetLoadout(ac_num);

    // Move to constructor
    // memset(pilot_data->weapon_data,0,sizeof(WeaponDataClass)*(HARDPOINT_MAX/2)+1);
    for (nw = 0, w = 0; w < HARDPOINT_MAX; w++)
    {
        wid = flight->GetUnitWeaponId(w, ac_num);

        if ( not wid)
            continue;

        wi = 0;

        /* // JB 010104 Marco Edit
         // Check if LAU3A - if so set to 2.75in FFAR
         if (wid == 71)
         wid = 163 ;
         // Check if UB-38-57 or UB-19-57 - if so set to 57mm S5 Rocket
         if (wid == 93 or wid == 94)
         wid = 163; //wid = 181 ;
         // JB 010104 Marco Edit
        */

        // 2002-04-14 MN use RocketDataType instead of hack
        bool entryfound = false;

        for (int j = 0; j < NumRocketTypes; j++)
        {
            if (wid == RocketDataTable[j].weaponId)
            {
                if (RocketDataTable[j].nweaponId) // 0 = don't change weapon ID
                    wid = RocketDataTable[j].nweaponId;

                entryfound = true;
                break;
            }
        }

        // 2002-04-16 MN sh*t... what should that do here ? copy and paste error...
        /* if ( not entryfound) // use generic 2.75mm rocket
         {
         wid = gRocketId;
         }
        */
        for (k = HARDPOINT_MAX / 2 + 1; k >= 0; k--)
        {
            if (pilot_data->weapon_data[k].weapon_id == wid)
            {
                wi = k;
                new_weap = 0;
            }

            if ( not pilot_data->weapon_data[k].weapon_id)
            {
                wi = k;
                new_weap = 1;
            }
        }

        if (new_weap)
        {
            nw++;
            _stprintf(pilot_data->weapon_data[wi].weapon_name, WeaponDataTable[wid].Name);
            pilot_data->weapon_data[wi].weapon_id = wid;
        }

        load = flight->GetUnitWeaponCount(w, ac_num);

        if (WeaponDataTable[wid].Flags bitand WEAP_ONETENTH)
            pilot_data->weapon_data[wi].starting_load += 10 * load;
        else
            pilot_data->weapon_data[wi].starting_load += load;
    }

    pilot_data->weapon_types = nw;

    return pilot_data;
}

PilotDataClass* MissionEvaluationClass::FindPilotData(FlightDataClass *flight_ptr, int pilot_num)
{
    PilotDataClass *pilot_data;

    CampEnterCriticalSection();

    pilot_data = flight_ptr->pilot_list;

    while ((pilot_data) and (pilot_data->pilot_slot not_eq pilot_num))
        pilot_data = pilot_data->next_pilot;

    CampLeaveCriticalSection();
    return pilot_data;
}

PilotDataClass* MissionEvaluationClass::FindPilotData(int flight_id, int pilot_num)
{
    FlightDataClass *flight_ptr;
    PilotDataClass *retval = NULL;

    CampEnterCriticalSection();

    flight_ptr = flight_data;

    while (flight_ptr and not retval)
    {
        if (flight_ptr->camp_id == flight_id)
            retval = FindPilotData(flight_ptr, pilot_num);

        flight_ptr = flight_ptr->next_flight;
    }

    CampLeaveCriticalSection();
    return retval;
}

PilotDataClass* MissionEvaluationClass::FindPilotDataFromAC(FlightDataClass *flight_ptr, int aircraft_slot)
{
    // Find the current pilot for this aircraft/flight combo.
    PilotDataClass *pilot_data = NULL;
    PilotDataClass *ret_data = NULL;

    if ( not flight_ptr) // JB 010628 CTD
        return NULL;

    CampEnterCriticalSection();

    // Since several players can have the same ac number, we take the last we find.
    pilot_data = flight_ptr->pilot_list;

    while (pilot_data)
    {
        if (pilot_data->aircraft_slot == aircraft_slot)
            ret_data = pilot_data;

        pilot_data = pilot_data->next_pilot;
    }

    CampLeaveCriticalSection();
    return ret_data;
}

FlightDataClass* MissionEvaluationClass::FindFlightData(Flight flight)
{
    FlightDataClass *flight_ptr;

    CampEnterCriticalSection();

    flight_ptr = TheCampaign.MissionEvaluator->flight_data;

    while (flight_ptr)
    {
        if (flight_ptr->flight_id == flight->Id())
        {
            CampLeaveCriticalSection();
            return flight_ptr;
        }

        flight_ptr = flight_ptr->next_flight;
    }

    CampLeaveCriticalSection();
    return NULL;
}

void MissionEvaluationClass::SetupPilots(FlightDataClass *flight_ptr, Flight flight)
{
    int i, p;
    FalconSessionEntity *session;

    CampEnterCriticalSection();

    if (FalconLocalGame->GetGameType() == game_Dogfight)
    {
        // Add all pilots
        for (i = 0; i < PILOTS_PER_FLIGHT; i++)
        {
            if (flight->plane_stats[i] == AIRCRAFT_AVAILABLE)
            {
                session = gCommsMgr->FindCampaignPlayer(flight->Id(), i);

                if (session)
                    AddNewPlayerPilot(flight_ptr, i, flight, session);
                else
                {
                    PilotDataClass *pilot_data = AddNewPilot(flight_ptr, i, i, flight);
                    // AI Pilots named by callsign
                    _stprintf(pilot_data->pilot_name, "%s%d", flight_ptr->name, i + 1);
                    _stprintf(pilot_data->pilot_callsign, "%s%d", flight_ptr->name, i + 1);
                }
            }
        }
    }
    else
    {
        // Add all default pilots
        for (i = 0; i < PILOTS_PER_FLIGHT; i++)
        {
            if (flight->plane_stats[i] == AIRCRAFT_AVAILABLE)
            {
                PilotDataClass *pilot_data = AddNewPilot(flight_ptr, i, i, flight);
                p = flight->GetPilotID(i);
                ::GetPilotName(p, pilot_data->pilot_name, 29);
                _stprintf(pilot_data->pilot_callsign, "%s%d", flight_ptr->name, i + 1);
            }
        }

        // Now add any current player pilots
        for (i = 0; i < PILOTS_PER_FLIGHT; i++)
        {
            if (flight->plane_stats[i] == AIRCRAFT_AVAILABLE)
            {
                session = gCommsMgr->FindCampaignPlayer(flight->Id(), i);

                if (session)
                    AddNewPlayerPilot(flight_ptr, i, flight, session);
            }
        }
    }

    CampLeaveCriticalSection();
}

void MissionEvaluationClass::SetPlayerPilot(Flight flight, uchar aircraft_slot)
{
    Flight element;
    FlightDataClass *flight_ptr;

    // KCK: Currently, this function isn't being called.
    CampEnterCriticalSection();
    CleanupPilotData();

    flight_ptr = flight_data;

    while (flight_ptr)
    {
        element = (Flight)FindUnit(flight_ptr->flight_id);

        if (element and flight_ptr->camp_id)
        {
            if (element == flight)
                player_element = flight_ptr;

            flight_ptr->num_pilots = 0;
            SetupPilots(flight_ptr, element);
        }
        else
            flight_ptr->camp_id = 0;

        flight_ptr = flight_ptr->next_flight;
    }

    CampLeaveCriticalSection();
}

void MissionEvaluationClass::RebuildEvaluationData(void)
{
    // KCK: This function will traverse all our data structures, throwing out any which
    // we don't need anymore and adding any new ones which have popped up

    // NOTE: Dogfight only
    if ( not FalconLocalGame or FalconLocalGame->GetGameType() not_eq game_Dogfight)
    {
        return;
    }
    else
    {
        Unit uelement;
        FlightDataClass *flight_ptr, *last_ptr = NULL, *tmp_ptr;
        PilotDataClass *pilot_data, *last_pilot, *tmp_pilot;
        Flight flight;
        int i, kill;
        static int evalCount = 0;
        FalconSessionEntity *session;

        ShiAssert(flight_data == NULL or FALSE == F4IsBadReadPtr(flight_data, sizeof * flight_data));

        // sfr @todo remove JB check
        if (flight_data and F4IsBadReadPtr(flight_data, sizeof(FlightDataClass))) // JB 010305 CTD
            return;

        CampEnterCriticalSection();

        // Traverse all our lists and remove anything we don't have references to anymore
        flight_ptr = flight_data;

        while (flight_ptr)
        {
            flight = (Flight)FindUnit(flight_ptr->flight_id);

            if ( not flight)
            {
                if (last_ptr)
                    last_ptr->next_flight = flight_ptr->next_flight;
                else
                    flight_data = flight_ptr->next_flight;

                tmp_ptr = flight_ptr;

                if (player_element == tmp_ptr)
                    player_element = NULL;

                if (package_element == tmp_ptr)
                    package_element = NULL;

                flight_ptr = flight_ptr->next_flight;
                delete tmp_ptr;
            }
            else
            {
                // Check for team change
                flight_ptr->flight_team = flight->GetTeam();
                // Check pilots
                last_pilot = NULL;
                pilot_data = flight_ptr->pilot_list;

                while (pilot_data)
                {
                    kill = 0;

                    if (flight->pilots[pilot_data->aircraft_slot] == NO_PILOT and flight->player_slots[pilot_data->aircraft_slot] == NO_PILOT)
                        kill++; // Neither slot
                    else if (flight->pilots[pilot_data->aircraft_slot] not_eq NO_PILOT and flight->player_slots[pilot_data->aircraft_slot] == NO_PILOT and pilot_data->pilot_slot not_eq pilot_data->aircraft_slot)
                        kill++; // Player in AI slot
                    else if (flight->player_slots[pilot_data->aircraft_slot] not_eq NO_PILOT and pilot_data->pilot_slot == pilot_data->aircraft_slot)
                        kill++; // AI in player slot

                    if (kill)
                    {
                        if (last_pilot)
                            last_pilot->next_pilot = pilot_data->next_pilot;
                        else
                            flight_ptr->pilot_list = pilot_data->next_pilot;

                        tmp_pilot = pilot_data;

                        if (player_pilot == tmp_pilot)
                        {
#ifdef FUNKY_KEVIN_DEBUG_STUFF
                            ShiAssert( not inMission);
#endif
                            player_pilot = NULL;
                        }

                        pilot_data = pilot_data->next_pilot;
                        delete tmp_pilot;
                    }
                    else
                    {
                        last_pilot = pilot_data;
                        pilot_data = pilot_data->next_pilot;
                    }
                }

                last_ptr = flight_ptr;
                flight_ptr = flight_ptr->next_flight;
            }
        }

        // Now add any additional flights which matter
        {
            VuListIterator flit(AllAirList);
            uelement = (Unit) flit.GetFirst();

            while (uelement)
            {
                if (uelement->IsFlight())
                {
                    flight = (FlightClass*)uelement;
                    flight_ptr = FindFlightData(flight);

                    if (flight_ptr)
                    {
                        // Already exists - just check for new players
                        for (i = 0; i < PILOTS_PER_FLIGHT; i++)
                        {
                            if (flight->plane_stats[i] == AIRCRAFT_AVAILABLE and not FindPilotDataFromAC(flight_ptr, i))
                            {
                                // Add this pilot
                                session = FindPlayer((Flight)uelement, i);

                                if (session)
                                    AddNewPlayerPilot(flight_ptr, i, (Flight)uelement, session);
                                else
                                {
                                    PilotDataClass *pilot_data = AddNewPilot(flight_ptr, i, i, flight);
                                    // AI Pilots named by callsign
                                    _stprintf(pilot_data->pilot_name, "%s%d", flight_ptr->name, i + 1);
                                    _stprintf(pilot_data->pilot_callsign, "%s%d", flight_ptr->name, i + 1);
                                }
                            }
                        }
                    }
                    else
                    {
                        // Add the whole thing
                        PreEvalFlight(flight, NULL);
                    }

                    uelement->SetInPackage(1);
                }

                uelement = (Unit) flit.GetNext();
            }
        }

        // evalCount++;
        // if (evalCount > 500 and not (SimDogfight.flags bitand DF_GAME_OVER))
        // {
        // // Periodically send any evaluation data we may have
        // SendAllEvalData();
        // }

        CampLeaveCriticalSection();
    }
}

// =========================
// Local functions
// =========================

Objective FindAlternateLandingStrip(Flight flight)
{
    WayPoint w;
    Objective as;
    GridIndex x, y;

    w = flight->GetFirstUnitWP();

    // Find the alternate waypoint, if we've got one
    while (w and not (w->GetWPFlags() bitand WPF_ALTERNATE))
    {
        w = w->GetNextWP();
    }

    if (w)
    {
        w->GetWPLocation(&x, &y);
        as = GetObjectiveByXY(x, y);

        if (as)
        {
            return as;
        }
    }

    return NULL;
}

void CheckForNewPlayer(FalconSessionEntity *session)
{
    Flight pflight = session->GetPlayerFlight();
    uchar pslot = session->GetPilotSlot();
    uchar acnum = session->GetAircraftNum();
    PilotDataClass *pilot_data;
    FlightDataClass *flight_ptr;

    if ( not pflight or pslot == 255 or acnum > PILOTS_PER_FLIGHT)
        return;

    if ((session) and (session->GetGame() not_eq FalconLocalGame))
    {
        MonoPrint("Session is not in this game\n");
        return;
    }

    CampEnterCriticalSection();

    // FRB - CTD's here
    flight_ptr = TheCampaign.MissionEvaluator->flight_data;

    while (flight_ptr)
    {
        if (flight_ptr->flight_id == pflight->Id())
        {
            pilot_data = flight_ptr->pilot_list;

            while (pilot_data)
            {
                if (pilot_data->pilot_slot == pslot)
                {
                    pilot_data->aircraft_slot = acnum;
                    CampLeaveCriticalSection();
                    return;
                }

                pilot_data = pilot_data->next_pilot;
            }

            TheCampaign.MissionEvaluator->AddNewPlayerPilot(flight_ptr, acnum, pflight, session);
            CampLeaveCriticalSection();
            return;
        }

        flight_ptr = flight_ptr->next_flight;
    }

    // KCK: I don't think this line is correct.
    // TheCampaign.MissionEvaluator->PreEvalFlight(pflight,NULL);
    CampLeaveCriticalSection();
}

// ==========================================
// Runtime mission evaluation routines
// ==========================================

/*
int OverFriendlyTerritory (Flight flight)
 {
 GridIndex x,y;

 flight->GetLocation(&x,&y);
 if (GetOwner(TheCampaign.CampMapData, x, y)  == flight->GetTeam())
 return 1;
 return 0;
 }
*/

/*
void SetMissionStarted (Flight flight)
 {
 if (flight)
 flight->SetEvalFlag (FEVAL_MISSION_STARTED);
 }

void SetAtTarget (Flight flight)
 {
 if (flight)
 {
 flight->SetEvalFlag (FEVAL_GOT_TO_TARGET);
 if (flight->InPackage())
 TheCampaign.MissionEvaluator->SetAtTarget(flight->GetCampID());
 }
 }

void SetTargetStatus (Flight flight)
 {
 if (flight and flight->InPackage())
 TheCampaign.MissionEvaluator->SetTargetStatus();
 }

void SetMissionOver (Flight flight)
 {
 if (flight)
 flight->SetEvalFlag (FEVAL_MISSION_OVER);
 }

void SetMissionComplete (Flight flight)
 {
 if (flight)
 {
 flight->SetEvalFlag (FEVAL_MISSION_FINISHED);
 if (flight->InPackage())
 TheCampaign.MissionEvaluator->SetMissionComplete(flight->GetCampID());
 }
 }
*/

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int ScoreCampaign(score_type type, int index)
{
    switch (type)
    {
        case SCORE_FIRE_WEAPON:
        {
            return -1;
        }

        case SCORE_GROUND_COLLISION:
        case SCORE_FEATURE_COLLISION:
        case SCORE_VEHICLE_COLLISION:
        case SCORE_DEBREE_KILL:
        case SCORE_BOMB_KILL:
        {
            return -25;
        }

        case SCORE_GROUND_COLLISION_KILL:
        case SCORE_VEHICLE_KILL:
        {
            return -15;
        }

        case SCORE_PILOT_EJECTED:
        case SCORE_PILOT_EJECTED_KILL:
        {
            return 10; // We ejected from a damaged airplane
        }

        case SCORE_PILOT_LOST:
        case SCORE_PILOT_FOUND:
        {
            return 0;
        }

        case SCORE_PILOT_LANDED:
        {
            return 2;
        }

        case SCORE_HIT_ENEMY:
        {
            return 1;
        }

        case SCORE_HIT_FRIENDLY:
        {
            return -2;
        }

        case SCORE_KILL_ENEMY:
        {
            return 4;
        }

        case SCORE_KILL_FRIENDLY:
        {
            return -15;
        }

        case SCORE_KILL_ENEMY_FEATURE:
        {
            if (index > 49) //  or (value > 0 and o->Id() == package_element->target_id))
                return 4; // High value or at our target
            else if (index > 0)
                return 2; // Moderate value
            else
                return 0;
        }

        case SCORE_KILL_FRIENDLY_FEATURE:
        {
            return -10;
        }

        case SCORE_KILL_ENEMY_NAVAL:
        {
            return 3 + index;
        }

        case SCORE_KILL_FRIENDLY_NAVAL:
        {
            return (-10 - index);
        }

        case SCORE_KILL_ENEMY_GROUND:
        {
            return 2;
        }

        case SCORE_KILL_FRIENDLY_GROUND:
        {
            return -8;
        }

        case SCORE_DIRECT_KILL_GROUND:
        {
            return 2;
        }

        case SCORE_HIT_OUR_TARGET:
        {
            return 2;
        }
    }

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int ScoreDogfight(score_type type, int index)
{
    MonoPrint("ScoreDogfight %d %d\n", type, index);

    switch (type)
    {
        case SCORE_KILL_ENEMY:
        {
            gRefreshScoresList = 1;
            return 1;
        }

        case SCORE_KILL_FRIENDLY:
        case SCORE_GROUND_COLLISION:
        case SCORE_FEATURE_COLLISION:
        case SCORE_VEHICLE_COLLISION:
        case SCORE_DEBREE_KILL:
        case SCORE_BOMB_KILL:
            // case SCORE_PILOT_EJECTED: // KCK: This will be scored when the ground collision comes through
        {
            gRefreshScoresList = 1;
            return -1;
        }
    }

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int ScoreInstantAction(score_type type, int index)
{
    switch (type)
    {
        case SCORE_FIRE_WEAPON:
        {
            return AddWeaponToUsageList(index);
        }

        case SCORE_KILL_ENEMY:
        {
            return AddAircraftToKillsList(index);
        }

        case SCORE_KILL_FRIENDLY:
        {
            return AddAircraftToKillsList(index);
        }

        case SCORE_KILL_ENEMY_GROUND:
        {
            return AddObjectToKillsList(index);
        }

        case SCORE_KILL_FRIENDLY_GROUND:
        {
            return AddObjectToKillsList(index);
        }

        case SCORE_PILOT_EJECTED:
        {
            return score_player_ejected();;
        }
    }

    return 1;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int CalcScore(score_type type, int index)
{
    switch (FalconLocalGame->GetGameType())
    {
        case game_InstantAction:
        {
            return ScoreInstantAction(type, index);
        }

        case game_Dogfight:
        {
            return ScoreDogfight(type, index);
        }

        case game_TacticalEngagement:
        case game_Campaign:
        {
            return ScoreCampaign(type, index);
        }
    }

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// Update remote scores for this guy, if necessary
void UpdateEvaluators(FlightDataClass *flight_data, PilotDataClass *pilot_data)
{
    // First, check if this is the session who is tracking this record
    FalconSessionEntity *session = NULL;
    FlightDataClass *flight_ptr;
    PilotDataClass *pilot_ptr;

    CampEnterCriticalSection();

    // Check if this record is the local player
    if (pilot_data->pilot_slot not_eq NO_PILOT)
        session = FindPlayer(flight_data->flight_id, pilot_data->aircraft_slot, pilot_data->pilot_slot);

    if ((session) and (session->GetGame() not_eq FalconLocalGame))
    {
        MonoPrint("Session not in this local game\n");
        CampLeaveCriticalSection();
        return;
    }

    if ( not session and FalconLocalGame and FalconLocalGame->GetGameType() == game_Dogfight)
    {
        // For Dogfight games, host controls all data
        if (FalconLocalGame->IsLocal())
            session = FalconLocalSession;
    }
    else if ( not session)
    {
        // For Campaign or TacE, the first player in the package controls the data
        flight_ptr = TheCampaign.MissionEvaluator->flight_data;

        while ( not session and flight_ptr)
        {
            // The tracking session will be the first player in the package
            pilot_ptr = flight_ptr->pilot_list;

            while ( not session and pilot_ptr)
            {
                if (pilot_ptr->pilot_slot not_eq NO_PILOT)
                    session = FindPlayer(flight_ptr->flight_id, pilot_ptr->aircraft_slot, pilot_ptr->pilot_slot);

                pilot_ptr = pilot_ptr->next_pilot;
            }

            flight_ptr = flight_ptr->next_flight;
        }
    }

    if (session == FalconLocalSession)
        SendEvalData(flight_data, pilot_data);

    CampLeaveCriticalSection();
}

// KCK: Start recording flight hours
void RecordPlayerFlightStart(void)
{
    TheCampaign.MissionEvaluator->player_start_time = vuxGameTime;
}
