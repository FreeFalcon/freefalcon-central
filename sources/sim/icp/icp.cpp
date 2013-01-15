#include "falclib.h"
#include "f4error.h"
#include "Graphics\Include\imagebuf.h"
#include "Graphics\Include\render2d.h"
#include "vu2.h"
#include "entity.h"
#include "fcc.h"
#include "campwp.h"
#include "aircrft.h"
#include "cpmanager.h"
#include "button.h"
#include "icp.h"
#include "simdrive.h"
#include "camp2sim.h"
#include "navsystem.h"
#include "fsound.h"
#include "falcsnd\voicemanager.h"
#include "falcsnd\voicefilter.h"
#include "airframe.h"
#include "fack.h"
#include "cmpclass.h"

// ASSO: //Cobra 10/31/04 TJL
const int NUM_COMM_CHANNELS = 15;

//MI for ICP stuff
extern bool g_bRealisticAvionics;
#include "hud.h"

char*	mpPointTypeNames[] = {"NONE", "GM", "POS"};

char *ICPModeNames[NUM_ICP_MODES] = {
				"STPT",
				"DLINK",
				"MARK",
				"ILS",
				"CRUISE",
				"COMM1",
				"COMM2",
				"FAULT",
				"ALOW",
				"NAV",
				"LIST",	//MI for new ICP stuff
				"IFF",	//MI for new ICP stuff
				"AA",
				"AG"
};

//==================================================//
//	ICPClass::~ICPClass
//==================================================//

ICPClass::~ICPClass() {

}

//==================================================//
//	ICPClass::ICPClass
//==================================================//

ICPClass::ICPClass() {

	if(!g_bRealisticAvionics)
	{
		//MI Original code
		strcpy(mpSelectedModeName, ICPModeNames[NAV_MODE]);
		mICPPrimaryMode				= AA_MODE;
		mICPSecondaryMode			= NONE_MODE;
		mICPTertiaryMode			= COMM1_MODE;
		mpTertiaryExclusiveButton	= NULL;
		mpSecondaryExclusiveButton	= NULL;
		mpPrimaryExclusiveButton	= NULL;

		mWPIndex					= 0;
		mMarkIndex				= 0;
		mDLinkIndex				= 0;
		mList						= STPT_LIST;
		HomeWP = 1;

		*mpLine1 = '\0';
		*mpLine2 = '\0';
		*mpLine3 = '\0';

		//Wombat778 10-20-2003  Initialize the PFL in easy avionics. Should fix easy avionics CTD. Should I just put the whole InitStuff() line here?
		m_FaultDisplay = false;
		m_subsystem = FaultClass::amux_fault;
		m_function = FaultClass::nofault;
		//end of PFL Initialization

		mUpdateFlags = CNI_UPDATE;
	}
	else
	{
		//MI modified for ICP stuff
		strcpy(mpSelectedModeName, ICPModeNames[NAV_MODE]);

		mICPPrimaryMode					= NAV_MODE;
		mICPSecondaryMode				= NONE_MODE;
		mICPTertiaryMode				= NONE_MODE;
		mpTertiaryExclusiveButton		= NULL;
		mpSecondaryExclusiveButton		= NULL;
		mpPrimaryExclusiveButton		= NULL;
		LastMode						= CNI_MODE;

		mWPIndex						= 0;
		mMarkIndex						= 0;
		mDLinkIndex						= 0;
		mList							= STPT_LIST;

		*mpLine1 = '\0';
		*mpLine2 = '\0';
		*mpLine3 = '\0';

		//Init our stuff
		InitStuff();

		mUpdateFlags = CNI_UPDATE;
	}
}


//==================================================//
//	ICPClass::GetTertiaryExclusiveButton
//==================================================//
CPButtonObject* ICPClass::GetTertiaryExclusiveButton(void) 
{
		return mpTertiaryExclusiveButton;
}

 
//==================================================//
//	ICPClass::GetPrimaryExclusiveButton
//==================================================//

CPButtonObject* ICPClass::GetPrimaryExclusiveButton(void) 
{
		return mpPrimaryExclusiveButton;
}


//==================================================//
//	ICPClass::GetSecondaryExclusiveButton
//==================================================//

CPButtonObject* ICPClass::GetSecondaryExclusiveButton(void) 
{
		return mpSecondaryExclusiveButton;
}

