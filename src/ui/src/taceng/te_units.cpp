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
#include "division.h"
#include "battalion.h"
#include "Find.h"
#include "F4Vu.h"
#include "strategy.h"
#include "Path.h"
#include "Campaign.h"
#include "Update.h"
#include "CampList.h"
#include "squadron.h"
#include "classtbl.h"
#include "vu2.h"
#include "campstr.h"
#include "chandler.h"
#include "ui95_ext.h"
#include "uicomms.h"
#include "userids.h"
#include "textids.h"
#include "tac_class.h"
#include "te_defs.h"
#include "cmap.h"
#include "gps.h"
#include "urefresh.h"
#include "teamdata.h"
#include "brief.h"
#include "shi/float.h"
#include "MsgInc/CampDataMsg.h"

enum
{
    TAC_LOCATION = 10000,
    TAC_NOTARGET = 10001,
};
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int FindUnitSupportRole(Unit u);
void MenuToggleUnitCB(long ID, short hittype, C_Base *control);
void SelectATOPackageCB(long ID, short hittype, C_Base *control);
void DeleteFlightFromPackage(long ID, short hittype, C_Base *control);
void FillListBoxWithACTypes(C_ListBox *lbox);
void tac_select_role(long ID, short hittype, C_Base *control);
void tac_select_target(long ID, short hittype, C_Base *control);
void tac_select_squadron(long ID, short hittype, C_Base *control);
void tac_select_squadron_aircraft(long ID, short hittype, C_Base *control);
void tac_select_squadron_airbase(long ID, short hittype, C_Base *control);
void tactical_cancel_package(long ID, short hittype, C_Base *ctrl);
void tactical_make_package(long ID, short hittype, C_Base *control);
void tactical_make_flight(long ID, short hittype, C_Base *control);
void tactical_add_flight(VU_ID id, C_Base *caller);
int IsValidMission(int dindex, int mission);
int IsValidTarget(Team team, int mission, CampEntity target);
int GetMissionFromTarget(Team team, int dindex, CampEntity target);
void AreYouSure(long TitleID, _TCHAR *text, void (*OkCB)(long, short, C_Base*), void (*CancelCB)(long, short, C_Base*));
void AreYouSure(long TitleID, long MessageID, void (*OkCB)(long, short, C_Base*), void (*CancelCB)(long, short, C_Base*));
void DiscardPackage(long ID, short hittype, C_Base *control);
void KeepPackage(long ID, short hittype, C_Base *control);
void SetPackageTimes(Package new_package, CampaignTime takeoffTime, CampaignTime targetTime);
void fixup_unit(Unit unit);
void SetupFlightSpecificControls(Flight flt);
void tac_select_aircraft(long ID, short hittype, C_Base *control);
void tactical_update_package(void);
void ChangePackTimeCB(long ID, short hittype, C_Base *control);
void ChangeTimeCB(long ID, short hittype, C_Base *control);
C_ATO_Flight *BuildATOFlightInfo(Flight fl);

void tac_select_skill(long ID, short hittype, C_Base *control); // M.N.

