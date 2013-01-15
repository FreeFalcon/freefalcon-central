#include "stdhdr.h"
#include "radarDoppler.h"
#include "simfile.h"
#include "object.h"
#include "simbase.h"
#include "simdrive.h"
#include "aircrft.h"
#include "dispcfg.h"
#include "fcc.h"
#include "mfd.h"
#include "sms.h"
#include "Graphics\Include\gmComposit.h"
#include "Graphics\Include\Mono2d.h"
#include "camp2sim.h"
#include "MsgInc\TrackMsg.h"
#include "team.h"
#include "hud.h"
//MI for master mode
#include "OTWDrive.h"
#include "cpmanager.h"
#include "icp.h"

extern bool g_bSmartCombatAP;
extern bool g_bIFF;	//MI
extern bool g_bMLU; //MI
extern bool g_bAGRadarFixes;	//MI

RadarDopplerClass::RadarDopplerClass (int type, SimMoverClass* self) : RadarClass (type, self)
{
	rangeScales[0] = 10.0F;
	rangeScales[1] = 20.0F;
	rangeScales[2] = 40.0F;
	rangeScales[3] = 80.0F;
	rangeScales[4] = 160.0F;
	velScales[0] = 1200.0F;
	velScales[1] = 2400.0F;
	rwsAzs[0] = 30.0F * DTR;
	rwsAzs[1] = 10.0F * DTR;
	rwsAzs[2] = 60.0F * DTR;
	rwsBars[0] = 1;
	rwsBars[1] = 2;
	rwsBars[2] = 4;
	twsAzs[0] = 10.0F * DTR;
	twsAzs[1] = 60.0F * DTR;
	twsAzs[2] = 25.0F * DTR;
	twsBars[0] = 4;
	twsBars[1] = 2;
	twsBars[2] = 3;
	vsVelIdx = 0;
	gmRangeIdx = 2;
	airRangeIdx = 2;
	twsAzIdx = lastTwsAzIdx = 2;  // MD
	rwsAzIdx = 2;
	vsAzIdx = 2;
	//MI
	gmAzIdx = 2;
	gmtAzIdx = 2;
	rwsBarIdx = 2;
	twsBarIdx = lastTwsBarIdx = 2;  // MD
	gmBarIdx = 0;
	vsBarIdx = 2;
	scanRate = 60.0F * DTR;
	designateCmd		= FALSE;
	dropTrackCmd		= FALSE;
	rangeChangeCmd		= FALSE;
	scanHeightCmd		= FALSE;
	scanWidthCmd		= FALSE;
	elSlewCmd			= FALSE;
	targetAz			= 0.0F;
	targetEl			= 0.0F;
	curScanTop			= 0.0F;
	curScanBottom		= 0.0F;
	curScanLeft			= 0.0F;
	curScanRight		= 0.0F;
	scanCenterAlt		= 0.0F;
	lastBars			= 1;
	cursRange			= 0.0F;
	subMode				= FALSE;
	lastFeatureUpdate = 0;
	reacqFlag         = 0;
	reacqEl           = 0.0F;
	displayAzScan     = 0.0F;
	displayRange = 10.f; // something JPO
	azScan = 60.0F * DTR;
	elScan = 0;

	beamAz = 0.0F;
	beamEl = 0.0F;
	mode = prevMode = RWS;
	Missovrradarmode = RWS;//me123
	Dogfovrradarmode = ACM_30x20;//me123
	noovrradarmode = RWS; //me123
	Overridemode = 0;//me123
	cursorX = cursorY = 0.0F;
	flags = NORM;
	lockedTargetData = NULL;
	curCursorRate = CursorRate;
	centerCmd = FALSE; // JPO initialise

	// MD -- 20040116: TWS mode init
	// MD -- 20040204: moved up to avoid CTD in ChangeMode()
	TWSTrackDirectory = (TWSTrackList *) NULL;
	GMSPPseudoWaypt = NULL;  // MD -- 20040214: pseudo waypoint for ground stabilised GM SP mode

	ChangeMode (mode);
	SetScan();

	// GM mode init
	GMFeatureListRoot = NULL;
	GMMoverListRoot = NULL;
	gainCmd = 0.0f;
	groundLookAz = 0.0F;

	//JPO initialise the stuff
	channelno = 1;
	histno = 4;
	level = 1;
	mkint = 1;
	bdelay = 0.0f;
	radarmodeflags = 0;
	fovStepCmd = 0;	//MI
	// JPO default declutter modes
	agdclt = DefaultAgDclt;
	aadclt = DefaultAaDclt;

	//MI
	DrawRCR = FALSE;
	//MI IFF stuff
	InterogateTimer = 0.0F;
	DoInterogate = FALSE;
	LOS = FALSE;
	SCAN = FALSE;
	CountUp = FALSE;
	Input = 0.0F;
	InterogateDelay = 0.5F;
	Mode1 = FALSE;
	Mode2 = FALSE;
	Mode3 = FALSE;
	Mode4 = FALSE;
	ModeC = FALSE;
	InterogateModus = 0;
	CmdLOS = FALSE;
	Pointer = 0;

	//MI remember our last MissOver Range
	MissOvrRangeIdx = 1;
	lastAirRangeIdx = 2;

	//MI
	LastAAMode = RWS;
	LastNAVMode = RWS;
	LastAGMode = GM;
	LastAGModes = 3;
	WasAutoAGRange = TRUE;
	GainPos = 10.0F;
	InitGain = TRUE;
	lastRngKnobPos = 0;
	antElevKnob = 0.0f;//TJL 05/30/04 This fixes the -99/-99 error some are having
//	GMTSlowSpeedReject = 5.0F;	MN externalised
//	GMTHighSpeedReject = 100.0F;
	iffmodeflags = 0;//Cobra 11/24/04
	wipeIFF = FALSE;//Cobra 11/24/04
	iffTimer = 0.0f;//Cobra 11/24/04
}

