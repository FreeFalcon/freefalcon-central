#include "icp.h"
#include "simdrive.h"
#include "aircrft.h"
#include "navsystem.h"
#include "flightdata.h"
#include "Phyconst.h"
#include "fcc.h"
#include "hud.h"
#include "cpmanager.h"
#include "commands.h"
#include "otwdrive.h"
#include "find.h"
#include "airframe.h"

#define PosA		1
#define PosB		2
#define PosC		7
#define PosD		8
#define PosE		13
#define PosF		14
#define PosG		20
#define PosH		21

extern bool g_bINS;
extern bool g_bIFF;

void ICPClass::ExecLISTMode(void)
{
	//Line1
	FillDEDMatrix(0,10,"LIST");
	sprintf(tempstr, "%d",mWPIndex + 1);
	if(mWPIndex+1 <10)
		FillDEDMatrix(0,22,tempstr);
	else
		FillDEDMatrix(0,21,tempstr);
	FillDEDMatrix(0,24,"\x01");
	//Line2
	FillDEDMatrix(1,PosA,"1",2);
	FillDEDMatrix(1,PosB,"DEST");
	FillDEDMatrix(1,PosC,"2",2);
	FillDEDMatrix(1,PosD,"BNGO");
	FillDEDMatrix(1,PosE,"3",2);
	FillDEDMatrix(1,PosF,"VIP");
	FillDEDMatrix(1,PosG,"R",2);
	FillDEDMatrix(1,PosH,"INTG");
	//Line3
	FillDEDMatrix(2,PosA,"4",2);
	FillDEDMatrix(2,PosB,"NAV");
	FillDEDMatrix(2,PosC,"5",2);
	FillDEDMatrix(2,PosD,"MAN");
	FillDEDMatrix(2,PosE,"6",2);
	FillDEDMatrix(2,PosF,"INS");
	FillDEDMatrix(2,PosG,"E",2);
	FillDEDMatrix(2,PosH,"DLNK");
	//Line4
	FillDEDMatrix(3,PosA,"7",2);
	FillDEDMatrix(3,PosB,"EWS");
	FillDEDMatrix(3,PosC,"8",2);
	FillDEDMatrix(3,PosD,"MODE");
	FillDEDMatrix(3,PosE,"9",2);
	FillDEDMatrix(3,PosF,"VRP");
	FillDEDMatrix(3,PosG,"0",2);
	FillDEDMatrix(3,PosH,"MISC");
}
void ICPClass::ExecDESTMode(void)
{
	//Line1
	FillDEDMatrix(0,4,"DEST  DIR");
	AddSTPT(0,22);
		
	//Get the current waypoint location
	xCurr = yCurr = zCurr = 0;
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if (playerAC && playerAC->curWaypoint)
		playerAC->curWaypoint->GetLocation(&xCurr, &yCurr, &zCurr);
		
	latitude	= (FALCON_ORIGIN_LAT * FT_PER_DEGREE + xCurr) / EARTH_RADIUS_FT;
	cosLatitude = (float)cos(latitude);
	longitude	= ((FALCON_ORIGIN_LONG * DTR * EARTH_RADIUS_FT * cosLatitude) + yCurr) / (EARTH_RADIUS_FT * cosLatitude);
		
	latitude	*= RTD;
	longitude	*= RTD;
	
	longDeg		= FloatToInt32(longitude);
	longMin		= (float)fabs(longitude - longDeg) * DEG_TO_MIN;

	latDeg		= FloatToInt32(latitude);
	latMin		= (float)fabs(latitude - latDeg) * DEG_TO_MIN;

	// format lat/long here
	if(latMin < 10.0F) 
		sprintf(latStr, "%3d*0%2.2f\'", latDeg, latMin);
	else 
		sprintf(latStr, "%3d*%2.2f\'", latDeg, latMin);
	if(longMin < 10.0F) 
		sprintf(longStr, "%3d*0%2.2f\'", longDeg, longMin);
	else 
		sprintf(longStr, "%3d*%2.2f\'", longDeg, longMin);

	//Line2
	if(IsICPSet(ICPClass::EDIT_LAT) && Manual_Input)
	{
		FillDEDMatrix(1,5,"LAT  N");
		PossibleInputs = 6;
		ScratchPad(1,13,24);
	}
	else
	{
		if(IsICPSet(ICPClass::EDIT_LAT))
		{
			FillDEDMatrix(1,13,"\x02",2);
			FillDEDMatrix(1,24,"\x02",2);
		}
		else
		{
			FillDEDMatrix(1,13," ");
			FillDEDMatrix(1,24," ");
		}
		FillDEDMatrix(1,5,"LAT  N");
		sprintf(tempstr,"%s", latStr);
		FillDEDMatrix(1,14,tempstr);
		
	}
	//Line3
	if(IsICPSet(ICPClass::EDIT_LONG) && Manual_Input)
	{
		FillDEDMatrix(2,5,"LNG  E");
		PossibleInputs = 7;
		ScratchPad(2,13,24);
	}
	else
	{	
		if(IsICPSet(ICPClass::EDIT_LONG))
		{
			FillDEDMatrix(2,13,"\x02",2);
			FillDEDMatrix(2,24,"\x02",2);
		}
		else
		{
			FillDEDMatrix(2,13," ");
			FillDEDMatrix(2,24," ");
		}
		FillDEDMatrix(2,5,"LNG  E");
		sprintf(tempstr, "%s", longStr);
		FillDEDMatrix(2,14,tempstr);
	}
	//Line4
	FillDEDMatrix(3,4,"ELEV");
	sprintf(tempstr,"%4.0fFT",-zCurr);
	FillDEDMatrix(3,13,tempstr);
	//Line5
	if(playerAC)
	{
		VU_TIME ETA = SimLibElapsedTime / SEC_TO_MSEC + FloatToInt32(Distance(
		playerAC->XPos(), playerAC->YPos(), xCurr, yCurr) 
		/ playerAC->af->vt);
		FormatTime(ETA, tempstr);
	}
	FillDEDMatrix(4,5,"TOS");
	FillDEDMatrix(4,13,tempstr);
}
void ICPClass::ExecOA1Mode(void)
{
	//Line1
	FillDEDMatrix(0,4,"DEST  OA1");
	AddSTPT(0,17);
	//Line3
	FillDEDMatrix(2,5,"RNG");
	if(OA_RNG)
	{
		PossibleInputs = 6;
		ScratchPad(2,9,18);
	}
	else
	{
		sprintf(tempstr,"%dFT", iOA_RNG);
		FillDEDMatrix(2,18-strlen(tempstr),tempstr);
	}
	//Line4
	FillDEDMatrix(3,5,"BRG");
	if(OA_BRG)
	{
		PossibleInputs = 4;
		ScratchPad(3,9,16);
	}
	else
	{
		sprintf(tempstr,"%3.1f'",fOA_BRG);
		FillDEDMatrix(3,(16-strlen(tempstr)),tempstr);
	}
	//Line5
	FillDEDMatrix(4,4,"ELEV");
	if(OA_ALT)
	{
		PossibleInputs = 6;
		ScratchPad(4,9,18);
	}
	else
	{
		sprintf(tempstr,"%dFT",iOA_ALT);
		FillDEDMatrix(4,(18-strlen(tempstr)),tempstr);
	}
}
void ICPClass::ExecOA2Mode(void)
{
	//Line1
	FillDEDMatrix(0,4,"DEST  OA2");
	AddSTPT(0,17);
	//Line3
	FillDEDMatrix(2,5,"RNG");
	if(OA_RNG)
	{
		PossibleInputs = 6;
		ScratchPad(2,9,18);
	}
	else
	{
		sprintf(tempstr,"%dFT", iOA_RNG2);
		FillDEDMatrix(2,18-strlen(tempstr),tempstr);
	}
	//Line4
	FillDEDMatrix(3,5,"BRG");
	if(OA_BRG)
	{
		PossibleInputs = 4;
		ScratchPad(3,9,16);
	}
	else
	{
		sprintf(tempstr,"%3.1f'",fOA_BRG2);
		FillDEDMatrix(3,16-strlen(tempstr),tempstr);
	}
	//Line5
	FillDEDMatrix(4,4,"ELEV");
	if(OA_ALT)
	{
		PossibleInputs = 6;
		ScratchPad(4,9,18);
	}
	else
	{
		sprintf(tempstr,"%dFT", iOA_ALT2);
		FillDEDMatrix(4,(18-strlen(tempstr)),tempstr);
	}
}
void ICPClass::ExecBingo(void)
{
	//Line1
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	FillDEDMatrix(0,10,"BINGO");
	AddSTPT(0,22);
	//Get our current FOB
	total = 0;
	if (playerAC)
		total = (long)((AircraftClass*)(playerAC))->GetTotalFuel();
	//Line3
	PossibleInputs = 5;
	FillDEDMatrix(2,5,"SET");
	ScratchPad(2,10,19);
	//Line4
	FillDEDMatrix(3,3,"TOTAL");
	sprintf(tempstr,"%dLBS", total);
	FillDEDMatrix(3,11,tempstr);
}
void ICPClass::ExecVIPMode(void)
{
	//Line 1
	FillDEDMatrix(0,10,"VIP-TO-TGT");
	//Line 2
	FillDEDMatrix(1,9,"VIP  8 \x01");
	//Line 3
	FillDEDMatrix(2,8,"TBRG");
	if(VIP_BRG)
	{
		PossibleInputs = 4;
		ScratchPad(2,14,21);
	}
	else
	{
		sprintf(tempstr,"%3.1f'",fVIP_BRG);
		FillDEDMatrix(2,21-strlen(tempstr),tempstr);
	}
	//Line4
	FillDEDMatrix(3,9,"RNG");
	if(VIP_RNG)
	{
		PossibleInputs = 6;
		ScratchPad(3,13,22);
	}
	else
	{
		sprintf(tempstr,"%dFT", iVIP_RNG);
		FillDEDMatrix(3,22-strlen(tempstr),tempstr);
	}
	//Line5
	FillDEDMatrix(4,8,"ELEV");
	if(VIP_ALT)
	{
		PossibleInputs = 5;
		ScratchPad(4,13,21);
	}
	else
	{
		sprintf(tempstr,"%dFT", iVIP_ALT);
		FillDEDMatrix(4,(21-strlen(tempstr)),tempstr);
	}
}
void ICPClass::ExecNAVMode(void)
{
	//Line1
	FillDEDMatrix(0,10,"NAV STATUS");
	AddSTPT(0,22);
	//Line2
	FillDEDMatrix(1,5,"SYS ACCUR   HIGH");
	//Line3
	FillDEDMatrix(2,5,"GPS ACCUR   HIGH");
	//Line4
	FillDEDMatrix(3,5,"MSN DUR");
	FillDEDMatrix(3,15,"\x02",2);
	FillDEDMatrix(3,17,"5");
	FillDEDMatrix(3,18,"\x02",2);
	FillDEDMatrix(3,20,"DAYS");
	//Line5
	FillDEDMatrix(4,5,"KEY VALID");
}
void ICPClass::ExecMANMode(void)
{
	//Line 1
	FillDEDMatrix(0,10,"MAN");
	AddSTPT(0,22);
	//Line 2
	FillDEDMatrix(1,6,"WSPAN");
	PossibleInputs = 3;
	ScratchPad(1,12,18);
	//Line 3
	FillDEDMatrix(2,10,"MBAL");
	//Line 4 
	FillDEDMatrix(3,5,"RNG");
	FillDEDMatrix(3,17,"0FT");
	//Line5
	FillDEDMatrix(4,6,"TOF");
	FillDEDMatrix(4,14,"0.0SEC");
}
void ICPClass::ExecINSMode(void)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	//Line1
	if(g_bINS)
	{
		GetINSInfo();					//Wombat778 10-17-2003  Update the INS info.  Was static before.
		FillDEDMatrix(0,5,"INS");
		AddSTPT(0,22);
		char tempstr1[10] = "";
		char tempstr2[10] = "";
		if(playerAC)
		{
			INSTime = playerAC->INSAlignmentTimer;
			int State = playerAC->INSStatus;
			
			INSTime /= 60;
			sprintf(tempstr2,"%d", State);
			sprintf(tempstr1,"%2.1f/", INSTime);
			FillDEDMatrix(0,9,tempstr1);
			FillDEDMatrix(0,14, tempstr2);
			if(!playerAC->HasAligned && 
				playerAC->INSState(AircraftClass::INS_Nav))
			{
				FillDEDMatrix(0,14,"00");
			}
			else if(!playerAC->INSState(AircraftClass::INS_Aligned))
			{
				if(State <= 70)
					FillDEDMatrix(0,17,"RDY");
				else
					FillDEDMatrix(0,17,"   ");
			}
			else if(playerAC->INSState(AircraftClass::INS_AlignNorm) &&
				playerAC->INSState(AircraftClass::INS_Aligned))
			{
				if(vuxRealTime & 0x200)
					FillDEDMatrix(0,17,"RDY");
				else
					FillDEDMatrix(0,17,"   ");
			}

			//coords stuff
			FillDEDMatrix(1,5,"LAT  N");
			FillDEDMatrix(2,5,"LNG  E");
			FillDEDMatrix(3,4,"SALT");
			FillDEDMatrix(4,3,"THDG");
			if(INSLine == 0 && Manual_Input)
			{
				//LAT line
				PossibleInputs = 6;
				ScratchPad(1,12,23);
			}
			else
			{
				if(playerAC->INSState(AircraftClass::INS_HUD_FPM)) //28 Jul 04 - If INS off/failed, we lose all cruise info
					{
						FillDEDMatrix(1, 13, INSLat);
					}
				if(INSLine == 0)
				{
					FillDEDMatrix(1,12,"\x02",2);
					FillDEDMatrix(1,23,"\x02",2);
				}
				else
				{
						FillDEDMatrix(1,12," ");
						FillDEDMatrix(1,23," ");
				}
			}
			if(INSLine == 1 && Manual_Input)
			{
				//LONG line
				PossibleInputs = 7;
				ScratchPad(2,12,23);
			}
			else
			{
				if(playerAC->INSState(AircraftClass::INS_HUD_FPM)) //28 Jul 04 - If INS off/failed, we lose all cruise info
				{
					FillDEDMatrix(2,13, INSLong);
				}
				if(INSLine == 1)
				{
					FillDEDMatrix(2,12,"\x02",2);
					FillDEDMatrix(2,23,"\x02",2);
				}
				else
				{
						FillDEDMatrix(2,12," ");
						FillDEDMatrix(2,23," ");
				}
			}
			if(INSLine == 2 && Manual_Input)
			{
				//ALT line
				PossibleInputs = 5;
				ScratchPad(3,14,22);
			}
			else
			{
				if(playerAC->INSState(AircraftClass::INS_HUD_FPM)) //28 Jul 04 - If INS off/failed, we lose all cruise info
				{
						FillDEDMatrix(3,21-(strlen(altStr)), " ");	//Wombat778 10-17-2003 make sure that old digit doesnt show below 10000ft
					FillDEDMatrix(3,22-(strlen(altStr)), altStr);
				}
				if(INSLine == 2)
				{
					FillDEDMatrix(3,14,"\x02",2);
					FillDEDMatrix(3,22,"\x02",2);
				}
				else
				{
					if(playerAC->INSState(AircraftClass::INS_HUD_FPM)) //28 Jul 04 - If INS off/failed, we lose all cruise info
					{
						FillDEDMatrix(3,14," ");
						FillDEDMatrix(3,22," ");
					}
				}
			}
			if(INSLine == 3 && Manual_Input)
			{
				//THDG line
				PossibleInputs = 4;
				ScratchPad(4,8,15);

				int GroundSpeed = FloatToInt32((float)sqrt(cockpitFlightData.xDot*cockpitFlightData.xDot + 
					cockpitFlightData.yDot*cockpitFlightData.yDot) * FTPSEC_TO_KNOTS);
				GroundSpeed = max(GroundSpeed, 999);
				sprintf(tempstr,"%d",GroundSpeed);
				FillDEDMatrix(4,17,"G/S");
				FillDEDMatrix(4,(25-strlen(tempstr)),tempstr);
			}
			else
			{
				if(playerAC->INSState(AircraftClass::INS_HUD_FPM)) //28 Jul 04 - If INS off/failed, we lose all cruise info
				{
					FillDEDMatrix(4,15-(strlen(INSHead)), INSHead);
				}
				if(INSLine == 3)
				{
					FillDEDMatrix(4,8,"\x02",2);
					FillDEDMatrix(4,15,"\x02",2);
				}
				else
				{
					FillDEDMatrix(4,8," ");
					FillDEDMatrix(4,15," ");

				}
				int GroundSpeed = FloatToInt32((float)sqrt(cockpitFlightData.xDot*cockpitFlightData.xDot + 
					cockpitFlightData.yDot*cockpitFlightData.yDot) * FTPSEC_TO_KNOTS);
				FillDEDMatrix(4,20, "     ");
				sprintf(tempstr,"%d",GroundSpeed);
				FillDEDMatrix(4,17,"G/S");
				if(playerAC->INSState(AircraftClass::INS_HUD_FPM)) //28 Jul 04 - If INS off/failed, we lose all cruise info
					{
					FillDEDMatrix(4,(25-strlen(tempstr)),tempstr);
					}
			}
		}
	}
	else
	{
		FillDEDMatrix(0,5,"INS 08.0/10");
		AddSTPT(0,22);
		//Display some bogus INS info here, along with the current
		//coords of the plane
		latitude	= (FALCON_ORIGIN_LAT * FT_PER_DEGREE + cockpitFlightData.x) / EARTH_RADIUS_FT;
		cosLatitude = (float)cos(latitude);
		longitude	= ((FALCON_ORIGIN_LONG * DTR * EARTH_RADIUS_FT * cosLatitude) + cockpitFlightData.y) / (EARTH_RADIUS_FT * cosLatitude);
		
		latitude	*= RTD;
		longitude	*= RTD;
		
		longDeg		= FloatToInt32(longitude);
		longMin		= (float)fabs(longitude - longDeg) * DEG_TO_MIN;

		latDeg		= FloatToInt32(latitude);
		latMin		= (float)fabs(latitude - latDeg) * DEG_TO_MIN;

		// format lat/long here
		if(latMin < 10.0F) 
			sprintf(latStr, "LAT  N %3d* 0%2.2f\'\n", latDeg, latMin);
		else 
			sprintf(latStr, "LAT  N %3d* %2.2f\'\n", latDeg, latMin);
		if(longMin < 10.0F) 
			sprintf(longStr, "LNG  E %3d* 0%2.2f\'\n", longDeg, longMin);
		else 
			sprintf(longStr, "LNG  E %3d* %2.2f\'\n", longDeg, longMin);
		//Line 2
		FillDEDMatrix(1,5,latStr);
		//Line3
		FillDEDMatrix(2,5,longStr);
		
		//Line4
		FillDEDMatrix(3,4,"SALT");
		sprintf(tempstr,  "%dFT", (long)-cockpitFlightData.z);
		FillDEDMatrix(3,15,tempstr);
		//Line5
		FillDEDMatrix(4,3,"THDG  228.2*");
		int GroundSpeed = FloatToInt32((float)sqrt(cockpitFlightData.xDot*cockpitFlightData.xDot + 
			cockpitFlightData.yDot*cockpitFlightData.yDot) * FTPSEC_TO_KNOTS);
		sprintf(tempstr,"%d",GroundSpeed);
		FillDEDMatrix(4,16,"G/S");
		FillDEDMatrix(4,(24-strlen(tempstr)),tempstr);
	}
}	
void ICPClass::ExecEWSMode(void)
{
	//Line1
	FillDEDMatrix(0,6,"EWS  CONTROL");
	AddSTPT(0,22);
	//Line2
	FillDEDMatrix(1,2,"CH");
	PossibleInputs = 2;
	if(IsICPSet(ICPClass::CHAFF_BINGO))
		ScratchPad(1,7,10);
	else
	{
		sprintf(tempstr,"%d",ChaffBingo);
		FillDEDMatrix(1,(10-strlen(tempstr)), tempstr);
	} 
	FillDEDMatrix(1,13,"REQJAM");
	if(IsICPSet(ICPClass::EDIT_JAMMER))
	{
		FillDEDMatrix(1,20,"\x02",2);
		FillDEDMatrix(1,24,"\x02",2);
	}
	if(EWS_JAMMER_ON)
		FillDEDMatrix(1,21," ON");
	else
		FillDEDMatrix(1,21,"OFF");
	//Line3
	FillDEDMatrix(2,2,"FL");
	if(IsICPSet(ICPClass::FLARE_BINGO))
		ScratchPad(2,7,10);
	else
	{ 
		sprintf(tempstr,"%d",FlareBingo);
		FillDEDMatrix(2,(10-strlen(tempstr)),tempstr);
	} 
	FillDEDMatrix(2,13,"FDBK"); //JPG 12 Jun 04 - Ideally, it would be nice to turn this & REQCTR on/off to coincide w/
	FillDEDMatrix(2,21," ON");   //the Betty words, but it's more trouble than it's worth.
	//Line4
	FillDEDMatrix(3,2,"O1");
	FillDEDMatrix(3,9,"0");
	FillDEDMatrix(3,12,"REQCTR");
	FillDEDMatrix(3,21," ON");
	//Line5
	FillDEDMatrix(4,2,"O2");
	FillDEDMatrix(4,9,"0");
	FillDEDMatrix(4,13,"BINGO");
	if(IsICPSet(ICPClass::EWS_EDIT_BINGO))
	{
		FillDEDMatrix(4,20,"\x02",2);
		FillDEDMatrix(4,24,"\x02",2);	
	}
	if(EWS_BINGO_ON)
		FillDEDMatrix(4,21," ON");
	else
		FillDEDMatrix(4,21,"OFF");
}
void ICPClass::ChaffPGM(void)
{
	//Line1
	FillDEDMatrix(0,6,"CMDS  CHAFF  PGM");
	ShowChaffIndex(0,22);
	//Line2
	FillDEDMatrix(1,10,"BQ");
	if(BQ)
	{
		PossibleInputs = 2;
		ScratchPad(1,13,16);
	}
	else
	{
		if(iCHAFF_BQ[CPI] <= 0)
			sprintf(tempstr," 0");
		else
			sprintf(tempstr,"%2.0d",iCHAFF_BQ[CPI]);
		FillDEDMatrix(1,14,tempstr);
	}
	//Line3
	FillDEDMatrix(2,10,"BI");
	if(BI)
	{
		PossibleInputs = 5;
		ScratchPad(2,13,20);
	}
	else
	{
		sprintf(tempstr,"%1.3f",fCHAFF_BI[CPI]);
		FillDEDMatrix(2,15,tempstr);
	}
	//Line4
	FillDEDMatrix(3,10,"SQ");
	if(SQ)
	{
		PossibleInputs = 2;
		ScratchPad(3,13,16);
	}
	else
	{
		if(iCHAFF_SQ[CPI] <= 0)
			sprintf(tempstr," 0");
		else
			sprintf(tempstr,"%2.0d",iCHAFF_SQ[CPI]);
		FillDEDMatrix(3,14,tempstr);
	}
	//Line5
	FillDEDMatrix(4,10,"SI");
	if(SI)
	{
		PossibleInputs = 5;
		ScratchPad(4,13,19);
	}
	else
	{
		sprintf(tempstr,"%1.2f",fCHAFF_SI[CPI]);
		FillDEDMatrix(4,15,tempstr);
	}
}
void ICPClass::FlarePGM(void)
{
	//Line1
	FillDEDMatrix(0,6,"CMDS  FLARE  PGM");
	ShowFlareIndex(0,22);
	//Line2
	FillDEDMatrix(1,10,"BQ");
	if(BQ)
	{
		PossibleInputs = 2;
		ScratchPad(1,13,16);
	}
	else
	{
		if(iFLARE_BQ[FPI] <= 0)
			sprintf(tempstr," 0");
		else
			sprintf(tempstr,"%2.0d",iFLARE_BQ[FPI]);
		FillDEDMatrix(1,14,tempstr);
	}
	//Line3
	FillDEDMatrix(2,10,"BI");
	if(BI)
	{
		PossibleInputs = 5;
		ScratchPad(2,13,20);
	}
	else
	{
		sprintf(tempstr,"%1.3f",fFLARE_BI[FPI]);
		FillDEDMatrix(2,15,tempstr);
	}
	//Line4
	FillDEDMatrix(3,10,"SQ");
	if(SQ)
	{
		PossibleInputs = 2;
		ScratchPad(3,13,16);
	}
	else
	{
		if(iFLARE_SQ[FPI] <= 0)
			sprintf(tempstr," 0");
		else
			sprintf(tempstr,"%2.0d",iFLARE_SQ[FPI]);
		FillDEDMatrix(3,14,tempstr);
	}
	//Line5
	FillDEDMatrix(4,10,"SI");
	if(SI)
	{
		PossibleInputs = 5;
		ScratchPad(4,13,19);
	}
	else
	{
		sprintf(tempstr,"%1.2f",fFLARE_SI[FPI]);
		FillDEDMatrix(4,15,tempstr);
	}
}
void ICPClass::ExecMODEMode(void)
{
	//Line1
	FillDEDMatrix(0,10,"MODE");
	ScratchPad(0,15,19);
	AddSTPT(0,22);
}
void ICPClass::UpdateMODEMode(void)
{
	if(IN_AA && mICPPrimaryMode != AA_MODE)
	{
		CPButtonObject* pButton = OTWDriver.pCockpitManager->GetButtonPointer( ICP_AA_BUTTON_ID );
		SimICPAA1(ICP_AA_BUTTON_ID, KEY_DOWN, pButton );
	}
	else if(IN_AG && mICPPrimaryMode != AG_MODE)
	{
		CPButtonObject* pButton = OTWDriver.pCockpitManager->GetButtonPointer( ICP_NAV_BUTTON_ID );
		SimICPAG1(ICP_AG_BUTTON_ID, KEY_DOWN, pButton );
	}
	else
	{
		CPButtonObject* pButton = OTWDriver.pCockpitManager->GetButtonPointer( ICP_NAV_BUTTON_ID );
		SimICPNav1(ICP_NAV_BUTTON_ID, KEY_DOWN, pButton );
	}
	ExecMODEMode();
}
void ICPClass::ExecVRPMode(void)
{
	//Line1
	FillDEDMatrix(0,9,"TGT-TO-VRP");
	//Line2
	FillDEDMatrix(1,8,"TGT   9 \x01");
	//Line3
	FillDEDMatrix(2,8,"TBRG");
	if(VRP_BRG)
	{
		PossibleInputs = 4;
		ScratchPad(2,13,20);
	}
	else
	{
		sprintf(tempstr,"%3.1f'",fVRP_BRG);
		FillDEDMatrix(2,20-strlen(tempstr),tempstr);
	}
	//Line4
	FillDEDMatrix(3,9,"RNG");
	if(VRP_RNG)
	{
		PossibleInputs = 6;
		ScratchPad(3,13,22);
	}
	else
	{
		sprintf(tempstr,"%dFT", iVRP_RNG);
		FillDEDMatrix(3,22-strlen(tempstr),tempstr);
	}
	//Line5
	FillDEDMatrix(4,8,"ELEV");
	if(VRP_ALT)
	{
		PossibleInputs = 5;
		ScratchPad(4,13,21);
	}
	else
	{
		sprintf(tempstr,"%dFT", iVRP_ALT);
		FillDEDMatrix(4,(21-strlen(tempstr)),tempstr);
	}
}
void ICPClass::ExecINTGMode(void)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if(!g_bIFF)
	{
		//Line1
		FillDEDMatrix(0,1,"INTG ON");
		FillDEDMatrix(0,18,"CPL");
		AddSTPT(0,22);
		//Line3
		FillDEDMatrix(2,2,"M1",2);
		FillDEDMatrix(2,5,"13");
		FillDEDMatrix(2,10,"MC",2);
		FillDEDMatrix(2,14,"(5)");
		FillDEDMatrix(2,18,"\x02",2);
		FillDEDMatrix(2,24,"\x02",2);
		//Line4
		FillDEDMatrix(3,2,"M2");
		FillDEDMatrix(3,5,"1234");
		FillDEDMatrix(3,10,"M4");
		FillDEDMatrix(3,13,"A(6)");
		//Line5
		FillDEDMatrix(4,2,"M3",2);
		FillDEDMatrix(4,5,"4000");
		FillDEDMatrix(4,17,"RST  (7)");
		return;
	}
	//Line1
	if(playerAC && !playerAC->HasPower(AircraftClass::IFFPower))
		FillDEDMatrix(0,1,"INTG OFF");
	else
		FillDEDMatrix(0,1,"INTG ON");
	FillDEDMatrix(0,18,"CPL");
	AddSTPT(0,22);
	//Line3
	if(IsIFFSet(ICPClass::MODE_1))
		FillDEDMatrix(2,2,"M1",2);
	else
		FillDEDMatrix(2,2,"M1");

	sprintf(tempstr, "%d", Mode1Code);
	FillDEDMatrix(2,5, tempstr);
	if(IsIFFSet(ICPClass::MODE_C))
		FillDEDMatrix(2,10,"MC",2);
	else
		FillDEDMatrix(2,10,"MC");

	FillDEDMatrix(2,14,"(5)");
	ScratchPad(2,18,24);
	PossibleInputs = 5;
	//Line4
	if(IsIFFSet(ICPClass::MODE_2))
		FillDEDMatrix(3,2,"M2",2);
	else
		FillDEDMatrix(3,2,"M2");

	sprintf(tempstr,"%d", Mode2Code);
	FillDEDMatrix(3,5,tempstr);
	if(IsIFFSet(ICPClass::MODE_4))
		FillDEDMatrix(3,10,"M4",2);
	else
		FillDEDMatrix(3,10,"M4");

	if(IsIFFSet(ICPClass::MODE_4B))
		FillDEDMatrix(3,13,"B(6)");
	else
		FillDEDMatrix(3,13,"A(6)");
	//Line5
	if(IsIFFSet(ICPClass::MODE_3))
		FillDEDMatrix(4,2,"M3",2);
	else
		FillDEDMatrix(4,2,"M3");

	sprintf(tempstr, "%d", Mode3Code);
	FillDEDMatrix(4,5,tempstr);
	FillDEDMatrix(4,17,"RST  (7)");
}
