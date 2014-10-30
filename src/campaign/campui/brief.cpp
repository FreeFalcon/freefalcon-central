//
// Brief reader functions
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <tchar.h>
#include "Debuggr.h"
#include "CampStr.h"
#include "CampLib.h"
#include "Find.h"
#include "Flight.h"
#include "Brief.h"
#include "Name.h"
#include "Package.h"
#include "feature.h"
#include "Team.h"
#include "MissEval.h"
#include "CmpClass.h"
#include "Campaign.h"
#include "WinGraph.h"
#include "classtbl.h"
#include "ui95/CHandler.h"
#include "../ui/include/tac_class.h"
#include "../ui/include/te_defs.h"
#include "Weather.h"
#include "atm.h"
#include "FalcSess.h"
#include "userids.h"
#include "ui95/CWindow.h"
#include "F4Version.h"

#ifdef DEBUG
#define FUNKY_KEVIN_DEBUG_STUFF 1
#endif

#ifdef FUNKY_KEVIN_DEBUG_STUFF
extern int inMission;
#endif

extern C_Handler *gMainHandler;

enum
{
    BID_DROPDOWN = 50300,
    BID_SCROLLCAP_TOP_ON = 50013,
    BID_SCROLLCAP_TOP_OFF = 50014,
    BID_SCROLLCAP_BOTTOM_ON = 50015,
    BID_SCROLLCAP_BOTTOM_OFF = 50016,
    BID_SCROLL = 50017,
};

// ============================
// Modular wide globals
// ============================

static long BDefaultFont = ARIAL_12; // Font used for string size determination
static short CBX; // Current x in pixels
static short CBY; // Current y in pixels
static int CLineStart; // Current wrap-to point, in pixels
static COLORREF CBColor; // Current color
static WayPoint CWayPoint; // Current waypoint
static WayPoint LWayPoint; // Last waypoint
static CampEntity CEntity; // Current entity
static Unit ESquad; // Enemy active squadron
static _TCHAR* CTextPtr;
static _TCHAR* CCurrentLine;
static PilotDataClass *CPilotData; // Static pointer to relevant pilot data

#ifdef DEBUG
int testDebrief = 0;
#endif

// ============================
// Prototypes
// ============================

// Main Brief builders
int BuildBriefString(C_Window *win, _TCHAR *brief);
int BuildDebriefString(C_Window *win, _TCHAR *brief);
int ReadScriptedBriefFile(char* filename, _TCHAR *current_line, C_Window *win, _TCHAR *brief, MissionEvaluationClass* mec, FlightDataClass *flight_data);

// These add controls to a window or a string. Either window or output should be non null, but not both.
void AddHorizontalLineToBrief(C_Window *window);
void AddStringToBrief(_TCHAR *buffer, C_Window *window, _TCHAR *output);
void AddTabToBrief(int tab, _TCHAR *buffer, C_Window *window, _TCHAR *output);
void AddTabToDebrief(int tab, _TCHAR *buffer, C_Window *window, _TCHAR *output);

void AddEOLToBrief(_TCHAR *buffer, C_Window *window, _TCHAR *output);
// This will return the current X bitand Y location in a window or string
void GetCurrentBriefXY(int *x, int *y, _TCHAR *buffer, C_Window *window, _TCHAR *output);

extern BOOL AddWordWrapTextToWindow(C_Window *win, short *x, short *y, short startcol, short endcol, COLORREF color, _TCHAR *str, long Client = 0);
extern void AddHorizontalLineToWindow(C_Window *win, short *x, short *y, short startcol, short endcol, COLORREF color, long Client = 0);

extern float get_air_speed(float, int);
extern int determine_tactical_rating(void);

extern int ConvertTeamToStringIndex(int team, int gender = 0, int usage = 0, int plural = 0);

static void GetWpActionToBuffer(WayPoint wp, _TCHAR *cline);
static void GetWpTimeToBuffer(WayPoint wp, _TCHAR *cline);
static void GetWptDist(WayPoint wp, WayPoint lwp, _TCHAR *cline);
static void GetWptSpeed(WayPoint wp, WayPoint lwp, _TCHAR *cline);
static int GetWpAlt(WayPoint wp, WayPoint lwp, _TCHAR *cline);
static void GetWpHeading(WayPoint wp, WayPoint lwp, _TCHAR *cline);
static void GetWpDescription(WayPoint wp, _TCHAR *cline);

extern bool g_bBriefHTML; //THW 2003-12-07 Don't ignore <tags> when parsing the .b layout files

// ============================
// Main brief builder functions
// ============================

void BuildCampBrief(C_Window *win)
{
    CBX = CBY = CLineStart = 0;
    F4CSECTIONHANDLE* Leave;

#ifdef DEBUG

    if (testDebrief)
    {
        C_Window *win2 = gMainHandler->FindWindow(DEBRIEF_WIN);
        BuildCampDebrief(win2);
        return;
    }

#endif

    Leave = UI_Enter(win);
    DeleteGroupList(BRIEF_WIN);
    win->ScanClientAreas();
    // KCK HACK: I can't find how to set colors the correct way..
    win->ReverseText = RGB(0, 255, 0);
    win->DisabledText = RGB(230, 230, 230);
    CBColor = win->NormalText = RGB(230, 230, 230);
    BuildBriefString(win, NULL);
    win->ScanClientAreas();
    win->RefreshWindow();
    UI_Leave(Leave);
}

void BuildCampBrief(_TCHAR *brief_string)
{
    CBX = CBY = CLineStart = 0;
    brief_string[0] = 0;
    BuildBriefString(NULL, brief_string);
}

void BuildCampDebrief(C_Window *win)
{
    CBX = CBY = CLineStart = 0;
    F4CSECTIONHANDLE* Leave;

    Leave = UI_Enter(win);
    DeleteGroupList(DEBRIEF_WIN);
    win->ScanClientAreas();
    win->ReverseText = RGB(0, 255, 0);
    win->DisabledText = RGB(230, 230, 230);
    CBColor = win->NormalText = RGB(230, 230, 230);
    BuildDebriefString(win, NULL);
    win->ScanClientAreas();
    win->RefreshWindow();
    UI_Leave(Leave);

#ifdef FUNKY_KEVIN_DEBUG_STUFF
    inMission = 0;
#endif
}

void BuildCampDebrief(_TCHAR *brief_string)
{
    CBX = CBY = CLineStart = 0;
    brief_string[0] = 0;
    BuildDebriefString(NULL, brief_string);

#ifdef FUNKY_KEVIN_DEBUG_STUFF
    inMission = 0;
#endif
}

int BuildBriefString(C_Window *win, _TCHAR *brief)
{
    int j, weaps, pilots;
    FlightDataClass *flight_data, *flight_ptr;
    _TCHAR current_line[MAX_STRLEN_PER_PARAGRAPH] = {0}; // Text in current line

    CampEnterCriticalSection();
    ESquad = NULL;
    flight_data = TheCampaign.MissionEvaluator->player_element;
    ReadScriptedBriefFile("Header.b", current_line, win, brief, TheCampaign.MissionEvaluator, flight_data);
    AddHorizontalLineToBrief(win);
    ReadScriptedBriefFile("Situate.b", current_line, win, brief, TheCampaign.MissionEvaluator, flight_data);
    // Check for mission diverts here
    flight_ptr = TheCampaign.MissionEvaluator->flight_data;

    while (flight_ptr)
    {
        ReadScriptedBriefFile("divert.b", current_line, win, brief, TheCampaign.MissionEvaluator, flight_ptr);
        flight_ptr = flight_ptr->next_flight;
    }

    if (g_bBriefHTML and ( not win))
        AddStringToBuffer("</table>", current_line);

    AddHorizontalLineToBrief(win);
    ReadScriptedBriefFile("PackHead.b", current_line, win, brief, TheCampaign.MissionEvaluator, flight_data);
    flight_ptr = TheCampaign.MissionEvaluator->flight_data;

    while (flight_ptr)
    {
        ReadScriptedBriefFile("Element.b", current_line, win, brief, TheCampaign.MissionEvaluator, flight_ptr);
        flight_ptr = flight_ptr->next_flight;
    }

    if (g_bBriefHTML and ( not win))
        AddStringToBuffer("</table>", current_line);

    AddHorizontalLineToBrief(win);
    ReadScriptedBriefFile("Threats.b", current_line, win, brief, TheCampaign.MissionEvaluator, flight_data);
    AddHorizontalLineToBrief(win);

    if ((TheCampaign.MissionEvaluator) and (TheCampaign.MissionEvaluator->player_element))
    {
        Flight fl;

        fl = (Flight)FindUnit(TheCampaign.MissionEvaluator->player_element->flight_id);

        if (fl)
        {
            ReadScriptedBriefFile("SteerPtH.b", current_line, win, brief, TheCampaign.MissionEvaluator, flight_data);
            CWayPoint = fl->GetFirstUnitWP();
            LWayPoint = NULL;
            TheCampaign.MissionEvaluator->curr_data = 1;
        }
    }

    while (CWayPoint)
    {
        ReadScriptedBriefFile("SteerPt.b", current_line, win, brief, TheCampaign.MissionEvaluator, flight_data);
        LWayPoint = CWayPoint;
        CWayPoint = CWayPoint->GetNextWP();
    }

    if (g_bBriefHTML and ( not win))
        AddStringToBuffer("</table>", current_line);

    AddHorizontalLineToBrief(win);
    ReadScriptedBriefFile("Loadouth.b", current_line, win, brief, TheCampaign.MissionEvaluator, flight_data);
    // List the various ordinances of component flights
    flight_ptr = TheCampaign.MissionEvaluator->flight_data;

    while (flight_ptr)
    {
        TheCampaign.MissionEvaluator->curr_pilot = flight_ptr->pilot_list;
        weaps = 0;
        pilots = flight_ptr->start_aircraft;

        while (TheCampaign.MissionEvaluator->curr_pilot)
        {
            if (TheCampaign.MissionEvaluator->curr_pilot->weapon_types > weaps)
                weaps = TheCampaign.MissionEvaluator->curr_pilot->weapon_types;

            TheCampaign.MissionEvaluator->curr_pilot = TheCampaign.MissionEvaluator->curr_pilot->next_pilot;
        }

        for (TheCampaign.MissionEvaluator->curr_data = 0; TheCampaign.MissionEvaluator->curr_data < pilots; TheCampaign.MissionEvaluator->curr_data += 2)
        {
            for (j = 0; j < weaps or not j; j++)
            {
                TheCampaign.MissionEvaluator->curr_pilot = NULL;
                TheCampaign.MissionEvaluator->curr_weapon = j;
                ReadScriptedBriefFile("Loadout.b", current_line, win, brief, TheCampaign.MissionEvaluator, flight_ptr);
            }
        }

        flight_ptr = flight_ptr->next_flight;
    }

    if (g_bBriefHTML and ( not win))
        AddStringToBuffer("</table>", current_line);

    TheCampaign.MissionEvaluator->curr_pilot = NULL;
    TheCampaign.MissionEvaluator->curr_data = 0;
    AddHorizontalLineToBrief(win);
    ReadScriptedBriefFile("Weather.b", current_line, win, brief, TheCampaign.MissionEvaluator, flight_data);
    AddHorizontalLineToBrief(win);

    if (g_bBriefHTML and ( not win))
        AddStringToBuffer("</table>", current_line);

    ReadScriptedBriefFile("Support.b", current_line, win, brief, TheCampaign.MissionEvaluator, flight_data);
    AddHorizontalLineToBrief(win);
    ReadScriptedBriefFile("RoE.b", current_line, win, brief, TheCampaign.MissionEvaluator, flight_data);
    AddHorizontalLineToBrief(win);
    ReadScriptedBriefFile("Emerganc.b", current_line, win, brief, TheCampaign.MissionEvaluator, flight_data);
    AddHorizontalLineToBrief(win);
    ReadScriptedBriefFile("End.b", current_line, win, brief, TheCampaign.MissionEvaluator, flight_data);
    CampLeaveCriticalSection();
    return 1;
}

