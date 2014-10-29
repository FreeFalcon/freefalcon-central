//
// Mission Evaluation Class
//
// Fills itself with data upon request by analysing the event list and other campaign data
//

#ifndef MISSEVAL_H
#define MISSEVAL_H

#include <tchar.h>
#include "flight.h"
#include "EvtParse.h"
#include "Mesg.h"
#include "Dogfight.h"
#include "ui/include/CampMiss.h"
#include "cmpclass.h"
//#include "CampWeap.h" //sfr: this thing is giving tons of compile errors
/*#include "Flight.h"

 */
// ====================================
// Some defines (maybe move to AIInput)
// ====================================

#define MAX_TARGET_FEATURES 5
#define MAX_POTENTIAL_TARGETS 5
#define MAX_COLLECTED_THREATS 5
#define MAX_RELATED_EVENTS 5
#define MINIMUM_VIABLE_THREAT 9

// Flight specific status flags
#define MISEVAL_FLIGHT_LOSSES 0x00000001
#define MISEVAL_FLIGHT_DESTROYED 0x00000002
#define MISEVAL_FLIGHT_ABORTED 0x00000004
#define MISEVAL_FLIGHT_GOT_AKILL 0x00000010 // Air kill
#define MISEVAL_FLIGHT_GOT_GKILL 0x00000020 // Ground kill
#define MISEVAL_FLIGHT_GOT_NKILL 0x00000040 // Naval kill
#define MISEVAL_FLIGHT_GOT_SKILL 0x00000080 // Static kill
#define MISEVAL_FLIGHT_HIT_HIGH_VAL 0x00000100 // Hit a high value target (Feature with value)
#define MISEVAL_FLIGHT_HIT_BY_AIR 0x00001000 // Suffered loss to air (during ingress only)
#define MISEVAL_FLIGHT_HIT_BY_GROUND 0x00002000 // Suffered loss to ground (during ingress only)
#define MISEVAL_FLIGHT_HIT_BY_NAVAL 0x00004000 // Suffered loss to naval (during ingress only)
#define MISEVAL_FLIGHT_TARGET_HIT 0x00010000 // We hit our target
#define MISEVAL_FLIGHT_TARGET_KILLED 0x00020000 // We killed out target
#define MISEVAL_FLIGHT_TARGET_ABORTED 0x00040000 // We forced our target to abort
#define MISEVAL_FLIGHT_AREA_HIT 0x00080000 // We hit an enemy in our target area
#define MISEVAL_FLIGHT_F_TARGET_HIT 0x00100000 // The friendly we were assigned to was hit
#define MISEVAL_FLIGHT_F_TARGET_KILLED 0x00200000 // The friendly we were assigned to was killed
#define MISEVAL_FLIGHT_F_TARGET_ABORTED 0x00400000 // The friendly we were assigned to aborted
#define MISEVAL_FLIGHT_F_AREA_HIT 0x00800000 // Our friendly target region was hit
#define MISEVAL_FLIGHT_STARTED_LATE 0x01000000 // This mission wasn't started in time to count full
#define MISEVAL_FLIGHT_GOT_TO_TARGET 0x02000000 // Flight got to target area
#define MISEVAL_FLIGHT_STATION_OVER 0x04000000 // Station/mission time is over
#define MISEVAL_FLIGHT_GOT_HOME 0x08000000 // We returned to friendly territory
#define MISEVAL_FLIGHT_RELIEVED 0x10000000 // Flight was allowed to leave by AWACS/FAC
#define MISEVAL_FLIGHT_OFF_STATION 0x20000000 // Flight left its station area

// 2002-02-13 MN
#define MISEVAL_FLIGHT_ABORT_BY_AWACS 0x40000000 // flight aborted by AWACS instruction - we occupied the target

// Mission Evaluator status flags
#define MISEVAL_MISSION_IN_PROGRESS 0x01 // We want to start evaluating stuff
#define MISEVAL_EVALUATE_HITS_IN_AREA 0x02 // Check to see if someone has bombed our area
#define MISEVAL_GAME_COMPLETED 0x04 // We finished a dogfight game
#define MISEVAL_ONLINE_GAME 0x08 // This was a multi-player game

// Pilot flags
#define PFLAG_PLAYER_CONTROLLED 0x01 // Pilot is a player
#define PFLAG_WON_GAME 0x02 // Pilot is on the winning side

// Kill tracking vs..
#define VS_AI 0
#define VS_HUMAN 1
#define VS_EITHER 2

class FalconWeaponsFire;
class FalconDamageMessage;
class FalconDeathMessage;
class FalconPlayerStatusMessage;
class FalconEjectMessage;
class FalconDivertMessage;
class FalconLandingMessage;
class CampaignClass; //sfr: added for cyclic dependencies

