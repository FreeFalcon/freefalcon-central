#include "stdafx.h"
#include "cpcb.h"
#include "cpdial.h"
#include "stdhdr.h"
#include "airframe.h"
#include "aircrft.h"
#include "cmpclass.h"
#include "vu.h"
#include "flightData.h"
#include "vdial.h"
#include "PilotInputs.h"	//MI
#include "phyconst.h" //TJL 01/05/04

extern bool g_bRealisticAvionics;

void CBEOilPressure(void * pObject){
	CPDial*	pCPDial;
	float		rpm;

	pCPDial						= (CPDial*) pObject;
	if(!pCPDial || !pCPDial->mpOwnship) return;
	rpm							= ((AircraftClass *)pCPDial->mpOwnship)->af->rpm;

	if(rpm < 0.7F) 
	{
		//rpm	= 40.0F;
		rpm = rpm * 40 / 0.7f;
	}
	else if(rpm <= 0.85) {
		rpm	= 40.0F + (100.0F - 40.0F) /  (0.85F - 0.7F) * (rpm - 0.7F);
	}
	else if(rpm <= 1.0) {
		rpm	= 100.0F;
	}
	else if(rpm <= 1.03) {
		rpm	= 100.0F + (103.0F - 100.0F) /  (1.03F - 1.0F) * (rpm - 1.00F);
	}
	else {
		rpm	= 103.0F;
	}

	pCPDial->mDialValue = pCPDial->mDialValue + (rpm - pCPDial->mDialValue) * 0.1F;
   cockpitFlightData.oilPressure = pCPDial->mDialValue;
}

//TJL 01/14/04 Multi-Engine
void CBEOilPressure2Dial(void * pObject){
	CPDial*	pCPDial;
	float		rpm2;

	pCPDial						= (CPDial*) pObject;
	if(!pCPDial || !pCPDial->mpOwnship) return;
	rpm2							= ((AircraftClass *)pCPDial->mpOwnship)->af->rpm2;

	if(rpm2 < 0.7F) 
	{
		//rpm	= 40.0F;
		rpm2 = rpm2 * 40 / 0.7f;
	}
	else if(rpm2 <= 0.85) {
		rpm2	= 40.0F + (100.0F - 40.0F) /  (0.85F - 0.7F) * (rpm2 - 0.7F);
	}
	else if(rpm2 <= 1.0) {
		rpm2	= 100.0F;
	}
	else if(rpm2 <= 1.03) {
		rpm2	= 100.0F + (103.0F - 100.0F) /  (1.03F - 1.0F) * (rpm2 - 1.00F);
	}
	else {
		rpm2	= 103.0F;
	}

	pCPDial->mDialValue = pCPDial->mDialValue + (rpm2 - pCPDial->mDialValue) * 0.1F;
   cockpitFlightData.oilPressure2 = pCPDial->mDialValue;
}



void CBEInletTemperature(void * pObject) {
	CPDial*	pCPDial;
	float rpm, retval;

	pCPDial						= (CPDial*) pObject;
	if(!pCPDial || !pCPDial->mpOwnship)
		return;


/* MODIFIED BY S.G. SO THE ENGINE HEAT IS USED INSTEAD OF THE RPM
	rpm = ((AircraftClass *)pCPDial->mpOwnship)->af->rpm;

	// FTIT values from Pete
	if (rpm < 0.7F)
	{
		retval = 5.1F;
	}
	else if (rpm < 0.9F)
	{
		retval = 5.1F + (rpm - 0.7F) / 0.2F * 1.0F;
	}
	else if (rpm < 1.0F)
	{
		retval = 6.1F + (rpm - 0.9F) / 0.1F * 1.5F;
	}
	else
	{
		retval = 7.6F + (rpm - 1.0F) / 0.03F * 0.1F;
	}
*/
	rpm = ((AircraftClass *)pCPDial->mpOwnship)->af->oldp01[0];

	// FTIT values from Sylvain :-)
	if (rpm < 0.2F)
	{
		retval = 5.1F * rpm/0.2f; // JPO adapt for < idle speeds.
	}
	else if (rpm < 0.6225F) // 0.9^4.5
	{
		retval = 5.1F + (rpm - 0.2F) / 0.4225F * 1.0F;
	}
	else if (rpm < 1.0F)
	{
		retval = 6.1F + (rpm - 0.6225F) / 0.3775F * 1.5F;
	}
	else
	{
		retval = 7.6F + (rpm - 1.0F) / 0.53F * 0.4F; // 0.53 is full afterburner
	}

	pCPDial->mDialValue	= retval;
}

