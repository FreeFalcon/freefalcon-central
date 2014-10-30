/*



 The Big OOB (Order of Battle)


*/

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
#include "userids.h"
#include "textids.h"
#include "classtbl.h"
#include "cmap.h"
#include "gps.h"
#include "logbook.h"
#include "remotelb.h"
#include "msginc/requestlogbook.h"
#include "ui_cmpgn.h"
#include "filters.h"
#include "urefresh.h"
#include "gps.h"
#include "cmap.h"
#include "teamdata.h"

enum
{
    SMALL_AF = 200100,
    SMALL_ARMY = 200101,
    SMALL_NAVY = 200102,
    SMALL_OBJ = 200103,
};

extern C_Handler *gMainHandler;
extern GlobalPositioningSystem *gGps;
C_Base *BuildOOBItem(CampEntity entity);
SQUADRONPLAYER *gPlayerSquadrons = NULL;
VU_ID gSelectedAirbase = FalconNullId;

extern GlobalPositioningSystem *gGps;
extern C_Map *gMapMgr;

static long TeamFlagBtnIDs[NUM_TEAMS] =
{
    GROUP1_FLAG,
    GROUP2_FLAG,
    GROUP3_FLAG,
    GROUP4_FLAG,
    GROUP5_FLAG,
    GROUP6_FLAG,
    GROUP7_FLAG,
    GROUP8_FLAG,
};

static long TeamColorCtrlIDs[NUM_TEAMS] =
{
    // C_Line
    GROUP1_COLOR,
    GROUP2_COLOR,
    GROUP3_COLOR,
    GROUP4_COLOR,
    GROUP5_COLOR,
    GROUP6_COLOR,
    GROUP7_COLOR,
    GROUP8_COLOR,
};

static long ObjectiveCategoryNames[] =
{
    0,
    TXT_AIRDEFENSES,
    TXT_AIRFIELDS,
    TXT_ARMY,
    TXT_CCC,
    TXT_INFRASTRUCTURE,
    TXT_LOGISTICS,
    TXT_OTHER,
    TXT_NAVIGATION,
    TXT_POLITICAL,
    TXT_WARPRODUCTION,
    TXT_NAVAL,
    0,
    0,
};

static long SectionFilterBtns[4] =
{
    AF_FILTER,
    ARMY_FILTER,
    NAVY_FILTER,
    OBJECTIVE_FILTER,
};

static long OOBCategories[4] =
{
    OOB_AIRFORCE,
    OOB_ARMY,
    OOB_NAVY,
    OOB_OBJECTIVE,
};

long TeamNameIDs[NUM_TEAMS] =
{
    TXT_NEUTRAL,
    TXT_COMBINEDFORCES,
    TXT_COMBINEDFORCES,
    TXT_JAPAN,
    TXT_CIS,
    TXT_CHINA,
    TXT_DPRK,
    TXT_NEUTRAL,
};

char gOOB_Visible[NUM_TEAMS];

// C_Button OOB_FIND_UNIT
// C_Button OOB_UNIT_INFO

C_TreeList *gOOBTree;
C_Squadron *BuildSquadronInfo(Squadron sqd);
C_Entity *BuildCategory(long Type);
void UpdateCategory(C_Entity *category);
void SetupSquadronInfoWindow(VU_ID TheID);
void SetupUnitInfoWindow(VU_ID TheID);
void SetupDivisionInfoWindow(long DivID, short owner);
void BuildSpecificTargetList(VU_ID targetID);

VU_ID gSelectedEntity;
long gSelectedDivision = 0;

void SelectOOBSquadronCB(long, short hittype, C_Base *control)
{
    C_Squadron *squad;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    gOOBTree->SetAllControlStates(0, gOOBTree->GetRoot());
    squad = (C_Squadron*)control;
    squad->SetState(1);
    squad->Refresh();

    gSelectedEntity = squad->GetVUID();
    gSelectedDivision = 0;
}

void SelectOOBEntityCB(long, short hittype, C_Base *control)
{
    C_Entity *ent;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    gOOBTree->SetAllControlStates(0, gOOBTree->GetRoot());
    ent = (C_Entity*)control;
    ent->SetState(1);
    ent->Refresh();

    if (ent->GetID() bitand UR_DIVISION)
    {
        gSelectedDivision = ent->GetID();
        gSelectedEntity = FalconNullId;
    }
    else
    {
        gSelectedEntity = ent->GetVUID();
        gSelectedDivision = 0;
    }
}

