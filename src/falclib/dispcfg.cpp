#include "dispcfg.h"
#include "fsound.h"
#include "f4find.h"
#include "Graphics/Include/setup.h"
#include "falcuser.h"
#include "FalcLib/include/playerop.h"
#include "FalcLib/include/dispopts.h"
#include <commctrl.h>

extern bool g_bForceSoftwareGUI;
void TheaterReload(char *theater, char *loddata);

LRESULT CALLBACK FalconMessageHandler(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

FalconDisplayConfiguration FalconDisplay;

FalconDisplayConfiguration::FalconDisplayConfiguration(void)
{
    xOffset = 40;
    yOffset = 40;

    width[Movie] = 640;
    height[Movie] = 480;
    depth[Movie] = 16;
    doubleBuffer[Movie] = FALSE;

    width[UI] = 800;
    height[UI] = 600;
    depth[UI] = 16;
    doubleBuffer[UI] = FALSE;

    width[UILarge] = 1024;
    height[UILarge] = 768;
    depth[UILarge] = 16;
    doubleBuffer[UILarge] = FALSE;

    width[Planner] = 800;
    height[Planner] = 600;
    depth[Planner] = 16;
    doubleBuffer[Planner] = FALSE;

    width[Layout] = 1024;
    height[Layout] = 768;
    depth[Layout] = 16;
    doubleBuffer[Layout] = FALSE;

    //default values
    width[Sim] = 1920;	// 3D default -- Full HD (was 640x480; overridden by SetSimMode from DispWidth)
    height[Sim] = 1080;
    depth[Sim] = 16;
    doubleBuffer[Sim] = TRUE;

    deviceNumber = 0;
#ifdef DEBUG
    char strName[40];
    DWORD dwSize = sizeof(strName);
    GetComputerName(strName, &dwSize);
#endif
}

FalconDisplayConfiguration::~FalconDisplayConfiguration(void)
{
}

void FalconDisplayConfiguration::Setup(int languageNum)
{
    WNDCLASS wc;

    // Setup the graphics databases - M.N. changed to Falcon3DDataDir for theater switching
    DeviceIndependentGraphicsSetup(FalconTerrainDataDir, Falcon3DDataDir, FalconMiscTexDataDir);

    // set up and register window class
    wc.style = CS_HREDRAW bitor CS_VREDRAW bitor CS_OWNDC bitor CS_NOCLOSE;
    wc.lpfnWndProc = FalconMessageHandler;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = sizeof(DWORD);
    wc.hInstance = NULL;
    //   wc.hIcon = NULL;
    wc.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(105)); // OW BC
    wc.hCursor = NULL;
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = "FalconDisplay";

    // Register this class.
    RegisterClass(&wc);
#if 0

    // Choose an appropriate window style
    if (displayFullScreen)
    {
        xOffset = 0;
        yOffset = 0;
        windowStyle = WS_POPUP;
    }
    else
    {
        windowStyle = WS_OVERLAPPEDWINDOW;
    }

    // Build a window for this application
    rect.top = rect.left = 0;
    rect.right = width[Movie];
    rect.bottom = height[Movie];
    AdjustWindowRect(&rect, windowStyle, FALSE);
    appWin = CreateWindow(
                 "FalconDisplay", /* class */
                 "3D Output", /* caption */
                 windowStyle, /* style */
                 50, /* init. x pos */
                 50, /* init. y pos */
                 rect.right - rect.left, /* init. x size */
                 rect.bottom - rect.top, /* init. y size */
                 NULL, /* parent window */
                 NULL, /* menu handle */
                 NULL, /* program handle */
                 NULL /* create parms */
             );

    if ( not appWin)
    {
        ShiError("Failed to construct main window");
    }

    // Display the new rendering window
    ShowWindow(appWin, SW_SHOW);
#endif
    MakeWindow();
    // Set up the display device manager
    devmgr.Setup(languageNum);
    //   TheaterReload(FalconTerrainDataDir); // JPO test if this works.
}

