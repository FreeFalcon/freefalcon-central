//
// Unit info stuff
//
//
#include <windows.h>
#include "vu2.h"
#include "falcsess.h"
#include "campbase.h"
#include "camplist.h"
#include "campstr.h"
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
#include "uicomms.h"
#include "userids.h"
#include "textids.h"
#include "teamdata.h"
#include "logbook.h"
#include "remotelb.h"
#include "find.h"
#include "gps.h"
#include "tac_class.h"
#include "te_defs.h"
#include "F4version.h"

extern C_Map *gMapMgr;
extern C_Handler *gMainHandler;
extern GlobalPositioningSystem *gGps;

void DeleteGroupList(long ID);
_TCHAR *AddCommas(_TCHAR *buf);
void SetupFlightSpecificControls(Flight flt);
void recalculate_waypoints(WayPointClass *wp);
void tactical_set_orders(Battalion bat, VU_ID obj, GridIndex tx, GridIndex ty);
void FindMapIcon(long ID);
extern void ForeignToUpper(_TCHAR *buffer);

static long VehList[256][2]; // Max 16, [0]=ID,[1]=Count
static short Count;

VU_ID gLastSquadron = FalconNullId;
extern VU_ID gActiveFlightID, gLoadoutFlightID, gSelectedFlightID; // 2001-10-25 M.N. Added gSelectedFlightID

int gDragWPNum = 0;

static void ClearUnitVehicles()
{
    int i;

    for (i = 0; i < 256; i++)
    {
        VehList[i][0] = 0;
        VehList[i][1] = 0;
    }

    Count = 0;
}

static void TallyUnitVehicles(Unit un)
{
    int i, j, ID;

    if ( not un)
        return;

    for (i = 0; i < 16; i++)
    {
        ID = un->GetVehicleID(i);

        if (ID)
        {
            for (j = 0; j < Count and Count < 256; j++)
                if (VehList[j][0] == ID)
                {
                    VehList[j][1] += un->GetNumVehicles(i);
                    ID = 0;
                }

            if (ID)
            {
                VehList[Count][0] = ID;
                VehList[Count++][1] = un->GetNumVehicles(i);
            }
        }
    }
}

void AddVehiclesToWindow(C_Window *win, long client)
{
    C_Text *txt;
    int i, y = 4;
    _TCHAR buffer[50];
    VehicleClassDataType *vc;

    for (i = 0; i < 16; i++)
    {
        if (VehList[i][0] and VehList[i][1])
        {
            vc = GetVehicleClassData(VehList[i][0]);

            if (vc)
            {
                txt = new C_Text;
                txt->Setup(C_DONT_CARE, 0);
                txt->SetXY(15, y);
                txt->SetClient(static_cast<short>(client));
                txt->SetFont(win->Font_);
                txt->SetFixedWidth(50);
                _stprintf(buffer, "%1ld %s", VehList[i][1], vc->Name);
                txt->SetText(buffer);
                txt->SetUserNumber(_UI95_DELGROUP_SLOT_, _UI95_DELGROUP_ID_);
                win->AddControl(txt);
                y += txt->GetH() + 2;
            }
        }
    }

    win->ScanClientArea(client);
    win->RefreshClient(client);
}

// Kludge
static long CVTRange(long Value, long MaxVal, long NumSteps)
{
    long step;

    if (NumSteps < 1)
        return(1);

    step = MaxVal / NumSteps;

    Value /= step;
    Value++;

    if (Value > NumSteps)
        Value = NumSteps;

    return(Value);
}

void SetupUnitInfoWindow(VU_ID unitID)
{
    C_Window *win;
    C_ListBox *lbox;
    C_EditBox *ebox;
    C_Text *txt;
    C_Line *line;
    C_Bitmap *bmp;
    UI_Refresher *urec;
    Objective obj;
    long Morale = 0;
    long Fatigue = 0;
    long Supply = 0;
    long Strength = 0; //,MaxStrength=0;
    long NumUnits = 0;
    long y, h;
    Unit un;
    Unit par;
    Division divpar;
    WayPoint wp;
    _TCHAR buffer[200];
    int i;
    F4CSECTIONHANDLE *Leave;

    win = gMainHandler->FindWindow(UNIT_WIN);

    if ( not win)
        return;

    un = (Unit)vuDatabase->Find(unitID);

    if ( not un)
        return;

    if ( not un->IsBattalion() and not un->IsBrigade())
        return;

    urec = (UI_Refresher*)gGps->Find(un->GetCampID());

    if ( not urec)
        return;

    Leave = UI_Enter(win);
    DeleteGroupList(UNIT_WIN);

    ClearUnitVehicles();

    if (un->IsBrigade())
    {
        par = un->GetFirstUnitElement();

        while (par)
        {
            Morale += par->GetUnitMorale();
            Fatigue += par->GetUnitFatigue();
            Supply += par->GetUnitSupply();
            Strength += (par->GetTotalVehicles() * 100) / par->GetFullstrengthVehicles();
            NumUnits++;
            TallyUnitVehicles(par);
            par = un->GetNextUnitElement();
        }

        if (NumUnits)
        {
            Morale /= NumUnits;
            Fatigue /= NumUnits;
            Supply /= NumUnits;
            Strength /= NumUnits;
        };
    }
    else
    {
        Morale = un->GetUnitMorale();
        Fatigue = un->GetUnitFatigue();
        Supply = un->GetUnitSupply();
        Strength = (un->GetTotalVehicles() * 100) / un->GetFullstrengthVehicles();
        TallyUnitVehicles(un);
    }

    txt = (C_Text*)win->FindControl(UNIT_TITLE);

    if (txt)
    {
        _TCHAR tmp[80], *sptr;
        // KCK: This is a better way of doing this, when taking into account localization
        un->GetName(tmp, 79, FALSE);
        sptr = _tcschr(tmp, ' ') + 1;
        _tcscpy(buffer, sptr);
        // txt->Refresh();
        // _tcscpy(buffer, un->GetUnitClassName());
        // _tcscat(buffer,_T(" "));
        // GetSizeName(un->GetDomain(),un->GetType(),tmp);
        // _tcscat(buffer,tmp);
        ForeignToUpper(buffer);
        txt->SetText(buffer);
        txt->Refresh();
    }

    bmp = (C_Bitmap*)win->FindControl(UNIT_ICON);

    if (bmp)
    {
        bmp->Refresh();

        if (urec->MapItem_ and urec->MapItem_->Icon)
        {
            bmp->SetImage(urec->MapItem_->Icon->GetImage());
            bmp->SetFlagBitOff(C_BIT_INVISIBLE);
        }
        else
            bmp->SetFlagBitOn(C_BIT_INVISIBLE);

        bmp->Refresh();
    }

    txt = (C_Text*)win->FindControl(UNIT_NAME);

    if (txt)
    {
        txt->Refresh();
        un->GetName(buffer, 40, FALSE);
        txt->SetText(buffer);
        txt->Refresh();
    }

    par = un->GetUnitParent();

    txt = (C_Text*)win->FindControl(UNIT_PARENT);

    if (txt)
    {
        txt->Refresh();

        if (un->IsBattalion())
        {
            par = un->GetUnitParent();

            if (par)
            {
                par->GetName(buffer, 40, FALSE);
                txt->SetFlagBitOff(C_BIT_INVISIBLE);
                txt->SetText(buffer);
            }
            else
                txt->SetFlagBitOn(C_BIT_INVISIBLE);
        }
        else
            txt->SetFlagBitOn(C_BIT_INVISIBLE);
    }

    txt = (C_Text*)win->FindControl(UNIT_GRANDPARENT);

    if (txt)
    {
        txt->Refresh();

        if (par)
            divpar = GetDivisionByUnit(par);
        else
            divpar = GetDivisionByUnit(un);

        if (divpar)
        {
            divpar->GetName(buffer, 40, FALSE);
            txt->SetText(buffer);
            txt->SetFlagBitOff(C_BIT_INVISIBLE);
            txt->Refresh();
        }
        else
            txt->SetFlagBitOn(C_BIT_INVISIBLE);
    }

    /* KCK: Changed to text box
     lbox=(C_ListBox*)win->FindControl(UNIT_OWNER);
     if(lbox)
     {
     // Use flag to determine owner name to choose.
     // This way TE works. Ideally, this would be a
     // text box and we'd set it to the team name string
     lbox->SetValue(TeamInfo[un->GetOwner()]->GetFlag()+1);
     lbox->Refresh();
     }
    */
    txt = (C_Text*)win->FindControl(UNIT_OWNER);

    if (txt)
    {
        txt->SetText(TeamInfo[un->GetOwner()]->GetName());
    }

    lbox = (C_ListBox*)win->FindControl(UNIT_EXPERIENCE);

    if (lbox)
    {
        if (un->GetRClass() == RCLASS_AIR)
            lbox->SetValue(CVTRange(TeamInfo[un->GetTeam()]->airExperience - 60, 40, 5));
        else if (un->GetRClass() == RCLASS_NAVAL)
            lbox->SetValue(CVTRange(TeamInfo[un->GetTeam()]->navalExperience - 60, 40, 5));
        else if (un->GetRClass() == RCLASS_AIRDEFENSE)
            lbox->SetValue(CVTRange(TeamInfo[un->GetTeam()]->airDefenseExperience - 60, 40, 5));
        else
            lbox->SetValue(CVTRange(TeamInfo[un->GetTeam()]->groundExperience - 60, 40, 5));

        lbox->Refresh();
    }

    lbox = (C_ListBox*)win->FindControl(UNIT_SUPPLY);

    if (lbox)
    {
        lbox->SetValue(CVTRange(Supply, 100, 4));
        lbox->Refresh();
    }

    lbox = (C_ListBox*)win->FindControl(UNIT_MORALE);

    if (lbox)
    {
        lbox->SetValue(CVTRange(Morale, 100, 4));
        lbox->Refresh();
    }

    lbox = (C_ListBox*)win->FindControl(UNIT_FATIGUE);

    if (lbox)
    {
        lbox->SetValue(CVTRange(Fatigue, 100, 4));
        lbox->Refresh();
    }

    txt = (C_Text*)win->FindControl(UNIT_ETA);

    if (txt)
    {
        txt->Refresh();
        i = 1;
        wp = un->GetCurrentUnitWP();

        if (wp)
        {
            GetTimeString(wp->GetWPArrivalTime(), buffer);
            txt->SetText(buffer);
        }
        else
            txt->SetText(gStringMgr->GetString(TXT_NONE));

        txt->Refresh();
    }

    lbox = (C_ListBox*)win->FindControl(UNIT_FORMATION);

    if (lbox)
    {
        lbox->SetValue(un->GetUnitFormation() + 1);
        lbox->Refresh();
    }

    ebox = (C_EditBox*)win->FindControl(UNIT_OBJECTIVE);

    if (ebox)
    {
        // Orders part (Get from the old list box)
        int orders = un->GetUnitOrders();
        lbox = (C_ListBox*)win->FindControl(UNIT_ORDERS);

        if (lbox)
        {
            lbox->SetValue(orders + 1);
            _tcscpy(buffer, lbox->GetText());
        }

        // Objective name part
        obj = un->GetUnitObjective();

        if (obj)
        {
            _TCHAR tmp[80];
            obj->GetName(tmp, 20, FALSE);

            if (gLangIDNum >= F4LANG_SPANISH)
            {
                _tcscat(buffer, _T(" - "));
                _tcscat(buffer, tmp);
            }
            else if (gLangIDNum == F4LANG_GERMAN)
            {
                _tcscat(buffer, _T(": "));
                _tcscat(buffer, tmp);
            }
            else
            {
                _tcscat(buffer, _T(" "));
                _tcscat(buffer, tmp);
            }
        }

        ebox->SetText(gStringMgr->GetText(gStringMgr->AddText(buffer)));
        ebox->Refresh();
    }

    line = (C_Line*)win->FindControl(UNIT_STRENGTH);

    if (line)
    {
        // Strength
        line->Refresh();
        h = line->GetUserNumber(C_STATE_1) * Strength / 100;
        y = line->GetUserNumber(C_STATE_0) + line->GetUserNumber(C_STATE_1) - h;
        line->SetY(y);
        line->SetH(h);
        line->Refresh();
    }

    AddVehiclesToWindow(win, 0);

    gMainHandler->ShowWindow(win);
    gMainHandler->WindowToFront(win);
    UI_Leave(Leave);
}

