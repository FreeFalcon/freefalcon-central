
#include "Mesg.h"
#include "find.h"
#include "cmpclass.h"
#include "MsgInc/RequestAircraftSlot.h"
#include "uicomms.h"
#include "ui_cmpgn.h"
#include "FalcSess.h"
#include "DispCfg.h"
#include "Flight.h"
#include "CampJoin.h"
#include "ui95/CHandler.h"
#include "userids.h"
#include "Options.h"
#include "Dogfight.h"
#include "TimerThread.h"
#include "MissEval.h"
#include "Team.h"
#include "Gtm.h"
#include "Ntm.h"

// JB 010731
#include "textids.h"
extern void CommsErrorDialog(long TitleID, long MessageID, void (*OKCB)(long, short, C_Base*), void (*CancelCB)(long, short, C_Base*));
extern char gUI_CampaignFile[];

// MN 020121
bool campaignStart;

// ============================
// Externals
// ============================

extern int FalconConnectionDescription;
extern C_Handler *gMainHandler;
extern _TCHAR gUI_ScenarioName[];
extern int gCurrentDataVersion;
extern int gCampDataVersion;
extern int CampSelMode;
extern int MainLastGroup;
extern VU_ID gPlayerSquadronId;
extern int tactical_mission_loaded;

extern char gCurDogfightFile[MAX_PATH];

extern void RecieveScenarioInfo();
extern void CopyinTempSettings(void);
extern void CampaignSetup(void);
extern void EnableCampaignMenus(void);
extern void DeleteGroupList(long ID);
extern void CopyDFSettingsToWindow(void);
extern void SetCampaignStartupMode(void);
extern void DogfightJoinSuccess(void);
extern void ActivateCampMissionSchedule(void);
extern void tactical_play_setup(void);
extern void RequestEvalData(void);
extern void ChooseBullseye(void); // 2002-04-18 MN

// ============================
// Globals
// ============================

int gCampJoinStatus = 0; // This stores what stage of loading we're currently in
ulong gCampJoinLastData = 0; // Last vuxRealtime we received data about this game
ulong gCampJoinTimeout = 0; // How long we're willing to wait for the next set of data
uchar gCampJoinTries = 0; // How many times we've re-requested campaign data
int gCampJoinGameType = 0; // Type of game we're joining (Campaign/TacEng/Dogfight)

// ============================
// Prototypes
// ============================

void CampaignConnectionTimer(void);
void DisplayJoinStatusWindow(int bits);
void OpenPriorityCB(long ID, short hittype, C_Base *control);

// ==========================================================================================
// The following functions are used exclusively for requesting and receiving campaign data
// - Either remotely or locally
// ==========================================================================================

// This function starts the data collection process (for joining or loading a game)
void StartCampaignGame(int local, int game_type)
{
    SYSTEMTIME time;
    long timestamp;

    SetTimeCompression(0);

    if (local)
    {
        // Load a Campaign
        gCampJoinStatus = 0;
        gCampJoinLastData = 0;
        gCampJoinTimeout = 0;
        gCampJoinTries = 0;
        gCampJoinGameType = game_type;
        SendMessage(FalconDisplay.appWin, FM_LOAD_CAMPAIGN, 0, game_type);
        _tcscpy(TheCampaign.SaveFile, gUI_ScenarioName);

        GetSystemTime(&time);
        timestamp  = time.wYear - 1998;
        timestamp  = time.wMonth + (timestamp * 12);
        timestamp  = time.wDay + (timestamp * 31);
        timestamp  = time.wHour + (timestamp * 24);
        timestamp  = time.wMinute + (timestamp * 60);
        timestamp  = time.wSecond + (timestamp * 60);

        if ( not TheCampaign.GetCreationIter())
        {
            TheCampaign.SetCreatorIP(FalconLocalSessionId.creator_);
            TheCampaign.SetCreationTime(timestamp);
            TheCampaign.SetCreationIter(1); // Iteration of this file
        }

        gCampDataVersion = gCurrentDataVersion;
    }
    else
    {
        DisplayJoinStatusWindow(0);
        // Join a Campaign
        gCampDataVersion = gCurrentDataVersion;
        gCampJoinStatus = JOIN_REQUEST_ALL_DATA;
        gCampJoinLastData = vuxRealTime;
        gCampJoinTries = 0;
        gCampJoinGameType = game_type;

        switch (FalconConnectionDescription)
        {
            case FCT_NoConnection:
            default:
                gCampJoinTimeout = 0;
                break;

            case FCT_ModemToModem:
            case FCT_NullModem:
                gCampJoinTimeout = 1000;
                break;

            case FCT_LAN:
                gCampJoinTimeout = 1000;
                break;

            case FCT_WAN:
            case FCT_Server:
            case FCT_TEN:
            case FCT_JetNet:
                gCampJoinTimeout = 1000;
                break;
        }

        // Set up our timeout callback
        gMainHandler->AddUserCallback(CampaignConnectionTimer);
        SendMessage(FalconDisplay.appWin, FM_JOIN_CAMPAIGN, JOIN_REQUEST_ALL_DATA, game_type);
    }
}

