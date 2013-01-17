#include "stdhdr.h"
#include <float.h>
#include "hud.h"
#include "guns.h"
#include "aircrft.h"
#include "fcc.h"
#include "object.h"
#include "airframe.h"
#include "otwdrive.h"
#include "playerop.h"
#include "Graphics/Include/Render2d.h"
#include "Graphics/Include/Mono2d.h"
#include "simdrive.h"
#include "atcbrain.h"
#include "campbase.h"
#include "ptdata.h"
#include "simfeat.h"
#include "mfd.h"
#include "camp2sim.h"
#include "fack.h"
#include "cpmanager.h"
#include "objectiv.h"
#include "fsound.h"
#include "soundfx.h"
#include "classtbl.h"
#include "navsystem.h"
#include "flightData.h"
#include "Icp.h"

#include "RadarDoppler.h"  // MD -- 20040219: added for GM SP pseudo waypoint tracking

#include "harmpod.h" // RV - I-Hawk

//MI for RALT and MSLFloor stuff
extern bool g_bRealisticAvionics;
extern bool g_bFallingHeadingTape;
#include "flightData.h"
extern bool g_bINS;

//HUD Fixes on/off switch. Smeghead, 16-Oct-2003.
extern bool g_bHUDFix;

void HudClass::DrawAirspeed (void)
{
	char tmpStr[12]={0};
	float rightEdge=0.0F;
	float leftEdge = 0.0F;	//MI
	float bigTickLen=0.0F;
	float smallTickLen=0.0F;
	float tickInc=0.0F, delta=0.0F;
	float x1=0.0F, x2=0.0F, y1=0.0F;
	float winCenter = hudWinY[AIRSPEED_WINDOW] + hudWinHeight[AIRSPEED_WINDOW] * 0.5F;
	float boxY = winCenter + display->TextHeight() * 0.5F;
	int i=0, a=0;
	int origfont = 0;//TJL 03/07/04

	if(!ownship) {	// VWF added 10/20/98 to avoid crash
		return;
	}

	//rightEdge = hudWinX[AIRSPEED_WINDOW] + hudWinWidth[AIRSPEED_WINDOW];
	rightEdge = hudWinX[AIRSPEED_WINDOW] + (hudWinWidth[AIRSPEED_WINDOW] * 0.5f);

	//MI
	//leftEdge = hudWinX[ALTITUDE_WINDOW] - (hudWinWidth[ALTITUDE_WINDOW] * 0.5F);
	leftEdge = hudWinX[AIRSPEED_WINDOW] - (hudWinWidth[AIRSPEED_WINDOW] * 0.5F);

	//MI
	if(g_bRealisticAvionics && (((AircraftClass*)ownship)->af->gearPos > 0.5F)){
		a = FloatToInt32(cockpitFlightData.kias);
		DrawWindowString (2, "C");
	}
	else{
		switch (velocitySwitch)	{
			case CAS:
				a = FloatToInt32(cockpitFlightData.kias);
				DrawWindowString (2, "C");
				break;

			case TAS:
				a = FloatToInt32(cockpitFlightData.vt * FTPSEC_TO_KNOTS);
				DrawWindowString (2, "T");
				break;

			case GND_SPD:
				a = FloatToInt32((float)sqrt(cockpitFlightData.xDot*cockpitFlightData.xDot +
							cockpitFlightData.yDot*cockpitFlightData.yDot) * FTPSEC_TO_KNOTS);
				DrawWindowString (2, "G");
				break;
		}
	}

	//Cobra let's add a slight delay
	int temp = FloatToInt32(cockpitFlightData.kias);
	if ((unsigned long)hudDelayTimer < SimLibElapsedTime)
	{
		aspeedHud=a;
		hudDelayTimer = SimLibElapsedTime + 250;
	}

	// M.N. added full realism mode
	if ((PlayerOptions.GetAvionicsType() == ATRealistic) || (PlayerOptions.GetAvionicsType() == ATRealisticAV)){
		if (temp < 60){
			aspeedHud = 0;//Cobra
			a = 0;
		}
		else {
			aspeedHud = max(aspeedHud, 60);
		}
	}
	sprintf (tmpStr, "%4d", min(aspeedHud, 9999));


	//MI hack to make the new code working when below 100 kts :-(
	if(aspeedHud < 100)
	{
		sprintf (tmpStr, " %03d", aspeedHud);
		tmpStr[4] = '\0';
	}
	ShiAssert (strlen(tmpStr) < sizeof(tmpStr));

	//TJL 03/07/04 Removing ticks from everything but F-16 or default HUD
	//if ((scalesSwitch == VAH || scalesSwitch == VV_VAH) && (FCC->GetMasterMode() != FireControlComputer::Dogfight))//me123 status test.
	if (
		(ownship->IsF16() || (ownship->af->GetTypeAC() == 0)) && 
		((scalesSwitch == VAH) || (scalesSwitch == VV_VAH)) && 
		(FCC->GetMasterMode() != FireControlComputer::Dogfight))//me123 status test.
	{  
		//MI
		if(!g_bRealisticAvionics){
			if (scalesSwitch == VAH){
				boxY = 2.0F;
			}
			else{
				display->TextRight(rightEdge - 0.03F, boxY, tmpStr, 8);
			}
			bigTickLen = hudWinWidth[AIRSPEED_WINDOW] * 0.5F;
			smallTickLen = bigTickLen * 0.5F;
			tickInc = hudWinHeight[AIRSPEED_WINDOW] / (float)(NUM_VERTICAL_TICKS - 1);
			display->Line(rightEdge * 0.95F, winCenter, rightEdge * 0.95F + bigTickLen, winCenter);
		}
		else {
			// this is the line which draws the airspeed box
			display->TextRight(rightEdge - 0.06F, boxY, tmpStr, 8);
			bigTickLen = hudWinWidth[AIRSPEED_WINDOW] * 0.2F;
			smallTickLen = bigTickLen * 0.6F;
			tickInc = hudWinHeight[AIRSPEED_WINDOW] / (float)(NUM_VERTICAL_TICKS - 1);
			display->Line(rightEdge * 0.95F, winCenter, rightEdge * 0.95F + hudWinWidth[AIRSPEED_WINDOW] * 0.5F, winCenter);
		}
		x1 = rightEdge;
		y1 = hudWinY[AIRSPEED_WINDOW] - (a%10) * tickInc * 0.1F;
		a = a/10 - 5;

		for (i=0; i<NUM_VERTICAL_TICKS; i++)
		{
			//MI don't draw into the text!
			/*if (a >= 0 &&
			  (y1 - boxY > display->TextHeight() * 1.1F ||
			  y1 - boxY < -tickInc))*/
			if(!g_bRealisticAvionics)
			{
				if((a >= 0) && ((y1 - boxY > tickInc) || (y1 - boxY < (-tickInc * 3.0F))))
				{
					if (a % 5){
						x2 = x1 - smallTickLen;
					}
					else {
						x2 = x1 - bigTickLen;
						display->TextRightVertical(x2 - smallTickLen * 0.5F, y1 + 0.01F, hudNumbers[min (a, 99)]);
					}
					display->Line (x1, y1, x2, y1);
				}
			}
			else
			{
				if (a % 5){
					x2 = x1 - smallTickLen;
				}
				else{
					x2 = x1 - bigTickLen;
				}
				if(!(a % 5)){
					if(a >= 0 && (y1 - boxY > tickInc || y1 - boxY < (-tickInc * 3.0F))){
						display->TextRightVertical(x2 - smallTickLen * 0.5F, y1 + 0.01F, hudNumbers[min (a, 99)]);			  
					}
				}
				display->Line (x1, y1, x2, y1);
			}                    
			a++;
			y1 += tickInc;
		}

		// Draw Desired speed caret
		{
			//MI we don't get this with gear down
			if(!g_bRealisticAvionics)
			{
				y1 = hudWinY[AIRSPEED_WINDOW] + hudWinHeight[AIRSPEED_WINDOW] * 0.5F;
				x1 = rightEdge;

				delta = waypointSpeed - (cockpitFlightData.kias);
				delta *= 0.1F;
				y1 += delta * tickInc;
				y1 = max ( min ( y1, hudWinY[AIRSPEED_WINDOW] + hudWinHeight[AIRSPEED_WINDOW] - smallTickLen),
						hudWinY[AIRSPEED_WINDOW]);
				display->Line (x1, y1, x1 + bigTickLen, y1 + (smallTickLen * 0.5F));
				display->Line (x1, y1, x1 + bigTickLen, y1 - (smallTickLen * 0.5F));
			}
			else {
				if(((AircraftClass*)ownship)->af->gearPos < 0.5F) {
					// sfr: changed order here (no semantic change, was if else if)
					//CruiseTOS in all modes, others only in NAV
					if (
						(FCC->GetMasterMode() == FireControlComputer::Nav) ||
						(OTWDriver.pCockpitManager->mpIcp->GetCruiseIndex() == 0)
					){
						DrawCruiseIndexes();
					}
				}
			}
		}
	}
	else
	{
		//MI
		if(!g_bRealisticAvionics)
		{
			display->TextRight (rightEdge - 0.03F, boxY, tmpStr, 8);
			//      if (FCC->GetMasterMode() == FireControlComputer::Nav)
			{
				// Desired speed to reach waypoint on time
				sprintf (tmpStr, "%.0f", max( min(waypointSpeed, 9999.0F), 0.0F));
				ShiAssert (strlen(tmpStr) < sizeof(tmpStr));
				DrawWindowString (35, tmpStr);
			}
		}
		else
		{
			//ATARIBABY to not SPD box jump if no scales or DF master mode //Cobra 11/04/04 TJL
			//display->TextRight (rightEdge - 0.03F, boxY, tmpStr, 8);
			display->TextRight (rightEdge - 0.06F, boxY, tmpStr, 8);
			if (FCC->GetMasterMode() == FireControlComputer::Nav)
			{
				// Desired speed to reach waypoint on time
				//sprintf (SpeedText, "%.0f", max( min(waypointSpeed, 9999.0F), 0.0F));
				//MI
				if(g_bRealisticAvionics)
				{
					if(((AircraftClass*)ownship)->af->gearPos < 0.5F)
					{
						//CruiseTOS in all modes, others only in NAV
						if(FCC->GetMasterMode() == FireControlComputer::Nav)
							DrawCruiseIndexes();
						else
						{
							if(OTWDriver.pCockpitManager->mpIcp->GetCruiseIndex() == 0)
								DrawCruiseIndexes();
						}
					}
				}
				else
				{
					DrawCruiseIndexes();
				}
				ShiAssert (strlen(SpeedText) < sizeof(SpeedText));
				//  DrawWindowString (35, SpeedText); //JPG 29 Apr 04 - This should not be here.
			}
		}
	}
}

