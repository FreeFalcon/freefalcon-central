/***************************************************************************\
    Cmap.cpp
    Peter Ward
 July 15, 1996

    This code handles drawing the Campaign Map with/without units (user selectable)
\***************************************************************************/
#include <cISO646>
#include <windows.h>
#include "unit.h"
#include "team.h"
#include "CmpGlobl.h"
#include "CampCell.h"
#include "CampTerr.h"
#include "find.h"
#include "division.h"
#include "flight.h"
#include "campwp.h"
#include "cmpclass.h"
#include "campstr.h"
#include "Listadt.h"
#include "objectiv.h"
#include "Campaign.h"
#include "chandler.h"
#include "ui95_ext.h"
#include "filters.h"
#include "cmap.h" // header file for this class
#include "userids.h"
#include "textids.h"
#include "classtbl.h"
#include "falcsess.h"
#include "gps.h"
#include "urefresh.h"
#include "battalion.h"

enum
{
    PLANNER_RESOURCE = 200110,
};

extern VU_ID gSelectedPackage, gActiveFlightID;;
extern int gMoveBattalion;

extern GlobalPositioningSystem *gGps;

extern C_Handler *gMainHandler;
void WaypointCB(long ID, short hittype, C_Base *ctrl);
void UnitCB(long ID, short hittype, C_Base *ctrl);
int IsValidWP(WayPointClass *wp, Flight flt);
void Uni_Float(_TCHAR *buffer);

#define FEET_PER_PIXEL (FEET_PER_KM / 2.0f)

#define ICON_UKN 10126 // 2002-02-21 S.G.
extern int gShowUnknown; // 2002-02-21 S.G.
extern bool g_bAWACSBackground; // 2002-03-10 MN

C_Map::C_Map()
{
    CenterX_ = 0;
    CenterY_ = 0;
    ZoomLevel_ = _MAX_ZOOM_LEVEL_;
    scale_ = 1.0f;
    flags_ = I_NEED_TO_DRAW bitor I_NEED_TO_DRAW_MAP;

    MapID = 0;
    Map_ = NULL;
    memset(&MapRect_, 0, sizeof(UI95_RECT));

    memset(&TeamFlags_[0], 0, sizeof(long)*_MAX_TEAMS_);
    memset(&Team_[0], 0, sizeof(MAPICONS)*_MAX_TEAMS_);
    memset(&TeamColor_[0], 0, sizeof(COLORREF)*_MAX_TEAMS_);
    CurWP_ = NULL;
    CurWPZ_ = NULL;
    WPUnitID_ = FalconNullId;
    CurIcons_ = NULL;
    CurWPArea_.left = -1;
    CurWPArea_.top = -1;
    CurWPArea_.right = -1;
    CurWPArea_.bottom = -1;

    BullsEye_ = NULL;
    Circles_ = 0;

    ObjectiveMask_ = 0;
    UnitMask_ = 0;
    NavalUnitMask_ = 0;
    AirUnitMask_ = 0;
    ThreatMask_ = 0;

    SmallMapCtrl_ = NULL;
    DrawWindow_ = NULL;
    WPZWindow_ = NULL;
    memset(&DrawRect_, 0, sizeof(UI95_RECT));
}

C_Map::~C_Map()
{
    if (Map_ or DrawWindow_ or SmallMapCtrl_)
        Cleanup();
}

void C_Map::Cleanup()
{
    short i, j, k;

    if (DrawWindow_)
    {
        RemoveListsFromWindow();
        DrawWindow_ = NULL;
    }

    if (WPZWindow_)
        WPZWindow_ = NULL;

    if (Map_)
    {
        Map_->Cleanup();
        delete Map_;
        Map_ = NULL;
    }

    if (BullsEye_)
    {
        BullsEye_->Cleanup();
        delete BullsEye_;
        BullsEye_ = NULL;
    }

    for (i = 0; i < _MAX_TEAMS_; i++)
    {
        if (Team_[i].Threats)
        {
            for (j = 0; j < _MAP_NUM_THREAT_TYPES_; j++)
                if (Team_[i].Threats->Type[j])
                {
                    Team_[i].Threats->Type[j]->Cleanup();
                    delete Team_[i].Threats->Type[j];
                }

            delete Team_[i].Threats;
            Team_[i].Threats = NULL;
        }

        if (Team_[i].Objectives)
        {
            for (j = 0; j < _MAP_NUM_OBJ_TYPES_; j++)
                if (Team_[i].Objectives->Type[j])
                {
                    Team_[i].Objectives->Type[j]->Cleanup();
                    delete Team_[i].Objectives->Type[j];
                }

            delete Team_[i].Objectives;
            Team_[i].Objectives = NULL;
        }

        if (Team_[i].NavalUnits)
        {
            for (j = 0; j < _MAP_NUM_NAV_TYPES_; j++)
                if (Team_[i].NavalUnits->Type[j])
                {
                    Team_[i].NavalUnits->Type[j]->Cleanup();
                    delete Team_[i].NavalUnits->Type[j];
                }

            delete Team_[i].NavalUnits;
            Team_[i].NavalUnits = NULL;
        }

        if (Team_[i].Units)
        {
            for (j = 0; j < _MAP_NUM_GND_TYPES_; j++)
                if (Team_[i].Units->Type[j])
                {
                    if (Team_[i].Units->Type[j])
                    {
                        for (k = 0; k < _MAP_NUM_GND_LEVELS_; k++)
                            if (Team_[i].Units->Type[j]->Levels[k])
                            {
                                Team_[i].Units->Type[j]->Levels[k]->Cleanup();
                                delete Team_[i].Units->Type[j]->Levels[k];
                            }

                        delete Team_[i].Units->Type[j];
                    }
                }

            delete Team_[i].Units;
            Team_[i].Units = NULL;
        }

        if (Team_[i].AirUnits)
        {
            for (j = 0; j < _MAP_NUM_AIR_TYPES_; j++)
                if (Team_[i].AirUnits->Type[j])
                {
                    Team_[i].AirUnits->Type[j]->Cleanup();
                    delete Team_[i].AirUnits->Type[j];
                }

            delete Team_[i].AirUnits;
            Team_[i].AirUnits = NULL;
        }

        if (Team_[i].Waypoints)
        {
            Team_[i].Waypoints->Cleanup();
            delete Team_[i].Waypoints;
            Team_[i].Waypoints = NULL;
        }
    }

    if (CurIcons_)
    {
        CurIcons_->Cleanup();
        delete CurIcons_;
        CurIcons_ = NULL;
    }

    if (CurWP_)
    {
        CurWP_->Cleanup();
        delete CurWP_;
        CurWP_ = NULL;
    }

    if (CurWPZ_)
    {
        CurWPZ_->Cleanup();
        delete CurWPZ_;
        CurWPZ_ = NULL;
    }

    SmallMapCtrl_ = NULL;
}

void C_Map::RemoveAllEntities()
{
    short i, j, k;

    for (i = 0; i < _MAX_TEAMS_; i++)
    {
        if (Team_[i].Threats)
        {
            for (j = 0; j < _MAP_NUM_THREAT_TYPES_; j++)
                if (Team_[i].Threats->Type[j])
                {
                    Team_[i].Threats->Type[j]->Cleanup();
                }
        }

        if (Team_[i].Objectives)
        {
            for (j = 0; j < _MAP_NUM_OBJ_TYPES_; j++)
                if (Team_[i].Objectives->Type[j])
                {
                    Team_[i].Objectives->Type[j]->Cleanup();
                }
        }

        if (Team_[i].NavalUnits)
        {
            for (j = 0; j < _MAP_NUM_NAV_TYPES_; j++)
                if (Team_[i].NavalUnits->Type[j])
                {
                    Team_[i].NavalUnits->Type[j]->Cleanup();
                }
        }

        if (Team_[i].Units)
        {
            for (j = 0; j < _MAP_NUM_GND_TYPES_; j++)
                if (Team_[i].Units->Type[j])
                {
                    if (Team_[i].Units->Type[j])
                    {
                        for (k = 0; k < _MAP_NUM_GND_LEVELS_; k++)
                            if (Team_[i].Units->Type[j]->Levels[k])
                            {
                                Team_[i].Units->Type[j]->Levels[k]->Cleanup();
                            }
                    }
                }
        }

        if (Team_[i].AirUnits)
        {
            for (j = 0; j < _MAP_NUM_AIR_TYPES_; j++)
                if (Team_[i].AirUnits->Type[j])
                {
                    Team_[i].AirUnits->Type[j]->Cleanup();
                }
        }

        if (Team_[i].Waypoints)
        {
            Team_[i].Waypoints->Cleanup();
        }
    }

    if (CurIcons_)
    {
        CurIcons_->Cleanup();
    }

    if (CurWP_)
    {
        CurWP_->Cleanup();
    }

    if (CurWPZ_)
    {
        CurWPZ_->Cleanup();
    }
}

// Private
void C_Map::CalculateDrawingParams()
{
    float ratio;
    long pixels;

    if (Map_ == NULL or DrawWindow_ == NULL)
        return;

    ratio = (float)(DrawRect_.bottom - DrawRect_.top) / (float)(DrawRect_.right - DrawRect_.left);
    pixels = ZoomLevel_;

    MapRect_.left = FloatToInt32(CenterX_) - pixels / 2;

    if (MapRect_.left < 0)
    {
        MapRect_.left = 0;
        CenterX_ = static_cast<float>(MapRect_.left + pixels / 2);
    }

    MapRect_.right = MapRect_.left + pixels;

    if (MapRect_.right > Map_->GetW())
    {
        MapRect_.right = Map_->GetW();
        MapRect_.left = MapRect_.right - pixels;
        CenterX_ = static_cast<float>(MapRect_.right - pixels / 2);
    }

    MapRect_.top = FloatToInt32(CenterY_) - (long)((float)pixels * ratio) / 2;

    if (MapRect_.top < 0)
    {
        MapRect_.top = 0;
        CenterY_ = static_cast<float>(MapRect_.top + (long)((float)pixels * ratio) / 2);
    }

    MapRect_.bottom = MapRect_.top + (long)((float)pixels * ratio);

    if (MapRect_.bottom > Map_->GetH())
    {
        MapRect_.bottom = Map_->GetH();
        MapRect_.top = MapRect_.bottom - (long)((float)pixels * ratio);
        CenterY_ = static_cast<float>(MapRect_.bottom - (long)((float)pixels * ratio) / 2);
    }

    Map_->SetSrcRect(&MapRect_);
    Map_->SetDestRect(&DrawRect_);
    Map_->SetScaleInfo(((MapRect_.right - MapRect_.left) * 1000) / (DrawRect_.right - DrawRect_.left));

    scale_ = (float)(DrawRect_.right - DrawRect_.left) / ((float)(MapRect_.right - MapRect_.left) * FEET_PER_PIXEL);

    DrawWindow_->VX_[0] = -(short)((float)MapRect_.left * FEET_PER_PIXEL * scale_) + DrawWindow_->ClientArea_[0].left;
    DrawWindow_->VY_[0] = -(short)((float)MapRect_.top * FEET_PER_PIXEL * scale_) + DrawWindow_->ClientArea_[0].top;
    SetTeamScales();

    flags_ or_eq I_NEED_TO_DRAW bitor I_NEED_TO_DRAW_MAP;
    short x, y;
    TheCampaign.GetBullseyeLocation(&x, &y);

    if (x not_eq BullsEyeX_ or y not_eq BullsEyeY_)
    {
        SetBullsEye(x * FEET_PER_KM, (TheCampaign.TheaterSizeY - y) * FEET_PER_KM);
        DrawMap();
    }
}

