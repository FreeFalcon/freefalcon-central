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
#include "division.h"
#include "misseval.h"
#include "cmpclass.h"
#include "msginc/WingmanMsg.h"
#include "ui95_dd.h"
#include "chandler.h"
#include "ui95_ext.h"
#include "AirUnit.h"
#include "uicomms.h"
#include "userids.h"
#include "textids.h"
#include "classtbl.h"
#include "ui_cmpgn.h"
#include "cmap.h"
#include "gps.h"
#include "urefresh.h"

#pragma warning(disable : 4127)

#define WPERROR_NO_TARGET 0x01
#define WPERROR_SPEED 0x02
#define WPERROR_TIME 0x04
#define WPERROR_FUEL 0x08

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

float get_ground_speed(float speed, int altitude);
float get_air_speed(float speed, int altitude);
void tactical_set_orders(Battalion bat, VU_ID obj, GridIndex tx, GridIndex ty);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void refresh_waypoint(WayPointClass * wp);
void recalculate_waypoints(WayPointClass *wp);
void recalculate_waypoint(WayPointClass *wp, int minSpeed, int maxSpeed);
void fixup_unit(Unit unit);
int IsValidAction(int mission, int action);
int IsValidEnrouteAction(int mission, int action);
int IsValidWP(WayPointClass *wp, Flight flt);
void ValidateWayPoints(Flight flt);
int WayPointErrorCode(WayPointClass *wp, Flight flt);
void DeleteWPCB(long ID, short hittype, C_Base *control);
void ChangeTimeCB(long ID, short hittype, C_Base *control);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern C_Handler *gMainHandler;
extern GlobalPositioningSystem *gGps;
extern C_Map *gMapMgr;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VU_ID gActiveFlightID = FalconNullId; // Currently selected waypoint flight
short gActiveWPNum = 0; // Current Waypoint in this list

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern bool g_bAnyWaypointTask; //Wombat778 9-27-2003

