#ifndef CMPCLASS_H
#define CMPCLASS_H

// ===============================================
// Das Campaign Uber-Class
// ===============================================
#include "Campaign.h"
#include "SquadUI.h"
#include "CUIEvent.h"

// Campaign flags
enum
{
    CAMP_RUNNING = 0x0000001, // Set if Campaign is running
    CAMP_LOADED = 0x0000002, // Set if a valid campaign is in memory
    CAMP_SHUTDOWN_REQUEST = 0x0000004, // Flagged when we want to shutdown
    CAMP_SUSPEND_REQUEST = 0x0000008, // Flagged when we want to pause
    CAMP_SUSPENDED = 0x0000010,
    CAMP_THEATER_LOADED = 0x0000020,
    CAMP_SLAVE = 0x0000040, // Set if another machine is doing timing
    CAMP_LIGHT = 0x0000080, // Only build player bubble and handle VU messages
    CAMP_PRELOADED = 0x0000100, // Preload has already been done
    CAMP_ONLINE = 0x0000200, // This is a multi-player game
    CAMP_TACTICAL = 0x0000400, // This is a tactical Engagement "Campaign"

    CAMP_GAME_FULL = 0x0010000, // This game is full
    DF_MATCH_IN_PROGRESS = 0x0020000, // This is a dogfight match game and it is in progress

    CAMP_TACTICAL_PAUSE = 0x0040000, // This means a tacitcal engagement is being run - don't do any movement.
    CAMP_TACTICAL_EDIT = 0x0080000, // This means a tacitcal engagement is being edited - don't do any movement.

    CAMP_NEED_ENTITIES = 0x00100000,
    CAMP_NEED_WEATHER = 0x00200000,
    CAMP_NEED_PERSIST = 0x00400000,
    CAMP_NEED_OBJ_DELTAS = 0x00800000,
    CAMP_NEED_PRELOAD = 0x01000000,
    CAMP_NEED_TEAM_DATA = 0x02000000,
    CAMP_NEED_UNIT_DATA = 0x04000000,
    CAMP_NEED_VC = 0x08000000,
    CAMP_NEED_PRIORITIES     = 0x10000000,

    CAMP_NEED_MASK = 0x1FF00000,

    CAMP_NAME_SIZE = 40, // Size of name string arrays for scenario name and such

    // wParam values for FM_JOIN_CAMPAIGN messages:
    JOIN_NOT_JOINING = 0,
    JOIN_PRELOAD_ONLY = 1,
    JOIN_REQUEST_ALL_DATA = 2,
    JOIN_CAMP_DATA_ONLY = 3,
};

class SquadronClass;
typedef SquadronClass* Squadron;
class FlightClass;
typedef FlightClass* Flight;
//sfr: we need this because Cmpclass includes misseval, which includes cmpclass
class MissionEvaluationClass;
class FalconGameEntity;
enum FalconGameType;

// =====================
// Campaign Class
// =====================

// This stores all data we need to know how to start a particular scenario
class CampaignClass
{
private:
public:
    CampaignTime      CurrentTime;
    CampaignTime      TE_StartTime;
    CampaignTime      TE_TimeLimit;
    CampaignTime TimeOfDay; // Time since last midnight
    CampaignTime lastGroundTask;
    CampaignTime lastAirTask;
    CampaignTime lastNavalTask;
    CampaignTime lastGroundPlan;
    CampaignTime lastAirPlan;
    CampaignTime lastNavalPlan;
    CampaignTime lastResupply;
    CampaignTime lastRepair;
    CampaignTime lastReinforcement;
    CampaignTime lastStatistic;
    CampaignTime lastMajorEvent;
    CampaignTime last_victory_time;
    volatile ulong Flags;
    bool InMainUI; // MN for weather UI
    short TimeStamp;
    short Group; // Multiplayer Session ID
    short GroundRatio; // Our strength vs their strength (at start conditions)
    short AirRatio;
    short AirDefenseRatio;
    short NavalRatio;
    short Brief; // Index to theater briefing
    short Processor;
    short TheaterSizeX;
    short TheaterSizeY;
    uchar             CurrentDay;
    uchar ActiveTeams; // Number of participating teams
    uchar             DayZero;          // Marks start of war
    uchar EndgameResult; // Is campaign over and who won?
    uchar Situation; // How're things going?
    uchar EnemyAirExp; // KCK: These two can probably be removed
    uchar EnemyADExp;
    uchar BullseyeName; // Bullseye data for the theater
    GridIndex BullseyeX;
    GridIndex BullseyeY;
    char TheaterName[CAMP_NAME_SIZE]; // Theater by name
    char Scenario[CAMP_NAME_SIZE]; // Name of scenario (one of x origional scenarios)
    char SaveFile[CAMP_NAME_SIZE]; // Name of save file (saved scenario)
    char UIName[CAMP_NAME_SIZE]; // UI's description of the game
    VuThread* vuThread; // Pointer to vu's thread structure
    VU_ID PlayerSquadronID; // VU_ID of player squadron (from last load)