THREAT_LIST *C_Map::AddThreat(CampEntity ent)
{
    THREAT_LIST *threat;
    long radar_short;
    long radar_long;
    long sam_short;
    long sam_long;
    short i;
    GridIndex x, y;

    threat = NULL;

    if (ent->IsObjective() and ent->IsEmitting())
    {
        ent->GetLocation(&x, &y);
        ShiAssert(Map_Max_Y > 0);
        y = static_cast<short>(Map_Max_Y - y);
        ShiAssert(y >= 0);

        radar_short = ent->GetElectronicDetectionRange(LowAir);
        radar_long = ent->GetElectronicDetectionRange(Air);

        if (radar_short or radar_long)
        {
#ifdef USE_SH_POOLS
            threat = (THREAT_LIST *)MemAllocPtr(UI_Pools[UI_GENERAL_POOL], sizeof(THREAT_LIST), FALSE);
#else
            threat = new THREAT_LIST;
#endif
            memset(threat, 0, sizeof(THREAT_LIST));

            if (Team_[ent->GetTeam()].Threats)
            {
                if (Team_[ent->GetTeam()].Threats->Type[_THREAT_RADAR_HIGH_])
                {
                    Team_[ent->GetTeam()].Threats->Type[_THREAT_RADAR_HIGH_]->AddCircle(ent->GetCampID(), C_Threat::THR_CIRCLE, x, y, radar_long);
                    threat->RadarHigh = Team_[ent->GetTeam()].Threats->Type[_THREAT_RADAR_HIGH_]->GetThreat(ent->GetCampID());
                }

                if (Team_[ent->GetTeam()].Threats->Type[_THREAT_RADAR_LOW_] and radar_short)
                {
                    Team_[ent->GetTeam()].Threats->Type[_THREAT_RADAR_LOW_]->AddCircle(ent->GetCampID(), C_Threat::THR_SLICE, x, y, radar_short);

                    for (i = 0; i < 8; i++)
                        // 2001-03-14 MODIFIED BY S.G. SO IF THERE IS NO RADAR RANGE DATA, THE radar_short VALUE IS USED INSTEAD
                        // Team_[ent->GetTeam()].Threats->Type[_THREAT_RADAR_LOW_]->SetRadius(ent->GetCampID(),i,static_cast<long>(min(ent->GetArcRange(i)*FT_TO_KM,radar_short)));
                        Team_[ent->GetTeam()].Threats->Type[_THREAT_RADAR_LOW_]->SetRadius(ent->GetCampID(), i, static_cast<long>(((ObjectiveClass *)ent)->HasRadarRanges() ? (min(ent->GetArcRange(i)*FT_TO_KM, radar_short)) : radar_short));

                    threat->RadarLow = Team_[ent->GetTeam()].Threats->Type[_THREAT_RADAR_LOW_]->GetThreat(ent->GetCampID());
                }
            }

            delete threat; // JPO - no idea what this threat variable is for
        }
    }

    // 2001-05-08 MODIFIED BY S.G. UNITS CAN STILL FIRE AT YOU, EVEN IF NOT EMITING THEMSELF SO RESERVE THE 'IsEmitting' FOR THREAT_RADAR_* CODE
    // if(ent->IsUnit() and ent->IsEmitting())
    if (ent->IsUnit())
        // THIS IS WHAT I DO IN 1.08i2 BUT NOT REQUIRED IN 1.07 (SEE AT END OF FUNCTION FOR DETAIL)
        // if(ent->IsUnit() and not ((Unit)ent)->Inactive() and (FindUnitType(ent) bitand (_UNIT_AIR_DEFENSE bitor _UNIT_BATTALION)))
        // UI_Refresher *gpsItem=NULL;
        // if(ent->IsUnit() and (gpsItem=(UI_Refresher*)gGps->Find(ent->GetCampID())) and gpsItem->MapItem_ and not (gpsItem->MapItem_->Flags bitand C_BIT_INVISIBLE))
    {
        ent->GetLocation(&x, &y);
        ShiAssert(Map_Max_Y > 0);
        y = static_cast<short>(Map_Max_Y - y);
        ShiAssert(y >= 0);
        radar_short = ent->GetElectronicDetectionRange(LowAir);
        radar_long = ent->GetElectronicDetectionRange(Air);
        sam_short = ent->GetAproxWeaponRange(LowAir);
        sam_long = ent->GetAproxWeaponRange(Air);

        if (radar_short or radar_long or sam_long or sam_short)
        {
            // 2001-06-22 ADDED BY S.G. IF EMITTING, DISPLAY THE RADAR THREATS
            if (((BattalionClass *)ent)->class_data->RadarVehicle < 16)
            {
                if ( not radar_short)
                    sam_short /= 128;

                if ( not radar_long)
                    sam_long /= 128;
            }

            // END OF ADDED SECTION

#ifdef USE_SH_POOLS
            threat = (THREAT_LIST *)MemAllocPtr(UI_Pools[UI_GENERAL_POOL], sizeof(THREAT_LIST), FALSE);
#else
            threat = new THREAT_LIST;
#endif
            memset(threat, 0, sizeof(THREAT_LIST));

            if (Team_[ent->GetTeam()].Threats)
            {
                // 2001-05-08 ADDED BY S.G. IF EMITTING, DISPLAY THE RADAR THREATS
                if (ent->IsEmitting())
                {
                    // END OF ADDED SECTION (EXCEPT FOR BLOCK INDENT)
                    if (Team_[ent->GetTeam()].Threats->Type[_THREAT_RADAR_HIGH_] and radar_long)
                    {
                        Team_[ent->GetTeam()].Threats->Type[_THREAT_RADAR_HIGH_]->AddCircle(ent->GetCampID(), C_Threat::THR_CIRCLE, x, y, radar_long);
                        threat->RadarHigh = Team_[ent->GetTeam()].Threats->Type[_THREAT_RADAR_HIGH_]->GetThreat(ent->GetCampID());
                    }

                    if (Team_[ent->GetTeam()].Threats->Type[_THREAT_RADAR_LOW_] and radar_short)
                    {
                        Team_[ent->GetTeam()].Threats->Type[_THREAT_RADAR_LOW_]->AddCircle(ent->GetCampID(), C_Threat::THR_SLICE, x, y, radar_short);

                        for (i = 0; i < 8; i++)
                            Team_[ent->GetTeam()].Threats->Type[_THREAT_RADAR_LOW_]->SetRadius(ent->GetCampID(), i, static_cast<short>(min(ent->GetArcRange(i)*FT_TO_KM, radar_short)));

                        threat->RadarLow = Team_[ent->GetTeam()].Threats->Type[_THREAT_RADAR_LOW_]->GetThreat(ent->GetCampID());
                    }

                    if (Team_[ent->GetTeam()].Threats->Type[_THREAT_SAM_HIGH_] and sam_long)
                    {
                        Team_[ent->GetTeam()].Threats->Type[_THREAT_SAM_HIGH_]->AddCircle(ent->GetCampID(), C_Threat::THR_CIRCLE, x, y, FTOL(sam_long / .539f));
                        threat->SamHigh = Team_[ent->GetTeam()].Threats->Type[_THREAT_SAM_HIGH_]->GetThreat(ent->GetCampID());
                    }

                    if (Team_[ent->GetTeam()].Threats->Type[_THREAT_SAM_LOW_] and sam_short)
                    {
                        Team_[ent->GetTeam()].Threats->Type[_THREAT_SAM_LOW_]->AddCircle(ent->GetCampID(), C_Threat::THR_SLICE, x, y, sam_short);

                        for (i = 0; i < 8; i++)
                            Team_[ent->GetTeam()].Threats->Type[_THREAT_SAM_LOW_]->SetRadius(ent->GetCampID(), i, static_cast<short>(min(ent->GetArcRange(i)*FT_TO_KM, sam_short)));

                        threat->SamLow = Team_[ent->GetTeam()].Threats->Type[_THREAT_SAM_LOW_]->GetThreat(ent->GetCampID());
                    }
                }
            }

            delete threat; // JPO - no idea what this threat variable is for S.G. I DO BUT I HAVE NO TIME TO FIX IT NOW. I'LL MAKE IT RP5 COMPATIBLE FIRST
        }
    }

    // 2001-05-09 MODIFIED BY S.G. WHY NOT RETURNING THAT STRUCTURE WE FILLED UP? CAN WE SAY 'MEMORY LEAK' HERE? PLUS WITHOUT THIS, THREAT CIRCLES ARE APPEARING FOR ANY UNITS WITH A 'Range' AGAINST 'Air' MOVEMENT TYPE...
    return(NULL);
    // THIS IS DIFFERENT THAN WHAT I DO IN 1.08i2 AND IS THE PREFERED WAY. I CAN'T DO IT IN 1.08i2 BEFORE TOO MUCH CODE WAS OPTOMIZED OUT BY THE COMPILER :-(
    // return (threat);
}


MAPICONLIST *C_Map::AddObjective(Objective Obj)
{
    long ObjType, TypeID;
    short numarcs;
    float radar_short, radar_long;
    ObjClassDataType *ObjPtr;
    _TCHAR Buffer[40];
    DETECTOR *detect;

    ObjType = GetObjectiveType(Obj);
    TypeID = FindTypeIndex(ObjType, OBJ_TypeList, _MAP_NUM_OBJ_TYPES_);

    if (TypeID not_eq -1)
    {
        if (ObjType)
        {
            ObjPtr = Obj->GetObjectiveClassData();

            if (ObjPtr)
            {
                Obj->GetName(Buffer, 39, TRUE);
                detect = NULL;
                radar_short = static_cast<float>(Obj->GetElectronicDetectionRange(LowAir));
                radar_long = static_cast<float>(Obj->GetElectronicDetectionRange(Air));

                if (radar_short or radar_long)
                {
#ifdef USE_SH_POOLS
                    detect = (DETECTOR *)MemAllocPtr(UI_Pools[UI_GENERAL_POOL], sizeof(DETECTOR), FALSE);
#else
                    detect = new DETECTOR;
#endif
                    detect->HighSam = 0.0f;
                    detect->HighRadar = radar_long * KM_TO_FT;
                    detect->LowSam = 0.0f;

                    if (radar_short)
                    {
                        numarcs = static_cast<short>(Obj->GetNumberOfArcs());
                        numarcs = 1; // TEMP kludge
#ifdef USE_SH_POOLS
                        detect->LowRadar = (ARC_LIST *)MemAllocPtr(UI_Pools[UI_GENERAL_POOL], sizeof(ARC_LIST), FALSE);
#else
                        detect->LowRadar = new ARC_LIST;
#endif
                        detect->LowRadar->numarcs = numarcs;
#ifdef USE_SH_POOLS
                        detect->LowRadar->arcs = (ARC_REC *)MemAllocPtr(UI_Pools[UI_GENERAL_POOL], sizeof(ARC_REC) * numarcs, FALSE);
#else
                        detect->LowRadar->arcs = new ARC_REC[numarcs];
#endif
                        // after getting rid of kludges...
                        // figure out how to do arcs right
                        // kludge
                        detect->LowRadar->arcs[0].arc = 0;
                        detect->LowRadar->arcs[0].range = radar_short;
                    }
                    else
                        detect->LowRadar = NULL;
                }

                return (Team_[Obj->GetTeam()].Objectives->Type[TypeID]->AddIconToList(
                            Obj->GetCampID(),
                            static_cast<short>(ObjType),
                            ObjPtr->IconIndex,
                            Obj->YPos(),
                            maxy - Obj->XPos(),
                            FALSE,
                            Buffer, 0, 0, 0, 0, 0, detect)
                       );
            }
        }
    }

    return(NULL);
}

MAPICONLIST *C_Map::AddDivision(Division div)
{
    UnitClassDataType *UnitPtr;
    Unit     u;
    MAPICONLIST *cur;
    GridIndex x, y;
    long UnitType, Type;
    long totalstr, curstr, perc;
    _TCHAR Buffer[40];

    u = div->GetFirstUnitElement();

    if (u)
    {
        UnitType = FindDivisionType(div->GetDivisionType()) bitand 0xffffff;
        Type = FindTypeIndex(UnitType bitand 0x0fff, GND_TypeList, _MAP_NUM_GND_TYPES_);

        // Figure out Status
        curstr = u->GetTotalVehicles();
        totalstr = u->GetFullstrengthVehicles();

        if (totalstr < 1) totalstr = 1;

        perc = (curstr * 100) / totalstr;

        if (perc > 100) perc = 100;

        cur = Team_[u->GetTeam()].Units->Type[Type]->Levels[0]->FindID(UR_DIVISION bitor div->nid);

        if (cur == NULL)
        {
            div->GetName(Buffer, 39, FALSE);
            UnitPtr = u->GetUnitClassData();
            div->GetLocation(&x, &y);

            return(Team_[u->GetTeam()].Units->Type[Type]->Levels[0]->AddIconToList(
                       UR_DIVISION bitor div->nid,
                       static_cast<short>(Type bitor _UNIT_DIVISION),
                       UnitPtr->IconIndex,
                       x * FEET_PER_KM,
                       maxy - y * FEET_PER_KM,
                       FALSE,
                       Buffer,
                       div->nid,
                       0, 0, (long)perc, 0)
                  );
        }
    }

    return(NULL);
}

MAPICONLIST *C_Map::AddUnit(Unit u)
{
    long UnitType, TypeID, LevelID = -1, brigid, batid, numarcs; //
    Unit upar;
    UnitClassDataType *UnitPtr;
    float totalstr, curstr, perc;
    DETECTOR *detect;
    float sam_short;
    float sam_long;
    float radar_short;
    float radar_long;
    _TCHAR Buffer[60];

    UnitType = FindUnitType(u);
    TypeID = -1;

    // Figure out Status
    curstr = (float)u->GetTotalVehicles();
    totalstr = (float)u->GetFullstrengthVehicles();

    if (totalstr < 1) totalstr = 1;

    perc = (curstr / totalstr) * 100.0f;

    if (perc > 100.0f) perc = 100.0f;

    if (UnitType bitand _UNIT_GROUND_MASK)
    {
        TypeID = FindTypeIndex(UnitType bitand 0x0fff, GND_TypeList, _MAP_NUM_GND_TYPES_);

        if (TypeID not_eq -1)
            LevelID = FindTypeIndex(UnitType bitand _UNIT_GROUND_MASK, GND_LevelList, _MAP_NUM_GND_LEVELS_);
        else
            LevelID = -1;
    }
    else if (UnitType bitand _UNIT_NAVAL_MASK)
    {
        TypeID = FindTypeIndex(UnitType bitand 0x0fff, NAV_TypeList, _MAP_NUM_NAV_TYPES_);
        LevelID = 1;
    }

    if (TypeID not_eq -1 and LevelID not_eq -1)
    {
        UnitPtr = u->GetUnitClassData();

        if (UnitPtr)
        {
            if (UnitType bitand _UNIT_GROUND_MASK)
            {
                if (LevelID == 1)
                {
                    brigid = u->GetUnitNameID();
                    batid = 0;
                }
                else if (LevelID == 2)
                {
                    upar = u->GetUnitParent();

                    if (upar)
                        brigid = upar->GetUnitNameID();
                    else
                        brigid = 0;

                    batid = u->GetUnitNameID();
                }
                else
                {
                    brigid = 0;
                    batid = 0;
                }

                detect = NULL;

                if (u->IsBattalion())
                {
                    radar_short = static_cast<float>(u->GetElectronicDetectionRange(LowAir));
                    radar_long = static_cast<float>(u->GetElectronicDetectionRange(Air));
                    sam_short = static_cast<float>(u->GetAproxWeaponRange(LowAir));
                    sam_long = static_cast<float>(u->GetAproxWeaponRange(Air));

                    if (radar_short or radar_long or sam_short or sam_long)
                    {
#ifdef USE_SH_POOLS
                        detect = (DETECTOR *)MemAllocPtr(UI_Pools[UI_GENERAL_POOL], sizeof(DETECTOR), FALSE);
#else
                        detect = new DETECTOR;
#endif
                        detect->HighSam = sam_long * KM_TO_FT;
                        detect->HighRadar = radar_long * KM_TO_FT;
                        detect->LowSam = sam_short * KM_TO_FT;

                        if (radar_short)
                        {
                            numarcs = u->GetNumberOfArcs();
                            numarcs = 1; // TEMP kludge

#ifdef USE_SH_POOLS
                            detect->LowRadar = (ARC_LIST *)MemAllocPtr(UI_Pools[UI_GENERAL_POOL], sizeof(ARC_LIST), FALSE);
#else
                            detect->LowRadar = new ARC_LIST;
#endif
                            detect->LowRadar->numarcs = static_cast<short>(numarcs);
#ifdef USE_SH_POOLS
                            detect->LowRadar->arcs = (ARC_REC *)MemAllocPtr(UI_Pools[UI_GENERAL_POOL], sizeof(ARC_REC) * numarcs, FALSE);
#else
                            detect->LowRadar->arcs = new ARC_REC[numarcs];
#endif

                            // after getting rid of kludges...
                            // figure out how to do arcs right
                            // kludge
                            detect->LowRadar->arcs[0].arc = 0;
                            detect->LowRadar->arcs[0].range = radar_short;
                        }
                        else
                            detect->LowRadar = NULL;
                    }
                }

                u->GetName(Buffer, 49, FALSE);
                return(Team_[u->GetTeam()].Units->Type[TypeID]->Levels[LevelID]->AddIconToList(
                           u->GetCampID(),
                           static_cast<short>(UnitType),
                           UnitPtr->IconIndex,
                           u->YPos(),
                           maxy - u->XPos(),
                           static_cast<short>(gMoveBattalion),
                           Buffer,
                           u->GetUnitDivision(),
                           brigid, batid,
                           (long)perc, 0, detect)
                      );
            }
            else if (UnitType bitand _UNIT_NAVAL_MASK)
            {
                u->GetName(Buffer, 49, FALSE);
                return(Team_[u->GetTeam()].NavalUnits->Type[TypeID]->AddIconToList(
                           u->GetCampID(),
                           static_cast<short>(UnitType),
                           UnitPtr->IconIndex,
                           u->YPos(),
                           maxy - u->XPos(),
                           static_cast<short>(gMoveBattalion), Buffer, (long)perc, 0)
                      );
            }
        }
    }

    return(NULL);
}

