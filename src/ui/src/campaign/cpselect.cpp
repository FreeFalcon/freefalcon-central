/***************************************************************************\
 UI_cpsel.cpp
 Peter Ward
 December 3, 1996

 Main UI screen stuff for FreeFalcon
\***************************************************************************/
#include <windows.h>
#include "falclib.h"
#include "targa.h"
#include "dxutil/ddutil.h"
#include "Graphics/Include/imagebuf.h"
#include "ui95_dd.h"
#include "chandler.h"
#include "ui95_ext.h"
#include "cmusic.h"
#include "entity.h"
#include "feature.h"
#include "vehicle.h"
#include "evtparse.h"
#include "Mesg.h"
#include "MsgInc/DamageMsg.h"
#include "MsgInc/WeaponFireMsg.h"
#include "MsgInc/DeathMessage.h"
#include "MsgInc/MissileEndMsg.h"
#include "MsgInc/LandingMessage.h"
#include "falcsess.h"
#include "squadron.h"
#include "cmpclass.h"
#include "campmap.h"
#include "CampJoin.h"
#include "division.h"
#include "campstr.h"
#include "find.h"
#include "playerop.h"
#include "options.h"
#include "falcuser.h"
#include "falclib/include/f4find.h"
#include "f4error.h"
#include "uicomms.h"
#include "cmap.h"
#include "gps.h"
#include "ui_cmpgn.h"
#include "userids.h"
#include "textids.h"
#include "teamdata.h"
#include "UI.h"
#include "comms/capi.h"
#include "Dispcfg.h"

#pragma warning(disable: 4244)

// This is a list for GetFileList... files which WON'T show up in the list window
// MUST be NULL terminated
extern _TCHAR *CampExcludeList[];

extern uchar max_veh[5];

extern C_Handler *gMainHandler;
extern C_Parser *gMainParser;;
extern C_Music  *gMusic;
extern int CPSelectLoaded;
extern int PlannerLoaded;
extern int CampaignLastGroup;
extern C_Map *gMapMgr;
extern GlobalPositioningSystem *gGps;
extern long gDFTeamID;
extern long _IsF16_;
extern int gCampDataVersion, gCurrentDataVersion;

extern bool g_bHiResUI;
extern bool g_LargeTheater;

IMAGE_RSC *gOccupationMap = NULL;
IMAGE_RSC *gBigOccupationMap = NULL;

int CampSelMode = 0;
//dpc ExitCampSelectFix
static int oldCampSelMode = 0;
extern bool g_bExitCampSelectFix;
extern bool g_bCampSavedMenuHack;
//end ExitCampSelectFix
extern long gLastUpdateGround, gLastUpdateAir;

extern short ConvertDFIDtoTeam(long ID);
extern void EnableCampaignMenus(void);
extern void create_tactical_scenario_info(void);

void CommsErrorDialog(long TitleID, long MessageID, void (*OKCB)(long, short, C_Base*), void (*CancelCB)(long, short, C_Base*));
void SetupInfoWindow(void (*tOkCB)(), void (*tCancelCB)());
void InfoButtonCB(long ID, short hittype, C_Base *control);
void CancelJoinCB();
void RedrawTreeWindowCB(long ID, short hittype, C_Base *control);
static void HookupCampaignSelectControls(long ID);
void StartCampaignGame(int local, int game_type);
void UI_Help_Guide_CB(long ID, short hittype, C_Base *ctrl);
void CleanupView();
void CreateGameList(long ID, uchar gamemask, void (*gcb)(long, short, C_Base *), void (*pcb)(long, short, C_Base *), long Client);
void CloseWindowCB(long ID, short hittype, C_Base *control);
void SetSingle_Comms_Ctrls();
void CampaignListCB();
void CampaignSoundEventCB();
void CampaignUpdateMapCB();
void LoadCampaignWindows();
void LoadPlannerWindows();
void MakeOccupationMap(IMAGE_RSC *Map);
void MakeBigOccupationMap(IMAGE_RSC *Map);
void ToggleATOInfoCB(long ID, short hittype, C_Base *control);
void SetupMapSquadronWindow(int x, int y);
void GenericTimerCB(long ID, short hittype, C_Base *control);
void DelSTRFileCB(long ID, short hittype, C_Base *control);
void DelDFSFileCB(long ID, short hittype, C_Base *control);
void DelLSTFileCB(long ID, short hittype, C_Base *control);
void DelCamFileCB(long ID, short hittype, C_Base *control);
void DelTacFileCB(long ID, short hittype, C_Base *control);
void DelTGAFileCB(long ID, short hittype, C_Base *control);
void DelVHSFileCB(long ID, short hittype, C_Base *control);
void DelKeyFileCB(long ID, short hittype, C_Base *control);
void SetDeleteCallback(void (*cb)(long, short, C_Base*));
void GetFileListTree(C_TreeList *tree, _TCHAR *fspec, _TCHAR *excludelist[], long group, BOOL cutext, long UseMenu);
_TCHAR *OrdinalString(long value);
void PlayUIMovie(long ID);
BOOL CheckExclude(_TCHAR *filename, _TCHAR *directory, _TCHAR *ExcludeList[], _TCHAR *extension);
void VerifyDelete(long TitleID, void (*YesCB)(long, short, C_Base*), void (*NoCB)(long, short, C_Base*));
void EnableScenarioInfo(long ID);
void DisableScenarioInfo();
void SelectScenarioCB(long ID, short hittype, C_Base *control);
static void PickSquadronCB(long ID, short hittype, C_Base *control);
static void PickAirbaseCB(long ID, short hittype, C_Base *control);
void DisplayJoinStatusWindow(int);
void MinMaxWindowCB(long ID, short hittype, C_Base *control);
void CopySettingsToTemp(void);
BOOL FileNameSortCB(TREELIST *list, TREELIST *newitem);
extern BOOL AddWordWrapTextToWindow(C_Window *win, short *x, short *y, short startcol, short endcol, COLORREF color, _TCHAR *str, long Client = 0);

char gUI_CampaignFile[MAX_PATH];
_TCHAR gUI_ScenarioName[64];
_TCHAR gLastCampFilename[MAX_PATH];

static int gSelectedSquadronID = -1;
void DeleteGroupList(long ID);
VU_ID gPlayerSquadronId;

extern C_TreeList *CampaignGames;
extern int mcnt;
extern int atocnt;
extern int uintcnt;

extern bool g_bHiResUI; // M.N.

enum
{
    SND_SCREAM        = 500005,
    SND_BAD1          = 500006,
    SND_SECOND        = 500007,
    SND_FIRST         = 500008,
    SND_NICE          = 500009,
    SND_BAD2          = 500010,
    SND_YOUSUCK       = 500011,
    SND_AMBIENT   = 500033,
    SND_CAMPAIGNMUSIC = 500050,
};

enum
{
    BLUE_TEAM_ICONS = 565120000,
    BLUE_TEAM_ICONS_W = 565120001,
    BROWN_TEAM_ICONS = 565120002, // M.N.
    BROWN_TEAM_ICONS_W = 565120003,
    GREEN_TEAM_ICONS = 565120004,
    GREEN_TEAM_ICONS_W = 565120005,
    GREY_TEAM_ICONS = 565120006,
    GREY_TEAM_ICONS_W = 565120007,
    ORANGE_TEAM_ICONS = 565120008,
    ORANGE_TEAM_ICONS_W = 565120009,
    RED_TEAM_ICONS = 565120010,
    RED_TEAM_ICONS_W = 565120011,
    WHITE_TEAM_ICONS = 565120012,
    WHITE_TEAM_ICONS_W = 565120013,
    YELLOW_TEAM_ICONS = 565120014,
    YELLOW_TEAM_ICONS_W = 565120015,
    CAMP_AIR_BASE_ICON          = 10003,
};

typedef struct
{
    uchar Challenge;
    uchar PilotSkill;
    uchar SAMSkill;
    uchar AirForces;
    uchar AirDefenses;
    uchar GroundForces;
    uchar NavalForces;
} CampaignSettings;

