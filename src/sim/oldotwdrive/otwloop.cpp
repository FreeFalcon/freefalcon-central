#include "stdhdr.h"
#include "Graphics\Include\TOD.h"
#include "Graphics\Include\renderow.h"
#include "Graphics\Include\RViewPnt.h"
#include "Graphics\Include\canvas3d.h"
#include "Graphics\Include\Drawbsp.h"
#include "Graphics\Include\Drawgrnd.h"
#include "Graphics\Include\draw2d.h"
#include "Graphics\Include\TimeMgr.h"
#include "hud.h"
#include "object.h"
#include "mfd.h"
#include "playerrwr.h"
#include "fsound.h"
#include "soundFX.h"
#include "cpmanager.h"
//MI extracting Data
#include "cphsi.h"
#include "ThreadMgr.h"
#include "aircrft.h"
#include "weather.h"
#include "simeject.h"
#include "resource.h"
#include "cpvbounds.h"
#include "playerop.h"
#include "dispopts.h"
#include "sfx.h"
#include "acmi\src\include\acmirec.h"
#include "camp2sim.h"
#include "SimLoop.h"
#include "simdrive.h"
#include "sinput.h"
#include "commands.h"
#include "dogfight.h"
#include "inpfunc.h"
#include "otwdrive.h"
#include "flightData.h"
#include "airframe.h"
#include "fack.h"
#include "campwp.h"
#include "sms.h"
#include "hardpnt.h"
#include "ui\include\uicomms.h"
#include "popmenu.h"
#include "digi.h"
#include "dofsnswitches.h"
#include "navsystem.h"
#include "falclib\include\fakerand.h"
#include "PilotInputs.h"
#include "IvibeData.h"

// OW needed for restoring textures after task switch
#include "graphics\include\texbank.h"
#include "graphics\include\fartex.h"
#include "graphics\include\terrtex.h"

#include "radiosubtitle.h"	// Retro 20Dec2003
#include "missile.h"		// Retro 20Dec2003 for the infobar
#include "profiler.h"	// Retro 20Dec2003

#ifdef DEBUG
#define SHOW_FRAME_RATE	1
extern int gTrailNodeCount, gVoiceCount;

#endif

int tactical_is_training (void);

// normalized coordinates where we start drawing messages TO us
#define MESSAGE_X			(-0.99f)
#define MESSAGE_Y			(0.3f)

// screen position of Pause/X2/X4 text
// Screen coordinates... woohoo!
// X is centered...
#define COMPRESS_Y			(5.0f)
#define COMPRESS_SPACING	(15.0f)

// Chat Box Size stuff
#define CHAT_BOX_HALF_WIDTH		(200.0f)
#define CHAT_BOX_HALF_HEIGHT	(10.0f)
#define CHAT_STR_X				(190.0f)
#define CHAT_STR_Y				(2.0f)

#define SCORENAME_X  (0.6f)
#define SCOREPOINT_X (0.9f)
#define SCORE_Y      (0.8f)

extern int weatherCondition; //JAM 16Nov03
extern long mHelmetIsUR; // hack for UR Helmet detected
extern bool g_bLookCloserFix;
extern bool g_bLensFlare; //THW 2003-11-10 Toggle Lens Flare

extern bool g_bCockpitAutoScale;		//Wombat778 12-12-2003
extern bool g_bRatioHack;				//Wombat778 12-12-2003
extern bool g_bACMIRecordMsgOff;		// JPG 10 Jan 04

extern float g_fMaximumFOV;		// Wombat778 1-15-03
extern float g_fMinimumFOV;		// Wombat778 1-15-03

enum
{
	FLY_BY_CAMERA=0,
	CHASE_CAMERA,
	ORBIT_CAMERA,
	SATELLITE_CAMERA,
	WEAPON_CAMERA,
	TARGET_TO_WEAPON_CAMERA,
	ENEMY_AIRCRAFT_CAMERA,
	FRIENDLY_AIRCRAFT_CAMERA,
	ENEMY_GROUND_UNIT_CAMERA,
	FRIENDLY_GROUND_UNIT_CAMERA,
	INCOMING_MISSILE_CAMERA,
	TARGET_CAMERA,
	TARGET_TO_SELF_CAMERA,
	ACTION_CAMERA,
	RECORDING,
	NUM_CAMERA_VIEWS,
};

// Display strings
extern char CompressionStr[5][20];
extern char CameraLabel[16][40];
extern int lTestFlag1;

// Score strings for Dogfight/Tactical Engagement
extern long gRefreshScoresList;
extern long gScoreColor[10];
extern _TCHAR gScoreName[10][30];
extern _TCHAR gScorePoints[10][10];
extern void MakeTacEngScoreList(); // And their functions
extern void MakeDogfightTopTen(int mode);

// 2000-11-24 ADDED BY S.G. FOR PADLOCKING OPTIONS
#define PLockModeNormal 0
#define PLockModeNearLabelColor 1
#define PLockModeNoSnap 2
#define PLockModeBreakLock 4
extern int g_nPadlockMode;
// END OF ADDED SECTION

//MI
extern bool g_bNoMFDsIn1View;
extern bool g_bShowFlaps;

void CallInputFunction (unsigned long val, int state);

//#define MAKE_MOVIE

extern int MajorVersion;
extern int MinorVersion;
extern int ShowVersion;
extern int ShowFrameRate;
extern SimBaseClass* eyeFlyTgt;
extern int gTotSfx;
extern int numObjsProcessed;
extern int numObjsInDrawList;

extern HWND mainMenuWnd;
extern void* gSharedMemPtr;
extern void *gSharedIntellivibe;

static char tmpStr[128];

// From OTWDrive.cpp
extern unsigned long nextCampObjectHeightRefresh;

// from SimInput
extern unsigned int chatterCount;
extern char chatterStr[256];


static int gSimTimeMax = 0;
static int gCampTimeMax = 0;
static int gGraphicsTimeLastMax = 0;
extern int gSimTime;
extern int gGraphicsTimeLast;
extern int gCampTime;
extern int gAveCampTime;
extern int gAveSimGraphicsTime;
void DebugMemoryReport( RenderOTW *renderer, int frameTime );

extern FalconEntity *gOtwCameraLocation;

char gAcmiStr[11];

LRESULT CALLBACK SimWndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

#ifdef USE_SH_POOLS
extern MEM_POOL gFartexMemPool;
#endif

/* Retro TrackIR stuff.. */
#include "TrackIR.h"				// Retro 26/09/03
extern bool g_bEnableTrackIR;		// Retro 26/09/03
extern TrackIR theTrackIRObject;	// Retro 27/09/03
extern int g_nTrackIRSampleFreq;	// Retro 02/10/03
/* ..ends */

void OTWDriverClass::Cycle(void)
{
	Prof(Cycle);	// Retro 20Dec2003

	frameStart = timeGetTime();
	frameTime = (frameTime * 7 + (frameStart - lastFrame) )/8;
	lastFrame = frameStart;

// OW
#if 0
	if ( frameTime < 10 )
		frameTime = 10;
#else
	if ( frameTime < 10 )
		frameTime = 5;
#endif

	// Should we consider doing SFX detail adjustments to maintain framerate?
	if (fcount++ % 10 == 0)
	{
		// OW
		#ifdef USE_SH_POOLS
		#ifndef NDEBUG
      if (SimulationLoopControl::InSim())
         MemPoolCheck (gFartexMemPool); // Debug code, REMOVE BEFORE FLIGHT
		#endif
		#endif

		// don't do any frame rate adjustments at highest levels
		if ( PlayerOptions.SfxLevel < 4.90 )
		{
			// let's try some on-the-fly sfx lod settings
			float frameRate = 1.0F / (float)(frameTime) * 1000.0F;

			// normalize to 15 FPS
			frameRate = min( frameRate * 0.06666f, 1.0f );

			frameRate *= PlayerOptions.SfxLevel;

			float currDetailLevel = gSfxLOD * 5.0f;

			// average changing the LOD level with the current level
			// at 3:1 weight ratio to new level
			SfxClass::SetLOD( ( frameRate + currDetailLevel * 3.0f ) * 0.25f );
		}
	}


	// This is a kinda annoying thing to do here, but this seems like the safest place for the short term...
	if (SimLibElapsedTime > nextCampObjectHeightRefresh) {

		FalconEntity*	campUnit;
		VuListIterator	vehicleWalker( SimDriver.combinedList );

		// Note:  We could do buildings too, but they generally deaggregate far enough away that it isn't a problem

		// Consider each element of the campaign object sim bubble
		for (campUnit = (FalconEntity*)vehicleWalker.GetFirst(); 
			 campUnit; 
			 campUnit = (FalconEntity*)vehicleWalker.GetNext()) {
			
			// Only deal with battalions for now
			if (campUnit->IsBattalion()) {
				
				// Lets go with an approximation since the x,y position is an approximation anyway
				float groundZ = GetApproxGroundLevel( campUnit->XPos(), campUnit->YPos() );

				// This is kinda annoying -- all we want to do is hammer the Z value, but this is going
				// to do a bunch of grid tree maintenance (and other?) junk.
				campUnit->SetPosition( campUnit->XPos(), campUnit->YPos(), groundZ );

			}
			
		}
		// Set the time for the next refresh
		nextCampObjectHeightRefresh = SimLibElapsedTime + 5000;	// Do it every 5 seconds
	}


	// do we run the acton camera?
	if ( actionCameraMode && actionCameraTimer <= vuxRealTime )
	{
		RunActionCamera();
	}

	// when end flight timer is set, we're ending the game when vuxGameTime
	// gets larger than it
	if ( endFlightTimer != 0 )
	{
		// make sure stuff is set correctly here (ie no hud, etc...)
		SetOTWDisplayMode(ModeChase);

		if (SimDriver.playerEntity && !SimDriver.playerEntity->IsEject())
		{
			// make sure autopilot is on
			AircraftClass *playerAircraft = (AircraftClass *)SimDriver.playerEntity;

			if(playerAircraft->OnGround())
			{
				if (playerAircraft->DBrain()->ATCStatus() < tReqTaxi)
				{
					if(playerAircraft->AutopilotType() != AircraftClass::APOff)
						playerAircraft->SetAutopilot( AircraftClass::APOff );
				}
				else if(playerAircraft->AutopilotType() != AircraftClass::CombatAP)
					playerAircraft->SetAutopilot( AircraftClass::CombatAP );
			}
			else if (playerAircraft->AutopilotType() != AircraftClass::ThreeAxisAP)
			{
				playerAircraft->SetAutopilot( AircraftClass::ThreeAxisAP );
			}
		}

		if (vuxRealTime >= endFlightTimer)
		{
			// We're done, so tell the TheSimLoop to shut us down after this cycle.
			SimulationLoopControl::StopGraphics();
		}
	}

	// handle the flyby camera timer
	if ( flybyTimer != 0 )
	{
		// make sure stuff is set correctly here (ie no hud, etc...)
		// SetOTWDisplayMode(ModeChase);
		mOTWDisplayMode = ModeChase;

		if ( SimLibElapsedTime >= flybyTimer )
		{
			flybyTimer = 0;
		}
	}

	if (exitMenuTimer)
		{
		if (vuxRealTime > exitMenuTimer)
			SetExitMenu(TRUE);
		}

	if(textTimeLeft[0] && textTimeLeft[0] < vuxRealTime)
		ScrollMessages();

	// Check for Deletions
	//F4EnterCriticalSection(objectCriticalSection);
	//DoSfxDrawList();
	//F4LeaveCriticalSection(objectCriticalSection);

	// Check for weather change
	if (!weatherCmd)
	{
	}
	else
	{
		float weatherRange;

		doWeather = 1 - doWeather;
		if (doWeather)
		{
			weatherRange = 32.0F * FEET_PER_KM;
		}
		else
		{
			weatherRange = 0.0F;
		}
//		viewPoint->SetWeatherRange (weatherRange);
		weatherCmd = 0;
	}

	DXContext *pCtx = OTWImage->GetDisplayDevice()->GetDefaultRC();
	HRESULT hr = pCtx->TestCooperativeLevel();

	if(FAILED(hr))
		return;

	if(hr == S_FALSE)
	{
		OTWImage->RestoreAll();
		TheTextureBank.RestoreAll();
		TheTerrTextures.RestoreAll();
		TheFarTextures.RestoreAll();
	}

	// Rendering work done here for each frame
	RenderFrame();

	// TODO:  It would be _REALLY_ nice to get some work done in here before blocking on the HW...
	OTWImage->SwapBuffers(false);
}


void OTWDriverClass::RenderFirstFrame(void)
{
   // Create objects requested before we started
//   MonoPrint("Starting Graphics.. %d\n",vuxRealTime);
//   ClearSfxLists();

//   MonoPrint("Rendering First Frame.. %d\n",vuxRealTime);

   // Check for Additions/Deletions
   //DoSfxDrawList();

//	CallInputFunction(0x52, KEY_DOWN | CTRL_KEY); // VWF: this is a hack for the stupid ECTS show
//	CallInputFunction(0x52, CTRL_KEY);

   RenderFrame();

   // Set to saved states
   if (!tactical_is_training())
      pCockpitManager->LoadCockpitDefaults();
}


void OTWDriverClass::DisplayChatBox(void)
{
	char tempChar;
	float strw;

	// Center Box on screen (Will ALWAYS be same size (in pixels w,h) regardless of screen resolution)
	// BASICALLY, put centerX,centerY at the centerpoint of where you want the box to draw
	float halfHeight = OTWDriver.renderer->TextHeight() * 0.75F;
	float halfWidth = 0.75F;

	OTWDriver.renderer->SetColor( 0x997B5200 );  				// 60% alpha blue
	OTWDriver.renderer->context.RestoreState( STATE_ALPHA_SOLID );

	OTWDriver.renderer->Tri(-halfWidth, -halfHeight,
		                      halfWidth, -halfHeight,
									 halfWidth,  halfHeight);
	OTWDriver.renderer->Tri(-halfWidth, -halfHeight,
		                     -halfWidth,  halfHeight,
									 halfWidth,  halfHeight);

	// Outline Translucent BLUE box
	OTWDriver.renderer->SetColor( 0xFF000000 );  				// black
	OTWDriver.renderer->Line(-halfWidth, -halfHeight, halfWidth, -halfHeight);
	OTWDriver.renderer->Line(-halfWidth, -halfHeight,-halfWidth,  halfHeight);
	OTWDriver.renderer->Line( halfWidth,  halfHeight, halfWidth, -halfHeight);
	OTWDriver.renderer->Line( halfWidth,  halfHeight,-halfWidth,  halfHeight);
	if(vuxRealTime & 0x100)
	{
		tempChar=chatterStr[chatterCount];
		if(chatterStr[0])
		{
			chatterStr[chatterCount]=0;
			strw=(float)OTWDriver.renderer->TextWidth(chatterStr);
			chatterStr[chatterCount]=tempChar;
		}
		else
			strw=0.0f;

		OTWDriver.renderer->SetColor(0xfffefefe); // Not perfect white so color won't change
		OTWDriver.renderer->Line( strw-(halfWidth*0.95F), -halfHeight*0.75F, strw-(halfWidth*0.95F), halfHeight*0.75F);
	}

	if(chatterStr[0])
	{
		OTWDriver.renderer->SetColor(0xff000000); // Not perfect white so color won't change
		OTWDriver.renderer->TextLeftVertical(-halfWidth*0.95F, 0.0F, chatterStr);
	}
}

void OTWDriverClass::ToggleInfoBar()	// Retro 20Dec2003
{
	PlayerOptions.SetInfoBar(!drawInfoBar);
}

//#define AXISTEST
#ifdef AXISTEST
#pragma message("__________AXISTEST defined, remove before release !__________")
#include "simio.h"
struct {
	GameAxis_t theAxis;
	char* theText;
} Axis2Text[] = {
	{ AXIS_PITCH,			"AXIS_PITCH" },
	{ AXIS_YAW,				"AXIS_YAW" },
	{ AXIS_ROLL,			"AXIS_ROLL" },
	{ AXIS_THROTTLE,		"AXIS_THROTTLE" },
	{ AXIS_THROTTLE2,		"AXIS_THROTTLE2" },
	{ AXIS_TRIM_PITCH,		"AXIS_TRIM_PITCH" },
	{ AXIS_TRIM_YAW,		"AXIS_TRIM_YAW" },
	{ AXIS_TRIM_ROLL,		"AXIS_TRIM_ROLL" },
	{ AXIS_BRAKE_LEFT,		"AXIS_BRAKE_LEFT" },
	{ AXIS_FOV,				"AXIS_FOV" },
	{ AXIS_ANT_ELEV,		"AXIS_ANT_ELEV" },
	{ AXIS_CURSOR_X,		"AXIS_CURSOR_X" },
	{ AXIS_CURSOR_Y,		"AXIS_CURSOR_Y" },
	{ AXIS_RANGE_KNOB,		"AXIS_RANGE_KNOB" },
	{ AXIS_COMM_VOLUME_1,	"AXIS_COMM_VOLUME_1" },
	{ AXIS_COMM_VOLUME_2,	"AXIS_COMM_VOLUME_2" },
	{ AXIS_MSL_VOLUME,		"AXIS_MSL_VOLUME" },
	{ AXIS_THREAT_VOLUME,	"AXIS_THREAT_VOLUME" },
	{ AXIS_HUD_BRIGHTNESS,	"AXIS_HUD_BRIGHTNESS" },
	{ AXIS_RET_DEPR,		"AXIS_RET_DEPR" },
	{ AXIS_ZOOM,			"AXIS_ZOOM" }
};
#endif

