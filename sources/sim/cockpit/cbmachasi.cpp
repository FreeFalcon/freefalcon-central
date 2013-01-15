#include "stdafx.h"
#include <windows.h>

#include "stdhdr.h"
#include "flightData.h"

#include "cpcb.h"
#include "cpdigits.h"
#include "cpmachasi.h"
#include "cpdial.h"  //Wombat778 7-06-04
#include "aircrft.h"



//MI
extern bool g_bRealisticAvionics;
extern bool g_bMachAsiDial;			//Wombat778 7-9-04

void CBEMachAsi (void * pObject)
{
	if (!pObject) return;
	//Wombat778 7-09-04 treat machasi as a dial

	if (!g_bMachAsiDial)
	{

		CPMachAsi*		pCPMachAsi;
		pCPMachAsi = (CPMachAsi*) pObject;
		//MI
		if(g_bRealisticAvionics)	
		{
			//limited between 80 and 850 kts, according to -1
			float value = cockpitFlightData.kias / 100.0F;
			if(value < 80 / 100)
				value = 0.0F;
			else if(value > 850.0F / 100)
				value = 850.0F / 100;

			pCPMachAsi->mAirSpeed = value;
		}
		else
			pCPMachAsi->mAirSpeed		= cockpitFlightData.kias / 100.0F;
	}
	else
	{
		CPDial*		pCPDial;

		pCPDial = (CPDial*) pObject;
		if(!pCPDial || !pCPDial->mpOwnship) return;
		if(F4IsBadReadPtr (pCPDial->mpOwnship, sizeof(pCPDial->mpOwnship))) return;
		//MI
		if(g_bRealisticAvionics)			
		{
			//limited between 80 and 850 kts, according to -1
			float value = ((AircraftClass *)pCPDial->mpOwnship)->GetKias() / 100.0F;
			if(value < 80.0f / 100.0f)
				value = 0.0f;
			else if(value > 850.0F / 100.0f)
				value = 850.0F / 100.0f;

			pCPDial->mDialValue = value;
		}
		else
			pCPDial->mDialValue		= ((AircraftClass *)pCPDial->mpOwnship)->GetKias() / 100.0F;			
	}
}


void CBEMach(void * pObject) {

	CPDigits*		pCPDigits	= (CPDigits*) pObject;
	float				machNumber;
	int				firstDigit;
	int				secondDigit;

	machNumber = cockpitFlightData.mach;

	firstDigit = (int) machNumber;
	secondDigit = (int) (10.0F * (machNumber - ((float) firstDigit)));

	//MI
	if(g_bRealisticAvionics)	
	{
		//limited between 0.5 and 2.2, according to -1
		float value = float(firstDigit * 10 + secondDigit);
		if(value < 5.0F)
			value = 0.0F;
		//if(value > 22.0F)
		//	value = 22.0F;

		pCPDigits->SetDigitValues((int)value);
	}
	else
		pCPDigits->SetDigitValues(firstDigit * 10 + secondDigit);
}
