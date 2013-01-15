#include "stdhdr.h"
#include "entity.h"
#include "object.h"
#include "sensors.h"
#include "PilotInputs.h"
#include "hud.h"
#include "simdrive.h"
#include "simmover.h"
#include "radarDoppler.h"
#include "sms.h"
#include "fcc.h" //MI
#include "aircrft.h"	//MI
#include "Graphics\Include\gmComposit.h"

// MD -- 20031231: added for analog Antenna Elevation Controls
#include "simio.h"
#include "simmath.h"
extern SIMLIB_IO_CLASS IO;

extern bool g_bMLU;
// 2001-02-21 MODIFIED BY S.G. APG68_BAR_WIDTH IS 2.2
//static const float APG68_BAR_WIDTH		= (2.0f * DTR);		// Here and in Modes.cpp
static const float APG68_BAR_WIDTH		= (2.2f * DTR);		// Here and in Modes.cpp

static const float SAM_PATTERN_TIME		= 5.0F;
extern float g_fCursorSpeed;
extern bool  g_bAGRadarFixes;	//MI

// MD -- 20031221: fixing antenna elevation knob functions
extern bool g_bAntElevKnobFix;

void RadarDopplerClass::ChangeMode (int newMode)
{
	int maxIdx;
	int wasGround = FALSE;
	
	/*--------------------------------*/
	/* Save the old mode just in case */
	/*--------------------------------*/
	
	//prevMode = mode;
	
	//static float oldseekerElCenter =0.0f;
	
	if (prevMode == SAM || prevMode == TWS
		|| prevMode == RWS || prevMode == LRS|| prevMode == VS)
		oldseekerElCenter = seekerElCenter;//me123
	
	if (prevMode == GM || prevMode == GMT || prevMode == SEA)
	{
		DropGMTrack();
		wasGround = TRUE;
		seekerElCenter = oldseekerElCenter;

		// MD -- 20040228: clear and GM SP related pseudo waypoint and revert to STPT nav
		if (IsSet(SP_STAB))
			ClearFlagBit(SP_STAB);
		if (GMSPWaypt())
		{
			// RV - Biker - Should fix CTD 
			// if (SimDriver.GetPlayerAircraft())
			//	SimDriver.GetPlayerAircraft()->FCC->SetStptMode(FireControlComputer::FCCWaypoint);
			//	SimDriver.GetPlayerAircraft()->FCC->waypointStepCmd = 127;
			AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
			if (playerAC) {
				playerAC->FCC->SetStptMode(FireControlComputer::FCCWaypoint);
				playerAC->FCC->waypointStepCmd = 127;
			}
			SetGMSPWaypt(NULL);
		}
	}
	
	// MD -- 20040118: clean up on mode change from TWS
	// sfr: removes JB hack. Seems to cure memory corruption somehow...
#define NEW_TWS_CHECK 1
#if NEW_TWS_CHECK
	if (newMode != TWS && TWSTrackDirectory){
		TWSTrackDirectory = TWSTrackDirectory->Purge();
	}
#else
	if (newMode != TWS && !F4IsBadReadPtr(TWSTrackDirectory, sizeof(RadarDopplerClass::TWSTrackList))){
		TWSTrackDirectory = TWSTrackDirectory->Purge();
	}
#endif

	// Drop SAM target on mode switch (except when bugging)
	if (mode == SAM && newMode != STT){
		//ClearSensorTarget(); me123 status ok no keep the target on mode switch
	}
	
	isOn = TRUE;
	if (TheHud){
		TheHud->HudData.Clear(HudDataType::RadarSlew);
		TheHud->HudData.Clear(HudDataType::RadarBoresight);
		TheHud->HudData.Clear(HudDataType::RadarVertical);
	}
	
	/*-------------------------------------*/
	/* Set the parameters for the new mode */
	/*-------------------------------------*/
	
   	switch (newMode)
	{
	case RWS:   // Range while search
	case LRS:
		prevMode = mode = (RadarMode)newMode;
		fovStepCmd = 0;	//MI
		SetFlagBit(SpaceStabalized);
		beamWidth = radarData->BeamHalfAngle;
		displayRange = 20.0F;
		curRangeIdx = airRangeIdx;
		displayRange = rangeScales[curRangeIdx];
		curAzIdx = rwsAzIdx;
		azScan = rwsAzs[curAzIdx];
		curBarIdx = rwsBarIdx;
		bars = rwsBars[curBarIdx];
		barWidth = APG68_BAR_WIDTH;
		ClearFlagBit(VerticalScan);
		SetFlagBit(HorizontalScan);
		scanDir  = ScanFwd;
		
		if (!g_bAntElevKnobFix)
			//me123 set the old el center
			seekerElCenter = min ( max (oldseekerElCenter, -MAX_ANT_EL + elScan), MAX_ANT_EL - elScan);
		else
			seekerElCenter = AntElevKnob();

		SetEmitting (TRUE);
		if (wasGround)
		{
            cursorX = cursorY = 0.0F;
		}
		
		// Go to sam if there is a lock
		//MI added check for RWS mode. If this isn't here, we never make it out of RWS/SAM
		//mode. This makes sure that if we switch to RWS, we go to SAM, which is right I think.
		if (lockedTarget  && (mode == RWS || mode == LRS))
		{
            ChangeMode (SAM);
		}
		break;
		
	case TWS:  // Track while scan
		prevMode = mode = TWS;
		fovStepCmd = 0;
		
		SetFlagBit(SpaceStabalized);
		curRangeIdx = airRangeIdx;
		displayRange = rangeScales[curRangeIdx];

		// MD -- 20040124: updated TWS mode gets its own specific scan patterns
		curAzIdx = twsAzIdx = lastTwsAzIdx;
		azScan = twsAzs[curAzIdx];
		displayAzScan = twsAzs[curAzIdx];

		curBarIdx = twsBarIdx = lastTwsBarIdx;
		bars = twsBars[curBarIdx];

		beamWidth = radarData->BeamHalfAngle;
		// 2001-02-21 MODIFIED BY S.G. IN TWS, APG68_BAR_WIDTH IS 3.2 (2.2 * 1.4545455)
		//			barWidth = APG68_BAR_WIDTH * 1.3f;
		barWidth = APG68_BAR_WIDTH * 1.4545455f;
		ClearFlagBit(VerticalScan);
		SetFlagBit(HorizontalScan);
		scanDir  = ScanFwd;

		if (!g_bAntElevKnobFix)
			//me123 set the old el center
			seekerElCenter = min ( max (oldseekerElCenter, -MAX_ANT_EL + elScan), MAX_ANT_EL - elScan);
		else
			if (!lockedTarget)
				seekerElCenter = AntElevKnob();

		SetEmitting (TRUE);
		if (wasGround)
		{
            cursorX = cursorY = 0.0F;
		}

		break;
		
	case VS:   // Velocity search
		mode = VS;
		displayRange = velScales[vsVelIdx];
		curAzIdx = vsAzIdx;
		curBarIdx = vsBarIdx;
		SetFlagBit(SpaceStabalized);
		beamWidth = radarData->BeamHalfAngle;
		azScan = rwsAzs[curAzIdx];
		curBarIdx = rwsBarIdx;
		bars = rwsBars[curBarIdx];
		barWidth = APG68_BAR_WIDTH;
		ClearFlagBit(VerticalScan);
		SetFlagBit(HorizontalScan);
		scanDir  = ScanFwd;

		if (!g_bAntElevKnobFix)
			//me123 set the old el center
			seekerElCenter = min ( max (oldseekerElCenter, -MAX_ANT_EL + elScan), MAX_ANT_EL - elScan);
		else
			seekerElCenter = AntElevKnob();

		SetEmitting (TRUE);
		if (wasGround)
		{
            cursorX = cursorY = 0.0F;
		}
		break;
		
	case ACM_30x20:  // Auto Aquisition - 30x20 FOV
		mode = ACM_30x20;
		seekerAzCenter = 0.0F * DTR;
		seekerElCenter = -5.0F * DTR;
		ClearFlagBit(SpaceStabalized);
		beamWidth = radarData->BeamHalfAngle;
		azScan = 15.0F * DTR - beamWidth;
		elScan = 10.0F * DTR - beamWidth;
		displayRange = 10.0F;
		bars = 4;
		barWidth = APG68_BAR_WIDTH;
		ClearFlagBit(VerticalScan);
		SetFlagBit(HorizontalScan);
		scanDir  = ScanFwd;
		// me123 status ok. We don't always want to stop emitting when we change to an acm mode from a non acm radar mode
		// we just wanna transfer to acm and get the symbolgy for that. 
		//MI but that's how it works in the Block 50 Viper, so let's change it back
		if (prevMode == ACM_30x20 || prevMode == ACM_BORE || prevMode == ACM_10x60 || prevMode == ACM_SLEW || lockedTarget)
            SetEmitting (TRUE);
		else
            SetEmitting (FALSE);
		
		prevMode = mode;
		break;
		
	case ACM_BORE: // Auto Aquisition - Boresight
		mode = ACM_BORE;
		//        ClearSensorTarget(); me123 status ok. don't ever drop the target just becourse we are enterign acm bore. it probaly not possible enyway to enter bore with a lock.
		ClearFlagBit(SpaceStabalized);
		beamWidth = radarData->BeamHalfAngle;
		azScan = 0.0F * DTR;
		displayRange = 10.0F;
		elScan = 0.0F * DTR;
		seekerAzCenter = 0.0F;
		seekerElCenter = -3.0F*DTR;//me123 from 0.00.0F;
		beamAz = 0.0F;
		beamEl = 0.0F;
		bars = 1;
		barWidth = APG68_BAR_WIDTH;
		ClearFlagBit(VerticalScan);
		SetFlagBit(HorizontalScan);
		scanDir  = ScanNone;
		if (TheHud)
		{
            TheHud->HudData.Set(HudDataType::RadarBoresight);
		}
		if (prevMode == ACM_30x20 || prevMode == ACM_BORE || prevMode == ACM_10x60 || prevMode == ACM_SLEW || lockedTarget)
            SetEmitting (TRUE);
		else
            SetEmitting (FALSE);
		
		prevMode = ACM_BORE;
		break;
		
	case ACM_10x60: // Auto Aquisition - Vertical Search
		mode = ACM_10x60;
		//        ClearSensorTarget(); me123 status ok. don't ever drop the target just becourse we are enterign acm
		seekerAzCenter = 0.0F * DTR;
		seekerElCenter = 23.0F * DTR;
		ClearFlagBit(SpaceStabalized);
		beamWidth = radarData->BeamHalfAngle;
		displayRange = 10.0F;
		elScan = 30.0F * DTR - beamWidth;
		azScan = 5.0F * DTR - beamWidth;
		bars = -4;  //JPG 28 Apr 04 - was -3  ????
		barWidth = APG68_BAR_WIDTH;
		SetFlagBit(VerticalScan);
		ClearFlagBit(HorizontalScan);
		scanDir  = ScanFwd;
		if (TheHud)
		{
            TheHud->HudData.Set(HudDataType::RadarVertical);
		}
		if (prevMode == ACM_30x20 || prevMode == ACM_BORE || prevMode == ACM_10x60 || prevMode == ACM_SLEW || lockedTarget)
            SetEmitting (TRUE);
		else
            SetEmitting (FALSE);
		
		prevMode = mode;
		break;
		
	case ACM_SLEW:  // Auto Aquisition - Slewable, 20x60 FOV
		mode = ACM_SLEW;
		//        ClearSensorTarget(); me123 status ok. don't ever drop the target just becourse we are enterign acm
		SetFlagBit(SpaceStabalized);
		beamWidth = radarData->BeamHalfAngle;
		displayRange = 10.0F;
		azScan = 30.0F * DTR - beamWidth;
		elScan = 10.0F * DTR - beamWidth;
		bars = 4;
		barWidth = APG68_BAR_WIDTH;
		ClearFlagBit(VerticalScan);
		SetFlagBit(HorizontalScan);
		scanDir  = ScanFwd;
		if (TheHud)
		{
            TheHud->HudData.Set(HudDataType::RadarSlew);
		}
		if (prevMode == ACM_30x20 || prevMode == ACM_BORE || prevMode == ACM_10x60 || prevMode == ACM_SLEW || lockedTarget)
            SetEmitting (TRUE);
		else
            SetEmitting (FALSE);
		if (wasGround)
		{
            cursorX = cursorY = 0.0F;
		}
		
		prevMode = mode;
		break;
		
	case STT:
		prevMode = mode = STT;
		ClearFlagBit(SpaceStabalized);
		beamWidth = radarData->BeamHalfAngle;
		azScan = 0.0F * DTR;
		bars = 1;
		barWidth = APG68_BAR_WIDTH;
		ClearFlagBit(VerticalScan);
		SetFlagBit(HorizontalScan);
		scanDir  = ScanNone;
		SetEmitting (TRUE);
		break;
		
	case SAM:
		//MI
		if(mode == RWS)
			prevMode = RWS;
		else if(mode == LRS)
			prevMode = LRS;
		mode = SAM;
		fovStepCmd = 0;	//MI
		subMode = SAM_AUTO_MODE;
		displayAzScan = rwsAzs[curAzIdx];
		if (lockedTarget)
		{
			//MI
            //prevMode = RWS;
            CalcSAMAzLimit();
		}
		SetEmitting (TRUE);
		break;
		
	case GM:
	case GMT:
	case SEA:
		fovStepCmd = 0;
		
		mode = (RadarMode)newMode;
		// New to GM radar, start in 40 mile scope
		if (prevMode != GM && prevMode != GMT && prevMode != SEA)
		{
			curRangeIdx = gmRangeIdx;
			displayRange = rangeScales[curRangeIdx];
			tdisplayRange = displayRange * NM_TO_FT;
			groundMapRange = tdisplayRange * 0.5F;
			flags &= SP;
			SetFlagBit(NORM);
			if(WasAutoAGRange && g_bRealisticAvionics && g_bAGRadarFixes)
				SetFlagBit(AutoAGRange);
			if(g_bRealisticAvionics && g_bAGRadarFixes)
			{
				if(mode == GM || mode == SEA)
					curAzIdx = gmAzIdx;
				else
					curAzIdx = gmtAzIdx;				 
				azScan = rwsAzs[curAzIdx];
			}
			else
				azScan = 60.0F * DTR;
			barWidth = APG68_BAR_WIDTH;
			ClearFlagBit(VerticalScan);
			SetFlagBit(HorizontalScan);
			scanDir  = ScanFwd;
			SetGMScan();
		}
		else
		{
			// Reuse current range, within limits of course
			if (flags & (DBS1 | DBS2) || mode == GMT || mode == SEA)
			{
				maxIdx = NUM_RANGES - 3;
			}
			else
			{
				maxIdx = NUM_RANGES - 2;
			}
			
			if (mode != GM)
			{
				ClearFlagBit(DBS1);
				ClearFlagBit(DBS2);
			}
			
			curRangeIdx += rangeChangeCmd;
			if (curRangeIdx > maxIdx)
				curRangeIdx = maxIdx;
			else if (curRangeIdx < 0)
				curRangeIdx = 0;
			
			gmRangeIdx = curRangeIdx;
		}
		//MI make sure we're at the correct range
		if(g_bRealisticAvionics && g_bAGRadarFixes)
		{
			displayRange = rangeScales[curRangeIdx];
  	         tdisplayRange = displayRange * NM_TO_FT;
			 SetGMScan();
			 //Set our gain
			 InitGain = TRUE;
		}
		// GMT starts in snowplow
		//      if (mode == GMT)
		//         flags |= SP;//me123 no don't start in SP
		SetEmitting (TRUE);
		prevMode = mode;
		break;
	case STBY:
		mode = STBY;
		SetEmitting(FALSE);
		break;
		//MI		
	case AGR:
		mode = AGR;
		break;
	case OFF:
		mode = OFF;
		SetPower( FALSE );
		break;
	}
	
	
	
	if (mode == VS)
		tdisplayRange = 80.0F * NM_TO_FT;
	else
		tdisplayRange = displayRange * NM_TO_FT;
	
	SetScan();
	SetFlagBit(HomingBeam);
	
	//MI
	FireControlComputer *FCC = NULL;
	if(SimDriver.GetPlayerAircraft())
		FCC = SimDriver.GetPlayerAircraft()->FCC;
	if(FCC)
	{
		if(FCC->GetMasterMode() == FireControlComputer::ClearOveride)
			return;
		
		if(FCC->IsAAMasterMode())
			LastAAMode = mode;
		else if(FCC->IsNavMasterMode())
			LastNAVMode = mode;
		else if(FCC->IsAGMasterMode())
		{
			if(mode != AGR)
				LastAGMode = mode;
		}
		else if(FCC->GetMasterMode() == FireControlComputer::MissileOverride)
			Missovrradarmode = mode;
		else if(FCC->GetMasterMode() == FireControlComputer::Dogfight)
		{
			if(mode == ACM_30x20 || mode == ACM_10x60 || mode == ACM_SLEW || mode == ACM_BORE)
				Dogfovrradarmode = mode;
		}
	}
}

