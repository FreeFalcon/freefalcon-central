
#include <windows.h>    // includes basic windows functionality
#include <commdlg.h>    // includes common dialog functionality
#include <dlgs.h>       // includes common dialog template defines
#include <stdio.h>      // includes standard file i/o functionality
#include <string.h>     // includes string functions
#include <cderr.h>      // includes the common dialog error codes
#include <fcntl.h>
#include <io.h>
#include <tchar.h>
#include "unitdlg.h"
#include "dialog.h"
#include "campterr.h"
#include "uiwin.h"
#include "campdriv.h"
#include "camplib.h"
#include "debuggr.h"
#include "resource.h"
#include "campdisp.h"
#include "wingraph.h"
#include "campaign.h"
#include "strategy.h"
#include "f4find.h"
#include "update.h"
#include "gtm.h"
#include "gndunit.h"
#include "navunit.h"
#include "mission.h"
#include "find.h"
#include "simdrive.h"
#include "simbase.h"
#include "f4thread.h"
#include "otwdrive.h"
#include "Graphics/Include/render2d.h"
#include "CampStr.h"
#include "MissEval.h"
#include "dispcfg.h"
#include "CmpClass.h"
#include "ThreadMgr.h"
#include "FalcSess.h"
#include "classtbl.h"
#include "brief.h"

// Vu private stuff
#include "vu2/src/vu_priv.h"
#include "vucoll.h"
extern float bubbleRatio;

#ifdef CAMPTOOL

int asAgg = 1;

int eldlgs[5][6] = {  { IDC_STATIC_U0, IDC_STATIC_U00, IDC_STATIC_U01, IDC_STATIC_U02, IDC_STATIC_U03, IDC_STATIC_U04 },
    { IDC_STATIC_U1, IDC_STATIC_U10, IDC_STATIC_U11, IDC_STATIC_U12, IDC_STATIC_U13, IDC_STATIC_U14 },
    { IDC_STATIC_U2, IDC_STATIC_U20, IDC_STATIC_U21, IDC_STATIC_U22, IDC_STATIC_U23, IDC_STATIC_U24 },
    { IDC_STATIC_U3, IDC_STATIC_U30, IDC_STATIC_U31, IDC_STATIC_U32, IDC_STATIC_U33, IDC_STATIC_U34 },
    { IDC_STATIC_U4, IDC_STATIC_U40, IDC_STATIC_U41, IDC_STATIC_U42, IDC_STATIC_U43, IDC_STATIC_U44 }
};
int SPTable[50];

extern int inButton(RECT *but, WORD xPos, WORD yPos);

#endif CAMPTOOL

char *BSP;
char *BTP;

// ========================================================
// External prototypes
// ========================================================

