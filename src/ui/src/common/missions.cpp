//
// Mission Building stuff for the UI
//
//
#include <windows.h>
#include "unit.h"
#include "campwp.h"
#include "campstr.h"
#include "squadron.h"
#include "division.h"
#include "flight.h"
#include "team.h"
#include "find.h"
#include "misseval.h"
#include "camplist.h"
#include "chandler.h"
#include "ui95_ext.h"
#include "cmap.h"
#include "uicomms.h"
#include "userids.h"
#include "textids.h"
#include "classtbl.h"
#include "ui_cmpgn.h"
#include "ACSelect.h"
#include "gps.h"
#include "urefresh.h"

void UpdateMissionWindow(long ID);
void MakeIndividualATO(VU_ID flightID);
void SetupFlightSpecificControls(Flight flt);
void SetSingle_Comms_Ctrls();
extern C_Handler *gMainHandler;

extern VU_ID gSelectedFlightID;
VU_ID gPlayerFlightID = FalconNullId;
VU_ID gCurrentFlightID = FalconNullId;
short gCurrentAircraftNum;
short gPlayerPlane = -1;
long StopLookingforMission = 0;
extern short InCleanup;
extern int gTimeModeServer;
extern bool g_bServer;
extern C_Map *gMapMgr;
extern GlobalPositioningSystem *gGps;

static short FlightStatusID[] = // 1 to 1 correspondence with enum list in ui_cmpgn.h
{
    TXT_BRIEFING,
    TXT_ENROUTE,
    TXT_INGRESS,
    TXT_PATROL,
    TXT_EGRESS,
    TXT_RETURNTOBASE,
    TXT_LANDING,
};

long GetFlightTime(Flight element)
{
    WayPoint wp;

    if ( not element)
        return(0);

    wp = element->GetCurrentUnitWP();

    if (wp)
        return(wp->GetWPDepartureTime());

    return(0);
}

short GetFlightStatusID(Flight element)
{
    WayPoint wp = NULL;
    int found = 0;
    short ID = 0;

    if ( not element)
        return(0);

    wp = element->GetCurrentUnitWP();

    if (wp)
    {
        if (wp->GetWPAction() == WP_TAKEOFF)
        {
            ID = _MIS_BRIEFING;
            found = 1;
        }
        else if (element->GetOverrideWP())
        {
            ID = _MIS_ENROUTE;
            found = 1;
        }
        else
        {
            found;
            ID = _MIS_RTB;

            while ((found == 0) and (wp))
            {
                if (wp->GetWPAction() == WP_ASSEMBLE)
                {
                    ID = _MIS_ENROUTE;
                    found = 1;
                }
                else if (wp->GetWPAction() == WP_POSTASSEMBLE)
                {
                    ID = _MIS_EGRESS;
                    found = 1;
                }
                else if (wp->GetWPFlags() bitand WPF_TARGET)
                {
                    if
                    (
                        element->GetUnitMission() == AMIS_BARCAP or
                        element->GetUnitMission() == AMIS_BARCAP2 or
                        element->GetUnitMission() == AMIS_HAVCAP or
                        element->GetUnitMission() == AMIS_RESCAP or
                        element->GetUnitMission() == AMIS_TARCAP or
                        element->GetUnitMission() == AMIS_AMBUSHCAP
                    )
                    {
                        ID = _MIS_PATROL;
                    }
                    else
                    {
                        ID = _MIS_INGRESS;
                    }

                    found = 1;
                }
                else if (wp->GetWPAction() == WP_LAND)
                {
                    ID = _MIS_LAND;
                    found = 0;
                }

                if (wp->GetWPAction() == WP_LAND)
                {
                    wp = NULL;
                }
                else
                {
                    wp = wp->GetNextWP();
                }
            }
        }

        if ((TheCampaign.Flags bitand CAMP_TACTICAL) and ( not found))
        {
            ID = _MIS_ENROUTE;
        }
    }
    else
    {
        ID = _MIS_RTB;
    }

    return(ID);
}

