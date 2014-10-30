#define _USE_MOVIE_ 1
/***************************************************************************\
 UI_Main.cpp
 Peter Ward
 December 3, 1996

 Main UI screen stuff for FreeFalcon
\***************************************************************************/
#include <cISO646>
#include <windows.h>
#include "falclib.h"

#ifdef USE_SH_POOLS
#include "SmartHeap/Include/smrtheap.hpp"
#endif

#include "targa.h"
#include "dxutil/ddutil.h"
#include "Graphics/Include/imagebuf.h"
#include "Graphics/Include/drawBSP.h"
#include "dispcfg.h"
#include "Graphics/Include/setup.h"
#include "Graphics/Include/TexBank.h"
#include "Graphics/Include/TerrTex.h"
#include "Graphics/Include/FarTex.h"
#include "chandler.h"
#include "ui95_ext.h"
#include "cmusic.h"
#include "entity.h"
#include "feature.h"
#include "vehicle.h"
#include "falcgame.h"
#include "CmpClass.h"
#include "division.h"
#include "evtparse.h"
#include "Mesg.h"
#include "MsgInc/DamageMsg.h"
#include "MsgInc/WeaponFireMsg.h"
#include "MsgInc/DeathMessage.h"
#include "MsgInc/MissileEndMsg.h"
#include "MsgInc/LandingMessage.h"
#include "find.h"
#include "flight.h"
#include "falcuser.h"
#include "falclib/include/f4find.h"
#include "f4error.h"
#include "cphoneb.h"
#include "uicomms.h"
#include "queue.h"
#include "ui_ia.h"
#include "cmap.h"
#include "userids.h"
#include "textids.h"
#include "Graphics/Include/matrix.h"
#include "cbsplist.h"
#include "c3dview.h"
#include "sim/include/inpFunc.h"
#include "sim/include/ascii.h"
#include "Falclib/Include/UI.h"
#include "te_include.h"
#include "PlayerOp.h"
#include "logbook.h"
#include "campmiss.h"
#include "resource.h"
#include "rules.h"
#include "teamdata.h"
#include "MissEval.h"
#include "sim/include/PilotInputs.h"
#include "DispOpts.h"

#include "sim/include/OTWDrive.h"

extern OTWDriverClass OTWDriver; // JB 010615
extern bool g_bHiResUI; // M.N. 2001-11-20

//JAM 18Nov03
#include "Weather.h"
const int numWeatherConditions = INCLEMENT;

#include "sim/include/IVibeData.h"
extern IntellivibeData g_intellivibeData;
extern void *gSharedIntellivibe;

unsigned char TestString1[] =
{
    192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 207, 209, 210, 211, 212, 213, 214, 0
};

unsigned char  TestString2[] =
{
    215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 0
};

unsigned char  TestString3[] =
{
    238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255, 0
};

enum
{
    SND_PIECE_1   = 600001,
    SND_PIECE_2   = 600002,
    SND_PIECE_3   = 600003,
    SND_PIECE_4   = 600004,
    SND_PIECE_5   = 600005,
    SND_PIECE_6   = 600006,
    SND_PIECE_7   = 600007,
    SND_PIECE_8   = 600008,
    SND_PIECE_9   = 600009,
    SND_PIECE_10  = 600010,
    SND_PIECE_11  = 600011,
    SND_PIECE_12  = 600012,
    SND_PIECE_13  = 600013,
    SND_PIECE_14  = 600014,
    SND_PIECE_15  = 600015,
    SND_PIECE_16  = 600016,
    SND_PIECE_17  = 600017,
    SND_PIECE_18  = 600018,
    SND_PIECE_19  = 600019,
    SND_PIECE_20  = 600020,
    SND_PIECE_21  = 600021,
    SND_PIECE_22  = 600022,
    SND_PIECE_23  = 600023,
    SND_PIECE_24  = 600024,
    SND_PIECE_25  = 600025,
    SND_PIECE_26  = 600026,
    SND_PIECE_27  = 600027,
    SND_PIECE_28  = 600028,
    SND_PIECE_29  = 600029,
    SND_PIECE_30  = 600030,
    SND_PIECE_31  = 600031,
    SND_PIECE_32  = 600032,
    SND_PIECE_33  = 600033,
    SND_PIECE_34  = 600034,
    SND_PIECE_35  = 600035,
    SND_PIECE_36  = 600036,
    SND_PIECE_37  = 600037,
    SND_PIECE_38  = 600038,
    SND_PIECE_39  = 600039,
    SND_PIECE_40  = 600040,
    SND_PIECE_41  = 600041,
    SND_PIECE_42  = 600042,
    SND_PIECE_43  = 600043,
    SND_PIECE_44  = 600044,
    SND_PIECE_45  = 600045,
    SND_PIECE_46  = 600046,
    SND_PIECE_47  = 600047,
    SND_PIECE_48  = 600048,
    SND_PIECE_49  = 600049,
    SND_PIECE_50  = 600050,
    SND_PIECE_51  = 600051,
    SND_PIECE_52  = 600052,
    SND_PIECE_53  = 600053,
    SND_PIECE_54  = 600054,
    SND_PIECE_55  = 600055,
    SND_PIECE_56  = 600056,
    SND_PIECE_57  = 600057,
    SND_PIECE_58  = 600058,
    SND_PIECE_59  = 600059,
    SND_PIECE_60  = 600060,
    SND_PIECE_61  = 600061,
    SND_PIECE_62  = 600062,
    SND_PIECE_63  = 600063,
    SND_PIECE_64  = 600064,
    SND_PIECE_65  = 600065,
    SND_PIECE_66  = 600066,
    SND_PIECE_67  = 600067,
    SND_PIECE_68  = 600068,
    SND_PIECE_69  = 600069,
    SND_PIECE_70  = 600070,
    SND_PIECE_71  = 600071,
    SND_PIECE_74  = 600074,
    SND_PIECE_75  = 600075,
    SND_PIECE_76  = 600076,
    SND_PIECE_77  = 600077,
};

// HACK Structure for loading interactive music
typedef struct
{
    long Section;
    long Group;
    long MusicID;
} INTER_MUSIC;

INTER_MUSIC IntList[] =
{
    C_STATE_0, C_STATE_0, SND_PIECE_1,
    C_STATE_0, C_STATE_0, SND_PIECE_2,
    C_STATE_0, C_STATE_0, SND_PIECE_3,
    C_STATE_0, C_STATE_0, SND_PIECE_4,
    C_STATE_0, C_STATE_0, SND_PIECE_5,
    C_STATE_0, C_STATE_0, SND_PIECE_6,
    C_STATE_0, C_STATE_1, SND_PIECE_7,
    C_STATE_0, C_STATE_1, SND_PIECE_8,
    C_STATE_0, C_STATE_1, SND_PIECE_9,
    C_STATE_0, C_STATE_1, SND_PIECE_10,
    C_STATE_0, C_STATE_1, SND_PIECE_11,
    C_STATE_0, C_STATE_1, SND_PIECE_12,
    C_STATE_0, C_STATE_2, SND_PIECE_13,
    C_STATE_0, C_STATE_2, SND_PIECE_14,
    C_STATE_0, C_STATE_2, SND_PIECE_15,
    C_STATE_0, C_STATE_2, SND_PIECE_16,
    C_STATE_0, C_STATE_2, SND_PIECE_17,
    C_STATE_0, C_STATE_2, SND_PIECE_18,
    C_STATE_0, C_STATE_3, SND_PIECE_19,
    C_STATE_0, C_STATE_3, SND_PIECE_20,
    C_STATE_0, C_STATE_3, SND_PIECE_21,
    C_STATE_0, C_STATE_3, SND_PIECE_22,
    C_STATE_0, C_STATE_3, SND_PIECE_23,
    C_STATE_0, C_STATE_3, SND_PIECE_24,
    C_STATE_0, C_STATE_4, SND_PIECE_25,
    C_STATE_0, C_STATE_4, SND_PIECE_26,
    C_STATE_0, C_STATE_4, SND_PIECE_27,
    C_STATE_0, C_STATE_4, SND_PIECE_28,
    C_STATE_0, C_STATE_4, SND_PIECE_29,
    C_STATE_0, C_STATE_4, SND_PIECE_30,
    C_STATE_1, C_STATE_0, SND_PIECE_31,
    C_STATE_1, C_STATE_0, SND_PIECE_32,
    C_STATE_1, C_STATE_0, SND_PIECE_33,
    C_STATE_1, C_STATE_0, SND_PIECE_34,
    C_STATE_1, C_STATE_0, SND_PIECE_35,
    C_STATE_1, C_STATE_0, SND_PIECE_36,
    C_STATE_1, C_STATE_0, SND_PIECE_37,
    C_STATE_1, C_STATE_0, SND_PIECE_38,
    C_STATE_1, C_STATE_0, SND_PIECE_39,
    C_STATE_1, C_STATE_0, SND_PIECE_40,
    C_STATE_1, C_STATE_0, SND_PIECE_41,
    C_STATE_1, C_STATE_0, SND_PIECE_42,
    C_STATE_1, C_STATE_0, SND_PIECE_43,
    C_STATE_1, C_STATE_0, SND_PIECE_44,
    C_STATE_1, C_STATE_0, SND_PIECE_45,
    C_STATE_1, C_STATE_0, SND_PIECE_46,
    C_STATE_1, C_STATE_1, SND_PIECE_47,
    C_STATE_1, C_STATE_1, SND_PIECE_48,
    C_STATE_1, C_STATE_1, SND_PIECE_49,
    C_STATE_1, C_STATE_1, SND_PIECE_50,
    C_STATE_1, C_STATE_1, SND_PIECE_51,
    C_STATE_1, C_STATE_1, SND_PIECE_52,
    C_STATE_1, C_STATE_2, SND_PIECE_53,
    C_STATE_1, C_STATE_2, SND_PIECE_54,
    C_STATE_1, C_STATE_2, SND_PIECE_55,
    C_STATE_1, C_STATE_2, SND_PIECE_56,
    C_STATE_1, C_STATE_2, SND_PIECE_57,
    C_STATE_1, C_STATE_2, SND_PIECE_58,
    C_STATE_1, C_STATE_2, SND_PIECE_59,
    C_STATE_1, C_STATE_2, SND_PIECE_60,
    C_STATE_1, C_STATE_2, SND_PIECE_61,
    C_STATE_1, C_STATE_2, SND_PIECE_62,
    C_STATE_1, C_STATE_2, SND_PIECE_63,
    C_STATE_1, C_STATE_2, SND_PIECE_64,
    C_STATE_1, C_STATE_2, SND_PIECE_65,
    C_STATE_1, C_STATE_3, SND_PIECE_66,
    C_STATE_1, C_STATE_3, SND_PIECE_67,
    C_STATE_1, C_STATE_3, SND_PIECE_68,
    C_STATE_1, C_STATE_3, SND_PIECE_69,
    C_STATE_1, C_STATE_3, SND_PIECE_70,
    C_STATE_1, C_STATE_3, SND_PIECE_71,
    C_STATE_1, C_STATE_4, SND_PIECE_74,
    C_STATE_1, C_STATE_4, SND_PIECE_75,
    C_STATE_1, C_STATE_4, SND_PIECE_76,
    C_STATE_1, C_STATE_4, SND_PIECE_77,
    0, 0, 0,
};

