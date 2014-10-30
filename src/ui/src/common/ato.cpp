// ATO Stuff Here
#include <windows.h>
#include "unit.h"
#include "campwp.h"
#include "campstr.h"
#include "squadron.h"
#include "flight.h"
#include "team.h"
#include "find.h"
#include "division.h"
#include "misseval.h"
#include "camplist.h"
#include "chandler.h"
#include "ui95_ext.h"
#include "uicomms.h"
#include "cmap.h"
#include "userids.h"
#include "textids.h"
#include "classtbl.h"

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

enum
{
    BID_DROPDOWN = 50300,
    BID_CHK1_OFF = 50301,
    BID_CHK1_ON = 50302,
};

void AddLocationToBrief(char type, GridIndex x, GridIndex y, _TCHAR *brief);
void GetMissionTarget(Package curpackage, Flight curflight, _TCHAR Buffer[]);
int IsValidMission(int dindex, int mission);
void SetupFlightSpecificControls(Flight flt);

extern C_Handler *gMainHandler;
//extern LISTBOX *gTaskList;

extern C_Map *gMapMgr;

C_TreeList *gATOAll = NULL;
C_TreeList *gATOPackage = NULL;

VU_ID gSelectedPackage = FalconNullId;
VU_ID gSelectedFlightID = FalconNullId;
VU_ID gSelectedATOFlight = FalconNullId; // Unselected when Package is selected
extern VU_ID gCurrentFlightID;

void ShowPackageWPs(VU_ID package)
{
    Package pkg;
    Flight flt;

    pkg = (Package)vuDatabase->Find(package);

    if (pkg and pkg->Final())
    {
        flt = (Flight)pkg->GetFirstUnitElement();

        while (flt)
        {
            gMapMgr->SetWaypointList(flt->Id());
            flt = (Flight)pkg->GetNextUnitElement();
        }
    }
}

void HidePackageWPs(VU_ID package)
{
    Package pkg;
    Flight flt;

    pkg = (Package)vuDatabase->Find(package);

    if (pkg and pkg->Final())
    {
        flt = (Flight)pkg->GetFirstUnitElement();

        while (flt)
        {
            gMapMgr->RemoveWaypoints(flt->GetTeam(), flt->GetCampID() << 8);
            flt = (Flight)pkg->GetNextUnitElement();
        }
    }
}

void UpdateTeamName(long team)
{
    TREELIST *item;
    C_Text *txt;

    if ( not gATOAll or team >= NUM_TEAMS or not TeamInfo[team])
        return;

    item = gATOAll->Find(team bitor 0x20000000);

    if (item)
    {
        txt = (C_Text*)item->Item_;

        if (txt)
        {
            txt->Refresh();
            txt->SetText(TeamInfo[team]->GetName());
            txt->Refresh();
        }
    }
}

void SelectATOItemCB(long, short hittype, C_Base *control)
{
    TREELIST *item;
    C_ATO_Flight *ato;
    C_ATO_Package *pkg;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    ShiAssert(gATOAll);

    if (gATOAll)
        gATOAll->SetAllControlStates(0, gATOAll->GetRoot());

    ato = (C_ATO_Flight*)control;
    ato->SetState(1);
    ato->Refresh();

    if (ato->GetOwner())
    {
        item = ato->GetOwner()->Parent;

        if (item and item->Item_)
        {
            pkg = (C_ATO_Package*)item->Item_;
            pkg->SetState(1);
            pkg->Refresh();
        }
    }

    gSelectedFlightID = ato->GetVUID();
    gSelectedATOFlight = gSelectedFlightID;

    if (TheCampaign.Flags bitand CAMP_TACTICAL_EDIT)
    {
        gCurrentFlightID = gSelectedFlightID;

        if (gMapMgr)
        {
            gMapMgr->SetCurrentWaypointList(gCurrentFlightID);
            SetupFlightSpecificControls((Flight)vuDatabase->Find(gCurrentFlightID));
            gMapMgr->FitFlightPlan();
            gMapMgr->DrawMap();
        }
    }
}

void SelectATOPackageCB(long, short hittype, C_Base *control)
{
    C_ATO_Package *package;
    VU_ID *tmpID;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    package = (C_ATO_Package*)control;
    package->SetState(1);
    package->Refresh();

    gSelectedATOFlight = FalconNullId;

    if (control->_GetCType_() == _CNTL_PACKAGE_)
        gSelectedPackage = package->GetVUID();
    else
    {
        tmpID = (VU_ID*)control->GetUserPtr(_UI95_VU_ID_SLOT_);

        if (tmpID)
            gSelectedPackage = *tmpID;
        else
            gSelectedPackage = FalconNullId;
    }

    if (package->GetWPState())
    {
        // Add a bunch of waypoints to the map
        ShowPackageWPs(gSelectedPackage);
    }
    else
    {
        // Remove a bunch of waypoints from the map
        HidePackageWPs(gSelectedPackage);
    }
}