int BuildDebriefString(C_Window *win, _TCHAR *brief)
{
    int i, j, pn, inbox, width, w, x, y;
    EventElement *theEvent;
    C_ListBox *eventListBox = NULL;
    FlightDataClass *flight_data, *flight_ptr;
    _TCHAR current_line[MAX_STRLEN_PER_PARAGRAPH] = {0}; // Text in current line

    CampEnterCriticalSection();
    flight_data = TheCampaign.MissionEvaluator->player_element;
    ShiAssert(flight_data);//Cobra 10/31/04 TJL

    if ( not TheCampaign.MissionEvaluator->player_element or // MLR 3/25/2004 -
 not TheCampaign.MissionEvaluator->player_pilot)
    {
        // this prevents a CTD, but makes the debried window empty
        return 0;
    }

    ShiAssert(flight_data->mission not_eq AMIS_TRAINING);
    ShiAssert(TheCampaign.MissionEvaluator->player_pilot);

    TheCampaign.MissionEvaluator->curr_pilot = TheCampaign.MissionEvaluator->player_pilot;
    ReadScriptedBriefFile("header.db", current_line, win, brief, TheCampaign.MissionEvaluator, flight_data);
    AddHorizontalLineToBrief(win);
    ReadScriptedBriefFile("pheader.db", current_line, win, brief, TheCampaign.MissionEvaluator, flight_data);
    flight_ptr = TheCampaign.MissionEvaluator->flight_data;

    while (flight_ptr)
    {
        ReadScriptedBriefFile("element.db", current_line, win, brief, TheCampaign.MissionEvaluator, flight_ptr);
        // Do Relevent events here
        GetCurrentBriefXY(&x, &y, current_line, win, brief);
        inbox = width = 0;
        theEvent = flight_ptr->root_event;

        while (theEvent)
        {
            if (win)
            {
                if ( not inbox)
                {
                    _TCHAR tmp[40] = "";
                    eventListBox = new C_ListBox;
                    eventListBox->Setup(C_DONT_CARE, 0, gMainHandler);
                    eventListBox->SetFont(win->Font_);
                    eventListBox->SetXY(x - 5, y);
                    eventListBox->SetDropDown(BID_DROPDOWN);
                    eventListBox->SetNormColor(CBColor);
                    eventListBox->SetSelColor(CBColor);
                    eventListBox->SetDisColor(CBColor);
                    eventListBox->SetBgColor(RGB(0, 0, 0));
                    eventListBox->SetBarColor(RGB(0, 80, 127));
                    eventListBox->SetParent(win);
                    eventListBox->SetClient(0);
                    eventListBox->SetFlagBitOn(C_BIT_USEBGFILL);
                    eventListBox->SetUserNumber(_UI95_DELGROUP_SLOT_, _UI95_DELGROUP_ID_);
                    eventListBox->AddScrollBar(BID_SCROLLCAP_TOP_OFF, BID_SCROLLCAP_TOP_ON, BID_SCROLLCAP_BOTTOM_OFF, BID_SCROLLCAP_BOTTOM_ON, BID_SCROLL);
                    inbox++;
                    AddIndexedStringToBuffer(151, tmp);
                    eventListBox->AddItem(inbox, C_TYPE_ITEM, tmp);
                }

                inbox++;
                eventListBox->AddItem(inbox, C_TYPE_ITEM, theEvent->eventString);
                w = gFontList->StrWidth(win->Font_, theEvent->eventString) + 10;

                if (w > width)
                    width = w;
            }
            else
            {
                CTextPtr = theEvent->eventString;
                ReadScriptedBriefFile("FlEvent.db", current_line, win, brief, TheCampaign.MissionEvaluator, flight_data);
            }

            theEvent = theEvent->next;
        }

        if (eventListBox)
        {
            C_Box *box = new C_Box;
            eventListBox->SetWH(width, gFontList->GetHeight(win->Font_));
            eventListBox->SetFlagBitOn(C_BIT_ENABLED);
            win->AddControl(eventListBox);
            eventListBox = NULL;
            box->Setup(C_DONT_CARE, C_TYPE_VERTICAL);
            box->SetXYWH(x - 6, y, width + 1, gFontList->GetHeight(win->Font_));
            // box->SetFlagBitOn(C_BIT_ABSOLUTE);
            box->SetColor(0);
            box->SetParent(win);
            box->SetClient(0);
            box->SetUserNumber(_UI95_DELGROUP_SLOT_, _UI95_DELGROUP_ID_);
            win->AddControl(box);
            AddEOLToBrief(current_line, win, brief);
        }

        flight_ptr = flight_ptr->next_flight;
    }

    AddHorizontalLineToBrief(win);
    ReadScriptedBriefFile("flight.db", current_line, win, brief, TheCampaign.MissionEvaluator, flight_data);
    pn = 0;
    TheCampaign.MissionEvaluator->curr_pilot = TheCampaign.MissionEvaluator->FindPilotDataFromAC(flight_data, pn);

    while (pn < PILOTS_PER_FLIGHT)
    {
        if (TheCampaign.MissionEvaluator->curr_pilot)
        {
            // Total ordinance fired by type - build and traverse table
            for (j = 0; j < TheCampaign.MissionEvaluator->curr_pilot->weapon_types; j++)
            {
                TheCampaign.MissionEvaluator->curr_weapon = j;
                ReadScriptedBriefFile("FOrdnce.db", current_line, win, brief, TheCampaign.MissionEvaluator, flight_data);
                // Do Relevent events here
                GetCurrentBriefXY(&x, &y, current_line, win, brief);
                inbox = width = 0;
                theEvent = TheCampaign.MissionEvaluator->curr_pilot->weapon_data[j].root_event;

                while (theEvent)
                {
                    if (win)
                    {
                        _TCHAR event_str[256] = {0};
                        _TCHAR temp_line[256] = {0};

                        if ( not inbox)
                        {
                            AddEOLToBrief(current_line, win, brief);
                            CCurrentLine = 0;
                            CBX = CLineStart = 0;
                            ReadScriptedBriefFile("FOrdWeap.db", temp_line, NULL, event_str, TheCampaign.MissionEvaluator, flight_data);
                            eventListBox = new C_ListBox;
                            eventListBox->Setup(C_DONT_CARE, 0, gMainHandler);
                            eventListBox->SetFont(win->Font_);
                            eventListBox->SetXY(x - 5, y);
                            eventListBox->SetDropDown(BID_DROPDOWN);
                            eventListBox->SetNormColor(CBColor);
                            eventListBox->SetSelColor(CBColor);
                            eventListBox->SetDisColor(CBColor);
                            eventListBox->SetBgColor(RGB(0, 0, 0));
                            eventListBox->SetBarColor(RGB(0, 80, 127));
                            eventListBox->SetParent(win);
                            eventListBox->SetClient(0);
                            eventListBox->SetFlagBitOn(C_BIT_USEBGFILL);
                            eventListBox->SetUserNumber(_UI95_DELGROUP_SLOT_, _UI95_DELGROUP_ID_);
                            eventListBox->AddScrollBar(BID_SCROLLCAP_TOP_OFF, BID_SCROLLCAP_TOP_ON, BID_SCROLLCAP_BOTTOM_OFF, BID_SCROLLCAP_BOTTOM_ON, BID_SCROLL);
                            inbox++;
                            eventListBox->AddItem(inbox, C_TYPE_ITEM, event_str);
                            width = gFontList->StrWidth(win->Font_, event_str) + 30;
                            event_str[0] = 0;
                        }

                        inbox++;
                        CTextPtr = theEvent->eventString;
                        CCurrentLine = 0;
                        CBX = CLineStart = 0;
                        ReadScriptedBriefFile("FOrdEvt.db", temp_line, NULL, event_str, TheCampaign.MissionEvaluator, flight_data);
                        AddTabToBrief(x, current_line, win, brief);
                        eventListBox->AddItem(inbox, C_TYPE_ITEM, event_str);
                        w = gFontList->StrWidth(win->Font_, event_str) + 10;
                        ShiAssert(w < 500);

                        if (w > width)
                            width = w;
                    }
                    else
                    {
                        if ( not inbox)
                        {
                            ReadScriptedBriefFile("FOrdWeap.db", current_line, win, brief, TheCampaign.MissionEvaluator, flight_data);
                            _tcscat(brief, "\n");
                        }

                        CTextPtr = theEvent->eventString;
                        ReadScriptedBriefFile("FOrdEvt.db", current_line, win, brief, TheCampaign.MissionEvaluator, flight_data);
                        AddEOLToBrief(current_line, NULL, brief);
                    }

                    theEvent = theEvent->next;
                }

                if (eventListBox)
                {
                    C_Box *box = new C_Box;
                    eventListBox->SetWH(width, gFontList->GetHeight(win->Font_));
                    eventListBox->SetFlagBitOn(C_BIT_ENABLED);
                    win->AddControl(eventListBox);
                    eventListBox = NULL;
                    box->Setup(C_DONT_CARE, C_TYPE_VERTICAL);
                    box->SetXYWH(x - 6, y, width + 1, gFontList->GetHeight(win->Font_));
                    box->SetColor(0);
                    box->SetParent(win);
                    box->SetClient(0);
                    box->SetUserNumber(_UI95_DELGROUP_SLOT_, _UI95_DELGROUP_ID_);
                    win->AddControl(box);
                }
            }
        }

        pn++;
        TheCampaign.MissionEvaluator->curr_pilot = TheCampaign.MissionEvaluator->FindPilotDataFromAC(flight_data, pn);
    }

    TheCampaign.MissionEvaluator->curr_pilot = NULL;
    AddHorizontalLineToBrief(win);
    ReadScriptedBriefFile("pilot.db", current_line, win, brief, TheCampaign.MissionEvaluator, flight_data);
    flight_ptr = TheCampaign.MissionEvaluator->flight_data;

    while (flight_ptr)
    {
        PilotDataClass *pilot_data, *ai_data;
        int done, players;

        // Traverse all pilots (combine player's stats with associated AI's if in the same aircraft)
        for (i = 0; i < PILOTS_PER_FLIGHT; i++)
        {
            done = players = 0;
            CPilotData = ai_data = NULL;
            pilot_data = flight_ptr->pilot_list;

            // Fast forward over AI pilots and collect the AI pilot_data for this aircraft.
            while (pilot_data and pilot_data->pilot_slot == pilot_data->aircraft_slot)
            {
                if (pilot_data->pilot_slot == i)
                    ai_data = pilot_data;

                pilot_data = pilot_data->next_pilot;
            }

            // Find all players who were in this AC first
            while ( not done)
            {
                while (pilot_data and pilot_data->aircraft_slot not_eq i)
                    pilot_data = pilot_data->next_pilot;

                if (pilot_data)
                {
                    // This is a player pilot
                    CPilotData = ai_data;
                    TheCampaign.MissionEvaluator->curr_pilot = pilot_data;
                    ReadScriptedBriefFile("PElement.db", current_line, win, brief, TheCampaign.MissionEvaluator, flight_ptr);
                    players++;
                    pilot_data = pilot_data->next_pilot;
                }
                else
                    done = 1;
            }

            // Now check if a player's been added, and if not do the AI's stats
            if ( not players and ai_data)
            {
                TheCampaign.MissionEvaluator->curr_pilot = ai_data;
                ReadScriptedBriefFile("PElement.db", current_line, win, brief, TheCampaign.MissionEvaluator, flight_ptr);
            }
        }

        TheCampaign.MissionEvaluator->curr_pilot = NULL;
        flight_ptr = flight_ptr->next_flight;
    }

    AddHorizontalLineToBrief(win);
    ReadScriptedBriefFile("results.db", current_line, win, brief, TheCampaign.MissionEvaluator, flight_data);
    AddHorizontalLineToBrief(win);
    ReadScriptedBriefFile("related.db", current_line, win, brief, TheCampaign.MissionEvaluator, flight_data);
    CampLeaveCriticalSection();

    return 1;
}

// These functions are intended to be called by the Sim for the kneeboard data
// This is a fairly dangerous function call, since the length argument isn't used to physically bound
// the string copys (any can't be with the current call interface). It is assumed the string is big enough
// to handle everything copied in (including foreign language-long-ass-strings). Just make sure the
// buffer is big enough.
int GetBriefingData(int query, int data, _TCHAR *buffer, int len)
{
    FlightDataClass *flight_ptr;
    Flight fl;

    _TCHAR current_line[MAX_STRLEN_PER_PARAGRAPH] = {0};
    buffer[0] = 0;

    switch (query)
    {
        case GBD_PLAYER_ELEMENT:
            if ( not TheCampaign.MissionEvaluator->player_element)
                return -1;

            if ( not TheCampaign.MissionEvaluator->player_pilot)
                return -1;

            AddStringToBuffer(TheCampaign.MissionEvaluator->player_element->name, buffer);
            AddNumberToBuffer(TheCampaign.MissionEvaluator->player_pilot->aircraft_slot + 1, buffer);
            break;

        case GBD_PLAYER_TASK:
            if ( not TheCampaign.MissionEvaluator->player_element)
                return -1;

            ReadScriptedBriefFile("task.gbd", current_line, NULL, buffer, TheCampaign.MissionEvaluator, TheCampaign.MissionEvaluator->player_element);
            break;

        case GBD_PACKAGE_LABEL:
            AddIndexedStringToBuffer(107, buffer);
            break;

        case GBD_PACKAGE_MISSION:
            if ( not TheCampaign.MissionEvaluator->package_element)
                return -1;

            ReadScriptedBriefFile("mission.gbd", current_line, NULL, buffer, TheCampaign.MissionEvaluator, TheCampaign.MissionEvaluator->package_element);
            break;

        case GBD_PACKAGE_ELEMENT_NAME:
            flight_ptr = TheCampaign.MissionEvaluator->flight_data;

            while (flight_ptr and data)
            {
                flight_ptr = flight_ptr->next_flight;
                data--;
            }

            if (flight_ptr)
                AddStringToBuffer(flight_ptr->name, buffer);
            else
                return -1;

            break;

        case GBD_PACKAGE_ELEMENT_TASK:
            flight_ptr = TheCampaign.MissionEvaluator->flight_data;

            while (flight_ptr and data)
            {
                flight_ptr = flight_ptr->next_flight;
                data--;
            }

            if (flight_ptr)
                AddIndexedStringToBuffer(300 + flight_ptr->mission, buffer);
            else
                return -1;

            break;

        case GBD_PACKAGE_STPTHDR:
            sprintf(buffer, "%3s %8s %3s %3s %4s %4s %s %s",
                    "Wpt", "Time", "Hdg", "Spd", "Alt", "Dist", "", "");
            break;

        case GBD_PACKAGE_STPT:
        {
            int wpno = 1;
            //JPO extra checks.
            ShiAssert(FALSE == F4IsBadReadPtr(TheCampaign.MissionEvaluator, sizeof * TheCampaign.MissionEvaluator));
            ShiAssert(FALSE == F4IsBadReadPtr(TheCampaign.MissionEvaluator->player_element, sizeof * TheCampaign.MissionEvaluator->player_element));

            fl = NULL;

            if (TheCampaign.MissionEvaluator and TheCampaign.MissionEvaluator->player_element)
                fl = (Flight)FindUnit(TheCampaign.MissionEvaluator->player_element->flight_id);

            if (fl)
            {
                CWayPoint = fl->GetFirstUnitWP();
                LWayPoint = NULL;
                TheCampaign.MissionEvaluator->curr_data = 1;
            }

            while (CWayPoint and data > 0)
            {
                data --;
                wpno ++;
                LWayPoint = CWayPoint;
                CWayPoint = CWayPoint->GetNextWP();
            }

            if (CWayPoint)
            {
                _TCHAR time[32], action[32], distance[32], hdg[32], spd[32], alt[32], desc[32];
                time[0] = action[0] = distance[0] = hdg[0] = spd[0] = alt[0] = desc[0] = 0;
                GetWpActionToBuffer(CWayPoint, action);
                GetWpTimeToBuffer(CWayPoint, time);
                GetWptDist(CWayPoint, LWayPoint, distance);
                GetWptSpeed(CWayPoint, LWayPoint, spd);
                GetWpAlt(CWayPoint, LWayPoint, alt);
                GetWpHeading(CWayPoint, LWayPoint, hdg);
                GetWpDescription(CWayPoint, desc);

                if (action[0] == '-') action[0] = 0;

                if (desc[0] == '-') desc[0] = 0;

                sprintf(buffer, "%3d %8s %3s %3s %5s %4s %s %s",
                        wpno, time, hdg, spd, alt, distance, action, desc);
            }
            else
                return -1;
        }
        break;

        default:
            break;
    }

    buffer[len - 1] = 0;
    return 1;
}

// =====================
// Support functions
// =====================

// JPO - broken out a bit so I can reuse them
static void GetWpActionToBuffer(WayPoint wp, _TCHAR *cline)
{
    if (wp->GetWPAction() == WP_NOTHING)
    {
        if (wp->GetWPRouteAction() == WP_NOTHING)
        {
            if (wp->GetWPFlags() bitand WPF_BREAKPOINT)
                AddStringToBuffer(WPActStr[31], cline);
            // AddIndexedStringToBuffer(381, cline);
            else if (wp->GetWPFlags() bitand WPF_IP)
                AddStringToBuffer(WPActStr[32], cline);
            // AddIndexedStringToBuffer(382, cline);
            else if (wp->GetWPFlags() bitand WPF_TURNPOINT)
                AddStringToBuffer(WPActStr[33], cline);
            // AddIndexedStringToBuffer(383, cline);
            else if (wp->GetWPFlags() bitand WPF_CP)
                AddStringToBuffer(WPActStr[WP_CASCP], cline);
            // AddIndexedStringToBuffer(359, cline);
            else
                AddStringToBuffer(WPActStr[0], cline);

            // AddIndexedStringToBuffer(350, cline);
        }
        else
        {
            AddStringToBuffer(WPActStr[wp->GetWPRouteAction()], cline);
            //  AddIndexedStringToBuffer(350+wp->GetWPRouteAction(), cline);
        }
    }
    else
        AddStringToBuffer(WPActStr[wp->GetWPAction()], cline);

    // AddIndexedStringToBuffer(350+wp->GetWPAction(), cline);
}

static void GetWpTimeToBuffer(WayPoint wp, _TCHAR *cline)
{
    if (wp->GetWPFlags() bitand WPF_ALTERNATE or wp->GetWPAction() == WP_REFUEL)
        AddIndexedStringToBuffer(1650, cline);
    else
        AddTimeToBuffer(wp->GetWPArrivalTime(), cline);
}

static void GetWptDist(WayPoint wp, WayPoint lwp, _TCHAR *cline)
{
    if (wp and lwp and wp->GetWPAction() not_eq WP_REFUEL and not (wp->GetWPFlags() bitand WPF_ALTERNATE))
    {
        GridIndex lx, ly, cx, cy;
        lwp->GetWPLocation(&lx, &ly);
        wp->GetWPLocation(&cx, &cy);
        AddNumberToBuffer(Distance(lx, ly, cx, cy)*KM_TO_NM, 1, cline);
    }
    else
        AddIndexedStringToBuffer(1650, cline);
}