extern char TargetTypeStr[7][15];
extern BOOL WINAPI FistOfGod(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

extern int ShowReal;
extern HWND mainMenuWnd;
void ChooseMission(void);
void GetString(char* buffer);

// ========================================================
// Prototypes
// ========================================================

void RefreshNames(HWND hDlg);

BOOL WINAPI BriefDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

// ========================================================
// Support functions
// ========================================================

#ifdef CAMPTOOL

// This looks up the SPType in the table built during the combo box's creation, and finds
// the correct integer value. Could also be done by compairing strings...
int GetAdjustedSP(HWND hDlg, int SPType)
{
    int i, j;

    j = SendMessage(GetDlgItem(hDlg, IDC_UNIT_VEHICLECOMBO), CB_GETCOUNT, 0, 0);

    for (i = 0; i < j; i++)
    {
        if (SPTable[i] == SPType)
            return i;
    }

    return 0;
}

void SetSizeCombo(HWND hDlg, Unit u)
{
    SendMessage(GetDlgItem(hDlg, IDC_UNIT_SIZECOMBO), CB_RESETCONTENT, 0, 0);
    SendMessage(GetDlgItem(hDlg, IDC_UNIT_SIZECOMBO), CB_ADDSTRING, 0, (LPARAM)"(none)");

    if (u->GetDomain() == DOMAIN_AIR)
    {
        SendMessage(GetDlgItem(hDlg, IDC_UNIT_SIZECOMBO), CB_ADDSTRING, 0, (LPARAM)"Flight");
        SendMessage(GetDlgItem(hDlg, IDC_UNIT_SIZECOMBO), CB_ADDSTRING, 0, (LPARAM)"Mission Group");
        SendMessage(GetDlgItem(hDlg, IDC_UNIT_SIZECOMBO), CB_ADDSTRING, 0, (LPARAM)"Squadron");
    }
    else if (u->GetDomain() == DOMAIN_LAND)
    {
        SendMessage(GetDlgItem(hDlg, IDC_UNIT_SIZECOMBO), CB_ADDSTRING, 0, (LPARAM)"Battalion");
        SendMessage(GetDlgItem(hDlg, IDC_UNIT_SIZECOMBO), CB_ADDSTRING, 0, (LPARAM)"Brigade");
    }
    else if (u->GetDomain() == DOMAIN_SEA)
        SendMessage(GetDlgItem(hDlg, IDC_UNIT_SIZECOMBO), CB_ADDSTRING, 0, (LPARAM)"Task Force");

    SendMessage(GetDlgItem(hDlg, IDC_UNIT_SIZECOMBO), CB_SETCURSEL, u->GetType(), 0);
}

void SetTypeCombo(HWND hDlg, Unit u)
{
    int i, j, k;
    char buffer[30];
    UnitClassDataType* uc;
    VehicleClassDataType* vc;

    // Set the possible types
    SendMessage(GetDlgItem(hDlg, IDC_UNIT_TYPECOMBO), CB_RESETCONTENT, 0, 0);
    SendMessage(GetDlgItem(hDlg, IDC_UNIT_TYPECOMBO), CB_ADDSTRING, 0, (LPARAM)"(none)");

    for (i = 1; i < 30; i++)
    {
        j = GetClassID(u->GetDomain(), CLASS_UNIT, u->GetType(), i, 0, 0, 0, 0);

        if (j)
        {
            // uc = (UnitClassDataType*) Falcon4ClassTable[j].dataPtr;
            // if (uc)
            // SendMessage(GetDlgItem(hDlg,IDC_UNIT_TYPECOMBO),CB_ADDSTRING,0,(LPARAM)uc->Name);
            SendMessage(GetDlgItem(hDlg, IDC_UNIT_TYPECOMBO), CB_ADDSTRING, 0, (LPARAM)GetSTypeName(u->GetDomain(), u->GetType(), i, buffer));
        }
        else
            SendMessage(GetDlgItem(hDlg, IDC_UNIT_TYPECOMBO), CB_ADDSTRING, 0, (LPARAM)"<Davism>");
    }

    SendMessage(GetDlgItem(hDlg, IDC_UNIT_TYPECOMBO), CB_SETCURSEL, u->GetSType(), 0);

    // Set the possible vehicles
    SendMessage(GetDlgItem(hDlg, IDC_UNIT_VEHICLECOMBO), CB_RESETCONTENT, 0, 0);
    k = 1;

    if (u->GetDomain() == DOMAIN_LAND and u->GetType() == TYPE_BRIGADE)
    {
        SendMessage(GetDlgItem(hDlg, IDC_UNIT_VEHICLECOMBO), CB_ADDSTRING, 0, (LPARAM)"(none)");
        SendMessage(GetDlgItem(hDlg, IDC_UNIT_VEHICLECOMBO), CB_ADDSTRING, 0, (LPARAM)"Generic");
        SendMessage(GetDlgItem(hDlg, IDC_UNIT_VEHICLECOMBO), CB_SETCURSEL, u->GetSPType(), 0);
    }
    else
    {
        SendMessage(GetDlgItem(hDlg, IDC_UNIT_VEHICLECOMBO), CB_ADDSTRING, 0, (LPARAM)"(none)");

        for (i = 1; i < 255; i++)
        {
            j = GetClassID(u->GetDomain(), CLASS_UNIT, u->GetType(), u->GetSType(), i, 0, 0, 0);

            if (j)
            {
                uc = (UnitClassDataType*) Falcon4ClassTable[j].dataPtr;

                if (uc)
                {
                    vc = (VehicleClassDataType*) Falcon4ClassTable[uc->VehicleType[0]].dataPtr;

                    if (vc and uc->VehicleType[0])
                    {
                        SendMessage(GetDlgItem(hDlg, IDC_UNIT_VEHICLECOMBO), CB_ADDSTRING, 0, (LPARAM)vc->Name);
                        SPTable[k] = i;
                        k++;
                    }
                }
            }
        }

        k = GetAdjustedSP(hDlg, u->GetSPType());
        SendMessage(GetDlgItem(hDlg, IDC_UNIT_VEHICLECOMBO), CB_SETCURSEL, k, 0);
    }
}

void SetOptionalBoxes(HWND hDlg, int edit, int combo)
{
    ShowWindow(GetDlgItem(hDlg, IDC_UNIT_OPT2EDIT), edit);
    ShowWindow(GetDlgItem(hDlg, IDC_UNIT_OPT2COMBO), combo);
}

void ResetDialogStats(HWND hDlg, int domain, int type)
{
    int i, j;

    // Clear all optional windows first
    SetWindowText(GetDlgItem(hDlg, IDC_STATIC_OPT1), "Orders:");
    SetWindowText(GetDlgItem(hDlg, IDC_STATIC_OPT2), "Division:");
    SetWindowText(GetDlgItem(hDlg, IDC_STATIC_OPT3), "Fatigue:");
    SetWindowText(GetDlgItem(hDlg, IDC_STATIC_OPT4), "Morale:");
    SetWindowText(GetDlgItem(hDlg, IDC_STATIC_OPT5), "Supply:");
    SetWindowText(GetDlgItem(hDlg, IDC_STATIC_OPT6), "Objective:");
    ShowWindow(GetDlgItem(hDlg, IDC_STATIC_OPT3), 1);
    ShowWindow(GetDlgItem(hDlg, IDC_STATIC_OPT4), 1);
    ShowWindow(GetDlgItem(hDlg, IDC_STATIC_OPT5), 1);
    ShowWindow(GetDlgItem(hDlg, IDC_UNIT_OPT3EDIT), 1);
    ShowWindow(GetDlgItem(hDlg, IDC_UNIT_OPT4EDIT), 1);
    ShowWindow(GetDlgItem(hDlg, IDC_UNIT_OPT5EDIT), 1);
    ShowWindow(GetDlgItem(hDlg, IDC_UNIT_OPT6EDIT), 1);

    for (i = 0; i < 5; i++)
        for (j = 0; j < 6; j++)
            ShowWindow(GetDlgItem(hDlg, eldlgs[i][j]), 0);

    for (i = 0; i < 16; i++)
        ShowWindow(GetDlgItem(hDlg, IDC_UNIT_0 + i), 0);

    if (domain == DOMAIN_LAND)
    {
        SetOptionalBoxes(hDlg, 1, 0);

        if (type == TYPE_BRIGADE)
        {
            for (j = 0; j < 5; j++)
                ShowWindow(GetDlgItem(hDlg, eldlgs[j][0]), 1);
        }
        else if (type == TYPE_BATTALION)
        {
            for (i = 0; i < 16; i++)
                ShowWindow(GetDlgItem(hDlg, IDC_UNIT_0 + i), 1);
        }
    }
    else if (domain == DOMAIN_AIR)
    {
        if (type == TYPE_SQUADRON)
        {
            SetWindowText(GetDlgItem(hDlg, IDC_STATIC_OPT1), "Role:");
            SetWindowText(GetDlgItem(hDlg, IDC_STATIC_OPT2), "Req Rng:");
            SetWindowText(GetDlgItem(hDlg, IDC_STATIC_OPT3), "Cur Rng:");
            SetWindowText(GetDlgItem(hDlg, IDC_STATIC_OPT4), "Fuel:");
            SetWindowText(GetDlgItem(hDlg, IDC_STATIC_OPT5), "Weapons:");
            SetOptionalBoxes(hDlg, 1, 0);
            ShowWindow(GetDlgItem(hDlg, IDC_UNIT_0), 1);
        }
        else if (type == TYPE_FLIGHT)
        {
            SetWindowText(GetDlgItem(hDlg, IDC_STATIC_OPT1), "Mission:");
            SetWindowText(GetDlgItem(hDlg, IDC_STATIC_OPT2), "Loadout:");
            SetWindowText(GetDlgItem(hDlg, IDC_STATIC_OPT3), "Target:");
            SetWindowText(GetDlgItem(hDlg, IDC_STATIC_OPT4), "Fuel:");
            SetWindowText(GetDlgItem(hDlg, IDC_STATIC_OPT5), "Weapons:");
            SetOptionalBoxes(hDlg, 0, 1);
            ShowWindow(GetDlgItem(hDlg, IDC_UNIT_0), 1);
        }
        else if (type == TYPE_PACKAGE)
        {
            SetWindowText(GetDlgItem(hDlg, IDC_STATIC_OPT1), "Package:");
            SetWindowText(GetDlgItem(hDlg, IDC_STATIC_OPT2), "Priorty:");
            ShowWindow(GetDlgItem(hDlg, IDC_STATIC_OPT3), 0);
            ShowWindow(GetDlgItem(hDlg, IDC_STATIC_OPT4), 0);
            ShowWindow(GetDlgItem(hDlg, IDC_STATIC_OPT5), 0);
            ShowWindow(GetDlgItem(hDlg, IDC_STATIC_OPT6), 0);
            ShowWindow(GetDlgItem(hDlg, IDC_UNIT_OPT3EDIT), 0);
            ShowWindow(GetDlgItem(hDlg, IDC_UNIT_OPT4EDIT), 0);
            ShowWindow(GetDlgItem(hDlg, IDC_UNIT_OPT5EDIT), 0);
            ShowWindow(GetDlgItem(hDlg, IDC_UNIT_OPT6EDIT), 0);
            SetOptionalBoxes(hDlg, 1, 0);

            for (j = 0; j < 5; j++)
                ShowWindow(GetDlgItem(hDlg, eldlgs[j][0]), 1);
        }
    }
    else if (domain == DOMAIN_SEA)
    {
        if (type == TYPE_TASKFORCE)
        {
            for (i = 0; i < 16; i++)
                ShowWindow(GetDlgItem(hDlg, IDC_UNIT_0 + i), 1);
        }

        SetOptionalBoxes(hDlg, 1, 0);
    }
}

void SetOptionalValues(HWND hDlg, Unit u)
{
    char buffer[80];
    int i, j;

    sprintf(buffer, "%d", u->Type() - VU_LAST_ENTITY_TYPE);
    SetWindowText(GetDlgItem(hDlg, IDC_INDEX), buffer);

    if (u->GetDomain() == DOMAIN_LAND)
    {
        Objective o;
        // Set up orders combo box
        SendMessage(GetDlgItem(hDlg, IDC_UNIT_OPT1COMBO), CB_RESETCONTENT, 0, 0);

        for (i = 0; i < GORD_LAST; i++)
            SendMessage(GetDlgItem(hDlg, IDC_UNIT_OPT1COMBO), CB_ADDSTRING, 0, (LPARAM)OrderStr[i]);

        SendMessage(GetDlgItem(hDlg, IDC_UNIT_OPT1COMBO), CB_SETCURSEL, u->GetUnitOrders(), 0);
        // Set the other values
        sprintf(buffer, "%d", u->GetUnitDivision());
        SetWindowText(GetDlgItem(hDlg, IDC_UNIT_OPT2EDIT), buffer);
        sprintf(buffer, "%d", u->GetUnitFatigue());
        SetWindowText(GetDlgItem(hDlg, IDC_UNIT_OPT3EDIT), buffer);
        sprintf(buffer, "%d", u->GetUnitMorale());
        SetWindowText(GetDlgItem(hDlg, IDC_UNIT_OPT4EDIT), buffer);
        sprintf(buffer, "%d", u->GetUnitSupply());
        SetWindowText(GetDlgItem(hDlg, IDC_UNIT_OPT5EDIT), buffer);
        o = u->GetUnitObjective();

        if (o)
            o->GetName(buffer, 80, FALSE);
        else
            sprintf(buffer, "NONE");

        SetWindowText(GetDlgItem(hDlg, IDC_UNIT_OPT6EDIT), buffer);
    }
    else if (u->GetDomain() == DOMAIN_AIR)
    {
        if (u->GetType() == TYPE_SQUADRON)
        {
            GridIndex x, y;
            int min, max, cur;

            // Set up the mission role combo box
            SendMessage(GetDlgItem(hDlg, IDC_UNIT_OPT1COMBO), CB_RESETCONTENT, 0, 0);
            SendMessage(GetDlgItem(hDlg, IDC_UNIT_OPT1COMBO), CB_ADDSTRING, 0, (LPARAM)"General");
            SendMessage(GetDlgItem(hDlg, IDC_UNIT_OPT1COMBO), CB_ADDSTRING, 0, (LPARAM)"Air to Air");
            SendMessage(GetDlgItem(hDlg, IDC_UNIT_OPT1COMBO), CB_ADDSTRING, 0, (LPARAM)"Air to Ground");
            SendMessage(GetDlgItem(hDlg, IDC_UNIT_OPT1COMBO), CB_SETCURSEL, u->GetUnitSpecialty(), 0);
            u->GetLocation(&x, &y);
            cur = FloatToInt32(DistanceToFront(x, y));
            min = u->GetUnitRange() / 30;
            max = u->GetUnitRange() / 3;
            sprintf(buffer, "%d < rng < %d", min, max);
            SetWindowText(GetDlgItem(hDlg, IDC_UNIT_OPT2EDIT), buffer);
            sprintf(buffer, "%d", cur);
            SetWindowText(GetDlgItem(hDlg, IDC_UNIT_OPT3EDIT), buffer);
            sprintf(buffer, "%d", u->GetSquadronFuel());
            SetWindowText(GetDlgItem(hDlg, IDC_UNIT_OPT4EDIT), buffer);

            for (i = j = 0; i < MAXIMUM_WEAPTYPES; i++)
                j += u->GetUnitStores(i);

            sprintf(buffer, "%d", j);
            SetWindowText(GetDlgItem(hDlg, IDC_UNIT_OPT5EDIT), buffer);
            // sprintf(buffer,"%d",u->GetUnitMorale());
            // SetWindowText(GetDlgItem(hDlg,IDC_UNIT_OPT4EDIT),buffer);
            // sprintf(buffer,"%d",u->GetUnitSupply());
            // SetWindowText(GetDlgItem(hDlg,IDC_UNIT_OPT5EDIT),buffer);
            SetWindowText(GetDlgItem(hDlg, IDC_UNIT_OPT6EDIT), "Blank");
        }
        else if (u->GetType() == TYPE_FLIGHT)
        {
            // Set up the mission combo box
            SendMessage(GetDlgItem(hDlg, IDC_UNIT_OPT1COMBO), CB_RESETCONTENT, 0, 0);

            for (i = 0; i < AMIS_OTHER; i++)
                SendMessage(GetDlgItem(hDlg, IDC_UNIT_OPT1COMBO), CB_ADDSTRING, 0, (LPARAM)MissStr[i]);

            SendMessage(GetDlgItem(hDlg, IDC_UNIT_OPT1COMBO), CB_SETCURSEL, u->GetUnitMission(), 0);
            // Set up the loadout combo box
            // HACK: Faked for now
            SendMessage(GetDlgItem(hDlg, IDC_UNIT_OPT2COMBO), CB_RESETCONTENT, 0, 0);
            SendMessage(GetDlgItem(hDlg, IDC_UNIT_OPT2COMBO), CB_ADDSTRING, 0, (LPARAM)"Standard");
            SendMessage(GetDlgItem(hDlg, IDC_UNIT_OPT2COMBO), CB_SETCURSEL, 0, 0);
            // SetWindowText(GetDlgItem(hDlg,IDC_UNIT_OPT3VAL),GetLoadoutName(u->GetUnitLoadout()));
            // Now the rest
            sprintf(buffer, "%d", u->GetUnitMissionTarget());
            SetWindowText(GetDlgItem(hDlg, IDC_UNIT_OPT3EDIT), buffer);
            sprintf(buffer, "%d", u->GetSquadronFuel());
            SetWindowText(GetDlgItem(hDlg, IDC_UNIT_OPT4EDIT), buffer);

            for (i = 0, j = 0; i < HARDPOINT_MAX; i++)
                j += u->GetUnitWeaponCount(i, 0);

            sprintf(buffer, "%d", j);
            SetWindowText(GetDlgItem(hDlg, IDC_UNIT_OPT5EDIT), buffer);
            SetWindowText(GetDlgItem(hDlg, IDC_UNIT_OPT6EDIT), "Blank");
        }
        else if (u->GetType() == TYPE_PACKAGE)
        {
            // Set up the package type combo box
            // Faked for now
            SendMessage(GetDlgItem(hDlg, IDC_UNIT_OPT1COMBO), CB_RESETCONTENT, 0, 0);
            SendMessage(GetDlgItem(hDlg, IDC_UNIT_OPT1COMBO), CB_ADDSTRING, 0, (LPARAM)"General");
            SendMessage(GetDlgItem(hDlg, IDC_UNIT_OPT1COMBO), CB_SETCURSEL, 0, 0);
            // sprintf(buffer,"%d",u->GetUnitPackage());
            sprintf(buffer, "%d", u->GetUnitPriority());
            SetWindowText(GetDlgItem(hDlg, IDC_UNIT_OPT2EDIT), buffer);
            SetWindowText(GetDlgItem(hDlg, IDC_UNIT_OPT3EDIT), "");
            SetWindowText(GetDlgItem(hDlg, IDC_UNIT_OPT4EDIT), "");
            SetWindowText(GetDlgItem(hDlg, IDC_UNIT_OPT5EDIT), "");
            SetWindowText(GetDlgItem(hDlg, IDC_UNIT_OPT6EDIT), "Blank");
        }
    }
    else if (u->GetDomain() == DOMAIN_SEA)
    {
        // Set up orders combo box
        SendMessage(GetDlgItem(hDlg, IDC_UNIT_OPT1COMBO), CB_RESETCONTENT, 0, 0);

        for (i = 0; i < NORD_OTHER; i++)
            SendMessage(GetDlgItem(hDlg, IDC_UNIT_OPT1COMBO), CB_ADDSTRING, 0, (LPARAM)OrderStr[i]);

        SendMessage(GetDlgItem(hDlg, IDC_UNIT_OPT1COMBO), CB_SETCURSEL, u->GetUnitOrders(), 0);
        SetWindowText(GetDlgItem(hDlg, IDC_UNIT_OPT2EDIT), "");
        SetWindowText(GetDlgItem(hDlg, IDC_UNIT_OPT3EDIT), "");
        SetWindowText(GetDlgItem(hDlg, IDC_UNIT_OPT4EDIT), "");
        SetWindowText(GetDlgItem(hDlg, IDC_UNIT_OPT5EDIT), "");
        SetWindowText(GetDlgItem(hDlg, IDC_UNIT_OPT6EDIT), "Blank");
        PostMessage(GetDlgItem(hDlg, IDC_RADIOTASKFORCE), BM_SETCHECK, 1, 0);
    }
}

void ParseOptionalButtons(HWND hDlg, int button, int message, Unit u)
{
    int i;
    char buffer[80];

    switch (button)
    {
        case IDC_UNIT_OPT1COMBO:
            if (message == CBN_SELENDOK)
            {
                i = SendMessage(GetDlgItem(hDlg, IDC_UNIT_OPT1COMBO), CB_GETCURSEL, 0, 0);

                if (i < 0)
                    return;

                if (u->IsFlight())
                    u->SetUnitMission(i);
                else if (u->IsSquadron())
                    u->SetUnitSpecialty(i);
                else if (u->IsPackage())
                    ((Package)u)->SetPackageType(i);
                else
                {
                    GridIndex x, y;
                    Objective o;

                    u->GetLocation(&x, &y);
                    o = FindNearestObjective(x, y, NULL);

                    if (o)
                        u->SetUnitOrders(i, o->Id());
                    else
                        u->SetUnitOrders(i);
                }
            }

            break;

        case IDC_UNIT_OPT2COMBO:
            if (message == CBN_SELENDOK)
            {
                i = SendMessage(GetDlgItem(hDlg, IDC_UNIT_OPT2COMBO), CB_GETCURSEL, 0, 0);

                if (i < 0)
                    return;

                // if (u->GetType() == TYPE_FLIGHT and u->GetDomain() == DOMAIN_AIR)
                // ; // u->SetUnitLoadout(i);
            }

            break;

        case IDC_UNIT_OPT2EDIT:
            if (message == EN_KILLFOCUS)
            {
                GetDlgItemText(hDlg, IDC_UNIT_OPT2EDIT, buffer, 79);
                i = atoi(buffer);

                if (i < 0)
                    return;

                if (u->GetType() == TYPE_FLIGHT and u->GetDomain() == DOMAIN_AIR)
                    return;
                else if (u->GetType() == TYPE_SQUADRON and u->GetDomain() == DOMAIN_AIR)
                    return; // u->SetUnitWeapons(i);
                else if (u->GetType() == TYPE_PACKAGE and u->GetDomain() == DOMAIN_AIR)
                    u->SetUnitPriority(i);
                else if (u->GetType() == TYPE_TASKFORCE and u->GetDomain() == DOMAIN_SEA)
                    return;
                else
                {
                    if (u->GetUnitDivision() not_eq i)
                    {
                        u->SetUnitDivision(i);
                        u->SetUnitNameID(0);
                    }

                    u->SetUnitNameID(FindUnitNameID(u));
                    RefreshNames(hDlg);
                }
            }

            break;

        case IDC_UNIT_OPT3EDIT:
            if (message == EN_KILLFOCUS)
            {
                GetDlgItemText(hDlg, IDC_UNIT_OPT3EDIT, buffer, 79);
                i = atoi(buffer);

                if (i < 0)
                    return;

                if (u->GetType() == TYPE_FLIGHT and u->GetDomain() == DOMAIN_AIR)
                    u->SetUnitMissionTarget(i);
                else if (u->GetType() == TYPE_SQUADRON and u->GetDomain() == DOMAIN_AIR)
                    u->SetSquadronFuel(i);
                else if (u->GetType() == TYPE_PACKAGE and u->GetDomain() == DOMAIN_AIR)
                    return;
                else
                    u->SetUnitFatigue(i);
            }

            break;

        case IDC_UNIT_OPT4EDIT:
            if (message == EN_KILLFOCUS)
            {
                GetDlgItemText(hDlg, IDC_UNIT_OPT4EDIT, buffer, 79);
                i = atoi(buffer);

                if (i < 0)
                    return;

                if (u->GetType() == TYPE_FLIGHT and u->GetDomain() == DOMAIN_AIR)
                    u->SetSquadronFuel(i);
                else if (u->GetType() == TYPE_PACKAGE and u->GetDomain() == DOMAIN_AIR)
                    return;
                else
                    u->SetUnitMorale(i);
            }

            break;

        case IDC_UNIT_OPT5EDIT:
            if (message == EN_KILLFOCUS)
            {
                GetDlgItemText(hDlg, IDC_UNIT_OPT5EDIT, buffer, 79);
                i = atoi(buffer);

                if (i < 0)
                    return;

                if (u->GetType() == TYPE_FLIGHT and u->GetDomain() == DOMAIN_AIR)
                    ; // Disable until loadout stuff is done
                else if (u->GetType() == TYPE_PACKAGE and u->GetDomain() == DOMAIN_AIR)
                    return;
                else
                    u->SetUnitSupply(i);
            }

            break;

        case IDC_UNIT_OPT6EDIT:
            if (message == EN_KILLFOCUS)
            {
                GetDlgItemText(hDlg, IDC_UNIT_OPT6EDIT, buffer, 79);
                // What to do here?
            }

            break;

        default:
            break;
    }
}

void SetButtons(HWND hDlg, RECT buttons[10], RECT ebuttons[5][6], int ulx, int uly)
{
    int i, j;
    RECT r;

    GetWindowRect(GetDlgItem(hDlg, IDC_UNIT_MAIN), &r);
    SetRect(&buttons[0], r.left - ulx, r.top - uly + 8, r.right - ulx, r.bottom - uly);
    GetWindowRect(GetDlgItem(hDlg, IDC_UNIT_OWNVAL), &r);
    SetRect(&buttons[1], r.left - ulx, r.top - uly, r.right - ulx, r.bottom - uly);
    GetWindowRect(GetDlgItem(hDlg, IDC_UNIT_TYPEVAL), &r);
    SetRect(&buttons[2], r.left - ulx, r.top - uly, r.right - ulx, r.bottom - uly);
    GetWindowRect(GetDlgItem(hDlg, IDC_UNIT_VEHVAL), &r);
    SetRect(&buttons[3], r.left - ulx, r.top - uly, r.right - ulx, r.bottom - uly);
    GetWindowRect(GetDlgItem(hDlg, IDC_UNIT_OPT1VAL), &r);
    SetRect(&buttons[4], r.left - ulx, r.top - uly, r.right - ulx, r.bottom - uly);
    GetWindowRect(GetDlgItem(hDlg, IDC_UNIT_OPT2VAL), &r);
    SetRect(&buttons[5], r.left - ulx, r.top - uly, r.right - ulx, r.bottom - uly);
    GetWindowRect(GetDlgItem(hDlg, IDC_UNIT_OPT3VAL), &r);
    SetRect(&buttons[6], r.left - ulx, r.top - uly, r.right - ulx, r.bottom - uly);
    GetWindowRect(GetDlgItem(hDlg, IDC_UNIT_0), &r);
    SetRect(&buttons[8], r.left - ulx, r.top - uly, r.right - ulx, r.bottom - uly);

    for (i = 0; i < 5; i++)
        for (j = 0; j < 6; j++)
        {
            GetWindowRect(GetDlgItem(hDlg, eldlgs[i][j]), &r);
            SetRect(&ebuttons[i][j], r.left - ulx - 1, r.top - uly + 5, r.right - ulx, r.bottom - uly + 8);
        }
}

void DisplayNextInStack(HWND hDlg, Unit u)
{
    int foundone = 0;
    GridIndex x, y, tx, ty;
    Unit e;

    VuListIterator *myit;

    if (ShowReal == 2)
    {
        myit = new VuListIterator(InactiveList);
    }
    else if (ShowReal == 1)
    {
        myit = new VuListIterator(AllRealList);
    }
    else
    {
        myit = new VuListIterator(AllParentList);
    }

    // Next unit in stack.
    u->GetLocation(&x, &y); // Current in stack
    e = GetFirstUnit(myit);

    while (e and e not_eq u)
    {
        e = GetNextUnit(myit); // Get to current location in list
    }

    // e should be u or be null here
    if ( not e)
    {
        delete myit;
        return;
    }

    e = GetNextUnit(myit);

    while (e and not foundone)
    {
        e->GetLocation(&tx, &ty);

        if (x == tx and y == ty and e not_eq u)
        {
            foundone = 1;
        }
        else
        {
            e = GetNextUnit(myit);
        }
    }

    if (e)
    {
        GlobUnit = e;
        DialogBox(hInst, MAKEINTRESOURCE(IDD_UNITDIALOG1), hDlg, (DLGPROC)EditUnit);
        GlobUnit = u;
    }

    delete myit;
}

void ShowSubunitInfo(HDC DC, HWND hDlg, Unit U, short Set, short i, int asagg)
{
    Unit  E;
    short j = 0;
    HWND hEWnd;
    HDC EDC;
    PAINTSTRUCT ps;

    E = U->GetFirstUnitElement();

    while (E not_eq NULL)
    {
        if (j > 4)
        {
            i++;
            j = 0;
        }

        switch (Set)
        {
            case 1:
                ShowSubunitInfo(DC, hDlg, E, Set + 1, j, asagg);
                hEWnd = GetDlgItem(hDlg, eldlgs[j][i]);
                EDC = BeginPaint(hEWnd, &ps);
                DisplayUnit(EDC, E, 0, 10, 36);
                EndPaint(hEWnd, &ps);
                break;

            case 2:
                hEWnd = GetDlgItem(hDlg, eldlgs[i][j + 1]);
                EDC = BeginPaint(hEWnd, &ps);
                DisplayUnit(EDC, E, 1, 6, 18);
                EndPaint(hEWnd, &ps);
                break;
        }

        E = U->GetNextUnitElement();
        j++;
    }
}

void ShowElementInfo(HDC DC, HWND hDlg, Unit U, short Set, short i, int asagg)
{
    short j = 0;
    short Rost;
    char buffer[20];

    if (( not asagg or not U->Father()) and Set == 1)
    {
        if (U->GetDomain() == DOMAIN_AIR)
        {
            Rost = 0;

            for (j = 0; j < VEHICLE_GROUPS_PER_UNIT; j++)
                Rost += U->GetNumVehicles(j);

            sprintf(buffer, "%d  %s\0", Rost, GetVehicleName(U->GetVehicleID(0)));
            SetWindowText(GetDlgItem(hDlg, IDC_UNIT_0), buffer);
        }
        else
        {
            for (j = 0; j < VEHICLE_GROUPS_PER_UNIT; j++)
            {
                sprintf(buffer, "%d  %s\0", U->GetNumVehicles(j), GetVehicleName(U->GetVehicleID(j)));
                SetWindowText(GetDlgItem(hDlg, IDC_UNIT_0 + j), buffer);
            }
        }
    }
    else
        ShowSubunitInfo(DC, hDlg, U, 1, 0, asAgg);
}

void RefreshControls(HWND hDlg)
{
    InvalidateRect(hDlg, NULL, FALSE);
    InvalidateRect(GetDlgItem(hDlg, IDC_UNIT_DOMAINCOMBO), NULL, TRUE);
    PostMessage(GetDlgItem(hDlg, IDC_UNIT_DOMAINCOMBO), WM_PAINT, 0, 0);
    InvalidateRect(GetDlgItem(hDlg, IDC_UNIT_SIZECOMBO), NULL, TRUE);
    PostMessage(GetDlgItem(hDlg, IDC_UNIT_SIZECOMBO), WM_PAINT, 0, 0);
    InvalidateRect(GetDlgItem(hDlg, IDC_UNIT_TYPECOMBO), NULL, TRUE);
    PostMessage(GetDlgItem(hDlg, IDC_UNIT_TYPECOMBO), WM_PAINT, 0, 0);
    InvalidateRect(GetDlgItem(hDlg, IDC_UNIT_VEHICLECOMBO), NULL, TRUE);
    PostMessage(GetDlgItem(hDlg, IDC_UNIT_VEHICLECOMBO), WM_PAINT, 0, 0);
    InvalidateRect(GetDlgItem(hDlg, IDC_UNIT_OWNERCOMBO), NULL, TRUE);
    PostMessage(GetDlgItem(hDlg, IDC_UNIT_OWNERCOMBO), WM_PAINT, 0, 0);
}

void RefreshNames(HWND hDlg)
{
    InvalidateRect(GetDlgItem(hDlg, IDC_UNIT_NAMEIDEDIT), NULL, TRUE);
    PostMessage(GetDlgItem(hDlg, IDC_UNIT_NAMEIDEDIT), WM_PAINT, 0, 0);
    InvalidateRect(GetDlgItem(hDlg, IDC_UNIT_NAMEVAL), NULL, TRUE);
    PostMessage(GetDlgItem(hDlg, IDC_UNIT_NAMEVAL), WM_PAINT, 0, 0);
    InvalidateRect(GetDlgItem(hDlg, IDC_UNIT_NAMEIDVAL), NULL, TRUE);
    PostMessage(GetDlgItem(hDlg, IDC_UNIT_NAMEIDVAL), WM_PAINT, 0, 0);
}

void ResetBox(HWND hDlg, Unit u)
{
    SetSizeCombo(hDlg, u);
    SetTypeCombo(hDlg, u);
    ResetDialogStats(hDlg, u->GetDomain(), u->GetType());
    SetOptionalValues(hDlg, u);
    InvalidateRect(hDlg, NULL, FALSE);
}

int GetNextType(Unit u)
{
    if (u->GetDomain() == DOMAIN_LAND)
        return TYPE_BATTALION;
    else if (u->GetDomain() == DOMAIN_AIR)
        return TYPE_FLIGHT;
    else
        return TYPE_TASKFORCE;
}

// ========================================================
// Our Dialog functions
// ========================================================

// =============================================
// The main unit dialog
// =============================================

BOOL WINAPI EditUnit(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    WORD  button;
    RECT rect;
    int aggmode, shifted = 0;
    // not int Shift=0,
    int None = 0; //not,Size=0; // Size determines which button to display
    //not int Change=-1;
    int     i, j, ulx, uly;
    Unit E, U = GlobUnit;
    static  Unit  C1;
    char buffer[128];

    // Private buttons:
    /*
     static  RECT elbuts[6][6] = { {{10,195,60,230}, {70,195,100,210}, {105,195,130,210}, {135,195,165,210}, {170,195,195,210}, {200,195,230,210}},
       {{10,235,60,270}, {70,235,100,250}, {105,235,130,250}, {135,235,165,250}, {170,235,195,250}, {200,235,230,250}},
     {{10,280,60,310}, {70,280,100,295}, {105,280,130,295}, {135,280,165,295}, {170,280,195,295}, {200,280,230,295}},
     {{10,320,60,350}, {70,320,100,335}, {105,320,130,335}, {135,320,165,335}, {170,320,195,335}, {200,320,230,335}},
     {{10,360,60,390}, {70,360,100,375}, {105,360,130,375}, {135,360,165,375}, {170,360,195,375}, {200,360,230,375}},
     {{10,420,60,450}, {70,405,100,420}, {105,405,130,420}, {135,405,165,420}, {170,405,195,420}, {200,405,230,420}}};
     static RECT contbuts[10] = {  {120,10,190,55}, {60,22,90,35},
     {200,22,225,35}, {10,140,145,150},
     {10,155,145,170},{10,120,145,130},
     {0,0,0,0},       {0,0,0,0},
     {0,0,0,0},       {0,0,0,0} };
    */
    RECT buttons[10];
    RECT ebuttons[5][6];

    if ( not U or U->CountUnitElements() == 0)
        None = 1;

    GetWindowRect(hDlg, &rect);
    ulx = rect.left + 2; // Upper left of dialog box client are in screen coords
    uly = rect.top + 2;
    SetButtons(hDlg, buttons, ebuttons, ulx, uly);

    button = LOWORD(wParam);

    switch (message)
    {
        case WM_INITDIALOG:              /* message: initialize dialog box */
            GridIndex x, y;

            U->GetLocation(&x, &y);

            // Fill the Domain Combo Box
            SendMessage(GetDlgItem(hDlg, IDC_UNIT_DOMAINCOMBO), CB_ADDSTRING, 0, (LPARAM)"(none)");
            SendMessage(GetDlgItem(hDlg, IDC_UNIT_DOMAINCOMBO), CB_ADDSTRING, 0, (LPARAM)"Abstract");
            SendMessage(GetDlgItem(hDlg, IDC_UNIT_DOMAINCOMBO), CB_ADDSTRING, 0, (LPARAM)"Air");
            SendMessage(GetDlgItem(hDlg, IDC_UNIT_DOMAINCOMBO), CB_ADDSTRING, 0, (LPARAM)"Ground");
            SendMessage(GetDlgItem(hDlg, IDC_UNIT_DOMAINCOMBO), CB_ADDSTRING, 0, (LPARAM)"Naval");
            SendMessage(GetDlgItem(hDlg, IDC_UNIT_DOMAINCOMBO), CB_SETCURSEL, U->GetDomain(), 0);

            // Set up the owner combo box
            for (i = 0; i < NUM_COUNS; i++)
                SendMessage(GetDlgItem(hDlg, IDC_UNIT_OWNERCOMBO), CB_ADDSTRING, 0, (LPARAM)Side[i]);

            SendMessage(GetDlgItem(hDlg, IDC_UNIT_OWNERCOMBO), CB_SETCURSEL, U->GetOwner(), 0);
            ResetBox(hDlg, U);

            // Do Spotdata
            for (i = 0; i < NUM_TEAMS; i++)
                buffer[i] = '0' + (char)U->GetSpotted(i);

            buffer[NUM_TEAMS] = 0;
            SetWindowText(GetDlgItem(hDlg, IDC_UNIT_SPOTVAL), buffer);

            // Initialize our buttons
            // if (GlobUnit->PlayerOk())
            // PostMessage(GetDlgItem(hDlg,IDC_CHECKPLAYEROK),BM_SETCHECK,1,0);
            if (GlobUnit->Scripted())
                PostMessage(GetDlgItem(hDlg, IDC_CHECKSCRIPTED), BM_SETCHECK, 1, 0);

            if (GlobUnit->Commando())
                PostMessage(GetDlgItem(hDlg, IDC_CHECKCOMMANDO), BM_SETCHECK, 1, 0);

            if (GlobUnit->DontPlan())
                PostMessage(GetDlgItem(hDlg, IDC_CHECKFIXED), BM_SETCHECK, 1, 0);

            if (GlobUnit->IsAggregate())
                PostMessage(GetDlgItem(hDlg, IDC_CHECKAGGREGATE), BM_SETCHECK, 1, 0);

            if (GlobUnit->Broken())
                PostMessage(GetDlgItem(hDlg, IDC_CHECKBROKEN), BM_SETCHECK, 1, 0);

            if (GlobUnit->Assigned())
                PostMessage(GetDlgItem(hDlg, IDC_CHECKASSIGNED), BM_SETCHECK, 1, 0);

            sprintf(buffer, "%d,%d", x, y);
            SetWindowText(GetDlgItem(hDlg, IDC_UNIT_LOCVAL), buffer);

            SetFocus(GetDlgItem(hDlg, IDOK));
            return (FALSE);
            break;

        case WM_PAINT:
        {
            HDC hDC, DC;
            PAINTSTRUCT ps, nps;
            HWND hCWnd;
            //not int c=0;

            if ( not U or U->IsDead())
            {
                if (U)
                    vuDatabase->Remove(U);

                EndDialog(hDlg, FALSE);     /* Exits the dialog box        */
            }

            if (GetUpdateRect(hDlg, &rect, FALSE))
            {
                GridIndex x, y;

                hDC = BeginPaint(hDlg, &ps);
                SetWindowText(GetDlgItem(hDlg, IDC_UNIT_OWN2VAL), Side[U->GetOwner()]);
                SetWindowText(GetDlgItem(hDlg, IDC_UNIT_STRVAL), buffer);

                if (U->GetDomain() == DOMAIN_AIR)
                    sprintf(buffer, "%d", U->GetCombatStrength(Air, 0));
                else if (U->GetDomain() == DOMAIN_SEA)
                    sprintf(buffer, "%d", U->GetCombatStrength(Naval, 0));
                else
                    sprintf(buffer, "%d", U->GetCombatStrength(Foot, 0));

                SetWindowText(GetDlgItem(hDlg, IDC_UNIT_STRVAL), buffer);
                U->GetFullName(buffer, 128, FALSE);
                SetWindowText(GetDlgItem(hDlg, IDC_UNIT_NAMEVAL), buffer);
                sprintf(buffer, "%d", U->GetCampID());
                SetWindowText(GetDlgItem(hDlg, IDC_UNIT_IDVAL), buffer);
                sprintf(buffer, "%d", U->GetUnitNameID());
                SetWindowText(GetDlgItem(hDlg, IDC_UNIT_NAMEIDVAL), buffer);
                SetWindowText(GetDlgItem(hDlg, IDC_UNIT_NAMEIDEDIT), buffer);
                x = U->GetUnitReinforcementLevel();
                sprintf(buffer, "%d", x);
                SetWindowText(GetDlgItem(hDlg, IDC_UNIT_REINFORCE_EDIT), buffer);
                CampEntity target = GlobUnit->GetCampTarget();

                if (target)
                {
                    target->GetLocation(&x, &y);
                    sprintf(buffer, "%d bitand %d,%d", target->GetCampID(), x, y);
                }
                else
                    sprintf(buffer, "None");

                SetWindowText(GetDlgItem(hDlg, IDC_UNIT_STRTARGET), buffer);

                SetOptionalValues(hDlg, U);
                ShowElementInfo(hDC, hDlg, U, 1, 0, asAgg);

                hCWnd = GetDlgItem(hDlg, IDC_UNIT_MAIN);
                DC = BeginPaint(hCWnd, &nps);
                DisplayUnit(DC, U, 0, 0, 48);
                EndPaint(hCWnd, &nps);

                EndPaint(hDlg, &ps);
            }
        }


        case WM_COMMAND:                 /* message: received a command */
            switch (button)
            {
                case IDOK:     /* "OK" box selected?        */
                case IDCANCEL:
                    TeamInfo[GlobUnit->GetTeam()]->SetActive(1);
                    EndDialog(hDlg, TRUE);     /* Exits the dialog box        */
                    return (TRUE);
                    break;

                case IDC_BUTTONDELETE:
                    // Remove Children
                    E = U->GetFirstUnitElement();

                    while (E)
                    {
                        U->RemoveChild(E->Id());
                        vuDatabase->Remove(E);
                        E = U->GetFirstUnitElement();
                    }

                    // Kill the unit
                    U->KillUnit();
                    vuDatabase->Remove(U);
                    // Remove parent, if we're the last element
                    E = U->GetUnitParent();

                    if (E and not E->GetFirstUnitElement())
                        vuDatabase->Remove(E);

                    GlobUnit = NULL;
                    asAgg = 1;
                    EndDialog(hDlg, TRUE);     /* Exits the dialog box        */
                    return TRUE;
                    break;

                case IDC_BUTTONPLAN:
                    break;

                case IDC_BUTTONSAVE:
                    SaveAsScriptedUnitFile(hMainWnd);
                    break;

                case IDC_BUTTONWAYPOINTS:
                    MainMapData->ShowWPs = TRUE;
                    WPUnit = U;
                    EndDialog(hDlg, TRUE);     /* Exits the dialog box        */
                    return (TRUE);
                    break;

                case IDC_UNIT_NAMEIDEDIT:
                    if (HIWORD(wParam) == EN_KILLFOCUS)
                    {
                        GetDlgItemText(hDlg, IDC_UNIT_NAMEIDEDIT, buffer, 79);
                        i = atoi(buffer);

                        if (i)
                            GlobUnit->SetUnitNameID(i);

                        RefreshNames(hDlg);
                    }

                    break;

                case IDC_BUTTONNEXTUNIT:
                    DisplayNextInStack(hDlg, U);
                    break;

                case IDC_UNIT_DOMAINCOMBO:
                    if (HIWORD(wParam) == CBN_SELENDOK)
                    {
                        i = SendMessage(GetDlgItem(hDlg, IDC_UNIT_DOMAINCOMBO), CB_GETCURSEL, 0, 0);

                        if (i >= 0)
                        {
                            if (i == DOMAIN_SEA)
                                GlobUnit = U = ConvertUnit(U, i, TYPE_TASKFORCE, 1, 1);
                            else if (i == DOMAIN_AIR)
                                GlobUnit = U = ConvertUnit(U, i, TYPE_SQUADRON, 1, 1);
                            else if (i == DOMAIN_LAND)
                                GlobUnit = U = ConvertUnit(U, i, TYPE_BRIGADE, 1, 1);
                        }

                        ResetBox(hDlg, U);
                        RefreshControls(hDlg);
                    }

                    break;

                case IDC_UNIT_SIZECOMBO:
                    if (HIWORD(wParam) == CBN_SELENDOK)
                    {
                        i = SendMessage(GetDlgItem(hDlg, IDC_UNIT_SIZECOMBO), CB_GETCURSEL, 0, 0);

                        if (i > 0)
                        {
                            if (U->GetDomain() == DOMAIN_LAND and i == TYPE_BATTALION)
                                GlobUnit = U = ConvertUnit(U, U->GetDomain(), i, 1, 2);
                            else
                                GlobUnit = U = ConvertUnit(U, U->GetDomain(), i, 1, 1);
                        }

                        ResetBox(hDlg, U);
                        RefreshControls(hDlg);
                    }

                    break;

                case IDC_UNIT_TYPECOMBO:
                    if (HIWORD(wParam) == CBN_SELENDOK)
                    {
                        i = SendMessage(GetDlgItem(hDlg, IDC_UNIT_TYPECOMBO), CB_GETCURSEL, 0, 0);

                        if (i >= 0)
                            U->SetUnitSType(i);

                        ResetBox(hDlg, U);
                    }

                    break;

                case IDC_UNIT_VEHICLECOMBO:
                    if (HIWORD(wParam) == CBN_SELENDOK)
                    {
                        i = SendMessage(GetDlgItem(hDlg, IDC_UNIT_VEHICLECOMBO), CB_GETCURSEL, 0, 0);

                        if (i >= 0)
                            U->SetUnitSPType(SPTable[i]);

                        InvalidateRect(hDlg, NULL, FALSE);
                    }

                    break;

                case IDC_UNIT_OWNERCOMBO:
                    if (HIWORD(wParam) == CBN_SELENDOK)
                    {
                        i = SendMessage(GetDlgItem(hDlg, IDC_UNIT_OWNERCOMBO), CB_GETCURSEL, 0, 0);

                        if (i >= 0)
                            U->SetOwner(i);

                        ResetBox(hDlg, U);
                        RefreshControls(hDlg);
                    }

                    break;

                case IDC_UNIT_REINFORCE_EDIT:
                    if (HIWORD(wParam) == EN_KILLFOCUS)
                    {
                        GetDlgItemText(hDlg, IDC_UNIT_REINFORCE_EDIT, buffer, 79);
                        i = atoi(buffer);
                        U->SetUnitReinforcementLevel(i);
                        U->SetInactive(i);

                        if (U->Father())
                        {
                            for (j = 0; j < MAX_UNIT_CHILDREN; j++)
                            {
                                E = U->GetUnitElement(j);

                                if (E)
                                {
                                    E->SetUnitReinforcementLevel(i);
                                    E->SetInactive(i);
                                }
                            }
                        }
                    }

                    break;

                case IDC_UNIT_OPT1COMBO:
                case IDC_UNIT_OPT2COMBO:
                case IDC_UNIT_OPT2EDIT:
                case IDC_UNIT_OPT3EDIT:
                case IDC_UNIT_OPT4EDIT:
                case IDC_UNIT_OPT5EDIT:
                case IDC_UNIT_OPT6EDIT:
                    ParseOptionalButtons(hDlg, button, HIWORD(wParam), U);
                    break;

                case IDC_CHECKPLAYEROK:
                    // if (GlobUnit->PlayerOk())
                    // GlobUnit->SetPlayerOk(0);
                    // else
                    // GlobUnit->SetPlayerOk(1);
                    break;

                case IDC_CHECKSCRIPTED:
                    if (GlobUnit->Scripted())
                        GlobUnit->SetScripted(0);
                    else
                        GlobUnit->SetScripted(1);

                    break;

                case IDC_CHECKCOMMANDO:
                    if (GlobUnit->Commando())
                        GlobUnit->SetCommando(0);
                    else
                        GlobUnit->SetCommando(1);

                    break;

                case IDC_CHECKFIXED:
                    if (GlobUnit->DontPlan())
                        GlobUnit->SetDontPlan(0);
                    else
                        GlobUnit->SetDontPlan(1);

                    break;

                default:
                    break;
            }

            if (button)
                PostMessage(hDlg, WM_PAINT, 0, 0);

            break;

        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
        {
            WORD xPos, yPos;

            xPos = LOWORD(lParam);  // horizontal position of cursor
            yPos = HIWORD(lParam);  // vertical position of cursor

            // Check for clicks in element boxes
            for (i = 0; i < 5; i++)
                for (j = 0; j < 6; j++)
                {
                    if (inButton(&ebuttons[i][j], xPos, yPos))
                    {
                        int k, l;

                        SetFocus(GetDlgItem(hDlg, IDOK));
                        E = NULL;

                        if ( not U->Real())
                        {
                            E = GlobUnit->GetFirstUnitElement();
                            k = i;
                            l = j;

                            while (E and k)
                            {
                                E = GlobUnit->GetNextUnitElement();
                                k--;
                            }

                            if (j and E)
                            {
                                E = E->GetFirstUnitElement();
                                l--;

                                while (E and l)
                                {
                                    E = GlobUnit->GetNextUnitElement();
                                    l--;
                                }
                            }
                        }

                        // Do stuff with the unit:
                        if (message == WM_LBUTTONDBLCLK and E)
                        {
                            GlobUnit = E;
                            aggmode = asAgg;
                            asAgg = 1;
                            DialogBox(hInst, MAKEINTRESOURCE(IDD_UNITDIALOG1), hDlg, (DLGPROC)EditUnit);
                            GlobUnit = U;
                            asAgg = aggmode;
                            GetClientRect(hDlg, &rect);
                            InvalidateRect(hDlg, &rect, TRUE);
                        }

                        if (message == WM_LBUTTONDBLCLK and not E and not U->Real())
                        {
                            GridIndex x, y;

                            if ( not j) // Primary child
                            {
                                if (U->GetType() == TYPE_BRIGADE)
                                    E = NewUnit(U->GetDomain(), GetNextType(U), U->GetSType(), 2, U);
                                else
                                    E = NewUnit(U->GetDomain(), GetNextType(U), U->GetSType(), U->GetSPType(), U);

                                E->SetOwner(U->GetOwner());
                                E->SetUnitNameID(i + 1);
                            }

                            /* else if (U->GetType() == TYPE_DIVISION)
                             {
                             E = GlobUnit->GetFirstUnitElement();
                             k=i;
                             while (E and k)
                             {
                             E = GetNextUnit(E);
                             k--;
                             }
                             if (E)
                             E = CreateUnit(E->GetOwner(), E->GetDomain(), GetNextType(E), E->GetSType(), E->GetSPType(), E);
                             }
                            */
                            U->GetLocation(&x, &y);
                            E->SetLocation(x, y);
                            E->SetUnitDestination(x, y);
                        }

                        if (message == WM_LBUTTONDOWN)
                        {

                            /*
                             if ( not E)
                             ;  // Add the unit
                             else if (E==C1)
                             C1->SetUnitSType((char)(C1->GetSType()-1));
                             else
                             C1 = E;
                            */
                        }

                        if (message == WM_RBUTTONDOWN and E and E == C1)
                        {
                            C1->SetUnitSType((char)(C1->GetSType() + 1));
                        }

                        InvalidateRect(hDlg, &ebuttons[i][j], TRUE);
                        PostMessage(hDlg, WM_PAINT, 0, 0);
                    }
                }

            for (i = 0; i < 10; i++)
            {
                if (inButton(&buttons[i], xPos, yPos))
                {
                    SetFocus(GetDlgItem(hDlg, IDOK));

                    switch (i)
                    {
                        case 0:
                            if (asAgg and message == WM_LBUTTONDBLCLK)
                            {
                                U = GlobUnit;
                                GlobUnit = U->GetUnitParent();

                                if (GlobUnit and GlobUnit not_eq U)
                                    DialogBox(hInst, MAKEINTRESOURCE(IDD_UNITDIALOG1), hDlg, (DLGPROC)EditUnit);

                                GlobUnit = U;
                                GetClientRect(hDlg, &rect);
                                InvalidateRect(hDlg, &rect, TRUE);
                            }

                            break;

                        case 8:
                            if (message == WM_RBUTTONDOWN)
                                U->ChangeVehicles(1);
                            else
                                U->ChangeVehicles(-1);

                            break;

                        default:
                            break;
                    }

                    InvalidateRect(hDlg, &buttons[i], TRUE);
                    PostMessage(hDlg, WM_PAINT, 0, 0);
                }
            }

            break;
        }

        case WM_KEYDOWN:
        {
            int      C;

            if ((int)wParam == 16)
                shifted = TRUE;

            C = (shifted ? (int)wParam : (int)wParam + 0x20);
            break;
        }
    }

    return (FALSE);                    /* Didn't process a message    */
    // avoid compiler warnings at W3
    lParam;
}

BOOL WINAPI EditWayPoint(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    WayPoint nw, w = GlobWP;
    RECT rect;
    GridIndex x, y;
    static int targettype, targetid, firstwpa, lastwpa;
    int i;
    char buffer[40];
    CampEntity e;

    if ( not w)
    {
        EndDialog(hDlg, TRUE);        /* Exits the dialog box        */
        return (FALSE);
    }

    switch (message)
    {
        case WM_INITDIALOG:              /* message: initialize dialog box */
            e = GlobWP->GetWPTarget();

            if (e)
            {
                targettype = e->GetClass();
                targetid = e->GetCampID();
            }
            else
            {
                targettype = 0;
                targetid = 0;
            }

            w->GetWPLocation(&x, &y);
            sprintf(buffer, "%d,%d", x, y);
            SetWindowText(GetDlgItem(hDlg, IDC_WP_LOCVAL), buffer);
            sprintf(buffer, "%s", GetTimeString(w->GetWPArrivalTime(), buffer));
            SetWindowText(GetDlgItem(hDlg, IDC_WP_ARRIVEVAL), buffer);
            sprintf(buffer, "%s", GetTimeString(w->GetWPDepartureTime(), buffer));
            SetWindowText(GetDlgItem(hDlg, IDC_WP_DEPARTVAL), buffer);
            sprintf(buffer, "%d", w->GetWPAltitude());
            SetWindowText(GetDlgItem(hDlg, IDC_WP_ALTEDIT), buffer);
            sprintf(buffer, "%d", w->GetWPStationTime());
            SetWindowText(GetDlgItem(hDlg, IDC_WP_STATIONEDIT), buffer);
            sprintf(buffer, "%d", targetid);
            SetWindowText(GetDlgItem(hDlg, IDC_WP_TARGETIDEDIT), buffer);
            sprintf(buffer, "%d", w->GetWPTargetBuilding());
            SetWindowText(GetDlgItem(hDlg, IDC_WP_SPECIFICEDIT), buffer);

            if (GlobUnit and GlobUnit->GetDomain() == DOMAIN_LAND)
            {
                firstwpa = WP_MOVEOPPOSED - 1;
                lastwpa = WP_SECURE;
            }
            else if (GlobUnit and GlobUnit->GetDomain() == DOMAIN_AIR)
            {
                firstwpa = 0;
                lastwpa = WP_FAC;
            }
            else
            {
                firstwpa = 0;
                lastwpa = WP_SECURE;
            }

            SendMessage(GetDlgItem(hDlg, IDC_WP_ACTIONCOMBO), WM_SETREDRAW, 0, 0);

            for (i = firstwpa; i <= lastwpa; i++)
                SendMessage(GetDlgItem(hDlg, IDC_WP_ACTIONCOMBO), CB_ADDSTRING, 0, (LPARAM)WPActStr[i]);

            SendMessage(GetDlgItem(hDlg, IDC_WP_ACTIONCOMBO), CB_SETCURSEL, GlobWP->GetWPAction() - firstwpa, 0);
            SendMessage(GetDlgItem(hDlg, IDC_WP_ACTIONCOMBO), WM_SETREDRAW, 1, 0);

            SendMessage(GetDlgItem(hDlg, IDC_WP_ENROUTECOMBO), WM_SETREDRAW, 0, 0);

            for (i = firstwpa; i <= lastwpa; i++)
                SendMessage(GetDlgItem(hDlg, IDC_WP_ENROUTECOMBO), CB_ADDSTRING, 0, (LPARAM)WPActStr[i]);

            SendMessage(GetDlgItem(hDlg, IDC_WP_ENROUTECOMBO), CB_SETCURSEL, GlobWP->GetWPRouteAction() - firstwpa, 0);
            SendMessage(GetDlgItem(hDlg, IDC_WP_ENROUTECOMBO), WM_SETREDRAW, 1, 0);

            SendMessage(GetDlgItem(hDlg, IDC_WP_TARGETCOMBO), WM_SETREDRAW, 0, 0);

            for (i = 0; i < 3; i++)
                SendMessage(GetDlgItem(hDlg, IDC_WP_TARGETCOMBO), CB_ADDSTRING, 0, (LPARAM)TargetTypeStr[i]);

            if (targettype == CLASS_UNIT)
                SendMessage(GetDlgItem(hDlg, IDC_WP_TARGETCOMBO), CB_SETCURSEL, 2, 0);
            else if (targettype == CLASS_OBJECTIVE)
                SendMessage(GetDlgItem(hDlg, IDC_WP_TARGETCOMBO), CB_SETCURSEL, 1, 0);
            else
                SendMessage(GetDlgItem(hDlg, IDC_WP_TARGETCOMBO), CB_SETCURSEL, 0, 0);

            SendMessage(GetDlgItem(hDlg, IDC_WP_TARGETCOMBO), WM_SETREDRAW, 1, 0);

            SendMessage(GetDlgItem(hDlg, IDC_WP_FORMATIONCOMBO), WM_SETREDRAW, 0, 0);

            for (i = 0; i < 3; i++)
                SendMessage(GetDlgItem(hDlg, IDC_WP_FORMATIONCOMBO), CB_ADDSTRING, 0, (LPARAM)FormStr[i]);

            SendMessage(GetDlgItem(hDlg, IDC_WP_FORMATIONCOMBO), CB_SETCURSEL, GlobWP->GetWPFormation(), 0);
            SendMessage(GetDlgItem(hDlg, IDC_WP_FORMATIONCOMBO), WM_SETREDRAW, 1, 0);
            break;

        case WM_PAINT:
        {
            HDC hDC;
            PAINTSTRUCT ps;

            if (GetUpdateRect(hDlg, &rect, FALSE))
            {
                hDC = BeginPaint(hDlg, &ps);
                w->GetWPLocation(&x, &y);
                sprintf(buffer, "%d,%d", x, y);
                SetWindowText(GetDlgItem(hDlg, IDC_WP_LOCVAL), buffer);
                sprintf(buffer, "%s", GetTimeString(w->GetWPArrivalTime(), buffer));
                SetWindowText(GetDlgItem(hDlg, IDC_WP_ARRIVEVAL), buffer);
                sprintf(buffer, "%s", GetTimeString(w->GetWPDepartureTime(), buffer));
                SetWindowText(GetDlgItem(hDlg, IDC_WP_DEPARTVAL), buffer);
                EndPaint(hDlg, &ps);
            }
        }
        break;

        case WM_COMMAND:                 /* message: received a command */
            switch (LOWORD(wParam))
            {
                case IDOK:    /* "OK" box selected?        */
                case IDCANCEL:
                    EndDialog(hDlg, TRUE);     /* Exits the dialog box        */
                    return (TRUE);
                    break;

                case IDC_ADD:
                    EndDialog(hDlg, TRUE);     /* Exits the dialog box        */
                    nw = new WayPointClass();
                    nw->CloneWP(w);
                    nw->InsertWP(w->GetNextWP());
                    w->InsertWP(nw);

                    if (nw)
                    {
                        GlobWP = nw;
                        DialogBox(hInst, MAKEINTRESOURCE(IDD_WPDIALOG), hDlg, (DLGPROC)EditWayPoint);
                        GlobWP = w;
                    }

                    return TRUE;
                    break;

                case IDC_BUTTONDELETE:
                    WPUnit->DeleteUnitWP(w);
                    EndDialog(hDlg, TRUE);     /* Exits the dialog box        */
                    return TRUE;
                    break;

                case IDC_WP_ACTIONCOMBO:
                    if (HIWORD(wParam) == CBN_SELENDOK)
                    {
                        i = SendMessage(GetDlgItem(hDlg, IDC_WP_ACTIONCOMBO), CB_GETCURSEL, 0, 0);

                        if (i >= 0)
                            GlobWP->SetWPAction(i + firstwpa);
                    }

                    break;

                case IDC_WP_ENROUTECOMBO:
                    if (HIWORD(wParam) == CBN_SELENDOK)
                    {
                        i = SendMessage(GetDlgItem(hDlg, IDC_WP_ENROUTECOMBO), CB_GETCURSEL, 0, 0);

                        if (i >= 0)
                            GlobWP->SetWPRouteAction(i + firstwpa);
                    }

                    break;

                case IDC_WP_FORMATIONCOMBO:
                    if (HIWORD(wParam) == CBN_SELENDOK)
                    {
                        i = SendMessage(GetDlgItem(hDlg, IDC_WP_FORMATIONCOMBO), CB_GETCURSEL, 0, 0);

                        if (i >= 0)
                            GlobWP->SetWPFormation(i);
                    }

                    break;

                case IDC_WP_TARGETCOMBO:
                    if (HIWORD(wParam) == CBN_SELENDOK)
                    {
                        targettype = SendMessage(GetDlgItem(hDlg, IDC_WP_TARGETCOMBO), CB_GETCURSEL, 0, 0);

                        if (targettype == 1)
                        {
                            Objective o;
                            targettype = CLASS_OBJECTIVE;
                            o = GetObjectiveByXY(CurX, CurY);

                            if (o)
                            {
                                targetid = o->GetCampID();
                            }

                            sprintf(buffer, "%d", targetid);
                            SetWindowText(GetDlgItem(hDlg, IDC_WP_TARGETIDEDIT), buffer);
                            GlobWP->SetWPTarget(o->Id());
                        }
                        else if (targettype == 2)
                        {
                            Unit u;
                            targettype = CLASS_UNIT;

                            u = FindUnitByXY(AllRealList, CurX, CurY, 0);

                            if (u)
                                targetid = u->GetCampID();

                            sprintf(buffer, "%d", targetid);
                            SetWindowText(GetDlgItem(hDlg, IDC_WP_TARGETIDEDIT), buffer);
                            GlobWP->SetWPTarget(u->Id());
                        }
                    }

                    break;

                case IDC_WP_ALTEDIT:
                    GetDlgItemText(hDlg, IDC_WP_ALTEDIT, buffer, 79);
                    i = atoi(buffer);

                    if (HIWORD(wParam) == EN_KILLFOCUS)
                        GlobWP->SetWPAltitude(i);

                    break;

                default:
                    break;
            }

            break;

        default:
            break;
    }

    return (FALSE);                    /* Didn't process a message    */
    // avoid compiler warnings at W3
    lParam;
}

// =========================================
// Selects a unit's mission
// =========================================

F4PFList ResetNewSquadron(HWND hDlg, Unit squadron, F4PFList flights)
{
    char buffer[120];
    Unit u;
    GridIndex x, y;
    RECT rect;

    if ( not squadron)
        return NULL;

    squadron->GetLocation(&x, &y);
    sprintf(buffer, "%d,%d", x, y);
    SetWindowText(GetDlgItem(hDlg, IDC_BASESTATIC), buffer);
    SetWindowText(GetDlgItem(hDlg, IDC_SPECIALSTATIC), SpecialStr[squadron->GetUnitSpecialty()]);
    flights->Purge();

    {
        VuListIterator myit(AllAirList);
        u = GetFirstUnit(&myit);

        while (u)
        {
            if (
                u->GetDomain() == DOMAIN_AIR and 
                u->GetType() == TYPE_FLIGHT and 
                u->GetUnitSquadronID() == squadron->Id()
            )
            {
                flights->ForcedInsert(u);
            }

            u = GetNextUnit(&myit);
        }
    }

    GetClientRect(hDlg, &rect);
    InvalidateRect(hDlg, &rect, TRUE);
    return flights;
}

BOOL WINAPI SelectMission(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    RECT rect;
    Unit u;
    static Unit squadron = NULL, flight = NULL;
    int i;
    GridIndex x, y;
    char buffer[120];
    static FalconPrivateList *squadrons;
    static FalconPrivateList *flights;

    switch (message)
    {
        case WM_INITDIALOG:              /* message: initialize dialog box */
        {
            squadrons = new FalconPrivateList(&AllAirFilter);
            squadrons->Register();
            flights = new FalconPrivateList(&AllAirFilter);
            flights->Register();

            if ( not squadrons or not flights)
                return FALSE;

            squadron = flight = NULL;
            {
                VuListIterator myit(AllAirList);
                u = GetFirstUnit(&myit);

                while (u)
                {
                    if (u->GetDomain() == DOMAIN_AIR and u->GetType() == TYPE_SQUADRON)
                        squadrons->ForcedInsert(u);

                    u = GetNextUnit(&myit);
                }
            }

            SendMessage(GetDlgItem(hDlg, IDC_SQUADRONCOMBO), WM_SETREDRAW, 0, 0);
            SendMessage(GetDlgItem(hDlg, IDC_SQUADRONCOMBO), CB_ADDSTRING, 0, (LPARAM)"None");
            SendMessage(GetDlgItem(hDlg, IDC_SQUADRONCOMBO), CB_SETCURSEL, 0, 0);
            squadron = (Unit)FalconLocalSession->GetPlayerSquadron();

            {
                VuListIterator sit(squadrons);
                u = GetFirstUnit(&sit);
                i = 0;

                while (u)
                {
                    i++;
                    sprintf(buffer, "Squadron %d - %s", u->GetUnitNameID(), GetVehicleName(u->GetVehicleID(0)));
                    SendMessage(GetDlgItem(hDlg, IDC_SQUADRONCOMBO), CB_ADDSTRING, 0, (LPARAM)buffer);

                    if ( not squadron)
                    {
                        squadron = u;
                        FalconLocalSession->SetPlayerSquadron((Squadron)squadron);
                    }

                    if (u == squadron)
                        SendMessage(GetDlgItem(hDlg, IDC_SQUADRONCOMBO), CB_SETCURSEL, i, 0);

                    u = GetNextUnit(&sit);
                }
            }
            SendMessage(GetDlgItem(hDlg, IDC_SQUADRONCOMBO), WM_SETREDRAW, 1, 0);
            ResetNewSquadron(hDlg, squadron, flights);
            SetWindowPos(hDlg, NULL, 0, 0, 0, 0, SWP_NOSIZE bitor SWP_NOZORDER);
            return (TRUE);
        }
        break;

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC DC;
            WayPoint w;
            int radio, text, target, tot, cs;
            CampaignTime dt;
            VehicleClassDataType *vc;

            if (GetUpdateRect(hDlg, &rect, FALSE) and flights)
            {
                VuListIterator fit(flights);
                DC = BeginPaint(hDlg, &ps);
                u = GetFirstUnit(&fit);
                i = 0;

                while (u and i < 16)
                {
                    radio = IDC_MISS_MISSION1 + i;
                    text = IDC_STATIC1 + i;
                    tot = IDC_TOT1 + i;
                    target = IDC_TARGET1 + i;
                    cs = IDC_CS_1 + i;
                    ShowWindow(GetDlgItem(hDlg, radio), 1);
                    ShowWindow(GetDlgItem(hDlg, text), 1);
                    ShowWindow(GetDlgItem(hDlg, tot), 1);
                    ShowWindow(GetDlgItem(hDlg, target), 1);
                    ShowWindow(GetDlgItem(hDlg, cs), 1);
                    i++;

                    w = u->GetCurrentUnitWP();

                    if (w)
                    {
                        dt = w->GetWPArrivalTime();

                        if (w->GetWPAction() not_eq WP_TAKEOFF)
                            sprintf(buffer, "In Progress");
                        else
                            sprintf(buffer, "%s", GetTimeString(dt, buffer));
                    }
                    else
                        sprintf(buffer, "%s", GetTimeString(0, buffer));

                    SetWindowText(GetDlgItem(hDlg, tot), buffer);
                    // Hacked target for now
                    w = u->GetFirstUnitWP();

                    while (w)
                    {
                        if (w->GetWPFlags() bitand WPF_TARGET)
                        {
                            CampEntity e;
                            w->GetWPLocation(&x, &y);
                            e = w->GetWPTarget();

                            if (e and e->IsUnit())
                                sprintf(buffer, "Unit at %d,%d", x, y);
                            else if (e and e->IsObjective())
                                sprintf(buffer, "Objective at %d,%d", x, y);
                            else
                                sprintf(buffer, "Location at %d,%d", x, y);
                        }

                        w = w->GetNextWP();
                    }

                    SetWindowText(GetDlgItem(hDlg, target), buffer);
                    sprintf(buffer, "%s", MissStr[u->GetUnitMission()]);
                    SetWindowText(GetDlgItem(hDlg, text), buffer);
                    sprintf(buffer, "%d", u->GetCampID());
                    SetWindowText(GetDlgItem(hDlg, radio), buffer);
                    vc = GetVehicleClassData(u->GetVehicleID(0));
                    _stprintf(buffer, vc->Name);
                    GetCallsign(((Flight)u)->callsign_id, ((Flight)u)->callsign_num, buffer);
                    SetWindowText(GetDlgItem(hDlg, cs), buffer);
                    u = GetNextUnit(&fit);
                }

                for (; i < 16; i++)
                {
                    ShowWindow(GetDlgItem(hDlg, IDC_MISS_MISSION1 + i), 0);
                    ShowWindow(GetDlgItem(hDlg, IDC_STATIC1 + i), 0);
                    ShowWindow(GetDlgItem(hDlg, IDC_TOT1 + i), 0);
                    ShowWindow(GetDlgItem(hDlg, IDC_TARGET1 + i), 0);
                    ShowWindow(GetDlgItem(hDlg, IDC_CS_1 + i), 0);
                }

                EndPaint(hDlg, &ps);
            }
        }
        break;

        case WM_COMMAND:                 /* message: received a command */
            switch (LOWORD(wParam))
            {
                case IDOK:
                case IDCANCEL:
                    squadrons->Unregister();
                    delete squadrons;
                    flights->Unregister();
                    delete flights;
                    squadrons = NULL;
                    flights = NULL;
                    EndDialog(hDlg, TRUE);        /* Exits the dialog box        */
                    return (TRUE);
                    break;

                case IDC_SQUADRONCOMBO:
                    if (HIWORD(wParam) == CBN_SELENDOK)
                    {
                        i = SendMessage(GetDlgItem(hDlg, IDC_SQUADRONCOMBO), CB_GETCURSEL, 0, 0);

                        if (i)
                        {
                            VuListIterator sit(squadrons);
                            squadron = GetFirstUnit(&sit);
                            i--;

                            while (i)
                            {
                                squadron = GetNextUnit(&sit);
                                i--;
                            }
                        }
                        else
                        {
                            squadron = NULL;
                        }

                        ResetNewSquadron(hDlg, squadron, flights);
                    }

                    break;

                case IDC_MISS_MISSION1:
                case IDC_MISS_MISSION2:
                case IDC_MISS_MISSION3:
                case IDC_MISS_MISSION4:
                case IDC_MISS_MISSION5:
                case IDC_MISS_MISSION6:
                case IDC_MISS_MISSION7:
                case IDC_MISS_MISSION8:
                case IDC_MISS_MISSION9:
                case IDC_MISS_MISSION10:
                case IDC_MISS_MISSION11:
                case IDC_MISS_MISSION12:
                case IDC_MISS_MISSION13:
                case IDC_MISS_MISSION14:
                case IDC_MISS_MISSION15:
                case IDC_MISS_MISSION16:
                {
                    VuListIterator fit(flights);
                    i = LOWORD(wParam) - IDC_MISS_MISSION1;
                    flight = GetFirstUnit(&fit);

                    while (i > 0)
                    {
                        flight = GetNextUnit(&fit);
                        i--;
                    }

                    FalconLocalSession->SetPlayerFlight((Flight)flight);
                }
                break;

                case IDC_MISS_FLY:
                    // Deaggregate flight
                    EndDialog(hDlg, TRUE);        /* Exits the dialog box        */
                    return (TRUE);
                    break;

                case IDC_MISS_FOG:
                    if (flight)
                        DialogBox(hInst, MAKEINTRESOURCE(IDD_FISTOFGOD), FalconDisplay.appWin, (DLGPROC)FistOfGod);

                    // Continue on to debrief for Fist of God...
                case IDC_DEBRIEF:
                    if (flight)
                    {
                        _TCHAR brief_string[16384];
                        char btitle[40] = "Mission Debrief";

                        BuildCampDebrief(brief_string);
                        BSP = brief_string;
                        BTP = btitle;
                        DialogBox(hInst, MAKEINTRESOURCE(IDD_BRIEFDIALOG), FalconDisplay.appWin, (DLGPROC)BriefDialog);
                        MessageBox(hDlg, brief_string, "Mission Debrief", MB_OK bitor MB_ICONINFORMATION bitor MB_SETFOREGROUND);
                    }

                    break;

                case IDC_BRIEF:
                    if (flight)
                    {
                        _TCHAR brief_string[8192];
                        char btitle[40] = "Mission Brief";

                        BuildCampBrief(brief_string);
                        BSP = brief_string;
                        BTP = btitle;
                        DialogBox(hInst, MAKEINTRESOURCE(IDD_BRIEFDIALOG), FalconDisplay.appWin, (DLGPROC)BriefDialog);
                        MessageBox(hDlg, brief_string, "Mission Brief", MB_OK bitor MB_ICONINFORMATION bitor MB_SETFOREGROUND);
                    }

                    break;

                default:
                    break;
            }

            break;

        default:
            break;
    }

    return (FALSE);                    /* Didn't process a message    */
    // avoid compiler warnings at W3
    lParam;
}


