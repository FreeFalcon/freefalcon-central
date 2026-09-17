// Campaign Menus

#include <windows.h>
#include "graphics/include/matrix.h"
#include "graphics/include/drawbsp.h"
#include "graphics/include/loader.h"
#include "entity.h"
#include "feature.h"
#include "vehicle.h"
#include "chandler.h"
#include "ui95_ext.h"
#include "find.h"
#include "cmpclass.h"
#include "division.h"
#include "cmap.h"
#include "cbsplist.h"
#include "c3dview.h"
#include "userids.h"
#include "filters.h"
#include "gps.h"
#include "urefresh.h"
#include "campstr.h"
// Artscout - 2026: for the campaign package builder below -- squadrons, flights, packages
// and the mission request they are filed with.
#include "squadron.h"
#include "flight.h"
#include "package.h"
#include "mission.h"
#include "unit.h"
#include "campaign.h"
#include "camplist.h"
#include "campwp.h"
#include "classtbl.h"
#include "textids.h"
#include "fflog.h" // Artscout - 2026: "Build package" submenu trace
#include "falcsess.h" // Artscout - 2026: FalconLocalSession, for the player's team

void DeleteGroupList(long ID);
void AddObjectiveToTargetTree(Objective obj);
void SetBullsEye(C_Window *);
void SetSlantRange(C_Window *);
void SetHeading(C_Window *);
void PositionCamera(OBJECTINFO *Info, C_Window *win, long client);
void SetupUnitInfoWindow(VU_ID Id);
void SetupDivisionInfoWindow(long DivID, short owner);
void GetObjectivesNear(float x, float y, float range);
void GetGroundUnitsNear(float x, float y, float range);
void ReconArea(float x, float y, float range);
void BuildTargetList(float x, float y, float range);
void BuildSpecificTargetList(VU_ID targetID);
void set_waypoint_action(WayPoint wp, int action);
void refresh_waypoint(WayPointClass *wp);
void tactical_add_victory_condition(VU_ID id, C_Base *caller);
void tactical_add_squadron(VU_ID id);
void tactical_add_flight(VU_ID ID, C_Base *caller);
void tactical_add_package(VU_ID ID, C_Base *caller);
void tactical_add_battalion(VU_ID ID, C_Base *control);
void recalculate_waypoints(WayPointClass *wp);
void tactical_edit_package(VU_ID id, C_Base *caller);
void fixup_unit(Unit unit);
void RefreshMapOnChange(void); // Artscout - 2026: campaign package builder
void SetupSquadronInfoWindow(VU_ID TheID);
void CloseAllRenderers(long openID);

C_TreeList *TargetTree = NULL;

extern C_Handler *gMainHandler;
extern C_Map *gMapMgr;
extern C_3dViewer *gUIViewer;
extern OBJECTINFO Recon;
extern GlobalPositioningSystem *gGps;
extern VU_ID gActiveFlightID, gCurrentFlightID;

extern int g_nUnidentifiedInUI; // 2002-02-24 S.G.
extern int gShowUnknown; // 2002-02-21 S.G.
#define MID_UNITS_SQUAD_UNKNOWN                                                \
    86051 // 2002-02-21 S.G. Until I add it to userids.h
extern GlobalPositioningSystem *gGps; // 2002-02-21 S.G.

// Used for enabling bitand disabling menus based on TE/Camp/Edit modes
static long GameType;
static long EditMode;

// this block Retro 26/10/03
// I save the state of every filter in this array and preinitialize the filters when entering the map screen again
// the state is saved in the 'toggle' routine for objectives, units, labels, bullseye and threats
// everything here is initialized to 'OFF' so that on first entering nothing appears
// this stuff has to be outside the campaign-map-screen scope so that it isn�t destroyed on entering the 3d.. so I made it global..
namespace FilterSaveStuff
{

enum
{
    // Legend stuff
    LE_BULLSEYE = 0,
    LE_LABELS,
    // Objectives
    OBJ_AIRFIELDS,
    OBJ_AIRDEFENSE,
    OBJ_ARMY,
    OBJ_CCC,
    OBJ_POLITICAL,
    OBJ_INFRA,
    OBJ_LOGISTICS,
    OBJ_WARPRODUCTION,
    OBJ_NAVIGATION,
    OBJ_OTHER,
    OBJ_NAVAL,
    OBJ_VICTORYCOND,
    // Units
    UNITS_DIV,
    UNITS_BRIG,
    UNITS_BAT,
    UNITS_COMBAT,
    UNITS_AIR_DEFENSE,
    UNITS_SUPPORT,
    UNITS_SQUAD_SQUADRON,
    UNITS_SQUAD_PACKAGE,
    UNITS_SQUAD_FIGHTER,
    // UNITS_SQUAD_FIGHTBOMB, // no idea what that is, isn�t used either..
    UNITS_SQUAD_ATTACK,
    UNITS_SQUAD_BOMBER,
    UNITS_SQUAD_SUPPORT,
    UNITS_HELICOPTER,
    UNITS_SQUAD_UNKNOWN,
    UNITS_NAVY_COMBAT,
    UNITS_NAVY_SUPPORT,
    // Sams/Radar
    CIRCLE_SAM_LOW,
    CIRCLE_SAM_HIGH,
    CIRCLE_RADAR_LOW,
    CIRCLE_RADAR_HIGH,
    END_OF_ENUM__USED_FOR_SIZE = CIRCLE_RADAR_HIGH + 1
};

// those can be preinitialized if the need arises..
bool filterState[END_OF_ENUM__USED_FOR_SIZE] = // Legend stuff
    {
        false, false,
        // Objectives
        true, false, false, false, false, false, false, false, false, false,
        false, false,
        // Units
        false, false,
        false, // this is a radiobutton, only 1 of them may be TRUE (all ot FALSE is ok)
        true, false, false, false, false, true, false, false, false, false,
        true, false, false,
        // Sams/Radar
        false, false, false,
        false // this is a radiobutton, only 1 of them may be TRUE (all ot FALSE is ok)
}; // ..ugly, but whatever..

// Artscout - 2026: which campaign overlay was up, as a C_Map::CAMP_OVERLAY_* value. Not a
// flag in the array above because these four are one radio group, not four checkboxes --
// see ShowCampaignOverlay for why only one of them can hold the map's palette.
long campLayer = 0;
} // namespace FilterSaveStuff, end Retro 26/10/03

void MenuToggleObjectiveCB(long ID, short, C_Base *control)
{
    using namespace FilterSaveStuff; // Retro 26/10/03. Here I take note if a certain objective filter is enabled or disabled

    switch (ID)
    {
    case MID_INST_AF:
        if (((C_PopupList *)control)->GetItemState(ID))
        {
            gMapMgr->ShowObjectiveType(_OBTV_AIR_FIELDS);
            filterState[OBJ_AIRFIELDS] = true;
        }
        else
        {
            gMapMgr->HideObjectiveType(_OBTV_AIR_FIELDS);
            filterState[OBJ_AIRFIELDS] = false;
        }

        break;

    case MID_INST_AD:
        if (((C_PopupList *)control)->GetItemState(ID))
        {
            gMapMgr->ShowObjectiveType(_OBTV_AIR_DEFENSE);
            filterState[OBJ_AIRDEFENSE] = true;
        }
        else
        {
            gMapMgr->HideObjectiveType(_OBTV_AIR_DEFENSE);
            filterState[OBJ_AIRDEFENSE] = false;
        }

        break;

    case MID_INST_ARMY:
        if (((C_PopupList *)control)->GetItemState(ID))
        {
            gMapMgr->ShowObjectiveType(_OBTV_ARMY);
            filterState[OBJ_ARMY] = true;
        }
        else
        {
            gMapMgr->HideObjectiveType(_OBTV_ARMY);
            filterState[OBJ_ARMY] = false;
        }

        break;

    case MID_INST_CCC:
        if (((C_PopupList *)control)->GetItemState(ID))
        {
            gMapMgr->ShowObjectiveType(_OBTV_CCC);
            filterState[OBJ_CCC] = true;
        }
        else
        {
            gMapMgr->HideObjectiveType(_OBTV_CCC);
            filterState[OBJ_CCC] = false;
        }

        break;

    case MID_INST_POLITICAL:
        if (((C_PopupList *)control)->GetItemState(ID))
        {
            gMapMgr->ShowObjectiveType(_OBTV_POLITICAL);
            filterState[OBJ_POLITICAL] = true;
        }
        else
        {
            gMapMgr->HideObjectiveType(_OBTV_POLITICAL);
            filterState[OBJ_POLITICAL] = false;
        }

        break;

    case MID_INST_INFRA:
        if (((C_PopupList *)control)->GetItemState(ID))
        {
            gMapMgr->ShowObjectiveType(_OBTV_INFRASTRUCTURE);
            filterState[OBJ_INFRA] = true;
        }
        else
        {
            gMapMgr->HideObjectiveType(_OBTV_INFRASTRUCTURE);
            filterState[OBJ_INFRA] = false;
        }

        break;

    case MID_INST_LOG:
        if (((C_PopupList *)control)->GetItemState(ID))
        {
            gMapMgr->ShowObjectiveType(_OBTV_LOGISTICS);
            filterState[OBJ_LOGISTICS] = true;
        }
        else
        {
            gMapMgr->HideObjectiveType(_OBTV_LOGISTICS);
            filterState[OBJ_LOGISTICS] = false;
        }

        break;

    case MID_INST_WARPROD:
        if (((C_PopupList *)control)->GetItemState(ID))
        {
            gMapMgr->ShowObjectiveType(_OBTV_WAR_PRODUCTION);
            filterState[OBJ_WARPRODUCTION] = true;
        }
        else
        {
            gMapMgr->HideObjectiveType(_OBTV_WAR_PRODUCTION);
            filterState[OBJ_WARPRODUCTION] = false;
        }

        break;

    case MID_INST_NAV:
        if (((C_PopupList *)control)->GetItemState(ID))
        {
            gMapMgr->ShowObjectiveType(_OBTV_NAVIGATION);
            filterState[OBJ_NAVIGATION] = true;
        }
        else
        {
            gMapMgr->HideObjectiveType(_OBTV_NAVIGATION);
            filterState[OBJ_NAVIGATION] = false;
        }

        break;

    case MID_INST_OTHER:
        if (((C_PopupList *)control)->GetItemState(ID))
        {
            gMapMgr->ShowObjectiveType(_OBTV_OTHER);
            filterState[OBJ_OTHER] = true;
        }
        else
        {
            gMapMgr->HideObjectiveType(_OBTV_OTHER);
            filterState[OBJ_OTHER] = false;
        }

        break;

    case MID_INST_NAVAL:
        if (((C_PopupList *)control)->GetItemState(ID))
        {
            gMapMgr->ShowObjectiveType(_OBTV_NAVAL);
            filterState[OBJ_NAVAL] = true;
        }
        else
        {
            gMapMgr->HideObjectiveType(_OBTV_NAVAL);
            filterState[OBJ_NAVAL] = false;
        }

        break;

    case MID_SHOW_VC:
        if (((C_PopupList *)control)->GetItemState(ID))
        {
            gMapMgr->ShowObjectiveType(_VC_CONDITION_);
            filterState[OBJ_VICTORYCOND] = true;
        }
        else
        {
            gMapMgr->HideObjectiveType(_VC_CONDITION_);
            filterState[OBJ_VICTORYCOND] = false;
        }

        break;
    }

    gMapMgr->DrawMap();
}

void MenuToggleUnitCB(long ID, short, C_Base *control)
{
    using namespace FilterSaveStuff; // Retro 26/10/03. Here I take note if a certain unit filter is enabled or disabled

    switch (ID)
    {
    case MID_UNITS_DIV:
        gMapMgr->SetUnitLevel(0);
        filterState[UNITS_DIV] = true;
        filterState[UNITS_BRIG] = filterState[UNITS_BAT] = false;
        break;

    case MID_UNITS_BRIG:
        gMapMgr->SetUnitLevel(1);
        filterState[UNITS_BRIG] = true;
        filterState[UNITS_DIV] = filterState[UNITS_BAT] = false;
        break;

    case MID_UNITS_BAT:
        gMapMgr->SetUnitLevel(2);
        filterState[UNITS_BAT] = true;
        filterState[UNITS_DIV] = filterState[UNITS_BRIG] = false;
        break;

    case MID_UNITS_COMBAT:
        if (((C_PopupList *)control)->GetItemState(ID))
        {
            gMapMgr->ShowUnitType(_UNIT_COMBAT);
            filterState[UNITS_COMBAT] = true;
        }
        else
        {
            gMapMgr->HideUnitType(_UNIT_COMBAT);
            filterState[UNITS_COMBAT] = false;
        }

        break;

    case MID_UNITS_AD:
        if (((C_PopupList *)control)->GetItemState(ID))
        {
            gMapMgr->ShowUnitType(_UNIT_AIR_DEFENSE);
            filterState[UNITS_AIR_DEFENSE] = true;
        }
        else
        {
            gMapMgr->HideUnitType(_UNIT_AIR_DEFENSE);
            filterState[UNITS_AIR_DEFENSE] = false;
        }

        break;

    case MID_UNITS_SUPPORT:
        if (((C_PopupList *)control)->GetItemState(ID))
        {
            gMapMgr->ShowUnitType(_UNIT_SUPPORT);
            filterState[UNITS_SUPPORT] = true;
        }
        else
        {
            gMapMgr->HideUnitType(_UNIT_SUPPORT);
            filterState[UNITS_SUPPORT] = false;
        }

        break;

    case MID_UNITS_SQUAD_SQUADRON:
        if (((C_PopupList *)control)->GetItemState(ID))
        {
            gMapMgr->ShowObjectiveType(_UNIT_SQUADRON);
            filterState[UNITS_SQUAD_SQUADRON] = true;
        }
        else
        {
            gMapMgr->HideObjectiveType(_UNIT_SQUADRON);
            filterState[UNITS_SQUAD_SQUADRON] = false;
        }

        break;

    case MID_UNITS_SQUAD_PACKAGE:
        if (((C_PopupList *)control)->GetItemState(ID))
        {
            gMapMgr->ShowObjectiveType(_UNIT_PACKAGE);
            filterState[UNITS_SQUAD_PACKAGE] = true;
        }
        else
        {
            gMapMgr->HideObjectiveType(_UNIT_PACKAGE);
            filterState[UNITS_SQUAD_PACKAGE] = false;
        }

        break;

    case MID_UNITS_SQUAD_FIGHTER:
        if (((C_PopupList *)control)->GetItemState(ID))
        {
            gMapMgr->ShowAirUnitType(_UNIT_FIGHTER);
            filterState[UNITS_SQUAD_FIGHTER] = true;
        }
        else
        {
            gMapMgr->HideAirUnitType(_UNIT_FIGHTER);
            filterState[UNITS_SQUAD_FIGHTER] = false;
        }

        break;

    case MID_UNITS_SQUAD_ATTACK:
        if (((C_PopupList *)control)->GetItemState(ID))
        {
            gMapMgr->ShowAirUnitType(_UNIT_ATTACK);
            filterState[UNITS_SQUAD_ATTACK] = true;
        }
        else
        {
            gMapMgr->HideAirUnitType(_UNIT_ATTACK);
            filterState[UNITS_SQUAD_ATTACK] = false;
        }

        break;

    case MID_UNITS_SQUAD_BOMBER:
        if (((C_PopupList *)control)->GetItemState(ID))
        {
            gMapMgr->ShowAirUnitType(_UNIT_BOMBER);
            filterState[UNITS_SQUAD_BOMBER] = true;
        }
        else
        {
            gMapMgr->HideAirUnitType(_UNIT_BOMBER);
            filterState[UNITS_SQUAD_BOMBER] = false;
        }

        break;

    case MID_UNITS_SQUAD_SUPPORT:
        if (((C_PopupList *)control)->GetItemState(ID))
        {
            gMapMgr->ShowAirUnitType(_UNIT_SUPPORT);
            filterState[UNITS_SQUAD_SUPPORT] = true;
        }
        else
        {
            gMapMgr->HideAirUnitType(_UNIT_SUPPORT);
            filterState[UNITS_SQUAD_SUPPORT] = false;
        }

        break;

    case MID_UNITS_SQUAD_HELI:
        if (((C_PopupList *)control)->GetItemState(ID))
        {
            gMapMgr->ShowAirUnitType(_UNIT_HELICOPTER);
            filterState[UNITS_HELICOPTER] = true;
        }
        else
        {
            gMapMgr->HideAirUnitType(_UNIT_HELICOPTER);
            filterState[UNITS_HELICOPTER] = false;
        }

        break;

        // 2002-02-21 ADDED BY S.G. Our new 'Unknown' option to Flights page so we can display unknown type of flight as well as identified one
    case MID_UNITS_SQUAD_UNKNOWN:
        if (((C_PopupList *)control)->GetItemState(ID))
        {
            filterState[UNITS_SQUAD_UNKNOWN] = true;
            gShowUnknown = 1;
        }
        else
        {
            filterState[UNITS_SQUAD_UNKNOWN] = false;
            gShowUnknown = 0;
        }

        if (gGps)
            gGps->Update();

        gMapMgr->RefreshAllAirUnitType();
        break;

    case MID_UNITS_NAVY_COMBAT:
        if (((C_PopupList *)control)->GetItemState(ID))
        {
            gMapMgr->ShowNavalUnitType(_UNIT_COMBAT);
            filterState[UNITS_NAVY_COMBAT] = true;
        }
        else
        {
            gMapMgr->HideNavalUnitType(_UNIT_COMBAT);
            filterState[UNITS_NAVY_COMBAT] = false;
        }

        break;

    case MID_UNITS_NAVY_SUPPLY:
        if (((C_PopupList *)control)->GetItemState(ID))
        {
            gMapMgr->ShowNavalUnitType(_UNIT_SUPPORT);
            filterState[UNITS_NAVY_SUPPORT] = true;
        }
        else
        {
            gMapMgr->HideNavalUnitType(_UNIT_SUPPORT);
            filterState[UNITS_NAVY_SUPPORT] = false;
        }

        break;
    }

    gMapMgr->DrawMap();
}