void SetupOOBWindow()
{
    C_Window *win;
    long i, j, cat, TeamID;
    C_Button *btn;
    C_Line   *line;
    C_Text *txt;
    TREELIST *team;
    C_Entity *category;
    short idx;

    win = gMainHandler->FindWindow(OOB_WIN);

    if (win)
    {
        idx = 0;

        for (i = 0; i < NUM_TEAMS; i++)
        {
            if (TeamInfo[i] and ((TeamInfo[i]->flags bitand TEAM_ACTIVE) or GetTeam(static_cast<uchar>(i)) not_eq i))
            {
                btn = (C_Button*)win->FindControl(TeamFlagBtnIDs[idx]);

                if (btn)
                {
                    btn->SetImage(C_STATE_0, FlagImageID[TeamInfo[i]->GetFlag()][BIG_VERT_DARK]);
                    btn->SetImage(C_STATE_1, FlagImageID[TeamInfo[i]->GetFlag()][BIG_VERT]);
                    btn->SetFlagBitOff(C_BIT_INVISIBLE);
                    btn->SetHelpText(gStringMgr->AddText(TeamInfo[i]->GetName()));
                    btn->SetUserNumber(0, i);

                    if (btn->GetState())
                        gOOB_Visible[i] = 1;
                    else
                        gOOB_Visible[i] = 0;
                }

                line = (C_Line*)win->FindControl(TeamColorCtrlIDs[idx]);

                if (line)
                {
                    if (TeamInfo[GetTeam(static_cast<uchar>(i))] and TeamInfo[GetTeam(static_cast<uchar>(i))]->flags bitand TEAM_ACTIVE)
                        line->SetColor(TeamColorList[TeamInfo[GetTeam(static_cast<uchar>(i))]->GetColor()]);
                    else
                        line->SetColor(TeamColorList[TeamInfo[i]->GetColor()]);

                    line->SetFlagBitOff(C_BIT_INVISIBLE);
                }

                idx++;
            }
            else
                gOOB_Visible[i] = 0;
        }

        while (idx < NUM_TEAMS)
        {
            btn = (C_Button*)win->FindControl(TeamFlagBtnIDs[idx]);

            if (btn)
                btn->SetFlagBitOn(C_BIT_INVISIBLE);

            line = (C_Line*)win->FindControl(TeamColorCtrlIDs[idx]);

            if (line)
                line->SetFlagBitOn(C_BIT_INVISIBLE);

            idx++;
        }

        // Setup OOB Team Branches
        if (gOOBTree)
        {
            idx = 0;

            for (i = 0; i < NUM_TEAMS; i++)
            {
                if (TeamInfo[i] and ((TeamInfo[i]->flags bitand TEAM_ACTIVE) or GetTeam(static_cast<uchar>(i)) not_eq i))
                {
                    for (j = 0; j < 4; j++)
                    {
                        btn = (C_Button*)win->FindControl(SectionFilterBtns[j]);

                        if (btn and btn->GetState())
                            cat = 1;
                        else
                            cat = 0;

                        TeamID = (i << 24) bitor OOBCategories[j];

                        team = gOOBTree->Find(TeamID);

                        if ( not team)
                        {
                            category = BuildCategory(TeamID);

                            if (category)
                            {
                                team = gOOBTree->CreateItem(TeamID,/* category->GetType() */ C_TYPE_MENU, category);
                                category->SetOwner(team);
                                category->SetFont(gOOBTree->GetParent()->Font_);

                                if (gOOB_Visible[i] and cat)
                                    category->SetFlagBitOff(C_BIT_INVISIBLE);
                                else
                                    category->SetFlagBitOn(C_BIT_INVISIBLE);
                            }
                            else
                            {
                                txt = new C_Text;
                                txt->Setup(TeamID, 0);
                                txt->SetText(gStringMgr->GetText(gStringMgr->AddText("Bermuda Triangle")));
                                txt->SetFont(gOOBTree->GetParent()->Font_);
                                team = gOOBTree->CreateItem(TeamID, C_TYPE_MENU, txt);

                                if (gOOB_Visible[i] and cat)
                                    txt->SetFlagBitOff(C_BIT_INVISIBLE);
                                else
                                    txt->SetFlagBitOn(C_BIT_INVISIBLE);
                            }

                            gOOBTree->AddItem(gOOBTree->GetRoot(), team);
                        }
                        else
                            UpdateCategory((C_Entity*)team->Item_);
                    }

                    idx++;
                }
            }
        }

        win->RefreshWindow();
    }
}

BOOL FindChildren(TREELIST *list, short owner)
{
    TREELIST *item;

    if ( not list or not owner)
        return(FALSE);

    item = list;

    while (item)
    {
        if (item->Item_->GetUserNumber(0) == owner)
            return(TRUE);

        if (item->Child)
            if (FindChildren(item->Child, owner))
                return(TRUE);

        item = item->Next;
    }

    return(FALSE);
}

BOOL FindOtherChildren(TREELIST *list, short owner)
{
    TREELIST *item;

    if ( not list or not owner)
        return(FALSE);

    item = list;

    while (item)
    {
        if ( not (item->Item_->GetFlags() bitand C_BIT_INVISIBLE))
        {
            if (item->Item_->GetUserNumber(0) not_eq owner)
                return(TRUE);

            if (item->Child)
                if (FindOtherChildren(item->Child, owner))
                    return(TRUE);
        }

        item = item->Next;
    }

    return(FALSE);
}

void ToggleOOBTeamCB(long, short hittype, C_Base *control)
{
    C_Window *win;
    C_Button *btn;
    long i, j, TeamID;
    short owner;
    BOOL TurnOn, DontTurnOff;
    TREELIST *root, *child;
    F4CSECTIONHANDLE *Leave;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    win = control->GetParent();

    if ( not win)
        return;

    Leave = UI_Enter(control->Parent_);

    owner = static_cast<short>(control->GetUserNumber(0));
    TeamID = GetTeam(static_cast<uchar>(owner));

    if (control->GetState())
    {
        gOOB_Visible[owner] = 1;
        TurnOn = TRUE;
    }
    else
    {
        gOOB_Visible[owner] = 0;
        TurnOn = FALSE;
    }

    if (TurnOn)
    {
        for (i = 0; i < 4; i++)
        {
            btn = (C_Button*)win->FindControl(SectionFilterBtns[i]);

            if (btn and btn->GetState())
            {
                root = gOOBTree->Find((TeamID << 24) bitor OOBCategories[i]);

                if (root and root->Item_->GetFlags() bitand C_BIT_INVISIBLE)
                    root->Item_->SetFlagBitOff(C_BIT_INVISIBLE);
            }
        }
    }
    else
    {
        DontTurnOff = FALSE;

        for (i = 0; i < NUM_TEAMS; i++)
        {
            btn = (C_Button *)win->FindControl(TeamFlagBtnIDs[i]);

            if (btn and i not_eq owner)
            {
                if ((GetTeam(static_cast<uchar>(btn->GetUserNumber(0))) == TeamID) and btn->GetState())
                    DontTurnOff = TRUE;
            }
        }

        if ( not DontTurnOff)
            for (i = 0; i < 4; i++)
            {
                root = gOOBTree->Find((TeamID << 24) bitor OOBCategories[i]);

                if (root)
                    root->Item_->SetFlagBitOn(C_BIT_INVISIBLE);
            }
    }

    for (i = 0; i < 4; i++)
    {
        for (j = 0; j < 8; j++)
        {
            root = gOOBTree->Find((j << 24) bitor OOBCategories[i]);

            if (root)
            {
                root = root->Child;

                while (root)
                {
                    if (TurnOn)
                    {
                        if (root->Item_->GetUserNumber(0) == owner or FindChildren(root->Child, owner))
                        {
                            root->Item_->SetFlagBitOff(C_BIT_INVISIBLE);
                            child = root->Child;

                            while (child)
                            {
                                if (child->Item_->GetUserNumber(0) == owner)
                                    child->Item_->SetFlagBitOff(C_BIT_INVISIBLE);

                                child = child->Next;
                            }
                        }
                    }
                    else
                    {
                        if (root->Item_->GetUserNumber(0) == owner and not FindOtherChildren(root->Child, owner))
                            root->Item_->SetFlagBitOn(C_BIT_INVISIBLE);
                        else
                        {
                            child = root->Child;

                            while (child)
                            {
                                if (child->Item_->GetUserNumber(0) == owner)
                                    child->Item_->SetFlagBitOn(C_BIT_INVISIBLE);

                                child = child->Next;
                            }
                        }
                    }

                    root = root->Next;
                }
            }
        }
    }

    gOOBTree->RecalcSize();
    gOOBTree->Parent_->RefreshClient(gOOBTree->GetClient());
    UI_Leave(Leave);
}

