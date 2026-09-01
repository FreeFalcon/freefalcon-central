#include "dispcfg.h"
#include "fsound.h"
#include "f4find.h"
#include "graphics/include/setup.h"
#include "falcuser.h"
#include "falclib/include/playerop.h"
#include "falclib/include/dispopts.h"
#include <commctrl.h>
#ifndef _WIN32
#include "ff_window.h" // #104: ffplatform::Window (SDL3) -- the native render window on Linux
#include "ff_events.h" // #104: ffevents::RegisterWindow/SetMainWindow/PostWindowMessage -- the message bus
#endif

extern bool g_bForceSoftwareGUI;
// #104 (Linux): set around EndUI() in the menu->sim FM_START handlers so _LeaveMode() skips tearing
// down the shared theDisplayDevice while the sim thread is re-initing it (see _LeaveMode comment).
bool g_bSkipDisplayHandoffCleanup = false;
void TheaterReload(char *theater, char *loddata);

LRESULT CALLBACK FalconMessageHandler(HWND hwnd, UINT message, WPARAM wParam,
                                      LPARAM lParam);

FalconDisplayConfiguration FalconDisplay;

FalconDisplayConfiguration::FalconDisplayConfiguration(void)
{
    xOffset = 40;
    yOffset = 40;

    // Artscout - 2026 (#104): default the small 2D modes to 1024x768 (was 640x480 Movie / 800x600 UI). The Movie
    // mode was the source of the 640x480 startup window; 1024x768 is a supported UI resolution (== UILarge) so the
    // menu layout is unaffected, and the GPU-backend swapchain comes up at a sane size instead of tiny 640x480.
    width[Movie] = 1024;
    height[Movie] = 768;
    depth[Movie] = 16;
    doubleBuffer[Movie] = FALSE;

    width[UI] = 1024;
    height[UI] = 768;
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
    width[Sim] =
        1920; // 3D default -- Full HD (was 640x480; overridden by SetSimMode from DispWidth)
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
    // Setup the graphics databases - M.N. changed to Falcon3DDataDir for theater switching
    DeviceIndependentGraphicsSetup(FalconTerrainDataDir, Falcon3DDataDir,
                                   FalconMiscTexDataDir);

#ifdef _WIN32
    // set up and register window class (Linux: the window is created by ffplatform/SDL3 in MakeWindow)
    WNDCLASS wc;
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
#endif // _WIN32 (window class registration)
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
#ifdef _WIN32
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
    extern const char *FREE_FALCON_BRAND;
    appWin = CreateWindow("FalconDisplay", /* class */
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

    if (not appWin)
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
#else
    // Linux (#104, subsystem 1 part 3): create the native SDL3 window (the Win32 CreateWindow path above is
    // #ifdef'd out). appWin carries the ffplatform::Window* as its HWND token -- the SAME token that flows
    // EnterMode -> DeviceManager::CreateContext -> DXContext::Init -> VulkanBackend::Init, which pulls the
    // SDL_Window* back out (via Window::GetSdlWindow) to build the Vulkan surface. The window is created
    // Vulkan-capable (ff_window.cpp adds SDL_WINDOW_VULKAN on Linux).
    extern const char *FREE_FALCON_BRAND;

    ffplatform::WindowDesc desc;
    desc.title = FREE_FALCON_BRAND;
    desc.width = width[UI] > 0 ? width[UI] : 1024;
    desc.height = height[UI] > 0 ? height[UI] : 768;
    desc.fullscreen = false;
    desc.resizable = false;

    ffplatform::Window *win = ffplatform::Window::Create(desc);

    if (not win)
        ShiError("Failed to construct main window (SDL3)");

    appWin = reinterpret_cast<HWND>(win);
    win->CenterOnDisplay();
    win->Show();

    // Wire the window into the SDL event bus: translated OS events and the internal FM_* command bus (routed here
    // by the win32shim PostMessage/SendMessage bridge) now reach FalconMessageHandler -- the same proc the Win32
    // "FalconDisplay" class used. Then kick the boot chain: Win32 delivers WM_CREATE synchronously from
    // CreateWindow (-> FM_START_GAME -> SystemLevelInit -> FM_START_UI -> EnterMode); SDL does not, so enqueue it.
    ffevents::RegisterWindow(win->GetSdlWindow(), appWin, FalconMessageHandler);
    ffevents::SetMainWindow(appWin);
    ffevents::PostWindowMessage(appWin, WM_CREATE, 0, 0);
#endif
}

// OW
#define _FORCE_MAIN_THREAD

#ifdef _FORCE_MAIN_THREAD
void FalconDisplayConfiguration::EnterMode(DisplayMode newMode, int theDevice,
                                           int Driver)
{
    // Force exectution in the main thread to avoid problems with worker threads setting directx cooperative levels (which is illegal)
    LRESULT result = SendMessage(appWin, FM_DISP_ENTER_MODE, newMode,
                                 theDevice bitor (Driver << 16));
}

void FalconDisplayConfiguration::_EnterMode(DisplayMode newMode, int theDevice,
                                            int Driver)
#else
void FalconDisplayConfiguration::_EnterMode(DisplayMode newMode, int theDevice,
                                            int Driver)
{
}

void FalconDisplayConfiguration::EnterMode(DisplayMode newMode, int theDevice,
                                           int Driver)
#endif
{
    RECT rect;

#ifdef _FORCE_MAIN_THREAD
    ShiAssert(::GetCurrentThreadId() ==
              GetWindowThreadProcessId(
                  appWin, NULL)); // Make sure this is called by the main thread
#endif

    // sfr: only after we are finished
    //currentMode = newMode;

#ifdef _WIN32
    rect.top = rect.left = 0;
    rect.right = width[newMode];
    rect.bottom = height[newMode];
    AdjustWindowRect(&rect, windowStyle, FALSE);
#endif

    DeviceManager::DDDriverInfo *pDI = FalconDisplay.devmgr.GetDriver(Driver);

#ifdef _WIN32
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
                     rect.right - rect.left, rect.bottom - rect.top,
                     SWP_NOZORDER);
    }
    else
    {
        SetWindowPos(appWin, NULL, xOffset, yOffset, rect.right - rect.left,
                     rect.bottom - rect.top, SWP_NOZORDER);
    }