static void GetWptSpeed(WayPoint wp, WayPoint lwp, _TCHAR *cline)
{
    if (wp and lwp and wp->GetWPAction() not_eq WP_REFUEL and not (wp->GetWPFlags() bitand WPF_ALTERNATE))
    {
        /*
        GridIndex cx,cy,lx,ly;
        float speed,dist,time;
        int ispeed;
        LWayPoint->GetWPLocation(&lx,&ly);
        wp->GetWPLocation(&cx,&cy);
        // Time (in hours)
        time = (float)(wp->GetWPArrivalTime() - lwp->GetWPDepartureTime())/CampaignHours;
        // Distance (in km)
        dist = Distance(lx,ly,cx,cy);
        // Speed (in knots)
        speed = (dist/time)*KPH_TO_FPS*FTPSEC_TO_KNOTS;
        */
        float speed;
        int ispeed;

        if (wp->GetWPFlags() bitand WPF_HOLDCURRENT)
            speed = get_air_speed(wp->GetWPSpeed() * KM_TO_NM, lwp->GetWPAltitude());
        else
            speed = get_air_speed(wp->GetWPSpeed() * KM_TO_NM, wp->GetWPAltitude());

        ispeed = FloatToInt32((speed + 2.5F) / 5.0F) * 5;
        AddNumberToBuffer(ispeed, cline);
    }
    else
        AddIndexedStringToBuffer(1650, cline);
}

static int GetWpAlt(WayPoint wp, WayPoint lwp, _TCHAR *cline)
{

    int alt = wp->GetWPAltitude();

    if (lwp and wp->GetWPFlags() bitand WPF_HOLDCURRENT)
        alt = lwp->GetWPAltitude();

    if (alt > 0)
        AddNumberToBuffer((float)(alt) / 1000.0F, 1, cline);
    else
        AddIndexedStringToBuffer(1650, cline);

    return alt;
}

static void GetWpDescription(WayPoint wp, _TCHAR *cline)
{
    if (wp->GetWPFlags() bitand WPF_ALTERNATE)
        AddIndexedStringToBuffer(237, cline);
    else if (wp->GetWPFlags() bitand WPF_REPEAT)
        AddIndexedStringToBuffer(247, cline);
    else if (wp->GetWPAction() == WP_NOTHING)
        AddIndexedStringToBuffer(1650 + wp->GetWPRouteAction(), cline);
    else
        AddIndexedStringToBuffer(1650 + wp->GetWPAction(), cline);
}

static void GetWpHeading(WayPoint wp, WayPoint lwp, _TCHAR *cline)
{
    if (wp and lwp and wp->GetWPAction() not_eq WP_REFUEL and not (wp->GetWPFlags() bitand WPF_ALTERNATE))
    {
        GridIndex cx, cy, lx, ly;
        float heading;
        int ih;
        lwp->GetWPLocation(&lx, &ly);
        wp->GetWPLocation(&cx, &cy);
        heading = AngleTo(lx, ly, cx, cy);
        ih = FloatToInt32((heading * RADIANS_TO_DEG) + 0.5F);

        if (ih < 0)
            ih += 360;

        AddNumberToBuffer(ih, cline);
    }
    else
        AddIndexedStringToBuffer(1650, cline);
}
// These add controls to a Window or String
void AddHorizontalLineToBrief(C_Window *window)
{
    if (window)
    {
        short CBYtemp = CBY + gFontList->GetHeight(window->Font_) / 2;
        short CBXtemp = 0;
        AddHorizontalLineToWindow(window , &CBXtemp, &CBYtemp,
                                  0, static_cast<short>(window->ClientArea_[0].right - window->ClientArea_[0].left - 10),
                                  CBColor);
    }
}

void AddStringToBrief(_TCHAR *buffer, C_Window *window, _TCHAR *output)
{
    if ( not buffer[0])
        return;

    if (window)
        AddWordWrapTextToWindow(window, &CBX, &CBY,
                                static_cast<short>(CLineStart),
                                static_cast<short>(window->ClientArea_[0].right - window->ClientArea_[0].left - 10),
                                CBColor, buffer);
    else
    {
        ShiAssert(output not_eq buffer);
        _tcscat(output, buffer);

        if ( not CCurrentLine)
            CCurrentLine = output;

#if 0 // this isn't helping the print out.

        if (gFontList)
            CBX = CLineStart + gFontList->StrWidth(BDefaultFont, CCurrentLine);
        else
#endif
            CBX = CLineStart + strlen(CCurrentLine);
    }

    buffer[0] = 0;
}

void GetCurrentBriefXY(int *x, int *y, _TCHAR *buffer, C_Window *window, _TCHAR *output)
{
    // KCK: The only way we can REALLY know current x bitand y position is to have the UI add it for us
    // (and therefore do all appropriate wrapping, compression of spaces, etc, etc)
    AddStringToBrief(buffer, window, output);
    *x = CBX;
    *y = CBY;
}

void AddTabToBrief(int tab, _TCHAR *buffer, C_Window *window, _TCHAR *output)
{
    // We have to dump our current line in order to goto a new location
    AddStringToBrief(buffer, window, output);

    if (window)
    {
        CBX = tab;
        CLineStart = CBX;
    }
    else
    {
        tab /= 10;
        // Insert spaces until we get to the right location
        // JPO - rework this so we always get at least one separator
        _TCHAR htmltab[5] = "<td>";
        _TCHAR txttab[2] = "\t";

        if (g_bBriefHTML)
            AddStringToBrief(htmltab, window, output);
        else
        {
            //do {
            //_TCHAR space[2] = " "; //THW get a real tab for better layout in files and printout
            AddStringToBrief(txttab, window, output);
            //} while (CBX < tab-1);
        }
    }
}

void AddTabToDebrief(int tab, _TCHAR *buffer, C_Window *window, _TCHAR *output)
{
    AddStringToBrief(buffer, window, output);

    if (window)
    {
        CBX = tab;
        CLineStart = CBX;
    }
    else
    {
        tab /= 10;
        _TCHAR txttab[2] = " ";
        //do {
        AddStringToBrief(txttab, window, output);
        //} while (CBX < tab-1);
    }
}

void AddEOLToBrief(_TCHAR *buffer, C_Window *window, _TCHAR *output)
{
    // Dump current line
    AddStringToBrief(buffer, window, output);

    if (window)
        CBY += gFontList->GetHeight(window->Font_);
    else if (output)
    {
        // Insert linefeed
        _TCHAR eol[2] = "\n";
        AddStringToBrief(eol, window, output);
        CCurrentLine = 0;
    }

    CBX = 0;
    CLineStart = 0;
}

void AddRightJustifiedStringToBrief(_TCHAR *string, int field_width, _TCHAR *buffer, C_Window *window, _TCHAR *output)
{
    int width, x, y;

    if (window)
        width = gFontList->StrWidth(window->Font_, string);

#if 0 // to improve the looks
    else if (gFontList)
        width = gFontList->StrWidth(BDefaultFont, string);

#endif
    else width = strlen(string);

    GetCurrentBriefXY(&x, &y, buffer, window, output);

    if (width < field_width)
        AddTabToBrief(x + field_width - width, "", window, output);

    AddStringToBrief(string, window, output);
}

void AddFontTextToBrief(_TCHAR *buffer, int font, C_Window *window, _TCHAR *output)
{
    if (window)
    {
        int height, oldheight, oldfont;
        oldfont = window->Font_;
        oldheight = gFontList->GetHeight(oldfont);
        height = gFontList->GetHeight(font);

        // KCK HACK: Some fonts just don't get very good heights
        if (font == 14)
        {
            if (gLangIDNum == F4LANG_GERMAN)
                ;
            else if (gLangIDNum >= F4LANG_SPANISH)
                height--;
            else
                height = 19;
        }

        CBY += oldheight - height;
        window->Font_ = font;
        AddStringToBrief(buffer, window, output);
        CBY -= oldheight - height;
        window->Font_ = oldfont;
    }
    else
        AddStringToBrief(buffer, window, output);
}

// These add strings to a buffer
void AddStringToBuffer(_TCHAR *string, _TCHAR *buffer)
{
    ShiAssert(string not_eq buffer);

    _tcscat(buffer, string);
}

void AddIndexedStringToBuffer(int sid, _TCHAR *buffer)
{
    _TCHAR wstring[MAX_STRLEN_PER_TOKEN];

    ReadIndexedString(sid, wstring, MAX_STRLEN_PER_TOKEN);
    AddStringToBuffer(wstring, buffer);
}

void AddNumberToBuffer(int num, _TCHAR *buffer)
{
    _TCHAR string[32];
    _stprintf(string, _T("%d"), num);
    _tcscat(buffer, string);
}

void AddNumberToBuffer(float num, int decimals, _TCHAR *buffer)
{
    _TCHAR string[32];

    // Hackish way to do this.. but..
    switch (decimals)
    {
        case 0:
            _stprintf(string, _T("%.0f"), num);
            break;

        case 1:
        default:
            _stprintf(string, _T("%.1f"), num);
            break;

        case 2:
            _stprintf(string, _T("%.2f"), num);
            break;
    }

    _tcscat(buffer, string);
}

void AddTimeToBuffer(CampaignTime time, _TCHAR *buffer, int seconds)
{
    _TCHAR tstring[MAX_STRLEN_PER_TOKEN];

    GetTimeString(time, tstring, seconds);
    AddStringToBuffer(tstring, buffer);
}

void AddLocationToBuffer(char type, GridIndex x, GridIndex y, _TCHAR *buffer)
{
    Objective po, bpo = NULL;
    float d, bd = 9999.0F;
    GridIndex ox, oy;
    _TCHAR wdstr[41], wtmp[41], name[61], format[80], dist[10];
    CampaignHeading h;
    VuListIterator *oit;

    if (type == 'G' or type == 'g')
        oit = new VuListIterator(POList);
    else if (type == 'E' or type == 'e')
        oit = new VuListIterator(AllObjList);
    else
        oit = new VuListIterator(SOList);

    po = GetFirstObjective(oit);

    while (po)
    {
        po->GetLocation(&ox, &oy);
        d = Distance(x, y, ox, oy);

        if (d < bd)
        {
            bd = d;
            bpo = po;
        }

        po = GetNextObjective(oit);
    }

    if (bpo)
    {
        bpo->GetLocation(&ox, &oy);
        bpo->GetName(name, 60, FALSE);

        if ((type == 'g' or type == 's') and bd < 2.0F)
            h = Here;
        else
            h = DirectionTo(ox, oy, x, y);

        if (h < Here or type == 'T' or type == 't' or type == 'E' or type == 'e')
        {
            ReadIndexedString(30 + h, wdstr, 40);

            switch (type)
            {
                case 'N':
                case 'n':

                    // Say 'direction of name'
                    if (gLangIDNum == F4LANG_FRENCH and (name[0] == 'A' or name[0] == 'a' or name[0] == 'E' or name[0] == 'e' or name[0] == 'I' or name[0] == 'i' or name[0] == 'O' or name[0] == 'o' or name[0] == 'U' or name[0] == 'u'))
                        ReadIndexedString(3993, format, MAX_STRLEN_PER_TOKEN);
                    else
                        ReadIndexedString(53, format, MAX_STRLEN_PER_TOKEN);

                    ConstructOrderedSentence(40, wtmp, format, wdstr, name);
                    break;

                case 'T':
                case 't':
                case 'e':
                case 'E':
                    // Say 'name'
                    _stprintf(wtmp, name);
                    break;

                case 'g':
                case 's':
                    // Say 'x nm direction of name'
                    bd = bd * GRID_SIZE_FT * FT_TO_NM;
                    _stprintf(dist, "%d", FloatToInt32(bd));

                    if (gLangIDNum == F4LANG_FRENCH and (name[0] == 'A' or name[0] == 'a' or name[0] == 'E' or name[0] == 'e' or name[0] == 'I' or name[0] == 'i' or name[0] == 'O' or name[0] == 'o' or name[0] == 'U' or name[0] == 'u'))
                        ReadIndexedString(3992, format, MAX_STRLEN_PER_TOKEN);
                    else
                        ReadIndexedString(52, format, MAX_STRLEN_PER_TOKEN);

                    ConstructOrderedSentence(40, wtmp,  format, dist, wdstr, name);
                    break;

                default:
                    // Say 'x km direction of name'
                    _stprintf(dist, "%d", FloatToInt32(bd));

                    if (gLangIDNum == F4LANG_FRENCH and (name[0] == 'A' or name[0] == 'a' or name[0] == 'E' or name[0] == 'e' or name[0] == 'I' or name[0] == 'i' or name[0] == 'O' or name[0] == 'o' or name[0] == 'U' or name[0] == 'u'))
                        ReadIndexedString(3991, format, MAX_STRLEN_PER_TOKEN);
                    else
                        ReadIndexedString(51, format, MAX_STRLEN_PER_TOKEN);

                    ConstructOrderedSentence(40, wtmp, format, dist, wdstr, name);
                    break;
            }
        }
        else if (bpo->GetType() == TYPE_CITY or bpo->GetType() == TYPE_TOWN)
        {
            if (type > 'a' and type < 'z')
            {
                // Say 'over x'
                ReadIndexedString(56, format, MAX_STRLEN_PER_TOKEN);
            }
            else
            {
                // Say 'within x'
                ReadIndexedString(55, format, MAX_STRLEN_PER_TOKEN);
            }

            ConstructOrderedSentence(40, wtmp, format, name);
        }
        else
        {
            // Just say 'near x'
            if (gLangIDNum == F4LANG_FRENCH and (name[0] == 'A' or name[0] == 'a' or name[0] == 'E' or name[0] == 'e' or name[0] == 'I' or name[0] == 'i' or name[0] == 'O' or name[0] == 'o' or name[0] == 'U' or name[0] == 'u'))
                ReadIndexedString(3994, format, MAX_STRLEN_PER_TOKEN);
            else
                ReadIndexedString(54, format, MAX_STRLEN_PER_TOKEN);

            ConstructOrderedSentence(40, wtmp, format, name);
        }

        AddStringToBuffer(wtmp, buffer);
    }

    delete oit;
}

void GetEntityName(CampEntity e, _TCHAR *name, char name_type, char objchar)
{
    int object = FALSE;

    if (objchar == 'O')
        object = TRUE;

    if ( not e)
        ReadIndexedString(168, name, 80);
    else if (e->IsObjective())
        e->GetName(name, 80, object);
    else if (e->IsUnit())
    {
        Unit parent_unit = NULL;

        switch (name_type)
        {
            case 'D':
                ((Unit)e)->GetDivisionName(name, 80, object);
                break;

            case 'P':
                // KCK: This is dangerous to do. Look into fixing this so we don't need to do it.
                parent_unit = ((Unit)e)->GetUnitParent();

                if (parent_unit)
                    ((Unit)parent_unit)->GetName(name, 80, object);
                else
                    ((Unit)e)->GetName(name, 80, object);

                break;

            case 'F':
                ((Unit)e)->GetFullName(name, 80, object);
                break;

            case 'U':
            default:
                ((Unit)e)->GetName(name, 80, object);
                break;
        }
    }
}

void GetEntityDestination(CampEntity e, _TCHAR *name)
{
    if ( not e)
        ReadIndexedString(38, name, 80);
    else if (e->IsUnit())
    {
        if (e->GetDomain() == DOMAIN_LAND)
        {
            Objective o = ((Unit)e)->GetUnitSecondaryObj();

            if (o)
                o->GetName(name, 80, FALSE);
            else
                ReadIndexedString(253, name, 80);
        }
        else if (e->GetDomain() == DOMAIN_AIR)
        {
            // Find out where this flight is going (find the target)
            WayPoint w = ((Unit)e)->GetFirstUnitWP();
            CampEntity t;

            while (w and not (w->GetWPFlags() bitand WPF_TARGET))
                w = w->GetNextWP();

            if (w)
            {
                t = w->GetWPTarget();
                GetEntityName(t, name, ' ', ' ');
            }
            else
                ReadIndexedString(253, name, 80);
        }
    }
    else
        GetEntityName(e, name, ' ', ' ');
}