MAPICONLIST *C_Map::AddFlight(Flight flight)
{
    long idx;
    long TypeID;
    UnitClassDataType *UnitPtr;
    _TCHAR Buffer[40];

    UnitPtr = flight->GetUnitClassData();

    if (UnitPtr)
    {
        idx = GetAirIcon(flight->GetSType());
        TypeID = FindTypeIndex(AirIcons[idx].UIType, AIR_TypeList, _MAP_NUM_AIR_TYPES_);

        if (TypeID not_eq -1)
        {
            if (UnitPtr->IconIndex)
            {
                if (gGps->GetTeamNo() == flight->GetTeam())
                {
                    GetCallsign(flight, Buffer);
                }
                else
                {
                    int vid = flight->GetVehicleID(0);
                    VehicleClassDataType *vc = GetVehicleClassData(vid);
                    strcpy(Buffer, vc ? vc->Name : "<unk>");
                }

                return (Team_[flight->GetTeam()].AirUnits->Type[TypeID]->AddIconToList(
                            flight->GetCampID(),
                            static_cast<short>(AirIcons[idx].UIType),
                            UnitPtr->IconIndex,
                            flight->YPos(),
                            maxy - flight->XPos(),
                            static_cast<short>(gMoveBattalion),
                            Buffer,
                            0,
                            ((Flight)flight)->GetLastDirection() bitand 0x7
                        ));
            }
        }
    }

    return(NULL);
}

MAPICONLIST *C_Map::AddSquadron(Squadron squadron)
{
    long UnitType, TypeID;
    UnitClassDataType *UnitPtr;
    _TCHAR Buffer[40];

    UnitType = GetObjectiveType(squadron);
    TypeID = FindTypeIndex(UnitType, OBJ_TypeList, _MAP_NUM_OBJ_TYPES_);

    if (TypeID not_eq -1)
    {
        if (UnitType)
        {
            UnitPtr = squadron->GetUnitClassData();

            if (UnitPtr)
            {
                squadron->GetName(Buffer, 39, FALSE);
                return(Team_[squadron->GetTeam()].Objectives->Type[TypeID]->AddIconToList(
                           squadron->GetCampID(),
                           static_cast<short>(UnitType),
                           10117,
                           squadron->YPos(),
                           maxy - squadron->XPos(),
                           FALSE, Buffer, 0, 0, 0, 0, 0, NULL)
                      );
            }
        }
    }

    return(NULL);
}

MAPICONLIST *C_Map::AddPackage(Package package)
{
    long UnitType, TypeID;
    _TCHAR Buffer[32];

    UnitType = GetObjectiveType(package);
    TypeID = FindTypeIndex(UnitType, OBJ_TypeList, _MAP_NUM_OBJ_TYPES_);

    if (TypeID not_eq -1)
    {
        if (UnitType)
        {
            _stprintf(Buffer, "%s %1ld", gStringMgr->GetString(TXT_PACKAGE), package->GetCampID());
            return(Team_[package->GetTeam()].Objectives->Type[TypeID]->AddIconToList(
                       package->GetCampID(),
                       static_cast<short>(UnitType),
                       10118,
                       package->YPos(),
                       maxy - package->XPos(),
                       FALSE, Buffer, 0, 0, 0, 0, 0, NULL)
                  );
        }
    }

    return(NULL);
}

MAPICONLIST *C_Map::AddVC(victory_condition *vc)
{
    long TypeID;
    _TCHAR Buffer[32];
    CampEntity ent;

    ent = (CampEntity)vuDatabase->Find(vc->get_vu_id());

    if (ent)
    {
        if ( not ent->IsUnit() and not ent->IsObjective())
            return(NULL);
    }
    else
        return(NULL);

    TypeID = FindTypeIndex(_VC_CONDITION_, OBJ_TypeList, _MAP_NUM_OBJ_TYPES_);

    if (TypeID not_eq -1)
    {
        _stprintf(Buffer, "%s %1ld", gStringMgr->GetString(TXT_VC), vc->get_number());
        return(Team_[vc->get_team()].Objectives->Type[TypeID]->AddIconToList(
                   vc->get_number(),
                   _VC_CONDITION_,
                   10119,
                   ent->YPos(),
                   maxy - ent->XPos(),
                   FALSE,
                   Buffer, 0, 0, 0, 0, 0, NULL)
              );
    }

    return(NULL);
}

void C_Map::UpdateVC(victory_condition *vc)
{
    long TypeID;
    CampEntity ent;
    MAPICONLIST *vcicon;

    ent = (CampEntity)vuDatabase->Find(vc->get_vu_id());

    if (ent)
    {
        if ( not ent->IsUnit() and not ent->IsObjective())
            ent = NULL;
    }

    TypeID = FindTypeIndex(_VC_CONDITION_, OBJ_TypeList, _MAP_NUM_OBJ_TYPES_);

    if (TypeID not_eq -1)
    {
        vcicon = Team_[vc->get_team()].Objectives->Type[TypeID]->FindID(vc->get_number());

        if ( not ent)
        {
            Team_[vc->get_team()].Objectives->Type[TypeID]->RemoveIcon(vc->get_number());
        }
        else if (vcicon)
        {
            vcicon->worldx = ent->YPos();
            vcicon->worldy = maxy - ent->XPos();
            vcicon->x = static_cast<short>(vcicon->worldx * scale_);
            vcicon->y = static_cast<short>(vcicon->worldy * scale_);
        }
    }
}

void C_Map::RemoveVC(long team, long ID)
{
    long TypeID;
    F4CSECTIONHANDLE *Leave;

    TypeID = FindTypeIndex(_VC_CONDITION_, OBJ_TypeList, _MAP_NUM_OBJ_TYPES_);

    if (TypeID not_eq -1)
    {
        Leave = UI_Enter(DrawWindow_);
        Team_[team].Objectives->Type[TypeID]->RemoveIcon(ID);
        UI_Leave(Leave);
    }
}