void RadarDopplerClass::UpdateState (int cursorXCmd, int cursorYCmd)
{
	int change = FALSE;
	int fromBump = FALSE;
	int maxIdx;
	float curCursorY = cursorY;
	
	if (IsSOI() && !IsAG() && mode != STBY)	//MI added STBY check. No cursors in STBY
	{
		if ((cursorXCmd != 0.0F || cursorYCmd != 0.0F) && !IsSet(STTingTarget))  // don't move cursor when you are in STT
		{
			if ((IO.AnalogIsUsed(AXIS_CURSOR_X) == true) && (IO.AnalogIsUsed(AXIS_CURSOR_Y) == true))
			{
				cursorX += (cursorXCmd / 10000.0F) * g_fCursorSpeed * (6.5F * CursorRate) * SimLibMajorFrameTime;
				cursorY += (cursorYCmd / 10000.0F) * g_fCursorSpeed * (6.5F * CursorRate) * SimLibMajorFrameTime;
			}
			else
			{
				cursorX += cursorXCmd * g_fCursorSpeed * curCursorRate * SimLibMajorFrameTime;
				cursorY += cursorYCmd * g_fCursorSpeed * curCursorRate * SimLibMajorFrameTime;
				static float test = 0.0f;
				static float testa = 0.0f;
				curCursorRate = min (curCursorRate + CursorRate * SimLibMajorFrameTime * (4.0F+test), (6.5F+testa) * CursorRate);
			}				
			if ((mode == TWS) && lockedTarget) // MD -- 20040125: in TWS constrain the cursors to the radar FOV when bugging
			{
				float tmpVal = TargetAz(platform, lockedTarget);
				cursorX = min ( max (cursorX, (tmpVal - (2 * azScan))), (tmpVal + (2 * azScan)));
			}
			cursorX = min ( max (cursorX, -1.0F), 1.0F);
			cursorY = min ( max (cursorY, -1.0F), 1.0F);
			flags |= WasMoving;
		}
		else
		{
			curCursorRate = CursorRate;
			flags &= ~WasMoving;
		}
		
	}
	
	if (modeDesiredCmd >= 0)
	{
		//      ClearSensorTarget();
		ChangeMode (modeDesiredCmd);
		modeDesiredCmd = -1;
	}
	
	switch (mode)
	{
	case RWS:
	case LRS:
		// Range change from cursor bump
		if (cursorY > 0.9F && curRangeIdx < 4)
		{
			rangeChangeCmd = 1;
			cursorY = 0.0F;
			//fromBump = TRUE;
		}
		else if (cursorY < -0.9F && curRangeIdx > 0)
		{
			rangeChangeCmd = -1;
			cursorY = 0.0F;
			//fromBump = TRUE;
		}
		//MI
		if((cursorX == 1.0F && cursorXCmd > 0) || (cursorX == -1.0F && cursorXCmd < 0))
			StepAzimuth(cursorX, cursorXCmd);
		if (azScan + beamWidth != MAX_ANT_EL)
			seekerAzCenter = cursorX * MAX_ANT_EL;
		else
			seekerAzCenter = 0.0F;
		
		seekerAzCenter = max ( min (seekerAzCenter, MAX_ANT_EL - azScan), -MAX_ANT_EL + azScan);
		
		if (!g_bAntElevKnobFix) {
			seekerElCenter = min ( max (seekerElCenter + elSlewCmd * EL_CHANGE_RATE,
				-MAX_ANT_EL), MAX_ANT_EL);
			
			if (centerCmd)
			{
				seekerElCenter = 0.0F;
				centerCmd = FALSE;
			}
		}
		else
			seekerElCenter = AntElevKnob();
		
		if (scanWidthCmd)
		{
			curAzIdx = (curAzIdx + scanWidthCmd) % NUM_RWS_AZS;
			azScan = rwsAzs[curAzIdx] - beamWidth * 0.5F;
			change = TRUE;
            rwsAzIdx = curAzIdx;
		}
		
		if (scanHeightCmd)
		{
			curBarIdx = (curBarIdx + scanHeightCmd) % NUM_RWS_BARS;
			bars = rwsBars[curBarIdx];
			change = TRUE;
            rwsBarIdx = curBarIdx;
		}
		break;
	case TWS:
		// Slewing Scan
		// Make sure that we only do this when were not locking or trying to lock
		if (!lockedTarget|| g_bMLU && !IsSet(STTingTarget))
		{
            seekerAzCenter = cursorX * MAX_ANT_EL;
            seekerAzCenter = max ( min (seekerAzCenter, MAX_ANT_EL - azScan), -MAX_ANT_EL + azScan);
			if (!g_bAntElevKnobFix) {
				seekerElCenter = min ( max (seekerElCenter + elSlewCmd * EL_CHANGE_RATE, -MAX_ANT_EL), MAX_ANT_EL);
				
				if (centerCmd)
				{
					seekerElCenter = 0.0F;
					centerCmd = FALSE;
				}
			}
			else
				// MD --20031223: always center on knob in absence of bugged target
				seekerElCenter = AntElevKnob();  
		}
		else if (lockedTarget && !IsSet(STTingTarget))
		{
			// MD -- 20040117: once a priority bugged target is established, the radar elevation centers 
			// more or less on that target until the bug is dropped or moved.
			// The AZ scan centers on the cursor but will not move past the point that the bugged target falls
			// outside the scan area.
			float tmpVal = TargetAz(platform, lockedTarget);
            seekerAzCenter = cursorX * MAX_ANT_EL;
			seekerAzCenter = max ( min (seekerAzCenter, (tmpVal + azScan)), (tmpVal - azScan));
			seekerAzCenter = min ( max (seekerAzCenter ,-MAX_ANT_EL + azScan), MAX_ANT_EL - azScan);
			
			tmpVal = TargetEl(platform, lockedTarget);
			seekerElCenter = min ( max (tmpVal, -MAX_ANT_EL + elScan), MAX_ANT_EL - elScan);
		}
		
		// Range change from cursor bump
		if (cursorY > 0.9F && curRangeIdx < 4)
		{
			rangeChangeCmd = 1;
			cursorY = 0.0F;
			//fromBump = TRUE;
		}
		else if (cursorY < -0.9F && curRangeIdx > 0)
		{
			rangeChangeCmd = -1;
			cursorY = 0.0F;
			//fromBump = TRUE;
		}
		//MI
		// MD -- 20040123: don't bump the scan pattern when there's a bugged track file
		if (!lockedTarget && ((cursorX == 1.0F && cursorXCmd > 0) || (cursorX == -1.0F && cursorXCmd < 0)))
			StepAzimuth(cursorX, cursorXCmd);
		
		//me123 az/bar change same as rws -- MD, umm no, that's not correct...there are three fixed
		// scan patterns for TWS so any bump in Az or Bars will bump the other dimension also.  Thus
		// these change commands now step both twsAzIdx and twsBarIdx at the same time!
		
		// MD -- 20040123: don't bump the scan pattern when there's a bugged track file
		if (!lockedTarget && scanWidthCmd)
		{ 
			curAzIdx = (curAzIdx + scanWidthCmd) % NUM_TWS_AZS;
			azScan = twsAzs[curAzIdx] - beamWidth * 0.5F;
			curBarIdx = curAzIdx;  // keep patterns in lock step as you change
			bars = twsBars[curBarIdx];
			change = TRUE;
			twsAzIdx = curAzIdx;
			twsBarIdx = curBarIdx;
		}
		
		// MD -- 20040123: don't bump the scan pattern when there's a bugged track file
		if (!lockedTarget && scanHeightCmd)
		{
			curBarIdx = (curBarIdx + scanHeightCmd) % NUM_TWS_BARS; //me123 max 3 bars in tws; MD -- but don't hard code it here
			bars = twsBars[curBarIdx];
			curAzIdx = curBarIdx;  // keep patterns in lock step as you change
			azScan = twsAzs[curAzIdx] - beamWidth * 0.5F;
			change = TRUE;
			twsBarIdx = curBarIdx;
			twsAzIdx = curAzIdx;
		}
		
		break;
	case SAM:
		if (!IsSet(STTingTarget))
		{
			if (subMode != SAM_AUTO_MODE)
			{
				if (cursorY > 0.9F && curRangeIdx < 4)
				{
					rangeChangeCmd = 1;
					cursorY = 0.0F;
				}
				else if (cursorY < -0.9F && curRangeIdx > 0)
				{
					rangeChangeCmd = -1;
					cursorY = 0.0F;
				}
				//MI
				if((cursorX == 1.0F && cursorXCmd > 0) || (cursorX == -1.0F && cursorXCmd < 0))
					StepAzimuth(cursorX, cursorXCmd);
				
				if (azScan != MAX_ANT_EL)
					seekerAzCenter = cursorX * MAX_ANT_EL;
				else
					seekerAzCenter = 0.0F;
				seekerAzCenter = max ( min (seekerAzCenter, MAX_ANT_EL - azScan), -MAX_ANT_EL + azScan);
				if (!g_bAntElevKnobFix) {
					
					seekerElCenter = min ( max (seekerElCenter + elSlewCmd * EL_CHANGE_RATE,
						-MAX_ANT_EL), MAX_ANT_EL);
					
					if (centerCmd)
					{
						seekerElCenter = 0.0F;
						centerCmd = FALSE;
					}
				}
				
			}
			
			if (g_bAntElevKnobFix)
				// MD -- 20032121: fixing the antenna elevation controls
				seekerElCenter = AntElevKnob(); // always center on the knob in SAM
			
			/*--------------*/
			/* Manual SAM ? */
			/*--------------*/
			if (((cursorXCmd != 0) || (cursorYCmd != 0))&& !targetUnderCursor)
            {
				subMode = SAM_MANUAL_MODE;
            }
			else if ((cursorXCmd == 0) && (cursorYCmd == 0) && lockedTarget && (targetUnderCursor == lockedTarget->BaseData()->Id()) )
			{
				subMode = SAM_AUTO_MODE;
			}
			
			if (scanWidthCmd)
			{
				curAzIdx = (curAzIdx + scanWidthCmd) % NUM_RWS_AZS;
				if (curAzIdx == 0)
					curAzIdx ++;
				azScan = rwsAzs[curAzIdx] - beamWidth * 0.5F;
				change = TRUE;
				rwsAzIdx = curAzIdx;
			}
			
			if (scanHeightCmd)
			{
				curBarIdx = (curBarIdx + scanHeightCmd) % NUM_RWS_BARS;
				bars = rwsBars[curBarIdx];
				change = TRUE;
				rwsBarIdx = curBarIdx;
			}
		}
		break;
	case ACM_SLEW:
		if (!lockedTarget)
		{
			seekerAzCenter = cursorX * MAX_ANT_EL;
			seekerElCenter = cursorY * MAX_ANT_EL;
		}
		
		seekerAzCenter = max ( min (seekerAzCenter, MAX_ANT_EL - azScan), -MAX_ANT_EL + azScan);
		if (TheHud)
		{
            TheHud->HudData.radarAz = seekerAzCenter;
            TheHud->HudData.radarEl = seekerElCenter;
		}
	case ACM_BORE:
	case ACM_30x20:
	case ACM_10x60:
		if (scanWidthCmd)
		{
            scanWidthCmd = FALSE;
			
            if (mode == ACM_SLEW)
				ChangeMode (ACM_BORE);
            else if (mode == ACM_BORE)
				ChangeMode (ACM_30x20);
            else if (mode == ACM_30x20)
				ChangeMode (ACM_10x60);
            else
				ChangeMode (ACM_SLEW);
		}
		
		if (!lockedTarget)
		{
            rangeChangeCmd = 0;
		}
		break;
	case VS:
		if (cursorY > 0.9F && vsVelIdx < 1)			
		{
			rangeChangeCmd = 1;
			cursorY = 0.0F;
		}
		else if (cursorY < -0.9F && vsVelIdx > 0)
		{
			rangeChangeCmd = -1;
			cursorY = 0.0F;
		}
		
		if (rangeChangeCmd)
		{
			vsVelIdx ++;
			if (vsVelIdx >= NUM_VELS)
				vsVelIdx = 0;
			else if (vsVelIdx == -1)
				vsVelIdx = NUM_VELS - 1;
			
			displayRange = velScales[vsVelIdx];
			tdisplayRange = 80.0F * NM_TO_FT;
		}
		
		//MI
		if((cursorX == 1.0F && cursorXCmd > 0) || (cursorX == -1.0F && cursorXCmd < 0))
			StepAzimuth(cursorX, cursorXCmd);
		
		// Scan Center
		if (azScan != MAX_ANT_EL)
			seekerAzCenter = cursorX * MAX_ANT_EL;
		else
			seekerAzCenter = 0.0F;
		
		seekerAzCenter = max ( min (seekerAzCenter, MAX_ANT_EL - azScan), -MAX_ANT_EL + azScan);
		
		if (!g_bAntElevKnobFix) {
			seekerElCenter = min ( max (seekerElCenter + elSlewCmd * EL_CHANGE_RATE,
				-MAX_ANT_EL), MAX_ANT_EL);
			
			if (centerCmd)
			{
				seekerElCenter = 0.0F;
				centerCmd = FALSE;
			}
		}
		else
			seekerElCenter = AntElevKnob();
		
		// Scan Volume
		if (scanWidthCmd)
		{
			curAzIdx = (curAzIdx + scanWidthCmd) % NUM_RWS_AZS;
			azScan = rwsAzs[curAzIdx] - beamWidth * 0.5F;
			change = TRUE;
            vsAzIdx = curAzIdx;
		}
		
		if (scanHeightCmd)
		{
			curBarIdx = (curBarIdx + scanHeightCmd) % NUM_RWS_BARS;
			bars = rwsBars[curBarIdx];
			change = TRUE;
            vsBarIdx = curBarIdx;
		}
		break;
	case GM:
	case GMT:
	case SEA:
		// No 80 mile range in DBS1, DBS2, GMT or SEA
		if (rangeChangeCmd)
		{
            if (flags & (DBS1 | DBS2) || mode == GMT || mode == SEA)
            {
				maxIdx = NUM_RANGES - 2;
            }
            else
            {
				maxIdx = NUM_RANGES - 1;
            }
			
            curRangeIdx += rangeChangeCmd;
			if (curRangeIdx >= maxIdx)
				curRangeIdx = maxIdx - 1;
			else if (curRangeIdx < 0)
				curRangeIdx = 0;
			displayRange = rangeScales[curRangeIdx];
  	         tdisplayRange = displayRange * NM_TO_FT;
			 SetGMScan();
			 rangeChangeCmd = FALSE;
			 gmRangeIdx = curRangeIdx;
		}
		if(scanWidthCmd && g_bRealisticAvionics && g_bAGRadarFixes)
		{
			curAzIdx = (curAzIdx + scanWidthCmd) % NUM_RWS_AZS;
			//azScan = rwsAzs[curAzIdx] - beamWidth * 0.5F;			
			if(mode == GM || mode == SEA)
				gmAzIdx = curAzIdx;
			else if(mode == GMT)
				gmtAzIdx = curAzIdx;
			
			azScan = displayAzScan = rwsAzs[curAzIdx];
			//azScan = rwsAzs[rwsAzIdx];
			
			SetGMScan();
			scanWidthCmd = FALSE;
		}
		if ((cursorXCmd != 0) || (cursorYCmd != 0))
		{
			SetAimPoint ((float)cursorXCmd, (float)cursorYCmd);
            SetGMScan();
            flags |= WasMoving;
		}
		else
		{
			if (flags & WasMoving)
			{
				// MD -- 20040229: switched ordering here - need WasMoving set to FALSE to bump!
				flags &= ~WasMoving;
			}
            rangeChangeCmd = CheckGMBump();
		}
		break;
	}
	
	// WHY is this here?  Isn't this already delt with in the switch above???
	if (rangeChangeCmd)
	{
		//MI remeber our MissileOverride range
		FireControlComputer *Fcc = NULL;
		// COBRA - RED - CTD Fix - If player is dead
		if(SimDriver.GetPlayerAircraft()) Fcc = SimDriver.GetPlayerAircraft()->FCC;

		if(Fcc)
		{
			if(Fcc->GetMasterMode() == FireControlComputer::MissileOverride)
			{
				if(mode == VS)
				{
					displayRange = velScales[vsVelIdx];
					tdisplayRange = 80.0F * NM_TO_FT;
					
					rangeChangeCmd = FALSE;
					return;
				}
				else
				{
					curRangeIdx += rangeChangeCmd;
					if (curRangeIdx >= NUM_RANGES)
						curRangeIdx = NUM_RANGES - 1;
					else if (curRangeIdx < 0)
						curRangeIdx = 0;
				}
				
				displayRange = rangeScales[curRangeIdx];
				
				float tmpRange = (curCursorY + 1.0F) * tdisplayRange;
				
				tdisplayRange = displayRange * NM_TO_FT;
				
				if (mode != VS && fromBump && tmpRange)
				{ 
					cursorY = tmpRange / tdisplayRange - 1.0F;
				} 
				rangeChangeCmd = FALSE;
				return;
			}
			
		}
		float tmpRange = (curCursorY + 1.0F) * tdisplayRange;
		
		if (mode != VS)
		{
			curRangeIdx += rangeChangeCmd;
			if (curRangeIdx >= NUM_RANGES)
			{
				curRangeIdx = NUM_RANGES - 1;
				//MI
				if(g_bRealisticAvionics && g_bAGRadarFixes)
					rangeChangeCmd = FALSE;
			}
			else if (curRangeIdx < 0)
			{
				curRangeIdx = 0;
				//MI
				if(g_bRealisticAvionics && g_bAGRadarFixes)
					rangeChangeCmd = FALSE;
			}
			
			displayRange = rangeScales[curRangeIdx];
			switch (mode)
			{
            case GM:
            case GMT:
            case SEA:
				gmRangeIdx = curRangeIdx;
				break;
				
            default:
				airRangeIdx = curRangeIdx;
				break;
			}
		}
		tdisplayRange = displayRange * NM_TO_FT;
		
		if (mode == GM || mode == GMT || mode == SEA)
		{
			SetGMScan();
			// MD -- 20040303: commenting this out -- when range is bumped in AUTO mode
			// this was being called and it was doing something very bogus to the cursor
			// positioning -- no useful value it seems.
			// AdjustGMOffset (rangeChangeCmd);
		}
		else if (mode != VS && fromBump && tmpRange)
		{
			cursorY = tmpRange / tdisplayRange - 1.0F;
		}
		rangeChangeCmd = FALSE;
	}
	
	if (change)
	{
		SetScan();
		scanWidthCmd    = FALSE;
		scanHeightCmd   = FALSE;
		rangeChangeCmd = FALSE;
	}
	elSlewCmd    = FALSE;
}
void RadarDopplerClass::SetScan(void)
{
	if (bars > 0)
		elScan = (bars - 1) * barWidth/1.99f;
	tbarWidth = barWidth ;//me123* 2.0F;
	
	// In SAM we have a fixed pattern time, and change volume to maintain
	if (mode == SAM)
	{
		patternTime = FloatToInt32(SAM_PATTERN_TIME * SEC_TO_MSEC);
	}
	else
	{
		patternTime = FloatToInt32((2.2F * abs(bars) * (azScan + beamWidth + barWidth * 2.0F) /
			scanRate * SEC_TO_MSEC) * 1.05F + 1);
	}
	//   ClearFlagBit(STTingTarget);
	platform->SetRdrCycleTime ((float)patternTime / SEC_TO_MSEC);
	
	if (mode != SAM && mode != STT)
		displayAzScan = azScan;
}