void CheckCampaignFlyButton()
{
    C_Window *win;
    C_Button *btn;
    Flight flt;
    short plane;
    short Enabled = 0;

    flt = FalconLocalSession->GetPlayerFlight();
    plane = FalconLocalSession->GetPilotSlot();

    if ((gCommsMgr) and (gCommsMgr->Online()))
    {
        // Don't care about restricting access when online
        if (flt and plane not_eq 255)
            Enabled = 1;
    }
    else
    {
        // OW - sylvains checkfly fix
#if 1
        if (flt and plane not_eq 255 and GetFlightStatusID(flt) < _MIS_EGRESS)
            // ADDED BY S.G. SO UNINITIALIZED FLIGHTS CAN'T TAKE OFF
        {
            int i, dontEnable = 0;

            if (GetFlightStatusID(flt) > _MIS_BRIEFING)
            {
                for (i = 0; i < 4 and dontEnable == 0; i++)
                {
                    if (FalconLocalSession->GetPlayerFlight()->plane_stats[i] == AIRCRAFT_AVAILABLE and FalconLocalSession->GetPlayerFlight()->pilots[i] == NO_PILOT)
                        dontEnable = 1;
                }
            }

            if ( not dontEnable)
                // END OF ADDED SECTION
                Enabled = 1;
        }

#else

        if (flt and plane not_eq 255 and GetFlightStatusID(flt) < _MIS_EGRESS)
            Enabled = 1;

#endif
    }

    win = gMainHandler->FindWindow(CP_TOOLBAR);

    if (win)
    {
        btn = (C_Button *)win->FindControl(SINGLE_FLY_CTRL);

        if (btn)
        {
            if (Enabled)
                btn->SetFlagBitOn(C_BIT_ENABLED);
            else
                btn->SetFlagBitOff(C_BIT_ENABLED);

            btn->Refresh();
        }

        btn = (C_Button *)win->FindControl(COMMS_FLY_CTRL);

        if (btn)
        {
            if (Enabled)
                btn->SetFlagBitOn(C_BIT_ENABLED);
            else
                btn->SetFlagBitOff(C_BIT_ENABLED);

            btn->Refresh();
        }
    }

    win = gMainHandler->FindWindow(TAC_TOOLBAR_WIN);

    if (win)
    {
        btn = (C_Button *)win->FindControl(SINGLE_FLY_CTRL);

        if (btn)
        {
            if (Enabled)
                btn->SetFlagBitOn(C_BIT_ENABLED);
            else
                btn->SetFlagBitOff(C_BIT_ENABLED);

            btn->Refresh();
        }

        btn = (C_Button *)win->FindControl(COMMS_FLY_CTRL);

        if (btn)
        {
            if (Enabled)
                btn->SetFlagBitOn(C_BIT_ENABLED);
            else
                btn->SetFlagBitOff(C_BIT_ENABLED);

            btn->Refresh();
        }
    }
}

static void MissionSelectCB(long, short hittype, C_Base *control)
{
    C_Window *win;
    Flight flight;
    F4CSECTIONHANDLE *Leave;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    Leave = UI_Enter(control->Parent_);
    win = control->Parent_;

    if (win)
    {
        gCurrentFlightID = ((C_Mission*)control)->GetVUID();

        gSelectedFlightID = gCurrentFlightID;
        flight = (Flight)vuDatabase->Find(gCurrentFlightID);

        if (flight and not flight->IsDead())
        {
            TheCampaign.MissionEvaluator->PreMissionEval(flight, 255);
            UpdateMissionWindow(win->GetID());
            MakeIndividualATO(gCurrentFlightID);

            if (gMapMgr)
            {
                gMapMgr->SetCurrentWaypointList(gCurrentFlightID);
                SetupFlightSpecificControls(flight);
                gMapMgr->FitFlightPlan();
                gMapMgr->DrawMap();
            }
        }
    }

    control->SetState(static_cast<short>(control->GetState() bitor 1));
    control->Refresh();
    CheckCampaignFlyButton();
    UI_Leave(Leave);
}

static void SelectMission(C_Base *control)
{
    C_Window *win;
    Flight flight;
    F4CSECTIONHANDLE *Leave;

    Leave = UI_Enter(control->Parent_);
    win = control->Parent_;

    if (win)
    {
        gCurrentFlightID = ((C_Mission*)control)->GetVUID();

        gSelectedFlightID = gCurrentFlightID;
        flight = (Flight)vuDatabase->Find(gCurrentFlightID);

        if (flight and not flight->IsDead())
        {
            TheCampaign.MissionEvaluator->PreMissionEval(flight, 255);
            UpdateMissionWindow(win->GetID());
            MakeIndividualATO(gCurrentFlightID);
        }
    }

    control->SetState(static_cast<short>(control->GetState() bitor 1));
    control->Refresh();
    CheckCampaignFlyButton();
    UI_Leave(Leave);
}

