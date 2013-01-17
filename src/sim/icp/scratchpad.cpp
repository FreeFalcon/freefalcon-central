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

void ICPClass::ScratchPad(int Line, int Start, int End)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	//ILS page
	if(IsICPSet(ICPClass::MODE_CNI) && mICPSecondaryMode == ONE_BUTTON)
	{
		if(Manual_Input)
		{
			if(Input_Digit7 < 10)
				InputString[2] = '0' + Input_Digit7;
			else
				InputString[2] = ' ';
			if(Input_Digit6 < 10)
				InputString[1] = '0' + Input_Digit6;
			else 
				InputString[1] = ' ';
			if(Input_Digit5 < 10)
				InputString[0] = '0' + Input_Digit5;
			else
				InputString[0] = ' ';
			InputString[3] = '\0';
		}
		else
		{
			ClearString();
			InputString[3] = '\0';
		}
	}
	//Bingo Page
	else if(IsICPSet(ICPClass::MODE_LIST) && mICPSecondaryMode == TWO_BUTTON)
	{
		if(!Manual_Input)
		{
			level = (long)((AircraftClass*)(playerAC))->GetBingoFuel();
			if(level < 10000)
				sprintf(InputString," %dLBS",level);
			else 
				sprintf(InputString,"%dLBS",level);
		}
		else
		{
			InputString[5] = 'L';
			InputString[6] = 'B';
			InputString[7] = 'S';
			if(Input_Digit7 < 10)
				InputString[4] = '0' + Input_Digit7;
			else
				InputString[4] = ' ';
			if(Input_Digit6 < 10)
				InputString[3] = '0' + Input_Digit6;
			else 
				InputString[3] = ' ';
			if(Input_Digit5 < 10)
				InputString[2] = '0' + Input_Digit5;
			else
				InputString[2] = ' ';
			if(Input_Digit4 < 10)
				InputString[1] = '0' + Input_Digit4;
			else
				InputString[1] = ' ';
			if(Input_Digit3 < 10)
				InputString[0] = '0' + Input_Digit3;
			else
				InputString[0] = ' ';
		}
		InputString[8] = '\0';
	}
	else if(IsICPSet(ICPClass::MODE_CNI) && mICPSecondaryMode == TWO_BUTTON)
	{
		if(!Manual_Input)
		{
			if(EDITMSLFLOOR)
				sprintf(InputString,"%dFT",TheHud->MSLFloor);
			else if(TFADV)
				sprintf(InputString,"%dFT",TheHud->TFAdv);
			else
			{
				if(EDITMSLFLOOR);
				else if(TFADV);
				else
				{
					long	alt;
					int	alt1, alt2;
					char	tmpstr[5] = "";
					alt = (long)TheHud->lowAltWarning;

					alt1 = (int) alt / 1000;
					alt2 = (int) alt % 1000;

					if(alt1) 
					{
						sprintf(tmpstr, "%-3d", alt2);
						if(alt2 < 100) 
							tmpstr[0] = '0';
						if(alt2 < 10)
							tmpstr[1] = '0';
						if(alt2 < 1)
							tmpstr[2] = '0';
						sprintf(InputString, "%-d%sFT", alt1, tmpstr);
					} 
					else
						sprintf(InputString, "%dFT", alt2);
				}
			}
		}
		else
		{
			InputString[7] = '\0';
			InputString[6] = 'T';
			InputString[5] = 'F';
			if(Input_Digit7 < 10)
				InputString[4] = '0' + Input_Digit7;
			else
				InputString[4] = ' ';
			if(Input_Digit6 < 10)
				InputString[3] = '0' + Input_Digit6;
			else 
				InputString[3] = ' ';
			if(Input_Digit5 < 10)
				InputString[2] = '0' + Input_Digit5;
			else
				InputString[2] = ' ';
			if(Input_Digit4 < 10)
				InputString[1] = '0' + Input_Digit4;
			else
				InputString[1] = ' ';
			if(Input_Digit3 < 10)
				InputString[0] = '0' + Input_Digit3;
			else
				InputString[0] = ' ';
		}
	}
	else if((IsICPSet(ICPClass::EDIT_LAT) || IsICPSet(ICPClass::EDIT_LONG)) && Manual_Input)
	{
		InputString[10] = '\0';
		InputString[9] = '\'';
		if(Input_Digit7 < 10)
			InputString[8] = '0' + Input_Digit7;
		else
			InputString[8] = '0';
		if(Input_Digit6 < 10)
			InputString[7] = '0' + Input_Digit6;
		else 
			InputString[7] = '0';
		InputString[6] = '.';
		if(Input_Digit5 < 10)
			InputString[5] = '0' + Input_Digit5;
		else
			InputString[5] = '0';
		if(Input_Digit4 < 10)
			InputString[4] = '0' + Input_Digit4;
		else
			InputString[4] = '0';
		InputString[3] = '*'; //this is a ° symbol
		if(Input_Digit3 < 10)
			InputString[2] = '0' + Input_Digit3;
		else
			InputString[2] = '0';
		if(Input_Digit2 < 10)
			InputString[1] = '0' + Input_Digit2;
		else
			InputString[1] = '0';
		if(IsICPSet(ICPClass::EDIT_LAT) && Manual_Input)
		{
			if(Input_Digit1 < 10)
				InputString[0] = '0' + Input_Digit1;
			else
				InputString[0] = '0';
		}
		else
			InputString[0] = ' ';
	}
	else if(IsICPSet(ICPClass::MODE_LIST) && mICPSecondaryMode == FIFE_BUTTON)
	{
		if(Manual_Input)
		{
			InputString[5] = '\0';
			InputString[3] = 'F';
			InputString[4] = 'T';
			if(Input_Digit7 < 10)
				InputString[2] = '0' + Input_Digit7;
			else
				InputString[2] = ' ';
			if(Input_Digit6 < 10)
				InputString[1] = '0' + Input_Digit6;
			else 
				InputString[1] = ' ';
			if(Input_Digit5 < 10)
				InputString[0] = '0' + Input_Digit5;
			else
				InputString[0] = ' ';
			
		}
		else
			sprintf(InputString,"%3.0fFT",ManualWSpan);
	}
	//INS page
	else if(IsICPSet(ICPClass::MODE_LIST) && mICPSecondaryMode == SIX_BUTTON && Manual_Input)
	{
		if(INSLine == 0)
		{
			//LAT String
			InputString[10] = '\0';
			InputString[9] = '\'';
			if(Input_Digit7 < 10)
				InputString[8] = '0' + Input_Digit7;
			else
				InputString[8] = '0';
			if(Input_Digit6 < 10)
				InputString[7] = '0' + Input_Digit6;
			else 
				InputString[7] = '0';
			InputString[6] = '.';
			if(Input_Digit5 < 10)
				InputString[5] = '0' + Input_Digit5;		
			else
				InputString[5] = '0';
			if(Input_Digit4 < 10)
				InputString[4] = '0' + Input_Digit4;
			else
				InputString[4] = '0';
			InputString[3] = '*'; //this is a ° symbol
			if(Input_Digit3 < 10)
				InputString[2] = '0' + Input_Digit3;
			else
				InputString[2] = '0';
			if(Input_Digit2 < 10)
				InputString[1] = '0' + Input_Digit2;
			else
				InputString[1] = '0';
		}
		else if(INSLine == 1)
		{
			//LONG String
			InputString[10] = '\0';
			InputString[9] = '\'';
			if(Input_Digit7 < 10)
				InputString[8] = '0' + Input_Digit7;
			else
				InputString[8] = '0';
			if(Input_Digit6 < 10)
				InputString[7] = '0' + Input_Digit6;
			else 
				InputString[7] = '0';
			InputString[6] = '.';
			if(Input_Digit5 < 10)
				InputString[5] = '0' + Input_Digit5;
			else
				InputString[5] = '0';
			if(Input_Digit4 < 10)
				InputString[4] = '0' + Input_Digit4;
			else
				InputString[4] = '0';
			InputString[3] = '*'; //this is a ° symbol
			if(Input_Digit3 < 10)
				InputString[2] = '0' + Input_Digit3;
			else
				InputString[2] = '0';
			if(Input_Digit2 < 10)
				InputString[1] = '0' + Input_Digit2;
			else
				InputString[1] = '0';
			if(Input_Digit1 < 10)
				InputString[0] = '0' + Input_Digit1;
			else
				InputString[0] = '0';
		}
		else if(INSLine == 2)
		{
			//ALT String
			InputString[7] = '\0';
			InputString[6] = 'T';
			InputString[5] = 'F';
			if(Input_Digit7 < 10)
				InputString[4] = '0' + Input_Digit7;
			else
				InputString[4] = ' ';
			if(Input_Digit6 < 10)
				InputString[3] = '0' + Input_Digit6;
			else 
				InputString[3] = ' ';
			if(Input_Digit5 < 10)
				InputString[2] = '0' + Input_Digit5;
			else
				InputString[2] = ' ';
			if(Input_Digit4 < 10)
				InputString[1] = '0' + Input_Digit4;
			else
				InputString[1] = ' ';
			if(Input_Digit3 < 10)
				InputString[0] = '0' + Input_Digit3;
			else
				InputString[0] = ' ';
		}
		else if(INSLine == 3)
		{
			//THDG String
			InputString[6] = '\0';
			InputString[5] = '*';
			if(Input_Digit7 < 10)
				InputString[4] = '0' + Input_Digit7;
			else
				InputString[4] = ' ';
			InputString[3] = '.';
			if(Input_Digit6 < 10)
				InputString[2] = '0' + Input_Digit6;
			else
				InputString[2] = ' ';
			if(Input_Digit5 < 10)
				InputString[1] = '0' + Input_Digit5;
			else 
				InputString[1] = ' ';
			if(Input_Digit4 < 10)
				InputString[0] = '0' + Input_Digit4;
			else
				InputString[0] = ' ';
		}
	}
	else if(IsICPSet(ICPClass::CHAFF_BINGO))
	{
		if(Manual_Input)
		{
			if(Input_Digit7 < 10)
				InputString[1] = '0' + Input_Digit7;
			else
				InputString[1] = '0';
			if(Input_Digit6 < 10)
				InputString[0] = '0' + Input_Digit6;
			else
				InputString[0] = ' ';
			InputString[2] = '\0';
		}
		else
			sprintf(InputString,"%d",ChaffBingo);
	}
	else if(IsICPSet(ICPClass::FLARE_BINGO))
	{
		if(Manual_Input)
		{
			if(Input_Digit7 < 10)
				InputString[1] = '0' + Input_Digit7;
			else
				InputString[1] = ' ';
			if(Input_Digit6 < 10)
				InputString[0] = '0' + Input_Digit6;
			else
				InputString[0] = ' ';
			InputString[2] = '\0';
		}
		else
			sprintf(InputString,"%d",FlareBingo);
	}
	else if(IsICPSet(ICPClass::MODE_LIST) && mICPSecondaryMode == EIGHT_BUTTON)
	{
		if(IN_AA)
		{
			sprintf(InputString,"A-A");
			if(mICPPrimaryMode == AA_MODE)
				IsSelected = TRUE;
			else
				IsSelected = FALSE;
		}
		else if(IN_AG)
		{
			sprintf(InputString,"A-G");
			if(mICPPrimaryMode == AG_MODE)
				IsSelected = TRUE;
			else
				IsSelected = FALSE;
		}
	}
	else if(BQ)
	{
		if(Manual_Input)
		{
			if(Input_Digit7 < 10)
				InputString[1] = '0' + Input_Digit7;
			else
				InputString[1] = ' ';
			if(Input_Digit6 < 10)
				InputString[0] = '0' + Input_Digit6;
			else
				InputString[0] = ' ';
			InputString[2] = '\0';
		}
		else
		{
			if(PGMChaff)
			{
				if(iCHAFF_BQ[CPI] <= 0)
					sprintf(InputString,"0");
				else
					sprintf(InputString,"%2.0d",iCHAFF_BQ[CPI]);
			}
			else if(PGMFlare)
			{
				if(iFLARE_BQ[FPI] <= 0)
					sprintf(InputString,"0");
				else
					sprintf(InputString,"%2.0d",iFLARE_BQ[FPI]);
			}
		}
	}
	else if(BI)
	{
		if(Manual_Input)
		{
			if(Input_Digit7 < 10)
				InputString[5] = '0' + Input_Digit7;
			else
				InputString[5] = ' ';
			if(Input_Digit6 < 10)
				InputString[4] = '0' + Input_Digit6;
			else 
				InputString[4] = ' ';
			if(Input_Digit5 < 10)
				InputString[3] = '0' + Input_Digit5;
			else
				InputString[3] = ' ';
			InputString[2] = '.';
			if(Input_Digit4 < 10)
				InputString[1] = '0' + Input_Digit4;
			else
				InputString[1] = ' ';
			if(Input_Digit3 < 10)
				InputString[0] = '0' + Input_Digit3;
			else
				InputString[0] = ' ';
			InputString[6] = '\0';
		}
		else
		{
			if(PGMChaff)
				sprintf(InputString,"%2.3f",fCHAFF_BI[CPI]);
			else if(PGMFlare)
				sprintf(InputString,"%2.3f",fFLARE_BI[FPI]);
		}
	}
	else if(SQ)
	{
		if(Manual_Input)
		{
			if(Input_Digit7 < 10)
				InputString[1] = '0' + Input_Digit7;
			else
				InputString[1] = ' ';
			if(Input_Digit6 < 10)
				InputString[0] = '0' + Input_Digit6;
			else
				InputString[0] = ' ';
			InputString[2] = '\0';
		}
		else
		{
			if(PGMChaff)
			{
				if(iCHAFF_SQ[CPI] <= 0)
					sprintf(InputString,"0");
				else
				{
					if(iCHAFF_SQ[FPI] < 10)
						sprintf(InputString,"%1.0d",iCHAFF_SQ[CPI]);
					else
						sprintf(InputString,"%2.0d",iCHAFF_SQ[CPI]);
				}
			}
			else if(PGMFlare)
			{
				if(iFLARE_SQ[FPI] <= 0)
					sprintf(InputString,"0");
				else
				{
					if(iFLARE_SQ[FPI] < 10)
						sprintf(InputString,"%1.0d",iFLARE_SQ[FPI]);
					else
						sprintf(InputString,"%2.0d",iFLARE_SQ[FPI]);
				}
			}
		}
	}
	else if(SI)
	{
		if(Manual_Input)
		{
			if(Input_Digit7 < 10)
				InputString[5] = '0' + Input_Digit7;
			else
				InputString[5] = ' ';
			if(Input_Digit6 < 10)
				InputString[4] = '0' + Input_Digit6;
			else 
				InputString[4] = ' ';
			InputString[3] = '.';
			if(Input_Digit5 < 10)
				InputString[2] = '0' + Input_Digit5;
			else
				InputString[2] = ' ';
			if(Input_Digit4 < 10)
				InputString[1] = '0' + Input_Digit4;
			else
				InputString[1] = ' ';
			if(Input_Digit3 < 10)
				InputString[0] = '0' + Input_Digit3;
			else
				InputString[0] = ' ';
			InputString[6] = '\0';
		}
		else
		{
			if(PGMChaff)
				sprintf(InputString,"%3.2f",fCHAFF_SI[CPI]);
			else if(PGMFlare)
				sprintf(InputString,"%3.2f",fFLARE_SI[FPI]);
		}
	}
	else if(OA1 || OA2)
	{
		if(Manual_Input)
		{
			if(OA_RNG || OA_ALT)
			{
				InputString[7] = 'T';
				InputString[6] = 'F';
				if(Input_Digit7 < 10)
					InputString[5] = '0' + Input_Digit7;
				else
					InputString[5] = ' ';
				if(Input_Digit6 < 10)
					InputString[4] = '0' + Input_Digit6;
				else 
					InputString[4] = ' ';
				if(Input_Digit5 < 10)
					InputString[3] = '0' + Input_Digit5;
				else
					InputString[3] = ' ';
				if(Input_Digit4 < 10)
					InputString[2] = '0' + Input_Digit4;
				else
					InputString[2] = ' ';
				if(Input_Digit3 < 10)
					InputString[1] = '0' + Input_Digit3;
				else
					InputString[1] = ' ';
				if(Input_Digit2 < 10)
					InputString[0] = '0' + Input_Digit2;
				else
					InputString[0] = ' ';
				InputString[8] = '\0';
			}
			else if(OA_BRG)
			{
				InputString[5] = '\'';
				if(Input_Digit7 < 10)
					InputString[4] = '0' + Input_Digit7;
				else
					InputString[4] = ' ';
				InputString[3] = '.';
				if(Input_Digit6 < 10)
					InputString[2] = '0' + Input_Digit6;
				else 
					InputString[2] = ' ';
				if(Input_Digit5 < 10)
					InputString[1] = '0' + Input_Digit5;
				else
					InputString[1] = ' ';
				if(Input_Digit4 < 10)
					InputString[0] = '0' + Input_Digit4;
				else
					InputString[0] = ' ';
				InputString[6] = '\0';
			}
		}
		else
		{
			if(OA1)
			{
				if(OA_RNG)
					sprintf(InputString,"%dFT", iOA_RNG);
				if(OA_BRG)
					sprintf(InputString,"%3.1f'",fOA_BRG);
				if(OA_ALT)
					sprintf(InputString,"%dFT",iOA_ALT);
			}
			else if(OA2)
			{
				if(OA_RNG)
					sprintf(InputString,"%dFT", iOA_RNG2);
				if(OA_BRG)
					sprintf(InputString,"%3.1f'",fOA_BRG2);
				if(OA_ALT)
					sprintf(InputString,"%dFT",iOA_ALT2);
			}
		}
	}
	//VIP and VRP
	else if(IsICPSet(ICPClass::MODE_LIST) && mICPSecondaryMode == THREE_BUTTON  ||
		IsICPSet(ICPClass::MODE_LIST) && mICPSecondaryMode == NINE_BUTTON)	
	{
		if(Manual_Input)
		{
			if(VIP_RNG || VRP_RNG)
			{
				InputString[7] = 'T';
				InputString[6] = 'F';
				if(Input_Digit7 < 10)
					InputString[5] = '0' + Input_Digit7;
				else
					InputString[5] = ' ';
				if(Input_Digit6 < 10)
					InputString[4] = '0' + Input_Digit6;
				else 
					InputString[4] = ' ';
				if(Input_Digit5 < 10)
					InputString[3] = '0' + Input_Digit5;
				else
					InputString[3] = ' ';
				if(Input_Digit4 < 10)
					InputString[2] = '0' + Input_Digit4;
				else
					InputString[2] = ' ';
				if(Input_Digit3 < 10)
					InputString[1] = '0' + Input_Digit3;
				else
					InputString[1] = ' ';
				if(Input_Digit2 < 10)
					InputString[0] = '0' + Input_Digit2;
				else
					InputString[0] = ' ';
				InputString[8] = '\0';
			}
			else if(VIP_BRG || VRP_BRG)
			{
				InputString[5] = '\'';
				if(Input_Digit7 < 10)
					InputString[4] = '0' + Input_Digit7;
				else
					InputString[4] = ' ';
				InputString[3] = '.';
				if(Input_Digit6 < 10)
					InputString[2] = '0' + Input_Digit6;
				else 
					InputString[2] = ' ';
				if(Input_Digit5 < 10)
					InputString[1] = '0' + Input_Digit5;
				else
					InputString[1] = ' ';
				if(Input_Digit4 < 10)
					InputString[0] = '0' + Input_Digit4;
				else
					InputString[0] = ' ';
				InputString[6] = '\0';
			}
			else if(VIP_ALT || VRP_ALT)
			{
				InputString[7] = 'T';
				InputString[6] = 'F';
				if(Input_Digit7 < 10)
					InputString[5] = '0' + Input_Digit7;
				else
					InputString[5] = ' ';
				if(Input_Digit6 < 10)
					InputString[4] = '0' + Input_Digit6;
				else 
					InputString[4] = ' ';
				if(Input_Digit5 < 10)
					InputString[3] = '0' + Input_Digit5;
				else
					InputString[3] = ' ';
				if(Input_Digit4 < 10)
					InputString[2] = '0' + Input_Digit4;
				else
					InputString[2] = ' ';
				if(Input_Digit3 < 10)
					InputString[1] = '0' + Input_Digit3;
				else
					InputString[1] = ' ';
				if(Input_Digit2 < 10)
					InputString[0] = '0' + Input_Digit2;
				else
					InputString[0] = ' ';
				InputString[8] = '\0';
			}

		}
		else
		{
			//VIP
			if(mICPSecondaryMode == THREE_BUTTON)
			{
				if(VIP_RNG)
					sprintf(InputString,"%dFT", iVIP_RNG);
				if(VIP_BRG)
					sprintf(InputString,"%3.1f'",fVIP_BRG);
				if(VIP_ALT)
					sprintf(InputString,"%dFT",iVIP_ALT);
			}
			else if(mICPSecondaryMode == NINE_BUTTON)
			{
				if(VRP_RNG)
					sprintf(InputString,"%dFT", iVRP_RNG);
				if(VRP_BRG)
					sprintf(InputString,"%3.1f'",fVRP_BRG);
				if(VRP_ALT)
					sprintf(InputString,"%dFT",iVRP_ALT);
			}
		}
	}
	else if((IsICPSet(ICPClass::MODE_LIST) && mICPSecondaryMode == 100) ||
		IsICPSet(ICPClass::MODE_IFF)) //INTG and IFF
	{
		if(Input_Digit7 < 10)
			InputString[4] = '0' + Input_Digit7;
		else
			InputString[4] = ' ';
		if(Input_Digit6 < 10)
			InputString[3] = '0' + Input_Digit6;
		else 
			InputString[3] = ' ';
		if(Input_Digit5 < 10)
			InputString[2] = '0' + Input_Digit5;
		else
			InputString[2] = ' ';
		if(Input_Digit4 < 10)
			InputString[1] = '0' + Input_Digit4;
		else
			InputString[1] = ' ';
		if(Input_Digit3 < 10)
			InputString[0] = '0' + Input_Digit3;
		else
			InputString[0] = ' ';
		InputString[5] = '\0';
	}
	//Laser Page
	else if(IsICPSet(ICPClass::MISC_MODE) && mICPSecondaryMode == FIFE_BUTTON)
	{
		if(LaserLine == 1)
		{
			if(Manual_Input)
			{
				if(Input_Digit7 < 10)
					InputString[3] = '0' + Input_Digit7;
				else
					InputString[3] = ' ';
				if(Input_Digit6 < 10)
					InputString[2] = '0' + Input_Digit6;
				else 
					InputString[2] = ' ';
				if(Input_Digit5 < 10)
					InputString[1] = '0' + Input_Digit5;
				else
					InputString[1] = ' ';
				if(Input_Digit4 < 10)
					InputString[0] = '0' + Input_Digit4;
				else
					InputString[0] = ' ';
				InputString[4] = '\0';
			}
			else
			{
				sprintf(InputString, "%d", LaserCode);
			}
		}
		else if(LaserLine == 2)
		{
			if(Manual_Input)
			{
				if(Input_Digit7 < 10)
					InputString[2] = '0' + Input_Digit7;
				else
					InputString[2] = ' ';
				if(Input_Digit6 < 10)
					InputString[1] = '0' + Input_Digit6;
				else 
					InputString[1] = ' ';
				if(Input_Digit5 < 10)
					InputString[0] = '0' + Input_Digit5;
				else
					InputString[0] = ' ';
				InputString[3] = '\0';
			}
			else
			{
				sprintf(InputString, "%d", LaserTime);
			}
		}
	}
	else
		ClearString();

	if(IsICPSet(ICPClass::FLASH_FLAG))
	{
		if(flash)
		{
			//Visible
			FillDEDMatrix(Line, Start+1,InputString);
			FillDEDMatrix(Line,Start,"\x02",2);
			FillDEDMatrix(Line,End,"\x02",2);
			MakeInverted(Line, Start+1, End);
		}
		else
		{
			//Don't draw anything
			ClearString();
			//Limit our string lenght
			InputString[End - (Start+1)] = '\0';
			FillDEDMatrix(Line, Start+1, InputString);
			FillDEDMatrix(Line,Start,"\x02",2);
			FillDEDMatrix(Line,End,"\x02",2);
			ClearInverted(Line, Start+1, End);
		}
	}
	else
	{
		FillDEDMatrix(Line, (End - strlen(InputString)), InputString);
		FillDEDMatrix(Line,Start,"\x02",2);
		FillDEDMatrix(Line,End,"\x02",2);
		if(Manual_Input ||  IsSelected)
		{
			MakeInverted(Line, Start, End);
		}
		else
			ClearInverted(Line, Start+1, End-1);
	}
}
