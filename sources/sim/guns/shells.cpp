/*
** Name: SHELLS.CPP
** Description:
**		Functions for handling shell-type of Guns
** History:
**	19-feb-98 (edg)  We go marching in.....
*/
#include "stdhdr.h"
#include "falcmesg.h"
#include "object.h"
#include "guns.h"
#include "object.h"
#include "simdrive.h"
#include "simmover.h"
#include "MsgInc\DamageMsg.h"
#include "MsgInc\MissileEndMsg.h"
#include "campbase.h"
#include "otwdrive.h"
#include "sfx.h"
#include "fsound.h"
#include "soundfx.h"
#include "fakerand.h"
#include "Graphics\Include\drawsgmt.h"
#include "Graphics\Include\RViewPnt.h"
#include "feature.h"
#include "acmi\src\include\acmirec.h"
#include "playerop.h"
#include "classtbl.h"
#include "entity.h"
#include "camplib.h"
#include "camp2sim.h"
#include "campbase.h"

#include "Ground.h" // 2002-03-12 S.G.

extern float g_fBiasFactorForFlaks; // 2002-03-12 S.G.
extern bool g_bUseSkillForFlaks;

/*
** Name: IsShell
**		Returns TRUE if the guns is shell type
*/
BOOL
GunClass::IsShell( void )
{
	if ( typeOfGun == GUN_SHELL )
		return TRUE;

	return FALSE;
}

/*
** Name: IsTracer
**		Returns TRUE if the guns is tracer type
*/
BOOL
GunClass::IsTracer( void )
{
	if ( typeOfGun == GUN_TRACER || typeOfGun == GUN_TRACER_BALL )
		return TRUE;

	return FALSE;
}

/*
** Name: ReadyToFire
**		Returns TRUE if the guns is ready to fire
*/
BOOL
GunClass::ReadyToFire( void )
{
	// tracers are always ready
	if ( typeOfGun == GUN_TRACER || typeOfGun == GUN_TRACER_BALL )
		return TRUE;

	// any rounds left?
	if ( numRoundsRemaining == 0 )
		return FALSE;

	// if we've got a target a shell is in the air
	if ( shellTargetPtr != NULL )
		return FALSE;

	// ok to fire
	return TRUE;
}

/*
** Name: GetDamageAssesment
**		Returns a value of estimated damage of this type of gun
**		against the target
*/
// VP_changes Nov 7, 2002
// This functiin should be modified
float
GunClass::GetDamageAssessment( SimBaseClass *target, float range )
{
	float zpos, zdelta;

	if ( range < minShellRange || range > maxShellRange )
		return 0.0f;

	// just random stuff for now....
	if ( gunDomain == wdBoth )
	{
		if ( typeOfGun == GUN_TRACER || typeOfGun == GUN_TRACER_BALL )
		{
			if ( !target->OnGround() )
			{
				// disallow tracers above 6000ft
				zpos = target->ZPos();
				// altitude diff with firing platform
				zdelta = zpos - parent->ZPos();
				if ( zdelta < -6000.0f )
					return 0.0f;
			}
			return 0.1f + PRANDFloatPos();
		}
		else
		{
			if ( !target->OnGround() )
			{
				// disallow shells below 1000ft?
				// target z in 1.5 secs
				zpos = target->ZPos() + target->ZDelta() * 1.5f;
				// altitude diff with firing platform
				zdelta = zpos - parent->ZPos();
				if ( zdelta > -6000.0f )
					return 0.0f;
			}

			return 0.3f + PRANDFloatPos();
		}
	}

	if ( gunDomain == wdGround && target->OnGround() )
	{
		if ( typeOfGun == GUN_TRACER || typeOfGun == GUN_TRACER_BALL )
			return 0.1f + PRANDFloatPos();
		else
			return 0.3f + PRANDFloatPos();
	}

	if ( gunDomain == wdAir && !target->OnGround() )
	{
		if ( typeOfGun == GUN_TRACER || typeOfGun == GUN_TRACER_BALL )
		{
			// disallow tracers above 6000ft
			// target z in 1.5 secs
			zpos = target->ZPos();
			// altitude diff with firing platform
			zdelta = zpos - parent->ZPos();
			if ( zdelta < -6000.0f )
				return 0.0f;

			return 0.1f + PRANDFloatPos();
		}
		else // shell
		{
			// disallow shells below 1000ft?
			// target z in 1.5 secs
			zpos = target->ZPos() + target->ZDelta() * 1.5f;
			// altitude diff with firing platform
			zdelta = zpos - parent->ZPos();
			if ( zdelta > -6000.0f )
				return 0.0f;

			return 0.3f + PRANDFloatPos();
		}
	}

	return 0.0f;
}

