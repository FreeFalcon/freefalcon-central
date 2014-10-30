// Main solution's file. Starts and stops everything else.


// SYSTEM INCLUDES
#include <AtlBase.h>
#include <AtlCom.h>
#include <AtlWin.h>
#include <direct.h>
#include <StdIO.h>
#include <time.h>
#include <windows.h>
#include <WinSock2.h>
// END OF SYSTEM INCLUDES


// SIM INCLUDES
#include "ASCII.h"
#include "Camp2Sim.h"
#include "campaign.h"
#include "CampJoin.h"
#include "CampStr.h"
#include "ClassTbl.h"
#include "CmpClass.h"
#include "dDraw.h"
#include "dialog.h" // Campaign tool includes
#include "DispCfg.h"
#include "DispOpts.h"
#include "eHandler.h"
#include "entity.h"
#include "F4Comms.h"
#include "F4Find.h"
#include "F4Version.h"
#include "FalcLib.h"
#include "FalcMem.h"
#include "FalcMesg.h"
#include "FalcUser.h"
#include "feature.h"
#include "find.h"
#include "fSound.h"
#include "HUD.h"
#include "iAction.h"
#include "InpFunc.h"
#include "LogBook.h"
#include "MissEval.h"
#include "OtwDrive.h"
#include "PlayerOp.h"
#include "RadioSubtitle.h"
#include "resource.h"
#include "rules.h"
#include "SimDrive.h"
#include "SimIO.h"
#include "SimLoop.h"
#include "SimObj.h"
#include "sInput.h"
#include "SMS.h"
#include "statistics.h"
#include "StdHdr.h"
#include "SubRange.h"
#include "TheaterDef.h"
#include "ThreadMgr.h"
#include "TimerThread.h"
#include "token.h" // default value Unz
#include "TrackIR.h"
#include "UI_ia.h"
#include "Uicomms.h"
#include "UserIDs.h"
#include "VrInput.h"
#include "weather.h"
// END OF SIM INCLUDES


// ADDITIONAL SIM INCLUDES
#include "CodeLib/resources/ResLib/src/ResMgr.h"
#include "FalcSnd/pSound.h"
#include "FalcSnd/VoiceMapper.h"
#include "FalcSnd/WinAmpFrontend.h"
#include "graphics/include/DrawParticleSys.h"
#include "graphics/include/ImageBuf.h"
#include "graphics/include/TexBank.h"
#include "include/ComSup.h"
#include "movie/AviMovie.h"
#include "UI95/cHandler.h"

extern "C"
{
#include "AmdLib.h"
}
// END OF ADDITIONAL SIM INCLUDES



// PREPROCESSOR DIRECTIVES
#undef fopen
#undef fclose

// These are needed for network support.
#pragma warning(disable:4192)
#import "gnet\bin\core.tlb"
#import "gnet\bin\shared.tlb" named_guids
#pragma warning(default:4192)
// END OF PREPROCESSOR DIRECTIVES



// GLOBAL CONSTANTS
// This is the only place in the entire code base where the name and the
// version should be defined.
const char* FREE_FALCON_BRAND = "FreeFalcon";
const char* FREE_FALCON_PROJECT = "Open Source Project";
const char* FREE_FALCON_VERSION = "7.0.0";
// END OF GLOBAL CONSTANTS



// GLOBAL VARIABLES
bool intro_movie = true;
bool cockpit_verifier = false;
bool g_writeMissionTbl = false;
bool g_writeSndTbl = false;
CComModule _Module;
char bottom_space[] = "                                                                               ";
char FalconCockpitThrDirectory[_MAX_PATH]; // Theater switching stuff
char FalconMovieDirectory[_MAX_PATH];
char FalconMovieMode[_MAX_PATH];
char FalconSoundThrDirectory[_MAX_PATH];
char FalconSplashThrDirectory[_MAX_PATH];
char FalconTacrefThrDirectory[_MAX_PATH];
char FalconUIArtDirectory[_MAX_PATH];
char FalconUIArtThrDirectory[_MAX_PATH];
char FalconUISoundDirectory[_MAX_PATH];
char FalconZipsThrDirectory[_MAX_PATH];
class tactical_mission;
extern bool g_bEnableUplink;
extern bool g_bEnumSoftwareDevices;
extern bool g_bPilotEntertainment;
extern BOOL ReadyToPlayMovie; // defined in UI_Cmpgn.cpp
extern C_Handler *gMainHandler;
extern C_SoundBite *gInstantBites, *gDogfightBites, *gCampaignBites;
extern char *BSP;
extern char *BTP;
extern char FalconPictureDirectory[_MAX_PATH]; // JB 010623
extern char g_strMasterServerName[0x40];
extern char g_strServerAdmin[0x40];
extern char g_strServerAdminEmail[0x40];
extern char g_strServerLocation[0x40];
extern char g_strServerName[0x40];
extern char gUI_AutoSaveName[];
extern char gUI_CampaignFile[];
extern float UR_HEAD_VIEW;
extern int flag_keep_smoke_trails;
extern int g_nMasterServerPort;
extern int gCampDataVersion, gCurrentDataVersion, gClearPilotInfo, gTacticalFullEdit;
extern int GraphicSettingMult;
extern int gTimeModeServer;
extern int gUnlimitedAmmo;
extern int HighResolutionHackFlag;
extern int MainLastGroup;
extern int voice_;
extern long CampEventSoundID;
extern long gScreenShotEnabled;
extern long MovieCount;
extern uchar gCampJoinTries;
extern ulong gCampJoinLastData; // Last vuxRealtime we received data about this game
falcon4LeakCheck flc;
GNETCORELib::IUplinkPtr m_pUplink;
HINSTANCE hInst;
HWND mainAppWnd;
HWND mainMenuWnd;
int ClearObjManualFlags = FALSE;
int DestroyObjective = FALSE;
int DisableSmoothing = FALSE;
int displayCampaign = FALSE;
int doNetwork = FALSE; // referred in splash.cpp
int doUI = FALSE;
int eyeFlyEnabled = FALSE;
int NoRudder = FALSE;
int noUIcomms = FALSE;
int NumHats = -1;
int numZips = 0;
int RepairObjective = FALSE;
int SimPathHandle = -1;
int wait_for_loaded = TRUE;
int weatherCondition = SUNNY;
int* resourceHandle;
RadioSubTitle* radioLabel = (RadioSubTitle*)0;
RealWeather *realWeather = NULL;
static HACCEL hAccel;
static int KeepFocus = 0;
TrackIR theTrackIRObject;
WinAmpFrontEnd* winamp = 0;
WSADATA windows_sockets_data;

extern "C" char g_strLgbk[20]; //sfr: logbook debug
char g_strLgbk[20];

#ifdef DEBUG
	extern int gCampPlayerInput;
	extern int gPlayerPilotLock;
	HANDLE gDispatchThreadID;
#endif

#ifdef CAMPTOOL
	// Renaming tool stuff
	extern int gRenameIds;
	extern VU_ID_NUMBER RenameTable[65536];
	// Window handles
	extern HWND hMainWnd;
	extern HWND hToolWnd;
#endif

#ifdef DEBUG// Debug Assert softswitches
	int f4AssertsOn = TRUE, f4HardCrashOn = FALSE;
	int shiAssertsOn = TRUE,
	shiWarningsOn = TRUE,
	shiHardCrashOn = FALSE;
	extern CampaignTime gConnectionTime;
	extern CampaignTime gResendTime;
	extern int gCampJoinStatus;
#endif

extern "C"
{
	extern int ComIPGetHostIDIndex;
	extern int force_ip_address;
	extern unsigned short force_port;
}
// END OF GLOBAL VARIABLES



