#include "stdhdr.h"
#include "simdrive.h"
#include "simbase.h"
#include "falclist.h"
#include "team.h"
#include "campBase.h"

SimBaseClass* SimulationDriver::FindFac (SimBaseClass* center)
{
   return FindNearest(center, facList);
}

SimBaseClass* SimulationDriver::FindATC (VU_ID desiredATC)
{
	SimBaseClass* curObj;
	SimBaseClass* retval = NULL;
	VuListIterator findWalker (SimDriver.atcList); // FRB

   if (desiredATC)
   {
      curObj = (SimBaseClass*)findWalker.GetFirst();
      while (curObj)
      {
         if (curObj->GetCampaignObject()->Id() == desiredATC)
         {
            retval = curObj;
            break;
         }
         curObj = (SimBaseClass*)findWalker.GetNext();
      }
   }

   return retval;
}

FlightClass* SimulationDriver::FindTanker (SimBaseClass* center)
{
	FlightClass* tankerFlight;
   
	tankerFlight = (FlightClass*) FindNearest(center, tankerList);

	if(tankerFlight) {
		return tankerFlight;
	}
	else {
		return NULL;
	}
}

//SimBaseClass* SimulationDriver::FindAirbase (SimBaseClass* center)
//{
//   return FindNearest(center, airbaseList);
//}

SimBaseClass* SimulationDriver::FindNearest (SimBaseClass* center, VuLinkedList* sourceList)
{
	SimBaseClass* curObj;
	SimBaseClass* retval = NULL;
	float tmpRng, rngSqr = FLT_MAX;
	VuListIterator findWalker (sourceList);
	float myX, myY, myZ;

   if (center)
   {
      curObj = (SimBaseClass*)findWalker.GetFirst();
      myX = center->XPos();
      myY = center->YPos();
      myZ = center->ZPos();
      while (curObj)
      {
         tmpRng = (curObj->XPos()-myX)*(curObj->XPos()-myX) +
                  (curObj->YPos()-myY)*(curObj->YPos()-myY) +
                  (curObj->ZPos()-myZ)*(curObj->ZPos()-myZ);

         if (tmpRng < rngSqr &&
            TeamInfo[curObj->GetTeam()]->TStance(center->GetTeam()) < Hostile)
         {
            rngSqr = tmpRng;
            retval = curObj;
         }
         curObj = (SimBaseClass*)findWalker.GetNext();
      }
   }

   return retval;
}