// Smart Heap Pools for the UI
#ifdef USE_SH_POOLS
MEM_POOL UI_Pools[UI_MAX_POOLS];
MEM_BOOL MEM_CALLBACK errPrint(MEM_ERROR_INFO *errorInfo);
extern MEM_ERROR_FN lastErrorFn;
#endif

#define _USE_REGISTRY_ 1
// 30 Second Delay between music playing
#define MUSIC_DELAY (60l * 1000l)

extern void UpdateDFPlayerList();
C_Handler *gMainHandler = NULL;
extern C_Music *gMusic;
extern BOOL gNewMessage;
extern C_Map *gMapMgr;
extern IMAGE_RSC *gOccupationMap;
extern CSoundMgr *gSoundDriver;
extern char *gUBuffer;
extern WORD *gScreenShotBuffer;
extern long gScreenShotEnabled;
extern long MusicStopped; // Delay between music

// M.N.
extern bool g_bHiResUI;

extern HINSTANCE hInst;

char gUI_AutoSaveName[MAX_PATH];
extern long gRanksTxt[NUM_RANKS];

extern char **KeyDescrips;
void CleanupKeys(void);
extern PhoneBook *gPlayerBook;
extern C_TreeList *People, *DogfightGames, *TacticalGames, *CampaignGames;
extern C_SoundBite *gInstantBites, *gDogfightBites, *gCampaignBites;

void InitFontTool();

void LoadHelpGuideWindows();
void GenericTimerCB(long ID, short hittype, C_Base *control);
void UI_Help_Guide_CB(long ID, short hittype, C_Base *ctrl);
void LoadInstantActionWindows();
void LoadCampaignSelectWindows();
void LoadCampaignWindows();
void LoadTacticalWindows();
void ValidateRackData();
void LoadPlannerWindows();
void LoadDogFightWindows();
void LoadSetupWindows();
void LoadTacticalReferenceWindows();
void LoadTacEngSelectWindows();
void LoadPeopleInfo(long client);
void LoadCommsWindows();
void CreateFontCB(long ID, short hittype, C_Base *control);
void CreateTheFontCB(long ID, short hittype, C_Base *control);
void SaveFontCB(long ID, short hittype, C_Base *control);
void HookupControls(long ID);
void HookupDogFightMenus();
void HookupCampaignMenus();
void UI_Cleanup();
void UIBuildColorTable();
void SetupMapWindow();
void RealLoadLogbook(); // without daves extra garbage
void SelectScenarioCB(long ID, short hittype, C_Base *control);
void CampaignListCB();
void CampaignSoundEventCB();
void SetCampaignSelectCB(long ID, short hittype, C_Base *control);
void CampaignUpdateMapCB();
BOOL TacRef_Setup();
void CampaignSetup();
void ShutdownSetup();
void CloseWindowCB(long ID, short hittype, C_Base *control);
void OpenTacticalReferenceCB(long ID, short hittype, C_Base *control);
void EndDogfightCB(long ID, short hittype, C_Base *control);
void LeaveDogfight();
void ChooseFontCB(long ID, short hittype, C_Base *control);
void gMusicCallback(SOUNDSTREAM *Stream, int MessageID);
void ExitVerify(long TitleID, void (*OkCB)(long, short, C_Base*), void (*CancelCB)(long, short, C_Base*));
void CheckPasswordCB(long ID, short hittype, C_Base *control);
void PasswordWindow(long TitleID, long MessageID, void (*YesCB)(long, short, C_Base*), void (*NoCB)(long, short, C_Base*));
void NoPasswordCB(long ID, short hittype, C_Base *control);
void DoResultsWindows(void);
int LoadAllRules(char *filename);
//void AwardWindow(void);
//void CourtMartialWindow(void);
//void PromotionWindow(void);
void INFOSetupControls(void);
void ACMIButtonCB(long ID, short hittype, C_Base *control);
void TheaterButtonCB(long ID, short hittype, C_Base *control);
void IncreaseLead(long ID, short hittype, C_Base *control);
void DecreaseLead(long ID, short hittype, C_Base *control);
void IncreaseTrail(long ID, short hittype, C_Base *control);
void DecreaseTrail(long ID, short hittype, C_Base *control);
void IncreaseWidth(long ID, short hittype, C_Base *control);
void DecreaseWidth(long ID, short hittype, C_Base *control);
void IncreaseKern(long ID, short hittype, C_Base *control);
void DecreaseKern(long ID, short hittype, C_Base *control);
void RebuildGameTree();
void GenericCloseWindowCB(long ID, short hittype, C_Base *control);
void ActivateCampMissionSchedule();
void CloseReconWindowCB(long ID, short hittype, C_Base *control);
void CloseMunitionsWindowCB(long ID, short hittype, C_Base *control);
void CloseSetupWindowCB(long ID, short hittype, C_Base *control);
void CloseACMI();
void ACMICloseCB(long ID, short hittype, C_Base *control);
void TACREFCloseWindowCB(long ID, short hittype, C_Base *control);

//void NewLogbookCB(long ID,short hittype,C_Base *control);
//void ClearLogBookCB(long ID,short hittype,C_Base *control);
//void SaveLogBookCB(long ID,short hittype,C_Base *control);
void OpenLogBookCB(long ID, short hittype, C_Base *control);
BOOL gMoviePlaying = FALSE;
#define DF_CLOSE_CTRL         21873

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Tactical Engagement Stuff

void restart_tactical_engagement();

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern int LogState;
char gTrackBuffer[64];

HCURSOR gCursors[MAX_CURSORS];

C_Parser *gMainParser = NULL;

VuThread *UI_VuThread = NULL;

int MainLastGroup = 0, CampaignLastGroup = 0;

long AmbientStreamID = -1;

int IALoaded = 0;
int ACMILoaded = 0;
int DFLoaded = 0;
int TACLoaded = 0;
int CPLoaded = 0;
int CPSelectLoaded = 0;
int LBLoaded = 0;
int STPLoaded = 0;
int COLoaded = 0;
int MainLoaded = 0;
int PlannerLoaded = 0;
int TACREFLoaded = 0;
int CommonLoaded = 0;
int INFOLoaded = 0;
int HelpLoaded = 0;
int TACSelLoaded = 0;
enum
{
    SND_FLY   = 500003,
    SND_SCREAM        = 500005,
    SND_BAD1          = 500006,
    SND_SECOND        = 500007,
    SND_FIRST         = 500008,
    SND_NICE          = 500009,
    SND_BAD2          = 500010,
    SND_YOUSUCK       = 500011,
    SND_TAKEOFF   = 500023,
    SND_CAMPAIGN   = 500024,
    SND_LIBYA   = 500025,
    SND_AMBIENT1   = 500051,
    SND_AMBIENT2   = 500052,
    SND_CAMPAIGN_GOOD = 500053,
    SND_CAMPAIGN_MEDIUM = 500054,
    SND_CAMPAIGN_BAD = 500055,
};