// This is called any time we've received Campaign Scenario Status data (preload data)
void CampaignPreloadSuccess(int remote_game)
{
    if (remote_game and not TheCampaign.IsLoaded() and gCampJoinStatus == JOIN_REQUEST_ALL_DATA)
    {
        // We want the rest of the data too.
        gCampJoinStatus = JOIN_CAMP_DATA_ONLY;
        PostMessage(FalconDisplay.appWin, FM_JOIN_CAMPAIGN, JOIN_CAMP_DATA_ONLY, gCampJoinGameType);
    }
}

void CampaignJoinSuccess(void)
{
    MonoPrint("Got all campaign data Starting it up\n");

    if (gMainHandler)
        gMainHandler->RemoveUserCallback(CampaignConnectionTimer);

    campaignStart = true;

    // Do all game dependent stuff here:
    if (FalconLocalGame->GetGameType() == game_Dogfight)
    {
        if (FalconLocalGame->IsLocal())
        {
            // Load Settings
            SimDogfight.SetFilename(gCurDogfightFile);
            SimDogfight.LoadSettings();
            CopyDFSettingsToWindow();
            SimDogfight.ApplySettings();
        }
        else
        {
            // Request Settings (quite possibly again)
            SimDogfight.RequestSettings(FalconLocalGame);
            TheCampaign.MissionEvaluator->RebuildEvaluationData();
            RequestEvalData();
        }

        DogfightJoinSuccess();
    }
    else if (FalconLocalGame->GetGameType() == game_TacticalEngagement)
    {
        tactical_mission_loaded = TRUE;

        if ( not (TheCampaign.Flags bitand CAMP_TACTICAL_EDIT))
        {
            tactical_play_setup();

            gCommsMgr->SetCampaignFlag(game_TacticalEngagement);

            if ( not FalconLocalGame->IsLocal())
                TheCampaign.StartRemoteCampaign(FalconLocalGame);

            gCommsMgr->LoadStats();
        }
        else
        {
        }
    }
    else if (FalconLocalGame->GetGameType() == game_Campaign)
    {
        C_Window *win;
        C_EditBox *ebox;
        CampEntity ps;

        ps = (CampEntity) FindUnit(gPlayerSquadronId);

        if (ps and ps->IsSquadron())
            FalconLocalSession->SetPlayerSquadron((Squadron)ps);

        if (gMainHandler)
        {
            CopyinTempSettings();

            if ( not CampSelMode)
                AdjustCampaignOptions();
            else
                AdjustExperienceLevels();

            win = gMainHandler->FindWindow(CS_PUA_WIN);

            if (win)
            {
                ebox = (C_EditBox *)win->FindControl(TITLE_LABEL);

                if (ebox)
                    _tcsnccpy(TheCampaign.SaveFile, ebox->GetText(), CAMP_NAME_SIZE - 1);

                TheCampaign.SaveFile[CAMP_NAME_SIZE - 1] = 0;
            }

            if ( not FalconLocalGame->IsLocal())
                TheCampaign.StartRemoteCampaign(FalconLocalGame);

            // 2002-04-18 MN redo bullseye selection once more after we have the theaters reference point from the .tri file
            ChooseBullseye();

            CampaignSetup();
            campaignStart = false; // next time CampaignSetup() is called, don't stop the clock.
            gMainHandler->EnterCritical();

            if (MainLastGroup)
                gMainHandler->DisableWindowGroup(MainLastGroup);

            DeleteGroupList(CS_SUA_WIN);
            gMainHandler->DisableWindowGroup(100);
            gMainHandler->DisableSection(100);
            gMainHandler->SetSection(200);
            gMainHandler->EnableWindowGroup(200);

            // 2002-01-03 M.N.
            // If we started a new campaign, make some first task manager calculations and pop up the priority windows
            if (FalconLocalGame->IsLocal() and 
                (strcmp(gUI_CampaignFile, "save0") == 0 or strcmp(gUI_CampaignFile, "save1") == 0 or strcmp(gUI_CampaignFile, "save2") == 0))
            {
                C_Window *winme = NULL;
                C_Button *ctrl = NULL;

                // Do first round of calculations to get PAK priorities and other stuff set up
                for (int t = 0; t < NUM_TEAMS; t++)
                {
                    if (TeamInfo[t])
                    {
                        TeamInfo[t]->atm->DoCalculations();
                        TeamInfo[t]->gtm->DoCalculations();
                        // TeamInfo[t]->ntm->DoCalculations(); // don't plan naval stuff at the beginning
                    }
                }

                // first pop up the "Start Campaign" window
                winme = gMainHandler->FindWindow(STARTCAMP_WIN);

                if (winme)
                {
                    gMainHandler->ShowWindow(winme);
                    gMainHandler->WindowToFront(winme);
                }

                // then the priority window - hack, mimic a "P" button press :-)
                winme = gMainHandler->FindWindow(CP_PUA_MAP);

                if (winme)
                {
                    ctrl = (C_Button*)winme->FindControl(SET_PRIORITIES);
                    OpenPriorityCB(0, C_TYPE_LMOUSEUP, ctrl);
                }
            }

            gCommsMgr->SetCampaignFlag(game_Campaign);
            ActivateCampMissionSchedule();
            gMainHandler->LeaveCritical();
            SetCursor(gCursors[CRSR_F16]);
            gCommsMgr->LoadStats();
        }

#ifdef DEBUG
        EnableCampaignMenus();
#endif
    }

    SetCampaignStartupMode();
}