void C_Map::BuildCurrentWPList(Unit unit)
{
    WayPoint wp = NULL, prevwp = NULL, firstwp = NULL;
    float x, y, z, tempx, tempy, tempz;
    long normID, selID, othrID;
    long i, numwp, xval;
    long starttime, endtime;
    long campID;
    long UseFlag, ZDrag;
    double distance;
    float lx, ly, lz, dx, dy, dz;
    WAYPOINTLIST *wpl = NULL;
    CampEntity target;
    short state;
    _TCHAR buf[40];
    VU_ID *tmpID = NULL;
    UI_Refresher *gpsItem = NULL;
    short airwps, lastwp;

    if (unit == NULL) return;

    if (unit->IsFlight() and not unit->Final()) return;

    if ( not CurWP_ or not CurWPZ_)
        return;

    airwps = static_cast<short>(unit->IsFlight());

    if ( not airwps)
    {
        firstwp = unit->GetCurrentUnitWP();

        if (firstwp and firstwp->GetPrevWP())
            firstwp = firstwp->GetPrevWP();
    }

    if ( not firstwp or airwps)
        firstwp = unit->GetFirstUnitWP();

    if ( not firstwp) return;

    wp = firstwp;

    starttime = wp->GetWPDepartureTime();
    endtime = wp->GetWPDepartureTime(); //so endtime has a value if it doesn't get otherwise initialized
    campID = unit->GetCampID() << 8;

    // set to 0 after Landing WP so we don't connect lines to following waypoints which are not on the agenda (Alt Land cit,Tanker etc)
    UseFlag = C_BIT_USELINE;

    prevwp = NULL;
    numwp = 0;

    while (wp)
    {
        numwp++;
        wp = wp->GetNextWP();
    }

    if (airwps)
    {
        CurWP_->SetMenu(STEERPOINT_POP);
        CurWPZ_->SetMenu(STEERPOINT_POP);
    }
    else
    {
        CurWP_->SetMenu(0);
        CurWPZ_->SetMenu(0);
    }

    CurIcons_->Cleanup();
    CurIcons_->Setup(CurIcons_->GetID(), 0);

    if (TheCampaign.Flags bitand CAMP_TACTICAL_EDIT)
    {
        CurWP_->SetFlagBitOn(C_BIT_DRAGABLE);
        CurWPZ_->SetFlagBitOn(C_BIT_DRAGABLE);
    }
    else
    {
        if (firstwp == unit->GetCurrentUnitWP() or not airwps)
        {
            CurWP_->SetFlagBitOn(C_BIT_DRAGABLE);
            CurWPZ_->SetFlagBitOn(C_BIT_DRAGABLE);
        }
        else
        {
            CurWP_->SetFlagBitOff(C_BIT_DRAGABLE);
            CurWPZ_->SetFlagBitOff(C_BIT_DRAGABLE);
        }
    }

    i = 1;
    wp = firstwp;

    while (wp)
    {
        wp->GetLocation(&x, &y, &z);

        if (x < CurWPArea_.left or CurWPArea_.left < 0)
            CurWPArea_.left = static_cast<long>(x);

        if (x > CurWPArea_.right or CurWPArea_.right < 0)
            CurWPArea_.right = static_cast<long>(x);

        if (y < CurWPArea_.top or CurWPArea_.top < 0)
            CurWPArea_.top = static_cast<long>(y);

        if (y > CurWPArea_.bottom or CurWPArea_.bottom < 0)
            CurWPArea_.bottom = static_cast<long>(y);

        if (wp->GetWPFlags() bitand WPF_TARGET)
        {
            // Set 2d Waypoint
            wpl = CurWP_->AddWaypointToList(0x20000000 + campID + i, 0, ASSIGNED_TGT_CUR, ASSIGNED_TGT_CUR, ASSIGNED_TGT_CUR, y, maxy - x, FALSE);

            if (wpl)
            {
                CurWP_->SetWPGroup(campID + i, campID + i);
                CurWP_->SetState(campID + i, 0);
                wpl->Flags and_eq compl C_BIT_ENABLED;
            }
        }
        else if ((wp->GetWPAction() == WP_TAKEOFF) or (wp->GetWPAction() == WP_LAND))
        {
            // Set 2d Waypoint
            wpl = CurWP_->AddWaypointToList(0x20000000 + campID + i, 0, HOME_BASE_CUR, HOME_BASE_CUR, HOME_BASE_CUR, y, maxy - x, FALSE);

            if (wpl)
            {
                CurWP_->SetWPGroup(campID + i, campID + i);
                CurWP_->SetState(campID + i, 0);
                wpl->Flags and_eq compl C_BIT_ENABLED;
            }
        }

        target = wp->GetWPTarget();

        if (target)
        {
            // use GPS to make visible
            gpsItem = (UI_Refresher*)gGps->Find(target->GetCampID());

            if (gpsItem and gpsItem->MapItem_)
                CurIcons_->Add(gpsItem->MapItem_);
        }

        i++;
        wp = wp->GetNextWP();
    }

    distance = 0.0f;
    wp = firstwp;
    wp->GetLocation(&lx, &ly, &lz);
    i = 1;
    lastwp = 0;

    while (wp)
    {
        if ( not wp->GetNextWP())
            lastwp = 1;

        if (wp->GetWPFlags() bitand WPF_TARGET)
        {
            normID = TGT_CUR;
            selID = TGT_CUR_SEL;
            othrID = TGT_CUR_ERROR;
        }
        else if (wp->GetWPFlags() bitand WPF_IP)
        {
            normID = IP_CUR;
            selID = IP_CUR_SEL;
            othrID = IP_CUR_ERROR;
        }
        else
        {
            normID = STPT_CUR;
            selID = STPT_CUR_SEL;
            othrID = STPT_CUR_ERROR;
        }

        if (unit->IsFlight() and not IsValidWP(wp, (Flight)unit))
            state = 2;
        else if (wp == unit->GetCurrentUnitWP())
            state = 1;
        else
            state = 0;

        wp->GetLocation(&x, &y, &z); // Note: for Sim -> UI (UI's) X = (Sim's) Y, (UI's Y) = (Sim's) [max y] - X (UI's) Z = (Sim's) -Z

        // Add Nub to insert a waypoint
        if (UseFlag and prevwp and airwps)
        {
            prevwp->GetLocation(&tempx, &tempy, &tempz); // Note: for Sim -> UI (UI's) X = (Sim's) Y, (UI's Y) = (Sim's) [max y] - X (UI's) Z = (Sim's) -Z
            dx = x - tempx;
            dy = y - tempy;
            _stprintf(buf, "%1.1f", sqrt(dx * dx + dy * dy) * FT_TO_NM);
            Uni_Float(buf);
            dx *= .5;
            dy *= .5;
            wpl = CurWP_->AddWaypointToList(0x40000000 + campID + i, 0, ADDLINE_CUR, ADDLINE_CUR_SEL, ADDLINE_CUR, tempy + dy, maxy - (tempx + dx), TRUE);

            if (wpl)
            {
                CurWP_->SetWPGroup(0x40000000 + campID + i, campID);
                CurWP_->SetUserNumber(C_STATE_0, static_cast<long>(maxy));
                CurWP_->SetLabel(0x40000000 + campID + i, gStringMgr->GetText(gStringMgr->AddText(buf)));
                CurWP_->SetTextOffset(0x40000000 + campID + i, 0, -15);
                CurWP_->SetState(0x40000000 + campID + i, state);
                CurWP_->SetLabelColor(0x40000000 + campID + i, 0x00ffffff, 0x0000ffff, 0x000000ff);
                CurWP_->SetLineColor(0x40000000 + campID + i, 0x00ffffff, 0x0000ffff, 0x000000ff);

                if ( not (TheCampaign.Flags bitand CAMP_TACTICAL_EDIT))
                {
                    if (firstwp not_eq unit->GetCurrentUnitWP())
                    {
                        wpl->Icon->SetText(0, TXT_SPACE);
                        wpl->Icon->SetText(1, TXT_SPACE);
                        wpl->Icon->SetText(2, TXT_SPACE);
                        wpl->Icon->SetFlagBitOff(C_BIT_ENABLED);
                    }
                }

                wpl->Flags or_eq UseFlag;
            }
        }

        // Set 2d Waypoint
        if (UseFlag)
            _stprintf(buf, "%1d", i);
        else
        {
            if (wp->GetWPAction() == WP_LAND)
            {
                _sntprintf(buf, 39, "%s", gStringMgr->GetString(TXT_ALTERNATE_FIELD));
                buf[39] = 0;
            }
            else if (wp->GetWPAction() == WP_REFUEL)
            {
                _sntprintf(buf, 39, "%s", gStringMgr->GetString(TXT_TANKER));
                buf[39] = 0;
            }
            else
                buf[0] = 0;
        }

        if (buf[0])
        {
            if (airwps)
            {
                if (i == 1)
                    wpl = CurWP_->AddWaypointToList(campID + i, 0, normID, selID, othrID, y, maxy - x, FALSE);
                else
                    wpl = CurWP_->AddWaypointToList(campID + i, 0, normID, selID, othrID, y, maxy - x, TRUE);
            }
            else
            {
                if (lastwp)
                    wpl = CurWP_->AddWaypointToList(campID + i, 0, normID, selID, othrID, y, maxy - x, TRUE);
                else
                    wpl = CurWP_->AddWaypointToList(campID + i, 0, 0, 0, 0, y, maxy - x, FALSE);
            }

            if (wpl)
            {
                CurWP_->SetWPGroup(campID + i, campID);
                CurWP_->SetUserNumber(C_STATE_0, static_cast<long>(maxy));

                if (airwps)
                {
                    CurWP_->SetLabel(campID + i, gStringMgr->GetText(gStringMgr->AddText(buf)));
                    CurWP_->SetTextOffset(campID + i, 0, -15);
                }

                CurWP_->SetState(campID + i, state);
                CurWP_->SetLabelColor(campID + i, 0x00ffffff, 0x0000ffff, 0x000000ff);
                CurWP_->SetLineColor(campID + i, 0x00ffffff, 0x0000ffff, 0x000000ff);
                tmpID = new VU_ID;
                *tmpID = unit->Id();
                wpl->Flags or_eq UseFlag;
                wpl->Icon->SetUserCleanupPtr(C_STATE_0, tmpID);
                wpl->Icon->SetUserNumber(C_STATE_1, i);
            }

            // Set Z Waypoint
            xval = (i - 1) * (650 / numwp) + 60;

            if (airwps)
            {
                if (wp->GetWPAction() == WP_LAND or wp->GetWPAction() == WP_TAKEOFF)
                    ZDrag = FALSE;
                else
                    ZDrag = TRUE;

                wpl = CurWPZ_->AddWaypointToList(static_cast<short>(campID + i), 0, normID, selID, othrID, static_cast<float>(xval), z, static_cast<short>(ZDrag));

                if (wpl)
                {
                    CurWPZ_->SetWPGroup(campID + i, campID);
                    CurWPZ_->SetLabel(campID + i, gStringMgr->GetText(gStringMgr->AddText(buf)));
                    CurWPZ_->SetState(campID + i, state);
                    CurWPZ_->SetLabelColor(campID + i, 0x00ffffff, 0x0000ffff, 0x000000ff);
                    CurWPZ_->SetLineColor(campID + i, 0x00ffffff, 0x0000ffff, 0x000000ff);
                    CurWPZ_->SetTextOffset(campID + i, 0, -15);

                    tmpID = new VU_ID;
                    *tmpID = unit->Id();
                    wpl->Flags or_eq UseFlag;
                    wpl->Icon->SetUserCleanupPtr(C_STATE_0, tmpID);
                    wpl->Icon->SetUserNumber(C_STATE_1, i);

                    if ( not (TheCampaign.Flags bitand CAMP_TACTICAL_EDIT))
                        if (firstwp not_eq unit->GetCurrentUnitWP())
                            wpl->Dragable = 0;
                }
            }

            dx = lx - x;
            dy = ly - y;
            dz = lz - z;

            lx = x;
            ly = y;
            lz = z;

            i++;
            distance += sqrt(dx * dx + dy * dy + dz * dz);

            if (wp->GetWPAction() == WP_LAND and UseFlag)
            {
                endtime = wp->GetWPArrivalTime();
                UseFlag = 0;
            }
        }

        prevwp = wp;
        wp = wp->GetNextWP();
    }

    if (airwps)
    {
        wp = ((Flight)unit)->GetOverrideWP();

        if (wp)
        {
            wp->GetLocation(&x, &y, &z); // Note: for Sim -> UI (UI's) X = (Sim's) Y, (UI's Y) = (Sim's) [max y] - X (UI's) Z = (Sim's) -Z

            _sntprintf(buf, 39, "%s", gStringMgr->GetString(TXT_DIVERT));
            buf[39] = 0;

            wpl = CurWP_->AddWaypointToList(campID + i, 0, TGT_CUR, TGT_CUR_SEL, TGT_CUR_ERROR, y, maxy - x, FALSE);

            if (wpl)
            {
                CurWP_->SetWPGroup(campID + i, campID);
                CurWP_->SetUserNumber(C_STATE_0, static_cast<long>(maxy));
                CurWP_->SetLabel(campID + i, gStringMgr->GetText(gStringMgr->AddText(buf)));
                CurWP_->SetTextOffset(campID + i, 0, -15);
                CurWP_->SetState(campID + i, 0);
                CurWP_->SetLabelColor(campID + i, 0x00ffffff, 0x0000ffff, 0x000000ff);
                CurWP_->SetLineColor(campID + i, 0x00ffffff, 0x0000ffff, 0x000000ff);
                tmpID = new VU_ID;
                *tmpID = unit->Id();
                wpl->Icon->SetUserCleanupPtr(C_STATE_0, tmpID);
                wpl->Icon->SetUserNumber(C_STATE_1, -1);
            }

            // Set Z Waypoint
            xval = (i - 1) * (650 / numwp) + 60;

            wpl = CurWPZ_->AddWaypointToList(campID + i, 0, TGT_CUR, TGT_CUR_SEL, TGT_CUR_ERROR, static_cast<float>(xval), z, FALSE);

            if (wpl)
            {
                CurWPZ_->SetWPGroup(campID + i, campID);
                CurWPZ_->SetUserNumber(C_STATE_0, static_cast<long>(maxy));
                CurWPZ_->SetLabel(campID + i, gStringMgr->GetText(gStringMgr->AddText(buf)));
                CurWPZ_->SetState(campID + i, 0);
                CurWPZ_->SetLabelColor(campID + i, 0x00ffffff, 0x0000ffff, 0x000000ff);
                CurWPZ_->SetLineColor(campID + i, 0x00ffffff, 0x0000ffff, 0x000000ff);
                CurWPZ_->SetTextOffset(campID + i, 0, -15);

                tmpID = new VU_ID;
                *tmpID = unit->Id();
                wpl->Icon->SetUserCleanupPtr(C_STATE_0, tmpID);
                wpl->Icon->SetUserNumber(C_STATE_1, -1);

                if ( not (TheCampaign.Flags bitand CAMP_TACTICAL_EDIT))
                    if (firstwp not_eq unit->GetCurrentUnitWP())
                        wpl->Dragable = 0;
            }
        }
    }

    CurWPZ_->SetUserNumber(C_STATE_1, endtime - starttime);
    CurWPZ_->SetUserNumber(C_STATE_2, (long)distance);
    CurWPZ_->SetUserNumber(C_STATE_3, 0 /* MPG */);
}

void C_Map::AddToCurIcons(MAPICONLIST *MapItem)
{
    if (MapItem)
        CurIcons_->Add(MapItem);
}

void C_Map::CenterOnIcon(MAPICONLIST *MapItem)
{
    if (MapItem)
    {
        //cx=(CurWPArea_.top/1640 + CurWPArea_.bottom/1640)/2;
        //cy=((maxy - CurWPArea_.left)/1640 + (maxy - CurWPArea_.right)/1640)/2;

        SetMapCenter(static_cast<long>(MapItem->worldx / FEET_PER_PIXEL), static_cast<long>(MapItem->worldy / FEET_PER_PIXEL));

        if (DrawWindow_)
            DrawWindow_->RefreshWindow();
    }
}

void C_Map::BuildWPList(C_Waypoint *wplist, C_Waypoint *, Unit unit)
{
    WayPoint wp = NULL;
    float x, y, z;
    long normID, selID, othrID;
    long i;
    long campID;
    long UseFlag;
    WAYPOINTLIST *wpl = NULL;
    short state;
    _TCHAR buf[39];
    VU_ID *tmpID = NULL;
    short airwps, lastwp;

    if (unit == NULL) return;

    if ( not unit->Final()) return;

    airwps = static_cast<short>(unit->IsFlight());

    if ( not airwps)
    {
        wp = unit->GetCurrentUnitWP();

        if (wp and wp->GetPrevWP())
            wp = wp->GetPrevWP();
    }

    if ( not wp or airwps)
        wp = unit->GetFirstUnitWP();

    if ( not wp) return;

    campID = unit->GetCampID() << 8;

    // set to 0 after Landing WP so we don't connect lines to following waypoints which are not on the agenda (Alt Land cit,Tanker etc)
    UseFlag = C_BIT_USELINE;

    i = 1;
    lastwp = 0;

    while (wp)
    {
        if ( not wp->GetNextWP())
            lastwp = 1;

        if (wp->GetWPFlags() bitand WPF_TARGET)
        {
            normID = TGT_OTR;
            selID = TGT_OTR_SEL;
            othrID = TGT_OTR_OTHER;
        }
        else if (wp->GetWPFlags() bitand WPF_IP)
        {
            normID = IP_OTR;
            selID = IP_OTR_SEL;
            othrID = IP_OTR_OTHER;
        }
        else
        {
            normID = STPT_OTR;
            selID = STPT_OTR_SEL;
            othrID = STPT_OTR_OTHER;
        }

        if (wp == unit->GetCurrentUnitWP())
            state = 1;
        else
            state = 0;

        wp->GetLocation(&x, &y, &z); // Note: for Sim -> UI (UI's) X = (Sim's) Y, (UI's Y) = (Sim's) [max y] - X (UI's) Z = (Sim's) -Z

        // Set 2d Waypoint
        if (UseFlag)
            _stprintf(buf, "%1d", i);
        else
        {
            if (wp->GetWPAction() == WP_LAND)
            {
                _sntprintf(buf, 39, "%s", gStringMgr->GetString(TXT_ALTERNATE_FIELD));
                buf[39] = 0;
            }
            else if (wp->GetWPAction() == WP_REFUEL)
            {
                _sntprintf(buf, 39, _T("%s"), gStringMgr->GetString(TXT_TANKER));
                buf[39] = 0;
            }
            else
                buf[0] = 0;
        }

        if (buf[0])
        {
            if (airwps)
            {
                if (i == 1)
                    wpl = wplist->AddWaypointToList(campID + i, 0, normID, selID, othrID, y, maxy - x, FALSE);
                else
                    wpl = wplist->AddWaypointToList(campID + i, 0, normID, selID, othrID, y, maxy - x, FALSE);
            }
            else
            {
                if (lastwp)
                    wpl = wplist->AddWaypointToList(campID + i, 0, normID, selID, othrID, y, maxy - x, FALSE);
                else
                    wpl = wplist->AddWaypointToList(campID + i, 0, 0, 0, 0, y, maxy - x, FALSE);
            }

            if (wpl)
            {
                wplist->SetWPGroup(campID + i, campID);

                // 2002-03-10 MN fix for black eagle on black ground ;-)
                if (g_bAWACSBackground)
                {
                    wplist->SetLabelColor(campID + i, 0x00999999, 0x00ffffff, 0x00999999);
                    wplist->SetLineColor(campID + i, 0x00999999, 0x00ffffff, 0x00999999);
                }
                else
                {
                    wplist->SetLabelColor(campID + i, 0, 0x00500000, 0);
                    wplist->SetLineColor(campID + i, 0, 0x00500000, 0);
                }

                tmpID = new VU_ID;
                *tmpID = unit->Id();
                wpl->Flags or_eq UseFlag;
                wpl->Icon->SetUserCleanupPtr(C_STATE_0, tmpID);
                wpl->Icon->SetUserNumber(C_STATE_1, i);
            }
        }

        i++;
        wp = wp->GetNextWP();
    }
}

BOOL C_Map::SetWaypointList(VU_ID unitID)
{
    F4CSECTIONHANDLE *Leave = NULL;
    Unit unit;

    if (unitID == FalconNullId)
        return(FALSE);

    unit = (Unit)FindUnit(unitID);

    if (unit == NULL) return(FALSE);

    if (DrawWindow_)
        Leave = UI_Enter(DrawWindow_);

    Team_[unit->GetTeam()].Waypoints->EraseWaypointGroup(unit->GetCampID() << 8);
    CampEnterCriticalSection();
    BuildWPList(Team_[unit->GetTeam()].Waypoints, NULL, unit);
    CampLeaveCriticalSection();
    Team_[unit->GetTeam()].Waypoints->Refresh();
    UI_Leave(Leave);
    return(TRUE);
}