static char *List1[] =
{
    "peterw",
    "charlesw",
    "joes",
    "leonr",
    NULL,
};

static char *List2[] =
{
    "jakeh",
    "jacobh",
    "bills",
    NULL,
};

static char *List3[] =
{
    NULL,
};

void CloseAllRenderers(long openID)
{
    C_Window *win;
    C_Button *btn;

    if (gMainHandler->GetWindowFlags(openID) bitand C_BIT_ENABLED)
        return;

    gMainHandler->EnterCritical();

    if (openID not_eq RECON_WIN and openID not_eq RECON_LIST_WIN)
    {
        win = gMainHandler->FindWindow(RECON_WIN);

        if (win and (gMainHandler->GetWindowFlags(RECON_WIN) bitand C_BIT_ENABLED))
        {
            btn = (C_Button*)win->FindControl(CLOSE_WINDOW);

            if (btn)
                CloseReconWindowCB(CLOSE_WINDOW, C_TYPE_LMOUSEUP, btn);
        }

        win = gMainHandler->FindWindow(RECON_LIST_WIN);

        if (win and (gMainHandler->GetWindowFlags(RECON_LIST_WIN) bitand C_BIT_ENABLED))
        {
            btn = (C_Button*)win->FindControl(CLOSE_WINDOW);

            if (btn)
                CloseReconWindowCB(CLOSE_WINDOW, C_TYPE_LMOUSEUP, btn);
        }
    }

    win = gMainHandler->FindWindow(MUNITIONS_WIN);

    if (win and (gMainHandler->GetWindowFlags(MUNITIONS_WIN) bitand C_BIT_ENABLED))
    {
        btn = (C_Button*)win->FindControl(CLOSE_WINDOW);

        if (btn)
            CloseMunitionsWindowCB(CLOSE_WINDOW, C_TYPE_LMOUSEUP, btn);
    }

    win = gMainHandler->FindWindow(SETUP_WIN);

    if (win)
    {
        btn = (C_Button*)win->FindControl(CLOSE_WINDOW);

        if (btn)
            CloseSetupWindowCB(CLOSE_WINDOW, C_TYPE_LMOUSEUP, btn);
    }

    win = gMainHandler->FindWindow(TAC_REF_WIN);

    if (win)
    {
        btn = (C_Button*)win->FindControl(CLOSE_WINDOW);

        if (btn)
            TACREFCloseWindowCB(CLOSE_WINDOW, C_TYPE_LMOUSEUP, btn);
    }

    CloseACMI();
    gMainHandler->LeaveCritical();
}

void LeaveCurrentGame()
{
    // KCK: This needs to NOT be here.
    // if ( not FalconLocalGame)
    // return;

    switch (FalconLocalGame->GetGameType())
    {
        case game_Dogfight:
            SendMessage(FalconDisplay.appWin, FM_SHUTDOWN_CAMPAIGN, 0, 0);
            LeaveDogfight();
            break;

        case game_InstantAction:
        case game_TacticalEngagement:
        default:
            SendMessage(FalconDisplay.appWin, FM_SHUTDOWN_CAMPAIGN, 0, 0);
            TheCampaign.Flags and_eq compl CAMP_TACTICAL;
            TheCampaign.Flags and_eq compl CAMP_TACTICAL_EDIT;
            break;

        case game_Campaign:
            SendMessage(FalconDisplay.appWin, FM_SHUTDOWN_CAMPAIGN, 0, 0);
            break;
    }
}


static char _cryptKey[] = "x~yz!#$%yeeour#$#$^1";

__forceinline void Encrypt(uchar startkey, uchar *buffer, long length)
{
    long i, xrlen, idx;
    uchar *ptr;
    uchar nextkey;

    if ( not buffer or length <= 0)
        return;

    idx = 0;
    xrlen = strlen(_cryptKey);

    ptr = buffer;

    for (i = 0; i < length; i++)
    {
        *ptr xor_eq _cryptKey[(idx++) % xrlen];
        *ptr xor_eq startkey;
        nextkey = *ptr++;
        startkey = nextkey;
    }
}

__forceinline
void Decrypt(uchar startkey, uchar *buffer, long length)
{
    long i, xrlen, idx;
    uchar *ptr;
    uchar nextkey;

    if ( not buffer or length <= 0)
        return;

    idx = 0;
    xrlen = strlen(_cryptKey);

    ptr = buffer;

    for (i = 0; i < length; i++)
    {
        nextkey = *ptr;
        *ptr xor_eq startkey;
        *ptr++ xor_eq _cryptKey[(idx++) % xrlen];
        startkey = nextkey;
    }
}

void LoadMainWindow()
{
    long ID;

    if (MainLoaded) return;

    if (_LOAD_ART_RESOURCES_)
    {
        gMainParser->LoadImageList("main_res.lst");
    }
    else
    {
        gMainParser->LoadImageList("main_art.lst"); // these aren't loaded anymore
    }

    gMainParser->LoadSoundList("main_snd.lst");
    gMainParser->LoadWindowList("main_scf.lst"); // Modified by M.N. - add art/art1024 by LoadWindowList

    ID = gMainParser->GetFirstWindowLoaded();

    while (ID)
    {
        HookupControls(ID);
        ID = gMainParser->GetNextWindowLoaded();
    }

    gMainParser->LoadPopupMenuList("art\\pop_scf.lst");
    HookupDogFightMenus();
    HookupCampaignMenus();
    LoadPeopleInfo(1); //VP_changes This should be modified
    MainLoaded++;
    gUBuffer = &gTrackBuffer[0];

    //Display the version on the main screen
    /*C_Window *win = gMainHandler->FindWindow(UI_MAIN_SCREEN);
    C_Text *txt=new C_Text;
    txt->Setup(C_DONT_CARE,0);
    txt->SetFont(BANK_GOTHIC_16);
    txt->SetFGColor(0x00ffffff);
    txt->SetXY(800,745);
    txt->SetText(gStringMgr->AddText(sVersion));
    win->AddControl(txt);*/
}

static void ExitTheGameCB(long , short hittype, C_Base *)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    //Cobra 12/29/04 Attempt to shut down comms if someone exits but forgot to shut down comms
    if (gCommsMgr->Online())
    {
        gCommsMgr->StopComms();
    }

    /* if (skycolor)
     delete [] skycolor;
     if (weatherPatternData)
     delete [] weatherPatternData;
    */

#ifdef DEBUG
    ShowCursor(TRUE); // Turn on mouse cursor for small window
#endif

    g_intellivibeData.IsExitGame = true;
    memcpy(gSharedIntellivibe, &g_intellivibeData, sizeof(g_intellivibeData));

    // PostMessage(gMainHandler->GetAppWnd(),FM_END_UI,0,0);
    PostMessage(gMainHandler->GetAppWnd(), FM_EXIT_GAME, 0, 0);
}

void ExitCloseWindowCB(long , short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    // gMainHandler->SetOutputDelay(80);
    // gMainHandler->SetControlDelay(80);
    if (control->GetGroup())
    {
        gMainHandler->DisableWindowGroup(control->GetGroup());

        if (MainLastGroup == control->GetGroup())
            MainLastGroup = 0;
    }
}

static void ExitButtonCB(long , short hittype, C_Base *)
{
    C_Window *win;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;


    win = gMainHandler->FindWindow(EXIT_WIN);

    if (win)
        win->VY_[1] = win->ClientArea_[1].top;

    // gMainHandler->SetOutputDelay(40);
    // gMainHandler->SetControlDelay(40);
    ExitVerify(TXT_EXIT_GAME, ExitTheGameCB, ExitCloseWindowCB);
}

void CloseWindowCB(long , short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    if (control->GetGroup())
    {
        gMainHandler->DisableWindowGroup(control->GetGroup());

        if (MainLastGroup == control->GetGroup())
            MainLastGroup = 0;
    }

}

void GenericCloseWindowCB(long , short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    gMainHandler->HideWindow(control->Parent_);
}

void MinMaxWindowCB(long , short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    if (control->Parent_->Minimized())
        control->Parent_->Maximize();
    else
        control->Parent_->Minimize();
}

void StartUITracking()
{
    strcpy(gTrackBuffer, gCommsMgr->GetUserInfo());
}

void DisableScenarioInfo()
{
    C_Window *win;
    C_Base *ctrl;

    gMainHandler->DisableWindowGroup(3050);
    gMainHandler->DisableWindowGroup(4050);
    win = gMainHandler->FindWindow(CS_TOOLBAR_WIN);

    if (win)
    {
        ctrl = win->FindControl(SINGLE_COMMIT_CTRL);

        if (ctrl)
        {
            ctrl->SetFlagBitOff(C_BIT_ENABLED);
            ctrl->Refresh();
        }

        ctrl = win->FindControl(COMMS_COMMIT_CTRL);

        if (ctrl)
        {
            ctrl->SetFlagBitOff(C_BIT_ENABLED);
            ctrl->Refresh();
        }
    }

    win = gMainHandler->FindWindow(TAC_LOAD_TOOLBAR);

    if (win)
    {
        ctrl = win->FindControl(SINGLE_COMMIT_CTRL);

        if (ctrl)
        {
            ctrl->SetFlagBitOff(C_BIT_ENABLED);
            ctrl->Refresh();
        }

        ctrl = win->FindControl(COMMS_COMMIT_CTRL);

        if (ctrl)
        {
            ctrl->SetFlagBitOff(C_BIT_ENABLED);
            ctrl->Refresh();
        }
    }
}

