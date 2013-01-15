#include "stdhdr.h"
#include "Graphics\Include\display.h"
#include "cmpclass.h"
#include "flightData.h"
#include "radardoppler.h"	//MI
#include "simdrive.h"	//MI
#include "aircrft.h"	//MI
#include "simdrive.h"	//MI
#include "simobj.h"
#include "object.h"	

extern bool g_bIFF;
extern bool g_bINS;
extern bool g_bSmallerBullseye;				//Wombat778 11-12-2003

void DrawBullseyeData (VirtualDisplay* display, float cursorX, float cursorY)
{
float bullseyeX, bullseyeY;
float azFrom, azTo, range;
float cursX, cursY;
float ownYaw = cockpitFlightData.yaw;
char str[12];
mlTrig trig;

   // Find current bullseye
   TheCampaign.GetBullseyeSimLocation (&bullseyeX, &bullseyeY);

   // Range and bearing from bullseye to cursor
   mlSinCos (&trig, ownYaw);
   cursorX *= NM_TO_FT;
   cursorY *= NM_TO_FT;
	cursX = cockpitFlightData.x + cursorX * trig.cos - cursorY * trig.sin;
	cursY = cockpitFlightData.y + cursorX * trig.sin + cursorY * trig.cos;

	azFrom = RTD * (float)atan2 (cursY - bullseyeY, cursX - bullseyeX);
	range = (float)sqrt( (cursX-bullseyeX)*(cursX-bullseyeX) + (cursY-bullseyeY)*(cursY-bullseyeY) );
	if (azFrom < 0.0F)
		azFrom += 360.0F;

	// Offset for Bullseye symbology
	display->AdjustOriginInViewport (-0.75F, -0.62F);//me123 from .80 to .75 and .65 to .62 to move the data

	sprintf (str, "%03.0f %02.0f", azFrom, range * FT_TO_NM);
	ShiAssert (strlen (str) < sizeof(str));
	display->TextLeft(-0.95F - -0.75f, 0.2F, str); // draw from left - to keep it on the screen

   // Range, bearing from bullseye to ownship
	azFrom = RTD * (float)atan2 (cockpitFlightData.y - bullseyeY, cockpitFlightData.x - bullseyeX);
	range = (float)sqrt( (cockpitFlightData.x-bullseyeX)*(cockpitFlightData.x-bullseyeX) + (cockpitFlightData.y-bullseyeY)*(cockpitFlightData.y-bullseyeY) );
	if (azFrom < 0.0F)
		azFrom += 360.0F;

   // Absolute bearing to bullseye from ownship
   azTo = (azFrom + 180.0F) * DTR;

   // Make relative to heading
   if (ownYaw < 0.0F)
      azTo -= 360.0F * DTR + ownYaw;
   else
      azTo -= ownYaw;

   // Draw the circle symbol
   display->Circle (0.0F, 0.0F, 0.1F);

   // Add Range
	sprintf (str, "%.0f", range * FT_TO_NM);
	ShiAssert (strlen (str) < sizeof(str));
   display->TextCenterVertical (0.0F, 0.0F, str);

   // Add bearing from
	sprintf (str, "%03.0f", azFrom);
	ShiAssert (strlen (str) < sizeof(str));
   display->TextCenter (0.0F, -0.15F, str);

   // Add heading to
   display->AdjustRotationAboutOrigin(azTo);
	display->Tri (0.0F, 0.15F, -0.025F, 0.1F, 0.025F, 0.1F);
   display->ZeroRotationAboutOrigin();
   display->CenterOriginInViewport ();
}
//MI draws the bullseye info for the cursor on the side of the MFD
void DrawCursorBullseyeData(VirtualDisplay* display, float cursorX, float cursorY)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	float bullseyeX, bullseyeY;
	float azFrom, range;
	float cursX, cursY;
	float ownYaw = cockpitFlightData.yaw;
	char str[12];
	mlTrig trig;

   // Find current bullseye
   TheCampaign.GetBullseyeSimLocation (&bullseyeX, &bullseyeY);

   // Range and bearing from bullseye to cursor
   mlSinCos (&trig, ownYaw);
   cursorX *= NM_TO_FT;
   cursorY *= NM_TO_FT;
	cursX = cockpitFlightData.x + cursorX * trig.cos - cursorY * trig.sin;
	cursY = cockpitFlightData.y + cursorX * trig.sin + cursorY * trig.cos;

	//with a locked target in STT, we always get bearing to target
	RadarDopplerClass* theRadar = (RadarDopplerClass*)FindSensor(playerAC, SensorClass::Radar);
	if(theRadar)
	{
		if(theRadar->CurrentTarget() && theRadar->CurrentTarget()->BaseData() && theRadar->IsSet(RadarDopplerClass::STTingTarget))
		{
			float yPos = theRadar->CurrentTarget()->BaseData()->YPos();
			float xPos = theRadar->CurrentTarget()->BaseData()->XPos();
			azFrom = RTD * (float)atan2(yPos - bullseyeY, xPos - bullseyeX);
			range = (float)sqrt((xPos - bullseyeX)*(xPos - bullseyeX) + (yPos - bullseyeY)*(yPos - bullseyeY) );
			if (azFrom < 0.0F)
				azFrom += 360.0F;
		}
		else
		{
			azFrom = RTD * (float)atan2 (cursY - bullseyeY, cursX - bullseyeX);
			range = (float)sqrt( (cursX-bullseyeX)*(cursX-bullseyeX) + (cursY-bullseyeY)*(cursY-bullseyeY) );
			if (azFrom < 0.0F)
				azFrom += 360.0F;
		}
	}
	// Offset for Bullseye symbology
	display->AdjustOriginInViewport (-0.75F, -0.62F);//me123 from .80 to .75 and .65 to .62 to move the data

	/*if(range * FT_TO_NM > 99)
		range = 99 * NM_TO_FT;*/
	sprintf (str, "%03.0f %02.0f", azFrom, range * FT_TO_NM);
	ShiAssert (strlen (str) < sizeof(str));

	if(g_bINS)
	{ 
		if(playerAC && !playerAC->INSState(AircraftClass::INS_HSD_STUFF))
		{
			display->CenterOriginInViewport ();
			return;
		}
	}

	display->TextLeft(-0.95F - -0.75f, 0.17F, str); // draw from left - to keep it on the screen

   display->CenterOriginInViewport ();
}

