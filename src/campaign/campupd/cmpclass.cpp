#include <io.h>
#include <stdio.h>
#include <fcntl.h>
#include <process.h>
#include "Cmpclass.h"
#include "CmpGlobl.h"
#include "F4VU.h"
#include "F4Find.h"
#include "falcmesg.h"
#include "F4Thread.h"
#include "CampTerr.h"
#include "Entity.h"
#include "Campaign.h"
#include "Team.h"
#include "Find.h"
#include "CmpEvent.h"
#include "CUIEvent.h"
#include "Dialog.h"
#include "Weather.h"
#include "Name.h"
#include "MsgInc/RequestCampaignData.h"
#include "CampStr.h"
#include "MissEval.h"
#include "Pilot.h"
#include "CampMap.h"
#include "AIInput.h"
#include "Division.h"
#include "ThreadMgr.h"
#include "falcsess.h"
#include "Persist.h"
#include "RLE.h"
#include "PlayerOp.h"
#include "uicomms.h"
#include "Utils/Lzss.h"
#include "classtbl.h"
#include "ui95/Chandler.h"
#include "Tacan.h"
#include "navsystem.h"
#include "rules.h"
#include "logbook.h"
#include "UserIDs.h"
#include "NavUnit.h"
#include "Dispcfg.h"
#include "ui/include/tac_class.h"
#include "ui/include/te_defs.h"
#include "atcbrain.h"
#include "simdrive.h"
#include "Gtm.h"
#include "TimerThread.h"

//sfr: added for checks
#include "InvalidBufferException.h"

// Begin - Uplink stuff
#include "include/comsup.h"

// sfr: ok we go crazy now, fuck those mutexes
#define NO_LOCKS_ON_INIT_EXIT 0

// The one and only CampaignClass instance:
CampaignClass TheCampaign;


enum
{
    _MAX_IN_GROUP_ = 256,
};
TimeAdjustClass TimeAdjust;

//JAM 06Dec03 - Bumped version number for realWeather changes.
//int gCurrentDataVersion = 76; // Current version of campaign data files
// Cobra - Revert back to SP3 version to make saved cam/tac files cpmpatible with Tacedit
int gCurrentDataVersion = 73; // SP3 version of campaign data files
int gCampDataVersion = gCurrentDataVersion;
int gClearPilotInfo = 0;
int gTacticalFullEdit = 0;

void Camp_MakeInstantAction(void);
int ReadVersionNumber(char *saveFile);
void WriteVersionNumber(char *saveFile);
void NukeHistoryFiles(void);
int SaveAfterRename(char *savefile, FalconGameType gametype);

// Some externals
//
extern unsigned int __stdcall CampaignThread(void);
extern void InitIALists(void);
extern void DisposeIALists(void);
//extern void DisposeRunwayList (void);
extern void ChooseBullseye(void);
extern void SetCampaignStartupMode(void);
extern int tactical_is_training(void);

extern C_Handler *gMainHandler;
extern short gLastId;

extern int PMRX;
extern int PMRY;
extern int MAXOI;

#ifdef CAMPTOOL
extern HWND hToolWnd;
extern void CampaignWindow(HINSTANCE hInstance, int nCmdShow);
// Renaming tool stuff
extern VU_ID_NUMBER RenameTable[65536];
extern int gRenameIds;
#endif CAMPTOOL

#ifdef CAMPTOOL
extern short CampIDRenameTable[MAX_CAMP_ENTITIES];
#endif

extern void InitBaseLists(void);
extern void InitCampaignLists(void);
extern void InitTheaterLists(void);
extern void DisposeBaseLists(void);
extern void DisposeCampaignLists(void);
extern void DisposeTheaterLists(void);

#define TIMEOUT_CYCLES 30 // Seconds to wait for requested info


// ==========================
// Old data
// ==========================

class OldSquadUIInfoClass
{
public:
    float x; // Sim coordinates of squadron
    float y;
    VU_ID id; // VU_ID (not valid til Campaign Loads)
    short dIndex; // Description Index
    short nameId; // The UI's id into name and patch data
    uchar specialty;
    uchar currentStrength; // # of current active aircraft
    uchar country;
    _TCHAR airbaseName[80]; // Name of airbase (string)
};

// ==========================
// Campaign Class
// ==========================

CampaignClass::CampaignClass(void)
{
    Flags = 0;
    Processor = 1;
    MissionEvaluator = NULL;
    vuThread = NULL;
    EndgameResult = 0;
    StandardEventQueue = NULL;
    PriorityEventQueue = NULL;
    TheaterSizeX = 0;
    TheaterSizeY = 0;
    CampMapSize = SamMapSize = RadarMapSize = 0;
    CampMapData = NULL;
    SamMapData = NULL;
    RadarMapData = NULL;
    LastIndexNum = 0;
    NumAvailSquadrons = 0;
    CampaignSquadronData = NULL;
    NumberOfValidTypes = 0;
    ValidAircraftTypes = NULL;
    BullseyeName = 0;
    BullseyeX = 0;
    BullseyeY = 0;
    CurrentGame.reset();
    last_victory_time = 0;

    CreatorIP = 0;
    CreationTime = 0;
    CreationRand = 0;

#ifdef USE_SH_POOLS
    // Initialize our Smart Heap pools
    ObjectiveClass::InitializeStorage();
    BattalionClass::InitializeStorage();
    BrigadeClass::InitializeStorage();
    FlightClass::InitializeStorage();
    SquadronClass::InitializeStorage();
    PackageClass::InitializeStorage();
    TaskForceClass::InitializeStorage();
    runwayQueueStruct::InitializeStorage();
    //LoadoutStruct::InitializeStorage();
#endif
    InitPersistantDatabase();
}

CampaignClass::~CampaignClass(void)
{
    DisposeEventLists();
    ChillTypes();
    FreeSquadronData();
    FreeCampMaps();

#ifdef USE_SH_POOLS
    // Initialize our Smart Heap pools
    ObjectiveClass::ReleaseStorage();
    BattalionClass::ReleaseStorage();
    BrigadeClass::ReleaseStorage();
    FlightClass::ReleaseStorage();
    SquadronClass::ReleaseStorage();
    PackageClass::ReleaseStorage();
    TaskForceClass::ReleaseStorage();
    runwayQueueStruct::ReleaseStorage();
    //LoadoutStruct::ReleaseStorage();
#endif
    CleanupPersistantDatabase();
}

// Reset sets variables to default values.
void CampaignClass::Reset(void)
{
    CurrentTime = 0;
    lastGroundTask = 0;
    lastAirTask = 0;
    lastNavalTask = 0;
    lastGroundPlan = 0;
    lastAirPlan = 0;
    lastNavalPlan = 0;
    lastResupply = 0;
    lastMajorEvent = 0;
    lastRepair = 0;
    lastReinforcement = 0;
    last_victory_time = 0;
    CurrentDay = 0;
    ActiveTeams = NUM_TEAMS;
    DayZero = 0;
    EndgameResult = 0;
    Brief = 0;
    // KCK NOTE: This data should have been cleaned up already, but it might be a good idea to check and clean
    // it up if it hasn't been.
    StandardEventQueue = NULL;
    PriorityEventQueue = NULL;
    CampMapSize = SamMapSize = RadarMapSize = 0;
    TheaterSizeX = 0;
    TheaterSizeY = 0;
    CampMapData = NULL;
    SamMapData = NULL;
    RadarMapData = NULL;
    LastIndexNum = -1;
    NumAvailSquadrons = 0;
    BullseyeName = 0;
    BullseyeX = 0;
    BullseyeY = 0;
    CampaignSquadronData = NULL;
    NumberOfValidTypes = CAMP_FLY_ANY_AIRCRAFT;
    ValidAircraftTypes = NULL;
    sprintf(TheaterName, "default");
    sprintf(Scenario, "default");
    sprintf(SaveFile, "default");
    sprintf(UIName, "Red Herring");

    if (MissionEvaluator)
        delete MissionEvaluator;

    MissionEvaluator = NULL;
    HotSpot = FalconNullId;
    Tempo = 100;
}

