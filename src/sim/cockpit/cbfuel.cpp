#include <windows.h>
#include "cpcb.h"

#include "stdhdr.h"
#include "airframe.h"
#include "cpgauge.h"

#define FUEL_FLOW_DIGITS 5
#define MAX_FUEL_FLOW_VAL 99999

void CBFuelExec(void * pObject){
	CPGauge* pCPGauge;

	pCPGauge					= (CPGauge*) pObject;
	pCPGauge->mCurrentVal = pCPGauge->mpCPManager->mpOTWPlatform->af->fuelFlow; //VWF KLUDGE 2/12/97
}

void CBFuelDisplay(void * pObject){
	static BOOL		Init = FALSE;
	float				Digit[FUEL_FLOW_DIGITS] = {0.0F, 0.0F, 0.0F, 0.0F, 0.0F};
	CPGauge*			pCPGauge;
	char				pFuelStr[FUEL_FLOW_DIGITS + 3];
	int				x[FUEL_FLOW_DIGITS];
	int				y[FUEL_FLOW_DIGITS];
	int				i;
	int				NumDigits;
	int				Offset;


	pCPGauge	= (CPGauge*) pObject;

//	pCPGauge->mCurrentVal += 1.2F; //VWF KLUDGE 2/12/97 for testing

	x[0]	= pCPGauge->mx;								y[0]	= pCPGauge->my;
	x[1]	= x[0] + pCPGauge->mWidthTapeBitmap + 1;	y[1]	= y[0];
	x[2]	= x[1] + pCPGauge->mWidthTapeBitmap + 1;	y[2]	= y[0];
	x[3]	= x[2] + pCPGauge->mWidthTapeBitmap + 1;	y[3]	= y[0];
	x[4]	= x[3] + pCPGauge->mWidthTapeBitmap + 1;	y[4]	= y[0];

	if(pCPGauge->mCurrentVal > MAX_FUEL_FLOW_VAL){
		for(i = 0; i < FUEL_FLOW_DIGITS; i++){
			Digit[i] = 9.0F;
		}
	}
	else{
		sprintf(pFuelStr, "%5.1f", pCPGauge->mCurrentVal);//lbs/hr 

		NumDigits				= strlen(pFuelStr) - 2;
		Offset					= FUEL_FLOW_DIGITS - NumDigits;
		pFuelStr[NumDigits]	= pFuelStr[NumDigits + 1];

		for(i = 0; i < NumDigits; i++){
			Digit[i + Offset] = (pFuelStr[i] - 0x30) + ((pFuelStr[i + 1] - 0x30) / 10.0F);
		}
	}

	for(i = 0; i < FUEL_FLOW_DIGITS; i++){
		pCPGauge->DrawTape(Digit[i], x[i], y[i], TRUE);
	}
}
