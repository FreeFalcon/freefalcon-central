#include "stdhdr.h"
#include "ground.h"
#include "gndunit.h"

void GroundClass::InitFromCampaignUnit(void)
{
	// Let's set up our initial destination and deltas using the actual AI logic..
	gai->Process();
}
