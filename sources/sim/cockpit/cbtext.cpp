#include "stdafx.h"
#include <windows.h>
#include "stdhdr.h"
#include "simdrive.h"
#include "aircrft.h"
#include "cpmanager.h"
#include "cpcb.h"
#include "cptext.h"
#include "hud.h"
#include "flightData.h"
#include "fack.h"

void CBExSpeedAlt(void* ptext)
{
	long	altitude;
	int	thousands;
	int	ones;
	int	airspeed;
	float hat, theAlt;
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	
	CPText *pCPText = (CPText*) ptext;

	hat = cockpitFlightData.z - OTWDriver.GetGroundLevel (SimDriver.GetPlayerEntity()->XPos(), SimDriver.GetPlayerEntity()->YPos());
	// Max hat if no rad alt
	if (playerAC->mFaults && playerAC->mFaults->GetFault(FaultClass::ralt_fault)){
		hat = -999999.9F;
	}
	
	// Choose the right scale	
	if (TheHud->GetRadarSwitch() == HudClass::BARO){
		theAlt = -cockpitFlightData.z;
	}
	else if (TheHud->GetRadarSwitch() == HudClass::ALT_RADAR){
		theAlt = -hat;
	}
	else {
		if (hat > -1200.0F || (cockpitFlightData.zDot < 0.0F && hat > -1500.0F)){
			theAlt = -hat;
		}
		else {
			theAlt = -cockpitFlightData.z;
		}
	}

	altitude		= FloatToInt32(theAlt);
	airspeed		= FloatToInt32(SimDriver.GetPlayerEntity()->GetKias());

	thousands = altitude / 1000;
	ones		 = altitude % 1000;

	if(thousands) {
		sprintf(pCPText->mpString[0], "ALT: %d,%03d ft", thousands, ones);
	}
	else {
		sprintf(pCPText->mpString[0], "ALT: %03d ft", ones);
	}
	sprintf(pCPText->mpString[1], "AIRSPD: %3d kts", airspeed);
}

