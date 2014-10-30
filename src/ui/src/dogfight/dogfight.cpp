/***************************************************************************\
 UI_dfght.cpp
 Peter Ward
 December 3, 1996
 Main UI screen stuff for FreeFalcon
\***************************************************************************/
#include <windows.h>
#include "falclib.h"
#include "targa.h"
#include "dxutil/ddutil.h"
#include "Graphics/Include/imagebuf.h"
#include "chandler.h"
#include "ui95_ext.h"
#include "entity.h"
#include "feature.h"
#include "vehicle.h"
#include "evtparse.h"
#include "f4vu.h"
#include "falcsess.h"
#include "sim/include/stdhdr.h"
#include "Mesg.h"
#include "MsgInc/DamageMsg.h"
#include "MsgInc/WeaponFireMsg.h"
#include "MsgInc/DeathMessage.h"
#include "MsgInc/MissileEndMsg.h"
#include "MsgInc/LandingMessage.h"
#include "playerop.h"
#include "falcuser.h"
#include "falclib/include/f4find.h"
#include "f4error.h"
#include "team.h"
#include "uicomms.h"
#include "dogfight.h"
#include "userids.h"
#include "iconids.h"
#include "textids.h"
#include "CmpClass.h"
#include "campaign.h"
#include "classtbl.h"
#include "Dispcfg.h"
#include "iconids.h"
#include "logbook.h"
#include "ui_dgfgt.h"
#include "ACSelect.h"
#include "MissEval.h"
#include "Team.h"

#define _USE_REGISTRY_ 1 // 0=No,1=Yes

extern C_Handler *gMainHandler;
extern _TCHAR gUI_ScenarioName[];
extern VU_ID gCurrentFlightID;
extern short gCurrentAircraftNum;
extern int MainLastGroup;
extern int DFLoaded;
extern C_Parser *gMainParser;
extern C_TreeList *DogfightGames;
extern RulesClass CurrRules;

C_Base *gDogfightControl;
C_SoundBite *gDogfightBites = NULL;

// These are prototypes I need since the revision.
extern int CompressCampaignUntilTakeoff(Flight flight);
extern void StartCampaignGame(int local, int game_type);
extern void DisplayJoinStatusWindow(int);

void SetupInfoWindow(void (*tOkCB)(), void (*tCancelCB)());
void AreYouSure(long TitleID, long MessageID, void (*OkCB)(long, short, C_Base*), void (*CancelCB)(long, short, C_Base*));
void AreYouSure(long TitleID, _TCHAR *text, void (*OkCB)(long, short, C_Base*), void (*CancelCB)(long, short, C_Base*));
BOOL CheckExclude(_TCHAR *filename, _TCHAR *directory, _TCHAR *ExcludeList[], _TCHAR *extension);
void VerifyDelete(long TitleID, void (*YesCB)(long, short, C_Base*), void (*NoCB)(long, short, C_Base*));
void CloseWindowCB(long ID, short hittype, C_Base *control);
void ChangeTimeCB(long ID, short hittype, C_Base *control);
void CheckFlyButton();
void CheckDelButtons();
void GenericTimerCB(long ID, short hittype, C_Base *control);
void BlinkCommsButtonTimerCB(long ID, short hittype, C_Base *control);
void ClearDFTeamLists();
short ConvertDFIDtoTeam(long ID);
uchar GetPlaneListID(long ID);
long GetACIDFromFlight(Flight flight);
void DeleteGroupList(long ID);
void GameOver_WaitForEveryone();
void ProcessEventList(C_Window *win, long client);
void DisplayDogfightResults();
void PlayDogfightBite();
static void HookupDogFightControls(long ID);
static void SelectDFSettingsFileCB(long ID, short hittype, C_Base *control);
void SetSingle_Comms_Ctrls();
TREELIST *StartTreeSearch(VU_ID findme, TREELIST *top, C_TreeList *tree);
void GetFileListTree(C_TreeList *tree, _TCHAR *fspec, _TCHAR *excludelist[], long group, BOOL cutext, long UseMenu);
void LoadAFile(long TitleID, _TCHAR *filespec, _TCHAR *excludelist[], void (*YesCB)(long, short, C_Base*), void (*NoCB)(long, short, C_Base*));
void SaveAFile(long TitleID, _TCHAR *filespec, _TCHAR *excludelist[], void (*YesCB)(long, short, C_Base*), void (*NoCB)(long, short, C_Base*), _TCHAR *filename);
void SaveDogfightResults(char *filename);
void UI_Help_Guide_CB(long ID, short hittype, C_Base *ctrl);
BOOL DateCB(C_Base *me);
BOOL FileNameSortCB(TREELIST *list, TREELIST *newitem);
void Uni_Float(_TCHAR *buffer);

void DelSTRFileCB(long ID, short hittype, C_Base *control);
void DelDFSFileCB(long ID, short hittype, C_Base *control);
void DelLSTFileCB(long ID, short hittype, C_Base *control);
void DelCamFileCB(long ID, short hittype, C_Base *control);
void DelTacFileCB(long ID, short hittype, C_Base *control);
void DelTGAFileCB(long ID, short hittype, C_Base *control);
void DelVHSFileCB(long ID, short hittype, C_Base *control);
void DelKeyFileCB(long ID, short hittype, C_Base *control);
void SetDeleteCallback(void (*cb)(long, short, C_Base*));
void OpenTacticalReferenceCB(long ID, short hittype, C_Base *ctrl);
void OpenLogBookCB(long ID, short hittype, C_Base *ctrl);
void OpenCommsCB(long ID, short hittype, C_Base *ctrl);
void OpenSetupCB(long ID, short hittype, C_Base *ctrl);
void ACMIButtonCB(long ID, short hittype, C_Base *ctrl);

void RebuildGameTree();
void CreateDogfightCB(void);
void CancelDogfightCB(void);
void ReallyJoinDogfightCB(void);
void JoinDogfightCB(long ID, short hittype, C_Base *control);
void CopyDFSettingsToWindow(void);
void AddDogfightPlayerCB(long ID, short hittype, C_Base *control);

void AddDogfightAIPlane(Flight flight, int type, int skill, int team);
void LeaveDogfight(void);
void DogfightMenuSetup();

void SendChatStringCB(long ID, short hittype, C_Base *control);

extern FILE* OpenCampFile(char *filename, char *ext, char *mode);

extern bool g_bEmptyFilenameFix; // 2002-04-18 MN

char gCurDogfightFile[MAX_PATH] = "campaign\\save\\New Game.dfs";

_TCHAR *DFExcludeList[] =
{
    "New Game",
    NULL,
};

float MapXtoRatio(C_Cursor* crsr);
float MapYtoRatio(C_Cursor* crsr);
long RatiotoMapX(float ratio, C_Cursor* crsr);
long RatiotoMapY(float ratio, C_Cursor* crsr);

static short AddToTeam = 0;

enum
{
    SND_SCREAM        = 500005,
    SND_BAD1          = 500006,
    SND_SECOND        = 500007,
    SND_FIRST         = 500008,
    SND_NICE          = 500009,
    SND_BAD2          = 500010,
    SND_YOUSUCK       = 500011,
};

enum // Map Icon Res IDs
{
    BLUE_AIR_NORTH = 565121004,
    BLUE_AIR_NORTH_W = 565121005,
    BROWN_AIR_NORTH = 565121020,
    BROWN_AIR_NORTH_W = 565121021,
    GREEN_AIR_NORTH = 565121036,
    GREEN_AIR_NORTH_W = 565121037,
    GREY_AIR_NORTH = 565121052,
    GREY_AIR_NORTH_W = 565121053,
    ORANGE_AIR_NORTH = 565121068,
    ORANGE_AIR_NORTH_W = 565121069,
    RED_AIR_NORTH = 565121084,
    RED_AIR_NORTH_W = 565121085,
    WHITE_AIR_NORTH = 565121100,
    WHITE_AIR_NORTH_W = 565121101,
    YELLOW_AIR_NORTH = 565121116,
    YELLOW_AIR_NORTH_W = 565121117,
    ICON_F16   = 10065,
    ICON_A10   = 10066,
    ICON_AH64   = 10067,
    ICON_AN2  = 10068,
    ICON_B52  = 10069,
    ICON_C130  = 10070,
    ICON_CH47  = 10071,
    ICON_F111 = 10072,
    ICON_F117  = 10073,
    ICON_F14  = 10074,
    ICON_F15C  = 10075,
    ICON_F18  = 10076,
    ICON_F4  = 10077,
    ICON_F5  = 10078,
    ICON_IL28  = 10079,
    ICON_IL76  = 10080,
    ICON_KA50  = 10081,
    ICON_KC10  = 10082,
    ICON_KC135   = 10083,
    ICON_MD500   = 10084,
    ICON_MI24  = 10085,
    ICON_MIG19   = 10086,
    ICON_MIG21   = 10087,
    ICON_MIG23 = 10088,
    ICON_MIG25   = 10089,
    ICON_MIG29   = 10090,
    ICON_OH58  = 10091,
    ICON_SU25  = 10092,
    ICON_SU27  = 10093,
    ICON_TU16N   = 10094,
    ICON_UH1H  = 10095,
    ICON_UH60  = 10096,
    ICON_AN24   = 10097,
    ICON_E3  = 10098,
    ICON_A50   = 10099,
    ICON_F22        = 10122,
    ICON_EA6B       = 10123,
    ICON_TU95       = 10124,
    ICON_B1B        = 10125,
    ICON_UKN        = 10126,
};

uchar calltable[5][5] =
{
    {   0,   0,   0,   0,   0 }, // UFO (No team)
    { CRIMSON_CALL_GROUP1,  CRIMSON_CALL_GROUP2, CRIMSON_CALL_GROUP3, CRIMSON_CALL_GROUP4, CRIMSON_CALL_GROUP5 }, // crimson team
    { SHARK_CALL_GROUP1,  SHARK_CALL_GROUP2, SHARK_CALL_GROUP3, SHARK_CALL_GROUP4, SHARK_CALL_GROUP5 }, // SHARK team
    { VIPER_CALL_GROUP1,  VIPER_CALL_GROUP2, VIPER_CALL_GROUP3, VIPER_CALL_GROUP4, VIPER_CALL_GROUP5 }, // VIPER team
    { TIGER_CALL_GROUP1,  TIGER_CALL_GROUP2, TIGER_CALL_GROUP3, TIGER_CALL_GROUP4, TIGER_CALL_GROUP5 }, // TIGER team
};

static long DFTeamIconResID[5][2] =
{
    { WHITE_AIR_NORTH, WHITE_AIR_NORTH_W }, // UFO Team
    { RED_AIR_NORTH, RED_AIR_NORTH_W }, // Crimson
    { BLUE_AIR_NORTH, BLUE_AIR_NORTH_W }, // Shark
    { WHITE_AIR_NORTH, WHITE_AIR_NORTH_W }, // FreeFalcon Team 
    { ORANGE_AIR_NORTH, ORANGE_AIR_NORTH_W }, // Tiger
};

typedef struct
{
    long ID;
    uchar teamid;
    uchar planeid;
} DF_ID_PLANE;

