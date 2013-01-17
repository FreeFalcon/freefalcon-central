#include "stdafx.h"
#include "cpcb.h"
#include "cpindicator.h"
#include "cpmisc.h"
#include "stdhdr.h"
#include "airframe.h"
#include "aircrft.h"
#include "flightData.h"
#include "phyconst.h"
#include "guns.h"

#define FUEL_FLOW_DIGITS 5
#define MAX_FUEL_FLOW_VAL 99999

#define ALTITUDE_DIGITS 5
#define MAX_ALTITUDE_VALUE 99999

//MI
extern bool g_bIFlyMirage;

void CBEAOA(void * pObject){
	CPIndicator* pCPIndicator;

	pCPIndicator						= (CPIndicator*) pObject;
	if(!g_bIFlyMirage)
		pCPIndicator->mpTapeValues[0]	= cockpitFlightData.alpha;
	else
		pCPIndicator->mpTapeValues[0]	= -cockpitFlightData.alpha;
}

void CBDAOA(void *){

}

void CBEVVI(void * pObject){
	CPIndicator* pCPIndicator;

	pCPIndicator						= (CPIndicator*) pObject;
	pCPIndicator->mpTapeValues[0]	= -cockpitFlightData.zDot * 60.0F;
}

void CBDVVI(void *){

}

void CBESpeedBreaks(void * pObject) {

	CPIndicator* pCPIndicator;

	pCPIndicator						= (CPIndicator*) pObject;
	pCPIndicator->mpTapeValues[0]	= ((AircraftClass *)pCPIndicator->mpOwnship)->af->dbrake;
}

void CBEFuelFlow(void * pObject) {

	CPIndicator*	pCPIndicator;
	long				flowRate;
	char				digitString[FUEL_FLOW_DIGITS + 1];
	int				i, j;
	int            len;

	pCPIndicator	= (CPIndicator*) pObject;
	flowRate			= (long) ((AircraftClass *)pCPIndicator->mpOwnship)->af->FuelFlow();

	if(flowRate > MAX_FUEL_FLOW_VAL){
		for(i = 0; i < pCPIndicator->mNumTapes; i++) {
			pCPIndicator->mpTapeValues[i] = 9.0F;
		}
	}
	else{
		_ltoa(flowRate, digitString, 10);

		len = (int) strlen(digitString);	
		memset(pCPIndicator->mpTapeValues, 0, pCPIndicator->mNumTapes * sizeof(float));

		if(len >= pCPIndicator->mNumTapes) {
			for(i = 0, j = 2 * pCPIndicator->mNumTapes - len - 1; i <= len - pCPIndicator->mNumTapes; i++, j++) {
				pCPIndicator->mpTapeValues[j] = (float)(digitString[i] - 0x30);
			}
	
			pCPIndicator->mpTapeValues[2] += (float) (digitString[len - 2] - 0x30) * 0.1F;
		}
	}
}


void CBEMagneticCompass(void * pObject) {

	CPIndicator*	pCPIndicator;
	float				yaw;

	pCPIndicator	= (CPIndicator*) pObject;
	yaw				= cockpitFlightData.yaw;

	if(yaw < 0.0F) {
		yaw += 2 * PI;
	}

	pCPIndicator->mpTapeValues[0] = yaw * RTD;
}

void CBEAltInd(void * pObject) {

	CPIndicator*	pCPIndicator;
	long				altitude;
	char				digitString[10];
	int				i, j;
	int				len;

	pCPIndicator	= (CPIndicator*) pObject;
	altitude			= (long) -cockpitFlightData.z;

	if(altitude > MAX_ALTITUDE_VALUE){
		for(i = 0; i < pCPIndicator->mNumTapes; i++) {
			pCPIndicator->mpTapeValues[i] = 9.0F;
		}
	}
	else{
		_ltoa(altitude, digitString, 10);

		len = (int) strlen(digitString);	
		memset(pCPIndicator->mpTapeValues, 0, pCPIndicator->mNumTapes * sizeof(float));

		if(len >= pCPIndicator->mNumTapes) {
			for(i = 0, j = 2 * pCPIndicator->mNumTapes - len - 1; i <= len - pCPIndicator->mNumTapes; i++, j++) {
				pCPIndicator->mpTapeValues[j] = (float)(digitString[i] - 0x30);
			}
	
			pCPIndicator->mpTapeValues[2] += (float) (digitString[len - 2] - 0x30) * 0.1F;
		}
	}
}

