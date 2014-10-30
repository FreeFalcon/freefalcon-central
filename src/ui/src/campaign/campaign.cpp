/***************************************************************************\
 UI_Cmpgn.cpp
 Peter Ward
 December 3, 1996

 Main UI screen stuff for FreeFalcon
\***************************************************************************/
#include <windows.h>
#include "falclib.h"
#include "targa.h"
#include "dxutil/ddutil.h"
#include "Graphics/Include/imagebuf.h"
#include "Graphics/Include/matrix.h"
#include "Graphics/Include/drawbsp.h"
#include "chandler.h"
#include "ui95_ext.h"
#include "cmusic.h"
#include "entity.h"
#include "feature.h"
#include "vehicle.h"
#include "evtparse.h"
#include "Mesg.h"
#include "MsgInc/DamageMsg.h"
#include "MsgInc/WeaponFireMsg.h"
#include "MsgInc/DeathMessage.h"
#include "MsgInc/MissileEndMsg.h"
#include "MsgInc/LandingMessage.h"
#include "find.h"
#include "cmpclass.h"
#include "division.h"
#include "misseval.h"
#include "campstr.h"
#include "team.h"
#include "history.h"
#include "falcuser.h"
#include "f4find.h"
#include "timerthread.h"
#include "f4error.h"
#include "cmap.h"
#include "uicomms.h"
#include "ui_cmpgn.h"
#include "cstores.h"
#include "cbsplist.h"
#include "c3dview.h"
#include "Graphics/Include/loader.h"
#include "gps.h"
#include "userids.h"
#include "textids.h"
#include "FalcSess.h"
#include "Campaign.h"
#include "Falclib/Include/ui.h"
#include "icondefs.h"
#include "teamdata.h"
#include "DispCfg.h"
#include "ACSelect.h"
#include "playerop.h"
#include "urefresh.h"
#include "teamdata.h"

void CampHackButton1CB(long ID, short hittype, C_Base *control);
void CampHackButton2CB(long ID, short hittype, C_Base *control);
void CampHackButton3CB(long ID, short hittype, C_Base *control);
void CampHackButton4CB(long ID, short hittype, C_Base *control);
void CampHackButton5CB(long ID, short hittype, C_Base *control);


GlobalPositioningSystem *gGps = NULL;

int StupidHackToCloseCSECT = 0;

extern C_Handler *gMainHandler;
extern C_Parser *gMainParser;;
extern int MainLastGroup;
extern int CPLoaded, COLoaded;
extern int CampaignLastGroup, TacLastGroup;
extern BOOL gNewMessage;
extern IMAGE_RSC *gOccupationMap;
extern long StopLookingforMission;
extern C_Base *CurMapTool;
extern int gTimeModeServer;
extern bool g_bServer;
extern OBJECTINFO Recon;
extern long gRefreshScoresList;

extern VU_ID gCurrentFlightID; // Current Mission Flight (Mission Window) Also sets gSelectedFlight
extern VU_ID gPlayerFlightID; // Flight Player is in (NULL) if not in a flight
extern short gPlayerPlane; // Player's current slot
extern VU_ID gActiveFlightID; // Current Selected Waypoint flight
extern short gActiveWPNum; // Current Waypoint
extern VU_ID gSelectedFlightID; // Last flight Selected (in ATO,OOB,Mission etc)
extern VU_ID gLoadoutFlightID; // Currnet flight being Loaded with weapons
extern VU_ID gSelectedEntity;
extern VU_ID gInterceptersId;
extern RulesClass CurrRules;
extern long ShowGameOverWindow;
//LISTBOX *gTaskList=NULL;
VU_ID ReconFlightID = FalconNullId;
int   ReconWPNum = 0;
int gAWWTimeout = 0;
int gMoveBattalion = FALSE;
C_3dViewer *gUIViewer = NULL;
extern C_TreeList *gATOAll, *gATOPackage, *gOOBTree, *gVCTree;
extern short g3dObjectID;
long LastForceCatID = 0;
extern uchar *te_restore_map;

short InCleanup = 0;

extern uchar gSelectedTeam;
extern IMAGE_RSC *PAKMap;
extern char gUI_CampaignFile[];
extern bool campaignStart;

extern bool g_bAWACSSupport;
extern bool g_bAWACSBackground;
extern bool g_bEmptyFilenameFix; // 2002-04-18 MN
extern bool g_bTakeoffSound; //TJL 10/24/03 Allows for removal of takeoff wav.

#ifdef DEBUG
extern int gPlayerPilotLock;
#endif


// HISTORY (JSTARS) Variables
static long JStarsFirst = 0;
static long JStarsLast = 0;
static long JStarsCurrent = 0;
static long JStarsPrevious = 0;
static long JStarsDirection = 0;
static long JStarsDelay = 0;

void EndGameEvaluation();
void UI_Help_Guide_CB(long ID, short hittype, C_Base *ctrl);
void add_all_vcs_to_ui();
void UpdateSierraHotel();
void PositionCamera(OBJECTINFO *Info, C_Window *win, long client);
void CampaignAutoSave(FalconGameType gametype);
void PlayUIMovie(long ID);
void PlayUIMusic();
void PlayCampaignMusic();
void ViewTimerCB(long ID, short hittype, C_Base *control);
void ViewObjectCB(long ID, short hittype, C_Base *control);
void MoveViewTimerCB(long ID, short hittype, C_Base *control);
void PositionSlider(C_Slider *slider, long value, long minv, long maxv);
void ClearMapToolStates(long ID);
void CheckCampaignFlyButton();
void DisplayView(long ID, short hittype, C_Base *control);
static void HookupCampaignControls(long ID);
void RemoveMissionCB(TREELIST *item);
static void MapMgrDrawCB(long ID, short hittype, C_Base *control);
static void MapMgrMoveCB(long ID, short hittype, C_Base *control);
void GotoPrevWaypointCB(long ID, short hittype, C_Base *control);
void GotoNextWaypointCB(long ID, short hittype, C_Base *control);
void UpdateWaypointWindowInfo(C_Window *win, WayPoint wp, int wpnum, int flag = TRUE);
void Uni_Float(_TCHAR *buffer);
void DelSTRFileCB(long ID, short hittype, C_Base *control);
void DelDFSFileCB(long ID, short hittype, C_Base *control);
void DelLSTFileCB(long ID, short hittype, C_Base *control);
void DelCamFileCB(long ID, short hittype, C_Base *control);
void DelTacFileCB(long ID, short hittype, C_Base *control);
void DelTGAFileCB(long ID, short hittype, C_Base *control);
void DelVHSFileCB(long ID, short hittype, C_Base *control);
void DelKeyFileCB(long ID, short hittype, C_Base *control);
void SetDeleteCallback(void (*cb)(long, short, C_Base*));
void CampaignSoundEventCB();
void CampaignListCB();
void TacEngListCB();
void ACMIButtonCB(long ID, short hittype, C_Base *control);
void SetupOOBWindow();
void CleanupTacticalEngagementUI(void);
void UpdateIntelBarCB(long ID, short hittype, C_Base *control);
void MakeOccupationMap(IMAGE_RSC *Map);
void MakeBigOccupationMap(IMAGE_RSC *Map);
void UpdateIntel(long ID);
short GetFlightStatusID(Flight element);
void RefreshEventList();
void FindMissionInBriefing(long ID);
void RefreshMapEventList(long winID, long client);
void do_tactical_debrief();
void SelectMissionSortCB(long ID, short hittype, C_Base *control);
void SetSingle_Comms_Ctrls();
void SelectScenarioCB(long ID, short hittype, C_Base *control);
void OpenLogBookCB(long ID, short hittype, C_Base *control);
void LoadCommsWindows();
void GenericTimerCB(long ID, short hittype, C_Base *control);
void RefreshGameListCB(long ID, short hittype, C_Base *control);
void BlinkCommsButtonTimerCB(long ID, short hittype, C_Base *control);
void LoadCommonWindows();
long gLastUpdateGround = 0l, gLastUpdateAir = 0;
extern int mcnt, atocnt, uintcnt;
void OpenMunitionsWindowCB(long ID, short hittype, C_Base *control);
void OpenSetupCB(long ID, short hittype, C_Base *control);
void SaveAFile(long TitleID, _TCHAR *filespec, _TCHAR *excludelist[], void (*YesCB)(long, short, C_Base*), void (*NoCB)(long, short, C_Base*), _TCHAR *filename);
void UpdateMissionList(long ID, VU_ID squadronID);
void TallyPlayerSquadrons();
void SetupUnitInfoWindow(VU_ID unitID);
void SetBullsEye(C_Window *);
void SetSlantRange(C_Window *);
void ACMIButtonCB(long ID, short hittype, C_Base *control);
void OpenTacticalReferenceCB(long ID, short hittype, C_Base *control);
void Cancel_Scramble_CB(long ID, short hittype, C_Base *control);
void LoadTroopMovementHistory();
void LoadForceLevelHistory();
void SetupSquadronInfoWindow(VU_ID TheID);
void PriorityTabsCB(long ID, short hittype, C_Base *control);
void UsePriotityCB(long ID, short hittype, C_Base *control);
void ResetPriorityCB(long ID, short hittype, C_Base *control);
void CancelPriorityCB(long ID, short hittype, C_Base *control);
void MapSelectPAKCB(long ID, short hittype, C_Base *control);
void SelectPAKCB(long ID, short hittype, C_Base *control);
void SetPAKPriorityCB(long ID, short hittype, C_Base *control);
void InitPAKNames();
BOOL PAKMapTimerCB(C_Base *me);
void SetCampaignPrioritiesCB(long ID, short hittype, C_Base *base);
void OpenPriorityCB(long ID, short hittype, C_Base *control);
static void OpenCampaignCB(long ID, short hittype, C_Base *control);
void ShutdownCampaign(void);
void gMapMgr_TACmover(long ID, short hittype, C_Base *control);
void SetupTacEngMenus(short Mode);
void SetupCampaignMenus();
void SetMapSettings();
void ValidateWayPoints(Flight flt);
void InitVCArgLists();
void CleanupVCArgLists();
void CloseWindowCB(long ID, short hittype, C_Base *control);
void InitTimeCompressionBox(long compression);
void AreYouSure(long TitleID, long MessageID, void (*OkCB)(long, short, C_Base*), void (*CancelCB)(long, short, C_Base*));
void AreYouSure(long TitleID, _TCHAR *text, void (*OkCB)(long, short, C_Base*), void (*CancelCB)(long, short, C_Base*));
BOOL CheckExclude(_TCHAR *filename, _TCHAR *directory, _TCHAR *ExcludeList[], _TCHAR *extension);
void VerifyDelete(long TitleID, void (*YesCB)(long, short, C_Base*), void (*NoCB)(long, short, C_Base*));
UI_Refresher *FindMissionItem(Flight flight);
void MakeTacEngScoreList();
void UpdateRemoteCompression();
static void CloseCampaignWindowCB(long ID, short hittype, C_Base *control);
int tactical_is_training(void);
void UpdateVCs();
// MN
void StartCampaignCB(long ID, short hittype, C_Base *control);
void SendChatStringCB(long ID, short hittype, C_Base *control);

extern bool g_bMPStartRestricted; // JB 0203111 Restrict takeoff/ramp options.
extern bool g_LargeTheater; // 2003-03-15 MN JSTARS map fix for 128x128 theaters

_TCHAR *CampExcludeList[] =
{
    "Save0",
    "Save1",
    "Save2",
    "Instant",
    NULL,
};

void OpenHistoryWindowCB(long, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    LoadTroopMovementHistory();
    gMainHandler->EnableWindowGroup(control->GetGroup());
}

void OpenForceLevelsWindowCB(long, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    LoadForceLevelHistory();
    gMainHandler->EnableWindowGroup(control->GetGroup());
}

void ActivateCampMissionSchedule()
{
    C_Window *win;
    C_Button *btn;

    win = gMainHandler->FindWindow(CB_MAIN_SCREEN);

    if (win)
    {
        btn = (C_Button*)win->FindControl(MISSION_MAIN_CTRL);

        if (btn)
            OpenCampaignCB(btn->GetID(), C_TYPE_LMOUSEUP, btn);
    }
}

void OpenOOBWindowCB(long, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    SetCursor(gCursors[CRSR_WAIT]);
    gMainHandler->EnableWindowGroup(control->GetGroup());
    gGps->SetAllowed(gGps->GetAllowed() bitor UR_OOB);
    long curtime = GetCurrentTime();
    gGps->Update();
    MonoPrint("GPS: OOB Allow time = %1ld\n", GetCurrentTime() - curtime);
    SetCursor(gCursors[CRSR_F16]);
}
void OpenSquadronWindowCB(long, short hittype, C_Base *)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    SetupSquadronInfoWindow(FalconLocalSession->GetPlayerSquadronID());
}

void OpenSierraHotelCB(long, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    UpdateSierraHotel();
    gMainHandler->EnableWindowGroup(control->GetGroup());
}

void BuildCampBrief(C_Window *win);
void BuildCampDebrief(C_Window *win);
void BuildCampBrief(_TCHAR *str);
void BuildCampDebrief(_TCHAR *str);
void DeleteGroupList(long ID);
void UpdateMissionWindow(long ID);
void CreateATOList(long ID);
void PickCampaignPlaneCB(long ID, short hittype, C_Base *control);
int CompressCampaignUntilTakeoff(Flight flight);
void CancelCampaignCompression(void);
extern void SetTimeCompression(int newComp);
int SendStringToPrinter(_TCHAR *str, _TCHAR *title);

BOOL ReadyToPlayMovie = FALSE;
BOOL MovieQInUse = FALSE;

_TCHAR *OrdinalString(long value);

C_Map *gMapMgr = NULL;

extern CampaignTime gOldCompressTillTime;
extern CampaignTime gOldCompressionRatio;

extern int targetCompressionRatio;
extern CampaignTime gCompressTillTime;

enum
{
    SND_SCREAM        = 500005,
    SND_BAD1          = 500006,
    SND_SECOND        = 500007,
    SND_FIRST         = 500008,
    SND_NICE          = 500009,
    SND_BAD2          = 500010,
    SND_YOUSUCK       = 500011,
    SND_TAKEOFF   = 500023,
    SND_CAMPAIGN   = 500024,
    SND_LIBYA   = 500025,
    SND_AMBIENT   = 500033,
    CSM1   = 400134,
};