// FUNCTION DECLARATIONS
BOOL CleanupDIJoystick(void);
BOOL DoSimOptions(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL SetupDIJoystick(HINSTANCE hInst, HWND hWnd);
BEGIN_OBJECT_MAP(ObjectMap)
END_OBJECT_MAP()
extern BOOL CALLBACK SelectMission(HWND, UINT, WPARAM, LPARAM);
extern BOOL SaveSFXTable();
extern BOOL WINAPI BriefDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
extern BOOL WINAPI SelectSquadron(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
extern BOOL WriteMissionData();
extern HRESULT  StartServer(HWND hDlg);  //me123
extern void CampaignConnectionTimer(void);
extern void CampaignJoinFail(void);
extern void CampaignJoinSuccess(void);
extern void CampaignPreloadSuccess(int remote);
extern void CampMain(HINSTANCE hInstance, int nCmdShow);
extern void CheckCampaignFlyButton(void);
extern void CopyDFSettingsToWindow(void);
extern void DisableCampaignMenus(void);
extern void DisplayJoinStatusWindow(int);
extern void EnableCampaignMenus(void);
extern void GameHasStarted(void);
extern void LoadTheaterList(); // JPO
extern void LoadTrails();
extern void PlayUIMovieQ(); // defined in UI_Main.cpp
extern void ReadCampAIInputs(char * name);
extern void ReadFalcon4Config();
extern void ServerBrowserExit();
extern void StopVoice();
extern void UIScramblePlayerFlight(void);
extern void update_tactical_flight_information(void);
extern void UpdateMissionWindow(long ID);
int FileVerify(void);
int tactical_is_training(void);
int UI_Startup();
LRESULT CALLBACK PlayVoicesProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK SimWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
static void CtrlAltDelMask(int state);
static void ParseCommandLine(LPSTR cmdLine);
static void SystemLevelExit(void);
static void SystemLevelInit(void);
struct __declspec(uuid("41C27D56-3A03-4E9D-BE01-3423126C3983")) GameSpyUplink;
void ConsoleWrite(char *);
void EndCommitCB(long ID, short hittype, C_Base *control);
void IncDecDataToPlay(int delta);
void IncDecMsgToPlay(int delta);
void IncDecTalkerToPlay(int delta);
void LeaveDogfight();
void OpenMainCampaignCB(long ID, short hittype, C_Base *control);
void OpenTEGameOverWindow();
void PlayMovie(char *filename, int left, int top, int w, int h, void *theSurface);// not void PlayMovie(char *filename,short left,short top,short w,short h,UInt theSurface);
void PlayThatFunkyMusicWhiteBoy();
void ProcessChatStr(CHATSTR *msg);
void RebuildCurrentWPList();
void RebuildGameTree();
void RecieveScenarioInfo();
void RelocateSquadron();
void ReplayUIMovie(long MovieID);
void SetVoiceVolumes(void);
void ShutdownCampaign(void);
void STPRender(C_Base *control);
void tactical_restart_mission(void);
void UI_Cleanup();
void UI_CommsErrorMessage(WORD error);
void UI_HandleAirbaseDestroyed();
void UI_HandleAirbaseDestroyed();
void UI_HandleAircraftDestroyed();
void UI_HandleFlightCancel();
void UI_LoadSkyWeatherData();
void UI_UpdateDogfight(long winID, short Setting); // LParam=Window,wParam=Setting
void UI_UpdateGameList();
void UI_UpdateOccupationMap();
void UI_UpdateVU();
void UIMain(void);
void UpdateRules(void);
void ViewRemoteLogbook(long playerID);

extern "C" int initialize_windows_sockets(WSADATA *wsaData);
// END OF FUNCTION DECLARATIONS



// FUNCTION DEFINITIONS
void BuildAscii()
{
    short i, kbd, scan;
    short vkey, shiftstates;

    for (i = 31; i < 256; i++)
    {
        vkey = VkKeyScan(static_cast<char>(i));

        if (vkey not_eq -1)
        {
            kbd = static_cast<short>(MapVirtualKey(vkey, 2) bitand 0xff);

            if (kbd)
            {
                shiftstates = static_cast<short>((vkey >> 8) bitand 0x07);
                scan = static_cast<short>(MapVirtualKey(vkey, 0) bitand 0xff);

                Key_Chart[scan].Ascii[shiftstates] = static_cast<char>(i);
                Key_Chart[scan].Flags[shiftstates] or_eq _IS_ASCII_;

                if (i >= '0' and i <= '9')
                    Key_Chart[scan].Flags[shiftstates] or_eq _IS_DIGIT_;

                if (isalpha(i))
                    Key_Chart[scan].Flags[shiftstates] or_eq _IS_ALPHA_;
            }
        }
    }
}


static BOOLEAN initApplication(HINSTANCE hInstance, HINSTANCE hPrevInstance, int)
{
    WNDCLASS wc;
    BOOL rc;

    if ( not hPrevInstance)
    {
        wc.style = CS_HREDRAW bitor CS_VREDRAW;
        wc.lpfnWndProc = (WNDPROC)SimWndProc; // The client window procedure.
        wc.cbClsExtra = 0;                     // No room reserved for extra data.
        wc.cbWndExtra = sizeof(DWORD);
        wc.hInstance = hInstance;
        //      wc.hIcon = LoadIcon (hInstance, "ICON1.ICO");
        wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1)); // OW BC
        wc.hCursor = NULL;
        wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
        wc.lpszMenuName = MAKEINTRESOURCE(F4_DEMO_MENU);
        wc.lpszClassName = "Falcon4Class";
        rc = RegisterClass(&wc);

        if ( not rc)
        {
            return FALSE;
        }
    }
	mainMenuWnd = CreateWindow("Falcon4Class",
							   "FreeFalcon 7.0 Debug Window",
                               WS_OVERLAPPEDWINDOW,
                               720,
                               100,
                               240,
                               100,
                               NULL,
                               NULL,
                               hInstance,
                               NULL);

#ifndef NDEBUG
    ShowWindow(mainMenuWnd, SW_SHOW);
#endif

    hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_FALCON4_ACC1));
    return TRUE;

}


// Initialize global debugging variables we need only in DEBUG.
void initialize_variables(void)
{

	cockpit_verifier = true;

};


signed int PASCAL handle_WinMain(HINSTANCE h_instance, 
								 HINSTANCE h_previous_instance,
                                 LPSTR command_line, signed int command_show)
{

#ifndef NDEBUG
	initialize_variables();
#endif // NDEBUG

    _Module.Init(ObjectMap, h_instance); // ATL initialization.

	// Initialize WinSock now, we need it for GNet.
    initialize_windows_sockets(&windows_sockets_data);

    HRESULT hr = CoInitialize(NULL);

    if (FAILED(hr))
        MonoPrint("handle_WinMain: Error 0x%X occured during COM initialization", hr);

    // Begin - Uplink stuff
    try
    {
        if (g_bEnableUplink)
        {
            // Make sure all objects are registered
            ComSup::RegisterServer("GNGameSpy.dll");
            ComSup::RegisterServer("GNCorePS.dll");
            ComSup::RegisterServer("GNShared.dll");

            m_pUplink->PutMasterServerName(g_strMasterServerName);
            m_pUplink->PutMasterServerPort(g_nMasterServerPort);
            m_pUplink->PutQueryPort(7778);
            m_pUplink->PutHeartbeatInterval(60000);
			m_pUplink->PutServerVersion(FREE_FALCON_VERSION);
			m_pUplink->PutServerVersionMin(FREE_FALCON_VERSION);
            m_pUplink->PutServerLocation(g_strServerLocation);
            m_pUplink->PutServerName(g_strServerName);
            m_pUplink->PutGameName(FREE_FALCON_BRAND);
            m_pUplink->PutGameMode("openplaying");
        }
    }
    catch (_com_error e)
    {
        MonoPrint("handle_WinMain: Error 0x%X occured during JetNet initialization", e.Error());
    }

    // End - Uplink stuff

    /**
     * Set up data associated with this window
    **/

#ifdef USE_SMART_HEAP
    MemRegisterTask();
#endif

#ifdef _MSC_VER
    // Set the FPU to Truncate
    _controlfp(_RC_CHOP, MCW_RC);

    // Set the FPU to 24bit precision
    _controlfp(_PC_24, MCW_PC);
#endif

    hInst = h_instance;

    ParseCommandLine(command_line);

    ReadFalcon4Config();

    realWeather = new WeatherClass();

    // This SHOULD NOT BE REQUIRED -- IT IS *VERY* EASY TO BREAK CODE THAT DEPENDS ON THIS
    // I'd like to make it go away soon...
    SetCurrentDirectory(FalconDataDirectory);

    FileVerify();

    sprintf(FalconCampaignSaveDirectory, "%s\\Campaign\\Save", FalconDataDirectory);
    sprintf(FalconCampUserSaveDirectory, "%s\\Campaign\\Save", FalconDataDirectory);

    // Initialize this
    sprintf(FalconMiscTexDataDir, "%s\\terrdata\\misctex", FalconDataDirectory);

    sprintf(FalconPictureDirectory, "%s\\Pictures", FalconDataDirectory);

    // Create PictureDir if not present
    _mkdir(FalconPictureDirectory);

    ResInit(NULL);
    ResCreatePath(FalconDataDirectory, FALSE);
    ResAddPath(FalconCampaignSaveDirectory, FALSE);
    char tmpPath[_MAX_PATH];
    sprintf(tmpPath, "%s\\Config", FalconDataDirectory);
    ResAddPath(tmpPath, FALSE);
    sprintf(tmpPath, "%s\\Art", FalconDataDirectory); // This one can go if zips are always used
    ResAddPath(tmpPath, TRUE);
    sprintf(tmpPath, "%s", FalconPictureDirectory);  // JB 010623
    ResAddPath(tmpPath, TRUE);  // JB 010623

    // This SHOULD NOT BE REQUIRED -- IT IS *VERY* EASY TO BREAK CODE THAT DEPENDS ON THIS
    // I'd like to make it go away soon...
    ResSetDirectory(FalconDataDirectory);

#ifdef __WATCOMC__
    chdir(FalconDataDirectory);
#else
    _chdir(FalconDataDirectory);
#endif
	char fileName[_MAX_PATH];
	sprintf(fileName, "%s\\%s.ini", FalconObjectDataDir, "Falcon4");

    gLangIDNum = GetPrivateProfileInt("Lang", "Id", 0, fileName);

    UI_LoadSkyWeatherData();

    DisplayOptions.LoadOptions("display");

    FalconDisplay.Setup(gLangIDNum);

    mainAppWnd = FalconDisplay.appWin;

    if (g_writeSndTbl)
        SaveSFXTable();

    if (g_writeMissionTbl)
        WriteMissionData();

    if (gSoundFlags bitand FSND_SOUND) // Switch for turning on/off sound stuff
        InitSoundManager(FalconDisplay.appWin, 0, FalconDataDirectory);

    g_voicemap.LoadVoices();

    if ( not initApplication(h_instance, h_previous_instance, command_show))
        return FALSE;

	MSG  msg;
	while (GetMessage(&msg, NULL, 0, 0) not_eq 0)
    {
        DispatchMessage(&msg);
    }

    SystemLevelExit();

    // Since its initialized here, finalize here
    delete realWeather;

    _Module.Term();

    if (m_pUplink)
        m_pUplink.Release();

    CoUninitialize();

    ExitProcess(EXIT_SUCCESS);
}