DF_AIRPLANE_TYPE DFAIPlanesDef[] =
{
    NULL, NULL, NULL, NULL, DF_NO_AC, 0, 0,
    TYPE_AIRPLANE,  STYPE_AIR_ATTACK,        SPTYPE_A10,       STYPE_UNIT_ATTACK, DF_AC_A10,         TXT_A10,        ICON_A10,
    TYPE_AIRPLANE,  STYPE_AIR_AWACS,         SPTYPE_A50,       STYPE_UNIT_AWACS, DF_AC_A50,         TXT_A50,        ICON_A50,
    // TYPE_AIRPLANE,  STYPE_AIR_ATTACK,        SPTYPE_AC130U,    STYPE_UNIT_ATTACK, DF_AC_AC130U,      TXT_AC130U,     ICON_C130,
    TYPE_HELICOPTER, STYPE_AIR_ATTACK_HELO,   SPTYPE_AH64,      STYPE_UNIT_ATTACK_HELO, DF_AC_AH64,        TXT_AH64,       ICON_AH64,
    TYPE_HELICOPTER, STYPE_AIR_ATTACK_HELO,   SPTYPE_AH64D,     STYPE_UNIT_ATTACK_HELO, DF_AC_AH64D,       TXT_AH64D,      ICON_AH64,
    TYPE_AIRPLANE,  STYPE_AIR_TRANSPORT,     SPTYPE_AN2,       STYPE_UNIT_AIR_TRANSPORT, DF_AC_AN2,         TXT_AN2,        ICON_AN2,
    TYPE_AIRPLANE,  STYPE_AIR_TRANSPORT,     SPTYPE_AN24,      STYPE_UNIT_AIR_TRANSPORT, DF_AC_AN24,        TXT_AN24,       ICON_AN24,
    TYPE_AIRPLANE,  STYPE_AIR_BOMBER,        SPTYPE_B1B,       STYPE_UNIT_BOMBER, DF_AC_B1B,         TXT_B1B,        ICON_B1B,
    TYPE_AIRPLANE,  STYPE_AIR_BOMBER,        SPTYPE_B52G,      STYPE_UNIT_BOMBER, DF_AC_B52G,        TXT_B52G,       ICON_B52,
    TYPE_AIRPLANE,  STYPE_AIR_TRANSPORT,     SPTYPE_C130,      STYPE_UNIT_AIR_TRANSPORT, DF_AC_C130,        TXT_C130,       ICON_C130,
    TYPE_HELICOPTER, STYPE_AIR_TRANSPORT_HELO, SPTYPE_CH47,      STYPE_UNIT_TRANSPORT_HELO, DF_AC_CH47,        TXT_CH47,       ICON_CH47,
    TYPE_AIRPLANE,  STYPE_AIR_AWACS,         SPTYPE_E3,        STYPE_UNIT_AWACS, DF_AC_E3,          TXT_E3,         ICON_E3,
    TYPE_AIRPLANE,  STYPE_AIR_ECM,           SPTYPE_EA6B,      STYPE_UNIT_ECM, DF_AC_EA6B,        TXT_EA6B,       ICON_F4,
    TYPE_AIRPLANE,  STYPE_AIR_ECM,           SPTYPE_EF111,     STYPE_UNIT_ECM, DF_AC_EF111,       TXT_EF111,      ICON_F111,
    TYPE_AIRPLANE,  STYPE_AIR_FIGHTER,       SPTYPE_F4E,       STYPE_UNIT_FIGHTER, DF_AC_F4E,         TXT_F4E,        ICON_F4,
    TYPE_AIRPLANE,  STYPE_AIR_ATTACK,        SPTYPE_F4G,       STYPE_UNIT_ATTACK, DF_AC_F4G,         TXT_F4G,        ICON_F4,
    TYPE_AIRPLANE,  STYPE_AIR_FIGHTER,       SPTYPE_F5E,       STYPE_UNIT_FIGHTER, DF_AC_F5E,         TXT_F5E,        ICON_F5,
    TYPE_AIRPLANE,  STYPE_AIR_FIGHTER,       SPTYPE_F14A,      STYPE_UNIT_FIGHTER, DF_AC_F14A,        TXT_F14A,       ICON_F14,
    TYPE_AIRPLANE,  STYPE_AIR_FIGHTER,       SPTYPE_F15C,      STYPE_UNIT_FIGHTER, DF_AC_F15C,        TXT_F15C,       ICON_F15C,
    TYPE_AIRPLANE,  STYPE_AIR_FIGHTER_BOMBER, SPTYPE_F15E,      STYPE_UNIT_FIGHTER_BOMBER, DF_AC_F15E,        TXT_F15E,       ICON_F15C,
    TYPE_AIRPLANE,  STYPE_AIR_FIGHTER_BOMBER, SPTYPE_F16C,      STYPE_UNIT_FIGHTER_BOMBER, DF_AC_F16C,        TXT_F16C,       ICON_F16,
    TYPE_AIRPLANE,  STYPE_AIR_FIGHTER_BOMBER, SPTYPE_F18A,      STYPE_UNIT_FIGHTER_BOMBER, DF_AC_F18A,        TXT_F18A,       ICON_F18,
    TYPE_AIRPLANE,  STYPE_AIR_FIGHTER_BOMBER, SPTYPE_F18D,      STYPE_UNIT_FIGHTER_BOMBER, DF_AC_F18D,        TXT_F18D,       ICON_F18,
    TYPE_AIRPLANE,  STYPE_AIR_FIGHTER,       SPTYPE_F22,       STYPE_UNIT_FIGHTER, DF_AC_F22,         TXT_F22,        ICON_F22,
    TYPE_AIRPLANE,  STYPE_AIR_FIGHTER_BOMBER, SPTYPE_F117,      STYPE_UNIT_FIGHTER_BOMBER, DF_AC_F117,        TXT_F117,       ICON_F117,
    TYPE_AIRPLANE,  STYPE_AIR_ATTACK,        SPTYPE_FB111,     STYPE_UNIT_ATTACK, DF_AC_FB111,       TXT_FB111,      ICON_F111,
    TYPE_AIRPLANE,  STYPE_AIR_ATTACK,        SPTYPE_IL28,      STYPE_UNIT_ATTACK, DF_AC_IL28,        TXT_IL28,       ICON_IL28,
    TYPE_AIRPLANE,  STYPE_AIR_TRANSPORT,     SPTYPE_IL76M,     STYPE_UNIT_AIR_TRANSPORT, DF_AC_IL76M,       TXT_IL76M,      ICON_IL76,
    TYPE_HELICOPTER, STYPE_AIR_ATTACK_HELO,   SPTYPE_KA50,      STYPE_UNIT_ATTACK_HELO, DF_AC_KA50,        TXT_KA50,       ICON_KA50,
    TYPE_AIRPLANE,  STYPE_AIR_TANKER,        SPTYPE_KC10,      STYPE_UNIT_TANKER, DF_AC_KC10,        TXT_KC10,       ICON_KC10,
    TYPE_AIRPLANE,  STYPE_AIR_TANKER,        SPTYPE_IL78,      STYPE_UNIT_TANKER, DF_AC_IL78,        TXT_IL78,       ICON_IL76,
    TYPE_AIRPLANE,  STYPE_AIR_TANKER,        SPTYPE_KC135,     STYPE_UNIT_TANKER, DF_AC_KC135,       TXT_KC135,      ICON_KC135,
    TYPE_HELICOPTER, STYPE_AIR_RECON_HELO,    SPTYPE_MD500,     STYPE_UNIT_RECON_HELO, DF_AC_MD500,       TXT_MD500,      ICON_MD500,
    TYPE_HELICOPTER, STYPE_AIR_ATTACK_HELO,   SPTYPE_MI24,      STYPE_UNIT_ATTACK_HELO, DF_AC_MI24,        TXT_MI24,       ICON_MI24,
    TYPE_AIRPLANE,  STYPE_AIR_FIGHTER,       SPTYPE_MIG19,     STYPE_UNIT_FIGHTER, DF_AC_MIG19,       TXT_MIG19,      ICON_MIG19,
    TYPE_AIRPLANE,  STYPE_AIR_FIGHTER,       SPTYPE_MIG21,     STYPE_UNIT_FIGHTER, DF_AC_MIG21,       TXT_MIG21,      ICON_MIG21,
    TYPE_AIRPLANE,  STYPE_AIR_FIGHTER,       SPTYPE_MIG23MS,   STYPE_UNIT_FIGHTER, DF_AC_MIG23MS,     TXT_MIG23MS,    ICON_MIG23,
    TYPE_AIRPLANE,  STYPE_AIR_FIGHTER,       SPTYPE_MIG25,     STYPE_UNIT_FIGHTER, DF_AC_MIG25,       TXT_MIG25,      ICON_MIG25,
    TYPE_AIRPLANE,  STYPE_AIR_FIGHTER,       SPTYPE_MIG29,     STYPE_UNIT_FIGHTER, DF_AC_MIG29,       TXT_MIG29,      ICON_MIG29,
    TYPE_HELICOPTER, STYPE_AIR_RECON_HELO,    SPTYPE_OH58D,     STYPE_UNIT_RECON_HELO, DF_AC_OH58D,       TXT_OH58D,      ICON_OH58,
    TYPE_AIRPLANE,  STYPE_AIR_ATTACK,        SPTYPE_SU25,      STYPE_UNIT_ATTACK, DF_AC_SU25,        TXT_SU25,       ICON_SU25,
    TYPE_AIRPLANE,  STYPE_AIR_FIGHTER,       SPTYPE_SU27,      STYPE_UNIT_FIGHTER, DF_AC_SU27,        TXT_SU27,       ICON_SU27,
    TYPE_AIRPLANE,  STYPE_AIR_BOMBER,        SPTYPE_TU16,      STYPE_UNIT_BOMBER, DF_AC_TU16,        TXT_TU16,       ICON_TU16N,
    TYPE_AIRPLANE,  STYPE_AIR_TANKER,        SPTYPE_TU16N,     STYPE_UNIT_TANKER, DF_AC_TU16N,       TXT_TU16N,      ICON_TU16N,
    TYPE_AIRPLANE,  STYPE_AIR_BOMBER,        SPTYPE_TU95,      STYPE_UNIT_BOMBER, DF_AC_TU95,        TXT_TU95,       ICON_TU95,
    TYPE_HELICOPTER, STYPE_AIR_TRANSPORT_HELO, SPTYPE_UH1N,      STYPE_UNIT_TRANSPORT_HELO, DF_AC_UH1N,        TXT_UH1N,       ICON_UH1H,
    TYPE_HELICOPTER, STYPE_AIR_TRANSPORT_HELO, SPTYPE_UH60L,     STYPE_UNIT_TRANSPORT_HELO, DF_AC_UH60L,       TXT_UH60L,      ICON_UH60,
    NULL, NULL, NULL, NULL, 0, 0, 0
};

DF_AIRPLANE_TYPE *DFAIPlanes;

void LoadDfPlanes()
{
    if (DFAIPlanes not_eq NULL)
        return;

    FILE *fp = OpenCampFile("teplanes", "lst", "r");

    if (fp == NULL)
    {
        fp = OpenCampFile("te-planes", "lst", "r");
    }

    if (fp == NULL)
    {
        DFAIPlanes = DFAIPlanesDef;
        fp = OpenCampFile("teplanes", "lst", "w");
        fprintf(fp, "// Type SubType Specific UnitSubType    ID text UnitIcon\n");

        for (int i = 0; DFAIPlanes[i].ID not_eq 0; i++)
        {
            fprintf(fp, "%7d %7d %8d %11d %5d %4d %8d\n",
                    DFAIPlanes[i].Type,
                    DFAIPlanes[i].SType,
                    DFAIPlanes[i].SPType,
                    DFAIPlanes[i].UnitSType,
                    DFAIPlanes[i].ID,
                    DFAIPlanes[i].TextID,
                    DFAIPlanes[i].IconID);
        }

        fclose(fp);
        return;
    }

    int maxdf = 0, curdf = 0;
    char buf[128];

    while (fgets(buf, sizeof buf, fp))
    {
        if (buf[0] == '#' or buf[0] == '/' or buf[0] == '\r' or buf[0] == '\n')
            continue;

        if (curdf >= maxdf - 2)   // time to grow the array
        {
            if (DFAIPlanes == NULL)
                DFAIPlanes = (DF_AIRPLANE_TYPE*)calloc(maxdf = 10, sizeof * DFAIPlanes);
            else
            {
                maxdf *= 2;
                DFAIPlanes = (DF_AIRPLANE_TYPE*)realloc(DFAIPlanes, maxdf * sizeof(*DFAIPlanes));
            }
        }

        int Type, SType, SPType, UnitSType;

        if (sscanf(buf, "%7d %7d %8d %11d %5ld %4ld %8ld",
                   &Type,
                   &SType,
                   &SPType,
                   &UnitSType,
                   &DFAIPlanes[curdf].ID,
                   &DFAIPlanes[curdf].TextID,
                   &DFAIPlanes[curdf].IconID) not_eq 7)
        {
            ShiWarning("Bad format file teplanes.lst");
            free(DFAIPlanes);
            DFAIPlanes = DFAIPlanesDef;
            break;
        }

        DFAIPlanes[curdf].Type = Type;
        DFAIPlanes[curdf].SType = SType;
        DFAIPlanes[curdf].SPType = SPType;
        DFAIPlanes[curdf].UnitSType = UnitSType;

        curdf ++;
        DFAIPlanes[curdf].Type = 0;
        DFAIPlanes[curdf].ID = 0; // JB 010531
    }

    fclose(fp);
}




// If we are playing a game... we need this type of squadron... don't make new ones
BOOL FindSquadronType(long ClassID, long teamid)
{
    VuListIterator iter(AllCampList);
    CampEntity entity;

    CampEnterCriticalSection();

    entity = GetFirstEntity(&iter);

    while (entity)
    {
        if (entity->GetTeam() == teamid and entity->IsSquadron() and not entity->IsDead())
        {
            if (entity->Type() == ClassID)
            {
                CampLeaveCriticalSection();
                return(TRUE);
            }
        }

        entity = GetNextEntity(&iter);
    }

    CampLeaveCriticalSection();

    return(FALSE);
}
// This routine is used in TAC_ENG to fill the AC Type listbox in the flight window

void FillListBoxWithACTypes(C_ListBox *lbox)
{
    short i;
    long ID;

    i = 1;

    LoadDfPlanes();

    while (DFAIPlanes and DFAIPlanes[i].Type) //ctd fix
    {
        ID = GetClassID(DOMAIN_AIR, CLASS_UNIT, TYPE_SQUADRON, DFAIPlanes[i].UnitSType, DFAIPlanes[i].SPType, VU_ANY, VU_ANY, VU_ANY);

        if (ID)
        {
            ID += VU_LAST_ENTITY_TYPE;

            if ( not (TheCampaign.Flags bitand CAMP_TACTICAL_EDIT))
            {
                if (FindSquadronType(ID, FalconLocalSession->GetTeam()))
                    lbox->AddItem(ID, C_TYPE_ITEM, DFAIPlanes[i].TextID);
            }
            else
                lbox->AddItem(ID, C_TYPE_ITEM, DFAIPlanes[i].TextID);
        }

        i++;
    }
}















// =====================================================
//
// KCK: Join/Host/Leave functions
//
// In dogfight these are horribly intermixed with the
// selection of aircraft.
// What I'm trying to do is separate the two - making
// joining a two step process.
//
// =====================================================

void SetCurrentGameState(C_TreeList *tree, short state)
{
    TREELIST *group;

    if (gCommsMgr->Online() and tree)
    {
        if (vuLocalGame not_eq vuPlayerPoolGroup)
        {
            group = StartTreeSearch(FalconLocalGame->Id(), tree->GetRoot(), tree);

            if (group and group->Item_)
            {
                group->Item_->SetState(state);
                group->Item_->Refresh();
            }
        }
    }
}

// This is what we call when we want to join an existing game
void JoinDogfightCB(long, short hittype, C_Base *)
{
    FalconGameEntity *game;
    C_Window *win;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    if (gCommsMgr == NULL)
        return;

    if ( not gCommsMgr->Online())
    {
        win = gMainHandler->FindWindow(PB_WIN);

        if (win)
            gMainHandler->EnableWindowGroup(win->GetGroup());

        return;
    }

    game = (FalconGameEntity*)gCommsMgr->GetTargetGame();

    if ( not game)
        return;

    win = gMainHandler->FindWindow(INFO_WIN);

    if (win)
    {
        SetupInfoWindow(ReallyJoinDogfightCB, CancelDogfightCB);
    }
}

// These are called as a callback after we've set up our game rules
void CreateDogfightCB(void)
{
    _tcscpy(gUI_ScenarioName, "Instant");
    StartCampaignGame(TRUE, game_Dogfight);
}

void CancelDogfightCB(void)
{
}

void GameHasStarted(void)
{
    AreYouSure(TXT_ERROR, TXT_GAMEHASSTARTED, NULL, CloseWindowCB);
}

void ReallyJoinDogfightCB(void)
{
    // if ((SimDogfight.GetDogfightGameStatus() >= dog_Starting) and (SimDogfight.GetGameType() == dog_TeamMatchplay))
    // {
    // GameHasStarted();
    // return;
    // }

    MonoPrint("Joining Dogfight game.\n");

    DisplayJoinStatusWindow(0);

    StartCampaignGame(FALSE, game_Dogfight);
}