//
// Init Campaign
//
// This is the guts of setting up a campaign, but should be called only internally
// There are 3 main paths to starting a campaign:
// 1) Call NewCampaign This initializes an empty world
// 2) Call LoadCampaign This loads a presaved world
// 3) Call JoinCampaign This gets campaign data remotely
//
// EndCampaign must be called afterwards to clean up.
//
F4THREADHANDLE CampaignClass::InitCampaign(FalconGameType gametype, FalconGameEntity *joingame)
{
    FalconGameEntity *newgame;
    _TCHAR *gamename;

    if (IsLoaded())
    {
        EndCampaign();
    }

    ResetNamespaces();
    NukeHistoryFiles();
    SetCampaignStartupMode();

    // Init the mission evaluator
    if (MissionEvaluator) delete MissionEvaluator; // JPO clear out old stuff.

    MissionEvaluator = new MissionEvaluationClass();
    MissionEvaluator->PreDogfightEval();

    if (joingame and joingame->GetGameType() == gametype)
    {
        newgame = joingame;
    }
    else
    {
        // Build the default game name
        gamename = LogBook.Callsign();

        // Setup game values
        if (gMainHandler)
        {
            C_Window *win = gMainHandler->FindWindow(INFO_WIN);

            if (win)
            {
                C_EditBox *ebox = (C_EditBox*)win->FindControl(INFO_GAMENAME);

                if (ebox)
                {
                    gamename = ebox->GetText();
                }
            }
        }

        // Create a new FalconGameEntity
        newgame = new FalconGameEntity(FalconLocalSession->Domain(), gamename);
        newgame->gameType = gametype;

        if (gCommsMgr and gCommsMgr->Online())
        {
            newgame->UpdateRules(gRules[RuleMode].GetRules());
        }

        newgame->SetMaxSessions(_MAX_IN_GROUP_);
    }

    //if (CurrentGame){
    // VuDeReferenceEntity(CurrentGame);
    //}
    //VuReferenceEntity(newgame);
    CurrentGame.reset(newgame);

    ShiAssert(gCommsMgr);

    // Join the game
    gCommsMgr->LookAtGame(newgame);
    gMainThread->JoinGame(newgame);

    // Now init the other needed modules
    ((WeatherClass*)realWeather)->Init((gametype == game_InstantAction or gametype == game_Dogfight));

    if ( not LoadTheater(TheaterName))
    {
        // JB 010731 return on fail
        return 0;
    }

    Flags or_eq CAMP_THEATER_LOADED;


    InitCampaignLists();
    TheaterSizeX = Map_Max_X;
    TheaterSizeY = Map_Max_Y;
    gLastId = 32767;
    InitTheaterLists();

    if (gTacanList)
    {
        delete gTacanList;
    }

    if (gNavigationSys)
    {
        delete gNavigationSys;
    }

    gTacanList = new TacanList;
    gNavigationSys = new NavigationSystem;

    SetTimeCompression(0);

#ifdef CAMPTOOL
    // KCK: Windows is so fucked up... I don't even want to start explaining
    // why I have to do this..  sigh.
    CampLeaveCriticalSection();

    if ( not (Flags bitand CAMP_LIGHT))
    {
        CampaignWindow(hInst, SW_SHOW);
    }

    CampEnterCriticalSection();
#endif
    return 1;
}

DWORD CampaignClass::LoopStarter(void)
{
    DWORD retval = NULL;

    campRunning = TRUE;

    ThreadManager::start_campaign_thread(CampaignThread);

    return (retval);
}

int CampaignClass::NewCampaign(FalconGameType gametype, char *savefile)
{
    if (IsLoaded())
    {
        EndCampaign();
    }

    CampEnterCriticalSection();
    Reset();
    InitCampaign(gametype, NULL);

    // Create necessary data entities
    AddNewTeams(Neutral);
    NewPilotInfo();
    strcpy(SaveFile, savefile);
    Flags or_eq CAMP_LOADED;
    CampLeaveCriticalSection();
    TheCampaign.Resume();
    return 1;
}

int CampaignClass::LoadCampaign(FalconGameType gametype, char *savefile)
{
    char from[MAX_PATH], to[MAX_PATH];

#ifdef CAMPTOOL

    if (gRenameIds)
        memset(RenameTable, 0, sizeof(VU_ID_NUMBER) * 65536);

    if (CampIDRenameTable[0])
    {
        for (int i = 0; i < MAX_CAMP_ENTITIES; i++)
            CampIDRenameTable[i] = 0;
    }

#endif

    if (IsLoaded())
    {
        EndCampaign();
    }

    if ( not IsPreLoaded() and not LoadScenarioStats(gametype, savefile))
    {
        EndCampaign();
        return 0;
    }

    InMainUI = false; // MN for weather UI

    TheCampaign.Suspend();

    //ShiAssert (gameCompressionRatio == 0);

    StartReadCampFile(gametype, savefile);

    gCampDataVersion = ReadVersionNumber(savefile);

#if not NO_LOCKS_ON_INIT_EXIT
    CampEnterCriticalSection();
#endif

    switch (gametype)
    {
        case game_InstantAction:
        case game_Dogfight:
        {
            Flags or_eq CAMP_LIGHT;
            DisposeEventLists();
            break;
        }

        case game_TacticalEngagement:
        {
            Flags or_eq CAMP_TACTICAL;
            DisposeEventLists();
            break;
        }
    }

    InitCampaign(gametype, NULL);

    // Load Savefile Data
    if ( not LoadTeams(savefile))
    {
        AddNewTeams(Neutral);
    }

    LoadBaseObjectives(Scenario);
    LoadObjectiveDeltas(savefile);

    if (gClearPilotInfo or not LoadPilotInfo(savefile))
    {
        NewPilotInfo();
    }

    LoadUnits(savefile);

    if ( not LoadCampaignEvents(savefile, Scenario))
    {
        NewCampaignEvents(Scenario);
    }

    LoadPersistantList(savefile);
    ChooseBullseye();

#ifdef CAMPTOOL

    if (gRenameIds)
    {
        EndReadCampFile();
        // The new scenario file should be set to our current save file
        sprintf(SaveFile, savefile);
        sprintf(Scenario, savefile);
        SaveAfterRename(savefile, gametype);
        CampLeaveCriticalSection();
        EndCampaign();
        return 0;
    }

    if (CampIDRenameTable[0])
    {
        CampEntity  ent;
        int         i = 0, id;

        while (CampIDRenameTable[i])
        {
            ent = GetEntityByCampID(CampIDRenameTable[i]);

            if (ent)
            {
                id = FindUniqueID();
                //            ent->SetCampID(id);
                MonoPrint("ID %d renamed to %d.\n", CampIDRenameTable[i], ent->GetCampID());
            }

            i++;
        }
    }

#endif

    if (gametype not_eq game_InstantAction)
    {
        RebuildObjectiveLists();

        if (gCurrentDataVersion > 67)
        {
            LoadPrimaryObjectiveList(savefile);
        }
    }

    if (gametype not_eq game_InstantAction)
    {
        BuildDivisionData();
    }

    if ( not (Flags bitand CAMP_LIGHT) and not (Flags bitand CAMP_TACTICAL))
    {
        // KCK: By telling weathermap that we're instant action, it won't
        // cause a reloading of weather for multiple instant action runs.
        ((WeatherClass*)realWeather)->CampLoad(savefile, 0);
        StandardRebuild();
        lastAirPlan = 0; // Force an air replan - To get squadron data into the ATM
        ChooseBullseye();
    }
    else
    {
        ((WeatherClass*)realWeather)->CampLoad(savefile, gametype);
    }

    // ChillTypes();
    Flags or_eq CAMP_LOADED;

    // Insert our game into the database - which will broadcast it if we're online
    VuGameEntity *game = gCommsMgr->GetTargetGame();
    vuDatabase->/*Quick*/Insert(game);
    EndReadCampFile();

    // Copy force ratio and history files into working file
    sprintf(from, "%s\\%s.his", FalconCampUserSaveDirectory, savefile);
    sprintf(to, "%s\\tmp.his", FalconCampUserSaveDirectory);
    CopyFile(from, to, FALSE);
    sprintf(from, "%s\\%s.frc", FalconCampUserSaveDirectory, savefile);
    sprintf(to, "%s\\tmp.frc", FalconCampUserSaveDirectory);
    CopyFile(from, to, FALSE);

    // KCK: Added code for tactical engagement missions which were saved with no gun..
    // This should go away once they've been converted..
    if (tactical_is_training())
    {
        VuListIterator myit(AllAirList);
        Unit u;
        u = (Unit) myit.GetFirst();

        while (u)
        {
            if (u->IsFlight())
            {
                LoadoutStruct *load = ((Flight)u)->GetLoadout();

                if ( not load)
                {
                    ((Flight)u)->LoadWeapons(NULL, DefaultDamageMods, Air, 2, WEAP_GUN, 0);
                }
                else
                {
                    if (load[0].WeaponID[0] and not load[0].WeaponCount[0])
                        load[0].WeaponCount[0] = 50;
                }
            }

            u = (Unit) myit.GetNext();
        }
    }

    if (gTacticalFullEdit)
    {
        // Copy in new country names
        for (int t = 0; t < NUM_TEAMS; t++)
        {
            if (TeamInfo[t])
            {
                TeamInfo[t]->SetName(CountryNameStr[t]);
            }
        }
    }

    gCampDataVersion = gCurrentDataVersion;
    TheCampaign.Resume();
#if not NO_LOCKS_ON_INIT_EXIT
    CampLeaveCriticalSection();
#endif
    return 1;
}