//this is defined in CmpClass.c
extern CampaignClass TheCampaign;

// ===================================
// success enums
// ===================================

enum SuccessType
{
    Failed,
    PartFailed,
    PartSuccess,
    Success,
    Incomplete,
    AWACSAbort // 2002-02-15 MN Added
};

enum RatingType
{
    Horrible,
    Poor,
    Average,
    Good,
    Excellent
};

// ===================================
// Data Storage
// ===================================

class WeaponDataClass
{
public:
    _TCHAR weapon_name[20];
    short weapon_id;
    short starting_load;
    uchar fired;
    uchar missed;
    uchar hit;
    short events; // number of events
    EventElement *root_event; // List of relevant events
public:
    WeaponDataClass(void);
    ~WeaponDataClass();
};

class PilotDataClass
{
public:
    _TCHAR pilot_name[30];
    _TCHAR pilot_callsign[30];
    uchar aircraft_slot; // Which aircraft we're occupying
    uchar pilot_slot; // Pilot's position in the pilot_list
    short pilot_id; // ID of the pre-assigned non-player pilot
    uchar pilot_flags;
    uchar pilot_status;
    uchar aircraft_status;
    uchar aa_kills;
    uchar ag_kills;
    uchar as_kills;
    uchar an_kills;
    uchar player_kills;
    uchar shot_at; // Times this player was shot at (only tracks for local player)
    short deaths[VS_EITHER]; // Dogfight statistics [AI/PLAYER]
    short score;
    uchar rating;
    uchar weapon_types; // number of actually different weapons
    bool donefiledebrief; //me123 has a file debrief already been made
    WeaponDataClass weapon_data[(HARDPOINT_MAX / 2) + 2]; // Weapon data for this pilot/aircraft
    PilotDataClass *next_pilot;
public:
    PilotDataClass(void);
    ~PilotDataClass();
};

class FlightDataClass
{
public:
    _TCHAR name[30];
    ulong status_flags;
    short camp_id;
    VU_ID flight_id;
    _TCHAR aircraft_name[10];
    uchar start_aircraft;
    uchar finish_aircraft;
    Team flight_team;
    uchar mission;
    uchar old_mission; // Old mission, if we were diverted
    VU_ID requester_id; // ID of entity which caused this mission
    VU_ID target_id;
    short target_camp_id;
    uchar target_building;
    GridIndex target_x;
    GridIndex target_y;
    uchar target_features[MAX_TARGET_FEATURES];
    uchar target_status;
    uchar mission_context;
    uchar mission_success;
    uchar failure_code;
    short failure_data;
    VU_ID failure_id;
    uchar num_pilots;
    _TCHAR context_entity_name[40]; // Name of the entity this mission was about
    PilotDataClass *pilot_list;
    short events; // number of events
    EventElement *root_event; // List of relevant events
    FlightDataClass *next_flight;

public:
    FlightDataClass(void);
    ~FlightDataClass();
};

class MissionEvaluationClass
{
public:
    ulong contact_score; // Total AA score of all actual contacts
    uchar pack_success; // Package success
    uchar package_mission;
    uchar package_context;
    FlightDataClass *package_element;
    FlightDataClass *player_element;
    PilotDataClass *player_pilot;
    uchar player_aircraft_slot;
    uchar friendly_losses;
    uchar friendly_aa_losses;
    uchar friendly_ga_losses;
    uchar action_type;
    Team team;
    short flags;
    short responses;
    short threat_ids[MAX_COLLECTED_THREATS]; // weapon ids of threats
    GridIndex threat_x[MAX_COLLECTED_THREATS];
    GridIndex threat_y[MAX_COLLECTED_THREATS];
    VU_ID alternate_strip_id;
    VU_ID requesting_ent;
    VU_ID intercepting_ent;
    VU_ID awacs_id;
    VU_ID jstar_id;
    VU_ID ecm_id;
    VU_ID tanker_id;
    VU_ID package_target_id;
    VU_ID potential_targets[MAX_POTENTIAL_TARGETS];
    CampaignTime assigned_tot;
    CampaignTime actual_tot;
    CampaignTime patrol_time;
    CampaignTime player_start_time; // Time player started flying the mission
    CampaignTime player_end_time; // Time player stopped flying the mission
    GridIndex tx, ty, abx, aby, awx, awy, jsx, jsy, tankx, tanky;
    FlightDataClass *flight_data;
    short curr_data; // Floating data point
    uchar curr_flight; // What flight we're talking about
    uchar curr_weapon; // What weapon we're talking about
    PilotDataClass *curr_pilot; // What pilot we're talking about
    uchar parse_types[(LastFalconEvent + 7) / 8]; // messages types to parse
    uchar rounds_won[MAX_DOGFIGHT_TEAMS]; // Dogfight rounds won
    uchar last_related_event;
    _TCHAR *related_events[MAX_RELATED_EVENTS];
    CAMP_MISS_STRUCT logbook_data; // Structure we'll pass to the logbook
public:
    MissionEvaluationClass(void);
    ~MissionEvaluationClass(void);

