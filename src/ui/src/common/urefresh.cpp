#include <windows.h>
#include "vu2.h"
#include "falcsess.h"
#include "campbase.h"
#include "camplist.h"
#include "unit.h"
#include "squadron.h"
#include "flight.h"
#include "division.h"
#include "objectiv.h"
#include "chandler.h"
#include "ui95_ext.h"
#include "cmap.h"
#include "gps.h"
#include "urefresh.h"
#include "classtbl.h"
#include "userids.h"
#include "textids.h"

extern bool g_bAWACSSupport;
extern bool g_bAWACSFuel;

C_Mission *MakeMissionItem(C_TreeList *tree, Flight element);
void MissionUpdateStatus(Flight element, C_Mission *mission);
long GetFlightTime(Flight element);
short GetFlightStatusID(Flight element);
void MissionUpdateTime(Flight element, C_Mission *mission);
C_ATO_Package *AddPackagetoATO(Package pkg);
C_ATO_Flight *AddFlighttoATO(Flight flt);
C_Squadron *AddSquadronToAirbase(Squadron sqd);
C_Entity *AddAirbaseToAirbase(CampEntity Base);
C_Entity *AddDivisionToOOB(Division div);
C_Base *AddItemToOOB(CampEntity entity);
void MoveAirbaseSquadron(Squadron sqd, C_Squadron *Squadron);
void MoveOOBSquadron(Squadron sqd, C_Squadron *Squadron);

static _TCHAR buffer[200];

extern C_Map *gMapMgr;

extern int g_nUnidentifiedInUI; // 2002-02-24 S.G.
int gShowUnknown = 0; // 2002-02-21 S.G.
#define ICON_UKN 10126 // 2002-02-21 S.G.

UI_Refresher::UI_Refresher()
{
    ID_ = FalconNullId;
    DivID_ = 0;
    Type_ = GPS_NOTHING;
    Allowed_ = UR_NOTHING;
    Side_ = 0;
    Owner_ = NULL;
    Mission_ = NULL;
    MapItem_ = NULL;
    Package_ = NULL;
    ATO_ = NULL;
    OOB_ = NULL;
    Threat_ = NULL;
}

UI_Refresher::~UI_Refresher()
{
}

void UI_Refresher::Setup(CampEntity entity, GlobalPositioningSystem *own, long allow)
{
    Owner_ = own;

    if ( not entity)
        return;

    SetID(entity->Id());
    SetSide(entity->GetTeam());
    SetType(entity->GetType());
    SetCampID(entity->GetCampID());

    if (allow bitand UR_MISSION)
        AddMission(entity);

    if (allow bitand UR_MAP)
        AddMapItem(entity);

    if (allow bitand UR_ATO)
        AddATOItem(entity);

    if (allow bitand UR_OOB)
        AddOOBItem(entity);

    Allowed_ = static_cast<short>(allow);
}

void UI_Refresher::Setup(Division div, GlobalPositioningSystem *own, long allow)
{
    Owner_ = own;

    SetDivID(div->nid);
    SetType(GPS_DIVISION);
    SetSide(static_cast<uchar>(div->owner));

    if (allow bitand UR_MAP)
        AddMapItem(div);

    if (allow bitand UR_OOB)
        AddOOBItem(div);

    Allowed_ = static_cast<short>(allow);
}

void UI_Refresher::Cleanup()
{
    Remove();
}

void UI_Refresher::Update(CampEntity entity, long allow)
{
    if (entity->GetTeam() not_eq Side_)
    {
        // Handle ownership change
        Remove();
        Setup(entity, Owner_, allow);
        return;
    };

    if (Mission_)
        UpdateMission(entity);
    else if ((allow bitand UR_MISSION) and not (Allowed_ bitand UR_MISSION))
        AddMission(entity);
    else if (entity->IsFlight())
        AddMission(entity);

    if (MapItem_)
        UpdateMapItem(entity);
    else if ((allow bitand UR_MAP) and not (Allowed_ bitand UR_MAP))
        AddMapItem(entity);
    else if (entity->IsFlight())
        AddMapItem(entity);

    if (Allowed_ bitand UR_ATO)
        UpdateATOItem(entity);
    else if (allow bitand UR_ATO)
        AddATOItem(entity);

    if (OOB_)
        UpdateOOBItem(entity);
    else if ((allow bitand UR_OOB) and not (Allowed_ bitand UR_OOB))
        AddOOBItem(entity);

    Allowed_ or_eq allow;
}

