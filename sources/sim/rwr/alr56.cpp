#include "stdhdr.h"
#include "object.h"
#include "simmover.h"
#include "ClassTbl.h"
#include "simdrive.h"
#include "fsound.h"
#include "mfd.h"
#include "msginc\TrackMsg.h"
#include "Graphics\Include\Display.h"
#include "airunit.h"
#include "aircrft.h"
#include "alr56.h"
#include "radarData.h"

ALR56Class::ALR56Class (int idx, SimMoverClass* self) : PlayerRwrClass (idx, self)
{
	priorityMode	= FALSE;
}

ALR56Class::~ALR56Class (void)
{
}


float ALR56Class::GetLethality (FalconEntity* theObject)
{
	int		alt			= (lowAltPriority) ? LOW_ALT_LETHALITY : HIGH_ALT_LETHALITY;

	if (F4IsBadReadPtr(theObject, sizeof(FalconEntity))) // JB 010404 CTD
		return 0;

	float	lethality	= RadarDataTable[theObject->GetRadarType()].Lethality[alt];

	// Scale lethality for normalized range
	float dx = theObject->XPos()-platform->XPos();
	float dy = theObject->YPos()-platform->YPos();
	float nomRange	= RadarDataTable[theObject->GetRadarType()].NominalRange;
	float range  = (float)sqrt(dx*dx + dy*dy) / (2.0f * nomRange);
	// lethality = max (lethality, 0.1F); - Place into code if you want search radars to be shown from emiterList
	if (range < 0.8f) {
		lethality *= 1 - range;
	} else {
		lethality *= 0.2f;
	}

	return lethality;
}


void ALR56Class::PushButton(int whichButton, int whichMFD)
{
   switch (whichButton)
   {
      case 0:
         MfdDisplay[whichMFD]->SetNewMode(MFDClass::MfdMenu);
      break;

      case 14:
         MFDSwapDisplays();
      break;
   }
}