void FindMissionInBriefing(long ID)
{
    C_Window *win;
    C_TreeList *tree;
    TREELIST *cur;
    Flight flight;

    win = gMainHandler->FindWindow(ID);

    if (win)
    {
        tree = (C_TreeList *)win->FindControl(MISSION_LIST_TREE);

        if (tree)
        {
            cur = tree->GetRoot();

            while (cur)
            {
                if (cur->Item_ and not (cur->Item_->GetFlags() bitand C_BIT_INVISIBLE))
                {
                    if (((C_Mission*)cur->Item_)->GetStatusID() < _MIS_EGRESS)
                    {
                        flight = (Flight)vuDatabase->Find(((C_Mission*)cur->Item_)->GetVUID());

                        if (flight)
                        {
                            if (flight->GetTotalVehicles())
                            {
                                cur->Item_->SetState(1);

                                if ( not StopLookingforMission)
                                {
                                    MissionSelectCB(cur->ID_, C_TYPE_LMOUSEUP, cur->Item_);

                                    // KLUDGE: Throw player in 1st slot
                                    // KCK: This should be done regardless of online status
                                    // if( not gCommsMgr->Online())
                                    // { // Throw player in 1st slot
                                    RequestACSlot(flight, 0, static_cast<uchar>(flight->GetAdjustedAircraftSlot(0)), 0, 0, 1);
                                    // }
                                    StopLookingforMission = 1;
                                }
                                else
                                    SelectMission(cur->Item_);

                                return;
                            }
                        }
                    }
                }

                cur = cur->Next;
            }
        }
    }
}

UI_Refresher *FindMissionItem(Flight flight)
{
    UI_Refresher *urec;

    urec = (UI_Refresher*)gGps->Find(flight->GetCampID());

    if (urec and urec->Mission_)
    {
        MissionSelectCB(urec->Mission_->GetID(), C_TYPE_LMOUSEUP, urec->Mission_);
        return urec;
    }

    return NULL;
}

C_Mission *MakeMissionItem(C_TreeList *tree, Flight element)
{
    C_Mission *mission;
    C_Window *win;
    _TCHAR buffer[200];
    TREELIST *item;
    int ent_mission;
    Package package;
    WayPoint wp;

    if ( not element->Final())
        return(NULL);

    if (TheCampaign.Flags bitand CAMP_TACTICAL)
    {
        if (element->GetOwner() not_eq FalconLocalSession->GetTeam())
        {
            return NULL;
        }
    }
    else
    {
        if (element->GetUnitSquadronID() not_eq FalconLocalSession->GetPlayerSquadronID())
            return(NULL);
    }

    if ( not tree or not element or not element->GetUnitParent())
        return(NULL);

    // Create new record
    mission = new C_Mission;

    if ( not mission)
        return(NULL);

    win = tree->GetParent();

    if ( not win)
        return(NULL);

    mission->Setup(element->GetCampID(), 0); // ID=element->CampID;
    mission->SetFont(win->Font_);
    mission->SetClient(tree->GetClient());
    mission->SetW(win->ClientArea_[tree->GetClient()].right - win->ClientArea_[tree->GetClient()].left);
    mission->SetH(gFontList->GetHeight(tree->GetFont()));
    // Set takeoff time string
    wp = element->GetFirstUnitWP();

    if (wp)   // JPO CTD fix
    {
        GetTimeString(wp->GetWPDepartureTime(), buffer);
        mission->SetTakeOff(static_cast<short>(tree->GetUserNumber(C_STATE_0)), 0, buffer);
        mission->SetTakeOffTime(wp->GetWPDepartureTime());
    }

    // Set Mission Type
    ent_mission = element->GetUnitMission();

    if (ent_mission == AMIS_ALERT)
        ent_mission = AMIS_INTERCEPT;

    mission->SetMission(static_cast<short>(tree->GetUserNumber(C_STATE_1)), 0, MissStr[ent_mission]);
    mission->SetMissionID(static_cast<short>(element->GetUnitMission()));

    // Set Package (campID of package)
    _stprintf(buffer, "%1d", element->GetUnitParent()->GetCampID());
    mission->SetPackage(static_cast<short>(tree->GetUserNumber(C_STATE_2)), 0, buffer);
    mission->SetPackageID(element->GetUnitParent()->GetCampID());

    // Set Mission Status String
    mission->SetStatusID(GetFlightStatusID(element));
    mission->SetStatus(static_cast<short>(tree->GetUserNumber(C_STATE_3)), 0, gStringMgr->GetString(FlightStatusID[mission->GetStatusID()]));

    // Set Mission Priority String
    package = element->GetUnitPackage();
    buffer[1] = 0;
    // KCK: It seemed more logical to get the priority from the flight, not the package's request
    // PJW: Why Kevin you are sooo fucking wrong... and ASK when making changes butt fuck
    // buffer[0]=(255 - element->GetUnitPriority()) / 51 + _T('A');
    buffer[0] = static_cast<char>((255 - package->GetMissionRequest()->priority) / 51 + _T('A'));
    mission->SetPriority(static_cast<short>(tree->GetUserNumber(C_STATE_4)), 0, buffer);
    mission->SetPriorityID(static_cast<short>(255 - package->GetMissionRequest()->priority));

    // Set a callback incase someone actually wants to see this mission
    // if (TheCampaign.Flags bitand CAMP_TACTICAL)
    // {
    // mission->SetCallback (TacMissionSelectCB);
    // }
    // else
    // {
    mission->SetCallback(MissionSelectCB);
    // }

    // Tack on the VU_ID
    mission->SetVUID(element->Id());
    mission->SetUserNumber(C_STATE_0, element->GetTeam());
    mission->SetUserNumber(C_STATE_1, wp ? wp->GetWPDepartureTime() : 0);
    mission->SetUserNumber(C_STATE_2, 1000 - element->GetUnitPriority()); // Priority
    mission->SetUserNumber(C_STATE_3, element->GetUnitMission());

    if ( not element->Final() or element->GetUnitMission() == AMIS_ALERT)
    {
        mission->SetFlagBitOn(C_BIT_INVISIBLE);
    }

    // Add to tree
    item = tree->CreateItem(element->GetCampID(), C_TYPE_ITEM, mission);
    mission->SetOwner(item);

    if (tree->AddItem(tree->GetRoot(), item))
        return(mission);

    mission->Cleanup();
    delete mission;
    delete item;
    return(NULL);
}