void UI_Refresher::Update(Division div, long allow)
{
    if (MapItem_)
        UpdateMapItem(div);
    else if ((allow bitand UR_MAP) and not (Allowed_ bitand UR_MAP))
        AddMapItem(div);

    if (OOB_)
        UpdateOOBItem(div);
    else if ((allow bitand UR_OOB) and not (Allowed_ bitand UR_OOB))
        AddOOBItem(div);

    Allowed_ or_eq allow;
}

void UI_Refresher::Remove()
{
    if (Mission_)
        RemoveMission();

    if (MapItem_)
        RemoveMapItem();

    if (ATO_)
        RemoveATOItem();

    if (Package_)
        RemoveATOItem();

    if (OOB_)
        RemoveOOBItem();
}

void UI_Refresher::AddMission(CampEntity entity)
{
    if (entity->IsFlight() and ((FlightClass*)entity)->Final())
    {
        if (((FlightClass*)entity)->GetUnitMission() not_eq AMIS_ALERT)
        {
            VehicleClassDataType* vc;
            Falcon4EntityClassType* classPtr;
            int vid;

            // KLUDGE... Make sure it's an F16
            // KCK LOOK HERE: Please make this work better
            vid = ((FlightClass*)entity)->GetVehicleID(0);
            vc = GetVehicleClassData(vid);
            classPtr = &Falcon4ClassTable[vid];

            // OW Fly any plane fix
#if 0

            if (classPtr->visType[0] == VIS_F16C)
#endif
            {
                Mission_ = MakeMissionItem(Owner_->MisTree_, (Flight)entity);

                if (Mission_)
                    Mission_->SetMenu(Owner_->MissionMenu_);

                Owner_->SetFlags(Owner_->GetFlags() bitor _GPS_MISSION_RESIZE_);
            }
        }
    }
}

void UI_Refresher::UpdateMission(CampEntity entity)
{
    if ( not entity->IsDead())
    {
        if (GetFlightTime((Flight)entity) not_eq Mission_->GetTakeOffTime())
        {
            MissionUpdateTime((Flight)entity, Mission_);
            Owner_->SetFlags(Owner_->GetFlags() bitor _GPS_RESORT_MISSION_);
        }

        if (GetFlightStatusID((Flight)entity) not_eq Mission_->GetStatusID())
            MissionUpdateStatus((Flight)entity, Mission_);

        if (((Flight)entity)->Final() and ((Flight)entity)->GetUnitMission() not_eq AMIS_ALERT)
            Mission_->SetFlagBitOff(C_BIT_INVISIBLE);
    }
    else
        Mission_->SetFlagBitOn(C_BIT_INVISIBLE);
}

void UI_Refresher::RemoveMission()
{
    long ID;
    ID = Mission_->GetOwner()->ID_;
    Owner_->MisTree_->DeleteItem(Mission_->GetOwner());
    ShiAssert( not Owner_->MisTree_->Find(ID));
    Mission_ = NULL;
    Owner_->SetFlags(Owner_->GetFlags() bitor _GPS_MISSION_RESIZE_);
}