static CampaignSettings TEMP_Settings;

void LoadCampaignSelectWindows()
{
    long ID;

    if (CPSelectLoaded) return;

    if (_LOAD_ART_RESOURCES_)
        gMainParser->LoadImageList("cs_res.lst");
    else
        gMainParser->LoadImageList("cs_art.lst");

    gMainParser->LoadSoundList("cs_snd.lst");
    gMainParser->LoadWindowList("cs_scf.lst");  // Modified by M.N. - add art/art1024 by LoadWindowList

    ID = gMainParser->GetFirstWindowLoaded();

    while (ID)
    {
        HookupCampaignSelectControls(ID);
        ID = gMainParser->GetNextWindowLoaded();
    }

    CPSelectLoaded++;
    SetSingle_Comms_Ctrls();

    if ( not PlannerLoaded)
        LoadPlannerWindows();

    CampSelMode = 0;
}

long SituationStr[] =
{
    TXT_SIT_BAD,
    TXT_SIT_POOR,
    TXT_SIT_AVERAGE,
    TXT_SIT_GOOD,
    TXT_SIT_GREAT,
};

long SpecialtyStr[] =
{
    TXT_GENERAL,
    TXT_AIR_TO_AIR,
    TXT_AIR_TO_GROUND,
};

IMAGE_RSC *CreateOccupationMap(long ID, long w, long h, long palsize)
{
    IMAGE_RSC *rsc;
    C_Resmgr  *res;
    unsigned char *data8;
    long size;
    DWORD r_mask;
    WORD r_shift;
    DWORD g_mask;
    WORD g_shift;
    DWORD b_mask;
    WORD b_shift;

    if (palsize > 256)
        return(NULL);

    size = w * h + palsize * 2;
    data8 = new unsigned char[size];

    if ( not data8)
        return(NULL);

    res = new C_Resmgr;
    res->Setup(ID);
    res->SetColorKey(UI95_RGB24Bit(0x00ff00ff));
    //UI95_GetScreenColorInfo(&r_mask,&r_shift,&g_mask,&g_shift,&b_mask,&b_shift);
    UI95_GetScreenColorInfo(r_mask, r_shift, g_mask, g_shift, b_mask, b_shift);
    res->SetScreenFormat(r_shift, g_shift, b_shift);

    rsc = new IMAGE_RSC;
    rsc->ID = ID;
    rsc->Header = new ImageHeader;
    rsc->Header->Type = _RSC_IS_IMAGE_;
    rsc->Header->ID[0] = 0;
    rsc->Header->flags = _RSC_8_BIT_;
    rsc->Header->w = static_cast<short>(w);
    rsc->Header->h = static_cast<short>(h);
    rsc->Header->centerx = 0;
    rsc->Header->centery = 0;
    rsc->Header->imageoffset = 0;
    rsc->Header->paletteoffset = w * h;
    rsc->Header->palettesize = palsize;
    rsc->Owner = res;

    res->AddIndex(ID, rsc);
    res->SetData((char*)data8);
    memset(data8, 0, size);
    return(rsc);
}

void SetupMapWindow()
{
    C_Window *win;
    C_Bitmap *bmp;

    win = gMainHandler->FindWindow(CS_MAP_WIN);

    // MN turn off the occupation maps for now for 128x128 theaters - they cause CTD's...
    if (win and not g_LargeTheater)
    {
        DeleteGroupList(CS_MAP_WIN);

        // Create Occupation Map
        if (gOccupationMap == NULL and (TheCampaign.TheaterSizeX and TheCampaign.TheaterSizeY))
            gOccupationMap = CreateOccupationMap(1, TheCampaign.TheaterSizeX / MAP_RATIO, TheCampaign.TheaterSizeY / MAP_RATIO, 16);

        if (gOccupationMap)
            MakeOccupationMap(gOccupationMap);

        // MN big occupation map when HiResUI
        if (g_bHiResUI)
        {
            if (gBigOccupationMap == NULL and (TheCampaign.TheaterSizeX and TheCampaign.TheaterSizeY))
                gBigOccupationMap = CreateOccupationMap(2, TheCampaign.TheaterSizeX / (MAP_RATIO / 2), TheCampaign.TheaterSizeY / (MAP_RATIO / 2), 16);

            if (gBigOccupationMap)
                MakeBigOccupationMap(gBigOccupationMap);
        }

        bmp = (C_Bitmap *)win->FindControl(CS_MAP_WIN);

        if (bmp)
        {
            /* if(gOccupationMap == NULL and (TheCampaign.TheaterSizeX and TheCampaign.TheaterSizeY))
             {
             // Create Occupation map...
             gOccupationMap=CreateOccupationMap(1,TheCampaign.TheaterSizeX/MAP_RATIO,TheCampaign.TheaterSizeY/MAP_RATIO,16);*/
            if (g_bHiResUI)
                bmp->SetImage(gBigOccupationMap);
            else
                bmp->SetImage(gOccupationMap);

            /* }
            // if(gOccupationMap)
             MakeOccupationMap(gOccupationMap);*/
        }
    }
}

void AddSquadronsToMap()
{
    C_Window *win;
    C_Button *btn;
    C_Resmgr *res, *res_w;
    IMAGE_RSC *rsc;
    int i;//was uint
    long IconID;
    int savex = -1, savey = -1;
    float x, y;
    float maxy;

    maxy = (float)(TheCampaign.TheaterSizeY) * FEET_PER_KM;

    win = gMainHandler->FindWindow(CS_MAP_WIN);

    if (win)
    {
        // This should be found based on your team...
        // NOT hard coded
        DeleteGroupList(CS_MAP_WIN);

        // Add all the Squadron Patches into the map M.N. use different colors per team
        for (i = 0; i < TheCampaign.NumAvailSquadrons; i++)
        {
            switch (TheCampaign.CampaignSquadronData[i].country)
            {
                case 0:
                    res = gImageMgr->GetImageRes(GREY_TEAM_ICONS);
                    res_w = gImageMgr->GetImageRes(GREY_TEAM_ICONS_W);
                    break;

                case 1:
                    res = gImageMgr->GetImageRes(WHITE_TEAM_ICONS);
                    res_w = gImageMgr->GetImageRes(WHITE_TEAM_ICONS_W);
                    break;

                case 2:
                    res = gImageMgr->GetImageRes(BLUE_TEAM_ICONS);
                    res_w = gImageMgr->GetImageRes(BLUE_TEAM_ICONS_W);
                    break;

                case 3:
                    res = gImageMgr->GetImageRes(BROWN_TEAM_ICONS);
                    res_w = gImageMgr->GetImageRes(BROWN_TEAM_ICONS_W);
                    break;

                case 4:
                    res = gImageMgr->GetImageRes(ORANGE_TEAM_ICONS);
                    res_w = gImageMgr->GetImageRes(ORANGE_TEAM_ICONS_W);
                    break;

                case 5:
                    res = gImageMgr->GetImageRes(YELLOW_TEAM_ICONS);
                    res_w = gImageMgr->GetImageRes(YELLOW_TEAM_ICONS_W);
                    break;

                case 6:
                    res = gImageMgr->GetImageRes(RED_TEAM_ICONS);
                    res_w = gImageMgr->GetImageRes(RED_TEAM_ICONS_W);
                    break;

                case 7:
                    res = gImageMgr->GetImageRes(GREEN_TEAM_ICONS);
                    res_w = gImageMgr->GetImageRes(GREEN_TEAM_ICONS_W);
                    break;

                default:
                    ShiWarning("Bad team found");
                    res = gImageMgr->GetImageRes(BLUE_TEAM_ICONS);
                    res_w = gImageMgr->GetImageRes(BLUE_TEAM_ICONS_W);
            }

            if (TheCampaign.IsValidSquadron(i)  or _IsF16_)
            {
                x = TheCampaign.CampaignSquadronData[i].y; // real world x bitand y are y bitand x
                y = TheCampaign.CampaignSquadronData[i].x;

                int mapratio = MAP_RATIO;

                // 2002-02-01 MN This fixes squad selection map in hires UI - still need a solution for lowres UI
                if (g_bHiResUI and not g_LargeTheater)
                {
                    mapratio /= 2;
                }

                x = x / (FEET_PER_KM * mapratio);
                y = (maxy - y) / (FEET_PER_KM * mapratio);

                // IconID=TheCampaign.CampaignSquadronData[i].airbaseIcon; // too big
                IconID = 10003;

                if (i == gSelectedSquadronID or savex == -1 or savey == -1)
                {
                    savex = static_cast<int>(x);
                    savey = static_cast<int>(y);
                }

                btn = new C_Button;
                btn->Setup(((long)x << 16) bitor (long)y, C_TYPE_RADIO, (int)x, (int)y);
                btn->SetFlagBitOn(C_BIT_HCENTER bitor C_BIT_VCENTER);
                btn->SetGroup(-100);
                btn->SetCluster(i + 1);

                if (res)
                {
                    rsc = (IMAGE_RSC*)res->Find(IconID);

                    if (rsc and rsc->Header->Type == _RSC_IS_IMAGE_)
                        btn->SetImage(C_STATE_0, rsc);
                }

                if (res_w)
                {
                    rsc = (IMAGE_RSC*)res_w->Find(IconID);

                    if (rsc and rsc->Header->Type == _RSC_IS_IMAGE_)
                        btn->SetImage(C_STATE_1, rsc);
                }

                btn->SetCallback(PickAirbaseCB);
                btn->SetUserNumber(_UI95_DELGROUP_SLOT_, _UI95_DELGROUP_ID_);
                btn->SetHelpText(HELP_PICK_AIRBASE);
                btn->SetCursorID(CRSR_F16_ON);

                if (gSelectedSquadronID == i)
                    btn->SetState(1);

                win->AddControl(btn);
            }
        }

        win->RefreshWindow();
        SetupMapSquadronWindow(savex, savey);
    }
}