void OTWDriverClass::DisplayAxisValues()
{
#ifdef AXISTEST
#pragma message("__________AXISTEST defined, remove before release !__________")
extern GameAxisSetup_t AxisSetup[AXIS_MAX];

	OTWDriver.renderer->SetColor(0xFFFF0000);
	
	OTWDriver.renderer->TextLeft(-0.75F,0.9F,"GetAxisValue");
	OTWDriver.renderer->TextLeft(-0.60F,0.9F,"ReadAnalog");
	OTWDriver.renderer->TextLeft(-0.45F,0.9F,"Center");
	OTWDriver.renderer->TextLeft(-0.30F,0.9F,"Cutoff");
	for (int i = AXIS_START; i < AXIS_MAX; i++)
	{
		char tmp[80] = {0};

		sprintf(tmp,"%s:",Axis2Text[i].theText);

		OTWDriver.renderer->TextLeft(-0.95F,0.85F-(i*0.05f),tmp);

		if (IO.AnalogIsUsed((GameAxis_t)i) == true)
		{
			sprintf(tmp,"%i",IO.GetAxisValue((GameAxis_t)i));
			OTWDriver.renderer->TextLeft(-0.70F,0.85F-(i*0.05f),tmp);

			sprintf(tmp,"%f",IO.ReadAnalog((GameAxis_t)i));
			OTWDriver.renderer->TextLeft(-0.60F,0.85F-(i*0.05f),tmp);

			sprintf(tmp,"%i",IO.analog[i].center);
			OTWDriver.renderer->TextLeft(-0.45F,0.85F-(i*0.05f),tmp);

			sprintf(tmp,"%i",IO.analog[i].cutoff);
			OTWDriver.renderer->TextLeft(-0.30F,0.85F-(i*0.05f),tmp);

			if ((i == AXIS_THROTTLE)||(i == AXIS_THROTTLE2))
			{
				if (IO.IsAxisCutOff((GameAxis_t)i) == true)
				{
					sprintf(tmp,"Throttle %i OFF",i-AXIS_THROTTLE+1);
					OTWDriver.renderer->TextLeft(-0.15F,0.85F-(i*0.05f),tmp);
				}
			}
		}
	}

#endif
}

void OTWDriverClass::DisplayInfoBar(void)
{
	Prof(DisplayInfoBar);
	// Center Box on screen (Will ALWAYS be same size (in pixels w,h) regardless of screen resolution)
	// BASICALLY, put centerX,centerY at the centerpoint of where you want the box to draw
	float halfHeight = OTWDriver.renderer->TextHeight() * 1.5F;
	float halfWidth = 1.F;

	OTWDriver.renderer->SetColor( 0x997B5200 );  				// 60% alpha blue
	OTWDriver.renderer->context.RestoreState( STATE_ALPHA_SOLID );

	OTWDriver.renderer->Tri(-halfWidth, -1.F,
							halfWidth, -1.F,
							halfWidth,  -1.F+halfHeight);
	OTWDriver.renderer->Tri(-halfWidth, -1.F,
		                    -halfWidth,  -1.F+halfHeight,
							 halfWidth,  -1.F+halfHeight);

	// Outline Translucent BLUE box
	OTWDriver.renderer->SetColor( 0xFF000000 );  				// black

	OTWDriver.renderer->Line( halfWidth,	-1.F+halfHeight,-halfWidth,	-1.F+halfHeight);

	struct Mode2Cam {
		OTWDisplayMode theMode;
		short	theCamID;
	};

	const static Mode2Cam theModeTable[] = {
		{ModeChase, FLY_BY_CAMERA},
//		{ModeChase, CHASE_CAMERA},	// theCamID doesn´t matter here..
		{ModeOrbit, ORBIT_CAMERA},
		{ModeSatellite, SATELLITE_CAMERA},
		{ModeWeapon, WEAPON_CAMERA},
		{ModeTargetToWeapon, TARGET_TO_WEAPON_CAMERA},
		{ModeAirEnemy, ENEMY_AIRCRAFT_CAMERA},
		{ModeAirFriendly, FRIENDLY_AIRCRAFT_CAMERA},
		{ModeGroundEnemy, ENEMY_GROUND_UNIT_CAMERA},
		{ModeGroundFriendly, FRIENDLY_GROUND_UNIT_CAMERA},
		{ModeIncoming, INCOMING_MISSILE_CAMERA},
		{ModeTarget, TARGET_CAMERA},
		{ModeTargetToSelf, TARGET_TO_SELF_CAMERA}
	};

	int ModeTableSize = sizeof(theModeTable)/sizeof(Mode2Cam);

	short cameraID=0;

	renderer->SetColor (0xff00ff00);	// green

	if (actionCameraMode)
	{
		cameraID = ACTION_CAMERA;
	}
	else
	{
		OTWDisplayMode tmpMode = GetOTWDisplayMode();
		for (int i = 0; i < ModeTableSize; i++)
		{
			if (theModeTable[i].theMode == tmpMode)
			{
				if (tmpMode != ModeChase)
					cameraID = theModeTable[i].theCamID;
				else
				{
					if (flybyTimer)
						cameraID = FLY_BY_CAMERA;
					else
						cameraID = CHASE_CAMERA;
				}
				break;
			}
		}
	}

	// put a line telling what the camera focus is (label)
	if ( otwPlatform &&
		 otwPlatform->drawPointer &&
		 *((DrawableBSP *)otwPlatform->drawPointer)->Label() )
	{
#define LOCAL_STRING_LENGTH 256	// avg length of the below string is 120 chars.. so no sweat, cept maybe for null-pointers ?
		char tmpo[LOCAL_STRING_LENGTH];
		// callsign
		strcpy( tmpo, CameraLabel[cameraID]);
		strcat( tmpo, ": " );
		strcat( tmpo, ((DrawableBSP *)otwPlatform->drawPointer)->Label() );

		// 2 issues here:
		//	a) string could be longer as the locally allocated one (bad thing (tm)) - however that´s unlikely, see above
		//	b) string could be longer than physical screen size.. falcon then displays nothing.. also a bit suboptimal..
		// solution for b) need to get renderer->TextWidth() working, if it is >1 then we don´t add a chunk.. or so..
		if ((!otwPlatform->IsGroundVehicle())&&(!otwPlatform->IsBomb()))
		{
			char tmp[30];

			strcat( tmpo," - heading: " );

			// heading
			float theYaw = ((SimMoverClass*)otwPlatform)->Yaw()*RTD;
			if (theYaw < 0)
				theYaw += 360.F;

			sprintf(tmp,"%3.i",(int)theYaw);
			strcat( tmpo,tmp);
			strcat( tmpo, " degrees, alt: ");

			// height
			sprintf(tmp,"%5.i",(DrawableBSP*)otwPlatform->GetAltitude());
			strcat( tmpo,tmp);
			strcat( tmpo, " ft (BARO), speed: " );

			float fvalue;
			// speed
			if (otwPlatform->IsMissile())
			{
				fvalue = ((MissileClass*)otwPlatform)->Vt() * FTPSEC_TO_KNOTS;
				sprintf(tmp,"%4.f",fvalue);
				strcat( tmpo,tmp);
				strcat( tmpo, " kts (GS), aoa: ");

				fvalue = ((MissileClass*)otwPlatform)->alpha;
				sprintf(tmp,"%+3.1f",fvalue);
				strcat( tmpo,tmp);
				strcat( tmpo, " degrees");
			}
			else
			{
				fvalue = otwPlatform->Kias();
				sprintf(tmp,"%4.f",fvalue);
				strcat( tmpo,tmp);
				strcat( tmpo, " kts (IAS)");
			}

			// power
			sprintf(tmp,", power: %3.f",otwPlatform->PowerOutput()*100.F);
			strcat( tmpo,tmp);
			strcat( tmpo, " percent" );

			// G forces, only for ac
			if (otwPlatform->IsAirplane())
			{
				fvalue = ((AircraftClass*)otwPlatform)->GetNz();
				sprintf(tmp,", %2.1f",fvalue);
				strcat( tmpo, tmp);
				strcat( tmpo, "G, aoa: ");

				fvalue = ((AircraftClass*)otwPlatform)->GetAlpha();
				sprintf(tmp,"%+2.1f",fvalue);
				strcat( tmpo, tmp);
				strcat( tmpo, " degrees");
			}
			
		}
#if 0	// this would display the text/screenwidth ratio..
		char tmp[20];
		
		float blubb = renderer->ScreenTextWidth(tmpo)/(float)renderer->GetXRes();
		sprintf(tmp,"%f",blubb);
		renderer->TextCenter (0.0F, (-1.F+2.F*OTWDriver.renderer->TextHeight()), tmp);
#endif
		renderer->TextCenter (0.0F, (-1.F+1.2F*OTWDriver.renderer->TextHeight()), tmpo);
	}
	else
	{
		renderer->TextCenter (0.0F, (-1.F+1.2F*OTWDriver.renderer->TextHeight()), CameraLabel[cameraID]);
	}
}

void OTWDriverClass::ToggleSubTitles()	// Retro 20Dec2003
{
	drawSubTitles = !drawSubTitles;
}

/* RETRO RADIOMESS LABELS */
void OTWDriverClass::DrawSubTitles(void)  // Retro 16Dec2003 (all)
{
	Prof(DrawSubTitles);
	
	if (radioLabel)
	{
		ColouredSubTitle** theLabels = radioLabel->GetTimeSortedMessages(vuxGameTime);
		if (theLabels)
		{
			int i = 0;
			while (theLabels[i])
			{
				renderer->SetColor(theLabels[i]->theColour);
//				renderer->TextLeft(-0.95F,  (0.90F-i*0.03F), theLabels[i]->theString);
				// Retro 10Jan2004 - lower so that they don´t collide with LEF/TEF display
				renderer->TextLeft(-0.95F,  (0.84F-i*0.03F), theLabels[i]->theString);
				free(theLabels[i]);
				theLabels[i] = 0;
				i++;
			}
			free(theLabels);
			theLabels = 0;
		}
	}
}	// Retro radio label end


// All Y'All, put your TEXT stuff in here... if it is not PART of the F16

#define _DO_OWN_START_FRAME_
// PJW: For lack of a better place... I put this here

