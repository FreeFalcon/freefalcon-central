#include <time.h>
#include "stdhdr.h"
#include "otwdrive.h"
#include "Graphics\Include\renderow.h"
#include "Graphics\Include\drawbsp.h"
#include "simdrive.h"
#include "mesg.h"
#include "MsgInc\AWACSMsg.h"
#include "MsgInc\ATCMsg.h"
#include "MsgInc\FACMsg.h"
#include "MsgInc\TankerMsg.h"
#include "falcmesg.h"
#include "aircrft.h"
#include "falclib\include\f4find.h"

#include "simio.h"	// Retro 25Mar2004

#include "fsound.h"
#include "fakerand.h"
#include "dogfight.h"
#include "falcsess.h"
#include "wingorder.h"
#include "classtbl.h"
#include "TimerThread.h"
#include "F4Version.h"
#include "ui\include\uicomms.h"
#include "entity.h"
#include "airframe.h"

extern int MajorVersion;
extern int MinorVersion;
extern int BuildNumber;
extern int ShowVersion;
extern int ShowFrameRate;
extern int endAbort;	// From OTWdrive.cpp
extern DrawableBSP *endDialogObject;
int endsAvail[3] = {0};
int	start = 0;
extern int CommandsKeyCombo;
extern int CommandsKeyComboMod;
static int exitMenuDesired = 0;

int tactical_is_training (void);

extern char FalconPictureDirectory[_MAX_PATH]; // JB 010623

#define EXITMENU_POPUP_TIME		15000			// Exit menu will pop up 15 seconds after death

void ResetVoices(void);

#include "sim\include\IVibeData.h"
extern IntellivibeData g_intellivibeData;
extern void *gSharedIntellivibe;
extern bool g_bShowFlaps;

void OTWDriverClass::ShowVersionString (void)
{
char verStr[24];

   if (ShowVersion == 1)
      sprintf (verStr, "%d.%02d", MajorVersion, MinorVersion);
   else
      sprintf (verStr, "%d.%02d%d%c", MajorVersion, MinorVersion, gLangIDNum);
   renderer->SetColor (0xff00ff00);
   renderer->TextCenter (-0.9F, 0.9F, verStr);
}

void OTWDriverClass::ShowPosition (void)
{
char posStr[40];

   renderer->SetColor (0xff00ff00);
   sprintf (posStr, "      X = %10.2f", flyingEye->XPos() / FEET_PER_KM);
   renderer->TextRight (0.95F, 0.95F, posStr);
   sprintf (posStr, "      Y = %10.2f", flyingEye->YPos() / FEET_PER_KM);
   renderer->TextRight (0.95F, 0.90F, posStr);
   sprintf (posStr, "      Z = %10.2f", flyingEye->ZPos());
   renderer->TextRight (0.95F, 0.85F, posStr);
   sprintf (posStr, "Heading = %10.2f", flyingEye->Yaw() * RTD);
   renderer->TextRight (0.95F, 0.80F, posStr);
   sprintf (posStr, "  Pitch = %10.2f", flyingEye->Pitch() * RTD);
   renderer->TextRight (0.95F, 0.75F, posStr);
   sprintf (posStr, "   Roll = %10.2f", flyingEye->Roll() * RTD); // 2002-01-31 ADDED BY S.G. Added roll to the readout
   renderer->TextRight (0.95F, 0.70F, posStr);
}


void OTWDriverClass::ShowAerodynamics (void)
{
    char posStr[120];
    
    if(otwPlatform && 
	otwPlatform == SimDriver.playerEntity && 
	otwPlatform->IsAirplane())
	
	{
	AirframeClass *af = ((AircraftClass*)otwPlatform)->af;
	renderer->SetColor (0xff00ff00);
	sprintf (posStr, "Cd %8.4f Cl %8.4f Cy %8.4f Mu %8.4f", af->Cd(), af->Cl(), af->Cy(), af->mu);
	renderer->TextRight (0.95F, 0.65F, posStr);
	sprintf (posStr, "XDrag %6.4f Thrust %8.1f Mass %8.1f", af->XSAero(), af->Thrust()*af->Mass(), af->Mass());
	renderer->TextRight (0.95F, 0.60F, posStr);
	sprintf (posStr, "AoABias %5.2f Lift %5.2f Down %6.4f", af->AOABias(), -af->ZSAero(), af->ZSProp());
	renderer->TextRight (0.95F, 0.55F, posStr);
	sprintf (posStr, "AOA %5.2f Tef %10.4f Lef %10.4f", af->alpha, af->tefFactor, af->lefFactor);
	renderer->TextRight (0.95F, 0.50F, posStr);
	}
	
}