// Join Campaign can be called initially to join a campaign game or additional times
// to re-request remaining data required.
int CampaignClass::JoinCampaign(FalconGameType gametype, FalconGameEntity *game)
{
    ulong need_from_master, need_from_all;
    FalconSessionEntity *masterSession;

    if (game not_eq CurrentGame)
    {
        EndCampaign(); // Shut down previous campaign request
    }

    if (IsLoaded())
    {
        return 1; // Already loaded, return success
    }

    if ( not IsPreLoaded())
    {
        EndCampaign();
        return 0;
    }

    F4Assert(gametype not_eq game_InstantAction);

    masterSession = (FalconSessionEntity*) vuDatabase->Find(game->OwnerId());

    if ( not masterSession or masterSession == vuLocalSessionEntity)
        return LoadCampaign(gametype, Scenario);

    if (stricmp(Scenario, "Instant") == 0)
        Flags or_eq CAMP_LIGHT;

    Suspend();

#if not NO_LOCKS_ON_INIT_EXIT
    CampEnterCriticalSection();
#endif

    if (Flags bitand CAMP_NEED_MASK)
    {
        return 0;
        // We're resuming a previous request.
        ShiAssert(0); // KCK: I don't want to do this anymore. These are sent reliably
        need_from_master = Flags bitand (CAMP_NEED_MASK bitand compl CAMP_NEED_ENTITIES);
        need_from_all = Flags bitand (CAMP_NEED_MASK bitand CAMP_NEED_ENTITIES);

        // resend our master session information requests here.
        if (need_from_master)
        {
            FalconRequestCampaignData *camprequest;
            camprequest = new FalconRequestCampaignData(masterSession->Id(), masterSession);
            camprequest->dataBlock.who = FalconLocalSessionId;
            camprequest->dataBlock.dataNeeded = need_from_master;

            if (need_from_master bitand CAMP_NEED_OBJ_DELTAS)
                camprequest->dataBlock.size += FS_MAXBLK / 8;

            if (need_from_master bitand CAMP_NEED_UNIT_DATA)
                camprequest->dataBlock.size += FS_MAXBLK / 8;

            if (camprequest->dataBlock.size > 0)
            {
                uchar *tmpptr;
                camprequest->dataBlock.data = tmpptr = new uchar[camprequest->dataBlock.size];

                if (need_from_master bitand CAMP_NEED_OBJ_DELTAS)
                {
                    memcpy(tmpptr, masterSession->objDataReceived, FS_MAXBLK / 8);
                    tmpptr += FS_MAXBLK / 8;
                }

                if (need_from_master bitand CAMP_NEED_UNIT_DATA)
                    memcpy(tmpptr, masterSession->unitDataReceived, FS_MAXBLK / 8);
            }

            FalconSendMessage(camprequest, TRUE);
        }
    }
    else
    {
        // This is a new request
        // Start up the campaign loop
        if ( not InitCampaign(gametype, game)) // JB 010731 return on fail
        {
            CampLeaveCriticalSection();
            return 0;
        }

        // Load initial objective data..
        StartReadCampFile(gametype, Scenario);

        gCampDataVersion = ReadVersionNumber(Scenario);

        LoadBaseObjectives(TheCampaign.Scenario);

        if ( not LoadTeams(TheCampaign.Scenario))
            AddNewTeams(Neutral);

        if ( not LoadPilotInfo(TheCampaign.Scenario))
            NewPilotInfo();

        if ( not LoadCampaignEvents(Scenario, Scenario))
            NewCampaignEvents(Scenario);

        if ( not (Flags bitand CAMP_LIGHT))
            ((WeatherClass*)realWeather)->CampLoad(TheCampaign.Scenario, game_Campaign);

        // Rebuild objective lists once, so our received data has somewhere to go
        // (especially the priority data)
        RebuildObjectiveLists();
        EndReadCampFile();

        gCampDataVersion = gCurrentDataVersion;

        // Clear previous requests
        Flags and_eq compl CAMP_NEED_MASK;

        if (Flags bitand CAMP_LIGHT)
        {
            need_from_master = CAMP_NEED_PERSIST bitor CAMP_NEED_OBJ_DELTAS bitor CAMP_NEED_UNIT_DATA;
            need_from_all = 0;
            // need_from_all = CAMP_NEED_ENTITIES;
            Flags or_eq need_from_master bitor need_from_all;
        }
        else
        {
            need_from_master = CAMP_NEED_WEATHER bitor CAMP_NEED_PERSIST bitor CAMP_NEED_PRIORITIES bitor CAMP_NEED_OBJ_DELTAS bitor CAMP_NEED_TEAM_DATA bitor CAMP_NEED_UNIT_DATA bitor CAMP_NEED_VC;
            need_from_all = 0;
            // need_from_all = CAMP_NEED_ENTITIES;
            Flags or_eq need_from_master bitor need_from_all;

            if ( not LoadPilotInfo(TheCampaign.Scenario))
                NewPilotInfo();
        }

        Flags or_eq CAMP_SLAVE;
        Flags or_eq CAMP_ONLINE;

        // Send our master session information requests here.
        FalconRequestCampaignData *camprequest;
        camprequest = new FalconRequestCampaignData(masterSession->Id(), masterSession);
        camprequest->dataBlock.who = FalconLocalSessionId;
        camprequest->dataBlock.dataNeeded = need_from_master;
        FalconSendMessage(camprequest, TRUE);

        /* // KCK: I don't think we need this anymore
        // Now send our entity request to everyone
        camprequest = new FalconRequestCampaignData(masterSession->Id(), FalconLocalGame);
        camprequest->dataBlock.who = FalconLocalSessionId;
        camprequest->dataBlock.dataNeeded = need_from_all;
        FalconSendMessage (camprequest,TRUE);
         */
    }


    // Assume we'll get all our entities in time - ie: don't block on this request
    Flags and_eq compl CAMP_NEED_ENTITIES;

    MonoPrint("Done requesting shit... \n");

#if not NO_LOCKS_ON_INIT_EXIT
    CampLeaveCriticalSection();
#endif

    return 1;
}

int CampaignClass::StartRemoteCampaign(FalconGameEntity *game)
{
    if ( not IsLoaded() or (Flags bitand CAMP_NEED_MASK))
        return 0;

    if ( not (Flags bitand CAMP_LIGHT))
    {
        lastAirPlan = 0; // Force an air replan - To get squadron data into the ATM
        RebuildObjectiveLists();
        BuildDivisionData();
        StandardRebuild();
    }

    // We're done.. Start 'er up
    Resume();
    SetTimeCompression(1);

    return 1;
}

// This gets called every time we receive startup information.
// We check if we have everything and set ourselves up if we do.
void CampaignClass::GotJoinData(void)
{
    ulong still_needed = Flags bitand CAMP_NEED_MASK;

    MonoPrint("Got Join data Still needed = %x\n", still_needed);

    if (still_needed or IsLoaded())
        return;

    // We're loaded
    Flags or_eq CAMP_LOADED;

    gMainThread->JoinGame(gCommsMgr->GetTargetGame());

    // Notify UI of our success
    if (gMainHandler)
        PostMessage(FalconDisplay.appWin, FM_JOIN_SUCCEEDED, not FalconLocalGame->IsLocal(), 0);
}

#define CAMP_SAVE_NORMAL 0
#define CAMP_SAVE_FULL 1
#define CAMP_SAVE_LIGHT 2

int CampaignClass::SaveCampaign(FalconGameType gametype, char *savefile, int save_mode)
{
    FILE* fp;
    char to[MAX_PATH], from[MAX_PATH];

    if ( not IsLoaded() or (Flags bitand CAMP_LIGHT and save_mode not_eq CAMP_SAVE_LIGHT))
        return 0;

    StartWriteCampFile(gametype, savefile);

    CampEnterCriticalSection();

    if (gTacticalFullEdit)
        save_mode = CAMP_SAVE_FULL;

    if (gametype == game_TacticalEngagement)
        current_tactical_mission->save_data(savefile);

    sprintf(SaveFile, savefile);

    if (save_mode == CAMP_SAVE_LIGHT)
    {
        sprintf(Scenario, "Instant");
        Camp_MakeInstantAction();
    }
    else
    {
        if ( not CampMapSize or not TheaterSizeX or not CampMapData or save_mode == CAMP_SAVE_FULL)
            MakeCampMap(MAP_OWNERSHIP);

        VerifySquadrons(FALCON_PLAYER_TEAM);
    }

    if (save_mode == CAMP_SAVE_FULL)
    {
        // The new scenario file should be set to our current save file
        sprintf(Scenario, savefile);
    }

    fp = OpenCampFile(savefile, "cmp", "wb");

    if (fp)
    {
        SaveData(fp);
        CloseCampFile(fp);

        switch (save_mode)
        {
            case CAMP_SAVE_LIGHT:
                // KCK: These won't save right - no lists
                // SaveBaseObjectives(savefile);
                // SaveObjectiveDeltas(savefile);
                break;

            case CAMP_SAVE_FULL:
                SaveBaseObjectives(savefile);

                // SaveObjectiveDeltas(savefile);
                // SaveBaseUnits(SaveFile);
                // KCK: Fall through to below
            case CAMP_SAVE_NORMAL:
            default:
                SaveObjectiveDeltas(savefile);
                SaveUnits(savefile);
                // SaveUnitDeltas(savefile);
                SaveTeams(savefile);
                SaveCampaignEvents(savefile);
                SavePilotInfo(savefile);
                SavePersistantList(savefile);
                ((WeatherClass*)realWeather)->Save(savefile);
                SavePrimaryObjectiveList(savefile);
                break;
        }
    }

    WriteVersionNumber(savefile);

    EndWriteCampFile();

    // Copy force ratio and history files into save file
    sprintf(to, "%s\\%s.his", FalconCampUserSaveDirectory, savefile);
    sprintf(from, "%s\\tmp.his", FalconCampUserSaveDirectory);
    CopyFile(from, to, FALSE);
    sprintf(to, "%s\\%s.frc", FalconCampUserSaveDirectory, savefile);
    sprintf(from, "%s\\tmp.frc", FalconCampUserSaveDirectory);
    CopyFile(from, to, FALSE);

    CampLeaveCriticalSection();
    return 1;
}

#if NEW_END_CAMPAIGN
// sfr: just request campaign end
void CampaignClass::EndCampaign(void)
{
    // returns only when campaign is not loaded anymore
    if ( not TheCampaign.IsLoaded())
    {
        return;
    }

    TheCampaign.Flags or_eq CAMP_SHUTDOWN_REQUEST;

    while (TheCampaign.IsLoaded())
    {
        Sleep(100);
    }
}