void MenuToggleNamesCB(long ID, short, C_Base *control)
{
    using namespace FilterSaveStuff; // Retro 26/10/03. Here I take note if map names are enabled or disabled

    if (((C_PopupList *)control)->GetItemState(ID))
    {
        gMapMgr->TurnOnNames();
        filterState[LE_LABELS] = true;
    }
    else
    {
        gMapMgr->TurnOffNames();
        filterState[LE_LABELS] = false;
    }

    gMapMgr->DrawMap();
}

void MenuToggleBullseyeCB(long ID, short, C_Base *control)
{
    using namespace FilterSaveStuff; // Retro 26/10/03. Here I take note if the bullseye is enabled or disabled

    if (((C_PopupList *)control)->GetItemState(ID))
    {
        gMapMgr->TurnOnBullseye();
        filterState[LE_BULLSEYE] = true;
    }
    else
    {
        gMapMgr->TurnOffBullseye();
        filterState[LE_BULLSEYE] = false;
    }

    gMapMgr->DrawMap();
}
/************************************************************************/
// Artscout - 2026: the Logistics submenu -- the campaign's supply model on the map.
//
// Same shape as MenuSetCirclesCB below it, and for the same reason: these four are a radio group
// because C_ScaleBitmap keeps ONE blended palette, so only one raster overlay can be live. Picking
// a layer here therefore also has to drop the threat rings, which is why this clears their four
// check marks -- otherwise the menu would claim a ring was still drawn when the map had taken the
// overlay away from it.
/************************************************************************/
void MenuSetCampLayerCB(long ID, short, C_Base *)
{
    using namespace FilterSaveStuff;

    C_PopupList *menu = gPopupMgr->GetMenu(MAP_POP);

    if (not menu or not gMapMgr)
        return;

    switch (ID)
    {
    case MID_CAMP_LAYER_POWER:
        campLayer = C_Map::CAMP_OVERLAY_POWER;
        break;

    case MID_CAMP_LAYER_SUPPLY:
        campLayer = C_Map::CAMP_OVERLAY_SUPPLY;
        break;

    case MID_CAMP_LAYER_PROD:
        campLayer = C_Map::CAMP_OVERLAY_PRODUCTION;
        break;

    case MID_CAMP_LAYER_DAMAGE:
        campLayer = C_Map::CAMP_OVERLAY_DAMAGE;
        break;

    default:
        campLayer = C_Map::CAMP_OVERLAY_OFF;
        break;
    }

    if (campLayer not_eq C_Map::CAMP_OVERLAY_OFF)
    {
        menu->SetItemState(MID_CIRCLE_SAM_LOW, 0);
        menu->SetItemState(MID_CIRCLE_SAM_HIGH, 0);
        menu->SetItemState(MID_CIRCLE_RADAR_LOW, 0);
        menu->SetItemState(MID_CIRCLE_RADAR_HIGH, 0);
        filterState[CIRCLE_SAM_LOW] = filterState[CIRCLE_SAM_HIGH] =
            filterState[CIRCLE_RADAR_LOW] = filterState[CIRCLE_RADAR_HIGH] =
                false;
    }

    gMapMgr->ShowCampaignOverlay(campLayer);
    gMapMgr->DrawMap();
}

// Artscout - 2026: the FLOT toggle. Outside the layer radio group on purpose -- the front is a
// bearing you keep while reading another layer, so it composites over whichever one is live and
// survives all of them being off.
void MenuToggleFlotCB(long, short, C_Base *)
{
    using namespace FilterSaveStuff; // campLayer: which radio layer is live underneath

    extern bool g_bCampFlotLine;

    C_PopupList *menu = gPopupMgr->GetMenu(MAP_POP);

    if (not menu or not gMapMgr)
        return;

    g_bCampFlotLine = menu->GetItemState(MID_CAMP_FLOT) ? true : false;

    gMapMgr->ShowCampaignOverlay(campLayer);
    gMapMgr->DrawMap();
}

void MenuSetCirclesCB(long, short, C_Base *)
{
    using namespace FilterSaveStuff; // Retro 26/10/03. Here I take note if a threat filter is enabled or disabled.
    // this works as radiobutton so only one may be active

    C_PopupList *menu;

    menu = gPopupMgr->GetMenu(MAP_POP);

    if (menu)
    {
        // Artscout - 2026: the rings and the Logistics layers share one palette, so
        // switching a ring on takes the overlay from whichever layer had it. Move that
        // group's check mark back to None so the menu still says what the map is showing.
        if (menu->GetItemState(MID_CIRCLE_SAM_LOW) or
            menu->GetItemState(MID_CIRCLE_SAM_HIGH) or
            menu->GetItemState(MID_CIRCLE_RADAR_LOW) or
            menu->GetItemState(MID_CIRCLE_RADAR_HIGH))
        {
            FilterSaveStuff::campLayer = C_Map::CAMP_OVERLAY_OFF;
            menu->SetItemState(MID_CAMP_LAYER_OFF, 1);
        }

        if (menu->GetItemState(MID_CIRCLE_SAM_LOW))
        {
            filterState[CIRCLE_SAM_LOW] = true;
            filterState[CIRCLE_SAM_HIGH] = filterState[CIRCLE_RADAR_LOW] =
                filterState[CIRCLE_RADAR_HIGH] = false;
            gMapMgr->ShowThreatType(_THR_SAM_LOW);
        }
        else if (menu->GetItemState(MID_CIRCLE_SAM_HIGH))
        {
            filterState[CIRCLE_SAM_HIGH] = true;
            filterState[CIRCLE_SAM_LOW] = filterState[CIRCLE_RADAR_LOW] =
                filterState[CIRCLE_RADAR_HIGH] = false;
            gMapMgr->ShowThreatType(_THR_SAM_HIGH);
        }
        else if (menu->GetItemState(MID_CIRCLE_RADAR_LOW))
        {
            filterState[CIRCLE_RADAR_LOW] = true;
            filterState[CIRCLE_SAM_LOW] = filterState[CIRCLE_SAM_HIGH] =
                filterState[CIRCLE_RADAR_HIGH] = false;
            gMapMgr->ShowThreatType(_THR_RADAR_LOW);
        }
        else if (menu->GetItemState(MID_CIRCLE_RADAR_HIGH))
        {
            filterState[CIRCLE_RADAR_HIGH] = true;
            filterState[CIRCLE_SAM_LOW] = filterState[CIRCLE_SAM_HIGH] =
                filterState[CIRCLE_RADAR_LOW] = false;
            gMapMgr->ShowThreatType(_THR_RADAR_HIGH);
        }
        else
        {
            filterState[CIRCLE_SAM_LOW] = filterState[CIRCLE_SAM_HIGH] =
                filterState[CIRCLE_RADAR_LOW] = filterState[CIRCLE_RADAR_HIGH] =
                    false;
            gMapMgr->HideThreatType(_THR_SAM_LOW bitor _THR_SAM_HIGH bitor
                                    _THR_RADAR_LOW bitor _THR_RADAR_HIGH);
        }

        gMapMgr->DrawMap();
    }
}

void MenuToggleTroupBoundariesCB(long, short, C_Base *)
{
}

void MenuToggleMovementArrowsCB(long, short, C_Base *)
{
}

// THE MAJOR CHANGE to this routine is making it support being called from
// either a C_MapIcon item OR a C_TreeList item
//
void MenuObjReconCB(long, short, C_Base *)
{
    Objective objective;
    C_Window *win;
    C_Base *caller;
    C_MapIcon *icon;
    C_DrawList *piggy;
    C_TreeList *tree;
    TREELIST *item;
    long iconid;
    UI_Refresher *urec = NULL;

    gPopupMgr->CloseMenu();

    caller = gPopupMgr->GetCallingControl();

    if (caller == NULL)
        return;

    if (caller->_GetCType_() == _CNTL_MAPICON_)
    {
        icon = (C_MapIcon *)caller;
        iconid = icon->GetIconID();
        urec = (UI_Refresher *)gGps->Find(iconid);
    }
    else if (caller->_GetCType_() == _CNTL_DRAWLIST_)
    {
        piggy = (C_DrawList *)caller;
        iconid = piggy->GetIconID();
        urec = (UI_Refresher *)gGps->Find(iconid);
    }
    else if (caller->_GetCType_() == _CNTL_TREELIST_)
    {
        tree = (C_TreeList *)caller;
        item = tree->GetLastItem();

        if (item)
            urec = (UI_Refresher *)gGps->Find(item->ID_);
    }

    gPopupMgr->CloseMenu();

    SetCursor(gCursors[CRSR_WAIT]);

    win = gMainHandler->FindWindow(RECON_WIN);

    if (win)
    {
        CloseAllRenderers(RECON_WIN);

        if (TargetTree)
            TargetTree->DeleteBranch(TargetTree->GetRoot());

        objective = (Objective)vuDatabase->Find(urec->GetID());

        if (objective == NULL)
            return;

        if (not objective->IsObjective())
            return;

        if (gUIViewer)
        {
            gUIViewer->Cleanup();
            delete gUIViewer;
        }

        gUIViewer = new C_3dViewer;
        gUIViewer->Setup();
        gUIViewer->Viewport(win, 0); // use client 0 for this window

        Recon.Heading = 0.0f;
        Recon.Pitch = 70.0f;
        Recon.Distance = 1000.0f;
        Recon.Direction = 0.0f;

        Recon.MinDistance = 250.0f;
        Recon.MaxDistance = 30000.0f;
        Recon.MinPitch = 5;
        Recon.MaxPitch = 90;
        Recon.CheckPitch = TRUE;

        Recon.PosX = objective->XPos();
        Recon.PosY = objective->YPos();
        Recon.PosZ = objective->ZPos();

        AddObjectiveToTargetTree(objective);
        GetGroundUnitsNear(Recon.PosX, Recon.PosY, 10000.0f);

        SetBullsEye(win);
        SetSlantRange(win);
        SetHeading(win);

        gUIViewer->SetPosition(Recon.PosX, Recon.PosY, Recon.PosZ);
        gUIViewer->InitOTW(30.0f, FALSE);
        gUIViewer->AddAllToView();

        win->ScanClientAreas();
        win->RefreshWindow();

        PositionCamera(&Recon, win, 0);
        TheLoader.WaitLoader();
        PositionCamera(&Recon, win, 0);

        gMainHandler->ShowWindow(win);
        gMainHandler->WindowToFront(win);
    }

    win = gMainHandler->FindWindow(RECON_LIST_WIN);

    if (win)
    {
        if (TargetTree)
            TargetTree->RecalcSize();

        gMainHandler->ShowWindow(win);
        gMainHandler->WindowToFront(win);
    }

    SetCursor(gCursors[CRSR_F16]);
}

void MenuAlternateCB(long, short, C_Base *)
{
    C_Base *caller;
    C_MapIcon *icon;
    C_DrawList *piggy;
    C_TreeList *tree;
    TREELIST *item;
    long iconid;
    UI_Refresher *urec = NULL;

    gPopupMgr->CloseMenu();

    caller = gPopupMgr->GetCallingControl();

    if (caller == NULL)
        return;

    if (caller->_GetCType_() == _CNTL_MAPICON_)
    {
        icon = (C_MapIcon *)caller;
        iconid = icon->GetIconID();
        urec = (UI_Refresher *)gGps->Find(iconid);
    }
    else if (caller->_GetCType_() == _CNTL_DRAWLIST_)
    {
        piggy = (C_DrawList *)caller;
        iconid = piggy->GetIconID();
        urec = (UI_Refresher *)gGps->Find(iconid);
    }
    else if (caller->_GetCType_() == _CNTL_TREELIST_)
    {
        tree = (C_TreeList *)caller;
        item = tree->GetLastItem();

        if (item)
            urec = (UI_Refresher *)gGps->Find(item->ID_);
    }
}

WayPointClass *GetSelectedWayPoint(void)
{
    C_Base *caller;
    C_Base *control;
    C_Waypoint *cwp;
    VU_ID *tmpID;
    Unit unit;
    WayPoint wp;
    int i;

    caller = gPopupMgr->GetCallingControl();

    if (caller == NULL)
        return NULL;

    if (gPopupMgr->GetCallingType() == C_TYPE_CONTROL and
        caller->_GetCType_() == _CNTL_WAYPOINT_)
    {
        // Waypoint
        cwp = (C_Waypoint *)caller;

        if (cwp and cwp->GetLast())
        {
            control = cwp->GetLast()->Icon;

            if (not control)
                return NULL;

            tmpID = (VU_ID *)control->GetUserPtr(C_STATE_0);

            if (not tmpID)
                return NULL;

            // Check if this is our current waypoint set, and make sure our
            // global ids match this information.
            if (gMapMgr->GetCurWP() == cwp)
            {
                VU_ID *tmpID;

                tmpID = (VU_ID *)control->GetUserPtr(C_STATE_0);

                if (tmpID and *tmpID not_eq gActiveFlightID)
                {
                    gActiveFlightID = *tmpID;
                    gCurrentFlightID = *tmpID;
                }
            }

            unit = FindUnit(*tmpID);

            if (unit and unit->IsFlight())
            {
                wp = unit->GetFirstUnitWP();
                i = 1;

                while (i < control->GetUserNumber(C_STATE_1) and wp)
                {
                    wp = wp->GetNextWP();
                    i++;
                }

                return wp;
            }
        }
    }

    return NULL;
}

void SteerPointMenuOpenCB(C_Base *, C_Base *)
{
    C_PopupList *menu;
    WayPoint wp;

    // We've opened a steerpoint's popup menu. Setup current values
    wp = GetSelectedWayPoint();

    menu = gPopupMgr->GetMenu(STEERPOINT_POP);

    if (menu and wp)
    {
        if (wp->GetWPFlags() bitand WPF_TIME_LOCKED)
            menu->SetItemState(MID_LOCK_TOS, 1);
        else
            menu->SetItemState(MID_LOCK_TOS, 0);

        if (wp->GetWPFlags() bitand WPF_SPEED_LOCKED)
            menu->SetItemState(MID_LOCK_SPEED, 1);
        else
            menu->SetItemState(MID_LOCK_SPEED, 0);

        if (wp->GetWPFlags() bitand WPF_HOLDCURRENT)
            menu->SetItemState(CLIMB_DELAY, 1);
        else
            menu->SetItemState(CLIMB_IMMEDIATE, 1);

        menu->SetItemState(wp->GetWPFormation() + 1, 1);
        menu->SetItemState(wp->GetWPAction() bitor 0x200, 1);
        menu->SetItemState(wp->GetWPRouteAction() bitor 0x100, 1);
    }
}

