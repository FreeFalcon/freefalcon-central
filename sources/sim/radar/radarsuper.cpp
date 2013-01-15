#include "stdhdr.h"
#include "Object.h"
#include "simmover.h"
#include "simdrive.h"
#include "otwdrive.h"
#include "camp2sim.h"
#include "team.h"
#include "Entity.h"
#include "classtbl.h"
#include "Graphics\Include\Display.h"
#include "MsgInc\TrackMsg.h"
#include "mfd.h"
#include "campbase.h"
#include "cmpclass.h"
#include "RadarSuper.h"

#include "simio.h"  // MD -- 20040111: added for analog cursor support

static const float	BLIP_SIZE				= 0.02f;
static const float	CURSOR_SIZE				= 0.03f;
static const float	TRACK_SIZE				= 0.05f;
static const float	VELOCITY_FLAG_SCALE		= 0.0000625f;	// .1/1600  len/kts


RadarSuperClass::RadarSuperClass (int type, SimMoverClass* parentPlatform) : RadarClass(type, parentPlatform)
{
	wantMode = mode = AA;
	wantRange = 20.0f;
	wantLock = lockCmd = NOCHANGE;
	prevRange = -1.0f;

	NewRange( wantRange );

	cursorX = 0.0f;
	cursorY = 0.5f;
	flags = 0;
}

void RadarSuperClass::ExecModes( int newDesignate, int newDrop )
{
	// Change modes if such has been requested
	if (mode != wantMode) {
		mode = wantMode;

		// Drop our current target, if any
		ClearSensorTarget();

		// Turn off auto-targeting unless we're in AA
		if (mode != AA) {
			lockCmd = NOCHANGE;
			wantLock = NOCHANGE;
		}
	}

	// Change ranges if such has been requested
	if (wantRange != rangeNM) {
		if ((wantRange >= 5.0f) && (wantRange <= 40.0f)) {

			// Update the cursor position
			if (flags & CursorMoving) {
				// Keep same real world cursor position
				cursorY = cursorY + 1.0f;
				cursorY *= rangeNM / wantRange;
				cursorY = cursorY - 1.0f;
			} else {
				// Recenter the cursors
				cursorX = 0.0F;
				cursorY = 0.5f;
			}
			NewRange( wantRange );
		}
	}

	// Handle targeting commands
	if (newDrop) {
		// Drop our current lock
		if (lockedTarget) SendTrackMsg( lockedTarget, Track_Unlock );
		ClearSensorTarget();
		lockCmd = NOCHANGE;
	} else if (newDesignate) {
		// Designate a new target
		lockCmd = CURSOR;
	} else {
		lockCmd = wantLock;
	}

	// If we're in auto targeting, keep trying to get a lock
	if (wantLock != AUTO) {
		wantLock = NOCHANGE;
	}
}


void RadarSuperClass::UpdateState( int cursorXCmd, int cursorYCmd )
{
	// Handle any requests for cursor movement
	if (cursorXCmd) {
		if ((IO.AnalogIsUsed(AXIS_CURSOR_X) == true) && (IO.AnalogIsUsed(AXIS_CURSOR_Y) == true))
			cursorX += (cursorXCmd / 10000.0F) * CursorRate * SimLibMajorFrameTime;
		else
			cursorX += cursorXCmd * CursorRate * SimLibMajorFrameTime;

		cursorX = min ( max (cursorX, -0.9F), 0.9F);
	}

	if (cursorYCmd) {
		if ((IO.AnalogIsUsed(AXIS_CURSOR_X) == true) && (IO.AnalogIsUsed(AXIS_CURSOR_Y) == true))
			cursorY += (cursorYCmd / 10000.0F) * CursorRate * SimLibMajorFrameTime;
		else
			cursorY += cursorYCmd * CursorRate * SimLibMajorFrameTime;
		cursorY = min ( max (cursorY, -0.9F), 0.9F);
	}

	// Update our display range if the cursors get too close or too far
	if (cursorY >=  0.8f)	RangeStep( 1 );
	if (cursorY <= -0.8f)	RangeStep( -1 );
	
	// Note if the cursors are in motion or not
	if ((cursorXCmd != 0) || (cursorYCmd != 0))
		flags |= CursorMoving;
	else
		flags &= ~CursorMoving;
}