//Wombat778 1/15/03 Added so that when bullseye mode is off, range/bearing to current STPT is displayed instead.

void DrawSteerPointCursorData(VirtualDisplay* display,FalconEntity* platform, float cursorX, float cursorY)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	float steerpointX, steerpointY, steerpointZ;
	float azFrom, range;
	float cursX, cursY;
	float ownYaw = cockpitFlightData.yaw;
	char str[12];
	mlTrig trig;

   // Find current steerpoint
   if (platform&&((SimVehicleClass*)platform)->curWaypoint)
   {
	   ((SimVehicleClass*)platform)->curWaypoint->GetLocation (&steerpointX, &steerpointY, &steerpointZ);

   // Range and bearing from steerpoint to cursor
		mlSinCos (&trig, ownYaw);
		cursorX *= NM_TO_FT;
		cursorY *= NM_TO_FT;
		cursX = cockpitFlightData.x + cursorX * trig.cos - cursorY * trig.sin;
		cursY = cockpitFlightData.y + cursorX * trig.sin + cursorY * trig.cos;

		//with a locked target in STT, we always get bearing to target
		RadarDopplerClass* theRadar = (RadarDopplerClass*)FindSensor(playerAC, SensorClass::Radar);
		if(theRadar)
		{
			if(theRadar->CurrentTarget() && theRadar->CurrentTarget()->BaseData() && theRadar->IsSet(RadarDopplerClass::STTingTarget))
			{
				float yPos = theRadar->CurrentTarget()->BaseData()->YPos();
				float xPos = theRadar->CurrentTarget()->BaseData()->XPos();
				azFrom = RTD * (float)atan2(yPos - steerpointY, xPos - steerpointX);
				range = (float)sqrt((xPos - steerpointX)*(xPos - steerpointX) + (yPos - steerpointY)*(yPos - steerpointY) );
				if (azFrom < 0.0F)
					azFrom += 360.0F;
			}
			else
			{
				azFrom = RTD * (float)atan2 (cursY - steerpointY, cursX - steerpointX);
				range = (float)sqrt( (cursX-steerpointX)*(cursX-steerpointX) + (cursY-steerpointY)*(cursY-steerpointY) );
				if (azFrom < 0.0F)
					azFrom += 360.0F;
			}
		}
		// Offset for steerpoint symbology
		display->AdjustOriginInViewport (-0.75F, -0.62F);//me123 from .80 to .75 and .65 to .62 to move the data
	
		/*if(range * FT_TO_NM > 99)
			range = 99 * NM_TO_FT;*/
		sprintf (str, "%03.0f %02.0f", azFrom, range * FT_TO_NM);
		ShiAssert (strlen (str) < sizeof(str));

		if(g_bINS)
		{ 
			if(playerAC && !playerAC->INSState(AircraftClass::INS_HSD_STUFF))
			{
				display->CenterOriginInViewport ();
				return;
			}
		}

		display->TextLeft(-0.95F - -0.75f, 0.17F, str); // draw from left - to keep it on the screen

		display->CenterOriginInViewport ();
   }
}