void ToggleOOBFilterCB(long ID, short hittype, C_Base *control)
{
    C_Button *btn = NULL;
    long i, Cat = 0, j = 0;
    short owner;
    TREELIST *root = NULL, *child = NULL;
    BOOL TurnOn = FALSE;

    F4CSECTIONHANDLE *Leave;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    Leave = UI_Enter(control->Parent_);

    owner = static_cast<short>(control->GetUserNumber(0));

    for (i = 0; i < 4; i++)
        if (ID == SectionFilterBtns[i])
            Cat = OOBCategories[i];

    if (control->GetState())
        TurnOn = TRUE;
    else
        TurnOn = FALSE;

    if (TurnOn)
    {
        for (i = 0; i < 8; i++)
        {
            btn = (C_Button*)control->GetParent()->FindControl(TeamFlagBtnIDs[i]);

            if (btn and not (btn->GetFlags() bitand C_BIT_INVISIBLE) and btn->GetState())
            {
                // 2002-01-04 MODIFIED BY S.G. GetTeam is 'based one' and not 'based zero' so I'll add '1' to i.
                // root=gOOBTree->Find((GetTeam(static_cast<uchar>(i)) << 24) bitor Cat);
                root = gOOBTree->Find((GetTeam(static_cast<uchar>(i + 1)) << 24) bitor Cat);

                if (root and root->Item_->GetFlags() bitand C_BIT_INVISIBLE)
                    root->Item_->SetFlagBitOff(C_BIT_INVISIBLE);
            }
        }
    }
    else
    {
        root = gOOBTree->GetRoot();

        while (root)
        {
            if (root->ID_ bitand Cat)
                root->Item_->SetFlagBitOn(C_BIT_INVISIBLE);

            root = root->Next;
        }
    }

    // 2002-01-04 REMOVED BY S.G. This section of code seems to do nothing but mess up the Objectives when something else is deselected.
    //                            The Objective names remains there but without the '+' side beside it.
    //                            It deals with child root object and only Objectives have child root object (like CCC) and seems to concur to what I'm seeing.
    //                            I'm seeing not ill effect with this code commented out.
    /*
     for(i=0;i<4;i++)
     {
     for(j=0;j<8;j++)
     {
     root=gOOBTree->Find((j << 24) bitor OOBCategories[i]); // Comment by S.G. Here j starting at zero is not going to harm anything since we do it for ALL teams so left as is
     if(root)
     {
     root=root->Child;
     while(root)
     {
     if(TurnOn)
     {
     if(root->Item_->GetUserNumber(0) == owner or FindChildren(root->Child,owner))
     {
     root->Item_->SetFlagBitOff(C_BIT_INVISIBLE);
     child=root->Child;
     while(child)
     {
     if(child->Item_->GetUserNumber(0) == owner)
     child->Item_->SetFlagBitOff(C_BIT_INVISIBLE);
     child=child->Next;
     }
     }
     }
     else
     {
     if(root->Item_->GetUserNumber(0) == owner and not FindOtherChildren(root->Child,owner))
     root->Item_->SetFlagBitOn(C_BIT_INVISIBLE);
     else
     {
     child=root->Child;
     while(child)
     {
     if(child->Item_->GetUserNumber(0) == owner)
     child->Item_->SetFlagBitOn(C_BIT_INVISIBLE);
     child=child->Next;
     }
     }
     }
     root=root->Next;
     }
     }
     }
     }
    */
    gOOBTree->RecalcSize();
    gOOBTree->Parent_->RefreshClient(gOOBTree->GetClient());
    UI_Leave(Leave);
}

C_Entity *BuildObjectiveInfo(Objective obj)
{
    C_Resmgr *res = NULL;
    IMAGE_RSC *rsc;
    C_Entity *newinfo;
    _TCHAR buffer[200];
    ObjClassDataType *ObjPtr;
    short type = 0, TypeID;
    long ObjType;

    ObjPtr = obj->GetObjectiveClassData();

    if (ObjPtr == NULL)
        return(NULL);

    ObjType = GetObjectiveType(obj);
    TypeID = static_cast<short>(FindTypeIndex(ObjType, OBJ_TypeList, _MAP_NUM_OBJ_TYPES_));

    // Create new parent class
    newinfo = new C_Entity;
    newinfo->Setup(obj->GetCampID(), TypeID);
    newinfo->SetWH(286, 37);
    newinfo->InitEntity();

    newinfo->SetVUID(obj->Id());
    newinfo->SetUserNumber(0, obj->GetOwner()); // MUST be GetOwner not GetTeam

    newinfo->SetOperational(obj->GetObjectiveStatus());

    if (gImageMgr and TeamInfo[obj->GetTeam()])
        res = gImageMgr->GetImageRes(TeamColorIconIDs[TeamInfo[obj->GetTeam()]->GetColor()][0]);

    if (res)
    {
        rsc = (IMAGE_RSC*)res->Find(ObjPtr->IconIndex);

        if (rsc and rsc->Header->Type == _RSC_IS_IMAGE_)
            type = rsc->Header->h;
        else
            type = 0;
    }
    else
        rsc = NULL;

    if (type > 35)
        newinfo->SetH(type + 2);
    else
        type = 35;

    // Set BG Area behind Airplane Icon
    newinfo->SetIconBgColor(0x7c5600, 0);
    newinfo->SetIconBg(0, 0, 32, type);

    // Set BG Area behind rest of Squadron text
    newinfo->SetInfoBgColor(0, 0x7c5600);
    newinfo->SetInfoBg(32, 0, 254, type);

    // Set Icon Image
    newinfo->SetIcon(15, static_cast<short>(type / 2), rsc);

    // Set Name
    obj->GetName(buffer, 40, TRUE);
    newinfo->SetName(35, 0, gStringMgr->GetText(gStringMgr->AddText(buffer)));

    // Set # bitand Airplane type
    _stprintf(buffer, "%1d%% %s", newinfo->GetOperational(), gStringMgr->GetString(TXT_OPERATIONAL));
    newinfo->SetStatus(35, 15, gStringMgr->GetText(gStringMgr->AddText(buffer)));

    return(newinfo);
}