void UI_Refresher::AddMapItem(CampEntity entity)
{
    WayPoint wp;

    if (entity->IsFlight() and ((Flight)entity)->Final() and not entity->IsDead())
    {
        MapItem_ = Owner_->Map_->AddFlight((Flight)entity);
        wp = ((Flight)entity)->GetFirstUnitWP();

        if (wp and MapItem_)
        {
            if (vuxGameTime < wp->GetWPDepartureTime())
                MapItem_->Flags and_eq compl C_BIT_ENABLED;
            else
                MapItem_->Flags or_eq C_BIT_ENABLED;
        }
        else if (MapItem_)
            MapItem_->Flags and_eq compl C_BIT_ENABLED;
    }
    else if (entity->IsSquadron())
    {
        MapItem_ = Owner_->Map_->AddSquadron((Squadron)entity);
    }
    else if (entity->IsPackage())
    {
        MapItem_ = Owner_->Map_->AddPackage((Package)entity);
    }
    else if (entity->IsBattalion())
    {
        MapItem_ = Owner_->Map_->AddUnit((Unit)entity);

        if (MapItem_ and entity->GetTeam() not_eq Owner_->TeamNo_)
            Threat_ = Owner_->Map_->AddThreat(entity);
    }
    else if (entity->IsBrigade())
        MapItem_ = Owner_->Map_->AddUnit((Unit)entity);
    else if (entity->IsTaskForce())
    {
        MapItem_ = Owner_->Map_->AddUnit((Unit)entity);
    }
    else if (entity->IsObjective())
    {
        MapItem_ = Owner_->Map_->AddObjective((Objective)entity);

        if (MapItem_ and entity->GetTeam() not_eq Owner_->TeamNo_)
            Threat_ = Owner_->Map_->AddThreat(entity);
    }

    if (MapItem_ and entity->IsUnit())
    {
        if (TheCampaign.Flags bitand CAMP_TACTICAL_EDIT)
        {
            MapItem_->Flags or_eq C_BIT_DRAGABLE;
        }

        if (entity->IsUnit() and ((Unit)entity)->Inactive())
        {
            MapItem_->Flags or_eq C_BIT_INVISIBLE;
        }

        if (entity->GetTeam() not_eq Owner_->TeamNo_ and Owner_->TeamNo_ >= 0 and not entity->IsSquadron())
        {
            if ( not entity->GetSpotted(static_cast<uchar>(Owner_->TeamNo_)) and entity->GetMovementType() not_eq NoMove)
            {
                MapItem_->Flags or_eq C_BIT_INVISIBLE;

                if (Threat_)
                {
                    if (Threat_->SamLow)
                        Threat_->SamLow->Flags or_eq C_BIT_INVISIBLE;

                    if (Threat_->SamHigh)
                        Threat_->SamHigh->Flags or_eq C_BIT_INVISIBLE;

                    if (Threat_->RadarLow)
                        Threat_->RadarLow->Flags or_eq C_BIT_INVISIBLE;

                    if (Threat_->RadarHigh)
                        Threat_->RadarHigh->Flags or_eq C_BIT_INVISIBLE;
                }
            }
        }
    }
}

void UI_Refresher::AddMapItem(Division div)
{
    MapItem_ = Owner_->Map_->AddDivision(div);
}