BOOL C_Map::SetCurrentWaypointList(VU_ID unitID)
{
    F4CSECTIONHANDLE *Leave = NULL;
    Unit unit;

    if (CurWP_->Dragging()) return(FALSE);

    if (CurWPZ_->Dragging()) return(FALSE);

    CampEnterCriticalSection();

    if (DrawWindow_)
        Leave = UI_Enter(DrawWindow_);

    CurWP_->Refresh();
    CurWPZ_->Refresh();

    CurWP_->EraseWaypointList();
    CurWPZ_->EraseWaypointList();

    CurWPArea_.top = -1;
    CurWPArea_.left = -1;
    CurWPArea_.bottom = -1;
    CurWPArea_.right = -1;

    WPUnitID_ = unitID;
    gActiveFlightID = unitID;

    if (unitID == FalconNullId)
    {
        UI_Leave(Leave);
        CampLeaveCriticalSection();
        return(FALSE);
    }

    unit = (Unit)FindUnit(unitID);

    if (unit == NULL)
    {
        UI_Leave(Leave);
        CampLeaveCriticalSection();
        return(FALSE);
    }

    BuildCurrentWPList(unit);
    flags_ or_eq I_NEED_TO_DRAW;
    UI_Leave(Leave);
    CampLeaveCriticalSection();
    return(TRUE);
}

void C_Map::UpdateWaypoint(Flight flt)
{
    WayPoint wp;
    WAYPOINTLIST *nub;
    short i, check;

    if (flt->Id() == WPUnitID_)
    {
        // We have to traverse all our waypoints to check for validity
        if (flt->GetCurrentUnitWP() == flt->GetFirstUnitWP())
            check = 1;
        else
            check = 0;

        i = 1;
        wp = flt->GetFirstUnitWP();

        while (wp)
        {
            if ( not IsValidWP(wp, flt) and check)
            {
                CurWP_->SetState((flt->GetCampID() << 8) + i, 2);
                CurWP_->SetState(0x40000000 bitor (flt->GetCampID() << 8) + i, 2);
                CurWPZ_->SetState((flt->GetCampID() << 8) + i, 2);
            }
            else if (wp == flt->GetCurrentUnitWP())
            {
                CurWP_->SetState((flt->GetCampID() << 8) + i, 1);
                CurWP_->SetState(0x40000000 bitor (flt->GetCampID() << 8) + i, 1);
                CurWPZ_->SetState((flt->GetCampID() << 8) + i, 1);
            }
            else
            {
                CurWP_->SetState((flt->GetCampID() << 8) + i, 0);
                CurWP_->SetState(0x40000000 bitor (flt->GetCampID() << 8) + i, 0);
                CurWPZ_->SetState((flt->GetCampID() << 8) + i, 0);
            }

            i++;
            wp = wp->GetNextWP();
        }

        if ( not (TheCampaign.Flags bitand CAMP_TACTICAL_EDIT))
        {
            if (i > 1)
            {
                nub = CurWP_->GetRoot();

                while (nub)
                {
                    if (nub->ID bitand 0x40000000)
                    {
                        nub->Icon->SetText(0, TXT_SPACE);
                        nub->Icon->SetText(1, TXT_SPACE);
                        nub->Icon->SetText(2, TXT_SPACE);
                        nub->Icon->SetFlagBitOff(C_BIT_ENABLED);
                    }

                    nub = nub->Next;
                }

                CurWP_->SetFlagBitOff(C_BIT_DRAGABLE);
                CurWPZ_->SetFlagBitOff(C_BIT_DRAGABLE);
            }
        }

        CurWP_->Refresh();
        CurWPZ_->Refresh();
    }
    else
    {
        i = 1;
        wp = flt->GetFirstUnitWP();

        while (wp and wp not_eq flt->GetCurrentUnitWP())
        {
            i++;
            wp = wp->GetNextWP();
        }

        Team_[flt->GetTeam()].Waypoints->SetGroupState(flt->GetCampID() << 8, 0);
        Team_[flt->GetTeam()].Waypoints->SetState((flt->GetCampID() << 8) + i, 1);
        Team_[flt->GetTeam()].Waypoints->SetState(0x40000000 bitor (flt->GetCampID() << 8) + i, 1);
        Team_[flt->GetTeam()].Waypoints->Refresh();
    }
}

void C_Map::RemoveCurWPList()
{
    CurWP_->Refresh();
    CurWP_->EraseWaypointList();
    CurWPZ_->Refresh();
    CurWPZ_->EraseWaypointList();
    CurIcons_->Cleanup();
    CurIcons_->Setup(CurIcons_->GetID(), 0);
    WPUnitID_ = FalconNullId;
}

void C_Map::RemoveWaypoints(short team, long group)
{
    Team_[team].Waypoints->Refresh();
    Team_[team].Waypoints->EraseWaypointGroup(group);
}

// this is called like every 5 or 10 seconds by a callback...
void C_Map::RemoveOldWaypoints()
{
    Unit un;

    // remove current set
    if (WPUnitID_ not_eq FalconNullId)
    {
        un = (Unit)vuDatabase->Find(WPUnitID_);

        if ( not un)
            RemoveCurWPList();
    }

    // go through the teams and remove old WP lists
}

void C_Map::RemoveAllWaypoints(short owner)
{
    if ((DrawWindow_) and (Team_[owner].Waypoints))
        Team_[owner].Waypoints->EraseWaypointList();
}

void C_Map::RemoveFromCurIcons(long ID)
{
    if (CurIcons_)
        CurIcons_->Remove(ID);
}

void C_Map::SetBullsEye(float x, float y)
{
    BullsEyeX_ = x;
    BullsEyeY_ = y;

    if (BullsEye_)
        BullsEye_->SetPos(x, y); // Real World XY (where 0,0 is top left corner)
}

void C_Map::TurnOnBullseye()
{
    if (BullsEye_)
    {
        BullsEye_->SetFlagBitOff(C_BIT_INVISIBLE);

        if (DrawWindow_)
            DrawWindow_->RefreshWindow();
    }
}

void C_Map::TurnOffBullseye()
{
    if (BullsEye_)
    {
        BullsEye_->SetFlagBitOn(C_BIT_INVISIBLE);

        if (DrawWindow_)
            DrawWindow_->RefreshWindow();
    }
}

void C_Map::RemapTeamColors(long team)
{
    short j, k;

    if (team >= NUM_TEAMS)
        return;

    if (Team_[team].Objectives)
    {
        for (j = 0; j < _MAP_NUM_OBJ_TYPES_; j++)
        {
            Team_[team].Objectives->Type[j]->SetMainImage(ObjIconIDs_[team][0], ObjIconIDs_[team][1]);
            Team_[team].Objectives->Type[j]->RemapIconImages();
        }
    }

    if (Team_[team].NavalUnits)
    {
        for (j = 0; j < _MAP_NUM_NAV_TYPES_; j++)
        {
            Team_[team].NavalUnits->Type[j]->SetMainImage(NavyIconIDs_[team][0], NavyIconIDs_[team][1]);
            Team_[team].NavalUnits->Type[j]->RemapIconImages();
        }
    }

    if (Team_[team].Units)
    {
        for (j = 0; j < _MAP_NUM_GND_TYPES_; j++)
        {
            for (k = 0; k < _MAP_NUM_GND_LEVELS_; k++)
            {
                Team_[team].Units->Type[j]->Levels[k]->SetMainImage(ArmyIconIDs_[team][0], ArmyIconIDs_[team][1]);
                Team_[team].Units->Type[j]->Levels[k]->RemapIconImages();
            }
        }
    }

    if (Team_[team].AirUnits)
    {
        for (j = 0; j < _MAP_NUM_AIR_TYPES_; j++)
        {
            Team_[team].AirUnits->Type[j]->SetMainImage(C_STATE_0, AirIconIDs_[team][0][0], AirIconIDs_[team][0][1]);
            Team_[team].AirUnits->Type[j]->SetMainImage(C_STATE_1, AirIconIDs_[team][1][0], AirIconIDs_[team][1][1]);
            Team_[team].AirUnits->Type[j]->SetMainImage(C_STATE_2, AirIconIDs_[team][2][0], AirIconIDs_[team][2][1]);
            Team_[team].AirUnits->Type[j]->SetMainImage(C_STATE_3, AirIconIDs_[team][3][0], AirIconIDs_[team][3][1]);
            Team_[team].AirUnits->Type[j]->SetMainImage(C_STATE_4, AirIconIDs_[team][4][0], AirIconIDs_[team][4][1]);
            Team_[team].AirUnits->Type[j]->SetMainImage(C_STATE_5, AirIconIDs_[team][5][0], AirIconIDs_[team][5][1]);
            Team_[team].AirUnits->Type[j]->SetMainImage(C_STATE_6, AirIconIDs_[team][6][0], AirIconIDs_[team][6][1]);
            Team_[team].AirUnits->Type[j]->SetMainImage(C_STATE_7, AirIconIDs_[team][7][0], AirIconIDs_[team][7][1]);
            Team_[team].AirUnits->Type[j]->RemapIconImages();
        }
    }
}

void C_Map::FitFlightPlan()
{
    // Based on CurWP_'s x bitand y (ie: CurWPArea_)
    long cx, cy;
    long w, h;

    if (CurWPArea_.left < 0 or CurWPArea_.top < 0 or CurWPArea_.right < 0 or CurWPArea_.bottom < 0)
        return;

    w = (CurWPArea_.right - CurWPArea_.left) / 1000; // 1100 = ft -> 500m * 1.64 (allow for icons to fit on map also)
    h = (CurWPArea_.bottom - CurWPArea_.top) / 1000;

    if (w > h)
        ZoomLevel_ = w;
    else
        ZoomLevel_ = h;

    if (ZoomLevel_ < MaxZoomLevel_)
        ZoomLevel_ = MaxZoomLevel_;

    if (ZoomLevel_ > MinZoomLevel_)
        ZoomLevel_ = MinZoomLevel_;

    cx = (CurWPArea_.top / 1640 + CurWPArea_.bottom / 1640) / 2;
    cy = static_cast<long>(((maxy - CurWPArea_.left) / 1640 + (maxy - CurWPArea_.right) / 1640) / 2);

    SetMapCenter(cx, cy);
}

// Public
void C_Map::ShowObjectiveType(long mask)
{
    short i, j;
    F4CSECTIONHANDLE *Leave;

    ObjectiveMask_ or_eq (1 << FindTypeIndex(mask, OBJ_TypeList, _MAP_NUM_OBJ_TYPES_));

    Leave = UI_Enter(DrawWindow_);

    for (i = 0; i < _MAX_TEAMS_; i++)
        if (Team_[i].Objectives)
        {
            for (j = 0; j < _MAP_NUM_OBJ_TYPES_; j++)
                if ( not Team_[i].Objectives->Flags[j] and (ObjectiveMask_ bitand (1 << j)))
                {
                    Team_[i].Objectives->Flags[j] = 1;
                    Team_[i].Objectives->Type[j]->SetFlagBitOff(C_BIT_INVISIBLE);
                    Team_[i].Objectives->Type[j]->Refresh();
                }
        }

    UI_Leave(Leave);
}

void C_Map::HideObjectiveType(long mask)
{
    short i, j;
    long offflag;
    F4CSECTIONHANDLE *Leave;

    offflag = (1 << FindTypeIndex(mask, OBJ_TypeList, _MAP_NUM_OBJ_TYPES_));

    Leave = UI_Enter(DrawWindow_);

    for (i = 0; i < _MAX_TEAMS_; i++)
        if (Team_[i].Objectives)
        {
            for (j = 0; j < _MAP_NUM_OBJ_TYPES_; j++)
                if (Team_[i].Objectives->Flags[j] and (offflag bitand (1 << j)))
                {
                    Team_[i].Objectives->Type[j]->Refresh();
                    Team_[i].Objectives->Flags[j] = 0;
                    Team_[i].Objectives->Type[j]->SetFlagBitOn(C_BIT_INVISIBLE);
                }
        }

    UI_Leave(Leave);
    ObjectiveMask_ and_eq compl offflag;
}

void C_Map::ShowUnitType(long mask)
{
    short i, j, k;
    F4CSECTIONHANDLE *Leave;

    UnitMask_ or_eq (1 << FindTypeIndex(mask, GND_TypeList, _MAP_NUM_GND_TYPES_));

    Leave = UI_Enter(DrawWindow_);

    for (i = 0; i < _MAX_TEAMS_; i++)
        if (Team_[i].Units)
        {
            for (j = 0; j < _MAP_NUM_GND_TYPES_; j++)
            {
                if ( not Team_[i].Units->Flags[j] and (UnitMask_ bitand (1 << j)))
                {
                    Team_[i].Units->Flags[j] = 1;

                    for (k = 0; k < _MAP_NUM_GND_LEVELS_; k++)
                    {
                        if (Team_[i].Units->Type[j]->Flags[k] == 1)
                        {
                            Team_[i].Units->Type[j]->Levels[k]->SetFlagBitOff(C_BIT_INVISIBLE);
                            Team_[i].Units->Type[j]->Levels[k]->Refresh();
                        }
                    }
                }
            }
        }

    UI_Leave(Leave);
}

void C_Map::HideUnitType(long mask)
{
    short i, j, k;
    long offflag;
    F4CSECTIONHANDLE *Leave;

    offflag = (1 << FindTypeIndex(mask, GND_TypeList, _MAP_NUM_GND_TYPES_));

    Leave = UI_Enter(DrawWindow_);

    for (i = 0; i < _MAX_TEAMS_; i++)
        if (Team_[i].Units)
        {
            for (j = 0; j < _MAP_NUM_GND_TYPES_; j++)
            {
                if (Team_[i].Units->Flags[j] and (offflag bitand (1 << j)))
                {
                    Team_[i].Units->Flags[j] = 0;

                    for (k = 0; k < _MAP_NUM_GND_LEVELS_; k++)
                    {
                        if (Team_[i].Units->Type[j]->Flags[k] == 1)
                        {
                            Team_[i].Units->Type[j]->Levels[k]->Refresh();
                            Team_[i].Units->Type[j]->Levels[k]->SetFlagBitOn(C_BIT_INVISIBLE);
                        }
                    }
                }
            }
        }

    UI_Leave(Leave);
    UnitMask_ and_eq compl offflag;
}

