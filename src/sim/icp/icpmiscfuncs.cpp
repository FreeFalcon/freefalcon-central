#include "icp.h"
#include "simdrive.h"
#include "aircrft.h"
#include "navsystem.h"
#include "fcc.h"
#include "hud.h"
#include "weather.h"
#include "find.h"
#include "flightdata.h"

#define S_IN_M		60
#define S_IN_H		3600

extern bool g_bIFF;

void ICPClass::PushedSame(int LastMode)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	ClearStrings();
	ClearFlags();
	ResetSubPages();
	Manual_Input = FALSE;
	ClearCount = 0;
	InputsMade = 0;
	MadeInput = FALSE;
	playerAC->FCC->SetStptMode(FireControlComputer::FCCWaypoint);
	playerAC->FCC->waypointStepCmd = 127;
	switch (LastMode)
	{
	case NONE_MODE:
		mICPTertiaryMode = CNI_MODE;
		SetICPFlag(ICPClass::MODE_CNI);
		SetICPFlag(ICPClass::EDIT_STPT);
		ExecCNIMode();
		break;
	case CNI_MODE:
		mICPTertiaryMode = CNI_MODE;
		SetICPFlag(ICPClass::MODE_CNI);
		SetICPFlag(ICPClass::EDIT_STPT);
		ExecCNIMode();
		break;
	case COMM1_MODE:
		mICPTertiaryMode = COMM1_MODE;
		SetICPFlag(ICPClass::MODE_COMM1);
		ExecCOMM1Mode();
		break;
	case COMM2_MODE:
		mICPTertiaryMode = COMM2_MODE;
		SetICPFlag(ICPClass::MODE_COMM2);
		ExecCOMM2Mode();
		break;
	case LIST_MODE:
		mICPTertiaryMode = LIST_MODE;
		SetICPFlag(ICPClass::MODE_LIST);
		mICPSecondaryMode = 0;
		ExecLISTMode();
		break;
	case IFF_MODE:
		mICPTertiaryMode = IFF_MODE;
		SetICPFlag(ICPClass::MODE_IFF);
		ExecIFFMode();
		break;
	default:
		break;
	}
}
int ICPClass::ManualInput(void)
{
	if(Manual_Input)
		return TRUE;
	if((IsICPSet(ICPClass::MODE_CNI) && mICPSecondaryMode == ONE_BUTTON &&
		ILSPageSel == 0) ||
		(IsICPSet(ICPClass::MODE_CNI) && mICPSecondaryMode == TWO_BUTTON) ||
		(IsICPSet(ICPClass::MODE_CNI) && mICPSecondaryMode == FOUR_BUTTON)||
		(IsICPSet(ICPClass::MODE_LIST) && mICPSecondaryMode == ONE_BUTTON) ||
		(IsICPSet(ICPClass::EDIT_LAT)) || (IsICPSet(ICPClass::EDIT_LONG)) ||
		(IsICPSet(ICPClass::FLARE_BINGO)) || (IsICPSet(ICPClass::CHAFF_BINGO)) ||
		(BQ || BI || SQ || SI || OA1 || OA2) ||
		(IsICPSet(ICPClass::MODE_LIST) && mICPSecondaryMode == TWO_BUTTON) ||
		(IsICPSet(ICPClass::MODE_LIST) && mICPSecondaryMode == THREE_BUTTON) ||
		(IsICPSet(ICPClass::MODE_LIST) && mICPSecondaryMode == FIFE_BUTTON) ||
		(IsICPSet(ICPClass::MODE_LIST) && mICPSecondaryMode == NINE_BUTTON) ||
		(IsICPSet(ICPClass::MODE_LIST) && mICPSecondaryMode == 100) || //INTG
		(IsICPSet(ICPClass::MODE_LIST) && mICPSecondaryMode == SIX_BUTTON) || //INS
		IsICPSet(ICPClass::MODE_IFF) || (IsICPSet(ICPClass::MISC_MODE) && 
			mICPSecondaryMode == FIFE_BUTTON))
	{
		if(OA1 && OA_BRG)
			tempvar1 = fOA_BRG;
		else if(OA2 && OA_BRG)
			tempvar1 = fOA_BRG2;
		if(VIP_BRG)
			tempvar1 = fVIP_BRG;
		if(VRP_BRG)
			tempvar1 = VRP_BRG;
		if(PGMChaff)
		{
			if(BI)
				tempvar1 = fCHAFF_BI[CPI];
			else if(SI)
				tempvar1 = fCHAFF_SI[CPI];
		}
		if(PGMFlare)
		{
			if(BI)
				tempvar1 = fFLARE_BI[FPI];
			else if(SI)
				tempvar1 = fFLARE_SI[FPI];
		}

		return TRUE;
	}
	else
		return FALSE;
}
int ICPClass::CheckMode(void)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if((IsICPSet(ICPClass::MODE_CNI) && mICPSecondaryMode == FIFE_BUTTON) && !IsICPSet(ICPClass::BLOCK_MODE))
	{
		StepCruise();
		return TRUE;
	}
	if(IsICPSet(ICPClass::MODE_LIST) && mICPSecondaryMode == EWS_MODE && !EWSMain)
	{
		//you can only change progs if you're in STBY
		if(playerAC && 
			playerAC->EWSPGM() != AircraftClass::EWSPGMSwitch::Stby)
			return TRUE;
	}
	
	if(!g_bIFF)
	{
		//Don't do anything for now when in COMM (temporary)/IFF/DLINK/FACK/MARK/INTG mode
		if(IsICPSet(ICPClass::MODE_COMM1) || IsICPSet(ICPClass::MODE_COMM2) ||
			IsICPSet(ICPClass::MODE_IFF) || IsICPSet(ICPClass::MODE_DLINK) ||
			IsICPSet(ICPClass::MODE_FACK) || IsICPSet(ICPClass::BLOCK_MODE) ||
			(IsICPSet(ICPClass::MODE_LIST) && mICPSecondaryMode == 100) ||	//INTG
			mICPSecondaryMode == MARK_MODE)
			return TRUE;
		else
			return FALSE;
	}
	else
	{
		//Don't do anything for now when in COMM (temporary)/IFF/DLINK/FACK/MARK/INTG mode
		if(IsICPSet(ICPClass::MODE_COMM1) || IsICPSet(ICPClass::MODE_COMM2) ||
			IsICPSet(ICPClass::MODE_DLINK) || IsICPSet(ICPClass::MODE_FACK) || 
			IsICPSet(ICPClass::BLOCK_MODE) ||
			mICPSecondaryMode == MARK_MODE)
			return TRUE;
		else
			return FALSE;
	}
}
void ICPClass::HandleManualInput(int button)
{
	//If we blocked our input, return
	if(IsICPSet(ICPClass::BLOCK_MODE))
		return;

	if(InputsMade == PossibleInputs)
		return;

	//Only shift our Input if we have something != 0
	if(MadeInput)
	{
		if(Input_Digit2 < 10)
			Input_Digit1 = Input_Digit2;
		if(Input_Digit3 < 10)
			Input_Digit2 = Input_Digit3;
		if(Input_Digit4 < 10)
			Input_Digit3 = Input_Digit4;
		if(Input_Digit5 < 10)
			Input_Digit4 = Input_Digit5;
		if(Input_Digit6 < 10)
			Input_Digit5 = Input_Digit6;
		if(Input_Digit7 < 10)
			Input_Digit6 = Input_Digit7;
	}
	else if(IsICPSet(ICPClass::MODE_CNI) && mICPSecondaryMode == ONE_BUTTON ||
		IsICPSet(ICPClass::MODE_CNI) && mICPSecondaryMode == TWO_BUTTON ||
		(BQ || BI || SQ || SI || OA1 ||OA2 || EDITMSLFLOOR) ||
		IsICPSet(ICPClass::MODE_LIST) && mICPSecondaryMode == ONE_BUTTON ||
		IsICPSet(ICPClass::MODE_LIST) && mICPSecondaryMode == TWO_BUTTON ||
		IsICPSet(ICPClass::MODE_LIST) && mICPSecondaryMode == THREE_BUTTON || 
		IsICPSet(ICPClass::MODE_LIST) && mICPSecondaryMode == FIFE_BUTTON ||
		IsICPSet(ICPClass::MODE_LIST) && mICPSecondaryMode == NINE_BUTTON ||
		IsICPSet(ICPClass::MODE_LIST) && mICPSecondaryMode == 100 ||  //INTG
		(IsICPSet(ICPClass::MODE_LIST) && mICPSecondaryMode == SIX_BUTTON) || //INS
		IsICPSet(ICPClass::MODE_IFF) || (IsICPSet(ICPClass::MISC_MODE) && 
		mICPSecondaryMode == FIFE_BUTTON))	
	{
		Manual_Input = TRUE;
	}
	else if(IsICPSet(ICPClass::MODE_CNI) && mICPSecondaryMode == FOUR_BUTTON)
	{
		ClearFlags();
		ClearStrings();
		SetICPFlag(ICPClass::MODE_LIST);
		Manual_Input = TRUE;
		mICPSecondaryMode = ONE_BUTTON;
		SetICPFlag(ICPClass::EDIT_LAT);
	}
	else if(IsICPSet(ICPClass::EDIT_LAT))
		Manual_Input = TRUE;

	else if(IsICPSet(ICPClass::EDIT_LONG))
		Manual_Input = TRUE;

	else if(IsICPSet(ICPClass::CHAFF_BINGO))
		Manual_Input = TRUE;

	else if(IsICPSet(ICPClass::FLARE_BINGO))
		Manual_Input = TRUE;
	
	InputsMade ++;
	if(ClearCount > 0)
		ClearCount --;

	switch(button)
	{
	case ONE_BUTTON:
			MadeInput = TRUE;
			Input_Digit7 = 1;
		break;
	case TWO_BUTTON:
			MadeInput = TRUE;
			Input_Digit7 = 2;
		break;
	case THREE_BUTTON:
			MadeInput = TRUE;
			Input_Digit7 = 3;
		break;
	case FOUR_BUTTON:
			MadeInput = TRUE;
			Input_Digit7 = 4;
		break;
	case FIFE_BUTTON:
			MadeInput = TRUE;
			Input_Digit7 = 5;
		break;
	case SIX_BUTTON:
			MadeInput = TRUE;
			Input_Digit7 = 6;
		break;
	case SEVEN_BUTTON:
			MadeInput = TRUE;
			Input_Digit7 = 7;
		break;
	case EIGHT_BUTTON:
			MadeInput = TRUE;
			Input_Digit7 = 8;
		break;
	case NINE_BUTTON:
			MadeInput = TRUE;
			Input_Digit7 = 9;
		break;
	case ZERO_BUTTON:
			Input_Digit7 = 0;
		break;
	}
}
void ICPClass::ClearFlags(void)
{
	ICPModeFlags = 0;
	BQ = FALSE;
	BI = FALSE; 
	SQ = FALSE;
	SI = FALSE;
	OA1 = FALSE;
	OA2 = FALSE;
	OA_RNG = FALSE;
	OA_BRG = FALSE;
	OA_ALT = FALSE;
	Manual_Input = FALSE;
	MadeInput = FALSE;
	VIP_RNG = FALSE;
	VIP_BRG = FALSE;
	VIP_ALT = FALSE;
	VRP_RNG = FALSE;
	VRP_BRG = FALSE;
	VRP_ALT = FALSE;
			
	//Reset our Input Variables
	Input_Digit1 = 25;
	Input_Digit2 = 25;
	Input_Digit3 = 25;
	Input_Digit4 = 25;
	Input_Digit5 = 25;
	Input_Digit6 = 25;
	Input_Digit7 = 25;

	ClearCount = 0;

	//Clear our secondary mode, used for later input
	mICPSecondaryMode = NONE_MODE;
	mpSecondaryExclusiveButton = NULL;
}
void ICPClass::ClearInput(void)
{
	//Have we blocked our input?
	if(IsICPSet(ICPClass::BLOCK_MODE))
	{
		//Clear all our input flags
		ClearICPFlag(ICPClass::BLOCK_MODE);
		ClearICPFlag(ICPClass::FLASH_FLAG);
		*tempstr = NULL;
		InputsMade = 0;
		ClearCount = 0;
		MadeInput = FALSE;
		ClearDigits();
		ClearStrings();
		ClearString();
		Manual_Input = FALSE;
		if(OA_BRG)
		{
			if(OA1)
				fOA_BRG = tempvar1;
			else
				fOA_BRG2 = tempvar1;
		}
		if(VIP_BRG || VRP_BRG)
		{
			if(VIP_BRG)
				fVIP_BRG = tempvar1;
			else
				fVRP_BRG = tempvar1;
		}
		if(PGMChaff)
		{
			if(BI)
				fCHAFF_BI[CPI] = tempvar1;
			else if(SI)
				fCHAFF_SI[CPI] = tempvar1;
		}
		if(PGMFlare)
		{
			if(BI)
				fFLARE_BI[FPI] = tempvar1;
			else if (SI)
				fFLARE_SI[FPI] = tempvar1;
		}
		return;
	}
	
	ClearCount ++;

	if(ClearCount == 2 || InputsMade == 1)
	{
		//Clear it all here, unset flags
		ClearCount = 0;
		ClearICPFlag(ICPClass::BLOCK_MODE);
		ClearICPFlag(ICPClass::FLASH_FLAG);
		ClearString();
		ClearStrings();
		ClearDigits();
		Manual_Input = FALSE;
		MadeInput = FALSE;
		InputsMade = 0;
		if(OA_BRG)
		{
			if(OA1)
				fOA_BRG = tempvar1;
			else
				fOA_BRG2 = tempvar1;
		}
		if(VIP_BRG || VRP_BRG)
		{
			if(VIP_BRG)
				fVIP_BRG = tempvar1;
			else
				fVRP_BRG = tempvar1;
		}
		if(PGMChaff)
		{
			if(BI)
				fCHAFF_BI[CPI] = tempvar1;
			else if(SI)
				fCHAFF_SI[CPI] = tempvar1;
		}
		if(PGMFlare)
		{
			if(BI)
				fFLARE_BI[FPI] = tempvar1;
			else if (SI)
				fFLARE_SI[FPI] = tempvar1;
		}
	}
	else
	{
		Input_Digit7 = Input_Digit6;
		Input_Digit6 = Input_Digit5;
		Input_Digit5 = Input_Digit4;
		Input_Digit4 = Input_Digit3;
		Input_Digit3 = Input_Digit2;
		Input_Digit2 = Input_Digit1;
		InputsMade --;
	}
}
void ICPClass::FillDEDMatrix(int Line, int Pos, char *str, int Inverted)
{
	ShiAssert(Line >=0 && Line < 5);
	ShiAssert(Pos >=0 && Pos+strlen(str) <= MAX_DED_LEN);

	for(int i = 0; str[i] != '\0'; i++)
	{
		DEDLines[Line][Pos + i] = str[i];
		Invert[Line][Pos + i] = Inverted;
	}
}
void ICPClass::GetWind(void)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	Tpoint			pos;
	pos.x = playerAC->XPos();
	pos.y =playerAC->YPos();
	pos.z = playerAC->ZPos();

	heading	= FloatToInt32(((WeatherClass*)realWeather)->WindHeadingAt(&pos) * RTD);
	if(heading <= 0) 
		heading += 180;
	else if(heading > 0) 
		heading -= 180;				
	if(heading < 0)
		heading = 360 + heading;


			windSpeed	= ((WeatherClass*)realWeather)->WindSpeedInFeetPerSecond(&pos) * FTPSEC_TO_KNOTS;

}
void ICPClass::AddSTPT(int Line, int Pos)
{
	if(mWPIndex + 1 > 9)
		sprintf(tempstr, "%d",mWPIndex + 1);
	else
		sprintf(tempstr," %d",mWPIndex + 1);
	FillDEDMatrix(Line,Pos,tempstr);
	FillDEDMatrix(Line,Pos + 2,"\x01");
}
void ICPClass::MakeInverted(int Line, int Start, int End)
{
	for(int i= Start; i < End; i++)
			Invert[Line][i] = 2;
}
void ICPClass::ClearStrings(void)
{
	for(int i = 0; i < 5; i++)
	{
		memset(DEDLines[i], ' ', MAX_DED_LEN - 1);
		DEDLines[i][MAX_DED_LEN - 1] = '\0';
		memset(Invert[i], ' ',MAX_DED_LEN - 1);
		Invert[i][MAX_DED_LEN - 1] = '\0';
	}
}
void ICPClass::ClearInverted(int Line, int Start, int End)
{
	for(int i= Start; i < End; i++)
			Invert[Line][i] = 0;
}

