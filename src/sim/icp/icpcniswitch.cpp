#include "icp.h"
#include "simdrive.h"
#include "aircrft.h"
#include "navsystem.h"
#include "fcc.h"

void ICPClass::CNISwitch(int mode)
{
	if(mode == UP_MODE)
	{
		ClearStrings();

		//CNI Page
		if(IsICPSet(ICPClass::MODE_CNI) && mICPSecondaryMode == NONE_MODE)
		{
			if(gNavigationSys->GetControlSrc() == NavigationSystem::AUXCOMM)
			{
				ClearICPFlag(ICPClass::EDIT_VHF);
				ClearICPFlag(ICPClass::EDIT_UHF);
				//Backup mode
				return;
			}
			else
			{
				// MD -- 20040204: actually the UHF entry is always on line one
				//if(WhichRadio == 0)	//COMM1 is active
				//{
					if(IsICPSet(ICPClass::EDIT_STPT))
					{
						ClearICPFlag(ICPClass::EDIT_STPT);
						SetICPFlag(ICPClass::EDIT_VHF);
					}
					else if(IsICPSet(ICPClass::EDIT_VHF))
					{
						ClearICPFlag(ICPClass::EDIT_VHF);
						SetICPFlag(ICPClass::EDIT_UHF);
					}
					else if(IsICPSet(ICPClass::EDIT_UHF))
					{
						ClearICPFlag(ICPClass::EDIT_UHF);
						SetICPFlag(ICPClass::EDIT_STPT);
					}
				//}
				//else	//COMM2 is active
				//{
				//	if(IsICPSet(ICPClass::EDIT_STPT))
				//	{
				//		ClearICPFlag(ICPClass::EDIT_STPT);
				//		SetICPFlag(ICPClass::EDIT_UHF);
				//	}
				//	else if(IsICPSet(ICPClass::EDIT_UHF))
				//	{
				//		ClearICPFlag(ICPClass::EDIT_UHF);
				//		SetICPFlag(ICPClass::EDIT_VHF);
				//	}
				//	else if(IsICPSet(ICPClass::EDIT_VHF))
				//	{
				//		ClearICPFlag(ICPClass::EDIT_VHF);
				//		SetICPFlag(ICPClass::EDIT_STPT);
				//	}
				//}
			}
		}
		//ILS
		if(IsICPSet(ICPClass::MODE_CNI) && mICPSecondaryMode == ONE_BUTTON)
		{
			if(ILSPageSel == 0)
				ILSPageSel = 1;
			else
				ILSPageSel = 0;
		}
		//ALOW
		if(IsICPSet(ICPClass::MODE_CNI) && mICPSecondaryMode == TWO_BUTTON)
		{
			if(!EDITMSLFLOOR && !TFADV)
				TFADV = TRUE;
			else if(TFADV)
			{
				TFADV = FALSE;
				EDITMSLFLOOR = TRUE;
			}
			else
				EDITMSLFLOOR = FALSE;
		}
		//DEST
		else if(IsICPSet(ICPClass::MODE_LIST) && mICPSecondaryMode == ONE_BUTTON)
		{
			if((IsICPSet(ICPClass::EDIT_LAT) || IsICPSet(ICPClass::EDIT_LONG)) && Manual_Input)
				return;

			if(OA1 ||OA2)
			{
				if(Manual_Input)
					return;

				ClearStrings();
				InputsMade = 0;
				if(OA_RNG)
				{
					OA_RNG = FALSE;
					OA_ALT = TRUE;
				}
				else if(OA_BRG)
				{
					OA_BRG = FALSE;
					OA_RNG = TRUE;
				}
				else if(OA_ALT)
				{
					OA_ALT = FALSE;
					OA_BRG = TRUE;
				}
			}
			else if(IsICPSet(ICPClass::EDIT_LAT))
			{ 
				ClearICPFlag(ICPClass::EDIT_LAT);
				SetICPFlag(ICPClass::EDIT_LONG);
			} 
			else
			{ 
				ClearICPFlag(ICPClass::EDIT_LONG);
				SetICPFlag(ICPClass::EDIT_LAT);
			} 
		}
		//VIP
		else if(IsICPSet(ICPClass::MODE_LIST) && mICPSecondaryMode == THREE_BUTTON)
		{
			if(Manual_Input)
				return;

			ClearStrings();
			InputsMade = 0;
			if(VIP_BRG)
			{
				VIP_BRG = FALSE;
				VIP_ALT = TRUE;
			}
			else if(VIP_RNG)
			{
				VIP_RNG = FALSE;
				VIP_BRG = TRUE;
			}
			else if(VIP_ALT)
			{
				VIP_ALT = FALSE;
				VIP_RNG = TRUE;
			}
		}
		//VRP
		else if(IsICPSet(ICPClass::MODE_LIST) && mICPSecondaryMode == NINE_BUTTON)
		{
			if(Manual_Input)
				return;

			ClearStrings();
			InputsMade = 0;
			if(VRP_BRG)
			{
				VRP_BRG = FALSE;
				VRP_ALT = TRUE;
			}
			else if(VRP_RNG)
			{
				VRP_RNG = FALSE;
				VRP_BRG = TRUE;
			}
			else if(VRP_ALT)
			{
				VRP_ALT = FALSE;
				VRP_RNG = TRUE;
			}
		}
		//EWS
		else if(IsICPSet(ICPClass::MODE_LIST) && mICPSecondaryMode == EWS_MODE)
		{
			if(EWSMain)
			{
				if(Manual_Input)
					return;

				if(IsICPSet(ICPClass::CHAFF_BINGO))
				{
					ClearStrings();
					ClearICPFlag(ICPClass::CHAFF_BINGO);
					SetICPFlag(ICPClass::EWS_EDIT_BINGO);
				}
				else if(IsICPSet(ICPClass::FLARE_BINGO))
				{
					ClearStrings();
					ClearICPFlag(ICPClass::FLARE_BINGO);
					SetICPFlag(ICPClass::CHAFF_BINGO);
					
				}
				else if(IsICPSet(ICPClass::EDIT_JAMMER))
				{
					ClearStrings();
					ClearICPFlag(ICPClass::EDIT_JAMMER);
					SetICPFlag(ICPClass::FLARE_BINGO);
				}
				else
				{
					ClearStrings();
					ClearICPFlag(ICPClass::EWS_EDIT_BINGO);
					SetICPFlag(ICPClass::EDIT_JAMMER);
				}
			}
			else if(PGMChaff || PGMFlare)
			{
				if(Manual_Input)
					return;

				ClearStrings();
				InputsMade = 0;
				if(BQ)
				{
					BQ = FALSE;
					SI = TRUE;
				}
				else if(SI)
				{
					SI = FALSE;
					SQ = TRUE;
				}
				else if(SQ)
				{
					SQ = FALSE;
					BI = TRUE;
				}
				else
				{
					BI = FALSE;
					BQ = TRUE;
				}
			}
		}
		//INS
		else if(IsICPSet(ICPClass::MODE_LIST) && mICPSecondaryMode == SIX_BUTTON)
		{
			INSLine--;
			if(INSLine < 0)
				INSLine = 3;
		}
		//Laser
		else if(IsICPSet(ICPClass::MISC_MODE) && mICPSecondaryMode == FIFE_BUTTON)
		{
			if(LaserLine == 1)
				LaserLine = 2;
			else
				LaserLine = 1;
		}
	}
	else if(mode == DOWN_MODE)
	{
		ClearStrings();

		//CNI Page
		if(IsICPSet(ICPClass::MODE_CNI) && mICPSecondaryMode == NONE_MODE)
		{
			if(gNavigationSys->GetControlSrc() == NavigationSystem::AUXCOMM)
			{
				ClearICPFlag(ICPClass::EDIT_VHF);
				ClearICPFlag(ICPClass::EDIT_UHF);
				//Backup mode
				return;
			}
			else
			{
				// MD -- 20040204: actually the UHF entry is always on line one
				//if(WhichRadio == 0)	//COMM1 active
				//{
					if(IsICPSet(ICPClass::EDIT_STPT))
					{
						ClearICPFlag(ICPClass::EDIT_STPT);
						SetICPFlag(ICPClass::EDIT_UHF);
					}
					else if(IsICPSet(ICPClass::EDIT_UHF))
					{
						ClearICPFlag(ICPClass::EDIT_UHF);
						SetICPFlag(ICPClass::EDIT_VHF);
					}
					else if(IsICPSet(ICPClass::EDIT_VHF))
					{
						ClearICPFlag(ICPClass::EDIT_VHF);
						SetICPFlag(ICPClass::EDIT_STPT);
					}
				//}
				//else	//COMM2 active
				//{
				//	if(IsICPSet(ICPClass::EDIT_STPT))
				//	{
				//		ClearICPFlag(ICPClass::EDIT_STPT);
				//		SetICPFlag(ICPClass::EDIT_VHF);
				//	}
				//	else if(IsICPSet(ICPClass::EDIT_VHF))
				//	{
				//		ClearICPFlag(ICPClass::EDIT_VHF);
				//		SetICPFlag(ICPClass::EDIT_UHF);
				//	}
				//	else if(IsICPSet(ICPClass::EDIT_UHF))
				//	{
				//		ClearICPFlag(ICPClass::EDIT_UHF);
				//		SetICPFlag(ICPClass::EDIT_STPT);
				//	}
				//}
			}
		}
		//ILS
		if(IsICPSet(ICPClass::MODE_CNI) && mICPSecondaryMode == ONE_BUTTON)
		{
			if(ILSPageSel == 0)
				ILSPageSel = 1;
			else
				ILSPageSel = 0;
		}
		//ALOW
		if(IsICPSet(ICPClass::MODE_CNI) && mICPSecondaryMode == TWO_BUTTON)
		{
			if(!EDITMSLFLOOR && !TFADV)
				EDITMSLFLOOR = TRUE;
			else if(EDITMSLFLOOR)
			{
				TFADV = TRUE;
				EDITMSLFLOOR = FALSE;
			}
			else
				TFADV = FALSE;
		}
		//DEST
		else if(IsICPSet(ICPClass::MODE_LIST) && mICPSecondaryMode == ONE_BUTTON)
		{
			if((IsICPSet(ICPClass::EDIT_LAT) || IsICPSet(ICPClass::EDIT_LONG)) && Manual_Input)
				return;

			if(OA1 || OA2)
			{
				if(Manual_Input)
					return;

				ClearStrings();
				InputsMade = 0;
				if(OA_RNG)
				{
					OA_RNG = FALSE;
					OA_BRG = TRUE;
				}
				else if(OA_BRG)
				{
					OA_BRG = FALSE;
					OA_ALT = TRUE;
				}
				else if(OA_ALT)
				{
					OA_ALT = FALSE;
					OA_RNG = TRUE;
				}
			}
			else if(IsICPSet(ICPClass::EDIT_LAT))
			{ 
				ClearICPFlag(ICPClass::EDIT_LAT);
				SetICPFlag(ICPClass::EDIT_LONG);
			} 
			else
			{ 
				ClearICPFlag(ICPClass::EDIT_LONG);
				SetICPFlag(ICPClass::EDIT_LAT);
			}
		}
		//VIP
		else if(IsICPSet(ICPClass::MODE_LIST) && mICPSecondaryMode == THREE_BUTTON)
		{
			if(Manual_Input)
				return;

			ClearStrings();
			InputsMade = 0;
			if(VIP_BRG)
			{
				VIP_BRG = FALSE;
				VIP_RNG = TRUE;
			}
			else if(VIP_RNG)
			{
				VIP_RNG = FALSE;
				VIP_ALT = TRUE;
			}
			else if(VIP_ALT)
			{
				VIP_ALT = FALSE;
				VIP_BRG = TRUE;
			}
		}
		//VRP
		else if(IsICPSet(ICPClass::MODE_LIST) && mICPSecondaryMode == NINE_BUTTON)
		{
			if(Manual_Input)
				return;

			ClearStrings();
			InputsMade = 0;	
			if(VRP_BRG)
			{
				VRP_BRG = FALSE;
				VRP_RNG = TRUE;
			}
			else if(VRP_RNG)
			{
				VRP_RNG = FALSE;
				VRP_ALT = TRUE;
			}
			else if(VRP_ALT)
			{
				VRP_ALT = FALSE;
				VRP_BRG = TRUE;
			}			
		}
		//EWS
		else if(IsICPSet(ICPClass::MODE_LIST) && mICPSecondaryMode == EWS_MODE)
		{
			if(EWSMain)
			{
				if(Manual_Input)
					return;

				ClearStrings();
				InputsMade = 0;
				if(IsICPSet(ICPClass::CHAFF_BINGO))
				{
					ClearICPFlag(ICPClass::CHAFF_BINGO);
					SetICPFlag(ICPClass::FLARE_BINGO);
				}
				else if(IsICPSet(ICPClass::FLARE_BINGO))
				{
					ClearICPFlag(ICPClass::FLARE_BINGO);
					SetICPFlag(ICPClass::EDIT_JAMMER);
				}
				else if(IsICPSet(ICPClass::EDIT_JAMMER))
				{
					ClearICPFlag(ICPClass::EDIT_JAMMER);
					SetICPFlag(ICPClass::EWS_EDIT_BINGO);
				}
				else
				{
					ClearICPFlag(ICPClass::EWS_EDIT_BINGO);
					SetICPFlag(ICPClass::CHAFF_BINGO);
				}
			}
			else if(PGMChaff || PGMFlare)
			{
				if(Manual_Input)
					return;

				ClearStrings();
				InputsMade = 0;
				if(BQ)
				{
					BQ = FALSE;
					BI = TRUE;
				}
				else if(BI)
				{
					BI = FALSE;
					SQ = TRUE;
				}
				else if(SQ)
				{
					SQ = FALSE;
					SI = TRUE;
				}
				else
				{
					SI = FALSE;
					BQ = TRUE;
				}
			}
		}
		//INS
		else if(IsICPSet(ICPClass::MODE_LIST) && mICPSecondaryMode == SIX_BUTTON)
		{
			INSLine++;
			if(INSLine > 3)
				INSLine = 0;
		}
		//Laser
		else if(IsICPSet(ICPClass::MISC_MODE) && mICPSecondaryMode == FIFE_BUTTON)
		{
			if(LaserLine == 1)
				LaserLine = 2;
			else
				LaserLine = 1;
		}
	}
	else if(mode == SEQ_MODE)
	{
		ClearDigits();

		if(!IsICPSet(ICPClass::MODE_FACK))
			ClearStrings();

		if(IsICPSet(ICPClass::MODE_CNI) && mICPSecondaryMode == NONE_MODE)
		{
			if(ShowWind == TRUE)
			{
				memset(DEDLines[1], ' ', MAX_DED_LEN - 1);
				ShowWind = FALSE;
			}
			else
				ShowWind = TRUE;
		}
		if(IsICPSet(ICPClass::MODE_CNI) && mICPSecondaryMode == ONE_BUTTON)
		{
			if(gNavigationSys->GetControlSrc() == NavigationSystem::AUXCOMM)
			{
				ClearICPFlag(ICPClass::EDIT_STPT);
				ClearICPFlag(ICPClass::EDIT_VHF);
				ClearICPFlag(ICPClass::EDIT_UHF);
				//Backup mode
				return;
			}
			else
			{
				if(gNavigationSys)
					gNavigationSys->ToggleDomain(NavigationSystem::ICP);
			}
		}
		else if(IsICPSet(ICPClass::MODE_CNI) && mICPSecondaryMode == FOUR_BUTTON)
		{
			if(MAN)
				MAN = FALSE;
			else
				MAN = TRUE;
		}
		else if(IsICPSet(ICPClass::MODE_CNI) && mICPSecondaryMode == FIFE_BUTTON)
			StepCruise();
		else if(IsICPSet(ICPClass::MODE_LIST) && mICPSecondaryMode == ONE_BUTTON)
		{
			ClearStrings();
			ClearCount = 0;
			Manual_Input = FALSE;

			if(OA1)
			{
				OA1 = FALSE;
				OA2 = TRUE;
				OA_RNG = TRUE;
				OA_BRG = FALSE;
				OA_ALT = FALSE;
			}
			else if(OA2)
			{
				OA2 = FALSE;
				OA_RNG = FALSE;
				OA_BRG = FALSE;
				OA_ALT = FALSE;
				SetICPFlag(ICPClass::EDIT_LAT);
			}
			else
			{
				OA1 = TRUE;
				OA_RNG = TRUE;
				OA_BRG = FALSE;
				OA_ALT = FALSE;
				ClearICPFlag(ICPClass::EDIT_LAT);
				ClearICPFlag(ICPClass::EDIT_LONG);
			}
		}
		//EWS
		else if(IsICPSet(ICPClass::MODE_LIST) && mICPSecondaryMode == EWS_MODE)
		{
			Manual_Input = FALSE;
			ClearCount = 0;
			InputsMade = 0;
			ClearStrings();
			
			if(EWSMain)
			{
				EWSMain = FALSE;
				PGMChaff = TRUE;
				BQ = TRUE;
				BI = FALSE;
				SQ = FALSE;
				SI = FALSE;
				ClearICPFlag(ICPClass::CHAFF_BINGO);
				ClearICPFlag(ICPClass::FLARE_BINGO);
				ClearICPFlag(ICPClass::EDIT_JAMMER);
				ClearICPFlag(ICPClass::EWS_EDIT_BINGO);
			}
			else if(PGMChaff)
			{
				PGMChaff = FALSE;
				PGMFlare = TRUE;
				BQ = TRUE;
				BI = FALSE;
				SQ = FALSE;
				SI = FALSE;
			}
			else
			{
				PGMFlare = FALSE;
				EWSMain = TRUE;
				BQ = FALSE;
				BI = FALSE;
				SQ = FALSE;
				SI = FALSE;
				SetICPFlag(ICPClass::CHAFF_BINGO);
			}
		}
		if(IsICPSet(ICPClass::MODE_LIST) && mICPSecondaryMode == EIGHT_BUTTON)
		{
			ClearCount = 0;
			InputsMade = 0;
			if(IN_AA)
			{
				IN_AA = FALSE;
				IN_AG = TRUE;
			}
			else
			{
				IN_AG = FALSE;
				IN_AA = TRUE;
			}
		}
	}
	else if(mode == CNI_MODE)
		ChangeToCNI();
}