//TJL 01/16/04 Multi-engine
void CBEInletTemperature2Dial(void * pObject) {
	CPDial*	pCPDial;
	float rpm, retval;

	pCPDial						= (CPDial*) pObject;
	if(!pCPDial || !pCPDial->mpOwnship)
		return;

	rpm = ((AircraftClass *)pCPDial->mpOwnship)->af->oldp01Eng2[0];

	// FTIT values from Sylvain :-)
	if (rpm < 0.2F)
	{
		retval = 5.1F * rpm/0.2f; // JPO adapt for < idle speeds.
	}
	else if (rpm < 0.6225F) // 0.9^4.5
	{
		retval = 5.1F + (rpm - 0.2F) / 0.4225F * 1.0F;
	}
	else if (rpm < 1.0F)
	{
		retval = 6.1F + (rpm - 0.6225F) / 0.3775F * 1.5F;
	}
	else
	{
		retval = 7.6F + (rpm - 1.0F) / 0.53F * 0.4F; // 0.53 is full afterburner
	}

	pCPDial->mDialValue	= retval;
}

void CBENozPos(void * pObject){
	CPDial*	pCPDial;
	float		rpmVal;

	pCPDial						= (CPDial*) pObject;
	if(!pCPDial || !pCPDial->mpOwnship) return;
	rpmVal                  = ((AircraftClass *)pCPDial->mpOwnship)->af->rpm;

	if(rpmVal <= 0.0F) {
		rpmVal	= 100.0F;
	}
	else if(rpmVal <= 0.83F) {
		rpmVal	= 100.0F + (0.0F - 100.0F) /  (0.83F) * rpmVal;
	}
	else if(rpmVal <= 0.99) {
		rpmVal	= 0.0F;
	}
	else if(rpmVal <= 1.03) {
		rpmVal	= (100.0F) /  (1.03F - 0.99F) * (rpmVal - 0.99F);
	}
	else {
		rpmVal	= 100.0F;
	}

	pCPDial->mDialValue = pCPDial->mDialValue + (rpmVal - pCPDial->mDialValue) * 0.1F;
   cockpitFlightData.nozzlePos = pCPDial->mDialValue;
}

//TJL 01/16/04 Multi-engine
void CBENozPos2Dial(void * pObject){
	CPDial*	pCPDial;
	float		rpmVal;

	pCPDial						= (CPDial*) pObject;
	if(!pCPDial || !pCPDial->mpOwnship) return;
	rpmVal                  = ((AircraftClass *)pCPDial->mpOwnship)->af->rpm2;

	if(rpmVal <= 0.0F) {
		rpmVal	= 100.0F;
	}
	else if(rpmVal <= 0.83F) {
		rpmVal	= 100.0F + (0.0F - 100.0F) /  (0.83F) * rpmVal;
	}
	else if(rpmVal <= 0.99) {
		rpmVal	= 0.0F;
	}
	else if(rpmVal <= 1.03) {
		rpmVal	= (100.0F) /  (1.03F - 0.99F) * (rpmVal - 0.99F);
	}
	else {
		rpmVal	= 100.0F;
	}

	pCPDial->mDialValue = pCPDial->mDialValue + (rpmVal - pCPDial->mDialValue) * 0.1F;
   cockpitFlightData.nozzlePos2 = pCPDial->mDialValue;
}

void CBERPM(void * pObject){
	CPDial* pCPDial;

	pCPDial						= (CPDial*) pObject;
	if(!pCPDial || !pCPDial->mpOwnship) return;
	pCPDial->mDialValue		= 100.0F * ((AircraftClass *)pCPDial->mpOwnship)->af->rpm;
}