#endif // _WIN32 (window sizing/positioning; SDL manages the window on Linux)

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

        if (not pDI->SupportsSRT() and DisplayOptions.bRender2Texture)
            DisplayOptions.bRender2Texture = false;
    }

    theDisplayDevice.Setup(Driver, theDevice, width[newMode], height[newMode],
                           depth[newMode], displayFullScreen,
                           doubleBuffer[newMode], appWin, newMode == Sim);

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
    ShiAssert(::GetCurrentThreadId() ==
              GetWindowThreadProcessId(
                  appWin, NULL)); // Make sure this is called by the main thread
#endif

    // #104 (Linux): during a menu->sim handoff the sim's graphics thread has already (or is about to)
    // EnterMode(Sim) and re-init the SHARED theDisplayDevice. On Windows the sim's FM_DISP_ENTER_MODE
    // marshals to the main thread and queues behind this LeaveMode, so the order is Cleanup-then-Setup.
    // On Linux FF_SendWindowMessage runs inline on the caller thread, so this menu Cleanup would race
    // the sim thread and NULL the freshly-created m_DXCtx mid-VCock_Init ("Failed to setup rendering
    // context"). Skip the teardown during the handoff -- the sim owns/re-inits the device.
    extern bool g_bSkipDisplayHandoffCleanup;
    if (g_bSkipDisplayHandoffCleanup)
        return;

    theDisplayDevice.Cleanup();
}

void FalconDisplayConfiguration::SetSimMode(int newwidth, int newheight,
                                            int newdepth)
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
#if defined(_FORCE_MAIN_THREAD) && defined(_WIN32)
    ShiAssert(::GetCurrentThreadId() ==
              GetWindowThreadProcessId(
                  appWin, NULL)); // Make sure this is called by the main thread
#endif

    LeaveMode();
#ifdef _WIN32
    DestroyWindow(
        appWin); // Linux: the window is owned by ffplatform/SDL, not recreated here
#endif
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
void FalconDisplayConfiguration::_EnterSimWindowMode(bool)
{
}

void FalconDisplayConfiguration::EnterSimWindowMode(bool windowed)
#endif
{
    if (mInSimWinMode or not appWin)
        return;
#ifdef _WIN32
    // Save the current (menu) window state for restoration on 3D exit.
    mSavedWinStyle = (long)GetWindowLong(appWin, GWL_STYLE);
    GetWindowRect(appWin, &mSavedWinRect);
    mSavedFullScreen = displayFullScreen;
    mInSimWinMode = true;

    const int sw = GetSystemMetrics(SM_CXSCREEN);
    const int sh = GetSystemMetrics(SM_CYSCREEN);

    if (windowed)
    {
        // Windowed: client = chosen 3D resolution, centered and clamped to the desktop.
        RECT rect = {0, 0, width[Sim], height[Sim]};
        AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
        int ww = rect.right - rect.left;
        int wh = rect.bottom - rect.top;
        if (ww > sw)
            ww = sw;
        if (wh > sh)
            wh = sh;
        int x = (sw - ww) / 2;
        if (x < 0)
            x = 0;
        int y = (sh - wh) / 2;
        if (y < 0)
            y = 0;
        SetWindowLong(appWin, GWL_STYLE, WS_OVERLAPPEDWINDOW);
        SetWindowPos(appWin, HWND_TOP, x, y, ww, wh,
                     SWP_FRAMECHANGED bitor SWP_SHOWWINDOW);
        displayFullScreen = false;
    }
    else
    {
        // Borderless fullscreen: cover the whole monitor; the back buffer is stretched to fit.
        SetWindowLong(appWin, GWL_STYLE, WS_POPUP);
        SetWindowPos(appWin, HWND_TOP, 0, 0, sw, sh,
                     SWP_FRAMECHANGED bitor SWP_SHOWWINDOW);
        displayFullScreen = true;
    }

    SetForegroundWindow(appWin);
    SetFocus(appWin);
#endif // _WIN32 (windowed-mode toggle; SDL handles window mode on Linux)
}

#ifdef _FORCE_MAIN_THREAD
void FalconDisplayConfiguration::LeaveSimWindowMode()
{
    SendMessage(appWin, FM_DISP_LEAVE_SIM_WINMODE, 0, 0);
}

void FalconDisplayConfiguration::_LeaveSimWindowMode()
#else
void FalconDisplayConfiguration::_LeaveSimWindowMode()
{
}

void FalconDisplayConfiguration::LeaveSimWindowMode()
#endif
{
    if (not mInSimWinMode)
        return;

#ifdef _WIN32
    if (appWin)
    {
        SetWindowLong(appWin, GWL_STYLE, mSavedWinStyle);
        SetWindowPos(appWin, HWND_TOP, mSavedWinRect.left, mSavedWinRect.top,
                     mSavedWinRect.right - mSavedWinRect.left,
                     mSavedWinRect.bottom - mSavedWinRect.top,
                     SWP_FRAMECHANGED bitor SWP_SHOWWINDOW);
    }
#endif

    displayFullScreen = mSavedFullScreen;
    mInSimWinMode = false;
}
