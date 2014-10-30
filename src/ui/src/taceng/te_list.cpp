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
#include "teamdata.h"
#include "f4find.h"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#define TAC_JOIN 30208

#ifdef _DEBUG
extern HWND mainAppWnd;
#endif
#ifdef CAMPTOOL
// Renaming tool stuff
extern int gRenameIds;
#endif CAMPTOOL

extern uchar gSelectedTeam;

_TCHAR gLastTEFilename[MAX_PATH];
_TCHAR gLastTEFile[MAX_PATH]; // without path

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SetDeleteCallback(void (*cb)(long, short, C_Base*));
void tactical_select_training(long, short, C_Base *);
static void update_sua_load_list(void);
static void update_pua_list(void);
void tactical_edit_mission(tactical_mission *);
void ActivateTacMissionBuilder();
BOOL CheckExclude(_TCHAR *filename, _TCHAR *directory, _TCHAR *ExcludeList[], _TCHAR *extension);
void VerifyDelete(long TitleID, void (*YesCB)(long, short, C_Base*), void (*NoCB)(long, short, C_Base*));
int tactical_is_training(void);
void SetupOccupationMap(void);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void CampaignListCB();
void GetMissionTarget(Package curpackage, Flight curflight, _TCHAR Buffer[]);
void TacEngListCB(void);
void RefreshMapEventList(long winID, long client);
void CleanupTacticalEngagementUI(void);
_TCHAR *UI_WordWrap(C_Window *win, _TCHAR *str, long fontid, short width, BOOL *status);
void GetFileListTree(C_TreeList *tree, _TCHAR *fspec, _TCHAR *excludelist[], long group, BOOL cutext, long UseMenu);
BOOL FileNameSortCB(TREELIST *list, TREELIST *newitem);
void EnableScenarioInfo(long ID);
void DisableScenarioInfo();
void DelSTRFileCB(long ID, short hittype, C_Base *control);
void DelDFSFileCB(long ID, short hittype, C_Base *control);
void DelLSTFileCB(long ID, short hittype, C_Base *control);
void DelCamFileCB(long ID, short hittype, C_Base *control);
void DelTacFileCB(long ID, short hittype, C_Base *control);
void DelTGAFileCB(long ID, short hittype, C_Base *control);
void DelVHSFileCB(long ID, short hittype, C_Base *control);
void DelKeyFileCB(long ID, short hittype, C_Base *control);