static void DogfightBeginCB(long ID, short hittype, C_Base *control)
{
    FalconGameEntity *game;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    if (gCommsMgr == NULL)
        return;

    gDogfightControl = control;

    // This is the callback made by the 2 icons on the LOAD toolbar...
    // Figure out which function to call based upon what the user
    // has done

    // KCK: Really, we shouldn't do anything (except maybe show a
    // 'connecting to game' dialog for the multiplayer case) UNTIL
    // we've successfully joined/loaded the game.
    game = (FalconGameEntity*)gCommsMgr->GetTargetGame();

    if (game)
    {
        // We've got a game selected, so actually, we're joining
        // Send the join request
        MonoPrint("Joining Dogfight game. \n");
        JoinDogfightCB(ID, hittype, control);
    }
    else
    {
        // Either call create dogfight for local case, or pop up
        // a rules dialog and then create a dogfight.
        MonoPrint("Create new Dogfight game. \n");

        if (gCommsMgr->Online())
            SetupInfoWindow(CreateDogfightCB, CancelDogfightCB);
        else
            CreateDogfightCB();
    }

    // KCK: Everything else is done as a result of a successfull load,
    // which calls the function below...
}

// KCK: Called as a result of a successfull dogfight game load
void DogfightJoinSuccess(void)
{
    C_Window
    *win;

    F4CSECTIONHANDLE *Leave;

    F4Assert(gDogfightControl);

    Leave = UI_Enter(gDogfightControl->Parent_);
    gMainHandler->DisableWindowGroup(100);
    gMainHandler->DisableWindowGroup(gDogfightControl->GetParent()->GetGroup());
    gMainHandler->EnableWindowGroup(gDogfightControl->GetGroup());

    win = gMainHandler->FindWindow(COMMLINK_WIN);

    if (win)
    {
        gMainHandler->HideWindow(win);
    }

    gDogfightControl = NULL;

    SetCurrentGameState(DogfightGames, C_STATE_2);
    AddDogfightPlayerCB(DF_CRIMSON, C_TYPE_LMOUSEUP, NULL);
    UI_Leave(Leave);
}

// ====================================================
//
// KCK: Aircraft selection functions
//
// ====================================================

// This is the 'add player' button callback
// Will attempt to add player to currently selected flight, or create a new one
// NOTE: control can be NULL so DON'T use it
void AddDogfightPlayerCB(long ID, short hittype, C_Base *)
{
    Flight flight;
    int idx, type;
    uchar teamid;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    // If we're not yet in a game, this equates to host
    if ( not FalconLocalGame or FalconLocalSession->Game() == vuPlayerPoolGroup)
        return;

    LoadDfPlanes();
    flight = (Flight)vuDatabase->Find(gCurrentFlightID);
    teamid = static_cast<uchar>(ConvertDFIDtoTeam(ID));
    idx = GetPlaneListID(DF_AC_F16C);
    type = GetClassID(DOMAIN_AIR, CLASS_UNIT, TYPE_FLIGHT, DFAIPlanes[idx].UnitSType, DFAIPlanes[idx].SPType, VU_ANY, VU_ANY, VU_ANY) + VU_LAST_ENTITY_TYPE;

    // For the furball buttons:
    // a) Change team of selected flight   OR
    // b) Request a new player flight
    if (ID == DF_MARK_CRIMSON or ID == DF_MARK_SHARK or ID == DF_MARK_VIPER or ID == DF_MARK_TIGER)
    {
        if (flight)
            RequestTeamChange(flight, teamid);
        else
            RequestACSlot(NULL, teamid, 0, 0, type, 1);
    }
    // For non-furball buttons:
    // a) Request to join selected flight if same team  OR
    // b) Request a new player flight
    else
    {
        if (flight and flight->GetTeam() == teamid)
            RequestACSlot(flight, flight->GetTeam(), 0, 0, (flight->Type() - VU_LAST_ENTITY_TYPE), 1);
        else
            RequestACSlot(NULL, teamid, 0, 0, type, 1);
    }
}

// Will attempt to add an AI aircraft to the currently selected flight
void AddDogfightAIPlane(Flight flight, int type, int skill, int team)
{
    RequestACSlot(flight, static_cast<uchar>(team), 0, static_cast<uchar>(skill), type, 0);
}

// Will attempt to remove currently selected aircraft/player
void RemoveDogfightPlane()
{
    Flight flight;

    // gCurrentFlightID is the ID of the SELECTED flight
    // gCurrentAircraftNum is the ID of the SELECTED aircraft (or potentially, first AI in flight)
    flight = (Flight)vuDatabase->Find(gCurrentFlightID);

    if (flight)
        LeaveACSlot(flight, static_cast<uchar>(gCurrentAircraftNum));

    gCurrentAircraftNum = -1;
    CheckDelButtons();
}

void ClearAllTreeStates()
{
    C_Window *win;
    C_TreeList *tree;

    win = gMainHandler->FindWindow(DF_TEAM_WIN);

    if (win)
    {
        tree = (C_TreeList*)win->FindControl(FURBALL_TREE);

        if (tree)
            tree->SetAllControlStates(0, tree->GetRoot());

        tree = (C_TreeList*)win->FindControl(CRIMSON_TREE);

        if (tree)
            tree->SetAllControlStates(0, tree->GetRoot());

        tree = (C_TreeList*)win->FindControl(SHARK_TREE);

        if (tree)
            tree->SetAllControlStates(0, tree->GetRoot());

        tree = (C_TreeList*)win->FindControl(TIGER_TREE);

        if (tree)
            tree->SetAllControlStates(0, tree->GetRoot());

        tree = (C_TreeList*)win->FindControl(VIPER_TREE);

        if (tree)
            tree->SetAllControlStates(0, tree->GetRoot());
    }
}

void CheckDelButtons()
{
    Flight flight;
    C_Window *win;
    C_Button *btn;

    win = gMainHandler->FindWindow(DF_TEAM_WIN);

    if (win)
    {
        btn = (C_Button*)win->FindControl(DEL_FURBALL_PLANE);

        if (btn)
        {
            btn->SetFlagBitOff(C_BIT_ENABLED);
            btn->Refresh();
        }

        btn = (C_Button*)win->FindControl(DEL_CRIMSON_PLANE);

        if (btn)
        {
            btn->SetFlagBitOff(C_BIT_ENABLED);
            btn->Refresh();
        }

        btn = (C_Button*)win->FindControl(DEL_SHARK_PLANE);

        if (btn)
        {
            btn->SetFlagBitOff(C_BIT_ENABLED);
            btn->Refresh();
        }

        btn = (C_Button*)win->FindControl(DEL_TBIRD_PLANE);

        if (btn)
        {
            btn->SetFlagBitOff(C_BIT_ENABLED);
            btn->Refresh();
        }

        btn = (C_Button*)win->FindControl(DEL_TIGER_PLANE);

        if (btn)
        {
            btn->SetFlagBitOff(C_BIT_ENABLED);
            btn->Refresh();
        }

        if (gCurrentFlightID == FalconNullId or gCurrentAircraftNum < 0)
            return;

        flight = (Flight)vuDatabase->Find(gCurrentFlightID);

        if (flight)
        {
            if (flight->player_slots[gCurrentAircraftNum] == 255)
            {
                switch (SimDogfight.GetGameType())
                {
                    case dog_TeamFurball:
                    case dog_TeamMatchplay:
                        switch (flight->GetTeam())
                        {
                            case 1:
                                btn = (C_Button*)win->FindControl(DEL_CRIMSON_PLANE);
                                break;

                            case 2:
                                btn = (C_Button*)win->FindControl(DEL_SHARK_PLANE);
                                break;

                            case 3:
                                btn = (C_Button*)win->FindControl(DEL_TBIRD_PLANE);
                                break;

                            case 4:
                                btn = (C_Button*)win->FindControl(DEL_TIGER_PLANE);
                                break;
                        }

                        if (btn)
                        {
                            btn->SetFlagBitOn(C_BIT_ENABLED);
                            btn->Refresh();
                        }

                        break;

                    default:
                        btn = (C_Button*)win->FindControl(DEL_FURBALL_PLANE);

                        if (btn)
                        {
                            btn->SetFlagBitOn(C_BIT_ENABLED);
                            btn->Refresh();
                        }

                        break;
                }
            }
        }
    }
}

void SelectDogfightFlightCB(long, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    if (((C_Dog_Flight*)control)->GetVUID() == gCurrentFlightID)
    {
        gCurrentFlightID = FalconNullId;
        control->SetState(0);
    }
    else
    {
        gCurrentFlightID = ((C_Dog_Flight*)control)->GetVUID();
        control->SetState(1);
    }

    gCurrentAircraftNum = -1;
}

void SelectDogfightPilotCB(long, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    if (((C_Pilot*)control)->GetVUID() == gCurrentFlightID)
    {
        gCurrentAircraftNum = -1;
        gCurrentFlightID = FalconNullId;
        control->SetState(0);
    }
    else
    {
        gCurrentFlightID = ((C_Pilot*)control)->GetVUID();
        gCurrentAircraftNum = ((C_Pilot*)control)->GetSlot();
        control->SetState(1);
    }
}

// Tree callback ONLY
void SelectDogfightItemCB(long, short hittype, C_Base *control)
{
    C_TreeList *tree;
    TREELIST *item;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    tree = (C_TreeList *)control;

    if ( not tree)
        return;

    item = tree->GetLastItem();

    if (item)
    {
        ClearAllTreeStates();

        if (item->Type_ == C_TYPE_MENU)
            SelectDogfightFlightCB(item->ID_, hittype, item->Item_);
        else if (item->Type_ == C_TYPE_ITEM)
        {
            item->Parent->Item_->SetState(1);
            SelectDogfightPilotCB(item->ID_, hittype, item->Item_);
        }
    }

    CheckDelButtons();
}

// ==============================================
//
// KCK: Aircraft list building stuff
//
// ==============================================

// Init the C_Pilot structure for a player
C_Pilot *MakePilot(C_TreeList *list, Flight flight, FalconSessionEntity *session, int acnum, int skill)
{
    long team, ACID;
    long idx;
    C_Resmgr *iconresdark, *iconreslite;
    C_Pilot *newpilot;
    _TCHAR callbuf[40];
    _TCHAR      acefactor[10];

    team = flight->GetTeam();
    ACID = GetACIDFromFlight(flight);

    idx = GetPlaneListID(ACID);
    iconresdark = gImageMgr->GetImageRes(DFTeamIconResID[team][0]);
    iconreslite = gImageMgr->GetImageRes(DFTeamIconResID[team][1]);

    newpilot = new C_Pilot;
    ShiAssert(newpilot);
    newpilot->Setup(C_DONT_CARE, 0);
    newpilot->SetVUID(flight->Id());

    // GetCallsign (flight->callsign_id, flight->callsign_num, buffer);
    // Set Callsign
    switch (skill)
    {
        case -1:
            _tcscpy(callbuf, session->GetPlayerCallsign());
            _stprintf(acefactor, "(%4.2f)", session->GetAceFactor());
            Uni_Float(acefactor);
            _tcscat(callbuf, acefactor);
            break;

        case 0:
            _stprintf(callbuf, "%s", gStringMgr->GetString(TXT_RECRUIT));
            break;

        case 1:
            _stprintf(callbuf, "%s", gStringMgr->GetString(TXT_CADET));
            break;

        case 2:
            _stprintf(callbuf, "%s", gStringMgr->GetString(TXT_ROOKIE));
            break;

        case 3:
            _stprintf(callbuf, "%s", gStringMgr->GetString(TXT_VETERAN));
            break;

        case 4:
            _stprintf(callbuf, "%s", gStringMgr->GetString(TXT_ACE));
            break;
    }

    newpilot->SetSkill(static_cast<short>(skill));
    newpilot->SetCallsign(0, 0, callbuf);
    newpilot->SetFont(list->GetFont());
    newpilot->SetParent(list->GetParent());
    newpilot->SetSubParents(list->GetParent());
    newpilot->SetSlot(static_cast<short>(acnum));

    if (skill == -1)
    {
        if (strstr(LogBook.Callsign(), "Super Fly"))
            newpilot->SetMenu(DF_AI_PILOT_POPUP);
        else
            newpilot->SetMenu(DF_PLAYER_POPUP);
    }
    else
        newpilot->SetMenu(DF_AI_PILOT_POPUP);

    return(newpilot);
}

// Init the C_Pilot structure for a flight
C_Dog_Flight *MakeFlight(C_TreeList *list, Flight flight)
{
    long team, ACID;
    long idx, w, h;
    C_Dog_Flight *newflt;
    C_Resmgr *iconresdark, *iconreslite;
    IMAGE_RSC *dark = NULL, *lite = NULL;
    _TCHAR callbuf[40];

    LoadDfPlanes();
    team = flight->GetTeam();
    ACID = GetACIDFromFlight(flight);

    idx = GetPlaneListID(ACID);
    iconresdark = gImageMgr->GetImageRes(DFTeamIconResID[team][0]);
    iconreslite = gImageMgr->GetImageRes(DFTeamIconResID[team][1]);

    newflt = new C_Dog_Flight;
    ShiAssert(newflt);
    newflt->Setup(C_DONT_CARE, 0);
    newflt->SetVUID(flight->Id());

    // Find AC_Icon in database
    if (iconresdark)
    {
        dark = (IMAGE_RSC*)iconresdark->Find(DFAIPlanes[idx].IconID);

        if ( not dark or dark->Header->Type not_eq _RSC_IS_IMAGE_)
            dark = NULL;
    }

    if (iconreslite)
    {
        lite = (IMAGE_RSC*)iconreslite->Find(DFAIPlanes[idx].IconID);

        if ( not lite or lite->Header->Type not_eq _RSC_IS_IMAGE_)
            lite = NULL;
    }

    if (dark)
    {
        w = dark->Header->w;
        h = dark->Header->h;
        newflt->SetIcon(static_cast<short>(18 - w / 2), static_cast<short>(15 - h / 2), dark, lite);
    }
    else
        newflt->SetIcon(0, 0, dark, lite);

    GetCallsign(flight->callsign_id, flight->callsign_num, callbuf);
    newflt->SetCallsign(34, 2, callbuf);

    _stprintf(callbuf, "%1ld %s", flight->GetTotalVehicles(), gStringMgr->GetString(DFAIPlanes[idx].TextID));

    newflt->SetAircraft(34, 16, callbuf);
    newflt->SetVUID(flight->Id());
    newflt->SetFont(list->GetFont());
    newflt->SetParent(list->GetParent());
    newflt->SetSubParents(list->GetParent());
    newflt->SetMenu(DF_FLIGHT_POPUP);
    return(newflt);
}