void HudClass::DrawAltitude(void)
{
	float leftEdge;
	float bigTickLen;
	float smallTickLen;
	float tickInc;
	float x1, x2, y1, y2;
	float winCenter, boxY;
	int i, a, lowAlt;
	float theAlt;
	//MI moved to the Class
	//char tmpStr[12];
	//float hat;
	int tickInterval, labelMod, labelInterval;
	char* formatStr;
	char highFormat[] = "%02d,%d";
	char lowFormat[] = "%d%02d";


	if(ownship == NULL) {	// vwf: to avoid crash after ownship gets destroyed.
		return;
	}

	// Height Above Terrain
	hat = cockpitFlightData.z - OTWDriver.GetGroundLevel (ownship->XPos(), ownship->YPos());

	// Max hat if no rad alt
	if (ownship->mFaults && ownship->mFaults->GetFault(FaultClass::ralt_fault))
		hat = -999999.9F;

	// Window 25 (ALOW warning)
	if(!g_bRealisticAvionics)
	{
		//MI original code
		if (-hat < lowAltWarning && flash && ((AircraftClass*)ownship)->af->gearPos < 0.5F)
		{
			sprintf (tmpStr, "AL %.0f", -hat);
			ShiAssert (strlen(tmpStr) < 40);
			DrawWindowString (25, tmpStr);
			F4SoundFXSetDist( ownship->af->GetAltitudeSnd(), FALSE, 0.0f, 1.0f );
		}
	}
	else
	{
		//MI modified stuff
		if(((AircraftClass*)ownship)->af->platform->IsPlayer())
		{
			if(radarSwitch == BARO){
				DrawRALTBox();
				DrawRALT();
			}
			else if(radarSwitch == RADAR_AUTO){
				if(-cockpitFlightData.z >= 1200){
					DrawRALTBox();
					DrawRALT();
				}
				else {
					if (-hat < lowAltWarning && (FindRollAngle(-hat) && FindPitchAngle(-hat)) &&
							((AircraftClass*)ownship)->af->platform->RaltReady())
					{
						if(Warnflash && !((AircraftClass*)ownship)->OnGround()){
							DrawALString();
						}
						else if(((AircraftClass*)ownship)->OnGround()){
							DrawALString();
						}
						if(((AircraftClass*)ownship)->af->gearPos < 0.8F){
							F4SoundFXSetDist(ownship->af->GetAltitudeSnd(), FALSE, 0.0f, 1.0f );
						}
					}
					else{
						DrawALString();
					}
				}
			}
			else {
				if(FCC->GetMasterMode() !=FireControlComputer::Dogfight && FCC->GetMasterMode() !=FireControlComputer::MissileOverride)
				{
					if(-hat < lowAltWarning && (FindRollAngle(-hat) && FindPitchAngle(-hat)) &&
							((AircraftClass*)ownship)->af->platform->RaltReady())
					{
						if(Warnflash && !((AircraftClass*)ownship)->OnGround()){
							DrawALString();
						}
						else if(((AircraftClass*)ownship)->OnGround()){
							DrawALString();
						}
						if(((AircraftClass*)ownship)->af->gearPos < 0.8F){
							F4SoundFXSetDist(ownship->af->GetAltitudeSnd(), FALSE, 0.0f, 1.0f );
						}
					}
					else {
						DrawALString();
					}
				}
			}
		}
	}



	winCenter = hudWinY[ALTITUDE_WINDOW] + hudWinHeight[ALTITUDE_WINDOW] * 0.5F;
	boxY = winCenter + display->TextHeight() * 0.5F;
	//TJL 03/07/04 Only for F16 or default HUD
	if ((ownship->IsF16() || ownship->af->GetTypeAC() == 0) && (scalesSwitch == VAH  || scalesSwitch == VV_VAH) 
			&& (FCC->GetMasterMode() !=FireControlComputer::Dogfight))//me123 status test.
	{
		//MI
		if(!g_bRealisticAvionics)
		{
			if (scalesSwitch == VAH)
				boxY = 2.0F;
		}
		//MI
		if(!g_bRealisticAvionics)
		{
			bigTickLen = hudWinWidth[ALTITUDE_WINDOW] * 0.5F;
			smallTickLen = bigTickLen * 0.5F;
		}
		else
		{
			bigTickLen = hudWinWidth[ALTITUDE_WINDOW] * 0.2F;
			smallTickLen = bigTickLen * 0.55F;
		}

		leftEdge = hudWinX[ALTITUDE_WINDOW];
		/*bigTickLen = hudWinWidth[ALTITUDE_WINDOW] * 0.5F;
		  smallTickLen = bigTickLen * 0.5F;*/
		tickInc = hudWinHeight[ALTITUDE_WINDOW] / (float)(NUM_VERTICAL_TICKS - 1);

		// Choose the right scale
		if (radarSwitch == BARO)
		{
			theAlt = cockpitFlightData.z;
		}
		else if (radarSwitch == ALT_RADAR)
		{
			//MI for CARA switch
			if(g_bRealisticAvionics)
			{
				if(((AircraftClass*)ownship)->af->platform->IsPlayer() &&
						((AircraftClass*)ownship)->af->platform->RaltReady())
				{
					if(FindRollAngle(-hat) && FindPitchAngle(-hat))
					{
						//CARA is ready and we're within limits
						DrawWindowString(6, "R");
						theAlt = hat;
					}
					else
						theAlt = cockpitFlightData.z;
				}
				else
				{
					//CARA is not yet ready
					theAlt = cockpitFlightData.z;
				}
			}
			else
			{
				DrawWindowString(6, "R");
				theAlt = hat;
			}
		}
		else
		{
			if (hat > -1200.0F || (cockpitFlightData.zDot < 0.0F && hat > -1500.0F))
			{
				//MI for CARA switch
				if(g_bRealisticAvionics)
				{
					if(((AircraftClass*)ownship)->af->platform->IsPlayer() &&
							((AircraftClass*)ownship)->af->platform->RaltReady())
					{
						if(FindRollAngle(-hat) && FindPitchAngle(-hat))
						{ 
							//CARA is ready and we're within limits
							DrawWindowString(6, "R");
							theAlt = hat;
						}
						else
							theAlt = cockpitFlightData.z;
					}
					else
					{
						//CARA is not yet ready
						theAlt = cockpitFlightData.z;
					}
				}
				else
				{
					DrawWindowString(6, "R");
					theAlt = hat;
				}
			}
			else
			{
				theAlt = cockpitFlightData.z;
			}
		}

		a = -FloatToInt32(theAlt);
		// Under 1500 going up or below 1200 going down
		if (a < 1200 || (cockpitFlightData.zDot < 0.0F && a < 1500)){
			lowAlt = TRUE;
		}
		else{
			lowAlt = FALSE;
		}

		// Non-Auto altitude scale
		if (radarSwitch != RADAR_AUTO || !lowAlt){
			if (lowAlt){
				tickInterval = 20;
				labelInterval = 100;
				labelMod = 100;
				formatStr = lowFormat;
			} 
			else {
				tickInterval = 100;
				labelInterval = 500;
				labelMod = 1000;
				formatStr = highFormat;
			}

			a -= labelInterval;
			a -= a%tickInterval;
			y1 = winCenter - (-theAlt - a) / tickInterval * tickInc;

			x1 = leftEdge;
			leftEdge *= 0.95F;   // JPG 0.95F

			// Draw index tick
			//MI
			if(g_bRealisticAvionics)
			{
				if (!(FCC->GetMasterMode() == FireControlComputer::Missile) &&
						!(FCC->GetMasterMode() == FireControlComputer::Dogfight) &&
						!(FCC->GetMasterMode() == FireControlComputer::MissileOverride))
				{
					display->Line(leftEdge, winCenter,
							leftEdge - hudWinWidth[ALTITUDE_WINDOW] * 0.5F, winCenter);
				}
			}
			else{
				display->Line (leftEdge, winCenter, leftEdge - bigTickLen, winCenter);
			}

			//Cobra Add in the delay 
			if ((unsigned long)hudAltDelayTimer < SimLibElapsedTime) {
				altHud=theAlt;
				altHudn=-theAlt;//Seed the other while in this loop
				hudAltDelayTimer = SimLibElapsedTime + 250;
			}

			// Add Discretes to tape
			if (theAlt > 0.0F){
				//MI
				if(g_bRealisticAvionics){
					//only show 10's of feet
					theAlt = ((static_cast<int>(altHud) + 5) / 10) * 10.0f;
				}
				sprintf (tmpStr, "%2d,%03d", -FloatToInt32(theAlt * 0.001F),
						FloatToInt32(theAlt - FloatToInt32(theAlt * 0.001F) * 1000.0F));
			}
			else{
				//MI
				if (g_bRealisticAvionics) {
					//only show 10's of feet
					theAlt = ((static_cast<int>(altHud) + 5) / 10) * 10.0f;
				}
				sprintf (tmpStr, "%2d,%03d", -FloatToInt32(theAlt * 0.001F),
						-(FloatToInt32(theAlt - FloatToInt32(theAlt * 0.001F) * 1000.0F)));
			}
			//MI
			//ATARIBABY fix for missaligned alt readout in 3d padlock
			//if(!g_bRealisticAvionics || OTWDriver.GetOTWDisplayMode() == OTWDriverClass::Mode3DCockpit)
			// sfr: caused hud altitude missaligned
#if 0
			if (!g_bRealisticAvionics || OTWDriver.GetOTWDisplayMode() == OTWDriverClass::Mode3DCockpit || OTWDriver.GetOTWDisplayMode() == OTWDriverClass::ModePadlockF3)
			{
				display->TextRight(hudWinX[ALTITUDE_WINDOW] + 0.01F + hudWinWidth[ALTITUDE_WINDOW],
						boxY, tmpStr, 4);
			}
			else
			{
				display->TextRight(hudWinX[ALTITUDE_WINDOW] + 0.17F + hudWinWidth[ALTITUDE_WINDOW],
						boxY, tmpStr, 4);
			}
#endif

			// sfr: above code was causing altitude hud missalignment and changed from .17 to .22
			display->TextRight(hudWinX[ALTITUDE_WINDOW] + 0.22F + hudWinWidth[ALTITUDE_WINDOW], boxY, tmpStr, 4);

			for (i=0; i<NUM_VERTICAL_TICKS; i++)
			{
				//MI don't draw into the text!
				/*if (a >= 0 && 
				  (y1 - boxY > display->TextHeight() * 1.1F ||
				  y1 - boxY < -tickInc))*/
				if(!g_bRealisticAvionics)
				{
					if (a >= 0 && (y1 - boxY > tickInc || y1 - boxY < (-tickInc * 3.0F)))
					{ 
						if (a % labelInterval)
							x2 = x1 + smallTickLen;
						else
						{
							if (formatStr == lowFormat)
								sprintf (tmpStr, formatStr, a/labelMod, a%labelMod);
							else
								sprintf (tmpStr, formatStr, a/labelMod, (a%labelMod) / 100);
							ShiAssert (strlen(tmpStr) < 12);
							x2 = x1 + bigTickLen;
							display->TextLeftVertical(x2 + smallTickLen * 0.5F, y1 + 0.01F, tmpStr);
						}
						display->Line (x1, y1, x2, y1);
					}                    
				}
				else
				{
					if (a % labelInterval)
						x2 = x1 + smallTickLen;
					else
					{
						if (formatStr == lowFormat)
							sprintf (tmpStr, formatStr, a/labelMod, a%labelMod);
						else
							sprintf (tmpStr, formatStr, a/labelMod, (a%labelMod) / 100);
						ShiAssert (strlen(tmpStr) < 12);
						x2 = x1 + bigTickLen;
						if(a >= 0 && (y1 - boxY > tickInc || y1 - boxY < (-tickInc * 3.0F)))
							display->TextLeftVertical(x2 + smallTickLen * 0.5F, y1 + 0.01F, tmpStr);
					}
					display->Line (x1, y1, x2, y1);
				}
				a += tickInterval;
				y1 += tickInc;
			}
		}
		else
			// Auto display in low altitude mode
		{
			y1 = hudWinY[ALTITUDE_WINDOW];

			x1 = leftEdge;
			tickInc *= 0.75F;

			// Label the bottom
			display->Line (x1, y1, x1 + bigTickLen, y1);
			y1 += tickInc;

			// Draw the thermometer scale
			for (i=1; i<NUM_VERTICAL_TICKS * 2; i++)
			{
				if (i % 2)
					x2 = x1 + smallTickLen;
				else
				{
					x2 = x1 + bigTickLen;
					if (i % 4 == 0)
					{
						sprintf (tmpStr, "%d", i / 2);
						ShiAssert (strlen(tmpStr) < 12);
						display->TextLeftVertical(x2 + smallTickLen * 0.5F, y1 + 0.01F, tmpStr);
					}
				}
				display->Line (x1, y1, x2, y1);

				y1 += tickInc;
			}
			x2 = x1 + bigTickLen;
			display->Line (x1, y1, x2, y1);
			display->TextLeft(x2 + smallTickLen * 0.5F, y1 + 0.01F, "15");

			// Add the current altitude
			x1 *= 0.95F;
			y1 = hudWinY[ALTITUDE_WINDOW];
			if (-theAlt < 1000.0F)
			{
				y2 = -theAlt / 1000.0F * (NUM_VERTICAL_TICKS * 2 - 2) * tickInc;
			}
			else
			{
				y2 = (NUM_VERTICAL_TICKS * 2 - 2) * tickInc + (-theAlt - 1000.0F) / 500.0F * 2 * tickInc;
			}
			display->Line (x1, y1, x1 - smallTickLen, y1);
			display->Line (x1 - smallTickLen, y1, x1 - smallTickLen, y1 + y2);
			display->Line (x1, y1 + y2, x1 - smallTickLen, y1 + y2);

			// Add the current alow setting
			if (lowAltWarning < 1000.0F)
			{
				y2 = lowAltWarning / 1000.0F * (NUM_VERTICAL_TICKS * 2 - 2) * tickInc;
			}
			else
			{
				y2 = (NUM_VERTICAL_TICKS * 2 - 2) * tickInc + (lowAltWarning - 1000.0F) / 500.0F * 2 * tickInc;
			}
			display->Line (x1 - bigTickLen, y1 + y2, x1 - smallTickLen, y1 + y2);
			display->Line (x1 - bigTickLen, y1 + y2 - smallTickLen, x1 - bigTickLen, y1 + y2 + smallTickLen);
		}

		// Add Vertical Velocity if needed
		if (FCC->GetMasterMode() == FireControlComputer::Nav && scalesSwitch == VV_VAH)
		{
			tickInc = hudWinHeight[ALTITUDE_WINDOW] / 9;
			leftEdge -= 2.0F * bigTickLen;
			y1 = winCenter - 6.0F * tickInc;
			x1 = leftEdge + bigTickLen;
			x2 = leftEdge + smallTickLen;

			// Add Scale
			for (i=0; i<14; i++)
			{
				if (i % 2 == 1)
					display->Line (leftEdge, y1, x1, y1);
				else
					display->Line (leftEdge, y1, x2, y1);

				y1 += tickInc;
			}

			// Add marker NOTE: ZDelta is Ft/Sec
			y1 = (-cockpitFlightData.zDot * MIN_TO_SEC) / 500.0F * tickInc;
			y1 = min ( max (y1, -6.0F * tickInc), 6.0F * tickInc);
			y1 += winCenter;
			display->Line (leftEdge, y1, leftEdge - smallTickLen, y1 + smallTickLen);
			display->Line (leftEdge, y1, leftEdge - smallTickLen, y1 - smallTickLen);
			display->Line (leftEdge - smallTickLen, y1 + smallTickLen, leftEdge - smallTickLen, y1 - smallTickLen);

		}
	}
	else	//Scales OFF
	{

		// Choose the right scale
		if (radarSwitch == BARO)
		{
			theAlt = -cockpitFlightData.z;
		}
		else if (radarSwitch == ALT_RADAR)
		{
			//MI for CARA switch
			if(g_bRealisticAvionics)
			{
				if(((AircraftClass*)ownship)->af->platform->IsPlayer() &&
						((AircraftClass*)ownship)->af->platform->RaltReady())
				{
					if(FindRollAngle(-hat) && FindPitchAngle(-hat))
					{
						//CARA is ready and we're within limits
						DrawWindowString(6, "R");
						theAlt = -hat;
					}
					else
						theAlt = -cockpitFlightData.z;
				}
				else
				{
					//CARA is not yet ready
					theAlt = -cockpitFlightData.z;
				}
			}
			else
			{
				DrawWindowString(6, "R");
				theAlt = -hat;
			}
		}
		else
		{
			if (hat > -1200.0F || (cockpitFlightData.zDot < 0.0F && hat > -1500.0F))
			{
				//MI for CARA switch
				if(g_bRealisticAvionics)
				{
					if(((AircraftClass*)ownship)->af->platform->IsPlayer() &&
							((AircraftClass*)ownship)->af->platform->RaltReady())
					{
						if(FindRollAngle(-hat) && FindPitchAngle(-hat))
						{ 
							//CARA is ready and we're within limits
							DrawWindowString(6, "R");
							theAlt = -hat;
						} 
						else
							theAlt = -cockpitFlightData.z;
					}
					else
					{
						//CARA is not yet ready
						theAlt = -cockpitFlightData.z;
					}

				}
				else
				{
					DrawWindowString(6, "R");
					theAlt = -hat;
				}
			}
			else
			{
				theAlt = -cockpitFlightData.z;
			}
		}
		//Cobra Add in the delay
		if ((unsigned long)hudAltDelayTimer < SimLibElapsedTime)
		{
			altHudn=theAlt;
			altHud = -theAlt;//seed the other while in this loop
			hudAltDelayTimer = SimLibElapsedTime + 250;
		}

		if (theAlt > 0.0F)
		{
			//MI
			if(g_bRealisticAvionics)
			{
				//only show 10's of feet
				theAlt = ((static_cast<int>(altHudn) + 5) / 10) * 10.0f;
			}
			sprintf (tmpStr, "%2d,%03d", FloatToInt32(theAlt * 0.001F),
					FloatToInt32(theAlt - FloatToInt32(theAlt * 0.001F) * 1000.0F));
		}
		else
		{
			//MI
			if(g_bRealisticAvionics)
			{
				//only show 10's of feet
				theAlt = ((static_cast<int>(altHudn) + 5) / 10) * 10.0f;
			}
			sprintf (tmpStr, "-%2d,%03d", abs(FloatToInt32(theAlt * 0.001F)),
					abs(FloatToInt32(theAlt - FloatToInt32(theAlt * 0.001F) * 1000.0F)));
		}
		ShiAssert (strlen(tmpStr) < 12);
		//MI
		//ATARIBABY fix for missaligned alt readout in 3d padlock
		//if(!g_bRealisticAvionics || OTWDriver.GetOTWDisplayMode() == OTWDriverClass::Mode3DCockpit)
		// sfr: caused hud missalignment
#if 0
		if(!g_bRealisticAvionics || OTWDriver.GetOTWDisplayMode() == OTWDriverClass::Mode3DCockpit || OTWDriver.GetOTWDisplayMode() == OTWDriverClass::ModePadlockF3)
			display->TextRight (hudWinX[ALTITUDE_WINDOW] + 0.01F + hudWinWidth[ALTITUDE_WINDOW],
					boxY, tmpStr, 4);
		else
			display->TextRight (hudWinX[ALTITUDE_WINDOW] + 0.17F + hudWinWidth[ALTITUDE_WINDOW],
					boxY, tmpStr, 4);
#endif
		// sfr: changed .17 to .20
		display->TextRight(hudWinX[ALTITUDE_WINDOW] + hudWinWidth[ALTITUDE_WINDOW] + 0.22F, boxY, tmpStr, 4);

	}
	//MI MSL Floor Check
	if(g_bRealisticAvionics)
	{
		CheckMSLFloor();
	}
}