//TJL 01/14/04 Multi-engine
void CBERPM2Dial(void * pObject){
	CPDial* pCPDial;

	pCPDial						= (CPDial*) pObject;
	if(!pCPDial || !pCPDial->mpOwnship) return;
	pCPDial->mDialValue		= 100.0F * ((AircraftClass *)pCPDial->mpOwnship)->af->rpm2;
}


void CBEAltDial(void * pObject){
	CPDial* pCPDial;
	
	float		altitude;
	pCPDial						= (CPDial*) pObject;

	if(!pCPDial || !pCPDial->mpOwnship) return;
	altitude						= -((AircraftClass *)pCPDial->mpOwnship)->ZPos();
	altitude						= (float)((int)altitude) - ((((int)altitude) / 1000) * 1000);
	pCPDial->mDialValue		= altitude;
}

void CBEInternalFuel(void * pObject) {

	CPDial* pCPDial;
	
	pCPDial						= (CPDial*) pObject;
	if(!pCPDial || !pCPDial->mpOwnship) return;
#if 0
	pCPDial->mDialValue		= 	((AircraftClass *)pCPDial->mpOwnship)->af->Fuel();
#else // JPO - the new order - fuel tanks and guage modelled.
	float fwd, aft, total;
	((AircraftClass *)pCPDial->mpOwnship)->af->GetFuel(&fwd, &aft, &total);
	pCPDial->mDialValue		= 	fwd;
#endif
}

void CBEExternalFuel(void * pObject) {

	CPDial* pCPDial;
	
	pCPDial						= (CPDial*) pObject;
	if(!pCPDial || !pCPDial->mpOwnship) return;
#if 0
	pCPDial->mDialValue		= ((AircraftClass *)pCPDial->mpOwnship)->af->ExternalFuel();
#else // JPO new fuel stuff
	float fwd, aft, total;
	((AircraftClass *)pCPDial->mpOwnship)->af->GetFuel(&fwd, &aft, &total);
	pCPDial->mDialValue		= 	aft;
#endif
}


void CBEEPUFuel(void * pObject) {
	CPDial*	pCPDial;
	
	pCPDial						= (CPDial*) pObject;
	if(!pCPDial || !pCPDial->mpOwnship) return;

	// me123/COdec fix - all cockpits use 0-40, so we adapt it to that range.
	pCPDial->mDialValue		= ((AircraftClass *)pCPDial->mpOwnship)->af->EPUFuel()*40.0f/100.0f;
}


void CBEClockHours(void * pObject) {
	CPDial*	pCPDial;
	VU_TIME	currentTime;
	VU_TIME	remainder;
	VU_TIME	hours;
	VU_TIME	minutes;
	VU_TIME	seconds;

	pCPDial						= (CPDial*) pObject;
	if(!pCPDial || !pCPDial->mpOwnship) return;

	// Get current time convert from ms to secs
	currentTime	= vuxGameTime / 1000;
	
	// Discect it
	remainder	= currentTime % 86400;		//86400 secs in a day
	hours			= remainder / 3600;			// 3600 secs in an hour
	remainder	= remainder - hours * 3600;

	if(hours > 12) {
		hours -= 12;
	}

	minutes		= remainder / 60;
	seconds		= remainder - minutes * 60;

	// Store in Miscellaneous
	// add back fraction of hour and fraction of minutes so that hour and min hand doesn't pop
	// 0.0166F = 1\60
	pCPDial->mpCPManager->mMiscStates.mHours = hours + minutes * 0.0166F;
	pCPDial->mpCPManager->mMiscStates.mMinutes = minutes + seconds * 0.0166F;
	pCPDial->mpCPManager->mMiscStates.mSeconds = (float)seconds;

	pCPDial->mDialValue	= (float)hours;
}

