#include <cISO646>
#include <windows.h>    // includes basic windows functionality
#include <commdlg.h>    // includes common dialog functionality
#include <commctrl.h>
#include <dlgs.h>       // includes common dialog template defines
#include <stdio.h>      // includes standard file i/o functionality
#include <string.h>     // includes string functions
#include <cderr.h>      // includes the common dialog error codes
#include <fcntl.h>
#include <io.h>
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
#include "find.h"
#include "update.h"
#include "mission.h"
#include "package.h"
#include "atm.h"
#include "name.h"
#include "falcsess.h"
#include "cmpclass.h"
#include "team.h"
#include "Weather.h"
#include "CampStr.h"
#include "MissEval.h"
#include "Feature.h"
#include "CampMap.h"
#include "PlayerOp.h"
#include "classtbl.h"
#include "falcgame.h"

// WARNING: ResMgr stub
extern void ResAddPath(char* a, int b) {}

extern void Camp_MakeInstantAction(void);
extern int RegroupFlight(Flight flight);

char      CampFile[MAX_PATH];
char TheaterFile[MAX_PATH];
OPENFILENAME CampFileName;
OPENFILENAME TheaterFileName;
CHAR        cmpFile[MAX_PATH]      = "\0";
CHAR        cmpFileTitle[MAX_PATH];
CHAR        thrFile[MAX_PATH]      = "\0";
CHAR        thrFileTitle[MAX_PATH];

// Filter specification for the OPENFILENAME struct
CHAR cmpFilter[] = "Campaign Files (*.CAM)\0*.CAM\0All Files (*.*)\0*.*\0";
CHAR thrFilter[] = "Theater Files (*.THR)\0*.THR\0All Files (*.*)\0*.*\0";
CHAR scuFilter[] = "Scripted Unit Files (*.SCU)\0*.SCU\0All Files (*.*)\0*.*\0";

CHAR        FileBuf[FILE_LEN];
DWORD       dwFileSize;
CHAR *      lpBufPtr = FileBuf;
HWND        hDlgFR = NULL;

extern _TCHAR GroundSTypesStr[20][20];

#ifdef CAMPTOOL

short ObjTypeConverter[35]; // Converts from alphabetical order to real types

extern short NameEntries;
extern _TCHAR *NameStream;

// Need to distinquish between CAS and Interdiction, Alert and DCA, etc.

// ========================================================
// External prototypes
// ========================================================

// ========================================================
// Support functions
// ========================================================

// Returns 1 if x,y is within rectangle
int inButton(RECT *but, WORD xPos, WORD yPos)
{
    if (xPos >= but->left and xPos <= but->right and yPos >= but->top and yPos <= but->bottom)
        return 1;
    else
        return 0;
}

/****************************************************************************
*
*    FUNCTION: About(HWND, UINT, WPARAM, LPARAM)
*
*    PURPOSE:  Processes messages for "About" dialog box
*
*    COMMENTS:
*
*       No initialization is needed for this particular dialog box, but TRUE
*       must be returned to Windows.
*
*       Wait for user to click on "Ok" button, then close the dialog box.
*
****************************************************************************/

BOOL WINAPI About(HWND hDlg, UINT message, /* type of message                 */
                  WPARAM wParam, LPARAM lParam)      /* message-specific information    */

{
    switch (message)
    {
        case WM_INITDIALOG:              /* message: initialize dialog box */
            return (TRUE);

        case WM_COMMAND:                 /* message: received a command */
            if (LOWORD(wParam) == IDOK    /* "OK" box selected?        */
                or LOWORD(wParam) == IDCANCEL)
            {
                /* System menu close command? */
                EndDialog(hDlg, TRUE); /* Exits the dialog box        */
                return (TRUE);
            }

            break;
    }

    return (FALSE);                    /* Didn't process a message    */
    // avoid compiler warnings at W3
    lParam;
}

void UpdateNames(HWND hDlg, Objective O)
{
    Objective o;
    char buffer[80];
    long i;

    o = O->GetObjectiveParent();
    i = O->GetObjectiveNameID();

    if (i < 1)
    {
        ShowWindow(GetDlgItem(hDlg, IDC_OBJ_NAMECOMBO), 0);
        ShowWindow(GetDlgItem(hDlg, IDC_OBJ_NAMEVAL), 1);
        SendMessage(GetDlgItem(hDlg, IDC_OBJ_NAMECOMBO), CB_SETCURSEL, 0, 0);
        SetDlgItemText(hDlg, IDC_OBJ_NAMEVAL, ReadNameString(O->GetObjectiveNameID(), buffer, 79));
        InvalidateRect(GetDlgItem(hDlg, IDC_OBJ_NAMEVAL), NULL, TRUE);
    }
    else
    {
        ShowWindow(GetDlgItem(hDlg, IDC_OBJ_NAMECOMBO), 1);
        ShowWindow(GetDlgItem(hDlg, IDC_OBJ_NAMEVAL), 0);
        i = SendMessage(GetDlgItem(hDlg, IDC_OBJ_NAMECOMBO), CB_FINDSTRINGEXACT, 0, (LPARAM)ReadNameString(O->GetObjectiveNameID(), buffer, 79));
        SendMessage(GetDlgItem(hDlg, IDC_OBJ_NAMECOMBO), CB_SETCURSEL, i, 0);
        InvalidateRect(GetDlgItem(hDlg, IDC_OBJ_NAMECOMBO), NULL, TRUE);
        // EnableWindow(GetDlgItem(hDlg,IDC_OBJ_NAMECOMBO),1);
    }

    if (o)
        SetDlgItemText(hDlg, IDC_OBJ_PARENTVAL, o->GetName(buffer, 80, FALSE));
    else
        SetDlgItemText(hDlg, IDC_OBJ_PARENTVAL, "");

    InvalidateRect(GetDlgItem(hDlg, IDC_OBJ_PARENTVAL), NULL, TRUE);
}

void UpdateSTypeCombo(HWND hDlg, Objective O)
{
    int i, j, type;

    // Fill stype combo box
    type = O->GetType();
    SendMessage(GetDlgItem(hDlg, IDC_OBJ_STYPEVAL), CB_RESETCONTENT, 0, 0);
    SendMessage(GetDlgItem(hDlg, IDC_OBJ_STYPEVAL), CB_ADDSTRING, 0, (LPARAM)"Vanilla");
    i = 1;
    j = -1;

    while (j)
    {
        j = GetClassID(DOMAIN_LAND, CLASS_OBJECTIVE, type, i, 0, 0, 0, 0);

        if (j)
        {
            ObjClassDataType* oc;
            oc = (ObjClassDataType*) Falcon4ClassTable[j].dataPtr;

            if (oc)
            {
                SendMessage(GetDlgItem(hDlg, IDC_OBJ_STYPEVAL), CB_ADDSTRING, 0, (LPARAM)oc->Name);
                i++;
            }
            else
                j = 0;
        }
    }

    SendMessage(GetDlgItem(hDlg, IDC_OBJ_STYPEVAL), CB_SETCURSEL, O->GetSType(), 0);
}

