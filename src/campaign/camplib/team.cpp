// ***************************************************************************
// Team.cpp
//
// Team related variables and routines
// ***************************************************************************

#include <cISO646>
#include <io.h>
#include <stdio.h>
#include <fcntl.h>
#include "campmap.h"
#include "cmpglobl.h"
#include "CampList.h"
#include "Manager.h"
#include "ATM.h"
#include "GTM.h"
#include "NTM.h"
#include "team.h"
#include "falcmesg.h"
#include "CmpClass.h"
#include "F4Find.h"
#include "find.h"
#include "mission.h"
#include "Campaign.h"
#include "CUIEvent.h"
#include "ui_ia.h"
#include "History.h"
#include "GndUnit.h"
#include "CampStr.h"
#include "FalcSess.h"
#include "AIInput.h"
#include "classtbl.h"
#include "MissEval.h"
#include "DispCfg.h"
#include "Falcuser.h"
//sfr: added for checks
#include "InvalidBufferException.h"
// =========================
// Defines
// =========================

#define SIDE_CHECK_DISTANCE 60


// A.S. begin, 2001-12-09 makes ACTION_RATE and ACTION_TIMEOUT configurable
#define ACTION_RATE (ActionRate*CampaignHours) // How often we can start new actions
#define ACTION_TIMEOUT (ActionTimeOut*CampaignHours) // Maximum time an offensive action can last
// end

// old code
//#define ACTION_RATE (8*CampaignHours) // How often we can start new actions
//#define ACTION_TIMEOUT (24*CampaignHours) // Maximum time an offensive action can last

// =========================
// Externals
// =========================

#ifdef CAMPTOOL
// Renaming tool stuff
extern VU_ID_NUMBER RenameTable[65536];
extern int gRenameIds;
#endif

#ifdef DEBUG
//extern int gPlaceholderOnly;
//extern VuAntiDatabase *vuAntiDB;
//extern char gPlaceholderFile[80];
#endif

extern int gCampDataVersion;
extern int g_nNoPlayerPlay; // JB 010926

TeamClass* TeamInfo[NUM_TEAMS] = { 0 };
short teamManagerDIndex;

char *CampGetNext(FILE* fptr);
void SwapCRLF(char *buf);
int AdjustMissionForAction(MissionRequest mis);
int GetTeamSituation(Team t);
void RemoveTeam(int teamNum);
void StartOffensive(int who, int initiative);

_TCHAR DogfightTeamNames[NUM_TEAMS][20] = // this will get hammered by the UI later
{
    "UFO",
    "Crimson",
    "Shark",
    "USA",
    "Tiger",
    "UFO",
    "UFO",
    "UFO",
};

#ifdef DEBUG
struct priority_structure
{
    char name[40];
    int number_queried;
    int average_score;
    int average_distance;
    int average_target;
    int average_mission;
    int average_random;
    int average_pak;
    int average_bonus;
    int total_score;
    int total_distance;
    int total_target;
    int total_mission;
    int total_random;
    int total_pak;
    int total_bonus;
    // Eventually, I'd like to keep a sorted list of these.
    struct priority_structure *prev;
    struct priority_structure *next;
};
priority_structure Priorities[AMIS_OTHER] = {0};
priority_structure *PriorityList;

int MaxPakPriority = 0;
int MinPakPriority  = 100;

int gReinforcementsAdded[NUM_TEAMS] = {0};

void InsertIntoSortedList(priority_structure *el);
void RemoveFromSortedList(priority_structure *el);
#endif

#ifdef DEBUG
extern int gSupplyFromOffensive[NUM_TEAMS];
extern int gFuelFromOffensive[NUM_TEAMS];
extern int gReplacmentsFromOffensive[NUM_TEAMS];
#endif

#ifdef DEBUG
int gFits = 0;
int gMisfits = 0;
#endif

// ============================
// ROE Table
// ============================

uchar RoEData[ROE_NAVAL_BOMBARD + 1][War + 1] =
{
    // NoRelations // Allied // Friendly // Neutral // Hostile // War
    { ROE_NOT_ALLOWED, ROE_NOT_ALLOWED, ROE_NOT_ALLOWED, ROE_NOT_ALLOWED, ROE_NOT_ALLOWED, ROE_NOT_ALLOWED }, // ROE_NOTHING
    { ROE_NOT_ALLOWED, ROE_NOT_ALLOWED, ROE_NOT_ALLOWED, ROE_NOT_ALLOWED, ROE_NOT_ALLOWED, ROE_ALLOWED }, // ROE_GROUND_FIRE
    { ROE_ALLOWED, ROE_ALLOWED, ROE_NOT_ALLOWED, ROE_NOT_ALLOWED, ROE_NOT_ALLOWED , ROE_ALLOWED }, // ROE_GROUND_MOVE
    { ROE_NOT_ALLOWED, ROE_NOT_ALLOWED, ROE_NOT_ALLOWED, ROE_NOT_ALLOWED, ROE_NOT_ALLOWED, ROE_ALLOWED }, // ROE_GROUND_CAPTURE
    { ROE_NOT_ALLOWED, ROE_NOT_ALLOWED, ROE_NOT_ALLOWED, ROE_NOT_ALLOWED, ROE_ALLOWED, ROE_ALLOWED }, // ROE_AIR_ENGAGE
    { ROE_NOT_ALLOWED, ROE_NOT_ALLOWED, ROE_NOT_ALLOWED, ROE_NOT_ALLOWED, ROE_NOT_ALLOWED, ROE_ALLOWED }, // ROE_AIR_FIRE
    { ROE_NOT_ALLOWED, ROE_NOT_ALLOWED, ROE_NOT_ALLOWED, ROE_NOT_ALLOWED, ROE_NOT_ALLOWED, ROE_NOT_ALLOWED }, // ROE_AIR_FIRE_BVR
    { ROE_ALLOWED, ROE_ALLOWED, ROE_ALLOWED, ROE_NOT_ALLOWED, ROE_NOT_ALLOWED, ROE_ALLOWED }, // ROE_AIR_OVERFLY
    { ROE_NOT_ALLOWED, ROE_NOT_ALLOWED, ROE_NOT_ALLOWED, ROE_NOT_ALLOWED, ROE_NOT_ALLOWED, ROE_ALLOWED }, // ROE_AIR_ATTACK
    { ROE_NOT_ALLOWED, ROE_ALLOWED, /*JB 010728 ROE_ALLOWED*/ROE_NOT_ALLOWED, ROE_NOT_ALLOWED, ROE_NOT_ALLOWED, ROE_NOT_ALLOWED }, // ROE_AIR_USE_BASES
    { ROE_NOT_ALLOWED, ROE_NOT_ALLOWED, ROE_NOT_ALLOWED, ROE_NOT_ALLOWED, ROE_ALLOWED, ROE_ALLOWED }, // ROE_NAVAL_FIRE
    { ROE_ALLOWED, ROE_ALLOWED, ROE_ALLOWED, ROE_NOT_ALLOWED, ROE_NOT_ALLOWED, ROE_ALLOWED }, // ROE_NAVAL_MOVE
    { ROE_NOT_ALLOWED, ROE_NOT_ALLOWED, ROE_NOT_ALLOWED, ROE_NOT_ALLOWED, ROE_ALLOWED, ROE_ALLOWED }
}; // ROE_NAVAL_BOMBARD

// =======================================
// Priority tables
// =======================================

uchar DefaultObjtypePriority[TAT_CAS][MAX_TGTTYPE]; // AI's suggested settings
uchar DefaultUnittypePriority[TAT_CAS][MAX_UNITTYPE]; //
uchar DefaultMissionPriority[TAT_CAS][AMIS_OTHER]; //

// ============================
// External Function Prototypes
// ============================

// ===============================
// Team Class
// ===============================

TeamClass::TeamClass(int typeindex, Control owner)
    : FalconEntity(typeindex, GetIdFromNamespace(VolatileNS))
{
    // these are read from file by each side
    SetSendCreate(VuEntity::VU_SC_DONT_SEND);
    InitLocalData(owner);
}

TeamClass::TeamClass(VU_BYTE **stream, long *rem)
    : FalconEntity(VU_LAST_ENTITY_TYPE, GetIdFromNamespace(VolatileNS))
{
    // these are read from file by each side
    SetSendCreate(VuEntity::VU_SC_DONT_SEND);

    VU_ID tmpID;

    // Read vu stuff here
    memcpychk(&share_.id_, stream, sizeof(VU_ID), rem);
    memcpychk(&share_.ownerId_, stream, sizeof(VU_ID), rem);
    memcpychk(&share_.entityType_, stream, sizeof(ushort), rem);
    SetEntityType(share_.entityType_);

    // Start reading the shit
    memcpychk(&who, stream, sizeof(Team), rem);
    ShiAssert(TeamInfo[who] and TeamInfo[who]->share_.id_ == share_.id_)

    memcpychk(&cteam, stream, sizeof(Team), rem);
    ShiAssert((cteam > 0 or who == 0) and cteam < NUM_TEAMS);
    memcpychk(&flags, stream, sizeof(short), rem);
    memcpychk(member, stream, sizeof(uchar)*NUM_COUNS, rem);
    memcpychk(stance, stream, sizeof(short)*NUM_TEAMS, rem);
    memcpychk(&firstColonel, stream, sizeof(short), rem);
    memcpychk(&firstCommander, stream, sizeof(short), rem);
    memcpychk(&firstWingman, stream, sizeof(short), rem);
    memcpychk(&lastWingman, stream, sizeof(short), rem);

    memcpychk(&airExperience, stream, sizeof(uchar), rem);
    memcpychk(&airDefenseExperience, stream, sizeof(uchar), rem);
    memcpychk(&groundExperience, stream, sizeof(uchar), rem);
    memcpychk(&navalExperience, stream, sizeof(uchar), rem);
    memcpychk(&initiative, stream, sizeof(short), rem);
    memcpychk(&supplyAvail, stream, sizeof(ushort), rem);
    memcpychk(&fuelAvail, stream, sizeof(ushort), rem);
    memcpychk(&replacementsAvail, stream, sizeof(ushort), rem);
    memcpychk(&playerRating, stream, sizeof(float), rem);
    memcpychk(&lastPlayerMission, stream, sizeof(CampaignTime), rem);
    memcpychk(&currentStats, stream, sizeof(TeamStatusType), rem);
    memcpychk(&startStats, stream, sizeof(TeamStatusType), rem);
    memcpychk(&reinforcement, stream, sizeof(short), rem);
    memcpychk(bonusObjs, stream, sizeof(VU_ID)*MAX_BONUSES, rem);
    memcpychk(bonusTime, stream, sizeof(CampaignTime)*MAX_BONUSES, rem);
    memcpychk(objtype_priority, stream, sizeof(uchar)*MAX_TGTTYPE, rem);
    memcpychk(unittype_priority, stream, sizeof(uchar)*MAX_UNITTYPE, rem);
    memcpychk(mission_priority, stream, sizeof(uchar)*AMIS_OTHER, rem);
    memcpychk(max_vehicle, stream, sizeof(uchar) * 4, rem);
    memcpychk(&teamFlag, stream, sizeof(uchar), rem);
    memcpychk(&teamColor, stream, sizeof(uchar), rem);
    memcpychk(&equipment, stream, sizeof(uchar), rem);
    memcpychk(name, stream, sizeof(_TCHAR)*MAX_TEAM_NAME_SIZE, rem);
    memcpychk(teamMotto, stream, sizeof(_TCHAR)*MAX_MOTTO_SIZE, rem);
    memcpychk(&groundAction, stream, sizeof(TeamGndActionType), rem);
    memcpychk(&defensiveAirAction, stream, sizeof(TeamAirActionType), rem);
    memcpychk(&offensiveAirAction, stream, sizeof(TeamAirActionType), rem);

    atm = NULL;
    gtm = NULL;
    ntm = NULL;

    dirty_team = 0;

    // Attach our managers, if we have them already
    memcpychk(&tmpID, stream, sizeof(VU_ID), rem);
    memcpychk(&tmpID, stream, sizeof(VU_ID), rem);
    memcpychk(&tmpID, stream, sizeof(VU_ID), rem);

    // Associate this entity with the game owner.
    if (FalconLocalGame)
    {
        SetAssociation(FalconLocalGame->Id());
    }

    ReadDoctrineFile();
}