RadarDopplerClass::~RadarDopplerClass (void)
{
   FreeGMList (GMFeatureListRoot);
   FreeGMList (GMMoverListRoot);
   GMFeatureListRoot = NULL;
   GMMoverListRoot = NULL;
   ClearSensorTarget();

   // MD -- 20040116: clean up TWS directory
   if (TWSTrackDirectory) {
	   TWSTrackDirectory = TWSTrackDirectory->Purge();
   }

}

void RadarDopplerClass::DisplayInit (ImageBuffer* newImage)
{
	DisplayExit();
	
	privateDisplay = new RenderGMComposite;
	((RenderGMComposite*)privateDisplay)->Setup(newImage, AddTargetReturnCallback, this);
	
	// Prep GM Radar
	if (mode == GM || mode == GMT || mode == SEA)
		SetGMScan();
	
	privateDisplay->SetColor (0xff00ff00);
}

void RadarDopplerClass::SetSensorTarget (SimObjectType* newTarget)
{
	if (newTarget)
		lockedTargetData = newTarget->localData;
	else
		lockedTargetData = NULL;
	RadarClass::SetSensorTarget(newTarget);
}

void RadarDopplerClass::ClearSensorTarget (void)
{
	 if (lockedTarget)
	{
		SendTrackMsg (lockedTarget, Track_Unlock);
	}
	lockedTargetData = NULL;
	RadarClass::ClearSensorTarget();
}

// JPO split by mode for simplicity
void RadarDopplerClass::PushButton(int whichButton, int whichMFD)
{
    switch (whichButton) { // common cases
    case 4:
	if (g_bRealisticAvionics)
	    ToggleFlag(CtlMode);
	break;
    case 3:
		//MI if we're in STBY, don't do anything with it.
		if(mode == STBY)
			return;
	SetEmitting (1 - isEmitting);
	break;

    case 11:
	if (g_bRealisticAvionics)
	    MfdDrawable::PushButton(whichButton, whichMFD);
	else if (mode != OFF) {
	    MfdDisplay[whichMFD]->SetNewMode(MFDClass::SMSMode);
	}
	break;
    case 12:
	if (g_bRealisticAvionics)
	    MfdDrawable::PushButton(whichButton, whichMFD);
	break;
    case 13:
	if (g_bRealisticAvionics)
	    MfdDrawable::PushButton(whichButton, whichMFD);
	else if (mode == OFF) {
	    MfdDisplay[whichMFD]->SetNewMode(MFDClass::MfdMenu);
	} else {
	    modeDesiredCmd = STBY;
	}
	break;
    case 14:
	if (g_bRealisticAvionics)
	    MfdDrawable::PushButton(whichButton, whichMFD);
	else MFDSwapDisplays();
	break;
    case 0:
	if (g_bRealisticAvionics) {
	    SetFlagBit(MenuMode);
	    break;
	}
	// else fall through

    default:
	// catch the menu and ctl buttons
	if (whichButton > 4 && whichButton < 20) {
	    if (IsSet(MenuMode)) {
		MenuPushButton (whichButton, whichMFD);
		return;
	    }
	    else if (IsSet(CtlMode)) {
		CtlPushButton (whichButton, whichMFD);
		return;
	    }
	}
	switch (mode) {
	case GM:
	case GMT:
	case SEA:
	    AGPushButton(whichButton, whichMFD);
	    break;
	case OFF:
	//case STBY:	//MI don't think we should do anything here. (acts weird now)
	    OtherPushButton(whichButton, whichMFD);
	    break;
	default:
	    AAPushButton(whichButton, whichMFD);
	    break;
	    
	}
	break;
    }
}