void SetupMapSquadronWindow(int airbasex, int airbasey)
{
    C_Window *win;
    C_Button *btn;
    C_Text   *txt;
    int i;//was uint
    int icony, mapratio;
    float x, y;
    float maxy;
    short NameShown = 0;
    SquadUIInfoClass *SquadPtr;
    _TCHAR buffer[30];

    maxy = (float)(TheCampaign.TheaterSizeY) * FEET_PER_KM;

    win = gMainHandler->FindWindow(CS_SUA_WIN);

    if (win)
    {
        DeleteGroupList(CS_SUA_WIN);

        icony = 40;

        txt = new C_Text;
        txt->Setup(C_DONT_CARE, C_TYPE_LEFT);
        txt->SetText(TXT_CHOOSE_SQUADRON);
        txt->SetXY(10, icony);
        txt->SetFont(win->Font_);
        txt->SetUserNumber(_UI95_DELGROUP_SLOT_, _UI95_DELGROUP_ID_);
        txt->SetFlagBitOn(C_BIT_LEFT);
        txt->SetFGColor(0x00e0e0e0);
        win->AddControl(txt);
        icony += gFontList->GetHeight(win->Font_) + 4;

        for (i = 0; i < TheCampaign.NumAvailSquadrons; i++)
        {
            if (TheCampaign.IsValidSquadron(i)  or _IsF16_)
            {
                x = TheCampaign.CampaignSquadronData[i].y; // real world x bitand y are y bitand x
                y = TheCampaign.CampaignSquadronData[i].x;

                // 2001-12-12 M.N. adapted for 1024 UI
                mapratio = MAP_RATIO;

                if (g_bHiResUI and not g_LargeTheater)
                    mapratio /= 2;

                x = x / (FEET_PER_KM * mapratio);
                y = (maxy - y) / (FEET_PER_KM * mapratio);

                if ((int)x == airbasex and (int)y == airbasey)
                {
                    SquadPtr = &TheCampaign.CampaignSquadronData[i];

                    if ( not NameShown and SquadPtr)
                    {
                        // Airbase Name
                        txt = new C_Text;
                        txt->Setup(C_DONT_CARE, C_TYPE_LEFT);
                        txt->SetXY(10, icony);
                        txt->SetFont(win->Font_);
                        txt->SetUserNumber(_UI95_DELGROUP_SLOT_, _UI95_DELGROUP_ID_);
                        txt->SetFlagBitOn(C_BIT_LEFT bitor C_BIT_WORDWRAP);
                        txt->SetW(123);
                        txt->SetFGColor(0x00e0e0e0);
                        txt->SetText(SquadPtr->airbaseName);
                        win->AddControl(txt);
                        icony += txt->GetH();
                        NameShown = 1;
                    }

                    _stprintf(buffer, "%s %s", OrdinalString(SquadPtr->nameId), gStringMgr->GetString(TXT_SQUADRON));
                    btn = new C_Button;
                    btn->Setup(i + 1, C_TYPE_RADIO, 20, icony);
                    btn->SetGroup(-200);
                    btn->SetFont(win->Font_);
                    btn->SetText(C_STATE_0, buffer);
                    btn->SetText(C_STATE_1, buffer);
                    btn->SetColor(C_STATE_0, 0xe0e0e0);
                    btn->SetColor(C_STATE_1, 0x00ff00);
                    btn->SetFlagBitOn(C_BIT_LEFT bitor C_BIT_USELINE);
                    btn->SetCallback(PickSquadronCB);
                    btn->SetUserNumber(_UI95_DELGROUP_SLOT_, _UI95_DELGROUP_ID_); // used in DeleteGameList to find records to remove
                    btn->SetHelpText(HELP_PICK_SQUADRON);
                    btn->SetCursorID(CRSR_F16_ON);
                    win->AddControl(btn);

                    if (i == gSelectedSquadronID or gSelectedSquadronID < 0)
                    {
                        gSelectedSquadronID = i;
                        btn->SetState(1);
                        PickSquadronCB(i + 1, C_TYPE_LMOUSEUP, btn);
                    }

                    icony += (btn->GetH() + 2);
                }
            }
        }

        win->RefreshWindow();
    }
}

