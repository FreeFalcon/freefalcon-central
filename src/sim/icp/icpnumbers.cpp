#include "icp.h"
#include "simdrive.h"
#include "aircrft.h"
#include "navsystem.h"
#include "fcc.h"

void ICPClass::OneButton(int mode)
{
	ClearStrings();
	mICPSecondaryMode = mode;
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

	if(IsICPSet(ICPClass::MODE_LIST))
	{
		//Set our LastMode
		LastMode = LIST_MODE;
		//DEST PAGE
		SetICPFlag(ICPClass::EDIT_STPT);
		SetICPFlag(ICPClass::EDIT_LAT);
		playerAC->FCC->waypointStepCmd = 127;
		ExecDESTMode();
	}
	else if(IsICPSet(ICPClass::MISC_MODE))
	{
		//Set our LastMode
		LastMode = LIST_MODE;
		ExecCORRMode();
		//Once we got here, only modechange will get us back
		SetICPFlag(ICPClass::BLOCK_MODE);
	}
	else
	{
		//ILS PAGE
		LeaveCNIPage();
		ExecILSMode();
	} 
}
void ICPClass::TwoButton(int mode)
{
	ClearStrings();
	mICPSecondaryMode = mode;
	if(IsICPSet(ICPClass::MODE_LIST))
	{
		LastMode = LIST_MODE;
		//BINGO PAGE
		SetICPFlag(ICPClass::EDIT_STPT);
		ExecBingo();
	}
	else if(IsICPSet(ICPClass::MISC_MODE))
	{
		//Set our LastMode
		LastMode = LIST_MODE;
		//MAGV PAGE
		ExecMAGVMode();
		//Once we got here, only modechange will get us back
		SetICPFlag(ICPClass::BLOCK_MODE);
	}
	else
	{
		LeaveCNIPage();
		SetICPFlag(ICPClass::EDIT_STPT);
		//ALOW PAGE
		ExecALOWMode();
	}
}
void ICPClass::ThreeButton(int mode)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	mICPSecondaryMode = mode;

	if(IsICPSet(ICPClass::MODE_CNI))
	{
		//DLINK page
		ClearStrings();
		LeaveCNI();
		ClearFlags();
		SetICPFlag(ICPClass::MODE_DLINK);
		playerAC->FCC->SetStptMode(FireControlComputer::FCCDLinkpoint);
		playerAC->FCC->waypointStepCmd = 127;
		ExecDLINKMode();
		//Once we got here, only modechange will get us back
		SetICPFlag(ICPClass::BLOCK_MODE);
	}	
	else if(IsICPSet(ICPClass::MODE_LIST))
	{
		ClearStrings();
		mICPSecondaryMode = mode;
		//Set our LastMode
		LastMode = LIST_MODE;
		//VIP PAGE
		VIP_BRG = TRUE;
		ExecVIPMode();
	}
	else if(IsICPSet(ICPClass::MISC_MODE))
	{
		ClearStrings();
		//Set our LastMode
		LastMode = LIST_MODE;
		//OFP PAGE
		ExecOFPMode();
		//Once we got here, only modechange will get us back
		SetICPFlag(ICPClass::BLOCK_MODE);
	}
}
void ICPClass::FourButton(int mode)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	ClearStrings();
	mICPSecondaryMode = mode;
	if(IsICPSet(ICPClass::MODE_LIST))
	{
		//Set our LastMode
		LastMode = LIST_MODE;
		SetICPFlag(ICPClass::EDIT_STPT);
		//NAV PAGE
		ExecNAVMode();
		//Once we got here, only modechange will get us back
		SetICPFlag(ICPClass::BLOCK_MODE);
	}
	else if(IsICPSet(ICPClass::MISC_MODE))
	{
		//Set our LastMode
		LastMode = LIST_MODE;
		//INSM PAGE
		ExecINSMMode();
		//Once we got here, only modechange will get us back
		SetICPFlag(ICPClass::BLOCK_MODE);
	}
	else
	{
		//STPT PAGE
		LeaveCNIPage();
		SetICPFlag(ICPClass::EDIT_STPT);
		playerAC->FCC->SetStptMode(FireControlComputer::FCCWaypoint);
		playerAC->FCC->waypointStepCmd = 127;
	}
}
void ICPClass::FifeButton(int mode)
{
	ClearStrings();
	mICPSecondaryMode = mode;
	if(IsICPSet(ICPClass::MODE_LIST))
	{
		//Set our LastMode
		LastMode = LIST_MODE;
		//MAN PAGE
		ExecMANMode();
	}
	else if(IsICPSet(ICPClass::MISC_MODE))
	{
		//Set our LastMode
		LastMode = LIST_MODE;
		//LASR PAGE
		ExecLASRMode();
		//Once we got here, only modechange will get us back
		//SetICPFlag(ICPClass::BLOCK_MODE);
	}
	else
	{
		LeaveCNIPage();
		//Only set this when not here
		if(!Cruise_HOME && !Cruise_RNG && !Cruise_TOS)
			SetICPFlag(ICPClass::EDIT_STPT);
		//CRUS PAGE
		ExecCRUSMode();
	}
}
void ICPClass::SixButton(int mode)
{
	ClearStrings();
	mICPSecondaryMode = mode;
	if(IsICPSet(ICPClass::MODE_LIST))
	{
		//Set our LastMode
		LastMode = LIST_MODE;
		SetICPFlag(ICPClass::EDIT_STPT);
		//INS PAGE
		ExecINSMode();
	}
	else if(IsICPSet(ICPClass::MISC_MODE))
	{
		LastMode = LIST_MODE;
		ExecGPSMode();
		//Once we got here, only modechange will get us back
		SetICPFlag(ICPClass::BLOCK_MODE);
	}
	else
	{
		LeaveCNIPage();
		ExecTimeMode();
		//Once we got here, only modechange will get us back
		SetICPFlag(ICPClass::BLOCK_MODE);
	}
}
void ICPClass::SevenButton(int mode)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	ClearStrings();
	mICPSecondaryMode = mode;
	if(IsICPSet(ICPClass::MODE_LIST))
	{
		//Set our LastMode
		LastMode = LIST_MODE;
		mICPSecondaryMode = EWS_MODE;
		SetICPFlag(ICPClass::CHAFF_BINGO);
		ClearICPFlag(ICPClass::EDIT_STPT);
		ExecEWSMode();
	}
	else if(IsICPSet(ICPClass::MISC_MODE))
	{
		LastMode = LIST_MODE;
		ExecDRNGMode();
		//Once we got here, only modechange will get us back
		SetICPFlag(ICPClass::BLOCK_MODE);
	}
	else
	{
		//MKPT PAGE
		LeaveCNIPage();
		playerAC->FCC->SetStptMode(FireControlComputer::FCCMarkpoint);
		playerAC->FCC->waypointStepCmd = 127;
		ExecMARKMode();
		//Once we got here, only modechange will get us back
		SetICPFlag(ICPClass::BLOCK_MODE);
	}
}
void ICPClass::EightButton(int mode)
{
	ClearStrings();
	mICPSecondaryMode = mode;
	if(IsICPSet(ICPClass::MODE_LIST))
	{
		//Set our LastMode
		LastMode = LIST_MODE;
		//MODE PAGE
		SetICPFlag(ICPClass::EDIT_STPT);
		ExecMODEMode();
	}
	else if(IsICPSet(ICPClass::MISC_MODE))
	{
		LastMode = LIST_MODE;
		ExecBullMode();
		//Once we got here, only modechange will get us back
		SetICPFlag(ICPClass::BLOCK_MODE);
	}
	else
	{
		//Fix page, called directly
		LeaveCNIPage();
		SetICPFlag(ICPClass::EDIT_STPT);
		//Once we got here, only modechange will get us back
		SetICPFlag(ICPClass::BLOCK_MODE);
	}
}
void ICPClass::NineButton(int mode)
{
	ClearStrings();
	if(IsICPSet(ICPClass::MODE_LIST))
	{
		ClearStrings();
		mICPSecondaryMode = mode;
		//Set our LastMode
		LastMode = LIST_MODE;
		//VIP PAGE
		VRP_BRG = TRUE;
		LastMode = LIST_MODE;
		ExecVRPMode();
	}
	else if(IsICPSet(ICPClass::MISC_MODE))
	{
		LastMode = LIST_MODE;
		ExecWPTMode();
		//Once we got here, only modechange will get us back
		SetICPFlag(ICPClass::BLOCK_MODE);
	}
	else
	{
		//ACAL page, called directly
		LeaveCNIPage();
		SetICPFlag(ICPClass::EDIT_STPT);
		//Once we got here, only modechange will get us back
		SetICPFlag(ICPClass::BLOCK_MODE);
	}

	mICPSecondaryMode = mode;
}
void ICPClass::ZeroButton(int mode)
{
	ClearStrings();
	if(IsICPSet(ICPClass::MODE_LIST))
	{
		if(IsICPSet(ICPClass::MODE_LIST) && mICPSecondaryMode == EIGHT_BUTTON)
			UpdateMODEMode();
		else
		{
			//Set our LastMode
			LastMode = LIST_MODE;
			//MISC PAGE
			SetICPFlag(ICPClass::MISC_MODE);
			ClearICPFlag(ICPClass::MODE_LIST);
			ExecMISCMode();
		}
	}
	else if(IsICPSet(ICPClass::MISC_MODE))
	{
extern bool g_bPilotEntertainment;	// Retro 3Jan2004

		// ORIGINAL STUFF	// Retro 3Jan2004
		if (g_bPilotEntertainment == false)
		{
			if(!CheckForHARM())
				return;

			LastMode = LIST_MODE;
			ExecHARMMode();

			mICPSecondaryMode = mode;
			//Once we got here, only modechange will get us back
			SetICPFlag(ICPClass::BLOCK_MODE);
		}
		// RETRO STUFF	// Retro 3Jan2004
		else
		{
			LastMode = LIST_MODE;
			ClearICPFlag(ICPClass::EDIT_STPT);
			ExecWinAmpMode();

			mICPSecondaryMode = mode;
			//Once we got here, only modechange will get us back
			SetICPFlag(ICPClass::BLOCK_MODE);
		}
	}
	//ILS
	else if(IsICPSet(ICPClass::MODE_CNI) && mICPSecondaryMode == ONE_BUTTON)
	{
		if(ILSPageSel == 1)
		{
			if(CMDSTRG)
				CMDSTRG = FALSE;
			else
				CMDSTRG = TRUE;
		}
	}
}
void ICPClass::ExecTimeMode(void)
{
	ClearStrings();
	//Line1
	FillDEDMatrix(0,13,"TIME");
	//Line3
	FillDEDMatrix(2,5,"SYSTEM");
	// Format Line 2: Current Time
	FormatTime(vuxGameTime / 1000, timeStr);		// Get game time and convert to secs
	sprintf(tempstr,"%8s",timeStr);
	FillDEDMatrix(2,13,"\x02",2);
	FillDEDMatrix(2,14,tempstr);
	FillDEDMatrix(2,22,"\x02",2);
	//Line4
	FillDEDMatrix(3,7,"HACK");
	if(running)
	{
		Difference = (vuxGameTime - Start);
		FormatTime(Difference / 1000, tempstr);
		FillDEDMatrix(3,14,tempstr);
	}
	else if(stopped)
	{
		FormatTime(Difference / 1000, tempstr);
		FillDEDMatrix(3,14,tempstr);
	}
	else
	{
		FormatTime(Difference / 1000, tempstr);
		FillDEDMatrix(3,14,tempstr);
	}
	FillDEDMatrix(3,23,"\x01");
	//Line5
	FillDEDMatrix(4,2,"DELTA TOS");
	FillDEDMatrix(4,14,"00:00:00");
}
void ICPClass::ExecFIXMode(void)
{
	//Line1
	FillDEDMatrix(0,10,"FIX");
	FillDEDMatrix(0,14,"\x02",2);
	FillDEDMatrix(0,15,"OFLY\x02",2);
	//Line2
	FillDEDMatrix(1,9,"STPT");
	AddSTPT(1,14);
	//Line3
	FillDEDMatrix(2,8,"DELTA");
	FillDEDMatrix(2,17,"0.1NM");
	//Line4
	FillDEDMatrix(3,7,"SYS ACCUR");
	FillDEDMatrix(3,18,"HIGH");
	//Line5
	FillDEDMatrix(4,7,"GPS ACCUR");
	FillDEDMatrix(4,18,"HIGH");
}
void ICPClass::ExecACALMode(void)
{
	//Line1
	FillDEDMatrix(0,1,"ACAL");
	FillDEDMatrix(0,6,"\x02",2);
	FillDEDMatrix(0,7,"RALT\x02",2);
	FillDEDMatrix(0,13,"ALT");
	AddSTPT(0,22);
	//Line2
	FillDEDMatrix(1,8,"MAN");
	//Line3
	FillDEDMatrix(2,8,"ELEV");
	FillDEDMatrix(2,16,"512FT");
	//Line4
	FillDEDMatrix(3,2,"ALT DELTA");
	FillDEDMatrix(3,17,"78FT");
	//Line5
	FillDEDMatrix(4,2,"POS DELTA");
	FillDEDMatrix(4,17,"0.0NM");
}
