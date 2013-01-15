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
#include "sms.h"

#define PosA		0
#define PosB		1
#define PosC		7
#define PosD		8
#define PosE		13
#define PosF		14
#define PosG		19
#define PosH		20

extern "C" double getVar(double lat,double lon,double elev,double year);

void ICPClass::ExecMISCMode(void)
{
	//Line1
	FillDEDMatrix(0,13,"MISC");
	//Line2
	FillDEDMatrix(1,PosA,"1",2);
	FillDEDMatrix(1,PosB,"CORR");
	FillDEDMatrix(1,PosC,"2",2);
	FillDEDMatrix(1,PosD,"MAGV");
	FillDEDMatrix(1,PosE,"3",2);
	FillDEDMatrix(1,PosF,"OFP");
	FillDEDMatrix(1,PosG,"R",2);
	//Line3
	FillDEDMatrix(2,PosA,"4",2);
	FillDEDMatrix(2,PosB,"INSM");
	FillDEDMatrix(2,PosC,"5",2);
	FillDEDMatrix(2,PosD,"LASR");
	FillDEDMatrix(2,PosE,"6",2);
	FillDEDMatrix(2,PosF,"GPS");
	FillDEDMatrix(2,PosG,"E",2);
	//Line4
	FillDEDMatrix(3,PosA,"7",2);
	FillDEDMatrix(3,PosB,"DRNG");
	FillDEDMatrix(3,PosC,"8",2);
	FillDEDMatrix(3,PosD,"BULL");
	FillDEDMatrix(3,PosE,"9",2);
	FillDEDMatrix(3,PosF,"WPT");
	FillDEDMatrix(3,PosG,"0",2);
	if(CheckForHARM())
		FillDEDMatrix(3,PosH,"HARM");
	else
		FillDEDMatrix(3,PosH,"    ");

}
void ICPClass::ExecCORRMode(void)
{
	//Line1
	FillDEDMatrix(0,10,"CORR  HUD1");
	//Line2
	FillDEDMatrix(2,2,"1");
	FillDEDMatrix(2,5,"\x02",2);
	FillDEDMatrix(2,7,"18157");
	FillDEDMatrix(2,13,"\x02",2);
	FillDEDMatrix(2,15,"4");
	FillDEDMatrix(2,20,"23567");
	//Line3
	FillDEDMatrix(2,2,"2");
	FillDEDMatrix(2,8,"2153");
	FillDEDMatrix(2,15,"5");
	FillDEDMatrix(2,20,"10029");
	//Line4
	FillDEDMatrix(3,2,"3");
	FillDEDMatrix(2,8,"6251");
}
void ICPClass::ExecMAGVMode(void)
{
	latitude	= (FALCON_ORIGIN_LAT * FT_PER_DEGREE + cockpitFlightData.x) / EARTH_RADIUS_FT;
	cosLatitude = (float)cos(latitude);
	longitude	= ((FALCON_ORIGIN_LONG * DTR * EARTH_RADIUS_FT * cosLatitude) + cockpitFlightData.y) / (EARTH_RADIUS_FT * cosLatitude);
	latitude	*= RTD;
	longitude	*= RTD;
	/****************************************************************
	*Latitude, degrees, north positive, 							*
	*Longitude, degrees, east positive (for 100W, enter -100),		*
	*Elevation, km above MSL",										*
	*Year, as a floating point number (enter 1994.5 for June '94),	*
	/***************************************************************/
	float MV = static_cast<float>(getVar(latitude, longitude, 0, 2001.0));
	//Line2
	FillDEDMatrix(0,10,"MAGV  AUTO");
	//Line3
	if(MV < 0)
	{
		sprintf(tempstr, "E %1.1f", -MV );
		FillDEDMatrix(2,12,tempstr);
	}
	else
	{
		sprintf(tempstr, "W %1.1f", MV);
		FillDEDMatrix(2,12,tempstr);
	}
}
void ICPClass::ExecOFPMode(void)
{
	//Line 1
	FillDEDMatrix(0,13,"OFP1");
	//Line 2
	FillDEDMatrix(1,3,"UFC  P07A   FCR  7010");
	//Line3
	FillDEDMatrix(2,3,"MFD  P07A   FCC  P07B");
	//Line4
	FillDEDMatrix(3,3,"SMS  P07B   DTE  P010");
	//Line5
	FillDEDMatrix(4,3,"FDR  P30A   HUD  002e");
}
void ICPClass::ExecINSMMode(void)
{
	//Line 1
	FillDEDMatrix(0,10,"INSM");
	//Line 3
	FillDEDMatrix(2,9,"\x02",2);
	FillDEDMatrix(2,14,"\x02",2);
}
void ICPClass::ExecLASRMode(void)
{
	//Line 1
	FillDEDMatrix(0,12,"LASER");
	//Line3
	FillDEDMatrix(2,3,"LASER CODE");
	//FillDEDMatrix(2,17,"\x02",2);
	if(LaserLine == 1)
	{
		PossibleInputs = 4;
		ScratchPad(2,17,22);
	}
	else
	{
		sprintf(tempstr, "%d", LaserCode);
		FillDEDMatrix(2,18,tempstr);
	}
	//FillDEDMatrix(2,22,"\x02",2);
	//Line5
	FillDEDMatrix(4,3,"LASER ST TIME");
	FillDEDMatrix(4,22,"SEC");
	if(LaserLine == 2)
	{
		PossibleInputs = 3;
		ScratchPad(4,16,20);
	}
	else
	{
		sprintf(tempstr, "%d", LaserTime);
		FillDEDMatrix(4, 21 - (strlen(tempstr)), tempstr);
	}
}
void ICPClass::ExecGPSMode(void)
{
	char tempstr[20] = "";
	//Line1
	FillDEDMatrix(0,1,"GPS  INIT1");
	FillDEDMatrix(0,12,"\x02",2);
	FillDEDMatrix(0,13,"DISPL/ENTR\x02",2);
	//Line2
	FormatTime(vuxGameTime / 1000, tempstr);
	FillDEDMatrix(1,6,"TIME");
	FillDEDMatrix(1,12,tempstr);
	//Line3
	FillDEDMatrix(2,2,"MM/DD/YY");			
//	FillDEDMatrix(2,12,"12/23/00");			//Wombat778 10-18-2003 Removed to make date live.  Date is current date/time

//  Calculate the date
	SYSTEMTIME time;
	GetLocalTime(&time);					//Wombat778 10-18-2003 is this going to hurt performance?  It doesnt really need to balculated every frame
	int temptime = time.wYear;
	
	if (temptime>1999)		//convert the date to 2 digits. This wont work > 2100 but falcon wont be around by then;-)
		temptime-=2000;
	else
		temptime-=1900;

	sprintf (tempstr, "%02d/%02d/%02d", time.wMonth, time.wDay, temptime);
	FillDEDMatrix(2,12,tempstr);

//	end of date code

	//Line4
	FillDEDMatrix(3,7,"G/S");
//	FillDEDMatrix(3,15,"2KTS");  //Wombat778 10-18-2003 Removed to make speed live. Added following instead

//  calculate the speed

	int GroundSpeed = FloatToInt32((float)sqrt(cockpitFlightData.xDot*cockpitFlightData.xDot + 
	cockpitFlightData.yDot*cockpitFlightData.yDot) * FTPSEC_TO_KNOTS);
	sprintf(tempstr,"%03d",GroundSpeed);	
	FillDEDMatrix(3,(15-strlen(tempstr)),tempstr);

//	end of speed code

	//Line5
	FillDEDMatrix(4,6,"MHDG");	
	//FillDEDMatrix(4,12,"003*");			//Wombat778 10-18-2003 Removed to make heading live

//calculate the heading
	int tempheading = FloatToInt32((cockpitFlightData.yaw * 10.0F * RTD)/10.0f);
	if (tempheading < 0) tempheading += 360;
	sprintf(tempstr, "%03d*",tempheading);
	FillDEDMatrix(4,12,tempstr);	
//end of heading code

}
void ICPClass::ExecDRNGMode(void)
{
	//Line1
	FillDEDMatrix(0,8,"DRNG");
	AddSTPT(0,22);
	//Line3
	FillDEDMatrix(2,5,"X");
	FillDEDMatrix(2,7,"\x02",2);
	FillDEDMatrix(2,11,"0FT");
	FillDEDMatrix(2,14,"\x02",2);
	FillDEDMatrix(2,20,"SHT");
	//Line4
	FillDEDMatrix(3,5,"Y");
	FillDEDMatrix(3,11,"0FT");
	FillDEDMatrix(3,20,"LFT");
}
void ICPClass::ExecBullMode(void)
{
	//Line1
	FillDEDMatrix(0,8,"\x02",2);
	if(ShowBullseyeInfo)
		FillDEDMatrix(0,9,"BULLSEYE",2);
	else
		FillDEDMatrix(0,9,"BULLSEYE");
	FillDEDMatrix(0,17,"\x02",2);
	//Line2
	FillDEDMatrix(1,9,"BULL");
	FillDEDMatrix(1,14,"1 \x01");
}
void ICPClass::ExecWPTMode(void)
{
	//Line1
	FillDEDMatrix(0,5,"\x02",2);
	FillDEDMatrix(0,6,"TGT-TO-WPT");
	FillDEDMatrix(0,16,"\x02",2);
	FillDEDMatrix(0,18,"RBL");
	//Line2
	FillDEDMatrix(1,5,"TGT");
	FillDEDMatrix(1,10,"18\x01");
	//Line3
	FillDEDMatrix(2,4,"TBRG");
	FillDEDMatrix(2,10,"075*");
	//Line4
	FillDEDMatrix(3,5,"RNG");
	FillDEDMatrix(3,12,"15.0 NM");
}
void ICPClass::ExecHARMMode(void)
{
	//Line1
	FillDEDMatrix(0,1,"HARM TBL1");
	FillDEDMatrix(0,11,"\x01");
	FillDEDMatrix(0,15,"T1");
	FillDEDMatrix(0,20,"206");
	//Line2
	FillDEDMatrix(1,15,"T2");
	FillDEDMatrix(1,20,"208");
	//Line3
	FillDEDMatrix(2,15,"T3");
	FillDEDMatrix(2,19,"\x02",2);
	FillDEDMatrix(2,20,"MN2");
	FillDEDMatrix(2,23,"\x02",2);
	//Line4
	FillDEDMatrix(3,15,"T4");
	FillDEDMatrix(3,20,"210");
	//Line5
	FillDEDMatrix(4,1,"SEQ=MN2");
	FillDEDMatrix(4,15,"T5");
	FillDEDMatrix(4,20,"308");
}