// Main entry point. Calls initialization functions, processes message loop.
// However, some code is called by callback functions so use breakpoints
// to debug properly
signed int PASCAL WinMain(HINSTANCE h_Instance, HINSTANCE h_previous_instance,
                          LPSTR command_line, signed int command_show)
{

// We want the SubRange template to run only under DEBUG.
// Don't use #ifdef DEBUG because the DEBUG macro is defined in RELEASE too
#ifndef NDEBUG
	SubRange<signed int, 0, 1> error_code;
#else
	signed int error_code;
#endif // NDEBUG

	error_code = EXIT_FAILURE;

	// Set up structured exception handling here.
		__try
    {
        error_code = handle_WinMain(h_Instance, h_previous_instance,
  								    command_line, command_show);
    }
    __except (RecordExceptionInfo(GetExceptionInformation(), "WinMain Thread"))
    {
        // Do nothing here - RecordExceptionInfo() has already done
        // everything that is needed. Actually this code won't even
        // get called unless you return EXCEPTION_EXECUTE_HANDLER from
        // the __except clause.
    }

    return error_code;
}


void EndUI(void)
{
    // Looking for multiplayer stomp...
    ShiAssert(TeamInfo[1] == NULL or TeamInfo[1]->cteam not_eq 0xFC);
    ShiAssert(TeamInfo[2] == NULL or TeamInfo[2]->cteam not_eq 0xFC);

    doUI = FALSE;
    TheCampaign.Suspend();
    UI_Cleanup();
    TheCampaign.Resume();

    SetFocus(mainMenuWnd);
}


LRESULT CALLBACK SimWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT retval = 0;

    // Looking for multiplayer stomp...
    ShiAssert(TeamInfo[1] == NULL or TeamInfo[1]->cteam not_eq 0xFC);
    ShiAssert(TeamInfo[2] == NULL or TeamInfo[2]->cteam not_eq 0xFC);

    switch (message)
    {
        case WM_CREATE :
            SetFocus(hwnd);
            retval = 0;
            break;

#ifdef _DEBUG // We only care about these in debug mode

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case ID_FILE_EXIT:
                    PostQuitMessage(0);
                    retval = 0;
                    break;

                case ID_CAMPAIGN_NEW:
                    TheCampaign.NewCampaign((FalconGameType)lParam, "default");
                    EnableCampaignMenus();
                    break;

                case ID_CAMPAIGN_LOAD:
                    if ( not OpenCampFile(hwnd))
                    {
                        TheCampaign.EndCampaign();
                        return 0;
                    }

                    EnableMenuItem(GetMenu(hwnd), ID_CAMPAIGN_SAVE, MF_ENABLED);
                    EnableCampaignMenus();
                    break;

                case ID_CAMPAIGN_EXIT:
                    DisableCampaignMenus();
                    TheCampaign.EndCampaign();
#if CAMPTOOL

                    if (hMainWnd)
                        PostMessage(hMainWnd, WM_CLOSE, 0, 0);

#endif
                    DisableCampaignMenus();
                    break;

                case ID_CAMPAIGN_SAVE:
                    SaveCampFile(hwnd, LOWORD(wParam));
                    break;

                case ID_CAMPAIGN_SAVEAS:
                case ID_CAMPAIGN_SAVEALLAS:
                case ID_CAMPAIGN_SAVEINSTANTAS:
                    SaveAsCampFile(hwnd, LOWORD(wParam));
                    break;

                case ID_CAMPAIGN_DISPLAY:
#ifdef CAMPTOOL
                    if ( not displayCampaign)
                    {
                        CampMain(hInst, SW_SHOW);
                        displayCampaign = TRUE;
                    }
                    else
                    {
                        if (hMainWnd)
                            PostMessage(hMainWnd, WM_CLOSE, 0, 0);

                        displayCampaign = FALSE;
                    }

                    CheckMenuItem(GetMenu(hwnd), ID_CAMPAIGN_DISPLAY, (displayCampaign ? MF_CHECKED : MF_UNCHECKED));
#endif CAMPTOOL
                    break;

                case ID_CAMPAIGN_PAUSED:
                    if (gameCompressionRatio)
                        SetTimeCompression(0);
                    else
                        SetTimeCompression(1);

                    CheckMenuItem(GetMenu(hwnd), ID_CAMPAIGN_PAUSED, (gameCompressionRatio ? MF_UNCHECKED : MF_CHECKED));
                    break;

#ifdef CAMPTOOL

                case ID_CAMPAIGN_SELECTSQUADRON:
                    DialogBox(hInst, MAKEINTRESOURCE(IDD_SQUADRONDIALOG), mainMenuWnd, (DLGPROC)SelectSquadron);
                    break;

                case ID_CAMPAIGN_FLYMISSION:
                    DialogBox(hInst, MAKEINTRESOURCE(IDD_MISSDIALOG), mainMenuWnd, (DLGPROC)SelectMission);
                    break;

                case ID_CAMPAIGN_RENAMINGON:
                    gRenameIds = 1 - gRenameIds;
                    CheckMenuItem(GetMenu(hwnd), ID_CAMPAIGN_RENAMINGON, (gRenameIds ? MF_CHECKED : MF_UNCHECKED));
                    break;
#endif

                case ID_SOUND_START:
                    F4SoundStart();
                    break;

                case ID_SOUND_STOP:
                    F4SoundStop();
                    break;

                case ID_VOICES_TOOL:
                    DialogBox(hInst, MAKEINTRESOURCE(IDD_PLAYVOICES), FalconDisplay.appWin, (DLGPROC)PlayVoicesProc);
                    break;

                case ID_UI_AIRBASE:
                    PostMessage(FalconDisplay.appWin, FM_START_UI, 0, 0); // Start UI
#ifdef DEBUG
                    ShowCursor(FALSE); // Turn off mouse cursor for until EXIT in UI is selected
#endif
                    break;

                default:
                    retval = 0;
                    break;
            }

            break;
#endif // Debug stuff...

        case WM_DESTROY :
            PostMessage(hwnd, WM_COMMAND, ID_FILE_EXIT, 0);
            retval = 0;
            break;

        case WM_ACTIVATE:
            retval = 0;
            break;

        case WM_USER:
            retval = 0;
            break;

        case FM_DISP_ENTER_MODE:
        {
            FalconDisplay._EnterMode((FalconDisplayConfiguration::DisplayMode) wParam, LOWORD(lParam), HIWORD(lParam));
            break;
        }

        case FM_DISP_LEAVE_MODE:
        {
            FalconDisplay._LeaveMode();
            break;
        }

        case FM_DISP_TOGGLE_FULLSCREEN:
        {
            FalconDisplay._ToggleFullScreen();
            break;
        }

        default:
            retval = DefWindowProc(hwnd, message, wParam, lParam);
            break;
    }

    return retval;
}


int get_ip(char *str)
{
    char
    *src;

    unsigned int
    addr = 0;

    if ( not str)
        return 0;

    src = str;

    while (*src)
    {
        if (*src not_eq '.')
            ++src;
        else
        {
            *src = 0;
            addr = addr * 256 + atoi(str);
            *src = '.';
            ++src;
            str = src;
        }
    }

    addr = addr * 256 + atoi(str);

    return addr;
}