void RadarDopplerClass::CalcSAMAzLimit (void)
{
float angleDelta;
float az, el;

	az = TargetAz (platform, lockedTarget->BaseData()->XPos(), lockedTarget->BaseData()->YPos());
	el = TargetEl (platform, lockedTarget->BaseData()->XPos(), lockedTarget->BaseData()->YPos(),
      lockedTarget->BaseData()->ZPos());

   angleDelta = (float)fabs (az - seekerAzCenter);
   angleDelta = max (angleDelta, (float)fabs(el - seekerElCenter));

   azScan = ((SAM_PATTERN_TIME - 1.0F)*scanRate - 2.2F*angleDelta - 3.0F * bars * barWidth) / (bars + 1);
   if (azScan < 0.0F)
      azScan = 0.0F;
}

void RadarDopplerClass::StepAAmode( void )
{
	//MI make sure we stay in dogfight modes when in dogfight override
	if(Overridemode == 2)
	{
		if(mode == ACM_30x20)
			modeDesiredCmd = ACM_SLEW;
		else if(mode == ACM_SLEW)
			modeDesiredCmd = ACM_BORE;
		else if(mode == ACM_BORE)
			modeDesiredCmd = ACM_10x60;
		else if(mode == ACM_10x60)
			modeDesiredCmd = ACM_30x20;

		return;
	}

    switch (mode) 
	{
    case RWS:
    case SAM:
	if (g_bRealisticAvionics)
	{
	    modeDesiredCmd = LRS;
		//MI
		if(mode == SAM && prevMode == LRS)
			modeDesiredCmd = VS;
	}
	else
	    modeDesiredCmd = VS;
	break;
    case LRS:
		modeDesiredCmd = VS;
	break;
	
    case VS:
		modeDesiredCmd = TWS;
	break;
	
    case TWS:
	// Marco Edit - don't cycle to ACM modes
	// modeDesiredCmd = ACM_30x20;
	if (g_bRealisticAvionics)
		modeDesiredCmd = RWS;
	else
		modeDesiredCmd = ACM_30x20;
	break;
	
    case ACM_30x20:
    case ACM_SLEW:
    case ACM_BORE:
    case ACM_10x60:
      if (lockedTarget)
      {
         prevMode = RWS;
         modeDesiredCmd = SAM;
         lastSAMAzScan = rwsAzs[curAzIdx] - beamWidth * 0.5F;
         lastSAMBars = rwsBars[curBarIdx];
         lastAzScan = lastSAMAzScan;
         lastBars = lastSAMBars;
      }
	  //MI OSB2 fix
	  if(mode == ACM_30x20)
		  modeDesiredCmd = ACM_SLEW;
	  else if(mode == ACM_SLEW)
		  modeDesiredCmd = ACM_BORE;
	  else if(mode == ACM_BORE)
		  modeDesiredCmd = ACM_10x60;
	  else if(mode == ACM_10x60)
		  modeDesiredCmd = RWS;
     break;

	  default:
		modeDesiredCmd = RWS;
		break;
	}
}