//==================================================//
//	ICPClass::InitPrimaryExclusiveButton
//==================================================//

void ICPClass::InitPrimaryExclusiveButton(CPButtonObject *pbutton) 
{
		mpPrimaryExclusiveButton = pbutton;
		mpPrimaryExclusiveButton->SetCurrentState(1);
}

//==================================================//
//	ICPClass::InitTertiaryExclusiveButton
//==================================================//

void ICPClass::InitTertiaryExclusiveButton(CPButtonObject *pbutton) 
{
		mpTertiaryExclusiveButton = pbutton;
		mpTertiaryExclusiveButton->SetCurrentState(1);
}

//==================================================//
//	ICPClass::SetOwnship
//==================================================//

void ICPClass::SetOwnship(void) 
{
	
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

	if(playerAC && playerAC->IsAirplane()) // 2002-02-15 MODIFIED BY S.G. Added the IsAirplane test since it could be an ejected pilot
	{

		// Waypoint Stuff
		mpWayPoints				= playerAC->waypoint; // head of the waypoint list
		mNumWayPts				= (BYTE) playerAC->numWaypoints;

		mCruiseWPIndex			= 0;
		mpCruiseWP				= mpWayPoints;
		mList					= STPT_LIST;

		// Fault stuff
		mFaultNum = 0;
		mFaultFunc = 0;
	}
	else { // 2002-02-15 ADDED BY S.G. Clear it up if we can't get the player's aircraft info
		// Waypoint Stuff
		mpWayPoints				= NULL;
		mNumWayPts				= 0;

		mCruiseWPIndex			= 0;
		mpCruiseWP				= mpWayPoints;
		mList					= STPT_LIST;

		// Fault stuff
		mFaultNum = 0;
		mFaultFunc = 0;
	}
}

#include "falcsnd\winampfrontend.h"	// Retro 3Jan2004
extern bool g_bPilotEntertainment;	// Retro 3Jan2004

//==================================================//
//	ICPClass::HandleInput
//==================================================//


