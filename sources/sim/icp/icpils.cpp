#include "stdhdr.h"
#include "otwdrive.h"
#include "cpmanager.h"
#include "icp.h"
#include "cpmisc.h"
#include "tacan.h"
#include "vu.h"
#include "navsystem.h"
#include "simdrive.h"
#include "classtbl.h"
#include "cphsi.h"
#include "cpobject.h"

//MI for ICP stuff
extern bool g_bRealisticAvionics;
extern int mpHsi;

void ICPClass::ExecILSMode(void) 
{
	if(!g_bRealisticAvionics)
	{
		//MI Original code
		VU_ID							id;
		VU_ID							homeid;
		VU_ID							ilsid;
		BOOL							isValidILS;
		BOOL							isValidRWY;
		int							channel;
		char							rwyNum[4] = "";
		TacanList::StationSet	set;
		char							setchar;
		char							p_signaltype[10] = "";
		NavigationSystem::Type	type;
		static int					frame = 0;


		if(mUpdateFlags & ILS_UPDATE) 
		{
			mUpdateFlags &= ~ILS_UPDATE;

			if (gNavigationSys)
			{
				gNavigationSys->GetTacanVUID(NavigationSystem::ICP, &id);
			}
			else
			{
				id == FalconNullId;
			}

			if(id == FalconNullId) 
			{
				sprintf(mpLine1, "NO MISSION PRESETS");
				sprintf(mpLine2, "");
				sprintf(mpLine3, "");
			}
			else 
			{
				
				type = gNavigationSys->GetType();

				// Line #1
				sprintf(mpLine1, "");

				// Line #2
				if(type == NavigationSystem::AIRBASE) 
				{
					gNavigationSys->GetHomeID(&homeid);

					isValidILS = gNavigationSys->GetILSAttribute(NavigationSystem::AIRBASE_ID, &ilsid);
					isValidRWY = gNavigationSys->GetILSAttribute(NavigationSystem::RWY_NUM, (char*)rwyNum);
			
					if(id == homeid) 
					{
						sprintf(mpLine2, "HOME RUNWAY ");
					}
					else 
					{
						sprintf(mpLine2, "ALTERNATE RUNWAY ");
					}

					if(id == ilsid && isValidILS && isValidRWY) 
					{
						strcat(mpLine2, rwyNum);
					}
				}	
				else if(type == NavigationSystem::TANKER) 
				{
					sprintf(mpLine2, "TANKER");
				}
				else if(type == NavigationSystem::CARRIER) 
				{
					if(id == homeid) 
					{
						sprintf(mpLine2, "HOME CARRIER");
					}
					else 
					{
						sprintf(mpLine2, "ALTERNATE CARRIER");
					}
				}

				// Line #3
				gNavigationSys->GetTacanChannel(NavigationSystem::ICP, &channel, &set);

				if(set == TacanList::X) {
					setchar = 'X';
				}
				else {
					setchar = 'Y';
				}

				if(type == NavigationSystem::TANKER) {
					strcpy(p_signaltype, "AA-TR");			
				}		
				else {
					strcpy(p_signaltype, "TR");
				}

				sprintf(mpLine3, "TCN %-3d%c %s", channel, setchar, p_signaltype);
			}
		}
		else if (frame == 9) 
		{

			if (gNavigationSys)
			{
				gNavigationSys->GetTacanVUID(NavigationSystem::ICP, &id);
				type = gNavigationSys->GetType();
				if(type == NavigationSystem::AIRBASE) 
				{
					gNavigationSys->GetHomeID(&homeid);

					isValidILS = gNavigationSys->GetILSAttribute(NavigationSystem::AIRBASE_ID, &ilsid);
					isValidRWY = gNavigationSys->GetILSAttribute(NavigationSystem::RWY_NUM, (char*)rwyNum);
			
					if(id == homeid) {
						sprintf(mpLine2, "HOME RUNWAY ");
					}
					else {
						sprintf(mpLine2, "ALTERNATE RUNWAY ");
					}

					if(id == ilsid && isValidILS && isValidRWY) 
					{
						strcat(mpLine2, rwyNum);
					}
				}
			}
			else
			{
				id == FalconNullId;
			}

			frame = 0;
		}
		else 
		{

			frame++;
		}
	}
	else
	{
		ClearStrings();
		//MI modified for ICP stuff
		VU_ID							id;
		VU_ID							homeid;
		VU_ID							ilsid;
		char							rwyNum[4] = "";
		char							p_signaltype[10] = "";
		static int						frame = 0;

		HSICourse = static_cast<int>
			(OTWDriver.pCockpitManager->mpHsi->GetValue(CPHsi::HSI_VAL_DESIRED_CRS));

		if(gNavigationSys->GetControlSrc() == NavigationSystem::AUXCOMM)
			ILSBackup();
		else
		{
			if(gNavigationSys->GetTacanBand(NavigationSystem::ICP) == TacanList::X)
				TacanBand = 'X';
			else
				TacanBand = 'Y';
			if (gNavigationSys)
				gNavigationSys->GetTacanVUID(NavigationSystem::ICP, &id);
			else
				id == FalconNullId;

			if(id == FalconNullId) 
				ILSOn = 0;
			else 
			{
				//We SHOULD only have ILS when our Tacanband is == X
				if(TacanBand == 'X')
					ILSOn = 1;
				else 
					ILSOn = 0;
			}
			if(gNavigationSys->GetDomain(NavigationSystem::ICP) == TacanList::AA)
			{
				//Line1
				FillDEDMatrix(0,1,"TCN A/A TR");

				if(ILSOn ==1)
					FillDEDMatrix(0,18,"ILS ON");
				else
					FillDEDMatrix(0,18,"ILS OFF");
			}
			else
			{
				//Line1
				FillDEDMatrix(0,1,"TCN TR");
				if(ILSOn ==1)
					FillDEDMatrix(0,18,"ILS ON");
				else
					FillDEDMatrix(0,18,"ILS OFF");
			}			

			Digit1 = gNavigationSys->GetTacanChannel(NavigationSystem::ICP, 2);
			Digit2 = gNavigationSys->GetTacanChannel(NavigationSystem::ICP, 1);
			Digit3 = gNavigationSys->GetTacanChannel(NavigationSystem::ICP, 0);
			TacanChannel = (Digit1 * 100 + Digit2 * 10 + Digit3);
			
			//FakeILSFreq();
			sprintf(Freq, "%3.2f", gNavigationSys->GetCurTCNILS());
			//BEGIN LINE 3
			PossibleInputs = 3;
			if(ILSPageSel == 0)
				ScratchPad(2,8,12);
			else if(ILSPageSel == 1)
				ScratchPad(2,16,25);

			if(GetCMDSTR())
				FillDEDMatrix(2,17,"CMD STRG", 2);
			else
				FillDEDMatrix(2,17,"CMD STRG");
			//END LINE 3

			//BEGIN LINE 4
			FillDEDMatrix(3,1,"CHAN");
			ClearString();
			sprintf(tempstr,"%3.0d",TacanChannel);
			FillDEDMatrix(3,9,tempstr);
			FillDEDMatrix(3,14,"FREQ");
			FillDEDMatrix(3,19,Freq);
			//END LINE 4

			//BEGIN LINE 5
			FillDEDMatrix(4,1,"BAND");
			sprintf(tempstr,"%c(0)",TacanBand);
			FillDEDMatrix(4,9,tempstr);
			FillDEDMatrix(4,14,"CRS");
			sprintf(tempstr,"%d*",HSICourse);
			FillDEDMatrix(4,18,tempstr);
			//END LINE 5
		}
	}
}