// AG specific buttons
void RadarDopplerClass::AGPushButton(int whichButton, int whichMFD)
{
    switch( whichButton ) {
    case 0:
	StepAGmode();
	break;
    case 1:
	if(g_bRealisticAvionics && g_bAGRadarFixes)
	{
		ToggleFlag(AutoAGRange);
		if(IsSet(AutoAGRange))
			WasAutoAGRange = TRUE;
		else
			WasAutoAGRange = FALSE;
	    //StepAGmode();	//MI this should definately not be here
	}
	else if(!g_bRealisticAvionics)
		StepAGmode();
	break;
    case 2:
	fovStepCmd = TRUE;
	break;
    case 6:
	ToggleAGfreeze();
	//LastAGModes = 1; // ASSOCIATOR: Redundant now
	break;
    case 7:
	SetAGSnowPlow(TRUE);
	if (g_bRealisticAvionics && g_bAGRadarFixes)
	{
		RestoreAGCursor();
	}
	LastAGModes = 2;
	break;
    case 8:
	ToggleAGcursorZero();
	break;
    case 9:
	SetAGSteerpoint(TRUE);
	if (g_bRealisticAvionics && g_bAGRadarFixes)
		RestoreAGCursor();
	LastAGModes = 3;
	break;
    case 10:
	if (g_bRealisticAvionics)
	    ToggleFlag(AGDecluttered);
	break;

    case 16:
	if (mode == OFF) {
	    modeDesiredCmd = ACM_30x20;
	} else {
	    scanHeightCmd = TRUE;
	}
	break;
    case 17:
	scanWidthCmd = TRUE;
	break;
    case 18:
	rangeChangeCmd = -1;
	//MI
	if(g_bRealisticAvionics && g_bAGRadarFixes)
	{
		if(IsSet(AutoAGRange))
		{
			ClearFlagBit(AutoAGRange);
			WasAutoAGRange = FALSE;
		}
	}
	break;
    case 19:
	rangeChangeCmd = 1;
	//MI
	if(g_bRealisticAvionics && g_bAGRadarFixes)
	{
		if(IsSet(AutoAGRange))
		{
			ClearFlagBit(AutoAGRange);
			WasAutoAGRange = FALSE;
		}
	}
	break;
	
    }
}
void RadarDopplerClass::AAPushButton(int whichButton, int whichMFD)
{
    switch( whichButton ) {
    case 0:
	StepAAmode();
	break;
    case 1:
	if (g_bRealisticAvionics)
	    StepAAmode ();
	else if ((mode == ACM_30x20)	|| (mode == ACM_SLEW) || (mode == ACM_BORE)
	    || (mode == ACM_10x60))
	{
	    scanWidthCmd = TRUE;
	}
	break;
    case 2:
	if ((mode == TWS || mode == RWS || mode == LRS || mode == SAM) && g_bRealisticAvionics)
	    	fovStepCmd = TRUE;
	break;
    case 10:
	if (g_bRealisticAvionics)
	    ToggleFlag(AADecluttered);
	break;
    case 16:
	scanHeightCmd = TRUE;
	break;
    case 17:
	if ((mode == RWS) || (mode == TWS) || (mode == VS) || (mode == LRS)) {
	    scanWidthCmd = TRUE;
	}
	break;
    case 18:
	if ((mode != ACM_SLEW)	&& (mode != ACM_30x20)	&& 
	    (mode != ACM_10x60)	&& (mode != ACM_BORE)) 
	{
	    rangeChangeCmd = -1;
	}
	break;
    case 19:
	if ((mode != ACM_SLEW)	&& (mode != ACM_30x20)	&& 
	    (mode != ACM_10x60)	&& (mode != ACM_BORE)) 
	{
	    rangeChangeCmd = 1;
	}
	break;
	
    }
}

void RadarDopplerClass::OtherPushButton(int whichButton, int whichMFD)
{
    switch( whichButton ) {
    case 0:
	break;
    case 5:
	modeDesiredCmd = GM;
	break;
    case 6:
	modeDesiredCmd = GMT;
	break;
    case 7:
	modeDesiredCmd = SEA;
	break;
    case 16:
	modeDesiredCmd = ACM_30x20;
	break;
    case 17:
	modeDesiredCmd = VS;
	break;
    case 18:
	modeDesiredCmd = RWS;
	break;
    case 19:
	modeDesiredCmd = TWS;
	break;
    }
}

static const struct RadarMenus {
    char *label1, *label2;
    RadarClass::RadarMode mode;
} rmenu[20] = {
#define NoLabel {NULL, NULL, RadarClass::RadarMode::OFF}
    NoLabel,
    NoLabel,
    NoLabel,
    NoLabel,
    NoLabel,
    {"GM", NULL, RadarClass::RadarMode::GM}, // 5
    {"GMT", NULL, RadarClass::RadarMode::GMT},
    {"SEA", NULL, RadarClass::RadarMode::SEA},
    {"BCN", NULL, RadarClass::RadarMode::BCN},
    {"STBY", NULL, RadarClass::RadarMode::STBY}, // 9
    NoLabel,
    NoLabel,
    NoLabel,
    NoLabel,
    NoLabel,
    NoLabel,
    NoLabel,
    NoLabel,
    {"ACM", NULL, RadarClass::RadarMode::ACM_30x20},
    {"CRM", NULL, RadarClass::RadarMode::RWS}, //20
};
//MI
static const struct RadarMenusAA {
    char *label1, *label2;
    RadarClass::RadarMode mode;
} rmenuaa[20] = {
#define NoLabel {NULL, NULL, RadarClass::RadarMode::OFF}
    NoLabel,
    NoLabel,
    NoLabel,
    NoLabel,
    NoLabel,
    NoLabel, // 5
    NoLabel,
    NoLabel,
    NoLabel,
    {"STBY", NULL, RadarClass::RadarMode::STBY}, // 9
    NoLabel,
    NoLabel,
    NoLabel,
    NoLabel,
    NoLabel,
    NoLabel,
    NoLabel,
    NoLabel,
    {"ACM", NULL, RadarClass::RadarMode::ACM_30x20},
    {"CRM", NULL, RadarClass::RadarMode::RWS}, //20
};
static const struct RadarMenusAG {
    char *label1, *label2;
    RadarClass::RadarMode mode;
} rmenuag[20] = {
#define NoLabel {NULL, NULL, RadarClass::RadarMode::OFF}
    NoLabel,
    NoLabel,
    NoLabel,
    NoLabel,
    NoLabel,
    {"GM", NULL, RadarClass::RadarMode::GM}, // 5
    {"GMT", NULL, RadarClass::RadarMode::GMT},
    {"SEA", NULL, RadarClass::RadarMode::SEA},
    {"BCN", NULL, RadarClass::RadarMode::BCN},
    {"STBY", NULL, RadarClass::RadarMode::STBY}, // 9
    NoLabel,
    NoLabel,
    NoLabel,
    NoLabel,
    NoLabel,
    NoLabel,
    NoLabel,
    NoLabel,
    {"ACM", NULL, RadarClass::RadarMode::ACM_30x20}, // ASSOCIATOR: Added these 2 modes to AG Radar Menu
    {"CRM", NULL, RadarClass::RadarMode::RWS}, //20
};