C_Entity *BuildDivisionInfo(Division div, Unit unit)
{
    C_Resmgr *res;
    IMAGE_RSC *rsc;
    C_Entity *newinfo;
    _TCHAR buffer[200];
    short type = 0;
    long totalstr, curstr, perc;

    UnitClassDataType *UnitPtr;

    UnitPtr = unit->GetUnitClassData();

    if (UnitPtr == NULL)
        return(NULL);

    //if( not UnitPtr->IconIndex)
    // return(NULL);

    // Create new parent class
    newinfo = new C_Entity;
    newinfo->Setup((unit->GetTeam() << 24) bitor div->nid bitor UR_DIVISION, C_TYPE_MENU);
    newinfo->SetWH(286, 37);
    newinfo->InitEntity();

    newinfo->SetUserNumber(0, unit->GetOwner()); // MUST be GetOwner not GetTeam

    curstr = unit->GetTotalVehicles();
    totalstr = unit->GetFullstrengthVehicles();

    if (totalstr < 1) totalstr = 1;

    perc = (curstr * 100) / totalstr;

    if (perc > 100) perc = 100;

    newinfo->SetOperational(static_cast<uchar>(perc));

    res = gImageMgr->GetImageRes(TeamColorIconIDs[TeamInfo[unit->GetTeam()]->GetColor()][0]);

    if (res)
    {
        rsc = (IMAGE_RSC*)res->Find(UnitPtr->IconIndex);

        if (rsc and rsc->Header->Type == _RSC_IS_IMAGE_)
            type = rsc->Header->h;
        else
            type = 0;
    }
    else
        rsc = NULL;

    if (type > 35)
        newinfo->SetH(type + 2);
    else
        type = 35;

    // Set BG Area behind Airplane Icon
    newinfo->SetIconBgColor(0x7c5600, 0);
    newinfo->SetIconBg(0, 0, 32, type);

    // Set BG Area behind rest of Squadron text
    newinfo->SetInfoBgColor(0, 0x7c5600);
    newinfo->SetInfoBg(32, 0, 254, type);

    // Set Icon Image
    newinfo->SetIcon(15, static_cast<short>(type / 2), rsc);

    // Set Name
    div->GetName(buffer, 40, FALSE);
    newinfo->SetName(35, 0, gStringMgr->GetText(gStringMgr->AddText(buffer)));

    // Set # bitand Airplane type
    MonoPrint("[Wrong]\n");
    _stprintf(buffer, "%1d%% %s", newinfo->GetOperational(), gStringMgr->GetString(TXT_OPERATIONAL));
    newinfo->SetStatus(35, 15, gStringMgr->GetText(gStringMgr->AddText(buffer)));

    return(newinfo);
}

C_Entity *BuildUnitInfo(Unit unit)
{
    C_Resmgr *res;
    IMAGE_RSC *rsc;
    C_Entity *newinfo;
    _TCHAR buffer[200];
    short type = 0;
    long totalstr, curstr, perc;

    UnitClassDataType *UnitPtr;

    UnitPtr = unit->GetUnitClassData();

    if (UnitPtr == NULL)
        return(NULL);

    //if( not UnitPtr->IconIndex)
    // return(NULL);

    // Create new parent class
    newinfo = new C_Entity;
    newinfo->Setup(unit->GetCampID(), C_TYPE_MENU);
    newinfo->SetWH(286, 37);
    newinfo->InitEntity();

    newinfo->SetVUID(unit->Id());
    newinfo->SetUserNumber(0, unit->GetOwner()); // MUST be GetOwner not GetTeam

    curstr = unit->GetTotalVehicles();
    totalstr = unit->GetFullstrengthVehicles();

    if (totalstr < 1) totalstr = 1;

    perc = (curstr * 100) / totalstr;

    if (perc > 100) perc = 100;

    newinfo->SetOperational(static_cast<char>(perc));

    // FRB - deleting too many flags (teams) CTD
    if (perc <= 0)
    {
        perc = 1;
        res = 0;
    }
    else
        res = gImageMgr->GetImageRes(TeamColorIconIDs[TeamInfo[unit->GetTeam()]->GetColor()][0]);

    if (res)
    {
        rsc = (IMAGE_RSC*)res->Find(UnitPtr->IconIndex);

        if (rsc and rsc->Header->Type == _RSC_IS_IMAGE_)
            type = rsc->Header->h;
        else
            type = 0;
    }
    else
        rsc = NULL;

    if (type > 35)
        newinfo->SetH(type + 2);
    else
        type = 35;

    // Set BG Area behind Airplane Icon
    newinfo->SetIconBgColor(0x7c5600, 0);
    newinfo->SetIconBg(0, 0, 32, type);

    // Set BG Area behind rest of Squadron text
    newinfo->SetInfoBgColor(0, 0x7c5600);
    newinfo->SetInfoBg(32, 0, 254, type);

    // Set Icon Image
    newinfo->SetIcon(15, static_cast<short>(type / 2), rsc);

    // Set Name
    if (unit->IsFlight())
        GetCallsign(((Flight)unit)->callsign_id, ((Flight)unit)->callsign_num, buffer);
    else
        unit->GetName(buffer, 40, FALSE);

    newinfo->SetName(35, 0, gStringMgr->GetText(gStringMgr->AddText(buffer)));

    // Set # bitand Airplane type
    _stprintf(buffer, "%1d%% %s", newinfo->GetOperational(), gStringMgr->GetString(TXT_OPERATIONAL));
    newinfo->SetStatus(35, 15, gStringMgr->GetText(gStringMgr->AddText(buffer)));

    return(newinfo);
}