// Update the C_Pilot structure for a flight
void UpdateFlight(C_Dog_Flight *newflt, Flight flight)
{
    long team, ACID;
    long idx, w, h;
    C_Resmgr *iconresdark, *iconreslite;
    IMAGE_RSC *dark = NULL, *lite = NULL;
    _TCHAR callbuf[40];

    if ( not newflt)
        return;

    // 2002-03-02 ADDED BY S.G. HACK so MP dogfights client sets their class_ptr since they don't get messages to set it.
    flight->class_data = (UnitClassDataType*) Falcon4ClassTable[flight->Type() - VU_LAST_ENTITY_TYPE].dataPtr;
    // END OF ADDED SECTION

    LoadDfPlanes();
    team = flight->GetTeam();
    ACID = GetACIDFromFlight(flight);

    idx = GetPlaneListID(ACID);
    iconresdark = gImageMgr->GetImageRes(DFTeamIconResID[team][0]);
    iconreslite = gImageMgr->GetImageRes(DFTeamIconResID[team][1]);

    newflt->Setup(C_DONT_CARE, 0);
    newflt->SetVUID(flight->Id());

    // Find AC_Icon in database
    if (iconresdark)
    {
        dark = (IMAGE_RSC*)iconresdark->Find(DFAIPlanes[idx].IconID);

        if (dark->Header->Type not_eq _RSC_IS_IMAGE_)
            dark = NULL;
    }

    if (iconreslite)
    {
        lite = (IMAGE_RSC*)iconreslite->Find(DFAIPlanes[idx].IconID);

        if ( not lite or lite->Header->Type not_eq _RSC_IS_IMAGE_)
            lite = NULL;
    }

    if (dark)
    {
        w = dark->Header->w;
        h = dark->Header->h;
        newflt->SetIcon(static_cast<short>(18 - w / 2), static_cast<short>(15 - h / 2), dark, lite);
    }
    else
        newflt->SetIcon(0, 0, dark, lite);

    GetCallsign(flight->callsign_id, flight->callsign_num, callbuf);
    newflt->SetCallsign(34, 2, callbuf);

    _stprintf(callbuf, "%1ld %s", flight->GetTotalVehicles(), gStringMgr->GetString(DFAIPlanes[idx].TextID));

    newflt->SetAircraft(34, 16, callbuf);
    newflt->SetVUID(flight->Id());
}

// Construct a cpilot control and add it to tree
C_Pilot *AddDogfightPilot(C_TreeList *list, Flight flight, int ac)
{
    C_Pilot *pilot;
    C_Dog_Flight *fltctrl;
    TREELIST *item, *flt, *cur;
    _TCHAR *str;
    long ID, count;
    int found = 0;

    // Create a unique ID
    ID  = flight->GetTeam() << 24;
    ID or_eq flight->callsign_id << 16;
    ID or_eq flight->callsign_num << 8;
    ID or_eq (ac + 1);

    if ( not list)
        return(NULL);

    item = list->Find(ID);

    if (item)
    {
        pilot = (C_Pilot*)item->Item_;

        if (pilot)
        {
            if ( not pilot->GetPlayer())
            {
                if (pilot->GetSkill() not_eq flight->pilots[ac])
                {
                    switch (flight->pilots[ac])
                    {
                        case 0:
                            pilot->SetCallsign(0, 0, gStringMgr->GetString(TXT_RECRUIT));
                            break;

                        case 1:
                            pilot->SetCallsign(0, 0, gStringMgr->GetString(TXT_CADET));
                            break;

                        case 2:
                            pilot->SetCallsign(0, 0, gStringMgr->GetString(TXT_ROOKIE));
                            break;

                        case 3:
                            pilot->SetCallsign(0, 0, gStringMgr->GetString(TXT_VETERAN));
                            break;

                        case 4:
                            pilot->SetCallsign(0, 0, gStringMgr->GetString(TXT_ACE));
                            break;
                    }
                }
            }

            return(pilot);
        }
    }

    flt = list->Find(ID bitand 0xffffff00);

    if ( not flt)
        return(NULL);

    if (flight->player_slots[ac] not_eq 255)
    {
        if (FalconLocalGame)
        {
            // Add a player pilot
            FalconSessionEntity *session;
            VuSessionsIterator sit(FalconLocalGame);

            // Find this player's session
            session = (FalconSessionEntity*) sit.GetFirst();

            while (session and not found)
            {
                if (session->GetAircraftNum() == ac and session->GetPlayerFlight() == flight and session->GetPilotSlot() == flight->player_slots[ac])
                    found = 1;
                else
                    session = (FalconSessionEntity*) sit.GetNext();
            }

            if ( not found)
                return(NULL);

            pilot = MakePilot(list, flight, session, ac, -1);
            pilot->SetPlayer(1);
        }
        else
        {
            return NULL;
        }
    }
    else
    {
        // Add an AI pilot

        pilot = MakePilot(list, flight, NULL, ac, flight->pilots[ac]);
        pilot->SetPlayer(0);
    }

    if ( not pilot)
        return(NULL);

    item = list->CreateItem(ID, C_TYPE_ITEM, pilot);
    list->AddChildItem(flt, item);

    // Kludge to update the # planes in dogfight
    fltctrl = (C_Dog_Flight*)flt->Item_;

    if (fltctrl)
    {
        count = 0;
        cur = flt->Child;

        while (cur)
        {
            count++;
            cur = cur->Next;
        };

        if (fltctrl->GetAircraft())
        {
            str = fltctrl->GetAircraft()->GetText();

            if (str)
                str[0] = static_cast<char>(count + '0');
        }
    }

    list->RecalcSize();

    if (list->GetParent())
        list->GetParent()->RefreshClient(list->GetClient());

    return(pilot);
}

C_Dog_Flight *AddDogfightFlight(C_TreeList *list, Flight flight)
{
    C_Dog_Flight *dfflight;
    TREELIST *item;
    long ID;

    // Create a unique ID
    ID  = flight->GetTeam() << 24;
    ID or_eq flight->callsign_id << 16;
    ID or_eq flight->callsign_num << 8;

    if ( not list)
        return(NULL);

    item = list->Find(ID);

    if (item)
    {
        UpdateFlight((C_Dog_Flight*)item->Item_, flight);
        return((C_Dog_Flight*)item->Item_);
    }

    dfflight = MakeFlight(list, flight);

    if ( not dfflight)
        return(NULL);

    item = list->CreateItem(ID, C_TYPE_MENU, dfflight);
    list->AddItem(list->GetRoot(), item);
    list->RecalcSize();

    if (list->GetParent())
        list->GetParent()->RefreshClient(list->GetClient());

    return(dfflight);
}

void EraseOldLimbs(C_TreeList *tree, TREELIST *first, long timestamp)
{
    TREELIST *item, *cur;

    cur = first;

    while (cur)
    {
        item = cur;
        cur = cur->Next;

        if (item->Item_ and item->Item_->GetUserNumber(0) not_eq timestamp)
        {
            if (item->Child)
                tree->DeleteBranch(item->Child);

            tree->DeleteItem(item);
        }
        else if (item->Child)
            EraseOldLimbs(tree, item->Child, timestamp);
    }
}

void ClearOldDFInfo(long timestamp)
{
    C_Window *win;
    C_TreeList *tree;

    win = gMainHandler->FindWindow(DF_TEAM_WIN);

    if (win)
    {
        tree = (C_TreeList *)win->FindControl(FURBALL_TREE);

        if (tree)
        {
            EraseOldLimbs(tree, tree->GetRoot(), timestamp);
            tree->RecalcSize();

            if (tree->Parent_)
                tree->Parent_->RefreshClient(tree->GetClient());
        }

        tree = (C_TreeList *)win->FindControl(CRIMSON_TREE);

        if (tree)
        {
            EraseOldLimbs(tree, tree->GetRoot(), timestamp);
            tree->RecalcSize();

            if (tree->Parent_)
                tree->Parent_->RefreshClient(tree->GetClient());
        }

        tree = (C_TreeList *)win->FindControl(SHARK_TREE);

        if (tree)
        {
            EraseOldLimbs(tree, tree->GetRoot(), timestamp);
            tree->RecalcSize();

            if (tree->Parent_)
                tree->Parent_->RefreshClient(tree->GetClient());
        }

        tree = (C_TreeList *)win->FindControl(TIGER_TREE);

        if (tree)
        {
            EraseOldLimbs(tree, tree->GetRoot(), timestamp);
            tree->RecalcSize();

            if (tree->Parent_)
                tree->Parent_->RefreshClient(tree->GetClient());
        }

        tree = (C_TreeList *)win->FindControl(VIPER_TREE);

        if (tree)
        {
            EraseOldLimbs(tree, tree->GetRoot(), timestamp);
            tree->RecalcSize();

            if (tree->Parent_)
                tree->Parent_->RefreshClient(tree->GetClient());
        }
    }
}

void BuildDFPlayerList()
{
    VuListIterator listit(AllRealList);
    Unit unit;
    Flight flight;
    int ac, team;
    C_Window *win;
    C_TreeList *flist, *tlist;
    C_Pilot *furplt, *teamplt;
    C_Dog_Flight *furflt, *teamflt;
    long timestamp;

    if ( not gMainHandler)
        return;

    win = gMainHandler->FindWindow(DF_TEAM_WIN);

    if ( not win)
        return;

    ClearAllTreeStates();

    timestamp = GetCurrentTime();
    unit = (Unit)listit.GetFirst();

    while (unit)
    {
        if (unit->IsFlight() and not unit->IsDead())
        {
            // Add this flight and all aircraft to our trees
            flight = (Flight)unit;

            // Add FLIGHT to Furball Tree
            flist = (C_TreeList *)win->FindControl(FURBALL_TREE);
            furflt = AddDogfightFlight(flist, flight);

            if (furflt)
                furflt->SetUserNumber(0, timestamp);

            // Add FLIGHT to Team Tree
            team = flight->GetTeam();

            switch (team)
            {
                case 1:
                    tlist = (C_TreeList *)win->FindControl(CRIMSON_TREE);
                    break;

                case 2:
                    tlist = (C_TreeList *)win->FindControl(SHARK_TREE);
                    break;

                case 3:
                    tlist = (C_TreeList *)win->FindControl(VIPER_TREE);
                    break;

                case 4:
                default:
                    tlist = (C_TreeList *)win->FindControl(TIGER_TREE);
                    break;
            }

            teamflt = AddDogfightFlight(tlist, flight);

            if (teamflt)
                teamflt->SetUserNumber(0, timestamp);

            if (gCurrentFlightID == flight->Id())
            {
                switch (SimDogfight.GetGameType())
                {
                    case dog_TeamFurball:
                    case dog_TeamMatchplay:
                        if (teamflt)
                            teamflt->SetState(1);

                        break;

                    default:
                        if (furflt)
                            furflt->SetState(1);

                        break;
                }
            }

            for (ac = 0; ac < PILOTS_PER_FLIGHT; ac++)
            {
                if (flight->pilots[ac] not_eq NO_PILOT or flight->player_slots[ac] not_eq NO_PILOT)
                {
                    // Add PLAYER to Furball Tree
                    furplt = AddDogfightPilot(flist, flight, ac);

                    if (furplt)
                        furplt->SetUserNumber(0, timestamp);

                    // Add PLAYER to Team Tree
                    teamplt = AddDogfightPilot(tlist, flight, ac);

                    if (teamplt)
                        teamplt->SetUserNumber(0, timestamp);

                    if (gCurrentFlightID == flight->Id() and gCurrentAircraftNum == ac)
                    {
                        switch (SimDogfight.GetGameType())
                        {
                            case dog_TeamFurball:
                            case dog_TeamMatchplay:
                                if (teamplt)
                                    teamplt->SetState(1);

                                break;

                            default:
                                if (furplt)
                                    furplt->SetState(1);

                                break;
                        }
                    }
                }
            }
        }

        unit = (Unit)listit.GetNext();
    }

    // Erase everything with UserNumber(0) not_eq timestamp
    ClearOldDFInfo(timestamp);
    CheckDelButtons();
    CheckFlyButton();
}

// ==============================================
//
// KCK: Group related peter stuff
// Seems like this belongs somewhere else...
//
// ==============================================

void DeleteGroupList(long ID)
{
    C_Window *win;
    CONTROLLIST *winctrls;
    F4CSECTIONHANDLE* Leave;

    // Clear controls from window with the userdata[_UI95_DELGROUP_SLOT_] == _UI95_DELGROUP_ID_

    win = gMainHandler->FindWindow(ID);

    if (win)
    {
        Leave = UI_Enter(win);
        winctrls = win->GetControlList();

        while (winctrls)
        {
            if (winctrls->Control_->GetUserNumber(_UI95_DELGROUP_SLOT_) == _UI95_DELGROUP_ID_)
            {
                winctrls = win->RemoveControl(winctrls);
            }
            else
                winctrls = winctrls->Next;
        }

        UI_Leave(Leave);
    }
}

// ==============================================
//
// KCK: Plane related peter stuff
//
// ==============================================

uchar GetPlaneListID(long ID)
{
    uchar i;

    LoadDfPlanes();
    i = 0;

    while (DFAIPlanes[i].ID and i < 255)
    {
        if (DFAIPlanes[i].ID == ID)
            return(i);

        i++;
    }

    return(0);
}

long GetACIDFromFlight(Flight flight)
{
    uchar i, stype = flight->GetSType(), sptype = flight->GetSPType();

    LoadDfPlanes();
    i = 0;

    while (DFAIPlanes[i].ID and i < 255)
    {
        if (DFAIPlanes[i].UnitSType and DFAIPlanes[i].SPType and DFAIPlanes[i].UnitSType == stype and DFAIPlanes[i].SPType == sptype)
            return(DFAIPlanes[i].ID);

        i++;
    }

    return(0);
}

// Use ONLY as a TreeList callback
static void SelectDogfightGameCB(long, short hittype, C_Base *control)
{
    VU_ID *tmpID;
    FalconGameEntity *game;
    TREELIST *item;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    item = ((C_TreeList *)control)->GetLastItem();

    if (item == NULL) return;

    if (item->Item_ == NULL) return;

    if (gCommsMgr->GetGame() not_eq vuPlayerPoolGroup)
        return;

    if (item->Type_ == C_TYPE_MENU)
    {
        if ( not item->Item_->GetState())
        {
            ((C_TreeList *)control)->SetAllControlStates(0, ((C_TreeList *)control)->GetRoot());
            item->Item_->SetState(1);
            item->Item_->Refresh();
            tmpID = (VU_ID *)item->Item_->GetUserPtr(_UI95_VU_ID_SLOT_);

            if (tmpID)
            {
                game = (FalconGameEntity*)vuDatabase->Find(*tmpID);
                gCommsMgr->LookAtGame(game);

                if (game)
                {
                    // if(game->GetGameType() == game_Dogfight)
                    // gCommsMgr->RequestSettings(game->Id());
                }
            }
        }
        else
        {
            item->Item_->SetState(0);
            item->Item_->Refresh();
            gCommsMgr->LookAtGame(NULL);
            ClearDFTeamLists();
        }
    }
}