/*
** Name: FireTarget
**		Reference the target and calc the time to detonate.
**		If target is NULL, deref any existing target
*/
void
GunClass::FireShell( SimObjectType *newTarget )
{
	if (newTarget == shellTargetPtr)
		return;

	// cannot set a new target if shell is flying
	if (shellTargetPtr && newTarget)
		return;

	// are we nulling out our target
	if ( shellTargetPtr )
	{
		// release ref
		shellTargetPtr->Release(  );
		shellTargetPtr = NULL;
		return;
	}

	// OK we're firing -- newTarget must be non NULL

	newTarget->Reference(  );
	shellTargetPtr = newTarget;
	// hack the time in now. 
	shellDetonateTime = SimLibElapsedTime + 1500;	//MI this is how long from shot till explosion
}

/*
** Name: UpdateShell
**		Determines when its time to detonate.  Rolls dice.  And
**		Sends messages as needed for effects and damage.
*/
void
GunClass::UpdateShell( void )
{
	float rangeSquare;
	FalconMissileEndMessage* endMessage;
	SimBaseClass *t;
	BOOL hitSomething = FALSE;
	float blastRange;

	if ( SimLibElapsedTime < shellDetonateTime )
		return;

	// ok, now we decide the extent of damage and send messages

	// get the target base class
	t = (SimBaseClass *)shellTargetPtr->BaseData();

	blastRange = 0.0f;
	// damage stuff goes here....
	if ( wcPtr->BlastRadius == 0 )
	{
		// must be a direct hit!  1-8 fer now
		// ground targets get much better chance of hit
		if ( t->OnGround() )
		{
			if ( (rand() & 0x7 ) == 0x7 )
			{
				hitSomething = TRUE;
				SendDamageMessage(t,
								  0.0f,
								  FalconDamageType::BulletDamage);
			}
		}
// 2000-08-30 MODIFIED BY S.G. TO ACCOMODATE FOR ALTITUDE AND SPEED.
// 2000-09-06 CHANGED AGAIN TO A NEW EQUATION (sqrt(speed / 100) * sqrt(range / 1000))
//		else if ( (rand() & 0x1F ) == 0x1F )
		// Marco Edit - tried to return back to 1.08i2 + RP4 values
		// else if (40.0f * (float)rand() / 32767.0f * (float)sqrt(shellTargetPtr->localData->range * ((SimMoverClass *)shellTargetPtr->BaseData())->GetKias() / 100000.0f) < 0.040625f)
		else if (31.0f * (float)rand() / 32767.0f * (float)sqrt(shellTargetPtr->localData->range * ((SimMoverClass *)shellTargetPtr->BaseData())->GetKias() / g_fBiasFactorForFlaks) < 0.0325f)
		{
			hitSomething = TRUE;
			SendDamageMessage(t,
							  0.0f,
							  FalconDamageType::BulletDamage);
		}
		else
		{
			blastRange = (float)wcPtr->BlastRadius;
		}
	}
	else
	{
		// proximity damage
		blastRange = rangeSquare = (float)wcPtr->BlastRadius;

		// ground targets get much better chance of hit
		if ( t->OnGround() )
			rangeSquare *= 1.5f * PRANDFloatPos();
		else {
// 2000-08-30 MODIFIED BY S.G. TO ACCOMODATE FOR ALTITUDE AND SPEED. 1.08i2 ALSO USES rand INSTEAD OF PRANDFloatPos
// 2000-09-06 CHANGED AGAIN TO A NEW EQUATION (sqrt(speed / 100) * sqrt(range / 1000))
//			rangeSquare *= 40.0f * PRANDFloatPos();
			rangeSquare *= 40.0f * rand() / 32767.0f * (float)sqrt(shellTargetPtr->localData->range * ((SimMoverClass *)shellTargetPtr->BaseData())->GetKias() / g_fBiasFactorForFlaks);

			// 2002-03-12 ADDED BY S.G. Use the ground troop skill if requested
			if (g_bUseSkillForFlaks && parent && parent->IsGroundVehicle()){
				GroundClass *gc = static_cast<GroundClass*>(parent.get());
				rangeSquare *= static_cast<float>(
					7.0f / (4 + gc->gai->skillLevel * gc->gai->skillLevel)
				);
			}
		}

		if ( rangeSquare < blastRange )
		{
			hitSomething = TRUE;
			rangeSquare *= rangeSquare;
			SendDamageMessage(t,
						  rangeSquare,
						  FalconDamageType::ProximityDamage);
		}
	}


	// missile end message
	endMessage = new FalconMissileEndMessage (Id(), FalconLocalGame);
	endMessage->dataBlock.fEntityID  = parent->Id();
	endMessage->dataBlock.fPilotID   = ((SimMoverClass*)parent.get())->pilotSlot;
	endMessage->dataBlock.fIndex     = parent->Type();
	endMessage->dataBlock.fCampID    = parent->GetCampID();
	endMessage->dataBlock.fSide      = parent->GetCountry();

	endMessage->dataBlock.dEntityID  = t->Id();
	endMessage->dataBlock.dCampID    = t->GetCampID();
	endMessage->dataBlock.dSide      = t->GetCountry();
	if ( t->IsSim() )
	{
	  endMessage->dataBlock.dPilotID   = ((SimMoverClass*)t)->pilotSlot;
	  endMessage->dataBlock.dCampSlot  = (char)((SimMoverClass*)t)->GetSlot();
	}
	else
	{
	  endMessage->dataBlock.dPilotID   = 255;
	  endMessage->dataBlock.dCampSlot  = 0;
	}

	endMessage->dataBlock.dIndex     = t->Type();
	endMessage->dataBlock.fWeaponUID = Id();
	endMessage->dataBlock.wIndex 	 = Type();
	if ( hitSomething )
		endMessage->dataBlock.endCode    = FalconMissileEndMessage::MissileKill;
	else
		endMessage->dataBlock.endCode    = FalconMissileEndMessage::Missed;
	endMessage->dataBlock.xDelta    = 0.0f;
	endMessage->dataBlock.yDelta    = 0.0f;
	endMessage->dataBlock.zDelta    = 0.0f;
	if ( t->OnGround() )
	{
		endMessage->dataBlock.x    = t->XPos() + blastRange * PRANDFloat();
		endMessage->dataBlock.y    = t->YPos() + blastRange * PRANDFloat();
		endMessage->dataBlock.z    = t->ZPos();
		endMessage->dataBlock.groundType    =
	   		(char)OTWDriver.GetGroundType ( endMessage->dataBlock.x,
	   											   		endMessage->dataBlock.y );
	}
	else
	{
		// edg note: eventually we want to check damage type prior to
		// placing the end effect
		// effect is 2 secs out from target's current position
		if ( blastRange == 0.0f  )
		{
			endMessage->dataBlock.x    = t->XPos() + t->XDelta() * SimLibMajorFrameTime;
			endMessage->dataBlock.y    = t->YPos() + t->YDelta() * SimLibMajorFrameTime;
			endMessage->dataBlock.z    = t->ZPos() + t->ZDelta() * SimLibMajorFrameTime;
		}
		else
		{
			endMessage->dataBlock.x    = t->XPos() + t->XDelta() * 2.0F;
			endMessage->dataBlock.y    = t->YPos() + t->YDelta() * 2.0F;
			//MI make it delayed a little
			//update our ZPosition every 3 seconds.
			//this makes ALT jinking useful
			if(CheckAltitude)
			{
				//Update our target's alt
				TargetAlt = t->ZPos();
				//start the Flaktimer
				AdjustForAlt = (SimLibElapsedTime + 10 * CampaignSeconds);
				//no need to update now
				CheckAltitude = FALSE;
			}
			if(SimLibElapsedTime > AdjustForAlt)
				CheckAltitude = TRUE;

			//endMessage->dataBlock.z    = t->ZPos();
			endMessage->dataBlock.z = TargetAlt;
		}
		endMessage->dataBlock.groundType    = -1;
	}

	FalconSendMessage (endMessage,FALSE);

	// clear out current shell target
	FireShell( NULL );
}

/*
** Name: GetSMSDomain
**		Returns Air, Land or Both depending on what type of things
**		Gun can shoot at.
*/
WeaponDomain GunClass::GetSMSDomain( void )
{
	return gunDomain;
}