void ICPClass::PNUpdateILSMode(int button, int)
{
	if(!g_bRealisticAvionics)
	{
		//MI original code
		if(button == PREV_BUTTON) 
		{
			gNavigationSys->StepPreviousTacan();
		}
		else 
		{
			gNavigationSys->StepNextTacan();
		}

		mUpdateFlags |= ILS_UPDATE;
	}
	else
		ExecILSMode();
}
void ICPClass::ENTRUpdateILSMode(){}

void ICPClass::FakeILSFreq(void)
{
	if(TacanChannel == 70)			//Kadena
		sprintf(Freq, "202.15");
	else if(TacanChannel == 73)		//Mandumi
		sprintf(Freq, "125.05");
	else if(TacanChannel == 74)		//Jeomcheon
		sprintf(Freq, "103.40");
	else if(TacanChannel == 75)		//Pusan
		sprintf(Freq, "100.35");
	else if(TacanChannel == 78)		//Haemi
		sprintf(Freq, "101.35");
	else if(TacanChannel == 79)		//Pyeongtaeg
		sprintf(Freq, "120.02");
	else if(TacanChannel == 80)		//Yecheon
		sprintf(Freq, "123.20");
	else if(TacanChannel == 81)		//Chungju
		sprintf(Freq, "114.65");
	else if(TacanChannel == 82)		//Gangneung
		sprintf(Freq, "109.25");
	else if(TacanChannel == 99)		//Taegu
		sprintf(Freq, "107.95");
	else if(TacanChannel == 100)	//Kwangju
		sprintf(Freq, "107.40");
	else if(TacanChannel == 101)	//Kunsan
		sprintf(Freq, "105.90");
	else if(TacanChannel == 102)	//Cheongju
		sprintf(Freq, "109.10");
	else if(TacanChannel == 105)	//Seoul
		sprintf(Freq, "108.65");
	else if(TacanChannel == 106)	//Kimpo Intl
		sprintf(Freq, "102.30");
	else if(TacanChannel == 108)	//Suweon
		sprintf(Freq, "115.35");
	else if(TacanChannel == 109)	//Osan
		sprintf(Freq, "101.45");
	else if(TacanChannel == 112)	//Kimhae Intl
		sprintf(Freq, "100.50");
	else if(TacanChannel == 113)	//Pohang
		sprintf(Freq, "111.10");
	else if(TacanChannel == 115)	//Samcheonpo
		sprintf(Freq, "108.25");
	else
		sprintf(Freq,"112.40");
}