static const float HeadingWidthDiff = 0.15F;
void HudClass::DrawHeading(void)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	char tmpStr[12]={0};
	float vert[2][2]={0.0F};
	int i=0, a=0, val=0;
	float headingTop=0.0F;
	float bigTickLen=0.0F, smallTickLen=0.0F, tickInc=0.0F;
	float textWidth=0.0F;

	float dy;
	static float HEADING_BOTTOM;

	if(g_bFallingHeadingTape)
		HEADING_BOTTOM = -2.0F;
	else
		HEADING_BOTTOM = -0.82F;

	if (headingPos == Off || (FCC->GetMasterMode() ==FireControlComputer::Dogfight))//me123 status test.)
	{
		return;
	}
	else if (headingPos == High || ((AircraftClass*)ownship)->af->gearPos > 0.5F)
	{
		//MI
		if(!g_bRealisticAvionics || ((AircraftClass*)ownship)->OnGround() ||
				(g_bRealisticAvionics && g_bINS && ownship && ownship->INSState(AircraftClass::INS_PowerOff) ||
				 !ownship->INSState(AircraftClass::INS_HUD_STUFF)))
		{

			headingTop = hudWinY[HEADING_WINDOW_HI] +
				hudWinHeight[HEADING_WINDOW_HI] * 0.5F;
		}
		else
			headingTop = (hudWinY[HEADING_WINDOW_LO] - 0.2F) +	//use this to make it always follow FPM	
				hudWinHeight[HEADING_WINDOW_HI] * 0.5F;

		// MI
		if (g_bRealisticAvionics && GetDriftCOSwitch() == DRIFT_CO_OFF)
		{
			if(((AircraftClass*)ownship)->af->gearPos > 0.5F && !((AircraftClass*)ownship->OnGround()))
			{
				dy = hudWinY[BORESIGHT_CROSS_WINDOW] +
					hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F -
					alphaHudUnits + 0.4F;

				if (dy > headingTop)
					headingTop = dy;
			}
			else
			{
				headingTop = hudWinY[HEADING_WINDOW_HI] +
					hudWinHeight[HEADING_WINDOW_HI] * 0.5F;
			}
		}
		if(GetDriftCOSwitch() != DRIFT_CO_OFF)
		{
			headingTop = hudWinY[HEADING_WINDOW_HI] +
				hudWinHeight[HEADING_WINDOW_HI] * 0.5F;
		}
	}
	else if (headingPos == Low)
	{
		headingTop = hudWinY[HEADING_WINDOW_LO] +
			hudWinHeight[HEADING_WINDOW_LO] * 0.5F;
		// Marco edit - Scroll heading tape downwards with FPM
		// if it's down the bottom
		if(g_bRealisticAvionics && GetDriftCOSwitch() == DRIFT_CO_OFF)
		{
			dy = hudWinY[BORESIGHT_CROSS_WINDOW] +
				hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F -
				alphaHudUnits - 0.12f;

			if(dy < HEADING_BOTTOM)	//don't fall off of the HUD. This can be seen in some of the vids
				dy = HEADING_BOTTOM;	//not sure if the position is right, but I think you should always see it
			//so let's keep it above the projector thingie
			if (dy < headingTop)	
				headingTop = dy;

		}
		//MI INS stuff
		if(g_bRealisticAvionics && g_bINS && ownship && ownship->INSState(AircraftClass::INS_PowerOff) ||
				!ownship->INSState(AircraftClass::INS_HUD_STUFF))
		{
			headingTop = hudWinY[HEADING_WINDOW_LO] +
				hudWinHeight[HEADING_WINDOW_LO] * 0.5F;
		}
	}
	tickInc = hudWinWidth[HEADING_WINDOW_HI] / (NUM_HORIZONTAL_TICKS + 1);

	/*   // Marco edit - Scroll heading tape downwards with FPM
	     if (g_bRealisticAvionics && dy < headingTop && ((AircraftClass*)ownship)->af->gearPos <= 0.5F)
	     {
	     headingTop = dy ;
	     }*/

	//MI make the spaces wider
	if(g_bRealisticAvionics)
		tickInc *= 2.0F;
	bigTickLen = hudWinHeight[HEADING_WINDOW_HI] * 0.5F;
	smallTickLen = hudWinHeight[HEADING_WINDOW_HI] * 0.25F;

	display->Line (0.0F, headingTop + smallTickLen,
			0.0F, headingTop + bigTickLen + smallTickLen);
	a = FloatToInt32(cockpitFlightData.yaw * 10.0F * RTD);
	if(!g_bRealisticAvionics)
		vert[0][0] = -(a%50) * tickInc * 0.02F - 4 * tickInc;
	else
		vert[0][0] = -(a%50) * tickInc * 0.02F - 2 * tickInc;
	vert[0][1] = headingTop;

	// OW: Sylvains Three-Digit heading patch
#if 1
	// ADDED BY S.G. SO 3 DIGIT ARE DISPLAYED FOR THE HEADING WHEN IN 'SS_OFF' MODE
	if (scalesSwitch == SS_OFF || scalesSwitch == VAH) {	//MI changed VV_VAH to VAH
		// a is multiplied by ten above, we don't need that here
		//MI 16/2/02 give us a 180 heading
		//MI better heading code
#if 0
		int tmpVal = 0;
		if(a > 1795 || a < -1795)
			tmpVal = 180;
		else
			tmpVal = (int)(a / 10);
		//int tmpVal = (((int)a + 5) / 10) * 10;
		// Heading 'tape' is NOT negative, convert to positive
		if (tmpVal < 0)
			tmpVal += 360;

		// Needed for the 'tick marks'
		a = a/50;

		sprintf (tmpStr, "%03d", tmpVal);
#else
		int tmpVal = a;
		if (tmpVal >= 0)
			tmpVal = (int)((a+5) / 10);
		else
			tmpVal = (int)((a-5) / 10);
		// Heading 'tape' is NOT negative, convert to positive
		if (tmpVal < 0)
			tmpVal += 360;

		// Needed for the 'tick marks'
		a = a/50;

		sprintf (tmpStr, "%03d", tmpVal);
#endif
	}
	else {
		// END OF ADDED SECTION - NEXT SECTION INDENTED SINCE IT'S NOW PART OF AN ELSE CLAUSE
		if (a % 100 > 50)
			val = a / 50 + 2;
		else
			val = a / 50;
		a = a/50;

		if (val <= 0)
			sprintf (tmpStr, "%02d", (val + 72) >> 1);    
		else
			sprintf (tmpStr, "%02d", val >> 1);
		// CLOSING BRACE ADDED BY S.G.
	}
#else
	if (a % 100 > 50)
		val = a / 50 + 2;
	else
		val = a / 50;
	a = a/50;

	if (val <= 0)
		sprintf (tmpStr, "%02d", (val + 72) >> 1);    
	else
		sprintf (tmpStr, "%02d", val >> 1);    