void EnableScenarioInfo(long ID)
{
    C_Window *win;
    C_Base*  ctrl;

    gMainHandler->EnableWindowGroup(ID);
    win = gMainHandler->FindWindow(CS_TOOLBAR_WIN);

    if (win)
    {
        ctrl = win->FindControl(SINGLE_COMMIT_CTRL);

        if (ctrl)
        {
            ctrl->SetFlagBitOn(C_BIT_ENABLED);
            ctrl->Refresh();
        }

        ctrl = win->FindControl(COMMS_COMMIT_CTRL);

        if (ctrl)
        {
            ctrl->SetFlagBitOn(C_BIT_ENABLED);
            ctrl->Refresh();
        }
    }

    win = gMainHandler->FindWindow(TAC_LOAD_TOOLBAR);

    if (win)
    {
        ctrl = win->FindControl(SINGLE_COMMIT_CTRL);

        if (ctrl)
        {
            ctrl->SetFlagBitOn(C_BIT_ENABLED);
            ctrl->Refresh();
        }

        ctrl = win->FindControl(COMMS_COMMIT_CTRL);

        if (ctrl)
        {
            ctrl->SetFlagBitOn(C_BIT_ENABLED);
            ctrl->Refresh();
        }
    }
}

static void OpenInstantActionCB(long , short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    DisableScenarioInfo();
    LeaveCurrentGame();

    RuleMode = rINSTANT_ACTION;

    SetCursor(gCursors[CRSR_WAIT]);

    if ( not IALoaded)
        LoadInstantActionWindows();

    if (MainLastGroup not_eq 0 and MainLastGroup not_eq control->GetGroup())
    {
        gMainHandler->DisableWindowGroup(MainLastGroup);
    }

    if (MainLastGroup not_eq control->GetGroup())
    {
        gMainHandler->EnableWindowGroup(control->GetGroup());
        MainLastGroup = control->GetGroup();
    }

    SetCursor(gCursors[CRSR_F16]);
}

static void OpenDogFightCB(long , short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    DisableScenarioInfo();
    LeaveCurrentGame();

    RuleMode = rDOGFIGHT;

    SetCursor(gCursors[CRSR_WAIT]);

    if ( not DFLoaded)
        LoadDogFightWindows();

    if (MainLastGroup not_eq 0 and MainLastGroup not_eq control->GetGroup())
    {
        gMainHandler->DisableWindowGroup(MainLastGroup);
    }

    if (MainLastGroup not_eq control->GetGroup())
    {
        gMainHandler->EnableWindowGroup(control->GetGroup());
        MainLastGroup = control->GetGroup();
    }

    SetCursor(gCursors[CRSR_F16]);
}

//VP_changes
static void OpenTacticalCB(long , short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    DisableScenarioInfo();
    LeaveCurrentGame();

    RuleMode = rTACTICAL_ENGAGEMENT;
    TheCampaign.Flags or_eq CAMP_TACTICAL;

    SetCursor(gCursors[CRSR_WAIT]);

    if ( not TACSelLoaded)
        LoadTacEngSelectWindows();

    if (MainLastGroup not_eq 0 and MainLastGroup not_eq control->GetGroup())
        gMainHandler->DisableWindowGroup(MainLastGroup);

    if (MainLastGroup not_eq control->GetGroup())
    {
        gMainHandler->EnableWindowGroup(control->GetGroup());
        MainLastGroup = control->GetGroup();
    }

    SetCursor(gCursors[CRSR_F16]);
}

void OpenMainCampaignCB(long , short hittype, C_Base *control)
{
    C_Button *btn;
    C_Window *win;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    DisableScenarioInfo();
    LeaveCurrentGame();

    RuleMode = rCAMPAIGN;

    SetCursor(gCursors[CRSR_WAIT]);

    if ( not CPSelectLoaded)
        LoadCampaignSelectWindows();

    if (MainLastGroup not_eq 0 and MainLastGroup not_eq control->GetGroup())
    {
        gMainHandler->DisableWindowGroup(MainLastGroup);
    }

    win = gMainHandler->FindWindow(CS_SELECT_WIN);

    if (win)
    {
        btn = (C_Button*)win->FindControl(CS_NEW_CTRL);

        if (btn and btn->GetState())
        {
            SetCampaignSelectCB(CS_NEW_CTRL, C_TYPE_LMOUSEUP, btn);
            SelectScenarioCB(CS_LOAD_SCENARIO1, C_TYPE_LMOUSEUP, NULL);
        }
    }

    gMainHandler->EnableWindowGroup(control->GetGroup());
    MainLastGroup = control->GetGroup();

    SetCursor(gCursors[CRSR_F16]);
}

void OpenCommsCB(long , short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
    {
        return;
    }

    control->SetFlagBitOff(C_BIT_FORCEMOUSEOVER);

    if ( not gCommsMgr->Online())
    {
        gMainHandler->EnableWindowGroup(control->GetUserNumber(1));
    }
    else
    {
        gMainHandler->EnableWindowGroup(control->GetUserNumber(0));
        gNewMessage = FALSE;
    }
}

void OpenTacticalReferenceCB(long nID, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    SetCursor(gCursors[CRSR_WAIT]);

    if ( not TACREFLoaded)
        LoadTacticalReferenceWindows();

    CloseAllRenderers(TAC_REF_WIN);

    if (TacRef_Setup())
    {
        gMainHandler->EnableWindowGroup(control->GetGroup());
    }

    SetCursor(gCursors[CRSR_F16]);
}

void OpenSetupCB(long , short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    SetCursor(gCursors[CRSR_WAIT]);
    LoadSetupWindows();

    CloseAllRenderers(SETUP_WIN);

    gMainHandler->EnableWindowGroup(control->GetGroup());
    SetCursor(gCursors[CRSR_F16]);
}


void OpenFontToolCB(long , short hittype, C_Base *)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    InitFontTool();
    gMainHandler->EnableWindowGroup(-100);
}


void GenericTimerCB(long , short , C_Base *control)
{
    if (control->GetUserNumber(_UI95_TIMER_COUNTER_) < 1)
    {
        control->SetReady(1);
        control->SetUserNumber(_UI95_TIMER_COUNTER_, control->GetUserNumber(_UI95_TIMER_DELAY_));
    }

    control->SetUserNumber(_UI95_TIMER_COUNTER_, control->GetUserNumber(_UI95_TIMER_COUNTER_) - 1);
}

void InfoGroupCB(long , short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    F4CSECTIONHANDLE* Leave = UI_Enter(control->GetParent());
    control->GetParent()->HideCluster(control->GetUserNumber(1));
    control->GetParent()->HideCluster(control->GetUserNumber(2));
    control->GetParent()->UnHideCluster(control->GetUserNumber(0));
    control->GetParent()->RefreshWindow();
    UI_Leave(Leave);

}

static void LoadArtwork()
{
    if (_LOAD_ART_RESOURCES_)
        gMainParser->LoadImageList("resimgs.lst");//DLP resimgs.lst DNE
    else
        gMainParser->LoadImageList("images.lst");//DLP images.lst DNE
}

static void LoadSoundFiles()
{
    gMainParser->LoadSoundList("sounds.lst");
    gSoundMgr->SetFlags(SND_FLY, gSoundMgr->GetFlags(SND_FLY) xor SOUND_STOPONEXIT);
}

static void LoadStringFiles()
{
    gMainParser->LoadStringList("strings.lst");
    gMainParser->LoadStringList("OIR Art\\Menus\\strings.lst");//DLP load strings for OIR menus
}

static void LoadMovieFiles()
{

    gMainParser->LoadMovieList("movies.lst");
}