void CampaignJoinFail(void)
{
    MonoPrint("Failed to get campaign data\n");

    StopCampaignLoad();

    // PETER TODO: Pop up a 'Failed to get data' message

    // JB 010731
    C_Window
    *win;

    win = gMainHandler->FindWindow(COMMLINK_WIN);

    if (win)
    {
        gMainHandler->HideWindow(win);
    }

    CommsErrorDialog(TXT_JOINING_GAME, TXT_COMMS_NO_SERVER, NULL, NULL);
}

void StopCampaignLoad(void)
{
    MonoPrint("Stop Campaign Load\n");

    gMainHandler->RemoveUserCallback(CampaignConnectionTimer);

    PostMessage(FalconDisplay.appWin, FM_SHUTDOWN_CAMPAIGN, 0, game_Campaign);
}

// This is called when we've gotten any sort of join data to keep us from timing out
void CampaignJoinKeepAlive(void)
{
    gCampJoinLastData = vuxRealTime;
}

// This is the timer routine which runs during the campaign join process
void CampaignConnectionTimer(void)
{
    ulong elapsedTime = vuxRealTime - gCampJoinLastData;

    // Abort entire load process if we've waited to long
    if (elapsedTime > gCampJoinTimeout)
    {
        gCampJoinTries++;

        // If we fail to many times, we quit
        if (gCampJoinTries > 600)
        {
            MonoPrint("Join Timed out\n");
            PostMessage(FalconDisplay.appWin, FM_JOIN_FAILED, 0, 0);
            return;
        }

        gCampJoinLastData = vuxRealTime;

        /* Don't repost messages - these are now sent reliably
         if ( not TheCampaign.IsPreLoaded())
         PostMessage(FalconDisplay.appWin,FM_JOIN_CAMPAIGN,JOIN_REQUEST_ALL_DATA,gCampJoinGameType);
         else
         PostMessage(FalconDisplay.appWin,FM_JOIN_CAMPAIGN,JOIN_CAMP_DATA_ONLY,gCampJoinGameType);
        */
    }
}