void ICPClass::HandleInput(int mode, CPButtonObject *pbutton) 
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

	if(!g_bRealisticAvionics)
	{
		//MI original code
		if(mode == AA_BUTTON || mode == AG_BUTTON || mode == NAV_BUTTON) 
		{
			if(pbutton != mpPrimaryExclusiveButton) 
			{
				mpPrimaryExclusiveButton->SetCurrentState(0);
				mICPPrimaryMode				= mode;
				mpPrimaryExclusiveButton	= pbutton;
				if(mICPSecondaryMode == NONE_MODE) 
				{
					mUpdateFlags |= CNI_UPDATE;
					ExecCNIMode();
				}
			}
		}
		else if(mode == PREV_BUTTON || mode == NEXT_BUTTON) 
		{
			HandlePrevNext(mode, mICPSecondaryMode);
		}
		else if(mode == ENTR_BUTTON) 
		{
			HandleENTR(mICPSecondaryMode);
		}
		else if(mode == COMM1_BUTTON || mode == COMM2_BUTTON) 
		{
			if(pbutton != mpTertiaryExclusiveButton) 
			{
				mpTertiaryExclusiveButton->SetCurrentState(0);
				mICPTertiaryMode				= mode;
				mpTertiaryExclusiveButton	= pbutton;
				mUpdateFlags |= CNI_UPDATE;
				if(VM) {
					if(mode == COMM1_BUTTON) 
					{
						VM->SetRadio(0);
					}
					else 
					{
						VM->SetRadio(1);
					}
				}
			}				
		}
		else 
		{
			if(pbutton == mpSecondaryExclusiveButton) 
			{
				mICPSecondaryMode = NONE_MODE;
				mpSecondaryExclusiveButton->SetCurrentState(0);
				mpSecondaryExclusiveButton  = NULL;
				mUpdateFlags |= CNI_UPDATE;
				ExecCNIMode();
			}
			else 
			{
				if(mpSecondaryExclusiveButton != NULL) 
				{
					mpSecondaryExclusiveButton->SetCurrentState(0);			
				}

				mICPSecondaryMode				= mode;
				mpSecondaryExclusiveButton = pbutton;


				switch(mode) 
				{
				case NONE_MODE:
					break;
				case STPT_BUTTON:
					playerAC->FCC->SetStptMode(FireControlComputer::FCCWaypoint);
					playerAC->FCC->waypointStepCmd = 127;
					mUpdateFlags |= STPT_UPDATE;
					break;
				case DLINK_BUTTON:
					playerAC->FCC->SetStptMode(FireControlComputer::FCCDLinkpoint);
					playerAC->FCC->waypointStepCmd = 127;
					mUpdateFlags |= DLINK_UPDATE;
					ExecDLINKMode();
					break;
				case MARK_BUTTON:
					playerAC->FCC->SetStptMode(FireControlComputer::FCCMarkpoint);
					playerAC->FCC->waypointStepCmd = 127;
					mUpdateFlags |= MARK_UPDATE;
					ExecMARKMode();
					break;
				case ILS_BUTTON:
					mUpdateFlags |= ILS_UPDATE;
					ExecILSMode();
					break;
				case CRUS_BUTTON:
					mUpdateFlags |= CRUS_UPDATE;
					ExecCRUSMode();
					break;
				case FACK_BUTTON:
					mUpdateFlags |= FACK_UPDATE;
				PNUpdateFACKMode (NEXT_BUTTON, FACK_BUTTON);
					ExecFACKMode();
					break;
				case ALOW_BUTTON:
					mUpdateFlags |= ALOW_UPDATE;
					break;
				}
			}
		}
	}
	else
	{
		if(!playerAC->HasPower(AircraftClass::UFCPower) ||
			playerAC->mFaults->GetFault(FaultClass::ufc_fault))
			return;

		//Master Modes
		if(mode == AA_BUTTON || mode == AG_BUTTON || mode == NAV_BUTTON) 
		{
			mICPPrimaryMode	= mode;
			if(IsICPSet(ICPClass::MODE_LIST) && mICPSecondaryMode == EIGHT_BUTTON)
				ExecMODEMode();
			if(mode == AA_BUTTON)
			{
				IN_AA = TRUE;
				IN_AG = FALSE;
			}
			else if(mode == AG_BUTTON)
			{
				IN_AA = FALSE;
				IN_AG = TRUE;
			}
		}

		/****************************************************/
		/*These are OVERRIDE buttons. With each of these	*/
		/*we can get into specific DED pages, no matter in	*/
		/*what mode we've been before.						*/
		/****************************************************/

		else if(mode == COMM1_BUTTON || mode == COMM2_BUTTON ||
			mode == LIST_BUTTON || mode == IFF_BUTTON) 
		{
			//This results in a new DED page, clear our strings
			ClearStrings();

			//Did we push the same button as before?
			if(mICPTertiaryMode == mode && mode != CNI_MODE)
				PushedSame(LastMode);
			else
				NewMode(mode);
		}
		//END OVERRIDE FUNCTIONS
		//special case, FACK
		else if(mode == FACK_BUTTON)
		{
		    if (playerAC && playerAC->mFaults) { 
			if (m_FaultDisplay == false) { // was off
			    m_FaultDisplay = true; // now on
			    //have a fault, update our display
			    playerAC->mFaults->GetFirstFault(&m_subsystem, &m_function);
			}
			else { // move to next fault
			    if (playerAC->mFaults->GetFFaultCount() <= 0 ||
				playerAC->mFaults->GetNextFault(&m_subsystem, &m_function) == FALSE)
				m_FaultDisplay = false;
			}
			mUpdateFlags |= FACK_UPDATE; // we need to do some work
		    }
		}

		//SENCONDAR FUNCTIONS
		else if(mode == PREV_BUTTON || mode == NEXT_BUTTON) 
		{
			ClearStrings();
			if(IsICPSet(ICPClass::EDIT_STPT))
				PNUpdateSTPTMode(mode, 0);

			else if(mICPSecondaryMode == CRUS_MODE)
				StepHOMERNGSTPT(mode);
			else if(IsICPSet(ICPClass::MODE_CNI) && mICPSecondaryMode == SEVEN_BUTTON)
				PNUpdateMARKMode(mode, 0);

			else if(IsICPSet(ICPClass::EDIT_VHF) || IsICPSet(ICPClass::EDIT_UHF))
				PNUpdateCOMMMode(mode, 0);

			else if(IsICPSet(ICPClass::MODE_DLINK))
				PNUpdateDLINKMode(mode, 0);

			else if(IsICPSet(ICPClass::MODE_COMM1))//Cobra 10/31/04 TJL
			{
				if(mode == NEXT_BUTTON)
				{
					if(PREUHF == NUM_COMM_CHANNELS)	// ASSO:
						PREUHF = 1;
					else
						PREUHF++;
				}
				else
				{
					if(PREUHF == 1)
						PREUHF = NUM_COMM_CHANNELS;	// ASSO:
					else
						PREUHF--;
				}
			}
			else if(IsICPSet(ICPClass::MODE_COMM2))
			{
				if(mode == NEXT_BUTTON)
				{
					if(PREVHF == NUM_COMM_CHANNELS)	// ASSO:
						PREVHF = 1;
					else
						PREVHF++;
				}
				else
				{
					if(PREVHF == 1)
						PREVHF = NUM_COMM_CHANNELS;	// ASSO:
					else
						PREVHF--;
				}
			}
			else if(IsICPSet(ICPClass::MODE_CNI) && mICPSecondaryMode == SIX_BUTTON)
			{
				if(mode == NEXT_BUTTON)
				{
					if(running)
					{
						running = FALSE;
						stopped = TRUE;
					}
					else if(stopped)
					{
						running = TRUE;
						stopped = FALSE;
					}
					else
					{
						Start = vuxGameTime;
						running = TRUE;
					}
				}
				else
				{
					Start = vuxGameTime;
					Difference = 0;
					running = FALSE;
					stopped = FALSE;
				}
			}
			else if(IsICPSet(ICPClass::MODE_LIST) && mICPSecondaryMode == EWS_MODE)
			{
				if(PGMFlare || PGMChaff)
					StepEWSProg(mode);
				else
					PNUpdateSTPTMode(mode, 0);
			}
			// Retro 3Jan2004 start
			else if ((g_bPilotEntertainment)&&(winamp)&&(IsICPSet(ICPClass::MISC_MODE) && mICPSecondaryMode == ZERO_MODE))
			{
				if (mode == PREV_BUTTON)
					winamp->VolDown();
				else if (mode == NEXT_BUTTON)
					winamp->VolUp();
			} // Retro 3Jan2004 end
		}
		else if(mode == ENTR_BUTTON) 
			ICPEnter();
		else if(mode == CLEAR_BUTTON)
		{
			if(IsICPSet(ICPClass::MODE_LIST) && mICPSecondaryMode == 0)
			{
				ClearStrings();
				ExecINTGMode();
				mICPSecondaryMode = 100;	//small hack
			}
			else
				ClearInput();
		}
		else if (mode == UP_MODE || mode == DOWN_MODE || mode == SEQ_MODE || mode == CNI_BUTTON)
		{
			if(mode == CNI_MODE)
				mICPTertiaryMode = CNI_MODE;
			CNISwitch(mode);

			// Retro 3Jan2004 start
			// Retro from here, kind of a hack having this here..
			if ((g_bPilotEntertainment)&&(winamp)&&(IsICPSet(ICPClass::MISC_MODE) && mICPSecondaryMode == ZERO_MODE))
			{
				if (mode == UP_MODE)
					winamp->Next();
				else if (mode == DOWN_MODE)
					winamp->Previous();
				else if (mode == SEQ_MODE)
					winamp->TogglePlayback();
			} // Retro 3Jan2004 end
		}
		//******************
		//******************
		//SECONDARY BUTTONS*
		//******************
		//******************

		//'1' BUTTON
		else if(mode == ONE_BUTTON)
		{
			if(CheckMode())
				return;
			else if(ManualInput())
				HandleManualInput(mode);
			else if(IsICPSet(ICPClass::EDIT_JAMMER) || IsICPSet(ICPClass::EWS_EDIT_BINGO))
				EWSOnOff();
			else
				OneButton(mode);
		}
		//'2' BUTTON
		else if(mode == TWO_BUTTON)
		{
			if(CheckMode())
				return;
			else if(ManualInput())
				HandleManualInput(mode);
			else if(IsICPSet(ICPClass::EDIT_JAMMER) || IsICPSet(ICPClass::EWS_EDIT_BINGO))
				EWSOnOff();
			else
				TwoButton(mode);
		}
		//'3' BUTTON
		else if(mode == THREE_BUTTON)
		{
			if(CheckMode())
				return;
			else if(ManualInput())
				HandleManualInput(mode);
			else if(IsICPSet(ICPClass::EDIT_JAMMER) || IsICPSet(ICPClass::EWS_EDIT_BINGO))
				EWSOnOff();
			else
				ThreeButton(mode);
		}
		//'4' BUTTON
		else if(mode  == FOUR_BUTTON)
		{
			if(CheckMode())
				return;
			else if(ManualInput())
				HandleManualInput(mode);
			else if(IsICPSet(ICPClass::EDIT_JAMMER) || IsICPSet(ICPClass::EWS_EDIT_BINGO))
				EWSOnOff();
			else
				FourButton(mode);
		}
		//'5' BUTTON
		else if(mode == FIFE_BUTTON)
		{
			if(CheckMode())
				return;
			else if(ManualInput())
				HandleManualInput(mode);
			else if(IsICPSet(ICPClass::EDIT_JAMMER) || IsICPSet(ICPClass::EWS_EDIT_BINGO))
				EWSOnOff();
			else
				FifeButton(mode);
		}
		//'6' BUTTON
		else if(mode == SIX_BUTTON)
		{
			if(CheckMode())
				return;
			else if(ManualInput())
				HandleManualInput(mode);
			else if(IsICPSet(ICPClass::EDIT_JAMMER) || IsICPSet(ICPClass::EWS_EDIT_BINGO))
				EWSOnOff();
			else
				SixButton(mode);
		}
		//'7' BUTTON
		else if(mode == SEVEN_BUTTON)
		{
			if(CheckMode())
				return;
			else if(ManualInput())
				HandleManualInput(mode);
			else if(IsICPSet(ICPClass::EDIT_JAMMER) || IsICPSet(ICPClass::EWS_EDIT_BINGO))
				EWSOnOff();
			else
				SevenButton(mode);
		}
		//'8' BUTTON
		else if(mode == EIGHT_BUTTON)
		{
			if(CheckMode())
				return;
			else if(ManualInput())
				HandleManualInput(mode);
			else if(IsICPSet(ICPClass::EDIT_JAMMER) || IsICPSet(ICPClass::EWS_EDIT_BINGO))
				EWSOnOff();
			else
				EightButton(mode);
		}
		//'9' BUTTON
		else if(mode == NINE_BUTTON)
		{
			if(CheckMode())
				return;
			else if(ManualInput())
				HandleManualInput(mode);
			else if(IsICPSet(ICPClass::EDIT_JAMMER) || IsICPSet(ICPClass::EWS_EDIT_BINGO))
				EWSOnOff();
			else
				NineButton(mode);
		}
		//'0' BUTTON
		else if(mode == ZERO_BUTTON)
		{
			if((IsICPSet(ICPClass::MODE_CNI) && mICPSecondaryMode == FIFE_BUTTON) && !IsICPSet(ICPClass::BLOCK_MODE))
			{
				if(Cruise_RNG)
				{
					if(GetCruiseIndex() == 1)
						SetCruiseIndex(-1);
					else
						SetCruiseIndex(1);
				}
				else if(Cruise_HOME)
				{
					if(GetCruiseIndex() == 2)
						SetCruiseIndex(-1);
					else
						SetCruiseIndex(2);
				}
				else if(Cruise_EDR)
				{
					if(GetCruiseIndex() == 3)
						SetCruiseIndex(-1);
					else
						SetCruiseIndex(3);
				}
				else
				{
					if(GetCruiseIndex() == 0)
						SetCruiseIndex(-1);
					else
						SetCruiseIndex(0);
				}
				return;
			}
			else if(IsICPSet(ICPClass::MISC_MODE) && mICPSecondaryMode == EIGHT_BUTTON)
			{
				if(ShowBullseyeInfo)
					ShowBullseyeInfo = FALSE;
				else
					ShowBullseyeInfo = TRUE;
				return;
			}
			if(CheckMode())
				return;
			else if(IsICPSet(ICPClass::EDIT_JAMMER) || IsICPSet(ICPClass::EWS_EDIT_BINGO))
				return;
			else if(ManualInput())
				HandleManualInput(mode);
			else
				ZeroButton(mode);
		}
		/*else if(mode == FACK_BUTTON)
		{
			ClearStrings();
			ClearFlags();
			SetICPFlag(ICPClass::MODE_FACK);
			PNUpdateFACKMode (NEXT_BUTTON, FACK_BUTTON);
			ExecFACKMode();
		}*/
	}
}


