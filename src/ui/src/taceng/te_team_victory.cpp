//////////////////////////////////////////////////////////////////////////////
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
#include "unit.h"
#include "team.h"
#include "timerthread.h"
#include "CmpGlobl.h"
#include "CampCell.h"
#include "CampTerr.h"
#include "find.h"
#include "division.h"
#include "cmap.h"
#include "flight.h"
#include "campwp.h"
#include "cmpclass.h"
#include "Listadt.h"
#include "objectiv.h"
#include "feature.h"
#include "Campaign.h"
#include "classtbl.h"
#include "falcsess.h"
#include "tac_class.h"
#include "gps.h"
#include "urefresh.h"
#include "te_defs.h"
#include "teamdata.h"
#include "sim/include/otwdrive.h"
#include "campmap.h"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#define BID_DROPDOWN 50300

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static void adjust_team_name(long ID, short hittype, C_Base *ctrl);
static void select_team_name(long ID, short hittype, C_Base *ctrl);
static void adjust_flag(long ID, short hittype, C_Base *ctrl);

static void show_all_vc(long ID, short hittype, C_Base *ctrl);
static void show_team_vc(long ID, short hittype, C_Base *ctrl);
static void show_achieved_vc(long ID, short hittype, C_Base *ctrl);
static void show_remaining_vc(long ID, short hittype, C_Base *ctrl);

static void new_victory_condition(long ID, short hittype, C_Base *ctrl);

static void change_vc_team_name(long ID, short hittype, C_Base *ctrl);
static void change_vc_team_action(long ID, short hittype, C_Base *ctrl);
static void change_vc_tolerance(long ID, short hittype, C_Base *ctrl);
static void change_vc_steerpoint(long ID, short hittype, C_Base *ctrl);
static void delete_current_vc(long ID, short hittype, C_Base *ctrl);

static void set_points_required_for_victory(long ID, short hittype, C_Base *ctrl);
static void delete_tactical_object(long ID, short hittype, C_Base *ctrl);
static void set_vc_points(long ID, short hittype, C_Base *ctrl);
static void add_vc_air_unit(long ID, short hittype, C_Base *ctrl);
static void update_victory_team_window(void);
void BuildSpecificTargetList(VU_ID targetID);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void delete_all_units_for_team(int team);
void tactical_remove_squadron(SquadronClass *);
void InitVCArgLists();
void SelectToolTypeCB(long ID, short hittype, C_Base *control);
void CloseReconWindowCB(long ID, short hittype, C_Base *control);
extern int check_victory_conditions(void);
void CancelCampaignCompression(void);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
extern C_Handler *gMainHandler;

extern VU_ID
gSelectedFlightID;
extern long gRefreshScoresList;

extern C_Map
*gMapMgr;

C_TreeList *gVCTree = NULL;
long VCSortType = 0;
long ShowGameOverWindow = 0;

static int
team_victory_id = 3000000,
team_mapping[8];

static C_Box
*team_colour_box[8] = {NULL};

extern uchar gSelectedTeam;

static C_EditBox
*team_name[8] = {NULL};

static C_Button
*new_vc_button = NULL,
 *show_all_button = NULL,
  *show_team_button = NULL,
   *show_achieved_button = NULL,
    *show_remaining_button = NULL,
     *new_team_button[8] = {NULL},
                           *team_flag_box[8] = {NULL};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// Targetting stuff
extern void (*OldReconCWCB)(long, short, C_Base*);
extern VU_ID FeatureID;
extern long FeatureNo;
extern C_TreeList *TargetTree;
void TgtAssignCWCB(long, short, C_Base*);

extern GlobalPositioningSystem
*gGps;