void RedrawTreeWindowCB(long, short, C_Base *control)
{
    if (control->Parent_)
        control->Parent_->RefreshClient(control->GetClient());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int MissionToATOMiss(int mistype)
{
    switch (mistype)
    {
        case AMIS_OCASTRIKE:
        case AMIS_SWEEP:
        case AMIS_TARCAP:
        case AMIS_ESCORT:
            return ATO_OCA;
            break;

        case AMIS_STRIKE:
        case AMIS_DEEPSTRIKE:
        case AMIS_STSTRIKE:
        case AMIS_STRATBOMB:
            return ATO_STRIKE;
            break;

        case AMIS_INTSTRIKE:
        case AMIS_INT:
        case AMIS_SAD:
        case AMIS_BAI:
            return ATO_INTERDICTION;
            break;

        case AMIS_SEADSTRIKE:
        case AMIS_SEADESCORT:
            return ATO_SEAD;
            break;

        case AMIS_PRPLANCAS:
        case AMIS_CAS:
        case AMIS_ONCALLCAS:
        case AMIS_FAC:
            return ATO_CAS;
            break;

        case AMIS_BARCAP:
        case AMIS_BARCAP2:
        case AMIS_HAVCAP:
        case AMIS_AMBUSHCAP:
        case AMIS_INTERCEPT:
        case AMIS_ALERT:
            return ATO_DCA;
            break;

        case AMIS_AWACS:
        case AMIS_JSTAR:
        case AMIS_ECM:
        case AMIS_RECON:
        case AMIS_BDA:
        case AMIS_RECONPATROL:
        case AMIS_PATROL:
            return ATO_CCCI;
            break;

        case AMIS_ASW:
        case AMIS_ASHIP:
            return ATO_MARITIME;
            break;

        case AMIS_TANKER:
        case AMIS_AIRLIFT:
        case AMIS_SAR:
        case AMIS_RESCAP:
            return ATO_SUPPORT;
            break;

        default:
            return ATO_OTHER;
            break;
    }
}

void ChangeFlightTypeCB(long, short hittype, C_Base *control)
{
    C_ListBox
    *listbox;

    Flight flight;
    int camp_id;
    uchar type;

    if (hittype not_eq C_TYPE_SELECT)
    {
        return;
    }

    listbox = (C_ListBox *) control;

    type = static_cast<uchar>(listbox->GetTextID());

    camp_id = listbox->GetID();

    flight = (Flight) GetEntityByCampID(camp_id);

    if (flight and not F4IsBadReadPtr(flight, sizeof(Flight))) // JB 010326 CTD
        flight->SetUnitMission(type);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

C_ATO_Flight *BuildATOFlightInfo(Flight fl)
{
    C_ATO_Flight *newinfo;
    C_ListBox *lbox;
    _TCHAR buffer[200];
    _TCHAR cmpbuf[200];
    CampEntity airbase;
    Unit squadron;
    UnitClassDataType *UnitPtr;
    int mission;

    UnitPtr = fl->GetUnitClassData();

    if (UnitPtr == NULL)
        return(NULL);

    // Create new parent class
    newinfo = new C_ATO_Flight;
    newinfo->Setup(fl->GetCampID(), 0);
    newinfo->SetWH(286, 52);
    newinfo->InitFlight(gMainHandler);
    newinfo->SetVUID(fl->Id());
    newinfo->SetUserNumber(0, fl->GetTeam());

    // Set BG Area behind Airplane Icon
    newinfo->SetIconBgColor(0x7c5600, 0);
    newinfo->SetIconBg(0, 0, 46, 50);
    // Set BG Area behind rest of ATO text
    newinfo->SetFlightBgColor(0, 0x7c5600);
    newinfo->SetFlightBg(46, 0, 240, 50);
    // Set Icon Image
    newinfo->SetIcon(23, 25, UnitPtr->IconIndex);
    // Select current Task type in List
    newinfo->SetCurrentTask(fl->GetUnitMission());

    // Set TaskList List
    lbox = newinfo->GetTaskCtrl();
    lbox->RemoveAllItems();
    newinfo->SetTask(49, 5, NULL);
    mission = fl->GetUnitMission();

    if (TheCampaign.Flags bitand CAMP_TACTICAL_EDIT)
    {
        // Setup list box with only valid roles
        int i, added;

        for (i = 1, added = 0; i < AMIS_OTHER; i++)
        {
            if (IsValidMission(fl->Type() - VU_LAST_ENTITY_TYPE, i))
            {
                lbox->AddItem(i, C_TYPE_ITEM, MissStr[i]);

                if ( not added or i == mission)
                {
                    lbox->SetValue(i);
                    added = 1;
                }
            }
        }

        lbox->SetFlagBitOn(C_BIT_ENABLED);
        lbox->SetCallback(ChangeFlightTypeCB);
    }
    else
    {

        lbox->AddItem(mission, C_TYPE_ITEM, MissStr[mission]);
        lbox->SetValue(mission);
        lbox->SetFlagBitOff(C_BIT_ENABLED);
        lbox->SetCallback(NULL);
    }

    // Set Callsign
    GetCallsign(fl, buffer);
    newinfo->SetCallsign(190, 5, gStringMgr->GetText(gStringMgr->AddText(buffer)));

    // Set # bitand Airplane type, Squadron
    squadron = fl->GetUnitSquadron();

    if (squadron)
        squadron->GetName(cmpbuf, 40, FALSE);
    else
        _tcscpy(cmpbuf, "Kevin, Which squadron is this?");

    _stprintf(buffer, "%1d %s  \"%s\"", fl->GetTotalVehicles(), GetVehicleName(fl->GetVehicleID(0)), cmpbuf);
    newinfo->SetPlanes(54, 20, gStringMgr->GetText(gStringMgr->AddText(buffer)));

    // Set Starting Airbase
    if (squadron)
        airbase = squadron->GetUnitAirbase();
    else
        airbase = fl->GetUnitAirbase();

    if (airbase)
        airbase->GetName(buffer, 40, FALSE);
    else
        _tcscpy(buffer, "Kevin, what airbase am I at?");

    newinfo->SetAirbase(54, 35, gStringMgr->GetText(gStringMgr->AddText(buffer)));

    return(newinfo);
}

C_ATO_Package *BuildATOPackageInfo(Package pkg)
{
    C_ATO_Package *newinfo;
    _TCHAR buffer[200];
    int mistype;

    // Create new parent class
    newinfo = new C_ATO_Package;
    newinfo->Setup(pkg->GetCampID(), 0);
    newinfo->SetWH(300, 22);
    newinfo->InitPackage(gMainHandler);
    newinfo->SetVUID(pkg->Id());
    newinfo->SetUserNumber(0, pkg->GetTeam());

    if (pkg->GetFirstUnitElement())
    {
        mistype = pkg->GetFirstUnitElement()->GetUnitMission();
        _stprintf(buffer, "%s %1d - %s", gStringMgr->GetString(TXT_PACKAGE), pkg->GetCampID(), MissStr[mistype]);
        newinfo->SetTitle(0, 0, gStringMgr->GetText(gStringMgr->AddText(buffer)));
    }
    else
    {
        _tcscpy(buffer, "Error: Package doesn't have a flight");
        newinfo->SetTitle(0, 0, gStringMgr->GetText(gStringMgr->AddText(buffer)));
    }

    newinfo->SetCheckBox(280, 0, BID_CHK1_OFF, BID_CHK1_ON);
    return(newinfo);
}

void ToggleATOInfoCB(long, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    if (((C_Button *)control)->GetState())
    {
        control->Parent_->HideCluster(control->GetUserNumber(C_STATE_0));
        control->Parent_->UnHideCluster(control->GetUserNumber(C_STATE_1));

        ShiAssert(gATOAll);

        if (gATOAll)
        {
            gATOAll->RecalcSize();
            control->Parent_->ScanClientArea(gATOAll->GetClient());
            control->Parent_->RefreshClient(gATOAll->GetClient());
        }
    }
    else
    {
        control->Parent_->HideCluster(control->GetUserNumber(C_STATE_1));
        control->Parent_->UnHideCluster(control->GetUserNumber(C_STATE_0));

        ShiAssert(gATOPackage);

        if (gATOPackage)
        {
            gATOPackage->RecalcSize();
            control->Parent_->ScanClientArea(gATOPackage->GetClient());
            control->Parent_->RefreshClient(gATOPackage->GetClient());
        }
    }
}

void MakeIndividualATO(VU_ID flightID)
{
    Flight flt;
    Package pkg;
    C_ATO_Flight *atoitem;
    TREELIST *item;
    F4CSECTIONHANDLE *Leave;

    ShiAssert(gATOPackage);

    if ( not gATOPackage)
        return;

    Leave = UI_Enter(gATOPackage->GetParent());
    gATOPackage->DeleteBranch(gATOPackage->GetRoot());

    flt = (Flight)vuDatabase->Find(flightID);

    if ( not flt)
    {
        UI_Leave(Leave);
        return;
    }

    pkg = (Package)flt->GetUnitParent();

    if ( not pkg)
    {
        UI_Leave(Leave);
        return;
    }

    flt = (Flight)pkg->GetFirstUnitElement();

    if (flt)
    {
        // TE:Add Flight type, BOTH:add target as info items to root
    }

    while (flt)
    {
        atoitem = BuildATOFlightInfo(flt);

        if (atoitem)
        {
            atoitem->SetMenu(UNIT_POP);
            atoitem->SetCallback(SelectATOItemCB);
            atoitem->SetFont(gATOPackage->GetFont());
            item = gATOPackage->CreateItem(atoitem->GetID(), C_TYPE_ITEM, atoitem);
            atoitem->SetOwner(item);
            gATOPackage->AddItem(gATOPackage->GetRoot(), item);
        }

        flt = (Flight)pkg->GetNextUnitElement();
    }

    gATOPackage->RecalcSize();

    if (gATOPackage->GetParent())
        gATOPackage->GetParent()->RefreshClient(gATOPackage->GetClient());

    UI_Leave(Leave);
}

































C_ATO_Package *AddPackagetoATO(Package FltPkg)
{
    C_ATO_Package *atopkg;
    Flight MainFlt;
    TREELIST *team;
    TREELIST *missiontype;
    TREELIST *package;
    MissionTypeEnum mistype;
    short AtoMiss;
    C_Text *txt;

    if ( not FltPkg)
        return(NULL);

    ShiAssert(gATOAll);

    if ( not gATOAll)
        return NULL;

    if ( not (TheCampaign.Flags bitand CAMP_TACTICAL_EDIT))
    {
        if (FltPkg->GetTeam() not_eq FalconLocalSession->GetTeam())
            return(NULL);
    }

    MainFlt = (Flight)FltPkg->GetFirstUnitElement();

    if ( not MainFlt)
        return(NULL);

    mistype = MainFlt->GetUnitMission();

    if (mistype == AMIS_ABORT or mistype == AMIS_ALERT)
        return(NULL);

    if (TheCampaign.Flags bitand CAMP_TACTICAL_EDIT)
    {
        team = gATOAll->Find(FltPkg->GetTeam() bitor 0x20000000);

        if ( not team)
        {
            txt = new C_Text;
            txt->Setup(FltPkg->GetTeam() bitor 0x20000000, 0);
            txt->SetText(TeamInfo[FltPkg->GetTeam()]->GetName());
            txt->SetFont(gATOAll->GetParent()->Font_);
            team = gATOAll->CreateItem(FltPkg->GetTeam() bitor 0x20000000, C_TYPE_ROOT, txt);
            gATOAll->AddItem(gATOAll->GetRoot(), team);
        }
    }
    else
        team = gATOAll->GetRoot();

    atopkg = NULL;
    mistype = MainFlt->GetUnitMission();

    if (mistype == AMIS_BARCAP2)
        mistype = AMIS_BARCAP;

    AtoMiss = static_cast<short>(MissionToATOMiss(mistype));

    missiontype = gATOAll->Find(AtoMiss bitor 0x40000000 bitor (FltPkg->GetTeam() << 16));

    if ( not missiontype)
    {
        txt = new C_Text;
        txt->Setup(mistype bitor 0x40000000, 0);
        txt->SetText(AtoMissStr[AtoMiss]);
        txt->SetFont(gATOAll->GetParent()->Font_);
        missiontype = gATOAll->CreateItem(AtoMiss bitor 0x40000000 bitor (FltPkg->GetTeam() << 16), C_TYPE_MENU, txt);

        if (TheCampaign.Flags bitand CAMP_TACTICAL_EDIT)
            gATOAll->AddChildItem(team, missiontype);
        else
            gATOAll->AddItem(team, missiontype);
    }

    atopkg = BuildATOPackageInfo(FltPkg);

    if (atopkg)
    {
        atopkg->SetFont(gATOAll->GetParent()->Font_);
        atopkg->SetCallback(SelectATOPackageCB);
        package = gATOAll->CreateItem(atopkg->GetID(), C_TYPE_MENU, atopkg);

        if (package)
        {
            atopkg->SetOwner(package);
            gATOAll->AddChildItem(missiontype, package);
        }

        return(atopkg);
    }

    return(NULL);
}

C_ATO_Flight *AddFlighttoATO(Flight flt)
{
    C_ATO_Flight *atoitem;
    Package FltPkg;
    TREELIST *item;
    TREELIST *package;

    ShiAssert(gATOAll);

    if ( not gATOAll)
        return NULL;

    if ( not (TheCampaign.Flags bitand CAMP_TACTICAL_EDIT))
    {
        if (flt->GetTeam() not_eq FalconLocalSession->GetTeam())
            return(NULL);
    }

    if (flt->GetUnitMission() == AMIS_ALERT or flt->GetUnitMission() == AMIS_ABORT)
        return(NULL);

    if ( not flt->GetUnitParent())
        return(NULL);

    FltPkg = (Package)flt->GetUnitParent();

    if ( not FltPkg)
        return(NULL);

    package = gATOAll->Find(FltPkg->GetCampID());

    if (package)
    {
        atoitem = BuildATOFlightInfo(flt);

        if (atoitem)
        {
            atoitem->SetMenu(UNIT_POP);
            atoitem->SetCallback(SelectATOItemCB);
            atoitem->SetFont(gATOAll->GetFont());
            item = gATOAll->CreateItem(atoitem->GetID(), C_TYPE_ITEM, atoitem);
            atoitem->SetOwner(item);
            gATOAll->AddChildItem(package, item);
            return(atoitem);
        }
    }

    return(NULL);
}





#if 0
OLD WAY

C_ATO_Flight *AddtoATO(Flight flt)
{
    C_ATO_Flight *atoitem;
    C_ATO_Package *atopkg;
    Package FltPkg;
    Flight MainFlt;
    TREELIST *item;
    TREELIST *team;
    TREELIST *missiontype;
    TREELIST *package;
    short mistype, AtoMiss;
    C_Text *txt;

    ShiAssert(gATOAll);

    if ( not gATOAll)
        return;

    if ( not (TheCampaign.Flags bitand CAMP_TACTICAL_EDIT))
    {
        if (flt->GetTeam() not_eq FalconLocalSession->GetTeam())
            return(NULL);
    }

    if (flt->GetUnitMission() == AMIS_ALERT or flt->GetUnitMission() == AMIS_ABORT)
        return(NULL);

    if ( not flt->GetUnitParent())
        return(NULL);

    FltPkg = (Package)flt->GetUnitParent();
    MainFlt = (Flight)FltPkg->GetFirstUnitElement();

    if ( not MainFlt)
        return(NULL);

    mistype = MainFlt->GetUnitMission();

    if (mistype == AMIS_ABORT or mistype == AMIS_ALERT)
        return(NULL);

    atoitem = BuildATOFlightInfo(flt);

    if (atoitem)
    {
        atoitem->SetMenu(UNIT_POP);
        atoitem->SetCallback(SelectATOItemCB);
        atoitem->SetFont(gATOAll->GetFont());

        if (TheCampaign.Flags bitand CAMP_TACTICAL_EDIT)
        {
            team = gATOAll->Find(flt->GetTeam() bitor 0x20000000);

            if ( not team)
            {
                txt = new C_Text;
                txt->Setup(flt->GetTeam() bitor 0x20000000, 0);
                txt->SetText(TeamInfo[flt->GetTeam()]->GetName());
                txt->SetFont(gATOAll->GetParent()->Font_);
                team = gATOAll->CreateItem(flt->GetTeam() bitor 0x20000000, C_TYPE_ROOT, txt);
                gATOAll->AddItem(gATOAll->GetRoot(), team);
            }
        }
        else
            team = gATOAll->GetRoot();

        mistype = MainFlt->GetUnitMission();
        package = gATOAll->Find(flt->GetUnitParent()->GetCampID() bitor (mistype << 16));

        if ( not package)
        {
            AtoMiss = MissionToATOMiss(mistype);

            missiontype = gATOAll->Find(AtoMiss bitor 0x40000000 bitor (flt->GetTeam() << 16));

            if ( not missiontype)
            {
                txt = new C_Text;
                txt->Setup(mistype bitor 0x40000000, 0);
                txt->SetText(AtoMissStr[AtoMiss]);
                txt->SetFont(gATOAll->GetParent()->Font_);
                missiontype = gATOAll->CreateItem(AtoMiss bitor 0x40000000 bitor (flt->GetTeam() << 16), C_TYPE_MENU, txt);

                if (TheCampaign.Flags bitand CAMP_TACTICAL_EDIT)
                    gATOAll->AddChildItem(team, missiontype);
                else
                    gATOAll->AddItem(team, missiontype);
            }

            if (mistype == AMIS_BARCAP2)
                mistype == AMIS_BARCAP;

            atopkg = BuildATOPackageInfo(FltPkg);

            if (atopkg)
            {
                atopkg->SetFont(gATOAll->GetParent()->Font_);
                atopkg->SetCallback(SelectATOPackageCB);
                package = gATOAll->CreateItem(atopkg->GetID() bitor (mistype << 16), C_TYPE_MENU, atopkg);
                gATOAll->AddChildItem(missiontype, package);
            }
            else
                package = team;
        }

        item = gATOAll->CreateItem(atoitem->GetID(), C_TYPE_ITEM, atoitem);
        atoitem->SetOwner(item);
        gATOAll->AddChildItem(package, item);
    }

    return(atoitem);
}

C_ATO_Package *AddPackagetoATO(Package FltPkg)
{
    C_ATO_Package *atopkg;
    Flight MainFlt;
    TREELIST *team;
    TREELIST *missiontype;
    TREELIST *package;
    short mistype, AtoMiss;
    C_Text *txt;

    ShiAssert(gATOAll);

    if ( not gATOAll)
        return;

    if ( not FltPkg)
        return(NULL);

    if ( not FltPkg->Final())
        return(NULL);

    if ( not (TheCampaign.Flags bitand CAMP_TACTICAL_EDIT))
    {
        if (FltPkg->GetTeam() not_eq FalconLocalSession->GetTeam())
            return(NULL);
    }

    MainFlt = (Flight)FltPkg->GetFirstUnitElement();

    if ( not MainFlt)
        return(NULL);

    mistype = MainFlt->GetUnitMission();

    if (mistype == AMIS_ABORT or mistype == AMIS_ALERT)
        return(NULL);

    if (TheCampaign.Flags bitand CAMP_TACTICAL_EDIT)
    {
        team = gATOAll->Find(FltPkg->GetTeam() bitor 0x20000000);

        if ( not team)
        {
            txt = new C_Text;
            txt->Setup(FltPkg->GetTeam() bitor 0x20000000, 0);
            txt->SetText(TeamInfo[FltPkg->GetTeam()]->GetName());
            txt->SetFont(gATOAll->GetParent()->Font_);
            team = gATOAll->CreateItem(FltPkg->GetTeam() bitor 0x20000000, C_TYPE_ROOT, txt);
            gATOAll->AddItem(gATOAll->GetRoot(), team);
        }
    }
    else
        team = gATOAll->GetRoot();

    atopkg = NULL;
    mistype = MainFlt->GetUnitMission();
    package = gATOAll->Find(FltPkg->GetCampID() bitor (mistype << 16));

    if ( not package)
    {
        AtoMiss = MissionToATOMiss(mistype);

        missiontype = gATOAll->Find(AtoMiss bitor 0x40000000 bitor (FltPkg->GetTeam() << 16));

        if ( not missiontype)
        {
            txt = new C_Text;
            txt->Setup(mistype bitor 0x40000000, 0);
            txt->SetText(AtoMissStr[AtoMiss]);
            txt->SetFont(gATOAll->GetParent()->Font_);
            missiontype = gATOAll->CreateItem(AtoMiss bitor 0x40000000 bitor (FltPkg->GetTeam() << 16), C_TYPE_MENU, txt);

            if (TheCampaign.Flags bitand CAMP_TACTICAL_EDIT)
                gATOAll->AddChildItem(team, missiontype);
            else
                gATOAll->AddItem(team, missiontype);
        }

        if (mistype == AMIS_BARCAP2)
            mistype == AMIS_BARCAP;

        atopkg = BuildATOPackageInfo(FltPkg);

        if (atopkg)
        {
            atopkg->SetFont(gATOAll->GetParent()->Font_);
            atopkg->SetCallback(SelectATOPackageCB);
            package = gATOAll->CreateItem(atopkg->GetID() bitor (mistype << 16), C_TYPE_MENU, atopkg);
            gATOAll->AddChildItem(missiontype, package);
        }
    }
    else
        atopkg = (C_ATO_Package*)package->Item_;

    return(atopkg);
}
#endif