void HookupControls(long ID)
{
    C_Window *winme;
    C_Button *ctrl;
    C_ListBox *lbox;

    winme = gMainHandler->FindWindow(ID);

    if (winme == NULL)
        return;

    // Hook up IDs here

    // Hook up Main Buttons...
    ctrl = (C_Button *)winme->FindControl(IA_MAIN_CTRL);

    if (ctrl)
        ctrl->SetCallback(OpenInstantActionCB);

    ctrl = (C_Button *)winme->FindControl(DF_MAIN_CTRL);

    if (ctrl)
        ctrl->SetCallback(OpenDogFightCB);

    ctrl = (C_Button *)winme->FindControl(TE_MAIN_CTRL);

    if (ctrl)
        ctrl->SetCallback(OpenTacticalCB);

    ctrl = (C_Button *)winme->FindControl(CP_MAIN_CTRL);

    if (ctrl)
        ctrl->SetCallback(OpenMainCampaignCB);

    ctrl = (C_Button *)winme->FindControl(TACREF_CTRL);

    if (ctrl)
        ctrl->SetCallback(OpenTacticalReferenceCB);

    ctrl = (C_Button *)winme->FindControl(LB_MAIN_CTRL);

    if (ctrl)
        ctrl->SetCallback(OpenLogBookCB);

    ctrl = (C_Button *)winme->FindControl(CO_MAIN_CTRL);

    if (ctrl)
        ctrl->SetCallback(OpenCommsCB);

    ctrl = (C_Button *)winme->FindControl(SP_MAIN_CTRL);

    if (ctrl)
        ctrl->SetCallback(OpenSetupCB);

    ctrl = (C_Button *)winme->FindControl(ACMI_CTRL);

    if (ctrl)
        ctrl->SetCallback(ACMIButtonCB);

    ctrl = (C_Button *)winme->FindControl(UI_THEATER_BUTTON);

    if (ctrl)
        ctrl->SetCallback(TheaterButtonCB);

    // Hook up Close Button
    ctrl = (C_Button *)winme->FindControl(CLOSE_WINDOW);

    if (ctrl)
        ctrl->SetCallback(CloseWindowCB);

    // Hook up Exit Button
    ctrl = (C_Button *)winme->FindControl(EXIT_CTRL);

    if (ctrl)
        ctrl->SetCallback(ExitButtonCB);

    ctrl = (C_Button *)winme->FindControl(DF_CLOSE_CTRL);

    if (ctrl)
        ctrl->SetCallback(EndDogfightCB);


    ctrl = (C_Button*)winme->FindControl(SAVE_FONT_CTRL);

    if (ctrl)
        ctrl->SetCallback(OpenFontToolCB);

    lbox = (C_ListBox*)winme->FindControl(CUR_FONT);

    if (lbox)
        lbox->SetCallback(ChooseFontCB);

    ctrl = (C_Button*)winme->FindControl(FONTED_CREATE);

    if (ctrl)
        ctrl->SetCallback(CreateFontCB);

    ctrl = (C_Button*)winme->FindControl(FONTED_EXPORT);

    if (ctrl)
        ctrl->SetCallback(CreateTheFontCB);

    ctrl = (C_Button*)winme->FindControl(FONTED_SAVE);

    if (ctrl)
        ctrl->SetCallback(SaveFontCB);

    ctrl = (C_Button*)winme->FindControl(LEAD_MORE);

    if (ctrl)
        ctrl->SetCallback(IncreaseLead);

    ctrl = (C_Button*)winme->FindControl(LEAD_LESS);

    if (ctrl)
        ctrl->SetCallback(DecreaseLead);

    ctrl = (C_Button*)winme->FindControl(TRAIL_MORE);

    if (ctrl)
        ctrl->SetCallback(IncreaseTrail);

    ctrl = (C_Button*)winme->FindControl(TRAIL_LESS);

    if (ctrl)
        ctrl->SetCallback(DecreaseTrail);

    ctrl = (C_Button*)winme->FindControl(WIDTH_MORE);

    if (ctrl)
        ctrl->SetCallback(IncreaseWidth);

    ctrl = (C_Button*)winme->FindControl(WIDTH_LESS);

    if (ctrl)
        ctrl->SetCallback(DecreaseWidth);

    ctrl = (C_Button*)winme->FindControl(KERN_MORE);

    if (ctrl)
        ctrl->SetCallback(IncreaseKern);

    ctrl = (C_Button*)winme->FindControl(KERN_LESS);

    if (ctrl)
        ctrl->SetCallback(DecreaseKern);

    // Help GUIDE thing
    ctrl = (C_Button*)winme->FindControl(UI_HELP_GUIDE);

    if (ctrl)
        ctrl->SetCallback(UI_Help_Guide_CB);


}

void GlobalSetup()
{
    DWORD r_mask;
    WORD r_shift;
    DWORD g_mask;
    WORD g_shift;
    DWORD b_mask;
    WORD b_shift;
    char *mouse;
    // FILE *fp;

    mouse = MAKEINTRESOURCE(UI_F16);
    gCursors[1] = LoadCursor(hInst, mouse);
    gCursors[0] = gCursors[1];
    mouse = MAKEINTRESOURCE(UI_F16_ON);
    gCursors[2] = LoadCursor(hInst, mouse);
    mouse = MAKEINTRESOURCE(UI_F16_ON_RM);
    gCursors[3] = LoadCursor(hInst, mouse);
    mouse = MAKEINTRESOURCE(UI_F16_RM);
    gCursors[4] = LoadCursor(hInst, mouse);
    mouse = MAKEINTRESOURCE(UI_DRAG);
    gCursors[5] = LoadCursor(hInst, mouse);
    mouse = MAKEINTRESOURCE(UI_DRAG_RM);
    gCursors[6] = LoadCursor(hInst, mouse);
    mouse = MAKEINTRESOURCE(UI_DRAG_STEERPOINT);
    gCursors[7] = LoadCursor(hInst, mouse);
    mouse = MAKEINTRESOURCE(UI_HDRAG);
    gCursors[8] = LoadCursor(hInst, mouse);
    mouse = MAKEINTRESOURCE(UI_HDRAG_ON);
    gCursors[9] = LoadCursor(hInst, mouse);
    mouse = MAKEINTRESOURCE(UI_HDRAG_RM);
    gCursors[10] = LoadCursor(hInst, mouse);
    mouse = MAKEINTRESOURCE(UI_VDRAG);
    gCursors[11] = LoadCursor(hInst, mouse);
    mouse = MAKEINTRESOURCE(UI_VDRAG_ON);
    gCursors[12] = LoadCursor(hInst, mouse);
    mouse = MAKEINTRESOURCE(UI_VDRAG_RM);
    gCursors[13] = LoadCursor(hInst, mouse);
    mouse = MAKEINTRESOURCE(UI_STEERPOINT);
    gCursors[14] = LoadCursor(hInst, mouse);
    mouse = MAKEINTRESOURCE(UI_LIST_F16);
    gCursors[15] = LoadCursor(hInst, mouse);
    mouse = MAKEINTRESOURCE(UI_NOT_ALLOWED);
    gCursors[16] = LoadCursor(hInst, mouse);
    mouse = MAKEINTRESOURCE(UI_MAP_ZOOM);
    gCursors[17] = LoadCursor(hInst, mouse);
    mouse = MAKEINTRESOURCE(UI_TARGET);
    gCursors[18] = LoadCursor(hInst, mouse);
    mouse = MAKEINTRESOURCE(UI_WAIT);
    gCursors[19] = LoadCursor(hInst, mouse);
    mouse = MAKEINTRESOURCE(UI_TEXT);
    gCursors[20] = LoadCursor(hInst, mouse);

    SetCursor(gCursors[CRSR_WAIT]);

    // fp=fopen("art\\main\\ascii.bin","rb");
    // if(fp)
    // {
    // fread(Key_Chart,sizeof(Key_Chart),1,fp);
    // fclose(fp);
    // }

    if (gImageMgr == NULL)
        gImageMgr = new C_Image;

    gImageMgr->Setup();
    gImageMgr->SetColorKey(UI95_RGB24Bit(0x00ff00ff));
    UI95_GetScreenColorInfo(r_mask, r_shift, g_mask, g_shift, b_mask, b_shift);
    //UI95_GetScreenColorInfo(&r_mask,&r_shift,&g_mask,&g_shift,&b_mask,&b_shift);
    gImageMgr->SetScreenFormat(r_shift, g_shift, b_shift);

    gFontList = new C_Font;
    gFontList->Setup(gMainHandler);

    gAnimMgr = new C_Animation;
    gAnimMgr->Setup();

    gSoundMgr = new C_Sound;
    gSoundMgr->Setup();

    gStringMgr = new C_String;
    gStringMgr->Setup(TXT_LAST_TEXT_ID);

    gPopupMgr = new C_PopupMgr;
    gPopupMgr->Setup(gMainHandler);

    gMovieMgr = new C_Movie;
    gMovieMgr->Setup();

    gMainParser = new C_Parser;
    gMainParser->Setup(gMainHandler, gImageMgr, gFontList, gSoundMgr, gPopupMgr, gAnimMgr, gStringMgr, gMovieMgr);

    gMainParser->SetCheck(0); // Used to find which IDs are NOT used

    gMainParser->LoadIDList("userids.lst");
    gMainParser->LoadIDList("fontids.lst");
    gMainParser->LoadIDList("imageids.lst");
    gMainParser->LoadIDList("soundids.lst");
    gMainParser->LoadIDList("textids.lst");
    gMainParser->LoadIDList("movieids.lst");

    gMainParser->SetCheck(1); // Used to find which IDs are NOT used

    gMainParser->ParseFont("art\\fonts\\fontrc.irc");

#ifdef DEBUG

    if (gMainParser->FindID("TXT_LAST_TEXT_ID") > TXT_LAST_TEXT_ID)
        MessageBox(NULL, "String database Out of Date", "Update Art Directory - May crash in C_Hash", MB_OK);

#endif

    gUBuffer = &gTrackBuffer[0];

    // This function goes through the class table searching for VIS IDs,
    // and returns the array index of the Vis ID... only needs to happen ONCE per execute
    ValidateRackData();
}

