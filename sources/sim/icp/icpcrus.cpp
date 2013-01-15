#include "icp.h"
#include "simdrive.h"
#include "aircrft.h"
#include "airframe.h"
#include "weather.h"
#include "fcc.h"
#include "navsystem.h"
#include "find.h"
#include "aircrft.h"
#include "flightdata.h"
#include "simveh.h"

//MI for ICP stuff
extern bool g_bRealisticAvionics;
extern bool g_bRQDFix;

#include "hud.h"

int CalcFuelOnSta(float distanceToSta) {
	
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	float	fuelConsumed;
	int	fuelOnStation;
	float speed = max(1.0F, playerAC->GetVt());

	//fuelConsumed	= distanceToSta / playerAC->GetVt() * playerAC->af->FuelFlow() / 3600.0F;
	fuelConsumed	= distanceToSta / speed * playerAC->af->FuelFlow() / 3600.0F;
	//MI this will not take any external tanks
#if 0
	fuelOnStation	= (int) (playerAC->af->Fuel() - fuelConsumed);
#else
	fuelOnStation	= (int) (playerAC->GetTotalFuel() - fuelConsumed);
#endif

	return fuelOnStation;
}


void ICPClass::ExecCRUSMode(void) {

	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	static int			heading=0;
	static float		windSpeed=0.0F;
//	char				offset;
	float				distanceToSta=0.0F;
	int					altitude1=0, altitude2=0;
	int					altitude=0;
	float				cruise=0.0F;
	int					wpflags=0, action=0;
	float				deltaX=0.0F, deltaY=0.0F;
	float				xpos=0.0F, ypos=0.0F, zpos=0.0F;
	WayPointClass*		pwaypoint=NULL;
	int					fos=0;

	if(!g_bRealisticAvionics)
	{
		if(mUpdateFlags & CRUS_UPDATE) {
			mUpdateFlags &= !CRUS_UPDATE;
			
			sprintf(mpLine1, "CRUISE");

			switch(mList) {

			case STPT_LIST:

				wpflags	= mpCruiseWP->GetWPFlags();
				action	= mpCruiseWP->GetWPAction();

				//check the steerpoint list
				//get current steerpoint
				if(action == WP_LAND && !(wpflags & WPF_ALTERNATE)) {

					sprintf(mpLine1, "CRUISE HOME");
				}
				else if(action == WP_LAND && wpflags & WPF_ALTERNATE) {

					sprintf(mpLine1, "CRUISE ALTERNATE %d", mCruiseWPIndex + 1);
				}
				else {

					sprintf(mpLine1, "CRUISE STPT %d", mCruiseWPIndex + 1);
				}
			break;

			case MARK_LIST:
				sprintf(mpLine1, "CRUISE MARK %d", mCruiseMarkIndex + 1);
			break;

			case DLINK_LIST:
				sprintf(mpLine1, "CRUISE DLINK %d", mCruiseDLinkIndex + 1);
			break;
			}
				 Tpoint			pos;
				 pos.x = playerAC->XPos();
				 pos.y = playerAC->YPos();
				 pos.z = playerAC->ZPos();

			heading		= FloatToInt32(((WeatherClass*)realWeather)->WindHeadingAt(&pos) * RTD);
			
			if(heading <= 0) {
				heading += 180;
			}
			else if(heading > 0) {
				heading -= 180;
			}
			
			if(heading < 0) {
				heading = 360 + heading;
			}

			windSpeed	= ((WeatherClass*)realWeather)->WindSpeedInFeetPerSecond(&pos) * FTPSEC_TO_KNOTS;

		}
		switch(mList) {

		case STPT_LIST:

			pwaypoint = mpCruiseWP;

			pwaypoint->GetLocation(&xpos, &ypos, &zpos);
			
			deltaX			= xpos - playerAC->XPos();
			deltaY			= ypos - playerAC->YPos();
			distanceToSta	= (float)sqrt(deltaX * deltaX + deltaY * deltaY);
			fos				= max(0, CalcFuelOnSta(distanceToSta));
			
			sprintf(mpLine3, "FOS %d WND %03d / %3.1f KTS", fos, heading, windSpeed);
		break;

		case MARK_LIST:

			gNavigationSys->GetMarkWayPoint(mCruiseMarkIndex, &pwaypoint);
			
			if(pwaypoint == NULL) {
				sprintf(mpLine2, "NO MARK PT");
				sprintf(mpLine3, "WND %03d / %3.1f KTS", heading, windSpeed);

			}
			else {
				pwaypoint->GetLocation(&xpos, &ypos, &zpos);

				deltaX			= xpos - playerAC->XPos();
				deltaY			= ypos - playerAC->YPos();
				distanceToSta	= (float)sqrt(deltaX * deltaX + deltaY * deltaY);
				fos				= max(0, CalcFuelOnSta(distanceToSta));
		
				sprintf(mpLine3, "FOS %d WND %03d / %3.1f KTS", fos, heading, windSpeed);
			}
		break;

		case DLINK_LIST:

			gNavigationSys->GetMarkWayPoint(mCruiseDLinkIndex, &pwaypoint);
						
			if(pwaypoint == NULL) {
				sprintf(mpLine2, "NO DLINK PT");
				sprintf(mpLine3, "WND %03d / %3.1f KTS", heading, windSpeed);
			}
			else {

				pwaypoint->GetLocation(&xpos, &ypos, &zpos);

				deltaX			= xpos - playerAC->XPos();
				deltaY			= ypos - playerAC->YPos();
				distanceToSta	= (float)sqrt(deltaX * deltaX + deltaY * deltaY);
				fos				= max(0, CalcFuelOnSta(distanceToSta));

				sprintf(mpLine3, "FOS %d WND %03d / %3.1f KTS", fos, heading, windSpeed);
			}
		break;
		}
		
		if(pwaypoint) {
			cruise			= playerAC->af->GetOptimumCruise();
			altitude			= FloatToInt32(playerAC->af->GetOptimumAltitude());
			altitude1		= altitude / 1000;
			altitude2		= altitude % 1000;

			if(altitude1 > 0) {
				sprintf(mpLine2, "MACH %2.2f ALT %2d,%03dFT", cruise, altitude1, altitude2);
			}
			else {
				sprintf(mpLine2, "MACH %2.2f ALT %3dFT", cruise, altitude2);
			}
		}
	}
	else
	{
		if(Cruise_RNG)
			CruiseRNG();
		else if(Cruise_HOME)
			CruiseHOME();
		else if(Cruise_EDR)
			CruiseEDR();
		else
			CruiseTOS();
	}
}


