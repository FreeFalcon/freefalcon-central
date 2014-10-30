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

#include "falclib.h"
#include "vu2.h"
#include "chandler.h"
#include "ui95_ext.h"
#include "uicomms.h"
#include "userids.h"
#include "textids.h"
#include "cmpclass.h"
#include "division.h"
#include "cmap.h"
#include "tac_class.h"
#include "te_defs.h"
#include "campwp.h"
#include "gps.h"
#include "teamdata.h"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern void SetTimeCompression(int newComp);
extern void SetTime(unsigned long currentTime);
void UI_Help_Guide_CB(long ID, short hittype, C_Base *ctrl);
BOOL CampaignClockCB(C_Base *);
static void hookup_main_buttons(C_Window *win);
void OpenLogBookCB(long ID, short hittype, C_Base *ctrl);
void OpenCommsCB(long ID, short hittype, C_Base *ctrl);
void OpenSetupCB(long ID, short hittype, C_Base *ctrl);
static void TacSelectGameCB(long ID, short hittype, C_Base *ctrl);
void ACMIButtonCB(long ID, short hittype, C_Base *ctrl);
void Open_Flight_WindowCB(long ID, short hittype, C_Base *control);
void EditFlightInPackage(long ID, short hittype, C_Base *control);
void DeleteFlightFromPackage(long ID, short hittype, C_Base *control);
void tactical_cancel_package(long ID, short hittype, C_Base *ctrl);
void KeepPackage(long ID, short hittype, C_Base *control);
void SetupTeamListValues();
void SetupCurrentTeamValues(long team);
void Hookup_Team_Win(C_Window *win);
void adjust_all_taceng_unit_times(CampaignTime dt);
void fixup_unit_starting_positions(void);
void ChangeTimeCB(long ID, short hittype, C_Base *control);
void ChangeStartTimeCB(long ID, short hittype, C_Base *control);
void ChangeEndTimeCB(long ID, short hittype, C_Base *control);
void ChangeCurrentTimeCB(long ID, short hittype, C_Base *control);
void SetVCSortTypeCB(long ID, short hittype, C_Base *control);
void TimeCompressionCB(long ID, short hittype, C_Base *control);
BOOL VCSortCB(TREELIST*, TREELIST*);
void SelectToolTypeCB(long ID, short hittype, C_Base *control);
void UpdateEventBlipsCB(long ID, short hittype, C_Base *control);
void SelectMissionSortCB(long ID, short hittype, C_Base *control);

void TEDelFileCB(long ID, short hittype, C_Base *control);
void TEDelVerifyCB(long ID, short hittype, C_Base *control);
void EnableScenarioInfo(long ID);
void DisableScenarioInfo();
void UpdateOwners();

extern long OwnershipChanged;
int TacLastGroup = 0;
extern C_TreeList *TacticalGames;
extern uchar gSelectedTeam;
extern GlobalPositioningSystem *gGps;
extern C_Map *gMapMgr;
extern C_TreeList *gVCTree;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void CloseTEWin(long ID, short hittype, C_Base *base)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    DisableScenarioInfo();
    CloseWindowCB(ID, hittype, base);
}

