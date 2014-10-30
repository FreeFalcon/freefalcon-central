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
#include "unit.h"
#include "team.h"
#include "CmpGlobl.h"
#include "CampCell.h"
#include "CampTerr.h"
#include "find.h"
#include "division.h"
#include "battalion.h"
#include "cmap.h"
#include "flight.h"
#include "campwp.h"
#include "cmpclass.h"
#include "Listadt.h"
#include "objectiv.h"
#include "Campaign.h"
#include "classtbl.h"
#include "falcsess.h"
#include "tac_class.h"
#include "te_defs.h"
#include "teamdata.h"
#include "gps.h"
#include "urefresh.h"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern C_Map
*gMapMgr;

extern GlobalPositioningSystem
*gGps;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern long
gLastUpdateGround,
gLastUpdateAir;

extern GridIndex
MapX, MapY;

float SimMapX, SimMapY;

C_Base *CurMapTool = NULL;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void tactical_draw_map(long ID, short hittype, C_Base *control);

static void tactical_maximize_map(long ID, short hittype, C_Base *control);
static void tactical_minimize_map(long ID, short hittype, C_Base *control);
static void gMapMgr_mover(long ID, short hittype, C_Base *control);
static void gMapMgr_zoom_in(long ID, short hittype, C_Base *control);
static void gMapMgr_zoom_out(long ID, short hittype, C_Base *control);
void gMapMgr_menu(long ID, short hittype, C_Base *control);
void tactical_objective_menu(long ID, short hittype, C_Base *control);
void tactical_airbase_menu(long ID, short hittype, C_Base *control);
void SelectToolTypeCB(long ID, short hittype, C_Base *control);

void OpenCrossSectionCB(long ID, short hittype, C_Base *control);
void tactical_add_victory_condition(VU_ID id, C_Base *caller);
void tactical_add_squadron(VU_ID id);
extern void tactical_add_flight(VU_ID ID, C_Base *caller);
extern void tactical_add_package(VU_ID ID, C_Base *caller);
extern void tactical_add_battalion(VU_ID ID, C_Base *control);
static void update_gMapMgr(void);
void PickTeamCB(long ID, short hittype, C_Base *control);
void UnitCB(long ID, short hittype, C_Base *ctrl);
void SelectVCTargetCB(long ID, short hittype, C_Base *control);
void FitFlightPlanCB(long ID, short hittype, C_Base *control);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void ClearMapToolStates(long ID)
{
    C_Window *win;
    C_Button *ctrl;

    win = gMainHandler->FindWindow(ID);

    if (win)
    {
        ctrl = (C_Button *) win->FindControl(ADD_FLIGHT);

        if (ctrl)
        {
            ctrl->SetState(0);
        }

        ctrl = (C_Button *) win->FindControl(ADD_PACKAGE);

        if (ctrl)
        {
            ctrl->SetState(0);
        }

        ctrl = (C_Button *) win->FindControl(ADD_BATTALION);

        if (ctrl)
        {
            ctrl->SetState(0);
        }

        ctrl = (C_Button *) win->FindControl(ADD_NAVAL);

        if (ctrl)
        {
            ctrl->SetState(0);
        }

        ctrl = (C_Button *) win->FindControl(ADD_SQUADRON);

        if (ctrl)
        {
            ctrl->SetState(0);
        }

        ctrl = (C_Button *) win->FindControl(COPY_UNIT);

        if (ctrl)
        {
            ctrl->SetState(0);
        }

        ctrl = (C_Button *) win->FindControl(DELETE_UNIT);

        if (ctrl)
        {
            ctrl->SetState(0);
        }

        ctrl = (C_Button *) win->FindControl(EDIT_UNIT);

        if (ctrl)
        {
            ctrl->SetState(0);
        }
    }
}