void MissionUpdateStatus(Flight element, C_Mission *mission)
{
    mission->Refresh();
    mission->SetStatusID(GetFlightStatusID(element));
    mission->SetStatus(gStringMgr->GetString(FlightStatusID[mission->GetStatusID()]));

    if (gPlayerFlightID == element->Id())
    {
        if (mission->GetStatusID() >= _MIS_EGRESS)
        {
            mission->SetState(static_cast<short>(mission->GetState() bitand 1));
            FalconLocalSession->SetPlayerFlight(NULL);
            FalconLocalSession->SetPilotSlot(255);
            gPlayerFlightID = FalconNullId;
            gPlayerPlane = -1;
        }
    }

    mission->Refresh();

    if (gCurrentFlightID == element->Id())
    {
        UpdateMissionWindow(CB_MISSION_SCREEN);
        UpdateMissionWindow(TAC_AIRCRAFT);
    }

    if (gMapMgr)
        gMapMgr->UpdateWaypoint(element);
}

void MissionUpdateTime(Flight element, C_Mission *mission)
{
    _TCHAR buffer[80];

    mission->Refresh();
    WayPoint wp = element->GetFirstUnitWP();

    if (wp)   // JPO CTD fix
    {
        GetTimeString(wp->GetWPDepartureTime(), buffer);
        mission->SetTakeOff(buffer);
        mission->SetTakeOffTime(wp->GetWPDepartureTime());
    }
    else
        mission->SetTakeOff(_T("Unknown"));

    mission->Refresh();
}

void RemoveMissionCB(TREELIST *item)
{
    C_Mission *mis;

    if ( not item)
        return;

    mis = (C_Mission*)item->Item_;

    if (gPlayerFlightID == mis->GetVUID())
    {
        gPlayerFlightID = FalconNullId;
        gPlayerPlane = -1;
    }

    if (gMapMgr)
        gMapMgr->RemoveWaypoints(static_cast<short>(item->Item_->GetUserNumber(0)), item->Item_->GetID() << 8);

    if (InCleanup)
        return;

    if (gCurrentFlightID == mis->GetVUID()) // our mission is being removed... find the next mission in briefing
    {
        if (TheCampaign.Flags bitand CAMP_TACTICAL)
        {
            if (gTimeModeServer or g_bServer)
            {
                FindMissionInBriefing(TAC_AIRCRAFT);
            }

            UpdateMissionWindow(TAC_AIRCRAFT);
        }
        else
        {
            if (gTimeModeServer or g_bServer)
            {
                FindMissionInBriefing(CB_MISSION_SCREEN);
                UpdateMissionWindow(CB_MISSION_SCREEN);
            }
        }
    }
}

