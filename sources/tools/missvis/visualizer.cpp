#include <windows.h>
#include <math.h>
#include <float.h>
#include "Utils\Setup.h"
#include "Utils\TimeMgr.h"
#include "DDstuff\DevMgr.h"
#include "Terrain\Tmap.h"
#include "Weather\WXmap.h"
#include "Renderer\RViewPnt.h"
#include "Renderer\RenderWire.h"
#include "Objects\DrawBSP.h"
#include "Objects\DrawSgmt.h"
#include "Texture\TerrTex.h"
#include "Utils\GraphicsRes.h"
#include "LineList.h"
#include "Sim.h"
#include "DataBase.h"



/*
 * This is the callback functions that draws each requested record
 */
void DrawCallback( DataPoint *dp )
{
	Tpoint	p1, p2;

	// Anchor all vectors at the current position
	p1.x = dp->x;
	p1.y = dp->y;
	p1.z = dp->z;


	// Put in a line to the target
	p2.x = dp->targetX;
	p2.y = dp->targetY;
	p2.z = dp->targetZ;

	// Color code with "hasTarget"
	switch (dp->targetState) {
	case 0:
		TheLineList.AddLine( &p1, &p2, 0xFF004000 );
		break;
	case 1:
		TheLineList.AddLine( &p1, &p2, 0xFF008000 );
		break;
	case 2:
		TheLineList.AddLine( &p1, &p2, 0xFF00FF00 );
		break;
	}

#if 0
	// Put in the a range metric going straight up
	p2.x = p1.x;
	p2.y = p1.y;
	p2.z = -dp->range + p1.z;
	TheLineList.AddLine( &p1, &p2, 0xFFFF0000 );
#endif


	// Put in the velocity vector (scaled)
	p2.x = dp->dx / 50.0f + p1.x;
	p2.y = dp->dy / 50.0f + p1.y;
	p2.z = dp->dz / 50.0f + p1.z;
	TheLineList.AddLine( &p1, &p2, 0xFF0000FF );


	// Projects the velocity vector onto the ground
	p1.z = dp->groundZ;
	p2.z = dp->groundZ;
	TheLineList.AddLine( &p1, &p2, 0xFF000080 );
}


void Redraw( unsigned startAt, unsigned endBefore )
{
	TheLineList.ClearAll();
	TheDataBase.Process( DrawCallback, startAt, endBefore );
}


/*
 * Initialization, message loop
 */
int PASCAL WinMain (HANDLE this_inst, HANDLE prev_inst, LPSTR cmdline, int cmdshow)

