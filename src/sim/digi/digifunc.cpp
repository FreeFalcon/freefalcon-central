#include "stdhdr.h"
#include "digi.h"

ACFormationData *acFormationData;

void ReadDigitalBrainData (void)
{
   acFormationData = new ACFormationData;
   DigitalBrain::ReadManeuverData ();
}

void FreeDigitalBrainData (void)
{
   delete acFormationData;
   DigitalBrain::FreeManeuverData ();
}