static const struct RadarMenus cmenu[20] = {
    NoLabel,
    NoLabel,
    NoLabel,
    NoLabel,
    NoLabel,
    {"CHAN", "2", RadarClass::RadarMode::OFF}, // 5
    {"MK INT", "3", RadarClass::RadarMode::OFF},
    {"BAND", "NARO", RadarClass::RadarMode::OFF},
    {"BCN DLY", "00.0", RadarClass::RadarMode::OFF},
    /*{"PM", "OFF", RadarClass::RadarMode::OFF},*/
    NoLabel,
    NoLabel,
    NoLabel,
    NoLabel,
    NoLabel,
    NoLabel,
    {"LVL", "1", RadarClass::RadarMode::OFF},
    {"TGT HIS", "3", RadarClass::RadarMode::OFF},
    {"ALT TRK", "OFF", RadarClass::RadarMode::OFF},
    {"MTR", "LO", RadarClass::RadarMode::OFF}, //20
};

void RadarDopplerClass::MENUDisplay(void) 
{
    if (IsSet(MenuMode)) {
	for (int i = 0; i < 20; i++) 
	{
		//MI make it master mode dependant
		if(OTWDriver.pCockpitManager && OTWDriver.pCockpitManager->mpIcp &&
			OTWDriver.pCockpitManager->mpIcp->IsICPSet(ICPClass::MODE_A_A))
		{
			if(rmenuaa[i].label1)
				LabelButton(i, rmenuaa[i].label1, rmenuaa[i].label2, mode == rmenuaa[i].mode);
		}
		else if(OTWDriver.pCockpitManager && OTWDriver.pCockpitManager->mpIcp &&
			OTWDriver.pCockpitManager->mpIcp->IsICPSet(ICPClass::MODE_A_G))
		{
			if(rmenuag[i].label1)
				LabelButton(i, rmenuag[i].label1, rmenuag[i].label2, mode == rmenuag[i].mode);
		}
		else
		{
			if(rmenu[i].label1)
			LabelButton(i, rmenu[i].label1, rmenu[i].label2, mode == rmenu[i].mode);
		}
	}
    }
    else {
	char tbuf[80];

	sprintf (tbuf, "%d", channelno);
	LabelButton(5, "CHAN", tbuf);
#if 0 // while we work out if this is ok.
	sprintf (tbuf, "%d", mkint);
	LabelButton(6, "MK INT", tbuf);
	LabelButton(7, "BAND", IsModeFlag(NaroBand) ? "NARO" : "WIDE");
	sprintf (tbuf, "%.2f", bdelay);
	LabelButton(8, "BCN DLY", tbuf);
	LabelButton(9, "PM", IsModeFlag(PmMode) ? "ON" : "OFF");

	sprintf (tbuf, "%d", level);
	LabelButton(16, "LVL", tbuf);
#endif
	if (IsIFFFlags(Dcpl))
		LabelButton(9, "DCPL");//Cobra
	else
		LabelButton(9, "CPL");//Cobra

	sprintf (tbuf, "%d", histno);
	LabelButton(17, "TGT HIS", tbuf);
#if 0 // while we work out if this is ok.
	LabelButton(18, "ALT TRK", IsModeFlag(AltTrack) ? "ON" : "OFF");
#endif
	LabelButton(19, "MTR", IsModeFlag(SpeedLo) ? "LO" : "HIGH");
    }
    BottomRow();
}

void RadarDopplerClass::MenuPushButton(int whichButton, int whichMFD)
{
	//MI changed
	if(OTWDriver.pCockpitManager && OTWDriver.pCockpitManager->mpIcp &&
		OTWDriver.pCockpitManager->mpIcp->IsICPSet(ICPClass::MODE_A_A))
	{
		if(rmenuaa[whichButton].label1)
		{
			modeDesiredCmd = rmenuaa[whichButton].mode;
			ClearFlagBit(MenuMode);
		}			
	}
	else if(OTWDriver.pCockpitManager && OTWDriver.pCockpitManager->mpIcp &&
		OTWDriver.pCockpitManager->mpIcp->IsICPSet(ICPClass::MODE_A_G))
	{
		if(rmenuag[whichButton].label1)
		{
			modeDesiredCmd = rmenuag[whichButton].mode;
			ClearFlagBit(MenuMode);
		}
	}
	else
	{
		if(rmenu[whichButton].label1) 
		{
			modeDesiredCmd = rmenu[whichButton].mode;
			ClearFlagBit(MenuMode);
		}
	}
}

void RadarDopplerClass::CtlPushButton(int whichButton, int whichMFD)
{
    switch (whichButton) {
    case 5:
	if (++channelno > 4)
	    channelno = 1;
	break;
    case 6:
	if (++mkint > 4)
	    mkint = 1;
	break;
    case 7:
	ToggleModeFlag (NaroBand);
	break;
    case 8:
	break;
    case 9:
	//ToggleModeFlag(PmMode);
	ToggleIFFFlags(Dcpl);//Cobra
	break;
    case 16:
	if (++level > 4)
	    level = 1;
	break;
    case 17:
	if (++histno > NUM_RADAR_HISTORY)
	    histno = 1;
	break;
    case 18:
	ToggleModeFlag(AltTrack);
	break;
    case 19:
	ToggleModeFlag(SpeedLo);
	break;
    }
}