void LoadSquadronInfo()
{
    C_Window *win;
    C_Text *txt;
    C_Bitmap *bmp;
    SquadUIInfoClass *SquadPtr;
    _TCHAR buffer[60];

    win = gMainHandler->FindWindow(CS_PUA_WIN);

    if (win)
    {
        // Need squadron info now

        if (gSelectedSquadronID < 0 or gSelectedSquadronID >= TheCampaign.NumAvailSquadrons)
        {
            // KCK: Pick a valid squadron with the lowest id.
            gSelectedSquadronID = -1;

            for (int i = 0; i < TheCampaign.NumAvailSquadrons; i++)
            {
                if ((TheCampaign.IsValidSquadron(i) or _IsF16_) and (gSelectedSquadronID < 0 or TheCampaign.CampaignSquadronData[i].id.num_ < TheCampaign.CampaignSquadronData[gSelectedSquadronID].id.num_))
                    gSelectedSquadronID = i;
            }
        }

        if (gSelectedSquadronID < 0)
            return;

        SquadPtr = &TheCampaign.CampaignSquadronData[gSelectedSquadronID];

        if (SquadPtr)
        {
            UnitClassDataType* uc = (UnitClassDataType*)(Falcon4ClassTable[SquadPtr->dIndex].dataPtr);
            VehicleClassDataType* vc = NULL;
            ShiAssert(uc);

            if (uc)
                vc = (VehicleClassDataType*)(Falcon4ClassTable[uc->VehicleType[0]].dataPtr);

            // Specialty
            txt = (C_Text *)win->FindControl(SPEC_FIELD);

            if (txt)
            {
                txt->SetText(SpecialtyStr[SquadPtr->specialty % 3]);
            }

            // status (# humans,%strength)
            txt = (C_Text *)win->FindControl(STATUS_FIELD);

            if (txt and vc)
            {
                if (CampSelMode == 2 and gCommsMgr and gCommsMgr->GetTargetGame())
                {
                    // Online game - Count # of players
                    int players = 0;
                    FalconSessionEntity *session;
                    VuSessionsIterator sit(gCommsMgr->GetTargetGame());

                    session = (FalconSessionEntity*) sit.GetFirst();

                    while (session)
                    {
                        // WM 09-28-03  Display the number of players actually in the selected
                        //  squadron.  Not just the number of total players in the game.
                        if (session->GetPlayerSquadronID()  and 
                            session->GetPlayerSquadronID().num_ == SquadPtr->id.num_)
                            players++;

                        session = (FalconSessionEntity*) sit.GetNext();
                    }

                    _stprintf(buffer, "%1d %s, %1d %s", players, gStringMgr->GetString(TXT_PLAYERS), SquadPtr->currentStrength, vc->Name);
                }
                else if ( not CampSelMode)
                {
                    // # of aircraft based on ratio setting for new games
                    int aircraft = 0, mv, i;
                    mv = max_veh[PlayerOptions.CampAirRatio];

                    for (i = 0; i < mv; i++)
                        aircraft += uc->NumElements[i];

                    _stprintf(buffer, "%d %s", aircraft, vc->Name);
                }
                else
                    _stprintf(buffer, "%d %s", SquadPtr->currentStrength, vc->Name);

                txt->SetText(buffer);
                txt->Refresh();
            }

            txt = (C_Text *)win->FindControl(SQUAD_NAME);

            if (txt)
            {
                _stprintf(buffer, "%s %s", OrdinalString(SquadPtr->nameId), gStringMgr->GetString(TXT_FS));
                txt->SetText(buffer);
            }

            txt = (C_Text *)win->FindControl(AIRBASE_NAME);

            if (txt)
            {
                _stprintf(buffer, "%s", SquadPtr->airbaseName);
                txt->SetText(buffer);
            }

            bmp = (C_Bitmap *)win->FindControl(PATCH);

            if (bmp)
            {
                if ( not SquadPtr->squadronPatch)
                {
                    SquadPtr->squadronPatch = AssignUISquadronID(SquadPtr->nameId);
                }

                bmp->SetImage(SquadronMatchIDs[SquadPtr->squadronPatch][0]);
            }
        }

        win->RefreshWindow();
    }
}

static void CalcChallengeLevel()
{
    C_Window *win;
    C_ListBox *lb;

    // Joes Equation Goes HERE
    TEMP_Settings.Challenge = TEMP_Settings.PilotSkill;
    TEMP_Settings.Challenge += TEMP_Settings.SAMSkill;
    TEMP_Settings.Challenge += (4 - TEMP_Settings.AirForces);
    TEMP_Settings.Challenge += (4 - TEMP_Settings.AirDefenses);
    TEMP_Settings.Challenge += (4 - TEMP_Settings.GroundForces);
    TEMP_Settings.Challenge += (4 - TEMP_Settings.NavalForces);

    TEMP_Settings.Challenge /= 6;

    win = gMainHandler->FindWindow(CHALLENGE_WIN);

    if (win)
    {
        lb = (C_ListBox *)win->FindControl(CHALLENGE_LIST);

        if (lb)
        {
            lb->Refresh();

            switch (TEMP_Settings.Challenge)
            {
                case 1:
                    lb->SetValue(CADET_LEVEL);
                    break;

                case 2:
                    lb->SetValue(ROOKIE_LEVEL);
                    break;

                case 3:
                    lb->SetValue(VETERAN_LEVEL);
                    break;

                case 4:
                    lb->SetValue(ACE_LEVEL);
                    break;

                default:
                    lb->SetValue(RECRUIT_LEVEL);
                    break;
            }

            lb->Refresh();
        }
    }
}

void LoadScenarioInfo()
{
    C_Window *win;
    C_Text *txt;
    C_EditBox *ebox;
    C_Button *btn;
    C_Clock *clk;
    _TCHAR buffer[60];

    CopySettingsToTemp();
    CalcChallengeLevel();

    win = gMainHandler->FindWindow(CS_PUA_WIN);

    if (win)
    {
        ebox = (C_EditBox *)win->FindControl(TITLE_LABEL);

        if (ebox)
        {
            _stprintf(buffer, "%s", gUI_ScenarioName);
            ebox->SetText(buffer);
        }

        // Situation
        txt = (C_Text *)win->FindControl(SIT_FIELD);

        if (txt)
        {
            txt->SetText(SituationStr[TheCampaign.Situation % 5]);
        }

        LoadSquadronInfo();
        btn = (C_Button *)win->FindControl(SET_CHALLENGE);

        if (btn)
        {
            btn->Refresh();
            btn->SetState(TEMP_Settings.Challenge);
            btn->Refresh();
        }
    }

    win = gMainHandler->FindWindow(CS_SUA_WIN);

    if (win)
    {
        clk = (C_Clock *)win->FindControl(TIME_ID);

        if (clk)
        {
            clk->SetTime(TheCampaign.CurrentTime / VU_TICS_PER_SECOND);
            clk->Refresh();
        }

    }

    SetupMapWindow();
    AddSquadronsToMap();
    EnableScenarioInfo(4050);
}

void EnableScenarioText(C_Base *control)
{
    C_Window *win;

    win = gMainHandler->FindWindow(CS_PUA_WIN);

    if (win)
    {
        if (control)
        {
            win->HideCluster(control->GetUserNumber(1));
            win->HideCluster(control->GetUserNumber(2));
            win->UnHideCluster(control->GetUserNumber(0));
        }
        else
        {
            win->HideCluster(2);
            win->HideCluster(3);
            win->UnHideCluster(1);
        }
    }
}

void CloseSelectWindowCB(long ID, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    DisableScenarioInfo();
    CloseWindowCB(ID, hittype, control);
}

void SelectScenarioButtons(long ID)
{
    C_Window *win;
    C_Button *btn;
    F4CSECTIONHANDLE *Leave;

    win = gMainHandler->FindWindow(CS_SELECT_WIN);

    if (win)
    {
        Leave = UI_Enter(win);
        btn = (C_Button*)win->FindControl(CS_LOAD_SCENARIO1);

        if (btn)
        {
            if (btn->GetID() == ID)
                btn->SetState(1);
            else
                btn->SetState(0);

            btn->Refresh();
        }

        btn = (C_Button*)win->FindControl(CS_LOAD_SCENARIO2);

        if (btn)
        {
            if (btn->GetID() == ID)
                btn->SetState(1);
            else
                btn->SetState(0);

            btn->Refresh();
        }

        btn = (C_Button*)win->FindControl(CS_LOAD_SCENARIO3);

        if (btn)
        {
            if (btn->GetID() == ID)
                btn->SetState(1);
            else
                btn->SetState(0);

            btn->Refresh();
        }

        UI_Leave(Leave);
    }
}

void SelectScenarioCB(long ID, short hittype, C_Base *control)
{
    int i;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    SelectScenarioButtons(ID);

    gLastCampFilename[0] = 0;

    switch (ID)
    {
        case CS_LOAD_SCENARIO1:
            strcpy(gUI_CampaignFile, "save0");
            _tcscpy(gUI_ScenarioName, gStringMgr->GetString(TXT_SCENARIO_1));
            break;

        case CS_LOAD_SCENARIO2:
            strcpy(gUI_CampaignFile, "save1");
            _tcscpy(gUI_ScenarioName, gStringMgr->GetString(TXT_SCENARIO_2));
            break;

        case CS_LOAD_SCENARIO3:
            strcpy(gUI_CampaignFile, "save2");
            _tcscpy(gUI_ScenarioName, gStringMgr->GetString(TXT_SCENARIO_3));
            break;

        default:
            return;
    }

    TheCampaign.LoadScenarioStats(game_Campaign, gUI_CampaignFile);
    // Pick the default squadron ID
    gSelectedSquadronID = -1;

    for (i = 0; i < TheCampaign.NumAvailSquadrons; i++)
    {
        if (TheCampaign.CampaignSquadronData[i].id == TheCampaign.PlayerSquadronID and (TheCampaign.IsValidSquadron(i) or _IsF16_))
            gSelectedSquadronID = i;
    }

    EnableScenarioText(control);

    //dpc ExitCampSelectFix
    //When exiting from Campaign module to Main UI if no mission was flown,
    //SAVED tab with last mission selected was shown, but Rolling Fire campaign
    //was actually loaded. This fix disables campaing info in that case, so
    //user has to select save file manually. This way it's not possible to start
    //wrong campaign.
    if (g_bExitCampSelectFix)
    {
        if (oldCampSelMode not_eq 1)
        {
            LoadScenarioInfo();
        }
    }
    else
        //end ExitCampSelectFix
        LoadScenarioInfo();

    if (control)
    {
        if (control->GetUserNumber(0))
        {
            PlayUIMovie(control->GetUserNumber(100));
        }
    }
}