void ParseCommandLine(LPSTR cmdLine)
{
    char* arg;
    LONG retval = ERROR_SUCCESS;
    DWORD value;
    DWORD type, size;
    HKEY theKey;



#ifdef DEBUG
	// These are debug options. Set whatever you need.
		//F4SetAsserts(TRUE);
		//ShiSetAsserts(TRUE);
		//RepairObjective = 1;
		//eyeFlyEnabled = TRUE;
  //      wait_for_loaded = FALSE;
  //      F4SetHardCrash(TRUE);
  //      ShiSetHardCrash(TRUE);
  //      gSoundFlags = 0;

		InitDebug(DEBUGGER_TEXT_MODE);
#endif

    size = sizeof(FalconDataDirectory);
    retval = RegOpenKeyEx(HKEY_LOCAL_MACHINE, FALCON_REGISTRY_KEY,
                          0, KEY_QUERY_VALUE, &theKey);

    size = sizeof(ComIPGetHostIDIndex);
    retval = RegQueryValueEx(theKey, "HostIDX", 0, &type, (LPBYTE)&value, &size);

    if (retval == ERROR_SUCCESS)
    {
        ComIPGetHostIDIndex = value;
    }

    retval = RegCloseKey(theKey);

    // Parse Command Line
    arg = strtok(cmdLine, " ");

    //sfr: zero lgbk
    memset(g_strLgbk, 0, 20);

    if (arg not_eq NULL)
    {
        do
        {
            if ( not stricmp(arg, "-file"))
                SimDriver.doFile = 1 - SimDriver.doFile;

            if ( not stricmp(arg, "-event"))
                SimDriver.doEvent = TRUE;

            if (_strnicmp(arg, "-repair", 2) == 0)
                RepairObjective = 1;

            if (stricmp(arg, "-armageddon") == 0)
                DestroyObjective = 1;

            if (stricmp(arg, "-log") == 0)
                log_frame_rate = TRUE;

            if (_strnicmp(arg, "-C", 2) == 0)
                ClearObjManualFlags = 1;

            if (_strnicmp(arg, "-UA", 3) == 0)
                gUnlimitedAmmo ++;

            if ( not _strnicmp(arg, "-g", 2))
            {
                int temp = atoi(&arg[2]);
                GraphicSettingMult = temp >= 1 ? temp : 1;
            }

            if ( not stricmp(arg, "-window"))
                FalconDisplay.displayFullScreen = false;

            if (stricmp(arg, "-hires") == 0)
                HighResolutionHackFlag = TRUE;

            if ( not stricmp(arg, "-norudder"))
                NoRudder = TRUE;

            if ( not stricmp(arg, "-nosmoothing"))
                DisableSmoothing = TRUE;

            if ( not stricmp(arg, "-numhats"))
                if ((arg = strtok(NULL, " ")) not_eq NULL)
                    NumHats = atoi(arg);

            if (_strnicmp(arg, "-nosound", 8) == 0)
                gSoundFlags and_eq (0xffffffff xor FSND_SOUND);

            if (_strnicmp(arg, "-nopete", 7) == 0)
                gSoundFlags and_eq (0xffffffff xor FSND_REPETE);

#ifdef DEBUG

            if (_strnicmp(arg, "-noassert", 9) == 0)
            {
                F4SetAsserts(FALSE);
                // KCK: If this line is causing your compile to fail, update
                // codelib, don't comment it out.
                ShiSetAsserts(FALSE);
            }

            // JB 010325
            if (_strnicmp(arg, "-nowarning", 10) == 0)
                ShiSetWarnings(FALSE);

            if (_strnicmp(arg, "-hardcrash", 9) == 0)
            {
                F4SetAsserts(TRUE);
                F4SetHardCrash(TRUE);
                // KCK: If this line is causing your compile to fail, update
                // codelib, don't comment it out.
                ShiSetHardCrash(TRUE);
                ShiSetAsserts(TRUE);
            }

            if (stricmp(arg, "-resetpilots") == 0)
                gClearPilotInfo = 1;

#endif

            if (stricmp(arg, "-tacedit") == 0)
                gTacticalFullEdit = 1;

            if (stricmp(arg, "-norsc") == 0)
                _LOAD_ART_RESOURCES_ = 0;

            if (stricmp(arg, "-usersc") == 0)
                _LOAD_ART_RESOURCES_ = 1;

			if (_strnicmp(arg, "-nomovie", 8) == 0)
				intro_movie = false;

            if (_strnicmp(arg, "-noUIcomms", 8) == 0)
                noUIcomms = TRUE;

            if (_strnicmp(arg, "-time", 5) == 0)
                gTimeModeServer = 1;

            if (_strnicmp(arg, "-noloader", 9) == 0)
                wait_for_loaded = FALSE;

#ifdef DEBUG

            if (_strnicmp(arg, "-campinput", 10) == 0)
                gCampPlayerInput = atoi(arg + 10);

#endif

            if ( not stricmp(arg, "-bandwidth") or not stricmp(arg, "-bandwith") or not stricmp(arg, "-bw"))
                if ((arg = strtok(NULL, " ")) not_eq NULL)
                {
                    F4CommsBandwidth = atoi(arg);

                    if (F4CommsBandwidth < -1)
                        F4CommsBandwidth *= -1;
                }

            if ( not stricmp(arg, "-urview"))
                if ((arg = strtok(NULL, " ")) not_eq NULL)
                {
                    UR_HEAD_VIEW = (float)atoi(arg);

                    if (UR_HEAD_VIEW < 50)
                        UR_HEAD_VIEW = 50;

                    if (UR_HEAD_VIEW > 160)
                        UR_HEAD_VIEW = 160;
                }

            if ( not stricmp(arg, "-latency"))
                if ((arg = strtok(NULL, " ")) not_eq NULL)
                {
                    F4CommsLatency = atoi(arg);

                    if (F4CommsLatency < 0)
                        F4CommsLatency *= -1;
                }

            if ( not stricmp(arg, "-drop"))
                if ((arg = strtok(NULL, " ")) not_eq NULL)
                {
                    F4CommsDropInterval = atoi(arg);

                    if (F4CommsDropInterval < 0)
                        F4CommsDropInterval *= -1;
                }

            if ( not stricmp(arg, "-session"))
                if ((arg = strtok(NULL, " ")) not_eq NULL)
                {
                    F4SessionUpdateTime = atoi(arg);
                }

            if (( not stricmp(arg, "-hostidx")) or ( not stricmp(arg, "-hostid")))
                if ((arg = strtok(NULL, " ")) not_eq NULL)
                {
                    ComIPGetHostIDIndex = atoi(arg);
                }

            if ( not stricmp(arg, "-alive"))
                if ((arg = strtok(NULL, " ")) not_eq NULL)
                {
                    F4SessionAliveTimeout = atoi(arg);
                }

            if ( not stricmp(arg, "-mono")) // turn on MONOCHROME support
                InitDebug(DEBUGGER_TEXT_MODE);

            if ( not stricmp(arg, "-nomono")) // turn off MONOCHROME support
                InitDebug(-1);

            if ( not stricmp(arg, "-head")) // turn on head tracking support
                OTWDriver.SetHeadTracking(TRUE);

            if ( not stricmp(arg, "-swap"))
                if ((arg = strtok(NULL, " ")) not_eq NULL)
                {
                    F4CommsLatency = atoi(arg);

                    if (F4CommsSwapInterval < 0)
                        F4CommsSwapInterval *= -1;
                }

            if ( not stricmp(arg, "-mtu"))  // Booster and Unz At work
            {
                if ((arg = strtok(NULL, " ")) not_eq NULL)
                {
                    F4CommsMTU = atoi(arg);

                    if (F4CommsMTU < 0)
                        F4CommsMTU *= -1;
                }
            }
            else F4CommsMTU = 500; // Unz Ugly...but it works

            if ( not stricmp(arg, "-ef"))
                eyeFlyEnabled = 1 - eyeFlyEnabled;

            if ( not stricmp(arg, "-ip"))
            {
                if ((arg = strtok(NULL, " ")) not_eq NULL)
                    force_ip_address = atoi(arg);

                MonoPrint("Force IP Address to %08x\n", force_ip_address);
            }

            //sfr converts
            // added for ports
            if ( not _strnicmp(arg, "-port", 5))
                if ((arg = strtok(NULL, " ")) not_eq NULL)
                    force_port = (unsigned short)atoi(arg);

            // sfr: no T&L checks
            // added for server and notebooks
            if ( not stricmp(arg, "-notnl"))
                // exclude emulation devices drivers
                DisplayOptionsClass::SetDevCaps(D3DDEVCAPS_HWRASTERIZATION);

            // force logbook
            if ( not _strnicmp(arg, "-lgbk", 5))
                if ((arg = strtok(NULL, " ")) not_eq NULL)
                {
                    // select a given logbook if it exists
                    sprintf(g_strLgbk, "%.19s", arg);

                }

            if ( not stricmp(arg, "-smoke"))
                flag_keep_smoke_trails = TRUE;

            // OW
            if ( not stricmp(arg, "-enumswdev"))
                g_bEnumSoftwareDevices = true;

            if ( not stricmp(arg, "-nocockpitverifier"))
                cockpit_verifier = false;

            if ( not stricmp(arg, "-writesndtbl"))
                g_writeSndTbl = true;

            if ( not stricmp(arg, "-writemissiontbl"))
                g_writeMissionTbl = true;

        }
        while ((arg = strtok(NULL, " ")) not_eq NULL);
    }

    size = sizeof(FalconDataDirectory);
    retval = RegOpenKeyEx(HKEY_LOCAL_MACHINE, FALCON_REGISTRY_KEY,
                          0, KEY_QUERY_VALUE, &theKey);
    retval = RegQueryValueEx(theKey, "baseDir", 0, &type, (LPBYTE)&FalconDataDirectory, &size);

    if (retval not_eq ERROR_SUCCESS)
    {
        SimLibPrintMessage("No Registry Variable\n");
        strcpy(FalconDataDirectory, ".\\");
    }

    size = sizeof(FalconTerrainDataDir);
    RegQueryValueEx(theKey, "theaterDir", 0, &type, (LPBYTE)FalconTerrainDataDir, &size);
    size = sizeof(FalconObjectDataDir);
    RegQueryValueEx(theKey, "objectDir", 0, &type, (LPBYTE)FalconObjectDataDir, &size);
    strcpy(Falcon3DDataDir, FalconObjectDataDir);
    size = sizeof(FalconMiscTexDataDir);

    size = sizeof(FalconMovieMode);
    retval = RegQueryValueEx(theKey, "movieMode", 0, &type, (LPBYTE)FalconMovieMode, &size);

    if (retval not_eq ERROR_SUCCESS)
        strcpy(FalconMovieMode, "Hurry");
    else if (size <= 1)
        strcpy(FalconMovieMode, "Hurry");

    size = sizeof(FalconUIArtDirectory);
    retval = RegQueryValueEx(theKey, "uiArtDir", 0, &type, (LPBYTE)FalconUIArtDirectory, &size);

    if (retval not_eq ERROR_SUCCESS)
    {
        strcpy(FalconUIArtDirectory, FalconDataDirectory);
        strcpy(FalconUIArtThrDirectory, FalconDataDirectory);
    }

    size = sizeof(FalconUISoundDirectory);
    retval = RegQueryValueEx(theKey, "uiSoundDir", 0, &type, (LPBYTE)FalconUISoundDirectory, &size);

    if (retval not_eq ERROR_SUCCESS)
    {
        strcpy(FalconUISoundDirectory, FalconDataDirectory);
    }

    strcpy(FalconSoundThrDirectory, FalconDataDirectory);
    strcat(FalconSoundThrDirectory, "\\sounds");
    retval = RegCloseKey(theKey);
}