//MI draws the circle
void DrawBullseyeCircle(VirtualDisplay* display, float cursorX, float cursorY)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

	float bullseyeX, bullseyeY;
	float azFrom, azTo, range;
	float cursX, cursY;
	float ownYaw = cockpitFlightData.yaw;
	char str[12];
	mlTrig trig;

   // Find current bullseye
   TheCampaign.GetBullseyeSimLocation (&bullseyeX, &bullseyeY);

   // Range and bearing from bullseye to cursor
   mlSinCos (&trig, ownYaw);
   cursorX *= NM_TO_FT;
   cursorY *= NM_TO_FT;
	cursX = cockpitFlightData.x + cursorX * trig.cos - cursorY * trig.sin;
	cursY = cockpitFlightData.y + cursorX * trig.sin + cursorY * trig.cos;

	azFrom = RTD * (float)atan2 (cursY - bullseyeY, cursX - bullseyeX);
	range = (float)sqrt( (cursX-bullseyeX)*(cursX-bullseyeX) + (cursY-bullseyeY)*(cursY-bullseyeY) );
	if (azFrom < 0.0F)
		azFrom += 360.0F;

	//Cobra removed the g_bIFF
	// Offset for Bullseye symbology
	if(/*g_bIFF||*/g_bSmallerBullseye)				//Wombat778 11-12-2003 made optional on g_bSmallerBullseye 11-07-2003		
		display->AdjustOriginInViewport (-0.90F, -0.80F);//me123 from .80 to .75 and .65 to .62 to move the data
	else
		display->AdjustOriginInViewport (-0.85F, -0.70F);//me123 from .80 to .75 and .65 to .62 to move the data

	
   //Range, bearing from bullseye to ownship
	azFrom = RTD * (float)atan2 (cockpitFlightData.y - bullseyeY, cockpitFlightData.x - bullseyeX);
	range = (float)sqrt( (cockpitFlightData.x-bullseyeX)*(cockpitFlightData.x-bullseyeX) + (cockpitFlightData.y-bullseyeY)*(cockpitFlightData.y-bullseyeY) );
	if (azFrom < 0.0F)
		azFrom += 360.0F;

   // Absolute bearing to bullseye from ownship
   azTo = (azFrom + 180.0F) * DTR;

   // Make relative to heading
   if (ownYaw < 0.0F)
      azTo -= 360.0F * DTR + ownYaw;
   else
      azTo -= ownYaw;

   //Cobra removed the g_bIff
   // Draw the circle symbol
   if(/*g_bIFF||*/g_bSmallerBullseye)			//Wombat778 11-12-2003 made optional on g_bSmallerBullseye 11-07-2003		Smaller bullseye is more realistic
   {
	   //set a smaller font (needed)
	   int ofont = display->CurFont();
	   display->SetFont(0);
	   display->Circle (0.0F, 0.0F, 0.06F);

	   //Add Range	but only if it's less then 99 miles, otherwise we don't see it
	   if((range * FT_TO_NM) > 99)
		   sprintf(str, "");
	   else
		   sprintf (str, "%.0f", range * FT_TO_NM);
   
	   ShiAssert (strlen (str) < sizeof(str));
	   display->TextCenterVertical(0.005F, 0.0F, str);

	   // Add bearing from
		sprintf (str, "%03.0f", azFrom);
		ShiAssert (strlen (str) < sizeof(str));
	   display->TextCenter (0.0F, -0.08F, str);

	   // Add heading to
	   display->AdjustRotationAboutOrigin(azTo);
	   display->Tri(0.0F, 0.10F, -0.025F, 0.06F, 0.025F, 0.06F);
	   display->ZeroRotationAboutOrigin();
	   display->CenterOriginInViewport ();
	   //restore the font
	   display->SetFont(ofont);
   }
   else
   {
	   if(g_bINS)
	   {
		   if(playerAC && !playerAC->INSState(AircraftClass::INS_HSD_STUFF))
		   {
			   // Draw the circle symbol
			   display->Circle (0.0F, 0.0F, 0.1F);
			   return;
		   }
	   }
	   // Draw the circle symbol
	   display->Circle (0.0F, 0.0F, 0.1F);

	   //Add Range	but only if it's less then 99 miles, otherwise we don't see it
	   if((range * FT_TO_NM) > 99)
		   sprintf(str, "");
	   else
		   sprintf (str, "%.0f", range * FT_TO_NM);
		ShiAssert (strlen (str) < sizeof(str));
	   display->TextCenterVertical (0.0F, 0.0F, str);

	   // Add bearing from
		sprintf (str, "%03.0f", azFrom);
		ShiAssert (strlen (str) < sizeof(str));
	   display->TextCenter (0.0F, -0.10F, str);

	   // Add heading to
	   display->AdjustRotationAboutOrigin(azTo);
		display->Tri (0.0F, 0.15F, -0.025F, 0.1F, 0.025F, 0.1F);
	   display->ZeroRotationAboutOrigin();
	   display->CenterOriginInViewport ();
   }
}



void GetBullseyeToOwnship(char *string)
{
	float bullseyeX, bullseyeY;
	float azFrom, range;

	TheCampaign.GetBullseyeSimLocation (&bullseyeX, &bullseyeY);
	azFrom = RTD * (float)atan2 (cockpitFlightData.y - bullseyeY, cockpitFlightData.x - bullseyeX);
	range = (float)sqrt( (cockpitFlightData.x-bullseyeX)*(cockpitFlightData.x-bullseyeX) + (cockpitFlightData.y-bullseyeY)*(cockpitFlightData.y-bullseyeY) );
	if (azFrom < 0.0F)
		azFrom += 360.0F;

	if (azFrom > 360.0f)
		azFrom -= 360.0f;


	if((range * FT_TO_NM) > 999)
		sprintf(string, "%03.0f  999", azFrom);		
	else 
		sprintf(string, "%03.0f  %03.0f", azFrom, range*FT_TO_NM);
}