void hookup_tactical_controls(long ID)
{
    C_Window
    *win;

    C_Button
    *ctrl;

    C_Clock
    *clk;

    C_ListBox
    *lbox;

    C_TreeList
    *tree;

    win = gMainHandler->FindWindow(ID);

    if ( not win)
    {
        return;
    }

    clk = (C_Clock *) win->FindControl(TIME_ID);

    if (clk)
    {
        clk->SetTimerCallback(CampaignClockCB);
        clk->SetFlagBitOn(C_BIT_TIMER);
        clk->Refresh();
    }

    ctrl = (C_Button *) win->FindControl(TIME_EARLIER);

    if (ctrl)
    {
        ctrl->SetCallback(ChangeCurrentTimeCB);
    }

    ctrl = (C_Button *) win->FindControl(TIME_LATER);

    if (ctrl)
    {
        ctrl->SetCallback(ChangeCurrentTimeCB);
    }

    lbox = (C_ListBox *) win->FindControl(ACCELERATION);

    if (lbox)
    {
        lbox->SetCallback(TimeCompressionCB);
    }

    ctrl = (C_Button *) win->FindControl(CLOSE_WINDOW);

    if (ctrl)
    {
        ctrl->SetCallback(CloseWindowCB);
    }

    ctrl = (C_Button *) win->FindControl(ADD_PACKAGE_FLIGHT);

    if (ctrl)
    {
        ctrl->SetCallback(Open_Flight_WindowCB);
    }

    ctrl = (C_Button *) win->FindControl(EDIT_PACKAGE_FLIGHT);

    if (ctrl)
    {
        ctrl->SetCallback(EditFlightInPackage);
    }

    ctrl = (C_Button *) win->FindControl(DELETE_PACKAGE_FLIGHT);

    if (ctrl)
    {
        ctrl->SetCallback(DeleteFlightFromPackage);
    }

    ctrl = (C_Button *) win->FindControl(CANCEL_PACK);

    if (ctrl)
    {
        ctrl->SetCallback(tactical_cancel_package);
    }

    ctrl = (C_Button *) win->FindControl(OK_PACK);

    if (ctrl)
    {
        ctrl->SetCallback(KeepPackage);
    }

    ctrl = (C_Button *) win->FindControl(START_TIME_DEC);

    if (ctrl)
    {
        ctrl->SetCallback(ChangeStartTimeCB);
    }

    ctrl = (C_Button *) win->FindControl(START_TIME_INC);

    if (ctrl)
    {
        ctrl->SetCallback(ChangeStartTimeCB);
    }

    ctrl = (C_Button *) win->FindControl(TIME_LIMIT_DEC);

    if (ctrl)
    {
        ctrl->SetCallback(ChangeEndTimeCB);
    }

    ctrl = (C_Button *) win->FindControl(TIME_LIMIT_INC);

    if (ctrl)
    {
        ctrl->SetCallback(ChangeEndTimeCB);
    }

    hookup_toolbar_buttons(win);
    hookup_list_buttons(win);
    hookup_flow_buttons(win);
    hookup_team_victory_window(win);
    hookup_map_windows(win);
    hookup_tactical_pick(win);
    hookup_edit_controls(win);
    Hookup_Team_Win(win);

    ctrl = (C_Button *)win->FindControl(SORT_PRIORITY);

    if (ctrl)
        ctrl->SetCallback(SelectMissionSortCB);

    ctrl = (C_Button *)win->FindControl(SORT_TAKEOFF);

    if (ctrl)
        ctrl->SetCallback(SelectMissionSortCB);

    ctrl = (C_Button *)win->FindControl(SORT_PACKAGE);

    if (ctrl)
        ctrl->SetCallback(SelectMissionSortCB);

    ctrl = (C_Button *)win->FindControl(SORT_ROLE);

    if (ctrl)
        ctrl->SetCallback(SelectMissionSortCB);

    ctrl = (C_Button *)win->FindControl(SORT_STATUS);

    if (ctrl)
        ctrl->SetCallback(SelectMissionSortCB);


    if (ID == NEW_SQUAD_WIN)
    {
        hookup_new_squad_window(win);
    }

    if (ID == NEW_BATT_WIN)
    {
        hookup_new_battalion_window(win);
    }

    hookup_main_buttons(win);
    // Help GUIDE thing
    ctrl = (C_Button*)win->FindControl(UI_HELP_GUIDE);

    if (ctrl)
        ctrl->SetCallback(UI_Help_Guide_CB);

    tree = (C_TreeList *)win->FindControl(TACTICAL_TREE);

    if (tree)
    {
        TacticalGames = tree;
        TacticalGames->SetCallback(TacSelectGameCB);
    }

    tree = (C_TreeList *)win->FindControl(VC_TREE);

    if (tree)
    {
        gVCTree = tree;
        tree->SetSortType(TREE_SORT_CALLBACK);
        tree->SetSortCallback(VCSortCB);
    }

    ctrl = (C_Button*)win->FindControl(SORT_VC_NUMBER);

    if (ctrl)
        ctrl->SetCallback(SetVCSortTypeCB);

    ctrl = (C_Button*)win->FindControl(SORT_VC_TEAM);

    if (ctrl)
        ctrl->SetCallback(SetVCSortTypeCB);

    ctrl = (C_Button*)win->FindControl(SORT_VC_TYPE);

    if (ctrl)
        ctrl->SetCallback(SetVCSortTypeCB);

    ctrl = (C_Button*)win->FindControl(SORT_VC_POINTS);

    if (ctrl)
        ctrl->SetCallback(SetVCSortTypeCB);

    ctrl = (C_Button*)win->FindControl(TAC_DELETE);

    if (ctrl)
        ctrl->SetCallback(TEDelVerifyCB);

    if (ID == TAC_HEADER_WIN)
    {
        ctrl = (C_Button*)win->FindControl(CLOSE_WINDOW);

        if (ctrl)
            ctrl->SetCallback(CloseTEWin);
    }

    if (ID == TAC_MISSION_SUA)
    {
        C_Blip *blip;
        C_TimerHook *tmr;
        short i, j;

        blip = new C_Blip;
        blip->Setup(9000000, 0);
        blip->SetClient(2);

        for (i = 0; i < 8; i++)
        {
            for (j = 0; j < 8; j++)
                blip->SetImage(BLIP_IDS[0][j], static_cast<uchar>(i), static_cast<uchar>(j));
        }

        blip->InitDrawer();
        win->AddControl(blip);

        tmr = new C_TimerHook;
        tmr->Setup(C_DONT_CARE, C_TYPE_TIMER);
        tmr->SetUpdateCallback(UpdateEventBlipsCB);
        tmr->SetUserNumber(_UI95_TIMER_DELAY_, 5 * _UI95_TICKS_PER_SECOND_);
        win->AddControl(tmr);
    }
}