void MenuUnitDeleteCB(long, short, C_Base *)
{
    C_Base *caller;
    C_MapIcon *icon;
    C_DrawList *piggy;
    C_TreeList *tree;
    TREELIST *item;
    long iconid, count;
    UI_Refresher *urec = NULL;
    Unit unit, sec;
    Package pkg;

    gPopupMgr->CloseMenu();

    caller = gPopupMgr->GetCallingControl();

    if (caller == NULL)
        return;

    if (gPopupMgr->GetCallingType() == C_TYPE_CONTROL)
    {
        // Recon this + very close Objects
        if (caller->_GetCType_() == _CNTL_MAPICON_)
        {
            icon = (C_MapIcon *)caller;
            iconid = icon->GetIconID();
            urec = (UI_Refresher *)gGps->Find(iconid);
        }
        else if (caller->_GetCType_() == _CNTL_DRAWLIST_)
        {
            piggy = (C_DrawList *)caller;
            iconid = piggy->GetIconID();
            urec = (UI_Refresher *)gGps->Find(iconid);
        }
        else if (caller->_GetCType_() == _CNTL_TREELIST_)
        {
            tree = (C_TreeList *)caller;
            item = tree->GetLastItem();

            if (item)
                urec = (UI_Refresher *)gGps->Find(item->ID_);
        }

        if (urec)
        {
            unit = (Unit)vuDatabase->Find(urec->GetID());

            if (unit and unit->IsUnit())
            {
                pkg = NULL;
                count = 0;

                if (unit->IsFlight())
                {
                    pkg = (Package)unit->GetUnitParent();

                    if (pkg and pkg->IsPackage())
                    {
                        sec = pkg->GetFirstUnitElement();

                        while (sec)
                        {
                            if (sec not_eq unit)
                                count++;

                            sec = pkg->GetNextUnitElement();
                        }
                    }
                    else
                        pkg = NULL;
                }

                unit->Remove();

                if (pkg and not count)
                    pkg->Remove();
            }
        }
    }
}

void MenuReconCB(long, short, C_Base *)
{
    C_Base *caller;
    C_MapIcon *icon;
    C_DrawList *piggy;
    C_TreeList *tree;
    C_Waypoint *wp;
    TREELIST *item;
    WAYPOINTLIST *wpicon;
    long iconid;
    UI_Refresher *urec = NULL;
    float x = 0.0f, y = 0.0f, range = 0.0f;
    short relx, rely;
    float maxy, scale;
    CampEntity ent;

    gPopupMgr->CloseMenu();

    caller = gPopupMgr->GetCallingControl();

    if (caller == NULL)
        return;

    if (gPopupMgr->GetCallingType() == C_TYPE_CONTROL)
    {
        // Recon this + very close Objects
        if (caller->_GetCType_() == _CNTL_MAPICON_)
        {
            icon = (C_MapIcon *)caller;
            iconid = icon->GetIconID();
            urec = (UI_Refresher *)gGps->Find(iconid);
        }
        else if (caller->_GetCType_() == _CNTL_WAYPOINT_)
        {
            wp = (C_Waypoint *)caller;

            if (wp)
            {
                wpicon = wp->GetLast();

                if (wpicon)
                {
                    x = caller->GetUserNumber(0) - wpicon->worldy;
                    y = wpicon->worldx;
                    range = 18000.0f;
                }
            }
        }
        else if (caller->_GetCType_() == _CNTL_DRAWLIST_)
        {
            piggy = (C_DrawList *)caller;
            iconid = piggy->GetIconID();
            urec = (UI_Refresher *)gGps->Find(iconid);
        }
        else if (caller->_GetCType_() == _CNTL_TREELIST_)
        {
            tree = (C_TreeList *)caller;
            item = tree->GetLastItem();

            if (item)
                urec = (UI_Refresher *)gGps->Find(item->ID_);
        }
        else if (caller->_GetCType_() == _CNTL_MAP_MOVER_)
        {
            // Recon Area...
            range = 18000.0f;

            relx = static_cast<short>(((C_MapMover *)caller)->GetRelX() +
                                      caller->GetX() + caller->Parent_->GetX());
            rely = static_cast<short>(((C_MapMover *)caller)->GetRelY() +
                                      caller->GetY() + caller->Parent_->GetY());
            gMapMgr->GetMapRelativeXY(&relx, &rely);

            scale = gMapMgr->GetMapScale();
            maxy = gMapMgr->GetMaxY();

            // x bitand y are reversed for SIM
            y = relx / scale;
            x = maxy - rely / scale;
        }

        if (urec)
        {
            ent = (CampEntity)vuDatabase->Find(urec->GetID());

            if (ent)
            {
                // 2002-02-21 ADDED BY S.G. If not spotted by the player's team or not editing a TE, can't recon...
                if (not(TheCampaign.Flags bitand CAMP_TACTICAL_EDIT) and
                    ent->IsFlight() and
                    (gGps->GetTeamNo() not_eq ent->GetTeam()) and
                    not ent->GetIdentified(
                        static_cast<Team>(gGps->GetTeamNo())))
                {
                    range = 0.0f;
                }
                else
                {
                    // END OF ADDED SECTION 2002-02-21
                    range = 6000.0f;
                    x = ent->XPos();
                    y = ent->YPos();
                }
            }
        }
    }

    if (range > 1.0f)
        ReconArea(x, y, range);
}

void MenuStatusCB(long, short, C_Base *)
{
    C_Base *caller;
    C_MapIcon *icon;
    C_DrawList *piggy;
    C_TreeList *tree;
    TREELIST *item;
    long iconid;
    UI_Refresher *urec = NULL;
    CampEntity ent;

    gPopupMgr->CloseMenu();

    caller = gPopupMgr->GetCallingControl();

    if (caller == NULL)
        return;

    if (gPopupMgr->GetCallingType() == C_TYPE_CONTROL)
    {
        // Recon this + very close Objects
        if (caller->_GetCType_() == _CNTL_MAPICON_)
        {
            icon = (C_MapIcon *)caller;
            iconid = icon->GetIconID();
            urec = (UI_Refresher *)gGps->Find(iconid);
        }
        else if (caller->_GetCType_() == _CNTL_DRAWLIST_)
        {
            piggy = (C_DrawList *)caller;
            iconid = piggy->GetIconID();
            urec = (UI_Refresher *)gGps->Find(iconid);
        }
        else if (caller->_GetCType_() == _CNTL_TREELIST_)
        {
            tree = (C_TreeList *)caller;
            item = tree->GetLastItem();

            if (item)
                urec = (UI_Refresher *)gGps->Find(item->ID_);
        }

        if (urec)
        {
            ent = (CampEntity)vuDatabase->Find(urec->GetID());

            if (ent)
            {
                BuildSpecificTargetList(ent->Id());
            }
        }
    }
}

void MenuUnitStatusCB(long, short, C_Base *)
{
    C_Window *win;
    C_Base *caller;
    C_MapIcon *icon;
    C_DrawList *piggy;
    C_TreeList *tree;
    long iconid = 0;
    TREELIST *item;
    CampEntity ent;
    UI_Refresher *urec = NULL;

    gPopupMgr->CloseMenu();

    caller = gPopupMgr->GetCallingControl();

    if (caller == NULL)
        return;

    if (caller->_GetCType_() == _CNTL_MAPICON_)
    {
        icon = (C_MapIcon *)caller;
        iconid = icon->GetIconID();
        urec = (UI_Refresher *)gGps->Find(iconid);
    }
    else if (caller->_GetCType_() == _CNTL_DRAWLIST_)
    {
        piggy = (C_DrawList *)caller;
        iconid = piggy->GetIconID();
        urec = (UI_Refresher *)gGps->Find(iconid);
    }
    else if (caller->_GetCType_() == _CNTL_TREELIST_)
    {
        tree = (C_TreeList *)caller;
        item = tree->GetLastItem();

        if (item)
        {
            iconid = item->ID_;
            urec = (UI_Refresher *)gGps->Find(
                item->ID_ bitand
                0x00ffffff); // strip off team (incase it is a division)
        }
    }

    if (urec)
    {
        win = gMainHandler->FindWindow(UNIT_WIN);

        if (win)
        {
            if (urec and urec->GetType() == GPS_DIVISION)
                SetupDivisionInfoWindow(
                    urec->GetDivID(),
                    urec->GetSide()); // Map bitand Tree save team # in top 8 bits (Needed to find division by team)
            else
            {
                ent = (CampEntity)vuDatabase->Find(urec->GetID());

                if (ent and ent->IsUnit())
                {
                    if (ent->IsFlight() or ent->IsSquadron())
                    {
                        // 2002-02-21 ADDED BY S.G. If not spotted by the player's team or not editing a TE, can't get its status...
                        if ((TheCampaign.Flags bitand CAMP_TACTICAL_EDIT) or
                            (gGps->GetTeamNo() == ent->GetTeam()) or
                            ent->GetIdentified(
                                static_cast<Team>(gGps->GetTeamNo())))
                        {
                            // END OF ADDED SECTION 2002-02-21
                            SetupSquadronInfoWindow(urec->GetID());
                        }
                    }
                    else if (ent->IsTaskForce())
                    {
                        BuildSpecificTargetList(urec->GetID());
                    }
                    else
                        SetupUnitInfoWindow(urec->GetID());
                }
            }
        }
    }
}

void MenuShowSquadronsCB(long, short, C_Base *)
{
}

void MenuOpenWpWindowCB(long, short, C_Base *)
{
}

void MenuLockCB(long ID, short, C_Base *)
{
    WayPoint wp = NULL;

    gPopupMgr->CloseMenu();

    wp = GetSelectedWayPoint();

    if (wp)
    {
        if (ID == MID_LOCK_TOS)
            wp->SetWPFlag(WPF_TIME_LOCKED);

        if (ID == MID_LOCK_SPEED)
            wp->SetWPFlag(WPF_SPEED_LOCKED);

        refresh_waypoint(wp);
    }
}

void MenuClimbCB(long ID, short, C_Base *)
{
    WayPoint wp = NULL;

    gPopupMgr->CloseMenu();

    wp = GetSelectedWayPoint();

    if (wp)
    {
        if (ID == CLIMB_DELAY)
            wp->SetWPFlag(WPF_HOLDCURRENT);
        else
            wp->UnSetWPFlag(WPF_HOLDCURRENT);

        refresh_waypoint(wp);
    }
}

void MenuFormationCB(long ID, short, C_Base *)
{
    WayPoint wp = NULL;
    int formation;

    gPopupMgr->CloseMenu();

    wp = GetSelectedWayPoint();

    if (wp)
    {
        formation = ID bitand 0xff;
        wp->SetWPFormation(formation - 1);
        refresh_waypoint(wp);
    }
}

void MenuEnrouteCB(long ID, short, C_Base *)
{
    WayPoint wp = NULL;
    int action;

    gPopupMgr->CloseMenu();

    wp = GetSelectedWayPoint();

    if (wp)
    {
        action = ID bitand 0xff;
        wp->SetWPRouteAction(action);
        refresh_waypoint(wp);
    }
}

void MenuActionCB(long ID, short, C_Base *)
{
    WayPoint wp = NULL;
    int action;

    gPopupMgr->CloseMenu();

    wp = GetSelectedWayPoint();

    if (wp)
    {
        action = ID bitand 0xff;
        set_waypoint_action(wp, action);
        refresh_waypoint(wp);
    }
}

void MenuAddWPCB(long, short, C_Base *)
{
    C_Waypoint *cwp;

    WayPoint wp;

    WAYPOINTLIST
    *wps;

    Unit un;

    int num;

    cwp = (C_Waypoint *)gPopupMgr->GetCallingControl();

    un = (Unit)vuDatabase->Find(gMapMgr->GetCurWPID());

    if (not un)
        return;

    wp = un->GetFirstUnitWP();
    // if(un->GetCurrentUnitWP() not_eq wp)
    // return;

    wps = cwp->GetLast();

    num = wps->ID bitand 0xff;

    while ((num > 1) and (wp))
    {
        wp = wp->GetNextWP();
        num--;
    }

    wp->SplitWP();

    gMapMgr->SetCurrentWaypointList(un->Id());
    gPopupMgr->CloseMenu();
}

void MenuDeleteWPCB(long, short hittype, C_Base *)
{
    WayPoint wp, rw;
    Unit un;

    un = (Unit)vuDatabase->Find(gMapMgr->GetCurWPID());

    if (not un)
        return;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    gPopupMgr->CloseMenu();

    wp = GetSelectedWayPoint();

    if (wp and (wp->GetPrevWP()) and (wp->GetNextWP()))
    {
        rw = wp->GetPrevWP();
        un->DeleteUnitWP(wp);
        recalculate_waypoints(rw);
    }

    gMapMgr->SetCurrentWaypointList(un->Id());

    if ((TheCampaign.Flags bitand CAMP_TACTICAL_EDIT) and un->IsFlight())
    {
        fixup_unit(un);
        gGps->Update();
        gMapMgr->DrawMap();
    }
}

void MenuEditPackageCB(long, short, C_Base *control)
{
    C_Base *caller;
    C_MapIcon *icon;
    C_DrawList *piggy;
    C_TreeList *tree;
    TREELIST *item;
    long iconid = 0;
    UI_Refresher *urec = NULL;

    gPopupMgr->CloseMenu();

    caller = gPopupMgr->GetCallingControl();

    if (caller == NULL)
        return;

    if (caller->_GetCType_() == _CNTL_MAPICON_)
    {
        icon = (C_MapIcon *)caller;
        iconid = icon->GetIconID();
        urec = (UI_Refresher *)gGps->Find(iconid);
    }
    else if (caller->_GetCType_() == _CNTL_DRAWLIST_)
    {
        piggy = (C_DrawList *)caller;
        iconid = piggy->GetIconID();
        urec = (UI_Refresher *)gGps->Find(iconid);
    }
    else if (caller->_GetCType_() == _CNTL_TREELIST_)
    {
        tree = (C_TreeList *)caller;
        item = tree->GetLastItem();

        if (item)
        {
            iconid = item->ID_;
            urec = (UI_Refresher *)gGps->Find(
                item->ID_ bitand
                0x00ffffff); // strip off team (incase it is a division)
        }
    }

    if (urec)
    {
        tactical_edit_package(urec->GetID(), control);
    }
}

void MenuSetOwnerCB(long ID, short, C_Base *)
{
    C_Base *caller;
    C_MapIcon *icon;
    C_DrawList *piggy;
    C_TreeList *tree;
    long iconid;
    TREELIST *item;
    UI_Refresher *urec;

    urec = NULL;

    caller = gPopupMgr->GetCallingControl();

    if (caller == NULL)
    {
        return;
    }

    gPopupMgr->CloseMenu();

    if (caller->_GetCType_() == _CNTL_MAPICON_)
    {
        icon = (C_MapIcon *)caller;
        iconid = icon->GetIconID();
        urec = (UI_Refresher *)gGps->Find(iconid);
    }
    else if (caller->_GetCType_() == _CNTL_DRAWLIST_)
    {
        piggy = (C_DrawList *)caller;
        iconid = piggy->GetIconID();
        urec = (UI_Refresher *)gGps->Find(iconid);
    }
    else if (caller->_GetCType_() == _CNTL_TREELIST_)
    {
        tree = (C_TreeList *)caller;
        item = tree->GetLastItem();

        if (item)
        {
            urec = (UI_Refresher *)gGps->Find(item->ID_);
        }
    }

    if (urec)
    {
        CampEntity ent;
        short teamid = 0;

        switch (ID)
        {
        case MID_TEAM_1:
            teamid = 1;
            break;

        case MID_TEAM_2:
            teamid = 2;
            break;

        case MID_TEAM_3:
            teamid = 3;
            break;

        case MID_TEAM_4:
            teamid = 4;
            break;

        case MID_TEAM_5:
            teamid = 5;
            break;

        case MID_TEAM_6:
            teamid = 6;
            break;

        case MID_TEAM_7:
            teamid = 7;
            break;

        default:
            teamid = 0;
            break;
        }

        ent = (CampEntity)vuDatabase->Find(urec->GetID());

        if (ent)
        {
            ent->SetOwner(static_cast<uchar>(teamid));
        }
    }
}