void RecieveScenarioInfo()
{
    _TCHAR *name;
    int i;
    VuGameEntity * game;

    if (gMainHandler == NULL)
        return;

    // KCK: These really should be a better way to figure out which windows need updating than
    // checking the flag field of TheCampaign. Doesn't the UI know which window groups are active?
    if (TheCampaign.Flags bitand CAMP_LIGHT)
    {
        // Do dogfight/IA stuff
    }
    else if (TheCampaign.Flags bitand CAMP_TACTICAL)
    {
        game = gCommsMgr->GetTargetGame();

        if (game)
        {
            name = game->GameName();
            _tcscpy(gUI_ScenarioName, name);
        }

        gSelectedSquadronID = -1;

        for (i = 0; i < TheCampaign.NumAvailSquadrons; i++)
        {
            if (TheCampaign.CampaignSquadronData[i].id == TheCampaign.PlayerSquadronID and (TheCampaign.IsValidSquadron(i) or _IsF16_))
                gSelectedSquadronID = i;
        }

        // Update the campaign selection windows
        create_tactical_scenario_info();
    }
    else
    {
        game = gCommsMgr->GetTargetGame();

        if (game)
        {
            name = game->GameName();
            _tcscpy(gUI_ScenarioName, name);
        }

        gSelectedSquadronID = -1;

        for (i = 0; i < TheCampaign.NumAvailSquadrons; i++)
        {
            if (TheCampaign.CampaignSquadronData[i].id == TheCampaign.PlayerSquadronID and (TheCampaign.IsValidSquadron(i) or _IsF16_))
                gSelectedSquadronID = i;
        }

        // Update the campaign selection windows
        //dpc ExitCampSelectFix
        if (g_bExitCampSelectFix)
        {
            if (oldCampSelMode not_eq 1)
            {
                LoadScenarioInfo();
            }
        }
        else
            //end ExitCampSelectFix
            LoadScenarioInfo();
    }
}

static void ReallyCreateCB(void)
{
    // Load a Campaign
    StartCampaignGame(TRUE, game_Campaign);
}

static void ReallyJoinCB(void)
{
    DisplayJoinStatusWindow(0);

    StartCampaignGame(FALSE, game_Campaign);
}

void CancelJoinCB(void)
{
}

static void CommitCB(long, short hittype, C_Base *)
{
    FalconGameEntity *game = NULL;
    C_EditBox *ebox;
    C_Window *win;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    if (TheCampaign.NumAvailSquadrons < 1)
        return;

    SetCursor(gCursors[CRSR_WAIT]);
    LoadCampaignWindows();

    if ( not TheCampaign.CampaignSquadronData)
        return;

    FalconLocalSession->SetCountry(TheCampaign.CampaignSquadronData[gSelectedSquadronID].country);
    gPlayerSquadronId = TheCampaign.CampaignSquadronData[gSelectedSquadronID].id;

    if (CampSelMode not_eq 2)
    {
        if (gCommsMgr->Online())
            SetupInfoWindow(ReallyCreateCB, CancelJoinCB);
        else
            ReallyCreateCB();
    }
    else
    {
        // Join a Campaign
        game = (FalconGameEntity*)gCommsMgr->GetTargetGame();

        if ( not game)
            return;

        if (game not_eq FalconLocalGame)
        {
            win = gMainHandler->FindWindow(INFO_WIN);

            if (win)
            {
                ebox = (C_EditBox*)win->FindControl(INFO_PASSWORD);

                if (ebox)
                {
                    // if( not PlayerOptions.InCompliance(game->GetRules()) or not game->CheckPassword(ebox->GetText()))
                    // {
                    SetupInfoWindow(ReallyJoinCB, CancelJoinCB);
                    // }
                    // else
                    // ReallyJoinCB();
                }
            }
        }
        else
            ReallyJoinCB();
    }
}

static void CommsCommitCB(long ID, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    if (TheCampaign.NumAvailSquadrons < 1)
        return;

    if ( not gCommsMgr->Online())
        return;

    CommitCB(ID, hittype, control); // Do rest of NORMAL Commit button
}

static void PickAirbaseCB(long, short, C_Base *control)
{
    gSelectedSquadronID = -1;
    SetupMapSquadronWindow(control->GetX(), control->GetY());
}

static void PickSquadronCB(long ID, short, C_Base *)
{
    gSelectedSquadronID = ID - 1;
    LoadSquadronInfo();
}

static void HostCampaignCB(long, short hittype, C_Base *)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;
}

static void LoadCampaignFileCB(long, short hittype, C_Base *control)
{
    C_TreeList *tree;
    TREELIST *item;
    C_Button   *btn;

    int i;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    tree = (C_TreeList*)control;

    if (tree)
    {
        item = tree->GetLastItem();

        if (item)
        {
            btn = (C_Button*)item->Item_;

            if (btn)
            {
                tree->SetAllControlStates(0, tree->GetRoot());
                btn->SetState(1);
                tree->Refresh();
                _tcscpy(gUI_CampaignFile, btn->GetText(C_STATE_0));
                // MN 2002-02-04 removed ".cam" to be able to also delete .his .frc-files
                _stprintf(gLastCampFilename, "%s\\%s", FalconCampUserSaveDirectory, gUI_CampaignFile);
                TheCampaign.LoadScenarioStats(game_Campaign, gUI_CampaignFile);

                // Pick the last selected squadron
                for (i = 0; i < TheCampaign.NumAvailSquadrons; i++)
                {
                    if (TheCampaign.CampaignSquadronData[i].id == TheCampaign.PlayerSquadronID)
                        gSelectedSquadronID = i;
                }

                _tcscpy(gUI_ScenarioName, TheCampaign.SaveFile);
                LoadScenarioInfo();
            }
        }
    }
}