// sfr: this really ends the campaign
void CampaignClass::ReallyEndCampaign()
#else
void CampaignClass::EndCampaign()
#endif
{
    MonoPrint("Calling EndCampaign.. \n");

#if not NEW_END_CAMPAIGN
    Suspend();
#endif
    SetTimeCompression(0);

#ifdef CAMPTOOL

    if (hToolWnd)
    {
        PostMessage(hToolWnd, WM_CLOSE, 0, 0);
    }

#endif

    // Now clean up.
    // KCK: I loath to do this, but VU just isn't threadsafe during shutdown
#if not NO_LOCKS_ON_INIT_EXIT
    CampEnterCriticalSection();
    VuEnterCriticalSection();
#endif

    // Clear out our pointers
    FalconLocalSession->ClearCameras();
    FalconLocalSession->SetPlayerEntity(NULL);
    FalconLocalSession->SetPlayerFlight(NULL);
    FalconLocalSession->SetPlayerSquadron(NULL);
    FalconLocalSession->SetFlyState(FLYSTATE_IN_UI);

#define JOIN_AT_END 1
#if not JOIN_AT_END
    // Join the player pool
    // sfr: get the current joining new
    VuGameEntity *oldGame = CurrentGame.get();

    if (gCommsMgr)
    {
        gCommsMgr->LookAtGame(vuPlayerPoolGroup);
    }

    gMainThread->JoinGame(vuPlayerPoolGroup);

    if ((oldGame not_eq NULL) and (oldGame->SessionCount() == 0))
    {
        vuDatabase->Remove(oldGame);
    }

#endif

    //if (CurrentGame){
    // if ( not CurrentGame->SessionCount()){
    // vuDatabase->Remove(CurrentGame);
    // }
    // VuDeReferenceEntity(CurrentGame);
    // CurrentGame = NULL;
    //}

    if (Flags bitand CAMP_LOADED)
    {
        if ( not (Flags bitand CAMP_LIGHT))
        {
            FreeTheaterTerrain();
            // Only remove teams if we're playing a local game
            DisposeCampaignLists();
            DisposePilotInfo();
            delete MissionEvaluator;
            MissionEvaluator = NULL;
            DumpDivisionData();
            DisposeCampaignEvents();
        }

        if ( not (Flags bitand CAMP_ONLINE))
        {
            RemoveTeams(); // KCK NOTE: These could be 'silent removes' instead
        }

        CleanupPersistantList();
        DisposeEventLists();
        FreeCampMaps();
        FreeSquadronData();
        ChillTypes();
        FreeNames();
        DisposeTheaterLists();
        // DSP Remove when no longer used
        //DisposeRunwayList();
    }

    if (gNavigationSys)
    {
        delete gNavigationSys;
    }

    if (gTacanList)
    {
        delete gTacanList;
    }

    gNavigationSys = NULL;
    gTacanList = NULL;
    Flags = CAMP_RUNNING;
    InMainUI = true;

#if JOIN_AT_END
    // Join the player pool
    // sfr: get the current joining new
    VuGameEntity *oldGame = CurrentGame.get();

    if (gCommsMgr)
    {
        gCommsMgr->LookAtGame(vuPlayerPoolGroup);
    }

    gMainThread->JoinGame(vuPlayerPoolGroup);

    if ((oldGame not_eq NULL) and (oldGame->SessionCount() == 0))
    {
        vuDatabase->Remove(oldGame);
    }

#endif


#if not NO_LOCKS_ON_INIT_EXIT
    VuExitCriticalSection(); // KCK: I loath to do this, but VU just isn't threadsafe during shutdown
    CampLeaveCriticalSection();
#endif
}


int CampaignClass::SetTheater(char* name)
{
    strcpy(TheaterName, name);
    return 1;
}

int CampaignClass::SetScenario(char* scenario)
{
    strcpy(Scenario, scenario);
    return 1;
}

int CampaignClass::GetCampaignDay(void)
{
    return (int)(GetCurrentDay() - DayZero);
}

int CampaignClass::GetCurrentDay(void)
{
    CurrentDay = (int)(CurrentTime / CampaignDay);
    return CurrentDay;
}

int CampaignClass::GetMinutesSinceMidnight(void)
{
    return TimeOfDay / CampaignMinutes;
}

// Read data from a file
int CampaignClass::LoadData(FILE *fp)
{
    uchar *buffer, *bufhead;
    long size;

    fread(&size, sizeof(long), 1, fp);
    bufhead = buffer = new uchar[size];
    fread(buffer, size, 1, fp);
    Decode(&buffer, &size);
    delete bufhead;
    return 1;
}

// Save data to a file
int CampaignClass::SaveData(FILE *fp)
{
    long size;
    uchar *buffer;

    size = Encode(&buffer);

    fwrite(&size, sizeof(long), 1, fp);
    fwrite(buffer, size, 1, fp);
    delete buffer;
    return size;
}

long CampaignClass::SaveSize(void)
{
    CampUIEventElement *event;
    ulong size = 0;

    size += sizeof(CampaignTime);
    size += sizeof(CampaignTime);
    size += sizeof(CampaignTime);
    size += sizeof(long);
    size += sizeof(long);
    size += sizeof(long);
    size += sizeof(long) * 8;
    size += sizeof(long) * 8;
    size += sizeof(long);
    size += sizeof(long) * 8;
    size += sizeof(long);

    size += sizeof(uchar) * 8;
    size += sizeof(uchar) * 8;
    size += sizeof(uchar) * 20 * 8;
    size += sizeof(uchar) * 200 * 8;

    size += sizeof(CampaignTime);
    size += sizeof(CampaignTime);
    size += sizeof(CampaignTime);
    size += sizeof(CampaignTime);
    size += sizeof(short);
    size += sizeof(short);

    size += sizeof(short);
    size += sizeof(short);
    size += sizeof(short);
    size += sizeof(short);
    size += sizeof(short);

    size += sizeof(short);
    size += sizeof(short);
    size += sizeof(uchar);
    size += sizeof(uchar);
    size += sizeof(uchar);
    size += sizeof(uchar);
    size += sizeof(uchar);
    size += sizeof(uchar);
    size += sizeof(uchar);
    size += sizeof(uchar);
    size += sizeof(GridIndex);
    size += sizeof(GridIndex);
    size += sizeof(char) * CAMP_NAME_SIZE;
    size += sizeof(char) * CAMP_NAME_SIZE;
    size += sizeof(char) * CAMP_NAME_SIZE;
    size += sizeof(char) * CAMP_NAME_SIZE;

    size += sizeof(VU_ID);

    size += sizeof(short);
    event = StandardEventQueue;

    while (event)
    {
        size += sizeof(uieventnode);
        size += sizeof(short);
        size += sizeof(_TCHAR) * _tcslen(event->eventText);
        event = event->next;
    }

    size += sizeof(short);
    event = PriorityEventQueue;

    while (event)
    {
        size += sizeof(uieventnode);
        size += sizeof(short);
        size += sizeof(_TCHAR) * _tcslen(event->eventText);
        event = event->next;
    }

    size += sizeof(short);
    size += CampMapSize;
    size += sizeof(short);
    size += sizeof(short);
    size += sizeof(SquadUIInfoClass) * NumAvailSquadrons;

    size += sizeof(uchar);

    size += sizeof(long);
    size += sizeof(long);
    size += sizeof(long);

    return size;
}

