/***************************************************************************\
    Device.cpp
    Scott Randolph
    November 12, 1996

    This class provides management of a single drawing device in a system.
\***************************************************************************/
#include "stdafx.h"
#include "device.h"
#include "context.h"
#include "falclib/include/dispcfg.h"
#ifdef _WIN32
#include "movie/avimovie.h"   // VFW-based movie player (unported on Linux)
#endif
#include "falclib/include/playerop.h"

// The pixel depth is hardwired for now
static const int BITS_PER_PIXEL = 16;


// Initialize our member variables
DisplayDevice::DisplayDevice()
{
    appWin = NULL;
    driverNumber = -1;

    m_DXCtx = NULL;
}


// Clean up after ourselves
DisplayDevice::~DisplayDevice()
{
    ShiAssert(not IsReady());
}


// Initialize our device.  This must be called before any images
// are constructed.
// PHASE 1 (D3D7->D3D11): under D3D11 DDraw mode enumeration is not needed.
extern int g_d3d11ReqWidth, g_d3d11ReqHeight, g_d3d11ReqDepth;

void DisplayDevice::Setup(int driverNum, int devNum, int width, int height,
                          int depth, bool fullScreen, BOOL dblBuffer, HWND win,
                          BOOL bWillCallSwapBuffer)
{
    RECT rect;
    DWORD style;
#ifdef _WIN32
    WNDCLASS
    wc; // legacy DDraw private-window class (Linux builds its window in ffplatform/SDL3)
#endif
    int resNum;
    UInt w, h, d;

    ShiAssert(not IsReady());


    // Remember our driver number so we know if we're software (driver 0) or hardware.
    driverNumber = driverNum;


#ifdef _DEBUG
    // For now we get the driver name again here.
    // TODO:  Eliminate the need for this by improving the MPR API?
    const char *lpDriverName;
    lpDriverName = FalconDisplay.devmgr.GetDriverName(driverNum);

    if (lpDriverName == NULL)
        ShiWarning("Failed to get a name for the driver number");

#endif

    // OW: always create flipping chain in fullscreen (even if we dont flip - we dont)
    dblBuffer =
        fullScreen; // warning: C_Handler::Setup relies on this behaviour

    // For now, we go figure out the number for the resolution we want
    // TODO:  Change the DisplayDevice API to require the resNum to be passed in?
    // #DX12: GPU mode (D3D11 OR D3D12) takes the requested resolution directly and skips the DDraw mode
    // enumeration (which is empty on modern Windows -> ShiError "unavailable resolution").
    // #DX12/#104: any modern GPU backend (D3D12 OR Vulkan) takes the requested resolution directly and skips
    // the dead DDraw mode enumeration -- gate on g_bUseGpu, not g_bUseD3D12 (Vulkan clears the latter).
    extern bool g_bUseGpu;
    if (g_bUseGpu)
    {
        g_d3d11ReqWidth = width;
        g_d3d11ReqHeight = height;
        g_d3d11ReqDepth = depth ? depth : 32;
        resNum = 0;
    }
    else
    {
        for (resNum = 0; TRUE; resNum++)
        {
            if (FalconDisplay.devmgr.GetMode(driverNum, devNum, resNum, &w, &h,
                                             &d))
            {
                if ((w == (unsigned)width) and (h == (unsigned)height) and
                    (d == (unsigned)depth))
                {
                    // Found it
                    break;
                }
            }
            else
            {
                // Ran off the end of the list
                char message[80];
                sprintf(message, "Requested unavailable resolution %0dx%0dx%0d",
                        width, height, depth);
                ShiError(message);
            }
        }
    }

    // Create an MPR device handle for this device
    m_DXCtx = FalconDisplay.devmgr.CreateContext(driverNumber, devNum, resNum,
                                                 fullScreen, win);
    if (not m_DXCtx)
    {
        // try default device
        driverNumber = 0;

        m_DXCtx = FalconDisplay.devmgr.CreateContext(driverNumber, 0, resNum,
                                                     fullScreen, win);
        if (not m_DXCtx)
            return;
    }

    // See if we need to build our own window
    if (win)
    {

        // Store the applications main window for later use
        appWin = win;
        privateWindow = FALSE;
    }
#ifdef _WIN32
    else
    {

        // set up and register window class
        wc.style = CS_HREDRAW bitor CS_VREDRAW bitor CS_OWNDC bitor
                   CS_DBLCLKS bitor CS_NOCLOSE;
        wc.lpfnWndProc = DefWindowProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = sizeof(DWORD);
        wc.hInstance = NULL;
        wc.hIcon = NULL;
        wc.hCursor = NULL;
        wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        wc.lpszMenuName = NULL;
        wc.lpszClassName = "RenderTarget";

        // Register this class.
        RegisterClass(&wc);

        // Choose an appropriate window style
        // PHASE 1 (D3D7->D3D11): under D3D11 we render in a WINDOW -- always WS_OVERLAPPEDWINDOW (frame/controls).
        extern bool g_bUseGpu;
        if (fullScreen &&
            !g_bUseGpu) // #DX12/#104: any GPU backend (D3D12/Vulkan) uses a windowed frame
        {
            style = WS_POPUP;
        }
        else
        {
            style = WS_OVERLAPPEDWINDOW;
        }

        // Build a window for this application
        rect.top = rect.left = 0;
        rect.right = width;
        rect.bottom = height;
        AdjustWindowRect(&rect, style, FALSE);
        appWin = CreateWindow("RenderTarget", /* class */
                              "FreeFalcon Demo", /* caption */
                              style, /* style */
                              50, /* init. x pos */
                              50, /* init. y pos */
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

        // Make note of the fact that we'll have to release this window when we're done
        privateWindow = TRUE;
    }
#endif // _WIN32 (legacy DDraw private-window creation; Linux uses the ffplatform/SDL3 window)


    // Ensure the rendering window is visible
    ShowWindow(appWin, SW_SHOW);


    // Select the requested mode of operation
    if (fullScreen)
    {
        // If this is other than the default primary display, shrink the target window
        // on the desktop and don't let DirectX muck with it.
#if 0
        SetWindowPos(appWin, HWND_TOP, 0, 200, 10, 4, SWP_NOCOPYBITS bitor SWP_SHOWWINDOW);
#endif

        // Create the primary surface(s)
        if (dblBuffer)
            image.Setup(this, width, height, Primary, Flip, appWin, FALSE, TRUE,
                        bWillCallSwapBuffer);
        else
            image.Setup(this, width, height, Primary,
                        IsHardware() ? VideoMem : SystemMem, appWin, FALSE,
                        TRUE, bWillCallSwapBuffer);
    }

    else
        image.Setup(this, width, height, Primary,
                    IsHardware() ? VideoMem : SystemMem, appWin, TRUE, FALSE,
                    bWillCallSwapBuffer);

    // Make sure we haven't gotten confused about how many contexts we have
    // ShiAssert( ContextMPR::StateSetupCounter == 0 );
    if (ContextMPR::StateSetupCounter not_eq 0)
        ContextMPR::StateSetupCounter =
            0; // Force it for now.  Shouldn't be required.

    // Create a rendering context for the primary surface
    m_DXCtx->SetRenderTarget(image.targetSurface());

#ifdef _WIN32
    movieInit(
        2,
        m_DXCtx
            ->m_pDD); // VFW movie subsystem (unported on Linux; movies disabled)
#endif
}


// Release the Direct Draw interface object.  No calls to this device are
// allowed after this one (except another call to Setup).
void DisplayDevice::Cleanup(void)
{
    ShiAssert(IsReady());

    if (m_DXCtx)
    {
        m_DXCtx->Release();
        //delete m_DXCtx;
        m_DXCtx = NULL;
    }

    // Destroy our primary surface and its default context
    image.Cleanup();

    // Make sure we haven't gotten confused about how many contexts we have
    // ShiAssert( ContextMPR::StateSetupCounter == 0 );
    if (ContextMPR::StateSetupCounter not_eq 0)
    {
        ContextMPR::StateSetupCounter =
            0; // Force it for now.  Shouldn't be required.
    }

    // Destroy the application window if we created it
#ifdef _WIN32
    if (privateWindow)
    {
        DestroyWindow(
            appWin); // only the legacy DDraw private window (never created on Linux)
    }
#endif

    appWin = NULL;

#ifdef _WIN32
    movieUnInit(); // VFW movie subsystem (unported on Linux)
#endif
}