void OTWDriverClass::DisplayFrontText(void)
{	
	Prof(DisplayFrontText);			// Retro 15/10/03

	Prof_update(ProfilerActive);	// Retro 16/10/03

	float centerx=DisplayOptions.DispWidth * 0.5f;
	float top;
	float left;
	float bottom;
	float right;
	short chat_cnt;
	int oldFont;

	// SetFont
	oldFont = VirtualDisplay::CurFont();
	VirtualDisplay::SetFont(pCockpitManager->GeneralFont());

	// save the current viewport
	OTWDriver.renderer->GetViewport(&left, &top, &right, &bottom);

#ifdef _DO_OWN_START_FRAME_
	OTWDriver.renderer->StartFrame();
#endif

	// set the new viewport
	OTWDriver.renderer->SetViewport(-1.0, 1.0, 1.0, -1.0);

///////////////////////////////////////////
// Any Text stuff you want drawn below here
//
//
	if(vuxRealTime & 0x200) // Blink every half second
	{
		// Display the PAUSE/X2/X4 compression strings HERE
		if ((vuxRealTime & 0x200) && (remoteCompressionRequests || targetCompressionRatio != 1))
		{
			float offset=0.0f;
			long color;
			// THIS SHOULD BE TRANSLATABLE TEXT
			//
			offset =-(OTWDriver.renderer->ScreenTextWidth(CompressionStr[0]) + COMPRESS_SPACING +
					  OTWDriver.renderer->ScreenTextWidth(CompressionStr[2]) + COMPRESS_SPACING +
					  OTWDriver.renderer->ScreenTextWidth(CompressionStr[3]) + COMPRESS_SPACING) *0.5f;

			if((!targetCompressionRatio) || (!FalconLocalSession->GetReqCompression()) || (remoteCompressionRequests & REMOTE_REQUEST_PAUSE))
			{
				// if ANY compression OR compression requests == 0... Draw PAUSE
				//		if compression == our compression == all requested compressions
				//			Use RED
				//		if compression == our compression != all requested compression
				//			Use YELLOW
				//		if compression != our compression
				//			Use GREEN
				//
				if((!targetCompressionRatio) && ((!FalconLocalSession->GetReqCompression()) && (remoteCompressionRequests & REMOTE_REQUEST_PAUSE) || !gCommsMgr->Online()))
					color=0xff0000ff;
				else if((!FalconLocalSession->GetReqCompression()))
					color=0xff00ff00;
				else
					color=0xff00ffff;
				OTWDriver.renderer->SetColor(color);
				OTWDriver.renderer->ScreenText(centerx+offset,COMPRESS_Y, CompressionStr[0]);
			}
			offset+=OTWDriver.renderer->ScreenTextWidth(CompressionStr[0]) + COMPRESS_SPACING;
			if((targetCompressionRatio == 2) || (FalconLocalSession->GetReqCompression() == 2) || (remoteCompressionRequests & REMOTE_REQUEST_2))
			{
				// if ANY compression OR compression requests == 2... Draw 2X
				//		if compression == our compression == all requested compressions
				//			Use RED
				//		if compression == our compression != all requested compression
				//			Use YELLOW
				//		if compression != our compression
				//			Use GREEN
				//
				if((targetCompressionRatio == 2) && ((FalconLocalSession->GetReqCompression() == 2) && (remoteCompressionRequests & REMOTE_REQUEST_2) || !gCommsMgr->Online()))
					color=0xff0000ff;
				else if((FalconLocalSession->GetReqCompression() == 2))
					color=0xff00ff00;
				else
					color=0xff00ffff;
				OTWDriver.renderer->SetColor(color);
				OTWDriver.renderer->ScreenText(centerx+offset,COMPRESS_Y, CompressionStr[2]);
			}
			// JB 010109 offset+=OTWDriver.renderer->ScreenTextWidth(CompressionStr[2]) + COMPRESS_SPACING; 
			if((targetCompressionRatio == 4) || (FalconLocalSession->GetReqCompression() == 4) || (remoteCompressionRequests & REMOTE_REQUEST_4))
			{
				// if ANY compression OR compression requests > 2... Draw 4X
				//		if compression == our compression == all requested compressions
				//			Use RED
				//		if compression == our compression != all requested compression
				//			Use YELLOW
				//		if compression != our compression
				//			Use GREEN
				//
				if((targetCompressionRatio == 4) && ((FalconLocalSession->GetReqCompression() == 4) && (remoteCompressionRequests & REMOTE_REQUEST_4) || !gCommsMgr->Online()))
					color=0xff0000ff;
				else if((FalconLocalSession->GetReqCompression() == 4))
					color=0xff00ff00;
				else
					color=0xff00ffff;
				OTWDriver.renderer->SetColor(color);
				OTWDriver.renderer->ScreenText(centerx+offset,COMPRESS_Y, CompressionStr[3]);
			}

			// JB 010109
			//offset+=OTWDriver.renderer->ScreenTextWidth(CompressionStr[3]) + COMPRESS_SPACING;
			if((targetCompressionRatio == 8) || (FalconLocalSession->GetReqCompression() == 8) || (remoteCompressionRequests & REMOTE_REQUEST_8))
			{
				// if ANY compression OR compression requests > 2... Draw 8X
				//		if compression == our compression == all requested compressions
				//			Use RED
				//		if compression == our compression != all requested compression
				//			Use YELLOW
				//		if compression != our compression
				//			Use GREEN
				//
				if((targetCompressionRatio == 8) && ((FalconLocalSession->GetReqCompression() == 8) && (remoteCompressionRequests & REMOTE_REQUEST_8) || !gCommsMgr->Online()))
					color=0xff0000ff;
				else if((FalconLocalSession->GetReqCompression() == 8))
					color=0xff00ff00;
				else
					color=0xff00ffff;
				OTWDriver.renderer->SetColor(color);
				OTWDriver.renderer->ScreenText(centerx+offset,COMPRESS_Y, "x8");
			}
			//offset+=OTWDriver.renderer->ScreenTextWidth("x8") + COMPRESS_SPACING;
			if((targetCompressionRatio == 16) || (FalconLocalSession->GetReqCompression() == 16) || (remoteCompressionRequests & REMOTE_REQUEST_16))
			{
				// if ANY compression OR compression requests > 2... Draw 16X
				//		if compression == our compression == all requested compressions
				//			Use RED
				//		if compression == our compression != all requested compression
				//			Use YELLOW
				//		if compression != our compression
				//			Use GREEN
				//
				if((targetCompressionRatio == 16) && ((FalconLocalSession->GetReqCompression() == 16) && (remoteCompressionRequests & REMOTE_REQUEST_16) || !gCommsMgr->Online()))
					color=0xff0000ff;
				else if((FalconLocalSession->GetReqCompression() == 16))
					color=0xff00ff00;
				else
					color=0xff00ffff;
				OTWDriver.renderer->SetColor(color);
				OTWDriver.renderer->ScreenText(centerx+offset,COMPRESS_Y, "x16");
			}
			//offset+=OTWDriver.renderer->ScreenTextWidth("x16") + COMPRESS_SPACING;
			if((targetCompressionRatio == 32) || (FalconLocalSession->GetReqCompression() == 32) || (remoteCompressionRequests & REMOTE_REQUEST_32))
			{
				// if ANY compression OR compression requests > 2... Draw 32X
				//		if compression == our compression == all requested compressions
				//			Use RED
				//		if compression == our compression != all requested compression
				//			Use YELLOW
				//		if compression != our compression
				//			Use GREEN
				//
				if((targetCompressionRatio == 32) && ((FalconLocalSession->GetReqCompression() == 32) && (remoteCompressionRequests & REMOTE_REQUEST_32) || !gCommsMgr->Online()))
					color=0xff0000ff;
				else if((FalconLocalSession->GetReqCompression() == 32))
					color=0xff00ff00;
				else
					color=0xff00ffff;
				OTWDriver.renderer->SetColor(color);
				OTWDriver.renderer->ScreenText(centerx+offset,COMPRESS_Y, "x32");
			}
			//offset+=OTWDriver.renderer->ScreenTextWidth("x32") + COMPRESS_SPACING;
			if((targetCompressionRatio == 64) || (FalconLocalSession->GetReqCompression() == 64) || (remoteCompressionRequests & REMOTE_REQUEST_64))
			{
				// if ANY compression OR compression requests > 2... Draw 64X
				//		if compression == our compression == all requested compressions
				//			Use RED
				//		if compression == our compression != all requested compression
				//			Use YELLOW
				//		if compression != our compression
				//			Use GREEN
				//
				if((targetCompressionRatio == 64) && ((FalconLocalSession->GetReqCompression() == 64) && (remoteCompressionRequests & REMOTE_REQUEST_64) || !gCommsMgr->Online()))
					color=0xff0000ff;
				else if((FalconLocalSession->GetReqCompression() == 64))
					color=0xff00ff00;
				else
					color=0xff00ffff;
				OTWDriver.renderer->SetColor(color);
				OTWDriver.renderer->ScreenText(centerx+offset,COMPRESS_Y, "x64");
			}
			//offset+=OTWDriver.renderer->ScreenTextWidth("x64") + COMPRESS_SPACING;
			if((targetCompressionRatio == 128) || (FalconLocalSession->GetReqCompression() == 128) || (remoteCompressionRequests & REMOTE_REQUEST_128))
			{
				// if ANY compression OR compression requests > 2... Draw 128X
				//		if compression == our compression == all requested compressions
				//			Use RED
				//		if compression == our compression != all requested compression
				//			Use YELLOW
				//		if compression != our compression
				//			Use GREEN
				//
				if((targetCompressionRatio == 128) && ((FalconLocalSession->GetReqCompression() == 128) && (remoteCompressionRequests & REMOTE_REQUEST_128) || !gCommsMgr->Online()))
					color=0xff0000ff;
				else if((FalconLocalSession->GetReqCompression() == 128))
					color=0xff00ff00;
				else
					color=0xff00ffff;
				OTWDriver.renderer->SetColor(color);
				OTWDriver.renderer->ScreenText(centerx+offset,COMPRESS_Y, "x128");
			}
			//offset+=OTWDriver.renderer->ScreenTextWidth("x128") + COMPRESS_SPACING;
			if((targetCompressionRatio == 256) || (FalconLocalSession->GetReqCompression() == 256) || (remoteCompressionRequests & REMOTE_REQUEST_256))
			{
				// if ANY compression OR compression requests > 2... Draw 256X
				//		if compression == our compression == all requested compressions
				//			Use RED
				//		if compression == our compression != all requested compression
				//			Use YELLOW
				//		if compression != our compression
				//			Use GREEN
				//
				if((targetCompressionRatio == 256) && ((FalconLocalSession->GetReqCompression() == 256) && (remoteCompressionRequests & REMOTE_REQUEST_256) || !gCommsMgr->Online()))
					color=0xff0000ff;
				else if((FalconLocalSession->GetReqCompression() == 256))
					color=0xff00ff00;
				else
					color=0xff00ffff;
				OTWDriver.renderer->SetColor(color);
				OTWDriver.renderer->ScreenText(centerx+offset,COMPRESS_Y, "x256");
			}
			//offset+=OTWDriver.renderer->ScreenTextWidth("x256") + COMPRESS_SPACING;
			if((targetCompressionRatio == 512) || (FalconLocalSession->GetReqCompression() == 512) || (remoteCompressionRequests & REMOTE_REQUEST_512))
			{
				// if ANY compression OR compression requests > 2... Draw 512X
				//		if compression == our compression == all requested compressions
				//			Use RED
				//		if compression == our compression != all requested compression
				//			Use YELLOW
				//		if compression != our compression
				//			Use GREEN
				//
				if((targetCompressionRatio == 512) && ((FalconLocalSession->GetReqCompression() == 512) && (remoteCompressionRequests & REMOTE_REQUEST_512) || !gCommsMgr->Online()))
					color=0xff0000ff;
				else if((FalconLocalSession->GetReqCompression() == 512))
					color=0xff00ff00;
				else
					color=0xff00ffff;
				OTWDriver.renderer->SetColor(color);
				OTWDriver.renderer->ScreenText(centerx+offset,COMPRESS_Y, "x512");
			}
			//offset+=OTWDriver.renderer->ScreenTextWidth("x512") + COMPRESS_SPACING;
			if((targetCompressionRatio == 1024) || (FalconLocalSession->GetReqCompression() == 1024) || (remoteCompressionRequests & REMOTE_REQUEST_1024))
			{
				// if ANY compression OR compression requests > 2... Draw 1024X
				//		if compression == our compression == all requested compressions
				//			Use RED
				//		if compression == our compression != all requested compression
				//			Use YELLOW
				//		if compression != our compression
				//			Use GREEN
				//
				if((targetCompressionRatio == 1024) && ((FalconLocalSession->GetReqCompression() == 1024) && (remoteCompressionRequests & REMOTE_REQUEST_1024) || !gCommsMgr->Online()))
					color=0xff0000ff;
				else if((FalconLocalSession->GetReqCompression() == 1024))
					color=0xff00ff00;
				else
					color=0xff00ffff;
				OTWDriver.renderer->SetColor(color);
				OTWDriver.renderer->ScreenText(centerx+offset,COMPRESS_Y, "x1024");
			}
			//offset+=OTWDriver.renderer->ScreenTextWidth("x1024") + COMPRESS_SPACING;
			// JB 010109
		}

		if (gameCompressionRatio && !SimDriver.MotionOn())
		{
			renderer->SetColor (0xff0000ff);
			renderer->ScreenText (centerx - OTWDriver.renderer->ScreenTextWidth(CompressionStr[4]) * 0.5f, COMPRESS_Y, CompressionStr[4]);
		}
	}

	if (!g_bACMIRecordMsgOff && gACMIRec.IsRecording())
		{
			int pct;
			int j;

			renderer->SetColor (0xff0000ff);
			renderer->TextCenter (0.0F, 0.95F, CameraLabel[RECORDING]);

			// get the percent tape is full....
			pct = gACMIRec.PercentTapeFull();
			if ( pct > 10 )
				pct = 10;
			for ( j = 0; j < pct; j++ )
				gAcmiStr[j] = '+';
			renderer->TextCenter (0.0F, 0.92F, gAcmiStr);
		}

	if(showFrontText & (SHOW_TE_SCORES|SHOW_DOGFIGHT_SCORES))
	{
		float centerX = DisplayOptions.DispWidth / 2.0F;
		float centerY =	DisplayOptions.DispHeight / 2.0F;
		short i;
		float x,y;
		float w,h;

		x=centerX + centerX * (SCORENAME_X - 0.02f);
		y=centerY - centerY * (SCORE_Y + 0.1f);
		w=centerX + (centerX * (SCOREPOINT_X + 0.02f)) - x;
		h=centerY * 12 * 0.05f;

		OTWDriver.renderer->SetColor( 0x997B5200 );  				// 60% alpha blue
		OTWDriver.renderer->context.RestoreState( STATE_ALPHA_SOLID );

		OTWDriver.renderer->Render2DTri( x,y,  x+w,y,  x+w,y+h);
		OTWDriver.renderer->Render2DTri( x,y,  x,y+h,  x+w,y+h);

		// Outline Translucent BLUE box
		OTWDriver.renderer->SetColor( 0xFF000000 );  				// black
		OTWDriver.renderer->Render2DLine( x,y,x+w,y );
		OTWDriver.renderer->Render2DLine( x,y+h,x+w,y+h );
		OTWDriver.renderer->Render2DLine( x,y,x,y+h );
		OTWDriver.renderer->Render2DLine( x+w,y,x+w,y+h );

		if(showFrontText & SHOW_DOGFIGHT_SCORES)
		{
			renderer->SetColor(0xfffefefe); // not quite white, so the color won't change
			renderer->TextCenter(SCORENAME_X + (SCOREPOINT_X - SCORENAME_X) * 0.5f,  SCORE_Y - (float)(-1) * 0.05f,"Sierra Hotel");
			if(gRefreshScoresList)
			{
				MakeDogfightTopTen(SimDogfight.GetGameType());
				gRefreshScoresList=0;
			}
		}
		else if(showFrontText & SHOW_TE_SCORES)
		{
			renderer->SetColor(0xfffefefe); // not quite white, so the color won't change
			renderer->TextCenter(SCORENAME_X + (SCOREPOINT_X - SCORENAME_X) * 0.5f,  SCORE_Y - (float)(-1) * 0.05f,"Game Over");
			if(gRefreshScoresList)
			{
				MakeTacEngScoreList();
				gRefreshScoresList=0;
			}
		}
		for(i=0;i<10;i++)
		{
//			renderer->SetColor(gScoreColor[i]); // Not set yet
			renderer->SetColor(0xfffefefe); // not quite white, so the color won't change

			if(gScoreName[i][0])
			{
				renderer->TextLeft(SCORENAME_X,  SCORE_Y - (float)i * 0.05f,gScoreName[i]);
				renderer->TextRight(SCOREPOINT_X,  SCORE_Y - (float)i * 0.05f,gScorePoints[i]);
			}
		}
	}

	// Display any Text Messages sent TO me
	if(showFrontText & (SHOW_MESSAGES) && textMessage[0][0]) // Check to see if there are any
	{
		chat_cnt=0;
		while(chat_cnt < MAX_CHAT_LINES && textMessage[chat_cnt][0])
		{
			if(TheHud)
				renderer->SetColor (TheHud->GetHudColor());
			else
				renderer->SetColor (0xff00ff00);
			renderer->TextLeft(MESSAGE_X,  MESSAGE_Y - (float)chat_cnt * 0.05f,textMessage[chat_cnt]);
			chat_cnt++;
		}
	}

	// Display a ChatBox if I am typeing a message
	//if(flags & ShowChatBox) // put this check in
	if(showFrontText & (SHOW_CHATBOX))
		DisplayChatBox();

	if ((drawInfoBar)&&(!DisplayInCockpit()))		// Retro 16Dec2003
	{
		DisplayInfoBar();							// Retro 16Dec2003
	}
	else											// Retro 16Dec2003
	{
		// display text for some camera settings
		// TODO:  This should be a string table, not a slew of "if"s
		if ( !DisplayInCockpit() )
		{
			short cameraID=0;

			renderer->SetColor (0xff00ff00);
			if ( actionCameraMode)
			{
				cameraID=ACTION_CAMERA;
			}
			else if (GetOTWDisplayMode() == ModeChase && flybyTimer )
			{
				cameraID=FLY_BY_CAMERA;
			}
			else if (GetOTWDisplayMode() == ModeChase && !flybyTimer )
			{
				cameraID=CHASE_CAMERA;
			}
			else if (GetOTWDisplayMode() == ModeOrbit )
			{
				cameraID=ORBIT_CAMERA;
			}
			else if (GetOTWDisplayMode() == ModeSatellite )
			{
				cameraID=SATELLITE_CAMERA;
			}
			else if (GetOTWDisplayMode() == ModeWeapon )
			{
				cameraID=WEAPON_CAMERA;
			}
			else if (GetOTWDisplayMode() == ModeTargetToWeapon )
			{
				cameraID=TARGET_TO_WEAPON_CAMERA;
			}
			else if (GetOTWDisplayMode() == ModeAirEnemy )
			{
				cameraID=ENEMY_AIRCRAFT_CAMERA;
			}
			else if (GetOTWDisplayMode() == ModeAirFriendly )
			{
				cameraID=FRIENDLY_AIRCRAFT_CAMERA;
			}
			else if (GetOTWDisplayMode() == ModeGroundEnemy )
			{
				cameraID=ENEMY_GROUND_UNIT_CAMERA;
			}
			else if (GetOTWDisplayMode() == ModeGroundFriendly )
			{
				cameraID=FRIENDLY_GROUND_UNIT_CAMERA;
			}
			else if (GetOTWDisplayMode() == ModeIncoming )
			{
				cameraID=INCOMING_MISSILE_CAMERA;
			}
			else if (GetOTWDisplayMode() == ModeTarget )
			{
				cameraID=TARGET_CAMERA;
			}
			else if (GetOTWDisplayMode() == ModeTargetToSelf )
			{
				cameraID=TARGET_TO_SELF_CAMERA;
			}

			// put a line telling what the camera focus is (label)
			if ( otwPlatform &&
				 otwPlatform->drawPointer &&
				 *((DrawableBSP *)otwPlatform->drawPointer)->Label() )
			{
				strcpy( tmpStr, CameraLabel[cameraID]);
				strcat( tmpStr, ": " );
				strcat( tmpStr, ((DrawableBSP *)otwPlatform->drawPointer)->Label() );
				renderer->TextCenter (0.0F, 0.89F, tmpStr);
			}
			else
				renderer->TextCenter (0.0F, 0.89F, CameraLabel[cameraID]);
		}
	}		// Retro 16Dec2003

	if (showPos)
		ShowPosition();
	if (showAero)
	    ShowAerodynamics();
	//if (g_bShowFlaps) TJL 11/09/03 Show Flaps
	if (!showFlaps)
	    ShowFlaps();

	if (ShowVersion)
		ShowVersionString();

	if (getNewCameraPos)
		GetUserPosition();

	if (showEngine)		// Retro 1Feb2004
		ShowEngine();	// Retro 1Feb2004

#ifdef SHOW_FRAME_RATE
	if (ShowFrameRate)
	{

		if ( gSimTime > gSimTimeMax )
			gSimTimeMax = gSimTime;
		if ( gCampTime > gCampTimeMax )
			gCampTimeMax = gCampTime;
		if ( gGraphicsTimeLast > gGraphicsTimeLastMax )
			gGraphicsTimeLastMax = gGraphicsTimeLast;

		renderer->SetColor (0xfff0f0f0); // Keeps this color from randomly changing
#ifdef MEM_DEBUG
		DebugMemoryReport( renderer, frameTime );
#else
		extern int vuentitycount, vuentitypeak;
		extern int vumessagecount, vumessagepeakcount;

		sprintf (tmpStr, 
"%.2f SFX=%3d PObjs=%3d DObjs=%3d Camp T=%2d AVE=%2d MAX=%4d; Sim T=%2d MAX=%4d; Graphics T=%2d AVE=%2d MAX=%4d;TrailNodes=%d Voices=%d",
			1.0F / (float)(frameTime) * 1000.0F,
			gTotSfx,
			numObjsProcessed,
			numObjsInDrawList,
			gCampTime,
			gAveCampTime,
			gCampTimeMax,
			gSimTime,
			gSimTimeMax,
			gGraphicsTimeLast,
			gAveSimGraphicsTime,
			gGraphicsTimeLastMax,
			gTrailNodeCount,
			gVoiceCount);

		renderer->TextLeft (-0.95F,  0.95F, tmpStr);
		sprintf (tmpStr, "Textures Terr %4d, far %4d Lods %d LodTex %d, Entities cur %4d max %4d VuMessageCount %4d MAX=%4d",
			TheTerrTextures.LoadedTextureCount,
			TheFarTextures.LoadedTextureCount,
			ObjectLOD::lodsLoaded, TextureBankClass::textureCount,
			vuentitycount, vuentitypeak,
			vumessagecount, vumessagepeakcount);
		renderer->TextLeft (-0.95F,  0.90F, tmpStr);
#endif
	}
	else
	{
		gCampTimeMax = 0;
		gSimTimeMax = 0;
		gGraphicsTimeLastMax = 0;
	}
#else
	if (ShowFrameRate)
	{
		int tmp = lTestFlag1;

		renderer->SetColor (0xfff0f0f0); // Keeps this color from randomly changing

		lTestFlag1 = 0;
		sprintf (tmpStr, "FPS %5.1f", 1.0F / (float)(frameTime) * 1000.0F );
		VirtualDisplay::SetFont (2);
		renderer->TextLeft (-0.95F,  0.95F, tmpStr, 2);
		lTestFlag1 = tmp;
	}
#endif

	if (drawSubTitles)	 // Retro 16Dec2003
	{
		DrawSubTitles();  // Retro 16Dec2003
	}
	
#ifdef Prof_ENABLED
	DisplayProfilerText();	// Retro 21Dec2003
#endif

#ifdef AXISTEST
#pragma message("__________AXISTEST defined, remove before release !__________")
	DisplayAxisValues();	// Retro 1Jan2004
#endif

//	if ((SimDriver.playerEntity) && (SimDriver.playerEntity->IsSetFalcFlag (FEC_INVULNERABLE)) && (vuxRealTime & 0x200))
//	{
//		renderer->SetColor (0xfffefefe); // Keeps this color from randomly changing
//		renderer->TextCenter (0.0F, 0.5F, "Invincible");
//	}

//
//
// No more drawing AFTER this
///////////////////////////////////////////
#ifdef _DO_OWN_START_FRAME_
	OTWDriver.renderer->FinishFrame();
#endif
	// restore the old viewport
	OTWDriver.renderer->SetViewport(left, top, right, bottom);
	VirtualDisplay::SetFont(oldFont);
}

/*****************************************************************************/
//	Retro 21Dec2003
/*****************************************************************************/
void OTWDriverClass::ToggleProfilerDisplay(void)
{
	DisplayProfiler = !DisplayProfiler;
}

/*****************************************************************************/
//	Retro 21Dec2003
/*****************************************************************************/
void OTWDriverClass::ToggleProfilerActive(void)
{
	ProfilerActive = !ProfilerActive;
}