void SystemLevelInit()
{
    SimDriver.InitializeSimMemoryPools();

    ASD = new AS_DataClass();
    srand((unsigned int) time(NULL));

    // This SHOULD NOT BE REQUIRED -- IT IS _VERY_ EASY TO BREAK CODE THAT DEPENDS ON THIS
    // I'd like to make it go away soon....
    ResSetDirectory(FalconDataDirectory);

    DrawableParticleSys::LoadParameters(); // MLR 1/31/2004 -

    // FRB - Restore access to \sim folder files before \zips\Simdata.zip
    char tmpPath[512];
    LoadTheaterList();
    TheaterDef *td;

    if ((td = g_theaters.GetCurrentTheater()))
    {
        SetCursor(gCursors[CRSR_WAIT]);
        g_theaters.SetNewTheater(td);

        if ((( not _strnicmp(td->m_name, "Korea", 5)) or ( not _strnicmp(td->m_name, "Eurowar", 7))) and (SimPathHandle == -1))
        {
            char tmpPath[256];
            sprintf(tmpPath, "%s\\sim", FalconDataDirectory); // JPO - so we can find raw sim files
            SimPathHandle = ResAddPath(tmpPath, TRUE);
        }

        g_theaters.DoSoundSetup();
        InitVU();
        SetCursor(gCursors[CRSR_F16]);
    }
    else
    {
        sprintf(tmpPath, "%s\\sim", FalconDataDirectory); // JPO - so we can find raw sim files

        if (SimPathHandle == -1)
            SimPathHandle = ResAddPath(tmpPath, TRUE);

        ReadCampAIInputs("Falcon4");

        if ( not LoadClassTable("Falcon4"))
        {
            MessageBox(NULL, "No Entities Loaded.", "Error", MB_OK bitor MB_ICONSTOP bitor MB_SETFOREGROUND);
            exit(0);
        }

        InitVU();

        if ( not LoadTactics("Falcon4"))
        {
            MessageBox(NULL, "No Tactics Loaded.", "Error", MB_OK bitor MB_ICONSTOP bitor MB_SETFOREGROUND);
            exit(0);
        }

        LoadTrails();
    }

#ifndef NO_TIMER_THREAD
    beginTimer();
#endif

#define NEW_SOUND_STARTUP_ORDER 1
#if NEW_SOUND_STARTUP_ORDER
    F4SoundStart();
#endif

    ThreadManager::setup();
    // sfr: simloop must begin before campaign
    SimulationLoopControl::StartSim();
    Camp_Init(1);


    BuildAscii();

    gCommsMgr = new UIComms;
    gCommsMgr->Setup(FalconDisplay.appWin);

    if (UI_logbk.Load())
    {
        LogBook.LoadData(&UI_logbk.Pilot);
    }

    SetupDIJoystick(hInst, FalconDisplay.appWin);

    // Retro 20Dec2003
    extern int g_nNumberOfSubTitles;
    extern int g_nSubTitleTTL;
    extern char g_strRadioflightCol[0x40]; // Retro 27Dec2003
    extern char g_strRadiotoPackageCol[0x40]; // Retro 27Dec2003
    extern char g_strRadioToFromPackageCol[0x40]; // Retro 27Dec2003
    extern char g_strRadioTeamCol[0x40]; // Retro 27Dec2003
    extern char g_strRadioProximityCol[0x40]; // Retro 27Dec2003
    extern char g_strRadioWorldCol[0x40]; // Retro 27Dec2003
    extern char g_strRadioTowerCol[0x40]; // Retro 27Dec2003
    extern char g_strRadioStandardCol[0x40]; // Retro 27Dec2003

    if (PlayerOptions.getSubtitles())
    {
        try
        {
            radioLabel = new RadioSubTitle(g_nNumberOfSubTitles, g_nSubTitleTTL);
            radioLabel->SetChannelColours(
                g_strRadioflightCol, g_strRadiotoPackageCol, g_strRadioToFromPackageCol,
                g_strRadioTeamCol, g_strRadioProximityCol, g_strRadioWorldCol,
                g_strRadioTowerCol, g_strRadioStandardCol
            );
        }
        catch (RadioSubTitle::Init_Error)
        {
            delete(radioLabel);
            radioLabel = 0;
            // Retro: hrmmm.. might want to consider telling the user about this somehow..
            PlayerOptions.SetSubtitles(false);
        }
    }

    // Retro 20Dec2003 ends

    theTrackIRObject.InitTrackIR(mainAppWnd); // Retro 26/09/03

    if (PlayerOptions.Get3dTrackIR() == false)
        OTWDriver.SetHeadTracking(FALSE); // Cobra - Make 3D pit mouselook work when TIR is user-selected "off".

    // Retro 3Jan2004 - starting up the winamp frontend class, the winamp win need not be active at this point
    if (g_bPilotEntertainment == true)
    {
        winamp = new WinAmpFrontEnd();

        if ( not winamp)
        {
            g_bPilotEntertainment = false;
        }
    }

    // ..ends

#if not NEW_SOUND_STARTUP_ORDER
    F4SoundStart();
#endif

    LoadFunctionTables();

#ifdef NDEBUG
    ShowCursor(FALSE);
#endif

}


void SystemLevelExit(void)
{
#ifdef NDEBUG
    ShowCursor(TRUE);
#endif

    while (ShowCursor(TRUE) < 0);

    ServerBrowserExit();
    StopVoice(); //me123
    CleanupDIAll();
    DrawableParticleSys::UnloadParameters(); // MLR 1/31/2004 -
    theTrackIRObject.ExitTrackIR(); // Retro 26/09/03

    // Retro 3Jan2004
    if (winamp)
    {
        delete(winamp);
        winamp = 0;
    }

    // ..ends

    if (radioLabel)
    {
        // Retro 20Dec2003 (all)
        delete(radioLabel);
        radioLabel = (RadioSubTitle*)0;
    }

    gCommsMgr->Cleanup();
    delete gCommsMgr;
    gCommsMgr = NULL;

#if not NEW_SOUND_STARTUP_ORDER
    ExitSoundManager();
#endif
    FalconDisplay.Cleanup();

    Camp_Exit();
    Camp_FreeMemory();
    // do this so simthread can leave
    SimulationLoopControl::StopSim();

#if NEW_SOUND_STARTUP_ORDER
    ExitSoundManager();
#endif

    UserFunctionTable.ClearTable();

#ifndef NO_TIMER_THREAD
    endTimer();
#endif

    ExitVU();
    UnloadClassTable();
    FreeTactics();

    for (int i = 0; i < numZips; i++)
    {
        ResDetach(resourceHandle[i]);
    }

    delete [] resourceHandle;
    ResExit();

    SimDriver.ReleaseSimMemoryPools();

    // Make sure there is no keytrapping
    CtrlAltDelMask(FALSE);
}


void CampaignAutoSave(FalconGameType gametype)
{
    if ( not tactical_is_training())
    {
        gCommsMgr->SaveStats();

        if (FalconLocalGame->IsLocal())
        {
            TheCampaign.SetCreationIter(TheCampaign.GetCreationIter() + 1);
            TheCampaign.SaveCampaign(gametype, gUI_AutoSaveName, 0);

            if (gCommsMgr->Online())
            {
                // Send messages to remote players with new Iter Number
                // So they can save their stats bitand update Iter in their campaign
                gCommsMgr->UpdateGameIter();
            }
        }
    }
}