void MenuAddUnitCB(long ID, short, C_Base *control)
{
    extern uchar gSelectedTeam;
    C_Base *caller;
    C_MapIcon *icon;
    C_DrawList *piggy;
    C_TreeList *tree;
    long iconid;
    TREELIST *item;
    UI_Refresher *urec;
    VU_ID vid = FalconNullId;

    urec = NULL;
    caller = gPopupMgr->GetCallingControl();

    if (not caller)
        return;

    if (caller->_GetCType_() == _CNTL_MAPICON_)
    {
        icon = (C_MapIcon *)caller;
        iconid = icon->GetIconID();
        urec = (UI_Refresher *)gGps->Find(iconid);

        if (urec)
            vid = urec->GetID();
    }
    else if (caller->_GetCType_() == _CNTL_DRAWLIST_)
    {
        piggy = (C_DrawList *)caller;
        iconid = piggy->GetIconID();
        urec = (UI_Refresher *)gGps->Find(iconid);

        if (urec)
            vid = urec->GetID();
    }
    else if (caller->_GetCType_() == _CNTL_TREELIST_)
    {
        tree = (C_TreeList *)caller;
        item = tree->GetLastItem();

        if (item)
        {
            urec = (UI_Refresher *)gGps->Find(item->ID_);

            if (urec)
                vid = urec->GetID();
        }
    }

    // Artscout - 2026: the Tactical Engagement builders read gSelectedTeam -- for the default
    // role here, and for mis.who and SetOwner once a flight is made. In a campaign that variable
    // is stale: TE drives it from its own team list box, the campaign assigns it once on entry,
    // and nothing keeps it in step. The "Build package" submenu hit this first -- a trace showed
    // every squadron in the theater failing the team test -- and these two entry points reach the
    // same code, so correct it on the way in rather than in each place it is read. GameType 1 is
    // the campaign; the TE screen keeps whatever its list box chose.
    if (GameType == 1 and FalconLocalSession)
        gSelectedTeam = FalconLocalSession->GetTeam();

    switch (ID)
    {
    case MID_ADD_FLIGHT:
        tactical_add_flight(vid, control);
        break;

    case MID_ADD_PACKAGE:
        tactical_add_package(vid, control);
        break;

    case MID_ADD_BATTALION:
        tactical_add_battalion(vid, control);
        break;

    case MID_ADD_SQUADRON:
        tactical_add_squadron(vid);
        break;
    }

    gPopupMgr->CloseMenu();
}

void MenuAddVCCB(long, short, C_Base *)
{
    C_Base *caller;
    C_MapIcon *icon;
    C_DrawList *piggy;
    C_TreeList *tree;
    long iconid;
    TREELIST *item;
    UI_Refresher *urec;

    urec = NULL;

    caller = gPopupMgr->GetCallingControl();

    if (caller == NULL)
    {
        return;
    }

    if (caller->_GetCType_() == _CNTL_MAPICON_)
    {
        icon = (C_MapIcon *)caller;

        iconid = icon->GetIconID();

        urec = (UI_Refresher *)gGps->Find(iconid);
    }
    else if (caller->_GetCType_() == _CNTL_DRAWLIST_)
    {
        piggy = (C_DrawList *)caller;

        iconid = piggy->GetIconID();

        urec = (UI_Refresher *)gGps->Find(iconid);
    }
    else if (caller->_GetCType_() == _CNTL_TREELIST_)
    {
        tree = (C_TreeList *)caller;

        item = tree->GetLastItem();

        if (item)
        {
            urec = (UI_Refresher *)gGps->Find(item->ID_);
        }
    }

    tactical_add_victory_condition(urec->GetID(), NULL);

    gPopupMgr->CloseMenu();
}

void SetMapSettings()
{
    C_PopupList *menu;

    menu = gPopupMgr->GetMenu(MAP_POP);

    // following block Retro 26/10/03 - re-inits the map filters
    {
        // trying to keep the namespace scope down..
        // works as following.. if a filter was set before the map screen was destructed, the corresponding
        // variable in the array should be TRUE.. I then make the UI code believe the filter is checked in the UI so
        // that the 'show this item' code is executed.
        // the normal, original UI init code follows after this block.
        using namespace FilterSaveStuff;

        // Legend stuff
        if (filterState[LE_BULLSEYE])
            menu->SetItemState(MID_LEG_BULLSEYE, 1);

        if (filterState[LE_LABELS])
            menu->SetItemState(MID_LEG_NAMES, 1);

        // Artscout - 2026: put the Logistics layer back the way it was left. The overlay
        // itself is rebuilt from live objective data, not restored, so what comes back is
        // the campaign as it stands now rather than a stale picture from last visit.
        if (campLayer not_eq C_Map::CAMP_OVERLAY_OFF and gMapMgr)
        {
            long want = campLayer;
            menu->SetItemState(
                (want == C_Map::CAMP_OVERLAY_POWER) ? MID_CAMP_LAYER_POWER :
                (want == C_Map::CAMP_OVERLAY_SUPPLY) ? MID_CAMP_LAYER_SUPPLY :
                (want == C_Map::CAMP_OVERLAY_DAMAGE) ? MID_CAMP_LAYER_DAMAGE :
                                                       MID_CAMP_LAYER_PROD,
                1);
            gMapMgr->ShowCampaignOverlay(want);
        }

        // Objectives
        if (filterState[OBJ_AIRFIELDS])
            menu->SetItemState(MID_INST_AF, 1);

        if (filterState[OBJ_AIRDEFENSE])
            menu->SetItemState(MID_INST_AD, 1);

        if (filterState[OBJ_ARMY])
            menu->SetItemState(MID_INST_ARMY, 1);

        if (filterState[OBJ_CCC])
            menu->SetItemState(MID_INST_CCC, 1);

        if (filterState[OBJ_POLITICAL])
            menu->SetItemState(MID_INST_POLITICAL, 1);

        if (filterState[OBJ_INFRA])
            menu->SetItemState(MID_INST_INFRA, 1);

        if (filterState[OBJ_LOGISTICS])
            menu->SetItemState(MID_INST_LOG, 1);

        if (filterState[OBJ_WARPRODUCTION])
            menu->SetItemState(MID_INST_WARPROD, 1);

        if (filterState[OBJ_NAVIGATION])
            menu->SetItemState(MID_INST_NAV, 1);

        if (filterState[OBJ_OTHER])
            menu->SetItemState(MID_INST_OTHER, 1);

        if (filterState[OBJ_NAVAL])
            menu->SetItemState(MID_INST_NAVAL, 1);

        if (filterState[OBJ_VICTORYCOND])
            menu->SetItemState(MID_SHOW_VC, 1);

        // Units

#if 0 // screwy, doesn�t work yet

        /* this is a radio box so only 1 of the 3 may be active */
        if (filterState[UNITS_DIV])
            menu->SetItemState(MID_UNITS_DIV, 1);
        else if (filterState[UNITS_BRIG])
            menu->SetItemState(MID_UNITS_BRIG, 1);
        else if (filterState[UNITS_BAT])
            menu->SetItemState(MID_UNITS_BAT, 1);

#endif

        if (filterState[UNITS_COMBAT])
            menu->SetItemState(MID_UNITS_COMBAT, 1);

        if (filterState[UNITS_AIR_DEFENSE])
            menu->SetItemState(MID_UNITS_AD, 1);

        if (filterState[UNITS_SUPPORT])
            menu->SetItemState(MID_UNITS_SUPPORT, 1);

        if (filterState[UNITS_SQUAD_SQUADRON])
            menu->SetItemState(MID_UNITS_SQUAD_SQUADRON, 1);

        if (filterState[UNITS_SQUAD_PACKAGE])
            menu->SetItemState(MID_UNITS_SQUAD_PACKAGE, 1);

        if (filterState[UNITS_SQUAD_FIGHTER])
            menu->SetItemState(MID_UNITS_SQUAD_FIGHTER, 1);

        if (filterState[UNITS_SQUAD_ATTACK])
            menu->SetItemState(MID_UNITS_SQUAD_ATTACK, 1);

        if (filterState[UNITS_SQUAD_BOMBER])
            menu->SetItemState(MID_UNITS_SQUAD_BOMBER, 1);

        if (filterState[UNITS_SQUAD_SUPPORT])
            menu->SetItemState(MID_UNITS_SQUAD_SUPPORT, 1);

        if (filterState[UNITS_HELICOPTER])
            menu->SetItemState(MID_UNITS_SQUAD_HELI, 1);

        if (filterState[UNITS_NAVY_COMBAT])
            menu->SetItemState(MID_UNITS_NAVY_COMBAT, 1);

        if (filterState[UNITS_NAVY_SUPPORT])
            menu->SetItemState(MID_UNITS_NAVY_SUPPLY, 1);

        /* not saved yet - WTF is this anyway ? */
        // if (filterState[UNITS_SQUAD_FIGHTBOMB])
        // menu->SetItemState(MID_UNITS_SQUAD_FIGHTBOMB,1);

        if (g_nUnidentifiedInUI)
            if (filterState[UNITS_SQUAD_UNKNOWN])
                menu->SetItemState(MID_UNITS_SQUAD_UNKNOWN, 1);

        // Sams/Radar
        if (filterState[CIRCLE_SAM_LOW])
            menu->SetItemState(MID_CIRCLE_SAM_LOW, 1);
        else if (filterState[CIRCLE_SAM_HIGH])
            menu->SetItemState(MID_CIRCLE_SAM_HIGH, 1);
        else if (filterState[CIRCLE_RADAR_LOW])
            menu->SetItemState(MID_CIRCLE_RADAR_LOW, 1);
        else if (filterState[CIRCLE_RADAR_HIGH])
            menu->SetItemState(MID_CIRCLE_RADAR_HIGH, 1);

    } // namespace-scope ends here, Retro 26/10/03 ends here

    if (menu)
    {
        // Legend stuff
        MenuToggleNamesCB(MID_LEG_NAMES, C_TYPE_LMOUSEUP, menu);
        MenuToggleBullseyeCB(MID_LEG_BULLSEYE, C_TYPE_LMOUSEUP, menu);

        // Objectives
        MenuToggleObjectiveCB(MID_INST_AF, C_TYPE_LMOUSEUP, menu);
        MenuToggleObjectiveCB(MID_INST_AD, C_TYPE_LMOUSEUP, menu);
        MenuToggleObjectiveCB(MID_INST_ARMY, C_TYPE_LMOUSEUP, menu);
        MenuToggleObjectiveCB(MID_INST_CCC, C_TYPE_LMOUSEUP, menu);
        MenuToggleObjectiveCB(MID_INST_POLITICAL, C_TYPE_LMOUSEUP, menu);
        MenuToggleObjectiveCB(MID_INST_INFRA, C_TYPE_LMOUSEUP, menu);
        MenuToggleObjectiveCB(MID_INST_LOG, C_TYPE_LMOUSEUP, menu);
        MenuToggleObjectiveCB(MID_INST_WARPROD, C_TYPE_LMOUSEUP, menu);
        MenuToggleObjectiveCB(MID_INST_NAV, C_TYPE_LMOUSEUP, menu);
        MenuToggleObjectiveCB(MID_INST_OTHER, C_TYPE_LMOUSEUP, menu);
        MenuToggleObjectiveCB(MID_INST_NAVAL, C_TYPE_LMOUSEUP, menu);
        MenuToggleObjectiveCB(MID_SHOW_VC, C_TYPE_LMOUSEUP, menu);

        // Units
        MenuToggleUnitCB(MID_UNITS_SQUAD_SQUADRON, C_TYPE_LMOUSEUP, menu);
        MenuToggleUnitCB(MID_UNITS_SQUAD_PACKAGE, C_TYPE_LMOUSEUP, menu);
        MenuToggleUnitCB(MID_UNITS_DIV, C_TYPE_LMOUSEUP, menu);
        MenuToggleUnitCB(MID_UNITS_BRIG, C_TYPE_LMOUSEUP, menu);
        MenuToggleUnitCB(MID_UNITS_BAT, C_TYPE_LMOUSEUP, menu);
        MenuToggleUnitCB(MID_UNITS_COMBAT, C_TYPE_LMOUSEUP, menu);
        MenuToggleUnitCB(MID_UNITS_AD, C_TYPE_LMOUSEUP, menu);
        MenuToggleUnitCB(MID_UNITS_SUPPORT, C_TYPE_LMOUSEUP, menu);
        MenuToggleUnitCB(MID_UNITS_SQUAD_FIGHTER, C_TYPE_LMOUSEUP, menu);
        MenuToggleUnitCB(MID_UNITS_SQUAD_FIGHTBOMB, C_TYPE_LMOUSEUP, menu);
        MenuToggleUnitCB(MID_UNITS_SQUAD_ATTACK, C_TYPE_LMOUSEUP, menu);
        MenuToggleUnitCB(MID_UNITS_SQUAD_BOMBER, C_TYPE_LMOUSEUP, menu);
        MenuToggleUnitCB(MID_UNITS_SQUAD_SUPPORT, C_TYPE_LMOUSEUP, menu);
        MenuToggleUnitCB(MID_UNITS_SQUAD_HELI, C_TYPE_LMOUSEUP, menu);

        if (g_nUnidentifiedInUI)
            MenuToggleUnitCB(
                MID_UNITS_SQUAD_UNKNOWN, C_TYPE_LMOUSEUP,
                menu); // 2002-02-21 ADDED BY S.G. For 'Unknown' type of flight

        MenuToggleUnitCB(MID_UNITS_NAVY_COMBAT, C_TYPE_LMOUSEUP, menu);
        MenuToggleUnitCB(MID_UNITS_NAVY_SUPPLY, C_TYPE_LMOUSEUP, menu);

        // Sams/Radar
        MenuSetCirclesCB(MID_OFF, C_TYPE_LMOUSEUP, menu);
    }
}

///////////////////////////////////////////////////////////////////////////////
// Artscout - 2026: build a package against a right-clicked target, from inside the campaign.
//
// BMS lets you right-click something on the campaign map and build a strike on it. FreeFalcon has
// had every piece needed for that from the start -- it is what the Tactical Engagement editor does
// -- but those pieces are driven through te_scf.lst's windows (PACKAGE_WIN, TAC_FLIGHT_WIN), and
// the campaign screen loads cp_scf.lst. So un-hiding MID_ADD_PACKAGE in the campaign produced a
// menu item that did nothing at all: tactical_add_package guards every use of its window on
// FindWindow returning non-NULL, and quietly no-opped when it did not.
//
// The obvious repair -- load the TE window set alongside the campaign one -- means owning the
// interaction between two screens' window groups, and the dialog it would raise asks for things
// the campaign already knows: which team you are, which target you clicked, what role suits it. So
// this does not use those windows. The picker IS the popup menu: a "Build package" submenu whose
// contents are rebuilt from the theater each time the menu is raised, listing the squadrons that
// could actually fly this mission against this target, nearest first. Clicking one files it.
//
// Nothing here invents a rule about who can fly what. The filter is the engine's own:
// GetMissionFromTarget picks the role a given airframe can usefully bring against a given target
// and returns 0 when the answer is "nothing", which is exactly the "should this squadron be on the
// list" question. Whether the sortie is actually flyable is left to BuildMission -- the same call
// the TE editor makes, which can still refuse for no free aircraft or impossible timing. That
// refusal is reported rather than pre-guessed, because the reasons it refuses (slot scheduling
// across ATO time blocks) are not cheap to evaluate once per squadron per right-click.
///////////////////////////////////////////////////////////////////////////////

#define CAMP_PKG_SLOTS (MID_CAMP_PKG_SQ_LAST - MID_CAMP_PKG_SQ_FIRST + 1)

// What the last rebuild put in the menu. The menu can only hand a callback an item ID, so the
// squadron behind each slot has to be remembered here; the target has to be as well, because by
// the time the item is clicked the popup has moved on and GetCallingControl no longer points at
// what was right-clicked.
static VU_ID gCampPkgSquadron[CAMP_PKG_SLOTS];
static uchar gCampPkgRole[CAMP_PKG_SLOTS];
static VU_ID gCampPkgTargetID = FalconNullId;
static GridIndex gCampPkgX = 0, gCampPkgY = 0;
static int gCampPkgSize = 2; // sticky across right-clicks, like the other menu preferences

extern uchar gSelectedTeam;

// Artscout - 2026: the player's team, from the local session rather than from gSelectedTeam.
//
// gSelectedTeam belongs to the Tactical Engagement editor. TE sets it from its team list box
// (te_list.cpp) and hardcodes it to 1 for training; the campaign assigns it in exactly one place,
// on entry (campaign.cpp), and nothing keeps it in step afterwards. Copying tactical_make_package
// literally therefore carried across a variable that is only maintained on the other screen -- a
// trace of a live campaign reported team=1 while all 112 squadrons in the theater sat on other
// teams, so the candidate filter rejected every one of them and the submenu came up greyed with
// nothing to show.
//
// Every other campaign screen -- the ATO and priority screens, the map's intel gate -- reads
// FalconLocalSession->GetTeam(), which is ::GetTeam(country) and is the same value ato.cpp
// filters the ATO by. That is the one this feature wants.
static uchar CampPkgPlayerTeam(void)
{
    if (FalconLocalSession)
        return FalconLocalSession->GetTeam();

    return gSelectedTeam;
}