void SetupDivisionInfoWindow(long DivID, short owner)
{
    C_Window *win;
    C_ListBox *lbox;
    C_EditBox *ebox;
    C_Text *txt;
    C_Line *line;
    C_Bitmap *bmp;
    UI_Refresher *urec;
    long Morale = 0;
    long Fatigue = 0;
    long Supply = 0;
    long Strength = 0; //,MaxStrength=0;
    long NumUnits = 0;
    long y, h;
    Unit un, tmpun;
    Unit par;
    Division div;
    _TCHAR buffer[200];
    F4CSECTIONHANDLE *Leave;

    win = gMainHandler->FindWindow(UNIT_WIN);

    if ( not win)
        return;

    div = GetFirstDivisionByCountry(owner);

    while (div and div->nid not_eq (DivID))
        div = GetNextDivisionByCountry(div, owner);

    if ( not div)
        return;

    urec = (UI_Refresher*)gGps->Find(div->nid bitor UR_DIVISION);

    if ( not urec)
        return;

    Leave = UI_Enter(win);
    DeleteGroupList(UNIT_WIN);

    ClearUnitVehicles();
    un = div->GetFirstUnitElement();
    par = un;

    while (par)
    {
        tmpun = par->GetFirstUnitElement();

        while (tmpun)
        {
            Morale += tmpun->GetUnitMorale();
            Fatigue += tmpun->GetUnitFatigue();
            Supply += tmpun->GetUnitSupply();
            Strength += (tmpun->GetTotalVehicles() * 100) / tmpun->GetFullstrengthVehicles();
            NumUnits++;
            TallyUnitVehicles(tmpun);
            tmpun = par->GetNextUnitElement();
        }

        par = div->GetNextUnitElement();
    }

    if (NumUnits)
    {
        Morale /= NumUnits;
        Fatigue /= NumUnits;
        Supply /= NumUnits;
        Strength /= NumUnits;
    }

    txt = (C_Text*)win->FindControl(UNIT_TITLE);

    if (txt)
    {
        _TCHAR tmp[80], *sptr;
        // KCK: This is a better way of doing this, when taking into account localization
        div->GetName(tmp, 79, FALSE);
        sptr = _tcschr(tmp, ' ') + 1;
        _tcscpy(buffer, sptr);
        // _tcscpy(buffer, un->GetUnitClassName());
        // _tcscat(buffer,_T(" "));
        // ReadIndexedString(613, tmp, 79);
        // _tcscat(buffer,tmp);
        ForeignToUpper(buffer);
        txt->SetText(buffer);
        txt->Refresh();
    }

    bmp = (C_Bitmap*)win->FindControl(UNIT_ICON);

    if (bmp)
    {
        bmp->Refresh();

        if (urec->MapItem_ and urec->MapItem_->Icon)
        {
            bmp->SetImage(urec->MapItem_->Icon->GetImage());
            bmp->SetFlagBitOff(C_BIT_INVISIBLE);
        }
        else
            bmp->SetFlagBitOn(C_BIT_INVISIBLE);

        bmp->Refresh();
    }


    txt = (C_Text*)win->FindControl(UNIT_NAME);

    if (txt)
    {
        txt->Refresh();
        div->GetName(buffer, 40, FALSE);
        txt->SetText(buffer);
        txt->Refresh();
    }

    par = un->GetUnitParent();

    txt = (C_Text*)win->FindControl(UNIT_PARENT);

    if (txt)
    {
        txt->Refresh();
        txt->SetFlagBitOn(C_BIT_INVISIBLE);
    }

    txt = (C_Text*)win->FindControl(UNIT_GRANDPARENT);

    if (txt)
    {
        txt->Refresh();
        txt->SetFlagBitOn(C_BIT_INVISIBLE);
    }

    /* KCK: Changed to text box
     lbox=(C_ListBox*)win->FindControl(UNIT_OWNER);
     if(lbox)
     {
     // Use flag to determine owner name to choose.
     // This way TE works. Ideally, this would be a
     // text box and we'd set it to the team name string
     lbox->SetValue(TeamInfo[owner]->GetFlag()+1);
     lbox->Refresh();
     }
    */

    txt = (C_Text*)win->FindControl(UNIT_OWNER);

    if (txt)
    {
        txt->SetText(TeamInfo[owner]->GetName());
    }

    lbox = (C_ListBox*)win->FindControl(UNIT_EXPERIENCE);

    if (lbox)
    {
        lbox->SetValue(CVTRange(TeamInfo[(DivID >> 24)]->groundExperience, 100, 5));
        lbox->Refresh();
    }

    lbox = (C_ListBox*)win->FindControl(UNIT_SUPPLY);

    if (lbox)
    {
        lbox->SetValue(CVTRange(Supply, 100, 4));
        lbox->Refresh();
    }

    lbox = (C_ListBox*)win->FindControl(UNIT_MORALE);

    if (lbox)
    {
        lbox->SetValue(CVTRange(Morale, 100, 4));
        lbox->Refresh();
    }

    lbox = (C_ListBox*)win->FindControl(UNIT_FATIGUE);

    if (lbox)
    {
        lbox->SetValue(CVTRange(Fatigue, 100, 4));
        lbox->Refresh();
    }

    txt = (C_Text*)win->FindControl(UNIT_ETA);

    if (txt)
    {
        txt->Refresh();
        txt->SetText(gStringMgr->GetString(TXT_NONE));
        txt->Refresh();
    }

    lbox = (C_ListBox*)win->FindControl(UNIT_ORDERS);

    if (lbox)
    {
        lbox->SetValue(0);
        lbox->Refresh();
    }

    lbox = (C_ListBox*)win->FindControl(UNIT_FORMATION);

    if (lbox)
    {
        lbox->SetValue(0);
        lbox->Refresh();
    }

    lbox = (C_ListBox*)win->FindControl(UNIT_TOE);

    if (lbox)
    {
        lbox->SetValue(un->GetTeam() + 1);
        lbox->Refresh();
    }

    ebox = (C_EditBox*)win->FindControl(UNIT_OBJECTIVE);

    if (ebox)
    {
        // Orders part (Get from the old list box)
        int orders = un->GetUnitOrders();
        lbox = (C_ListBox*)win->FindControl(UNIT_ORDERS);

        if (lbox)
        {
            lbox->SetValue(orders + 1);
            _tcscpy(buffer, lbox->GetText());
        }

        // Objective name part
        Objective obj = un->GetUnitObjective();

        if (obj)
        {
            _TCHAR tmp[80];
            obj->GetName(tmp, 20, FALSE);

            if (gLangIDNum >= F4LANG_SPANISH)
            {
                _tcscat(buffer, _T(" - "));
                _tcscat(buffer, tmp);
            }
            else if (gLangIDNum == F4LANG_GERMAN)
            {
                _tcscat(buffer, _T(": "));
                _tcscat(buffer, tmp);
            }
            else
            {
                _tcscat(buffer, _T(" "));
                _tcscat(buffer, tmp);
            }
        }

        ebox->SetText(gStringMgr->GetText(gStringMgr->AddText(buffer)));
        ebox->Refresh();
    }

    line = (C_Line*)win->FindControl(UNIT_STRENGTH);

    if (line)
    {
        // Strength
        line->Refresh();
        h = line->GetUserNumber(C_STATE_1) * Strength / 100;
        y = line->GetUserNumber(C_STATE_0) + line->GetUserNumber(C_STATE_1) - h;
        line->SetY(y);
        line->SetH(h);
        line->Refresh();
    }

    AddVehiclesToWindow(win, 0);

    gMainHandler->ShowWindow(win);
    gMainHandler->WindowToFront(win);
    UI_Leave(Leave);
}