void ICPClass::FindEDR(long hours, char* timeStr) 
{
	long	minutes, secs;
	char	hoursStr[3] = "";
	char	minutesStr[3] = "";
	char	secsStr[3] = "";
	minutes	= hours % S_IN_H;			// generate hours column
	hours		= hours  / S_IN_H;		
	secs		= minutes % S_IN_M;		// generate secs column
	minutes	= minutes / S_IN_M;			// generate minutes column
	if(hours <= 0 && minutes <= 0 && secs <= 0)
	{
		sprintf(timeStr, "00:00:00");
		return;
	}
	sprintf(hoursStr, "%2d", hours); 
	if(hours < 10) 
		*hoursStr = 0x30;
	sprintf(minutesStr, "%2d", minutes); 
	if(minutes < 10) 
		*minutesStr = 0x30;
	sprintf(secsStr, "%2d", secs); 
	if(secs < 10) 
		*secsStr = 0x30;
	sprintf(timeStr, "%2s:%2s:%2s", hoursStr, minutesStr, secsStr);
}
void ICPClass::LeaveCNI(void)
{
	ClearICPFlag(ICPClass::MODE_CNI);
	ClearICPFlag(ICPClass::EDIT_STPT);
	ClearICPFlag(ICPClass::EDIT_VHF);
	ClearICPFlag(ICPClass::EDIT_UHF);
}
void ICPClass::ClearDigits(void)
{
	Input_Digit1 = 25;
	Input_Digit2 = 25;
	Input_Digit3 = 25;
	Input_Digit4 = 25;
	Input_Digit5 = 25;
	Input_Digit6 = 25;
	Input_Digit7 = 25;
}
void ICPClass::StepCruise(void)
{
	ClearStrings();
	ClearCount = 0;
	if(Cruise_TOS)
	{
		Cruise_TOS = FALSE;
		Cruise_RNG = TRUE;
		ClearICPFlag(ICPClass::EDIT_STPT);
	}
	else if(Cruise_RNG)
	{
		Cruise_RNG = FALSE;
		Cruise_HOME = TRUE;
		ClearICPFlag(ICPClass::EDIT_STPT);
	}
	else if(Cruise_HOME)
	{
		Cruise_HOME = FALSE;
		Cruise_EDR = TRUE;
		SetICPFlag(ICPClass::EDIT_STPT);
	}
	else if(Cruise_EDR)
	{
		Cruise_EDR = FALSE;
		Cruise_TOS = TRUE;
		ClearICPFlag(ICPClass::EDIT_STPT);
	}
}
void ICPClass::ClearString(void)
{
	memset(InputString, ' ', 15);
	InputString[15] = '\0';
}
void ICPClass::EWSOnOff(void)
{
	if(IsICPSet(ICPClass::EDIT_JAMMER))		
	{
		if(EWS_JAMMER_ON)
			EWS_JAMMER_ON = FALSE;
		else
			EWS_JAMMER_ON = TRUE;
	}
	else if(IsICPSet(ICPClass::EWS_EDIT_BINGO))
	{
		if(EWS_BINGO_ON)
			EWS_BINGO_ON = FALSE;
		else
			EWS_BINGO_ON = TRUE;
	}
}
void ICPClass::CheckDigits(void)
{
	if(Input_Digit1 > 10)
		Input_Digit1 = 0;
	if(Input_Digit2 > 10)
		Input_Digit2 = 0;
	if(Input_Digit3 > 10)
		Input_Digit3 = 0;
	if(Input_Digit4 > 10)
		Input_Digit4 = 0;
	if(Input_Digit5 > 10)
		Input_Digit5 = 0;
	if(Input_Digit6 > 10)
		Input_Digit6 = 0;
	if(Input_Digit7 > 10)
		Input_Digit7 = 0;
}
void ICPClass::ResetSubPages(void)
{
	//Dest Stuff
	OA1 = FALSE;
	OA2 = FALSE;
	OA_RNG = TRUE;

	//EWS Stuff
	EWSMain = TRUE;
	PGMChaff = FALSE;
	PGMFlare = FALSE;
}
int ICPClass::AddUp(void)
{
	int var = 0;
	CheckDigits();
	var += Input_Digit7;
	var += Input_Digit6*10;
	var += Input_Digit5*100;
	var += Input_Digit4*1000;
	var += Input_Digit3*10000;
	var += Input_Digit2*100000;
	var += Input_Digit1*1000000;
	return var;
}
float ICPClass::AddUpFloat(void)
{
	float var = 0;
	CheckDigits();
	var += Input_Digit7;
	var += Input_Digit6*10;
	var += Input_Digit5*100;
	var += Input_Digit4*1000;
	var += Input_Digit3*10000;
	var += Input_Digit2*100000;
	var += Input_Digit1*1000000;
	return var;
}
long ICPClass::AddUpLong(void)
{
	long var = 0;
	CheckDigits();
	var += Input_Digit7;
	var += Input_Digit6*10;
	var += Input_Digit5*100;
	var += Input_Digit4*1000;
	var += Input_Digit3*10000;
	var += Input_Digit2*100000;
	var += Input_Digit1*1000000;
	return var;
}
void ICPClass::CheckAutoSTPT(void)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	//no autostpt when in AG mode
	if(playerAC && 
		playerAC->FCC && 
		playerAC->FCC->GetMasterMode() == FireControlComputer::AirGroundBomb) // MLR-NOTE Needs to consider Rockets???
		return;

	if(!playerAC || !playerAC->curWaypoint)
		return;

	playerAC->curWaypoint->GetLocation(&xCurr, &yCurr, &zCurr);
	float deltaX = xCurr - playerAC->XPos();
	float deltaY = yCurr - playerAC->YPos();
	float distanceToSta	= ((float)sqrt(deltaX * deltaX + deltaY * deltaY) * FT_TO_NM);
	if(distanceToSta <= 2)
		PNUpdateSTPTMode(NEXT_BUTTON, 0);
}
void ICPClass::InitStuff(void)
{
	//Init our matrixes
	ClearStrings();
	//Clear our Flags
	ClearFlags();
	//Init our InputString
	ClearString();
	//Init our Input Digits
	ClearDigits();
	//Init our input variables
	Manual_Input = FALSE;
	InputsMade = 0;
	IsSelected = FALSE;
	//Init Cruise page
	Cruise_TOS = TRUE;
	Cruise_RNG = FALSE;
	Cruise_HOME = FALSE;
	Cruise_EDR = FALSE;
	HomeWP = 1;
	RangeWP = 1;
	TOSWP = 1;
	CruiseMode = 0;
	//Time page
	Difference = 0;
	running = FALSE;
	stopped = FALSE;
	//Comm page
	WhichRadio = 0;
	CurrChannel = 0;
	PREUHF = 4;
	PREVHF = 4;
	//CNI page
	SetICPFlag(ICPClass::MODE_CNI);
	SetICPFlag(ICPClass::EDIT_STPT);
	ShowWind = FALSE;
	//Dest page
	OA_RNG = FALSE;
	OA_BRG = FALSE;
	OA_ALT = FALSE;
	iOA_RNG = 0; 
	fOA_BRG = 0;
	iOA_ALT = 0;
	iOA_RNG2 = 0; 
	fOA_BRG2 = 0;
	iOA_ALT2 = 0;
	//EWS stuff
	ChaffBingo = 10;
	FlareBingo = 10;
	EWS_JAMMER_ON = TRUE;
	EWS_BINGO_ON = TRUE;
	BQ = FALSE;
	BI = FALSE;
	SQ = FALSE;
	SI = FALSE;
	//Prog 1, optimised for High-Med Alt Sams
	iCHAFF_BQ[0] = 3;
	fCHAFF_BI[0] = 0.5F;
	iCHAFF_SQ[0] = 3;
	fCHAFF_SI[0] = 2.0F;
	iFLARE_BQ[0] = 0;
	fFLARE_BI[0] = 0.0F;
	iFLARE_SQ[0] = 0;
	fFLARE_SI[0] = 0.0F;
	//Prog 2, optimised for Merge
	iCHAFF_BQ[1] = 1;
	fCHAFF_BI[1] = 0.5F;
	iCHAFF_SQ[1] = 3;
	fCHAFF_SI[1] = 3.0F;
	iFLARE_BQ[1] = 4;
	fFLARE_BI[1] = 0.25F;
	iFLARE_SQ[1] = 2;
	fFLARE_SI[1] = 1.0F;
	//Prog 3, optimised for Pop-Up, Chaff only
	iCHAFF_BQ[2] = 2;
	fCHAFF_BI[2] = 0.5F;
	iCHAFF_SQ[2] = 4;
	fCHAFF_SI[2] = 3.0F;
	iFLARE_BQ[2] = 0;
	fFLARE_BI[2] = 0.0F;
	iFLARE_SQ[2] = 0;
	fFLARE_SI[2] = 0.0F;
	//Prog 3, optimised for Pop-Up, Flare only
	iCHAFF_BQ[3] = 2;
	fCHAFF_BI[3] = 0.5F;
	iCHAFF_SQ[3] = 4;
	fCHAFF_SI[3] = 3.0F;
	iFLARE_BQ[3] = 2;
	fFLARE_BI[3] = 0.5F;
	iFLARE_SQ[3] = 3;
	fFLARE_SI[3] = 3.0F;

	CPI = 0;
	FPI = 0;

	MadeInput = FALSE;
	//MAN Page
	IN_AA = TRUE;
	IN_AG = FALSE;
	//STPT stuff
	MAN = TRUE;	
	//ALOW
	EDITMSLFLOOR = FALSE;
	TFADV = FALSE;
	ManWSpan = 35.0F;
	ManualWSpan = 35.0F;
	//VIP
	VIP_RNG = FALSE;
	VIP_BRG = FALSE;
	VIP_ALT = FALSE;
	iVIP_RNG = 0; 
	fVIP_BRG = 0;
	iVIP_ALT = 0;
	//VRP
	VRP_RNG = FALSE;
	VRP_BRG = FALSE;
	VRP_ALT = FALSE;
	iVRP_RNG = 0; 
	fVRP_BRG = 0;
	iVRP_ALT = 0;
	//FACK timer
	m_FaultDisplay = false;
	m_subsystem = FaultClass::amux_fault;
	m_function = FaultClass::nofault;
	//Comand steering
	CMDSTRG = TRUE; //JPG 23 Feb 04 - CMD STRG defaults to being mode-selected
	ILSPageSel = 0;
	//Bullseye stuff
	ShowBullseyeInfo = FALSE;
	//voicestuff me123
	transmitingvoicecom1 = FALSE;
	transmitingvoicecom2 = FALSE;
	//IFF Stuff
	IFFModes = 0;
	SetIFFFlag(ICPClass::MODE_1);
	SetIFFFlag(ICPClass::MODE_3);
	SetIFFFlag(ICPClass::MODE_C);
	Mode1Code = 22;
	Mode2Code = 3412;
	Mode3Code = 1234;

	//INS
	INSTime = 0;
	INSLine = 0;
	sprintf(INSLong, "");
	sprintf(INSLat, "");
	//fill our LAT and LONG strings, so we can display then on the INS page
	GetINSInfo();
	sprintf(altStr, "");
	sprintf(INSHead,"");
	INSEnter = FALSE;
	EnteredHDG = FALSE;
	INSLATDiff = 0.0F;
	INSLONGDiff = 0.0F;
	INSALTDiff = 0.0F;
	INSHDGDiff = 0;
	StartLat = 0.0F;
	StartLong = 0.0F;
	FillStrings = TRUE;

	//Laser page
	LaserTime = 8;	//8 seconds
	LaserLine = 1;
	LaserCode = 1123;
	ClearPFLLines();

	//Voice com volume
	Comm1Volume = Comm2Volume = 4; // MLR 2003-10-20 Was 0
}
void ICPClass::ResetInput(void)
{
	ClearDigits();
	ClearStrings();
	Manual_Input = FALSE;
	MadeInput = FALSE;
	InputsMade = 0;
	CNISwitch(DOWN_MODE);
}
int ICPClass::CheckBackupPages(void)
{
	if(IsICPSet(ICPClass::MODE_CNI) || IsICPSet(ICPClass::MODE_IFF) ||
		IsICPSet(ICPClass::MODE_COMM1) || IsICPSet(ICPClass::MODE_COMM2))
		return TRUE;
	else
		return FALSE;

	return FALSE;
}
void ICPClass::LeaveCNIPage(void)
{
	ClearICPFlag(ICPClass::EDIT_STPT);
	ClearICPFlag(ICPClass::EDIT_VHF);
	ClearICPFlag(ICPClass::EDIT_UHF);
}
//PFL
void ICPClass::ClearPFLLines(void)
{
	for(int i = 0; i < 5; i++)
	{
		memset(PFLLines[i], ' ', MAX_PFL_LEN - 1);
		PFLLines[i][MAX_PFL_LEN - 1] = '\0';
		memset(PFLInvert[i], ' ',MAX_PFL_LEN - 1);
		PFLInvert[i][MAX_PFL_LEN - 1] = '\0';
	}
}
void ICPClass::FillPFLMatrix(int Line, int Pos, char *str, int Inverted)
{
	ShiAssert(Line >=0 && Line < 5);
	ShiAssert(Pos >=0 && Pos+strlen(str) < MAX_PFL_LEN);

	for(int i = 0; str[i] != '\0'; i++)
	{
		PFLLines[Line][Pos + i] = str[i];
		PFLInvert[Line][Pos + i] = Inverted;
	}
}
void ICPClass::StepEWSProg(int mode)
{
	if(mode == NEXT_BUTTON)
	{
		if(PGMChaff)
		{
			if(CPI < 3)
				CPI++;
			else
				CPI = 0;
		}
		else if(PGMFlare)
		{
			if(FPI < 3)
				FPI++;
			else
				FPI = 0;
		}
	}
	else
	{
		if(PGMChaff)
		{
			if(CPI > 0)
				CPI--;
			else
				CPI = 3;
		}
		else if(PGMFlare)
		{
			if(FPI > 0)
				FPI--;
			else
				FPI = 3;
		}
	}
}
void ICPClass::ShowFlareIndex(int Line,int Pos)
{
	sprintf(tempstr," %d", FPI + 1);
	FillDEDMatrix(Line,Pos,tempstr);
	FillDEDMatrix(Line,Pos + 2,"\x01");
}
void ICPClass::ShowChaffIndex(int Line,int Pos)
{
	sprintf(tempstr," %d", CPI + 1);
	FillDEDMatrix(Line,Pos,tempstr);
	FillDEDMatrix(Line,Pos + 2,"\x01");
}
void ICPClass::FillIFFString(char *string)
{
	//clear it first, in case there's something unwanted
	sprintf(string,"");
	string[0] = 'M';
	//highest modes first
	if(IsIFFSet(ICPClass::MODE_4))
		string[1] = '4';
	else
		string[1] = ' ';

	if(IsIFFSet(ICPClass::MODE_3))
		string[2] = '3';
	else
		string[2] = ' ';

	if(IsIFFSet(ICPClass::MODE_2))
		string[3] = '2';
	else
		string[3] = ' ';

	if(IsIFFSet(ICPClass::MODE_1))
		string[4] = '1';
	else
		string[4] = ' ';

	if(IsIFFSet(ICPClass::MODE_C))
		string[5] = 'C';
	else
		string[5] = ' ';

	string[6] = '\0';
}
float ICPClass::GetNumScans(void)
{
	float num = 0.0F;
	if(IsIFFSet(ICPClass::MODE_4))
		num++;
	if(IsIFFSet(ICPClass::MODE_3))
		num++;
	if(IsIFFSet(ICPClass::MODE_2))
		num++;
	if(IsIFFSet(ICPClass::MODE_1))
		num++;
	if(IsIFFSet(ICPClass::MODE_C))
		num++;

	ShiAssert(num < 6.0F);
	return num;
}
//functions for INS only
void ICPClass::GetINSInfo(void)
{
	latitude	= (FALCON_ORIGIN_LAT * FT_PER_DEGREE + cockpitFlightData.x) / EARTH_RADIUS_FT;
	cosLatitude = (float)cos(latitude);
	longitude	= ((FALCON_ORIGIN_LONG * DTR * EARTH_RADIUS_FT * cosLatitude) + cockpitFlightData.y) / (EARTH_RADIUS_FT * cosLatitude);

	latitude	*= RTD;
	longitude	*= RTD;

	StartLat = latitude;
	StartLong = longitude;
	
	latDeg	= FloatToInt32(latitude);
	latMin	= (float)fabs(latitude - latDeg) * DEG_TO_MIN;
	longDeg		= FloatToInt32(longitude);
	longMin		= (float)fabs(longitude - longDeg) * DEG_TO_MIN;
	
	// format lat/long here
	if(latMin < 10.0F) 
		sprintf(INSLat, "%3d*0%2.2f\'\n", latDeg, latMin);
	else 
		sprintf(INSLat, "%3d*%2.2f\'\n", latDeg, latMin);	

	if(longMin < 10.0F) 
		sprintf(INSLong, "%3d*0%2.2f\'\n", longDeg, longMin);
	else 
		sprintf(INSLong, "%3d*%2.2f\'\n", longDeg, longMin);

	sprintf(altStr, "%dFT", (long)-cockpitFlightData.z);

	float yaw = cockpitFlightData.yaw;
	if(yaw < 0.0F)
		yaw += 2 * PI;
	sprintf(INSHead, "%3.1f*", yaw * RTD);
	INSHead[3] = '.';
}