void FalconDisplayConfiguration::Cleanup(void)
{
    devmgr.Cleanup();

    DeviceIndependentGraphicsCleanup();
}

void FalconDisplayConfiguration::MakeWindow(void)
{
    RECT rect;

    // Choose an appropriate window style
    if (displayFullScreen)
    {
        xOffset = 0;
        yOffset = 0;
        windowStyle = WS_POPUP;
    }
    else
    {
        windowStyle = WS_OVERLAPPEDWINDOW;
        xOffset = 0;
        yOffset = 0;
    }

    // Build a window for this application
    rect.top = rect.left = 0;
    rect.right = width[Movie];
    rect.bottom = height[Movie];
    AdjustWindowRect(&rect, windowStyle, FALSE);
	extern const char* FREE_FALCON_BRAND;
    appWin = CreateWindow(
                 "FalconDisplay", /* class */
				 FREE_FALCON_BRAND, /* caption */
                 windowStyle, /* style */
                 xOffset, /* init. x pos */
                 yOffset, /* init. y pos */
                 rect.right - rect.left, /* init. x size */
                 rect.bottom - rect.top, /* init. y size */
                 NULL, /* parent window */
                 NULL, /* menu handle */
                 NULL, /* program handle */
                 NULL /* create parms */
             );

    if ( not appWin)
    {
        ShiError("Failed to construct main window");
    }

    // sfr: track mouseleave
    TRACKMOUSEEVENT tme;
    tme.cbSize = sizeof(tme);
    tme.dwFlags = TME_LEAVE;
    tme.hwndTrack = appWin;
    //if ( not TrackMouseEvent(&tme)) {
    // ShiError( "Failed to track mouseleave");
    //}


    UpdateWindow(appWin);
    SetFocus(appWin);

    // Display the new rendering window
    ShowWindow(appWin, SW_SHOW);
}

// OW
#define _FORCE_MAIN_THREAD

#ifdef _FORCE_MAIN_THREAD
void FalconDisplayConfiguration::EnterMode(DisplayMode newMode, int theDevice, int Driver)
{
    // Force exectution in the main thread to avoid problems with worker threads setting directx cooperative levels (which is illegal)
    LRESULT result = SendMessage(appWin, FM_DISP_ENTER_MODE, newMode, theDevice bitor (Driver << 16));
}

void FalconDisplayConfiguration::_EnterMode(DisplayMode newMode, int theDevice, int Driver)
#else
void FalconDisplayConfiguration::_EnterMode(DisplayMode newMode, int theDevice, int Driver)
{
}

