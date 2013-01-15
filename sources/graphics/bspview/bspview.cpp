/***************************************************************************\
    BSPview.cpp
    Scott Randolph
    January 29, 1998

    This is the tool which reads a Multigen FLT file and displays it
	through the BSPlib routines.
\***************************************************************************/
#include <stdlib.h>
#include <conio.h>
#include <float.h>
#include <windows.h>
#include <commctrl.h>
#include "shi\ShiError.h"
#include "Matrix.h"
#include "Loader.h"
#include "TimeMgr.h"
#include "DevMgr.h"
#include "Render3d.h"
#include "Tex.h"
#include "ObjectInstance.h"
#include "StateStack.h"
#include "..\BspUtil\ParentBuildList.h"
#include "JoyInput.h"
#include "resource.h"
#include "..\BSPBuild\PlayerOp.h"
#include "falclib\include\dispcfg.h"
#include "debuggr.h"

#define DTR  0.01745329F
#include <Commdlg.h>
#include "Weather.h"
// OW - Todo:
// - window resizing doesnt work
// - add mouse control

//#define SCR_STUFF
PlayerOptionsClass PlayerOptions;
char *FalconTerrainDataDir = "";
char *FalconObjectDataDir = "";
char *FalconMiscTexDataDir = "";
char *Falcon3DDataDir = "";		// M.N.
long __stdcall FalconMessageHandler (HWND hw, unsigned int msg, unsigned int wp, long lp);
char g_CardDetails[1024];

int NumHats;

// Helper functions
BOOL GetNameFromUser( char *target, int targetSize );
int ChooseObjectID( int defaultID );
void DrawGroundPlane( Render3D *renderer, Texture *tex );
void ControlPanel(void);
BOOL CALLBACK DialogProc( HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam );	
void ConstructDeltaMatrix( float p, float r, float y, Trotation *T );

void ResetLatLong(void) {}
void SetLatLong(float, float) {}
float WeatherClass::WxAltShift	= -2000.0f;
// Globals shared by multiple functions
//DeviceManager		devmgr;
//DisplayDevice		device;
Render3D			renderer;
ImageBuffer			*image = NULL;
ObjectInstance		*obj = NULL;
BOOL				run = FALSE;
float		    zoompos = 0.0f;
float	    roll = 0.0f, pitch = 0.0f, yaw = 0.0f;

DWORD				BACKGROUND_COLOR = 0xFFE5CCB2;
enum {CAM_ORBIT, CAM_PAN, CAM_ORIGIN} cameraMode = CAM_ORBIT;

extern "C" {
int movieInit( int numMovies, LPVOID lpDD ) { return 0;}
void movieUnInit( void )
{}
}

// Time stamp from utility library
extern char FLTtoGeometryTimeStamp[];

// Unfortunatly, this is required by ShiError.h
int shiAssertsOn=1,shiHardCrashOn=0, shiWarningsOn = 1;

#ifdef USE_SH_POOLS
extern MEM_POOL gBSPLibMemPool, glMemPool;
#endif
static void 
Register ()
{
	WNDCLASS	wc;
       // set up and register window class
   wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_NOCLOSE;
   wc.lpfnWndProc = FalconMessageHandler;
   wc.cbClsExtra = 0;
   wc.cbWndExtra = sizeof(DWORD);
   wc.hInstance = NULL;
//   wc.hIcon = NULL;
     wc.hIcon = LoadIcon (GetModuleHandle(NULL), MAKEINTRESOURCE(105));	// OW BC
   wc.hCursor = NULL;
   wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
   wc.lpszMenuName = NULL;
   wc.lpszClassName = "FalconDisplay";

   // Register this class.
   ATOM m = RegisterClass(&wc);
}

/*
 * Initialization, message loop
 */
