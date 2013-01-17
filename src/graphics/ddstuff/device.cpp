/***************************************************************************\
    Device.cpp
    Scott Randolph
    November 12, 1996

    This class provides management of a single drawing device in a system.
\***************************************************************************/
#include "stdafx.h"
#include "Device.h"
#include "context.h"
#include "falclib\include\dispcfg.h"
#include "movie\avimovie.h"
#include "FalcLib\include\playerop.h"

// The pixel depth is hardwired for now
static const int BITS_PER_PIXEL	= 16;


// Initialize our member variables
DisplayDevice::DisplayDevice()
{
	appWin			= NULL;
	driverNumber	= -1;

	m_DXCtx = NULL;
}


// Clean up after ourselves
DisplayDevice::~DisplayDevice()
{
	ShiAssert( !IsReady() );
}


// Intialize our device.  This must be called before any images
// are constructed.
void DisplayDevice::Setup( int driverNum, int devNum, int width, int height, int depth, BOOL fullScreen, BOOL dblBuffer, HWND win, BOOL bWillCallSwapBuffer)
{
    RECT		rect;
	DWORD		style;
	WNDCLASS	wc;
	int			resNum;
	UInt		w, h, d;

	ShiAssert( !IsReady() );


	// Remember our driver number so we know if we're software (driver 0) or hardware.
	driverNumber = driverNum;


	#ifdef _DEBUG
	// For now we get the driver name again here.
	// TODO:  Eliminate the need for this by improving the MPR API?
	const char *lpDriverName;
	lpDriverName = FalconDisplay.devmgr.GetDriverName( driverNum);
	if(lpDriverName == NULL) ShiWarning( "Failed to get a name for the driver number" );
	#endif

	// OW: always create flipping chain in fullscreen (even if we dont flip - we dont)
	dblBuffer = fullScreen;	// warning: C_Handler::Setup relies on this behaviour

	// For now, we go figure out the number for the resolution we want
	// TODO:  Change the DisplayDevice API to require the resNum to be passed in?
	for (resNum=0; TRUE; resNum++) {
		if (FalconDisplay.devmgr.GetMode( driverNum, devNum, resNum, &w, &h, &d)) {
			if((w == (unsigned) width) && (h == (unsigned)height) && (d == (unsigned)depth)) {
				// Found it
				break;
			}
		} else {
			// Ran off the end of the list
			char message[80];
			sprintf( message, "Requested unavilable resolution %0dx%0dx%0d", width, height, depth);
			ShiError( message );
		}
	}

	// Create an MPR device handle for this device
	m_DXCtx = FalconDisplay.devmgr.CreateContext(driverNumber, devNum, resNum, fullScreen, win);

	if(!m_DXCtx)
	{
		// try default device
		driverNumber = 0;

		m_DXCtx = FalconDisplay.devmgr.CreateContext(driverNumber, 0, resNum, fullScreen, win);
		if(!m_DXCtx)
			return;
	}

	// See if we need to build our own window
	if (win) {

		// Store the applications main window for later use
		appWin = win;
		privateWindow = FALSE;

	} else {

		// set up and register window class
		wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS | CS_NOCLOSE;
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
		if (fullScreen) {
			style = WS_POPUP;
		} else {
			style = WS_OVERLAPPEDWINDOW;
		}

		// Build a window for this application
		rect.top = rect.left = 0;
		rect.right = width;
		rect.bottom = height;
		AdjustWindowRect(&rect,	style, FALSE);
		appWin = CreateWindow(
	    	"RenderTarget",			/* class */
			"Falcon 4.0 Demo",		/* caption */
			style,					/* style */
			50,			/* init. x pos */
			50,			/* init. y pos */
			rect.right-rect.left,	/* init. x size */
			rect.bottom-rect.top,	/* init. y size */
			NULL,					/* parent window */
			NULL,					/* menu handle */
			NULL,					/* program handle */
			NULL					/* create parms */
		);
		if (!appWin) {
			ShiError( "Failed to construct main window" );
		}

		// Make note of the fact that we'll have to release this window when we're done
		privateWindow = TRUE;
	}


	// Ensure the rendering window is visible
	ShowWindow( appWin, SW_SHOW );


	// Select the requested mode of operation
	if(fullScreen)
	{
		// If this is other than the default primary display, shrink the target window
		// on the desktop and don't let DirectX muck with it.
#if 0
		SetWindowPos( appWin, HWND_TOP, 0, 200, 10, 4, SWP_NOCOPYBITS | SWP_SHOWWINDOW );
#endif

		// Create the primary surface(s)
		if(dblBuffer)
			image.Setup( this, width, height, Primary, Flip, appWin, FALSE, TRUE, bWillCallSwapBuffer );
		else
			image.Setup( this, width, height, Primary, IsHardware() ? VideoMem : SystemMem, appWin, FALSE, TRUE, bWillCallSwapBuffer);
	}

	else
		image.Setup( this, width, height, Primary, IsHardware() ? VideoMem : SystemMem, appWin, TRUE, FALSE, bWillCallSwapBuffer);

	// Make sure we haven't gotten confused about how many contexts we have
//	ShiAssert( ContextMPR::StateSetupCounter == 0 );
	if(ContextMPR::StateSetupCounter != 0)	
		ContextMPR::StateSetupCounter = 0;	// Force it for now.  Shouldn't be required.

	// Create a rendering context for the primary surface
	m_DXCtx->SetRenderTarget(image.targetSurface());

	movieInit(2, m_DXCtx->m_pDD);
	}


// Release the Direct Draw interface object.  No calls to this device are
// allowed after this one (except another call to Setup).
void DisplayDevice::Cleanup( void )
{
	ShiAssert( IsReady() );

	if(m_DXCtx)
	{
	    m_DXCtx->Release();
		//delete m_DXCtx;
		m_DXCtx = NULL;
	}

	// Destroy our primary surface and its default context
	image.Cleanup();

	// Make sure we haven't gotten confused about how many contexts we have
//	ShiAssert( ContextMPR::StateSetupCounter == 0 );
	if (ContextMPR::StateSetupCounter != 0) {	
		ContextMPR::StateSetupCounter = 0;	// Force it for now.  Shouldn't be required.
	}

	// Destroy the application window if we created it
	if ( privateWindow ) {
		DestroyWindow( appWin );
	}
	appWin = NULL;

	movieUnInit();
}