BOOL WINAPI EditObjective(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    Objective o, O = GlobObj;
    RECT rect;
    GridIndex x, y;
    char buffer[256], *sptr;
    static char lastname[80];
    static int editing = 0, unique = 0, renaming = 0, recalc = 0;
    int i, j;

    if ( not O)
    {
        EndDialog(hDlg, TRUE);        /* Exits the dialog box        */
        return (FALSE);
    }

    switch (message)
    {
        case WM_INITDIALOG:              /* message: initialize dialog box */

            // Fill type combo box
            for (i = 1; i <= NumObjectiveTypes; i++)
            {
                j = GetClassID(DOMAIN_LAND, CLASS_OBJECTIVE, i, 0, 0, 0, 0, 0);

                if (j)
                {
                    ObjClassDataType* oc;
                    oc = (ObjClassDataType*) Falcon4ClassTable[j].dataPtr;

                    if (oc)
                        SendMessage(GetDlgItem(hDlg, IDC_OBJ_TYPEVAL), CB_ADDSTRING, 0, (LPARAM)ObjectiveStr[i]);
                }
                else
                    SendMessage(GetDlgItem(hDlg, IDC_OBJ_TYPEVAL), CB_ADDSTRING, 0, (LPARAM)"<Davism>");
            }

            // Build my converter array
            for (i = 0; i < NumObjectiveTypes; i++)
            {
                SendMessage(GetDlgItem(hDlg, IDC_OBJ_TYPEVAL), CB_GETLBTEXT, i, (LPARAM)buffer);

                for (j = 0; j <= NumObjectiveTypes; j++)
                {
                    if (strcmp(ObjectiveStr[j], buffer) == 0)
                        ObjTypeConverter[i] = j;
                }

                if (ObjTypeConverter[i] == O->GetType())
                    SendMessage(GetDlgItem(hDlg, IDC_OBJ_TYPEVAL), CB_SETCURSEL, i, 0);
            }

            // Fill the SType combo box
            UpdateSTypeCombo(hDlg, O);

            // Fill Oldown and occ Combo box
            for (i = 0; i < NUM_COUNS; i++)
            {
                SendMessage(GetDlgItem(hDlg, IDC_OBJ_OWNVAL), CB_ADDSTRING, 0, (LPARAM)Side[i]);
                SendMessage(GetDlgItem(hDlg, IDC_OBJ_OCCVAL), CB_ADDSTRING, 0, (LPARAM)Side[i]);
            }

            SendMessage(GetDlgItem(hDlg, IDC_OBJ_OWNVAL), CB_SETCURSEL, O->GetObjectiveOldown(), 0);
            SendMessage(GetDlgItem(hDlg, IDC_OBJ_OCCVAL), CB_SETCURSEL, O->GetOwner(), 0);
            unique = 0;

            // Load the name stream into memory, so this stuff goes faster.
            if ( not NameStream)
                LoadNameStream();

            // Fill Name Combo box
            SendMessage(GetDlgItem(hDlg, IDC_OBJ_NAMECOMBO), CB_ADDSTRING, 0, (LPARAM)"<None>");

            for (i = 2; i < NameEntries; i++)
            {
                sptr = ReadNameString(i, buffer, 79);

                if (sptr[0] and sptr[0] not_eq '<')
                    SendMessage(GetDlgItem(hDlg, IDC_OBJ_NAMECOMBO), CB_ADDSTRING, 0, (LPARAM)ReadNameString(i, buffer, 79));
            }

            i = O->GetObjectiveNameID();

            if (i > 0)
            {
                i = SendMessage(GetDlgItem(hDlg, IDC_OBJ_NAMECOMBO), CB_FINDSTRINGEXACT, 0, (LPARAM)ReadNameString(i, buffer, 79));
                SendMessage(GetDlgItem(hDlg, IDC_OBJ_NAMECOMBO), CB_SETCURSEL, i, 0);
            }
            else
                SendMessage(GetDlgItem(hDlg, IDC_OBJ_NAMECOMBO), CB_SETCURSEL, 0, 0);

            UpdateNames(hDlg, O);

            // Set up static data
            sprintf(buffer, "%d", O->GetCampID());
            SetWindowText(GetDlgItem(hDlg, IDC_OBJ_IDVAL), buffer);
            O->GetLocation(&x, &y);
            sprintf(buffer, "%d,%d", x, y);
            SetWindowText(GetDlgItem(hDlg, IDC_OBJ_LOCVAL), buffer);

            if (O->IsFrontline())
                sprintf(buffer, "Frontline");
            else if (O->IsSecondline())
                sprintf(buffer, "Secondline");
            else if (O->IsThirdline())
                sprintf(buffer, "Thirdline");
            else
                sprintf(buffer, "Rearward");

            SetWindowText(GetDlgItem(hDlg, IDC_OBJ_FRONTVAL), buffer);

            for (i = 0; i < NUM_TEAMS; i++)
                buffer[i] = '0' + (char)O->GetSpotted(i);

            buffer[NUM_TEAMS] = 0;
            SetWindowText(GetDlgItem(hDlg, IDC_OBJ_SPOTVAL), buffer);

            // Set up check boxes
            if (O->SamSite())
                PostMessage(GetDlgItem(hDlg, IDC_OBJ_SAM), BM_SETCHECK, 1, 0);

            if (O->ArtillerySite())
                PostMessage(GetDlgItem(hDlg, IDC_OBJ_ART), BM_SETCHECK, 1, 0);

            if (O->AmbushCAPSite())
                PostMessage(GetDlgItem(hDlg, IDC_OBJ_AMBUSH), BM_SETCHECK, 1, 0);

            if (O->BorderSite())
                PostMessage(GetDlgItem(hDlg, IDC_OBJ_BORDER), BM_SETCHECK, 1, 0);

            if (O->MountainSite())
                PostMessage(GetDlgItem(hDlg, IDC_OBJ_MOUNTAIN), BM_SETCHECK, 1, 0);

            if (O->FlatSite())
                PostMessage(GetDlgItem(hDlg, IDC_OBJ_FLAT), BM_SETCHECK, 1, 0);

            if (O->ManualSet())
                PostMessage(GetDlgItem(hDlg, IDC_OBJ_MANUAL), BM_SETCHECK, 1, 0);

            if (O->CommandoSite())
                PostMessage(GetDlgItem(hDlg, IDC_OBJ_COMMANDO), BM_SETCHECK, 1, 0);

            if (O->RadarSite())
                PostMessage(GetDlgItem(hDlg, IDC_OBJ_RADAR), BM_SETCHECK, 1, 0);

            // Zero invalid features
            for (i = 0; i < O->GetTotalFeatures(); i++)
            {
                if (O->GetFeatureStatus(i) > 0 and not O->GetFeatureID(i))
                    O->SetFeatureStatus(i, VIS_DESTROYED);
            }

            if (GlobList)
            {
                VuListIterator myit(GlobList);
                o = GetFirstObjective(&myit);

                if (o)
                {
                    GlobObj = o;
                    GlobList->Remove(o);
                    DialogBox(hInst, MAKEINTRESOURCE(IDD_OBJECTIVEDIALOG), hDlg, (DLGPROC)EditObjective);
                    GlobObj = O;
                }
            }

            recalc = 0;
            return (TRUE);
            break;

        case WM_PAINT:
        {
            HDC hDC, DC;
            PAINTSTRUCT ps, nps;
            HWND hCWnd;
            int c = 0;

            InvalidateRect(GetDlgItem(hDlg, IDC_OBJ_NAMEVAL), NULL, TRUE);
            PostMessage(GetDlgItem(hDlg, IDC_OBJ_NAMEVAL), WM_PAINT, 0, 0);
            InvalidateRect(GetDlgItem(hDlg, IDC_OBJ_PARENTVAL), NULL, TRUE);
            PostMessage(GetDlgItem(hDlg, IDC_OBJ_PARENTVAL), WM_PAINT, 0, 0);
            InvalidateRect(GetDlgItem(hDlg, IDC_OBJ_OCCVAL), NULL, TRUE);
            PostMessage(GetDlgItem(hDlg, IDC_OBJ_OCCVAL), WM_PAINT, 0, 0);
            j = O->GetObjectiveNameID();

            if (O->IsSecondary() and not j)
                PostMessage(hDlg, WM_COMMAND, IDC_OBJ_RENAME, 0);

            if (GetUpdateRect(hDlg, &rect, FALSE))
            {
                hDC = BeginPaint(hDlg, &ps);

                // SetWindowText(GetDlgItem(hDlg,IDC_OBJ_OWNVAL),Side[O->GetObjectiveOldown()]);
                // SetWindowText(GetDlgItem(hDlg,IDC_OBJ_OCCVAL),Side[O->GetObjectiveControl()]);
                // SetWindowText(GetDlgItem(hDlg,IDC_OBJ_TYPEVAL),O->GetObjectiveClassName());
                sprintf(buffer, "%d%%", O->GetObjectiveStatus());
                SetWindowText(GetDlgItem(hDlg, IDC_OBJ_STATVAL), buffer);
                sprintf(buffer, "%d", O->GetObjectiveSupply());
                SetWindowText(GetDlgItem(hDlg, IDC_OBJ_SUPPLY), buffer);
                sprintf(buffer, "%d", O->GetObjectiveFuel());
                SetWindowText(GetDlgItem(hDlg, IDC_OBJ_FUEL), buffer);
                ObjFeatureStr(O, buffer);
                SetWindowText(GetDlgItem(hDlg, IDC_OBJ_ROSTVAL), buffer);

                sprintf(buffer, "%d", O->GetObjectivePriority());
                SetWindowText(GetDlgItem(hDlg, IDC_OBJ_PRIVAL), buffer);


                hCWnd = GetDlgItem(hDlg, IDC_OBJ_OBJBOX);
                DC = BeginPaint(hCWnd, &nps);
                DisplayObjective(DC, O, 0, 0, 48);
                _setcolor(DC, SideColor[O->GetOwner()]);
                _rectangle(DC, _GFILLINTERIOR, 10, 60, 20, 70);
                _setcolor(DC, TeamColor[O->GetTeam()]);
                _rectangle(DC, _GFILLINTERIOR, 30, 60, 40, 70);
                EndPaint(hCWnd, &nps);

                EndPaint(hDlg, &ps);
            }
        }

        case WM_COMMAND:                 /* message: received command */
            switch (LOWORD(wParam))
            {
                case IDOK:    /* "OK" box selected. */
                case IDCANCEL:
                    i = O->GetObjectiveNameID();

                    if (i == 1)
                        O->SetObjectiveNameID(0);

                    if (renaming)
                        SendMessage(hDlg, WM_COMMAND, IDC_OBJ_NAMECOMBO bitor (CBN_KILLFOCUS << 16) , 0);

                    renaming = 0;

                    if ( not recalc)
                        GlobObj = NULL;

                    recalc = 0;
                    TeamInfo[O->GetTeam()]->SetActive(1);
                    EndDialog(hDlg, TRUE);     /* Exits the dialog box       */
                    return (TRUE);
                    break;

                case IDC_ADD:
                    O->GetLocation(&x, &y);
                    o = AddObjectiveToCampaign(x, y);

                    if (o)
                    {
                        RebuildObjectiveLists();
                        RebuildParentsList();
                        o->RecalculateParent();
                        GlobObj = o;
                        GlobList->Remove(o);
                        DialogBox(hInst, MAKEINTRESOURCE(IDD_OBJECTIVEDIALOG), hDlg, (DLGPROC)EditObjective);
                        GlobObj = O;
                    }

                    break;

                case IDC_OBJ_RECALC:
                    RebuildObjectiveLists();
                    RebuildParentsList();
                    break;

                case IDC_BUTTONDELETE:
                    O->DisposeObjective();
                    recalc = 0;
                    EndDialog(hDlg, TRUE);     /* Exits the dialog box       */
                    return TRUE;
                    break;

                case IDC_OBJ_REPAIR:
                    for (i = 0; i < O->GetTotalFeatures(); i++)
                    {
                        if (O->GetFeatureID(i))
                            O->SetFeatureStatus(i, VIS_NORMAL);
                        else
                            O->SetFeatureStatus(i, VIS_DESTROYED);
                    }

                    O->ResetObjectiveStatus();
                    InvalidateRect(hDlg, NULL, TRUE);
                    PostMessage(hDlg, WM_PAINT, 0, 0);
                    break;

                case IDC_OBJ_NAMECOMBO:
                    if (HIWORD(wParam) == CBN_SELENDOK)
                    {
                        i = SendMessage(GetDlgItem(hDlg, IDC_OBJ_NAMECOMBO), CB_GETCURSEL, 0, 0);

                        if (i > 1)
                        {
                            SendMessage(GetDlgItem(hDlg, IDC_OBJ_NAMECOMBO), CB_GETLBTEXT, i, (LPARAM)buffer);
                            O->SetObjectiveNameID(FindName(buffer));
                        }
                        else
                            O->SetObjectiveNameID(0);

                        UpdateNames(hDlg, O);
                    }

                    if (HIWORD(wParam) == CBN_EDITCHANGE)
                        renaming = 1;

                    if (HIWORD(wParam) == CBN_KILLFOCUS and renaming)
                    {
                        GetDlgItemText(hDlg, IDC_OBJ_NAMECOMBO, buffer, 79);
                        SendMessage(GetDlgItem(hDlg, IDC_OBJ_NAMECOMBO), CB_ADDSTRING, 0, (LPARAM)buffer);
                        j = SendMessage(GetDlgItem(hDlg, IDC_OBJ_NAMECOMBO), CB_GETCURSEL, 0, 0);
                        i = O->GetObjectiveNameID();

                        if (i > 1)
                        {
                            _TCHAR last[80];
                            j = SendMessage(GetDlgItem(hDlg, IDC_OBJ_NAMECOMBO), CB_FINDSTRINGEXACT, 0, (LPARAM)ReadNameString(i, last, 79));
                            SendMessage(GetDlgItem(hDlg, IDC_OBJ_NAMECOMBO), CB_DELETESTRING, j, 0);
                            SetName(i, buffer);
                        }
                        else if (j < 0)
                            i = AddName(buffer);
                        else
                            i = 0;

                        renaming = 0;
                        O->SetObjectiveNameID(i);
                    }

                    break;

                case IDC_OBJ_NAMEVAL:
                    break;

                case IDC_OBJ_TYPEVAL:
                    if (HIWORD(wParam) == CBN_SELENDOK)
                    {
                        i = SendMessage(GetDlgItem(hDlg, IDC_OBJ_TYPEVAL), CB_GETCURSEL, 0, 0);

                        if (ObjTypeConverter[i] > 0)
                            O->SetObjectiveType((uchar)ObjTypeConverter[i]);

                        j = O->GetObjectiveNameID();

                        if (O->IsSecondary() and not j)
                            PostMessage(hDlg, WM_COMMAND, IDC_OBJ_RENAME, 0);
                        else if ( not O->IsSecondary() and j == 1)
                            O->SetObjectiveNameID(0);

                        UpdateNames(hDlg, O);
                        UpdateSTypeCombo(hDlg, O);
                        PostMessage(hDlg, WM_COMMAND, IDC_OBJ_REPAIR, 0);
                    }

                    break;

                case IDC_OBJ_STYPEVAL:
                    if (HIWORD(wParam) == CBN_SELENDOK)
                    {
                        i = SendMessage(GetDlgItem(hDlg, IDC_OBJ_STYPEVAL), CB_GETCURSEL, 0, 0);
                        O->SetObjectiveSType(i);
                        PostMessage(hDlg, WM_COMMAND, IDC_OBJ_REPAIR, 0);
                    }

                    break;

                case IDC_OBJ_OWNVAL:
                    if (HIWORD(wParam) == CBN_SELENDOK)
                    {
                        i = SendMessage(GetDlgItem(hDlg, IDC_OBJ_OWNVAL), CB_GETCURSEL, 0, 0);

                        if (i >= 0)
                        {
                            O->SetObjectiveOldown(i);
                            O->SetOwner(i);
                            SendMessage(GetDlgItem(hDlg, IDC_OBJ_OCCVAL), CB_SETCURSEL, i, 0);
                        }

                        O->RecalculateParent();
                        UpdateNames(hDlg, O);
                        InvalidateRect(hDlg, NULL, TRUE);
                        PostMessage(hDlg, WM_PAINT, 0, 0);
                    }

                    break;

                case IDC_OBJ_OCCVAL:
                    if (HIWORD(wParam) == CBN_SELENDOK)
                    {
                        i = SendMessage(GetDlgItem(hDlg, IDC_OBJ_OCCVAL), CB_GETCURSEL, 0, 0);

                        if (i >= 0)
                            O->SetOwner(i);

                        O->RecalculateParent();
                        UpdateNames(hDlg, O);
                        InvalidateRect(hDlg, NULL, TRUE);
                        PostMessage(hDlg, WM_PAINT, 0, 0);
                    }

                    break;

                case IDC_OBJ_PRIVAL:
                    GetDlgItemText(hDlg, IDC_OBJ_PRIVAL, buffer, 79);
                    i = atoi(buffer);

                    if (HIWORD(wParam) == EN_KILLFOCUS or i > 0)
                    {
                        if (i > 0)
                            O->SetObjectivePriority(i);

                        O->RecalculateParent();
                        recalc = 1;
                    }

                    break;

                case IDC_OBJ_RENAME:
                    i = O->GetObjectiveNameID();

                    if ( not i)
                        O->SetObjectiveNameID(1);

                    UpdateNames(hDlg, O);
                    break;

                case IDC_OBJ_REMOVE:
                    i = SendMessage(GetDlgItem(hDlg, IDC_OBJ_NAMECOMBO), CB_GETCURSEL, 0, 0);

                    if (i > 0)
                    {
                        SendMessage(GetDlgItem(hDlg, IDC_OBJ_NAMECOMBO), CB_GETLBTEXT, i, (LPARAM)buffer);
                        SendMessage(GetDlgItem(hDlg, IDC_OBJ_NAMECOMBO), CB_DELETESTRING, i, 0);
                        SendMessage(GetDlgItem(hDlg, IDC_OBJ_NAMECOMBO), CB_SETCURSEL, 0, 0);
                        RemoveName(buffer);
                    }

                    O->SetObjectiveNameID(0);
                    renaming = 0;
                    recalc = 1;
                    UpdateNames(hDlg, O);
                    break;

                case IDC_OBJ_DEL: // Inherit name, but don't remove from list
                    O->SetObjectiveNameID(0);
                    UpdateNames(hDlg, O);
                    renaming = 0;
                    recalc = 1;
                    break;

                case IDC_OBJ_SAM:
                    if (O->SamSite())
                        O->SetSamSite(0);
                    else
                        O->SetSamSite(1);

                    break;

                case IDC_OBJ_ART:
                    if (O->ArtillerySite())
                        O->SetArtillerySite(0);
                    else
                        O->SetArtillerySite(1);

                    break;

                case IDC_OBJ_AMBUSH:
                    if (O->AmbushCAPSite())
                        O->SetAmbushCAPSite(0);
                    else
                        O->SetAmbushCAPSite(1);

                    break;

                case IDC_OBJ_BORDER:
                    if (O->BorderSite())
                        O->SetBorderSite(0);
                    else
                        O->SetBorderSite(1);

                    break;

                case IDC_OBJ_MOUNTAIN:
                    if (O->MountainSite())
                        O->SetMountainSite(0);
                    else
                        O->SetMountainSite(1);

                    break;

                case IDC_OBJ_FLAT:
                    if (O->FlatSite())
                        O->SetFlatSite(0);
                    else
                        O->SetFlatSite(1);

                    break;

                case IDC_OBJ_RADAR:
                    if (O->RadarSite())
                        O->SetRadarSite(0);
                    else
                        O->SetRadarSite(1);

                    break;

                case IDC_OBJ_MANUAL:
                    if (O->ManualSet())
                        O->SetManual(0);
                    else
                        O->SetManual(1);

                case IDC_OBJ_COMMANDO:
                    if (O->CommandoSite())
                        O->SetCommandoSite(0);
                    else
                        O->SetCommandoSite(1);

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

BOOL WINAPI MissionTriggerProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    RECT rect;
    static int mission, tottype, toteam, vsteam, targettype;
    static CampEntity target;
    char buffer[40];
    int i;

    switch (message)
    {
        case WM_INITDIALOG:              /* message: initialize dialog box */
            vsteam = targettype = 0;
            target = NULL;
            sprintf(buffer, "%d", CurX);
            SetWindowText(GetDlgItem(hDlg, IDC_TARGETX), buffer);
            sprintf(buffer, "%d", CurY);
            SetWindowText(GetDlgItem(hDlg, IDC_TARGETY), buffer);
            sprintf(buffer, "%d", 0);
            SetWindowText(GetDlgItem(hDlg, IDC_TARGETID), buffer);
            sprintf(buffer, "%d", Camp_GetCurrentTime() + 75 * CampaignMinutes);
            SetWindowText(GetDlgItem(hDlg, IDC_TOT), buffer);
            sprintf(buffer, "%d", 5);
            SetWindowText(GetDlgItem(hDlg, IDC_PRIORITY), buffer);
            sprintf(buffer, "%d", 0);
            SetWindowText(GetDlgItem(hDlg, IDC_FLAGS), buffer);
            SendMessage(GetDlgItem(hDlg, IDC_MT_MISSIONCOMBO), WM_SETREDRAW, 0, 0);

            for (i = 0; i < AMIS_OTHER; i++)
                SendMessage(GetDlgItem(hDlg, IDC_MT_MISSIONCOMBO), CB_ADDSTRING, 0, (LPARAM)MissStr[i]);

            SendMessage(GetDlgItem(hDlg, IDC_MT_MISSIONCOMBO), CB_SETCURSEL, 0, 0);
            SendMessage(GetDlgItem(hDlg, IDC_MT_MISSIONCOMBO), WM_SETREDRAW, 1, 0);

            SendMessage(GetDlgItem(hDlg, IDC_MT_TARGETTYPECOMBO), WM_SETREDRAW, 0, 0);

            for (i = 0; i < 3; i++)
                SendMessage(GetDlgItem(hDlg, IDC_MT_TARGETTYPECOMBO), CB_ADDSTRING, 0, (LPARAM)TargetTypeStr[i]);

            SendMessage(GetDlgItem(hDlg, IDC_MT_TARGETTYPECOMBO), CB_SETCURSEL, 0, 0);
            SendMessage(GetDlgItem(hDlg, IDC_MT_TARGETTYPECOMBO), WM_SETREDRAW, 1, 0);

            for (i = 0; i < 7; i++)
                SendMessage(GetDlgItem(hDlg, IDC_MT_TYPECOMBO), CB_ADDSTRING, 0, (LPARAM)TOTStr[i]);

            SendMessage(GetDlgItem(hDlg, IDC_MT_TYPECOMBO), CB_SETCURSEL, 0, 0);

            for (i = 0; i < NUM_TEAMS; i++)
            {
                sprintf(buffer, "TEAM %d", i);
                SendMessage(GetDlgItem(hDlg, IDC_MT_TEAMCOMBO), CB_ADDSTRING, 0, (LPARAM)buffer);
            }

            SendMessage(GetDlgItem(hDlg, IDC_MT_TEAMCOMBO), CB_SETCURSEL, 0, 0);
            mission = tottype = toteam = 0;
            break;

        case WM_PAINT:
        {
            HDC hDC;
            PAINTSTRUCT ps;

            if (GetUpdateRect(hDlg, &rect, FALSE))
            {
                hDC = BeginPaint(hDlg, &ps);
                GetWindowText(GetDlgItem(hDlg, IDC_MISSIONTYPE), buffer, 10);
                mission = atoi(buffer);
                SetWindowText(GetDlgItem(hDlg, IDC_MISSIONTYPE), MissStr[mission]);
                EndPaint(hDlg, &ps);
            }
        }
        break;

        case WM_COMMAND:                 /* message: received a command */
            switch (LOWORD(wParam))
            {
                case IDOK:    /* "OK" box selected?  */
                {
                    MissionRequestClass mis;

                    // Trigger Mission
                    // mis.requesterID;
                    // mis.targetID;
                    // mis.secondaryID;
                    GetWindowText(GetDlgItem(hDlg, IDC_TOT), buffer, 39);
                    mis.tot = atoi(buffer);
                    GetWindowText(GetDlgItem(hDlg, IDC_TARGETX), buffer, 39);
                    mis.tx = atoi(buffer);
                    GetWindowText(GetDlgItem(hDlg, IDC_TARGETY), buffer, 39);
                    mis.ty = atoi(buffer);
                    GetWindowText(GetDlgItem(hDlg, IDC_FLAGS), buffer, 39);
                    mis.flags = atoi(buffer);
                    GetWindowText(GetDlgItem(hDlg, IDC_PRIORITY), buffer, 39);
                    mis.priority = atoi(buffer);
                    mis.who = toteam;
                    mis.vs = vsteam;
                    mis.tot_type = tottype;
                    mis.mission = mission;

                    if (target)
                        mis.targetID = target->Id();
                    else
                        mis.targetID = FalconNullId;

                    mis.roe_check = ROE_AIR_ENGAGE;
                    mis.RequestMission();
                    EndDialog(hDlg, TRUE);
                }
                break;

                case IDCANCEL:
                    EndDialog(hDlg, TRUE);     /* Exits the dialog box        */
                    return (TRUE);
                    break;

                case IDC_MT_MISSIONCOMBO:
                    if (HIWORD(wParam) == CBN_SELENDOK)
                        mission = SendMessage(GetDlgItem(hDlg, IDC_MT_MISSIONCOMBO), CB_GETCURSEL, 0, 0);

                    break;

                case IDC_MT_TYPECOMBO:
                    if (HIWORD(wParam) == CBN_SELENDOK)
                        tottype = SendMessage(GetDlgItem(hDlg, IDC_MT_TYPECOMBO), CB_GETCURSEL, 0, 0);

                    break;

                case IDC_MT_TARGETTYPECOMBO:
                    if (HIWORD(wParam) == CBN_SELENDOK)
                    {
                        targettype = SendMessage(GetDlgItem(hDlg, IDC_MT_TARGETTYPECOMBO), CB_GETCURSEL, 0, 0);

                        if (targettype == 1)
                        {
                            Objective o;
                            targettype = CLASS_OBJECTIVE;
                            o = GetObjectiveByXY(CurX, CurY);

                            if (o)
                            {
                                target = (CampEntity)o;
                                vsteam = o->GetTeam();
                                sprintf(buffer, "%d", o->GetCampID());
                            }
                            else
                                sprintf(buffer, "0");

                            SetWindowText(GetDlgItem(hDlg, IDC_TARGETID), buffer);
                        }
                        else if (targettype == 2)
                        {
                            Unit u;
                            targettype = CLASS_UNIT;
                            u = FindUnitByXY(AllRealList, CurX, CurY, 0);

                            if (u)
                            {
                                target = (CampEntity)u;
                                vsteam = u->GetTeam();
                                sprintf(buffer, "%d", u->GetCampID());
                            }
                            else
                                sprintf(buffer, "0");

                            SetWindowText(GetDlgItem(hDlg, IDC_TARGETID), buffer);
                        }
                    }

                    break;

                case IDC_MT_TEAMCOMBO:
                    if (HIWORD(wParam) == CBN_SELENDOK)
                    {
                        toteam = SendMessage(GetDlgItem(hDlg, IDC_MT_TEAMCOMBO), CB_GETCURSEL, 0, 0);

                        if ( not vsteam)
                            vsteam = GetEnemyTeam(toteam);
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

BOOL WINAPI WeatherEditProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    RECT rect;
    static int head;
    char buffer[40];
    int i;

    switch (message)
    {
        case WM_INITDIALOG:              /* message: initialize dialog box */
            sprintf(buffer, "%d", (int)(((WeatherClass*)realWeather)->temperature));
            SetWindowText(GetDlgItem(hDlg, IDC_WD_TEMPVAL), buffer);
            head = (int)(((WeatherClass*)realWeather)->windHeading * 180 / PI);
            head = head % 360;
            sprintf(buffer, "%d", head);
            SetWindowText(GetDlgItem(hDlg, IDC_WD_WHVAL), buffer);
            sprintf(buffer, "%5.1f", ((WeatherClass*)realWeather)->windSpeed);
            SetWindowText(GetDlgItem(hDlg, IDC_WD_WSVAL), buffer);
            sprintf(buffer, "%d", ((WeatherClass*)realWeather)->GetCloudCover(CurX, CurY));
            SetWindowText(GetDlgItem(hDlg, IDC_WD_COVEREDIT), buffer);
            sprintf(buffer, "%d", ((WeatherClass*)realWeather)->GetCloudLevel(CurX, CurY));
            SetWindowText(GetDlgItem(hDlg, IDC_WD_LEVELEDIT), buffer);
            sprintf(buffer, "%d", ((WeatherClass*)realWeather)->temperature);
            SetWindowText(GetDlgItem(hDlg, IDC_WD_FORCASTTEMP), buffer);
            sprintf(buffer, "%d", ((WeatherClass*)realWeather)->windSpeed);
            SetWindowText(GetDlgItem(hDlg, IDC_WD_FORCASTSPEED), buffer);
            sprintf(buffer, "%d", ((WeatherClass*)realWeather)->stratusBase);
            SetWindowText(GetDlgItem(hDlg, IDC_WD_FORCASTLEVEL), buffer);
            break;

        case WM_PAINT:
        {
            HDC hDC;
            PAINTSTRUCT ps;

            if (GetUpdateRect(hDlg, &rect, FALSE))
            {
                hDC = BeginPaint(hDlg, &ps);
                EndPaint(hDlg, &ps);
            }
        }
        break;

        case WM_COMMAND:                 /* message: received a command */
            switch (LOWORD(wParam))
            {
                case IDOK:    /* "OK" box selected?  */
                    EndDialog(hDlg, TRUE);
                    break;

                case IDCANCEL:
                    EndDialog(hDlg, TRUE);     /* Exits the dialog box        */
                    return (TRUE);
                    break;

                case IDC_WD_COVEREDIT:
                    GetDlgItemText(hDlg, IDC_WD_COVEREDIT, buffer, 79);
                    i = atoi(buffer);

                    if (HIWORD(wParam) == EN_KILLFOCUS)
                        ((WeatherClass*)realWeather)->SetCloudCover(CurX, CurY, i);

                    break;

                case IDC_WD_LEVELEDIT:
                    GetDlgItemText(hDlg, IDC_WD_LEVELEDIT, buffer, 79);
                    i = atoi(buffer);

                    if (HIWORD(wParam) == EN_KILLFOCUS)
                        ((WeatherClass*)realWeather)->SetCloudLevel(CurX, CurY, i);

                    break;

                case IDC_WD_FORCASTTEMP:
                    GetDlgItemText(hDlg, IDC_WD_FORCASTTEMP, buffer, 79);
                    i = atoi(buffer);

                    if (HIWORD(wParam) == EN_KILLFOCUS)
                        ((WeatherClass*)realWeather)->temperature = (float) i;

                    break;

                case IDC_WD_FORCASTSPEED:
                    GetDlgItemText(hDlg, IDC_WD_FORCASTSPEED, buffer, 79);
                    i = atoi(buffer);

                    if (HIWORD(wParam) == EN_KILLFOCUS)
                        ((WeatherClass*)realWeather)->windSpeed = (float) i;

                    break;

                case IDC_WD_FORCASTLEVEL:
                    GetDlgItemText(hDlg, IDC_WD_FORCASTLEVEL, buffer, 79);
                    i = atoi(buffer);

                    if (HIWORD(wParam) == EN_KILLFOCUS)
                        ((WeatherClass*)realWeather)->stratusBase = i;

                    break;

                case IDC_WD_WSVAL:
                    GetDlgItemText(hDlg, IDC_WD_WSVAL, buffer, 79);
                    i = atoi(buffer);

                    if (HIWORD(wParam) == EN_KILLFOCUS)
                        ((WeatherClass*)realWeather)->windSpeed = (float) i;

                    break;

                case IDC_WD_WHVAL:
                    GetDlgItemText(hDlg, IDC_WD_WHVAL, buffer, 79);
                    i = atoi(buffer);

                    if (HIWORD(wParam) == EN_KILLFOCUS)
                    {
                        head = i;
                        ((WeatherClass*)realWeather)->windHeading = (float)(i * PI / 180.0F);
                    }

                    break;

                case IDC_WD_TEMPVAL:
                    GetDlgItemText(hDlg, IDC_WD_TEMPVAL, buffer, 79);
                    i = atoi(buffer);

                    if (HIWORD(wParam) == EN_KILLFOCUS)
                        ((WeatherClass*)realWeather)->temperature = (float) i;

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

BOOL WINAPI EditRelations(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    char buffer[40];
    int i, j, r;

    switch (message)
    {
        case WM_INITDIALOG:              /* message: initialize dialog box */
        {
            HWND cb;

            SetWindowText(GetDlgItem(hDlg, IDC_COUNTRYLABEL1), Side[1]);
            SetWindowText(GetDlgItem(hDlg, IDC_COUNTRYLABEL2), Side[2]);
            SetWindowText(GetDlgItem(hDlg, IDC_COUNTRYLABEL3), Side[3]);
            SetWindowText(GetDlgItem(hDlg, IDC_COUNTRYLABEL4), Side[4]);
            SetWindowText(GetDlgItem(hDlg, IDC_COUNTRYLABEL5), Side[5]);
            SetWindowText(GetDlgItem(hDlg, IDC_COUNTRYLABEL6), Side[6]);

            for (i = 1; i < 7; i++)
            {
                cb = GetDlgItem(hDlg, IDC_COUNTRY1 + i - 1);

                for (j = 0; j < NUM_TEAMS; j++)
                {
                    sprintf(buffer, "TEAM %d", j);
                    SendMessage(cb, CB_ADDSTRING, 0, (LPARAM)buffer);
                }

                if (i < NUM_COUNS)
                    SendMessage(cb, CB_SETCURSEL, GetTeam(i), 0);
                else
                    SendMessage(cb, CB_SETCURSEL, 0, 0);
            }

            for (i = 1; i < NUM_TEAMS; i++)
            {
                for (j = 1; j < NUM_TEAMS; j++)
                {
                    cb = GetDlgItem(hDlg, IDC_TEAM0 + ((i - 1) * 6) + (j - 1));
                    SendMessage(cb, CB_ADDSTRING, 0, (LPARAM)"None");
                    SendMessage(cb, CB_ADDSTRING, 0, (LPARAM)"Allied");
                    SendMessage(cb, CB_ADDSTRING, 0, (LPARAM)"Friendly");
                    SendMessage(cb, CB_ADDSTRING, 0, (LPARAM)"Neutral");
                    SendMessage(cb, CB_ADDSTRING, 0, (LPARAM)"Hostile");
                    SendMessage(cb, CB_ADDSTRING, 0, (LPARAM)"War");
                    SendMessage(cb, CB_SETCURSEL, GetTTRelations(i, j), 0);
                }

                if (TeamInfo[i]->HasSatelites())
                    PostMessage(GetDlgItem(hDlg, (IDC_TEAM_SAT1 - 1 + i)), BM_SETCHECK, 1, 0);

                if (TeamInfo[i]->flags bitand TEAM_ACTIVE)
                    PostMessage(GetDlgItem(hDlg, (IDC_TEAM_ACT1 - 1 + i)), BM_SETCHECK, 1, 0);
            }

            for (i = 1; i < NUM_TEAMS; i++)
            {
                sprintf(buffer, "%d", TeamInfo[i]->GetInitiative());
                SetWindowText(GetDlgItem(hDlg, IDC_TEAM_INIT1 - 1 + i), buffer);
            }
        }
        break;

        case WM_PAINT:
        {
            HDC hDC;
            PAINTSTRUCT ps;
            RECT rect;

            if (GetUpdateRect(hDlg, &rect, FALSE))
            {
                hDC = BeginPaint(hDlg, &ps);
                EndPaint(hDlg, &ps);
            }
        }
        break;

        case WM_COMMAND:                 /* message: received a command */
            i = LOWORD(wParam);

            switch (i)
            {
                case IDOK:    /* "OK" box selected?  */
                case IDCANCEL:
                    EndDialog(hDlg, TRUE);     /* Exits the dialog box        */
                    return (TRUE);
                    break;

                case IDC_COUNTRY1:
                case IDC_COUNTRY2:
                case IDC_COUNTRY3:
                case IDC_COUNTRY4:
                case IDC_COUNTRY5:
                case IDC_COUNTRY6:
                    if (HIWORD(wParam) == CBN_SELENDOK)
                    {
                        j = SendMessage(GetDlgItem(hDlg, i), CB_GETCURSEL, 0, 0);

                        if (j >= 0)
                            SetTeam(i - IDC_COUNTRY1 + 1, j);
                    }

                    break;

                case IDC_TEAM_SAT1:
                case IDC_TEAM_SAT2:
                case IDC_TEAM_SAT3:
                case IDC_TEAM_SAT4:
                case IDC_TEAM_SAT5:
                case IDC_TEAM_SAT6:
                    j = i - IDC_TEAM_SAT1 + 1;

                    if (TeamInfo[j]->HasSatelites())
                        TeamInfo[j]->flags xor_eq TEAM_HASSATS;
                    else
                        TeamInfo[j]->flags or_eq TEAM_HASSATS;

                    break;

                case IDC_TEAM_ACT1:
                case IDC_TEAM_ACT2:
                case IDC_TEAM_ACT3:
                case IDC_TEAM_ACT4:
                case IDC_TEAM_ACT5:
                case IDC_TEAM_ACT6:
                    j = i - IDC_TEAM_ACT1 + 1;

                    if (TeamInfo[j]->flags bitand TEAM_ACTIVE)
                        TeamInfo[j]->SetActive(0);
                    else
                        TeamInfo[j]->SetActive(1);

                    break;

                case IDC_TEAM_INIT1:
                case IDC_TEAM_INIT2:
                case IDC_TEAM_INIT3:
                case IDC_TEAM_INIT4:
                case IDC_TEAM_INIT5:
                case IDC_TEAM_INIT6:
                    j = i - IDC_TEAM_INIT1 + 1;
                    GetDlgItemText(hDlg, i, buffer, 39);
                    i = atoi(buffer);

                    if (HIWORD(wParam) == EN_KILLFOCUS)
                        TeamInfo[j]->SetInitiative(i);

                    break;

                case IDC_TEAM0:
                case IDC_TEAM1:
                case IDC_TEAM2:
                case IDC_TEAM3:
                case IDC_TEAM4:
                case IDC_TEAM5:
                case IDC_TEAM6:
                case IDC_TEAM7:
                case IDC_TEAM8:
                case IDC_TEAM9:
                case IDC_TEAM10:
                case IDC_TEAM11:
                case IDC_TEAM12:
                case IDC_TEAM13:
                case IDC_TEAM14:
                case IDC_TEAM15:
                case IDC_TEAM16:
                case IDC_TEAM17:
                case IDC_TEAM18:
                case IDC_TEAM19:
                case IDC_TEAM20:
                case IDC_TEAM21:
                case IDC_TEAM22:
                case IDC_TEAM23:
                case IDC_TEAM24:
                case IDC_TEAM25:
                case IDC_TEAM26:
                case IDC_TEAM27:
                case IDC_TEAM28:
                case IDC_TEAM29:
                case IDC_TEAM30:
                case IDC_TEAM31:
                case IDC_TEAM32:
                case IDC_TEAM33:
                case IDC_TEAM34:
                case IDC_TEAM35:
                    if (HIWORD(wParam) == CBN_SELENDOK)
                    {
                        r = SendMessage(GetDlgItem(hDlg, i), CB_GETCURSEL, 0, 0);

                        if (r >= 0)
                        {
                            j = (i - IDC_TEAM0) % 6 + 1;
                            i = (i - IDC_TEAM0) / 6 + 1;
                            SetTTRelations(i, j, r);
                        }
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

BOOL WINAPI EditTeams(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    char buffer[256], temp[40];
    int i, j;
    static int so, su, sm;
    static TeamClass *tempTeam;

    switch (message)
    {
        case WM_INITDIALOG:              /* message: initialize dialog box */
        {
            HWND cb;

            // Team combo box
            for (i = 0; i < NUM_TEAMS; i++)
                SendMessage(GetDlgItem(hDlg, IDC_TEAM_COMBO), CB_ADDSTRING, 0, (LPARAM)TeamInfo[i]->GetName());

            SendMessage(GetDlgItem(hDlg, IDC_TEAM_COMBO), CB_SETCURSEL, 0, 0);

            // Relations combo
            for (i = 0; i < NUM_TEAMS; i++)
            {
                cb = GetDlgItem(hDlg, IDC_TEAM_REL_0 + i);
                SendMessage(cb, CB_RESETCONTENT, 0, 0);
                SendMessage(cb, CB_ADDSTRING, 0, (LPARAM)"None");
                SendMessage(cb, CB_ADDSTRING, 0, (LPARAM)"Allied");
                SendMessage(cb, CB_ADDSTRING, 0, (LPARAM)"Friendly");
                SendMessage(cb, CB_ADDSTRING, 0, (LPARAM)"Neutral");
                SendMessage(cb, CB_ADDSTRING, 0, (LPARAM)"Hostile");
                SendMessage(cb, CB_ADDSTRING, 0, (LPARAM)"War");
            }

            // Set selected priorities
            so = su = sm = 0;

            // Set up the copy
            tempTeam = new TeamClass(GetClassID(DOMAIN_ABSTRACT, CLASS_MANAGER, TYPE_TEAM, 0, 0, 0, 0, 0) + VU_LAST_ENTITY_TYPE, 0);
            memcpy(tempTeam, TeamInfo[0], sizeof(TeamClass));

        }
        break;

        case WM_PAINT:
        {
            HDC hDC;
            PAINTSTRUCT ps;
            RECT rect;

            if (GetUpdateRect(hDlg, &rect, FALSE))
            {
                hDC = BeginPaint(hDlg, &ps);

                SetWindowText(GetDlgItem(hDlg, IDC_TEAM_NAME), tempTeam->GetName());
                sprintf(buffer, "%d", tempTeam->GetInitiative());
                SetWindowText(GetDlgItem(hDlg, IDC_INITIATIVE), buffer);
                sprintf(buffer, "%d", tempTeam->airExperience);
                SetWindowText(GetDlgItem(hDlg, IDC_EXPERIENCE1), buffer);
                sprintf(buffer, "%d", tempTeam->airDefenseExperience);
                SetWindowText(GetDlgItem(hDlg, IDC_EXPERIENCE2), buffer);
                sprintf(buffer, "%d", tempTeam->groundExperience);
                SetWindowText(GetDlgItem(hDlg, IDC_EXPERIENCE3), buffer);
                sprintf(buffer, "%d", tempTeam->navalExperience);
                SetWindowText(GetDlgItem(hDlg, IDC_EXPERIENCE4), buffer);
                sprintf(buffer, "%d", tempTeam->GetReinforcement());
                SetWindowText(GetDlgItem(hDlg, IDC_REINFORCEMENTS), buffer);
                // sprintf(buffer,"%d",tempTeam->attackTime);
                // SetWindowText(GetDlgItem(hDlg,IDC_ATTACKTIME),buffer);
                // sprintf(buffer,"%d",tempTeam->offensiveLoss);
                // SetWindowText(GetDlgItem(hDlg,IDC_OFFENSELOSS),buffer);
                sprintf(buffer, "%d", tempTeam->teamFlag);
                SetWindowText(GetDlgItem(hDlg, IDC_FLAGID), buffer);
                sprintf(buffer, "%d", tempTeam->equipment);
                SetWindowText(GetDlgItem(hDlg, IDC_EQUIPMENT), buffer);
                sprintf(buffer, "%d", tempTeam->GetSupplyAvail());
                SetWindowText(GetDlgItem(hDlg, IDC_SUPPLY), buffer);
                sprintf(buffer, "%d", tempTeam->GetFuelAvail());
                SetWindowText(GetDlgItem(hDlg, IDC_FUEL), buffer);
                sprintf(buffer, "%d", tempTeam->startStats.supplyLevel);
                SetWindowText(GetDlgItem(hDlg, IDC_SUPPLY_LEVEL), buffer);
                sprintf(buffer, "%d", tempTeam->startStats.fuelLevel);
                SetWindowText(GetDlgItem(hDlg, IDC_FUEL_LEVEL), buffer);
                sprintf(buffer, "%d", tempTeam->firstColonel);
                SetWindowText(GetDlgItem(hDlg, IDC_PILOT0), buffer);
                sprintf(buffer, "%d", tempTeam->firstCommander);
                SetWindowText(GetDlgItem(hDlg, IDC_PILOT1), buffer);
                sprintf(buffer, "%d", tempTeam->firstWingman);
                SetWindowText(GetDlgItem(hDlg, IDC_PILOT2), buffer);
                sprintf(buffer, "%d", tempTeam->lastWingman);
                SetWindowText(GetDlgItem(hDlg, IDC_PILOT3), buffer);

                for (i = 0; i < NUM_COUNS; i++)
                {
                    if (tempTeam->member[i])
                        PostMessage(GetDlgItem(hDlg, (IDC_COUNTRY_MEM_0 + i)), BM_SETCHECK, 1, 0);
                    else
                        PostMessage(GetDlgItem(hDlg, (IDC_COUNTRY_MEM_0 + i)), BM_SETCHECK, BST_UNCHECKED, 0);
                }

                for (i = 0; i < NUM_TEAMS; i++)
                    SendMessage(GetDlgItem(hDlg, IDC_TEAM_REL_0 + i), CB_SETCURSEL, tempTeam->stance[i], 0);

                if (tempTeam->HasSatelites())
                    PostMessage(GetDlgItem(hDlg, (IDC_SATS)), BM_SETCHECK, 1, 0);
                else
                    PostMessage(GetDlgItem(hDlg, (IDC_SATS)), BM_SETCHECK, BST_UNCHECKED, 0);

                if (tempTeam->flags bitand TEAM_ACTIVE)
                    PostMessage(GetDlgItem(hDlg, (IDC_ACTIVE)), BM_SETCHECK, 1, 0);
                else
                    PostMessage(GetDlgItem(hDlg, (IDC_ACTIVE)), BM_SETCHECK, BST_UNCHECKED, 0);

                buffer[0] = 0;

                for (i = 0; i < MAX_TGTTYPE; i++)
                {
                    sprintf(temp, "%d  ", tempTeam->GetObjTypePriority(i));
                    strcat(buffer, temp);
                }

                SetWindowText(GetDlgItem(hDlg, IDC_OBJPRI), buffer);

                // Obj priority combo box
                SendMessage(GetDlgItem(hDlg, IDC_OBJPRICOMBO), CB_RESETCONTENT, 0, 0);

                for (i = 0; i < MAX_TGTTYPE; i++)
                {
                    sprintf(buffer, "%s:   %d", ObjectiveStr[i], tempTeam->GetObjTypePriority(i));
                    SendMessage(GetDlgItem(hDlg, IDC_OBJPRICOMBO), CB_ADDSTRING, 0, (LPARAM)buffer);
                }

                SendMessage(GetDlgItem(hDlg, IDC_OBJPRICOMBO), CB_SETCURSEL, so, 0);
                sprintf(buffer, "%d", tempTeam->GetObjTypePriority(so));
                SetWindowText(GetDlgItem(hDlg, IDC_OBJPRIEDIT), buffer);

                buffer[0] = 0;

                for (i = 0; i < MAX_UNITTYPE; i++)
                {
                    sprintf(temp, "%d  ", tempTeam->GetUnitTypePriority(i));
                    strcat(buffer, temp);
                }

                SetWindowText(GetDlgItem(hDlg, IDC_UNITPRI), buffer);

                // Obj priority combo box
                SendMessage(GetDlgItem(hDlg, IDC_UNITPRICOMBO), CB_RESETCONTENT, 0, 0);

                for (i = 0; i < MAX_UNITTYPE; i++)
                {
                    sprintf(buffer, "%s:   %d", GroundSTypesStr[i], tempTeam->GetUnitTypePriority(i));
                    SendMessage(GetDlgItem(hDlg, IDC_UNITPRICOMBO), CB_ADDSTRING, 0, (LPARAM)buffer);
                }

                SendMessage(GetDlgItem(hDlg, IDC_UNITPRICOMBO), CB_SETCURSEL, su, 0);
                sprintf(buffer, "%d", tempTeam->GetUnitTypePriority(su));
                SetWindowText(GetDlgItem(hDlg, IDC_UNITPRIEDIT), buffer);

                buffer[0] = 0;

                for (i = 0; i < AMIS_OTHER; i++)
                {
                    sprintf(temp, "%d  ", tempTeam->GetMissionPriority(i));
                    strcat(buffer, temp);
                }

                SetWindowText(GetDlgItem(hDlg, IDC_MISSPRI), buffer);

                // Obj priority combo box
                SendMessage(GetDlgItem(hDlg, IDC_MISSPRICOMBO), CB_RESETCONTENT, 0, 0);

                for (i = 0; i < AMIS_OTHER; i++)
                {
                    sprintf(buffer, "%s:   %d", MissStr[i], tempTeam->GetMissionPriority(i));
                    SendMessage(GetDlgItem(hDlg, IDC_MISSPRICOMBO), CB_ADDSTRING, 0, (LPARAM)buffer);
                }

                SendMessage(GetDlgItem(hDlg, IDC_MISSPRICOMBO), CB_SETCURSEL, sm, 0);
                sprintf(buffer, "%d", tempTeam->GetMissionPriority(sm));
                SetWindowText(GetDlgItem(hDlg, IDC_MISSPRIEDIT), buffer);

                EndPaint(hDlg, &ps);
            }
        }
        break;

        case WM_COMMAND:                 /* message: received a command */
            i = LOWORD(wParam);

            switch (i)
            {
                case IDOK:    /* "OK" box selected? */
                    memcpy(TeamInfo[tempTeam->who], tempTeam, sizeof(TeamClass));
                    EndDialog(hDlg, TRUE); /* Exits the dialog box */
                    break;

                case IDCANCEL:
                    EndDialog(hDlg, TRUE); /* Exits the dialog box */
                    return (TRUE);
                    break;

                case IDC_SAVE_TEAMS:
                    SaveTeams(TheCampaign.SaveFile);
                    break;

                case IDC_TEAM_COMBO:
                    if (HIWORD(wParam) == CBN_SELENDOK)
                    {
                        j = SendMessage(GetDlgItem(hDlg, i), CB_GETCURSEL, 0, 0);

                        if (j not_eq tempTeam->who)
                        {
                            memcpy(TeamInfo[tempTeam->who], tempTeam, sizeof(TeamClass));
                            memcpy(tempTeam, TeamInfo[j], sizeof(TeamClass));
                        }
                    }

                    InvalidateRect(hDlg, NULL, FALSE);
                    PostMessage(GetDlgItem(hDlg, IDD_TEAMEDIT_DIALOG), WM_PAINT, 0, 0);
                    break;

                case IDC_TEAM_NAME:
                    GetDlgItemText(hDlg, i, buffer, 39);

                    if (HIWORD(wParam) == EN_KILLFOCUS)
                        tempTeam->SetName(buffer);

                    break;

                case IDC_INITIATIVE:
                    GetDlgItemText(hDlg, i, buffer, 39);
                    j = atoi(buffer);

                    if (HIWORD(wParam) == EN_KILLFOCUS)
                        tempTeam->SetInitiative(j);

                    break;

                case IDC_EXPERIENCE1:
                    GetDlgItemText(hDlg, i, buffer, 39);
                    j = atoi(buffer);

                    if (HIWORD(wParam) == EN_KILLFOCUS)
                        tempTeam->airExperience = j;

                    break;

                case IDC_EXPERIENCE2:
                    GetDlgItemText(hDlg, i, buffer, 39);
                    j = atoi(buffer);

                    if (HIWORD(wParam) == EN_KILLFOCUS)
                        tempTeam->airDefenseExperience = j;

                    break;

                case IDC_EXPERIENCE3:
                    GetDlgItemText(hDlg, i, buffer, 39);
                    j = atoi(buffer);

                    if (HIWORD(wParam) == EN_KILLFOCUS)
                        tempTeam->groundExperience = j;

                    break;

                case IDC_EXPERIENCE4:
                    GetDlgItemText(hDlg, i, buffer, 39);
                    j = atoi(buffer);

                    if (HIWORD(wParam) == EN_KILLFOCUS)
                        tempTeam->navalExperience = j;

                    break;

                case IDC_REINFORCEMENTS:
                    GetDlgItemText(hDlg, i, buffer, 39);
                    j = atoi(buffer);

                    if (HIWORD(wParam) == EN_KILLFOCUS)
                        tempTeam->SetReinforcement(j);

                    break;

                case IDC_ATTACKTIME:
                    GetDlgItemText(hDlg, i, buffer, 39);
                    j = atoi(buffer);
                    // if (HIWORD(wParam) == EN_KILLFOCUS)
                    // tempTeam->attackTime = j;
                    break;

                case IDC_OFFENSELOSS:
                    GetDlgItemText(hDlg, i, buffer, 39);
                    j = atoi(buffer);
                    // if (HIWORD(wParam) == EN_KILLFOCUS)
                    // tempTeam->offensiveLoss = j;
                    break;

                case IDC_FLAGID:
                    GetDlgItemText(hDlg, i, buffer, 39);
                    j = atoi(buffer);

                    if (HIWORD(wParam) == EN_KILLFOCUS)
                        tempTeam->teamFlag = j;

                    break;

                case IDC_EQUIPMENT:
                    GetDlgItemText(hDlg, i, buffer, 39);
                    j = atoi(buffer);

                    if (HIWORD(wParam) == EN_KILLFOCUS)
                        tempTeam->equipment = j;

                    break;

                case IDC_SUPPLY:
                    GetDlgItemText(hDlg, i, buffer, 39);
                    j = atoi(buffer);

                    if (HIWORD(wParam) == EN_KILLFOCUS)
                        tempTeam->SetSupplyAvail(j);

                    break;

                case IDC_FUEL:
                    GetDlgItemText(hDlg, i, buffer, 39);
                    j = atoi(buffer);

                    if (HIWORD(wParam) == EN_KILLFOCUS)
                        tempTeam->SetFuelAvail(j);

                    break;

                case IDC_SUPPLY_LEVEL:
                    GetDlgItemText(hDlg, i, buffer, 39);
                    j = atoi(buffer);

                    if (HIWORD(wParam) == EN_KILLFOCUS)
                        tempTeam->startStats.supplyLevel = j;

                    break;

                case IDC_FUEL_LEVEL:
                    GetDlgItemText(hDlg, i, buffer, 39);
                    j = atoi(buffer);

                    if (HIWORD(wParam) == EN_KILLFOCUS)
                        tempTeam->startStats.fuelLevel = j;

                    break;

                case IDC_PILOT0:
                    GetDlgItemText(hDlg, i, buffer, 39);
                    j = atoi(buffer);

                    if (HIWORD(wParam) == EN_KILLFOCUS)
                        tempTeam->firstColonel = j;

                    break;

                case IDC_PILOT1:
                    GetDlgItemText(hDlg, i, buffer, 39);
                    j = atoi(buffer);

                    if (HIWORD(wParam) == EN_KILLFOCUS)
                        tempTeam->firstCommander = j;

                    break;

                case IDC_PILOT2:
                    GetDlgItemText(hDlg, i, buffer, 39);
                    j = atoi(buffer);

                    if (HIWORD(wParam) == EN_KILLFOCUS)
                        tempTeam->firstWingman = j;

                    break;

                case IDC_PILOT3:
                    GetDlgItemText(hDlg, i, buffer, 39);
                    j = atoi(buffer);

                    if (HIWORD(wParam) == EN_KILLFOCUS)
                        tempTeam->lastWingman = j;

                    break;

                case IDC_OBJPRICOMBO:
                    if (HIWORD(wParam) == CBN_SELENDOK)
                        so = SendMessage(GetDlgItem(hDlg, i), CB_GETCURSEL, 0, 0);

                    InvalidateRect(hDlg, NULL, FALSE);
                    InvalidateRect(GetDlgItem(hDlg, IDC_OBJPRIEDIT), NULL, FALSE);
                    PostMessage(GetDlgItem(hDlg, IDC_OBJPRIEDIT), WM_PAINT, 0, 0);
                    break;

                case IDC_UNITPRICOMBO:
                    if (HIWORD(wParam) == CBN_SELENDOK)
                        su = SendMessage(GetDlgItem(hDlg, i), CB_GETCURSEL, 0, 0);

                    InvalidateRect(hDlg, NULL, FALSE);
                    InvalidateRect(GetDlgItem(hDlg, IDC_UNITPRIEDIT), NULL, FALSE);
                    PostMessage(GetDlgItem(hDlg, IDC_UNITPRIEDIT), WM_PAINT, 0, 0);
                    break;

                case IDC_MISSPRICOMBO:
                    if (HIWORD(wParam) == CBN_SELENDOK)
                        sm = SendMessage(GetDlgItem(hDlg, i), CB_GETCURSEL, 0, 0);

                    InvalidateRect(hDlg, NULL, FALSE);
                    InvalidateRect(GetDlgItem(hDlg, IDC_MISSPRIEDIT), NULL, FALSE);
                    PostMessage(GetDlgItem(hDlg, IDC_MISSPRIEDIT), WM_PAINT, 0, 0);
                    break;

                case IDC_OBJPRIEDIT:
                    GetDlgItemText(hDlg, i, buffer, 39);
                    j = atoi(buffer);

                    if (HIWORD(wParam) == EN_KILLFOCUS)
                        tempTeam->SetObjTypePriority(so, j);

                    break;

                case IDC_UNITPRIEDIT:
                    GetDlgItemText(hDlg, i, buffer, 39);
                    j = atoi(buffer);

                    if (HIWORD(wParam) == EN_KILLFOCUS)
                        tempTeam->SetUnitTypePriority(su, j);

                    break;

                case IDC_MISSPRIEDIT:
                    GetDlgItemText(hDlg, i, buffer, 39);
                    j = atoi(buffer);

                    if (HIWORD(wParam) == EN_KILLFOCUS)
                        tempTeam->SetMissionPriority(sm, j);

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

extern COLORREF SideColRGB[NUM_COUNS];

BOOL WINAPI MapDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    int x, y, i;
    static int type = IDC_MAP_OWNERSHIP;

    switch (message)
    {
        case WM_INITDIALOG:              /* message: initialize dialog box */
            return (TRUE);
            break;

        case WM_PAINT:
        {
            HDC hDC, DC;
            PAINTSTRUCT ps, nps;
            HWND hCWnd;
            RECT rect;
            int c = 0, lastcol = -1, hi, dat, s = 0;
            uchar *map_data;

            switch (type)
            {
                case IDC_MAP_SAMS0:
                case IDC_MAP_SAMS1:
                case IDC_MAP_SAMS2:
                case IDC_MAP_SAMS3:
                    map_data = TheCampaign.SamMapData;

                    if ( not map_data)
                    {
                        TheCampaign.SamMapData = MakeCampMap(MAP_SAMCOVERAGE, TheCampaign.SamMapData, 0);
                        map_data = TheCampaign.SamMapData;
                    }

                    s = (type - IDC_MAP_SAMS0) * 2;
                    break;

                case IDC_MAP_RADAR0:
                case IDC_MAP_RADAR1:
                case IDC_MAP_RADAR2:
                case IDC_MAP_RADAR3:
                    map_data = TheCampaign.RadarMapData;

                    if ( not map_data)
                    {
                        TheCampaign.RadarMapData = MakeCampMap(MAP_RADARCOVERAGE, TheCampaign.RadarMapData, 0);
                        map_data = TheCampaign.RadarMapData;
                    }

                    s = (type - IDC_MAP_RADAR0) * 2;
                    break;

                case IDC_MAP_OWNERSHIP:
                default:
                    map_data = TheCampaign.CampMapData;

                    if ( not map_data)
                    {
                        TheCampaign.MakeCampMap(MAP_OWNERSHIP);
                        map_data = TheCampaign.CampMapData;
                    }

                    break;
            }

            if (GetUpdateRect(hDlg, &rect, FALSE))
            {
                hDC = BeginPaint(hDlg, &ps);
                hCWnd = GetDlgItem(hDlg, IDC_MAP_STATIC);
                DC = BeginPaint(hCWnd, &nps);

                for (x = 0; x < (Map_Max_X / MAP_RATIO); x++)
                {
                    for (y = 0; y < (Map_Max_Y / MAP_RATIO); y++)
                    {
                        i = (((Map_Max_Y / MAP_RATIO) - y - 1) * (Map_Max_X / MAP_RATIO)) + x;

                        switch (type)
                        {
                            case IDC_MAP_RADAR0:
                            case IDC_MAP_RADAR1:
                            case IDC_MAP_RADAR2:
                            case IDC_MAP_RADAR3:
                            case IDC_MAP_SAMS0:
                            case IDC_MAP_SAMS1:
                            case IDC_MAP_SAMS2:
                            case IDC_MAP_SAMS3:
                                dat = (map_data[i] >> s) bitand 0x03;

                                if ( not dat)
                                    SetPixel(DC, x, y, RGB_BLACK);
                                else if (dat == 1)
                                    SetPixel(DC, x, y, RGB_LIGHTBLUE);
                                else if (dat == 2)
                                    SetPixel(DC, x, y, RGB_BLUE);
                                else
                                    SetPixel(DC, x, y, RGB_RED);

                                break;

                            case IDC_MAP_OWNERSHIP:
                            default:
                                hi = 4 * (i % 2);
                                dat = (map_data[i / 2] >> hi) bitand 0xF;

                                if (dat == 0xF)
                                    SetPixel(DC, x, y, RGB_GRAY);
                                else if (dat)
                                    SetPixel(DC, x, y, SideColRGB[dat]);
                                else
                                    SetPixel(DC, x, y, RGB_BLACK);

                                break;
                        }
                    }
                }

                EndPaint(hCWnd, &nps);
                EndPaint(hDlg, &ps);
            }
        }
        break;

        case WM_COMMAND:                 /* message: received a command */
            switch (LOWORD(wParam))
            {
                case IDOK:
                    EndDialog(hDlg, TRUE);      /* Exits the dialog box        */
                    return (TRUE);

                case IDC_MAP_OWNERSHIP:
                case IDC_MAP_SAMS0:
                case IDC_MAP_SAMS1:
                case IDC_MAP_SAMS2:
                case IDC_MAP_SAMS3:
                case IDC_MAP_RADAR0:
                case IDC_MAP_RADAR1:
                case IDC_MAP_RADAR2:
                case IDC_MAP_RADAR3:
                    type = LOWORD(wParam);
                    RedrawWindow(hDlg, NULL, NULL, RDW_INTERNALPAINT bitor RDW_INVALIDATE);
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

BOOL WINAPI AdjustForceRatioProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG:              /* message: initialize dialog box */
            SendMessage(GetDlgItem(hDlg, IDC_SLIDER1), TBM_SETRANGE, 0, (LPARAM)MAKELONG(0, 4));
            SendMessage(GetDlgItem(hDlg, IDC_SLIDER2), TBM_SETRANGE, 0, (LPARAM)MAKELONG(0, 4));
            SendMessage(GetDlgItem(hDlg, IDC_SLIDER3), TBM_SETRANGE, 0, (LPARAM)MAKELONG(0, 4));
            SendMessage(GetDlgItem(hDlg, IDC_SLIDER4), TBM_SETRANGE, 0, (LPARAM)MAKELONG(0, 4));
            SendMessage(GetDlgItem(hDlg, IDC_SLIDER1), TBM_SETPOS, (WPARAM)TRUE, (LPARAM)2);
            SendMessage(GetDlgItem(hDlg, IDC_SLIDER2), TBM_SETPOS, (WPARAM)TRUE, (LPARAM)2);
            SendMessage(GetDlgItem(hDlg, IDC_SLIDER3), TBM_SETPOS, (WPARAM)TRUE, (LPARAM)2);
            SendMessage(GetDlgItem(hDlg, IDC_SLIDER4), TBM_SETPOS, (WPARAM)TRUE, (LPARAM)2);
            return (TRUE);
            break;

        case WM_PAINT:
            break;

        case WM_COMMAND:                 /* message: received a command */
            switch (LOWORD(wParam))
            {
                case IDOK:
                    TheCampaign.GroundRatio = static_cast<short>(SendMessage(GetDlgItem(hDlg, IDC_SLIDER1), TBM_GETPOS, 0, 0));
                    TheCampaign.AirRatio = static_cast<short>(SendMessage(GetDlgItem(hDlg, IDC_SLIDER2), TBM_GETPOS, 0, 0));
                    TheCampaign.NavalRatio = static_cast<short>(SendMessage(GetDlgItem(hDlg, IDC_SLIDER3), TBM_GETPOS, 0, 0));
                    TheCampaign.AirDefenseRatio = static_cast<short>(SendMessage(GetDlgItem(hDlg, IDC_SLIDER4), TBM_GETPOS, 0, 0));
                    EndDialog(hDlg, TRUE);      /* Exits the dialog box        */
                    return (TRUE);

                case IDCANCEL:
                    EndDialog(hDlg, TRUE);      /* Exits the dialog box        */
                    return (FALSE);

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

BOOL WINAPI CampClipperProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    char buffer[80], filename[80];
    int left, right, top, bottom, newleft, newbottom, width, height;

    switch (message)
    {
        case WM_INITDIALOG:              /* message: initialize dialog box */
            SetWindowText(GetDlgItem(hDlg, IDC_CC_LEFT), "0");
            sprintf(buffer, "%d", Map_Max_X);
            SetWindowText(GetDlgItem(hDlg, IDC_CC_RIGHT), buffer);
            SetWindowText(GetDlgItem(hDlg, IDC_CC_WIDTH), buffer);
            sprintf(buffer, "%d", Map_Max_Y);
            SetWindowText(GetDlgItem(hDlg, IDC_CC_TOP), buffer);
            SetWindowText(GetDlgItem(hDlg, IDC_CC_HEIGHT), buffer);
            SetWindowText(GetDlgItem(hDlg, IDC_CC_BOTTOM), "0");
            SetWindowText(GetDlgItem(hDlg, IDC_CC_LEFT2), "0");
            SetWindowText(GetDlgItem(hDlg, IDC_CC_BOTTOM2), "0");
            SetWindowText(GetDlgItem(hDlg, IDC_CC_FILENAME), "CampClip.out");
            return (TRUE);
            break;

        case WM_PAINT:
            break;

        case WM_COMMAND:                 /* message: received a command */
            switch (LOWORD(wParam))
            {
                case IDOK:
                {
                    GetWindowText(GetDlgItem(hDlg, IDC_CC_LEFT), buffer, 39);
                    left = atoi(buffer);
                    GetWindowText(GetDlgItem(hDlg, IDC_CC_RIGHT), buffer, 39);
                    right = atoi(buffer);
                    GetWindowText(GetDlgItem(hDlg, IDC_CC_TOP), buffer, 39);
                    top = atoi(buffer);
                    GetWindowText(GetDlgItem(hDlg, IDC_CC_BOTTOM), buffer, 39);
                    bottom = atoi(buffer);
                    GetWindowText(GetDlgItem(hDlg, IDC_CC_WIDTH), buffer, 39);
                    width = atoi(buffer);
                    GetWindowText(GetDlgItem(hDlg, IDC_CC_HEIGHT), buffer, 39);
                    height = atoi(buffer);
                    GetWindowText(GetDlgItem(hDlg, IDC_CC_LEFT2), buffer, 39);
                    newleft = atoi(buffer);
                    GetWindowText(GetDlgItem(hDlg, IDC_CC_BOTTOM2), buffer, 39);
                    newbottom = atoi(buffer);
                    GetWindowText(GetDlgItem(hDlg, IDC_CC_FILENAME), filename, 79);
                    // Now actually do the work.
                    Map_Max_X = width;
                    Map_Max_Y = height;
                    TheCampaign.TheaterSizeX = Map_Max_X;
                    TheCampaign.TheaterSizeY = Map_Max_Y;
                    CampEntity ent;
                    GridIndex x, y;
                    int i, j, k;
                    short count[1000] = {0};
                    short first[1000] = {0};
                    short fx[1000] = {0};
                    short fy[1000] = {0};

                    {
                        VuListIterator myit(AllCampList);
                        ent = (CampEntity) myit.GetFirst();

                        while (ent)
                        {
                            ent->GetLocation(&x, &y);

                            if (x >= left and x <= right and y >= bottom and y <= top)
                            {
                                // It's a keeper
                                x = x - left + newleft;
                                y = y - bottom + newbottom;
                                ent->SetLocation(x, y);

                                if (ent->IsObjective())
                                {
                                    for (i = 0; i < ((ObjectiveClass*)ent)->GetTotalFeatures(); i++)
                                    {
                                        for (j = 0; j < 7; j++)
                                        {
                                            k = Falcon4ClassTable[((ObjectiveClass*)ent)->GetFeatureID(i)].visType[j];

                                            if ( not count[k])
                                            {
                                                first[k] = ent->Type() - VU_LAST_ENTITY_TYPE;
                                                fx[k] = x;
                                                fy[k] = y;
                                            }

                                            count[k]++;
                                        }
                                    }
                                }
                            }
                            else
                                vuDatabase->Remove(ent);

                            ent = (CampEntity) myit.GetNext();
                        }
                    }
                    // Zap theater terrain
                    FreeTheaterTerrain();
                    InitTheaterTerrain();
                    // Save data file
                    FILE *fp;
                    fp = fopen(filename, "w");

                    for (i = 1; i < 1000; i++)
                    {
                        if (count[i])
                            fprintf(fp, "Vis ID %d: %d - first: %d @ %d,%d\n", i, count[i], first[i], fx[i], fy[i]);
                    }

                    fclose(fp);
                    EndDialog(hDlg, TRUE);      /* Exits the dialog box        */
                }

                return (TRUE);

                case IDCANCEL:
                    EndDialog(hDlg, TRUE);      /* Exits the dialog box        */
                    return (FALSE);

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

BOOL WINAPI FistOfGod(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    char buffer[80];
    _TCHAR tbuffer1[80] = { "" }, tbuffer2[80] = { "" };
    int i;
    CampEntity target;

    switch (message)
    {
        case WM_INITDIALOG:              /* message: initialize dialog box */
            ReadIndexedString(300 + TheCampaign.MissionEvaluator->player_element->mission, tbuffer1, 512);
            ReadIndexedString(300 + TheCampaign.MissionEvaluator->package_mission, tbuffer2, 512);
            sprintf(buffer, "%s (%s)", tbuffer1, tbuffer2);
            SetWindowText(GetDlgItem(hDlg, IDC_FIST_MISSION), buffer);

            SendMessage(GetDlgItem(hDlg, IDC_FIST_SUCCESS), CB_ADDSTRING, 0, (LPARAM)"Horrible");
            SendMessage(GetDlgItem(hDlg, IDC_FIST_SUCCESS), CB_ADDSTRING, 0, (LPARAM)"Poor");
            SendMessage(GetDlgItem(hDlg, IDC_FIST_SUCCESS), CB_ADDSTRING, 0, (LPARAM)"Average");
            SendMessage(GetDlgItem(hDlg, IDC_FIST_SUCCESS), CB_ADDSTRING, 0, (LPARAM)"Good");
            SendMessage(GetDlgItem(hDlg, IDC_FIST_SUCCESS), CB_ADDSTRING, 0, (LPARAM)"Excellent");
            SendMessage(GetDlgItem(hDlg, IDC_FIST_SUCCESS), CB_SETCURSEL, 4, 0);

            SendMessage(GetDlgItem(hDlg, IDC_FIST_SUCCESS2), CB_ADDSTRING, 0, (LPARAM)"Failed");
            SendMessage(GetDlgItem(hDlg, IDC_FIST_SUCCESS2), CB_ADDSTRING, 0, (LPARAM)"Partially Failed");
            SendMessage(GetDlgItem(hDlg, IDC_FIST_SUCCESS2), CB_ADDSTRING, 0, (LPARAM)"Partially Accomplished");
            SendMessage(GetDlgItem(hDlg, IDC_FIST_SUCCESS2), CB_ADDSTRING, 0, (LPARAM)"Accomplished");
            SendMessage(GetDlgItem(hDlg, IDC_FIST_SUCCESS2), CB_SETCURSEL, 3, 0);
            return (TRUE);
            break;

        case WM_PAINT:
            target = FindEntity(TheCampaign.MissionEvaluator->player_element->target_id);
            SetWindowText(GetDlgItem(hDlg, IDC_FIST_TARGET_FEATURE), "None");

            if (target and target->GetClass() == CLASS_OBJECTIVE)
            {
                i = TheCampaign.MissionEvaluator->player_element->target_building;

                if (i < ((Objective)target)->GetTotalFeatures())
                {
                    FeatureClassDataType *fc;
                    fc = GetFeatureClassData(((Objective)target)->GetFeatureID(i));
                    sprintf(buffer, fc->Name);
                    SetWindowText(GetDlgItem(hDlg, IDC_FIST_TARGET_FEATURE), buffer);
                }

                sprintf(buffer, "%s (%d%%)", target->GetName(buffer, 40, FALSE), ((Objective)target)->GetObjectiveStatus());
            }
            else if (target)
            {
                sprintf(buffer, "%s (%d vehs)", target->GetFullName(buffer, 40, FALSE), ((Unit)target)->GetTotalVehicles());
                target->GetFullName(buffer, 40, FALSE);
            }
            else
                sprintf(buffer, "None");

            SetWindowText(GetDlgItem(hDlg, IDC_FIST_TARGET), buffer);
            break;

        case WM_COMMAND:                 /* message: received a command */
            switch (LOWORD(wParam))
            {
                case IDOK:
                {
                    int rating;
                    Objective o;
                    Flight flight;
                    rating = 10 * (SendMessage(GetDlgItem(hDlg, IDC_FIST_SUCCESS), CB_GETCURSEL, 0, 0) - 2);
                    // KCK: Technically, we should probably use the parent PO if there is one, but whatever...
                    o = FindNearestObjective(POList, TheCampaign.MissionEvaluator->tx, TheCampaign.MissionEvaluator->ty, NULL);

                    if (o)
                        ApplyPlayerInput(FalconLocalSession->GetTeam(), o->Id(), rating);

                    flight = FalconLocalSession->GetPlayerFlight();

                    if (flight)
                        RegroupFlight(flight);

                    EndDialog(hDlg, TRUE);      /* Exits the dialog box        */
                    return (TRUE);
                }

                case IDCANCEL:
                    EndDialog(hDlg, TRUE);      /* Exits the dialog box        */
                    return (TRUE);

                case IDC_FIST_SUCCESS:
                    break;

                    /* case IDC_FIST_SUCCESS2:
                     if (HIWORD(wParam) == CBN_SELENDOK)
                     {
                     i = SendMessage(GetDlgItem(hDlg,IDC_FIST_SUCCESS2),CB_GETCURSEL,0,0);
                     // NEW MISSION SUCCESS HERE
                     }
                     break;
                    */
                case IDC_FIST_DESTROYED:
                {
                    target = FindEntity(TheCampaign.MissionEvaluator->player_element->target_id);

                    if (target)
                    {
                        if (target->IsObjective())
                        {
                            uchar target_b = TheCampaign.MissionEvaluator->player_element->target_building;

                            if (target_b >= ((Objective)target)->GetTotalFeatures() or ((Objective)target)->GetFeatureStatus(target_b) == VIS_DESTROYED)
                            {
                                int count = 0;
                                target_b = rand() % ((Objective)target)->GetTotalFeatures();

                                while (count < 20 and (((Objective)target)->GetFeatureStatus(target_b) == VIS_DESTROYED or not ((Objective)target)->GetFeatureValue(target_b)))
                                {
                                    count++;
                                    target_b = rand() % ((Objective)target)->GetTotalFeatures();
                                }
                            }

                            ((Objective)target)->SetFeatureStatus(target_b, VIS_DESTROYED);
                            ((Objective)target)->ResetObjectiveStatus();
                        }
                        else
                        {
                            for (i = 0; i < 5; i++)
                            {
                                int count = 0, veh, n;
                                veh = rand() % VEHICLE_GROUPS_PER_UNIT;

                                while (count < 20 and ((Unit)target)->GetNumVehicles(veh) == 0)
                                {
                                    count++;
                                    veh = rand() % VEHICLE_GROUPS_PER_UNIT;
                                }

                                n = ((Unit)target)->GetNumVehicles(veh);

                                if (n > 0)
                                    ((Unit)target)->SetNumVehicles(veh, n - 1);
                            }
                        }

                        SetWindowText(GetDlgItem(hDlg, IDC_FIST_DESTROYED), "Click here to smash target again");
                    }
                    else
                    {
                        // No target found
                        SetWindowText(GetDlgItem(hDlg, IDC_FIST_DESTROYED), "No target found.  No smashing done.");
                    }

                    InvalidateRect(hDlg, NULL, FALSE);
                    InvalidateRect(GetDlgItem(hDlg, IDC_FIST_TARGET), NULL, TRUE);
                    PostMessage(GetDlgItem(hDlg, IDC_FIST_TARGET), WM_PAINT, 0, 0);
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

// Check tool globals
int gCheatRating;
int gCheatPriority;
int gCheatingOn = 0;
ulong gCheatNextTime;
ulong gCheatTimeInterval;

int DoACheatFlight(void)
{
    Unit u;
    Flight flight, best = NULL;
    int done = 0;

    {
        VuListIterator flit(AllAirList);
        u = (Unit) flit.GetFirst();

        while (u and not done)
        {
            if (u->IsFlight() and ((Flight)u)->GetUnitSquadronID() == FalconLocalSession->GetPlayerSquadronID())
            {
                flight = (Flight)u;

                if ( not best)
                    best = flight;

                if (gCheatPriority == 0 and flight->GetUnitPriority() > best->GetUnitPriority())
                    best = flight;
                else if (gCheatPriority == 1 and flight->GetUnitPriority() < best->GetUnitPriority())
                    best = flight;
                else if (gCheatPriority == 2)
                    done = 1;
            }

            u = (Unit) flit.GetNext();
        }
    }

    if (best)
    {
        Objective o;
        Package pack = (Package)best->GetUnitParent();

        if (pack)
        {
            GridIndex tx, ty;
            pack->GetUnitDestination(&tx, &ty);
            o = FindNearestObjective(POList, tx, ty, NULL);

            if (o)
                ApplyPlayerInput(FalconLocalSession->GetTeam(), o->Id(), gCheatRating);

            RegroupFlight(best);
            return 1;
        }
    }

    return 0;
}

void CheckForCheatFlight(ulong time)
{
    if (gCheatingOn and time > gCheatNextTime)
    {
        if (DoACheatFlight())
            gCheatNextTime = time + gCheatTimeInterval;
        else
            gCheatNextTime = time + CampaignMinutes;
    }
}

BOOL WINAPI CheatTool(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    char buffer[80];

    switch (message)
    {
        case WM_INITDIALOG:              /* message: initialize dialog box */
            SetWindowText(GetDlgItem(hDlg, IDC_FREQ_EDIT), "30");

            SendMessage(GetDlgItem(hDlg, IDC_PRIORITY_COMBO), CB_ADDSTRING, 0, (LPARAM)"Highest");
            SendMessage(GetDlgItem(hDlg, IDC_PRIORITY_COMBO), CB_ADDSTRING, 0, (LPARAM)"Lowest");
            SendMessage(GetDlgItem(hDlg, IDC_PRIORITY_COMBO), CB_ADDSTRING, 0, (LPARAM)"Random");
            SendMessage(GetDlgItem(hDlg, IDC_PRIORITY_COMBO), CB_SETCURSEL, 0, 0);

            SendMessage(GetDlgItem(hDlg, IDC_RESULT_COMBO), CB_ADDSTRING, 0, (LPARAM)"Horrible");
            SendMessage(GetDlgItem(hDlg, IDC_RESULT_COMBO), CB_ADDSTRING, 0, (LPARAM)"Poor");
            SendMessage(GetDlgItem(hDlg, IDC_RESULT_COMBO), CB_ADDSTRING, 0, (LPARAM)"Average");
            SendMessage(GetDlgItem(hDlg, IDC_RESULT_COMBO), CB_ADDSTRING, 0, (LPARAM)"Good");
            SendMessage(GetDlgItem(hDlg, IDC_RESULT_COMBO), CB_ADDSTRING, 0, (LPARAM)"Excellent");
            SendMessage(GetDlgItem(hDlg, IDC_RESULT_COMBO), CB_SETCURSEL, 4, 0);

            if (gCheatingOn)
                PostMessage(GetDlgItem(hDlg, IDC_CHEATONOFF), BM_SETCHECK, 1, 0);

            return (TRUE);
            break;

        case WM_PAINT:
            break;

        case WM_COMMAND:                 /* message: received a command */
            switch (LOWORD(wParam))
            {
                case IDOK:
                {
                    GetWindowText(GetDlgItem(hDlg, IDC_FREQ_EDIT), buffer, 39);
                    gCheatRating = 10 * (SendMessage(GetDlgItem(hDlg, IDC_RESULT_COMBO), CB_GETCURSEL, 0, 0) - 2);
                    gCheatPriority = SendMessage(GetDlgItem(hDlg, IDC_PRIORITY_COMBO), CB_GETCURSEL, 0, 0);
                    gCheatTimeInterval = atoi(buffer) * CampaignMinutes;
                    gCheatingOn = 1;
                    gCheatNextTime = TheCampaign.CurrentTime;
                    EndDialog(hDlg, TRUE);      /* Exits the dialog box        */
                    return (TRUE);
                }

                case IDCANCEL:
                    EndDialog(hDlg, TRUE);      /* Exits the dialog box        */
                    return (TRUE);

                case IDC_CHEATONOFF:
                    if (gCheatingOn)
                    {
                        gCheatingOn = 0;
                        PostMessage(GetDlgItem(hDlg, IDC_CHEATONOFF), BM_SETCHECK, 0, 0);
                        SetWindowText(GetDlgItem(hDlg, IDC_CHEATONOFF), "Cheating OFF");
                    }
                    else
                    {
                        gCheatingOn = 1;
                        PostMessage(GetDlgItem(hDlg, IDC_CHEATONOFF), BM_SETCHECK, 1, 0);
                        SetWindowText(GetDlgItem(hDlg, IDC_CHEATONOFF), "Cheating ON");
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

/****************************************************************************
*
* File Open Routines
*
****************************************************************************/

BOOL CALLBACK FileOpenHookProc(
    HWND hDlg,                /* window handle of the dialog box */
    UINT message,             /* type of message                 */
    WPARAM wParam,            /* message-specific information    */
    LPARAM lParam)
{
    CHAR  szTempText[256];

    switch (message)
    {
        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK)
                GetDlgItemText(hDlg, edt1, szTempText, sizeof(szTempText) - 1);

            break;
    }

    return (FALSE);
    // avoid compiler warnings at W3
    lParam;
}

BOOL OpenCampFile(HWND hWnd)
{
    char *sptr;

    strcpy(cmpFile, "");
    strcpy(cmpFileTitle, "");

    CampFileName.lStructSize       = sizeof(OPENFILENAME);
    CampFileName.hwndOwner         = hWnd;
    CampFileName.hInstance         = hInst;
    CampFileName.lpstrCustomFilter = (LPSTR) NULL;
    CampFileName.nMaxCustFilter    = 0L;
    CampFileName.nFilterIndex      = 1L;
    CampFileName.lpstrFile         = cmpFile;
    CampFileName.nMaxFile          = sizeof(cmpFile);
    CampFileName.lpstrFileTitle    = cmpFileTitle;
    CampFileName.nMaxFileTitle     = sizeof(cmpFileTitle);
    CampFileName.lpstrInitialDir   = FalconCampaignSaveDirectory;
    // CampFileName.lpstrInitialDir   = F4FindFile("savegame.dir",buf,80,&offset,&len);
    CampFileName.nFileOffset       = 0;
    CampFileName.nFileExtension    = 0;
    CampFileName.lCustData         = 0;
    CampFileName.Flags = OFN_SHOWHELP bitor OFN_PATHMUSTEXIST bitor OFN_FILEMUSTEXIST |
                         OFN_NOCHANGEDIR bitor OFN_HIDEREADONLY /* bitor OFN_ENABLEHOOK */ bitor OFN_LONGNAMES;
    // CampFileName.lpfnHook = (LPOFNHOOKPROC)MakeProcInstance(FileOpenHookProc, NULL);
    CampFileName.lpfnHook = NULL;
    CampFileName.lpstrFilter       = cmpFilter;
    CampFileName.lpstrTitle        = "Open a Campaign file";
    CampFileName.lpstrDefExt       = "*.cam";

    if ( not GetOpenFileName(&CampFileName))
    {
        ProcessCDError(CommDlgExtendedError(), hWnd);
        return FALSE;
    }

    sprintf(CampFile, CampFileName.lpstrFile);
    sptr = strrchr(CampFile, '.');

    if (sptr)
        *sptr = '\0';

    sptr = strrchr(CampFile, '\\');

    if (sptr)
    {
        *sptr = 0;
        sprintf(FalconCampUserSaveDirectory, CampFile);

        if (strcmp(FalconCampUserSaveDirectory, FalconCampaignSaveDirectory))
            ResAddPath(FalconCampUserSaveDirectory, FALSE);

        sprintf(CampFile, sptr + 1);
    }
    else
        strcpy(FalconCampUserSaveDirectory, FalconCampaignSaveDirectory);

    if ( not TheCampaign.LoadCampaign(game_Campaign, CampFile))
    {
        MonoPrint("Open Failed\n");
        return (FALSE);
    }

    return TRUE;
}

BOOL OpenTheaterFile(HWND hWnd)
{
    char *sptr;

    strcpy(thrFile, "");
    strcpy(thrFileTitle, "");

    TheaterFileName.lStructSize       = sizeof(OPENFILENAME);
    TheaterFileName.hwndOwner         = hWnd;
    TheaterFileName.hInstance         = hInst;
    TheaterFileName.lpstrCustomFilter = (LPSTR) NULL;
    TheaterFileName.nMaxCustFilter    = 0L;
    TheaterFileName.nFilterIndex      = 1L;
    TheaterFileName.lpstrFile         = thrFile;
    TheaterFileName.nMaxFile          = sizeof(thrFile);
    TheaterFileName.lpstrFileTitle    = thrFileTitle;
    TheaterFileName.nMaxFileTitle     = sizeof(thrFileTitle);
    TheaterFileName.lpstrInitialDir   = FalconCampaignSaveDirectory;
    TheaterFileName.nFileOffset       = 0;
    TheaterFileName.nFileExtension    = 0;
    TheaterFileName.lCustData         = 0;
    TheaterFileName.Flags = OFN_SHOWHELP bitor OFN_PATHMUSTEXIST bitor OFN_FILEMUSTEXIST bitor OFN_HIDEREADONLY bitor OFN_ENABLEHOOK;
    TheaterFileName.lpfnHook = (LPOFNHOOKPROC)MakeProcInstance(FileOpenHookProc, NULL);

    TheaterFileName.lpstrFilter       = thrFilter;
    TheaterFileName.lpstrTitle        = "Open a Theater file";
    TheaterFileName.lpstrDefExt       = "*.thr";

    if ( not GetOpenFileName(&TheaterFileName))
    {
        ProcessCDError(CommDlgExtendedError(), hWnd);
        return FALSE;
    }

    sprintf(TheaterFile, TheaterFileName.lpstrFile);
    sptr = strchr(TheaterFile, '.');

    if (sptr)
        *sptr = '\0';

    sptr = strrchr(TheaterFile, '\\');

    if (sptr)
    {
        *sptr = 0;
        sprintf(FalconCampUserSaveDirectory, TheaterFile);

        if (strcmp(FalconCampUserSaveDirectory, FalconCampaignSaveDirectory))
            ResAddPath(FalconCampUserSaveDirectory, FALSE);

        sprintf(TheaterFile, sptr + 1);
    }
    else
        strcpy(FalconCampUserSaveDirectory, FalconCampaignSaveDirectory);

    TheCampaign.SetTheater(TheaterFile);

    if ( not LoadTheater(TheaterFile))
    {
        MonoPrint("Open Failed.\n");
        return (FALSE);
    }

    return TRUE;
}

/***********************************************************
*
* File Save routines
*
************************************************************/

BOOL CheckFile(HWND hWnd, OPENFILENAME file)
{
    int hFile;
    OFSTRUCT OfStruct;
    WORD wStyle;
    CHAR buf[256];

    if (file.Flags bitand OFN_FILEMUSTEXIST)
        wStyle = OF_READWRITE;
    else
        wStyle = OF_READWRITE bitor OF_CREATE;

    if ((hFile = OpenFile(file.lpstrFile, &OfStruct, wStyle)) == -1)
    {
        sprintf(buf, "Could not create file %s", file.lpstrFile);
        MessageBox(hWnd, buf, NULL, MB_OK);
        return FALSE;
    }

    _lclose(hFile);
    return TRUE;
}

BOOL SaveCampFile(HWND hWnd, int mode)
{
    CHAR buf[256], *sptr;
    int save_mode = 0;

    if (strlen(CampFileName.lpstrFile) < 1)
        return FALSE;

    if ( not CheckFile(hWnd, CampFileName))
        return FALSE;

    // write it's contents into a file
    sprintf(CampFile, CampFileName.lpstrFile);
    sptr = strchr(CampFile, '.');

    if (sptr)
        *sptr = '\0';

    sptr = strrchr(CampFile, '\\');

    if (sptr)
    {
        *sptr = 0;
        sprintf(FalconCampUserSaveDirectory, CampFile);
        sprintf(CampFile, sptr + 1);
    }
    else
        strcpy(FalconCampUserSaveDirectory, FalconCampaignSaveDirectory);

    if (mode == ID_CAMPAIGN_SAVE or mode == ID_CAMPAIGN_SAVEAS)
        save_mode = 0;
    else if (mode == ID_CAMPAIGN_SAVEALLAS)
        save_mode = 1;
    else if (mode == ID_CAMPAIGN_SAVEINSTANTAS)
    {
        save_mode = 2;
        Camp_MakeInstantAction();
    }

    if ( not TheCampaign.SaveCampaign(game_Campaign, CampFile, save_mode))
    {
        sprintf(buf, "Error writing file %s", CampFileName.lpstrFile);
        MessageBox(hWnd, buf, NULL, MB_OK);
        return FALSE;
    }

    sprintf(buf, "%s", CampFileName.lpstrFile);
    MessageBox(hWnd, buf, "File Saved", MB_OK);
    return TRUE;
}

BOOL SaveTheaterFile(HWND hWnd)
{
    CHAR  buf[256];
    char *sptr;

    if (strlen(TheaterFileName.lpstrFile) < 1)
        return FALSE;

    if ( not CheckFile(hWnd, TheaterFileName))
        return FALSE;

    // Set Current Campaign to use this theater
    sprintf(TheaterFile, TheaterFileName.lpstrFile);
    sptr = strchr(TheaterFile, '.');

    if (sptr)
        *sptr = '\0';

    sptr = strrchr(TheaterFile, '\\');

    if (sptr)
    {
        *sptr = 0;
        sprintf(FalconCampUserSaveDirectory, TheaterFile);
        sprintf(TheaterFile, sptr + 1);
    }
    else
        strcpy(FalconCampUserSaveDirectory, FalconCampaignSaveDirectory);

    if ( not SaveTheater(TheaterFile))
    {
        sprintf(buf, "Error writing theater file");
        MessageBox(hWnd, buf, NULL, MB_OK);
        return FALSE;
    }

    TheCampaign.SetTheater(TheaterFile);
    return TRUE;
}

#ifdef CAMPTOOL
/*
BOOL SaveScriptedUnitFile (HWND hWnd, OPENFILENAME file)
 {
 CHAR filename[80];
 FILE* fp;

 if (strlen(file.lpstrFile) < 1)
 return FALSE;
 if ( not CheckFile(hWnd,file))
 return FALSE;
 // Set Current Campaign to use this theater
 sprintf(filename,file.lpstrFile);
 fp = fopen(filename, "wb");
 if (GlobUnit not_eq NULL)
 GlobUnit->Save(fp);
 fclose(fp);
 MessageBox( hWnd, filename, "File Saved", MB_OK );
 return TRUE;
 }
*/
#endif

/****************************************************************************
*
* Save As proceedures
*
****************************************************************************/

BOOL CALLBACK FileSaveHookProc(
    HWND hDlg,                /* window handle of the dialog box */
    UINT message,             /* type of message                 */
    WPARAM wParam,            /* message-specific information    */
    LPARAM lParam)
{
    CHAR szTempText[256];

    switch (message)
    {
        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK)
            {
                GetDlgItemText(hDlg, edt1, szTempText, sizeof(szTempText) - 1);
                break;
            }

            break;

        default:
            break;
    }

    return (FALSE);
    // avoid compiler warnings at W3
    lParam;
}

BOOL SaveAsCampFile(HWND hWnd, int mode)
{
    char buf[80];
    int offset, len;

    strcpy(cmpFile, "");
    strcpy(cmpFileTitle, "");

    CampFileName.lStructSize       = sizeof(OPENFILENAME);
    CampFileName.hwndOwner         = hWnd;
    CampFileName.hInstance         = hInst;
    CampFileName.lpstrFile         = cmpFile;
    CampFileName.lpstrCustomFilter = (LPSTR) NULL;
    CampFileName.nMaxCustFilter    = 0L;
    CampFileName.nFilterIndex      = 1L;
    CampFileName.nMaxFile          = sizeof(cmpFile);
    CampFileName.lpstrFileTitle    = cmpFileTitle;
    CampFileName.nMaxFileTitle     = sizeof(cmpFileTitle);
    CampFileName.lpstrInitialDir   = F4FindFile("savegame.dir", buf, 80, &offset, &len);
    CampFileName.nFileOffset       = 0;
    CampFileName.nFileExtension    = 0;
    CampFileName.lCustData         = 0;
    CampFileName.Flags = OFN_ENABLEHOOK bitor OFN_HIDEREADONLY bitor OFN_OVERWRITEPROMPT bitor OFN_NOCHANGEDIR;
    CampFileName.lpfnHook = (LPOFNHOOKPROC)MakeProcInstance(FileSaveHookProc, NULL);
    CampFileName.lpTemplateName = (LPSTR)NULL;

    CampFileName.lpstrFilter       = cmpFilter;
    CampFileName.lpstrTitle        = "Save Campaign As";
    CampFileName.lpstrDefExt       = "cmp";

    if (GetSaveFileName(&CampFileName))
        return(SaveCampFile(hWnd, mode));
    else
    {
        ProcessCDError(CommDlgExtendedError(), hWnd);
        return FALSE;
    }

    return (FALSE);
}

BOOL SaveAsTheaterFile(HWND hWnd)
{
    char buf[80];
    int offset, len;

    strcpy(thrFile, "");
    strcpy(thrFileTitle, "");

    TheaterFileName.lStructSize       = sizeof(OPENFILENAME);
    TheaterFileName.hwndOwner         = hWnd;
    TheaterFileName.hInstance         = hInst;
    TheaterFileName.lpstrCustomFilter = (LPSTR) NULL;
    TheaterFileName.nMaxCustFilter    = 0L;
    TheaterFileName.nFilterIndex      = 1L;
    TheaterFileName.lpstrFile         = thrFile;
    TheaterFileName.nMaxFile          = sizeof(thrFile);
    TheaterFileName.lpstrFileTitle    = thrFileTitle;
    TheaterFileName.nMaxFileTitle     = sizeof(thrFileTitle);
    TheaterFileName.lpstrInitialDir   = F4FindFile("theater.dir", buf, 80, &offset, &len);
    TheaterFileName.nFileOffset       = 0;
    TheaterFileName.nFileExtension    = 0;
    TheaterFileName.lCustData         = 0;
    TheaterFileName.Flags = OFN_ENABLEHOOK bitor OFN_HIDEREADONLY bitor OFN_OVERWRITEPROMPT bitor OFN_NOCHANGEDIR;
    TheaterFileName.lpfnHook = (LPOFNHOOKPROC)MakeProcInstance(FileSaveHookProc, NULL);
    TheaterFileName.lpTemplateName = (LPSTR)NULL;

    TheaterFileName.lpstrFilter       = thrFilter;
    TheaterFileName.lpstrTitle        = "Save Theater As";
    TheaterFileName.lpstrDefExt       = "thr";

    if (GetSaveFileName(&TheaterFileName))
        return(SaveTheaterFile(hWnd));
    else
    {
        ProcessCDError(CommDlgExtendedError(), hWnd);
        return FALSE;
    }

    return (FALSE);
}

#ifdef CAMPTOOL

BOOL SaveAsScriptedUnitFile(HWND hWnd)
{
    char buf[80];
    CHAR        file[256]      = "";
    CHAR        file_title[256] = "";
    int offset, len;
    OPENFILENAME this_file;

    this_file.lStructSize       = sizeof(OPENFILENAME);
    this_file.hwndOwner         = hWnd;
    this_file.hInstance         = hInst;
    this_file.lpstrCustomFilter = (LPSTR) NULL;
    this_file.nMaxCustFilter    = 0L;
    this_file.nFilterIndex      = 1L;
    this_file.lpstrFile         = file;
    this_file.nMaxFile          = sizeof(file);
    this_file.lpstrFileTitle    = file_title;
    this_file.nMaxFileTitle     = sizeof(file_title);
    this_file.lpstrInitialDir   = F4FindFile("theater.dir", buf, 80, &offset, &len);
    this_file.nFileOffset       = 0;
    this_file.nFileExtension    = 0;
    this_file.lCustData         = 0;
    this_file.Flags = OFN_ENABLEHOOK bitor OFN_HIDEREADONLY bitor OFN_OVERWRITEPROMPT bitor OFN_NOCHANGEDIR;
    this_file.lpfnHook = (LPOFNHOOKPROC)MakeProcInstance(FileSaveHookProc, NULL);
    this_file.lpTemplateName = (LPSTR)NULL;

    this_file.lpstrFilter       = scuFilter;
    this_file.lpstrTitle        = "Save Unit As";
    this_file.lpstrDefExt       = "scu";

    // if (GetSaveFileName(&this_file))
    // return(SaveScriptedUnitFile(hWnd,this_file));
    // else
    {
        ProcessCDError(CommDlgExtendedError(), hWnd);
        return FALSE;
    }
    return (FALSE);
}

#endif

/****************************************************************************
*
*    FUNCTION: ProcessCDError(DWORD)
*
*    PURPOSE:  Processes errors from the common dialog functions.
*
*    COMMENTS:
*
*        This function is called whenever a common dialog function
*        fails.  The CommonDialogExtendedError() value is passed to
*        the function which maps the error value to a string table.
*        The string is loaded and displayed for the user.
*
*    RETURN VALUES:
*        void.
*
****************************************************************************/
void ProcessCDError(DWORD dwErrorCode, HWND hWnd)
{
    //   WORD  wStringID;
    CHAR  buf[256];

    switch (dwErrorCode)
    {
            /*
                     case CDERR_DIALOGFAILURE:   wStringID=IDS_DIALOGFAILURE;   break;
                     case CDERR_STRUCTSIZE:      wStringID=IDS_STRUCTSIZE;      break;
                     case CDERR_INITIALIZATION:  wStringID=IDS_INITIALIZATION;  break;
                     case CDERR_NOTEMPLATE:      wStringID=IDS_NOTEMPLATE;      break;
                     case CDERR_NOHINSTANCE:     wStringID=IDS_NOHINSTANCE;     break;
                     case CDERR_LOADSTRFAILURE:  wStringID=IDS_LOADSTRFAILURE;  break;
                     case CDERR_FINDRESFAILURE:  wStringID=IDS_FINDRESFAILURE;  break;
                     case CDERR_LOADRESFAILURE:  wStringID=IDS_LOADRESFAILURE;  break;
                     case CDERR_LOCKRESFAILURE:  wStringID=IDS_LOCKRESFAILURE;  break;
                     case CDERR_MEMALLOCFAILURE: wStringID=IDS_MEMALLOCFAILURE; break;
                     case CDERR_MEMLOCKFAILURE:  wStringID=IDS_MEMLOCKFAILURE;  break;
                     case CDERR_NOHOOK:          wStringID=IDS_NOHOOK;          break;
                     case PDERR_SETUPFAILURE:    wStringID=IDS_SETUPFAILURE;    break;
                     case PDERR_PARSEFAILURE:    wStringID=IDS_PARSEFAILURE;    break;
                     case PDERR_RETDEFFAILURE:   wStringID=IDS_RETDEFFAILURE;   break;
                     case PDERR_LOADDRVFAILURE:  wStringID=IDS_LOADDRVFAILURE;  break;
                     case PDERR_GETDEVMODEFAIL:  wStringID=IDS_GETDEVMODEFAIL;  break;
                     case PDERR_INITFAILURE:     wStringID=IDS_INITFAILURE;     break;
                     case PDERR_NODEVICES:       wStringID=IDS_NODEVICES;       break;
                     case PDERR_NODEFAULTPRN:    wStringID=IDS_NODEFAULTPRN;    break;
                     case PDERR_DNDMMISMATCH:    wStringID=IDS_DNDMMISMATCH;    break;
                     case PDERR_CREATEICFAILURE: wStringID=IDS_CREATEICFAILURE; break;
                     case PDERR_PRINTERNOTFOUND: wStringID=IDS_PRINTERNOTFOUND; break;
                     case CFERR_NOFONTS:         wStringID=IDS_NOFONTS;         break;
                     case FNERR_SUBCLASSFAILURE: wStringID=IDS_SUBCLASSFAILURE; break;
                     case FNERR_INVALIDFILENAME: wStringID=IDS_INVALIDFILENAME; break;
                     case FNERR_BUFFERTOOSMALL:  wStringID=IDS_BUFFERTOOSMALL;  break;
            */
        case 0:   //User may have hit CANCEL or we got a *very* random error
            return;

        default:
            ;
            //            wStringID=IDS_UNKNOWNERROR;
    }

    /*
       LoadString(NULL, wStringID, buf, sizeof(buf));
    */
    MessageBox(hWnd, buf, NULL, MB_OK);
    return;
}