long CampEventSoundID;

enum
{
    MOVIE_Q_SIZE = 15,
};

long MovieQTime[MOVIE_Q_SIZE];
long MovieQ[MOVIE_Q_SIZE];
_TCHAR MovieQDesc[MOVIE_Q_SIZE][80];
long MovieCount = 0;
short MovieY = 5;
static BOOL NewEvents = FALSE;

void ViewTimerCB(long, short, C_Base *control)
{
    if (control->GetFlags() bitand C_BIT_ABSOLUTE)
    {
        control->Parent_->RefreshWindow();
    }
    else
        control->Parent_->RefreshClient(control->GetClient());
}

void ViewTimerAnimCB(long, short, C_Base *control)
{
    if (control->GetUserNumber(_UI95_TIMER_COUNTER_) < 1)
    {
        if (control->GetFlags() bitand C_BIT_ABSOLUTE)
        {
            control->Parent_->RefreshWindow();
        }
        else
            control->Parent_->RefreshClient(control->GetClient());

        control->SetUserNumber(_UI95_TIMER_COUNTER_, control->GetUserNumber(_UI95_TIMER_DELAY_));
    }

    control->SetUserNumber(_UI95_TIMER_COUNTER_, control->GetUserNumber(_UI95_TIMER_COUNTER_) - 1);
}

void UI_UpdateEventList()
{
    if (gMainHandler)
        NewEvents = TRUE;
}

void StartMovieQ()
{
    if (gMainHandler and ReadyToPlayMovie and not MovieQInUse)
        if (MovieCount > 0)
            PostMessage(gMainHandler->GetAppWnd(), FM_PLAY_UI_MOVIE, 0, 0);
}

void StartAMovieCB(long, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    PostMessage(gMainHandler->GetAppWnd(), FM_REPLAY_UI_MOVIE, 0, control->GetUserNumber(0));
}

void InitNewFlash()
{
    C_Window *win;
    C_Button *btn;

    DeleteGroupList(NEWS_FLASH_WIN);
    win = gMainHandler->FindWindow(CP_SUA);

    if (win)
    {
        btn = (C_Button*)win->FindControl(NEWS_FLASH);

        if (btn)
        {
            btn->Refresh();
            btn->SetFlagBitOn(C_BIT_INVISIBLE);
        }
    }
}

void AddToNewsWindow(long timestamp, _TCHAR *desc, long MovieID)
{
    _TCHAR buffer[100];
    C_Window *win;
    C_Button *btn;

    if ( not gMainHandler)
        return;

    win = gMainHandler->FindWindow(NEWS_FLASH_WIN);

    if (win)
    {
        _stprintf(buffer, "%02ld:%02ld  %s", (timestamp / (60 * 60 * 1000)) % 24, (timestamp / (60 * 1000)) % 60, desc);
        btn = new C_Button;
        btn->Setup(C_DONT_CARE, C_TYPE_NORMAL, 5, MovieY);
        btn->SetText(C_STATE_0, buffer);
        btn->SetText(C_STATE_1, buffer);
        btn->SetColor(C_STATE_0, 0xeeeeee);
        btn->SetColor(C_STATE_1, 0x00ff00);
        btn->SetFont(win->Font_);
        btn->SetFlagBitOn(C_BIT_USELINE);
        btn->SetClient(1);
        btn->SetUserNumber(0, MovieID);
        btn->SetCallback(StartAMovieCB);
        btn->SetUserNumber(_UI95_DELGROUP_SLOT_, _UI95_DELGROUP_ID_);

        win->AddControl(btn);
        win->ScanClientArea(1);
        win->RefreshClient(1);
        MovieY = static_cast<short>(MovieY + btn->GetH() + 2);
    }

    win = gMainHandler->FindWindow(CP_SUA);

    if (win)
    {
        btn = (C_Button*)win->FindControl(NEWS_FLASH);

        if (btn)
        {
            btn->SetFlagBitOff(C_BIT_INVISIBLE);
            btn->Refresh();
        }
    }
}

void PlayUIMovieQ()
{
    int i;

    if (gMainHandler and ReadyToPlayMovie and not MovieQInUse)
    {
        MovieQInUse = 1;

        while (MovieCount > 0)
        {
            TheCampaign.Suspend();
            PlayUIMovie(MovieQ[0]);
            TheCampaign.Resume();
            AddToNewsWindow(MovieQTime[0], MovieQDesc[0], MovieQ[0]);

            for (i = 1; i < MovieCount; i++)
            {
                MovieQTime[i - 1] = MovieQTime[i];
                MovieQ[i - 1] = MovieQ[i];
                memcpy(MovieQDesc[i - 1], MovieQDesc[i], 80 * sizeof(_TCHAR));
            }

            MovieQTime[i - 1] = 0;
            MovieQ[i - 1] = 0;
            memset(MovieQDesc[i - 1], 0, 80 * sizeof(_TCHAR));
            MovieCount--;
        }

        // KCK: When we're done playing the last movie, exit the campaign
        if (TheCampaign.EndgameResult)
            PostMessage(FalconDisplay.appWin, FM_CAMPAIGN_OVER, TheCampaign.EndgameResult, 0);

        MovieQInUse = 0;
    }
}

void ReplayUIMovie(long MovieID)
{
    if (gMainHandler and ReadyToPlayMovie and not MovieQInUse)
    {
        MovieQInUse = 1;
        TheCampaign.Suspend();
        PlayUIMovie(MovieID);
        TheCampaign.Resume();
        MovieQInUse = 0;
    }
}

// KCK: This timer will close the Attack Warning Window after a few minutes
void CloseAWWWindowTimer(void)
{
    gAWWTimeout -= UI_TIMER_INTERVAL;
    Flight interceptors = (Flight) vuDatabase->Find(gInterceptersId);

    // KCK: Close the window on timeout, interceptor death, or interceptor takeoff
    if (gAWWTimeout < 0 or not interceptors) // or interceptors->Moving())
    {
        Cancel_Scramble_CB(0, C_TYPE_LMOUSEUP, NULL);
    }
}

void UI_HandleAirbaseDestroyed()
{
    C_Window *win;

    SetTimeCompression(1);
    UpdateRemoteCompression();
    win = gMainHandler->FindWindow(CP_COUNTDOWN_WIN);

    if (win)
        gMainHandler->HideWindow(win);
}

void UI_HandleFlightCancel()
{
    C_Window *win;

    SetTimeCompression(1);
    UpdateRemoteCompression();
    win = gMainHandler->FindWindow(CP_COUNTDOWN_WIN);

    if (win)
        gMainHandler->HideWindow(win);

    AreYouSure(TXT_FLIGHT_CANCELED, TXT_YOUR_FLIGHT_CANCELED, CloseWindowCB, CloseWindowCB);
}

void UI_HandleFlightScrub()
{
    C_Window *win;

    SetTimeCompression(1);
    UpdateRemoteCompression();
    win = gMainHandler->FindWindow(CP_COUNTDOWN_WIN);

    if (win)
        gMainHandler->HideWindow(win);

    AreYouSure(TXT_FLIGHT_CANCELED, TXT_THIS_FLIGHT_SCRUBBED, CloseWindowCB, CloseWindowCB);
}

void UI_HandleAircraftDestroyed()
{
    C_Window *win;

    SetTimeCompression(1);
    UpdateRemoteCompression();
    win = gMainHandler->FindWindow(CP_COUNTDOWN_WIN);

    if (win)
        gMainHandler->HideWindow(win);

    AreYouSure(TXT_AC_DESTROYED, TXT_YOUR_AC_DESTROYED, CloseWindowCB, CloseWindowCB);
}

// All this really does is notify the player of an impending attack.
// Scramble_Intercept_CB() is the accept callback
// Cancel_Scramble_CB() is the cancel callback
void UIScramblePlayerFlight(void)
{
    C_Window *win;

    if ( not gMainHandler)
        return;

    gSoundMgr->PlaySound(500017); // Airraid sound?
    CampEnterCriticalSection();

    if (gCompressTillTime)
    {
        gOldCompressTillTime = gCompressTillTime;
        gOldCompressionRatio = 0xffffffff;//-1;
        gCompressTillTime = 0;
    }
    else
    {
        gOldCompressTillTime = 0;
        gOldCompressionRatio = targetCompressionRatio;
    }

    SetTimeCompression(1);
    UpdateRemoteCompression();
    CampLeaveCriticalSection();

    win = gMainHandler->FindWindow(SCRAMBLE_WIN);

    if (win)
    {
        gMainHandler->ShowWindow(win);
        gMainHandler->WindowToFront(win);
        gSoundMgr->PlaySound(CSM1);
        gAWWTimeout = 60000; // 60 second timeout period
        gMainHandler->EnterCritical();
        gMainHandler->AddUserCallback(CloseAWWWindowTimer);
        gMainHandler->LeaveCritical();
    }
}

// This just closes the SCRAMBLE_WIN and resets compression to what we had previously
// PETER TODO: The close window 'X' should call this as well but currently doesn't.
void Cancel_Scramble_CB(long, short hittype, C_Base *)
{
    C_Window *win;

    if (hittype not_eq C_TYPE_LMOUSEUP or gOldCompressionRatio < 0)
        return;

    win = gMainHandler->FindWindow(SCRAMBLE_WIN);

    if (win)
        gMainHandler->HideWindow(win);

    gMainHandler->EnterCritical();
    gMainHandler->RemoveUserCallback(CloseAWWWindowTimer);
    gMainHandler->LeaveCritical();

    if (gOldCompressTillTime)
    {
        gCompressTillTime = gOldCompressTillTime;
        gOldCompressTillTime = 0;
    }
    else
    {
        SetTimeCompression(gOldCompressionRatio);
        UpdateRemoteCompression();
        gOldCompressionRatio = 0xffffffff;//-1;
    }
}

// KCK: This should attempt to pick one of the interceptor aircraft in addition to
// closing the box and resetting the previous compression ratio (done in Cancel_Scramble)
void Scramble_Intercept_CB(long ID, short hittype, C_Base *control)
{
    C_Window *win;
    Flight flight = (Flight)vuDatabase->Find(gInterceptersId);

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    // Try to find the correct list item
    if (flight)
    {
        UI_Refresher *urec;

        urec = FindMissionItem(flight);

        if (urec)
        {
            // Update this item
            // if(urec->Mission_)
            // urec->RemoveMission();
            // urec->AddMission(flight);
            //... renumber... or set final
            RequestACSlot(flight, 0, static_cast<uchar>(flight->GetAdjustedAircraftSlot(0)), 0, 0, 1);
            gCurrentFlightID = gInterceptersId;
            UpdateMissionWindow(CB_MISSION_SCREEN);
            // Close the waiting screen, if we're there.
            win = gMainHandler->FindWindow(CP_COUNTDOWN_WIN);

            if (win)
                gMainHandler->HideWindow(win);

            gOldCompressionRatio = 1;
        }
    }

    Cancel_Scramble_CB(ID, hittype, control);
}

void UI_AddMovieToList(long ID, long timestamp, _TCHAR *Description)
{
    if (MovieCount < MOVIE_Q_SIZE)
    {
        MovieQTime[MovieCount] = timestamp;
        MovieQ[MovieCount] = ID;
        _tcsnccpy(MovieQDesc[MovieCount], Description, 79);
        MovieQDesc[MovieCount][79];
        MovieCount++;
    }

    if (gMainHandler and ReadyToPlayMovie)
        StartMovieQ();
}

void UI_UpdateOccupationMap()
{
    if (gOccupationMap and gMainHandler)
    {
        C_Window *win;
        C_Bitmap *bmp;

        MakeOccupationMap(gOccupationMap);

        win = gMainHandler->FindWindow(CP_SUA);

        if (win)
        {
            bmp = (C_Bitmap*)win->FindControl(CP_SUA);

            if (bmp)
                bmp->Refresh();
        }

        win = gMainHandler->FindWindow(TAC_MISSION_SUA);

        if (win)
        {
            bmp = (C_Bitmap*)win->FindControl(TAC_OVERLAY);

            if (bmp)
                bmp->Refresh();
        }
    }
}

void AircraftLaunch(Flight f)
{
    // TJL 10/26/03 Added Config variable to turn off this sound
    if (f and gMainHandler and FalconLocalSession->GetPlayerSquadronID() and g_bTakeoffSound)
    {
        if (f->GetUnitSquadronID() == FalconLocalSession->GetPlayerSquadronID())
            CampEventSoundID = SND_TAKEOFF;
    }
}

void PlayAmbientSound()
{
}

void SetupMover(C_Window *win, long MoverMenu, void (*callback)(long ID, short hittype, C_Base *control))
{
    C_MapMover *mover;

    mover = (C_MapMover *)win->FindControl(MAP_MOVER);

    if (mover)
    {
        mover->SetXYWH(win->ClientArea_[mover->GetClient()].left, win->ClientArea_[mover->GetClient()].top, win->ClientArea_[mover->GetClient()].right - win->ClientArea_[mover->GetClient()].left, win->ClientArea_[mover->GetClient()].bottom - win->ClientArea_[mover->GetClient()].top);
        mover->SetDrawCallback(MapMgrDrawCB);
        mover->SetFlagBitOn(C_BIT_ABSOLUTE);
        mover->SetCallback(callback);
        mover->SetMenu(MoverMenu);
    }
    else
    {
        mover = new C_MapMover;
        mover->Setup(MAP_MOVER, 0);
        mover->SetXYWH(win->ClientArea_[mover->GetClient()].left, win->ClientArea_[mover->GetClient()].top, win->ClientArea_[mover->GetClient()].right - win->ClientArea_[mover->GetClient()].left, win->ClientArea_[mover->GetClient()].bottom - win->ClientArea_[mover->GetClient()].top);
        mover->SetDrawCallback(MapMgrDrawCB);
        mover->SetFlagBitOn(C_BIT_ABSOLUTE);
        mover->SetCallback(callback);
        mover->SetMenu(MoverMenu);
        win->AddControl(mover);
    }
}

