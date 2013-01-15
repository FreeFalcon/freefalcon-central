#include "stdhdr.h"
#include "Object.h"
#include "simmover.h"
#include "camp2sim.h"
#include "team.h"
#include "MsgInc\TrackMsg.h"
#include "RadarAGOnly.h"
#include "campbase.h"

void CalcRelGeom (SimBaseClass* ownObject, SimObjectType* targetList, TransformMatrix vmat, float elapsedTimeInverse);

RadarAGOnlyClass::RadarAGOnlyClass (int type, SimMoverClass* parentPlatform) : RadarDigiClass(type, parentPlatform)
{
	mode = GM;
	NewRange( 20.0f );
	platform->SetRdrAz( 0.0F);
	platform->SetRdrEl( 0.0F );
	platform->SetRdrCycleTime( 3.0F );
	platform->SetRdrAzCenter( 0.0f );
	platform->SetRdrElCenter( 0.0f );
}

//SimObjectType* RadarAGOnlyClass::Exec (SimObjectType* unusedTargetList)
SimObjectType* RadarAGOnlyClass::Exec (SimObjectType*)
{
	int				sendThisFrame;
	int				canSee;

	// Validate our locked target
	CheckLockedTarget();

	// If we don't have a locked target, we don't have anything to do.
	if (!lockedTarget) {
		return NULL;
	}

	// See if it is time to send a "painted" list update
	sendThisFrame = FALSE;

	// Time to see if we can still track our target
	canSee = TRUE;

	// Can't hold a lock while we're off
	if (!isEmitting)
   {
		canSee = FALSE;
	}

	// Can't hold a lock if its outside our radar cone
	if (fabs(lockedTarget->localData->ata) > radarData->ScanHalfAngle)
	{
		canSee = FALSE;
	}

	// Only track object in the correct domain (air/land)
	if ( !lockedTarget->BaseData()->OnGround() )
	{
		canSee = FALSE;
	}

	// Drop lock if the guy has been below the signal strength threshhold too long
	if (ReturnStrength(lockedTarget) < 1.0f)
   {
		// He's faded.  How long has he been hiding?
		if (SimLibElapsedTime - lockedTarget->localData->rdrLastHit > radarData->CoastTime)
      {
			// Give up and drop lock
			canSee = FALSE;
		}
	}


	// If we can't see the target, drop lock
	if (!canSee)
   {
		SetDesiredTarget( NULL );
	}


	// Tell the base class and the rest of the world where we're looking
	if (lockedTarget)
	{
		SetSeekerPos( TargetAz(platform, lockedTarget), TargetEl(platform, lockedTarget) );
		platform->SetRdrAz( radarData->BeamHalfAngle );
		platform->SetRdrEl( radarData->BeamHalfAngle );
		platform->SetRdrCycleTime( 0.0F );
		platform->SetRdrAzCenter( lockedTarget->localData->az );
		platform->SetRdrElCenter( lockedTarget->localData->el );

		// Tag the target as seen this frame
		lockedTarget->localData->rdrLastHit = SimLibElapsedTime;

		// Tell our current target he's locked
		if (sendThisFrame) {
			SendTrackMsg(lockedTarget, Track_Lock );
			lastTargetLockSend = SimLibElapsedTime;
		}
	}
	else
	{
		SetSeekerPos( 0.0f, 0.0f );
		platform->SetRdrAz( 0.0F);
		platform->SetRdrEl( 0.0F );
		platform->SetRdrCycleTime( 3.0F );
		platform->SetRdrAzCenter( 0.0f );
		platform->SetRdrElCenter( 0.0f );
	}

	return lockedTarget;
}

void RadarAGOnlyClass::SetDesiredTarget( SimObjectType* newTarget )
{
   // Only accept ground targets
   if (!newTarget || newTarget->BaseData()->OnGround())
      RadarClass::SetDesiredTarget( newTarget );
}