void RadarDopplerClass::DefaultAAMode (void)
{
	//MI
	FireControlComputer *FCC = NULL;
	if(g_bRealisticAvionics)
	{
		if(SimDriver.GetPlayerAircraft())
			FCC = SimDriver.GetPlayerAircraft()->FCC;
		if(FCC)
		{
			if(FCC->IsAAMasterMode())
				modeDesiredCmd = LastAAMode;
			else if(FCC->IsNavMasterMode())
				modeDesiredCmd = LastNAVMode;
			else if(FCC->IsAGMasterMode())
			{
				modeDesiredCmd = LastAGMode;
				if(LastAGModes == 2)	//SnowPlow
				{
					SetAGSnowPlow(TRUE);
				}
				else if(LastAGModes == 3)	//STPT
					SetAGSnowPlow(FALSE);
				else
					ShiWarning("Inconsistant state");
			}
			else if(FCC->GetMasterMode() == FireControlComputer::ClearOveride)
			{
				if(FCC->GetLastMasterMode() == FireControlComputer::AAGun)	//AA mode
					modeDesiredCmd = LastAAMode;
			}
			else if(FCC->GetMasterMode() == FireControlComputer::Dogfight)
				modeDesiredCmd = Dogfovrradarmode;
			else if(FCC->GetMasterMode() == FireControlComputer::MissileOverride)
				modeDesiredCmd = Missovrradarmode;
			else
				ShiWarning("How did you get here?");
		}
		return;
	}
	if (mode == GMT ||
       mode == SEA ||
       mode == GM)
   {
      modeDesiredCmd = RWS;
   }
	else {
		modeDesiredCmd = -1;
	}
}

