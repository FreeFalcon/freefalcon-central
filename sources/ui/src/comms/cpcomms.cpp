/** @file cpcomms.cpp
* UI Comms Driver stuff (ALL of it which is NOT part of VU)
*/

//#include <windows.h>
//#include "resource.h"
//#include <tchar.h>
//#include "f4vu.h"
//#include "stdhdr.h"
//#include "MsgInc/SendDogfightInfo.h"
//#include "falcsess.h"
//#include "mesg.h"
//#include "falcmesg.h"
//#include "simdrive.h"
//#include "Find.h"
#include "vu2.h"
#include "uicomms.h"
#include "Flight.h"



FalconSessionEntity *UIComms::FindCampaignPlayer(VU_ID flightID, uchar planeid){
	Flight flight = (Flight) vuDatabase->Find(flightID);

	if(flight){
		VuSessionsIterator sessionWalker(FalconLocalGame);
		FalconSessionEntity *curSession;

		curSession = (FalconSessionEntity*)sessionWalker.GetFirst();
		while (curSession){
			if (
				curSession->GetPlayerFlightID() == flightID && 
				curSession->GetAircraftNum() == planeid && 
				curSession->GetPilotSlot() < 255
			){
				return (curSession);
			}
			curSession = (FalconSessionEntity*)sessionWalker.GetNext();
		}
		// Clear out any slot which we thought we had
		//if(planeid != 255)
		//flight->player_slots[planeid] = 255;
	}
	return(NULL);
}