SimObjectType* RadarDopplerClass::Exec (SimObjectType* targetList)
{
	int i, rngCell[MAX_OBJECTS], angCell[MAX_OBJECTS], velCell[MAX_OBJECTS];
	SimObjectType* rdrObj;
	SimObjectType* lastLocked;
	SimObjectLocalData* rdrData;
	float rVal, delta;
	int sendThisFrame;
	int lockedFound = FALSE;
	int testTime;

    if (!((AircraftClass*)platform)->HasPower(AircraftClass::FCRPower)) {
		SetPower(FALSE);
		SetEmitting(FALSE);
		if (mode != OFF) {
			prevMode = mode;
			mode = OFF;
		}
    }
    else if (mode == OFF) {
		modeDesiredCmd = STBY;
    }
	
	// Update the hud
	if (TheHud) // JB 010615 CTD
	{
		if (isEmitting)
			TheHud->HudData.Clear(HudDataType::RadarNoRad);
		else if (radarData->NominalRange != 0.0) // JB 010706 Only set if the aircraft has radar to begin with
			TheHud->HudData.Set(HudDataType::RadarNoRad);
	}
	
	// Validate our locked target
	lastLocked = lockedTarget;
	if (lastLocked)
		lastLocked->Reference();
	
	CheckLockedTarget();
	
	// Maintain track history across agg/deag boundary
	if (lockedTarget != lastLocked && lockedTarget && lastLocked)
	{
		ShiAssert( lockedTargetData == lockedTarget->localData );
		memcpy (lockedTargetData, lastLocked->localData, sizeof (SimObjectLocalData));
	}
	if (lastLocked)
		lastLocked->Release();
	lastLocked = lockedTarget;

	if(!lastLocked) // MLR 6/21/2004 - So cursors come back after loosing tgt
	{
		ClearFlagBit(STTingTarget);
	}
	
	// JB 010224 Start Enable the CombatAP to shoot A2A missiles
	//if (g_bSmartCombatAP && lockedTarget) && 
	if (g_bSmartCombatAP && lockedTarget && ((mode > 1) && (mode < 14)) && 
		((AircraftClass*) platform)->autopilotType == AircraftClass::CombatAP	)
	{
		int digimode;
		switch (mode)
		{
		case RWS:
		case LRS:
		case TWS:
		case VS:
		case ACM_30x20:
		case ACM_SLEW:
		case ACM_BORE:
		case ACM_10x60:
		case SAM:
		case SAM_AUTO_MODE:
		case SAM_MANUAL_MODE:
		case STT:
			digimode = AA;
			break;
		default:
			digimode = 0;
		}
		
#define SG_NOLOCK 0x00
#define SG_LOCK 0x01
#define SG_JAMMING 0x02
#define SG_FADING 0x04
		// Weight on wheels inhibit
		if (platform->IsAirplane())
		{
			if (platform->OnGround())
			{
				SetSeekerPos( 0.0f, 0.0f );
				platform->SetRdrRng( 0.0F );
				platform->SetRdrAz( 0.0F );
				platform->SetRdrEl( 0.0F );
				platform->SetRdrCycleTime( 9999.0F );//me123
				platform->SetRdrAzCenter( 0.0f );
				platform->SetRdrElCenter( 0.0f );
				SetDesiredTarget( NULL );
				return NULL;
			}
			
			// Original code check for isEmiting again, its useless since we return if not emiting above anyway
			platform->SetRdrRng( radarData->NominalRange );
			platform->SetRdrAz( radarData->ScanHalfAngle );
			platform->SetRdrEl( radarData->ScanHalfAngle );
			platform->SetRdrCycleTime( 8.0F );//me123
			platform->SetRdrAzCenter( 0.0f );
			platform->SetRdrElCenter( 0.0f );
		}
		
		// See if it is time to send a "painted" list update
		if (lastLocked != lockedTarget)
		{
			if (lastLocked)
			{
				SendTrackMsg (lastLocked, Track_Unlock);
			}
			sendThisFrame = TRUE;
		} else {
			// See if it is time to send a "painted" list update (AA mode only)
			sendThisFrame = (SimLibElapsedTime - lastTargetLockSend > TrackUpdateTime);
		}
		
		// S.G. Now this is where we scan for each target in our target list, à la EyeballClass::Exec
		SimObjectType* tmpPtr = targetList;
		
		// Just in case we don't have a list but we do have a locked target OR IF THE RADAR IS NOT IN AA MODE
		// I noticed the radar isn't really used in air to ground mode so we'll do just the lockedTarget then					
		if (!tmpPtr || digimode != AA)
			tmpPtr = lockedTarget;
		
		while (tmpPtr) {
			// S.G. canSee has THREE meaning now. Bit 0 is when we can see signal, bit 1 is when the signal is jammed and bit 2 is when the signal is fading
			// By default, we can see the target (bit 0 at value 1)
			int canSee = SG_LOCK;

			// 2000-10-07 S.G. POSSIBLE BUG! If we are looking at our lockedTarget and we are the only one referencing it, clearing it
			// might invalidate tmpPtr->next. So I'll read it ahead of time
			SimObjectType* tmpPtrNext = tmpPtr->next;

			// FRB - CTD below
			if ((!tmpPtr->BaseData()) || (!tmpPtr->localData))
				tmpPtr = tmpPtrNext;
			if (!tmpPtr)
				break;

			
			if (TeamInfo[platform->GetTeam()]->TStance(tmpPtr->BaseData()->GetTeam()) <= Neutral)
			{//me123 don't lock up freindlys
				canSee = SG_NOLOCK;
			}
			// Can't hold a lock if its outside our radar cone
			if (fabs(tmpPtr->localData->ata) > radarData->ScanHalfAngle)
			{
				canSee = SG_NOLOCK;
			}
			else {
				// Only track object in the correct domain (air/land)
				if ( tmpPtr->BaseData()->OnGround() )
				{
					if (digimode == AA)
					{
						// Skip ground objects in AA mode
						canSee = SG_NOLOCK;
					}
				}
				else
				{
					// Ground mode is more than just GM, it can be GMT and SEA as well (not sure about SEA though, but gndAttck.cpp DO put the digital radar in GMT mode)
					// But AFAIK, if NOT in AA, you're in GM or GMT mode (SEA is NOT used by AI after all)
					if (digimode != AA)
					{
						// Skip air objects in AA mode
						canSee = SG_NOLOCK;
					}
				}
			}

			// Now we check the signal strength only if the target is in the radar cone and the right mode
			if (canSee) {
				// Drop lock if the guy has been below the signal strength threshhold too long
				if (ReturnStrength(tmpPtr) < 1.0f) {
					// Ok so it's too low, but is it jamming? If so, follow anyway...
					if (tmpPtr->BaseData()->IsSPJamming())
						canSee |= SG_JAMMING; // That's our second bit being used
					// So it's too low and were are not jamming. When did we loose the signal?
					else if (SimLibElapsedTime - tmpPtr->localData->rdrLastHit > radarData->CoastTime) {
						// Give up and drop lock
						canSee = SG_NOLOCK;
					}
					// We just lost the signal, but we can still follow it, right?
					else
						canSee |= SG_FADING;
				}
			}
			
			// Now set that target sensor track according to what we got
			if (canSee) {		
				
				//me123 first time we atempt a lock, it requires some time to lock it up
				if (tmpPtr->localData->sensorState[Radar] == NoTrack) 
				{
					tmpPtr->localData->rdrLastHit = SimLibElapsedTime;// we are starting to lock the guy
					canSee |= SG_FADING; // this will make the sensor state max set to detection
				}
				if (radarDatFile && tmpPtr->localData->sensorState[Radar] == Detection &&  SimLibElapsedTime - tmpPtr->localData->rdrLastHit < (unsigned)radarDatFile->TimeToLock)
				{
					canSee |= SG_FADING;// we are attempting a lock so don't go higher then detection
				}
				
				// Can we see it (either with a valid lock, a jammed or fading signal?
				if (canSee & (SG_JAMMING | SG_FADING))						// Is it a jammed or fading signal?
					tmpPtr->localData->sensorState[Radar] = Detection;			// Yep, say so (weapon can't lock on 'Detection' but digi plane can track it)
				else
					tmpPtr->localData->sensorState[Radar] = SensorTrack;		// It's a valid lock, mark it as such. Even when fading, we can launch
				
				if (!(canSee & SG_FADING))									// Is the signal fading?
					tmpPtr->localData->rdrLastHit = SimLibElapsedTime;// No, so update the last hit field
			}
			else
				tmpPtr->localData->sensorState[Radar] = NoTrack;				// Sorry, we lost that target...
			//
			// 2000-10-07 S.G. POSSIBLE BUG! If we are looking at our lockedTarget and we are the only one referencing it, clearing it
			// might invalidate tmpPtr->next. So I'll read it ahead of time
			//SimObjectType* tmpPtrNext = tmpPtr->next;
			tmpPtrNext = tmpPtr->next;
			
			// Now is it time to do lockedTarget housekeeping stuff?
			// 2000-09-18 S.G. We need to check the base data because targetList item and lockedData might not be the same
			if (lockedTarget && tmpPtr->BaseData() == lockedTarget->BaseData()) 
			{
				// 2000-09-18 S.G. Update the lockedTarget radar sensor state with what we just calculated.
				lockedTarget->localData->sensorState[Radar] = tmpPtr->localData->sensorState[Radar];
				
				// If we can see our target, Tell the base class and the rest of the world where we're looking (if we are looking somewhere)
				if (canSee) 
				{
					tmpPtr->localData->painted = TRUE;
					// FRB - rdrObj and rdrData not intialized yet
					//tmpPtr->localData->rdrDetect = rdrData->rdrDetect >> 1;
					SetSeekerPos( TargetAz(platform, tmpPtr), TargetEl(platform, tmpPtr) );
					platform->SetRdrAz( radarData->BeamHalfAngle );
					platform->SetRdrEl( radarData->BeamHalfAngle );
					// 2002-02-10 MODIFIED BY S.G. Different radar cycle timer for different radar mode
					if (digiRadarMode == DigiSTT)
						platform->SetRdrCycleTime( 0.5F ); // Original line used to be 0.0f Made it 0.5f like the digiRadar
					else if (digiRadarMode == DigiSAM)
						platform->SetRdrCycleTime( 3.0F );
					else if (digiRadarMode == DigiTWS)
						platform->SetRdrCycleTime( 3.0F );
					else
						platform->SetRdrCycleTime( 6.0F );
					platform->SetRdrAzCenter( tmpPtr->localData->az );
					platform->SetRdrElCenter( tmpPtr->localData->el );
					
					// Tag the target as seen from this frame, unless the target is fading
					if (!(canSee & SG_FADING)) 
					{
						if (sendThisFrame) 
						{
							SendTrackMsg( tmpPtr, Track_Lock, digiRadarMode );
							lastTargetLockSend = SimLibElapsedTime;
						}
					}
				}
				// If we cannot see it, we clear it...
				else
					SetDesiredTarget( NULL );
			}
			
			// Now it's time to try our next target... (2000-10-07: We'll use what we saved earlier)
			tmpPtr = tmpPtrNext;
		}
		
		// If we do not have a locked target, leave the radar centered...
		if (!lockedTarget)
		{
			SetSeekerPos( 0.0f, 0.0f );
			platform->SetRdrAz( radarData->ScanHalfAngle );
			platform->SetRdrEl( radarData->ScanHalfAngle );
			platform->SetRdrCycleTime( 8.0F );
			platform->SetRdrAzCenter( 0.0f );
			platform->SetRdrElCenter( 0.0f );

			// FRB 
			if (SimDriver.GetPlayerAircraft()->AutopilotType() == AircraftClass::CombatAP)
				platform->SetRdrCycleTime( 3.0F );

		}

		return lockedTarget;
	}
	// JB 010224 End Enable the CombatAP to shoot A2A missiles

	// JPO - radar doesn't scan with WOW
	//if (g_bRealisticAvionics && platform->OnGround()) 
	//{ 
	//	// do nothing
	//}
	//else if (isEmitting) // only scans if its scanning!

	// FRB - Some problems interpreting the above conditionals
	if (!platform->OnGround() && (isEmitting)) 
	//if (isEmitting) 
	{
		MoveBeam();
	}
	
	/*------------------------*/
	/* Go through the objects */
	/*------------------------*/
	i = 0;
	rdrObj = targetList;
	
	// Hack -- You really can lock a target, etc. in these modes...
	if (mode == GM || mode == GMT || mode == SEA)
		return (NULL);
	
	targetUnderCursor = FalconNullId;
	
	while (rdrObj)
	{
		if (rdrObj == lockedTarget)
			lockedFound = TRUE;
		
		// Is this the target under the cursor?
		if (mode != VS)
		{
			if (IsUnderCursor(rdrObj, platform->Yaw()))
				targetUnderCursor = rdrObj->BaseData()->Id();
		}
		else
		{
			if (IsUnderVSCursor(rdrObj, platform->Yaw()))
				targetUnderCursor = rdrObj->BaseData()->Id();
		}
		
		/*-----------------*/
		/* Clear res cells */
		/*-----------------*/
		rngCell[i] = -1;
		angCell[i] = -1;
		velCell[i] = -1;
		rdrData = rdrObj->localData;

		// Did the beam cross the object
		if (isEmitting && rdrObj->BaseData() &&// Radar On?
			!rdrObj->BaseData()->OnGround() && // In the Air? 
			(!rdrObj->BaseData()->IsSim() ||   // Campaign Entity
			(!rdrObj->BaseData()->IsExploding() && // Live none weapon sim thing
			!rdrObj->BaseData()->IsWeapon())) && 
			LookingAtObject (rdrObj))
		{
			rdrData->painted = TRUE;
			rdrData->rdrDetect = rdrData->rdrDetect >> 1;
			
			if (ObjectDetected (rdrObj))
			{
				if (!InResCell (rdrObj, i, rngCell, angCell,velCell))
				{
					i ++;
					ShiAssert (i<MAX_OBJECTS);
					rdrData->rdrDetect = rdrData->rdrDetect | 0x0010;
					// 2000-11-17 ADDED BY S.G. ONLY WHEN WE HAVE 'NoTrack' DO WE SET IT TO 'Detection'. OTHERWISE WE DON'T TOUCH IT (IT MIGHT ALREADY BE 'SensorTrack')
					if (rdrData->sensorState[Radar] == NoTrack)
						// END OF ADDED SECTION
						rdrData->sensorState[Radar] = Detection;
					rdrData->sensorLoopCount[Radar] = SimLibElapsedTime;
					rdrData->extrapolateStart = 0;  // MD -- 20040121: reset for no extrapolation
				}
			}
		}
		else
		{
			if (!isEmitting)
				rdrData->sensorLoopCount[Radar] = 0;
			rdrData->painted = FALSE;
			//Cobra 11/21/04 Wipe history if IFF not functioning
			if (!((AircraftClass*)platform)->iffEnabled)
				rdrData->interrogated = FALSE;
		}
		
		
		/*-------------------------------------------------*/
		/* if the target dies or goes out of the radar fov */
		/* shift in one 0 per frame to "age" the track.    */
		/*-------------------------------------------------*/
		testTime = SimLibElapsedTime - patternTime;
		if (rdrData->sensorLoopCount[Radar] < testTime)
		{
			/*-------------*/
			/* shift right */
			/*-------------*/
			rdrData->rdrDetect = rdrData->rdrDetect >> 1;
			
			/*----------------------------------*/
			/* slip, shift in a 0 for no detect */
			/*----------------------------------*/
			rdrData->rdrDetect = rdrData->rdrDetect & 0x000f;
			
			if (mode == TWS )
			{
				if (rdrData->extrapolateStart == 0)  // start the counter
					rdrData->extrapolateStart = SimLibElapsedTime;
				if ((SimLibElapsedTime - rdrData->extrapolateStart) < TwsExtrapolateTime)
				{
					ExtrapolateHistory (rdrObj);
				}
				else
				{
					if (rdrObj == lockedTarget)
					{
						reacqEl = lockedTargetData->el;
						SetSensorTarget(NULL);
					}
					ClearHistory (rdrObj);
					rdrData->extrapolateStart = 0;  // MD -- 20040121: reset for no extrapolation
					// MD -- 20040117: and remove from the TWS track directory after extrapolation expires
					if (TWSTrackDirectory)
						TWSTrackDirectory = TWSTrackDirectory->Remove(rdrObj);
				}
				//MI test
				// MD -- 20040117: looks like this shouldn't have been left in...don't wipe extrapolation data!
				//AddToHistory(rdrObj, None);
			}
			else // MD -- 20040118: don't do this for TWS tracks as well!
			if (!rdrData->TWSTrackFileOpen)
				if (rdrData->rdrSy[0] == Solid || rdrData->rdrSy[0] == None) 
					AddToHistory(rdrObj, None);
				else
					SlipHistory(rdrObj);
			
			/*----------------------*/
			/* reset search counter */
			/*----------------------*/
			rdrData->sensorLoopCount[Radar] = SimLibElapsedTime;
		}
		
		rdrObj = rdrObj->next;
	}
	
	if (!lockedFound && lockedTarget)
		lockedTargetData->rdrDetect = lockedTargetData->rdrDetect >> 1;
	
	// Update output paramters
	// NOTE:  These should be set to the _overall_ active serach volume,
	// instead of sweeping with the beam to reduce the amount of dirty data we
	// generate.
	platform->SetRdrAz(beamWidth);
	platform->SetRdrEl(beamWidth);
	
	// WARNING:  THIS IS WRONG.
	// seekerAzCenter is NOT body rotated (for pitch and roll)
	// while beamAz and beamEl might be.
	// In any case, SetRdrAzCenter and SetRdrElCenter SHOULD BE
	// body rolled so the RWR can properly interpret them...
	platform->SetRdrAzCenter(beamAz + seekerAzCenter);
	platform->SetRdrElCenter(beamEl + seekerElCenter);
	
	
	// If we changed locks, tell our previous target he's off the hook
	if (lastLocked != lockedTarget)
	{
		if (lastLocked)
		{
			SendTrackMsg (lastLocked, Track_Unlock);
		}
		sendThisFrame = TRUE;
	} else {
		// See if it is time to send a "painted" list update (AA mode only)
		sendThisFrame = (SimLibElapsedTime - lastTargetLockSend > TrackUpdateTime);
	}
	
	// 2002-02-09 MODIFIED BY S.G. Since radarMode is sent as well and is passed to the AI, will let him make the decision to deal with us or not...
	// Tell our current target he's locked
	/*	if (sendThisFrame && lockedTarget && (mode != TWS || IsSet(STTingTarget)))
	{
	SendTrackMsg (lockedTarget, Track_Lock);
	lastTargetLockSend = SimLibElapsedTime;
	}
	else if (lockedTarget && mode == TWS && lockedTarget->localData->lockmsgsend == Track_Lock)
	SendTrackMsg (lockedTarget, Track_Unlock);
	*/
	if (sendThisFrame && lockedTarget)
	{
		int radarMode;
		
		if (IsSet(STTingTarget)) // Prioritize STT over other modes
			radarMode = DigiSTT;
		else if (mode == SAM)
			radarMode = DigiSAM;//me123 when pinged by a SAM mode you realize someone is targeting/interested in you. 2002-02-19 MODIFIED BY S.G. Uses the new DigiSAM mode so that if I lock another player, his RWR doesn't go wild.
		else if (mode == TWS)
			radarMode = DigiTWS;
		else
			radarMode = DigiRWS;
		
#ifdef DEBUG
		MonoPrint("Player sending a type %d Track_Lock\n", radarMode);
#endif
		SendTrackMsg (lockedTarget, Track_Lock, radarMode);
		lastTargetLockSend = SimLibElapsedTime;
	}
	// END OF MODFIIED SECTION
	
	// Do NCTR Stuff
	if (lockedTarget)
	{
		// Saw this frame, so update our best guess
		if (radarDatFile && lockedTargetData->painted && lockedTargetData->range < radarDatFile->MaxNctrRange &&
			lockedTargetData->ataFrom < 45 * DTR)
		{
			// Make a guess based on range
			rVal = ((float)rand() / RAND_MAX);
			delta = radarDatFile->NctrDelta * lockedTargetData->range / radarDatFile->MaxNctrRange * rVal;
			
			// Add in the truth, with a range factor
			rVal = 1.0F - lockedTargetData->range / radarDatFile->MaxNctrRange;
			if ( TeamInfo[platform->GetTeam()]->TStance(lockedTarget->BaseData()->GetTeam()) == War )
				rVal *= -1.0F;
			rVal += delta;
			
			// integrate NCTR Percentage
			nctrData = 0.3F * rVal + 0.7F * nctrData;
		}
	}
	else
	{
		nctrData = 0.0F;
	}
	return lockedTarget;
}