static C_Base *priorpilot = NULL;

long ratingstr[] =
{
    TXT_PILOT_RATE_0,
    TXT_PILOT_RATE_1,
    TXT_PILOT_RATE_2,
    TXT_PILOT_RATE_3,
    TXT_PILOT_RATE_4,
};

void PickPilotCB(long, short hittype, C_Base *control)
{
    C_Bitmap *bmp;
    C_Text *txt;
    _TCHAR buffer[10];
    F4CSECTIONHANDLE *Leave;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    Leave = UI_Enter(control->Parent_);
    bmp = (C_Bitmap*)control->Parent_->FindControl(UNIT_PICTURE);

    if (bmp)
    {
        if (priorpilot)
        {
            priorpilot->SetState(0);
            priorpilot->Refresh();
        }

        control->SetState(1);
        control->Refresh();
        bmp->Refresh();

        if (control->GetUserNumber(C_STATE_0))
            bmp->SetImage(control->GetUserNumber(C_STATE_0));
        else
        {
            // Need to load a file
        }

        bmp->Refresh();
    }

    txt = (C_Text*)control->Parent_->FindControl(UNIT_NUM_MISSIONS);

    if (txt)
    {
        _stprintf(buffer, "%1d", control->GetUserNumber(C_STATE_2));
        txt->SetText(buffer);
        txt->Refresh();
    }

    txt = (C_Text*)control->Parent_->FindControl(UNIT_AVG_RATING);

    if (txt)
    {
        if (control->GetUserNumber(C_STATE_3))
            txt->SetText(ratingstr[(short)(((float)control->GetUserNumber(C_STATE_3) / 25.0f + 0.5f)) % 5]);
        else
            txt->SetText(TXT_NO_RATING);

        txt->Refresh();
    }

    txt = (C_Text*)control->Parent_->FindControl(UNIT_A_A_KILLS);

    if (txt)
    {
        txt->Refresh();
        _stprintf(buffer, "%1d", control->GetUserNumber(C_STATE_4));
        txt->SetText(buffer);
        txt->Refresh();
    }

    txt = (C_Text*)control->Parent_->FindControl(UNIT_A_G_KILLS);

    if (txt)
    {
        txt->Refresh();
        _stprintf(buffer, "%1d", control->GetUserNumber(C_STATE_5));
        txt->SetText(buffer);
        txt->Refresh();
    }

    txt = (C_Text*)control->Parent_->FindControl(UNIT_NAVAL_KILLS);

    if (txt)
    {
        txt->Refresh();
        _stprintf(buffer, "%1d", control->GetUserNumber(C_STATE_6));
        txt->SetText(buffer);
        txt->Refresh();
    }

    txt = (C_Text*)control->Parent_->FindControl(UNIT_STATIC_KILLS);

    if (txt)
    {
        txt->Refresh();
        _stprintf(buffer, "%1d", control->GetUserNumber(C_STATE_7));
        txt->SetText(buffer);
        txt->Refresh();
    }

    UI_Leave(Leave);
    priorpilot = control;
}

extern long gRanksTxt[NUM_RANKS];
long GetRank(_TCHAR *str)
{
    _TCHAR *rnk;
    long i;

    i = 0;

    while (i < NUM_RANKS)
    {
        rnk = gStringMgr->GetString(gRanksTxt[i]);

        if (rnk)
        {
            if ( not _tcsncicmp(rnk, str, _tcsclen(rnk)))
                return(i);
        }

        i++;
    }

    return(0);
}