void SetCampaignSelectCB(long ID, short hittype, C_Base *control)
{
    C_Window *win;
    C_Slider *sldr;
    C_TreeList *tree;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    switch (ID)
    {
        case CS_NEW_CTRL:
            if ( not CampSelMode)
                return;

            break;

        case CS_LOAD_CTRL:

            //dpc CampSavedWindowRefreshHack
            //Enable refresh of Campaign load menu if CAMP.SAVED TAB is clicked.
            //This is a hack to enable user to refresh saved campaign list manually.
            //Ideally after exiting Campaing to Main UI this list should be refreshed automatically
            //and last played campaign selected. This is a compromise solution - last
            //choosen campaign is selected but user has to refresh list manually.
            if ( not g_bCampSavedMenuHack)
            {
                if (CampSelMode == 1)
                    return;
            }

            //end CampSavedWindowRefreshHack
            break;

        case CS_JOIN_CTRL:
            if (CampSelMode == 2)
                return;

            break;
    }

    SelectScenarioButtons(0); // Unselect any Scenario

    DisableScenarioInfo();

    if (control->Parent_)
    {
        control->Parent_->UnHideCluster(control->GetUserNumber(0));
        control->Parent_->HideCluster(control->GetUserNumber(1));
        control->Parent_->HideCluster(control->GetUserNumber(2));
    }

    control->Parent_->RefreshWindow();
    gLastCampFilename[0] = 0;

    switch (ID)
    {
        case CS_NEW_CTRL:
            CampSelMode = 0;
            break;

        case CS_LOAD_CTRL:
            CampSelMode = 1;
            tree = (C_TreeList*)control->Parent_->FindControl(FILELIST_TREE);

            if (tree)
            {
                tree->DeleteBranch(tree->GetRoot());
                tree->SetUserNumber(0, 1);
                tree->SetSortType(TREE_SORT_CALLBACK);
                tree->SetSortCallback(FileNameSortCB);
                tree->SetCallback(LoadCampaignFileCB);
                char path[_MAX_PATH];
                sprintf(path, "%s\\*.cam", FalconCampaignSaveDirectory);
                GetFileListTree(tree, path, CampExcludeList, C_TYPE_ITEM, TRUE, 0);
                tree->RecalcSize();

                if (tree->Parent_)
                    tree->Parent_->RefreshClient(tree->GetClient());
            }

            break;

        case CS_JOIN_CTRL:
            CampSelMode = 2;

            if ( not gCommsMgr->Online())
            {
                win = gMainHandler->FindWindow(PB_WIN);

                if (win)
                    gMainHandler->EnableWindowGroup(win->GetGroup());
            }

            break;
    }

    win = gMainHandler->FindWindow(CHALLENGE_WIN);

    if (win)
    {
        sldr = (C_Slider *)win->FindControl(BAR_1_SCROLL);

        if (sldr)
        {
            if (CampSelMode)
                sldr->SetFlagBitOff(C_BIT_ENABLED);
            else
                sldr->SetFlagBitOn(C_BIT_ENABLED);
        }

        sldr = (C_Slider *)win->FindControl(BAR_2_SCROLL);

        if (sldr)
        {
            if (CampSelMode)
                sldr->SetFlagBitOff(C_BIT_ENABLED);
            else
                sldr->SetFlagBitOn(C_BIT_ENABLED);
        }

        sldr = (C_Slider *)win->FindControl(BAR_3_SCROLL);

        if (sldr)
        {
            if (CampSelMode)
                sldr->SetFlagBitOff(C_BIT_ENABLED);
            else
                sldr->SetFlagBitOn(C_BIT_ENABLED);
        }

        sldr = (C_Slider *)win->FindControl(BAR_4_SCROLL);

        if (sldr)
        {
            if (CampSelMode)
                sldr->SetFlagBitOff(C_BIT_ENABLED);
            else
                sldr->SetFlagBitOn(C_BIT_ENABLED);
        }
    }

    //dpc ExitCampSelectFix
    if (g_bExitCampSelectFix)
        oldCampSelMode = CampSelMode;

    //end ExitCampSelectFix
}

// ONLY use this as a Tree Callback... or it will CRASH Badly
static void CampSelectGameCB(long, short hittype, C_Base *control)
{
    VU_ID *tmpID;
    FalconGameEntity *game;
    TREELIST *item;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    SetCursor(gCursors[CRSR_WAIT]);
    item = ((C_TreeList *)control)->GetLastItem();

    if (item == NULL) return;

    if (item->Item_ == NULL) return;

    if (item->Type_ == C_TYPE_MENU)
    {
        if ( not item->Item_->GetState())
        {
            item->Item_->SetState(1);
            item->Item_->Refresh();
            tmpID = (VU_ID *)item->Item_->GetUserPtr(_UI95_VU_ID_SLOT_);

            if (tmpID)
            {
                game = (FalconGameEntity*) vuDatabase->Find(*tmpID);
                gCommsMgr->LookAtGame(game);

                if (game)
                {
                    if (game->GetGameType() == game_Campaign)
                        SendMessage(gMainHandler->GetAppWnd(), FM_JOIN_CAMPAIGN, JOIN_PRELOAD_ONLY, game_Campaign);
                }
            }
        }
        else
        {
            item->Item_->SetState(0);
            item->Item_->Refresh();
            gCommsMgr->LookAtGame(NULL);
            DisableScenarioInfo();
        }
    }

    //SetCursor(gCursors[CRSR_F16]);
}

static void SetCampaignLevels()
{
    C_Window *win;
    C_Slider *sldr;
    C_ListBox *lb;
    long val;

    win = gMainHandler->FindWindow(CHALLENGE_WIN);

    if (win)
    {
        lb = (C_ListBox *)win->FindControl(CHALLENGE_LIST);

        if (lb)
        {
            lb->Refresh();

            switch (TEMP_Settings.Challenge)
            {
                case 1:
                    lb->SetValue(CADET_LEVEL);
                    break;

                case 2:
                    lb->SetValue(ROOKIE_LEVEL);
                    break;

                case 3:
                    lb->SetValue(VETERAN_LEVEL);
                    break;

                case 4:
                    lb->SetValue(ACE_LEVEL);
                    break;

                default:
                    lb->SetValue(RECRUIT_LEVEL);
                    break;
            }

            lb->Refresh();
        }

        lb = (C_ListBox *)win->FindControl(PILOT_SKILL);

        if (lb)
        {
            lb->Refresh();

            switch (TEMP_Settings.PilotSkill)
            {
                case 1:
                    lb->SetValue(CADET_LEVEL);
                    break;

                case 2:
                    lb->SetValue(ROOKIE_LEVEL);
                    break;

                case 3:
                    lb->SetValue(VETERAN_LEVEL);
                    break;

                case 4:
                    lb->SetValue(ACE_LEVEL);
                    break;

                default:
                    lb->SetValue(RECRUIT_LEVEL);
                    break;
            }

            lb->Refresh();
        }

        lb = (C_ListBox *)win->FindControl(SAM_SKILL);

        if (lb)
        {
            lb->Refresh();

            switch (TEMP_Settings.SAMSkill)
            {
                case 1:
                    lb->SetValue(CADET_LEVEL);
                    break;

                case 2:
                    lb->SetValue(ROOKIE_LEVEL);
                    break;

                case 3:
                    lb->SetValue(VETERAN_LEVEL);
                    break;

                case 4:
                    lb->SetValue(ACE_LEVEL);
                    break;

                default:
                    lb->SetValue(RECRUIT_LEVEL);
                    break;
            }

            lb->Refresh();
        }

        sldr = (C_Slider *)win->FindControl(BAR_1_SCROLL);

        if (sldr)
        {
            sldr->Refresh();
            val = ((sldr->GetSliderMax() - sldr->GetSliderMin()) * TEMP_Settings.AirForces) / 4;
            sldr->SetSliderPos(val + sldr->GetSliderMin());
            sldr->Refresh();
        }

        sldr = (C_Slider *)win->FindControl(BAR_2_SCROLL);

        if (sldr)
        {
            sldr->Refresh();
            val = ((sldr->GetSliderMax() - sldr->GetSliderMin()) * TEMP_Settings.AirDefenses) / 4;
            sldr->SetSliderPos(val + sldr->GetSliderMin());
            sldr->Refresh();
        }

        sldr = (C_Slider *)win->FindControl(BAR_3_SCROLL);

        if (sldr)
        {
            sldr->Refresh();
            val = ((sldr->GetSliderMax() - sldr->GetSliderMin()) * TEMP_Settings.GroundForces) / 4;
            sldr->SetSliderPos(val + sldr->GetSliderMin());
            sldr->Refresh();
        }

        sldr = (C_Slider *)win->FindControl(BAR_4_SCROLL);

        if (sldr)
        {
            sldr->Refresh();
            val = ((sldr->GetSliderMax() - sldr->GetSliderMin()) * TEMP_Settings.NavalForces) / 4;
            sldr->SetSliderPos(val + sldr->GetSliderMin());
            sldr->Refresh();
        }
    }
}