C_Entity *BuildNavalUnitInfo(Unit unit)
{
    C_Resmgr *res = NULL;
    IMAGE_RSC *rsc = NULL;
    C_Entity *newinfo = NULL;
    _TCHAR buffer[200] = {0};
    short type = 32;
    long totalstr = 0, curstr = 0, perc = 0;

    UnitClassDataType *UnitPtr = NULL;

    UnitPtr = unit->GetUnitClassData();

    if (UnitPtr == NULL)
        return(NULL);

    // Create new parent class
    newinfo = new C_Entity;
    newinfo->Setup(unit->GetCampID(), C_TYPE_MENU);
    newinfo->SetWH(270, 32);
    newinfo->InitEntity();

    newinfo->SetVUID(unit->Id());
    newinfo->SetUserNumber(0, unit->GetOwner()); // MUST be GetOwner not GetTeam

    curstr = unit->GetTotalVehicles();
    totalstr = unit->GetFullstrengthVehicles();

    if (totalstr < 1) totalstr = 1;

    perc = (curstr * 100) / totalstr;

    if (perc > 100) perc = 100;

    newinfo->SetOperational(static_cast<uchar>(perc));

    // FRB - CTD's Here
    // FRB - deleting too many flags (teams) CTD
    if (perc <= 0)
    {
        perc = 1;
        res = 0;
    }
    else
        res = gImageMgr->GetImageRes(TeamColorIconIDs[TeamInfo[unit->GetTeam()]->GetColor()][0]);

    if (res)
    {
        rsc = (IMAGE_RSC*)res->Find(UnitPtr->IconIndex);

        if (rsc and rsc->Header->Type == _RSC_IS_IMAGE_)
            type = rsc->Header->h;
        else
            type = 0;
    }
    else
        rsc = NULL;

    if (type > 32)
        newinfo->SetH(type + 2);
    else
        type = 32;

    // Set BG Area behind Airplane Icon
    newinfo->SetIconBgColor(0x7c5600, 0);
    newinfo->SetIconBg(0, 0, 80, type);

    // Set BG Area behind rest of Squadron text
    newinfo->SetInfoBgColor(0, 0x7c5600);
    newinfo->SetInfoBg(80, 0, 190, type);

    // Set Icon Image
    newinfo->SetIcon(40, static_cast<short>(type / 2), rsc);

    // Set Name
    unit->GetName(buffer, 40, FALSE);
    newinfo->SetName(84, 0, gStringMgr->GetText(gStringMgr->AddText(buffer)));

    // Set # bitand Airplane type
    _stprintf(buffer, "%1d%% %s", newinfo->GetOperational(), gStringMgr->GetString(TXT_OPERATIONAL));
    newinfo->SetStatus(84, 15, gStringMgr->GetText(gStringMgr->AddText(buffer)));

    return(newinfo);
}

void TallyPlayerSquadrons()
{
    SQUADRONPLAYER *cur, *prev;
    FalconSessionEntity *session;
    int done;

    cur = gPlayerSquadrons;

    while (cur)
    {
        prev = cur;
        cur = cur->Next;
        delete prev;
    }

    gPlayerSquadrons = NULL;

    if (gCommsMgr->GetGame())
    {
        VuSessionsIterator sessionWalker(gCommsMgr->GetGame());
        session = (FalconSessionEntity*)sessionWalker.GetFirst();

        while (session)
        {
            done = 0;
            cur = gPlayerSquadrons;

            while (cur and not done)
            {
                if (cur->SquadronID == session->GetPlayerSquadronID())
                {
                    cur->PlayerCount++;
                    done = 1;
                }
                else
                    cur = cur->Next;
            }

            if ( not cur)
            {
                prev = new SQUADRONPLAYER;
                prev->SquadronID = session->GetPlayerSquadronID();
                prev->PlayerCount = 1;
                prev->Next = NULL;

                if ( not gPlayerSquadrons)
                    gPlayerSquadrons = prev;
                else
                {
                    cur = gPlayerSquadrons;

                    while (cur->Next)
                        cur = cur->Next;

                    cur->Next = prev;
                }
            }

            session = (FalconSessionEntity*)sessionWalker.GetNext();
        }
    }
}

void GetPlayerInfo(VU_ID ID)
{
    UI_RequestLogbook *rlb;
    FalconSessionEntity *session;

    session = (FalconSessionEntity*)vuDatabase->Find(ID);

    if (session and session not_eq FalconLocalSession)
    {
        rlb = new UI_RequestLogbook(FalconNullId, session);
        rlb->dataBlock.fromID = FalconLocalSessionId;

        FalconSendMessage(rlb, true);
    }
}


short GetSquadronPlayerCount(VU_ID SquadID)
{
    SQUADRONPLAYER *cur;

    cur = gPlayerSquadrons;

    while (cur)
    {
        if (cur->SquadronID == SquadID)
            return(cur->PlayerCount);

        cur = cur->Next;
    }

    return(0);
}

C_Squadron *BuildSquadronInfo(Squadron sqd)
{
    C_Squadron *newinfo;
    _TCHAR buffer[200];
    IMAGE_RSC *img;
    short type;
    UnitClassDataType *UnitPtr;

    UnitPtr = sqd->GetUnitClassData();

    if (UnitPtr == NULL)
        return(NULL);

    // Create new parent class
    newinfo = new C_Squadron;
    newinfo->Setup(sqd->GetCampID(), C_TYPE_MENU);
    newinfo->SetWH(286, 52);
    newinfo->InitSquadron();

    newinfo->SetVUID(sqd->Id());
    newinfo->SetUserNumber(0, sqd->GetOwner()); // MUST be GetOwner not GetTeam

    newinfo->SetNumVehicles(static_cast<short>(sqd->GetTotalVehicles()));
    newinfo->SetNumPilots(static_cast<short>(sqd->NumActivePilots()));
    newinfo->SetNumPlayers(GetSquadronPlayerCount(sqd->Id()));

    img = gImageMgr->GetImage(UnitPtr->IconIndex);

    if (img)
        type = img->Header->h;
    else
        type = 0;

    if (type > 50)
        newinfo->SetH(type + 2);
    else
        type = 50;

    // Set BG Area behind Airplane Icon
    newinfo->SetIconBgColor(0x7c5600, 0);
    newinfo->SetIconBg(0, 0, 46, type);

    // Set BG Area behind rest of Squadron text
    newinfo->SetInfoBgColor(0, 0x7c5600);
    newinfo->SetInfoBg(46, 0, 240, type);

    // Set Icon Image
    newinfo->SetIcon(23, static_cast<short>(type / 2), UnitPtr->IconIndex);

    // Set Name
    sqd->GetName(buffer, 40, FALSE);
    newinfo->SetName(54, 5, gStringMgr->GetText(gStringMgr->AddText(buffer)));

    // Set # bitand Airplane type
    _stprintf(buffer, "%1d %s", newinfo->GetNumVehicles(), GetVehicleName(sqd->GetVehicleID(0)));
    newinfo->SetPlanes(54, 20, gStringMgr->GetText(gStringMgr->AddText(buffer)));

    // Set Total # of Pilots
    _stprintf(buffer, "%1d %s", newinfo->GetNumPilots() + newinfo->GetNumPlayers(), gStringMgr->GetString(TXT_PILOTS));
    newinfo->SetPilots(160, 20, gStringMgr->GetText(gStringMgr->AddText(buffer)));

    // Set #  of Human Pilots
    _stprintf(buffer, "%1d %s", newinfo->GetNumPlayers(), gStringMgr->GetString(TXT_PLAYERS));
    newinfo->SetPlayers(54, 35, gStringMgr->GetText(gStringMgr->AddText(buffer)));
    return(newinfo);
}