void ICPClass::PNUpdateCRUSMode(int button, int) {

	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

	if(!playerAC) {
		return;
	}

	switch(mList) {

	case STPT_LIST:

		if(button == PREV_BUTTON) {

			mpCruiseWP = mpCruiseWP->GetPrevWP();

			if(mpCruiseWP == NULL) {
				mpCruiseWP		= playerAC->waypoint;
				mCruiseWPIndex = 0;
				//MI Done as comment told us to
				//mCruiseMarkIndex		= MAX_MARKPOINTS - 1; //Change this to mCruiseDLinkIndex =  MAX_DLINKPOINTS - 1; when DLINK goes back in game
				mCruiseDLinkIndex =  MAX_DLINKPOINTS - 1;
				mList				= MARK_LIST;
			}
			else {
				mCruiseWPIndex--;
			}
		}
		else {

			mpCruiseWP = mpCruiseWP->GetNextWP();

			if(mpCruiseWP == NULL) {
				mpCruiseWP		= playerAC->waypoint;
				mCruiseWPIndex = mNumWayPts - 1;
				mCruiseMarkIndex		= 0;
				mList				= MARK_LIST;
			}
			else {
				mCruiseWPIndex++;
			}
		}

	break;
		
	case MARK_LIST:

		if(button == PREV_BUTTON) {

			mCruiseMarkIndex--;

			if(mCruiseMarkIndex < 0) {
				mList		= STPT_LIST;
				mCruiseWPIndex = 0;

				mpCruiseWP	= playerAC->waypoint;

				while (mpCruiseWP && mpCruiseWP->GetNextWP())
				{
					mpCruiseWP = mpCruiseWP->GetNextWP();
					mCruiseWPIndex++;
				}
			}
		}
		else if(button == NEXT_BUTTON) {

			mCruiseMarkIndex++;

			if(mCruiseMarkIndex > MAX_MARKPOINTS - 1) {
				mCruiseWPIndex		= 0;
				mList			= STPT_LIST;
				mpCruiseWP	= playerAC->waypoint;
			}
		}

	break;
//MI changed for DLINK stuff, as comment tells us to
//#if 0			// Remove #if 0 when we put DLINK back in game
	case DLINK_LIST:

		if(button == PREV_BUTTON) {

			mCruiseDLinkIndex--;

			if(mCruiseDLinkIndex < 0) {
				mCruiseMarkIndex	= MAX_DLINKPOINTS - 1;
				mList			= MARK_LIST;
			}
		}
		else if(button == NEXT_BUTTON) {

			mCruiseDLinkIndex++;

			if(mCruiseDLinkIndex > MAX_DLINKPOINTS - 1) {
				mCruiseWPIndex		= 0;
				mList			= STPT_LIST;
				mpCruiseWP	= mpOwnship->waypoint;
			}
		}
	
	break;
//#endif
	}

	mUpdateFlags |= CRUS_UPDATE;
}