    CampUIEventElement* StandardEventQueue; // Queue of last few standard events
    CampUIEventElement* PriorityEventQueue; // Queue of last few priority events
    // JPO - upgraded the next 3 to long, to allow bigger campaigns.
    long CampMapSize; // Size of currently allocated map data
    long SamMapSize;  // Size of currently allocated map data
    long RadarMapSize; // Size of currently allocated map data
    uchar* CampMapData; // Data for tiny occupation map.
    uchar* SamMapData; // Data for other maps
    uchar* RadarMapData;
    short LastIndexNum; // Last index assigned to point into the name/patch data
    short NumAvailSquadrons; // Number of active selectable squadrons
    SquadUIInfoClass *CampaignSquadronData; // The data array
    short NumberOfValidTypes; // Number of different types of aircraft we can fly
    short *ValidAircraftTypes; // An array of dIndexs for squadron's we're allowed to join
    MissionEvaluationClass* MissionEvaluator; // Mission evaluation class (for player)
    VuBin<FalconGameEntity> CurrentGame;

    VU_ID HotSpot; // The most important primary currently
    uchar Tempo; // How fast/dense we want things to happen
    long CreatorIP; // IP Address when started
    long CreationTime; // Time when started
    long CreationRand; // Random Number to pretty much guarantee we are unique to the universe
    long TE_VictoryPoints; // TE Points required to win
    long TE_type; // Type of tacitcal engagement
    long TE_number_teams; // Number of teams
    long TE_number_aircraft[8]; // Number of Aircraft per team
    long TE_number_f16s[8]; // Number of F16s per team
    long TE_team; // Number of teams
    long TE_team_pts[8]; // Points per team
    long TE_flags; // Various TE Flags

    uchar team_flags[8]; // Flags
    uchar team_colour[8]; // Colour
    uchar team_name[8][20]; // Name
    uchar team_motto[8][200]; // Motto

public:
    BOOL campRunning;
    DWORD LoopStarter(void);
    CampaignClass(void);
    ~CampaignClass(void);
    void Reset(void);
    F4THREADHANDLE InitCampaign(FalconGameType gametype, FalconGameEntity *joingame); // Don't call directly.
    void SetCurrentTime(CampaignTime newTime)
    {
        CurrentTime = newTime;
    }
    void SetTEStartTime(CampaignTime newTime)
    {
        TE_StartTime = newTime;
    }
    void SetTETimeLimitTime(CampaignTime newTime)
    {
        TE_TimeLimit = newTime;
    }
    CampaignTime GetCampaignTime(void)
    {
        return CurrentTime;
    }
    CampaignTime GetTEStartTime(void)
    {
        return TE_StartTime;
    }
    CampaignTime GetTETimeLimitTime(void)
    {
        return TE_TimeLimit;
    }
    int GetActiveTeams(void)
    {
        return ActiveTeams;
    }
    int GetCampaignDay(void);
    int GetCurrentDay(void);
    int GetMinutesSinceMidnight(void);
    char* GetTheaterName(void)
    {
        return TheaterName;
    }
    int SetTheater(char* name);
    int SetScenario(char* scenario);
    char* GetScenario(void)
    {
        return Scenario;
    }
    char* GetSavedName(void)
    {
        return SaveFile;
    }
    void SetPlayerSquadronID(VU_ID id)
    {
        PlayerSquadronID = id;
    }
    VU_ID GetPlayerSquadronID(void)
    {
        return PlayerSquadronID;
    }
    void GetPlayerLocation(short *x, short *y);
    void GotJoinData(void);
    int IsRunning(void)
    {
        return Flags bitand CAMP_RUNNING;
    }
    int IsLoaded(void)
    {
        return Flags bitand CAMP_LOADED;
    }
    int IsPreLoaded(void)
    {
        return Flags bitand CAMP_PRELOADED;
    }
    int IsSuspended(void)
    {
        return Flags bitand CAMP_SUSPENDED;
    }
    int IsMaster(void);
    int IsOnline(void)
    {
        return Flags bitand CAMP_ONLINE;
    }