void BuildPilotList(C_TreeList *tree, Squadron sqd)
{
    C_Button *btn;
    _TCHAR buffer[30];
    short i;
    long ID = 1;
    TREELIST *item;
    RemoteLB *lbptr;

    tree->DeleteBranch(tree->GetRoot());
    priorpilot = NULL;

    VuSessionsIterator sessionWalker(FalconLocalGame);
    FalconSessionEntity *session;

    session = (FalconSessionEntity*)sessionWalker.GetFirst();

    while (session)
    {
        if (session->GetPlayerSquadronID() == sqd->Id())
        {
            btn = new C_Button;

            if (btn)
            {
                btn->Setup(ID, C_TYPE_CUSTOM, 0, 0);
                btn->SetText(C_STATE_0, gStringMgr->GetText(gStringMgr->AddText(session->GetPlayerName())));
                btn->SetText(C_STATE_1, gStringMgr->GetText(gStringMgr->AddText(session->GetPlayerName())));
                btn->SetColor(C_STATE_0, 0xeeeeee);
                btn->SetColor(C_STATE_1, 0x00ff00);
                btn->SetCallback(PickPilotCB);

                if (session == FalconLocalSession)
                {
                    if (LogBook.GetPictureResource()) // Temporary
                        btn->SetUserNumber(C_STATE_0, LogBook.GetPictureResource()); // Image ID goes here
                    else
                    {
                        // Need to load a file
                        btn->SetUserNumber(C_STATE_0, NOFACE); // Image ID goes here
                    }
                }
                else
                {
                    lbptr = (RemoteLB*)gCommsMgr->GetRemoteLB(session->Id().creator_);

                    if (lbptr and lbptr->Pilot_.PictureResource)
                    {
                        btn->SetUserNumber(C_STATE_0, lbptr->Pilot_.PictureResource); // Image ID goes here
                    }
                    else
                        btn->SetUserNumber(C_STATE_0, NOFACE); // Image ID goes here
                }

                btn->SetUserNumber(C_STATE_2, session->GetMissions()); // Num Missions
                btn->SetUserNumber(C_STATE_3, session->GetRating()); // Mission Rating
                btn->SetUserNumber(C_STATE_4, session->GetKill(FalconSessionEntity::_AIR_KILLS_));
                btn->SetUserNumber(C_STATE_5, session->GetKill(FalconSessionEntity::_GROUND_KILLS_));
                btn->SetUserNumber(C_STATE_6, session->GetKill(FalconSessionEntity::_NAVAL_KILLS_));
                btn->SetUserNumber(C_STATE_7, session->GetKill(FalconSessionEntity::_STATIC_KILLS_));
                btn->SetUserNumber(20, GetRank(session->GetPlayerName()));
                item = tree->CreateItem(ID, C_TYPE_ITEM, btn);

                if (item)
                    tree->AddItem(tree->GetRoot(), item);

                ID++;
            }
        }

        session = (FalconSessionEntity*)sessionWalker.GetNext();
    }

    for (i = 0; i < PILOTS_PER_SQUADRON; i++)
    {
        // if(sqd->GetPilotData(i)->pilot_status == PILOT_IN_USE or sqd->GetPilotData(i)->pilot_status == PILOT_AVAILABLE)
        if (sqd->GetPilotData(i)->pilot_status not_eq PILOT_KIA)
        {
            GetPilotName(sqd->GetPilotData(i)->pilot_id, buffer, 25);
            btn = new C_Button;

            if (btn)
            {
                btn->Setup(ID, C_TYPE_CUSTOM, 0, 0);

                btn->SetText(C_STATE_0, gStringMgr->GetText(gStringMgr->AddText(buffer)));
                btn->SetText(C_STATE_1, gStringMgr->GetText(gStringMgr->AddText(buffer)));
                btn->SetColor(C_STATE_0, 0xeeeeee);
                btn->SetColor(C_STATE_1, 0x00ff00);
                btn->SetCallback(PickPilotCB);

                if (sqd->GetOwner() == FalconLocalSession->GetCountry())
                    btn->SetUserNumber(C_STATE_0, PilotImageIDs[PilotInfo[sqd->GetPilotData(i)->pilot_id].photo_id]); // Image ID goes here
                else
                    btn->SetUserNumber(C_STATE_0, FlagImageID[TeamInfo[sqd->GetOwner()]->GetFlag()][BIG_HORIZ]); // Image ID goes here

                btn->SetUserNumber(C_STATE_2, sqd->GetPilotData(i)->missions_flown); // Num Missions
                btn->SetUserNumber(C_STATE_3, sqd->GetPilotData(i)->GetPilotRating() * 25); // Mission Rating
                btn->SetUserNumber(C_STATE_4, sqd->GetPilotData(i)->aa_kills);
                btn->SetUserNumber(C_STATE_5, sqd->GetPilotData(i)->ag_kills);
                btn->SetUserNumber(C_STATE_6, sqd->GetPilotData(i)->an_kills);
                btn->SetUserNumber(C_STATE_7, sqd->GetPilotData(i)->as_kills);
                btn->SetUserNumber(20, GetRank(buffer));
                item = tree->CreateItem(ID, C_TYPE_ITEM, btn);

                if (item)
                    tree->AddItem(tree->GetRoot(), item);

                if (ID == 1)
                    PickPilotCB(ID, C_TYPE_LMOUSEUP, btn);

                ID++;
            }
        }
    }

    tree->RecalcSize();

    if (tree->Parent_)
        tree->Parent_->RefreshClient(tree->GetClient());
}

void PickSquadronStatsCB(long, short hittype, C_Base *control)
{
    C_Text *txt;
    C_Bitmap *bmp;
    _TCHAR buffer[10];

    F4CSECTIONHANDLE *Leave;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    Leave = UI_Enter(control->Parent_);

    bmp = (C_Bitmap*)control->Parent_->FindControl(UNIT_PICTURE);

    if (bmp)
    {
        bmp->SetImage(SquadronMatchIDs[control->GetUserNumber(10)][0]);
        bmp->Refresh();
    }

    txt = (C_Text*)control->Parent_->FindControl(UNIT_NUM_MISSIONS);

    if (txt)
    {
        _stprintf(buffer, "%1d", control->GetUserNumber(C_STATE_2));
        txt->SetText(buffer);
        txt->Refresh();
    }

    txt = (C_Text*)control->Parent_->FindControl(UNIT_AVG_RATING);

    if (txt)
    {
        if (control->GetUserNumber(C_STATE_2))
            txt->SetText(ratingstr[(short)(((float)control->GetUserNumber(C_STATE_3) / 25.0f + 0.5f)) % 5]);
        else
            txt->SetText(TXT_NO_RATING);

        txt->Refresh();
    }

    txt = (C_Text*)control->Parent_->FindControl(UNIT_A_A_KILLS);

    if (txt)
    {
        _stprintf(buffer, "%1d", control->GetUserNumber(C_STATE_4));
        txt->SetText(buffer);
        txt->Refresh();
    }

    txt = (C_Text*)control->Parent_->FindControl(UNIT_A_G_KILLS);

    if (txt)
    {
        _stprintf(buffer, "%1d", control->GetUserNumber(C_STATE_5));
        txt->SetText(buffer);
        txt->Refresh();
    }

    txt = (C_Text*)control->Parent_->FindControl(UNIT_NAVAL_KILLS);

    if (txt)
    {
        _stprintf(buffer, "%1d", control->GetUserNumber(C_STATE_6));
        txt->SetText(buffer);
        txt->Refresh();
    }

    txt = (C_Text*)control->Parent_->FindControl(UNIT_STATIC_KILLS);

    if (txt)
    {
        _stprintf(buffer, "%1d", control->GetUserNumber(C_STATE_7));
        txt->SetText(buffer);
        txt->Refresh();
    }

    UI_Leave(Leave);
}

void SquadronAirUnitCB(long ID, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    if (control)
    {
        control->Parent_->DisableCluster(control->GetUserNumber(C_STATE_1));
        PickSquadronStatsCB(ID, hittype, control);
        control->Parent_->EnableCluster(control->GetUserNumber(C_STATE_0));
        control->Parent_->RefreshWindow();
    }
}

void PilotAirUnitCB(long, short hittype, C_Base *control)
{
    C_TreeList *tree = NULL;
    C_Button *btn = NULL;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    if (control)
    {
        control->Parent_->DisableCluster(control->GetUserNumber(C_STATE_1));
        tree = (C_TreeList*)control->Parent_->FindControl(UNIT_PILOTLIST);

        if (tree)
        {
            if (tree->GetRoot())
                btn = (C_Button*)tree->GetRoot()->Item_;

            if (btn)
                PickPilotCB(btn->GetID(), hittype, btn);

            control->Parent_->EnableCluster(control->GetUserNumber(C_STATE_0));
            tree->RecalcSize();

            if (tree->Parent_)
                tree->Parent_->RefreshClient(tree->GetClient());
        }

        control->Parent_->RefreshWindow();
    }
}

void SquadronFindCB(long, short hittype, C_Base *)
{
    Squadron sqd;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    sqd = (Squadron)vuDatabase->Find(gLastSquadron);

    if (sqd and sqd->IsSquadron())
    {
        FindMapIcon(sqd->GetCampID());
    }
}

BOOL SortPilotByNameCB(TREELIST *list, TREELIST *newitem)
{
    C_Button *btn1, *btn2;

    if ( not list or not newitem)
        return(FALSE);

    if ( not list->Item_ or not newitem->Item_)
        return(FALSE);

    btn1 = (C_Button *)list->Item_;
    btn2 = (C_Button *)newitem->Item_;

    if (btn2->GetUserNumber(20) > btn1->GetUserNumber(20))
        return(TRUE);
    else if (btn2->GetUserNumber(20) == btn1->GetUserNumber(20))
    {
        if (_tcsicmp(btn2->GetText(0), btn1->GetText(0)) < 0)
            return(TRUE);
    }

    return(FALSE);
}

