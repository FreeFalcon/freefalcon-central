// 2001-10-25 MOVED BY S.G. To the top of the file, outside of the #ifdef CAMPTOOL since it is used by other files as well
#ifdef _DEBUG
int gDumping = 0;
#endif

#ifdef CAMPTOOL

#include <windows.h>
#include <ctype.h>
#include "FalcLib.h"
#include "fsound.h"
#include "F4Vu.h"
#include "FalcSess.h"
#include "stdhdr.h"
#include "debuggr.h"
#include "camplib.h"
#include "camp2sim.h"
#include "threadmgr.h"
#include "cmpglobl.h"
#include "math.h"
#include "time.h"
#include "errorLog.h"

#include "campdisp.h"
#include "wingraph.h"
#include "Weather.h"

#include "dialog.h"
#include "uiwin.h"
#include "resource.h"
#include "unitdlg.h"
#include "falcmesg.h"
#include "campdriv.h"
#include "cmpevent.h"
#include "team.h"
#include "PlayerOp.h"
#include "ThreadMgr.h"
#include "classtbl.h"

// Campaign Specific Includes
#include "campaign.h"
#include "path.h"
#include "listADT.h"
#include "find.h"
#include "strategy.h"
#include "update.h"
#include "atm.h"
#include "gtm.h"
#include "CampList.h"
#include "CmpClass.h"
#include "CampBase.h"
#include "Convert.h"
#include "CampStr.h"
#include "CampMap.h"
#include "Options.h"
#include "CmpRadar.h"

#include "Graphics/Include/TMap.h"

// ========================================
// Global Variables
// ========================================

// Map sizing Info
#define  MAX_XPIX 1536
#define  MAX_YPIX 768

// Mapping data
MapData MainMapData;
int MaxXSize;
int MaxYSize;
unsigned char ReBlt = FALSE;

// Windows global handles
HWND hMainWnd = NULL, hToolWnd = NULL;
HDC hMainDC, hToolDC, hMapDC = NULL;
HMENU hMainMenu;
HBITMAP hMapBM;
HCURSOR hCurWait, hCurPoint, hCur = NULL;

// Threads
static F4THREADHANDLE hCampaignThread;
static DWORD CampaignThreadID;

// CampTool Specific
unsigned char Drawing = FALSE;
unsigned char RoadsOn = TRUE;
unsigned char RailsOn = TRUE;
unsigned char PBubble = FALSE;
unsigned char PBubbleDrawn = FALSE;
unsigned char SuspendDraw = TRUE;
unsigned char ShowCodes = FALSE;
unsigned char RefreshAll = FALSE;
unsigned char ShowSearch = FALSE;
unsigned char FreshDraw = FALSE;
unsigned char Saved;
unsigned char StateEdit;
unsigned char Moving = FALSE;
unsigned char Linking = FALSE;
unsigned char LinkTool = FALSE;
unsigned char FindPath = FALSE;
unsigned char WPDrag = FALSE;
unsigned char ShowFlanks = FALSE;
unsigned char ShowPaths = FALSE;
Team ThisTeam = 0;
char Mode = 1, EditMode = 8;
char TempPriority, DrawRoad = 0, DrawRail = 0, ObjMode = 0;
short CellSize;
ReliefType DrawRelief = Flat;
CoverType DrawCover = Water;
int DrawWeather = 0;
int ShowReal = 1;
GridIndex CurX, CurY, CenX, CenY, Movx, Movy, PBx, PBy, Sel1X, Sel1Y, Sel2X, Sel2Y;
costtype cost;
CampaignState StateToEdit = NULL;
ObjectiveType TempType;
Objective OneObjective, FromObjective, ToObjective, Neighbor, GlobObj, DragObjective = NULL;
Unit OneUnit, MoveUnit, PathUnit, GlobUnit, WPUnit, DragUnit = NULL;
WORD CampHour, CampMinute, CampDay;
WayPoint GlobWP = NULL;
F4PFList GlobList;
short ptSelected = 0;

extern int asAgg;

// Returns upper left hand corner of Cell, in window pixel coordinates
#define POSX(X) (short)((X)*md->CellSize-md->PFX)
#define POSY(Y) (short)(md->PLY-(Y+1)*md->CellSize)