void CopySettingsToTemp(void)
{
    if ( not CampSelMode)
    {
        TEMP_Settings.PilotSkill = static_cast<uchar>(PlayerOptions.CampaignEnemyAirExperience());
        TEMP_Settings.SAMSkill = static_cast<uchar>(PlayerOptions.CampaignEnemyGroundExperience());

        TEMP_Settings.AirForces = static_cast<uchar>(PlayerOptions.CampaignAirRatio());
        TEMP_Settings.AirDefenses = static_cast<uchar>(PlayerOptions.CampaignAirDefenseRatio());
        TEMP_Settings.GroundForces = static_cast<uchar>(PlayerOptions.CampaignGroundRatio());
        TEMP_Settings.NavalForces = static_cast<uchar>(PlayerOptions.CampaignNavalRatio());
    }
    else
    {
        TEMP_Settings.PilotSkill = TheCampaign.EnemyAirExp;
        TEMP_Settings.SAMSkill = TheCampaign.EnemyADExp;

        TEMP_Settings.AirForces = static_cast<uchar>(TheCampaign.AirRatio);
        TEMP_Settings.AirDefenses = static_cast<uchar>(TheCampaign.AirDefenseRatio);
        TEMP_Settings.GroundForces = static_cast<uchar>(TheCampaign.GroundRatio);
        TEMP_Settings.NavalForces = static_cast<uchar>(TheCampaign.NavalRatio);
    }
}

static void ChallengeLevelCB(long, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_SELECT)
        return;

    switch (((C_ListBox *)control)->GetTextID())
    {
        case ACE_LEVEL:
            TEMP_Settings.Challenge = 4;
            TEMP_Settings.PilotSkill = 4;
            TEMP_Settings.SAMSkill = 4;

            if ( not CampSelMode)
            {
                TEMP_Settings.AirForces = 0;
                TEMP_Settings.AirDefenses = 0;
                TEMP_Settings.GroundForces = 0;
                TEMP_Settings.NavalForces = 0;
            }

            break;

        case VETERAN_LEVEL:
            TEMP_Settings.Challenge = 3;
            TEMP_Settings.PilotSkill = 3;
            TEMP_Settings.SAMSkill = 3;

            if ( not CampSelMode)
            {
                TEMP_Settings.AirForces = 1;
                TEMP_Settings.AirDefenses = 1;
                TEMP_Settings.GroundForces = 1;
                TEMP_Settings.NavalForces = 1;
            }

            break;

        case ROOKIE_LEVEL:
            TEMP_Settings.Challenge = 2;
            TEMP_Settings.PilotSkill = 2;
            TEMP_Settings.SAMSkill = 2;

            if ( not CampSelMode)
            {
                TEMP_Settings.AirForces = 2;
                TEMP_Settings.AirDefenses = 2;
                TEMP_Settings.GroundForces = 2;
                TEMP_Settings.NavalForces = 2;
            }

            break;

        case CADET_LEVEL:
            TEMP_Settings.Challenge = 1;
            TEMP_Settings.PilotSkill = 1;
            TEMP_Settings.SAMSkill = 1;

            if ( not CampSelMode)
            {
                TEMP_Settings.AirForces = 3;
                TEMP_Settings.AirDefenses = 3;
                TEMP_Settings.GroundForces = 3;
                TEMP_Settings.NavalForces = 3;
            }

            break;

        default:
            TEMP_Settings.Challenge = 0;
            TEMP_Settings.PilotSkill = 0;
            TEMP_Settings.SAMSkill = 0;

            if ( not CampSelMode)
            {
                TEMP_Settings.AirForces = 4;
                TEMP_Settings.AirDefenses = 4;
                TEMP_Settings.GroundForces = 4;
                TEMP_Settings.NavalForces = 4;
            }

            break;
    }

    SetCampaignLevels();
    CalcChallengeLevel();
}

void OpenChallengeCB(long, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    // Copy in current settings
    CopySettingsToTemp();
    CalcChallengeLevel();
    SetCampaignLevels();

    gMainHandler->EnableWindowGroup(control->GetGroup());
}

void CloseChallengeCB(long ID, short hittype, C_Base *control)
{
    CopySettingsToTemp();
    CloseWindowCB(ID, hittype, control);
}

void CampDelFileCB(long, short hittype, C_Base *control)
{
    C_TreeList *tree;
    C_Window *win;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    win = gMainHandler->FindWindow(CS_SELECT_WIN);

    if ( not win)
        return;

    gMainHandler->HideWindow(control->Parent_); // Close Verify Window

    // 2002-02-04 MN modified to also delete .his/.frc file
    _TCHAR filename[MAX_PATH];
    _stprintf(filename, "%s.cam", gLastCampFilename);

    if ( not CheckExclude(filename, FalconCampUserSaveDirectory, CampExcludeList, "cam"))
    {
        DeleteFile(filename); // .cam file
        _stprintf(filename, "%s.his", gLastCampFilename);
        DeleteFile(filename);
        _stprintf(filename, "%s.frc", gLastCampFilename);
        DeleteFile(filename);
        gLastCampFilename[0] = 0;
    }

    tree = (C_TreeList*)win->FindControl(FILELIST_TREE);

    if (tree)
    {
        char path[_MAX_PATH];
        sprintf(path, "%s\\*.cam", FalconCampaignSaveDirectory);
        tree->DeleteBranch(tree->GetRoot());
        GetFileListTree(tree, path, CampExcludeList, C_TYPE_ITEM, TRUE, 0);
        tree->RecalcSize();

        if (tree->Parent_)
            tree->Parent_->RefreshClient(tree->GetClient());
    }

    DisableScenarioInfo();
}

static void CampDelVerifyCB(long, short hittype, C_Base *)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    if (gLastCampFilename[0])
        VerifyDelete(0, CampDelFileCB, CloseWindowCB);
}

static void SetSkillSettingsCB(long ID, short hittype, C_Base *control)
{
    uchar value;

    if (hittype not_eq C_TYPE_SELECT)
        return;

    switch (((C_ListBox *)control)->GetTextID())
    {
        case CADET_LEVEL:
            value = 1;
            break;

        case ROOKIE_LEVEL:
            value = 2;
            break;

        case VETERAN_LEVEL:
            value = 3;
            break;

        case ACE_LEVEL:
            value = 4;
            break;

        default:
            value = 0;
            break;
    }

    switch (ID)
    {
        case PILOT_SKILL:
            TEMP_Settings.PilotSkill = value;
            break;

        case SAM_SKILL:
            TEMP_Settings.SAMSkill = value;
            break;
    }

    CalcChallengeLevel();
}

static void SetSliderSettingsCB(long ID, short hittype, C_Base *control)
{
    long val;
    C_Slider *sldr;

    if (hittype not_eq C_TYPE_MOUSEMOVE)
        return;

    sldr = (C_Slider *)control;

    val  = (sldr->GetSliderPos() - sldr->GetSliderMin()) * 4;
    val /= (sldr->GetSliderMax() - sldr->GetSliderMin());

    switch (ID)
    {
        case BAR_1_SCROLL:
            if (val not_eq TEMP_Settings.AirForces)
            {
                TEMP_Settings.AirForces = static_cast<uchar>(val);
                // Force the aircraft field to update
                LoadSquadronInfo();
            }

            break;

        case BAR_2_SCROLL:
            TEMP_Settings.AirDefenses = static_cast<uchar>(val);
            break;

        case BAR_3_SCROLL:
            TEMP_Settings.GroundForces = static_cast<uchar>(val);
            break;

        case BAR_4_SCROLL:
            TEMP_Settings.NavalForces = static_cast<uchar>(val);
            break;

        default:
            return;
    }

    CalcChallengeLevel();
}

static void UseChallengeSettingsCB(long, short hittype, C_Base *control)
{
    C_Window *win;
    C_Button *btn;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    if (CampSelMode not_eq 2)
    {
        PlayerOptions.SetCampEnemyAirExperience(TEMP_Settings.PilotSkill);
        PlayerOptions.SetCampEnemyGroundExperience(TEMP_Settings.SAMSkill);
    }

    if ( not CampSelMode)
    {
        PlayerOptions.SetCampAirRatio(TEMP_Settings.AirForces);
        PlayerOptions.SetCampAirDefenseRatio(TEMP_Settings.AirDefenses);
        PlayerOptions.SetCampGroundRatio(TEMP_Settings.GroundForces);
        PlayerOptions.SetCampNavalRatio(TEMP_Settings.NavalForces);
    }

    win = gMainHandler->FindWindow(CS_PUA_WIN);

    if (win)
    {
        btn = (C_Button *)win->FindControl(SET_CHALLENGE);

        if (btn)
        {
            btn->Refresh();
            btn->SetState(TEMP_Settings.Challenge);
            btn->Refresh();
        }
    }

    LoadSquadronInfo();
    gMainHandler->DisableWindowGroup(control->GetGroup());
}