void FalconDisplayConfiguration::EnterMode(DisplayMode newMode, int theDevice, int Driver)
#endif
{
    RECT rect;

#ifdef _FORCE_MAIN_THREAD
    ShiAssert(::GetCurrentThreadId() == GetWindowThreadProcessId(appWin, NULL)); // Make sure this is called by the main thread
#endif

    // sfr: only after we are finished
    //currentMode = newMode;

    rect.top = rect.left = 0;
    rect.right = width[newMode];
    rect.bottom = height[newMode];
    AdjustWindowRect(&rect, windowStyle, FALSE);

    DeviceManager::DDDriverInfo *pDI = FalconDisplay.devmgr.GetDriver(Driver);

    // RV - RED - Sim window in windowed mode, always centered
    if (newMode == Sim and not displayFullScreen)
    {

        int wx = GetSystemMetrics(SM_CXSCREEN);
        int wy = GetSystemMetrics(SM_CYSCREEN);

        int NewXOffset = 0;
        int NewYOffset = 0;

        if ((rect.right > wx) or (rect.bottom > wy))
        {
            NewXOffset = 0;
            NewYOffset = 0;
            rect.right = wx - 2; // border
            rect.bottom = wy - 20; // Title bar + border
        }
        else
        {
            NewXOffset = (wx - (rect.right - rect.left)) / 2;
            NewYOffset = (wy - (rect.bottom - rect.top)) / 2;
        }

        SetWindowPos(appWin, NULL, NewXOffset, NewYOffset,
                     rect.right - rect.left, rect.bottom - rect.top, SWP_NOZORDER);

    }
    else
    {
        SetWindowPos(appWin, NULL, xOffset, yOffset,
                     rect.right - rect.left, rect.bottom - rect.top, SWP_NOZORDER);
    }

    if (pDI)
    {
        /*JAM 01Dec03 if((g_bForceSoftwareGUI or pDI->Is3dfx() or not pDI->CanRenderWindowed()) and newMode not_eq Sim)
         {
         // V1, V2 workaround - use primary display adapter with RGB Renderer
         int nIndexPrimary = FalconDisplay.devmgr.FindPrimaryDisplayDriver();
         ShiAssert(nIndexPrimary not_eq -1);

         if(nIndexPrimary not_eq -1)
         {
         DeviceManager::DDDriverInfo *pDI = FalconDisplay.devmgr.GetDriver(nIndexPrimary);
         int nIndexRGBRenderer = pDI->FindRGBRenderer();
         ShiAssert(nIndexRGBRenderer not_eq -1);

         if(nIndexRGBRenderer not_eq -1)
         {
         Driver = nIndexPrimary;
         theDevice = nIndexRGBRenderer;
         }
         }
         }*/

        if ( not pDI->SupportsSRT() and DisplayOptions.bRender2Texture)
            DisplayOptions.bRender2Texture = false;
    }

    theDisplayDevice.Setup(
        Driver, theDevice,
        width[newMode], height[newMode], depth[newMode],
        displayFullScreen, doubleBuffer[newMode], appWin, newMode == Sim
    );

    SetForegroundWindow(appWin);
    // sfr: here
    currentMode = newMode;

    Sleep(0);
}

#ifdef _FORCE_MAIN_THREAD
void FalconDisplayConfiguration::LeaveMode(void)
{
    // Force exectution in the main thread to avoid problems with worker threads setting directx cooperative levels (which is illegal)
    LRESULT result = SendMessage(appWin, FM_DISP_LEAVE_MODE, 0, 0);
}

void FalconDisplayConfiguration::_LeaveMode(void)
#else
void FalconDisplayConfiguration::_LeaveMode(void)
{
}

void FalconDisplayConfiguration::LeaveMode(void)
#endif
{
#ifdef _FORCE_MAIN_THREAD
    ShiAssert(::GetCurrentThreadId() == GetWindowThreadProcessId(appWin, NULL)); // Make sure this is called by the main thread
#endif

    theDisplayDevice.Cleanup();
}

void FalconDisplayConfiguration::SetSimMode(int newwidth, int newheight, int newdepth)
{
    // Artscout - 2026: guard against uninitialized/garbage dimensions. DispWidth/DispHeight can be
    // unset (e.g. an old/short options.pop leaves the field uninitialized -> 0xCCCC = 52428 in debug);
    // that propagated into width[Sim] -> a 52428x52428 swapchain/depth/MSAA on 3D entry (CreateTexture2D
    // INVALIDDIMENSIONS + "no buffers available" -> broken device). Reject out-of-range values and keep
    // the current (constructor default 1920x1080) Sim mode so the device inits at a sane size.
    if (newwidth < 1 or newwidth > 16384 or newheight < 1 or newheight > 16384)
        return;

    width[Sim] = newwidth;
    height[Sim] = newheight;
    depth[Sim] = newdepth;
}

#ifdef _FORCE_MAIN_THREAD
void FalconDisplayConfiguration::ToggleFullScreen(void)
{
    // Force exectution in the main thread to avoid problems with worker threads setting directx cooperative levels (which is illegal)
    LRESULT result = SendMessage(appWin, FM_DISP_TOGGLE_FULLSCREEN, 0, 0);
}

void FalconDisplayConfiguration::_ToggleFullScreen(void)
#else
void FalconDisplayConfiguration::_ToggleFullScreen(void)
{
}