// SetOwner takes a COUNTRY, not a team: CampBaseClass::GetTeam() is ::GetTeam(owner), so an owner
// holding a team number only reads back as the right team where the two coincide. TE passes
// gSelectedTeam and gets away with it because its team numbers are its country numbers; a
// campaign's need not be. Take the session's country, and fall back to the team when it has none
// -- country can arrive as 0 after a campaign load, which is the case te_flow.cpp documents.
static uchar CampPkgPlayerCountry(void)
{
    if (FalconLocalSession and FalconLocalSession->GetCountry())
        return FalconLocalSession->GetCountry();

    return CampPkgPlayerTeam();
}

extern void AreYouSure(long TitleID, _TCHAR *text,
                       void (*OkCB)(long, short, C_Base *),
                       void (*CancelCB)(long, short, C_Base *));
extern void CloseWindowCB(long ID, short hittype, C_Base *control);
int GetMissionFromTarget(Team team, int dindex, CampEntity target);

// Short names for the roles GetMissionFromTarget can return. The ATO draws mission types as icons
// rather than text (C_Flight::SetCurrentTask feeds an image list), so there is no string table to
// borrow -- and a menu line has to say what it is going to build.
static const _TCHAR *CampPkgRoleName(int role)
{
    switch (role)
    {
    case AMIS_OCASTRIKE:
        return "OCA Strike";

    case AMIS_INTSTRIKE:
        return "Interdiction";

    case AMIS_STRIKE:
        return "Strike";

    case AMIS_SEADSTRIKE:
        return "SEAD Strike";

    case AMIS_PRPLANCAS:
        return "CAS";

    case AMIS_ONCALLCAS:
        return "On-call CAS";

    case AMIS_ASHIP:
        return "Anti-ship";

    case AMIS_INTERCEPT:
        return "Intercept";

    case AMIS_HAVCAP:
        return "HAVCAP";

    case AMIS_BARCAP:
        return "BARCAP";

    case AMIS_FAC:
        return "FAC";

    case AMIS_AWACS:
        return "AWACS";

    case AMIS_JSTAR:
        return "JSTAR";

    case AMIS_TANKER:
        return "Tanker";

    case AMIS_ECM:
        return "ECM";

    case AMIS_AIRLIFT:
        return "Airlift";

    case AMIS_RECONPATROL:
        return "Recon";

    default:
        return "Mission";
    }
}

// The right-clicked object, however the popup was raised. MenuAddUnitCB works this out inline for
// its own use; the submenu needs the same answer one step earlier, when the menu opens rather than
// when an item is picked.
static VU_ID CampaignPopupEntityID(C_Base *caller)
{
    UI_Refresher *urec = NULL;

    if (not caller)
        return FalconNullId;

    if (caller->_GetCType_() == _CNTL_MAPICON_)
        urec = (UI_Refresher *)gGps->Find(((C_MapIcon *)caller)->GetIconID());
    else if (caller->_GetCType_() == _CNTL_DRAWLIST_)
        urec = (UI_Refresher *)gGps->Find(((C_DrawList *)caller)->GetIconID());
    else if (caller->_GetCType_() == _CNTL_TREELIST_)
    {
        TREELIST *item = ((C_TreeList *)caller)->GetLastItem();

        if (item)
            urec = (UI_Refresher *)gGps->Find(item->ID_);
    }

    return urec ? urec->GetID() : FalconNullId;
}

// An empty package must not be left behind. The TE editor can get away with dropping the pointer
// (tactical_cancel_package does exactly that) because its dialog stays up holding the package for
// the next attempt, and the whole editor session is torn down afterwards. Here the failure is a
// right-click that came to nothing, and a childless package left in the database is one the ATO
// and the map both still see. Disposed the way CancelFlight disposes a flight.
static void CampaignDropPackage(Package pkg)
{
    if (not pkg)
        return;

    pkg->SetDead(1);
    pkg->Remove();
}

// A package holding one flight of `size` aircraft from `squadronID`, tasked `role` against
// whatever the last rebuild recorded as the target.
//
// This is tactical_make_package and tactical_make_flight with the list-box lookups replaced by
// arguments -- deliberately the same sequence in the same order, because the order matters:
// NewUnit before NewFlight (the flight needs its parent), FindAvailableAircraft before
// BuildMission (which reads the slots it fills in), and RecordFlightAddition after BuildMission
// succeeds. The parts of those two functions that only exist to drive TE widgets -- pilot skill
// overrides, the ATO tree, the map's current waypoint list -- are left out.
static int CampaignFilePackage(VU_ID squadronID, int role, int size)
{
    Squadron squadron = (Squadron)vuDatabase->Find(squadronID);

    if (not squadron)
        return PRET_NO_ASSETS;

    CampEntity target = (CampEntity)vuDatabase->Find(gCampPkgTargetID);

    Package pkg = (Package)NewUnit(DOMAIN_AIR, TYPE_PACKAGE, 0, 0, NULL);

    if (not pkg)
        return PRET_NO_ASSETS;

    MissionRequestClass mis;

    mis.who = CampPkgPlayerTeam();
    mis.tx = gCampPkgX;
    mis.ty = gCampPkgY;

    if (target)
    {
        mis.targetID = mis.requesterID = target->Id();
        mis.vs = target->GetTeam();

        if (target->IsObjective())
            mis.target_num = ((Objective)target)->GetBestTarget();
    }

    mis.mission = static_cast<uchar>(role);
    mis.aircraft = static_cast<uchar>(size);
    mis.tot_type = TYPE_NE;
    mis.tot = TheCampaign.CurrentTime + CampaignMinutes;

    pkg->SetUnitDestination(gCampPkgX, gCampPkgY);
    pkg->SetLocation(gCampPkgX, gCampPkgY);
    *(pkg->GetMissionRequest()) = mis;
    pkg->SetPackageFlags(MissionData[mis.mission].flags);
    pkg->SetFinal(0);
    pkg->SetOwner(CampPkgPlayerCountry());

    int tid = GetClassID(DOMAIN_AIR, CLASS_UNIT, TYPE_FLIGHT,
                         squadron->GetSType(), squadron->GetSPType(), 0, 0, 0);

    if (not tid)
    {
        CampaignDropPackage(pkg);
        return PRET_NO_ASSETS;
    }

    tid += VU_LAST_ENTITY_TYPE;

    Flight flight = NewFlight(tid, pkg, squadron);

    if (not flight)
    {
        CampaignDropPackage(pkg);
        return PRET_NO_ASSETS;
    }

    // Take off from now rather than hold a time on target: a package built by hand is wanted as
    // soon as it can fly, and the campaign clock is running while the menu is open.
    mis.tot_type = TOT_TAKEOFF;
    mis.tot = TheCampaign.CurrentTime + CampaignMinutes;
    mis.flags or_eq REQF_ALLOW_ERRORS bitor REQF_TE_MISSION;

    GridIndex hx, hy;
    squadron->GetLocation(&hx, &hy);
    flight->SetLocation(hx, hy);
    flight->SetOwner(squadron->GetOwner());
    squadron->FindAvailableAircraft(&mis);

    const int error = flight->BuildMission(&mis);

    if (error not_eq PRET_SUCCESS)
    {
        pkg->CancelFlight(flight);
        CampaignDropPackage(pkg);
        return error;
    }

    flight->SetUnitMissionTarget(mis.targetID);

    // Lock the target waypoint's time and nothing else, so the planner is free to move the rest of
    // the route around it. Same rule tactical_make_flight applies when no takeoff time was pinned.
    WayPoint w = flight->GetFirstUnitWP();
    int done = 0;

    while (w)
    {
        if ((w->GetWPFlags() bitand WPF_TARGET) and not done)
        {
            w->SetWPFlag(WPF_TIME_LOCKED);
            done = 1;
        }
        else
            w->UnSetWPFlag(WPF_TIME_LOCKED);

        w = w->GetNextWP();
    }

    pkg->RecordFlightAddition(flight, &mis, 0);
    flight->SetFinal(1);
    pkg->SetFinal(1);
    fixup_unit(flight);
    gGps->Update();

    return PRET_SUCCESS;
}

// Clicking one of the squadron slots, or one of the two flight-size radios.
static void MenuCampPackageCB(long ID, short hittype, C_Base *)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    if (ID == MID_CAMP_PKG_SIZE2 or ID == MID_CAMP_PKG_SIZE4)
    {
        gCampPkgSize = (ID == MID_CAMP_PKG_SIZE4) ? 4 : 2;
        return; // a preference, not an order: leave the menu open
    }

    const int slot = ID - MID_CAMP_PKG_SQ_FIRST;

    if (slot < 0 or slot >= CAMP_PKG_SLOTS or
        gCampPkgSquadron[slot] == FalconNullId)
        return;

    // Down before anything else happens. C_PopupList::Process does not close the menu itself --
    // every other item callback here ends with this -- and the failure path below raises a dialog
    // that would otherwise come up behind a popup still holding the mouse.
    gPopupMgr->CloseMenu();

    const int error = CampaignFilePackage(gCampPkgSquadron[slot],
                                          gCampPkgRole[slot], gCampPkgSize);

    if (error not_eq PRET_SUCCESS)
    {
        // Worth naming the two cases apart: one is "ask a different squadron", the other is "ask
        // for a later time", and a single "failed" would send you round the list for nothing.
        static _TCHAR noAssets[] =
            "That squadron has no aircraft free in this time block.";
        static _TCHAR noTiming[] =
            "The mission could not be planned for this target.";
        AreYouSure(TXT_ERROR,
                   (error == PRET_NO_ASSETS) ? noAssets : noTiming, NULL,
                   CloseWindowCB);
        return;
    }

    gMapMgr->DrawMap();
    RefreshMapOnChange();
}

// Rebuild the submenu for whatever was just right-clicked. Called from each campaign popup's open
// callback, which is the only moment both facts are known: which menu is about to be shown, and
// what it was raised over.
void CampaignPackageMenuRebuild(C_PopupList *menu, C_Base *caller)
{
    extern bool g_bCampaignAddMission;
    extern bool g_bLogCampMenu;

    if (not menu)
        return;

    C_PopupList *sub = menu->GetSubMenu(MID_CAMP_PACKAGE);

    if (not sub)
    {
        // Worth a line: this is also what an accidentally un-attached menu looks like, and it
        // is indistinguishable from a menu that deliberately does not carry the item.
        if (g_bLogCampMenu)
            FFDebugLog("[PKGMENU] no submenu on this popup -- not attached\n");

        return; // this menu does not carry the item
    }

    if (not g_bCampaignAddMission)
    {
        menu->SetItemFlagBitOn(MID_CAMP_PACKAGE, C_BIT_INVISIBLE);
        return;
    }

    int i;

    for (i = 0; i < CAMP_PKG_SLOTS; i++)
    {
        gCampPkgSquadron[i] = FalconNullId;
        gCampPkgRole[i] = 0;
        menu->SetItemFlagBitOn(MID_CAMP_PKG_SQ_FIRST + i, C_BIT_INVISIBLE);
    }

    menu->SetItemState(MID_CAMP_PKG_SIZE2, (gCampPkgSize == 2) ? 1 : 0);
    menu->SetItemState(MID_CAMP_PKG_SIZE4, (gCampPkgSize == 4) ? 1 : 0);

    gCampPkgTargetID = CampaignPopupEntityID(caller);
    CampEntity target = (CampEntity)vuDatabase->Find(gCampPkgTargetID);

    if (target)
        target->GetLocation(&gCampPkgX, &gCampPkgY);
    else
    {
        // Right-clicked bare map. Still useful -- a CAP or a recon over a point -- so take the
        // location the popup manager recorded and let GetMissionFromTarget pick a targetless role.
        short px = 0, py = 0;
        gPopupMgr->GetCurrentXY(&px, &py);
        gMapMgr->GetMapRelativeXY(&px, &py);
        const float scale = gMapMgr->GetMapScale();
        const float maxy = gMapMgr->GetMaxY();
        gCampPkgX = SimToGrid(px / scale);
        gCampPkgY = SimToGrid(maxy - py / scale);
        gCampPkgTargetID = FalconNullId;
    }

    // Collect the candidates, nearest first. Insertion into a fixed array rather than a sort of
    // the whole air list: the theater has hundreds of squadrons and only the nearest dozen will
    // fit in a menu, so there is no reason to order the rest of them.
    VU_ID bestID[CAMP_PKG_SLOTS];
    uchar bestRole[CAMP_PKG_SLOTS];
    float bestDist[CAMP_PKG_SLOTS];
    uchar bestGeneric[CAMP_PKG_SLOTS]; // 1 = the role ignores the target; sorts last
    int found = 0;

    // Counted only so the trace can say which of the three filters emptied the list. All three
    // end in the same silence otherwise -- a greyed-out parent item.
    long nSquadrons = 0, nOtherTeam = 0, nNoVehicles = 0, nNoRole = 0,
         nCandidates = 0, nGeneric = 0;
    long byTeam[NUM_TEAMS] = {0};

    const uchar myTeam = CampPkgPlayerTeam();

    VuListIterator iter(AllAirList);

    for (CampEntity e = GetFirstEntity(&iter); e; e = GetNextEntity(&iter))
    {
        if (not e->IsSquadron())
            continue;

        nSquadrons++;

        const uchar sqTeam = e->GetTeam();

        if (sqTeam < NUM_TEAMS)
            byTeam[sqTeam]++;

        if (sqTeam not_eq myTeam)
        {
            nOtherTeam++;
            continue;
        }

        Squadron sq = (Squadron)e;

        if (sq->GetTotalVehicles() < 1)
        {
            nNoVehicles++;
            continue;
        }

        const int role = GetMissionFromTarget(
            myTeam, sq->Type() - VU_LAST_ENTITY_TYPE, target);

        if (not role)
        {
            nNoRole++;
            continue; // this airframe brings nothing to this target
        }

        nCandidates++; // uncapped, unlike found -- this is what a list without twelve slots holds

        // Does this role actually engage what was clicked, or did the engine give up on the
        // target and hand back a generic sortie?
        //
        // GetMissionFromTarget does not report failure by returning 0. When the airframe cannot
        // strike the target it sets target = NULL and falls through to a list that depends only
        // on the airframe -- BARCAP, FAC, on-call CAS, AWACS, tanker, ECM, airlift. So a
        // right-click on an airbase came back offering UH-60 Airlift and MD-500 FAC, which are
        // not attacks on that airbase and are not what the twelve nearest slots are for.
        //
        // Asking the same function what it would say with no target at all separates the two
        // without inventing a rule about roles: an identical answer means the target contributed
        // nothing. Those stay on the list -- a CAP over the field you just clicked is a real
        // thing to want -- but they sort below everything that does engage it.
        const bool generic =
            target and role == GetMissionFromTarget(
                                   myTeam, sq->Type() - VU_LAST_ENTITY_TYPE,
                                   NULL);

        if (generic)
            nGeneric++;

        GridIndex sx, sy;
        sq->GetLocation(&sx, &sy);
        const float d = Distance(sx, sy, gCampPkgX, gCampPkgY);

        int at = found;

        while (at > 0 and
               (bestGeneric[at - 1] > (generic ? 1 : 0) or
                (bestGeneric[at - 1] == (generic ? 1 : 0) and
                 bestDist[at - 1] > d)))
            at--;

        if (at >= CAMP_PKG_SLOTS)
            continue;

        for (i = (found < CAMP_PKG_SLOTS ? found : CAMP_PKG_SLOTS - 1); i > at;
             i--)
        {
            bestID[i] = bestID[i - 1];
            bestRole[i] = bestRole[i - 1];
            bestDist[i] = bestDist[i - 1];
            bestGeneric[i] = bestGeneric[i - 1];
        }

        bestID[at] = sq->Id();
        bestRole[at] = static_cast<uchar>(role);
        bestDist[at] = d;
        bestGeneric[at] = static_cast<uchar>(generic ? 1 : 0);

        if (found < CAMP_PKG_SLOTS)
            found++;
    }

    for (i = 0; i < found; i++)
    {
        Squadron sq = (Squadron)vuDatabase->Find(bestID[i]);

        if (not sq)
            continue;

        _TCHAR name[48] = {0};
        sq->GetName(name, 40, FALSE);

        const _TCHAR *ac = "";
        VehicleClassDataType *vc = GetVehicleClassData(sq->GetVehicleID(0));

        if (vc)
            ac = vc->Name;

        _TCHAR label[128];
        sprintf(label, "%s  %s x%d  %.0fnm  %s", name, ac,
                (int)sq->GetTotalVehicles(), bestDist[i] * 0.5399568f,
                CampPkgRoleName(bestRole[i]));

        // The slots exist from hookup; only their text and visibility change here. SetItemLabel
        // rather than AddItem, so the callbacks attached once at hookup stay attached.
        menu->SetItemLabel(MID_CAMP_PKG_SQ_FIRST + i, label);
        menu->SetItemFlagBitOff(MID_CAMP_PKG_SQ_FIRST + i, C_BIT_INVISIBLE);
        menu->SetItemFlagBitOn(MID_CAMP_PKG_SQ_FIRST + i, C_BIT_ENABLED);

        gCampPkgSquadron[i] = bestID[i];
        gCampPkgRole[i] = bestRole[i];
    }

    // Nothing can fly this: say so on the parent rather than opening an empty submenu.
    if (found)
        menu->SetItemFlagBitOn(MID_CAMP_PACKAGE, C_BIT_ENABLED);
    else
        menu->SetItemFlagBitOff(MID_CAMP_PACKAGE, C_BIT_ENABLED);

    menu->SetItemFlagBitOff(MID_CAMP_PACKAGE, C_BIT_INVISIBLE);

    if (g_bLogCampMenu)
    {
        // The per-team tally is here because the team test is the one filter that can reject the
        // whole theater on a value that looks perfectly reasonable on its own. The trace that
        // named this bug could only say that all 112 squadrons failed the test -- which team they
        // were actually on had to be reasoned out from the code. Printing the roster means the
        // next wrong team is read rather than deduced.
        _TCHAR hist[96];
        int at = 0;

        for (int t = 0; t < NUM_TEAMS; t++)
            if (byTeam[t])
                at += sprintf(&hist[at], "%s%d:%ld", at ? " " : "", t,
                              byTeam[t]);

        if (not at)
            strcpy(hist, "none");

        _TCHAR line[420];
        sprintf(line,
                "[PKGMENU] myTeam=%d (session country=%d, gSelectedTeam=%d) "
                "target=%s type=%d tgtTeam=%d | squadrons=%ld by team [%s] | "
                "otherTeam=%ld noVehicles=%ld noRole=%ld | candidates=%ld "
                "(engage target=%ld, generic=%ld) | shown=%d of %d slots -> "
                "%s\n",
                (int)myTeam,
                FalconLocalSession ? (int)FalconLocalSession->GetCountry() : -1,
                (int)gSelectedTeam,
                target ? (target->IsObjective()
                              ? "objective"
                              : (target->IsUnit() ? "unit" : "other"))
                       : "none(bare map)",
                target ? (int)target->GetType() : -1,
                target ? (int)target->GetTeam() : -1, nSquadrons, hist,
                nOtherTeam, nNoVehicles, nNoRole, nCandidates,
                nCandidates - nGeneric, nGeneric, found, CAMP_PKG_SLOTS,
                found ? "ENABLED" : "greyed out");
        FFDebugLog(line);
    }
}