void OTWDriverClass::ShowFlaps(void)
{
    char posStr[120];
    
	//TJL 11/09/03 Removing AOA since AOA is now in HUD for non-F16's
	// Making this always on and with a keystroke to turn it off
	// Human players need to know flap positions and most don't know about
	// g_bShowFlaps.
    if(otwPlatform && 
	otwPlatform == SimDriver.playerEntity && 
	otwPlatform->IsAirplane()) {
	AirframeClass *af = ((AircraftClass*)otwPlatform)->af;
	if (af->HasManualFlaps()) 
	{
	    renderer->SetColor (0xff00ff00);
		//TJL 02/28/04 F18 AUTO/HALF/FULL code
		if (af->flapPos == 20)
		{ 
			sprintf (posStr, "HALF MODE Flaps %3.0f LEFs %3.0f",
			af->TefDegrees(), 
			af->LefDegrees());
		}
		else if (af->flapPos == 30)
		{
			sprintf (posStr, "FULL MODE Flaps %3.0f LEFs %3.0f",
			af->TefDegrees(), 
			af->LefDegrees());
		}
		else if (af->flapPos == 10)
		{
			sprintf (posStr, "AUTO MODE Flaps %3.0f LEFs %3.0f",
			af->TefDegrees(), 
			af->LefDegrees());
		}
		else
		{
			sprintf (posStr, "Flaps %3.0f LEFs %3.0f",
			af->TefDegrees(), 
			af->LefDegrees());
		}
	    renderer->TextLeft (-0.95F, 0.90F, posStr);
	}
	else
		showFlaps = true;	// Retro 1Feb2004 so that we don´t enter here if the ac has no flaps anyway
    }
}

// Retro 1Feb2004 start
//	display some dual-throttle debug stuff
#include "PilotInputs.h"
void OTWDriverClass::ShowEngine(void)
{
	if	(otwPlatform && 
		otwPlatform == SimDriver.playerEntity && 
		otwPlatform->IsAirplane())
	{
		AirframeClass *af = ((AircraftClass*)otwPlatform)->af;
		if (af->GetNumberEngines() == 2)
		{
			renderer->SetColor (0xff00ff00);
			char tmp[80];
			//TJL 02/16/04 Added RPM output to engine select
			switch (UserStickInputs.getCurrentEngine())
			{
			case PilotInputs::Left_Engine:	sprintf(tmp,"Left Engine rpm %5.2f rpm2 %5.2f", af->rpm, af->rpm2); break;
			case PilotInputs::Right_Engine:	sprintf(tmp,"Right Engine rpm %5.2f rpm2 %5.2f", af->rpm, af->rpm2); break;
			case PilotInputs::Both_Engines:	sprintf(tmp,"Both Engines rpm %5.2f rpm2 %5.2f", af->rpm, af->rpm2); break;
			default:	sprintf(tmp,"Unknown");break;
			}

//			renderer->TextLeft(0.1F,0.9F,tmp);
			renderer->TextLeft (-0.95F, 0.88F, tmp);
			
		}
		else
			showEngine = false;
	}
}
// Retro 1Feb2004 end

// Retro 25Feb2004 start
struct {
	GameAxis_t theAxis;
	char* theText;
} Axis2Text[] = {
	{ AXIS_PITCH,			"AXIS_PITCH" },
	{ AXIS_ROLL,			"AXIS_ROLL" },
	{ AXIS_YAW,				"AXIS_YAW" },
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
//	{ AXIS_INTERCOM_VOLUME,	"AXIS_INTERCOM_VOLUME" }
};

