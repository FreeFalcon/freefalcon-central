// Dogfight Menus
//
//

#include <windows.h>
#include "falclib.h"
#include "f4vu.h"
#include "falcent.h"
#include "chandler.h"
#include "ui95_ext.h"
#include "uicomms.h"
#include "dfcomms.h"
#include "ui_dgfgt.h"
#include "userids.h"
#include "textids.h"
#include "dogfight.h"
#include "Flight.h"
#include "ClassTbl.h"
#include "ACSelect.h"

void AreYouSure(long TitleID, long MessageID, void (*OkCB)(long, short, C_Base*), void (*CancelCB)(long, short, C_Base*));
void AreYouSure(long TitleID, _TCHAR *text, void (*OkCB)(long, short, C_Base*), void (*CancelCB)(long, short, C_Base*));
void CloseWindowCB(long ID, short hittype, C_Base *control);
uchar GetPlaneListID(long ID);
void MovePlayerAround(FalconSessionEntity *entity);
void RemovePlayerFromGame(VU_ID player);
void CheckDelButtons();

extern void LoadDfPlanes();
extern DF_AIRPLANE_TYPE *DFAIPlanes;
extern VU_ID gCurrentFlightID;
extern short gCurrentAircraftNum;

void HookupSelectACTypes(C_PopupList *menu, void (*cb)(long, short, C_Base *))
{
    int i;

    LoadDfPlanes();

    if (menu)
    {
        // Add Plane Types to menu
        i = 0;

        while (DFAIPlanes[i].ID)
        {
            if (gUI_Tracking_Flag bitand _UI_TRACK_FLAG02)
                menu->AddItem(DFAIPlanes[i].ID, C_TYPE_RADIO, DFAIPlanes[i].TextID, MID_DF_TYPE);

            menu->SetCallback(DFAIPlanes[i].ID, cb);
            i++;
        }
    }
}

// Kevin, don't fuck with these anymore
static void SetACTypeCB(long ID, short hittype, C_Base *)
{
    C_PopupList *mainmenu = NULL;
    uchar team = 0, skill = 0;
    int idx = 0, type = 0;
    C_TreeList   *tree = NULL;
    TREELIST     *item = NULL;
    C_Dog_Flight *dfflight = NULL;
    Flight        flt = NULL;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    LoadDfPlanes();

    if (gPopupMgr->GetCallingType() == C_TYPE_CONTROL)
    {
        tree = (C_TreeList*)gPopupMgr->GetCallingControl();

        if (tree)
        {
            item = tree->GetLastItem();

            if (item)
            {
                dfflight = (C_Dog_Flight*)item->Item_;

                if (dfflight)
                {
                    flt = (Flight)vuDatabase->Find(dfflight->GetVUID());
                    idx = GetPlaneListID(ID);
                    type = GetClassID(DOMAIN_AIR, CLASS_UNIT, TYPE_FLIGHT, DFAIPlanes[idx].UnitSType, DFAIPlanes[idx].SPType, VU_ANY, VU_ANY, VU_ANY) + VU_LAST_ENTITY_TYPE;
                    RequestTypeChange(flt, type);
                }
            }
        }
    }
    else // No previous item selected... find which client
    {
        mainmenu = gPopupMgr->GetCurrent();

        if (mainmenu)
        {
            if (mainmenu->GetItemState(DF_SKILL_NOVICE))
                skill = 0;
            else if (mainmenu->GetItemState(DF_SKILL_CADET))
                skill = 1;
            else if (mainmenu->GetItemState(DF_SKILL_ROOKIE))
                skill = 2;
            else if (mainmenu->GetItemState(DF_SKILL_VETERAN))
                skill = 3;
            else if (mainmenu->GetItemState(DF_SKILL_ACE))
                skill = 4;

            if (gPopupMgr->GetCallingClient() == 4)
            {
                if (mainmenu->GetItemState(DF_MARK_CRIMSON))
                    team = 1;
                else if (mainmenu->GetItemState(DF_MARK_SHARK))
                    team = 2;
                else if (mainmenu->GetItemState(DF_MARK_VIPER))
                    team = 3;
                else if (mainmenu->GetItemState(DF_MARK_TIGER))
                    team = 4;
                else team = 1;
            }
            else
            {
                switch (gPopupMgr->GetCallingClient())
                {
                    case 0:
                        team = 1;
                        break;

                    case 1:
                        team = 2;
                        break;

                    case 2:
                        team = 3;
                        break;

                    case 3:
                        team = 4;
                        break;

                    default:
                        team = 1;
                        break;
                }
            }

            idx = GetPlaneListID(ID);
            type = GetClassID(DOMAIN_AIR, CLASS_UNIT, TYPE_FLIGHT, DFAIPlanes[idx].UnitSType, DFAIPlanes[idx].SPType, VU_ANY, VU_ANY, VU_ANY) + VU_LAST_ENTITY_TYPE;
            RequestACSlot(NULL, team, 0, skill, type, 0);
        }
    }

    gPopupMgr->CloseMenu();
}