void ReadComments(FILE* fh)
{
    int c;

    c = fgetc(fh);

    while (c == '\n')
        c = fgetc(fh);

    while (c == '/' and not feof(fh))
    {
        c = fgetc(fh);

        while (c not_eq '\n' and not feof(fh))
            c = fgetc(fh);

        while (c == '\n')
            c = fgetc(fh);
    }

    ungetc(c, fh);
}

char* ReadToken(FILE *fp, char name[], int len)
{
    char buffer[256];
    char *sptr;

    fgets(buffer, 256, fp);
    strncpy(name, buffer, len);

    if (name[len - 1])
        name[len - 1] = 0;

    sptr = strchr(name, '\n');

    if (sptr)
        *sptr = '\0';

    return name;
}

char* ReadMemToken(char **ptr, char name[], int len)
{
    char
    *src,
    *dst;

    src = *ptr;
    dst = name;

    while ((*src == '\n') or (*src == '\r'))
    {
        *src ++;
    }

    while ((len) and (*src))
    {
        if ((*src == '\n') or (*src == '\r'))
        {
            break;
        }

        *dst ++ = *src ++;

        len --;
    }

    *dst = 0;

    return name;
}

void ConstructOrderedSentence(short maxsize, _TCHAR *string, _TCHAR *format, ...)
{
    int done = 0, count = 0, index = 0, size;
    va_list params;
    _TCHAR argstring[MAX_STRLEN_PER_TOKEN], addchar[2];

    string[0] = 0;
    size = _tcslen(format);

    while (format[index])
    {
        if (format[index] == '#')
        {
            // read and add the numbered argument
            index++;
            count = format[index] - '0'; // arg #
            va_start(params, format);       // Initialize variable arguments.

            while (count >= 0)
            {
                sprintf(argstring, va_arg(params, _TCHAR*));
                count--;
            }

            va_end(params); // Reset variable arguments.
            // Check for buffer overflow
            size -= 2; // The substitution
            ShiAssert(static_cast<short>(size + _tcslen(argstring)) < maxsize);
            size += _tcslen(argstring);

            if (size > maxsize)
                return;

            _tcscat(string, argstring);
        }
        else
        {
            // Add the character
            addchar[0] = format[index];
            addchar[1] = 0;
            _tcscat(string, addchar);
        }

        index++;
    }
}

int GetGender(CampEntity entity, int div)
{
    // Gender is hardcoded for now- no data exists
    if ( not entity or gLangIDNum < F4LANG_GERMAN)
        return F4LANG_MASCULINE;

    if (div and ( not entity->IsUnit() or not ((Unit)entity)->GetUnitDivision()))
        div = 0;

    if (gLangIDNum == F4LANG_GERMAN)
    {
        if (div)
            return F4LANG_FEMININE;
        else if (entity->IsBattalion())
            return F4LANG_NEUTER;
        else if (entity->IsFlight())
            return F4LANG_MASCULINE;
        else
            return F4LANG_FEMININE;
    }
    else if (gLangIDNum == F4LANG_FRENCH)
    {
        if (div)
            return F4LANG_FEMININE;
        else if (entity->IsBattalion() or entity->IsSquadron() or entity->IsTaskForce())
            return F4LANG_MASCULINE;
        else
            return F4LANG_FEMININE;
    }
    else if (gLangIDNum == F4LANG_ITALIAN)
    {
        if (div)
            return F4LANG_FEMININE;
        else if (entity->IsBattalion() or entity->IsPackage())
            return F4LANG_MASCULINE;
        else
            return F4LANG_FEMININE;
    }
    else if (gLangIDNum == F4LANG_SPANISH)
    {
        if (div)
            return F4LANG_FEMININE;
        else if (entity->IsBattalion() or entity->IsFlight() or entity->IsSquadron())
            return F4LANG_MASCULINE;
        else
            return F4LANG_FEMININE;
    }
    else if (gLangIDNum == F4LANG_PORTUGESE)
    {
        if (div)
            return F4LANG_FEMININE;
        else if (entity->IsBattalion() or entity->IsPackage() or entity->IsSquadron())
            return F4LANG_MASCULINE;
        else
            return F4LANG_FEMININE;
    }

    return F4LANG_MASCULINE;
}

void ConstructOrderedGenderedSentence(short maxsize, _TCHAR *string, EventDataClass *data, ...)
{
    _TCHAR argstring[MAX_STRLEN_PER_TOKEN], addchar[2];
    _TCHAR format[1024];
    int size, index = 0;
    int plural, to_upper, gender, usage, stridx, mode;
    CampEntity entity;
    va_list params;

    string[0] = 0;
    ReadIndexedString(data->formatId, format, 1024);
    size = _tcslen(format);

    while (format[index])
    {
        if (format[index] == '#')
        {
            // Evaluate the expression
            index++;
            plural = 0;
            to_upper = 0;
            usage = 0;
            mode = 0;
            gender = -1;
            argstring[0] = 0;

            // Check for an artical, country name, description or Location
            if (format[index] == 'A' or format[index] == 'a')
            {
                if (format[index] == 'A')
                    to_upper = 1;

                index++;

                // Check for Gender
                if (format[index] == 'm' or format[index] == 'M')
                {
                    if (format[index] == 'M')
                        plural = 1;

                    index++;
                    gender = F4LANG_MASCULINE;
                }
                else if (format[index] == 'f' or format[index] == 'F')
                {
                    if (format[index] == 'F')
                        plural = 1;

                    index++;
                    gender = F4LANG_FEMININE;
                }
                else if (format[index] == 'n' or format[index] == 'N')
                {
                    if (format[index] == 'N')
                        plural = 1;

                    index++;
                    gender = F4LANG_NEUTER;
                }

                // Check for Usage
                if (format[index] == 'O')
                {
                    index++;
                    usage = 1;
                }
                // Check for Usage
                else if (format[index] == 'o')
                {
                    index++;
                    usage = 2;
                }

                // Now find which item it refers to
                if (gender < 0)
                {
                    // Find the entity
                    ShiAssert(format[index] == '0' or format[index] == '1');
                    entity = (CampEntity) vuDatabase->Find(data->vuIds[format[index] - '0']);
                    // Check for force to division
                    _TCHAR *sptr = strstr(format, "#DD");
                    int div = 0;

                    if (sptr and sptr[3] == format[index])
                        div = 1;

                    gender = GetGender(entity, div);
                }

                // Find the artical
                stridx = 3800 + 6 * usage + 3 * plural + gender;
                AddIndexedStringToBuffer(stridx, argstring);

                if (to_upper and _istlower(argstring[0]))
                    argstring[0] = _toupper(argstring[0]);
            }
            else if (format[index] == 'C' or format[index] == 'c')
            {
                if (format[index] == 'C')
                    to_upper = 1;

                index++;

                // Check for Gender
                if (format[index] == 'm' or format[index] == 'M')
                {
                    if (format[index] == 'M')
                        plural = 1;

                    index++;
                    gender = F4LANG_MASCULINE;
                }
                else if (format[index] == 'f' or format[index] == 'F')
                {
                    if (format[index] == 'F')
                        plural = 1;

                    index++;
                    gender = F4LANG_FEMININE;
                }
                else if (format[index] == 'n' or format[index] == 'N')
                {
                    if (format[index] == 'N')
                        plural = 1;

                    index++;
                    gender = F4LANG_NEUTER;
                }

                // Check for Usage
                if (format[index] == 'O')
                {
                    index++;
                    usage = 1;
                }
                else if (format[index] == 'o')
                {
                    index++;
                    usage = 2;
                }

                // Now find which item it refers to
                if (gender < 0)
                {
                    // Find the entity
                    ShiAssert(format[index] == '0' or format[index] == '1');
                    entity = (CampEntity) vuDatabase->Find(data->vuIds[format[index] - '0']);
                    // Check for force to division
                    _TCHAR *sptr = strstr(format, "#DD");
                    int div = 0;

                    if (sptr and sptr[3] == format[index])
                        div = 1;

                    gender = 0;

                    if (entity)
                        gender = GetGender(entity, div);
                }

                ShiAssert(format[index] == '0' or format[index] == '1');
                // Find the adjective
                stridx = ConvertTeamToStringIndex(data->owners[format[index] - '0'], gender, usage, plural);
                // stridx = 3820 + 20*data->owners[format[index] - '0'] + 6*usage + 3*plural + gender;
                AddIndexedStringToBuffer(stridx, argstring);

                if (to_upper and _istlower(argstring[0]))
                    argstring[0] = _toupper(argstring[0]);
            }
            else if (format[index] == 'D')
            {
                index++;
                // Check for description type
                ShiAssert(format[index] == 'D' or format[index] == 'F' or format[index] == 'B');
                mode = format[index];
                index++;

                // Check for Usage
                if (format[index] == 'O')
                {
                    index++;
                    usage = 1;
                }
                else if (format[index] == 'o')
                {
                    index++;
                    usage = 2;
                }

                // Find the entity
                ShiAssert(format[index] == '0' or format[index] == '1');
                entity = (CampEntity) vuDatabase->Find(data->vuIds[format[index] - '0']);

                if (entity)
                {
                    switch (mode)
                    {
                        case 'D':
                            entity->GetDivisionName(argstring, MAX_STRLEN_PER_TOKEN, usage);
                            break;

                        case 'F':
                            entity->GetFullName(argstring, MAX_STRLEN_PER_TOKEN, usage);
                            break;

                        case 'P':
                            entity->GetName(argstring, MAX_STRLEN_PER_TOKEN, usage);

                            if (entity->IsUnit())
                            {
                                entity = ((Unit)entity)->GetUnitParent();

                                if (entity)
                                    entity->GetName(argstring, MAX_STRLEN_PER_TOKEN, usage);
                            }

                            break;

                        default:
                            entity->GetName(argstring, MAX_STRLEN_PER_TOKEN, usage);
                            break;
                    }
                }

                // Hack for sentance beginnings
                if (string[0] == 0 and _istlower(argstring[0]))
                    argstring[0] = _toupper(argstring[0]);
            }
            else if (format[index] == 'L')
            {
                // add a location
                index++;
                ShiAssert(format[index] == 'N' or format[index] == 'T' or format[index] == 'S' or format[index] == 'G' or format[index] == 'n' or format[index] == 't' or format[index] == 's' or format[index] == 'g' or format[index] == 'E' or format[index] == 'e');
                AddLocationToBuffer(format[index], data->xLoc, data->yLoc, argstring);
            }
            else if (format[index] == 'I')
            {
                // Indexed replacement
                index++;

                if (data->textIds[format[index] - '0'] < 0)
                {
                    VehicleClassDataType *vc;
                    vc = GetVehicleClassData(-1 * data->textIds[format[index] - '0']);
                    ShiAssert(vc not_eq NULL);
                    _stprintf(argstring, vc ? vc->Name : "<unk>");
                }
                else
                {
                    // read and add the numbered argument's string index
                    stridx = data->textIds[format[index] - '0'];
                    AddIndexedStringToBuffer(stridx, argstring);
                }
            }
            else
            {
                // Otherwise direct string substitution from argument list
                int argnum = format[index] - '0'; // arg #
                va_start(params, data); // Initialize variable arguments.

                while (argnum >= 0)
                {
                    sprintf(argstring, va_arg(params, _TCHAR*));
                    argnum--;
                }

                va_end(params); // Reset variable arguments.
            }

            // Check for buffer overflow
            ShiAssert(static_cast<short>(size + _tcslen(argstring)) < maxsize);
            size += _tcslen(argstring);

            if (size > maxsize)
                return;

            _tcscat(string, argstring);
        }
        else
        {
            // Add the character
            addchar[0] = format[index];
            addchar[1] = 0;
            _tcscat(string, addchar);
        }

        index++;
    }
}

// JPO - C++ idea...
class ScriptClass
{
private:
    int stack_active[MAX_STACK];
    int cstack;
    MissionEvaluationClass* mec;
    FlightDataClass *flight_data;
    CampEntity target, ptarget;
    C_Window *win;
    _TCHAR *buffer;
    int len;
public:
    ScriptClass(MissionEvaluationClass* mec, FlightDataClass *flight_data, C_Window *win);
    ScriptClass(MissionEvaluationClass* mec, FlightDataClass *flight_data, _TCHAR *buffer);
    ~ScriptClass();
};