void SetupMapMgr(bool noawacsmap)
{
    long i, j, idx;
    C_Window *win;

    if ( not gMapMgr)
    {
        gMapMgr = new C_Map;
        gMapMgr->SetMapCenter(1536 / 2, 2048 / 2);

        // 2002-01-30 MN special AWACS map background, e.g. black with country outlines
        // 2002-03-06 MN don't display Awacsmap in TE edit mode
        if (g_bAWACSSupport and g_bAWACSBackground and not noawacsmap)
        {
            if (gImageMgr->GetImage(BIG_AWACS_MAP_ID))
                gMapMgr->SetMapImage(BIG_AWACS_MAP_ID);
            else
                gMapMgr->SetMapImage(BIG_MAP_ID);
        }
        else
            gMapMgr->SetMapImage(BIG_MAP_ID);

        gMapMgr->SetZoomLevel(1);
        gMapMgr->SetTeamFlags(0, _MAP_OBJECTIVES_ bitor _MAP_UNITS_);
        gMapMgr->SetLogRanges(0.0f, 0.0f, 740.0f, 70.0f);
        gMapMgr->SetStrtRanges(0.0f, 0.0f, 740.0f, 1000.0f);

        // Set Icon Image IDs (Really the C_Resmgr ID)
        for (i = 0; i < NUM_TEAMS; i++)
        {
            if (TeamInfo[i] and (TeamInfo[i]->flags bitand TEAM_ACTIVE))
                idx = TeamInfo[i]->GetColor();
            else
                idx = 0;

            for (j = 0; j < 8; j++)
            {
                gMapMgr->SetAirIcons(i, j, TeamFlightColorIconIDs[idx][j][0], TeamFlightColorIconIDs[idx][j][1]);
            }

            gMapMgr->SetArmyIcons(i, TeamColorIconIDs[idx][0], TeamColorIconIDs[idx][1]);
            gMapMgr->SetNavyIcons(i, TeamColorIconIDs[idx][0], TeamColorIconIDs[idx][1]);
            gMapMgr->SetObjectiveIcons(i, TeamColorIconIDs[idx][0], TeamColorIconIDs[idx][1]);
        }
    }

    if (TheCampaign.Flags bitand CAMP_TACTICAL_EDIT)
        win = gMainHandler->FindWindow(MB_CSECT_WIN);
    else
        win = gMainHandler->FindWindow(CSECT_WIN);

    gMapMgr->SetWPZWindow(win);
}

void SetupGPS(C_TreeList *MissionTree)
{
    if ( not gGps)
    {
        gGps = new GlobalPositioningSystem;
        gGps->Setup();
    }

    gGps->SetMap(gMapMgr);
    gGps->SetMissionTree(MissionTree);
    gGps->SetATOTree(gATOAll);
    gGps->SetOOBTree(gOOBTree);
    gGps->SetAllowed(UR_MISSION bitor UR_MAP);
    gGps->SetObjectiveMenu(OBJECTIVE_POP);
    gGps->SetUnitMenu(UNIT_POP);
    gGps->SetMissionMenu(0); // Don't know what these are yet
    gGps->SetSquadronMenu(UNIT_POP); // Don't know what these are yet
}

void CampBriefPrintCB(long, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    _TCHAR string[8192];
    BuildCampBrief(string);
    SendStringToPrinter(string, "Briefing");
}

void CampDeBriefPrintCB(long, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    _TCHAR string[8192];
    BuildCampDebrief(string);
    SendStringToPrinter(string, "Debriefing");
}

static void CampSaveFileCB(long, short hittype, C_Base *control)
{
    C_Window *win;
    C_EditBox *edit_box;
    long saveIP, saveIter;

    char buffer[MAX_PATH];

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    win = gMainHandler->FindWindow(SAVE_WIN);

    if ( not win)
        return;

    gMainHandler->HideWindow(control->Parent_); // verify window...
    gMainHandler->HideWindow(win);

    edit_box = (C_EditBox*) win->FindControl(FILE_NAME);

    if (edit_box)
    {
        _tcscpy(buffer, edit_box->GetText());
    }

    // Save As Campaign file
    if (FalconLocalGame->IsLocal())
    {
        gCommsMgr->SaveStats();
        TheCampaign.SetCreationIter(TheCampaign.GetCreationIter() + 1);
        TheCampaign.SaveCampaign(game_Campaign, buffer , 0);

        if (gCommsMgr->Online())
        {
            // Send messages to remote players with new Iter Number
            // So they can save their stats bitand update Iter in their campaign
            gCommsMgr->UpdateGameIter();
        }
    }
    else
    {
        // This person is making a LOCAL copy of someone elses game...
        // we can save His stats... but remote people will be SOL if he
        // tries to make this a remote game for them
        saveIP = TheCampaign.GetCreatorIP();
        saveIter = TheCampaign.GetCreationIter();
        TheCampaign.SetCreatorIP(FalconLocalSessionId.creator_);
        TheCampaign.SetCreationIter(1);

        gCommsMgr->SaveStats();
        TheCampaign.SaveCampaign(game_Campaign, buffer , 0);
        TheCampaign.SetCreatorIP(saveIP);
        TheCampaign.SetCreationIter(saveIter);
    }
}

static void CampVerifySaveFileCB(long ID, short hittype, C_Base *control)
{
    C_EditBox *edit_box;
    FILE *fp;
    char buffer[MAX_PATH];

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    edit_box = (C_EditBox*) control->Parent_->FindControl(FILE_NAME);

    if (edit_box)
    {
        //dpc EmptyFilenameSaveFix
        if (g_bEmptyFilenameFix)
        {
            if (_tcslen(edit_box->GetText()) == 0)
            {
                AreYouSure(TXT_WARNING, TXT_ENTER_FILENAME, CloseWindowCB, CloseWindowCB);
                return;
            }
        }

        //end EmptyFilenameSaveFix
        _stprintf(buffer, "%s\\%s.cam", FalconCampUserSaveDirectory, edit_box->GetText());
        fp = fopen(buffer, "r");

        if (fp)
        {
            fclose(fp);

            if (CheckExclude(buffer, FalconCampUserSaveDirectory, CampExcludeList, "cam"))
                AreYouSure(TXT_ERROR, TXT_CANT_OVERWRITE, CloseWindowCB, CloseWindowCB);
            else
                AreYouSure(TXT_SAVE_CAMPAIGN, TXT_FILE_EXISTS, CampSaveFileCB, CloseWindowCB);
        }
        else
        {
            if (CheckExclude(buffer, FalconCampUserSaveDirectory, CampExcludeList, "cam"))
                AreYouSure(TXT_ERROR, TXT_CANT_OVERWRITE, CloseWindowCB, CloseWindowCB);
            else
                CampSaveFileCB(ID, hittype, control);
        }
    }
}

void CampSaveAsCB(long, short hittype, C_Base *)
{
    C_Window *win;
    C_EditBox *ebox;
    _TCHAR buffer[MAX_PATH];

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    sprintf(buffer, "%s\\*.cam", FalconCampUserSaveDirectory);

    SetDeleteCallback(DelCamFileCB);
    SaveAFile(TXT_SAVE_CAMPAIGN, buffer, CampExcludeList, CampVerifySaveFileCB, CloseWindowCB, "");
    win = gMainHandler->FindWindow(SAVE_WIN);

    if (win)
    {
        ebox = (C_EditBox *)win->FindControl(FILE_NAME);

        if (ebox)
        {
            _stprintf(buffer, "%s-%s %1ld %02d %02d %02d", gStringMgr->GetString(TXT_SAVE), gStringMgr->GetString(TXT_DAY), (long)(TheCampaign.CurrentTime / (24 * 60 * 60 * VU_TICS_PER_SECOND)) + 1, (long)(TheCampaign.CurrentTime / (60 * 60 * VU_TICS_PER_SECOND)) % 24, (long)(TheCampaign.CurrentTime / (60 * VU_TICS_PER_SECOND)) % 60, (long)(TheCampaign.CurrentTime / (VU_TICS_PER_SECOND)) % 60);
            ebox->SetText(buffer);
            ebox->Refresh();
        }
    }
}


//
// DO NOT USE THIS FUNCTION FOR TACENG
//
void CampaignSetup() // Everything that needs to be done to start the campaign (except loading the window)
{
    C_Window *win;
    C_Text *txt;
    C_TreeList *tree;
    C_Bitmap *bmp;
    C_Button *btn;
    C_Blip *blip;
    _TCHAR buffer[200];
    Squadron ps;
    GridIndex x, y;

    InCleanup = 0;

    MovieY = 5;
    gMusic->ClearQ();
    gMusic->FadeOut_Stop();
    gMusic->ToggleStream();

    // 2002-03-06 MN added parameter
    SetupMapMgr(false);
    SetupOOBWindow();
    InitPAKNames();
    TallyPlayerSquadrons();
    SetupCampaignMenus();
    StupidHackToCloseCSECT = 0;
    StopLookingforMission = 0;
    CurMapTool = NULL;

    win = gMainHandler->FindWindow(CP_PUA_MAP);

    if (win)
    {
        SetupMover(win, MAP_POP, MapMgrMoveCB);
        gMapMgr->SetWindow(win);
        gMapMgr->SetupOverlay();
    }

    win = gMainHandler->FindWindow(CB_FULLMAP_WIN);

    if (win)
        SetupMover(win, MAP_POP, MapMgrMoveCB);

    win = gMainHandler->FindWindow(CB_MISSION_SCREEN);

    if (win)
    {
        tree = (C_TreeList*)win->FindControl(MISSION_LIST_TREE);
        SetupGPS(tree);
    }

    win = gMainHandler->FindWindow(CB_MISSION_SCREEN);

    if (win)
    {
        txt = (C_Text *)win->FindControl(CB_MISS_TITLE);

        if (txt)
        {
            ps = FalconLocalSession->GetPlayerSquadron();

            if (ps)
            {
                _stprintf(buffer, "%s %s %s", OrdinalString(ps->GetUnitNameID()), gStringMgr->GetString(TXT_FS), gStringMgr->GetString(TXT_FRAG_ORDER));
                txt->SetText(buffer);
            }
        }
    }

    win = gMainHandler->FindWindow(CB_MAIN_SCREEN);

    if (win)
    {
        FalconLocalSession->SetPlayerFlight(NULL);
        btn = (C_Button*)win->FindControl(CO_MAIN_CTRL);

        if (btn)
        {
            if ( not gCommsMgr->Online())
                btn->SetFlagBitOff(C_BIT_ENABLED);
            else
                btn->SetFlagBitOn(C_BIT_ENABLED);
        }
    }

    win = gMainHandler->FindWindow(CP_SUA);

    if (win)
    {
        bmp = (C_Bitmap*)win->FindControl(CP_SUA);

        if (bmp)
            bmp->SetImage(gOccupationMap);
    }

    InitNewFlash();
    SetSingle_Comms_Ctrls();
    gSelectedFlightID = FalconNullId;
    gLoadoutFlightID = FalconNullId;
    gActiveFlightID = FalconNullId;
    gActiveWPNum = 0;
    gCurrentFlightID = FalconNullId;
    gPlayerFlightID = FalconNullId;
    gPlayerPlane = -1;
    mcnt = 0;
    atocnt = 0;
    uintcnt = 0;
    gLastUpdateGround = 0l;
    gLastUpdateAir = 0l;

    if (gATOAll)
        gATOAll->SetMenu(0);

    if (gATOPackage)
        gATOPackage->SetMenu(0);

    gGps->SetTeamNo(FalconLocalSession->GetTeam()); // See ONLY what is spotted
    gGps->Update();

    win = gMainHandler->FindWindow(CP_SUA);

    if (win)
    {
        short i, j;

        blip = (C_Blip*)win->FindControl(9000000);

        if (blip)
        {
            for (i = 0; i < 8; i++)
            {
                if (TeamInfo[i] and TeamInfo[i]->flags bitand TEAM_ACTIVE)
                {
                    for (j = 0; j < 8; j++)
                        blip->SetImage(BLIP_IDS[TeamInfo[i]->GetColor()][j], static_cast<uchar>(i), static_cast<uchar>(j));
                }
                else
                {
                    for (j = 0; j < 8; j++)
                        blip->SetImage(BLIP_IDS[0][j], static_cast<uchar>(i), static_cast<uchar>(j));
                }
            }
        }
    }

    RefreshEventList();
    RefreshMapEventList(CP_SUA, 0);

    // Now finally startup up the Campaign loop
    // 2002-01-03 M.N.
    // If we have a local game, so we are host in a multiplayer environment and start a new campaign,
    // stop the time and let the camp priorities window pop up (in CAMPUI/CampJoin.cpp)

    if (FalconLocalGame->IsLocal() and 
        (strcmp(gUI_CampaignFile, "save0") == 0 or strcmp(gUI_CampaignFile, "save1") == 0 or strcmp(gUI_CampaignFile, "save2") == 0) and 
        campaignStart) // fixes clock being set to "STOP" after a campaign mission
    {
        SetTimeCompression(0);
        InitTimeCompressionBox(0);
    }
    else
    {
        SetTimeCompression(1);
        // UpdateRemoteCompression(); done by InitTimeCompressionBox
        InitTimeCompressionBox(1);
    }

    if (CampaignLastGroup)
    {
        win = gMainHandler->FindWindow(DEBRIEF_WIN);

        // KCK: Added the check for a pilot list so that we don't debrief after a
        // discarded mission
        if (win and TheCampaign.MissionEvaluator and TheCampaign.MissionEvaluator->flight_data)
        {
            BuildCampDebrief(win);
            gMainHandler->EnableWindowGroup(win->GetGroup());
            // JPO - attempt to add handlers for these
            C_Button *ctrl = (C_Button*)win->FindControl(BRIEF_PRINT);

            if (ctrl)
                ctrl->SetCallback(CampDeBriefPrintCB);
        }
    }

    gMainHandler->AddUserCallback(CampaignListCB);
    gMainHandler->AddUserCallback(CampaignSoundEventCB);

    // Choose our next mission (default)
    if ( not gTimeModeServer and not g_bServer)
    {
        FindMissionInBriefing(CB_MISSION_SCREEN);
    }

    UpdateMissionWindow(CB_MISSION_SCREEN);

    CheckCampaignFlyButton();

    TheCampaign.GetBullseyeLocation(&x, &y);
    gMapMgr->SetBullsEye(x * FEET_PER_KM, (TheCampaign.TheaterSizeY - y) * FEET_PER_KM);
    SetMapSettings();
    ReadyToPlayMovie = TRUE;
    PlayCampaignMusic();
    StartMovieQ();
}