//SimObjectType* RadarSuperClass::Exec (SimObjectType* unused)
SimObjectType* RadarSuperClass::Exec (SimObjectType*)
{
	// Validate our locked target
	CheckLockedTarget();


	// Quit now if we're turned off
	if (!isEmitting)  {
		if (lockedTarget) SendTrackMsg( lockedTarget, Track_Unlock );
		ClearSensorTarget();
		return NULL;
	}

	// Run the appropriate target processing
	if (mode == AA) {
		ExecAA();
	} else {
		ExecAG();
	}

	if (lockedTarget) {
		// Update our seeker center of attention
		SetSeekerPos( TargetAz(platform, lockedTarget), TargetEl(platform, lockedTarget) );
		platform->SetRdrAz( radarData->BeamHalfAngle );
		platform->SetRdrEl( radarData->BeamHalfAngle );
		platform->SetRdrCycleTime (0.0F);
		platform->SetRdrAzCenter( lockedTarget->localData->az );
		platform->SetRdrElCenter( lockedTarget->localData->el );

		// Tag the target as seen this frame
		lockedTarget->localData->rdrLastHit = SimLibElapsedTime;
		//lockedTarget->localData->sensorState[Radar] = SensorTrack; // JB 010210 // JB 010318 taken out
	} else {
		if (mode == AA) {
			SetSeekerPos( 0.0f, 0.0f );
		} else {
			// Compute cursor location in world coordinates, but with us at the origin
			mlTrig yawTrig;
			
			mlSinCos (&yawTrig, platform->Yaw());
			
			float cx =  cursorX;
			float cy = (cursorY + 1.0f);

			float x = (cy*yawTrig.cos - cx*yawTrig.sin) * rangeFT/2.0f;
			float y = (cy*yawTrig.sin + cx*yawTrig.cos) * rangeFT/2.0f;
			
			// Get our height above the ground height at the cursor location
			float z = platform->ZPos() - OTWDriver.GetGroundLevel( x+platform->XPos(), y+platform->YPos() );
#if 0
			// Transform from world space into body space
			float rx = platform->dmx[0][0]*x + platform->dmx[0][1]*y + platform->dmx[0][2]*z;
			float ry = platform->dmx[1][0]*x + platform->dmx[1][1]*y + platform->dmx[1][2]*z;
			float rz = platform->dmx[2][0]*x + platform->dmx[2][1]*y + platform->dmx[2][2]*z;
			
			// Calculate the body relative angles required for the sensor
			float az	= (float)atan2(ry,rx);
			float el	= (float)atan2(rz,sqrt(rx*rx+ry*ry));
#else
			// Calculate the pseudo-body relative angles the sensor _really_ wants right now
			float az	= (float)atan2(y,x) - platform->Yaw();
			float el	= (float)atan2(z,sqrt(x*x+y*y)) - platform->Pitch();
#endif
			SetSeekerPos( az, el );
		}
		platform->SetRdrAz( radarData->ScanHalfAngle );
		platform->SetRdrEl( radarData->ScanHalfAngle );
		platform->SetRdrCycleTime (3.0F);
		platform->SetRdrAzCenter( 0.0f );
		platform->SetRdrElCenter( 0.0f );
	}

	return lockedTarget;
}