static void TeamJoinCB(long, short hittype, C_Base *)
{
    C_TreeList   *tree;
    TREELIST     *item;
    C_Dog_Flight *dfflight;
    C_Pilot      *pilot;
    Flight        flt;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    LoadDfPlanes();

    if (gPopupMgr->GetCallingType() == C_TYPE_CONTROL)
    {
        tree = (C_TreeList*)gPopupMgr->GetCallingControl();

        if (tree)
        {
            item = tree->GetLastItem();

            if (item)
            {
                if (item->Item_)
                {
                    if (item->Item_->_GetCType_() == _CNTL_DOG_FLIGHT_)
                    {
                        dfflight = (C_Dog_Flight*)item->Item_;

                        if (dfflight)
                        {
                            flt = (Flight)vuDatabase->Find(dfflight->GetVUID());

                            if (flt)
                            {
                                RequestACSlot(flt, flt->GetTeam(), 0, 0, 0, 1);
                            }
                        }
                    }
                    else if (item->Item_->_GetCType_() == _CNTL_PILOT_)
                    {
                        pilot = (C_Pilot*)item->Item_;

                        if (pilot)
                        {
                            flt = (Flight)vuDatabase->Find(pilot->GetVUID());

                            if (flt)
                            {
                                RequestACSlot(flt, flt->GetTeam(), 0, 0, 0, 1);
                            }
                        }
                    }
                }
            }
        }
    }
    else
    {
        long idx, type;

        idx = GetPlaneListID(DF_AC_F16C);
        type = GetClassID(DOMAIN_AIR, CLASS_UNIT, TYPE_FLIGHT, DFAIPlanes[idx].UnitSType, DFAIPlanes[idx].SPType, VU_ANY, VU_ANY, VU_ANY) + VU_LAST_ENTITY_TYPE;

        switch (gPopupMgr->GetCallingClient())
        {
            case 0:
                RequestACSlot(NULL, 1, 0, 0, type, 1);
                break;

            case 1:
                RequestACSlot(NULL, 2, 0, 0, type, 1);
                break;

            case 2:
                RequestACSlot(NULL, 3, 0, 0, type, 1);
                break;

            case 3:
                RequestACSlot(NULL, 4, 0, 0, type, 1);
                break;

            case 4:
                RequestACSlot(NULL, 1, 0, 0, type, 1);
                break;
        }
    }

    gPopupMgr->CloseMenu();
}

static void DeleteFlightCB(long, short hittype, C_Base *)
{
    C_TreeList   *tree;
    TREELIST     *item;
    C_Dog_Flight *dfflight;
    Flight        flt;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    tree = (C_TreeList*)gPopupMgr->GetCallingControl();

    if (tree)
    {
        item = tree->GetLastItem();

        if (item)
        {
            dfflight = (C_Dog_Flight*)item->Item_;

            if (dfflight)
            {
                flt = (Flight)vuDatabase->Find(dfflight->GetVUID());

                if ( not flt->IsPlayer())
                    RequestFlightDelete(flt);
            }
        }
    }

    gPopupMgr->CloseMenu();
}