// For testing
unsigned char SHOWSTATS;
int NoInput = 1;
int gMoveFlags = PATH_ENEMYOK bitor PATH_ROADOK;
int gMoveType = Tracked;
int gMoveWho = 6;
extern BOOL WINAPI SelectSquadron(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

// Renaming tool stuff
VU_ID_NUMBER RenameTable[65536] = { 0 };
int gRenameIds = 0;

extern int displayCampaign;
extern int maxSearch;

extern void ShowMissionLists(void);

// 2001-10-25 MOVED BY S.G. To the top of the file, outside of the #ifdef CAMPTOOL since it is used by other files as well
//#ifdef _DEBUG
//int gDumping = 0;
//#endif

// ========================================
// Necessary Prototypes
// ========================================

LRESULT CALLBACK CampaignWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

void CampMain(HINSTANCE hInstance, int    nCmdShow);

void ProcessCommand(int Key);

void SetRefresh(MapData md);

void GetCellScreenRect(MapData md, GridIndex X, GridIndex Y, RECT *r);

// ====================================================================
// Campaign Support Functions (These are called by the campaign itself)
// ====================================================================

void ReBltWnd(MapData md, HDC DC)
{
    RECT r;

    if ( not md)
        md = MainMapData;

    if ( not MainMapData)
        return; // Map's not open

    GetClientRect(md->hMapWnd, &r);
    BitBlt(md->hMapDC, r.left, r.top, r.right, r.bottom, DC, r.left, r.top, SRCCOPY);
    // MonoPrint("blt to storage: %d,%d:%d,%d\n",r.left,r.top,r.right-r.left,r.bottom-r.top);
}

// This is a temporary routine for testing...
// Shows the pattern taken with my search routine.
void ShowWhere(MapData md, GridIndex x, GridIndex y, int color)
{
    RECT r;
    PAINTSTRUCT ps;
    HDC DC;

    if ( not md)
        md = MainMapData;

    if ( not MainMapData)
        return; // Map's not open

    GetCellScreenRect(md, x, y, &r);
    InvalidateRect(md->hMapWnd, &r, FALSE);
    DC = BeginPaint(md->hMapWnd, &ps);
    _setcolor(DC, color);
    _rectangle(DC, _GFILLINTERIOR, POSX(x), POSY(y), POSX(x + 1), POSY(y + 1));
    EndPaint(md->hMapWnd, &ps);
}

// Show the time
void ShowTime(CampaignTime t)
{
    RECT r;
    WORD nm, nh;

    if ( not hToolWnd)
        return;

    while (t > CampaignDay)
        t -= CampaignDay;

    nh = (WORD)(t / CampaignHours);
    nm = (WORD)((t - nh * CampaignHours) / CampaignMinutes);

    if (nm not_eq CampMinute)
    {
        _setcolor(hToolDC, White);
        CampHour = nh;
        CampMinute = nm;
        CampDay = (WORD)TheCampaign.GetCurrentDay();
        GetClientRect(hToolWnd, &r);
        InvalidateRect(hToolWnd, &r, FALSE);
        PostMessage(hToolWnd, WM_PAINT, 0, 0);
    }
}

void ShowLink(MapData md, Objective o, Objective n, int color)
{
    RECT r;
    PAINTSTRUCT ps;
    HDC DC;
    GridIndex x, y;

    if ( not md)
        md = MainMapData;

    if ( not MainMapData)
        return; // Map's not open

    GetClientRect(md->hMapWnd, &r);
    InvalidateRect(md->hMapWnd, &r, FALSE);
    DC = BeginPaint(md->hMapWnd, &ps);
    _setcolor(DC, color);
    o->GetLocation(&x, &y);
    _moveto(DC, POSX(x) + (md->CellSize >> 1), POSY(y) + (md->CellSize >> 1));
    n->GetLocation(&x, &y);
    _lineto(DC, POSX(x) + (md->CellSize >> 1), POSY(y) + (md->CellSize >> 1));
    EndPaint(md->hMapWnd, &ps);
}

// Draw a waypoint box
void ShowWP(MapData md, GridIndex X, GridIndex Y, int color)
{
    RECT r;
    PAINTSTRUCT ps;
    HDC DC;

    if ( not md)
        md = MainMapData;

    if ( not MainMapData)
        return; // Map's not open

    GetCellScreenRect(md, X, Y, &r);
    InvalidateRect(md->hMapWnd, &r, FALSE);
    DC = BeginPaint(md->hMapWnd, &ps);
    _setcolor(DC, color);
    _rectangle(DC, _GBORDER, POSX(X), POSY(Y), POSX(X) + md->CellSize * 4, POSY(Y) + md->CellSize * 4);
    EndPaint(md->hMapWnd, &ps);
}

void ShowWPLeg(MapData md, GridIndex x, GridIndex y, GridIndex X, GridIndex Y, int color)
{
    RECT r;
    PAINTSTRUCT ps;
    HDC DC;

    if ( not md)
        md = MainMapData;

    if ( not MainMapData)
        return; // Map's not open

    GetClientRect(md->hMapWnd, &r);
    InvalidateRect(md->hMapWnd, &r, FALSE);
    DC = BeginPaint(md->hMapWnd, &ps);
    _setcolor(DC, color);
    _moveto(DC, POSX(x) + (md->CellSize >> 1), POSY(y) + (md->CellSize >> 1));
    _lineto(DC, POSX(X) + (md->CellSize >> 1), POSY(Y) + (md->CellSize >> 1));
    EndPaint(md->hMapWnd, &ps);
    ShowWP(md, x, y, color);
    ShowWP(md, X, Y, color);
}

void ShowPath(MapData md, GridIndex X, GridIndex Y, Path p, int color)
{
    RECT r;
    PAINTSTRUCT ps;
    HDC DC;
    int i, d, step;
    GridIndex x, y;

    if ( not md)
        md = MainMapData;

    if ( not MainMapData)
        return; // Map's not open

    GetClientRect(md->hMapWnd, &r);
    InvalidateRect(md->hMapWnd, &r, FALSE);
    DC = BeginPaint(md->hMapWnd, &ps);
    _setcolor(DC, color);
    _moveto(DC, POSX(X) + (md->CellSize >> 1), POSY(Y) + (md->CellSize >> 1));

    if (QuickSearch)
        step = QuickSearch;
    else
        step = 1;

    for (i = 0; i < p->GetLength(); i++)
    {
        d = p->GetDirection(i);

        if (d >= 0)
        {
            x = X + dx[d] * step;
            y = Y + dy[d] * step;
            _lineto(DC, POSX(x) + (md->CellSize >> 1), POSY(y) + (md->CellSize >> 1));
            X = x;
            Y = y;
        }
    }

    EndPaint(md->hMapWnd, &ps);
}

void ShowObjectivePath(MapData md, Objective o, Path p, int color)
{
    RECT r;
    PAINTSTRUCT ps;
    HDC DC;
    int i, d;
    GridIndex x, y;

    if ( not md)
        md = MainMapData;

    if ( not MainMapData)
        return; // Map's not open

    GetClientRect(md->hMapWnd, &r);
    InvalidateRect(md->hMapWnd, &r, FALSE);
    DC = BeginPaint(md->hMapWnd, &ps);
    _setcolor(DC, color);
    o->GetLocation(&x, &y);
    _moveto(DC, POSX(x) + (md->CellSize >> 1), POSY(y) + (md->CellSize >> 1));

    for (i = 0; i < p->GetLength(); i++)
    {
        d = p->GetDirection(i);

        if (d >= 0)
        {
            o = o->GetNeighbor(d);
            o->GetLocation(&x, &y);
            _lineto(DC, POSX(x) + (md->CellSize >> 1), POSY(y) + (md->CellSize >> 1));
        }
    }

    EndPaint(md->hMapWnd, &ps);
}

void ShowPathHistory(MapData md, GridIndex X, GridIndex Y, Path p, int color)
{
    RECT r;
    PAINTSTRUCT ps;
    HDC DC;
    int i, d, step;
    GridIndex x, y;

    if ( not md)
        md = MainMapData;

    if ( not MainMapData)
        return; // Map's not open

    GetClientRect(md->hMapWnd, &r);
    InvalidateRect(md->hMapWnd, &r, FALSE);
    DC = BeginPaint(md->hMapWnd, &ps);
    _setcolor(DC, color);
    _moveto(DC, POSX(X) + (md->CellSize >> 1), POSY(Y) + (md->CellSize >> 1));
    i = 1;

    if (QuickSearch)
        step = QuickSearch;
    else
        step = 1;

    d = p->GetPreviousDirection(i);

    while (d not_eq Here)
    {
        x = X - dx[d] * step;
        y = Y - dy[d] * step;
        _lineto(DC, POSX(x) + (md->CellSize >> 1), POSY(y) + (md->CellSize >> 1));
        X = x;
        Y = y;
        i++;
    }

    EndPaint(md->hMapWnd, &ps);
}

void ShowRange(MapData md, GridIndex X, GridIndex Y, int range, int color)
{
    RECT r;
    PAINTSTRUCT ps;
    HDC DC;
    GridIndex x, y;

    if ( not md)
        md = MainMapData;

    if ( not MainMapData)
        return; // Map's not open

    GetClientRect(md->hMapWnd, &r);
    InvalidateRect(md->hMapWnd, &r, FALSE);
    DC = BeginPaint(md->hMapWnd, &ps);
    _setcolor(DC, color);
    range *= md->CellSize;
    x = (short)(POSX(X) + (md->CellSize >> 1));
    y = (short)(POSY(Y) + (md->CellSize >> 1));
    _ellipse(DC, _GBORDER, x - range, y - range, x + range, y + range);
    EndPaint(md->hMapWnd, &ps);
}

void ShowLinkCosts(Objective f, Objective t)
{
    int i, j;

    MonoPrint("Link Costs: ");

    for (i = 0; i < f->NumLinks(); i++)
    {
        if (f->GetNeighbor(i) == t)
        {
            for (j = 0; j < MOVEMENT_TYPES; j++)
                MonoPrint("%d ", f->link_data[i].costs[j]);
        }
    }

    MonoPrint("\n");
}

void ShowDivisions(HDC DC, MapData md)
{
    int t;
    Division d;
    Unit u;

    for (t = 0; t < NUM_TEAMS; t++)
    {
        d = GetFirstDivision(t);

        while (d)
        {
            u = d->GetFirstUnitElement();

            while (u)
            {
                if (u->GetSType() == d->type)
                {
                    DisplayUnit(DC, u, (short)(POSX(d->x - 2) + (md->CellSize >> 3) * 5), (short)(POSY(d->y + 2) + (md->CellSize >> 2) * 5), (short)(md->CellSize >> 1) * 5);
                    u = NULL;
                }
                else
                    u = d->GetNextUnitElement();
            }

            d = d->next;
        }
    }
}

void RedrawUnit(Unit u)
{
    GridIndex  x, y;
    PAINTSTRUCT ps;
    RECT r;
    HDC DC;
    MapData md = MainMapData;

    if ( not MainMapData)
        return;

    u->GetLocation(&x, &y);
    GetCellScreenRect(md, x, y, &r);
    InvalidateRect(md->hMapWnd, &r, FALSE);
    DC = BeginPaint(md->hMapWnd, &ps);
    DisplayUnit(DC, u, (short)(POSX(x) + (md->CellSize >> 3)), (short)(POSY(y) + (md->CellSize >> 2)), (short)(md->CellSize >> 1));
    EndPaint(md->hMapWnd, &ps);
}

// return 0 or 1 depending on if this unit is a type we want to display
int DisplayOk(Unit u)
{
    if (ShowReal == 1 and u->Real() and not u->Inactive())
        return 1;
    else if ( not ShowReal and u->Parent() and not u->Inactive())
        return 1;
    else if (ShowReal == 2 and u->Inactive())
        return 1;

    return 0;
}

// ===============================================
// Mutual support functions
// ===============================================

void GetCellScreenRect(MapData md, GridIndex X, GridIndex Y, RECT *r)
{
    if ( not md)
        return;

    r->left = POSX(X);
    r->top = POSY(Y);
    r->right = r->left + md->CellSize;
    r->bottom = r->top + md->CellSize;
}

void RedrawCell(MapData md, GridIndex X, GridIndex Y)
{
    RECT r;
    PAINTSTRUCT ps;
    Objective  o;
    Unit u = NULL;
    HDC DC;
    short scale = 1;

    if ( not md)
        md = MainMapData;

    if ( not MainMapData)
        return; // Map's not open

    if (X < md->FX or X > md->LX or Y < md->FY or Y > md->LY)
        return;

    GetCellScreenRect(md, X, Y, &r);
    InvalidateRect(md->hMapWnd, &r, FALSE);
    DC = BeginPaint(md->hMapWnd, &ps);
    DisplayCellData(DC, X, Y, POSX(X), POSY(Y), md->CellSize, Mode, RoadsOn, RailsOn);
    o = GetObjectiveByXY(X, Y);

    if (o)
    {
        DisplayObjective(DC, o, POSX(X), POSY(Y), md->CellSize);
    }

    if (ShowReal == 1)
        u = FindUnitByXY(AllRealList, X, Y, 0);
    else if ( not ShowReal)
    {
        u = FindUnitByXY(AllParentList, X, Y, 0);
        scale = 3;
        X--;
        Y++;
    }
    else if (ShowReal == 2)
        u = FindUnitByXY(InactiveList, X, Y, 0);
    else
    {
        Division d;
        d = FindDivisionByXY(X, Y);

        if (d)
            u = d->GetUnitElement(0);

        X -= 2;
        Y += 2;
        scale = 5;
    }

    if (u)
        DisplayUnit(DC, u, (short)(POSX(X) + (md->CellSize >> 3)*scale), (short)(POSY(Y) + (md->CellSize >> 2)*scale), (short)((md->CellSize >> 1)*scale));

    EndPaint(md->hMapWnd, &ps);
}

// =======================================
// Tool support functions
// =======================================

void ShowEmitters(MapData md, HDC DC)
{
    CampEntity e;
    GridIndex x, y;
    int range;
    VuListIterator myit(EmitterList);

    e = GetFirstEntity(&myit);

    while (e)
    {
        range = e->GetDetectionRange(Air);

        if (range > 0)
        {
            e->GetLocation(&x, &y);
            DisplaySideRange(DC, e->GetOwner(), (short)(POSX(x) + (md->CellSize >> 1)), (short)(POSY(y) + (md->CellSize >> 1)), range * md->CellSize);
        }

        e = GetNextEntity(&myit);
    }
}

void ShowSAMs(MapData md, HDC DC)
{
    CampEntity e;
    GridIndex x, y;
    int range;
    VuListIterator myit(AirDefenseList);

    e = GetFirstEntity(&myit);

    while (e)
    {
        if (md->SAMs == 1)
            range = e->GetWeaponRange(Air);
        else
            range = e->GetWeaponRange(LowAir);

        if (range > 0 and (e->IsObjective() or (e->GetDomain() not_eq DOMAIN_AIR and e->IsUnit() and not ((Unit)e)->Moving())))
        {
            e->GetLocation(&x, &y);
            DisplaySideRange(DC, e->GetOwner(), (short)(POSX(x) + (md->CellSize >> 1)), (short)(POSY(y) + (md->CellSize >> 1)), range * md->CellSize);
        }

        e = GetNextEntity(&myit);
    }
}

void ShowPlayerBubble(MapData md, HDC DC, int show_ok)
{
    //GridIndex x=0,y=0;
    //int bubble_size = 20; // This is actually a per-entity value

    //if (FalconLocalSession->Camera(0)){
    // // sfr: xy order
    // ::vector pos = { FalconLocalSession->Camera(0)->XPos(), FalconLocalSession->Camera(0)->YPos() };
    // ConvertSimToGrid(&pos, &x, &y );
    // y = SimToGrid(FalconLocalSession->Camera(0)->XPos());
    // x = SimToGrid(FalconLocalSession->Camera(0)->YPos());
    //}
    //else {
    // show_ok = FALSE;
    //}

    //if (PBubbleDrawn){
    // SetROP2(DC, R2_NOT);
    // DisplaySideRange(
    // DC,0,(short)(POSX(PBx)+(md->CellSize>>1)),(short)(POSY(PBy)+(md->CellSize>>1)),bubble_size*md->CellSize
    // );
    // DisplaySideRange(
    // DC,0,(short)(POSX(PBx)+(md->CellSize>>1)),(short)(POSY(PBy)+(md->CellSize>>1)),1*md->CellSize
    // );
    // SetROP2(DC, R2_COPYPEN);
    // PBubbleDrawn = FALSE;
    //}
    //if (show_ok){
    // SetROP2(DC, R2_NOT);
    // DisplaySideRange(
    // DC,0,(short)(POSX(x)+(md->CellSize>>1)),(short)(POSY(y)+(md->CellSize>>1)),bubble_size*md->CellSize
    // );
    // DisplaySideRange(
    // DC,0,(short)(POSX(x)+(md->CellSize>>1)),(short)(POSY(y)+(md->CellSize>>1)),1*md->CellSize
    // );
    // SetROP2(DC, R2_COPYPEN);
    // PBubbleDrawn = TRUE;
    // PBx = x;
    // PBy = y;
    //}
}

void RedrawPlayerBubble()
{
    RECT r;
    PAINTSTRUCT  ps;
    HDC DC;
    MapData md = MainMapData;


    GetClientRect(md->hMapWnd, &r);
    InvalidateRect(md->hMapWnd, &r, FALSE);
    DC = BeginPaint(md->hMapWnd, &ps);
    ShowPlayerBubble(md, DC, 1);
    EndPaint(md->hMapWnd, &ps);
}

void ChangeCell(GridIndex X, GridIndex Y)
{
    int i;//,x,y;
    PAINTSTRUCT ps;
    HDC DC;
    RECT r;
    MapData md = MainMapData;

    Trim(&X, &Y);

    switch (EditMode)
    {
        case 0:
        case 1:
            SetGroundCover(GetCell(X, Y), DrawCover);
            break;

        case 2:
            SetReliefType(GetCell(X, Y), DrawRelief);
            break;

        case 3:
            SetRoadCell(GetCell(X, Y), DrawRoad);

            for (i = 0; i < 8; i++)
            {
                FreshDraw = TRUE;
                RedrawCell(MainMapData, (short)(X + dx[i]), (short)(Y + dy[i]));
            }

            break;

        case 4:
            SetRailCell(GetCell(X, Y), DrawRail);

            for (i = 0; i < 8; i++)
            {
                FreshDraw = TRUE;
                RedrawCell(MainMapData, (short)(X + dx[i]), (short)(Y + dy[i]));
            }

            break;

        case 5:
            ((WeatherClass*)realWeather)->SetCloudCover(X, Y, DrawWeather);
            GetClientRect(md->hMapWnd, &r);
            InvalidateRect(md->hMapWnd, &r, FALSE);
            DC = BeginPaint(md->hMapWnd, &ps);

            if (PBubble)
                ShowPlayerBubble(md, DC, 0);

            //JAM - FIXME
            // for (x=X-FloatToInt32(CLOUD_CELL_SIZE); x<X+FloatToInt32(CLOUD_CELL_SIZE); x++)
            // for (y=Y-FloatToInt32(CLOUD_CELL_SIZE); y<Y+FloatToInt32(CLOUD_CELL_SIZE); y++)
            // DisplayCellData(DC,x,y,POSX(x),POSY(y),md->CellSize,Mode,0,0);
            ReBltWnd(md, DC);

            if (PBubble)
                ShowPlayerBubble(md, DC, 1);

            EndPaint(md->hMapWnd, &ps);
            break;

        case 6:
            ((WeatherClass*)realWeather)->SetCloudLevel(X, Y, DrawWeather);
            GetClientRect(md->hMapWnd, &r);
            InvalidateRect(md->hMapWnd, &r, FALSE);
            DC = BeginPaint(md->hMapWnd, &ps);

            if (PBubble)
                ShowPlayerBubble(md, DC, 0);

            //JAM - FIXME
            // for (x=X-FloatToInt32(CLOUD_CELL_SIZE); x<X+FloatToInt32(CLOUD_CELL_SIZE); x++)
            // for (y=Y-FloatToInt32(CLOUD_CELL_SIZE); y<Y+FloatToInt32(CLOUD_CELL_SIZE); y++)
            // DisplayCellData(DC,x,y,POSX(x),POSY(y),md->CellSize,Mode,0,0);
            ReBltWnd(md, DC);

            if (PBubble)
                ShowPlayerBubble(md, DC, 1);

            EndPaint(md->hMapWnd, &ps);
            break;

        default:
            break;
    }

    FreshDraw = TRUE;
    RedrawCell(MainMapData, X, Y);
}

void ResizeCursor(void)
{
    if (hCur)
        DestroyCursor(hCur);

    switch (CellSize)
    {
        case 2:
            hCur = LoadCursor(hInst, MAKEINTRESOURCE(IDC_CURSOR1));
            break;

        case 4:
            hCur = LoadCursor(hInst, MAKEINTRESOURCE(IDC_CURSOR2));
            break;

        case 8:
            hCur = LoadCursor(hInst, MAKEINTRESOURCE(IDC_CURSOR3));
            break;

        case 16:
            hCur = LoadCursor(hInst, MAKEINTRESOURCE(IDC_CURSOR4));
            break;

        default:
            hCur = LoadCursor(hInst, MAKEINTRESOURCE(IDC_CURSOR0));
            break;
    }

    SetCursor(hCur);
}

void FindBorders(MapData md)
{
    RECT windim;
    short xsize, ysize;
    GridIndex tx, ty;

    GetClientRect(md->hMapWnd, &windim);
    xsize = (short)(windim.right); // - windim->left);
    ysize = (short)(windim.bottom); // - windim->top);
    tx = md->CenX - (xsize / md->CellSize) / 2;
    ty = md->CenY + (ysize / md->CellSize) / 2;

    if (tx - md->FX > 1 or md->FX - tx > 1)
        md->FX = tx;

    if (ty - md->LY > 1 or md->LY - ty > 1)
        md->LY = ty;

    md->LX = md->FX + (xsize / md->CellSize) + 1;
    md->FY = md->LY - (ysize / md->CellSize) - 1;

    if (md->LX > Map_Max_X)
    {
        md->LX = Map_Max_X;
        md->FX = Map_Max_X - xsize / md->CellSize;

        if (xsize / md->CellSize)
            md->FX--;
    }

    if (md->LY > Map_Max_Y)
    {
        md->LY = Map_Max_Y;
        md->FY = Map_Max_Y - ysize / md->CellSize;
    }

    if (md->FX < 0)
    {
        md->FX = 0;
        md->LX = xsize / md->CellSize + 1;
    }

    if (md->FY < 0)
    {
        md->FY = 0;
        md->LY = ysize / md->CellSize + 1;
    }

    if (md->LX > Map_Max_X)
        md->LX = Map_Max_X;

    if (md->LY > Map_Max_Y)
        md->LY = Map_Max_Y;

    md->PFX = md->FX * md->CellSize;
    md->PLY = md->LY * md->CellSize;
    md->PLX = md->PFX + xsize;
    md->PFY = md->PLY - ysize;
    md->CenX = CenX = (md->LX - md->FX) / 2 + md->FX;
    md->CenY = CenY = (md->LY - md->FY) / 2 + md->FY;
    GetWindowRect(md->hMapWnd, &windim);
    md->WULX = (short)windim.left;
    md->WULY = (short)windim.top;
    md->CULX = GetSystemMetrics(SM_CXSIZEFRAME);
    md->CULY = (SHORT)(windim.bottom - windim.top - ysize);
    md->CULY -= GetSystemMetrics(SM_CYHSCROLL);
    md->CULY -= GetSystemMetrics(SM_CYSIZEFRAME);
}

int CenPer(MapData md, GridIndex cen, int side)
{
    RECT windim;
    int size, range, per;

    GetClientRect(md->hMapWnd, &windim);

    if (side == XSIDE)
    {
        size = (short)(windim.right); // - windim->left);
        range = 1 + Map_Max_X - (size / md->CellSize);
        per = (cen - ((size / md->CellSize) / 2)) * 100 / range;
    }
    else
    {
        size = (short)(windim.bottom); // - windim->top);
        range = 1 + Map_Max_Y - (size / md->CellSize);
        per = ((Map_Max_Y - cen) - ((size / md->CellSize) / 2)) * 100 / range;
    }

    return per;
}

F4PFList GetSquadsFlightList(VU_ID id)
{
    F4PFList list = new FalconPrivateList(&AllAirFilter);
    Unit u;

    VuListIterator myit(AllRealList);
    list->Register();
    u = GetFirstUnit(&myit);

    while (u)
    {
        if (u->GetUnitParentID() == id and u->GetDomain() == DOMAIN_AIR and u->GetType() == TYPE_FLIGHT)
        {
            list->ForcedInsert(u);
        }

        u = GetNextUnit(&myit);
    }

    return list;
}

#define MAX_OBJ_TYPES 33 // WARNING: This should go someplace real...

typedef struct
{
    short ClassId;
    short stype;
} OBJ_DATA;

void MakeCampaignNew(void)
{
    // Remove all flights, reset times, and generally do what is necessary
    // to make a campaign be "unrun"
    Unit u;
    int i, id;
    ATMAirbaseClass *cur;

    // Clearup unit data
    {
        VuListIterator uit(AllUnitList);
        u = (Unit)uit.GetFirst();

        while (u)
        {
            if (u->IsFlight() or u->IsPackage())
            {
                u->KillUnit();
                u->Remove();
            }
            else if (u->IsBattalion())
            {
                ((Battalion)u)->SetUnitSupply(100);
                ((Battalion)u)->last_move = 0;
                ((Battalion)u)->last_combat = 0;
                ((Battalion)u)->SetSpottedTime(0);
                ((Battalion)u)->SetUnitFatigue(0);
                ((Battalion)u)->SetUnitMorale(100);
                ((Battalion)u)->SetFullStrength(0);
                ((Battalion)u)->last_obj = FalconNullId;
                ((Battalion)u)->deag_data = NULL;
                ((Battalion)u)->DisposeWayPoints();
                ((Battalion)u)->ClearUnitPath();
                ((Battalion)u)->SetOrders(0);
                ((Battalion)u)->SetPObj(FalconNullId);
                ((Battalion)u)->SetSObj(FalconNullId);
                ((Battalion)u)->SetAObj(FalconNullId);
            }
            else if (u->IsBrigade()/*u->IsBattalion()*/)
            {
                ((Brigade)u)->SetFullStrength(0);
                ((Brigade)u)->SetOrders(0);
                ((Brigade)u)->SetPObj(FalconNullId);
                ((Brigade)u)->SetSObj(FalconNullId);
                ((Brigade)u)->SetAObj(FalconNullId);
            }
            else if (u->IsSquadron())
            {
                ((Squadron)u)->SetUnitAirbase(FalconNullId);

                for (i = 0; i < VEHICLE_GROUPS_PER_UNIT; i++)
                    ((Squadron)u)->ClearSchedule(i);

                ((Squadron)u)->SetHotSpot(FalconNullId);
                ((Squadron)u)->SetAssigned(0);

                for (i = 0; i < ARO_OTHER; i++)
                    ((Squadron)u)->SetRating(i, 0);

                ((Squadron)u)->SetAAKills(0);
                ((Squadron)u)->SetAGKills(0);
                ((Squadron)u)->SetASKills(0);
                ((Squadron)u)->SetANKills(0);
                ((Squadron)u)->SetMissionsFlown(0);
                ((Squadron)u)->SetMissionScore(0);
                ((Squadron)u)->SetTotalLosses(0);
                ((Squadron)u)->SetPilotLosses(0);

                for (i = 0; i < PILOTS_PER_SQUADRON; i++)
                {
                    id = ((Squadron)u)->GetPilotData(i)->pilot_id;
                    // 2000-11-17 MODIFIED BY S.G. NEED TO PASS THE 'airExperience' OF THE TEAM SO I CAN USE IT AS A BASE
                    // ((Squadron)u)->GetPilotData(i)->ResetStats();
                    ((Squadron)u)->GetPilotData(i)->ResetStats(TeamInfo[((Squadron)u)->GetOwner()]->airExperience);

                    ((Squadron)u)->GetPilotData(i)->pilot_id = id;
                }
            }

            u = (Unit)uit.GetNext();
        }

    }
    // Clear team and planner data

    for (i = 0; i < NUM_TEAMS; i++)
    {
        TeamInfo[i]->atm->squadrons = 0;
        TeamInfo[i]->atm->requestList->Purge();
        TeamInfo[i]->atm->delayedList->Purge();
        TeamInfo[i]->atm->supplyBase = 0;
        cur = TeamInfo[i]->atm->airbaseList;

        while (cur)
        {
            cur->id = FalconNullId;
            memset(cur->schedule, 0, sizeof(uchar)*ATM_MAX_CYCLES);
            cur->usage = 0;
            cur = cur->next;
        }
    }

    // Clear campaign data
    TheCampaign.lastGroundTask = 0;
    TheCampaign.lastAirTask = 0;
    TheCampaign.lastNavalTask = 0;
    TheCampaign.lastGroundPlan = 0;
    TheCampaign.lastAirPlan = 0;
    TheCampaign.lastNavalPlan = 0;
    TheCampaign.lastResupply = 0;
    TheCampaign.lastRepair = 0;
    TheCampaign.lastReinforcement = 0;
    TheCampaign.lastStatistic = 0;
    TheCampaign.lastMajorEvent = 0;
    TheCampaign.last_victory_time = 0;
    TheCampaign.TimeStamp = 0;
    TheCampaign.CurrentDay = 0;
    TheCampaign.FreeCampMaps();
}

void MatchObjectiveTypes(void)
{
    char *file, buffer[80];
    GridIndex x, y;
    FILE *fp;
    int i, j, k, TexIndex, id, set;
    short *counts, type, index, matches;
    ObjClassDataType* oc;

    GridIndex *locs; //DSP

    OBJ_DATA stypes[20];

    memset(stypes, 0, sizeof(OBJ_DATA) * 20);

    CampEnterCriticalSection();
    fp = OpenCampFile("TypeErro", "too", "wb");
    //FromObjective = GetFirstObjective(&oit);
    counts = new short[(MaxTextureType + 1)*NumEntities];
    memset(counts, 0, sizeof(short) * (MaxTextureType + 1)*NumEntities);

    locs = new GridIndex[(MaxTextureType + 1)*NumEntities * 2]; //DSP
    memset(locs, 0, sizeof(GridIndex) * (MaxTextureType + 1)*NumEntities * 2);

    while (FromObjective not_eq NULL)
    {
        if (FromObjective->ManualSet())
        {
            // We want to leave this as we set it.
            /*FromObjective = GetNextObjective(&oit);*/
            continue;
        }

        FromObjective->GetLocation(&x, &y);
        file = GetFilename(x, y);
        file = strchr(file, '.') - 3;
        TexIndex = GetTextureIndex(x, y);
        j = i = 1;
        set = 0;
        matches = 0;
        index = FromObjective->Type() - VU_LAST_ENTITY_TYPE;
        type = FromObjective->GetType();

        while (j)
        {
            j = GetClassID(DOMAIN_LAND, CLASS_OBJECTIVE, (uchar)type, i, 0, 0, 0, 0);

            if (j)
            {
                oc = (ObjClassDataType*) Falcon4ClassTable[j].dataPtr;

                if (oc and strncmp(oc->Name, file, 3) == 0)
                {
                    //record values for matches, so we can choose later

                    stypes[matches].ClassId = j;
                    stypes[matches].stype = i;
                    matches++;
                }
            }

            i++;
        }


        //if ( not set) // no matching texture
        if ( not matches)
        {
            FromObjective->SetObjectiveSType(1);
            index = GetClassID(DOMAIN_LAND, CLASS_OBJECTIVE, (uchar)type, 1, 0, 0, 0, 0);
            counts[TexIndex * NumEntities + index]--;
            locs[(TexIndex * NumEntities + index) * 2] = x; //DSP
            locs[(TexIndex * NumEntities + index) * 2 + 1] = y;
        }
        else
        {
            //use number of matches found to randomly choose which values to use
            id = rand() % matches;
            FromObjective->SetObjectiveSType((uchar)stypes[id].stype);
            index = stypes[id].ClassId;
            memset(stypes, 0, sizeof(OBJ_DATA) * 20);

            counts[TexIndex * NumEntities + index]++;
            locs[(TexIndex * NumEntities + index) * 2] = x; //DSP
            locs[(TexIndex * NumEntities + index) * 2 + 1] = y;
        }

        //FromObjective = GetNextObjective(&oit);
    }

    // Now output totals:
    for (i = 1; i < MAX_OBJ_TYPES; i++)
    {
        j = index = 1;

        while (index)
        {
            index = GetClassID(DOMAIN_LAND, CLASS_OBJECTIVE, (uchar)i, (uchar)j, 0, 0, 0, 0);
            oc = (ObjClassDataType*) Falcon4ClassTable[index].dataPtr;

            for (k = 0; k < MaxTextureType + 1 and index; k++)
            {
                if ( not counts[k * NumEntities + index])
                    continue;

                file = GetTextureId(k);

                if (counts[k * NumEntities + index] > 0)
                    sprintf(buffer, "%s - %s on texture %s: %d - x: %d  y: %d\n", ObjectiveStr[i], oc->Name, file, counts[k * NumEntities + index], locs[(k * NumEntities + index) * 2], locs[(k * NumEntities + index) * 2 + 1]);
                else if (counts[k * NumEntities + index] < 0 and index)
                    sprintf(buffer, "%s - %s on texture %s: %d - NEEDED x: %d  y: %d\n", ObjectiveStr[i], oc->Name, file, -counts[k * NumEntities + index], locs[(k * NumEntities + index) * 2], locs[(k * NumEntities + index) * 2 + 1]); //DSP

                fwrite(buffer, strlen(buffer), 1, fp);
            }

            j++;
        }
    }

    delete [] locs ;
    CloseCampFile(fp);
    CampLeaveCriticalSection();
}

void RecalculateBrigadePositions(void)
{
    Unit unit;
    Unit battalion;
    GridIndex   x, y, SumX, SumY;
    int count;

    VuListIterator myit(AllParentList);
    unit = (Unit) myit.GetFirst();

    while (unit not_eq NULL)
    {
        SumX = SumY = count = 0;

        if (unit->GetType() == TYPE_BRIGADE)
        {
            battalion = unit->GetFirstUnitElement();

            while (battalion)
            {
                battalion->GetLocation(&x, &y);
                SumX += x;
                SumY += y;
                count++;
                battalion = unit->GetNextUnitElement();
            }

            if (count)
            {
                x = SumX / count;
                y = SumY / count;
                unit->SetLocation(x, y);
            }
        }

        unit = (Unit) myit.GetNext();
    }

}

void MoveUnits(void)
{
    Unit unit;
    int x, y, deltaX, deltaY;
    int initX, endX, initY, endY;

    deltaX = CurX - Sel1X;
    deltaY = CurY - Sel1Y;

    if (Sel1X < Sel2X)
    {
        initX = Sel1X;
        endX = Sel2X;
    }
    else
    {
        initX = Sel2X;
        endX = Sel1X;
    }

    if (Sel1Y < Sel2Y)
    {
        initY = Sel1Y;
        endY = Sel2Y;
    }
    else
    {
        initY = Sel2Y;
        endY = Sel1Y;
    }

    for (x = initX; x <= endX; x++)
    {
        for (y = initY; y <= endY; y++)
        {
            if (ShowReal == 2)
            {
                do
                {
                    unit = FindUnitByXY(InactiveList, x, y, 0);

                    if (unit)
                    {
                        unit->SetLocation(x + deltaX, y + deltaY);
                    }
                }
                while (unit);
            }
            else
            {
                do
                {
                    unit = FindUnitByXY(AllParentList, x, y, 0);

                    if (unit)
                    {
                        unit->SetLocation(x + deltaX, y + deltaY);
                    }
                }
                while (unit);

                do
                {
                    unit = FindUnitByXY(AllRealList, x, y, 0);

                    if (unit)
                    {
                        unit->SetLocation(x + deltaX, y + deltaY);
                    }
                }
                while (unit);
            }
        }
    }

    SetRefresh(MainMapData);
    ptSelected = 0;
}

void RegroupBrigades(void)
{
    Unit unit;
    Unit battalion;
    int x, y;
    int initX, endX, initY, endY;


    if (Sel1X < Sel2X)
    {
        initX = Sel1X;
        endX = Sel2X;
    }
    else
    {
        initX = Sel2X;
        endX = Sel1X;
    }

    if (Sel1Y < Sel2Y)
    {
        initY = Sel1Y;
        endY = Sel2Y;
    }
    else
    {
        initY = Sel2Y;
        endY = Sel1Y;
    }

    for (x = initX; x <= endX; x++)
    {
        for (y = initY; y <= endY; y++)
        {
            unit = FindUnitByXY(AllParentList, x, y, 0);

            if (unit)
            {
                if (unit->GetType() == TYPE_BRIGADE)
                {
                    battalion = unit->GetFirstUnitElement();

                    while (battalion)
                    {
                        battalion->SetLocation(x , y);
                        battalion = unit->GetNextUnitElement();
                    }
                }
            }
        }
    }

    SetRefresh(MainMapData);
    ptSelected = 0;
}

void AssignBattToBrigDiv(void)
{
    Unit unit;
    Unit battalion;

    VuListIterator myit(AllParentList);
    unit = (Unit) myit.GetFirst();

    while (unit not_eq NULL)
    {
        if (unit->GetType() == TYPE_BRIGADE)
        {
            battalion = unit->GetFirstUnitElement();

            while (battalion)
            {
                battalion->SetUnitDivision(((GroundUnitClass *)unit)->GetUnitDivision());
                battalion = unit->GetNextUnitElement();
            }
        }

        unit = (Unit) myit.GetNext();
    }

}

void DeleteUnit(Unit unit)
{
    if ( not unit)
        return;

    Unit E;

    CampEnterCriticalSection();
    E = unit->GetFirstUnitElement();

    while (E)
    {
        unit->RemoveChild(E->Id());
        vuDatabase->Remove(E);
        E = unit->GetFirstUnitElement();
    }

    // Kill the unit
    unit->KillUnit();
    vuDatabase->Remove(unit);
    // Remove parent, if we're the last element
    E = unit->GetUnitParent();

    if (E and not E->GetFirstUnitElement())
        vuDatabase->Remove(E);

    GlobUnit = NULL;
    asAgg = 1;
    CampLeaveCriticalSection();
}

void StartUnitEdit(void)
{
    GridIndex   X, Y;
    X = (GridIndex) CurX;
    Y = (GridIndex) CurY;

    TheCampaign.Suspend();

    if (ShowReal == 1)
        OneUnit = FindUnitByXY(AllRealList, X, Y, 0);
    else if (ShowReal == 0)
        OneUnit = FindUnitByXY(AllParentList, X, Y, 0);
    else if (ShowReal == 2)
        OneUnit = FindUnitByXY(InactiveList, X, Y, 0);
    else
        OneUnit = NULL; // Can't edit divisions

    if (OneUnit)
        GlobUnit = OneUnit;
    else
        GlobUnit = NULL;

    if (GlobUnit)
        DialogBox(hInst, MAKEINTRESOURCE(IDD_UNITDIALOG1), hMainWnd, (DLGPROC)EditUnit);

    if (MainMapData->ShowWPs)
        SetRefresh(MainMapData);
    else
    {
        InvalidateRect(MainMapData->hMapWnd, NULL, FALSE);
        PostMessage(MainMapData->hMapWnd, WM_PAINT, (WPARAM)hMainDC, 0);
    }

    TheCampaign.Resume();
}

void StartObjectiveEdit(void)
{
    GridIndex   X, Y;
    X = (GridIndex) CurX;
    Y = (GridIndex) CurY;

    CampEnterCriticalSection();
    OneObjective = GetObjectiveByXY(X, Y);

    if ( not OneObjective)
    {
        OneObjective = AddObjectiveToCampaign(X, Y);
    }

    GlobObj = OneObjective;
    CampLeaveCriticalSection();
    DialogBox(hInst, MAKEINTRESOURCE(IDD_OBJECTIVEDIALOG), hMainWnd, (DLGPROC)EditObjective);
    MainMapData->ShowObjectives = TRUE;
    InvalidateRect(MainMapData->hMapWnd, NULL, FALSE);
    PostMessage(MainMapData->hMapWnd, WM_PAINT, (WPARAM)hMainDC, 0);
}

void DamageBattalions(void)
{
    Unit unit;
    int x, y;
    int initX, endX, initY, endY;


    if (Sel1X < Sel2X)
    {
        initX = Sel1X;
        endX = Sel2X;
    }
    else
    {
        initX = Sel2X;
        endX = Sel1X;
    }

    if (Sel1Y < Sel2Y)
    {
        initY = Sel1Y;
        endY = Sel2Y;
    }
    else
    {
        initY = Sel2Y;
        endY = Sel1Y;
    }

    for (x = initX; x <= endX; x++)
    {
        for (y = initY; y <= endY; y++)
        {
            unit = FindUnitByXY(AllRealList, x, y, 0);

            if (unit)
            {
                if (unit->GetType() == TYPE_BATTALION)
                {

                }
            }
        }
    }

    SetRefresh(MainMapData);
    ptSelected = 0;
}


void SetObjOwnersArea(int team)
{
    Objective obj;
    int x, y;
    int initX, endX, initY, endY;

    if (Sel1X < Sel2X)
    {
        initX = Sel1X;
        endX = Sel2X;
    }
    else
    {
        initX = Sel2X;
        endX = Sel1X;
    }

    if (Sel1Y < Sel2Y)
    {
        initY = Sel1Y;
        endY = Sel2Y;
    }
    else
    {
        initY = Sel2Y;
        endY = Sel1Y;
    }

    for (x = initX; x <= endX; x++)
    {
        for (y = initY; y <= endY; y++)
        {
            obj = GetObjectiveByXY(x, y);

            if (obj)
            {
                obj->SetOwner(team);
            }
        }
    }

    SetRefresh(MainMapData);
    ptSelected = 0;
}

// ===================================
// Timing functions
// ===================================

/*
   ulong Camp_VU_clock(void)
      {
      return (ulong)clock*1000/CLK_TCK;
      }

   ulong Camp_VU_game_time(void)
      {
      return (ulong)(Camp_GetCurrentTime()*1000);
      }
*/

// ====================================
// Map display functions
// ====================================

void SetRefresh(MapData md)
{
    RECT r;

    if ( not md or not md->hMapWnd)
        return;

    RefreshAll = TRUE;
    ReBlt = TRUE;
    GetClientRect(md->hMapWnd, &r);
    InvalidateRect(md->hMapWnd, &r, FALSE);
    PostMessage(md->hMapWnd, WM_PAINT, 0, 0);
    SuspendDraw = TRUE;
}

void RefreshCampMap(void)
{
    SetRefresh(MainMapData);
}

void RefreshMap(MapData md, HDC DC, RECT *rect)
{
    GridIndex x, y, NFX = 0, NFY = 0, NLX = 0, NLY = 0;
    int clipx, clipy, ysize;
    RECT r;
    GridIndex xx, yy;
    int side = 0;
    WayPoint w;

    // if (SuspendDraw)
    CampEnterCriticalSection();

    if (RefreshAll)
    {
        SetCursor(hCurWait);
        NFX = md->FX;
        NFY = md->FY;
        NLX = md->LX;
        NLY = md->LY;
        RefreshAll = FALSE;
    }
    else if (FreshDraw)
    {
        NFX = (GridIndex)(rect->left / CellSize) + md->FX;
        NFY = (GridIndex)(rect->top / CellSize) + md->FY;
        NLX = (GridIndex)(rect->right / CellSize) + md->FX + 1;
        NLY = (GridIndex)(rect->bottom / CellSize) + md->FY + 1;
        FreshDraw = FALSE;
    }
    else
    {
        SetCursor(hCurWait);

        if (PBubble)
            ShowPlayerBubble(md, DC, 0); // Undraw the player bubble

        // Update what we can with our bmap
        NFX = md->FX;
        NFY = md->FY;
        NLX = md->FX;
        NLY = md->FY;
        GetClientRect(md->hMapWnd, &r);
        ysize = r.bottom;

        if (md->PFX < md->PMFX)
        {
            NFX = md->FX;
            NLX = (md->PMFX / md->CellSize) + 1;
            NLY = md->LY;
            r.left = md->PMFX - md->PFX;
            side or_eq 1;
        }

        if (md->PFY < md->PMFY)
        {
            NFY = md->FY;
            NLY = (md->PMFY / md->CellSize) + 1;
            NLX = md->LX;
            r.bottom = ysize - (md->PMFY - md->PFY);
            side or_eq 2;
        }

        if (md->PLX > md->PMLX)
        {
            NLX = md->LX;
            NFX = md->PMLX / md->CellSize;
            NLY = md->LY;
            r.right = md->PMLX - md->PFX;
            side or_eq 4;
        }

        if (md->PLY > md->PMLY)
        {
            NLY = md->LY;
            NFY = md->PMLY / md->CellSize;
            NLX = md->LX;
            r.top = ysize - (md->PMLY - md->PFY);
            side or_eq 8;
        }

        if ((side bitand 0x5) == 0x5)
        {
            NFX = md->FX;
            NLX = md->LX;
        }

        if ((side bitand 0xa) == 0xa)
        {
            NFY = md->FY;
            NLY = md->LY;
        }

        if ((side bitand 0xC) == 0xC or (side bitand 0x3) == 0x3)
        {
            NFX = md->FX;
            NFY = md->FY;
            NLX = md->LX;
            NLY = md->LY;
        }

        clipx = md->PFX - md->PMFX;
        clipy = md->PMLY - md->PLY;

        if (clipx < 0)
            clipx = 0;

        if (clipy < 0)
            clipy = 0;

        // Blit from storage to the screen
        BitBlt(DC, r.left, r.top, r.right - r.left, r.bottom - r.top, md->hMapDC,
               clipx, clipy, SRCCOPY);

        if (NFX < md->FX)
            NFX = md->FX;

        if (NFY < md->FY)
            NFY = md->FY;

        if (NLX > md->LX)
            NLX = md->LX;

        if (NLY > md->LY)
            NLY = md->LY;
    }

    // Now draw the rest.
    for (y = NLY - 1; y >= NFY; y--)
        for (x = NFX; x < NLX; x++)
            DisplayCellData(DC, x, y, POSX(x), POSY(y), md->CellSize, Mode, RoadsOn, RailsOn);

    PBubbleDrawn = FALSE;

    // Reblt before drawing Objectives or Units - do that each time.
    if (ReBlt)
        ReBltWnd(md, DC);

    md->PMFX = md->PFX;
    md->PMLX = md->PLX;
    md->PMFY = md->PFY;
    md->PMLY = md->PLY;

    if (md->ShowObjectives)
    {
#ifdef VU_GRID_TREE_Y_MAJOR
        VuGridIterator oit(ObjProxList, (BIG_SCALAR)GridToSim(md->CenX), (BIG_SCALAR)GridToSim(md->CenY), (BIG_SCALAR)GridToSim((short)(Distance(md->LX, md->LY, md->CenX, md->CenY))));
#else
        VuGridIterator oit(ObjProxList, (BIG_SCALAR)GridToSim(md->CenY), (BIG_SCALAR)GridToSim(md->CenX), (BIG_SCALAR)GridToSim((short)(Distance(md->LX, md->LY, md->CenX, md->CenY))));
#endif
        int i;

        OneObjective = GetFirstObjective(&oit);

        while (OneObjective not_eq NULL)
        {
            OneObjective->GetLocation(&x, &y);

            if (x > md->LX or x < md->FX or y < md->FY or y > md->LY)
                ; // Out of window bounds
            else
            {
                if (md->ShowLinks)
                {
                    _setcolor(DC, White);

                    for (i = 0; i < OneObjective->NumLinks(); i++)
                    {
                        Neighbor = OneObjective->GetNeighbor(i);

                        if (Neighbor)
                        {
                            Neighbor->GetLocation(&xx, &yy);
                            _moveto(DC, POSX(x) + (md->CellSize >> 1), POSY(y) + (md->CellSize >> 1));
                            _lineto(DC, POSX(xx) + (md->CellSize >> 1), POSY(yy) + (md->CellSize >> 1));
                        }
                    }
                }

                DisplayObjective(DC, OneObjective, POSX(x), POSY(y), md->CellSize);
            }

            OneObjective = GetNextObjective(&oit);
        }
    }

    if (md->ShowUnits)
    {
        VuListIterator *uit = NULL;
        short scale = 1, xd = 0, yd = 0;

        if (ShowReal == 1)
            uit = new VuListIterator(AllRealList);
        else if ( not ShowReal)
        {
            uit = new VuListIterator(AllParentList);
            scale = 3;
            xd = -1;
            yd = 1;
        }
        else if (ShowReal == 2)
            uit = new VuListIterator(InactiveList);
        else
            ShowDivisions(DC, md);

        if (uit)
        {
            OneUnit = GetFirstUnit(uit);

            while (OneUnit not_eq NULL)
            {
                OneUnit->GetLocation(&x, &y);
                DisplayUnit(DC, OneUnit, (short)(POSX(x + xd) + (md->CellSize >> 3)*scale), (short)(POSY(y + yd) + (md->CellSize >> 2)*scale), (short)((md->CellSize >> 1)*scale));

                if (ShowPaths and OneUnit->IsBattalion())
                {
                    if (OneUnit->GetType() == TYPE_BATTALION)
                    {
                        // sfr: this is pointer now... took out &
                        //ShowPath(md, x, y, &((Battalion)OneUnit)->path, White);
                        //ShowPathHistory(md, x, y, &((Battalion)OneUnit)->path, Red);
                        ShowPath(md, x, y, ((Battalion)OneUnit)->path, White);
                        ShowPathHistory(md, x, y, ((Battalion)OneUnit)->path, Red);
                    }
                }

#ifdef USE_FLANKS

                if (ShowFlanks and OneUnit->GetDomain() == DOMAIN_LAND)
                {
                    GridIndex fx, fy;
                    OneUnit->GetLeftFlank(&fx, &fy);
                    _setcolor(DC, SideColor[OneUnit->GetOwner()]);
                    _moveto(DC, POSX(fx) + (md->CellSize >> 1), POSY(fy) + (md->CellSize >> 1));
                    _lineto(DC, POSX(x) + (md->CellSize >> 1), POSY(y) + (md->CellSize >> 1));
                    OneUnit->GetRightFlank(&fx, &fy);
                    _lineto(DC, POSX(fx) + (md->CellSize >> 1), POSY(fy) + (md->CellSize >> 1));
                }

#endif
                OneUnit = GetNextUnit(uit);
            }

            delete uit;
        }
    }

    if (md->ShowWPs)
    {
        if ( not WPUnit or WPUnit->IsDead())
            WPUnit = NULL;
        else
        {
            WPUnit->GetLocation(&x, &y);
            w = WPUnit->GetCurrentUnitWP();

            while (w)
            {
                w->GetWPLocation(&xx, &yy);
                ShowWPLeg(MainMapData, x, y, xx, yy, White);
                w = w->GetNextWP();
                x = xx;
                y = yy;
            }
        }
    }

    if (md->Emitters)
    {
        ShowEmitters(md, DC);
    }

    if (md->SAMs)
    {
        ShowSAMs(md, DC);
    }

    if (PBubble)
        ShowPlayerBubble(md, DC, 1);

    SetCursor(hCur);
    CampLeaveCriticalSection();
}

void zoomOut(MapData md)
{
    md->CellSize /= 2;

    if (md->CellSize < 1)
        md->CellSize = 1;

    CellSize = md->CellSize;
    FindBorders(md);
    MaxXSize = MAX_XPIX / md->CellSize;
    MaxYSize = MAX_YPIX / md->CellSize;
    SetRefresh(md);
    ResizeCursor();
    // nPos = (Map_Max_Y-md->CenY)*100/Map_Max_Y;
    SetScrollPos(md->hMapWnd, SB_VERT, CenPer(MainMapData, CenY, YSIDE), TRUE);
    // nPos = md->CenX*100/Map_Max_X;
    SetScrollPos(md->hMapWnd, SB_HORZ, CenPer(MainMapData, CenX, XSIDE), TRUE);
}

void zoomIn(MapData md)
{
    md->CellSize *= 2;

    if (md->CellSize > 16)
        md->CellSize = 16;

    CellSize = md->CellSize;
    FindBorders(md);
    MaxXSize = MAX_XPIX / md->CellSize;
    MaxYSize = MAX_YPIX / md->CellSize;
    SetRefresh(md);
    ResizeCursor();
    // nPos = (Map_Max_Y-md->CenY)*100/Map_Max_Y;
    SetScrollPos(md->hMapWnd, SB_VERT, CenPer(MainMapData, CenY, YSIDE), TRUE);
    // nPos = md->CenX*100/Map_Max_X;
    SetScrollPos(md->hMapWnd, SB_HORZ, CenPer(MainMapData, CenX, XSIDE), TRUE);
}

// ============================================
// Tool window procedure
// ============================================

LRESULT CALLBACK ToolWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT  retval = 0;

    switch (message)
    {
        case WM_PAINT:
        {
            char        time[80], buffer[60];
            RECT  r;
            PAINTSTRUCT ps;
            HDC DC;

            if (GetUpdateRect(hToolWnd, &r, FALSE))
            {
                DC = BeginPaint(hToolWnd, &ps);
                /* sprintf(time,"Day %2.2d, %2.2d:%2.2d x%d      ",CampDay, CampHour, CampMinute, gameCompressionRatio);*/
                _moveto(DC, 1, 0);
                _outgtext(DC, time);
                // Theater Name
                _moveto(DC, 190, 0);
                _outgtext(DC, TheCampaign.TheaterName);
                // Position
                sprintf(time, "%d,%d          ", CurX, CurY);
                _moveto(DC, 1, 18);
                _outgtext(DC, time);

                // Texture file
                if (ShowCodes)
                {
                    char *file;

                    file = GetFilename(CurX, CurY);
                    sprintf(buffer, "%s      ", file);
                    _moveto(DC, 190, 18);
                    _outgtext(DC, buffer);
                }

                if (OneObjective)
                    sprintf(time, "%s               ", OneObjective->GetName(buffer, 60, FALSE));
                else
                    sprintf(time, "                                                  ");

                _moveto(DC, 1, 36);
                _outgtext(DC, time);

                if (OneUnit)
                    if (OneUnit->GetType() == TYPE_BATTALION and OneUnit->GetDomain() == DOMAIN_LAND)
                    {
                        if (((GroundUnitClass *)OneUnit)->GetDivision())
                        {
                            Unit unit;
                            unit = OneUnit->GetUnitParent();

                            if (unit)
                                sprintf(time, "%d-%d-%s               ", ((GroundUnitClass *)OneUnit)->GetDivision(), unit->GetNameId(), OneUnit->GetName(buffer, 60, FALSE));
                            else
                                sprintf(time, "%d-%s               ", ((GroundUnitClass *)OneUnit)->GetDivision(), OneUnit->GetName(buffer, 60, FALSE));

                        }
                        else
                        {
                            sprintf(time, "%s               ", OneUnit->GetName(buffer, 60, FALSE));
                        }
                    }
                    else if (OneUnit->GetType() == TYPE_BRIGADE and OneUnit->GetDomain() == DOMAIN_LAND)
                    {
                        if (((GroundUnitClass *)OneUnit)->GetDivision())
                        {
                            sprintf(time, "%d-%s               ", ((GroundUnitClass *)OneUnit)->GetDivision(), OneUnit->GetName(buffer, 60, FALSE));
                        }
                        else
                        {
                            sprintf(time, "%s               ", OneUnit->GetName(buffer, 60, FALSE));
                        }
                    }
                    else
                    {
                        sprintf(time, "%s               ", OneUnit->GetName(buffer, 60, FALSE));

                    }
                else
                    sprintf(time, "                                                  ");

                _moveto(DC, 1, 54);
                _outgtext(DC, time);

                if (Linking)
                    sprintf(time, "Linking..");
                else if (LinkTool and FromObjective)
                    sprintf(time, "Linking #%d    ", FromObjective->GetCampID());
                else
                    sprintf(time, "                                 ");

                _moveto(DC, 1, 72);
                _outgtext(DC, time);
                EndPaint(hToolWnd, &ps);
            }

            retval = 0;
            break;
        }

        case WM_DESTROY:
            hToolWnd = NULL;
            retval = DefWindowProc(hwnd, message, wParam, lParam);
            break;

        default:
            retval = DefWindowProc(hwnd, message, wParam, lParam);
            break;
    }

    return retval;
}