short ConvertDFIDtoTeam(long ID)
{
    switch (ID)
    {
        case DF_CRIMSON:
        case ADD_CRIMSON_PLANE:
        case DF_MARK_CRIMSON:
            return(1);
            break;

        case DF_SHARK:
        case ADD_SHARK_PLANE:
        case DF_MARK_SHARK:
            return(2);
            break;

        case DF_VIPER:
        case ADD_USA_PLANE:
        case DF_MARK_VIPER:
            return(3);
            break;

        case DF_TIGER:
        case ADD_TIGER_PLANE:
        case DF_MARK_TIGER:
            return(4);
            break;
    }

    return(0);
}

void AddDogfightFlightCB(long, short hittype, C_Base *control)
{
    Flight flight = NULL;
    long teamid = 0, skill = 0, acid = 0, idx = 0, type = 0, callgroup = 0;
    C_ListBox *lbox = NULL;
    long value = 0;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    if ( not FalconLocalGame or FalconLocalSession->Game() == vuPlayerPoolGroup)
        return;

    LoadDfPlanes();

    switch (AddToTeam)
    {
        case DF_CRIMSON_CALLS:
            teamid = 1;
            break;

        case DF_SHARK_CALLS:
            teamid = 2;
            break;

        case DF_TBIRD_CALLS:
            teamid = 3;
            break;

        case DF_TIGER_CALLS:
            teamid = 4;
            break;

        case DF_FURBALL_CALLS:
            teamid = 0;
            break;
    }

    lbox = (C_ListBox*)control->Parent_->FindControl(AddToTeam);

    if (lbox)
        value = lbox->GetTextID();

    if (teamid)
        callgroup = value - 1;
    else
    {
        callgroup = 0;
        teamid = value;
    }

    lbox = (C_ListBox*)control->Parent_->FindControl(DF_AIRCRAFT_TYPE);

    if (lbox)
        acid = lbox->GetTextID();
    else
        acid = DF_AC_F16C;

    lbox = (C_ListBox*)control->Parent_->FindControl(DF_SKILL);

    if (lbox)
        skill = lbox->GetTextID() - 1;
    else
        skill = 0;

    switch (SimDogfight.GetGameType())
    {
        case dog_TeamMatchplay:
        case dog_TeamFurball:
            flight = (Flight)vuDatabase->Find(gCurrentFlightID);

            if (flight and flight->GetTeam() not_eq teamid)
                flight = NULL;

            break;

        default:
            flight = NULL;
            break;
    }

    idx = GetPlaneListID(acid);
    type = GetClassID(DOMAIN_AIR, CLASS_UNIT, TYPE_FLIGHT, DFAIPlanes[idx].UnitSType, DFAIPlanes[idx].SPType, VU_ANY, VU_ANY, VU_ANY) + VU_LAST_ENTITY_TYPE;
    AddDogfightAIPlane(flight, type, skill, teamid);

    gMainHandler->DisableWindowGroup(control->GetGroup());
}

void AddDogfightAICB(long ID, short hittype, C_Base *control)
{
    C_Window *win;
    C_ListBox *lbox;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    if (gCommsMgr and gCommsMgr->Online() and SimDogfight.GetDogfightGameStatus() not_eq dog_Waiting)
    {
        GameHasStarted();
        gMainHandler->DropControl();
        CopyDFSettingsToWindow();
        return;
    }

    switch (ID)
    {
        case ADD_CRIMSON_PLANE:
            AddToTeam = DF_CRIMSON_CALLS;
            break;

        case ADD_SHARK_PLANE:
            AddToTeam = DF_SHARK_CALLS;
            break;

        case ADD_USA_PLANE:
            AddToTeam = DF_TBIRD_CALLS;
            break;

        case ADD_TIGER_PLANE:
            AddToTeam = DF_TIGER_CALLS;
            break;

        case ADD_FURBALL_PLANE:
            AddToTeam = DF_FURBALL_CALLS;
            break;
    }

    win = gMainHandler->FindWindow(DF_FLIGHT_WIN);

    if (win)
    {
        lbox = (C_ListBox*)win->FindControl(AddToTeam);

        if (lbox)
        {
            win->HideCluster(lbox->GetUserNumber(1));
            win->HideCluster(lbox->GetUserNumber(2));
            win->HideCluster(lbox->GetUserNumber(3));
            win->HideCluster(lbox->GetUserNumber(4));
            win->UnHideCluster(lbox->GetUserNumber(0));
        }
    }

    gMainHandler->EnableWindowGroup(control->GetGroup());
}

void RemoveAICB(long ID, short hittype, C_Base *)
{
    Flight flight;
    short teamid = 0;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    flight = (Flight)vuDatabase->Find(gCurrentFlightID);

    if ( not flight)
        return;

    if (gCommsMgr->Online() and SimDogfight.GetDogfightGameStatus() not_eq dog_Waiting)
    {
        GameHasStarted();
        gMainHandler->DropControl();
        CopyDFSettingsToWindow();
        return;
    }

    switch (ID)
    {
        case DEL_CRIMSON_PLANE:
            teamid = 1;
            break;

        case DEL_SHARK_PLANE:
            teamid = 2;
            break;

        case DEL_TBIRD_PLANE:
            teamid = 3;
            break;

        case DEL_TIGER_PLANE:
            teamid = 4;
            break;

        case DEL_FURBALL_PLANE:
            teamid = -1;
            break;
    }

    if (teamid == -1)
        RemoveDogfightPlane();
    else if (flight->GetTeam() == teamid)
        RemoveDogfightPlane();
}

void PositionSlider(C_Slider *slider, long value, long minv, long maxv)
{
    int pos;

    if (slider)
    {
        pos = ((slider->GetSliderMax() - slider->GetSliderMin()) * (value - minv)) / (maxv - minv + 1) + 1;
        slider->Refresh();
        slider->SetSliderPos(pos);
        slider->Refresh();
    }
}

void UpdateDogfightWindows(void)
{
    C_Window *win;
    C_Button *btn;

    win = gMainHandler->FindWindow(DF_GAME_HEADER_WIN);

    if (win)
    {
        btn = (C_Button *)win->FindControl(DF_GAME_TITLE);

        if (btn)
        {
            switch (SimDogfight.GetGameType())
            {
                case dog_TeamFurball:
                    btn->SetState(1);
                    break;

                case dog_TeamMatchplay:
                    btn->SetState(2);
                    break;

                default:
                    btn->SetState(0);
                    break;
            }
        }

        btn->Refresh();
    }

    win = gMainHandler->FindWindow(DF_TEAM_WIN);

    if (win)
    {
        if (SimDogfight.GetGameType() not_eq dog_Furball)
        {
            win->HideCluster(200);
            win->UnHideCluster(100);
            win->SetClientFlags(C_STATE_0, C_BIT_ENABLED);
            win->SetClientFlags(C_STATE_1, C_BIT_ENABLED);
            win->SetClientFlags(C_STATE_2, C_BIT_ENABLED);
            win->SetClientFlags(C_STATE_3, C_BIT_ENABLED);
            win->SetClientFlags(C_STATE_4, 0);
        }
        else
        {
            win->HideCluster(100);
            win->UnHideCluster(200);
            win->SetClientFlags(C_STATE_0, 0);
            win->SetClientFlags(C_STATE_1, 0);
            win->SetClientFlags(C_STATE_2, 0);
            win->SetClientFlags(C_STATE_3, 0);
            win->SetClientFlags(C_STATE_4, C_BIT_ENABLED);
        }

        win->RefreshWindow();
    }
}

void CopyDFSettingsToWindow(void)
{
    C_Window *win;
    C_EditBox *ebox;
    C_Button *btn;
    C_Slider *sldr;
    C_ListBox *lbox;
    C_Cursor *crsr;
    C_Clock *clk;

    if ( not gMainHandler)
        return;

    gMainHandler->EnterCritical();
    win = gMainHandler->FindWindow(DF_SETTINGS_WIN);

    if (win)
    {
        ebox = (C_EditBox *)win->FindControl(RADAR_READOUT);
        ebox->Refresh();

        if (ebox)
        {
            ebox->SetInteger(SimDogfight.GetNumRadarMissiles());
            ebox->Refresh();
            sldr = (C_Slider *)win->FindControl(RADAR_SLIDER);

            if (sldr)
                PositionSlider(sldr, ebox->GetInteger(), ebox->GetMinInteger(), ebox->GetMaxInteger());
        }

        ebox = (C_EditBox *)win->FindControl(ALLIR_READOUT);

        if (ebox)
        {
            ebox->SetInteger(SimDogfight.GetNumAllAspectMissiles());
            ebox->Refresh();
            sldr = (C_Slider *)win->FindControl(ALLIR_SLIDER);

            if (sldr)
                PositionSlider(sldr, ebox->GetInteger(), ebox->GetMinInteger(), ebox->GetMaxInteger());
        }

        ebox = (C_EditBox *)win->FindControl(RIR_READOUT);

        if (ebox)
        {
            ebox->SetInteger(SimDogfight.GetNumRearAspectMissiles());
            ebox->Refresh();
            sldr = (C_Slider *)win->FindControl(RIR_SLIDER);

            if (sldr)
                PositionSlider(sldr, ebox->GetInteger(), ebox->GetMinInteger(), ebox->GetMaxInteger());
        }

        ebox = (C_EditBox *)win->FindControl(RANGE_READOUT);

        if (ebox)
        {
            ebox->SetInteger(static_cast<long>(SimDogfight.startRange * FT_TO_NM));
            ebox->Refresh();
            sldr = (C_Slider *)win->FindControl(RANGE_SLIDER);

            if (sldr)
                PositionSlider(sldr, ebox->GetInteger(), ebox->GetMinInteger(), ebox->GetMaxInteger());
        }

        ebox = (C_EditBox *)win->FindControl(ALTITUDE_READOUT);

        if (ebox)
        {
            ebox->SetInteger(FloatToInt32(-1.0F * SimDogfight.startAltitude));
            ebox->Refresh();
            sldr = (C_Slider *)win->FindControl(ALTITUDE_SLIDER);

            if (sldr)
                PositionSlider(sldr, ebox->GetInteger(), ebox->GetMinInteger(), ebox->GetMaxInteger());
        }

        ebox = (C_EditBox *)win->FindControl(MP_READOUT);

        if (ebox)
        {
            if (SimDogfight.rounds)
                ebox->SetInteger(SimDogfight.rounds);
            else
                ebox->SetText("Unlimited");

            ebox->Refresh();
            sldr = (C_Slider *)win->FindControl(MP_SLIDER);

            if (sldr)
                PositionSlider(sldr, ebox->GetInteger(), ebox->GetMinInteger(), ebox->GetMaxInteger());
        }

        lbox = (C_ListBox *)win->FindControl(DF_GAME_TYPE);

        if (lbox)
        {
            switch (SimDogfight.GetGameType())
            {
                case dog_Furball:
                    lbox->SetValue(DF_GAME_FURBALL);
                    break;

                case dog_TeamFurball:
                    lbox->SetValue(DF_GAME_TEAM_FURBALL);
                    break;

                case dog_TeamMatchplay:
                    lbox->SetValue(DF_GAME_TEAM_MATCH);
                    break;
            }

            lbox->Refresh();
        }

        btn = (C_Button *)win->FindControl(GUN_CTRL);

        if (btn)
        {
            if (SimDogfight.IsSetFlag(DF_UNLIMITED_GUNS))
                btn->SetState(1);
            else
                btn->SetState(0);

            btn->Refresh();
        }

        btn = (C_Button *)win->FindControl(ECM_CTRL);

        if (btn)
        {
            if (SimDogfight.IsSetFlag(DF_ECM_AVAIL))
                btn->SetState(1);
            else
                btn->SetState(0);

            btn->Refresh();
        }
    }

    win = gMainHandler->FindWindow(DF_MAP_WIN);

    if (win)
    {
        crsr = (C_Cursor*)win->FindControl(DF_MAP_CURSOR);

        if (crsr)
        {
            crsr->SetXY(RatiotoMapX(SimDogfight.xRatio, crsr), RatiotoMapY(SimDogfight.yRatio, crsr));
            crsr->Refresh();
        }
    }

    win = gMainHandler->FindWindow(DF_PLAY_SUA_WIN);

    if (win)
    {
        clk = (C_Clock*)win->FindControl(TIME_ID);

        if (clk)
        {
            clk->SetHour(SimDogfight.startTime / CampaignHours);
            clk->SetMinute((SimDogfight.startTime / CampaignMinutes) % 60);
            clk->SetSecond((SimDogfight.startTime / CampaignSeconds) % 60);
            clk->Refresh();
        }
    }

    UpdateDogfightWindows();

    if (TheCampaign.IsLoaded())
        BuildDFPlayerList();

    DogfightMenuSetup();
    gMainHandler->LeaveCritical();
}

// Same as above function, but only copys in the time, essentially
// (for the preloaded dogfight selection).
// Later we should also apply # of player, etc.. But I think Joe axed the window
// to display this.
void CopyDFSettingsToSelectWindow(void)
{
    C_Window *win;
    C_Clock *clk;
    C_Cursor *crsr;

    if ( not gMainHandler)
        return;

    gMainHandler->EnterCritical();
    win = gMainHandler->FindWindow(DF_SUA_WIN);

    if (win)
    {
        // Copy in the time
        clk = (C_Clock*)win->FindControl(TIME_ID);

        if (clk)
        {
            clk->SetHour(SimDogfight.startTime / CampaignHours);
            clk->SetMinute((SimDogfight.startTime / CampaignMinutes) % 60);
            clk->SetSecond((SimDogfight.startTime / CampaignSeconds) % 60);
            clk->Refresh();
        }
    }

    // Update the map too.
    win = gMainHandler->FindWindow(DF_MAP_WIN);

    if (win)
    {
        crsr = (C_Cursor*)win->FindControl(DF_MAP_CURSOR);

        if (crsr)
        {
            crsr->SetXY(RatiotoMapX(SimDogfight.xRatio, crsr), RatiotoMapY(SimDogfight.yRatio, crsr));
            crsr->Refresh();
        }
    }

    UpdateDogfightWindows();
    gMainHandler->LeaveCritical();
}