void ICPClass::CruiseRNG(void)
{
	//Check if this is valid. (Cause of CTD's?)
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

	if(!playerAC)
		return;
	//WayPointClass *wp = playerAC->GetWayPointNo(RangeWP);
	ClearStrings();
	//Line1
	FillDEDMatrix(0,8,"CRUS");
	FillDEDMatrix(0,13,"\x02",2);
	if(GetCruiseIndex() == 1)
		FillDEDMatrix(0,14,"RNG", 2);
	else
		FillDEDMatrix(0,14,"RNG");
	FillDEDMatrix(0,17,"\x02",2);
	//Line2
	FillDEDMatrix(1,8,"STPT");
	sprintf(tempstr, "%d", mWPIndex + 1);//RangeWP);
	FillDEDMatrix(1,15,tempstr);
	FillDEDMatrix(1,17,"\x01");
	//Line3
	FillDEDMatrix(2,8,"FUEL");
	/*if(wp)
		wp->GetLocation(&xCurr, &yCurr, &zCurr);*/
	if(playerAC->curWaypoint)
		playerAC->curWaypoint->GetLocation(&xCurr, &yCurr, &zCurr);
	
	float deltaX			= xCurr - playerAC->XPos();
	float deltaY			= yCurr - playerAC->YPos();
	float distanceToSta	= (float)sqrt(deltaX * deltaX + deltaY * deltaY);
	int fos				= max(0, CalcFuelOnSta(distanceToSta));
		sprintf(tempstr,"%dLBS",fos);
	if(playerAC->INSState(AircraftClass::INS_HUD_FPM)) //28 Jul 04 - If INS off/failed, we lose all cruise info
	{
		FillDEDMatrix(2,13,tempstr);
	}
	//Line5
	FillDEDMatrix(4,8,"WIND");
	GetWind();
	if(windSpeed > 1 && windSpeed < 9)
			sprintf(tempstr, "%d*   %dKTS", heading, (int)windSpeed);
		else if(windSpeed > 9)
			sprintf(tempstr, "%d*  %dKTS", heading, (int)windSpeed);
		else
		{
			windSpeed = 0;
			sprintf(tempstr, "%d*   %dKTS", heading, (int)windSpeed);
		}
	if(playerAC->INSState(AircraftClass::INS_HUD_FPM)) //28 Jul 04 - If INS off/failed, we lose all cruise info
	{
		FillDEDMatrix(4,13,tempstr);
	}
}
void ICPClass::CruiseHOME(void)
{
	//Check if this is valid. (Cause of CTD's?)
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

	if(!playerAC)
		return;
	WayPointClass *wp = playerAC->GetWayPointNo(HomeWP);
	ClearStrings();
	//Line1
	FillDEDMatrix(0,8,"CRUS");
	FillDEDMatrix(0,13,"\x02",2);
	//FillDEDMatrix(0,14,"HOME");
	if(GetCruiseIndex() == 2)
		FillDEDMatrix(0,14,"HOME", 2);
	else
		FillDEDMatrix(0,14,"HOME");
	FillDEDMatrix(0,18,"\x02",2);
	//Line2
	FillDEDMatrix(1,8,"HMPT");
	sprintf(tempstr, "%d",HomeWP);
	FillDEDMatrix(1,16,tempstr);
	FillDEDMatrix(1,18,"\x01");
	//Line3
	FillDEDMatrix(2,8,"FUEL");
	if(wp)
		wp->GetLocation(&xCurr, &yCurr,&zCurr);
	float deltaX			= xCurr - playerAC->XPos();
	float deltaY			= yCurr - playerAC->YPos();
	float distanceToSta	= (float)sqrt(deltaX * deltaX + deltaY * deltaY);
	int fos				= max(0, CalcFuelOnSta(distanceToSta));
	if (playerAC->af->platform->IsF16())
	{
		float	fuelConsumed;
	 	int	fuelOnStation;
	 	fuelConsumed	= distanceToSta / 6000.0f * 10.0f * 0.67f;
		fuelConsumed	+= min(1,distanceToSta / 6000.0f / 80.0f) *(500.0f - (-playerAC->ZPos()) /40.0f*0.5f);
	 	fuelOnStation	= (int) (playerAC->GetTotalFuel() - fuelConsumed);
		fos = fuelOnStation;
	}
		sprintf(tempstr,"%dLBS",fos);
	if(playerAC->INSState(AircraftClass::INS_HUD_FPM)) //28 Jul 04 - If INS off/failed, we lose all cruise info
	{
		FillDEDMatrix(2,13,tempstr);
	}
	//Line4
	FillDEDMatrix(3,5,"OPT ALT");
	int altitude		= FloatToInt32(playerAC->af->GetOptimumAltitude());
	int altitude1		= altitude / 1000;
	int altitude2		= altitude % 1000;
	if(altitude1 > 0) 
		sprintf(tempstr, "%2d,%03dFT",altitude1, altitude2);
	else
		sprintf(tempstr, "%3dFT", altitude2);
	if(playerAC->INSState(AircraftClass::INS_HUD_FPM)) //28 Jul 04 - If INS off/failed, we lose all cruise info
	{
		FillDEDMatrix(3,13,tempstr);
	}
	//Line5
	FillDEDMatrix(4,8,"WIND");
	GetWind();
	if(windSpeed > 1 && windSpeed < 9)
		sprintf(tempstr, "%d*   %dKTS", heading, (int)windSpeed);
	else if(windSpeed > 9)
		sprintf(tempstr, "%d*  %dKTS", heading, (int)windSpeed);
	else
	{
		windSpeed = 0;
		sprintf(tempstr, "%d*   %dKTS", heading, (int)windSpeed);
	}
	if(playerAC->INSState(AircraftClass::INS_HUD_FPM)) //28 Jul 04 - If INS off/failed, we lose all cruise info
	{
		FillDEDMatrix(4,(24-strlen(tempstr)),tempstr);
	}
}
void ICPClass::CruiseEDR(void)
{
	//Check if this is valid. (Cause of CTD's?)
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if(!playerAC)
		return;
	ClearStrings();
	//Line1
	FillDEDMatrix(0,8,"CRUS");
	FillDEDMatrix(0,13,"\x02",2);
	if(GetCruiseIndex() == 3)
		FillDEDMatrix(0,14,"EDR", 2);
	else
		FillDEDMatrix(0,14,"EDR");
	//FillDEDMatrix(0,14,"EDR");
	FillDEDMatrix(0,17,"\x02",2);
	//Line2
	FillDEDMatrix(1,8,"STPT");
	sprintf(tempstr, "%d",mWPIndex + 1);
	FillDEDMatrix(1,14,tempstr);
	FillDEDMatrix(1,16,"\x01");
	//Line3
	FillDEDMatrix(2,5,"TO BNGO");
	//Get our actual Bingo setting
	level = (long)((AircraftClass*)(playerAC))->GetBingoFuel();
	//Total fuel
	total = (long)((AircraftClass*)(playerAC))->GetTotalFuel();
	float EDR = (float)total - (float)level;
	long FF = (long)((AircraftClass*)(playerAC))->af->FuelFlow();
	EDR = EDR / FF;
	FindEDR((long)(EDR * 3600) ,tempstr);
	if(playerAC->INSState(AircraftClass::INS_HUD_FPM)) //28 Jul 04 - If INS off/failed, we lose all cruise info
	{
		FillDEDMatrix(2,14,tempstr);
	}
	//Line4
	FillDEDMatrix(3,4,"OPT MACH");
	float cruise = playerAC->af->GetOptimumCruise();
	sprintf(tempstr,"%2.2f",cruise);
	if(playerAC->INSState(AircraftClass::INS_HUD_FPM)) //28 Jul 04 - If INS off/failed, we lose all cruise info
	{
		FillDEDMatrix(3,14,tempstr);
	}
	//Line5
	FillDEDMatrix(4,8,"WIND");
	GetWind();
	if(windSpeed > 1 && windSpeed < 9)
		sprintf(tempstr, "%d*   %dKTS", heading, (int)windSpeed);
	else if(windSpeed > 9)
		sprintf(tempstr, "%d*  %dKTS", heading, (int)windSpeed);
	else
	{
		windSpeed = 0;
		sprintf(tempstr, "%d*   %dKTS", heading, (int)windSpeed);
	}
	if(playerAC->INSState(AircraftClass::INS_HUD_FPM)) //28 Jul 04 - If INS off/failed, we lose all cruise info
	{
		FillDEDMatrix(4,(24-strlen(tempstr)),tempstr);
	}
}
void ICPClass::CruiseTOS(void)
{
	//Check if this is valid. (Cause of CTD's?)
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

	if(!playerAC)
		return;
	//WayPointClass *wp = playerAC->GetWayPointNo(TOSWP);
	ClearStrings();
	char timeStr[16] = "";
	int hr, minute, sec;
	VU_TIME ETA;
	//Line1
	FillDEDMatrix(0,8,"CRUS");
	FillDEDMatrix(0,15,"\x02",2);
	//FillDEDMatrix(0,16,"TOS");
	if(GetCruiseIndex() == 0)
		FillDEDMatrix(0,16,"TOS", 2);
	else
		FillDEDMatrix(0,16,"TOS");
	FillDEDMatrix(0,19,"\x02",2);
	if(TOSWP > 9)
		sprintf(tempstr, "%2d", mWPIndex + 1);//TOSWP);
	else
		sprintf(tempstr," %d", mWPIndex + 1);//TOSWP);
	FillDEDMatrix(0,21,tempstr);
	FillDEDMatrix(0,23,"\x01");
	//Line2
	if(running)
	{
		FillDEDMatrix(1,5, "HK TIME");
		Difference = (vuxGameTime - Start);
		FormatTime(Difference / 1000, tempstr);
		FillDEDMatrix(1,16,tempstr);
	} 
	else
	{
		FillDEDMatrix(1,8,"SYSTEM");
		FormatTime(vuxGameTime / 1000, timeStr);
		FillDEDMatrix(1,16,timeStr);
	}
	//Line3
	//float ttg = TheHud->waypointArrival;

	float ttg = 0.0F;
	if(playerAC->curWaypoint)
		ttg = (float)playerAC->curWaypoint->GetWPArrivalTime() / SEC_TO_MSEC;

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
		sprintf(tempstr, "%02d:%02d:%02d", hr, abs(minute), abs(sec));
    else if (sec >= 0)
		sprintf(tempstr, "   %02d:%02d", abs(minute), sec);
    else
  		sprintf(tempstr, "  -%02d:%02d", abs(minute), abs(sec));
	FillDEDMatrix(2,5,"DES TOS");
	if(playerAC->INSState(AircraftClass::INS_HUD_FPM)) //28 Jul 04 - If INS off/failed, we lose all cruise info
	{
		FillDEDMatrix(2,16,tempstr);
	}
	//Line4
	FillDEDMatrix(3,9,"ETA");
	/*if(wp)
		wp->GetLocation(&xCurr, &yCurr,&zCurr);*/
	if(playerAC->curWaypoint)
		playerAC->curWaypoint->GetLocation(&xCurr, &yCurr, &zCurr);

	ETA	= SimLibElapsedTime / SEC_TO_MSEC + FloatToInt32(Distance(playerAC->XPos(),
		playerAC->YPos(), xCurr, yCurr) / playerAC->af->vt);
	if(!playerAC->OnGround())
		FormatTime(ETA, timeStr);
	if(playerAC->INSState(AircraftClass::INS_HUD_FPM)) //28 Jul 04 - If INS off/failed, we lose all cruise info
	{
		FillDEDMatrix(3,16,timeStr);
	}
	//Line5
	FillDEDMatrix(4,5,"RQD G/S");
	//Find the required G/S
//dpc RQD G/S fix - use GNDSPD instead of CAS speed
	float GroundSpeed = 0.0f;
	if(g_bRQDFix)
	{
		GroundSpeed = TheHud->waypointGNDSpeed;
	}
	else
	{
		GroundSpeed = TheHud->waypointSpeed;
		GroundSpeed += GroundSpeed * 0.1F;	//<--- This is NOT correct, but it's approximate
	}
	GroundSpeed = max(min(GroundSpeed, 9999),0);	
	sprintf(tempstr,"%3.0f",GroundSpeed);
	strcat(tempstr,"KTS");
	if(playerAC->INSState(AircraftClass::INS_HUD_FPM)) //28 Jul 04 - If INS off/failed, we lose all cruise info
	{
		FillDEDMatrix(4,16,tempstr);
	}
}
void ICPClass::StepHOMERNGSTPT(int mode)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

	if(Cruise_HOME)
		HomeWP = GetHOMERNGSTPTNum(HomeWP, mode);
	/*else if(Cruise_RNG)
		RangeWP = GetHOMERNGSTPTNum(RangeWP, mode);
	else
		TOSWP = GetHOMERNGSTPTNum(TOSWP, mode);*/
	else
	{
		if(mode == PREV_BUTTON)
			((AircraftClass*)(playerAC))->FCC->waypointStepCmd = -1;
		else
			((AircraftClass*)(playerAC))->FCC->waypointStepCmd = 1;
	}
}
int ICPClass::GetHOMERNGSTPTNum(int var, int mode)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

	WayPointClass* nextWaypoint;
	WayPointClass* curWaypoint;
	int tempvar = var;
	
	if(mode == NEXT_BUTTON)
	{
		curWaypoint	= playerAC->GetWayPointNo(tempvar);
		if(curWaypoint && curWaypoint->GetNextWP())	// OW 
			nextWaypoint = curWaypoint->GetNextWP();
		else
			nextWaypoint	= NULL;

		if (nextWaypoint)
			tempvar++;
		else
			tempvar = 1;
	}
	else
	{
		curWaypoint	= playerAC->GetWayPointNo(tempvar);
		if(curWaypoint && curWaypoint->GetPrevWP())
			nextWaypoint = curWaypoint->GetPrevWP();
		else
			nextWaypoint = NULL;

		if(nextWaypoint)
		    tempvar--;
		else
		{
			while(curWaypoint && curWaypoint->GetNextWP())
			{
				curWaypoint = curWaypoint->GetNextWP();
				tempvar++;
			}
		}
	}
	return tempvar;
}