void RadarSuperClass::ExecAG( void )
{
	FalconPrivateOrderedList	*list=NULL;	
	FalconEntity*				object=NULL;
	FalconEntity*				newLock=NULL;
	float						x=0.0F, y=0.0F;			// Screen space coordinates (x left/right)
	float						dx=0.0F, dy=0.0F, dz=0.0F;
	float						range=0.0F;
	float						bestSoFar = 1e20f;;
	mlTrig						yaw={0.0F};
	float						scaledCosYaw=1.0F, scaledSinYaw=0.0F;
	float			cursorDelta = BLIP_SIZE*2.0f;
	VU_ID			cursorTgtID = FalconNullId;

	// Convienience synonym for the "At" vector of the platform...
	const float atx	= platform->dmx[0][0];
	const float aty	= platform->dmx[0][1];
	const float atz	= platform->dmx[0][2];


	// Choose the appropriate sim list based on the radar mode
	if (mode == GM) {
		list = SimDriver.combinedFeatureList;	// Buildings
	} else {
		list = SimDriver.combinedList;			// Vehicles
	}
	VuListIterator	objectWalker( list );


	// Prepare our range metrics if we're stepping targets
	if (lockedTarget) {
		newLock = lockedTarget->BaseData();
	} else {
		newLock = NULL;
	}
	if (lockCmd == BORE) {
		bestSoFar = (float)cos( radarData->BeamHalfAngle );
		newLock = NULL;
	} else if (lockCmd == NEXT) {			// Want one further out
		if (lockedTarget) {
			dx = lockedTarget->BaseData()->XPos() - platform->XPos();
			dy = lockedTarget->BaseData()->YPos() - platform->YPos();
			lockedTarget->localData->range = (float)sqrt( dx*dx + dy*dy );
		}
		bestSoFar = 1e20f;
	} else if (lockCmd == PREV) {			// Want one closer in
		if (lockedTarget) {
			dx = lockedTarget->BaseData()->XPos() - platform->XPos();
			dy = lockedTarget->BaseData()->YPos() - platform->YPos();
			lockedTarget->localData->range = (float)sqrt( dx*dx + dy*dy );
		}
		bestSoFar = -1e20f;
	}
	else
	{
		bestSoFar = 0.0F;
	}

	
	// Prepare our rotation from world space into heading north space including feet->screen scaleing
	mlSinCos( &yaw, platform->Yaw() );
	scaledCosYaw = yaw.cos * 2.0f * invRangeFT;
	scaledSinYaw = yaw.sin * 2.0f * invRangeFT;


	// Consider each potential target for locking and cursor identification
	for (object = (FalconEntity*)objectWalker.GetFirst(); 
		 object; 
		 object = (FalconEntity*)objectWalker.GetNext()) {

		// Skip things not on the ground
		if (!object->OnGround()) {
			continue;
		}

		// Figure the first bit of relative geometry we need
		dx = object->XPos() - platform->XPos();
		dy = object->YPos() - platform->YPos();
		dz = object->ZPos() - platform->ZPos();
		range = (float)sqrt( dx*dx + dy*dy );

		// Skip the object if it is out of range or _really_ close
		if ((range > rangeFT) || (range < 500.0f)) {
			continue;
		}

		// Decide where the target will fall on the display
		x = dy*scaledCosYaw - dx*scaledSinYaw;
		y = dy*scaledSinYaw + dx*scaledCosYaw - 1.0f;

		// Skip the object if it is off screen
		if ((fabs(x) > 1.0f) || (fabs(y) > 1.0f)) {
			continue;
		}

		// See if this target is under the cursors
		float d = CursorDelta(x,y);
		if (d < cursorDelta)
		{
			cursorDelta = d;
			cursorTgtID = object->Id();
		}


		// We're done unless we need to lock something up
		if (!lockCmd) {
			continue;
		}


		// Handle the lock command
		switch (lockCmd) {

		  case BORE:
			// We've been asked to lock the target nearest our nose
			{
				float cosATA = (atx*dx + aty*dy + atz*dz) / (float)sqrt(range*range+dz*dz);
				if (cosATA > bestSoFar) {
					bestSoFar = cosATA;
					newLock = object;
					wantLock = NOCHANGE;
				}
			}
			break;

		  case CURSOR:
			// We've been asked to lock a specific target, so find which one...
			if (object->Id() == targetUnderCursor) {
				newLock = object;
				wantLock = NOCHANGE;
			}
			break;

		  case NEXT:
			if (range < bestSoFar) {
				if ((!lockedTarget) || (range > lockedTarget->localData->range)) {
					bestSoFar = range;
					newLock = object;
					wantLock = NOCHANGE;
				}
			}
			break;

		  case PREV:
			if (range > bestSoFar) {
				if ((!lockedTarget) || (range < lockedTarget->localData->range)) {
					bestSoFar = range;
					newLock = object;
					wantLock = NOCHANGE;
				}
			}
			break;

		  default:
			ShiWarning( "Bad lock command" );
		}
	}	// End of our target list traversal loop


	/// Update our locked target
	SetSensorTargetHack( newLock );
	if (lockedTarget)
	{
		dx = lockedTarget->BaseData()->XPos() - platform->XPos();
		dy = lockedTarget->BaseData()->YPos() - platform->YPos();

		lockedTarget->localData->range = (float)sqrt( dx*dx + dy*dy );
		CalcRelValues(	platform, lockedTarget->BaseData(),
						&lockedTarget->localData->az, 
						&lockedTarget->localData->el, 
						&lockedTarget->localData->ata, 
						&lockedTarget->localData->ataFrom, 
						&lockedTarget->localData->droll );
	}


	// Publish the ID of the target under the cursor
	targetUnderCursor = cursorTgtID;
}