void RadarDopplerClass::GetCursorPosition (float* xPos, float* yPos)
{
float range, az;
mlTrig trig;

   // Correct for azimuth skew towards bottom of the hud
   range = (cursorY + 1.0F) * displayRange * 0.5F;
   az = cursorX * MAX_ANT_EL;
   mlSinCos (&trig, az);

   *xPos = range * trig.cos;
   *yPos = range * trig.sin;
}
//MI added function
void RadarDopplerClass::DrawRCRCount(void)
{
}

// MD -- 20040228: access function for GM SP ground stabilized pseudo waypoint
void RadarDopplerClass::SetGMSPWaypt(WayPointClass* pt)
{
	if (GMSPPseudoWaypt)
	{ 
		// MLR 5/10/2004 - CTD/HEAP issues, the FCC was using this object after it had
		// been freed. 
		if(((AircraftClass *)platform)->curWaypoint == GMSPPseudoWaypt)
		{
			((AircraftClass *)platform)->curWaypoint = NULL;
		}
		delete GMSPPseudoWaypt;
	}
	if (pt)
	{
		GMSPPseudoWaypt = pt;
		// MD -- 20040515: following up on Mike's catch for the HEAP related CTD:
		// if you do set a new valid Pseudo point, make sure the FCC knows about it.
		((AircraftClass *)platform)->curWaypoint = GMSPPseudoWaypt;
	}
	else
	{
		GMSPPseudoWaypt = (WayPointClass *)NULL;
	}
}