static void DeletePilotCB(long, short hittype, C_Base *)
{
    C_TreeList *tree;
    TREELIST   *item;
    C_Pilot    *pilot;
    Flight      flt;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    tree = (C_TreeList*)gPopupMgr->GetCallingControl();

    if (tree)
    {
        item = tree->GetLastItem();

        if (item)
        {
            pilot = (C_Pilot*)item->Item_;

            if (pilot and not pilot->GetPlayer())
            {
                flt = (Flight)vuDatabase->Find(pilot->GetVUID());

                if (flt)
                    LeaveACSlot(flt, static_cast<uchar>(pilot->GetSlot()));
            }
        }
    }

    gPopupMgr->CloseMenu();
    gCurrentAircraftNum = -1;
    CheckDelButtons();
}

static void AddAIToFlightCB(long ID, short hittype, C_Base *)
{
    C_TreeList   *tree;
    TREELIST     *item;
    C_Dog_Flight *dfflight;
    Flight        flt;
    uchar SkillLevel;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    switch (ID)
    {
        case DF_SKILL_CADET:
            SkillLevel = 1;
            break;

        case DF_SKILL_ROOKIE:
            SkillLevel = 2;
            break;

        case DF_SKILL_VETERAN:
            SkillLevel = 3;
            break;

        case DF_SKILL_ACE:
            SkillLevel = 4;
            break;

        default:
            SkillLevel = 0;
            break;
    }

    tree = (C_TreeList*)gPopupMgr->GetCallingControl();

    if (tree)
    {
        item = tree->GetLastItem();

        if (item)
        {
            dfflight = (C_Dog_Flight*)item->Item_;

            if (dfflight)
            {
                flt = (Flight)vuDatabase->Find(dfflight->GetVUID());

                if (flt)
                {
                    RequestACSlot(flt, flt->GetTeam(), 0, SkillLevel, 0, 0);
                }
            }
        }
    }

    gPopupMgr->CloseMenu();
}

static void AddAIPilotCB(long ID, short hittype, C_Base *)
{
    C_TreeList   *tree;
    TREELIST     *item, *child;
    C_Dog_Flight *dfflight;
    Flight        flt;
    uchar SkillLevel;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    switch (ID)
    {
        case DF_SKILL_CADET:
            SkillLevel = 1;
            break;

        case DF_SKILL_ROOKIE:
            SkillLevel = 2;
            break;

        case DF_SKILL_VETERAN:
            SkillLevel = 3;
            break;

        case DF_SKILL_ACE:
            SkillLevel = 4;
            break;

        default:
            SkillLevel = 0;
            break;
    }

    tree = (C_TreeList*)gPopupMgr->GetCallingControl();

    if (tree)
    {
        child = tree->GetLastItem();

        if (child)
        {
            item = child->Parent;

            if (item)
            {
                dfflight = (C_Dog_Flight*)item->Item_;

                if (dfflight)
                {
                    flt = (Flight)vuDatabase->Find(dfflight->GetVUID());

                    if (flt)
                    {
                        RequestACSlot(flt, flt->GetTeam(), 0, SkillLevel, 0, 0);
                    }
                }
            }
        }
    }

    gPopupMgr->CloseMenu();
}

static void SetAIPilotSkillCB(long ID, short hittype, C_Base *)
{
    C_TreeList   *tree;
    TREELIST     *item, *child;
    C_Dog_Flight *dfflight;
    C_Pilot      *pilot;
    Flight        flt;
    short SkillLevel;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    switch (ID)
    {
        case DF_SET_SKILL_CADET:
            SkillLevel = 1;
            break;

        case DF_SET_SKILL_ROOKIE:
            SkillLevel = 2;
            break;

        case DF_SET_SKILL_VETERAN:
            SkillLevel = 3;
            break;

        case DF_SET_SKILL_ACE:
            SkillLevel = 4;
            break;

        default:
            SkillLevel = 0;
            break;
    }

    tree = (C_TreeList*)gPopupMgr->GetCallingControl();

    if (tree)
    {
        child = tree->GetLastItem();

        if (child)
        {
            pilot = (C_Pilot*)child->Item_;

            item = child->Parent;

            if (item and pilot)
            {
                dfflight = (C_Dog_Flight*)item->Item_;

                if (dfflight)
                {
                    flt = (Flight)vuDatabase->Find(dfflight->GetVUID());

                    if (flt)
                        RequestSkillChange(flt, pilot->GetSlot(), SkillLevel);
                }
            }
        }
    }

    gPopupMgr->CloseMenu();
}