void TestOpenCB(long, short hittype, C_Base *control)
{
    long idx, cluster;
    C_Window *win1, *win2;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    if (TacLastGroup)
        gMainHandler->DisableWindowGroup(TacLastGroup);

    TacLastGroup = control->GetGroup();
    gMainHandler->EnableWindowGroup(TacLastGroup);
    win1 = gMainHandler->FindWindow(TAC_TOOLBAR_WIN);
    win2 = gMainHandler->FindWindow(TAC_EDIT_TOOLBAR);

    //Hide Icons which shouldn't be seen with this window
    idx = 200;
    cluster = control->GetUserNumber(idx);

    while (cluster)
    {
        if (win1)
            win1->HideCluster(cluster);

        if (win2)
            win2->HideCluster(cluster);

        idx++;
        cluster = control->GetUserNumber(idx);
    }

    //Unhide Icons which should be seen with this window
    idx = 100;
    cluster = control->GetUserNumber(idx);

    while (cluster)
    {
        if (win1)
            win1->UnHideCluster(cluster);

        if (win2)
            win2->UnHideCluster(cluster);

        idx++;
        cluster = control->GetUserNumber(idx);
    }
}

void OpenTeamWindowCB(long ID, short hittype, C_Base *base)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    if (OwnershipChanged)
        UpdateOwners();

    SetupCurrentTeamValues(gSelectedTeam);
    SetupTeamListValues();
    TestOpenCB(ID, hittype, base);
}

void OpenVCWindowCB(long ID, short hittype, C_Base *base)
{
    C_Window *win;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    if (OwnershipChanged)
        UpdateOwners();

    win = gMainHandler->FindWindow(TAC_VC_WIN);

    if (win)
    {
        gMapMgr->SetWindow(win);
        SelectToolTypeCB(0, C_TYPE_LMOUSEUP, NULL);
        gMapMgr->GetCurWP()->SetFlagBitOn(C_BIT_INVISIBLE);
        TestOpenCB(ID, hittype, base);
    }
}

void OpenBuilderWindowCB(long ID, short hittype, C_Base *base)
{
    C_Window *win;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    if (OwnershipChanged)
        UpdateOwners();

    win = gMainHandler->FindWindow(TAC_EDIT_WIN);

    if (win)
    {
        gMapMgr->SetWindow(win);
        SelectToolTypeCB(0, C_TYPE_LMOUSEUP, NULL);
        gMapMgr->GetCurWP()->SetFlagBitOff(C_BIT_INVISIBLE);
        TestOpenCB(ID, hittype, base);
    }
}

void OpenMissionWindowCB(long ID, short hittype, C_Base *base)
{
    C_Window *win;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    if (OwnershipChanged)
        UpdateOwners();

    win = gMainHandler->FindWindow(TAC_PUA_MAP);

    if (win)
    {
        gMapMgr->SetWindow(win);
        SelectToolTypeCB(0, C_TYPE_LMOUSEUP, NULL);
        gMapMgr->GetCurWP()->SetFlagBitOff(C_BIT_INVISIBLE);
        TestOpenCB(ID, hittype, base);
    }
}

void hookup_main_buttons(C_Window *win)
{
    C_Button *btn;

    btn = (C_Button*)win->FindControl(BUILDER_MAIN_CTRL);

    if (btn)
        btn->SetCallback(OpenBuilderWindowCB);

    btn = (C_Button*)win->FindControl(TEAMS_MAIN_CTRL);

    if (btn)
        btn->SetCallback(OpenTeamWindowCB);

    btn = (C_Button*)win->FindControl(TAC_MISS_MAIN_CTRL);

    if (btn)
        btn->SetCallback(OpenMissionWindowCB);

    btn = (C_Button*)win->FindControl(VC_MAIN_CTRL);

    if (btn)
        btn->SetCallback(OpenVCWindowCB);

    btn = (C_Button *)win->FindControl(LB_MAIN_CTRL);

    if (btn)
        btn->SetCallback(OpenLogBookCB);

    btn = (C_Button *)win->FindControl(CO_MAIN_CTRL);

    if (btn)
        btn->SetCallback(OpenCommsCB);

    btn = (C_Button *)win->FindControl(SP_MAIN_CTRL);

    if (btn)
        btn->SetCallback(OpenSetupCB);

    btn = (C_Button *)win->FindControl(ACMI_CTRL);

    if (btn)
        btn->SetCallback(ACMIButtonCB);
}

