///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
// Tactical Engagement - Robin Heydon
//
// Implements the user interface for the tactical engagement section
// of FreeFalcon
//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <ddraw.h>
#include "falclib.h"
#include "unit.h"
#include "team.h"
#include "CmpGlobl.h"
#include "CampCell.h"
#include "CampTerr.h"
#include "Listadt.h"
#include "objectiv.h"
#include "Campaign.h"
#include "campmap.h"
#include "campwp.h"
#include "campstr.h"
#include "squadron.h"
#include "feature.h"
#include "pilot.h"
#include "team.h"
#include "find.h"
#include "misseval.h"
#include "cmpclass.h"
#include "ui95_dd.h"
#include "chandler.h"
#include "ui95_ext.h"
#include "AirUnit.h"
#include "uicomms.h"
#include "userids.h"
#include "classtbl.h"
#include "textids.h"
#include "tac_class.h"
#include "te_defs.h"
#include "division.h"
#include "cmap.h"
#include "ui_cmpgn.h"
#include "MsgInc/RequestAircraftSlot.h"
#include "vu2.h"
#include "F4Find.h"
#include "F4Error.h"
#include "gps.h"
#include "camplist.h" // M.N. Needed for Front/FLOTlist

#ifdef CAMPTOOL
// Renaming tool stuff
extern int gRenameIds;
#endif CAMPTOOL

#pragma warning(disable : 4127) // Conditional Expression is constant warning

extern C_Map   *gMapMgr;


extern int MainLastGroup, TacLastGroup;
extern void RebuildFLOTList(void); // 2001-10-31 M.N.
extern int RebuildFrontList(void);  // 2001-10-31 M.N.
extern _TCHAR gLastTEFile[MAX_PATH]; // 2002-03-12 MN
extern bool g_bEmptyFilenameFix; // 2002-04-18 MN


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SaveAFile(long TitleID, _TCHAR *filespec, _TCHAR *excludelist[], void (*YesCB)(long, short, C_Base*), void (*NoCB)(long, short, C_Base*), _TCHAR *filename);
static void update_taceng_clock(void);

static void TACNewCB(long ID, short hittype, C_Base *control);
static void TACEditCB(long ID, short hittype, C_Base *control);
static void TACAcceptCB(long ID, short hittype, C_Base *control);
static void TACHostCB(long ID, short hittype, C_Base *control);
static void TACInfoCB(long ID, short hittype, C_Base *control);
static void TACExitCB(long ID, short hittype, C_Base *control);
static void TACRevertCB(long ID, short hittype, C_Base *control);
static void TACSaveAsCB(long ID, short hittype, C_Base *control);

void SetupInfoWindow(void (*tOkCB)(), void (*tCancelCB)());
void SetDeleteCallback(void (*cb)(long, short, C_Base*));
void TacticalEngagementSetup(bool noawacsmap);
void PauseTacticalEngagement(void);
void MakeTacticalEdit(void);
void RemoveTacticalEdit(void);
void SetupMapMgr(bool noawacsmap);
void LoadTacticalWindows(void);
void LoadTacEngSelectWindows();
void SetTimeCompression(int);
void CleanupTacticalEngagementUI();
void PickTeamColors();
void InitTimeCompressionBox(long compression);
void UpdateOwners();
extern long OwnershipChanged;
void tactical_edit_mission(tactical_mission *);
static void tactical_revert_mission(void);

void SetupInfoWindow(void (*tOkCB)(), void (*tCancelCB)());
void SetupOccupationMap(void);
void DelSTRFileCB(long ID, short hittype, C_Base *control);
void DelDFSFileCB(long ID, short hittype, C_Base *control);
void DelLSTFileCB(long ID, short hittype, C_Base *control);
void DelCamFileCB(long ID, short hittype, C_Base *control);
void DelTacFileCB(long ID, short hittype, C_Base *control);
void DelTGAFileCB(long ID, short hittype, C_Base *control);
void DelVHSFileCB(long ID, short hittype, C_Base *control);
void DelKeyFileCB(long ID, short hittype, C_Base *control);
void SetupTeamData(void);
void OpenBuilderWindowCB(long ID, short hittype, C_Base *base);
void OpenMissionWindowCB(long ID, short hittype, C_Base *base);
void AreYouSure(long TitleID, long MessageID, void (*OkCB)(long, short, C_Base*), void (*CancelCB)(long, short, C_Base*));
void AreYouSure(long TitleID, _TCHAR *text, void (*OkCB)(long, short, C_Base*), void (*CancelCB)(long, short, C_Base*));
BOOL CheckExclude(_TCHAR *filename, _TCHAR *directory, _TCHAR *ExcludeList[], _TCHAR *extension);
void VerifyDelete(long TitleID, void (*YesCB)(long, short, C_Base*), void (*NoCB)(long, short, C_Base*));
void GetTacticalFileList();

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern GlobalPositioningSystem
*gGps;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int
tactical_debriefing = FALSE,
tactical_mission_loaded = FALSE;