static void SetFlightLeadCB(long, short hittype, C_Base *)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    gPopupMgr->CloseMenu();
}

void DogfightMenuSetup()
{
    C_PopupList *menu;

    switch (SimDogfight.GetGameType())
    {
        case dog_Furball:
            menu = gPopupMgr->GetMenu(DF_FLIGHT_POPUP);

            if (menu)
            {
                menu->SetItemFlagBitOff(MID_DF_ADD, C_BIT_ENABLED);
                menu->SetItemFlagBitOff(MID_DF_JOIN, C_BIT_ENABLED);
                menu->SetItemFlagBitOn(MID_DF_MARKINGS, C_BIT_ENABLED);
            }

            menu = gPopupMgr->GetMenu(DF_AI_PILOT_POPUP);

            if (menu)
            {
                menu->SetItemFlagBitOff(MID_DF_TYPE, C_BIT_ENABLED);
                menu->SetItemFlagBitOff(MID_DF_JOIN, C_BIT_ENABLED);
                menu->SetItemFlagBitOff(MID_DF_LEAD, C_BIT_ENABLED);
            }

            menu = gPopupMgr->GetMenu(DF_PLAYER_POPUP);

            if (menu)
            {
                menu->SetItemFlagBitOff(MID_DF_TYPE, C_BIT_ENABLED);
                menu->SetItemFlagBitOff(MID_DF_JOIN, C_BIT_ENABLED);
                menu->SetItemFlagBitOff(MID_DF_LEAD, C_BIT_ENABLED);
            }

            break;

        case dog_TeamFurball:
        case dog_TeamMatchplay:
            menu = gPopupMgr->GetMenu(DF_FLIGHT_POPUP);

            if (menu)
            {
                menu->SetItemFlagBitOn(MID_DF_ADD, C_BIT_ENABLED);
                menu->SetItemFlagBitOn(MID_DF_JOIN, C_BIT_ENABLED);
                menu->SetItemFlagBitOff(MID_DF_MARKINGS, C_BIT_ENABLED);
            }

            menu = gPopupMgr->GetMenu(DF_AI_PILOT_POPUP);

            if (menu)
            {
                menu->SetItemFlagBitOn(MID_DF_TYPE, C_BIT_ENABLED);
                menu->SetItemFlagBitOn(MID_DF_JOIN, C_BIT_ENABLED);
                menu->SetItemFlagBitOn(MID_DF_LEAD, C_BIT_ENABLED);
            }

            menu = gPopupMgr->GetMenu(DF_PLAYER_POPUP);

            if (menu)
            {
                menu->SetItemFlagBitOn(MID_DF_TYPE, C_BIT_ENABLED);
                menu->SetItemFlagBitOn(MID_DF_JOIN, C_BIT_ENABLED);
                menu->SetItemFlagBitOn(MID_DF_LEAD, C_BIT_ENABLED);
            }

            break;
    }
}

void CheckForPlayerCB(C_Base *themenu, C_Base *caller)
{
    C_TreeList *tree;
    TREELIST *item;
    C_Dog_Flight *dfflight;
    Flight flt;
    C_PopupList *menu;

    if ( not themenu or not caller)
        return;

    menu = (C_PopupList*)themenu;

    if (gPopupMgr->GetCallingType() == C_TYPE_CONTROL)
    {
        tree = (C_TreeList*)gPopupMgr->GetCallingControl();

        if (tree)
        {
            item = tree->GetLastItem();

            if (item)
            {
                dfflight = (C_Dog_Flight*)item->Item_;

                if (dfflight)
                {
                    flt = (Flight)vuDatabase->Find(dfflight->GetVUID());

                    if (flt)
                    {
                        if (0) //me123 flt->IsSetFalcFlag(FEC_HASPLAYERS))
                            menu->SetItemFlagBitOff(MID_DF_TYPE, C_BIT_ENABLED);
                        else
                            menu->SetItemFlagBitOn(MID_DF_TYPE, C_BIT_ENABLED);
                    }
                }
            }
        }
    }
}

