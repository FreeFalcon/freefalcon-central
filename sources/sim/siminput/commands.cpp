#include "stdhdr.h"
#include "commands.h"
#include "simdrive.h"
#include "OTWDrive.h"
#include "aircrft.h"
#include "fcc.h"
#include "sms.h"
#include "SmsDraw.h"
#include "airframe.h"
#include "radar.h"
#include "radardoppler.h"
#include "mfd.h"
#include "voicecomunication/voicecom.h"//me123
#include "dispcfg.h"
#include "resource.h"
#include "cpmanager.h"
#include "fsound.h"
#include "falcsnd/psound.h"
#include "soundfx.h"
#include "threadMgr.h"
#include "hud.h"
#include "camp2sim.h"
#include "fack.h"
#include "acmi/src/include/acmirec.h"
#include "cpmisc.h"
#include "navsystem.h"
#include "cphsi.h"
#include "KneeBoard.h"
#include "ui/include/uicomms.h"
#include "missile.h"
#include "mavdisp.h"
#include "laserpod.h"
#include "HarmPod.h"
#include "playerop.h"
#include "alr56.h"
#include "MsgInc/TrackMsg.h"
#include "falcsnd/voicemanager.h" //just for debug :DSP
#include "digi.h"
#include "PilotInputs.h"
#include "lantirn.h"
#include "dofsnswitches.h"
#include "TimerThread.h"

#include "campbase.h"  // Marco for AIM9P

//MI Bullseye
#include "campaign.h"
#include "cmpclass.h"
#include "caution.h"

#include "ColorBank.h"

#include "SimIO.h"	// Retro 3Jan2004
#include "Drawbsp.h"// Retro 8May2004

#define RESCALE(in,inmin,inmax,outmin,outmax) ( ((float)(in) - (inmin)) * ((outmax) - (outmin)) / ((inmax) - (inmin)) + (outmin))


int ShowFrameRate = 0;
int testFlag = 0;
int narrowFOV = 0;
int keyboardPickleOverride = 0;
int keyboardTriggerOverride = 0;
float throttleOffsetRate = 0.0F;
float throttleOffset = 0.0F;
float rudderOffset = 0.0F;
float rudderOffsetRate = 0.0F;
float pitchStickOffset = 0.0F;
float rollStickOffset = 0.0F;
float pitchStickOffsetRate = 0.0F;
float rollStickOffsetRate = 0.0F;
float pitchRudderTrimRate = 0.0F;
float pitchAileronTrimRate = 0.0f;
float pitchElevatorTrimRate = 0.0f;
float pitchManualTrim = 0.0F;	//MI
float yawManualTrim = 0.0F;		//MI
float rollManualTrim = 0.0F;	//MI
int	UseKeyboardThrottle = FALSE;
extern int ShowVersion;
extern float gSpeedyGonzales;
int gDoOwnshipSmoke = 0;
extern PilotInputs UserStickInputs;
extern bool g_bEnableTrackIR;		// Retro 24Dez2004
extern bool g_bTrackIRon;		// Retro 24Dez2004

extern int CommandsKeyCombo;
extern int CommandsKeyComboMod;
extern BOOL WINAPI FistOfGod(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam);
extern HINSTANCE hInst;
extern void RequestPlayerDivert(void);
extern int radioMenu;
extern SensorClass* FindSensor (SimMoverClass* theObject, int sensorType);
extern SensorClass* FindLaserPod (SimMoverClass* theObject);
static int theFault = 1;
extern int supercruise;
extern int curColorIdx;

//extern bool g_bHardCoreReal; //me123		MI replaced with g_bRealisticAvionics
//MI for ICP stuff
extern bool g_bRealisticAvionics;
extern bool g_bMLU;
extern bool g_bIFF;
extern bool g_bINS;
extern float g_fFOVIncrement;  //Wombat778 9-27-2003 
extern float g_fMaximumFOV;  //Wombat778 10-11-2003 
extern float g_fDefaultFOV;  //Wombat778 10-31-2003 
extern float g_fMinimumFOV;	 //Wombat778 1-15-2003
extern float g_fNarrowFOV;	 //Wombat778 2-21-2004
extern float g_fWideviewFOV; //Wombat778 2-21-2004
extern bool g_b3DClickableCockpitDebug;
extern int g_nMaxSimTimeAcceleration;
extern int g_nShowDebugLabels;
extern int g_nMaxDebugLabel;
extern float g_f3DHeadTilt; // Cobra
extern float g_f3DPitFOV; // Cobra

#ifdef DEBUG
bool g_bShowTextures = 1;
#endif

// MN for keyboard control stuff
extern float g_fAFRudderRight;
extern float g_fAFRudderLeft;
extern float g_fAFThrottleDown;
extern float g_fAFThrottleUp;
extern float g_fAFAileronLeft;
extern float g_fAFAileronRight;
extern float g_fAFElevatorDown;
extern float g_fAFElevatorUp;
extern float g_frollStickOffset;
extern float g_fpitchStickOffset;
extern float g_frudderOffset;

// MD -- 20040210: analog idle/cutoff
extern bool g_bUseAnalogIdleCutoff;

extern bool g_bAGRadarFixes;	//MD
extern bool g_bExtViewOnGround;	// RAS -5Dec04- Allow external sat/orbit view while on ground

#ifdef DAVE_DBG
int MoveBoom = FALSE;
#endif

#ifdef _DO_VTUNE_

BOOL VtuneNoop(void) {
	return FALSE;
}

int doVtune = FALSE;
BOOL(*pauseFn)(void) = VtuneNoop;
BOOL(*resumeFn)(void) = VtuneNoop;
HINSTANCE hlib = 0;
int lTestFlag1 = 0;
int lTestFlag2 = 0;

void ToggleVtune(unsigned long, int state, void*)
{
	if(!hlib)
	{
		hlib = LoadLibrary( "vtuneapi.dll" );
		ShiAssert( hlib );

		pauseFn  = (int (__cdecl *)(void))GetProcAddress( hlib, "VtPauseSampling" );
		if (!pauseFn)
			pauseFn = VtuneNoop;
		resumeFn = (int (__cdecl *)(void))GetProcAddress( hlib, "VtResumeSampling" );
		if (!resumeFn)
			resumeFn = VtuneNoop;
	}

	if (state & KEY_DOWN)
	{
		if(doVtune)
		{
			doVtune = FALSE;
			pauseFn();
//			MonoPrint( "VTUNE PAUSED\n" );
		}
		else
		{
//			MonoPrint( "VTUNE RECORDING\n" );
			doVtune = TRUE;
			resumeFn();
		}
	}
}
#endif	// _DO_VTUNE_


void KneeboardTogglePage(unsigned long, int state, void*)
{
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered)
	{
		KneeBoard *board = OTWDriver.pCockpitManager->mpKneeBoard;
		ShiAssert( board );

		if (board->GetPage() == KneeBoard::MAP) {
			board->SetPage( KneeBoard::BRIEF );
		}
		else if (board->GetPage() == KneeBoard::BRIEF) {
		    board->SetPage(KneeBoard::STEERPOINT);
		} else {
			board->SetPage( KneeBoard::MAP );
		}
	}
}

void ToggleNVGMode(unsigned long, int state, void*)
{
	if (state & KEY_DOWN)
		OTWDriver.NVGToggle();
}


void SimToggleDropPattern(unsigned long, int state, void*)
{
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
	PlayerRwrClass* theRwr = (PlayerRwrClass*)FindSensor(SimDriver.GetPlayerAircraft(), SensorClass::RWR);

		if(theRwr)
      {
			theRwr->ToggleAutoDrop();
		}
	}
}

void ToggleSmoke(unsigned long, int state, void*)
{
   	if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   	{
		// toggle our current smoke state
		gDoOwnshipSmoke ^= 1;

		// edg: I'm not sure if there's a better message to handle this,
		// but I'm using track message
		// HACK HACK HACK!  This should be status info on the aircraft concerned...
		FalconTrackMessage* trackMsg = new FalconTrackMessage(1, SimDriver.GetPlayerAircraft()->Id(), FalconLocalGame );
		ShiAssert( trackMsg );
		if ( gDoOwnshipSmoke )
			trackMsg->dataBlock.trackType = Track_SmokeOn;
		else
			trackMsg->dataBlock.trackType = Track_SmokeOff;
		trackMsg->dataBlock.id = SimDriver.GetPlayerAircraft()->Id();
	
		// Send our track list
		FalconSendMessage (trackMsg, TRUE);
	}
}

extern long gRefreshScoresList;

void OTWToggleScoreDisplay(unsigned long, int state, void*) {
	if (state & KEY_DOWN) {
		unsigned int	flag;

		if (SimDriver.RunningDogfight()) {
			flag = SHOW_DOGFIGHT_SCORES;
		} else if (SimDriver.RunningTactical()) {
			flag = SHOW_TE_SCORES;
		} else {
			return;
		}

		gRefreshScoresList = TRUE;

		if (OTWDriver.GetFrontTextFlags() & flag) {
			OTWDriver.SetFrontTextFlags(OTWDriver.GetFrontTextFlags() & ~flag);
		} else {
			OTWDriver.SetFrontTextFlags(OTWDriver.GetFrontTextFlags() | flag);
		}
	}
}

void OTWToggleSidebar(unsigned long, int state, void*)
{	
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
      OTWDriver.ToggleSidebar();
	}
}

void SimRadarAAModeStep(unsigned long, int state, void*)
{
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
   RadarClass* theRadar = (RadarClass*)FindSensor(SimDriver.GetPlayerAircraft(), SensorClass::Radar);

      if (theRadar)
         theRadar->StepAAmode();
   }
}

void SimRadarAGModeStep(unsigned long, int state, void*)
{
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
   RadarClass* theRadar = (RadarClass*)FindSensor(SimDriver.GetPlayerAircraft(), SensorClass::Radar);

      if (theRadar)
         theRadar->StepAGmode();
   }
}

void SimRadarGainUp(unsigned long, int state, void*)
{
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
   RadarClass* theRadar = (RadarClass*)FindSensor(SimDriver.GetPlayerAircraft(), SensorClass::Radar);

      if (theRadar)
      {
         theRadar->StepAGgain( 1 );
      }
   }
}

void SimRadarGainDown(unsigned long, int state, void*)
{
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
   RadarClass* theRadar = (RadarClass*)FindSensor(SimDriver.GetPlayerAircraft(), SensorClass::Radar);

      if (theRadar)
      {
         theRadar->StepAGgain( -1 );
      }
   }
}

void SimRadarStandby(unsigned long, int state, void*)
{
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
   RadarClass* theRadar = (RadarClass*)FindSensor(SimDriver.GetPlayerAircraft(), SensorClass::Radar);

      if (theRadar)
      {
         theRadar->SetEmitting( !theRadar->IsEmitting() );
      }
   }
}

void SimRadarRangeStepUp(unsigned long, int state, void*)
{
	if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		RadarDopplerClass* theRadar = (RadarDopplerClass*)FindSensor(SimDriver.GetPlayerAircraft(), SensorClass::Radar);
		if(theRadar)
			theRadar->RangeStep( 1 );
		// MD -- 20040305: saint asked that the range change commands behave like the OSBs that you
		// should use for changing the range.  This becomes more important now the GM range scale bump
		// actually works preperly.
		if(g_bRealisticAvionics && g_bAGRadarFixes)
		{
			if(theRadar->GetRadarMode() == RadarClass::GM ||
				theRadar->GetRadarMode() == RadarClass::GMT ||
				theRadar->GetRadarMode() == RadarClass::SEA)
			{
				if(theRadar->IsSet(RadarDopplerClass::AutoAGRange))
				{
					theRadar->ClearFlagBit(RadarDopplerClass::AutoAGRange);
					theRadar->SetAutoAGRange(FALSE);
				}
			}
		}
	} 		   
	lTestFlag1 = 1 - lTestFlag1;
}

void SimRadarRangeStepDown(unsigned long, int state, void*)
{
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
	   RadarDopplerClass* theRadar = (RadarDopplerClass*)FindSensor(SimDriver.GetPlayerAircraft(), SensorClass::Radar);
	   
	   if(theRadar)
		   theRadar->RangeStep( -1 );
		// MD -- 20040305: saint asked that the range change commands behave like the OSBs that you
		// should use for changing the range.  This becomes more important now the GM range scale bump
		// actually works preperly.
		if(g_bRealisticAvionics && g_bAGRadarFixes)
		{
			if(theRadar->GetRadarMode() == RadarClass::GM ||
				theRadar->GetRadarMode() == RadarClass::GMT ||
				theRadar->GetRadarMode() == RadarClass::SEA)
			{
				if(theRadar->IsSet(RadarDopplerClass::AutoAGRange))
				{
					theRadar->ClearFlagBit(RadarDopplerClass::AutoAGRange);
					theRadar->SetAutoAGRange(FALSE);
				}
			}
		}
   }
   lTestFlag2 = 1 - lTestFlag2;
}

void SimRadarNextTarget(unsigned long, int state, void*)	
{
   // ASSOCIATOR: Added a g_bRealisticAvionics check here to fix a cheat
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN) && !g_bRealisticAvionics )
   {
      RadarClass* theRadar = (RadarClass*)FindSensor(SimDriver.GetPlayerAircraft(), SensorClass::Radar);
      if (theRadar)
         theRadar->NextTarget();

// M.N. added full realism mode
	  if (PlayerOptions.GetAvionicsType() != ATRealistic && PlayerOptions.GetAvionicsType() != ATRealisticAV) {
		  HarmTargetingPod *theHTS = (HarmTargetingPod*)FindSensor(SimDriver.GetPlayerAircraft(), SensorClass::HTS);
		  if (theHTS)
			  theHTS->NextTarget();
	  }
   }
}

void SimRadarPrevTarget(unsigned long, int state, void*)
{
   // ASSOCIATOR: Added a g_bRealisticAvionics check here to fix a cheat
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN) && !g_bRealisticAvionics )
   {
      RadarClass* theRadar = (RadarClass*)FindSensor(SimDriver.GetPlayerAircraft(), SensorClass::Radar);
      if (theRadar)
         theRadar->PrevTarget();

// M.N. added full realism mode
	  if (PlayerOptions.GetAvionicsType() != ATRealistic && PlayerOptions.GetAvionicsType() != ATRealisticAV) {
		  HarmTargetingPod *theHTS = (HarmTargetingPod*)FindSensor(SimDriver.GetPlayerAircraft(), SensorClass::HTS);
		  if (theHTS)
			  theHTS->PrevTarget();
	  }
   }
}

void SimRadarBarScanChange(unsigned long, int state, void*)
{
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
   RadarClass* theRadar = (RadarClass*)FindSensor(SimDriver.GetPlayerAircraft(), SensorClass::Radar);

      if (theRadar)
         theRadar->StepAAscanHeight();
   }
}

void SimRadarFOVStep(unsigned long, int state, void*)
{
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
   RadarClass* theRadar = (RadarClass*)FindSensor(SimDriver.GetPlayerAircraft(), SensorClass::Radar);

      if (theRadar)
         theRadar->StepAGfov();
   }
}

void SimMaverickFOVStep(unsigned long, int state, void*)
{
	AircraftClass *pac = SimDriver.GetPlayerAircraft();
	if (pac && pac->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN) && pac->Sms){
		if (pac->Sms->curWeaponType == wtAgm65 && pac->Sms->curWeapon.get()){
			MaverickDisplayClass* mavDisplay = 
				(MaverickDisplayClass*)((MissileClass*)pac->Sms->curWeapon.get())->display;
			mavDisplay->ToggleFOV();
		}
		else if (pac->Sms->curWeaponClass == wcGbuWpn){
			LaserPodClass* laserPod = (LaserPodClass* )FindLaserPod(pac);
			if (laserPod){
				laserPod->ToggleFOV();
			}
		}
	}
}

void SimSOIFOVStep(unsigned long, int state, void*){
	AircraftClass *pac = SimDriver.GetPlayerAircraft();

	if (pac && pac->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN) && pac->Sms){
		RadarClass* theRadar = (RadarClass*)FindSensor(pac, SensorClass::Radar);
		if (theRadar && theRadar->IsSOI()){
			theRadar->StepAGfov();
		}
		else if (pac->Sms->curWeaponType == wtAgm65 && pac->Sms->curWeapon){
			MaverickDisplayClass* mavDisplay = 
				(MaverickDisplayClass*)((MissileClass*)pac->Sms->curWeapon.get())->display;
			if (mavDisplay->IsSOI()){
				mavDisplay->ToggleFOV();
			}
		}
		else if (pac->Sms->curWeaponClass == wcGbuWpn){
			LaserPodClass* laserPod = (LaserPodClass* )FindLaserPod(pac);
			if (laserPod){
				laserPod->ToggleFOV();
			}
		}
	}
}

void SimRadarFreeze(unsigned long, int state, void*)
{
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
   RadarClass* theRadar = (RadarClass*)FindSensor(SimDriver.GetPlayerAircraft(), SensorClass::Radar);

      if (theRadar)
         theRadar->ToggleAGfreeze();
   }
}

void SimRadarSnowplow(unsigned long, int state, void*)
{
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
   RadarClass* theRadar = (RadarClass*)FindSensor(SimDriver.GetPlayerAircraft(), SensorClass::Radar);

      if (theRadar)
         theRadar->ToggleAGsnowPlow();
   }
}

void SimRadarCursorZero(unsigned long, int state, void*)
{
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
   RadarClass* theRadar = (RadarClass*)FindSensor(SimDriver.GetPlayerAircraft(), SensorClass::Radar);

      if (theRadar)
         theRadar->ToggleAGcursorZero();
   }
}

void SimRadarAzimuthScanChange(unsigned long, int state, void*)
{
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
   RadarClass* theRadar = (RadarClass*)FindSensor(SimDriver.GetPlayerAircraft(), SensorClass::Radar);

      if (theRadar)
         theRadar->StepAAscanWidth();
   }
}

void SimDesignate(unsigned long, int state, void*)
{
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP))
   {
      if (state & KEY_DOWN)
      {
		  //MI
		  if(g_bRealisticAvionics)
		  {
			  if(SimDriver.GetPlayerAircraft()->FCC)
			  {
				  if(SimDriver.GetPlayerAircraft()->FCC->IsSOI)
					SimDriver.GetPlayerAircraft()->FCC->HSDDesignate = 1;
				  else
					  SimDriver.GetPlayerAircraft()->FCC->designateCmd = TRUE;
			  }
		  }
		  else
			  SimDriver.GetPlayerAircraft()->FCC->designateCmd = TRUE;
      }
      else
      {
		  //MI
		  if(g_bRealisticAvionics)
		  {
			  if(SimDriver.GetPlayerAircraft()->FCC)
			  {
				  SimDriver.GetPlayerAircraft()->FCC->HSDDesignate = 0;
				  SimDriver.GetPlayerAircraft()->FCC->designateCmd = FALSE;
			  }
		  }
		  else
			  SimDriver.GetPlayerAircraft()->FCC->designateCmd = FALSE;
      }
   }
}

void SimDropTrack(unsigned long, int state, void*)
{
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP))
   {
      if ( state & KEY_DOWN)
	  {
		  //MI
		  if(g_bRealisticAvionics)
		  {
			  if(SimDriver.GetPlayerAircraft()->FCC)
			  {
				  if(SimDriver.GetPlayerAircraft()->FCC->IsSOI)
					SimDriver.GetPlayerAircraft()->FCC->HSDDesignate = -1;
				  else
					  SimDriver.GetPlayerAircraft()->FCC->dropTrackCmd = TRUE;
			  }
		  }
		  else
			  SimDriver.GetPlayerAircraft()->FCC->dropTrackCmd = TRUE;
	  }
      else
	  {
		  //MI
		  if(g_bRealisticAvionics)
		  {
			  if(SimDriver.GetPlayerAircraft()->FCC)
			  {
				  SimDriver.GetPlayerAircraft()->FCC->HSDDesignate = 0;
				  SimDriver.GetPlayerAircraft()->FCC->dropTrackCmd = FALSE;
			  }
		  }
		  else
			  SimDriver.GetPlayerAircraft()->FCC->dropTrackCmd = FALSE;
	  }
   }
}

void SimACMBoresight(unsigned long, int state, void*)
{
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
      RadarClass* theRadar = (RadarClass*)FindSensor(SimDriver.GetPlayerAircraft(), SensorClass::Radar);
      if (theRadar)
         theRadar->SelectACMBore();

// M.N. added full realism mode
	  if (PlayerOptions.GetAvionicsType() != ATRealistic && PlayerOptions.GetAvionicsType() != ATRealisticAV) {
		  HarmTargetingPod *theHTS = (HarmTargetingPod*)FindSensor(SimDriver.GetPlayerAircraft(), SensorClass::HTS);
		  if (theHTS)
			  theHTS->BoresightTarget();
	  }
   }
}

void SimACMVertical(unsigned long, int state, void*)
{
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
   RadarClass* theRadar = (RadarClass*)FindSensor(SimDriver.GetPlayerAircraft(), SensorClass::Radar);

      if (theRadar && theRadar->IsSOI())
         theRadar->SelectACMVertical();
   }
}

void SimACMSlew(unsigned long, int state, void*)
{
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
   RadarClass* theRadar = (RadarClass*)FindSensor(SimDriver.GetPlayerAircraft(), SensorClass::Radar);

      if (theRadar && theRadar->IsSOI())
         theRadar->SelectACMSlew();
   }
}

void SimACM30x20 (unsigned long, int state, void*)
{
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
   RadarClass* theRadar = (RadarClass*)FindSensor(SimDriver.GetPlayerAircraft(), SensorClass::Radar);

      if (theRadar && theRadar->IsSOI())
         theRadar->SelectACM30x20();
   }
}

void SimRadarElevationDown(unsigned long, int state, void*)
{
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
   RadarClass* theRadar = (RadarClass*)FindSensor(SimDriver.GetPlayerAircraft(), SensorClass::Radar);
      if (theRadar)
      {
// 2001-02-21 MODIFIED BY S.G. MOVES TOO MUCH
		//theRadar->StepAAelvation( -8 );
		theRadar->StepAAelvation( -4 );
      }
   }
}

void SimRadarElevationUp(unsigned long, int state, void*)
{
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
   RadarClass* theRadar = (RadarClass*)FindSensor(SimDriver.GetPlayerAircraft(), SensorClass::Radar);
      if (theRadar)
      {
// 2001-02-21 MODIFIED BY S.G. MOVES TOO MUCH
//		theRadar->StepAAelvation( 8 );
		theRadar->StepAAelvation( 4 );
      }
   }
}

void SimRadarElevationCenter(unsigned long, int state, void*)
{
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
   RadarClass* theRadar = (RadarClass*)FindSensor(SimDriver.GetPlayerAircraft(), SensorClass::Radar);
      if (theRadar)
      {
		theRadar->StepAAelvation( 0 );
      }
   }
}

// RWR Stuff (ALR56)

void SimRWRSetPriority (unsigned long, int state, void*)
{
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
   PlayerRwrClass* theRwr = (PlayerRwrClass*)FindSensor(SimDriver.GetPlayerAircraft(), SensorClass::RWR);

      if (theRwr)
      {
        theRwr->TogglePriority ();
				if (OTWDriver.GetVirtualCockpit())
					if (theRwr->IsPriority() != FALSE)
						OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_RWR_PRIORITY, 2);
					else
						OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_RWR_PRIORITY, 1);
      }
   }
}

//void SimRWRSetSound (unsigned long, int state, void*)
//{
//   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
//   {
//   PlayerRwrClass* theRwr = (PlayerRwrClass*)FindSensor(SimDriver.GetPlayerAircraft(), SensorClass::RWR);
//
//      if (theRwr)
//      {
//         theRwr->SetSound (1 - theRwr->PlaySound());
//      }
//   }
//}

void SimRWRSetTargetSep (unsigned long, int state, void*)
{
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
   PlayerRwrClass* theRwr = (PlayerRwrClass*)FindSensor(SimDriver.GetPlayerAircraft(), SensorClass::RWR);

      if (theRwr)
      {
        theRwr->ToggleTargetSep ();
				if (OTWDriver.GetVirtualCockpit())
					if (theRwr->TargetSep() != FALSE)
						OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_RWR_TGT_SEP, 2);
					else
						OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_RWR_TGT_SEP, 1);
      }
   }
}

void SimRWRSetUnknowns (unsigned long, int state, void*)
{
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
   PlayerRwrClass* theRwr = (PlayerRwrClass*)FindSensor(SimDriver.GetPlayerAircraft(), SensorClass::RWR);

      if (theRwr)
      {
          theRwr->ToggleUnknowns ();
				if (OTWDriver.GetVirtualCockpit())
					if (theRwr->ShowUnknowns() != FALSE)
						OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_RWR_UNKS, 2);
					else
						OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_RWR_UNKS, 1);
      }
   }
}

void SimRWRSetNaval (unsigned long, int state, void*)
{
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
   PlayerRwrClass* theRwr = (PlayerRwrClass*)FindSensor(SimDriver.GetPlayerAircraft(), SensorClass::RWR);

      if (theRwr)
      {
         theRwr->ToggleNaval ();
				if (OTWDriver.GetVirtualCockpit())
					if (theRwr->ShowNaval() != FALSE)
						OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_RWR_NAVAL, 2);
					else
						OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_RWR_NAVAL, 1);
      }
   }
}

void SimRWRSetGroundPriority (unsigned long, int state, void*)
{
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
   PlayerRwrClass* theRwr = (PlayerRwrClass*)FindSensor(SimDriver.GetPlayerAircraft(), SensorClass::RWR);

      if (theRwr)
      {
         theRwr->ToggleLowAltPriority();
				if (OTWDriver.GetVirtualCockpit())
					if (theRwr->ShowLowAltPriority() != FALSE)
						OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_RWR_GND_PRI, 2);
					else
						OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_RWR_GND_PRI, 1);
      }
   }
}

void SimRWRSetSearch (unsigned long, int state, void*)
{
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
   PlayerRwrClass* theRwr = (PlayerRwrClass*)FindSensor(SimDriver.GetPlayerAircraft(), SensorClass::RWR);

      if (theRwr)
      {
         theRwr->ToggleSearch();
				if (OTWDriver.GetVirtualCockpit())
					if (theRwr->ShowSearch() != FALSE)
						OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_RWR_SEARCH, 2);
					else
						OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_RWR_SEARCH, 1);
      }
   }
}

void SimRWRHandoff (unsigned long, int state, void*)
{
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
   PlayerRwrClass* theRwr = (PlayerRwrClass*)FindSensor(SimDriver.GetPlayerAircraft(), SensorClass::RWR);

      if (theRwr)
      {
				if (OTWDriver.GetVirtualCockpit())
					OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_RWR_HNDOFF, 2);

         theRwr->SelectNextEmitter();

// JB 010727 RP5 RWR
// 2001-02-15 ADDED BY S.G. SO WE HEAR THE SOUND ***RIGHT AWAY*** I WON'T PASS A targetList (although I could) SO NOT ALL OF THE ROUTINE IS DONE
//            IN 1.08i2, DoAudio PROCESSES THE WHOLE CONTACT LIST BY ITSELF AND NOT JUST THE PASSED CONTACT. SINCE 1.07 DOESN'T I'M STUCK AT DOING THIS :-(
//            THIS WON'T BE FPS INTENSIVE ANYWAY SINCE IT ONLY RUNS WHEN THE HANDOFF BUTTON IS PRESSED
//            LATER ON, I MIGHT MAKE THIS CODE 1.08i2 'COMPATIBLE'
				 theRwr->Exec(NULL);

				if (OTWDriver.GetVirtualCockpit())
					OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_RWR_HNDOFF, 1);
      }
   }
}

void SimPrevWaypoint(unsigned long, int state, void*)
{
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
      SimDriver.GetPlayerAircraft()->FCC->waypointStepCmd = -1;
   }
}

void SimNextWaypoint(unsigned long, int state, void*)
{
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
      SimDriver.GetPlayerAircraft()->FCC->waypointStepCmd = 1;
   }
}

void SimTogglePaused(unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
      SimDriver.TogglePause();
}

void SimSpeedyGonzalesUp(unsigned long, int state, void*)
{
	if ((state & KEY_DOWN) && (!gCommsMgr->Online()))
	{
		gSpeedyGonzales *= 1.25;
	}
	
	if (gSpeedyGonzales >= 32.0)
	{
		gSpeedyGonzales = 32.0;
	}
	
//	MonoPrint ("Speedy Up %f\n", gSpeedyGonzales);
}

void SimSpeedyGonzalesDown(unsigned long, int state, void*)
{
	if ((state & KEY_DOWN) && (!gCommsMgr->Online()))
	{
		gSpeedyGonzales /= 1.25;
	}
	
	if (gSpeedyGonzales < 1.0)
	{
		gSpeedyGonzales = 1.0;
	}

//	MonoPrint ("Speedy Down %f\n", gSpeedyGonzales);
}

void SimPickle(unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
   {
      keyboardPickleOverride = TRUE;
	  //MI
	  if(g_bRealisticAvionics)
	  {
		  if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && SimDriver.GetPlayerAircraft()->AVTRState(AircraftClass::AVTRStateFlags::AVTR_AUTO)) // 2002-02-15 MODIFIED BY S.G. Check if MOTION_OWNSHIP before going in otherwise it might CTD just after eject
		  { 
			  if(SimDriver.GetPlayerAircraft()->Sms && SimDriver.GetPlayerAircraft()->Sms->MasterArm() ==
				SMSBaseClass::Safe)
				return;

			  if(SimDriver.AVTROn() == FALSE)
			  {
				  SimDriver.SetAVTR(TRUE);
				  SimDriver.GetPlayerAircraft()->AddAVTRSeconds();
				  ACMIToggleRecording(0, state, NULL);
			  }
			  else
				  SimDriver.GetPlayerAircraft()->AddAVTRSeconds();
		  }
		  
		  //Targeting Pod, Fire laser automatically

		//JAM 04Jan04 - Fixing pickle-after-ejaculate-CTD.
		if( SimDriver.GetPlayerAircraft() && // MLR 5/4/2004 - <-- THIS IS NULL!!! Fixing pickle-after-ejaculate-CTD.
			!((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered &&
			!((AircraftClass*)SimDriver.GetPlayerAircraft())->doEjectCountdown) 
			//&& !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectCountdown)		//Wombat778 4-02-04	Removed, as the ejectcountdown value is not a stable number, and causes the laser NEVER to fire automatically
		{
			if(SimDriver.GetPlayerAircraft()->FCC
			&&SimDriver.GetPlayerAircraft()->FCC->LaserArm
			&&SimDriver.GetPlayerAircraft()->FCC->GetMasterMode() == FireControlComputer::AirGroundLaser)
			{
				SimDriver.GetPlayerAircraft()->FCC->CheckForLaserFire = TRUE;
				SimDriver.GetPlayerAircraft()->FCC->LaserWasFired = FALSE;
			}
		}
		//JAM
	  }
   }
   else
   {
      keyboardPickleOverride = FALSE;
   }
}

void SimTrigger(unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
   {
	   keyboardTriggerOverride = TRUE;
	   if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && SimDriver.GetPlayerAircraft()->AVTRState(AircraftClass::AVTRStateFlags::AVTR_AUTO)) // 2002-02-15 ADDED BY S.G. Check if MOTION_OWNSHIP before going in otherwise it might CTD just after eject
	   { 
		   if(SimDriver.GetPlayerAircraft()->Sms && SimDriver.GetPlayerAircraft()->Sms->MasterArm() ==
			   SMSBaseClass::Safe)
			   return;

		   if(SimDriver.AVTROn() == FALSE)
		   {
			   SimDriver.SetAVTR(TRUE);
			   SimDriver.GetPlayerAircraft()->AddAVTRSeconds();
			   ACMIToggleRecording(0, state, NULL);
		   }
		   else
			   SimDriver.GetPlayerAircraft()->AddAVTRSeconds();
	   }
   } 
   else
   {
      keyboardTriggerOverride = FALSE;
   }
}

void SimMissileStep(unsigned long, int state, void*)//me123 addet nosewheel stearing
{
	static VU_TIME mslsteptimer = 0;

	if ( SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) ) 
	{
		if (!SimDriver.GetPlayerAircraft()->af->IsSet(AirframeClass::InAir) )
		{
			//if (g_bHardCoreReal)	MI
			if(g_bRealisticAvionics)
			{
				if( state & KEY_DOWN)
				{
					if (!SimDriver.GetPlayerAircraft()->af->IsSet(AirframeClass::NoseSteerOn))	
						SimDriver.GetPlayerAircraft()->af->SetFlag(AirframeClass::NoseSteerOn);
					else 
						SimDriver.GetPlayerAircraft()->af->ClearFlag(AirframeClass::NoseSteerOn);
				}
			}
			return;
		}
	
		if( state & KEY_DOWN)
		{
			if (mslsteptimer == 0)  
			{
				mslsteptimer = SimLibElapsedTime;
			}
		}
		else
		{   
			FireControlComputer::FCCMasterMode masterMode;
			masterMode = SimDriver.GetPlayerAircraft()->FCC->GetMasterMode();
			if(SimLibElapsedTime - mslsteptimer >= 500)
			{
				
				if  (  masterMode == FireControlComputer::Missile
					|| masterMode == FireControlComputer::Dogfight
					|| masterMode == FireControlComputer::MissileOverride
					|| masterMode == FireControlComputer::AAGun /*Cobra TJL 11/12/04*/
					) 
				{
					SimDriver.GetPlayerAircraft()->Sms->StepAAWeapon();
				}

				if(		masterMode == FireControlComputer::AirGroundBomb
					||	masterMode == FireControlComputer::AirGroundMissile
					||	masterMode == FireControlComputer::AirGroundHARM
					||	masterMode == FireControlComputer::AirGroundLaser
					||	masterMode == FireControlComputer::AirGroundRocket )
				{
					SimDriver.GetPlayerAircraft()->Sms->StepAGWeapon();
				}
			}
			else
			{
				// COBRA - RED - in Bombs Mode, step thru FCC Sub modes
				if(		masterMode == FireControlComputer::AirGroundBomb
					||	masterMode == FireControlComputer::AirGroundLaser)
					SimDriver.GetPlayerAircraft()->FCC->NextSubMode();
				else
					SimDriver.GetPlayerAircraft()->FCC->WeaponStep();
			}
			mslsteptimer = 0;
		}
	}
}

/* sfr: I used to right duplicated code like it was here when I was 8 years old.
* Now these functions call SimCursor static routine.
*/
#if 0
void SimCursorUp(unsigned long, int state, void*)
{
	// MD -- 20040110: disable keys if we are using analog axis for this control
	// Also removed the ACM transition to SLEW mode from here and put it back into
	// the ACMMode() function where it probably belonged anyway.  This needed to
	// happen to make the "drop track" on reversion to SLEW mode work right for the
	// analog support anyway.
	if ((IO.AnalogIsUsed(AXIS_CURSOR_X) == true) && (IO.AnalogIsUsed(AXIS_CURSOR_Y) == true)){
		return;
	}

	AircraftClass *pac = SimDriver.GetPlayerAircraft();
	if (pac && pac->IsSetFlag(MOTION_OWNSHIP)){
		RadarDopplerClass* theRadar = (RadarDopplerClass*)FindSensor(pac, SensorClass::Radar);
		FireControlComputer* pFCC = pac->GetFCC();
		if (state & KEY_DOWN){
			//MI
			if(g_bRealisticAvionics){
				LaserPodClass* laserPod = (LaserPodClass* )FindLaserPod (pac);
				MaverickDisplayClass* mavDisplay = NULL;
				HarmTargetingPod *theHTS = (HarmTargetingPod*)FindSensor(pac, SensorClass::HTS);
				if (pac->Sms->curWeaponType == wtAgm65 && pac->Sms->curWeapon.get()){
					mavDisplay = (MaverickDisplayClass*)((MissileClass*)pac->Sms->curWeapon.get())->display;
				}
				//ACM Modes get's us directly into ACM Slew
				//if(theRadar && (theRadar->GetRadarMode() == RadarClass::ACM_30x20 ||
				//	  theRadar->GetRadarMode() == RadarClass::ACM_BORE ||
				//	  theRadar->GetRadarMode() == RadarClass::ACM_10x60))
				//{
				//if we have a lock, break it
				//	  SimDriver.GetPlayerAircraft()->FCC->dropTrackCmd = TRUE;
				//	  theRadar->SelectACMSlew();
				//}
				//  else
				if(
					(theRadar && theRadar->IsSOI()) || (mavDisplay && mavDisplay->IsSOI()) || 
					(laserPod && laserPod->IsSOI())
				){
					pac->FCC->cursorYCmd = 1;
				}
				else if(pFCC && pFCC->IsSOI){
					pac->FCC->HSDCursorYCmd = 1;
				}
				else if(theHTS && SimDriver.GetPlayerAircraft()->GetSOI() == SimVehicleClass::SOI_WEAPON){
					pac->FCC->cursorYCmd = 1;
				}
				else if(TheHud && TheHud->IsSOI()){
					pac->FCC->cursorYCmd = 1;
				}
			}
			else {
				pac->FCC->cursorYCmd = 1;
			}
		}
		else
		{
			//if(g_bRealisticAvionics)
			//	  SimDriver.GetPlayerAircraft()->FCC->dropTrackCmd = FALSE;
			pac->FCC->cursorYCmd = 0;
			pac->FCC->HSDCursorYCmd = 0;
		}
	}
}

void SimCursorDown(unsigned long, int state, void*)
{
	// MD -- 20040110: disable keys if we are using analog axis for this control
	// Also removed the ACM transition to SLEW mode from here and put it back into
	// the ACMMode() function where it probably belonged anyway.  This needed to
	// happen to make the "drop track" on reversion to SLEW mode work right for the
	// analog support anyway.
	if ((IO.AnalogIsUsed(AXIS_CURSOR_X) == true) && (IO.AnalogIsUsed(AXIS_CURSOR_Y) == true)){
		return;
	}

	AircraftClass *pac = SimDriver.GetPlayerAircraft();
	
	// sfr: @todo remove JB check
	if (
		pac && pac->IsSetFlag(MOTION_OWNSHIP) && 
		!F4IsBadReadPtr(pac->FCC, sizeof(FireControlComputer))// JB 010408 CTD
	){
		RadarDopplerClass* theRadar = (RadarDopplerClass*)FindSensor(pac, SensorClass::Radar);
		FireControlComputer* pFCC = pac->GetFCC();
		if (state & KEY_DOWN){
			//MI
			if(g_bRealisticAvionics){
				LaserPodClass* laserPod = (LaserPodClass* )FindLaserPod (pac);
				MaverickDisplayClass* mavDisplay = NULL;
				HarmTargetingPod *theHTS = (HarmTargetingPod*)FindSensor(pac, SensorClass::HTS);
				if (pac->Sms->curWeaponType == wtAgm65 && pac->Sms->curWeapon){
					mavDisplay = (MaverickDisplayClass*)((MissileClass*)pac->Sms->curWeapon.get())->display;
				}
				//ACM Modes get's us directly into ACM Slew
				//if(theRadar && (theRadar->GetRadarMode() == RadarClass::ACM_30x20 ||
				//	theRadar->GetRadarMode() == RadarClass::ACM_BORE ||
				//	theRadar->GetRadarMode() == RadarClass::ACM_10x60))
				//{
					//if we have a lock, break it
				//	SimDriver.GetPlayerAircraft()->FCC->dropTrackCmd = TRUE;
				//	theRadar->SelectACMSlew();
				//}
				//else
				if(
					(theRadar && theRadar->IsSOI()) || (mavDisplay && mavDisplay->IsSOI()) || 
					(laserPod && laserPod->IsSOI())
				){
					pac->FCC->cursorYCmd = -1;
				}
				else if(pFCC && pFCC->IsSOI){
					pac->FCC->HSDCursorYCmd = -1;
				}
				else if(theHTS && pac->GetSOI() == SimVehicleClass::SOI_WEAPON){
					SimDriver.GetPlayerAircraft()->FCC->cursorYCmd = -1;
				}
				else if(TheHud && TheHud->IsSOI()){
					pac->FCC->cursorYCmd = -1;
				}
			}
			else {
				pac->FCC->cursorYCmd = -1;
			}
		}
		else {
			//if(g_bRealisticAvionics)
			//	SimDriver.GetPlayerAircraft()->FCC->dropTrackCmd = FALSE;
			pac->FCC->cursorYCmd = 0;
			pac->FCC->HSDCursorYCmd = 0;
		}
	}
}

void SimCursorLeft(unsigned long, int state, void*)
{
	// MD -- 20040110: disable keys if we are using analog axis for this control
	// Also removed the ACM transition to SLEW mode from here and put it back into
	// the ACMMode() function where it probably belonged anyway.  This needed to
	// happen to make the "drop track" on reversion to SLEW mode work right for the
	// analog support anyway.
	if ((IO.AnalogIsUsed(AXIS_CURSOR_X) == true) && (IO.AnalogIsUsed(AXIS_CURSOR_Y) == true)){
		return;
	}

	AircraftClass *pac = SimDriver.GetPlayerAircraft();
	
	if (pac && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)){
		RadarDopplerClass* theRadar = (RadarDopplerClass*)FindSensor(pac, SensorClass::Radar);
		FireControlComputer* pFCC = pac->GetFCC();
		if (state & KEY_DOWN){
			//MI
			if(g_bRealisticAvionics){
				LaserPodClass* laserPod = (LaserPodClass* )FindLaserPod (pac);
				MaverickDisplayClass* mavDisplay = NULL;
				HarmTargetingPod *theHTS = (HarmTargetingPod*)FindSensor(pac, SensorClass::HTS);
				if (pac->Sms->curWeaponType == wtAgm65 && pac->Sms->curWeapon){
					mavDisplay = (MaverickDisplayClass*)((MissileClass*)pac->Sms->curWeapon.get())->display;
				}
				//ACM Modes get's us directly into ACM Slew
				//if(theRadar && (theRadar->GetRadarMode() == RadarClass::ACM_30x20 ||
				//	theRadar->GetRadarMode() == RadarClass::ACM_BORE ||
				//	theRadar->GetRadarMode() == RadarClass::ACM_10x60))
				//{
					//if we have a lock, break it
				//	SimDriver.GetPlayerAircraft()->FCC->dropTrackCmd = TRUE;
				//	theRadar->SelectACMSlew();
				//}
				//else
				if (
					(theRadar && theRadar->IsSOI()) || (mavDisplay && mavDisplay->IsSOI()) || 
					(laserPod && laserPod->IsSOI())
				){
					pac->FCC->cursorXCmd = -1;
				}
				else if(pFCC && pFCC->IsSOI){
					pac->FCC->HSDCursorXCmd = -1;
				}
				else if(theHTS && SimDriver.GetPlayerAircraft()->GetSOI() == SimVehicleClass::SOI_WEAPON){
					pac->FCC->cursorXCmd = -1;
				}
				else if(TheHud && TheHud->IsSOI()){
					pac->FCC->cursorXCmd = -1; //VP_changes here we are
				}
			}
			else {
				pac->FCC->cursorXCmd = -1;
			}
		}
		else
		{
			//if(g_bRealisticAvionics)
			//	SimDriver.GetPlayerAircraft()->FCC->dropTrackCmd = FALSE;
			pac->FCC->cursorXCmd = 0;
			pac->FCC->HSDCursorXCmd = 0;
		}
	}
}

void SimCursorRight(unsigned long, int state, void*)
{
	// MD -- 20040110: disable keys if we are using analog axis for this control
	// Also removed the ACM transition to SLEW mode from here and put it back into
	// the ACMMode() function where it probably belonged anyway.  This needed to
	// happen to make the "drop track" on reversion to SLEW mode work right for the
	// analog support anyway.
	if ((IO.AnalogIsUsed(AXIS_CURSOR_X) == true) && (IO.AnalogIsUsed(AXIS_CURSOR_Y) == true)){
		return;
	}

	AircraftClass *pac = SimDriver.GetPlayerAircraft();
	if (pac && pac->IsSetFlag(MOTION_OWNSHIP)){
		RadarDopplerClass* theRadar = (RadarDopplerClass*)FindSensor(pac, SensorClass::Radar);
		FireControlComputer* pFCC = pac->GetFCC();
		if (state & KEY_DOWN)
		{
			//MI
			if(g_bRealisticAvionics)
			{
				LaserPodClass* laserPod = (LaserPodClass* )FindLaserPod (pac);
				MaverickDisplayClass* mavDisplay = NULL;
				HarmTargetingPod *theHTS = (HarmTargetingPod*)FindSensor(pac, SensorClass::HTS);
				if (pac->Sms->curWeaponType == wtAgm65 && pac->Sms->curWeapon){
					mavDisplay = (MaverickDisplayClass*)((MissileClass*)pac->Sms->curWeapon.get())->display;
				}
				//ACM Modes get's us directly into ACM Slew
				//if(theRadar && (theRadar->GetRadarMode() == RadarClass::ACM_30x20 ||
				//	theRadar->GetRadarMode() == RadarClass::ACM_BORE ||
				//	theRadar->GetRadarMode() == RadarClass::ACM_10x60))
				//{
					//if we have a lock, break it
				//	SimDriver.GetPlayerAircraft()->FCC->dropTrackCmd = TRUE;
				//	theRadar->SelectACMSlew();
				//}
				//else
				if (
					(theRadar && theRadar->IsSOI()) || (mavDisplay && mavDisplay->IsSOI()) || 
					(laserPod && laserPod->IsSOI())
				){
					pac->FCC->cursorXCmd = 1;
				}
				else if (pFCC && pFCC->IsSOI){
					pac->FCC->HSDCursorXCmd = 1;
				}
				else if(theHTS && SimDriver.GetPlayerAircraft()->GetSOI() == SimVehicleClass::SOI_WEAPON){
					pac->FCC->cursorXCmd = 1;
				}
				else if (TheHud && TheHud->IsSOI()){
					pac->FCC->cursorXCmd = 1;
				}
			}
			else {
				pac->FCC->cursorXCmd = 1;
			}
		}
		else {
			//if(g_bRealisticAvionics)
			//	SimDriver.GetPlayerAircraft()->FCC->dropTrackCmd = FALSE;
			pac->FCC->cursorXCmd = 0;
			pac->FCC->HSDCursorXCmd = 0;
		}
	}
}
#endif

// static namespace
namespace {
/** sfr: Now this function is called by the SimCursor{Up|Doown|Left|Right} routines. */
void SimCursorFunc(int state, int xOff, int yOff){
	// MD -- 20040110: disable keys if we are using analog axis for this control
	// Also removed the ACM transition to SLEW mode from here and put it back into
	// the ACMMode() function where it probably belonged anyway.  This needed to
	// happen to make the "drop track" on reversion to SLEW mode work right for the
	// analog support anyway.
	if ((IO.AnalogIsUsed(AXIS_CURSOR_X) == true) && (IO.AnalogIsUsed(AXIS_CURSOR_Y) == true)){
		return;
	}
	AircraftClass *pac = SimDriver.GetPlayerAircraft();
	if (pac && pac->IsSetFlag(MOTION_OWNSHIP)){
		RadarDopplerClass* theRadar = (RadarDopplerClass*)FindSensor(pac, SensorClass::Radar);
		FireControlComputer* pFCC = pac->GetFCC();
		if (state & KEY_DOWN){
			LaserPodClass* laserPod = (LaserPodClass* )FindLaserPod (pac);
			MaverickDisplayClass* mavDisplay = NULL;
			HarmTargetingPod *theHTS = (HarmTargetingPod*)FindSensor(pac, SensorClass::HTS);
			if (pac->Sms->curWeaponType == wtAgm65 && pac->Sms->GetCurrentWeapon()){
				mavDisplay = (MaverickDisplayClass*)((MissileClass*)pac->Sms->curWeapon.get())->display;
			}
			if(
				(theRadar && theRadar->IsSOI()) || 
				(mavDisplay && mavDisplay->IsSOI()) || 
				(laserPod && laserPod->IsSOI()) ||
				(theHTS && pac->GetSOI() == SimVehicleClass::SOI_WEAPON) ||
				(TheHud && TheHud->IsSOI())
			){
				pac->FCC->cursorXCmd = xOff;
				pac->FCC->cursorYCmd = yOff;
			}
			else if	(pFCC && pFCC->IsSOI){
				pac->FCC->HSDCursorXCmd = xOff;
				pac->FCC->HSDCursorYCmd = yOff;
			}
		}
		else {
			pac->FCC->cursorXCmd = pac->FCC->cursorYCmd = 
				pac->FCC->HSDCursorXCmd = pac->FCC->HSDCursorYCmd = 0;
		}
	}
}
} // static namespace end

void SimCursorRight(unsigned long, int state, void*){
	SimCursorFunc(state, 1, 0);
}
void SimCursorLeft(unsigned long, int state, void*){
	SimCursorFunc(state, -1, 0);
}
void SimCursorUp(unsigned long, int state, void*){
	SimCursorFunc(state, 0, 1);
}
void SimCursorDown(unsigned long, int state, void*){
	SimCursorFunc(state, 0,  -1);
}

void SimToggleAutopilot(unsigned long, int state, void*)
{
    if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
    {
	switch (PlayerOptions.GetAutopilotMode())
	{
	case APIntelligent: // allowed even in realistic.
	    SimDriver.GetPlayerAircraft()->ToggleAutopilot();
	    break;
	    
	case APEnhanced:
	    if (!SimDriver.GetPlayerAircraft()->OnGround() || 
		SimDriver.GetPlayerAircraft()->AutopilotType() == AircraftClass::CombatAP)
		SimDriver.GetPlayerAircraft()->ToggleAutopilot();
	    break;
	case APNormal: // POGO/JPO - if auto pilot normal, && realistic, this isn't used.
		if ((!SimDriver.GetPlayerAircraft()->OnGround() || SimDriver.GetPlayerAircraft()->AutopilotType() == AircraftClass::CombatAP))
		{
			if (!g_bRealisticAvionics)
				SimDriver.GetPlayerAircraft()->ToggleAutopilot();
			else
				SimRightAPSwitch(0, state, NULL);
		}
    break;
	}
    }
    
}

void SimStepSMSLeft(unsigned long, int state, void*)
{
SMSClass* Sms;

   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
      Sms = SimDriver.GetPlayerAircraft()->Sms;

      if (Sms)
      {
         Sms->drawable->StepDisplayMode();
      }
   }
}

void SimStepSMSRight(unsigned long, int, void*)
{
}

void SimSelectSRMOverride (unsigned long, int state, void*)
{
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
	   if(SimDriver.GetPlayerAircraft()->FCC->GetMasterMode() == FireControlComputer::Dogfight)
		   return;

	   //SimDriver.GetPlayerAircraft()->FCC->SetMasterMode(FireControlComputer::Dogfight);
	   SimDriver.GetPlayerAircraft()->FCC->EnterDogfightMode(); // MLR 4/11/2004 - 
	   MfdDisplay[1]->SetNewMasterMode(SimDriver.GetPlayerAircraft()->FCC->GetMainMasterMode());
	   if(SimDriver.GetPlayerAircraft()->Sms && SimDriver.GetPlayerAircraft()->Sms->drawable)
		   SimDriver.GetPlayerAircraft()->Sms->drawable->SetDisplayMode(SmsDrawable::Wpn);

	   //MI 02/02/02
	   if(SimDriver.GetPlayerAircraft()->Sms && SimDriver.GetPlayerAircraft()->Sms->GetCoolState() == SMSClass::WARM 
		   && SimDriver.GetPlayerAircraft()->Sms->MasterArm() == SMSClass::Arm)
	   {
		   SimDriver.GetPlayerAircraft()->Sms->SetCoolState(SMSClass::COOLING);
	   }
   }
}

void SimSelectMRMOverride(unsigned long, int state, void*)
{
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
	   if(SimDriver.GetPlayerAircraft()->FCC->GetMasterMode() == FireControlComputer::MissileOverride)
		   return;


	   //SimDriver.GetPlayerAircraft()->FCC->SetMasterMode(FireControlComputer::MissileOverride);
	   SimDriver.GetPlayerAircraft()->FCC->EnterMissileOverrideMode(); // MLR 4/11/2004 - 
	   MfdDisplay[1]->SetNewMasterMode(SimDriver.GetPlayerAircraft()->FCC->GetMainMasterMode());
	   if(SimDriver.GetPlayerAircraft()->Sms && SimDriver.GetPlayerAircraft()->Sms->drawable)
		   SimDriver.GetPlayerAircraft()->Sms->drawable->SetDisplayMode(SmsDrawable::Wpn);

	   //MI 02/02/02
	   if(SimDriver.GetPlayerAircraft()->Sms && SimDriver.GetPlayerAircraft()->Sms->GetCoolState() == SMSClass::WARM 
		   && SimDriver.GetPlayerAircraft()->Sms->MasterArm() == SMSClass::Arm)
	   {
		   SimDriver.GetPlayerAircraft()->Sms->SetCoolState(SMSClass::COOLING);
	   }
   }
}

void SimDeselectOverride(unsigned long, int state, void*)
{
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
	   SimDriver.GetPlayerAircraft()->FCC->ClearOverrideMode();
	   MfdDisplay[1]->SetNewMasterMode(SimDriver.GetPlayerAircraft()->FCC->GetMainMasterMode());
   }
}

// Support for AIM9 Uncage/Cage
void SimToggleMissileCage(unsigned long, int state, void*)
{
	AircraftClass *pac = SimDriver.GetPlayerAircraft();
	if (pac && pac->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)){
		//MI check for MAV Displays
		if (pac->Sms->curWeaponType == wtAgm65 && pac->Sms->curWeapon.get() && pac->Sms->Powered){ 
			static_cast<MissileClass*>(pac->Sms->curWeapon.get())->Covered = FALSE;
			return;
		}
		pac->FCC->missileCageCmd = TRUE;
	}
}

// Marco Edit - Support for AIM9 Spot/Scan mode(s)
void SimToggleMissileSpotScan(unsigned long, int state, void*)
{
	AircraftClass *pac = SimDriver.GetPlayerAircraft();
	if (pac && pac->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN) && pac->Sms){
		SimWeaponClass* wpn = pac->Sms->GetCurrentWeapon();
		if (g_bRealisticAvionics && wpn && ((CampBaseClass*)wpn)->GetSPType() == SPTYPE_AIM9P){
			pac->FCC->missileSpotScanCmd = FALSE;
		}
		else {
			pac->FCC->missileSpotScanCmd = TRUE;
		}
	}
}

// Marco Edit - Support for Bore/Slave
void SimToggleMissileBoreSlave (unsigned long val, int state, void *)
{
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
      SimDriver.GetPlayerAircraft()->FCC->missileSlaveCmd = TRUE;
   }
}

// Marco Edit - Support for TD/BP
void SimToggleMissileTDBPUncage (unsigned long val, int state, void *)
{
	AircraftClass *pac = SimDriver.GetPlayerAircraft();
	if (pac && pac->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN) && pac->Sms){
		SimWeaponClass* wpn = SimDriver.GetPlayerAircraft()->Sms->GetCurrentWeapon();
		if (g_bRealisticAvionics && wpn && ((CampBaseClass*)wpn)->GetSPType() == SPTYPE_AIM9P){
			pac->FCC->missileTDBPCmd = FALSE;
		}
		else {
			pac->FCC->missileTDBPCmd = TRUE;
		}
	}
}

void SimDropChaff(unsigned long, int state, void*)
{
	static unsigned int realEWSProgNum = FALSE;
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
	   
	   //MI
       // RV - Biker - Hack AC with IFF now can drop programmed EWS (player only)
       // if(!g_bRealisticAvionics || !SimDriver.GetPlayerAircraft()->af->platform->IsF16())
       if(!g_bRealisticAvionics || !(SimDriver.GetPlayerAircraft()->af->platform->IsF16() || SimDriver.GetPlayerAircraft()->af->platform->GetiffEnabled()))
		   SimDriver.GetPlayerAircraft()->dropChaffCmd = TRUE;
	   else if (g_bMLU)
	   {
		//me123 hack hack  i want the posibility to assign two programs...so now a chaff hit will default to program 1
			
			if (!realEWSProgNum) realEWSProgNum= SimDriver.GetPlayerAircraft()->EWSProgNum;
			SimDriver.GetPlayerAircraft()->EWSProgNum = 0;
		    SimDriver.GetPlayerAircraft()->DropEWS();
		    SimDropProgrammed(0, KEY_DOWN, NULL);

	   }
	   else
	   {
		   SimDropProgrammed(0, KEY_DOWN, NULL);
	   }
   }
   else if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) &&
	   g_bMLU)
   {
	   SimDriver.GetPlayerAircraft()->EWSProgNum = realEWSProgNum ;
	   realEWSProgNum = FALSE;
   }
}

void SimDropFlare(unsigned long, int state, void*)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC && playerAC->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		// RV - Biker - Hack AC with IFF now can drop programmed EWS (player only)
		// if(!g_bRealisticAvionics || !SimDriver.GetPlayerAircraft()->af->platform->IsF16())
		if (
			!g_bRealisticAvionics || 
			!(playerAC->af->platform->IsF16() || playerAC->af->platform->GetiffEnabled())
		){
		   playerAC->dropFlareCmd = TRUE;
		}
		else {
			SimDropProgrammed(0, KEY_DOWN, NULL);
		}
	}
}

void SimHSDRangeStepUp (unsigned long, int state, void*)
{
   //if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) // JB 010220 CTD
	 if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && !F4IsBadReadPtr(SimDriver.GetPlayerAircraft(), sizeof(AircraftClass)) && SimDriver.GetPlayerAircraft()->FCC && !F4IsBadWritePtr(SimDriver.GetPlayerAircraft()->FCC, sizeof(FireControlComputer)) && (state & KEY_DOWN)) // JB 010220 CTD
   {
      SimDriver.GetPlayerAircraft()->FCC->HSDRangeStepCmd = 1;
   }
}

void SimHSDRangeStepDown (unsigned long, int state, void*)
{
   //if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) // JB 010220 CTD
	 if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && !F4IsBadReadPtr(SimDriver.GetPlayerAircraft(), sizeof(AircraftClass)) && SimDriver.GetPlayerAircraft()->FCC && !F4IsBadWritePtr(SimDriver.GetPlayerAircraft()->FCC, sizeof(FireControlComputer)) && (state & KEY_DOWN)) // JB 010220 CTD
   {
      SimDriver.GetPlayerAircraft()->FCC->HSDRangeStepCmd = -1;
   }
}

void SimToggleInvincible(unsigned long, int state, void*)
{
	
	if(FalconLocalGame && !FalconLocalGame->rules.InvulnerableOn())
		return;	
	
	if (state & KEY_DOWN)
	{
		if(PlayerOptions.InvulnerableOn())
			{
			PlayerOptions.ClearSimFlag(SIM_INVULNERABLE);
			SimDriver.GetPlayerAircraft()->UnSetFalcFlag(FEC_INVULNERABLE);
			}
		else
			{
			PlayerOptions.SetSimFlag(SIM_INVULNERABLE);
			SimDriver.GetPlayerAircraft()->SetFalcFlag(FEC_INVULNERABLE);
			}
	}
}

void SimFCCSubModeStep (unsigned long, int state, void*)
{
	if (
		SimDriver.GetPlayerAircraft() && 
		SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)
	){
		SimDriver.GetPlayerAircraft()->FCC->NextSubMode();
	}
}

void SimEndFlight(unsigned long, int state, void*)
{
	if ((state & KEY_DOWN)){
		OTWDriver.EndFlight();
	}
}

void SimNextAAWeapon(unsigned long val, int state, void* pButton)
{
	SMSClass* Sms;

   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
	   if(!g_bRealisticAvionics)
	   {
		   //MI original Code
		   if ( ((CPButtonObject*)pButton)->GetCurrentState() == CPBUTTON_OFF ) {
			   SimICPAA(val, state, pButton);
			   Sms = SimDriver.GetPlayerAircraft()->Sms;
			   if (Sms)
			    //Sms->GetNextWeapon(wdGround); // MLR 2/8/2004 - changed
			    Sms->StepAAWeapon();
		   } else {
			   Sms = SimDriver.GetPlayerAircraft()->Sms;
			   if (Sms)
			    //Sms->GetNextWeapon(wdGround); // MLR 2/8/2004 - changed
			    Sms->StepAAWeapon();
			   OTWDriver.pCockpitManager->mpIcp->HandleInput(AA_BUTTON, (CPButtonObject*)pButton);
			   MfdDisplay[0]->SetNewMode(MFDClass::FCRMode);
			   MfdDisplay[1]->SetNewMode(MFDClass::SMSMode);
		   }
	   }
	   else
	   {
		   //MI modified for ICP
		   /* // MLR 2/8/2004 - 
			if( SimDriver.GetPlayerAircraft()->FCC->GetSubMode() == (FireControlComputer::STRAF) ||	// ASSOCIATOR: Added a STAF check here so we can get out of it
			   SimDriver.GetPlayerAircraft()->FCC->GetMasterMode() != (FireControlComputer::AAGun) &&
			   SimDriver.GetPlayerAircraft()->FCC->GetMasterMode() != (FireControlComputer::Missile) &&
			   SimDriver.GetPlayerAircraft()->FCC->GetMasterMode() != (FireControlComputer::Dogfight) &&
			   SimDriver.GetPlayerAircraft()->FCC->GetMasterMode() != (FireControlComputer::MissileOverride))
				  SimDriver.GetPlayerAircraft()->FCC->SetMasterMode( SimDriver.GetPlayerAircraft()->FCC->GetLastAaMasterMode() );
			*/
		   Sms = SimDriver.GetPlayerAircraft()->Sms;
		   // MLR 2/8/2004 - renamed function
		   //Sms->StepWeaponClass();	// ASSOCIATOR: now it steps the weapon type instead of Missile stepping each weapon 
		   Sms->StepAAWeapon();	// ASSOCIATOR: now it steps the weapon type instead of Missile stepping each weapon 

		   // ASSOCIATOR: This whole section is redundant now and is handled in a central place in the SelectWeapon method
		   /*if (Sms && SimDriver.GetPlayerAircraft()->FCC->GetMasterMode() != FireControlComputer::Dogfight && 
					SimDriver.GetPlayerAircraft()->FCC->GetMasterMode() != FireControlComputer::MissileOverride ) 
		   {
			   //Sms->GetNextWeapon(wdAir);
			   Sms->curWeaponDomain = wdAir;
			   Sms->StepWeaponClass();

			   Sms->drawable->SetDisplayMode(SmsDrawable::Wpn);
			   
			   // ASSOCIATOR 03/12/03: Commented out this section becasue with new fixes it is redundant
			   // Marco Edit - Dogfight check for AIM120
			   //if (SimDriver.GetPlayerAircraft()->FCC->GetMasterMode() == (FireControlComputer::Dogfight) && Sms->curWeaponType == wtAim120)
			   //{
			   //	SimDriver.GetPlayerAircraft()->FCC->SetDgftSubMode(FireControlComputer::Aim120);
			   //}
			}
			*/
	
			// ASSOCIATOR 03/12/03: Put the radar in the its default AA mode but not while in Dogfight mode
			if( SimDriver.GetPlayerAircraft()->FCC->GetMasterMode() != FireControlComputer::Dogfight )
			{
				// Put the radar in the its default AA mode
				RadarClass* pradar = (RadarClass*) FindSensor (SimDriver.GetPlayerAircraft(), SensorClass::Radar);
				if (pradar)
					pradar->DefaultAAMode();
			}

		   OTWDriver.pCockpitManager->mpIcp->SetICPFlag(ICPClass::MODE_A_A);
		   OTWDriver.pCockpitManager->mpIcp->ClearICPFlag(ICPClass::MODE_A_G);
		   MfdDisplay[0]->SetNewMasterMode(SimDriver.GetPlayerAircraft()->FCC->GetMainMasterMode());
		   MfdDisplay[1]->SetNewMasterMode(SimDriver.GetPlayerAircraft()->FCC->GetMainMasterMode());
	   }
   }
}

void SimStepMasterArm(unsigned long, int state, void*)
{
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN) && SimDriver.GetPlayerAircraft()->Sms)
   {
      SimDriver.GetPlayerAircraft()->Sms->StepMasterArm();
			if (OTWDriver.GetVirtualCockpit())
			{
				if (SimDriver.GetPlayerAircraft()->Sms->MasterArm() == SMSBaseClass::Arm)
				{
					OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_MASTER_ARM, 2);
				}
				else
				if (SimDriver.GetPlayerAircraft()->Sms->MasterArm() ==  SMSBaseClass::Sim)
				{
					OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_MASTER_ARM, 3);
				}
				else // safe
					OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_MASTER_ARM, 1);
			}
   }
}

void SimArmMasterArm(unsigned long, int state, void*)
{
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN) && SimDriver.GetPlayerAircraft()->Sms)
   {
      SimDriver.GetPlayerAircraft()->Sms->SetMasterArm(SMSBaseClass::Arm);
   }
}

void SimSafeMasterArm(unsigned long, int state, void*)
{
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN) && SimDriver.GetPlayerAircraft()->Sms)
   {
      SimDriver.GetPlayerAircraft()->Sms->SetMasterArm(SMSBaseClass::Safe);
			if (OTWDriver.GetVirtualCockpit())
					OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_MASTER_ARM, 3);
   }
}

void SimSimMasterArm(unsigned long, int state, void*)
{
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN) && SimDriver.GetPlayerAircraft()->Sms)
   {
      SimDriver.GetPlayerAircraft()->Sms->SetMasterArm(SMSBaseClass::Sim);
			if (OTWDriver.GetVirtualCockpit())
					OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_MASTER_ARM, 2);
   }
}

void SimNextAGWeapon (unsigned long val, int state, void* pButton)
{
SMSClass* Sms;

	if( F4IsBadReadPtr(SimDriver.GetPlayerAircraft(), sizeof(AircraftClass)) || !SimDriver.GetPlayerAircraft()->FCC || // JB 010305 CTD
		 F4IsBadReadPtr(SimDriver.GetPlayerAircraft()->FCC, sizeof(FireControlComputer)) || // JB 010305 CTD
		 SimDriver.GetPlayerAircraft()->FCC->GetMasterMode() == FireControlComputer::MissileOverride ||
		 SimDriver.GetPlayerAircraft()->FCC->GetMasterMode() == FireControlComputer::Dogfight ) 
		return;

	if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		if(!g_bRealisticAvionics)
		{
			//MI original code
			if ( ((CPButtonObject*)pButton)->GetCurrentState() == CPBUTTON_OFF ) {
				SimICPAG(val, state, pButton);
				Sms = SimDriver.GetPlayerAircraft()->Sms;
				if (Sms)
			    //Sms->GetNextWeapon(wdGround); // MLR 2/8/2004 - changed
			    Sms->StepAGWeapon();
			} else {
				Sms = SimDriver.GetPlayerAircraft()->Sms;
				if (Sms)
			    //Sms->GetNextWeapon(wdGround); // MLR 2/8/2004 - changed
			    Sms->StepAGWeapon();
			   OTWDriver.pCockpitManager->mpIcp->HandleInput(AG_BUTTON, (CPButtonObject*)pButton);
			   MfdDisplay[0]->SetNewMode(MFDClass::FCRMode);
			   MfdDisplay[1]->SetNewMode(MFDClass::SMSMode);
			}
		}
		else
		{	
			 //MI modified for ICP
			Sms = SimDriver.GetPlayerAircraft()->Sms;
			if (Sms) 
			{
			    //Sms->GetNextWeapon(wdGround); // MLR 2/8/2004 - changed
			    Sms->StepAGWeapon();
				Sms->drawable->SetDisplayMode(SmsDrawable::Wpn);
			}
			//Put the radar in the its default AG mode
			RadarClass* pradar = (RadarClass*) FindSensor (SimDriver.GetPlayerAircraft(), SensorClass::Radar);
			if (pradar)
				pradar->DefaultAGMode();
			OTWDriver.pCockpitManager->mpIcp->SetICPFlag(ICPClass::MODE_A_G);
			OTWDriver.pCockpitManager->mpIcp->ClearICPFlag(ICPClass::MODE_A_A);
			// Configure the MFDs
			MfdDisplay[0]->SetNewMasterMode(SimDriver.GetPlayerAircraft()->FCC->GetMainMasterMode());
			MfdDisplay[1]->SetNewMasterMode(SimDriver.GetPlayerAircraft()->FCC->GetMainMasterMode());
		}
	}
}

void SimNextNavMode (unsigned long val, int state, void* pButton)
{
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered)
	{
		if (OTWDriver.pCockpitManager->mpIcp->GetICPPrimaryMode() != NAV_MODE) {
			SimICPNav(val, state, pButton);
		} else {
			SimICPTILS(val, state, OTWDriver.pCockpitManager->GetButtonPointer( ICP_ILS_BUTTON_ID ));
		}
	}
}

void SimEject(unsigned long, int state, void*)
{
	if(SimDriver.GetPlayerAircraft() != NULL && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP))
	{
		// We only want to eject if the eject key combo is held 
		// for > 1 second.
		if(((AircraftClass *)SimDriver.GetPlayerAircraft())->ejectTriggered == FALSE)
		{
			if(state & KEY_DOWN)
			{
				//MI
				if(g_bRealisticAvionics)
				{
					if(((AircraftClass*)SimDriver.GetPlayerAircraft())->SeatArmed)
					{
						// Start the timer
						((AircraftClass *)SimDriver.GetPlayerAircraft())->ejectCountdown = 0.5; //THW 2004-02-17 Changed from 1.0 to 0.5 seconds
						((AircraftClass *)SimDriver.GetPlayerAircraft())->doEjectCountdown = TRUE;
					}
				}
				else
				{
					// Start the timer
					((AircraftClass *)SimDriver.GetPlayerAircraft())->ejectCountdown = 0.5; //THW 2004-02-17 Changed from 1.0 to 0.5 seconds
					((AircraftClass *)SimDriver.GetPlayerAircraft())->doEjectCountdown = TRUE;
				}
			}
			else
			{
				// Cancel the timer
				((AircraftClass *)SimDriver.GetPlayerAircraft())->doEjectCountdown = FALSE;
			}
		}
	}
}


/// SIM Time Management
void TimeAccelerate (unsigned long, int state, void*)
{
   // edg: it's ok to accel time when ejected....
   // if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP))
   {
      if (state & KEY_DOWN)
      {
         if (gameCompressionRatio != 2)
            SetTimeCompression (2);
         else
            SetTimeCompression (1);
		 F4HearVoices();
      }	  
   }
}

void TimeAccelerateMaxToggle (unsigned long, int state, void*)
{
   // edg: it's ok to accel time when ejected....
   // if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP))
   {
      if (state & KEY_DOWN)
      {
         if (gameCompressionRatio != 4)
            SetTimeCompression (4);
         else
            SetTimeCompression (1);
		 F4HearVoices();
      }
   }
}

// JB 010109
void TimeAccelerateInc (unsigned long, int state, void*)
{
	int newcomp;

	if (state & KEY_DOWN)
	{
		newcomp = gameCompressionRatio * 2;
		if (newcomp > g_nMaxSimTimeAcceleration)
			newcomp = g_nMaxSimTimeAcceleration;

		SetTimeCompression (newcomp);
	
		F4HearVoices();
	}	  
}

void TimeAccelerateDec (unsigned long, int state, void*)
{
	int newcomp;

	if (state & KEY_DOWN)
	{
		newcomp = gameCompressionRatio / 2;
		if (newcomp == 0)
			newcomp = 1;

		SetTimeCompression (newcomp);
	
		F4HearVoices();
	}	  
}
// JB 010109

// SMS Control
void BombRippleIncrement (unsigned long, int state, void*)
{
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP))
   {
      if (state & KEY_DOWN && SimDriver.GetPlayerAircraft()->Sms)
      {
         SimDriver.GetPlayerAircraft()->Sms->IncrementRippleCount();
      }
   }
}

void BombIntervalIncrement (unsigned long, int state, void*)
{
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP))
   {
      if (state & KEY_DOWN && SimDriver.GetPlayerAircraft()->Sms)
      {
         SimDriver.GetPlayerAircraft()->Sms->IncrementRippleInterval();
      }
   }
}

void BombRippleDecrement (unsigned long, int state, void*)
{
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP))
   {
      if (state & KEY_DOWN && SimDriver.GetPlayerAircraft()->Sms)
      {
         SimDriver.GetPlayerAircraft()->Sms->DecrementRippleCount();
      }
   }
}

void BombIntervalDecrement (unsigned long, int state, void*)
{
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP))
   {
      if (state & KEY_DOWN && SimDriver.GetPlayerAircraft()->Sms)
      {
         SimDriver.GetPlayerAircraft()->Sms->DecrementRippleInterval();
      }
   }
}

void BombBurstIncrement (unsigned long, int state, void*)
{
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP))
   {
      if (state & KEY_DOWN && SimDriver.GetPlayerAircraft()->Sms)
      {
         SimDriver.GetPlayerAircraft()->Sms->IncrementBurstHeight();
      }
   }
}

void BombBurstDecrement (unsigned long, int state, void*)
{
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP))
   {
      if (state & KEY_DOWN && SimDriver.GetPlayerAircraft()->Sms)
      {
         SimDriver.GetPlayerAircraft()->Sms->DecrementBurstHeight();
      }
   }
}

void BombPairRelease (unsigned long, int state, void*)
{
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP))
   {
      if (state & KEY_DOWN && SimDriver.GetPlayerAircraft()->Sms)
      {
         //SimDriver.GetPlayerAircraft()->Sms->SetPair(TRUE);
         SimDriver.GetPlayerAircraft()->Sms->SetAGBPair(TRUE);
      }
   }
}

void BombSGLRelease (unsigned long, int state, void*)
{
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP))
   {
      if (state & KEY_DOWN && SimDriver.GetPlayerAircraft()->Sms)
      {
         //SimDriver.GetPlayerAircraft()->Sms->SetPair(FALSE);
         SimDriver.GetPlayerAircraft()->Sms->SetAGBPair(FALSE);
      }
   }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void AFBrakesOut (unsigned long, int state, void*)
{
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP))
   {
      if (SimDriver.GetPlayerAircraft()->af->HydraulicA() == 0) return;
      if (state & KEY_DOWN)
      {
        SimDriver.GetPlayerAircraft()->af->speedBrake = 1.0F;
				SimDriver.GetPlayerAircraft()->brakePos = 3;
      }
      else
      {
         SimDriver.GetPlayerAircraft()->af->speedBrake = 0.0F;
      }
   }
}

void AFBrakesToggle (unsigned long, int state, void*)
{
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP))
   {
      if (SimDriver.GetPlayerAircraft()->af->HydraulicA() == 0) return;
      if (state & KEY_DOWN)
      {
				//Close the brake
				if(SimDriver.GetPlayerAircraft()->af->dbrake > 0)
				{
					SimDriver.GetPlayerAircraft()->af->speedBrake = -1.0F;
					SimDriver.GetPlayerAircraft()->brakePos = 4;
				}
				else
				{
					SimDriver.GetPlayerAircraft()->af->speedBrake = 1.0F;
					SimDriver.GetPlayerAircraft()->brakePos = 3;
				}

				SimDriver.GetPlayerAircraft()->af->BrakesToggle = TRUE;
      }
   }
}

void AFBrakesIn (unsigned long, int state, void*)
{
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP))
   {
      if (SimDriver.GetPlayerAircraft()->af->HydraulicA() == 0) return;
      if (state & KEY_DOWN)
	  {
         SimDriver.GetPlayerAircraft()->af->speedBrake = -1.0F;
		 SimDriver.GetPlayerAircraft()->speedBrakeState = SimDriver.GetPlayerAircraft()->af->dbrake;
		 if(SimDriver.GetPlayerAircraft()->af->dbrake == 0.00f)
		 {
			SimDriver.GetPlayerAircraft()->brakePos = 4;
		 }
	  }
      else
         SimDriver.GetPlayerAircraft()->af->speedBrake = 0.0F;
   }
}

void AFGearToggle (unsigned long, int state, void*)
{
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && SimDriver.GetPlayerAircraft()->af && (state & KEY_DOWN)) // 2002-02-15 MODIFIED BY S.G. Uncommented the ...IsSetFlag(MOTION_OWNSHIP) section and moved it before the ...af check since MOTION_OWNSHIP is only cleared when we're ejected (and we have no gears when ejected anyway)
   {
       // check to see if gear is working
	   //MI but we want to be able to move our handle with no hydraulics,
	   //at least when on ground as nothing happens with the gear anyway
	   if(SimDriver.GetPlayerAircraft()->af->IsSet(AirframeClass::InAir))
	   {
		   if ( SimDriver.GetPlayerAircraft()->mFaults->GetFault( FaultClass::gear_fault ) ||
			   SimDriver.GetPlayerAircraft()->af->HydraulicB() == 0 ||
			   SimDriver.GetPlayerAircraft()->af->altGearDeployed)
		   {
			   return;
		   }
	   }
       
         if (SimDriver.GetPlayerAircraft()->af->gearHandle > 0.0F)
		 {
            SimDriver.GetPlayerAircraft()->af->gearHandle = -1.0F;
			/*
	   		F4SoundFXSetPos( SFX_GEARUP, TRUE,
				SimDriver.GetPlayerAircraft()->XPos(),
				SimDriver.GetPlayerAircraft()->YPos(),
				SimDriver.GetPlayerAircraft()->ZPos(), 1.0f );
			 */
		 }
         else
		 {
            SimDriver.GetPlayerAircraft()->af->gearHandle = 1.0F;
			/*
	   		F4SoundFXSetPos( SFX_GEARDN, TRUE,
				SimDriver.GetPlayerAircraft()->XPos(),
				SimDriver.GetPlayerAircraft()->YPos(),
				SimDriver.GetPlayerAircraft()->ZPos(), 1.0f );
			*/
		 }
   }
}

// MD -- 20031120: Adding explicit commands for up and down placement of the gear handle
// which for cockpit builders.

void AFGearUp (unsigned long, int state, void*)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;

	if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && SimDriver.GetPlayerAircraft()->af && (state & KEY_DOWN)) // 2002-02-15 MODIFIED BY S.G. Uncommented the ...IsSetFlag(MOTION_OWNSHIP) section and moved it before the ...af check since MOTION_OWNSHIP is only cleared when we're ejected (and we have no gears when ejected anyway)
	{
	   if(SimDriver.GetPlayerAircraft()->af->IsSet(AirframeClass::InAir))
	   {
		   if ( SimDriver.GetPlayerAircraft()->mFaults->GetFault( FaultClass::gear_fault ) ||
			   SimDriver.GetPlayerAircraft()->af->HydraulicB() == 0 ||
			   SimDriver.GetPlayerAircraft()->af->altGearDeployed)
		   {
			   return;
		   }
	   }
	   SimDriver.GetPlayerAircraft()->af->gearHandle = -1.0F;
   }
}

void AFGearDown (unsigned long, int state, void*)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;

	if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && SimDriver.GetPlayerAircraft()->af && (state & KEY_DOWN)) // 2002-02-15 MODIFIED BY S.G. Uncommented the ...IsSetFlag(MOTION_OWNSHIP) section and moved it before the ...af check since MOTION_OWNSHIP is only cleared when we're ejected (and we have no gears when ejected anyway)
	{
	   if(SimDriver.GetPlayerAircraft()->af->IsSet(AirframeClass::InAir))
	   {
		   if ( SimDriver.GetPlayerAircraft()->mFaults->GetFault( FaultClass::gear_fault ) ||
			   SimDriver.GetPlayerAircraft()->af->HydraulicB() == 0 ||
			   SimDriver.GetPlayerAircraft()->af->altGearDeployed)
		   {
			   return;
		   }
	   }
	   SimDriver.GetPlayerAircraft()->af->gearHandle = 1.0F;
   }
}

void AFElevatorUp (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
//      pitchStickOffsetRate = 0.5F;
		pitchStickOffsetRate = g_fAFElevatorUp;
   else
   {
      pitchStickOffsetRate = 0.0F;
   }
}

void AFElevatorDown (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
//      pitchStickOffsetRate -= 0.5F;
		pitchStickOffsetRate = -g_fAFElevatorDown;
   else
   {
      pitchStickOffsetRate = 0.0F;
   }
}

void AFAileronRight (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
//      rollStickOffsetRate += 0.8F;
		rollStickOffsetRate = g_fAFAileronRight;
   else
   {
      rollStickOffsetRate = 0.0F;
   }
}

void AFAileronLeft (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
//      rollStickOffsetRate -= 0.8F;
		rollStickOffsetRate = -g_fAFAileronLeft;
   else
   {
      rollStickOffsetRate = 0.0F;
   }
}

void AFThrottleUp (unsigned long, int state, void*)
{
	if (state & KEY_DOWN)
	{
		if(!UseKeyboardThrottle)
		{
			UseKeyboardThrottle = TRUE;
			throttleOffset = UserStickInputs.throttle;
		}			
		//throttleOffsetRate += 0.01F;
		throttleOffsetRate = g_fAFThrottleUp;
	}
	else
		throttleOffsetRate = 0.0F;
}


void AFThrottleDown (unsigned long, int state, void*)
{
	if (state & KEY_DOWN)
	{
		if(!UseKeyboardThrottle)
		{
			UseKeyboardThrottle = TRUE;
			throttleOffset = UserStickInputs.throttle;
		}
//		throttleOffsetRate -= 0.01F;
		throttleOffsetRate = -g_fAFThrottleDown;
	}
	else
		throttleOffsetRate = 0.0F;
}

void AFRudderLeft (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
//      rudderOffsetRate = 0.5F;
		rudderOffsetRate = g_fAFRudderLeft;
   else
   {
      rudderOffsetRate = 0.0F;
   }
}

void AFRudderRight (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
//      rudderOffsetRate = -0.5F;
		rudderOffsetRate = -g_fAFRudderRight;
   else
   {
      rudderOffsetRate = 0.0F;
   }
}

void AFCoarseThrottleUp (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
   {
   int tmpThrottle = FloatToInt32(throttleOffset * 100.0F);

      if (throttleOffset < 1.0F)
      {
         throttleOffset = (tmpThrottle + 25) * 0.01F;
      }
      else
      {
         throttleOffset = (tmpThrottle + 50/30) * 0.01F;
      }
   }
}

void AFCoarseThrottleDown (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
   {
   int tmpThrottle = FloatToInt32(throttleOffset * 100.0F);

      if (throttleOffset < 1.0F)
      {
         throttleOffset = (tmpThrottle - 25) * 0.01F;
      }
      else
      {
         throttleOffset = (tmpThrottle - 50/30) * 0.01F;
      }
   }
}

void AFABFull (unsigned long, int state, void*)
{
   if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft()) && (SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)))
      throttleOffset = max (1.50F, throttleOffset);
}

void AFABOn (unsigned long, int state, void*)
{
   if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft()) && (SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)))
      throttleOffset = max (1.01F, throttleOffset);
}

void AFIdle (unsigned long, int state, void*)
{
   if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft()) && (SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)))
      throttleOffset = 0.0F;
}

// 2000-11-23 S.G. Back to its original state. I've added the 'SimCATSwitch' routine
#if 0
	void OTWTimeOfDayStep (unsigned long, int state, void*)//me123 now cat III
	{
	   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN) && SimDriver.GetPlayerAircraft()->Sms)
		  SimDriver.GetPlayerAircraft()->Sms->StepCatIII();
	}
#else
	void OTWTimeOfDayStep (unsigned long, int state, void*)
	{
	#ifdef _DEBUG
	   if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft()) && (SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)))
		  OTWDriver.todOffset += 1800.0F;
	#endif
	}
#endif

void OTWStepNextAC (unsigned long, int state, void*)
{
   if ((state & KEY_DOWN))
      OTWDriver.ViewStepNext();
}

void OTWStepPrevAC (unsigned long, int state, void*)
{
   if ((state & KEY_DOWN))
      OTWDriver.ViewStepPrev();
}

void OTWStepNextPadlock (unsigned long, int state, void*)
{
	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft()) && (SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP))) {
		// 2002-02-08 ADDED BY S.G. If we're not in PadlockF3 or PadlockEFOV mode, we need to switch to it first...
		if (FalconLocalGame->rules.GetPadlockMode() != PDDisabled && OTWDriver.GetOTWDisplayMode() != OTWDriverClass::ModePadlockF3 && OTWDriver.GetOTWDisplayMode() != OTWDriverClass::ModePadlockEFOV) {
			OTWDriver.Padlock_SetPriority(OTWDriverClass::PriorityNone);
			OTWDriver.SetOTWDisplayMode(OTWDriverClass::ModePadlockF3);
		}
		else // END OF ADDED SECTION 2002-02-08
			OTWDriver.TargetStepNext();
	}
}

// 2002-03-12 ADDED BY S.G. So we can priorotize air things
void OTWStepNextPadlockAA (unsigned long, int state, void*)
{
	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft()) && (SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP))) {
		// 2002-02-08 ADDED BY S.G. If we're not in PadlockF3 or PadlockEFOV mode, we need to switch to it first...
		if (FalconLocalGame->rules.GetPadlockMode() != PDDisabled && OTWDriver.GetOTWDisplayMode() != OTWDriverClass::ModePadlockF3 && OTWDriver.GetOTWDisplayMode() != OTWDriverClass::ModePadlockEFOV) {
			OTWDriver.Padlock_SetPriority(OTWDriverClass::PriorityAA);
			OTWDriver.SetOTWDisplayMode(OTWDriverClass::ModePadlockF3);
		}
		else // END OF ADDED SECTION 2002-02-08
			OTWDriver.TargetStepNext();
	}
}

// 2002-03-12 ADDED BY S.G. So we can priorotize ground things
void OTWStepNextPadlockAG (unsigned long, int state, void*)
{
	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft()) && (SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP))) {
		// 2002-02-08 ADDED BY S.G. If we're not in PadlockF3 or PadlockEFOV mode, we need to switch to it first...
		if (FalconLocalGame->rules.GetPadlockMode() != PDDisabled && OTWDriver.GetOTWDisplayMode() != OTWDriverClass::ModePadlockF3 && OTWDriver.GetOTWDisplayMode() != OTWDriverClass::ModePadlockEFOV) {
			OTWDriver.Padlock_SetPriority(OTWDriverClass::PriorityAG);
			OTWDriver.SetOTWDisplayMode(OTWDriverClass::ModePadlockF3);
		}
		else // END OF ADDED SECTION 2002-02-08
			OTWDriver.TargetStepNext();
	}
}

void OTWStepPrevPadlock (unsigned long, int state, void*)
{
	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft()) && (SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP))) {
		// 2002-02-08 ADDED BY S.G. If we're not in PadlockF3 or PadlockEFOV mode, we need to switch to it first...
		if (FalconLocalGame->rules.GetPadlockMode() != PDDisabled && OTWDriver.GetOTWDisplayMode() != OTWDriverClass::ModePadlockF3 && OTWDriver.GetOTWDisplayMode() != OTWDriverClass::ModePadlockEFOV) {
			OTWDriver.Padlock_SetPriority(OTWDriverClass::PriorityNone);
			OTWDriver.SetOTWDisplayMode(OTWDriverClass::ModePadlockF3);
		}
		else // END OF ADDED SECTION 2002-02-08
			OTWDriver.TargetStepPrev();
	}
}

// 2002-03-12 ADDED BY S.G. So we can priorotize air things
void OTWStepPrevPadlockAA (unsigned long, int state, void*)
{
	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft()) && (SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP))) {
		// 2002-02-08 ADDED BY S.G. If we're not in PadlockF3 or PadlockEFOV mode, we need to switch to it first...
		if (FalconLocalGame->rules.GetPadlockMode() != PDDisabled && OTWDriver.GetOTWDisplayMode() != OTWDriverClass::ModePadlockF3 && OTWDriver.GetOTWDisplayMode() != OTWDriverClass::ModePadlockEFOV) {
			OTWDriver.Padlock_SetPriority(OTWDriverClass::PriorityAA);
			OTWDriver.SetOTWDisplayMode(OTWDriverClass::ModePadlockF3);
		}
		else // END OF ADDED SECTION 2002-02-08
			OTWDriver.TargetStepPrev();
	}
}

// 2002-03-12 ADDED BY S.G. So we can priorotize ground things
void OTWStepPrevPadlockAG (unsigned long, int state, void*)
{
	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft()) && (SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP))) {
		// 2002-02-08 ADDED BY S.G. If we're not in PadlockF3 or PadlockEFOV mode, we need to switch to it first...
		if (FalconLocalGame->rules.GetPadlockMode() != PDDisabled && OTWDriver.GetOTWDisplayMode() != OTWDriverClass::ModePadlockF3 && OTWDriver.GetOTWDisplayMode() != OTWDriverClass::ModePadlockEFOV) {
			OTWDriver.Padlock_SetPriority(OTWDriverClass::PriorityAG);
			OTWDriver.SetOTWDisplayMode(OTWDriverClass::ModePadlockF3);
		}
		else // END OF ADDED SECTION 2002-02-08
			OTWDriver.TargetStepPrev();
	}
}

void OTWToggleNames (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
      OTWDriver.IDTagToggle();
}

void OTWToggleCampNames (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
      OTWDriver.CampTagToggle();
}

void OTWToggleActionCamera (unsigned long, int state, void*)
{
	if (state & KEY_DOWN && FalconLocalGame->rules.ExternalViewOn())
      OTWDriver.ToggleActionCamera();
}

void OTWSelectEFOVPadlockMode (unsigned long, int state, void*)
{
   if ( (FalconLocalGame->rules.GetPadlockMode() != PDDisabled) && (state & KEY_DOWN) && \
	   (SimDriver.GetPlayerAircraft()) && (SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)))
   {
	  // 20020-03-12 MODIFIED BY S.G. Not working, lets try with three different functions, one that doesn't care, one that checks for AA and one for AG
/*	  if (state & CTRL_KEY)
		 OTWDriver.Padlock_SetPriority(OTWDriverClass::PriorityAG);
	  else
		 OTWDriver.Padlock_SetPriority(OTWDriverClass::PriorityAA);
*/
	  OTWDriver.Padlock_SetPriority(OTWDriverClass::PriorityNone);
	  // END OF MODIFIED SECTION
	  OTWDriver.SetOTWDisplayMode(OTWDriverClass::ModePadlockEFOV);
   }
}

// 2002-03-12 ADDED BY S.G. So we can priorotize air things
void OTWSelectEFOVPadlockModeAA (unsigned long, int state, void*)
{
   if ( (FalconLocalGame->rules.GetPadlockMode() != PDDisabled) && (state & KEY_DOWN) && \
	   (SimDriver.GetPlayerAircraft()) && (SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)))
   {
	  OTWDriver.Padlock_SetPriority(OTWDriverClass::PriorityAA);
	  OTWDriver.SetOTWDisplayMode(OTWDriverClass::ModePadlockEFOV);
   }
}

// 2002-03-12 ADDED BY S.G. So we can priorotize ground things
void OTWSelectEFOVPadlockModeAG (unsigned long, int state, void*)
{
   if ( (FalconLocalGame->rules.GetPadlockMode() != PDDisabled) && (state & KEY_DOWN) && \
	   (SimDriver.GetPlayerAircraft()) && (SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)))
   {
	  OTWDriver.Padlock_SetPriority(OTWDriverClass::PriorityAG);
	  OTWDriver.SetOTWDisplayMode(OTWDriverClass::ModePadlockEFOV);
   }
}

void OTWSelectF3PadlockMode (unsigned long, int state, void*)
{
   if ( (FalconLocalGame->rules.GetPadlockMode() != PDDisabled) && (state & KEY_DOWN) && \
	   (SimDriver.GetPlayerAircraft()) && (SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)))
   {
	  // 20020-03-12 ADDED BY S.G. If we ask for, allow multi state padlocking by choosing what to look for	  
	  // 20020-03-12 MODIFIED BY S.G. Not working, lets try with three different functions, one that doesn't care, one that checks for AA and one for AG
/*	  if (state & CTRL_KEY)
		 OTWDriver.Padlock_SetPriority(OTWDriverClass::PriorityAG);
	  else
		 OTWDriver.Padlock_SetPriority(OTWDriverClass::PriorityAA);
*/
	  OTWDriver.Padlock_SetPriority(OTWDriverClass::PriorityNone);
	  // END OF MODIFIED SECTION
	  OTWDriver.SetOTWDisplayMode(OTWDriverClass::ModePadlockF3);
   }
}

// 2002-03-12 ADDED BY S.G. So we can priorotize air things
void OTWSelectF3PadlockModeAA (unsigned long, int state, void*)
{
   if ( (FalconLocalGame->rules.GetPadlockMode() != PDDisabled) && (state & KEY_DOWN) && \
	   (SimDriver.GetPlayerAircraft()) && (SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)))
   {
	  OTWDriver.Padlock_SetPriority(OTWDriverClass::PriorityAA);
	  OTWDriver.SetOTWDisplayMode(OTWDriverClass::ModePadlockF3);
   }
}

// 2002-03-12 ADDED BY S.G. So we can priorotize ground things
void OTWSelectF3PadlockModeAG (unsigned long, int state, void*)
{
   if ( (FalconLocalGame->rules.GetPadlockMode() != PDDisabled) && (state & KEY_DOWN) && \
	   (SimDriver.GetPlayerAircraft()) && (SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)))
   {
	  OTWDriver.Padlock_SetPriority(OTWDriverClass::PriorityAG);
	  OTWDriver.SetOTWDisplayMode(OTWDriverClass::ModePadlockF3);
   }
}

void OTWStepMFD1 (unsigned long, int state, void*)
{
   if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft()) && (SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)))
   {
      MfdDisplay[0]->changeMode = TRUE_NEXT;
	  //MI
	  if(SimDriver.GetPlayerAircraft()->FCC && SimDriver.GetPlayerAircraft()->FCC->IsSOI && g_bRealisticAvionics)
	  {
		  if(MfdDisplay[0]->CurMode() == MFDClass::FCCMode)
			  SimDriver.GetPlayerAircraft()->StepSOI(2);
	  }
   } 
}

void OTWStepMFD2 (unsigned long, int state, void*)
{
   if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft()) && (SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)))
   {
      MfdDisplay[1]->changeMode = TRUE_NEXT;
	  //MI
	  if(SimDriver.GetPlayerAircraft()->FCC && SimDriver.GetPlayerAircraft()->FCC->IsSOI && g_bRealisticAvionics)
	  {
		  if(MfdDisplay[1]->CurMode() == MFDClass::FCCMode)
			  SimDriver.GetPlayerAircraft()->StepSOI(2);
	  }
   } 
}

void OTWStepMFD3 (unsigned long, int state, void*)
{
   if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft()) && (SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)))
   {
      MfdDisplay[2]->changeMode = TRUE_NEXT;
	  //MI
	  if(SimDriver.GetPlayerAircraft()->FCC && SimDriver.GetPlayerAircraft()->FCC->IsSOI && g_bRealisticAvionics)
	  {
		  if(MfdDisplay[2]->CurMode() == MFDClass::FCCMode)
			  SimDriver.GetPlayerAircraft()->StepSOI(2);
	  }
   }
}

void OTWStepMFD4 (unsigned long, int state, void*)
{
   if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft()) && (SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)))
   {
      MfdDisplay[3]->changeMode = TRUE_NEXT;
	  //MI
	  if(SimDriver.GetPlayerAircraft()->FCC && SimDriver.GetPlayerAircraft()->FCC->IsSOI && g_bRealisticAvionics)
	  {
		  if(MfdDisplay[3]->CurMode() == MFDClass::FCCMode)
			  SimDriver.GetPlayerAircraft()->StepSOI(2);
	  }
   }
}

void OTWToggleScales (unsigned long, int state, void*)
{
   if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft()) && (SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)))
   {
		TheHud->CycleScalesSwitch();
		if (OTWDriver.GetVirtualCockpit())
			if (TheHud->GetScalesSwitch() < 3)
				//OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_HUD_VAH, 3 - TheHud->GetScalesSwitch());
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_HUD_VAH, 1<<(2 - TheHud->GetScalesSwitch()));
			else
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_HUD_VAH, 1);
   }
}

// MD -- 20031025: Adding commands to set the Scales switch explicitly which
// should help cockpit builders to map the game functions to physical switches better.

void SimScalesVVVAH(unsigned long, int state, void*)
{
	if(!g_bRealisticAvionics)
		return;
	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft()) && (SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)))
	{
		TheHud->SetScalesSwitch(HudClass::VV_VAH);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_HUD_VAH, 2);
	}
}

void SimScalesVAH(unsigned long, int state, void*)
{
	if(!g_bRealisticAvionics)
		return;
	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft()) && (SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)))
	{
		TheHud->SetScalesSwitch(HudClass::VAH);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_HUD_VAH, 4);
	}
}

void SimScalesOff(unsigned long, int state, void*)
{
	if(!g_bRealisticAvionics)
		return;
	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft()) && (SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)))
	{
		TheHud->SetScalesSwitch(HudClass::SS_OFF);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_HUD_VAH, 1);
	}
}

void OTWTogglePitchLadder (unsigned long, int state, void*)
{
   if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft()) && (SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)))
   {
		TheHud->CycleFPMSwitch();
		if (OTWDriver.GetVirtualCockpit())
			//OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_HUD_FPM_LADD, TheHud->GetFPMSwitch()+1);
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_HUD_FPM_LADD, 1<<TheHud->GetFPMSwitch());
   }
}

// MD -- 20031026: adding commands to place FPM switch directly which should make it
// easier for cockpit builders to map this function to a physical switch

void SimPitchLadderOff(unsigned long, int state, void*)
{
	if(!g_bRealisticAvionics)
		return;
   if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft()) && (SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)))
   {
	   TheHud->SetFPMSwitch(HudClass::FPM_OFF);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_HUD_FPM_LADD, 1);
   }
}

void SimPitchLadderFPM(unsigned long, int state, void*)
{
	if(!g_bRealisticAvionics)
		return;
   if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft()) && (SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)))
   {
	   TheHud->SetFPMSwitch(HudClass::FPM);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_HUD_FPM_LADD, 2);
   }
}

void SimPitchLadderATTFPM(unsigned long, int state, void*)
{
	if(!g_bRealisticAvionics)
		return;
   if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft()) && (SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)))
   {
	   TheHud->SetFPMSwitch(HudClass::ATT_FPM);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_HUD_FPM_LADD, 4);
   }
}

void OTWStepHeadingScale (unsigned long, int state, void*)
{
   if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft()) && (SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)))
      OTWDriver.StepHeadingScale();
}

void OTWSelectHUDMode (unsigned long, int state, void*)
{
	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft()) && (SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)))
		OTWDriver.SetOTWDisplayMode(OTWDriverClass::ModeHud);
}

void OTWToggleGLOC (unsigned long, int state, void*)
{
   if ((state & KEY_DOWN) && !FalconLocalGame->rules.BlackoutOn() && \
	   (SimDriver.GetPlayerAircraft()) && (SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)))
      OTWDriver.ToggleGLOC();
}

void OTWSelectChaseMode (unsigned long, int state, void*)
{
   if (FalconLocalGame && FalconLocalGame->rules.ExternalViewOn() && \
		(state & KEY_DOWN) && (SimDriver.GetPlayerAircraft()) && \
		(SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) )
		OTWDriver.SetOTWDisplayMode(OTWDriverClass::ModeChase);
}

//RAS -5Dec04- Added ext view on ground if g_bExtViewOnGround is set
void OTWSelectOrbitMode (unsigned long, int state, void*)
{
	if (FalconLocalGame && SimDriver.GetPlayerAircraft()) //added SimDriver check
	{
		if(FalconLocalGame->rules.ExternalViewOn() || (!FalconLocalGame->rules.ExternalViewOn()
			&& SimDriver.GetPlayerAircraft()->OnGround()&& g_bExtViewOnGround))
		{
			if(state & KEY_DOWN)
			{
				// if(SimDriver.GetPlayerAircraft())
				// {
				// if(SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP))
				OTWDriver.SetOTWDisplayMode(OTWDriverClass::ModeOrbit);
				// }
			}
		}
	}
}

void OTWTrackExternal(unsigned long, int state, void*) 
{
   if (FalconLocalGame && FalconLocalGame->rules.ExternalViewOn() && 
	   (state & KEY_DOWN) && (SimDriver.GetPlayerAircraft()) && 
	   (SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) )
      OTWDriver.SetOTWDisplayMode(OTWDriverClass::ModeTargetToSelf);
}

void OTWTrackTargetToWeapon(unsigned long, int state, void*)
{
   if (FalconLocalGame && FalconLocalGame->rules.ExternalViewOn() && \
	   (state & KEY_DOWN) && (SimDriver.GetPlayerAircraft()) && \
	   (SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) )
      OTWDriver.SetOTWDisplayMode(OTWDriverClass::ModeTargetToWeapon);
}

void OTWSelectAirFriendlyMode (unsigned long, int state, void*)
{
   if (FalconLocalGame && FalconLocalGame->rules.ExternalViewOn() && \
	   (state & KEY_DOWN) && (SimDriver.GetPlayerAircraft()) && \
	   (SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)))
      OTWDriver.SetOTWDisplayMode(OTWDriverClass::ModeAirFriendly);
}

void OTWSelectIncomingMode (unsigned long, int state, void*)
{
   if (FalconLocalGame && FalconLocalGame->rules.ExternalViewOn() && \
	   (state & KEY_DOWN) && (SimDriver.GetPlayerAircraft()) && \
	   (SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)))
      OTWDriver.SetOTWDisplayMode(OTWDriverClass::ModeIncoming);
}

void OTWSelectGroundFriendlyMode (unsigned long, int state, void*)
{
   if (FalconLocalGame && FalconLocalGame->rules.ExternalViewOn() && \
	   (state & KEY_DOWN) && (SimDriver.GetPlayerAircraft()) && \
	   (SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)))
      OTWDriver.SetOTWDisplayMode(OTWDriverClass::ModeGroundFriendly);
}

void OTWSelectAirEnemyMode (unsigned long, int state, void*)
{
   if (FalconLocalGame && FalconLocalGame->rules.ExternalViewOn() && \
	   (state & KEY_DOWN) && (SimDriver.GetPlayerAircraft()) && \
	   (SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)))
      OTWDriver.SetOTWDisplayMode(OTWDriverClass::ModeAirEnemy);
}

void OTWSelectGroundEnemyMode (unsigned long, int state, void*)
{
   if (FalconLocalGame && FalconLocalGame->rules.ExternalViewOn() && \
	   (state & KEY_DOWN) && (SimDriver.GetPlayerAircraft()) && \
	   (SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)))
      OTWDriver.SetOTWDisplayMode(OTWDriverClass::ModeGroundEnemy);
}

void OTWSelectTargetMode (unsigned long, int state, void*)
{
   if (FalconLocalGame && FalconLocalGame->rules.ExternalViewOn() && \
	   (state & KEY_DOWN) && (SimDriver.GetPlayerAircraft()) && \
	   (SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)))
      OTWDriver.SetOTWDisplayMode(OTWDriverClass::ModeTarget);
}

void OTWSelectWeaponMode (unsigned long, int state, void*)
{
   if (FalconLocalGame && FalconLocalGame->rules.ExternalViewOn() && \
	   (state & KEY_DOWN) && (SimDriver.GetPlayerAircraft()) && \
	   (SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)))
      OTWDriver.SetOTWDisplayMode(OTWDriverClass::ModeWeapon);
}

//RAS -5Dec04- Added ext view if on ground if g_bExtViewOnGround is set
void OTWSelectSatelliteMode (unsigned long, int state, void*)
{
   if (FalconLocalGame && SimDriver.GetPlayerAircraft() && (FalconLocalGame->rules.ExternalViewOn() ||
       !FalconLocalGame->rules.ExternalViewOn() && SimDriver.GetPlayerAircraft()->OnGround()&&
	   g_bExtViewOnGround) && \
       (state & KEY_DOWN) &&  \
       (SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)))
      OTWDriver.SetOTWDisplayMode(OTWDriverClass::ModeSatellite);
}

void OTWSelectFlybyMode (unsigned long, int state, void*)
{
   if (FalconLocalGame && FalconLocalGame->rules.ExternalViewOn() && \
	   (state & KEY_DOWN) && (SimDriver.GetPlayerAircraft()) && \
	   (SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)))
      OTWDriver.SetOTWDisplayMode(OTWDriverClass::ModeFlyby);
}

void OTWShowTestVersion (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
   {
      if (ShowVersion != 2)
         ShowVersion = 2;
      else
         ShowVersion = 0;
   }
}

void OTWShowVersion (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
   {
      if (ShowVersion != 1)
         ShowVersion = 1;
      else
         ShowVersion = 0;
   }
}


void OTWSelect2DCockpitMode (unsigned long, int state, void*)
{
   if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft()) && (SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)))
	{
		OTWDriver.SetOTWDisplayMode(OTWDriverClass::Mode2DCockpit);
	}
}

void OTWSelect3DCockpitMode (unsigned long, int state, void*)
{
   if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft()) && (SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)))
      OTWDriver.SetOTWDisplayMode(OTWDriverClass::Mode3DCockpit);
}

void OTWToggleBilinearFilter (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
      OTWDriver.ToggleBilinearFilter();
}

void OTWToggleShading (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
      OTWDriver.ToggleShading();
}

void OTWToggleHaze (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
      OTWDriver.ToggleHaze();
}

void OTWToggleLocationDisplay (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
      OTWDriver.ToggleLocationDisplay();
}

void OTWToggleAeroDisplay (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
      OTWDriver.ToggleAeroDisplay();
}

//TJL 11/09/03 On/Off Flap Display
void OTWToggleFlapDisplay (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
      OTWDriver.ToggleFlapDisplay();
}

// Retro 1Feb2004 start
void OTWToggleEngineDisplay (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
      OTWDriver.ToggleEngineDisplay();
}
// Retro 1Feb2004 end

void OTWScaleDown (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
      OTWDriver.ScaleDown();
}

void OTWScaleUp (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
      OTWDriver.ScaleUp();
}

void OTWSetObjDetail (unsigned long val, int state, void*)
{
   if (state & KEY_DOWN)
      OTWDriver.SetDetail(val - DIK_1);
}

void OTWObjDetailDown (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
      OTWDriver.DetailDown();
}

void OTWObjDetailUp (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
      OTWDriver.DetailUp();
}

/*JAM 01Dec03 - Removing these
void OTWTextureIncrease (unsigned long, int state, void*) {

   if (state & KEY_DOWN)
      OTWDriver.TextureUp();
}

//JAM 01Dec03 - Removing these
void OTWTextureDecrease (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
      OTWDriver.TextureDown();
}
*/
void OTWToggleClouds (unsigned long, int state, void*)
{
   if (state & KEY_DOWN && FalconLocalGame->rules.WeatherOn())
      OTWDriver.ToggleWeather();
}

void OTWStepHudColor (unsigned long, int state, void*)
{
   if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft()) && (SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)))
	 {
		if(TheHud)
		{
      TheHud->HudColorStep();
			if (OTWDriver.GetVirtualCockpit())
				//OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_ICP_BRT_WHEEL, curColorIdx+1);
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_ICP_BRT_WHEEL, 1<<curColorIdx);
		}
	 }
}

void OTWStepHudContrastDn (unsigned long, int state, void*)
{
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
  {
		if(TheHud)
		{
			TheHud->ContWheelPos -= 0.1F;
			if(TheHud->ContWheelPos < 0.0F) TheHud->ContWheelPos = 0.0F;
			TheHud->SetContrastLevel();
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_ICP_BRT_WHEEL, 1<<(((int)(TheHud->ContWheelPos*10))+1));
    }
	}
}

void OTWStepHudContrastUp (unsigned long, int state, void*)
{
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
    {
		if(TheHud)
		{
			TheHud->ContWheelPos += 0.1F;
			if(TheHud->ContWheelPos > 1.0F) TheHud->ContWheelPos = 1.0F;
			TheHud->SetContrastLevel();
		}
    }
}


void OTWToggleEyeFly (unsigned long, int state, void*)
{
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) // 2002-02-15 MODIFIED BY S.G. ToggleEyeFly brings you to another AC and can crash when you're ejected since your class isn't AircraftClass anymore
      OTWDriver.ToggleEyeFly();
}

void OTWEnterPosition (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
      OTWDriver.StartLocationEntry();
}

void OTWToggleFrameRate (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
      ShowFrameRate = 1 - ShowFrameRate;
}

void OTWToggleAutoScale (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
	   FalconDisplay.ToggleFullScreen();
      //OTWDriver.ToggleAutoScale();
}

void OTWSetScale (unsigned long val, int state, void*)
{
   if (state & KEY_DOWN)
   {
      OTWDriver.SetScale((float)(val - DIK_1 + 1));
      OTWDriver.RescaleAllObjects();
   }
}

void OTWViewUpRight (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
   {
      OTWDriver.ViewTiltUp();
	  OTWDriver.ViewSpinRight();
	  }
   else
      OTWDriver.ViewTiltHold();
}

void OTWViewUpLeft (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
   {
      OTWDriver.ViewTiltUp();
	  OTWDriver.ViewSpinLeft();
	  }
   else
      OTWDriver.ViewTiltHold();
}

void OTWViewDownRight (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
   {
      OTWDriver.ViewTiltDown();
	  OTWDriver.ViewSpinRight();
	  }
   else
      OTWDriver.ViewTiltHold();
}

void OTWViewDownLeft (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
   {
      OTWDriver.ViewTiltDown();
	  OTWDriver.ViewSpinLeft();
	  }
   else
      OTWDriver.ViewTiltHold();
}

void OTWViewUp (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
      OTWDriver.ViewTiltUp();
   else
      OTWDriver.ViewTiltHold();
}

void OTWViewDown (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
      OTWDriver.ViewTiltDown();
   else
      OTWDriver.ViewTiltHold();
}

void OTWViewLeft (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
      OTWDriver.ViewSpinLeft();
   else
      OTWDriver.ViewSpinHold();
}

void OTWViewRight (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
      OTWDriver.ViewSpinRight();
   else
      OTWDriver.ViewSpinHold();
}

void OTWViewReset (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
      OTWDriver.ViewReset();
}

void OTWViewZoomIn (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
   {
      OTWDriver.ViewZoomIn();

	  /*
	  ** edg: what is this?!
	  ** leave it our becuase its causing a CRASH when ejected
      theFault = theFault ++;
      if (theFault > 32)
         theFault = 0;
      MonoPrint ("Next fail %s\n", FaultClass::mpFSubSystemNames[theFault]);
	  */
   }
}

void OTWViewZoomOut (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
   {
      OTWDriver.ViewZoomOut();
	  /*
	  ** edg: what is this?!
	  ** leave it our becuase its causing a CRASH when ejected
      ((AircraftClass*)SimDriver.GetPlayerAircraft())->AddFault(1, (1 << theFault), 1, 0);
	  */
   }
}

void OTWSwapMFDS (unsigned long, int state, void*)
{
   if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft()) && (SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)))
      MFDSwapDisplays();
}

void OTWGlanceForward (unsigned long, int, void*)
{
	if ((SimDriver.GetPlayerAircraft()) && (SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)))
		OTWDriver.GlanceForward();
}

void OTWCheckSix (unsigned long, int, void*)
{
	if ((SimDriver.GetPlayerAircraft()) && (SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)))
		OTWDriver.GlanceAft();
}

//JAM 08Nov03 - Can CTD? DOES CTD!!
void OTWStateStep (unsigned long, int state, void*)
{
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) // 2002-02-15 MODIFIED BY S.G. ToggleEyeFly brings you to another AC and can crash when you're ejected since your class isn't AircraftClass anymore
      OTWDriver.EyeFlyStateStep();
}


void CommandsSetKeyCombo (unsigned long val, int state, void*)
{
   if (state & KEY_DOWN)
   {
      CommandsKeyCombo = val;
      CommandsKeyComboMod = state & MODS_MASK;
   }
}

void KevinsFistOfGod (unsigned long, int state, void*)
{
	if (state & KEY_DOWN)
		RequestPlayerDivert();
}

void SuperCruise (unsigned long, int state, void*)
{
	if (state & KEY_DOWN)
	{
		supercruise = 1 - supercruise;
	}
}

// 2000-11-10 FUNCTION ADDED BY S.G. TO HANDLE THE 'driftCO' switch
void SimDriftCO (unsigned long, int state, void*)
{
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) // 2002-02-15 MODIFIED BY S.G. Only valid when not ejected
	{
		TheHud->CycleDriftCOSwitch();
		if (OTWDriver.GetVirtualCockpit())
			//OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_ICP_DRIFTCO, TheHud->GetDriftCOSwitch()+1);
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_ICP_DRIFTCO, 1<<TheHud->GetDriftCOSwitch());
	}
}

// MD -- 20031025: Adding commands to place the Drift/CO switch explicitly to help
// cockpit builders map this to a physical switch more easily.  NB: Since the physical
// switch for this control is an ON-OFF-(ON) with momentary to the Warn Reset side, there
// is no need for separate functions for the Warn Reset half.

void SimDriftCOOn (unsigned long, int state, void*)
{
	if(!g_bRealisticAvionics)
		return;
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) // 2002-02-15 MODIFIED BY S.G. Only valid when not ejected
	{
		TheHud->SetDriftCOSwitch(HudClass::DRIFT_CO_ON);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_ICP_DRIFTCO, 2);
	}
}

void SimDriftCOOff (unsigned long, int state, void*)
{
	if(!g_bRealisticAvionics)
		return;
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) // 2002-02-15 MODIFIED BY S.G. Only valid when not ejected
	{
		TheHud->SetDriftCOSwitch(HudClass::DRIFT_CO_OFF);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_ICP_DRIFTCO, 1);
	}
}

// END OF ADDED SECTION

// 2000-11-17 FUNCTION ADDED BY S.G. TO HANDLE THE 'Cat I/III' switch
void SimCATSwitch(unsigned long, int state, void*)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC && playerAC->IsSetFlag(MOTION_OWNSHIP) &&  (state & KEY_DOWN) &&  playerAC->Sms){
		// MI Changed to what we have before, so everything works like before
		// SimDriver.GetPlayerAircraft()->Sms->ChooseLimiterMode(127); // 127 means check Cat config
		SimDriver.GetPlayerAircraft()->Sms->StepCatIII();
		if (OTWDriver.GetVirtualCockpit())
		{
			if(SimDriver.GetPlayerAircraft()->af->IsSet(AirframeClass::CATLimiterIII))
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_STORES_CAT, 2);
			else
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_STORES_CAT, 1);
		}
	}
}

// MD -- 20031120: adding explicit commands to place the CAT switch for cockpit builders.

void SimCATI(unsigned long, int state, void*)
{
	if(!g_bRealisticAvionics)
		return;

	if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN) && SimDriver.GetPlayerAircraft()->Sms)
	{
		SimDriver.GetPlayerAircraft()->af->ClearFlag(AirframeClass::CATLimiterIII);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_STORES_CAT, 1);
	}
}

void SimCATIII(unsigned long, int state, void*)
{
	if(!g_bRealisticAvionics)
		return;

	if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN) && SimDriver.GetPlayerAircraft()->Sms)
	{
		SimDriver.GetPlayerAircraft()->af->SetFlag(AirframeClass::CATLimiterIII);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_STORES_CAT, 2);
	}
}

// END OF ADDED SECTION

void OTW1200DView (unsigned long, int state, void*){
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) {
		OTWDriver.pCockpitManager->SetActivePanel(100);	//100 = id for 12:00 Down panel
	}
}

void OTW1200HUDView (unsigned long, int state, void*){
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) {
		OTWDriver.pCockpitManager->SetActivePanel(0);	//0 = id for 12:00 hud panel
	}
}

void OTW1200View (unsigned long, int state, void*)
{
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) 
   {
	   //MI check for nightlighting
	   if(SimDriver.GetPlayerAircraft())
	   {
		   if(SimDriver.GetPlayerAircraft()->WideView)
			   OTWDriver.pCockpitManager->SetActivePanel(91100);
		   else
			   OTWDriver.pCockpitManager->SetActivePanel(1100);
	   }
	   else
		   OTWDriver.pCockpitManager->SetActivePanel(1100);	//1100 = id for 12:00 50-50 panel
	}
}

void OTW1200LView (unsigned long, int state, void*){
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) {
		OTWDriver.pCockpitManager->SetActivePanel(600 );	//100 = id for 12:00 panel
	}
}

void OTW1000View(unsigned long, int state, void*){
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) {
		OTWDriver.pCockpitManager->SetActivePanel(500);	//101 = id for 10:00 panel
	}
}

void OTW200View(unsigned long, int state, void*){
	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft()) && (SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)))
	{
		OTWDriver.pCockpitManager->SetActivePanel(200);	//102 = id for 2:00 panel
	}
}

void OTW300View(unsigned long, int state, void*)
{
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) 
   {
		OTWDriver.pCockpitManager->SetActivePanel(300);	//103 = id for 3:00 panel
   } 
}

void OTW400View(unsigned long, int state, void*){
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) {
		OTWDriver.pCockpitManager->SetActivePanel(400);	//104 = id for 4:00 panel
	}
}

void OTW800View(unsigned long, int state, void*){
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) {
		OTWDriver.pCockpitManager->SetActivePanel(800);	//105 = id for 8:00 panel
	}
}

void OTW900View(unsigned long, int state, void*){
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) {
		OTWDriver.pCockpitManager->SetActivePanel(900);	//106 = id for 9:00 panel
	}
}

void OTW1200RView(unsigned long, int state, void*){
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) {
		OTWDriver.pCockpitManager->SetActivePanel(700);	//106 = id for 9:00 panel
	}
}

void SimToggleChatMode(unsigned long, int, void*)
{
   if (SimDriver.GetPlayerAircraft())
   {
//	   F4ChatToggleXmitReceive();
   }
}

void SimReverseThrusterToggle(unsigned long, int state, void*)
{
	if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && SimDriver.GetPlayerAircraft()->OnGround())
	{
		if (SimDriver.GetPlayerAircraft()->af->thrustReverse == 0)
		{
			SimDriver.GetPlayerAircraft()->af->thrustReverse = 2;
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_REV_THRUSTER, 2);
		}
		else
		{
			SimDriver.GetPlayerAircraft()->af->thrustReverse = 0;
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_REV_THRUSTER, 1);
		}
	}
}

void SimReverseThrusterOn(unsigned long, int state, void*)
{
	if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && SimDriver.GetPlayerAircraft()->OnGround())
	{
		SimDriver.GetPlayerAircraft()->af->thrustReverse = 2;
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_REV_THRUSTER, 2);
	}
}

void SimReverseThrusterOff(unsigned long, int state, void*)
{
	if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && SimDriver.GetPlayerAircraft()->OnGround())
	{
		SimDriver.GetPlayerAircraft()->af->thrustReverse = 0;
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_REV_THRUSTER, 1);
	}
}

void SimWheelBrakes(unsigned long, int state, void*)
{
	//Cobra double tap to turn on thrust reverse
	static VU_TIME thrrevtimer = 0;
	

	if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP))
	{
		if( state & KEY_DOWN)
		{
			if (thrrevtimer == 0)  
			{
				thrrevtimer = SimLibElapsedTime;
			}
		}
		else
		{   
			if(SimLibElapsedTime - thrrevtimer <= 500 && SimDriver.GetPlayerAircraft()->af->thrustReverse < 2)
			{
				SimDriver.GetPlayerAircraft()->af->thrustReverse += 1;
			}
			else
			{
				SimDriver.GetPlayerAircraft()->af->thrustReverse = 0;
			}
		thrrevtimer = 0;
		}
	}

	// MD -- 20040106: adding support for analog wheel braking channel.
	// Right now there is no support for differential braking!
	if (IO.AnalogIsUsed(AXIS_BRAKE_LEFT))
		return;
	
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP))
   {
	   //TJL 01/05/04 JFS/Brake Accumulators have 75 seconds of brake power stored.
	   //Removed the HydB condition as this is not realistic.
	   //TODO: If HydB is 0 or Engine RPM <12% then 75 seconds of accumulator braking.
      //if (SimDriver.GetPlayerAircraft()->af->HydraulicB() == 0) return;
      if (state & KEY_DOWN && !SimDriver.GetPlayerAircraft()->af->IsSet(AirframeClass::GearBroken))
         SimDriver.GetPlayerAircraft()->af->SetFlag(AirframeClass::WheelBrakes);
      else
         SimDriver.GetPlayerAircraft()->af->ClearFlag(AirframeClass::WheelBrakes);
   }
}

void SimMotionFreeze(unsigned long, int state, void*)
{
	if (
		(state & KEY_DOWN) && 
		(SimDriver.GetPlayerAircraft()) && 
		(SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP))
	){
		SimDriver.SetMotion(1 - SimDriver.MotionOn());
		SetTimeCompression(1);
		F4HearVoices();
	}
}


void ScreenShot(unsigned long, int state, void*)
{
	if (state & KEY_DOWN){
		OTWDriver.takeScreenShot = TRUE;
	}
}

// Retro 7May2004 - disable all 2d overlay stuff (text, labels, chat..) before taking shot
void PrettyScreenShot(unsigned long val, int state, void*)
{
   if (state & KEY_DOWN)
   {
	   // my little screen shot state machine.. see OTWDriver.h
	   if (OTWDriver.takePrettyScreenShot == OTWDriverClass::OFF)
	   {
		  OTWDriver.takePrettyScreenShot = OTWDriverClass::EXECUTE;
		  // note state labels, deactivate them for shot
		  OTWDriver.LabelState = DrawableBSP::drawLabels;
		  DrawableBSP::drawLabels = FALSE;
	   }
   }
}

void FOVToggle(unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
   {
			if (fabs(OTWDriver.GetFOV() -  ( g_fDefaultFOV * DTR)) < 1.0f * DTR ) 	
												//Wombat778 2/19/03 changed to so that minor differences in FOV will be allowed 10/31/2003 changed to g_fDefaultFOV 
				                //Wombat778 Original "> 30.0F"  9-27-2003  This change will set the FOV to 20 
												//degrees when it is at 60, and will set it to 60, whenever it is not at 60;
      {
         narrowFOV = TRUE;
         OTWDriver.SetFOV( g_fNarrowFOV * DTR );		//Wombat778 2-21-2004  Changed to g_fNarrowFov
      }
      else
      {
         narrowFOV = FALSE;
         OTWDriver.SetFOV( g_fDefaultFOV * DTR );		//Wombat778 10-31-2003 changed to default FOV
      }
   }
}

void FOVDecrease(unsigned long, int state, void*)		//Wombat778	9-27-2003
{
   if (state & KEY_DOWN)
   {
      if (OTWDriver.GetFOV() > (g_fMinimumFOV + g_fFOVIncrement) * DTR)	//Added g_fMinimumFOV //Ensure that we never set the FOV to 0 or less
      {
         OTWDriver.SetFOV( OTWDriver.GetFOV()  - (g_fFOVIncrement * DTR ) );
		 
				if (OTWDriver.GetFOV() == g_fDefaultFOV * DTR)	//Wombat778 10-31-2003 changed to default FOV  //set narrowFOV to false if we are back to the default FOV
					narrowFOV = FALSE;
				else
					narrowFOV = TRUE;
   
      }
   }
}

void FOVIncrease(unsigned long, int state, void*)		//Wombat778	9-27-2003
{													
   if (state & KEY_DOWN)
   {
	   if (OTWDriver.GetFOV() <= (g_fMaximumFOV - g_fFOVIncrement) * DTR)		//10/11/03 Ensure that we never set the FOV to greater than the max
	   {
				narrowFOV = TRUE;								
				OTWDriver.SetFOV( OTWDriver.GetFOV()  + (g_fFOVIncrement * DTR ) );
		 
				if (OTWDriver.GetFOV() == g_fDefaultFOV * DTR)			//Wombat778 10-31-2003 changed to Default FOV //set narrowFOV to false if we are back to the default FOV
					narrowFOV = FALSE;
				else
					narrowFOV = TRUE;
	   }
   }
}

void FOVDefault(unsigned long, int state, void*)		//Wombat778 9-27-2003
{
   if (state & KEY_DOWN)
   {
			if (OTWDriver.GetOTWDisplayMode() == OTWDriverClass::Mode3DCockpit)
			{
				narrowFOV = FALSE;
				OTWDriver.ViewReset();
				OTWDriver.SetCameraPanTilt(0.0f, (g_f3DHeadTilt * DTR));
				OTWDriver.SetFOV( g_f3DPitFOV * DTR ); // Cobra
			}
			else
			{
				narrowFOV = FALSE;
				OTWDriver.SetFOV( g_fDefaultFOV * DTR );		//Wombat778 10-31-2003 changed to default FOV
			}
   }
}

/*JAM 01Dec03 - Removing this
void OTWToggleAlpha(unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
      OTWDriver.ToggleAlpha ();
}
*/
void ACMIToggleRecording(unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
	  SimDriver.doFile = TRUE;
}


void SimSelectiveJettison(unsigned long, int state, void*)
{
	if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
	if (SimDriver.GetPlayerAircraft()->Sms && SimDriver.GetPlayerAircraft()->Sms->drawable)
         SimDriver.GetPlayerAircraft()->Sms->drawable->SetDisplayMode(SmsDrawable::SelJet);
	}
}

void SimEmergencyJettison(unsigned long, int state, void*)
{
 	if(SimDriver.GetPlayerAircraft() != NULL && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP))
 	{
		if(!SimDriver.GetPlayerAircraft()->Sms || !SimDriver.GetPlayerAircraft()->Sms->drawable)
			return;
		//MI
 		if(g_bRealisticAvionics)
 		{
 			//not if we're on the ground an our switch isn't set
 			if(SimDriver.GetPlayerAircraft()->OnGround() && 
 				SimDriver.GetPlayerAircraft()->Sms && !SimDriver.GetPlayerAircraft()->Sms->GndJett)
 			{
 				return;
 			}
		}
 		//Emergency Jettison is only happening if we hold the button more then 1 sec
 		//if(((AircraftClass *)SimDriver.GetPlayerAircraft())->EmerJettTriggered == FALSE)
 		{
 			if(state & KEY_DOWN)
 			{
 				//MI
 				if(g_bRealisticAvionics)
 				{
					//Set our display mode
					if(MfdDisplay[1]->GetCurMode() != MFDClass::SMSMode || 
						SimDriver.GetPlayerAircraft()->Sms->drawable->DisplayMode() != SmsDrawable::EmergJet)
					{
						MfdDisplay[0]->EmergStoreMode = MfdDisplay[0]->CurMode();
						MfdDisplay[1]->EmergStoreMode = MfdDisplay[1]->CurMode();
						MfdDisplay[1]->SetNewMode(MFDClass::SMSMode);
						if(SimDriver.GetPlayerAircraft()->Sms->drawable->DisplayMode() !=
							SmsDrawable::EmergJet)
						{
							SimDriver.GetPlayerAircraft()->Sms->drawable->EmergStoreMode = 
								SimDriver.GetPlayerAircraft()->Sms->drawable->DisplayMode();

							SimDriver.GetPlayerAircraft()->Sms->drawable->SetDisplayMode(
								SmsDrawable::EmergJet);
						}
					}
					// Start the timer
					((AircraftClass *)SimDriver.GetPlayerAircraft())->JettCountown = 1.0;
					((AircraftClass *)SimDriver.GetPlayerAircraft())->doJettCountdown = TRUE;
				}
 				else
 				{
 					SimDriver.GetPlayerAircraft()->Sms->EmergencyJettison();
 				}
 			}
 			else
 			{
				MfdDisplay[1]->SetNewMode(MfdDisplay[1]->EmergStoreMode);
				SimDriver.GetPlayerAircraft()->Sms->drawable->SetDisplayMode(
					SimDriver.GetPlayerAircraft()->Sms->drawable->EmergStoreMode);
				// Cancel the timer
 				((AircraftClass *)SimDriver.GetPlayerAircraft())->doJettCountdown = FALSE;
 			}
 		}
 	}
}

void SimECMOn(unsigned long, int state, void*)
{
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
      if (SimDriver.GetPlayerAircraft()->IsSetFlag(ECM_ON))
      {
         // Can't turn off ECM w/ ECM pod broken
         if (!SimDriver.GetPlayerAircraft()->mFaults->GetFault(FaultClass::epod_fault))
		 {
            SimDriver.GetPlayerAircraft()->UnSetFlag(ECM_ON);
			//MI EWS stuff
			if(g_bRealisticAvionics)
				SimDriver.GetPlayerAircraft()->ManualECM = FALSE;
		 }
      }
      else
      {
         // Can't turn on ECM w/ broken blanker
         if (SimDriver.GetPlayerAircraft()->HasSPJamming() && !SimDriver.GetPlayerAircraft()->mFaults->GetFault(FaultClass::blkr_fault))
		 {
			 //MI no Jammer with WOW, unless ground Jett is on
			 if(g_bRealisticAvionics && SimDriver.GetPlayerAircraft()->Sms)
			 {
				 if(SimDriver.GetPlayerAircraft()->OnGround() && !SimDriver.GetPlayerAircraft()->Sms->GndJett)
					 return;
			 }
			 SimDriver.GetPlayerAircraft()->SetFlag(ECM_ON);
			 
			 //MI EWS stuff
			 if(g_bRealisticAvionics)
				 SimDriver.GetPlayerAircraft()->ManualECM = TRUE;
		 }
      }
   }
}

//Wombat778 11-3-2003 Added for the pitbuilders.  Really SimECMOn should be named SimECMToggle, but I don't want to break any keystrokes files.
// MD -- 20031128: minor changes following up on conversation with the originator of the request for this feature.

void SimECMStandby(unsigned long, int state, void*)		//Switches ECM off.  Map to HOTAS CMS hat "right"
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;

   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
		 // Can't turn off ECM w/ ECM pod broken
		 if (!SimDriver.GetPlayerAircraft()->mFaults->GetFault(FaultClass::epod_fault))
		 {
			SimDriver.GetPlayerAircraft()->UnSetFlag(ECM_ON);
			//MI EWS stuff
			if(g_bRealisticAvionics)
				SimDriver.GetPlayerAircraft()->ManualECM = FALSE;
		 }     
   }
}

//Wombat778 11-3-2003 Added for the pitbuilders.  Really SimECMOn should be named SimECMToggle, but I don't want to break any keystrokes files.
// MD -- 20031128: minor changes following up on conversation with the originator of the request for this feature.

void SimECMConsent(unsigned long, int state, void*)		//Switches ECM on.  Map to HOTAS CMS hat "down"
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;

	if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
         // Can't turn on ECM w/ broken blanker
         if (SimDriver.GetPlayerAircraft()->HasSPJamming() && !SimDriver.GetPlayerAircraft()->mFaults->GetFault(FaultClass::blkr_fault))
		 {
			 //MI no Jammer with WOW, unless ground Jett is on
			 if(g_bRealisticAvionics && SimDriver.GetPlayerAircraft()->Sms)
			 {
				 if(SimDriver.GetPlayerAircraft()->OnGround() && !SimDriver.GetPlayerAircraft()->Sms->GndJett)
					 return;
			 }
			 SimDriver.GetPlayerAircraft()->SetFlag(ECM_ON);
			 
			 //MI EWS stuff
			 if(g_bRealisticAvionics)
				 SimDriver.GetPlayerAircraft()->ManualECM = TRUE;
		 }
	}
}

void SoundOff(unsigned long, int state, void*)
{
	if(state & KEY_DOWN)
	{
		if(gSoundDriver->GetMasterVolume() <= -7000)
			gSoundDriver->SetMasterVolume(PlayerOptions.GroupVol[MASTER_SOUND_GROUP]);
		else
			gSoundDriver->SetMasterVolume(-10000);
	}
}

/////////////// Vince's Cockpit Stuff /////////////////
void SimHsiCourseInc (unsigned long, int state, void*)
{
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected... Vince, you should know better than this ;-)
		OTWDriver.pCockpitManager->mpHsi->IncState(CPHsi::HSI_STA_CRS_STATE);
		//int val = (int)(OTWDriver.pCockpitManager->mpHsi->GetValue(CPHsi::HSI_VAL_DESIRED_CRS)/36.0f)+1;
		int val = 1<<(int)(OTWDriver.pCockpitManager->mpHsi->GetValue(CPHsi::HSI_VAL_DESIRED_CRS)/36.0f);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_HSI_COURSE, val);
	}
}

void SimHsiCourseDec(unsigned long, int state, void*)
{
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected... Vince, you should know better than this ;-)
		OTWDriver.pCockpitManager->mpHsi->DecState(CPHsi::HSI_STA_CRS_STATE);
		//int val = (int)(OTWDriver.pCockpitManager->mpHsi->GetValue(CPHsi::HSI_VAL_DESIRED_CRS)/36.0f)+1;
		int val = 1<<(int)(OTWDriver.pCockpitManager->mpHsi->GetValue(CPHsi::HSI_VAL_DESIRED_CRS)/36.0f);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_HSI_COURSE, val);
	}
}

void SimHsiHeadingInc(unsigned long, int state, void*)
{
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected... Vince, you should know better than this ;-)
		OTWDriver.pCockpitManager->mpHsi->IncState(CPHsi::HSI_STA_HDG_STATE);
		//int val = (int)(OTWDriver.pCockpitManager->mpHsi->GetValue(CPHsi::HSI_VAL_DESIRED_HEADING)/36.0f)+1;
		int val = 1<<(int)(OTWDriver.pCockpitManager->mpHsi->GetValue(CPHsi::HSI_VAL_DESIRED_HEADING)/36.0f);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_HSI_HEADING, val);
	}
}

void SimHsiHeadingDec(unsigned long, int state, void*)
{
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected... Vince, you should know better than this ;-)
		OTWDriver.pCockpitManager->mpHsi->DecState(CPHsi::HSI_STA_HDG_STATE);
		//int val = (int)(OTWDriver.pCockpitManager->mpHsi->GetValue(CPHsi::HSI_VAL_DESIRED_HEADING)/36.0f)+1;
		int val = 1<<(int)(OTWDriver.pCockpitManager->mpHsi->GetValue(CPHsi::HSI_VAL_DESIRED_HEADING)/36.0f);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_HSI_HEADING, val);
	}
}

// MD -- 20040118: adding commands to increment/decrement HSI values by one degree at a time
void SimHsiCrsIncBy1 (unsigned long, int state, void*)
{
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected... Vince, you should know better than this ;-)
		OTWDriver.pCockpitManager->mpHsi->IncState(CPHsi::HSI_STA_CRS_STATE, 1.0F);
		//int val = (int)(OTWDriver.pCockpitManager->mpHsi->GetValue(CPHsi::HSI_VAL_DESIRED_CRS)/36.0f)+1;
		int val = 1<<(int)(OTWDriver.pCockpitManager->mpHsi->GetValue(CPHsi::HSI_VAL_DESIRED_CRS)/36.0f);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_HSI_COURSE, val);
	}
}

void SimHsiCrsDecBy1(unsigned long, int state, void*)
{
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected... Vince, you should know better than this ;-)
		OTWDriver.pCockpitManager->mpHsi->DecState(CPHsi::HSI_STA_CRS_STATE, 1.0F);
		//int val = (int)(OTWDriver.pCockpitManager->mpHsi->GetValue(CPHsi::HSI_VAL_DESIRED_CRS)/36.0f)+1;
		int val = 1<<(int)(OTWDriver.pCockpitManager->mpHsi->GetValue(CPHsi::HSI_VAL_DESIRED_CRS)/36.0f);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_HSI_COURSE, val);
	}
}

void SimHsiHdgIncBy1(unsigned long, int state, void*)
{
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected... Vince, you should know better than this ;-)
		OTWDriver.pCockpitManager->mpHsi->IncState(CPHsi::HSI_STA_HDG_STATE, 1.0F);
		//int val = (int)(OTWDriver.pCockpitManager->mpHsi->GetValue(CPHsi::HSI_VAL_DESIRED_HEADING)/36.0f)+1;
		int val = 1<<(int)(OTWDriver.pCockpitManager->mpHsi->GetValue(CPHsi::HSI_VAL_DESIRED_HEADING)/36.0f);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_HSI_HEADING, val);
	}
}

void SimHsiHdgDecBy1(unsigned long, int state, void*)
{
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected... Vince, you should know better than this ;-)
		OTWDriver.pCockpitManager->mpHsi->DecState(CPHsi::HSI_STA_HDG_STATE, 1.0F);
		//int val = (int)(OTWDriver.pCockpitManager->mpHsi->GetValue(CPHsi::HSI_VAL_DESIRED_HEADING)/36.0f)+1;
		int val = 1<<(int)(OTWDriver.pCockpitManager->mpHsi->GetValue(CPHsi::HSI_VAL_DESIRED_HEADING)/36.0f);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_HSI_HEADING, val);
	}
}

void SimAVTRToggle(unsigned long val, int state, void* pButton)
{
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) {
		// edg: folded my command function in here.  Not sure if
		// a simdriver setting needs to be made, but keeping it in
		ACMIToggleRecording ( val, state, pButton);
		if(SimDriver.AVTROn() == TRUE) {
			SimDriver.SetAVTR(FALSE);
		}
		else {
			SimDriver.SetAVTR(TRUE);
		}
	}
}


void SimMPOToggle(unsigned long, int state, void*)
{
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) 
	 {
	   ShiAssert(SimDriver.GetPlayerAircraft()->af);
	   if(SimDriver.GetPlayerAircraft()->af->IsSet(AirframeClass::MPOverride))
	   {
		   SimDriver.GetPlayerAircraft()->af->ClearFlag(AirframeClass::MPOverride);
			 if (OTWDriver.GetVirtualCockpit())
					OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_MPO, 1);
	   }
	   else
	   {
		   SimDriver.GetPlayerAircraft()->af->SetFlag(AirframeClass::MPOverride);
			 if (OTWDriver.GetVirtualCockpit())
					OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_MPO, 2);
	   }
	}
}

// MD -- 20031120: adding commands to place the MPO switch exsplicitly for cockpit builders
// MD -- 20031206: Mav points out that this switch is in fact a momentary!

void SimMPO(unsigned long, int state, void*)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;

   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) {
	   if (state & KEY_DOWN)
		   SimDriver.GetPlayerAircraft()->af->SetFlag(AirframeClass::MPOverride);
	   else
		   SimDriver.GetPlayerAircraft()->af->ClearFlag(AirframeClass::MPOverride);
   }
}

void BreakToggle(unsigned long, int, void*)
{
	// You can set a global flag here if you want.
	// This (might) be mapped to "Ctrl-z, t"
}

void SimSilenceHorn(unsigned long, int state, void*)
{
   if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) {
		ShiAssert(SimDriver.GetPlayerAircraft()->af);
		SimDriver.GetPlayerAircraft()->af->SetFlag(AirframeClass::HornSilenced);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_SILENCE_HORN, 2);
	}
}

void SimStepHSIMode(unsigned long, int state, void*)
{
  if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) 
	{
		gNavigationSys->StepInstrumentMode();
		if (OTWDriver.GetVirtualCockpit())
		{
			if (gNavigationSys->GetInstrumentMode() == NavigationSystem::ILS_TACAN) 
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_HSI_MODE, 1);
			else
			if (gNavigationSys->GetInstrumentMode() == NavigationSystem::TACAN) 
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_HSI_MODE, 2);
			else
			if (gNavigationSys->GetInstrumentMode() == NavigationSystem::NAV) 
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_HSI_MODE, 4);
			else
			if (gNavigationSys->GetInstrumentMode() == NavigationSystem::ILS_NAV) 
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_HSI_MODE, 8);
			else
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_HSI_MODE, 1);
		}
	}
}

// MD -- 20031016: adding separate key bindings for each of the positions on the HSI Modes knob because the wrapping
// "left one" SimStepHSIMode() function really makes life hard for cockpit builders with a real knob to work that
// turns *both* ways ;-)

void SimHSIIlsTcn(unsigned long, int state, void*)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;

	if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) {
		if(g_bRealisticAvionics && g_bINS && SimDriver.GetPlayerAircraft())
		{
			SimDriver.GetPlayerAircraft()->LOCValid = TRUE;	//Flag not visible
			SimDriver.GetPlayerAircraft()->GSValid = TRUE;	//Flag not visible
		}
		gNavigationSys->SetInstrumentMode(NavigationSystem::ILS_TACAN);
		SimDriver.GetPlayerAircraft()->FCC->SetMasterMode(FireControlComputer::ILS);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_HSI_MODE, 1);
	}
}

void SimHSITcn(unsigned long, int state, void*)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;

	if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) {
		if(g_bRealisticAvionics && g_bINS && SimDriver.GetPlayerAircraft())
		{
			SimDriver.GetPlayerAircraft()->LOCValid = TRUE;	//Flag not visible
			SimDriver.GetPlayerAircraft()->GSValid = TRUE;	//Flag not visible
		}
		gNavigationSys->SetInstrumentMode(NavigationSystem::TACAN);
		SimDriver.GetPlayerAircraft()->FCC->SetMasterMode(FireControlComputer::Nav);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_HSI_MODE, 2);
	}
}

void SimHSINav(unsigned long, int state, void*)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;

	if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) {
		if(g_bRealisticAvionics && g_bINS && SimDriver.GetPlayerAircraft())
		{
			SimDriver.GetPlayerAircraft()->LOCValid = TRUE;	//Flag not visible
			SimDriver.GetPlayerAircraft()->GSValid = TRUE;	//Flag not visible
		}
		gNavigationSys->SetInstrumentMode(NavigationSystem::NAV);
		SimDriver.GetPlayerAircraft()->FCC->SetMasterMode(FireControlComputer::Nav);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_HSI_MODE, 4);
	}
}

void SimHSIIlsNav(unsigned long, int state, void*)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;

	if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) {
		if(g_bRealisticAvionics && g_bINS && SimDriver.GetPlayerAircraft())
		{
			SimDriver.GetPlayerAircraft()->LOCValid = TRUE;	//Flag not visible
			SimDriver.GetPlayerAircraft()->GSValid = TRUE;	//Flag not visible
		}
		gNavigationSys->SetInstrumentMode(NavigationSystem::ILS_NAV);
		SimDriver.GetPlayerAircraft()->FCC->SetMasterMode(FireControlComputer::ILS);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_HSI_MODE, 4);
	}
}
	
//////////////////////////////////////

// =============================================//
// Callback Function CBEOSB_1
// =============================================//
void SimCBEOSB_1L(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(0, 1, 1);
		MfdDisplay[0]->ButtonPushed(0,0);
	}
}

void SimCBEOSB_1R(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(1, 1, 1);			//Wombat778 4-12-04 changed from 0,1,1 to 1,1,1.  This must have been a bug.
	   MfdDisplay[1]->ButtonPushed(0,1);
	}
}

//Wombat778 4-12-04 Added support for the additional MFDs (T = three, F = four)

void SimCBEOSB_1T(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { 
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(2, 1, 1);
		MfdDisplay[2]->ButtonPushed(0,2);
	}
}

void SimCBEOSB_1F(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { 
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(3, 1, 1);			
	   MfdDisplay[3]->ButtonPushed(0,3);
	}
}

// =============================================//
// Callback Function CBEOSB_2
// =============================================//

void SimCBEOSB_2L(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(0, 2, 1);
		MfdDisplay[0]->ButtonPushed(1,0);
	}
}

void SimCBEOSB_2R(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(1, 2, 1);
		MfdDisplay[1]->ButtonPushed(1,1);
	}
}

//Wombat778 4-12-04 Added support for the additional MFDs (T = three, F = four)

void SimCBEOSB_2T(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { 
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(2, 2, 1);
		MfdDisplay[2]->ButtonPushed(1,2);
	}
}

void SimCBEOSB_2F(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { 
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(3, 2, 1);			
	   MfdDisplay[3]->ButtonPushed(1,3);
	}
}


// =============================================//
// Callback Function CBEOSB_3
// =============================================//

void SimCBEOSB_3L(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(0, 3, 1);
		MfdDisplay[0]->ButtonPushed(2,0);
	}
}

void SimCBEOSB_3R(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(1, 3, 1);
		MfdDisplay[1]->ButtonPushed(2,1);
	}
}

//Wombat778 4-12-04 Added support for the additional MFDs (T = three, F = four)

void SimCBEOSB_3T(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { 
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(2, 3, 1);
		MfdDisplay[2]->ButtonPushed(2,2);
	}
}

void SimCBEOSB_3F(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { 
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(3, 3, 1);			
	   MfdDisplay[3]->ButtonPushed(2,3);
	}
}

// =============================================//
// Callback Function CBEOSB_4
// =============================================//

void SimCBEOSB_4L(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(0, 4, 1);
		MfdDisplay[0]->ButtonPushed(3,0);
	}
}

void SimCBEOSB_4R(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(1, 4, 1);
		MfdDisplay[1]->ButtonPushed(3,1);
	}
}

//Wombat778 4-12-04 Added support for the additional MFDs (T = three, F = four)

void SimCBEOSB_4T(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { 
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(2, 4, 1);
		MfdDisplay[2]->ButtonPushed(3,2);
	}
}

void SimCBEOSB_4F(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { 
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(3, 4, 1);			
	   MfdDisplay[3]->ButtonPushed(3,3);
	}
}


// =============================================//
// Callback Function CBEOSB_5
// =============================================//

void SimCBEOSB_5L(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(0, 5, 1);
	   MfdDisplay[0]->ButtonPushed(4,0);
	}
}

void SimCBEOSB_5R(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(1, 5, 1);
		MfdDisplay[1]->ButtonPushed(4,1);
	}
}

//Wombat778 4-12-04 Added support for the additional MFDs (T = three, F = four)

void SimCBEOSB_5T(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) {
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(2, 5, 1);
		MfdDisplay[2]->ButtonPushed(4,2);
	}
}

void SimCBEOSB_5F(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { 
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(3, 5, 1);			
	   MfdDisplay[3]->ButtonPushed(4,3);
	}
}


// =============================================//
// Callback Function CBEOSB_6
// =============================================//

void SimCBEOSB_6L(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(0, 6, 1);
		MfdDisplay[0]->ButtonPushed(5,0);
	}
}

void SimCBEOSB_6R(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(1, 6, 1);
		MfdDisplay[1]->ButtonPushed(5,1);
	}
}

//Wombat778 4-12-04 Added support for the additional MFDs (T = three, F = four)

void SimCBEOSB_6T(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { 
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(2, 6, 1);
		MfdDisplay[2]->ButtonPushed(5,2);
	}
}

void SimCBEOSB_6F(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { 
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(3, 6, 1);			
	   MfdDisplay[3]->ButtonPushed(5,3);
	}
}


// =============================================//
// Callback Function CBEOSB_7
// =============================================//
void SimCBEOSB_7L(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(0, 7, 1);
		MfdDisplay[0]->ButtonPushed(6,0);
	}
}

void SimCBEOSB_7R(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(1, 7, 1);
		MfdDisplay[1]->ButtonPushed(6,1);
	}
}

//Wombat778 4-12-04 Added support for the additional MFDs (T = three, F = four)

void SimCBEOSB_7T(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { 
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(2, 7, 1);
		MfdDisplay[2]->ButtonPushed(6,2);
	}
}

void SimCBEOSB_7F(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { 
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(3, 7, 1);		
	   MfdDisplay[3]->ButtonPushed(6,3);
	}
}


// =============================================//
// Callback Function CBEOSB_8
// =============================================//
void SimCBEOSB_8L(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(0, 8, 1);
		MfdDisplay[0]->ButtonPushed(7,0);
	}
}

void SimCBEOSB_8R(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(1, 8, 1);
		MfdDisplay[1]->ButtonPushed(7,1);
	}
}

//Wombat778 4-12-04 Added support for the additional MFDs (T = three, F = four)

void SimCBEOSB_8T(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { 
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(2, 8, 1);
		MfdDisplay[2]->ButtonPushed(7,2);
	}
}

void SimCBEOSB_8F(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { 
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(3, 8, 1);			
	   MfdDisplay[3]->ButtonPushed(7,3);
	}
}


// =============================================//
// Callback Function CBEOSB_9
// =============================================//

void SimCBEOSB_9L(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(0, 9, 1);
		MfdDisplay[0]->ButtonPushed(8,0);
	}
}

void SimCBEOSB_9R(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(1, 9, 1);
		MfdDisplay[1]->ButtonPushed(8,1);
	}
}

//Wombat778 4-12-04 Added support for the additional MFDs (T = three, F = four)

void SimCBEOSB_9T(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) {
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(2, 9, 1);
		MfdDisplay[2]->ButtonPushed(8,2);
	}
}

void SimCBEOSB_9F(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { 
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(3, 9, 1);			
	   MfdDisplay[3]->ButtonPushed(8,3);
	}
}


// =============================================//
// Callback Function CBEOSB_10
// =============================================//
void SimCBEOSB_10L(unsigned long, int state, void*) {
	//MI
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(0, 10, 1);
		MfdDisplay[0]->ButtonPushed(9,0);
	}
}

void SimCBEOSB_10R(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(1, 10, 1);
		MfdDisplay[1]->ButtonPushed(9,1);
	}
}

//Wombat778 4-12-04 Added support for the additional MFDs (T = three, F = four)

void SimCBEOSB_10T(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { 
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(2, 10, 1);
		MfdDisplay[2]->ButtonPushed(9,2);
	}
}

void SimCBEOSB_10F(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { 
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(3, 10, 1);			
	   MfdDisplay[3]->ButtonPushed(9,3);
	}
}


// =============================================//
// Callback Function CBEOSB_11
// =============================================//

void SimCBEOSB_11L(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(0, 11, 1);
		MfdDisplay[0]->ButtonPushed(10,0);
	}
}

void SimCBEOSB_11R(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(1, 11, 1);
		MfdDisplay[1]->ButtonPushed(10,1);
	}
}

//Wombat778 4-12-04 Added support for the additional MFDs (T = three, F = four)

void SimCBEOSB_11T(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { 
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(2, 11, 1);
		MfdDisplay[2]->ButtonPushed(10,2);
	}
}

void SimCBEOSB_11F(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { 
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(3, 11, 1);			
	   MfdDisplay[3]->ButtonPushed(10,3);
	}
}

// =============================================//
// Callback Function CBEOSB_12
// =============================================//

void SimCBEOSB_12L(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(0, 12, 1);
		MfdDisplay[0]->ButtonPushed(11,0);
	}
}

void SimCBEOSB_12R(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(1, 12, 1);
		MfdDisplay[1]->ButtonPushed(11,1);
	}
}

//Wombat778 4-12-04 Added support for the additional MFDs (T = three, F = four)

void SimCBEOSB_12T(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { 
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(2, 12, 1);
		MfdDisplay[2]->ButtonPushed(11,2);
	}
}

void SimCBEOSB_12F(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { 
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(3, 12, 1);			
	   MfdDisplay[3]->ButtonPushed(11,3);
	}
}

// =============================================//
// Callback Function CBEOSB_13
// =============================================//
void SimCBEOSB_13L(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(0, 13, 1);
		MfdDisplay[0]->ButtonPushed(12,0);
	}
}

void SimCBEOSB_13R(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(1, 13, 1);
		MfdDisplay[1]->ButtonPushed(12,1);
	}
}

//Wombat778 4-12-04 Added support for the additional MFDs (T = three, F = four)

void SimCBEOSB_13T(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { 
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(2, 13, 1);
		MfdDisplay[2]->ButtonPushed(12,2);
	}
}

void SimCBEOSB_13F(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { 
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(3, 13, 1);			
	   MfdDisplay[3]->ButtonPushed(12,3);
	}
}

// =============================================//
// Callback Function CBEOSB_14
// =============================================//
void SimCBEOSB_14L(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(0, 14, 1);
		MfdDisplay[0]->ButtonPushed(13,0);
	}
}

void SimCBEOSB_14R(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(1, 14, 1);
		MfdDisplay[1]->ButtonPushed(13,1);
	}
}

//Wombat778 4-12-04 Added support for the additional MFDs (T = three, F = four)

void SimCBEOSB_14T(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { 
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(2, 14, 1);
		MfdDisplay[2]->ButtonPushed(13,2);
	}
}

void SimCBEOSB_14F(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { 
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(3, 14, 1);			
	   MfdDisplay[3]->ButtonPushed(13,3);
	}
}

// =============================================//
// Callback Function CBEOSB_15
// =============================================//
void SimCBEOSB_15L(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(0, 15, 1);
		MfdDisplay[0]->ButtonPushed(14,0);
	}
}

void SimCBEOSB_15R(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(1, 15, 1);
		MfdDisplay[1]->ButtonPushed(14,1);
	}
}

//Wombat778 4-12-04 Added support for the additional MFDs (T = three, F = four)

void SimCBEOSB_15T(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { 
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(2, 15, 1);
		MfdDisplay[2]->ButtonPushed(14,2);
	}
}

void SimCBEOSB_15F(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { 
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(3, 15, 1);			
	   MfdDisplay[3]->ButtonPushed(14,3);
	}
}


// =============================================//
// Callback Function CBEOSB_16
// =============================================//
void SimCBEOSB_16L(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(0, 16, 1);
		MfdDisplay[0]->ButtonPushed(15,0);
	}
}

void SimCBEOSB_16R(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(1, 16, 1);
		MfdDisplay[1]->ButtonPushed(15,1);
	}
}

//Wombat778 4-12-04 Added support for the additional MFDs (T = three, F = four)

void SimCBEOSB_16T(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { 
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(2, 16, 1);
		MfdDisplay[2]->ButtonPushed(15,2);
	}
}

void SimCBEOSB_16F(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { 
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(3, 16, 1);			
	   MfdDisplay[3]->ButtonPushed(15,3);
	}
}


// =============================================//
// Callback Function CBEOSB_17
// =============================================//
void SimCBEOSB_17L(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(0, 17, 1);
		MfdDisplay[0]->ButtonPushed(16,0);
	}
}

void SimCBEOSB_17R(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(1, 17, 1);
		MfdDisplay[1]->ButtonPushed(16,1);
	}
}

//Wombat778 4-12-04 Added support for the additional MFDs (T = three, F = four)

void SimCBEOSB_17T(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) {
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(2, 17, 1);
		MfdDisplay[2]->ButtonPushed(16,2);
	}
}

void SimCBEOSB_17F(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { 
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(3, 17, 1);			
	   MfdDisplay[3]->ButtonPushed(16,3);
	}
}

// =============================================//
// Callback Function CBEOSB_18
// =============================================//

void SimCBEOSB_18L(unsigned long, int state, void*) {
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(0, 18, 1);
		MfdDisplay[0]->ButtonPushed(17,0);
	}
}

void SimCBEOSB_18R(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(1, 18, 1);
		MfdDisplay[1]->ButtonPushed(17,1);
	}
}

//Wombat778 4-12-04 Added support for the additional MFDs (T = three, F = four)

void SimCBEOSB_18T(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { 
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(2, 18, 1);
		MfdDisplay[2]->ButtonPushed(17,2);
	}
}

void SimCBEOSB_18F(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) {
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(3, 18, 1);			
	   MfdDisplay[3]->ButtonPushed(17,3);
	}
}


// =============================================//
// Callback Function CBEOSB_19
// =============================================//
void SimCBEOSB_19L(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(0, 19, 1);
		MfdDisplay[0]->ButtonPushed(18,0);
	}
}

void SimCBEOSB_19R(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(1, 19, 1);
		MfdDisplay[1]->ButtonPushed(18,1);
	}
}

//Wombat778 4-12-04 Added support for the additional MFDs (T = three, F = four)

void SimCBEOSB_19T(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { 
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(2, 19, 1);
		MfdDisplay[2]->ButtonPushed(18,2);
	}
}

void SimCBEOSB_19F(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { 
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(3, 19, 1);			
	   MfdDisplay[3]->ButtonPushed(18,3);
	}
}


// =============================================//
// Callback Function CBEOSB_20
// =============================================//
void SimCBEOSB_20L(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(0, 20, 1);
		MfdDisplay[0]->ButtonPushed(19,0);
	}
}

void SimCBEOSB_20R(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(1, 20, 1);
		MfdDisplay[1]->ButtonPushed(19,1);
	}
}

//Wombat778 4-12-04 Added support for the additional MFDs (T = three, F = four)

void SimCBEOSB_20T(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { 
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(2, 20, 1);
		MfdDisplay[2]->ButtonPushed(19,2);
	}
}

void SimCBEOSB_20F(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { 
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(3, 20, 1);			
	   MfdDisplay[3]->ButtonPushed(19,3);
	}
}

void SimCBEOSB_GAINUP_R(unsigned long, int state, void*)
{
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
	{
		MfdDisplay[1]->IncreaseBrightness();
	}
}

void SimCBEOSB_GAINUP_L(unsigned long, int state, void*)
{
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
	{
		MfdDisplay[0]->IncreaseBrightness();
	}
}

//Wombat778 4-12-04

void SimCBEOSB_GAINUP_T(unsigned long, int state, void*)
{
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
	{
		MfdDisplay[2]->IncreaseBrightness();
	}
}

void SimCBEOSB_GAINUP_F(unsigned long, int state, void*)
{
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
	{
		MfdDisplay[3]->IncreaseBrightness();
	}
}


void SimCBEOSB_GAINDOWN_R(unsigned long, int state, void*)
{
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
	{
		MfdDisplay[1]->DecreaseBrightness();
	}
}

void SimCBEOSB_GAINDOWN_L(unsigned long, int state, void*)
{
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
	{
		MfdDisplay[0]->DecreaseBrightness();
	}
}

//Wombat778 4-12-04

void SimCBEOSB_GAINDOWN_T(unsigned long, int state, void*)
{
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
	{
		MfdDisplay[2]->DecreaseBrightness();
	}
}

void SimCBEOSB_GAINDOWN_F(unsigned long, int state, void*)
{
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
	{
		MfdDisplay[3]->DecreaseBrightness();
	}
}


// =============================================//
// Callback Function CBEICPTILS
// =============================================//

void SimICPTILS(unsigned long, int state, void* pButton) 
{
	if(!g_bRealisticAvionics)
	{
		//MI Original code
		if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered) {
			OTWDriver.pCockpitManager->mpIcp->HandleInput(ILS_BUTTON, (CPButtonObject*)pButton);

			// If we're in NAV mode, update the FCC/HUD mode
			if (OTWDriver.pCockpitManager->mpIcp->GetICPPrimaryMode() == NAV_MODE) {
				if (OTWDriver.pCockpitManager->mpIcp->GetICPSecondaryMode() == ILS_MODE) {
					SimDriver.GetPlayerAircraft()->FCC->SetMasterMode(FireControlComputer::ILS);
				} else {
					SimDriver.GetPlayerAircraft()->FCC->SetMasterMode(FireControlComputer::Nav);
				}
			}
		}
	}
	else
	{
		//MI modified/added for ICP stuff
	    if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered) 
		{
	 	   OTWDriver.pCockpitManager->mpIcp->HandleInput(ILS_BUTTON, (CPButtonObject*)pButton);
		}

	    if(!OTWDriver.pCockpitManager->mpIcp->IsICPSet(ICPClass::MODE_CNI))
		    return;
	    else
		{
		    // If we're in NAV mode, update the FCC/HUD mode
		    //if (OTWDriver.pCockpitManager->mpIcp->GetICPPrimaryMode() == NAV_MODE) 
			if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->FCC && SimDriver.GetPlayerAircraft()->FCC->IsNavMasterMode())
			{  
			    if (OTWDriver.pCockpitManager->mpIcp->GetICPSecondaryMode() == ILS_MODE &&
					(gNavigationSys->GetInstrumentMode() == NavigationSystem::ILS_NAV ||
					 gNavigationSys->GetInstrumentMode() == NavigationSystem::ILS_TACAN))
				{
				    SimDriver.GetPlayerAircraft()->FCC->SetMasterMode(FireControlComputer::ILS);
				} 
			    else	 
				{
				    SimDriver.GetPlayerAircraft()->FCC->SetMasterMode(FireControlComputer::Nav);
				}
			} 
		} 
	}
}

// =============================================//
// Callback Function CBEICPALOW
// =============================================//

void SimICPALOW(unsigned long, int state, void* pButton) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered) {
		OTWDriver.pCockpitManager->mpIcp->HandleInput(ALOW_BUTTON, (CPButtonObject*)pButton);
	}
}

// =============================================//
// Callback Function CBEICPFAck
// =============================================//
void SimICPFAck(unsigned long, int state, void* pButton) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered) {
		OTWDriver.pCockpitManager->mpIcp->HandleInput(FACK_BUTTON, (CPButtonObject*)pButton);
		if(g_bRealisticAvionics)
			SimDriver.GetPlayerAircraft()->mFaults->ClearAvioncFault();
	}
}


// =============================================//
// Callback Function CBEICPPrevious
// =============================================//

void SimICPPrevious(unsigned long, int state, void* pButton) 
{
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered) 
	 {
			OTWDriver.pCockpitManager->mpIcp->HandleInput(PREV_BUTTON, (CPButtonObject*)pButton);
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_ICP_NEXT, 4);
	 }
}

// =============================================//
// Callback Function CBEICPNext
// =============================================//

void SimICPNext(unsigned long, int state, void* pButton) 
{
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered) 
	 {
			OTWDriver.pCockpitManager->mpIcp->HandleInput(NEXT_BUTTON, (CPButtonObject*)pButton);
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_ICP_NEXT, 2);
	 }
}

// =============================================//
// Callback Function CBEICPLink
// =============================================//

void SimICPLink(unsigned long, int state, void* pButton) {
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (
		state & KEY_DOWN && playerAC && playerAC->IsSetFlag( MOTION_OWNSHIP ) && !playerAC->ejectTriggered
	){
		// test code
		FalconDLinkMessage *pmsg;
		pmsg = new FalconDLinkMessage (SimDriver.GetPlayerAircraft()->Id(), FalconLocalGame);
		
		pmsg->dataBlock.numPoints	= 4;
		pmsg->dataBlock.targetType	= 50;
		pmsg->dataBlock.threatType	= 90;
   
		
		pmsg->dataBlock.ptype[0]	= FalconDLinkMessage::IP;
		pmsg->dataBlock.px[0]		= 300;
		pmsg->dataBlock.py[0]		= 300;
		pmsg->dataBlock.pz[0]		= 300;
		pmsg->dataBlock.arrivalTime[0]	= SimLibElapsedTime;

		pmsg->dataBlock.ptype[1]	= FalconDLinkMessage::TGT;
		pmsg->dataBlock.px[1]		= 400;
		pmsg->dataBlock.py[1]		= 500;
		pmsg->dataBlock.pz[1]		= 600;
		pmsg->dataBlock.arrivalTime[1]		= SimLibElapsedTime;

		pmsg->dataBlock.ptype[2]	= FalconDLinkMessage::EGR;
		pmsg->dataBlock.px[2]		= 500;
		pmsg->dataBlock.py[2]		= 800;
		pmsg->dataBlock.pz[2]		= 400;
		pmsg->dataBlock.arrivalTime[2]	= SimLibElapsedTime;

		pmsg->dataBlock.ptype[3]	= FalconDLinkMessage::CP;
		pmsg->dataBlock.px[3]		= 600;
		pmsg->dataBlock.py[3]		= 700;
		pmsg->dataBlock.pz[3]		= 300;
		pmsg->dataBlock.arrivalTime[3]	= SimLibElapsedTime;
	
		FalconSendMessage (pmsg, TRUE);
		//end test code

		OTWDriver.pCockpitManager->mpIcp->HandleInput(DLINK_BUTTON, (CPButtonObject*)pButton);
	}
}

// =============================================//
// Callback Function CBEICPCrus
// =============================================//

void SimICPCrus(unsigned long, int state, void* pButton) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered) {
		OTWDriver.pCockpitManager->mpIcp->HandleInput(CRUS_BUTTON, (CPButtonObject*)pButton);
	}
}

// =============================================//
// Callback Function CBEICPStpt
// =============================================//

void SimICPStpt(unsigned long, int state, void* pButton) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered) {
		OTWDriver.pCockpitManager->mpIcp->HandleInput(STPT_BUTTON, (CPButtonObject*)pButton);
	}
}

// =============================================//
// Callback Function CBEICPMark
// =============================================//

void SimICPMark(unsigned long, int state, void* pButton) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered) {
		OTWDriver.pCockpitManager->mpIcp->HandleInput(MARK_BUTTON, (CPButtonObject*)pButton);
	}
}

// =============================================//
// Callback Function CBEICPEnter
// =============================================//

void SimICPEnter(unsigned long, int state, void* pButton) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered) {
		OTWDriver.pCockpitManager->mpIcp->HandleInput(ENTR_BUTTON, (CPButtonObject*)pButton);
	}
}

// =============================================//
// Callback Function CBEICPCom
// =============================================//

void SimICPCom1(unsigned long, int state, void* pButton) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered) {
		OTWDriver.pCockpitManager->mpIcp->HandleInput(COMM1_BUTTON, (CPButtonObject*)pButton);
	}
}


void SimICPCom2(unsigned long, int state, void* pButton) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered) {
		OTWDriver.pCockpitManager->mpIcp->HandleInput(COMM2_BUTTON, (CPButtonObject*)pButton);
	}
}

// =============================================//
// Callback Function CBEICPNav
// =============================================//

void SimICPNav(unsigned long, int state, void* pButton) {

   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered) {

		OTWDriver.pCockpitManager->mpIcp->HandleInput(NAV_BUTTON, (CPButtonObject*)pButton);
if (
	 F4IsBadReadPtr(SimDriver.GetPlayerAircraft(), sizeof(AircraftClass)) || // JB 010317 CTD
	 !SimDriver.GetPlayerAircraft()->FCC || // JB 010307 CTD
	 F4IsBadReadPtr(SimDriver.GetPlayerAircraft()->FCC, sizeof(FireControlComputer)) || // JB 010307 CTD
	 SimDriver.GetPlayerAircraft()->FCC->GetMasterMode() == FireControlComputer::MissileOverride ||
	 SimDriver.GetPlayerAircraft()->FCC->GetMasterMode() == FireControlComputer::Dogfight) return;

		// Select our FCC/HUD mode based on the NAV sub mode (ILS or not)
		if (OTWDriver.pCockpitManager->mpIcp->GetICPSecondaryMode() == ILS_MODE) {
			SimDriver.GetPlayerAircraft()->FCC->SetMasterMode(FireControlComputer::ILS);
		} else {
			SimDriver.GetPlayerAircraft()->FCC->SetMasterMode(FireControlComputer::Nav);
		}

		// Put the radar in the its default AA mode
		RadarClass* pradar = (RadarClass*) FindSensor (SimDriver.GetPlayerAircraft(), SensorClass::Radar);
		if (pradar && !F4IsBadCodePtr((FARPROC) pradar)) // JB 010220 CTD
			pradar->DefaultAAMode();
		
		if (g_bRealisticAvionics) {
		    MfdDisplay[0]->SetNewMasterMode(SimDriver.GetPlayerAircraft()->FCC->GetMainMasterMode());
		    MfdDisplay[1]->SetNewMasterMode(SimDriver.GetPlayerAircraft()->FCC->GetMainMasterMode());
		} else {
		    // Configure the MFDs
		    MfdDisplay[0]->SetNewMode(MFDClass::FCRMode);
		    MfdDisplay[1]->SetNewMode(MFDClass::FCCMode);
		}
	}
}

//MI Added for backup ICP Mode setting
void SimICPNav1(unsigned long, int state, void* pButton) 
{
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered) 
	{

		OTWDriver.pCockpitManager->mpIcp->HandleInput(NAV_BUTTON, (CPButtonObject*)pButton);

		// Select our FCC/HUD mode based on the NAV sub mode (ILS or not)
		if (OTWDriver.pCockpitManager->mpIcp->GetICPSecondaryMode() == ILS_MODE) 
		{
			SimDriver.GetPlayerAircraft()->FCC->SetMasterMode(FireControlComputer::ILS);
		} 
		else 
		{
			SimDriver.GetPlayerAircraft()->FCC->SetMasterMode(FireControlComputer::Nav);
		}

		// Put the radar in the its default AA mode
		RadarClass* pradar = (RadarClass*) FindSensor (SimDriver.GetPlayerAircraft(), SensorClass::Radar);
		if (pradar)
			pradar->DefaultAAMode();

		// Configure the MFDs
		MfdDisplay[0]->SetNewMasterMode(SimDriver.GetPlayerAircraft()->FCC->GetMainMasterMode());
		MfdDisplay[1]->SetNewMasterMode(SimDriver.GetPlayerAircraft()->FCC->GetMainMasterMode());
	}
}

// =============================================//
// Callback Function CBEICPAA
// =============================================//

void SimICPAA(unsigned long, int state, void* pButton){
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (!playerAC || !playerAC->IsSetFlag( MOTION_OWNSHIP )){
		// 2002-02-15 ADDED BY S.G. Do it once here and removed the corresponding line from below
		return;
	}

	if(!g_bRealisticAvionics)
	{
		//MI Original code
		if (state & KEY_DOWN && !playerAC->ejectTriggered) {

			OTWDriver.pCockpitManager->mpIcp->HandleInput(AA_BUTTON, (CPButtonObject*)pButton);
			playerAC->FCC->SetMasterMode(FireControlComputer::AAGun);

			// Put the radar in the its default AA mode
			RadarClass* pradar = (RadarClass*) FindSensor(playerAC, SensorClass::Radar);
			// sfr: @todo remove JB check
			if (pradar && !F4IsBadReadPtr(pradar, sizeof(RadarClass))){
				// JB 010404 CTD
				pradar->DefaultAAMode();
			}

			// Configure the MFDs
			MfdDisplay[0]->SetNewMode(MFDClass::FCRMode);
			MfdDisplay[1]->SetNewMode(MFDClass::SMSMode);
		}
	}
	else
	{
		//MI 3/1/2002 if we're in DF or MRM, don't do anything
		if (
			playerAC->FCC && 
			(
				playerAC->FCC->GetMasterMode() == FireControlComputer::Dogfight || 
				playerAC->FCC->GetMasterMode() == FireControlComputer::MissileOverride
			)
		){
			return;
		}

		//Set our display mode
		// Marco edit - SimDriver.GetPlayerAircraft() was often 00000000 in dogfight - CTD S.G. Caugth above now
		if (playerAC->Sms){
			if (playerAC->Sms->drawable){
				//me123 ctd in dogfight games on entry...this was 00000000
				playerAC->Sms->drawable->SetDisplayMode(SmsDrawable::Wpn);
			}
		}

	   //MI added/modified for ICP Stuff
	   if (state & KEY_DOWN && !playerAC->ejectTriggered){ 
		   //Player hit the button. Is our MODE_A_A Flag set?
		   //Since we start in NAV Mode by default, we want to get into
		   //A_A mode the first time we push the button
		   if(OTWDriver.pCockpitManager->mpIcp->IsICPSet(ICPClass::MODE_A_A))
		   {
			   //Player pushed this button previously, so let's go back
			   //to NAV mode
			   //SimDriver.GetPlayerAircraft()->FCC->SetMasterMode(FireControlComputer::Nav);
			   playerAC->FCC->SetMasterMode( playerAC->FCC->GetLastNavMasterMode() ); //ASSOCIATOR
			   
			   //passes our puttonstate to the ICP
			   OTWDriver.pCockpitManager->mpIcp->HandleInput(NAV_BUTTON, (CPButtonObject*)pButton);
			   
			   // Put the radar in the its default AA mode
			   RadarClass* pradar = (RadarClass*) FindSensor (playerAC, SensorClass::Radar);
			   if (pradar){
					pradar->DefaultAAMode();			   
			   }

			   // Configure the MFDs
			   MfdDisplay[0]->SetNewMasterMode(playerAC->FCC->GetMainMasterMode());
			   MfdDisplay[1]->SetNewMasterMode(playerAC->FCC->GetMainMasterMode());

			   //Clear the flag indicating we are in A_A mode
			   //This is so we know that we have to go into A_A mode next time
			   OTWDriver.pCockpitManager->mpIcp->ClearICPFlag(ICPClass::MODE_A_A);

			   //If our A_G mode flag is set, clear it so we get into
			   //A_G mode when we push the A_G button
			   if(OTWDriver.pCockpitManager->mpIcp->IsICPSet(ICPClass::MODE_A_G)){
				   OTWDriver.pCockpitManager->mpIcp->ClearICPFlag(ICPClass::MODE_A_G);
			   }
		   }
		   //no, we press this button the first time
		   else {
				playerAC->FCC->EnterAAMasterMode();
								
			   //passes our puttonstate to the ICP
			   OTWDriver.pCockpitManager->mpIcp->HandleInput(AA_BUTTON, (CPButtonObject*)pButton);
			   
			   // Put the radar in the its default AA mode
			   RadarClass* pradar = (RadarClass*) FindSensor (playerAC, SensorClass::Radar);
			   if (pradar){
					pradar->DefaultAAMode();
			   }

			   // Configure the MFDs
			   MfdDisplay[0]->SetNewMasterMode(playerAC->FCC->GetMainMasterMode());
			   MfdDisplay[1]->SetNewMasterMode(playerAC->FCC->GetMainMasterMode());
			   
			   //Set the flag indicating that we pushed the button
			   //so we go into NAV mode the next time we push it
			   OTWDriver.pCockpitManager->mpIcp->SetICPFlag(ICPClass::MODE_A_A);

			   //If our A_G NAV mode flag is set, clear it so we get into
			   //A_G mode when we push that button
			   if(OTWDriver.pCockpitManager->mpIcp->IsICPSet(ICPClass::MODE_A_G)){
				   OTWDriver.pCockpitManager->mpIcp->ClearICPFlag(ICPClass::MODE_A_G);
			   }
		   }
	   } 	
	}
}

//MI added for backup ICP Mode setting
void SimICPAA1(unsigned long, int state, void* pButton) 
{
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered) 
	{
		//MI 3/1/2002 if we're in DF or MRM, don't do anything
		if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->FCC && (SimDriver.GetPlayerAircraft()->FCC->GetMasterMode() == 
			FireControlComputer::Dogfight || SimDriver.GetPlayerAircraft()->FCC->GetMasterMode() == FireControlComputer::MissileOverride))
			return;

		OTWDriver.pCockpitManager->mpIcp->HandleInput(AA_BUTTON, (CPButtonObject*)pButton);
		SimDriver.GetPlayerAircraft()->FCC->SetMasterMode(FireControlComputer::AAGun);

		// Put the radar in the its default AA mode
		RadarClass* pradar = (RadarClass*) FindSensor (SimDriver.GetPlayerAircraft(), SensorClass::Radar);
		if (pradar)
			pradar->DefaultAAMode();

		// Configure the MFDs
		MfdDisplay[0]->SetNewMasterMode(SimDriver.GetPlayerAircraft()->FCC->GetMainMasterMode());
		MfdDisplay[1]->SetNewMasterMode(SimDriver.GetPlayerAircraft()->FCC->GetMainMasterMode());
	}
}

// =============================================//
// Callback Function CBEICPAG
// =============================================//

void SimICPAG(unsigned long, int state, void* pButton) 
{
	if (!SimDriver.GetPlayerAircraft() || !SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP )) // 2002-02-15 ADDED BY S.G. Do it once here and removed the corresponding line from below
		return;

	if(!g_bRealisticAvionics)
	{
		//MI Original Code
	   if (state & KEY_DOWN && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered) {

			OTWDriver.pCockpitManager->mpIcp->HandleInput(AG_BUTTON, (CPButtonObject*)pButton);
			SimDriver.GetPlayerAircraft()->FCC->SetMasterMode(FireControlComputer::AirGroundBomb);

			// Put the radar in the its default AG mode
			RadarClass* pradar = (RadarClass*) FindSensor (SimDriver.GetPlayerAircraft(), SensorClass::Radar);
			if (pradar)
				pradar->DefaultAGMode();

			// Configure the MFDs
			MfdDisplay[0]->SetNewMode(MFDClass::FCRMode);
			MfdDisplay[1]->SetNewMode(MFDClass::SMSMode);
		}
	}
	else
	{
		//MI 3/1/2002 if we're in DF or MRM, don't do anything
		if(SimDriver.GetPlayerAircraft()->FCC && (SimDriver.GetPlayerAircraft()->FCC->GetMasterMode() == 
			FireControlComputer::Dogfight || SimDriver.GetPlayerAircraft()->FCC->GetMasterMode() == FireControlComputer::MissileOverride))
			return;

		if (SimDriver.GetPlayerAircraft()->Sms)	//JPO CTDfix	//Set our display mode
			SimDriver.GetPlayerAircraft()->Sms->drawable->SetDisplayMode(SmsDrawable::Wpn);
		//MI added/modified for ICP Stuff
		if (state & KEY_DOWN && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered) 
		{
		   //Have we pushed this button before?
		   if(OTWDriver.pCockpitManager->mpIcp->IsICPSet(ICPClass::MODE_A_G))
		   {
			   //Yes, we pushed it before. Let's go back into NAV mode
			   //Set our FCC into NAV mode
			   //SimDriver.GetPlayerAircraft()->FCC->SetMasterMode(FireControlComputer::Nav);
			   SimDriver.GetPlayerAircraft()->FCC->SetMasterMode( SimDriver.GetPlayerAircraft()->FCC->GetLastNavMasterMode() ); //ASSOCIATOR

			   //passes our puttonstate to the ICP
			   OTWDriver.pCockpitManager->mpIcp->HandleInput(NAV_BUTTON, (CPButtonObject*)pButton);
			   
			   // Put the radar in the its default AA mode
			   RadarClass* pradar = (RadarClass*) FindSensor (SimDriver.GetPlayerAircraft(), SensorClass::Radar);
			   if (pradar)
					pradar->DefaultAGMode();

			   // Configure the MFDs
			   		// Configure the MFDs
			   MfdDisplay[0]->SetNewMasterMode(SimDriver.GetPlayerAircraft()->FCC->GetMainMasterMode());
			   MfdDisplay[1]->SetNewMasterMode(SimDriver.GetPlayerAircraft()->FCC->GetMainMasterMode());


			   //Clear the flag indicating we are in A_G mode
			   //This is so we know that we have to go into A_G mode next time
			   OTWDriver.pCockpitManager->mpIcp->ClearICPFlag(ICPClass::MODE_A_G);

			   //If our A_A NAV mode flag is set, clear it so we get into
			   //A_A mode when we push that button
			   if(OTWDriver.pCockpitManager->mpIcp->IsICPSet(ICPClass::MODE_A_A))
				   OTWDriver.pCockpitManager->mpIcp->ClearICPFlag(ICPClass::MODE_A_A);

		   }
		   else if (SimDriver.GetPlayerAircraft()->Sms)
		   {
			   SimDriver.GetPlayerAircraft()->FCC->EnterAGMasterMode();
			   //passes our puttonstate to the ICP
			   OTWDriver.pCockpitManager->mpIcp->HandleInput(AG_BUTTON, (CPButtonObject*)pButton);

			   // Put the radar in the its default AG mode
			   //Makes us go into NAV mode next time we push it.
			   RadarClass* pradar = (RadarClass*) FindSensor (SimDriver.GetPlayerAircraft(), SensorClass::Radar);
			   if (pradar)
					pradar->DefaultAGMode();

			   // Configure the MFDs
			   MfdDisplay[0]->SetNewMasterMode(SimDriver.GetPlayerAircraft()->FCC->GetMainMasterMode());
			   MfdDisplay[1]->SetNewMasterMode(SimDriver.GetPlayerAircraft()->FCC->GetMainMasterMode());

			   //Set the Flag that we've been here before
			   OTWDriver.pCockpitManager->mpIcp->SetICPFlag(ICPClass::MODE_A_G);

			   //If our A_A mode flag is set, clear it so we get into
			   //A_A mode when we push the A_A button
			   if(OTWDriver.pCockpitManager->mpIcp->IsICPSet(ICPClass::MODE_A_A))
				   OTWDriver.pCockpitManager->mpIcp->ClearICPFlag(ICPClass::MODE_A_A);
		   }
	   }
	}
}

//MI added for backup ICP Mode setting
void SimICPAG1(unsigned long, int state, void* pButton) 
{
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered && SimDriver.GetPlayerAircraft()->Sms) 
	{
		//MI 3/1/2002 if we're in DF or MRM, don't do anything
		if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->FCC && (SimDriver.GetPlayerAircraft()->FCC->GetMasterMode() == 
			FireControlComputer::Dogfight || SimDriver.GetPlayerAircraft()->FCC->GetMasterMode() == FireControlComputer::MissileOverride))
			return;

		OTWDriver.pCockpitManager->mpIcp->HandleInput(AG_BUTTON, (CPButtonObject*)pButton);
		SimDriver.GetPlayerAircraft()->FCC->SetMasterMode(FireControlComputer::AirGroundBomb);
		//SimDriver.GetPlayerAircraft()->Sms->GetNextWeapon(wdGround); // MLR 2/8/2004 - 
		SimDriver.GetPlayerAircraft()->Sms->StepAGWeapon();

		// Put the radar in the its default AG mode
		RadarClass* pradar = (RadarClass*) FindSensor (SimDriver.GetPlayerAircraft(), SensorClass::Radar);
		if (pradar)
			pradar->DefaultAGMode();

		// Configure the MFDs
		MfdDisplay[0]->SetNewMasterMode(SimDriver.GetPlayerAircraft()->FCC->GetMainMasterMode());
		MfdDisplay[1]->SetNewMasterMode(SimDriver.GetPlayerAircraft()->FCC->GetMainMasterMode());
	}
}
// =============================================//
// Callback Function SimHUDScales
// =============================================//

void SimHUDScales(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		TheHud->CycleScalesSwitch();
	}
}

// =============================================//
// Callback Function SimHUDFPM
// =============================================//

void SimHUDFPM(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		TheHud->CycleFPMSwitch();
	}
}

// =============================================//
// Callback Function SimHUDDED
// =============================================//

void SimHUDDED(unsigned long, int state, void*) 
{
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP))  // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
	{
		TheHud->CycleDEDSwitch();
		if (OTWDriver.GetVirtualCockpit())
		{
			if (TheHud->GetDEDSwitch() == HudClass::PFL_DATA)
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_HUD_DED_PFL, 2);
			else
			if (TheHud->GetDEDSwitch() == HudClass::DED_DATA)
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_HUD_DED_PFL, 4);
			else
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_HUD_DED_PFL, 1);
		}
	}
}

// MD -- 20031026: adding commands to position HUD control DED switch directly which
// should help cockpit builders map this function to a physical switch better

void SimHUDDEDOff(unsigned long, int state, void*)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP))
	{
	  TheHud->SetDEDSwitch(HudClass::DED_OFF);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_HUD_DED_PFL, 1);
	}
}

void SimHUDDEDPFL(unsigned long, int state, void*)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP))
	{
	  TheHud->SetDEDSwitch(HudClass::PFL_DATA);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_HUD_DED_PFL, 2);
	}
}

void SimHUDDEDDED(unsigned long, int state, void*)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP))
	{ 
	  TheHud->SetDEDSwitch(HudClass::DED_DATA);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_HUD_DED_PFL, 4);
	}
}

// =============================================//
// Callback Function 
// =============================================//

void SimHUDVelocity(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP))  // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
	 {
			TheHud->CycleVelocitySwitch();
			if (OTWDriver.GetVirtualCockpit())
			{
				if (TheHud->GetVelocitySwitch() == HudClass::CAS)
					OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_HUD_VELOCITY, 4);
				else
				if (TheHud->GetVelocitySwitch() == HudClass::TAS)
					OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_HUD_VELOCITY, 2);
				else
					OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_HUD_VELOCITY, 1);
			}
	 }
}

// MD -- 20031026: adding commands to position HUD control velocity switch directly which
// should help cockpit builders map this function to a physical switch better

void SimHUDVelocityCAS(unsigned long, int state, void*)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP))  // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
	{
		TheHud->SetVelocitySwitch(HudClass::CAS);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_HUD_VELOCITY, 4);
	}
}

void SimHUDVelocityTAS(unsigned long, int state, void*)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP))  // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
	{
		TheHud->SetVelocitySwitch(HudClass::TAS);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_HUD_VELOCITY, 2);
	}
}

void SimHUDVelocityGND(unsigned long, int state, void*)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP))  // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
	{
		TheHud->SetVelocitySwitch(HudClass::GND_SPD);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_HUD_VELOCITY, 1);
	}
}

// =============================================//
// Callback Function SimHUDRadar
// =============================================//

void SimHUDRadar(unsigned long, int state, void*) {
  if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP))  // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
	{
		TheHud->CycleRadarSwitch();
		if (OTWDriver.GetVirtualCockpit())
		{
			if (TheHud->GetRadarSwitch() == HudClass::ALT_RADAR)
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_HUD_RAL_BARO, 4);
			else
			if (TheHud->GetRadarSwitch() == HudClass::BARO)
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_HUD_RAL_BARO, 2);
			else
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_HUD_RAL_BARO, 1);
		}
	}
}

// MD -- 20031026: adding commands to position HUD control velocity switch directly which
// should help cockpit builders map this function to a physical switch better

void SimHUDAltRadar(unsigned long, int state, void*)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP))  // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
	{
		TheHud->SetRadarSwitch(HudClass::ALT_RADAR);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_HUD_RAL_BARO, 4);
	}
}

void SimHUDAltBaro(unsigned long, int state, void*)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP))  // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
	{
		TheHud->SetRadarSwitch(HudClass::BARO);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_HUD_RAL_BARO, 2);
	}
}

void SimHUDAltAuto(unsigned long, int state, void*)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP))  // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
	{
		TheHud->SetRadarSwitch(HudClass::RADAR_AUTO);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_HUD_RAL_BARO, 1);
	}
}

// =============================================//
// Callback Function SimHUDBrightness
// =============================================//

void SimHUDBrightness(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
	 {
			TheHud->CycleBrightnessSwitch();
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_HUD_DAY_NITE, 1<<(TheHud->GetBrightnessSwitch()-1));
	 }
}

// MD -- 20031026: adding commands to position HUD brightness control switch directly which
// should help cockpit builders map this function to a physical switch better

void SimHUDBrtDay(unsigned long, int state, void*)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
	{
		TheHud->SetBrightnessSwitch(HudClass::DAY);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_HUD_DAY_NITE, 2);
	}
}

void SimHUDBrtAuto(unsigned long, int state, void*)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
	{
		TheHud->SetBrightnessSwitch(HudClass::BRIGHT_AUTO);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_HUD_DAY_NITE, 1);
	}
}

void SimHUDBrtNight(unsigned long, int state, void*)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
	if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
	{
		TheHud->SetBrightnessSwitch(HudClass::NIGHT);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_HUD_DAY_NITE, 4);
	}
}

//MI
void SimHUDBrightnessUp(unsigned long, int state, void*) 
{
	if(!g_bRealisticAvionics)
		return;
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
	{
		TheHud->CycleBrightnessSwitchUp();
		if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_HUD_DAY_NITE, 1<<(TheHud->GetBrightnessSwitch()-1));
	}
}
void SimHUDBrightnessDown(unsigned long, int state, void*) 
{
	if(!g_bRealisticAvionics)
		return;
   if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
	{
		TheHud->CycleBrightnessSwitchDown();
		if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_HUD_DAY_NITE, 1<<(TheHud->GetBrightnessSwitch()-1));
	}
}

// =============================================//
// Callback Function ExtinguishMasterCaution
// =============================================//

void ExtinguishMasterCaution(unsigned long, int state, void*) {
	
	if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		if(SimDriver.GetPlayerAircraft()->mFaults->MasterCaution() == TRUE) {
			SimDriver.GetPlayerAircraft()->mFaults->ClearMasterCaution();
		}
		else {
			OTWDriver.pCockpitManager->mMiscStates.SetMasterCautionEvent();
		}
	}

}

void SimCycleLeftAuxComDigit(unsigned long, int state, void*) {
	if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) 
	{
		gNavigationSys->StepTacanChannelDigit(NavigationSystem::AUXCOMM, 2);
		int val = gNavigationSys->GetTacanChannel(NavigationSystem::AUXCOMM, 2);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetDOFangle( COMP_3DPIT_TACAN_LEFT, val * 0.6283F); // radians per digit
	}
}

void SimDecLeftAuxComDigit(unsigned long, int state, void*) {
	if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) 
	{
		gNavigationSys->StepTacanChannelDigit(NavigationSystem::AUXCOMM, 2, -1);
		int val = gNavigationSys->GetTacanChannel(NavigationSystem::AUXCOMM, 2);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetDOFangle( COMP_3DPIT_TACAN_LEFT, val * 0.6283F);
	}
}


void SimCycleCenterAuxComDigit(unsigned long, int state, void*) {
	if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) 
	{
		gNavigationSys->StepTacanChannelDigit(NavigationSystem::AUXCOMM, 1);
		int val = gNavigationSys->GetTacanChannel(NavigationSystem::AUXCOMM, 1);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetDOFangle( COMP_3DPIT_TACAN_CENTER, val * 0.6283F);
	}
}

void SimDecCenterAuxComDigit(unsigned long, int state, void*) {
	if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) 
	{
		gNavigationSys->StepTacanChannelDigit(NavigationSystem::AUXCOMM, 1, -1);
		int val = gNavigationSys->GetTacanChannel(NavigationSystem::AUXCOMM, 1);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetDOFangle( COMP_3DPIT_TACAN_CENTER, val * 0.6283F);
	}
}

void SimCycleRightAuxComDigit(unsigned long, int state, void*) {
	if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) 
	{
		gNavigationSys->StepTacanChannelDigit(NavigationSystem::AUXCOMM, 0);
		int val = gNavigationSys->GetTacanChannel(NavigationSystem::AUXCOMM, 0);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetDOFangle( COMP_3DPIT_TACAN_RIGHT, val * 0.6283F);
	}
}
void SimDecRightAuxComDigit(unsigned long, int state, void*) {
	if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) 
	{
		gNavigationSys->StepTacanChannelDigit(NavigationSystem::AUXCOMM, 0, -1);
		int val = gNavigationSys->GetTacanChannel(NavigationSystem::AUXCOMM, 0);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetDOFangle( COMP_3DPIT_TACAN_RIGHT, val * 0.6283F);
	}
}
void SimCycleBandAuxComDigit(unsigned long, int state, void*) {
	if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) 
	{
		gNavigationSys->StepTacanBand(NavigationSystem::AUXCOMM);
		if (OTWDriver.GetVirtualCockpit())
			if(gNavigationSys->GetTacanBand(NavigationSystem::AUXCOMM) == TacanList::X)
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_TACAN_BAND, 1);
			else
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_TACAN_BAND, 2);
	}
}

void SimToggleAuxComMaster(unsigned long, int state, void*) {
	if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) 
	{
		gNavigationSys->ToggleControlSrc();
		gNavigationSys->ToggleUHFSrc();  // MD -- 20031121: added here to match reality
		if (OTWDriver.GetVirtualCockpit())
			if(gNavigationSys->GetControlSrc() == NavigationSystem::AUXCOMM)
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_AUX_COMM_SRC, 1);
			else
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_AUX_COMM_SRC, 2);
	}
}

// MD -- 20031120: adding explicit commands to place the backup/UFC control knob explicitly.

void SimAuxComBackup(unsigned long, int state, void*) {
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
	if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) 
	{
		gNavigationSys->SetControlSrc(NavigationSystem::AUXCOMM);
		gNavigationSys->SetUHFSrc(NavigationSystem::UHF_Mode_Type::UHF_BACKUP);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_AUX_COMM_SRC, 1);
	}
}

void SimAuxComUFC(unsigned long, int state, void*) {
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
	if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) 
	{
		gNavigationSys->SetControlSrc(NavigationSystem::ICP);
		gNavigationSys->SetUHFSrc(NavigationSystem::UHF_Mode_Type::UHF_NORM);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_AUX_COMM_SRC, 2);
	}
}

void SimToggleAuxComAATR(unsigned long, int state, void*) {
	if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) 
	{
		gNavigationSys->ToggleDomain(NavigationSystem::AUXCOMM);
		int val = 1<<gNavigationSys->GetDomain(NavigationSystem::AUXCOMM);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_AUX_COMM_MSTR, val);
	}
}

// MD -- 20031120: adding explicit commands to set TACAN domain switch position.
// NB: game code currently only models two positions of this three place switch.

void SimTACANTR(unsigned long, int state, void*) {
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
	if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		gNavigationSys->SetDomain(NavigationSystem::AUXCOMM, TacanList::AG);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_AUX_COMM_MSTR, 1);
	}
}

void SimTACANAATR(unsigned long, int state, void*) {
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
	if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) 
	{
		gNavigationSys->SetDomain(NavigationSystem::AUXCOMM, TacanList::AA);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_AUX_COMM_MSTR, 2);
	}
}

// MD -- 20031121: This corresponded to the UHF function control knob on the UHF control
// panel.  That knob does not control whether the channel controls of the UHF panel are 
// active or not; the CNI select knob on the AUX COMM panel does that.  Moved the UHF source
// selection controls to the CNI knob functions.  TO DO (one day maybe): add implementation
// of what this control really does! ;)

void SimToggleUHFMaster(unsigned long, int state, void*) {
//	if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) {
//		gNavigationSys->ToggleUHFSrc();
//	}
}

#include <dvoice.h>
extern IDirectPlayVoiceClient* g_pVoiceClient;
void SimTransmitCom1(unsigned long val, int state, void *)
{
	    HRESULT             hr = S_OK;
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
	Transmit(1);
	}
	else
	{
	Transmit(0);
	}
}
void SimTransmitCom2(unsigned long val, int state, void *)
{
	    HRESULT             hr = S_OK;
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
	Transmit(2);
	}
	else
	{
	Transmit(0);
	}
}

// =============================================//

void IncreaseAlow(unsigned long, int state , void*) {
	//MI
	if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		if(TheHud && TheHud->lowAltWarning >= 1000.0F) {
			TheHud->lowAltWarning += 1000.0F;
			TheHud->lowAltWarning = min(max(0.0F, TheHud->lowAltWarning), 99999.0F);
		}
		else if(TheHud){
			TheHud->lowAltWarning += 100.0F;
			TheHud->lowAltWarning = min(max(0.0F, TheHud->lowAltWarning), 99999.0F);
		}
	}
}


void DecreaseAlow(unsigned long, int state, void*) {
	//MI
	if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		if(TheHud && TheHud->lowAltWarning > 1000.0F) {
			TheHud->lowAltWarning -= 1000.0F;
			TheHud->lowAltWarning = min(max(0.0F, TheHud->lowAltWarning), 99999.0F);
		}
		else if(TheHud){
			TheHud->lowAltWarning -= 100.0F;
			TheHud->lowAltWarning = min(max(0.0F, TheHud->lowAltWarning), 99999.0F);
		}
	}
}

void SaveCockpitDefaults(unsigned long, int state, void*)
{
   if (OTWDriver.pCockpitManager && (state & KEY_DOWN))
   {
      OTWDriver.pCockpitManager->SaveCockpitDefaults();
   }
}

void LoadCockpitDefaults(unsigned long, int state, void*)
{
   if (OTWDriver.pCockpitManager && (state & KEY_DOWN))
   {
      OTWDriver.pCockpitManager->LoadCockpitDefaults();
   }
}

// JB 000509
void SimSetBubbleSize (unsigned long val, int state, void*)
{
   if (state & KEY_DOWN && !(gCommsMgr && gCommsMgr->Online()))
      FalconLocalSession->SetBubbleRatio(.5f + float(val - DIK_1) / 2.0f);
}
// JB 000509


// JPO - allow throttle to go past idle detent setting
void SimThrottleIdleDetent(unsigned long, int state, void*)
{
	if (g_bUseAnalogIdleCutoff)
		return;

  if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_IDLE_DETENT, 2);
		//TJL 01/18/04 Multi-engine
		if (SimDriver.GetPlayerAircraft()->af->GetNumberEngines() == 2)
		{
			switch (UserStickInputs.getCurrentEngine())
			{
				//case PilotInputs::Left_Engine:
				case UserStickInputs.Left_Engine:
					if (SimDriver.GetPlayerAircraft()->af->engine1Throttle < 0.1f)
					{
						SimDriver.GetPlayerAircraft()->af->SetFlag(AirframeClass::EngineStopped);
						SimDriver.GetPlayerAircraft()->af->SetFlag(AirframeClass::EngineOff);
					}
					else if (SimDriver.GetPlayerAircraft()->af->rpm >= 0.20f)
					{
						// engine light 
						SimDriver.GetPlayerAircraft()->af->ClearFlag(AirframeClass::EngineStopped);
						SimDriver.GetPlayerAircraft()->af->ClearFlag(AirframeClass::EngineOff);
						SimDriver.GetPlayerAircraft()->mFaults->ClearFault(FaultClass::eng_fault, FaultClass::fl_out);
					}
					break;

				case UserStickInputs.Right_Engine:
					if (SimDriver.GetPlayerAircraft()->af->engine2Throttle < 0.1f)
					{
						SimDriver.GetPlayerAircraft()->af->SetEngineFlag(AirframeClass::EngineStopped2);
						SimDriver.GetPlayerAircraft()->af->SetFlag(AirframeClass::EngineOff2);
					}
					else if (SimDriver.GetPlayerAircraft()->af->rpm2 >= 0.20f)
					{
						// engine light 
						SimDriver.GetPlayerAircraft()->af->ClearEngineFlag(AirframeClass::EngineStopped2);
						SimDriver.GetPlayerAircraft()->af->ClearFlag(AirframeClass::EngineOff);
						SimDriver.GetPlayerAircraft()->mFaults->ClearFault(FaultClass::eng2_fault, FaultClass::fl_out);
					}
					break;

				case UserStickInputs.Both_Engines:
					if (SimDriver.GetPlayerAircraft()->af->engine1Throttle < 0.1f && 
						SimDriver.GetPlayerAircraft()->af->engine2Throttle < 0.1f)
					{
						SimDriver.GetPlayerAircraft()->af->SetFlag(AirframeClass::EngineStopped);
						SimDriver.GetPlayerAircraft()->af->SetEngineFlag(AirframeClass::EngineStopped2);
						SimDriver.GetPlayerAircraft()->af->SetFlag(AirframeClass::EngineOff);
						SimDriver.GetPlayerAircraft()->af->SetFlag(AirframeClass::EngineOff2);
					}
					else if (SimDriver.GetPlayerAircraft()->af->rpm >= 0.20f && 
							SimDriver.GetPlayerAircraft()->af->rpm >= 0.20f)
					{
						// engine light2
						SimDriver.GetPlayerAircraft()->af->ClearFlag(AirframeClass::EngineStopped);
						SimDriver.GetPlayerAircraft()->af->ClearFlag(AirframeClass::EngineOff);
						SimDriver.GetPlayerAircraft()->mFaults->ClearFault(FaultClass::eng_fault, FaultClass::fl_out);
						SimDriver.GetPlayerAircraft()->af->ClearEngineFlag(AirframeClass::EngineStopped2);
						SimDriver.GetPlayerAircraft()->af->ClearFlag(AirframeClass::EngineOff2);
						SimDriver.GetPlayerAircraft()->mFaults->ClearFault(FaultClass::eng2_fault, FaultClass::fl_out);
					}
					break;

				default:
					break;
			}
		}
		else
		{
		// Orig code 
		// throttle to off?
			if (SimDriver.GetPlayerAircraft()->af->Throtl() < 0.1f)
			{
				SimDriver.GetPlayerAircraft()->af->SetFlag(AirframeClass::EngineStopped);
			}
			else if (SimDriver.GetPlayerAircraft()->af->rpm >= 0.20f)
			{
				// engine light 
				SimDriver.GetPlayerAircraft()->af->ClearFlag(AirframeClass::EngineStopped);
				SimDriver.GetPlayerAircraft()->mFaults->ClearFault(FaultClass::eng_fault, FaultClass::fl_out);
			}
		}
  }
}

// set Jfs to position
void SimJfsStart(unsigned long, int state, void*)
{
  if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) 
	{
		// if jfs allready running, clear it.
		if (SimDriver.GetPlayerAircraft()->af->IsSet(AirframeClass::JfsStart)) 
		{
			SimDriver.GetPlayerAircraft()->af->ClearFlag(AirframeClass::JfsStart);
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_JSF_START, 1);
		}
		// otherwise - if throttle is at idle, and accumulators charged, attempt JFS start
		else if (SimDriver.GetPlayerAircraft()->af->Throtl() < 0.1f) 
		{
			SimDriver.GetPlayerAircraft()->af->JfsEngineStart ();
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_JSF_START, 2);
		}
  }
}

// step the EPU setting
void SimEpuToggle (unsigned long, int state, void*)
{
  if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) 
	{
		// toggle EPU off/auto/on.
		SimDriver.GetPlayerAircraft()->af->StepEpuSwitch();
		int val = SimDriver.GetPlayerAircraft()->af->GetEpuSwitch()+1;
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_EPU, val);
  }
}

// MD -- 20031024: Adding commands to set the EPU Switch position explicitly to
// make it easier for cockpit builders to map the commands to a physical switch.

void SimEpuOff(unsigned long, int state, void*)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
  if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) 
	{
		SimDriver.GetPlayerAircraft()->af->SetEpuSwitch(AirframeClass::OFF);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_EPU, 1);
  }
}

void SimEpuAuto(unsigned long, int state, void*)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
  if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) 
	{
		SimDriver.GetPlayerAircraft()->af->SetEpuSwitch(AirframeClass::AUTO);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_EPU, 2);
  }
}

void SimEpuOn(unsigned long, int state, void*)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
  if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) 
	{
		SimDriver.GetPlayerAircraft()->af->SetEpuSwitch(AirframeClass::ON);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_EPU, 4);
  }
}

// JPO: TRIM command
void AFRudderTrimLeft (unsigned long, int state, void*)
{
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 ADDED BY S.G. Moved outside the two if statement
		//MI additions
		if(g_bRealisticAvionics)
		{
			if(SimDriver.GetPlayerAircraft()->TrimAPDisc)
				return;
		}

		if (state & KEY_DOWN)
			pitchRudderTrimRate = -0.25F;
		else
			pitchRudderTrimRate = 0.0F;
	}
}

void AFRudderTrimRight (unsigned long, int state, void*)
{
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 ADDED BY S.G. Moved outside the two if statement
	//MI additions
		if(g_bRealisticAvionics)
		{
			if(SimDriver.GetPlayerAircraft()->TrimAPDisc)
				return;
		}

		if (state & KEY_DOWN)
			pitchRudderTrimRate = 0.25F;
		else
			pitchRudderTrimRate = 0.0F;
	}
}

void AFAileronTrimLeft (unsigned long, int state, void*)
{
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 ADDED BY S.G. Moved outside the two if statement
	//MI additions
		if(g_bRealisticAvionics)
		{
			if(SimDriver.GetPlayerAircraft()->TrimAPDisc)
				return;
		}

		if (state & KEY_DOWN)
			pitchAileronTrimRate = -0.25F;
		else
			pitchAileronTrimRate = 0.0F;
	}
}

void AFAileronTrimRight (unsigned long, int state, void*)
{
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 ADDED BY S.G. Moved outside the two if statement
		//MI additions
		if(g_bRealisticAvionics)
		{
			if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->TrimAPDisc)
				return;
		}

		if (state & KEY_DOWN)
			pitchAileronTrimRate = 0.25F;
		else
			pitchAileronTrimRate = 0.0F;
	}
}

void AFElevatorTrimUp (unsigned long, int state, void*)
{
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 ADDED BY S.G. Moved outside the two if statement
		//MI additions
		if(g_bRealisticAvionics)
		{
			if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->TrimAPDisc)
				return;
		}

		if (state & KEY_DOWN)
			pitchElevatorTrimRate = -0.25F;
		else
			pitchElevatorTrimRate = 0.0F;
	}
}

void AFElevatorTrimDown (unsigned long, int state, void*)
{
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 ADDED BY S.G. Moved outside the two if statement
	//MI additions
		if(g_bRealisticAvionics)
		{
			if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->TrimAPDisc)
				return;
		}

		if (state & KEY_DOWN)
			pitchElevatorTrimRate = 0.25F;
		else
			pitchElevatorTrimRate = 0.0F;
	}
}

void AFAlternateGear (unsigned long, int state, void*)
{
	if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP))
	{
		if (state & KEY_DOWN) {
			SimDriver.GetPlayerAircraft()->af->gearHandle = 1.0F;
			SimDriver.GetPlayerAircraft()->af->altGearDeployed = true;
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_ALT_GEAR, 2);
		}
	}
}

void AFAlternateGearReset (unsigned long, int state, void*)
{
	if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP))
	{
		if (state & KEY_DOWN) {
			SimDriver.GetPlayerAircraft()->af->altGearDeployed = false;
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_ALT_GEAR, 1);
		}
	}
}

void AFResetTrim (unsigned long, int state, void*)
{
	if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP))
	{
		if (state & KEY_DOWN) {
			UserStickInputs.Reset();
		}
	}
}

//MI ICP Stuff
void SimICPIFF(unsigned long, int state, void* pButton)
{
	if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		if(g_bRealisticAvionics)
		{ 
			//Player pushed the button, tell it to our ICP class
			OTWDriver.pCockpitManager->mpIcp->HandleInput(IFF_BUTTON, (CPButtonObject*)pButton);
		}
	}		
}

void SimICPLIST(unsigned long, int state, void* pButton)
{
	if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		if(g_bRealisticAvionics)
		{ 
			//Player pushed the button, tell it to our ICP class
			OTWDriver.pCockpitManager->mpIcp->HandleInput(LIST_BUTTON, (CPButtonObject*)pButton);
		}
	}		
}

void SimICPTHREE(unsigned long, int state, void* pButton)
{
	if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		if(g_bRealisticAvionics)
		{ 
			//Player pushed the button, tell it to our ICP class
			OTWDriver.pCockpitManager->mpIcp->HandleInput(THREE_BUTTON, (CPButtonObject*)pButton);
		}
	}		
}

void SimICPSIX(unsigned long, int state, void* pButton)
{
	if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		if(g_bRealisticAvionics)
		{ 
			//Player pushed the button, tell it to our ICP class
			OTWDriver.pCockpitManager->mpIcp->HandleInput(SIX_BUTTON, (CPButtonObject*)pButton);
		}
		else
		{
			if (state & KEY_DOWN && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.GetPlayerAircraft())->ejectTriggered) 
			{
				OTWDriver.pCockpitManager->mpIcp->HandleInput(DLINK_BUTTON, (CPButtonObject*)pButton);
			}
		}
	}		
}

void SimICPEIGHT(unsigned long, int state, void* pButton)
{
	if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		if(g_bRealisticAvionics)
		{ 
			//Player pushed the button, tell it to our ICP class
			OTWDriver.pCockpitManager->mpIcp->HandleInput(EIGHT_BUTTON, (CPButtonObject*)pButton);
		}
	}		
}

void SimICPNINE(unsigned long, int state, void* pButton)
{
	if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		if(g_bRealisticAvionics)
		{ 
			//Player pushed the button, tell it to our ICP class
			OTWDriver.pCockpitManager->mpIcp->HandleInput(NINE_BUTTON, (CPButtonObject*)pButton);
		}
	}		
}

void SimICPZERO(unsigned long, int state, void* pButton)
{
	if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		if(g_bRealisticAvionics)
		{ 
			//Player pushed the button, tell it to our ICP class
			OTWDriver.pCockpitManager->mpIcp->HandleInput(ZERO_BUTTON, (CPButtonObject*)pButton);
		}
	}		
}

void SimICPResetDED(unsigned long, int state, void* pButton)
{
	if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		if(g_bRealisticAvionics)
		{ 
			//Player pushed the button, tell it to our ICP class
			OTWDriver.pCockpitManager->mpIcp->HandleInput(CNI_MODE, (CPButtonObject*)pButton);
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_ICP_DED, 8);
		}
	}		
	else
	{
		OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_ICP_DRIFTCO, 1);
	}
}

void SimICPDEDUP(unsigned long, int state, void* pButton)
{
	if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		if(g_bRealisticAvionics)
		{ 
			//Player pushed the button, tell it to our ICP class
			OTWDriver.pCockpitManager->mpIcp->HandleInput(UP_BUTTON, (CPButtonObject*)pButton);
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_ICP_DED, 2);
		}
	}		
}

void SimICPDEDDOWN(unsigned long, int state, void* pButton)
{
	if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		if(g_bRealisticAvionics)
		{ 
			//Player pushed the button, tell it to our ICP class
			OTWDriver.pCockpitManager->mpIcp->HandleInput(DOWN_BUTTON, (CPButtonObject*)pButton);
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_ICP_DED, 8);
		}
	}		
	else
	{
		OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_ICP_DRIFTCO, 1);
	}
}

void SimICPDEDSEQ(unsigned long, int state, void* pButton)
{
	if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		if(g_bRealisticAvionics)
		{ 
			//Player pushed the button, tell it to our ICP class
			OTWDriver.pCockpitManager->mpIcp->HandleInput(SEQ_BUTTON, (CPButtonObject*)pButton);
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_ICP_DED, 16);
		}
	}
	else
	{
		OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_ICP_DRIFTCO, 1);
	}
}

void SimICPCLEAR(unsigned long, int state, void* pButton)
{
	if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		if(g_bRealisticAvionics)
		{ 
			//Player pushed the button, tell it to our ICP class
			OTWDriver.pCockpitManager->mpIcp->HandleInput(CLEAR_BUTTON, (CPButtonObject*)pButton);
		}
	}		
}

void SimRALTSTDBY(unsigned long, int state, void* pButton)
{
  if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
  {
		SimDriver.GetPlayerAircraft()->af->platform->RaltStdby();
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_RALT_PWR, 2);
  }
}

void SimRALTON(unsigned long, int state, void* pButton)
{
  if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
  {
		//Set it on here 
		SimDriver.GetPlayerAircraft()->af->platform->RaltOn();
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_RALT_PWR, 4);
  }
}

void SimRALTOFF(unsigned long, int state, void* pButton)
{
    if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
    {
			//Set it off here
			SimDriver.GetPlayerAircraft()->af->platform->RaltOff();
			//take the juice
			SimDriver.GetPlayerAircraft()->PowerOff(AircraftClass::RaltPower);
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_RALT_PWR, 1);
    }
}

void SimFLIRToggle(unsigned long, int state, void* pButton)
{
    if (theLantirn && SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
    {
			theLantirn->ToggleFLIR();
    }
}

void SimSMSPower(unsigned long val, int state, void *)
{
    if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		{
			SimDriver.GetPlayerAircraft()->PowerToggle(AircraftClass::SMSPower);
			if (OTWDriver.GetVirtualCockpit())
				//OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_SMS_PWR, SimDriver.GetPlayerAircraft()->HasPower(AircraftClass::SMSPower)+1);
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_SMS_PWR, SimDriver.GetPlayerAircraft()->HasPower(AircraftClass::SMSPower));
		}
}

// MD -- 20031130: adding commands to place the SMS power switch directly

void SimSMSOn(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;

    if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		{
			SimDriver.GetPlayerAircraft()->PowerOn(AircraftClass::SMSPower);
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_SMS_PWR, 2);
		}
}

void SimSMSOff(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;

    if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		{
			SimDriver.GetPlayerAircraft()->PowerOff(AircraftClass::SMSPower);
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_SMS_PWR, 1);
		}
}

void SimFCCPower(unsigned long val, int state, void *)
{
    if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		{
			SimDriver.GetPlayerAircraft()->PowerToggle (AircraftClass::FCCPower);
			if (OTWDriver.GetVirtualCockpit())
				//OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_FCC_PWR, SimDriver.GetPlayerAircraft()->HasPower(AircraftClass::FCCPower)+1);
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_FCC_PWR, 1<<SimDriver.GetPlayerAircraft()->HasPower(AircraftClass::FCCPower));
		}
}

// MD -- 20031130: adding commands to place the FCC power switch directly

void SimFCCOn(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;

    if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		{
			SimDriver.GetPlayerAircraft()->PowerOn(AircraftClass::FCCPower);
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_FCC_PWR, 2);
		}
}

void SimFCCOff(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;

    if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		{
			SimDriver.GetPlayerAircraft()->PowerOff(AircraftClass::FCCPower);
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_FCC_PWR, 1);
		}
}

void SimMFDPower(unsigned long val, int state, void *)
{
    if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		{
			SimDriver.GetPlayerAircraft()->PowerToggle (AircraftClass::MFDPower);
			if (OTWDriver.GetVirtualCockpit())
				//OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_MFD_PWR, SimDriver.GetPlayerAircraft()->HasPower(AircraftClass::MFDPower)+1);
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_MFD_PWR, 1<<SimDriver.GetPlayerAircraft()->HasPower(AircraftClass::MFDPower));
		}
}

// MD -- 20031130: adding commands to place the MFD power switch directly

void SimMFDOn(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;

    if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		{
			SimDriver.GetPlayerAircraft()->PowerOn(AircraftClass::MFDPower);
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_MFD_PWR, 2);
		}
}

void SimMFDOff(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;

    if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		{
			SimDriver.GetPlayerAircraft()->PowerOff(AircraftClass::MFDPower);
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_MFD_PWR, 1);
		}
}

void SimUFCPower(unsigned long val, int state, void *)
{
    if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		{
			SimDriver.GetPlayerAircraft()->PowerToggle (AircraftClass::UFCPower);
			if (OTWDriver.GetVirtualCockpit())
				//OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_UFC_PWR, SimDriver.GetPlayerAircraft()->HasPower(AircraftClass::UFCPower)+1);
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_UFC_PWR, 1<<SimDriver.GetPlayerAircraft()->HasPower(AircraftClass::UFCPower));
		}
}

// MD -- 20031130: adding commands to place the UFC power switch directly

void SimUFCOn(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;

    if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		{
			SimDriver.GetPlayerAircraft()->PowerOn(AircraftClass::UFCPower);
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_UFC_PWR, 2);
		}
}

void SimUFCOff(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;

    if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		{
			SimDriver.GetPlayerAircraft()->PowerOff(AircraftClass::UFCPower);
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_UFC_PWR, 1);
		}
}

void SimGPSPower(unsigned long val, int state, void *)
{
    if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		{
			SimDriver.GetPlayerAircraft()->PowerToggle (AircraftClass::GPSPower);
			if (OTWDriver.GetVirtualCockpit())
				//OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_GPS_PWR, SimDriver.GetPlayerAircraft()->HasPower(AircraftClass::GPSPower)+1);
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_GPS_PWR, 1<<SimDriver.GetPlayerAircraft()->HasPower(AircraftClass::GPSPower));
		}
}

// MD -- 20031130: adding commands to place the GPS power switch directly

void SimGPSOn(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;

    if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		{
			SimDriver.GetPlayerAircraft()->PowerOn(AircraftClass::GPSPower);
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_GPS_PWR, 2);
		}
}

void SimGPSOff(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;

    if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		{
			SimDriver.GetPlayerAircraft()->PowerOff(AircraftClass::GPSPower);
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_GPS_PWR, 1);
		}
}

void SimDLPower(unsigned long val, int state, void *)
{
    if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		{
			// RV - Biker
			if (SimDriver.GetPlayerAircraft()->af->GetDataLinkCapLevel() > 0)
				SimDriver.GetPlayerAircraft()->PowerToggle (AircraftClass::DLPower);
			else
				SimDriver.GetPlayerAircraft()->PowerOff (AircraftClass::DLPower);
			if (OTWDriver.GetVirtualCockpit())
				//OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_DL_PWR, SimDriver.GetPlayerAircraft()->HasPower(AircraftClass::DLPower)+1);
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_DL_PWR, 1<<SimDriver.GetPlayerAircraft()->HasPower(AircraftClass::DLPower));
		}
}

// MD -- 20031130: adding commands to place the DL power switch directly

void SimDLOn(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;

    if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		{
			// RV - Biker
			if (SimDriver.GetPlayerAircraft()->af->GetDataLinkCapLevel() > 0)
				SimDriver.GetPlayerAircraft()->PowerOn(AircraftClass::DLPower);
			else
				SimDriver.GetPlayerAircraft()->PowerOff (AircraftClass::DLPower);
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_DL_PWR, 2);
		}
}

void SimDLOff(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;

    if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		{
			SimDriver.GetPlayerAircraft()->PowerOff(AircraftClass::DLPower);
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_DL_PWR, 1);
		}
}

void SimMAPPower(unsigned long val, int state, void *)
{
    if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		{
			SimDriver.GetPlayerAircraft()->PowerToggle (AircraftClass::MAPPower);
			if (OTWDriver.GetVirtualCockpit())
				//OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_MAP_PWR, SimDriver.GetPlayerAircraft()->HasPower(AircraftClass::MAPPower)+1);
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_MAP_PWR, 1<<SimDriver.GetPlayerAircraft()->HasPower(AircraftClass::MAPPower));
		}
}

// MD -- 20031130: adding commands to place the MAP power switch directly

void SimMAPOn(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;

    if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		{
			SimDriver.GetPlayerAircraft()->PowerOn(AircraftClass::MAPPower);
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_MAP_PWR, 2);
		}
}

void SimMAPOff(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;

    if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		{
			SimDriver.GetPlayerAircraft()->PowerOff(AircraftClass::MAPPower);
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_MAP_PWR, 1);
		}
}

void SimRightHptPower(unsigned long val, int state, void *)
{
    if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		SimDriver.GetPlayerAircraft()->PowerToggle (AircraftClass::RightHptPower);
		//MI
		if(g_bRealisticAvionics)
		{
			if(SimDriver.GetPlayerAircraft()->FCC)
			{
				if(SimDriver.GetPlayerAircraft()->HasPower(AircraftClass::RightHptPower))
				{
					SimDriver.GetPlayerAircraft()->FCC->LaserFire = FALSE;
					SimDriver.GetPlayerAircraft()->FCC->InhibitFire = FALSE;
					if (OTWDriver.GetVirtualCockpit())
						OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_RIGHT_HPT_PWR, 2);
				}
				else
				{
					SimDriver.GetPlayerAircraft()->FCC->LaserFire = FALSE;
					SimDriver.GetPlayerAircraft()->FCC->InhibitFire = TRUE;
					if (OTWDriver.GetVirtualCockpit())
						OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_RIGHT_HPT_PWR, 1);
				}
			}
		}
	}
}

// MD -- 20031129: adding commands to place the right hardpoint power switch directly.

void SimRightHptOn(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;

    if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		SimDriver.GetPlayerAircraft()->PowerOn(AircraftClass::RightHptPower);
		if(SimDriver.GetPlayerAircraft()->FCC)
		{
			SimDriver.GetPlayerAircraft()->FCC->LaserFire = FALSE;
			SimDriver.GetPlayerAircraft()->FCC->InhibitFire = FALSE;
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_RIGHT_HPT_PWR, 2);
		}
	}
}

void SimRightHptOff(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;

    if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		SimDriver.GetPlayerAircraft()->PowerOff(AircraftClass::RightHptPower);
		if(SimDriver.GetPlayerAircraft()->FCC)
		{
			SimDriver.GetPlayerAircraft()->FCC->LaserFire = FALSE;
			SimDriver.GetPlayerAircraft()->FCC->InhibitFire = TRUE;
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_RIGHT_HPT_PWR, 1);
		}
	}
}

void SimLeftHptPower(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;

    if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		{
			SimDriver.GetPlayerAircraft()->PowerToggle (AircraftClass::LeftHptPower);
			if (OTWDriver.GetVirtualCockpit())
				if (SimDriver.GetPlayerAircraft()->HasPower(AircraftClass::LeftHptPower))
					OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_LEFT_HPT_PWR, 2);
				else
					OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_LEFT_HPT_PWR, 1);
		}
}

// MD -- 20031129: adding commands to place the left hardpoint power switch directly.

void SimLeftHptOn(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;

    if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		{
			SimDriver.GetPlayerAircraft()->PowerOn(AircraftClass::LeftHptPower);
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_LEFT_HPT_PWR, 2);
		}
}

void SimLeftHptOff(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;

    if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		{
			SimDriver.GetPlayerAircraft()->PowerOff(AircraftClass::LeftHptPower);
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_LEFT_HPT_PWR, 1);
		}
}


void SimTISLPower(unsigned long val, int state, void *)
{
    if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		SimDriver.GetPlayerAircraft()->PowerToggle (AircraftClass::TISLPower);
}

void SimFCRPower(unsigned long val, int state, void *)
{
    if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		{
			SimDriver.GetPlayerAircraft()->PowerToggle (AircraftClass::FCRPower);
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_FCR_PWR, SimDriver.GetPlayerAircraft()->HasPower(AircraftClass::FCRPower)+1);
		}
}

// MD -- 20031129: adding commands to place the fire control radar power switch directly

void SimFCROn(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;

  if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		SimDriver.GetPlayerAircraft()->PowerOn(AircraftClass::FCRPower);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_FCR_PWR, 2);
	}
}

void SimFCROff(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;

  if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		SimDriver.GetPlayerAircraft()->PowerOff(AircraftClass::FCRPower);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_FCR_PWR, 1);
	}
}

void SimHUDPower(unsigned long val, int state, void *)
{
  if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		SimDriver.GetPlayerAircraft()->PowerToggle (AircraftClass::HUDPower);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_HUD_PWR, SimDriver.GetPlayerAircraft()->HasPower(AircraftClass::HUDPower)+1);
	}
}

// MD -- 20031129: adding commands to map to the on/off switch of the thumb wheel HUD brightness control on the ICP

void SimHUDOn(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;

  if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		SimDriver.GetPlayerAircraft()->PowerOn(AircraftClass::HUDPower);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_HUD_PWR, 2);
	}
}

void SimHUDOff(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;

  if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		SimDriver.GetPlayerAircraft()->PowerOff(AircraftClass::HUDPower);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_HUD_PWR, 1);
	}
}

void SimToggleRealisticAvionics(unsigned long val, int state, void *)
{
    if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) // 2002-02-10 MODIFIED BY S.G. Removed ; at the end of the if statement
		g_bRealisticAvionics = !g_bRealisticAvionics;
}

void SimIncFuelSwitch(unsigned long val, int state, void *)
{
    if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		{
			SimDriver.GetPlayerAircraft()->af->IncFuelSwitch();
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_FUEL_QTY, (SimDriver.GetPlayerAircraft()->af->GetFuelSwitch()+1));
		}
}

void SimDecFuelSwitch(unsigned long val, int state, void *)
{
    if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		{
			SimDriver.GetPlayerAircraft()->af->DecFuelSwitch();
			if (OTWDriver.GetVirtualCockpit())
				//OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_FUEL_QTY, (SimDriver.GetPlayerAircraft()->af->GetFuelSwitch()+1));
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_FUEL_QTY, 1<<(SimDriver.GetPlayerAircraft()->af->GetFuelSwitch()));
		}
}

// MD -- 20031024: Adding commands to set the Fuel Pump Knob position explicitly to
// make it easier for cockpit builders to map the commands to a physical knob.

void SimFuelSwitchTest(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
    if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		{
			SimDriver.GetPlayerAircraft()->af->SetFuelSwitch(AirframeClass::FS_TEST);
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_FUEL_QTY, 32);
		}
}

void SimFuelSwitchNorm(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
    if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		{
			SimDriver.GetPlayerAircraft()->af->SetFuelSwitch(AirframeClass::FS_NORM);
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_FUEL_QTY, 1);
		}
}

void SimFuelSwitchResv(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
    if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		{
			SimDriver.GetPlayerAircraft()->af->SetFuelSwitch(AirframeClass::FS_RESV);
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_FUEL_QTY, 2);
		}
}
void SimFuelSwitchWingInt(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
    if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		{
			SimDriver.GetPlayerAircraft()->af->SetFuelSwitch(AirframeClass::FS_WINGINT);
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_FUEL_QTY, 4);
		}
}
void SimFuelSwitchWingExt(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
    if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		{
			SimDriver.GetPlayerAircraft()->af->SetFuelSwitch(AirframeClass::FS_WINGEXT);
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_FUEL_QTY, 8);
		}
}
void SimFuelSwitchCenterExt(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
    if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		{
			SimDriver.GetPlayerAircraft()->af->SetFuelSwitch(AirframeClass::FS_CENTEREXT);
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_FUEL_QTY, 16);
		}
}

void SimIncFuelPump(unsigned long val, int state, void *)
{
    if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		{
			SimDriver.GetPlayerAircraft()->af->IncFuelPump();
			SimDriver.GetPlayerAircraft()->af->SetFuelPump(AirframeClass::FP_OFF);
			//int val = SimDriver.GetPlayerAircraft()->af->GetFuelPump()+1;
			int val = 1<<SimDriver.GetPlayerAircraft()->af->GetFuelPump();
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_REFUEL_PUMP, val);
		}
}

void SimDecFuelPump(unsigned long val, int state, void *)
{
    if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		{
			SimDriver.GetPlayerAircraft()->af->DecFuelPump();
			//int val = SimDriver.GetPlayerAircraft()->af->GetFuelPump()+1;
			int val = 1<<SimDriver.GetPlayerAircraft()->af->GetFuelPump();
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_REFUEL_PUMP, val);
		}
}

// MD -- 20031024: Adding commands to set the Fuel Pump Knob position explicitly to
// make it easier for cockpit builders to map the commands to a physical knob.

void SimFuelPumpOff(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
    if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		{
			SimDriver.GetPlayerAircraft()->af->SetFuelPump(AirframeClass::FP_OFF);
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_REFUEL_PUMP, 1);
		}
}

void SimFuelPumpNorm(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
    if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		{
			SimDriver.GetPlayerAircraft()->af->SetFuelPump(AirframeClass::FP_NORM);
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_REFUEL_PUMP, 2);
		}
}

void SimFuelPumpAft(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
    if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		{
			SimDriver.GetPlayerAircraft()->af->SetFuelPump(AirframeClass::FP_AFT);
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_REFUEL_PUMP, 4);
		}
}

void SimFuelPumpFwd(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
    if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		{
			SimDriver.GetPlayerAircraft()->af->SetFuelPump(AirframeClass::FP_FWD);
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_REFUEL_PUMP, 8);
		}
}

void SimToggleMasterFuel(unsigned long val, int state, void *)
{
  if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		SimDriver.GetPlayerAircraft()->af->ToggleEngineFlag(AirframeClass::MasterFuelOff);
		if (OTWDriver.GetVirtualCockpit())
			if (SimDriver.GetPlayerAircraft()->af->IsEngineFlag(AirframeClass::MasterFuelOff))
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_REFUEL_MSTR, 1);
			else
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_REFUEL_MSTR, 2);
	}
}

// MD -- 20031125: Adding separate commands to turn fuel master on and off directly.

void SimMasterFuelOn(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
  if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		SimDriver.GetPlayerAircraft()->af->ClearEngineFlag(AirframeClass::MasterFuelOff);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_REFUEL_MSTR, 2);
	}
}

void SimMasterFuelOff(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
  if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		SimDriver.GetPlayerAircraft()->af->SetEngineFlag(AirframeClass::MasterFuelOff);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_REFUEL_MSTR, 1);
	}
}

void SimIncAirSource(unsigned long val, int state, void *)
{
  if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		SimDriver.GetPlayerAircraft()->af->IncAirSource();
		if (OTWDriver.GetVirtualCockpit())
			//OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_AIR_SOURCE, SimDriver.GetPlayerAircraft()->af->GetAirSource()+1);
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_AIR_SOURCE, 1<<SimDriver.GetPlayerAircraft()->af->GetAirSource());
	}
}

void SimDecAirSource(unsigned long val, int state, void *)
{
  if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		SimDriver.GetPlayerAircraft()->af->DecAirSource();
		if (OTWDriver.GetVirtualCockpit())
			//OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_AIR_SOURCE, SimDriver.GetPlayerAircraft()->af->GetAirSource()+1);
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_AIR_SOURCE, 1<<SimDriver.GetPlayerAircraft()->af->GetAirSource());
	}
}

// MD -- 20031024: Adding commands to set the Air Source Knob position explicitly to
// make it easier for cockpit builders to map the commands to a physical knob.

void SimAirSourceOff(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
  if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		SimDriver.GetPlayerAircraft()->af->SetAirSource(AirframeClass::AS_OFF);
		if (OTWDriver.GetVirtualCockpit())
			//OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_AIR_SOURCE, SimDriver.GetPlayerAircraft()->af->GetAirSource()+1);
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_AIR_SOURCE, 1<<SimDriver.GetPlayerAircraft()->af->GetAirSource());
	}
}

void SimAirSourceNorm(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
  if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		SimDriver.GetPlayerAircraft()->af->SetAirSource(AirframeClass::AS_NORM);
		if (OTWDriver.GetVirtualCockpit())
			//OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_AIR_SOURCE, SimDriver.GetPlayerAircraft()->af->GetAirSource()+1);
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_AIR_SOURCE, 1<<SimDriver.GetPlayerAircraft()->af->GetAirSource());
	}
}
void SimAirSourceDump(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
  if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		SimDriver.GetPlayerAircraft()->af->SetAirSource(AirframeClass::AS_DUMP);
		if (OTWDriver.GetVirtualCockpit())
			//OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_AIR_SOURCE, SimDriver.GetPlayerAircraft()->af->GetAirSource()+1);
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_AIR_SOURCE, 1<<SimDriver.GetPlayerAircraft()->af->GetAirSource());
	}
}
void SimAirSourceRam(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
  if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		SimDriver.GetPlayerAircraft()->af->SetAirSource(AirframeClass::AS_RAM);
		if (OTWDriver.GetVirtualCockpit())
			//OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_AIR_SOURCE, SimDriver.GetPlayerAircraft()->af->GetAirSource()+1);
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_AIR_SOURCE, 1<<SimDriver.GetPlayerAircraft()->af->GetAirSource());
	}
}

// COBRA - RED - Test feature... to switch PS from old to new engine...
extern	bool	DebugTrail;

void SimLandingLightToggle(unsigned long val, int state, void *)
{
	AircraftClass *pac = SimDriver.GetPlayerAircraft();
	if(pac && pac->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		if (pac->IsAcStatusBitsSet(AircraftClass::ACSTATUS_EXT_LANDINGLIGHT))
		{
			pac->ClearAcStatusBits(AircraftClass::ACSTATUS_EXT_LANDINGLIGHT);
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_LAND_LIGHT, 1);
		}
		else
		{
			pac->SetAcStatusBits(AircraftClass::ACSTATUS_EXT_LANDINGLIGHT);
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_LAND_LIGHT, 2);
		}
	}
}

// MD -- 20031128: adding commands to place the landing light switch directly

void SimLandingLightOn(unsigned long val, int state, void *){
	AircraftClass *pac = SimDriver.GetPlayerAircraft();	
	if (pac && pac->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		pac->SetAcStatusBits(AircraftClass::ACSTATUS_EXT_LANDINGLIGHT);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_LAND_LIGHT, 2);
	}
}

void SimLandingLightOff(unsigned long val, int state, void *){
	AircraftClass *pac = SimDriver.GetPlayerAircraft();
	if (pac && pac->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		pac->ClearAcStatusBits(AircraftClass::ACSTATUS_EXT_LANDINGLIGHT);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_LAND_LIGHT, 1);
	}
}

void SimParkingBrakeToggle(unsigned long val, int state, void *)
{
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		SimDriver.GetPlayerAircraft()->af->TogglePB();
	if (OTWDriver.GetVirtualCockpit())
	{
		if (SimDriver.GetPlayerAircraft()->af->PBON == TRUE)
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_PARK_BRAKE, 2);
		else
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_PARK_BRAKE, 1);
	}
}

// MD -- 20031128: adding commands to place the parking brake switch directly

void SimParkingBrakeOn(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
	
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) {
		if(SimDriver.GetPlayerAircraft()->af->vt > 1.0F * KNOTS_TO_FTPSEC) {
			SimDriver.GetPlayerAircraft()->af->PBON = FALSE;
			SimDriver.GetPlayerAircraft()->af->ClearFlag(AirframeClass::WheelBrakes);
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_PARK_BRAKE, 1);
			return;
		}
		SimDriver.GetPlayerAircraft()->af->PBON = TRUE;
		SimDriver.GetPlayerAircraft()->af->SetFlag(AirframeClass::WheelBrakes);		
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_PARK_BRAKE, 2);
	}
}

void SimParkingBrakeOff(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
	
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) {
		SimDriver.GetPlayerAircraft()->af->PBON = FALSE;
		SimDriver.GetPlayerAircraft()->af->ClearFlag(AirframeClass::WheelBrakes);		
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_PARK_BRAKE, 1);
	}
}

// JB carrier start
void SimHookToggle(unsigned long val, int state, void *)
{
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN) 
		//&& SimDriver.GetPlayerAircraft()->mFaults->GetFault( FaultClass::hook_fault )
		)
	{
		SimDriver.GetPlayerAircraft()->af->ToggleHook();
		if (OTWDriver.GetVirtualCockpit())
		{
			if (SimDriver.GetPlayerAircraft()->af->IsSet(AirframeClass::Hook))
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_HOOK, 2);
			else
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_HOOK, 1);
		}
	}
}

// MD -- 20031128: adding commands to place the hook switch directly.

void SimHookUp(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
	
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		SimDriver.GetPlayerAircraft()->af->HookUp();
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_HOOK, 1);
	}
}

void SimHookDown(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
	
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		SimDriver.GetPlayerAircraft()->af->HookDown();
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_HOOK, 2);
	}
}

// JB carrier end
void SimLaserArmToggle(unsigned long val, int state, void *)
{
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		if(SimDriver.GetPlayerAircraft()->FCC)
			SimDriver.GetPlayerAircraft()->FCC->ToggleLaserArm();
		if (OTWDriver.GetVirtualCockpit())
			if (SimDriver.GetPlayerAircraft()->FCC->LaserArm)
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_LASER_ARM, 2);
			else
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_LASER_ARM, 1);
	}
}

// MD -- 20031129: adding commands to place the laser arm switch directly

void SimLaserArmOn(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
	
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		if(SimDriver.GetPlayerAircraft()->Sms->MasterArm() != SMSBaseClass::MasterArmState::Arm)
			return;

		if(SimDriver.GetPlayerAircraft()->FCC)
			SimDriver.GetPlayerAircraft()->FCC->LaserArm = TRUE;
			SimDriver.GetPlayerAircraft()->FCC->LaserFire = FALSE;

		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_LASER_ARM, 2);
	}
}
void SimLaserArmOff(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
	
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		// Set state to OFF regarless of the Master Arm state.
		if(SimDriver.GetPlayerAircraft()->FCC)
			SimDriver.GetPlayerAircraft()->FCC->LaserArm = FALSE;
			SimDriver.GetPlayerAircraft()->FCC->LaserFire = FALSE;

		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_LASER_ARM, 1);
	}
}

void SimFuelDoorToggle(unsigned long val, int state, void *)
{
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		SimDriver.GetPlayerAircraft()->af->ToggleEngineFlag(AirframeClass::FuelDoorOpen);
		if (OTWDriver.GetVirtualCockpit())
			if (SimDriver.GetPlayerAircraft()->af->IsEngineFlag(AirframeClass::FuelDoorOpen))
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_REFUEL_DOOR, 2);
			else
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_REFUEL_DOOR, 1);
	}
}

// MD -- 20031125: Adding commands to place the refueling door switch directly

void SimFuelDoorOpen(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
	
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		SimDriver.GetPlayerAircraft()->af->SetEngineFlag(AirframeClass::FuelDoorOpen);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_REFUEL_DOOR, 2);
	}
}

void SimFuelDoorClose(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
	
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		SimDriver.GetPlayerAircraft()->af->ClearEngineFlag(AirframeClass::FuelDoorOpen);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_REFUEL_DOOR, 1);
	}
}

//Autopilot
//Right switch. Three positions. ALT Hold, OFF and ATT Hold
//ALT Hold holds ALT, and left switch controls what we hold.

// MD -- 20031124: replaced SetAPParameters() call for "off" position with SetAutopilot()
// this fixes the SP3 bug where the nose would pitch up uncontrollably on landing if the AP
// was used in pitch hold mode during any flight.

void SimRightAPSwitch(unsigned long val, int state, void *)
{
	//This is the right switch, in the upper position.
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		if(g_bRealisticAvionics)
		{
			//We get Altitude hold now.?
			if(SimDriver.GetPlayerAircraft()->IsOn(AircraftClass::AltHold))	//up
			{
				//from Alt hold to Att hold
				SimDriver.GetPlayerAircraft()->ClearAPFlag(AircraftClass::AltHold);
				//can't be both
				SimDriver.GetPlayerAircraft()->SetAPFlag(AircraftClass::AttHold);	//down
				SimDriver.GetPlayerAircraft()->SetNewPitch();
			//	SimDriver.GetPlayerAircraft()->SetNewRoll();  // ...but only if RollHold is selected.
			// Following test and set is overkill but guards against the possibility that someone entered a jet that
			// had the right switch in something other than center position as the default.
				if ( (!SimDriver.GetPlayerAircraft()->IsOn(AircraftClass::StrgSel)) && (!SimDriver.GetPlayerAircraft()->IsOn(AircraftClass::StrgSel))) {
					SimDriver.GetPlayerAircraft()->SetAPFlag(AircraftClass::RollHold); // needed in case this is the first time and switch is still in default position
					SimDriver.GetPlayerAircraft()->SetNewRoll();
				}
				//Tell us what we want
				SimDriver.GetPlayerAircraft()->SetAPParameters();
				// now apply power
				SimDriver.GetPlayerAircraft()->PowerOn(AircraftClass::APPower);
			}
			else
				if(SimDriver.GetPlayerAircraft()->IsOn(AircraftClass::AttHold))
				{
					//all off
					SimDriver.GetPlayerAircraft()->ClearAPFlag(AircraftClass::AltHold);
					SimDriver.GetPlayerAircraft()->ClearAPFlag(AircraftClass::AttHold);
					// SimDriver.GetPlayerAircraft()->SetAPParameters(); <- replaced to fix the "SP3 pitch up on landing after AP use" bug
					SimDriver.GetPlayerAircraft()->SetAutopilot(AircraftClass::APOff);
					//take the juice
					SimDriver.GetPlayerAircraft()->PowerOff(AircraftClass::APPower);
				}
				else
				{
					//from off to Alt Hold
					SimDriver.GetPlayerAircraft()->SetAPFlag(AircraftClass::AltHold);
					SimDriver.GetPlayerAircraft()->ClearAPFlag(AircraftClass::AttHold);
					//tell us what we should hold
					SimDriver.GetPlayerAircraft()->SetNewAlt();
					// MD -- 20031109: we should be holding roll at this point as well if the roll more switch is
					// centered.  Also a good place to set the RollHold flag in case this is the first time that the
					// AP has been activated.
					if ( (!SimDriver.GetPlayerAircraft()->IsOn(AircraftClass::StrgSel)) && (!SimDriver.GetPlayerAircraft()->IsOn(AircraftClass::StrgSel))) {
						SimDriver.GetPlayerAircraft()->SetAPFlag(AircraftClass::RollHold); // needed in case this is the first time and switch is still in default position
						SimDriver.GetPlayerAircraft()->SetNewRoll();
					}
					//Tell us what we want
					SimDriver.GetPlayerAircraft()->SetAPParameters();
					// now apply power
					SimDriver.GetPlayerAircraft()->PowerOn(AircraftClass::APPower);
				}

				if (OTWDriver.GetVirtualCockpit())
				{
					// Right switch up position
					if(SimDriver.GetPlayerAircraft()->IsOn(AircraftClass::AltHold))
						OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_RT_AP_SW, 4);
					// Right switch down position
					else if(SimDriver.GetPlayerAircraft()->IsOn(AircraftClass::AttHold))
						OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_RT_AP_SW, 1);
					// Right switch middle position (off)
					else
						OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_RT_AP_SW, 1);
				}
		}
	}
}

// MD -- 20031017: adding key commands for each specific position of the two autopilot
// control switches.

void SimRightAPUp(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
	//This is the right switch, going to the ALT HOLD position
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) {
		SimDriver.GetPlayerAircraft()->ClearAPFlag(AircraftClass::AttHold);
		//can't be both
		SimDriver.GetPlayerAircraft()->SetAPFlag(AircraftClass::AltHold);
		//tell us what we should hold
		SimDriver.GetPlayerAircraft()->SetNewAlt();
		// MD -- 20031108: fixing AP ATT HLD modes.  Pitch switch should not set roll unless the
		// roll mode is set to roll hold.
		if ( (!SimDriver.GetPlayerAircraft()->IsOn(AircraftClass::StrgSel)) && (!SimDriver.GetPlayerAircraft()->IsOn(AircraftClass::HDGSel))) {
			SimDriver.GetPlayerAircraft()->SetAPFlag(AircraftClass::RollHold); // needed in case this is the first time and switch is still in default position
			SimDriver.GetPlayerAircraft()->SetNewRoll();
		}
		//Tell us what we want
		SimDriver.GetPlayerAircraft()->SetAPParameters();
				// now apply power
		SimDriver.GetPlayerAircraft()->PowerOn(AircraftClass::APPower);

		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_RT_AP_SW, 2);
	}
}

void SimRightAPMid(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
	//This is the right switch, going to the A/P OFF position
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) {
		SimDriver.GetPlayerAircraft()->ClearAPFlag(AircraftClass::AttHold);
		SimDriver.GetPlayerAircraft()->ClearAPFlag(AircraftClass::AltHold);
		//take the juice
		SimDriver.GetPlayerAircraft()->PowerOff(AircraftClass::APPower);
		//Tell us what we want
		SimDriver.GetPlayerAircraft()->SetAutopilot(AircraftClass::APOff);

		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_RT_AP_SW, 1);
	}
}

void SimRightAPDown(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
	//This is the right switch, going to the ATT HOLD position
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) {
		SimDriver.GetPlayerAircraft()->ClearAPFlag(AircraftClass::AltHold);
		//can't be both
		SimDriver.GetPlayerAircraft()->SetAPFlag(AircraftClass::AttHold);	//down
		SimDriver.GetPlayerAircraft()->SetNewPitch();
		// MD -- 20031108: fixing AP ATT HLD modes.  Pitch switch should not set roll unless the
		// roll mode is set to roll hold.
		if ( (!SimDriver.GetPlayerAircraft()->IsOn(AircraftClass::StrgSel)) && (!SimDriver.GetPlayerAircraft()->IsOn(AircraftClass::HDGSel))) {
			SimDriver.GetPlayerAircraft()->SetAPFlag(AircraftClass::RollHold); // needed in case this is the first time and switch is still in default position
			SimDriver.GetPlayerAircraft()->SetNewRoll();
		}
		//Tell us what we want
		SimDriver.GetPlayerAircraft()->SetAPParameters();
		// now apply power
		SimDriver.GetPlayerAircraft()->PowerOn(AircraftClass::APPower);

		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_RT_AP_SW, 4);
	}
}

//Left AP Switch
void SimLeftAPSwitch(unsigned long val, int state, void *)
{
	//This is the right switch, in the upper position.
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		//Middle position
		if(SimDriver.GetPlayerAircraft()->IsOn(AircraftClass::RollHold))
		{
			SimDriver.GetPlayerAircraft()->ClearAPFlag(AircraftClass::RollHold);
			SimDriver.GetPlayerAircraft()->ClearAPFlag(AircraftClass::HDGSel);
			SimDriver.GetPlayerAircraft()->SetAPFlag(AircraftClass::StrgSel);	//waypoint AP
		}
		//down position
		else if(SimDriver.GetPlayerAircraft()->IsOn(AircraftClass::StrgSel))
		{
			SimDriver.GetPlayerAircraft()->ClearAPFlag(AircraftClass::StrgSel);
			SimDriver.GetPlayerAircraft()->ClearAPFlag(AircraftClass::RollHold);
			SimDriver.GetPlayerAircraft()->SetAPFlag(AircraftClass::HDGSel);
		}
		//up position
		else
		{
			SimDriver.GetPlayerAircraft()->ClearAPFlag(AircraftClass::HDGSel);
			SimDriver.GetPlayerAircraft()->ClearAPFlag(AircraftClass::StrgSel);
			SimDriver.GetPlayerAircraft()->SetAPFlag(AircraftClass::RollHold);
		}
		if((SimDriver.GetPlayerAircraft()->IsOn(AircraftClass::AttHold)) || (SimDriver.GetPlayerAircraft()->IsOn(AircraftClass::AltHold)))
		{
			if(SimDriver.GetPlayerAircraft()->IsOn(AircraftClass::RollHold))
			{
				SimDriver.GetPlayerAircraft()->SetNewRoll();
			//	SimDriver.GetPlayerAircraft()->SetNewPitch();  // MD -- 20031109: no, just set roll here.
			}
		}
		if (OTWDriver.GetVirtualCockpit())
		{
			// Left switch Middle position
			if(SimDriver.GetPlayerAircraft()->IsOn(AircraftClass::RollHold))
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_LT_AP_SW, 4);
			// Left switch down position
			else if(SimDriver.GetPlayerAircraft()->IsOn(AircraftClass::StrgSel))
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_LT_AP_SW, 1);
			// Left switch up position
			else
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_LT_AP_SW, 2);
		}
	}
}

void SimLeftAPUp(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
	//This is the left switch, going to the HDG SEL position.
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) {
		SimDriver.GetPlayerAircraft()->ClearAPFlag(AircraftClass::StrgSel);
		SimDriver.GetPlayerAircraft()->ClearAPFlag(AircraftClass::RollHold);
		SimDriver.GetPlayerAircraft()->SetAPFlag(AircraftClass::HDGSel);

		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_LT_AP_SW, 2);
	}
}

void SimLeftAPMid(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
	//This is the left switch, going to the HDG SEL position.
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) {
		SimDriver.GetPlayerAircraft()->ClearAPFlag(AircraftClass::StrgSel);
		SimDriver.GetPlayerAircraft()->ClearAPFlag(AircraftClass::HDGSel);
		SimDriver.GetPlayerAircraft()->SetAPFlag(AircraftClass::RollHold);
		if((SimDriver.GetPlayerAircraft()->IsOn(AircraftClass::AttHold)) || (SimDriver.GetPlayerAircraft()->IsOn(AircraftClass::AltHold))) {
			SimDriver.GetPlayerAircraft()->SetNewRoll();
			// MD -- 20031108: fixing AP ATT HLD modes.  Roll switch should set roll not pitch.
			// SimDriver.GetPlayerAircraft()->SetNewPitch();
		}

		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_LT_AP_SW, 1);
	}
}

void SimLeftAPDown(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
	//This is the left switch, going to the HDG SEL position.
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) {
		SimDriver.GetPlayerAircraft()->ClearAPFlag(AircraftClass::RollHold);
		SimDriver.GetPlayerAircraft()->ClearAPFlag(AircraftClass::HDGSel);
		SimDriver.GetPlayerAircraft()->SetAPFlag(AircraftClass::StrgSel);

		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_LT_AP_SW, 4);
	}
}

void SimAPOverride(unsigned long val, int state, void *)
{
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP))
	{
		if(state & KEY_DOWN)
		{
		    // JPO - save AP type.
			//MI only if it's != APOFF
			if(SimDriver.GetPlayerAircraft()->AutopilotType() != AircraftClass::APOff)
				SimDriver.GetPlayerAircraft()->lastapType = SimDriver.GetPlayerAircraft()->AutopilotType();
			if(SimDriver.GetPlayerAircraft()->autopilotType != AircraftClass::APOff)
				SimDriver.GetPlayerAircraft()->SetAutopilot(AircraftClass::APOff);
		    SimDriver.GetPlayerAircraft()->SetAPFlag(AircraftClass::Override);
		}
		else
		{
			SimDriver.GetPlayerAircraft()->SetAutopilot(SimDriver.GetPlayerAircraft()->lastapType);
		    SimDriver.GetPlayerAircraft()->ClearAPFlag(AircraftClass::Override);
		    if (SimDriver.GetPlayerAircraft()->lastapType != AircraftClass::LantirnAP) { // JPO - lantirn stays
				SimDriver.GetPlayerAircraft()->SetAPParameters();
				if(SimDriver.GetPlayerAircraft()->IsOn(AircraftClass::RollHold))
					SimDriver.GetPlayerAircraft()->SetNewRoll();
				if(SimDriver.GetPlayerAircraft()->IsOn(AircraftClass::AttHold))
					SimDriver.GetPlayerAircraft()->SetNewPitch();
				if(SimDriver.GetPlayerAircraft()->IsOn(AircraftClass::AltHold))
					SimDriver.GetPlayerAircraft()->SetNewAlt();
		    }
		}
	}
}

void SimToggleTFR (unsigned long val, int state, void *)
{
    if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) {
		if (SimDriver.GetPlayerAircraft()->AutopilotType() == AircraftClass::APOff) {
			if (theLantirn->GetTFRMode() == LantirnClass::TFR_STBY)
			theLantirn->SetTFRMode(LantirnClass::TFR_NORM);
			SimDriver.GetPlayerAircraft()->SetAutopilot(AircraftClass::LantirnAP);
			//MI turn off any other AP modes
			SimDriver.GetPlayerAircraft()->ClearAPFlag(AircraftClass::AltHold);
			SimDriver.GetPlayerAircraft()->ClearAPFlag(AircraftClass::AttHold);
			//MI update our lastAP state, for operating the RF Switch
			SimDriver.GetPlayerAircraft()->lastapType = AircraftClass::LantirnAP;
		}
		else 
		{
			// MD -- 2003127: adding a check to put the TFR state back into standby
			if (theLantirn->GetTFRMode() != LantirnClass::TFR_STBY)
				theLantirn->SetTFRMode(LantirnClass::TFR_STBY);
			SimDriver.GetPlayerAircraft()->SetAutopilot(AircraftClass::APOff);
			SimDriver.GetPlayerAircraft()->lastapType = AircraftClass::APOff;
		}
    }
}
void SimWarnReset(unsigned long val, int state, void *)
{
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		SimDriver.GetPlayerAircraft()->mFaults->ClearWarnReset();
		SimDriver.GetPlayerAircraft()->mFaults->SetManWarnReset();
		TheHud->ResetMaxG();
		if (OTWDriver.GetVirtualCockpit())
		{
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_ICP_DRIFTCO, 4);
			// Delay....then off = momentary switch
			//OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_ICP_DRIFTCO, 0);
		}
	else
		{
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_ICP_DRIFTCO, 1);
		}
	}
}
void SimReticleSwitch(unsigned long val, int state, void *)
{
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		if(TheHud->WhichMode == 0)	// Off
		{
			TheHud->WhichMode = 1;	// PRI
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_HUD_RETICLE, 2);
		}
		else if(TheHud->WhichMode == 1)	// PRI
		{
			TheHud->WhichMode = 2;	// STBY
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_HUD_RETICLE, 4);
		}
		else
		{
			TheHud->WhichMode = 0;	// Off
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_HUD_RETICLE, 1);
		}
	}
}

// MD -- 20031026: adding commands to set the DEPR RET switch directly which
// should help cockpit builders map this function to a physical switch more easily.

void SimReticlePri(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		TheHud->WhichMode = 1;	// PRI
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_HUD_RETICLE, 2);
	}
}

void SimReticleStby(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		TheHud->WhichMode = 2;	// STBY
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_HUD_RETICLE, 4);
	}
}

void SimReticleOff(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		TheHud->WhichMode = 0;	// OFF
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_HUD_RETICLE, 1);
	}
}

void SimInteriorLight(unsigned long val, int state, void *)
{
	//sfr: i removed the power on calls, so we override old lighting system
	AircraftClass *ac = SimDriver.GetPlayerAircraft();
	if (!ac) { return; }

  if (ac->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) 
	{
		switch (ac->GetInteriorLight()) 
		{
			default:
			case AircraftClass::LT_OFF:
				ac->SetInteriorLight(AircraftClass::LT_LOW);
				break;
			case AircraftClass::LT_LOW:
				ac->SetInteriorLight(AircraftClass::LT_NORMAL);
				break;
			case AircraftClass::LT_NORMAL:
				ac->SetInteriorLight(AircraftClass::LT_OFF);
				break;
		}

		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_INTERIOR_LITE, 1<<SimDriver.GetPlayerAircraft()->GetInteriorLight());

		TheColorBank.PitLightLevel = (int)ac->GetInteriorLight();
		if (OTWDriver.pCockpitManager != NULL)
		{
			OTWDriver.pCockpitManager->UpdatePalette();
		}
  }
}

void SimInstrumentLight(unsigned long val, int state, void *)
{
	//sfr: same here
  if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) 
	{
		switch (SimDriver.GetPlayerAircraft()->GetInstrumentLight()) 
		{
			default:
			case AircraftClass::LT_OFF:
				SimDriver.GetPlayerAircraft()->SetInstrumentLight(AircraftClass::LT_LOW);
				break;
			case AircraftClass::LT_LOW:
				SimDriver.GetPlayerAircraft()->SetInstrumentLight(AircraftClass::LT_NORMAL);
				break;
			case AircraftClass::LT_NORMAL:
				SimDriver.GetPlayerAircraft()->SetInstrumentLight(AircraftClass::LT_OFF);
				break;
		}

		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_INSTR_LITE, 1<<SimDriver.GetPlayerAircraft()->GetInstrumentLight());

		TheColorBank.PitLightLevel = (int)SimDriver.GetPlayerAircraft()->GetInstrumentLight();
		//sfr: this is messing light system, interior has its own switch
		//SimDriver.GetPlayerAircraft()->SetInteriorLight(AircraftClass::LT_OFF);

		if (OTWDriver.pCockpitManager != NULL)
		{
			OTWDriver.pCockpitManager->UpdatePalette();
		}
  }
}
void SimSpotLight(unsigned long val, int state, void *)
{
  if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) 
	{
		switch (SimDriver.GetPlayerAircraft()->GetSpotLight()) 
		{
			default:
			case AircraftClass::LT_OFF:
				SimDriver.GetPlayerAircraft()->SetSpotLight(AircraftClass::LT_LOW);
				SimDriver.GetPlayerAircraft()->PowerOn(AircraftClass::SpotLightPower);
				break;
			case AircraftClass::LT_LOW:
				SimDriver.GetPlayerAircraft()->SetSpotLight(AircraftClass::LT_NORMAL);
				SimDriver.GetPlayerAircraft()->PowerOn(AircraftClass::SpotLightPower);
				break;
			case AircraftClass::LT_NORMAL:
				SimDriver.GetPlayerAircraft()->SetSpotLight(AircraftClass::LT_OFF);
				SimDriver.GetPlayerAircraft()->PowerOff(AircraftClass::SpotLightPower);
				break;
		}

		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_SPOT_LITE, 1<<SimDriver.GetPlayerAircraft()->GetSpotLight());
  }
}
//MI TMS switch
void SimTMSUp(unsigned long val, int state, void *){
	AircraftClass *pac = SimDriver.GetPlayerAircraft();
	if(pac && pac->IsSetFlag(MOTION_OWNSHIP)){
		if(state & KEY_DOWN){
			if(g_bRealisticAvionics){
				RadarDopplerClass* theRadar = (RadarDopplerClass*)FindSensor(pac, SensorClass::Radar);
				bool HasMavs = FALSE;
				LaserPodClass* laserPod = (LaserPodClass* )FindLaserPod (pac);
				MaverickDisplayClass* mavDisplay = NULL;
				HarmTargetingPod *theHTS = (HarmTargetingPod*)FindSensor(pac, SensorClass::HTS);
				if (pac->Sms->curWeaponType == wtAgm65 && pac->Sms->curWeapon){
					HasMavs = TRUE;
					mavDisplay = (MaverickDisplayClass*)((MissileClass*)pac->Sms->curWeapon.get())->display;
				}
				
				if (pac->FCC && pac->FCC->IsSOI){
					pac->FCC->HSDDesignate = 1;
					return;
				}
				else if (theRadar && theRadar->IsSOI()){
					//ACM Modes
					if (
						theRadar->GetRadarMode() == RadarClass::ACM_30x20 ||
						theRadar->GetRadarMode() == RadarClass::ACM_SLEW ||
						theRadar->GetRadarMode() == RadarClass::ACM_BORE ||
						theRadar->GetRadarMode() == RadarClass::ACM_10x60
					){
						pac->FCC->dropTrackCmd = FALSE;
						theRadar->SelectACMBore();
					}
					else if(
						theRadar->GetRadarMode() == RadarClass::RWS ||
						theRadar->GetRadarMode() == RadarClass::LRS ||
						theRadar->GetRadarMode() == RadarClass::VS ||
						theRadar->GetRadarMode() == RadarClass::TWS ||
						theRadar->GetRadarMode() == RadarClass::SAM ||
						theRadar->IsAG())
					{
						pac->FCC->designateCmd = TRUE;
					}
				}
				else if(TheHud && TheHud->IsSOI()){
					pac->FCC->designateCmd = TRUE;
				}
				else if(HasMavs && mavDisplay && mavDisplay->IsSOI())
				{
					if (
						pac->Sms->curWeapon && !((MissileClass*)pac->Sms->curWeapon.get())->Covered &&
						pac->Sms->MavCoolTimer < 0.0F
					){
						pac->FCC->designateCmd = TRUE;
					}
				}
				else if (laserPod && laserPod->IsSOI()){
					if (pac->FCC->preDesignate)
					{
						pac->FCC->SetLastDesignate();
						pac->FCC->preDesignate = FALSE;
					}
					pac->FCC->designateCmd = TRUE;
				}
				else if (theHTS){
					pac->FCC->designateCmd = TRUE;
				}
			}
		}
		else {
			pac->FCC->designateCmd = FALSE;
			pac->FCC->dropTrackCmd = FALSE;
			pac->FCC->HSDDesignate = 0;
		}
	}
} 
void SimTMSLeft(unsigned long val, int state, void *)
{
	AircraftClass *pac = SimDriver.GetPlayerAircraft();
	bool HasMavs;
	if (pac && pac->IsSetFlag(MOTION_OWNSHIP)){ 
		if(state & KEY_DOWN){
			if(g_bRealisticAvionics && !g_bMLU){
				RadarDopplerClass* theRadar = (RadarDopplerClass*)FindSensor(pac, SensorClass::Radar);
				LaserPodClass *laserPod = (LaserPodClass* )FindLaserPod(pac);
				MaverickDisplayClass* mavDisplay = NULL;
				if (pac->Sms->curWeaponType == wtAgm65 && pac->Sms->curWeapon){
					HasMavs = TRUE;
					mavDisplay = (MaverickDisplayClass*)((MissileClass*)pac->Sms->curWeapon.get())->display;
				}
				if(theRadar && theRadar->IsSOI()){
					if(theRadar->DrawRCR){
						theRadar->DrawRCR = FALSE;
					}
					//Cobra
					if(g_bIFF){
						SimIFFIn(0, KEY_DOWN, NULL);
					}
					else{
						theRadar->DrawRCR = TRUE;
					}
				}
				
				//laserpod, toggle BHOT
				if(laserPod && laserPod->IsSOI()){
					laserPod->TogglePolarity();
				}
				else if (mavDisplay && mavDisplay->IsSOI()){
					MissileClass *m = static_cast<MissileClass*>(pac->Sms->GetCurrentWeapon());
					m->HOC = !m->HOC;
				}
			}
			//else if(g_bRealisticAvionics && g_bIFF)
				//SimIFFIn(0, KEY_DOWN, NULL);
		}
		//else if(g_bRealisticAvionics && g_bMLU)
			//SimIFFIn(0, 0, NULL);
		else if (g_bRealisticAvionics && g_bIFF){
			SimIFFIn(0,0,NULL);
		}
	}
}
void SimTMSDown(unsigned long val, int state, void *)
{
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP))
	{
		if(state & KEY_DOWN)
		{
			if(g_bRealisticAvionics)
			{
				RadarDopplerClass* theRadar = (RadarDopplerClass*)FindSensor(SimDriver.GetPlayerAircraft(), SensorClass::Radar);
				if(theRadar && theRadar->IsSOI())
				{
					//ACM Modes
					if(theRadar->GetRadarMode() == RadarClass::ACM_30x20 ||
						theRadar->GetRadarMode() == RadarClass::ACM_SLEW ||
						theRadar->GetRadarMode() == RadarClass::ACM_BORE ||
						theRadar->GetRadarMode() == RadarClass::ACM_10x60)
					{
						//First, drop our track
						SimDriver.GetPlayerAircraft()->FCC->dropTrackCmd = FALSE;//Changed from TRUE
						theRadar->SelectACMVertical();//Cobra BMS bug fix 01/29/05
					}
					else if(theRadar->GetRadarMode() == RadarClass::ACM_30x20 ||
						theRadar->GetRadarMode() == RadarClass::ACM_SLEW ||
						theRadar->GetRadarMode() == RadarClass::ACM_BORE)
					{
						theRadar->SelectACMVertical();
					}
					else if(theRadar->GetRadarMode() == RadarClass::TWS)
					{
						// MD -- 20040118: revised TWS mode function -- only select RWS if the TWS track directory is empty
						if(theRadar->CurrentTarget() || theRadar->twsTrackDirectory())
						{
							//First, drop our track
							SimDriver.GetPlayerAircraft()->FCC->dropTrackCmd = TRUE;
						}
						else // if(!theRadar->CurrentTarget())  
						{
							theRadar->SelectRWS();
						}
					}
					else if(theRadar->GetRadarMode() == RadarClass::SAM && theRadar->IsSet(RadarDopplerClass::STTingTarget))
						theRadar->SelectSAM();
					else
						SimDriver.GetPlayerAircraft()->FCC->dropTrackCmd = TRUE;
				}
				if(SimDriver.GetPlayerAircraft()->FCC && SimDriver.GetPlayerAircraft()->FCC->IsSOI)
				{
					SimDriver.GetPlayerAircraft()->FCC->HSDDesignate = -1;
					return;
				}
				else
					SimDriver.GetPlayerAircraft()->FCC->dropTrackCmd = TRUE;
			}
		}
		else
		{
			SimDriver.GetPlayerAircraft()->FCC->dropTrackCmd = FALSE;
			SimDriver.GetPlayerAircraft()->FCC->HSDDesignate = 0;
		}
	}
}
void SimTMSRight(unsigned long val, int state, void *)
{
static VU_TIME tmstimer = 0;
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP))
	{
		if(g_bMLU)
		{
			if(state & KEY_DOWN && (!g_bMLU || !tmstimer || SimLibElapsedTime - tmstimer < 500))
			{
				if (!tmstimer)  tmstimer = SimLibElapsedTime;
			}
			else
			{
				if (SimLibElapsedTime - tmstimer < 500 || !g_bMLU)
				{
					if(g_bRealisticAvionics)
					{
						RadarDopplerClass* theRadar = (RadarDopplerClass*)FindSensor(SimDriver.GetPlayerAircraft(), SensorClass::Radar);
						if(theRadar)
						{
							//ACM Modes
							if(theRadar->GetRadarMode() == RadarClass::ACM_30x20 ||
								theRadar->GetRadarMode() == RadarClass::ACM_SLEW ||
								theRadar->GetRadarMode() == RadarClass::ACM_BORE ||
								theRadar->GetRadarMode() == RadarClass::ACM_10x60)
							{
								SimDriver.GetPlayerAircraft()->FCC->dropTrackCmd = FALSE;
								theRadar->SelectACM30x20();
								theRadar->SetEmitting(TRUE);
							}
							else if(theRadar->GetRadarMode() == RadarClass::RWS ||
									theRadar->GetRadarMode() == RadarClass::LRS ||
									theRadar->GetRadarMode() == RadarClass::VS ||
									theRadar->GetRadarMode() == RadarClass::SAM)
								theRadar->SelectTWS();
							else if(theRadar->GetRadarMode() == RadarClass::TWS)
								theRadar->NextTarget();
						}
					}
				}
				else 
				{
						RadarDopplerClass* theRadar = (RadarDopplerClass*)FindSensor(SimDriver.GetPlayerAircraft(), SensorClass::Radar);
						if(theRadar)
						{
							//ACM Modes
							if(theRadar->GetRadarMode() == RadarClass::ACM_30x20 ||
								theRadar->GetRadarMode() == RadarClass::ACM_SLEW ||
								theRadar->GetRadarMode() == RadarClass::ACM_BORE ||
								theRadar->GetRadarMode() == RadarClass::ACM_10x60)
							{
								SimDriver.GetPlayerAircraft()->FCC->dropTrackCmd = FALSE;
								theRadar->SelectACM30x20();
								theRadar->SetEmitting(TRUE);
							}
							else if(theRadar->GetRadarMode() == RadarClass::RWS ||
									theRadar->GetRadarMode() == RadarClass::LRS ||
									theRadar->GetRadarMode() == RadarClass::SAM)
								theRadar->SelectTWS();
							else if(theRadar->GetRadarMode() == RadarClass::TWS)
								theRadar->SelectRWS();
						}
				}
			tmstimer = 0;
			SimDriver.GetPlayerAircraft()->FCC->dropTrackCmd = FALSE;
			}
		}
		else
		{
			if(state & KEY_DOWN)
			{
				if(g_bRealisticAvionics)
				{
					RadarDopplerClass* theRadar = (RadarDopplerClass*)FindSensor(SimDriver.GetPlayerAircraft(), SensorClass::Radar);
					if(theRadar)
					{
						//ACM Modes
						if(theRadar->GetRadarMode() == RadarClass::ACM_30x20 ||
							theRadar->GetRadarMode() == RadarClass::ACM_SLEW ||
							theRadar->GetRadarMode() == RadarClass::ACM_BORE ||
							theRadar->GetRadarMode() == RadarClass::ACM_10x60)
						{
							SimDriver.GetPlayerAircraft()->FCC->dropTrackCmd = FALSE;
							theRadar->SelectACM30x20();
							theRadar->SetEmitting(TRUE);
						}
						else if(theRadar->GetRadarMode() == RadarClass::RWS ||
								theRadar->GetRadarMode() == RadarClass::LRS ||
								theRadar->GetRadarMode() == RadarClass::VS ||
								theRadar->GetRadarMode() == RadarClass::SAM)
							theRadar->SelectTWS();
						else if(theRadar->GetRadarMode() == RadarClass::TWS)
							theRadar->NextTarget();
					}
				}
			}
			else
			{
				SimDriver.GetPlayerAircraft()->FCC->dropTrackCmd = FALSE;
			}
		}
	}
}
void SimSeatArm(unsigned long val, int state, void *)
{
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{ 
		if(g_bRealisticAvionics)
		{
			SimDriver.GetPlayerAircraft()->StepSeatArm();
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask(COMP_3DPIT_SEAT_ARM, SimDriver.GetPlayerAircraft()->SeatArmed+1);
		}
	}
}


// MD -- 20031130: adding commands to place the seat arm lever directly.

void SimSeatOn(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{ 
		if(g_bRealisticAvionics)
		{
			SimDriver.GetPlayerAircraft()->SeatArmed = TRUE;
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask(COMP_3DPIT_SEAT_ARM, SimDriver.GetPlayerAircraft()->SeatArmed+1);
		}
	}
}

void SimSeatOff(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{ 
		if(g_bRealisticAvionics)
		{
			SimDriver.GetPlayerAircraft()->SeatArmed = FALSE;
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask(COMP_3DPIT_SEAT_ARM, SimDriver.GetPlayerAircraft()->SeatArmed+1);
		}
	}
}

void SimEWSRWRPower(unsigned long val, int state, void *)
{
	if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{ 
		SimDriver.GetPlayerAircraft()->PowerToggle(AircraftClass::EWSRWRPower);
		if (OTWDriver.GetVirtualCockpit())
			//OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_EWS_RWR_PWR, SimDriver.GetPlayerAircraft()->HasPower(AircraftClass::EWSRWRPower)+1);
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_EWS_RWR_PWR, 1<<SimDriver.GetPlayerAircraft()->HasPower(AircraftClass::EWSRWRPower));
	}
}

// MD -- 20031128: adding commands to place CMDS RWR power switch directly.

void SimEWSRWROn(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
	if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{ 
		SimDriver.GetPlayerAircraft()->PowerOn(AircraftClass::EWSRWRPower);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_EWS_RWR_PWR, 2);
	}
}

void SimEWSRWROff(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
	if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{ 
		SimDriver.GetPlayerAircraft()->PowerOff(AircraftClass::EWSRWRPower);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_EWS_RWR_PWR, 1);
	}
}


void SimEWSJammerPower(unsigned long val, int state, void *)
{
	if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{ 
		SimDriver.GetPlayerAircraft()->PowerToggle(AircraftClass::EWSJammerPower);
		if (OTWDriver.GetVirtualCockpit())
			//OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_EWS_JMR_PWR, SimDriver.GetPlayerAircraft()->HasPower(AircraftClass::EWSJammerPower)+1);
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_EWS_JMR_PWR, 1<<SimDriver.GetPlayerAircraft()->HasPower(AircraftClass::EWSJammerPower));
	}
}

// MD -- 20031128: adding commands to place CMDS Jammer power switch directly.

void SimEWSJammerOn(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
	if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{ 
		SimDriver.GetPlayerAircraft()->PowerOn(AircraftClass::EWSJammerPower);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_EWS_JMR_PWR, 2);
	}
}

void SimEWSJammerOff(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
	if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{ 
		SimDriver.GetPlayerAircraft()->PowerOff(AircraftClass::EWSJammerPower);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_EWS_JMR_PWR, 1);
	}
}

void SimEWSChaffPower(unsigned long val, int state, void *)
{
	if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{ 
		SimDriver.GetPlayerAircraft()->PowerToggle(AircraftClass::EWSChaffPower);
		if (OTWDriver.GetVirtualCockpit())
			//OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_EWS_CHAFF_PWR, SimDriver.GetPlayerAircraft()->HasPower(AircraftClass::EWSChaffPower)+1);
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_EWS_CHAFF_PWR, 1<<SimDriver.GetPlayerAircraft()->HasPower(AircraftClass::EWSChaffPower));
	}
}

// MD -- 20031128: adding commands to place CMDS Chaff power switch directly.

void SimEWSChaffOn(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
	if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{ 
		SimDriver.GetPlayerAircraft()->PowerOn(AircraftClass::EWSChaffPower);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_EWS_CHAFF_PWR, 2);
	}
}

void SimEWSChaffOff(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
	if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{ 
		SimDriver.GetPlayerAircraft()->PowerOff(AircraftClass::EWSChaffPower);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_EWS_CHAFF_PWR, 1);
	}
}

void SimEWSFlarePower(unsigned long val, int state, void *)
{
	if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{ 
		SimDriver.GetPlayerAircraft()->PowerToggle(AircraftClass::EWSFlarePower);
		if (OTWDriver.GetVirtualCockpit())
			//OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_EWS_FLARE_PWR, SimDriver.GetPlayerAircraft()->HasPower(AircraftClass::EWSFlarePower)+1);
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_EWS_FLARE_PWR, 1<<SimDriver.GetPlayerAircraft()->HasPower(AircraftClass::EWSFlarePower));
	}
}

// MD -- 20031128: adding commands to place CMDS Chaff power switch directly.

void SimEWSFlareOn(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
	if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{ 
		SimDriver.GetPlayerAircraft()->PowerOn(AircraftClass::EWSFlarePower);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_EWS_FLARE_PWR, 2);
	}
}

void SimEWSFlareOff(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
	if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{ 
		SimDriver.GetPlayerAircraft()->PowerOff(AircraftClass::EWSFlarePower);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_EWS_FLARE_PWR, 1);
	}
}

void SimEWSPGMInc(unsigned long val, int state, void *)
{
  if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
  { 
		SimDriver.GetPlayerAircraft()->IncEWSPGM();
		int val = SimDriver.GetPlayerAircraft()->EWSPGM()+ 1;
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_EWS_MODE, val);
  }
}
void SimEWSPGMDec(unsigned long val, int state, void *)
{
  if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
  { 
		SimDriver.GetPlayerAircraft()->DecEWSPGM();
		int val = SimDriver.GetPlayerAircraft()->EWSPGM()+ 1;
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_EWS_MODE, val);
  }
}

// MD -- 20031025: adding functions to set the EWS mode knob directly which should make it
// easier for cockpit builders to map the EWS to a physical rotary knob.  This may also be
// interesting to HOTAS programmers since it will avoid the need to "track" the position
// of the knob in a stick program.

void SimEWSModeOff(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
  if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
  { 
		SimDriver.GetPlayerAircraft()->SetPGM(AircraftClass::EWSPGMSwitch::Off);
		PlayerRwrClass* theRwr = (PlayerRwrClass*)FindSensor(SimDriver.GetPlayerAircraft(), SensorClass::RWR);
		if(theRwr)
		{
			theRwr->InEWSLoop = FALSE;
			theRwr->ReleaseManual = FALSE;
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_EWS_MODE, 1);
		}
	} 
}

void SimEWSModeStby(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
    if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
    { 
			SimDriver.GetPlayerAircraft()->SetPGM(AircraftClass::EWSPGMSwitch::Stby);
			PlayerRwrClass* theRwr = (PlayerRwrClass*)FindSensor(SimDriver.GetPlayerAircraft(), SensorClass::RWR);
			if(theRwr)
			{
				theRwr->InEWSLoop = FALSE;
				theRwr->ReleaseManual = FALSE;
				if (OTWDriver.GetVirtualCockpit())
					OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_EWS_MODE, 2);
			}
    }
}

void SimEWSModeMan(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
    if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
    { 
		if ((SimDriver.GetPlayerAircraft()->EWSPGM() == AircraftClass::EWSPGMSwitch::Off) ||
			(SimDriver.GetPlayerAircraft()->EWSPGM() == AircraftClass::EWSPGMSwitch::Stby)) {
			PlayerRwrClass* theRwr = (PlayerRwrClass*)FindSensor(SimDriver.GetPlayerAircraft(), SensorClass::RWR);
			if(theRwr)
			{	
				theRwr->InEWSLoop = FALSE;
				theRwr->ReleaseManual = FALSE;
				if (OTWDriver.GetVirtualCockpit())
					OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_EWS_MODE, 4);
			}
		}
		SimDriver.GetPlayerAircraft()->SetPGM(AircraftClass::EWSPGMSwitch::Man);
    }
}

void SimEWSModeSemi(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
  if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
  { 
		SimDriver.GetPlayerAircraft()->SetPGM(AircraftClass::EWSPGMSwitch::Semi);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_EWS_MODE, 8);
  }
}

void SimEWSModeAuto(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
  if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
  { 
		SimDriver.GetPlayerAircraft()->SetPGM(AircraftClass::EWSPGMSwitch::Auto);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_EWS_MODE, 16);
  }
}

void SimEWSProgInc(unsigned long val, int state, void *)
{
  if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
  { 
		SimDriver.GetPlayerAircraft()->IncEWSProg();
		int val = SimDriver.GetPlayerAircraft()->EWSProgNum+1;
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_EWS_PROG, val);
  }
}
void SimEWSProgDec(unsigned long val, int state, void *)
{
  if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
  { 
		SimDriver.GetPlayerAircraft()->DecEWSProg();
		int val = SimDriver.GetPlayerAircraft()->EWSProgNum+1;
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_EWS_PROG, val);
  }
}

// MD -- 20031025: adding functions to set the EWS program knob directly which should make it
// easier for cockpit builders to map the EWS to a physical rotary knob.  This may also be
// interesting to HOTAS programmers since it will avoid the need to "track" the position
// of the knob in a stick program.

void SimEWSProgOne(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
  if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
  {
		PlayerRwrClass* theRwr = (PlayerRwrClass*)FindSensor(SimDriver.GetPlayerAircraft(), SensorClass::RWR);
		if(theRwr)
		{
			theRwr->InEWSLoop = FALSE;
			theRwr->ReleaseManual = FALSE;
		}
		SimDriver.GetPlayerAircraft()->SetEWSProg(0);  // First knob position is zero
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_EWS_PROG, 1);
  }
}

void SimEWSProgTwo(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
  if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
  { 
		PlayerRwrClass* theRwr = (PlayerRwrClass*)FindSensor(SimDriver.GetPlayerAircraft(), SensorClass::RWR);
		if(theRwr)
		{
			theRwr->InEWSLoop = FALSE;
			theRwr->ReleaseManual = FALSE;
		}
		SimDriver.GetPlayerAircraft()->SetEWSProg(1);  // Second knob position is one
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_EWS_PROG, 2);
  }
}

void SimEWSProgThree(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
  if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
  { 
		PlayerRwrClass* theRwr = (PlayerRwrClass*)FindSensor(SimDriver.GetPlayerAircraft(), SensorClass::RWR);
		if(theRwr)
		{
			theRwr->InEWSLoop = FALSE;
			theRwr->ReleaseManual = FALSE;
		}
		SimDriver.GetPlayerAircraft()->SetEWSProg(2);  // Third knob position is two
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_EWS_PROG, 4);
  }
}

void SimEWSProgFour(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
  if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
  { 
		PlayerRwrClass* theRwr = (PlayerRwrClass*)FindSensor(SimDriver.GetPlayerAircraft(), SensorClass::RWR);
		if(theRwr)
		{
			theRwr->InEWSLoop = FALSE;
			theRwr->ReleaseManual = FALSE;
		}
		SimDriver.GetPlayerAircraft()->SetEWSProg(3);  // Fourth knob position is three
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_EWS_PROG, 8);
  }
}

void SimMainPowerInc(unsigned long val, int state, void *)
{
    if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
    { 
			SimDriver.GetPlayerAircraft()->IncMainPower();
			if (OTWDriver.GetVirtualCockpit())
			if (OTWDriver.GetVirtualCockpit())
				//OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_MAIN_PWR, SimDriver.GetPlayerAircraft()->MainPowerOn()+1);
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_MAIN_PWR, 1<<SimDriver.GetPlayerAircraft()->MainPowerOn());
    }
}
void SimMainPowerDec(unsigned long val, int state, void *)
{
    if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
    { 
			SimDriver.GetPlayerAircraft()->DecMainPower();
			if (OTWDriver.GetVirtualCockpit())
				//OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_MAIN_PWR, SimDriver.GetPlayerAircraft()->MainPowerOn()+1);
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_MAIN_PWR, 1<<SimDriver.GetPlayerAircraft()->MainPowerOn());
    }
}

// MD -- 20031024: Adding commands to set the Main Power position explicitly to
// make it easier for cockpit builders to map the commands to a physical switch.

void SimMainPowerOff(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
    if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
    { 
			SimDriver.GetPlayerAircraft()->SetMainPower(AircraftClass::MainPowerOff);
			if(!g_bINS)
			{
				SimDriver.GetPlayerAircraft()->INSOff(AircraftClass::INS_ADI_OFF_IN);
				SimDriver.GetPlayerAircraft()->INSOff(AircraftClass::INS_ADI_AUX_IN);;
				SimDriver.GetPlayerAircraft()->INSOff(AircraftClass::INS_HUD_STUFF);
				SimDriver.GetPlayerAircraft()->INSOff(AircraftClass::INS_HSI_OFF_IN);
				SimDriver.GetPlayerAircraft()->INSOff(AircraftClass::INS_HSD_STUFF);
				SimDriver.GetPlayerAircraft()->INSOff(AircraftClass::BUP_ADI_OFF_IN);
				SimDriver.GetPlayerAircraft()->GSValid = FALSE;
				SimDriver.GetPlayerAircraft()->LOCValid = FALSE;
			}
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_MAIN_PWR, 1);
    }
}

void SimMainPowerBatt(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
    if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
    { 
			SimDriver.GetPlayerAircraft()->SetMainPower(AircraftClass::MainPowerBatt);
			if(!g_bINS)
			{
				SimDriver.GetPlayerAircraft()->INSOn(AircraftClass::INS_ADI_OFF_IN);
				SimDriver.GetPlayerAircraft()->INSOn(AircraftClass::INS_ADI_AUX_IN);;
				SimDriver.GetPlayerAircraft()->INSOn(AircraftClass::INS_HUD_STUFF);
				SimDriver.GetPlayerAircraft()->INSOn(AircraftClass::INS_HSI_OFF_IN);
				SimDriver.GetPlayerAircraft()->INSOn(AircraftClass::INS_HSD_STUFF);
				SimDriver.GetPlayerAircraft()->INSOn(AircraftClass::BUP_ADI_OFF_IN);
				SimDriver.GetPlayerAircraft()->GSValid = TRUE;
				SimDriver.GetPlayerAircraft()->LOCValid = TRUE;
			}
 			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_MAIN_PWR, 2);
   }
}

void SimMainPowerMain(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;
    if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
    { 
			SimDriver.GetPlayerAircraft()->SetMainPower(AircraftClass::MainPowerMain);
			// Probably overkill to repeat this here but just in case someone's cockpit or
			// HOTAS programming has the power switch go direct from "off" to "main"...
			if(!g_bINS)
			{
				SimDriver.GetPlayerAircraft()->INSOn(AircraftClass::INS_ADI_OFF_IN);
				SimDriver.GetPlayerAircraft()->INSOn(AircraftClass::INS_ADI_AUX_IN);;
				SimDriver.GetPlayerAircraft()->INSOn(AircraftClass::INS_HUD_STUFF);
				SimDriver.GetPlayerAircraft()->INSOn(AircraftClass::INS_HSI_OFF_IN);
				SimDriver.GetPlayerAircraft()->INSOn(AircraftClass::INS_HSD_STUFF);
				SimDriver.GetPlayerAircraft()->INSOn(AircraftClass::BUP_ADI_OFF_IN);
				SimDriver.GetPlayerAircraft()->GSValid = TRUE;
				SimDriver.GetPlayerAircraft()->LOCValid = TRUE;
			}
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_MAIN_PWR, 4);
    }
}

void SimInhibitVMS(unsigned long val, int state, void *)
{
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{ 
		if(g_bRealisticAvionics)
		{
			SimDriver.GetPlayerAircraft()->ToggleBetty();
			if (OTWDriver.GetVirtualCockpit())
			{
				if (SimDriver.GetPlayerAircraft()->playBetty)
					OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_VMS_PWR, 1);
				else
					OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_VMS_PWR, 2);
			}
		}
	}
}

// MD -- 20031129: adding commands to place the VMS inhibit switch directly

void SimVMSOn(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;

	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{ 
		SimDriver.GetPlayerAircraft()->playBetty = TRUE;
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_VMS_PWR, 2);
	}
}

void SimVMSOff(unsigned long val, int state, void *)
{
	if (!g_bRealisticAvionics)  // not intended for anything other than realistic mode
		return;

	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{ 
		SimDriver.GetPlayerAircraft()->playBetty = FALSE;
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_VMS_PWR, 1);
	}
}

void SimRFSwitch(unsigned long val, int state, void *)
{
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{ 
		if(state & KEY_DOWN)
		{
			if(g_bRealisticAvionics)
			{
				RadarDopplerClass* theRadar = (RadarDopplerClass*)FindSensor(SimDriver.GetPlayerAircraft(), SensorClass::Radar);
				if(theRadar)
				{
					if(SimDriver.GetPlayerAircraft()->RFState == 0)
					{
						//QUIET --> no Radar
						SimDriver.GetPlayerAircraft()->RFState = 1;
						theRadar->SetEmitting(FALSE);
						if (OTWDriver.GetVirtualCockpit())
							OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_RF_QUIET, 1);
					}
					else if(SimDriver.GetPlayerAircraft()->RFState == 1)
					{
						//store our current AP mode
						if(SimDriver.GetPlayerAircraft()->AutopilotType() == AircraftClass::LantirnAP) 
						{
							SimDriver.GetPlayerAircraft()->lastapType = SimDriver.GetPlayerAircraft()->AutopilotType();
							SimDriver.GetPlayerAircraft()->SetAutopilot(AircraftClass::APOff);
						}
						//SILENT --> No CARA, no TFR, no Radar
						SimDriver.GetPlayerAircraft()->RFState = 2;
						theRadar->SetEmitting(FALSE);
						if (OTWDriver.GetVirtualCockpit())
							OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_RF_QUIET, 4);
					}
					else
					{
						//NORM
						SimDriver.GetPlayerAircraft()->RFState = 0;
						theRadar->SetEmitting(TRUE);
						//restore AP
						// MD -- 20031214: but only for LantirnAP -- ASSOCI8TOR found this old SP3 bug
						if (SimDriver.GetPlayerAircraft()->lastapType == AircraftClass::LantirnAP)
							SimDriver.GetPlayerAircraft()->SetAutopilot(SimDriver.GetPlayerAircraft()->lastapType);
						if (OTWDriver.GetVirtualCockpit())
							OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_RF_QUIET, 2);
					}
				}
			}
		}
	}
}

// MD -- 20031025: Adding commands to place the RF switch directly in its positions.
// This should help cockpit builders map the RF function to a physical switch.

void SimRFNorm(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{ 
		if(state & KEY_DOWN)
		{
			RadarDopplerClass* theRadar = (RadarDopplerClass*)FindSensor(SimDriver.GetPlayerAircraft(), SensorClass::Radar);
			if (theRadar)
				theRadar->SetEmitting(TRUE);
			if (SimDriver.GetPlayerAircraft()->RFState == 2) //restore AP if the RF was in "silent"
				if (SimDriver.GetPlayerAircraft()->lastapType == AircraftClass::LantirnAP)
					SimDriver.GetPlayerAircraft()->SetAutopilot(SimDriver.GetPlayerAircraft()->lastapType);
			SimDriver.GetPlayerAircraft()->RFState = 0;
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_RF_QUIET, 2);
		}
	}
}

void SimRFQuiet(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{ 
		if(state & KEY_DOWN)
		{
			RadarDopplerClass* theRadar = (RadarDopplerClass*)FindSensor(SimDriver.GetPlayerAircraft(), SensorClass::Radar);
			if (theRadar)
				theRadar->SetEmitting(FALSE);
			if (SimDriver.GetPlayerAircraft()->RFState == 2) //restore AP if the RF was in "silent"
				if (SimDriver.GetPlayerAircraft()->lastapType == AircraftClass::LantirnAP)
					SimDriver.GetPlayerAircraft()->SetAutopilot(SimDriver.GetPlayerAircraft()->lastapType);
			SimDriver.GetPlayerAircraft()->RFState = 1;
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_RF_QUIET, 1);
		}
	}
}

void SimRFSilent(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{ 
		if(state & KEY_DOWN)
		{
			RadarDopplerClass* theRadar = (RadarDopplerClass*)FindSensor(SimDriver.GetPlayerAircraft(), SensorClass::Radar);
			if (theRadar)
				theRadar->SetEmitting(FALSE);
			//store our current AP mode
			if(SimDriver.GetPlayerAircraft()->AutopilotType() == AircraftClass::LantirnAP) 
			{
				SimDriver.GetPlayerAircraft()->lastapType = SimDriver.GetPlayerAircraft()->AutopilotType();
				SimDriver.GetPlayerAircraft()->SetAutopilot(AircraftClass::APOff);
			}
			SimDriver.GetPlayerAircraft()->RFState = 2;
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_RF_QUIET, 4);
		}
	}
}

void SimRwrPower(unsigned long val, int state, void *)
{
    if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
    { 
			PlayerRwrClass* theRwr = (PlayerRwrClass*)FindSensor(SimDriver.GetPlayerAircraft(), SensorClass::RWR);
			if(theRwr) 
			{
				theRwr->SetPower(!theRwr->IsOn());
				if (OTWDriver.GetVirtualCockpit())
				 if (theRwr->IsOn())
						OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_RWR_PWR, 2);
					else
						OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_RWR_PWR, 1);
			}
    }
}

void SimDropProgrammed(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;

    if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
    {
		if(SimDriver.GetPlayerAircraft()->OnGround())
			return;

		//drop our program manually
		if(g_bRealisticAvionics)
		{
			PlayerRwrClass* theRwr = (PlayerRwrClass*)FindSensor(SimDriver.GetPlayerAircraft(), SensorClass::RWR);

			//Check for Power and Failure
			if(!SimDriver.GetPlayerAircraft()->HasPower(AircraftClass::UFCPower) ||
				SimDriver.GetPlayerAircraft()->mFaults->GetFault(FaultClass::ufc_fault) ||
				SimDriver.GetPlayerAircraft()->IsExploding())
				return;

			//Check for our switch
			if(SimDriver.GetPlayerAircraft()->EWSPGM() == AircraftClass::EWSPGMSwitch::Off ||
				SimDriver.GetPlayerAircraft()->EWSPGM() == AircraftClass::EWSPGMSwitch::Stby)
				return;

			if(theRwr)
			{
				SimDriver.GetPlayerAircraft()->DropEWS();
				if(g_bMLU && val)
					theRwr->ReleaseManual = 2;
				else
					theRwr->ReleaseManual = TRUE;
			}
		}
    }
}
//MI
void SimPinkySwitch(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics){
		return;
	}

	AircraftClass *pac = SimDriver.GetPlayerAircraft();
	if (pac && pac->IsSetFlag(MOTION_OWNSHIP) && pac->Sms){
		if(state & KEY_DOWN){
			if(g_bRealisticAvionics){
				//check what all we got first
				RadarDopplerClass* theRadar = (RadarDopplerClass*)FindSensor(pac, SensorClass::Radar);
				bool HasMavs = FALSE;
				LaserPodClass* laserPod = (LaserPodClass* )FindLaserPod (pac);
				MaverickDisplayClass* mavDisplay = NULL;
				if (pac->Sms->curWeaponType == wtAgm65 && pac->Sms->curWeapon){
					HasMavs = TRUE;
					mavDisplay = (MaverickDisplayClass*)((MissileClass*)pac->Sms->GetCurrentWeapon())->display;
				}
				//now check what to toggle
				if(theRadar && theRadar->IsSOI())
				{
					//Toggle EXP and NORM
					if(theRadar->GetRadarMode() == RadarClass::RWS ||
						theRadar->GetRadarMode() == RadarClass::LRS ||
						theRadar->GetRadarMode() == RadarClass::SAM ||
						theRadar->GetRadarMode() == RadarClass::TWS)
					{
						theRadar->ToggleFlag(RadarDopplerClass::EXP);
					}
					else if(theRadar->GetRadarMode() == RadarClass::GM ||
						theRadar->GetRadarMode() == RadarClass::GMT ||
						theRadar->GetRadarMode() == RadarClass::SEA)
					{ 
						theRadar->StepAGfov();
					} 
				}
				else if(SimDriver.GetPlayerAircraft()->FCC && SimDriver.GetPlayerAircraft()->FCC->IsSOI)
					SimDriver.GetPlayerAircraft()->FCC->ToggleHSDZoom();
				else if(HasMavs && mavDisplay && mavDisplay->IsSOI())
					mavDisplay->ToggleFOV();
				else if(laserPod && laserPod->IsSOI())
					laserPod->ToggleFOV();
			}
		}
	}
}
//MI
void SimGndJettEnable(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;

    if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
    {
		if(SimDriver.GetPlayerAircraft()->Sms)
		{
			if(SimDriver.GetPlayerAircraft()->Sms->GndJett)
			{
				SimDriver.GetPlayerAircraft()->Sms->GndJett = FALSE;
				//No jammer when on ground
				if(SimDriver.GetPlayerAircraft()->OnGround())
					SimDriver.GetPlayerAircraft()->UnSetFlag(ECM_ON);
			}
			else
				SimDriver.GetPlayerAircraft()->Sms->GndJett = TRUE;
		}
	}
}

// MD -- 20031128: adding commands to place the Ground jettison enable directly.

void SimGndJettOn(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;

    if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
    {
		if(SimDriver.GetPlayerAircraft()->Sms)
		{
			SimDriver.GetPlayerAircraft()->Sms->GndJett = TRUE;
		}
	}
}

void SimGndJettOff(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;

    if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
    {
		if(SimDriver.GetPlayerAircraft()->Sms)
		{
			SimDriver.GetPlayerAircraft()->Sms->GndJett = FALSE;
			if(SimDriver.GetPlayerAircraft()->OnGround())  // Jammer is disabled when on the ground unless GND JETT is enabled
				SimDriver.GetPlayerAircraft()->UnSetFlag(ECM_ON);
		}
	}
}

void SimToggleExtLights(unsigned long, int state, void*) {
// JPO changes 
    // only applies if complex aircraft and also sets the extra status flags.
	AircraftClass *ac = SimDriver.GetPlayerAircraft();
	if (!ac){ return; }
    if (ac->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN) && ac->IsComplex()) 
	{
		if (ac->IsAcStatusBitsSet(AircraftClass::ACSTATUS_EXT_LIGHTS))
		{
			// lights are on
			ac->SetSwitch(COMP_TAIL_STROBE, FALSE);
			ac->SetSwitch(COMP_NAV_LIGHTS, FALSE);
			ac->ClearAcStatusBits(
				AircraftClass::ACSTATUS_EXT_LIGHTS | AircraftClass::ACSTATUS_EXT_NAVLIGHTS |
				AircraftClass::ACSTATUS_EXT_NAVLIGHTSFLASH | AircraftClass::ACSTATUS_EXT_TAILSTROBE	);
			if (OTWDriver.GetVirtualCockpit())
			{
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_EXT_LITE_MSTR, 1);
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_ANTI_COLL, 1);
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_EXT_WING, 1);
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_EXT_FUSELAGE, 1);
			}
		}
		else 
		{
			ac->SetSwitch(COMP_TAIL_STROBE, TRUE);
			ac->SetSwitch(COMP_NAV_LIGHTS, TRUE);
			ac->SetAcStatusBits(
				AircraftClass::ACSTATUS_EXT_LIGHTS | AircraftClass::ACSTATUS_EXT_NAVLIGHTS |
				AircraftClass::ACSTATUS_EXT_NAVLIGHTSFLASH | AircraftClass::ACSTATUS_EXT_TAILSTROBE	);
			if (OTWDriver.GetVirtualCockpit())
			{
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_EXT_LITE_MSTR, 2);
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_ANTI_COLL, 2);
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_EXT_WING, 2);
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_EXT_FUSELAGE, 2);
			}
		}
  }
}
//MI
void SimExtlPower(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;

	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)
		&& SimDriver.GetPlayerAircraft()->af->IsSet(AirframeClass::HasComplexGear))
	{  
		AircraftClass *ac = SimDriver.GetPlayerAircraft();

		if(ac->ExtlState(AircraftClass::ExtlLightFlags::Extl_Main_Power))
		{
			ac->ExtlOff(AircraftClass::ExtlLightFlags::Extl_Main_Power);
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_EXT_LITE_MSTR, 1);
		}
		else
		{
			ac->ExtlOn(AircraftClass::ExtlLightFlags::Extl_Main_Power);	
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_EXT_LITE_MSTR, 2);
		}
	}
}

// MD -- 20031128: adding commands to place the external light master switch directly.

void SimExtlMasterNorm(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;

	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)
		&& SimDriver.GetPlayerAircraft()->af->IsSet(AirframeClass::HasComplexGear))
	{  
		SimDriver.GetPlayerAircraft()->ExtlOn(AircraftClass::ExtlLightFlags::Extl_Main_Power);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_EXT_LITE_MSTR, 2);
	}
}

void SimExtlMasterOff(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;

	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)
    && SimDriver.GetPlayerAircraft()->af->IsSet(AirframeClass::HasComplexGear))
	{  
		SimDriver.GetPlayerAircraft()->ExtlOff(AircraftClass::ExtlLightFlags::Extl_Main_Power);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_EXT_LITE_MSTR, 1);
	}
}

void SimExtlAntiColl(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;

	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN) 
		&& SimDriver.GetPlayerAircraft()->af->IsSet(AirframeClass::HasComplexGear))
	{
		AircraftClass *ac = SimDriver.GetPlayerAircraft();

		if(ac->ExtlState(AircraftClass::ExtlLightFlags::Extl_Anti_Coll))
		{
			ac->SetSwitch(COMP_TAIL_STROBE, FALSE);
			ac->ClearAcStatusBits(AircraftClass::ACSTATUS_EXT_TAILSTROBE);
			ac->ExtlOff(AircraftClass::ExtlLightFlags::Extl_Anti_Coll);
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_ANTI_COLL, 2);
		}
		else
		{
			ac->SetSwitch(COMP_TAIL_STROBE, TRUE);
			ac->SetAcStatusBits(AircraftClass::ACSTATUS_EXT_TAILSTROBE);
			ac->ExtlOn(AircraftClass::ExtlLightFlags::Extl_Anti_Coll);
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_ANTI_COLL, 1);
		}
	}
}

// MD -- 20031128: adding commands to place the position anti-collision light switch directly

void SimAntiCollOn(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;

	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN) 
		&& SimDriver.GetPlayerAircraft()->af->IsSet(AirframeClass::HasComplexGear))
	{
		SimDriver.GetPlayerAircraft()->SetSwitch(COMP_TAIL_STROBE, TRUE);
		SimDriver.GetPlayerAircraft()->SetAcStatusBits(AircraftClass::ACSTATUS_EXT_TAILSTROBE);
		SimDriver.GetPlayerAircraft()->ExtlOn(AircraftClass::ExtlLightFlags::Extl_Anti_Coll);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_ANTI_COLL, 2);
	}
}

void SimAntiCollOff(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;

	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN) 
		&& SimDriver.GetPlayerAircraft()->af->IsSet(AirframeClass::HasComplexGear))
	{
		SimDriver.GetPlayerAircraft()->SetSwitch(COMP_TAIL_STROBE, FALSE);
		SimDriver.GetPlayerAircraft()->ClearAcStatusBits(AircraftClass::ACSTATUS_EXT_TAILSTROBE);
		SimDriver.GetPlayerAircraft()->ExtlOff(AircraftClass::ExtlLightFlags::Extl_Anti_Coll);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_ANTI_COLL, 1);
	}
}

void SimExtlWing(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;

	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)
		&& SimDriver.GetPlayerAircraft()->af->IsSet(AirframeClass::HasComplexGear))
	{    
		AircraftClass *ac = SimDriver.GetPlayerAircraft();

		if(ac->ExtlState(AircraftClass::ExtlLightFlags::Extl_Wing_Tail))
		{
			ac->ClearAcStatusBits(AircraftClass::ACSTATUS_EXT_NAVLIGHTS);
		  ac->ExtlOff(AircraftClass::ExtlLightFlags::Extl_Wing_Tail);
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_EXT_WING, 1);
		}
		else
		{
			ac->SetAcStatusBits(AircraftClass::ACSTATUS_EXT_NAVLIGHTS);
			ac->ExtlOn(AircraftClass::ExtlLightFlags::Extl_Wing_Tail);
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_EXT_WING, 2);
		}
	}
}

// MD -- 20031128: adding commands to place the wing light switch directly.
// NB: seems like there are only two placements modeled in the game for this
// three position switch.

void SimWingLightBrt(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;

	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)
		&& SimDriver.GetPlayerAircraft()->af->IsSet(AirframeClass::HasComplexGear))
	{    
		SimDriver.GetPlayerAircraft()->SetAcStatusBits(AircraftClass::ACSTATUS_EXT_NAVLIGHTS);
		SimDriver.GetPlayerAircraft()->ExtlOn(AircraftClass::ExtlLightFlags::Extl_Wing_Tail);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_EXT_WING, 4);
	}
}

void SimWingLightOff(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;

	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)
		&& SimDriver.GetPlayerAircraft()->af->IsSet(AirframeClass::HasComplexGear))
	{    
		SimDriver.GetPlayerAircraft()->ClearAcStatusBits(AircraftClass::ACSTATUS_EXT_NAVLIGHTS);
		SimDriver.GetPlayerAircraft()->ExtlOff(AircraftClass::ExtlLightFlags::Extl_Wing_Tail);
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_EXT_WING, 1);
	}
}

void SimExtlSteady(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;

	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)
		&& SimDriver.GetPlayerAircraft()->af->IsSet(AirframeClass::HasComplexGear))
	{    
		AircraftClass *ac = SimDriver.GetPlayerAircraft();

		if(ac->ExtlState(AircraftClass::ExtlLightFlags::Extl_Flash))
		{
		  ac->ExtlOff(AircraftClass::ExtlLightFlags::Extl_Flash);
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_EXT_FLASH, 2);
		}
		else
		{
			ac->ExtlOn(AircraftClass::ExtlLightFlags::Extl_Flash);
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_EXT_FLASH, 1);
		}
	}
}

// MD -- 20031128: adding commands to place the steady/flash switch direcly.
// not flashing aka. OFF
void SimLightsSteady(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;

	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN) 
		&& SimDriver.GetPlayerAircraft()->af->IsSet(AirframeClass::HasComplexGear))
	{
		SimDriver.GetPlayerAircraft()->ExtlOff(AircraftClass::ExtlLightFlags::Extl_Flash);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_EXT_FLASH, 1);
	}
}
// flashing aka. ON
void SimLightsFlash(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;

	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN) 
		&& SimDriver.GetPlayerAircraft()->af->IsSet(AirframeClass::HasComplexGear))
	{
		SimDriver.GetPlayerAircraft()->ExtlOn(AircraftClass::ExtlLightFlags::Extl_Flash);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_EXT_FLASH, 2);
	}
}

//MI DMS
void SimDMSUp(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;

	//Up always goes to the HUD (where possible)
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		SimDriver.GetPlayerAircraft()->StepSOI(1);
	}
}
void SimDMSLeft(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;

	if ((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft()) && (SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)))
	{
		MfdDisplay[0]->changeMode = TRUE_NEXT;
		if(SimDriver.GetPlayerAircraft()->FCC && SimDriver.GetPlayerAircraft()->FCC->IsSOI && g_bRealisticAvionics)
		{ 
			if(MfdDisplay[0]->CurMode() == MFDClass::FCCMode)
				SimDriver.GetPlayerAircraft()->StepSOI(2);
		}
	}		
}

void SimDMSDown(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;

	//Down toggles between MFD's
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		SimDriver.GetPlayerAircraft()->StepSOI(2);
	}
}
void SimDMSRight(unsigned long val, int state, void *)
{
	if((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft()) && (SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)))
	{
		MfdDisplay[1]->changeMode = TRUE_NEXT;
		if(SimDriver.GetPlayerAircraft()->FCC && SimDriver.GetPlayerAircraft()->FCC->IsSOI && g_bRealisticAvionics)
		{ 
		  if(MfdDisplay[1]->CurMode() == MFDClass::FCCMode)
			  SimDriver.GetPlayerAircraft()->StepSOI(2);
		} 
	}
}
void SimAVTRSwitch(unsigned long val, int state, void *pButton)
{
	if(!g_bRealisticAvionics)
		return;

	if((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft()) && (SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)))
	{
		//off-auto-on
		if(SimDriver.GetPlayerAircraft()->AVTRState(AircraftClass::AVTRStateFlags::AVTR_OFF))
		{
			SimDriver.GetPlayerAircraft()->AVTROn(AircraftClass::AVTRStateFlags::AVTR_AUTO);
			SimDriver.GetPlayerAircraft()->AVTROff(AircraftClass::AVTRStateFlags::AVTR_OFF);
			SimDriver.GetPlayerAircraft()->AVTROff(AircraftClass::AVTRStateFlags::AVTR_ON);	//just in case
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_AVTR_SW, 2);
		}
		else if(SimDriver.GetPlayerAircraft()->AVTRState(AircraftClass::AVTRStateFlags::AVTR_AUTO))
		{
			SimDriver.GetPlayerAircraft()->AVTROn(AircraftClass::AVTRStateFlags::AVTR_ON);
			SimDriver.GetPlayerAircraft()->AVTROff(AircraftClass::AVTRStateFlags::AVTR_AUTO);
			SimDriver.GetPlayerAircraft()->AVTROff(AircraftClass::AVTRStateFlags::AVTR_OFF);	//just in case
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_AVTR_SW, 4);
			if(SimDriver.AVTROn() == FALSE)
			{
				SimDriver.SetAVTR(TRUE);
				ACMIToggleRecording ( val, state, pButton);
			}
		}
		else
		{
			SimDriver.GetPlayerAircraft()->AVTROn(AircraftClass::AVTRStateFlags::AVTR_OFF);
			SimDriver.GetPlayerAircraft()->AVTROff(AircraftClass::AVTRStateFlags::AVTR_ON);
			SimDriver.GetPlayerAircraft()->AVTROff(AircraftClass::AVTRStateFlags::AVTR_AUTO);	//just in case
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_AVTR_SW, 1);
			if(SimDriver.AVTROn() == TRUE) 
			{
				SimDriver.SetAVTR(FALSE);
				ACMIToggleRecording( val, state, pButton);
			}
		}		
	}
}

// MD -- 20031024: Adding commands to set the AVTR mode switch position explicitly to
// make it easier for cockpit builders to map the commands to a physical switch.

void SimAVTRSwitchOff(unsigned long val, int state, void *pButton)
{
	if(!g_bRealisticAvionics)
		return;

	if((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft()) && (SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)))
	{
		SimDriver.GetPlayerAircraft()->AVTROn(AircraftClass::AVTRStateFlags::AVTR_OFF);
		SimDriver.GetPlayerAircraft()->AVTROff(AircraftClass::AVTRStateFlags::AVTR_ON);
		SimDriver.GetPlayerAircraft()->AVTROff(AircraftClass::AVTRStateFlags::AVTR_AUTO);	//just in case
		if(SimDriver.AVTROn() == TRUE) 
		{
			SimDriver.SetAVTR(FALSE);
			ACMIToggleRecording( val, state, pButton);
		}
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_AVTR_SW, 1);
	}
}

void SimAVTRSwitchAuto(unsigned long val, int state, void *pButton)
{
	if(!g_bRealisticAvionics)
		return;

	if((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft()) && (SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)))
	{
		SimDriver.GetPlayerAircraft()->AVTROn(AircraftClass::AVTRStateFlags::AVTR_AUTO);
		SimDriver.GetPlayerAircraft()->AVTROff(AircraftClass::AVTRStateFlags::AVTR_OFF);
		SimDriver.GetPlayerAircraft()->AVTROff(AircraftClass::AVTRStateFlags::AVTR_ON);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_AVTR_SW, 2);
	}
}

void SimAVTRSwitchOn(unsigned long val, int state, void *pButton)
{
	if(!g_bRealisticAvionics)
		return;

	if((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft()) && (SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)))
	{
		SimDriver.GetPlayerAircraft()->AVTROn(AircraftClass::AVTRStateFlags::AVTR_ON);
		SimDriver.GetPlayerAircraft()->AVTROff(AircraftClass::AVTRStateFlags::AVTR_AUTO);
		SimDriver.GetPlayerAircraft()->AVTROff(AircraftClass::AVTRStateFlags::AVTR_OFF);	//just in case
		if(SimDriver.AVTROn() == FALSE)
		{
			SimDriver.SetAVTR(TRUE);
			ACMIToggleRecording ( val, state, pButton);
		}
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_AVTR_SW, 4);
	}
}

//MI
void SimAutoAVTR(unsigned long val, int state, void *pButton)
{
	if(!g_bRealisticAvionics)
		return;
	if((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft()) && (SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)))
	{
		if(SimDriver.GetPlayerAircraft()->Sms && SimDriver.GetPlayerAircraft()->Sms->MasterArm() ==
			SMSBaseClass::Safe)
			return;

		if(!SimDriver.GetPlayerAircraft()->AVTRState(AircraftClass::AVTRStateFlags::AVTR_AUTO))
			return;

		if(SimDriver.AVTROn() == FALSE)
		{ 
			SimDriver.SetAVTR(TRUE);
			SimDriver.GetPlayerAircraft()->AddAVTRSeconds();
			ACMIToggleRecording(0, state, NULL);
		}
		else
			SimDriver.GetPlayerAircraft()->AddAVTRSeconds();
	}
}
//MI
void SimIFFPower(unsigned long val, int state, void *)
{
//Cobra 11/20/04 Ok, let's make this switch work
	if(!g_bRealisticAvionics)
		return;
	if((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft()) && (SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)))
		{
			if (!SimDriver.GetPlayerAircraft()->HasPower(AircraftClass::IFFPower))
			{
				SimDriver.GetPlayerAircraft()->PowerOn(AircraftClass::IFFPower);
				if (OTWDriver.GetVirtualCockpit())
					OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_IFF_PWR, 2);
			}
			else 
			{
				SimDriver.GetPlayerAircraft()->PowerOff(AircraftClass::IFFPower);
				if (OTWDriver.GetVirtualCockpit())
					OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_IFF_PWR, 1);
			}
		}

}
//MI
void SimIFFIn(unsigned long val, int state, void *)
{
//Cobra 11/21/04  This comes from TMS Left.  We have commanded an interrogation
if(!g_bRealisticAvionics)
		return;
if (!SimDriver.GetPlayerAircraft()->iffEnabled)
	return;

	if((state & KEY_DOWN) && (SimDriver.GetPlayerAircraft()) && (SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP)))
	{
		SimDriver.GetPlayerAircraft()->runIFFInt = TRUE;
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_IFF_QUERY, 2);
	}
	else
	{
		SimDriver.GetPlayerAircraft()->interrogating = FALSE;
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_IFF_QUERY, 1);
	}

}
//MI
void SimINSInc(unsigned long val, int state, void *)
{
	//don't bother
	if(!g_bINS || !g_bRealisticAvionics)
		return;

	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		if(SimDriver.GetPlayerAircraft()->INSState(AircraftClass::INS_PowerOff))
		{
			SimDriver.GetPlayerAircraft()->SwitchINSToAlign();
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_INS_MODE, 2);
		}
		else if(SimDriver.GetPlayerAircraft()->INSState(AircraftClass::INS_AlignNorm))
		{
			SimDriver.GetPlayerAircraft()->SwitchINSToNav();
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_INS_MODE, 4);
		}
		else if(SimDriver.GetPlayerAircraft()->INSState(AircraftClass::INS_Nav))
		{
			SimDriver.GetPlayerAircraft()->SwitchINSToInFLT();
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_INS_MODE, 8);
		}
	}
}
//MI
void SimINSDec(unsigned long val, int state, void *)
{
	//don't bother
	if(!g_bINS || !g_bRealisticAvionics)
		return;

	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		if(SimDriver.GetPlayerAircraft()->INSState(AircraftClass::INS_AlignFlight))
		{
			SimDriver.GetPlayerAircraft()->SwitchINSToNav();
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_INS_MODE, 4);
		}
		else if(SimDriver.GetPlayerAircraft()->INSState(AircraftClass::INS_Nav))
		{
			SimDriver.GetPlayerAircraft()->SwitchINSToAlign();
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_INS_MODE, 2);
		}
		else if(SimDriver.GetPlayerAircraft()->INSState(AircraftClass::INS_AlignNorm))
		{
			SimDriver.GetPlayerAircraft()->SwitchINSToOff();
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_INS_MODE, 1);
		}
	}
}

// MD -- 20031026: Adding commands to set the INS mode Knob position explicitly to
// make it easier for cockpit builders to map the commands to a physical knob.

void SimINSOff(unsigned long val, int state, void *)
{
	//don't bother
	if(!g_bINS || !g_bRealisticAvionics)
		return;

	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		SimDriver.GetPlayerAircraft()->SwitchINSToOff();
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_INS_MODE, 1);
	}
}

void SimINSNorm(unsigned long val, int state, void *)
{
	//don't bother
	if(!g_bINS || !g_bRealisticAvionics)
		return;

	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		SimDriver.GetPlayerAircraft()->SwitchINSToAlign();
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_INS_MODE, 2);
	}
}

void SimINSNav(unsigned long val, int state, void *)
{
	//don't bother
	if(!g_bINS || !g_bRealisticAvionics)
		return;

	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		SimDriver.GetPlayerAircraft()->SwitchINSToNav();
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_INS_MODE, 4);
	}
}

void SimINSInFlt(unsigned long val, int state, void *)
{
	//don't bother
	if(!g_bINS || !g_bRealisticAvionics)
		return;

	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		SimDriver.GetPlayerAircraft()->SwitchINSToInFLT();
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_INS_MODE, 8);
	}
}

//MI
void SimLEFLockSwitch(unsigned long val, int state, void *)
{
	//don't bother
	if(!g_bRealisticAvionics)
		return;

	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		if(SimDriver.GetPlayerAircraft()->LEFLocked)
		{
			SimDriver.GetPlayerAircraft()->LEFLocked = FALSE;
			if(SimDriver.GetPlayerAircraft()->mFaults && !SimDriver.GetPlayerAircraft()->LEFState(AircraftClass::LEFSASYNCH))
				SimDriver.GetPlayerAircraft()->mFaults->ClearFault(lef_fault);
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_LEF_FLAPS, 1);
		}
		else
		{
			SimDriver.GetPlayerAircraft()->LEFLocked = TRUE;			
			if(SimDriver.GetPlayerAircraft()->mFaults)
				SimDriver.GetPlayerAircraft()->mFaults->SetCaution(lef_fault);
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_LEF_FLAPS, 2);
		}
	}	
}

// MD -- 20031125: adding separate commands to place the LEF Lock switch explicitly

void SimLEFLock(unsigned long val, int state, void *)
{
	//don't bother
	if(!g_bRealisticAvionics)
		return;

	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		SimDriver.GetPlayerAircraft()->LEFLocked = TRUE;			
		if(SimDriver.GetPlayerAircraft()->mFaults)
			SimDriver.GetPlayerAircraft()->mFaults->SetCaution(lef_fault);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_LEF_FLAPS, 2);
	}	
}

void SimLEFAuto(unsigned long val, int state, void *)
{
	//don't bother
	if(!g_bRealisticAvionics)
		return;

	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		SimDriver.GetPlayerAircraft()->LEFLocked = FALSE;
		if(SimDriver.GetPlayerAircraft()->mFaults && !SimDriver.GetPlayerAircraft()->LEFState(AircraftClass::LEFSASYNCH))
			SimDriver.GetPlayerAircraft()->mFaults->ClearFault(lef_fault);
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_LEF_FLAPS, 1);
	}	
}


void SimDigitalBUP(unsigned long val, int state, void *)
{
}
void SimAltFlaps(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;

	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		if(SimDriver.GetPlayerAircraft()->TEFExtend)
		{
			SimDriver.GetPlayerAircraft()->TEFExtend = FALSE;
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_ALT_FLAPS, 1);
		}
		else
		{
			SimDriver.GetPlayerAircraft()->TEFExtend = TRUE;
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_ALT_FLAPS, 2);
		}
	}
}

// MD -- 20031125: adding commands to place the ALT FLAPS switch explicitly.

void SimAltFlapsNorm(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;

	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		SimDriver.GetPlayerAircraft()->TEFExtend = FALSE;
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_ALT_FLAPS, 1);
	}
}

void SimAltFlapsExtend(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;

	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		SimDriver.GetPlayerAircraft()->TEFExtend = TRUE;
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_ALT_FLAPS, 2);
	}
}

void SimManualFlyup(unsigned long val, int state, void *)
{
}
void SimFLCSReset(unsigned long val, int state, void *)
{
}
void SimFLTBIT(unsigned long val, int state, void *)
{
}
void SimOBOGSBit(unsigned long val, int state, void *)
{
}
//MI
void SimMalIndLights(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;
	if (OTWDriver.GetVirtualCockpit())
	{
		if (SimDriver.GetPlayerAircraft()->TestLights == TRUE)
			SimDriver.GetPlayerAircraft()->TestLights = FALSE;
		else
			SimDriver.GetPlayerAircraft()->TestLights = TRUE;
		return;
	}

	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP))
	{
		if(state & KEY_DOWN)
			SimDriver.GetPlayerAircraft()->TestLights = TRUE;
		else
			SimDriver.GetPlayerAircraft()->TestLights = FALSE;
	}
}
void SimProbeHeat(unsigned long val, int state, void *)
{
}
void SimEPUGEN(unsigned long val, int state, void *)
{
}
void SimTestSwitch(unsigned long val, int state, void *)
{
}
void SimOverHeat(unsigned long val, int state, void *)
{
}

// MD -- 20031128: zero the trim values dial in from the HAT control when the switch moves
// to the DISC position.
void SimTrimAPDisc(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;

	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		if(SimDriver.GetPlayerAircraft()->TrimAPDisc)
		{
			SimDriver.GetPlayerAircraft()->TrimAPDisc = FALSE;
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_TRIM_AP, 2);
		}
		else 
		{
			SimDriver.GetPlayerAircraft()->TrimAPDisc = TRUE;
			pitchRudderTrimRate = 0.0F;
			pitchAileronTrimRate = 0.0f;
			pitchElevatorTrimRate = 0.0f;
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_TRIM_AP, 1);
		}
	}
}

// MD -- 20031125: adding separate commands to place the AP/DISC switch directly.
// MD -- 20031128: zero the trim values dial in from the HAT control when the switch moves
// to the DISC position.

void SimTrimAPDISC(unsigned long val, int state, void *)  // case *is* significant for command names
{
	if(!g_bRealisticAvionics)
		return;

	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		SimDriver.GetPlayerAircraft()->TrimAPDisc = TRUE;
		pitchRudderTrimRate = 0.0F;
		pitchAileronTrimRate = 0.0f;
		pitchElevatorTrimRate = 0.0f;
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_TRIM_AP, 1);
	}
}

void SimTrimAPNORM(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;

	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		SimDriver.GetPlayerAircraft()->TrimAPDisc = FALSE;
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_TRIM_AP, 2);
	}
}

void SimMaxPower(unsigned long val, int state, void *)
{
}
void SimABReset(unsigned long val, int state, void *)
{
}
void SimTrimNoseUp(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		pitchManualTrim = 0.50F;
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetDOFangle( COMP_3DPIT_TRIM_PITCH, cockpitFlightData.TrimPitch + 0.5f );
	}
}
void SimTrimNoseDown(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		pitchManualTrim = -0.50F;
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetDOFangle( COMP_3DPIT_TRIM_PITCH, cockpitFlightData.TrimPitch - 0.5f );
	}
}
void SimTrimYawLeft(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		yawManualTrim = -2.0F;
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetDOFangle( COMP_3DPIT_TRIM_YAW, cockpitFlightData.TrimYaw - 2.0f );
	}
}
void SimTrimYawRight(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		yawManualTrim = 2.0F;
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetDOFangle( COMP_3DPIT_TRIM_YAW, cockpitFlightData.TrimYaw + 2.0f );
	}
}
void SimTrimRollLeft(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		rollManualTrim = -0.50F;
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetDOFangle( COMP_3DPIT_TRIM_ROLL, cockpitFlightData.TrimRoll - 0.5f );
	}
}
void SimTrimRollRight(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		rollManualTrim = 0.50F;
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetDOFangle( COMP_3DPIT_TRIM_ROLL, cockpitFlightData.TrimRoll + 0.5f );
	}
}
void SimStepMissileVolumeDown(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;

	if (IO.AnalogIsUsed(AXIS_MSL_VOLUME) == false)	// Retro 3Jan2004
	{
		if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		{
			SimDriver.GetPlayerAircraft()->MissileVolume++;
			if(SimDriver.GetPlayerAircraft()->MissileVolume > 8)
				SimDriver.GetPlayerAircraft()->MissileVolume = 8;
			//int vall = 9 - SimDriver.GetPlayerAircraft()->MissileVolume;
			int vall = 1<<(8 - SimDriver.GetPlayerAircraft()->MissileVolume);
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_MISSILE_VOL, vall);
		}
	}	// Retro 3Jan2004
}
void SimStepMissileVolumeUp(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;

	if (IO.AnalogIsUsed(AXIS_MSL_VOLUME) == false)	// Retro 3Jan2004
	{
		if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		{
			SimDriver.GetPlayerAircraft()->MissileVolume--;
			if(SimDriver.GetPlayerAircraft()->MissileVolume < 0)
				SimDriver.GetPlayerAircraft()->MissileVolume = 0;
			//int vall = 9 - SimDriver.GetPlayerAircraft()->MissileVolume;
			int vall = 1<<(8 - SimDriver.GetPlayerAircraft()->MissileVolume);
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_MISSILE_VOL, vall);
		}
	}	// Retro 3Jan2004
}
void SimStepThreatVolumeDown(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;

	if (IO.AnalogIsUsed(AXIS_THREAT_VOLUME) == false)	// Retro 3Jan2004
	{
		if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		{
			SimDriver.GetPlayerAircraft()->ThreatVolume++;
			if(SimDriver.GetPlayerAircraft()->ThreatVolume > 8)
				SimDriver.GetPlayerAircraft()->ThreatVolume = 8;
			//int vall = 9 - SimDriver.GetPlayerAircraft()->MissileVolume;
			int vall = 1<<(8 - SimDriver.GetPlayerAircraft()->ThreatVolume);
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_THREAT_VOL, vall);
		}
	}	// Retro 3Jan2004
}
void SimStepThreatVolumeUp(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;

	if (IO.AnalogIsUsed(AXIS_THREAT_VOLUME) == false)	// Retro 3Jan2004
	{
		if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		{
			SimDriver.GetPlayerAircraft()->ThreatVolume--;
			if(SimDriver.GetPlayerAircraft()->ThreatVolume < 0)
				SimDriver.GetPlayerAircraft()->ThreatVolume = 0;
			//int vall = 9 - SimDriver.GetPlayerAircraft()->MissileVolume;
			int vall = 1<<(8 - SimDriver.GetPlayerAircraft()->ThreatVolume);
			if (OTWDriver.GetVirtualCockpit())
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_THREAT_VOL, vall);
		}
	}	// Retro 3Jan2004
}
void SimTriggerFirstDetent(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP))
	{
		LaserPodClass* laserPod = (LaserPodClass* )FindLaserPod (SimDriver.GetPlayerAircraft());
		if(state & KEY_DOWN)
		{
			//AVTR
			if(SimDriver.GetPlayerAircraft()->AVTRState(AircraftClass::AVTRStateFlags::AVTR_AUTO))
			{
				if(SimDriver.GetPlayerAircraft()->Sms && SimDriver.GetPlayerAircraft()->Sms->MasterArm() ==
					SMSBaseClass::Safe)
					return;

				if(SimDriver.AVTROn() == FALSE)
				{
					SimDriver.SetAVTR(TRUE);
					SimDriver.GetPlayerAircraft()->AddAVTRSeconds();
					ACMIToggleRecording(0, state, NULL);
				}
				else
					SimDriver.GetPlayerAircraft()->AddAVTRSeconds();
			}
			//Targeting Pod, Fire laser
			if(laserPod && SimDriver.GetPlayerAircraft()->FCC->LaserArm && SimDriver.GetPlayerAircraft()->FCC->GetMasterMode()
				== FireControlComputer::AirGroundLaser)
			{
				if(!SimDriver.GetPlayerAircraft()->FCC->InhibitFire)
				{
					if(SimDriver.GetPlayerAircraft()->HasPower(AircraftClass::RightHptPower))
					{
						SimDriver.GetPlayerAircraft()->FCC->ManualFire = TRUE;
						SimDriver.GetPlayerAircraft()->FCC->CheckForLaserFire = FALSE;
					}
				}
			}
		}
		else
		{
			SimDriver.GetPlayerAircraft()->FCC->ManualFire = FALSE;
			SimDriver.GetPlayerAircraft()->FCC->CheckForLaserFire = FALSE;
		}
	}
}
void SimTriggerSecondDetent(unsigned long val, int state, void *)
{
	//if we push this one, we must have pushed the first stage as well
	if(!g_bRealisticAvionics)
		return;
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP))
	{
		LaserPodClass* laserPod = (LaserPodClass* )FindLaserPod (SimDriver.GetPlayerAircraft());
		if(state & KEY_DOWN)
		{
			//First, check for AVTR. The 30 seconds start when we release the trigger completely.
			 if(SimDriver.GetPlayerAircraft()->AVTRState(AircraftClass::AVTRStateFlags::AVTR_AUTO))
			 {
				 if(SimDriver.GetPlayerAircraft()->Sms && SimDriver.GetPlayerAircraft()->Sms->MasterArm() ==
					 SMSBaseClass::Safe)
					 return;

				 if(SimDriver.AVTROn() == FALSE)
				 {
					 SimDriver.SetAVTR(TRUE);
					 SimDriver.GetPlayerAircraft()->AddAVTRSeconds();
					 ACMIToggleRecording(0, state, NULL);
				 }
				 else
					 SimDriver.GetPlayerAircraft()->AddAVTRSeconds();
			 }
			//Gun
			if(SimDriver.GetPlayerAircraft()->FCC &&
				(SimDriver.GetPlayerAircraft()->FCC->GetMasterMode() == FireControlComputer::Dogfight ||
				SimDriver.GetPlayerAircraft()->FCC->GetMasterMode() == FireControlComputer::MissileOverride || 
				SimDriver.GetPlayerAircraft()->FCC->GetSubMode() == FireControlComputer::STRAF || 
				SimDriver.GetPlayerAircraft()->Sms->curWeaponType == wtGuns))
			{
				SimDriver.GetPlayerAircraft()->GunFire = TRUE;
			}
			//Targeting Pod, Fire laser
			if(laserPod && SimDriver.GetPlayerAircraft()->FCC->LaserArm && SimDriver.GetPlayerAircraft()->FCC->GetMasterMode()
				== FireControlComputer::AirGroundLaser)
			{
				if(!SimDriver.GetPlayerAircraft()->FCC->InhibitFire)
				{
					if(SimDriver.GetPlayerAircraft()->HasPower(AircraftClass::RightHptPower))
					{
						SimDriver.GetPlayerAircraft()->FCC->ManualFire = TRUE;
						SimDriver.GetPlayerAircraft()->FCC->CheckForLaserFire = FALSE;
					}
				}
			}
		}
		else
		{
			SimDriver.GetPlayerAircraft()->GunFire = FALSE;
			SimDriver.GetPlayerAircraft()->FCC->ManualFire = TRUE;
			SimDriver.GetPlayerAircraft()->FCC->CheckForLaserFire = FALSE;
		}
		
	}
}

void AFFullFlap(unsigned long, int state, void*)
{
    if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	//TJL 02/28/04 Adding Auto/Half/Full for F18's
	//10 = AUTO, 20 = HALF, 30 = FULL
	{
		if (SimDriver.GetPlayerAircraft()->af->GetTypeAC() == 8 ||
			SimDriver.GetPlayerAircraft()->af->GetTypeAC() == 9 ||
			SimDriver.GetPlayerAircraft()->af->GetTypeAC() == 10)
		{
			if (
				SimDriver.GetPlayerAircraft()->af->flapPos != 10 &&
				SimDriver.GetPlayerAircraft()->af->flapPos != 20 &&
				SimDriver.GetPlayerAircraft()->af->flapPos != 30
			){
				SimDriver.GetPlayerAircraft()->af->flapPos = 10;
			}

			SimDriver.GetPlayerAircraft()->af->flapPos += 10;
			if (SimDriver.GetPlayerAircraft()->af->flapPos > 30){
				SimDriver.GetPlayerAircraft()->af->flapPos = 10;
			}
		}
		//orig code
		else {
			SimDriver.GetPlayerAircraft()->af->TEFMax();
			SimDriver.GetPlayerAircraft()->af->flapPos = 0;//TJL 02/28/04
		}
	}
}
void AFNoFlap(unsigned long, int state, void*)
{
    if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
    {
		SimDriver.GetPlayerAircraft()->af->TEFClose();
		SimDriver.GetPlayerAircraft()->af->flapPos = 3;//TJL 02/28/04
    }
}
void AFIncFlap(unsigned long, int state, void*) 
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
    if (playerAC && playerAC->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)){
		playerAC->af->TEFInc();
		playerAC->af->flapPos = 0;//TJL 02/28/04
    }
}
void AFDecFlap(unsigned long, int state, void*){
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
    if(playerAC && playerAC->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)){
		playerAC->af->TEFDec();
		playerAC->af->flapPos = 4;//TJL 02/28/04
    }
}

void AFFullLEF(unsigned long, int state, void*)
{
    if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
    {
		SimDriver.GetPlayerAircraft()->af->LEFMax();
    }
}

void AFNoLEF(unsigned long, int state, void*)
{
    if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
    {
		SimDriver.GetPlayerAircraft()->af->LEFClose();
    }
}

void AFIncLEF(unsigned long, int state, void*)
{
    if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
    {
		SimDriver.GetPlayerAircraft()->af->LEFInc();
    }
}

void AFDecLEF(unsigned long, int state, void*)
{
    if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
    {
		SimDriver.GetPlayerAircraft()->af->LEFDec();
    }
}

void AFDragChute(unsigned long, int state, void*)
{
    if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
    {
			if (SimDriver.GetPlayerAircraft()->af->HasDragChute())
				switch (SimDriver.GetPlayerAircraft()->af->dragChute) 
				{
					case AirframeClass::DRAGC_STOWED:
						SimDriver.GetPlayerAircraft()->af->dragChute = AirframeClass::DRAGC_DEPLOYED;
						break;
					case AirframeClass::DRAGC_DEPLOYED:
					case AirframeClass::DRAGC_TRAILING:
						SimDriver.GetPlayerAircraft()->af->dragChute = AirframeClass::DRAGC_JETTISONNED;
						break;
				}
			if (SimDriver.GetPlayerAircraft()->af->dragChute == AirframeClass::DRAGC_STOWED)
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_DRAGCHUTE, 1);
			else
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_DRAGCHUTE, 2);
    }
}

void SimRetUp(unsigned long, int state, void*)
{
	if(!g_bRealisticAvionics)
		return;

	if (IO.AnalogIsUsed(AXIS_RET_DEPR) == false)	// Retro 3Jan2004
	{
		if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		{
			if(TheHud)
			{
				TheHud->RetPos -= 1;
				TheHud->MoveRetCenter();
				if(TheHud->ReticlePosition > -12)
					TheHud->ReticlePosition--;
			}
		}
	}		// Retro 3Jan2004
}
void SimRetDn(unsigned long, int state, void*)
{
	if(!g_bRealisticAvionics)
		return;

	if (IO.AnalogIsUsed(AXIS_RET_DEPR) == false)	// Retro 3Jan2004
	{
		if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		{
			if(TheHud)
			{
				TheHud->RetPos +=1;
				TheHud->MoveRetCenter();
				if(TheHud->ReticlePosition < 0)
					TheHud->ReticlePosition++;
			}
		}
	}		// Retro 3Jan2004
}
void SimCursorEnable(unsigned long, int state, void*)
{
	if(!g_bRealisticAvionics)
		return;

	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
    {
		if(SimDriver.GetPlayerAircraft()->Sms)
		{
			if(SimDriver.GetPlayerAircraft()->Sms->curWeaponType == wtAgm65 && SimDriver.GetPlayerAircraft()->Sms->curWeapon)
				SimDriver.GetPlayerAircraft()->Sms->StepMavSubMode();
			else if(SimDriver.GetPlayerAircraft()->Sms->curWeaponType == wtAim9 || SimDriver.GetPlayerAircraft()->Sms->curWeaponType == wtAim120)
			{
				if(SimDriver.GetPlayerAircraft()->FCC)
					SimDriver.GetPlayerAircraft()->FCC->missileSlaveCmd = TRUE;
			}
		}		
	}
}

void AFCanopyToggle (unsigned long, int state, void*)
{
  if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
  {
		SimDriver.GetPlayerAircraft()->af->CanopyToggle();
		if (OTWDriver.GetVirtualCockpit())
			if (SimDriver.GetPlayerAircraft()->af->canopyState == true)
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_CANOPY, 2);
			else
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_CANOPY, 1);
  }
}
void SimStepComm1VolumeUp(unsigned long val, int state, void *)
{
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		if(OTWDriver.pCockpitManager && OTWDriver.pCockpitManager->mpIcp)
			OTWDriver.pCockpitManager->mpIcp->Comm1Volume--;
		if(OTWDriver.pCockpitManager->mpIcp->Comm1Volume < 0)
			OTWDriver.pCockpitManager->mpIcp->Comm1Volume = 0;
 // MLR 1/29/2004 - 
		PlayerOptions.GroupVol[COM1_SOUND_GROUP] = (int)(RESCALE(OTWDriver.pCockpitManager->mpIcp->Comm1Volume,7,0,-2000,0));
		if(OTWDriver.pCockpitManager->mpIcp->Comm1Volume==8) PlayerOptions.GroupVol[COM1_SOUND_GROUP]=-10000;
		SetVoiceVolume(0);
		DirectVoiceSetVolume(0);
		//int vall = 9 - OTWDriver.pCockpitManager->mpIcp->Comm1Volume;
		int vall = 1<<(8 - OTWDriver.pCockpitManager->mpIcp->Comm1Volume);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_COMM1_VOL, vall);
	}
}
void SimStepComm1VolumeDown(unsigned long val, int state, void *)
{
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		if(OTWDriver.pCockpitManager && OTWDriver.pCockpitManager->mpIcp)
			OTWDriver.pCockpitManager->mpIcp->Comm1Volume++;
		if(OTWDriver.pCockpitManager->mpIcp->Comm1Volume > 8)
			OTWDriver.pCockpitManager->mpIcp->Comm1Volume = 8;
 // MLR 1/29/2004 - 
		PlayerOptions.GroupVol[COM1_SOUND_GROUP] = (int)(RESCALE(OTWDriver.pCockpitManager->mpIcp->Comm1Volume,7,0,-2000,0));
		if(OTWDriver.pCockpitManager->mpIcp->Comm1Volume==8) PlayerOptions.GroupVol[COM1_SOUND_GROUP]=-10000;
		SetVoiceVolume(0);
		DirectVoiceSetVolume(0);
		//int vall = 9 - OTWDriver.pCockpitManager->mpIcp->Comm1Volume;
		int vall = 1<<(8 - OTWDriver.pCockpitManager->mpIcp->Comm1Volume);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_COMM1_VOL, vall);
	}
}
void SimStepComm2VolumeUp(unsigned long val, int state, void *)
{
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		if(OTWDriver.pCockpitManager && OTWDriver.pCockpitManager->mpIcp)
			OTWDriver.pCockpitManager->mpIcp->Comm2Volume--;
		if(OTWDriver.pCockpitManager->mpIcp->Comm2Volume < 0)
			OTWDriver.pCockpitManager->mpIcp->Comm2Volume = 0;
		
 // MLR 1/29/2004 - 
		PlayerOptions.GroupVol[COM2_SOUND_GROUP] = (int)(RESCALE(OTWDriver.pCockpitManager->mpIcp->Comm2Volume,7,0,-2000,0));
		if(OTWDriver.pCockpitManager->mpIcp->Comm2Volume==8) PlayerOptions.GroupVol[COM2_SOUND_GROUP]=-10000;
		SetVoiceVolume(1);
		DirectVoiceSetVolume(1);
		//int vall = 9 - OTWDriver.pCockpitManager->mpIcp->Comm2Volume;
		int vall = 1<<(8 - OTWDriver.pCockpitManager->mpIcp->Comm2Volume);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_COMM2_VOL, vall);
	}
}
void SimStepComm2VolumeDown(unsigned long val, int state, void *)
{
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		if(OTWDriver.pCockpitManager && OTWDriver.pCockpitManager->mpIcp)
			OTWDriver.pCockpitManager->mpIcp->Comm2Volume++;
		if(OTWDriver.pCockpitManager->mpIcp->Comm2Volume > 8)
			OTWDriver.pCockpitManager->mpIcp->Comm2Volume = 8;

 // MLR 1/29/2004 - 
		PlayerOptions.GroupVol[COM2_SOUND_GROUP] = (int)(RESCALE(OTWDriver.pCockpitManager->mpIcp->Comm2Volume,7,0,-2000,0));
		if(OTWDriver.pCockpitManager->mpIcp->Comm2Volume==8) PlayerOptions.GroupVol[COM2_SOUND_GROUP]=-10000;
		SetVoiceVolume(1);
		DirectVoiceSetVolume(1);
		//int vall = 9 - OTWDriver.pCockpitManager->mpIcp->Comm2Volume;
		int vall = 1<<(8 - OTWDriver.pCockpitManager->mpIcp->Comm2Volume);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_COMM2_VOL, vall);
	}
}

void Sim3DCkptHelpOnOff(unsigned long val, int state, void *)
{
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		g_b3DClickableCockpitDebug = !g_b3DClickableCockpitDebug;
	}
}

#ifdef DEBUG
void SimSwitchTextureOnOff(unsigned long val, int state, void *)
{
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		g_bShowTextures = !g_bShowTextures;
	}
}
#endif
void SimSymWheelUp(unsigned long, int state, void*)
{
	if(!g_bRealisticAvionics)
		return;

	if (IO.AnalogIsUsed(AXIS_HUD_BRIGHTNESS) == true)	// Retro 4Jan2004
		return;

	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
  {
		if(TheHud)
		{
			// COBRA - RED - Rewritten in a more sensate way
			TheHud->SymWheelPos += 0.1F;
			if(TheHud->SymWheelPos > 1.0F) TheHud->SymWheelPos = 1.0F;
			if((TheHud->SymWheelPos >= 0.1F)&&(!SimDriver.GetPlayerAircraft()->HasPower(AircraftClass::HUDPower))){
				SimDriver.GetPlayerAircraft()->PowerOn(AircraftClass::HUDPower);
				//TheHud->SymWheelPos = 1.0F;									// COBRA - RED - Power on at MAX Value
			}
			TheHud->SetLightLevel();
			if (OTWDriver.GetVirtualCockpit())
				//OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_ICP_SYM_WHEEL, TheHud->SymWheelPos);
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_ICP_SYM_WHEEL, 1<<((int)(TheHud->SymWheelPos*10.0f)));
		}
  }
}
void SimSymWheelDn(unsigned long, int state, void*)
{
	if(!g_bRealisticAvionics)
		return;

	if (IO.AnalogIsUsed(AXIS_HUD_BRIGHTNESS) == true)	// Retro 4Jan2004
		return;

	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
  {
		if(TheHud)
		{
			// COBRA - RED - Rewritten in a more sensate way
			TheHud->SymWheelPos -= 0.1F;
			if(TheHud->SymWheelPos < 0.0F) TheHud->SymWheelPos=0.0f;
			if((TheHud->SymWheelPos < 0.1F)&&(SimDriver.GetPlayerAircraft()->HasPower(AircraftClass::HUDPower)))
				SimDriver.GetPlayerAircraft()->PowerOff(AircraftClass::HUDPower);
			TheHud->SetLightLevel();
			if (OTWDriver.GetVirtualCockpit())
				//OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_ICP_SYM_WHEEL, TheHud->SymWheelPos);
				OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_ICP_SYM_WHEEL, 1<<((int)(TheHud->SymWheelPos*10.0f)));
		}
  }
}

//THW 2003-11-16 Let's break something just for fun
void SimRandomError(unsigned long, int state, void*)
{
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
    {
		SimDriver.GetPlayerAircraft()->mFaults->RandomFailure();
		/*
		int	failures;	//How many things can possibly fail?
		long failuresPossible; //which Systems are allowed to fail? (Here:All [All Bits set to 1])

		//failuresPossible = 1 + long((2 ^ FaultClass::NumFaultListSubSystems - 1) * rand()/(RAND_MAX + 1.0));
		failuresPossible = 134217728-1; //1 + long(134217728 * rand()/(RAND_MAX + 1.0)); //2^27

		//failures = 1 + int(27 * rand()/(RAND_MAX + 1.0));	//Generate between 1 and 10 errors (theoretical max: 27)
		failures = 27;
		failures = 999;
		
		((AircraftClass*)SimDriver.GetPlayerAircraft())->AddFault(failures, failuresPossible, 1, 1);
		*/
		//THW Now that the systems are broken, add some more damage ;-) [another blatant copy from codec]
		if (rand() % 100 < 20) { // 20% failure chance of A system
			SimDriver.GetPlayerAircraft()->af->HydrBreak (AirframeClass::HYDR_A_SYSTEM);
		}
		if (rand() % 100 < 20) { // 20% failure chance of B system
			SimDriver.GetPlayerAircraft()->af->HydrBreak (AirframeClass::HYDR_B_SYSTEM);
		}
		// also break the generators now and then
		if (rand() % 7 == 1)
			SimDriver.GetPlayerAircraft()->af->GeneratorBreak(AirframeClass::GenStdby);
		if (rand() % 7 == 1)
			SimDriver.GetPlayerAircraft()->af->GeneratorBreak(AirframeClass::GenMain);
    }
}

void SimToggleCockpit(unsigned long, int state, void*)
{
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
  {
		CPPanel* curPanel = OTWDriver.pCockpitManager->GetActivePanel();
		if(!curPanel)
			return;
		if(curPanel->mIdNum == 5000)
		{
			if(SimDriver.GetPlayerAircraft()->WideView)
			{
				if (OTWDriver.pCockpitManager->SetActivePanel(1100))		//Wombat778 4-13-04 Check for the presence of the panel before changing variables
				{
					SimDriver.GetPlayerAircraft()->WideView = FALSE;				
					if (g_fWideviewFOV)						//Wombat778 2-21-04  if g_fWideviewFOV has a value, then set FOV to default when switching to normal view
						OTWDriver.SetFOV(g_fDefaultFOV*DTR);
				}
			}
			else
			{
				if (OTWDriver.pCockpitManager->SetActivePanel(91100))		//Wombat778 4-13-04 Check for the presence of the panel before changing variables
				{
					SimDriver.GetPlayerAircraft()->WideView = TRUE;
					if (g_fWideviewFOV)						//Wombat778 2-21-04  if g_fWideviewFOV has a value, then set it when switching to wideview
						OTWDriver.SetFOV(g_fWideviewFOV*DTR);
				}
			}	
		}
		else if(SimDriver.GetPlayerAircraft()->WideView)
		{
			if (OTWDriver.pCockpitManager->SetActivePanel(curPanel->mIdNum - 90000)) //Wombat778 4-13-04 Check for the presence of the panel before changing variables
			{
				SimDriver.GetPlayerAircraft()->WideView = FALSE;			
				if (g_fWideviewFOV)						//Wombat778 2-21-04  if g_fWideviewFOV has a value, then set FOV to default when switching to normal view
					OTWDriver.SetFOV(g_fDefaultFOV*DTR);
			}
		}
		else
		{
			if (OTWDriver.pCockpitManager->SetActivePanel(curPanel->mIdNum + 90000)) //Wombat778 4-13-04 Check for the presence of the panel before changing variables
			{
				SimDriver.GetPlayerAircraft()->WideView = TRUE;
				if (g_fWideviewFOV)						//Wombat778 2-21-04  if g_fWideviewFOV has a value, then set it when switching to wideview
					OTWDriver.SetFOV(g_fWideviewFOV*DTR);
			}
		}			   
  }
}
void SimToggleGhostMFDs(unsigned long, int state, void*)
{
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
  {
		CPPanel* curPanel = OTWDriver.pCockpitManager->GetActivePanel();
		if(!curPanel)
			return;
		if(curPanel->mIdNum == 5000)
		{
			//Switch it off
			if(SimDriver.GetPlayerAircraft()->WideView)
				OTWDriver.pCockpitManager->SetActivePanel(91100);
			else
				OTWDriver.pCockpitManager->SetActivePanel(1100);
		}
		else
		{
			//Switch it on
			OTWDriver.pCockpitManager->SetActivePanel(5000);
		}			   
  }
}
// JB 020313
void SimFuelDump(unsigned long val, int state, void *)
{
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		float fueltodump = max((SimDriver.GetPlayerAircraft()->af->Fuel() +	SimDriver.GetPlayerAircraft()->af->ExternalFuel()) / 15.0f, 100);
		SimDriver.GetPlayerAircraft()->af->AddFuel(-fueltodump);
	}
}
// JB 020316
void SimCycleDebugLabels(unsigned long val, int state, void *)
{
	if(state & KEY_DOWN)
	{
		if (g_nShowDebugLabels > 0)
		{
			g_nShowDebugLabels *= 2;
			if (g_nShowDebugLabels >= g_nMaxDebugLabel)
				g_nShowDebugLabels = 0;
		}
		else
			g_nShowDebugLabels = 1;
	}
}

// 2002-03-22 ADDED BY S.G.
#include "dogfight.h"
void SimRegen(unsigned long val, int state, void *)
{
	AircraftClass *pac = SimDriver.GetPlayerAircraft();
	if(state & KEY_DOWN && pac){
		if (FalconLocalGame && FalconLocalGame->GetGameType() == game_Dogfight) {
			pac->SetFalcFlag(FEC_REGENERATING);
			pac->SetDead(1);
		}
	}
}
//MI
void SimRangeKnobDown(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;

	// MD -- 20040108: adding support for analog RNG knob.
	// This key does nothing if the RNG is an analog control.
	if (IO.AnalogIsUsed(AXIS_RANGE_KNOB))
		return;

	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{ 
		RadarDopplerClass* theRadard = (RadarDopplerClass*)FindSensor(SimDriver.GetPlayerAircraft(), SensorClass::Radar);
		//MI
		if(theRadard) 
		{			   
			if(theRadard->GetRadarMode() == RadarClass::GM ||
				theRadard->GetRadarMode() == RadarClass::GMT ||
				theRadard->GetRadarMode() == RadarClass::SEA)
			{
				theRadard->StepAGgain(-1);
			}
			else
				theRadard->RangeStep(-1);
		}
	}
} 
void SimRangeKnobUp(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;

	// MD -- 20040108: adding support for analog RNG knob.
	// This key does nothing if the RNG is an analog control.
	if (IO.AnalogIsUsed(AXIS_RANGE_KNOB))
		return;

	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{ 
		RadarDopplerClass* theRadard = (RadarDopplerClass*)FindSensor(SimDriver.GetPlayerAircraft(), SensorClass::Radar);
		//MI
		if(theRadard) 
		{			   
			if(theRadard->GetRadarMode() == RadarClass::GM ||
				theRadard->GetRadarMode() == RadarClass::GMT ||
				theRadard->GetRadarMode() == RadarClass::SEA)
			{
				theRadard->StepAGgain(1);
			}
			else
				theRadard->RangeStep(1);
		}
	}
}

// MD -- 20031003 Adding missing routine for fuel transfer mode switch on the
// FUEL QTY SEL panel
void SimExtFuelTrans(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;
	if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		SimDriver.GetPlayerAircraft()->af->ToggleEngineFlag(AirframeClass::WingFirst);
}

#include "Profiler.h"	// Retro 20Dec2003

void ToggleProfiler(unsigned long, int state, void*)	// Retro 16/10/03
{
#ifdef Prof_ENABLED
   if (state & KEY_DOWN)
   {
	  OTWDriver.ToggleProfilerActive();
   }
#endif	// Prof_ENABLED
}

void Profiler_HistoryBack(unsigned long, int state, void*)	// Retro 22May2004
{
#ifdef Prof_ENABLED
	if (state & KEY_DOWN)
	{
		Prof_move_frame(-1);
	}
#endif	// Prof_ENABLED
}

void Profiler_HistoryBackFast(unsigned long, int state, void*)	// Retro 22May2004
{
#ifdef Prof_ENABLED
	if (state & KEY_DOWN)
	{
		Prof_move_frame(-10);
	}
#endif	// Prof_ENABLED
}

void Profiler_HistoryFwd(unsigned long, int state, void*)	// Retro 22May2004
{
#ifdef Prof_ENABLED
	if (state & KEY_DOWN)
	{
		Prof_move_frame(1);
	}
#endif	// Prof_ENABLED
}

void Profiler_HistoryFwdFast(unsigned long, int state, void*)	// Retro 22May2004
{
#ifdef Prof_ENABLED
	if (state & KEY_DOWN)
	{
		Prof_move_frame(10);
	}
#endif	// Prof_ENABLED
}

void ToggleProfilerDisplay(unsigned long, int state, void*)	// Retro 16/10/03
{
#ifdef Prof_ENABLED
   if (state & KEY_DOWN)
   {
		OTWDriver.ToggleProfilerDisplay();
   }
#endif	// Prof_ENABLED
}

void Profiler_Self(unsigned long, int state, void*)	// Retro 16/10/03
{
#ifdef Prof_ENABLED
   if (state & KEY_DOWN)
   {
	   if (OTWDriver.IsProfilerDisplaying())
	   {
			Prof_set_report_mode(Prof_SELF_TIME);
	   }
   }
#endif	// Prof_ENABLED
}

void Profiler_Hier(unsigned long, int state, void*)	// Retro 16/10/03
{
#ifdef Prof_ENABLED
   if (state & KEY_DOWN)
   {
	   if (OTWDriver.IsProfilerDisplaying())
	   {
			Prof_set_report_mode(Prof_HIERARCHICAL_TIME);
	   }
   }
#endif	// Prof_ENABLED
}

void Profiler_Select(unsigned long, int state, void*)	// Retro 16/10/03
{
#ifdef Prof_ENABLED
   if (state & KEY_DOWN)
   {
	   if (OTWDriver.IsProfilerDisplaying())
	   {
			Prof_select();
	   }
   }
#endif	// Prof_ENABLED
}

void Profiler_Parent(unsigned long, int state, void*)	// Retro 16/10/03
{
#ifdef Prof_ENABLED
   if (state & KEY_DOWN)
   {
	   if (OTWDriver.IsProfilerDisplaying())
	   {
			Prof_select_parent();
	   }
   }
#endif	// Prof_ENABLED
}

void Profiler_CursorUp(unsigned long, int state, void*)	// Retro 16/10/03
{
#ifdef Prof_ENABLED
   if (state & KEY_DOWN)
   {
	   if (OTWDriver.IsProfilerDisplaying())
	   {
			int virtualCursor = Prof_get_cursor();
			virtualCursor--;
			Prof_set_cursor(virtualCursor);
		}
   }
#endif	// Prof_ENABLED
}

void Profiler_CursorDown(unsigned long, int state, void*)	// Retro 16/10/03
{
#ifdef Prof_ENABLED
   if (state & KEY_DOWN)
   {
	   if (OTWDriver.IsProfilerDisplaying())
	   {
			int virtualCursor = Prof_get_cursor();
			virtualCursor++;
			Prof_set_cursor(virtualCursor);
	   }
   }
#endif	// Prof_ENABLED
}

void SimFuelTransNorm(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;
	if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		SimDriver.GetPlayerAircraft()->af->ClearEngineFlag(AirframeClass::WingFirst);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_FUEL_EXT_TRANS, 1);
	}
}

void SimFuelTransWing(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;
	if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		SimDriver.GetPlayerAircraft()->af->SetEngineFlag(AirframeClass::WingFirst);
		if (OTWDriver.GetVirtualCockpit())
			OTWDriver.GetVirtualCockpit()->SetSwitchMask( COMP_3DPIT_FUEL_EXT_TRANS, 2);
	}
}

// Retro 19Dec2003
void ToggleSubTitles(unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
   {
	   OTWDriver.ToggleSubTitles();
   }
}
void ToggleInfoBar(unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
   {
	   OTWDriver.ToggleInfoBar();
   }
}
// Retro 19Dec2003 end

void ToggleDisplacementCam(unsigned long, int state, void*)	// Retro 24Dec2003
{
   if (state & KEY_DOWN)
   {
	   OTWDriver.toggleDisplaceCamera();
   }
}

// Retro 4Jan2004 - winamp commands
extern bool g_bPilotEntertainment;
#include "falcsnd\winampfrontend.h"
void WinAmpNextTrack(unsigned long, int state, void*)
{
	if (state & KEY_DOWN)
	{
		if ((g_bPilotEntertainment)&&(winamp))
		{
			winamp->Next();
		}
	}
}
void WinAmpPreviousTrack(unsigned long, int state, void*)
{
	if (state & KEY_DOWN)
	{
		if ((g_bPilotEntertainment)&&(winamp))
		{
			winamp->Previous();
		}
	}
}
void WinAmpStopPlayback(unsigned long, int state, void*)
{
	if (state & KEY_DOWN)
	{
		if ((g_bPilotEntertainment)&&(winamp))
		{
			winamp->Stop();
		}
	}
}
void WinAmpStartPlayback(unsigned long, int state, void*)
{
	if (state & KEY_DOWN)
	{
		if ((g_bPilotEntertainment)&&(winamp))
		{
			winamp->Start();
		}
	}
}
void WinAmpTogglePlayback(unsigned long, int state, void*)
{
	if (state & KEY_DOWN)
	{
		if ((g_bPilotEntertainment)&&(winamp))
		{
			winamp->TogglePlayback();
		}
	}
}
void WinAmpVolumeUp(unsigned long, int state, void*)
{
	if (state & KEY_DOWN)
	{
		if ((g_bPilotEntertainment)&&(winamp))
		{
			winamp->VolUp();
		}
	}
}
void WinAmpVolumeDown(unsigned long, int state, void*)
{
	if (state & KEY_DOWN)
	{
		if ((g_bPilotEntertainment)&&(winamp))
		{
			winamp->VolDown();
		}
	}
}
// Retro 4Jan2004 - winamp commands end

// Retro 12Jan2004
void CycleEngine(unsigned long, int state, void*)
{
	if (state & KEY_DOWN)
	{
		UserStickInputs.cycleCurrentEngine();
	}
}

void selectLeftEngine(unsigned long, int state, void*)
{
	if (state & KEY_DOWN)
	{
		UserStickInputs.selectLeftEngine();
	}
}

void selectRightEngine(unsigned long, int state, void*)
{
	if (state & KEY_DOWN)
	{
		UserStickInputs.selectRightEngine();
	}
}

void selectBothEngines(unsigned long, int state, void*)
{
	if (state & KEY_DOWN)
	{
		UserStickInputs.selectBothEngines();
	}
}
// Retro 12Jan2004 end

//Wombat778 1-22-04 Keyboard toggle for mouselook mode

void ToggleClickablePitMode(unsigned long, int state, void*)
{
	if (state & KEY_DOWN)
	{
#if 0	// Retro 15Feb2004
		PlayerOptions.SetClickablePitMode(!PlayerOptions.GetClickablePitMode());
#else	// Retro 15Feb2004
extern bool clickableMouseMode;
		clickableMouseMode = !clickableMouseMode;
#endif
	}
}

// Cobra - Make 3D pit mouselook work when TIR is user-selected "On".
void ToggleTIR(unsigned long, int state, void*)
{
	if (state & KEY_DOWN)
	{
		if ((g_bEnableTrackIR) && (PlayerOptions.Get3dTrackIR() == true)) // TIR running?
		{
			if (OTWDriver.IsHeadTracking())
			{
				OTWDriver.SetHeadTracking(FALSE); // 3D pit mouselook On and TIR is Off.
				g_bTrackIRon = false; // Tell trackir
			}
			else
			{
				OTWDriver.SetHeadTracking(TRUE); // 3D pit mouselook Off and TIR is On.
				g_bTrackIRon = true; // Tell trackir
			}
		}
	}
}

//Wombat778 4-12-04  Add a keystroke to jump to a "rear" panel

void SimToggleRearView(unsigned long, int state, void*)
{
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
    {
		CPPanel* curPanel = OTWDriver.pCockpitManager->GetActivePanel();
		if(!curPanel)
			return;
		if(curPanel->mIdNum == 5100)
		{
			//Switch it off
			if(SimDriver.GetPlayerAircraft()->WideView)
				OTWDriver.pCockpitManager->SetActivePanel(91100);
			else
				OTWDriver.pCockpitManager->SetActivePanel(1100);
		}
		else
		{
			//Switch it on
			OTWDriver.pCockpitManager->SetActivePanel(5100);
		}			   
    }
}

void SimToggleAltView(unsigned long, int state, void*)
{
	if(SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
    {
		CPPanel* curPanel = OTWDriver.pCockpitManager->GetActivePanel();
		if(!curPanel)
			return;
		if(curPanel->mIdNum == OTWDriver.pCockpitManager->AltPanel())
		{
			//Switch it off
			if(SimDriver.GetPlayerAircraft()->WideView)
				OTWDriver.pCockpitManager->SetActivePanel(91100);
			else
				OTWDriver.pCockpitManager->SetActivePanel(1100);
		}
		else
		{
			//Switch it on
			OTWDriver.pCockpitManager->SetActivePanel(OTWDriver.pCockpitManager->AltPanel());
		}			   
    }
}

//Wombat778 End