void FalconDisplayConfiguration::ToggleFullScreen(void)
#endif
{
#ifdef _FORCE_MAIN_THREAD
    ShiAssert(::GetCurrentThreadId() == GetWindowThreadProcessId(appWin, NULL)); // Make sure this is called by the main thread
#endif

    LeaveMode();
    DestroyWindow(appWin);
	displayFullScreen ? displayFullScreen = false : displayFullScreen = true;
    MakeWindow();
    EnterMode(currentMode);
}

// #33: enter the 3D-session window mode (windowed or borderless fullscreen). The shared app
// window is restyled IN PLACE (no DestroyWindow/MakeWindow -> avoids the #41 enter/exit hang
// area); the swap chain is left untouched and DXGI stretches the back buffer to the client
// area. The previous (menu) style/rect are saved so LeaveSimWindowMode restores them exactly.
#ifdef _FORCE_MAIN_THREAD
void FalconDisplayConfiguration::EnterSimWindowMode(bool windowed)
{
    SendMessage(appWin, FM_DISP_ENTER_SIM_WINMODE, windowed ? 1 : 0, 0);
}

void FalconDisplayConfiguration::_EnterSimWindowMode(bool windowed)
#else
void FalconDisplayConfiguration::_EnterSimWindowMode(bool) {}

void FalconDisplayConfiguration::EnterSimWindowMode(bool windowed)
#endif
{
    if (mInSimWinMode or not appWin) return;

    // Save the current (menu) window state for restoration on 3D exit.
    mSavedWinStyle   = (long)GetWindowLong(appWin, GWL_STYLE);
    GetWindowRect(appWin, &mSavedWinRect);
    mSavedFullScreen = displayFullScreen;
    mInSimWinMode    = true;

    const int sw = GetSystemMetrics(SM_CXSCREEN);
    const int sh = GetSystemMetrics(SM_CYSCREEN);

    if (windowed)
    {
        // Windowed: client = chosen 3D resolution, centered and clamped to the desktop.
        RECT rect = { 0, 0, width[Sim], height[Sim] };
        AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
        int ww = rect.right - rect.left;
        int wh = rect.bottom - rect.top;
        if (ww > sw) ww = sw;
        if (wh > sh) wh = sh;
        int x = (sw - ww) / 2; if (x < 0) x = 0;
        int y = (sh - wh) / 2; if (y < 0) y = 0;
        SetWindowLong(appWin, GWL_STYLE, WS_OVERLAPPEDWINDOW);
        SetWindowPos(appWin, HWND_TOP, x, y, ww, wh, SWP_FRAMECHANGED bitor SWP_SHOWWINDOW);
        displayFullScreen = false;
    }
    else
    {
        // Borderless fullscreen: cover the whole monitor; the back buffer is stretched to fit.
        SetWindowLong(appWin, GWL_STYLE, WS_POPUP);
        SetWindowPos(appWin, HWND_TOP, 0, 0, sw, sh, SWP_FRAMECHANGED bitor SWP_SHOWWINDOW);
        displayFullScreen = true;
    }

    SetForegroundWindow(appWin);
    SetFocus(appWin);
}

#ifdef _FORCE_MAIN_THREAD
void FalconDisplayConfiguration::LeaveSimWindowMode()
{
    SendMessage(appWin, FM_DISP_LEAVE_SIM_WINMODE, 0, 0);
}

void FalconDisplayConfiguration::_LeaveSimWindowMode()
#else
void FalconDisplayConfiguration::_LeaveSimWindowMode() {}

void FalconDisplayConfiguration::LeaveSimWindowMode()
#endif
{
    if (not mInSimWinMode) return;

    if (appWin)
    {
        SetWindowLong(appWin, GWL_STYLE, mSavedWinStyle);
        SetWindowPos(appWin, HWND_TOP,
                     mSavedWinRect.left, mSavedWinRect.top,
                     mSavedWinRect.right - mSavedWinRect.left,
                     mSavedWinRect.bottom - mSavedWinRect.top,
                     SWP_FRAMECHANGED bitor SWP_SHOWWINDOW);
    }

    displayFullScreen = mSavedFullScreen;
    mInSimWinMode = false;
}