C_Base *BuildOOBItem(CampEntity entity)
{
    C_Base *builder;

    if (entity->IsSquadron())
    {
        builder = BuildSquadronInfo((Squadron)entity);

        if (builder)
        {
            ((C_Squadron*)builder)->SetCallback(SelectOOBSquadronCB);
            return(builder);
        }
    }
    else if (entity->IsBattalion() or entity->IsBrigade())
    {
        builder = BuildUnitInfo((Unit)entity);

        if (builder)
        {
            ((C_Entity*)builder)->SetCallback(SelectOOBEntityCB);
            return(builder);
        }
    }
    else if (entity->IsTaskForce())
    {
        builder = BuildNavalUnitInfo((Unit)entity);

        if (builder)
        {
            ((C_Entity*)builder)->SetCallback(SelectOOBEntityCB);
            return(builder);
        }
    }
    else if (entity->IsObjective())
    {
        builder = BuildObjectiveInfo((Objective)entity);

        if (builder)
        {
            ((C_Entity*)builder)->SetCallback(SelectOOBEntityCB);
            return(builder);
        }
    }

    return(NULL);
}

C_Entity *BuildCategory(long Type)
{
    C_Entity *newinfo;
    _TCHAR buffer[200];
    IMAGE_RSC *img;
    int i;

    // This type DOESN'T have the BLACK bitand BLUE Background

    // Create new parent class
    newinfo = new C_Entity;
    newinfo->Setup(Type, 0);
    newinfo->SetWH(320, 22);
    newinfo->InitEntity();

    if (Type bitand OOB_AIRFORCE)
    {
        img = gImageMgr->GetImage(SMALL_AF);

        if (img)
            i = img->Header->h;
        else
            i = 20;

        if (i > 20)
            newinfo->SetH(i + 2);

        newinfo->SetIcon(15, static_cast<short>(i / 2), SMALL_AF);

        if (TheCampaign.Flags bitand CAMP_TACTICAL)
            _stprintf(buffer, "%s : %s", gStringMgr->GetString(TXT_AIRFORCES), TeamInfo[(Type bitand OOB_TEAM_MASK) >> 24]->GetName());
        else
            _stprintf(buffer, "%s : %s", gStringMgr->GetString(TXT_AIRFORCES), gStringMgr->GetString(TeamNameIDs[(Type bitand OOB_TEAM_MASK) >> 24]));
    }
    else if (Type bitand OOB_ARMY)
    {
        img = gImageMgr->GetImage(SMALL_AF);

        if (img)
            i = img->Header->h;
        else
            i = 20;

        if (i > 20)
            newinfo->SetH(i + 2);

        newinfo->SetIcon(15, static_cast<short>(i / 2), SMALL_ARMY);

        if (TheCampaign.Flags bitand CAMP_TACTICAL)
            _stprintf(buffer, "%s : %s", gStringMgr->GetString(TXT_ARMY), TeamInfo[(Type bitand OOB_TEAM_MASK) >> 24]->GetName());
        else
            _stprintf(buffer, "%s : %s", gStringMgr->GetString(TXT_ARMY), gStringMgr->GetString(TeamNameIDs[(Type bitand OOB_TEAM_MASK) >> 24]));
    }
    else if (Type bitand OOB_NAVY)
    {
        img = gImageMgr->GetImage(SMALL_AF);

        if (img)
            i = img->Header->h;
        else
            i = 20;

        if (i > 20)
            newinfo->SetH(i + 2);

        newinfo->SetIcon(15, static_cast<short>(i / 2), SMALL_NAVY);

        if (TheCampaign.Flags bitand CAMP_TACTICAL)
            _stprintf(buffer, "%s : %s", gStringMgr->GetString(TXT_NAVALFORCES), TeamInfo[(Type bitand OOB_TEAM_MASK) >> 24]->GetName());
        else
            _stprintf(buffer, "%s : %s", gStringMgr->GetString(TXT_NAVALFORCES), gStringMgr->GetString(TeamNameIDs[(Type bitand OOB_TEAM_MASK) >> 24]));
    }
    else if (Type bitand OOB_OBJECTIVE)
    {
        img = gImageMgr->GetImage(SMALL_AF);

        if (img)
            i = img->Header->h;
        else
            i = 20;

        if (i > 20)
            newinfo->SetH(i + 2);

        newinfo->SetIcon(15, static_cast<short>(i / 2), SMALL_OBJ);

        if (TheCampaign.Flags bitand CAMP_TACTICAL)
            _stprintf(buffer, "%s : %s", gStringMgr->GetString(TXT_OBJECTIVES), TeamInfo[(Type bitand OOB_TEAM_MASK) >> 24]->GetName());
        else
            _stprintf(buffer, "%s : %s", gStringMgr->GetString(TXT_OBJECTIVES), gStringMgr->GetString(TeamNameIDs[(Type bitand OOB_TEAM_MASK) >> 24]));
    }

    // Set Name
    newinfo->SetName(35, 0, gStringMgr->GetText(gStringMgr->AddText(buffer)));

    return(newinfo);
}