int main( int argc, char *argv[] )
{   
	char	msgbuf [1024];
	char	filename[_MAX_PATH];
	char	drive[_MAX_DRIVE];
	char	path[_MAX_DIR];
	char	fname[_MAX_FNAME];
	char	ext[_MAX_EXT];
	int		result;
	int		id = 0;
	
    InitDebug(DEBUGGER_TEXT_MODE);
	// Set the FPU to 24 bit precision
	_controlfp( _PC_24,   MCW_PC );
    _controlfp( _RC_CHOP, MCW_RC);
#ifdef  USE_SH_POOLS
	glMemPool = MemPoolInit( 0 );
	Palette::InitializeStorage ();
#endif
	#ifdef USE_SH_POOLS
	if ( gBSPLibMemPool == NULL )
	{
		gBSPLibMemPool = MemPoolInit( 0 );
	}
	#endif


	// Display are startup banner
	printf( "BSPview compiled %s.  FLT reader %s\n", __TIMESTAMP__, FLTtoGeometryTimeStamp );


	// See if we got a filename on the command line
	if ( argc == 2) {
		result = GetFullPathName( argv[1], sizeof( filename ), filename, NULL );
	} else {
		result = 0;
	}

	// If we didn't get it on the command line, ask the user
	if (result == 0) {
		strcpy( filename, "" );
		if ( !GetNameFromUser( filename, sizeof(filename) )) {
			return -1;
		}
	}

	_splitpath( filename, drive, path, fname, ext );


	/***********************************************************************************\
		Here we decide how to deal with the file the user provided
	\***********************************************************************************/
	if (stricmp(ext, ".FLT") == 0) {
		// We've got a FLT file, so process and display it
	
		// Add it to our object list
		id = 0;
		TheParentBuildList.AddItem( id, filename );

		// Now construct the object array and read each object
		if ( !TheParentBuildList.BuildParentTable() ) {
			printf("ERROR:  We got no objects to process.\n");
			exit( -1 );
		}

	} else 	if (stricmp(ext, ".HDR") == 0) {
		// We've got a master object file, so load it
		strcpy( filename, drive );
		strcat( filename, path );
//		strcat( filename, "KoreaObj" );
		strcat( filename, fname );
		ObjectParent::SetupTable( filename );

		// Ask the user which object ID to display
		id = ChooseObjectID( 2 );

	} else {
		char message[256];
		sprintf( message, "Unrecognized input file type %s", ext );
		ShiError( message );
	}



	/***********************************************************************************\
		Read some default info from the registry, then confirm it with the user
	\***********************************************************************************/
	int				DriverNumber;
	int				DeviceNumber;
	int				screenWidth;
	DWORD			type, size;
	HKEY			theKey;

	RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\MicroProse\\Falcon\\BSPview",
	0, KEY_ALL_ACCESS, &theKey);

	size = sizeof (DriverNumber);
	if (RegQueryValueEx(theKey, "DriverNumber", 0, &type, (LPBYTE)&DriverNumber, &size) != ERROR_SUCCESS) {
		DriverNumber = -1;
	}

	size = sizeof (DeviceNumber);
	if (RegQueryValueEx(theKey, "DeviceNumber", 0, &type, (LPBYTE)&DeviceNumber, &size) != ERROR_SUCCESS) {
		DeviceNumber = -1;
	}

	size = sizeof (screenWidth);
	if (RegQueryValueEx(theKey, "ScreenWidth", 0, &type, (LPBYTE)&screenWidth, &size) != ERROR_SUCCESS) {
		screenWidth = 640;
	}


	/***********************************************************************************\
		Here begins the display setup
	\***********************************************************************************/
	BOOL				drawGroundPlane;
	DWORD				start;
	DWORD				now;
	char				frameRate[80];
	Ppoint				lightVec = {0.0f, 0.0f, -1.0f};
	Texture				groundTexture;

	FalconDisplay.devmgr.Setup();
	FalconDisplay.displayFullScreen = 0;
	if ((DriverNumber < 0) || (DeviceNumber < 0)) {
		FalconDisplay.devmgr.ChooseDevice( &DriverNumber, &DeviceNumber, &screenWidth );
	}
	Register();