void OTWDriverClass::DisplayAxisValues()
{
extern GameAxisSetup_t AxisSetup[AXIS_MAX];

	OTWDriver.renderer->SetColor(0xFFFF0000);
	
	OTWDriver.renderer->TextLeft(-0.75F,0.9F,"GetAxisValue");
	OTWDriver.renderer->TextLeft(-0.60F,0.9F,"ReadAnalog");
	OTWDriver.renderer->TextLeft(-0.45F,0.9F,"Center");
	OTWDriver.renderer->TextLeft(-0.30F,0.9F,"Cutoff");
	OTWDriver.renderer->TextLeft(-0.15F,0.9F,"Smoothing");
	for (int i = AXIS_START; i < AXIS_MAX; i++)
	{
		char tmp[128] = {0};

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
					OTWDriver.renderer->TextLeft(0.0F,0.85F-(i*0.05f),tmp);
				}
			}

//			sprintf(tmp,"%i",IO.analog[i].smoothingFactor);
			OTWDriver.renderer->TextLeft(-0.15F,0.85F-(i*0.05F),tmp);
		}
	}
}
// Retro 25Feb2004 end

void OTWDriverClass::TakeScreenShot(void)
{
char fileName[_MAX_PATH];
char tmpStr[_MAX_PATH];
time_t ltime;
struct tm* today;

   time( &ltime );
   takeScreenShot = FALSE;
   today = localtime (&ltime);
   //strftime( tmpStr, _MAX_PATH-1,"%m_%d_%Y-%H_%M_%S", today );
   strftime( tmpStr, _MAX_PATH-1, "%Y-%m-%d_%H%M%S", today ); //THW Let's have ISO-Dates! Darn Americans :)
   //MI put them where they belong
#if 0
   sprintf (fileName, "%s\\%s", FalconDataDirectory, tmpStr);
#else
   sprintf (fileName, "%s\\%s", FalconPictureDirectory, tmpStr);
#endif

   OTWImage->BackBufferToRAW(fileName);
}

// NOTE Exit Menu looks like this
//
//    End     Mission
//
//    Resume  Mission
//
//    Discard Mission
//

void OTWDriverClass::DrawExitMenu (void)
{
Tpoint origin;
int oldState;

   float tempFOV = GetFOV();		//Wombat778 3-26-04 Save the current FOV;
   SetFOV(45.0f*DTR);				//Wombat778 3-26-04 Set the FOV to 45 degrees to make it not dark (temporary fix till jam comes up with the real solution, then change to 60.0f)


   if(exitMenuOn != exitMenuDesired)
   {
      ChangeExitMenu (exitMenuDesired);
   }

   if (exitMenuOn)
   {
	   ShiAssert( endDialogObject );

      if (SimDriver.RunningDogfight())
      {
         // TODO: Show final score board
         endDialogObject->SetSwitchMask( 0, TRUE);
         endDialogObject->SetSwitchMask( 1, FALSE);
         endDialogObject->SetSwitchMask( 2, FALSE);
         endsAvail[0] = TRUE;
         endsAvail[1] = FALSE;
         endsAvail[2] = FALSE;
      }
      else if (SimDriver.RunningInstantAction())
      {
         endDialogObject->SetSwitchMask( 0, TRUE);
         endDialogObject->SetSwitchMask( 2, FALSE);
         endsAvail[0] = TRUE;
         endsAvail[2] = FALSE;

         if (SimDriver.playerEntity)
         {
            endDialogObject->SetSwitchMask( 1, TRUE);
            endsAvail[1] = TRUE;
         }
         else
         {
            endDialogObject->SetSwitchMask( 1, FALSE);
            endsAvail[1] = FALSE;
         }
      }
      else if (SimDriver.RunningCampaignOrTactical())
      {
         endDialogObject->SetSwitchMask( 0, TRUE);
         endDialogObject->SetSwitchMask( 1, TRUE);
         endsAvail[0] = TRUE;
         endsAvail[1] = TRUE;
		 if (tactical_is_training() || (gCommsMgr && gCommsMgr->Online()))
            {
            endDialogObject->SetSwitchMask( 2, FALSE);
            endsAvail[2] = FALSE;
            }
         else
            {
            endDialogObject->SetSwitchMask( 2, TRUE);
            endsAvail[2] = TRUE;
            }

      }

      oldState = renderer->GetObjectTextureState();
	  renderer->SetObjectTextureState( TRUE );
      renderer->StartFrame();
	  renderer->SetViewport(-1.0F, 1.0F, 1.0F, -1.0F);
	  renderer->SetCamera( &origin, &IMatrix );
	  endDialogObject->Draw( renderer );
      renderer->FinishFrame();
	  renderer->SetObjectTextureState( oldState );
   }

   SetFOV(tempFOV);			//Wombat778 3-26-04 Restore the FOV
}