    void ProcessEvents(void);

    // Here's the UI Interface routines:
    int LoadScenarioStats(FalconGameType type, char *savefile);
    int RequestScenarioStats(FalconGameEntity *game);
    void ClearCurrentPreload(void);
    int NewCampaign(FalconGameType gametype, char *scenario); // Calls InitCampaign Internally
    int LoadCampaign(FalconGameType gametype, char *savefile); // Calls InitCampaign Internally
    int JoinCampaign(FalconGameType gametype, FalconGameEntity *game); // Calls InitCampaign Internally
    int StartRemoteCampaign(FalconGameEntity *game);
    int SaveCampaign(FalconGameType type, char *savefile, int save_scenario_data);
    // sfr: this ends the campaign on a thread safe place...
#define NEW_END_CAMPAIGN 1
#if NEW_END_CAMPAIGN
    /** really ends the campaign. */
    void ReallyEndCampaign();
    /** used to request campaign end. Returns only after it has ended. */
    void EndCampaign();
#else
    void EndCampaign(); // Ends Campaign Instance, halts thread.
#endif
    void Suspend(void);
    void Resume(void);
    void SetOnlineStatus(int online);

    // PJW These functions are for my player matching stuff
    // should ONLY get set when someone does a "NEW" campaign
    void SetCreatorIP(long ip)
    {
        CreatorIP = ip;
    }
    void SetCreationTime(long time)
    {
        CreationTime = time;
    }
    void SetCreationIter(long rand)
    {
        CreationRand = rand;
    }
    long GetCreatorIP()
    {
        return(CreatorIP);
    }
    long GetCreationTime()
    {
        return(CreationTime);
    }
    long GetCreationIter()
    {
        return(CreationRand);
    }

    void SetTEVictoryPoints(long points)
    {
        TE_VictoryPoints = points;
    }
    long GetTEVictoryPoints()
    {
        return(TE_VictoryPoints);
    }

    // Bullseye data
    void GetBullseyeLocation(GridIndex *x, GridIndex *y);
    void GetBullseyeSimLocation(float *x, float *y);
    uchar GetBullseyeName(void);
    void SetBullseye(uchar nameid, GridIndex x, GridIndex y);
    int BearingToBullseyeDeg(float x, float y);
    int RangeToBullseyeFt(float x, float y);

    // Some serialization routines;
    int LoadData(FILE *fp);
    int SaveData(FILE *fp);
    int Decode(VU_BYTE **stream, long *rem);
    int Encode(VU_BYTE **stream);
    long SaveSize(void);

    // The Campaign Event manipulation functions
    CampUIEventElement* GetRecentEventlist(void);
    CampUIEventElement* GetRecentPriorityEventList(void);
    void AddCampaignEvent(CampUIEventElement *newEvent);

    void DisposeEventLists(void);
    void TrimCampUILists(void);

    // Map Stuff (small map)
    uchar* MakeCampMap(int type);
    void FreeCampMaps(void);

    // Squadron UI data stuff
    void VerifySquadrons(int team); // Rebuilds any changable squadron data
    void FreeSquadronData(void);
    void ReadValidAircraftTypes(char *typefile); // Reads text file with valid squadron types
    int IsValidAircraftType(Unit u); // Checks if passed Squadron is valid
    int IsValidSquadron(int id);
    void ChillTypes(void);

};

// The one and only Campaign instance:
extern CampaignClass TheCampaign;

// Current data version
extern int gCampDataVersion;

// ======================
// Time adjustment class
// ======================

class TimeAdjustClass
{
public:
    CampaignTime currentTime;
    uchar currentDay;
    short compression;
};

extern TimeAdjustClass TimeAdjust;

#endif