static WayPointClass *get_current_waypoint(void)
{
    int
    i;

    FlightClass
    *flt;

    WayPointClass
    *wp;

    flt = (Flight) vuDatabase->Find(gActiveFlightID);

    if ( not flt)
        return NULL;

    wp = flt->GetFirstUnitWP();
    i = 1;

    while (wp and i < gActiveWPNum)
    {
        wp = wp->GetNextWP();
        i ++;
    }

    return wp;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// KCK: This will set up the "special" data associated with certain actions/flags
static void SetSteerPointValues(C_Window *win, WayPointClass *wp, int)
{
    CampEntity ent;
    Flight flt = NULL;
    long hr, mn, sc, action, flags, fid;
    CampaignTime time;
    C_Text *txt = NULL;
    C_Clock *clk;
    VU_ID *tmpID;
    _TCHAR buffer[130];

    flt = (Flight)vuDatabase->Find(gActiveFlightID);

    if ( not flt or not wp)
        return;

    // Depending on a combination of waypoint flags and action type, we may have special
    // data associated with this waypoint
    action = wp->GetWPAction();
    flags = wp->GetWPFlags();

    if ((flags bitand WPF_TAKEOFF) or (flags bitand WPF_LAND))
    {
        // Associate an airbase/carrier
        ent = wp->GetWPTarget();
        txt = (C_Text *)win->FindControl(LANDING_FIELD); // Takeoff/landing airbase name

        if (txt)
        {
            tmpID = (VU_ID*)txt->GetUserPtr(_UI95_VU_ID_SLOT_);

            if (ent)
            {
                tmpID = new VU_ID;
                *tmpID = ent->Id();
                txt->SetUserCleanupPtr(_UI95_VU_ID_SLOT_, tmpID);
                tmpID = NULL;

                if (ent->IsObjective())
                    ent->GetName(buffer, 30, TRUE);
                else
                    ent->GetName(buffer, 30, FALSE);

                txt->SetText(buffer);
            }
            else
            {
                if (tmpID)
                    txt->SetUserPtr(_UI95_VU_ID_SLOT_, NULL);

                _tcscpy(buffer, gStringMgr->GetString(TXT_NO_AIRBASE));
                txt->SetText(buffer);
            }

            txt->Refresh();
        }
    }

    if (((flags bitand WPF_TARGET) and (action == WP_INTERCEPT or action == WP_NAVSTRIKE or action == WP_GNDSTRIKE)) or action == WP_PICKUP or action == WP_AIRDROP)
    {
        // Associate a UNIT target
        ent = wp->GetWPTarget();

        if (ent)
        {
            if (ent->IsFlight())
                GetCallsign(((Flight)ent)->callsign_id, ((Flight)ent)->callsign_num, buffer);
            else if (ent->IsObjective())
                ent->GetName(buffer, 30, TRUE);
            else
                ent->GetName(buffer, 30, FALSE);
        }
        else
            _tcscpy(buffer, gStringMgr->GetString(TXT_NO_TARGET));

        txt = (C_Text *)win->FindControl(UNIT_FIELD); // Target Name

        if (txt)
        {
            tmpID = (VU_ID*)txt->GetUserPtr(_UI95_VU_ID_SLOT_);

            if (ent)
            {
                tmpID = new VU_ID;
                *tmpID = ent->Id();
                txt->SetUserCleanupPtr(_UI95_VU_ID_SLOT_, tmpID);
                tmpID = NULL;
            }
            else if (tmpID)
                txt->SetUserPtr(_UI95_VU_ID_SLOT_, NULL);

            txt->SetText(buffer);
            txt->Refresh();
        }
    }

    if ((flags bitand WPF_TARGET) and (action == WP_STRIKE or action == WP_BOMB or action == WP_RECON))
    {
        // Associate an OBJECTIVE target
        ent = wp->GetWPTarget();

        if (ent)
        {
            if (ent->IsObjective())
                ent->GetName(buffer, 30, TRUE);
            else
                ent->GetName(buffer, 30, FALSE);
        }
        else
            _tcscpy(buffer, gStringMgr->GetString(TXT_NO_TARGET));

        txt = (C_Text *)win->FindControl(OBJECTIVE_FIELD); // Target Name

        if (txt)
        {
            tmpID = (VU_ID*)txt->GetUserPtr(_UI95_VU_ID_SLOT_);

            if (ent)
            {
                tmpID = new VU_ID;
                *tmpID = ent->Id();
                txt->SetUserCleanupPtr(_UI95_VU_ID_SLOT_, tmpID);
                tmpID = NULL;
            }
            else if (tmpID)
                txt->SetUserPtr(_UI95_VU_ID_SLOT_, NULL);

            txt->SetText(buffer);
            txt->Refresh();
        }

        txt = (C_Text *)win->FindControl(FEATURE_FIELD); // Target building

        if (txt and ent and ent->IsObjective())
        {
            FeatureClassDataType *fc;
            fid = wp->GetWPTargetBuilding();

            if (fid < 255)
                fc = GetFeatureClassData(((Objective)ent)->GetFeatureID(fid));
            else
                fc = NULL;

            txt->SetUserNumber(C_STATE_0, wp->GetWPTargetBuilding());

            if (fc)
                txt->SetText(fc->Name);
            else
            {
                _tcscpy(buffer, gStringMgr->GetString(TXT_NOT_ASSIGNED));
                txt->SetText(buffer);
            }

            txt->Refresh();
        }
    }

    if ((flags bitand WPF_REPEAT) or (wp->GetNextWP() and (wp->GetNextWP()->GetWPFlags() bitand WPF_REPEAT)))
    {
        // Associate a patrol/station time
        if (flags bitand WPF_REPEAT)
            time = wp->GetWPStationTime();
        else
            time = wp->GetNextWP()->GetWPStationTime();

        hr = time / CampaignHours;
        mn = (time / CampaignMinutes) % 60;
        sc = (time / CampaignSeconds) % 60;
        clk = (C_Clock *)win->FindControl(PATROL_FIELD);

        if (clk)
        {
            clk->SetHour(hr);
            clk->SetMinute(mn);
            clk->SetSecond(sc);
            clk->Refresh();
        }
    }

    if (action == WP_PICKUP or action == WP_AIRDROP)
    {
        // Associate a patrol/station time
        time = wp->GetWPStationTime();
        hr = time / CampaignHours;
        mn = (time / CampaignMinutes) % 60;
        sc = (time / CampaignSeconds) % 60;
        clk = (C_Clock *)win->FindControl(LOITER_FIELD);

        if (clk)
        {
            clk->SetHour(hr);
            clk->SetMinute(mn);
            clk->SetSecond(sc);
            clk->Refresh();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void RefreshActionClusters(int action, C_Window *win)
{
    win->HideCluster(13063);
    win->HideCluster(13065);
    win->HideCluster(13068);
    win->HideCluster(13070);
    win->HideCluster(13074);
    win->HideCluster(13075);
    win->HideCluster(13079);

    switch (action)
    {
        case WP_TAKEOFF:
        case WP_LAND:
            win->UnHideCluster(13068);
            break;

        case WP_TANKER:
        case WP_REARM:
        case WP_ELINT:
        case WP_JAM:
        case WP_CAP:
        case WP_CASCP:
            win->UnHideCluster(13070);
            break;

        case WP_INTERCEPT:
            win->UnHideCluster(13074);
            break;

        case WP_PICKUP:
        case WP_AIRDROP:
            win->UnHideCluster(13074);
            win->UnHideCluster(13075);
            break;

        case WP_STRIKE:
        case WP_BOMB:
            win->UnHideCluster(13079);
            break;

        default:
            win->UnHideCluster(13063);
            break;
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UpdateWaypointWindowInfo(C_Window *win, WayPointClass *wp, int wpnum, int callsign_update = TRUE)
{
    long ispeed, errors;
    long alt_delta;
    CampaignTime tm;
    C_Text *txt;
    C_EditBox *ebox;
    C_ListBox *lbox;
    C_Button *btn;
    C_Clock *clk;
    Package Pkg;
    Flight flt, curflt;
    WayPointClass *prev_wp;
    _TCHAR buffer[90];

    flt = (Flight)vuDatabase->Find(gActiveFlightID);

    if ( not win or not wp or not flt)
        return;

    SetSteerPointValues(win, wp, wpnum);
    RefreshActionClusters(wp->GetWPAction(), win);

    prev_wp = wp->GetPrevWP();

    btn = (C_Button *) win->FindControl(PREV_STPT);

    if (prev_wp)
        btn->SetFlagBitOff(C_BIT_INVISIBLE);
    else
        btn->SetFlagBitOn(C_BIT_INVISIBLE);

    btn = (C_Button *) win->FindControl(NEXT_STPT);

    if (wp->GetNextWP())
        btn->SetFlagBitOff(C_BIT_INVISIBLE);
    else
        btn->SetFlagBitOn(C_BIT_INVISIBLE);

    gActiveWPNum = static_cast<short>(wpnum);

    txt = (C_Text *)win->FindControl(FLIGHT_TITLE);

    if (txt and flt->GetUnitParent())
    {
        _stprintf(buffer, "%s %1d", gStringMgr->GetString(TXT_PKG), ((Package)flt->GetUnitParent())->GetCampID());
        txt->SetText(buffer);
    }

    btn = (C_Button *) win->FindControl(DELETE_WAYPOINT);

    if (btn)
        btn->SetCallback(DeleteWPCB);

    if (callsign_update)
    {
        lbox = (C_ListBox*)win->FindControl(LID_FLIGHT_CALLSIGN);

        if (lbox)
        {
            lbox->RemoveAllItems();
            Pkg = (Package)flt->GetUnitParent();

            if (Pkg)
            {
                curflt = (Flight)Pkg->GetFirstUnitElement();

                while (curflt)
                {
                    GetCallsign(curflt, buffer);
                    lbox->AddItem(curflt->GetCampID(), C_TYPE_ITEM, buffer);
                    curflt = (Flight)Pkg->GetNextUnitElement();
                }

                lbox->SetFlagBitOn(C_BIT_ENABLED);
            }
            else
            {
                GetCallsign(flt, buffer);
                lbox->AddItem(flt->GetCampID(), C_TYPE_ITEM, buffer);
                lbox->SetFlagBitOff(C_BIT_ENABLED);
            }

            lbox->SetValue(flt->GetCampID());
        }
    }

    lbox = (C_ListBox *)win->FindControl(STPT_LABEL);

    if (lbox)
    {
        if (wp->GetWPFlags() bitand WPF_TARGET)
            lbox->SetValue(TGT_WP);
        else if (wp->GetWPFlags() bitand WPF_IP)
            lbox->SetValue(IP_WP);
        else if (wp->GetWPFlags() bitand WPF_CP)
            lbox->SetValue(CP_WP);
        else
            lbox->SetValue(STPT_WP);
    }

    ebox = (C_EditBox *)win->FindControl(STPT_FIELD);

    if (ebox)
    {
        ebox->SetInteger(wpnum);
        ebox->Refresh();
    }

    btn = (C_Button *) win->FindControl(TOS_LOCK);

    if (btn)
    {
        if (wp->GetWPFlags() bitand WPF_TIME_LOCKED)
            btn->SetState(1);
        else
            btn->SetState(0);
    }

    clk = (C_Clock *)win->FindControl(TOS_FIELD);

    if (clk)
    {
        tm = wp->GetWPArrivalTime() / VU_TICS_PER_SECOND;
        // tm += 30*CampaignMinutes; // What the hell is this about? - RH
        clk->SetTime(tm);
        clk->Refresh();
    }

    btn = (C_Button *) win->FindControl(AIRSPEED_LOCK);

    if (btn)
    {
        if (wp->GetWPFlags() bitand WPF_SPEED_LOCKED)
            btn->SetState(1);
        else
            btn->SetState(0);
    }

    txt = (C_Text *)win->FindControl(AIRSPEED_FIELD);

    if (txt)
    {
        if (prev_wp)
        {
            float speed;

            // Air Speed in nm/hr
            if (wp->GetWPFlags() bitand WPF_HOLDCURRENT)
                speed = static_cast<float>(FloatToInt32(get_air_speed(wp->GetWPSpeed() * KM_TO_NM, prev_wp->GetWPAltitude())));
            else
                speed = static_cast<float>(FloatToInt32(get_air_speed(wp->GetWPSpeed() * KM_TO_NM, wp->GetWPAltitude())));

            ispeed = FloatToInt32(speed + 2.5F) / 5;
            ispeed *= 5;

            if (speed > 999) ispeed = 999;
        }
        else
        {
            ispeed = 0;
        }

        _stprintf(buffer, "%1ld %s", ispeed, gStringMgr->GetString(TXT_KTS));

        txt->SetText(buffer);
    }

    txt = (C_Text *) win->FindControl(ALT_FIELD);

    if (txt)
    {
        _stprintf(buffer, "%1ld", wp->GetWPAltitude());
        txt->SetText(buffer);
    }

    // Figure out whether we have an altitude change and show the appropriate choices
    if (prev_wp)
        alt_delta = wp->GetWPAltitude() - prev_wp->GetWPAltitude();
    else
        alt_delta = 0;

    if (alt_delta < 0)
    {
        win->HideCluster(200);
        win->UnHideCluster(300);
        win->UnHideCluster(100);
    }
    else if (alt_delta > 0)
    {
        win->HideCluster(300);
        win->UnHideCluster(200);
        win->UnHideCluster(100);
    }
    else
    {
        win->HideCluster(200);
        win->HideCluster(300);
        win->HideCluster(100);
    }

    lbox = (C_ListBox *)win->FindControl(CLIMB_LISTBOX);

    if (lbox)
    {
        if (wp->GetWPFlags() bitand WPF_HOLDCURRENT)
            lbox->SetValue(CLIMB_DELAY);
        else
            lbox->SetValue(CLIMB_IMMEDIATE);
    }

    lbox = (C_ListBox *)win->FindControl(FORM_LISTBOX);

    if (lbox)
    {
        lbox->SetValue(wp->GetWPFormation() + 1);
    }

    lbox = (C_ListBox *)win->FindControl(ENR_ACTION_LISTBOX);

    if (lbox)
    {
        lbox->SetValue(wp->GetWPRouteAction() + 1);
    }

    lbox = (C_ListBox *)win->FindControl(ACTION_LISTBOX);

    if (lbox)
    {
        lbox->SetValue(wp->GetWPAction() + 1);
        // SteerPointTypeCB(ACTION_LISTBOX,C_TYPE_SELECT,lbox);
    }

    // Draw Error boxes
    errors = WayPointErrorCode(wp, flt);
    win->HideCluster(80);
    win->HideCluster(81);
    win->HideCluster(82);

    if (errors bitand WPERROR_NO_TARGET)
        win->UnHideCluster(82);

    if (errors bitand WPERROR_SPEED)
        win->UnHideCluster(81);

    if (errors bitand WPERROR_TIME)
        win->UnHideCluster(80);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void GotoPrevWaypointCB(long, short hittype, C_Base *control)
{
    C_Window *win;
    Flight flt;
    WayPoint wp, prevwp;
    int i;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    if (gActiveFlightID == FalconNullId)
        return;

    if (gActiveWPNum < 2)
        return;

    win = control->Parent_;

    if (win)
    {
        gActiveWPNum--;

        flt = (Flight)FindUnit(gActiveFlightID);

        if (flt == NULL) return;

        wp = flt->GetFirstUnitWP();
        i = 1;
        prevwp = wp;

        while (i < gActiveWPNum and wp)
        {
            prevwp = wp;
            wp = wp->GetNextWP();
            i++;
        }

        if (wp)
        {
            UpdateWaypointWindowInfo(win, wp, i);
            win->RefreshWindow();
        }
    }
}

void GotoNextWaypointCB(long, short hittype, C_Base *control)
{
    C_Window *win;
    Flight flt;
    WayPoint wp, prevwp;
    int i;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    if (gActiveFlightID == FalconNullId)
        return;

    win = control->Parent_;

    if (win)
    {
        flt = (Flight)FindUnit(gActiveFlightID);

        if (flt == NULL)
            return;

        wp = flt->GetFirstUnitWP();
        i = 0;
        prevwp = wp;

        while (i < gActiveWPNum and wp)
        {
            prevwp = wp;
            wp = wp->GetNextWP();
            i++;
        }

        if (wp)
        {
            gActiveWPNum++;
            UpdateWaypointWindowInfo(win, wp, gActiveWPNum);
            win->RefreshWindow();
        }
    }
}

void DeleteWPCB(long, short hittype, C_Base *)
{
    WayPoint wp, nw;
    Flight flt;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    flt = (Flight)vuDatabase->Find(gActiveFlightID);
    wp = get_current_waypoint();

    if ( not wp or not flt)
        return;

    nw = wp->GetPrevWP();

    if ((wp->GetPrevWP()) and (wp->GetNextWP()))
    {
        flt->DeleteUnitWP(wp);
    }

    recalculate_waypoints(nw);
    gMapMgr->SetCurrentWaypointList(flt->Id());

    if (TheCampaign.Flags bitand CAMP_TACTICAL_EDIT)
    {
        Flight un;

        un = (Flight)vuDatabase->Find(gMapMgr->GetCurWPID());

        if (un)
        {
            fixup_unit(un);
            gGps->Update();
            gMapMgr->DrawMap();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SetupFlightSpecificControls(Flight flt)
{
    C_PopupList *menu;
    C_ListBox *lbox;
    C_Window *win;
    int i, mission;

    if ( not flt) // it is possible to this to be null... so avoid PJW
        return;

    mission = flt->GetUnitMission();
    win = gMainHandler->FindWindow(FLIGHT_PLAN_WIN);
    menu = gPopupMgr->GetMenu(STEERPOINT_POP);

    // Setup our possible waypoint actions
    lbox = (C_ListBox *)win->FindControl(ACTION_LISTBOX);

    if (lbox)
    {
        // KCK NOTE: Peter mentioned that this should eventually simply disable invalid
        // actions, not rebuild the whole thingy.
        lbox->RemoveAllItems();

        for (i = 0; i < WP_LAST; i++)
        {
            if (IsValidAction(mission, i) or g_bAnyWaypointTask)    //Wombat778 9-27-2003 added or g_bAnyWaypointTask to allow selection of all tasks
            {
                if ( not i)
                    lbox->AddItem(i + 1, C_TYPE_ITEM, WPActStr[39]);
                else
                    lbox->AddItem(i + 1, C_TYPE_ITEM, WPActStr[i]);

                menu->SetItemFlagBitOff(i bitor 0x200, C_BIT_INVISIBLE);
            }
            else
            {
                menu->SetItemFlagBitOn(i bitor 0x200, C_BIT_INVISIBLE);
            }
        }
    }

    // Setup our possible enroute actions
    lbox = (C_ListBox *)win->FindControl(ENR_ACTION_LISTBOX);

    if (lbox)
    {
        lbox->RemoveAllItems();

        for (i = 0; i < WP_LAST; i++)
        {
            if (IsValidEnrouteAction(mission, i))
            {
                if ( not i)
                    lbox->AddItem(i + 1, C_TYPE_ITEM, WPActStr[39]);
                else
                    lbox->AddItem(i + 1, C_TYPE_ITEM, WPActStr[i]);

                menu->SetItemFlagBitOff(i bitor 0x100, C_BIT_INVISIBLE);
            }
            else
            {
                menu->SetItemFlagBitOn(i bitor 0x100, C_BIT_INVISIBLE);
            }
        }
    }

}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// ListBox Callback...
void GotoFlightCB(long ID, short hittype, C_Base *control)
{
    C_Window *win;
    WayPoint wp, prevwp = NULL;
    Package Pkg;
    Flight flt, curflt;
    int i;

    if (hittype not_eq C_TYPE_SELECT)
        return;

    ID = ((C_ListBox*)control)->GetTextID();

    win = gMainHandler->FindWindow(FLIGHT_PLAN_WIN);

    if (win)
    {
        flt = (Flight)vuDatabase->Find(gActiveFlightID);

        if (flt == NULL)
            return;

        if (flt->GetCampID() not_eq ID)
        {
            Pkg = (Package)flt->GetUnitParent();

            if (Pkg)
            {
                curflt = (Flight)Pkg->GetFirstUnitElement();

                while (curflt and curflt->GetCampID() not_eq ID)
                    curflt = (Flight)Pkg->GetNextUnitElement();
            }
            else
                curflt = NULL;

            flt = curflt;
        }

        if ( not flt)
            return;

        if (gActiveFlightID not_eq flt->Id())
        {
            gActiveFlightID = flt->Id();
            ValidateWayPoints(flt);
        }

        wp = flt->GetFirstUnitWP();
        prevwp = wp;
        i = 1;

        while (i < gActiveWPNum and wp)
        {
            prevwp = wp;
            wp = wp->GetNextWP();
            i++;
        }

        if (wp)
        {
            UpdateWaypointWindowInfo(win, wp, i, FALSE);
            win->RefreshWindow();
            gMainHandler->ShowWindow(win);
            gMainHandler->WindowToFront(win);
            gActiveWPNum = static_cast<short>(i);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//sfr: this is called when a player changes the time of a waypoint. We MUST make waypoint dirty
// to update other flights
void ChangeTOSCB(long ID, short hittype, C_Base *control)
{
    C_Clock *clk;
    long t;
    WayPoint wp;

    if ((hittype not_eq C_TYPE_LMOUSEUP) and (hittype not_eq C_TYPE_LMOUSEDBLCLK) and (hittype not_eq C_TYPE_REPEAT))
        return;

    clk = (C_Clock*)control->Parent_->FindControl(control->GetUserNumber(0));

    if (clk)
    {
        ChangeTimeCB(ID, hittype, control);

        wp = get_current_waypoint();

        if (wp)
        {
            wp->SetWPFlag(WPF_TIME_LOCKED);
            t = clk->GetTime() * VU_TICS_PER_SECOND;
            wp->SetWPTimes(t);
            recalculate_waypoints(wp);
        }

        if (TheCampaign.Flags bitand CAMP_TACTICAL_EDIT)
        {
            Flight un;
            un = (Flight)vuDatabase->Find(gMapMgr->GetCurWPID());

            if (un)
            {
                fixup_unit(un);
                gGps->Update();
                gMapMgr->DrawMap();
            }
        }
    }

    //sfr: lets get the units owner of this waypoints
    Unit unit = static_cast<Unit>(vuDatabase->Find(gActiveFlightID));
    unit->MakeWaypointsDirty();

    /*
     C_Clock *clk;
     long t;
     WayPoint wp;

     if ((hittype not_eq C_TYPE_LMOUSEUP) and (hittype not_eq C_TYPE_LMOUSEDBLCLK) and (hittype not_eq C_TYPE_REPEAT))
     return;

     clk=(C_Clock*)control->Parent_->FindControl(control->GetUserNumber(0));
     if(clk)
     {
     ChangeTimeCB(ID,hittype,control);

     wp = get_current_waypoint ();
     if (wp)
     {
     wp->SetWPFlag (WPF_TIME_LOCKED);
     t = clk->GetTime() * VU_TICS_PER_SECOND;
     wp->SetWPTimes(t);
     recalculate_waypoints(wp);
     }

     if (TheCampaign.Flags bitand CAMP_TACTICAL_EDIT)
     {
     Flight un;
     un = (Flight)vuDatabase->Find(gMapMgr->GetCurWPID());
     if(un)
     {
     fixup_unit (un);
     gGps->Update();
     gMapMgr->DrawMap();
     }
     }
     }
     */
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void ChangeAirspeedCB(long ID, short hittype, C_Base *)
{
    WayPointClass *wp = NULL;
    float speed = 0.0F;
    int dir = 0;

    if ((hittype not_eq C_TYPE_LMOUSEUP) and (hittype not_eq C_TYPE_REPEAT) and (hittype not_eq C_TYPE_LMOUSEDBLCLK))
        return;

    if (ID == AIRSPEED_DECR)
        dir = -1;
    else if (ID == AIRSPEED_INC)
        dir = 1;

    wp = get_current_waypoint();

    if (wp)
    {
        if (wp->GetPrevWP())
        {
            speed = wp->GetWPSpeed() + 10 * dir;
            wp->SetWPFlag(WPF_SPEED_LOCKED);
            wp->SetWPSpeed(speed);
            recalculate_waypoints(wp);
        }

        if (TheCampaign.Flags bitand CAMP_TACTICAL_EDIT)
        {
            Flight un;

            un = (Flight)vuDatabase->Find(gMapMgr->GetCurWPID());

            if (un)
            {
                fixup_unit(un);
                gGps->Update();
                gMapMgr->DrawMap();
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void ChangeAltCB(long ID, short hittype, C_Base *)
{
    int value = 0, dir = 0;
    WayPointClass *wp = NULL;
    float speed = 0.0F;

    if ((hittype not_eq C_TYPE_LMOUSEUP) and (hittype not_eq C_TYPE_REPEAT) and (hittype not_eq C_TYPE_LMOUSEDBLCLK))
        return;

    if (ID == ALT_INC)
        dir = 1;
    else if (ID == ALT_DECR)
        dir = -1;

    wp = get_current_waypoint();

    // 2002-03-15 ADDED BY S.G. Seen wp as NULL (bugtrack #1014). If that happens, return right away
    if ( not wp)
        return;

    value = wp->GetWPAltitude();
    speed = get_air_speed(wp->GetWPSpeed() * KM_TO_NM, value);

    if (value < 1000)
        value += 100 * dir;
    else if (value < 40000)
        value += 500 * dir;
    else
        value += 1000 * dir;

    if (value < 100)
        value = 100;
    else if (value > 80000)
        value = 80000;

    if (wp->GetWPAction() == WP_TAKEOFF or wp->GetWPAction() == WP_LAND)
        value = 0;

    wp->SetWPAltitude(value);

    if (wp->GetWPFlags() bitand WPF_SPEED_LOCKED)
    {
        // Keep IAS constant
        speed = get_ground_speed(speed, value);
        wp->SetWPSpeed(speed * NM_TO_KM);
        recalculate_waypoints(wp);
    }
    else
        refresh_waypoint(wp);

    // KCK HACK: Not sure how to just refresh this waypoint - so, I'm going to only rebuild the list
    // once in a while for repeat, or on mouseup.
    if (hittype == C_TYPE_LMOUSEUP or not (rand() % 4))
    {
        // Rebuild the Z waypoint list (would be nice to just refresh)
        gMapMgr->SetCurrentWaypointList(gActiveFlightID);
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void ChangePatrolCB(long ID, short hittype, C_Base *control)
{
    C_Clock *clk;
    CampaignTime time;
    WayPoint wp;

    if (hittype not_eq C_TYPE_LMOUSEUP and hittype not_eq C_TYPE_REPEAT)
        return;

    clk = (C_Clock*)control->Parent_->FindControl(control->GetUserNumber(0));

    if (clk)
    {
        ChangeTimeCB(ID, hittype, control);

        wp = get_current_waypoint();

        if (wp and not (wp->GetWPFlags() bitand WPF_REPEAT))
        {
            if (wp->GetNextWP()) // 2001-11-17 M.N. this crashes in really rare conditions
                wp = wp->GetNextWP();
        }

        if ( not wp and not (wp->GetWPFlags() bitand WPF_REPEAT))
            return;

        time = clk->GetTime() * VU_TICS_PER_SECOND;
        wp->SetWPStationTime(time);
        recalculate_waypoints(wp);

        if (TheCampaign.Flags bitand CAMP_TACTICAL_EDIT)
        {
            Flight un = (Flight)vuDatabase->Find(gMapMgr->GetCurWPID());

            if (un)
            {
                fixup_unit(un);
                gGps->Update();
                gMapMgr->DrawMap();
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void ChangeAirspeedLockCB(long, short hittype, C_Base *control)
{
    WayPointClass
    *wp;

    wp = get_current_waypoint();

    // JB 010109 right clicks affect all waypoints
    if (hittype == C_TYPE_RMOUSEDOWN)
    {
        WayPointClass *w;
        int lock = wp->GetWPFlags() bitand WPF_SPEED_LOCKED;
        FlightClass *flt = (Flight) vuDatabase->Find(gActiveFlightID);
        w = flt->GetFirstUnitWP();

        while (w)
        {
            if (lock)
                w->UnSetWPFlag(WPF_SPEED_LOCKED);
            else
                w->SetWPFlag(WPF_SPEED_LOCKED);

            w = w->GetNextWP();
        }

        if (lock)
            control->SetState(0);
        else
            control->SetState(1);

        return;
    }

    // JB 010109

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    if (wp->GetWPFlags() bitand WPF_SPEED_LOCKED)
        wp->UnSetWPFlag(WPF_SPEED_LOCKED);
    else
        wp->SetWPFlag(WPF_SPEED_LOCKED);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void ChangeTOSLockCB(long, short hittype, C_Base *control)
{
    WayPointClass
    *wp;

    wp = get_current_waypoint();

    // JB 010109 right clicks affect all waypoints
    if (hittype == C_TYPE_RMOUSEDOWN)
    {
        WayPointClass *w;
        int lock = wp->GetWPFlags() bitand WPF_TIME_LOCKED;
        FlightClass *flt = (Flight) vuDatabase->Find(gActiveFlightID);
        w = flt->GetFirstUnitWP();

        while (w)
        {
            if (lock)
                w->UnSetWPFlag(WPF_TIME_LOCKED);
            else
                w->SetWPFlag(WPF_TIME_LOCKED);

            w = w->GetNextWP();
        }

        if (lock)
            control->SetState(0);
        else
            control->SetState(1);

        return;
    }

    // JB 010109

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    if (wp->GetWPFlags() bitand WPF_TIME_LOCKED)
        wp->UnSetWPFlag(WPF_TIME_LOCKED);
    else
        wp->SetWPFlag(WPF_TIME_LOCKED);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static void calculate_waypoint_times_before(WayPointClass *wp);
static void calculate_waypoint_times_after(WayPointClass *wp);
static void calculate_waypoint_times_between(WayPointClass *wp1, WayPointClass *wp2);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void refresh_waypoint(WayPointClass * wp)
{
    C_Window *win = NULL;
    C_Waypoint *cwp = NULL;
    FlightClass *flt = NULL;
    WayPointClass *w = NULL;
    int i = 0, campID = 0;

    flt = (Flight) vuDatabase->Find(gActiveFlightID);

    if ( not flt)
        return;

    w = flt->GetFirstUnitWP();
    i = 1;

    while (w and w not_eq wp)
    {
        w = w->GetNextWP();
        i++;
    }

    if ( not w)
        return;

    // Paint leg red, if we have errors
    campID = flt->GetCampID();
    cwp = gMapMgr->GetCurWP();

    if ( not cwp)
        return;

    if ( not IsValidWP(w, flt))
    {
        cwp->SetState(campID * 256 + i, 2);
        cwp->SetState(0x40000000 + (campID * 256) + i, 2);
    }
    else if (w == flt->GetCurrentUnitWP())
    {
        cwp->SetState(campID * 256 + i, 0);
        cwp->SetState(0x40000000 + (campID * 256) + i, 1);
    }
    else
    {
        cwp->SetState(campID * 256 + i, 0);
        cwp->SetState(0x40000000 + (campID * 256) + i, 0);
    }

    cwp->Refresh();

    gActiveWPNum = static_cast<short>(i);

    if (gMainHandler)
        win = gMainHandler->FindWindow(FLIGHT_PLAN_WIN);

    if (win)
    {
        UpdateWaypointWindowInfo(win, wp, gActiveWPNum);
        win->RefreshWindow();
    }
}

// We've changed waypoint wp's location, speed, or time and need to recalculate
void recalculate_waypoint_list(WayPointClass *wp, int minSpeed, int maxSpeed)
{
    // First, find out if we're time locked ourselves
    if (wp->GetWPFlags() bitand WPF_TIME_LOCKED)
    {
        // We're gonna have to do two recalculations, one before and one after this waypoint
        WayPointClass *pw, *nw;
        float dist;
        int time;
        pw = wp->GetPrevWP();
        nw = wp->GetNextWP();

        if (pw and not (pw->GetWPFlags() bitand WPF_TIME_LOCKED))
            recalculate_waypoint(pw, minSpeed, maxSpeed);
        else if (pw)
        {
            // Simply set Speed between pw and wp
            dist = wp->DistanceTo(pw);
            time = wp->GetWPArrivalTime() - pw->GetWPDepartureTime();

            if (time not_eq 0)
                wp->SetWPSpeed((dist * CampaignHours) / time);
            else
                wp->SetWPSpeed(0);
        }

        if (nw and not (nw->GetWPFlags() bitand WPF_TIME_LOCKED))
            recalculate_waypoint(nw, minSpeed, maxSpeed);
        else if (nw)
        {
            // Simply set Speed between nw and wp
            dist = wp->DistanceTo(nw);
            time = nw->GetWPArrivalTime() - wp->GetWPDepartureTime();

            if (time not_eq 0)
                nw->SetWPSpeed((dist * CampaignHours) / time);
            else
                nw->SetWPSpeed(0);
        }
    }
    else
        recalculate_waypoint(wp, minSpeed, maxSpeed);
}

// We've changed waypoint wp's location, speed, or time and need to recalculate
void recalculate_waypoints(WayPointClass *wp)
{
    FlightClass *flt;
    int minSpeed, maxSpeed;

    flt = (Flight) vuDatabase->Find(gActiveFlightID);

    //if ( not flt or not wp or not flt->IsFlight())
    if ( not flt or not wp) // JB 010326 Allow ground units
        return;

    //TJL 11/22/03 Remove /2 division
    //minSpeed = flt->GetCruiseSpeed()/2;
    minSpeed = flt->GetCruiseSpeed();
    maxSpeed = flt->GetMaxSpeed();

    // Do the recalculation
    recalculate_waypoint_list(wp, minSpeed, maxSpeed);

    // Check for errors
    ValidateWayPoints(flt);

    // Refresh
    refresh_waypoint(wp);
}

void recalculate_waypoint(WayPointClass *wp, int, int)
{
    GridIndex x, y, nx, ny;
    float speed, dist, d;
    WayPointClass *pw, *nw, *w;
    CampaignTime startTime = 0, endTime = 0, lockedTime = 0, now;

    if ( not wp)
        return;

    ShiAssert( not (wp->GetWPFlags() bitand WPF_TIME_LOCKED));

    // KCK: This is annoyingly complex.
    // Basically, we're trying to either move times in/out from the changed waypoint
    // or smooth speeds between two time locked waypoints,

    // First, find out if we're between a time locked pair, or if we've got open ends
    pw = nw = wp;
    w = wp->GetPrevWP();

    while (w)
    {
        pw = w;

        if (w->GetWPFlags() bitand WPF_TIME_LOCKED)
            break;

        w = w->GetPrevWP();
    }

    w = wp->GetNextWP();

    while (w)
    {
        nw = w;

        if (w->GetWPFlags() bitand WPF_TIME_LOCKED)
            break;

        w = w->GetNextWP();
    }

    if ((pw->GetWPFlags() bitand WPF_TIME_LOCKED) and (nw->GetWPFlags() bitand WPF_TIME_LOCKED))
    {
        // We're between timelocked stuff. smooth the speeds
        startTime = pw->GetWPDepartureTime();
        endTime = nw->GetWPArrivalTime();
        // Now calculate the total adjustable distance and time
        dist = 0.0F;
        pw->GetWPLocation(&x, &y);
        w = pw->GetNextWP();

        while (w)
        {
            w->GetWPLocation(&nx, &ny);
            d = Distance(x, y, nx, ny);

            if (w->GetWPFlags() bitand WPF_SPEED_LOCKED)
                lockedTime += FloatToInt32((d * CampaignHours) / w->GetWPSpeed()); // This time is locked up
            else
                dist += d;

            x = nx;
            y = ny;

            if (w == nw)
                break;

            w = w->GetNextWP();
        }

        // Now calculate our new average speed and apply it
        if (startTime > endTime)
            speed = (dist * CampaignHours) / (((startTime - endTime) * -1) - lockedTime);
        else
            speed = (dist * CampaignHours) / ((endTime - startTime) - lockedTime);

        pw->GetWPLocation(&x, &y);
        now = startTime;
        w = pw->GetNextWP();

        while (w)
        {
            w->GetWPLocation(&nx, &ny);
            d = Distance(x, y, nx, ny);

            if ( not (w->GetWPFlags() bitand WPF_SPEED_LOCKED))
            {
                w->SetWPSpeed(speed);

                if (speed)
                    now += FloatToInt32((d * CampaignHours) / speed);
            }
            else if (w->GetWPSpeed())
                now += FloatToInt32((d * CampaignHours) / w->GetWPSpeed());

            if (w == nw)
                break;

            w->SetWPTimes(now);
            now += w->GetWPStationTime();
            x = nx;
            y = ny;
            w = w->GetNextWP();
        }
    }
    else
    {
        // We've got one or more open ends, adjust times while keeping speeds
        if (nw and (nw->GetWPFlags() bitand WPF_TIME_LOCKED))
        {
            // Push backwards from nw (keep speeds constant)
            now = nw->GetWPArrivalTime();
            nw->GetWPLocation(&x, &y);
            w = nw->GetPrevWP();

            while (w)
            {
                w->GetWPLocation(&nx, &ny);
                d = Distance(x, y, nx, ny);
                speed = nw->GetWPSpeed();

                if (speed)
                    now -= FloatToInt32((d * CampaignHours) / speed);

                now -= w->GetWPStationTime();
                w->SetWPTimes(now);

                if (w == pw)
                    break;

                x = nx;
                y = ny;
                nw = w;
                w = w->GetPrevWP();
            }
        }
        else if (pw)
        {
            // Push forward from pw
            now = pw->GetWPDepartureTime();
            pw->GetWPLocation(&x, &y);
            w = pw->GetNextWP();

            while (w)
            {
                w->GetWPLocation(&nx, &ny);
                d = Distance(x, y, nx, ny);
                speed = w->GetWPSpeed();

                if (speed)
                    now += FloatToInt32((d * CampaignHours) / speed);

                w->SetWPTimes(now);
                now = w->GetWPDepartureTime();

                if (w == nw)
                    break;

                x = nx;
                y = ny;
                w = w->GetNextWP();
            }
        }
        else
            ShiAssert(0);
    }
}

int WayPointErrorCode(WayPointClass *wp, Flight flt)
{
    WayPointClass *pw;
    float dist, speed, minSpeed, maxSpeed;
    int errors = 0, action;
    unsigned int fuelAvail;
    long time;
    CampaignTime missionTime, takeoff, land;

    // Check for bad target/airbase/patrol times
    action = wp->GetWPAction();

    if (action == WP_TAKEOFF or action == WP_LAND or action == WP_STRIKE or action == WP_BOMB or
        action == WP_INTERCEPT or action == WP_RECON or action == WP_NAVSTRIKE)
    {
        // Requires a valid campaign entity
        CampEntity ent = wp->GetWPTarget();

        if ( not ent)
            errors or_eq WPERROR_NO_TARGET;
    }

    // Check for fuel
    pw = flt->GetFirstUnitWP();
    takeoff = land = pw->GetWPDepartureTime();

    while (pw and pw->GetWPAction() not_eq WP_LAND)
        pw = pw->GetNextWP();

    if (pw)
        land = pw->GetWPArrivalTime();

    missionTime = land - takeoff;
    fuelAvail = flt->CalculateFuelAvailable(255);

    if ((missionTime / CampaignMinutes) * flt->GetUnitClassData()->Rate > fuelAvail)
        errors or_eq WPERROR_FUEL;

    // Do minimum speed checks
    //TJL 11/23/03 Cruise/Max distinction removed. Speeds based on straight KM to NM
    //converion from movespeed in F4browse.  Make error aircraft specific.
    //minSpeed = flt->GetCruiseSpeed()/2.0F - 20.0F;
    //maxSpeed = flt->GetMaxSpeed() + 20.0F;
    minSpeed = flt->GetCruiseSpeed() * 0.7F;
    maxSpeed = flt->GetMaxSpeed() * 1.3F;

    if ((wp->GetWPFlags() bitand WPF_ALTERNATE) or wp->GetWPAction() == WP_REFUEL)
        return errors;

    if (wp->GetWPSpeed() < minSpeed and wp->GetPrevWP())
        errors or_eq WPERROR_SPEED;

    if (wp->GetWPSpeed() > maxSpeed)
        errors or_eq WPERROR_SPEED;

    // Check for valid speeds/times
    pw = wp->GetPrevWP();

    if (pw)
    {
        time = wp->GetWPArrivalTime() - pw->GetWPDepartureTime();

        if (time <= 1)
            errors or_eq WPERROR_TIME;

        dist = wp->DistanceTo(pw);

        if (time)
            speed = (dist * CampaignHours) / time;
        else
            speed = 0.0F;

        if (speed < minSpeed or speed > maxSpeed or fabs(speed - wp->GetWPSpeed()) > 10.0F)
            errors or_eq WPERROR_SPEED;
    }

    return errors;
}

int IsValidWP(WayPointClass *wp, Flight flt)
{
    int errors;

    errors = WayPointErrorCode(wp, flt);

    if (errors)
        return FALSE;

    return TRUE;
}

void ValidateWayPoints(Flight flt)
{
    C_Waypoint *cwp;
    WayPoint w;
    int i = 1;
    int campID;

    if ( not flt)
        return;

    campID = flt->GetCampID();
    cwp = gMapMgr->GetCurWP();
    w = flt->GetFirstUnitWP();

    while (w)
    {
        if ( not IsValidWP(w, flt))
        {
            cwp->SetState(campID * 256 + i, 2);
            cwp->SetState(0x40000000 + (campID * 256) + i, 2);
        }
        else if (w == flt->GetCurrentUnitWP())
        {
            cwp->SetState(campID * 256 + i, 0);
            cwp->SetState(0x40000000 + (campID * 256) + i, 1);
        }
        else
        {
            cwp->SetState(campID * 256 + i, 0);
            cwp->SetState(0x40000000 + (campID * 256) + i, 0);
        }

        i++;
        w = w->GetNextWP();
    }

    cwp->Refresh();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void set_waypoint_climb_mode(long, short hittype, C_Base *control)
{
    C_ListBox
    *lbox;

    WayPointClass
    *wp;

    C_Window
    *win;

    if (hittype not_eq C_TYPE_SELECT)
        return;

    win = control->Parent_;

    if ( not win)
        return;

    lbox = (C_ListBox*) control;

    wp = get_current_waypoint();

    if ((lbox) and (wp))
    {
        if (lbox->GetTextID() == CLIMB_DELAY)
            wp->SetWPFlag(WPF_HOLDCURRENT);
        else
            wp->UnSetWPFlag(WPF_HOLDCURRENT);
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void set_waypoint_enroute_action(long, short hittype, C_Base *control)
{
    C_ListBox
    *lbox;

    WayPointClass
    *wp;

    C_Window
    *win;

    if (hittype not_eq C_TYPE_SELECT)
        return;

    win = control->Parent_;

    if ( not win)
        return;

    lbox = (C_ListBox*) control;

    wp = get_current_waypoint();

    if ((lbox) and (wp))
        wp->SetWPRouteAction(lbox->GetTextID() - 1);

    refresh_waypoint(wp);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void set_waypoint_action(WayPoint wp, int action)
{
    int flags, oldaction, oldflags;
    WayPoint pw;
    CampBaseClass *ent;
    GridIndex x, y, ex = -1, ey = -1;

    if ( not wp)
        return;

    oldaction = wp->GetWPAction();
    oldflags = wp->GetWPFlags();
    flags = 0;

    switch (action)
    {
        case WP_TAKEOFF:
            flags = WPF_TAKEOFF;
            // Look for an airbase
            wp->GetWPLocation(&x, &y);
            ent = GetObjectiveByXY(x, y);

            if (ent and (ent->GetType() == TYPE_AIRBASE or ent->GetType() == TYPE_AIRSTRIP))
                wp->SetWPTarget(ent->Id());
            else
                wp->SetWPTarget(FalconNullId);

            break;

        case WP_LAND:
            flags = WPF_LAND;
            // Look for an airbase
            wp->GetWPLocation(&x, &y);
            ent = GetObjectiveByXY(x, y);

            if (ent and (ent->GetType() == TYPE_AIRBASE or ent->GetType() == TYPE_AIRSTRIP))
                wp->SetWPTarget(ent->Id());
            else
                wp->SetWPTarget(FalconNullId);

            break;

        case WP_ASSEMBLE:
        case WP_POSTASSEMBLE:
            flags = WPF_ASSEMBLE;
            wp->SetWPTarget(FalconNullId);
            break;

        case WP_TANKER:
        case WP_JAM:
        case WP_SAD:
        case WP_ELINT:
            flags = WPF_TARGET;
            pw = wp->GetPrevWP();

            if (pw and pw->GetWPAction() == action)
                flags or_eq WPF_REPEAT;

            wp->SetWPTarget(FalconNullId);
            break;

        case WP_GNDSTRIKE:
        case WP_CAP:
            flags = WPF_TARGET;
            pw = wp->GetPrevWP();

            if (pw and pw->GetWPAction() == action)
                flags or_eq WPF_REPEAT;
            else if (pw)
            {
                pw = wp->GetNextWP();

                if (pw and pw->GetWPAction() == action)
                    flags or_eq WPF_CP;
            }

            break;

        case WP_ESCORT:
        case WP_CA:
        case WP_RESCUE:
        case WP_ASW:
            flags = WPF_TARGET;
            break;

        case WP_AIRDROP:
            flags = WPF_TARGET bitor WPF_LAND bitor WPF_TAKEOFF;
            break;

        case WP_PICKUP:
            flags = WPF_LAND bitor WPF_TAKEOFF;
            break;

        case WP_INTERCEPT:
        case WP_NAVSTRIKE:
        case WP_SEAD:
            flags = WPF_TARGET;
            // Look for a unit target
            wp->GetWPLocation(&x, &y);
            ent = wp->GetWPTarget();

            if (ent and ent->IsUnit())
                ent->GetLocation(&ex, &ey);

            if (x not_eq ex or y not_eq ey)
            {
                ent = GetUnitByXY(x, y);

                if (ent)
                    wp->SetWPTarget(ent->Id());
                else
                    wp->SetWPTarget(FalconNullId);
            }

            break;

        case WP_STRIKE:
        case WP_BOMB:
        case WP_RECON:
            flags = WPF_TARGET;
            // Look for an objective target
            wp->GetWPLocation(&x, &y);
            ent = wp->GetWPTarget();

            if (ent and ent->IsObjective())
                ent->GetLocation(&ex, &ey);

            if (x not_eq ex or y not_eq ey)
            {
                ent = GetObjectiveByXY(x, y);

                if (ent)
                {
                    wp->SetWPTarget(ent->Id());
                }
                else
                {
                    wp->SetWPTarget(FalconNullId);
                }
            }

            break;

        default:
            break;
    }

    wp->UnSetWPFlag(WPF_CRITICAL_MASK);
    wp->SetWPFlag(flags);
    wp->SetWPAction(action);

    if (action not_eq oldaction and flags not_eq oldflags)
        gMapMgr->SetCurrentWaypointList(gActiveFlightID);
}

void set_waypoint_action(long, short hittype, C_Base *control)
{
    C_ListBox *lbox;
    WayPointClass *wp;
    C_Window *win;
    int action;

    if (hittype not_eq C_TYPE_SELECT)
        return;

    win = control->Parent_;

    if ( not win)
        return;

    lbox = (C_ListBox *) control;

    if ( not lbox)
        return;

    action = lbox->GetTextID() - 1;
    RefreshActionClusters(action, control->Parent_);
    control->Refresh();

    wp = get_current_waypoint();
    set_waypoint_action(wp, action);

    SetSteerPointValues(control->Parent_, wp, gActiveWPNum);
    refresh_waypoint(wp);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void set_waypoint_formation(long, short hittype, C_Base *control)
{
    C_ListBox
    *lbox;

    WayPointClass
    *wp;

    C_Window
    *win;

    if (hittype not_eq C_TYPE_SELECT)
        return;

    win = control->Parent_;

    if ( not win)
        return;

    lbox = (C_ListBox *) control;

    wp = get_current_waypoint();

    if ((lbox) and (wp))
        wp->SetWPFormation(lbox->GetTextID() - 1);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// Takes speed in nm/hr and altitude in ft
float get_ground_speed(float speed, int altitude)
{
    float vcas, vtas, error;
    vtas = speed;
    error = speed / 2.0F;

    while (error > 0.1F)
    {
        vcas = get_air_speed(vtas, altitude);

        if (vcas < speed)
        {
            vtas += error;
        }
        else
        {
            vtas -= error;
        }

        error /= 2.0F;
    }

    return vtas;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// Takes speed in nm/hr and altitude in ft
float get_air_speed(float speed, int altitude)
{
    float rsigma, pa, mach, qc, qpasl1, vcas, oper, ttheta;

    if (altitude <= 36089)
    {
        ttheta = 1.0F - 0.000006875F * altitude;
        rsigma = static_cast<float>(pow(ttheta, 4.256F));
    }
    else
    {
        ttheta = 0.7519F;
        rsigma = static_cast<float>(0.2971F * pow(2.718F, 0.00004806F * (36089.0F - altitude)));
    }

    mach = static_cast<float>(speed / (sqrt(ttheta) * AASLK));
    pa  = ttheta * rsigma * PASL;

    if (mach <= 1.0F)
    {
        qc = ((float)pow((1.0F + 0.2F * mach * mach), 3.5F) - 1.0F) * pa;
    }
    else
    {
        qc = static_cast<float>(((166.9 * mach * mach) / (float)(pow((7.0F - 1.0F / (mach * mach)), 2.5F)) - 1.0F) * pa);
    }

    qpasl1 = qc / PASL + 1.0F;
    vcas = static_cast<float>(1479.12F * sqrt(pow(qpasl1, 0.285714F) - 1.0F));

    if (qc > 1889.64F)
    {
        oper = static_cast<float>(qpasl1 * pow((7.0F - AASLK * AASLK / (vcas * vcas)), 2.5F));

        // sfr: holy shit, is this correct?
        if (oper < 0.0F) oper = 0.1F;

        {
            vcas = static_cast<float>(51.1987F * sqrt(oper));
        }
    }

    return vcas;
}

void RebuildCurrentWPList()
{
    if (gMapMgr)
        gMapMgr->SetCurrentWaypointList(gMapMgr->GetCurWPID());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// Find a new target/airbase etc if this waypoint requires it
void DropWayPoint(WayPoint wp)
{
    GridIndex x, y;
    CampBaseClass *ent;
    int action;
    Unit unit;

    if ( not wp)
        return;

    unit = (Unit)vuDatabase->Find(gActiveFlightID);

    action = wp->GetWPAction();
    wp->GetWPLocation(&x, &y);

    if (action == WP_TAKEOFF or action == WP_LAND)
    {
        ent = GetObjectiveByXY(x, y);

        if (ent and (ent->GetType() == TYPE_AIRBASE or ent->GetType() == TYPE_AIRSTRIP))
        {
            wp->SetWPTarget(ent->Id());
        }
        else
        {
            wp->SetWPTarget(FalconNullId);
        }
    }
    else if (action == WP_STRIKE or action == WP_BOMB or action == WP_RECON)
    {
        ent = GetObjectiveByXY(x, y);

        if (ent)
            wp->SetWPTarget(ent->Id());
        else
            wp->SetWPTarget(FalconNullId);
    }
    else if (action == WP_INTERCEPT or action == WP_NAVSTRIKE or action == WP_GNDSTRIKE)
    {
        ent = GetUnitByXY(x, y);

        if (ent)
            wp->SetWPTarget(ent->Id());
        else
            wp->SetWPTarget(FalconNullId);
    }

    if (unit->IsFlight())
    {
        // 2001-04-23 ADDED BY S.G. ONLY SET THE UNIT MISSION TARGET IF WE HAVE A TARGET (SO WE DON'T CLEAR PREVIOUS TARGETS)
        if (wp->GetWPTargetID())
            // END OF ADDED SECTION
            ((Flight)unit)->SetUnitMissionTarget(wp->GetWPTargetID());
    }
}

/*
   void GotoWaypoint ()
   {
   C_Window *win;
   WayPoint wp,prevwp=NULL;
   int i;
   Unit un;
   Flight flt;
   VU_ID *tmpID;

   if(hittype not_eq C_TYPE_LMOUSEUP)
   return;

   win=gMainHandler->FindWindow(FLIGHT_PLAN_WIN);
   if(win)
   {
   tmpID=(VU_ID *)control->GetUserPtr(C_STATE_0);

   if ( not tmpID) return;

   un = FindUnit (*tmpID);

   if ((un) and (un->IsFlight ()))
   {
   flt=(Flight)un;

   if (gActiveFlightID not_eq *tmpID)
   gActiveFlightID=*tmpID;

   wp=flt->GetFirstUnitWP();
   prevwp=wp;
   i=1;
   while(i < control->GetUserNumber(C_STATE_1) and wp)
   {
   prevwp=wp;
   wp=wp->GetNextWP();
   i++;
   }
   if(wp)
   {
   UpdateWaypointWindowInfo(win,wp,i);
   win->RefreshWindow();
   gMainHandler->ShowWindow(win);
   gMainHandler->WindowToFront(win);
   gActiveWPNum=control->GetUserNumber(C_STATE_1);
   }
   }
   }
   }
 */

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void WaypointCB(long ID, short hittype, class C_Base *ctrl)
{
    C_Window *win = NULL;
    Unit un = NULL;
    float wx = 0.0F, wy = 0.0F, wz = 0.0F;
    GridIndex gx = 0, gy = 0;
    C_Waypoint *cwp = NULL;
    WAYPOINTLIST *wpicon = NULL;
    WayPointClass *wp = NULL; //,*lwp=NULL;

    cwp = (C_Waypoint *) ctrl;

    if (cwp)
        wpicon = cwp->GetLast();

    if ( not cwp or not wpicon)
        return;

    // The reason I changed this routine... (And it didn't screw up the waypoints)... is because you guys took out
    // the altitude part of this routine (probably Robin)...
    // This routine is used for both XY draging AND Z dragging, and you guys just assumed it was used for
    // XY only, which screwed everything else up.
    // Therefore, the Check for DragXY bitand DragY (where DragY is for altitude)
    // beyond that, there has been NO logic changes to this routine
    if (hittype == C_TYPE_LDROP)
    {
        MonoPrint("TYPE_LDROP in WaypointCB\n");

        // Drop and recalculate a waypoint if we're dropping a nub, otherwise
        // Just relocate and recalculate the waypoint
        un = (Unit)vuDatabase->Find(gMapMgr->GetCurWPID());

        if (un)
        {
            gActiveFlightID = un->Id();

            if (wpicon->Icon->GetUserNumber(C_STATE_1) < 0)
            {
                // KCK: Peter, this is pointless, as the campaign will just replan it,
                // And will not do what you think it will.

                // This MAY be pointless, however, if the user drags this... it might as well let them do it
                // besides, it is used for the SIM CAS waypoint as well as the Alternate Landing strip
                // If the Alternate landing (or tanker) can't be changed (ever) the dragging should be disabled,
                // NOT the call saying to move it
                // Divert WP in Flight
                if (un->IsFlight())
                {
                    wp = ((Flight)un)->GetOverrideWP();

                    if (wp)
                    {
                        if (cwp->GetType() == C_TYPE_DRAGXY)
                        {
                            float x, y, z;
                            wx = wpicon->worldx;
                            wy = wpicon->worldy;

                            x = (float) cwp->GetUserNumber(C_STATE_0) - wy;
                            y = wx;
                            z = wp->GetWPAltitude() * -1.0F;

                            wp->SetLocation(x, y, z);
                        }
                        else if (cwp->GetType() == C_TYPE_DRAGY)
                        {
                            wp->SetWPAltitude(-1 * FloatToInt32(wpicon->worldy));
                        }
                    }
                }
            }
            else
            {
                if (cwp->GetType() == C_TYPE_DRAGXY)
                {
                    // Standard WP or WP nub
                    wx = wpicon->worldx;
                    wy = gMapMgr->GetMaxY() - wpicon->worldy;
                    gx = SimToGrid(wx);
                    gy = SimToGrid(wy);

                    if (un->IsBattalion())
                    {
                        CampEnterCriticalSection();
                        tactical_set_orders((Battalion)un, FalconNullId, gx, gy);
                        CampLeaveCriticalSection();
                        return;
                    }
                    else if (un->IsFlight())
                    {
                        gActiveWPNum = static_cast<short>(ID bitand 0xff);
                        wp = get_current_waypoint();

                        if (wp)
                        {
                            if (ID bitand 0x40000000)
                            {
                                int alt = wp->GetWPAltitude();
                                WayPoint pw = wp->GetPrevWP();

                                if (pw)
                                {
                                    if (pw->GetWPFlags() bitand WPF_HOLDCURRENT)
                                        alt = pw->GetWPAltitude();

                                    wp = un->AddWPAfter(pw, gx, gy, alt, FloatToInt32(wp->GetWPSpeed()), 0, wp->GetWPRouteAction(), WP_NOTHING);
                                }
                            }

                            wp->SetWPLocation(gx, gy);
                            recalculate_waypoints(wp);

                            if ( not (ID bitand 0x60000000))
                            {
                                DropWayPoint(wp);

                                // Look for the Triangle OR square... and move to this spot also
                                if (cwp->UpdateInfo(0x20000000 + ID, wpicon->worldx, wpicon->worldy))
                                    cwp->Refresh();
                            }
                        }
                    }
                }
                else if (cwp->GetType() == C_TYPE_DRAGY)
                {
                    // Standard WP or WP nub
                    wz = wpicon->worldy; // scale it

                    gActiveWPNum = static_cast<short>(ID bitand 0xff);
                    wp = get_current_waypoint();

                    if (wp)
                    {
                        wp->SetWPAltitude(-1 * FloatToInt32(wpicon->worldy));
                        recalculate_waypoints(wp);
                        DropWayPoint(wp);
                    }
                }

                if (ID bitand 0x40000000)
                    PostMessage(gMainHandler->GetAppWnd(), FM_REBUILD_WP_LIST, 0, 0); // Have to do this because we can't delete the caller of this CB
            }

            if (TheCampaign.Flags bitand CAMP_TACTICAL_EDIT)
            {
                UI_Refresher *cur;
                fixup_unit(un);
                cur = (UI_Refresher*)gGps->Find(un->GetCampID());

                if (cur)
                    cur->Update(un, 0);

                gMapMgr->GetCurWP()->Refresh();
            }

            un->MakeWaypointsDirty();
        }
    }
    else if (hittype == C_TYPE_LMOUSEUP)
    {
        // Just activate this unit and waypoint
        un = (Unit)vuDatabase->Find(gMapMgr->GetCurWPID());

        if ( not un)
            return;

        MonoPrint("TYPE_LMOUSEUP in WaypointCB\n");

        if (un->IsFlight())
        {
            gActiveFlightID = un->Id();

            if ( not (ID bitand 0x40000000))
            {
                gActiveWPNum = static_cast<short>(ID bitand 0xff);
                // gActiveWPNum=control->GetUserNumber(C_STATE_1);
                wp = get_current_waypoint();
            }

            win = gMainHandler->FindWindow(FLIGHT_PLAN_WIN);

            if (win and wp)
            {
                UpdateWaypointWindowInfo(win, wp, gActiveWPNum);
                win->RefreshWindow();
                gMainHandler->ShowWindow(win);
                gMainHandler->WindowToFront(win);
            }
        }
    }
    else if (hittype == C_TYPE_LMOUSEDOWN)
    {
        // Just activate this unit and record the waypoint's current speed
        if ( not (ID bitand 0x40000000))
        {
            un = (Unit)vuDatabase->Find(gMapMgr->GetCurWPID());

            if ( not un)
                return;

            gActiveFlightID = un->Id();
            gActiveWPNum = static_cast<short>(ID bitand 0xff);
            wp = get_current_waypoint();

            if (wp)
                SetWPSpeed(wp);
        }
    }
    else if (hittype == C_TYPE_MOUSEMOVE)
    {
        // Check for Waypoints vs distance boxes
        if ( not (ID bitand 0x40000000))
        {
            // Just update the waypoint's location and check for validity
            un = (Unit)vuDatabase->Find(gMapMgr->GetCurWPID());

            if ( not un or not un->IsFlight())
                return;

            gActiveFlightID = un->Id();
            gActiveWPNum = static_cast<short>(ID bitand 0xff);
            wp = get_current_waypoint();

            if (wp)
            {
                if (cwp->GetType() == C_TYPE_DRAGXY)
                {
                    wx = wpicon->worldx;
                    wy = gMapMgr->GetMaxY() - wpicon->worldy;
                    gx = SimToGrid(wx);
                    gy = SimToGrid(wy);
                    wp->SetWPLocation(gx, gy);
                }
                else if (cwp->GetType() == C_TYPE_DRAGY)
                {
                    wp->SetWPAltitude(-1 * FloatToInt32(wpicon->worldy));
                }

                recalculate_waypoints(wp);
            }

            if (TheCampaign.Flags bitand CAMP_TACTICAL_EDIT)
            {
                UI_Refresher *cur;
                fixup_unit(un);
                cur = (UI_Refresher*)gGps->Find(un->GetCampID());

                if (cur)
                    cur->Update(un, 0);

                gMapMgr->GetCurWP()->Refresh();
                gMapMgr->GetCurWPZ()->Refresh();
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void update_active_flight(UnitClass *un)
{
    if (gMapMgr and vuDatabase->Find(gActiveFlightID) == un)
    {
        gMapMgr->SetCurrentWaypointList(un->Id());
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