void RadarSuperClass::ExecAA( void )
{
	SimObjectType*	object;
	SimObjectType*	newLock;
	float			x, y;
	float			bestSoFar;
	int				sendThisFrame;
	float			cursorDelta = BLIP_SIZE*2.0f;
	VU_ID			cursorTgtID = FalconNullId;

	
	// See if we need to drop lock
	if (lockedTarget) {

		// Drop lock if the guy is outside our radar cone
		if ((fabs( lockedTarget->localData->az ) > radarData->ScanHalfAngle) ||
			(fabs( lockedTarget->localData->el ) > radarData->ScanHalfAngle) ) {
			if (lockedTarget) SendTrackMsg( lockedTarget, Track_Unlock );
			ClearSensorTarget();
			lockCmd = NOCHANGE;
		} else {

			// Drop lock if the guy has been below the signal strength threshhold too long
			if (ReturnStrength(lockedTarget) < 1.0f) {
				// He's faded.  How long has he been hiding?
				if (SimLibElapsedTime - lockedTarget->localData->rdrLastHit > radarData->CoastTime) {
					// Give up and drop lock
					if (lockedTarget) SendTrackMsg( lockedTarget, Track_Unlock );
					ClearSensorTarget();
					lockCmd = NOCHANGE;
				}
			}
		}
	}


	// Prepare our range metrics if we're stepping targets
	newLock = lockedTarget;
	if (lockCmd == AUTO) {
		bestSoFar = rangeFT;
	} else if (lockCmd == BORE) {
		bestSoFar = radarData->BeamHalfAngle;		// Model a somewhat narrow beam
		newLock = NULL;
	} else if (lockCmd == NEXT) {			// Want one further out
		if (lockedTarget) {
			x = lockedTarget->BaseData()->XPos() - platform->XPos();
			y = lockedTarget->BaseData()->YPos() - platform->YPos();
			lockedTarget->localData->range = (float)sqrt( x*x + y*y );
		}
		bestSoFar = 1e20f;
	} else if (lockCmd == PREV) {			// Want one closer in
		if (lockedTarget) {
			x = lockedTarget->BaseData()->XPos() - platform->XPos();
			y = lockedTarget->BaseData()->YPos() - platform->YPos();
			lockedTarget->localData->range = (float)sqrt( x*x + y*y );
		}
		bestSoFar = -1e20f;
	}
	else
	{
		bestSoFar = 0.0F;
	}


	// Consider each potential target in our environment for lock and cursor identification
	for (object = platform->targetList; object; object = object->next) {

		// Skip ground objects in AA mode
		if ( object->BaseData()->OnGround() ) {
			continue;
		}

		// Skip anything beyond our display range
		if (object->localData->range > rangeFT) {
			continue;
		}

		// Skip weapons
		if ( object->BaseData()->IsMissile() || object->BaseData()->IsBomb())
		{
			continue;
		}

		// Skip ejected pilots
		if ( object->BaseData()->IsEject() )
		{
			continue;
		}

		// Skip anything not in our radar pyramid
		if (max(fabs(object->localData->az), fabs(object->localData->el)) > radarData->ScanHalfAngle) {
			continue;
		}

		// Relative position for target under cursor
		x = TargetAz( platform, object ) / radarData->ScanHalfAngle;
		y = 2.0f * object->localData->range * invRangeFT - 1.0f;
		float d = CursorDelta(x,y);
		if (d < cursorDelta)
		{
			cursorDelta = d;
			cursorTgtID = object->BaseData()->Id();
		}

		// We're done unless we need to acquire a lock
		if (!lockCmd) {
			continue;
		}

		// Don't allow locks on anything below the signal strength threshhold
		if (ReturnStrength(object) < 1.0f) {
			continue;
		}

		// Handle the lock command
		switch (lockCmd) {

		  case AUTO:
			// While holding a lock, ignore this command
			if (lockedTarget)
				break;

			// If this is the nearest "threat" object in front of us, pick it
			if (object->localData->range <= bestSoFar) {
				if ( TeamInfo[platform->GetTeam()]->TStance(object->BaseData()->GetTeam()) == War ) {
					bestSoFar = object->localData->range;
					newLock = object;
				}
			}
			break;

		  case BORE:
			// We've been asked to lock the target nearest our nose
			if (object->localData->ata < bestSoFar) {
				bestSoFar = object->localData->ata;
				newLock = object;
				wantLock = NOCHANGE;
			}
			break;

		  case CURSOR:
			// We've been asked to lock a specific target, so find which one...
			if (object->BaseData()->Id() == targetUnderCursor) {
				newLock = object;
				wantLock = NOCHANGE;
			}
			break;

		  case NEXT:
			if (object->localData->range < bestSoFar) {
				if ((!lockedTarget) || (object->localData->range > lockedTarget->localData->range)) {
					bestSoFar = object->localData->range;
					newLock = object;
					wantLock = NOCHANGE;
				}
			}
			break;

		  case PREV:
			if (object->localData->range > bestSoFar) {
				if ((!lockedTarget) || (object->localData->range < lockedTarget->localData->range)) {
					bestSoFar = object->localData->range;
					newLock = object;
					wantLock = NOCHANGE;
				}
			}
			break;

		  default:
			ShiWarning( "Bad lock command" );
		}
	}	// End of our target list traversal loop


	// If we changed locks, immediatly notify those concerned and update our state
	if (newLock != lockedTarget) {
		SetDesiredTarget( newLock );
		sendThisFrame = TRUE;
	} else {
		// See if it is time to send a "lock" update
		sendThisFrame = lockedTarget && (SimLibElapsedTime - lastTargetLockSend > TrackUpdateTime);
	}

	// Send our periodic lock message
	if ((sendThisFrame) && (lockedTarget)) {
		SendTrackMsg( lockedTarget, Track_Lock );
		lastTargetLockSend = SimLibElapsedTime;
	}


	// Publish the ID of the target under the cursor
	targetUnderCursor = cursorTgtID;
}


void RadarSuperClass::Display(VirtualDisplay *activeDisplay)
{
	// For now we have to do this silly thing to placate the SMS display routine --
	// we really should get ride of the display/privateDisplay dicotemy.
	display = activeDisplay;

	// Quit now if we're turned off
	if (!isEmitting) {
		return;
		display->TextCenter( 0.0f, 0.0f, "RADAR OFF" );
	}

	// Now draw the radar cursors and locked target data
//	display->SetColor( 0x0000FF00 );
	DrawButtons();
	DrawCursor();
	DrawWaterline();
	DrawBullseyeData();

	// Display the appropriate targets 
	if (mode == AA) {
		DisplayAAReturns();
	} else {
		DisplayAGReturns();
	}
}