    void CleanupFlightData(void);
    void CleanupPilotData(void);
    void PreDogfightEval(void);
    int PreMissionEval(Flight flight, uchar aircraft_slot);
    void PreEvalFlight(Flight element, Flight flight);
    void RecordTargetStatus(FlightDataClass *flight_ptr, CampBaseClass *target);
    int PostMissionEval(void);
    void ServerFileLog(FalconPlayerStatusMessage *fpsm);
    int MissionSuccess(FlightDataClass *flight_ptr);
    void SetPackageData(void);
    void ClearPackageData(void);
    void ClearPotentialTargets(void);
    void FindPotentialTargets(void);
    void CollectThreats(Flight flight, WayPoint tw);
    void CollectThreats(GridIndex X, GridIndex Y, int Z, int flags, int *dists);

    // Dogfight kill evalutators
    void GetTeamKills(short *kills);
    void GetTeamDeaths(short *deaths);
    void GetTeamScore(short *score);
    int GetKills(FalconSessionEntity *player);
    int GetMaxKills(void);
    int GetMaxScore(void);

    // Event list builders
    void RegisterDivert(FalconDivertMessage *dm);
    void RegisterShotAtPlayer(FalconWeaponsFire *wfm, unsigned short CampID, uchar fPilotID);
    void RegisterShot(FalconWeaponsFire *wfm);
    void RegisterHit(FalconDamageMessage *dmm);
    void RegisterKill(FalconDeathMessage *dtm, int type, int pilot_status);
    void RegisterPlayerJoin(FalconPlayerStatusMessage *fpsm);
    void RegisterEjection(FalconEjectMessage *em, int pilot_status);
    void RegisterLanding(FalconLandingMessage *lm, int pilot_status);
    void RegisterContact(Unit contact);
    void ParseTime(CampaignTime time, char* time_str);
    void ParseTime(double time, char* time_str);
    void AddEventToList(EventElement* theEvent, FlightDataClass *flight_ptr, PilotDataClass* pilot_data, int wn);

    // Event trigger checks
    void RegisterKill(FalconEntity *shooter, FalconEntity *target, int targetEl);
    void RegisterMove(Flight flight);
    void RegisterAbort(Flight flight);
    void RegisterRelief(Flight flight);
    void RegisterDivert(Flight flight, MissionRequestClass *mis);
    void RegisterRoundWon(int team);
    void RegisterWin(int team);
    void RegisterEvent(GridIndex x, GridIndex y, int team, int type, _TCHAR *event);

    // Register 3D AWACS abort call
    void Register3DAWACSabort(Flight flight);

    void SetFinalAircraft(Flight flight);

    int GetPilotName(int pilot_num, TCHAR* buffer);
    int GetFlightName(TCHAR* buffer);

    PilotDataClass* AddNewPlayerPilot(FlightDataClass *flight_ptr, int pilot_num, Flight flight, FalconSessionEntity *player);
    PilotDataClass* AddNewPilot(FlightDataClass *flight_ptr, int pilot_num, int ac_num, Flight flight);
    PilotDataClass* FindPilotData(FlightDataClass *flight_ptr, int pilot_num);
    PilotDataClass* FindPilotData(int flight_id, int pilot_num);
    PilotDataClass* FindPilotDataFromAC(FlightDataClass *flight_ptr, int aircraft_slot);

    FlightDataClass* FindFlightData(Flight flight);

    void SetupPilots(FlightDataClass *flight_ptr, Flight flight);
    void SetPlayerPilot(Flight flight, uchar aircraft_slot);

    void RebuildEvaluationData(void);

};

// ===========================================================
// class used for temporary pilot list building/sorting
// ===========================================================

class PilotSortClass
{
public:
    short team;
    PilotDataClass *pilot_data;
    PilotSortClass *next;

    PilotSortClass(PilotDataClass *pilot_ptr)
    {
        pilot_data = pilot_ptr;
        next = NULL;
    };
};

// =============================
// Global setter/query functions
// =============================

int OverFriendlyTerritory(Flight flight);

#endif
