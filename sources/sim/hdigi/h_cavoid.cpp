#include "stdhdr.h"
#include "classtbl.h"
#include "geometry.h"
#include "hdigi.h"
#include "simveh.h"
#include "object.h"
#include "Entity.h"

#define GS_LIMIT 9.0F

void HeliBrain::CollisionCheck(void)
{
	float relAz, relEl, range, reactTime;
	float hRange    = 200.0F; /* range to miss a hostile tgt / fireball */
	float hRangeSq = 40000.0F; /* square of hRange */
	//float reactFact = 0.75F; /* fudge factor for reaction time */
	float timeToImpact,rngSq,dt,pastRngSq;
	float ox,oy,oz,tx,ty,tz;
	int    collision;
	SimObjectType* obj;
	Falcon4EntityClassType* classPtr;
	SimObjectLocalData* localData;

	// Reaction time is a function of gs available and
	// agression level (gs allowed). 2 seconds is the bare
	// minimum for most situations. Modify with reaction factor
	//reactTime = ((GS_LIMIT / af->gsAvail) + (GS_LIMIT / maxGs)) * reactFact;

	// just hard coded in....
	reactTime = 2.0;

	collision = FALSE;

	// check objects
	obj = self->targetList;
	while (obj) {
		localData = obj->localData;

		// aircraft objects only
		classPtr = (Falcon4EntityClassType*)obj->BaseData()->EntityType();
		if (classPtr->vuClassData.classInfo_[VU_TYPE] == TYPE_AIRPLANE || 
      		classPtr->vuClassData.classInfo_[VU_TYPE] == TYPE_HELICOPTER  )
		{
			timeToImpact = (localData->range - hRange) / localData->rangedot;

			// not a problem
			if (timeToImpact < 0.0 || timeToImpact > reactTime) {
				obj = obj->next;
				continue;
			}

			// check for close approach. Linearly extrapolate out velocity
			// vector for react time
			pastRngSq = 10.0e7F;
			dt = 0.05F;

			while (dt < reactTime) {
				ox = self->XPos() + self->XDelta()*dt;
				oy = self->YPos() + self->YDelta()*dt;
				oz = self->ZPos() + self->ZDelta()*dt;

				tx = obj->BaseData()->XPos() + obj->BaseData()->XDelta()*dt;
				ty = obj->BaseData()->YPos() + obj->BaseData()->YDelta()*dt;
				tz = obj->BaseData()->ZPos() + obj->BaseData()->ZDelta()*dt;

				rngSq = (ox-tx)*(ox-tx) + (oy-ty)*(oy-ty) + (oz-tz)*(oz-tz);

				// collision possible if within hRange of target
				if (rngSq <= hRangeSq) {
					collision = TRUE;
					break;
				}
				
				// break out of loop if range begins to diverge
				if (rngSq > pastRngSq) {
					break;
				}
				pastRngSq = rngSq;

				dt += 0.1F;
			}

			// take action or bite the dust
			if (collision) {
            
				// Find a point in the maneuver plane
				// relEl = 45.0F * DTR;
				relEl = 0.0;
				if (localData->droll > 0.0) {
					relAz = -90.0F * DTR;
				}
				else {
					relAz = 90.0F * DTR;
				}

				range  = 4000.0F;

				GetXYZ (self, relAz, relEl, range, &trackX, &trackY, &trackZ);

				AddMode(CollisionAvoidMode);

				break;
			}
		}
		obj = obj->next;
	}

	// exit mode when collision no longer probable
	if (curMode == CollisionAvoidMode && !collision) {
	}
}

void HeliBrain::CollisionAvoid(void)
{
	float desHeading;
	float rollLoad;
	float rollDir;
	float desSpeed;

    desSpeed = 0.0f;
    rollDir = 0.0f;
    rollLoad = 0.0f;

	// OutputDebugString( "In Collision Avoid\n" );

	// Range to current track
	// rng = (trackX - self->XPos()) * (trackX - self->XPos()) + (trackY - self->YPos()) *	(trackY - self->YPos());

	// Heading error for current waypoint
	desHeading = (float)atan2 ( trackY - self->YPos(), trackX - self->XPos()) - self->Yaw();
	if (desHeading > 180.0F * DTR)
		desHeading -= 360.0F * DTR;
	else if (desHeading < -180.0F * DTR)
		desHeading += 360.0F * DTR;

	// rollLoad is normalized (0-1) factor of how far off-heading we are
	// to target
	rollLoad = desHeading / (180.0F * DTR);
	if (rollLoad < 0.0F)
		rollLoad = -rollLoad;

	if ( desHeading > 0.0f )
		rollDir = 1.0f;
	else
		rollDir = -1.0f;

	// 1/2 forward pitch
	desSpeed = 0.50;

	LevelTurn (rollLoad, rollDir, TRUE);
	// RV - Biker - No more AltitudeHold
	MachHold(desSpeed, self->GetWPalt(), TRUE);
	//MachHold(desSpeed, 300.0f, TRUE);
}
