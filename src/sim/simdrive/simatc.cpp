#include "stdhdr.h"
#include "simdrive.h"
#include "atcbrain.h"
#include "falclist.h"
#include "objectiv.h"
#include "camplib.h"

void SimulationDriver::UpdateATC(void)
{
	
	ObjectiveClass* curObj;
	//VuListIterator findWalker (ATCBrain::atcList);
	VuListIterator findWalker (atcList);

	if (nextATCTime < SimLibElapsedTime){
		curObj = (ObjectiveClass*)findWalker.GetFirst();
		while (curObj){
			if (curObj->IsLocal() && curObj->brain){
				curObj->brain->Exec();
			}
			curObj = (ObjectiveClass*)findWalker.GetNext();
		}

		// RAS - 25Jan04 - Note in atcBrain.cpp said update should only run every 5 seconds, I found it 
		// was running every 1/4 seconds so I changed it.  Testing now to see if there are any problems
		nextATCTime = SimLibElapsedTime + CampaignSeconds * 5;  //(CampaignSeconds/4);
	} 
}