#endif

	ShiAssert (strlen(tmpStr) < sizeof(tmpStr));
	a -= 4;

	//MI
	if(g_bINS && g_bRealisticAvionics)
	{
		if(!ownship->INSState(AircraftClass::INS_HUD_STUFF))
			sprintf(tmpStr,"   ");

		display->TextCenter (0.0F, vert[0][1] - 0.075F, tmpStr, 1 );
	}
	else
		display->TextCenter (0.0F, vert[0][1] - 0.075F, tmpStr, 1 );

	textWidth = display->TextWidth (tmpStr);

	if (scalesSwitch == VAH || scalesSwitch == VV_VAH || scalesSwitch == H )
	{
		//MI
		if(!g_bRealisticAvionics)
		{
			for (i=0; i<NUM_HORIZONTAL_TICKS; i++)
			{ 
				vert[1][0] = vert[0][0];

				if (a & 1)
					vert[1][1] = vert[0][1] - smallTickLen;
				else
				{
					vert[1][1] = vert[0][1] - bigTickLen;
					if (a <= 0)
						sprintf (tmpStr, "%02d", (a + 72) >> 1);    
					else
						sprintf (tmpStr, "%02d", a >> 1);    
					ShiAssert (strlen(tmpStr) < sizeof(tmpStr));

					// don't draw text in virtual display -- too crowded
					if (fabs (vert[1][0]) > 1.5F * textWidth)
					{
						if (display->type == VirtualDisplay::DISPLAY_GENERAL)
						{
							display->TextCenter (vert[1][0], vert[0][1] - 0.075F, tmpStr );
						}
						else
						{ 
							if
								(
								 ((a < 0) && (((-a) % 6) < 2)) ||
								 ((a >= 0) && ((a % 6) < 2))
								)
								{
									display->TextCenter (vert[1][0], vert[0][1] - 0.075F, tmpStr );
								}
						}
					}
				} 
				display->Line (vert[0][0], vert[0][1], vert[1][0], vert[1][1]);

				a ++ ;
				vert[0][0] += tickInc;
			} 
		}
		else
		{
			for (i=0; i<NUM_HORIZONTAL_TICKS/2; i++)
			{ 
				vert[1][0] = vert[0][0];

				if (a & 1)
					vert[1][1] = vert[0][1] - smallTickLen;
				else
				{
					vert[1][1] = vert[0][1] - bigTickLen;
					if (scalesSwitch == VV_VAH) 
					{
						if (a <= 0)
						{
							sprintf (tmpStr, "%3d", ((a + 74) * 10) >> 1);
							if(atoi(tmpStr) == 370)	//MI HACK to prevent "37" beeing written
								sprintf(tmpStr,"010");
						}
						else
							sprintf (tmpStr, "%3d", ((a + 2) * 10) >> 1);
					}
					else 
					{
						if (a <= 0)
						{
							sprintf (tmpStr, "%02d", (a + 74) >> 1);
							if(atoi(tmpStr) == 37)	//MI HACK to prevent "37" beeing written
								sprintf(tmpStr,"01");
						}
						else
							sprintf (tmpStr, "%02d", (a + 2) >> 1);
					}

					ShiAssert (strlen(tmpStr) < sizeof(tmpStr));

					if (fabs (vert[1][0]) > 1.5F * textWidth)
					{
						// don't draw text in virtual display -- too crowded
						if (display->type == VirtualDisplay::DISPLAY_GENERAL)
						{
							if(g_bINS)
							{
								if(ownship->INSState(AircraftClass::INS_HUD_STUFF))
								{
									display->TextCenter (vert[1][0], vert[0][1] - 0.075F, tmpStr );
								}
							}
							else
								display->TextCenter (vert[1][0], vert[0][1] - 0.075F, tmpStr );
						}
						else
						{
							if
								(
								 ((a < 0) && (((-a) % 6) < 2)) ||
								 ((a >= 0) && ((a % 6) < 2))
								)
								{
									if(g_bINS)
									{
										if(ownship->INSState(AircraftClass::INS_HUD_STUFF))
										{
											display->TextCenter (vert[1][0], vert[0][1] - 0.075F, tmpStr );
										}
									}
									else
										display->TextCenter (vert[1][0], vert[0][1] - 0.075F, tmpStr );
								}
						}
					}
				} 
				display->Line (vert[0][0], vert[0][1], vert[1][0], vert[1][1]);

				a ++ ;
				vert[0][0] += tickInc;
			}
		}
	}

	DrawWaypoint();
	//MI
	if(!g_bRealisticAvionics)
		DrawTadpole();
	else
	{
		if(FCC && FCC->GetMasterMode() == FireControlComputer::ILS &&
				OTWDriver.pCockpitManager && OTWDriver.pCockpitManager->mpIcp->GetCMDSTR())
			DrawCMDSTRG();
		else
			DrawTadpole();
	}
	//MI
	if(!g_bRealisticAvionics)
		DrawRollCue();
	else
	{
		//MI appears to be based on mode and geardown/switch, not only switch
#if 0
		if(fpmSwitch != FPM_OFF && scalesSwitch == VV_VAH)
			DrawBankIndicator();
		else if(fpmSwitch != FPM_OFF && scalesSwitch == VAH)
			DrawRollCue();
#else
		if(FCC && FCC->IsAGMasterMode() || (playerAC->af->gearPos > 0.5F &&
					fpmSwitch != FPM_OFF && scalesSwitch == VV_VAH))
			DrawBankIndicator();
		//not there in Dogfight
		else
		{
			if(FCC && FCC->GetMasterMode() != FireControlComputer::Dogfight &&
					GetDEDSwitch() == DED_OFF)
				DrawRollCue();
		}
#endif
	}
	//MI
	if(g_bRealisticAvionics)
	{
		DrawOA();
		DrawVIP();
		DrawVRP();
	}
}


void HudClass::DrawTadpole (void)
{
	float dx, dy;
	float x, x1, y;
	float len;
	mlTrig trig;

	//MI INS stuff
	if(g_bRealisticAvionics && g_bINS)
	{
		if(ownship && ownship->INSState(AircraftClass::INS_PowerOff) ||
				!ownship->INSState(AircraftClass::INS_HUD_STUFF))
			return;
	}

	if (!waypointValid || (FCC->GetMasterMode() == FireControlComputer::Dogfight))//me123 status test.
	{
		return;
	}

	dx = betaHudUnits;
	dy = hudWinY[BORESIGHT_CROSS_WINDOW] +
		hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F -
		alphaHudUnits;

	display->AdjustOriginInViewport (dx, dy);
	display->AdjustRotationAboutOrigin(-cockpitFlightData.roll);
	x1 = max (min (waypointBearing / (10.0F * DTR), 1.0F), -1.0F) * hudWinWidth[PITCH_LADDER_WINDOW] * 0.75F;

	display->Circle (x1, 0.0F, RadToHudUnits(0.003F));
	//MI
	if(!g_bRealisticAvionics)
		len = RadToHudUnits(0.012F);
	else
		len = RadToHudUnits(0.016F);

	mlSinCos (&trig, waypointBearing + cockpitFlightData.roll);
	y = len * trig.cos;
	x = len * trig.sin;

	if(!g_bRealisticAvionics)
		display->Line (x1 + x, y, x1 + x * 0.25F, y * 0.25F);
	else
		display->Line (x1 + x, y, x1 + x * 0.1875F, y * 0.1875F);

	display->ZeroRotationAboutOrigin();

	display->AdjustOriginInViewport (-dx, -dy);
}