void CopyDFSettingsFromWindow(void)
{
    C_Window *win;
    C_Cursor *crsr;
    C_ListBox *lbox;
    C_EditBox *ebox;
    C_Button *btn;
    C_Clock *clk;

    if ( not gMainHandler) return;

    if (gCommsMgr->Online() and SimDogfight.GetDogfightGameStatus() not_eq dog_Waiting)
        return;

    gMainHandler->EnterCritical();

    // Now the main window
    win = gMainHandler->FindWindow(DF_SETTINGS_WIN);

    if (win)
    {
        ebox = (C_EditBox *)win->FindControl(RADAR_READOUT);
        ebox->Refresh();

        if (ebox)
            SimDogfight.SetNumRadarMissiles(static_cast<uchar>(ebox->GetInteger()));

        ebox = (C_EditBox *)win->FindControl(ALLIR_READOUT);

        if (ebox)
            SimDogfight.SetNumAllAspectMissiles(static_cast<uchar>(ebox->GetInteger()));

        ebox = (C_EditBox *)win->FindControl(RIR_READOUT);

        if (ebox)
            SimDogfight.SetNumRearAspectMissiles(static_cast<uchar>(ebox->GetInteger()));

        ebox = (C_EditBox *)win->FindControl(RANGE_READOUT);

        if (ebox)
            SimDogfight.startRange = ebox->GetInteger() * NM_TO_FT;

        ebox = (C_EditBox *)win->FindControl(ALTITUDE_READOUT);

        if (ebox)
            SimDogfight.startAltitude = ebox->GetInteger() * -1.0F;

        ebox = (C_EditBox *)win->FindControl(MP_READOUT);

        if (ebox)
            SimDogfight.rounds = static_cast<uchar>(ebox->GetInteger());

        btn = (C_Button *)win->FindControl(GUN_CTRL);

        if (btn)
        {
            if (btn->GetState())
                SimDogfight.SetFlag(DF_UNLIMITED_GUNS);
            else
                SimDogfight.UnSetFlag(DF_UNLIMITED_GUNS);
        }

        btn = (C_Button *)win->FindControl(ECM_CTRL);

        if (btn)
        {
            if (btn->GetState())
                SimDogfight.SetFlag(DF_ECM_AVAIL);
            else
                SimDogfight.UnSetFlag(DF_ECM_AVAIL);
        }
    }

    win = gMainHandler->FindWindow(DF_MAP_WIN);

    if (win)
    {
        //sfr: another temporary hack to check this
        //crsr=(C_Cursor*)win->FindControl(10049);
        crsr = (C_Cursor*)win->FindControl(DF_MAP_CURSOR);

        if (crsr)
        {
            SimDogfight.xRatio = MapXtoRatio(crsr);
            SimDogfight.yRatio = MapYtoRatio(crsr);
        }
    }

    win = gMainHandler->FindWindow(DF_SETTINGS_WIN);

    if (win)
    {
        lbox = (C_ListBox *)win->FindControl(DF_GAME_TYPE);

        if (lbox)
        {
            switch (lbox->GetTextID())
            {
                case DF_GAME_TEAM_MATCH:
                    SimDogfight.SetGameType(dog_TeamMatchplay);
                    break;

                case DF_GAME_TEAM_FURBALL:
                    SimDogfight.SetGameType(dog_TeamFurball);
                    break;

                default:
                    SimDogfight.SetGameType(dog_Furball);
                    break;
            }
        }
    }

    win = gMainHandler->FindWindow(DF_PLAY_SUA_WIN);

    if (win)
    {
        clk = (C_Clock*)win->FindControl(TIME_ID);

        if (clk)
            SimDogfight.startTime = clk->GetHour() * CampaignHours + clk->GetMinute() * CampaignMinutes + clk->GetSecond() * CampaignSeconds;
    }

    UpdateDogfightWindows();

    if (TheCampaign.IsLoaded())
    {
        SimDogfight.ApplySettings();
        SimDogfight.SendSettings(NULL);
    }

    DogfightMenuSetup();
    gMainHandler->LeaveCritical();
}

static void DFGameModeCB(long, short hittype, C_Base *)
{
    if (hittype not_eq C_TYPE_SELECT)
        return;

    if (gCommsMgr->Online() and SimDogfight.GetDogfightGameStatus() not_eq dog_Waiting)
    {
        GameHasStarted();
        gMainHandler->DropControl();
        CopyDFSettingsToWindow();
        return;
    }

    CopyDFSettingsFromWindow();
    CheckFlyButton();
}

void CheckFlyButton()
{
    C_Window *win;
    C_Button *btn1, *btn2;
    BOOL Enabled;

    win = gMainHandler->FindWindow(DF_PLAY_TOOLBAR_WIN);

    if (win == NULL)
        return;

    btn1 = (C_Button *)win->FindControl(SINGLE_FLY_CTRL);
    btn2 = (C_Button *)win->FindControl(COMMS_FLY_CTRL);

    if (btn1 == NULL or btn2 == NULL)
        return;

    btn1->SetFlagBitOff(C_BIT_ENABLED);
    btn2->SetFlagBitOff(C_BIT_ENABLED);

    Enabled = FALSE;

    if (vuLocalGame not_eq vuPlayerPoolGroup and FalconLocalSession->GetTeam() not_eq 255 and FalconLocalSession->GetPilotSlot() not_eq 255)
        Enabled = TRUE;

    // In dogfight games, check ready state
    if (FalconLocalGame and FalconLocalGame->GetGameType() == game_Dogfight and not SimDogfight.ReadyToStart())
        Enabled = FALSE;

    if (Enabled)
    {
        btn1->SetFlagBitOn(C_BIT_ENABLED);
        btn2->SetFlagBitOn(C_BIT_ENABLED);
    }

    btn1->Refresh();
    btn2->Refresh();
}

static void SetDogFightStartup()
{
    C_Window *win;

    if ((MainLastGroup == 2000))
    {
        // these functions (with scoring) MUST be in this order to work properly
        win = gMainHandler->FindWindow(DF_DBRF_WIN);

        if (win)
        {
            ProcessEventList(win, 1);

            DisplayDogfightResults();
            win->ScanClientAreas();
            win->RefreshWindow();
            gMainHandler->EnableWindowGroup(win->GetGroup());

            CopyDFSettingsToWindow();
            gMainHandler->EnableWindowGroup(2020);
        }
    }
}

extern _TCHAR DogfightTeamNames[NUM_TEAMS][20];

void LoadDogFightWindows()
{
    C_Window *win;
    C_TreeList *tree;
    C_TimerHook *tmr;
    long ID;

    if ( not DFLoaded)
    {
        if (_LOAD_ART_RESOURCES_)
            gMainParser->LoadImageList("df_res.lst");
        else
            gMainParser->LoadImageList("df_art.lst");

        gMainParser->LoadSoundList("df_snd.lst");

        if ( not gDogfightBites)
            gDogfightBites = gMainParser->ParseSoundBite("art\\dgft\\play\\uiddf.scf");

        gMainParser->LoadWindowList("df_scf.lst"); // Modified by M.N. - add art/art1024 by LoadWindowList

        ID = gMainParser->GetFirstWindowLoaded();

        while (ID)
        {
            HookupDogFightControls(ID);
            ID = gMainParser->GetNextWindowLoaded();
        }

        win = gMainHandler->FindWindow(DGFT_MAIN_SCREEN);

        if (win)
        {
            tmr = new C_TimerHook;
            tmr->Setup(C_DONT_CARE, C_TYPE_TIMER);
            tmr->SetUpdateCallback(GenericTimerCB);
            tmr->SetRefreshCallback(BlinkCommsButtonTimerCB);
            tmr->SetUserNumber(_UI95_TIMER_DELAY_, 1 * _UI95_TICKS_PER_SECOND_); // Timer activates every 2 seconds (Only when this window is open)

            win->AddControl(tmr);
        }

        DFLoaded++;
    }

    _tcscpy(DogfightTeamNames[1], gStringMgr->GetString(TXT_CRIMSONFLIGHT));
    _tcscpy(DogfightTeamNames[2], gStringMgr->GetString(TXT_SHARKFLIGHT));
    _tcscpy(DogfightTeamNames[3], gStringMgr->GetString(TXT_VIPREFLIGHT));
    _tcscpy(DogfightTeamNames[4], gStringMgr->GetString(TXT_TIGERFLIGHT));
    SetDogFightStartup();

    SetSingle_Comms_Ctrls();

    if (gCommsMgr->Online())
        RebuildGameTree();

    win = gMainHandler->FindWindow(DF_LOAD_WIN);

    if (win)
    {
        tree = (C_TreeList*)win->FindControl(FILELIST_TREE);

        if (tree)
        {
            tree->DeleteBranch(tree->GetRoot());
            tree->SetUserNumber(0, 0);
            tree->SetSortType(TREE_SORT_CALLBACK);
            tree->SetSortCallback(FileNameSortCB);
            tree->SetCallback(SelectDFSettingsFileCB);
            char path[_MAX_PATH];
            sprintf(path, "%s\\*.DFS", FalconCampaignSaveDirectory);

            GetFileListTree(tree, path, DFExcludeList, C_TYPE_ITEM, TRUE, 0);
            tree->RecalcSize();

            if (tree->Parent_)
                tree->Parent_->RefreshClient(tree->GetClient());
        }
    }
}

static void DogFightSLDRCB(long ID, short hittype, C_Base *control)
{
    C_Slider *sldr = NULL;
    C_Window *winme = NULL;
    C_EditBox *ebox = NULL;
    long value = 0, step = 0, temp = 0;

    if (hittype not_eq C_TYPE_MOUSEMOVE)
        return;

    if (gCommsMgr->Online() and SimDogfight.GetDogfightGameStatus() not_eq dog_Waiting)
    {
        GameHasStarted();
        gMainHandler->DropControl();
        CopyDFSettingsToWindow();
        return;
    }

    winme = control->Parent_;

    ebox = NULL;

    switch (ID)
    {
        case RADAR_SLIDER:
            ebox = (C_EditBox *)winme->FindControl(RADAR_READOUT);
            step = 2;
            break;

        case ALLIR_SLIDER:
            ebox = (C_EditBox *)winme->FindControl(ALLIR_READOUT);
            step = 2;
            break;

        case RIR_SLIDER:
            ebox = (C_EditBox *)winme->FindControl(RIR_READOUT);
            step = 2;
            break;

        case RANGE_SLIDER:
            ebox = (C_EditBox *)winme->FindControl(RANGE_READOUT);
            step = 5;
            break;

        case ALTITUDE_SLIDER:
            ebox = (C_EditBox *)winme->FindControl(ALTITUDE_READOUT);
            step = 1000;
            break;

        case MP_SLIDER:
            ebox = (C_EditBox *)winme->FindControl(MP_READOUT);
            step = 1;
            break;
    }

    if (ebox)
    {
        sldr = (C_Slider *)control;
        value = ((ebox->GetMaxInteger() - ebox->GetMinInteger()) * sldr->GetSliderPos() / (sldr->GetSliderMax() - sldr->GetSliderMin())) + 1;
        value += ebox->GetMinInteger();

        if (step > 1)
            temp = value % step;
        else
            temp = step;

        value -= temp;

        if (ebox->GetInteger() == value)
            return;

        // Minimum starting range
        if (ID == RANGE_SLIDER and value < 5)
            value = 5;

        ebox->SetInteger(value);
        PositionSlider(sldr, value, ebox->GetMinInteger(), ebox->GetMaxInteger());

        if (ID == MP_SLIDER)
        {
            if (value > 100)
            {
                ebox->SetText(TXT_FOREVER);
            }
            else if (value < 1)
            {
                value = 1;
                ebox->SetInteger(value);
            }
        }

        ebox->Refresh();
    }

    CopyDFSettingsFromWindow();
}

void ClearDFTeamLists()
{
    C_Window *win;
    C_TreeList *list;

    win = gMainHandler->FindWindow(DF_TEAM_WIN);

    if (win)
    {
        list = (C_TreeList *)win->FindControl(CRIMSON_TREE);

        if (list)
        {
            list->DeleteBranch(list->GetRoot());
            list->RecalcSize();

            if ( not (list->GetFlags() bitand C_BIT_INVISIBLE))
                win->RefreshClient(list->GetClient());
        }

        list = (C_TreeList *)win->FindControl(SHARK_TREE);

        if (list)
        {
            list->DeleteBranch(list->GetRoot());
            list->RecalcSize();

            if ( not (list->GetFlags() bitand C_BIT_INVISIBLE))
                win->RefreshClient(list->GetClient());
        }

        list = (C_TreeList *)win->FindControl(TIGER_TREE);

        if (list)
        {
            list->DeleteBranch(list->GetRoot());
            list->RecalcSize();

            if ( not (list->GetFlags() bitand C_BIT_INVISIBLE))
                win->RefreshClient(list->GetClient());
        }

        list = (C_TreeList *)win->FindControl(VIPER_TREE);

        if (list)
        {
            list->DeleteBranch(list->GetRoot());
            list->RecalcSize();

            if ( not (list->GetFlags() bitand C_BIT_INVISIBLE))
                win->RefreshClient(list->GetClient());
        }

        list = (C_TreeList *)win->FindControl(FURBALL_TREE);

        if (list)
        {
            list->DeleteBranch(list->GetRoot());
            list->RecalcSize();

            if ( not (list->GetFlags() bitand C_BIT_INVISIBLE))
                win->RefreshClient(list->GetClient());
        }
    }
}

void ClearDFTeamButtons()
{
    C_Window *win;
    C_Button *btn;

    win = gMainHandler->FindWindow(DF_TEAM_WIN);

    if (win)
    {
        btn = (C_Button *)win->FindControl(DF_CRIMSON);

        if (btn)
        {
            if (btn->GetState())
            {
                btn->SetState(0);
                btn->Refresh();
            }
        }

        btn = (C_Button *)win->FindControl(DF_SHARK);

        if (btn)
        {
            if (btn->GetState())
            {
                btn->SetState(0);
                btn->Refresh();
            }
        }

        btn = (C_Button *)win->FindControl(DF_TIGER);

        if (btn)
        {
            if (btn->GetState())
            {
                btn->SetState(0);
                btn->Refresh();
            }
        }

        btn = (C_Button *)win->FindControl(DF_VIPER);

        if (btn)
        {
            if (btn->GetState())
            {
                btn->SetState(0);
                btn->Refresh();
            }
        }

        btn = (C_Button *)win->FindControl(DF_MARK_CRIMSON);

        if (btn)
        {
            if (btn->GetState())
            {
                btn->SetState(0);
                btn->Refresh();
            }
        }

        btn = (C_Button *)win->FindControl(DF_MARK_SHARK);

        if (btn)
        {
            if (btn->GetState())
            {
                btn->SetState(0);
                btn->Refresh();
            }
        }

        btn = (C_Button *)win->FindControl(DF_MARK_TIGER);

        if (btn)
        {
            if (btn->GetState())
            {
                btn->SetState(0);
                btn->Refresh();
            }
        }

        btn = (C_Button *)win->FindControl(DF_MARK_VIPER);

        if (btn)
        {
            if (btn->GetState())
            {
                btn->SetState(0);
                btn->Refresh();
            }
        }
    }
}