void HookupDogFightMenus()
{
    C_PopupList *menu;

    menu = gPopupMgr->GetMenu(DF_FURBALL_NEW_POPUP);

    if (menu)
    {
        HookupSelectACTypes(menu, SetACTypeCB);
    }

    menu = gPopupMgr->GetMenu(DF_TEAM_NEW_POPUP);

    if (menu)
    {
        HookupSelectACTypes(menu, SetACTypeCB);
        menu->SetCallback(MID_DF_JOIN, TeamJoinCB);
    }

    menu = gPopupMgr->GetMenu(DF_FLIGHT_POPUP);

    if (menu)
    {
        menu->SetOpenCallback(CheckForPlayerCB);
        HookupSelectACTypes(menu, SetACTypeCB); // Change flight type (IF NO player is in it)

        menu->SetCallback(MID_DF_DELETE, DeleteFlightCB); // Remove flight or all AI pilots if human in flight
        menu->SetCallback(DF_SKILL_NOVICE, AddAIToFlightCB); // add AI
        menu->SetCallback(DF_SKILL_CADET, AddAIToFlightCB); // add AI
        menu->SetCallback(DF_SKILL_ROOKIE, AddAIToFlightCB); // add AI
        menu->SetCallback(DF_SKILL_VETERAN, AddAIToFlightCB); // add AI
        menu->SetCallback(DF_SKILL_ACE, AddAIToFlightCB); // add AI
        menu->SetCallback(MID_DF_JOIN, TeamJoinCB);
    }

    menu = gPopupMgr->GetMenu(DF_AI_PILOT_POPUP);

    if (menu)
    {
        menu->SetCallback(MID_DF_DELETE, DeletePilotCB); // Remove AI pilot
        menu->SetCallback(DF_SKILL_NOVICE, AddAIPilotCB); // add AI
        menu->SetCallback(DF_SKILL_CADET, AddAIPilotCB); // add AI
        menu->SetCallback(DF_SKILL_ROOKIE, AddAIPilotCB); // add AI
        menu->SetCallback(DF_SKILL_VETERAN, AddAIPilotCB); // add AI
        menu->SetCallback(DF_SKILL_ACE, AddAIPilotCB); // add AI
        menu->SetCallback(MID_DF_JOIN, TeamJoinCB);
        menu->SetCallback(DF_SET_SKILL_NOVICE, SetAIPilotSkillCB); // add AI
        menu->SetCallback(DF_SET_SKILL_CADET, SetAIPilotSkillCB); // add AI
        menu->SetCallback(DF_SET_SKILL_ROOKIE, SetAIPilotSkillCB); // add AI
        menu->SetCallback(DF_SET_SKILL_VETERAN, SetAIPilotSkillCB); // add AI
        menu->SetCallback(DF_SET_SKILL_ACE, SetAIPilotSkillCB); // add AI
        menu->SetCallback(MID_DF_LEAD, SetFlightLeadCB);
    }

    menu = gPopupMgr->GetMenu(DF_PLAYER_POPUP);

    if (menu)
    {
        menu->SetCallback(MID_DF_DELETE, DeletePilotCB); // Remove AI pilot
        menu->SetCallback(DF_SKILL_NOVICE, AddAIPilotCB); // add AI
        menu->SetCallback(DF_SKILL_CADET, AddAIPilotCB); // add AI
        menu->SetCallback(DF_SKILL_ROOKIE, AddAIPilotCB); // add AI
        menu->SetCallback(DF_SKILL_VETERAN, AddAIPilotCB); // add AI
        menu->SetCallback(DF_SKILL_ACE, AddAIPilotCB); // add AI
        menu->SetCallback(MID_DF_JOIN, TeamJoinCB);
        menu->SetCallback(MID_DF_LEAD, SetFlightLeadCB);
    }
}