/*****************************************************************************/
//	Retro 21Dec2003
//	requests a report from the profiler and displays it
//	also displays a 'virtual cursor' so that user can navigate the call graph
/*****************************************************************************/
void OTWDriverClass::DisplayProfilerText(void)
{
#ifdef Prof_ENABLED	// Retro 15/10/03
	if (DisplayProfiler)
	{
		Prof(DisplayProfilerOutput_Scope);	// Retro 15/10/03
		
#define MAX_LINE_NUM 35

		char** theReport = Prof_dumpOverlay();

		if (theReport)
		{
			int virtualCursor = Prof_get_cursor();
			renderer->SetColor (0xffff0000); // blue

#define XPOS_NAME	-0.95f
#define XPOS_SELF	-0.50f
#define XPOS_HIER	-0.25f
#define XPOS_COUNT	-0.1f

#define YPOS_START 0.90f
#define YPOS_DELTA 0.05f

			float xpos = XPOS_NAME;
			float ypos = YPOS_START;

			// first line is the title, with info about frametime, avg fps etc
			// second line can be speedstep-like timer warning
			for (int titleIndex = 0; titleIndex < 2; titleIndex++)
			{		
				if (theReport[titleIndex])
				{
					renderer->TextLeft (xpos,  ypos, theReport[titleIndex]);
					ypos -= YPOS_DELTA;
					free(theReport[titleIndex]);
					theReport[titleIndex] = 0;
				}
			}
			// Retro 26Dec2003 - fixed display of "speedstep-like timer" warning
			for (int i = 2; i < (MAX_LINE_NUM*4)+2; i++)
			{
				if (theReport[i])
				{
					if (i == (virtualCursor*4)+2)
						renderer->SetColor (0xFF0000FF);		// Retro, red

					switch (i%4)
					{
					case 1: xpos = XPOS_COUNT; break;
					case 2: xpos = XPOS_NAME;	ypos -= YPOS_DELTA; break;
					case 3: xpos = XPOS_SELF; break;
					case 0: xpos = XPOS_HIER; break;
					}

					if (i == (virtualCursor*4)+6)
						renderer->SetColor (0xffff0000);		// Retro, get back to blue

					renderer->TextLeft (xpos,  ypos, theReport[i]);
				}
				free(theReport[i]);
				theReport[i] = 0;
			}
			free(theReport);
			theReport = 0;
		}
	}
#endif	// Prof_ENABLED - Retro 15/10/03
}

#include "simio.h"			 	// Retro 31Dec2003
extern SIMLIB_IO_CLASS IO;	 	// Retro 31Dec2003