RECT		rect;

   // Choose an appropriate window style
   if (FalconDisplay.displayFullScreen)
   {
		FalconDisplay.xOffset = 0;
		FalconDisplay.yOffset = 0;
		FalconDisplay.windowStyle = WS_POPUP;
   }
   else
   {
		FalconDisplay.windowStyle = WS_OVERLAPPEDWINDOW;
   }

   // Build a window for this application
   rect.top = rect.left = 0;
   rect.right = screenWidth;
   rect.bottom = screenWidth*3/4;
   AdjustWindowRect(&rect,	FalconDisplay.windowStyle, FALSE);
   FalconDisplay.appWin = CreateWindow(
	   "FalconDisplay",			/* class */
	   "3D Output",				/* caption */
	   FalconDisplay.windowStyle,					/* style */
	   40,			/* init. x pos */
	   40,			/* init. y pos */
	   rect.right-rect.left,	/* init. x size */
	   rect.bottom-rect.top,	/* init. y size */
	   NULL,					/* parent window */
	   NULL,					/* menu handle */
	   NULL,					/* program handle */
	   NULL					/* create parms */
   );
   if (!FalconDisplay.appWin) {
	   ShiError( "Failed to construct main window" );
   }

   UpdateWindow(FalconDisplay.appWin);
   SetFocus(FalconDisplay.appWin);

   // Display the new rendering window
   ShowWindow( FalconDisplay.appWin, SW_SHOW );

	DWORD err  = GetLastError();
	FalconDisplay.theDisplayDevice.Setup( DriverNumber, DeviceNumber, screenWidth, screenWidth*3/4, 16, FALSE, TRUE, FalconDisplay.appWin, TRUE);
	image = FalconDisplay.theDisplayDevice.GetImageBuffer();

	TheLoader.Setup();
	TheTimeManager.Setup( 2000, 257 );
	Texture::SetupForDevice( FalconDisplay.theDisplayDevice.GetDefaultRC(), "." );
	renderer.Setup( image );
	renderer.SetColor( 0xFFFFFFFF );
	renderer.SetBackground( BACKGROUND_COLOR );

	TheJoystick.Setup();

	// If a ground tile texture is available, load it and set the flag to draw it
	static char groundPlaneName[] = "BSPViewGround.pcx";
	if (GetFileAttributes(groundPlaneName) == 0xFFFFFFFF) {
		drawGroundPlane = FALSE;
	} else {
		groundTexture.LoadAndCreate( groundPlaneName, MPR_TI_PALETTE );
		drawGroundPlane = TRUE;
	}


	Ppoint		camInput = {0.0f, 0.0f, 0.0f};
	Ppoint		camPos;
	Pmatrix		camRot = IMatrix;
	Ppoint		objPos = Origin;
	Pmatrix		objRot = IMatrix;
	Pmatrix		rot;

	float			LODBias	= 1.0f;
	float			Scale	= 1.0f;
	float			angle	= 0.0f;
	float			dx		= 0.0f;
	int				sw = 0;

	obj = new ObjectInstance( id );
	run = TRUE;
	int frameCnt = 0;
	frameRate[0] = 0;
	start = GetTickCount();


	/***********************************************************************************\
		Here begins the display loop
	\***********************************************************************************/
	while (run) {
		frameCnt++;
		
	    	MSG	msg;

	    while(PeekMessage(&msg, FalconDisplay.appWin, 0, 0, PM_REMOVE))
		{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	    }

		// Process keyboard input
		if (_kbhit()) {
			switch (_getch()) {
			  case 'Q':
			  case 'q':
				run = FALSE;
				break;
			  case 'C':
			  case 'c':
				ControlPanel();
				break;
			}
		}

		if (TheJoystick.HaveJoystick()) {
		    // Process joystick input
		    TheJoystick.Update( GetTickCount() );
		    
		    if (TheJoystick.buttons & JOY_BUTTON2) {
			run = FALSE;
		    }
		    if (TheJoystick.buttons & JOY_BUTTON1) {
			MatrixMult( &camRot, &TheJoystick.deltaMatrix, &rot );
			camRot = rot;
		    }
		    if (TheJoystick.buttons & JOY_BUTTON3) {
			ControlPanel();
		    }
		    if (TheJoystick.buttons & JOY_BUTTON4) {
			camRot	= IMatrix;
			objRot	= IMatrix;
			LODBias	= 1.0f;
			TheStateStack.SetLODBias( LODBias );
			Scale	= 1.0f;
			angle	= 0.0f;
		    }
		    
		    
#ifdef SCR_STUFF
		    if (TheJoystick.buttons & JOY_BUTTON5) {
			MatrixMult( &objRot, &TheJoystick.deltaMatrix, &rot );
			objRot = rot;
		    }
#endif
		    
		    // Update the camera position
		    camInput.x = -(obj->Radius()*0.75f + TheJoystick.throttle*obj->Radius()*300.f);
		}
		else {
		    camInput.x = -(obj->Radius()*0.75f + zoompos*obj->Radius());
		    Pmatrix deltaMatrix;
		    ConstructDeltaMatrix (pitch, roll, yaw, &deltaMatrix);

//		    MatrixMult( &camRot, &deltaMatrix, &rot );
		    camRot = deltaMatrix;
		}
		switch (cameraMode) {
		  case CAM_ORBIT:
			MatrixMult( &camRot, &camInput, &camPos );
			break;
		  case CAM_PAN:
			camPos = camInput;
			break;
		  case CAM_ORIGIN:
			camPos = Origin;
			break;
		}

		// Draw the frame
		TheTimeManager.SetTime( GetTickCount() );
		renderer.SetCamera( &camPos, &camRot );

		renderer.StartFrame();
		renderer.ClearFrame();

		if (drawGroundPlane) {
			DrawGroundPlane( &renderer, &groundTexture );
		}

//		renderer.Render3DObject(id, &objPos, &objRot);
		TheStateStack.DrawObject( obj, &objRot, &objPos, Scale );

		sprintf(msgbuf, "id %0d frame %0d rate %s fps  Range %1.0f ft Radius %1.1f ft", id, frameCnt, frameRate, -camInput.x, obj->Radius() );
		renderer.SetColor( 0xFF000000 );

		// OW Render2D::ScreenText requires the font textures to be loaded which is nearly impossible without shipping bspview with the falcon data stuff
#if 0
		//renderer.ScreenText( 1.0f, 1.0f, msgbuf );
#else
		renderer.context.RestoreState(STATE_SOLID);
		renderer.context.TextOut(1, 1, 0, msgbuf);
#endif

		renderer.FinishFrame();
//		image->SwapBuffers(renderer.context.rc);
		image->SwapBuffers(false);
		now = GetTickCount();
		if (now-start > 1000) {

//			image->SwapBuffers(renderer.context.rc);
			if (now == start)  start -= 1;
			sprintf(frameRate, "%0.1f", frameCnt*1000.0f/(float)(now-start));
			ShiAssert( strlen(frameRate) < sizeof(frameRate) );
			start = now;
			frameCnt = 0;
//			OutputDebugString (msgbuf); OutputDebugString("\n");
		}
	}


	// If we read from a raw FLT file, we need to release our phantom references
	if (stricmp(ext, ".FLT") == 0) {
		TheParentBuildList.ReleasePhantomReferences();
	}

	// If we had a ground plane, we need to free its texture
	if (drawGroundPlane) {
		groundTexture.FreeAll();
	}


	// Clean up the objects
	for (int i=0; i<obj->ParentObject->nSlots; i++) {
		if (obj->SlotChildren[i]) {
			delete (obj->SlotChildren[i]);
		}
	}
	delete obj;
	ObjectParent::CleanupTable();

	image = NULL;
	renderer.Cleanup();
	FalconDisplay.theDisplayDevice.Cleanup();
	FalconDisplay.devmgr.Cleanup();
	TheJoystick.Cleanup();
	TheLoader.Cleanup();
	TheTimeManager.Cleanup();

	return 0;
}