//
// DO NOT USE THIS FUNCTION FOR CAMPAIGN
//
void TacticalEngagementSetup(bool noawacsmap) // Everything that needs to be done to start the campaign (except loading the window)
{
    C_Window *win;
    C_TreeList *tree;
    C_Bitmap *bmp;
    C_Button *btn;
    C_EditBox *ebox;
    C_ListBox *lbox;
    C_Clock *clk;
    GridIndex x, y;

    InCleanup = 0;

    gMusic->ClearQ();
    gMusic->FadeOut_Stop();
    gMusic->ToggleStream();
    StupidHackToCloseCSECT = 0;
    StopLookingforMission = 0;
    CurMapTool = NULL;

    ClearMapToolStates(TAC_FULLMAP_WIN);
    ClearMapToolStates(TAC_EDIT_WIN);
    ClearMapToolStates(TAC_PUA_MAP);

    // 2002-03-06 MN if noawacsmap = true, don't display it (which means we're in TE edit mode)
    SetupMapMgr(noawacsmap);
    SetupOOBWindow();
    TallyPlayerSquadrons();

    // Mode 0=Play,1=Edit)
    if (TheCampaign.Flags bitand CAMP_TACTICAL_EDIT)
    {
        SetupTacEngMenus(1);
        win = gMainHandler->FindWindow(TAC_VC_WIN);

        if (win)
        {
            ebox = (C_EditBox*)win->FindControl(PTS_REQ_VICTORY);

            if (ebox)
                ebox->SetFlagBitOn(C_BIT_ENABLED);

            lbox = (C_ListBox*)win->FindControl(TAC_TYPE);

            if (lbox)
                lbox->SetFlagBitOn(C_BIT_ENABLED);

            clk = (C_Clock*)win->FindControl(START_TIME);

            if (clk)
            {
                clk->SetTime(TheCampaign.GetTEStartTime() / VU_TICS_PER_SECOND);
                clk->SetFlagBitOn(C_BIT_ENABLED);
            }

            clk = (C_Clock*)win->FindControl(TIME_LIMIT);

            if (clk)
            {
                clk->SetTime(TheCampaign.GetTETimeLimitTime() / VU_TICS_PER_SECOND);
                clk->SetFlagBitOn(C_BIT_ENABLED);
            }

            if (gVCTree)
                gVCTree->SetFlagBitOn(C_BIT_ENABLED);

            win->UnHideCluster(100);
        }
    }
    else
    {
        SetupTacEngMenus(0);
        win = gMainHandler->FindWindow(TAC_VC_WIN);

        if (win)
        {
            ebox = (C_EditBox*)win->FindControl(PTS_REQ_VICTORY);

            if (ebox)
                ebox->SetFlagBitOff(C_BIT_ENABLED);

            lbox = (C_ListBox*)win->FindControl(TAC_TYPE);

            if (lbox)
                lbox->SetFlagBitOff(C_BIT_ENABLED);

            clk = (C_Clock*)win->FindControl(START_TIME);

            if (clk)
            {
                clk->SetTime(TheCampaign.GetTEStartTime() / VU_TICS_PER_SECOND);
                clk->SetFlagBitOn(C_BIT_ENABLED);
            }

            clk = (C_Clock*)win->FindControl(TIME_LIMIT);

            if (clk)
            {
                clk->SetTime(TheCampaign.GetTETimeLimitTime() / VU_TICS_PER_SECOND);
                clk->SetFlagBitOn(C_BIT_ENABLED);
            }

            if (gVCTree)
                gVCTree->SetFlagBitOff(C_BIT_ENABLED);

            win->HideCluster(100);
        }

        win = gMainHandler->FindWindow(TAC_END_WIN);

        if (win)
        {
            ebox = (C_EditBox*)win->FindControl(PTS_REQ_VICTORY);

            if (ebox)
                ebox->SetFlagBitOff(C_BIT_ENABLED);

            clk = (C_Clock*)win->FindControl(TIME_LIMIT);

            if (clk)
            {
                clk->SetTime(TheCampaign.GetTETimeLimitTime() / VU_TICS_PER_SECOND);
                clk->SetFlagBitOn(C_BIT_ENABLED);
            }
        }
    }

    win = gMainHandler->FindWindow(TAC_EDIT_WIN);

    if (win)
        SetupMover(win, MAP_POP, gMapMgr_TACmover);

    win = gMainHandler->FindWindow(TAC_VC_WIN);

    if (win)
        SetupMover(win, MAP_POP, gMapMgr_TACmover);

    win = gMainHandler->FindWindow(TAC_PUA_MAP);

    if (win)
    {
        SetupMover(win, MAP_POP, gMapMgr_TACmover);
        gMapMgr->SetWindow(win);
        gMapMgr->SetupOverlay();
    }

    win = gMainHandler->FindWindow(TAC_FULLMAP_WIN);

    if (win)
        SetupMover(win, MAP_POP, gMapMgr_TACmover);

    win = gMainHandler->FindWindow(TAC_AIRCRAFT);

    if (win)
    {
        tree = (C_TreeList*)win->FindControl(MISSION_LIST_TREE);
        SetupGPS(tree);
    }

    win = gMainHandler->FindWindow(TAC_EDIT_SCREEN);

    if (win)
    {
        FalconLocalSession->SetPlayerFlight(NULL);
        btn = (C_Button*)win->FindControl(CO_MAIN_CTRL);

        if (btn)
        {
            if ( not gCommsMgr->Online())
                btn->SetFlagBitOff(C_BIT_ENABLED);
            else
                btn->SetFlagBitOn(C_BIT_ENABLED);
        }
    }

    win = gMainHandler->FindWindow(TAC_PLAY_SCREEN);

    if (win)
    {
        FalconLocalSession->SetPlayerFlight(NULL);
        btn = (C_Button*)win->FindControl(CO_MAIN_CTRL);

        if (btn)
        {
            if ( not gCommsMgr->Online())
                btn->SetFlagBitOff(C_BIT_ENABLED);
            else
                btn->SetFlagBitOn(C_BIT_ENABLED);
        }
    }

    win = gMainHandler->FindWindow(TAC_MISSION_SUA);

    if (win)
    {
        bmp = (C_Bitmap*)win->FindControl(TAC_OVERLAY);

        if (bmp)
            bmp->SetImage(gOccupationMap);
    }

    win = gMainHandler->FindWindow(TAC_MISSION_SUA);

    if (win)
    {
        short i, j;
        C_Blip *blip;

        blip = (C_Blip*)win->FindControl(9000000);

        if (blip)
        {
            for (i = 0; i < 8; i++)
            {
                if (TeamInfo[i] and TeamInfo[i]->flags bitand TEAM_ACTIVE)
                {
                    for (j = 0; j < 8; j++)
                        blip->SetImage(BLIP_IDS[TeamInfo[i]->GetColor()][j], static_cast<uchar>(i), static_cast<uchar>(j));
                }
                else
                {
                    for (j = 0; j < 8; j++)
                        blip->SetImage(BLIP_IDS[0][j], static_cast<uchar>(i), static_cast<uchar>(j));
                }
            }
        }
    }

    InitNewFlash();
    RefreshMapEventList(TAC_MISSION_SUA, 1);

    SetSingle_Comms_Ctrls();
    gSelectedFlightID = FalconNullId;
    gLoadoutFlightID = FalconNullId;
    gActiveFlightID = FalconNullId;
    gActiveWPNum = 0;
    gCurrentFlightID = FalconNullId;
    gPlayerFlightID = FalconNullId;
    gPlayerPlane = -1;
    mcnt = 0;
    atocnt = 0;
    uintcnt = 0;
    gLastUpdateGround = 0l;
    gLastUpdateAir = 0l;

    if (TheCampaign.Flags bitand CAMP_TACTICAL_EDIT)
    {
        gMoveBattalion = TRUE;
        gGps->SetTeamNo(-1); // Perfect Intel
    }
    else if (tactical_is_training())
    {
        gMoveBattalion = FALSE;
        gGps->SetTeamNo(-1); // Perfect Intel
        gSelectedTeam = FalconLocalSession->GetTeam();
    }
    else
    {
        gMoveBattalion = FALSE;
        gGps->SetTeamNo(FalconLocalSession->GetTeam()); // See ONLY what is spotted
        gSelectedTeam = FalconLocalSession->GetTeam();
    }

    if (TacLastGroup)
        do_tactical_debrief();

    gGps->Update();
    gMainHandler->AddUserCallback(TacEngListCB);
    gMainHandler->AddUserCallback(CampaignSoundEventCB);

    CheckCampaignFlyButton();

    if ( not gTimeModeServer and not g_bServer)
    {
        FindMissionInBriefing(TAC_AIRCRAFT);
    }

    UpdateMissionWindow(TAC_AIRCRAFT);

    // gATOAll->SetMenu (AIRUNIT_MENU);
    // gATOPackage->SetMenu (AIRUNIT_MENU);

    TheCampaign.GetBullseyeLocation(&x, &y);
    gMapMgr->SetBullsEye(x * FEET_PER_KM, (TheCampaign.TheaterSizeY - y) * FEET_PER_KM);
    SetMapSettings();

    InitVCArgLists();
    add_all_vcs_to_ui();
    gRefreshScoresList = 1;
    PlayCampaignMusic();

    if (ShowGameOverWindow)
        PostMessage(gMainHandler->GetAppWnd(), FM_OPEN_GAME_OVER_WIN, game_TacticalEngagement, 0);
}

void LoadCampaignWindows()
{
    long ID;
    C_Window *win;
    C_TimerHook *tmr;

    if (CPLoaded) return;

    CampEventSoundID = 0;

    if (_LOAD_ART_RESOURCES_)
    {
        gMainParser->LoadImageList("cp_res.lst");
    }
    else
    {
        gMainParser->LoadImageList("cp_art.lst");
    }

    gMainParser->LoadSoundList("cp_snd.lst");
    gMainParser->LoadWindowList("cp_scf.lst");   // Modified by M.N. - add art/art1024 by LoadWindowList

    ID = gMainParser->GetFirstWindowLoaded();

    while (ID)
    {
        HookupCampaignControls(ID);
        ID = gMainParser->GetNextWindowLoaded();
    }

    LoadCommonWindows();

    CPLoaded++;
    win = gMainHandler->FindWindow(CB_MAIN_SCREEN);

    if (win)
    {
        tmr = new C_TimerHook;
        tmr->Setup(C_DONT_CARE, C_TYPE_TIMER);
        tmr->SetUpdateCallback(GenericTimerCB);
        tmr->SetRefreshCallback(BlinkCommsButtonTimerCB);
        tmr->SetUserNumber(_UI95_TIMER_DELAY_, 1 * _UI95_TICKS_PER_SECOND_); // Timer activates every 2 seconds (Only when this window is open)

        win->AddControl(tmr);
    }
}

int mcnt = 0;
int atocnt = 0;
int uintcnt = 0;
void CampaignListCB()
{
    if (uintcnt < 1)
    {
        UpdateIntel(CB_INTEL_WIN);
        uintcnt = 20;
    }

    uintcnt--;

    if (NewEvents)
    {
        NewEvents = FALSE;
        RefreshEventList();
        RefreshMapEventList(CP_SUA, 0);
    }

    if (mcnt < 1)
    {
        if (gGps)
        {
            Flight flt;
            gGps->Update();
            flt = (Flight)vuDatabase->Find(gCurrentFlightID);

            if (flt)
            {
                if (flt->GetTotalVehicles() < 1 or flt->IsDead())
                {
                    if ( not gTimeModeServer and not g_bServer)
                    {
                        FindMissionInBriefing(CB_MISSION_SCREEN);
                    }

                    UpdateMissionWindow(CB_MISSION_SCREEN);
                }
            }
            else
            {
                if ( not gTimeModeServer and not g_bServer)
                {
                    FindMissionInBriefing(CB_MISSION_SCREEN);
                    UpdateMissionWindow(CB_MISSION_SCREEN);
                }
            }
        }

        if (gMapMgr)
            gMapMgr->RemoveOldWaypoints();

        mcnt = 5;
    }

    mcnt--;
}

void TacEngListCB()
{
    if (NewEvents)
    {
        NewEvents = FALSE;
        RefreshMapEventList(TAC_MISSION_SUA, 1);
    }

    if (mcnt < 1)
    {
        if (gGps)
        {
            Flight flt;
            gGps->Update();
            UpdateVCs();
            flt = (Flight)vuDatabase->Find(gCurrentFlightID);

            if (flt)
            {
                if (flt->GetTotalVehicles() < 1 or flt->IsDead())
                {
                    if ( not gTimeModeServer and not g_bServer)
                    {
                        FindMissionInBriefing(TAC_AIRCRAFT);
                    }

                    UpdateMissionWindow(TAC_AIRCRAFT);
                }
            }
            else
            {
                if ( not gTimeModeServer and not g_bServer)
                {
                    FindMissionInBriefing(TAC_AIRCRAFT);
                    UpdateMissionWindow(TAC_AIRCRAFT);
                }
            }
        }

        if (gMapMgr)
            gMapMgr->RemoveOldWaypoints();

        mcnt = 5;
    }

    mcnt--;
}

void CampaignSoundEventCB()
{
    if (CampEventSoundID > 0)
    {
        gSoundMgr->PlaySound(CampEventSoundID);
        CampEventSoundID = 0;
    }
}