void UI_Refresher::UpdateMapItem(CampEntity entity)
{
    long curstr, totalstr;
    long perc = 0, heading = 0;
    WayPoint wp;

    if (entity->IsUnit())
    {
        curstr = ((Unit)entity)->GetTotalVehicles();
        totalstr = ((Unit)entity)->GetFullstrengthVehicles();

        if (totalstr < 1) totalstr = 1;

        perc = (curstr * 100) / totalstr;

        if (perc > 100) perc = 100;

        if (entity->IsFlight())
        {
            if ( not entity->IsDead())
            {
                // 2002-02-21 ADDED BY S.G. 'Fog of war code'. If an enemy flight and not identified
                //and not editing a TE, change the IconIndex of the unit to ICON_UKN
                //and its label to 'Bandit'
                if (g_nUnidentifiedInUI and ( not (TheCampaign.Flags bitand CAMP_TACTICAL_EDIT) and MapItem_ and entity->GetTeam() not_eq Owner_->TeamNo_))
                {
                    if (entity->GetIdentified(static_cast<uchar>(Owner_->TeamNo_)))
                    {
                        ((MAPICONLIST *)MapItem_)->ImageID = ((UnitClass *)entity)->GetUnitClassData()->IconIndex;

                        if ( not g_bAWACSSupport)
                        {
                            int vid = ((Unit)entity)->GetVehicleID(0);
                            VehicleClassDataType *vc = GetVehicleClassData(vid);
                            ((MAPICONLIST *)MapItem_)->Label->SetText(gStringMgr->GetText(gStringMgr->AddText(vc ? vc->Name : "<unk>")));
                        }
                    }
                    else
                    {
                        ((MAPICONLIST *)MapItem_)->ImageID = ICON_UKN;

                        if ( not g_bAWACSSupport)
                            ((MAPICONLIST *)MapItem_)->Label->SetText(gStringMgr->GetText(gStringMgr->AddText("Bandit")));
                    }
                }

                // END OF ADDED SECTION 2002-02-21

                // KCK: Convert yaw to heading 0-7
                heading = ((long)(((entity->Yaw() * RTD) + 360.0F + 22.5F) / 45.0F)) % 8;

                if (g_bAWACSSupport)
                {
                    // ****** Marco/Julian Edit

                    char name[40] ;
                    char buffer[256] ;
                    int bearing, range ;
                    float x, y, z ;
                    extern GlobalPositioningSystem *gGps;


                    // Get pointer to flight entity
                    Flight flt = (Flight)entity ;

                    // Grab callsign of flight (eg Cowboy 1-1)
                    if (gGps->GetTeamNo() == entity->GetTeam())
                        GetCallsign(flt, name);
                    // 2002-02-24 ADDED BY S.G. Don't give AWACS more info than it can get...
                    else if (g_nUnidentifiedInUI and ( not (TheCampaign.Flags bitand CAMP_TACTICAL_EDIT) and MapItem_ and entity->IsFlight() and entity->GetTeam() not_eq Owner_->TeamNo_ and not entity->GetIdentified(static_cast<uchar>(Owner_->TeamNo_))))
                        strcpy(name, "Bandit");
                    else
                    {
                        int vid = ((Unit)entity)->GetVehicleID(0);
                        VehicleClassDataType *vc = GetVehicleClassData(vid);
                        strcpy(name, vc ? vc->Name : "<unk>");
                    }

                    // Write out to buffer what we want displayed
                    // Callsign, number, bullseye position, altitude

                    // Bullseye stuff
                    flt->GetRealPosition(&x, &y, &z) ;
                    // Get Bearing/Distance from Bullseye
                    bearing = 180 + TheCampaign.BearingToBullseyeDeg(x, y) ;
                    range = (int)(TheCampaign.RangeToBullseyeFt(x, y) * FT_TO_NM);

                    if (g_bAWACSFuel)
                        sprintf(buffer, "%d*%s \n%03dx%d \nAlt %d \nSpd %d \nFuel %lu",
                                flt->GetACCount(),  name, bearing, range,
                                (int)(-flt->ZPos() / 1000.0), (int)flt->GetKias(), (long)flt->CalculateFuelAvailable(255));
                    else
                        sprintf(buffer, "%d*%s \n%03dx%d \nAlt %d \nSpd %d",
                                // sprintf (buffer, "%d*%s@%03dx%d/A%d/V%d",
                                flt->GetACCount(),  name, bearing, range,
                                (int)(-flt->ZPos() / 1000.0), (int)flt->GetKias());

                    // 2002-02-24 ADDED BY S.G. Needs to set a logical line lenght and wrap it
                    MapItem_->Label->SetWordWrapWidth(72) ; // Avg for 12 chars
                    MapItem_->Label->SetFlags(MapItem_->Label->GetFlags() bitor C_BIT_WORDWRAP) ;  // Add the 'word wrap flag to what's there already
                    // END OF ADDED SECTION 2002-02-24

                    MapItem_->Label->SetText(gStringMgr->GetText(gStringMgr->AddText(buffer))) ;
                    // ****** Marco Edit End
                }

                // JPO end
                if ( not (MapItem_->Flags bitand C_BIT_ENABLED))
                {
                    wp = ((Flight)entity)->GetFirstUnitWP();

                    if (wp)
                        if (vuxGameTime >= wp->GetWPDepartureTime())
                            MapItem_->Flags or_eq C_BIT_ENABLED;
                }
            }
            else
                MapItem_->Flags and_eq compl C_BIT_ENABLED;
        }
    }
    else if (entity->IsObjective())
    {
        perc = ((Objective)entity)->GetObjectiveStatus();

        if (perc > 100) perc = 100;
    }

    // sfr: entities are stored in sim coordinates which are xy inverted
    float x = entity->YPos();
    float y = Owner_->Map_->GetMaxY() - entity->XPos();

    if (MapItem_->Owner->UpdateInfo(MapItem_, x, y, perc, heading))
    {
        Owner_->SetFlags(Owner_->GetFlags() bitor _GPS_MAP_REFRESH_);
    }

    // 2001-05-09 COMMENTED OUT BY S.G. X AND Y ARE ALREADY FINE AND THIS SCREWS THEM UP
    /* if(Threat_)
     {
     if(Threat_->SamLow)
     {
     Threat_->SamLow->x=static_cast<long>(entity->YPos());
     Threat_->SamLow->y=static_cast<long>(Owner_->Map_->GetMaxY()-entity->XPos());
     }
     if(Threat_->SamHigh)
     {
     Threat_->SamHigh->x=static_cast<long>(entity->YPos());
     Threat_->SamHigh->y=static_cast<long>(Owner_->Map_->GetMaxY()-entity->XPos());
     }
     if(Threat_->RadarLow)
     {
     Threat_->RadarLow->x=static_cast<long>(entity->YPos());
     Threat_->RadarLow->y=static_cast<long>(Owner_->Map_->GetMaxY()-entity->XPos());
     }
     if(Threat_->RadarHigh)
     {
     Threat_->RadarHigh->x=static_cast<long>(entity->YPos());
     Threat_->RadarHigh->y=static_cast<long>(Owner_->Map_->GetMaxY()-entity->XPos());
     }
     }
    */
    if (MapItem_ and entity->IsUnit() and entity->GetTeam() not_eq Owner_->TeamNo_)
    {
        if (Owner_->TeamNo_ < 0)
        {
            MapItem_->Flags and_eq compl C_BIT_INVISIBLE;

            if (Threat_)
            {
                if (Threat_->SamLow)
                    Threat_->SamLow->Flags and_eq compl C_BIT_INVISIBLE;

                if (Threat_->SamHigh)
                    Threat_->SamHigh->Flags and_eq compl C_BIT_INVISIBLE;

                if (Threat_->RadarLow)
                    Threat_->RadarLow->Flags and_eq compl C_BIT_INVISIBLE;

                if (Threat_->RadarHigh)
                    Threat_->RadarHigh->Flags and_eq compl C_BIT_INVISIBLE;
            }
        }
        else
        {
            if ( not entity->GetSpotted(static_cast<uchar>(Owner_->TeamNo_)) and entity->GetMovementType() not_eq NoMove)
            {
                MapItem_->Flags or_eq C_BIT_INVISIBLE;

                if (Threat_)
                {
                    if (Threat_->SamLow)
                        Threat_->SamLow->Flags or_eq C_BIT_INVISIBLE;

                    if (Threat_->SamHigh)
                        Threat_->SamHigh->Flags or_eq C_BIT_INVISIBLE;

                    if (Threat_->RadarLow)
                        Threat_->RadarLow->Flags or_eq C_BIT_INVISIBLE;

                    if (Threat_->RadarHigh)
                        Threat_->RadarHigh->Flags or_eq C_BIT_INVISIBLE;
                }
            }
            else
            {
                MapItem_->Flags and_eq compl C_BIT_INVISIBLE;

                if (Threat_)
                {
                    if (Threat_->SamLow)
                        Threat_->SamLow->Flags and_eq compl C_BIT_INVISIBLE;

                    if (Threat_->SamHigh)
                        Threat_->SamHigh->Flags and_eq compl C_BIT_INVISIBLE;

                    if (Threat_->RadarLow)
                        Threat_->RadarLow->Flags and_eq compl C_BIT_INVISIBLE;

                    if (Threat_->RadarHigh)
                        Threat_->RadarHigh->Flags and_eq compl C_BIT_INVISIBLE;
                }
            }
        }

        if (entity->IsUnit() and ((Unit)entity)->Inactive())
        {
            MapItem_->Flags or_eq C_BIT_INVISIBLE;
        }

        // 2002-02-21 ADDED BY S.G. 'Fog of war code'. If an enemy flight and not identified, not editing a TE and 'showUnknown' isn't set, hide it
        if (g_nUnidentifiedInUI and not gShowUnknown and MapItem_->ImageID == ICON_UKN)
            MapItem_->Flags or_eq C_BIT_INVISIBLE;

        // END OF ADDED SECTION 2002-02-21
    }
}