TeamClass::TeamClass(FILE *file) :
    FalconEntity(VU_LAST_ENTITY_TYPE, GetIdFromNamespace(VolatileNS))
{
    // these are read from file by each side
    SetSendCreate(VuEntity::VU_SC_DONT_SEND);

    // Read vu stuff here
    fread(&share_.id_, sizeof(VU_ID), 1, file);
    fread(&share_.entityType_, sizeof(ushort), 1, file);
    // SetEntityType(share_.entityType_);
    //
    //#ifdef DEBUG
    // // Somehow this entity type got changed..
    // if (gCampDataVersion < 35)
    // SetEntityType(883);
    //#endif

    //#ifdef CAMPTOOL
    // if (gRenameIds)
    // {
    // VU_ID new_id = FalconNullId;
    //
    // // Rename this ID
    // for (new_id.num_ = LAST_NON_VOLATILE_VU_ID_NUMBER-1; new_id.num_ > FIRST_NON_VOLATILE_VU_ID_NUMBER ; new_id.num_--)
    // {
    // if ( not vuDatabase->Find(new_id))
    // {
    // RenameTable[share_.id_.num_] = new_id.num_;
    // share_.id_ = new_id;
    // break;
    // }
    // }
    // }
    //#endif

    // Reset the entity type, just to be sure.
    SetEntityType(teamManagerDIndex);
    fread(&who, sizeof(Team), 1, file);
    fread(&cteam, sizeof(Team), 1, file);
    ShiAssert((cteam > 0 or who == 0) and cteam < NUM_TEAMS);
    fread(&flags, sizeof(short), 1, file);

    if (gCampDataVersion > 2)
    {
        fread(member, sizeof(uchar), NUM_COUNS, file);
        fread(stance, sizeof(short), NUM_TEAMS, file);
    }
    else
    {
        memset(member, 0, NUM_COUNS);
        fread(member, sizeof(uchar), 7, file);
        fread(stance, sizeof(short), 7, file);
    }

    fread(&firstColonel, sizeof(short), 1, file);
    fread(&firstCommander, sizeof(short), 1, file);
    fread(&firstWingman, sizeof(short), 1, file);
    fread(&lastWingman, sizeof(short), 1, file);

    playerRating = 0.0F;
    lastPlayerMission = 0;

    if (gCampDataVersion > 11)
    {
        fread(&airExperience, sizeof(uchar), 1, file);
        fread(&airDefenseExperience, sizeof(uchar), 1, file);
        fread(&groundExperience, sizeof(uchar), 1, file);
        fread(&navalExperience, sizeof(uchar), 1, file);
    }
    else
    {
        short junk;
        fread(&junk, sizeof(short), 1, file);
        fread(&junk, sizeof(short), 1, file);
        airExperience = 80;
        airDefenseExperience = 80;
        groundExperience = 80;
        navalExperience = 80;
    }

    fread(&initiative, sizeof(short), 1, file);
    fread(&supplyAvail, sizeof(ushort), 1, file);
    fread(&fuelAvail, sizeof(ushort), 1, file);

    if (gCampDataVersion > 53)
    {
        fread(&replacementsAvail, sizeof(ushort), 1, file);
        fread(&playerRating, sizeof(float), 1, file);
        fread(&lastPlayerMission, sizeof(CampaignTime), 1, file);
    }
    else
    {
        replacementsAvail = 0;
        playerRating = 0.0F;
        lastPlayerMission = 0;
    }

    if (gCampDataVersion < 40)
    {
        ushort
        dummy;

        fread(&dummy, sizeof(ushort), 1, file);
        fread(&dummy, sizeof(ushort), 1, file);
    }

    fread(&currentStats, sizeof(TeamStatusType), 1, file);
    fread(&startStats, sizeof(TeamStatusType), 1, file);
    fread(&reinforcement, sizeof(short), 1, file);
    fread(bonusObjs, sizeof(VU_ID), MAX_BONUSES, file);
    fread(bonusTime, sizeof(CampaignTime), MAX_BONUSES, file);
    fread(objtype_priority, sizeof(uchar), MAX_TGTTYPE, file);
    fread(unittype_priority, sizeof(uchar), MAX_UNITTYPE, file);

    if (gCampDataVersion < 30)
        fread(mission_priority, sizeof(uchar), 40, file);
    else
        fread(mission_priority, sizeof(uchar), AMIS_OTHER, file);

    if (gCampDataVersion < 46)
    {
        memcpy(objtype_priority, DefaultObjtypePriority[TAT_CAS - 1], sizeof(uchar)*MAX_TGTTYPE);
        memcpy(unittype_priority, DefaultUnittypePriority[TAT_CAS - 1], sizeof(uchar)*MAX_UNITTYPE);
        memcpy(mission_priority, DefaultMissionPriority[TAT_CAS - 1], sizeof(uchar)*AMIS_OTHER);
    }

    if (gCampDataVersion < 34)
    {
        CampaignTime attackTime;
        uchar offensiveLoss;
        fread(&attackTime, sizeof(CampaignTime), 1, file);
        fread(&offensiveLoss, sizeof(uchar), 1, file);
    }

    fread(max_vehicle, sizeof(uchar), 4, file);

    if (gCampDataVersion > 4)
    {
        fread(&teamFlag, sizeof(uchar), 1, file);

        if (gCampDataVersion > 32)
        {
            fread(&teamColor, sizeof(uchar), 1, file);
        }
        else
        {
            teamColor = 0;
        }

        fread(&equipment, sizeof(uchar), 1, file);
        fread(name, sizeof(_TCHAR)*MAX_TEAM_NAME_SIZE, 1, file);
    }

    if (gCampDataVersion < 41)
    {
        // Hand set colors correctly
        if (FalconLocalGame and FalconLocalGame->GetGameType() == game_Campaign)
        {
            _TCHAR owner[41];
            ReadIndexedString(40 + who, owner, 40);
            _tcscpy(name, owner);
            teamColor = who;
            teamFlag = who;
        }
        else if (FalconLocalGame and FalconLocalGame->GetGameType() == game_TacticalEngagement)
        {
            if (who == 1)
                teamColor = 2;
        }

        if (FalconLocalGame->GetGameType() not_eq game_InstantAction)
            stance[0] = NoRelations;

        if (FalconLocalGame->GetGameType() == game_TacticalEngagement)
        {
            int i;

            for (i = 0; i < NUM_TEAMS; i++)
            {
                if ( not i or not who)
                    stance[i] = NoRelations;
                else if (i not_eq who)
                    stance[i] = War;
                else
                    stance[i] = Allied;

                max_vehicle[0] = max_vehicle[1] = max_vehicle[2] = max_vehicle[3] = 16;
            }
        }
    }

    // KCK HACK: Hand clobber instant action and dogfight colors
    if (FalconLocalGame and FalconLocalGame->GetGameType() == game_Dogfight)
    {
        if (who == 0)
            teamColor = 0;
        else if (who == 1)
            teamColor = 6;
        else if (who == 2)
            teamColor = 2;
        else if (who == 3)
            teamColor = 0;
        else if (who == 4)
            teamColor = 5;
        else
            teamColor = who;

        SetName(DogfightTeamNames[who]);
    }
    else if (FalconLocalGame and FalconLocalGame->GetGameType() == game_InstantAction)
    {
        if (who == 0)
            teamColor = 1;
        else if (who == 1)
        {
            cteam = 1;
            member[2] = 0;
            teamColor = 2;
        }

        if (who == 2)
        {
            cteam = 2;
            teamColor = 6;
            member[1] = 0;
        }
    }

    // SCR HACK: Hand clobber instant action skill levels
    if (FalconLocalGame and FalconLocalGame->GetGameType() == game_InstantAction)
    {
        ShiAssert(InstantActionSettings.PilotLevel >= 0 and InstantActionSettings.PilotLevel <= 4);
        airExperience = 60 + 10 * InstantActionSettings.PilotLevel;
        airDefenseExperience = 60 + 10 * InstantActionSettings.PilotLevel;
    }

    // KCK HACK: For campaign (and localization), clobber team names
    if (FalconLocalGame and FalconLocalGame->GetGameType() == game_Campaign)
        ReadIndexedString(40 + who, name, MAX_TEAM_NAME_SIZE);

    if (gCampDataVersion > 32)
    {
        fread(teamMotto, sizeof(_TCHAR)*MAX_MOTTO_SIZE, 1, file);
    }
    else
    {
        memset(teamMotto, 0, sizeof(_TCHAR)*MAX_MOTTO_SIZE);
    }

    if (gCampDataVersion > 33)
    {
        if (gCampDataVersion > 50)
            fread(&groundAction, sizeof(TeamGndActionType), 1, file);
        else if (gCampDataVersion > 41)
        {
            fread(&groundAction, 27, 1, file);
            memset(&groundAction, 0, sizeof(TeamGndActionType));
        }
        else
        {
            fread(&groundAction, 23, 1, file);
            memset(&groundAction, 0, sizeof(TeamGndActionType));
        }

        fread(&defensiveAirAction, sizeof(TeamAirActionType), 1, file);
        fread(&offensiveAirAction, sizeof(TeamAirActionType), 1, file);
    }
    else
    {
        memset(&groundAction, 0, sizeof(TeamGndActionType));
        memset(&defensiveAirAction, 0, sizeof(TeamAirActionType));
        memset(&offensiveAirAction, 0, sizeof(TeamAirActionType));
    }

    if (gCampDataVersion < 43)
    {
        groundAction.actionType = GACTION_CONSOLIDATE;
        supplyAvail = fuelAvail = 1000;
    }

    if (gCampDataVersion < 60 and FalconLocalGame and FalconLocalGame->GetGameType() == game_Campaign)
    {
        if (who == COUN_US)
            equipment = toe_us;
        else if (who == COUN_SOUTH_KOREA)
            equipment = toe_rok;
        else if (who == COUN_JAPAN)
            equipment = toe_us;
        else if (who == COUN_RUSSIA)
            equipment = toe_soviet;
        else if (who == COUN_CHINA)
            equipment = toe_chinese;
        else
            equipment = toe_dprk;
    }

    if (gCampDataVersion < 51)
    {
        if (who == COUN_RUSSIA)
        {
            firstColonel = 500;
            firstCommander = 505;
            firstWingman = 538;
            lastWingman = 583;
        }
        else if (who == COUN_CHINA)
        {
            firstColonel = 600;
            firstCommander = 605;
            firstWingman = 639;
            lastWingman = 686;
        }
        else if (who == COUN_US)
        {
            firstColonel = 0;
            firstCommander = 20;
            firstWingman = 149;
            lastWingman = 373;
        }
        else
        {
            firstColonel = 400;
            firstCommander = 408;
            firstWingman = 460;
            lastWingman = 499;
        }
    }

    // Set the owner to the game master.
    if ((FalconLocalGame) and ( not FalconLocalGame->IsLocal()))
    {
        SetOwner(FalconLocalGame->OwnerId());
        flags and_eq compl TEAM_UPDATED; // We're not updated until we get data from the master
    }
    else
        flags or_eq TEAM_UPDATED;

    atm = NULL;
    gtm = NULL;
    ntm = NULL;
    dirty_team = 0;

    // Associate this entity with the game owner.
    if (FalconLocalGame)
    {
        SetAssociation(FalconLocalGame->Id());
    }

    ReadDoctrineFile();
}

TeamClass::~TeamClass(void)
{
}

void TeamClass::InitData()
{
    FalconEntity::InitData();
    InitLocalData(who);
}

void TeamClass::InitLocalData(Control owner)
{
    _TCHAR towner[41];
    short tid;
    who = owner;
    cteam = owner;
    flags = 0;
    memset(member, 0, NUM_COUNS);
    memset(stance, 0, sizeof(short)*NUM_TEAMS);
    // These members are for this COUNTRY
    firstColonel = FIRST_COLONEL; // Pilot ID indexies for this country - init to US names
    firstCommander = FIRST_COMMANDER;
    firstWingman = FIRST_WINGMAN;
    lastWingman = LAST_WINGMAN;
    airExperience = 80; // Experience for aircraft (effects pilot's skill)
    airDefenseExperience = 80; // Experience for air defenses
    groundExperience = 80; // Experience for ground troops
    navalExperience = 80; // Experience for ships
    equipment = 0;
    // These members are for this TEAM
    initiative = 50;
    supplyAvail = 1000;
    fuelAvail = 1000;
    reinforcement = -1;
    ReadIndexedString(40 + who, towner, 40);
    _tcscpy(name, towner);
    teamColor = who;
    teamFlag = who;
    memset(teamMotto, 0, sizeof(_TCHAR)*MAX_MOTTO_SIZE);
    currentStats.airDefenseVehs = startStats.airDefenseVehs = 0;
    currentStats.aircraft = startStats.aircraft = 0;
    currentStats.groundVehs = startStats.groundVehs = 0;
    currentStats.ships = startStats.ships = 0;
    currentStats.supply = startStats.supply = 0;
    currentStats.fuel = startStats.fuel = 0;
    currentStats.airbases = startStats.airbases = 0;
    currentStats.supplyLevel = startStats.supplyLevel = 100;
    currentStats.fuelLevel = startStats.fuelLevel = 100;
    memset(bonusObjs, 0, sizeof(VU_ID)*MAX_BONUSES);
    memset(bonusTime, 0, sizeof(CampaignTime)*MAX_BONUSES);
    memcpy(objtype_priority, DefaultObjtypePriority[TAT_CAS - 1], sizeof(uchar)*MAX_TGTTYPE);
    memcpy(unittype_priority, DefaultUnittypePriority[TAT_CAS - 1], sizeof(uchar)*MAX_UNITTYPE);
    memcpy(mission_priority, DefaultMissionPriority[TAT_CAS - 1], sizeof(uchar)*AMIS_OTHER);
    memset(max_vehicle, 16, sizeof(uchar) * 4);
    memset(&groundAction, 0, sizeof(TeamGndActionType));
    memset(&defensiveAirAction, 0, sizeof(TeamAirActionType));
    memset(&offensiveAirAction, 0, sizeof(TeamAirActionType));

    // Set some default values
    for (tid = 1; tid < NUM_TEAMS; tid++)
    {
        stance[tid] = Neutral;
    }

    stance[owner] = Allied;
    member[owner] = 1;
    stance[0] = NoRelations;

    atm = NULL;
    gtm = NULL;
    ntm = NULL;

    playerRating = 0.0F;
    lastPlayerMission = 0;

    ReadDoctrineFile();
    dirty_team = 0;

    if (FalconLocalGame)
    {
        SetAssociation(FalconLocalGame->Id());
    }
}

// KCK NOTE: this save size is only valid for saving to a stream, not to a file
int TeamClass::SaveSize()
{
    return sizeof(VU_ID)
           + sizeof(VU_ID)
           + sizeof(ushort)
           + sizeof(Team)
           + sizeof(Team)
           + sizeof(short)
           + sizeof(uchar) * NUM_COUNS
           + sizeof(short) * NUM_TEAMS
           + sizeof(short)
           + sizeof(short)
           + sizeof(short)
           + sizeof(short)
           + sizeof(uchar)
           + sizeof(uchar)
           + sizeof(uchar)
           + sizeof(uchar)
           + sizeof(short)
           + sizeof(ushort)
           + sizeof(ushort)
           + sizeof(ushort)
           + sizeof(float)
           + sizeof(CampaignTime)
           + sizeof(TeamStatusType)
           + sizeof(TeamStatusType)
           + sizeof(short)
           + sizeof(VU_ID) * MAX_BONUSES
           + sizeof(CampaignTime) * MAX_BONUSES
           + sizeof(uchar) * MAX_TGTTYPE
           + sizeof(uchar) * MAX_UNITTYPE
           + sizeof(uchar) * AMIS_OTHER
           + sizeof(uchar) * 4
           + sizeof(uchar)
           + sizeof(uchar)
           + sizeof(uchar)
           + sizeof(_TCHAR) * MAX_TEAM_NAME_SIZE
           + sizeof(_TCHAR) * MAX_MOTTO_SIZE
           + sizeof(TeamGndActionType)
           + sizeof(TeamAirActionType)
           + sizeof(TeamAirActionType)
           + sizeof(VU_ID)
           + sizeof(VU_ID)
           + sizeof(VU_ID);
}