// Attach the item and its fixed slots to one menu. Called once per campaign popup at hookup.
void CampaignPackageMenuAttach(C_PopupList *menu)
{
    if (not menu)
        return;

    static _TCHAR lblPkg[] = "Build package";
    static _TCHAR lbl2[] = "Two ship";
    static _TCHAR lbl4[] = "Four ship";
    static _TCHAR lblEmpty[] = " ";

    if (not menu->AddItem(MID_CAMP_PACKAGE, C_TYPE_MENU, lblPkg, 0))
        return;

    menu->AddItem(MID_CAMP_PKG_SIZE2, C_TYPE_RADIO, lbl2, MID_CAMP_PACKAGE);
    menu->AddItem(MID_CAMP_PKG_SIZE4, C_TYPE_RADIO, lbl4, MID_CAMP_PACKAGE);
    menu->SetItemGroup(MID_CAMP_PKG_SIZE2, MID_CAMP_PKG_SIZE_GROUP);
    menu->SetItemGroup(MID_CAMP_PKG_SIZE4, MID_CAMP_PKG_SIZE_GROUP);
    menu->SetCallback(MID_CAMP_PKG_SIZE2, MenuCampPackageCB);
    menu->SetCallback(MID_CAMP_PKG_SIZE4, MenuCampPackageCB);
    menu->SetItemState(MID_CAMP_PKG_SIZE2, 1);

    // A popup separator is an item of no type carrying no label -- that is what the menu
    // resource's own MID_SEP_* entries are, and AddItem has an explicit exemption for it.
    menu->AddItem(MID_CAMP_PKG_SEP, C_TYPE_NOTHING, (_TCHAR *)NULL,
                  MID_CAMP_PACKAGE);

    // The squadron rows are created empty and hidden. Their labels are rewritten on every open,
    // but a callback can only be attached to an item that exists, and attaching twelve of them per
    // right-click would be twelve list walks for no gain.
    for (int i = 0; i < CAMP_PKG_SLOTS; i++)
    {
        menu->AddItem(MID_CAMP_PKG_SQ_FIRST + i, C_TYPE_ITEM, lblEmpty,
                      MID_CAMP_PACKAGE);
        menu->SetCallback(MID_CAMP_PKG_SQ_FIRST + i, MenuCampPackageCB);
        menu->SetItemFlagBitOn(MID_CAMP_PKG_SQ_FIRST + i, C_BIT_INVISIBLE);
    }
}

// Artscout - 2026: the stock "Add Flight" / "Add Package" items stay hidden in the campaign.
//
// They were un-hidden here first, on the reasoning that the machinery behind them is complete --
// and it is, but it is reached through PACKAGE_WIN and TAC_FLIGHT_WIN, and tactical_add_package
// finds no window. Every use of it is guarded on FindWindow being non-NULL, so the item came up
// and did nothing at all. A menu entry that silently does nothing is worse than no entry.
//
// This comment used to say that was a campaign problem -- that te_scf.lst carries those windows
// and only the Tactical Engagement screen loads it -- and that the items therefore still work on
// the TE screen. That was wrong about PACKAGE_WIN, and it is worth saying so here rather than
// leaving the correction in a commit message. te_scf.lst does NOT carry it: art\taceng\package.scf
// is named by art\tenew_scf.lst alone, and nothing in this source tree loads tenew_scf.lst. The
// window was unreachable everywhere, TE included. campaign.cpp now loads it (CampaignPackageWindow)
// along with the FF4 skin its art lives in; until that is wired through to these items, they stay
// hidden here.
//
// "Build package" (CampaignPackageMenuAttach, above) needs neither window. These two stay hidden;
// MenuAddUnitCB is still wired to them for the Tactical Engagement screen, where TAC_FLIGHT_WIN
// at least does load.
static void CampaignMissionItems(C_PopupList *menu)
{
    extern bool g_bCampaignPackageWindow;

    if (not menu)
        return;

    // Add Package comes back when campaign.cpp has loaded PACKAGE_WIN, and only then. Everything
    // behind it was always there -- MenuAddUnitCB resolves the clicked entity and hands it to
    // tactical_add_package, which handles a _CNTL_POPUPLIST_ caller by design. The item was dead
    // for one reason: the window did not exist to be found.
    if (g_bCampaignPackageWindow)
        menu->SetItemFlagBitOff(MID_ADD_PACKAGE, C_BIT_INVISIBLE);
    else
        menu->SetItemFlagBitOn(MID_ADD_PACKAGE, C_BIT_INVISIBLE);

    // Add Flight stays hidden. Its standalone path builds a package implicitly and defaults the
    // flight to "start at target", which is a different flow from the one asked for here; reaching
    // a flight through Add Package's own Add Flight button is the path that gives a package
    // several flights with their own targets.
    menu->SetItemFlagBitOn(MID_ADD_FLIGHT, C_BIT_INVISIBLE);
}

void SetupCampaignMenus()
{
    C_PopupList *menu;

    GameType = 1;
    EditMode = 0;
    // Map Menu
    menu = gPopupMgr->GetMenu(MAP_POP);

    if (menu)
    {
        // Add Package was already left visible here by a previous author ("sfr: add
        // package"), so right-clicking empty map in campaign has offered it for some time.
        // Add Flight was still hidden; put both on the same switch.
        CampaignMissionItems(menu);
        menu->SetItemFlagBitOn(MID_ADD_BATTALION, C_BIT_INVISIBLE);
        menu->SetItemFlagBitOn(MID_SHOW_VC, C_BIT_INVISIBLE);
    }

    // Objective Menu
    menu = gPopupMgr->GetMenu(OBJECTIVE_POP);

    if (menu)
    {
        CampaignMissionItems(menu);
        menu->SetItemFlagBitOn(MID_ADD_BATTALION, C_BIT_INVISIBLE);
        menu->SetItemFlagBitOn(MID_ADD_VC, C_BIT_INVISIBLE);
        menu->SetItemFlagBitOn(MID_SET_OWNER, C_BIT_INVISIBLE);
        menu->SetItemFlagBitOn(MID_SQUADRONS, C_BIT_INVISIBLE);
    }

    // Unit Menu
    menu = gPopupMgr->GetMenu(UNIT_POP);

    if (menu)
    {
        menu->SetItemFlagBitOn(MID_DELETE_UNIT, C_BIT_INVISIBLE);
        CampaignMissionItems(menu);
        menu->SetItemFlagBitOn(MID_ADD_BATTALION, C_BIT_INVISIBLE);
        menu->SetItemFlagBitOn(MID_ADD_VC, C_BIT_INVISIBLE);
        menu->SetItemFlagBitOn(MID_SET_OWNER, C_BIT_INVISIBLE);
    }

    // Unit Menu
    menu = gPopupMgr->GetMenu(NAVAL_POP);

    if (menu)
    {
        menu->SetItemFlagBitOn(MID_DELETE_UNIT, C_BIT_INVISIBLE);
        CampaignMissionItems(menu);
        menu->SetItemFlagBitOn(MID_ADD_BATTALION, C_BIT_INVISIBLE);
        menu->SetItemFlagBitOn(MID_ADD_VC, C_BIT_INVISIBLE);
        menu->SetItemFlagBitOn(MID_SET_OWNER, C_BIT_INVISIBLE);
    }

    // Unit Menu
    menu = gPopupMgr->GetMenu(AIRUNIT_MENU);

    if (menu)
    {
        menu->SetItemFlagBitOn(MID_DELETE_UNIT, C_BIT_INVISIBLE);
        CampaignMissionItems(menu);
        menu->SetItemFlagBitOn(MID_ADD_VC, C_BIT_INVISIBLE);
        menu->SetItemFlagBitOn(MID_SET_OWNER, C_BIT_INVISIBLE);
    }

    // Package Menu
    menu = gPopupMgr->GetMenu(PACKAGE_POP);

    if (menu)
    {
        menu->SetItemFlagBitOn(MID_DELETE_UNIT, C_BIT_INVISIBLE);
    }
}

// Mode 0=Play,1=Edit)
void SetupTacEngMenus(short Mode)
{
    C_PopupList *menu;

    GameType = 2;
    EditMode = Mode;

    // Map Menu
    menu = gPopupMgr->GetMenu(MAP_POP);

    if (menu)
    {
        menu->SetItemFlagBitOff(MID_ADD_FLIGHT, C_BIT_INVISIBLE);
        menu->SetItemFlagBitOff(MID_ADD_PACKAGE, C_BIT_INVISIBLE);
        menu->SetItemFlagBitOn(MID_ADD_FLIGHT, C_BIT_ENABLED);
        menu->SetItemFlagBitOn(MID_ADD_PACKAGE, C_BIT_ENABLED);

        menu->SetItemFlagBitOff(MID_SHOW_VC, C_BIT_INVISIBLE);
    }

    // Objective Menu
    menu = gPopupMgr->GetMenu(OBJECTIVE_POP);

    if (menu)
    {
        menu->SetItemFlagBitOff(MID_ADD_FLIGHT, C_BIT_INVISIBLE);
        menu->SetItemFlagBitOff(MID_ADD_PACKAGE, C_BIT_INVISIBLE);
        menu->SetItemFlagBitOn(MID_ADD_FLIGHT, C_BIT_ENABLED);
        menu->SetItemFlagBitOn(MID_ADD_PACKAGE, C_BIT_ENABLED);

        menu->SetItemFlagBitOff(MID_ADD_VC, C_BIT_INVISIBLE);
        menu->SetItemFlagBitOff(MID_SET_OWNER, C_BIT_INVISIBLE);
        menu->SetItemFlagBitOn(MID_SQUADRONS, C_BIT_INVISIBLE);

        if (EditMode)
        {
            menu->SetItemFlagBitOn(MID_ADD_VC, C_BIT_ENABLED);
            menu->SetItemFlagBitOn(MID_SET_OWNER, C_BIT_ENABLED);
            menu->SetItemFlagBitOff(MID_ADD_BATTALION, C_BIT_INVISIBLE);
        }
        else
        {
            menu->SetItemFlagBitOff(MID_ADD_VC, C_BIT_ENABLED);
            menu->SetItemFlagBitOff(MID_SET_OWNER, C_BIT_ENABLED);
            menu->SetItemFlagBitOn(MID_ADD_BATTALION, C_BIT_INVISIBLE);
        }
    }

    // Unit Menu
    menu = gPopupMgr->GetMenu(UNIT_POP);

    if (menu)
    {
        menu->SetItemFlagBitOff(MID_ADD_FLIGHT, C_BIT_INVISIBLE);
        menu->SetItemFlagBitOff(MID_ADD_PACKAGE, C_BIT_INVISIBLE);
        menu->SetItemFlagBitOn(MID_ADD_FLIGHT, C_BIT_ENABLED);
        menu->SetItemFlagBitOn(MID_ADD_PACKAGE, C_BIT_ENABLED);

        menu->SetItemFlagBitOff(MID_DELETE_UNIT, C_BIT_INVISIBLE);

        if (EditMode)
        {
            menu->SetItemFlagBitOn(MID_DELETE_UNIT, C_BIT_ENABLED);
            menu->SetItemFlagBitOn(MID_ADD_VC, C_BIT_ENABLED);
            menu->SetItemFlagBitOn(MID_SET_OWNER, C_BIT_ENABLED);
            menu->SetItemFlagBitOff(MID_ADD_BATTALION, C_BIT_INVISIBLE);
        }
        else
        {
            menu->SetItemFlagBitOff(MID_DELETE_UNIT, C_BIT_ENABLED);
            menu->SetItemFlagBitOff(MID_ADD_VC, C_BIT_ENABLED);
            menu->SetItemFlagBitOff(MID_SET_OWNER, C_BIT_ENABLED);
            menu->SetItemFlagBitOn(MID_ADD_BATTALION, C_BIT_INVISIBLE);
        }
    }

    // Unit Menu
    menu = gPopupMgr->GetMenu(NAVAL_POP);

    if (menu)
    {
        menu->SetItemFlagBitOff(MID_ADD_FLIGHT, C_BIT_INVISIBLE);
        menu->SetItemFlagBitOff(MID_ADD_PACKAGE, C_BIT_INVISIBLE);
        menu->SetItemFlagBitOn(MID_ADD_FLIGHT, C_BIT_ENABLED);
        menu->SetItemFlagBitOn(MID_ADD_PACKAGE, C_BIT_ENABLED);

        menu->SetItemFlagBitOff(MID_DELETE_UNIT, C_BIT_INVISIBLE);

        if (EditMode)
        {
            menu->SetItemFlagBitOn(MID_DELETE_UNIT, C_BIT_ENABLED);
            menu->SetItemFlagBitOn(MID_ADD_VC, C_BIT_ENABLED);
            menu->SetItemFlagBitOn(MID_SET_OWNER, C_BIT_ENABLED);
            menu->SetItemFlagBitOff(MID_ADD_BATTALION, C_BIT_INVISIBLE);
        }
        else
        {
            menu->SetItemFlagBitOff(MID_DELETE_UNIT, C_BIT_ENABLED);
            menu->SetItemFlagBitOff(MID_ADD_VC, C_BIT_ENABLED);
            menu->SetItemFlagBitOff(MID_SET_OWNER, C_BIT_ENABLED);
            menu->SetItemFlagBitOn(MID_ADD_BATTALION, C_BIT_INVISIBLE);
        }
    }

    // Unit Menu
    menu = gPopupMgr->GetMenu(AIRUNIT_MENU);

    if (menu)
    {
        menu->SetItemFlagBitOff(MID_ADD_FLIGHT, C_BIT_INVISIBLE);
        menu->SetItemFlagBitOff(MID_ADD_PACKAGE, C_BIT_INVISIBLE);
        menu->SetItemFlagBitOn(MID_ADD_FLIGHT, C_BIT_ENABLED);
        menu->SetItemFlagBitOn(MID_ADD_PACKAGE, C_BIT_ENABLED);

        menu->SetItemFlagBitOff(MID_DELETE_UNIT, C_BIT_INVISIBLE);

        if (EditMode)
        {
            menu->SetItemFlagBitOn(MID_DELETE_UNIT, C_BIT_ENABLED);
            menu->SetItemFlagBitOn(MID_ADD_VC, C_BIT_ENABLED);
            menu->SetItemFlagBitOn(MID_SET_OWNER, C_BIT_ENABLED);
            menu->SetItemFlagBitOff(MID_ADD_BATTALION, C_BIT_INVISIBLE);
        }
        else
        {
            menu->SetItemFlagBitOff(MID_DELETE_UNIT, C_BIT_ENABLED);
            menu->SetItemFlagBitOff(MID_ADD_VC, C_BIT_ENABLED);
            menu->SetItemFlagBitOff(MID_SET_OWNER, C_BIT_ENABLED);
            menu->SetItemFlagBitOn(MID_ADD_BATTALION, C_BIT_INVISIBLE);
        }
    }

    // Package Menu
    menu = gPopupMgr->GetMenu(PACKAGE_POP);

    if (menu)
    {
        menu->SetItemFlagBitOff(MID_DELETE_UNIT, C_BIT_INVISIBLE);

        if (EditMode)
            menu->SetItemFlagBitOn(MID_DELETE_UNIT, C_BIT_ENABLED);
        else
            menu->SetItemFlagBitOff(MID_DELETE_UNIT, C_BIT_ENABLED);
    }

    // VC Menu
    menu = gPopupMgr->GetMenu(VC_POP);

    if (menu)
    {
        if (EditMode)
        {
            menu->SetItemFlagBitOn(MID_DELETE_UNIT, C_BIT_ENABLED);
            menu->SetItemFlagBitOn(MID_SET_OWNER, C_BIT_ENABLED);
        }
        else
        {
            menu->SetItemFlagBitOff(MID_DELETE_UNIT, C_BIT_ENABLED);
            menu->SetItemFlagBitOff(MID_SET_OWNER, C_BIT_ENABLED);
        }
    }
}