extern _TCHAR *TEExcludeList[];
extern uchar gSelectedTeam;
extern long OwnershipChanged;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void hookup_flow_buttons(C_Window *winme)
{
    C_Button
    *ctrl;

    // New Mission Button
    ctrl = (C_Button *) winme->FindControl(TAC_NEW_MISSION);

    if (ctrl)
    {
        ctrl->SetCallback(TACNewCB);
    }

    ctrl = (C_Button *) winme->FindControl(TAC_EDIT);

    if (ctrl)
    {
        ctrl->SetCallback(TACEditCB);
    }

    // Accept Button
    ctrl = (C_Button *) winme->FindControl(SINGLE_COMMIT_CTRL);

    if (ctrl)
    {
        ctrl->SetCallback(TACAcceptCB);
    }

    // Accept Button
    ctrl = (C_Button *) winme->FindControl(COMMS_COMMIT_CTRL);

    if (ctrl)
    {
        ctrl->SetCallback(TACAcceptCB);
    }

    // Host Button
    ctrl = (C_Button *) winme->FindControl(TAC_HOST);

    if (ctrl)
    {
        ctrl->SetCallback(TACHostCB);
        ctrl->SetFlagBitOn(C_BIT_INVISIBLE);
    }

    // Info Button
    ctrl = (C_Button *) winme->FindControl(TAC_INFO);

    if (ctrl)
    {
        ctrl->SetCallback(TACInfoCB);
    }

    // Exit Button
    ctrl = (C_Button *) winme->FindControl(TAC_EXIT);

    if (ctrl)
    {
        ctrl->SetCallback(TACExitCB);
    }

    // Revert Button
    ctrl = (C_Button *) winme->FindControl(TAC_RESTORE_CTRL);

    if (ctrl)
    {
        ctrl->SetCallback(TACRevertCB);
    }

    // SaveAS Button
    ctrl = (C_Button *) winme->FindControl(TAC_SAVE_CTRL);

    if (ctrl)
    {
        ctrl->SetCallback(TACSaveAsCB);
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void ActivateTacMissionSchedule()
{
    C_Window *win;
    C_Button *btn;

    win = gMainHandler->FindWindow(TAC_PLAY_SCREEN);

    if (win)
    {
        btn = (C_Button*)win->FindControl(TAC_MISS_MAIN_CTRL);

        if (btn)
            OpenMissionWindowCB(btn->GetID(), C_TYPE_LMOUSEUP, btn);

        // Select 1st mission
    }

    SetupTeamData();
    SetupOccupationMap();
}

void ActivateTacMissionBuilder()
{
    C_Window *win;
    C_Button *btn;

    win = gMainHandler->FindWindow(TAC_EDIT_SCREEN);

    if (win)
    {
        btn = (C_Button*)win->FindControl(BUILDER_MAIN_CTRL);

        if (btn)
            OpenBuilderWindowCB(btn->GetID(), C_TYPE_LMOUSEUP, btn);
    }

    SetupTeamData();
    SetupOccupationMap();
    // 2001-10-31 M.N. needed to calculate distance of target to FLOT
    RebuildFrontList(TRUE, FALSE);
    RebuildFLOTList();
}

static void TACNewCB(long, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
    {
        return;
    }

    LoadTacticalWindows();
    gMainHandler->EnterCritical();
    gMainHandler->DisableWindowGroup(100);
    gMainHandler->DisableSection(100);
    gMainHandler->SetSection(200);

    if (current_tactical_mission)
    {
        delete current_tactical_mission;
    }

    TheCampaign.EndCampaign();

    char path[_MAX_PATH];
    sprintf(path, "%s\\te_new.tac", FalconCampaignSaveDirectory);

    current_tactical_mission = new tactical_mission(path);

#ifdef CAMPTOOL

    if (gRenameIds)
        SendMessage(gMainHandler->GetAppWnd(), FM_LOAD_CAMPAIGN, 0, game_TacticalEngagement);
    else
#endif
        tactical_edit_mission(current_tactical_mission);

    gMainHandler->EnableWindowGroup(control->GetGroup());
    ActivateTacMissionBuilder();
    gSelectedTeam = 1;
    PickTeamColors();
    gMainHandler->LeaveCritical();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static void TACEditCB(long ID, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
    {
        return;
    }

    if ( not current_tactical_mission)
    {
        TACNewCB(ID, hittype, control);
        return;
    }

    LoadTacticalWindows();
    gMainHandler->EnterCritical();
    gMainHandler->DisableWindowGroup(100);
    gMainHandler->DisableSection(100);
    gMainHandler->SetSection(200);

#ifdef CAMPTOOL

    if (gRenameIds)
        SendMessage(gMainHandler->GetAppWnd(), FM_LOAD_CAMPAIGN, 0, game_TacticalEngagement);
    else
#endif
        tactical_edit_mission(current_tactical_mission);

    gMainHandler->EnableWindowGroup(control->GetGroup());
    ActivateTacMissionBuilder();
    gSelectedTeam = 1;
    PickTeamColors();
    gMainHandler->LeaveCritical();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static void TACCancelJoinCB(void)
{
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static void TACReallyAcceptCB(void)
{
    if ( not current_tactical_mission)
    {
        return;
    }

    SetCursor(gCursors[CRSR_WAIT]);

    LoadTacticalWindows();

    gMainHandler->EnterCritical();
    tactical_accept_mission();
    gMainHandler->LeaveCritical();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static void TACAcceptCB(long, short hittype, C_Base *)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
    {
        return;
    }

    SetCursor(gCursors[CRSR_WAIT]);

    if ( not current_tactical_mission)
    {
        return;
    }

    if (gCommsMgr->Online())
        SetupInfoWindow(TACReallyAcceptCB, TACCancelJoinCB);
    else
        TACReallyAcceptCB();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UpdateVCs()
{
    victory_condition *vc;

    if (current_tactical_mission and gMapMgr)
    {
        vc = current_tactical_mission->get_first_unfiltered_victory_condition();

        while (vc)
        {
            gMapMgr->UpdateVC(vc);
            vc = current_tactical_mission->get_next_unfiltered_victory_condition();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static void TACExitCB(long, short hittype, C_Base *)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
    {
        return;
    }

    tactical_mission_loaded = FALSE;

    RemoveTacticalEdit();

    gMainHandler->EnterCritical();

    TheCampaign.EndCampaign();
    CleanupTacticalEngagementUI();
    TacLastGroup = 0;

    gMainHandler->DisableSection(200);
    gMainHandler->SetSection(100);
    gMainHandler->EnableWindowGroup(100);
    gMainHandler->EnableWindowGroup(MainLastGroup);
    gMainHandler->LeaveCritical();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static void TACInfoCB(long, short hittype, C_Base *)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
    {
        return;
    }

    SetupInfoWindow(NULL, NULL);
    // gMainHandler->EnableWindowGroup (control->GetGroup ());

    //MonoPrint ("Info\n");
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static void TACHostCB(long, short hittype, C_Base *)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
    {
        return;
    }

    if (current_tactical_mission)
    {
        tactical_accept_mission();
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static void TACRevertCB(long, short hittype, C_Base *)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
    {
        return;
    }

    tactical_revert_mission();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static void TACSaveFileCB(long, short hittype, C_Base *control)
{
    C_EditBox *edit_box;
    C_Window *win;

    char buffer[100];

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    win = gMainHandler->FindWindow(SAVE_WIN);

    if ( not win)
        return;

    gMainHandler->HideWindow(win);
    gMainHandler->HideWindow(control->Parent_);

    edit_box = (C_EditBox*) win->FindControl(FILE_NAME);

    if (edit_box)
    {
        _tcscpy(buffer, edit_box->GetText());
    }

    current_tactical_mission->save(buffer);
    GetTacticalFileList();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static void TACVerifySaveFileCB(long ID, short hittype, C_Base *control)
{
    C_EditBox *edit_box;
    FILE *fp;

    char buffer[MAX_PATH];

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    edit_box = (C_EditBox*) control->Parent_->FindControl(FILE_NAME);

    if (edit_box)
    {
        //dpc EmptyFilenameSaveFix, modified by MN - added a warning to enter a filename
        if (g_bEmptyFilenameFix)
        {
            if (_tcslen(edit_box->GetText()) == 0)
            {
                AreYouSure(TXT_WARNING, TXT_ENTER_FILENAME, CloseWindowCB, CloseWindowCB);
                return;
            }
        }

        //end EmptyFilenameSaveFix
        _stprintf(buffer, "%s\\%s.tac", FalconCampUserSaveDirectory, edit_box->GetText());
        fp = fopen(buffer, "r");

        if (fp)
        {
            fclose(fp);

            if (CheckExclude(buffer, FalconCampUserSaveDirectory, TEExcludeList, "tac"))
                AreYouSure(TXT_ERROR, TXT_CANT_OVERWRITE, CloseWindowCB, CloseWindowCB);
            else
                AreYouSure(TXT_SAVE_ENGAGEMENT, TXT_FILE_EXISTS, TACSaveFileCB, CloseWindowCB);
        }
        else
        {
            if (CheckExclude(buffer, FalconCampUserSaveDirectory, TEExcludeList, "tac"))
                AreYouSure(TXT_ERROR, TXT_CANT_OVERWRITE, CloseWindowCB, CloseWindowCB);
            else
                TACSaveFileCB(ID, hittype, control);
        }
    }
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void LoadSaveSelectFileCB(long, short hittype, C_Base *control)
{
    C_EditBox *ebox;
    C_Button *btn;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    if (control)
    {
        btn = (C_Button*)control;
        ebox = (C_EditBox*)btn->Parent_->FindControl(FILE_NAME);

        if (ebox)
        {
            ebox->Refresh();
            ebox->SetText(btn->GetText(C_STATE_0));
            ebox->Refresh();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static void TACSaveAsCB(long, short hittype, C_Base *)
{
    _TCHAR buffer[MAX_PATH];
    _TCHAR filename [MAX_PATH];

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    if (OwnershipChanged)
        UpdateOwners();

    if (strlen(gLastTEFile) > 1) // a tac file has at least 1 char
        sprintf(filename, "%s", gLastTEFile);
    else
        sprintf(filename, "");

    sprintf(buffer, "%s\\*.tac", FalconCampUserSaveDirectory);

    SetDeleteCallback(DelTacFileCB);
    SaveAFile(TXT_SAVE_ENGAGEMENT, buffer, TEExcludeList, TACVerifySaveFileCB, CloseWindowCB, filename);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
extern bool g_bServer;
void tactical_play_setup()
{
    short i;

    if ( not gMainHandler)
        return;

    // MonoPrint ("Tactical Play Setup\n");

    gMainHandler->EnterCritical();
    gMainHandler->DisableWindowGroup(100);
    gMainHandler->DisableSection(100);
    gMainHandler->SetSection(200);
    gMainHandler->EnableWindowGroup(3025);

    tactical_update_campaign_entities();

    TheCampaign.Flags or_eq CAMP_TACTICAL;


    // MONUMENTOUS HACK to get team color bitand Flag initialized (If they aren't already)
    for (i = 0; i < NUM_TEAMS; i++)
    {
        if (TeamInfo[i])
        {
            if ( not TeamInfo[i]->GetFlag())
                TeamInfo[i]->SetFlag(static_cast<uchar>(i));

            if ( not TeamInfo[i]->GetColor())
                TeamInfo[i]->SetColor(static_cast<uchar>(i));

            if (i and not (TeamInfo[i]->flags bitand TEAM_ACTIVE))
                TeamInfo[i]->flags or_eq TEAM_ACTIVE;
        }
    }

    update_taceng_clock();

    // 2002-03-06 MN added parameter
    TacticalEngagementSetup(false);

    update_missions_details(TAC_AIRCRAFT);

    gGps->SetAllowed(0xffffffff);

    if ( not g_bServer and current_tactical_mission->get_type() == tt_engagement)
    {
        InitTimeCompressionBox(1);
        SetTimeCompression(1);
    }
    else
    {
        //PauseTacticalEngagement();
        InitTimeCompressionBox(0);
        SetTimeCompression(0);
    }

    ActivateTacMissionSchedule();
    gMainHandler->LeaveCritical();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void tactical_accept_mission(void)
{
    if (current_tactical_mission)
    {
        current_tactical_mission->load();
    }
    else
    {
        // Cannot accept a new mission - sorry
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void tactical_restart_mission(void)
{
    tactical_type
    current_type;

    if (current_tactical_mission)
    {
        current_type = current_tactical_mission->get_type();

        current_tactical_mission->load();

        current_tactical_mission->set_type(current_type);
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void restart_tactical_engagement(void)
{
    if (current_tactical_mission->get_type() not_eq tt_engagement)
    {
        TheCampaign.Suspend();
        InitTimeCompressionBox(0);
        SetTimeCompression(0);
    }

    gMainHandler->SetSection(200);

    LoadTacEngSelectWindows();
    LoadTacticalWindows();

    tactical_play_setup();

    if (TacLastGroup)
        gMainHandler->EnableWindowGroup(TacLastGroup);
    else
        ActivateTacMissionSchedule();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void tactical_edit_mission(tactical_mission *)
{
    short i;
    int
    loop;

    char
    filename[MAX_PATH];

    FILE
    *fp;

    SetTimeCompression(0);

    if (current_tactical_mission)
    {
        MakeTacticalEdit();

        current_tactical_mission->load();
    }
    else
    {
        ShiAssert( not "This doesn't work, so should not be allowed");

        return;

        // We are trying to edit a new mission
        for (loop = 0; loop < 10000; loop ++)
        {
            sprintf(filename, "%s\\mission%d.te", FalconCampaignSaveDirectory, loop);

            fp = fopen(filename, "r");

            if (fp)
            {
                fclose(fp);
            }
            else
            {
                current_tactical_mission = new tactical_mission(filename);
                break;
            }
        }

        F4Assert(current_tactical_mission);

        current_tactical_mission->set_type(tt_engagement);

        MakeTacticalEdit();

        current_tactical_mission->new_setup();
    }

    tactical_mission_loaded = TRUE;

    // MONUMENTOUS HACK to get team color bitand Flag initialized (If they aren't already)
    for (i = 0; i < NUM_TEAMS; i++)
    {
        if (TeamInfo[i])
        {
            if ( not TeamInfo[i]->GetFlag())
                TeamInfo[i]->SetFlag(static_cast<uchar>(i));

            if ( not TeamInfo[i]->GetColor())
                TeamInfo[i]->SetColor(static_cast<uchar>(i));

            if (i and not (TeamInfo[i]->flags bitand TEAM_ACTIVE))
                TeamInfo[i]->flags or_eq TEAM_ACTIVE;
        }
    }

    update_taceng_clock();

    tactical_update_campaign_entities();

    TheCampaign.Flags or_eq CAMP_TACTICAL bitor CAMP_TACTICAL_EDIT;

    TacticalEngagementSetup(true);

    PauseTacticalEngagement();

    gGps->SetAllowed(0xffffffff);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// This basically Ends, then starts
void tactical_revert_mission(void)
{
    gMainHandler->EnterCritical();
    tactical_mission_loaded = FALSE;

    RemoveTacticalEdit();

    CleanupTacticalEngagementUI();
    TheCampaign.EndCampaign();

#ifdef CAMPTOOL

    if (gRenameIds)
        SendMessage(gMainHandler->GetAppWnd(), FM_LOAD_CAMPAIGN, 0, game_TacticalEngagement);
    else
#endif
        tactical_edit_mission(current_tactical_mission);

    ActivateTacMissionBuilder();
    gSelectedTeam = 1;
    PickTeamColors();
    gMainHandler->LeaveCritical();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void update_taceng_clock(void)
{
    CampaignTime
    time,
    hrs,
    min,
    sec;

    C_Window
    *win;

    C_Clock
    *clk;

    win = gMainHandler->FindWindow(TAC_TIME);

    if ( not win)
    {
        return;
    }

    clk = (C_Clock *) win->FindControl(TIME_ID);

    if (clk)
    {
        time = TheCampaign.CurrentTime;

        hrs = (time / (1000 * 60 * 60)) % 24;
        min = (time / (1000 * 60)) % 60;
        sec = (time / 1000) % 60;

        clk->SetHour(hrs);
        clk->SetMinute(min);
        clk->SetSecond(sec);
        clk->Refresh();
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