// Read data from a stream
int CampaignClass::Decode(VU_BYTE **stream, long *rem)
{
    short entries = 0, size = 0;
    CampUIEventElement *event = NULL, *last = NULL;
    int i = 0, loop = 0;
    long datasize = 0;
    uchar *buffer = NULL, *bufhead = NULL;

    CampEnterCriticalSection();

    memcpychk(&datasize, stream, sizeof(long), rem);

    long newRem = datasize + 4096;//1024;
    buffer = new VU_BYTE[newRem];
    bufhead = buffer;
    //sfr: we should check the return value here...
    LZSS_Expand(*stream, rem[0], buffer, datasize);

    memcpychk(&CurrentTime, &buffer, sizeof(CampaignTime), &newRem);

    if (CurrentTime == 0)
    {
        CurrentTime = 1;
    }

    SetTime(CurrentTime);

    if (gCampDataVersion >= 48)
    {
        memcpychk(&TE_StartTime, &buffer, sizeof(CampaignTime), &newRem);
        memcpychk(&TE_TimeLimit, &buffer, sizeof(CampaignTime), &newRem);

        if (gCampDataVersion > 49)
        {
            memcpychk(&TE_VictoryPoints, &buffer, sizeof(long), &newRem);
        }
        else
        {
            TE_VictoryPoints = 0;
        }
    }
    else
    {
        TE_StartTime = CurrentTime;
        TE_TimeLimit = CurrentTime + 60 * 60 * 5 * VU_TICS_PER_SECOND;
        TE_VictoryPoints = 0;
    }

    if (gCampDataVersion >= 52)
    {
        memcpychk(&TE_type, &buffer, sizeof(long), &newRem);
        memcpychk(&TE_number_teams, &buffer, sizeof(long), &newRem);
        memcpychk(TE_number_aircraft, &buffer, sizeof(long) * 8, &newRem);
        memcpychk(TE_number_f16s, &buffer, sizeof(long) * 8, &newRem);
        memcpychk(&TE_team, &buffer, sizeof(long), &newRem);
        memcpychk(TE_team_pts, &buffer, sizeof(long) * 8, &newRem);
        memcpychk(&TE_flags, &buffer, sizeof(long), &newRem);

        for (loop = 0; loop < 8; loop ++)
        {
            memcpychk(&team_flags[loop], &buffer, 1, &newRem);
            memcpychk(&team_colour[loop], &buffer, 1, &newRem);
            memcpychk(&team_name[loop], &buffer, 20, &newRem);
            memcpychk(&team_motto[loop], &buffer, 200, &newRem);
        }
    }
    else
    {
        TE_type = 0;
        TE_number_teams = 0;
        memset(TE_number_aircraft, 0, 8 * sizeof(long));
        memset(TE_number_f16s, 0, 8 * sizeof(long));
        TE_team = 0;
        memset(TE_team_pts, 0, 8 * sizeof(long));
        TE_flags = 0;
    }

    // Don't bother with tasking times, we'll have to rebuild anyway...
    lastGroundTask = 0;
    lastAirTask = 0;
    lastNavalTask = 0;
    lastGroundPlan = 0;
    lastAirPlan = 0;
    lastNavalPlan = 0;
    lastStatistic = 0;

    if (gCampDataVersion >= 19)
    {
        memcpychk(&lastMajorEvent, &buffer, sizeof(CampaignTime), &newRem);
    }

    memcpychk(&lastResupply, &buffer, sizeof(CampaignTime), &newRem);
    memcpychk(&lastRepair, &buffer, sizeof(CampaignTime), &newRem);
    memcpychk(&lastReinforcement, &buffer, sizeof(CampaignTime), &newRem);
    memcpychk(&TimeStamp, &buffer, sizeof(short), &newRem);
    memcpychk(&Group, &buffer, sizeof(short), &newRem);
    // Dont read flags
    memcpychk(&GroundRatio, &buffer, sizeof(short), &newRem);
    memcpychk(&AirRatio, &buffer, sizeof(short), &newRem);
    memcpychk(&AirDefenseRatio, &buffer, sizeof(short), &newRem);
    memcpychk(&NavalRatio, &buffer, sizeof(short), &newRem);
    memcpychk(&Brief, &buffer, sizeof(short), &newRem);
    // Dont read Processor
    memcpychk(&TheaterSizeX, &buffer, sizeof(short), &newRem);
    memcpychk(&TheaterSizeY, &buffer, sizeof(short), &newRem);
    // Adjust some globals
    MRX = TheaterSizeX / MAP_RATIO;
    MRY = TheaterSizeY / MAP_RATIO;
    PMRX = TheaterSizeX / PAK_MAP_RATIO;
    PMRY = TheaterSizeY / PAK_MAP_RATIO;
    MAXOI = sizeof(uchar) * MRX * MRY / 2;
    // Continue reading...
    memcpychk(&CurrentDay, &buffer, sizeof(uchar), &newRem);
    memcpychk(&ActiveTeams, &buffer, sizeof(uchar), &newRem);
    memcpychk(&DayZero, &buffer, sizeof(uchar), &newRem);
    memcpychk(&EndgameResult, &buffer, sizeof(uchar), &newRem);
    memcpychk(&Situation, &buffer, sizeof(uchar), &newRem);
    memcpychk(&EnemyAirExp, &buffer, sizeof(uchar), &newRem);
    memcpychk(&EnemyADExp, &buffer, sizeof(uchar), &newRem);
    memcpychk(&BullseyeName, &buffer, sizeof(uchar), &newRem);
    memcpychk(&BullseyeX, &buffer, sizeof(GridIndex), &newRem);
    memcpychk(&BullseyeY, &buffer, sizeof(GridIndex), &newRem);
    memcpychk(TheaterName, &buffer, sizeof(char)*CAMP_NAME_SIZE, &newRem);
    memcpychk(Scenario, &buffer, sizeof(char)*CAMP_NAME_SIZE, &newRem);
    memcpychk(SaveFile, &buffer, sizeof(char)*CAMP_NAME_SIZE, &newRem);
    memcpychk(UIName, &buffer, sizeof(char)*CAMP_NAME_SIZE, &newRem);
    // Might as well get the other guy's squadron
    memcpychk(&PlayerSquadronID, &buffer, sizeof(VU_ID), &newRem);
    FalconLocalSession->SetPlayerSquadronID(PlayerSquadronID);
    // Load the recent event queues
    DisposeEventLists();
    memcpychk(&entries, &buffer, sizeof(short), &newRem);

    for (i = 0; i < entries; i++)
    {
        event = new CampUIEventElement();
        memcpychk(event, &buffer, sizeof(uieventnode), &newRem);
        memcpychk(&size, &buffer, sizeof(short), &newRem);
        event->eventText = new _TCHAR[size + 1];
        memcpychk(event->eventText, &buffer, sizeof(_TCHAR)*size, &newRem);
        event->eventText[size] = 0;
        event->next = NULL;

        if ( not StandardEventQueue)
        {
            StandardEventQueue = event;
            last = event;
        }
        else
        {
            last->next = event;
            last = event;
        }
    }

    memcpychk(&entries, &buffer, sizeof(short), &newRem);

    for (i = 0; i < entries; i++)
    {
        event = new CampUIEventElement();
        memcpychk(event, &buffer, sizeof(uieventnode), &newRem);
        memcpychk(&size, &buffer, sizeof(short), &newRem);
        event->eventText = new _TCHAR[size + 1];
        memcpychk(event->eventText, &buffer, sizeof(_TCHAR)*size, &newRem);
        event->eventText[size] = 0;
        event->next = NULL;

        if ( not PriorityEventQueue)
        {
            PriorityEventQueue = event;
            last = event;
        }
        else
        {
            if (last)
                last->next = event;

            last = event;
        }
    }

    // Read the map data
    FreeCampMaps();
    memcpychk(&CampMapSize, &buffer, sizeof(short), &newRem);

    if (CampMapSize > 0)
    {
        CampMapData = new uchar[CampMapSize];
        memcpychk(CampMapData, &buffer, CampMapSize, &newRem);
    }

    // Read the squadron data
    FreeSquadronData();
    memcpychk(&LastIndexNum, &buffer, sizeof(short), &newRem);
    memcpychk(&NumAvailSquadrons, &buffer, sizeof(short), &newRem);

    if (NumAvailSquadrons > 0)
    {
        if (gCampDataVersion < 42)
        {
            OldSquadUIInfoClass *osic = new OldSquadUIInfoClass[NumAvailSquadrons];
            memcpychk(osic, &buffer, sizeof(OldSquadUIInfoClass)*NumAvailSquadrons, &newRem);
            CampaignSquadronData = new SquadUIInfoClass[NumAvailSquadrons];

            for (i = 0; i < NumAvailSquadrons; i++)
            {
                CampaignSquadronData[i].x = osic[i].x;
                CampaignSquadronData[i].y = osic[i].y;
                CampaignSquadronData[i].id = osic[i].id;
                CampaignSquadronData[i].dIndex = osic[i].dIndex;
                CampaignSquadronData[i].nameId = osic[i].nameId;
                CampaignSquadronData[i].specialty = osic[i].specialty;
                CampaignSquadronData[i].currentStrength = osic[i].currentStrength;
                CampaignSquadronData[i].country = osic[i].country;
                _tcsnccpy(CampaignSquadronData[i].airbaseName, osic[i].airbaseName, 39);
                CampaignSquadronData[i].airbaseName[39] = 0;
            }

            delete[] osic;
        }
        else
        {
            CampaignSquadronData = new SquadUIInfoClass[NumAvailSquadrons];
            memcpychk(CampaignSquadronData, &buffer, sizeof(SquadUIInfoClass)*NumAvailSquadrons, &newRem);
        }
    }

    if (gCampDataVersion >= 31)
    {
        memcpychk(&Tempo, &buffer, sizeof(uchar), &newRem);
    }

    if (gCampDataVersion >= 43)
    {
        memcpychk(&CreatorIP, &buffer, sizeof(long), &newRem);
        memcpychk(&CreationTime, &buffer, sizeof(long), &newRem);
        memcpychk(&CreationRand, &buffer, sizeof(long), &newRem);
    }

    // Now we're preloaded
    Flags or_eq CAMP_PRELOADED;

    CampLeaveCriticalSection();

    delete bufhead;

    if (gCampDataVersion > 5)
    {
        ShiAssert((int)(buffer - bufhead) == datasize);
    }

    return (int)(buffer - bufhead);
}

