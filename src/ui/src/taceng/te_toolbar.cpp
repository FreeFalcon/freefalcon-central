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

#include "CmpGlobl.h"
#include "ListADT.h"
#include "vutypes.h"
#include "Objectiv.h"
//#include "Relation.h" JAM 19Sep03 - Does not exist?
#include "Find.h"
#include "F4Vu.h"
#include "strategy.h"
#include "Path.h"
#include "ASearch.h"
#include "Campaign.h"
#include "Update.h"
#include "CampList.h"
#include "squadron.h"
#include "classtbl.h"
#include "vu2.h"
#include "chandler.h"
#include "ui95_ext.h"
#include "uicomms.h"
#include "userids.h"
#include "textids.h"
#include "tac_class.h"
#include "te_defs.h"
#include "division.h"
#include "misseval.h"
#include "cmap.h"
#include "ui_cmpgn.h"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static void tactical_team_victory_button(long, short, C_Base *);
static void tactical_ato_button(long, short, C_Base *);
static void tactical_oob_button(long, short, C_Base *);
static void tactical_flight_plan_button(long, short, C_Base *);
static void tactical_munitions_button(long, short, C_Base *);
static void tactical_briefing_button(long, short, C_Base *);
static void tactical_briefing_print(long, short hittype, C_Base *ctrl);
static void tactical_debriefing_print(long, short hittype, C_Base *ctrl);

void CampaignButtonCB(long ID, short hittype, C_Base *control);

void tactical_show_ato_window(void);
void tactical_show_oob_window(void);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

short GetFlightStatusID(Flight element);
int CompressCampaignUntilTakeoff(Flight flight);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

//extern int tactical_enable_motion;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void hookup_toolbar_buttons(C_Window *winme)
{
    C_Button
    *ctrl;

    // Hook up Fly Button
    ctrl = (C_Button *)winme->FindControl(SINGLE_FLY_CTRL);

    if (ctrl)
        ctrl->SetCallback(CampaignButtonCB);

    ctrl = (C_Button *)winme->FindControl(COMMS_FLY_CTRL);

    if (ctrl)
        ctrl->SetCallback(CampaignButtonCB);

    // VC Button
    ctrl = (C_Button *) winme->FindControl(VC_BUTTON);

    if (ctrl not_eq NULL)
    {
        ctrl->SetCallback(tactical_team_victory_button);
    }

    // ATO Button
    ctrl = (C_Button *) winme->FindControl(ATO_BUTTON);

    if (ctrl)
    {
        ctrl->SetCallback(tactical_ato_button);
    }

    // OOB Button
    ctrl = (C_Button *) winme->FindControl(OOB_BUTTON);

    if (ctrl)
    {
        ctrl->SetCallback(tactical_oob_button);
    }

    // Flight Plan Button
    ctrl = (C_Button *) winme->FindControl(FLIGHT_PLAN_BUTTON);

    if (ctrl)
    {
        ctrl->SetCallback(tactical_flight_plan_button);
    }

    // Munitions Button
    ctrl = (C_Button *) winme->FindControl(MUNITIONS_BUTTON);

    if (ctrl)
    {
        ctrl->SetCallback(tactical_munitions_button);
    }

    // Briefing
    ctrl = (C_Button *) winme->FindControl(BRIEF_BUTTON);

    if (ctrl not_eq NULL)
    {
        ctrl->SetCallback(tactical_briefing_button);
    }

    // TacRef
    ctrl = (C_Button *) winme->FindControl(TACREF_CTRL);

    if (ctrl not_eq NULL)
    {
        ctrl->SetCallback(OpenTacticalReferenceCB);
    }

}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/*
static void tactical_start_engagement (long ID, short hittype, C_Base *ctrl)
{
 Flight fl;
 int pilotSlot;

 if(hittype not_eq C_TYPE_LMOUSEUP)
 return;

 fl = FalconLocalSession->GetPlayerFlight();
 pilotSlot = FalconLocalSession->GetPilotSlot();

 if (tactical_mission_loaded and fl and pilotSlot)
 {
 OTWDriver.todOffset = 0.0F;

 // tactical_enable_motion = 1; // not current_tactical_mission->is_flag_on (tf_start_paused);

 // Trigger the campaign to compress time and takeoff.
 if ( not CompressCampaignUntilTakeoff(fl))
 return;
 }
 else
 {
 // PETER TODO: Clear mission window's selection TOO
 FalconLocalSession->SetPlayerFlight(NULL);
 FalconLocalSession->SetPilotSlot(255);
 return;
 }
}
*/

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static void tactical_team_victory_button(long, short hittype, C_Base *)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
    {
        return;
    }

    update_team_victory_window();

    gMainHandler->EnableWindowGroup(3400);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static void tactical_ato_button(long, short hittype, C_Base *)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
    {
        return;
    }

    //MonoPrint ("ATO Window\n");

    tactical_show_ato_window();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static void tactical_oob_button(long, short hittype, C_Base *)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
    {
        return;
    }

    //MonoPrint ("OOB Window\n");

    tactical_show_oob_window();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern VU_ID
gSelectedFlightID,
gActiveFlightID,
gCurrentFlightID;

void UpdateWaypointWindowInfo(C_Window *win, WayPoint wp, int wpnum, int flag = TRUE);