void RadarDopplerClass::StepAGmode( void )
{
	switch (mode) {
	  case GM:
		modeDesiredCmd = GMT;
		break;

	  case GMT:
		modeDesiredCmd = SEA;
		break;

	  case SEA:
		modeDesiredCmd = GM;
		break;

	  default:
		modeDesiredCmd = GM;
		break;
	}
}

void RadarDopplerClass::SetMode (RadarMode cmd)
{
   // Default AA mode is RWS
   if (cmd == AA)
      cmd = RWS;

   if (cmd != mode)
      modeDesiredCmd = cmd;
}

void RadarDopplerClass::DefaultAGMode (void)
{
	//MI
	FireControlComputer *FCC = NULL;
	if(g_bRealisticAvionics)
	 {
		if(SimDriver.GetPlayerAircraft())
			FCC = SimDriver.GetPlayerAircraft()->FCC;
		if(FCC)
		{
			if(FCC->IsAAMasterMode())
				modeDesiredCmd = LastAAMode;
			else if(FCC->IsNavMasterMode())
				modeDesiredCmd = LastNAVMode;
			else if(FCC->IsAGMasterMode())
			{
				// ASSOCIATOR moved this check from RadarDopplerClass::Display in Bscope.cpp to here so that it only
				// defaults to AGR when first selected and than be changed manually to any other radar mode
				if(FCC->GetSubMode() == FireControlComputer::CCIP || FCC->GetSubMode() == FireControlComputer::DTOSS ||
					FCC->GetSubMode() == FireControlComputer::STRAF || 
					FCC->GetSubMode() == FireControlComputer::MAN ||   // Cobra - for JSOWa
					FCC->GetMasterMode() == FireControlComputer::AirGroundRocket/*FCC->GetSubMode() == FireControlComputer::RCKT*/ && mode != AGR )
				{
					mode = AGR;
					return;
				}
				modeDesiredCmd = LastAGMode;
				if(LastAGModes == 2)	//SnowPlow
				{
					SetAGSnowPlow(TRUE);
				}
				else if(LastAGModes == 3)	//STPT
					SetAGSnowPlow(FALSE);
				else
					ShiWarning("Inconsistant state");
			}
			else if(FCC->GetMasterMode() == FireControlComputer::ClearOveride)
			{
				if(FCC->GetLastMasterMode() == FireControlComputer::AAGun)	//AA mode
					modeDesiredCmd = LastAAMode;
			}
			else if(FCC->GetMasterMode() == FireControlComputer::Dogfight)
				modeDesiredCmd = Dogfovrradarmode;
			else if(FCC->GetMasterMode() == FireControlComputer::MissileOverride)
				modeDesiredCmd = Missovrradarmode;
			else
				ShiWarning("How did you get here?");
		}
		return;
	}

   if (mode != GMT &&
       mode != SEA &&
       mode != GM)
   {
      modeDesiredCmd = GMT;
   }
	else {
		modeDesiredCmd = -1;
	}
}