// Write data into a stream
int CampaignClass::Encode(VU_BYTE **stream)
{
    short entries, size;
    CampUIEventElement *event;
    uchar *buffer, *bufhead, *sptr;
    int loop;
    long newsize, datasize;

    if ( not (Flags bitand CAMP_LIGHT))
    {
        // Get up to data squadron information
        VerifySquadrons(FALCON_PLAYER_TEAM);
    }

    datasize = SaveSize();

    buffer = new uchar[datasize + 1];
    bufhead = buffer;

    // Encode the data
    memcpy(buffer, &CurrentTime, sizeof(CampaignTime));
    buffer += sizeof(CampaignTime);
    memcpy(buffer, &TE_StartTime, sizeof(CampaignTime));
    buffer += sizeof(CampaignTime);
    memcpy(buffer, &TE_TimeLimit, sizeof(CampaignTime));
    buffer += sizeof(CampaignTime);
    memcpy(buffer, &TE_VictoryPoints, sizeof(long));
    buffer += sizeof(long);

    memcpy(buffer, &TE_type, sizeof(long));
    buffer += sizeof(long);
    memcpy(buffer, &TE_number_teams, sizeof(long));
    buffer += sizeof(long);
    memcpy(buffer, TE_number_aircraft, sizeof(long) * 8);
    buffer += sizeof(long) * 8;
    memcpy(buffer, TE_number_f16s, sizeof(long) * 8);
    buffer += sizeof(long) * 8;
    memcpy(buffer, &TE_team, sizeof(long));
    buffer += sizeof(long);
    memcpy(buffer, TE_team_pts, sizeof(long) * 8);
    buffer += sizeof(long) * 8;
    memcpy(buffer, &TE_flags, sizeof(long));
    buffer += sizeof(long);

    for (loop = 0; loop < 8; loop ++)
    {
        if (TeamInfo[loop])
        {
            memcpy(buffer, &TeamInfo[loop]->teamFlag, 1);
            buffer += 1;
            memcpy(buffer, &TeamInfo[loop]->teamColor, 1);
            buffer += 1;
            memcpy(buffer, &TeamInfo[loop]->name, 20);
            buffer += 20;
            memcpy(buffer, &TeamInfo[loop]->teamMotto, 200);
            buffer += 200;
        }
        else
        {
            memset(buffer, 0, 1 + 1 + 20 + 200);
            buffer += 1 + 1 + 20 + 200;
        }
    }

    // Don't bother with tasking times, they'll have to rebuild anyway...
    memcpy(buffer, &lastMajorEvent, sizeof(CampaignTime));
    buffer += sizeof(CampaignTime);
    memcpy(buffer, &lastResupply, sizeof(CampaignTime));
    buffer += sizeof(CampaignTime);
    memcpy(buffer, &lastRepair, sizeof(CampaignTime));
    buffer += sizeof(CampaignTime);
    memcpy(buffer, &lastReinforcement, sizeof(CampaignTime));
    buffer += sizeof(CampaignTime);
    memcpy(buffer, &TimeStamp, sizeof(short));
    buffer += sizeof(short);
    memcpy(buffer, &Group, sizeof(short));
    buffer += sizeof(short);
    // Dont read flags
    memcpy(buffer, &GroundRatio, sizeof(short));
    buffer += sizeof(short);
    memcpy(buffer, &AirRatio, sizeof(short));
    buffer += sizeof(short);
    memcpy(buffer, &AirDefenseRatio, sizeof(short));
    buffer += sizeof(short);
    memcpy(buffer, &NavalRatio, sizeof(short));
    buffer += sizeof(short);
    memcpy(buffer, &Brief, sizeof(short));
    buffer += sizeof(short);
    // Dont read Processor
    memcpy(buffer, &TheaterSizeX, sizeof(short));
    buffer += sizeof(short);
    memcpy(buffer, &TheaterSizeY, sizeof(short));
    buffer += sizeof(short);
    memcpy(buffer, &CurrentDay, sizeof(uchar));
    buffer += sizeof(uchar);
    memcpy(buffer, &ActiveTeams, sizeof(uchar));
    buffer += sizeof(uchar);
    memcpy(buffer, &DayZero, sizeof(uchar));
    buffer += sizeof(uchar);
    memcpy(buffer, &EndgameResult, sizeof(uchar));
    buffer += sizeof(uchar);

    if (FalconLocalSession->GetTeam() < NUM_TEAMS and TeamInfo[FalconLocalSession->GetTeam()])
    {
        Situation = TeamInfo[FalconLocalSession->GetTeam()]->Initiative() / 20;
    }
    else if (TeamInfo[2])
    {
        Situation = TeamInfo[2]->Initiative() / 20;
    }
    else
    {
        Situation = 0;
    }

    memcpy(buffer, &Situation, sizeof(uchar));
    buffer += sizeof(uchar);
    memcpy(buffer, &EnemyAirExp, sizeof(uchar));
    buffer += sizeof(uchar);
    memcpy(buffer, &EnemyADExp, sizeof(uchar));
    buffer += sizeof(uchar);
    memcpy(buffer, &BullseyeName, sizeof(uchar));
    buffer += sizeof(uchar);
    memcpy(buffer, &BullseyeX, sizeof(GridIndex));
    buffer += sizeof(GridIndex);
    memcpy(buffer, &BullseyeY, sizeof(GridIndex));
    buffer += sizeof(GridIndex);
    memcpy(buffer, TheaterName, sizeof(char)*CAMP_NAME_SIZE);
    buffer += sizeof(char) * CAMP_NAME_SIZE;
    memcpy(buffer, Scenario, sizeof(char)*CAMP_NAME_SIZE);
    buffer += sizeof(char) * CAMP_NAME_SIZE;
    memcpy(buffer, SaveFile, sizeof(char)*CAMP_NAME_SIZE);
    buffer += sizeof(char) * CAMP_NAME_SIZE;
    memcpy(buffer, UIName, sizeof(char)*CAMP_NAME_SIZE);
    buffer += sizeof(char) * CAMP_NAME_SIZE;
    // Might as well send this guy's squadron
    PlayerSquadronID = FalconLocalSession->GetPlayerSquadronID();
#ifdef DEBUG
    PlayerSquadronID.num_ and_eq 0x0000ffff;
#endif
#ifdef CAMPTOOL

    if (gRenameIds)
        PlayerSquadronID.num_ = RenameTable[PlayerSquadronID.num_];

#endif
    memcpy(buffer, &PlayerSquadronID, sizeof(VU_ID));
    buffer += sizeof(VU_ID);
    // write the recent event queues
    entries = 0;
    event = StandardEventQueue;

    while (event)
    {
        entries++;
        event = event->next;
    }

    memcpy(buffer, &entries, sizeof(short));
    buffer += sizeof(short);
    event = StandardEventQueue;

    while (event)
    {
        memcpy(buffer, event, sizeof(uieventnode));
        buffer += sizeof(uieventnode);
        size = _tcslen(event->eventText);
        memcpy(buffer, &size, sizeof(short));
        buffer += sizeof(short);
        memcpy(buffer, event->eventText, sizeof(_TCHAR)*size);
        buffer += sizeof(_TCHAR) * size;
        event = event->next;
    }

    // Write priority list
    entries = 0;
    event = PriorityEventQueue;

    while (event)
    {
        entries++;
        event = event->next;
    }

    memcpy(buffer, &entries, sizeof(short));
    buffer += sizeof(short);
    event = PriorityEventQueue;

    while (event)
    {
        memcpy(buffer, event, sizeof(uieventnode));
        buffer += sizeof(uieventnode);
        size = _tcslen(event->eventText);
        memcpy(buffer, &size, sizeof(short));
        buffer += sizeof(short);
        memcpy(buffer, event->eventText, sizeof(_TCHAR)*size);
        buffer += sizeof(_TCHAR) * size;
        event = event->next;
    }

    // Write the map data
    memcpy(buffer, &CampMapSize, sizeof(short));
    buffer += sizeof(short);

    if (CampMapSize)
        memcpy(buffer, CampMapData, CampMapSize);

    buffer += CampMapSize;
    // Write the squadron data
    memcpy(buffer, &LastIndexNum, sizeof(short));
    buffer += sizeof(short);
    memcpy(buffer, &NumAvailSquadrons, sizeof(short));
    buffer += sizeof(short);

    if (NumAvailSquadrons > 0)
    {
        ShiAssert(CampaignSquadronData);
        memcpy(buffer, CampaignSquadronData, sizeof(SquadUIInfoClass)*NumAvailSquadrons);
        buffer += sizeof(SquadUIInfoClass) * NumAvailSquadrons;
    }

    memcpy(buffer, &Tempo, sizeof(uchar));
    buffer += sizeof(uchar);
    memcpy(buffer, &CreatorIP, sizeof(long));
    buffer += sizeof(long);
    memcpy(buffer, &CreationTime, sizeof(long));
    buffer += sizeof(long);
    memcpy(buffer, &CreationRand, sizeof(long));
    buffer += sizeof(long);
    ShiAssert((int)(buffer - bufhead) == datasize);

    // Compress it and return
    *stream = new VU_BYTE[datasize + sizeof(long) + MAX_POSSIBLE_OVERWRITE];
    sptr = *stream;
    memcpy(sptr, &datasize, sizeof(long));
    sptr += sizeof(long);
    newsize = LZSS_Compress(bufhead, sptr, datasize);

    delete bufhead;

    return newsize + sizeof(long);
}

int CampaignClass::LoadScenarioStats(FalconGameType type, char *savefile)
{
    unsigned char /* *data,*/ *data_ptr;

    if (IsLoaded())
        return 0;

    ClearCurrentPreload();

    StartReadCampFile(type, savefile);

    gCampDataVersion = ReadVersionNumber(savefile);

    CampaignData cd;
    cd = ReadCampFile(savefile, "cmp");

    if (cd.dataSize == -1)
    {
        EndReadCampFile();
        return 0;
    }

    SetTimeCompression(0);

    TheCampaign.Suspend();

    CampEnterCriticalSection();

    data_ptr = (unsigned char *)cd.data;
    //we ignore this 4 bytes for some reason
    long size = cd.dataSize - 4;
    data_ptr += 4;
    Decode(&data_ptr, &size);
    delete cd.data;

    Flags or_eq CAMP_PRELOADED;

    if (type == game_TacticalEngagement)
        Flags or_eq CAMP_TACTICAL;

    // Let's set up our valid aircraft types
    ChillTypes();

    ReadValidAircraftTypes("ValidAC");

    CampLeaveCriticalSection();
    sprintf(SaveFile, savefile);

    TheCampaign.Resume();


    EndReadCampFile();

    gCampDataVersion = gCurrentDataVersion;

    // Notify UI of our successfull preload
    PostMessage(FalconDisplay.appWin, FM_GOT_CAMPAIGN_DATA, CAMP_NEED_PRELOAD, 0);
    return 1;
}