void RadarSuperClass::DisplayAGReturns(void)
{
	FalconPrivateOrderedList	*list;	
	FalconEntity				*object;
	float						scaledCosYaw, scaledSinYaw;
	float						x, y;			// Screen space coordinates (x left/right)
	float						dx, dy, dz;
	float						range;
	mlTrig trig;


	// Choose the appropriate sim list based on the radar mode
	if (mode == GM) {
		list = SimDriver.combinedFeatureList;	// Buildings
	} else {
		list = SimDriver.combinedList;			// Vehicles
	}
	VuListIterator	objectWalker( list );


	// Prepare our rotation from world space into heading north space including feet->screen scaleing
	mlSinCos (&trig, platform->Yaw());
	scaledCosYaw = trig.cos * 2.0f * invRangeFT;
	scaledSinYaw = trig.sin * 2.0f * invRangeFT;

	// Adjust screen space to make display easier
	display->AdjustOriginInViewport( 0.0f, -1.0f );

	// Consider each potential target for display
	for (object = (FalconEntity*)objectWalker.GetFirst(); 
		 object; 
		 object = (FalconEntity*)objectWalker.GetNext()) {

		// Skip things not on the ground
		if (!object->OnGround()) {
			continue;
		}

		// Figure out the little bit of relative geometry we need
		dx = object->XPos() - platform->XPos();
		dy = object->YPos() - platform->YPos();
		dz = object->ZPos() - platform->ZPos();
		range = (float)sqrt( dx*dx + dy*dy + dz*dz );
		x = dy*scaledCosYaw - dx*scaledSinYaw;	// Rotate into heading up plan view space
		y = dy*scaledSinYaw + dx*scaledCosYaw;	// and scale from feet into viewport space


		// Skip the object if it is out of range or _really_ close
		if ((range > rangeFT) || (range < 1000.0f)) {
			continue;
		}

		// Draw the appropriate target symbol
		if (lockedTarget && object == lockedTarget->BaseData()) {

			DrawLockedGndInfo( x, y );

		} else {

			display->Tri( x-BLIP_SIZE, y-BLIP_SIZE, x-BLIP_SIZE, y+BLIP_SIZE, x+BLIP_SIZE, y+BLIP_SIZE );
			display->Tri( x-BLIP_SIZE, y-BLIP_SIZE, x+BLIP_SIZE, y-BLIP_SIZE, x+BLIP_SIZE, y+BLIP_SIZE );

		}
	}

	// Clear the viewport shift and spin
	display->CenterOriginInViewport();
	display->ZeroRotationAboutOrigin();
}


void RadarSuperClass::DisplayAAReturns(void)
{
	SimObjectType*		object;
	float				x, y;			// Screen space coordinates (x left/right)
	char				tmpStr[4];
	int tmpColor = display->Color();


	// Consider each potential target for display
	for (object = platform->targetList; object; object = object->next) {

		// Only consider objects in the appropriate domain (air/land)
		if (object->BaseData()->OnGround()) {
			continue;
		}

		// Skip the object if it is out of range
		if (object->localData->range > rangeFT) {
			continue;
		}

		// Skip weapons
		if (object->BaseData()->IsMissile() || object->BaseData()->IsBomb()) {
			continue;
		}

		// Skip ejected pilots
		if ( object->BaseData()->IsEject() )
		{
			continue;
		}

		// Skip anything not in our radar pyramid
		if (max(fabs(object->localData->az), fabs(object->localData->el)) > radarData->ScanHalfAngle) {
			continue;
		}

		// Okay, it qualifies, so draw the appropriate target symbol
		x = TargetAz( platform, object ) / radarData->ScanHalfAngle;
		y = object->localData->range * 2.0f * invRangeFT - 1.0f;

		if (object == lockedTarget) {

			DrawLockedAirInfo( x, y );

		} else {

			// Dim the target mark if it is below the signal strength threshhold
			if (ReturnStrength(object) >= 1.0f)
			{
				// Full intensity
				display->SetColor( tmpColor );
			} else {
				// Dimmed
				display->SetColor( (tmpColor > 4 ) && 0xFF00 );
			}

			display->Tri( x-BLIP_SIZE, y-BLIP_SIZE, x-BLIP_SIZE, y+BLIP_SIZE, x+BLIP_SIZE, y+BLIP_SIZE );
			display->Tri( x-BLIP_SIZE, y-BLIP_SIZE, x+BLIP_SIZE, y-BLIP_SIZE, x+BLIP_SIZE, y+BLIP_SIZE );

			if (object->BaseData()->Id() == targetUnderCursor)
			{
				sprintf (tmpStr, "%.0f", -object->BaseData()->ZPos() * 0.001F);
				ShiAssert( strlen(tmpStr) < sizeof(tmpStr) );
				display->TextCenter (x, y - 1.5F * BLIP_SIZE, tmpStr);
			}
		}
	}

	// Put the display color back the way we got it
	display->SetColor( tmpColor );
}


float RadarSuperClass::CursorDelta( float x, float y )
{
	return (float)max( fabs(x - cursorX), fabs(y - cursorY) );
}