// ================================================
// Main Campaign Window procedure
// ================================================

BOOL MainWndCommandProc(HWND hWndFrame, WPARAM wParam, LONG lParam)
{
    lParam = lParam;

    switch (wParam)
    {
        case ID_FILE_NEWTHEATER:
            InitTheaterTerrain();
            ((WeatherClass*)realWeather)->Init();
            // WARNING: Things could get fucked if we changed the theater size
            // DisposeProxLists();
            // InitProximityLists();
            // WARNING: entities will not be reinserted into prox lists
            SetRefresh(MainMapData);
            break;

        case ID_FILE_OPENTHEATER:
            OpenTheaterFile(hMainWnd);
            ((WeatherClass*)realWeather)->Init();
            // WARNING: Things could get fucked if we changed the theater size
            // DisposeProxLists();
            // InitProximityLists();
            // WARNING: entities will not be reinserted into prox lists
            SetRefresh(MainMapData);
            break;

        case ID_FILE_SAVETHEATER:
            // SaveTheaterFile(hMainWnd);
            break;

        case ID_FILE_SAVEASTHEATER:
            SaveAsTheaterFile(hMainWnd);
            break;

        case ID_FILE_EXIT:
            PostQuitMessage(0);
            break;

        case ID_EDIT_COVER:
            EditMode = 0;
            break;

        case ID_EDIT_RELIEF:
            EditMode = 2;
            break;

        case ID_EDIT_ROADS:
            EditMode = 3;
            DrawRoad = 1;
            break;

        case ID_EDIT_RAILS:
            EditMode = 4;
            DrawRail = 1;
            break;

        case ID_EDIT_WEATHER:
            EditMode = 5;
            DrawWeather = 1;
            break;

        case ID_EDIT_OBJECTIVES:
            EditMode = 7;
            MainMapData->ShowObjectives = 1;
            break;

        case ID_EDIT_UNITS:
            EditMode = 8;
            MainMapData->ShowUnits = 1;

        case ID_EDIT_WATER:
            DrawCover = Water;
            break;

        case ID_EDIT_BOG:
            DrawCover = Bog;
            break;

        case ID_EDIT_BARREN:
            DrawCover = Barren;
            break;

        case ID_EDIT_PLAINS:
            DrawCover = Plain;
            break;

        case ID_EDIT_BRUSH:
            DrawCover = Brush;
            break;

        case ID_EDIT_LFOREST:
            DrawCover = LightForest;
            break;

        case ID_EDIT_HFOREST:
            DrawCover = HeavyForest;
            break;

        case ID_EDIT_SURBAN:
            DrawCover = Urban;
            break;

        case ID_EDIT_URBAN:
            DrawCover = Urban;
            break;

        case ID_EDIT_FLAT:
            DrawRelief = Flat;
            break;

        case ID_EDIT_ROUGH:
            DrawRelief = Rough;
            break;

        case ID_EDIT_HILLY:
            DrawRelief = Hills;
            break;

        case ID_EDIT_MOUNTAIN:
            DrawRelief = Mountains;
            break;

        case ID_EDIT_CLEARCOV:
            DrawWeather = 0;
            break;

        case ID_EDIT_LOWFOG:
            DrawWeather = 1;
            break;

        case ID_EDIT_SCATTERED:
            DrawWeather = 2;
            break;

        case ID_EDIT_BROKEN:
            DrawWeather = 3;
            break;

        case ID_EDIT_OVERCAST:
            DrawWeather = 4;
            break;

        case ID_EDIT_RAIN:
            DrawWeather = 5;
            break;

        case ID_CAMPAIGN_NEW:
            TheCampaign.Reset();
            SetRefresh(MainMapData);
            break;

        case ID_CAMPAIGN_MAKENEW:
            MakeCampaignNew();
            break;

        case ID_VIEW_ZOOMIN:
            zoomIn(MainMapData);
            break;

        case ID_VIEW_ZOOMOUT:
            zoomOut(MainMapData);
            break;

        case ID_VIEW_BOTH:
            Mode = EditMode = 0;
            CheckMenuItem(GetMenu(hMainWnd), ID_VIEW_BOTH, MF_BYCOMMAND bitor MF_CHECKED);
            CheckMenuItem(GetMenu(hMainWnd), ID_VIEW_COVER, MF_BYCOMMAND bitor MF_UNCHECKED);
            CheckMenuItem(GetMenu(hMainWnd), ID_VIEW_RELIEF, MF_BYCOMMAND bitor MF_UNCHECKED);
            CheckMenuItem(GetMenu(hMainWnd), ID_VIEW_CLOUDS, MF_BYCOMMAND bitor MF_UNCHECKED);
            CheckMenuItem(GetMenu(hMainWnd), ID_VIEW_LEVELS, MF_BYCOMMAND bitor MF_UNCHECKED);
            SetRefresh(MainMapData);
            break;

        case ID_VIEW_COVER:
            Mode = EditMode = 1;
            CheckMenuItem(GetMenu(hMainWnd), ID_VIEW_BOTH, MF_BYCOMMAND bitor MF_UNCHECKED);
            CheckMenuItem(GetMenu(hMainWnd), ID_VIEW_COVER, MF_BYCOMMAND bitor MF_CHECKED);
            CheckMenuItem(GetMenu(hMainWnd), ID_VIEW_RELIEF, MF_BYCOMMAND bitor MF_UNCHECKED);
            CheckMenuItem(GetMenu(hMainWnd), ID_VIEW_CLOUDS, MF_BYCOMMAND bitor MF_UNCHECKED);
            CheckMenuItem(GetMenu(hMainWnd), ID_VIEW_LEVELS, MF_BYCOMMAND bitor MF_UNCHECKED);
            SetRefresh(MainMapData);
            break;

        case ID_VIEW_RELIEF:
            Mode = EditMode = 2;
            CheckMenuItem(GetMenu(hMainWnd), ID_VIEW_BOTH, MF_BYCOMMAND bitor MF_UNCHECKED);
            CheckMenuItem(GetMenu(hMainWnd), ID_VIEW_COVER, MF_BYCOMMAND bitor MF_UNCHECKED);
            CheckMenuItem(GetMenu(hMainWnd), ID_VIEW_RELIEF, MF_BYCOMMAND bitor MF_CHECKED);
            CheckMenuItem(GetMenu(hMainWnd), ID_VIEW_CLOUDS, MF_BYCOMMAND bitor MF_UNCHECKED);
            CheckMenuItem(GetMenu(hMainWnd), ID_VIEW_LEVELS, MF_BYCOMMAND bitor MF_UNCHECKED);
            SetRefresh(MainMapData);
            break;

        case ID_VIEW_CLOUDS:
            Mode = EditMode = 5;
            CheckMenuItem(GetMenu(hMainWnd), ID_VIEW_BOTH, MF_BYCOMMAND bitor MF_UNCHECKED);
            CheckMenuItem(GetMenu(hMainWnd), ID_VIEW_COVER, MF_BYCOMMAND bitor MF_UNCHECKED);
            CheckMenuItem(GetMenu(hMainWnd), ID_VIEW_RELIEF, MF_BYCOMMAND bitor MF_UNCHECKED);
            CheckMenuItem(GetMenu(hMainWnd), ID_VIEW_CLOUDS, MF_BYCOMMAND bitor MF_CHECKED);
            CheckMenuItem(GetMenu(hMainWnd), ID_VIEW_LEVELS, MF_BYCOMMAND bitor MF_UNCHECKED);
            SetRefresh(MainMapData);
            break;

        case ID_VIEW_LEVELS:
            Mode = EditMode = 6;
            CheckMenuItem(GetMenu(hMainWnd), ID_VIEW_BOTH, MF_BYCOMMAND bitor MF_UNCHECKED);
            CheckMenuItem(GetMenu(hMainWnd), ID_VIEW_COVER, MF_BYCOMMAND bitor MF_UNCHECKED);
            CheckMenuItem(GetMenu(hMainWnd), ID_VIEW_RELIEF, MF_BYCOMMAND bitor MF_UNCHECKED);
            CheckMenuItem(GetMenu(hMainWnd), ID_VIEW_CLOUDS, MF_BYCOMMAND bitor MF_UNCHECKED);
            CheckMenuItem(GetMenu(hMainWnd), ID_VIEW_LEVELS, MF_BYCOMMAND bitor MF_CHECKED);
            SetRefresh(MainMapData);

        case ID_VIEW_ROADS:
            RoadsOn = not RoadsOn;

            if (RoadsOn)
                CheckMenuItem(GetMenu(hMainWnd), ID_VIEW_ROADS, MF_BYCOMMAND bitor MF_CHECKED);
            else
                CheckMenuItem(GetMenu(hMainWnd), ID_VIEW_ROADS, MF_BYCOMMAND bitor MF_UNCHECKED);

            SetRefresh(MainMapData);
            break;

        case ID_VIEW_RAILS:
            RailsOn = not RailsOn;

            if (RailsOn)
                CheckMenuItem(GetMenu(hMainWnd), ID_VIEW_RAILS, MF_BYCOMMAND bitor MF_CHECKED);
            else
                CheckMenuItem(GetMenu(hMainWnd), ID_VIEW_RAILS, MF_BYCOMMAND bitor MF_UNCHECKED);

            SetRefresh(MainMapData);
            break;

        case ID_VIEW_EMITTERS:
            // MainMapData->Emitters = not MainMapData->Emitters;
            Mode = 11;

            if (MainMapData->Emitters)
                CheckMenuItem(GetMenu(hMainWnd), ID_VIEW_EMITTERS, MF_BYCOMMAND bitor MF_CHECKED);
            else
                CheckMenuItem(GetMenu(hMainWnd), ID_VIEW_EMITTERS, MF_BYCOMMAND bitor MF_UNCHECKED);

            SetRefresh(MainMapData);
            break;

        case ID_VIEW_SAMS:
            // MainMapData->SAMs = not MainMapData->SAMs;
            Mode = 10;

            if (MainMapData->SAMs)
                CheckMenuItem(GetMenu(hMainWnd), ID_VIEW_SAMS, MF_BYCOMMAND bitor MF_CHECKED);
            else
                CheckMenuItem(GetMenu(hMainWnd), ID_VIEW_SAMS, MF_BYCOMMAND bitor MF_UNCHECKED);

            SetRefresh(MainMapData);
            break;

        case ID_VIEW_PLAYERBUBBLE:
            PBubble = not PBubble;

            if (PBubble)
                CheckMenuItem(GetMenu(hMainWnd), ID_VIEW_PLAYERBUBBLE, MF_BYCOMMAND bitor MF_CHECKED);
            else
                CheckMenuItem(GetMenu(hMainWnd), ID_VIEW_PLAYERBUBBLE, MF_BYCOMMAND bitor MF_UNCHECKED);

            SetRefresh(MainMapData);
            break;

        case ID_VIEW_OBJPRIORITY:
            ObjMode = 1;
            CheckMenuItem(GetMenu(hMainWnd), ID_VIEW_OBJOWNER, MF_BYCOMMAND bitor MF_UNCHECKED);
            CheckMenuItem(GetMenu(hMainWnd), ID_VIEW_OBJPRIORITY, MF_BYCOMMAND bitor MF_CHECKED);
            CheckMenuItem(GetMenu(hMainWnd), ID_VIEW_OBJTYPE, MF_BYCOMMAND bitor MF_UNCHECKED);
            SetRefresh(MainMapData);
            break;

        case ID_VIEW_OBJOWNER:
            ObjMode = 0;
            CheckMenuItem(GetMenu(hMainWnd), ID_VIEW_OBJOWNER, MF_BYCOMMAND bitor MF_CHECKED);
            CheckMenuItem(GetMenu(hMainWnd), ID_VIEW_OBJPRIORITY, MF_BYCOMMAND bitor MF_UNCHECKED);
            CheckMenuItem(GetMenu(hMainWnd), ID_VIEW_OBJTYPE, MF_BYCOMMAND bitor MF_UNCHECKED);
            SetRefresh(MainMapData);
            break;

        case ID_VIEW_OBJTYPE:
            ObjMode = 2;
            CheckMenuItem(GetMenu(hMainWnd), ID_VIEW_OBJOWNER, MF_BYCOMMAND bitor MF_UNCHECKED);
            CheckMenuItem(GetMenu(hMainWnd), ID_VIEW_OBJPRIORITY, MF_BYCOMMAND bitor MF_UNCHECKED);
            CheckMenuItem(GetMenu(hMainWnd), ID_VIEW_OBJTYPE, MF_BYCOMMAND bitor MF_CHECKED);
            SetRefresh(MainMapData);
            break;

        case ID_VIEW_REALUNITS:
            ShowReal = 1;
            CheckMenuItem(GetMenu(hMainWnd), ID_VIEW_REALUNITS, MF_BYCOMMAND bitor MF_CHECKED);
            CheckMenuItem(GetMenu(hMainWnd), ID_VIEW_PARENTUNITS, MF_BYCOMMAND bitor MF_UNCHECKED);
            CheckMenuItem(GetMenu(hMainWnd), ID_VIEW_REINFORCEMENTS, MF_BYCOMMAND bitor MF_UNCHECKED);
            CheckMenuItem(GetMenu(hMainWnd), ID_VIEW_DIVISIONS, MF_BYCOMMAND bitor MF_UNCHECKED);
            SetRefresh(MainMapData);
            break;

        case ID_VIEW_PARENTUNITS:
            ShowReal = 0;
            CheckMenuItem(GetMenu(hMainWnd), ID_VIEW_REALUNITS, MF_BYCOMMAND bitor MF_UNCHECKED);
            CheckMenuItem(GetMenu(hMainWnd), ID_VIEW_PARENTUNITS, MF_BYCOMMAND bitor MF_CHECKED);
            CheckMenuItem(GetMenu(hMainWnd), ID_VIEW_REINFORCEMENTS, MF_BYCOMMAND bitor MF_UNCHECKED);
            CheckMenuItem(GetMenu(hMainWnd), ID_VIEW_DIVISIONS, MF_BYCOMMAND bitor MF_UNCHECKED);
            SetRefresh(MainMapData);
            break;

        case ID_VIEW_DIVISIONS:
            ShowReal = 3;
            CheckMenuItem(GetMenu(hMainWnd), ID_VIEW_REALUNITS, MF_BYCOMMAND bitor MF_UNCHECKED);
            CheckMenuItem(GetMenu(hMainWnd), ID_VIEW_PARENTUNITS, MF_BYCOMMAND bitor MF_UNCHECKED);
            CheckMenuItem(GetMenu(hMainWnd), ID_VIEW_REINFORCEMENTS, MF_BYCOMMAND bitor MF_UNCHECKED);
            CheckMenuItem(GetMenu(hMainWnd), ID_VIEW_DIVISIONS, MF_BYCOMMAND bitor MF_CHECKED);
            SetRefresh(MainMapData);
            break;

        case ID_VIEW_REINFORCEMENTS:
            ShowReal = 2;
            CheckMenuItem(GetMenu(hMainWnd), ID_VIEW_REALUNITS, MF_BYCOMMAND bitor MF_UNCHECKED);
            CheckMenuItem(GetMenu(hMainWnd), ID_VIEW_PARENTUNITS, MF_BYCOMMAND bitor MF_UNCHECKED);
            CheckMenuItem(GetMenu(hMainWnd), ID_VIEW_REINFORCEMENTS, MF_BYCOMMAND bitor MF_CHECKED);
            CheckMenuItem(GetMenu(hMainWnd), ID_VIEW_DIVISIONS, MF_BYCOMMAND bitor MF_UNCHECKED);
            SetRefresh(MainMapData);
            break;

        case ID_VIEW_TEXTURECODES:
            if (ShowCodes)
            {
                ShowCodes = 0;
                CleanupConverter();
                CheckMenuItem(GetMenu(hMainWnd), ID_VIEW_TEXTURECODES, MF_BYCOMMAND bitor MF_UNCHECKED);
            }
            else
            {
                ShowCodes = 1;
                InitConverter(TheCampaign.TheaterName);
                CheckMenuItem(GetMenu(hMainWnd), ID_VIEW_TEXTURECODES, MF_BYCOMMAND bitor MF_CHECKED);
            }

            break;

        case ID_VIEW_REFRESH:
            SetRefresh(MainMapData);
            break;

        case ID_TOOLS_RECALULATELINKS:
        {
            int i, done;
            PathClass path;
            MSG msg;
            GridIndex ox, oy;

            // KCK: First, I need to place roads to ports
            {
                VuListIterator oit(AllObjList);
                FromObjective = GetFirstObjective(&oit);

                while (FromObjective not_eq NULL)
                {
                    if (FromObjective->GetType() == TYPE_PORT)
                    {
                        FromObjective->GetLocation(&ox, &oy);
                        SetRoadCell(GetCell(ox, oy), 1);

                        // Attempt to find an adjacent land space
                        for (i = 0, done = 0; i < 8 and not done; i += 2)
                        {
                            if (GetCover(ox + dx[i], oy + dy[i]) not_eq Water)
                            {
                                SetRoadCell(GetCell(ox + dx[i], oy + dy[i]), 1);
                                done = 1;
                            }
                        }

                        // Attempt to find a diagonal if no luck
                        for (i = 1; i < 8 and not done; i += 2)
                        {
                            if (GetCover(ox + dx[i], oy + dy[i]) not_eq Water)
                            {
                                SetRoadCell(GetCell(ox + dx[i], oy + dy[i]), 1);
                                SetRoadCell(GetCell(ox + dx[i - 1], oy + dy[i - 1]), 1);
                                done = 1;
                            }
                        }
                    }

                    FromObjective = GetNextObjective(&oit);
                }

            }
            // Next, do the linking
            LinkTool = TRUE;
            // CampEnterCriticalSection();
            memset(CampSearch, 0, sizeof(uchar)*MAX_CAMP_ENTITIES);

            /* FromObjective = GetFirstObjective(&oit);*/
            while (FromObjective not_eq NULL)
            {
                CampSearch[FromObjective->GetCampID()] = 1;

                for (i = 0; i < FromObjective->NumLinks(); i++)
                {
                    ToObjective = FromObjective->GetNeighbor(i);

                    if (ToObjective and not CampSearch[ToObjective->GetCampID()])
                        LinkCampaignObjectives(&path, FromObjective, ToObjective);
                }

                /* FromObjective = GetNextObjective(&oit);*/
                // Still handle window messages while we loop
                if (GetMessage(&msg, NULL, 0, 0))
                    DispatchMessage(&msg);    // Dispatch message to window.
            }

            // CampLeaveCriticalSection();
            LinkTool = FALSE;
        }
        break;

        case ID_TOOLS_SETOBJTYPES:
            if ( not ShowCodes)
            {
                // Load the texture codes
                ShowCodes = 1;
                InitConverter(TheCampaign.TheaterName);
                CheckMenuItem(GetMenu(hMainWnd), ID_VIEW_TEXTURECODES, MF_BYCOMMAND bitor MF_CHECKED);
            }

            MatchObjectiveTypes();
            break;

        case ID_TOOLS_OUTPUTREINFORCEMENTS:
        {
            GridIndex x, y;
            FILE *fp;
            Unit unit;
            Unit brigade;
            Objective obj;
            char buffer[60], domain[8], type[12], div[5], brig[5], vehicle[20], objective[40];

            CampEnterCriticalSection();
            fp = OpenCampFile("Reinloc", "txt", "w");

            if (fp)
            {
                VuListIterator myit(InactiveList);
                unit = (Unit) myit.GetFirst();

                while (unit not_eq NULL)
                {
                    unit->GetLocation(&x, &y);
                    obj = GetObjectiveByXY(x, y);

                    if (obj)
                    {
                        sprintf(objective, "%s", obj->GetName(buffer, 60, FALSE));
                    }
                    else
                    {
                        strcpy(objective, "none");
                    }

                    if (unit->GetDomain() == DOMAIN_AIR)
                    {
                        strcpy(domain, "AIR");

                        if (unit->GetType() == TYPE_SQUADRON)
                        {
                            strcpy(div, "n/a");
                            strcpy(brig, "n/a");
                            strcpy(type, "SQUADRON");
                            strcpy(vehicle, GetVehicleName(unit->GetVehicleID(0)));
                        }
                    }
                    else if (unit->GetDomain() == DOMAIN_SEA)
                    {
                        strcpy(domain, "SEA");

                        if (unit->GetType() == TYPE_TASKFORCE)
                        {
                            strcpy(div, "n/a");
                            strcpy(brig, "n/a");
                            strcpy(type, "TASKFORCE");
                            strcpy(vehicle, GetVehicleName(unit->GetVehicleID(0)));
                        }
                    }
                    else if (unit->GetDomain() == DOMAIN_LAND)
                    {
                        strcpy(domain, "LAND");

                        if (unit->GetType() == TYPE_BRIGADE)
                        {
                            sprintf(div, "%d", ((GroundUnitClass *)unit)->GetDivision());
                            strcpy(brig, "n/a");
                            strcpy(type, "BRIGADE");
                            strcpy(vehicle, "n/a");
                        }
                        else if (unit->GetType() == TYPE_BATTALION)
                        {
                            brigade = unit->GetUnitParent();

                            if (brigade)
                                sprintf(brig, "%d", brigade->GetNameId());
                            else
                                strcpy(brig, "--");

                            sprintf(div, "%d", ((GroundUnitClass *)unit)->GetDivision());
                            strcpy(type, "BATTALION");
                            strcpy(vehicle, GetVehicleName(unit->GetVehicleID(0)));
                        }
                    }

                    fprintf(fp, "%d : %-4s : %-8s : %-5s : %-4s : %-4s : %-15s : %-20s : x : %d : y : %d : %s \n",
                            unit->GetReinforcement(), Side[unit->GetOwner()], domain, type, div, brig, unit->GetName(buffer, 60, FALSE), vehicle, (int)x, (int)y, objective);
                    unit = (Unit) myit.GetNext();
                }

                CloseCampFile(fp);
            }

            CampLeaveCriticalSection();
        }
        break;

        case ID_TOOLS_OUTPUTUNITLOCATIONS:
        {
            GridIndex x, y;
            FILE *fp;
            Unit unit;
            Unit brigade;
            Objective obj;
            char buffer[60], domain[8], type[12], div[5], brig[5], vehicle[20], objective[40];

            CampEnterCriticalSection();
            fp = OpenCampFile("Unitloc", "txt", "w");

            if (fp)
            {
                VuListIterator myit(AllUnitList);
                unit = (Unit) myit.GetFirst();

                while (unit not_eq NULL)
                {
                    unit->GetLocation(&x, &y);
                    obj = GetObjectiveByXY(x, y);

                    if (obj)
                        sprintf(objective, "%s", obj->GetName(buffer, 60, FALSE));
                    else
                        strcpy(objective, "none");

                    if (unit->GetDomain() == DOMAIN_AIR)
                    {
                        strcpy(domain, "AIR");

                        if (unit->GetType() == TYPE_SQUADRON)
                        {
                            strcpy(div, "n/a");
                            strcpy(brig, "n/a");
                            strcpy(type, "SQUADRON");

                            strcpy(vehicle, GetVehicleName(unit->GetVehicleID(0)));
                            //cur = DistanceToFront(x,y);
                            //min = u->GetUnitRange() / 50;
                            //max = u->GetUnitRange() / 4;
                        }
                    }
                    else if (unit->GetDomain() == DOMAIN_SEA)
                    {
                        strcpy(domain, "SEA");

                        if (unit->GetType() == TYPE_TASKFORCE)
                        {
                            strcpy(div, "n/a");
                            strcpy(brig, "n/a");
                            strcpy(type, "TASKFORCE");
                            strcpy(vehicle, GetVehicleName(unit->GetVehicleID(0)));
                        }
                    }
                    else if (unit->GetDomain() == DOMAIN_LAND)
                    {
                        strcpy(domain, "LAND");

                        if (unit->GetType() == TYPE_BRIGADE)
                        {
                            sprintf(div, "%d", ((GroundUnitClass *)unit)->GetDivision());
                            strcpy(brig, "n/a");
                            strcpy(type, "BRIGADE");
                            strcpy(vehicle, "n/a");
                        }
                        else if (unit->GetType() == TYPE_BATTALION)
                        {
                            brigade = unit->GetUnitParent();

                            if (brigade)
                                sprintf(brig, "%d", brigade->GetNameId());
                            else
                                strcpy(brig, "--");

                            sprintf(div, "%d", ((GroundUnitClass *)unit)->GetDivision());
                            strcpy(type, "BATTALION");
                            strcpy(vehicle, GetVehicleName(unit->GetVehicleID(0)));
                        }
                    }

                    fprintf(fp, "%-4s : %-8s : %-5s : %-4s : %-4s : %-15s : %-20s : x : %d : y : %d : %s : %d\n",
                            Side[unit->GetOwner()], domain, type, div, brig, unit->GetName(buffer, 60, FALSE), vehicle, (int)x, (int)y, objective, unit->Type());
                    unit = (Unit) myit.GetNext();
                }

                CloseCampFile(fp);
            }

            CampLeaveCriticalSection();
        }
        break;

        case ID_TOOLS_OUTPUTOBJLOCATIONS:
        {
            uchar *objmap, type;
            GridIndex x, y;
            FILE *fp;
            ObjClassDataType* oc;
            char *file;
            char buffer[60];

            InitConverter(TheCampaign.TheaterName);

            // This outputs two files - a text file and a raw file with
            // objective placement info.
            CampEnterCriticalSection();

            fp = OpenCampFile("Objloc", "txt", "w");

            if ( not fp)
            {
                CampLeaveCriticalSection();
                break;
            }

            objmap = new unsigned char[Map_Max_X * Map_Max_Y];
            memset(objmap, 255, sizeof(uchar)*Map_Max_X * Map_Max_Y);
            type = 1;

            // Collect data and print text file
            while (type <= NumObjectiveTypes)
            {
                VuListIterator oit(AllObjList);
                FromObjective = GetFirstObjective(&oit);

                while (FromObjective not_eq NULL)
                {
                    if (FromObjective->GetType() == type)
                    {
                        FromObjective->GetLocation(&x, &y);
                        file = GetFilename(x, y);
                        objmap[((Map_Max_Y - y - 1)*Map_Max_X) + x] = type;
                        oc = (ObjClassDataType*) Falcon4ClassTable[FromObjective->Type() - VU_LAST_ENTITY_TYPE].dataPtr;

                        if (oc)
                            fprintf(fp, "%-4s : %-15s : %-15s : %-20s : on : %-14s : at x: %4d : y: %4d : %d : %d\n",
                                    Side[FromObjective->GetOwner()], ObjectiveStr[type], oc->Name, FromObjective->GetName(buffer, 60, FALSE), file, y, x, FromObjective->GetObjectivePriority(), FromObjective->GetCampId());
                    }

                    FromObjective = GetNextObjective(&oit);
                }

                type++;
            }

            CloseCampFile(fp);
            // Now dump the RAW file
            fp = OpenCampFile("Objloc", "raw", "wb");
            fwrite(objmap, sizeof(uchar), Map_Max_X * Map_Max_Y, fp);
            CloseCampFile(fp);
            delete [] objmap;
            CampLeaveCriticalSection();
        }
        break;

        case ID_TOOLS_APPLYFORCERATIOS:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_FORCERARIODIALOG), hMainWnd, (DLGPROC)AdjustForceRatioProc);
            AdjustForceRatios();
            break;

        case ID_TOOLS_CLIPCAMPAIGN:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_CAMP_CLIPPER), hMainWnd, (DLGPROC)CampClipperProc);
            break;

        case ID_TOOLS_AUTOSETSAMARTSITES:
        {
            GridIndex x, y;
            float rel;

            CampEnterCriticalSection();
            {
                VuListIterator oit(AllObjList);
                FromObjective = GetFirstObjective(&oit);

                while (FromObjective)
                {
                    // SAM Site logic
                    if (FromObjective->GetType() == TYPE_AIRBASE or
                        FromObjective->GetType() == TYPE_ARMYBASE or
                        FromObjective->GetType() == TYPE_PORT or
                        FromObjective->GetType() == TYPE_FACTORY or
                        FromObjective->GetType() == TYPE_NUCLEAR or
                        FromObjective->GetType() == TYPE_POWERPLANT or
                        FromObjective->GetType() == TYPE_REFINERY or
                        FromObjective->GetType() == TYPE_CHEMICAL or
                        FromObjective->GetType() == TYPE_COM_CONTROL or
                        FromObjective->GetType() == TYPE_DEPOT or
                        FromObjective->GetType() == TYPE_RADAR or
                        FromObjective->GetType() == TYPE_SAM_SITE)
                        FromObjective->SetSamSite(1);
                    else
                        FromObjective->SetSamSite(0);

                    // HART Site logic
                    if (FromObjective->GetType() == TYPE_HARTS or
                        FromObjective->GetType() == TYPE_HILL_TOP)
                        FromObjective->SetArtillerySite(1);
                    else
                        FromObjective->SetArtillerySite(0);

                    // Commando logic
                    if (FromObjective->GetType() == TYPE_AIRBASE)
                        FromObjective->SetCommandoSite(1);
                    else
                        FromObjective->SetCommandoSite(0);

                    // Radar logic
                    if (FromObjective->GetType() == TYPE_AIRBASE or
                        FromObjective->GetType() == TYPE_RADAR)
                        FromObjective->SetRadarSite(1);
                    else
                        FromObjective->SetRadarSite(0);

                    // Mountain/Flat site logic
                    FromObjective->GetLocation(&x, &y);
                    rel = ((float)(GetRelief(x, y) + GetRelief(x + 1, y) + GetRelief(x - 1, y) + GetRelief(x, y + 1) + GetRelief(x, y - 1))) / 5.0F;

                    if (rel > 2.0F)
                    {
                        FromObjective->SetMountainSite(1);
                        FromObjective->SetFlatSite(0);
                    }
                    else if (rel < 0.5F)
                    {
                        FromObjective->SetMountainSite(0);
                        FromObjective->SetFlatSite(1);
                    }

                    FromObjective = GetNextObjective(&oit);
                }
            }
            CampLeaveCriticalSection();
        }
        break;

        case ID_TOOLS_PREORDERSAMARTILLERY:
        {
            Unit u;
            GridIndex x, y;
            Objective o;
            int role;
            VuListIterator uit(AllUnitList);
            u = (Unit) uit.GetFirst();

            while (u)
            {
                if (u->IsBattalion())
                {
                    role = u->GetUnitNormalRole();

                    if (role == GRO_FIRESUPPORT or role == GRO_AIRDEFENSE)
                    {
                        u->GetLocation(&x, &y);
                        o = FindNearestObjective(x, y, NULL);

                        if (o)
                            u->SetUnitOrders(GetGroundOrders(role), o->Id());
                    }
                }

                u = (Unit) uit.GetNext();
            }
        }
        break;

        case ID_TOOLS_CALCULATERADARARCS:
        {
            Objective o;
            float ox, oy, oz, x, y, r;
            float angle, ratio, max_ratio;
            mlTrig sincos;
            int arc;

            CampEnterCriticalSection();
            {
                VuListIterator oit(AllObjList);
                o = GetFirstObjective(&oit);

                while (o)
                {
                    if (o->SamSite() or o->RadarSite() or o->GetElectronicDetectionRange(LowAir) > 0)
                    {
                        // This place needs arc data
                        ox = o->XPos();
                        oy = o->YPos();
                        oz = TheMap.GetMEA(ox, oy);

                        if (o->static_data.radar_data)
                            delete o->static_data.radar_data;

                        o->static_data.radar_data = new RadarRangeClass;

                        for (arc = 0; arc < NUM_RADAR_ARCS; arc++)
                        {
                            // Find this arc's ratio
                            max_ratio = MINIMUM_RADAR_RATIO;
                            angle = arc * (360 / NUM_RADAR_ARCS) * DTR;

                            while (angle < (arc + 1) * (360 / NUM_RADAR_ARCS) * DTR)
                            {
                                // Trace a ray every 10 degrees
                                r = KM_TO_FT;
                                mlSinCos(&sincos, angle);

                                while (r * max_ratio < LOW_ALTITUDE_CUTOFF)
                                {
                                    // Step one km at a time
                                    x = ox + r * sincos.cos;
                                    y = oy + r * sincos.sin;
                                    ratio = (TheMap.GetMEA(x, y) - oz) / r;

                                    if (ratio > max_ratio)
                                        max_ratio = ratio;

                                    r += KM_TO_FT;
                                }

                                angle += 10 * DTR;
                            }

                            o->static_data.radar_data->SetArcRatio(arc, max_ratio);
                        }
                    }

                    o = GetNextObjective(&oit);
                }
            }
            CampLeaveCriticalSection();
        }
        break;

        case ID_TOOLS_GENERATEPAKMAP:
        {
            uchar *data = NULL;
            ulong size;
            FILE *fp;

            size = sizeof(uchar) * (Map_Max_X / 4) * (Map_Max_Y / 4);
            RebuildObjectiveLists();
            data = MakeCampMap(MAP_PAK_BUILD, data, 0);
            fp = fopen("pakmap.raw", "wb");

            if (fp)
            {
                fwrite(data, size, 1, fp);
                fclose(fp);
            }

            delete data;
            break;
        }

        case ID_HELP_ABOUTCAMPTOOL:
        {
            FARPROC lpProcAbout;

            lpProcAbout = MakeProcInstance((FARPROC)About, hInst);
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTDIALOG), hMainWnd, (DLGPROC)lpProcAbout);
            FreeProcInstance(lpProcAbout);
            break;
        }

        case ID_PB_CENTERONPLAYERBUBBLE:
            if (PBubble and FalconLocalSession->GetPlayerEntity())
            {
                TheCampaign.GetPlayerLocation(&MainMapData->CenX, &MainMapData->CenY);
                SetRefresh(MainMapData);
            }

            break;

        case ID_CAMP_REFRESHPB:
            RedrawPlayerBubble();
            break;

        default:
            return(FALSE);
    }

    return(TRUE);
}