void SetStartupFlags()
{
    for (int i = 0; List1[i]; ++i)
        if (stricmp(List1[i], gUBuffer) == 0)
        {
            gUI_Tracking_Flag or_eq _UI_TRACK_FLAG00;
            break;
        }

    for (int i = 0; List2[i]; ++i)
        if (stricmp(List2[i], gUBuffer) == 0)
        {
            gUI_Tracking_Flag or_eq _UI_TRACK_FLAG01;
            break;
        }

    for (int i = 0; List3[i]; ++i)
        if (stricmp(List3[i], gUBuffer) == 0)
        {
            gUI_Tracking_Flag or_eq _UI_TRACK_FLAG02;
            break;
        }
}

void StartCommsQueue()
{
    CommsQueue *newQueue;

    newQueue = new CommsQueue;
    newQueue->Setup(FalconDisplay.appWin);

    gUICommsQ = newQueue;
}

void UI_UpdateVU()
{
    if (UI_VuThread)
    {
        UI_VuThread->Update(3);
    }
}

static int LastUIPlayed = 0;
static short MusicTypePlayed = 0;
static short LastTypePlayed = 0;

void PlayUIMusic()
{
    MusicTypePlayed = 1; // Main UI Music
    LastTypePlayed = 1;

    if (LastUIPlayed not_eq SND_AMBIENT1 and LastUIPlayed not_eq SND_AMBIENT2)
    {
        if (rand() bitand 1)
            LastUIPlayed = SND_AMBIENT1;
    }

    if (LastUIPlayed not_eq SND_AMBIENT1)
    {
        gMusic->AddQ(SND_AMBIENT1);
        LastUIPlayed = SND_AMBIENT1;
    }
    else
    {
        gMusic->AddQ(SND_AMBIENT2);
        LastUIPlayed = SND_AMBIENT2;
    }

    gMusic->PlayQ();
    gMusic->SetVolume(PlayerOptions.GroupVol[MUSIC_SOUND_GROUP]);
}

void PlayCampaignMusic() // This function should figure out whether we are happy,mellow,sad :)
{
    // and play music accordingly
    // Team[MyTeam]... Initiative()  0->33 Bad 34->66 Medium 67->100 Good
    if ( not TeamInfo[FalconLocalSession->GetTeam()])
    {
        PlayUIMusic();
        return;
    }

    MusicTypePlayed = 2; // Campaign Music
    LastTypePlayed = 1;

    if (TeamInfo[FalconLocalSession->GetTeam()]->playerRating < -5)
        // if(TeamInfo[2]->Initiative() < 34)
        gMusic->AddQ(SND_CAMPAIGN_BAD);
    else if (TeamInfo[FalconLocalSession->GetTeam()]->playerRating < 5)
        // else if (TeamInfo[2]->Initiative() < 67)
        gMusic->AddQ(SND_CAMPAIGN_MEDIUM);
    else
        gMusic->AddQ(SND_CAMPAIGN_GOOD);

    gMusic->PlayQ();
}

void PlayThatFunkyMusicWhiteBoy()
{
    if ( not gMusic or not MusicStopped)
        return;

    // if(GetCurrentTime() < (MusicStopped + 60000l))
    // return;

    if (LastTypePlayed not_eq 1)
    {
        if (MusicTypePlayed == 2)
            PlayCampaignMusic();
        else
            PlayUIMusic();
    }
    else
    {
        gMusic->StartInteractive(rand() bitand 0x01, 0);
        LastTypePlayed = 2;
    }
}

void PlayUIMovie(long ID)
{
#ifdef _USE_MOVIE_
    C_Window *win;

    win = gMainHandler->FindWindow(VIDEO_WIN);

    if ((win) and (gMovieMgr->GetMovie(ID)))
    {
        gMoviePlaying = TRUE;
        gMusic->FadeOut_Pause();
        gMainHandler->ShowWindow(win);
        gMainHandler->WindowToFront(win);
        gMovieMgr->SetXY(win->GetX() + win->ClientArea_[0].left, win->GetY() + win->ClientArea_[0].top);
        gMovieMgr->Play(ID);
        gMainHandler->HideWindow(win);
        gMusic->Resume();
        gMoviePlaying = FALSE;
    }

#endif
}

void UI_LoadSkyWeatherData()
{
    /* // M.N.
     FILE* fp;
     int i = 0;
     char file[1024];
     _TCHAR image1[MAX_PATH], image2[MAX_PATH], image3[MAX_PATH], image4[MAX_PATH];
     _TCHAR name[50], filename[50], picname[50];

     // Skycolor data readin
    // sprintf(file,"%s\\weather\\todtable.dat",FalconTerrainDataDir);

     /* format:
     [NumberOfSkyColors]
     [UI display name]
     [TOD file name] [Image1] [Image2] [Image3] [Image4]
     */

    /* if( not (fp=fopen(file,"rt")))
     return;
     NumberOfSkyColors = atoi(fgets(file,1024,fp));

     skycolor = new SkyColorDataType[NumberOfSkyColors];
     while (i<NumberOfSkyColors) //for (int i=0; i<NumberOfSkyColors; i++)
     {
     fgets(file,1024,fp);
     if (file[0] == '\r' or file[0] == '#' or file[0] == ';' or file[0] == '\n')
     continue;

     strcpy(name,file);
     strcpy(skycolor[i].name,name);
     fgets(file,1024,fp);
     sscanf(file, "%s %s %s %s %s",filename,image1,image2,image3,image4);
     strcpy(skycolor[i].todname,filename);
     strcpy(skycolor[i].image1,image1);
     if ( not strlen(image2)) // If we have no entry, use the main image
     strcpy(image2,image1);
     strcpy(skycolor[i].image2,image2);
     if ( not strlen(image3))
     strcpy(image3,image1);
     strcpy(skycolor[i].image3,image3);
     if ( not strlen(image4))
     strcpy(image4,image1);
     strcpy(skycolor[i].image4,image4);
     strcpy(image1,"");
     strcpy(image2,"");
     strcpy(image3,"");
     strcpy(image4,"");
     i++;
     }
     fclose(fp);

     prevskycol = PlayerOptions.skycol;

     sprintf(file,"%s\\weather\\weathertable.dat",FalconTerrainDataDir);

     if( not (fp=fopen(file,"rt")))
     return;
     NumWeatherPatterns = atoi(fgets(file,1024,fp));
     i = 0;
     weatherPatternData = new WeatherPatternDataType[NumWeatherPatterns];
     while (i<NumWeatherPatterns)
    // for (i=0; i<NumWeatherPatterns; i++)
     {
     fgets(file,1024,fp);
     if (file[0] == '\r' or file[0] == '#' or file[0] == ';' or file[0] == '\n')
     continue;
     strcpy(weatherPatternData[i].name,file);
     fgets(file,1024,fp);
     sscanf(file,"%s %s",filename, picname); // filename = RAW, picname = picture
     strcpy(weatherPatternData[i].filename,filename);
     strcpy(weatherPatternData[i].picfname,picname);
     i++;
     }
     fclose(fp);
    */
}