int TeamClass::Save(VU_BYTE **stream)
{
    VU_ID tmpID;

    // Write vu stuff here
    memcpy(*stream, &share_.id_, sizeof(VU_ID));
    *stream += sizeof(VU_ID);
    memcpy(*stream, &share_.ownerId_, sizeof(VU_ID));
    *stream += sizeof(VU_ID);
    memcpy(*stream, &share_.entityType_, sizeof(ushort));
    *stream += sizeof(ushort);

    memcpy(*stream, &who, sizeof(Team));
    *stream += sizeof(Team);
    memcpy(*stream, &cteam, sizeof(Team));
    *stream += sizeof(Team);
    memcpy(*stream, &flags, sizeof(short));
    *stream += sizeof(short);
    memcpy(*stream, member, sizeof(uchar)*NUM_COUNS);
    *stream += sizeof(uchar) * NUM_COUNS;
    memcpy(*stream, stance, sizeof(short)*NUM_TEAMS);
    *stream += sizeof(short) * NUM_TEAMS;
    memcpy(*stream, &firstColonel, sizeof(short));
    *stream += sizeof(short);
    memcpy(*stream, &firstCommander, sizeof(short));
    *stream += sizeof(short);
    memcpy(*stream, &firstWingman, sizeof(short));
    *stream += sizeof(short);
    memcpy(*stream, &lastWingman, sizeof(short));
    *stream += sizeof(short);
    memcpy(*stream, &airExperience, sizeof(uchar));
    *stream += sizeof(uchar);
    memcpy(*stream, &airDefenseExperience, sizeof(uchar));
    *stream += sizeof(uchar);
    memcpy(*stream, &groundExperience, sizeof(uchar));
    *stream += sizeof(uchar);
    memcpy(*stream, &navalExperience, sizeof(uchar));
    *stream += sizeof(uchar);
    memcpy(*stream, &initiative, sizeof(short));
    *stream += sizeof(short);
    memcpy(*stream, &supplyAvail, sizeof(ushort));
    *stream += sizeof(ushort);
    memcpy(*stream, &fuelAvail, sizeof(ushort));
    *stream += sizeof(ushort);
    memcpy(*stream, &replacementsAvail, sizeof(ushort));
    *stream += sizeof(ushort);
    memcpy(*stream, &playerRating, sizeof(float));
    *stream += sizeof(float);
    memcpy(*stream, &lastPlayerMission, sizeof(CampaignTime));
    *stream += sizeof(CampaignTime);
    memcpy(*stream, &currentStats, sizeof(TeamStatusType));
    *stream += sizeof(TeamStatusType);
    memcpy(*stream, &startStats, sizeof(TeamStatusType));
    *stream += sizeof(TeamStatusType);
    memcpy(*stream, &reinforcement, sizeof(short));
    *stream += sizeof(short);
    memcpy(*stream, bonusObjs, sizeof(VU_ID)*MAX_BONUSES);
    *stream += sizeof(VU_ID) * MAX_BONUSES;
    memcpy(*stream, bonusTime, sizeof(CampaignTime)*MAX_BONUSES);
    *stream += sizeof(CampaignTime) * MAX_BONUSES;
    memcpy(*stream, objtype_priority, sizeof(uchar)*MAX_TGTTYPE);
    *stream += sizeof(uchar) * MAX_TGTTYPE;
    memcpy(*stream, unittype_priority, sizeof(uchar)*MAX_UNITTYPE);
    *stream += sizeof(uchar) * MAX_UNITTYPE;
    memcpy(*stream, mission_priority, sizeof(uchar)*AMIS_OTHER);
    *stream += sizeof(uchar) * AMIS_OTHER;
    memcpy(*stream, max_vehicle, sizeof(uchar) * 4);
    *stream += sizeof(uchar) * 4;
    memcpy(*stream, &teamFlag, sizeof(uchar));
    *stream += sizeof(uchar);
    memcpy(*stream, &teamColor, sizeof(uchar));
    *stream += sizeof(uchar);
    memcpy(*stream, &equipment, sizeof(uchar));
    *stream += sizeof(uchar);
    memcpy(*stream, name, sizeof(_TCHAR)*MAX_TEAM_NAME_SIZE);
    *stream += sizeof(_TCHAR) * MAX_TEAM_NAME_SIZE;
    memcpy(*stream, teamMotto, sizeof(_TCHAR)*MAX_MOTTO_SIZE);
    *stream += sizeof(_TCHAR) * MAX_MOTTO_SIZE;
    memcpy(*stream, &groundAction, sizeof(TeamGndActionType));
    *stream += sizeof(TeamGndActionType);
    memcpy(*stream, &defensiveAirAction, sizeof(TeamAirActionType));
    *stream += sizeof(TeamAirActionType);
    memcpy(*stream, &offensiveAirAction, sizeof(TeamAirActionType));
    *stream += sizeof(TeamAirActionType);

    // Send our managersz
    if (atm)
    {
        tmpID = atm->Id();
    }
    else
    {
        tmpID = FalconNullId;
    }

    memcpy(*stream, &tmpID, sizeof(VU_ID));
    *stream += sizeof(VU_ID);

    if (gtm)
    {
        tmpID = gtm->Id();
    }
    else
    {
        tmpID = FalconNullId;
    }

    memcpy(*stream, &tmpID, sizeof(VU_ID));
    *stream += sizeof(VU_ID);

    if (ntm)
    {
        tmpID = ntm->Id();
    }
    else
    {
        tmpID = FalconNullId;
    }

    memcpy(*stream, &tmpID, sizeof(VU_ID));
    *stream += sizeof(VU_ID);

    return TeamClass::SaveSize() + 3 * sizeof(VU_ID);
}

int TeamClass::Save(FILE *file)
{
    int retval = 0;

    if (file)
    {
        // Write vu stuff here
        retval += fwrite(&share_.id_, sizeof(VU_ID), 1, file);
        retval += fwrite(&share_.entityType_, sizeof(ushort), 1, file);

        retval += fwrite(&who, sizeof(Team), 1, file);
        retval += fwrite(&cteam, sizeof(Team), 1, file);
        retval += fwrite(&flags, sizeof(short), 1, file);
        retval += fwrite(member, sizeof(uchar), NUM_COUNS, file);
        retval += fwrite(stance, sizeof(short), NUM_TEAMS, file);
        retval += fwrite(&firstColonel, sizeof(short), 1, file);
        retval += fwrite(&firstCommander, sizeof(short), 1, file);
        retval += fwrite(&firstWingman, sizeof(short), 1, file);
        retval += fwrite(&lastWingman, sizeof(short), 1, file);
        retval += fwrite(&airExperience, sizeof(uchar), 1, file);
        retval += fwrite(&airDefenseExperience, sizeof(uchar), 1, file);
        retval += fwrite(&groundExperience, sizeof(uchar), 1, file);
        retval += fwrite(&navalExperience, sizeof(uchar), 1, file);
        retval += fwrite(&initiative, sizeof(short), 1, file);
        retval += fwrite(&supplyAvail, sizeof(ushort), 1, file);
        retval += fwrite(&fuelAvail, sizeof(ushort), 1, file);
        retval += fwrite(&replacementsAvail, sizeof(ushort), 1, file);
        retval += fwrite(&playerRating, sizeof(float), 1, file);
        retval += fwrite(&lastPlayerMission, sizeof(CampaignTime), 1, file);
        retval += fwrite(&currentStats, sizeof(TeamStatusType), 1, file);
        retval += fwrite(&startStats, sizeof(TeamStatusType), 1, file);
        retval += fwrite(&reinforcement, sizeof(short), 1, file);
#ifdef CAMPTOOL

        if (gRenameIds)
        {
            for (int i = 0; i < MAX_BONUSES; i++)
                bonusObjs[i].num_ = RenameTable[bonusObjs[i].num_];
        }

#endif
        retval += fwrite(bonusObjs, sizeof(VU_ID), MAX_BONUSES, file);
        retval += fwrite(bonusTime, sizeof(CampaignTime), MAX_BONUSES, file);
        retval += fwrite(objtype_priority, sizeof(uchar), MAX_TGTTYPE, file);
        retval += fwrite(unittype_priority, sizeof(uchar), MAX_UNITTYPE, file);
        retval += fwrite(mission_priority, sizeof(uchar), AMIS_OTHER, file);
        retval += fwrite(max_vehicle, sizeof(uchar), 4, file);
        retval += fwrite(&teamFlag, sizeof(uchar), 1, file);
        retval += fwrite(&teamColor, sizeof(uchar), 1, file);
        retval += fwrite(&equipment, sizeof(uchar), 1, file);
        retval += fwrite(name, sizeof(_TCHAR) * MAX_TEAM_NAME_SIZE, 1, file);
        retval += fwrite(teamMotto, sizeof(_TCHAR) * MAX_MOTTO_SIZE, 1, file);
        retval += fwrite(&groundAction, sizeof(TeamGndActionType), 1, file);
        retval += fwrite(&defensiveAirAction, sizeof(TeamAirActionType), 1, file);
        retval += fwrite(&offensiveAirAction, sizeof(TeamAirActionType), 1, file);
        retval += atm->Save(file);
        retval += gtm->Save(file);
        retval += ntm->Save(file);
    }

    return retval;
}

// Standard doctrine stuff (read from file)
void TeamClass::ReadDoctrineFile(void)
{
    FILE *fp;
    char tmpStr[40];

    sprintf(tmpStr, "doctrine%d", who);
    fp = OpenCampFile(tmpStr, "txt", "r");
    doctrine.simFlags = atoi(CampGetNext(fp));
    doctrine.radarShootShootPct = (float)atof(CampGetNext(fp));
    doctrine.heatShootShootPct  = (float)atof(CampGetNext(fp));
    CloseCampFile(fp);
}

#if 0
int TeamClass::CheckControl(GridIndex X, GridIndex Y)
{
    Objective o, bo = NULL;
    Int32 d, bd = 9999;
    GridIndex x, y;

    {
        VuListIterator myit(CCC);
        o = GetFirstObjective(&myit);

        while (o)
        {
            o->GetLocation(&x, &y);
            d = FloatToInt32(Distance(x, y, X, Y));

            if (d < bd)
            {
                bd = d;
                bo = o;
            }

            o = GetNextObjective(&myit);
        }
    }

    if ( not bo) // Test case- Should always have CCC
        return 1;

    /* Don't do this right now
     if ( not bo->Targeted() and bo->GetObjectiveStatus() > rand()%100) // The message got through (trigger a mission)
     {
     MissionRequestClass mis;
     bo->SetTargeted(1);
     bo->GetLocation(&x,&y);
     mis.vs = who;
     mis.tx = x;
     mis.ty = y;
     mis.targetID = bo->Id();
     mis.mission = AMIS_OCASTRIKE;
     mis.roe_check = ROE_AIR_ATTACK;
     mis.RequestEnemyMission();
     return 1;
     }
     */
    // Failure
    return 0;
}
#endif

void TeamClass::SetActive(int act)
{
    flags or_eq TEAM_ACTIVE;

    if ( not act)
        flags xor_eq TEAM_ACTIVE;
}

void TeamClass::DumpHeader(void)
{
    MonoPrint("Team CbtPow ADPow  AirPow GndPow NvlPow Sup  Fuel Sats  Tran Mrle Exp  Prod\n");
}

void TeamClass::Dump(void)
{
    // MonoPrint("%d    %5.5d  %4.4d   %4.4d   %4.4d   %4.4d",who,combatPower,airDefense,airPower,groundPower,navalPower);
    // MonoPrint("   %3.3d  %3.3d  %3.3d  %3.3d  %3.3d  %3.3d  %3.3d\n",supply,fuel,satelites,transportNet,morale,experience,production);
}

void TeamClass::DoFullUpdate(VuTargetEntity *target)
{
    if ( not target)
        target = FalconLocalGame;

    VuEvent *event = new VuFullUpdateEvent(this, target);
    event->RequestReliableTransmit();
    VuMessageQueue::PostVuMessage(event);

    if (atm)
    {
        VuEvent *event = new VuFullUpdateEvent(atm, target);
        event->RequestReliableTransmit();
        VuMessageQueue::PostVuMessage(event);
    }

    if (gtm)
    {
        VuEvent *event = new VuFullUpdateEvent(gtm, target);
        event->RequestReliableTransmit();
        VuMessageQueue::PostVuMessage(event);
    }

    if (ntm)
    {
        VuEvent *event = new VuFullUpdateEvent(ntm, target);
        event->RequestReliableTransmit();
        VuMessageQueue::PostVuMessage(event);
    }
}

// event handlers
int TeamClass::Handle(VuEvent *event)
{
    //Event Handler
    return (VuEntity::Handle(event));
}

int TeamClass::Handle(VuFullUpdateEvent *event)
{
    TeamClass* tmpTeam = (TeamClass*)(event->expandedData_.get());
    int retval;

    // Copy in new data
    memcpy(&share_.id_, &tmpTeam->share_.id_, sizeof(VU_ID));
    memcpy(&share_.entityType_, &tmpTeam->share_.entityType_, sizeof(ushort));
    memcpy(&who, &tmpTeam->who, sizeof(Team));
    memcpy(&cteam, &tmpTeam->cteam, sizeof(Team));
    ShiAssert((cteam > 0 or cteam == 0) and cteam < NUM_TEAMS);
    memcpy(&flags, &tmpTeam->flags, sizeof(short));
    memcpy(member, tmpTeam->member, sizeof(uchar)*NUM_COUNS);
    memcpy(stance, tmpTeam->stance, sizeof(short)*NUM_TEAMS);
    memcpy(&airExperience, &tmpTeam->airExperience, sizeof(uchar));
    memcpy(&airDefenseExperience, &tmpTeam->airDefenseExperience, sizeof(uchar));
    memcpy(&groundExperience, &tmpTeam->groundExperience, sizeof(uchar));
    memcpy(&navalExperience, &tmpTeam->navalExperience, sizeof(uchar));
    memcpy(&initiative, &tmpTeam->initiative, sizeof(short));
    memcpy(&supplyAvail, &tmpTeam->supplyAvail, sizeof(ushort));
    memcpy(&fuelAvail, &tmpTeam->fuelAvail, sizeof(ushort));
    memcpy(&currentStats, &tmpTeam->currentStats, sizeof(TeamStatusType));
    memcpy(&startStats, &tmpTeam->startStats, sizeof(TeamStatusType));
    memcpy(&reinforcement, &tmpTeam->reinforcement, sizeof(short));
    memcpy(bonusObjs, tmpTeam->bonusObjs, sizeof(VU_ID)*MAX_BONUSES);
    memcpy(bonusTime, tmpTeam->bonusTime, sizeof(CampaignTime)*MAX_BONUSES);
    memcpy(objtype_priority, tmpTeam->objtype_priority, sizeof(uchar)*MAX_TGTTYPE);
    memcpy(unittype_priority, tmpTeam->unittype_priority, sizeof(uchar)*MAX_UNITTYPE);
    memcpy(mission_priority, tmpTeam->mission_priority, sizeof(uchar)*AMIS_OTHER);
    memcpy(max_vehicle, tmpTeam->max_vehicle, sizeof(uchar) * 4);
    memcpy(&groundAction, &tmpTeam->groundAction, sizeof(TeamGndActionType));
    memcpy(&defensiveAirAction, &tmpTeam->defensiveAirAction, sizeof(TeamAirActionType));
    memcpy(&offensiveAirAction, &tmpTeam->offensiveAirAction, sizeof(TeamAirActionType));

    retval = VuEntity::Handle(event);

    // Set our flag if we've got all the team's info
    // WARNING: we don't guarentee receipt of the managers
    flags or_eq TEAM_UPDATED;

    // Mark team data as received if we have all the teams.
    for (int i = 0; i < NUM_TEAMS; i++)
    {
        if ((TeamInfo[i]) and ( not (TeamInfo[i]->flags bitand TEAM_UPDATED)))
            return retval;
    }

    if (TheCampaign.Flags bitand CAMP_SLAVE)
    {
        TheCampaign.Flags and_eq compl CAMP_NEED_TEAM_DATA;
        TheCampaign.GotJoinData();
    }

    return retval;
}