void RadarDopplerClass::NextTarget (void)
{
	//Step target starting with the closest one to us
	SimObjectType* rdrObj = platform->targetList;
	SimObjectType* NextFurther = NULL;
	
	
	if (mode != TWS)
	{
		float RangeToBeat; //next target needs to be further then this
		float MinRange = tdisplayRange;	//make sure we check the whole radar range
		
		if(lockedTarget)
		{
			//store our dist
			RangeToBeat = lockedTarget->localData->range;
			while(rdrObj)
			{
				//only what we've detected
				if(ObjectDetected(rdrObj))
				{
					//can't lock onto these
					if(!rdrObj->BaseData()->OnGround() && !rdrObj->BaseData()->IsMissile() && 
						!rdrObj->BaseData()->IsBomb() && !rdrObj->BaseData()->IsEject() &&
						rdrObj->localData->rdrDetect)
					{
						if(MinRange > rdrObj->localData->range)
						{ 
							if(rdrObj->localData->range > RangeToBeat)
							{
								NextFurther = rdrObj;
								MinRange = rdrObj->localData->range;
							}
						}
					}
				}
				if(!rdrObj->next)
					break;
				rdrObj = rdrObj->next;
			}
			if(NextFurther)
				SetDesiredTarget(NextFurther);
			else
				FindClosest(MinRange);
		}
		else
			FindClosest(MinRange);
	}
	else  // revised TWS mode TWS directory based targeting
	{
		if (IsSet(STTingTarget))
			return;  // no cycling if we are in STT

		if (lockedTarget && TWSTrackDirectory)
		{
			if (!IsSet(STTingTarget)) 
			{
				TWSTrackList* tmp = TWSTrackDirectory->OnList(lockedTarget);
				AddToHistory(lockedTarget, Track);  // demote current from bug to a track
				if (tmp)
					do
					{
						if (!tmp->Next())
							tmp = TWSTrackDirectory;
						else
							tmp = tmp->Next();
					} while ((tmp->TrackFile()->localData->extrapolateStart != 0)
						&& (tmp->TrackFile() != lockedTarget));

				ClearSensorTarget();  // ...and release it.
				// ...and if there's a suitable candidate bug it instead
				if (tmp && (tmp->TrackFile()->localData->extrapolateStart == 0))
				{
					SetDesiredTarget(tmp->TrackFile());
				}
			}
		}
		else if (TWSTrackDirectory)
			SetDesiredTarget(TWSTrackDirectory->TrackFile());
		else
			FindClosest(tdisplayRange);

		if (lockedTarget)
			AddToHistory(lockedTarget, Bug);  // promote new one to a bug
	}
}

