#include "stdhdr.h"
#include "entity.h"
#include "Object.h"
#include "simbase.h"
#include "camp2sim.h"
#include "team.h"
#include "simmath.h"
#include "simmover.h"
#include "MsgInc\TrackMsg.h"
#include "Missile.h"
#include "RadarMissile.h"


RadarMissileClass::RadarMissileClass( int type, SimMoverClass* parentPlatform ) : RadarClass(type, parentPlatform)
{
	lastChaffID	= FalconNullId;
	couldGuide	= TRUE;
}


SimObjectType* RadarMissileClass::Exec (SimObjectType* targetList)
{
	SimObjectType	*newLock = NULL;
	BOOL			canGuide = FALSE;


	// Quit now if we're turned off
	if (!isEmitting)  {
		return NULL;
	}

	// Validate our locked target
	CheckLockedTarget();

	// Decide if we can still see our locked target
	if (lockedTarget) {

		// Only track air objects
		if ( !lockedTarget->BaseData()->OnGround() )
		{
			// Don't track when the signal strength is too low
			if (ReturnStrength(lockedTarget) > 0.5f)
			{
				if (couldGuide)
				{
					// Hold lock inside our gimbal limit cone
					if (fabs(lockedTarget->localData->ata) <= radarData->ScanHalfAngle)
					{
						canGuide = TRUE;
					}
				}
				else 
				{
					// Reacquire lock only inside our seek FOV cone
// 2000-08-31 MODIFIED BY S.G. SINCE THE RADAR IS STILL POINTING WHERE WHEN WE STILL HAD A LOCK, WE NEED TO OFFSET OUR BeamHalfAngle ACCORDINGLY!
// WARNING: In the 1.08i2 patch, I forgot to add '* 2.0F' after 'radarData->BeamHalfAngle'. I'll do it in the source since it SHOULD be this way anyhow (see the other line similar below)
//					if (fabs(lockedTarget->localData->ata) <= radarData->BeamHalfAngle)
					if (fabs(lockedTarget->localData->ata - (float)acos(cos(platform->RdrAzCenter()) * cos(platform->RdrElCenter()))) <= radarData->BeamHalfAngle * 2.0F)
					{
						canGuide = TRUE;
					}
				}
			}

		   // See if we should bite counter measures dispensed by our target
         if (canGuide)
		      newLock = ConsiderDecoy( lockedTarget, canGuide );
		}
	}
   else
   {
      // Check the passed list to see if we find anything
      newLock = targetList;
      while (newLock)
      {
		   // Only track air objects
		   if ( !newLock->BaseData()->OnGround() )
		   {
			   // Don't track when the signal strength is too low
			   if (ReturnStrength(newLock) > 1.0f)
			   {
				   if (couldGuide)
				   {
					   // Hold lock inside our gimbal limit cone
					   if (fabs(newLock->localData->ata) <= radarData->ScanHalfAngle)
					   {
						   canGuide = TRUE;
                     break;
					   }
				   }
				   else 
				   {
					   // Reacquire lock only inside our seek FOV cone
// 2000-08-31 MODIFIED BY S.G. SINCE THE RADAR IS STILL POINTING WHERE WHEN WE STILL HAD A LOCK, WE NEED TO OFFSET OUR BeamHalfAngle ACCORDINGLY!
//					   if (fabs(newLock->localData->ata) <= radarData->BeamHalfAngle * 2.0F)
					   if (fabs(newLock->localData->ata - (float)acos(cos(platform->RdrAzCenter()) * cos(platform->RdrElCenter()))) <= radarData->BeamHalfAngle * 2.0F)
					   {
						   canGuide = TRUE;
                     break;
					   }
				   }
			   }
		   }
         newLock = newLock->next;
      }
   }


	// If we changed locks, update our pointers, otherwise see if its time for another "paint" message
	if (newLock != lockedTarget)
	{
		SetDesiredTarget( newLock );
	}
	else if (lockedTarget && canGuide && (SimLibElapsedTime - lastTargetLockSend > TrackUpdateTime))
	{
		// Tell our current target he's locked (if he's not a countermeasure)
		if (!lockedTarget->BaseData()->IsWeapon()) {
			SendTrackMsg( lockedTarget, Track_Launch );

			lastTargetLockSend = SimLibElapsedTime;
		}
	}
	

	// If we can guide, update our data and return the target entity
	if (canGuide)
	{
		couldGuide = TRUE;

		// Tell the base class where we're looking
		SetSeekerPos( TargetAz(platform, lockedTarget), TargetEl(platform, lockedTarget) );
		platform->SetRdrAz( radarData->BeamHalfAngle );
		platform->SetRdrEl( radarData->BeamHalfAngle );
		platform->SetRdrCycleTime( 0.0F );
		platform->SetRdrAzCenter( lockedTarget->localData->az );
		platform->SetRdrElCenter( lockedTarget->localData->el );

		// Tag the target as seen this frame
//	S.G. NEED MORE THAN SIMPLE DETECTION HERE
//		lockedTarget->localData->sensorState[Radar] = Detection;
		lockedTarget->localData->sensorState[Radar] = SensorTrack;
		lockedTarget->localData->rdrLastHit = SimLibElapsedTime;

		// Return the target entity for the missile to guide upon
		return lockedTarget;
	}

   // Leave the seeker where it is, and continue looking
   /*
   else {
		SetSeekerPos( 0.0f, 0.0f );
		platform->SetRdrAz( radarData->ScanHalfAngle );
		platform->SetRdrEl( radarData->ScanHalfAngle );
		platform->SetRdrCycleTime( 3.0F );
		platform->SetRdrAzCenter( 0.0f );
		platform->SetRdrElCenter( 0.0f );
   }
   */

	// If we were guiding, but now we're not, have the missile "coast"
	if (couldGuide) {
		couldGuide = FALSE;

		if (lockedTarget) {
			ShiAssert( platform );
			ShiAssert( platform->IsMissile() );

			// Tell our parent missile where the target was when last seen
			((MissileClass*)platform)->SetTargetPosition(
				lockedTarget->BaseData()->XPos(), 
				lockedTarget->BaseData()->YPos(), 
				lockedTarget->BaseData()->ZPos()
			);
			((MissileClass*)platform)->SetTargetVelocity(
				lockedTarget->BaseData()->XDelta(), 
				lockedTarget->BaseData()->YDelta(), 
				lockedTarget->BaseData()->ZDelta() 
			);
		}
	}

	return NULL;	// Indicate that we can't see our target
}