int TeamClass::Handle(VuPositionUpdateEvent *event)
{
    return (VuEntity::Handle(event));
}

int TeamClass::Handle(VuEntityCollisionEvent *event)
{
    return (VuEntity::Handle(event));
}

int TeamClass::Handle(VuTransferEvent *event)
{
    return (VuEntity::Handle(event));
}

int TeamClass::Handle(VuSessionEvent *event)
{
    return (VuEntity::Handle(event));
}

VU_ERRCODE TeamClass::InsertionCallback(void)
{
    if (TeamInfo[who] not_eq this)
    {
        RemoveTeam(who);
        TeamInfo[who] = this;
        VuReferenceEntity(this);
    }

    return FalconEntity::InsertionCallback();
}

VU_ERRCODE TeamClass::RemovalCallback(void)
{
    if (TeamInfo[who] == this)
    {
        TeamInfo[who] = NULL;
        VuDeReferenceEntity(this);
    }

    return FalconEntity::RemovalCallback();
}

void TeamClass::SetName(_TCHAR *newname)
{
    strncpy(name, newname, MAX_TEAM_NAME_SIZE);
}

_TCHAR* TeamClass::GetName(void)
{
    return name;
}

void TeamClass::SetMotto(_TCHAR *newmotto)
{
    strncpy(teamMotto, newmotto, MAX_MOTTO_SIZE);
}

_TCHAR* TeamClass::GetMotto(void)
{
    return teamMotto;
}

void TeamClass::SelectGroundAction(void)
{
    // Find the highest priority primary objective
    Objective o, bo = NULL;
    POData pd;
    int best = 0, sup;
    Team t;
    TeamGndActionType enemyAction;

    if ( not (flags bitand TEAM_ACTIVE) or not IsLocal())
        return;

    // A.S. begin, 2001-12-09

    if (NewInitiativePoints)   // New trigers for new initiative point system
    {
        MIN_COUNTER_ATTACK_INITIATIVE = 50;
        MIN_FULL_OFFENSIVE_INITIATIVE = 55;
    }

    // end added section
    // Check for action timeout
    if (TheCampaign.CurrentTime > groundAction.actionTimeout)
    {
        // Go to consolidation action
        groundAction.actionType = GACTION_CONSOLIDATE;
    }

    // If we're not currently in an action, see if we can start one
    if (groundAction.actionType == GACTION_CONSOLIDATE and 
        POList and 
        initiative >= MIN_COUNTER_ATTACK_INITIATIVE and 
        TheCampaign.CurrentTime >= groundAction.actionTimeout + ACTION_RATE)
    {
        // Select our objective
        {
            VuListIterator poit(POList);
            o = GetFirstObjective(&poit);

            while (o)
            {
                pd = GetPOData(o);

                // KCK: I Hope Nearfront() is sufficient to allow the team to consolidate around this objective
                // before going on to the next. Maybe I should use distance to front
                if (pd->ground_priority[who] > best and (o->IsNearfront() or GetRoE(who, o->GetTeam(), ROE_GROUND_CAPTURE) == ROE_ALLOWED))
                {
                    bo = o;
                    best = pd->ground_priority[who];
                }

                o = GetNextObjective(&poit);
            }
        }

        if (bo)
        {
            // Offensive Action Yahoo
            if (initiative >= MIN_FULL_OFFENSIVE_INITIATIVE)
                groundAction.actionType = GACTION_OFFENSIVE;
            else
                groundAction.actionType = GACTION_MINOROFFENSIVE;

            groundAction.actionTime = TheCampaign.CurrentTime + ACTION_PREP_TIME;
            groundAction.actionTime = (int)(groundAction.actionTime / CampaignHours) * CampaignHours;
            groundAction.actionTimeout = TheCampaign.CurrentTime + ACTION_TIMEOUT;
            groundAction.actionObjective = bo->Id();
            groundAction.actionTempo = best / 2;
            groundAction.actionPoints = (uchar) initiative;
            sup = ((currentStats.groundVehs + currentStats.aircraft) * initiative) / 1000;

            if (supplyAvail < sup)
                supplyAvail = sup;

            if (fuelAvail < sup)
                fuelAvail = sup;

            // This is where we really cheat. We're going to add (initiative %) to both supply and strength.
            StartOffensive(who, initiative);
            //MakeTeamDirty (DIRTY_GROUND_ACTION, DDP[12].priority);
            MakeTeamDirty(DIRTY_GROUND_ACTION, SEND_EVENTUALLY);

            // Now force a defensive action on our enemies
            enemyAction.actionTime = groundAction.actionTime;
            enemyAction.actionObjective = bo->Id();
            enemyAction.actionTempo = groundAction.actionTempo;
            enemyAction.actionType = GACTION_DEFENSIVE;
            enemyAction.actionTimeout = TheCampaign.CurrentTime + ACTION_TIMEOUT;
            enemyAction.actionPoints = 0;

            for (t = 0; t < NUM_TEAMS; t++)
            {
                if (GetRoE(who, t, ROE_GROUND_CAPTURE) == ROE_ALLOWED)
                {
                    sup = ((TeamInfo[t]->GetCurrentStats()->groundVehs + TeamInfo[t]->GetCurrentStats()->aircraft) * TeamInfo[t]->GetInitiative()) / 1000;

                    if (TeamInfo[t]->supplyAvail > sup)
                        TeamInfo[t]->supplyAvail = sup;

                    if (TeamInfo[t]->fuelAvail > sup)
                        TeamInfo[t]->fuelAvail = sup;

                    TeamInfo[t]->SetGroundAction(&enemyAction);
                }
            }

            return;
        }
    }

    // If we're currently performing an action, check if it's still valid
    if (groundAction.actionType not_eq GACTION_CONSOLIDATE)
    {
        // Validate our current action
        bo = (Objective) vuDatabase->Find(groundAction.actionObjective);

        // A.S. begin, 2001-12-09  if initiative < 40 then consolidate
        if (NewInitiativePoints)
        {
            if (groundAction.actionPoints and bo and initiative >= 40)
                return; // We've still got umph, or havn't started yet, and havn't captured our objective
        }
        else   // *** old code ***
        {
            if (groundAction.actionPoints and bo) // and GetRoE(who,bo->GetTeam(),ROE_GROUND_CAPTURE) == ROE_ALLOWED)
                return; // We've still got umph, or havn't started yet, and havn't captured our objective
        }

        // end added section. This replaces( not ) the section marked with +++ old code +++



        if (groundAction.actionType == GACTION_DEFENSIVE)
        {
            // Validate enemy actions
            for (t = 0; t < NUM_TEAMS; t++)
            {
                if (GetRoE(t, who, ROE_GROUND_CAPTURE) == ROE_ALLOWED and (TeamInfo[t]->GetGroundActionType() == GACTION_OFFENSIVE or TeamInfo[t]->GetGroundActionType() == GACTION_MINOROFFENSIVE))
                    return;
            }
        }
        else
        {
            // We've captured/completed our goal. Reset timeout
            groundAction.actionTimeout = TheCampaign.CurrentTime;
        }
    }

    // Otherwise, nothing is going on currently (we're not attacking or being attacked.
    // Go to a consolidation action until we decide to go on the offensive, or are forced onto the
    // defensive by an enemy offensive.
    groundAction.actionType = GACTION_CONSOLIDATE;
    groundAction.actionTime = TheCampaign.CurrentTime;
    groundAction.actionObjective = FalconNullId;
    groundAction.actionTempo = 0;
    groundAction.actionPoints = 0;
    //MakeTeamDirty (DIRTY_GROUND_ACTION, DDP[13].priority);
    MakeTeamDirty(DIRTY_GROUND_ACTION, SEND_EVENTUALLY);
}

void TeamClass::SelectAirActions(void)
{
    // Find the highest priority primary objective
    Objective bo = NULL;
    POData pd;
    int oa, ta = 1, airRatio, action, missions_requested, paks = 0;
    Team t;
    ListNode lp;
    MissionRequest mis;
    CampaignTime current_time;

    if ( not (flags bitand TEAM_ACTIVE) or not IsLocal())
        return;

    if (TheCampaign.CurrentTime > defensiveAirAction.actionStopTime)
    {
        // Clear old defensive action
        defensiveAirAction.actionType = AACTION_NOTHING;
    }


    if (TheCampaign.CurrentTime < offensiveAirAction.actionStopTime)
        return;

    // Plan a new offensive action

    // Determine percentage of non-action offensive missions to fill during next action cycle, based on tempo
    missions_requested = 0;
    lp = atm->requestList->GetLastElement();

    while (lp)
    {
        mis = (MissionRequest) lp->GetUserData();

        if ( not mis->action_type and not (MissionData[mis->mission].flags bitand AMIS_FLYALWAYS))
            missions_requested++;

        lp = lp->GetPrev();
    }

    atm->missionsToFill = (missions_requested * groundAction.actionTempo) / 100;
    atm->missionsFilled = 0;

    // Get ratio of air forces
    oa = currentStats.aircraft;

    for (t = 0; t < NUM_TEAMS; t++)
    {
        if (GetRoE(who, t, ROE_AIR_FIRE) == ROE_ALLOWED)
            ta += TeamInfo[t]->currentStats.aircraft;
    }

    airRatio = (oa * 100) / ta;

    // Select Air Action Type
    // TJL 01/03/04 Eliminate the ratio for INTERDICT.  It was too high and never got there
    // OCA is now the offensive mode.  All target types are now available based on OCA mode.
    // See ATM.CPP
    //if (airRatio > 200)
    //
    //action = AACTION_INTERDICT; // If we can afford it, do interdiction action
    // else if (airRatio > 60)
    if (airRatio > 60)
        action = AACTION_OCA; // Otherwise try for air superiority with OCA action
    else
        action = AACTION_DCA; // Otherwise, defense only (this will not create an action)



    // Couple of special cases
    if ((groundAction.actionType == GACTION_OFFENSIVE) and (action == AACTION_DCA) and (airRatio > 40))
    {
        action = AACTION_OCA;
    }

    if (action == AACTION_DCA)
    {
        return;
    }

    // Now select a target PAK
    //TJL 01/03/04 Remove INTERDICT
    //if (action == AACTION_OCA or action == AACTION_INTERDICT)
    if (action == AACTION_OCA)
    {
        Objective o, fo;
        int best = -200, score, found, step, dist;
        float xd, yd;
        GridIndex tx, ty, fx, fy, x, y;
        List objectiveList;

        {
            VuListIterator poit(POList);
            o = GetFirstObjective(&poit);

            while (o)
            {
                if (GetRoE(who, o->GetTeam(), ROE_AIR_ATTACK) == ROE_ALLOWED)
                {
                    pd = GetPOData(o);

                    if (pd->player_priority[who] >= 0)
                        score = pd->player_priority[who];
                    else
                        score = pd->air_priority[who];

                    if (o->Id() == groundAction.actionObjective)
                        score += 50;

                    if (o->Id() == offensiveAirAction.lastActionObjective)
                        score -= 200;

                    if (score > best)
                    {
                        bo = o;
                        best = score;
                    }
                }

                o = GetNextObjective(&poit);
            }
        }

        if ( not bo)
            return;

        // Set up our action
        offensiveAirAction.actionStartTime = current_time = TheCampaign.CurrentTime + CampaignHours;
        offensiveAirAction.actionType = action;
        offensiveAirAction.actionObjective = bo->Id();
        offensiveAirAction.lastActionObjective = bo->Id();
        objectiveList = new ListClass(LADT_SORTED_LIST);

        // Calculate our clear path bitand build list of all additional target PAKs
        // This is a very rough way to find all PAKs along the path to our target PAK
        bo->GetLocation(&tx, &ty);
        fo = FindNearestObjective(FrontList, tx, ty, NULL);
        ShiAssert(fo);
        fo->GetLocation(&fx, &fy);
        dist = FloatToInt32(Distance(fx, fy, tx, ty));
        xd = (float)(tx - fx) / dist;
        yd = (float)(ty - fy) / dist;

        for (step = 20; step <= dist; step += 30)
        {
            x = fx + FloatToInt32(xd * step + 0.5F);
            y = fy + FloatToInt32(yd * step + 0.5F);
            VuListIterator poit(POList);
            o = GetFirstObjective(&poit);

            while (o)
            {
                o->GetLocation(&tx, &ty);

                if (o not_eq bo and DistSqu(x, y, tx, ty) < SIDE_CHECK_DISTANCE * SIDE_CHECK_DISTANCE)
                {
                    found = 0;
                    lp = objectiveList->GetFirstElement();

                    while (lp)
                    {
                        if (lp->GetUserData() == o)
                            found = 1;

                        lp = lp->GetNext();
                    }

                    if ( not found)
                    {
                        objectiveList->InsertNewElement(FloatToInt32(DistanceToFront(fx, fy)), o);
                        paks++;
                    }
                }

                o = GetNextObjective(&poit);
            }
        }

        // Target all PAKs
        lp = objectiveList->GetFirstElement();

        while (lp)
        {
            o = (Objective) lp->GetUserData();
            TargetAllSites(o, AACTION_OCA, who, current_time);
            current_time += 20 * CampaignMinutes;
            lp = lp->GetNext();
        }

        offensiveAirAction.actionStopTime = current_time + 30 * CampaignMinutes;
        TargetAllSites(bo, action, who, current_time);
        delete objectiveList;

        //MakeTeamDirty (DIRTY_OFFAIR_ACTION, DDP[14].priority);
        MakeTeamDirty(DIRTY_OFFAIR_ACTION, SEND_EVENTUALLY);

        // Warp enemy defensive action to this location.

        TeamInfo[bo->GetTeam()]->defensiveAirAction.actionType = AACTION_DCA;
        TeamInfo[bo->GetTeam()]->defensiveAirAction.actionObjective = bo->Id();
        TeamInfo[bo->GetTeam()]->defensiveAirAction.actionStartTime = current_time;
        TeamInfo[bo->GetTeam()]->defensiveAirAction.actionStopTime = offensiveAirAction.actionStopTime;

        //TeamInfo[bo->GetTeam()]->MakeTeamDirty (DIRTY_DEFAIR_ACTION, DDP[15].priority);
        TeamInfo[bo->GetTeam()]->MakeTeamDirty(DIRTY_DEFAIR_ACTION, SEND_EVENTUALLY);
    }
}

void TeamClass::SetGroundAction(TeamGndActionType *action)
{
    groundAction.actionTime = action->actionTime;
    groundAction.actionObjective = action->actionObjective;
    groundAction.actionType = action->actionType;
    groundAction.actionTimeout = action->actionTimeout;
    groundAction.actionTempo = action->actionTempo;
    groundAction.actionPoints = action->actionPoints;
    //MakeTeamDirty (DIRTY_GROUND_ACTION, DDP[16].priority);
    MakeTeamDirty(DIRTY_GROUND_ACTION, SEND_EVENTUALLY);
}

// =============================
// Global functions
// =============================