LRESULT CALLBACK CampaignWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT retval = 0;
    static  int shifted;
    HBITMAP OldMap;

    switch (message)
    {
        case WM_CREATE:
            MainMapData = new MapWindowType;
            MainMapData->CellSize = CellSize;
            MainMapData->hMapWnd = hwnd;
            MainMapData->CenX = CenX;
            MainMapData->CenY = CenY;
            MainMapData->ShowUnits = TRUE;
            MainMapData->ShowObjectives = TRUE;
            MainMapData->ShowLinks = FALSE;
            MainMapData->Emitters = FALSE;
            MainMapData->ShowWPs = FALSE;
            MainMapData->SAMs = FALSE;
            hMainDC = GetDC(hMainWnd);
            hMapBM = CreateCompatibleBitmap(hMainDC, MAX_XPIX, MAX_YPIX);
            hMapDC = CreateCompatibleDC(hMainDC);
            MainMapData->hMapDC = hMapDC;
            OldMap = (HBITMAP)SelectObject(hMapDC, hMapBM);
            DeleteObject(OldMap);
            ResizeCursor();

            // set up it's menu bar
            EnableMenuItem(GetMenu(hMainWnd), ID_FILE_MYSAVE, MF_BYCOMMAND bitor MF_DISABLED);

            if (hMainMenu = GetSubMenu(GetMenu(hMainWnd), 0))
                DrawMenuBar(hMainWnd);

            // Init my graphics pointers
            _initgraphics(hMainDC);

            ReBlt = TRUE;
            SetRefresh(MainMapData);
            retval = 0;
            break;

        case WM_HSCROLL:
        {
            WORD nPos, nScrollCode;
            RECT r;

            nScrollCode = (int) LOWORD(wParam);  // scroll bar value
            nPos = (short int) HIWORD(wParam);   // scroll box position

            if (nScrollCode == 4)
            {
                ReBlt = TRUE;
                CenX = nPos * Map_Max_X / 100;
            }

            if (nScrollCode == SB_PAGELEFT)
            {
                ReBlt = TRUE;
                CenX = MainMapData->FX - ((MainMapData->LX - MainMapData->FX) / 4);

                if (CenX < 0)
                    CenX = 0;
            }

            if (nScrollCode == SB_PAGERIGHT)
            {
                ReBlt = TRUE;
                CenX = MainMapData->LX + ((MainMapData->LX - MainMapData->FX) / 4);

                if (CenX > Map_Max_X)
                    CenX = Map_Max_X;
            }

            if (nScrollCode == SB_LINELEFT)
            {
                ReBlt = TRUE;
                CenX -= Map_Max_X / 100;

                if (CenX < 0)
                    CenX = 0;
            }

            if (nScrollCode == SB_LINERIGHT)
            {
                ReBlt = TRUE;
                CenX += Map_Max_X / 100 + 1;

                if (CenX > Map_Max_X)
                    CenX = Map_Max_X;
            }

            if (ReBlt and nScrollCode not_eq 8)
            {
                MainMapData->CenX = CenX;
                MainMapData->CenY = CenY;
                GetClientRect(hMainWnd, &r);
                InvalidateRect(hMainWnd, &r, FALSE);
                //    SetScrollPos(hMainWnd, SB_HORZ, nPos, TRUE);
            }

            break;
        }

        case WM_VSCROLL:
        {
            WORD nPos, nScrollCode;
            RECT r;

            nScrollCode = (int) LOWORD(wParam);  // scroll bar value
            nPos = (short int) HIWORD(wParam);   // scroll box position

            if (nScrollCode == 4)
            {
                ReBlt = TRUE;
                CenY = Map_Max_Y - (nPos * Map_Max_Y / 100);
            }

            if (nScrollCode == SB_PAGELEFT)
            {
                ReBlt = TRUE;
                CenY = MainMapData->LY + ((MainMapData->LY - MainMapData->FY) / 4);

                if (CenY > Map_Max_Y)
                    CenY = Map_Max_Y;
            }

            if (nScrollCode == SB_PAGERIGHT)
            {
                ReBlt = TRUE;
                CenY = MainMapData->FY - ((MainMapData->LY - MainMapData->FY) / 4);

                if (CenY < 0)
                    CenY = 0;
            }

            if (nScrollCode == SB_LINELEFT)
            {
                ReBlt = TRUE;
                CenY += Map_Max_Y / 100 + 1;

                if (CenY > Map_Max_Y)
                    CenY = Map_Max_Y;
            }

            if (nScrollCode == SB_LINERIGHT)
            {
                ReBlt = TRUE;
                CenY -= Map_Max_Y / 100;

                if (CenY < 0)
                    CenY = 0;
            }

            if (ReBlt and nScrollCode not_eq 8)
            {
                MainMapData->CenX = CenX;
                MainMapData->CenY = CenY;
                GetClientRect(hMainWnd, &r);
                InvalidateRect(hMainWnd, &r, FALSE);
            }

            break;
        }

        case WM_MOUSEMOVE:
        {
            WORD fwKeys, xPos, yPos, xn, yn;
            static int coffx = 0, coffy = 0;
            POINT pt;
            RECT r;

            fwKeys = wParam;         // key flags
            /*
             if (fwKeys == MK_LBUTTON and Drawing)
             {
             xPos = LOWORD(lParam);
             yPos = HIWORD(lParam);
             ChangeCell((GridIndex)((xPos+PFX)/CellSize),(GridIndex)((PLY-yPos)/CellSize));
             }
            */
            GetCursorPos(&pt); // Get the real mouse position
            pt.x += coffx; // Add old offset
            pt.y += coffy;
            xPos = (WORD)(pt.x - MainMapData->WULX - MainMapData->CULX); // Get position relative to client area
            yPos = (WORD)(pt.y - MainMapData->WULY - MainMapData->CULY);
            CurX = (WORD)(xPos + MainMapData->PFX) / MainMapData->CellSize; // Get grid location in Campaign terms
            CurY = (MainMapData->PLY - 1 - yPos) / MainMapData->CellSize;

            if (fwKeys == MK_LBUTTON and Drawing)
                ChangeCell(CurX, CurY);

            coffx = xPos % CellSize; // Find our offset, if any
            coffy = yPos % CellSize;
            xn = (WORD) pt.x - coffx; // Find new grid location to snap to.
            yn = (WORD) pt.y - coffy;
            SetCursor(hCur);
            SetCursorPos(xn, yn); // Snap to the grid
            retval = 0;

            OneObjective = GetObjectiveByXY(CurX, CurY);

            if (ShowReal == 1)
                OneUnit = FindUnitByXY(AllRealList, CurX, CurY, 0);
            else if (ShowReal == 0)
                OneUnit = FindUnitByXY(AllParentList, CurX, CurY, 0);
            else if (ShowReal == 2)
            {
                OneUnit = FindUnitByXY(InactiveList, CurX, CurY, 0);
                /* if (OneUnit and not OneUnit->Real())
                 {
                 int foundone=0;
                 GridIndex x,y;
                 Unit e;
                 VuListIterator myit(InactiveList);

                 // Next unit in stack.
                 e = (Unit) myit.GetFirst();
                 while (e and e not_eq OneUnit)
                 e = GetNextUnit(&myit); // Get to current location in list
                 // e should be OneUnit or be null here
                 if (e)
                 {
                 e = GetNextUnit(&myit);
                 while (e and not foundone)
                 {
                 e->GetLocation(&x,&y);
                 if (x==CurX and y==CurY and e not_eq OneUnit)
                 foundone = 1;
                 else
                 e = GetNextUnit(&myit);
                 }
                 }
                 OneUnit = e;
                 }
                */
            }
            else
                OneUnit = NULL; // Can't edit divisions

            // Post a message to Toolbar window to redraw (And show new x,y pos)
            if (hToolWnd)
            {
                GetClientRect(hToolWnd, &r);
                InvalidateRect(hToolWnd, &r, FALSE);
                PostMessage(hToolWnd, WM_PAINT, 0, 0);
            }

            break;
        }

        case WM_KEYUP:
            if ((int)wParam == 16)
                shifted = FALSE;

            retval = 0;
            break;

        case WM_KEYDOWN:
        {
            int      C = (int)wParam;

            if (C == 16)
                shifted = TRUE;

            //if(isalpha(C) and shifted)
            //C += 0x20;

            if (isalpha(C) and not (GetKeyState(VK_SHIFT) bitand 0x80))
                C += 0x20;

            ProcessCommand(C);
            retval = 0;
            break;
        }

        case WM_DESTROY :
            ReleaseDC(hMainWnd, hMainDC);
            DeleteDC(hMapDC);
            DeleteObject(hMapBM);
            _shutdowngraphics();
            delete MainMapData;

            if (ShowCodes)
            {
                CleanupConverter();
                ShowCodes = FALSE;
            }

            displayCampaign = FALSE;
            hMainWnd = NULL;
            retval = 0;
            break;

        case WM_SIZE:
        {
            WORD nWidth, nHeight;
            int trunc, resize = 0;
            RECT r;

            nWidth = LOWORD(lParam);   // width of client area
            nHeight = HIWORD(lParam);  // height of client area
            ReBlt = TRUE;
            GetClientRect(hMainWnd, &r);

            if (nWidth % 16 not_eq 0)
            {
                trunc = 1 + nWidth / 16;
                r.right = 16 * trunc;
                resize = 1;

                if (r.right < 304)
                    r.right = 304;
            }

            if (nHeight % 16 not_eq 0)
            {
                if (r.right < 304)
                    r.right = 304;

                trunc = 1 + nHeight / 16;
                r.bottom = 16 * trunc;
                resize = 1;
            }

            if (resize)
            {
                // Note: This should work, but currently doesnt. MSVC++ bug?
                // AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW bitor  WS_HSCROLL bitor WS_VSCROLL, TRUE);
                AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, TRUE);
                r.right += GetSystemMetrics(SM_CXVSCROLL);
                r.bottom += GetSystemMetrics(SM_CYHSCROLL);
                SetWindowPos(hMainWnd, HWND_TOP, 0, 0, r.right - r.left, r.bottom - r.top, SWP_NOMOVE bitor SWP_NOZORDER);
                PostMessage(hMainWnd, WM_PAINT, 0, 0);
            }
            else
                PostMessage(hMainWnd, WM_PAINT, 0, 0);

            retval = 0;
            break;
        }

        case WM_MOVE:
            FindBorders(MainMapData);
            break;

        case WM_ACTIVATE:
            retval = 0;

            if (LOWORD(wParam) == WA_INACTIVE)
                ShowCursor(0);
            else
                ShowCursor(1);

            break;

        case WM_USER:
            retval = 0;
            break;

        case WM_LBUTTONDOWN:
        {
            WORD  fwKeys = wParam;

            // if (fwKeys=MK_SHIFT)
            // ;
            if (MainMapData->ShowWPs and WPUnit)
            {
                WayPoint w;
                GridIndex x, y;

                w = WPUnit->GetFirstUnitWP();

                while (w)
                {
                    w->GetWPLocation(&x, &y);

                    if (x == CurX and y == CurY)
                    {
                        GlobWP = w;
                        WPDrag = TRUE;
                    }

                    w = w->GetNextWP();
                }
            }
            else if (EditMode == 7)
            {
                DragObjective = OneObjective;
            }
            else if (EditMode == 8)
            {
                DragUnit = OneUnit;
            }
            else
            {
                ChangeCell(CurX, CurY);
                Drawing = TRUE;
            }
        }
        break;

        case WM_LBUTTONUP:
            Drawing = FALSE;

            if (WPDrag)
            {
                WayPoint w;
                GlobWP->SetWPLocation(CurX, CurY);
                w = WPUnit->GetFirstUnitWP();
                SetWPTimes(w, w->GetWPArrivalTime(), WPUnit->GetCruiseSpeed(), 0);
                WPDrag = FALSE;
                SetRefresh(MainMapData);
            }

            if (DragObjective)
            {
                DragObjective->SetLocation(CurX, CurY);
                RecalculateLinks(DragObjective);
                DragObjective = NULL;
                SetRefresh(MainMapData);
            }

            if (DragUnit)
            {
                DragUnit->SetLocation(CurX, CurY);
                DragUnit = NULL;
                SetRefresh(MainMapData);
            }

            break;

        case WM_RBUTTONDOWN:
        {
            switch (EditMode)
            {
                case 0:
                case 1:
                    DrawCover = GetGroundCover(GetCell(CurX, CurY));
                    break;

                case 2:
                    DrawRelief = GetReliefType(GetCell(CurX, CurY));
                    break;

                case 3:
                    DrawRoad = GetRoadCell(GetCell(CurX, CurY));
                    break;

                case 4:
                    DrawRail = GetRailCell(GetCell(CurX, CurY));
                    break;

                case 5:
                    DrawWeather = ((WeatherClass*)realWeather)->GetCloudCover(CurX, CurY);
                    break;

                case 6:
                    DrawWeather = ((WeatherClass*)realWeather)->GetCloudLevel(CurX, CurY);
                    break;

                case 7:
                    StartObjectiveEdit();
                    break;

                case 8:
                    StartUnitEdit();
                    break;

                default:
                    break;
            }

            break;
        }

        case WM_PAINT:
        {
            RECT  r;
            PAINTSTRUCT ps;
            HDC DC;

            if (GetUpdateRect(hMainWnd, &r, FALSE))
            {
                DC = BeginPaint(hMainWnd, &ps);
                FindBorders(MainMapData);
                SetScrollPos(hMainWnd, SB_HORZ, CenPer(MainMapData, CenX, XSIDE), TRUE);
                SetScrollPos(hMainWnd, SB_VERT, CenPer(MainMapData, CenY, YSIDE), TRUE);

                if (r.right > Map_Max_X * CellSize)
                {
                    _setcolor(DC, Blue);
                    _rectangle(DC, _GFILLINTERIOR, Map_Max_X * CellSize, 0, r.right, r.bottom);
                }

                if (r.bottom > Map_Max_Y * CellSize)
                {
                    _setcolor(DC, Blue);
                    _rectangle(DC, _GFILLINTERIOR, 0, Map_Max_Y * CellSize, r.right, r.bottom);
                }

                RefreshMap(MainMapData, DC, &r);
                ReBlt = FALSE;
                EndPaint(hMainWnd, &ps);
            }

            retval = 0;
            break;
        }

        case WM_COMMAND:
            MainWndCommandProc(hMainWnd, wParam, lParam);
            break;

        default:
            retval = DefWindowProc(hwnd, message, wParam, lParam);
            break;
    }

    return retval;
}