void SetupSquadronInfoWindow(VU_ID TheID)
{
    C_Window *win;
    C_Text *txt;
    C_ListBox *lbox;
    C_Button *btn;
    C_TreeList *tree;
    C_Bitmap *bmp;
    UI_Refresher *urec;
    Squadron sqd;
    CampEntity ent;
    VU_ID sqdID;
    Flight flt;
    long kills[4];
    _TCHAR buffer[200];
    short count, i;
    long total;
    int pilots;
    F4CSECTIONHANDLE *Leave;

    VuSessionsIterator sessionWalker(FalconLocalGame);
    FalconSessionEntity *session;

    ent = (CampEntity)vuDatabase->Find(TheID);

    if ( not ent)
        return;

    gLastSquadron = TheID;

    if (ent->IsFlight())
    {
        flt = (Flight)ent;
        sqdID = flt->GetUnitSquadronID();
        sqd = (Squadron)vuDatabase->Find(sqdID);

        if ( not sqd)
            return;

        pilots = sqd->NumActivePilots();
    }
    else if (ent->IsSquadron())
    {
        sqd = (Squadron)ent;
        pilots = sqd->NumActivePilots();
    }
    else
        return;

    urec = (UI_Refresher *)gGps->Find(sqd->GetCampID());

    if ( not urec)
        return;

    win = gMainHandler->FindWindow(AIR_UNIT_WIN);

    if (win)
    {
        Leave = UI_Enter(win);

        btn = (C_Button*)win->FindControl(UNIT_SQUADRON_TAB);

        if (btn)
        {
            btn->SetState(1);
            btn->Refresh();
        }

        btn = (C_Button*)win->FindControl(UNIT_PILOT_TAB);

        if (btn)
        {
            btn->SetState(0);
            btn->Refresh();
        }

        bmp = (C_Bitmap*)win->FindControl(UNIT_ICON);

        if (bmp)
        {
            bmp->Refresh();

            if (urec->MapItem_ and urec->MapItem_->Icon)
            {
                bmp->SetImage(urec->MapItem_->Icon->GetImage());
                bmp->SetFlagBitOff(C_BIT_INVISIBLE);
            }
            else
                bmp->SetFlagBitOn(C_BIT_INVISIBLE);

            bmp->Refresh();
        }

        bmp = (C_Bitmap*)win->FindControl(UNIT_PATCH);

        if (bmp)
        {
            bmp->SetImage(SquadronMatchIDs[sqd->GetPatchID()][0]);
            bmp->Refresh();
        }

        btn = (C_Button*)win->FindControl(UNIT_SQUADRON_TAB);

        if (btn)
        {
            win->DisableCluster(btn->GetUserNumber(C_STATE_1));
            win->EnableCluster(btn->GetUserNumber(C_STATE_0));
        }

        txt = (C_Text*)win->FindControl(UNIT_TITLE);

        if (txt)
        {
            _TCHAR tmp[80], *sptr;
            // KCK: This is a better way of doing this, when taking into account localization
            sqd->GetName(tmp, 79, FALSE);
            sptr = _tcschr(tmp, ' ') + 1;
            _tcscpy(buffer, sptr);
            // _tcscpy(buffer,sqd->GetUnitClassName());
            // _tcscat(buffer," ");
            // _tcscat(buffer,gStringMgr->GetString(TXT_SQUADRON));
            ForeignToUpper(buffer);
            txt->SetText(buffer);
        }

        txt = (C_Text*)win->FindControl(UNIT_NAME);

        if (txt)
        {
            sqd->GetName(buffer, 40, FALSE);
            txt->SetText(buffer);
        }

        txt = (C_Text*)win->FindControl(UNIT_OWNER);

        if (txt)
        {
            txt->SetText(TeamInfo[sqd->GetOwner()]->GetName());
        }

        txt = (C_Text*)win->FindControl(UNIT_ROLE);

        if (txt)
        {
            switch (sqd->GetUnitSpecialty())
            {
                case SQUADRON_SPECIALTY_AA:
                    txt->SetText(TXT_AIR_TO_AIR);
                    break;

                case SQUADRON_SPECIALTY_AG:
                    txt->SetText(TXT_AIR_TO_GROUND);
                    break;

                default:
                    txt->SetText(TXT_GENERAL);
                    break;
            }
        }

        txt = (C_Text*)win->FindControl(UNIT_NUM_AIRCRAFT);

        if (txt)
        {
            _stprintf(buffer, "%1d", sqd->GetTotalVehicles());
            txt->SetText(buffer);
        }

        txt = (C_Text*)win->FindControl(UNIT_NUM_PILOTS);

        if (txt)
        {
            _stprintf(buffer, "%1d", pilots);
            txt->SetText(buffer);
        }

        txt = (C_Text*)win->FindControl(UNIT_AIRCRAFT_LOSSES);

        if (txt)
        {
            _stprintf(buffer, "%1d", sqd->GetTotalLosses());
            txt->SetText(buffer);
        }

        txt = (C_Text*)win->FindControl(UNIT_PILOT_LOSSES);

        if (txt)
        {
            _stprintf(buffer, "%1d", sqd->GetPilotLosses());
            txt->SetText(buffer);
        }

        txt = (C_Text*)win->FindControl(UNIT_PLAYERS);

        if (txt)
        {
            count = 0;

            if (gCommsMgr->Online())
            {
                VuSessionsIterator sessionWalker(FalconLocalGame);
                FalconSessionEntity *session;

                session = (FalconSessionEntity*)sessionWalker.GetFirst();

                while (session)
                {
                    if (session->GetPlayerSquadronID() == sqd->Id())
                        count++;

                    session = (FalconSessionEntity*)sessionWalker.GetNext();
                }
            }
            else
            {
                if (FalconLocalSession->GetPlayerSquadronID() == sqd->Id())
                    count++;
            }

            _stprintf(buffer, "%1d", count);
            txt->SetText(buffer);
        }

        lbox = (C_ListBox*)win->FindControl(UNIT_EXPERIENCE);

        if (lbox)
        {
            count = 0;
            total = 0;

            for (i = 0; i < PILOTS_PER_SQUADRON; i++)
            {
                if (sqd->GetPilotData(i)->pilot_status == PILOT_IN_USE or sqd->GetPilotData(i)->pilot_status == PILOT_AVAILABLE)
                {
                    total += sqd->GetPilotData(i)->GetPilotSkill();
                    count++;
                }
            }

            if (count)
                // 2000-11-17 MODIFIED BY S.G. SO WE TAKE AN *AVERAGE* OF THE SKILL
                // total/=count;
                total = (long)((float)total / (float)count + 0.5F);

            lbox->SetValue(total + 1);
        }

        lbox = (C_ListBox*)win->FindControl(UNIT_MORALE);

        if (lbox)
        {
            // KCK: Base moral off pilot losses/active pilots..
            if (pilots > (PILOTS_PER_SQUADRON * 3) / 4)
                lbox->SetValue(MORALE_HIGH);
            else if (pilots > PILOTS_PER_SQUADRON / 2)
                lbox->SetValue(MORALE_NORMAL);
            else
                lbox->SetValue(MORALE_LOW);
        }

        txt = (C_Text*)win->FindControl(UNIT_RESUPPLY_DATE);

        if (txt)
        {
            CampaignTime time = (sqd->GetLastResupplyTime() + sqd->GetUnitSupplyTime());
            // Round to nearest hour
            time = (time / CampaignHours) * CampaignHours;
            GetTimeString(time, buffer, FALSE);
            txt->SetText(buffer);
        }

        txt = (C_Text*)win->FindControl(UNIT_LAST_AIRCRAFT_RECEIVED);

        if (txt)
        {
            _stprintf(buffer, "%d", sqd->GetLastResupply());
            txt->SetText(buffer);
        }

        lbox = (C_ListBox*)win->FindControl(UNIT_SUPPLY);

        if (lbox)
        {
            int supply = 100, need = sqd->GetUnitSupplyNeed(FALSE);

            if (need)
                supply = (sqd->GetUnitSupplyNeed(TRUE) * 100) / need;

            if (supply < 20)
                lbox->SetValue(SUPPLY_CRITICAL);
            else if (supply < 45)
                lbox->SetValue(SUPPLY_LOW);
            else if (supply < 65)
                lbox->SetValue(SUPPLY_NORMAL);
            else
                lbox->SetValue(SUPPLY_FULL);
        }

        btn = (C_Button*)win->FindControl(UNIT_AIRBASE);

        if (btn)
        {
            ent = sqd->GetUnitAirbase();

            if (ent)
            {
                if (ent->IsObjective())
                    ent->GetName(buffer, 40, TRUE);
                else
                    ent->GetName(buffer, 40, FALSE);
            }
            else
                _tcscpy(buffer, "Area 51");

            btn->SetText(C_STATE_0, buffer);
        }

        btn = (C_Button*)win->FindControl(UNIT_SQUADRON_TAB);

        if (btn)
        {
            total = 0;

            for (i = 0; i < ARO_OTHER; i++)
                total += sqd->GetRating(i);

            total /= ARO_OTHER;

            if (total < 0)
                total = 0;

            if (total > 4)
                total = 4;

            kills[0] = 0;
            kills[1] = 0;
            kills[2] = 0;
            kills[3] = 0;
            session = (FalconSessionEntity*)sessionWalker.GetFirst();

            while (session)
            {
                if (session->GetPlayerSquadronID() == sqd->Id())
                {
                    kills[0] += session->GetKill(FalconSessionEntity::_AIR_KILLS_);
                    kills[1] += session->GetKill(FalconSessionEntity::_GROUND_KILLS_);
                    kills[2] += session->GetKill(FalconSessionEntity::_NAVAL_KILLS_);
                    kills[3] += session->GetKill(FalconSessionEntity::_STATIC_KILLS_);
                }

                session = (FalconSessionEntity*)sessionWalker.GetNext();
            }


            btn->SetUserNumber(C_STATE_2, sqd->GetMissionsFlown());
            btn->SetUserNumber(C_STATE_3, total * 25);
            btn->SetUserNumber(C_STATE_4, sqd->GetAAKills() + kills[0]);
            btn->SetUserNumber(C_STATE_5, sqd->GetAGKills() + kills[1]);
            btn->SetUserNumber(C_STATE_6, sqd->GetANKills() + kills[2]);
            btn->SetUserNumber(C_STATE_7, sqd->GetASKills() + kills[3]);
            btn->SetUserNumber(10, sqd->GetPatchID());
            SquadronAirUnitCB(btn->GetID(), C_TYPE_LMOUSEUP, btn);
        }

        tree = (C_TreeList*)win->FindControl(UNIT_PILOTLIST);

        if (tree)
        {
            tree->SetSortType(TREE_SORT_CALLBACK);
            tree->SetSortCallback(SortPilotByNameCB);
            BuildPilotList(tree, sqd);
        }

        gMainHandler->ShowWindow(win);
        gMainHandler->WindowToFront(win);
        win->RefreshWindow();

        //dpc SquadronInfoFix for other squadrons
        //somehow this updates the squad info correctly, while the first call above doesn't....weird
        if (btn)
        {
            SquadronAirUnitCB(btn->GetID(), C_TYPE_LMOUSEUP, btn);
        }

        //end SquadronInfoFix

        UI_Leave(Leave);
    }
}