void AddTeam(int teamNum, int defaultStance)
{
    TeamClass *temp;
    AirTaskingManagerClass *atm;
    GroundTaskingManagerClass *gtm;
    NavalTaskingManagerClass *ntm;
    int tid, j;

    if (TeamInfo[teamNum])
    {
        RemoveTeam(teamNum);
    }

    tid = GetClassID(DOMAIN_ABSTRACT, CLASS_MANAGER, TYPE_TEAM, 0, 0, 0, 0, 0) + VU_LAST_ENTITY_TYPE;
    temp = new TeamClass(tid, teamNum);

    for (j = 0; j < NUM_TEAMS; j++)
    {
        if (teamNum not_eq j and teamNum and j)
            temp->stance[j] = defaultStance;
        else if (teamNum not_eq j)
            temp->stance[j] = Neutral;
        else
            temp->stance[j] = Allied;
    }

    vuDatabase->/*Silent*/Insert(temp);
    tid = GetClassID(DOMAIN_AIR, CLASS_MANAGER, TYPE_ATM, 0, 0, 0, 0, 0) + VU_LAST_ENTITY_TYPE;
    atm = new AirTaskingManagerClass(tid, teamNum);
    vuDatabase->/*Silent*/Insert(atm);
    VuReferenceEntity(atm);
    tid = GetClassID(DOMAIN_LAND, CLASS_MANAGER, TYPE_GTM, 0, 0, 0, 0, 0) + VU_LAST_ENTITY_TYPE;
    gtm = new GroundTaskingManagerClass(tid, teamNum);
    vuDatabase->/*Silent*/Insert(gtm);
    VuReferenceEntity(gtm);
    tid = GetClassID(DOMAIN_SEA, CLASS_MANAGER, TYPE_NTM, 0, 0, 0, 0, 0) + VU_LAST_ENTITY_TYPE;
    ntm = new NavalTaskingManagerClass(tid, teamNum);
    vuDatabase->/*Silent*/Insert(ntm);
    VuReferenceEntity(ntm);
}

// Creates team classes, with their associated managers
void AddNewTeams(RelType defaultStance)
{
    int i;
    RemoveTeams();

    for (i = 0; i < NUM_TEAMS; i++)
    {
        AddTeam(i, defaultStance);
    }
}

void RemoveTeam(int teamNum)
{
    if (TeamInfo[teamNum])
    {
        vuDatabase->Remove(TeamInfo[teamNum]->atm);
        VuDeReferenceEntity(TeamInfo[teamNum]->atm);
        vuDatabase->Remove(TeamInfo[teamNum]->gtm);
        VuDeReferenceEntity(TeamInfo[teamNum]->gtm);
        vuDatabase->Remove(TeamInfo[teamNum]->ntm);
        VuDeReferenceEntity(TeamInfo[teamNum]->ntm);
        vuDatabase->Remove(TeamInfo[teamNum]);
    }
}

// Cleans up the team stuff.
void RemoveTeams()
{
    int i;

    for (i = 0; i < NUM_TEAMS; i++)
    {
        RemoveTeam(i);
    }
}

// Loads team info from a file
int LoadTeams(char* scenario)
{
    int i;
    short teams;
    FILE *fp;
    TeamClass *temp;
    AirTaskingManagerClass *atm;
    GroundTaskingManagerClass *gtm;
    NavalTaskingManagerClass *ntm;

    RemoveTeams();

    // Set up the DIndex for our team manager class
    teamManagerDIndex = GetClassID(DOMAIN_ABSTRACT, CLASS_MANAGER, TYPE_TEAM, 0, 0, 0, 0, 0);

    if ( not teamManagerDIndex)
        return 0;

    teamManagerDIndex += VU_LAST_ENTITY_TYPE;

    if ((fp = OpenCampFile(scenario, "tea", "rb")) == NULL)
        return 0;

    fread(&teams, sizeof(short), 1, fp);

    if (teams > NUM_TEAMS)
        teams = NUM_TEAMS;

    for (i = 0; i < teams; i++)
    {
        temp = new TeamClass(fp);
        vuDatabase->/*Silent*/Insert(temp);
        atm = new AirTaskingManagerClass(fp);
        vuDatabase->/*Silent*/Insert(atm);
        VuReferenceEntity(atm);
        gtm = new GroundTaskingManagerClass(fp);
        vuDatabase->/*Silent*/Insert(gtm);
        VuReferenceEntity(gtm);
        ntm = new NavalTaskingManagerClass(fp);
        vuDatabase->/*Silent*/Insert(ntm);
        VuReferenceEntity(ntm);
    }

    // Removed to make Tactical Engagement's concept of teams work.
    // Should make no difference to Campaign. - RH

    // for (; i<NUM_TEAMS; i++)
    // AddTeam(i);

    CloseCampFile(fp);
    return 1;
}

int SaveTeams(char* scenario)
{
    FILE *fp;
    int i;
    short teams = 0;

    if ((fp = OpenCampFile(scenario, "tea", "wb")) == NULL)
    {
        return 0;
    }

    i = 0;

    while (i < NUM_TEAMS)
    {
        if (TeamInfo[i] and TeamInfo[i]->VuState() == VU_MEM_ACTIVE)
        {
            teams++;
        }

        i++;
    }

    fwrite(&teams, sizeof(short), 1, fp);
    i = 0;

    while (i < NUM_TEAMS)
    {
        if (TeamInfo[i] and TeamInfo[i]->VuState() == VU_MEM_ACTIVE)
        {
            TeamInfo[i]->Save(fp);
        }

        i++;
    }

    CloseCampFile(fp);
    return 1;
}

void LoadPriorityTables(void)
{
    FILE *fp;
    int n, t;

    memset(DefaultObjtypePriority, 0, TAT_CAS * MAX_TGTTYPE);
    memset(DefaultUnittypePriority, 0, TAT_CAS * MAX_UNITTYPE);
    memset(DefaultMissionPriority, 0, TAT_CAS * AMIS_OTHER);

    for (t = 0; t < TAT_CAS; t++)
    {
        switch (t + 1)
        {
            case TAT_DEFENSIVE:
                fp = OpenCampFile("defense", "pri", "r");
                break;

            case TAT_OFFENSIVE:
                fp = OpenCampFile("offense", "pri", "r");
                break;

            case TAT_ATTRITION:
                fp = OpenCampFile("attrit", "pri", "r");
                break;

            case TAT_CAS:
                fp = OpenCampFile("cas", "pri", "r");
                break;

            default:
                fp = OpenCampFile("intdict", "pri", "r");
                break;
        }

        ShiAssert(fp);
        n = atoi(CampGetNext(fp)); // # of objective target priorities

        while (n >= 0)
        {
            DefaultObjtypePriority[t][n] = atoi(CampGetNext(fp));
            n = atoi(CampGetNext(fp));
        }

        n = atoi(CampGetNext(fp)); // # of unit target priorities

        while (n >= 0)
        {
            DefaultUnittypePriority[t][n] = atoi(CampGetNext(fp));
            n = atoi(CampGetNext(fp));
        }

        n = atoi(CampGetNext(fp)); // # of mission priorities

        while (n >= 0)
        {
            DefaultMissionPriority[t][n] = atoi(CampGetNext(fp));
            n = atoi(CampGetNext(fp));
        }

        CloseCampFile(fp);
    }
}

// This is high level RoE. Lower level Rules of Engagement are dealt with on the unit level.
int GetRoE(Team a, Team b, int type)
{
    ShiAssert(TeamInfo[a]);

    if (TeamInfo[a])
    {
        return RoEData[type][TeamInfo[a]->stance[b]];
    }
    else
    {
        return ROE_ALLOWED;
    }
}

void TransferInitiative(Team from, Team to, int i)
{
    if ((TeamInfo[to]) and (TeamInfo[from]))
    {
        // MonoPrint ("Transfering %d initiative from %d to %d.\n",i,from,to);
        TeamInfo[to]->AddInitiative(i);
        TeamInfo[from]->AddInitiative(-i);
    }
}

//////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void TeamClass::AddInitiative(short i)
{
    initiative += i;

    if (initiative > 100) initiative = 100;

    if (initiative < 0) initiative = 0;

    //MakeTeamDirty (DIRTY_TEAM_INITIATIVE, DDP[17].priority);
    MakeTeamDirty(DIRTY_TEAM_INITIATIVE, SEND_EVENTUALLY);
}

//////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void TeamClass::AddReinforcement(short i)
{
    int min;

    // Calculate minimum reinforcement number
    // KCK TODO: Impliment a "start time"
    min = (TheCampaign.CurrentTime - 9 * CampaignHours) / CampaignHours;

    if (reinforcement + i < min)
        return;

    reinforcement += i;

    //MakeTeamDirty (DIRTY_TEAM_REINFORCEMENT, DDP[18].priority);
    MakeTeamDirty(DIRTY_TEAM_REINFORCEMENT, SEND_EVENTUALLY);
}

//////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

float AirExperienceAdjustment(Team t)
{
    return (float)(TeamInfo[t]->airExperience / 100.0F);
}

float AirDefenseExperienceAdjustment(Team t)
{
    return (float)(TeamInfo[t]->airDefenseExperience / 100.0F);
}

float GroundExperienceAdjustment(Team t)
{
    return (float)(TeamInfo[t]->groundExperience / 100.0F);
}

float NavalExperienceAdjustment(Team t)
{
    return (float)(TeamInfo[t]->navalExperience / 100.0F);
}

float CombatBonus(Team t, VU_ID poid)
{
    int i;
    CampaignTime currtime = Camp_GetCurrentTime();
    float bonus = 1.0F, mult;

    // Area bonuses
    if (poid not_eq FalconNullId)
    {
        for (i = 0; i < MAX_BONUSES; i++)
        {
            if (TeamInfo[t]->bonusObjs[i] == poid)
            {
                if (TeamInfo[t]->bonusTime[i] > currtime)
                {
                    // MonoPrint("Applying x3 specific bonus at %x.\n",poid.num_);
                    bonus *= 3.0F; // Tripple combat effects for positive time
                }
            }
        }
    }

    mult = ((TeamInfo[t]->playerRating + 20) / 20);
    // MonoPrint("Applying team %d %3.3f multiplier at %x.\n",t,mult,poid.num_);
    bonus = bonus * mult;
    return bonus;
}

void ApplyBonus(Team who, VU_ID poid, int rating)
{
    int i, applied = FALSE;

    // Apply a local combat bonus
    for (i = 0; i < MAX_BONUSES; i++)
    {
        if (TeamInfo[who]->bonusObjs[i] == poid)
        {
            TeamInfo[who]->bonusTime[i] += rating * 4 * CampaignMinutes;
            applied = TRUE;
        }

        if (TeamInfo[who]->bonusTime[i] < TheCampaign.CurrentTime)
            TeamInfo[who]->bonusObjs[i] = FalconNullId;
    }

    if (rating < 0 or applied)
        return;

    for (i = 0; i < MAX_BONUSES and not applied; i++)
    {
        if (TeamInfo[who]->bonusObjs[i] == FalconNullId)
        {
            TeamInfo[who]->bonusObjs[i] = poid;
            TeamInfo[who]->bonusTime[i] = TheCampaign.CurrentTime + (rating * 4 * CampaignMinutes);
            applied = TRUE;
        }
    }
}

// Expects rating from between -20 to 20 (20 being good)
void ApplyPlayerInput(Team who, VU_ID poid, int rating)
{
    ShiAssert(TeamInfo);

    if ( not TeamInfo)
        return;

    int i, os, ts, initiative, maxInitiative, initDelta, pts;
    Team et = NUM_TEAMS + 1;

    if (poid == FalconNullId)
        poid = TeamInfo[who]->GetGroundAction()->actionObjective;

    // Rolling average, weighted towards most recent
    TeamInfo[who]->playerRating = (TeamInfo[who]->playerRating * 4.0F + rating * 4.0F) / 8.0F;
    TeamInfo[who]->lastPlayerMission = TheCampaign.CurrentTime;

    // Shift initiative in our favor, if force ratios allow it.
    os = 0;

    if (TeamInfo[who]->GetCurrentStats())
        os = TeamInfo[who]->GetCurrentStats()->groundVehs;

    for (i = 0, ts = 0; i < NUM_TEAMS; i++)
    {
        if (GetTTRelations(i, who) == War)
        {
            ts += TeamInfo[i]->GetCurrentStats()->groundVehs;
            et = i;
        }
    }

    if (et >= NUM_TEAMS)
        return;

    if (os > ts)
        maxInitiative = 100;
    else if ( not os)
        maxInitiative = 0;
    else
        maxInitiative = (os * 100) / ts;

    // A.S. 2001-12-09 begin,
    if (NewInitiativePoints) // The above constraint, maxInitiative = (os*100)/ts, is no longer needed.
        maxInitiative = 100; // The new procedure NewInitiativePointSetting does the job.

    // end added section

    initiative = TeamInfo[who]->GetInitiative();
    initDelta = rating / 4;

    if (initiative + initDelta > maxInitiative)
        initDelta = maxInitiative - initiative;

    if (initiative + initDelta < 0)
        initDelta = 0 - initiative;

    TransferInitiative(et, who, initDelta);

    // Shift reinforcement schedule
    if (rating > 15)
        TeamInfo[who]->AddReinforcement(1);

    if (rating > 5)
        TeamInfo[et]->AddReinforcement(-1);

    if (rating < -5)
        TeamInfo[who]->AddReinforcement(-1);

    if (rating < -15)
        TeamInfo[et]->AddReinforcement(1);

    // A.S. debug
    //FILE *deb;
    //deb = fopen("c:\\temp\\deb3.txt", "a");
    //fprintf(deb, "Team %2d  Init = %3d  Rating = %3.2f  TIME = %d\n\n", who, TeamInfo[who]->GetInitiative(), TeamInfo[who]->playerRating, TheCampaign.CurrentTime/(3600*1000));
    //fclose(deb);
    // debug end

    // Apply a local combat bonus
    if (rating > 2 or rating < -2)
    {
        ApplyBonus(who, poid, rating);
        ApplyBonus(et, poid, -1 * rating);
    }

    // Modify our or our enemy's action points
    pts = TeamInfo[who]->GetGroundAction()->actionPoints + (rating / 2);

    if (pts > 0 and pts < 100)
        TeamInfo[who]->GetGroundAction()->actionPoints = pts;

    pts = TeamInfo[et]->GetGroundAction()->actionPoints - (rating / 2);

    if (pts > 0 and pts < 100)
        TeamInfo[et]->GetGroundAction()->actionPoints = pts;

    // MonoPrint("Applying player input: Rating: %d, Location: %d, i:%d,r:%3.3f\n",rating,poid.num_,TeamInfo[who]->GetInitiative(),TeamInfo[who]->playerRating);
}