LISTBOX *team_lbox = NULL;
LISTBOX *action_lbox = NULL;
LISTBOX *percent_lbox = NULL;
LISTBOX *intercept_lbox = NULL;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void hookup_team_buttons
(
    C_Window *winme,
    int team,
    long colour_id,
    long new_team_id,
    long team_id,
    long flag_id
)
{
    C_Button
    *ctrl;

    C_PopupList
    *popup;

    C_EditBox
    *editbox;

    C_Box
    *box;

    box = (C_Box *) winme->FindControl(colour_id);

    if (box)
    {
        team_colour_box[team] = box;
    }

    ctrl = (C_Button *) winme->FindControl(new_team_id);

    if (ctrl)
    {
        new_team_button[team] = ctrl;
    }

    editbox = (C_EditBox *) winme->FindControl(team_id);

    if (editbox)
    {
        team_name[team] = editbox;
    }

    ctrl = (C_Button *) winme->FindControl(flag_id);

    if (ctrl)
    {
        team_flag_box[team] = ctrl;
    }

    popup = gPopupMgr->GetMenu(DELETE_POPUP);

    if (popup)
    {
        popup->SetCallback(LIST_DELETE, delete_tactical_object);
    }

    popup = gPopupMgr->GetMenu(AIRUNIT_MENU);

    if (popup)
    {
        popup->SetCallback(LIST_VC, add_vc_air_unit);
        popup->SetCallback(LIST_DELETE, delete_tactical_object);
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void hookup_team_victory_window(C_Window *winme)
{
    C_Button *ctrl;
    C_EditBox *editbox;

    ctrl = (C_Button *) winme->FindControl(NEW_VC);

    if (ctrl)
        ctrl->SetCallback(new_victory_condition);

    ctrl = (C_Button *) winme->FindControl(TAC_DELETE_VC);

    if (ctrl)
        ctrl->SetCallback(delete_current_vc);

    editbox = (C_EditBox *) winme->FindControl(PTS_REQ_VICTORY);

    if (editbox)
        editbox->SetCallback(set_points_required_for_victory);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static LISTBOX *mk_item(long ID, long TextID)
{
    LISTBOX *lbitem;

    lbitem = new LISTBOX;
    lbitem->Label_ = new C_Button;
    lbitem->Label_->Setup(ID, C_TYPE_RADIO, 0, 0);
    lbitem->Label_->SetText(C_STATE_0, TextID);
    lbitem->Label_->SetText(C_STATE_1, TextID);
    lbitem->Label_->SetGroup(5551212);
    lbitem->Next = NULL;

    return(lbitem);
}

static LISTBOX *mk_item(long ID, _TCHAR *Text)
{
    LISTBOX *lbitem;

    lbitem = new LISTBOX;
    lbitem->Label_ = new C_Button;
    lbitem->Label_->Setup(ID, C_TYPE_RADIO, 0, 0);
    lbitem->Label_->SetText(C_STATE_0, Text);
    lbitem->Label_->SetText(C_STATE_1, Text);
    lbitem->Label_->SetGroup(5551212);
    lbitem->Next = NULL;

    return(lbitem);
}

void RebuildTeamLists()
{
    victory_condition *vc = NULL;
    LISTBOX *last = NULL, *item = NULL, *cur = NULL;
    C_Victory *vctrl = NULL;
    short i = 0;

    gMainHandler->EnterCritical();

    cur = team_lbox;

    while (cur)
    {
        last = cur;
        cur = cur->Next;
        last->Label_->Cleanup();
        delete last->Label_;
        delete last;
    }

    team_lbox = NULL;
    last = NULL;

    for (i = 0; i < NUM_TEAMS; i++)
    {
        if (TeamInfo[i] and (TeamInfo[i]->flags bitand TEAM_ACTIVE))
            item = mk_item(i, TeamInfo[i]->GetName());
        else
        {
            item = mk_item(i, "No Team");
            item->Label_->SetFlagBitOn(C_BIT_INVISIBLE);
        }

        if ( not team_lbox)
            team_lbox = item;
        else
            last->Next = item;

        last = item;
    }

    vc = current_tactical_mission->get_first_unfiltered_victory_condition();

    while (vc)
    {
        vctrl = (C_Victory*)vc->control;

        if (vctrl)
            vctrl->GetTeam()->SetRoot(team_lbox);

        vc = current_tactical_mission->get_next_unfiltered_victory_condition();
    }

    gMainHandler->LeaveCritical();
}

void InitVCArgLists()
{
    LISTBOX *last = NULL, *item = NULL;
    short i = 0;

    if ( not team_lbox)
    {
        for (i = 0; i < NUM_TEAMS; i++)
        {
            if (TeamInfo[i] and (TeamInfo[i]->flags bitand TEAM_ACTIVE))
                item = mk_item(i, TeamInfo[i]->GetName());
            else
            {
                item = mk_item(i, "No Team");
                item->Label_->SetFlagBitOn(C_BIT_INVISIBLE);
            }

            if ( not team_lbox)
                team_lbox = item;
            else
                last->Next = item;

            last = item;
        }
    }

    if ( not action_lbox)
    {
        last = mk_item(vt_occupy, TXT_OCCUPY);
        action_lbox = last;
        item = mk_item(vt_destroy, TXT_DESTROY);
        last->Next = item;
        last = item;
        item = mk_item(vt_attrit, TXT_ATTRIT);
        last->Next = item;
        last = item;
        item = mk_item(vt_intercept, TXT_INTERCEPT);
        last->Next = item;
        last = item;
        item = mk_item(vt_degrade, TXT_DEGRADE);
        last->Next = item;
    }

    // 10 -> 100 %
    if ( not percent_lbox)
    {
        last = mk_item(1, TXT_TEN_PERC);
        percent_lbox = last;
        item = mk_item(2, TXT_TWENTY_PERC);
        last->Next = item;
        last = item;
        item = mk_item(3, TXT_THIRTY_PERC);
        last->Next = item;
        last = item;
        item = mk_item(4, TXT_FORTY_PERC);
        last->Next = item;
        last = item;
        item = mk_item(5, TXT_FIFTY_PERC);
        last->Next = item;
        last = item;
        item = mk_item(6, TXT_SIXTY_PERC);
        last->Next = item;
        last = item;
        item = mk_item(7, TXT_SEVENTY_PERC);
        last->Next = item;
        last = item;
        item = mk_item(8, TXT_EIGHTY_PERC);
        last->Next = item;
        last = item;
        item = mk_item(9, TXT_NINETY_PERC);
        last->Next = item;
        last = item;
        item = mk_item(10, TXT_HUNDRED_PERC);
        last->Next = item;
    }

    if ( not intercept_lbox)
    {
        last = mk_item(1, TXT_ONE);
        intercept_lbox = last;
        item = mk_item(2, TXT_TWO);
        last->Next = item;
        last = item;
        item = mk_item(3, TXT_THREE);
        last->Next = item;
        last = item;
        item = mk_item(4, TXT_FOUR);
        last->Next = item;
    }

}

void CleanupVCArgLists()
{
    LISTBOX *cur, *last;

    if (gVCTree)
        gVCTree->DeleteBranch(gVCTree->GetRoot());

    cur = team_lbox;

    while (cur)
    {
        last = cur;
        cur = cur->Next;
        last->Label_->Cleanup();
        delete last->Label_;
        delete last;
    }

    team_lbox = NULL;

    cur = action_lbox;

    while (cur)
    {
        last = cur;
        cur = cur->Next;
        last->Label_->Cleanup();
        delete last->Label_;
        delete last;
    }

    action_lbox = NULL;

    cur = percent_lbox;

    while (cur)
    {
        last = cur;
        cur = cur->Next;
        last->Label_->Cleanup();
        delete last->Label_;
        delete last;
    }

    percent_lbox = NULL;

    cur = intercept_lbox;

    while (cur)
    {
        last = cur;
        cur = cur->Next;
        last->Label_->Cleanup();
        delete last->Label_;
        delete last;
    }

    intercept_lbox = NULL;
}

void SetVCSortTypeCB(long ID, short hittype, C_Base *)
{
    F4CSECTIONHANDLE *Leave;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    VCSortType = ID;

    if (gVCTree)
    {
        Leave = UI_Enter(gVCTree->Parent_);
        gVCTree->ReorderBranch(gVCTree->GetRoot());
        gVCTree->RecalcSize();

        if (gVCTree->Parent_)
            gVCTree->Parent_->RefreshClient(gVCTree->GetClient());

        UI_Leave(Leave);
    }
}

BOOL VCSortCB(TREELIST *list, TREELIST *newitem)
{
    C_Victory *lvc, *nvc;

    if ( not list or not newitem)
        return(FALSE);

    lvc = (C_Victory*)list->Item_;
    nvc = (C_Victory*)newitem->Item_;

    if ( not lvc or not nvc)
        return(FALSE);

    switch (VCSortType)
    {
        case SORT_VC_TEAM: // Sort by Team
            if (nvc->GetTeam()->GetTextID() < lvc->GetTeam()->GetTextID())
                return(TRUE);
            else if (nvc->GetTeam()->GetTextID() == lvc->GetTeam()->GetTextID() and newitem->ID_ < list->ID_)
                return(TRUE);

            break;

        case SORT_VC_TYPE: // Sort by Action
            if (nvc->GetAction()->GetTextID() < lvc->GetAction()->GetTextID())
                return(TRUE);
            else if (nvc->GetAction()->GetTextID() == lvc->GetAction()->GetTextID() and newitem->ID_ < list->ID_)
                return(TRUE);

            break;

        case SORT_VC_POINTS: // Sort by Points
            if (nvc->GetPoints()->GetInteger() > lvc->GetPoints()->GetInteger())
                return(TRUE);
            else if (nvc->GetPoints()->GetInteger() == lvc->GetPoints()->GetInteger() and newitem->ID_ < list->ID_)
                return(TRUE);

            break;

        case SORT_VC_NUMBER: // Sort by ID
        default:
            if (newitem->ID_ < list->ID_)
                return(TRUE);

            break;
    }

    return(FALSE);
}

// Called AFTER MakeVC...() is called, when the user changes Action or Target
// I've just added some error correction code...
// If the vc type doesn't match the entity type (vt_attrit with an objective for example)
// change the type to match the entity
void UpdateVCOptions(victory_condition *vc)
{
    // Handles changing targets etc...
    C_Victory *vctrl;
    C_ListBox *lbox;
    CampEntity ent;
    _TCHAR buffer[60];
    FeatureClassDataType *fc;
    long classID, i;

    if ( not vc)
        return;

    vctrl = (C_Victory*)vc->control;

    if ( not vctrl or not vctrl->Parent_)
        return;

    ent = (CampEntity)vuDatabase->Find(vc->get_vu_id());

    if (ent)
    {
        // Set location... on Map...
        gMapMgr->UpdateVC(vc);

        if (ent->IsFlight())
            GetCallsign(((Flight)ent)->callsign_id, ((Flight)ent)->callsign_num, buffer);
        else if (ent->IsObjective())
            ent->GetName(buffer, 35, TRUE);
        else
            ent->GetName(buffer, 35, FALSE);

        if (ent->IsObjective() and vc->get_type() == vt_destroy and vc->get_sub_objective() >= 0)
        {
            classID = ((Objective)ent)->GetFeatureID(vc->get_sub_objective());

            if (classID)
            {
                fc = GetFeatureClassData(classID);

                if (fc and not (fc->Flags bitand FEAT_VIRTUAL))
                {
                    _tcscat(buffer, ", ");
                    _tcscat(buffer, fc->Name);
                }
            }
        }

        vctrl->GetTarget()->SetText(0, buffer);

        if (vc->get_type() == vt_destroy)
            vctrl->GetTarget()->SetUserNumber(10, 1);
        else
            vctrl->GetTarget()->SetUserNumber(10, 0);

        lbox = vctrl->GetAction();

        if (lbox)
        {
            if (ent)
            {
                if (ent->IsObjective())
                {
                    lbox->SetItemFlags(vt_occupy, C_BIT_ENABLED);
                    lbox->SetItemFlags(vt_destroy, C_BIT_ENABLED);
                    lbox->SetItemFlags(vt_degrade, C_BIT_ENABLED);

                    lbox->SetItemFlags(vt_attrit, C_BIT_INVISIBLE);
                    lbox->SetItemFlags(vt_intercept, C_BIT_INVISIBLE);

                    lbox->SetFlagBitOff(C_BIT_INVISIBLE);

                    if (vc->get_type() == vt_attrit or vc->get_type() == vt_intercept)
                        vc->set_type(vt_degrade);

                }
                else if (ent->IsFlight())
                {
                    lbox->SetItemFlags(vt_occupy, C_BIT_INVISIBLE);
                    lbox->SetItemFlags(vt_destroy, C_BIT_INVISIBLE);
                    lbox->SetItemFlags(vt_degrade, C_BIT_INVISIBLE);
                    lbox->SetItemFlags(vt_attrit, C_BIT_INVISIBLE);

                    lbox->SetItemFlags(vt_intercept, C_BIT_ENABLED);
                    lbox->SetValue(vt_intercept);
                    lbox->SetFlagBitOff(C_BIT_INVISIBLE);

                    if (vc->get_type() not_eq vt_intercept)
                        vc->set_type(vt_intercept);
                }
                else if (ent->IsBattalion())
                {
                    lbox->SetItemFlags(vt_occupy, C_BIT_INVISIBLE);
                    lbox->SetItemFlags(vt_destroy, C_BIT_INVISIBLE);
                    lbox->SetItemFlags(vt_degrade, C_BIT_INVISIBLE);
                    lbox->SetItemFlags(vt_intercept, C_BIT_INVISIBLE);

                    lbox->SetItemFlags(vt_attrit, C_BIT_ENABLED);
                    lbox->SetValue(vt_attrit);
                    lbox->SetFlagBitOff(C_BIT_INVISIBLE);

                    if (vc->get_type() not_eq vt_attrit)
                        vc->set_type(vt_attrit);
                }
                else
                    lbox->SetFlagBitOn(C_BIT_INVISIBLE);

                lbox->SetValue(vc->get_type());
            }
            else
                lbox->SetFlagBitOn(C_BIT_INVISIBLE);
        }

        lbox = vctrl->GetArgs();

        if (lbox)
        {
            switch (vc->get_type())
            {
                case vt_intercept:
                    lbox->SetRoot(intercept_lbox);
                    lbox->SetFlagBitOff(C_BIT_INVISIBLE);

                    if (ent and ent->IsFlight())
                    {
                        vc->set_tolerance(max(1, min(vc->get_tolerance(), 4)));
                    }
                    else
                    {
                        vc->set_tolerance(1);
                    }

                    lbox->SetValue(max(1, min(4, vc->get_tolerance())));

                    for (i = 1; i <= ((Flight)ent)->GetTotalVehicles(); i++)
                        lbox->SetItemFlags(i, C_BIT_ENABLED);

                    for (; i <= 4; i++)
                        lbox->SetItemFlags(i, C_BIT_INVISIBLE);

                    break;

                case vt_attrit:
                case vt_degrade:
                    lbox->SetRoot(percent_lbox);
                    lbox->SetFlagBitOff(C_BIT_INVISIBLE);

                    if (vc->get_tolerance() > 10)
                        vc->set_tolerance(10);
                    else if (vc->get_tolerance() < 1)
                        vc->set_tolerance(1);

                    lbox->SetValue(max(1, min(10, vc->get_tolerance())));
                    break;

                default:
                    vctrl->GetArgs()->SetRoot(percent_lbox);
                    vctrl->GetArgs()->SetFlagBitOn(C_BIT_INVISIBLE);
                    lbox->SetValue(1);
                    break;
            }
        }

        vctrl->Refresh();
    }
}

// sets up team stuff
void VCChangeTeamNoCB(long, short hittype, C_Base *)
{
    victory_condition *vc;
    C_Victory *vctrl;
    TREELIST *item;

    if (hittype not_eq C_TYPE_SELECT)
        return;

    item = gVCTree->GetLastItem();

    if (item and item->Item_)
    {
        vctrl = (C_Victory*)item->Item_;
        vc = (victory_condition*)vctrl->GetPtr();

        if (vc)
        {
            if (vctrl->GetTeam()->GetTextID() not_eq vc->get_team())
            {
                gMapMgr->RemoveVC(vc->get_team(), vc->get_number());
                vc->set_team(vctrl->GetTeam()->GetTextID());
                gMapMgr->AddVC(vc);
            }
        }
    }

    update_team_victory_window();
}

// sets up the actions
void VCChangeActionCB(long, short hittype, C_Base *control)
{
    victory_condition *vc;
    C_Victory *vctrl;
    TREELIST *item;

    if (hittype not_eq C_TYPE_SELECT)
        return;

    item = gVCTree->GetLastItem();

    if (item and item->Item_)
    {
        vctrl = (C_Victory*)item->Item_;
        vc = (victory_condition*)vctrl->GetPtr();

        if (vc)
        {
            vc->set_type((victory_type)((C_ListBox*)control)->GetTextID());
            UpdateVCOptions(vc);

            if (vc->get_type() == vt_destroy and vc->get_sub_objective() < 0)
            {
                // we need to target a building
                C_Window *win;
                C_Base *btn;

                SetCursor(gCursors[CRSR_WAIT]);

                win = gMainHandler->FindWindow(RECON_LIST_WIN);

                if (win)
                {
                    if (TargetTree)
                        TargetTree->DeleteBranch(TargetTree->GetRoot());

                    if ( not OldReconCWCB)
                    {
                        btn = win->FindControl(CLOSE_WINDOW);

                        if (btn)
                        {
                            OldReconCWCB = btn->GetCallback();
                            btn->SetCallback(TgtAssignCWCB);
                        }
                    }

                    btn = win->FindControl(SET_VC);

                    if (btn)
                        btn->SetFlagBitOff(C_BIT_INVISIBLE);

                    btn->Refresh();
                }

                SelectToolTypeCB(TARGET_VC, C_TYPE_LMOUSEUP, vctrl);
                BuildSpecificTargetList(vc->get_vu_id());
            }
            else if (vc->get_type() not_eq vt_destroy)
                vc->set_sub_objective(-1);
        }
    }
}

// sets up the target
void VCSetTargetCB(long, short hittype, C_Base *control)
{
    victory_condition *vc;
    C_Victory *vctrl;
    TREELIST *item;
    C_Window *win;
    C_Base *btn;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    if (control and control->GetUserNumber(10))
    {
        SetCursor(gCursors[CRSR_WAIT]);

        item = gVCTree->GetLastItem();

        if (item and item->Item_)
        {
            vctrl = (C_Victory*)item->Item_;
            vc = (victory_condition*)vctrl->GetPtr();
            win = gMainHandler->FindWindow(RECON_LIST_WIN);

            if (win and vc)
            {
                if (TargetTree)
                    TargetTree->DeleteBranch(TargetTree->GetRoot());

                if ( not OldReconCWCB)
                {
                    btn = win->FindControl(CLOSE_WINDOW);

                    if (btn)
                    {
                        OldReconCWCB = btn->GetCallback();
                        btn->SetCallback(TgtAssignCWCB);
                    }
                }

                btn = win->FindControl(SET_VC);

                if (btn)
                    btn->SetFlagBitOff(C_BIT_INVISIBLE);

                btn->Refresh();
                BuildSpecificTargetList(vc->get_vu_id());
            }
        }
    }

    SelectToolTypeCB(TARGET_VC, hittype, control);
}

// sets up the target bitand Args controls
void VCArgsCB(long, short hittype, C_Base *)
{
    victory_condition *vc;
    C_Victory *vctrl;
    TREELIST *item;

    if (hittype not_eq C_TYPE_SELECT)
        return;

    item = gVCTree->GetLastItem();

    if (item and item->Item_)
    {
        vctrl = (C_Victory*)item->Item_;
        vc = (victory_condition*)vctrl->GetPtr();

        if (vc)
        {
            vc->set_tolerance(vctrl->GetArgs()->GetTextID());
        }
    }
}

// sets up the target bitand Args controls
void VCSetPointsCB(long, short hittype, C_Base *control)
{
    victory_condition *vc;
    C_Victory *vctrl;
    TREELIST *item;

    if (hittype and hittype not_eq DIK_RETURN)
        return;

    item = gVCTree->GetLastItem();

    if (item and item->Item_)
    {
        vctrl = (C_Victory*)item->Item_;
        vc = (victory_condition*)vctrl->GetPtr();

        if (vc)
        {
            vc->set_points(((C_EditBox*)control)->GetInteger());
        }
    }

    update_team_victory_window();
}

void VCSelectVCCB(long, short hittype, C_Base *)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    if (gVCTree)
        gVCTree->SetAllControlStates(0, gVCTree->GetRoot());

    SelectToolTypeCB(0, C_TYPE_LMOUSEUP, NULL);
}

void AssignVCCB(long ID, short hittype, C_Base *control)
{
    TREELIST *item;
    C_Victory *vctrl;
    victory_condition *vc;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    CloseReconWindowCB(ID, hittype, control);

    if (FeatureID not_eq FalconNullId and FeatureNo >= 0)
    {
        if (gVCTree)
        {
            item = gVCTree->GetLastItem();

            if (item) // MUST MAKE SURE they didn't switch Items while targetting
            {
                vctrl = (C_Victory*)item->Item_;

                if (vctrl)
                {
                    vc = (victory_condition*)vctrl->GetPtr();

                    if (vc)
                    {
                        vc->set_vu_id(FeatureID);
                        vc->set_sub_objective(FeatureNo);
                        gMapMgr->AddVC(vc);
                        UpdateVCOptions(vc); // sets valid list box stuff etc
                        // Clear TOOL stuff
                        SelectToolTypeCB(0, C_TYPE_LMOUSEUP, NULL);
                    }
                }
            }
        }
    }
}

void SetVCTargetInfo(CampEntity ent)
{
    TREELIST *item;
    C_Victory *vctrl;
    victory_condition *vc;

    if ( not gVCTree)
        return;

    if (ent->IsFlight())
    {
        item = gVCTree->GetLastItem();

        if (item)
        {
            vctrl = (C_Victory*)item->Item_;

            if (vctrl)
            {
                vc = (victory_condition*)vctrl->GetPtr();

                if (vc)
                {
                    vc->set_vu_id(ent->Id());
                    vc->set_type(vt_intercept);
                    vc->set_tolerance(max(1, min(4, ((Flight)ent)->GetTotalVehicles())));
                    gMapMgr->AddVC(vc);
                    UpdateVCOptions(vc);
                }
            }
        }
    }
    else if (ent->IsObjective())
    {
        C_Window *win;
        C_Base *btn;

        SetCursor(gCursors[CRSR_WAIT]);

        win = gMainHandler->FindWindow(RECON_LIST_WIN);

        if (win)
        {
            if (TargetTree)
                TargetTree->DeleteBranch(TargetTree->GetRoot());

            if ( not OldReconCWCB)
            {
                btn = win->FindControl(CLOSE_WINDOW);

                if (btn)
                {
                    OldReconCWCB = btn->GetCallback();
                    btn->SetCallback(TgtAssignCWCB);
                }
            }

            btn = win->FindControl(SET_VC);

            if (btn)
                btn->SetFlagBitOff(C_BIT_INVISIBLE);

            btn->Refresh();
            BuildSpecificTargetList(ent->Id());
        }
    }
    else if (ent->IsBattalion())
    {
        item = gVCTree->GetLastItem();

        if (item)
        {
            vctrl = (C_Victory*)item->Item_;

            if (vctrl)
            {
                vc = (victory_condition*)vctrl->GetPtr();

                if (vc)
                {
                    vc->set_vu_id(ent->Id());
                    vc->set_type(vt_attrit);
                    vc->set_tolerance(10);
                    gMapMgr->AddVC(vc);
                    UpdateVCOptions(vc);
                }
            }
        }
    }
}

void SelectVCTargetCB(long ID, short hittype, C_Base *control)
{
    C_MapIcon *icon;
    UI_Refresher *urec;
    CampEntity ent;
    C_Victory *vctrl;
    victory_condition *vc;
    TREELIST *item;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    icon = (C_MapIcon*)control;

    if (control)
    {
        ID = icon->GetIconID();
        urec = (UI_Refresher*)gGps->Find(ID);

        if (urec)
        {
            item = gVCTree->GetLastItem();

            if (item)
            {
                vctrl = (C_Victory*)item->Item_;

                if (vctrl)
                {
                    vc = (victory_condition*)vctrl->GetPtr();

                    if (vc)
                    {
                        ent = (CampEntity)vuDatabase->Find(urec->GetID());

                        if (ent)
                        {
                            vc->set_vu_id(urec->GetID());
                            gMapMgr->AddVC(vc);

                            if (ent->IsObjective())
                            {
                                if (vc->get_type() == vt_destroy)
                                {
                                    SetVCTargetInfo(ent);
                                }
                                else
                                {
                                    if (vc->get_type() not_eq vt_occupy and vc->get_type() not_eq vt_degrade)
                                    {
                                        vc->set_type(vt_degrade);
                                        vc->set_tolerance(10);
                                    }

                                    UpdateVCOptions(vc);
                                }
                            }
                            else if (ent->IsFlight())
                            {
                                vc->set_type(vt_intercept);
                                vc->set_tolerance(min(4, ((Flight)ent)->GetTotalVehicles()));
                                UpdateVCOptions(vc);
                            }
                            else if (ent->IsBattalion())
                            {
                                vc->set_type(vt_attrit);
                                vc->set_tolerance(10);
                                UpdateVCOptions(vc);
                            }
                        }
                    }
                }
            }
        }
    }
}

void VCActionOpenCB(C_Base *me)
{
    TREELIST *item;
    C_Victory *vctrl;
    victory_condition *vc;

    if ( not gVCTree)
        return;

    item = gVCTree->GetLastItem();

    if ( not item)
        return;

    vctrl = (C_Victory *)item->Item_;

    if ( not vctrl)
        return;

    vc = (victory_condition*)vctrl->GetPtr();

    if ( not vc)
        return;

    switch (vc->get_type())
    {
        case vt_intercept:
            ((C_ListBox*)me)->SetItemFlags(vt_degrade, C_BIT_INVISIBLE);
            ((C_ListBox*)me)->SetItemFlags(vt_occupy, C_BIT_INVISIBLE);
            ((C_ListBox*)me)->SetItemFlags(vt_destroy, C_BIT_INVISIBLE);
            ((C_ListBox*)me)->SetItemFlags(vt_attrit, C_BIT_INVISIBLE);
            ((C_ListBox*)me)->SetItemFlags(vt_intercept, C_BIT_ENABLED);
            break;

        case vt_attrit:
            ((C_ListBox*)me)->SetItemFlags(vt_degrade, C_BIT_INVISIBLE);
            ((C_ListBox*)me)->SetItemFlags(vt_occupy, C_BIT_INVISIBLE);
            ((C_ListBox*)me)->SetItemFlags(vt_destroy, C_BIT_INVISIBLE);
            ((C_ListBox*)me)->SetItemFlags(vt_attrit, C_BIT_ENABLED);
            ((C_ListBox*)me)->SetItemFlags(vt_intercept, C_BIT_INVISIBLE);
            break;

        case vt_degrade:
        case vt_occupy:
        case vt_destroy:
            ((C_ListBox*)me)->SetItemFlags(vt_degrade, C_BIT_ENABLED);
            ((C_ListBox*)me)->SetItemFlags(vt_occupy, C_BIT_ENABLED);
            ((C_ListBox*)me)->SetItemFlags(vt_destroy, C_BIT_ENABLED);
            ((C_ListBox*)me)->SetItemFlags(vt_attrit, C_BIT_INVISIBLE);
            ((C_ListBox*)me)->SetItemFlags(vt_intercept, C_BIT_INVISIBLE);
            break;
    }
}

void VCArgsOpenCB(C_Base *me)
{
    TREELIST *item;
    C_Victory *vctrl;
    victory_condition *vc;
    CampEntity ent;
    long i;

    if ( not gVCTree)
        return;

    item = gVCTree->GetLastItem();

    if ( not item)
        return;

    vctrl = (C_Victory *)item->Item_;

    if ( not vctrl)
        return;

    vc = (victory_condition*)vctrl->GetPtr();

    if ( not vc)
        return;

    if (vc->get_type() == vt_intercept)
    {
        ent = (CampEntity)vuDatabase->Find(vc->get_vu_id());

        if (ent and ent->IsFlight())
        {
            for (i = 1; i <= ((Flight)ent)->GetTotalVehicles(); i++)
                ((C_ListBox*)me)->SetItemFlags(i, C_BIT_ENABLED);

            for (; i <= 4; i++)
                ((C_ListBox*)me)->SetItemFlags(i, C_BIT_INVISIBLE);
        }
    }
}

C_Victory *MakeVCControl(victory_condition *vc)
{
    C_Victory *vcntrl;
    C_ListBox *lbox;
    C_Button *btn;
    C_EditBox *ebox;
    C_Text *txt;
    CampEntity ent;
    _TCHAR buffer[60];
    long fh;

    if ( not gVCTree)
        return(NULL);

    fh = gFontList->GetHeight(gVCTree->GetFont()) + 2;

    ent = (CampEntity)vuDatabase->Find(vc->get_vu_id());

    vcntrl = new C_Victory;
    vcntrl->Setup(vc->get_number(), 0);
    vcntrl->SetPtr(vc);
    vcntrl->SetCallback(VCSelectVCCB);

    // VC ID #
    _stprintf(buffer, "%1d", vc->get_number());
    txt = new C_Text;
    txt->Setup(C_DONT_CARE, 0);
    txt->SetFixedWidth(4);
    txt->SetText(buffer);
    txt->SetFlagBitOn(C_BIT_RIGHT);
    vcntrl->SetNumber(txt);

    // Team Name
    lbox = new C_ListBox;
    lbox->Setup(1, 0, gMainHandler);
    lbox->SetWH(gVCTree->GetUserNumber(11), fh);
    lbox->SetBgFill(0, 0, 0, 0);
    lbox->SetBgColor(11370561); //
    lbox->SetRoot(team_lbox);
    lbox->SetValue(vc->get_team());
    lbox->SetDropDown(BID_DROPDOWN);
    lbox->SetCallback(VCChangeTeamNoCB);
    lbox->SetFlagBitOff(C_BIT_ENABLED);
    lbox->SetFlagBitOn(gVCTree->GetFlags() bitand C_BIT_ENABLED);
    vcntrl->SetTeam(lbox);

    // Action
    lbox = new C_ListBox;
    lbox->Setup(2, 0, gMainHandler);
    lbox->SetWH(gVCTree->GetUserNumber(12), fh);
    lbox->SetBgFill(0, 0, 0, 0);
    lbox->SetBgColor(11370561); //
    lbox->SetRoot(action_lbox);
    lbox->SetValue(vc->get_type());
    lbox->SetOpenCallback(VCActionOpenCB);
    lbox->SetCallback(VCChangeActionCB);
    lbox->SetFlagBitOff(C_BIT_ENABLED);
    lbox->SetFlagBitOn(gVCTree->GetFlags() bitand C_BIT_ENABLED);
    lbox->SetDropDown(BID_DROPDOWN);

    if ( not ent)
        lbox->SetFlagBitOn(C_BIT_INVISIBLE);

    vcntrl->SetAction(lbox);


    if (ent)
    {
        if (ent->IsFlight())
            GetCallsign(((Flight)ent)->callsign_id, ((Flight)ent)->callsign_num, buffer);
        else if (ent->IsObjective())
            ent->GetName(buffer, 40, TRUE);
        else
            ent->GetName(buffer, 40, FALSE);
    }

    // Target
    btn = new C_Button;
    btn->Setup(TARGET_VC, C_TYPE_NORMAL, 0, 0);

    if (ent)
        btn->SetText(0, buffer);
    else
        btn->SetText(0, TXT_ASSIGN);

    btn->SetFlagBitOn(C_BIT_USELINE);
    btn->SetCallback(VCSetTargetCB);
    btn->SetUserNumber(1, HELP_PICK_VC_TARGET);

    if (vc->get_type() == vt_destroy)
        btn->SetUserNumber(10, 1);

    vcntrl->SetTarget(btn);

    // Arguments (other than target)
    lbox = new C_ListBox;
    lbox->Setup(4, 0, gMainHandler);
    lbox->SetWH(gVCTree->GetUserNumber(14), fh);
    lbox->SetBgFill(0, 0, 0, 0);
    lbox->SetBgColor(11370561); //
    lbox->SetDropDown(BID_DROPDOWN);
    lbox->SetCallback(VCArgsCB);
    lbox->SetOpenCallback(VCArgsOpenCB);
    lbox->SetFlagBitOff(C_BIT_ENABLED);
    lbox->SetFlagBitOn(gVCTree->GetFlags() bitand C_BIT_ENABLED);

    if (vc->get_type() == vt_intercept)
        lbox->SetRoot(intercept_lbox);
    else
        lbox->SetRoot(percent_lbox);

    vcntrl->SetArgs(lbox);

    switch (vc->get_type())
    {
        case vt_attrit:
        case vt_intercept:
        case vt_degrade:
            lbox->SetFlagBitOff(C_BIT_INVISIBLE);
            break;

        default:
            lbox->SetFlagBitOn(C_BIT_INVISIBLE);
            break;
    }

    lbox->SetValue(vc->get_tolerance());

    if ( not ent)
        lbox->SetFlagBitOn(C_BIT_INVISIBLE);

    // Points
    ebox = new C_EditBox;
    ebox->Setup(5, C_TYPE_INTEGER);
    ebox->SetMaxLen(6);
    ebox->SetWH(gVCTree->GetUserNumber(15), fh);
    ebox->SetMinInteger(-99999);
    ebox->SetMaxInteger(99999);
    ebox->SetInteger(vc->get_points());
    ebox->SetBgColor(11370561); //
    ebox->SetFlagBitOn(C_BIT_USEOUTLINE bitor C_BIT_RIGHT);
    ebox->SetCallback(VCSetPointsCB);
    vcntrl->SetPoints(ebox);

    vcntrl->SetNumberX(static_cast<short>(gVCTree->GetUserNumber(0)));
    vcntrl->SetTeamX(static_cast<short>(gVCTree->GetUserNumber(1)));
    vcntrl->SetActionX(static_cast<short>(gVCTree->GetUserNumber(2)));
    vcntrl->SetTargetX(static_cast<short>(gVCTree->GetUserNumber(3)));
    vcntrl->SetArgsX(static_cast<short>(gVCTree->GetUserNumber(4)));
    vcntrl->SetPointsX(static_cast<short>(gVCTree->GetUserNumber(5)));
    vcntrl->SetFont(gVCTree->GetFont());
    vcntrl->SetWH(ebox->GetX() + ebox->GetW(), fh + 1);
    vcntrl->SetState(0);
    return(vcntrl);
}

int advance_team(int team, int state)
{
    int
    ok,
    test,
    loop;

    for (test = 1; test < NUM_COUNS; test ++)
    {
        state ++;

        if (state > 8) state = 1;

        ok = TRUE;

        for (loop = 1; loop < NUM_TEAMS; loop ++)
        {
            if
            (
                (loop not_eq team) and 
                (TeamInfo[loop]) and 
                (TeamInfo[loop]->GetFlag() == state)
            )
            {
                ok = FALSE;
                break;
            }
        }

        if (ok)
        {
            return state;
        }
    }

    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

long VCFlagIDs[NUM_TEAMS][4] =
{
    { COLOR_1, FLAG_1, TEAM_01, CURRENT_PTS_01 },
    { COLOR_2, FLAG_2, TEAM_02, CURRENT_PTS_02 },
    { COLOR_3, FLAG_3, TEAM_03, CURRENT_PTS_03 },
    { COLOR_4, FLAG_4, TEAM_04, CURRENT_PTS_04 },
    { COLOR_5, FLAG_5, TEAM_05, CURRENT_PTS_05 },
    { COLOR_6, FLAG_6, TEAM_06, CURRENT_PTS_06 },
    { COLOR_7, FLAG_7, TEAM_07, CURRENT_PTS_07 },
    { COLOR_8, FLAG_8, TEAM_08, CURRENT_PTS_08 },
};

long GetCurrentVCScore(long teamno)
{
    victory_condition *vc;
    long value;

    value = 0;
    vc = current_tactical_mission->get_first_unfiltered_victory_condition();

    while (vc)
    {
        if (vc->get_team() == teamno and vc->get_active())
            value += vc->get_points();

        vc = current_tactical_mission->get_next_unfiltered_victory_condition();
    }

    return(value);
}

long GetPossibleVCScore(long teamno)
{
    victory_condition *vc;
    long value;

    value = 0;
    vc = current_tactical_mission->get_first_unfiltered_victory_condition();

    while (vc)
    {
        if (vc->get_team() == teamno and vc->get_points() > 0)
            value += vc->get_points();

        vc = current_tactical_mission->get_next_unfiltered_victory_condition();
    }

    return(value);
}

void UpdateVCScoring(long WinID, short mode)
{
    long sortindex[NUM_TEAMS], points[NUM_TEAMS];
    long i, j, line, tmp;
    C_Window *win;
    C_EditBox *ebox;
    C_Bitmap *bmp;
    C_Box *box;

    memset(sortindex, 0, sizeof(sortindex));
    memset(points, 0, sizeof(points));

    line = 0;

    for (i = 0; i < NUM_TEAMS; i++)
    {
        if (TeamInfo[i] and (TeamInfo[i]->flags bitand TEAM_ACTIVE))
        {
            sortindex[line] = i;

            if ( not mode) // Show current score for this team
            {
                points[line] = GetCurrentVCScore(i);
            }
            else // Show possible score for this team
            {
                points[line] = GetPossibleVCScore(i);
            }

            line++;
        }
    }

    if ( not mode)
    {
        for (i = 1; i < line; i++)
        {
            for (j = 0; j < i; j++)
            {
                if (points[sortindex[j]] > points[sortindex[i]])
                {
                    tmp = sortindex[i];
                    sortindex[i] = sortindex[j];
                    sortindex[j] = tmp;
                    tmp = points[i];
                    points[i] = points[j];
                    points[j] = tmp;
                }
            }
        }
    }

    win = gMainHandler->FindWindow(WinID);

    if (win)
    {
        ebox = (C_EditBox*)win->FindControl(PTS_REQ_VICTORY);

        if (ebox)
        {
            ebox->SetInteger(current_tactical_mission->get_points_required());
            ebox->Refresh();
        }

        for (i = 0; i < line; i++)
        {
            if (TeamInfo[sortindex[i]])
            {
                box = (C_Box*)win->FindControl(VCFlagIDs[i][0]);

                if (box)
                {
                    box->SetColor(TeamColorList[TeamInfo[sortindex[i]]->GetColor()]);
                    box->Refresh();
                }

                bmp = (C_Bitmap*)win->FindControl(VCFlagIDs[i][1]);

                if (bmp)
                {
                    bmp->SetImage(FlagImageID[TeamInfo[sortindex[i]]->GetFlag()][BIG_HORIZ]);
                    bmp->Refresh();
                }

                ebox = (C_EditBox*)win->FindControl(VCFlagIDs[i][2]);

                if (ebox)
                {
                    ebox->SetText(TeamInfo[sortindex[i]]->GetName());
                    ebox->Refresh();
                }

                ebox = (C_EditBox*)win->FindControl(VCFlagIDs[i][3]);

                if (ebox)
                {
                    ebox->SetInteger(points[i]);
                    ebox->Refresh();
                }

                win->UnHideCluster(i + 1);
            }
        }

        while (i <= NUM_TEAMS)
        {
            win->HideCluster(i + 1);

            i ++;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void update_team_victory_window(void)
{
    if (gMainHandler)
    {
        if (TheCampaign.Flags bitand CAMP_TACTICAL_EDIT)
            UpdateVCScoring(TAC_VC_WIN, 1);
        else
            UpdateVCScoring(TAC_VC_WIN, 0);
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static void evaluate_filter(void)
{
    victory_condition_filter
    vcf;
#if 0

    if (show_team_button->GetState())
    {
        if (show_achieved_button->GetState())
        {
            vcf = vcf_team_achieved;
        }
        else if (show_remaining_button->GetState())
        {
            vcf = vcf_team_remaining;
        }
        else
        {
            vcf = vcf_team;
        }
    }
    else
    {
        if (show_achieved_button->GetState())
        {
            vcf = vcf_all_achieved;
        }
        else if (show_remaining_button->GetState())
        {
            vcf = vcf_all_remaining;
        }
        else
        {
            vcf = vcf_all;
        }
    }

#endif
    vcf = vcf_all;

    if (current_tactical_mission)
    {
        current_tactical_mission->set_victory_condition_filter(vcf);
        current_tactical_mission->set_victory_condition_team_filter(1);
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


static void delete_current_vc(long, short hittype, C_Base *)
{
    TREELIST *item;
    C_Victory *vctrl;
    victory_condition *vc;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    if ( not gVCTree)
        return;

    SelectToolTypeCB(0, C_TYPE_LMOUSEUP, NULL);
    item = gVCTree->GetLastItem();

    if (item)
    {
        vctrl = (C_Victory*)item->Item_;

        if (vctrl)
        {
            vc = (victory_condition*)vctrl->GetPtr();

            if (vc)
            {
                gMapMgr->RemoveVC(vc->get_team(), vc->get_number());
                gVCTree->DeleteItem(item->ID_);
                delete vc;
                gVCTree->RecalcSize();
                gVCTree->Parent_->RefreshClient(gVCTree->GetClient());
            }
        }
    }

    update_team_victory_window();
}

static void new_victory_condition(long, short hittype, C_Base *)
{
    victory_condition *vc;
    TREELIST *item;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    //MonoPrint ("New Victory Condition\n");

    if ( not gVCTree)
        return;

    vc = new victory_condition(current_tactical_mission);
    vc->set_team(1);
    vc->control = MakeVCControl(vc);

    item = gVCTree->CreateItem(vc->get_number(), C_TYPE_ITEM, vc->control);

    if (item)
    {
        gVCTree->AddItem(gVCTree->GetRoot(), item);
        ((C_Victory*)vc->control)->SetOwner(item);
        vc->control->SetReady(1);
        vc->control->SetClient(gVCTree->GetClient());
        vc->control->SetParent(gVCTree->Parent_);
        vc->control->SetSubParents(gVCTree->Parent_);
        gVCTree->RecalcSize();

        if (gVCTree->Parent_)
            gVCTree->Parent_->RefreshClient(gVCTree->GetClient());
    }

    if (gMapMgr)
    {
        gMapMgr->AddVC(vc);
    }

    UpdateVCOptions(vc);

    evaluate_filter();

    update_team_victory_window();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#if 0
static void change_vc_team_name(long ID, short hittype, C_Base *ctrl)
{
    victory_condition
    *vc;

    C_ListBox
    *listbox;

    int
    id,
    old_team,
    new_team;

    if (hittype not_eq C_TYPE_SELECT)
    {
        return;
    }

    listbox = (C_ListBox *) ctrl;

    vc = (victory_condition *) listbox->GetUserNumber(0);

    old_team = vc->get_team();
    new_team = listbox->GetTextID();

    vc->set_team(new_team);

    if
    (
        ((old_team > 0) and (new_team > 0)) or
        ((new_team < 0) and (old_team < 0))
    )
    {
        // don't need to do anything else, cos the team hasn't changed type.
        return;
    }

    id = ctrl->GetID();

    listbox = (C_ListBox *) ctrl->Parent_->FindControl(id + 1);

    if (listbox)
    {
        listbox->RemoveAllItems();

        listbox->AddItem(vt_occupy, C_TYPE_ITEM, "Occupy");
        listbox->AddItem(vt_destroy, C_TYPE_ITEM, "Destroy");
        listbox->AddItem(vt_attrit, C_TYPE_ITEM, "Attrit");
        listbox->AddItem(vt_intercept, C_TYPE_ITEM, "Intercept");
        listbox->AddItem(vt_degrade, C_TYPE_ITEM, "Degrade");

        /*
         if (new_team == -1)
         {
         listbox->AddItem (vt_kills, C_TYPE_ITEM, "Kills");
         listbox->AddItem (vt_frags, C_TYPE_ITEM, "Frags");
         listbox->AddItem (vt_deaths, C_TYPE_ITEM, "Deaths");
         listbox->AddItem (vt_landing, C_TYPE_ITEM, "Landing");
         }

         listbox->AddItem (vt_tos, C_TYPE_ITEM, "Tos");
         listbox->AddItem (vt_airspeed, C_TYPE_ITEM, "Airspeed");
         listbox->AddItem (vt_altitude, C_TYPE_ITEM, "Altitude");
         listbox->AddItem (vt_position, C_TYPE_ITEM, "Position");
        */
    }

    gMapMgr->RemoveVC(old_team, vc->get_number());
    gMapMgr->AddVC(vc);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static void change_vc_team_action(long ID, short hittype, C_Base *ctrl)
{
    victory_condition
    *vc;

    C_ListBox
    *listbox;

    C_Window
    *win;

    if (hittype not_eq C_TYPE_SELECT)
    {
        return;
    }

    win = ctrl->GetParent();

    listbox = (C_ListBox *) ctrl;

    vc = (victory_condition *) listbox->GetUserNumber(0);

    vc->set_type((victory_type) listbox->GetTextID());

    if (vc->arg1_control)
    {
        vc->arg1_control->SetFlagBitOn(C_BIT_INVISIBLE);
    }

    if (vc->arg2_control)
    {
        vc->arg2_control->SetFlagBitOn(C_BIT_INVISIBLE);
    }

    if (vc->arg3_control)
    {
        vc->arg3_control->SetFlagBitOn(C_BIT_INVISIBLE);
    }

    team_victory_id = add_team_args(vc->team_control->GetY(), win, vc, team_victory_id);

    win->ScanClientAreas();
    win->RefreshClient(2);
    win->RefreshWindow();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static void change_vc_tolerance(long ID, short hittype, C_Base *ctrl)
{
    victory_condition
    *vc;

    C_ListBox
    *listbox;

    C_Window
    *win;

    if (hittype not_eq C_TYPE_SELECT)
    {
        return;
    }

    win = ctrl->GetParent();

    listbox = (C_ListBox *) ctrl;

    vc = (victory_condition *) listbox->GetUserNumber(0);

    vc->set_tolerance(listbox->GetTextID());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static void change_vc_steerpoint(long ID, short hittype, C_Base *ctrl)
{
    victory_condition
    *vc;

    C_ListBox
    *listbox;

    C_Window
    *win;

    if (hittype not_eq C_TYPE_SELECT)
    {
        return;
    }

    win = ctrl->GetParent();

    listbox = (C_ListBox *) ctrl;

    vc = (victory_condition *) listbox->GetUserNumber(0);

    vc->set_steerpoint(listbox->GetTextID());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static void set_vc_points(long ID, short hittype, C_Base *ctrl)
{
    C_EditBox
    *editbox;

    victory_condition
    *vc;

    editbox = (C_EditBox *) ctrl;

    vc = (victory_condition *) editbox->GetUserNumber(0);

    vc->set_points(editbox->GetInteger());
    update_team_victory_window();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#endif

static void set_points_required_for_victory(long, short hittype, C_Base *ctrl)
{
    C_EditBox
    *editbox;

    if (hittype not_eq DIK_RETURN)
        return;

    editbox = (C_EditBox *) ctrl;

    current_tactical_mission->set_points_required(editbox->GetInteger());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static void delete_tactical_object(long, short, C_Base *)
{
    C_Base
    *parent;

    victory_condition
    *next_vc,
    *vc;

    unsigned char
    *src;

    int
    id,
    x, y,
    old_team,
    width,
    height,
    loop;

    C_Squadron
    *c_squadron;

    C_ATO_Flight
    *c_flight;

    SquadronClass
    *squadron;

    FlightClass
    *flight;

    VU_ID
    vu_id;

    parent = gPopupMgr->GetCallingControl();

    gPopupMgr->CloseMenu();

    id = parent->GetID();

    if (id == ATO_ALL_TREE)
    {
        c_flight = (C_ATO_Flight *)((C_TreeList *) parent)->GetLastItem()->Item_;

        flight = (Flight) FindEntity(c_flight->GetVUID());

        if (flight)
        {
            flight->Remove();

            gGps->Update();
        }
    }
    else if (id == ALL_SQUADRON_TREE)
    {
        // Its a squadron :-)
        c_squadron = (C_Squadron *)((C_TreeList *) parent)->GetLastItem()->Item_;

        squadron = (Squadron) FindEntity(c_squadron->GetVUID());

        if (squadron)
        {
            tactical_remove_squadron(squadron);

            squadron->Remove();

            gGps->Update();
        }
    }
    else if (id >= 3000000)
    {
        // Its a victory condition - delete it.

        vc = (victory_condition *) parent->GetUserNumber(0);
        gMapMgr->RemoveVC(vc->get_team(), vc->get_number());

        delete vc;

        evaluate_filter();

        update_team_victory_window();
    }
    else
    {
        // Its actually a team we are trying to delete - just overloaded the function a little :-)
        old_team = 0;

        for (loop = 1; loop < 8; loop ++)
        {
            if (TeamInfo[loop])
            {
                old_team ++;
            }
        }

        if (old_team > 1) // must have some old team if you want to delete it :-)
        {
            for (loop = 0; loop < 8; loop ++)
            {
                if (parent == team_name[loop])
                {
                    RemoveTeam(team_mapping[loop]);

                    // Need to remove Team Ownership on land.

                    src = TheCampaign.CampMapData;

                    width = TheCampaign.TheaterSizeX / MAP_RATIO;
                    height = TheCampaign.TheaterSizeY / MAP_RATIO;

                    for (y = 0; y < height; y ++)
                    {
                        for (x = 0; x < width; x ++)
                        {
                            if (x bitand 1)
                            {
                                old_team = (src[(y * width + x) / 2] bitand 0xf0) >> 4;

                                if (old_team == team_mapping[loop])
                                {
                                    src[(y * width + x) / 2] and_eq 0x0f;
                                }
                            }
                            else
                            {
                                old_team = src[(y * width + x) / 2] bitand 0xf;

                                if (old_team == team_mapping[loop])
                                {
                                    src[(y * width + x) / 2] and_eq 0xf0;
                                }
                            }
                        }
                    }

                    // Need to Fix all Objectives / Squadrons
                    delete_all_units_for_team(team_mapping[loop]);

                    // Remove all of this team Victory Conditions
                    vc = current_tactical_mission->get_first_unfiltered_victory_condition();

                    while (vc)
                    {
                        next_vc = current_tactical_mission->get_next_unfiltered_victory_condition();

                        if (vc->get_team() == team_mapping[loop])
                        {
                            gMapMgr->RemoveVC(vc->get_team(), vc->get_number());
                            delete vc;
                        }

                        vc = next_vc;
                    }

                    update_team_victory_window();

                    break;
                }
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void tactical_add_victory_condition(VU_ID id, C_Base *)
{
    CampEntity ent;
    victory_condition *vc;
    TREELIST *item;

    ent = (CampEntity)vuDatabase->Find(id);

    if ( not ent)
        return;

    if (ent->IsObjective() or ent->IsFlight() or ent->IsBattalion())
    {
        vc = new victory_condition(current_tactical_mission);

        vc->set_team(gSelectedTeam);
        vc->set_vu_id(id);

        if (ent->IsObjective())
        {
            vc->set_type(vt_degrade);
            vc->set_tolerance(10); // default to 100%
        }
        else if (ent->IsFlight())
        {
            vc->set_type(vt_intercept);
            vc->set_tolerance(((Flight)ent)->GetTotalVehicles()); // default to ALL
        }
        else
        {
            vc->set_type(vt_attrit);
            vc->set_tolerance(10); // default to 100%
        }

        vc->set_sub_objective(-1); // NO individual target assigned

        if (gVCTree)
        {
            vc->control = MakeVCControl(vc);

            // Add VC control to VC tree
            item = gVCTree->CreateItem(vc->get_number(), C_TYPE_ITEM, vc->control);

            if (item)
            {
                gVCTree->AddItem(gVCTree->GetRoot(), item);
                ((C_Victory*)vc->control)->SetOwner(item);
                vc->control->SetReady(1);
                vc->control->SetClient(gVCTree->GetClient());
                vc->control->SetParent(gVCTree->Parent_);
                vc->control->SetSubParents(gVCTree->Parent_);
                gVCTree->RecalcSize();

                if (gVCTree->Parent_)
                    gVCTree->Parent_->RefreshClient(gVCTree->GetClient());
            }
        }

        if (gMapMgr)
        {
            gMapMgr->AddVC(vc);
        }

        UpdateVCOptions(vc);
        update_team_victory_window();
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int get_tactical_number_of_teams(void)
{
    int
    num,
    loop;

    num = 0;

    for (loop = 1; loop < 8; loop ++)
    {
        if (TeamInfo[loop])
        {
            num ++;
        }
    }

    return num;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void add_vc_air_unit(long, short, C_Base *)
{
    C_Base
    *parent;

    victory_condition
    *vc;

    FlightClass
    *flight;

    VU_ID
    vu_id;

    C_ATO_Flight
    *c_flight;

    parent = gPopupMgr->GetCallingControl();

    gPopupMgr->CloseMenu();

    c_flight = (C_ATO_Flight *)((C_TreeList *) parent)->GetLastItem()->Item_;

    flight = (Flight) FindEntity(c_flight->GetVUID());

    if (flight)
    {
        vc = new victory_condition(current_tactical_mission);

        vc->set_team(flight->GetTeam());
        vc->set_type(vt_intercept);
        vc->set_vu_id(vu_id);

        if (gMapMgr)
        {
            gMapMgr->AddVC(vc);
        }

        update_team_victory_window();

        gMainHandler->EnableWindowGroup(3400);
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void add_all_vcs_to_ui(void)
{
    TREELIST *item;
    victory_condition *vc;

    if ( not gVCTree)
        return;

    gVCTree->DeleteBranch(gVCTree->GetRoot());

    if (gMapMgr)
    {
        vc = current_tactical_mission->get_first_unfiltered_victory_condition();

        while (vc)
        {
            // Add VC Icons to Map

            // Init VC control
            vc->control = MakeVCControl(vc);

            // Add VC control to VC tree
            item = gVCTree->CreateItem(vc->get_number(), C_TYPE_ITEM, vc->control);

            if (item)
            {
                gVCTree->AddItem(gVCTree->GetRoot(), item);
                ((C_Victory*)vc->control)->SetOwner(item);
                vc->control->SetReady(1);
                vc->control->SetClient(gVCTree->GetClient());
                vc->control->SetParent(gVCTree->Parent_);
                vc->control->SetSubParents(gVCTree->Parent_);
            }

            gMapMgr->AddVC(vc);
            UpdateVCOptions(vc);

            vc = current_tactical_mission->get_next_unfiltered_victory_condition();
        }
    }

    gVCTree->RecalcSize();

    if (gVCTree->Parent_)
        gVCTree->Parent_->RefreshClient(gVCTree->GetClient());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// returns LISTBOX ID to use
long EvaluateSituation()
{
    short i, teams, winners, higuys, myteam;
    long TeamScores[NUM_TEAMS];
    long VictoryPoints, HighestPoints;

    memset(TeamScores, 0, sizeof(TeamScores));
    teams = 0;

    for (i = 0; i < NUM_TEAMS; i++)
    {
        if (TeamInfo[i] and (TeamInfo[i]->flags bitand TEAM_ACTIVE))
        {
            TeamScores[i] = GetCurrentVCScore(i);
            teams++;
        }
    }

    winners = 0;
    HighestPoints = 0;
    VictoryPoints = current_tactical_mission->get_points_required();

    for (i = 0; i < NUM_TEAMS; i++)
    {
        if (TeamScores[i] >= VictoryPoints)
            winners++;

        if (TeamScores[i] > HighestPoints)
            HighestPoints = TeamScores[i];
    }

    higuys = 0;

    for (i = 0; i < NUM_TEAMS; i++)
    {
        if (TeamScores[i] >= HighestPoints)
            higuys++;
    }

    // Handle 1 team stuff
    if (teams < 2)
    {
        if (winners)
            return(TAC_SUCCESS);
        else
            return(TAC_FAILURE);
    }
    else if (winners) // Handle Multiple team stuff
    {
        myteam = FalconLocalSession->GetTeam();

        if (TeamScores[myteam] >= VictoryPoints) // We are among the winners
        {
            if (TeamScores[myteam] >= HighestPoints) // We are on top
            {
                if (higuys > 1) // Draw... oh well
                    return(TAC_DRAW);
                else
                {
                    if (winners > 1)
                        return(TAC_MARGINAL_WIN);
                    else
                        return(TAC_DECISIVE_WIN);
                }
            }
            else
                return(TAC_DEFEAT);
        }
        else // We are a loser :)
        {
            return(TAC_MAJOR_DEFEAT);
        }
    }

    return(TAC_STALEMATE);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// This is called to open the EndGame window after returning to the UI
void EndGameEvaluation()
{
    C_Window *win;
    C_ListBox *lbox;
    LISTBOX *lbitem;
    long eval;

    win = gMainHandler->FindWindow(TAC_END_WIN);
    {
        UpdateVCScoring(TAC_END_WIN, 0);
        lbox = (C_ListBox*)win->FindControl(TAC_WIN_TITLE);

        if (lbox)
        {
            eval = EvaluateSituation();
            lbox->SetValue(eval);
            lbitem = lbox->FindID(eval);

            if (lbitem and lbitem->Label_)
            {
                win->HideCluster(lbitem->Label_->GetUserNumber(1));
                win->HideCluster(lbitem->Label_->GetUserNumber(2));
                win->UnHideCluster(lbitem->Label_->GetUserNumber(0));
            }
        }

        win->RefreshWindow();
        gMainHandler->ShowWindow(win);
        gMainHandler->WindowToFront(win);
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void TriggerTacEndGame(void)
{
    ShowGameOverWindow = 1;

    if (gMainHandler and ShowGameOverWindow)
    {
        C_Window *win;

        TheCampaign.EndgameResult = 1;
        SetTimeCompression(0);

        if (gMainHandler->GetWindowFlags(CP_COUNTDOWN_WIN) bitand C_BIT_ENABLED)
        {
            win = gMainHandler->FindWindow(CP_COUNTDOWN_WIN);

            if (win)
            {
                CancelCampaignCompression();
                gMainHandler->HideWindow(win);
            }
        }

        PostMessage(gMainHandler->GetAppWnd(), FM_OPEN_GAME_OVER_WIN, game_TacticalEngagement, 0);
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void OpenTEGameOverWindow()
{
    if ( not gMainHandler)
        return;

    EndGameEvaluation();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void TacEngGameOver()
{
    if (current_tactical_mission)
    {
        current_tactical_mission->set_game_over(1);
        check_victory_conditions();

        TriggerTacEndGame(); // Tell UI to open window
        OTWDriver.SetFrontTextFlags(OTWDriver.GetFrontTextFlags() bitor SHOW_TE_SCORES);
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// Remote version...
void TacEngSetVCCompleted(long ID, int value)
{
    victory_condition *vc;

    if (current_tactical_mission)
    {
        vc = current_tactical_mission->get_first_unfiltered_victory_condition();

        while (vc)
        {
            if (vc->get_number() == ID)
            {
                vc->set_active(value);
            }

            vc = current_tactical_mission->get_next_unfiltered_victory_condition();
        }

        gRefreshScoresList = 1;

        if ( not current_tactical_mission->get_game_over() and check_victory_conditions())
        {
            TriggerTacEndGame(); // Tell UI to open window
            OTWDriver.SetFrontTextFlags(OTWDriver.GetFrontTextFlags() bitor SHOW_TE_SCORES);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// Host Version
void CheckForVictory(void)
{
    if (current_tactical_mission)
    {
        // Victory Condition Checks
        if ( not current_tactical_mission->get_game_over() and check_victory_conditions())
        {
            // Kevin, when you transmit the EndgameResult variable... there is a duplicate section of code to this in
            // te_team_victory.cpp at the bottom
            TriggerTacEndGame(); // Tell UI to open window
            OTWDriver.SetFrontTextFlags(OTWDriver.GetFrontTextFlags() bitor SHOW_TE_SCORES);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