//==================================================//
//	ICPClass::Exec
//==================================================//

void ICPClass::Exec()
{
	if(!g_bRealisticAvionics)
	{
		//MI original code
		switch (mICPSecondaryMode)
		{
		case NONE_MODE:
			ExecCNIMode();
			break;
		case STPT_MODE:
			ExecSTPTMode();
			break;
		case DLINK_MODE:
			ExecDLINKMode();
			break;
		case MARK_MODE:
			ExecMARKMode();
			break;
		case ILS_MODE:
			ExecILSMode();
			break;
		case CRUS_MODE:
			ExecCRUSMode();
			break;
	#if 0	
		case COMM1_MODE:
		case COMM2_MODE:
			ExecCNIMode();
			break;
	#endif
		case FACK_MODE:
			ExecFACKMode();
			break;
		case ALOW_MODE:
			ExecALOWMode();
			break;
		default:
			ShiWarning ("BAD ICP Mode");
			break;
		}
	}
	else
	{
		//for flashing stuff
		flash = (vuxRealTime & 0x180);

		//do this once here
		if(FillStrings)
		{
			GetINSInfo();
			FillStrings = FALSE;
		}
		
		//automaticaly switch waypoints when in parameters and selected
		if(!MAN)
			CheckAutoSTPT();

		if(IsICPSet(ICPClass::MODE_FACK) || TheHud && TheHud->GetDEDSwitch() == HudClass::PFL_DATA)
			ExecFACKMode();

		if(IsICPSet(ICPClass::MODE_COMM1))
		{
			ExecCOMM1Mode();
			return;
		}
		else if(IsICPSet(ICPClass::MODE_COMM2))
		{
			ExecCOMM2Mode();
			return;
		}
		else if(IsICPSet(ICPClass::MODE_IFF))
			ExecIFFMode();
		
		else if(IsICPSet(ICPClass::MODE_CNI))
		{
			switch(mICPSecondaryMode)
			{
			case NONE_MODE:
				ExecCNIMode();
				break;
			case ONE_BUTTON:
				ExecILSMode();
				break;
			case TWO_BUTTON:
				ExecALOWMode();
				break;
			case THREE_BUTTON:
				ExecFACKMode();
				break;
			case FOUR_BUTTON:
				ExecSTPTMode();
				break;
			case FIFE_BUTTON:
				ExecCRUSMode();
				break;
			case SIX_BUTTON:
				ExecTimeMode();
				break;
			case SEVEN_BUTTON:
				ExecMARKMode();
				break;
			case EIGHT_BUTTON:
				ExecFIXMode();
				break;
			case NINE_BUTTON:
				ExecACALMode();
				break;
			default:
				break;
			}
		}
		else if(IsICPSet(ICPClass::MODE_LIST))
		{
			switch(mICPSecondaryMode)
			{
			case ONE_BUTTON:
				if(OA1)
					ExecOA1Mode();
				else if(OA2)
					ExecOA2Mode();
				else
					ExecDESTMode();
				break;
			case TWO_BUTTON:
				ExecBingo();
				break;
			case THREE_BUTTON:
				ExecVIPMode();
				break;
			case FOUR_BUTTON:
				ExecNAVMode();
				break;
			case FIFE_BUTTON:
				ExecMANMode();
				break;
			case SIX_BUTTON:
				ExecINSMode();
				break;
			case EWS_MODE:
				if(EWSMain)
					ExecEWSMode();
				else if(PGMChaff)
					ChaffPGM();
				else
					FlarePGM();
				break;
			case EIGHT_BUTTON:
				ExecMODEMode();
				break;
			case NINE_BUTTON:
				ExecVRPMode();
				break;
			case ZERO_BUTTON:
				ExecMISCMode();
				break;
			case 100:
				ExecINTGMode();
				break;
			case NONE_MODE:
				ExecLISTMode();
				break;
			default:
				break;
			}
		}
		else if(IsICPSet(ICPClass::MISC_MODE))
		{
			switch(mICPSecondaryMode)
			{
			case ONE_BUTTON:
				ExecCORRMode();
				break;
			case TWO_BUTTON:
				ExecMAGVMode();
				break;
			case THREE_BUTTON:
				ExecOFPMode();
				break;
			case FOUR_BUTTON:
				ExecINSMMode();
				break;
			case FIFE_BUTTON:
				ExecLASRMode();
				break;
			case SIX_BUTTON:
				ExecGPSMode();
				break;
			case SEVEN_BUTTON:
				ExecDRNGMode();
				break;
			case EIGHT_BUTTON:
				ExecBullMode();
				break;
			case NINE_BUTTON:
				ExecWPTMode();
				break;
			case ZERO_BUTTON:
				{	// Retro 3Jan2004 start
					if (g_bPilotEntertainment == false)
						ExecHARMMode();
					else
						ExecWinAmpMode();
					break;
				}	// Retro 3Jan2004 end
			case NONE_MODE:
				ExecMISCMode();
			default:
				break;
			}
		}
	}
}

