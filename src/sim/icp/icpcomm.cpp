#include "icp.h"
#include "simdrive.h"
#include "aircrft.h"
#include "fsound.h"
#include "falcsnd\voicemanager.h"
#include "navsystem.h"

//MI for ICP stuff
extern bool g_bRealisticAvionics;

#if 0
char *RadioStrings[8] = 
{
				"OFF",
				"FLIGHT",				
				"PACKAGE",
				"FROM PACKAGE",
				"PROXIMITY",
				"GUARD",			
				"BROADCAST",
				"TOWER"
};

void ICPClass::ExecCOMMMode(void) 
{
	
	if(mUpdateFlags & COMM_UPDATE) 
	{
		mUpdateFlags &= !COMM_UPDATE;

		FormatRadioString();
	}
}
#endif

void ICPClass::ExecCOMM1Mode(void)
{

	//change our active radio
	WhichRadio = 0;
	if(VM)
	{
		if(VM->radiofilter[0] == rcfOff)
			CommChannel = 0;
		else if(VM->radiofilter[0] == rcfFlight1)
			CommChannel = 1;
		else if(VM->radiofilter[0] == rcfFlight2)
			CommChannel = 2;
		else if(VM->radiofilter[0] == rcfFlight3)
			CommChannel = 3;
		else if(VM->radiofilter[0] == rcfFlight4)
			CommChannel = 4;
		else if(VM->radiofilter[0] == rcfFlight5)
			CommChannel = 5;
		else if(VM->radiofilter[0] == rcfPackage1)
			CommChannel = 6;
		else if(VM->radiofilter[0] == rcfPackage2)
			CommChannel = 7;
		else if(VM->radiofilter[0] == rcfPackage3)
			CommChannel = 8;
		else if(VM->radiofilter[0] == rcfPackage4)
			CommChannel = 9;
		else if(VM->radiofilter[0] == rcfPackage5)
			CommChannel = 10;
		else if(VM->radiofilter[0] == rcfFromPackage)
			CommChannel = 11;
		else if(VM->radiofilter[0] == rcfProx)
			CommChannel = 12;
		else if(VM->radiofilter[0] == rcfTeam)
			CommChannel = 13;
		else if(VM->radiofilter[0] == rcfAll)
			CommChannel = 14;
		else if(VM->radiofilter[0] == rcfTower)
			CommChannel = 15;
	}
	else
		CommChannel = 8;

	if(PREUHF == 1)
		UHFChann = 125.32;
	else if(PREUHF == 2)
		UHFChann = 106.95;
	else if(PREUHF == 3)
		UHFChann = 140.75;
	else if(PREUHF == 4)
		UHFChann = 123.62;
	else if(PREUHF == 5)
		UHFChann = 142.02;
	else if(PREUHF == 6)
		UHFChann = 185.62;
	else if(PREUHF == 7)
		UHFChann = 262.80;
	else if(PREUHF == 8)
		UHFChann = 142.80;
	else if(PREUHF == 9)
		UHFChann = 322.70;
	else if(PREUHF == 10)
		UHFChann = 362.60;
	else if(PREUHF == 11)
		UHFChann = 322.20;
	else if(PREUHF == 12)
		UHFChann = 362.40;
	else if(PREUHF == 13)
		UHFChann = 222.80;
	else if(PREUHF == 14)
		UHFChann = 263.80;
	else if(PREUHF == 15)//Cobra TJL 10/31/04
		UHFChann = 308.80;
		

	if(gNavigationSys->GetControlSrc() == NavigationSystem::AUXCOMM)
		UHFBackup();
	else
	{
		//Line1
		FillDEDMatrix(0,10,"UHF  BOTH");  // MD -- 20031121: oops, fixed UFH typo
		//Line2
		sprintf(tempstr,"%d",CommChannel);
		FillDEDMatrix(1,9,tempstr);
		//Line3
		PossibleInputs = 5;
		ScratchPad(2,15,22);
		//Line4
		FillDEDMatrix(3,2,"PRE");
		sprintf(tempstr, "%1.0f", PREUHF);
		FillDEDMatrix(3,10,tempstr);
		FillDEDMatrix(3,13,"\x01");
		FillDEDMatrix(3,19, "TOD");
		//Line5
		sprintf(tempstr,"%3.2f",UHFChann);
		FillDEDMatrix(4,6,tempstr);
		FillDEDMatrix(4,20,"NB");
	}
}
void ICPClass::ExecCOMM2Mode(void)
{
	//change our active radio
	WhichRadio = 1;
	if(VM)
	{
		if(VM->radiofilter[1] == rcfOff)
			CommChannel = 0;
		else if(VM->radiofilter[1] == rcfFlight1)
		{
			VHFChann = 145.27;
			CommChannel = 1;
		}
		else if(VM->radiofilter[1] == rcfFlight2)
		{
			VHFChann = 145.27;
			CommChannel = 2;
		}
		else if(VM->radiofilter[1] == rcfFlight3)
		{
			VHFChann = 145.27;
			CommChannel = 3;
		}
		else if(VM->radiofilter[1] == rcfFlight4)
		{
			VHFChann = 145.27;
			CommChannel = 4;
		}
		else if(VM->radiofilter[1] == rcfFlight5)
		{
			VHFChann = 145.27;
			CommChannel = 5;
		}
		else if(VM->radiofilter[1] == rcfPackage1)
		{
			VHFChann = 222.10;
			CommChannel = 6;
		}
		else if(VM->radiofilter[1] == rcfPackage2)
		{
			VHFChann = 222.10;
			CommChannel = 7;
		}
		else if(VM->radiofilter[1] == rcfPackage3)
		{
			VHFChann = 222.10;
			CommChannel = 8;
		}
		else if(VM->radiofilter[1] == rcfPackage4)
		{
			VHFChann = 222.10;
			CommChannel = 9;
		}
		else if(VM->radiofilter[1] == rcfPackage5)
		{
			VHFChann = 222.10;
			CommChannel = 10;
		}
		else if(VM->radiofilter[1] == rcfFromPackage)
		{
			VHFChann = 147.25;
			CommChannel = 3;
		}
		else if(VM->radiofilter[1] == rcfProx)
		{
			VHFChann = 256.32;
			CommChannel = 11;
		}
		else if(VM->radiofilter[1] == rcfTeam)
		{
			VHFChann = 186.35;
			CommChannel = 12;
		}
		else if(VM->radiofilter[1] == rcfAll)
		{
			VHFChann = 198.97;
			CommChannel = 13;
		}
		else if(VM->radiofilter[1] == rcfTower)
		{
			VHFChann = 176.35;
			CommChannel = 14;
		}
	}
	else
		CommChannel = 8;

	if(PREVHF == 1)
		VHFChann = 40.39;
	else if(PREVHF == 2)
		VHFChann = 36.72;
	else if(PREVHF == 3)
		VHFChann = 29.05;
	else if(PREVHF == 4)
		VHFChann = 45.62;
	else if(PREVHF == 5)
		VHFChann = 27.02;
	else if(PREVHF == 6)
		VHFChann = 53.62;
	else if(PREVHF == 7)
		VHFChann = 58.80;
	else if(PREVHF == 8)
		VHFChann = 36.72;
	else if(PREVHF == 9)
		VHFChann = 29.05;
	else if(PREVHF == 10)
		VHFChann = 45.62;
	else if(PREVHF == 11)
		VHFChann = 27.02;
	else if(PREVHF == 12)
		VHFChann = 53.62;
	else if(PREVHF == 13)
		VHFChann = 58.80;
	else if(PREVHF == 14)
		VHFChann = 58.80;
	else if(PREVHF == 15)
  		VHFChann = 122.10;//Cobra 10/31/04 TJL
	if(gNavigationSys->GetControlSrc() == NavigationSystem::AUXCOMM)
		VHFBackup();
	else
	{
		//Line1
		FillDEDMatrix(0,10,"VHF  ON");
		//Line2
		FillDEDMatrix(1,3,"512.26");
		//Line3
		PossibleInputs = 5;
		ScratchPad(2,15,22);
		//Line4
		FillDEDMatrix(3,2,"PRE");
		sprintf(tempstr, "%1.0f", PREVHF);
		FillDEDMatrix(3,10,tempstr);
		FillDEDMatrix(3,13,"\x01");
		//Line5
		sprintf(tempstr, "%3.2f", VHFChann);
		FillDEDMatrix(4,6,tempstr);
		FillDEDMatrix(4,20,"WB");
	}
}
void ICPClass::PNUpdateCOMMMode(int button, int)
{
	if(!g_bRealisticAvionics)
	{
		//MI Original Code
		if(button == PREV_BUTTON) 
		{
			if(gNavigationSys->GetUHFSrc() == NavigationSystem::UHF_NORM) {

				if(GetICPTertiaryMode() == COMM1_MODE) 
				{
					VM->BackwardCycleFreq(0);
				}
				else 
				{
					VM->BackwardCycleFreq(1);
				}
			}
		}
		else 
		{
			if(gNavigationSys->GetUHFSrc() == NavigationSystem::UHF_NORM) 
			{
				if(GetICPTertiaryMode() == COMM1_MODE) 
				{
					VM->ForwardCycleFreq(0);
				}
				else 
				{
					VM->ForwardCycleFreq(1);
				}
			}
		}

		mUpdateFlags |= CNI_UPDATE;
	}
	else
	{
		//MI Modified stuff
		if(button == PREV_BUTTON) 
		{
			if(gNavigationSys->GetUHFSrc() == NavigationSystem::UHF_NORM) 
			{
				if(IsICPSet(ICPClass::EDIT_UHF))
					VM->BackwardCycleFreq(0);
				else if(IsICPSet(ICPClass::EDIT_VHF))
					VM->BackwardCycleFreq(1);
			}
		}
		else 
		{
			if(gNavigationSys->GetUHFSrc() == NavigationSystem::UHF_NORM) 
			{
				if(IsICPSet(ICPClass::EDIT_UHF))
					VM->ForwardCycleFreq(0);
				else if(IsICPSet(ICPClass::EDIT_VHF))
					VM->ForwardCycleFreq(1);
			}
		}
	}
}
void ICPClass::ENTRUpdateCOMMMode() 
{

}