void OTWDriverClass::RenderFrame(void)
{
	Prof(RenderFrame);	// Retro 15/10/03

	int				i;
	float			dT;
	static int		count = 0;
	float			top, bottom;
	ViewportBounds	viewportBounds;
	Tpoint			viewPos;
	int camCount;
	int oldFont;


	// convert frame loop time to secs from ms
	dT = (float)frameTime * 0.001F;
	// clamp dT
	if ( dT < 0.01f )
		dT = 0.01f;
	else if ( dT > 0.5f )
		dT = 0.5f;

	BOOL okToDoCockpitStuff = TRUE; // 2002-02-15 MOVED FROM BELOW BY S.G. I need to set it to false if the current otwPlatform is not SimDriver.playerEntity

extern bool MouseMenuActive;
	// Retro 31Dec2003 start
	// the position here might not be the best.. has to coordinated with the g_bLookCloserFix I think..
	// Should be coordinated with wombat´s keypresses: if this active
	//	is used, then the keypresses (and maybe the 'l' key) should
	//	be deactivated
	if ((!actionCameraMode)&&(!MouseMenuActive))	// Retro 20Feb2004 - no FOV control in actioncam and when the 'Exit mission' menu is active
	{
		if (IO.AnalogIsUsed(AXIS_FOV))
		{
			//Wombat778 1-15-03 rewrote slighty so that center of axis is the middle of the FOV range
			float theFOV = ( g_fMaximumFOV - g_fMinimumFOV ) * IO.GetAxisValue(AXIS_FOV) / 15000.;	
			OTWDriver.SetFOV( ( g_fMinimumFOV + theFOV ) * DTR);
		}
	}
	 // Retro 31Dec2003 end

#if 0
	
	// If we're sitting in our own aircraft...
	if(	otwPlatform && otwPlatform == SimDriver.playerEntity)
	{
		if(GetOTWDisplayMode() == ModePadlockF3 || (GetOTWDisplayMode() == Mode3DCockpit && mDoSidebar == TRUE))
		{
			pPadlockCPManager->SetNextView();
		}
		else if (GetOTWDisplayMode() == Mode2DCockpit)
		{
			pCockpitManager->SetNextView();
			OTWDriver.SetCameraPanTilt(pCockpitManager->GetPan(), pCockpitManager->GetTilt());
		}
	}


#else

	//JAM 17Dec03 - Tidied up a little
	// If we're sitting in our own aircraft...
    if(otwPlatform && otwPlatform == SimDriver.playerEntity)
	{
		if(GetOTWDisplayMode() == ModePadlockF3 || (GetOTWDisplayMode() == Mode3DCockpit && mDoSidebar == TRUE))
		{
	        pPadlockCPManager->SetNextView();
        }
        else if (GetOTWDisplayMode() == Mode2DCockpit)
		{
			pCockpitManager->SetNextView();
//dpc LookCloserFix start
//If Hud is present in 2D Cockpit view, make look closer zoom through the boresight cross in the HUD
//This assumes that normal FOV = 60 deg., narrow FOV = 20 deg.
//and that cockpit designer took time to align hud position correctly
//(boresight cross should be drawn exactly at 0 deg. pan and 0 deg. tilt pixel
//			OTWDriver.SetCameraPanTilt(pCockpitManager->GetPan(), pCockpitManager->GetTilt());
			if(g_bLookCloserFix)
			{
	            float pan = pCockpitManager->GetPan();
				float tilt = pCockpitManager->GetTilt();
				//Wombat778 10-31-2003	shouldnt be necessary anymore
                //extern int narrowFOV;
				extern ViewportBounds hudViewportBounds;
                float normHFOV = 60.0F * DTR;
                //float narrowHFOV = 20.0F * DTR;
				//Wombat778 9-27-2003 Modified to allow fix to work with the current FOV (not 20 degrees)
				float narrowHFOV = OTWDriver.GetFOV();
                float normVFOV = 2 * (float)atan2( 3.0f * tan(normHFOV/2), 4.0f );
                float narrowVFOV = 2 * (float)atan2( 3.0f * tan(narrowHFOV/2), 4.0f );
                float ratioH = normHFOV/narrowHFOV;
                float ratioV = normVFOV/narrowVFOV;

				//Wombat778 10-31-2003 changed looking at narrowFOV to checking actual FOV
                if(OTWDriver.GetFOV() != 60.0f && pCockpitManager->ShowHud()
                && pCockpitManager->GetViewportBounds(&hudViewportBounds,BOUNDS_HUD))
                {
					//make sure we don't div with 0
                    if(fabs(pan)> 0.5 * DTR)
						pan = pan/ratioH * (float)( tan(pan) * tan(narrowHFOV/2) / (tan(pan/ratioH)*tan(normHFOV/2)));
                    else
						pan = pan/ratioH;

                    if(fabs(tilt)> 0.5 * DTR)
						tilt = tilt/ratioV * (float)( tan(tilt) * tan(narrowVFOV/2) / (tan(tilt/ratioV)*tan(normVFOV/2)));  
                    else
					    tilt = tilt/ratioV;
                }
   
				OTWDriver.SetCameraPanTilt(pan, tilt);
			}
			else
				OTWDriver.SetCameraPanTilt(pCockpitManager->GetPan(), pCockpitManager->GetTilt());
//dpc LookCloserFix end
		}
	}
// 2002-02-15 ADDED BY S.G. If the otwPlatform is NOT us, don't do its cockpit stuff since I'll never get in his seat
	else
		okToDoCockpitStuff = FALSE;

#endif


	// Find new cockpit if needed
	if (viewStep != 0)
	{
		lastotwPlatform = otwPlatform;
		FindNewOwnship();

	/*
	** edg note: I don't think we need eject cam anymore
		if(ejectCam)
		{
			SetOTWDisplayMode(ModeOrbit);
			prevChase = 0;
			ejectCam = 0;
		}
	*/
	}
	// 2002-02-15 ADDED BY S.G. If the otwPlatform is NOT us, don't do its cockpit stuff since I'll never get in his seat
	if(	!otwPlatform || otwPlatform != SimDriver.playerEntity)
		okToDoCockpitStuff = FALSE;

	if((GetOTWDisplayMode() == ModePadlockF3 || GetOTWDisplayMode() == Mode3DCockpit) && mDoSidebar)
	{
	// Set viewport for Padlock
		renderer->SetViewport(padlockWindow[0][0], padlockWindow[0][1], padlockWindow[0][2], padlockWindow[0][3]);
	}
	else if(otwPlatform == SimDriver.playerEntity && pCockpitManager)
	{
		renderer->SetViewport(-1.0F, 1.0F, 1.0F, pCockpitManager->GetCockpitMaskTop());
	}
	else
	{
		renderer->SetViewport(-1.0F, 1.0F, 1.0F, -1.0F);
	}

	// This updates the positions of all the drawable objects
	UpdateVehicleDrawables();

	// Add the F3 Padlock if required
	if (GetOTWDisplayMode() == ModePadlockF3 && (otwPlatform == SimDriver.playerEntity))
	{
		if (tgtStep)
		{
			Padlock_FindNextPriority(FALSE);
			tgtStep = 0;
		}
	}


//	BOOL okToDoCockpitStuff = TRUE; 2002-02-15 MOVED ABOVE BY S.G. I need to set it to false if the current otwPlatform is not SimDriver.playerEntity

	// Update flight instrument data used by HUD and cockpit
	if (otwPlatform && otwPlatform->IsAirplane() && okToDoCockpitStuff) // 2002-02-15 MODIFIED BY S.G. Added okToDoCockpitStuff so this is done only when the cockpit is from our plane
	{
		FackClass* mFaults = ((AircraftClass*)otwPlatform)->mFaults;
		float tmpVal, tmpVal2;//TJL 01/14/04 multi-engine

	// Check for massive hardware failure
		if (!(mFaults &&
			  mFaults->GetFault(FaultClass::cadc_fault) &&
			  mFaults->GetFault(FaultClass::ins_fault) &&
			  mFaults->GetFault(FaultClass::gps_fault)))
		{
			cockpitFlightData.x = otwPlatform->XPos();
			cockpitFlightData.y = otwPlatform->YPos();
			cockpitFlightData.z = otwPlatform->ZPos();
			cockpitFlightData.xDot = otwPlatform->XDelta();
			cockpitFlightData.yDot = otwPlatform->YDelta();
			cockpitFlightData.zDot = otwPlatform->ZDelta();
			cockpitFlightData.alpha = ((AircraftClass*)otwPlatform)->GetAlpha();
			cockpitFlightData.beta = ((AircraftClass*)otwPlatform)->GetBeta();
			cockpitFlightData.gamma = ((AircraftClass*)otwPlatform)->GetGamma();
			cockpitFlightData.pitch = otwPlatform->Pitch();
			cockpitFlightData.roll = otwPlatform->Roll();
			cockpitFlightData.yaw = otwPlatform->Yaw();
			cockpitFlightData.mach = ((AircraftClass*)otwPlatform)->af->mach;
			cockpitFlightData.kias = otwPlatform->Kias();
			cockpitFlightData.vt = otwPlatform->Vt();
			cockpitFlightData.gs = ((AircraftClass*)otwPlatform)->GetNz();

			// Correct for wind
			float yaw = cockpitFlightData.yaw;
			if (yaw < 0.0F)
				yaw += 360.0F * DTR;

							 Tpoint			posit;
				posit.x = ((AircraftClass*)otwPlatform)->XPos();
				posit.y = ((AircraftClass*)otwPlatform)->YPos();
				 posit.z = ((AircraftClass*)otwPlatform)->ZPos();

			// Find angle between heading and wind
			yaw = ((WeatherClass*)realWeather)->WindHeadingAt(&posit) - yaw;

			// Project wind speed

			yaw = ((WeatherClass*)realWeather)->WindSpeedInFeetPerSecond(&posit) * (float)sin(yaw);

			// Find angle
			yaw = (float)atan2 (yaw, cockpitFlightData.vt);
			cockpitFlightData.windOffset = yaw;
		}

		// Add Engine data
		cockpitFlightData.fuelFlow = ((AircraftClass*)otwPlatform)->af->FuelFlow();
		cockpitFlightData.gearPos = ((AircraftClass*)otwPlatform)->af->gearPos;
		cockpitFlightData.speedBrake = ((AircraftClass*)otwPlatform)->af->dbrake;
		cockpitFlightData.ftit = ((AircraftClass*)otwPlatform)->af->rpm * 135.0F + 700.0F;
		cockpitFlightData.ftit2 = ((AircraftClass*)otwPlatform)->af->rpm2 * 135.0F + 700.0F;//TJL 01/14/04 multi-engine
		cockpitFlightData.rpm = 100.0F * ((AircraftClass*)otwPlatform)->af->rpm;
		cockpitFlightData.rpm2 = 100.0F * ((AircraftClass*)otwPlatform)->af->rpm2;//TJL 01/14/04 Multi-engine
		cockpitFlightData.internalFuel = ((AircraftClass*)otwPlatform)->af->Fuel();
		cockpitFlightData.externalFuel = ((AircraftClass*)otwPlatform)->af->ExternalFuel();
		// MD -- 20031011: make sure all fuel values needed are updated even if we aren't looking at the gauge
		((AircraftClass*)otwPlatform)->af->GetFuel(&cockpitFlightData.fwd, &cockpitFlightData.aft, &cockpitFlightData.total);
		cockpitFlightData.epuFuel = ((AircraftClass*)otwPlatform)->af->EPUFuel();
		tmpVal = ((AircraftClass*)otwPlatform)->af->rpm;
		tmpVal2 = ((AircraftClass*)otwPlatform)->af->rpm2;//TJL 01/14/04 multi-engine
		
		// Nozzle Position
		if(tmpVal <= 0.0F)
		{
			tmpVal	= 100.0F;
		}
		else if(tmpVal <= 0.83F)
		{
			tmpVal	= 100.0F + (0.0F - 100.0F) /  (0.83F) * tmpVal;
		}
		else if(tmpVal <= 0.99)
		{
			tmpVal	= 0.0F;
		}
		else if(tmpVal <= 1.03)
		{
			tmpVal	= (100.0F) /  (1.03F - 0.99F) * (tmpVal - 0.99F);
		}
		else
		{
			tmpVal	= 100.0F;
		}
	//TJL 01/14/04 Multi-engine (just adding to the spaghetti code)
	if(tmpVal2 <= 0.0F)
		{
			tmpVal2	= 100.0F;
		}
		else if(tmpVal2 <= 0.83F)
		{
			tmpVal2	= 100.0F + (0.0F - 100.0F) /  (0.83F) * tmpVal2;
		}
		else if(tmpVal2 <= 0.99)
		{
			tmpVal2	= 0.0F;
		}
		else if(tmpVal2 <= 1.03)
		{
			tmpVal2	= (100.0F) /  (1.03F - 0.99F) * (tmpVal2 - 0.99F);
		}
		else
		{
			tmpVal2	= 100.0F;
		}
		cockpitFlightData.nozzlePos = tmpVal;
		cockpitFlightData.nozzlePos2 = tmpVal2;

		//MI extracting Data
		// get the chaff/flare count
		cockpitFlightData.ChaffCount = ((AircraftClass*)otwPlatform)->counterMeasureStation[CHAFF_STATION].weaponCount;
		cockpitFlightData.FlareCount = ((AircraftClass*)otwPlatform)->counterMeasureStation[FLARE_STATION].weaponCount;


        // get the DED lines
		if(pCockpitManager->mpIcp) 
		{
			OTWDriver.pCockpitManager->mpIcp->Exec();
			OTWDriver.pCockpitManager->mpIcp->ExecPfl();
			if(mFaults &&
			    ((AircraftClass*)otwPlatform)->HasPower(AircraftClass::UFCPower) &&
				!mFaults->GetFault(FaultClass::ufc_fault))
			{
				for(int j = 0; j < 5; j++)
				{
					for(int i = 0; i < 26; i++)
					{
						cockpitFlightData.DEDLines[j][i] = pCockpitManager->mpIcp->DEDLines[j][i];
						cockpitFlightData.Invert[j][i] = pCockpitManager->mpIcp->Invert[j][i];
					}
				}
				for(int h = 0; h < 5; h++)
				{
					for(int k = 0; k < 26; k++)
					{
						cockpitFlightData.PFLLines[h][k] = pCockpitManager->mpIcp->PFLLines[h][k];
						cockpitFlightData.PFLInvert[h][k] = pCockpitManager->mpIcp->PFLInvert[h][k];
					}
				}

				//and UFC Tacan channel
				if(gNavigationSys) 
					cockpitFlightData.UFCTChan = gNavigationSys->GetTacanChannel(NavigationSystem::ICP);
			}
		}

		//update version of shared memarea
		cockpitFlightData.VersionNum = 110;

		//AUX Tacan channel
		if(gNavigationSys) 
			cockpitFlightData.AUXTChan = gNavigationSys->GetTacanChannel(NavigationSystem::AUXCOMM);

		// get the position of the three landing gears
		cockpitFlightData.NoseGearPos = ((AircraftClass*)otwPlatform)->GetDOFValue(ComplexGearDOF[0]);
		cockpitFlightData.LeftGearPos = ((AircraftClass*)otwPlatform)->GetDOFValue(ComplexGearDOF[1]);
		cockpitFlightData.RightGearPos = ((AircraftClass*)otwPlatform)->GetDOFValue(ComplexGearDOF[2]);

		// ADI data
		cockpitFlightData.AdiIlsHorPos = pCockpitManager->ADIGpDevReading;
		cockpitFlightData.AdiIlsVerPos = pCockpitManager->ADIGsDevReading;

		if (otwPlatform == NULL ||
			otwPlatform->IsExploding() ||
			otwPlatform->IsDead() ||
			!otwPlatform->IsAwake() ||
			TheHud->Ownship() == NULL)
		{
			okToDoCockpitStuff = FALSE;
		}

		// HSI States
		if(okToDoCockpitStuff && pCockpitManager->mpHsi && SimDriver.playerEntity)
				pCockpitManager->mpHsi->Exec();

		cockpitFlightData.courseState = pCockpitManager->mpHsi->GetState(CPHsi::HSI_STA_CRS_STATE);
		cockpitFlightData.headingState = pCockpitManager->mpHsi->GetState(CPHsi::HSI_STA_HDG_STATE);
		cockpitFlightData.totalStates = pCockpitManager->mpHsi->GetState(CPHsi::HSI_STA_TOTAL_STATES);
		// HSI Values
	    cockpitFlightData.courseDeviation = pCockpitManager->mpHsi->GetValue(CPHsi::HSI_VAL_CRS_DEVIATION);
		cockpitFlightData.desiredCourse	= pCockpitManager->mpHsi->GetValue(CPHsi::HSI_VAL_DESIRED_CRS);
		cockpitFlightData.distanceToBeacon = pCockpitManager->mpHsi->GetValue(CPHsi::HSI_VAL_DISTANCE_TO_BEACON);
	    cockpitFlightData.bearingToBeacon = pCockpitManager->mpHsi->GetValue(CPHsi::HSI_VAL_BEARING_TO_BEACON);
		cockpitFlightData.currentHeading = pCockpitManager->mpHsi->GetValue(CPHsi::HSI_VAL_CURRENT_HEADING);
		cockpitFlightData.desiredHeading = pCockpitManager->mpHsi->GetValue(CPHsi::HSI_VAL_DESIRED_HEADING);
		cockpitFlightData.deviationLimit = pCockpitManager->mpHsi->GetValue(CPHsi::HSI_VAL_DEV_LIMIT);
		cockpitFlightData.halfDeviationLimit = pCockpitManager->mpHsi->GetValue(CPHsi::HSI_VAL_HALF_DEV_LIMIT);
		cockpitFlightData.localizerCourse = pCockpitManager->mpHsi->GetValue(CPHsi::HSI_VAL_LOCALIZER_CRS);
		cockpitFlightData.airbaseX = pCockpitManager->mpHsi->GetValue(CPHsi::HSI_VAL_AIRBASE_X);
		cockpitFlightData.airbaseY = pCockpitManager->mpHsi->GetValue(CPHsi::HSI_VAL_AIRBASE_Y);
		cockpitFlightData.totalValues = pCockpitManager->mpHsi->GetValue(CPHsi::HSI_VAL_TOTAL_VALUES);
		// HSI Flags
		if (pCockpitManager->mpHsi->GetFlag(CPHsi::HSI_FLAG_TO_TRUE)) 
			cockpitFlightData.SetHsiBit(FlightData::ToTrue);
		else 
			cockpitFlightData.ClearHsiBit(FlightData::ToTrue);

		if (pCockpitManager->mpHsi->GetFlag(CPHsi::HSI_FLAG_ILS_WARN)) 
			cockpitFlightData.SetHsiBit(FlightData::IlsWarning);
		else 
			cockpitFlightData.ClearHsiBit(FlightData::IlsWarning);
        
		if (pCockpitManager->mpHsi->GetFlag(CPHsi::HSI_FLAG_CRS_WARN)) 
			cockpitFlightData.SetHsiBit(FlightData::CourseWarning);
		else 
			cockpitFlightData.ClearHsiBit(FlightData::CourseWarning);

		if (pCockpitManager->mpHsi->GetFlag(CPHsi::HSI_FLAG_INIT)) 
			cockpitFlightData.SetHsiBit(FlightData::Init);
		else 
			cockpitFlightData.ClearHsiBit(FlightData::Init);

		if (pCockpitManager->mpHsi->GetFlag(CPHsi::HSI_FLAG_TOTAL_FLAGS)) 
			cockpitFlightData.SetHsiBit(FlightData::TotalFlags);
		else 
			cockpitFlightData.ClearHsiBit(FlightData::TotalFlags);

		// MD -- 20031011: Moved here to ensure the bits are set regardless of whether the player
		// is looking at the cockpit panels or not.
		if(SimDriver.playerEntity->INSState(AircraftClass::INS_ADI_OFF_IN))
			cockpitFlightData.ClearHsiBit(FlightData::ADI_OFF);
		else
			cockpitFlightData.SetHsiBit(FlightData::ADI_OFF);
		
		if(SimDriver.playerEntity->INSState(AircraftClass::INS_ADI_AUX_IN))
			cockpitFlightData.ClearHsiBit(FlightData::ADI_AUX);
		else
			cockpitFlightData.SetHsiBit(FlightData::ADI_AUX);

		if(SimDriver.playerEntity->INSState(AircraftClass::INS_HSI_OFF_IN))
			cockpitFlightData.ClearHsiBit(FlightData::HSI_OFF);
		else
			cockpitFlightData.SetHsiBit(FlightData::HSI_OFF);

		if(SimDriver.playerEntity->INSState(AircraftClass::BUP_ADI_OFF_IN))
			cockpitFlightData.ClearHsiBit(FlightData::BUP_ADI_OFF);
		else
			cockpitFlightData.SetHsiBit(FlightData::BUP_ADI_OFF);

		if(SimDriver.AVTROn())
			cockpitFlightData.SetHsiBit(FlightData::AVTR);
		else
			cockpitFlightData.ClearHsiBit(FlightData::AVTR);

		if(SimDriver.playerEntity->GSValid == FALSE || SimDriver.playerEntity->currentPower == AircraftClass::PowerNone)
			cockpitFlightData.SetHsiBit(FlightData::ADI_GS);
		else
			cockpitFlightData.ClearHsiBit(FlightData::ADI_GS);

		if(SimDriver.playerEntity->LOCValid == FALSE || SimDriver.playerEntity->currentPower == AircraftClass::PowerNone)
			cockpitFlightData.SetHsiBit(FlightData::ADI_LOC);
		else
			cockpitFlightData.ClearHsiBit(FlightData::ADI_LOC);

		if(SimDriver.playerEntity->currentPower < AircraftClass::PowerEmergencyBus)
		{
			cockpitFlightData.SetHsiBit(FlightData::VVI);
			cockpitFlightData.SetHsiBit(FlightData::AOA);
		}
		else
		{
			cockpitFlightData.ClearHsiBit(FlightData::VVI);
			cockpitFlightData.ClearHsiBit(FlightData::AOA);
		}

		// MD -- 2003: end of SetHsiBit() fixes

		// Oil Pressure
		tmpVal = ((AircraftClass*)otwPlatform)->af->rpm;
		tmpVal2 = ((AircraftClass*)otwPlatform)->af->rpm2;//TJL 01/14/04 multi-engine
		
		if(tmpVal < 0.7F)
		{
			tmpVal	= 40.0F;
		}
		else if(tmpVal <= 0.85)
		{
			tmpVal	= 40.0F + (100.0F - 40.0F) /  (0.85F - 0.7F) * (tmpVal - 0.7F);
		}
		else if(tmpVal <= 1.0)
		{
			tmpVal	= 100.0F;
		}
		else if(tmpVal <= 1.03)
		{
			tmpVal	= 100.0F + (103.0F - 100.0F) /  (1.03F - 1.0F) * (tmpVal - 1.00F);
		}
		else
		{
			tmpVal	= 103.0F;
		}
		//TJL 01/14/04 Multi-engine (stop the insanity)
		if(tmpVal2 < 0.7F)
		{
			tmpVal2	= 40.0F;
		}
		else if(tmpVal2 <= 0.85)
		{
			tmpVal2	= 40.0F + (100.0F - 40.0F) /  (0.85F - 0.7F) * (tmpVal2 - 0.7F);
		}
		else if(tmpVal2 <= 1.0)
		{
			tmpVal2	= 100.0F;
		}
		else if(tmpVal2 <= 1.03)
		{
			tmpVal2	= 100.0F + (103.0F - 100.0F) /  (1.03F - 1.0F) * (tmpVal2 - 1.00F);
		}
		else
		{
			tmpVal2	= 103.0F;
		}

		//Trim values
		cockpitFlightData.TrimPitch = UserStickInputs.ptrim;
		cockpitFlightData.TrimRoll = UserStickInputs.rtrim;
		cockpitFlightData.TrimYaw = UserStickInputs.ytrim;

		cockpitFlightData.oilPressure = tmpVal;
		cockpitFlightData.oilPressure2 = tmpVal2;

//		if ((gSharedMemPtr)&&(!g_bEnableTrackIR))	// Retro 02/10/03
		if (gSharedMemPtr)
		{
//			if (!mHelmetIsUR)
			if ((!mHelmetIsUR)&&(!g_bEnableTrackIR))	// Retro 8Jan2004 - almost killed the shared mem for good, silly me :/
			{
				cockpitFlightData.headYaw = ((FlightData*)gSharedMemPtr)->headYaw;
				cockpitFlightData.headPitch = ((FlightData*)gSharedMemPtr)->headPitch;
				cockpitFlightData.headRoll = ((FlightData*)gSharedMemPtr)->headRoll;
			}
			memcpy (gSharedMemPtr, &cockpitFlightData, sizeof (FlightData));
		}
	}

	// Set up the camera based on the current view chosen
	if (endFlightTimer || flybyTimer)
	{
		SetFlybyCameraPosition(dT);
	}
	else if (eyeFly)
	{
		SetEyeFlyCameraPosition(dT);
	}
	else if (otwPlatform)
	{
		// If we _were_ in eyeFly and now aren't, clean up.
		// SCR:  Could this be better done once at mode change???
		if (eyeFlyTgt)
		{
			eyeFlyTgt->drawPointer->SetLabel ("", 0x0);
			eyeFlyTgt = NULL;
		}

		// Select on of the "standard" views
		if (DisplayInCockpit())
		{
			SetInternalCameraPosition(dT);
		}
		else
		{
			SetExternalCameraPosition(dT);
		}
	}
	else
	{
		SetExternalCameraPosition(dT);
	//	SetFlybyCameraPosition(dT);
	}



	// Compute the new viewpoint position
	viewPos.x = focusPoint.x + cameraPos.x;
	viewPos.y = focusPoint.y + cameraPos.y;
	viewPos.z = focusPoint.z + cameraPos.z;

	if (otwPlatform && /*otwPlatform->IsAirplane() &&*/ gSharedIntellivibe) 
	{
	  g_intellivibeData.eyex = viewPos.x;
	  g_intellivibeData.eyey = viewPos.y;
	  g_intellivibeData.eyez = viewPos.z;
	  g_intellivibeData.IsFrozen = SimDriver.MotionOn() == 0;
	  g_intellivibeData.IsPaused = targetCompressionRatio == 0;
	  g_intellivibeData.IsFiringGun = otwPlatform->IsFiring() != 0;
	  g_intellivibeData.IsEjecting = otwPlatform->IsEject() != 0;
	  g_intellivibeData.IsOnGround = otwPlatform->OnGround() != 0;
		
		if (otwPlatform->IsAirplane())
		{
		  AircraftClass *air = (AircraftClass *)otwPlatform;
		  g_intellivibeData.Gforce = air->GetNz();
		}
	  
		memcpy (gSharedIntellivibe, &g_intellivibeData, sizeof(g_intellivibeData));
	}

	// edg: this is a band aid solution for the action cam.  It seems that
	// something is causing cameraPos to have wild numbers -- perhaps an
	// uninitialed matrix or something somewhere.  Since the maximum range for
	// this camera is +/- 20000, in satellite mode, we'll clamp the values
	// here....
	// LRKLUDGE Keep the action camera happy. NOTE: We need to find out why it is going out into left field
	// edg: actually, there's no reason to restrict this to action camera...

	// if ( actionCameraMode )
	{
		if ( _isnan (viewPos.x) || !_finite(viewPos.x))
		{
			ShiWarning ("Bad action camera X pos: Why?");
			viewPos.x = 250000.0f;
		}

		if ( _isnan (viewPos.y) || !_finite(viewPos.y))
		{
			ShiWarning ("Bad action camera Y pos: Why?");
			viewPos.y = 250000.0f;
		}

		if ( _isnan (viewPos.z) || !_finite(viewPos.z))
		{
			ShiWarning ("Bad action camera Z pos: Why?");
			viewPos.z = -10000.0f;
		}
	}

	// now we want to tell the bubble rebuild where our camera is if we're
	// not attached to the player.  we do this by setting attaching and/or
	// setting the position of the camera entity.
	camCount = FalconLocalSession->CameraCount(); 
	if ( otwPlatform != SimDriver.playerEntity )
	{
		gOtwCameraLocation->SetPosition (viewPos.x, viewPos.y, viewPos.z);
		if ( camCount <= 1 )
		{
			FalconLocalSession->AttachCamera (gOtwCameraLocation->Id());
		}
		else
		{
			for (i=0; i<camCount; i++)
			{
				if (FalconLocalSession->Camera(i) == gOtwCameraLocation)
					break;
			}

			if (i == camCount )
			{
				// remove 2nd camera (should be mav camera) and insert ourself
				if (!F4IsBadReadPtr(FalconLocalSession->Camera(1), sizeof(VuEntity))) // JB 010318 CTD
					FalconLocalSession->RemoveCamera(FalconLocalSession->Camera(1)->Id());
				FalconLocalSession->AttachCamera (gOtwCameraLocation->Id());
			}
		}
	}
	else if ( camCount > 1 )
	{
		for (i=0; i<camCount; i++)
		{
			if (FalconLocalSession->Camera(i) == gOtwCameraLocation)
			{
				FalconLocalSession->RemoveCamera(gOtwCameraLocation->Id());
				break;
			}
		}
	}

	// Keep the viewpoint above ground
	GetAreaFloorAndCeiling(&bottom, &top);
	if (viewPos.z > top)
	{
		viewPos.z = min (viewPos.z, GetGroundLevel(viewPos.x, viewPos.y) - 5.0F);
	}


	// Now update everyone
	viewPoint->Update( &viewPos);
//	F4SoundFXSetCamPos( viewPos.x, viewPos.y, viewPos.z );
//  MLR 2003-10-17  10-27 returned to normal
	F4SoundFXSetCamPosAndOrient(&viewPos, &cameraRot, &cameraVel);
	
	TheTimeManager.SetTime(vuxGameTime + (unsigned long)FloatToInt32(todOffset * 1000.0F));
	((WeatherClass*)realWeather)->UpdateWeather();
	BuildExternalNearList();


	// Set up the black out effects
	if (DisplayInCockpit() && doGLOC && otwPlatform == SimDriver.playerEntity && otwPlatform)
	{
		float glocFactor = ((AircraftClass*)otwPlatform)->glocFactor;

		if (glocFactor >= 0.0F)
		{
			renderer->SetTunnelPercent( glocFactor, 0 );
		}
		else
		{
			renderer->SetTunnelPercent( -glocFactor, FloatToInt32(-255.0F * glocFactor) );
		}
	}
	else if ( endFlightTimer )
	{
		float pct = (float)(endFlightTimer - vuxRealTime)/5000.0f;
		pct = 1.0f - pct;
		pct *= pct;
		if ( vuxRealTime > endFlightTimer )
			pct = 1.0f;
		renderer->SetTunnelPercent( pct, 0 );
	}
	else
	{
		renderer->SetTunnelPercent( 0.0F, 0 );
	}


	//JAM 12Dec03
	if(DisplayOptions.bZBuffering)
		renderer->context.SetZBuffering(TRUE);

	// Actually draw the scene
	renderer->StartFrame();

	// Set the font here for labels
	oldFont = VirtualDisplay::CurFont();
	VirtualDisplay::SetFont(pCockpitManager->LabelFont());
	renderer->DrawScene((struct Tpoint *) &Origin, (struct Trotation *) &cameraRot);
	VirtualDisplay::SetFont(oldFont);

	// Put it back :)

	// edg: We've got crashes due to ownship dead and trying to do cockpit
	// stuff.  I'm going to sum up the ok-ness of doing cockpit stuff into
	// 1 bool to be used in tests below
	if ( !DisplayInCockpit() ||
		  otwPlatform == NULL ||
		  otwPlatform->IsExploding() ||
		  otwPlatform->IsDead() ||
		 !otwPlatform->IsAwake() ||
		  TheHud->Ownship() == NULL ||
//		  FalconLocalSession->GetFlyState() != FLYSTATE_FLYING ||
		  eyeFly)
	{
		okToDoCockpitStuff = FALSE;

		switch (GetOTWDisplayMode ())
		{
			case Mode2DCockpit:
			case ModePadlockF3:
			case Mode3DCockpit:
			case ModePadlockEFOV:
			case ModeHud:
			{
				SetOTWDisplayMode (ModeOrbit);
				break;
			}
			
			default:
			{
				break;
			}
		}
	}

	//JAM 17Dec03
	if(DisplayInCockpit() && okToDoCockpitStuff)
	{
		if(DisplayOptions.bZBuffering)
			renderer->context.FlushPolyLists();

		if(GetOTWDisplayMode() == Mode2DCockpit)
		{
			ShiAssert(SimDriver.playerEntity == otwPlatform);
			pCockpitManager->GeometryDraw();
		}
		else if(GetOTWDisplayMode() == ModePadlockF3)
		{
			ShiAssert(SimDriver.playerEntity == otwPlatform);
			Padlock_DrawSquares(TRUE);
			VCock_Exec();
		}
		else if(GetOTWDisplayMode() == Mode3DCockpit)
		{
			ShiAssert(SimDriver.playerEntity == otwPlatform);
			VCock_Exec();
		}
		else if(GetOTWDisplayMode() == ModePadlockEFOV)
		{
			ShiAssert(SimDriver.playerEntity == otwPlatform);
			Padlock_DrawSquares(TRUE);
		}
	}
	else
	{
		if(otwPlatform && otwPlatform->drawPointer)
		{
			if(!otwPlatform->OnGround())
			{
				DrawExternalViewTarget();
			}
		}

		if(DisplayOptions.bZBuffering)
			renderer->context.FlushPolyLists();

		if(g_bLensFlare)
			Draw2DLensFlare(renderer);
	}
	//JAM

	// Clear out the "near" list now that we're done drawing it
	FlushNearList();

	// Add a cross hair marker in eye fly mode
// JB 011121 Remove
/*
	if (eyeFly)
	{
		renderer->SetColor (0xff00ff00);
		renderer->Line (0.0F, 0.05F, 0.0F, -0.05F);
		renderer->Line (0.05F, 0.0F, -0.05F, 0.0F);
	}
*/

	// Do the first layer of drawn cockpit stuff (pre BLT)
	if (okToDoCockpitStuff)
	{

		// Should we Draw the HUD?
		if ( pCockpitManager->ShowHud() )
		{
			if (GetOTWDisplayMode() == ModePadlockEFOV ||
				GetOTWDisplayMode() == ModeHud ||
				GetOTWDisplayMode() == Mode2DCockpit)
			{
				Draw2DHud();
			}
		}

		// Should we draw the EFOV window?
		if (GetOTWDisplayMode() == ModePadlockEFOV && ((SimMoverClass*)otwPlatform)->targetList)
		{
			ShiAssert( otwPlatform->IsLocal() );	// SCR: Was a condition above, but in EFOV view it MUST be the player, right?

			Padlock_CheckPadlock(dT);
			PadlockEFOV_Draw ();
		}

		if (GetOTWDisplayMode() == ModePadlockF3 || GetOTWDisplayMode() == Mode3DCockpit)
		{
			if (mDoSidebar)
			{
				PadlockF3_Draw();
			}
		}
	}

	// Might we need to draw cockpit stuff?
	if(okToDoCockpitStuff)
	{
		ShiAssert( otwPlatform == SimDriver.playerEntity );

		if(GetOTWDisplayMode() == Mode2DCockpit)
		{
			renderer->FinishFrame();

			// OW
			if(!DisplayOptions.bRender2DCockpit)
			{
				// Draw in the 2D cockpit panels
				pCockpitManager->Exec();
				pCockpitManager->DisplayBlit();
				pCockpitManager->DisplayDraw();
			}

			else
			{
				pCockpitManager->Exec();
				renderer->StartFrame();

				float top, left, bottom, right;
				renderer->GetViewport(&left, &top, &right, &bottom); // save the current viewport
				renderer->SetViewport(-1.0F, 1.0F, 1.0F, -1.0F);	// set fullscreen viewport
				pCockpitManager->DisplayBlit3D();	// draw 3d stuff
				renderer->FinishFrame();
				renderer->SetViewport(left, top, right, bottom);	// restore viewport

				// Draw in the 2D cockpit panels
				pCockpitManager->DisplayBlit();
				pCockpitManager->DisplayDraw();
			}

			// Draw MFDs and RWR
			PlayerRwrClass* theRwr;
			AircraftClass *playerAircraft = (AircraftClass *)SimDriver.playerEntity;
			if(playerAircraft)
			{
				theRwr = (PlayerRwrClass*)FindSensor(playerAircraft, SensorClass::RWR);
				if(pCockpitManager->ShowRwr() && theRwr) 
				{
#if DO_HIRESCOCK_HACK
					if(!gDoCockpitHack)
#endif
					{
						renderer->StartFrame();
						pCockpitManager->GetViewportBounds(&viewportBounds, BOUNDS_RWR);
						renderer->SetColor(0xFF00FF00);
						renderer->SetViewport(viewportBounds.left, viewportBounds.top, viewportBounds.right, viewportBounds.bottom);	         

						theRwr->SetGridVisible(FALSE);
						theRwr->Display(renderer);
						renderer->FinishFrame();
					}
				}

				// SetFont
				oldFont = VirtualDisplay::CurFont();
				VirtualDisplay::SetFont(pCockpitManager->MFDFont());
				if(pCockpitManager->GetViewportBounds(&viewportBounds, BOUNDS_MFDLEFT))
				{
					MfdDisplay[0]->SetImageBuffer(OTWImage, viewportBounds.left, viewportBounds.top, viewportBounds.right, viewportBounds.bottom);
					MfdDisplay[0]->Exec(FALSE, FALSE);
				}
				if(pCockpitManager->GetViewportBounds(&viewportBounds, BOUNDS_MFDRIGHT))
				{
					MfdDisplay[1]->SetImageBuffer(OTWImage, viewportBounds.left, viewportBounds.top, viewportBounds.right, viewportBounds.bottom);
					MfdDisplay[1]->Exec(FALSE, FALSE);
				}
				VirtualDisplay::SetFont(oldFont);
			}

			//Wombat778 12-12-2003 Moved this here from cpmanager.  Should mean it runs more reliably
			//			10-18-2003 Hack for 1.25 ratio resolutions.  Draws a black box at the bottom 1/16 of the screen so you dont get garbage

			if (g_bCockpitAutoScale && g_bRatioHack && ((float) DisplayOptions.DispWidth / (float) DisplayOptions.DispHeight) == 1.25)		//Wombat778 10-24-2003 added g_bCockpitAutoScale	//so we are in a 1.25 ratio
			{
				RECT tempsrcrect, tempdestrect;

				tempsrcrect.top=0;
				tempsrcrect.left=0;
				tempsrcrect.right=DisplayOptions.DispWidth;
				//tempsrcrect.bottom=FloatToInt32((float)DisplayOptions.DispHeight/16.0f);
				tempsrcrect.bottom=FloatToInt32((DisplayOptions.DispHeight-(float)DisplayOptions.DispHeight*0.9375f)+0.5f);		//should ensure that everything is more accurate Add a 0.5f for rounding safety


				tempdestrect.top=FloatToInt32(((float)DisplayOptions.DispHeight*0.9375f) + 0.5f);    //This is 15/16  //Wombat778 10-24-2003 added 0.5f just to be safe
				tempdestrect.left=0;    //This is 15/16
				tempdestrect.right=DisplayOptions.DispWidth;
				tempdestrect.bottom=DisplayOptions.DispHeight;

				pCockpitManager->mpOTWImage->Compose(pCockpitManager->RatioBuffer,&tempsrcrect, &tempdestrect);
			}

			//end of ratio hack

		}
		else if ((GetOTWDisplayMode() == ModeHud || GetOTWDisplayMode() == ModePadlockEFOV) &&
			!g_bNoMFDsIn1View)	//MI added g_bNoMFDsIn1View check. Removes MFD's if TRUE
		{
			// SetFont
			oldFont = VirtualDisplay::CurFont();
			VirtualDisplay::SetFont(pCockpitManager->MFDFont());
			renderer->FinishFrame();
			for(i = 0; i < NUM_MFDS; i++)
			{
				MfdDisplay[i]->Exec(TRUE, FALSE);
			}
			VirtualDisplay::SetFont(oldFont);
		}
		else if (GetOTWDisplayMode() == ModePadlockF3 || GetOTWDisplayMode() == Mode3DCockpit)
		{
			if (mDoSidebar && pPadlockCPManager)
			{
				// Draw in the 2D reference panels
				pPadlockCPManager->Exec();

				// OW
				float top, left, bottom, right;
				renderer->GetViewport(&left, &top, &right, &bottom); // save the current viewport
				renderer->SetViewport(-1.0F, 1.0F, 1.0F, -1.0F);	// set fullscreen viewport
				pPadlockCPManager->DisplayBlit3D();	// draw 3d stuff
				renderer->FinishFrame();
				renderer->SetViewport(left, top, right, bottom);	// restore viewport

				// Draw in the 2D cockpit panels
				pPadlockCPManager->DisplayBlit();
				pPadlockCPManager->DisplayDraw();
			}

			else
				renderer->FinishFrame();
		}
	}
	else
	{
		renderer->FinishFrame();
	}


	// Draw wingman, tanker, awacs menus
	pMenuManager->DisplayDraw();


	// Draws ANY text in front of everything else
	// the Chat box at least MUST show up AFTER the cockpit is drawn
	DisplayFrontText();


	// Draw GLOC effect
	if (doGLOC && otwPlatform && otwPlatform == SimDriver.playerEntity && otwPlatform->IsLocal())
	{
		renderer->DrawTunnelBorder();
	}
	else if (TheTimeOfDay.GetNVGmode() || endFlightTimer)
	{
		renderer->DrawTunnelBorder();
	}

/*	if (renderer->IsThunder() && renderer->viewpoint) 
	{
		static int uid=0;
	    // clap of thunder, somewhere around here.
	    F4SoundFXSetPos(SFX_THUNDER, TRUE, 
		viewPos.x + 200.0f * PRANDFloat(), 
		viewPos.y + 200.0f * PRANDFloat(), 
		renderer->viewpoint->GetLocalCloudTops(),
		1,0,0,0,0,uid);
		uid++;
	}
	if (renderer->RainFactor() > 0 || renderer->SnowFactor()) {
	    float bfact = renderer->RainFactor() + renderer->SnowFactor();
	    float vfact = 20.0f / bfact;
	    vfact = vfact * vfact;*/

	//JAM 18Nov03
	if( weatherCondition == INCLEMENT && cameraPos.z > realWeather->stratusZ )
	{
	    if(DisplayInCockpit())
			F4SoundFXSetDist(SFX_RAININT,FALSE,10.f,2.f);
	    else
			F4SoundFXSetDist(SFX_RAINEXT,FALSE,10.f,2.f);
	}

	// Show the exit menu if needed
	DrawExitMenu();

#ifdef MAKE_MOVIE
	if (gACMIRec.IsRecording())
	{
		TakeScreenShot();
	}
#else
	if (takeScreenShot)
	{
		TakeScreenShot();

		if(pMenuManager)
		{
			pMenuManager->DeActivate();
		}
	}
#endif

	// Finish and swap the buffers
	// Force cursors if in exit menu
	
	if (InExitMenu())
	{
		int tmp = gSelectedCursor;
		gSelectedCursor = 1;
		ClipAndDrawCursor(OTWDriver.pCockpitManager->GetCockpitWidth(), OTWDriver.pCockpitManager->GetCockpitHeight());
		gSelectedCursor = tmp;		
	}


	//Wombat778 1-23-04 Changed from gTimeLastMouseMove to gTimeLastCursorUpdate because gTimeLastMouseMove reports ALL changes in mouse movement, not just cursor updates.

	else if (gSimInputEnabled && SimDriver.playerEntity && vuxRealTime - /*gTimeLastMouseMove*/gTimeLastCursorUpdate < SI_MOUSE_TIME_DELTA)		
	{
		//Wombat778 12-16-2003 Commented out old code and replaced with code below

		/*if	(GetOTWDisplayMode() == Mode2DCockpit &&
			(gSelectedCursor >= 0) &&
			(otwPlatform == SimDriver.playerEntity))
		{
			ClipAndDrawCursor(OTWDriver.pCockpitManager->GetCockpitWidth(), OTWDriver.pCockpitManager->GetCockpitHeight());
		}*/

		//Wombat778 Draw the cursor in the 3d pit as well
		if	((GetOTWDisplayMode() == Mode2DCockpit || GetOTWDisplayMode() == Mode3DCockpit || GetOTWDisplayMode() == ModePadlockF3 || GetOTWDisplayMode() == ModePadlockEFOV) &&
			(gSelectedCursor >= 0) &&
			(otwPlatform == SimDriver.playerEntity))
		{
			ClipAndDrawCursor(OTWDriver.pCockpitManager->GetCockpitWidth(), OTWDriver.pCockpitManager->GetCockpitHeight());
		}
		
	}
	count++;
}