void HudClass::DrawILS (void)
{
	float hDev = 0.0F;   // glide path deviation.  Positive is glide path on right side
	float vDev = 0.0F;   // glide slope deviation.  Positive is glide slope above.
	int hValid, vValid;
	float xOffset, yOffset;

	//ATARIBABY ILS needles HUD fix
	// ILS needles on hud not get properly updated because it query pCockpitManager->mHiddenFlag 
	// and this seems not get updated in 3d pit view
	// if (gNavigationSys && !OTWDriver.pCockpitManager->mHiddenFlag)
	if (gNavigationSys && (gNavigationSys->GetInstrumentMode() == NavigationSystem::ILS_TACAN ||
				gNavigationSys->GetInstrumentMode() == NavigationSystem::ILS_NAV) &&
			gNavigationSys->GetILSAttribute(NavigationSystem::GP_DEV, &hDev))
	{
		gNavigationSys->GetILSAttribute(NavigationSystem::GP_DEV, &hDev);
		gNavigationSys->GetILSAttribute(NavigationSystem::GS_DEV, &vDev);
		hDev *= RTD;
		vDev *= RTD;
		hValid = TRUE;
		vValid = TRUE;
	}
	else
	{
		hValid = FALSE;
		vValid = FALSE;
	}

	hDev = min ( max (hDev, -3.75F), 3.75F) / 3.75F;
	vDev = min ( max (vDev, -0.75F), 0.75F) / 0.75F;

	// Draw symbology centered about FPM
	xOffset = betaHudUnits;
	yOffset = hudWinY[BORESIGHT_CROSS_WINDOW] +
		hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F -
		alphaHudUnits;
	display->AdjustOriginInViewport( xOffset,  yOffset);

	// Horizontal Dev
	if(hValid)
	{
		display->Line (0.18F * hDev, 0.18F, 0.18F * hDev, -0.18F);
	}
	else
	{
		display->Line (0.18F * hDev, 0.15F, 0.18F * hDev,  0.09F);
		display->Line (0.18F * hDev,-0.15F, 0.18F * hDev, -0.09F);
	}
	//MI
	if(!g_bRealisticAvionics)
	{
		display->Line (0.18F * hDev - 0.03F, 0.18F, 0.18F * hDev + 0.03F, 0.18F);
		display->Line (0.18F * hDev - 0.03F, 0.12F, 0.18F * hDev + 0.03F, 0.12F);
		display->Line (0.18F * hDev - 0.03F, 0.06F, 0.18F * hDev + 0.03F, 0.06F);
		display->Line (0.18F * hDev - 0.03F, 0.00F, 0.18F * hDev + 0.03F, 0.00F);
		display->Line (0.18F * hDev - 0.03F,-0.18F, 0.18F * hDev + 0.03F,-0.18F);
		display->Line (0.18F * hDev - 0.03F,-0.12F, 0.18F * hDev + 0.03F,-0.12F);
		display->Line (0.18F * hDev - 0.03F,-0.06F, 0.18F * hDev + 0.03F,-0.06F);
	}
	else
	{
		//lines are 2° left and right, not like above
		display->Line (0.18F * hDev - 0.018F, 0.18F, 0.18F * hDev + 0.018F, 0.18F);
		display->Line (0.18F * hDev - 0.018F, 0.09F, 0.18F * hDev + 0.018F, 0.09F);
		display->Line (0.18F * hDev - 0.018F,-0.18F, 0.18F * hDev + 0.018F,-0.18F);
		display->Line (0.18F * hDev - 0.018F,-0.09F, 0.18F * hDev + 0.018F,-0.09F);
	}

	// Vertical Dev
	if(vValid)
	{
		display->Line (0.18F, 0.18F * vDev, -0.18F, 0.18F * vDev);
	}
	else
	{
		display->Line ( 0.15F, 0.18F * vDev,  0.09F, 0.18F * vDev);
		display->Line (-0.15F, 0.18F * vDev, -0.09F, 0.18F * vDev);
	}
	//MI
	if(!g_bRealisticAvionics)
	{
		display->Line ( 0.18F, 0.18F * vDev - 0.03F, 0.18F, 0.18F * vDev + 0.03F);
		display->Line ( 0.12F, 0.18F * vDev - 0.03F, 0.12F, 0.18F * vDev + 0.03F);
		display->Line ( 0.06F, 0.18F * vDev - 0.03F, 0.06F, 0.18F * vDev + 0.03F);
		display->Line ( 0.00F, 0.18F * vDev - 0.03F, 0.00F, 0.18F * vDev + 0.03F);
		display->Line (-0.18F, 0.18F * vDev - 0.03F,-0.18F, 0.18F * vDev + 0.03F);
		display->Line (-0.12F, 0.18F * vDev - 0.03F,-0.12F, 0.18F * vDev + 0.03F);
		display->Line (-0.06F, 0.18F * vDev - 0.03F,-0.06F, 0.18F * vDev + 0.03F);
	}
	else
	{
		//lines are 2° left and right, not like above
		display->Line ( 0.18F, 0.18F * vDev - 0.018F, 0.18F, 0.18F * vDev + 0.018F);
		display->Line ( 0.09F, 0.18F * vDev - 0.018F, 0.09F, 0.18F * vDev + 0.018F);
		display->Line (-0.18F, 0.18F * vDev - 0.018F,-0.18F, 0.18F * vDev + 0.018F);
		display->Line (-0.09F, 0.18F * vDev - 0.018F,-0.09F, 0.18F * vDev + 0.018F);
	}
	display->AdjustOriginInViewport(-xOffset, -yOffset);
}
const static float Lenght = 0.04F;
void HudClass::DrawWaypoint (void)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	//MI INS stuff
	if(g_bRealisticAvionics && g_bINS)
	{
		if(ownship && !ownship->INSState(AircraftClass::INS_HUD_STUFF))
			return;
	}
	float headingTop;
	float tickInc, tickLen;
	float headingError;
	float xPos, yPos;

	float dy;
	static float HEADING_BOTTOM;

	if(g_bFallingHeadingTape)
		HEADING_BOTTOM = -2.0F;
	else
		HEADING_BOTTOM = -0.82F;

	if (!waypointValid || ownship == NULL || (FCC->GetMasterMode() == FireControlComputer::Dogfight))//me123 status test.)
	{
		return;
	}

	tickInc = hudWinWidth[HEADING_WINDOW_HI] / (NUM_HORIZONTAL_TICKS + 1);
	tickLen = hudWinHeight[HEADING_WINDOW_HI] * 0.5F;
	headingError = waypointBearing * RTD;
	if (headingError > 180.0F)
		headingError -= 360.0F;
	headingError = max ( min (headingError, 25.0F), -25.0F);
	headingError /= 5.0F;
	headingError *= tickInc;

	if (headingPos == Off)
	{
		return;
	}
	else if (headingPos == High || ((AircraftClass*)ownship)->af->gearPos > 0.5F)
	{
		//MI
		if(!g_bRealisticAvionics || ((AircraftClass*)ownship)->OnGround() ||
				(g_bRealisticAvionics && g_bINS && ownship && ownship->INSState(AircraftClass::INS_PowerOff) ||
				 !ownship->INSState(AircraftClass::INS_HUD_STUFF)))
		{

			headingTop = hudWinY[HEADING_WINDOW_HI] +
				hudWinHeight[HEADING_WINDOW_HI] * 0.5F;
		}
		else
			headingTop = (hudWinY[HEADING_WINDOW_LO] - 0.2F) +	//use this to make it always follow FPM	
				hudWinHeight[HEADING_WINDOW_HI] * 0.5F;

		// MI
		if (g_bRealisticAvionics && GetDriftCOSwitch() == DRIFT_CO_OFF)
		{
			if(((AircraftClass*)ownship)->af->gearPos > 0.5F && !((AircraftClass*)ownship->OnGround()))
			{
				dy = hudWinY[BORESIGHT_CROSS_WINDOW] +
					hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F -
					alphaHudUnits + 0.4F;

				if (dy > headingTop)
					headingTop = dy;
			}
			else
			{
				headingTop = hudWinY[HEADING_WINDOW_HI] +
					hudWinHeight[HEADING_WINDOW_HI] * 0.5F;
			}
		}
		if(GetDriftCOSwitch() != DRIFT_CO_OFF)
		{
			headingTop = hudWinY[HEADING_WINDOW_HI] +
				hudWinHeight[HEADING_WINDOW_HI] * 0.5F;
		}
	}
	else
	{
		headingTop = hudWinY[HEADING_WINDOW_LO] +
			hudWinHeight[HEADING_WINDOW_LO] * 0.5F;
		// Marco edit - Scroll heading tape downwards with FPM
		// if it's down the bottom
		if (g_bRealisticAvionics && GetDriftCOSwitch() == DRIFT_CO_OFF)
		{
			dy = hudWinY[BORESIGHT_CROSS_WINDOW] +
				hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F -
				alphaHudUnits - 0.12f;

			if(dy < HEADING_BOTTOM)	//don't fall off of the HUD. This can be seen in some of the vids
				dy = HEADING_BOTTOM;	//not sure if the position is right, but I think you should always see it
			//so let's keep it above the projector thingie

			if (dy < headingTop)
				headingTop = dy;
		}
	}
	//MI
	if(!g_bRealisticAvionics)
	{
		display->Line (headingError, headingTop + tickLen * 0.1F,
				headingError + 0.1F * tickLen, headingTop + tickLen * 0.65F);
		display->Line (headingError, headingTop + tickLen * 0.1F,
				headingError - 0.1F * tickLen, headingTop + tickLen * 0.65F);
		display->Line (headingError, headingTop + tickLen * 1.1F,
				headingError + 0.1F * tickLen, headingTop + tickLen * 0.65F);
		display->Line (headingError, headingTop + tickLen * 1.1F,
				headingError - 0.1F * tickLen, headingTop + tickLen * 0.65F);
	}
	else
	{
		//in ILS, we get a "V" as our cue
		if(FCC && FCC->GetMasterMode() == FireControlComputer::ILS)
		{
			display->Line(headingError, headingTop + tickLen * 0.1F,
					headingError + 0.3F * tickLen, headingTop + tickLen * 0.8F);
			display->Line(headingError, headingTop + tickLen * 0.1F,
					headingError - 0.3F * tickLen, headingTop + tickLen * 0.8F);
		}
		else
		{
			//Here we get a inverted arrow  // JPG 17 Dec 03 NOT HERE IN REAL JET YOU DOPE!!
			//   display->Line(headingError, headingTop + tickLen * 0.1F,
			//	    headingError + 0.3F * tickLen, headingTop + tickLen * 0.8F);
			//   display->Line(headingError + 0.25F * tickLen, headingTop + tickLen * 0.8F,
			//	   headingError - 0.3F * tickLen, headingTop + tickLen * 0.8F);
			//   display->Line(headingError, headingTop + tickLen * 0.1F,
			//	    headingError - 0.3F * tickLen, headingTop + tickLen * 0.8F);
		}
	}

	// Draw the waypoint on the ground
	switch (FCC->GetMasterMode())
	{
		case FireControlComputer::Nav:
		case FireControlComputer::AirGroundBomb:
		case FireControlComputer::AirGroundRocket: // MLR 4/3/2004 - 
		case FireControlComputer::AirGroundLaser:
		case FireControlComputer::AirGroundMissile:
		case FireControlComputer::AirGroundHARM:
		case FireControlComputer::AirGroundCamera:
			//Normally we only consider drawing the waypoint if it's less than 90deg 
			//in either direction off the nose. However, the HUD fix for constraining 
			//the waypoint to the HUD's edge requires us to always draw it. Smeghead, 16-Oct-2003
			if ((g_bHUDFix == true) || (fabs(waypointAz) < (90.0F * DTR)) )
			{
				if((g_bHUDFix == true) && (g_bRealisticAvionics))
				{
					//If the waypoint is behind us, then really bad things tend to happen when the
					//waypoint is drawn. Stuff like the waypoint crawling up the HUD when we pitch up
					//(think about it) and so forth. Clamp the position of the drawn waypoint to 
					//+/- 45 deg off the nose to keep things nice and smooth. Easier to do than 
					//pissing around with a bunch of trig to figure out what to do.
					if (waypointAz > (45.0F * DTR))
					{
						yPos = RadToHudUnitsY(45.0F * DTR);
					}
					else if (waypointAz < (-45.0F * DTR))
					{
						yPos = RadToHudUnitsY(-45.0F * DTR);
					}
					else
					{
						yPos = RadToHudUnitsY(waypointEl);
					}
				}
				else
				{
					yPos = RadToHudUnitsY(waypointEl);
				}
				xPos = RadToHudUnitsX(waypointAz);

				display->AdjustOriginInViewport (0.0F, (hudWinY[BORESIGHT_CROSS_WINDOW] +
							hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F));
				//MI draw our WP symbol centered on the WP, not edge-on
#if 0
				display->Line (xPos, yPos + 0.04F, xPos + 0.04F, yPos);
				display->Line (xPos, yPos - 0.04F, xPos + 0.04F, yPos);
				display->Line (xPos, yPos + 0.04F, xPos - 0.04F, yPos);
				display->Line (xPos, yPos - 0.04F, xPos - 0.04F, yPos);
#else
				if(g_bRealisticAvionics)
				{
					//No symbology if in AA MasterMode
					if(FCC->IsAAMasterMode())
					{
						display->AdjustOriginInViewport (0.0F, -(hudWinY[BORESIGHT_CROSS_WINDOW] +
									hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F));
						return;
					}

					//HUD_Fixes.pdf #4 - restrain Steerpoint/target box within bounds of 
					//HUD. Smeg, 16-Oct-2003.
					if(g_bHUDFix == true)
					{
						float boresightOffset = hudWinY[BORESIGHT_CROSS_WINDOW] +
							hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F;
						float horizConstraint = 1.00F - Lenght;
						float vertConstraintBottom = -horizConstraint - boresightOffset;
						float vertConstraintTop = vertConstraintBottom + 1.85F; //HUD is 2 units tall, minus slight fudge.
						bool  stptConstrained = false;

						//NOTE: If Stpt is almost directly behind us, then it's constrained to the HUD, 
						//but it seems to be drawn the wrong way up - as you raise the nose, the stpt on 
						//the hud rises, which makes bugger all sense. This really needs to be looked at...
						if((xPos <= -horizConstraint) || (waypointAz < (-90.0F * DTR)) ) //Left edge
						{
							xPos = -horizConstraint;
							stptConstrained = true;
						}
						else if ((xPos > horizConstraint) || (waypointAz > (90.0F * DTR))) //Right
						{
							xPos = horizConstraint;
							stptConstrained = true;
						}
						if ((yPos <= vertConstraintBottom) || (waypointEl < (-90.0F * DTR))) //Bottom
						{
							yPos = vertConstraintBottom;
							stptConstrained = true;
						}
						else if ((yPos > vertConstraintTop) || (waypointEl > (90.0F * DTR))) //Top
						{
							yPos = vertConstraintTop;
							stptConstrained = true;
						}

						//If Stpt was constrained to HUD, then draw a cross on top of it 
						//to warn that it's unreliable.
						if(stptConstrained == true)
						{
							float crossSize = Lenght; 
							display->Line (-crossSize + xPos, -crossSize + yPos,
									crossSize + xPos, crossSize + yPos);
							display->Line (-crossSize + xPos, crossSize + yPos, 
									crossSize + xPos, -crossSize + yPos);
						}
					} //End of HUD fix.

					if(playerAC && playerAC->curWaypoint &&
							playerAC->curWaypoint->GetWPFlags() & WPF_TARGET &&
							FCC && FCC->IsAGMasterMode() &&
							FCC->GetSubMode() != FireControlComputer::CCIP)
					{
						//This is our target, so the WP is a square, but not in CCIP, and only in AG 
						//mode.
						display->Line(xPos - Lenght, yPos - (Lenght), xPos - (Lenght), yPos + (Lenght));
						display->Line(xPos - Lenght, yPos + (Lenght), xPos + (Lenght), yPos + (Lenght));
						display->Line(xPos + Lenght, yPos + (Lenght), xPos + (Lenght), yPos - (Lenght));
						display->Line(xPos + Lenght, yPos - (Lenght), xPos - (Lenght), yPos - (Lenght));
						display->Point(xPos, yPos);
					}
					else
					{
						display->Line(xPos - Lenght, yPos, xPos, yPos + Lenght);
						display->Line(xPos, yPos + Lenght, xPos + Lenght, yPos);
						display->Line(xPos + Lenght, yPos, xPos, yPos - Lenght);
						display->Line(xPos, yPos - Lenght, xPos - Lenght, yPos);
					}

				}
				else
				{
					display->Line(xPos - Lenght, yPos, xPos, yPos + Lenght);
					display->Line(xPos, yPos + Lenght, xPos + Lenght, yPos);
					display->Line(xPos + Lenght, yPos, xPos, yPos - Lenght);
					display->Line(xPos, yPos - Lenght, xPos - Lenght, yPos);
				}
#endif
				display->AdjustOriginInViewport (0.0F, -(hudWinY[BORESIGHT_CROSS_WINDOW] +
							hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F));
			}
			break;

		default:
			break;
	}
}

void HudClass::DrawNav (void)
{
	//char tmpStr[24];

	DrawTDBox();

	/*  now drawn in rangetostearpoint me123 if (targetPtr)
	    {
	    if (targetPtr->localData->range > 1.0F * NM_TO_FT)
	    sprintf (tmpStr, "F %4.1f", max ( min (100.0F, targetPtr->localData->range * FT_TO_NM), 0.0F));
	    else
	    sprintf (tmpStr, "F %03.0f", max ( min (10000.0F, targetPtr->localData->range * 0.01F), 0.0F));
	    ShiAssert(strlen(tmpStr) < sizeof(tmpStr));
	    DrawWindowString (10, tmpStr);
	    }*/

	if (waypointValid)
	{
		TimeToSteerpoint();
		RangeToSteerpoint();
	}
}

void HudClass::TimeToSteerpoint(void)
{
	//MI INS stuff
	if(g_bRealisticAvionics && g_bINS)
	{
		if(ownship && !ownship->INSState(AircraftClass::INS_HUD_STUFF))
			return;
	}
	char tmpStr[24];
	char tmpStr1[24];
	int hr, minute, sec;
	float ttg;

	// Time to waypoint
	if (waypointArrival > 0.0F)
	{
		ttg = waypointArrival;
		// burn any days in the number
		hr = FloatToInt32(ttg / (3600.0F * 24.0F));
		ttg -= hr * 3600.0F * 24.0F;
		hr  = FloatToInt32(ttg / 3600.0F);
		hr  = max (hr, 0);
		ttg -= hr * 3600.0F;
		ttg = max (ttg, 0.0F);
		minute = FloatToInt32(ttg / 60.0F);
		ttg -= minute * 60.0F;
		sec = FloatToInt32(ttg);
		if (hr != 0)
			sprintf (tmpStr, "%03d:%02d", abs(minute), sec);  //JPG 5 Feb 04
		else if (sec >= 0)
		{
			if(!g_bRealisticAvionics)
				sprintf (tmpStr, "   %02d:%02d", abs(minute), sec);
			else
				sprintf (tmpStr, "%03d:%02d", abs(minute), sec);  //JPG "%02d:%02d"
		}
		else
		{
			if(!g_bRealisticAvionics)
				sprintf (tmpStr, "  -%02d:%02d", abs(minute), abs(sec));
			else
				sprintf (tmpStr, "-%02d:%02d", abs(minute), abs(sec));
		}
	}
	else
	{
		strcpy (tmpStr, "XX:XX");
	}

	//MI
	if(!g_bRealisticAvionics)
	{
		switch (FCC->GetSubMode())
		{
			case FireControlComputer::TimeToGo:         
				sprintf (tmpStr1, ">%s<", tmpStr);
				break;

			case FireControlComputer::ETA:
				sprintf (tmpStr1, "<%s>", tmpStr);
				break;

			default:
				strcpy (tmpStr1, tmpStr);
				break;
		}
	}
	else									// JPG
	{

		sprintf(tmpStr1, "%s", tmpStr);
	}
	ShiAssert (strlen(tmpStr1) < sizeof(tmpStr1));
	if(!g_bRealisticAvionics)
		DrawWindowString (13, tmpStr1);
	else
		display->TextLeft(0.45F, -0.43F, tmpStr1 );  // JPG .40F & .44F
}