void RadarSuperClass::DrawCursor(void)
{
	char	str[8];
	float	ang;
	float	high;
	float	low;

	display->AdjustOriginInViewport (cursorX, cursorY);

	// Draw the vertical cursor bars
	display->Line( -CURSOR_SIZE, -CURSOR_SIZE, -CURSOR_SIZE, CURSOR_SIZE );
	display->Line(  CURSOR_SIZE, -CURSOR_SIZE,  CURSOR_SIZE, CURSOR_SIZE );

	// Compute the evelation limits of the scan volume
	ang = platform->Pitch() + radarData->ScanHalfAngle;
	if (ang > 90.0f * DTR) {
		high = 1.0;
	} else {
		high = (float)sin( ang );
	}

	ang = platform->Pitch() - radarData->ScanHalfAngle;
	if (ang < -90.0f * DTR) {
		low = -1.0;
	} else {
		low = (float)sin( ang );
	}

	// Convert to feet and factor in the height of our platform
	high = 0.001f * (high * rangeFT - platform->ZPos());
	low  = 0.001f * (low  * rangeFT - platform->ZPos());

	// Clamp to legal display range
	high	= min( max(high, 0.0F), 99.0F);
	low		= min( max(low,  0.0F), 99.0F);


	// Print the evelation limits of the scan volume
	sprintf(str,"%02d",(int)high);
	ShiAssert (strlen(str) < sizeof(str));
	display->TextLeft(0.06F, 0.055F, str);

	sprintf(str,"%02d",(int)low);
	ShiAssert (strlen(str) < sizeof(str));
	display->TextLeft(0.06F,-0.035F, str);

	display->CenterOriginInViewport();
}


void RadarSuperClass::DrawBullseyeData( void )
{
	float az, range;
	float cursX, cursY;
	char str[12];
	float bullseyeX, bullseyeY;
   mlTrig trig;
	
	TheCampaign.GetBullseyeSimLocation (&bullseyeX, &bullseyeY);

	// Compute cursor location in world space
	if (mode == AA) {
		// BScope presentation
		az    = cursorX * radarData->ScanHalfAngle;
		range = (cursorY + 1.0f) * 0.5f * rangeFT;
      mlSinCos (&trig, az);
		cursX = trig.sin * range;
		cursY = trig.cos * range;
		cursX += platform->XPos();
		cursY += platform->YPos();
	} else {
		// God's eye presentation
		GetAGCenter ( &cursX, &cursY );
	}

	// Compute azmuth and range from bullseye point
	az = RTD * (float)atan2 (cursY - bullseyeY, cursX - bullseyeX);
	range = (float)sqrt( (cursX-bullseyeX)*(cursX-bullseyeX) + (cursY-bullseyeY)*(cursY-bullseyeY) );
	if (az < -0.6f)
		az += 360.0f;

	// Present the data on the display
	sprintf( str, "%03.0f %02.0f NM", az, range*FT_TO_NM );
	ShiAssert (strlen (str) < sizeof(str));
	display->TextLeft( -0.95F, -0.75F, str );
	display->TextLeft( -0.95f, -0.65f, "BULLSEYE");
}