int UI_Startup()
{
    C_Window *win;
    ImageBuffer *Primary;
    int i;
    DWORD r_mask, g_mask, b_mask;

    // OW
    //ShowCursor(TRUE);
    while (ShowCursor(TRUE) < 0);

    // OW
    // SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

    FalconMessageFilter UIFilter(FalconEvent::UIThread, 0);

    // OW: Enable UI Hardware acceleration
    // FalconDisplay.EnterMode(FalconDisplayConfiguration::UI);

    // M.N. Large UI
    if (g_bHiResUI)
    {
        FalconDisplay.EnterMode(
            FalconDisplayConfiguration::UILarge, DisplayOptions.DispVideoCard, DisplayOptions.DispVideoDriver
        );
    }
    else
    {
        FalconDisplay.EnterMode(
            FalconDisplayConfiguration::UI, DisplayOptions.DispVideoCard, DisplayOptions.DispVideoDriver
        );
    }

    Primary = FalconDisplay.GetImageBuffer();

    Primary->GetColorMasks(&r_mask, &g_mask, &b_mask);
    UI95_SetScreenColorInfo(static_cast<DWORD>(r_mask), static_cast<DWORD>(g_mask), static_cast<DWORD>(b_mask));

    UIBuildColorTable();

    if ( not gPlayerBook)
    {
        gPlayerBook = new PhoneBook;
        gPlayerBook->Setup();
        gPlayerBook->Load("phonebkn.da2");
    }

    // THESE 2 LINES ARE VERY VERY Important
    ShowWindow(FalconDisplay.appWin, SW_SHOWNORMAL);
    UpdateWindow(FalconDisplay.appWin);

    if (gScreenShotEnabled)
    {
        if (g_bHiResUI)
            gScreenShotBuffer = new WORD[1024l * 768l];
        else
            gScreenShotBuffer = new WORD[800l * 600l];
    }

    gMainHandler = new C_Handler;
    gMainHandler->Setup(FalconDisplay.appWin, NULL, Primary);
    // gMainHandler->SetCallback(UIMainMouse);

    GlobalSetup();
    LoadArtwork();
    LoadSoundFiles();
    LoadStringFiles();
    LoadMovieFiles();
    SetStartupFlags();

    LoadMainWindow();
    LoadCommsWindows();
    LoadHelpGuideWindows();
    RealLoadLogbook(); // without daves extra garbage

    _tcscpy(gUI_AutoSaveName, gStringMgr->GetString(TXT_AUTOSAVENAME));


    if (gCommsMgr->Online())
    {
        StartCommsQueue();
        RebuildGameTree();
    }

    INFOSetupControls();

    gMusic = new C_Music;
    gMusic->Setup(gSoundDriver);
    i = 0;

    while (IntList[i].MusicID)
    {
        gMusic->AddInteractiveMusic(IntList[i].Section, IntList[i].Group, IntList[i].MusicID);
        i++;
    }

    gMainHandler->SetSection(100);

    if (MainLastGroup)
    {
        // Returning from the sim - Post eval our flight
        // KCK: Added the check for a pilot list so that we don't post-eval after a
        // discarded mission
        if (TheCampaign.MissionEvaluator and TheCampaign.MissionEvaluator->flight_data)
            TheCampaign.MissionEvaluator->PostMissionEval();

        if (MainLastGroup == 1000)
        {
            LoadInstantActionWindows();
            gMainHandler->EnableWindowGroup(100);
            gMainHandler->EnableWindowGroup(MainLastGroup);
        }
        else if (MainLastGroup == 2000)
        {
            LoadDogFightWindows();
        }
        else if (MainLastGroup == 3000)
        {
            LoadCampaignSelectWindows();
            restart_tactical_engagement();
        }
        else if (MainLastGroup == 4000)
        {
            gMainHandler->SetSection(200);
            LoadCampaignSelectWindows();
            SetupMapWindow();
            LoadCampaignWindows();
            CampaignSetup();
            gMainHandler->EnableWindowGroup(200);

            if (CampaignLastGroup)
            {
                win = gMainHandler->FindWindow(CP_TOOLBAR);

                if (win)
                    win->UnHideCluster(CampaignLastGroup);

                gMainHandler->EnableWindowGroup(CampaignLastGroup);
            }

            ActivateCampMissionSchedule();

            DoResultsWindows();
            /*
            if(MissionResult bitand PROMOTION)
             PromotionWindow();
            if(MissionResult bitand AWARD_MEDAL)
             AwardWindow();
            else if(MissionResult bitand COURT_MARTIAL)
             CourtMartialWindow();
            */

        }
        else
        {
            gMainHandler->EnableWindowGroup(MainLastGroup);
        }
    }
    else
        gMainHandler->EnableWindowGroup(100);

    if (CampaignLastGroup not_eq 4000)
        PlayUIMusic();

    gSoundMgr->SetAllVolumes(PlayerOptions.GroupVol[UI_SOUND_GROUP]);


    gMainHandler->StartTimerThread(UI_TIMER_INTERVAL); // 1 second intervals

    gMainHandler->SetEnableTime(GetCurrentTime() + 100);
    gMainHandler->SetDrawFlag(1); // allow drawing (currently a true/false flag)

    UI_VuThread = new VuThread(&UIFilter, F4_EVENT_QUEUE_SIZE);

    // Test KLUDGE for speedup
    DeviceDependentGraphicsSetup(&FalconDisplay.theDisplayDevice);
    //DrawableBSP::LockAndLoad(VIS_F16C);

    SetCursor(gCursors[CRSR_F16]);

    if ( not (LogState bitand LB_LOADED_ONCE))
    {
        LogState or_eq LB_LOADED_ONCE;
        LogBook.Initialize();
        UI_logbk.Initialize();
        PlayerOptions.Initialize();
        DisplayOptions.Initialize();
    }

    if ( not LogBook.CheckPassword(_T("")) and not (LogState bitand LB_CHECKED))
        PasswordWindow(TXT_LOG_IN, TXT_LOG_IN_MESSAGE, CheckPasswordCB, NoPasswordCB);
    else
    {
        FalconLocalSession->SetPlayerName(LogBook.NameWRank());
        FalconLocalSession->SetPlayerCallsign(LogBook.Callsign());
        FalconLocalSession->SetAceFactor(LogBook.AceFactor());
        FalconLocalSession->SetInitAceFactor(LogBook.AceFactor());
    }

    F4HearVoices();
    UserStickInputs.Reset();

    return(0);
}

void UI_Cleanup()
{
    int i;

    SetCursor(gCursors[CRSR_WAIT]);

    if (UI_VuThread)
    {
        delete UI_VuThread;
        UI_VuThread = NULL;
    }

    if (gCommsMgr)
        for (i = 0; i < game_MaxGameTypes; i++)
            gCommsMgr->SetCallback(i, NULL); // Disable callbacks we don't care about when NOT in the UI

    // End Event Loop
    Sleep(10);

    if (gMapMgr)
    {
        gMapMgr->Cleanup();
        delete gMapMgr;
        gMapMgr = NULL;
    }

    if (gMainHandler)
    {
        gMainHandler->Cleanup();
        delete gMainHandler;
        gMainHandler = NULL;
    }

    if (gUIViewer)
    {
        delete gUIViewer;
        gUIViewer = NULL;
    }

    OTWDriver.CleanViewpoint(); // JB 010615
    DeviceDependentGraphicsCleanup(&FalconDisplay.theDisplayDevice);

    if (gMusic)
    {
        gMusic->Cleanup();
        delete gMusic;
        gMusic = NULL;
    }

    if (gSoundMgr)
    {
        gSoundMgr->Cleanup();
        delete gSoundMgr;
        gSoundMgr = NULL;
    }

    if (gPopupMgr)
    {
        gPopupMgr->Cleanup();
        delete gPopupMgr;
        gPopupMgr = NULL;
    }

    if (gImageMgr)
    {
        gImageMgr->Cleanup();
        delete gImageMgr;
        gImageMgr = NULL;
    }

    if (gAnimMgr)
    {
        gAnimMgr->Cleanup();
        delete gAnimMgr;
        gAnimMgr = NULL;
    }

    if (gOccupationMap)
    {
        C_Resmgr *mgr;
        mgr = gOccupationMap->Owner;
        mgr->Cleanup();
        delete mgr;
        gOccupationMap = NULL;
    }

    if (gMainParser)
    {
        gMainParser->Cleanup();
        delete gMainParser;
        gMainParser = NULL;
    }

    if (gFontList)
    {
        gFontList->Cleanup();
        delete gFontList;
        gFontList = NULL;
    }

    if (gStringMgr)
    {
        gStringMgr->Cleanup();
        delete gStringMgr;
        gStringMgr = NULL;
    }

    if (gMovieMgr)
    {
        gMovieMgr->Cleanup();
        delete gMovieMgr;
        gMovieMgr = NULL;
    }

    IALoaded = 0;
    ACMILoaded = 0;
    DFLoaded = 0;
    TACLoaded = 0;
    CPLoaded = 0;
    CPSelectLoaded = 0;
    LBLoaded = 0;
    STPLoaded = 0;
    COLoaded = 0;
    MainLoaded = 0;
    PlannerLoaded = 0;
    TACREFLoaded = 0;
    CommonLoaded = 0;
    INFOLoaded = 0;
    HelpLoaded = 0;
    TACSelLoaded = 0;

    if (gPlayerBook)
    {
        gPlayerBook->Save("phonebkn.da2");
        gPlayerBook->Cleanup();
        delete gPlayerBook;
        gPlayerBook = NULL;
    }

#ifdef __WATCOMC__
#pragma warning 379 4;
#endif

    if (People)
    {
        // Now maintained in a script
        People = NULL;
    }

    if (DogfightGames)
    {
        // Now maintained in a script
        DogfightGames = NULL;
    }

    if (TacticalGames)
    {
        // Now maintained in a script
        TacticalGames = NULL;
    }

    if (CampaignGames)
    {
        // Now maintained in a script
        CampaignGames = NULL;
    }

    if (gUICommsQ)
    {
        F4EnterCriticalSection(vuCritical);
        gUICommsQ->Cleanup();
        delete gUICommsQ;
        gUICommsQ = NULL;
        F4LeaveCriticalSection(vuCritical);
    }

    if (gInstantBites)
    {
        gInstantBites->Cleanup();
        delete gInstantBites;
        gInstantBites = NULL;
    }

    if (gDogfightBites)
    {
        gDogfightBites->Cleanup();
        delete gDogfightBites;
        gDogfightBites = NULL;
    }

    if (gCampaignBites)
    {
        gCampaignBites->Cleanup();
        delete gCampaignBites;
        gCampaignBites = NULL;
    }

    ShutdownSetup();

    PlayerOptions.SaveOptions();
    LogBook.SaveData();

    if (KeyDescrips)
        CleanupKeys();

    //UserFunctionTable.ClearTable();

    for (i = 1; i < MAX_CURSORS; i++)
        if (gCursors[i])
            DeleteObject(gCursors[i]);

    if (gScreenShotEnabled and gScreenShotBuffer)
    {
        delete gScreenShotBuffer;
        gScreenShotBuffer = NULL;
    }

#ifdef USE_SH_POOLS
#if 0
    // Shrink Memory Pools to find out how much is still active
    MemPoolShrink(UI_Pools[UI_GENERAL_POOL]);
    MemPoolShrink(UI_Pools[UI_CONTROL_POOL]);
    MemPoolShrink(UI_Pools[UI_ART_POOL]);
    MemPoolShrink(UI_Pools[UI_SOUND_POOL]);

    // Display amout of leakage in these Pools
    MonoPrint("Memory Blocks allocated AFTER cleanup\n");
    MonoPrint("UI_Pools[UI_GENERAL_POOL] size = %1lu\n", MemPoolCount(UI_Pools[UI_GENERAL_POOL]));
    MonoPrint("UI_Pools[UI_CONTROL_POOL] size = %1lu\n", MemPoolCount(UI_Pools[UI_CONTROL_POOL]));
    MonoPrint("UI_Pools[UI_ART_POOL] size = %1lu\n", MemPoolCount(UI_Pools[UI_ART_POOL]));
    MonoPrint("UI_Pools[UI_SOUND_POOL] size = %1lu\n", MemPoolCount(UI_Pools[UI_SOUND_POOL]));

    dbgMemSetCheckpoint(11);

    if (lastErrorFn)
    {
        MemSetErrorHandler(lastErrorFn);
        dbgMemSetDefaultErrorOutput(DBGMEM_OUTPUT_FILE, "uileak1.log");
        dbgMemReportLeakage(UI_Pools[UI_GENERAL_POOL], 10, 11);
        dbgMemSetDefaultErrorOutput(DBGMEM_OUTPUT_FILE, "uileak2.log");
        dbgMemReportLeakage(UI_Pools[UI_CONTROL_POOL], 10, 11);
        dbgMemSetDefaultErrorOutput(DBGMEM_OUTPUT_FILE, "uileak3.log");
        dbgMemReportLeakage(UI_Pools[UI_ART_POOL], 10, 11);
        dbgMemSetDefaultErrorOutput(DBGMEM_OUTPUT_FILE, "uileak4.log");
        dbgMemReportLeakage(UI_Pools[UI_SOUND_POOL], 10, 11);
        MemSetErrorHandler(errPrint);
    }

#endif
    // Free up Memory Pools
    MemPoolFree(UI_Pools[UI_GENERAL_POOL]);
    MemPoolFree(UI_Pools[UI_CONTROL_POOL]);
    MemPoolFree(UI_Pools[UI_ART_POOL]);
    MemPoolFree(UI_Pools[UI_SOUND_POOL]);
#endif

    FalconDisplay.LeaveMode();

    // OW
    //ShowCursor(FALSE);
    while (ShowCursor(FALSE) >= 0);

    // OW
    //    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
}