void CBEClockMinutes(void * pObject) {	
	CPDial* pCPDial;

	pCPDial						= (CPDial*) pObject;
	if(!pCPDial || !pCPDial->mpOwnship) return;

	pCPDial->mDialValue		= 	pCPDial->mpCPManager->mMiscStates.mMinutes;
}

void CBEClockSeconds(void * pObject) {	
	CPDial* pCPDial;

	pCPDial						= (CPDial*) pObject;
	if(!pCPDial || !pCPDial->mpOwnship) return;

	pCPDial->mDialValue		= 	pCPDial->mpCPManager->mMiscStates.mSeconds;
}



void CBEVOilPressure(void * pObject){
	VDial*	pCPDial;
	float		rpm;

	pCPDial						= (VDial*) pObject;
	if(!pCPDial || !pCPDial->mpOwnship) return;
	rpm							= ((AircraftClass *)pCPDial->mpOwnship)->af->rpm;

	if(rpm < 0.7F) {
		rpm	= 40.0F;
	}
	else if(rpm <= 0.85) {
		rpm	= 40.0F + (100.0F - 40.0F) /  (0.85F - 0.7F) * (rpm - 0.7F);
	}
	else if(rpm <= 1.0) {
		rpm	= 100.0F;
	}
	else if(rpm <= 1.03) {
		rpm	= 100.0F + (103.0F - 100.0F) /  (1.03F - 1.0F) * (rpm - 1.00F);
	}
	else {
		rpm	= 103.0F;
	}

	pCPDial->mDialValue = pCPDial->mDialValue + (rpm - pCPDial->mDialValue) * 0.1F;
   cockpitFlightData.oilPressure = pCPDial->mDialValue;
}

void CBEVInletTemperature(void * pObject) {
	VDial*	pCPDial;

	pCPDial						= (VDial*) pObject;
	if(!pCPDial || !pCPDial->mpOwnship) return;

	//Scale from 200 - 1200 to 2 - 12
	// 0.8 lag factor
	pCPDial->mDialValue		= 0.01F * (((AircraftClass *)pCPDial->mpOwnship)->af->rpm * 135.0F + 700.0F);
}

void CBEVNozPos(void * pObject){
	VDial*	pCPDial;
	float		rpmVal;

	pCPDial						= (VDial*) pObject;
	if(!pCPDial || !pCPDial->mpOwnship) return;
	rpmVal                  = ((AircraftClass *)pCPDial->mpOwnship)->af->rpm;

	if(rpmVal <= 0.0F) {
		rpmVal	= 100.0F;
	}
	else if(rpmVal <= 0.83F) {
		rpmVal	= 100.0F + (0.0F - 100.0F) /  (0.83F) * rpmVal;
	}
	else if(rpmVal <= 0.99) {
		rpmVal	= 0.0F;
	}
	else if(rpmVal <= 1.03) {
		rpmVal	= (100.0F) /  (1.03F - 0.99F) * (rpmVal - 0.99F);
	}
	else {
		rpmVal	= 100.0F;
	}

	pCPDial->mDialValue = pCPDial->mDialValue + (rpmVal - pCPDial->mDialValue) * 0.1F;
   cockpitFlightData.nozzlePos = pCPDial->mDialValue;
}




void CBEVRPM(void * pObject){
	VDial* pCPDial;

	pCPDial						= (VDial*) pObject;
	if(!pCPDial || !pCPDial->mpOwnship) return;
	pCPDial->mDialValue		= 100.0F * ((AircraftClass *)pCPDial->mpOwnship)->af->rpm;
}



void CBEVAltDial(void * pObject){
	VDial* pCPDial;
	
	float		altitude;
	pCPDial						= (VDial*) pObject;

	if(!pCPDial || !pCPDial->mpOwnship) return;
	altitude						= -((AircraftClass *)pCPDial->mpOwnship)->ZPos();
	altitude						= (float)((int)altitude) - ((((int)altitude) / 1000) * 1000);
	pCPDial->mDialValue		= altitude;
}
//MI
void CBETrimNose(void * pObject)
{
	CPDial* pCPDial;
	
	pCPDial						= (CPDial*) pObject;
	if(!pCPDial || !pCPDial->mpOwnship) 
		return;

	pCPDial->mDialValue	= UserStickInputs.ptrim;
}
void CBETrimWing(void * pObject)
{
	CPDial* pCPDial;
	
	pCPDial						= (CPDial*) pObject;
	if(!pCPDial || !pCPDial->mpOwnship) 
		return;

	pCPDial->mDialValue = -UserStickInputs.rtrim;
}