// Returns TRUE if I want to insert newitem before list item
static BOOL CampHotelSortCB(TREELIST *list, TREELIST *newitem)
{
    if ( not list or not newitem)
        return(FALSE);

    if (((C_Custom*)newitem->Item_)->GetValue(1) > ((C_Custom*)list->Item_)->GetValue(1))
        return(TRUE);

    return(FALSE);
}

void UpdateSierraHotel()
{
    C_Window *win;
    C_TreeList *tree;
    C_Text *txt;
    C_Bitmap *bmp;
    _TCHAR buffer[30];
    C_Custom *ctrl;
    O_Output *output;
    TREELIST *item;
    Squadron sqd;
    short i;
    long ItemID = 1;
    long kills;
    RemoteLB *lbptr;
    F4CSECTIONHANDLE *Leave;

    win = gMainHandler->FindWindow(CAMP_SH);

    if (win)
    {
        sqd = FalconLocalSession->GetPlayerSquadron();

        if ( not sqd)
            return;

        Leave = UI_Enter(win);

        tree = (C_TreeList*)win->FindControl(SH_PILOT_LIST);

        if ( not tree)
            return;

        tree->SetSortCallback(CampHotelSortCB);
        tree->SetSortType(TREE_SORT_CALLBACK);

        tree->DeleteBranch(tree->GetRoot()); // Clear it
        //if(gCommsMgr->Online())
        {
            VuSessionsIterator sessionWalker(FalconLocalGame);
            FalconSessionEntity *session;

            session = (FalconSessionEntity*)sessionWalker.GetFirst();

            while (session)
            {
                if (session->GetPlayerSquadronID() == sqd->Id())
                {
                    kills = 0;
                    kills = session->GetKill(FalconSessionEntity::_AIR_KILLS_);
                    // if(kills or 1)
                    {
                        ctrl = new C_Custom;
                        ctrl->Setup(C_DONT_CARE, FalconSessionEntity::_VS_HUMAN_, 2);

                        if (session == FalconLocalSession)
                        {
                            if (LogBook.GetPictureResource()) // Temporary
                                ctrl->SetUserNumber(C_STATE_0, LogBook.GetPictureResource()); // Image ID goes here
                            else
                            {
                                // Need to load a file
                                ctrl->SetUserNumber(C_STATE_0, NOFACE); // Image ID goes here
                            }
                        }
                        else
                        {
                            lbptr = (RemoteLB*)gCommsMgr->GetRemoteLB(session->Id().creator_);

                            if (lbptr and lbptr->Pilot_.PictureResource)
                            {
                                ctrl->SetUserNumber(C_STATE_0, lbptr->Pilot_.PictureResource); // Image ID goes here
                            }
                            else
                                ctrl->SetUserNumber(C_STATE_0, NOFACE); // Image ID goes here
                        }

                        ctrl->SetUserNumber(C_STATE_2, session->GetMissions()); // Num Missions
                        ctrl->SetUserNumber(C_STATE_3, session->GetRating()); // Mission Rating
                        ctrl->SetUserNumber(C_STATE_4, session->GetKill(FalconSessionEntity::_AIR_KILLS_));
                        ctrl->SetUserNumber(C_STATE_5, session->GetKill(FalconSessionEntity::_GROUND_KILLS_));
                        ctrl->SetUserNumber(C_STATE_6, session->GetKill(FalconSessionEntity::_NAVAL_KILLS_));
                        ctrl->SetUserNumber(C_STATE_7, session->GetKill(FalconSessionEntity::_STATIC_KILLS_));

                        // Set pilot name
                        output = ctrl->GetItem(0);
                        output->SetTextWidth(20);
                        output->SetText(session->GetPlayerName());
                        output->SetFont(tree->GetFont());
                        output->SetXY(tree->GetUserNumber(C_STATE_0), 0);
                        output->SetFgColor(0xeeeeee);
                        output->SetInfo();

                        // set pilot score (with commas)
                        ctrl->SetValue(1, kills); // used for sorting
                        _stprintf(buffer, "%1ld", kills);
                        AddCommas(buffer);

                        output = ctrl->GetItem(1);
                        output->SetTextWidth(15);
                        output->SetText(buffer);
                        output->SetXY(tree->GetUserNumber(C_STATE_1), 0);
                        output->SetFgColor(0xeeeeee);
                        output->SetFlags(output->GetFlags() bitor C_BIT_RIGHT);
                        output->SetInfo();

                        item = tree->CreateItem(ItemID, C_TYPE_ITEM, ctrl);

                        if (item)
                        {
                            tree->AddItem(tree->GetRoot(), item);
                            ItemID++;
                        }
                    }
                }

                session = (FalconSessionEntity*)sessionWalker.GetNext();
            }
        }

        for (i = 0; i < PILOTS_PER_SQUADRON; i++)
        {
            if (sqd->GetPilotData(i)->pilot_status == PILOT_IN_USE or sqd->GetPilotData(i)->pilot_status == PILOT_AVAILABLE)
            {
                kills = sqd->GetPilotData(i)->aa_kills;

                // if(kills or 1)
                {
                    GetPilotName(sqd->GetPilotData(i)->pilot_id, buffer, 25);
                    ctrl = new C_Custom;
                    ctrl->Setup(C_DONT_CARE, FalconSessionEntity::_VS_AI_, 2);

                    if (sqd->GetOwner() == FalconLocalSession->GetCountry())
                        ctrl->SetUserNumber(C_STATE_0, PilotImageIDs[PilotInfo[sqd->GetPilotData(i)->pilot_id].photo_id]); // Image ID goes here
                    else
                        ctrl->SetUserNumber(C_STATE_0, FlagImageID[TeamInfo[sqd->GetOwner()]->GetFlag()][BIG_HORIZ]); // Image ID goes here

                    ctrl->SetUserNumber(C_STATE_2, sqd->GetPilotData(i)->missions_flown); // Num Missions
                    ctrl->SetUserNumber(C_STATE_3, sqd->GetPilotData(i)->GetPilotRating() * 25); // Mission Rating
                    ctrl->SetUserNumber(C_STATE_4, sqd->GetPilotData(i)->aa_kills);
                    ctrl->SetUserNumber(C_STATE_5, sqd->GetPilotData(i)->ag_kills);
                    ctrl->SetUserNumber(C_STATE_6, sqd->GetPilotData(i)->an_kills);
                    ctrl->SetUserNumber(C_STATE_7, sqd->GetPilotData(i)->as_kills);

                    // Set pilot name
                    output = ctrl->GetItem(0);
                    output->SetTextWidth(20);
                    output->SetText(buffer);
                    output->SetFont(tree->GetFont());
                    output->SetXY(tree->GetUserNumber(C_STATE_0), 0);
                    output->SetFgColor(0xeeeeee);
                    output->SetInfo();

                    // set pilot score (with commas)
                    ctrl->SetValue(1, kills); // used for sorting
                    _stprintf(buffer, "%1ld", kills);
                    AddCommas(buffer);

                    output = ctrl->GetItem(1);
                    output->SetTextWidth(15);
                    output->SetText(buffer);
                    output->SetXY(tree->GetUserNumber(C_STATE_1), 0);
                    output->SetFgColor(0xeeeeee);
                    output->SetFlags(output->GetFlags() bitor C_BIT_RIGHT);
                    output->SetInfo();

                    item = tree->CreateItem(ItemID, C_TYPE_ITEM, ctrl);

                    if (item)
                    {
                        tree->AddItem(tree->GetRoot(), item);
                        ItemID++;
                    }
                }
            }
        }

        // Set ACE of the base
        item = tree->GetRoot();

        if (item)
        {
            ctrl = (C_Custom*)item->Item_;

            switch (ctrl->GetType())
            {
                case FalconSessionEntity::_VS_AI_:
                    bmp = (C_Bitmap*)win->FindControl(PILOT_PIC);

                    if (bmp)
                    {
                        bmp->Refresh();
                        bmp->SetImage(ctrl->GetUserNumber(C_STATE_0));
                        bmp->Refresh();
                    }

                    txt = (C_Text*)win->FindControl(AI_MISS_FIELD);

                    if (txt)
                    {
                        _stprintf(buffer, "%1ld", ctrl->GetUserNumber(C_STATE_2));
                        txt->Refresh();
                        txt->SetText(buffer);
                        txt->Refresh();
                    }

                    txt = (C_Text*)win->FindControl(AI_RATING_FIELD);

                    if (txt)
                    {
                        //_stprintf(buffer,"%1ld",ctrl->GetUserNumber(C_STATE_3));
                        txt->Refresh();
                        txt->SetText(ratingstr[(short)(((float)ctrl->GetUserNumber(C_STATE_3) / 25.0f + 0.5f)) % 5]);
                        txt->Refresh();
                    }

                    txt = (C_Text*)win->FindControl(AI_AA_KILLS_FIELD);

                    if (txt)
                    {
                        _stprintf(buffer, "%1ld", ctrl->GetUserNumber(C_STATE_4));
                        txt->Refresh();
                        txt->SetText(buffer);
                        txt->Refresh();
                    }

                    txt = (C_Text*)win->FindControl(AI_AG_KILLS_FIELD);

                    if (txt)
                    {
                        _stprintf(buffer, "%1ld", ctrl->GetUserNumber(C_STATE_5));
                        txt->Refresh();
                        txt->SetText(buffer);
                        txt->Refresh();
                    }

                    txt = (C_Text*)win->FindControl(AI_NAVAL_KILLS_FIELD);

                    if (txt)
                    {
                        _stprintf(buffer, "%1ld", ctrl->GetUserNumber(C_STATE_6));
                        txt->Refresh();
                        txt->SetText(buffer);
                        txt->Refresh();
                    }

                    txt = (C_Text*)win->FindControl(AI_STATIC_KILLS_FIELD);

                    if (txt)
                    {
                        _stprintf(buffer, "%1ld", ctrl->GetUserNumber(C_STATE_7));
                        txt->Refresh();
                        txt->SetText(buffer);
                        txt->Refresh();
                    }

                    win->HideCluster(1);
                    win->UnHideCluster(2);
                    break;

                case FalconSessionEntity::_VS_HUMAN_:
                    bmp = (C_Bitmap*)win->FindControl(PILOT_PIC);

                    if (bmp)
                    {
                        bmp->Refresh();
                        bmp->SetImage(ctrl->GetUserNumber(C_STATE_0));
                        bmp->Refresh();
                    }

                    txt = (C_Text*)win->FindControl(COMMISSIONED_FIELD);

                    if (txt)
                    {
                        txt->Refresh();
                        txt->SetText(LogBook.Commissioned());
                        txt->Refresh();
                    }

                    txt = (C_Text*)win->FindControl(CAMP_CAMPAIGNS_FIELD);

                    if (txt)
                    {
                        kills = LogBook.GetCampaign()->GamesWon;
                        kills += LogBook.GetCampaign()->GamesLost;
                        kills += LogBook.GetCampaign()->GamesTied;

                        _stprintf(buffer, "%1ld", kills);
                        txt->Refresh();
                        txt->SetText(buffer);
                        txt->Refresh();
                    }

                    txt = (C_Text*)win->FindControl(HOURS_FIELD);

                    if (txt)
                    {
                        _stprintf(buffer, "%.1f", LogBook.FlightHours());
                        txt->Refresh();
                        txt->SetText(buffer);
                        txt->Refresh();
                    }

                    txt = (C_Text*)win->FindControl(CAMP_MISS_FIELD);

                    if (txt)
                    {
                        _stprintf(buffer, "%1ld", ctrl->GetUserNumber(C_STATE_2));
                        txt->Refresh();
                        txt->SetText(buffer);
                        txt->Refresh();
                    }

                    txt = (C_Text*)win->FindControl(CAMP_RATING_FIELD);

                    if (txt)
                    {
                        //_stprintf(buffer,"%1ld",ctrl->GetUserNumber(C_STATE_3));
                        txt->Refresh();
                        txt->SetText(ratingstr[(short)(((float)ctrl->GetUserNumber(C_STATE_3) / 25.0f + 0.5f)) % 5]);
                        txt->Refresh();
                    }

                    txt = (C_Text*)win->FindControl(CAMP_AA_KILLS_FIELD);

                    if (txt)
                    {
                        _stprintf(buffer, "%1ld", ctrl->GetUserNumber(C_STATE_4));
                        txt->Refresh();
                        txt->SetText(buffer);
                        txt->Refresh();
                    }

                    txt = (C_Text*)win->FindControl(CAMP_AG_KILLS_FIELD);

                    if (txt)
                    {
                        _stprintf(buffer, "%1ld", ctrl->GetUserNumber(C_STATE_5));
                        txt->Refresh();
                        txt->SetText(buffer);
                        txt->Refresh();
                    }

                    txt = (C_Text*)win->FindControl(CAMP_NAVAL_KILLS_FIELD);

                    if (txt)
                    {
                        _stprintf(buffer, "%1ld", ctrl->GetUserNumber(C_STATE_6));
                        txt->Refresh();
                        txt->SetText(buffer);
                        txt->Refresh();
                    }

                    txt = (C_Text*)win->FindControl(CAMP_STATIC_KILLS_FIELD);

                    if (txt)
                    {
                        _stprintf(buffer, "%1ld", ctrl->GetUserNumber(C_STATE_7));
                        txt->Refresh();
                        txt->SetText(buffer);
                        txt->Refresh();
                    }

                    win->HideCluster(2);
                    win->UnHideCluster(1);
                    break;
            }

            item->Item_->SetFlagBitOn(C_BIT_INVISIBLE); // Hide ACE... since we show name at top
            txt = (C_Text*)win->FindControl(TOP_JOCK);

            if (txt)
            {
                txt->Refresh();
                txt->SetText(((C_Custom*)item->Item_)->GetItem(0)->GetText());
                txt->Refresh();
            }

            txt = (C_Text*)win->FindControl(TOP_SCORE);

            if (txt)
            {
                txt->Refresh();
                txt->SetText(((C_Custom*)item->Item_)->GetItem(1)->GetText());
                txt->Refresh();
            }
        }
        else
        {
            win->HideCluster(1);
            win->HideCluster(2);
        }

        tree->RecalcSize();

        if (tree->Parent_)
            tree->Parent_->RefreshClient(tree->GetClient());

        win->ScanClientArea(0);
        win->RefreshClient(0);
        UI_Leave(Leave);
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

WayPointClass* GetWayPointUnder(Unit unit)
{
    WayPoint w = unit->GetFirstUnitWP();
    GridIndex x, y, wx, wy;

    unit->GetLocation(&x, &y);
    gDragWPNum = 0;

    if ( not w)
        return NULL;

    while (w)
    {
        gDragWPNum++;
        w->GetWPLocation(&wx, &wy);

        if (wx == x and wy == y)
            return w;

        w = w->GetNextWP();
    }

    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UnitCB(long ID, short hittype, C_Base *ctrl)
{
    CampEntity entity;
    Unit unit;
    float wx, wy;
    short bx, by;
    MAPICONLIST *item;
    WayPointClass *wp, *pw = NULL, *nw;

    if (hittype == C_TYPE_LDROP)
    {
        entity = GetEntityByCampID(ID);

        if (entity and entity->IsUnit())
        {
            unit = (UnitClass *) entity;

            item = ((C_MapIcon*)ctrl)->FindID(ID);

            if (item)
            {
                wx = item->worldx;
                wy = gMapMgr->GetMaxY() - item->worldy;
                bx = SimToGrid(wx);
                by = SimToGrid(wy);

                // Find any waypoints we may be sitting on
                wp = GetWayPointUnder(unit);

                // Change our location and our waypoint's
                unit->SetLocation(bx, by);

                if (unit->IsBattalion())
                    tactical_set_orders((Battalion)unit, unit->GetUnitObjectiveID(), bx, by);
                else if (wp)
                {
                    wp->SetWPLocation(bx, by);
                    recalculate_waypoints(wp);
                    gMapMgr->SetCurrentWaypointList(unit->Id());
                }
            }
        }
    }
    else if (hittype == C_TYPE_LMOUSEUP)
    {
        entity = GetEntityByCampID(ID);

        if (entity and entity->IsUnit())
        {
            unit = (UnitClass *) entity;

            if ((TheCampaign.Flags bitand CAMP_TACTICAL_EDIT) or (GetTeam(unit->GetOwner()) == FalconLocalSession->GetTeam()))
            {
                gMapMgr->SetCurrentWaypointList(unit->Id());

                if (entity->IsFlight())
                {
                    gActiveFlightID = unit->Id();
                    gLoadoutFlightID = unit->Id();
                    // 2001-10-25 ADDED by M.N. gSelectedFlightID is used for munition, flight plan and steer point info
                    gSelectedFlightID = unit->Id();
                    // END of added section
                    SetupFlightSpecificControls((Flight)entity);
                }
            }
        }
    }
    else if (hittype == C_TYPE_MOUSEMOVE)
    {
        entity = GetEntityByCampID(ID);

        if (entity and entity->IsUnit())
        {
            unit = (UnitClass *) entity;

            if (entity->IsFlight())
            {
                gActiveFlightID = unit->Id();
                gLoadoutFlightID = unit->Id();
                // 2001-10-25 ADDED by M.N. gSelectedFlightID is used for munition, flight plan and steer point info
                gSelectedFlightID = unit->Id();
                // END of added section
                SetupFlightSpecificControls((Flight)entity);
            }

            item = ((C_MapIcon*)ctrl)->FindID(ID);

            if (item)
            {
                wx = item->worldx;
                wy = gMapMgr->GetMaxY() - item->worldy;
                bx = SimToGrid(wx);
                by = SimToGrid(wy);

                wp = GetWayPointUnder(unit);

                // Change our location
                unit->SetLocation(bx, by);

                if ( not unit->IsFlight())
                    return;

                // Add a new waypoint at current location or move the one we were sitting on
                if (wp)
                    wp->SetWPLocation(bx, by);
                else
                {
                    gDragWPNum = 1;
                    wp = unit->GetFirstUnitWP();

                    while (wp and wp not_eq unit->GetCurrentUnitWP())
                    {
                        gDragWPNum++;
                        wp = wp->GetNextWP();
                    }

                    if (wp)
                    {
                        pw = wp->GetPrevWP();
                        ShiAssert(pw);
                        nw = new WayPointClass(bx, by, wp->GetWPAltitude(), static_cast<int>(wp->GetWPSpeed()), TheCampaign.CurrentTime, 0, WP_NOTHING, 0);
                        nw->SetWPSpeed(wp->GetWPSpeed());
                        // Lock time? basically we're saying we want the flight here at this time, so I'd guess yes
                        nw->SetWPFlags(WPF_TIME_LOCKED);
                        pw->InsertWP(nw);
                    }

                    // Rebuild the waypoint list upon this addition
                    recalculate_waypoints(wp);
                    gMapMgr->SetCurrentWaypointList(unit->Id());
                }

                recalculate_waypoints(wp);
                gMapMgr->GetCurWP()->UpdateInfo((unit->GetCampID() * 256) + gDragWPNum, item->worldx, item->worldy);
                gMapMgr->GetCurWP()->Refresh();
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void fixup_unit(Unit unit)
{
    WayPointClass *wp, *pwp;
    CampaignHeading h;
    GridIndex x, y, dx, dy, wx, wy, pwx, pwy;
    int z, ndt, dt;
    CampaignTime current_time;
    float heading;

    current_time = TheCampaign.CurrentTime;

    wp = unit->GetFirstUnitWP();

    while (wp and (wp->GetWPDepartureTime() < current_time) and wp->GetWPAction() not_eq WP_LAND)
    {
        // Some special case stuff for air mobile
        if (wp->GetWPAction() == WP_PICKUP)
        {
            // Load the airborne battalion.
            Unit cargo = (Unit) wp->GetWPTarget();

            if (cargo)
            {
                unit->SetCargoId(cargo->Id());
                cargo->SetCargoId(unit->Id());
                cargo->SetInactive(1);
            }

            unit->LoadUnit(cargo);
        }
        else if (wp->GetWPAction() == WP_AIRDROP)
        {
            // Unload the airborne battalion.
            Unit cargo = (Unit) wp->GetWPTarget();
            unit->UnloadUnit();

            if (cargo)
            {
                cargo->SetCargoId(FalconNullId);
                cargo->SetInactive(0);
                wp->GetWPLocation(&x, &y);
                cargo->SetLocation(x, y);
            }
        }

        wp = wp->GetNextWP();
    }

    if (wp)
    {
        unit->SetCurrentUnitWP(wp);
        pwp = wp->GetPrevWP();

        if (pwp)
        {
            wp->GetWPLocation(&wx, &wy);

            if (wp->GetWPFlags() bitand WPF_HOLDCURRENT)
                z = pwp->GetWPAltitude();
            else
                z = wp->GetWPAltitude();

            pwp->GetWPLocation(&pwx, &pwy);

            // KCK NOTE: We may want to find grid paths for battalions...
            // But for now, just move in a staight line.
            dx = static_cast<short>(wx - pwx);
            dy = static_cast<short>(wy - pwy);
            ndt = current_time - pwp->GetWPDepartureTime();
            dt = wp->GetWPDepartureTime() - pwp->GetWPDepartureTime();

            if (ndt > dt)
                ndt = dt;

            dx = static_cast<short>(FloatToInt32(dx * ((float)ndt / (float)dt)));
            dy = static_cast<short>(FloatToInt32(dy * ((float)ndt / (float)dt)));
            heading = AngleTo(pwx, pwy, wx, wy);
            h = DirectionTo(pwx, pwy, wx, wy);

            unit->SetYPR(heading, 0.0F, 0.0F);

            if (unit->IsFlight())
                ((Flight)unit)->SetLastDirection(h);

            unit->SetUnitLastMove(current_time);
            unit->SetLocation(static_cast<short>(pwx + dx), static_cast<short>(pwy + dy));
            unit->SetUnitAltitude(z);
        }
        else
        {
            wp->GetWPLocation(&x, &y);
            unit->SetLocation(x, y);

            if (unit->IsFlight())
                unit->SetUnitDestination(x, y);

            pwp = wp->GetNextWP();

            if (pwp)
            {
                pwp->GetWPLocation(&wx, &wy);
                heading = AngleTo(x, y, wx, wy);
                unit->SetYPR(heading, 0.0F, 0.0F);
            }
        }
    }

    while (wp)
    {
        // Some special case stuff for air mobile
        if (wp->GetWPAction() == WP_PICKUP)
        {
            Unit cargo = (Unit) wp->GetWPTarget();

            if (cargo)
            {
                // Put airborne battalion at the pickup point.
                unit->SetCargoId(cargo->Id());
                cargo->SetCargoId(unit->Id());
                wp->GetWPLocation(&x, &y);
                cargo->SetLocation(x, y);
                cargo->SetInactive(0);
            }
        }

        wp = wp->GetNextWP();
    }
}

void fixup_unit_starting_positions(void)
{
    VuListIterator iter(AllRealList);
    UnitClass *unit;
    victory_condition *vc;

    unit = GetFirstUnit(&iter);

    while (unit)
    {
        fixup_unit(unit);
        unit = GetNextUnit(&iter);
    }

    if (gMapMgr)
    {
        vc = current_tactical_mission->get_first_unfiltered_victory_condition();

        while (vc)
        {
            gMapMgr->UpdateVC(vc);
            vc = current_tactical_mission->get_next_unfiltered_victory_condition();
        }
    }
}