int ReadScriptedBriefFile(char* filename, _TCHAR *current_line, C_Window *win, _TCHAR *brief, MissionEvaluationClass* mec, FlightDataClass *flight_data)
{
    FILE* fp;
    int i, font = 0, done = 0, curr_stack = 0, stack_active[MAX_STACK] = { 1 };
    char token[128], *sptr;
    _TCHAR eol[2] = { '\n', 0 };
    CampEntity target, ptarget;

    if ( not mec or not mec->flight_data or not mec->flight_data->camp_id)
        return 0;

    if ((fp = OpenCampFile(filename, "", "r")) == NULL)
        return 0;

    if (F4IsBadReadPtr(flight_data, sizeof(FlightDataClass))) // JB 010305 CTD
        return 0;

    target = FindEntity(flight_data->target_id);
    ptarget = FindEntity(mec->package_target_id);

    while ( not done)
    {
        ReadComments(fp);
        ReadToken(fp, token, 120);

        if ( not token[0])
            continue;

        // Handle standard tokens
        if (strncmp(token, "#IF", 3) == 0)
        {
            curr_stack++;

            if ( not stack_active[curr_stack - 1])
                stack_active[curr_stack] = 0;
            else
                stack_active[curr_stack] = 1;
        }
        else if (strcmp(token, "#ELSE") == 0)
        {
            if (curr_stack > 0 and stack_active[curr_stack - 1])
                stack_active[curr_stack] = not stack_active[curr_stack];

            continue;
        }
        else if (strcmp(token, "#ENDIF") == 0)
        {
            if ( not curr_stack)
                MonoPrint("<Brief reading Error - unmatched #ENDIF>\n");
            else
                curr_stack--;

            continue;
        }
        else if (strcmp(token, "#ENDSCRIPT") == 0)
        {
            done = 1;
            continue;
        }

        // Check for section activity
        if (stack_active[curr_stack])
        {
            // This section is active, handle tokens
            if (strncmp(token, "#IF", 3) == 0)
            {
                if (curr_stack >= MAX_STACK)
                {
                    MonoPrint("<Brief Reading Error - stack overflow. Max stacks = %d", MAX_STACK);
                    CloseCampFile(fp);
                    return 0;
                }

                // Add all our if conditions here
                if (strcmp(token, "#IF_HAVE_TARGET") == 0)
                {
                    if ( not target)
                        stack_active[curr_stack] = 0;
                    else
                        stack_active[curr_stack] = 1;
                }
                else if (strcmp(token, "#IF_HAVE_PACKAGE_TARGET") == 0)
                {
                    if ( not ptarget)
                        stack_active[curr_stack] = 0;
                    else
                        stack_active[curr_stack] = 1;
                }
                else if (strcmp(token, "#IF_HAVE_TARGET_BUILDING") == 0)
                {
                    if (target and flight_data->target_building < FEATURES_PER_OBJ)
                        stack_active[curr_stack] = 1;
                    else
                        stack_active[curr_stack] = 0;
                }
                else if (strcmp(token, "#IF_HAVE_PATROL_TIME") == 0)
                {
                    if (mec->patrol_time > 0.0F)
                        stack_active[curr_stack] = 1;
                    else
                        stack_active[curr_stack] = 0;
                }
                else if (strncmp(token, "#IF_PACKAGE_MISSION_EQ", 21) == 0)
                {
                    sptr = strchr(token, ' ');

                    if (sptr)
                        sptr++;

                    stack_active[curr_stack] = 0;

                    while (sptr and atoi(sptr))
                    {
                        if (atoi(sptr) == mec->package_mission)
                            stack_active[curr_stack] = 1;

                        sptr = strchr(sptr, ' ');

                        if (sptr)
                            sptr++;
                    }
                }
                else if (strncmp(token, "#IF_MISSION_EQ", 13) == 0)
                {
                    sptr = strchr(token, ' ');

                    if (sptr)
                        sptr++;

                    stack_active[curr_stack] = 0;

                    while (sptr and atoi(sptr))
                    {
                        if (atoi(sptr) == flight_data->mission)
                            stack_active[curr_stack] = 1;

                        sptr = strchr(sptr, ' ');

                        if (sptr)
                            sptr++;
                    }
                }
                else if (strncmp(token, "#IF_OLD_MISSION_EQ", 13) == 0)
                {
                    sptr = strchr(token, ' ');

                    if (sptr)
                        sptr++;

                    stack_active[curr_stack] = 0;

                    while (sptr and atoi(sptr))
                    {
                        if (atoi(sptr) == flight_data->old_mission)
                            stack_active[curr_stack] = 1;

                        sptr = strchr(sptr, ' ');

                        if (sptr)
                            sptr++;
                    }
                }
                else if (strncmp(token, "#IF_TARGET_OBJ", 14) == 0)
                {
                    if (target and target->GetClass() == CLASS_OBJECTIVE)
                        stack_active[curr_stack] = 1;
                    else
                        stack_active[curr_stack] = 0;
                }
                else if (strncmp(token, "#IF_TARGET_UNIT", 15) == 0)
                {
                    if (target and target->GetClass() == CLASS_UNIT)
                        stack_active[curr_stack] = 1;
                    else
                        stack_active[curr_stack] = 0;
                }
                else if (strncmp(token, "#IF_FLIGHT_CONTEXT_EQ", 21) == 0)
                {
                    sptr = strchr(token, ' ');

                    if (sptr)
                        sptr++;

                    stack_active[curr_stack] = 0;

                    while (sptr and (atoi(sptr) or *sptr == '0'))
                    {
                        if (atoi(sptr) == flight_data->mission_context)
                            stack_active[curr_stack] = 1;

                        sptr = strchr(sptr, ' ');

                        if (sptr)
                            sptr++;
                    }
                }
                else if (strncmp(token, "#IF_CONTEXT_EQ", 14) == 0)
                {
                    sptr = strchr(token, ' ');

                    if (sptr)
                        sptr++;

                    stack_active[curr_stack] = 0;

                    while (sptr and (atoi(sptr) or *sptr == '0'))
                    {
                        if (atoi(sptr) == mec->package_context)
                            stack_active[curr_stack] = 1;

                        sptr = strchr(sptr, ' ');

                        if (sptr)
                            sptr++;
                    }
                }
                else if (strcmp(token, "#IF_PRIMARY_FLIGHT") == 0)
                {
                    if (mec->package_element == flight_data)
                        stack_active[curr_stack] = 1;
                    else
                        stack_active[curr_stack] = 0;
                }
                else if (strcmp(token, "#IF_PLAYER_FLIGHT") == 0)
                {
                    if (mec->player_element == flight_data)
                        stack_active[curr_stack] = 1;
                    else
                        stack_active[curr_stack] = 0;
                }
                else if (strcmp(token, "#IF_PLAYER_PLANE") == 0)
                {
                    if (mec->player_element == flight_data and mec->player_pilot == mec->curr_pilot)
                        stack_active[curr_stack] = 1;
                    else
                        stack_active[curr_stack] = 0;
                }
                else if (strcmp(token, "#IF_SOLO_PACKAGE") == 0)
                {
                    if (mec->flight_data[1].camp_id > 0)
                        stack_active[curr_stack] = 0;
                    else
                        stack_active[curr_stack] = 1;
                }
                else if (strcmp(token, "#IF_ENEMY_AIR_RESPONSE") == 0)
                {
                    if (mec->responses bitand PRESPONSE_CA or ESquad)
                        stack_active[curr_stack] = 1;
                    else
                        stack_active[curr_stack] = 0;
                }
                else if (strncmp(token, "#IF_ENEMY_CA_ACTIVITY_GT", 24) == 0)
                {
                    int eaa;

                    for (i = 0, eaa = 0; i < NUM_TEAMS; i++)
                    {
                        if (TeamInfo[i] and TeamInfo[i]->atm and GetTTRelations(i, mec->team) == War)
                            eaa += TeamInfo[i]->atm->averageCAMissions;
                    }

                    sptr = strchr(token, ' ');

                    if (sptr)
                        sptr++;

                    if (eaa >= atoi(sptr))
                        stack_active[curr_stack] = 1;
                    else
                        stack_active[curr_stack] = 0;
                }
                /* else if (strncmp(token,"#IF_THREAT_LOC_EQ",17)==0)
                 {
                 if (sptr = strchr(token,' '))
                 sptr++;
                 stack_active[curr_stack] = 0;
                 while (sptr and (atoi(sptr) or *sptr == '0'))
                 {
                 if (atoi(sptr) == ((mec->threat_stats >> 8) bitand 0x0F))
                 stack_active[curr_stack] = 1;
                 if (sptr = strchr(sptr,' '))
                 sptr++;
                 }
                 }
                 else if (strncmp(token,"#IF_THREAT_TYPE_EQ",18)==0)
                 {
                 if (sptr = strchr(token,' '))
                 sptr++;
                 stack_active[curr_stack] = 0;
                 while (sptr and (atoi(sptr) or *sptr == '0'))
                 {
                 if (atoi(sptr) == ((mec->threat_stats >> 4) bitand 0x0F))
                 stack_active[curr_stack] = 1;
                 if (sptr = strchr(sptr,' '))
                 sptr++;
                 }
                 }
                */
                else if (strcmp(token, "#IF_HAVE_THREATS") == 0)
                {
                    if (mec->threat_ids[0])
                        stack_active[curr_stack] = 1;
                    else
                        stack_active[curr_stack] = 0;
                }
                else if (strcmp(token, "#IF_ALTERNATE_STRIP") == 0)
                {
                    if (mec->alternate_strip_id not_eq FalconNullId)
                        stack_active[curr_stack] = 1;
                    else
                        stack_active[curr_stack] = 0;
                }
                else if (strcmp(token, "#IF_HAVE_PILOT") == 0)
                {
                    if ( not mec->curr_pilot)
                        stack_active[curr_stack] = 0;
                    else
                        stack_active[curr_stack] = 1;
                }
                else if (strcmp(token, "#IF_HAVE_WEAPON") == 0)
                {
                    if ( not mec->curr_pilot)
                        stack_active[curr_stack] = 0;
                    else if (mec->curr_pilot->weapon_data[mec->curr_weapon].weapon_id)
                        stack_active[curr_stack] = 1;
                    else
                        stack_active[curr_stack] = 0;
                }
                else if (strcmp(token, "#IF_FIRST_WEAPON") == 0)
                {
                    if (mec->curr_weapon)
                        stack_active[curr_stack] = 0;
                    else
                        stack_active[curr_stack] = 1;
                }
                else if (strcmp(token, "#IF_WEAPONS_FIRED") == 0)
                {
                    stack_active[curr_stack] = 0;

                    for (i = 0; i < mec->curr_pilot->weapon_types; i++)
                    {
                        if (mec->curr_pilot->weapon_data[i].fired)
                            stack_active[curr_stack] = 1;
                    }
                }
                else if (strcmp(token, "#IF_FIRST_PILOT_SET") == 0)
                {
                    if (mec->curr_data)
                        stack_active[curr_stack] = 0;
                    else
                        stack_active[curr_stack] = 1;
                }
                else if (strcmp(token, "#IF_WEAPON_FIRED") == 0)
                {
                    if (mec->curr_pilot->weapon_data[mec->curr_weapon].fired)
                        stack_active[curr_stack] = 1;
                    else
                        stack_active[curr_stack] = 0;
                }
                else if (strncmp(token, "#IF_WEAPON_MISSED_EQ", 20) == 0)
                {
                    sptr = strchr(token, ' ');

                    if (sptr)
                        sptr++;

                    stack_active[curr_stack] = 0;

                    while (sptr and (atoi(sptr) or *sptr == '0'))
                    {
                        if (atoi(sptr) == mec->curr_pilot->weapon_data[mec->curr_weapon].missed)
                            stack_active[curr_stack] = 1;

                        sptr = strchr(sptr, ' ');

                        if (sptr)
                            sptr++;
                    }
                }
                else if (strncmp(token, "#IF_WEAPON_HIT_EQ", 17) == 0)
                {
                    sptr = strchr(token, ' ');

                    if (sptr)
                        sptr++;

                    stack_active[curr_stack] = 0;

                    while (sptr and (atoi(sptr) or *sptr == '0'))
                    {
                        if (atoi(sptr) == mec->curr_pilot->weapon_data[mec->curr_weapon].hit)
                            stack_active[curr_stack] = 1;

                        sptr = strchr(sptr, ' ');

                        if (sptr)
                            sptr++;
                    }
                }
                else if (strcmp(token, "#IF_THREAT_IS_MISSILE") == 0)
                {
                    int wid;
                    wid = GetBestVehicleWeapon(mec->threat_ids[mec->curr_data], DefaultDamageMods, LowAir, 0, &i);

                    if (Falcon4ClassTable[WeaponDataTable[wid].Index].vuClassData.classInfo_[VU_TYPE] == TYPE_MISSILE and Falcon4ClassTable[WeaponDataTable[wid].Index].vuClassData.classInfo_[VU_CLASS] == CLASS_VEHICLE)
                        stack_active[curr_stack] = 1;
                    else
                        stack_active[curr_stack] = 0;
                }
                else if (strcmp(token, "#IF_TARGET_VEH_IS_MISSILE") == 0)
                {
                    int wid;
                    stack_active[curr_stack] = 0;

                    if (ptarget and ptarget->IsUnit())
                    {
                        wid = GetBestVehicleWeapon(((Unit)ptarget)->GetVehicleID(0), DefaultDamageMods, LowAir, 0, &i);

                        if (Falcon4ClassTable[WeaponDataTable[wid].Index].vuClassData.classInfo_[VU_TYPE] == TYPE_MISSILE and Falcon4ClassTable[WeaponDataTable[wid].Index].vuClassData.classInfo_[VU_CLASS] == CLASS_VEHICLE)
                            stack_active[curr_stack] = 1;
                    }
                }
                else if (strcmp(token, "#IF_AWACS") == 0)
                {
                    Flight flight = (Flight) vuDatabase->Find(mec->awacs_id);

                    if (flight)
                        stack_active[curr_stack] = 1;
                    else
                        stack_active[curr_stack] = 0;
                }
                else if (strcmp(token, "#IF_JSTAR") == 0)
                {
                    Flight flight = (Flight) vuDatabase->Find(mec->jstar_id);

                    if (flight)
                        stack_active[curr_stack] = 1;
                    else
                        stack_active[curr_stack] = 0;
                }
                else if (strcmp(token, "#IF_TANKER") == 0)
                {
                    Flight flight = (Flight) vuDatabase->Find(mec->tanker_id);

                    if (flight)
                        stack_active[curr_stack] = 1;
                    else
                        stack_active[curr_stack] = 0;
                }
                else if (strcmp(token, "#IF_ECM") == 0)
                {
                    Flight flight = (Flight) vuDatabase->Find(mec->ecm_id);

                    if (flight)
                        stack_active[curr_stack] = 1;
                    else
                        stack_active[curr_stack] = 0;
                }

                else if (strncmp(token, "#IF_PACKAGE_SUCCESS_EQ", 22) == 0)
                {
                    sptr = strchr(token, ' ');

                    if (sptr)
                        sptr++;

                    stack_active[curr_stack] = 0;

                    while (sptr and (atoi(sptr) or *sptr == '0'))
                    {
                        if (atoi(sptr) == mec->pack_success)
                            stack_active[curr_stack] = 1;

                        sptr = strchr(sptr, ' ');

                        if (sptr)
                            sptr++;
                    }
                }
                else if (strcmp(token, "#IF_NOT_SUPPORT_HEADER") == 0)
                    stack_active[curr_stack] = not mec->curr_data;
                else if (strcmp(token, "#IF_PLAYER_PILOT") == 0)
                {
                    if (CPilotData)
                        stack_active[curr_stack] = 1;
                    else
                        stack_active[curr_stack] = 0;
                }
                else if (strcmp(token, "#IF_RELATED_EVENTS") == 0)
                {
                    if (mec->related_events[0])
                        stack_active[curr_stack] = 1;
                    else
                        stack_active[curr_stack] = 0;
                }
                else if (strcmp(token, "#IF_DIVERTED") == 0)
                {
                    if (flight_data->old_mission not_eq flight_data->mission)
                        stack_active[curr_stack] = 1;
                    else
                        stack_active[curr_stack] = 0;
                }
                else if (strcmp(token, "#IF_OFFENSIVE_PLANNED") == 0)
                {
                    if (TeamInfo[mec->team]->GetGroundAction()->actionType >= GACTION_MINOROFFENSIVE)
                        stack_active[curr_stack] = 1;
                    else
                        stack_active[curr_stack] = 0;
                }
                else if (strcmp(token, "#IF_DEFENSIVE_PLANNED") == 0)
                {
                    if (TeamInfo[mec->team]->GetGroundAction()->actionType == GACTION_DEFENSIVE and TheCampaign.CurrentTime + 30 * CampaignMinutes > TeamInfo[mec->team]->GetGroundAction()->actionTime)
                        stack_active[curr_stack] = 1;
                    else
                        stack_active[curr_stack] = 0;
                }
                else if (strcmp(token, "#IF_CLEAR_WEATHER") == 0)
                {
                    int cc = (((WeatherClass*)realWeather)->GetCloudCover(mec->tx, mec->ty) +
                                     ((WeatherClass*)realWeather)->GetCloudCover(mec->tx - 1, mec->ty) +
                                     ((WeatherClass*)realWeather)->GetCloudCover(mec->tx, mec->ty - 1) +
                                     ((WeatherClass*)realWeather)->GetCloudCover(mec->tx + 1, mec->ty) +
                                     ((WeatherClass*)realWeather)->GetCloudCover(mec->tx, mec->ty + 1)) / 5;

                    if (cc < 2)
                        stack_active[curr_stack] = 1;
                    else
                        stack_active[curr_stack] = 0;
                }
                else if (strcmp(token, "#IF_TACTICAL_ENGAGEMENT") == 0)
                {
                    if (FalconLocalGame and FalconLocalGame->GetGameType() == game_TacticalEngagement)
                        stack_active[curr_stack] = 1;
                    else
                        stack_active[curr_stack] = 0;
                }
                else
                    stack_active[curr_stack] = 0;

                continue;
            }

            // special tokens
            if (strcmp(token, "#EOL") == 0)
            {
                if (g_bBriefHTML and ( not win))
                    AddStringToBuffer("<p>", current_line);

                AddEOLToBrief(current_line, win, brief);
                continue;
            }
            else if (strcmp(token, "#SPACE") == 0)
            {
                if (g_bBriefHTML and ( not win))
                    AddStringToBuffer("&nbsp;", current_line);
                else
                    AddStringToBuffer(" ", current_line);

                continue;
            }
            else if (strcmp(token, "#COLON") == 0)
            {
                AddStringToBuffer(":", current_line);
                continue;
            }
            else if (strncmp(token, "#TAB", 4) == 0)
            {
                sptr = token + 4;
                i = atoi(sptr);

                if ((strncmp(filename, "FOrdWeap.db", 11) == 0) or (strncmp(filename, "FOrdEvt.db", 10) == 0))  //THW Kludge to remove tab tags from debrief
                    AddTabToDebrief(i, current_line, NULL, brief);
                else
                    AddTabToBrief(i, current_line, win, brief);

                continue;
            }
            else if (strncmp(token, "#COLOR", 6) == 0)
            {
                AddStringToBrief(current_line, win, brief);
                sptr = token + 6;
                i = atoi(sptr);

                if (win)
                {
                    switch (i)
                    {
                        case 1:
                            CBColor = win->ReverseText;
                            break;

                        case 2:
                            CBColor = win->DisabledText;
                            break;

                        default:
                            CBColor = win->NormalText;
                            break;
                    }
                }

                continue;
            }
            else if (strncmp(token, "#FONT", 5) == 0)
            {
                AddStringToBrief(current_line, win, brief);
                sptr = token + 5;
                i = atoi(sptr);

                if (win)
                    font = i;
            }
            else if (strncmp(token, "#ENDFONT", 8) == 0)
            {
                AddFontTextToBrief(current_line, font, win, brief);
                font = 0;
            }
            else if (strncmp(token, "#INC", 4) == 0)
            {
                sptr = token + 5;

                if (strcmp(sptr, "PILOT") == 0)
                {
                    int slot;

                    if (mec->curr_pilot)
                    {
                        slot = mec->curr_pilot->aircraft_slot + 1;
                        mec->curr_pilot = NULL;
                    }
                    else
                        slot = 0;

                    while ( not mec->curr_pilot and slot < PILOTS_PER_FLIGHT)
                    {
                        mec->curr_pilot = mec->FindPilotDataFromAC(flight_data, slot);
                        slot++;
                    }
                }

                if (strcmp(sptr, "WEAPON") == 0)
                    mec->curr_weapon++;

                if (strcmp(sptr, "DATA") == 0)
                    mec->curr_data++;
            }

            // Text string ids
            if (atoi(token) > 0)
                AddIndexedStringToBuffer(atoi(token), current_line);
            // Add all our script tokens here
            else if (strcmp(token, "FLIGHT_NUM") == 0)
                AddNumberToBuffer(flight_data->camp_id, current_line);
            else if (strcmp(token, "FLIGHT_NAME") == 0)
                AddStringToBuffer(flight_data->name, current_line);
            else if (strcmp(token, "PLANE_NAME") == 0)
            {
                AddStringToBuffer(flight_data->name, current_line);
                AddNumberToBuffer(mec->curr_pilot->aircraft_slot + 1, current_line);
            }
            else if (strcmp(token, "MISSION_NAME") == 0)
                AddIndexedStringToBuffer(300 + flight_data->mission, current_line);
            else if (strcmp(token, "OLD_MISSION_NAME") == 0)
                AddIndexedStringToBuffer(300 + flight_data->old_mission, current_line);
            else if (strncmp(token, "MISSION_DESCRIPTION", 19) == 0)
            {
                sptr = token + 19;
                i = sptr[0] - '1';
                sptr++;

                if (sptr[0] == ' ')
                    sptr++;

                if (sptr[i] == 'O')
                    AddIndexedStringToBuffer(400 + i * 50 + flight_data->old_mission, current_line);
                else
                    AddIndexedStringToBuffer(400 + i * 50 + flight_data->mission, current_line);
            }
            else if (strncmp(token, "OLD_MISSION_DESCRIPTION", 23) == 0)
            {
                sptr = token + 23;
                i = sptr[0] - '1';
                AddIndexedStringToBuffer(400 + i * 50 + flight_data->old_mission, current_line);
            }
            else if (strncmp(token, "PACKAGE_MISSION_DESCRIPTION", 27) == 0)
            {
                sptr = token + 27;
                i = sptr[0] - '1';
                AddIndexedStringToBuffer(900 + i * 50 + mec->package_mission, current_line);
            }
            else if (strncmp(token, "PACKAGE_TARGET_NAME", 19) == 0)
            {
                char name[81];
                sptr = token + 20;
                GetEntityName(ptarget, name, sptr[0], sptr[1]);
                AddStringToBuffer(name, current_line);
            }
            else if (strncmp(token, "OLD_TARGET_NAME", 11) == 0)
            {
                _TCHAR name[81];
                sptr = token + 12;
                GetEntityName(NULL, name, sptr[0], sptr[1]);
                AddStringToBuffer(name, current_line);
            }
            else if (strncmp(token, "TARGET_NAME", 11) == 0)
            {
                _TCHAR name[81];
                sptr = token + 12;
                GetEntityName(target, name, sptr[0], sptr[1]);
                AddStringToBuffer(name, current_line);
            }
            else if (strcmp(token, "TARGET_VEHICLE_NAME") == 0)
            {
                VehicleClassDataType *vc;

                if (ptarget and ptarget->IsUnit())
                {
                    Unit u = (UnitClass*) ptarget;

                    if (u->Father())
                        u = u->GetFirstUnitElement();

                    if (u)
                    {
                        vc = (VehicleClassDataType*) Falcon4ClassTable[u->GetVehicleID(0)].dataPtr;
                        AddStringToBuffer(vc->Name, current_line);
                    }
                }
            }
            else if (strcmp(token, "TARGET_OWNER") == 0)
            {
                // HACKED country adjective stuff (Used only for modification to "aircraft")
                if (ptarget)
                {
                    if (gLangIDNum == F4LANG_GERMAN)
                        AddIndexedStringToBuffer(ConvertTeamToStringIndex(ptarget->GetOwner(), F4LANG_FEMININE), current_line);
                    // AddIndexedStringToBuffer(3821 + 20*ptarget->GetOwner(), current_line);
                    else
                        AddIndexedStringToBuffer(ConvertTeamToStringIndex(ptarget->GetOwner()), current_line);

                    // AddIndexedStringToBuffer(3820 + 20*ptarget->GetOwner(), current_line);
                }
            }
            else if (strcmp(token, "PACKAGE_TARGET_BUILDING") == 0)
            {
                if (ptarget and ptarget->GetClass() == CLASS_OBJECTIVE and mec->package_element->target_building < FEATURES_PER_OBJ)
                {
                    FeatureClassDataType *fc;
                    fc = GetFeatureClassData(((Objective)ptarget)->GetFeatureID(mec->package_element->target_building));
                    AddStringToBuffer(fc->Name, current_line);
                }
            }
            else if (strcmp(token, "TARGET_BUILDING") == 0)
            {
                if (target and target->GetClass() == CLASS_OBJECTIVE and flight_data->target_building < FEATURES_PER_OBJ)
                {
                    FeatureClassDataType *fc;
                    fc = GetFeatureClassData(((Objective)target)->GetFeatureID(flight_data->target_building));
                    AddStringToBuffer(fc->Name, current_line);
                }
            }
            else if (strncmp(token, "REQUESTING_UNIT_NAME", 20) == 0)
            {
                ShiAssert(0);
                /* CampEntity reqe;
                 _TCHAR name[81];

                 sptr = token + 21;
                 reqe = FindEntity(flight_data->requester_id);
                 if (reqe and reqe->GetClass() == CLASS_UNIT)
                 {
                 GetEntityName(reqe, name, sptr[0], sptr[1]);
                 AddStringToBuffer(name, current_line);
                 }
                */
            }
            else if (strcmp(token, "REQUESTING_UNIT_DEST") == 0)
            {
                ShiAssert(0);
                /*
                 CampEntity reqe;
                 _TCHAR name[61];
                 Objective o;

                 reqe = FindEntity(flight_data->requester_id);
                 if (reqe and reqe->GetClass() == CLASS_UNIT)
                 {
                 if (reqe->GetDomain() == DOMAIN_LAND)
                 {
                 o = ((Unit)reqe)->GetUnitSecondaryObj();
                 if (o)
                 {
                 o->GetName(name,60,TRUE);
                 AddStringToBuffer(name, current_line);
                 }
                 else
                 AddIndexedStringToBuffer(253, current_line);
                 }
                 else if (reqe->GetDomain() == DOMAIN_AIR)
                 {
                 CampEntity etar;
                 // Find out where this flight is going (find the target)
                 WayPoint w = ((Unit)reqe)->GetFirstUnitWP();
                 while (w and not (w->GetWPFlags() bitand WPF_TARGET))
                 w = w->GetNextWP();
                 if (w)
                 {
                 etar = w->GetWPTarget();
                 etar->GetName(name,60,FALSE);
                 AddStringToBuffer(name, current_line);
                 }
                 }
                 }
                */
            }
            else if (strcmp(token, "REQUESTING_UNIT_VEHICLE") == 0)
            {
                ShiAssert(0);
                /*
                 CampEntity reqe;

                 reqe = FindEntity(flight_data->requester_id);
                 if (reqe and reqe->GetClass() == CLASS_UNIT)
                 {
                 if (reqe->GetDomain() == DOMAIN_AIR and reqe->GetType() == TYPE_PACKAGE)
                 reqe = ((Package)reqe)->GetFirstUnitElement();
                 ShiAssert (reqe);
                 VehicleClassDataType *vc = (VehicleClassDataType*) Falcon4ClassTable[((Unit)reqe)->GetVehicleID(0)].dataPtr;
                 if (gLangIDNum >= F4LANG_SPANISH)
                 {
                 AddStringToBuffer(vc->Name, current_line);
                 AddStringToBuffer(" ", current_line);
                 AddIndexedStringToBuffer(3820 + 20*reqe->GetOwner(), current_line);
                 }
                 else if (gLangIDNum == F4LANG_GERMAN)
                 {
                 AddIndexedStringToBuffer(3821 + 20*reqe->GetOwner(), current_line);
                 AddStringToBuffer(" ", current_line);
                 AddStringToBuffer(vc->Name, current_line);
                 }
                 else
                 {
                 AddIndexedStringToBuffer(3820 + 20*reqe->GetOwner(), current_line);
                 AddStringToBuffer(" ", current_line);
                 AddStringToBuffer(vc->Name, current_line);
                 }
                 }
                */
            }
            else if (strcmp(token, "INTERCEPTOR_NAME") == 0)
            {
                CampEntity ent;

                ent = FindEntity(mec->intercepting_ent);

                if ( not ent)
                    ent = ESquad; // Special case for fighters at airbases

                if (ent and ent->GetClass() == CLASS_UNIT)
                {
                    VehicleClassDataType *vc = (VehicleClassDataType*) Falcon4ClassTable[((Unit)ent)->GetVehicleID(0)].dataPtr;

                    if (gLangIDNum >= F4LANG_SPANISH)
                    {
                        AddStringToBuffer(vc->Name, current_line);
                        AddStringToBuffer(" ", current_line);
                        AddIndexedStringToBuffer(ConvertTeamToStringIndex(ent->GetOwner()), current_line);
                        // AddIndexedStringToBuffer(3820 + 20*ent->GetOwner(), current_line);
                    }
                    else if (gLangIDNum == F4LANG_GERMAN)
                    {
                        AddIndexedStringToBuffer(ConvertTeamToStringIndex(ent->GetOwner(), F4LANG_FEMININE), current_line);
                        // AddIndexedStringToBuffer(3821 + 20*ent->GetOwner(), current_line);
                        AddStringToBuffer(" ", current_line);
                        AddStringToBuffer(vc->Name, current_line);
                    }
                    else
                    {
                        AddIndexedStringToBuffer(ConvertTeamToStringIndex(ent->GetOwner()), current_line);
                        // AddIndexedStringToBuffer(3820 + 20*ent->GetOwner(), current_line);
                        AddStringToBuffer(" ", current_line);
                        AddStringToBuffer(vc->Name, current_line);
                    }
                }
            }
            else if (strcmp(token, "AWACS_NAME") == 0)
            {
                Flight awacs = (Flight) vuDatabase->Find(mec->awacs_id);
                _TCHAR name[128];

                if (awacs and not awacs->IsDead())
                {
                    VehicleClassDataType *vc = (VehicleClassDataType*) Falcon4ClassTable[awacs->GetVehicleID(0)].dataPtr;
                    GetCallsign(awacs->callsign_id, awacs->callsign_num, name);
                    AddStringToBuffer(name, current_line);
                    AddStringToBuffer(" ", current_line);
                    AddIndexedStringToBuffer(220, current_line);
                    AddTabToBrief(130, current_line, win, brief);
                    AddNumberToBuffer(awacs->GetTotalVehicles(), current_line);
                    AddStringToBuffer(" ", current_line);
                    AddStringToBuffer(vc->Name, current_line);
                }
            }
            else if (strcmp(token, "JSTAR_NAME") == 0)
            {
                Flight jstar = (Flight) vuDatabase->Find(mec->jstar_id);
                _TCHAR name[128];

                if (jstar and not jstar->IsDead())
                {
                    VehicleClassDataType *vc = (VehicleClassDataType*) Falcon4ClassTable[jstar->GetVehicleID(0)].dataPtr;
                    GetCallsign(jstar->callsign_id, jstar->callsign_num, name);
                    AddStringToBuffer(name, current_line);
                    AddStringToBuffer(" ", current_line);
                    AddIndexedStringToBuffer(221, current_line);
                    AddTabToBrief(130, current_line, win, brief);
                    AddNumberToBuffer(jstar->GetTotalVehicles(), current_line);
                    AddStringToBuffer(" ", current_line);
                    AddStringToBuffer(vc->Name, current_line);
                }
            }
            else if (strcmp(token, "TANKER_NAME") == 0)
            {
                Flight tanker = (Flight) vuDatabase->Find(mec->tanker_id);
                _TCHAR name[128];

                if (tanker and not tanker->IsDead())
                {
                    VehicleClassDataType *vc = (VehicleClassDataType*) Falcon4ClassTable[tanker->GetVehicleID(0)].dataPtr;
                    GetCallsign(tanker->callsign_id, tanker->callsign_num, name);
                    AddStringToBuffer(name, current_line);
                    AddStringToBuffer(" ", current_line);
                    AddIndexedStringToBuffer(222, current_line);
                    AddTabToBrief(130, current_line, win, brief);
                    AddNumberToBuffer(tanker->GetTotalVehicles(), current_line);
                    AddStringToBuffer(" ", current_line);
                    AddStringToBuffer(vc->Name, current_line);
                }
            }
            else if (strcmp(token, "ECM_NAME") == 0)
            {
                Flight ecm = (Flight) vuDatabase->Find(mec->ecm_id);
                _TCHAR name[128];

                if (ecm and not ecm->IsDead())
                {
                    VehicleClassDataType *vc = (VehicleClassDataType*) Falcon4ClassTable[ecm->GetVehicleID(0)].dataPtr;
                    GetCallsign(ecm->callsign_id, ecm->callsign_num, name);
                    AddStringToBuffer(name, current_line);
                    AddStringToBuffer(" ", current_line);
                    AddIndexedStringToBuffer(260, current_line);
                    AddTabToBrief(130, current_line, win, brief);
                    AddNumberToBuffer(ecm->GetTotalVehicles(), current_line);
                    AddStringToBuffer(" ", current_line);
                    AddStringToBuffer(vc->Name, current_line);
                }
            }
            else if (strncmp(token, "CONTEXT_STR", 11) == 0)
            {
                int strCount = 0, mission_context = 0;
                _TCHAR str[5][256] = { 0 }, context[1024];
                CampEntity tar, re;
                EventDataClass  data;
                FlightDataClass *fptr;

                // Point to beginning of argument list
                sptr = token + 12;

                // We need to find the target and requesting entity this situation is refering to.
                // Generally this will be the package's data, but if our first argument is an 'F'
                // it will be the flight's data.
                tar = ptarget;
                fptr = mec->package_element;
                re = FindEntity(mec->requesting_ent);
                mission_context = mec->package_context;

                // Check first argument and adjust data if necessary
                while (*sptr == ' ')
                    sptr++;

                if (*sptr == 'F')
                {
                    sptr++;
                    tar = target;
                    fptr = flight_data;
                    re = FindEntity(flight_data->requester_id);
                    mission_context = fptr->mission_context;
                }

                // Set up the genderable builder data
                data.formatId = 700 + mission_context;
                data.xLoc = mec->tx;
                data.yLoc = mec->ty;

                if (tar)
                {
                    data.vuIds[0] = tar->Id();
                    data.owners[0] = tar->GetOwner();
                }

                if (re)
                {
                    data.vuIds[1] = re->Id();
                    data.owners[1] = re->GetOwner();
                }

                // Parse any adjustment arguments, and collect substitution strings
                while (*sptr)
                {
                    switch (*sptr)
                    {
                        case '2':
                            data.formatId = 800 + mission_context;
                            break;

                        case 'R':
                            // Requesting Entity Destination/Vehicle
                            sptr++;

                            if (*sptr == 'D')
                                GetEntityDestination(re, str[strCount]);
                            else
                            {
                                if (re and re->IsPackage())
                                    re = ((Package)re)->GetFirstUnitElement();

                                if (re and re->IsUnit())
                                {
                                    VehicleClassDataType *vc = (VehicleClassDataType*) Falcon4ClassTable[((Unit)re)->GetVehicleID(0)].dataPtr;
                                    _tcscpy(str[strCount], vc->Name);
                                }
                                else
                                    AddIndexedStringToBuffer(256, str[strCount]);
                            }

                            strCount++;
                            break;

                        case 't':
                            // Time on target
                            AddTimeToBuffer(mec->assigned_tot, str[strCount]);
                            strCount++;
                            break;

                        case 'O':
                            sptr++;

                            if (*sptr == 'S') // Operation String
                            {
                                ReadIndexedString(854 + mec->action_type, str[strCount], 256);
                                strCount++;
                            }
                            else // Operation Objective
                            {
                                CampEntity ot = (CampEntity) vuDatabase->Find(TeamInfo[mec->team]->GetOffensiveAirAction()->actionObjective);
                                ot->GetName(str[strCount], 80, FALSE);
                                strCount++;
                            }

                            break;

                        default:
                            break;
                    }

                    sptr++;

                    if (*sptr == ' ')
                        sptr++;
                }

                // Build the sentance
                ConstructOrderedGenderedSentence(1024, context, &data, str[0], str[1], str[2], str[3], str[4]);
                AddStringToBuffer(context, current_line);
            }
            else if (strncmp(token, "RESULT_STR", 10) == 0)
            {
                int strCount = 0, success, losses, mission_context;
                _TCHAR str[5][256] = { 0 }, result[1024];
                CampEntity tar, re;
                EventDataClass  data;
                FlightDataClass *fptr;

                // Point to beginning of argument list
                sptr = token + 12;

                // We need to find the target and requesting entity this situation is refering to.
                // Generally this will be the package's data, but if our first argument is an 'F'
                // it will be the flight's data.
                tar = ptarget;
                fptr = mec->package_element;
                re = FindEntity(mec->requesting_ent);
                mission_context = mec->package_context;

                // Check first argument and adjust data if necessary
                while (*sptr == ' ')
                    sptr++;

                if (*sptr == 'F')
                {
                    sptr++;
                    tar = target;
                    fptr = flight_data;
                    re = FindEntity(flight_data->requester_id);
                    mission_context = fptr->mission_context;
                }

                // Set up the genderable builder data
                data.formatId = 1104;
                data.xLoc = mec->tx;
                data.yLoc = mec->ty;

                if (tar)
                {
                    data.vuIds[0] = tar->Id();
                    data.owners[0] = tar->GetOwner();
                }

                if (re)
                {
                    data.vuIds[1] = re->Id();
                    data.owners[1] = re->GetOwner();
                }

                // Determine success/losses
                success = fptr->mission_success;

                // 2002-02-13 MN added AWACSAbort
                if (success not_eq Incomplete /* and success not_eq AWACSAbort*/)
                {
                    // Determine losses, if necessary
                    losses = fptr->target_status;

                    if (tar and tar->IsObjective())
                        losses = 100 - ((Objective)tar)->GetObjectiveStatus();
                    else if (tar and tar->IsUnit())
                        losses = fptr->target_status - ((Unit)tar)->GetTotalVehicles();

                    // Parse the arguments
                    sptr = token + 11;

                    while (*sptr)
                    {
                        switch (*sptr)
                        {
                            case 'L':
                                // Losses
                                _stprintf(str[strCount], _T("%d"), losses);
                                strCount++;
                                break;

                            case 'S':

                                // % strength
                                if (tar and tar->IsObjective())
                                    _stprintf(str[strCount], _T("%d"), ((Objective)tar)->GetObjectiveStatus());
                                else if (tar and tar->IsUnit())
                                    _stprintf(str[strCount], _T("%d"), (((Unit)tar)->GetTotalVehicles() * 100) / ((Unit)tar)->GetFullstrengthVehicles());

                                strCount++;

                            case 'C':
                                // Critical Entity Name (in case it doesn't still exist)
                                _tcscpy(str[strCount], fptr->context_entity_name);
                                strCount++;
                                break;

                            default:
                                break;
                        }

                        sptr++;

                        if (*sptr == ' ')
                            sptr++;
                    }

                    // Now choose which format to use
                    if ( not tar or (tar->IsUnit() and (((Unit)tar)->Broken() or ((Unit)tar)->IsDead()))
                        or (tar->IsObjective() and ((Objective)tar)->GetObjectiveStatus() < 10))
                    {
                        // Target is completely broken or destroyed
                        if (success == Success or success == PartSuccess or success == AWACSAbort)
                            data.formatId = 1200 + mission_context;
                        else
                            data.formatId = 1400 + mission_context;
                    }
                    else if ((tar->IsUnit() and (float)losses / (float)fptr->target_status > 0.01F)
                             or (tar->IsObjective() and losses > 0))
                        // KCK: Needed to use this one until we have strings entered for
                        // posibility of partial destruction and successfull mission
                        // or (tar->IsObjective() and losses > 10))
                    {
                        // Target is partially broken or destroyed
                        if (success == Success or success == PartSuccess or success == AWACSAbort)
                            data.formatId = 1200 + mission_context;
                        else
                            data.formatId = 1400 + mission_context;
                    }
                    else
                    {
                        // Target took little or no damage
                        if (success == Success or success == PartSuccess or success == AWACSAbort)
                            data.formatId = 1300 + mission_context;
                        else
                            data.formatId = 1500 + mission_context;
                    }

                }

                /* else if (success == AWACSAbort)
                 {
                 // MNLOOK AWACSABort stuff, needs new strings.wch entry
                 }*/
                // Now build the string
                ConstructOrderedGenderedSentence(1024, result, &data, str[0], str[1], str[2], str[3], str[4]);
                AddStringToBuffer(result, current_line);
            }
            else if (strcmp(token, "TE_SUCCESS") == 0)
                AddIndexedStringToBuffer(1150 + determine_tactical_rating(), current_line);
            else if (strcmp(token, "NUM_AIRCRAFT") == 0)
            {
                i = flight_data->start_aircraft;
                AddIndexedStringToBuffer(i, current_line);
            }
            else if (strcmp(token, "AIRCRAFT_TYPE") == 0)
                AddStringToBuffer(flight_data->aircraft_name, current_line);
            else if (strcmp(token, "TIME_ON_TARGET") == 0 or strcmp(token, "TIME_ON_STATION_LABEL") == 0)
            {
                AddTimeToBuffer(mec->assigned_tot, current_line);
            }
            else if (strcmp(token, "ACTUAL_TIME_ON_TARGET") == 0)
            {
                int seconds_off;

                if (mec->actual_tot < 1.0F)
                    AddIndexedStringToBuffer(239, current_line);
                else
                {
                    AddTimeToBuffer(mec->actual_tot, current_line);
                    seconds_off = (int)(mec->actual_tot - mec->assigned_tot) / CampaignSeconds;

                    if (seconds_off > 10)
                    {
                        AddIndexedStringToBuffer(187, current_line);

                        if (seconds_off > 500)
                        {
                            AddNumberToBuffer(int(seconds_off / 60), current_line);
                            AddIndexedStringToBuffer(251, current_line);
                        }
                        else
                        {
                            AddNumberToBuffer(seconds_off, current_line);
                            AddIndexedStringToBuffer(240, current_line);
                        }

                        AddIndexedStringToBuffer(188, current_line);
                    }
                    else if (seconds_off < -10)
                    {
                        AddIndexedStringToBuffer(187, current_line);

                        if (seconds_off < -500)
                        {
                            AddNumberToBuffer((int)(seconds_off / -60), current_line);
                            AddIndexedStringToBuffer(252, current_line);
                        }
                        else
                        {
                            AddNumberToBuffer(-1 * seconds_off, current_line);
                            AddIndexedStringToBuffer(241, current_line);
                        }

                        AddIndexedStringToBuffer(188, current_line);
                    }
                }
            }
            else if (strcmp(token, "PATROL_TIME") == 0)
                AddTimeToBuffer(mec->patrol_time, current_line);
            else if (strcmp(token, "ALTERNATE_STRIP_NAME") == 0)
            {
                Objective o;
                _TCHAR name[80];
                o = FindObjective(mec->alternate_strip_id);

                if (o)
                {
                    o->GetName(name, 79, FALSE);
                    AddStringToBuffer(name, current_line);
                }
            }
            else if (strncmp(token, "GENERAL_LOCATION", 16) == 0 or strncmp(token, "SPECIFIC_LOCATION", 17) == 0 or strncmp(token, "NEAREST_LOCATION", 16) == 0 or strncmp(token, "THE_LOCATION", 12) == 0)
            {
                GridIndex x = 0, y = 0;

                sptr = token + 17;

                if (strncmp(token, "SPECIFIC_LOCATION", 17) == 0)
                    sptr++;

                if (strncmp(token, "THE_LOCATION", 12) == 0)
                    sptr -= 4;

                if (strcmp(sptr, "TARGET") == 0)
                {
                    x = mec->tx;
                    y = mec->ty;
                }

                if (strcmp(sptr, "AWACS") == 0)
                {
                    Flight flight = (Flight) vuDatabase->Find(mec->awacs_id);
                    Package pack = flight->GetUnitPackage();
                    pack->GetUnitDestination(&x, &y);
                }

                if (strcmp(sptr, "JSTAR") == 0)
                {
                    Flight flight = (Flight) vuDatabase->Find(mec->jstar_id);
                    Package pack = flight->GetUnitPackage();
                    pack->GetUnitDestination(&x, &y);
                }

                if (strcmp(sptr, "TANKER") == 0)
                {
                    Flight flight = (Flight) vuDatabase->Find(mec->tanker_id);
                    Package pack = flight->GetUnitPackage();
                    pack->GetUnitDestination(&x, &y);
                }

                if (strcmp(sptr, "ECM") == 0)
                {
                    Flight flight = (Flight) vuDatabase->Find(mec->ecm_id);
                    Package pack = flight->GetUnitPackage();
                    pack->GetUnitDestination(&x, &y);
                }

                if (strcmp(sptr, "ALT_AIRBASE") == 0)
                {
                    x = mec->abx;
                    y = mec->aby;
                }

                if (strcmp(sptr, "THREAT") == 0)
                {
                    x = mec->threat_x[mec->curr_data];
                    y = mec->threat_y[mec->curr_data];
                }

                AddLocationToBuffer(tolower(token[0]), x, y, current_line);
            }

            //
            // Mission evaluation tokens here
            //
            else if (strcmp(token, "PACKAGE_SUCCESS") == 0)
            {
                i = mec->package_element->mission_success;
                // 2002-02-16 MN modified, now from index 19 for the addition of AWACSAbort success condition
                // AddIndexedStringToBuffer(20+i, current_line);
                AddIndexedStringToBuffer(19 + i, current_line);
            }
            else if (strcmp(token, "FLIGHT_SUCCESS") == 0)
            {
                if (mec->player_element) // JB 010121
                    // 2002-02-16 MN modified, now from index 19 for the addition of AWACSAbort success condition
                    // AddIndexedStringToBuffer(20+mec->player_element->mission_success, current_line);
                    AddIndexedStringToBuffer(19 + mec->player_element->mission_success, current_line);
            }
            else if (strcmp(token, "PILOT_RATING") == 0)
            {
                if (mec->curr_pilot) // JB 010121
                    AddIndexedStringToBuffer(10 + mec->curr_pilot->rating, current_line);
            }
            else if (strcmp(token, "PILOT_NAME") == 0)
            {
                if (mec->curr_pilot) // JB 010121
                    AddStringToBuffer(mec->curr_pilot->pilot_name, current_line);
            }
            else if (strcmp(token, "PILOT_STATUS") == 0)
            {
                ShiAssert(mec->curr_pilot->pilot_status >= 0 and mec->curr_pilot->pilot_status <= 4);
                AddIndexedStringToBuffer(95 + mec->curr_pilot->pilot_status, current_line);
            }
            else if (strcmp(token, "AA_KILLS") == 0)
            {
                if (mec->curr_pilot) // JB 010121
                    AddNumberToBuffer(mec->curr_pilot->aa_kills, current_line);
            }
            else if (strcmp(token, "AG_KILLS") == 0)
            {
                if (mec->curr_pilot) // JB 010121
                    AddNumberToBuffer(mec->curr_pilot->ag_kills + mec->curr_pilot->as_kills + mec->curr_pilot->an_kills, current_line);
            }
            else if (strcmp(token, "AI_AA_KILLS") == 0)
            {
                AddNumberToBuffer(CPilotData->aa_kills, current_line);
            }
            else if (strcmp(token, "AI_AG_KILLS") == 0)
            {
                AddNumberToBuffer(CPilotData->ag_kills + CPilotData->as_kills + CPilotData->an_kills, current_line);
            }
            else if (strcmp(token, "FRIENDLY_LOSSES") == 0)
            {
                AddIndexedStringToBuffer(mec->friendly_losses, current_line);
            }
            else if (strcmp(token, "LONG_MISSION_SUCCESS") == 0)
            {
                _TCHAR tstring[80], wstring[80];

                // 2002-02-17 MN changed from 25+flight_data... to 4000+flight_data, new entries in strings.wch
                AddIndexedStringToBuffer(4000 + flight_data->mission_success, current_line);
                ReadIndexedString(1000 + flight_data->failure_code, wstring, 80);
                _stprintf(tstring, wstring, flight_data->failure_data);
                AddStringToBuffer(tstring, current_line);
            }
            else if (strcmp(token, "PLANE_STATUS") == 0)
            {
                if (mec->curr_pilot) // JB 010121
                    AddIndexedStringToBuffer(90 + mec->curr_pilot->aircraft_status, current_line);
            }
            else if (strcmp(token, "SHOW_THREATS") == 0)
            {
                for (i = 0; i < MAX_COLLECTED_THREATS; i++)
                {
                    if (mec->threat_ids[i])
                    {
                        mec->curr_data = i;
                        ReadScriptedBriefFile("threat.b", current_line, win, brief, mec, mec->package_element);
                    }
                }
            }
            else if (strcmp(token, "THREAT_VEHICLE_NAME") == 0)
            {
                VehicleClassDataType *vc = (VehicleClassDataType*) Falcon4ClassTable[mec->threat_ids[mec->curr_data]].dataPtr;
                AddStringToBuffer(vc->Name, current_line);
            }
            else if (strcmp(token, "WEAPON_LOAD") == 0)
            {
                if (mec->curr_pilot) // JB 010121
                    AddNumberToBuffer(mec->curr_pilot->weapon_data[mec->curr_weapon].starting_load, current_line);
            }
            else if (strcmp(token, "WEAPON_NAME") == 0)
            {
                if (mec->curr_pilot) // JB 010121
                    AddStringToBuffer(mec->curr_pilot->weapon_data[mec->curr_weapon].weapon_name, current_line);
            }
            else if (strcmp(token, "WEAPON_FIRED") == 0)
            {
                if (mec->curr_pilot) // JB 010121
                    AddNumberToBuffer(mec->curr_pilot->weapon_data[mec->curr_weapon].fired, current_line);
            }
            else if (strcmp(token, "WEAPON_HIT") == 0)
            {
                if (mec->curr_pilot) // JB 010121
                    AddNumberToBuffer(mec->curr_pilot->weapon_data[mec->curr_weapon].hit, current_line);
            }
            else if (strcmp(token, "WEAPON_MISSED") == 0)
            {
                if (mec->curr_pilot) // JB 010121
                    AddNumberToBuffer(mec->curr_pilot->weapon_data[mec->curr_weapon].missed, current_line);
            }
            else if (strcmp(token, "WEAPON_HIT_RATIO") == 0)
            {
                // JB 010121
                if (mec->curr_pilot) // JB 010121
                {
                    i = (100 * mec->curr_pilot->weapon_data[mec->curr_weapon].hit) / mec->curr_pilot->weapon_data[mec->curr_weapon].fired;
                    AddNumberToBuffer(i, current_line);
                }
            }
            else if (strcmp(token, "SHOW_EVENT") == 0)
            {
                _TCHAR *sptr;
                sptr = _tcschr(CTextPtr, '@');

                if (sptr)
                {
                    // replace the tab char with one of our tabs
                    sptr[0] = 0;
                    AddStringToBuffer(CTextPtr, current_line);
                    CTextPtr = sptr + 3; // Skip the tab values
                }
                else
                    AddStringToBuffer(CTextPtr, current_line);
            }
            else if (strcmp(token, "SHOW_RESULT") == 0)
                AddStringToBuffer(CTextPtr, current_line);

            //dpc - reversed wind heading to show correctly and consistent with DED
            // if (strcmp(token,"WIND_HEADING")==0)
            //   AddNumberToBuffer(FloatToInt32(((WeatherClass*)realWeather)->WindHeading*RTD),current_line);
            float reversedWindHeading = ((WeatherClass*)realWeather)->windHeading * RTD + 180.0F;

            if (reversedWindHeading > 360.0F) reversedWindHeading -= 360.0F;

            if (strcmp(token, "WIND_HEADING") == 0)
                AddNumberToBuffer(FloatToInt32(reversedWindHeading), current_line);
            //end fix
            else if (strcmp(token, "WIND_SPEED") == 0)
                //AddNumberToBuffer(FloatToInt32(((WeatherClass*)realWeather)->WindSpeed),current_line);
                //MI fix to show Knots/H instead of KM/H
                AddNumberToBuffer(FloatToInt32((((WeatherClass*)realWeather)->windSpeed + 0.5F) *
                                               KPH_TO_FPS * FTPSEC_TO_KNOTS), current_line);
            else if (strcmp(token, "TEMPERATURE") == 0)
                AddNumberToBuffer(FloatToInt32(((WeatherClass*)realWeather)->temperature), current_line);
            else if (strcmp(token, "CLOUD_TYPE") == 0)
                //JAM 17Nov03
            {
                char szTemp[256];

                if (realWeather->weatherCondition == SUNNY)
                    sprintf(szTemp, "Sunny ");
                else if (realWeather->weatherCondition == FAIR)
                    sprintf(szTemp, "Fair ");
                else if (realWeather->weatherCondition == POOR)
                    sprintf(szTemp, "Poor ");
                else if (realWeather->weatherCondition == INCLEMENT)
                    sprintf(szTemp, "Inclement ");

                _tcscat(current_line, szTemp);

                /* int cc = (((WeatherClass*)realWeather)->GetCloudCover(mec->tx,mec->ty) +
                 ((WeatherClass*)realWeather)->GetCloudCover(mec->tx-1,mec->ty) +
                 ((WeatherClass*)realWeather)->GetCloudCover(mec->tx,mec->ty-1) +
                 ((WeatherClass*)realWeather)->GetCloudCover(mec->tx+1,mec->ty) +
                 ((WeatherClass*)realWeather)->GetCloudCover(mec->tx,mec->ty+1))/5;
                 if (cc > 4)
                 AddIndexedStringToBuffer(603,current_line);
                 else if (cc > 2)
                 AddIndexedStringToBuffer(602,current_line);
                 else
                 AddIndexedStringToBuffer(600,current_line);
                */
            }

            else if (strcmp(token, "CLOUD_TYPETXT") == 0)
            {
                //THW 2003-12-07 Hack to display Cloud Type text and still have tab later
                AddStringToBuffer("Situation:", current_line);
            }

            else if (strcmp(token, "CLOUD_BASETXT") == 0)
            {
                //THW 2003-12-07 Hack to display Cloud Base text and still have tab later
                AddStringToBuffer("Cloud Base:", current_line);
            }

            else if (strcmp(token, "CLOUD_BASE") == 0)
            {
                //JAM 17Nov03
                //char szTemp[256];

                //sprintf(szTemp,"Clouds:           %d",-realWeather->stratusZ/1000);
                //_tcscat(current_line,szTemp);
                AddNumberToBuffer(FloatToInt32(-((WeatherClass*)realWeather)->stratusZ / 1000.0f + 0.5f), current_line);

            }
            else if (strcmp(token, "CON_LAYER") == 0)
                // AddNumberToBuffer(((WeatherClass*)realWeather)->contrailLow/1000.f,current_line); // Cobra - contrail not in 100's of feet anymore
                AddNumberToBuffer(FloatToInt32(-((WeatherClass*)realWeather)->stratus2Z / 1000.0f + 0.5f), current_line);
            else if (strcmp(token, "WAYPOINT_NUM") == 0)
                AddNumberToBuffer(mec->curr_data, current_line);
            else if (strcmp(token, "WAYPOINT_ACTION") == 0)
                GetWpActionToBuffer(CWayPoint, current_line);
            else if (strcmp(token, "WAYPOINT_TIME") == 0)
                GetWpTimeToBuffer(CWayPoint, current_line);
            else if (strcmp(token, "WAYPOINT_DISTANCE") == 0)
            {
                _TCHAR string[32] = {0};
                GetWptDist(CWayPoint, LWayPoint, string);
                AddRightJustifiedStringToBrief(string, 40, current_line, win, brief);
            }
            else if (strcmp(token, "WAYPOINT_HEADING") == 0)
            {
                _TCHAR string[32] = {0};
                GetWpHeading(CWayPoint, LWayPoint, string);

                AddRightJustifiedStringToBrief(string, 40, current_line, win, brief);
            }
            else if (strcmp(token, "WAYPOINT_SPEED") == 0)
            {
                _TCHAR string[32] = {0};
                GetWptSpeed(CWayPoint, LWayPoint, string);

                AddRightJustifiedStringToBrief(string, 40, current_line, win, brief);
            }
            else if (strcmp(token, "WAYPOINT_ALT") == 0)
            {
                _TCHAR string[32] = {0};
                int alt = GetWpAlt(CWayPoint, LWayPoint, string);

                if (alt > 0)
                {
                    AddRightJustifiedStringToBrief(string, 40, current_line, win, brief);

                    if (alt < MINIMUM_ASL_ALTITUDE)
                        AddIndexedStringToBuffer(1606, current_line);
                    else
                        AddIndexedStringToBuffer(1605, current_line);
                }
                else
                {
                    AddRightJustifiedStringToBrief(string, 40, current_line, win, brief);
                }
            }
            else if (strcmp(token, "WAYPOINT_CLIMB") == 0)
            {
                WayPoint nw = NULL;

                if (CWayPoint and CWayPoint->GetWPAction() not_eq WP_LAND and CWayPoint->GetWPAction() not_eq WP_REFUEL)
                    nw = CWayPoint->GetNextWP();

                if (CWayPoint and nw and nw->GetWPAltitude() not_eq CWayPoint->GetWPAltitude())
                {
                    if (CWayPoint->GetWPFlags() bitand WPF_HOLDCURRENT)
                    {
                        AddIndexedStringToBuffer(1600, current_line);
                        AddNumberToBuffer(CWayPoint->GetWPAltitude(), current_line);
                    }
                    else
                    {
                        if (nw->GetWPAltitude() < CWayPoint->GetWPAltitude())
                            AddIndexedStringToBuffer(1602, current_line);
                        else
                            AddIndexedStringToBuffer(1601, current_line);

                        AddNumberToBuffer(nw->GetWPAltitude(), current_line);
                    }
                }
                else
                    AddIndexedStringToBuffer(1650, current_line);
            }
            else if (strcmp(token, "WAYPOINT_DESC") == 0)
            {
                GetWpDescription(CWayPoint, current_line);
            }
            else if (strcmp(token, "ENEMY_SQUADRONS") == 0)
            {
                Unit u;
                GridIndex x, y;
                int got = 0;

                {
                    VuListIterator myit(AllAirList);
                    u = (Unit) myit.GetFirst();

                    while (u)
                    {
                        u->GetLocation(&x, &y);

                        if (x == mec->tx and y == mec->ty and u->GetType() == TYPE_SQUADRON)
                        {
                            CEntity = u;
                            ReadScriptedBriefFile("Squad.b", current_line, win, brief, mec, mec->package_element);

                            if (u->GetSType() == STYPE_UNIT_FIGHTER or u->GetSType() == STYPE_UNIT_FIGHTER_BOMBER)
                                ESquad = u;

                            got++;
                        }

                        u = (Unit) myit.GetNext();
                    }
                }

                if ( not got)
                {
                    ReadScriptedBriefFile("NoSquad.b", current_line, win, brief, mec, mec->package_element);
                }
            }
            else if (strncmp(token, "ENTITY_NAME", 11) == 0)
            {
                _TCHAR buffer[40];
                sptr = token + 12;
                GetEntityName(CEntity, buffer, sptr[0], sptr[1]);
                AddStringToBuffer(buffer, current_line);
            }
            else if (strcmp(token, "ENTITY_ELEMENT_NAME") == 0)
            {
                if (CEntity and CEntity->IsObjective())
                {
                    FeatureClassDataType *fc;
                    fc = (FeatureClassDataType*) Falcon4ClassTable[((Objective)CEntity)->GetFeatureID(mec->curr_data)].dataPtr;
                    AddStringToBuffer(fc->Name, current_line);
                }
                else if (CEntity and CEntity->IsUnit())
                {
                    VehicleClassDataType *vc;
                    UnitClassDataType *uc;
                    uc = ((Unit)CEntity)->GetUnitClassData();
                    vc = (VehicleClassDataType*) Falcon4ClassTable[uc->VehicleType[mec->curr_data]].dataPtr;
                    AddStringToBuffer(vc->Name, current_line);
                }
            }
            else if (strcmp(token, "ENTITY_OPERATIONAL") == 0)
            {
                int oper;

                if (CEntity->IsObjective())
                    AddNumberToBuffer(((Objective)CEntity)->GetObjectiveStatus(), current_line);
                else if (CEntity->IsUnit())
                {
                    oper = 100 * ((Unit)CEntity)->GetTotalVehicles() / ((Unit)CEntity)->GetFullstrengthVehicles();
                    AddNumberToBuffer(oper, current_line);
                }
            }
            else if (strcmp(token, "BEST_FEATURES") == 0)
            {
                int i, j, f, skip;
                _TCHAR names[MAX_TARGET_FEATURES][30];

                if ( not ptarget or not ptarget->IsObjective())
                    continue;

                for (i = 0; i < MAX_TARGET_FEATURES; i++)
                {
                    f = mec->package_element->target_features[i];

                    if (f < FEATURES_PER_OBJ)
                    {
                        // KCK: Don't show identical names (i.e. "Runway") twice.
                        FeatureClassDataType *fc;
                        fc = (FeatureClassDataType*) Falcon4ClassTable[((Objective)ptarget)->GetFeatureID(f)].dataPtr;

                        for (j = 0, skip = 0; j < i; j++)
                        {
                            if (_tcscmp(fc->Name, names[j]) == 0)
                                skip = 1;
                        }

                        _tcscpy(names[i], fc->Name);

                        if ( not skip)
                        {
                            CEntity = ptarget;
                            mec->curr_data = f;
                            ReadScriptedBriefFile("Feature.b", current_line, win, brief, mec, mec->package_element);
                        }
                    }
                }
            }
            else if (strcmp(token, "POTENTIAL_TARGETS") == 0)
            {
                Objective o;
                int i;

                for (i = 0; i < MAX_POTENTIAL_TARGETS; i++)
                {
                    o = FindObjective(mec->potential_targets[i]);

                    if (o)
                    {
                        CEntity = o;
                        ReadScriptedBriefFile("Objectiv.b", current_line, win, brief, mec, mec->package_element);
                    }
                }
            }
            else if (strncmp(token, "RELATED_EVENT", 13) == 0)
            {
                sptr = strchr(token, ' ');

                if (sptr)
                    sptr++;

                i = atoi(sptr) - 1;

                if (mec->related_events[i])
                {
                    AddStringToBuffer(mec->related_events[i], current_line);
                    AddEOLToBrief(current_line, win, brief);
                }
            }
            else if (strcmp(token, "SHOW_PLANNED_OFFENSIVE") == 0)
            {
                _TCHAR format[256], time[40] = {0}, objective[40], total[512];
                Objective o = (Objective) vuDatabase->Find(TeamInfo[mec->team]->GetGroundAction()->actionObjective);
                ReadIndexedString(890, format, 255);
                GetEntityName(o, objective, 'O', ' ');
                AddTimeToBuffer(TeamInfo[mec->team]->GetGroundAction()->actionTime, time);
                ConstructOrderedSentence(512, total, format, time, objective);
                AddStringToBuffer(total, current_line);
            }
            else if (strcmp(token, "SHOW_PLANNED_DEFENSIVE") == 0)
            {
                _TCHAR format[256], time[40] = {0}, objective[40], total[512];
                Objective o = (Objective) vuDatabase->Find(TeamInfo[mec->team]->GetGroundAction()->actionObjective);
                ReadIndexedString(891, format, 255);
                GetEntityName(o, objective, 'O', ' ');
                AddTimeToBuffer(TeamInfo[mec->team]->GetGroundAction()->actionTime, time);
                ConstructOrderedSentence(512, total, format, time, objective);
                AddStringToBuffer(total, current_line);
            }
            //THW 2003-12-07 HTML handler
            else if (strncmp(token, "<", 1) == 0)
            {
                //if ((g_bBriefHTML) and (filename not_eq ""))
                if ((g_bBriefHTML) and ( not win))
                    AddStringToBuffer(token, current_line);
            }

            // End active stack section
        }

        // End token handler
    }

    CloseCampFile(fp);
    return 1;
}