LRESULT CALLBACK FalconMessageHandler(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT retval = 0;
    static int InTimer = 0;

    // Looking for multiplayer stomp...
    ShiAssert(TeamInfo[1] == NULL or TeamInfo[1]->cteam not_eq 0xFC);
    ShiAssert(TeamInfo[2] == NULL or TeamInfo[2]->cteam not_eq 0xFC);

#ifdef _MSC_VER
    // Set the FPU to Truncate
    _controlfp(_RC_CHOP, MCW_RC);

    // Set the FPU to 24bit precision
    _controlfp(_PC_24, MCW_PC);
#endif

    switch (message)
    {
#ifdef NDEBUG

        case WM_NCACTIVATE:
            if (FalconDisplay.displayFullScreen)
            {
                if ( not wParam)
                    return 0L;
            }

            break;
#endif

            // Scott OR someone competent needs to trap for surface lost
            // until then UI is only thing that can handle surface lost
        case WM_ACTIVATEAPP:
        case WM_ACTIVATE:
            if (doUI and FalconDisplay.displayFullScreen)
            {
                RECT rect;

                // restore surfaces
                if (gMainHandler)
                {
                    GetWindowRect(FalconDisplay.appWin, &rect);
                    InvalidateRect(FalconDisplay.appWin, &rect, FALSE);
                }
            }

            break;

        case WM_KILLFOCUS:
            if (KeepFocus and FalconDisplay.displayFullScreen)
                PostMessage(hwnd, FM_GIVE_FOCUS, 0, 0);

            break;

        case WM_CREATE :
            PostMessage(hwnd, FM_START_GAME, 0, 0);
            break;

        case FM_STP_START_RENDER:
            //this allows the UI to refresh the controls BEFORE it enters this function
            Sleep(100);
            STPRender((C_Base *)lParam);
            break;

        case FM_UPDATE_RULES:
            //UpdateRules();
            break;

        case FM_START_GAME:
            SystemLevelInit();

            if (intro_movie)
                SendMessage(hwnd, FM_PLAY_INTRO_MOVIE, 0, 0); // Play Movie

            PostMessage(hwnd, FM_START_UI, 0, 0); // Start UI

            break;

        case FM_START_UI:
            KeepFocus = 0;
            TheCampaign.Suspend();

            if (wParam)
                g_theaters.DoSoundSetup();

            FalconLocalSession->SetFlyState(FLYSTATE_IN_UI);
#ifdef DEBUG
            gPlayerPilotLock = 0;
#endif
            doUI = TRUE;

            UI_Startup();
            TheCampaign.Resume();
            break;

        case FM_END_UI:
            //edg : as far as i can tell this is never called
            EndUI();
            break;

        case FM_UI_UPDATE_GAMELIST: // I Use this to update my trees which display who is playing
            UI_UpdateGameList();
            break;

        case FM_REFRESH_DOGFIGHT:
            CopyDFSettingsToWindow();
            break;

        case FM_REFRESH_TACTICAL:
            if (gMainHandler not_eq NULL)
            {
                UpdateMissionWindow(TAC_AIRCRAFT);
                CheckCampaignFlyButton();
            }

            break;

        case FM_REFRESH_CAMPAIGN:
            if (gMainHandler not_eq NULL)
            {
                UpdateMissionWindow(CB_MISSION_SCREEN);
                CheckCampaignFlyButton();
            }

            break;

        case FM_TIMER_UPDATE:
            if (gMainHandler not_eq NULL)
            {
                if (InTimer)
                    break;

                InTimer = 1;
                PlayThatFunkyMusicWhiteBoy();
                UI_UpdateVU();

                if (gCommsMgr)
                    RebuildGameTree();

                gMainHandler->ProcessUserCallbacks();
                InTimer = 0;
            }

            break;

        case FM_BOOT_PLAYER:
            switch (wParam)
            {
                case game_Dogfight:
                    LeaveDogfight();
                    break;
            }

            break;

        case FM_TACREF_BUTTON_HANDLER:
            break;

            // =========================================================
            // KCK: These are used for loading/joining/ending a campaign
            // and are called under all four game sections.
            // =========================================================

        case FM_LOAD_CAMPAIGN:
            
            // Load a campaign here (this should allow tactical engagements too, so we
            // So we can eliminate the LOAD_TACTICAL case.
            if (
                (FalconGameType)lParam not_eq game_Campaign and 
                (FalconGameType)lParam not_eq game_TacticalEngagement
            )
            {
                strcpy(gUI_CampaignFile, "Instant");
            }

            retval = TheCampaign.LoadCampaign((FalconGameType)lParam, gUI_CampaignFile);

            // Notify UI of our success
            if (retval)
            {
                PostMessage(FalconDisplay.appWin, FM_JOIN_SUCCEEDED, 0, 0);
            }
            else
            {
                PostMessage(FalconDisplay.appWin, FM_JOIN_FAILED, 0, 0);
            }
            

            break;

        case FM_REVERT_CAMPAIGN:
        {
            int gametype = FalconLocalGame->GetGameType();

            // Game aborted - reload current campaign
            strcpy(gUI_CampaignFile, TheCampaign.SaveFile);
            SendMessage(hwnd, FM_SHUTDOWN_CAMPAIGN, 0, 0);

            // KCK: This is well and truely stupid
            if (gametype == game_Campaign)
            {
                StartCampaignGame(1, gametype);
            }
            else if (gametype == game_TacticalEngagement)
            {
                tactical_restart_mission();
            }

            break;
        }

        case FM_AUTOSAVE_CAMPAIGN:
            CampaignAutoSave((FalconGameType)lParam);
            break;

        case FM_JOIN_CAMPAIGN:
			// Join a campaign here
			if (gCommsMgr)
			{
				FalconGameEntity *game = (FalconGameEntity*)gCommsMgr->GetTargetGame();

				if ( not game or (VuGameEntity*)game == vuPlayerPoolGroup)
				{
					MonoPrint("Campaign Join Error: Not a valid game.\n");
					PostMessage(FalconDisplay.appWin, FM_JOIN_FAILED, 0, 0);
					return 0;
				}

				// wParam determines phase of loading we'd like to perform:
				switch (wParam)
				{
					case JOIN_PRELOAD_ONLY: // Preload only
						MonoPrint("Requesting campaign preload.\n");
						retval = TheCampaign.RequestScenarioStats(game);
						break;

					case JOIN_REQUEST_ALL_DATA: // Request all game data
						MonoPrint("Requesting all campaign data.\n");
						retval = TheCampaign.RequestScenarioStats(game);
						break;

					case JOIN_CAMP_DATA_ONLY: // Request only non-preload data (Called by Campaign only)
						MonoPrint("Requesting campaign data.\n");
						retval = TheCampaign.JoinCampaign((FalconGameType)lParam, game);
						break;
				}
			}

			if ( not retval)
				PostMessage(FalconDisplay.appWin, FM_JOIN_FAILED, 0, 0);

            break;

        case FM_JOIN_SUCCEEDED:
            MonoPrint("Starting %s game.\n", (wParam) ? "remote" : "local");
            CampaignJoinSuccess();

            if ( not gMainHandler)
                SendMessage(hwnd, FM_START_UI, 0, 0);

            break;

        case FM_JOIN_FAILED:
            // Theoretically, the error code should be in wParam
            // PETER TODO: Pop up a dialog explaining reason for failure
            MonoPrint("Failed to join game.\n");
            CampaignJoinFail();
            break;

        case FM_GAME_FULL:
            MonoPrint("Game Is Full\n");
            DisplayJoinStatusWindow(CAMP_GAME_FULL);
            CampaignJoinFail();
            break;

        case FM_MATCH_IN_PROGRESS:
            MonoPrint("Match Play game in progress\n");
            GameHasStarted();
            CampaignJoinFail();
            break;

        case FM_ONLINE_STATUS:
            if ( not doUI)
                break;

            if ( not gMainHandler)
                break;

            UI_CommsErrorMessage(static_cast<WORD>(wParam));
            break;

        case FM_SHUTDOWN_CAMPAIGN:
            // Remove any connection callbacks we might have had running
            ShutdownCampaign();
            break;

            // ==========================================================
            // KCK: These are for sim entry/exit from the varios sections
            // ==========================================================

        case FM_START_INSTANTACTION:
            // Mark us as loading
            FalconLocalSession->SetFlyState(FLYSTATE_LOADING);

            instant_action::set_campaign_time();
            instant_action::move_player_flight();
            instant_action::create_wave();

            MonoPrint("Starting.. %d\n", vuxRealTime);

            SimulationLoopControl::StartGraphics();
            EndUI();
            KeepFocus = 1;
            break;

        case FM_END_INSTANTACTION:
            break;

        case FM_START_DOGFIGHT:
            // Mark us as loading
            FalconLocalSession->SetFlyState(FLYSTATE_LOADING);
            SimulationLoopControl::StartGraphics();
            EndUI();
            KeepFocus = 1;

            break;

        case FM_START_CAMPAIGN:
            // Mark us as loading
            FalconLocalSession->SetFlyState(FLYSTATE_LOADING);
            SimulationLoopControl::StartGraphics();
            EndUI();
            KeepFocus = 1;

            break;

        case FM_START_TACTICAL:
            // Mark us as loading
            FalconLocalSession->SetFlyState(FLYSTATE_LOADING);
            SimulationLoopControl::StartGraphics();
            EndUI();
            KeepFocus = 1;

            break;

        case FM_GOT_CAMPAIGN_DATA:
            switch (wParam)
            {
                    // KCK: This is the data we just got - In case Peter wants to check off the
                    // data in some sort of "Getting campaign data" dialog or otherwise do something
                    // with it.
                case CAMP_NEED_PRELOAD:
                    MonoPrint("Got Scenario Stats.\n");
                    gCampJoinTries = 0;

                    if (FalconLocalGame)
                        CampaignPreloadSuccess( not FalconLocalGame->IsLocal());

                    if (gMainHandler) // Removed GameType check - RH
                        RecieveScenarioInfo();

                    SetCursor(gCursors[CRSR_F16]);
                    break;

                case CAMP_NEED_ENTITIES:
                    if ( not FalconLocalGame or vuPlayerPoolGroup == vuLocalGame)
                        break;

                    gCampJoinLastData = vuxRealTime;
                    MonoPrint("Got Entities.\n");
                    gCampJoinTries = 0;
                    TheCampaign.GotJoinData();
                    DisplayJoinStatusWindow(wParam);
                    break;

                case CAMP_NEED_WEATHER:
                    if ( not FalconLocalGame or  vuPlayerPoolGroup == vuLocalGame)
                        break;

                    gCampJoinLastData = vuxRealTime;
                    MonoPrint("Got weather.\n");
                    gCampJoinTries = 0;
                    TheCampaign.GotJoinData();
                    DisplayJoinStatusWindow(wParam);
                    break;

                case CAMP_NEED_PERSIST:
                    if ( not FalconLocalGame or  vuPlayerPoolGroup == vuLocalGame)
                        break;

                    gCampJoinLastData = vuxRealTime;
                    MonoPrint("Got persistant lists.\n");
                    gCampJoinTries = 0;
                    TheCampaign.GotJoinData();
                    DisplayJoinStatusWindow(wParam);
                    break;

                case CAMP_NEED_OBJ_DELTAS:
                    if ( not FalconLocalGame or vuPlayerPoolGroup == vuLocalGame)
                        break;

                    gCampJoinLastData = vuxRealTime;
                    gCampJoinTries = 0;
                    MonoPrint("Got objective data.\n");
                    TheCampaign.GotJoinData();
                    DisplayJoinStatusWindow(wParam);
                    break;

                case CAMP_NEED_TEAM_DATA:
                    if ( not FalconLocalGame or  vuPlayerPoolGroup == vuLocalGame)
                        break;

                    gCampJoinLastData = vuxRealTime;
                    gCampJoinTries = 0;
                    MonoPrint("Got team data.\n");
                    TheCampaign.GotJoinData();
                    DisplayJoinStatusWindow(wParam);
                    break;

                case CAMP_NEED_UNIT_DATA:
                    if ( not FalconLocalGame or  vuPlayerPoolGroup == vuLocalGame)
                        break;

                    gCampJoinLastData = vuxRealTime;
                    MonoPrint("Got unit data.\n");
                    gCampJoinTries = 0;
                    TheCampaign.GotJoinData();
                    DisplayJoinStatusWindow(wParam);
                    break;

                case CAMP_NEED_VC:
                    if ( not FalconLocalGame or  vuPlayerPoolGroup == vuLocalGame)
                        break;

                    gCampJoinLastData = vuxRealTime;
                    MonoPrint("Got VC data.\n");
                    gCampJoinTries = 0;
                    TheCampaign.GotJoinData();
                    DisplayJoinStatusWindow(wParam);
                    break;

                case CAMP_NEED_PRIORITIES:
                    if ( not FalconLocalGame or  vuPlayerPoolGroup == vuLocalGame)
                        break;

                    gCampJoinLastData = vuxRealTime;
                    MonoPrint("Got Priorities data.\n");
                    gCampJoinTries = 0;
                    TheCampaign.GotJoinData();
                    DisplayJoinStatusWindow(wParam);
                    break;
            }

            break;

            // =========================================================
            // KCK: Campaign triggered events
            // =========================================================

        case FM_CAMPAIGN_OVER:

            // This is called when the campaign is over (endgame triggered)
            // wParam is the win result (win/lose/draw)
            // lParam is TRUE if this is our first attempt at calling this
            if (lParam)
                LogBook.FinishCampaign(static_cast<short>(wParam));

            // KCK: If UI is running, pause the Campaign
            if (gMainHandler)
                SetTimeCompression(0);

            TheCampaign.EndgameResult = static_cast<uchar>(wParam);
            // KCK: We should make sure the fly button and time compression check
            // "TheCampaign.EndgameResult". If it's non zero, the player shouldn't be
            // able to perform these actions.
            break;

        case FM_OPEN_GAME_OVER_WIN:
            switch (wParam)
            {
                case game_InstantAction:
                    break;

                case game_Dogfight:
                    break;

                case game_Campaign:
                    break;

                case game_TacticalEngagement:
                    OpenTEGameOverWindow();
                    break;
            }

            break;

        case FM_CAMPAIGN_EVENT:
            // Currently unused
            break;

        case FM_ATTACK_WARNING:
            // Basically let the UI know to let the player know we're about to be attacked
            // and what interceptors to take (if any)
            UIScramblePlayerFlight();
            break;

        case FM_AIRBASE_ATTACK:
            CampEventSoundID = 500005;
            break;

        case FM_AIRBASE_DISABLED:
            // NOTE: this will be accompanied by a squadron rebase/recall.
            UI_HandleAirbaseDestroyed();
            break;

        case FM_SQUADRON_RECALLED:
            MonoPrint("Player squadron recalled\n");
            break;

        case FM_SQUADRON_REBASED:
            RelocateSquadron();
            break;

        case FM_REFRESH_CAMPMAP:
            UI_UpdateOccupationMap();
            break;

        case FM_REBUILD_WP_LIST:
            RebuildCurrentWPList();
            break;

        case FM_PLAYER_FLIGHT_CANCELED:
            UI_HandleFlightCancel();
            break;

        case FM_PLAYER_AIRCRAFT_DESTROYED:
            // PETER TODO:
            // Post message saying player aircraft destroyed
            // (while waiting for takeoff) and go back to mission screen
            UI_HandleAircraftDestroyed();
            break;

        case FM_RECEIVE_CHAT:
            ProcessChatStr((CHATSTR*)lParam);
            break;


        case FM_PLAY_UI_MOVIE:
            if (gMainHandler and ReadyToPlayMovie)
                PlayUIMovieQ();

            break;

        case FM_REPLAY_UI_MOVIE:
            if (gMainHandler and ReadyToPlayMovie)
                ReplayUIMovie(lParam);

            break;

        case FM_REMOTE_LOGBOOK:
            if (gMainHandler and gCommsMgr)
                ViewRemoteLogbook(lParam);

            break;

        case FM_PLAY_INTRO_MOVIE:
            FalconDisplay.EnterMode(FalconDisplayConfiguration::Movie);
            SetFocus(hwnd);

            // RV - Biker - Add theater switching for into movie
            char tmpPath[MAX_PATH];
            sprintf(tmpPath, "%s\\intro.avi", FalconMovieDirectory);
            PlayMovie(tmpPath, -1, -1, 0, 0, FalconDisplay.GetImageBuffer()->frontSurface());
            FalconDisplay.LeaveMode();
            break;

        case FM_EXIT_GAME:
            EndUI();

            PostQuitMessage(0);
            retval = 0;

            break;

        case FM_GIVE_FOCUS:
            SetActiveWindow(FalconDisplay.appWin);
            SetFocus(FalconDisplay.appWin);
            break;

        case WM_DESTROY :
            break;

        case WM_USER:
            break;

        case FM_DISP_ENTER_MODE:
        {
            FalconDisplay._EnterMode((FalconDisplayConfiguration::DisplayMode) wParam, LOWORD(lParam), HIWORD(lParam));
            break;
        }

        case FM_DISP_LEAVE_MODE:
        {
            FalconDisplay._LeaveMode();
            break;
        }

        case FM_DISP_TOGGLE_FULLSCREEN:
        {
            FalconDisplay._ToggleFullScreen();
            break;
        }

        default:
        {
            if (gMainHandler not_eq NULL)
            {
                if (gMainHandler->EventHandler(hwnd, message, wParam, lParam))
                {
                    retval = DefWindowProc(hwnd, message, wParam, lParam);
                }
                else
                {
                    retval = 0;
                }
            }
            else
            {
                // sfr: touchbhuddy support
                // we let the message pass through to be processed by other handlers
                static bool mouseIn = false;

                switch (message)
                {
                        // indicates mouse inside area FreeFalcon window area
                    case WM_MOUSELEAVE:
                    {
                        mouseIn = false;
                        SimMouseStopProcessing();
                    }
                    break;

                    case WM_MOUSEMOVE:
                    {
                        // we want to be notified when mouse leaves again
                        if ( not mouseIn)
                        {
                            // sfr: track mouseleave
                            TRACKMOUSEEVENT tme;
                            tme.cbSize = sizeof(tme);
                            tme.dwFlags = TME_LEAVE;
                            tme.hwndTrack = hwnd;
                            //TrackMouseEvent(&tme);
                            // mouse cursor position
                            SimMouseResumeProcessing(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
                            mouseIn = true;
                        }
                    }
                    break;
                }

                // process event
                retval = DefWindowProc(hwnd, message, wParam, lParam);
            }
        }
        // end default
        break;
    }


    // Looking for multiplayer stomp...
    ShiAssert(TeamInfo[1] == NULL or TeamInfo[1]->cteam not_eq 0xFC);
    ShiAssert(TeamInfo[2] == NULL or TeamInfo[2]->cteam not_eq 0xFC);

    // Looking for multiplayer stomp...
    ShiAssert(TeamInfo[1] == NULL or TeamInfo[1]->cteam not_eq 0xFC);
    ShiAssert(TeamInfo[2] == NULL or TeamInfo[2]->cteam not_eq 0xFC);

    return retval;
}


void PlayMovie(char *filename, int left, int top, int w, int h, void *theSurface)
{
    HWND hwnd;
    int hMovie = -1, mode;
    char movieFile[_MAX_PATH];
    MSG               msg;
    int stopMovie = FALSE;
    RECT theRect;
    POINT pt;

    hwnd = FalconDisplay.appWin;
    // RV - Biker - Path is in filename already
    sprintf(movieFile, "%s", filename);
    //sprintf(movieFile, "%s\\%s", FalconMovieDirectory, filename);

    if (left == -1)
    {
        GetClientRect(hwnd, &theRect);
        pt.x = 0;
        pt.y = 0;
        ClientToScreen(hwnd, &pt);
        OffsetRect(&theRect, pt.x, pt.y);
        left = theRect.left;
        top = theRect.top;
        mode = MOVIE_MODE_INTERLACE;

        if ( not stricmp(FalconMovieMode, "Hurry"))
        {
            mode or_eq MOVIE_MODE_HURRY;
        }
    }
    else
    {
        pt.x = left;
        pt.y = top;
        ClientToScreen(hwnd, &pt);
        SetRect(&theRect, left, top, left + w, left + h);
        left = pt.x;
        top = pt.y;
        mode = MOVIE_MODE_NORMAL;
    }

    F4SilenceVoices();

    MonoPrint("stopMovie = %d.\n", stopMovie);

    ShowCursor(FALSE);

    while (TRUE)
    {
        if (PeekMessage(&msg, NULL, 0 , 0, PM_NOREMOVE))
        {
            if ( not GetMessage(&msg, NULL, 0, 0))
                break;

            if (msg.message == WM_KEYUP) // any key press will stop the movie
                stopMovie = TRUE;
            else if (msg.message == WM_LBUTTONUP) // lmouse click stops it,too
                stopMovie = TRUE;


            if (msg.message not_eq WM_ACTIVATEAPP)
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            else
            {
                Sleep(1);
            }
        }

        Sleep(1);

        if (hMovie == -1)
        {

            MonoPrint("theSurface %x\n", theSurface);
            MonoPrint("movieFile  %x\n", movieFile);


            hMovie =  movieOpen(movieFile, NULL, (LPVOID) theSurface, 0, NULL,
                                left, top, mode, MOVIE_USE_AUDIO);

            MonoPrint("movieOpen() returned %d\n", hMovie);
            MonoPrint("stopMovie = %d.\n", stopMovie);

            if (hMovie < 0)
            {
                MonoPrint("Move file %s not found.\n", movieFile);
                break;
            }

            if (movieStart(hMovie) not_eq MOVIE_OK)
            {
                MonoPrint("error with movieStart.\n");
                movieClose(hMovie);
                break;
            }
        }

        if (stopMovie or not movieIsPlaying(hMovie))
        {
            MonoPrint("Premature movie exit.\n");
            movieClose(hMovie);
            InvalidateRect(hwnd, &theRect, FALSE);
            break;
        }
    } // end while (1)

    ShowCursor(TRUE);
    F4HearVoices();
}


void ShutdownCampaign(void)
{
    if (gMainHandler)
        gMainHandler->RemoveUserCallback(CampaignConnectionTimer);

    // Shutdown campaign stuff here
    TheCampaign.EndCampaign();
#if CAMPTOOL

    if (hMainWnd)
        PostMessage(hMainWnd, WM_CLOSE, 0, 0);

#endif
    SetTimeCompression(0);
    DisableCampaignMenus();
    gCampJoinStatus = 0;
}


void EnableCampaignMenus(void)
{
    EnableMenuItem(GetMenu(mainMenuWnd), ID_CAMPAIGN_SAVEAS, MF_ENABLED);
    EnableMenuItem(GetMenu(mainMenuWnd), ID_CAMPAIGN_SAVEALLAS, MF_ENABLED);
    EnableMenuItem(GetMenu(mainMenuWnd), ID_CAMPAIGN_SAVEINSTANTAS, MF_ENABLED);
    EnableMenuItem(GetMenu(mainMenuWnd), ID_CAMPAIGN_LOAD, MF_GRAYED);
    EnableMenuItem(GetMenu(mainMenuWnd), ID_CAMPAIGN_NEW, MF_GRAYED);
    EnableMenuItem(GetMenu(mainMenuWnd), ID_CAMPAIGN_JOIN, MF_GRAYED);
#ifdef CAMPTOOL
    EnableMenuItem(GetMenu(mainMenuWnd), ID_CAMPAIGN_DISPLAY, MF_ENABLED);
#endif
    EnableMenuItem(GetMenu(mainMenuWnd), ID_CAMPAIGN_PAUSED, MF_ENABLED);
    EnableMenuItem(GetMenu(mainMenuWnd), ID_CAMPAIGN_SELECTSQUADRON, MF_ENABLED);
    EnableMenuItem(GetMenu(mainMenuWnd), ID_CAMPAIGN_FLYMISSION, MF_ENABLED);
    EnableMenuItem(GetMenu(mainMenuWnd), ID_CAMPAIGN_EXIT, MF_ENABLED);
}


void DisableCampaignMenus(void)
{
    EnableMenuItem(GetMenu(mainMenuWnd), ID_CAMPAIGN_SAVE, MF_GRAYED);
    EnableMenuItem(GetMenu(mainMenuWnd), ID_CAMPAIGN_SAVEAS, MF_GRAYED);
    EnableMenuItem(GetMenu(mainMenuWnd), ID_CAMPAIGN_SAVEALLAS, MF_GRAYED);
    EnableMenuItem(GetMenu(mainMenuWnd), ID_CAMPAIGN_SAVEINSTANTAS, MF_GRAYED);
    EnableMenuItem(GetMenu(mainMenuWnd), ID_CAMPAIGN_LOAD, MF_ENABLED);
    EnableMenuItem(GetMenu(mainMenuWnd), ID_CAMPAIGN_NEW, MF_ENABLED);
    EnableMenuItem(GetMenu(mainMenuWnd), ID_CAMPAIGN_PAUSED, MF_GRAYED);
    EnableMenuItem(GetMenu(mainMenuWnd), ID_CAMPAIGN_DISPLAY, MF_GRAYED);
    EnableMenuItem(GetMenu(mainMenuWnd), ID_CAMPAIGN_FLYMISSION, MF_GRAYED);
    EnableMenuItem(GetMenu(mainMenuWnd), ID_CAMPAIGN_SELECTSQUADRON, MF_GRAYED);

    if (doNetwork)
        EnableMenuItem(GetMenu(mainMenuWnd), ID_CAMPAIGN_JOIN, MF_ENABLED);

    CheckMenuItem(GetMenu(mainMenuWnd), ID_CAMPAIGN_PAUSED, MF_CHECKED);
}


void ConsoleWrite(char* str)
{
    HANDLE hStdIn;
    DWORD num;
    LPVOID lpMsgBuf;

    hStdIn = GetStdHandle(STD_OUTPUT_HANDLE);

    if (hStdIn == INVALID_HANDLE_VALUE)
    {
        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER bitor FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            GetLastError(),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
            (LPTSTR) &lpMsgBuf,
            0,
            NULL
        );
    }

    WriteConsole(hStdIn, str, strlen(str), &num, NULL);
}


void CtrlAltDelMask(int state)
{
    int was;

    if (state)
        SystemParametersInfo(SPI_SCREENSAVERRUNNING, TRUE, &was, 0);
    else SystemParametersInfo(SPI_SCREENSAVERRUNNING, FALSE, &was, 0);
}
// END OF FUNCTION DEFINITIONS