int CampaignClass::RequestScenarioStats(FalconGameEntity *game)
{
    FalconSessionEntity* masterSession;
    FalconRequestCampaignData* camprequest;

    if (IsLoaded())
    {
        if (game not_eq CurrentGame)
        {
            EndCampaign(); // End any current game if it's different
        }
        else
        {
            // Notify UI of our successfull preload
            PostMessage(FalconDisplay.appWin, FM_GOT_CAMPAIGN_DATA, CAMP_NEED_PRELOAD, 0);
            return 1; // Already preloaded, return success
        }
    }

    if ( not game)
    {
        return 0;
    }

    if (game not_eq CurrentGame)
    {
        ClearCurrentPreload();
        gCommsMgr->LookAtGame(game);
        //if (CurrentGame){
        // VuDeReferenceEntity(CurrentGame);
        //}
        //VuReferenceEntity(game);
        CurrentGame.reset(game);
    }

    SetTimeCompression(0);

    // Figure out who to send the request to.
    masterSession = (FalconSessionEntity*) vuDatabase->Find(game->OwnerId());

    if ( not masterSession or masterSession == vuLocalSessionEntity)
        return 0;

    switch (game->GetGameType())
    {
        case game_InstantAction:
        case game_Dogfight:
        {
            Flags or_eq CAMP_LIGHT;
            break;
        }

        case game_TacticalEngagement:
        {
            Flags or_eq CAMP_TACTICAL;
            break;
        }
    }

    Flags or_eq CAMP_NEED_PRELOAD;
    camprequest = new FalconRequestCampaignData(masterSession->Id(), masterSession);
    camprequest->dataBlock.who = vuLocalSessionEntity->Id();
    camprequest->dataBlock.dataNeeded = CAMP_NEED_PRELOAD;
    FalconSendMessage(camprequest, TRUE);

    // Let's set up our valid aircraft types
    ChillTypes();
    ReadValidAircraftTypes("ValidAC");

    return 1;
}

void CampaignClass::ClearCurrentPreload(void)
{
    if (Flags bitand CAMP_PRELOADED)
    {
        // Cleanup from previous preload
        CampEnterCriticalSection();
        DisposeEventLists();
        FreeCampMaps();
        FreeSquadronData();
        ChillTypes();
        CampLeaveCriticalSection();
        Flags and_eq compl CAMP_PRELOADED;
    }
}

// Suspend now just keeps the Campaign from entering it's loop
// Esentially pauses the thread without stopping time
void CampaignClass::Suspend(void)
{
    if (IsSuspended())
    {
        return;
    }

    ThreadManager::fast_campaign();
    Flags or_eq CAMP_SUSPEND_REQUEST;

    while ( not IsSuspended() and (Flags bitand CAMP_SUSPEND_REQUEST))
    {
        Sleep(100); // Wait until the campaign is actually suspended
    }

    ThreadManager::slow_campaign();
}

void CampaignClass::Resume(void)
{
    if ( not IsSuspended())
    {
        return;
    }

    Flags xor_eq CAMP_SUSPENDED;
}

void CampaignClass::SetOnlineStatus(int online)
{
    if (online)
    {
        Flags or_eq CAMP_ONLINE;
    }
    else
    {
        Flags and_eq compl CAMP_ONLINE;
    }
}

void CampaignClass::GetBullseyeLocation(GridIndex *x, GridIndex *y)
{
    *x = BullseyeX;
    *y = BullseyeY;
}

void CampaignClass::GetBullseyeSimLocation(float *x, float *y)
{
    // KCK: Remember - swap axises.
    *y = GridToSim(BullseyeX);
    *x = GridToSim(BullseyeY);
}

uchar CampaignClass::GetBullseyeName(void)
{
    return BullseyeName;
}

void CampaignClass::SetBullseye(uchar nameid, GridIndex x, GridIndex y)
{
    BullseyeName = nameid;
    BullseyeX = x;
    BullseyeY = y;
}

int CampaignClass::BearingToBullseyeDeg(float x, float y)
{
    float bx, by, theta;

    // KCK: Remember - swap axises.
    by = GridToSim(BullseyeX);
    bx = GridToSim(BullseyeY);
    theta = (float)atan2((bx - x) , (by - y));
    theta = (float)atan2((by - y) , (bx - x));
    return FloatToInt32(theta * RTD);
}

int CampaignClass::RangeToBullseyeFt(float x, float y)
{
    float bx, by;

    // KCK: Remember - swap axises.
    by = GridToSim(BullseyeX);
    bx = GridToSim(BullseyeY);
    return FloatToInt32(Distance(x, y, bx, by));
}

void CampaignClass::GetPlayerLocation(GridIndex *x, GridIndex *y)
{
    vector pos;
    VuEntity *player;

    player = FalconLocalSession->GetPlayerEntity();

    if ( not player)        // not in the cockpit
    {
        *x = *y = 0;
        return;
    }

    pos.x = player->XPos();
    pos.y = player->YPos();
    ConvertSimToGrid(&pos, x, y);
}

int CampaignClass::IsMaster(void)
{
    return (FalconLocalGame->IsLocal());
}

// ==============================================
// Shit to figure out which events to show the UI
// ==============================================

// This will return a list of the most recent standard campaign events.
CampUIEventElement* CampaignClass::GetRecentEventlist(void)
{
    return StandardEventQueue;
}

// This will return a list of the most recent priority events (objective captured and triggered events).
CampUIEventElement* CampaignClass::GetRecentPriorityEventList(void)
{
    return PriorityEventQueue;
}

// This adds an event noed to the queue we're keeping - must be called from the event process function
void CampaignClass::AddCampaignEvent(CampUIEventElement *newEvent)
{
    int added = 0;

    CampEnterCriticalSection();

    if (newEvent->flags bitand 0x01)
    {
        if ( not PriorityEventQueue or strcmp(PriorityEventQueue->eventText, newEvent->eventText) not_eq 0)
        {
            newEvent->next = PriorityEventQueue;
            PriorityEventQueue = newEvent;
            added = 1;
            TrimEventList(PriorityEventQueue, PRIORITY_EVENT_LENGTH);
        }
        else
        {
            delete newEvent->eventText;
            delete newEvent;
        }
    }
    else
    {
        if ( not StandardEventQueue or strcmp(StandardEventQueue->eventText, newEvent->eventText) not_eq 0)
        {
            newEvent->next = StandardEventQueue;
            StandardEventQueue = newEvent;
            added = 1;
            TrimEventList(StandardEventQueue, STANDARD_EVENT_LENGTH);
        }
        else
        {
            delete newEvent->eventText;
            delete newEvent;
        }
    }

#ifdef KEV_DEBUG

    if (added)
        MonoPrint("CampEvent: %s\n", newEvent->eventText);

#endif

    CampLeaveCriticalSection();
}

void CampaignClass::DisposeEventLists(void)
{
    CampEnterCriticalSection();
    DisposeEventList(TheCampaign.StandardEventQueue);
    DisposeEventList(TheCampaign.PriorityEventQueue);
    StandardEventQueue = NULL;
    PriorityEventQueue = NULL;
    CampLeaveCriticalSection();
}

// This function disposes all but the last 10 eventsm from each list, just to keep our size
// down if the UI isn't calling us.
void CampaignClass::TrimCampUILists(void)
{
    return;
    /*
       CampUIEventElement *sEvent,*pEvent;
       int i;

       sEvent = StandardEventQueue;
       pEvent = PriorityEventQueue;
       for (i=0; i<STANDARD_EVENT_LENGTH; i++)
       {
       if (sEvent)
       sEvent = sEvent->next;
       if (pEvent)
       pEvent = pEvent->next;
       }
       if (sEvent)
       {
       DisposeEventList(sEvent->next);
       sEvent->next = NULL;
       }
       if (pEvent)
       {
       DisposeEventList(pEvent->next);
       pEvent->next = NULL;
       }
     */
}

// ==========================
// Map Stuff (small map)
// ==========================

uchar* CampaignClass::MakeCampMap(int type)
{
    switch (type)
    {
        case MAP_SAMCOVERAGE:
            SamMapData = ::MakeCampMap(type, SamMapData, SamMapSize);
            SamMapSize = sizeof(uchar) * (Map_Max_X / MAP_RATIO) * (Map_Max_Y / MAP_RATIO);
            return SamMapData;

        case MAP_RADARCOVERAGE:
            RadarMapData = ::MakeCampMap(type, RadarMapData, RadarMapSize);
            RadarMapSize = sizeof(uchar) * (Map_Max_X / MAP_RATIO) * (Map_Max_Y / MAP_RATIO);
            return RadarMapData;

        default:
            CampMapData = ::MakeCampMap(type, CampMapData, CampMapSize);
            CampMapSize = sizeof(uchar) * (Map_Max_X / MAP_RATIO) * (Map_Max_Y / MAP_RATIO) / 2;
            return CampMapData;
    }

    return NULL;
}

void CampaignClass::FreeCampMaps(void)
{
    FreeCampMap(CampMapData);
    CampMapData = NULL;
    FreeCampMap(SamMapData);
    SamMapData = NULL;
    FreeCampMap(RadarMapData);
    RadarMapData = NULL;
    CampMapSize = SamMapSize = RadarMapSize = 0;
}

// ==========================
// Squadron UI data stuff
// ==========================

