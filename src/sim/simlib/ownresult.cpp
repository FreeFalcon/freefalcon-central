#include "stdhdr.h"
#include "OwnResult.h"

OwnshipResultsClass OwnResults;

OwnshipResultsClass::OwnshipResultsClass(void)
{
   ClearData();
}

OwnshipResultsClass::~OwnshipResultsClass (void)
{
}

void OwnshipResultsClass::ClearData (void)
{
int i;

   for (i=0; i<NumWeaponTypes; i++)
      numWeaponsUsed[i] = 0;

   numEnemyAirKills = 0;
   numEnemyGroundKills = 0;
   numFriendlyAirKills = 0;
   numFriendlyGroundKills = 0;
   targetStatus = 0;
   escortStatus = 0;
   didFire = 0;
   endStatus = 0;
   tot = 0.0F;
   plannedTOT = 0.0F;
}