void RadarDopplerClass::PrevTarget (void)
{
}

void RadarDopplerClass::SetSRMOverride( void )
{
   SetPower(TRUE);
   //MI
   if(mode == SAM)
	   noovrradarmode = prevMode;
   else
	   noovrradarmode = mode;//me123
   ChangeMode (Dogfovrradarmode); 
   Overridemode = 2;//me123 dogfight flag
}

void RadarDopplerClass::SetMRMOverride( void )
{
   SetPower(TRUE);
    //MI
   if(mode == SAM)
	   noovrradarmode = prevMode;
   else
	   noovrradarmode = mode;//me123
	if (!lockedTarget)
	{
		//MI changed so it remembers our last mode
		//airRangeIdx = 1;//me123
		//MI
		lastAirRangeIdx = airRangeIdx;
		airRangeIdx = MissOvrRangeIdx;
	}
	ChangeMode (Missovrradarmode);
	Overridemode = 1;//me123 missovride flag
}

void RadarDopplerClass::ClearOverride( void )
{

	if (Overridemode == 2) //me123 dogfight flag
	{
		if(mode == ACM_30x20 || mode == ACM_10x60 || mode == ACM_SLEW || mode == ACM_BORE)
			Dogfovrradarmode = mode;

		//restore our range
		airRangeIdx = lastAirRangeIdx;
	}

	if (Overridemode == 1)//me123 missovride flag
	{
		Missovrradarmode = mode;
		//restore our range
		airRangeIdx = lastAirRangeIdx;
		MissOvrRangeIdx = curRangeIdx;
	}

	Overridemode = 0;
	ChangeMode (noovrradarmode);//me123
}
void RadarDopplerClass::FindClosest(float MinRange)
{
	SimObjectType* Closest = NULL;
	SimObjectType* rdrObj = platform->targetList;

	while(rdrObj)
	{
		if(ObjectDetected(rdrObj))
		{
			//can't lock onto these
			if(!rdrObj->BaseData()->OnGround() && !rdrObj->BaseData()->IsMissile() && 
				!rdrObj->BaseData()->IsBomb() && !rdrObj->BaseData()->IsEject() &&
				rdrObj->localData->rdrDetect)
			{
				if(rdrObj->localData->range < MinRange)
				{
					Closest = rdrObj;
					MinRange = rdrObj->localData->range;
				}
			}
		}
		if(!rdrObj->next)
			break;
		rdrObj = rdrObj->next;
	}
	if(Closest)
	{
		SetDesiredTarget(Closest);
		if (mode == TWS)  // MD -- 20040122: bug the target right away for TWS
			AddToHistory(Closest, Bug);
	}
}
void RadarDopplerClass::StepAzimuth(float X, int Cmd)
{
	// MD -- 20040111: modified test for cmd value to be compatible with analog cursor support
	// MD -- 20040123: modified to take into account revised TWS scan pattern implementation
	if(X == 1.0F && Cmd > 0)
	{
		if (mode == TWS)
		{
			// only bump between 25 and 60 degree patterns
			if(curAzIdx == 2)
				curAzIdx = 1;
			else
				curAzIdx = 2;
			azScan =twsAzs[curAzIdx] - beamWidth * 0.5F;
			displayAzScan = twsAzs[curAzIdx];
			curBarIdx = twsBarIdx = curAzIdx;  // move bars in lock step
			bars = twsBars[curBarIdx];
		}
		else
		{
			//step our Azimuth, only step 30 and 60 degree
			//2 = 60°, 1 = 30°
			if(curAzIdx == 0)
				curAzIdx = 2;
			else
				curAzIdx = 0;
			azScan = rwsAzs[curAzIdx] - beamWidth * 0.5F;
			displayAzScan = rwsAzs[curAzIdx];
		}

		//bump the cursor back to the center, if no locked target
		//otherwise, just bump it back a bit.
		if(!lockedTarget)
			cursorX = 0.0F;
		else
			cursorX = 0.9F;
	}
	else if(X == -1.0F && Cmd < 0)
	{
		if (mode == TWS)
		{
			// only bump between 25 and 60 degree patterns
			if(curAzIdx == 2)
				curAzIdx = 1;
			else
				curAzIdx = 2;
			azScan =twsAzs[curAzIdx] - beamWidth * 0.5F;
			displayAzScan = twsAzs[curAzIdx];
			curBarIdx = twsBarIdx = curAzIdx;  // move bars in lock step
			bars = twsBars[curBarIdx];
		}
		else
		{
			//step our Azimuth, only step 30 and 60 degree
			//2 = 60°, 1 = 30°
			if(curAzIdx == 0)
				curAzIdx = 2;
			else
				curAzIdx = 0;
			azScan = rwsAzs[curAzIdx] - beamWidth * 0.5F;
			displayAzScan = rwsAzs[curAzIdx];
		}
		//bump the cursor back to the center, if no locked target
		//otherwise, just bump it back a bit.
		if(!lockedTarget)
			cursorX = 0.0F;
		else
			cursorX = -0.9F;
	}
}