static void tactical_flight_plan_button(long, short hittype, C_Base *ctrl)
{
    C_Window
    *win;

    WayPoint
    wp;

    Flight
    flt;

    if (hittype not_eq C_TYPE_LMOUSEUP)
    {
        return;
    }

    flt = (Flight)vuDatabase->Find(gCurrentFlightID);

    if ( not flt)
    {
        return;
    }

    win = gMainHandler->FindWindow(FLIGHT_PLAN_WIN);

    if (win)
    {
        if ( not (gMainHandler->GetWindowFlags(FLIGHT_PLAN_WIN) bitand C_BIT_ENABLED))
        {
            gActiveFlightID = gSelectedFlightID;

            wp = flt->GetFirstUnitWP();

            if (wp)
            {
                UpdateWaypointWindowInfo(win, wp, 1);
                gMainHandler->EnableWindowGroup(ctrl->GetGroup());
            }
        }
        else
        {
            gMainHandler->WindowToFront(win);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void OpenMunitionsWindowCB(long ID, short hittype, C_Base *control);

static void tactical_munitions_button(long ID, short hittype, C_Base *control)
{
    OpenMunitionsWindowCB(ID, hittype, control);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void do_tactical_briefing(C_Base *);

static void tactical_briefing_button(long, short hittype, C_Base *ctrl)
{
    Flight flight;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    flight = (Flight) vuDatabase->Find(gSelectedFlightID);

    if ( not flight or not flight->IsFlight())
        return;

    // KCK: This should only need to be called upon selecting a flight -
    // but in edit mode there seems to be close to a zillion ways to select
    // a flight, so I'm just going to redo it every time we look at the briefing -
    // and before flying.
    TheCampaign.MissionEvaluator->PreMissionEval(flight, FalconLocalSession->GetPilotSlot());

    do_tactical_briefing(ctrl);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/*
_TCHAR *UI_WordWrap(C_Window *win,_TCHAR *str,long fontid,short width,BOOL *status);

void add_briefing_text (C_Window *win, int &x, int &y, char *str)
{
 int
 color,
 startcol,
 endcol,
 wrap_w;

 C_Text
 *txt;

 _TCHAR
 *wrap;

 BOOL
 status,
 retval;

 x = 0;
 y = 0;
 startcol = 0;
 endcol = win->ClientArea_[0].right - win->ClientArea_[0].left - 10;
 color = 0xffffff;

 if (str)
 {
 retval=TRUE;

 if ((win == NULL) or (str == NULL))
 {
 return;
 }

 wrap_w = endcol - x;

 if (str)
 {
 wrap = UI_WordWrap (win, str, win->Font_, wrap_w, &status);

 if ( not status)
 {
 retval = status;
 }

 while (wrap)
 {
 wrap_w = endcol - startcol;

 txt = new C_Text;
 txt->Setup (C_DONT_CARE, C_TYPE_NORMAL);
 txt->SetFixedWidth (_tcsclen (wrap)+1);
 txt->SetText (wrap);
 txt->SetXY (x, y);
 txt->SetFGColor (color);
 txt->SetFont (win->Font_);
 txt->SetUserNumber (_UI95_DELGROUP_SLOT_, _UI95_DELGROUP_ID_);
 txt->SetFlagBitOn (C_BIT_LEFT);
 txt->SetClient (0);
 win->AddControl (txt);
 wrap = UI_WordWrap (win, NULL, win->Font_, wrap_w, &status);

 if (wrap)
 {
 x = startcol;
 y += gFontList->GetHeight (win->Font_);
 }

 if ( not status)
 {
 retval = status;
 }
 }

 x = txt->GetX() + txt->GetW();
 }
 }
}
*/

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void BuildCampBrief(C_Window *);
void BuildCampBrief(_TCHAR *txt);
int SendStringToPrinter(_TCHAR *str, _TCHAR *title);

void do_tactical_briefing(C_Base *control)
{
    C_Window
    *win;

    win = gMainHandler->FindWindow(BRIEF_WIN);

    if (win)
    {
        BuildCampBrief(win);
        gMainHandler->EnableWindowGroup(control->GetGroup());
        // JPO - attempt to add handlers for these
        C_Button *ctrl = (C_Button*)win->FindControl(BRIEF_PRINT);

        if (ctrl)
            ctrl->SetCallback(tactical_briefing_print);
    }
}

static void tactical_briefing_print(long, short hittype, C_Base *ctrl)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    _TCHAR string[8192];
    BuildCampBrief(string);
    SendStringToPrinter(string, "Briefing");
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void BuildCampDebrief(C_Window *win);
void BuildCampDebrief(_TCHAR *txt);
#ifdef DEBUG
#define FUNKY_KEVIN_DEBUG_STUFF 1
#endif

#ifdef FUNKY_KEVIN_DEBUG_STUFF
extern int inMission;
#endif

void do_tactical_debrief(void)
{
    C_Window
    *win;

    if (current_tactical_mission->get_type() not_eq tt_training)
    {
        win = gMainHandler->FindWindow(DEBRIEF_WIN);

        // KCK: Added the check for a pilot list so that we don't debrief after a
        // discarded mission
        if (win and TheCampaign.MissionEvaluator and TheCampaign.MissionEvaluator->flight_data and TheCampaign.MissionEvaluator->flight_data->mission not_eq AMIS_TRAINING)
        {
            BuildCampDebrief(win);
            gMainHandler->EnableWindowGroup(win->GetGroup());
            // JPO - attempt to add handlers for these
            C_Button *ctrl = (C_Button*)win->FindControl(BRIEF_PRINT);

            if (ctrl)
                ctrl->SetCallback(tactical_debriefing_print);
        }

#ifdef FUNKY_KEVIN_DEBUG_STUFF
        else inMission = 0; // JPO allow training missions to finish in debug mode

#endif
    }
}

static void tactical_debriefing_print(long, short hittype, C_Base *ctrl)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    if (TheCampaign.MissionEvaluator == NULL or
        TheCampaign.MissionEvaluator->flight_data == NULL or
        TheCampaign.MissionEvaluator->flight_data->mission == AMIS_TRAINING)
        return;

    _TCHAR string[8192];
    BuildCampDebrief(string);
    SendStringToPrinter(string, "DeBriefing");
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