// This basically changes the team name
void UpdateCategory(C_Entity *category)
{
    long Type;
    _TCHAR buffer[200];

    if ( not category)
        return;

    Type = category->GetID();

    if (Type bitand OOB_AIRFORCE)
    {
        _stprintf(buffer, "%s : %s", gStringMgr->GetString(TXT_AIRFORCES), TeamInfo[(Type bitand OOB_TEAM_MASK) >> 24]->GetName());
    }
    else if (Type bitand OOB_ARMY)
    {
        _stprintf(buffer, "%s : %s", gStringMgr->GetString(TXT_ARMY), TeamInfo[(Type bitand OOB_TEAM_MASK) >> 24]->GetName());
    }
    else if (Type bitand OOB_NAVY)
    {
        _stprintf(buffer, "%s : %s", gStringMgr->GetString(TXT_NAVALFORCES), TeamInfo[(Type bitand OOB_TEAM_MASK) >> 24]->GetName());
    }
    else if (Type bitand OOB_OBJECTIVE)
    {
        _stprintf(buffer, "%s : %s", gStringMgr->GetString(TXT_OBJECTIVES), TeamInfo[(Type bitand OOB_TEAM_MASK) >> 24]->GetName());
    }

    category->SetName(35, 0, gStringMgr->GetText(gStringMgr->AddText(buffer)));
}

C_Entity *AddDivisionToOOB(Division div)
{
    TREELIST *Team_Cat = NULL, *item = NULL;
    Unit un;
    C_Entity *oobitem = NULL;
    long TeamID, Cat;

    if ( not div)
        return(NULL);

    un = div->GetFirstUnitElement();

    if ( not un)
        return(NULL);

    while (un and un->GetSType() not_eq div->type)
        un = div->GetNextUnitElement();

    if ( not un)
        un = div->GetFirstUnitElement();

    TeamID = (un->GetTeam() << 24);
    Cat = FindUnitCategory(un);

    if ( not Cat)
        return(NULL);

    TeamID or_eq Cat;
    Team_Cat = gOOBTree->Find(TeamID);

    if (Team_Cat)
    {
        item = gOOBTree->Find((un->GetTeam() << 24) bitor div->nid bitor UR_DIVISION);

        if (item)
            return((C_Entity *)item->Item_);

        oobitem = BuildDivisionInfo(div, un);

        if (oobitem)
        {
            oobitem->SetCallback(SelectOOBEntityCB);
            item = gOOBTree->CreateItem(oobitem->GetID(),/* oobitem->GetType()*/ C_TYPE_ITEM, oobitem);

            if (item)
            {
                if (oobitem->_GetCType_() == _CNTL_ENTITY_)
                    ((C_Entity*)oobitem)->SetOwner(item);
                else if (oobitem->_GetCType_() == _CNTL_SQUAD_)
                    ((C_Squadron*)oobitem)->SetOwner(item);

                oobitem->SetFont(gOOBTree->GetFont());
                gOOBTree->AddChildItem(Team_Cat, item);
            }
            else
            {
                oobitem->Cleanup();
                delete oobitem;
                oobitem = NULL;
            }
        }
    }

    return(oobitem);
}

C_Base *AddItemToOOB(CampEntity entity)
{
    TREELIST *Team_Cat, *item, *subcat;
    CampEntity Base;
    C_Entity  *BaseInfo;
    C_Base *oobitem = NULL;
    C_Text *txt;
    Unit upar;
    long TeamID, Cat, Type, idx, DivID;

    TeamID = (entity->GetTeam() << 24);

    if (entity->IsUnit())
        Cat = FindUnitCategory((Unit)entity);
    else if (entity->IsObjective())
        Cat = GetObjectiveCategory((Objective)entity);
    else
        Cat = 0;

    if ( not Cat)
        return(NULL);

    TeamID or_eq Cat;
    Team_Cat = gOOBTree->Find(TeamID);

    if (Team_Cat)
    {
        item = gOOBTree->Find(entity->GetCampID());

        if (item)
            return(item->Item_);

        oobitem = BuildOOBItem(entity);

        if (oobitem)
        {
            if (gOOB_Visible[entity->GetOwner()]) // MUST be GetOwner not GetTeam
                oobitem->SetFlagBitOff(C_BIT_INVISIBLE);
            else
                oobitem->SetFlagBitOn(C_BIT_INVISIBLE);

            if (Cat not_eq OOB_OBJECTIVE)
            {
                if (entity->IsSquadron())
                {
                    BaseInfo = NULL;
                    Base = ((Squadron)entity)->GetUnitAirbase();

                    if (Base and Base not_eq entity)
                    {
                        subcat = gOOBTree->Find(Base->GetCampID());

                        if ( not subcat)
                        {
                            BaseInfo = (C_Entity*)AddItemToOOB(Base);

                            if (BaseInfo)
                                subcat = BaseInfo->GetOwner();
                            else
                            {
                                oobitem->Cleanup();
                                delete oobitem;
                                return(NULL);
                            }
                        }
                    }
                    else
                        subcat = Team_Cat;
                }
                else if (entity->IsBattalion())
                {
                    upar = ((Unit)entity)->GetUnitParent();

                    if (upar)
                    {
                        subcat = gOOBTree->Find(upar->GetCampID());

                        if ( not subcat)
                        {
                            BaseInfo = (C_Entity*)AddItemToOOB(upar);

                            if (BaseInfo)
                                subcat = BaseInfo->GetOwner();
                            else
                            {
                                oobitem->Cleanup();
                                delete oobitem;
                                return(NULL);
                            }
                        }
                    }
                    else
                    {
                        DivID = ((Unit)entity)->GetUnitDivision();

                        if (DivID)
                        {
                            subcat = gOOBTree->Find((entity->GetTeam() << 24) bitor DivID bitor UR_DIVISION);

                            if ( not subcat)
                            {
                                BaseInfo = AddDivisionToOOB(GetDivisionByUnit((Unit)entity));

                                if (BaseInfo)
                                    subcat = BaseInfo->GetOwner();
                                else
                                {
                                    oobitem->Cleanup();
                                    delete oobitem;
                                    return(NULL);
                                }
                            }
                        }
                        else
                            subcat = Team_Cat;
                    }
                }
                else if (entity->IsBrigade())
                {
                    DivID = ((Unit)entity)->GetUnitDivision();

                    if (DivID)
                    {
                        subcat = gOOBTree->Find((entity->GetTeam() << 24) bitor DivID bitor UR_DIVISION);

                        if ( not subcat)
                        {
                            BaseInfo = AddDivisionToOOB(GetDivisionByUnit((Unit)entity));

                            if (BaseInfo)
                                subcat = BaseInfo->GetOwner();
                            else
                            {
                                oobitem->Cleanup();
                                delete oobitem;
                                return(NULL);
                            }
                        }
                    }
                    else
                        subcat = Team_Cat;
                }
                else
                    subcat = Team_Cat;
            }
            else if (Cat == OOB_OBJECTIVE)
            {
                idx = FindObjectiveIndex((Objective)entity);

                if (idx >= 0)
                {
                    Type = ObjectiveFilters[idx].UIType;
                    subcat = gOOBTree->Find((entity->GetTeam() << 24) bitor Type);

                    if ( not subcat)
                    {
                        txt = new C_Text;
                        txt->Setup(Type, 0);
                        txt->SetFont(gOOBTree->GetFont());
                        txt->SetText(ObjectiveCategoryNames[FindTypeIndex(Type, OBJ_TypeList, _MAP_NUM_OBJ_TYPES_)]);
                        subcat = gOOBTree->CreateItem((entity->GetTeam() << 24) bitor Type, C_TYPE_MENU, txt);
                        gOOBTree->AddChildItem(Team_Cat, subcat);
                    }
                }
                else
                    subcat = Team_Cat;
            }
            else
                subcat = Team_Cat;

            item = gOOBTree->CreateItem(oobitem->GetID(),/* oobitem->GetType() */ C_TYPE_ITEM, oobitem);

            if (item)
            {
                if (oobitem->_GetCType_() == _CNTL_ENTITY_)
                {
                    ((C_Entity*)oobitem)->SetOwner(item);
                    ((C_Entity*)oobitem)->SetFont(gOOBTree->GetFont());
                }
                else if (oobitem->_GetCType_() == _CNTL_SQUAD_)
                {
                    ((C_Squadron*)oobitem)->SetOwner(item);
                    ((C_Squadron*)oobitem)->SetFont(gOOBTree->GetFont());
                }

                gOOBTree->AddChildItem(subcat, item);
            }
            else
            {
                oobitem->Cleanup();
                delete oobitem;
                oobitem = NULL;
            }
        }
    }

    return(oobitem);
}