void HudClass::RangeToSteerpoint(void)
{
	//MI INS stuff
	if(g_bRealisticAvionics && g_bINS)
	{
		if(ownship && !ownship->INSState(AircraftClass::INS_HUD_STUFF))
			return;
	}
	char tmpStr[24];
	char str[24];
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

	// MD -- 20040220: adding override for GM SP ground stabilization
	if ((FCC->GetStptMode() == FireControlComputer::FCCGMPseudoPoint) && playerAC)
	{
		float wpX=0.0f, wpY=0.0f, wpZ=0.0f, dx=0.0f, dy=0.0f, dz=0.0f;
		FCC->SavedWaypoint()->GetLocation (&wpX, &wpY, &wpZ);
		// add in INS Drift
		if(g_bINS && g_bRealisticAvionics)
		{
			if(playerAC)
			{
				wpX += playerAC->GetINSLatDrift();
				wpY += playerAC->GetINSLongDrift();
			}
		}

		dx = wpX - playerAC->XPos();
		dy = wpY - playerAC->YPos();
		dz = OTWDriver.GetApproxGroundLevel(wpX, wpY) - playerAC->ZPos();
		// add in INS Offset
		if(g_bINS && g_bRealisticAvionics)
		{
			if(playerAC)
				dz -= playerAC->GetINSAltOffset();
		}

		sprintf (tmpStr, "%03.0f>%02d", ((float)sqrt(dx*dx + dy*dy)) * FT_TO_NM, waypointNum + 1);
	}
	else
		// Range and Number
		//MI      JPG 1 Feb 04  - Why would you do this??  ugh
		// if(!g_bRealisticAvionics)
		//	sprintf (tmpStr, "%03.0f > %02d", waypointRange * FT_TO_NM, waypointNum + 1);
		// else
		sprintf (tmpStr, "%03.0f>%02d", waypointRange * FT_TO_NM, waypointNum + 1);

	ShiAssert (strlen(tmpStr) < sizeof(tmpStr));
	if(!g_bRealisticAvionics)
		DrawWindowString (14, tmpStr);
	else
		display->TextLeft(0.45F, -0.50F, tmpStr);  //JPG .52F

	//MI Slant range
	float xPos = 0.45F;   //JPG .40F
	float yPos = -0.39F;
	float range = SlantRange * FT_TO_NM;

	// RV - I-Hawk - Do not display target range if in HARM HAS mode
	bool displayRange = true;
	if ( FCC->GetSubMode() == FireControlComputer::HARM )
	{
		HarmTargetingPod* harmPod = (HarmTargetingPod*)FindSensor(ownship, SensorClass::HTS);
		if ( harmPod && harmPod->GetSubMode() == HarmTargetingPod::HAS ||
			 harmPod && harmPod->GetSubMode() == HarmTargetingPod::Handoff ||
			 harmPod && harmPod->GetSubMode() == HarmTargetingPod::FilterMode )
		{
			displayRange = false;
		}
	}

	if (targetPtr) 
		range = targetData->range * FT_TO_NM;

	if ( displayRange == true )
	{
		if(range >= 100)
		{
			if(!targetPtr)
			{
				sprintf(str, "B%3.1f", range);
			}
			else
				sprintf(str, "F%3.1f", range);		
		}
		else if(range < 100 && range >= 10)
		{
			if (!targetPtr)
			{
				sprintf(str, "B0%2.1f", range);
			}
			else
				sprintf(str, "F0%2.1f", range);
		}
		else if(range < 10 && range >= 1)
		{
			if(!targetPtr) 
			{
				sprintf(str, "B00%1.1f", range);
			}
			else
				sprintf(str, "F00%1.1f", range);
		}
		else
		{
			range *= NM_TO_FT;
			range /= 100;
			if(!targetPtr) 
				sprintf(str, "B 0%2.0f", range);
			else
				sprintf(str, "F 0%2.0f", range);
		}
	}

	ShiAssert (strlen(tmpStr) < sizeof(tmpStr));
	if(!g_bRealisticAvionics)
	{
		if (targetPtr)
		{
			if (targetPtr->localData->range > 1.0F * NM_TO_FT)
				sprintf (tmpStr, "F %4.1f", max ( min (100.0F, targetPtr->localData->range * FT_TO_NM), 0.0F));
			else
				sprintf (tmpStr, "F %03.0f", max ( min (10000.0F, targetPtr->localData->range * 0.01F), 0.0F));
			ShiAssert(strlen(tmpStr) < sizeof(tmpStr));
			DrawWindowString (10, tmpStr);
		}
	}
	else if ( displayRange )
	{
		display->TextLeft(xPos, yPos + 0.03F, str);

	}
}

//MI functions that find our angles
int HudClass::FindRollAngle(float Alt)
{
	if (!ownship){
		// JB 010528
		return FALSE; // JB 010528
	}

	/*Roll is from 0 to 180 and -180 to 0 clockwise
	  Alt is hight above terrain*/
	if (Alt <= 3000){
		//Below 3000ft we always have 60°
		if (((AircraftClass*)ownship)->af->platform->IsPlayer()){
			if((cockpitFlightData.roll * RTD) <= 60 && cockpitFlightData.roll * RTD >= -60){
				return TRUE;
			}
			else {
				return FALSE;
			}
		}
	}
	else if(Alt > 3000 && Alt <= 5000){
		if(((AircraftClass*)ownship)->af->platform->IsPlayer()){
			float factor = 2000 / 30; //30° less bank in 2000ft
			float Roll = (Alt - 3000) / factor;
			float Angle = 60 - Roll;
			if (cockpitFlightData.roll * RTD <= Angle && cockpitFlightData.roll * RTD >= -Angle){
				return TRUE;
			}
			else {
				return FALSE;
			}
		}
	}
	else if(Alt > 5000 && Alt <= 10000)
	{
		if(((AircraftClass*)ownship)->af->platform->IsPlayer())
		{
			float factor = 5000 / 5; //5° less bank in 5000ft
			float Roll = (Alt - 5000) / factor;
			float Angle = 30 - Roll;
			if(cockpitFlightData.roll * RTD <= Angle &&
					cockpitFlightData.roll * RTD >= -Angle)
			{
				return TRUE;
			}
			else
				return FALSE;
		}
	}
	else if(Alt > 10000 && Alt <= 25000)
	{
		if(((AircraftClass*)ownship)->af->platform->IsPlayer())
		{
			float factor = 15000 / 10; //10° less bank in 15000ft
			float Roll = (Alt - 15000) / factor;
			float Angle = 25 - Roll;
			if(cockpitFlightData.roll * RTD <= Angle &&
					cockpitFlightData.roll * RTD >= -Angle)
			{
				return TRUE;
			}
			else
				return FALSE;
		}
	}
	else if(Alt > 25000 && Alt <= 50000)
	{
		if(((AircraftClass*)ownship)->af->platform->IsPlayer())
		{
			float factor = 25000 / 5; //5° less bank in 25000ft
			float Roll = (Alt - 25000) / factor;
			float Angle = 15 - Roll;
			if(cockpitFlightData.roll * RTD <= Angle &&
					cockpitFlightData.roll * RTD >= -Angle)
			{
				return TRUE;
			}
			else
				return FALSE;
		}
	}
	else
		return FALSE;

	return FALSE;
}

int HudClass::FindPitchAngle(float Alt)
{
	if(!ownship)
		return FALSE;

	if(Alt <= 5000)
	{
		//Below 5000ft we always have 30°
		if(((AircraftClass*)ownship)->af->platform->IsPlayer())
		{
			if((cockpitFlightData.pitch * RTD) <= 30 &&
					cockpitFlightData.pitch * RTD >= -30)
			{
				return TRUE;
			}
			else
				return FALSE;
		}
	}
	else if(Alt > 5000 && Alt <= 10000)
	{
		if(((AircraftClass*)ownship)->af->platform->IsPlayer())
		{
			float factor = 5000 / 5; //5° less bank in 5000ft
			float Pitch = (Alt - 5000) / factor;
			float Angle = 30 - Pitch;
			if(cockpitFlightData.pitch * RTD <= Angle &&
					cockpitFlightData.pitch * RTD >= -Angle)
			{
				return TRUE;
			}
			else
				return FALSE;
		}
	}
	else if(Alt > 10000 && Alt <= 25000)
	{
		if(((AircraftClass*)ownship)->af->platform->IsPlayer())
		{
			float factor = 15000 / 10; //10° less bank in 15000ft
			float Pitch = (Alt - 15000) / factor;
			float Angle = 25 - Pitch;
			if(cockpitFlightData.pitch * RTD <= Angle &&
					cockpitFlightData.pitch * RTD >= -Angle)
			{
				return TRUE;
			}
			else
				return FALSE;
		}
	}
	else if(Alt > 25000 && Alt <= 50000)
	{
		if(((AircraftClass*)ownship)->af->platform->IsPlayer())
		{
			float factor = 25000 / 5; //5° less bank in 25000ft
			float Pitch = (Alt - 25000) / factor;
			float Angle = 15 - Pitch;
			if(cockpitFlightData.pitch * RTD <= Angle &&
					cockpitFlightData.pitch * RTD >= -Angle)
			{
				return TRUE;
			}
			else
				return FALSE;
		}
	}
	else
		return FALSE;

	return FALSE;
}