BOOL ICPClass::CheckForHARM(void)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if(!playerAC || !playerAC->Sms)
		return FALSE;

	if(playerAC->Sms->curWeapon && playerAC->Sms->curWeaponClass == wcHARMWpn)
		return TRUE;
	else
		return FALSE;

	return FALSE;
}
/*			curWeapon && curWeaponClass == wcHARMWpn
	sms->hardPoint[hp] && sms->hardPoint[hp]->weaponPointer && sms->hardPoint[hp]->GetWeaponType() == wtAgm88)
				hasHARM += sms->hardPoint[hp]->weaponCount;
}*/
	// Retro 3Jan2004 from here...
#include "falcsnd\winampfrontend.h"
extern bool g_bPilotEntertainment;	// Retro 3Jan2004

void ICPClass::ExecWinAmpMode(void)
{

	if (!g_bPilotEntertainment)
	{
		ShiAssert(false);	// we really shouldn´t be here then !
		return;
	}

	if (!winamp)
		return;

	winamp->Refresh(vuxRealTime);	// have to use real timer here, in case of pause etc..

	ClearStrings();

	//Line1
	FillDEDMatrix(0,1,"BMS Crew Entertainment");
	//Line3
	char* title = winamp->getDEDTitle(0);
	if (title)
		FillDEDMatrix(2,1,title);
	//Line4
	title = winamp->getDEDTitle(1);
	if (title)
		FillDEDMatrix(3,1,title);

	//Line5
	FillDEDMatrix(4,2,"Volume");
	FillDEDMatrix(4,10,"\x02",2);
	char tmp[10];
	sprintf(tmp,"%3i",winamp->getVolume());
	FillDEDMatrix(4,12,tmp);
	FillDEDMatrix(4,16,"\x02",2);
}
// Retro 3Jan2004 end..
