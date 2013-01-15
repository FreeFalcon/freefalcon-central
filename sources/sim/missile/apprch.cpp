#include "stdhdr.h"
#include "otwdrive.h"
#include "object.h"
#include "missile.h"
#include "classtbl.h"

float FindMinDistance (vector* a, vector* c, vector* b, vector* d);

void MissileClass::ClosestApproach(void)
{
vector a, b, c, d;
float newMin;

	// Don't do proximity checking for surface to surface missiles, let them hit the ground
	if(GetSType() == STYPE_MISSILE_SURF_SURF)
		return;

   if (targetPtr && range <= auxData->proximityfuserange && range >= 5.0F)
   {
      flags |= EndGame;
      a.x = x;
      a.y = y;
      a.z = z;
      b.x = xdot * SimLibMajorFrameTime;
      b.y = ydot * SimLibMajorFrameTime;
      b.z = zdot * SimLibMajorFrameTime;
      c.x = targetPtr->BaseData()->XPos();
      c.y = targetPtr->BaseData()->YPos();
      c.z = targetPtr->BaseData()->ZPos();
      d.x = (targetPtr->BaseData()->XDelta() + 0.5F) * SimLibMajorFrameTime;
      d.y = targetPtr->BaseData()->YDelta() * SimLibMajorFrameTime;
      d.z = targetPtr->BaseData()->ZDelta() * SimLibMajorFrameTime;

	  if (targetPtr->BaseData()->IsCampaign()) {
		  // To save time, we're using the appoximate ground level at the target's location.
		  // This will potentially introduce some error in the ultimate impact point, but should be acceptable.
		  // We have to make this corection since all campaign entities store
		  // their z position as "AGL" instead of world space height above sea level.
		  c.z += OTWDriver.GetApproxGroundLevel( targetPtr->BaseData()->XPos(), targetPtr->BaseData()->YPos() );

		  // Could probably also just use "groundZ" since we're close to the target at this point...
	  }
	  
      newMin = FindMinDistance(&a, &b, &c, &d);
      if (newMin > 1.2F * ricept)
         flags |= ClosestApprch;
      ricept = min (newMin, ricept);
   }
   else if ( auxData && range <= auxData->proximityfuserange ) 
   { static int count = 0;
	  if (rand()%100 < auxData->ProximityfuseChange)
	  {
		  flags |= ClosestApprch;
	  }
	  count++;
   }
   else
   {
      ricept = min (range, ricept);
   }   
}