void MapMenuOpenCB(C_Base *themenu, C_Base *caller)
{
    C_PopupList *menu;

    if (not themenu or not caller or not caller->Parent_)
        return;

    menu = (C_PopupList *)themenu;

    CampaignPackageMenuRebuild(menu, caller);

    // Enable certain stuff for TE VC window
    if (caller->Parent_->GetID() == TAC_VC_WIN)
    {
        menu->SetItemFlagBitOn(MID_ADD_VC, C_BIT_ENABLED);
        menu->SetItemFlagBitOff(MID_ADD_FLIGHT, C_BIT_ENABLED);
        menu->SetItemFlagBitOff(MID_ADD_PACKAGE, C_BIT_ENABLED);
        menu->SetItemFlagBitOff(MID_ADD_BATTALION, C_BIT_ENABLED);
    }
    else
    {
        if (EditMode)
        {
            menu->SetItemFlagBitOn(MID_ADD_VC, C_BIT_ENABLED);
            menu->SetItemFlagBitOn(MID_ADD_FLIGHT, C_BIT_ENABLED);
            menu->SetItemFlagBitOn(MID_ADD_PACKAGE, C_BIT_ENABLED);
            menu->SetItemFlagBitOn(MID_ADD_BATTALION, C_BIT_ENABLED);
        }
        else
        {
            menu->SetItemFlagBitOff(MID_ADD_VC, C_BIT_ENABLED);
            menu->SetItemFlagBitOff(MID_ADD_FLIGHT, C_BIT_ENABLED);
            menu->SetItemFlagBitOn(MID_ADD_PACKAGE, C_BIT_ENABLED);
            menu->SetItemFlagBitOff(MID_ADD_BATTALION, C_BIT_ENABLED);
        }
    }
}

void OpenUnitMenuCB(C_Base *themenu, C_Base *caller)
{
    C_PopupList *menu;

    if (not themenu or not caller or not caller->Parent_)
        return;

    menu = (C_PopupList *)themenu;

    CampaignPackageMenuRebuild(menu, caller);

    if (menu)
    {
        if (TeamInfo[1])
        {
            menu->SetItemLabel(MID_TEAM_1, TeamInfo[1]->GetName());
            menu->SetItemFlagBitOff(MID_TEAM_1, C_BIT_INVISIBLE);
        }
        else
            menu->SetItemFlagBitOn(MID_TEAM_1, C_BIT_INVISIBLE);

        if (TeamInfo[2])
        {
            menu->SetItemLabel(MID_TEAM_2, TeamInfo[2]->GetName());
            menu->SetItemFlagBitOff(MID_TEAM_2, C_BIT_INVISIBLE);
        }
        else
            menu->SetItemFlagBitOn(MID_TEAM_2, C_BIT_INVISIBLE);

        if (TeamInfo[3])
        {
            menu->SetItemLabel(MID_TEAM_3, TeamInfo[3]->GetName());
            menu->SetItemFlagBitOff(MID_TEAM_3, C_BIT_INVISIBLE);
        }
        else
            menu->SetItemFlagBitOn(MID_TEAM_3, C_BIT_INVISIBLE);

        if (TeamInfo[4])
        {
            menu->SetItemLabel(MID_TEAM_4, TeamInfo[4]->GetName());
            menu->SetItemFlagBitOff(MID_TEAM_4, C_BIT_INVISIBLE);
        }
        else
            menu->SetItemFlagBitOn(MID_TEAM_4, C_BIT_INVISIBLE);

        if (TeamInfo[5])
        {
            menu->SetItemLabel(MID_TEAM_5, TeamInfo[5]->GetName());
            menu->SetItemFlagBitOff(MID_TEAM_5, C_BIT_INVISIBLE);
        }
        else
            menu->SetItemFlagBitOn(MID_TEAM_5, C_BIT_INVISIBLE);

        if (TeamInfo[6])
        {
            menu->SetItemLabel(MID_TEAM_6, TeamInfo[6]->GetName());
            menu->SetItemFlagBitOff(MID_TEAM_6, C_BIT_INVISIBLE);
        }
        else
            menu->SetItemFlagBitOn(MID_TEAM_6, C_BIT_INVISIBLE);

        if (TeamInfo[7])
        {
            menu->SetItemLabel(MID_TEAM_7, TeamInfo[7]->GetName());
            menu->SetItemFlagBitOff(MID_TEAM_7, C_BIT_INVISIBLE);
        }
        else
            menu->SetItemFlagBitOn(MID_TEAM_7, C_BIT_INVISIBLE);
    }
}

void OpenNavalMenuCB(C_Base *themenu, C_Base *caller)
{
    C_PopupList *menu;

    if (not themenu or not caller or not caller->Parent_)
        return;

    menu = (C_PopupList *)themenu;

    CampaignPackageMenuRebuild(menu, caller);

    if (menu)
    {
        if (TeamInfo[1])
        {
            menu->SetItemLabel(MID_TEAM_1, TeamInfo[1]->GetName());
            menu->SetItemFlagBitOff(MID_TEAM_1, C_BIT_INVISIBLE);
        }
        else
            menu->SetItemFlagBitOn(MID_TEAM_1, C_BIT_INVISIBLE);

        if (TeamInfo[2])
        {
            menu->SetItemLabel(MID_TEAM_2, TeamInfo[2]->GetName());
            menu->SetItemFlagBitOff(MID_TEAM_2, C_BIT_INVISIBLE);
        }
        else
            menu->SetItemFlagBitOn(MID_TEAM_2, C_BIT_INVISIBLE);

        if (TeamInfo[3])
        {
            menu->SetItemLabel(MID_TEAM_3, TeamInfo[3]->GetName());
            menu->SetItemFlagBitOff(MID_TEAM_3, C_BIT_INVISIBLE);
        }
        else
            menu->SetItemFlagBitOn(MID_TEAM_3, C_BIT_INVISIBLE);

        if (TeamInfo[4])
        {
            menu->SetItemLabel(MID_TEAM_4, TeamInfo[4]->GetName());
            menu->SetItemFlagBitOff(MID_TEAM_4, C_BIT_INVISIBLE);
        }
        else
            menu->SetItemFlagBitOn(MID_TEAM_4, C_BIT_INVISIBLE);

        if (TeamInfo[5])
        {
            menu->SetItemLabel(MID_TEAM_5, TeamInfo[5]->GetName());
            menu->SetItemFlagBitOff(MID_TEAM_5, C_BIT_INVISIBLE);
        }
        else
            menu->SetItemFlagBitOn(MID_TEAM_5, C_BIT_INVISIBLE);

        if (TeamInfo[6])
        {
            menu->SetItemLabel(MID_TEAM_6, TeamInfo[6]->GetName());
            menu->SetItemFlagBitOff(MID_TEAM_6, C_BIT_INVISIBLE);
        }
        else
            menu->SetItemFlagBitOn(MID_TEAM_6, C_BIT_INVISIBLE);

        if (TeamInfo[7])
        {
            menu->SetItemLabel(MID_TEAM_7, TeamInfo[7]->GetName());
            menu->SetItemFlagBitOff(MID_TEAM_7, C_BIT_INVISIBLE);
        }
        else
            menu->SetItemFlagBitOn(MID_TEAM_7, C_BIT_INVISIBLE);
    }
}

void ObjMenuOpenCB(C_Base *themenu, C_Base *caller)
{
    C_PopupList *menu;
    bool isAirbase = false;

    if (not themenu or not caller or not caller->Parent_)
        return;

    if (caller->_GetCType_() == _CNTL_DRAWLIST_)
    {
        MAPICONLIST *icon;

        icon = ((C_DrawList *)caller)->GetLastItem();

        if (icon and icon->Owner)
        {
            if (icon->Owner->GetType() == _OBTV_AIR_FIELDS)
            {
                isAirbase = true;
            }
        }
    }
    else if (caller->_GetCType_() == _CNTL_MAPICON_)
    {
        if (caller->GetType() == _OBTV_AIR_FIELDS)
        {
            isAirbase = true;
        }
    }
    else if (caller->_GetCType_() == _CNTL_TREELIST_)
    {
    }

    menu = (C_PopupList *)themenu;

    CampaignPackageMenuRebuild(menu, caller);

    if (isAirbase) // Airbase
    {
        menu->SetItemFlagBitOn(MID_SQUADRONS, C_BIT_ENABLED);
        menu->SetItemFlagBitOff(MID_SQUADRONS, C_BIT_INVISIBLE);

        if (GameType == 1 or not EditMode)
            menu->SetItemFlagBitOff(MID_ADD_SQUADRON, C_BIT_ENABLED);
        else
            menu->SetItemFlagBitOn(MID_ADD_SQUADRON, C_BIT_ENABLED);
    }
    else
    {
        menu->SetItemFlagBitOff(MID_SQUADRONS, C_BIT_ENABLED);
        menu->SetItemFlagBitOn(MID_SQUADRONS, C_BIT_INVISIBLE);
        menu->SetItemFlagBitOff(MID_ADD_SQUADRON, C_BIT_ENABLED);
    }

    if (TeamInfo[1])
    {
        menu->SetItemLabel(MID_TEAM_1, TeamInfo[1]->GetName());
        menu->SetItemFlagBitOff(MID_TEAM_1, C_BIT_INVISIBLE);
    }
    else
        menu->SetItemFlagBitOn(MID_TEAM_1, C_BIT_INVISIBLE);

    if (TeamInfo[2])
    {
        menu->SetItemLabel(MID_TEAM_2, TeamInfo[2]->GetName());
        menu->SetItemFlagBitOff(MID_TEAM_2, C_BIT_INVISIBLE);
    }
    else
        menu->SetItemFlagBitOn(MID_TEAM_2, C_BIT_INVISIBLE);

    if (TeamInfo[3])
    {
        menu->SetItemLabel(MID_TEAM_3, TeamInfo[3]->GetName());
        menu->SetItemFlagBitOff(MID_TEAM_3, C_BIT_INVISIBLE);
    }
    else
        menu->SetItemFlagBitOn(MID_TEAM_3, C_BIT_INVISIBLE);

    if (TeamInfo[4])
    {
        menu->SetItemLabel(MID_TEAM_4, TeamInfo[4]->GetName());
        menu->SetItemFlagBitOff(MID_TEAM_4, C_BIT_INVISIBLE);
    }
    else
        menu->SetItemFlagBitOn(MID_TEAM_4, C_BIT_INVISIBLE);

    if (TeamInfo[5])
    {
        menu->SetItemLabel(MID_TEAM_5, TeamInfo[5]->GetName());
        menu->SetItemFlagBitOff(MID_TEAM_5, C_BIT_INVISIBLE);
    }
    else
        menu->SetItemFlagBitOn(MID_TEAM_5, C_BIT_INVISIBLE);

    if (TeamInfo[6])
    {
        menu->SetItemLabel(MID_TEAM_6, TeamInfo[6]->GetName());
        menu->SetItemFlagBitOff(MID_TEAM_6, C_BIT_INVISIBLE);
    }
    else
        menu->SetItemFlagBitOn(MID_TEAM_6, C_BIT_INVISIBLE);

    if (TeamInfo[7])
    {
        menu->SetItemLabel(MID_TEAM_7, TeamInfo[7]->GetName());
        menu->SetItemFlagBitOff(MID_TEAM_7, C_BIT_INVISIBLE);
    }
    else
        menu->SetItemFlagBitOn(MID_TEAM_7, C_BIT_INVISIBLE);
}