{   
	MSG		msg;
    int		done = FALSE;
	int		screenWidth;
	int		DeviceNumber;
	int		DriverNumber;
	HWND	win;

	DWORD	type, size;
	HKEY	theKey;
	char	terrDir[_MAX_PATH];
	char	objDir[_MAX_PATH];
	char	misctexDir[_MAX_PATH];
	char	filename[_MAX_PATH];

	DeviceManager		devmgr;
	DisplayDevice		device;
	ImageBuffer			*image;
	RenderOTW			*renderer;
	RViewPoint			viewpoint;
	SimClass			Sim;
	Texture				wire;

	Tpoint			InitialPosition;
	Trotation		ViewerOrientation;


   /*
	*	Initialize Smart Heap (required by version 3.x only)
	*/
#ifdef USE_SMART_HEAP
	MemRegisterTask();
#endif


	// Get the missile test data
	TheDataBase.ReadData( "C:\\temp\\MissileTrack.bin" );



   /*
	*	Read some default info from the registry, then confirm it with the user
	*/
	RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\MicroProse\\Falcon\\4.0",
	0, KEY_ALL_ACCESS, &theKey);

	size = sizeof (terrDir);
	if (RegQueryValueEx(theKey, "FlyerTheaterDir", 0, &type, (LPBYTE)terrDir, &size) != ERROR_SUCCESS) {
		if (RegQueryValueEx(theKey, "theaterDir", 0, &type, (LPBYTE)terrDir, &size) != ERROR_SUCCESS) {
			strcpy( terrDir, "j:\\terrdata\\Korea3" );
		}
	}

	size = sizeof (objDir);
	if (RegQueryValueEx(theKey, "FlyerObjectDir", 0, &type, (LPBYTE)objDir, &size) != ERROR_SUCCESS) {
		if (RegQueryValueEx(theKey, "objectDir", 0, &type, (LPBYTE)objDir, &size) != ERROR_SUCCESS) {
			strcpy( objDir, "j:\\terrdata\\ObjectsDummy" );
		}
	}

	size = sizeof (misctexDir);
	if (RegQueryValueEx(theKey, "FlyerMisctexDir", 0, &type, (LPBYTE)misctexDir, &size) != ERROR_SUCCESS) {
		if (RegQueryValueEx(theKey, "misctexDir", 0, &type, (LPBYTE)misctexDir, &size) != ERROR_SUCCESS) {
			strcpy( misctexDir, "j:\\terrdata\\MiscTex" );
		}
	}

	size = sizeof (screenWidth);
	if (RegQueryValueEx(theKey, "FlyerScreenWidth", 0, &type, (LPBYTE)&screenWidth, &size) != ERROR_SUCCESS) {
		screenWidth = 640;
	}

	RegCloseKey(theKey);


	// Get the path of the map the user wants to fly over
	OPENFILENAME dialogInfo;
	sprintf( filename, "%s\\terrain\\Theater", terrDir );
	dialogInfo.lStructSize = sizeof( dialogInfo );
	dialogInfo.hwndOwner = NULL;
	dialogInfo.hInstance = NULL;
	dialogInfo.lpstrFilter = "Falcon 4 Terrain Map\0*.MAP\0\0";
	dialogInfo.lpstrCustomFilter = NULL;
	dialogInfo.nMaxCustFilter = 0;
	dialogInfo.nFilterIndex = 1;
	dialogInfo.lpstrFile = filename;
	dialogInfo.nMaxFile = sizeof( filename );
	dialogInfo.lpstrFileTitle = NULL;
	dialogInfo.nMaxFileTitle = 0;
	dialogInfo.lpstrInitialDir = terrDir;
	dialogInfo.lpstrTitle = "Select a map file";
	dialogInfo.Flags = OFN_FILEMUSTEXIST;
	dialogInfo.lpstrDefExt = "MAP";


	if ( !GetOpenFileName( &dialogInfo ) ) {
		return -1;
	}

	// Extract the path to the directory two above the one containing the selected file
	// (the "root" of the data tree)
	char *p = &filename[ strlen(filename)-1 ];
	while ( (*p != ':') && (*p != '\\') && (p != filename) ) {
		p--;
	}
	p--;
	while ( (*p != ':') && (*p != '\\') && (p != filename) ) {
		p--;
	}
	*p = '\0';
	while ( (*p != ':') && (*p != '\\') && (p != filename) ) {
		p--;
	}
	*p = '\0';



   /*
	* Initialization and setup
	*/
#if defined(_MSC_VER)
	// Set the FPU to 24 bit precision
	_controlfp( _PC_24,   MCW_PC );
    _controlfp( _RC_CHOP, MCW_RC);
#endif


#ifdef GRAPHICS_USE_RES_MGR
	// Initialize the resource manager (compressed data files)
	ResInit( NULL );
	ResCreatePath( terrDir, FALSE );
#endif

	
	// Do device independent setup
	TheWeather = new WeatherMap;
	DeviceIndependentGraphicsSetup( terrDir, objDir, misctexDir );


	// Initialize the device manager
	devmgr.Setup();


	// Intialize the rendering buffer managment class (creats its own window for now)
#ifdef	FULL_SCREEN
	BOOL fullscreen = TRUE;
#else
	BOOL fullscreen = FALSE;
#endif
	devmgr.ChooseDevice( &DriverNumber, &DeviceNumber, &screenWidth );
	device.Setup( DriverNumber, DeviceNumber, screenWidth, screenWidth*3/4, fullscreen );
	win				= device.GetAppWin();
	image			= device.GetImageBuffer();


	// Setup the terrain, environment, and graphics libraries
	DeviceDependentGraphicsSetup( &device );
	sprintf( filename, "%s\\weather\\Weather.RAW", terrDir );
	TheWeather->Load( filename, 1 );

	// Update the time of day (apply our adjustable skew factor) 
	TheTimeManager.SetTime( 12 * 60 * 60 * 1000UL );

	// Setup the database viewpoint object
	wire.LoadAndCreate( "WireTile.GIF", MPR_TI_PALETTE );
	wire.FreeImage();
	TheTerrTextures.SetOverrideTexture( wire.TexHandle() );

	float groundRange	= 15.0f * FEET_PER_KM;
	float weatherRange	= 0.0f;
	viewpoint.Setup( groundRange, 0, 2, 0.0f );

	// Setup the out the window renderer object
	renderer = new RenderWire;
	renderer->Setup( image, &viewpoint );