void hookup_map_windows(C_Window *win)
{
    C_Button
    *ctrl;

    gMainHandler->AddUserCallback(update_gMapMgr);

    ctrl = (C_Button *) win->FindControl(MAP_MAXIMIZE);

    if (ctrl)
    {
        ctrl->SetCallback(tactical_maximize_map);
    }

    ctrl = (C_Button *) win->FindControl(MAP_MINIMIZE);

    if (ctrl)
    {
        ctrl->SetCallback(tactical_minimize_map);
    }

    ctrl = (C_Button *) win->FindControl(TAC_ZOOM_IN);

    if (ctrl)
    {
        ctrl->SetCallback(gMapMgr_zoom_in);
    }

    ctrl = (C_Button *) win->FindControl(TAC_ZOOM_OUT);

    if (ctrl)
    {
        ctrl->SetCallback(gMapMgr_zoom_out);
    }

    ctrl = (C_Button *)win->FindControl(BI_LIN_CTRL);

    if (ctrl)
        ctrl->SetCallback(OpenCrossSectionCB);

    ctrl = (C_Button *)win->FindControl(BI_LOG_CTRL);

    if (ctrl)
        ctrl->SetCallback(OpenCrossSectionCB);

    ctrl = (C_Button *)win->FindControl(BI_FIT_CTRL);

    if (ctrl)
        ctrl->SetCallback(FitFlightPlanCB);

    ctrl = (C_Button *) win->FindControl(ADD_FLIGHT);

    if (ctrl)
    {
        ctrl->SetCallback(SelectToolTypeCB);
    }

    ctrl = (C_Button *) win->FindControl(ADD_PACKAGE);

    if (ctrl)
    {
        ctrl->SetCallback(SelectToolTypeCB);
    }

    ctrl = (C_Button *) win->FindControl(ADD_BATTALION);

    if (ctrl)
    {
        ctrl->SetCallback(SelectToolTypeCB);
    }

    ctrl = (C_Button *) win->FindControl(ADD_NAVAL);

    if (ctrl)
    {
        ctrl->SetCallback(SelectToolTypeCB);
    }

    ctrl = (C_Button *) win->FindControl(ADD_SQUADRON);

    if (ctrl)
    {
        ctrl->SetCallback(SelectToolTypeCB);
    }

    ctrl = (C_Button *) win->FindControl(COPY_UNIT);

    if (ctrl)
    {
        ctrl->SetCallback(SelectToolTypeCB);
    }

    ctrl = (C_Button *) win->FindControl(DELETE_UNIT);

    if (ctrl)
    {
        ctrl->SetCallback(SelectToolTypeCB);
    }

    ctrl = (C_Button *) win->FindControl(EDIT_UNIT);

    if (ctrl)
    {
        ctrl->SetCallback(SelectToolTypeCB);
    }

    ctrl = (C_Button *) win->FindControl(TEAM_SELECTOR);

    if (ctrl)
    {
        ctrl->SetCallback(PickTeamCB);
    }
}

static void AddSquadronToAirbaseCB(long ID, short hittype, C_Base *ctrl)
{
    C_MapIcon *icon;
    UI_Refresher *urec;
    VU_ID id;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    icon = (C_MapIcon*)ctrl;

    if (ctrl)
    {
        ID = icon->GetIconID();
        urec = (UI_Refresher*)gGps->Find(ID);

        if (urec)
            tactical_add_squadron(urec->GetID());
    }
}

void SelectTargetCB(long ID, short hittype, C_Base *ctrl)
{
    C_MapIcon *icon;
    UI_Refresher *urec;
    VU_ID id;

    if (hittype not_eq C_TYPE_LMOUSEUP)
    {
        // We're either dragging or mouse down - check to see if we should drag a unit
        UnitCB(ID, hittype, ctrl);
        return;
    }

    icon = (C_MapIcon*)ctrl;

    if (ctrl)
    {
        ID = icon->GetIconID();
        urec = (UI_Refresher*)gGps->Find(ID);

        if (urec)
        {
            switch (CurMapTool->GetID())
            {
                case ADD_FLIGHT: // Click anywhere on the map to add a flight
                    tactical_add_flight(urec->GetID(), ctrl);
                    break;

                case ADD_PACKAGE: // Click anywhere on the map to add a package?
                    tactical_add_package(urec->GetID(), ctrl);
                    break;

                case ADD_BATTALION: // Click anywhere on the map to add a unit
                    tactical_add_battalion(urec->GetID(), ctrl);
                    break;

                case ADD_SQUADRON: // Click on an airbase to add a squadron
                    tactical_add_squadron(urec->GetID());
                    break;

                    /* case ADD_TASKFORCE: // Click on an airbase to add a squadron
                     tactical_add_taskforce(urec->GetID(),ctrl);
                     break;
                    */
                default:
                    break;
            }
        }
    }
}