void HookupCampaignMenus()
{
    C_PopupList *menu;
    int i;

    menu = gPopupMgr->GetMenu(MAP_POP);

    if (menu)
    {
        menu->SetOpenCallback(MapMenuOpenCB);

        menu->SetCallback(MID_RECON, MenuReconCB);
        menu->SetCallback(MID_ADD_FLIGHT, MenuAddUnitCB);
        menu->SetCallback(MID_ADD_PACKAGE, MenuAddUnitCB);
        menu->SetCallback(MID_ADD_BATTALION, MenuAddUnitCB);
        menu->SetCallback(MID_ADD_SQUADRON, MenuAddUnitCB);
        // Legend stuff
        menu->SetCallback(MID_LEG_NAMES, MenuToggleNamesCB);
        menu->SetCallback(MID_LEG_BULLSEYE, MenuToggleBullseyeCB);

        // Objectives
        menu->SetCallback(MID_INST_AF, MenuToggleObjectiveCB);
        menu->SetCallback(MID_INST_AD, MenuToggleObjectiveCB);
        menu->SetCallback(MID_INST_ARMY, MenuToggleObjectiveCB);
        menu->SetCallback(MID_INST_CCC, MenuToggleObjectiveCB);
        menu->SetCallback(MID_INST_POLITICAL, MenuToggleObjectiveCB);
        menu->SetCallback(MID_INST_INFRA, MenuToggleObjectiveCB);
        menu->SetCallback(MID_INST_LOG, MenuToggleObjectiveCB);
        menu->SetCallback(MID_INST_WARPROD, MenuToggleObjectiveCB);
        menu->SetCallback(MID_INST_NAV, MenuToggleObjectiveCB);
        menu->SetCallback(MID_INST_OTHER, MenuToggleObjectiveCB);
        menu->SetCallback(MID_INST_NAVAL, MenuToggleObjectiveCB);
        menu->SetCallback(MID_SHOW_VC, MenuToggleObjectiveCB);

        // Units
        menu->SetCallback(MID_UNITS_SQUAD_SQUADRON, MenuToggleUnitCB);
        menu->SetCallback(MID_UNITS_SQUAD_PACKAGE, MenuToggleUnitCB);
        menu->SetCallback(MID_UNITS_DIV, MenuToggleUnitCB);
        menu->SetCallback(MID_UNITS_BRIG, MenuToggleUnitCB);
        menu->SetCallback(MID_UNITS_BAT, MenuToggleUnitCB);
        menu->SetCallback(MID_UNITS_COMBAT, MenuToggleUnitCB);
        menu->SetCallback(MID_UNITS_AD, MenuToggleUnitCB);
        menu->SetCallback(MID_UNITS_SUPPORT, MenuToggleUnitCB);
        menu->SetCallback(MID_UNITS_SQUAD_FIGHTER, MenuToggleUnitCB);
        menu->SetCallback(MID_UNITS_SQUAD_FIGHTBOMB, MenuToggleUnitCB);
        menu->SetCallback(MID_UNITS_SQUAD_ATTACK, MenuToggleUnitCB);
        menu->SetCallback(MID_UNITS_SQUAD_BOMBER, MenuToggleUnitCB);
        menu->SetCallback(MID_UNITS_SQUAD_SUPPORT, MenuToggleUnitCB);
        menu->SetCallback(MID_UNITS_SQUAD_HELI, MenuToggleUnitCB);

        if (g_nUnidentifiedInUI)
            menu->SetCallback(
                MID_UNITS_SQUAD_UNKNOWN,
                MenuToggleUnitCB); // 2002-02-21 ADDED BY S.G. For 'Unknown' type of flight

        menu->SetCallback(MID_UNITS_NAVY_COMBAT, MenuToggleUnitCB);
        menu->SetCallback(MID_UNITS_NAVY_SUPPLY, MenuToggleUnitCB);

        // Sams/Radar
        menu->SetCallback(MID_OFF, MenuSetCirclesCB);
        menu->SetCallback(MID_CIRCLE_SAM_LOW, MenuSetCirclesCB);
        menu->SetCallback(MID_CIRCLE_SAM_HIGH, MenuSetCirclesCB);
        menu->SetCallback(MID_CIRCLE_RADAR_LOW, MenuSetCirclesCB);
        menu->SetCallback(MID_CIRCLE_RADAR_HIGH, MenuSetCirclesCB);

        // Artscout - 2026: the Logistics submenu. These items have no entry in the menu resource --
        // that is game data we do not ship -- so build them here. C_PopupList::AddItem takes a plain
        // label and the submenu it creates inherits this menu's font, colours and check icon, so an
        // item added in code is indistinguishable from one the resource loaded. Each needs its own
        // radio group, or Process would clear the threat rings' marks along with its own.
        static _TCHAR lblLayers[] = "Logistics";
        static _TCHAR lblOff[] = "None";
        static _TCHAR lblPower[] = "Power coverage";
        static _TCHAR lblSupply[] = "Supply flow";
        static _TCHAR lblProd[] = "Production";
        static _TCHAR lblDmg[] = "Target damage";

        if (menu->AddItem(MID_CAMP_LAYERS, C_TYPE_MENU, lblLayers, 0))
        {
            menu->AddItem(MID_CAMP_LAYER_OFF, C_TYPE_RADIO, lblOff,
                          MID_CAMP_LAYERS);
            menu->AddItem(MID_CAMP_LAYER_POWER, C_TYPE_RADIO, lblPower,
                          MID_CAMP_LAYERS);
            menu->AddItem(MID_CAMP_LAYER_SUPPLY, C_TYPE_RADIO, lblSupply,
                          MID_CAMP_LAYERS);
            menu->AddItem(MID_CAMP_LAYER_PROD, C_TYPE_RADIO, lblProd,
                          MID_CAMP_LAYERS);
            menu->AddItem(MID_CAMP_LAYER_DAMAGE, C_TYPE_RADIO, lblDmg,
                          MID_CAMP_LAYERS);

            menu->SetItemGroup(MID_CAMP_LAYER_OFF, MID_CAMP_LAYER_GROUP);
            menu->SetItemGroup(MID_CAMP_LAYER_POWER, MID_CAMP_LAYER_GROUP);
            menu->SetItemGroup(MID_CAMP_LAYER_SUPPLY, MID_CAMP_LAYER_GROUP);
            menu->SetItemGroup(MID_CAMP_LAYER_PROD, MID_CAMP_LAYER_GROUP);
            menu->SetItemGroup(MID_CAMP_LAYER_DAMAGE, MID_CAMP_LAYER_GROUP);

            menu->SetCallback(MID_CAMP_LAYER_OFF, MenuSetCampLayerCB);
            menu->SetCallback(MID_CAMP_LAYER_POWER, MenuSetCampLayerCB);
            menu->SetCallback(MID_CAMP_LAYER_SUPPLY, MenuSetCampLayerCB);
            menu->SetCallback(MID_CAMP_LAYER_PROD, MenuSetCampLayerCB);
            menu->SetCallback(MID_CAMP_LAYER_DAMAGE, MenuSetCampLayerCB);

            menu->SetItemState(MID_CAMP_LAYER_OFF, 1);
        }

        // Artscout - 2026: the FLOT toggle, a sibling of Names and Bullseye rather than a layer.
        //
        // The row is always here; CampFlotLine is the state it starts in, not whether it exists.
        // Hiding the row when the knob was off made "default off" and "not available" the same
        // thing, which would have left no way to switch it on.
        {
            extern bool g_bCampFlotLine;
            static _TCHAR lblFlot[] = "FLOT line";

            if (menu->AddItem(MID_CAMP_FLOT, C_TYPE_TOGGLE, lblFlot, 0))
            {
                menu->SetCallback(MID_CAMP_FLOT, MenuToggleFlotCB);
                menu->SetItemState(MID_CAMP_FLOT, g_bCampFlotLine ? 1 : 0);
            }
        }

        CampaignPackageMenuAttach(menu);
    }

    menu = gPopupMgr->GetMenu(OBJECTIVE_POP);

    if (menu)
    {
        menu->SetOpenCallback(ObjMenuOpenCB);

        menu->SetCallback(MID_RECON, MenuReconCB);
        menu->SetCallback(MID_STATUS, MenuStatusCB);
        menu->SetCallback(MID_ADD_FLIGHT, MenuAddUnitCB);
        menu->SetCallback(MID_ADD_PACKAGE, MenuAddUnitCB);
        menu->SetCallback(MID_ADD_BATTALION, MenuAddUnitCB);
        menu->SetCallback(MID_ADD_SQUADRON, MenuAddUnitCB);
        menu->SetCallback(MID_ADD_VC, MenuAddVCCB);
        CampaignPackageMenuAttach(
            menu); // Artscout - 2026: the campaign's own package builder
        menu->SetCallback(MID_TEAM_0, MenuSetOwnerCB);
        menu->SetCallback(MID_TEAM_1, MenuSetOwnerCB);
        menu->SetCallback(MID_TEAM_2, MenuSetOwnerCB);
        menu->SetCallback(MID_TEAM_3, MenuSetOwnerCB);
        menu->SetCallback(MID_TEAM_4, MenuSetOwnerCB);
        menu->SetCallback(MID_TEAM_5, MenuSetOwnerCB);
        menu->SetCallback(MID_TEAM_6, MenuSetOwnerCB);
        menu->SetCallback(MID_TEAM_7, MenuSetOwnerCB);
    }

    menu = gPopupMgr->GetMenu(SQUADRON_POP);

    if (menu)
    {
        menu->SetCallback(MID_RECON, MenuReconCB);
        menu->SetCallback(MID_ADD_FLIGHT, MenuAddUnitCB);
        menu->SetCallback(MID_ADD_PACKAGE, MenuAddUnitCB);
        menu->SetCallback(MID_ADD_BATTALION, MenuAddUnitCB);
        menu->SetCallback(MID_STATUS, MenuUnitStatusCB);
        menu->SetCallback(MID_DELETE_UNIT, MenuUnitDeleteCB);
        menu->SetCallback(MID_ADD_VC, MenuAddVCCB);
    }

    menu = gPopupMgr->GetMenu(UNIT_POP);

    if (menu)
    {
        menu->SetOpenCallback(OpenUnitMenuCB);
        menu->SetCallback(MID_RECON, MenuReconCB);
        menu->SetCallback(MID_ADD_FLIGHT, MenuAddUnitCB);
        menu->SetCallback(MID_ADD_PACKAGE, MenuAddUnitCB);
        menu->SetCallback(MID_ADD_BATTALION, MenuAddUnitCB);
        menu->SetCallback(MID_STATUS, MenuUnitStatusCB);
        menu->SetCallback(MID_DELETE_UNIT, MenuUnitDeleteCB);
        menu->SetCallback(MID_ADD_VC, MenuAddVCCB);
        CampaignPackageMenuAttach(
            menu); // Artscout - 2026: the campaign's own package builder
        menu->SetCallback(MID_TEAM_0, MenuSetOwnerCB);
        menu->SetCallback(MID_TEAM_1, MenuSetOwnerCB);
        menu->SetCallback(MID_TEAM_2, MenuSetOwnerCB);
        menu->SetCallback(MID_TEAM_3, MenuSetOwnerCB);
        menu->SetCallback(MID_TEAM_4, MenuSetOwnerCB);
        menu->SetCallback(MID_TEAM_5, MenuSetOwnerCB);
        menu->SetCallback(MID_TEAM_6, MenuSetOwnerCB);
        menu->SetCallback(MID_TEAM_7, MenuSetOwnerCB);
    }

    menu = gPopupMgr->GetMenu(AIRUNIT_MENU);

    if (menu)
    {
        menu->SetOpenCallback(OpenUnitMenuCB);
        menu->SetCallback(MID_RECON, MenuReconCB);
        menu->SetCallback(MID_ADD_FLIGHT, MenuAddUnitCB);
        menu->SetCallback(MID_ADD_PACKAGE, MenuAddUnitCB);
        menu->SetCallback(MID_ADD_BATTALION, MenuAddUnitCB);
        menu->SetCallback(MID_STATUS, MenuUnitStatusCB);
        menu->SetCallback(MID_DELETE_UNIT, MenuUnitDeleteCB);
        menu->SetCallback(MID_ADD_VC, MenuAddVCCB);
        CampaignPackageMenuAttach(
            menu); // Artscout - 2026: the campaign's own package builder
        menu->SetCallback(MID_TEAM_0, MenuSetOwnerCB);
        menu->SetCallback(MID_TEAM_1, MenuSetOwnerCB);
        menu->SetCallback(MID_TEAM_2, MenuSetOwnerCB);
        menu->SetCallback(MID_TEAM_3, MenuSetOwnerCB);
        menu->SetCallback(MID_TEAM_4, MenuSetOwnerCB);
        menu->SetCallback(MID_TEAM_5, MenuSetOwnerCB);
        menu->SetCallback(MID_TEAM_6, MenuSetOwnerCB);
        menu->SetCallback(MID_TEAM_7, MenuSetOwnerCB);
    }

    menu = gPopupMgr->GetMenu(NAVAL_POP);

    if (menu)
    {
        menu->SetOpenCallback(OpenNavalMenuCB);
        menu->SetCallback(MID_RECON, MenuReconCB);
        menu->SetCallback(MID_ADD_FLIGHT, MenuAddUnitCB);
        menu->SetCallback(MID_ADD_PACKAGE, MenuAddUnitCB);
        menu->SetCallback(MID_ADD_BATTALION, MenuAddUnitCB);
        menu->SetCallback(MID_STATUS, MenuUnitStatusCB);
        menu->SetCallback(MID_DELETE_UNIT, MenuUnitDeleteCB);
        menu->SetCallback(MID_ADD_VC, MenuAddVCCB);
        CampaignPackageMenuAttach(
            menu); // Artscout - 2026: the campaign's own package builder
        menu->SetCallback(MID_TEAM_0, MenuSetOwnerCB);
        menu->SetCallback(MID_TEAM_1, MenuSetOwnerCB);
        menu->SetCallback(MID_TEAM_2, MenuSetOwnerCB);
        menu->SetCallback(MID_TEAM_3, MenuSetOwnerCB);
        menu->SetCallback(MID_TEAM_4, MenuSetOwnerCB);
        menu->SetCallback(MID_TEAM_5, MenuSetOwnerCB);
        menu->SetCallback(MID_TEAM_6, MenuSetOwnerCB);
        menu->SetCallback(MID_TEAM_7, MenuSetOwnerCB);
    }

    menu = gPopupMgr->GetMenu(PACKAGE_POP);

    if (menu)
    {
        menu->SetCallback(MID_RECON, MenuReconCB);
        menu->SetCallback(MID_SHOW_FLIGHTS, MenuEditPackageCB);
        menu->SetCallback(MID_DELETE_UNIT, MenuUnitDeleteCB);
        menu->SetCallback(MID_ADD_FLIGHT, MenuAddUnitCB);
        menu->SetCallback(MID_ADD_PACKAGE, MenuAddUnitCB);
        menu->SetCallback(MID_ADD_VC, MenuAddVCCB);
    }

    menu = gPopupMgr->GetMenu(STEERPOINT_POP);

    if (menu)
    {
        menu->SetOpenCallback(SteerPointMenuOpenCB);

        menu->SetCallback(MID_ADD_STPT, MenuAddWPCB);
        menu->SetCallback(MID_DELETE_STPT, MenuDeleteWPCB);
        menu->SetCallback(MID_RECON, MenuReconCB);

        // Main
        menu->SetCallback(MID_DETAILS, MenuOpenWpWindowCB);
        menu->SetCallback(MID_LOCK_TOS, MenuLockCB);
        menu->SetCallback(MID_LOCK_SPEED, MenuLockCB);

        // Climb Menu
        menu->SetCallback(CLIMB_IMMEDIATE, MenuClimbCB);
        menu->SetCallback(CLIMB_DELAY, MenuClimbCB);

        // Formation Menu
        for (i = 1; i < 9; i++)
            menu->SetCallback(i, MenuFormationCB);

        // Enroute menu (hand add the valid ones)
        menu->AddItem(WP_NOTHING bitor 0x100, C_TYPE_RADIO, WPActStr[39],
                      MID_ENR_ACTION);
        menu->SetCallback(WP_NOTHING bitor 0x100, MenuEnrouteCB);
        menu->SetItemGroup(WP_NOTHING bitor 0x100, 3);
        menu->AddItem(WP_CA bitor 0x100, C_TYPE_RADIO, WPActStr[WP_CA],
                      MID_ENR_ACTION);
        menu->SetCallback(WP_CA bitor 0x100, MenuEnrouteCB);
        menu->SetItemGroup(WP_CA bitor 0x100, 3);
        menu->AddItem(WP_ESCORT bitor 0x100, C_TYPE_RADIO, WPActStr[WP_ESCORT],
                      MID_ENR_ACTION);
        menu->SetCallback(WP_ESCORT bitor 0x100, MenuEnrouteCB);
        menu->SetItemGroup(WP_ESCORT bitor 0x100, 3);
        menu->AddItem(WP_SEAD bitor 0x100, C_TYPE_RADIO, WPActStr[WP_SEAD],
                      MID_ENR_ACTION);
        menu->SetCallback(WP_SEAD bitor 0x100, MenuEnrouteCB);
        menu->SetItemGroup(WP_SEAD bitor 0x100, 3);
        menu->AddItem(WP_SAD bitor 0x100, C_TYPE_RADIO, WPActStr[WP_SAD],
                      MID_ENR_ACTION);
        menu->SetCallback(WP_SAD bitor 0x100, MenuEnrouteCB);
        menu->SetItemGroup(WP_SAD bitor 0x100, 3);
        menu->AddItem(WP_ELINT bitor 0x100, C_TYPE_RADIO, WPActStr[WP_ELINT],
                      MID_ENR_ACTION);
        menu->SetCallback(WP_ELINT bitor 0x100, MenuEnrouteCB);
        menu->SetItemGroup(WP_ELINT bitor 0x100, 3);
        menu->AddItem(WP_TANKER bitor 0x100, C_TYPE_RADIO, WPActStr[WP_TANKER],
                      MID_ENR_ACTION);
        menu->SetCallback(WP_TANKER bitor 0x100, MenuEnrouteCB);
        menu->SetItemGroup(WP_TANKER bitor 0x100, 3);
        menu->AddItem(WP_JAM bitor 0x100, C_TYPE_RADIO, WPActStr[WP_JAM],
                      MID_ENR_ACTION);
        menu->SetCallback(WP_JAM bitor 0x100, MenuEnrouteCB);
        menu->SetItemGroup(WP_JAM bitor 0x100, 3);
        menu->AddItem(WP_ASW bitor 0x100, C_TYPE_RADIO, WPActStr[WP_ASW],
                      MID_ENR_ACTION);
        menu->SetCallback(WP_ASW bitor 0x100, MenuEnrouteCB);
        menu->SetItemGroup(WP_ASW bitor 0x100, 3);
        menu->AddItem(WP_RECON bitor 0x100, C_TYPE_RADIO, WPActStr[WP_RECON],
                      MID_ENR_ACTION);
        menu->SetCallback(WP_RECON bitor 0x100, MenuEnrouteCB);
        menu->SetItemGroup(WP_RECON bitor 0x100, 3);

        // Action Menu
        for (i = 0; i <= WP_FAC; i++)
        {
            if (not i)
                menu->AddItem(i bitor 0x200, C_TYPE_RADIO, WPActStr[39],
                              MID_ACTION);
            else
                menu->AddItem(i bitor 0x200, C_TYPE_RADIO, WPActStr[i],
                              MID_ACTION);

            menu->SetCallback(i bitor 0x200, MenuActionCB);
            menu->SetItemGroup(i bitor 0x200, 4);
        }
    }
}