static void OpenFlightPlanWindowCB(long, short hittype, C_Base *control)
{
    C_Window *win;
    WayPoint wp;
    Flight flt;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    flt = (Flight)vuDatabase->Find(gActiveFlightID);

    if ( not flt)
        return;

    win = gMainHandler->FindWindow(FLIGHT_PLAN_WIN);

    if (win)
    {
        if ( not (gMainHandler->GetWindowFlags(FLIGHT_PLAN_WIN) bitand C_BIT_ENABLED))
        {
            wp = flt->GetFirstUnitWP();

            if (wp)
            {
                UpdateWaypointWindowInfo(win, wp, 1);
                gMainHandler->EnableWindowGroup(control->GetGroup());
            }
        }
        else
            gMainHandler->WindowToFront(win);
    }
}

// "Fly" button
void CampaignButtonCB(long, short hittype, C_Base *)
{
    Flight fl;
    int pilotSlot, entryType;
    WayPoint wp;
    C_Window *win;
    C_Button *btn;

    // Check for correct mouse click and campaign over state
    if (hittype not_eq C_TYPE_LMOUSEUP or TheCampaign.EndgameResult)
        return;

    if (TheCampaign.EndgameResult)
    {
        AreYouSure(TXT_ERROR, TXT_GAME_OVER, NULL, CloseWindowCB);
        return;
    }

    fl = FalconLocalSession->GetPlayerFlight();
    pilotSlot = FalconLocalSession->GetPilotSlot();

    if ( not fl or pilotSlot == 255)
    {
        // PETER TODO: Clear mission window's selection TOO
        FalconLocalSession->SetPlayerFlight(NULL);
        FalconLocalSession->SetPilotSlot(255);
        return;
    }

    ReadyToPlayMovie = FALSE;

    TheCampaign.MissionEvaluator->PreMissionEval(fl, static_cast<uchar>(pilotSlot));

    // KCK HACK TO ISOLATE KNEEBOARD CRASH BUG
    // if ( not TheCampaign.MissionEvaluator->player_pilot)
    // *((unsigned int *) 0x00) = 0;
    // END HACK

#ifdef DEBUG
    gPlayerPilotLock = 1;
#endif

    ShiAssert(TheCampaign.MissionEvaluator->player_pilot);

    // Trigger the campaign to compress time and takeoff.
    entryType = CompressCampaignUntilTakeoff(fl);

    // 2002-03-09 MN Send a "[Commiting now]" message to the chat windows
    enum { PSEUDO_CONTROL = 565419999 };

    C_EditBox control;
    control.Setup(PSEUDO_CONTROL, 39);
    _TCHAR buffer[21];
    _stprintf(buffer, "( is commiting now )");
    control.SetText(buffer);

    SendChatStringCB(0, DIK_RETURN, &control);
    control.Cleanup();

    wp = fl->GetFirstUnitWP();

    if (wp)
    {
        if (TheCampaign.CurrentTime < wp->GetWPArrivalTime())
        {
            // RV - Biker - Disable ramp and taxi for carrier take off
            Flight fl = FalconLocalSession->GetPlayerFlight();

            CampEntity ent = NULL;

            if (fl)
                ent = fl->GetUnitAirbase();

            if (ent and ent->IsTaskForce())
                PlayerOptions.SetStartFlag(PlayerOptionsClass::START_RUNWAY);

            // Open Countdown window if we're waiting for takeoff
            win = gMainHandler->FindWindow(CP_COUNTDOWN_WIN);

            if (win and entryType < 0)
            {
                gMainHandler->EnterCritical();

                btn = (C_Button*)win->FindControl(WAIT_TAXI);

                if (btn)
                {
                    if (0)//(gCommsMgr) and (gCommsMgr->Online ()))
                    {
                        btn->SetFlagBitOn(C_BIT_INVISIBLE);
                    }
                    else
                    {
                        btn->SetFlagBitOff(C_BIT_INVISIBLE);

                        if (PlayerOptions.GetStartFlag() == PlayerOptionsClass::START_TAXI)
                            btn->SetState(1);
                        else
                            btn->SetState(0);
                    }

                    btn->Refresh();
                }

                btn = (C_Button*)win->FindControl(WAIT_TAKEOFF);

                if (btn)
                {
                    if (g_bMPStartRestricted and gCommsMgr and gCommsMgr->Online())
                    {
                        btn->SetFlagBitOn(C_BIT_INVISIBLE);
                    }
                    else
                    {
                        btn->SetFlagBitOff(C_BIT_INVISIBLE);

                        if (PlayerOptions.GetStartFlag() == PlayerOptionsClass::START_RUNWAY)
                            btn->SetState(1);
                        else
                            btn->SetState(0);
                    }

                    btn->Refresh();
                }

                btn = (C_Button*)win->FindControl(WAIT_RAMP);

                if (btn)
                {
                    if (g_bMPStartRestricted and gCommsMgr and gCommsMgr->Online())
                    {
                        btn->SetFlagBitOn(C_BIT_INVISIBLE);
                    }
                    else
                    {
                        btn->SetFlagBitOff(C_BIT_INVISIBLE);

                        if (PlayerOptions.GetStartFlag() == PlayerOptionsClass::START_RAMP)
                            btn->SetState(1);
                        else
                            btn->SetState(0);
                    }

                    btn->Refresh();
                }

                gMainHandler->ShowWindow(win);
                gMainHandler->WindowToFront(win);
                gMainHandler->LeaveCritical();
            }
        }
    }

    // Autosave the game, if campaign
    CampaignAutoSave(FalconLocalGame->GetGameType());

    gMusic->ClearQ();
    gMusic->FadeOut_Stop();

    // Force Compliance... since they already agreed before
    if (gCommsMgr and gCommsMgr->Online())
    {
        PlayerOptions.ComplyWRules(CurrRules.GetRules());
        PlayerOptions.SaveOptions();
    }
}

static void CloseCampaignWindowCB(long, short hittype, C_Base *)
{
    C_Window *win;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    if (CampaignLastGroup)
    {
        gMainHandler->DisableWindowGroup(CampaignLastGroup);
        win = gMainHandler->FindWindow(CP_TOOLBAR);

        if (win)
            win->HideCluster(CampaignLastGroup);

        CampaignLastGroup = 0;
    }
}

static void GenericCloseCB(long, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    gMainHandler->DisableWindowGroup(control->GetGroup());
}

static void MapMgrDrawCB(long, short, C_Base *)
{
    //gMapMgr->SetFlags(I_NEED_TO_DRAW_MAP);
    //gMapMgr->DrawMap();
}

// CA map callback function for move events
static void MapMgrMoveCB(long, short hittype, C_Base *control)
{
    if ( not control)
    {
        return;
    }

    switch (hittype)
    {
        case C_TYPE_MOUSEMOVE:
            gMapMgr->MoveCenter(-((C_MapMover *)control)->GetHRange(), -((C_MapMover *)control)->GetVRange());
            control->Parent_->RefreshClient(0);
            break;

        case C_TYPE_MOUSEWHEEL:
            C_Control *c = static_cast<C_Control*>(control);

            if (c->GetIncrement() > 0)
            {
                gMapMgr->ZoomOut();
            }
            else
            {
                gMapMgr->ZoomIn();
            }

            break;
    }
}

static void OpenFullMapCB(long, short hittype, C_Base *control)
{
    F4CSECTIONHANDLE *Leave;
    C_Window *win;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    Leave = UI_Enter(control->Parent_);
    win = gMainHandler->FindWindow(CB_FULLMAP_WIN);

    if (win)
    {
        gMapMgr->SetWindow(win);
        gMapMgr->DrawMap();
    }

    win = gMainHandler->FindWindow(CP_PUA_MAP);

    if (win)
        gMainHandler->HideWindow(win);

    gMainHandler->EnableWindowGroup(control->GetGroup());
    UI_Leave(Leave);
}

static void CloseFullMapCB(long, short hittype, C_Base *control)
{
    F4CSECTIONHANDLE *Leave;
    C_Window *win;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    Leave = UI_Enter(control->Parent_);
    gMainHandler->DisableWindowGroup(control->GetGroup());
    gMainHandler->DisableWindowGroup(6401);
    StupidHackToCloseCSECT = 0;
    win = gMainHandler->FindWindow(CP_PUA_MAP);

    if (win)
    {
        gMapMgr->SetWindow(win);
        gMapMgr->DrawMap();
        gMainHandler->ShowWindow(win);
        gMainHandler->WindowToFront(win);
    }

    UI_Leave(Leave);
}

static void OpenCampaignCB(long, short hittype, C_Base *control)
{
    C_Window *win;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    win = gMainHandler->FindWindow(CP_TOOLBAR);

    if (CampaignLastGroup not_eq 0 and CampaignLastGroup not_eq control->GetGroup())
    {
        gMainHandler->DisableWindowGroup(CampaignLastGroup);

        if (win)
            win->HideCluster(CampaignLastGroup);
    }

    if (CampaignLastGroup not_eq control->GetGroup())
    {
        gMainHandler->EnableWindowGroup(control->GetGroup());
        CampaignLastGroup = control->GetGroup();

        if (win)
            win->UnHideCluster(CampaignLastGroup);
    }

    CheckCampaignFlyButton();
}

BOOL CampaignClockCB(C_Base *me)
{
    long curtime;

    if (me == NULL) return(FALSE);

    // curtime=TheCampaign.CurrentTime/VU_TICS_PER_SECOND;
    curtime = vuxGameTime / VU_TICS_PER_SECOND;

    if (((C_Clock*)me)->GetTime() not_eq curtime)
    {
        ((C_Clock*)me)->SetTime(curtime);
        me->Refresh();
        return(TRUE);
    }

    return(FALSE);
}

extern _TCHAR *ObjStr[5];
void CleanupCampaignUI()
{
    C_Window *win;
    C_Blip *blip;
    C_Bitmap *bmp;
    int i;

    InCleanup = 1;

    ReadyToPlayMovie = FALSE;
    gCurrentFlightID = FalconNullId;

    if (gMainHandler)
    {
        gMainHandler->EnterCritical();
        gMainHandler->RemoveUserCallback(CampaignListCB);
        gMainHandler->RemoveUserCallback(CampaignSoundEventCB);

        if (gGps)
        {
            gGps->Cleanup();
            delete gGps;
            gGps = NULL;
        }

        if (gMapMgr)
        {
            gMapMgr->Cleanup();
            delete gMapMgr;
            gMapMgr = NULL;
        }

        if (gATOPackage)
        {
            gATOPackage->DeleteBranch(gATOPackage->GetRoot());
        }

        if (gATOAll)
        {
            gATOAll->DeleteBranch(gATOAll->GetRoot());
        }

        if (gOOBTree)
        {
            gOOBTree->DeleteBranch(gOOBTree->GetRoot());
        }

        /* if(gTaskList)
         {
         while(gTaskList)
         {
         last=gTaskList;
         gTaskList=gTaskList->Next;
         if(last->Label_)
         {
         last->Label_->Cleanup();
         delete last->Label_;
         }
         delete last;
         }
         gTaskList=NULL;
         }
        */
        win = gMainHandler->FindWindow(STRAT_WIN);

        if (win)
        {
            bmp = (C_Bitmap*)win->FindControl(PAK_REGION);

            if (bmp)
                bmp->SetImage((long)NULL);
        }

        if (PAKMap)
        {
            C_Resmgr *mgr;
            mgr = PAKMap->Owner;
            mgr->Cleanup();
            delete mgr;
            PAKMap = NULL;
        }

        for (i = 0; i < 5; i++)
        {
            if (ObjStr[i])
            {
                delete ObjStr[i];
            }

            ObjStr[i] = NULL;
        }

        if (gUIViewer)
        {
            delete gUIViewer;
            gUIViewer = NULL;
        }

        win = gMainHandler->FindWindow(CP_SUA);

        if (win)
        {
            DeleteGroupList(CP_SUA);
            blip = (C_Blip*)win->FindControl(9000000);

            if (blip)
                blip->RemoveAll();
        }

        gMainHandler->LeaveCritical();
    }
}

void CleanupTacticalEngagementUI()
{
    C_Window *win;
    C_Blip *blip;

    InCleanup = 1;

    if (te_restore_map)
    {
        delete te_restore_map;
        te_restore_map = NULL;
    }

    ReadyToPlayMovie = FALSE;
    gCurrentFlightID = FalconNullId;

    if (gMainHandler)
    {
        gMainHandler->EnterCritical();
        gMainHandler->RemoveUserCallback(TacEngListCB);
        gMainHandler->RemoveUserCallback(CampaignSoundEventCB);

        if (gGps)
        {
            gGps->Cleanup();
            delete gGps;
            gGps = NULL;
        }

        if (gMapMgr)
        {
            gMapMgr->Cleanup();
            delete gMapMgr;
            gMapMgr = NULL;
        }

        if (gATOPackage)
        {
            gATOPackage->DeleteBranch(gATOPackage->GetRoot());
        }

        if (gATOAll)
        {
            gATOAll->DeleteBranch(gATOAll->GetRoot());
        }

        if (gOOBTree)
        {
            gOOBTree->DeleteBranch(gOOBTree->GetRoot());
        }

        CleanupVCArgLists();

        if (gVCTree)
            gVCTree->DeleteBranch(gVCTree->GetRoot());

        /* if(gTaskList)
         {
         while(gTaskList)
         {
         last=gTaskList;
         gTaskList=gTaskList->Next;
         if(last->Label_)
         {
         last->Label_->Cleanup();
         delete last->Label_;
         }
         delete last;
         }
         gTaskList=NULL;
         }
        */
        if (gUIViewer)
        {
            delete gUIViewer;
            gUIViewer = NULL;
        }

        win = gMainHandler->FindWindow(TAC_MISSION_SUA);

        if (win)
        {
            DeleteGroupList(TAC_MISSION_SUA);
            blip = (C_Blip*)win->FindControl(9000000);

            if (blip)
                blip->RemoveAll();
        }

        gMainHandler->LeaveCritical();
    }
}