// ====================================
// Keyboard Command dispatch function
// ====================================

void ProcessCommand(int Key)
{
    GridIndex   X, Y, x, y;
    int i;

    X = (GridIndex) CurX;
    Y = (GridIndex) CurY;

    switch (Key)
    {
        case 'a':
            CampEnterCriticalSection();
            GlobUnit = AddUnit(X, Y, COUN_NORTH_KOREA);
            CampLeaveCriticalSection();
            DialogBox(hInst, MAKEINTRESOURCE(IDD_UNITDIALOG), hMainWnd, (DLGPROC)EditUnit);

            if (GlobUnit)
                GlobUnit = NULL;

            MainMapData->ShowUnits = TRUE;

            if (MainMapData->ShowWPs)
                SetRefresh(MainMapData);
            else
            {
                InvalidateRect(MainMapData->hMapWnd, NULL, FALSE);
                PostMessage(MainMapData->hMapWnd, WM_PAINT, (WPARAM)hMainDC, 0);
            }

            Saved = FALSE;
            break;

        case 'A':
            AssignBattToBrigDiv();
            break;

        case 'b':
            SHOWSTATS = not SHOWSTATS;
            break;

        case 'B':
            TheCampaign.MakeCampMap(MAP_SAMCOVERAGE);
            TheCampaign.MakeCampMap(MAP_RADARCOVERAGE);
            TheCampaign.MakeCampMap(MAP_OWNERSHIP);
            // RecalculateBrigadePositions();
            // SetRefresh(MainMapData);
            break;

        case 'c':
            DialogBox(hInst, MAKEINTRESOURCE(IDD_WEATHERDIALOG), hMainWnd, (DLGPROC)WeatherEditProc);
            break;

        case 'C':
            if (ptSelected == 2)
            {
                RegroupBrigades();
            }
            else
            {
                Sel1X = Sel2X = CurX;
                Sel1Y = Sel2Y = CurY;
                RegroupBrigades();
            }

            break;

        case 'd':
            gDumping = compl gDumping;
            break;

        case 'D':
            DeleteUnit(OneUnit);
            SetRefresh(MainMapData);
            break;

        case 'e':
            DialogBox(hInst, MAKEINTRESOURCE(IDD_TEAMEDIT_DIALOG), hMainWnd, (DLGPROC)EditTeams);
            break;

        case 'E':
            StateEdit = not StateEdit;

            if (ThisTeam == 0 or not StateToEdit)
                StateEdit = FALSE;

            SetRefresh(MainMapData);
            break;

        case 'f':
            break;

        case 'F':
            if (ShowFlanks)
                ShowFlanks = 0;
            else
                ShowFlanks = 1;

            SetRefresh(MainMapData);
            break;

        case 'G':
            ptSelected = 0;
            Sel1X = Sel2X = 0;
            Sel1Y = Sel2Y = 0;
            break;

        case 'g':
            if (ptSelected)
            {
                Sel2X = CurX;
                Sel2Y = CurY;
                ptSelected = 2;
            }
            else
            {
                Sel1X = CurX;
                Sel1Y = CurY;
                ptSelected = 1;
            }

            break;

        case 'H':

            // if (MainMapData->SAMs == 1)
            // MainMapData->SAMs = 2;
            // else if (MainMapData->SAMs == 2)
            // MainMapData->SAMs = 1;
            if (Mode == 10)
                Mode = 11;
            else if (Mode == 11)
                Mode = 10;

            SetRefresh(MainMapData);
            break;

        case 'l':
            if (OneObjective)
            {
                if ( not Linking)
                {
                    FromObjective = OneObjective;
                    Linking = TRUE;
                }
                else
                {
                    ToObjective = OneObjective;

                    ShowLinkCosts(FromObjective, ToObjective);

                    // CampEnterCriticalSection();
                    if ( not UnLinkCampaignObjectives(FromObjective, ToObjective))
                    {
                        PathClass path;
                        i = LinkCampaignObjectives(&path, FromObjective, ToObjective);

                        if (i < 1)
                            MessageBox(NULL, "No valid path found", "Error", MB_OK bitor MB_ICONSTOP bitor MB_SETFOREGROUND);

                        if (MainMapData->ShowLinks and i > 0)
                        {
                            RECT r;
                            PAINTSTRUCT ps;
                            HDC DC;
                            MapData md = MainMapData;

                            GetClientRect(hMainWnd, &r);
                            InvalidateRect(hMainWnd, &r, FALSE);
                            DC = BeginPaint(hMainWnd, &ps);
                            _setcolor(DC, White);
                            ToObjective->GetLocation(&x, &y);
                            _moveto(DC, POSX(x) + (CellSize >> 1), POSY(y) + (CellSize >> 1));
                            FromObjective->GetLocation(&x, &y);
                            _lineto(DC, POSX(x) + (CellSize >> 1), POSY(y) + (CellSize >> 1));
                            EndPaint(hMainWnd, &ps);
                        }
                        else
                        {
                            MainMapData->ShowLinks = TRUE;
                            SetRefresh(MainMapData);
                        }
                    }
                    else if (MainMapData->ShowLinks)
                        SetRefresh(MainMapData);

                    ToObjective->RecalculateParent();
                    FromObjective->RecalculateParent();

                    ShowLinkCosts(FromObjective, ToObjective);

                    // CampLeaveCriticalSection();
                    Linking = FALSE;
                    Saved = FALSE;
                }
            }

            break;

        case 'L':
            MainMapData->ShowLinks = not MainMapData->ShowLinks;
            SetRefresh(MainMapData);
            break;

        case 'm':
            DialogBox(hInst, MAKEINTRESOURCE(IDD_MISSTRIGDIALOG), hMainWnd, (DLGPROC)MissionTriggerProc);
            break;

        case 'M':
            DialogBox(hInst, MAKEINTRESOURCE(IDD_MAPDIALOG), hMainWnd, (DLGPROC)MapDialogProc);
            PostMessage(hMainWnd, WM_KEYUP, 16, 0);
            break;

        case 'n':
            if (StateEdit)
            {
                OneObjective = FindNearestObjective(X, Y, NULL);
                OneObjective->GetLocation(&x, &y);

                if (x == X and y == Y)
                {
                    RedrawCell(MainMapData, X, Y);
                }
            }

            break;

        case 'N':
            Mode++;

            if (Mode > 2)
                Mode = 0;

            SetRefresh(MainMapData);
            break;

        case 'o':
            StartObjectiveEdit();
            break;

        case 'O':
            MainMapData->ShowObjectives = not MainMapData->ShowObjectives;
            RebuildParentsList();
            SetRefresh(MainMapData);
            break;

        case 'p':
            if ( not FindPath)
            {
                Movx = X;
                Movy = Y;
                FindPath = TRUE;
            }
            else
            {
                PathClass path;

                CampEnterCriticalSection();
                maxSearch = GROUND_PATH_MAX;
                GetGridPath(&path, Movx, Movy, X, Y, gMoveType, gMoveWho, gMoveFlags);
                maxSearch = MAX_SEARCH;
                CampLeaveCriticalSection();
                ShowPath(MainMapData, Movx, Movy, &path, White);
                FindPath = FALSE;
            }

            break;

        case 'P':
            if ( not FindPath)
            {
                if (OneObjective)
                {
                    FromObjective = OneObjective;
                    FindPath = TRUE;
                }
            }
            else
            {
                PathClass path;

                if (OneObjective)
                {
                    CampEnterCriticalSection();
                    GetObjectivePath(&path, FromObjective, OneObjective, gMoveType, gMoveWho, gMoveFlags);
                    CampLeaveCriticalSection();
                    ShowObjectivePath(MainMapData, FromObjective, &path, Red);
                    FindPath = FALSE;
                }
            }

            /* if (ShowPaths)
             ShowPaths = 0;
             else
             ShowPaths = 1;
             SetRefresh(MainMapData);
            */
            break;

        case 'Q':
            ShowMissionLists();
            break;

        case 'r':
            DialogBox(hInst, MAKEINTRESOURCE(IDD_TEAMDIALOG), hMainWnd, (DLGPROC)EditRelations);
            break;

        case 'R':
            RoadsOn = not RoadsOn;
            RailsOn = not RailsOn;
            SetRefresh(MainMapData);
            break;

        case 's':
            DialogBox(hInst, MAKEINTRESOURCE(IDD_SQUADRONDIALOG), hMainWnd, (DLGPROC)SelectSquadron);
            break;

        case 'S':
            ShowSearch = not ShowSearch;
            break;

        case 't':
            //SetTimeCompression (gameCompressionRatio / 2);
            break;

        case 'T':
            //if (gameCompressionRatio < 1)
            // SetTimeCompression (1);
            //else
            // SetTimeCompression *2; /*(gameCompressionRatio * 2);*/
            break;

        case 'u':
            if (ptSelected == 2)
            {
                MoveUnits();
            }
            else
            {
                StartUnitEdit();
                /*
                if (ShowReal == 1)
                 OneUnit = FindUnitByXY(AllRealList,X,Y,0);
                else if (ShowReal == 0)
                 OneUnit = FindUnitByXY(AllParentList,X,Y,0);
                else if (ShowReal == 2)
                 OneUnit = FindUnitByXY(InactiveList,X,Y,0);
                else
                 OneUnit = NULL; // Can't edit divisions
                if (OneUnit)
                 GlobUnit = OneUnit;
                else
                 GlobUnit = NULL;

                if (GlobUnit)
                 DialogBox(hInst,MAKEINTRESOURCE(IDD_UNITDIALOG1),hMainWnd,(DLGPROC)EditUnit);
                if (MainMapData->ShowWPs)
                 SetRefresh(MainMapData);
                else
                 {
                 InvalidateRect(MainMapData->hMapWnd,NULL,FALSE);
                 PostMessage(MainMapData->hMapWnd,WM_PAINT,(WPARAM)hMainDC,0);
                 }*/
            }

            break;

        case 'U':
            MainMapData->ShowUnits = not MainMapData->ShowUnits;
            SetRefresh(MainMapData);
            break;

        case 'v':
        {
            extern int GetTopPriorityObjectives(int team, _TCHAR * buffers[5]);
            extern int GetTeamSituation(Team t);
            _TCHAR* junk[5];

            for (i = 0; i < 5; i++)
                junk[i] = new _TCHAR[100];

            GetTopPriorityObjectives(2, junk);
            i = GetTeamSituation(2);
        }
        break;

        case 'w':
            if (WPUnit)
            {
                WayPoint w;
                int gotone = 0;

                w = WPUnit->GetFirstUnitWP();

                while (w and not gotone)
                {
                    w->GetWPLocation(&x, &y);

                    if (x == X and y == Y)
                    {
                        GlobWP = w;
                        DialogBox(hInst, MAKEINTRESOURCE(IDD_WPDIALOG), hMainWnd, (DLGPROC)EditWayPoint);
                        gotone = 1;
                    }

                    w = w->GetNextWP();
                }

                if ( not gotone)
                {
                    w = WPUnit->GetFirstUnitWP();

                    if ( not w)
                    {
                        // WPUnit->AddCurrentWP (X,Y,0,0,0.0F,0,WP_NOTHING);
                        w = WPUnit->GetFirstUnitWP();
                        SetWPTimes(w, w->GetWPArrivalTime(), WPUnit->GetCruiseSpeed(), 0);
                    }
                    else
                    {
                        while (w->GetNextWP())
                            w = w->GetNextWP();

                        // GlobWP = WPUnit->AddWPAfter(w,X,Y,0,0,0.0F,0,WP_NOTHING);
                        w = WPUnit->GetFirstUnitWP();
                        SetWPTimes(w, w->GetWPArrivalTime(), WPUnit->GetCruiseSpeed());
                        DialogBox(hInst, MAKEINTRESOURCE(IDD_WPDIALOG), hMainWnd, (DLGPROC)EditWayPoint);
                    }

                    SetRefresh(MainMapData);
                }
            }

            break;

        case 'W':
            if (WPUnit and not MainMapData->ShowWPs)
                MainMapData->ShowWPs = TRUE;
            else
                MainMapData->ShowWPs = FALSE;

            SetRefresh(MainMapData);
            break;

        case 'x':
        case 'X':
            MainMapData->CenX = CenX = X;
            MainMapData->CenY = CenY = Y;
            zoomOut(MainMapData);
            break;

        case 'z':
        case 'Z':
            MainMapData->CenX = CenX = X;
            MainMapData->CenY = CenY = Y;
            zoomIn(MainMapData);
            break;

        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
            if (ptSelected == 2)
            {
                SetObjOwnersArea(Key - '1' + 1);
                SetRefresh(MainMapData);
            }
            else
            {
                OneObjective = GetObjectiveByXY(X, Y);

                if (OneObjective)
                {
                    OneObjective->SetOwner(Key - '1' + 1);
                }

                SetRefresh(MainMapData);
            }

            break;

        case 64:
            NoInput = 0;
            break;

        case 139:
            switch (Mode)
            {
                case 0:
                case 1:
                    i = GetReliefType(GetCell(X, Y));
                    i++;
                    SetReliefType(GetCell(X, Y), (ReliefType)i);
                    break;

                case 5:
                case 6:
                    i = ((WeatherClass*)realWeather)->GetCloudLevel(X, Y);
                    i++;
                    ((WeatherClass*)realWeather)->SetCloudLevel(X, Y, i);
                    break;

                default:
                    break;
            }

            Saved = FALSE;
            RedrawCell(MainMapData, X, Y);
            break;

        case 141:
            switch (Mode)
            {
                case 0:
                case 1:
                    i = GetReliefType(GetCell(X, Y));
                    i--;
                    SetReliefType(GetCell(X, Y), (ReliefType)i);
                    break;

                case 5:
                case 6:
                    i = ((WeatherClass*)realWeather)->GetCloudLevel(X, Y);
                    i--;
                    ((WeatherClass*)realWeather)->SetCloudLevel(X, Y, i);
                    break;

                default:
                    break;
            }

            Saved = FALSE;
            RedrawCell(MainMapData, X, Y);
            break;

        default:
            break;
    }
}