/*
 * Get a filename from the user
 */
BOOL GetNameFromUser( char *target, int targetSize )
{
	ShiAssert( target );
	ShiAssert( targetSize > 0 );
	ShiAssert( strlen(target) < (unsigned)targetSize );

	OPENFILENAME dialogInfo;
	dialogInfo.lStructSize = sizeof( dialogInfo );
	dialogInfo.hwndOwner = NULL;
	dialogInfo.hInstance = NULL;
	dialogInfo.lpstrFilter = "MuliGen OpenFlight file\0*.FLT\0Falcon 4.0 BSP Library\0*.HDR\0\0";
	dialogInfo.lpstrCustomFilter = NULL;
	dialogInfo.nMaxCustFilter = 0;
	dialogInfo.nFilterIndex = 1;
	dialogInfo.lpstrFile = target;
	dialogInfo.nMaxFile = targetSize;
	dialogInfo.lpstrFileTitle = NULL;
	dialogInfo.nMaxFileTitle = 0;
	dialogInfo.lpstrInitialDir = NULL;
	dialogInfo.lpstrTitle = "Select an object source";
	dialogInfo.Flags = OFN_FILEMUSTEXIST;
	dialogInfo.lpstrDefExt = "FLT";

	if ( !GetOpenFileName( &dialogInfo ) ) {
		return FALSE;
	}
	return TRUE;
}



/*
 * Get an object ID from the user
 */
int ChooseObjectID( int defaultID )
{
    RECT			rect;
	HWND			editWin;
	char			string[32];

	// Build a window for this query
	rect.top = rect.left = 0;
	rect.right = 200;
	rect.bottom = 100;
	AdjustWindowRect(&rect,	WS_OVERLAPPEDWINDOW, FALSE);
	editWin = CreateWindow(
	    "EDIT",					/* class */
		"Enter Visual Object ID",/* caption */
		WS_OVERLAPPEDWINDOW,	/* style */
		CW_USEDEFAULT,			/* init. x pos */
		CW_USEDEFAULT,			/* init. y pos */
		rect.right-rect.left,	/* init. x size */
		rect.bottom-rect.top,	/* init. y size */
		NULL,					/* parent window */
		NULL,					/* menu handle */
		NULL,					/* program handle */
		NULL					/* create parms */
	);
	if (!editWin) {
		ShiError( "Failed to construct list box window" );
	}

	// Set the default value and display the window
	SendMessage( editWin, EM_SETLIMITTEXT, 4, 0 );
	sprintf( string, "%d", defaultID );
	SendMessage( editWin, WM_SETTEXT, 0, (DWORD)string );
	ShowWindow( editWin, SW_SHOW );

	// Stop here until we get a choice from the user
	MessageBox( NULL, "Click OK when you've made your choice", "", MB_OK );
	SendMessage( editWin, WM_GETTEXT, sizeof(string), (DWORD)&string );

	// Get rid of the list box now that we're done with it
	DestroyWindow( editWin );
	editWin = NULL;

	return atoi( string );
}


// Draw a 1 KM square in the X/Y plane centered on the origin
void DrawGroundPlane( Render3D *renderer, Texture *tex )
{
	Tpoint			worldSpace;
	ThreeDVertex	v0, v1, v2, v3;
	static const float	HALF_KM = FEET_PER_KM / 2.0f;

	worldSpace.z = 0.0f;

	// South West
	worldSpace.x = -HALF_KM, 		worldSpace.y = -HALF_KM;
	renderer->TransformPoint( &worldSpace, &v0 );						   
	v0.u = 0.0f,	v0.v = 1.0f,	v0.q = v0.csZ * Q_SCALE;

	// North West
	worldSpace.x =  HALF_KM, 		worldSpace.y = -HALF_KM;
	renderer->TransformPoint( &worldSpace, &v1 );
	v1.u = 0.0f,	v1.v = 0.0f,	v1.q = v1.csZ * Q_SCALE;

	// South East
	worldSpace.x = -HALF_KM, 		worldSpace.y =  HALF_KM;
	renderer->TransformPoint( &worldSpace, &v2 );
	v2.u = 1.0f,	v2.v = 1.0f,	v2.q = v2.csZ * Q_SCALE;

	// North East
	worldSpace.x =  HALF_KM, 		worldSpace.y =  HALF_KM;
	renderer->TransformPoint( &worldSpace, &v3 );
	v3.u = 1.0f,	v3.v = 0.0f,	v3.q = v3.csZ * Q_SCALE;

	v0.r = v1.r = v2.r = v3.r = 0.0f;
	v0.g = v1.g = v2.g = v3.g = 0.0f;
	v0.b = v1.b = v2.b = v3.b = 0.0f;

	// Setup the drawing state for these polygons
	renderer->context.RestoreState( STATE_TEXTURE_PERSPECTIVE );

#if 0	// Enable to use Bilinear filtering
	renderer->context.SetState( MPR_STA_ENABLES, MPR_SE_FILTERING );
	renderer->context.SetState( MPR_STA_TEX_FILTER, MPR_TX_BILINEAR );
	renderer->context.InvalidateState();
#endif

	renderer->context.SelectTexture( tex->TexHandle() );

	renderer->DrawSquare( &v0, &v1, &v3, &v2, CULL_ALLOW_ALL );
}