void LeaveDogfight()
{
    TheCampaign.EndCampaign();

    if (gCommsMgr->Online())
    {
        if (vuLocalGame not_eq vuPlayerPoolGroup)
        {
            SetCurrentGameState(DogfightGames, C_STATE_0);
            vuLocalSessionEntity->JoinGame(vuPlayerPoolGroup);
        }

        gCommsMgr->LookAtGame(NULL);
    }

    ClearDFTeamLists();
    ClearDFTeamButtons();
    FalconLocalSession->SetPlayerSquadron(NULL);
    FalconLocalSession->SetPlayerFlight(NULL);
    FalconLocalSession->SetCountry(255);
    FalconLocalSession->SetPilotSlot(255);
    SendMessage(gMainHandler->GetAppWnd(), FM_SHUTDOWN_CAMPAIGN, 0, 0);
    CheckFlyButton();
}

void EnableDFTeamGroup()
{
    CheckFlyButton();
}

void InfoButtonCB(long, short hittype, C_Base *control)
{
    VU_ID gameID;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    SetupInfoWindow(NULL, NULL);
    gMainHandler->EnableWindowGroup(control->GetGroup());
}

void SaveItCB(long, short hittype, C_Base *control)
{
    C_EditBox *ebox;
    C_Window *win;
    C_TreeList *tree;
    char filename[MAX_PATH];

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    win = gMainHandler->FindWindow(SAVE_WIN);

    if ( not win)
        return;

    gMainHandler->HideWindow(win);
    gMainHandler->HideWindow(control->Parent_);

    ebox = (C_EditBox*)win->FindControl(FILE_NAME);

    if (ebox)
    {
        _stprintf(filename, "%s\\%s.dfs", FalconCampUserSaveDirectory, ebox->GetText());

        // SAVE SETTINGS HERE
        SimDogfight.SaveSettings(filename);
    }

    win = gMainHandler->FindWindow(DF_LOAD_WIN);

    if (win)
    {
        tree = (C_TreeList*)win->FindControl(FILELIST_TREE);

        if (tree)
        {
            _stprintf(filename, "%s\\*.dfs", FalconCampUserSaveDirectory);
            tree->DeleteBranch(tree->GetRoot());
            GetFileListTree(tree, filename, DFExcludeList, C_TYPE_ITEM, TRUE, 0);
            tree->RecalcSize();

            if (tree->Parent_)
                tree->Parent_->RefreshClient(tree->GetClient());
        }
    }
}

void VerifySaveItCB(long ID, short hittype, C_Base *control)
{
    C_EditBox *ebox;
    FILE *fp;
    char filename[MAX_PATH];

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    ebox = (C_EditBox*)control->Parent_->FindControl(FILE_NAME);

    if (ebox)
    {
        //dpc EmptyFilenameSaveFix, modified by MN - added a warning to enter a filename
        if (g_bEmptyFilenameFix)
        {
            if (_tcslen(ebox->GetText()) == 0)
            {
                AreYouSure(TXT_WARNING, TXT_ENTER_FILENAME, CloseWindowCB, CloseWindowCB);
                return;
            }
        }

        //end EmptyFilenameSaveFix
        _stprintf(filename, "%s\\%s.dfs", FalconCampUserSaveDirectory, ebox->GetText());
        fp = fopen(filename, "r");

        if (fp)
        {
            fclose(fp);

            if (CheckExclude(filename, FalconCampUserSaveDirectory, DFExcludeList, "dfs"))
                AreYouSure(TXT_ERROR, TXT_CANT_OVERWRITE, CloseWindowCB, CloseWindowCB);
            else
                AreYouSure(TXT_WARNING, TXT_FILE_EXISTS, SaveItCB, CloseWindowCB);
        }
        else
        {
            if (CheckExclude(filename, FalconCampUserSaveDirectory, DFExcludeList, "dfs"))
                AreYouSure(TXT_ERROR, TXT_CANT_OVERWRITE, CloseWindowCB, CloseWindowCB);
            else
                SaveItCB(ID, hittype, control);
        }
    }
}

static void SaveDFSettingsCB(long, short hittype, C_Base *)
{
    _TCHAR fname[MAX_PATH];

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    SetDeleteCallback(DelDFSFileCB);
    _stprintf(fname, "%s\\*.dfs", FalconCampUserSaveDirectory);
    SaveAFile(TXT_SAVE_DOGFIGHT, fname, DFExcludeList, VerifySaveItCB, CloseWindowCB, "");
}

// Callback from clicking on a saved game's name
static void SelectDFSettingsFileCB(long, short hittype, C_Base *control)
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

            if (btn)
            {
                if ( not btn->GetState())
                {
                    tree->SetAllControlStates(0, tree->GetRoot());
                    btn->SetState(1);
                    tree->Refresh();
                    _stprintf(gCurDogfightFile, "%s\\%s.DFS", FalconCampUserSaveDirectory, btn->GetText(C_STATE_0));
                    SimDogfight.SetFilename(gCurDogfightFile);
                    SimDogfight.LoadSettings();
                    CopyDFSettingsToSelectWindow();
                }
                else
                {
                    tree->SetAllControlStates(0, tree->GetRoot());
                    tree->Refresh();
                    gCurDogfightFile[0] = 0;
                }

                if (gCurDogfightFile[0] == 0)
                {
                    _tcscpy(gCurDogfightFile, FalconCampUserSaveDirectory);
                    _tcscat(gCurDogfightFile, "\\New Game.dfs");
                }
            }
        }
    }
}

// Callback from clicking on a remote game's name
void SelectDFGameFileCB(long, short hittype, C_Base *)
{
    FalconGameEntity *game = NULL; // KCK: Need to get game associated with button

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    if ( not game)
        return;

    // Request Settings here
    SimDogfight.RequestSettings(game);
}

void LoadRadioCB(long, short hittype, C_Base *)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    // Probably don't want to load settings until after we load the campaign
    // LoadDFSettingsCB(ID,hittype,control);
    CheckFlyButton();
}

void JoinRadioCB(long, short hittype, C_Base *control)
{
    C_Window *win;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    if ( not gCommsMgr->Online())
    {
        win = gMainHandler->FindWindow(PB_WIN);

        if (win)
            gMainHandler->EnableWindowGroup(win->GetGroup());
    }

    if (DogfightGames)
        DogfightGames->RecalcSize();

    control->Parent_->RefreshClient(0);
    CheckFlyButton();
}

static void ToggleGunCB(long, short hittype, C_Base *)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    if (gCommsMgr->Online() and SimDogfight.GetDogfightGameStatus() not_eq dog_Waiting)
    {
        GameHasStarted();
        gMainHandler->DropControl();
        CopyDFSettingsToWindow();
        return;
    }

    CopyDFSettingsFromWindow();
}

static void ToggleECMCB(long, short hittype, C_Base *)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    if (gCommsMgr->Online() and SimDogfight.GetDogfightGameStatus() not_eq dog_Waiting)
    {
        GameHasStarted();
        gMainHandler->DropControl();
        CopyDFSettingsToWindow();
        return;
    }

    CopyDFSettingsFromWindow();
}

static void MoveGameLocationCB(long, short hittype, C_Base *)
{
    if (hittype not_eq C_TYPE_LDROP)
        return;

    if (gCommsMgr->Online() and SimDogfight.GetDogfightGameStatus() not_eq dog_Waiting)
    {
        GameHasStarted();
        gMainHandler->DropControl();
        CopyDFSettingsToWindow();
        return;
    }

    CopyDFSettingsFromWindow();
}

float MapXtoSimX(C_Cursor* crsr)
{
    float xpos, ratio;

    ratio = (float)crsr->GetX() / (float)(crsr->MaxX_ - crsr->MinX_);
    xpos = TheCampaign.TheaterSizeX * FEET_PER_KM * ratio;
    return xpos;
    // KCK: This is cut and paste from peter's code. I don't pretend to understand how it works.
    // xpos = ((crsr->GetX()+(crsr->GetW()/2)-crsr->MinX_) * 3072.0f) / ((crsr->MaxX_-crsr->MinX_)*256.0f);
    // xpos *= FEET_PER_KM / 1000.0F;
    // return xpos;
}

float MapYtoSimY(C_Cursor* crsr)
{
    float ypos, ratio;

    ratio = (float)(crsr->MaxY_ - crsr->GetY()) / (float)(crsr->MaxY_ - crsr->MinY_);
    ypos = TheCampaign.TheaterSizeY * FEET_PER_KM * ratio;
    return ypos;
    // KCK: This is cut and paste from peter's code. I don't pretend to understand how it works.
    // ypos = (((crsr->MaxY_-crsr->MinY_) - (crsr->GetY()+(crsr->GetH()/2)-crsr->MinY_)) * 4096.0f) / ((crsr->MaxY_-crsr->MinY_)*256.0f);
    // ypos *= FEET_PER_KM / 1000.0F;
    // return ypos;
}

long SimXtoMapX(float simx, C_Cursor* crsr)
{
    float ratio = 0.5F;
    int mapx;

    if (TheCampaign.TheaterSizeX)
        ratio = simx / (TheCampaign.TheaterSizeX * FEET_PER_KM);

    mapx = FloatToInt32((crsr->MaxX_ - crsr->MinX_) * ratio);
    return mapx;
    // simx = (simx * 1000.0F) / FEET_PER_KM;
    // return FloatToInt32(((simx * ((crsr->MaxX_-crsr->MinX_)*256.0f)) /  3072.0F) - (crsr->GetW()/2) + crsr->MinX_);
}

long SimYtoMapY(float simy, C_Cursor* crsr)
{
    float ratio = 0.5F;
    int mapy;

    if (TheCampaign.TheaterSizeY)
        ratio = 1.0F - (simy / (TheCampaign.TheaterSizeY * FEET_PER_KM));

    mapy = FloatToInt32((crsr->MaxY_ - crsr->MinY_) * ratio);
    return mapy;
    // simy = (simy * 1000.0F) / FEET_PER_KM;
    // return -1 * FloatToInt32(((simy * ((crsr->MaxY_-crsr->MinY_)*256.0f)) /  4096.0F) + crsr->MinY_ + (crsr->GetH()/2) - (crsr->MaxY_-crsr->MinY_));
}

float MapXtoRatio(C_Cursor* crsr)
{
    float ratio;
    ratio = (float)(crsr->GetX() + crsr->GetW() / 2) / (float)(crsr->MaxX_ - crsr->MinX_);
    return ratio;
}

float MapYtoRatio(C_Cursor* crsr)
{
    float ratio;
    ratio = (float)(crsr->MaxY_ - (crsr->GetY() + crsr->GetH() / 2)) / (float)(crsr->MaxY_ - crsr->MinY_);
    return ratio;
}

long RatiotoMapX(float ratio, C_Cursor* crsr)
{
    int mapx;
    mapx = FloatToInt32((crsr->MaxX_ - crsr->MinX_) * ratio) - crsr->GetW() / 2;
    return mapx;
}

long RatiotoMapY(float ratio, C_Cursor* crsr)
{
    int mapy;
    mapy = FloatToInt32((crsr->MaxY_ - crsr->MinY_) * (1.0F - ratio)) - crsr->GetH() / 2;
    return mapy;
}

static void DogfightFlyCB(long, short hittype, C_Base *)
{
    Flight flight;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    flight = FalconLocalSession->GetPlayerFlight();

    if ( not flight)
        return;

    // TheCampaign.MissionEvaluator->PreMissionEval(flight,FalconLocalSession->GetPilotSlot());

    // Trigger the campaign to compress time and takeoff.
    flight->SetAborted(0);
    flight->SetDead(0);

    if ( not CompressCampaignUntilTakeoff(flight))
        return;

    // 2002-03-09 MN Send a "[Commiting now]" message to the chat windows
    enum { PSEUDO_CONTROL_DF = 565419998 };

    C_EditBox control;
    control.Setup(PSEUDO_CONTROL_DF, 39);
    _TCHAR buffer[21];
    _stprintf(buffer, "( is commiting now )");
    control.SetText(buffer);

    SendChatStringCB(0, DIK_RETURN, &control);
    control.Cleanup();

    // Force Compliance... since they already agreed before
    if ((gCommsMgr) and (gCommsMgr->Online()))
    {
        PlayerOptions.ComplyWRules(CurrRules.GetRules());
        PlayerOptions.SaveOptions();
    }
}

void SaveResultsFileCB(long, short hittype, C_Base *control)
{
    C_EditBox *ebox;
    _TCHAR fname[MAX_PATH];
    C_Window *win;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    win = gMainHandler->FindWindow(SAVE_WIN);

    if ( not win)
        return;

    gMainHandler->HideWindow(win);
    gMainHandler->HideWindow(control->Parent_);

    ebox = (C_EditBox*)win->FindControl(FILE_NAME);

    if (ebox)
    {
        _tcscpy(fname, ebox->GetText());

        if (fname[0] == 0)
            return;

        _tcscat(fname, ".LST");

        SaveDogfightResults(fname);
    }
}

void VerifySaveResultsFileCB(long ID, short hittype, C_Base *control)
{
    C_EditBox *ebox;
    _TCHAR fname[MAX_PATH];
    FILE *fp;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    ebox = (C_EditBox*)control->Parent_->FindControl(FILE_NAME);

    if (ebox)
    {
        //dpc EmptyFilenameSaveFix, modified by MN - added a warning to enter a filename
        if (g_bEmptyFilenameFix)
        {
            if (_tcslen(ebox->GetText()) == 0)
            {
                AreYouSure(TXT_WARNING, TXT_ENTER_FILENAME, CloseWindowCB, CloseWindowCB);
                return;
            }
        }

        //end EmptyFilenameSaveFix
        _stprintf(fname, "%s.lst", ebox->GetText());
        fp = fopen(fname, "r");

        if (fp)
        {
            fclose(fp);
            AreYouSure(TXT_WARNING, TXT_FILE_EXISTS, SaveResultsFileCB, CloseWindowCB);
        }
        else
            SaveResultsFileCB(ID, hittype, control);
    }
}