void EndCommitCB(long, short hittype, C_Base *)
{
    C_Window *win;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    CampaignAutoSave(FalconLocalGame->GetGameType());

    gMusic->ClearQ();
    gMusic->FadeOut_Stop();
    gMusic->ToggleStream();
    PlayUIMusic();

    gMainHandler->EnterCritical();

    if (CampaignLastGroup)
    {
        gMainHandler->DisableWindowGroup(CampaignLastGroup);
        win = gMainHandler->FindWindow(CP_TOOLBAR);

        if (win)
            win->HideCluster(CampaignLastGroup);

        CampaignLastGroup = 0;
    }

    gMainHandler->DisableWindowGroup(200);

    gMainHandler->DisableSection(200);

    gMainHandler->SetSection(100);
    gMainHandler->EnableWindowGroup(100);
    gMainHandler->EnableWindowGroup(MainLastGroup);

    gMainHandler->LeaveCritical();
    FalconLocalSession->SetPlayerSquadron(NULL);
    FalconLocalSession->SetPlayerFlight(NULL);

    CleanupCampaignUI();
    ShutdownCampaign();
    SelectScenarioCB(CS_LOAD_SCENARIO1, C_TYPE_LMOUSEUP, NULL);
    gCommsMgr->SetCampaignFlag(game_PlayerPool);
}

static void SmallMapZoomInCB(long, short hittype, C_Base *control)
{
    F4CSECTIONHANDLE *Leave;

    if ((hittype not_eq C_TYPE_LMOUSEUP) and (hittype not_eq C_TYPE_REPEAT))
        return;

    Leave = UI_Enter(control->Parent_);
    gMapMgr->ZoomIn();
    gMapMgr->DrawMap();
    UI_Leave(Leave);
}

static void SmallMapZoomOutCB(long, short hittype, C_Base *control)
{
    F4CSECTIONHANDLE *Leave;

    if ((hittype not_eq C_TYPE_LMOUSEUP) and (hittype not_eq C_TYPE_REPEAT))
        return;

    Leave = UI_Enter(control->Parent_);
    gMapMgr->ZoomOut();
    gMapMgr->DrawMap();
    UI_Leave(Leave);
}

void OpenPlannerWindowCB(long, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    gMainHandler->EnableWindowGroup(control->GetGroup());
}

static void OpenATOWindowCB(long, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    gGps->SetAllowed(gGps->GetAllowed() bitor UR_ATO bitor UR_SQUADRON);
    gGps->Update();

    gMainHandler->EnableWindowGroup(control->GetGroup());
}

static void OpenBriefingWindowCB(long, short hittype, C_Base *control)
{
    C_Window *win;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    if (gSelectedFlightID == FalconNullId)
        return;

    win = gMainHandler->FindWindow(BRIEF_WIN);

    if (win)
    {
        BuildCampBrief(win);
        // JPO - attempt to add handlers for these
        C_Button *ctrl = (C_Button*)win->FindControl(BRIEF_PRINT);

        if (ctrl)
            ctrl->SetCallback(CampBriefPrintCB);

    }

    gMainHandler->EnableWindowGroup(control->GetGroup());
}

void CheckPlayersFlight(FalconSessionEntity *session)
{
    if ( not session)
        return;

    UpdateMissionWindow(CB_MISSION_SCREEN);
    UpdateMissionWindow(TAC_AIRCRAFT);
}

//sfr: this gets called when we pick a plane in the UI
void PickCampaignPlaneCB(long ID, short hittype, C_Base *)
{
    int playerPlane;
    Flight flight;

    if (hittype not_eq C_TYPE_LMOUSEUP)
    {
        return;
    }

    if ((flight = static_cast<Flight>(vuDatabase->Find(gCurrentFlightID))) == NULL)
    {
        return;
    }

    if ((gCommsMgr) and (gCommsMgr->Online()))
    {
        // Don't care about restricting access when online
    }
    else if (GetFlightStatusID(flight) >= _MIS_EGRESS)
    {
        return;
    }

    switch (ID)
    {
        case CB_1_1:
        case CB_2_1:
        case CB_3_1:
        case CB_4_1:
            playerPlane = 0;
            break;

        case CB_2_2:
        case CB_3_2:
        case CB_4_2:
            playerPlane = 1;
            break;

        case CB_3_3:
        case CB_4_3:
            playerPlane = 2;
            break;

        case CB_4_4:
            playerPlane = 3;
            break;

        default:
            return;
            break;
    }

    // playerPlane = flight->GetAdjustedAircraftSlot(playerPlane);
    if ( not gTimeModeServer and not g_bServer)
    {
        RequestACSlot(flight, 0, static_cast<uchar>(playerPlane), 0, 0, 1);
    }
}

void UpdateEventBlipsCB(long, short, C_Base *control)
{
    C_Window *win;
    C_Blip *Blip;
    F4CSECTIONHANDLE *Leave;

    win = control->Parent_;

    if (win)
    {

        Blip = (C_Blip*)win->FindControl(9000000);

        if (Blip)
        {
            Leave = UI_Enter(win);

            if (control->GetUserNumber(_UI95_TIMER_COUNTER_) bitand 1)
                Blip->BlinkLast();

            if (control->GetUserNumber(_UI95_TIMER_COUNTER_) < 1)
            {
                Blip->Update(vuxGameTime / (VU_TICS_PER_SECOND * 60));
                control->SetUserNumber(_UI95_TIMER_COUNTER_, control->GetUserNumber(_UI95_TIMER_DELAY_));
            }

            UI_Leave(Leave);
        }
    }

    control->SetUserNumber(_UI95_TIMER_COUNTER_, control->GetUserNumber(_UI95_TIMER_COUNTER_) - 1);
}

void OpenCampaignCommsCB(long, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    if ( not gCommsMgr->Online())
        gMainHandler->EnableWindowGroup(control->GetUserNumber(1));
    else
    {
        gNewMessage = NULL;
        gMainHandler->EnableWindowGroup(control->GetUserNumber(0));
    }
}

void HistoryPlayForward(long, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    JStarsDirection = 1;
    JStarsDelay = 1 * _UI95_TICKS_PER_SECOND_; // Seconds
    control->Parent_->SetGroupState(-200, 0);
    control->SetState(1);
}

void HistoryPlayReverse(long, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    JStarsDirection = -1;
    JStarsDelay = 1 * _UI95_TICKS_PER_SECOND_; // Seconds
    control->Parent_->SetGroupState(-200, 0);
    control->SetState(1);
}

void HistoryStepForward(long, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    JStarsDirection = 0;

    if (JStarsCurrent < (JStarsLast - 1))
        JStarsCurrent++;

    control->Parent_->SetGroupState(-200, 0);
}

void HistoryStepReverse(long, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    JStarsDirection = 0;

    if (JStarsCurrent > JStarsFirst)
        JStarsCurrent--;

    control->Parent_->SetGroupState(-200, 0);
}

void HistoryFastForward(long, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    JStarsDirection = 1;
    JStarsDelay = static_cast<long>(0.25 * _UI95_TICKS_PER_SECOND_); // Second
    control->Parent_->SetGroupState(-200, 0);
    control->SetState(1);
}

void HistoryFastReverse(long, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    JStarsDirection = -1;
    JStarsDelay = static_cast<long>(0.25 * _UI95_TICKS_PER_SECOND_); // Second
    control->Parent_->SetGroupState(-200, 0);
    control->SetState(1);
}

void HistoryStop(long, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    JStarsDirection = 0;
    control->Parent_->SetGroupState(-200, 0);
}

void HistoryDragBallCB(long, short hittype, C_Base *control)
{
    C_Slider *sldr;
    long Offset, step, Range;

    if (hittype not_eq C_TYPE_MOUSEMOVE)
        return;

    JStarsDirection = 0;

    sldr = (C_Slider*)control;
    Offset = sldr->GetSliderPos() - sldr->GetSliderMin();
    Range = sldr->GetSliderMax() - sldr->GetSliderMin();

    if (Range >= 1)
    {
        step = (Offset * (JStarsLast - 2)) / Range + JStarsFirst;

        if (step < JStarsFirst)
            step = JStarsFirst;

        if (step >= (JStarsLast - 1))
            step = JStarsLast - 1;

        JStarsCurrent = step;
    }
}

void HistoryTimerCB(long, short, C_Base *control)
{
    C_History *hist;
    C_Text *txt;
    C_Slider *sldr;
    long TimeID;
    _TCHAR buffer[15];
    F4CSECTIONHANDLE *Leave;

    if (JStarsCurrent not_eq JStarsPrevious)
    {
        Leave = UI_Enter(control->Parent_);
        control->Parent_->HideGroup(JStarsPrevious);
        JStarsPrevious = JStarsCurrent;
        control->Parent_->UnHideGroup(JStarsPrevious);
        control->Parent_->RefreshClient(control->GetClient());
        txt = (C_Text*)control->Parent_->FindControl(COUNTER);

        if (txt)
        {
            hist = (C_History*)control->Parent_->FindControl(JStarsCurrent);

            if (hist)
            {
                TimeID = hist->GetUserNumber(0);
                _stprintf(buffer, "%s%2ld  %02d:00", gStringMgr->GetString(TXT_DAY), TimeID / 1440l + 1, (TimeID / 60) % 24);
                txt->Refresh();
                txt->SetText(buffer);
                txt->Refresh();
            }
        }

        sldr = (C_Slider*)control->Parent_->FindControl(PLAYBALL);

        if (sldr)
            PositionSlider(sldr, JStarsCurrent, JStarsFirst, JStarsLast - 2);

        UI_Leave(Leave);
    }

    if ( not JStarsDirection)
        return;

    if (control->GetUserNumber(_UI95_TIMER_COUNTER_) < 1)
    {
        JStarsCurrent += JStarsDirection;

        if (JStarsCurrent >= JStarsLast)
        {
            JStarsDirection = 0;
            JStarsCurrent = JStarsLast - 1;
            control->Parent_->SetGroupState(-200, 0);
        }

        if (JStarsCurrent < JStarsFirst)
        {
            JStarsCurrent = JStarsFirst;
            JStarsDirection = 0;
            control->Parent_->SetGroupState(-200, 0);
        }

        control->SetUserNumber(_UI95_TIMER_COUNTER_, JStarsDelay);
    }

    control->SetUserNumber(_UI95_TIMER_COUNTER_, control->GetUserNumber(_UI95_TIMER_COUNTER_) - 1);
}

void FitFlightPlanCB(long, short hittype, C_Base *control)
{
    F4CSECTIONHANDLE *Leave;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    if (gMapMgr)
    {
        Leave = UI_Enter(control->Parent_);

        gMapMgr->FitFlightPlan();
        gMapMgr->DrawMap();

        UI_Leave(Leave);
    }
}

void OpenCrossSectionCB(long, short hittype, C_Base *control)
{
    C_Text *txt;
    C_Window *win;
    C_Waypoint *wpz;
    _TCHAR buffer[30];
    F4CSECTIONHANDLE *Leave;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    win = gMapMgr->GetZWindow();

    if (win)
    {
        Leave = UI_Enter(win);

        if (StupidHackToCloseCSECT not_eq control->GetUserNumber(0))
        {
            StupidHackToCloseCSECT = control->GetUserNumber(0);
            gMapMgr->RecalcWaypointZs(StupidHackToCloseCSECT / 100);
            wpz = gMapMgr->GetCurWPZ();
            txt = (C_Text*)win->FindControl(DISTANCE_FIELD);

            if (txt)
            {
                if (wpz)
                    _stprintf(buffer, "%5.1f%s", (float)(wpz->GetUserNumber(2))*FT_TO_NM, gStringMgr->GetString(TXT_NM));
                else
                    _tcscpy(buffer, " ");

                Uni_Float(buffer);
                txt->SetText(buffer);
            }

            txt = (C_Text*)win->FindControl(FUEL_FIELD);

            if (txt)
            {
                if (wpz)
                    _stprintf(buffer, "%1ld", wpz->GetUserNumber(3));
                else
                    _tcscpy(buffer, " ");

                txt->SetText(buffer);
            }

            txt = (C_Text*)win->FindControl(DURATION_FIELD);

            if (txt)
            {
                if (wpz)
                    GetTimeString(wpz->GetUserNumber(1), buffer);
                else
                    _tcscpy(buffer, " ");

                txt->SetText(buffer);
            }

            win->HideCluster(control->GetUserNumber(1));
            win->UnHideCluster(StupidHackToCloseCSECT);
            win->RefreshWindow();
            gMainHandler->ShowWindow(win);
        }
        else
        {
            gMainHandler->HideWindow(win);
            StupidHackToCloseCSECT = 0;
        }

        UI_Leave(Leave);
    }
}

void LoadTroopMovementHistory()
{
    C_Window *win;
    C_History *hist;
    C_Slider *sldr;
    int i, recnum;
    CampaignTime time;
    UnitHistoryType *rec;
    char *filedata;
    FILE *fp;
    short Reading;
    short count;
    C_Text *txt;
    long TimeID;
    _TCHAR buffer[15];

    JStarsFirst = 0;
    JStarsLast = 0;
    JStarsCurrent = 0;
    recnum = 1;
    DeleteGroupList(HISTORY_WIN);

    int size = 1024, factor = 1;

    if (g_LargeTheater)
    {
        size = 2048;
        factor = 2;
    }

    win = gMainHandler->FindWindow(HISTORY_WIN);

    if (win)
    {
        fp = OpenCampFile("tmp", "his", "rb");

        if ( not fp)
            return;

        CampEnterCriticalSection();
        Reading = 1;

        while (Reading)
        {
            Reading = static_cast<short>(fread(&time, sizeof(CampaignTime), 1, fp));

            if (Reading)
            {
                fread(&count, sizeof(short), 1, fp);

                if (count)
                {
                    if (recnum == 1)
                    {
                        txt = (C_Text*)win->FindControl(COUNTER);

                        if (txt)
                        {
                            TimeID = time / (VU_TICS_PER_SECOND * 60);
                            _stprintf(buffer, "%s%2ld  %02d:00", gStringMgr->GetString(TXT_DAY), TimeID / 1440l + 1, (TimeID / 60) % 24);
                            txt->Refresh();
                            txt->SetText(buffer);
                            txt->Refresh();
                        }
                    }

                    hist = new C_History;
                    hist->Setup(recnum, 0, count);
                    hist->SetFlagBitOn(C_BIT_INVISIBLE);
                    hist->SetGroup(recnum);
                    hist->SetClient(1);
                    hist->SetUserNumber(0, (long)time / (VU_TICS_PER_SECOND * 60));
                    hist->SetUserNumber(_UI95_DELGROUP_SLOT_, _UI95_DELGROUP_ID_);
                    hist->SetImage(0, REDDOT5);
                    hist->SetImage(1, BLUEDOT5);
                    hist->SetImage(2, BLUEDOT5);
                    hist->SetImage(3, BLUEDOT5);
                    hist->SetImage(4, REDDOT5);
                    hist->SetImage(5, REDDOT5);
                    hist->SetImage(6, REDDOT5);
                    hist->SetImage(7, REDDOT5);

                    filedata = new char[count * sizeof(UnitHistoryType)];
                    fread(filedata, count * sizeof(UnitHistoryType), 1, fp);
                    rec = (UnitHistoryType*)filedata;

                    for (i = 0; i < count; i++)
                    {
                        // 2003-03-15 MN Support for 2048x2048 theaters
                        //hist->AddIconSet(static_cast<short>(i),rec->team,(short)((float)rec->x * 0.36198),(short)((float)(1024-rec->y) * 0.36198));
                        hist->AddIconSet(static_cast<short>(i), rec->team, (short)((float)(rec->x * 0.36198) / factor), (short)((float)((size - rec->y) * 0.36198)) / factor);
                        rec++;
                    }

                    hist->SetReady(1);
                    delete filedata;
                    win->AddControl(hist);
                    recnum++;
                }
            }
        }

        fclose(fp);
        CampLeaveCriticalSection();

        JStarsDirection = 0;
        JStarsFirst = 1;
        JStarsLast = recnum;
        JStarsCurrent = 1;
        JStarsPrevious = 1;
        sldr = (C_Slider*)win->FindControl(PLAYBALL);

        if (sldr)
        {
            sldr->Refresh();
            sldr->SetSliderPos(0);
            sldr->Refresh();

            if (recnum > 2)
                sldr->SetFlagBitOn(C_BIT_ENABLED);
            else
                sldr->SetFlagBitOff(C_BIT_ENABLED);
        }

        win->UnHideGroup(JStarsCurrent);
    }
}