void UI_Refresher::UpdateMapItem(Division)
{
}

void UI_Refresher::RemoveMapItem()
{
    if (gMapMgr)
        gMapMgr->RemoveFromCurIcons(MapItem_->ID);

    MapItem_->Owner->RemoveIcon(MapItem_->ID);
    MapItem_ = NULL;

    if (Threat_)
    {
        if (Threat_->SamLow)
        {
            Threat_->SamLow->Owner->Remove(Threat_->SamLow->ID);
        }

        if (Threat_->SamHigh)
        {
            Threat_->SamHigh->Owner->Remove(Threat_->SamHigh->ID);
        }

        if (Threat_->RadarLow)
        {
            Threat_->RadarLow->Owner->Remove(Threat_->RadarLow->ID);
        }

        if (Threat_->RadarHigh)
        {
            Threat_->RadarHigh->Owner->Remove(Threat_->RadarHigh->ID);
        }

        delete Threat_;
        Threat_ = NULL;
    }
}

void UI_Refresher::AddATOItem(CampEntity entity)
{
    if (entity->IsPackage() and ((Package)entity)->Final())
    {
        Package_ = AddPackagetoATO((Package)entity);

        if (Package_)
            Package_->SetMenu(PACKAGE_POP);

        Owner_->SetFlags(Owner_->GetFlags() bitor _GPS_ATO_RESIZE_);
    }
    else if (entity->IsFlight() and ((Flight)entity)->Final())
    {
        ATO_ = AddFlighttoATO((Flight)entity);

        if (ATO_)
            ATO_->SetMenu(AIRUNIT_MENU);

        Owner_->SetFlags(Owner_->GetFlags() bitor _GPS_ATO_RESIZE_);
    }
}