void C_Map::SetUnitLevel(long level)
{
    short i, j;
    F4CSECTIONHANDLE *Leave;

    Leave = UI_Enter(DrawWindow_);

    for (i = 0; i < _MAX_TEAMS_; i++)
        if (Team_[i].Units)
        {
            for (j = 0; j < _MAP_NUM_GND_TYPES_; j++)
                if (Team_[i].Units->Type[j])
                {
                    if (level >= 0 and level < _MAP_NUM_GND_LEVELS_)
                    {
                        Team_[i].Units->Type[j]->Flags[0] = 0;

                        if (level)
                            Team_[i].Units->Type[j]->Levels[0]->Refresh();

                        Team_[i].Units->Type[j]->Levels[0]->SetFlagBitOn(C_BIT_INVISIBLE);
                        Team_[i].Units->Type[j]->Flags[1] = 0;

                        if (level not_eq 1)
                            Team_[i].Units->Type[j]->Levels[1]->Refresh();

                        Team_[i].Units->Type[j]->Levels[1]->SetFlagBitOn(C_BIT_INVISIBLE);
                        Team_[i].Units->Type[j]->Flags[2] = 0;

                        if (level not_eq 2)
                            Team_[i].Units->Type[j]->Levels[2]->Refresh();

                        Team_[i].Units->Type[j]->Levels[2]->SetFlagBitOn(C_BIT_INVISIBLE);
                        Team_[i].Units->Type[j]->Flags[level] = 1;

                        if (Team_[i].Units->Flags[j])
                        {
                            Team_[i].Units->Type[j]->Levels[level]->SetFlagBitOff(C_BIT_INVISIBLE);
                            Team_[i].Units->Type[j]->Levels[level]->Refresh();
                        }
                    }
                }
        }

    UI_Leave(Leave);
}

void C_Map::ShowAirUnitType(long mask)
{
    short i, j;
    F4CSECTIONHANDLE *Leave;

    AirUnitMask_ or_eq (1 << FindTypeIndex(mask, AIR_TypeList, _MAP_NUM_AIR_TYPES_));

    Leave = UI_Enter(DrawWindow_);

    for (i = 0; i < _MAX_TEAMS_; i++)
        if (Team_[i].AirUnits)
        {
            for (j = 0; j < _MAP_NUM_AIR_TYPES_; j++)
                if ( not Team_[i].AirUnits->Flags[j] and (AirUnitMask_ bitand (1 << j)))
                {
                    Team_[i].AirUnits->Flags[j] = 1;
                    Team_[i].AirUnits->Type[j]->SetFlagBitOff(C_BIT_INVISIBLE);
                    Team_[i].AirUnits->Type[j]->Refresh();
                }
        }

    UI_Leave(Leave);
}

void C_Map::HideAirUnitType(long mask)
{
    short i, j;
    long offflag;
    F4CSECTIONHANDLE *Leave;

    offflag = (1 << FindTypeIndex(mask, AIR_TypeList, _MAP_NUM_AIR_TYPES_));

    Leave = UI_Enter(DrawWindow_);

    for (i = 0; i < _MAX_TEAMS_; i++)
        if (Team_[i].AirUnits)
        {
            for (j = 0; j < _MAP_NUM_AIR_TYPES_; j++)
                if (Team_[i].AirUnits->Flags[j] and (offflag bitand (1 << j)))
                {
                    Team_[i].AirUnits->Type[j]->Refresh();
                    Team_[i].AirUnits->Flags[j] = 0;
                    Team_[i].AirUnits->Type[j]->SetFlagBitOn(C_BIT_INVISIBLE);
                }
        }

    UI_Leave(Leave);
    AirUnitMask_ and_eq compl offflag;
}

// 2002-02-21 ADDED BY S.G. Goes throuh all the AIR_TypeList and refresh them
void C_Map::RefreshAllAirUnitType(void)
{
    short i, j;
    F4CSECTIONHANDLE *Leave;

    Leave = UI_Enter(DrawWindow_);

    for (i = 0; i < _MAX_TEAMS_; i++)
    {
        if (Team_[i].AirUnits)
        {
            for (j = 0; j < _MAP_NUM_AIR_TYPES_; j++)
                Team_[i].AirUnits->Type[j]->Refresh();
        }
    }

    UI_Leave(Leave);
}

void C_Map::ShowNavalUnitType(long mask)
{
    short i, j;
    F4CSECTIONHANDLE *Leave;

    NavalUnitMask_ or_eq (1 << FindTypeIndex(mask, NAV_TypeList, _MAP_NUM_NAV_TYPES_));

    Leave = UI_Enter(DrawWindow_);

    for (i = 0; i < _MAX_TEAMS_; i++)
        if (Team_[i].NavalUnits)
        {
            for (j = 0; j < _MAP_NUM_NAV_TYPES_; j++)
                if ( not Team_[i].NavalUnits->Flags[j] and (NavalUnitMask_ bitand (1 << j)))
                {
                    Team_[i].NavalUnits->Flags[j] = 1;
                    Team_[i].NavalUnits->Type[j]->SetFlagBitOff(C_BIT_INVISIBLE);
                    Team_[i].NavalUnits->Type[j]->Refresh();
                }
        }

    UI_Leave(Leave);
}

void C_Map::HideNavalUnitType(long mask)
{
    short i, j;
    long offflag;
    F4CSECTIONHANDLE *Leave;

    offflag = (1 << FindTypeIndex(mask, NAV_TypeList, _MAP_NUM_NAV_TYPES_));

    Leave = UI_Enter(DrawWindow_);

    for (i = 0; i < _MAX_TEAMS_; i++)
        if (Team_[i].NavalUnits)
        {
            for (j = 0; j < _MAP_NUM_NAV_TYPES_; j++)
                if (Team_[i].NavalUnits->Flags[j] and (offflag bitand (1 << j)))
                {
                    Team_[i].NavalUnits->Type[j]->Refresh();
                    Team_[i].NavalUnits->Flags[j] = 0;
                    Team_[i].NavalUnits->Type[j]->SetFlagBitOn(C_BIT_INVISIBLE);
                }
        }

    UI_Leave(Leave);
    NavalUnitMask_ and_eq compl offflag;
}

void C_Map::ShowThreatType(long mask)
{
    short i, j;
    long timestamp;
    F4CSECTIONHANDLE *Leave;

    ThreatMask_ = (1 << FindTypeIndex(mask, THR_TypeList, _MAP_NUM_THREAT_TYPES_));

    timestamp = GetCurrentTime();
    MonoPrint("Start at %1ld...", timestamp);
    Leave = UI_Enter(DrawWindow_);

    for (i = 0; i < _MAX_TEAMS_; i++)
        if (Team_[i].Threats)
        {
            for (j = 0; j < _MAP_NUM_THREAT_TYPES_; j++)
                if ( not Team_[i].Threats->Flags[j] and (ThreatMask_ bitand (1 << j)))
                {
                    Team_[i].Threats->Flags[j] = 1;
                    Team_[i].Threats->Type[j]->SetFlagBitOff(C_BIT_INVISIBLE);
                    Team_[i].Threats->Type[j]->Refresh();
                }
        }

    if (ThreatMask_ bitand _THR_SAM_LOW)
    {
        Map_->PreparePalette(0x0000ff);
        Map_->ClearOverlay();

        for (i = 0; i < _MAX_TEAMS_; i++)
            if (Team_[i].Threats)
            {
                if (Team_[i].Threats->Flags[_THREAT_SAM_LOW_])
                {
                    Team_[i].Threats->Type[_THREAT_SAM_LOW_]->BuildOverlay(Map_->GetOverlay(), Map_->GetW(), Map_->GetH(), 2);
                }
            }

        Map_->UseOverlay();
    }
    else if (ThreatMask_ bitand _THR_SAM_HIGH)
    {
        Map_->PreparePalette(0x00ffff);
        Map_->ClearOverlay();

        for (i = 0; i < _MAX_TEAMS_; i++)
            if (Team_[i].Threats)
            {
                if (Team_[i].Threats->Flags[_THREAT_SAM_HIGH_])
                {
                    Team_[i].Threats->Type[_THREAT_SAM_HIGH_]->BuildOverlay(Map_->GetOverlay(), Map_->GetW(), Map_->GetH(), 2);
                }
            }

        Map_->UseOverlay();
    }
    else if (ThreatMask_ bitand _THR_RADAR_LOW)
    {
        Map_->PreparePalette(0xffff00);
        Map_->ClearOverlay();

        for (i = 0; i < _MAX_TEAMS_; i++)
            if (Team_[i].Threats)
            {
                if (Team_[i].Threats->Flags[_THREAT_RADAR_LOW_])
                {
                    Team_[i].Threats->Type[_THREAT_RADAR_LOW_]->BuildOverlay(Map_->GetOverlay(), Map_->GetW(), Map_->GetH(), 2);
                }
            }

        Map_->UseOverlay();
    }
    else if (ThreatMask_ bitand _THR_RADAR_HIGH)
    {
        Map_->PreparePalette(0xff0000);
        Map_->ClearOverlay();

        for (i = 0; i < _MAX_TEAMS_; i++)
            if (Team_[i].Threats)
            {
                if (Team_[i].Threats->Flags[_THREAT_RADAR_HIGH_])
                {
                    Team_[i].Threats->Type[_THREAT_RADAR_HIGH_]->BuildOverlay(Map_->GetOverlay(), Map_->GetW(), Map_->GetH(), 2);
                }
            }

        Map_->UseOverlay();
    }

    timestamp = GetCurrentTime() - timestamp;
    MonoPrint("Total time=%1ld\n", timestamp);
#if 0
    Circles_ = ThreatMask_;

    for (i = 0; i < _MAX_TEAMS_; i++)
        for (j = 0; j < _MAP_NUM_GND_TYPES_; j++)
            Team_[i].Units->Type[j]->Levels[2]->ShowCircles(Circles_);

#endif
    flags_ or_eq I_NEED_TO_DRAW_MAP;
    UI_Leave(Leave);
}

void C_Map::HideThreatType(long mask)
{
    short i, j;
    long offflag;
    F4CSECTIONHANDLE *Leave;

    offflag = (1 << FindTypeIndex(mask, THR_TypeList, _MAP_NUM_THREAT_TYPES_));

    Leave = UI_Enter(DrawWindow_);

    for (i = 0; i < _MAX_TEAMS_; i++)
        if (Team_[i].Threats)
        {
            for (j = 0; j < _MAP_NUM_THREAT_TYPES_; j++)
                if (Team_[i].Threats->Flags[j] and (offflag bitand (1 << j)))
                {
                    Team_[i].Threats->Type[j]->Refresh();
                    Team_[i].Threats->Flags[j] = 0;
                    Team_[i].Threats->Type[j]->SetFlagBitOn(C_BIT_INVISIBLE);
                }
        }

    ThreatMask_ and_eq compl offflag;

    Map_->NoOverlay();
#if 0
    Circles_ = ThreatMask_;

    for (i = 0; i < _MAX_TEAMS_; i++)
        for (j = 0; j < _MAP_NUM_GND_TYPES_; j++)
            Team_[i].Units->Type[j]->Levels[2]->ShowCircles(Circles_);

#endif
    flags_ or_eq I_NEED_TO_DRAW_MAP;
    UI_Leave(Leave);
}

void C_Map::SetMapImage(long ID)
{
    MapID = ID;

    if (Map_ == NULL)
    {
        Map_ = new C_ScaleBitmap;
        Map_->Setup(5551200, 0, MapID);
    }

    Map_->SetImage(MapID);
    maxy = (float)(Map_->GetH()) * FEET_PER_PIXEL;
    MinZoomLevel_ = Map_->GetW() / _MIN_ZOOM_LEVEL_;
    MaxZoomLevel_ = Map_->GetW() / _MAX_ZOOM_LEVEL_;
    ZoomStep_ = (MinZoomLevel_ - MaxZoomLevel_) / 64;

    if (Map_ and DrawWindow_)
    {
        DrawRect_ = DrawWindow_->ClientArea_[0];
        CalculateDrawingParams();
    }
}

void C_Map::SetWindow(C_Window *win)
{
    if (win)
    {
        if (DrawWindow_)
            RemoveListsFromWindow();

        DrawWindow_ = win;

        if (DrawWindow_)
        {
            DrawRect_ = DrawWindow_->ClientArea_[0];
            AddListsToWindow();
            CalculateDrawingParams();
        }
    }
}

void C_Map::SetupOverlay()
{
    if (Map_)
        Map_->InitOverlay();
}

void C_Map::SetWPZWindow(C_Window *win)
{
    WPZWindow_ = win;
}

void C_Map::SetZoomLevel(short zoom)
{
    if (zoom >= _MIN_ZOOM_LEVEL_ and zoom <= _MAX_ZOOM_LEVEL_ and zoom not_eq ZoomLevel_ and Map_)
    {
        ZoomLevel_ = Map_->GetW() / zoom;
        CalculateDrawingParams();
    }
}

void C_Map::ZoomIn()
{
    long tempzoom;

    tempzoom = ZoomLevel_ - (ZoomStep_ + (ZoomLevel_ >> 6));

    if (tempzoom > MinZoomLevel_)
        tempzoom = MinZoomLevel_;

    if (tempzoom < MaxZoomLevel_)
        tempzoom = MaxZoomLevel_;

    if (tempzoom not_eq ZoomLevel_)
    {
        ZoomLevel_ = tempzoom;
        CalculateDrawingParams();
    }
}

void C_Map::ZoomOut()
{
    long tempzoom;

    tempzoom = ZoomLevel_ + (ZoomStep_ + (ZoomLevel_ >> 6));

    if (tempzoom > MinZoomLevel_)
        tempzoom = MinZoomLevel_;

    if (tempzoom < MaxZoomLevel_)
        tempzoom = MaxZoomLevel_;

    if (tempzoom not_eq ZoomLevel_)
    {
        ZoomLevel_ = tempzoom;
        CalculateDrawingParams();
    }
}