void MoveOOBSquadron(Squadron sqd, C_Squadron *Squadron)
{
    TREELIST *item = NULL;
    TREELIST *newloc = NULL;
    CampEntity Base = NULL;
    C_Entity *BaseInfo = NULL;
    F4CSECTIONHANDLE *Leave;


    item = Squadron->GetOwner();

    if ( not item)
        return;

    Leave = UI_Enter(gOOBTree->GetParent());

    Base = sqd->GetUnitAirbase();

    if (Base)
    {
        newloc = gOOBTree->Find(Base->GetCampID());

        if ( not newloc)
        {
            BaseInfo = (C_Entity*)AddItemToOOB(Base);

            if (BaseInfo)
                newloc = BaseInfo->GetOwner();
            else
                newloc = gOOBTree->Find((sqd->GetTeam() << 24) bitor OOB_AIRFORCE);
        }
    }
    else
        newloc = gOOBTree->Find((sqd->GetTeam() << 24) bitor OOB_AIRFORCE);

    if (newloc and newloc not_eq item->Parent)
    {
        gOOBTree->MoveChildItem(newloc, item);

        if (BaseInfo)
            Squadron->SetBaseID(BaseInfo->GetID());
        else
            Squadron->SetBaseID(0);
    }

    UI_Leave(Leave);
}

void FindIcon(UI_Refresher *urec)
{
    if (urec->MapItem_)
    {
        gMapMgr->AddToCurIcons(urec->MapItem_);
        gMapMgr->CenterOnIcon(urec->MapItem_);
    }
}

void FindMapIcon(long CampID)
{
    UI_Refresher *urec;

    urec = (UI_Refresher *)gGps->Find(CampID);

    if (urec)
        FindIcon(urec);
}

void OOBFindCB(long, short hittype, C_Base *)
{
    CampEntity ent;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    if (gSelectedEntity not_eq FalconNullId)
    {
        ent = (CampEntity)vuDatabase->Find(gSelectedEntity);

        if (ent)
        {
            if (ent->IsSquadron())
            {
                FindMapIcon(ent->GetCampID());
            }
            else if (ent->IsObjective())
            {
                FindMapIcon(ent->GetCampID());
            }
            else if (ent->IsTaskForce())
            {
                FindMapIcon(ent->GetCampID());
            }
            else if (ent->IsUnit())
            {
                FindMapIcon(ent->GetCampID());
            }
        }
    }
    else if (gSelectedDivision)
    {
        UI_Refresher *urec;
        urec = (UI_Refresher *)gGps->Find(gSelectedDivision bitand 0x00ffffff); // strip off team (incase it is a division)

        if (urec and urec->GetType() == GPS_DIVISION)
            FindIcon(urec); // Map bitand Tree save team # in top 8 bits (Needed to find division by team)
    }
}

void OOBInfoCB(long, short hittype, C_Base *)
{
    CampEntity ent;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    if (gSelectedEntity not_eq FalconNullId)
    {
        ent = (CampEntity)vuDatabase->Find(gSelectedEntity);

        if (ent)
        {
            if (ent->IsSquadron())
            {
                SetupSquadronInfoWindow(gSelectedEntity);
            }
            else if (ent->IsObjective())
            {
                BuildSpecificTargetList(gSelectedEntity);
            }
            else if (ent->IsTaskForce())
            {
                BuildSpecificTargetList(gSelectedEntity);
            }
            else if (ent->IsUnit())
            {
                SetupUnitInfoWindow(gSelectedEntity);
            }
        }
    }
    else if (gSelectedDivision)
    {
        UI_Refresher *urec;
        urec = (UI_Refresher *)gGps->Find(gSelectedDivision bitand 0x00ffffff); // strip off team (incase it is a division)

        if (urec and urec->GetType() == GPS_DIVISION)
            SetupDivisionInfoWindow(urec->GetDivID(), urec->GetSide()); // Map bitand Tree save team # in top 8 bits (Needed to find division by team)
    }
}