void HudClass::DrawRollCue()
{
	static const float basey = hudWinY[HEADING_WINDOW_LO] +
		hudWinHeight[HEADING_WINDOW_LO] * 0.5F - 0.075f;
	//static bool initonce = false;
	static const float MAXROLL = 45 * DTR;
	static const float angles[] = { MAXROLL, 30 *DTR, 20 * DTR, 10 * DTR};
	static const int nangles = sizeof(angles)/sizeof(angles[0]);
	static float x1points[nangles], x2points[nangles], y1points[nangles], y2points[nangles];
	static const float sdist = 0.35f, oedist = 0.40f;

	//if (initonce == false) { // JPO precalculate drawing points
	if(CalcRoll){
		float edist = oedist;
		mlTrig trig;
		for (int i = 0; i  < nangles; i++) {
			if (i > 1) edist = 0.38f;
			mlSinCos( &trig, angles[i]);
			x1points[i] = sdist*trig.sin;
			y1points[i] = -sdist*trig.cos;
			x2points[i] = edist*trig.sin;
			y2points[i] = -edist*trig.cos;
		}
		//initonce = true;
		CalcRoll = FALSE;
	}

	display->AdjustOriginInViewport( 0,  basey);

	for (int i = 0; i  < nangles; i++) {
		display->Line(x1points[i], y1points[i], x2points[i], y2points[i]);
		display->Line(-x1points[i], y1points[i], -x2points[i], y2points[i]);
#if 0
		mlSinCos( &trig, angles[i]);
		display->Line ( sdist*trig.sin, -sdist*trig.cos,  edist*trig.sin, -edist*trig.cos);
		display->Line (-sdist*trig.sin, -sdist*trig.cos, -edist*trig.sin, -edist*trig.cos);
		if (i > 1) edist = 0.38f;
#endif
	}
	display->Line(0, -sdist, 0, -oedist); // final cue line
	float roll = -cockpitFlightData.roll;
	// Only draw the roll if within limits.
	if (roll <= MAXROLL && roll >= -MAXROLL) {
		display->AdjustRotationAboutOrigin(roll); // do the roll offset
		// draw empty triange there.
		//MI INS stuff
		if(g_bRealisticAvionics && g_bINS)
		{
			if(ownship && !ownship->INSState(AircraftClass::INS_HUD_STUFF))
			{
				display->ZeroRotationAboutOrigin();
				display->AdjustOriginInViewport( 0,  -basey);
				return;
			}
		}
		display->Line(0, -oedist, -0.025f, -oedist-0.05f);
		display->Line(-0.025f, -oedist-0.05f, 0.025f, -oedist-0.05f);
		display->Line(0.025f, -oedist-0.05f, 0, -oedist);
		display->ZeroRotationAboutOrigin();
	}
	display->AdjustOriginInViewport( 0,  -basey);
}
void HudClass::CheckMSLFloor(void)
{
	//No Message if gear down
	if(((AircraftClass*)ownship)->af->gearPos > 0.5F)
		return;
	//Above our setting and descending
	if(-cockpitFlightData.z <= MSLFloor + 20.0F &&
			-cockpitFlightData.zDot < 0)
	{
		if(WasAboveMSLFloor)
		{
			for(int i = 0; i < 2; i++)
				F4SoundFXSetDist( ownship->af->GetAltitudeSnd(), FALSE, 0.0f, 1.0f );

			WasAboveMSLFloor = FALSE;
		}
	}

	if(-cockpitFlightData.z > (MSLFloor + 20.0F) && !WasAboveMSLFloor)
		WasAboveMSLFloor = TRUE;
}
const static float TrigWidth = 0.04F;
const static float TrigHeight = 0.075F;
void HudClass::DrawOA(void)
{
	//MI INS stuff
	if(g_bRealisticAvionics && g_bINS)
	{
		if(ownship && !ownship->INSState(AircraftClass::INS_HUD_STUFF))
			return;
	}
	float xPos,yPos;
	if ((!OA1Valid && !OA2Valid) || (FCC->GetMasterMode() == FireControlComputer::Dogfight))//me123 status test.)
		return;
	else
	{
		// Draw the waypoint on the ground
		switch (FCC->GetMasterMode())
		{
			case FireControlComputer::Nav:
			case FireControlComputer::AirGroundBomb:
			case FireControlComputer::AirGroundRocket: // MLR 4/3/2004 - 
			case FireControlComputer::AirGroundLaser:
			case FireControlComputer::AirGroundMissile:
			case FireControlComputer::AirGroundHARM:
			case FireControlComputer::AirGroundCamera:
				if (fabs(OA1Az) < (90.0F * DTR))
				{
					xPos = RadToHudUnitsX(OA1Az);
					yPos = RadToHudUnitsY(OA1Elev);
					display->AdjustOriginInViewport (0.0F, (hudWinY[BORESIGHT_CROSS_WINDOW] +
								hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F));
					display->Line(xPos - TrigWidth/2, yPos - TrigHeight/2, xPos + TrigWidth/2, yPos - TrigHeight/2);
					display->Line(xPos + TrigWidth/2, yPos - TrigHeight/2, xPos, yPos + TrigHeight/2);
					display->Line(xPos, yPos + TrigHeight/2, xPos - TrigWidth/2, yPos - TrigHeight/2);
					display->AdjustOriginInViewport (0.0F, -(hudWinY[BORESIGHT_CROSS_WINDOW] +
								hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F));
				}
				break;
			default:
				break;
		}
	}
	if(!OA2Valid || (FCC->GetMasterMode() == FireControlComputer::Dogfight))//me123 status test.)
		return;
	else
	{
		// Draw the waypoint on the ground
		switch (FCC->GetMasterMode())
		{
			case FireControlComputer::Nav:
			case FireControlComputer::AirGroundBomb:
			case FireControlComputer::AirGroundRocket: // MLR 4/3/2004 - 
			case FireControlComputer::AirGroundLaser:
			case FireControlComputer::AirGroundMissile:
			case FireControlComputer::AirGroundHARM:
			case FireControlComputer::AirGroundCamera:
				if (fabs(OA2Az) < (90.0F * DTR))
				{
					xPos = RadToHudUnitsX(OA2Az);
					yPos = RadToHudUnitsY(OA2Elev);
					display->AdjustOriginInViewport (0.0F, (hudWinY[BORESIGHT_CROSS_WINDOW] +
								hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F));
					display->Line(xPos - TrigWidth/2, yPos - TrigHeight/2, xPos + TrigWidth/2, yPos - TrigHeight/2);
					display->Line(xPos + TrigWidth/2, yPos - TrigHeight/2, xPos, yPos + TrigHeight/2);
					display->Line(xPos, yPos + TrigHeight/2, xPos - TrigWidth/2, yPos - TrigHeight/2);
					display->AdjustOriginInViewport (0.0F, -(hudWinY[BORESIGHT_CROSS_WINDOW] +
								hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F));
				}
				break;
				break;
			default:
				break;
		}
	}
}
const static float Diam = 0.03F;
void HudClass::DrawVIP(void)
{
	//MI INS stuff
	if(g_bRealisticAvionics && g_bINS)
	{
		if(ownship && !ownship->INSState(AircraftClass::INS_HUD_STUFF))
			return;
	}
	float xPos,yPos;
	if (!VIPValid || (FCC->GetMasterMode() == FireControlComputer::Dogfight))//me123 status test.)
		return;
	else
	{
		// Draw the waypoint on the ground
		switch (FCC->GetMasterMode())
		{
			case FireControlComputer::Nav:
			case FireControlComputer::AirGroundBomb:
			case FireControlComputer::AirGroundLaser:
			case FireControlComputer::AirGroundMissile:
			case FireControlComputer::AirGroundHARM:
			case FireControlComputer::AirGroundCamera:
				if (fabs(VIPAz) < (90.0F * DTR))
				{
					xPos = RadToHudUnitsX(VIPAz);
					yPos = RadToHudUnitsY(VIPElev);
					display->AdjustOriginInViewport (0.0F, (hudWinY[BORESIGHT_CROSS_WINDOW] +
								hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F));
					display->Line(xPos - Lenght, yPos, xPos, yPos + Lenght);
					display->Line(xPos, yPos + Lenght, xPos + Lenght, yPos);
					display->Line(xPos + Lenght, yPos, xPos, yPos - Lenght);
					display->Line(xPos, yPos - Lenght, xPos - Lenght, yPos);
					// display->Circle(xPos - Diam/2, yPos, Diam);  // JPG 17 Dec 03 No no no, not a circle, draw a diamond
					display->AdjustOriginInViewport (0.0F, -(hudWinY[BORESIGHT_CROSS_WINDOW] +
								hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F));
				}
				break;
			default:
				break;
		}
	}
}
void HudClass::DrawVRP(void)
{
	//MI INS stuff
	if(g_bRealisticAvionics && g_bINS)
	{
		if(ownship && !ownship->INSState(AircraftClass::INS_HUD_STUFF))
			return;
	}
	float xPos,yPos;
	if (!VRPValid || (FCC->GetMasterMode() == FireControlComputer::Dogfight))//me123 status test.)
		return;
	else
	{
		// Draw the waypoint on the ground
		switch (FCC->GetMasterMode())
		{
			case FireControlComputer::Nav:
			case FireControlComputer::AirGroundBomb:
			case FireControlComputer::AirGroundRocket: // MLR 4/3/2004 - 
			case FireControlComputer::AirGroundLaser:
			case FireControlComputer::AirGroundMissile:
			case FireControlComputer::AirGroundHARM:
			case FireControlComputer::AirGroundCamera:
				if (fabs(VRPAz) < (90.0F * DTR))
				{
					xPos = RadToHudUnitsX(VRPAz);
					yPos = RadToHudUnitsY(VRPElev);
					display->AdjustOriginInViewport (0.0F, (hudWinY[BORESIGHT_CROSS_WINDOW] +
								hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F));

					display->Line(xPos - Lenght, yPos, xPos, yPos + Lenght);
					display->Line(xPos, yPos + Lenght, xPos + Lenght, yPos);
					display->Line(xPos + Lenght, yPos, xPos, yPos - Lenght);
					display->Line(xPos, yPos - Lenght, xPos - Lenght, yPos);

					// display->Circle(xPos - Diam/2, yPos, Diam);  // JPG 17 Dec 03 - A circle is for the PUP, not this crap

					display->AdjustOriginInViewport (0.0F, -(hudWinY[BORESIGHT_CROSS_WINDOW] +
								hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F));
				}
				break;
			default:
				break;
		}
	}
}
void HudClass::DrawRALT(void)
{
	if(!ownship)
		return;

	if(FCC->GetMasterMode() == FireControlComputer::Dogfight || FCC->GetMasterMode() == FireControlComputer::MissileOverride)
		return;

	//NO RALT if RF Switch in SILENT
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if(playerAC && playerAC->RFState == 2)
		return;

	if(-hat < lowAltWarning && (FindRollAngle(-hat) && FindPitchAngle(-hat)) &&
			((AircraftClass*)ownship)->af->platform->RaltReady())
	{
		if(!((AircraftClass*)ownship)->OnGround())
		{
			if(((AircraftClass*)ownship)->af->platform->RALTStatus == AircraftClass::RaltStatus::ROFF ||
					((AircraftClass*)ownship)->af->platform->RALTStatus == AircraftClass::RaltStatus::RSTANDBY)
				DrawALString();
			else if(Warnflash)
				DrawALString();
		}
		else if(((AircraftClass*)ownship)->OnGround())
		{
			//if(((AircraftClass*)ownship)->af->platform->RALTStatus != AircraftClass::RaltStatus::ROFF)
			DrawALString();
		}
		if(((AircraftClass*)ownship)->af->gearPos < 0.8F)
			F4SoundFXSetDist(ownship->af->GetAltitudeSnd(), FALSE, 0.0f, 1.0f );
	}
	else
	{
		//if(((AircraftClass*)ownship)->af->platform->RALTStatus != AircraftClass::RaltStatus::ROFF)
		DrawALString();
	}
	if(((AircraftClass*)ownship)->af->platform->RaltReady())
	{ 
		if(FindRollAngle(-hat) && FindPitchAngle(-hat))
		{
			//Cobra
			if ((unsigned long)hudRAltDelayTimer < SimLibElapsedTime)
			{
				raltHud=hat;
				hudRAltDelayTimer = SimLibElapsedTime + 250;
			}

			//only show 10's of feet
			hat = ((static_cast<int>(raltHud) + 5) / 10) * 10.0f;
			sprintf (tmpStr, "%.0f", -hat);  //JPG "%.0f"
			display->TextRight(0.74F, YRALTText, tmpStr);
		}
	}
}