void SaveResultsCB(long, short hittype, C_Base *)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    SetDeleteCallback(DelLSTFileCB);
    SaveAFile(TXT_SAVE_RESULTS, "*.LST", NULL, VerifySaveResultsFileCB, CloseWindowCB, "");
}

void CleanupDebriefCB(long, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    gMainHandler->DisableWindowGroup(control->GetGroup());
    // ClearPilotList();
}

void SeeDFFilesCB(long, short hittype, C_Base *control)
{
    C_TreeList *tree;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    tree = (C_TreeList*)control->Parent_->FindControl(FILELIST_TREE);

    if (tree)
    {
        tree->DeleteBranch(tree->GetRoot());
        tree->SetUserNumber(0, 0);
        tree->SetSortType(TREE_SORT_CALLBACK);
        tree->SetSortCallback(FileNameSortCB);
        tree->SetCallback(SelectDFSettingsFileCB);
        char path[_MAX_PATH];
        sprintf(path, "%s\\*.DFS", FalconCampaignSaveDirectory);
        GetFileListTree(tree, path, DFExcludeList, C_TYPE_ITEM, TRUE, 0);
        tree->RecalcSize();

        if (tree->Parent_)
            tree->Parent_->RefreshClient(tree->GetClient());
    }

    control->Parent_->HideCluster(control->GetUserNumber(1));
    control->Parent_->UnHideCluster(control->GetUserNumber(0));
}

void SeeDFGamesCB(long, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    if ( not gCommsMgr->Online())
        gMainHandler->EnableWindowGroup(6001);

    control->Parent_->HideCluster(control->GetUserNumber(1));
    control->Parent_->UnHideCluster(control->GetUserNumber(0));
}

void DogfightChangeTimeCB(long ID, short hittype, C_Base *control)
{
    if (hittype not_eq C_TYPE_LMOUSEUP and hittype not_eq C_TYPE_REPEAT)
        return;

    if (gCommsMgr->Online() and SimDogfight.GetDogfightGameStatus() not_eq dog_Waiting)
    {
        GameHasStarted();
        gMainHandler->DropControl();
        CopyDFSettingsToWindow();
        return;
    }

    ChangeTimeCB(ID, hittype, control);

    CopyDFSettingsFromWindow();
}

void DeleteCurrentFileCB(long, short hittype, C_Base *control)
{
    C_TreeList *tree;
    C_Window *win;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    win = gMainHandler->FindWindow(DF_LOAD_WIN);

    if ( not win)
        return;

    gMainHandler->HideWindow(control->Parent_); // Close Verify Window

    if ( not CheckExclude(gCurDogfightFile, FalconCampUserSaveDirectory, DFExcludeList, "dfs"))
    {
        DeleteFile(gCurDogfightFile);
        _tcscpy(gCurDogfightFile, FalconCampUserSaveDirectory);
        _tcscat(gCurDogfightFile, "\\new game.dfs");

        tree = (C_TreeList*)win->FindControl(FILELIST_TREE);

        if (tree)
        {
            tree->DeleteBranch(tree->GetRoot());
            tree->SetUserNumber(0, 0);
            tree->SetSortType(TREE_SORT_CALLBACK);
            tree->SetSortCallback(FileNameSortCB);
            tree->SetCallback(SelectDFSettingsFileCB);
            char path[_MAX_PATH];
            sprintf(path, "%s\\*.DFS", FalconCampaignSaveDirectory);

            GetFileListTree(tree, path, DFExcludeList, C_TYPE_ITEM, TRUE, 0);
            tree->RecalcSize();

            if (tree->Parent_)
                tree->Parent_->RefreshClient(tree->GetClient());
        }
    }
}

void DeleteVerifyFileCB(long, short hittype, C_Base *)
{
    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    if ( not CheckExclude(gCurDogfightFile, FalconCampUserSaveDirectory, DFExcludeList, "dfs"))
        VerifyDelete(0, DeleteCurrentFileCB, CloseWindowCB);
}

void EndDogfightCB(long, short hittype, C_Base *control)
{
    F4CSECTIONHANDLE *Leave;

    if (hittype not_eq C_TYPE_LMOUSEUP)
        return;

    LeaveDogfight();

    Leave = UI_Enter(control->Parent_);
    gMainHandler->DisableWindowGroup(control->GetGroup());
    gMainHandler->EnableWindowGroup(100);
    gMainHandler->EnableWindowGroup(MainLastGroup);
    UI_Leave(Leave);
}

static void HookupDogFightControls(long ID)
{
    C_Window *winme;
    C_Button *ctrl;
    C_Slider *sldr;
    C_ListBox *lbox;
    C_Clock *clk;
    C_Cursor *crsr;
    C_TreeList *tree;

    winme = gMainHandler->FindWindow(ID);

    if (winme == NULL)
        return;

    // DF_SUA Callbacks
    // Time/Date CB
    clk = (C_Clock *)winme->FindControl(TIME_ID);

    if (clk)
    {
        clk->SetTime(12 * 60 * 60);
        clk->Refresh();
    }

    ctrl = (C_Button *)winme->FindControl(TIME_EARLIER);

    if (ctrl)
        ctrl->SetCallback(DogfightChangeTimeCB);

    ctrl = (C_Button *)winme->FindControl(TIME_LATER);

    if (ctrl)
        ctrl->SetCallback(DogfightChangeTimeCB);

    // DF_MAIN_WINDOW callbacks

    ctrl = (C_Button *)winme->FindControl(DGFT_SAVE_CTRL);

    if (ctrl)
        ctrl->SetCallback(SaveDFSettingsCB);

    ctrl = (C_Button *)winme->FindControl(DGFT_RESTORE_CTRL);

    if (ctrl)
        ctrl->SetCallback(NULL);

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

    ctrl = (C_Button *)winme->FindControl(DGFT_EXIT_CTRL);

    if (ctrl)
        ctrl->SetCallback(EndDogfightCB);

    ctrl = (C_Button *)winme->FindControl(ACMI_CTRL);

    if (ctrl)
        ctrl->SetCallback(ACMIButtonCB);

    // DF_HDR Callback

    // Hook up Close Button
    ctrl = (C_Button *)winme->FindControl(CLOSE_WINDOW);

    if (ctrl)
        ctrl->SetCallback(CloseWindowCB);

    // DF_MAKE A GAME
    ctrl = (C_Button *)winme->FindControl(DF_OFFLINE_CTRL);

    if (ctrl)
        ctrl->SetCallback(DogfightBeginCB);

    // DF_MAKE A GAME
    ctrl = (C_Button *)winme->FindControl(DF_HOST_CTRL);

    if (ctrl)
        ctrl->SetCallback(DogfightBeginCB);

    //DF_SET window callbacks

    sldr = (C_Slider *)winme->FindControl(RADAR_SLIDER);

    if (sldr)
        sldr->SetCallback(DogFightSLDRCB);

    sldr = (C_Slider *)winme->FindControl(ALLIR_SLIDER);

    if (sldr)
        sldr->SetCallback(DogFightSLDRCB);

    sldr = (C_Slider *)winme->FindControl(RIR_SLIDER);

    if (sldr)
        sldr->SetCallback(DogFightSLDRCB);

    sldr = (C_Slider *)winme->FindControl(RANGE_SLIDER);

    if (sldr)
        sldr->SetCallback(DogFightSLDRCB);

    sldr = (C_Slider *)winme->FindControl(ALTITUDE_SLIDER);

    if (sldr)
        sldr->SetCallback(DogFightSLDRCB);

    sldr = (C_Slider *)winme->FindControl(MP_SLIDER);

    if (sldr)
        sldr->SetCallback(DogFightSLDRCB);

    ctrl = (C_Button *)winme->FindControl(GUN_CTRL);

    if (ctrl)
        ctrl->SetCallback(ToggleGunCB);

    ctrl = (C_Button *)winme->FindControl(ECM_CTRL);

    if (ctrl)
        ctrl->SetCallback(ToggleECMCB);

    lbox = (C_ListBox *)winme->FindControl(DF_GAME_TYPE);

    if (lbox)
        lbox->SetCallback(DFGameModeCB);

    // DF_LOAD Callbacks

    ctrl = (C_Button *)winme->FindControl(DF_LOAD_CTRL);

    if (ctrl)
        ctrl->SetCallback(SeeDFFilesCB);

    ctrl = (C_Button *)winme->FindControl(DF_JOIN_CTRL);

    if (ctrl)
        ctrl->SetCallback(SeeDFGamesCB);

    ctrl = (C_Button *)winme->FindControl(DF_SAVE_CTRL);

    if (ctrl)
        ctrl->SetCallback(SaveDFSettingsCB);

    // DF_TOOL Callbacks

    ctrl = (C_Button *)winme->FindControl(SINGLE_FLY_CTRL);

    if (ctrl)
        ctrl->SetCallback(DogfightFlyCB);

    ctrl = (C_Button *)winme->FindControl(COMMS_FLY_CTRL);

    if (ctrl)
        ctrl->SetCallback(DogfightFlyCB);

    // DF_TEAMS Callbacks

    // Team play stuff
    ctrl = (C_Button *)winme->FindControl(DF_CRIMSON);

    if (ctrl)
        ctrl->SetCallback(AddDogfightPlayerCB);

    ctrl = (C_Button *)winme->FindControl(DF_SHARK);

    if (ctrl)
        ctrl->SetCallback(AddDogfightPlayerCB);

    ctrl = (C_Button *)winme->FindControl(DF_VIPER);

    if (ctrl)
        ctrl->SetCallback(AddDogfightPlayerCB);

    ctrl = (C_Button *)winme->FindControl(DF_TIGER);

    if (ctrl)
        ctrl->SetCallback(AddDogfightPlayerCB);

    // On your own stuff
    ctrl = (C_Button *)winme->FindControl(DF_MARK_CRIMSON);

    if (ctrl)
        ctrl->SetCallback(AddDogfightPlayerCB);

    ctrl = (C_Button *)winme->FindControl(DF_MARK_SHARK);

    if (ctrl)
        ctrl->SetCallback(AddDogfightPlayerCB);

    ctrl = (C_Button *)winme->FindControl(DF_MARK_VIPER);

    if (ctrl)
        ctrl->SetCallback(AddDogfightPlayerCB);

    ctrl = (C_Button *)winme->FindControl(DF_MARK_TIGER);

    if (ctrl)
        ctrl->SetCallback(AddDogfightPlayerCB);

    ctrl = (C_Button*)winme->FindControl(ADD_CRIMSON_PLANE);

    if (ctrl)
        ctrl->SetCallback(AddDogfightAICB);

    ctrl = (C_Button*)winme->FindControl(ADD_SHARK_PLANE);

    if (ctrl)
        ctrl->SetCallback(AddDogfightAICB);

    ctrl = (C_Button*)winme->FindControl(ADD_USA_PLANE);

    if (ctrl)
        ctrl->SetCallback(AddDogfightAICB);

    ctrl = (C_Button*)winme->FindControl(ADD_TIGER_PLANE);

    if (ctrl)
        ctrl->SetCallback(AddDogfightAICB);

    ctrl = (C_Button*)winme->FindControl(ADD_FURBALL_PLANE);

    if (ctrl)
        ctrl->SetCallback(AddDogfightAICB);

    ctrl = (C_Button*)winme->FindControl(DEL_CRIMSON_PLANE);

    if (ctrl)
        ctrl->SetCallback(RemoveAICB);

    ctrl = (C_Button*)winme->FindControl(DEL_SHARK_PLANE);

    if (ctrl)
        ctrl->SetCallback(RemoveAICB);

    ctrl = (C_Button*)winme->FindControl(DEL_TBIRD_PLANE);

    if (ctrl)
        ctrl->SetCallback(RemoveAICB);

    ctrl = (C_Button*)winme->FindControl(DEL_TIGER_PLANE);

    if (ctrl)
        ctrl->SetCallback(RemoveAICB);

    ctrl = (C_Button*)winme->FindControl(DEL_FURBALL_PLANE);

    if (ctrl)
        ctrl->SetCallback(RemoveAICB);

    ctrl = (C_Button*)winme->FindControl(OK_FLIGHT);

    if (ctrl)
        ctrl->SetCallback(AddDogfightFlightCB);

    ctrl = (C_Button*)winme->FindControl(CANCEL_FLIGHT);

    if (ctrl)
        ctrl->SetCallback(CloseWindowCB);

    // Tree stuff for dogfight players
    tree = (C_TreeList *)winme->FindControl(CRIMSON_TREE);

    if (tree)
        tree->SetCallback(SelectDogfightItemCB);

    tree = (C_TreeList *)winme->FindControl(SHARK_TREE);

    if (tree)
        tree->SetCallback(SelectDogfightItemCB);

    tree = (C_TreeList *)winme->FindControl(VIPER_TREE);

    if (tree)
        tree->SetCallback(SelectDogfightItemCB);

    tree = (C_TreeList *)winme->FindControl(TIGER_TREE);

    if (tree)
        tree->SetCallback(SelectDogfightItemCB);

    tree = (C_TreeList *)winme->FindControl(FURBALL_TREE);

    if (tree)
        tree->SetCallback(SelectDogfightItemCB);

    // DF_MAP Callbacks
    // sfr: hack to fix
    //crsr=(C_Cursor *)winme->FindControl(10049);
    crsr = (C_Cursor *)winme->FindControl(DF_MAP_CURSOR);

    if (crsr)
        crsr->SetCallback(MoveGameLocationCB);

    // Debriefing callbacks
    ctrl = (C_Button *)winme->FindControl(DF_CLOSE_WINDOW);

    if (ctrl)
        ctrl->SetCallback(CleanupDebriefCB);

    ctrl = (C_Button *)winme->FindControl(DF_SAVE_DBRF_CTRL);

    if (ctrl)
        ctrl->SetCallback(SaveResultsCB);

    // Delete File button
    ctrl = (C_Button *)winme->FindControl(DGFT_DELETE);

    if (ctrl)
        ctrl->SetCallback(DeleteVerifyFileCB);

    // Help GUIDE thing
    ctrl = (C_Button*)winme->FindControl(UI_HELP_GUIDE);

    if (ctrl)
        ctrl->SetCallback(UI_Help_Guide_CB);

    tree = (C_TreeList *)winme->FindControl(DOGFIGHT_TREE);

    if (tree)
    {
        DogfightGames = tree;
        DogfightGames->SetCallback(SelectDogfightGameCB);
    }
}