void C_Map::SetMapCenter(long x, long y)
{
    CenterX_ = static_cast<float>(x);
    CenterY_ = static_cast<float>(y);

    float mx = Map_ ? Map_->GetW() : 2048.0F;
    float my = Map_ ? Map_->GetH() : 2048.0F;

    if (CenterX_ < 0) CenterX_ = 0;

    if (CenterX_ >= mx) CenterX_ = mx - 1;

    if (CenterY_ < 0) CenterY_ = 0;

    if (CenterY_ >= my) CenterY_ = my - 1;

    CalculateDrawingParams();
}

void C_Map::MoveCenter(long x, long y)
{
    float distance;

    if (Map_ == NULL or DrawWindow_ == NULL)
        return;

    if ( not x and not y)
        return;

    distance = (float)(MapRect_.right - MapRect_.left) / (DrawWindow_->ClientArea_[0].right - DrawWindow_->ClientArea_[0].left);

    CenterX_ += (float)x * distance;
    CenterY_ += (float)y * distance;

    if (CenterX_ < 0) CenterX_ = 0;

    if (CenterX_ >= Map_->GetW()) CenterX_ = Map_->GetW() - 1.0f;

    if (CenterY_ < 0) CenterY_ = 0;

    if (CenterY_ >= Map_->GetH()) CenterY_ = Map_->GetH() - 1.0f;

    CalculateDrawingParams();
}

void C_Map::TurnOnNames()
{
    short i, j, k;

    for (i = 0; i < _MAX_TEAMS_; i++)
    {
        for (j = 0; j < _MAP_NUM_OBJ_TYPES_; j++)
            Team_[i].Objectives->Type[j]->SetFlagBitOff(C_BIT_NOLABEL);

        for (j = 0; j < _MAP_NUM_NAV_TYPES_; j++)
            Team_[i].NavalUnits->Type[j]->SetFlagBitOff(C_BIT_NOLABEL);

        for (j = 0; j < _MAP_NUM_GND_TYPES_; j++)
            for (k = 0; k < _MAP_NUM_GND_LEVELS_; k++)
                Team_[i].Units->Type[j]->Levels[k]->SetFlagBitOff(C_BIT_NOLABEL);

        for (j = 0; j < _MAP_NUM_AIR_TYPES_; j++)
            Team_[i].AirUnits->Type[j]->SetFlagBitOff(C_BIT_NOLABEL);
    }

    flags_ or_eq I_NEED_TO_DRAW_MAP;
}

void C_Map::TurnOnBoundaries()
{
}

void C_Map::TurnOnArrows()
{
}

void C_Map::TurnOffNames()
{
    short i, j, k;

    for (i = 0; i < _MAX_TEAMS_; i++)
    {
        for (j = 0; j < _MAP_NUM_OBJ_TYPES_; j++)
            Team_[i].Objectives->Type[j]->SetFlagBitOn(C_BIT_NOLABEL);

        for (j = 0; j < _MAP_NUM_NAV_TYPES_; j++)
            Team_[i].NavalUnits->Type[j]->SetFlagBitOn(C_BIT_NOLABEL);

        for (j = 0; j < _MAP_NUM_GND_TYPES_; j++)
            for (k = 0; k < _MAP_NUM_GND_LEVELS_; k++)
                Team_[i].Units->Type[j]->Levels[k]->SetFlagBitOn(C_BIT_NOLABEL);

        for (j = 0; j < _MAP_NUM_AIR_TYPES_; j++)
            Team_[i].AirUnits->Type[j]->SetFlagBitOn(C_BIT_NOLABEL);
    }

    flags_ or_eq I_NEED_TO_DRAW_MAP;
}

void C_Map::TurnOffBoundaries()
{
}

void C_Map::TurnOffArrows()
{
}

void C_Map::SetObjCallbacks(long type, void (*cb)(long, short, C_Base *))
{
    long i;

    if (type < _MAP_NUM_OBJ_TYPES_)
    {
        for (i = 0; i < _MAX_TEAMS_; i++)
            if (Team_[i].Objectives->Type[type])
                Team_[i].Objectives->Type[type]->SetCallback(cb);
    }
}

void C_Map::SetAllObjCallbacks(void (*cb)(long, short, C_Base *))
{
    long i, j;

    for (i = 0; i < _MAX_TEAMS_; i++)
        for (j = 0; j < _MAP_NUM_OBJ_TYPES_; j++)
            if (Team_[i].Objectives->Type[j])
                Team_[i].Objectives->Type[j]->SetCallback(cb);
}

void C_Map::SetAllAirUnitCallbacks(void (*cb)(long, short, C_Base*))
{
    long i, j;

    for (i = 0; i < _MAX_TEAMS_; i++)
        for (j = 0; j < _MAP_NUM_AIR_TYPES_; j++)
            if (Team_[i].AirUnits->Type[j])
                Team_[i].AirUnits->Type[j]->SetCallback(cb);
}

void C_Map::SetAllGroundUnitCallbacks(void (*cb)(long, short, C_Base*))
{
    long i, j, k;

    for (i = 0; i < _MAX_TEAMS_; i++)
        for (j = 0; j < _MAP_NUM_GND_TYPES_; j++)
            for (k = 0; k < _MAP_NUM_GND_LEVELS_; k++)
                if (Team_[i].Units->Type[j])
                    if (Team_[i].Units->Type[j]->Levels[k])
                        Team_[i].Units->Type[j]->Levels[k]->SetCallback(cb);
}

void C_Map::SetAllNavalUnitCallbacks(void (*cb)(long, short, C_Base*))
{
    long i, j;

    for (i = 0; i < _MAX_TEAMS_; i++)
        for (j = 0; j < _MAP_NUM_NAV_TYPES_; j++)
            if (Team_[i].NavalUnits->Type[j])
                Team_[i].NavalUnits->Type[j]->SetCallback(cb);
}

void C_Map::SetAirUnitCallbacks(long type, void (*cb)(long, short, C_Base*))
{
    long i;

    for (i = 0; i < _MAX_TEAMS_; i++)
        if (Team_[i].AirUnits->Type[type])
            Team_[i].AirUnits->Type[type]->SetCallback(cb);
}


void C_Map::SetGroundUnitCallbacks(long level, long type, void (*cb)(long, short, C_Base*))
{
    long i;

    for (i = 0; i < _MAX_TEAMS_; i++)
        if (Team_[i].Units->Type[type])
            if (Team_[i].Units->Type[type]->Levels[level])
                Team_[i].Units->Type[type]->Levels[level]->SetCallback(cb);
}

void C_Map::SetNavalUnitCallbacks(long type, void (*cb)(long, short, C_Base*))
{
    long i;

    for (i = 0; i < _MAX_TEAMS_; i++)
        if (Team_[i].NavalUnits->Type[type])
            Team_[i].NavalUnits->Type[type]->SetCallback(cb);
}

C_MapIcon *C_Map::GetObjIconList(long team, long type)
{
    return(Team_[team].Objectives->Type[type]);
}

void C_Map::AddListsToWindow()
{
    short i, j, k;
    long Font = 6;

    if (Map_ == NULL)
    {
        Map_ = new C_ScaleBitmap;
        Map_->Setup(5551000, 0, MapID);
    }

    DrawWindow_->AddControl(Map_);

    // Although The Threats are created here... they don't actually get put into a window

    if ( not BullsEye_)
    {
        BullsEye_ = new C_BullsEye;
        BullsEye_->Setup(5550900, 0);
        BullsEye_->SetColor(0x005500);
        BullsEye_->SetFlagBitOn(C_BIT_INVISIBLE);
    }

    DrawWindow_->AddControl(BullsEye_);

    for (i = 0; i < _MAX_TEAMS_; i++)
    {
        if (Team_[i].Threats == NULL)
        {
            Team_[i].Threats = new THR_LIST;

            for (j = 0; j < _MAP_NUM_THREAT_TYPES_; j++)
            {
                Team_[i].Threats->Type[j] = new C_Threat;
                Team_[i].Threats->Type[j]->Setup(0, 0);

                if (ThreatMask_ bitand (1 << j))
                {
                    Team_[i].Threats->Flags[j] = 1;
                    Team_[i].Threats->Type[j]->SetFlagBitOff(C_BIT_INVISIBLE);
                }
                else
                {
                    Team_[i].Threats->Flags[j] = 0;
                    Team_[i].Threats->Type[j]->SetFlagBitOn(C_BIT_INVISIBLE);
                }
            }

            SetTeamFlags(i, GetTeamFlags(i) bitor _MAP_THREATS_);
        }

        if (Team_[i].Objectives == NULL)
        {
            Team_[i].Objectives = new OBJ_LIST;

            for (j = 0; j < _MAP_NUM_OBJ_TYPES_; j++)
            {
                Team_[i].Objectives->Type[j] = new C_MapIcon;
                Team_[i].Objectives->Type[j]->Setup(5551000 + i + j * 10, j);
                Team_[i].Objectives->Type[j]->SetFont(Font);
                Team_[i].Objectives->Type[j]->SetTeam(i);
                Team_[i].Objectives->Type[j]->SetMainImage(ObjIconIDs_[i][0], ObjIconIDs_[i][1]);

                if (j == FindTypeIndex(_UNIT_PACKAGE, OBJ_TypeList, _MAP_NUM_OBJ_TYPES_))
                    Team_[i].Objectives->Type[j]->SetMenu(PACKAGE_POP);
                else if (j == FindTypeIndex(_UNIT_SQUADRON, OBJ_TypeList, _MAP_NUM_OBJ_TYPES_))
                    Team_[i].Objectives->Type[j]->SetMenu(SQUADRON_POP);
                else
                    Team_[i].Objectives->Type[j]->SetMenu(OBJECTIVE_POP);

                Team_[i].Objectives->Type[j]->SetCursorID(CRSR_F16_RM);
                Team_[i].Objectives->Type[j]->SetFlagBitOn(C_BIT_NOLABEL);

                if (ObjectiveMask_ bitand (1 << j))
                {
                    Team_[i].Objectives->Flags[j] = 1;
                    Team_[i].Objectives->Type[j]->SetFlagBitOff(C_BIT_INVISIBLE);
                }
                else
                {
                    Team_[i].Objectives->Flags[j] = 0;
                    Team_[i].Objectives->Type[j]->SetFlagBitOn(C_BIT_INVISIBLE);
                }
            }

            SetTeamFlags(i, GetTeamFlags(i) bitor _MAP_OBJECTIVES_);
        }

        for (j = 0; j < _MAP_NUM_OBJ_TYPES_; j++)
            DrawWindow_->AddControl(Team_[i].Objectives->Type[j]);
    }

    for (i = 0; i < _MAX_TEAMS_; i++)
    {
        if (Team_[i].NavalUnits == NULL)
        {
            Team_[i].NavalUnits = new NAV_LIST;

            for (j = 0; j < _MAP_NUM_NAV_TYPES_; j++)
            {
                Team_[i].NavalUnits->Type[j] = new C_MapIcon;
                Team_[i].NavalUnits->Type[j]->Setup(5551800 + i + j * 10, j);
                Team_[i].NavalUnits->Type[j]->SetMainImage(NavyIconIDs_[i][0], NavyIconIDs_[i][1]);
                Team_[i].NavalUnits->Type[j]->SetFont(Font);
                Team_[i].NavalUnits->Type[j]->SetTeam(i);
                Team_[i].NavalUnits->Type[j]->SetMenu(NAVAL_POP);
                Team_[i].NavalUnits->Type[j]->SetCallback(UnitCB);
                Team_[i].NavalUnits->Type[j]->SetCursorID(CRSR_F16_RM);
                Team_[i].NavalUnits->Type[j]->SetFlagBitOn(C_BIT_NOLABEL);

                if (TheCampaign.Flags bitand CAMP_TACTICAL_EDIT)
                    Team_[i].NavalUnits->Type[j]->SetFlagBitOn(C_BIT_DRAGABLE);
                else
                    Team_[i].NavalUnits->Type[j]->SetFlagBitOff(C_BIT_DRAGABLE);

                if (NavalUnitMask_ bitand (1 << j))
                {
                    Team_[i].NavalUnits->Flags[j] = 1;
                    Team_[i].NavalUnits->Type[j]->SetFlagBitOff(C_BIT_INVISIBLE);
                }
                else
                {
                    Team_[i].NavalUnits->Flags[j] = 0;
                    Team_[i].NavalUnits->Type[j]->SetFlagBitOn(C_BIT_INVISIBLE);
                }
            }

            SetTeamFlags(i, GetTeamFlags(i) bitor _MAP_NAVAL_UNITS_);
        }

        for (j = 0; j < _MAP_NUM_NAV_TYPES_; j++)
            DrawWindow_->AddControl(Team_[i].NavalUnits->Type[j]);
    }

    for (i = 0; i < _MAX_TEAMS_; i++)
    {
        if (Team_[i].Units == NULL)
        {
            Team_[i].Units = new GND_LIST;

            for (j = 0; j < _MAP_NUM_GND_TYPES_; j++)
            {
                Team_[i].Units->Flags[j] = 0;
                Team_[i].Units->Type[j] = new GND_SIZE;

                for (k = 0; k < _MAP_NUM_GND_LEVELS_; k++)
                {
                    Team_[i].Units->Type[j]->Flags[k] = 0;
                    Team_[i].Units->Type[j]->Levels[k] = new C_MapIcon;
                    Team_[i].Units->Type[j]->Levels[k]->Setup(static_cast<short>(5551300 + i + j * 10 + k * 100), static_cast<short>(j + (k << 8)));
                    Team_[i].Units->Type[j]->Levels[k]->SetMainImage(ArmyIconIDs_[i][0], ArmyIconIDs_[i][1]);
                    Team_[i].Units->Type[j]->Levels[k]->SetFont(Font);
                    Team_[i].Units->Type[j]->Levels[k]->SetTeam(i);
                    Team_[i].Units->Type[j]->Levels[k]->SetMenu(UNIT_POP);
                    Team_[i].Units->Type[j]->Levels[k]->SetCursorID(CRSR_F16_RM);
                    Team_[i].Units->Type[j]->Levels[k]->SetFlagBitOn(C_BIT_NOLABEL);
                    Team_[i].Units->Type[j]->Levels[k]->SetCallback(UnitCB);

                    if (TheCampaign.Flags bitand CAMP_TACTICAL_EDIT)
                        Team_[i].Units->Type[j]->Levels[k]->SetFlagBitOn(C_BIT_DRAGABLE);
                    else
                        Team_[i].Units->Type[j]->Levels[k]->SetFlagBitOff(C_BIT_DRAGABLE);

                    if (UnitMask_ bitand (1 << j))
                    {
                        Team_[i].Units->Type[j]->Flags[k] = 1;
                        Team_[i].Units->Type[j]->Levels[k]->SetFlagBitOff(C_BIT_INVISIBLE);
                    }
                    else
                    {
                        Team_[i].Units->Type[j]->Flags[k] = 0;
                        Team_[i].Units->Type[j]->Levels[k]->SetFlagBitOn(C_BIT_INVISIBLE);
                    }
                }
            }

            SetUnitLevel(0);
            SetTeamFlags(i, GetTeamFlags(i) bitor _MAP_UNITS_);
        }

        for (j = 0; j < _MAP_NUM_GND_TYPES_; j++)
            for (k = 0; k < _MAP_NUM_GND_LEVELS_; k++)
                DrawWindow_->AddControl(Team_[i].Units->Type[j]->Levels[k]);
    }

    if (CurIcons_ == NULL)
    {
        CurIcons_ = new C_DrawList;
        CurIcons_->Setup(5553000, 0);
    }

    DrawWindow_->AddControl(CurIcons_);

    for (i = 0; i < _MAX_TEAMS_; i++)
    {
        if (Team_[i].Waypoints == NULL)
        {
            Team_[i].Waypoints = new C_Waypoint;
            Team_[i].Waypoints->Setup(5551272 + i, C_TYPE_DRAGXY);
            Team_[i].Waypoints->SetCallback(WaypointCB);
            Team_[i].Waypoints->SetFont(Font);
            Team_[i].Waypoints->SetFlagBitOn(C_BIT_TOP);
            Team_[i].Waypoints->SetCursorID(CRSR_STEERPOINT);
            SetTeamFlags(i, GetTeamFlags(i) bitor _MAP_WAYPOINTS_);
        }

        DrawWindow_->AddControl(Team_[i].Waypoints);
    }

    // Current Waypoints (After the other waypoints... so they show up infront... (behind the airplanes though))
    if ( not CurWP_)
    {
        CurWP_ = new C_Waypoint;
        CurWP_->Setup(5555000, C_TYPE_DRAGXY);
        CurWP_->SetFlagBitOn(C_BIT_DRAGABLE);
        CurWP_->SetCallback(WaypointCB);
        CurWP_->SetFont(Font);
        CurWP_->SetFlagBitOn(C_BIT_TOP);
        CurWP_->SetCursorID(CRSR_STEERPOINT);
    }

    DrawWindow_->AddControl(CurWP_);

    for (i = 0; i < _MAX_TEAMS_; i++)
    {
        if (Team_[i].AirUnits == NULL)
        {
            Team_[i].AirUnits = new AIR_LIST;

            for (j = 0; j < _MAP_NUM_AIR_TYPES_; j++)
            {
                Team_[i].AirUnits->Type[j] = new C_MapIcon;
                Team_[i].AirUnits->Type[j]->Setup(5551201 + i + j * 10, j);
                Team_[i].AirUnits->Type[j]->SetFont(Font);
                Team_[i].AirUnits->Type[j]->SetTeam(i);

                Team_[i].AirUnits->Type[j]->SetMainImage(C_STATE_0, AirIconIDs_[i][0][0], AirIconIDs_[i][0][1]);
                Team_[i].AirUnits->Type[j]->SetMainImage(C_STATE_1, AirIconIDs_[i][1][0], AirIconIDs_[i][1][1]);
                Team_[i].AirUnits->Type[j]->SetMainImage(C_STATE_2, AirIconIDs_[i][2][0], AirIconIDs_[i][2][1]);
                Team_[i].AirUnits->Type[j]->SetMainImage(C_STATE_3, AirIconIDs_[i][3][0], AirIconIDs_[i][3][1]);
                Team_[i].AirUnits->Type[j]->SetMainImage(C_STATE_4, AirIconIDs_[i][4][0], AirIconIDs_[i][4][1]);
                Team_[i].AirUnits->Type[j]->SetMainImage(C_STATE_5, AirIconIDs_[i][5][0], AirIconIDs_[i][5][1]);
                Team_[i].AirUnits->Type[j]->SetMainImage(C_STATE_6, AirIconIDs_[i][6][0], AirIconIDs_[i][6][1]);
                Team_[i].AirUnits->Type[j]->SetMainImage(C_STATE_7, AirIconIDs_[i][7][0], AirIconIDs_[i][7][1]);

                Team_[i].AirUnits->Type[j]->SetFlagBitOn(C_BIT_NOLABEL);

                if (TheCampaign.Flags bitand CAMP_TACTICAL_EDIT)
                    Team_[i].AirUnits->Type[j]->SetFlagBitOn(C_BIT_DRAGABLE);
                else
                    Team_[i].AirUnits->Type[j]->SetFlagBitOff(C_BIT_DRAGABLE);

                Team_[i].AirUnits->Type[j]->SetCursorID(CRSR_F16_RM);
                Team_[i].AirUnits->Type[j]->SetMenu(AIRUNIT_MENU);
                Team_[i].AirUnits->Type[j]->SetCallback(UnitCB);

                if (AirUnitMask_ bitand (1 << j))
                {
                    Team_[i].AirUnits->Flags[j] = 1;
                    Team_[i].AirUnits->Type[j]->SetFlagBitOff(C_BIT_INVISIBLE);
                }
                else
                {
                    Team_[i].AirUnits->Flags[j] = 0;
                    Team_[i].AirUnits->Type[j]->SetFlagBitOn(C_BIT_INVISIBLE);
                }
            }

            SetTeamFlags(i, GetTeamFlags(i) bitor _MAP_AIR_UNITS_);
        }

        for (j = 0; j < _MAP_NUM_AIR_TYPES_; j++)
            DrawWindow_->AddControl(Team_[i].AirUnits->Type[j]);
    }

    if ( not CurWPZ_)
    {
        CurWPZ_ = new C_Waypoint;
        CurWPZ_->Setup(5555000, C_TYPE_DRAGY);
        CurWPZ_->SetScaleType(1);
        CurWPZ_->SetWorldRange(LogMinX_, LogMinY_, LogMaxX_, LogMaxY_);
        CurWPZ_->SetFlagBitOn(C_BIT_DRAGABLE);
        CurWPZ_->SetCallback(WaypointCB);
        CurWPZ_->SetFont(Font);
        CurWPZ_->SetClient(1);
        CurWPZ_->SetFlagBitOn(C_BIT_TOP);
        CurWPZ_->SetCursorID(CRSR_STEERPOINT);
    }

    if (WPZWindow_)
        WPZWindow_->AddControl(CurWPZ_);
}