//==================================================//
//	ICPClass::HandleENTR
//==================================================//

void ICPClass::HandleENTR(int mode){

	switch(mode) {
		case MARK_MODE:
			ENTRUpdateMARKMode();
		break;

		case ILS_MODE:
			ENTRUpdateILSMode();
		break;

		case COMM1_MODE:
		case COMM2_MODE:
			ENTRUpdateCOMMMode();
		break;
	}
}


//==================================================//
//	ICPClass::HandlePrevNext
//==================================================//

void ICPClass::HandlePrevNext(int button, int mode){
	// critial section this
	switch(mode){
		case STPT_MODE:
			PNUpdateSTPTMode(button, mode);
		break;

		case DLINK_MODE:
			PNUpdateDLINKMode(button, mode);
		break;

		case MARK_MODE:
			PNUpdateMARKMode(button, mode);
		break;

		case ILS_MODE:
			PNUpdateILSMode(button, mode);
		break;

		case CRUS_MODE:
			PNUpdateCRUSMode(button, mode);
		break;

		case FACK_MODE:
			PNUpdateFACKMode(button, mode);
		break;

		case ALOW_MODE:
			PNUpdateALOWMode(button, mode);
		break;
		
		case NONE_MODE:
		case COMM1_MODE:
		case COMM2_MODE:
			PNUpdateCOMMMode(button, mode);
		break;

		default:
			ShiWarning ("BAD ICP Mode");
		break;
	}
}
void ICPClass::GetDEDStrings(char* pstr1, char* pstr2, char* pstr3) 
{
	strcpy(pstr1, mpLine1);
	strcpy(pstr2, mpLine2);
	strcpy(pstr3, mpLine3);
}
void ICPClass::NewMode(int mode)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

	//Clear our Flags
	ClearFlags();
	//Clear our strings
	ClearStrings();
	
	LastMode = mICPTertiaryMode;
	mICPTertiaryMode = mode;

	ResetSubPages();
	Manual_Input = FALSE;
	MadeInput = FALSE;

	ClearCount = 0;
	InputsMade = 0;

	playerAC->FCC->SetStptMode(FireControlComputer::FCCWaypoint);
	playerAC->FCC->waypointStepCmd = 127;

	switch (mode)
	{
	case COMM1_BUTTON:
		LeaveCNI();
		SetICPFlag(ICPClass::MODE_COMM1);
		if(VM) 
			VM->SetRadio(0);
		WhichRadio = 0;
		ExecCOMM1Mode();
		break;
	case COMM2_BUTTON:
		LeaveCNI();
		SetICPFlag(ICPClass::MODE_COMM2);
		if(VM) 
			VM->SetRadio(1);
		WhichRadio = 1;
		ExecCOMM2Mode();
		break;
	case LIST_BUTTON:
		LeaveCNI();
		SetICPFlag(ICPClass::MODE_LIST);
		SetICPFlag(ICPClass::EDIT_STPT);
		ExecLISTMode();
		break;
	case IFF_BUTTON:
		LeaveCNI();
		SetICPFlag(ICPClass::MODE_IFF);
		ExecIFFMode();
		break;
	default:
		break;
	}
}
void ICPClass::ChangeToCNI(void)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

	ResetSubPages();
	if (playerAC && playerAC->FCC)
	{
		playerAC->FCC->SetStptMode(FireControlComputer::FCCWaypoint);
		playerAC->FCC->waypointStepCmd = 127;
	}
	ClearFlags();
	ClearStrings();
	ClearString();
	ClearDigits();
	ClearCount = 0;
	InputsMade = 0;
	SetICPFlag(ICPClass::MODE_CNI);
	SetICPFlag(ICPClass::EDIT_STPT);
	LaserLine = 1;
}