void RadarSuperClass::DrawLockedAirInfo( float h, float v )
{
	static const float		trackTriH = TRACK_SIZE * (float)cos( DTR * 30.0f );
	static const float		trackTriV = TRACK_SIZE * (float)sin( DTR * 30.0f );
	float					value;
	float					x, y;		// Screen space coordinates (x left/right)
	char					str[24];
	Falcon4EntityClassType*	classPtr;
	int tmpColor = display->Color();


	ShiAssert( lockedTarget );


	// Display the locked target's track data
	// Aspect
	value = lockedTarget->localData->aspect * RTD;
	sprintf (str, "%02.0f%c", value, (lockedTarget->localData->azFrom > 0.0F ? 'R' : 'L'));
	ShiAssert( strlen(str) < sizeof(str) );
	display->TextLeft(-0.875F, 0.825F, str);

	// Heading
	value = lockedTarget->BaseData()->Yaw() * RTD;
	if (value < 1.0f)  value += 360.0f;
	sprintf (str, "%03.0f", floor(value));
	ShiAssert( strlen(str) < sizeof(str) );
	display->TextLeft(-0.5F, 0.825F, str);

	// Target calibrated airspeed
    value = lockedTarget->BaseData()->GetKias();
	sprintf (str, "%03.0fkt", value);
	ShiAssert( strlen(str) < sizeof(str) );
	display->TextRight(0.40F, 0.825F, str);

	// Closure
	value = -lockedTarget->localData->rangedot * FTPSEC_TO_KNOTS;
	sprintf (str, "%03.0f", value);
	ShiAssert( strlen(str) < sizeof(str) );
	display->TextRight(0.875F, 0.825F, str);

	// Target ID (NCTR)
	classPtr = (Falcon4EntityClassType*)lockedTarget->BaseData()->EntityType();
	if (lockedTarget->BaseData()->IsSim() && (!((SimBaseClass*)lockedTarget->BaseData())->IsExploding()) && 
		(classPtr->dataType == DTYPE_VEHICLE)) {
		sprintf (str, "%s", ((VehicleClassDataType*)(classPtr->dataPtr))->Name);
		ShiAssert( strlen(str) < sizeof(str) );
		display->TextCenter(0.0F, 0.75F, str);
	}


	// Mark our antenna elevation and azmuth to the locked target
	y = lockedTarget->localData->el / radarData->ScanHalfAngle;
	x = lockedTarget->localData->az / radarData->ScanHalfAngle;

	display->Line( -0.85f, y,           -0.80f, y );
	display->Line( -0.85f, y+BLIP_SIZE, -0.85f, y-BLIP_SIZE );
	display->Line( x,           -0.80f, x,           -0.85f );
	display->Line( x+BLIP_SIZE, -0.85f, x-BLIP_SIZE, -0.85f );


	// Mark where the locked target will be
	display->AdjustOriginInViewport( h, v );
	display->Circle ( 0.0f, 0.0f, TRACK_SIZE );

	// Dim the target mark if it is below the signal strength threshhold
	if (ReturnStrength(lockedTarget) >= 1.0f)
	{
		// Full intensity
		display->SetColor( tmpColor );
	} else {
		// Dimmed
		display->SetColor( (tmpColor >> 4) & 0xFF00 );
	}

	// Draw the locked target's symbol (triangle with velocity line)
	value = lockedTarget->BaseData()->Yaw() - platform->Yaw() - h*radarData->ScanHalfAngle;
	display->AdjustRotationAboutOrigin( value );
	display->Tri (0.0f, TRACK_SIZE, trackTriH, -trackTriV, -trackTriH, -trackTriV);
	display->Line (0.0f, TRACK_SIZE, 0.0f, TRACK_SIZE + lockedTarget->BaseData()->GetVt()*VELOCITY_FLAG_SCALE);
	display->ZeroRotationAboutOrigin();

	display->SetColor( tmpColor );


	// Display the target's altitude
	value = -lockedTarget->BaseData()->ZPos();
	sprintf(str,"%02d",(int)(value*0.001f));
	ShiAssert (strlen(str) < sizeof(str));
	display->TextCenter(0.0F, -0.06F, str);

	// Undo the viewpoint origin shift
	display->CenterOriginInViewport();
}


void RadarSuperClass::DrawLockedGndInfo( float h, float v )
{
	float					x, y;		// Screen space coordinates (x left/right)
	char					string[16];

	ShiAssert( lockedTarget );


	// Shift to a target centric frame
	display->AdjustOriginInViewport( h, v );

	// Mark where the locked target will be
	display->Line ( 0.0f, TRACK_SIZE, TRACK_SIZE, 0.0f );
	display->Line ( TRACK_SIZE, 0.0f, 0.0f, -TRACK_SIZE );
	display->Line ( 0.0f, -TRACK_SIZE, -TRACK_SIZE, 0.0f );
	display->Line ( -TRACK_SIZE, 0.0f, 0.0f, TRACK_SIZE );

	// Draw the locked target's symbol (just a blip inside the lock marker)
	display->Tri( -BLIP_SIZE, -BLIP_SIZE, -BLIP_SIZE, +BLIP_SIZE, +BLIP_SIZE, +BLIP_SIZE );
	display->Tri( -BLIP_SIZE, -BLIP_SIZE, +BLIP_SIZE, -BLIP_SIZE, +BLIP_SIZE, +BLIP_SIZE );


	// Clear the viewport shift and spin
	display->CenterOriginInViewport();
	display->ZeroRotationAboutOrigin();


	// Target ID (NCTR)
	if (lockedTarget->BaseData()->IsSim() && !((SimBaseClass*)lockedTarget->BaseData())->IsExploding())
	{
		Falcon4EntityClassType *classPtr = (Falcon4EntityClassType*)lockedTarget->BaseData()->EntityType();

		if (classPtr->dataType == DTYPE_VEHICLE) {
			sprintf (string, "%s", ((VehicleClassDataType*)(classPtr->dataPtr))->Name);
			ShiAssert( strlen(string) < sizeof(string) );
			display->TextCenter(0.0F, 0.75F, string);
		} else if (classPtr->dataType == DTYPE_FEATURE) {
			sprintf (string, "%s", "BLDG");
			ShiAssert( strlen(string) < sizeof(string) );
			display->TextCenter(0.0F, 0.75F, string);
		}
	}

	// Mark our antenna elevation and azmuth to the locked target
	y = lockedTarget->localData->el / radarData->ScanHalfAngle;
	x = lockedTarget->localData->az / radarData->ScanHalfAngle;

	display->Line( -0.85f, y,           -0.80f, y );
	display->Line( -0.85f, y+BLIP_SIZE, -0.85f, y-BLIP_SIZE );
	display->Line( x,           -0.80f, x,           -0.85f );
	display->Line( x+BLIP_SIZE, -0.85f, x-BLIP_SIZE, -0.85f );


	// Put the viewport shift back the way we got it
	display->AdjustOriginInViewport( 0.0f, -1.0f );
}