void CBEFlapPos(void * pObject) {

	CPIndicator* pCPIndicator;

	pCPIndicator						= (CPIndicator*) pObject;
	pCPIndicator->mpTapeValues[0]	= ((AircraftClass *)pCPIndicator->mpOwnship)->af->tefPos;
}

void CBELEFPos(void * pObject) {

	CPIndicator* pCPIndicator;

	pCPIndicator						= (CPIndicator*) pObject;
	pCPIndicator->mpTapeValues[0]	= ((AircraftClass *)pCPIndicator->mpOwnship)->af->lefPos;
}

//TJL 01/04/04 RPM Tape
void CBERPMTape(void * pObject){
	CPIndicator* pCPIndicator;

	pCPIndicator						= (CPIndicator*) pObject;
	pCPIndicator->mpTapeValues[0]	= 100.0F * ((AircraftClass *)pCPIndicator->mpOwnship)->af->rpm;
}

//TJL 01/16/04 RPM Tape Engine 2
void CBERPM2Tape(void * pObject){
	CPIndicator* pCPIndicator;

	pCPIndicator						= (CPIndicator*) pObject;
	pCPIndicator->mpTapeValues[0]	= 100.0F * ((AircraftClass *)pCPIndicator->mpOwnship)->af->rpm2;
}

//TJL 01/04/04 WingSweep
void CBEWingSweepTape(void * pObject){
	CPIndicator* pCPIndicator;

	pCPIndicator						= (CPIndicator*) pObject;
	pCPIndicator->mpTapeValues[0]	= (((AircraftClass *)pCPIndicator->mpOwnship)->wingSweep * RTD);
}

//TJL 01/07/04
void CBETotalFuelTape(void * pObject){
	CPIndicator* pCPIndicator;
	pCPIndicator						= (CPIndicator*) pObject;

	float  fwd, aft, total;
	((AircraftClass *)pCPIndicator->mpOwnship)->af->GetFuel(&fwd, &aft, &total);
	
	pCPIndicator->mpTapeValues[0]	= total;
}

//TJL 04/25/04
void CBEFuelFlowLeftTape(void * pObject){
	CPIndicator* pCPIndicator;

	pCPIndicator						= (CPIndicator*) pObject;
	pCPIndicator->mpTapeValues[0]	= (((AircraftClass *)pCPIndicator->mpOwnship)->af->GetFuelFlowLeft());
}

//TJL 04/25/04
void CBEFuelFlowRightTape(void * pObject){
	CPIndicator* pCPIndicator;

	pCPIndicator						= (CPIndicator*) pObject;
	pCPIndicator->mpTapeValues[0]	= (((AircraftClass *)pCPIndicator->mpOwnship)->af->GetFuelFlowRight());
}
//TJL 09/11/04 //Cobra 10/30/04 TJL all below
void CBEFTITLeftTape(void * pObject){
	CPIndicator* pCPIndicator;

	pCPIndicator						= (CPIndicator*) pObject;
	pCPIndicator->mpTapeValues[0]	= (((AircraftClass *)pCPIndicator->mpOwnship)->af->GetFTITLeft());
}
//TJL 09/11/04
void CBEFTITRightTape(void * pObject){
	CPIndicator* pCPIndicator;

	pCPIndicator						= (CPIndicator*) pObject;
	pCPIndicator->mpTapeValues[0]	= (((AircraftClass *)pCPIndicator->mpOwnship)->af->GetFTITRight());
}

//TJL 09/16/04
void CBERoundsRemaining(void * pObject) {

	CPIndicator*	pCPIndicator;
	long			rounds;
	char			digitString[5];
	int				i, j;
	int             len;

	pCPIndicator	= (CPIndicator*) pObject;
	rounds			= (long) ((AircraftClass *)pCPIndicator->mpOwnship)->Guns->numRoundsRemaining;

	if(rounds > 9999){
		for(i = 0; i < pCPIndicator->mNumTapes; i++) {
			pCPIndicator->mpTapeValues[i] = 9.0F;
		}
	}
	else{
		_ltoa(rounds, digitString, 10);

		len = (int) strlen(digitString);	
		memset(pCPIndicator->mpTapeValues, 0, pCPIndicator->mNumTapes * sizeof(float));

		if(len >= pCPIndicator->mNumTapes) {
			for(i = 0, j = 2 * pCPIndicator->mNumTapes - len - 1; i <= len - pCPIndicator->mNumTapes; i++, j++) {
				pCPIndicator->mpTapeValues[j] = (float)(digitString[i] - 0x30);
			}
	
			pCPIndicator->mpTapeValues[2] += (float) (digitString[len - 2] - 0x30) * 0.1F;
		}
	}
}