// This should be called just before the Campaign is saved.
// It builds scaled down database of available squadrons
void CampaignClass::VerifySquadrons(int team)
{
    Unit u;
    int s, squadrons = 0;
    SquadUIInfoClass *newData;

    if ( not AllAirList)
        return;

    FreeSquadronData();

    // First, let's see how many squadrons we have
    VuListIterator myit(AllAirList);
    u = (Unit) myit.GetFirst();

    while (u)
    {
        if (u->IsSquadron())
        {
            squadrons++;
        }

        u = (Unit) myit.GetNext();
    }

    if ( not squadrons)
        return;

    // Next, allocate a new array of data and fill it
    newData = new SquadUIInfoClass[squadrons];
    s = 0;
    u = (Unit) myit.GetFirst();

    while (u)
    {
        if (u->IsSquadron())
        {
            GridIndex x, y;
            Objective o;

            newData[s].x = u->XPos();
            newData[s].y = u->YPos();
            newData[s].id = u->Id();
            newData[s].dIndex = u->Type() - VU_LAST_ENTITY_TYPE;
            newData[s].specialty = (uchar) u->GetUnitSpecialty();
            newData[s].nameId = u->GetUnitNameID();
            newData[s].country = u->GetOwner();
            newData[s].currentStrength = u->GetTotalVehicles();
            newData[s].squadronPatch = ((Squadron)u)->GetPatchID();
            u->GetLocation(&x, &y);
            o = GetObjectiveByXY(x, y);

            if (o)
            {
                newData[s].airbaseName[0] = 0;
                o->GetName(newData[s].airbaseName, 39, TRUE);
                newData[s].airbaseIcon = o->GetObjectiveClassData()->IconIndex;
            }

            s++;
        }

        u = (Unit) myit.GetNext();
    }

    NumAvailSquadrons = squadrons;
    delete CampaignSquadronData;
    CampaignSquadronData = newData;
}

void CampaignClass::FreeSquadronData(void)
{
    NumAvailSquadrons = 0;
    LastIndexNum = -1;
    delete CampaignSquadronData;
    CampaignSquadronData = NULL;
}

void CampaignClass::ReadValidAircraftTypes(char *typefile)
{
    char /* *data, */*data_ptr;
    int eClass[8], i, size;

    CampaignData cd;
    cd = ReadCampFile(typefile, "act");

    if (cd.dataSize == -1)
    {
        return;
    }

    data_ptr = cd.data;

    sscanf(data_ptr, "%d%n", &NumberOfValidTypes, &size);

    if (NumberOfValidTypes == CAMP_FLY_ANY_AIRCRAFT)
        return;

    data_ptr += size;
    ValidAircraftTypes = new short[NumberOfValidTypes];

    for (i = 0; i < NumberOfValidTypes; i++)
    {
        sscanf(data_ptr, "%d %d %d %d %d %d %d %d%n", &eClass[0], &eClass[1], &eClass[2], &eClass[3], &eClass[4], &eClass[5], &eClass[6], &eClass[7], &size);
        data_ptr += size;
        ValidAircraftTypes[i] = GetClassID(eClass[0], eClass[1], eClass[2], eClass[3], eClass[4], eClass[5], eClass[6], eClass[7]);
    }

    //delete data;
    delete cd.data;
}

int CampaignClass::IsValidAircraftType(Unit u)
{
    int type, i;

    // KCK: I'm intentionally return 0 in the FLY_ANY_AIRCRAFT
    // state, because we use this to determine if a mission should
    // be assigned to a 'player' type squadron
    if (NumberOfValidTypes < CAMP_FLY_ANY_AIRCRAFT)
    {
        type = u->Type() - VU_LAST_ENTITY_TYPE;

        if ( not type)
            return 0;

        for (i = 0; i < NumberOfValidTypes; i++)
        {
            if (type == ValidAircraftTypes[i])
                return 1;
        }
    }

    return 0;
}

int CampaignClass::IsValidSquadron(int id)
{
    if (id >= NumAvailSquadrons)
        return 0;

    if (NumberOfValidTypes == CAMP_FLY_ANY_AIRCRAFT)
        return 1;

    for (int i = 0; i < NumberOfValidTypes; i++)
    {
        if (CampaignSquadronData[id].dIndex == ValidAircraftTypes[i])
            return 1;
    }

    return 0;
}

void CampaignClass::ChillTypes(void)
{
    if (NumberOfValidTypes > 0 and NumberOfValidTypes not_eq CAMP_FLY_ANY_AIRCRAFT)
    {
        NumberOfValidTypes = 0;
        delete [] ValidAircraftTypes;
        ValidAircraftTypes = NULL;
    }
}

// ==========================
// CampLib functions
// ==========================

void Camp_Init(int processor)
{
    TheCampaign.Flags = 0;
    TheCampaign.InMainUI = true; // we start in the main UI, don't we ? ;)
    TheCampaign.Reset();
    TheCampaign.Processor = processor;
    campCritical = F4CreateCriticalSection("campCritical");
#if MF_DONT_PROCESS_DELETE or VU_USE_ENUM_FOR_TYPES
    FalconMessageFilter campFilter(FalconEvent::CampaignThread, 0);
#else
    FalconMessageFilter campFilter(FalconEvent::CampaignThread, VU_DELETE_EVENT_BITS);
#endif
    TheCampaign.vuThread = new VuThread(&campFilter, F4_EVENT_QUEUE_SIZE * 4);
    SetTimeCompression(0);
    TheCampaign.LoopStarter();
    TheCampaign.Suspend();
    {
        F4ScopeLock l(campCritical);
        InitBaseLists();
        ReadIndex("Strings");
        LoadPriorityTables();
    }
}

void Camp_Exit(void)
{
    {
        F4ScopeLock l(campCritical);

        if (TheCampaign.IsLoaded())
        {
            TheCampaign.EndCampaign();
        }

        FreeIndex();

        if (TheCampaign.IsSuspended())
        {
            TheCampaign.Resume(); // Unpause, so we can exit
        }
    }

    // sfr: must be outside lock since campaign can be locked, causing deadlock
    ThreadManager::stop_campaign_thread();

    {
        F4ScopeLock l(campCritical);

        if (TheCampaign.vuThread)
        {
#if CAP_DISPATCH
            TheCampaign.vuThread->Update(-1);
#else
            TheCampaign.vuThread->Update();
#endif
            delete TheCampaign.vuThread;
            TheCampaign.vuThread = NULL;
        }

        DisposeBaseLists();
    }
    F4DestroyCriticalSection(campCritical);
    campCritical = NULL; // JB 010108
}

CampaignTime Camp_GetCurrentTime(void)
{
    return TheCampaign.GetCampaignTime();
}

void Camp_FreeMemory(void)
{
    delete ASD;
    ASD = NULL;
    //sfr: Real weather destructor shouldnt be here
    /* if (realWeather not_eq NULL){
     delete realWeather;
     realWeather = NULL;
     }
    */
}

void TrashCampaignUnits(void)
{
    Unit u;

    FalconLocalSession->SetPlayerEntity(NULL);
    FalconLocalSession->SetPlayerFlight(NULL);
    FalconLocalSession->SetPlayerSquadron(NULL);
    VuListIterator myit(AllUnitList);
    u = (Unit) myit.GetFirst();

    while (u)
    {
        u->KillUnit();
        vuDatabase->Remove(u);
        u = (Unit) myit.GetNext();
    }
}

// This will convert the current campaign to instant action format
void Camp_MakeInstantAction(void)
{
    TheCampaign.Flags or_eq CAMP_LIGHT;
    TheCampaign.DisposeEventLists();
    TheCampaign.FreeCampMaps();
    TheCampaign.FreeSquadronData();
    TheCampaign.ChillTypes();
    TheCampaign.CurrentTime = 0;
    //TrashCampaignUnits();
}

int ReadVersionNumber(char *saveFile)
{
    //char *data;

    int vers = 1; // If we can't load a version file, assume the worst ? RH

    CampaignData cd = ReadCampFile(saveFile, "ver");

    if (cd.dataSize not_eq -1)
    {
        sscanf(cd.data, "%d", &vers);
        delete cd.data;
    }

    return vers;
}

void WriteVersionNumber(char *saveFile)
{
    FILE *fp;
    int vers = gCurrentDataVersion;

    fp = OpenCampFile(saveFile, "ver", "w");

    if (fp)
    {
        fprintf(fp, "%d", vers);
        CloseCampFile(fp);
    }
}

void NukeHistoryFiles(void)
{
    char filename[MAX_PATH];

    sprintf(filename, "%s\\tmp.his", FalconCampUserSaveDirectory, TheCampaign.SaveFile);
    unlink(filename);
    sprintf(filename, "%s\\tmp.frc", FalconCampUserSaveDirectory, TheCampaign.SaveFile);
    unlink(filename);
#ifdef DEBUG
    sprintf(filename, "campaign\\save\\dump\\errors.log");
    unlink(filename);
#endif
}

int SaveAfterRename(char *savefile, FalconGameType gametype)
{
    FILE* fp;
    char filename[MAX_PATH];

    strcpy(filename, savefile);

    StartWriteCampFile(gametype, filename);

    CampEnterCriticalSection();

    if (gametype == game_TacticalEngagement)
    {
        current_tactical_mission->save_data(filename);
    }

    // if ( not CampMapSize or not TheaterSizeX or not CampMapData or save_mode == CAMP_SAVE_FULL)
    // MakeCampMap(MAP_OWNERSHIP);
    TheCampaign.VerifySquadrons(FALCON_PLAYER_TEAM);

    fp = OpenCampFile(filename, "cmp", "wb");

    if ( not fp)
    {
        MonoPrint("Error opening file %s\n", filename);
        return 0;
    }

    TheCampaign.SaveData(fp);
    CloseCampFile(fp);
    SaveBaseObjectives(filename);
    SaveObjectiveDeltas(filename);
    SaveUnits(filename);
    SaveTeams(filename);
    SaveCampaignEvents(filename);
    SavePilotInfo(filename);
    SavePersistantList(filename);
    ((WeatherClass*)realWeather)->Save(filename);

    WriteVersionNumber(filename);
    EndWriteCampFile();

    CampLeaveCriticalSection();

    MonoPrint("RENAMING OF %s COMPLETED SUCCESSFULLY\n", filename);
    return 1;
}