void HudClass::DrawRALTBox(void)
{
	if(FCC->GetMasterMode() == FireControlComputer::Dogfight || FCC->GetMasterMode() == FireControlComputer::MissileOverride)
		return;

	if(OTWDriver.GetOTWDisplayMode() == OTWDriverClass::ModeHud)
	{
		//window flashes if RALT fault
		if(ownship && ownship->mFaults && ownship->mFaults->GetFault(FaultClass::ralt_fault))
		{
			if(Warnflash)
			{
				display->Line(0.605F,-0.16F,0.74F,-0.16F);
				display->Line(0.74F,-0.16F,0.74F,-0.22F);
				display->Line(0.74F,-0.22F,0.605F,-0.22F);
				display->Line(0.605F,-0.22F,0.605F,-0.16F);
			}
		}
		else
		{
			display->Line(0.605F,-0.16F,0.74F,-0.16F);
			display->Line(0.74F,-0.16F,0.74F,-0.22F);
			display->Line(0.74F,-0.22F,0.605F,-0.22F);
			display->Line(0.605F,-0.22F,0.605F,-0.16F);
		}

		if(((AircraftClass*)ownship)->af->platform->RaltReady())
		{
			if(radarSwitch == RADAR_AUTO)
				display->TextLeft(0.550F,YRALTText,"AR");
			else if(radarSwitch == BARO)
				display->TextLeft(0.550F,YRALTText,"R");
		}
	} 
	else
	{
		if(ownship && ownship->mFaults && ownship->mFaults->GetFault(FaultClass::ralt_fault))
		{
			if(Warnflash)
			{
				display->Line(0.50F,-0.17F,0.75F,-0.17F);
				display->Line(0.75F,-0.17F,0.75F,-0.25F);
				display->Line(0.75F,-0.25F,0.50F,-0.25F);
				display->Line(0.50F,-0.25F,0.50F,-0.17F);
			}
		}
		else
		{
			display->Line(0.50F,-0.17F,0.75F,-0.17F);
			display->Line(0.75F,-0.17F,0.75F,-0.25F);
			display->Line(0.75F,-0.25F,0.50F,-0.25F);
			display->Line(0.50F,-0.25F,0.50F,-0.17F);
		}
		if(((AircraftClass*)ownship)->af->platform->RaltReady())
		{ 
			if(radarSwitch == RADAR_AUTO)
				display->TextLeft(0.42F,YRALTText,"AR");
			else if(radarSwitch == BARO)
				display->TextLeft(0.45F,YRALTText,"R");	
		}
	}	   
}
void HudClass::DrawALString(void)
{
	if(FCC->GetMasterMode() == FireControlComputer::Dogfight || FCC->GetMasterMode() == FireControlComputer::MissileOverride)
		return;

	//here in any case, according to the list member
	if(OTWDriver.GetOTWDisplayMode() == OTWDriverClass::ModeHud)
		display->TextLeft(0.550F,YALText,"AL");
	else
		display->TextLeft(0.45F,YALText,"AL");

	if(((AircraftClass*)ownship)->af->platform->RALTStatus == AircraftClass::RaltStatus::ROFF)
		return;

	if(OTWDriver.GetOTWDisplayMode() == OTWDriverClass::ModeHud)
	{ 
		//display->TextLeft(0.550F,YALText,"AL");
		sprintf(tmpStr, "%.0f", lowAltWarning);
		display->TextRight(0.74F,YALText,tmpStr);
	}
	else
	{
		//display->TextLeft(0.40F,YALText,"AL");
		sprintf(tmpStr, "%.0f", lowAltWarning);
		display->TextRight(0.74F,YALText,tmpStr);
	}
}
//MI
void HudClass::DrawCMDSTRG(void)
{
	//get where our FPM is located
	float dx = betaHudUnits;
	float dy = hudWinY[BORESIGHT_CROSS_WINDOW] +
		hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F -
		alphaHudUnits;

	float LocalizerDev = 0.0F, RWYHeading = 0.0F, GlideSlopeDev = 0.0F, DistToSta = 0.0F;
	float x1 = 0.0F;
	float y1 = 0.0F;
	float Intercept = 0.0F, Slope = 0.0F, Descent = 0.0F;
	float Bearing = 0.0F, Bearing1 = 0.0F, Bearing2 = 0.0F;

	if(gNavigationSys)
		gNavigationSys->GetILSData(&LocalizerDev, &RWYHeading, &GlideSlopeDev, &DistToSta);

	// MD -- 20040605: no signals from the ILS system is you are beyond ~25nm.
	// or approximately (25 * 6000) feet
	if (DistToSta > 150000.0F)
		return;

	LocalizerDev *= RTD;
	GlideSlopeDev *= RTD;

	//up to 5° we're on a correction
	float CorrectionLimit = 5.0F;

	//set correction sensitivity
	float hDeviationGain = 45.0F / CorrectionLimit; // this way there is no jump  when switching from intercept
	float vDeviationGain = 3.0F; // must be greater than 1, how soon you want to align with ILS
	float GlideSlopeAngle = -3.0F;

	mlTrig trig;
	mlSinCos (&trig, cockpitFlightData.roll);

	//Calculate our current heading from FPM position;
	float hor_Offset = cockpitFlightData.beta * trig.cos + cockpitFlightData.alpha * trig.sin
		+ RTD * cockpitFlightData.windOffset * trig.cos;
	float curHeading = cockpitFlightData.yaw * RTD + hor_Offset;

	//Calcualte correction phase Bearing1
	Bearing1 = RWYHeading - curHeading + (LocalizerDev * hDeviationGain);
	while ((Bearing1 < -180) || (Bearing1 > 180)) 
	{
		if(Bearing1 > 180)
			Bearing1 -= 360;
		else if(Bearing1 < -180)
			Bearing1 += 360;
	}

	//Calculate correction phase descent Slope
	Descent = cockpitFlightData.gamma * RTD;
	Slope = (GlideSlopeAngle - Descent) + GlideSlopeDev * vDeviationGain;
	while ((Slope < -180) || (Slope > 180)) 
	{
		if(Slope > 180)
			Slope -= 360;
		else if(Slope < -180)
			Slope += 360;
	}


	//Calculate 45° intercept Bearing2
	float headingDiff = RWYHeading - curHeading;
	//left or right turn?
	if(LocalizerDev < 0)
		Intercept = RWYHeading - 45;    //right of RWY
	else
		Intercept = RWYHeading + 45;    //left or RWY

	Bearing2 = Intercept - curHeading;
	if(Bearing2 > 180)
		Bearing2 -= 360;
	else if(Bearing2 < -180)
		Bearing2 += 360;

	//When to apply which bearing and slope...
	if(LocalizerDev < CorrectionLimit && LocalizerDev > -CorrectionLimit)
	{
		Bearing = Bearing1;

		//Smooth slope movement
		float tmpSmooth = 0.9F;
		if ((LocalizerDev > tmpSmooth * CorrectionLimit) || (LocalizerDev < -tmpSmooth * CorrectionLimit))
		{
			if (LocalizerDev > 0)
				Slope = Slope * ((CorrectionLimit - LocalizerDev) / ((1-tmpSmooth) * CorrectionLimit));
			else
				Slope = Slope * ((CorrectionLimit + LocalizerDev) / ((1-tmpSmooth) * CorrectionLimit));
		}
	}
	else
	{
		Bearing = Bearing2;
		Slope = 0;
	}
	Bearing *= DTR;
	Slope *= DTR;

	display->AdjustOriginInViewport (dx, dy);
	display->AdjustRotationAboutOrigin(-cockpitFlightData.roll);
	x1 = max(min(Bearing, hudWinWidth[PITCH_LADDER_WINDOW] * 0.75F),-hudWinWidth[PITCH_LADDER_WINDOW] * 0.75F);
	y1 = max(min(Slope, hudWinWidth[PITCH_LADDER_WINDOW] * 0.75F), -hudWinWidth[PITCH_LADDER_WINDOW] * 0.75F);

	float circlesize = 0.003F;
	float linesize = 0.016F;
	//float circlesize = 0.006F;
	//float linesize = 0.012F;
	float ratio = circlesize / linesize;
	display->Circle (x1, y1, RadToHudUnits(circlesize));
	float len = RadToHudUnits(linesize);
	mlSinCos (&trig, cockpitFlightData.roll);
	float y = len * trig.cos;
	float x = len * trig.sin;
	display->Line (x1 + x, y1 + y, x1 + x * ratio, y1 + y * ratio);
	display->ZeroRotationAboutOrigin();
	display->AdjustOriginInViewport (-dx, -dy);
}
//MI
void HudClass::DrawBankIndicator(void)
{
	//get where our FPM is located
	float dx = betaHudUnits;
	float dy = hudWinY[BORESIGHT_CROSS_WINDOW] +
		hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F -
		alphaHudUnits;

	static const float angles[] = { 60 * DTR, 30 * DTR, 20 * DTR, 10 * DTR, 0 * DTR, -10 * DTR, -20 * DTR, -30 * DTR, -60 * DTR };
	static const int nangles = sizeof(angles)/sizeof(angles[0]);
	static float x1points[nangles], x2points[nangles], y1points[nangles], y2points[nangles];
	static float sdist = 0.11f, oedist = 0.13f;
	mlTrig trig;

	display->AdjustOriginInViewport(dx, dy); 
	if(CalcBank)
	{
		for (int i = 0; i  < nangles; i++) 
		{
			if (i ==0 || i == 1 || i == 4 || i == 7 || i == 8) 
				oedist = 0.16f;
			else
				oedist = 0.13f;

			mlSinCos( &trig, angles[i]);
			x1points[i] = sdist*trig.sin;
			y1points[i] = sdist*trig.cos;
			x2points[i] = oedist*trig.sin;
			y2points[i] = oedist*trig.cos;
		}
		CalcBank = FALSE;
	}
	display->AdjustRotationAboutOrigin(-cockpitFlightData.roll);
	for(int i = 0; i < nangles; i++)
	{
		display->Line(x1points[i], y1points[i], x2points[i], y2points[i]);
		display->Line(-x1points[i], y1points[i], -x2points[i], y2points[i]);
	}
	display->ZeroRotationAboutOrigin();
	display->AdjustOriginInViewport(-dx, -dy);
}
void HudClass::DrawAirSpeedCarret(float Speed)
{
	if(FCC->GetMasterMode() == FireControlComputer::Dogfight){
		return;
	}

	//TJL 07/31/04 F14 - F18 Airspeed Carret //Cobra 11/04/04 TJL
	//Cobra fixed non-F16, non-HUD specific airspeed carret issue
	if (ownship->af->GetTypeAC() == 6 || ownship->af->GetTypeAC() == 7 || ownship->af->GetTypeAC() == 8 ||
			ownship->af->GetTypeAC() == 9 || ownship->af->GetTypeAC() == 10)
	{
		float delta = 0.0f;
		delta = Speed - (cockpitFlightData.kias);
		display->Line (-0.90F, 0.1F, -0.90F, 0.02F);//Vertical Line
		if (delta > 5.0f)
		{
			//Arrow Left
			display->Line (-0.95F, 0.1F, -0.97F, 0.05F);
			display->Line (-0.95F, 0.1F, -0.93F, 0.05F);
		}
		else if (delta < -5.0f)
		{
			//Arrow Right
			display->Line (-0.85F, 0.1F, -0.87F, 0.05F);
			display->Line (-0.85F, 0.1F, -0.83F, 0.05F);
		}
		else
		{	
			//Arrow Center
			display->Line (-0.90F, 0.1F, -0.92F, 0.05F);
			display->Line (-0.90F, 0.1F, -0.88F, 0.05F);
		}
		return;
	}
	//F15s
	else if (ownship->af->GetTypeAC() == 3 || ownship->af->GetTypeAC() == 4 || ownship->af->GetTypeAC() == 5)
	{
		float delta = 0.0f;
		delta = Speed - (cockpitFlightData.kias);
		display->Line (-0.90F, 0.25F, -0.90F, 0.32F);//Vertical Line
		if (delta > 5.0f)
		{
			//Arrow Left
			display->Line (-0.95F, 0.25F, -0.97F, 0.3F);
			display->Line (-0.95F, 0.25F, -0.93F, 0.3F);
		}
		else if (delta < -5.0f)
		{
			//Arrow Right
			display->Line (-0.85F, 0.25F, -0.87F, 0.3F);
			display->Line (-0.85F, 0.25F, -0.83F, 0.3F);
		}
		else
		{	
			//Arrow Center
			display->Line (-0.90F, 0.25F, -0.92F, 0.3F);
			display->Line (-0.90F, 0.25F, -0.88F, 0.3F);
		}
		return;
	}

	float rightEdge=0.0F;
	float bigTickLen=0.0F;
	float smallTickLen=0.0F;
	float tickInc=0.0F, delta=0.0F;
	float x1=0.0F, x2=0.0F, y1=0.0F;

	rightEdge = hudWinX[AIRSPEED_WINDOW] + hudWinWidth[AIRSPEED_WINDOW];

	bigTickLen = hudWinWidth[AIRSPEED_WINDOW] * 0.5F;
	smallTickLen = bigTickLen * 0.5F;
	tickInc = hudWinHeight[AIRSPEED_WINDOW] / (float)(NUM_VERTICAL_TICKS - 1);

	y1 = hudWinY[AIRSPEED_WINDOW] + hudWinHeight[AIRSPEED_WINDOW] * 0.5F;
	x1 = rightEdge;

	delta = Speed - (cockpitFlightData.kias);
	delta *= 0.1F;
	y1 += delta * tickInc;
	y1 = max ( min ( y1, hudWinY[AIRSPEED_WINDOW] + hudWinHeight[AIRSPEED_WINDOW] - smallTickLen),
			hudWinY[AIRSPEED_WINDOW]);

	display->Line (x1, y1, x1 + bigTickLen, y1 + (smallTickLen * 0.5F));
	display->Line (x1, y1, x1 + bigTickLen, y1 - (smallTickLen * 0.5F));
}

void HudClass::DrawAltCarret(float Alt)
{
	if(FCC->GetMasterMode() == FireControlComputer::Dogfight)
		return;
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	float leftEdge = 0.0F;
	float bigTickLen=0.0F;
	float smallTickLen=0.0F;
	float tickInc=0.0F, delta=0.0F;
	float x1=0.0F, x2=0.0F, y1=0.0F;

	leftEdge = hudWinX[ALTITUDE_WINDOW] - (hudWinWidth[ALTITUDE_WINDOW] * 0.5F);

	bigTickLen = hudWinWidth[ALTITUDE_WINDOW] * 0.5F;
	smallTickLen = bigTickLen * 0.5F;
	tickInc = hudWinHeight[ALTITUDE_WINDOW] / (float)(NUM_VERTICAL_TICKS - 1);
	tickInc *= 0.75F;

	y1 = hudWinY[ALTITUDE_WINDOW] + hudWinHeight[ALTITUDE_WINDOW] * 0.5F;
	x1 = leftEdge * 0.95F;
	if(playerAC && playerAC->curWaypoint)
	{
		delta = Alt - -ownship->ZPos();
		delta *= 0.01F;
		y1 += delta * tickInc;
		y1 = max ( min ( y1, hudWinY[ALTITUDE_WINDOW] + hudWinHeight[ALTITUDE_WINDOW] - smallTickLen),
				hudWinY[ALTITUDE_WINDOW]);
		display->Line (x1 + bigTickLen, y1, x1, y1 + (smallTickLen * 0.5F));
		display->Line (x1 + bigTickLen, y1, x1, y1 - (smallTickLen * 0.5F));
	}
}
//MI
void HudClass::DrawCruiseIndexes(void)
{
	if(!OTWDriver.pCockpitManager || !OTWDriver.pCockpitManager->mpIcp){
		return;
	}

	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	switch(OTWDriver.pCockpitManager->mpIcp->GetCruiseIndex())
	{
		case 0:	//Cruise TOS
			//Speed
			DrawAirSpeedCarret(waypointSpeed);
			sprintf(SpeedText, "%.0f", max( min(waypointSpeed, 9999.0F), 0.0F));
			break;
		case 1:	//Cruise RNG
			//Speed
			if(playerAC &&  playerAC->af)
			{
				DrawAirSpeedCarret(playerAC->af->GetOptKias(2));
				sprintf(SpeedText, "%.0f", max(min(playerAC->af->GetOptKias(2), 9999.0F), 0.0F));
			}
			break;
		case 2:	//Cruise Home
			//opt speed and alt with climb profile and such
			if(OTWDriver.pCockpitManager && OTWDriver.pCockpitManager->mpIcp && playerAC)
			{
				float xCurr, yCurr, zCurr = 0;
				WayPointClass *wp = playerAC->GetWayPointNo(
						OTWDriver.pCockpitManager->mpIcp->HomeWP);
				if(wp)
				{		
					wp->GetLocation(&xCurr, &yCurr,&zCurr);
					float deltaX			= xCurr - playerAC->XPos();
					float deltaY			= yCurr - playerAC->YPos();
					float distanceToSta	= (float)sqrt(deltaX * deltaX + deltaY * deltaY);

					if (playerAC->af)
					{
						// we are within glide distance let's go max rng and decent
						float decentpoint = ((5000.0f - playerAC->ZPos()*0.8f)/1000.0f)/
							(max(100.0f,(float)playerAC->af->GetDragIndex())/100.0f);
						if (decentpoint > distanceToSta*FT_TO_NM)
						{
							DrawAirSpeedCarret(playerAC->af->GetOptKias(2));
							sprintf(SpeedText, "%.0f", max(min(playerAC->af->GetOptKias(2), 9999.0F), 0.0F));
						}
						// we are at optimum cruice let's cruice range here
						else if (fabs(-playerAC->ZPos() - playerAC->af->GetOptimumAltitude()) < 200.0f  &&
								fabs(playerAC->ZDelta())< 50.0f ||
								fabs(-playerAC->ZPos() - playerAC->af->GetOptimumAltitude()) < 1000.0f  &&
								playerAC->ZDelta()> 0.0f ||
								-playerAC->ZPos() > playerAC->af->GetOptimumAltitude())
						{
							DrawAirSpeedCarret(playerAC->af->GetOptKias(2));
							DrawAltCarret(playerAC->af->GetOptimumAltitude());
							sprintf(SpeedText, "%.0f", max(min(playerAC->af->GetOptKias(2), 9999.0F), 0.0F));
						}
						// we are in the climp towards best cruice altitude
						else
						{
							DrawAirSpeedCarret(playerAC->af->GetOptKias(0));
							DrawAltCarret(playerAC->af->GetOptimumAltitude());
							sprintf(SpeedText, "%.0f", max(min(playerAC->af->GetOptKias(0), 9999.0F), 0.0F));
						}
					}
				}
			}	
			//opt speed and alt with climb profile and such
			break;
		case 3:	//Cruise Edr
			//Speed
			if(playerAC &&  playerAC->af)
			{
				DrawAirSpeedCarret(playerAC->af->GetOptKias(1));
				sprintf(SpeedText, "%.0f", max(min(playerAC->af->GetOptKias(1), 9999.0F), 0.0F));
			}
			break;
		default:
			sprintf(SpeedText, "");
			break;
	}
}