// A.S. begin 2001-12-09, New Procedure for Initiative Points
void NewInitiativePointSetting(Team who)
{

    int i, os, ts, initiative, initiative_old;
    int oa, ta, os_start, oa_start, ts_start, ta_start;
    int longRunInitiative, longRunInitiative1, longRunInitiative2, longRunInitiative3;
    float oloss, tloss;
    Team et = NUM_TEAMS + 1;

    os = TeamInfo[who]->GetCurrentStats()->groundVehs; // Calculating the strenght of the player's team relativ to the others
    oa = TeamInfo[who]->GetCurrentStats()->aircraft;
    os_start = TeamInfo[who]->startStats.groundVehs;
    oa_start = TeamInfo[who]->startStats.aircraft;

    for (i = 0, ts = 0, ta = 0, ts_start = 0, ta_start = 0; i < NUM_TEAMS; i++)
    {
        if (GetTTRelations(i, who) == War)
        {
            ts += TeamInfo[i]->GetCurrentStats()->groundVehs;
            ta += TeamInfo[i]->GetCurrentStats()->aircraft;
            ts_start += TeamInfo[i]->startStats.groundVehs;
            ta_start += TeamInfo[i]->startStats.aircraft;
            et = i;
        }
    }

    if (et >= NUM_TEAMS)
        return;

    oloss = (os_start * 1.0f - os * 1.0f);

    if (oloss == 0)
        oloss = 1;

    tloss = (ts_start * 1.0f - ts * 1.0f);

    if (tloss == 0)
        tloss = 1;

    longRunInitiative1 = (int)(atan((os * 1.0) / ts) * 100 / (3.142 / 2) + 1); // We need a function, which maps [0, +inf] into [0, 100].
    longRunInitiative2 = (int)(atan((oa * 1.0) / ta) * 100 / (3.142 / 2) + 1); // The ArcusTangens-function is used for this purpose. We have to divide by Pi/2.
    longRunInitiative3 = (int)(atan(tloss / oloss) * 100 / (3.142 / 2) + 1); // e.g., if the relative strenght is 1, longRunInitiative = 50.
    longRunInitiative = (5 * longRunInitiative1 + 4 * longRunInitiative2 + 1 * longRunInitiative3) / 10;


    initiative = TeamInfo[who]->GetInitiative();
    initiative_old = initiative;
    initiative = (int)(0.5F * initiative  +  0.5F * longRunInitiative);   // Adaptive mechanism to long run equilibrium.

    TeamInfo[who]->SetInitiative(initiative);
    TeamInfo[et]->SetInitiative((100 - initiative)); // enemy team gets 100 minus our initiative points

    //debug
    //FILE *deb;
    //deb = fopen("c:\\temp\\deb2.txt", "a");
    //fprintf(deb, "Team %2d  Init = %3d  Init_old %d  LRInit = %3d  osts = %f  adta = %f  TIME = %d\n", who, TeamInfo[who]->GetInitiative(), initiative_old, longRunInitiative, (os*1.0)/ts, (oa*1.0)/ta, TheCampaign.CurrentTime/(3600*1000));
    //fprintf(deb, "Aircraft_start =  %d %d TheirLoss = %5.1f GroungVehicle_start = %d %d %5.1f\n", oa_start, oa, tloss, os_start, os, oloss);
    //fprintf(deb, "Team %d  Rating = %3.2f    bitor    Team %d  Rating = %3.2f\n\n", who, TeamInfo[who]->playerRating, et, TeamInfo[et]->playerRating);
    //fclose(deb);
    // end debug


    for (i = 0; i < NUM_TEAMS; i++) // old code for the other teams (replaces same code in UpdateTeamStatistics-procedure)
    {
        if (i not_eq who and i not_eq et and TeamInfo[i]->flags bitand TEAM_ACTIVE)
        {
            if (TeamInfo[i]->GetInitiative() < 40)
                TeamInfo[i]->AddInitiative((INITIATIVE_LEAK_PER_HOUR * MIN_RECALCULATE_STATISTICS) / 60);
            else if (TeamInfo[i]->GetInitiative() <= 100)  // 100 instead of 60
                TeamInfo[i]->AddInitiative(-(INITIATIVE_LEAK_PER_HOUR * MIN_RECALCULATE_STATISTICS) / 60);
        }
    }

}
// end added section



/*
   void SetCombatBonus(Team who, VU_ID poid, int rating)
   {
   int i;
   CampaignTime time = (rating - 2) * 15 * CampaignMinutes;
   CampaignTime currtime = Camp_GetCurrentTime();

// Give an initiative bonus/penalty
if (rating > 0)
TeamInfo[who]->AddInitiative(rating * 5);
else if (rating < 0)
TeamInfo[who]->AddInitiative (rating * 2);

// KCK TODO: Need a slight global bonus

// Reinforcement bonus on excellent rating
if (rating == Excellent)
TeamInfo[who]->AddReinforcement (1);

// First pass - look for existing entry
for (i=0; i<MAX_BONUSES; i++)
{
if (TeamInfo[who]->bonusObjs[i] == poid)
{
if (TeamInfo[who]->bonusTime[i] >= 0)
{
if (TeamInfo[who]->bonusTime[i] > currtime and time > 0)
TeamInfo[who]->bonusTime[i] += time;
else if (TeamInfo[who]->bonusTime[i] > currtime and time < 0)
TeamInfo[who]->bonusTime[i] = -currtime + time + (TeamInfo[who]->bonusTime[i]-currtime);
else if (TeamInfo[who]->bonusTime[i] < currtime and time > 0)
TeamInfo[who]->bonusTime[i] = currtime + time;
else if (TeamInfo[who]->bonusTime[i] < currtime and time < 0)
TeamInfo[who]->bonusTime[i] = -currtime + time;
}
else
{
if (TeamInfo[who]->bonusTime[i] > -currtime and time > 0)
TeamInfo[who]->bonusTime[i] = currtime + time;
else if (TeamInfo[who]->bonusTime[i] > -currtime and time < 0)
TeamInfo[who]->bonusTime[i] = -currtime + time;
else if (TeamInfo[who]->bonusTime[i] < -currtime and time > 0)
TeamInfo[who]->bonusTime[i] = currtime + time + (TeamInfo[who]->bonusTime[i]+currtime);
else if (TeamInfo[who]->bonusTime[i] < -currtime and time < 0)
TeamInfo[who]->bonusTime[i] += time;
}
return;
}
}
// Second pass - look for empty entry
for (i=0; i<MAX_BONUSES; i++)
{
if (TeamInfo[who]->bonusObjs[i] == FalconNullId)
{
TeamInfo[who]->bonusObjs[i] = poid;
if (time > 0)
TeamInfo[who]->bonusTime[i] = currtime+time;
else
TeamInfo[who]->bonusTime[i] = -currtime+time;
return;
}
}
}
 */

Team GetEnemyTeam(Team who)
{
    Team enemy, best = 0;

    ShiAssert(who < NUM_TEAMS);

    for (enemy = 1; enemy < NUM_TEAMS; enemy++)
    {
        if (GetTTRelations(who, enemy) == War)
        {
            return enemy;
        }

        if (GetTTRelations(who, enemy) == Hostile)
        {
            best = enemy;
        }
    }

    return best;
}

// Returns priority of this mission depending on the team, the mission and the target
int GetPriority(MissionRequest mis)
{
    int priority, d, bonus;
    int target_priority = 0, mission_priority, random_priority, pak_priority = 0, distance_priority = 0;
    Objective po = NULL;
    POData pd;
    CampEntity e;

    bonus = mis->priority;
    mis->priority = 0;
    e = (CampEntity) vuDatabase->Find(mis->targetID);

    if (e and e->IsPackage())
        e = ((Package)e)->GetFirstUnitElement();

    // KCK: Make sure we ignore any requests if the player has specified 0 priority
    if ( not TeamInfo[mis->who]->GetMissionPriority(mis->mission))
        return -1;

    // Mission priority (0 - 100)
    if (mis->flags bitand REQF_PART_OF_ACTION)
        mission_priority = DefaultMissionPriority[mis->action_type - 1][mis->mission];
    else
        mission_priority = TeamInfo[mis->who]->GetMissionPriority(mis->mission);

    // Target Priority (0 - 100)
    if ( not e)
    {
        // Take the mission priority if no target
        target_priority = mission_priority;
        po = FindNearestObjective(POList, mis->tx, mis->ty, NULL);
    }
    else if (e->IsObjective())
    {
        // KCK: Make sure we ignore any requests if the player has specified 0 priority
        if ( not TeamInfo[mis->who]->GetObjTypePriority(e->GetType()))
            return -1;

        // o Objective Target Type component (0-50)
        if (mis->flags bitand REQF_PART_OF_ACTION)
            target_priority = DefaultObjtypePriority[mis->action_type - 1][e->GetType()] / 2;
        else
            target_priority = TeamInfo[mis->who]->GetObjTypePriority(e->GetType()) / 2;

        // o Objective Priority component (0-50)
        target_priority += (target_priority * ((Objective)e)->GetObjectivePriority()) / 100;
        po = ((Objective)e)->GetObjectivePrimary();

        if ( not po)
            po = FindNearestObjective(POList, mis->tx, mis->ty, NULL);
    }
    else if (e->IsUnit() and e->GetDomain() == DOMAIN_LAND)
    {
        // KCK: Make sure we ignore any requests if the player has specified 0 priority
        if ( not TeamInfo[mis->who]->GetUnitTypePriority(e->GetSType()))
            return -1;

        // o Unit Target Type component (0-50)
        if (mis->flags bitand REQF_PART_OF_ACTION)
            target_priority = DefaultUnittypePriority[mis->action_type - 1][e->GetSType()] / 2;
        else
            target_priority = TeamInfo[mis->who]->GetUnitTypePriority(e->GetSType()) / 2;

        // o Unit size/range component (0-50)
        if (e->GetSType() == STYPE_UNIT_AIR_DEFENSE)
            target_priority = (target_priority + e->GetAproxWeaponRange(Air) * 2) / 2;
        else
            target_priority = (target_priority * ((Unit)e)->GetTotalVehicles()) / 50;

        po = FindNearestObjective(POList, mis->tx, mis->ty, NULL);
    }
    else if (e->IsTaskForce())
    {
        // o TaskForce component (0-100)
        target_priority = TeamInfo[mis->who]->GetUnitTypePriority(MAX_UNITTYPE - 1);

        // KCK: Make sure we ignore any requests if the player has specified 0 priority
        if ( not target_priority)
            return -1;

        po = FindNearestObjective(POList, mis->tx, mis->ty, NULL);
    }
    else
    {
        // Default
        target_priority = mission_priority;
        po = FindNearestObjective(POList, mis->tx, mis->ty, NULL);
    }

    if ( not po)
        return -1;

    // Pak priority (0 - 100)
    pd = GetPOData(po);

    if ( not pd)
        return -1;

    if (pd->player_priority[mis->who] >= 0)
    {
        pak_priority = pd->player_priority[mis->who];

        // KCK: Make sure we ignore any requests if the player has specified 0 priority
        if ( not pak_priority)
            return -1;
    }
    else
        pak_priority = pd->air_priority[mis->who];

    mis->pakID = po->Id();

    // Distance priority (0 - 200)
    if (MissionData[mis->mission].flags bitand AMIS_NO_DIST_BONUS)
        distance_priority = mission_priority * 2;
    else
    {
        d = FloatToInt32(DistanceToFront(mis->tx, mis->ty));

        if (d < 50)
            distance_priority = 100 + (50 - d) * 2; // 100 - 200
        else if (d < 250)
            distance_priority = 100 - (d - 50) / 2; // 0 - 100
        else
            distance_priority = 0; // 0

        if (d > 100 and not mis->action_type and mis->mission == AMIS_SWEEP)
        {
            priority = -1;
            return -1;
        }
    }

    // Random priority (0 - 50)
    random_priority = rand() % 50;

    // KCK HACK:
    // Special case adjustments (it'd be nice to do this to the priorities themselves)
    if ((mis->mission == AMIS_CAS or mis->mission == AMIS_ONCALLCAS or mis->mission == AMIS_PRPLANCAS or mis->mission == AMIS_BAI) and mis->vs)
    {
        // Bonus/penalty based on ground force ratios.
        float ratio = (float)sqrt((float)TeamInfo[mis->who]->GetCurrentStats()->groundVehs / (float)TeamInfo[mis->vs]->GetCurrentStats()->groundVehs);
        target_priority = FloatToInt32(ratio * target_priority);
        mission_priority = FloatToInt32(ratio * mission_priority);
    }

    // Total priority (0 - 550) + bonus
    priority = bonus + target_priority + mission_priority + pak_priority + distance_priority + random_priority;

    // Scaled roughly to 0-255
    priority = (priority * 300) / 500;

    if (priority > 255)
        priority = 255;

    if (priority < 0)
        priority = 0;

    mis->priority = priority;

#ifdef DEBUG
    // Keep statistics on priorities
    strcpy(Priorities[mis->mission].name, MissStr[mis->mission]);
    Priorities[mis->mission].number_queried++;
    Priorities[mis->mission].total_score += priority;
    Priorities[mis->mission].total_distance += distance_priority;
    Priorities[mis->mission].total_target += target_priority;
    Priorities[mis->mission].total_mission += mission_priority;
    Priorities[mis->mission].total_random += random_priority;
    Priorities[mis->mission].total_pak += pak_priority;
    Priorities[mis->mission].total_bonus += bonus;;
    Priorities[mis->mission].average_score = Priorities[mis->mission].total_score / Priorities[mis->mission].number_queried;
    Priorities[mis->mission].average_distance = Priorities[mis->mission].total_distance / Priorities[mis->mission].number_queried;
    Priorities[mis->mission].average_target = Priorities[mis->mission].total_target / Priorities[mis->mission].number_queried;
    Priorities[mis->mission].average_mission = Priorities[mis->mission].total_mission / Priorities[mis->mission].number_queried;
    Priorities[mis->mission].average_random = Priorities[mis->mission].total_random / Priorities[mis->mission].number_queried;
    Priorities[mis->mission].average_pak = Priorities[mis->mission].total_pak / Priorities[mis->mission].number_queried;
    Priorities[mis->mission].average_bonus = Priorities[mis->mission].total_bonus / Priorities[mis->mission].number_queried;
    RemoveFromSortedList(&Priorities[mis->mission]);
    InsertIntoSortedList(&Priorities[mis->mission]);

    if (pak_priority > MaxPakPriority)
        MaxPakPriority = pak_priority;

    if (pak_priority < MinPakPriority)
        MinPakPriority = pak_priority;

#endif
    return priority;
}