extern GlobalPositioningSystem *gGps;
extern C_Map *gMapMgr;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void RefreshMapOnChange(void)
{
    gGps->Update();
    gMapMgr->DrawMap();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

enum
{
    ATO_OCA = 1,
    ATO_STRIKE,
    ATO_INTERDICTION,
    ATO_SEAD,
    ATO_CAS,
    ATO_DCA,
    ATO_CCCI,
    ATO_MARITIME,
    ATO_SUPPORT,
    ATO_OTHER,
};

static long AtoMissStr[] =
{
    0,
    TXT_OCA,
    TXT_STRIKE,
    TXT_INTERDICTION,
    TXT_SEAD,
    TXT_CAS,
    TXT_DCA,
    TXT_CCCI,
    TXT_MARITIME,
    TXT_SUPPORT,
    TXT_OTHER,
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct
{
    table_of_equipment_manufacturers side;
    char stype;
    char sptype;
} toe;

toe table_of_equipment_def[] =
{
    toe_chinese, STYPE_UNIT_ARMOR, SPTYPE_CHINESE_TYPE80,
    toe_chinese, STYPE_UNIT_ARMOR, SPTYPE_CHINESE_TYPE85II,
    toe_chinese, STYPE_UNIT_ARMOR, SPTYPE_CHINESE_TYPE90II,
    toe_chinese, STYPE_UNIT_AIR_DEFENSE, SPTYPE_CHINESE_SA6,
    toe_chinese, STYPE_UNIT_AIR_DEFENSE, SPTYPE_CHINESE_ZU23,
    toe_chinese, STYPE_UNIT_HQ, SPTYPE_CHINESE_HQ,
    toe_chinese, STYPE_UNIT_INFANTRY, SPTYPE_CHINESE_INF,
    toe_chinese, STYPE_UNIT_MECHANIZED, SPTYPE_CHINESE_MECH,
    toe_chinese, STYPE_UNIT_SP_ARTILLERY, SPTYPE_CHINESE_SP,
    toe_chinese, STYPE_UNIT_TOWED_ARTILLERY, SPTYPE_CHINESE_ART,
    toe_dprk, STYPE_UNIT_AIR_DEFENSE, SPTYPE_DPRK_AAA,
    toe_dprk, STYPE_UNIT_AIR_DEFENSE, SPTYPE_DPRK_SA2,
    toe_dprk, STYPE_UNIT_AIR_DEFENSE, SPTYPE_DPRK_SA3,
    toe_dprk, STYPE_UNIT_AIR_DEFENSE, SPTYPE_DPRK_SA5,
    toe_dprk, STYPE_UNIT_AIRMOBILE, SPTYPE_DPRK_AIR_MOBILE,
    toe_dprk, STYPE_UNIT_ARMOR, SPTYPE_DPRK_T55,
    toe_dprk, STYPE_UNIT_ARMOR, SPTYPE_DPRK_T62,
    toe_dprk, STYPE_UNIT_HQ, SPTYPE_DPRK_HQ,
    toe_dprk, STYPE_UNIT_INFANTRY, SPTYPE_DPRK_INF,
    toe_dprk, STYPE_UNIT_MECHANIZED, SPTYPE_DPRK_BMP1,
    toe_dprk, STYPE_UNIT_MECHANIZED, SPTYPE_DPRK_BMP2,
    toe_dprk, STYPE_UNIT_ROCKET, SPTYPE_DPRK_BM21,
    toe_dprk, STYPE_UNIT_SP_ARTILLERY, SPTYPE_DPRK_SP_122,
    toe_dprk, STYPE_UNIT_SP_ARTILLERY, SPTYPE_DPRK_SP_152,
    toe_dprk, STYPE_UNIT_SS_MISSILE, SPTYPE_DPRK_FROG,
    toe_dprk, STYPE_UNIT_SS_MISSILE, SPTYPE_DPRK_SCUD,
    toe_dprk, STYPE_UNIT_TOWED_ARTILLERY, SPTYPE_DPRK_TOW_ART,
    toe_rok, STYPE_UNIT_AIR_DEFENSE, SPTYPE_ROK_AAA,
    toe_rok, STYPE_UNIT_AIR_DEFENSE, SPTYPE_ROK_HAWK,
    toe_rok, STYPE_UNIT_AIR_DEFENSE, SPTYPE_ROK_NIKE,
    toe_rok, STYPE_UNIT_ARMOR, SPTYPE_ROK_M48,
    toe_rok, STYPE_UNIT_HQ, SPTYPE_ROK_HQ,
    toe_rok, STYPE_UNIT_INFANTRY, SPTYPE_ROK_INF,
    toe_rok, STYPE_UNIT_MARINE, SPTYPE_ROK_MARINE,
    toe_rok, STYPE_UNIT_MECHANIZED, SPTYPE_ROK_M113,
    toe_rok, STYPE_UNIT_SP_ARTILLERY, SPTYPE_ROK_SP,
    toe_rok, STYPE_UNIT_TOWED_ARTILLERY, SPTYPE_ROK_M198,
    toe_soviet, STYPE_UNIT_AIR_DEFENSE, SPTYPE_SOVIET_SA15,
    toe_soviet, STYPE_UNIT_AIR_DEFENSE, SPTYPE_SOVIET_SA6,
    toe_soviet, STYPE_UNIT_AIR_DEFENSE, SPTYPE_SOVIET_SA8,
    toe_soviet, STYPE_UNIT_AIRMOBILE, SPTYPE_SOVIET_AIR,
    toe_soviet, STYPE_UNIT_ARMOR, SPTYPE_SOVIET_T72,
    toe_soviet, STYPE_UNIT_ARMOR, SPTYPE_SOVIET_T80,
    toe_soviet, STYPE_UNIT_ARMOR, SPTYPE_SOVIET_T90,
    toe_soviet, STYPE_UNIT_ENGINEER, SPTYPE_SOVIET_ENG,
    toe_soviet, STYPE_UNIT_HQ, SPTYPE_SOVIET_HQ,
    toe_soviet, STYPE_UNIT_INFANTRY, SPTYPE_SOVIET_INF,
    toe_soviet, STYPE_UNIT_MARINE, SPTYPE_SOVIET_MARINE,
    toe_soviet, STYPE_UNIT_MECHANIZED, SPTYPE_SOVIET_MECH,
    toe_soviet, STYPE_UNIT_SS_MISSILE, SPTYPE_SOVIET_SCUD,
    toe_soviet, STYPE_UNIT_SS_MISSILE, SPTYPE_SOVIET_FROG7,
    toe_soviet, STYPE_UNIT_SP_ARTILLERY, SPTYPE_SOVIET_SP,
    // toe_soviet, STYPE_UNIT_SUPPLY, SPTYPE_SOVIET_SUP,
    toe_soviet, STYPE_UNIT_ROCKET, SPTYPE_SOVIET_BM21,
    toe_soviet, STYPE_UNIT_ROCKET, SPTYPE_SOVIET_BM24,
    toe_soviet, STYPE_UNIT_ROCKET, SPTYPE_SOVIET_BM9A52,
    toe_soviet, STYPE_UNIT_TOWED_ARTILLERY, SPTYPE_SOVIET_ART,
    toe_us, STYPE_UNIT_AIR_DEFENSE, SPTYPE_US_PATRIOT,
    toe_us, STYPE_UNIT_AIR_DEFENSE, SPTYPE_US_HAWK,
    toe_us, STYPE_UNIT_AIRMOBILE, SPTYPE_US_AIR,
    toe_us, STYPE_UNIT_ARMOR, SPTYPE_US_M1,
    toe_us, STYPE_UNIT_ARMOR, SPTYPE_US_M60,
    toe_us, STYPE_UNIT_ARMORED_CAV, SPTYPE_US_CAV,
    toe_us, STYPE_UNIT_ENGINEER, SPTYPE_US_ENG,
    toe_us, STYPE_UNIT_HQ, SPTYPE_US_HQ,
    toe_us, STYPE_UNIT_INFANTRY, SPTYPE_US_INF,
    toe_us, STYPE_UNIT_MARINE, SPTYPE_US_LAV25,
    toe_us, STYPE_UNIT_MECHANIZED, SPTYPE_US_M2,
    toe_us, STYPE_UNIT_ROCKET, SPTYPE_US_MLRS,
    toe_us, STYPE_UNIT_SP_ARTILLERY, SPTYPE_US_M109,
    // toe_us, STYPE_UNIT_SUPPLY, SPTYPE_US_SUP,
    toe_unknown, 0, 0
};

toe *table_of_equipment ;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void display_air_units(Unit unit);
void display_land_units(Unit unit);
void tactical_release_flight(long ID, short hittype, C_Base *ctrl);
static void tactical_flight_size(long ID, short hittype, C_Base *ctrl);
static void tactical_create_squadron(long ID, short hittype, C_Base *ctrl);
static void tactical_cancel_squadron(long ID, short hittype, C_Base *ctrl);
static void set_squadron_role(long ID, short hittype, C_Base *ctrl);
static void set_squadron_stype(long ID, short hittype, C_Base *ctrl);
static void set_squadron_sptype(long ID, short hittype, C_Base *ctrl);

static void update_new_battalion_window(void);
void tactical_create_battalion(long ID, short hittype, C_Base *ctrl);
static void tactical_cancel_battalion(long ID, short hittype, C_Base *ctrl);
static void set_battalion_table_of_equipment(long ID, short hittype, C_Base *ctrl);
static void set_battalion_type(long ID, short hittype, C_Base *ctrl);

int MissionToATOMiss(int mistype);

int gATOPackageSelected;
int gATOSquadronSelected;
int gLastAircraftType = 0;
int gLastAirbase = 0;
int gLastRole = 0;
int WeAreAddingPackage = 0;
int EdittingPackage = 0;
CampEntity gLastTarget = NULL;

int gLastPilotSkill = 0; // M.N. 2001-11-19

VU_ID
gLastBattalionObjID,
gLastAirbaseID,
gNewSelectFlight;

extern VU_ID
gSelectedATOFlight,
gSelectedPackage,
gSelectedSquadron,
gActiveFlightID,
gCurrentFlightID,
gSelectedFlightID;

uchar
gSelectedTeam = 1;

CampEntity
new_package_target;

CampaignTime
gPackageTOT = 0,
gTakeoffTime = 0;

extern GlobalPositioningSystem
*gGps;

extern C_Map
*gMapMgr;

GridIndex
MapX, MapY;

Package
new_package;

Flight
new_flight;

static int
gLastEquipment = 0,
gLastUnitType = -1,
first_stype = TRUE,
last_stype = 0,
gPackagePriority = 0;

extern C_TreeList
*gATOAll;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void LoadTEUnits()
{
    if (table_of_equipment not_eq NULL)
        return;

    FILE *fp = OpenCampFile("teunits", "lst", "r");

    if (fp == NULL)
        fp = OpenCampFile("te-units", "lst", "r");

    if (fp == NULL)
    {
        table_of_equipment = table_of_equipment_def;
        fp = OpenCampFile("teunits", "lst", "w");
        fprintf(fp, "// Side SubType Specific\n");

        for (int i = 0; table_of_equipment[i].side not_eq 0; i++)
        {
            fprintf(fp, "%7d %7d %8d\n",
                    table_of_equipment[i].side,
                    table_of_equipment[i].stype,
                    table_of_equipment[i].sptype);
        }

        fclose(fp);
        return;
    }

    int maxdf = 0, curdf = 0;
    char buf[128];

    while (fgets(buf, sizeof buf, fp))
    {
        if (buf[0] == '#' or buf[0] == '/' or buf[0] == '\r' or buf[0] == '\n')
            continue;

        if (curdf >= maxdf - 2)   // time to grow the array
        {
            if (table_of_equipment == NULL)
                table_of_equipment = (toe*)calloc(maxdf = 10, sizeof * table_of_equipment);
            else
            {
                maxdf *= 2;
                table_of_equipment = (toe*)realloc(table_of_equipment, maxdf * sizeof(*table_of_equipment));
            }
        }

        int SType, SPType;
        table_of_equipment_manufacturers side;

        if (sscanf(buf, "%7d %7d %8d",
                   &side,
                   &SType,
                   &SPType) not_eq 3)
        {
            ShiWarning("Bad format file teunits.lst");
            free(table_of_equipment);
            table_of_equipment = table_of_equipment_def;
            break;
        }

        table_of_equipment[curdf].side = side;
        table_of_equipment[curdf].stype = SType;
        table_of_equipment[curdf].sptype = SPType;
        curdf ++;
        table_of_equipment[curdf].side = toe_unknown;
        table_of_equipment[curdf].stype = 0;
        table_of_equipment[curdf].sptype = 0;
    }

    fclose(fp);
}

void hookup_new_squad_window(C_Window *win)
{
    C_Button *btn;
    C_ListBox *lbox;

    if ( not win)
        return;

    if ( not gLastAircraftType)
        gLastAircraftType = GetClassID(DOMAIN_AIR, CLASS_UNIT, TYPE_SQUADRON, STYPE_UNIT_FIGHTER_BOMBER, SPTYPE_F16C, VU_ANY, VU_ANY, VU_ANY) + VU_LAST_ENTITY_TYPE;

    // Setup aircraft type listbox
    lbox = (C_ListBox*)win->FindControl(TAC_AIRCRAFT_TYPE);

    if (lbox)
    {
        FillListBoxWithACTypes(lbox);
        // Select last type selected
        lbox->SetValue(gLastAircraftType);
        lbox->SetCallback(tac_select_squadron_aircraft);
    }

    // Now setup the airbase list
    lbox = (C_ListBox*)win->FindControl(TAC_AIRBASE_LIST);

    if (lbox)
        lbox->SetCallback(tac_select_squadron_airbase);

    // Setup cancel/ok buttons
    btn = (C_Button *) win->FindControl(CREATE_UNIT);

    if (btn not_eq NULL)
        btn->SetCallback(tactical_create_squadron);

    btn = (C_Button *) win->FindControl(CANCEL_UNIT);

    if (btn not_eq NULL)
        btn->SetCallback(tactical_cancel_squadron);

    btn = (C_Button *) win->FindControl(CLOSE_WINDOW);

    if (btn not_eq NULL)
        btn->SetCallback(tactical_cancel_squadron);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void hookup_new_battalion_window(C_Window *win)
{
    C_Button *btn;
    C_ListBox *list;
    _TCHAR buffer[80];

    btn = (C_Button *) win->FindControl(CREATE_UNIT);

    if (btn not_eq NULL)
        btn->SetCallback(tactical_create_battalion);

    btn = (C_Button *) win->FindControl(CANCEL_UNIT);

    if (btn not_eq NULL)
        btn->SetCallback(tactical_cancel_battalion);

    btn = (C_Button *) win->FindControl(CLOSE_WINDOW);

    if (btn not_eq NULL)
        btn->SetCallback(tactical_cancel_battalion);

    list = (C_ListBox *) win->FindControl(UNIT_TOE);

    if (list)
    {
        list->SetCallback(set_battalion_table_of_equipment);
        list->RemoveAllItems();
        // Generate list of teams
        buffer[0] = 0;
        AddIndexedStringToBuffer(670, buffer);
        list->AddItem(toe_us, C_TYPE_ITEM, buffer);
        buffer[0] = 0;
        AddIndexedStringToBuffer(671, buffer);
        list->AddItem(toe_soviet, C_TYPE_ITEM, buffer);
        buffer[0] = 0;
        AddIndexedStringToBuffer(672, buffer);
        list->AddItem(toe_chinese, C_TYPE_ITEM, buffer);
        buffer[0] = 0;
        AddIndexedStringToBuffer(673, buffer);
        list->AddItem(toe_rok, C_TYPE_ITEM, buffer);
        buffer[0] = 0;
        AddIndexedStringToBuffer(674, buffer);
        list->AddItem(toe_dprk, C_TYPE_ITEM, buffer);
    }

    list = (C_ListBox *) win->FindControl(UNIT_TYPE);

    if (list)
    {
        list->SetCallback(set_battalion_type);
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void PickTeamColors()
{
    C_Window *win;
    C_Button *btn;
    C_Line *line;

    win = gMainHandler->FindWindow(TAC_EDIT_WIN);

    if (win)
    {
        btn = (C_Button*)win->FindControl(TEAM_SELECTOR);

        if (btn)
        {
            btn->SetState(gSelectedTeam);
            btn->Refresh();
        }

        line = (C_Line*)win->FindControl(TEAM_COLOR);

        if (line)
        {
            if (gSelectedTeam >= 0 and 
 not F4IsBadReadPtr(TeamInfo[gSelectedTeam], sizeof * TeamInfo) and 
                TeamInfo[gSelectedTeam]->GetColor() >= 0 and 
                TeamInfo[gSelectedTeam]->GetColor() < NUM_TEAM_COLORS)
                line->SetColor(TeamColorList[TeamInfo[gSelectedTeam]->GetColor()]);

            line->Refresh();
        }
    }

    win = gMainHandler->FindWindow(TAC_PUA_MAP);

    if (win)
    {
        btn = (C_Button*)win->FindControl(TEAM_SELECTOR);

        if (btn)
        {
            btn->SetState(gSelectedTeam);
            btn->Refresh();
        }

        line = (C_Line*)win->FindControl(TEAM_COLOR);

        if (line)
        {
            if (gSelectedTeam >= 0 and 
 not F4IsBadReadPtr(TeamInfo[gSelectedTeam], sizeof * TeamInfo) and 
                TeamInfo[gSelectedTeam]->GetColor() >= 0 and 
                TeamInfo[gSelectedTeam]->GetColor() < NUM_TEAM_COLORS)
                line->SetColor(TeamColorList[TeamInfo[gSelectedTeam]->GetColor()]);

            line->Refresh();
        }
    }

    win = gMainHandler->FindWindow(TAC_FULLMAP_WIN);

    if (win)
    {
        btn = (C_Button*)win->FindControl(TEAM_SELECTOR);

        if (btn)
        {
            btn->SetState(gSelectedTeam);
            btn->Refresh();
        }

        line = (C_Line*)win->FindControl(TEAM_COLOR);

        if (line)
        {
            if (gSelectedTeam >= 0 and 
 not F4IsBadReadPtr(TeamInfo[gSelectedTeam], sizeof * TeamInfo) and 
                TeamInfo[gSelectedTeam]->GetColor() >= 0 and 
                TeamInfo[gSelectedTeam]->GetColor() < NUM_TEAM_COLORS)
                line->SetColor(TeamColorList[TeamInfo[gSelectedTeam]->GetColor()]);

            line->Refresh();
        }
    }
}

void PickTeamCB(long, short hittype, C_Base *)
{
    short StartTeam;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    StartTeam = gSelectedTeam;

    do
    {
        gSelectedTeam++;

        if (gSelectedTeam >= NUM_TEAMS)
            gSelectedTeam = 1;
    }
    while ( not TeamInfo[gSelectedTeam] and gSelectedTeam not_eq StartTeam);

    gLastEquipment = 0;
    PickTeamColors();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void tactical_show_ato_window(void)
{
    gGps->SetAllowed(gGps->GetAllowed() bitor UR_ATO bitor UR_SQUADRON);
    gGps->Update();

    gMainHandler->EnableWindowGroup(7001);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void tactical_show_oob_window(void)
{
    gGps->SetAllowed(gGps->GetAllowed() bitor UR_OOB);

    gMainHandler->EnableWindowGroup(7009);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void tactical_new_flight_select(long, short hittype, C_Base *control)
{
    C_TreeList *tree;
    TREELIST *item;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    if ( not control)
        return;

    tree = (C_TreeList*)control;
    item = tree->GetLastItem();
    tree->SetAllControlStates(0, tree->GetRoot());

    if (item and item->Item_)
    {
        item->Item_->SetState(1);
        gNewSelectFlight = ((C_ATO_Flight*)item->Item_)->GetVUID();
    }

    tree->Refresh();
}

void FillListBoxWithSquadrons(C_ListBox *lbox, long team, long aircraft_dindex)
{
    VU_ID id = FalconNullId;
    _TCHAR buffer[50];
    VuListIterator iter(AllAirList);
    CampEntity entity, sq = NULL;
    short count = 0;

    if (TheCampaign.Flags bitand CAMP_TACTICAL_EDIT)
    {
        // Add the new item
        lbox->AddItem(1, C_TYPE_ITEM, TXT_NEW);
        lbox->SetValue(1);
    }

    // Add the squadrons
    entity = GetFirstEntity(&iter);

    while (entity)
    {
        if (entity->IsSquadron())
        {
            if (entity->GetTeam() == team and entity->GetSType() == Falcon4ClassTable[aircraft_dindex].vuClassData.classInfo_[VU_STYPE] and entity->GetSPType() == Falcon4ClassTable[aircraft_dindex].vuClassData.classInfo_[VU_SPTYPE])
            {
                entity->GetName(buffer, 40, FALSE);
                lbox->AddItem(entity->GetCampID(), C_TYPE_ITEM, buffer);

                if ( not count)
                {
                    lbox->SetValue(entity->GetCampID());
                    sq = entity;
                }

                count++;
            }
        }

        entity = GetNextEntity(&iter);
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Squadron Stuff
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void tac_select_squadron_aircraft(long, short hittype, C_Base *control)
{
    C_Window *win;

    if (hittype not_eq C_TYPE_SELECT)
        return;

    win = gMainHandler->FindWindow(NEW_SQUAD_WIN);

    if ( not win)
        return;

    gLastAircraftType = ((C_ListBox*)control)->GetTextID();
}

void tac_select_squadron_airbase(long, short hittype, C_Base *control)
{
    C_Window *win;
    CampBaseClass* airbase;

    if (hittype not_eq C_TYPE_SELECT)
        return;

    win = gMainHandler->FindWindow(NEW_SQUAD_WIN);

    if ( not win)
        return;

    airbase = GetEntityByCampID(((C_ListBox*)control)->GetTextID());

    if (airbase and airbase->IsObjective() and airbase->GetType() == TYPE_AIRBASE)
    {
        gLastAirbaseID = airbase->Id();
        gLastAirbase = airbase->GetCampID();
    }
}

SquadronClass* tactical_make_squadron(VU_ID id, long ac_type)
{
    GridIndex x, y;
    CampBaseClass* airbase;
    Squadron new_squadron;

    airbase = (CampBaseClass*) FindEntity(id);

    if ( not airbase or not airbase->IsObjective() or airbase->GetType() not_eq TYPE_AIRBASE)
        // KCK: Should probably just pick one
        return NULL;

    gLastAirbaseID = airbase->Id();
    gLastAirbase = airbase->GetCampID();

    new_squadron = NewSquadron(ac_type);
    new_squadron->SetOwner(airbase->GetOwner());
    new_squadron->SetUnitAirbase(airbase->Id());
    new_squadron->BuildElements();
    airbase->GetLocation(&x, &y);
    new_squadron->SetLocation(x, y);

    new_squadron->SetSendCreate(VuEntity::VU_SC_SEND_OOB);
    vuDatabase->/*Quick*/Insert(new_squadron);
    // 2000-11-17 ADDED BY S.G. NEED TO CALL InitPilots AGAIN SO THE SKILL ARE SET FOR THE TEAM.
    // THE FIRST TIME IT WAS CALLED, THE TEAM WASN'T KNOWN SO THE SKILLS WERE 'GENERIC'.
    new_squadron->InitPilots();
    // END OF ADDED SECTION

    return new_squadron;
}

// This is called when we want to create a new squadron (ID is the ID of the airbase)
void tactical_add_squadron(VU_ID id)
{
    C_ListBox *lbox;
    C_Window *win;
    CampBaseClass* airbase;

    win = gMainHandler->FindWindow(NEW_SQUAD_WIN);

    if ( not win)
        return;

    airbase = (CampBaseClass*) FindEntity(id);

    if ( not airbase or not airbase->IsObjective() or airbase->GetType() not_eq TYPE_AIRBASE)
        // KCK: Should probably just pick one
        return;

    lbox = (C_ListBox*)win->FindControl(TAC_AIRCRAFT_TYPE);

    if (lbox)
    {
        lbox->RemoveAllItems();
        FillListBoxWithACTypes(lbox);
        lbox->SetValue(gLastAircraftType);
    }

    // Setup the airbase list box
    gLastAirbaseID = airbase->Id();
    gLastAirbase = airbase->GetCampID();
    lbox = (C_ListBox*)win->FindControl(TAC_AIRBASE_LIST);

    if (lbox)
    {
        VuListIterator ait(AllObjList);
        Objective o;
        _TCHAR name[80];

        lbox->RemoveAllItems();
        o = (Objective) ait.GetFirst();

        while (o)
        {
            if (o->GetType() == TYPE_AIRBASE and GetRoE(gSelectedTeam, o->GetTeam(), ROE_AIR_USE_BASES) == ROE_ALLOWED)
            {
                // Add airbase name to listbox
                o->GetName(name, 79, TRUE);
                lbox->AddItem(o->GetCampID(), C_TYPE_ITEM, name);

                if ( not gLastAirbase)
                    gLastAirbase = o->GetCampID();

                if (o->GetCampID() == gLastAirbase)
                    lbox->SetValue(gLastAirbase);
            }

            o = (Objective) ait.GetNext();
        }

        lbox->Refresh();
    }

    gMainHandler->EnableWindowGroup(31000);
}

void tactical_create_squadron(long, short hittype, C_Base *)
{
    int tid;
    Squadron squadron;
    C_PopupList *menu;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    gMainHandler->DisableWindowGroup(31000);

    tid = gLastAircraftType - VU_LAST_ENTITY_TYPE;

    if (tid < 0)
        tid = GetClassID(DOMAIN_AIR, CLASS_UNIT, TYPE_SQUADRON, STYPE_UNIT_FIGHTER_BOMBER, SPTYPE_F16C, 0, 0, 0);

    if ( not tid)
        return;

    squadron = tactical_make_squadron(gLastAirbaseID, tid + VU_LAST_ENTITY_TYPE);

    if ( not squadron)
        return;

    menu = gPopupMgr->GetMenu(MAP_POP);

    if ( not menu)
        return;

    // Make sure we can see this squadron
    menu->SetItemState(MID_UNITS_SQUAD_SQUADRON, 1);
    MenuToggleUnitCB(MID_UNITS_SQUAD_SQUADRON, 0, menu);

    display_air_units(squadron);
    // gGps->Update(); done by RefreshMapOnChange()
    // MN 2002-01-04 show up the squadron faster
    // RefreshMapOnChange();

}

void tactical_cancel_squadron(long, short hittype, C_Base *)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    gMainHandler->DisableWindowGroup(31000);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Package Stuff
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void LockTakeoffTimeCB(long, short hittype, C_Base *)
{
    C_Button *btn;
    C_Window *win;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    win = gMainHandler->FindWindow(PACKAGE_WIN);

    if (win)
    {
        btn = (C_Button*)win->FindControl(PAK_TAKEOFF_LOCK);

        if (btn and btn->GetState() == 1)
        {
            // Clear lock on TOT, if any
            btn = (C_Button*)win->FindControl(PAK_TOT_LOCK);

            if (btn)
            {
                btn->SetState(0);
                btn->Refresh();
            }
        }
        else
        {
            btn = (C_Button*)win->FindControl(PAK_TOT_LOCK);

            if (btn)
            {
                btn->SetState(1);
                btn->Refresh();
            }
        }
    }
}

void LockTimeOnTargetCB(long, short hittype, C_Base *)
{
    C_Button *btn;
    C_Window *win;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    win = gMainHandler->FindWindow(PACKAGE_WIN);

    if (win)
    {
        btn = (C_Button*)win->FindControl(PAK_TOT_LOCK);

        if (btn and btn->GetState() == 1)
        {
            // Clear lock on TOT, if any
            btn = (C_Button*)win->FindControl(PAK_TAKEOFF_LOCK);

            if (btn)
            {
                btn->SetState(0);
                btn->Refresh();
            }
        }
        else
        {
            btn = (C_Button*)win->FindControl(PAK_TAKEOFF_LOCK);

            if (btn)
            {
                btn->SetState(1);
                btn->Refresh();
            }
        }
    }
}

void ChangePackTimeCB(long ID, short hittype, C_Base *control)
{
    ChangeTimeCB(ID, hittype, control);

    if (hittype == C_TYPE_LMOUSEUP)
        tactical_update_package();
}

void tactical_update_package(void)
{
    C_Window *win;
    C_ListBox *lbox;
    C_Text *text;
    C_EditBox *edit;
    C_Button *btn;
    C_Clock *clock;
    Unit element = NULL;
    int start_day = 0;

    if (new_package)
        element = new_package->GetFirstUnitElement();

    win = gMainHandler->FindWindow(PACKAGE_WIN);

    if (win)
    {
        text = (C_Text *)win->FindControl(PACKAGE_TARGET);

        if (text)
        {
            _TCHAR buffer[80] = {0};

            if (new_package_target)
            {
                if (new_package_target->IsFlight())
                    GetCallsign(((Flight)new_package_target)->callsign_id, ((Flight)new_package_target)->callsign_num, buffer);
                else
                    new_package_target->GetName(buffer, 79, FALSE);
            }
            else
            {
                // KCK: Need to figure out where we clicked
                AddLocationToBuffer('n', MapX, MapY, buffer);
            }

            text->SetText(buffer);
            text->Refresh();
        }

        // Set Package Type listbox
        lbox = (C_ListBox *)win->FindControl(TAC_PACKAGE_TYPE);

        if (lbox)
        {
            if (element)
                lbox->SetValue(MissionToATOMiss(element->GetUnitMission()));
            else
                lbox->SetValue(ATO_OTHER);

            lbox->Refresh();
        }

        lbox = (C_ListBox *)win->FindControl(PACKAGE_PRIORITY_LIST);

        if (lbox)
        {
            gPackagePriority = lbox->GetTextID();

            switch (gPackagePriority)
            {
                case 1:
                    gPackagePriority = 255;
                    break;

                case 2:
                    gPackagePriority = 175;
                    break;

                case 3:
                    gPackagePriority = 125;
                    break;

                case 4:
                    gPackagePriority = 75;
                    break;

                case 5:
                    gPackagePriority = 5;
                    break;

                default:
                    gPackagePriority = 0;
                    break;
            }
        }

        edit = (C_EditBox*)win->FindControl(PACKAGE_DAY);

        if (edit)
            start_day = edit->GetInteger() - 1;

        if (start_day < 0)
            start_day = 0;

        gTakeoffTime = gPackageTOT = 0;
        btn = (C_Button*)win->FindControl(PAK_TOT_LOCK);

        if (btn and btn->GetState() == 1)
        {
            // Get our _locked_ time on target
            clock = (C_Clock*)win->FindControl(PAK_TOT_TIME);

            if (clock)
            {
                int hr, mn, se;
                hr = clock->GetHour();
                mn = clock->GetMinute();
                se = clock->GetSecond();
                gPackageTOT = start_day * CampaignDay + hr * CampaignHours + mn * CampaignMinutes + se * CampaignSeconds;
                // KCK: We should make sure the package times are all set to this
                // (in case the user changed this control after creating the flights)
                SetPackageTimes(new_package, 0, gPackageTOT);
            }
        }

        btn = (C_Button*)win->FindControl(PAK_TAKEOFF_LOCK);

        if (btn and btn->GetState() == 1 and not gPackageTOT)
        {
            // Get our _locked_ takeoff time
            clock = (C_Clock*)win->FindControl(PAK_TAKEOFF_TIME);

            if (clock)
            {
                int hr, mn, se;
                hr = clock->GetHour();
                mn = clock->GetMinute();
                se = clock->GetSecond();
                gTakeoffTime = start_day * CampaignDay + hr * CampaignHours + mn * CampaignMinutes + se * CampaignSeconds;

                // Check for bad takeoff time in running mode
                if ( not (TheCampaign.Flags bitand CAMP_TACTICAL_EDIT) and gTakeoffTime < TheCampaign.CurrentTime)
                {
                    gTakeoffTime = TheCampaign.CurrentTime + CampaignSeconds;
                    hr = gTakeoffTime / CampaignHours;
                    hr = hr % 24;
                    mn = (gTakeoffTime / CampaignMinutes) % 60;
                    se = (gTakeoffTime / CampaignSeconds) % 60;
                    clock->SetHour(hr);
                    clock->SetMinute(mn);
                    clock->SetSecond(se);
                    clock->Refresh();
                }

                // KCK: We should make sure the package times are all set to this
                // (in case the user changed this control after creating the flights)
                SetPackageTimes(new_package, gTakeoffTime, 0);
            }
        }
    }
}

void SetupPackageControls(C_Window *win, C_Base *caller)
{
    C_Button *btn;
    C_ListBox *lbox;
    C_Clock *clock;
    C_Text *txt;
    C_EditBox *edit;
    int i;
    CampaignTime takeoff, tot;

    if ( not win)
        return;

    // Initial clock times based off of either current time or actual package statistics
    takeoff = TheCampaign.CurrentTime + 2 * CampaignMinutes;
    tot = TheCampaign.CurrentTime + 30 * CampaignMinutes;

    if (new_package and new_package->GetUnitElement(0))
    {
        // Set times based off of first flight in package
        Flight flight = (Flight) new_package->GetUnitElement(0);
        WayPoint w = flight->GetFirstUnitWP();

        if (w)
            takeoff = w->GetWPDepartureTime();

        while (w and not (w->GetWPFlags() bitand WPF_TARGET))
            w = w->GetNextWP();

        if (w)
            tot = w->GetWPArrivalTime();
    }

    // sfr: addpackage
    if ( /* not (TheCampaign.Flags bitand (CAMP_TACTICAL|CAMP_TACTICAL_EDIT)) or*/
        (EdittingPackage and not (TheCampaign.Flags bitand CAMP_TACTICAL_EDIT) and takeoff < TheCampaign.CurrentTime))
    {
        // Disable these controls in campaign, or in run mode if the package has departed
        btn = (C_Button*)win->FindControl(ADD_PACKAGE_FLIGHT);

        if (btn)
            btn->SetFlagBitOn(C_BIT_INVISIBLE);

        btn = (C_Button*)win->FindControl(DELETE_PACKAGE_FLIGHT);

        if (btn)
            btn->SetFlagBitOn(C_BIT_INVISIBLE);

        btn = (C_Button*)win->FindControl(PAK_TAKEOFF_LOCK);

        if (btn)
            btn->SetFlagBitOn(C_BIT_INVISIBLE);

        btn = (C_Button*)win->FindControl(PAK_TAKEOFF_INC);

        if (btn)
            btn->SetFlagBitOn(C_BIT_INVISIBLE);

        btn = (C_Button*)win->FindControl(PAK_TAKEOFF_DEC);

        if (btn)
            btn->SetFlagBitOn(C_BIT_INVISIBLE);

        btn = (C_Button*)win->FindControl(PAK_TOT_LOCK);

        if (btn)
            btn->SetFlagBitOn(C_BIT_INVISIBLE);

        btn = (C_Button*)win->FindControl(PAK_TOT_INC);

        if (btn)
            btn->SetFlagBitOn(C_BIT_INVISIBLE);

        btn = (C_Button*)win->FindControl(PAK_TOT_DEC);

        if (btn)
            btn->SetFlagBitOn(C_BIT_INVISIBLE);

        clock = (C_Clock*)win->FindControl(PAK_TAKEOFF_TIME);

        if (clock)
            clock->SetFlagBitOff(C_BIT_ENABLED);

        clock = (C_Clock*)win->FindControl(PAK_TOT_TIME);

        if (clock)
            clock->SetFlagBitOff(C_BIT_ENABLED);
    }
    else
    {
        // These controls are allowed in tactical engagement unless we're editing a package in run mode
        btn = (C_Button*)win->FindControl(ADD_PACKAGE_FLIGHT);

        if (btn)
            btn->SetFlagBitOff(C_BIT_INVISIBLE);

        btn = (C_Button*)win->FindControl(DELETE_PACKAGE_FLIGHT);

        if (btn)
            btn->SetFlagBitOff(C_BIT_INVISIBLE);

        btn = (C_Button*)win->FindControl(PAK_TAKEOFF_LOCK);

        if (btn)
            btn->SetFlagBitOff(C_BIT_INVISIBLE);

        btn = (C_Button*)win->FindControl(PAK_TAKEOFF_INC);

        if (btn)
            btn->SetFlagBitOff(C_BIT_INVISIBLE);

        btn = (C_Button*)win->FindControl(PAK_TAKEOFF_DEC);

        if (btn)
            btn->SetFlagBitOff(C_BIT_INVISIBLE);

        btn = (C_Button*)win->FindControl(PAK_TOT_LOCK);

        if (btn)
            btn->SetFlagBitOff(C_BIT_INVISIBLE);

        btn = (C_Button*)win->FindControl(PAK_TOT_INC);

        if (btn)
            btn->SetFlagBitOff(C_BIT_INVISIBLE);

        btn = (C_Button*)win->FindControl(PAK_TOT_DEC);

        if (btn)
            btn->SetFlagBitOff(C_BIT_INVISIBLE);

        clock = (C_Clock*)win->FindControl(PAK_TAKEOFF_TIME);

        if (clock)
            clock->SetFlagBitOn(C_BIT_ENABLED);

        clock = (C_Clock*)win->FindControl(PAK_TOT_TIME);

        if (clock)
            clock->SetFlagBitOn(C_BIT_ENABLED);
    }

    if ( not EdittingPackage)
    {
        // These buttons are only allowed if creating a new package
        btn = (C_Button*)win->FindControl(CANCEL_PACK);

        if (btn)
            btn->SetFlagBitOff(C_BIT_INVISIBLE);
    }
    else
    {
        btn = (C_Button*)win->FindControl(CANCEL_PACK);

        if (btn)
            btn->SetFlagBitOn(C_BIT_INVISIBLE);
    }

    if (TheCampaign.Flags bitand CAMP_TACTICAL)
    {
        // These are allowed always in tactical engagement
        lbox = (C_ListBox*)win->FindControl(PACKAGE_PRIORITY_LIST);

        if (lbox)
            lbox->SetFlagBitOn(C_BIT_ENABLED);
    }
    else
    {
        lbox = (C_ListBox*)win->FindControl(PACKAGE_PRIORITY_LIST);

        if (lbox)
            lbox->SetFlagBitOff(C_BIT_ENABLED);
    }

    if (EdittingPackage and new_package)
    {
        _TCHAR buffer[40];

        txt = (C_Text*)win->FindControl(PACKAGE_DESIGNATOR);

        if (txt)
        {
            _stprintf(buffer, "%s %1ld", gStringMgr->GetString(TXT_PACKAGE), new_package->GetCampID());
            txt->SetText(buffer);
            txt->Refresh();
        }
    }
    else
    {
        txt = (C_Text*)win->FindControl(PACKAGE_DESIGNATOR);

        if (txt)
        {
            txt->SetText(TXT_ADD_PACKAGE_UC);
            txt->Refresh();
        }
    }

    // Set Package Type listbox
    lbox = (C_ListBox *)win->FindControl(TAC_PACKAGE_TYPE);

    if (lbox)
    {
        for (i = 1; i <= ATO_OTHER; i++)
            lbox->AddItem(i, C_TYPE_ITEM, AtoMissStr[i]);
    }

    // Initialize takeoff time to now
    clock = (C_Clock*)win->FindControl(PAK_TAKEOFF_TIME);

    if (clock)
    {
        int hr, min, sec;
        hr = takeoff / CampaignHours;
        hr = hr % 24;
        min = (takeoff / CampaignMinutes) % 60;
        sec = (takeoff / CampaignSeconds) % 60;
        clock->SetHour(hr);
        clock->SetMinute(min);
        clock->SetSecond(sec);
        clock->Refresh();
    }

    // Initialize takeoff time to now plus estimated time to target
    clock = (C_Clock*)win->FindControl(PAK_TOT_TIME);

    if (clock)
    {
        int hr, min, sec;
        hr = tot / CampaignHours;
        hr = hr % 24;
        min = (tot / CampaignMinutes) % 60;
        sec = (tot / CampaignSeconds) % 60;
        clock->SetHour(hr);
        clock->SetMinute(min);
        clock->SetSecond(sec);
        clock->Refresh();
    }

    // Initial package day edit
    edit = (C_EditBox*)win->FindControl(PACKAGE_DAY);

    if (edit)
        edit->SetText((takeoff / CampaignDay) + 1);

    // Initialize priority list box
    lbox = (C_ListBox *)win->FindControl(PACKAGE_PRIORITY_LIST);

    if (lbox)
    {
        lbox->AddItem(1, C_TYPE_ITEM, "A");
        lbox->AddItem(2, C_TYPE_ITEM, "B");
        lbox->AddItem(3, C_TYPE_ITEM, "C");
        lbox->AddItem(4, C_TYPE_ITEM, "D");
        lbox->AddItem(5, C_TYPE_ITEM, "E");
        lbox->AddItem(6, C_TYPE_ITEM, "F");
        lbox->SetValue(1);
    }

    // Setup time lock buttons
    btn = (C_Button*)win->FindControl(PAK_TAKEOFF_LOCK);

    if (btn)
        btn->SetCallback(LockTakeoffTimeCB);

    btn = (C_Button*)win->FindControl(PAK_TOT_LOCK);

    if (btn)
    {
        btn->SetCallback(LockTimeOnTargetCB);

        if ( not btn->GetState())
            LockTakeoffTimeCB(0, C_TYPE_LMOUSEUP, NULL);
    }

    // Setup cancel and ok
    btn = (C_Button*)win->FindControl(CANCEL_PACK);

    if (btn)
        btn->SetCallback(DiscardPackage);

    btn = (C_Button*)win->FindControl(OK_PACK);

    if (btn)
        btn->SetCallback(KeepPackage);

    btn = (C_Button*)win->FindControl(PAK_TOT_DEC);

    if (btn)
        btn->SetCallback(ChangePackTimeCB);

    btn = (C_Button*)win->FindControl(PAK_TOT_INC);

    if (btn)
        btn->SetCallback(ChangePackTimeCB);

    btn = (C_Button*)win->FindControl(PAK_TAKEOFF_DEC);

    if (btn)
        btn->SetCallback(ChangePackTimeCB);

    btn = (C_Button*)win->FindControl(PAK_TAKEOFF_INC);

    if (btn)
        btn->SetCallback(ChangePackTimeCB);

    if (caller->_GetCType_() not_eq _CNTL_BUTTON_)
        gMainHandler->EnableWindowGroup(win->GetGroup());
}

// KCK: This is called when we've decided to add a package -
// i.e: when we're in package create mode and clicked on the map
void tactical_add_package(VU_ID id, C_Base *caller)
{
    C_Window *win = NULL;
    C_TreeList *tree = NULL;
    CampEntity ent = NULL;
    GridIndex x = 0, y = 0;
    float mx, my, maxy, scale;

    EdittingPackage = 0;
    WeAreAddingPackage = 1;

    if (caller->_GetCType_() == _CNTL_POPUPLIST_)
    {
        gPopupMgr->GetCurrentXY(&x, &y);
        gMapMgr->GetMapRelativeXY(&x, &y);
    }
    else if (caller->_GetCType_() == _CNTL_MAP_MOVER_)
    {
        x = static_cast<short>(((C_MapMover*)caller)->GetRelX() + caller->GetX() + caller->Parent_->GetX());
        y = static_cast<short>(((C_MapMover*)caller)->GetRelY() + caller->GetY() + caller->Parent_->GetY());
        gMapMgr->GetMapRelativeXY(&x, &y);
    }

    gTakeoffTime = gPackageTOT = 0;
    new_package = NULL;
    gNewSelectFlight = FalconNullId;
    scale = gMapMgr->GetMapScale();
    maxy = gMapMgr->GetMaxY();

    mx = x / scale;
    my = maxy - y / scale;

    // Determine target (or target location)
    ent = (CampEntity) vuDatabase->Find(id);

    if (ent)
    {
        gLastTarget = new_package_target = ent;
        new_package_target->GetLocation(&MapX, &MapY);
    }
    else
    {
        MapX = SimToGrid(mx);
        MapY = SimToGrid(my);
        new_package_target = NULL;
    }

    // Determine default mission type
    if ( not gLastAircraftType)
        gLastAircraftType = GetClassID(DOMAIN_AIR, CLASS_UNIT, TYPE_SQUADRON, STYPE_UNIT_FIGHTER_BOMBER, SPTYPE_F16C, VU_ANY, VU_ANY, VU_ANY) + VU_LAST_ENTITY_TYPE;

    gLastRole = GetMissionFromTarget(gSelectedTeam, gLastAircraftType - VU_LAST_ENTITY_TYPE, ent);

    win = gMainHandler->FindWindow(PACKAGE_WIN);

    if (win)
    {
        // Clear any old flight trees
        tree = (C_TreeList *)win->FindControl(ATO_PACKAGE_TREE);

        if (tree)
        {
            tree->DeleteBranch(tree->GetRoot());
            tree->RecalcSize();
            tree->Parent_->RefreshClient(tree->GetClient());
        }

        SetupPackageControls(win, caller);
    }

    tactical_update_package();
}

void tactical_edit_package(VU_ID id, C_Base *caller)
{
    C_Window *win;
    C_TreeList *tree;
    Flight flight;

    new_package = (Package) vuDatabase->Find(id);

    if ( not new_package)
        return;

    EdittingPackage = 1;
    WeAreAddingPackage = 1;
    gTakeoffTime = gPackageTOT = 0;
    gNewSelectFlight = FalconNullId;

    gLastTarget = new_package_target = (CampBaseClass*) vuDatabase->Find(new_package->GetMissionRequest()->targetID);

    if (new_package_target)
        new_package_target->GetLocation(&MapX, &MapY);
    else
        new_package->GetLocation(&MapX, &MapY);

    // Determine default mission type
    flight = (Flight)new_package->GetFirstUnitElement();

    if (flight)
        gLastRole = flight->GetUnitMission();

    tactical_update_package();

    win = gMainHandler->FindWindow(PACKAGE_WIN);

    if (win)
    {
        // Clear any old flight trees
        tree = (C_TreeList *)win->FindControl(ATO_PACKAGE_TREE);

        if (tree)
        {
            C_ATO_Flight *atoflt;
            TREELIST *item;

            tree->DeleteBranch(tree->GetRoot());
            tree->RecalcSize();
            tree->Parent_->RefreshClient(tree->GetClient());
            tree->SetCallback(tactical_new_flight_select);

            // Add current flights
            flight = (Flight)new_package->GetFirstUnitElement();

            while (flight)
            {
                atoflt = BuildATOFlightInfo(flight);

                if (atoflt)
                {
                    item = tree->CreateItem(flight->GetCampID(), C_TYPE_ITEM, atoflt);

                    if (item)
                        tree->AddItem(tree->GetRoot(), item);
                }

                flight = (Flight)new_package->GetNextUnitElement();
            }

            tree->RecalcSize();
            tree->Parent_->RefreshClient(tree->GetClient());
        }
    }

    SetupPackageControls(win, caller);
}

void Open_Flight_WindowCB(long, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    if (new_package_target)
        tactical_add_flight(new_package_target->Id(), control);
    else
        tactical_add_flight(FalconNullId, control);
}

void EditFlightInPackage(long, short hittype, C_Base *)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;
}

void DeleteFlightFromPackage(long, short hittype, C_Base *control)
{
    Flight flt;
    C_TreeList *tree;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    tree = (C_TreeList*)control->Parent_->FindControl(ATO_PACKAGE_TREE);

    if (tree)
    {
        flt = (Flight)vuDatabase->Find(gNewSelectFlight);

        if (flt)
        {
            tree->DeleteItem(flt->GetCampID());
            //flt->DisposeWayPoints();
            gMapMgr->SetCurrentWaypointList(flt->Id());
            RegroupFlight(flt);
            flt->Remove();
            gNewSelectFlight = FalconNullId;
            tree->RecalcSize();
            tree->Parent_->RefreshClient(tree->GetClient());
        }
    }
}

void DiscardPackage(long, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    // if( not EdittingPackage)
    // {
    gNewSelectFlight = FalconNullId;

    if (new_package)
    {
        new_package->KillUnit();
        new_package->Remove();
    }

    // }
    EdittingPackage = 0;
    new_package = NULL;
    WeAreAddingPackage = 0;
    gMainHandler->DisableWindowGroup(control->Parent_->GetGroup());
}

void KeepPackage(long ID, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    if (new_package and not new_package->GetFirstUnitElement())
        DiscardPackage(ID, hittype, control);

    gNewSelectFlight = FalconNullId;

    if (new_package)
    {
        // If we're a client creating a package on the fly, we need to set the owner
        // to the host and send a full update
        new_package->SetFinal(1);

        if ( not FalconLocalGame->IsLocal())
        {
            FalconSessionEntity *host = (FalconSessionEntity*) vuDatabase->Find(FalconLocalGame->OwnerId());
            FalconEntity *element;
            new_package->DoFullUpdate();
            new_package->FalconEntity::SetOwner(host);
            element = (FalconEntity*) new_package->GetFirstUnitElement();

            while (element)
            {
                element->DoFullUpdate();
                element->FalconEntity::SetOwner(host);
                element = (FalconEntity*) new_package->GetNextUnitElement();
            }
        }

        new_package = NULL;
    }

    EdittingPackage = 0;
    new_package = NULL;
    WeAreAddingPackage = 0;
    gMainHandler->DisableWindowGroup(control->Parent_->GetGroup());
}

// KCK: This is called when we've decided to create a package -
// which just before we're planning on creating a flight with no current parent package
void tactical_make_package(long, short hittype, C_Base *)
{
    C_Window *win;
    C_ListBox *lbox;
    C_TreeList *tree;
    long type = 0; //start_day=0,;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    win = gMainHandler->FindWindow(PACKAGE_WIN);

    if (win)
    {
        // Clear any previous lists
        gNewSelectFlight = FalconNullId;
        tree = (C_TreeList *)win->FindControl(ATO_PACKAGE_TREE);

        if (tree)
        {
            tree->DeleteBranch(tree->GetRoot());
            tree->RecalcSize();
            tree->Parent_->RefreshClient(tree->GetClient());
        }

        // Find type
        lbox = (C_ListBox *)win->FindControl(TAC_PACKAGE_TYPE);

        if (lbox)
            type = lbox->GetTextID();

        new_package = (Package) NewUnit(DOMAIN_AIR, TYPE_PACKAGE, 0, 0, NULL);

        if (new_package)
        {
            MissionRequestClass mis;

            // Set up our mission request
            mis.who = gSelectedTeam;

            if (new_package_target)
            {
                mis.targetID = mis.requesterID = new_package_target->Id();

                if (new_package_target->IsObjective())
                    mis.target_num = ((Objective)new_package_target)->GetBestTarget();

                mis.vs = new_package_target->GetTeam();
            }

            mis.tot = gPackageTOT;

            if (gPackageTOT)
                mis.tot_type = TYPE_EQ;
            else
                mis.tot_type = TYPE_NE;

            mis.tx = MapX;
            mis.ty = MapY;
            mis.mission = static_cast<uchar>(type);
            mis.priority = static_cast<short>(gPackagePriority);
            mis.aircraft = 2;
            mis.flags or_eq REQF_ALLOW_ERRORS bitor REQF_TE_MISSION;

            new_package->SetUnitDestination(MapX, MapY);
            new_package->SetLocation(MapX, MapY);
            *(new_package->GetMissionRequest()) = mis;
            new_package->SetPackageFlags(MissionData[mis.mission].flags);
            new_package->SetFinal(0);
            new_package->SetOwner(gSelectedTeam);
        }
    }
}

void SetPackageTimes(Package new_package, CampaignTime takeoffTime, CampaignTime targetTime)
{
    Flight flight;
    WayPoint w;
    int delta = 0, count = 0;

    if ( not new_package)
        return;

    flight = (Flight) new_package->GetFirstUnitElement();

    while (flight)
    {
        w = flight->GetFirstUnitWP();

        if (w and not flight->Moving())
        {
            if (takeoffTime)
                delta = takeoffTime - w->GetWPDepartureTime();

            if (count)
                delta += MissionData[flight->GetUnitMission()].separation * CampaignSeconds;
            else if (targetTime)
            {
                while (w and not (w->GetWPFlags() bitand WPF_TARGET))
                    w = w->GetNextWP();

                if (w)
                    delta = (targetTime + MissionData[flight->GetUnitMission()].separation * CampaignSeconds) - w->GetWPArrivalTime();
            }

            if (delta)
                SetWPTimes(flight->GetFirstUnitWP(), delta, 0);
        }

        count++;
        flight = (Flight) new_package->GetNextUnitElement();
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//Flight Stuff
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void tac_select_aircraft(long, short hittype, C_Base *control)
{
    C_ListBox *lbox;
    int i, ac_type, mission = -1;
    C_Window *win;

    if (hittype not_eq C_TYPE_SELECT)
        return;

    win = gMainHandler->FindWindow(TAC_FLIGHT_WIN);

    if ( not win)
        return;

    gLastAircraftType = ((C_ListBox*)control)->GetTextID();
    ac_type = gLastAircraftType - VU_LAST_ENTITY_TYPE;

    // Everything else is based off of aircraft type

    // Setup role listbox
    lbox = (C_ListBox *)win->FindControl(TAC_ROLE);

    if (lbox)
    {
        // Clear old roles
        lbox->RemoveAllItems();

        for (i = 1; i < AMIS_OTHER; i++)
        {
            if (IsValidMission(ac_type, i))
            {
                lbox->AddItem(i, C_TYPE_ITEM, MissStr[i]);

                if (mission < 0)
                    mission = i;

                if (i == gLastRole)
                    lbox->SetValue(i);
            }
        }

        lbox->SetCallback(tac_select_role);
        tac_select_role(mission, C_TYPE_SELECT, lbox);
    }

    // Setup Squadron listbox - list all squadrons of aircraft type above, plus "new"
    lbox = (C_ListBox*)win->FindControl(TAC_SQUADRON_LIST);

    if (lbox)
    {
        lbox->RemoveAllItems();
        FillListBoxWithSquadrons(lbox, gSelectedTeam, ac_type);
        lbox->SetCallback(tac_select_squadron);
        // Now setup the airbase list (result of squadron selection)
        tac_select_squadron(lbox->GetTextID(), C_TYPE_SELECT, lbox);
    }
}

// 2001-11-19 M.N. Pilot skill

void tac_select_skill(long, short hittype, C_Base *control)
{
    int mission = -1;
    C_Window *win;

    if (hittype not_eq C_TYPE_SELECT)
        return;

    win = gMainHandler->FindWindow(TAC_FLIGHT_WIN);

    if ( not win)
        return;

    gLastPilotSkill = ((C_ListBox*)control)->GetTextID();
}

void tac_select_role(long, short hittype, C_Base *control)
{
    C_ListBox *lbox;
    C_Window *win;
    int mission;

    if (hittype not_eq C_TYPE_SELECT)
        return;

    win = gMainHandler->FindWindow(TAC_FLIGHT_WIN);

    if ( not win)
        return;

    gLastRole = mission = ((C_ListBox*)control)->GetTextID();

    // Setup target listbox (based off of role)
    lbox = (C_ListBox *)win->FindControl(TARGET_LIST);

    if (lbox)
    {
        VuListIterator cit(AllCampList);
        CampEntity ent;
        _TCHAR buffer[80] = {0};
        int gott = 0;

        // Clear old targets
        lbox->RemoveAllItems();

        // Check for location target
        if (IsValidTarget(gSelectedTeam, mission, NULL))
        {
            AddLocationToBuffer('n', MapX, MapY, buffer);
            lbox->AddItem(TAC_LOCATION, C_TYPE_ITEM, buffer);

            if ( not gLastTarget)
                lbox->SetValue(TAC_LOCATION);

            gott++;
        }

        // Check all entities
        ent = (CampEntity) cit.GetFirst();

        while (ent)
        {
            if (IsValidTarget(gSelectedTeam, mission, ent))
            {
                if (ent->IsFlight())
                    GetCallsign(((Flight)ent)->callsign_id, ((Flight)ent)->callsign_num, buffer);
                else if (ent->IsObjective())
                    ent->GetName(buffer, 79, TRUE);
                else
                    ent->GetName(buffer, 79, FALSE);

                lbox->AddItem(ent->GetCampID(), C_TYPE_ITEM, buffer);

                if (ent == gLastTarget)
                    lbox->SetValue(ent->GetCampID());

                gott++;
            }

            ent = (CampEntity) cit.GetNext();
        }

        if ( not gott)
        {
            _TCHAR tmp[40];
            ReadIndexedString(257, tmp, 39);
            lbox->AddItem(TAC_NOTARGET, C_TYPE_ITEM, tmp);
            lbox->SetValue(TAC_NOTARGET);
        }

        lbox->SetCallback(tac_select_target);
    }
}

void tac_select_squadron(long ID, short hittype, C_Base *control)
{
    C_ListBox *lbox;
    UI_Refresher *urec;
    Squadron sqd;
    C_Window *win;
    _TCHAR buffer[50];

    if (hittype not_eq C_TYPE_SELECT)
        return;

    win = gMainHandler->FindWindow(TAC_FLIGHT_WIN);

    if ( not win)
        return;

    ID = ((C_ListBox*)control)->GetTextID();

    // Now setup the airbase list
    lbox = (C_ListBox*)win->FindControl(TAC_AIRBASE_LIST);

    if (lbox)
    {
        lbox->RemoveAllItems();

        // If we have a specific squadron selected, just show this airbase)
        if (ID > 1)
        {
            CampEntity airbase;

            urec = (UI_Refresher*)gGps->Find(ID);

            if (urec)
                sqd = (Squadron)vuDatabase->Find(urec->GetID());
            else
                sqd = NULL;

            if (sqd) // Don't use sqd if NULL
            {
                airbase = sqd->GetUnitAirbase();

                //if(airbase)
                if (airbase and not F4IsBadReadPtr(airbase, sizeof(CampBaseClass))) // JB 010326 CTD
                {
                    airbase->GetName(buffer, 40, TRUE);
                    lbox->AddItem(airbase->GetCampID(), C_TYPE_ITEM, buffer);
                    lbox->SetValue(airbase->GetCampID());
                }
            }
        }
        // Otherwise, show all airbases owned by our team
        else
        {
            VuListIterator ait(AllObjList);
            Objective o;
            _TCHAR name[80];
            GridIndex x, y;
            float dsq, bdsq = FLT_MAX;

            o = (Objective) ait.GetFirst();

            while (o)
            {
                if (o->GetType() == TYPE_AIRBASE and GetRoE(gSelectedTeam, o->GetTeam(), ROE_AIR_USE_BASES) == ROE_ALLOWED)
                {
                    // Add airbase name to listbox
                    o->GetName(name, 79, TRUE);
                    lbox->AddItem(o->GetCampID(), C_TYPE_ITEM, name);
                    o->GetLocation(&x, &y);
                    dsq = static_cast<float>(DistSqu(MapX, MapY, x, y));

                    if (dsq < bdsq)
                    {
                        gLastAirbase = o->GetCampID();
                        bdsq = dsq;
                    }

                    if (o->GetCampID() == gLastAirbase)
                        lbox->SetValue(gLastAirbase);
                }

                o = (Objective) ait.GetNext();
            }
        }

        gLastAirbase = lbox->GetTextID();
        lbox->Refresh();
    }
}

void tac_select_airbase(long, short hittype, C_Base *control)
{
    C_ListBox *lbox;

    if (hittype not_eq C_TYPE_SELECT)
        return;

    lbox = (C_ListBox*)control->Parent_->FindControl(TAC_AIRBASE_LIST);

    if (lbox)
        gLastAirbase = lbox->GetTextID();
}

void tac_select_target(long, short hittype, C_Base *control)
{
    C_ListBox *lbox;
    int cid;

    if (hittype not_eq C_TYPE_SELECT)
        return;

    lbox = (C_ListBox*)control->Parent_->FindControl(TARGET_LIST);

    if (lbox)
    {
        cid = lbox->GetTextID();

        if (cid < TAC_LOCATION)
            gLastTarget = GetEntityByCampID(cid);
    }
}

// This is called as a result of choosing to create a flight -
// either after a map click in flight add mode, or from within the tactical_add_package() function
void tactical_add_flight(VU_ID id, C_Base *caller)
{
    C_Window *win = NULL;
    C_ListBox *lbox = NULL;
    C_Button *btn = NULL;
    CampEntity ent = NULL;
    short x = 0, y = 0;
    float mx, my, maxy, scale;

    win = gMainHandler->FindWindow(TAC_FLIGHT_WIN);

    if (caller->_GetCType_() == _CNTL_BUTTON_)
    {
        mx = GridToSim(MapX);
        my = GridToSim(MapY);

        if (win)
        {
            lbox = (C_ListBox *) win->FindControl(START_AT_LIST);

            if (lbox)
                lbox->SetValue(1); // Default to "start at takeoff" for package add mode
        }
    }
    else
    {
        if (caller->_GetCType_() == _CNTL_POPUPLIST_)
        {
            gPopupMgr->GetCurrentXY(&x, &y);
            gMapMgr->GetMapRelativeXY(&x, &y);
        }
        else if (caller->_GetCType_() == _CNTL_MAP_MOVER_)
        {
            x = static_cast<short>(((C_MapMover*)caller)->GetRelX() + caller->GetX() + caller->Parent_->GetX());
            y = static_cast<short>(((C_MapMover*)caller)->GetRelY() + caller->GetY() + caller->Parent_->GetY());
            gMapMgr->GetMapRelativeXY(&x, &y);
        }

        gTakeoffTime = 0;
        gPackageTOT = TheCampaign.CurrentTime + CampaignMinutes;
        scale = gMapMgr->GetMapScale();
        maxy = gMapMgr->GetMaxY();
        mx = x / scale;
        my = maxy - y / scale;
        new_package = NULL;

        if (win)
        {
            lbox = (C_ListBox *) win->FindControl(START_AT_LIST);

            if (lbox)
                lbox->SetValue(4); // Default to "start at target" for direct add mode
        }
    }

    // Determine target (or target location)
    ent = (CampEntity) vuDatabase->Find(id);

    if (ent)
    {
        gLastTarget = new_package_target = ent;
        new_package_target->GetLocation(&MapX, &MapY);
    }
    else
    {
        MapX = SimToGrid(mx);
        MapY = SimToGrid(my);
        new_package_target = NULL;
    }

    // Determine default mission type
    if ( not gLastAircraftType)
        gLastAircraftType = GetClassID(DOMAIN_AIR, CLASS_UNIT, TYPE_SQUADRON, STYPE_UNIT_FIGHTER_BOMBER, SPTYPE_F16C, VU_ANY, VU_ANY, VU_ANY) + VU_LAST_ENTITY_TYPE;

    gLastRole = GetMissionFromTarget(gSelectedTeam, gLastAircraftType - VU_LAST_ENTITY_TYPE, ent);

    if (win)
    {
        // Setup aircraft type listbox
        lbox = (C_ListBox*)win->FindControl(TAC_AIRCRAFT_TYPE);

        if (lbox)
        {
            lbox->RemoveAllItems();
            FillListBoxWithACTypes(lbox);

            if ( not lbox->GetRoot())
            {
                AreYouSure(TXT_ERROR, TXT_NO_SQUADRONS_AVAIL, NULL, CloseWindowCB);
                return;
            }

            // Select last type selected
            lbox->SetValue(gLastAircraftType);
            lbox->SetCallback(tac_select_aircraft);
            // Everything else is based off of aircraft type
            tac_select_aircraft(0, C_TYPE_SELECT, lbox);
        }

        // 2001-11-19 M.N. we want to have a pilots skill selection in the TE editor

        if ( not gLastPilotSkill)
            gLastPilotSkill = 3; // Rookie as default

        lbox = (C_ListBox*)win->FindControl(PILOT_SKILL);

        if (lbox)
        {
            lbox->SetValue(gLastPilotSkill);
            lbox->SetCallback(tac_select_skill);
            tac_select_skill(0, C_TYPE_SELECT, lbox);
        }

        // Setup cancel and ok
        btn = (C_Button*)win->FindControl(CANCEL_FLIGHT);

        if (btn)
            btn->SetCallback(CloseWindowCB);

        btn = (C_Button*)win->FindControl(OK_FLIGHT);

        if (btn)
            btn->SetCallback(tactical_make_flight);

        gMainHandler->EnableWindowGroup(win->GetGroup());
    }
}

// This is called when we've decided to actually create a requested flight
void tactical_make_flight(long ID, short hittype, C_Base *control)
{
    C_Window *win;
    UI_Refresher *urec = NULL;
    Flight new_flight = NULL;
    Squadron squadron = NULL;
    WayPoint w;
    C_ListBox *lbox;
    int tid;
    GridIndex x, y;
    int num_vehicles = 0, start_at = 1, done = 0, flights = 0, error;
    VU_ID pid, sid;
    long scampid, acampid, ac_type = 0;
    // VehicleClassDataType *vc=NULL;
    CampEntity target = NULL;
    MissionRequestClass mis;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    gMainHandler->DisableWindowGroup(control->Parent_->GetGroup());

    // Create our package, if we don't current have one
    if ( not new_package)
        tactical_make_package(ID, hittype, control);

    ShiAssert(new_package);

    if ( not new_package)
        return;

    win = gMainHandler->FindWindow(TAC_FLIGHT_WIN);

    if (win)
    {
        lbox = (C_ListBox*)win->FindControl(TAC_AIRCRAFT_TYPE);

        if (lbox)
            ac_type = lbox->GetTextID();

        lbox = (C_ListBox*)win->FindControl(TAC_SQUADRON_LIST);

        if (lbox)
        {
            scampid = lbox->GetTextID();

            if (scampid == 1)
            {
                lbox = (C_ListBox*)win->FindControl(TAC_AIRBASE_LIST);

                if (lbox)
                {
                    acampid = lbox->GetTextID();
                    urec = (UI_Refresher*)gGps->Find(acampid);

                    if (urec)
                        squadron = tactical_make_squadron(urec->GetID(), ac_type);
                }
            }
            else
            {
                // Use this squadron
                urec = (UI_Refresher*)gGps->Find(scampid);

                if ( not urec)
                {
                    MonoPrint("Selected Squadron NOT found\n");
                    return;
                }

                squadron = (Squadron)vuDatabase->Find(urec->GetID());
            }
        }

        if ( not squadron)
            return;

        // Get our flight size
        lbox = (C_ListBox *) win->FindControl(TAC_FLIGHT_SIZE);

        if (lbox)
            num_vehicles = lbox->GetTextID();

        if ( not num_vehicles)
            return;

        tactical_update_package();

        // Find our selected target
        lbox = (C_ListBox *) win->FindControl(TARGET_LIST);

        if (lbox)
        {
            int cid = lbox->GetTextID();

            if (cid < TAC_LOCATION)
                target = GetEntityByCampID(cid);
            else
                target = NULL;

            // Set our package targe to our target if we're the first element
            if (new_package and not new_package->GetFirstUnitElement())
                new_package_target = target;
        }

        lbox = (C_ListBox *) win->FindControl(START_AT_LIST);

        if (lbox)
            start_at = lbox->GetTextID();

        tid = GetClassID(DOMAIN_AIR, CLASS_UNIT, TYPE_FLIGHT, squadron->GetSType(), squadron->GetSPType(), 0, 0, 0);

        if ( not tid)
            return;

        tid += VU_LAST_ENTITY_TYPE;

        // M.N. Set the squadron's pilot experience
        int i;

        for (i = 0; i < PILOTS_PER_SQUADRON; i++)
        {
            ((Squadron)squadron)->GetPilotData(i)->SetTEPilotRating(gLastPilotSkill - 1); // 0-4 = Recruit->Ace
        }

        // Count current flights in package
        while (new_package->GetUnitElement(flights))
            flights++;

        // Add the new flight
        new_flight = NewFlight(tid, new_package, squadron);

        if ( not new_flight)
            return;

        // Build the flight's mission request
        mis = *(new_package->GetMissionRequest());
        mis.aircraft = static_cast<uchar>(num_vehicles);
        mis.priority = static_cast<short>(gPackagePriority);
        lbox = (C_ListBox*)win->FindControl(TAC_ROLE);

        if (lbox)
            mis.mission = static_cast<uchar>(lbox->GetTextID());
        else
            mis.mission = AMIS_TRAINING;

        if ( not mis.tx or not mis.ty)
        {
            // This is probably a package being editing, and the target x,y have got nuked.
            // We can get them from the package destination
            new_package->GetUnitDestination(&mis.tx, &mis.ty);
        }

        // 2001-04-07 HACK ADDED By S.G. IF WE ARE ADDING AN ECM FLIGHT, FLAG THE PACKAGE HAS HAVING ECM PROTECTION
        // 2001-04-07 HACK ADDED By S.G. ALSO, WE MUST SET THE FLIGHT'S MISSION targetID TO WHATEVER TARGET WAS GIVEN IN 'target'
        if (mis.mission == AMIS_ECM)
        {
            // Only if there's not already an ECM flight attached to it...
            if (new_package->GetECM() == FalconNullId)
                new_package->SetECM(new_flight->Id());
        }

        if (target)
            mis.targetID = target->Id();

        // END OF ADDED SECTION

        // Setup our 'fixed' time correctly
        mis.tot = TheCampaign.CurrentTime;

        if (gPackageTOT)
        {
            // Locked time on target
            mis.tot_type = TYPE_EQ;
            mis.tot = gPackageTOT;
        }
        else if (start_at == 1)
        {
            mis.tot_type = TOT_TAKEOFF;

            if (gTakeoffTime)
                mis.tot = gTakeoffTime;
            else
                mis.tot = TheCampaign.CurrentTime + CampaignMinutes;
        }
        else if (start_at == 2)
            mis.tot_type = TOT_ENROUTE;
        else if (start_at == 3)
            mis.tot_type = TOT_INGRESS;
        else
        {
            mis.tot_type = TYPE_EQ;
            mis.tot = TheCampaign.CurrentTime + CampaignMinutes;
        }

        if ( not (TheCampaign.Flags bitand CAMP_TACTICAL_EDIT) and mis.tot < TheCampaign.CurrentTime)
            mis.tot = TheCampaign.CurrentTime + 3 * CampaignSeconds;

        // Adjust for additional flights (flights is # of previous flights in the package)
        /* if (MissionData[mis.mission].loitertime)
         mis.tot += MissionData[mis.mission].loitertime * flights * CampaignMinutes;
         else
         mis.tot += (CampaignTime)((flights) * 10 *CampaignSeconds);
        */
        if (flights)
            mis.tot += MissionData[mis.mission].separation * CampaignSeconds;

        // Set flight's location to home base for now
        squadron->GetLocation(&x, &y);
        new_flight->SetLocation(x, y);
        new_flight->SetOwner(squadron->GetOwner());
        squadron->FindAvailableAircraft(&mis);

        if (mis.mission == AMIS_AIRCAV)
        {
            Unit unit = (Unit) vuDatabase->Find(mis.requesterID);
            GridIndex ux, uy;

            if (unit)
            {
                unit->GetLocation(&ux, &uy);
                mis.tx = static_cast<short>(ux + 20);
            }

            if (start_at > 3)
                mis.tot += 10 * CampaignMinutes; // Try and get us in front of our pickup point
        }

        mis.flags or_eq REQF_ALLOW_ERRORS bitor REQF_TE_MISSION;
        error = new_flight->BuildMission(&mis);

        if (error not_eq PRET_SUCCESS)
        {
            // Show an error message box notifying user this action was not able to be performed
            // Errors are: PRET_NO_ASSETS - The aircraft wern't available
            // PRET_ABORTED - Timing was impossible (takeoff before current time, for example)
            MonoPrint("Error planning flight. Aborting\n");
            AreYouSure(TXT_FLIGHT_CANCELED, TXT_ERROR, CloseWindowCB, CloseWindowCB);
            new_package->CancelFlight(new_flight);
            return;
        }

        new_flight->SetUnitMissionTarget(mis.targetID);

        // KCK: Traverse all waypoints and only lock the time of either the target or takeoff
        w = new_flight->GetFirstUnitWP();

        while (w)
        {
            if (gTakeoffTime)
            {
                if (w->GetWPAction() == WP_TAKEOFF)
                    w->SetWPFlag(WPF_TIME_LOCKED);
                else
                    w->UnSetWPFlag(WPF_TIME_LOCKED);
            }
            else
            {
                if ((w->GetWPFlags() bitand WPF_TARGET) and not done)
                {
                    w->SetWPFlag(WPF_TIME_LOCKED);
                    done = 1;
                }
                else
                    w->UnSetWPFlag(WPF_TIME_LOCKED);
            }

            w = w->GetNextWP();
        }

        new_package->RecordFlightAddition(new_flight, &mis, 0);
        new_flight->SetFinal(1);
        fixup_unit(new_flight);
        gGps->Update();

        gMapMgr->SetCurrentWaypointList(new_flight->Id());
        SetupFlightSpecificControls(new_flight);
        gMapMgr->DrawMap();
        gCurrentFlightID = new_flight->Id();
        gSelectedFlightID = new_flight->Id();

        if (WeAreAddingPackage)
        {
            win = gMainHandler->FindWindow(PACKAGE_WIN);

            if (win)
            {
                C_ATO_Flight *atoflt;
                C_TreeList *tree;
                TREELIST *item;
                tree = (C_TreeList *)win->FindControl(ATO_PACKAGE_TREE);

                if (tree)
                {
                    tree->SetCallback(tactical_new_flight_select);

                    atoflt = BuildATOFlightInfo(new_flight);

                    if (atoflt)
                    {
                        item = tree->CreateItem(new_flight->GetCampID(), C_TYPE_ITEM, atoflt);

                        if (item)
                            tree->AddItem(tree->GetRoot(), item);
                    }

                    tree->RecalcSize();
                    tree->Parent_->RefreshClient(tree->GetClient());
                }
                else
                    new_package->SetLocation(MapX, MapY);
            }
        }
        else
        {
            new_package->SetFinal(1);
            new_package = NULL;
        }
    }

    display_air_units(new_flight);
    new_flight = NULL;
    // MN 2002-01-04 show up the flight faster
    // RefreshMapOnChange();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void display_air_units(Unit u)
{
    int stype;
    C_PopupList *menu;

    menu = gPopupMgr->GetMenu(MAP_POP);

    if ( not menu)
        return;

    // Make sure we can see this unit
    stype = u->GetSType();

    if (stype == STYPE_UNIT_FIGHTER or stype == STYPE_UNIT_FIGHTER_BOMBER)
    {
        menu->SetItemState(MID_UNITS_SQUAD_FIGHTER, 1);
        MenuToggleUnitCB(MID_UNITS_SQUAD_FIGHTER, 0, menu);
    }
    else if (stype == STYPE_UNIT_BOMBER)
    {
        menu->SetItemState(MID_UNITS_SQUAD_BOMBER, 1);
        MenuToggleUnitCB(MID_UNITS_SQUAD_BOMBER, 0, menu);
    }
    else if (stype == STYPE_UNIT_ATTACK_HELO or stype == STYPE_UNIT_RECON_HELO or stype == STYPE_UNIT_TRANSPORT_HELO)
    {
        menu->SetItemState(MID_UNITS_SQUAD_HELI, 1);
        MenuToggleUnitCB(MID_UNITS_SQUAD_HELI, 0, menu);
    }
    else if (stype == STYPE_UNIT_ATTACK)
    {
        menu->SetItemState(MID_UNITS_SQUAD_ATTACK, 1);
        MenuToggleUnitCB(MID_UNITS_SQUAD_ATTACK, 0, menu);
    }
    else
    {
        menu->SetItemState(MID_UNITS_SQUAD_SUPPORT, 1);
        MenuToggleUnitCB(MID_UNITS_SQUAD_SUPPORT, 0, menu);
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void tactical_cancel_package(long, short hittype, C_Base *ctrl)
{
    Flight flt;

    if (hittype not_eq C_TYPE_LMOUSEUP)
    {
        return;
    }

    gMainHandler->DisableWindowGroup(ctrl->Parent_->GetGroup());

    //  Delete the unwanted package
    if (new_package)
    {
        flt = (Flight)new_package->GetFirstUnitElement();

        while (flt)
        {
            if (flt->IsFlight())
            {
                RegroupFlight(flt);
                flt = (Flight)new_package->GetNextUnitElement();
            }
        }

        new_package = NULL;
    }

    gGps->Update();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Battalion Stuff
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// The id passed in should be the id of any objective we clicked on
void tactical_add_battalion(VU_ID id, C_Base *caller)
{
    C_ListBox *lbox = NULL;
    C_Window *win = NULL;
    short x = 0, y = 0;
    float mx, my, maxy, scale;
    CampBaseClass *ent = NULL;

    win = gMainHandler->FindWindow(NEW_BATT_WIN);

    if ( not win)
        return;

    if (caller->_GetCType_() == _CNTL_POPUPLIST_)
    {
        gPopupMgr->GetCurrentXY(&x, &y);
        gMapMgr->GetMapRelativeXY(&x, &y);
    }
    else if (caller->_GetCType_() == _CNTL_MAP_MOVER_)
    {
        x = static_cast<short>(((C_MapMover*)caller)->GetRelX() + caller->GetX() + caller->Parent_->GetX());
        y = static_cast<short>(((C_MapMover*)caller)->GetRelY() + caller->GetY() + caller->Parent_->GetY());
        gMapMgr->GetMapRelativeXY(&x, &y);
    }

    scale = gMapMgr->GetMapScale();
    maxy = gMapMgr->GetMaxY();
    mx = x / scale;
    my = maxy - y / scale;

    MapX = SimToGrid(mx);
    MapY = SimToGrid(my);

    // Determine target (or target location)
    ent = (CampEntity) vuDatabase->Find(id);

    if (ent and ent->IsObjective())
    {
        ent->GetLocation(&MapX, &MapY);
        gLastBattalionObjID = ent->Id();
    }
    else
    {
        gLastBattalionObjID = FalconNullId;
        MapX = SimToGrid(mx);
        MapY = SimToGrid(my);
    }

    // Setup list box for our team (or last equipment choice)
    lbox = (C_ListBox *) win->FindControl(UNIT_TOE);

    if (lbox)
    {
        // KCK Hack: Convert from flags to his equipment table
        if (gLastEquipment)
            lbox->SetValue(gLastEquipment);
        else if (TeamInfo[gSelectedTeam]->GetFlag() == COUN_RUSSIA)
            lbox->SetValue(toe_soviet);
        else if (TeamInfo[gSelectedTeam]->GetFlag() == COUN_CHINA)
            lbox->SetValue(toe_chinese);
        else if (TeamInfo[gSelectedTeam]->GetFlag() == COUN_SOUTH_KOREA)
            lbox->SetValue(toe_rok);
        else if (TeamInfo[gSelectedTeam]->GetFlag() == COUN_NORTH_KOREA)
            lbox->SetValue(toe_dprk);
        else
            lbox->SetValue(toe_us);
    }

    if (gLastEquipment and gLastUnitType not_eq -1)
    {
        lbox = (C_ListBox *) win->FindControl(UNIT_TYPE);

        if (lbox)
            lbox->SetValue(gLastUnitType);
    }

    update_new_battalion_window();
    gMainHandler->EnableWindowGroup(32000);
}

Objective FindValidObjective(Battalion bat, VU_ID current_obj, GridIndex x, GridIndex y)
{
    Objective o = (Objective)vuDatabase->Find(current_obj);
    //float last=-1.0F;

    // Non-mobile battalions always snap to their objectives
    // So clear their objective
    if (bat->GetMovementType() == NoMove)
    {
        gLastBattalionObjID = FalconNullId;
        o = NULL;
    }

    // Snap battalion to nearest objective
    /*KCK: Commented out as per a beta-tester request. Let's see how it works
    if (bat->GetUnitNormalRole() == GRO_AIRDEFENSE)
     {
     // Air defense units need to snap to SAM sites
     if ( not o)
     o = FindNearestObjective(x,y,&last,999);
     while (o and not o->SamSite())
     o = FindNearestObjective(x,y,&last,999);
     }
    */
    if ( not o)
    {
        o = FindNearestObjective(x, y, NULL, 999);
    }

    return o;
}

void tactical_set_orders(Battalion bat, VU_ID obj, GridIndex tx, GridIndex ty)
{
    Objective o;
    WayPoint wp, nw;
    GridIndex x, y, xd, yd;
    int role, oldgs;

    o = FindValidObjective(bat, obj, tx, ty);

    if ( not o or not bat)
    {
        return;
    }

    CampEnterCriticalSection();

    if (o->GetTeam() == bat->GetTeam())
    {
        role = bat->GetUnitNormalRole();

        if (role == GRO_AIRDEFENSE and o->SamSite())
            bat->SetUnitOrders(GORD_AIRDEFENSE, o->Id());
        else if (role == GRO_FIRESUPPORT)
            bat->SetUnitOrders(GORD_SUPPORT, o->Id());
        else if (role == GRO_DEFENSE or role == GRO_ATTACK or role == GRO_RECON)
            bat->SetUnitOrders(GORD_DEFEND, o->Id());
        else
            bat->SetUnitOrders(GORD_RESERVE, o->Id());
    }
    else
    {
        bat->SetUnitOrders(GORD_CAPTURE, o->Id());
    }

    o->GetLocation(&x, &y);
    bat->SetUnitDestination(x, y);

    if (bat->GetMovementType() not_eq NoMove)
    {
        oldgs = OBJ_GROUND_PATH_MAX_SEARCH;
        OBJ_GROUND_PATH_MAX_SEARCH = MAX_SEARCH;
        bat->BuildMission();
        OBJ_GROUND_PATH_MAX_SEARCH = static_cast<short>(oldgs);
        bat->GetLocation(&xd, &yd);

        // Tack on a waypoint at our current location, if we don't have one
        wp = bat->GetFirstUnitWP();

        if (wp)
        {
            wp->GetWPLocation(&x, &y);
        }

        if ( not wp or x not_eq xd or y not_eq yd)
        {
            wp = bat->AddWPAfter(NULL, xd, yd, 0, 0, TheCampaign.CurrentTime, 0, 0);
        }

        // Tack on a waypoint at our destination location, if we don't have one or need at least two waypoints
        bat->GetUnitDestination(&xd, &yd);

        while (wp and wp->GetNextWP())
        {
            wp = wp->GetNextWP();
        }

        wp->GetWPLocation(&x, &y);

        if (x not_eq xd or y not_eq yd or wp == bat->GetFirstUnitWP())
        {
            nw = new WayPointClass(xd, yd, 0, 0, wp->GetWPDepartureTime() + 30 * CampaignMinutes, 0, 0, 0);
            wp->InsertWP(nw);
        }
    }
    else
    {
        bat->SetLocation(x, y);
    }

    // If we don't own this unit, send the orders to the host
    if ( not bat->IsLocal())
    {
        FalconSessionEntity *host = (FalconSessionEntity*) vuDatabase->Find(FalconLocalGame->OwnerId());
        FalconCampDataMessage *msg = new FalconCampDataMessage(bat->Id(), host);
        uchar *dataptr;
        VU_ID tmpId;

        msg->dataBlock.type = FalconCampDataMessage::campOrdersData;
        // Encode objective and location
        msg->dataBlock.size = sizeof(tmpId) + sizeof(x) + sizeof(y);
        msg->dataBlock.data = dataptr = new uchar[msg->dataBlock.size];
        tmpId = o->Id();
        memcpy(dataptr, &tmpId, sizeof(tmpId));
        dataptr += sizeof(tmpId);
        memcpy(dataptr, &x, sizeof(x));
        dataptr += sizeof(x);
        memcpy(dataptr, &y, sizeof(y));
        dataptr += sizeof(y);
        FalconSendMessage(msg, TRUE);
    }
    else
    {
        // Otherwise dirty the necessary data
        //bat->MakeUnitDirty (DIRTY_WP_LIST, DDP[177].priority);
        bat->MakeUnitDirty(DIRTY_WP_LIST, SEND_NOW);
        //bat->MakeUnitDirty (DIRTY_DESTINATION, DDP[178].priority);
        bat->MakeUnitDirty(DIRTY_DESTINATION, SEND_NOW);
        //bat->MakeUnitDirty (DIRTY_WAYPOINT, DDP[179].priority);
        bat->MakeUnitDirty(DIRTY_WAYPOINT, SEND_NOW);
    }

    CampLeaveCriticalSection();

    if (gActiveFlightID == bat->Id())
    {
        gMapMgr->SetCurrentWaypointList(bat->Id());
        RefreshMapOnChange();
    }
}

void tactical_create_battalion(long, short hittype, C_Base *)
{
    Battalion new_battalion;
    int tid;
    Objective o;
    GridIndex x, y;

    LoadTEUnits();

    if (gLastUnitType < 0)
        return;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    tid = GetClassID(DOMAIN_LAND, CLASS_UNIT, TYPE_BATTALION, table_of_equipment[gLastUnitType].stype, table_of_equipment[gLastUnitType].sptype, 0, 0, 0);

    if ( not tid)
        return;

    new_battalion = NewBattalion(tid + VU_LAST_ENTITY_TYPE, NULL);

    if ( not new_battalion)
        return;

    new_battalion->SetOwner(gSelectedTeam);

    // Snap battalion to nearest objective
    o = FindValidObjective(new_battalion, gLastBattalionObjID, MapX, MapY);

    if (o)
    {
        o->GetLocation(&x, &y);
        new_battalion->SetLocation(x, y);
        new_battalion->SetUnitDestination(x, y);
    }
    else
    {
        new_battalion->SetLocation(MapX, MapY);
        new_battalion->SetUnitDestination(MapX, MapY);
    }

    tactical_set_orders(new_battalion, gLastBattalionObjID, MapX, MapY);

    if (new_battalion->GetFirstUnitWP())
        new_battalion->SetCurrentWaypoint(1);

    new_battalion->BuildElements();
    new_battalion->SetFinal(1);
    // sfr: new insertion
    new_battalion->SetSendCreate(VuEntity::VU_SC_SEND_OOB);
    vuDatabase->/*Quick*/Insert(new_battalion);

    new_battalion->SetEmitting(0);

    if (new_battalion->GetRadarType() not_eq RDR_NO_RADAR)
        new_battalion->SetEmitting(1);

    for (int i = 0; i < NUM_TEAMS; i++)
        new_battalion->SetSpotted(static_cast<uchar>(i), TheCampaign.CurrentTime);

    gMainHandler->DisableWindowGroup(32000);
    display_land_units(new_battalion);
    update_new_battalion_window();
    // MN 2002-01-04 show up the battalion faster
    RefreshMapOnChange();
}

void tactical_cancel_battalion(long, short hittype, C_Base *)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    gMainHandler->DisableWindowGroup(32000);
}

static void set_battalion_table_of_equipment(long, short hittype, C_Base *ctrl)
{
    C_ListBox *list;

    if (hittype not_eq C_TYPE_SELECT)
        return;

    list = (C_ListBox *) ctrl;
    gLastEquipment = list->GetTextID();

    update_new_battalion_window();
}

static void set_battalion_type(long, short hittype, C_Base *ctrl)
{
    C_ListBox *list;

    if (hittype not_eq C_TYPE_SELECT)
        return;

    list = (C_ListBox *) ctrl;

    if (list)
        gLastUnitType = list->GetTextID();

    update_new_battalion_window();
}

static void update_new_battalion_window(void)
{
    C_Window *win;
    C_Text *text;
    C_ListBox *list;
    UnitClassDataType *uc;
    VehicleClassDataType *vc;
    short types[VEHICLE_GROUPS_PER_UNIT];
    static int batt[8] = {BATT_VEH1, BATT_VEH2, BATT_VEH3, BATT_VEH4, BATT_VEH5, BATT_VEH6, BATT_VEH7, BATT_VEH8};
    static int last_equip = -1;
    int num[VEHICLE_GROUPS_PER_UNIT], tid, equipment = -1, prev, loop;
    _TCHAR buffer[100];

    win = gMainHandler->FindWindow(NEW_BATT_WIN);

    if ( not win)
        return;

    LoadTEUnits();

    list = (C_ListBox *) win->FindControl(UNIT_TOE);

    if (list)
        equipment = list->GetTextID();

    if (last_equip not_eq equipment or gLastUnitType < 0)
    {
        gLastEquipment = last_equip = equipment;
        gLastUnitType = -1;
        list = (C_ListBox *) win->FindControl(UNIT_TYPE);

        if (list)
        {
            // Rebuild the list of equipment
            list->RemoveAllItems();

            for (loop = 0; table_of_equipment[loop].side; loop ++)
            {
                if (table_of_equipment[loop].side == equipment)
                {
                    tid = GetClassID(DOMAIN_LAND, CLASS_UNIT, TYPE_BATTALION, table_of_equipment[loop].stype, table_of_equipment[loop].sptype, 0, 0, 0);

                    if (tid)
                    {
                        uc = (UnitClassDataType*) Falcon4ClassTable[tid].dataPtr;

                        if (uc)
                        {
                            vc = (VehicleClassDataType*) Falcon4ClassTable[uc->VehicleType[0]].dataPtr;

                            if ((vc) and (uc->VehicleType[0]))
                            {
                                sprintf(buffer, "%s - %s", uc->Name, vc->Name);
                                list->AddItem(loop, C_TYPE_ITEM, buffer);
                            }
                        }
                    }

                    // Record default
                    if (gLastUnitType < 0)
                        gLastUnitType = loop;
                }
            }

            list->SetValue(gLastEquipment);
            list->Refresh();
        }
    }

    tid = GetClassID(DOMAIN_LAND, CLASS_UNIT, TYPE_BATTALION, table_of_equipment[gLastUnitType].stype, table_of_equipment[gLastUnitType].sptype, 0, 0, 0);
    uc = (UnitClassDataType*) Falcon4ClassTable[tid].dataPtr;

    for (loop = 0; loop < VEHICLE_GROUPS_PER_UNIT; loop ++)
    {
        types[loop] = 0;
        num[loop] = 0;
    }

    if (uc)
    {
        for (loop = 0; loop < VEHICLE_GROUPS_PER_UNIT; loop ++)
        {
            if (uc->NumElements[loop])
            {
                for (prev = 0; prev < VEHICLE_GROUPS_PER_UNIT; prev ++)
                {
                    if (types[prev] == uc->VehicleType[loop])
                    {
                        num[prev] += uc->NumElements[loop];
                        break;
                    }

                    if (types[prev] == 0)
                    {
                        types[prev] = uc->VehicleType[loop];
                        num[prev] = uc->NumElements[loop];
                        break;
                    }
                }
            }
        }

        for (loop = 0; loop < VEHICLE_GROUPS_PER_UNIT; loop ++)
        {
            if (types[loop])
                vc = (VehicleClassDataType*) Falcon4ClassTable[types[loop]].dataPtr;

            text = (C_Text *) win->FindControl(batt[loop]);

            if (text)
            {
                if (types[loop])
                {
                    vc = (VehicleClassDataType*) Falcon4ClassTable[types[loop]].dataPtr;
                    sprintf(buffer, "%d x %s", num[loop], vc->Name);
                    text->SetText(buffer);
                }
                else
                    text->SetText("");

                text->Refresh();
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void display_land_units(Unit u)
{
    C_PopupList *menu;

    menu = gPopupMgr->GetMenu(MAP_POP);

    if ( not menu)
        return;

    // Make sure we can see this unit
    menu->SetItemState(MID_UNITS_BAT, 1);
    MenuToggleUnitCB(MID_UNITS_BAT, 0, menu);

    if (u->GetUnitNormalRole() == GRO_AIRDEFENSE)
    {
        menu->SetItemState(MID_UNITS_AD, 1);
        MenuToggleUnitCB(MID_UNITS_AD, 0, menu);
    }
    else if (FindUnitSupportRole(u))
    {
        menu->SetItemState(MID_UNITS_SUPPORT, 1);
        MenuToggleUnitCB(MID_UNITS_SUPPORT, 0, menu);
    }
    else
    {
        menu->SetItemState(MID_UNITS_COMBAT, 1);
        MenuToggleUnitCB(MID_UNITS_COMBAT, 0, menu);
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void delete_all_units_for_team(int old_team)
{
    VuListIterator
    iter(AllAirList);

    Unit
    unit;

    unit = GetFirstUnit(&iter);

    while (unit)
    {
        if (unit->GetTeam() == old_team)
        {
            unit->Remove();
        }

        unit = GetNextUnit(&iter);
    }

    gMapMgr->RemoveAllWaypoints(static_cast<short>(old_team));

    MonoPrint("\n");

    gGps->Update();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void tactical_remove_squadron(SquadronClass *squadron)
{
    VuListIterator
    iter(AllAirList);

    Flight
    flight;

    int
    type;

    Unit
    unit;

    unit = GetFirstUnit(&iter);

    gMapMgr->RemoveAllWaypoints(squadron->GetTeam());

    while (unit)
    {
        type = unit->GetType();

        if (type == TYPE_FLIGHT)
        {
            flight = (Flight) unit;

            if (flight->GetUnitSquadron() == squadron)
            {
                MonoPrint("DELETE > ");
            }

            MonoPrint
            (
                "Flight %d %d\n",
                flight->GetCampID(),
                flight->GetUnitPackage()->GetCampID()
            );

            // gMapMgr->SetWaypointList (flight->Id ());

            if (flight->GetUnitSquadron() == squadron)
            {
                RegroupFlight(flight);
                //flight->Remove ();
            }
        }

        unit = GetNextUnit(&iter);
    }

    MonoPrint("\n");

    gGps->Update();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int get_tactical_number_of_aircraft(int team)
{
    VuListIterator
    iter(AllAirList);

    Squadron
    squadron;

    Flight
    flight;

    Unit
    unit;

    int
    num,
    type;

    if ( not TeamInfo[team])
    {
        return 0;
    }

    unit = GetFirstUnit(&iter);

    num = 0;

    while (unit)
    {
        type = unit->GetType();

        if (type == TYPE_SQUADRON)
        {
            squadron = (Squadron) unit;

            if (squadron->GetTeam() == team)
            {
                num += squadron->GetTotalVehicles();
            }
        }
        else if (type == TYPE_FLIGHT)
        {
            flight = (Flight) unit;

            if (flight->GetTeam() == team)
            {
                num += flight->GetTotalVehicles();
            }
        }

        unit = GetNextUnit(&iter);
    }

    return num;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int get_tactical_number_of_f16s(int team)
{
    VuListIterator
    iter(AllAirList);

    Squadron
    squadron;

    Flight
    flight;

    Unit
    unit;

    int
    num,
    type;

    if ( not TeamInfo[team])
    {
        return 0;
    }

    unit = GetFirstUnit(&iter);

    num = 0;

    while (unit)
    {
        type = unit->GetType();

        if (type == TYPE_SQUADRON)
        {
            squadron = (Squadron) unit;

            if (squadron->GetTeam() == team)
            {
                if (squadron->GetVehicleID(0) == 0xad)
                {
                    num += squadron->GetTotalVehicles();
                }
            }
        }
        else if (type == TYPE_FLIGHT)
        {
            flight = (Flight) unit;

            if (flight->GetTeam() == team)
            {
                if (flight->GetVehicleID(0) == 0xad)
                {
                    num += flight->GetTotalVehicles();
                }
            }
        }

        unit = GetNextUnit(&iter);
    }

    return num;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void tactical_release_flight(long, short hittype, C_Base *)
{
    Flight
    flight;

    int
    team;

    if (hittype not_eq C_TYPE_LMOUSEUP)
    {
        return;
    }

    MonoPrint
    (
        "Release Flight %d\n",
        gSelectedATOFlight.num_
    );

    flight = (Flight) FindEntity(gSelectedATOFlight);

    if (flight)
    {
        team = flight->GetTeam();

        flight->Remove();

        gMapMgr->RemoveAllWaypoints(static_cast<short>(team));

        gGps->Update();
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