void ControlPanel(void)
{
	HINSTANCE	hInstance;
	int		result;
	
	hInstance = GetModuleHandle(NULL);
	ShiAssert( hInstance );

	result = DialogBox( hInstance, MAKEINTRESOURCE(IDD_BSPVIEW_CONTROLS), NULL, (DLGPROC)DialogProc );
	ShiAssert( result != -1 );
}


void ApplyControlPanelChanges( HWND dlg )
{
	int		id;
	BOOL	result;
	HWND	control;
	DWORD	listSlot;
	char	string[80];
	float	value;
	DWORD	sliderPos;


	// Camera settings
	if (IsDlgButtonChecked( dlg, IDC_CAM_ORBIT )	== BST_CHECKED)	cameraMode = CAM_ORBIT;
	if (IsDlgButtonChecked( dlg, IDC_CAM_PAN )		== BST_CHECKED)	cameraMode = CAM_PAN;
	if (IsDlgButtonChecked( dlg, IDC_CAM_ORIGIN )	== BST_CHECKED)	cameraMode = CAM_ORIGIN;


	// Background color
	BACKGROUND_COLOR = 0;

	control = GetDlgItem( dlg, IDC_SLIDER_BG_R );
	sliderPos = SendMessage( control, TBM_GETPOS, 0, 0 );
	BACKGROUND_COLOR |= sliderPos;

	control = GetDlgItem( dlg, IDC_SLIDER_BG_G );
	sliderPos = SendMessage( control, TBM_GETPOS, 0, 0 );
	BACKGROUND_COLOR |= sliderPos << 8;

	control = GetDlgItem( dlg, IDC_SLIDER_BG_B );
	sliderPos = SendMessage( control, TBM_GETPOS, 0, 0 );
	BACKGROUND_COLOR |= sliderPos << 16;

	renderer.SetBackground( BACKGROUND_COLOR );


	// Switch settings
	if (IsDlgButtonChecked( dlg, IDC_SW_ENABLE )	== BST_CHECKED)	{

		control = GetDlgItem( dlg, IDC_SW_NUMBER );
		listSlot = SendMessage( control, LB_GETCURSEL, 0, 0 );
		if (listSlot != LB_ERR) {
			id = SendMessage( control, LB_GETITEMDATA, listSlot, 0 );
			DWORD mask = 0;

			if (IsDlgButtonChecked( dlg, IDC_SW_BIT0  )	== BST_CHECKED)	mask |= 0x00000001;
			if (IsDlgButtonChecked( dlg, IDC_SW_BIT1  )	== BST_CHECKED)	mask |= 0x00000002;
			if (IsDlgButtonChecked( dlg, IDC_SW_BIT2  )	== BST_CHECKED)	mask |= 0x00000004;
			if (IsDlgButtonChecked( dlg, IDC_SW_BIT3  )	== BST_CHECKED)	mask |= 0x00000008;
			if (IsDlgButtonChecked( dlg, IDC_SW_BIT4  )	== BST_CHECKED)	mask |= 0x00000010;
			if (IsDlgButtonChecked( dlg, IDC_SW_BIT5  )	== BST_CHECKED)	mask |= 0x00000020;
			if (IsDlgButtonChecked( dlg, IDC_SW_BIT6  )	== BST_CHECKED)	mask |= 0x00000040;
			if (IsDlgButtonChecked( dlg, IDC_SW_BIT7  )	== BST_CHECKED)	mask |= 0x00000080;
			if (IsDlgButtonChecked( dlg, IDC_SW_BIT8  )	== BST_CHECKED)	mask |= 0x00000100;
			if (IsDlgButtonChecked( dlg, IDC_SW_BIT9  )	== BST_CHECKED)	mask |= 0x00000200;
			if (IsDlgButtonChecked( dlg, IDC_SW_BIT10 )	== BST_CHECKED)	mask |= 0x00000400;
			if (IsDlgButtonChecked( dlg, IDC_SW_BIT11 )	== BST_CHECKED)	mask |= 0x00000800;
			if (IsDlgButtonChecked( dlg, IDC_SW_BIT12 )	== BST_CHECKED)	mask |= 0x00001000;
			if (IsDlgButtonChecked( dlg, IDC_SW_BIT13 )	== BST_CHECKED)	mask |= 0x00002000;
			if (IsDlgButtonChecked( dlg, IDC_SW_BIT14 )	== BST_CHECKED)	mask |= 0x00004000;
			if (IsDlgButtonChecked( dlg, IDC_SW_BIT15 )	== BST_CHECKED)	mask |= 0x00008000;
			if (IsDlgButtonChecked( dlg, IDC_SW_BIT16 )	== BST_CHECKED)	mask |= 0x00010000;
			if (IsDlgButtonChecked( dlg, IDC_SW_BIT17 )	== BST_CHECKED)	mask |= 0x00020000;
			if (IsDlgButtonChecked( dlg, IDC_SW_BIT18 )	== BST_CHECKED)	mask |= 0x00040000;
			if (IsDlgButtonChecked( dlg, IDC_SW_BIT19 )	== BST_CHECKED)	mask |= 0x00080000;
			if (IsDlgButtonChecked( dlg, IDC_SW_BIT20 )	== BST_CHECKED)	mask |= 0x00100000;
			if (IsDlgButtonChecked( dlg, IDC_SW_BIT21 )	== BST_CHECKED)	mask |= 0x00200000;
			if (IsDlgButtonChecked( dlg, IDC_SW_BIT22 )	== BST_CHECKED)	mask |= 0x00400000;
			if (IsDlgButtonChecked( dlg, IDC_SW_BIT23 )	== BST_CHECKED)	mask |= 0x00800000;
			if (IsDlgButtonChecked( dlg, IDC_SW_BIT24 )	== BST_CHECKED)	mask |= 0x01000000;
			if (IsDlgButtonChecked( dlg, IDC_SW_BIT25 )	== BST_CHECKED)	mask |= 0x02000000;
			if (IsDlgButtonChecked( dlg, IDC_SW_BIT26 )	== BST_CHECKED)	mask |= 0x04000000;
			if (IsDlgButtonChecked( dlg, IDC_SW_BIT27 )	== BST_CHECKED)	mask |= 0x08000000;
			if (IsDlgButtonChecked( dlg, IDC_SW_BIT28 )	== BST_CHECKED)	mask |= 0x10000000;
			if (IsDlgButtonChecked( dlg, IDC_SW_BIT29 )	== BST_CHECKED)	mask |= 0x20000000;
			if (IsDlgButtonChecked( dlg, IDC_SW_BIT30 )	== BST_CHECKED)	mask |= 0x40000000;
			if (IsDlgButtonChecked( dlg, IDC_SW_BIT31 )	== BST_CHECKED)	mask |= 0x80000000;

			obj->SetSwitch( id, mask );
		}
	}

			
	// DOF Rotation settings
	if (IsDlgButtonChecked( dlg, IDC_DOFROT_ENABLE )	== BST_CHECKED)	{

		control = GetDlgItem( dlg, IDC_DOFROT_NUMBER );
		listSlot = SendMessage( control, LB_GETCURSEL, 0, 0 );
		if (listSlot != LB_ERR) {
			id = SendMessage( control, LB_GETITEMDATA, listSlot, 0 );
			GetDlgItemText( dlg, IDC_DOFROT_VALUE, string, sizeof(string) );
			value = (float)atof( string );
			value *= PI / 180.0f;
			obj->SetDOFrotation( id, value );
		}
	}

	// DOF Translation settings
	if (IsDlgButtonChecked( dlg, IDC_DOFTRANS_ENABLE )	== BST_CHECKED)	{

		control = GetDlgItem( dlg, IDC_DOFTRANS_NUMBER );
		listSlot = SendMessage( control, LB_GETCURSEL, 0, 0 );
		if (listSlot != LB_ERR) {
			id = SendMessage( control, LB_GETITEMDATA, listSlot, 0 );

			GetDlgItemText( dlg, IDC_DOFTRANS_VALUE, string, sizeof(string) );
			value = (float)atof( string );
			obj->SetDOFxlation( id, value );
		}
	}

	// Slot children settings (NOTE: the children are never cleaned up...)
	if (IsDlgButtonChecked( dlg, IDC_SLOT_ENABLE )	== BST_CHECKED)	{

		control = GetDlgItem( dlg, IDC_SLOT_NUMBER );
		listSlot = SendMessage( control, LB_GETCURSEL, 0, 0 );
		if (listSlot != LB_ERR) {
			id = SendMessage( control, LB_GETITEMDATA, listSlot, 0 );
			int childVisID = GetDlgItemInt( dlg, IDC_SLOT_CHILD, &result, FALSE );

			if (result) {
				if (obj->SlotChildren[id]) {
					delete (obj->SlotChildren[id]);
				}
				ObjectInstance *childObj = new ObjectInstance( childVisID );
				obj->SetSlotChild( id, childObj );
			}
		}
	}
}