void AddReinforcements(Team who, int inc)
{
    Unit u, e;
    Objective o;
    GridIndex x, y;
    int added;

    if ((inc <= 0) or (TeamInfo[who] == NULL) or ( not (TeamInfo[who]->flags bitand TEAM_ACTIVE)))
        return;

    TeamInfo[who]->AddReinforcement(inc);
    // Traverse reinforcement list, adding new units
    VuListIterator myit(InactiveList);
    u = GetFirstUnit(&myit);

    while (u)
    {
        // Activate any waiting reinforcements (note: cargoed units are inactive too, so keep an eye out)
        if (u->GetTeam() == who and not u->Cargo() and u->GetUnitReinforcementLevel() <= TeamInfo[who]->GetReinforcement() and u->Parent())
        {
            added = 0;

            if (u->IsBrigade())
            {
                e = u->GetFirstUnitElement();
            }
            else
            {
                e = u;
            }

            while (e)
            {
                e->GetLocation(&x, &y);
                // RV - Biker - 100km is too big perimeter (carrier near coast)
                o = FindNearestObjective(x, y, NULL, 25);

                if ( not o or o->GetTeam() == who)
                {
                    // Activate this unit and force list reinsertion
                    e->BroadcastUnitMessage(e->Id(), FalconUnitMessage::unitActivate, 0, 0, 0);
#ifdef DEBUG
                    gReinforcementsAdded[who]++;
#endif
                    added++;
                }

                if (u->IsBrigade())
                    e = u->GetNextUnitElement();
                else
                    e = NULL;
            }

            if (added)
            {
                if (u->IsBrigade())
                    u->BroadcastUnitMessage(u->Id(), FalconUnitMessage::unitActivate, 0, 0, 0);

                // Do a news event?
                FalconCampEventMessage *newEvent = new FalconCampEventMessage(u->Id(), FalconLocalGame);
                newEvent->dataBlock.team = who;
                newEvent->dataBlock.eventType = FalconCampEventMessage::unitReinforcement;
                u->GetLocation(&newEvent->dataBlock.data.xLoc, &newEvent->dataBlock.data.yLoc);
                newEvent->dataBlock.data.formatId = 1850;
                newEvent->dataBlock.data.owners[0] = u->GetOwner();
                SendCampUIMessage(newEvent);
            }
        }

        u = GetNextUnit(&myit);
    }
}

void UpdateTeamStatistics(void)
{
    Unit u;
    int i, ths, thf;
    int shave[NUM_TEAMS], swant[NUM_TEAMS], fhave[NUM_TEAMS], fwant[NUM_TEAMS];

    // Clear vehicle counts
    for (i = 0; i < NUM_TEAMS; i++)
    {
        if ( not TeamInfo[i])
            return;

        TeamInfo[i]->SetCurrentStats()->airDefenseVehs = 0;
        TeamInfo[i]->SetCurrentStats()->aircraft = 0;
        TeamInfo[i]->SetCurrentStats()->groundVehs = 0;
        TeamInfo[i]->SetCurrentStats()->ships = 0;
        TeamInfo[i]->SetCurrentStats()->supply = 0;
        TeamInfo[i]->SetCurrentStats()->fuel = 0;
        shave[i] = swant[i] = fhave[i] = fwant[i] = 0;
    }

    // Add units to count
    {
        VuListIterator uit(AllUnitList);
        u = GetFirstUnit(&uit);

        while (u)
        {
            // ALFREDs FIX - ignore brigades as they are counted already.
            if (u->IsBrigade())
            {
                u = GetNextUnit(&uit);
                continue;
            }

            if (u->GetDomain() == DOMAIN_LAND)
            {
                if (u->GetUnitNormalRole() == GRO_AIRDEFENSE)
                    TeamInfo[u->GetTeam()]->SetCurrentStats()->airDefenseVehs += u->GetTotalVehicles();
                else
                    TeamInfo[u->GetTeam()]->SetCurrentStats()->groundVehs += u->GetTotalVehicles();
            }
            else if (u->GetDomain() == DOMAIN_AIR)
            {
                // RV - Biker - Only count attack AC for statistic
                if (u->GetType() == TYPE_SQUADRON and 
                    (u->GetSType() == STYPE_UNIT_ATTACK or
                     u->GetSType() == STYPE_UNIT_ATTACK_HELO or
                     u->GetSType() == STYPE_UNIT_BOMBER or
                     u->GetSType() == STYPE_UNIT_FIGHTER or
                     u->GetSType() == STYPE_UNIT_FIGHTER_BOMBER))
                {
                    if (u->GetUnitAirbase() == NULL or u->GetSType() == STYPE_UNIT_ATTACK_HELO or not u->GetUnitAirbase()->IsObjective())
                        TeamInfo[u->GetTeam()]->SetCurrentStats()->aircraft += u->GetTotalVehicles();
                    else
                    {
                        Objective o = (Objective)u->GetUnitAirbase();
                        TeamInfo[u->GetTeam()]->SetCurrentStats()->aircraft += int(u->GetTotalVehicles() * o->GetObjectiveStatus() / 100);
                    }
                }
            }
            else if (u->GetDomain() == DOMAIN_SEA)
            {
                TeamInfo[u->GetTeam()]->SetCurrentStats()->ships += u->GetTotalVehicles();
            }

            if (u->GetDomain() == DOMAIN_LAND or u->IsSquadron())
            {
                i = u->GetTeam();
                ths = u->GetUnitSupplyNeed(TRUE);
                thf = u->GetUnitFuelNeed(TRUE);
                shave[i] += ths;
                fhave[i] += thf;
                swant[i] += ths + u->GetUnitSupplyNeed(FALSE);
                fwant[i] += thf + u->GetUnitFuelNeed(FALSE);
            }

            u = GetNextUnit(&uit);
        }
    }

    // If we havn't gotten a start score yet, set it.
    for (i = 0; i < NUM_TEAMS; i++)
    {
        if (TeamInfo[i]->flags bitand TEAM_ACTIVE)
        {
            // Do supply and fuel
            TeamInfo[i]->SetCurrentStats()->supply = TeamInfo[i]->GetSupplyAvail();
            TeamInfo[i]->SetCurrentStats()->fuel = TeamInfo[i]->GetFuelAvail();

            if (TeamInfo[i]->startStats.airDefenseVehs < TeamInfo[i]->GetCurrentStats()->airDefenseVehs)
                TeamInfo[i]->startStats.airDefenseVehs = TeamInfo[i]->GetCurrentStats()->airDefenseVehs;

            if (TeamInfo[i]->startStats.aircraft < TeamInfo[i]->GetCurrentStats()->aircraft)
                TeamInfo[i]->startStats.aircraft = TeamInfo[i]->GetCurrentStats()->aircraft;

            if (TeamInfo[i]->startStats.groundVehs < TeamInfo[i]->GetCurrentStats()->groundVehs)
                TeamInfo[i]->startStats.groundVehs = TeamInfo[i]->GetCurrentStats()->groundVehs;

            if (TeamInfo[i]->startStats.ships < TeamInfo[i]->GetCurrentStats()->ships)
                TeamInfo[i]->startStats.ships = TeamInfo[i]->GetCurrentStats()->ships;

            if (TeamInfo[i]->startStats.supply < TeamInfo[i]->GetCurrentStats()->supply)
                TeamInfo[i]->startStats.supply = TeamInfo[i]->GetCurrentStats()->supply;

            if (TeamInfo[i]->startStats.fuel < TeamInfo[i]->GetCurrentStats()->fuel)
                TeamInfo[i]->startStats.fuel = TeamInfo[i]->GetCurrentStats()->fuel;

            if (TeamInfo[i]->startStats.airbases < TeamInfo[i]->GetCurrentStats()->airbases)
                TeamInfo[i]->startStats.airbases = TeamInfo[i]->GetCurrentStats()->airbases;

            // A.S. 2001-12-09
            if ( not NewInitiativePoints) // The new procedure NewInitiativePointSetting does the job.
            {
                // same as old code
                if (TeamInfo[i]->GetInitiative() < 40)
                    TeamInfo[i]->AddInitiative((INITIATIVE_LEAK_PER_HOUR * MIN_RECALCULATE_STATISTICS) / 60);
                else if (TeamInfo[i]->GetInitiative() < 60)
                    TeamInfo[i]->AddInitiative(-(INITIATIVE_LEAK_PER_HOUR * MIN_RECALCULATE_STATISTICS) / 60);
            }

            // end section

            /* A.S.
            // +++ old code +++ begin
            // Adjust initiative
            if (TeamInfo[i]->GetInitiative() < 40)
            TeamInfo[i]->AddInitiative ((INITIATIVE_LEAK_PER_HOUR * MIN_RECALCULATE_STATISTICS) / 60);
            else if (TeamInfo[i]->GetInitiative() < 60)
            TeamInfo[i]->AddInitiative (-(INITIATIVE_LEAK_PER_HOUR * MIN_RECALCULATE_STATISTICS) / 60);
            // +++ old code +++ end
             */

            // Calculate current supply percentages
            if (swant[i])
                TeamInfo[i]->SetCurrentStats()->supplyLevel = shave[i] * 100 / swant[i];
            else if (shave[i])
                TeamInfo[i]->SetCurrentStats()->supplyLevel = 100;
            else
                TeamInfo[i]->SetCurrentStats()->supplyLevel = 0;

            if (fwant[i])
                TeamInfo[i]->SetCurrentStats()->fuelLevel = fhave[i] * 100 / fwant[i];
            else if (fhave[i])
                TeamInfo[i]->SetCurrentStats()->fuelLevel = 100;
            else
                TeamInfo[i]->SetCurrentStats()->fuelLevel = 0;
        }
        else
        {
            // Mark as non-existant
            TeamInfo[i]->SetCurrentStats()->supplyLevel = 255;
        }
    }

    if (FalconLocalSession->GetTeam() < NUM_TEAMS)
        GetTeamSituation(FalconLocalSession->GetTeam());

    // A.S. 2001-12-09. Call of the new initiative points procedure
    if (NewInitiativePoints)
        NewInitiativePointSetting(FalconLocalSession->GetTeam());

    // end added section

    // Write data to file
    CampaignTime timestamp;
    FILE *fp;
    short d, count;
    UnitHistoryType hist;

    // SPLIT SAVE FILE into 2 FILES SO I DON'T HAVE TO READ bitand DISCARD A BUNCH OF DATA
    // Teaminfo => .frc
    // History  => .his
    // PJW

    CampEnterCriticalSection();
    TheCampaign.TimeStamp++;

    if (TheCampaign.TimeStamp > 24)
        TheCampaign.TimeStamp = 0;

    fp = OpenCampFile("tmp", "frc", "ab");

    if (fp)
    {
        fseek(fp, 0, SEEK_END);
        timestamp = Camp_GetCurrentTime();
        fwrite(&timestamp, sizeof(CampaignTime), 1, fp);
        d = NUM_TEAMS;
        fwrite(&d, sizeof(short), 1, fp);

        for (i = 0; i < NUM_TEAMS; i++)
            fwrite(TeamInfo[i]->SetCurrentStats(), sizeof(TeamStatusType), 1, fp);

        CloseCampFile(fp);
    }

    fp = OpenCampFile("tmp", "his", "ab");

    if (fp)
    {
        fseek(fp, 0, SEEK_END);
        timestamp = Camp_GetCurrentTime();
        fwrite(&timestamp, sizeof(CampaignTime), 1, fp);
        // Count brigades and write to file
        count = 0;
        {
            VuListIterator uit(AllUnitList);
            u = GetFirstUnit(&uit);

            while (u)
            {
                if (u->IsBattalion())
                    count++;

                u = GetNextUnit(&uit);
            }
        }

        fwrite(&count, sizeof(short), 1, fp);
        {
            VuListIterator uit(AllUnitList);
            u = GetFirstUnit(&uit);

            while (u)
            {
                if (u->IsBattalion())
                {
                    u->GetLocation(&hist.x, &hist.y);
                    hist.team = u->GetTeam();
                    fwrite(&hist, sizeof(UnitHistoryType), 1, fp);
                }

                u = GetNextUnit(&uit);
            }
        }
        CloseCampFile(fp);
    }

    CampLeaveCriticalSection();

    // Apply player input if player hasn't flown any missions in the last hour
    /* if (TeamInfo[FalconLocalSession->GetTeam()]->lastPlayerMission - TheCampaign.CurrentTime > g_nNoPlayerPlay*CampaignHours)
     ApplyPlayerInput(FalconLocalSession->GetTeam(),FalconNullId,-10);*/

    // Fix by Alfred, ApplyPlayerInput is called every hour
    int rating;

    if (TheCampaign.CurrentTime < CampaignHours * 10)
        rating = 0;
    else        // don't apply input at campaign start
        rating = -10;

    if (TheCampaign.CurrentTime - TeamInfo[FalconLocalSession->GetTeam()]->lastPlayerMission >
        (unsigned int)g_nNoPlayerPlay * CampaignHours)
    {
        ApplyPlayerInput(FalconLocalSession->GetTeam(), FalconNullId, rating);
    }
}

// Returns a value 0-4 for team situation (0 is bad)
int GetTeamSituation(Team t)
{
    /* int i;
     float etotal=0.0F,mytotal=0.0F;

     for (i=0; i<NUM_TEAMS; i++)
     {
     if ((GetTTRelations(t,i) == War) and (TeamInfo[i]))
     {
     etotal += TeamInfo[i]->GetCurrentStats()->airDefenseVehs +
     TeamInfo[i]->GetCurrentStats()->aircraft * 2.0F +
     TeamInfo[i]->GetCurrentStats()->groundVehs +
     TeamInfo[i]->GetCurrentStats()->ships * 5.0F +
     TeamInfo[i]->GetCurrentStats()->supplyLevel * 100.0F;
     TeamInfo[i]->GetCurrentStats()->fuelLevel * 100.0F;
     }
     if ((GetTTRelations(t,i) == Allied or t == i) and (TeamInfo[i]))
     {
     mytotal += TeamInfo[i]->GetCurrentStats()->airDefenseVehs +
     TeamInfo[i]->GetCurrentStats()->aircraft * 2.0F +
     TeamInfo[i]->GetCurrentStats()->groundVehs +
     TeamInfo[i]->GetCurrentStats()->ships * 5.0F +
     TeamInfo[i]->GetCurrentStats()->supplyLevel * 100.0F;
     TeamInfo[i]->GetCurrentStats()->fuelLevel * 100.0F;
     }
     }
     if (mytotal * 1.6F < etotal)
     TheCampaign.Situation = 0;
     else if (mytotal * 1.2F < etotal)
     TheCampaign.Situation = 1;
     else if (mytotal < etotal * 1.2F)
     TheCampaign.Situation = 2;
     else if (mytotal < etotal * 1.6F)
     TheCampaign.Situation = 3;
     else
     TheCampaign.Situation = 4;
     */
    return TheCampaign.Situation;
}

// KCK: TODO
int NavalSuperiority(Team who)
{
    return 0;
}

int AirSuperiority(Team who)
{
    return 0;
}

// File reader that allows for comments in a text file
char *CampGetNext(FILE* fptr)
{
    static char aline[160];
    int is_comment;

    // Can we read
    F4Assert(fptr);

    do
    {
        fscanf(fptr, "%s", aline);
        SwapCRLF(aline);

        if (aline[0] == ';' or aline[0] == '#')
        {
            if (fgets(aline, 160, fptr) == NULL)
                break;

            is_comment = TRUE;
        }
        else
            is_comment = FALSE;
    }
    while (is_comment);

    return (aline);
}