void UI_Refresher::UpdateATOItem(CampEntity entity)
{
    if (Package_)
    {
    }
    else if (entity->IsPackage() and ((Package)entity)->Final())
    {
        Package_ = AddPackagetoATO((Package)entity);

        if (Package_)
            Package_->SetMenu(PACKAGE_POP);

        Owner_->SetFlags(Owner_->GetFlags() bitor _GPS_ATO_RESIZE_);
    }

    if (ATO_)
    {
        // Status changes...
    }
    else if (entity->IsFlight() and ((Flight)entity)->Final())
    {
        ATO_ = AddFlighttoATO((Flight)entity);

        if (ATO_)
            ATO_->SetMenu(AIRUNIT_MENU);

        Owner_->SetFlags(Owner_->GetFlags() bitor _GPS_ATO_RESIZE_);
    }
}

void UI_Refresher::RemoveATOItem()
{
    if (gMapMgr)
    {
        gMapMgr->RemoveWaypoints(Side_, CampID_ << 8);

        if (gMapMgr->GetCurWPID() == ID_)
            gMapMgr->RemoveCurWPList();
    }

    if (ATO_)
        Owner_->AtoTree_->DeleteItem(ATO_->GetOwner());

    if (Package_)
        Owner_->AtoTree_->DeleteItem(Package_->GetOwner());

    Package_ = NULL;
    ATO_ = NULL;
    Owner_->SetFlags(Owner_->GetFlags() bitor _GPS_ATO_RESIZE_);
}

void UI_Refresher::AddOOBItem(CampEntity entity)
{
    if (entity->IsObjective())
    {
        OOB_ = AddItemToOOB(entity);

        if (OOB_)
            OOB_->SetMenu(Owner_->ObjectiveMenu_);
    }
    else if (entity->IsSquadron())
    {
        OOB_ = AddItemToOOB(entity);

        if (OOB_)
            OOB_->SetMenu(Owner_->SquadronMenu_);
    }
    else if (entity->IsTaskForce())
    {
        OOB_ = AddItemToOOB(entity);

        if (OOB_)
            OOB_->SetMenu(Owner_->UnitMenu_);
    }
    else if (entity->IsBrigade() or entity->IsBattalion())
    {
        OOB_ = AddItemToOOB(entity);

        if (OOB_)
            OOB_->SetMenu(Owner_->UnitMenu_);
    }

    Owner_->SetFlags(Owner_->GetFlags() bitor _GPS_OOB_RESIZE_);
}

void UI_Refresher::AddOOBItem(Division div)
{
    OOB_ = AddDivisionToOOB(div);

    if (OOB_)
        OOB_->SetMenu(Owner_->UnitMenu_);

    Owner_->SetFlags(Owner_->GetFlags() bitor _GPS_OOB_RESIZE_);
}

void UI_Refresher::UpdateOOBItem(CampEntity entity)
{
    if (entity->IsSquadron() or entity->IsTaskForce() or entity->IsBrigade() or entity->IsBattalion())
    {
    }
    else if (entity->IsObjective())
    {
    }
}

void UI_Refresher::UpdateOOBItem(Division)
{
}

void UI_Refresher::RemoveOOBItem()
{
    if (OOB_->_GetCType_() == _CNTL_ENTITY_)
        Owner_->OOBTree_->DeleteItem(((C_Entity*)OOB_)->GetOwner());
    else if (OOB_->_GetCType_() == _CNTL_SQUAD_)
        Owner_->OOBTree_->DeleteItem(((C_Squadron*)OOB_)->GetOwner());

    OOB_ = NULL;
    Owner_->SetFlags(Owner_->GetFlags() bitor _GPS_OOB_RESIZE_);
}