//JAM 27Dec03 - Bookmark
void OTWDriverClass::SetInternalCameraPosition (float dT)
{
	Prof(SetInternalCameraPosition);	// Retro 15/10/03

	// Display is 1st person, cockpit type....
	ShiAssert( DisplayInCockpit() );


	UpdateCameraFocus();

	// courtesy of the code that calls this function otwPlatform is always valid
	cameraVel.x=otwPlatform->XDelta();  // MLR 12/9/2003 - 
	cameraVel.y=otwPlatform->YDelta();
	cameraVel.z=otwPlatform->ZDelta();

#if 1  // MLR 12/1/2003 - Eye position is controlled by pilotEyePos, which is gotten from the players AC
	MatrixMult( &ownshipRot, &pilotEyePos, &cameraPos );
#else
	// OLD EYE PLACEMENT

	// These two constants are based on rough ruler measurements
	// from a schematic of the F16.
	// TODO:  Get these from the object some how.
	static const float	EyeFromCGfwd	= 15.0f;
	static const float	EyeFromCGup		=  3.0f;

	// Move the camera position from the CG to the cockpit
	cameraPos.x = EyeFromCGfwd * ownshipRot.M11 - EyeFromCGup * ownshipRot.M13;
	cameraPos.y = EyeFromCGfwd * ownshipRot.M21 - EyeFromCGup * ownshipRot.M23;
	cameraPos.z = EyeFromCGfwd * ownshipRot.M31 - EyeFromCGup * ownshipRot.M33;
#endif

	// Adjust for head angle (if enabled)
	if (GetOTWDisplayMode() == Mode2DCockpit)
	{
		if ((g_bEnableTrackIR)&&(PlayerOptions.Get2dTrackIR() == true))	// Retro 27/09/03
		{
#ifdef DEBUG_TRACKIR_STUFF
			FILE* fp = fopen("TIR_Debug_2.txt","at");
			fprintf(fp,"%x",vuxRealTime);
			if (vuxRealTime & g_nTrackIRSampleFreq)
			{
				fprintf(fp," - chk");
				if (theTrackIRObject.Get_Panning_Allowed())
					fprintf(fp," - allowed");
			}
			fprintf(fp,"\n");
			fclose(fp);
#endif
			/* sample time is user-configurable */
			if (vuxRealTime & g_nTrackIRSampleFreq)	// Retro 26/09/03 - check every 512 ms (default value)
			{
				if (theTrackIRObject.Get_Panning_Allowed())
					SimDriver.POVKludgeFunction(theTrackIRObject.TrackIR_2D_Map());	// Retro 26/09/03
			}
			else
			{
				theTrackIRObject.Allow_2D_Panning();
			}
		}
		// Update the head matrix
		BuildHeadMatrix( FALSE, YAW_PITCH, eyePan, eyeTilt, 0.0F );
		// Combine the head and airplane matrices
		MatrixMult (&ownshipRot, &headMatrix, &cameraRot);

	}
	else if (GetOTWDisplayMode() == Mode3DCockpit)
	{
		VCock_GiveGilmanHead(dT);
	} 
	else if (GetOTWDisplayMode() == ModePadlockF3)
	{
// 2000-11-12 MODIFIED BY S.G. SO PANNING BREAKS THE PADLOCK
//		Padlock_CheckPadlock(dT);
//		PadlockF3_CalcCamera(dT);
		if (g_nPadlockMode & PLockModeBreakLock) {
			if (azDir == 0.0F && elDir == 0.0F) {
				Padlock_CheckPadlock(dT);
				PadlockF3_CalcCamera(dT);
			}
			else {
				float oldAzDir = azDir, oldElDir = elDir; // 2002-03-12 ADDED BY S.G. Lets remember these so I can set them back after
				float tmpEyePan  = eyePan;
				float tmpEyeTilt = eyeTilt;
				float tmpEyeHeadRoll = eyeHeadRoll;
				mIsSlewInit = FALSE;
				SetOTWDisplayMode(OTWDriverClass::Mode3DCockpit);
				eyePan  = tmpEyePan;
				eyeTilt = tmpEyeTilt;
				eyeHeadRoll = tmpEyeHeadRoll;
				azDir = oldAzDir; // 2002-03-12 ADDED BY S.G. Set them back to the saved value so head starts moving right away without requiring the key to let go and pushed again...
				elDir = oldElDir;
			}
		}
		else {
			Padlock_CheckPadlock(dT);
			PadlockF3_CalcCamera(dT);
		}
// END OF MODIFIED SECTION
	}
	else
	{
		memcpy (&cameraRot, &ownshipRot, sizeof (Trotation));
	}
}

#ifdef MEM_DEBUG

#include "guns.h"
#include "simfeat.h"
#include "airframe.h"
#include "ground.h"
#include "gndai.h"
#include "missile.h"
#include "bomb.h"
#include "battalion.h"
#include "brigade.h"
#include "navunit.h"
#include "objectiv.h"
#include "package.h"
#include "squadron.h"
#include "flight.h"
#include "persist.h"
#include "helo.h"
#include "hdigi.h"
#include "atcbrain.h"
#include "limiters.h"
#include "simvudrv.h"
#include "listadt.h"
#include "falcsnd\voicemanager.h"
#include "objects\drawtrcr.h"
#include "objects\drawbldg.h"
#include "objects\drawbsp.h"
#include "objects\drawbrdg.h"
#include "objects\drawovc.h"
#include "objects\drawplat.h"
#include "objects\drawrdbd.h"
#include "objects\drawsgmt.h"
#include "objects\drawpuff.h"
#include "objects\drawshdw.h"
#include "objects\drawpnt.h"
#include "objects\drawguys.h"
#include "objects\drawpole.h"
#include "bsplib\objectlod.h"
#include "terrain\tblock.h"
#include "division.h"
#include "evtparse.h"

// edg: temp comment out

extern MEM_POOL glMemPool;
extern MEM_POOL graphicsDOFDataPool;
extern MEM_POOL vuRBNodepool;
extern MEM_POOL gDivVUIDs;
extern MEM_POOL gTextMemPool;
extern MEM_POOL gTexDBMemPool;
extern MEM_POOL gTPostMemPool;
extern MEM_POOL gFartexMemPool;
extern MEM_POOL gBSPLibMemPool;
extern MEM_POOL gObjMemPool;
extern MEM_POOL gReadInMemPool;
extern MEM_POOL gSoundMemPool;
extern MEM_POOL gTacanMemPool;
extern MEM_POOL gVuFilterMemPool;
extern MEM_POOL gInputMemPool;
extern "C" MEM_POOL gResmgrMemPool;

extern int ObjectNodes, ObjectReferences;