#ifdef DEBUG
// =====================================================
// Statistics crap
// =====================================================

void InsertIntoSortedList(priority_structure *el)
{
    priority_structure *tmp, *last = NULL;

    if ( not PriorityList)
    {
        // Front of list
        PriorityList = el;
        return;
    }

    tmp = PriorityList;

    while (tmp and tmp->average_score > el->average_score)
    {
        last = tmp;
        tmp = tmp->next;
    }

    if ( not tmp)
    {
        // Back of list
        ShiAssert(last);
        ShiAssert(last->next == NULL);
        last->next = el;
        el->prev = last;
        ShiAssert(el->next not_eq el);
        ShiAssert(el->prev not_eq el);
        ShiAssert(last->next not_eq last);
        return;
    }

    el->prev = tmp->prev;
    tmp->prev = el;
    el->next = tmp;

    if (el->prev)
        el->prev->next = el;

    if (tmp == PriorityList)
        PriorityList = el;

    ShiAssert(el->next not_eq el);
    ShiAssert(el->prev not_eq el);
    ShiAssert(tmp->prev not_eq tmp);
}

void RemoveFromSortedList(priority_structure *el)
{
    if (el->prev)
    {
        el->prev->next = el->next;
        ShiAssert(el->prev->next not_eq el->prev);
    }

    if (el->next)
    {
        el->next->prev = el->prev;
        ShiAssert(el->next->prev not_eq el->next);
    }

    if (el == PriorityList)
    {
        PriorityList = el->next;
        ShiAssert(el->prev == NULL);
    }

    el->prev = NULL;
    el->next = NULL;
}

#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void TeamClass::SetMissionPriority(int i, uchar c)
{
    mission_priority[i] = c;

    //MakeTeamDirty (DIRTY_MISSION_PRIORITY, DDP[19].priority);
    MakeTeamDirty(DIRTY_MISSION_PRIORITY, SEND_EVENTUALLY);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

uchar *TeamClass::SetAllMissionPriority(void)
{
    //MakeTeamDirty (DIRTY_MISSION_PRIORITY, DDP[20].priority);
    MakeTeamDirty(DIRTY_MISSION_PRIORITY, SEND_EVENTUALLY);

    return mission_priority;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void TeamClass::SetUnitTypePriority(int i, uchar c)
{
    unittype_priority[i] = c;

    //MakeTeamDirty (DIRTY_UNITTYPE_PRIORITY, DDP[21].priority);
    MakeTeamDirty(DIRTY_UNITTYPE_PRIORITY, SEND_EVENTUALLY);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

uchar *TeamClass::SetAllUnitTypePriority(void)
{
    //MakeTeamDirty (DIRTY_UNITTYPE_PRIORITY, DDP[22].priority);
    MakeTeamDirty(DIRTY_UNITTYPE_PRIORITY, SEND_EVENTUALLY);

    return unittype_priority;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void TeamClass::SetObjTypePriority(int i, uchar c)
{
    //MakeTeamDirty (DIRTY_OBJTYPE_PRIORITY, DDP[23].priority);
    MakeTeamDirty(DIRTY_OBJTYPE_PRIORITY, SEND_EVENTUALLY);

    objtype_priority[i] = c;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

uchar *TeamClass::SetAllObjTypePriority(void)
{
    //MakeTeamDirty (DIRTY_OBJTYPE_PRIORITY, DDP[24].priority);
    MakeTeamDirty(DIRTY_OBJTYPE_PRIORITY, SEND_EVENTUALLY);

    return objtype_priority;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void TeamClass::SetFuelAvail(int fa)
{
    //MakeTeamDirty (DIRTY_SUPPLY_FUEL_AVAIL, DDP[25].priority);
    MakeTeamDirty(DIRTY_SUPPLY_FUEL_AVAIL, SEND_EVENTUALLY);

    if (fa < 0)
    {
        fa = 0;
    }

    fuelAvail = fa;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void TeamClass::SetSupplyAvail(int sa)
{
    //MakeTeamDirty (DIRTY_SUPPLY_FUEL_AVAIL, DDP[26].priority);
    MakeTeamDirty(DIRTY_SUPPLY_FUEL_AVAIL, SEND_EVENTUALLY);

    if (sa < 0)
    {
        sa = 0;
    }

    supplyAvail = sa;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void TeamClass::SetReplacementsAvail(int r)
{
    //MakeTeamDirty (DIRTY_SUPPLY_FUEL_AVAIL, DDP[27].priority);
    MakeTeamDirty(DIRTY_SUPPLY_FUEL_AVAIL, SEND_EVENTUALLY);

    if (r < 0)
    {
        r = 0;
    }

    replacementsAvail = r;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void TeamClass::SetInitiative(short i)
{
    //MakeTeamDirty (DIRTY_TEAM_INITIATIVE, DDP[28].priority);
    MakeTeamDirty(DIRTY_TEAM_INITIATIVE, SEND_EVENTUALLY);
    initiative = i;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void TeamClass::SetReinforcement(short r)
{
    //MakeTeamDirty (DIRTY_TEAM_REINFORCEMENT, DDP[29].priority);
    MakeTeamDirty(DIRTY_TEAM_REINFORCEMENT, SEND_EVENTUALLY);
    reinforcement = r;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TeamStatusType *TeamClass::SetCurrentStats(void)
{
    //MakeTeamDirty (DIRTY_CURRENT_STATS, DDP[30].priority);
    MakeTeamDirty(DIRTY_CURRENT_STATS, SEND_EVENTUALLY);
    return &currentStats;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TeamGndActionType *TeamClass::SetGroundAction(void)
{
    //MakeTeamDirty (DIRTY_GROUND_ACTION, DDP[31].priority);
    MakeTeamDirty(DIRTY_GROUND_ACTION, SEND_EVENTUALLY);
    return &groundAction;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void TeamClass::MakeTeamDirty(Dirty_Team bits, Dirtyness score)
{
    if (( not IsLocal()) or (VuState() not_eq VU_MEM_ACTIVE))
    {
        return;
    }

    dirty_team or_eq bits;
    MakeDirty(DIRTY_TEAM, score);
}

void TeamClass::WriteDirty(unsigned char **stream)
{
    unsigned char *ptr;

    ptr = *stream;
    // Encode it up
    *(ushort*)ptr = (ushort) dirty_team;
    ptr += sizeof(ushort);

    if (dirty_team bitand DIRTY_MISSION_PRIORITY)
    {
        memcpy(ptr, mission_priority, sizeof(mission_priority));
        ptr += sizeof(mission_priority);
    }

    if (dirty_team bitand DIRTY_UNITTYPE_PRIORITY)
    {
        memcpy(ptr, unittype_priority, sizeof(unittype_priority));
        ptr += sizeof(unittype_priority);
    }

    if (dirty_team bitand DIRTY_OBJTYPE_PRIORITY)
    {
        memcpy(ptr, objtype_priority, sizeof(objtype_priority));
        ptr += sizeof(objtype_priority);
    }

    if (dirty_team bitand DIRTY_SUPPLY_FUEL_AVAIL)
    {
        *(ushort*)ptr = supplyAvail;
        ptr += sizeof(ushort);

        *(ushort*)ptr = fuelAvail;
        ptr += sizeof(ushort);

        *(ushort*)ptr = replacementsAvail;
        ptr += sizeof(ushort);
    }

    if (dirty_team bitand DIRTY_TEAM_INITIATIVE)
    {
        *(short*)ptr = initiative;
        ptr += sizeof(short);
    }

    if (dirty_team bitand DIRTY_TEAM_REINFORCEMENT)
    {
        *(short*)ptr = reinforcement;
        ptr += sizeof(short);
    }

    if (dirty_team bitand DIRTY_CURRENT_STATS)
    {
        memcpy(ptr, &currentStats, sizeof(currentStats));
        ptr += sizeof(currentStats);
    }

    if (dirty_team bitand DIRTY_GROUND_ACTION)
    {
        memcpy(ptr, &groundAction, sizeof(groundAction));
        ptr += sizeof(groundAction);
    }

    if (dirty_team bitand DIRTY_OFFAIR_ACTION)
    {
        memcpy(ptr, &offensiveAirAction, sizeof(offensiveAirAction));
        ptr += sizeof(offensiveAirAction);
    }

    if (dirty_team bitand DIRTY_DEFAIR_ACTION)
    {
        memcpy(ptr, &defensiveAirAction, sizeof(defensiveAirAction));
        ptr += sizeof(defensiveAirAction);
    }

    if (dirty_team bitand DIRTY_TEAM_RELATIONS)
    {
        ShiAssert((cteam bitand 0xff) not_eq 0xfc);
        memcpy(ptr, &cteam, sizeof(uchar));
        ptr += sizeof(uchar);
        memcpy(ptr, member, sizeof(uchar) * NUM_COUNS);
        ptr += sizeof(uchar) * NUM_COUNS;
        memcpy(ptr, stance, sizeof(short) * NUM_TEAMS);
        ptr += sizeof(short) * NUM_TEAMS;
    }

    dirty_team = 0;
    *stream = ptr;
}

void TeamClass::ReadDirty(unsigned char **stream, long *rem)
{

    ushort bits;

    // Encode it up
    memcpychk(&bits, stream, sizeof(ushort), rem);

    if (bits bitand DIRTY_MISSION_PRIORITY)
    {
        memcpychk(mission_priority, stream, sizeof(mission_priority), rem);
    }

    if (bits bitand DIRTY_UNITTYPE_PRIORITY)
    {
        memcpychk(unittype_priority, stream, sizeof(unittype_priority), rem);
    }

    if (bits bitand DIRTY_OBJTYPE_PRIORITY)
    {
        memcpychk(objtype_priority, stream, sizeof(objtype_priority), rem);
    }

    if (bits bitand DIRTY_SUPPLY_FUEL_AVAIL)
    {
        memcpychk(&supplyAvail, stream, sizeof(ushort), rem);
        memcpychk(&fuelAvail, stream, sizeof(ushort), rem);
        memcpychk(&replacementsAvail, stream, sizeof(ushort), rem);
    }

    if (bits bitand DIRTY_TEAM_INITIATIVE)
    {
        memcpychk(&initiative, stream, sizeof(short), rem);
    }

    if (bits bitand DIRTY_TEAM_REINFORCEMENT)
    {
        memcpychk(&reinforcement, stream, sizeof(short), rem);
    }

    if (bits bitand DIRTY_CURRENT_STATS)
    {
        memcpychk(&currentStats, stream, sizeof(currentStats), rem);
    }

    if (bits bitand DIRTY_GROUND_ACTION)
    {
        memcpychk(&groundAction, stream, sizeof(groundAction), rem);
    }

    if (bits bitand DIRTY_OFFAIR_ACTION)
    {
        memcpychk(&offensiveAirAction, stream, sizeof(offensiveAirAction), rem);
    }

    if (bits bitand DIRTY_DEFAIR_ACTION)
    {
        memcpychk(&defensiveAirAction, stream, sizeof(defensiveAirAction), rem);
    }

    if (bits bitand DIRTY_TEAM_RELATIONS)
    {
        memcpychk(&cteam, stream, sizeof(uchar), rem);

        if ( not ((cteam > 0) and (cteam < NUM_TEAMS)))
        {
            char err[200];
            sprintf(err, "%s %d: error reading dirty, invalid cteam", __FILE__, __LINE__);
            throw InvalidBufferException(err);
        }

        memcpychk(member, stream, sizeof(uchar) * NUM_COUNS, rem);
        memcpychk(stance, stream, sizeof(short) * NUM_TEAMS, rem);
        TheCampaign.MakeCampMap(MAP_OWNERSHIP);
        PostMessage(FalconDisplay.appWin, FM_REFRESH_CAMPMAP, 0, 0);
    }
}

// This converts a team number to the appropriate string
int ConvertTeamToStringIndex(int team, int gender, int usage, int plural)
{
    int stridx = 3820 + 20;

    if (TeamInfo[team])
        stridx = 3820 + 20 * TeamInfo[team]->GetFlag() + 6 * usage + 3 * plural + gender;

    return stridx;
}

#ifdef DEBUG
int gVehsAddedWithCheat = 0;
int gACAddedWithCheat = 0;
int gSupplyAddedWithCheat = 0;
int gFuelAddedWithCheat = 0;
#endif

void StartOffensive(int team, int bonus)
{
    // A.S. 2001-12-09. This makes the offensive bonuses configurable
#ifdef DEBUG
    if (TeamInfo[team]->GetSupplyAvail() < StartOffBonusSup)
        gSupplyFromOffensive[team] += StartOffBonusSup - TeamInfo[team]->GetSupplyAvail();

    if (TeamInfo[team]->GetFuelAvail() < StartOffBonusFuel)
        gFuelFromOffensive[team] += StartOffBonusFuel - TeamInfo[team]->GetFuelAvail();

    if (TeamInfo[team]->GetReplacementsAvail() < StartOffBonusRepl)
        gReplacmentsFromOffensive[team] += StartOffBonusRepl - TeamInfo[team]->GetReplacementsAvail();

#endif

    if (TeamInfo[team]->GetSupplyAvail() < StartOffBonusSup)
        TeamInfo[team]->SetSupplyAvail(StartOffBonusSup);         //from 5000 to StartOffBonusSup

    if (TeamInfo[team]->GetFuelAvail() < StartOffBonusFuel)
        TeamInfo[team]->SetFuelAvail(StartOffBonusFuel);          //from 5000 to StartOffBonusFuel

    if (TeamInfo[team]->GetReplacementsAvail() < StartOffBonusRepl)
        TeamInfo[team]->SetReplacementsAvail(StartOffBonusRepl);   //from 1000 to StartOffBonusRep

    // end added section

    /* +++ old code +++
    #ifdef DEBUG
    if (TeamInfo[team]->GetSupplyAvail() < 5000)
    gSupplyFromOffensive[team] += 5000 - TeamInfo[team]->GetSupplyAvail();
    if (TeamInfo[team]->GetFuelAvail() < 5000)
    gFuelFromOffensive[team] += 5000 - TeamInfo[team]->GetFuelAvail();
    if (TeamInfo[team]->GetReplacementsAvail() < 1000)
    gReplacmentsFromOffensive[team] += 1000 - TeamInfo[team]->GetReplacementsAvail();
    #endif
    if (TeamInfo[team]->GetSupplyAvail() < 5000)
    TeamInfo[team]->SetSupplyAvail(5000);
    if (TeamInfo[team]->GetFuelAvail() < 5000)
    TeamInfo[team]->SetFuelAvail(5000);
    if (TeamInfo[team]->GetReplacementsAvail() < 1000)
    TeamInfo[team]->SetReplacementsAvail(1000);
     */
}