BOOL CALLBACK DialogProc( HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	BOOL	result;
	char	string[10];
	HWND	control;
	DWORD	listSlot;
	int		i;

	switch (msg) {
	  case WM_INITDIALOG:
		switch (cameraMode) {
		  case CAM_ORBIT:
			CheckRadioButton( dlg, IDC_CAM_ORBIT, IDC_CAM_ORIGIN, IDC_CAM_ORBIT );
			break;
		  case CAM_PAN:
			CheckRadioButton( dlg, IDC_CAM_ORBIT, IDC_CAM_ORIGIN, IDC_CAM_PAN );
			break;
		  case CAM_ORIGIN:
			CheckRadioButton( dlg, IDC_CAM_ORBIT, IDC_CAM_ORIGIN, IDC_CAM_ORIGIN );
			break;
		}


		// Background color stuff
		// RED
		control = GetDlgItem( dlg, IDC_SLIDER_BG_R );
		SendMessage( control, TBM_SETRANGE, TRUE, (LPARAM)MAKELONG(0, 255) );
		SendMessage( control, TBM_SETPOS,   TRUE, BACKGROUND_COLOR & 0xFF );

		// GREEN
		control = GetDlgItem( dlg, IDC_SLIDER_BG_G );
		SendMessage( control, TBM_SETRANGE, TRUE, (LPARAM)MAKELONG(0, 255) );
		SendMessage( control, TBM_SETPOS,   TRUE, (BACKGROUND_COLOR >> 8) & 0xFF );

		// BLUE
		control = GetDlgItem( dlg, IDC_SLIDER_BG_B );
		SendMessage( control, TBM_SETRANGE, TRUE, (LPARAM)MAKELONG(0, 255) );
		SendMessage( control, TBM_SETPOS,   TRUE, (BACKGROUND_COLOR >> 16) & 0xFF );



		control = GetDlgItem( dlg, IDC_SW_NUMBER );
		for (i=0; i<obj->ParentObject->nSwitches; i++) {
			listSlot = SendMessage( control, LB_ADDSTRING, 0, (LPARAM)_itoa(i, string, sizeof(string)) );
			SendMessage( control, LB_SETITEMDATA, listSlot, i );
		}
		if (i==0) {
			EnableWindow( GetDlgItem( dlg, IDC_SW_ENABLE ), FALSE );
		}

		control = GetDlgItem( dlg, IDC_DOFROT_NUMBER );
		for (i=0; i<obj->ParentObject->nDOFs; i++) {
			listSlot = SendMessage( control, LB_ADDSTRING, 0, (LPARAM)_itoa(i, string, sizeof(string)) );
			SendMessage( control, LB_SETITEMDATA, listSlot, i );
		}
		if (i==0) {
			EnableWindow( GetDlgItem( dlg, IDC_DOFROT_ENABLE ), FALSE );
		}

		control = GetDlgItem( dlg, IDC_DOFTRANS_NUMBER );
		for (i=0; i<obj->ParentObject->nDOFs; i++) {
			listSlot = SendMessage( control, LB_ADDSTRING, 0, (LPARAM)_itoa(i, string, sizeof(string)) );
			SendMessage( control, LB_SETITEMDATA, listSlot, i );
		}
		if (i==0) {
			EnableWindow( GetDlgItem( dlg, IDC_DOFTRANS_ENABLE ), FALSE );
		}

		control = GetDlgItem( dlg, IDC_SLOT_NUMBER );
		for (i=0; i<obj->ParentObject->nSlots; i++) {
			listSlot = SendMessage( control, LB_ADDSTRING, 0, (LPARAM)_itoa(i, string, sizeof(string)) );
			SendMessage( control, LB_SETITEMDATA, listSlot, i );
		}
		if (i==0) {
			EnableWindow( GetDlgItem( dlg, IDC_SLOT_ENABLE ), FALSE );
		}

		result = TRUE;
		break;

	  case WM_COMMAND:
		switch(LOWORD(wParam)) {
		  case IDC_SW_ENABLE:
			if (IsDlgButtonChecked( dlg, IDC_SW_ENABLE ) == BST_CHECKED) {
				EnableWindow( GetDlgItem( dlg, IDC_SW_NUMBER ), TRUE );
			} else {
				control = GetDlgItem( dlg, IDC_SW_NUMBER );
				SendMessage( control, LB_SETCURSEL, (WPARAM)-1, 0 );
				EnableWindow( control, FALSE );
			}
			break;

		  case IDC_DOFROT_ENABLE:
			if (IsDlgButtonChecked( dlg, IDC_DOFROT_ENABLE ) == BST_CHECKED) {
				EnableWindow( GetDlgItem( dlg, IDC_DOFROT_NUMBER ), TRUE );
			} else {
				control = GetDlgItem( dlg, IDC_DOFROT_NUMBER );
				SendMessage( control, LB_SETCURSEL, (WPARAM)-1, 0 );
				EnableWindow( control, FALSE );
			}
			break;

		  case IDC_DOFTRANS_ENABLE:
			if (IsDlgButtonChecked( dlg, IDC_DOFTRANS_ENABLE ) == BST_CHECKED) {
				EnableWindow( GetDlgItem( dlg, IDC_DOFTRANS_NUMBER ), TRUE );
			} else {
				control = GetDlgItem( dlg, IDC_DOFTRANS_NUMBER );
				SendMessage( control, LB_SETCURSEL, (WPARAM)-1, 0 );
				EnableWindow( control, FALSE );
			}
			break;

		  case IDC_SLOT_ENABLE:
			if (IsDlgButtonChecked( dlg, IDC_SLOT_ENABLE ) == BST_CHECKED) {
				EnableWindow( GetDlgItem( dlg, IDC_SLOT_NUMBER ), TRUE );
			} else {
				control = GetDlgItem( dlg, IDC_SLOT_NUMBER );
				SendMessage( control, LB_SETCURSEL, (WPARAM)-1, 0 );
				EnableWindow( control, FALSE );
			}
			break;

		  case IDOK:
			ApplyControlPanelChanges(dlg);
			EndDialog(dlg, 0);
			break;

		  case IDCANCEL:
			EndDialog(dlg, 0);
			break;

		  case IDQUIT:
			run = FALSE;
			EndDialog(dlg, 0);
			break;
		}
		break;

	  default:
		result = FALSE;
		break;
	}

	return result;
}