/*************************************************************************\
*
*  FUNCTION: CampMain(HANDLE, HANDLE, LPSTR, int)
*
*  PURPOSE: calls initialization function, processes message loop
*
*  COMMENTS:
*
\*************************************************************************/
void CampMain(HINSTANCE hInstance, int nCmdShow)
{
    RECT rect;
    WNDCLASS  mainwc;

    CenX = CenY = Map_Max_X / 2;
    CellSize = 4;
    MaxXSize = MAX_XPIX / CellSize;
    MaxYSize = MAX_YPIX / CellSize;

    srand((unsigned) time(NULL));
    InitDebug(DEBUGGER_TEXT_MODE);

    if (campCritical == NULL)
    {
        campCritical = F4CreateCriticalSection("campCritical");
    }

    hCurWait = LoadCursor(NULL, IDC_WAIT);
    hCurPoint = LoadCursor(NULL, IDC_ARROW);

    // Set up the main window
    mainwc.style = CS_HREDRAW bitor CS_VREDRAW;
    mainwc.lpfnWndProc = (WNDPROC)CampaignWndProc; // The client window procedure.
    mainwc.cbClsExtra = 0;                     // No room reserved for extra data.
    mainwc.cbWndExtra = sizeof(DWORD);
    mainwc.hInstance = hInstance;
    mainwc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    mainwc.hCursor = NULL;
    mainwc.hbrBackground = Brushes[Blue];
    mainwc.lpszMenuName = MAKEINTRESOURCE(IDR_CAMPAIGN_MENU);
    mainwc.lpszClassName = "CampTool";
    RegisterClass(&mainwc);
    rect.top = rect.left = 0;
    rect.right = 320;
    rect.bottom = 256;
    // Note: This should work, but currently doesnt. MSVC++ bug?
    // AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW bitor  WS_HSCROLL bitor WS_VSCROLL, TRUE);
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, TRUE);
    rect.right += GetSystemMetrics(SM_CXVSCROLL);
    rect.bottom += GetSystemMetrics(SM_CYHSCROLL);
    hMainWnd = CreateWindow("CampTool",
                            "Campaign Tool",
                            WS_OVERLAPPEDWINDOW bitor  WS_HSCROLL bitor WS_VSCROLL, // bitor WS_MAXIMIZE,
                            CW_USEDEFAULT, //  WS_CLIPCHILDREN |
                            CW_USEDEFAULT,
                            rect.right - rect.left, /* init. x size */
                            rect.bottom - rect.top, /* init. y size */
                            NULL,
                            NULL,
                            hInstance,
                            NULL);

    // set up data associated with this window
    SetWindowPos(hMainWnd, HWND_TOP, 600, 400, 650, 600, SWP_NOSIZE bitor SWP_NOZORDER);
    ShowWindow(hMainWnd, SW_SHOW);
    UpdateWindow(hMainWnd);
    RefreshCampMap();
}