static void SetToolbarDirections(long textid)
{
    C_Window *win;
    C_Text *txt;

    win = gMainHandler->FindWindow(TAC_FULLMAP_WIN);

    if (win)
    {
        txt = (C_Text*)win->FindControl(HELP_MESSAGE);

        if (txt)
        {
            txt->Refresh();
            txt->SetText(textid);
            txt->Refresh();
        }
    }

    win = gMainHandler->FindWindow(TAC_EDIT_WIN);

    if (win)
    {
        txt = (C_Text*)win->FindControl(HELP_MESSAGE);

        if (txt)
        {
            txt->Refresh();
            txt->SetText(textid);
            txt->Refresh();
        }
    }

    win = gMainHandler->FindWindow(TAC_VC_WIN);

    if (win)
    {
        txt = (C_Text*)win->FindControl(VC_HELP_MESSAGE);

        if (txt)
        {
            txt->Refresh();
            txt->SetText(textid);
            txt->Refresh();
        }
    }
}

void SelectToolTypeCB(long, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    gMapMgr->SetAllObjCallbacks(NULL);
    gMapMgr->SetAllAirUnitCallbacks(UnitCB);
    gMapMgr->SetAllGroundUnitCallbacks(UnitCB);
    gMapMgr->SetAllNavalUnitCallbacks(UnitCB);

    if (CurMapTool and CurMapTool == control)
    {
        CurMapTool->SetState(0);
        CurMapTool->Refresh();
        CurMapTool = NULL;
        SetToolbarDirections(TXT_SPACE);
    }
    else if ( not control)
    {
        if (CurMapTool)
        {
            CurMapTool->SetState(0);
            CurMapTool->Refresh();
            CurMapTool = NULL;
        }

        SetToolbarDirections(TXT_SPACE);
    }
    else
    {
        CurMapTool = control;

        switch (CurMapTool->GetID())
        {
            case ADD_FLIGHT:
                if (TheCampaign.Flags bitand CAMP_TACTICAL_EDIT)
                {
                    gMapMgr->SetAllObjCallbacks(SelectTargetCB);
                    gMapMgr->SetAllAirUnitCallbacks(SelectTargetCB);
                    gMapMgr->SetAllGroundUnitCallbacks(SelectTargetCB);
                    gMapMgr->SetAllNavalUnitCallbacks(SelectTargetCB);
                    SetToolbarDirections(control->GetUserNumber(1));
                }

                break;

            case ADD_PACKAGE:
                gMapMgr->SetAllObjCallbacks(SelectTargetCB);
                gMapMgr->SetAllAirUnitCallbacks(SelectTargetCB);
                gMapMgr->SetAllGroundUnitCallbacks(SelectTargetCB);
                gMapMgr->SetAllNavalUnitCallbacks(SelectTargetCB);
                SetToolbarDirections(control->GetUserNumber(1));
                break;

            case ADD_BATTALION: // Click anywhere on the map to add a unit
                if (TheCampaign.Flags bitand CAMP_TACTICAL_EDIT)
                {
                    gMapMgr->SetAllObjCallbacks(SelectTargetCB);
                    SetToolbarDirections(control->GetUserNumber(1));
                }

                break;

            case ADD_NAVAL: // Click anywhere on the map to add a unit
                if (TheCampaign.Flags bitand CAMP_TACTICAL_EDIT)
                {
                    gMapMgr->SetAllObjCallbacks(SelectTargetCB);
                    SetToolbarDirections(control->GetUserNumber(1));
                }

                break;

            case ADD_SQUADRON: // Click on an airbase to add a squadron
                if (TheCampaign.Flags bitand CAMP_TACTICAL_EDIT)
                {
                    gMapMgr->SetAllObjCallbacks(SelectTargetCB);
                    SetToolbarDirections(control->GetUserNumber(1));
                    gMapMgr->SetObjCallbacks(1, AddSquadronToAirbaseCB);
                }

                break;

            case COPY_UNIT: // Pick a unit to copy
                if (TheCampaign.Flags bitand CAMP_TACTICAL_EDIT)
                {
                    SetToolbarDirections(control->GetUserNumber(1));
                }

                break;

            case DELETE_UNIT: // Pick a unit to delete
                if (TheCampaign.Flags bitand CAMP_TACTICAL_EDIT)
                {
                    SetToolbarDirections(control->GetUserNumber(1));
                }

                break;

            case EDIT_UNIT: // Pick a unit to edit
                if (TheCampaign.Flags bitand CAMP_TACTICAL_EDIT)
                {
                    SetToolbarDirections(control->GetUserNumber(1));
                }

                break;

            case TARGET_VC:
                gMapMgr->SetAllObjCallbacks(SelectVCTargetCB);
                gMapMgr->SetAllAirUnitCallbacks(SelectVCTargetCB);
                gMapMgr->SetAllGroundUnitCallbacks(SelectVCTargetCB);
                gMapMgr->SetAllNavalUnitCallbacks(SelectVCTargetCB);
                SetToolbarDirections(control->GetUserNumber(1));
                break;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void tactical_draw_map(long, short, C_Base *)
{
    // gMapMgr->SetFlags(I_NEED_TO_DRAW_MAP);

    if (gMapMgr)
    {
        gMapMgr->DrawMap();
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static void tactical_maximize_map(long, short hittype, C_Base *control)
{
    C_Window
    *win;

    if (hittype not_eq C_TYPE_LMOUSEUP)
    {
        return;
    }

    win = gMainHandler->FindWindow(TAC_FULLMAP_WIN);

    if (win not_eq NULL)
    {
        gMapMgr->SetWindow(win);
        gMapMgr->DrawMap();
    }

    win = gMainHandler->FindWindow(TAC_PUA_MAP);

    if (win not_eq NULL)
    {
        gMainHandler->HideWindow(win);
    }

    gMainHandler->EnableWindowGroup(control->GetGroup());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static void tactical_minimize_map(long, short hittype, C_Base *control)
{
    C_Window
    *win;

    if (hittype not_eq C_TYPE_LMOUSEUP)
    {
        return;
    }

    win = gMainHandler->FindWindow(TAC_PUA_MAP);

    //MonoPrint ("Minimize %d %08x\n", control->GetGroup (), win);

    if (win not_eq NULL)
    {
        gMapMgr->SetWindow(win);
        gMapMgr->DrawMap();
    }

    win = gMainHandler->FindWindow(TAC_PUA_MAP);

    if (win not_eq NULL)
    {
        gMainHandler->ShowWindow(win);
    }

    gMainHandler->DisableWindowGroup(control->GetGroup());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


// callback for map events
void gMapMgr_TACmover(long, short hittype, C_Base *control)
{
    C_MapMover
    *mm;
    float mx, my, scale, maxy;
    short x = 0, y = 0;

    mm = (C_MapMover *) control;

    switch (hittype)
    {
        case (C_TYPE_MOUSEMOVE):
        {
            if (gMapMgr)
            {
                // JPO - this was null once...
                gMapMgr->MoveCenter(-((C_MapMover *)control)->GetHRange(), -((C_MapMover *)control)->GetVRange());
            }

            if (control)
            {
                control->Parent_->RefreshClient(0);
            }

            break;
        }

        case (C_TYPE_LMOUSEUP):
        {
            if (CurMapTool)
            {
                if (control->_GetCType_() == _CNTL_POPUPLIST_)
                {
                    gPopupMgr->GetCurrentXY(&x, &y);
                    gMapMgr->GetMapRelativeXY(&x, &y);
                }
                else if (control->_GetCType_() == _CNTL_MAP_MOVER_)
                {
                    x = static_cast<short>(((C_MapMover*)control)->GetRelX() + control->GetX() + control->Parent_->GetX());
                    y = static_cast<short>(((C_MapMover*)control)->GetRelY() + control->GetY() + control->Parent_->GetY());
                    gMapMgr->GetMapRelativeXY(&x, &y);
                }

                scale = gMapMgr->GetMapScale();
                maxy = gMapMgr->GetMaxY();

                mx = x / scale;
                my = maxy - y / scale;

                SimMapX = my;
                SimMapY = mx;

                MapX = SimToGrid(mx);
                MapY = SimToGrid(my);

                switch (CurMapTool->GetID())
                {
                    case ADD_FLIGHT: // Click anywhere on the map to add a flight
                        tactical_add_flight(FalconNullId, control);
                        break;

                    case ADD_PACKAGE: // Click anywhere on the map to add a package?
                        tactical_add_package(FalconNullId, control);
                        break;

                    case ADD_BATTALION: // Click anywhere on the map to add a unit
                        tactical_add_battalion(FalconNullId, control);
                        break;

                    case ADD_NAVAL: // Click anywhere on the map to add a unit
                        break;

                    case ADD_SQUADRON: // Click on an airbase to add a squadron
                        break;

                    case COPY_UNIT: // Pick a unit to copy
                        break;

                    case DELETE_UNIT: // Pick a unit to delete
                        break;

                    case EDIT_UNIT: // Pick a unit to edit
                        break;
                }
            }

            break;
        }

        case (C_TYPE_MOUSEWHEEL):
        {
            if ( not control->IsControl())
            {
                break;
            }

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
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static void gMapMgr_zoom_in(long, short hittype, C_Base *ctrl)
{
    F4CSECTIONHANDLE *Leave;

    if ((hittype not_eq C_TYPE_LMOUSEUP) and (hittype not_eq C_TYPE_REPEAT))
        return;

    Leave = UI_Enter(ctrl->Parent_);
    gMapMgr->ZoomIn();
    gMapMgr->DrawMap();
    UI_Leave(Leave);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static void gMapMgr_zoom_out(long, short hittype, C_Base *ctrl)
{
    F4CSECTIONHANDLE *Leave;

    if ((hittype not_eq C_TYPE_LMOUSEUP) and (hittype not_eq C_TYPE_REPEAT))
        return;

    Leave = UI_Enter(ctrl->Parent_);
    gMapMgr->ZoomOut();
    gMapMgr->DrawMap();
    UI_Leave(Leave);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void gMapMgr_menu(long ID, short hittype, C_Base *control)
{
    int
    objective = -1,
    zoom;

    if (hittype not_eq C_TYPE_LMOUSEUP)
    {
        return;
    }

    switch (ID)
    {
        case MID_RECON:
        {
            MonoPrint("Map Menu Recon\n");

            gPopupMgr->CloseMenu();
            break;
        }

        case MID_ADD_BATTALION:
        {
            tactical_add_battalion(FalconNullId, control);

            gPopupMgr->CloseMenu();
            break;
        }

        case MID_ADD_PACKAGE:
        {
            tactical_add_package(FalconNullId, control);

            gPopupMgr->CloseMenu();
            break;
        }

        case MID_ZOOM_IN:
        {
            zoom = gMapMgr->GetZoomLevel();

            if (zoom >= 64) // MAX_ZOOM_LEVEL
            {
                return;
            }

            zoom *= 2;

            //MonoPrint ("Zoom In %d\n", zoom);

            gMapMgr->SetZoomLevel(static_cast<short>(zoom));
            gMapMgr->DrawMap();

            gPopupMgr->CloseMenu();
            break;
        }

        case MID_ZOOM_OUT:
        {
            zoom = gMapMgr->GetZoomLevel();

            if (zoom <= 1) // MAX_ZOOM_LEVEL
            {
                return;
            }

            zoom /= 2;

            //MonoPrint ("Zoom In %d\n", zoom);

            gMapMgr->SetZoomLevel(static_cast<short>(zoom));
            gMapMgr->DrawMap();

            gPopupMgr->CloseMenu();
            break;
        }

        case MID_INST_AF:
            objective = _OBTV_AIR_FIELDS;
            break;

        case MID_INST_AD:
            objective = _OBTV_AIR_DEFENSE;
            break;

        case MID_INST_ARMY:
            objective = _OBTV_ARMY;
            break;

        case MID_INST_CCC:
            objective = _OBTV_CCC;
            break;

        case MID_INST_POLITICAL:
            objective = _OBTV_POLITICAL;
            break;

        case MID_INST_INFRA:
            objective = _OBTV_INFRASTRUCTURE;
            break;

        case MID_INST_LOG:
            objective = _OBTV_LOGISTICS;
            break;

        case MID_INST_WARPROD:
            objective = _OBTV_WAR_PRODUCTION;
            break;

        case MID_INST_NAV:
            objective = _OBTV_NAVIGATION;
            break;

        case MID_INST_OTHER:
            objective = _OBTV_OTHER;
            break;

        case MID_INST_NAVAL:
            objective = _OBTV_NAVAL;
            break;

            break;

        case MID_UNITS_COMBAT:
        {
            gMapMgr->SetUnitLevel(2);

            if (((C_PopupList *)control)->GetItemState(ID))
                gMapMgr->ShowUnitType(_UNIT_COMBAT);
            else
                gMapMgr->HideUnitType(_UNIT_COMBAT);

            gMapMgr->DrawMap();

            break;
        }

        case MID_UNITS_AD:
        {
            gMapMgr->SetUnitLevel(2);

            if (((C_PopupList *)control)->GetItemState(ID))
                gMapMgr->ShowUnitType(_UNIT_AIR_DEFENSE);
            else
                gMapMgr->HideUnitType(_UNIT_AIR_DEFENSE);

            gMapMgr->DrawMap();

            break;
        }

        case MID_UNITS_SUPPORT:
        {
            gMapMgr->SetUnitLevel(2);

            if (((C_PopupList *)control)->GetItemState(ID))
                gMapMgr->ShowUnitType(_UNIT_SUPPORT);
            else
                gMapMgr->HideUnitType(_UNIT_SUPPORT);

            gMapMgr->DrawMap();

            break;
        }

        case MID_UNITS_SQUAD_FIGHTER:
        {
            if (((C_PopupList *)control)->GetItemState(ID))
                gMapMgr->ShowAirUnitType(_UNIT_FIGHTER);
            else
                gMapMgr->HideAirUnitType(_UNIT_FIGHTER);

            gMapMgr->DrawMap();

            break;
        }

        case MID_UNITS_SQUAD_ATTACK:
        {
            if (((C_PopupList *)control)->GetItemState(ID))
                gMapMgr->ShowAirUnitType(_UNIT_ATTACK);
            else
                gMapMgr->HideAirUnitType(_UNIT_ATTACK);

            gMapMgr->DrawMap();

            break;
        }

        case MID_UNITS_SQUAD_BOMBER:
        {
            if (((C_PopupList *)control)->GetItemState(ID))
                gMapMgr->ShowAirUnitType(_UNIT_BOMBER);
            else
                gMapMgr->HideAirUnitType(_UNIT_BOMBER);

            gMapMgr->DrawMap();

            break;
        }

        case MID_UNITS_SQUAD_SUPPORT:
        {
            if (((C_PopupList *)control)->GetItemState(ID))
                gMapMgr->ShowAirUnitType(_UNIT_SUPPORT);
            else
                gMapMgr->HideAirUnitType(_UNIT_SUPPORT);

            gMapMgr->DrawMap();

            break;
        }

        case MID_UNITS_SQUAD_HELI:
        {
            if (((C_PopupList *)control)->GetItemState(ID))
                gMapMgr->ShowAirUnitType(_UNIT_HELICOPTER);
            else
                gMapMgr->HideAirUnitType(_UNIT_HELICOPTER);

            gMapMgr->DrawMap();

            break;
        }

        case MID_LEG_NAMES:
        {
            if (((C_PopupList *)control)->GetItemState(ID))
            {
                gMapMgr->TurnOnNames();
            }
            else
            {
                gMapMgr->TurnOffNames();
            }

            gMapMgr->DrawMap();

            return;
        }

        default:
        {
            //MonoPrint ("Map Menu %d %08x\n", ID, control);

            gMapMgr->DrawMap();

            gPopupMgr->CloseMenu();
            break;
        }
    }

    switch (ID)
    {
        case MID_INST_AF:
        case MID_INST_AD:
        case MID_INST_ARMY:
        case MID_INST_CCC:
        case MID_INST_POLITICAL:
        case MID_INST_INFRA:
        case MID_INST_LOG:
        case MID_INST_WARPROD:
        case MID_INST_NAV:
        case MID_INST_OTHER:
        case MID_INST_NAVAL:
        {
            if (((C_PopupList *)control)->GetItemState(ID))
            {
                gMapMgr->ShowObjectiveType(objective);
            }
            else
            {
                gMapMgr->HideObjectiveType(objective);
            }

            gMapMgr->DrawMap();

            return;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void tactical_objective_menu(long ID, short, C_Base *)
{
    C_Base
    *caller;

    C_MapIcon
    *icon;

    C_DrawList
    *piggy;

    C_TreeList
    *tree;

    long
    iconid;

    TREELIST
    *item;

    UI_Refresher
    *urec;

    urec = NULL;

    caller = gPopupMgr->GetCallingControl();

    if (caller == NULL)
    {
        return;
    }

    if (caller->_GetCType_() == _CNTL_MAPICON_)
    {
        icon = (C_MapIcon *) caller;

        iconid = icon->GetIconID();

        urec = (UI_Refresher *) gGps->Find(iconid);
    }
    else if (caller->_GetCType_() == _CNTL_DRAWLIST_)
    {
        piggy = (C_DrawList *) caller;

        iconid = piggy->GetIconID();

        urec = (UI_Refresher *) gGps->Find(iconid);
    }
    else if (caller->_GetCType_() == _CNTL_TREELIST_)
    {
        tree = (C_TreeList *) caller;

        item = tree->GetLastItem();

        if (item)
        {
            urec = (UI_Refresher *) gGps->Find(item->ID_);
        }
    }

    switch (ID)
    {
        case MID_RECON:
        {
            //MonoPrint ("Recon\n");

            gPopupMgr->CloseMenu();

            break;
        }

        case MID_ADD_PACKAGE:
        {
            tactical_add_package(urec->GetID(), caller);

            gPopupMgr->CloseMenu();
            break;
        }

        case MID_ADD_VC:
        {
            tactical_add_victory_condition(urec->GetID(), NULL);

            gPopupMgr->CloseMenu();
            break;
        }

        case MID_SET_OWNER:
        {
            //MonoPrint ("Set Owner\n");
            break;
        }

        case MID_SQUADRONS:
        {
            //MonoPrint ("Squadrons\n");
            break;
        }

        case MID_ALTERNATE_FIELD:
        {
            //MonoPrint ("Alternate Field %d\n", ((C_PopupList *)control)->GetItemState(ID));
            break;
        }

        default:
        {
            //MonoPrint ("Unknown Object Menu Item\n");

            gPopupMgr->CloseMenu();

            break;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void tactical_airbase_menu(long ID, short, C_Base *control)
{
    C_Base
    *caller;

    long
    iconid;

    C_MapIcon
    *icon;

    C_DrawList
    *piggy;

    C_TreeList
    *tree;

    TREELIST
    *item;

    UI_Refresher
    *urec;

    urec = NULL;

    caller = gPopupMgr->GetCallingControl();

    if (caller == NULL)
    {
        return;
    }

    if (caller->_GetCType_() == _CNTL_MAPICON_)
    {
        icon = (C_MapIcon *) caller;

        iconid = icon->GetIconID();

        urec = (UI_Refresher *) gGps->Find(iconid);
    }
    else if (caller->_GetCType_() == _CNTL_DRAWLIST_)
    {
        piggy = (C_DrawList *) caller;

        iconid = piggy->GetIconID();

        urec = (UI_Refresher *) gGps->Find(iconid);
    }
    else if (caller->_GetCType_() == _CNTL_TREELIST_)
    {
        tree = (C_TreeList *) caller;

        item = tree->GetLastItem();

        if (item)
        {
            urec = (UI_Refresher *) gGps->Find(item->ID_);
        }
    }

    switch (ID)
    {
        case MID_RECON:
        {
            //MonoPrint ("Recon\n");

            gPopupMgr->CloseMenu();

            break;
        }

        case MID_ADD_PACKAGE:
        {
            tactical_add_package(urec->GetID(), control);

            gPopupMgr->CloseMenu();
            break;
        }

        case MID_ADD_VC:
        {
            tactical_add_victory_condition(urec->GetID(), NULL);

            gPopupMgr->CloseMenu();
            break;
        }

        case MID_SET_OWNER:
        {
            //MonoPrint ("Set Owner\n");

            gPopupMgr->CloseMenu();
            break;
        }

        case MID_ADD_SQUADRON:
        {
            tactical_add_squadron(urec->GetID());

            gPopupMgr->CloseMenu();
            break;
        }

        case MID_ALTERNATE_FIELD:
        {
            //MonoPrint ("Alternate Field %d\n", ((C_PopupList *)control)->GetItemState(ID));
            break;
        }

        default:
        {
            //MonoPrint ("Unknown Object Menu Item\n");

            gPopupMgr->CloseMenu();

            break;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static void update_gMapMgr(void)
{
    // This is NOW handled by the GPS
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void tactical_update_campaign_entities(void)
{
    // This is NOW handled by the GPS
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
