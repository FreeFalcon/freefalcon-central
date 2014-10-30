// ***************************************************************************
// Team.h
//
// Team related variables and routines
// ***************************************************************************

#include <tchar.h>
#include "listadt.h"
#include "FalcMesg.h"
#include "Mission.h"
#include "FalcEnt.h"
#include "F4Vu.h"
#include "MsgInc/TeamMsg.h"

#ifndef TEAM_H
#define TEAM_H

// Country defines
enum CountryListEnum
{
    COUN_NONE = 0,
    COUN_US,
    COUN_SOUTH_KOREA,
    COUN_JAPAN,
    COUN_RUSSIA,
    COUN_CHINA,
    COUN_NORTH_KOREA,
    COUN_GORN,
    NUM_COUNS,
};

// Team data defines
enum TeamDataEnum
{
    TEAM_NEUTRAL = 0,
    TEAM_1,
    TEAM_2,
    TEAM_3,
    TEAM_4,
    TEAM_5,
    TEAM_6,
    TEAM_7,
    NUM_TEAMS,
};

#define MAX_TEAM_NAME_SIZE 20
#define MAX_MOTTO_SIZE 200
#define MAX_AIR_ACTIONS 14
#define ACTION_TIME_MIN 30 // Time per action, in minutes

// Team flags
enum TeamFlagEnum
{
    TEAM_ACTIVE         = 0x01, // Set if team is being used
    TEAM_HASSATS        = 0x02, // Has satelites
    TEAM_UPDATED        = 0x04, // We've gotten remote data for this team
};

// Rules of engagement query types, add as needed.
enum ROEEngagementQueryTypeEnum
{
    ROE_GROUND_FIRE     = 1, // Fire on their ground troops?
    ROE_GROUND_MOVE     = 2, // Move through their territory?
    ROE_GROUND_CAPTURE  = 3, // Capture their territory?
    ROE_AIR_ENGAGE      = 4, // Maneuver against their aircraft?
    ROE_AIR_FIRE        = 5, // Fire at their aircraft? (any range)
    ROE_AIR_FIRE_BVR    = 6, // Fire at their aircraft BVR
    ROE_AIR_OVERFLY     = 7, // Fly over their territory?
    ROE_AIR_ATTACK      = 8, // Bomb/attack their territory?
    ROE_AIR_USE_BASES   = 9, // Can we based aircraft at their airbases?
    ROE_NAVAL_FIRE      = 10, // Attack their shipping?
    ROE_NAVAL_MOVE      = 11, // Move into/through their harbors/straights?
    ROE_NAVAL_BOMBARD   = 12, // Bombard them messily?
};

enum ROEAllowedEnum
{
    ROE_ALLOWED         = 1,
    ROE_NOT_ALLOWED     = 0,
};

#define MAX_BONUSES 20 // Number of SOs which can receive bonuses at one time
#define MAX_TGTTYPE 36
#define MAX_UNITTYPE 20

// Ground Action types
enum GroundActionTypeEnum
{
    GACTION_DEFENSIVE      = 1,
    GACTION_CONSOLIDATE    = 2,
    GACTION_MINOROFFENSIVE = 3,
    GACTION_OFFENSIVE      = 4,
};

// Air Action types
enum AirActionTypeEnum
{
    AACTION_NOTHING        = 0,
    AACTION_DCA            = 1,
    AACTION_OCA            = 2,
    AACTION_INTERDICT      = 3,
    AACTION_ATTRITION      = 4,
    AACTION_CAS            = 5,
};

// Air tactic types
enum AirTacticTypeEnum
{
    TAT_DEFENSIVE          = 1,
    TAT_OFFENSIVE          = 2,
    TAT_INTERDICT          = 3,
    TAT_ATTRITION          = 4,
    TAT_CAS                = 5, // CAS must always be last tactic
};

enum table_of_equipment_manufacturers
{
    toe_unknown,
    toe_chinese,
    toe_dprk,
    toe_rok,
    toe_soviet,
    toe_us
};

// =======================================
// Priority tables
// =======================================

extern uchar DefaultObjtypePriority[TAT_CAS][MAX_TGTTYPE]; // AI's suggested settings
extern uchar DefaultUnittypePriority[TAT_CAS][MAX_UNITTYPE]; //
extern uchar DefaultMissionPriority[TAT_CAS][AMIS_OTHER]; //

// =======================================
// Local classes
// =======================================

class AirTaskingManagerClass;
class GroundTaskingManagerClass;
class NavalTaskingManagerClass;
class CampBaseClass;
typedef AirTaskingManagerClass* ATM;
typedef GroundTaskingManagerClass* GTM;
typedef NavalTaskingManagerClass* NTM;
typedef CampBaseClass* CampEntity;
Team GetTeam(Control country);

#pragma pack(1) // place on byte boundary
struct TeamStatusType
{
    ushort airDefenseVehs;
    ushort aircraft;
    ushort groundVehs;
    ushort ships;
    ushort supply;
    ushort fuel;
    ushort airbases;
    uchar supplyLevel; // Supply in terms of pecentage
    uchar fuelLevel; // fuel in terms of pecentage
};
#pragma pack()