void ChangeStartTimeCB(long ID, short hittype, C_Base *control)
{
    C_Clock *clk;
    long time, deltatime;

    if (hittype not_eq C_TYPE_LMOUSEUP and hittype not_eq C_TYPE_REPEAT)
        return;

    clk = (C_Clock*)control->Parent_->FindControl(control->GetUserNumber(0));

    if (clk)
    {
        time = clk->GetTime();
        ChangeTimeCB(ID, hittype, control);

        deltatime = (clk->GetTime() - time) * VU_TICS_PER_SECOND;

        TheCampaign.SetTEStartTime(clk->GetTime() * VU_TICS_PER_SECOND);
        // Adjust ALL times relative to the deltatime
    }
}

void ChangeEndTimeCB(long ID, short hittype, C_Base *control)
{
    C_Clock *clk;
    unsigned long time;

    if (hittype not_eq C_TYPE_LMOUSEUP and hittype not_eq C_TYPE_REPEAT)
        return;

    clk = (C_Clock*)control->Parent_->FindControl(control->GetUserNumber(0));

    if (clk)
    {
        ChangeTimeCB(ID, hittype, control);
        time = clk->GetTime() * VU_TICS_PER_SECOND;

        if (time > TheCampaign.GetTEStartTime())
        {
            MonoPrint("Set TE Time Limit %08x\n", time);
            TheCampaign.SetTETimeLimitTime(time);
        }
        else
        {
            clk->SetTime(TheCampaign.GetTEStartTime() / VU_TICS_PER_SECOND);
            clk->Refresh();
        }
    }
}

void ChangeCurrentTimeCB(long ID, short hittype, C_Base *control)
{
    C_Clock *clk;
    Unit un;

    if (hittype not_eq C_TYPE_LMOUSEUP and hittype not_eq C_TYPE_REPEAT)
        return;

    clk = (C_Clock*)control->Parent_->FindControl(control->GetUserNumber(0));

    if (clk)
    {
        ChangeTimeCB(ID, hittype, control);

        SetTime(clk->GetTime() * VU_TICS_PER_SECOND);

        fixup_unit_starting_positions();

        gGps->Update();
        un = (Unit)vuDatabase->Find(gMapMgr->GetCurWPID());

        if (un and un->IsFlight())
            gMapMgr->UpdateWaypoint((Flight)un);

        gMapMgr->DrawMap();
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static void TacSelectGameCB(long, short hittype, C_Base *control)
{
    VU_ID *tmpID;
    FalconGameEntity *game;
    TREELIST *item;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    MonoPrint("Select TAC Online Game\n");

    if (current_tactical_mission)
    {
        delete current_tactical_mission;
    }

    current_tactical_mission = new tactical_mission;

    SetCursor(gCursors[CRSR_WAIT]);
    item = ((C_TreeList *)control)->GetLastItem();

    if (item == NULL) return;

    if (item->Item_ == NULL) return;

    if (item->Type_ == C_TYPE_MENU)
    {
        if ( not item->Item_->GetState())
        {
            item->Item_->SetState(1);
            item->Item_->Refresh();
            tmpID = (VU_ID *)item->Item_->GetUserPtr(_UI95_VU_ID_SLOT_);

            if (tmpID)
            {
                game = (FalconGameEntity*) vuDatabase->Find(*tmpID);
                gCommsMgr->LookAtGame(game);

                if (game)
                {
                    if (game->GetGameType() == game_TacticalEngagement)
                        SendMessage(gMainHandler->GetAppWnd(), FM_JOIN_CAMPAIGN, JOIN_PRELOAD_ONLY, game_TacticalEngagement);
                }
            }
        }
        else
        {
            item->Item_->SetState(0);
            item->Item_->Refresh();
            gCommsMgr->LookAtGame(NULL);
        }
    }

    //SetCursor(gCursors[CRSR_F16]);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void adjust_all_taceng_unit_times(CampaignTime dt)
{
    VuListIterator myit(AllRealList);

    Unit un;
    WayPointClass *wp;
    un = GetFirstUnit(&myit);

    while (un)
    {
        wp = un->GetFirstUnitWP();

        while (wp)
        {
            wp->AddWPTimeDelta(dt);
            wp = wp->GetNextWP();
        }

        un = GetNextUnit(&myit);
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int tactical_is_training(void)
{
    if (current_tactical_mission and current_tactical_mission->get_type() == tt_training)
        return TRUE;

    return FALSE;
}