void DebugMemoryReport( RenderOTW *renderer, int frameTime )
{
	float row, col;
	int objCount = 0;
	int totCount = 0;
	int totSize = 0;

		sprintf (tmpStr, "%.2f SFX=%d Proc Objs=%d Draw Objs=%d CT=%d MAX=%d ST=%d MAX=%d GT=%d MAX=%d AVESGT=%d AVECT=%d MemC=%d MemS=%d",
			1.0F / (float)(frameTime) * 1000.0F,
			gTotSfx,
			numObjsProcessed,
			numObjsInDrawList,
			gCampTime,
			gCampTimeMax,
			gSimTime,
			gSimTimeMax,
			gGraphicsTimeLast,
			gGraphicsTimeLastMax,
			gAveSimGraphicsTime,
			gAveCampTime,
			dbgMemTotalCount(),
			dbgMemTotalSize() );
		renderer->TextLeft (-0.95F,  0.95F, tmpStr);

		col = -0.95f;
		row =  0.90f;

		// DEFAULT POOL

		sprintf( tmpStr, "DEFAULT POOL C=%d S=%d",
				 MemPoolCount( MemDefaultPool ),
				 MemPoolSize( MemDefaultPool ) );

		renderer->TextLeft (col, row, tmpStr);

		row -= 0.05f;

		// Campaign Stuff

		sprintf( tmpStr, "C Obj C=%d S=%d",
				 MemPoolCount( ObjectiveClass::pool ),
				 MemPoolSize( ObjectiveClass::pool ) );

		renderer->TextLeft (col, row, tmpStr);

		sprintf( tmpStr, "C Bat C=%d S=%d",
				 MemPoolCount( BattalionClass::pool ),
				 MemPoolSize( BattalionClass::pool ) );

		renderer->TextLeft (col+0.6F, row, tmpStr);

		sprintf( tmpStr, "C Bri C=%d S=%d",
				 MemPoolCount( BrigadeClass::pool ),
				 MemPoolSize( BrigadeClass::pool ) );

		renderer->TextLeft (col+1.2F, row, tmpStr);

		row -= 0.05f;

		sprintf( tmpStr, "C Fli C=%d S=%d",
				 MemPoolCount( FlightClass::pool ),
				 MemPoolSize( FlightClass::pool ) );

		renderer->TextLeft (col, row, tmpStr);

		sprintf( tmpStr, "C Squ C=%d S=%d",
				 MemPoolCount( SquadronClass::pool ),
				 MemPoolSize( SquadronClass::pool ) );

		renderer->TextLeft (col+0.6F, row, tmpStr);

		sprintf( tmpStr, "C Pak C=%d S=%d",
				 MemPoolCount( PackageClass::pool ),
				 MemPoolSize( PackageClass::pool ) );

		renderer->TextLeft (col+1.2F, row, tmpStr);

		row -= 0.05f;

		sprintf( tmpStr, "C Tsk C=%d S=%d",
				 MemPoolCount( TaskForceClass::pool ),
				 MemPoolSize( TaskForceClass::pool ) );

		renderer->TextLeft (col, row, tmpStr);

		sprintf( tmpStr, "C Per C=%d S=%d",
				 MemPoolCount( SimPersistantClass::pool ),
				 MemPoolSize( SimPersistantClass::pool ) );

		renderer->TextLeft (col+0.6f, row, tmpStr);


		row -= 0.05f;

		// Drawables

		sprintf( tmpStr, "D 2D C=%d S=%d",
				 MemPoolCount( Drawable2D::pool ),
				 MemPoolSize( Drawable2D::pool ) );

		renderer->TextLeft (col, row, tmpStr);

		sprintf( tmpStr, "D Tcr C=%d S=%d",
				 MemPoolCount( DrawableTracer::pool ),
				 MemPoolSize( DrawableTracer::pool ) );

		renderer->TextLeft (col+0.6f, row, tmpStr);

		sprintf( tmpStr, "D Gvh C=%d S=%d",
				 MemPoolCount( DrawableGroundVehicle::pool ),
				 MemPoolSize( DrawableGroundVehicle::pool ) );

		renderer->TextLeft (col+1.2f, row, tmpStr);

		row -= 0.05f;

		sprintf( tmpStr, "D bld C=%d S=%d",
				 MemPoolCount( DrawableBuilding::pool ),
				 MemPoolSize( DrawableBuilding::pool ) );

		renderer->TextLeft (col, row, tmpStr);

		sprintf( tmpStr, "D BSP C=%d S=%d",
				 MemPoolCount( DrawableBSP::pool ),
				 MemPoolSize( DrawableBSP::pool ) );

		renderer->TextLeft (col+0.6f, row, tmpStr);

		sprintf( tmpStr, "D sha C=%d S=%d",
				 MemPoolCount( DrawableShadowed::pool ),
				 MemPoolSize( DrawableShadowed::pool ) );

		renderer->TextLeft (col+1.2f, row, tmpStr);

		row -= 0.05f;

		sprintf( tmpStr, "D brg C=%d S=%d",
				 MemPoolCount( DrawableBridge::pool ),
				 MemPoolSize( DrawableBridge::pool ) );

		renderer->TextLeft (col, row, tmpStr);

		sprintf( tmpStr, "D ovc C=%d S=%d",
				 MemPoolCount( DrawableOvercast::pool ),
				 MemPoolSize( DrawableOvercast::pool ) );

		renderer->TextLeft (col+0.6f, row, tmpStr);

		sprintf( tmpStr, "D pla C=%d S=%d",
				 MemPoolCount( DrawablePlatform::pool ),
				 MemPoolSize( DrawablePlatform::pool ) );

		renderer->TextLeft (col+1.2f, row, tmpStr);

		row -= 0.05f;

		sprintf( tmpStr, "D rdb C=%d S=%d",
				 MemPoolCount( DrawableRoadbed::pool ),
				 MemPoolSize( DrawableRoadbed::pool ) );

		renderer->TextLeft (col, row, tmpStr);

		sprintf( tmpStr, "D seg C=%d S=%d",
				 MemPoolCount( DrawableTrail::pool ),
				 MemPoolSize( DrawableTrail::pool ) );

		renderer->TextLeft (col+0.6f, row, tmpStr);

		sprintf( tmpStr, "D TrE C=%d S=%d",
				 MemPoolCount( TrailElement::pool ),
				 MemPoolSize( TrailElement::pool ) );

		renderer->TextLeft (col+1.2f, row, tmpStr);

		row -= 0.05f;

		sprintf( tmpStr, "D puf C=%d S=%d",
				 MemPoolCount( DrawablePuff::pool ),
				 MemPoolSize( DrawablePuff::pool ) );

		renderer->TextLeft (col, row, tmpStr);

		sprintf( tmpStr, "D pnt C=%d S=%d",
				 MemPoolCount( DrawablePoint::pool ),
				 MemPoolSize( DrawablePoint::pool ) );

		renderer->TextLeft (col+0.6f, row, tmpStr);

		sprintf( tmpStr, "D guy C=%d S=%d",
				 MemPoolCount( DrawableGuys::pool ),
				 MemPoolSize( DrawableGuys::pool ) );

		renderer->TextLeft (col+1.2f, row, tmpStr);

		row -= 0.05f;

		sprintf( tmpStr, "T blk C=%d S=%d",
				 MemPoolCount( TBlock::pool ),
				 MemPoolSize( TBlock::pool ) );

		renderer->TextLeft (col, row, tmpStr);

		sprintf( tmpStr, "T LstE C=%d S=%d",
				 MemPoolCount( TListEntry::pool ),
				 MemPoolSize( TListEntry::pool ) );

		renderer->TextLeft (col+0.6f, row, tmpStr);

		sprintf( tmpStr, "T BlkL C=%d S=%d",
				 MemPoolCount( TBlockList::pool ),
				 MemPoolSize( TBlockList::pool ) );

		renderer->TextLeft (col+1.2f, row, tmpStr);

		row -= 0.05f;

		sprintf( tmpStr, "G OLOD C=%d S=%d",
				 MemPoolCount( ObjectLOD::pool ),
				 MemPoolSize( ObjectLOD::pool ) );

		renderer->TextLeft (col, row, tmpStr);

		sprintf( tmpStr, "G 3DLib C=%d S=%d",
				 MemPoolCount( glMemPool ),
				 MemPoolSize( glMemPool ) );

		renderer->TextLeft (col+0.6f, row, tmpStr);

		row -= 0.05f;

		sprintf( tmpStr, "S List C=%d S=%d",
				 MemPoolCount( displayList::pool ),
				 MemPoolSize( displayList::pool ) );

		renderer->TextLeft (col, row, tmpStr);

		sprintf( tmpStr, "S SfxR C=%d S=%d",
				 MemPoolCount( sfxRequest::pool ),
				 MemPoolSize( sfxRequest::pool ) );

		renderer->TextLeft (col+0.6f, row, tmpStr);

		sprintf( tmpStr, "S AirF C=%d S=%d",
				 MemPoolCount( AirframeClass::pool ),
				 MemPoolSize( AirframeClass::pool ) );

		renderer->TextLeft (col+1.2f, row, tmpStr);

		row -= 0.05f;

		sprintf( tmpStr, "S AirC C=%d S=%d",
				 MemPoolCount( AircraftClass::pool ),
				 MemPoolSize( AircraftClass::pool ) );

		objCount += MemPoolCount( AircraftClass::pool );

		renderer->TextLeft (col, row, tmpStr);

		sprintf( tmpStr, "S Miss C=%d S=%d",
				 MemPoolCount( MissileClass::pool ),
				 MemPoolSize(  MissileClass::pool ) );

		renderer->TextLeft (col+0.6f, row, tmpStr);

		sprintf( tmpStr, "S Bomb C=%d S=%d",
				 MemPoolCount( BombClass::pool ),
				 MemPoolSize( BombClass::pool ) );

		renderer->TextLeft (col+1.2f, row, tmpStr);

		row -= 0.05f;

		sprintf( tmpStr, "S SObj C=%d S=%d",
				 MemPoolCount( SimObjectType::pool ),
				 MemPoolSize( SimObjectType::pool ) );

		renderer->TextLeft (col, row, tmpStr);

		sprintf( tmpStr, "S Ldat C=%d S=%d",
				 MemPoolCount( SimObjectLocalData::pool ),
				 MemPoolSize(  SimObjectLocalData::pool ) );

		renderer->TextLeft (col+0.6f, row, tmpStr);

		sprintf( tmpStr, "S Sfx C=%d S=%d",
				 MemPoolCount( SfxClass::pool ),
				 MemPoolSize( SfxClass::pool ) );

		renderer->TextLeft (col+1.2f, row, tmpStr);

		row -= 0.05f;

		sprintf( tmpStr, "S Gun C=%d S=%d",
				 MemPoolCount( GunClass::pool ),
				 MemPoolSize( GunClass::pool ) );

		renderer->TextLeft (col, row, tmpStr);

		objCount += MemPoolCount( GroundClass::pool );

		sprintf( tmpStr, "S Grnd C=%d S=%d",
				 MemPoolCount( GroundClass::pool ),
				 MemPoolSize(  GroundClass::pool ) );

		renderer->TextLeft (col+0.6f, row, tmpStr);

		sprintf( tmpStr, "S GAI C=%d S=%d",
				 MemPoolCount( GNDAIClass::pool ),
				 MemPoolSize( GNDAIClass::pool ) );

		renderer->TextLeft (col+1.2f, row, tmpStr);

		row -= 0.05f;

		objCount += MemPoolCount( SimFeatureClass::pool );

		sprintf( tmpStr, "S Feat C=%d S=%d",
				 MemPoolCount( SimFeatureClass::pool ),
				 MemPoolSize( SimFeatureClass::pool ) );

		renderer->TextLeft (col, row, tmpStr);

		sprintf( tmpStr, "S MIFD C=%d S=%d",
				 MemPoolCount( MissileInFlightData::pool ),
				 MemPoolSize(  MissileInFlightData::pool ) );

		renderer->TextLeft (col+0.6f, row, tmpStr);

		sprintf( tmpStr, "S Dlst C=%d S=%d",
				 MemPoolCount( drawPtrList::pool ),
				 MemPoolSize( drawPtrList::pool ) );

		renderer->TextLeft (col+1.2f, row, tmpStr);

		row -= 0.05f;

		sprintf( tmpStr, "Waypnt C=%d S=%d",
				 MemPoolCount( WayPointClass::pool ),
				 MemPoolSize( WayPointClass::pool ) );

		renderer->TextLeft (col, row, tmpStr);

		sprintf( tmpStr, "SMSBase C=%d S=%d",
				 MemPoolCount( SMSBaseClass::pool ),
				 MemPoolSize(  SMSBaseClass::pool ) );

		renderer->TextLeft (col+0.6f, row, tmpStr);

		sprintf( tmpStr, "SMS C=%d S=%d",
				 MemPoolCount( SMSClass::pool ),
				 MemPoolSize( SMSClass::pool ) );

		renderer->TextLeft (col+1.2f, row, tmpStr);

		row -= 0.05f;

		sprintf( tmpStr, "BWeapn C=%d S=%d",
				 MemPoolCount( BasicWeaponStation::pool ),
				 MemPoolSize( BasicWeaponStation::pool ) );

		renderer->TextLeft (col, row, tmpStr);

		sprintf( tmpStr, "AWeapn C=%d S=%d",
				 MemPoolCount( AdvancedWeaponStation::pool ),
				 MemPoolSize(  AdvancedWeaponStation::pool ) );

		renderer->TextLeft (col+0.6f, row, tmpStr);

		sprintf( tmpStr, "Helos C=%d S=%d",
				 MemPoolCount( HelicopterClass::pool ),
				 MemPoolSize(  HelicopterClass::pool ) );

		objCount += MemPoolCount( HelicopterClass::pool );

		renderer->TextLeft (col+1.2f, row, tmpStr);

		row -= 0.05f;

		sprintf( tmpStr, "VM CONV C=%d S=%d",
				 MemPoolCount( CONVERSATION::pool ),
				 MemPoolSize( CONVERSATION::pool ) );

		renderer->TextLeft (col, row, tmpStr);

		sprintf( tmpStr, "VM BUFF C=%d S=%d",
				 MemPoolCount( VM_BUFFLIST::pool ),
				 MemPoolSize(  VM_BUFFLIST::pool ) );

		renderer->TextLeft (col+0.6f, row, tmpStr);

		sprintf( tmpStr, "VM CONV C=%d S=%d",
				 MemPoolCount( VM_CONVLIST::pool ),
				 MemPoolSize(  VM_CONVLIST::pool ) );

		renderer->TextLeft (col+1.2f, row, tmpStr);

		row -= 0.05f;

		sprintf( tmpStr, "RWY Que C=%d S=%d",
				 MemPoolCount( runwayQueueStruct::pool ),
				 MemPoolSize( runwayQueueStruct::pool ) );

		renderer->TextLeft (col, row, tmpStr);

		sprintf( tmpStr, "H BRAIN C=%d S=%d",
				 MemPoolCount( HeliBrain::pool ),
				 MemPoolSize(  HeliBrain::pool ) );

		renderer->TextLeft (col+0.6f, row, tmpStr);

		sprintf( tmpStr, "AF DATA C=%d S=%d",
				 MemPoolCount( AirframeDataPool ),
				 MemPoolSize(  AirframeDataPool ) );

		renderer->TextLeft (col+1.2f, row, tmpStr);

		row -= 0.05f;

		sprintf( tmpStr, "3P Lim C=%d S=%d",
				 MemPoolCount( ThreePointLimiter::pool ),
				 MemPoolSize( ThreePointLimiter::pool ) );

		renderer->TextLeft (col, row, tmpStr);

		sprintf( tmpStr, "Val Lim C=%d S=%d",
				 MemPoolCount( ValueLimiter::pool ),
				 MemPoolSize(  ValueLimiter::pool ) );

		renderer->TextLeft (col+0.6f, row, tmpStr);

		sprintf( tmpStr, "Pct Lim C=%d S=%d",
				 MemPoolCount( PercentLimiter::pool ),
				 MemPoolSize(  PercentLimiter::pool ) );

		renderer->TextLeft (col+1.2f, row, tmpStr);

		row -= 0.05f;

		sprintf( tmpStr, "Line Lim C=%d S=%d",
				 MemPoolCount( LineLimiter::pool ),
				 MemPoolSize( LineLimiter::pool ) );

		renderer->TextLeft (col, row, tmpStr);

		sprintf( tmpStr, "Vu Drv C=%d S=%d",
				 MemPoolCount( SimVuDriver::pool ),
				 MemPoolSize(  SimVuDriver::pool ) );

		renderer->TextLeft (col+0.6f, row, tmpStr);

		sprintf( tmpStr, "Vu Slave C=%d S=%d",
				 MemPoolCount( SimVuSlave::pool ),
				 MemPoolSize(  SimVuSlave::pool ) );

		renderer->TextLeft (col+1.2f, row, tmpStr);

		row -= 0.05f;

		sprintf( tmpStr, "Obj Geom C=%d S=%d",
				 MemPoolCount( ObjectGeometry::pool ),
				 MemPoolSize( ObjectGeometry::pool ) );

		renderer->TextLeft (col, row, tmpStr);

		sprintf( tmpStr, "List Class C=%d S=%d",
				 MemPoolCount( ListClass::pool ),
				 MemPoolSize( ListClass::pool ) );

		renderer->TextLeft (col+0.6f, row, tmpStr);

		sprintf( tmpStr, "List Elem C=%d S=%d",
				 MemPoolCount( ListElementClass::pool ),
				 MemPoolSize( ListElementClass::pool ) );

		renderer->TextLeft (col+1.2f, row, tmpStr);

		row -= 0.05f;


		sprintf( tmpStr, "VuLinkNode C=%d S=%d",
				 MemPoolCount( VuLinkNode::pool ),
				 MemPoolSize( VuLinkNode::pool ) );

		renderer->TextLeft (col, row, tmpStr);

		sprintf( tmpStr, "VuRBNode C=%d S=%d",
				 MemPoolCount( vuRBNodepool ),
				 MemPoolSize( vuRBNodepool ) );

		renderer->TextLeft (col+0.6f, row, tmpStr);

		sprintf( tmpStr, "DOF Data C=%d S=%d",
				 MemPoolCount( graphicsDOFDataPool ),
				 MemPoolSize( graphicsDOFDataPool ) );

		renderer->TextLeft (col+1.2f, row, tmpStr);

		row -= 0.05f;


//		sprintf( tmpStr, "Loadout C=%d S=%d",
//				 MemPoolCount( LoadoutStruct::pool ),
//				 MemPoolSize( LoadoutStruct::pool ) );

		renderer->TextLeft (col, row, tmpStr);

		sprintf( tmpStr, "Vu Message C=%d S=%d",
				 MemPoolCount( gVuMsgMemPool ),
				 MemPoolSize( gVuMsgMemPool ) );

		renderer->TextLeft (col+0.6f, row, tmpStr);

		sprintf( tmpStr, "Unit Deagg C=%d S=%d",
				 MemPoolCount( UnitDeaggregationData::pool ),
				 MemPoolSize( UnitDeaggregationData::pool ) );

		renderer->TextLeft (col+1.2f, row, tmpStr);

		row -= 0.05f;

		sprintf( tmpStr, "Division C=%d S=%d",
				 MemPoolCount( DivisionClass::pool ),
				 MemPoolSize( DivisionClass::pool ) );

		renderer->TextLeft (col, row, tmpStr);

		sprintf( tmpStr, "Miss Req C=%d S=%d",
				 MemPoolCount( MissionRequestClass::pool ),
				 MemPoolSize( MissionRequestClass::pool ) );

		renderer->TextLeft (col+0.6f, row, tmpStr);

		sprintf( tmpStr, "Event El C=%d S=%d",
				 MemPoolCount( EventElement::pool ),
				 MemPoolSize( EventElement::pool ) );

		renderer->TextLeft (col+1.2f, row, tmpStr);

		row -= 0.05f;

		if ( gDivVUIDs )
		{
			sprintf( tmpStr, "Div VUIDs C=%d S=%d",
				 MemPoolCount( gDivVUIDs ),
				 MemPoolSize( gDivVUIDs ) );

			renderer->TextLeft (col, row, tmpStr);
		}

		sprintf( tmpStr, "Faults C=%d S=%d",
				 MemPoolCount( gFaultMemPool ),
				 MemPoolSize( gFaultMemPool ) );

		renderer->TextLeft (col+0.6f, row, tmpStr);

		sprintf( tmpStr, "Text C=%d S=%d",
				 MemPoolCount( gTextMemPool ),
				 MemPoolSize( gTextMemPool ) );

		renderer->TextLeft (col+1.2f, row, tmpStr);

		row -= 0.05f;

		sprintf( tmpStr, "TPosts C=%d S=%d",
				 MemPoolCount( gTPostMemPool ),
				 MemPoolSize( gTPostMemPool ) );

		renderer->TextLeft (col, row, tmpStr);

		sprintf( tmpStr, "FarTex C=%d S=%d",
				 MemPoolCount( gFartexMemPool ),
				 MemPoolSize( gFartexMemPool ) );

		renderer->TextLeft (col+0.6f, row, tmpStr);

		sprintf( tmpStr, "Cock C=%d S=%d",
				 MemPoolCount( gCockMemPool ),
				 MemPoolSize( gCockMemPool ) );

		renderer->TextLeft (col+1.2f, row, tmpStr);

		row -= 0.05f;

		sprintf( tmpStr, "BSPLib C=%d S=%d",
				 MemPoolCount( gBSPLibMemPool ),
				 MemPoolSize( gBSPLibMemPool ) );

		renderer->TextLeft (col, row, tmpStr);

		sprintf( tmpStr, "Obj Heap C=%d S=%d",
				 MemPoolCount( gObjMemPool ),
				 MemPoolSize( gObjMemPool ) );

		renderer->TextLeft (col+0.6f, row, tmpStr);

		sprintf( tmpStr, "ReadIn C=%d S=%d",
				 MemPoolCount( gReadInMemPool ),
				 MemPoolSize( gReadInMemPool ) );

		renderer->TextLeft (col+1.2f, row, tmpStr);

		row -= 0.05f;

		sprintf( tmpStr, "ResMgr C=%d S=%d",
				 MemPoolCount( gResmgrMemPool ),
				 MemPoolSize( gResmgrMemPool ) );

		renderer->TextLeft (col, row, tmpStr);

		sprintf( tmpStr, "TexDB C=%d S=%d",
				 MemPoolCount( gTexDBMemPool ),
				 MemPoolSize( gTexDBMemPool ) );

		renderer->TextLeft (col+0.6f, row, tmpStr);

		sprintf( tmpStr, "ATC Brain C=%d S=%d",
				 MemPoolCount( ATCBrain::pool ),
				 MemPoolSize( ATCBrain::pool ) );

		renderer->TextLeft (col+1.2f, row, tmpStr);

		row -= 0.05f;

		sprintf( tmpStr, "Palette C=%d S=%d",
				 MemPoolCount( Palette::pool ),
				 MemPoolSize( Palette::pool ) );

		renderer->TextLeft (col, row, tmpStr);

		sprintf( tmpStr, "Sound C=%d S=%d",
				 MemPoolCount( gSoundMemPool ),
				 MemPoolSize( gSoundMemPool ) );

		renderer->TextLeft (col+0.6f, row, tmpStr);

		sprintf( tmpStr, "Vu Filter C=%d S=%d",
				 MemPoolCount( gVuFilterMemPool ),
				 MemPoolSize( gVuFilterMemPool ) );

		renderer->TextLeft (col+1.2f, row, tmpStr);

		row -= 0.05f;

		sprintf( tmpStr, "Input C=%d S=%d",
				 MemPoolCount( gInputMemPool ),
				 MemPoolSize( gInputMemPool ) );

		renderer->TextLeft (col, row, tmpStr);

		sprintf( tmpStr, "Tacan C=%d S=%d",
				 MemPoolCount( gTacanMemPool ),
				 MemPoolSize( gTacanMemPool ) );

		renderer->TextLeft (col+0.6f, row, tmpStr);

		row -= 0.05f;

#ifdef DEBUG
		sprintf( tmpStr, "SimObj Nodes=%d SimObj Refs=%d",
				 ObjectNodes,
				 ObjectReferences );
#endif
		renderer->TextLeft (col, row, tmpStr);

		row -= 0.05f;

		sprintf( tmpStr, "Total Object Count %d", objCount );

		renderer->TextLeft (col, row, tmpStr);

		 totCount += MemPoolCount( ObjectiveClass::pool );
		 totSize += MemPoolSize( ObjectiveClass::pool ) ;

		 totCount += MemPoolCount( BattalionClass::pool );
		 totSize += MemPoolSize( BattalionClass::pool ) ;

		 totCount += MemPoolCount( BrigadeClass::pool );
		 totSize += MemPoolSize( BrigadeClass::pool ) ;

		 totCount += MemPoolCount( FlightClass::pool );
		 totSize += MemPoolSize( FlightClass::pool ) ;

		 totCount += MemPoolCount( SquadronClass::pool );
		 totSize += MemPoolSize( SquadronClass::pool ) ;

		 totCount += MemPoolCount( PackageClass::pool );
		 totSize += MemPoolSize( PackageClass::pool ) ;

		 totCount += MemPoolCount( TaskForceClass::pool );
		 totSize += MemPoolSize( TaskForceClass::pool ) ;

		 totCount += MemPoolCount( SimPersistantClass::pool );
		 totSize += MemPoolSize( SimPersistantClass::pool ) ;

		 totCount += MemPoolCount( Drawable2D::pool );
		 totSize += MemPoolSize( Drawable2D::pool ) ;

		 totCount += MemPoolCount( DrawableTracer::pool );
		 totSize += MemPoolSize( DrawableTracer::pool ) ;

		 totCount += MemPoolCount( DrawableGroundVehicle::pool );
		 totSize += MemPoolSize( DrawableGroundVehicle::pool ) ;

		 totCount += MemPoolCount( DrawableBuilding::pool );
		 totSize += MemPoolSize( DrawableBuilding::pool ) ;

		 totCount += MemPoolCount( DrawableBSP::pool );
		 totSize += MemPoolSize( DrawableBSP::pool ) ;

		 totCount += MemPoolCount( DrawableShadowed::pool );
		 totSize += MemPoolSize( DrawableShadowed::pool ) ;

		 totCount += MemPoolCount( DrawableBridge::pool );
		 totSize += MemPoolSize( DrawableBridge::pool ) ;

		 totCount += MemPoolCount( DrawableOvercast::pool );
		 totSize += MemPoolSize( DrawableOvercast::pool ) ;

		 totCount += MemPoolCount( DrawablePlatform::pool );
		 totSize += MemPoolSize( DrawablePlatform::pool ) ;

		 totCount += MemPoolCount( DrawableRoadbed::pool );
		 totSize += MemPoolSize( DrawableRoadbed::pool ) ;

		 totCount += MemPoolCount( DrawableTrail::pool );
		 totSize += MemPoolSize( DrawableTrail::pool ) ;

		 totCount += MemPoolCount( TrailElement::pool );
		 totSize += MemPoolSize( TrailElement::pool ) ;

		 totCount += MemPoolCount( DrawablePuff::pool );
		 totSize += MemPoolSize( DrawablePuff::pool ) ;

		 totCount += MemPoolCount( DrawablePoint::pool );
		 totSize += MemPoolSize( DrawablePoint::pool ) ;

		 totCount += MemPoolCount( DrawableGuys::pool );
		 totSize += MemPoolSize( DrawableGuys::pool ) ;

		 totCount += MemPoolCount( TBlock::pool );
		 totSize += MemPoolSize( TBlock::pool ) ;

		 totCount += MemPoolCount( TListEntry::pool );
		 totSize += MemPoolSize( TListEntry::pool ) ;

		 totCount += MemPoolCount( TBlockList::pool );
		 totSize += MemPoolSize( TBlockList::pool ) ;

		 totCount += MemPoolCount( ObjectLOD::pool );
		 totSize += MemPoolSize( ObjectLOD::pool ) ;

		 totCount += MemPoolCount( glMemPool );
		 totSize += MemPoolSize( glMemPool ) ;

		 totCount += MemPoolCount( displayList::pool );
		 totSize += MemPoolSize( displayList::pool ) ;

		 totCount += MemPoolCount( sfxRequest::pool );
		 totSize += MemPoolSize( sfxRequest::pool ) ;

		 totCount += MemPoolCount( AirframeClass::pool );
		 totSize += MemPoolSize( AirframeClass::pool ) ;

		 totCount += MemPoolCount( AircraftClass::pool );
		 totSize += MemPoolSize( AircraftClass::pool ) ;

		 totCount += MemPoolCount( MissileClass::pool );
		 totSize += MemPoolSize(  MissileClass::pool ) ;

		 totCount += MemPoolCount( BombClass::pool );
		 totSize += MemPoolSize( BombClass::pool ) ;

		 totCount += MemPoolCount( SimObjectType::pool );
		 totSize += MemPoolSize( SimObjectType::pool ) ;

		 totCount += MemPoolCount( SimObjectLocalData::pool );
		 totSize += MemPoolSize(  SimObjectLocalData::pool ) ;

		 totCount += MemPoolCount( SfxClass::pool );
		 totSize += MemPoolSize( SfxClass::pool ) ;

		 totCount += MemPoolCount( GunClass::pool );
		 totSize += MemPoolSize( GunClass::pool ) ;

		 totCount += MemPoolCount( GroundClass::pool );
		 totSize += MemPoolSize(  GroundClass::pool ) ;

		 totCount += MemPoolCount( GNDAIClass::pool );
		 totSize += MemPoolSize( GNDAIClass::pool ) ;

		 totCount += MemPoolCount( SimFeatureClass::pool );
		 totSize += MemPoolSize( SimFeatureClass::pool ) ;

		 totCount += MemPoolCount( MissileInFlightData::pool );
		 totSize += MemPoolSize(  MissileInFlightData::pool ) ;

		 totCount += MemPoolCount( drawPtrList::pool );
		 totSize += MemPoolSize( drawPtrList::pool ) ;

		 totCount += MemPoolCount( WayPointClass::pool );
		 totSize += MemPoolSize( WayPointClass::pool ) ;

		 totCount += MemPoolCount( SMSBaseClass::pool );
		 totSize += MemPoolSize(  SMSBaseClass::pool ) ;

		 totCount += MemPoolCount( SMSClass::pool );
		 totSize += MemPoolSize( SMSClass::pool ) ;

		 totCount += MemPoolCount( BasicWeaponStation::pool );
		 totSize += MemPoolSize( BasicWeaponStation::pool ) ;

		 totCount += MemPoolCount( AdvancedWeaponStation::pool );
		 totSize += MemPoolSize(  AdvancedWeaponStation::pool ) ;

		 totCount += MemPoolCount( HelicopterClass::pool );
		 totSize += MemPoolSize(  HelicopterClass::pool ) ;

		 totCount += MemPoolCount( CONVERSATION::pool );
		 totSize += MemPoolSize( CONVERSATION::pool ) ;

		 totCount += MemPoolCount( VM_BUFFLIST::pool );
		 totSize += MemPoolSize(  VM_BUFFLIST::pool ) ;

		 totCount += MemPoolCount( VM_CONVLIST::pool );
		 totSize += MemPoolSize(  VM_CONVLIST::pool ) ;

		 totCount += MemPoolCount( runwayQueueStruct::pool );
		 totSize += MemPoolSize( runwayQueueStruct::pool ) ;

		 totCount += MemPoolCount( HeliBrain::pool );
		 totSize += MemPoolSize(  HeliBrain::pool ) ;

		 totCount += MemPoolCount( AirframeDataPool );
		 totSize += MemPoolSize(  AirframeDataPool ) ;

		 totCount += MemPoolCount( ThreePointLimiter::pool );
		 totSize += MemPoolSize( ThreePointLimiter::pool ) ;

		 totCount += MemPoolCount( ValueLimiter::pool );
		 totSize += MemPoolSize(  ValueLimiter::pool ) ;

		 totCount += MemPoolCount( PercentLimiter::pool );
		 totSize += MemPoolSize(  PercentLimiter::pool ) ;

		 totCount += MemPoolCount( LineLimiter::pool );
		 totSize += MemPoolSize( LineLimiter::pool ) ;

		 totCount += MemPoolCount( SimVuDriver::pool );
		 totSize += MemPoolSize(  SimVuDriver::pool ) ;

		 totCount += MemPoolCount( SimVuSlave::pool );
		 totSize += MemPoolSize(  SimVuSlave::pool ) ;

		 totCount += MemPoolCount( ObjectGeometry::pool );
		 totSize += MemPoolSize( ObjectGeometry::pool ) ;

		 totCount += MemPoolCount( ListClass::pool );
		 totSize += MemPoolSize( ListClass::pool ) ;

		 totCount += MemPoolCount( ListElementClass::pool );
		 totSize += MemPoolSize( ListElementClass::pool ) ;

		 totCount += MemPoolCount( VuLinkNode::pool );
		 totSize += MemPoolSize( VuLinkNode::pool ) ;

		 totCount += MemPoolCount( vuRBNodepool );
		 totSize += MemPoolSize( vuRBNodepool ) ;

		 totCount += MemPoolCount( graphicsDOFDataPool );
		 totSize += MemPoolSize( graphicsDOFDataPool ) ;

//		 totCount += MemPoolCount( LoadoutStruct::pool );
//		 totSize += MemPoolSize( LoadoutStruct::pool ) ;

		 totCount += MemPoolCount( gVuMsgMemPool );
		 totSize += MemPoolSize( gVuMsgMemPool ) ;

		 totCount += MemPoolCount( UnitDeaggregationData::pool );
		 totSize += MemPoolSize( UnitDeaggregationData::pool ) ;

		 totCount += MemPoolCount( DivisionClass::pool );
		 totSize += MemPoolSize( DivisionClass::pool ) ;

		 totCount += MemPoolCount( MissionRequestClass::pool );
		 totSize += MemPoolSize( MissionRequestClass::pool ) ;

		 totCount += MemPoolCount( EventElement::pool );
		 totSize += MemPoolSize( EventElement::pool ) ;

		if ( gDivVUIDs )
		{
 			totCount += MemPoolCount( gDivVUIDs );
		 	totSize += MemPoolSize( gDivVUIDs ) ;
		}

		 totCount += MemPoolCount( gFaultMemPool );
		 totSize += MemPoolSize( gFaultMemPool ) ;

		 totCount += MemPoolCount( gTextMemPool );
		 totSize += MemPoolSize( gTextMemPool ) ;

		 totCount += MemPoolCount( gTPostMemPool );
		 totSize += MemPoolSize( gTPostMemPool ) ;

		 totCount += MemPoolCount( gFartexMemPool );
		 totSize += MemPoolSize( gFartexMemPool ) ;

		 totCount += MemPoolCount( gCockMemPool );
		 totSize += MemPoolSize( gCockMemPool ) ;

		 totCount += MemPoolCount( gBSPLibMemPool );
		 totSize += MemPoolSize( gBSPLibMemPool ) ;

		 totCount += MemPoolCount( gObjMemPool );
		 totSize += MemPoolSize( gObjMemPool ) ;

		 totCount += MemPoolCount( gReadInMemPool );
		 totSize += MemPoolSize( gReadInMemPool ) ;

		 totCount += MemPoolCount( gResmgrMemPool );
		 totSize += MemPoolSize( gResmgrMemPool ) ;

		 totCount += MemPoolCount( gTexDBMemPool );
		 totSize += MemPoolSize( gTexDBMemPool ) ;

		 totCount += MemPoolCount( Palette::pool );
		 totSize += MemPoolSize( Palette::pool ) ;

		 totCount += MemPoolCount( ATCBrain::pool );
		 totSize += MemPoolSize( ATCBrain::pool ) ;

		 totCount += MemPoolCount( gSoundMemPool );
		 totSize += MemPoolSize( gSoundMemPool ) ;

		 totCount += MemPoolCount( gInputMemPool );
		 totSize += MemPoolSize( gInputMemPool ) ;

		 totCount += MemPoolCount( gTacanMemPool );
		 totSize += MemPoolSize( gTacanMemPool ) ;

		 totCount += MemPoolCount( gVuFilterMemPool );
		 totSize += MemPoolSize( gVuFilterMemPool ) ;

		row -= 0.05f;

		sprintf( tmpStr, "Total Falc Pool Count %d, Size %d", totCount, totSize );

		renderer->TextLeft (col, row, tmpStr);

		row -= 0.05f;

		sprintf( tmpStr, "Total Falc+Def Pool Count %d, Size %d",
			 MemPoolCount( MemDefaultPool ) + totCount,
			 MemPoolSize( MemDefaultPool ) + totSize );


		renderer->TextLeft (col, row, tmpStr);

}
#endif // MEM_DEBUG