LRESULT CALLBACK FalconMessageHandler (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT retval = 0;
    static InTimer=0;
    
    
#ifdef _MSC_VER
    // Set the FPU to Truncate
    _controlfp(_RC_CHOP,MCW_RC);
    
    // Set the FPU to 24bit precision
    _controlfp(_PC_24,MCW_PC);
#endif
    
    switch(message) {
    case WM_PAINT:
	{
	    PAINTSTRUCT ps;
	    BeginPaint(FalconDisplay.appWin, &ps);
	    EndPaint(FalconDisplay.appWin, &ps);
	    retval = 0;
	    break;
	}
	
    case WM_CHAR:
	{
	    switch (wParam) {
	    case 'Q':
	    case 'q':
		run = FALSE;
		break;
	    case 'C':
	    case 'c':
		ControlPanel();
		break;
	    case 'z':
		zoompos -= 0.01f;
		break;
	    case 'Z':
		zoompos -= 0.1f;
		break;
	    case 'x':
		zoompos += 0.01f;
		break;
	    case 'X':
		zoompos += 0.1f;
		break;
	    case 'p':
		pitch += 1.0f * DTR;
		break;
	    case 'P':
		pitch -= 1.0f * DTR;
		break;
	    case 'r':
		roll += 1.0f * DTR;
		break;
	    case 'R':
		roll -= 1.0f * DTR;
		break;
	    case 'y':
		yaw += 1.0f * DTR;
		break;
	    case 'Y':
		yaw -= 1.0f * DTR;
		break;
	    case '0':
		yaw = pitch = roll = 0.0f;
		break;
	    }

	    retval = 0;
	    break;
	}
    case WM_MOVE:
	{
	    RECT	dest;
	    GetClientRect(FalconDisplay.appWin, &dest);
	    ClientToScreen(FalconDisplay.appWin, (LPPOINT)&dest);
	    ClientToScreen(FalconDisplay.appWin, (LPPOINT)&dest+1);
	    if(image && image->frontSurface()) 
	    {
		image->UpdateFrontWindowRect(&dest);
	    }
	    InvalidateRect(FalconDisplay.appWin,&dest,FALSE);
	}
	retval = 0;
	break;
	
    case WM_QUIT:
	run = FALSE;
	break;
	
    default:
	return DefWindowProc (hwnd, message, wParam, lParam);
    }
    
    return retval;
}