void OTWDriverClass::Timeout (void)
{
	SetExitMenu( FALSE );

	// See if we were already on the way out...
	if (endFlightTimer)
	{
		// Just ignore this case - we are exiting and that's all we care about
	}
	else
	{
		// end in 5 seconds
		endFlightTimer = vuxRealTime + 5000;
		ResetVoices();
	}
	
	// no hud when ending flight -- may want to make sure other stuff
	// isn't set too....
	SetOTWDisplayMode(ModeNone);
	
	// Get out of eyeFly if we were in it
	if (eyeFly)
	{
#if 1
		ToggleEyeFly();
#else
		otwPlatform = lastotwPlatform;
		lastotwPlatform = NULL;
		eyeFly = FALSE;
#endif
	}

	// if we've got an otwplatform (ie the f16) jump out ahead of it for
	// a ways to get a fly-by effect
	if ( endFlightPointSet == FALSE )
	{
		if ( otwPlatform )
		{
//			MonoPrint ("Panning exit\n");
			SetEndFlightPoint( otwPlatform->XPos() + otwPlatform->dmx[0][0] * 10.0f + otwPlatform->XDelta() * 2.0f,
							   otwPlatform->YPos() + otwPlatform->dmx[0][1] * 10.0f + otwPlatform->YDelta() * 2.0f,
							   otwPlatform->ZPos() + otwPlatform->dmx[0][2] * 10.0f + otwPlatform->ZDelta() * 2.0f - 20.0f );
			SetEndFlightVec(0.0f, 0.0f, 0.0f);
		}
		else
		{
//          MonoPrint ("Fixed exit\n");
			// not otwplatform, use last focus point with some randomness...
			SetEndFlightPoint(	focusPoint.x + 100.0f * PRANDFloat(), 
								focusPoint.y + 100.0f * PRANDFloat(), 
								focusPoint.z - 100.0f );
			SetEndFlightVec(0.0f, 0.0f, 0.0f);
		}
	}
}

void OTWDriverClass::ExitMenu (unsigned long i)
{
   if (i == DIK_ESCAPE)
   {
      SetExitMenu( FALSE );
   }
   else if ((i == DIK_E && endsAvail[0]) || (i == DIK_D && endsAvail[2]))
   {
     g_intellivibeData.IsEndFlight = true;
		 memcpy (gSharedIntellivibe, &g_intellivibeData, sizeof(g_intellivibeData));

     if (i == DIK_D || tactical_is_training())
         endAbort = TRUE;

      SetExitMenu( FALSE );

      // if already set end now
      if ( endFlightTimer || !gameCompressionRatio )
      {
   	   endFlightTimer = vuxRealTime;
      }
      else
      {
         // end in 5 seconds
         endFlightTimer = vuxRealTime + 5000;
		 ResetVoices();
      }

       // no hud when ending flight -- may want to make sure other stuff
      // isn't set too....
      SetOTWDisplayMode(ModeChase);

      // if we've got an otwplatform (ie the f16) jump out ahead of it for
      // a ways to get a fly-by effect
      if ( endFlightPointSet == FALSE )
      {
	      if ( otwPlatform )
	      {
//			   MonoPrint ("Panning exit\n");
			   SetEndFlightPoint( otwPlatform->XPos() + otwPlatform->dmx[0][0] * 10.0f + otwPlatform->XDelta() * 2.0f,
			                      otwPlatform->YPos() + otwPlatform->dmx[0][1] * 10.0f + otwPlatform->YDelta() * 2.0f,
			                      otwPlatform->ZPos() + otwPlatform->dmx[0][2] * 10.0f + otwPlatform->ZDelta() * 2.0f - 20.0f );
			   SetEndFlightVec(0.0f, 0.0f, 0.0f);
	      }
	      else
	      {
//          MonoPrint ("Fixed exit\n");
		       // not otwplatform, use last focus point with some randomness...
               SetEndFlightPoint(focusPoint.x + 100.0f * PRANDFloat(), focusPoint.y + 100.0f * PRANDFloat(), focusPoint.z - 100.0f);
               SetEndFlightVec(0.0f, 0.0f, 0.0f);
	      }
      }

      if (eyeFly)
      {
#if 1
		ToggleEyeFly();
#else
		otwPlatform = lastotwPlatform;
		lastotwPlatform = NULL;
		eyeFly = FALSE;
#endif
      }
   }
   else if (i == DIK_R && endsAvail[1])
   {
	   // Start E3 HACK
	   if (SimDriver.RunningInstantAction () && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag (MOTION_OWNSHIP))
	   {
		   SimDriver.playerEntity->ResetFuel ();
	   }
	   // End E3 HACK

	   SetExitMenu( FALSE );
   }
}