void RadarSuperClass::DrawWaterline(void)
{
	float	yPos, theta;
	mlTrig	rot;

	static const float	InsideEdge	= 0.08f;
	static const float	OutsideEdge	= 0.40f;
	static const float	Height		= 0.04f;

	theta  = -platform->Pitch();
	if(theta > 45.0F * DTR)
		theta = 45.0F * DTR;
	else if( theta < -45.0F * DTR)
		theta = -45.0F * DTR;

	yPos = theta / (60.0f * DTR);

	// Rotate the local origin to account for body roll
	mlSinCos (&rot, -platform->Roll());
	display->AdjustOriginInViewport (rot.sin*yPos, rot.cos*yPos);
	display->AdjustRotationAboutOrigin (-platform->Roll());

	display->Line(	OutsideEdge,	-Height,	 OutsideEdge,	0.0f);
	display->Line(	OutsideEdge,	 0.0f,		 InsideEdge,	0.0f);
	display->Line(	-OutsideEdge,	-Height,	-OutsideEdge,	0.0f);
	display->Line(	-OutsideEdge,	 0.0f,		-InsideEdge,	0.0f);

	display->ZeroRotationAboutOrigin();
	display->CenterOriginInViewport();
}


void RadarSuperClass::DrawButtons( void )
{
	static const float arrowH = 0.0375f;
	static const float arrowW = 0.0433f;
	char string[4];

	// Draw the range label and change arrows
	sprintf( string, "%0.0f", rangeNM );
	ShiAssert( strlen(string) < sizeof(string) );
	display->AdjustOriginInViewport( -0.92f, 0.0f );
	display->TextCenter( arrowW, 0.385f, string );
	display->AdjustOriginInViewport( 0.0f, 0.50F );
	display->Tri( 0.0f, arrowH, arrowW, -arrowH, -arrowW, -arrowH );
	display->AdjustOriginInViewport( 0.0f, -0.25F );
	display->Tri( 0.0f, -arrowH, arrowW, arrowH, -arrowW, arrowH );
	display->CenterOriginInViewport();

	// Draw the mode change button and indicate current mode
	if (mode == AA) {
		LabelButton (1, "ACM", "", lockCmd == AUTO);
	}
	LabelButton (2, "AA",  "", mode == AA);
	if (mode == GMT) {
		LabelButton (3, "GMT",  "", TRUE);
	} else {
		LabelButton (3, "GM",  "", mode == GM);
	}
	LabelButton (13, "FCR", NULL, 1);
	LabelButton (14, "SWAP", "");
}


void RadarSuperClass::PushButton( int whichButton, int whichMFD )
{
	switch( whichButton ) {
	case 1:
		if (mode == AA) {
			wantLock = AUTO;
		}
		break;
	case 2:
		StepAAmode();
		break;
	case 3:
		StepAGmode();
		break;
	case 13:
		MfdDisplay[whichMFD]->SetNewMode(MFDClass::MfdMenu);
		break;
	case 14:
		MFDSwapDisplays();
		break;
	case 18:
		RangeStep( -1 );
		break;
	case 19:
		RangeStep( 1 );
		break;
	}
}


void RadarSuperClass::NewRange( float newRange )
{
	// Round the new range to the nearest integer
	newRange = (float)floor( newRange + 0.5f );

	// Keep our various measures of range consistent
	rangeFT = newRange*NM_TO_FT;
	rangeNM = newRange;
	invRangeFT = 1.0f/rangeFT;
}


void RadarSuperClass::StepAAmode( void )
{
	if (wantMode == AA) {
		if (wantLock == AUTO) {
			wantLock = NOCHANGE;
		} else {
			wantLock = AUTO;
		}
	} else {
		wantMode = AA; 
		wantLock = NOCHANGE;
	}
}


void RadarSuperClass::SetSRMOverride(void)
{
	if (prevRange < 0.0f) {
		prevMode = mode;
		prevRange = rangeNM;
	}
	wantMode = AA;
	wantLock = AUTO;
	wantRange = 10.0f;
}


void RadarSuperClass::SetMRMOverride(void)
{
	if (prevRange < 0.0f) {
		prevMode = mode;
		prevRange = rangeNM;
	}

	wantMode = AA;
	wantLock = NOCHANGE;
	wantRange = 20.0f;
}


void RadarSuperClass::ClearOverride(void)
{
	if (prevRange > 0.0f) {
		wantMode = prevMode;
		wantRange = prevRange;
		wantLock = NOCHANGE;
		prevRange = -1.0f;
	}
}

void RadarSuperClass::GetAGCenter (float *x, float *y)
{
	mlTrig yawTrig;
	mlSinCos( &yawTrig, platform->Yaw() );
	
	float cx =  cursorX;
	float cy = (cursorY + 1.0f);
	
	*x = (cy*yawTrig.cos - cx*yawTrig.sin) * rangeFT/2.0f + platform->XPos();
	*y = (cy*yawTrig.sin + cx*yawTrig.cos) * rangeFT/2.0f + platform->YPos();
}

void RadarSuperClass::SetMode (RadarMode cmd)
{
   wantMode = cmd;
}