_TCHAR *TEExcludeList[] =
{
    "te_new",
    NULL,
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

tactical_mission
*current_tactical_mission = NULL;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern VU_ID gCurrentFlightID;
extern C_Map
*gMapMgr;

extern long TeamBtnIDs[NUM_TEAMS];
extern long TeamLineIDs[NUM_TEAMS];

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void create_tactical_scenario_info(void)
{
    TheCampaign.Flags or_eq CAMP_TACTICAL;

    update_sua_load_list();

    update_pua_list();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static void tactical_mission_selection(int hack_training)
{
    current_tactical_mission->preload();

    if (hack_training)
    {
        current_tactical_mission->set_type(tt_training);
    }

    update_sua_load_list();

    update_pua_list();

#ifdef _DEBUG
    PostMessage(mainAppWnd, FM_GIVE_FOCUS, NULL, NULL);
#endif

}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void TE_LoadMissionCB(long, short hittype, C_Base *control)
{
    C_TreeList *tree;
    TREELIST *item;
    C_Button   *btn;
    _TCHAR buffer[MAX_PATH];

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    tree = (C_TreeList*)control;

    if (tree)
    {
        item = tree->GetLastItem();

        if (item)
        {
            btn = (C_Button*)item->Item_;

            if (btn)
            {
                tree->SetAllControlStates(0, tree->GetRoot());
                btn->SetState(1);
                tree->Refresh();

                strcpy(buffer, FalconCampaignSaveDirectory);
                strcat(buffer, "\\");
                strcat(buffer, btn->GetText(0));
                strcat(buffer, ".tac");

                strcpy(gLastTEFilename, buffer);
                strcpy(gLastTEFile, btn->GetText(0));

                if (current_tactical_mission)
                {
                    delete current_tactical_mission;
                }

                current_tactical_mission = new tactical_mission(buffer);

                tactical_mission_selection(FALSE);
            }
        }
    }
}

void TE_LoadTrainingMissionCB(long, short hittype, C_Base *control)
{
    C_TreeList *tree;
    TREELIST *item;
    C_Button   *btn;
    _TCHAR buffer[MAX_PATH];

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    tree = (C_TreeList*)control;

    if (tree)
    {
        item = tree->GetLastItem();

        if (item)
        {
            btn = (C_Button*)item->Item_;

            if (btn)
            {
                tree->SetAllControlStates(0, tree->GetRoot());
                btn->SetState(1);
                tree->Refresh();

                strcpy(buffer, FalconCampaignSaveDirectory);
                strcat(buffer, "\\");
                strcat(buffer, btn->GetText(0));
                strcat(buffer, ".trn");

                if (current_tactical_mission)
                {
                    delete current_tactical_mission;
                }

                current_tactical_mission = new tactical_mission(buffer);

                tactical_mission_selection(TRUE);
            }
        }
    }
}

void GetTrainingFileList()
{
    C_Window *win;
    C_TreeList *tree;

    win = gMainHandler->FindWindow(TAC_MISSION_WIN);

    if (win)
    {
        tree = (C_TreeList *)win->FindControl(TRAINLIST_TREE);

        if (tree)
        {
            tree->DeleteBranch(tree->GetRoot());
            tree->SetUserNumber(0, 1);
            tree->SetSortType(TREE_SORT_CALLBACK);
            tree->SetSortCallback(FileNameSortCB);
            tree->SetCallback(TE_LoadTrainingMissionCB);
            char path[_MAX_PATH];
            sprintf(path, "%s\\*.TRN", FalconCampaignSaveDirectory);

            GetFileListTree(tree, path, NULL, C_TYPE_ITEM, TRUE, 0);
            tree->RecalcSize();
            win->RefreshClient(tree->GetClient());
        }
    }

    gLastTEFilename[0] = 0;
    gLastTEFile[0] = 0;
}

void GetTacticalFileList()
{
    C_Window *win;
    C_TreeList *tree;


    win = gMainHandler->FindWindow(TAC_MISSION_WIN);

    if (win)
    {
        tree = (C_TreeList *)win->FindControl(FILELIST_TREE);

        if (tree)
        {
            tree->DeleteBranch(tree->GetRoot());
            tree->SetUserNumber(0, 1);
            tree->SetSortType(TREE_SORT_CALLBACK);
            tree->SetSortCallback(FileNameSortCB);
            tree->SetCallback(TE_LoadMissionCB);
            char path[_MAX_PATH];
            sprintf(path, "%s\\*.TAC", FalconCampaignSaveDirectory);

            GetFileListTree(tree, path, TEExcludeList, C_TYPE_ITEM, TRUE, 0);
            tree->RecalcSize();
            win->RefreshClient(tree->GetClient());
        }
    }

    gLastTEFilename[0] = 0;
    gLastTEFile[0] = 0;
}


void TEDelFileCB(long, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    gMainHandler->HideWindow(control->Parent_); // Close Verify Window

    if ( not CheckExclude(gLastTEFilename, FalconCampUserSaveDirectory, TEExcludeList, "tac"))
        DeleteFile(gLastTEFilename);

    gLastTEFilename[0] = 0;
    gLastTEFile[0] = 0;

    if (current_tactical_mission)
    {
        delete current_tactical_mission;
        current_tactical_mission = NULL;
    }

    DisableScenarioInfo();
    GetTacticalFileList();
}

void TEDelVerifyCB(long, short hittype, C_Base *)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    if (gLastTEFilename[0])
        VerifyDelete(0, TEDelFileCB, CloseWindowCB);
}

void tac_flag_btn_cb(long, short hittype, C_Base *ctrl)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    if (current_tactical_mission)
    {
        if (ctrl->GetState())
        {
            current_tactical_mission->set_flag(ctrl->GetUserNumber(0));
        }
        else
        {
            current_tactical_mission->clear_flag(ctrl->GetUserNumber(0));
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void choose_eng_type_cb(long , short hittype, C_Base *ctrl)
{
    if (hittype not_eq C_TYPE_SELECT)
        return;

    short type = static_cast<short>(((C_ListBox *)ctrl)->GetTextID());

    switch (type)
    {
            //need to set engagement type appropriately
        case TYPE_CONTINUOUS:
            current_tactical_mission->set_type(tt_engagement);
            break;

        case TYPE_SINGLE:
            current_tactical_mission->set_type(tt_single);
            break;

        case TYPE_TRAINING:
            current_tactical_mission->set_type(tt_training);
            break;
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


void hookup_edit_controls(C_Window *win)
{
    if ( not win)
        return;

    C_Button *btn;
    C_ListBox *lbox;

    lbox = (C_ListBox *)win->FindControl(TAC_TYPE);

    if (lbox)
    {
        lbox->SetCallback(choose_eng_type_cb);
    }

    btn = (C_Button *)win->FindControl(HIDE_ENEMY);

    if (btn)
    {
        btn->SetUserNumber(0, tf_hide_enemy);
        btn->SetCallback(tac_flag_btn_cb);
    }

    btn = (C_Button *)win->FindControl(LOCK_OOB);

    if (btn)
    {
        btn->SetUserNumber(0, tf_lock_oob);
        btn->SetCallback(tac_flag_btn_cb);
    }

    btn = (C_Button *)win->FindControl(LOCK_ATO);

    if (btn)
    {
        btn->SetUserNumber(0, tf_lock_ato);
        btn->SetCallback(tac_flag_btn_cb);
    }

    btn = (C_Button *)win->FindControl(FROZEN_START);

    if (btn)
    {
        btn->SetUserNumber(0, tf_start_paused);
        btn->SetCallback(tac_flag_btn_cb);
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void hookup_list_buttons(C_Window *winme)
{
    C_Button
    *ctrl;

    // Training Button
    ctrl = (C_Button *) winme->FindControl(TAC_TRAIN_CTRL);

    if (ctrl)
    {
        ctrl->SetCallback(tactical_select_training);
        ctrl->SetState(1);
    }

    // Load Button
    ctrl = (C_Button *) winme->FindControl(TAC_LOAD_CTRL);

    if (ctrl)
    {
        ctrl->SetCallback(tactical_select_load);
        ctrl->SetState(0);
    }

    // Join Button
    ctrl = (C_Button *) winme->FindControl(TAC_JOIN_CTRL);

    if (ctrl)
    {
        ctrl->SetCallback(tactical_select_join);
        ctrl->SetState(0);
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#if 0

void create_tactical_list(void)
{
    C_Window
    *win;

    C_Button
    *but;

    int
    group,
    loop,
    y;

    long
    id;

    char
    buffer[100];

    tactical_mission
    *miss;

    y = 0;
    id = 3000000;

    win = gMainHandler->FindWindow(TAC_MISSION_WIN);

    if ( not win)
    {
        MonoPrint("Cannot Find TAC_MISSION_WIN\n");
        return;
    }


    current_tactical_mission = NULL;

    DeleteGroupList(TAC_MISSION_WIN);

    miss = tactical_mission::get_first_mission(current_tactical_mode);

    for (loop = 0; miss; loop ++)
    {
        if (current_tactical_mode == tm_load)
        {
            strcpy(buffer, miss->get_title());
            group = 3001;
        }
        else if (current_tactical_mode == tm_join)
        {
            sprintf(buffer, "Join %d", loop);
            group = 3002;
        }
        else
        {
            strcpy(buffer, miss->get_title());
            group = 3003;
        }

        but = new C_Button;

        but->Setup(id, C_TYPE_RADIO, 0, y);
        but->SetFlagBitOn(C_BIT_ENABLED);

        but->SetText(C_STATE_0, buffer);
        but->SetText(C_STATE_1, buffer);
        but->SetFont(win->Font_);

        but->SetUserNumber(0, (int) miss);
        but->SetUserNumber(_UI95_DELGROUP_SLOT_, _UI95_DELGROUP_ID_);

        but->SetFgColor(C_STATE_0, 0x00e0e0e0);
        but->SetFgColor(C_STATE_1, 0x0000ff00);
        but->SetGroup(group);

        if (loop == 0)
        {
            current_tactical_mission = miss;

            but->SetState(1);
        }
        else
        {
            but->SetState(0);
        }

        but->SetClient(1);
        but->SetCallback(tactical_mission_selection);

        win->AddControl(but);

        but->SetHotSpot
        (
            0,
            0,
            win->ClientArea_[1].right - win->ClientArea_[1].left,
            but->GetH()
        );

        id ++;
        y += but->GetH();

        miss = tactical_mission::get_next_mission();
    }

#if 0

    if (current_tactical_mode == tm_load)
    {
        but = new C_Button;

        but->Setup(id, C_TYPE_RADIO, 0, y);
        but->SetFlagBitOn(C_BIT_ENABLED);

        but->SetText(C_STATE_0, "[NEW MISSION]");
        but->SetText(C_STATE_1, "[NEW MISSION]");
        but->SetFont(win->Font_);

        if (loop == 0)
        {
            current_tactical_mission = NULL;

            but->SetState(1);
        }
        else
        {
            but->SetState(0);
        }

        but->SetUserNumber(0, 0);
        but->SetUserNumber(_UI95_DELGROUP_SLOT_, _UI95_DELGROUP_ID_);

        but->SetFgColor(C_STATE_0, 0x00e0e0e0);
        but->SetFgColor(C_STATE_1, 0x0000ff00);
        but->SetGroup(-10);

        but->SetClient(1);
        but->SetCallback(tactical_mission_selection);

        win->AddControl(but);

        but->SetHotSpot
        (
            0,
            0,
            win->ClientArea_[1].right - win->ClientArea_[1].left,
            but->GetH()
        );

        id ++;
        y += but->GetH();
    }

#endif
    current_tactical_mission->preload();

    update_sua_load_list();

    update_pua_list();

    win->ScanClientAreas();
    win->RefreshWindow();
}

#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void tactical_team_selection(long, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    if (current_tactical_mission)
    {
        current_tactical_mission->set_team(control->GetUserNumber(0));
        update_pua_list();
    }
}

void JoinTacTeamCB(long, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    if (control)
        gSelectedTeam = static_cast<uchar>(control->GetUserNumber(0));

    current_tactical_mission->set_team(gSelectedTeam);
    update_pua_list();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static void update_sua_load_list(void)
{
    C_Window *win;
    C_Button *but;
    C_Line *line;

    int
    id,
    loop,
    MaxTeams;

    // ONLY show Team 1 if a training mission
    if (current_tactical_mission->get_type() == tt_training)
        MaxTeams = 2;
    else
        MaxTeams = NUM_TEAMS;

    win = gMainHandler->FindWindow(TAC_SUA_WIN);

    if (current_tactical_mission and win)
    {
        gSelectedTeam = 0;
        //gSelectedTeam=-1;
        current_tactical_mission->set_team(0);
        id = 0;

        for (loop = 1; loop < MaxTeams; loop++)
        {
            if (TheCampaign.team_flags[loop])
            {
                but = (C_Button*)win->FindControl(TeamBtnIDs[id]);

                if (but)
                {
                    but->SetImage(0, FlagImageID[TheCampaign.team_flags[loop]][SMALL_HORIZ]);
                    but->SetImage(1, FlagImageID[TheCampaign.team_flags[loop]][SMALL_HORIZ]);
                    but->SetAllLabel((_TCHAR*)TheCampaign.team_name[loop]);
                    but->SetCallback(JoinTacTeamCB);
                    but->SetUserNumber(0, loop);
                    but->SetUserNumber(1, id);

                    if (gSelectedTeam < 1)
                        gSelectedTeam = static_cast<uchar>(loop);
                }

                line = (C_Line*)win->FindControl(TeamLineIDs[id]);

                if (line)
                {
                    if (TheCampaign.team_flags[loop])
                    {
                        line->SetColor(TeamColorList[TheCampaign.team_colour[loop]]);;
                    }
                }

                win->UnHideCluster(id + 1);
                id++;
            }
        }

        while (id < NUM_TEAMS)
        {
            win->HideCluster(id + 1);
            id++;
        }

        // Do Map Overlay
        SetupOccupationMap();
        EnableScenarioInfo(3050);
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static void update_pua_list(void)
{
    C_Window
    *win;

    C_Text
    *txt;

    C_Button
    *btn;

    char
    buffer[100],
           *text;

    win = gMainHandler->FindWindow(TAC_PUA_WIN);

    if (current_tactical_mission)
    {
        txt = (C_Text *) win->FindControl(MISSION_NAME);

        if (txt)
        {
            text = current_tactical_mission->get_title();

            if (text)
            {
                txt->SetText(text);
            }
            else
            {
                txt->SetText("");
            }

            txt->Refresh();
        }

        txt = (C_Text *) win->FindControl(TAC_TEAMS);

        if (txt)
        {
            sprintf
            (
                buffer, "%d",
                current_tactical_mission->get_number_of_teams()
            );

            txt->SetText(buffer);
        }


        txt = (C_Text *) win->FindControl(TAC_F16S);

        if (txt)
        {
            sprintf
            (
                buffer, "%d",
                current_tactical_mission->get_number_of_f16s(gSelectedTeam)
            );

            txt->SetText(buffer);
        }


        txt = (C_Text *) win->FindControl(TAC_AIRCRAFT);

        if (txt)
        {
            sprintf
            (
                buffer, "%d",
                current_tactical_mission->get_number_of_aircraft(gSelectedTeam)
            );

            txt->SetText(buffer);
        }

        txt = (C_Text *) win->FindControl(TAC_PLAYERS);

        if (txt)
        {
            sprintf
            (
                buffer, "%d",
                current_tactical_mission->get_number_of_players(gSelectedTeam)
            );

            txt->SetText(buffer);
        }

        txt = (C_Text *) win->FindControl(TAC_TEAM_NAME);

        if (txt)
        {
            txt->SetText((_TCHAR*)TheCampaign.team_name[gSelectedTeam]);
            txt->Refresh();
        }

        btn = (C_Button *)win->FindControl(TAC_FLAG);

        if (btn)
        {
            btn->SetImage(0, FlagImageID[TheCampaign.team_flags[gSelectedTeam]][BIG_HORIZ]);
            btn->Refresh();
        }

        txt = (C_Text*)win->FindControl(UNIT_MOTTO);

        if (txt)
        {
            if (tactical_is_training())
            {
                long textid = TRN_MISSION_01 + atol(current_tactical_mission->get_title()) - 1;

                if (textid >= TRN_MISSION_01 and textid <= TRN_MISSION_31)
                    txt->SetText(textid);
                else
                    txt->SetText((char*)TheCampaign.team_motto[gSelectedTeam]);
            }
            else
            {
                txt->SetText((char*)TheCampaign.team_motto[gSelectedTeam]);
            }
        }
    }
    else
    {
        txt = (C_Text *) win->FindControl(MISSION_NAME);

        if (txt)
        {
            txt->SetText("No Mission");
        }

        txt = (C_Text *) win->FindControl(TAC_TEAMS);

        if (txt)
        {
            txt->SetText("0");
        }

        txt = (C_Text *) win->FindControl(TAC_F16S);

        if (txt)
        {
            txt->SetText("0");
        }

        txt = (C_Text *) win->FindControl(TAC_AIRCRAFT);

        if (txt)
        {
            txt->SetText("0");
        }

        txt = (C_Text *) win->FindControl(TAC_PLAYERS);

        if (txt)
        {
            txt->SetText("0");
        }

        txt = (C_Text *) win->FindControl(TAC_TEAM_NAME);

        if (txt)
        {
            txt->SetText(" ");
        }

        txt = (C_Text *) win->FindControl(TAC_TEAM_NAME2);

        if (txt)
        {
            txt->SetText(" ");
        }

        btn = (C_Button *)win->FindControl(TAC_FLAG);

        if (btn)
        {
            btn->SetState(0);
        }
    }

    win->RefreshWindow();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void update_missions_details(long winID)
{
    Package curpackage;
    Flight curflight, flight;
    WayPoint wp;
    _TCHAR Task[200];
    _TCHAR Mission[200];
    _TCHAR TOT[200];
    _TCHAR Buffer[200];
    C_Text *text;

    C_Window *win = gMainHandler->FindWindow(winID);

    if ( not win)
        return;

    if (winID == TAC_AIRCRAFT)
    {
        text = (C_Text *)win->FindControl(TAC_MISS_TITLE);

        if (text)
        {
            text->Refresh();

            if (current_tactical_mission)
                text->SetText(current_tactical_mission->get_title());
            else
                text->SetText("No Mission");//should NEVER happen

            text->Refresh();
        }

        text = (C_Text *)win->FindControl(TEAM_NAME);

        if (text)
        {
            text->Refresh();

            if (current_tactical_mission)
                text->SetText(current_tactical_mission->get_team_name(current_tactical_mission->get_team()));
            else
                text->SetText("No Mission");//should NEVER happen

            text->Refresh();
        }
    }

    curflight = (Flight)vuDatabase->Find(gCurrentFlightID);

    if (curflight)
        curpackage = (Package)curflight->GetUnitParent();
    else
        curpackage = NULL;

    if (curpackage)
    {
        // Get Task string
        flight = (Flight)curpackage->GetFirstUnitElement();

        if (flight)
        {
            // Get Task string
            _tcscpy(Task, MissStr[flight->GetUnitMission()]);
        }
        else
        {
            _tcscpy(Task, " ");
        }

        // Get Mission string
        if (curflight->GetUnitMission() not_eq AMIS_ABORT)
        {
            GetMissionTarget(curpackage, curflight, Buffer);

            _tcscpy(Mission, Buffer);

            // Time on Target (TOT)
            wp = curflight->GetFirstUnitWP();

            _tcscpy(TOT, " ");

            while (wp)
            {
                if (wp->GetWPFlags() bitand WPF_TARGET)
                {
                    GetTimeString(wp->GetWPArrivalTime(), TOT);
                    wp = NULL;
                }
                else
                    wp = wp->GetNextWP();
            }
        }
        else
        {
            _tcscpy(Task, gStringMgr->GetString(TXT_RETURN_TO_BASE));
            _tcscpy(Mission, gStringMgr->GetString(TXT_MISSION_ABORTED));
            _tcscpy(TOT, gStringMgr->GetString(TXT_ABORTED));
        }
    }
    else
    {
        _tcscpy(Task, "");
        _tcscpy(Mission, "");
        _tcscpy(TOT, "");
    }




    text = (C_Text *)win->FindControl(MISSION_FIELD);

    if (text)
    {
        text->Refresh();
        text->SetText(Task);
        text->Refresh();
    }

    text = (C_Text *)win->FindControl(TGT_FIELD);

    if (text)
    {
        text->Refresh();
        text->SetText(Mission);
        text->Refresh();
    }

    text = (C_Text *)win->FindControl(TOT_FIELD);

    if (text)
    {
        text->Refresh();
        text->SetText(TOT);
        text->Refresh();
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void tactical_select_join(long, short hittype, C_Base *ctrl)
{
    C_Window
    *win;

    if (hittype not_eq C_TYPE_LMOUSEUP)
    {
        return;
    }

    DisableScenarioInfo();

    if ( not gCommsMgr->Online())
        gMainHandler->EnableWindowGroup(6001);

    ctrl->Parent_->HideCluster(ctrl->GetUserNumber(1));
    ctrl->Parent_->HideCluster(ctrl->GetUserNumber(2));
    ctrl->Parent_->UnHideCluster(ctrl->GetUserNumber(0));

    if ( not gCommsMgr->Online())
    {
        win = gMainHandler->FindWindow(PB_WIN);

        if (win)
        {
            gMainHandler->EnableWindowGroup(win->GetGroup());
        }
    }

    gLastTEFilename[0] = 0;
    gLastTEFile[0] = 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void tactical_select_load(long, short hittype, C_Base *ctrl)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
    {
        return;
    }

    DisableScenarioInfo();

    ctrl->Parent_->HideCluster(ctrl->GetUserNumber(1));
    ctrl->Parent_->HideCluster(ctrl->GetUserNumber(2));
    ctrl->Parent_->UnHideCluster(ctrl->GetUserNumber(0));

    GetTacticalFileList();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void tactical_select_training(long, short hittype, C_Base *ctrl)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
    {
        return;
    }

    DisableScenarioInfo();

    ctrl->Parent_->HideCluster(ctrl->GetUserNumber(1));
    ctrl->Parent_->HideCluster(ctrl->GetUserNumber(2));
    ctrl->Parent_->UnHideCluster(ctrl->GetUserNumber(0));

    GetTrainingFileList();
}