#define _MAX_CATEGORIES_ (7)

long FrcLvlCatID[_MAX_CATEGORIES_] =
{
    STAT_1,
    STAT_2,
    STAT_3,
    STAT_4,
    STAT_5,
    STAT_6,
    STAT_7,
};

static long FindStatIndex(long ID)
{
    short i;

    for (i = 0; i < _MAX_CATEGORIES_; i++)
        if (ID == FrcLvlCatID[i])
            return(i);

    return(0);
}

void SelectForceCategoryCB(long, short hittype, C_Base *control)
{
    C_Window *win;
    C_Text *txt;
    C_Level *lvl;

    if (hittype not_eq C_TYPE_SELECT)
        return;

    win = gMainHandler->FindWindow(FORCE_WIN);

    if (win)
    {
        win->HideGroup(LastForceCatID);
        LastForceCatID = ((C_ListBox *)control)->GetTextID();
        lvl = (C_Level*)win->FindControl(4441313 + FindStatIndex(LastForceCatID));

        if (lvl)
        {
            txt = (C_Text*)win->FindControl(HIGH_Y);

            if (txt)
            {
                txt->Refresh();
                txt->SetText(lvl->GetYLabel(2));
                txt->Refresh();
            }

            txt = (C_Text*)win->FindControl(HALF_Y);

            if (txt)
            {
                txt->Refresh();
                txt->SetText(lvl->GetYLabel(1));
                txt->Refresh();
            }

            txt = (C_Text*)win->FindControl(LOW_Y);

            if (txt)
            {
                txt->Refresh();
                txt->SetText(lvl->GetYLabel(0));
                txt->Refresh();
            }
        }

        win->UnHideGroup(LastForceCatID);
        win->RefreshClient(1);
    }
}

COLORREF FrcTeamColors[_LEVEL_MAX_TEAMS_] =
{
    0x00000000, // Team 0
    0x00ff0000, // Team 1 (US)
    0x00f5f502, // Team 2 (ROK)
    0x00000000, // Team 3 (Japan)
    0x000098ec, // Team 4 (CIS)
    0x0000ffff, // Team 5 (China)
    0x000202f8, // Team 6 (DPRK)
    0x00000000, // Team 7
};

void LoadForceLevelHistory()
{
    C_Window *win;
    C_Level *stats[_MAX_CATEGORIES_];
    C_Text *txt;
    short i, j;
    short numteams;
    CampaignTime time;
    TeamStatusType teamstats[_LEVEL_MAX_TEAMS_];
    C_ListBox *lbox;
    FILE *fp;
    short Reading, First = 1;
    long MaxValue, MinValue;
    long TimeID, StatIdx, count, start = 0, end = 0;
    _TCHAR buffer[20];

    DeleteGroupList(FORCE_WIN);

    win = gMainHandler->FindWindow(FORCE_WIN);

    if (win)
    {
        fp = OpenCampFile("tmp", "frc", "rb");

        if ( not fp)
            return;

        for (i = 0; i < _MAX_CATEGORIES_; i++)
        {
            stats[i] = new C_Level;
            stats[i]->Setup(4441313 + i, 0);
            stats[i]->SetDrawArea(1, 1,
                                  static_cast<short>(win->ClientArea_[1].right - win->ClientArea_[1].left - 2),
                                  static_cast<short>(win->ClientArea_[1].bottom - win->ClientArea_[1].top - 2));
            stats[i]->SetGroup(FrcLvlCatID[i]);
            stats[i]->SetClient(1);
            stats[i]->SetFlagBitOn(C_BIT_INVISIBLE);
            stats[i]->SetUserNumber(_UI95_DELGROUP_SLOT_, _UI95_DELGROUP_ID_);

            for (j = 0; j < _LEVEL_MAX_TEAMS_; j++)
                stats[i]->SetTeamColor(static_cast<short>(j), FrcTeamColors[j]);
        }

        CampEnterCriticalSection();
        Reading = 1;
        numteams = 0;
        count = 0;

        while (Reading)
        {
            Reading = static_cast<short>(fread(&time, sizeof(CampaignTime), 1, fp));

            if (Reading)
            {
                count++;
                TimeID = time / (VU_TICS_PER_SECOND * 60);

                if (First)
                    start = TimeID;

                end = TimeID;

                if ( not fread(&numteams, sizeof(short), 1, fp))
                    MonoPrint("Error Reading Force Level\n");

                for (i = 0; i < numteams; i++)
                {
                    if ( not fread(&teamstats[i], sizeof(TeamStatusType), 1, fp))
                        MonoPrint("Error Reading Force Level\n");
                }

                for (i = 0; i < _MAX_CATEGORIES_; i++)
                {
                    if (First)
                        stats[i]->SetStart(TimeID);

                    stats[i]->SetEnd(TimeID);
                }

                First = 0;

                for (i = 0; i < numteams; i++)
                {
                    if (teamstats[i].supplyLevel < 255)
                    {
                        // Team is active
                        stats[0]->AddPoint(i, teamstats[i].airDefenseVehs);
                        stats[1]->AddPoint(i, teamstats[i].ships);
                        stats[2]->AddPoint(i, teamstats[i].supplyLevel);
                        stats[3]->AddPoint(i, teamstats[i].fuelLevel);
                        stats[4]->AddPoint(i, teamstats[i].airbases);
                        stats[5]->AddPoint(i, teamstats[i].aircraft);
                        stats[6]->AddPoint(i, teamstats[i].groundVehs);
                    }
                    else
                    {
                        // Team is inactive.
                        stats[0]->AddPoint(i, -1);
                        stats[1]->AddPoint(i, -1);
                        stats[2]->AddPoint(i, -1);
                        stats[3]->AddPoint(i, -1);
                        stats[4]->AddPoint(i, -1);
                        stats[5]->AddPoint(i, -1);
                        stats[6]->AddPoint(i, -1);
                    }
                }
            }
        }

        fclose(fp);
        CampLeaveCriticalSection();

        for (i = 0; i < _MAX_CATEGORIES_; i++)
        {
            stats[i]->CalcPositions();
            MaxValue = stats[i]->GetMaxValue();
            MinValue = stats[i]->GetMinValue();

            _stprintf(buffer, "%1ld", MaxValue);
            stats[i]->SetYLabel(2, buffer);
            _stprintf(buffer, "%1ld", (MaxValue - MinValue) / 2);
            stats[i]->SetYLabel(1, buffer);
            _stprintf(buffer, "%1ld", MinValue);
            stats[i]->SetYLabel(0, buffer);

            win->AddControl(stats[i]);
        }

        _stprintf(buffer, "%s %1ld %02ld:00", gStringMgr->GetString(TXT_DAY), start / (60 * 24) + 1, (start / 60) % 24);
        txt = (C_Text*)win->FindControl(LOW_X);

        if (txt)
        {
            txt->Refresh();
            txt->SetText(buffer);
            txt->Refresh();
        }

        _stprintf(buffer, "%s %1ld %02ld:00", gStringMgr->GetString(TXT_DAY), end / (60 * 24) + 1, (end / 60) % 24);
        txt = (C_Text*)win->FindControl(HIGH_X);

        if (txt)
        {
            txt->Refresh();
            txt->SetText(buffer);
            txt->Refresh();
        }

        lbox = (C_ListBox *)win->FindControl(CHART);

        if (lbox)
            LastForceCatID = lbox->GetTextID();
        else
            LastForceCatID = STAT_1;

        StatIdx = FindStatIndex(LastForceCatID);
        txt = (C_Text*)win->FindControl(HIGH_Y);

        if (txt)
        {
            txt->Refresh();
            txt->SetText(stats[StatIdx]->GetYLabel(2));
            txt->Refresh();
        }

        txt = (C_Text*)win->FindControl(HALF_Y);

        if (txt)
        {
            txt->Refresh();
            txt->SetText(stats[StatIdx]->GetYLabel(1));
            txt->Refresh();
        }

        txt = (C_Text*)win->FindControl(LOW_Y);

        if (txt)
        {
            txt->Refresh();
            txt->SetText(stats[StatIdx]->GetYLabel(0));
            txt->Refresh();
        }

        win->UnHideGroup(LastForceCatID);
    }
}

void UpdateRemoteCompression()
{
    C_Window *win;
    C_ListBox *lbox;
    long color, remreq;


    if ( not gMainHandler)
        return;

    remreq = 1;

    if (remoteCompressionRequests bitand REMOTE_REQUEST_2)
        remreq = 2;
    else if (remoteCompressionRequests bitand REMOTE_REQUEST_4)
        remreq = 4;
    else if (remoteCompressionRequests bitand REMOTE_REQUEST_PAUSE)
        remreq = 0;

    if ( not gCommsMgr->Online())
        color = 0x00ff00;
    else
    {
        if (targetCompressionRatio == FalconLocalSession->GetReqCompression())
        {
            if (remreq == targetCompressionRatio)
                color = 0x00ff00;
            else
                color = 0x00ffff;
        }
        else
            color = 0x0000ff;
    }

    // Campaign
    win = gMainHandler->FindWindow(CP_SUA);

    if (win)
    {
        lbox = (C_ListBox*)win->FindControl(ACCELERATION);

        if (lbox)
        {
            lbox->SetLabelColor(color);
            lbox->Refresh();
            lbox->SetValue(FalconLocalSession->GetReqCompression());
        }
    }

    // Taceng
    win = gMainHandler->FindWindow(TAC_MISSION_SUA);

    if (win)
    {
        lbox = (C_ListBox*)win->FindControl(ACCELERATION);

        if (lbox)
        {
            lbox->SetLabelColor(color);
            lbox->Refresh();
            lbox->SetValue(FalconLocalSession->GetReqCompression());
        }
    }
}

void InitTimeCompressionBox(long compression)
{
    C_Window *win;
    C_ListBox *lbox;

    UpdateRemoteCompression();
    // Campaign
    win = gMainHandler->FindWindow(CP_SUA);

    if (win)
    {
        lbox = (C_ListBox*)win->FindControl(ACCELERATION);

        if (lbox)
        {
            if (compression)
                lbox->SetValue(compression);
            else
                lbox->SetValue(ACCEL_0); // stopped
        }
    }

    // Taceng
    win = gMainHandler->FindWindow(TAC_MISSION_SUA);

    if (win)
    {
        lbox = (C_ListBox*)win->FindControl(ACCELERATION);

        if (lbox)
        {
            if (compression)
                lbox->SetValue(compression);
            else
                lbox->SetValue(ACCEL_0); // stopped
        }
    }
}


void TimeCompressionCB(long, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_SELECT)
        return;

    if (((C_ListBox *)control)->GetTextID() == ACCEL_0)
    {
        SetTimeCompression(0);
    }
    else
    {
        SetTimeCompression(((C_ListBox *)control)->GetTextID());
    }

    UpdateRemoteCompression();
}

void OpenNewsWindowCB(long, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    gMainHandler->EnableWindowGroup(control->GetGroup());
}