void CampaignWindow(HINSTANCE hInstance, int nCmdShow)
{
    WNDCLASS  toolwc;

    // Set up the time/location window
    toolwc.style = CS_HREDRAW bitor CS_VREDRAW;
    toolwc.lpfnWndProc = (WNDPROC)ToolWndProc; // The client window procedure.
    toolwc.cbClsExtra = 0;                     // No room reserved for extra data.
    toolwc.cbWndExtra = sizeof(DWORD);
    toolwc.hInstance = hInstance;
    toolwc.hIcon = NULL;          // LoadIcon (NULL, IDI_APPLICATION);
    toolwc.hCursor = NULL;
    toolwc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    toolwc.lpszMenuName = "";
    toolwc.lpszClassName = "Toolbar";
    RegisterClass(&toolwc);
    hToolWnd = CreateWindow("Toolbar",
                            "Toolbar",
                            WS_OVERLAPPEDWINDOW,
                            CW_USEDEFAULT,
                            CW_USEDEFAULT,
                            300,
                            100,
                            NULL,
                            NULL,
                            hInstance,
                            NULL);

    // set up data associated with this window
    SetWindowPos(hToolWnd, HWND_TOP, 10, 10, 10, 10, SWP_NOSIZE bitor SWP_NOZORDER);
    ShowWindow(hToolWnd, SW_SHOW);
    UpdateWindow(hToolWnd);
}

void ShowCampaign(void)
{
    if (hMainWnd)
    {
        ShowWindow(hMainWnd, SW_SHOW);
        UpdateWindow(hMainWnd);
    }

    if (hToolWnd)
    {
        ShowWindow(hToolWnd, SW_SHOW);
        UpdateWindow(hToolWnd);
    }
}

#endif CAMPTOOL