// theater specifc stuff
#include "TheaterDef.h"

extern BOOL FileNameSortCB(TREELIST *list, TREELIST *newitem);
static void SelectTheater(TheaterDef *td);

static void TheaterBackCB(long, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    TheaterDef *td = NULL;
    gMainHandler->HideWindow(control->Parent_);
    MainLastGroup = 0;

    C_Window *win = gMainHandler->FindWindow(UI_THEATER_WINDOW);

    C_Button *btn = (C_Button*)win->FindControl(UI_THEATER_IMAGE);

    if (btn)
    {
        td = (TheaterDef *)btn->GetUserPtr(0);

        if (td)
        {
            SetCursor(gCursors[CRSR_WAIT]);
            g_theaters.SetNewTheater(td);
            SetCursor(gCursors[CRSR_F16]);
        }
    }

    PostMessage(gMainHandler->GetAppWnd(), FM_END_UI, 0, 0);
    // wParam "1" calls "DoSoundSetup" in TheaterDef.cpp
    // Can't call it here, as the UI must be shut down before the sound gets setup again
    PostMessage(gMainHandler->GetAppWnd(), FM_START_UI, 1, 0);
}

static void TheaterCancelCB(long, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    gMainHandler->HideWindow(control->Parent_);
    MainLastGroup = 0;
}

static void TheaterLoadCB(long ID, short hittype, C_Base *control)
{
    C_TreeList *tree;
    TREELIST *item;
    C_Button   *btn;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    tree = (C_TreeList*)control;

    if (tree)
    {
        item = tree->GetLastItem();

        if (item)
        {
            btn = (C_Button*)item->Item_;

            tree->SetAllControlStates(0, tree->GetRoot());
            btn->SetState(1);
            tree->Refresh();

            int n = btn->GetUserNumber(C_STATE_0);
            TheaterDef *td;

            if (td = g_theaters.GetTheater(n))
                SelectTheater(td);
        }
    }
}

static void LoadTheaterWindows(C_Window *win)
{
    C_Button *ctrl;
    ctrl = (C_Button*)win->FindControl(UI_THEATER_BACK);

    if (ctrl)
        ctrl->SetCallback(TheaterCancelCB);

    ctrl = (C_Button *)win->FindControl(CANCEL);

    if (ctrl)
        ctrl->SetCallback(TheaterCancelCB);

    ctrl = (C_Button *)win->FindControl(CLOSE_WINDOW);

    if (ctrl)
        ctrl->SetCallback(TheaterBackCB);

}

static void FillTheaterTree(C_TreeList *tree)
{
    TheaterDef *td;
    long UniqueID = tree->GetUserNumber(0);
    TREELIST *item;
    TheaterDef *cthr = g_theaters.GetCurrentTheater();

    if ( not UniqueID)
        UniqueID++;

    for (int i = 0; td = g_theaters.GetTheater(i); i++)
    {
        C_Button *btn = new C_Button;

        if (btn)
        {
            btn->Setup(UniqueID, C_TYPE_CUSTOM, 0, 0);
            btn->SetFont(tree->GetFont());
            btn->SetText(C_STATE_0, td->m_name);
            btn->SetText(C_STATE_1, td->m_name);
            btn->SetText(C_STATE_DISABLED, td->m_name);
            btn->SetFgColor(C_STATE_0, 0xcccccc);
            btn->SetFgColor(C_STATE_1, 0x00ff00);
            btn->SetFgColor(C_STATE_DISABLED, 0x808080);
            btn->SetUserNumber(0, 0);
            btn->SetCursorID(tree->GetCursorID());
            btn->SetDragCursorID(tree->GetDragCursorID());
            btn->SetMenu(0);
            btn->SetUserNumber(C_STATE_0, i);

            item = tree->CreateItem(UniqueID++, C_TYPE_ITEM, btn);

            if (item)
                tree->AddItem(tree->GetRoot(), item);

            if (cthr == td)
            {
                btn->SetState(1);
                SelectTheater(td);
            }
        }
    }

    tree->SetUserNumber(0, UniqueID);

}

static void SelectTheater(TheaterDef *td)
{
    C_Window *win = gMainHandler->FindWindow(UI_THEATER_WINDOW);

    C_Text *txt = (C_Text*)win->FindControl(UI_THEATER_DESC);

    if (txt)
    {
        if (td)
            txt->SetText(td->m_description);
        else txt->SetText("");
    }

    C_Button *btn = (C_Button*)win->FindControl(UI_THEATER_IMAGE);

    if (btn)
    {
        btn->ClearImage(0, UI_THEATER_BITMAP);
        gImageMgr->RemoveImage(UI_THEATER_BITMAP);

        if (td and td->m_bitmap)
        {
            gImageMgr->LoadImage(UI_THEATER_BITMAP, td->m_bitmap, 0, 0);
            btn->SetImage(0, UI_THEATER_BITMAP);
            btn->Refresh();
        }

        btn->SetUserPtr(0, td);
    }

    win->RefreshWindow();
}


static void LoadAllTheaters(C_Window *win)
{
    C_TreeList *tree = (C_TreeList *)win->FindControl(FILELIST_TREE);

    if (tree)
    {
        tree->DeleteBranch(tree->GetRoot());
        tree->SetUserNumber(0, 1);
        tree->SetSortType(TREE_SORT_CALLBACK);
        tree->SetSortCallback(FileNameSortCB);
        tree->SetCallback(TheaterLoadCB);
        FillTheaterTree(tree);
        tree->RecalcSize();
        win->RefreshClient(tree->GetClient());
    }
}

// JPO reveal the theaters.
void TheaterButtonCB(long ID, short hittype, C_Base *control)
{
    static bool theatersetupdone = false;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    C_Window *win;
    win = gMainHandler->FindWindow(UI_THEATER_WINDOW);

    if (win == NULL)
        return;

    DisableScenarioInfo();
    LeaveCurrentGame();

    if (MainLastGroup not_eq 0 and MainLastGroup not_eq control->GetGroup())
    {
        gMainHandler->DisableWindowGroup(MainLastGroup);
    }

    MainLastGroup = 0;
#if 0

    if (MainLastGroup not_eq control->GetGroup())
    {
        gMainHandler->EnableWindowGroup(control->GetGroup());
        MainLastGroup = control->GetGroup();
    }

#endif

    LoadTheaterWindows(win);
    SelectTheater(NULL);
    LoadAllTheaters(win);
    win->RefreshWindow();
    gMainHandler->ShowWindow(win);
    gMainHandler->WindowToFront(win);
}