static void HookupCampaignControls(long ID)
{
    C_Window *winme;
    C_Button *ctrl;
    C_ListBox *lbox;
    C_TimerHook *tmr;
    C_Bitmap *bmp;
    C_Blip *blip;
    C_Slider *sldr;
    C_Clock *clk;
    int i, j;

    winme = gMainHandler->FindWindow(ID);

    if (winme == NULL)
        return;

    // Time/Date CB
    clk = (C_Clock *)winme->FindControl(TIME_ID);

    if (clk)
    {
        clk->SetTimerCallback(CampaignClockCB);
        clk->SetFlagBitOn(C_BIT_TIMER);
        clk->Refresh();
    }

    // Hook up IDs here
    // Set Commit Button
    ctrl = (C_Button *)winme->FindControl(MISSION_MAIN_CTRL);

    if (ctrl)
        ctrl->SetCallback(OpenCampaignCB);

    // Set Commit Button
    ctrl = (C_Button *)winme->FindControl(SQUAD_MAIN_CTRL);

    if (ctrl)
        ctrl->SetCallback(OpenCampaignCB);

    // Set Commit Button
    ctrl = (C_Button *)winme->FindControl(INTEL_MAIN_CTRL);

    if (ctrl)
        ctrl->SetCallback(OpenCampaignCB);

    ctrl = (C_Button *)winme->FindControl(SP_MAIN_CTRL);

    if (ctrl)
        ctrl->SetCallback(OpenSetupCB);

    ctrl = (C_Button *)winme->FindControl(ACMI_CTRL);

    if (ctrl)
        ctrl->SetCallback(ACMIButtonCB);

    ctrl = (C_Button *)winme->FindControl(TACREF_CTRL);

    if (ctrl)
        ctrl->SetCallback(OpenTacticalReferenceCB);

    ctrl = (C_Button *)winme->FindControl(NEWS_FLASH);

    if (ctrl)
        ctrl->SetCallback(OpenNewsWindowCB);

    // Save AS button
    ctrl = (C_Button *)winme->FindControl(CAMP_SAVE_CTRL);

    if (ctrl)
        ctrl->SetCallback(CampSaveAsCB);

    // Set Commit Button
    ctrl = (C_Button *)winme->FindControl(CAMP_CLOSE_CTRL);

    if (ctrl)
        ctrl->SetCallback(EndCommitCB);

    ctrl = (C_Button *)winme->FindControl(CLOSE_WINDOW);

    if (ctrl)
        ctrl->SetCallback(GenericCloseCB);

    ctrl = (C_Button *)winme->FindControl(BI_M_MX_CTRL);

    if (ctrl)
        ctrl->SetCallback(OpenFullMapCB);

    ctrl = (C_Button *)winme->FindControl(CB_FULLMAP_CLOSE_CTRL);

    if (ctrl)
        ctrl->SetCallback(CloseFullMapCB);

    // Hook up Close Button
    ctrl = (C_Button *)winme->FindControl(CAMP_CLOSE);

    if (ctrl)
        ctrl->SetCallback(CloseCampaignWindowCB);

    ctrl = (C_Button *)winme->FindControl(BI_LIN_CTRL);

    if (ctrl)
        ctrl->SetCallback(OpenCrossSectionCB);

    ctrl = (C_Button *)winme->FindControl(BI_LOG_CTRL);

    if (ctrl)
        ctrl->SetCallback(OpenCrossSectionCB);

    ctrl = (C_Button *)winme->FindControl(BI_FIT_CTRL);

    if (ctrl)
        ctrl->SetCallback(FitFlightPlanCB);

    // Hook up Small MAP Buttons
    ctrl = (C_Button *)winme->FindControl(BI_ZIN_CTRL);

    if (ctrl)
        ctrl->SetCallback(SmallMapZoomInCB);

    ctrl = (C_Button *)winme->FindControl(BI_ZOUT_CTRL);

    if (ctrl)
        ctrl->SetCallback(SmallMapZoomOutCB);

    ctrl = (C_Button *)winme->FindControl(SINGLE_FLY_CTRL);

    if (ctrl)
        ctrl->SetCallback(CampaignButtonCB);

    ctrl = (C_Button *)winme->FindControl(COMMS_FLY_CTRL);

    if (ctrl)
        ctrl->SetCallback(CampaignButtonCB);

    ctrl = (C_Button *)winme->FindControl(BRIEF_BUTTON);

    if (ctrl)
        ctrl->SetCallback(OpenBriefingWindowCB);

    ctrl = (C_Button *)winme->FindControl(ATO_BUTTON);

    if (ctrl)
        ctrl->SetCallback(OpenATOWindowCB);

    ctrl = (C_Button *)winme->FindControl(FLIGHT_PLAN_BUTTON);

    if (ctrl)
        ctrl->SetCallback(OpenFlightPlanWindowCB);

    ctrl = (C_Button *)winme->FindControl(MUNITIONS_BUTTON);

    if (ctrl)
        ctrl->SetCallback(OpenMunitionsWindowCB);

    ctrl = (C_Button *)winme->FindControl(FORCE_LEVELS_BUTTON);

    if (ctrl)
        ctrl->SetCallback(OpenForceLevelsWindowCB);

    ctrl = (C_Button *)winme->FindControl(CAMP_HISTORY_BUTTON);

    if (ctrl)
        ctrl->SetCallback(OpenHistoryWindowCB);

    ctrl = (C_Button *)winme->FindControl(CAMP_OOB_BUTTON);

    if (ctrl)
        ctrl->SetCallback(OpenOOBWindowCB);

    ctrl = (C_Button *)winme->FindControl(SQUADRON_BUTTON);

    if (ctrl)
        ctrl->SetCallback(OpenSquadronWindowCB);

    ctrl = (C_Button *)winme->FindControl(SIERRA_HOTEL);

    if (ctrl)
        ctrl->SetCallback(OpenSierraHotelCB);

    ctrl = (C_Button *)winme->FindControl(CAMP_ACMI_CTRL);

    if (ctrl)
        ctrl->SetCallback(ACMIButtonCB);

    ctrl = (C_Button*)winme->FindControl(STOP);

    if (ctrl)
        ctrl->SetCallback(HistoryStop);

    ctrl = (C_Button*)winme->FindControl(REVERSE);

    if (ctrl)
        ctrl->SetCallback(HistoryPlayReverse);

    ctrl = (C_Button*)winme->FindControl(FAST_REVERSE);

    if (ctrl)
        ctrl->SetCallback(HistoryFastReverse);

    ctrl = (C_Button*)winme->FindControl(STEP_REVERSE);

    if (ctrl)
        ctrl->SetCallback(HistoryStepReverse);

    ctrl = (C_Button*)winme->FindControl(PLAY);

    if (ctrl)
        ctrl->SetCallback(HistoryPlayForward);

    ctrl = (C_Button*)winme->FindControl(FAST_FORWARD);

    if (ctrl)
        ctrl->SetCallback(HistoryFastForward);

    ctrl = (C_Button*)winme->FindControl(STEP_FORWARD);

    if (ctrl)
        ctrl->SetCallback(HistoryStepForward);

    sldr = (C_Slider*)winme->FindControl(PLAYBALL);

    if (sldr)
        sldr->SetCallback(HistoryDragBallCB);

    ctrl = (C_Button *)winme->FindControl(SORT_PRIORITY);

    if (ctrl)
        ctrl->SetCallback(SelectMissionSortCB);

    ctrl = (C_Button *)winme->FindControl(SORT_TAKEOFF);

    if (ctrl)
        ctrl->SetCallback(SelectMissionSortCB);

    ctrl = (C_Button *)winme->FindControl(SORT_PACKAGE);

    if (ctrl)
        ctrl->SetCallback(SelectMissionSortCB);

    ctrl = (C_Button *)winme->FindControl(SORT_ROLE);

    if (ctrl)
        ctrl->SetCallback(SelectMissionSortCB);

    ctrl = (C_Button *)winme->FindControl(SORT_STATUS);

    if (ctrl)
        ctrl->SetCallback(SelectMissionSortCB);

    ctrl = (C_Button *)winme->FindControl(CB_1_1);

    if (ctrl)
        ctrl->SetCallback(PickCampaignPlaneCB);

    ctrl = (C_Button *)winme->FindControl(CB_2_1);

    if (ctrl)
        ctrl->SetCallback(PickCampaignPlaneCB);

    ctrl = (C_Button *)winme->FindControl(CB_2_2);

    if (ctrl)
        ctrl->SetCallback(PickCampaignPlaneCB);

    ctrl = (C_Button *)winme->FindControl(CB_3_1);

    if (ctrl)
        ctrl->SetCallback(PickCampaignPlaneCB);

    ctrl = (C_Button *)winme->FindControl(CB_3_2);

    if (ctrl)
        ctrl->SetCallback(PickCampaignPlaneCB);

    ctrl = (C_Button *)winme->FindControl(CB_3_3);

    if (ctrl)
        ctrl->SetCallback(PickCampaignPlaneCB);

    ctrl = (C_Button *)winme->FindControl(CB_4_1);

    if (ctrl)
        ctrl->SetCallback(PickCampaignPlaneCB);

    ctrl = (C_Button *)winme->FindControl(CB_4_2);

    if (ctrl)
        ctrl->SetCallback(PickCampaignPlaneCB);

    ctrl = (C_Button *)winme->FindControl(CB_4_3);

    if (ctrl)
        ctrl->SetCallback(PickCampaignPlaneCB);

    ctrl = (C_Button *)winme->FindControl(CB_4_4);

    if (ctrl)
        ctrl->SetCallback(PickCampaignPlaneCB);

    ctrl = (C_Button *)winme->FindControl(LB_MAIN_CTRL);

    if (ctrl)
        ctrl->SetCallback(OpenLogBookCB);

    lbox = (C_ListBox *)winme->FindControl(BAR_1);

    if (lbox)
        lbox->SetCallback(UpdateIntelBarCB);

    lbox = (C_ListBox *)winme->FindControl(BAR_2);

    if (lbox)
        lbox->SetCallback(UpdateIntelBarCB);

    lbox = (C_ListBox *)winme->FindControl(BAR_3);

    if (lbox)
        lbox->SetCallback(UpdateIntelBarCB);

    lbox = (C_ListBox *)winme->FindControl(BAR_4);

    if (lbox)
        lbox->SetCallback(UpdateIntelBarCB);

    lbox = (C_ListBox *)winme->FindControl(ACCELERATION);

    if (lbox)
        lbox->SetCallback(TimeCompressionCB);

    lbox = (C_ListBox *)winme->FindControl(CHART);

    if (lbox)
        lbox->SetCallback(SelectForceCategoryCB);

    ctrl = (C_Button *)winme->FindControl(CO_MAIN_CTRL);

    if (ctrl)
        ctrl->SetCallback(OpenCampaignCommsCB);

    bmp = (C_Bitmap*)winme->FindControl(CP_SUA);

    if (bmp)
    {
        bmp->SetImage(gOccupationMap);

        blip = new C_Blip;
        blip->Setup(9000000, 0);
        blip->SetClient(1);

        for (i = 0; i < 8; i++)
        {
            for (j = 0; j < 8; j++)
                blip->SetImage(BLIP_IDS[0][j], static_cast<uchar>(i), static_cast<uchar>(j));
        }

        blip->InitDrawer();
        winme->AddControl(blip);

        tmr = new C_TimerHook;
        tmr->Setup(C_DONT_CARE, C_TYPE_TIMER);
        tmr->SetUpdateCallback(UpdateEventBlipsCB);
        tmr->SetUserNumber(_UI95_TIMER_DELAY_, 5 * _UI95_TICKS_PER_SECOND_);
        winme->AddControl(tmr);
    }

    if (ID == HISTORY_WIN)
    {
        tmr = new C_TimerHook;
        tmr->Setup(C_DONT_CARE, C_TYPE_TIMER);
        tmr->SetClient(1);
        tmr->SetUpdateCallback(HistoryTimerCB);
        winme->AddControl(tmr);
    }

    // Controls for setting priorities window (strategy.scf)
    ctrl = (C_Button*)winme->FindControl(OK);

    if (ctrl)
        ctrl->SetCallback(UsePriotityCB);

    ctrl = (C_Button*)winme->FindControl(CANCEL);

    if (ctrl)
        ctrl->SetCallback(CancelPriorityCB);

    // OLD Control... removed by Joe
    // ctrl=(C_Button*)winme->FindControl(RESET);
    // if(ctrl)
    // ctrl->SetCallback(ResetPriorityCB);

    ctrl = (C_Button*)winme->FindControl(TARGET_PRIORITIES);

    if (ctrl)
        ctrl->SetCallback(PriorityTabsCB);

    ctrl = (C_Button*)winme->FindControl(MISSION_PRIORITIES);

    if (ctrl)
        ctrl->SetCallback(PriorityTabsCB);

    ctrl = (C_Button*)winme->FindControl(PAK_PRIORITIES);

    if (ctrl)
        ctrl->SetCallback(PriorityTabsCB);

    ctrl = (C_Button*)winme->FindControl(THEATER_256);

    if (ctrl)
        ctrl->SetCallback(MapSelectPAKCB);

    ctrl = (C_Button*)winme->FindControl(HQ_FLAG);

    if (ctrl)
        ctrl->SetCallback(SetCampaignPrioritiesCB);

    lbox = (C_ListBox*)winme->FindControl(PAK_TITLE);

    if (lbox)
        lbox->SetCallback(SelectPAKCB);

    sldr = (C_Slider*)winme->FindControl(PAK_SLIDER);

    if (sldr)
        sldr->SetCallback(SetPAKPriorityCB);

    bmp = (C_Bitmap*)winme->FindControl(PAK_REGION);

    if (bmp)
    {
        bmp->SetTimerCallback(PAKMapTimerCB);
    }

    ctrl = (C_Button*)winme->FindControl(SET_PRIORITIES);

    if (ctrl)
        ctrl->SetCallback(OpenPriorityCB);

    // Help GUIDE thing
    ctrl = (C_Button*)winme->FindControl(UI_HELP_GUIDE);

    if (ctrl)
        ctrl->SetCallback(UI_Help_Guide_CB);

    // HACK... remove these before shipping
    ctrl = (C_Button*)winme->FindControl(CAMP_HACK_BTN_1);

    if (ctrl)
        ctrl->SetCallback(CampHackButton1CB);

    ctrl = (C_Button*)winme->FindControl(CAMP_HACK_BTN_2);

    if (ctrl)
        ctrl->SetCallback(CampHackButton2CB);

    ctrl = (C_Button*)winme->FindControl(CAMP_HACK_BTN_3);

    if (ctrl)
        ctrl->SetCallback(CampHackButton3CB);

    ctrl = (C_Button*)winme->FindControl(CAMP_HACK_BTN_4);

    if (ctrl)
        ctrl->SetCallback(CampHackButton4CB);

    ctrl = (C_Button*)winme->FindControl(CAMP_HACK_BTN_5);

    if (ctrl)
        ctrl->SetCallback(CampHackButton5CB);

    winme = gMainHandler->FindWindow(STARTCAMP_WIN);

    if ( not winme)
        return;

    ctrl = (C_Button*)winme->FindControl(START_CAMP);

    if (ctrl)
        ctrl->SetCallback(StartCampaignCB);

}

void StartCampaignCB(long, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    SetTimeCompression(1);
    UpdateRemoteCompression();
    CloseWindowCB(STARTCAMP_WIN, hittype, control);
}