// This is for Ally - Temporary for Oct 4 Demo - KCK
BOOL WINAPI SelectSquadron(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{

    RECT rect;
    Unit u;
    static Unit squadron = NULL;
    int i;
    GridIndex x, y;
    char buffer[120];
    static FalconPrivateList *squadrons = NULL;

    switch (message)
    {
        case WM_INITDIALOG:              /* message: initialize dialog box */
        {
            squadrons = new FalconPrivateList(&AllAirFilter);
            squadrons->Register();
            squadron = NULL;
            {
                VuListIterator airit(AllAirList);
                u = GetFirstUnit(&airit);

                while (u)
                {
                    if (u->GetDomain() == DOMAIN_AIR and u->GetType() == TYPE_SQUADRON)
                        squadrons->ForcedInsert(u);

                    u = GetNextUnit(&airit);
                }
            }

            SendMessage(GetDlgItem(hDlg, IDC_SQUADRONCOMBO), WM_SETREDRAW, 0, 0);
            SendMessage(GetDlgItem(hDlg, IDC_SQUADRONCOMBO), CB_ADDSTRING, 0, (LPARAM)"None");
            SendMessage(GetDlgItem(hDlg, IDC_SQUADRONCOMBO), CB_SETCURSEL, 0, 0);
            {
                VuListIterator sit(squadrons);
                u = GetFirstUnit(&sit);

                while (u)
                {
                    u->GetName(buffer, 120, FALSE);
                    SendMessage(GetDlgItem(hDlg, IDC_SQUADRONCOMBO), CB_ADDSTRING, 0, (LPARAM)buffer);
                    u = GetNextUnit(&sit);
                }
            }
            SendMessage(GetDlgItem(hDlg, IDC_SQUADRONCOMBO), WM_SETREDRAW, 1, 0);

            if ( not squadron)
            {
                /* squadron = GetFirstUnit(&sit);*/
                SendMessage(GetDlgItem(hDlg, IDC_SQUADRONCOMBO), CB_SETCURSEL, 1, 0);
            }

            return (TRUE);
        }

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC DC;

            if (GetUpdateRect(hDlg, &rect, FALSE))
            {
                DC = BeginPaint(hDlg, &ps);

                if (squadron)
                {
                    squadron->GetLocation(&x, &y);
                    sprintf(buffer, "%d,%d", x, y);
                    SetWindowText(GetDlgItem(hDlg, IDC_BASESTATIC), buffer);
                    SetWindowText(GetDlgItem(hDlg, IDC_SPECIALSTATIC), SpecialStr[squadron->GetUnitSpecialty()]);
                    sprintf(buffer, "%d %s", squadron->GetTotalVehicles(), GetVehicleName(squadron->GetVehicleID(0)));
                    SetWindowText(GetDlgItem(hDlg, IDC_SQUAD_ACVAL), buffer);
                }
                else
                {
                    SetWindowText(GetDlgItem(hDlg, IDC_BASESTATIC), "0,0");
                    SetWindowText(GetDlgItem(hDlg, IDC_SPECIALSTATIC), "General");
                    SetWindowText(GetDlgItem(hDlg, IDC_SQUAD_ACVAL), "0");
                }

                EndPaint(hDlg, &ps);
            }
        }

        case WM_COMMAND:                 /* message: received a command */
            switch (LOWORD(wParam))
            {
                case IDOK:
                    squadrons->Unregister();
                    delete squadrons;
                    squadrons = NULL;
                    FalconLocalSession->SetPlayerSquadron((Squadron)squadron);
                    EndDialog(hDlg, TRUE);        /* Exits the dialog box        */
                    return (TRUE);

                case IDCANCEL:
                    squadrons->Unregister();
                    delete squadrons;
                    squadrons = NULL;
                    FalconLocalSession->SetPlayerSquadron(NULL);
                    EndDialog(hDlg, FALSE);        /* Exits the dialog box        */
                    return (TRUE);
                    break;

                case IDC_SQUADRONCOMBO:
                    if (HIWORD(wParam) == CBN_SELENDOK)
                    {
                        i = SendMessage(GetDlgItem(hDlg, IDC_SQUADRONCOMBO), CB_GETCURSEL, 0, 0);

                        if (i)
                        {
                            VuListIterator sit(squadrons);
                            squadron = GetFirstUnit(&sit);
                            i--;

                            while (i)
                            {
                                squadron = GetNextUnit(&sit);
                                i--;
                            }
                        }
                        else
                        {
                            squadron = NULL;
                        }

                        GetClientRect(hDlg, &rect);
                        InvalidateRect(hDlg, &rect, TRUE);
                    }

                    break;

                default:
                    break;
            }

            break;

        default:
            break;
    }

    return (FALSE);                    /* Didn't process a message    */
    // avoid compiler warnings at W3
    lParam;
}

BOOL WINAPI BriefDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG:              /* message: initialize dialog box */
            // SetTitle(BTP);
            SetWindowText(GetDlgItem(hDlg, IDC_BRIEF_TEXT), BSP);
            return (TRUE);
            break;

        case WM_PAINT:
            break;

        case WM_COMMAND:                 /* message: received a command */
            switch (LOWORD(wParam))
            {
                case IDOK:
                    EndDialog(hDlg, TRUE);      /* Exits the dialog box        */
                    return (TRUE);

                default:
                    break;
            }

            break;

        default:
            break;
    }

    return (FALSE);                    /* Didn't process a message    */
    // avoid compiler warnings at W3
    lParam;
}

#endif CAMPTOOL