//TJL 01/04/04  Adding VVI dial
void CBEVVDial(void * pObject)
{
	CPDial* pCPDial;
	
	pCPDial						= (CPDial*) pObject;
	if(!pCPDial || !pCPDial->mpOwnship) 
		return;

	pCPDial->mDialValue = -cockpitFlightData.zDot * 60.0F;
}

//TJL 01/04/04  Adding G dial
void CBEGDial(void * pObject)
{
	CPDial* pCPDial;
	
	pCPDial						= (CPDial*) pObject;
	if(!pCPDial || !pCPDial->mpOwnship){
		return;
	}

	// sfr: why returning the opposite G??
	//pCPDial->mDialValue = -cockpitFlightData.gs;
	pCPDial->mDialValue = cockpitFlightData.gs;
}

//TJL 01/05/04  Adding Wing Sweep Dial (just in case older aircraft types use it instead of a tape)
void CBEWingSweepDial(void * pObject)
{
	CPDial* pCPDial;
	
	pCPDial						= (CPDial*) pObject;
	if(!pCPDial || !pCPDial->mpOwnship) 
		return;

	pCPDial->mDialValue = (((AircraftClass *)pCPDial->mpOwnship)->wingSweep * RTD);
}

//TJL 01/05/04  Adding AOA Dial (many aircraft have a backup or primary dial)
void CBEAOADial(void * pObject)
{
	CPDial* pCPDial;
	
	pCPDial						= (CPDial*) pObject;
	if(!pCPDial || !pCPDial->mpOwnship) 
		return;

	pCPDial->mDialValue = cockpitFlightData.alpha;
}

//TJL 01/05/04  Adding TEF Dial
void CBETEFDial(void * pObject)
{
	CPDial* pCPDial;
	
	pCPDial						= (CPDial*) pObject;
	if(!pCPDial || !pCPDial->mpOwnship) 
		return;

	pCPDial->mDialValue = ((AircraftClass *)pCPDial->mpOwnship)->af->tefPos;
}

//TJL 01/05/04  Adding LEF Dial
void CBELEFDial(void * pObject){
	CPDial* pCPDial;
	
	pCPDial	= (CPDial*) pObject;
	if (!pCPDial || !pCPDial->mpOwnship){
		return;
	}

	pCPDial->mDialValue = static_cast<AircraftClass*>(pCPDial->mpOwnship)->af->lefPos;
}

//TJL 01/07/04
void CBETotalFuelDial(void * pObject)
{
	CPDial* pCPDial;
	
	pCPDial						= (CPDial*) pObject;
	if(!pCPDial || !pCPDial->mpOwnship) 
		return;

	float  fwd, aft, total;
	((AircraftClass *)pCPDial->mpOwnship)->af->GetFuel(&fwd, &aft, &total);
	pCPDial->mDialValue = total;
}
//TJL 09/12/04 //Cobra 10/31/04 TJL
void CBEFTITLeftDial(void * pObject)
{
	CPDial* pCPDial;
	
	pCPDial						= (CPDial*) pObject;
	if(!pCPDial || !pCPDial->mpOwnship) 
		return;

	pCPDial->mDialValue = ((AircraftClass *)pCPDial->mpOwnship)->af->GetFTITLeft();
}

//TJL 09/12/04
void CBEFTITRightDial(void * pObject)
{
	CPDial* pCPDial;
	
	pCPDial						= (CPDial*) pObject;
	if(!pCPDial || !pCPDial->mpOwnship) 
		return;

	pCPDial->mDialValue = ((AircraftClass *)pCPDial->mpOwnship)->af->GetFTITRight();
}