//	renderer->SetSmoothShadingMode( TRUE );


	// Setup the flight model simulator
	InitialPosition.x = TheDataBase.TheData[0].x;
	InitialPosition.y = TheDataBase.TheData[0].y;
	InitialPosition.z = TheDataBase.TheData[0].z;
	Sim.Setup();
	Sim.position = InitialPosition;


	// Draw everything all at once for now
	Redraw( 0, 0xFFFFFFFF );


	// Find the record with the smallest range to target
	DataPoint	record;
	record.range = 1e6f;
	for (int i=TheDataBase.TheDataLength-1; i>=0; i--) {
		if (TheDataBase.TheData[i].range < record.range)
			record = TheDataBase.TheData[i];
	}


	/*
	* Main Loop
	*/
     while (!done) {

		while (PeekMessage(&msg, win, 0, 0, PM_REMOVE)) {

	    	switch (msg.message) {
    		  case WM_CHAR:
			  	switch (msg.wParam) {
				  case 'q':
	    			PostQuitMessage(0);
					break;
				  case 'u':
					if (-Sim.position.z > 8000.0f) {
					  	Sim.Up(500.0f);
					} else {
					  	Sim.Up(250.0f);
					}
					break;
				  case 'd':
					if (-Sim.position.z > 8000.0f) {
					  	Sim.Down(500.0f);
					} else {
					  	Sim.Down(250.0f);
					}
					break;
				  case 'f':		// 3D rendererpoint forward (along the ground)
					Sim.Forward(3200.0f);
					break;
				  case 'b':		// 3D rendererpoint backward (along the ground)
					Sim.Backward(3200.0f);
					break;
				  case 'r':		// 3D rendererpoint right (along the ground)
					Sim.Rightward(3200.0f);
					break;
				  case 'l':		// 3D rendererpoint left (along the ground)
					Sim.Leftward(3200.0f);
					break;

				  case 'L':		// Turn text labels on/off
					DrawableBSP::drawLabels = !DrawableBSP::drawLabels;
					break;

				  case 'S':		// Save the Sim state to disk
					Sim.Save("SimSave.DAT");
					break;

				  case 's':		// Restore the Sim state from disk
					Sim.Restore("SimSave.DAT");
					break;
				}
				break;

			  case WM_MOVE:
				{
					RECT	rect;
					POINT	clientOrigin = { 0, 0 };

					ClientToScreen( win, &clientOrigin );

					rect.left = clientOrigin.x;
					rect.top = clientOrigin.y;
					rect.right = rect.left + image->targetXres();
					rect.bottom = rect.top + image->targetYres();

					image->UpdateFrontWindowRect( &rect );
				}
				break;

	    	  case WM_QUIT:
	    		done = TRUE;
				break;
			}

	    	TranslateMessage(&msg);
			DispatchMessage(&msg);
		}


	   /*
		* "Simulation" work done here for each frame
		*/

		// Update our simulated eye position and orientation
		Sim.Update( GetTickCount() );

		// Update the renderer's viewpoint
		viewpoint.Update( &Sim.position );

		// Update the camera
		// Always look in direction of flight
		Sim.headYaw = Sim.headPitch = 0.0f;
		Sim.headMatrix = IMatrix;
		ViewerOrientation = Sim.rotation;


	   /*
		* Rendering work done here for each frame
		*/

		// Start the rendering process for the primary display
		renderer->StartFrame();

		// Draw the out the window scene
		renderer->DrawScene( NULL, &ViewerOrientation );

		// Draw all the queued up line segments
		TheLineList.DrawAll( renderer );


		// Print the data from the record of interest
		{
			char message[80];
		
			renderer->SetColor( 0xFFFFFFFF );

			sprintf( message, "Range %0.0f  groundZ %0.0f  targetZ %0.0f  missileZ %0.0f",
						record.range, record.groundZ, record.targetZ, record.z );
			renderer->ScreenText( 10.0f, 10.0f, message );

			sprintf( message, "yawCmd %0.0f  pitchCmd %0.0f", record.yawCmd, record.pitchCmd );
			renderer->ScreenText( 10.0f, 20.0f, message );
		}

		// Finish the frame and display it
		renderer->FinishFrame();
		image->SwapBuffers(renderer->context.rc);
	}

   /*
	* Rendering cleanup done here
	*/

	renderer->Cleanup();
	delete renderer;
	viewpoint.Cleanup();
	Sim.Cleanup();
	wire.FreeAll();

	DeviceDependentGraphicsCleanup( &device );
	DeviceIndependentGraphicsCleanup();

#ifdef GRAPHICS_USE_RES_MGR
	ResExit();
#endif

	device.Cleanup();
	devmgr.Cleanup();

	TheDataBase.FreeData();

	return msg.wParam;
}


// Debug Assert softswitches
#ifdef DEBUG
int shiAssertsOn=1,shiHardCrashOn=0;
#endif