void CopyinTempSettings(void)
{
    TheCampaign.GroundRatio = TEMP_Settings.GroundForces;
    TheCampaign.AirRatio = TEMP_Settings.AirForces;
    TheCampaign.AirDefenseRatio = TEMP_Settings.AirDefenses;
    TheCampaign.NavalRatio = TEMP_Settings.NavalForces;
    TheCampaign.EnemyAirExp = TEMP_Settings.PilotSkill;
    TheCampaign.EnemyADExp = TEMP_Settings.SAMSkill;
}

static void HookupCampaignSelectControls(long ID)
{
    C_Window *winme;
    C_Button *ctrl;
    C_ListBox *lb;
    C_Slider *sldr;
    C_TreeList *tree;

    winme = gMainHandler->FindWindow(ID);

    if (winme == NULL)
        return;

    // Hook up IDs here

    // Set Commit Button
    ctrl = (C_Button *)winme->FindControl(SINGLE_COMMIT_CTRL);

    if (ctrl)
        ctrl->SetCallback(CommitCB);

    // Set Commit Button
    ctrl = (C_Button *)winme->FindControl(CS_DELETE);

    if (ctrl)
        ctrl->SetCallback(CampDelVerifyCB);

    ctrl = (C_Button *)winme->FindControl(COMMS_COMMIT_CTRL);

    if (ctrl)
        ctrl->SetCallback(CommsCommitCB);

    // Hook up Close Button
    ctrl = (C_Button *)winme->FindControl(CLOSE_WINDOW);

    if (ctrl)
        ctrl->SetCallback(CloseWindowCB);

    ctrl = (C_Button *)winme->FindControl(SELECT_CLOSE_WINDOW);

    if (ctrl)
        ctrl->SetCallback(CloseSelectWindowCB);

    ctrl = (C_Button *)winme->FindControl(CS_LOAD_SCENARIO1);

    if (ctrl)
        ctrl->SetCallback(SelectScenarioCB);

    ctrl = (C_Button *)winme->FindControl(CS_LOAD_SCENARIO2);

    if (ctrl)
        ctrl->SetCallback(SelectScenarioCB);

    ctrl = (C_Button *)winme->FindControl(CS_LOAD_SCENARIO3);

    if (ctrl)
        ctrl->SetCallback(SelectScenarioCB);

    ctrl = (C_Button *)winme->FindControl(CS_HOST_CTRL);

    if (ctrl)
        ctrl->SetCallback(HostCampaignCB);

    ctrl = (C_Button *)winme->FindControl(CS_NEW_CTRL);

    if (ctrl)
        ctrl->SetCallback(SetCampaignSelectCB);

    ctrl = (C_Button *)winme->FindControl(CS_LOAD_CTRL);

    if (ctrl)
        ctrl->SetCallback(SetCampaignSelectCB);

    ctrl = (C_Button *)winme->FindControl(CS_JOIN_CTRL);

    if (ctrl)
    {
        ctrl->SetCallback(SetCampaignSelectCB);
    }

    lb = (C_ListBox *)winme->FindControl(CHALLENGE_LIST);

    if (lb)
        lb->SetCallback(ChallengeLevelCB);

    lb = (C_ListBox *)winme->FindControl(PILOT_SKILL);

    if (lb)
        lb->SetCallback(SetSkillSettingsCB);

    lb = (C_ListBox *)winme->FindControl(SAM_SKILL);

    if (lb)
        lb->SetCallback(SetSkillSettingsCB);

    ctrl = (C_Button *)winme->FindControl(SET_CHALLENGE);

    if (ctrl)
        ctrl->SetCallback(OpenChallengeCB);

    ctrl = (C_Button *)winme->FindControl(CHALL_OK);

    if (ctrl)
        ctrl->SetCallback(UseChallengeSettingsCB);

    ctrl = (C_Button *)winme->FindControl(CHALL_CANCEL);

    if (ctrl)
        ctrl->SetCallback(CloseChallengeCB);

    sldr = (C_Slider *)winme->FindControl(BAR_1_SCROLL);

    if (sldr)
        sldr->SetCallback(SetSliderSettingsCB);

    sldr = (C_Slider *)winme->FindControl(BAR_2_SCROLL);

    if (sldr)
        sldr->SetCallback(SetSliderSettingsCB);

    sldr = (C_Slider *)winme->FindControl(BAR_3_SCROLL);

    if (sldr)
        sldr->SetCallback(SetSliderSettingsCB);

    sldr = (C_Slider *)winme->FindControl(BAR_4_SCROLL);

    if (sldr)
        sldr->SetCallback(SetSliderSettingsCB);

    ctrl = (C_Button *)winme->FindControl(CS_INFO_CTRL);

    if (ctrl)
        ctrl->SetCallback(InfoButtonCB);

    tree = (C_TreeList *)winme->FindControl(CAMPAIGN_TREE);

    if (tree)
    {
        CampaignGames = tree;
        CampaignGames->SetCallback(CampSelectGameCB);
    }

    // Help GUIDE thing
    ctrl = (C_Button*)winme->FindControl(UI_HELP_GUIDE);

    if (ctrl)
        ctrl->SetCallback(UI_Help_Guide_CB);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static void JoinStatusCancelCB(long, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    MonoPrint("Cancel Join\n");

    gMainHandler->HideWindow(control->Parent_);

    StopCampaignLoad();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static int
join_status_bits = 0;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
/*
static void UpdateJoinStatusWindow (C_Window *win)
{
 short
 x,
 y;

 char
 buffer[10];

 sprintf (buffer, "%08x", join_status_bits);

 x=0; y=0;
 DeleteGroupList(win->GetID());
 AddWordWrapTextToWindow
 (
 win,
 &x,
 &y,
 0,
 win->ClientArea_[1].right-win->ClientArea_[1].left,
 0xe0e0e0,
 buffer,
 1
 );
}
*/
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void DisplayJoinStatusWindow(int bits)
{
    static long
    last_id;

    long
    id = 0;//uninitialized

    C_Window
    *win;

    if (( not gCommsMgr->Online()) or (FalconLocalGame->IsLocal()))
    {
        return;
    }

    if (TheCampaign.IsLoaded())
    {
        return;
    }

    if (bits == 0)
    {
        join_status_bits = 0;
    }
    else
    {
        join_status_bits or_eq bits;
    }

    switch (bits)
    {
        case CAMP_GAME_FULL:
            id = TXT_GAME_IS_OVER;
            break;

        case 0:
        case CAMP_NEED_PRELOAD:
            id = TXT_JOIN_STARTING;
            break;

        case CAMP_NEED_ENTITIES:
        case CAMP_NEED_WEATHER:
            id = TXT_JOIN_WEATHER;
            break;

        case CAMP_NEED_PERSIST:
        case CAMP_NEED_OBJ_DELTAS:
            id = TXT_JOIN_OBJECTS;
            break;

        case CAMP_NEED_TEAM_DATA:
        case CAMP_NEED_UNIT_DATA:
        case CAMP_NEED_VC:
        case CAMP_NEED_PRIORITIES:
            id = TXT_JOIN_UNITS;
            break;
    }

    if (id not_eq last_id)
    {
        win = gMainHandler->FindWindow(COMMLINK_WIN);

        if (win)
        {
            gMainHandler->HideWindow(win);
        }

        last_id = id;
    }

    CommsErrorDialog(TXT_JOINING_GAME, id, NULL, JoinStatusCancelCB);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