void C_Map::RemoveListsFromWindow()
{
    short i, j, k;

    if (DrawWindow_ == NULL) return;

    DrawWindow_->RemoveControl(Map_->GetID());

    if (BullsEye_)
        DrawWindow_->RemoveControl(BullsEye_->GetID());

    for (i = 0; i < _MAX_TEAMS_; i++)
    {
        if (Team_[i].Objectives)
            for (j = 0; j < _MAP_NUM_OBJ_TYPES_; j++)
                if (Team_[i].Objectives->Type[j])
                    DrawWindow_->RemoveControl(Team_[i].Objectives->Type[j]->GetID());

        if (Team_[i].NavalUnits)
            for (j = 0; j < _MAP_NUM_NAV_TYPES_; j++)
                if (Team_[i].NavalUnits->Type[j])
                    DrawWindow_->RemoveControl(Team_[i].NavalUnits->Type[j]->GetID());

        if (Team_[i].Units)
            for (j = 0; j < _MAP_NUM_GND_TYPES_; j++)
                if (Team_[i].Units->Type[j])
                    for (k = 0; k < _MAP_NUM_GND_LEVELS_; k++)
                        if (Team_[i].Units->Type[j]->Levels[k])
                            DrawWindow_->RemoveControl(Team_[i].Units->Type[j]->Levels[k]->GetID());

        if (Team_[i].AirUnits)
            for (j = 0; j < _MAP_NUM_AIR_TYPES_; j++)
                if (Team_[i].AirUnits->Type[j])
                    DrawWindow_->RemoveControl(Team_[i].AirUnits->Type[j]->GetID());

        if (Team_[i].Waypoints)
            DrawWindow_->RemoveControl(Team_[i].Waypoints->GetID());
    }

    if (CurIcons_ and DrawWindow_)
        DrawWindow_->RemoveControl(CurIcons_->GetID());

    if (CurWP_ and DrawWindow_)
        DrawWindow_->RemoveControl(CurWP_->GetID());

    if (CurWPZ_ and WPZWindow_)
        WPZWindow_->RemoveControl(CurWPZ_->GetID());
}

void C_Map::SetTeamScales()
{
    short i, j, k;

    if (BullsEye_)
        BullsEye_->SetScale(scale_);

    for (i = 0; i < _MAX_TEAMS_; i++)
    {
        if (Team_[i].Objectives)
        {
            for (j = 0; j < _MAP_NUM_OBJ_TYPES_; j++)
                Team_[i].Objectives->Type[j]->SetScaleFactor(scale_);
        }

        if (Team_[i].NavalUnits)
        {
            for (j = 0; j < _MAP_NUM_NAV_TYPES_; j++)
                Team_[i].NavalUnits->Type[j]->SetScaleFactor(scale_);
        }

        if (Team_[i].Units)
        {
            for (j = 0; j < _MAP_NUM_GND_TYPES_; j++)
                for (k = 0; k < _MAP_NUM_GND_LEVELS_; k++)
                    Team_[i].Units->Type[j]->Levels[k]->SetScaleFactor(scale_);
        }

        if (Team_[i].AirUnits)
        {
            for (j = 0; j < _MAP_NUM_AIR_TYPES_; j++)
                Team_[i].AirUnits->Type[j]->SetScaleFactor(scale_);
        }

        if (Team_[i].Waypoints)
            Team_[i].Waypoints->SetScaleFactor(scale_);
    }

    if (CurWP_)
        CurWP_->SetScaleFactor(scale_);
}

void C_Map::RecalcWaypointZs(long scaletype) // 1=Log, 2=straight
{
    if (scaletype == 1)
    {
        if (CurWPZ_)
        {
            CurWPZ_->SetWorldRange(LogMinX_, LogMinY_, LogMaxX_, LogMaxY_);
            CurWPZ_->SetScaleType(static_cast<short>(scaletype));
        }
    }
    else if (scaletype == 2)
    {
        if (CurWPZ_)
        {
            CurWPZ_->SetWorldRange(StrtMinX_, StrtMinY_, StrtMaxX_, StrtMaxY_);
            CurWPZ_->SetScaleType(static_cast<short>(scaletype));
        }
    }
}

void C_Map::DrawMap()
{
    short i, j, k;
    short x, y;

    if (flags_ bitand (I_NEED_TO_DRAW bitor I_NEED_TO_DRAW_MAP))
    {
        // 2002-04-16 MN update the bullseye location when it changed
        TheCampaign.GetBullseyeLocation(&x, &y);

        if (x not_eq BullsEyeX_ or y not_eq BullsEyeY_)
            SetBullsEye(x * FEET_PER_KM, (TheCampaign.TheaterSizeY - y) * FEET_PER_KM);

        if (flags_ bitand I_NEED_TO_DRAW_MAP)
        {

            if (Map_)
            {
                Map_->Refresh();
            }

            flags_ xor_eq I_NEED_TO_DRAW_MAP;
        }

        if (BullsEye_)
            BullsEye_->Refresh();

        for (i = 0; i < _MAX_TEAMS_; i++)
            if ((TeamFlags_[i] bitand _MAP_OBJECTIVES_) and Team_[i].Objectives)
            {
                for (j = 0; j < _MAP_NUM_OBJ_TYPES_; j++)
                {
                    if (Team_[i].Objectives->Flags[j])
                        Team_[i].Objectives->Type[j]->Refresh();
                }
            }

        for (i = 0; i < _MAX_TEAMS_; i++)
            if ((TeamFlags_[i] bitand _MAP_NAVAL_UNITS_) and Team_[i].NavalUnits)
            {
                for (j = 0; j < _MAP_NUM_NAV_TYPES_; j++)
                {
                    if (Team_[i].NavalUnits->Flags[j])
                        Team_[i].NavalUnits->Type[j]->Refresh();
                }
            }

        for (i = 0; i < _MAX_TEAMS_; i++)
            if ((TeamFlags_[i] bitand _MAP_UNITS_) and Team_[i].Units)
            {
                for (j = 0; j < _MAP_NUM_GND_TYPES_; j++)
                {
                    if (Team_[i].Units->Flags[j])
                    {
                        for (k = 0; k < _MAP_NUM_GND_LEVELS_; k++)
                        {
                            if (Team_[i].Units->Type[j]->Flags[k])
                                Team_[i].Units->Type[j]->Levels[k]->Refresh();
                        }
                    }
                }
            }

        for (i = 0; i < _MAX_TEAMS_; i++)
            if ((TeamFlags_[i] bitand _MAP_WAYPOINTS_) and Team_[i].Waypoints)
                Team_[i].Waypoints->Refresh();

        for (i = 0; i < _MAX_TEAMS_; i++)
            if ((TeamFlags_[i] bitand _MAP_AIR_UNITS_) and Team_[i].AirUnits)
            {
                for (j = 0; j < _MAP_NUM_AIR_TYPES_; j++)
                {
                    if (Team_[i].AirUnits->Flags[j])
                        Team_[i].AirUnits->Type[j]->Refresh();
                }
            }

        if (CurIcons_)
            CurIcons_->Refresh();

        if (CurWP_)
            CurWP_->Refresh();

        flags_ xor_eq I_NEED_TO_DRAW;
    }
}