#pragma pack(1) // place on byte boundary
struct TeamGndActionType
{
    CampaignTime actionTime; // When we start.
    CampaignTime actionTimeout; // Our action will fail if not completed by this time
    VU_ID actionObjective; // Primary objective this is all about
    uchar actionType;
    uchar actionTempo; // How "active" we want the action to be
    uchar actionPoints; // Countdown of how much longer it will go on
};
#pragma pack()

struct TeamAirActionType
{
    CampaignTime actionStartTime; // When we start.
    CampaignTime actionStopTime; // When we are supposed to be done by.
    VU_ID actionObjective; // Primary objective this is all about
    VU_ID lastActionObjective;
    uchar actionType;
};

class TeamDoctrine
{
public:
    int simFlags;
    float radarShootShootPct;
    float heatShootShootPct;

    TeamDoctrine(void)
    {
        simFlags = 0;
    }
    enum
    {
        SimRadarShootShoot = 0x1,
        SimHeatShootShoot  = 0x2
    };
    int IsSet(int val)
    {
        return simFlags bitand val;
    };
    void Set(int val)
    {
        simFlags or_eq val;
    };
    void Clear(int val)
    {
        simFlags and_eq compl val;
    };
    float RadarShootShootPct(void)
    {
        return radarShootShootPct;
    }
    float HeatShootShootPct(void)
    {
        return heatShootShootPct;
    }
};

// =============================================
// Team class
// =============================================

class TeamClass : public FalconEntity
{
private:
    short initiative;
    ushort supplyAvail;
    ushort fuelAvail;
    ushort replacementsAvail;
    TeamStatusType currentStats;
    short reinforcement;
    uchar objtype_priority[MAX_TGTTYPE]; // base priority, based on target type (obj)
    uchar unittype_priority[MAX_UNITTYPE]; // base priority for unit types (cmbt/AD)
    uchar mission_priority[AMIS_OTHER]; // bonus by mission type
    TeamGndActionType groundAction; // Team's current ground action
    TeamAirActionType defensiveAirAction; // Current defensive air action
    TeamAirActionType offensiveAirAction; // Current offensive air action
    int dirty_team;

public:
    Team  who;
    Team cteam; // The team this relative is on (for quick reference)
    short flags;
    _TCHAR name[MAX_TEAM_NAME_SIZE];
    _TCHAR teamMotto[MAX_MOTTO_SIZE];
    uchar member[NUM_COUNS];
    short stance[NUM_TEAMS];
    short firstColonel; // Pilot ID indexies for this country
    short firstCommander;
    short firstWingman;
    short lastWingman;
    float playerRating; // Average player rating over last 5 player missions
    CampaignTime lastPlayerMission; // Last player mission flown
    uchar airExperience; // Experience for aircraft (affects pilot's skill)
    uchar airDefenseExperience; // Experience for air defenses
    uchar groundExperience; // Experience for ground troops
    uchar navalExperience; // Experience for ships
    TeamStatusType startStats;
    VU_ID bonusObjs[MAX_BONUSES];
    CampaignTime bonusTime[MAX_BONUSES];
    uchar max_vehicle[4]; // Max vehicle slot by air/ground/airdefense/naval
    ATM atm;
    GTM gtm;
    NTM ntm;
    uchar teamFlag; // This team's flag (as in cloth)
    uchar teamColor; // This team's color [Index into color table for TE]
    uchar equipment; // What equipment table to use
    TeamDoctrine doctrine;

public:
    // Constructors
    TeamClass(int typeindex, Control owner);
    TeamClass(VU_BYTE **stream, long *rem);
    TeamClass(FILE *file);
    ~TeamClass(void);
    virtual void InitData();
private:
    void InitLocalData(Control owner);
public:
    // pure virtual interface
    virtual float GetVt() const
    {
        return 0;
    }
    virtual float GetKias() const
    {
        return 0;
    }

    // event handlers
    virtual int Handle(VuEvent *event);
    virtual int Handle(VuFullUpdateEvent *event);
    virtual int Handle(VuPositionUpdateEvent *event);
    virtual int Handle(VuEntityCollisionEvent *event);
    virtual int Handle(VuTransferEvent *event);
    virtual int Handle(VuSessionEvent *event);
    virtual VU_ERRCODE InsertionCallback(void);
    virtual VU_ERRCODE RemovalCallback(void);
    virtual int Wake(void)
    {
        return 0;
    };
    virtual int Sleep(void)
    {
        return 0;
    };