// This controls how effective countermeasures are as a function of seeker range from target
static const float	cmRangeArray[]		= {0.0F,  12000.0f,  24000.0f,  48000.0f,  120000.0f};
static const float	cmBiteChanceArray[]	= {0.0F,      0.0F,     0.75F,     0.75F,       0.0F};
static const int	cmArrayLength		= sizeof(cmRangeArray) / sizeof(cmRangeArray[0]);


SimObjectType* RadarMissileClass::ConsiderDecoy( SimObjectType *target, BOOL canGuide )
{
	VU_ID			id;
	FalconEntity	*cm;
	float			chance;
	int				dummy = 0;

	// No counter measures deployed by campaign things
	if (!target || !target->BaseData()->IsSim()) {
		return target;
	}

	// Get the ID of the most recently launched counter measure from our target
	id = ((SimBaseClass*)target->BaseData())->NewestChaffID();
	
	// If we have a new chaff bundle to deal with
	if (id != lastChaffID)
	{
		// Stop here if there isn't a counter measure in play
		if (id == FalconNullId) {
			lastChaffID = id;
			return target;
		}

		// Try to find the counter measure entity in the database
		cm = (FalconEntity*)vuDatabase->Find( id );

//		MonoPrint ("ConsiderDecoy %08x %f: ", cm, target->localData->range);

		if (!cm) {
			// We'll have to wait until next time
			// (probably because the create event hasn't been processed locally yet)
			return target;
		}

		// Start with the suceptability of this seeker to counter measures
		chance = radarData->ChaffChance;

		// Adjust with a range to target based chance of an individual countermeasure working
		chance *= Math.OnedInterp(target->localData->range, cmRangeArray, cmBiteChanceArray, cmArrayLength, &dummy);

		float Vr =(float)cos(target->localData->ataFrom) * target->BaseData()->GetVt();
		if (fabs(Vr) > radarData->NotchSpeed)  
			chance = min (0.95f, chance * radarData->NotchPenalty);
// 2000-11-17 REMOVED BY S.G. WHY SHOULD IT?
		// Player countermeasures work better
//		if (target->BaseData()->IsPlayer()) {
//			chance *= 1.15F;
//		}

		// If we've beaten the missile guidance, countermeasures work two times better
		if (!canGuide) {
			chance *= 2.0f;
		}

		// Roll the dice
		if (chance > (float)rand()/RAND_MAX)
		{
			// Compute some relative geometry stuff
			const float atx	= platform->dmx[0][0];
			const float aty	= platform->dmx[0][1];
			const float atz	= platform->dmx[0][2];
			const float dx	= cm->XPos() - platform->XPos();
			const float dy	= cm->YPos() - platform->YPos();
			const float dz	= cm->ZPos() - platform->ZPos();
			const float range	= (float)sqrt( dx*dx + dy*dy );
			const float cosATA	= (atx*dx + aty*dy + atz*dz) / (float)sqrt(range*range+dz*dz);
			
			// Only take the bait if we can see the thing
			// TODO:  Should probably use beam width instead of scan angle...
			if (cosATA >= cos(radarData->ScanHalfAngle))
			{
//#ifdef DEBUG
//				target = new SimObjectType(OBJ_TAG, platform, cm);
//#else
//				
//#endif
				target = new SimObjectType(cm);
			}
			MonoPrint ("Yes %f %08x\n", chance, target);
		}
		else
		{
//			MonoPrint ("No %f %08x\n", chance, target);
		}

		// Note that we've considered this countermeasure
		lastChaffID = id;
	}

	return target;
}