void ConstructDeltaMatrix( float p, float r, float y, Trotation *T )
{                    
	Tpoint	at, up, rt;
	float	mag;

	// TODO:  Add in roll component.

	// at is a point on the unit sphere
	at.x = cos(y) * cos(p);
	at.y = sin(y) * cos(p);
	at.z = -sin(p);

	// (0, 0, 1) rolled and crossed with at [normalized]
	up.x = 0.0f;
	up.y = -sin(r);
	up.z = cos(r);
	rt.x = up.y * at.z - up.z * at.y;
	rt.y = up.z * at.x - up.x * at.z;
	rt.z = up.x * at.y - up.y * at.x;
	mag = sqrt(rt.x * rt.x + rt.y * rt.y + rt.z * rt.z);
	rt.x /= mag;
	rt.y /= mag;
	rt.z /= mag;

	// at cross rt [normalized]
	up.x = at.y * rt.z - at.z * rt.y;
	up.y = at.z * rt.x - at.x * rt.z;
	up.z = at.x * rt.y - at.y * rt.x;
	mag = sqrt(up.x * up.x + up.y * up.y + up.z * up.z);
	up.x /= mag;
	up.y /= mag;
	up.z /= mag;

	// Load the matrix
	T->M11 = at.x;	T->M12 = rt.x;	T->M13 = up.x;
	T->M21 = at.y;	T->M22 = rt.y;	T->M23 = up.y;
	T->M31 = at.z;	T->M32 = rt.z;	T->M33 = up.z;
}