int OTWDriverClass::HandleMouseClick (long x, long y)
{
int key = 0;
float xRes = (float)renderer->GetXRes();
float yRes = (float)renderer->GetYRes();
float logicalX, logicalY;
int	passThru = TRUE;

	if(InExitMenu()) {

		passThru = FALSE;

		// Correct for screen resolution
		logicalX = (float)x / xRes;
		logicalY = (float)y / yRes;

		if (logicalX >= 230.0F/640.0F && logicalX <= 250.0F/640.0F)
		{
			if (logicalY >= 200.0F/480.0F && logicalY <= 220.0F/480.0F && endsAvail[0])
			{
				key = DIK_E;
			}
			else if (logicalY >= 235.0F/480.0F && logicalY <= 255.0F/480.0F && endsAvail[1])
			{
				key = DIK_R;
			}
			else if (logicalY >= 270.0F/480.0F && logicalY <= 290.0F/480.0F && endsAvail[2])
			{
				key = DIK_D;
			}
		}

		if (key)
			ExitMenu (key);
	}

   return passThru;
}

bool MouseMenuActive = false;	// Retro 15Feb2004 - see simouse.cpp for explanation
extern bool clickableMouseMode;	// Retro 15Feb2004

void OTWDriverClass::SetExitMenu (int newVal)
{
	static bool lastclickablepitmode = clickableMouseMode;

	if (newVal==TRUE)
	{


		//Wombat778 3-26-04 Moved all this crap to DrawExitMenu and made the change ONLY affect the drawing of the exit menu.
		//Retro25Mar2004		OTWDriver.SetFOV( 60.0f * DTR );									//Wombat778 11-2-2003	Added so that exit dialog box is the right size on exit
		//OTWDriver.SetFOV( 45.0f * DTR );	// Retro 25Mar2004 - nailed it down to 45 for now, so that text is readable again
		lastclickablepitmode = clickableMouseMode;
		clickableMouseMode = true;
		MouseMenuActive = true;	// Retro 15Feb2004
	}
	else
	{
		clickableMouseMode = lastclickablepitmode;
		MouseMenuActive = false;	// Retro 15Feb2004
	}

	exitMenuDesired = newVal;
	exitMenuTimer = 0;
}

void OTWDriverClass::ChangeExitMenu (int newVal)
{
int texSet;

	if (newVal == TRUE)
   {
      // if already set end now
      if ( endFlightTimer)
      {
   	   endFlightTimer = vuxRealTime;
         newVal = FALSE;
      }
      else if (!endDialogObject)
      {
				Tpoint	pos = {4.0f, 0.f, 0.0f };
         pos.x *= (60.0F * DTR) / GetFOV();
			endDialogObject = new DrawableBSP(MapVisId(VIS_END_MISSION), &pos, &IMatrix, 1.0f );

         switch (gLangIDNum)
         {
            case F4LANG_UK:               // UK
            case F4LANG_ENGLISH:          // US
               texSet = 0;
            break;

            case F4LANG_GERMAN:           // DE
               texSet = 1;
            break;

            case F4LANG_FRENCH:           // FR
               texSet = 2;
            break;

		      case F4LANG_SPANISH:
   				texSet = 3;
            break;

		      case F4LANG_ITALIAN:
   				texSet = 4;
            break;

		      case F4LANG_PORTUGESE:
   				texSet = 5;
            break;

            default:
               texSet = 0;
            break;
         }
         endDialogObject->SetTextureSet(texSet);
		}
	}
   else
   {
		delete endDialogObject;
		endDialogObject = NULL;
	}

	exitMenuOn = newVal;
}

void OTWDriverClass::StartExitMenuCountdown(void)
	{
	exitMenuTimer = vuxRealTime + EXITMENU_POPUP_TIME;
	}

void OTWDriverClass::CancelExitMenuCountdown(void)
	{
	exitMenuTimer = 0;
	}