// MD -- 20031221: adding function to replace inline macro.  This one will keep track of the angle
// commanded via the antenna elevation knob on the HOTAS throttle handle.  RWS, SAM and TWS modes
// (with no bugged target) should all center elevation on the position of the knob and the position
// of the knob should be changeable regardless of the radar mode.  This really needs to be done via
// an analog axis mapped to the HOTAS control but until that is possible, this will work.

void RadarDopplerClass::StepAAelvation( int cmd )
{
	if (IO.AnalogIsUsed(AXIS_ANT_ELEV))
		return;

	if (!g_bAntElevKnobFix) {
		if (cmd)
			elSlewCmd = cmd;
		else
			centerCmd = TRUE;
	}
	else
	{
		if (cmd == 0)
			antElevKnob = 0.0F;
		else
			antElevKnob = min ( max ( (antElevKnob + (cmd * EL_CHANGE_RATE)), -MAX_ANT_EL), MAX_ANT_EL);
	}
}

float RadarDopplerClass::AntElevKnob(void)
{
	// MD -- 20031223: make this a regular function because soon this will be integrated with analog
	// axis support for the HOTAS Antenna Knob

	// MD -- 20031231: and now for the analog support thanks to Retr0
	// Antenna elevation is stored in Radians.  Analog curves probably
	// want to be shaped to ensure relatively good control near the
	// center detent and less fine adjustment near the ends of travel.

	if (IO.AnalogIsUsed(AXIS_ANT_ELEV)) {
		antElevKnob = ((IO.GetAxisValue(AXIS_ANT_ELEV) / 10000.0F) * MAX_ANT_EL);
	}
	
	return antElevKnob;
}