    // Access Functions
    short GetInitiative(void)
    {
        return initiative;
    }
    ushort GetSupplyAvail(void)
    {
        return supplyAvail;
    }
    ushort GetFuelAvail(void)
    {
        return fuelAvail;
    }
    ushort GetReplacementsAvail(void)
    {
        return replacementsAvail;
    }
    short GetReinforcement(void)
    {
        return reinforcement;
    }
    uchar GetObjTypePriority(int type)
    {
        return objtype_priority[type];
    }
    uchar GetUnitTypePriority(int type)
    {
        return unittype_priority[type];
    }
    uchar GetMissionPriority(int type)
    {
        return mission_priority[type];
    }
    TeamStatusType *GetCurrentStats(void)
    {
        return &currentStats;
    }
    TeamGndActionType *GetGroundAction(void)
    {
        return &groundAction;
    }
    TeamAirActionType *GetDefensiveAirAction(void)
    {
        return &defensiveAirAction;
    }
    TeamAirActionType *GetOffensiveAirAction(void)
    {
        return &offensiveAirAction;
    }
    virtual short GetCampID(void)
    {
        return 0;
    }
    virtual uchar GetTeam(void)
    {
        return 0;
    }
    virtual uchar GetCountry(void)
    {
        return 0;
    }

    void SetInitiative(short);
    void SetReinforcement(short);
    uchar *SetAllObjTypePriority(void);
    uchar *SetAllUnitTypePriority(void);
    uchar *SetAllMissionPriority(void);
    void SetObjTypePriority(int, uchar);
    void SetUnitTypePriority(int, uchar);
    void SetMissionPriority(int, uchar);
    void SetSupplyAvail(int);
    void SetFuelAvail(int);
    void SetReplacementsAvail(int);
    TeamStatusType *SetCurrentStats(void);
    TeamGndActionType *SetGroundAction(void);
    TeamAirActionType *SetDefensiveAirAction(void);
    TeamAirActionType *SetOffensiveAirAction(void);

    void AddInitiative(short);
    void AddReinforcement(short);

    // Core functions
    virtual int SaveSize(void);
    virtual int Save(VU_BYTE **stream);
    virtual int Save(FILE *file);

    void ReadDoctrineFile(void);
    void ReadPriorityFile(int tactic);
    int CheckControl(GridIndex X, GridIndex Y);
    void SetActive(int act);
    void DumpHeader(void);
    void Dump(void);
    void DoFullUpdate(VuTargetEntity *target);

    int CStance(Control country)
    {
        return stance[::GetTeam(country)];
    }
    int TStance(Team team)
    {
        return stance[team];
    }
    int Initiative(void)
    {
        return initiative;
    }
    int HasSatelites(void)
    {
        return flags bitand TEAM_HASSATS;
    }
    ATM GetATM(void)
    {
        return atm;
    }
    GTM GetGTM(void)
    {
        return gtm;
    }
    NTM GetNTM(void)
    {
        return ntm;
    }
    void SetName(_TCHAR *newname);
    _TCHAR* GetName(void);
    void SetFlag(uchar flag)
    {
        teamFlag = flag;
    }
    void SetEquipment(uchar e)
    {
        equipment = e;
    }
    int GetFlag(void)
    {
        return (int) teamFlag;
    }
    void SetColor(uchar color)
    {
        teamColor = color;
    }
    int GetColor(void)
    {
        return (int) teamColor;
    }
    int GetEquipment(void)
    {
        return (int) equipment;
    }
    void SetMotto(_TCHAR *motto);
    _TCHAR *GetMotto(void);
    TeamDoctrine* GetDoctrine(void)
    {
        return &doctrine;
    }
    // int OnOffensive(void) { return offensiveLoss; }
    uchar GetGroundActionType(void)
    {
        return groundAction.actionType;
    }
    void SelectGroundAction(void);
    void SelectAirActions(void);
    void SetGroundAction(TeamGndActionType *action);

    virtual int IsTeam(void)
    {
        return TRUE;
    };

    // Dirty Data
    void MakeTeamDirty(Dirty_Team bits, Dirtyness score);
    void WriteDirty(unsigned char **stream);
    //sfr: added rem
    void ReadDirty(unsigned char **stream, long *rem);
};

extern TeamClass* TeamInfo[NUM_TEAMS];

// =============================================
// Global functions
// =============================================

void AddTeam(int);
void RemoveTeam(int);

void AddNewTeams(RelType defaultStance);

void RemoveTeams(void);

int LoadTeams(char* scenario);

int SaveTeams(char* scenario);

void LoadPriorityTables(void);

Team GetTeam(Control country);

int GetCCRelations(Control who, Control with);

int GetCTRelations(Control who, Team with);

int GetTTRelations(Team who, Team with);

int GetTCRelations(Team who, Control with);

void SetTeam(Control country, int team);

void SetCTRelations(Control who, Team with, int rel);

void SetTTRelations(Team who, Team with, int rel);

int GetRoE(Team a, Team b, int type);

void TransferInitiative(Team from, Team to, int i);

float AirExperienceAdjustment(Team t);

float AirDefenseExperienceAdjustment(Team t);

float GroundExperienceAdjustment(Team t);

float NavalExperienceAdjustment(Team t);

float CombatBonus(Team who, VU_ID poid);

void ApplyPlayerInput(Team who, VU_ID poid, int rating);

Team GetEnemyTeam(Team who);

int GetPriority(MissionRequest mis);

void AddReinforcements(Team who, int inc);

void UpdateTeamStatistics(void);

int NavalSuperiority(Team who);

int AirSuperiority(Team who);

#endif
