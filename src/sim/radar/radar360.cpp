#include "stdhdr.h"
#include "classtbl.h"
#include "entity.h"
#include "Object.h"
#include "simdrive.h"
#include "simmover.h"
#include "otwdrive.h"
#include "camp2sim.h"
#include "team.h"
#include "Graphics/Include/Display.h"
#include "MsgInc/TrackMsg.h"
#include "mfd.h"
#include "Entity.h"
#include "campbase.h"
#include "cmpclass.h"
#include "Radar360.h"

#include "simio.h"  // MD -- 20040111: added for analog cursor support

static const float	BLIP_SIZE				= 0.04f;
static const float	RADAR_CONE_ANGLE		= 0.33f * PI;
static const float	SIN_RADAR_CONE_ANGLE	= (float)sin(RADAR_CONE_ANGLE);
static const float	COS_RADAR_CONE_ANGLE	= (float)cos(RADAR_CONE_ANGLE);
static const float	TAN_RADAR_CONE_ANGLE	= (float)tan(RADAR_CONE_ANGLE);


Radar360Class::Radar360Class( int type, SimMoverClass* parentPlatform ) : RadarClass(type, parentPlatform)
{
	wantMode = mode = AA;
	wantRange = 20.0f;
	wantLock = lockCmd = NOCHANGE;

	SetSeekerPos( 0.0f, 0.0f );
	NewRange( wantRange );

	cursorX = 0.0f;
	cursorY = 0.5f;
	flags = 0;

	maxRangeNM = 40.0; // JB 011213
	AWACSMode = false; // JB 011213
}


// NOTE:  Right now AUTO mode is functional, BUT THERE IS NOT WAY TO GET INTO IT.  
//        I'm considering removing it.  SCR 9/2/98
void Radar360Class::ExecModes( int newDesignate, int newDrop )
{
	// Stick in AUTO mode until we are commaned out or get a lock
	if ((lockCmd == AUTO) && (lockedTarget == NULL)) {
		wantLock = AUTO;
	}

	// Change modes if such has been requested
	if (mode != wantMode) {
		mode = wantMode;
		wantLock = NOCHANGE;

		// Drop our current target, if any
		ClearSensorTarget();
	}

	// Change ranges if such has been requested
	if (wantRange != rangeNM) {
		if ((wantRange >= 5.0f) && (wantRange <= max(40.0, maxRangeNM * 1.5))) {
         if (!(flags & CursorMoving)) {
            cursorY = 0.0F;
         }
			NewRange( wantRange );
		}
	}

	// Handle targeting commands
	if (newDrop) {
		lockCmd = NOCHANGE;
		ClearSensorTarget();
	} else if (newDesignate) {
		// Designate a new target
		lockCmd = CURSOR;
	} else {
		lockCmd = wantLock;
	}

	wantLock = NOCHANGE;
}


void Radar360Class::UpdateState( int cursorXCmd, int cursorYCmd )
{
	// Handle any requests for cursor movement
	if (cursorXCmd != 0) {
		if ((IO.AnalogIsUsed(AXIS_CURSOR_X) == true) && (IO.AnalogIsUsed(AXIS_CURSOR_Y) == true))
			cursorX += (cursorXCmd / 10000.0F) * CursorRate * SimLibMajorFrameTime;
		else
			cursorX += cursorXCmd * CursorRate * SimLibMajorFrameTime;

		cursorX = min ( max (cursorX, -1.0F), 1.0F);

		if (!AWACSMode && fabs(cursorX) > cursorY*TAN_RADAR_CONE_ANGLE ) {
			cursorY = (float)fabs(cursorX) / TAN_RADAR_CONE_ANGLE;
		}
	}

	if (cursorYCmd != 0) {
		if ((IO.AnalogIsUsed(AXIS_CURSOR_X) == true) && (IO.AnalogIsUsed(AXIS_CURSOR_Y) == true))
			cursorY += (cursorYCmd / 10000.0F) * CursorRate * SimLibMajorFrameTime;
		else
			cursorY += cursorYCmd * CursorRate * SimLibMajorFrameTime;

		if (!AWACSMode)
			cursorY = min ( max (cursorY,  0.0F), 1.0F);
		else
			cursorY = min ( max (cursorY,  -1.0F), 1.0F);

		if (!AWACSMode && fabs(cursorX) > cursorY*TAN_RADAR_CONE_ANGLE ) {
			if (cursorX >= 0.0f) {
				cursorX = cursorY * TAN_RADAR_CONE_ANGLE;
			} else {
				cursorX = -cursorY * TAN_RADAR_CONE_ANGLE;
			}
		}
	}

   if ((cursorXCmd != 0) || (cursorYCmd != 0))
      flags |= CursorMoving;
   else
      flags &= ~CursorMoving;
}


//SimObjectType* Radar360Class::Exec (SimObjectType* unused)
SimObjectType* Radar360Class::Exec (SimObjectType*)
{
	// Validate our locked target
	CheckLockedTarget();

	// Quit now if we're turned off
	if (!isEmitting)  {
		SetSensorTargetHack( NULL );
		return NULL;
	}

	// Do the exec appropriate for the selected mode
	if (mode == AA) {
		ExecAA();
	} else {
		ExecAG();
	}
	
	// Tell the base class and the rest of the world where we're looking
	if (lockedTarget)
	{
		// TODO:  Should be radarData->BeamHalfAngle
		platform->SetRdrAz( 5.0F * DTR );
		platform->SetRdrEl( 5.0F * DTR );
		platform->SetRdrCycleTime (0.0F);
		platform->SetRdrAzCenter( lockedTarget->localData->az );
		platform->SetRdrElCenter( lockedTarget->localData->el );
	}
	else
	{
		platform->SetRdrAz( radarData->ScanHalfAngle );
		platform->SetRdrEl( radarData->ScanHalfAngle );
		platform->SetRdrCycleTime (3.0F);
		platform->SetRdrAzCenter( 0.0f );
		platform->SetRdrElCenter( 0.0f );
	}

	return lockedTarget;
}


void Radar360Class::ExecAA( void )
{
	SimObjectType*	object=NULL;
	SimObjectType*	newLock=NULL;
	float			dx=0.0F, dy=0.0F;
	float			x=0.0F, y=0.0F;
	float			scaledCosYaw=1.0F, scaledSinYaw=0.0F;
	int				sendThisFrame=FALSE;
	float			bestSoFar = rangeFT;
	float			range=0.0F;
	float			cursorDelta = BLIP_SIZE*2.0f;
	VU_ID			cursorTgtID = FalconNullId;


	// Prepare our rotation from world space into heading north space including feet->screen scaleing
	scaledCosYaw = (float)cos( platform->Yaw() ) * invRangeFT;
	scaledSinYaw = (float)sin( platform->Yaw() ) * invRangeFT;


	// Prepare our range metrics if we're stepping targets
	newLock = lockedTarget;
	if (lockCmd == AUTO) {
		bestSoFar = rangeFT;
		newLock = NULL;
	} else if (lockCmd == BORE) {
		bestSoFar = RADAR_CONE_ANGLE;
		newLock = NULL;
	} else if (lockCmd == NEXT) {	// Want one further out
		if (lockedTarget) {
			dx = lockedTarget->BaseData()->XPos() - platform->XPos();
			dy = lockedTarget->BaseData()->YPos() - platform->YPos();
			lockedTarget->localData->range = (float)sqrt( dx*dx + dy*dy );
		}
		bestSoFar = 1e20f;
	} else if (lockCmd == PREV) {	// Want one closer in
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


	// Consider each potential target in our environment
	for (object = platform->targetList; object; object = object->next)
	{

		if ( object->BaseData()->OnGround() )
		{
		   // Skip ground objects in AA mode
		   continue;
		}

		// Can't lock onto weapons in flight
		if (object->BaseData()->IsMissile() || object->BaseData()->IsBomb() )
		{
			continue;
		}

		// Can't lock onto ejected pilots
		if (object->BaseData()->IsEject())
		{
			continue;
		}

		// If we're looking for a lock or to check for under cursor, we'll need this stuff...
		dx = object->BaseData()->XPos() - platform->XPos();
		dy = object->BaseData()->YPos() - platform->YPos();
		x = dy*scaledCosYaw - dx*scaledSinYaw;	// Rotate into heading up plan view space
		y = dy*scaledSinYaw + dx*scaledCosYaw;	// and scale from feet into viewport space
		range = (float)sqrt( dx*dx + dy*dy );

		// Mark the thing under the cursor
		float d = CursorDelta(x,y);
		if (d < cursorDelta)
		{
			cursorDelta = d;
			cursorTgtID = object->BaseData()->Id();
		}

		// We're done unless we need to lock something up
		if ((!lockCmd) || (range > rangeFT))
		{
			continue;
		}

		// Skip the object if it can't be locked
		if ( !AWACSMode && !InAALockZone( object, x, y ) ) {	// M.N. in AWACS mode, allow 360° locking
			continue;
		}


		// Handle the lock command
		switch (lockCmd) {

		  case AUTO:
			// If this is the nearest "threat" object in front of us, pick it
			if (range <= bestSoFar)
			{
				if ( TeamInfo[platform->GetTeam()]->TStance(object->BaseData()->GetTeam()) == War )
				{
					bestSoFar = range;
					newLock = object;
				}
			}
			break;

		  case CURSOR:
			// We've been asked to lock a specific target, so find which one...
			if (object->BaseData()->Id() == targetUnderCursor)
			{
				newLock = object;
			}
			break;

		  case BORE:
			// We've been asked to lock the target nearest our nose
			if (object->localData->ata < bestSoFar)
			{
				bestSoFar = object->localData->ata;
				newLock = object;
			}
			break;

		  case NEXT:
			if (range < bestSoFar)
			{
				if ((!lockedTarget) || (range > lockedTarget->localData->range))
				{
					if ( TeamInfo[platform->GetTeam()]->TStance(object->BaseData()->GetTeam()) == War )
					{
						bestSoFar = range;
						newLock = object;
					}
				}
			}
			break;

		  case PREV:
			if (range > bestSoFar)
			{
				if ((!lockedTarget) || (range < lockedTarget->localData->range))
				{
					if ( TeamInfo[platform->GetTeam()]->TStance(object->BaseData()->GetTeam()) == War )
					{
						bestSoFar = range;
						newLock = object;
					}
				}
			}
			break;

		  default:
			ShiWarning( "Bad lock command" );
		}
	}	// End of our target list traversal loop


	// If we changed locks, tell our previous target he's off the hook
	if (newLock != lockedTarget)
	{
		SetDesiredTarget( newLock );
		sendThisFrame = TRUE;
	} else {
		// See if it is time to send a "painted" list update
		sendThisFrame = (SimLibElapsedTime - lastTargetLockSend > TrackUpdateTime);
	}

	// Tell our current target he's locked
	if (sendThisFrame && lockedTarget)
	{
		SendTrackMsg( lockedTarget, Track_Lock );
		lastTargetLockSend = SimLibElapsedTime;
	}


	// Update our seeker center of attention
	if (lockedTarget)
	{
		SetSeekerPos( TargetAz(platform, lockedTarget), TargetEl(platform, lockedTarget) );
	}
	else
	{
		SetSeekerPos( 0.0f, 0.0f );
	}

	// Publish the ID of the target under the cursor
	targetUnderCursor = cursorTgtID;
}


void Radar360Class::ExecAG( void )
{
	FalconEntity*	object;
	FalconEntity*	newLock;
	float			dx, dy, dz;
	float			x, y;
	float			scaledCosYaw, scaledSinYaw;
	float			range;
	float			cosATA;
	float			bestSoFar;
	VuListIterator	*walker;
	mlTrig			yaw;
	float			cursorDelta = BLIP_SIZE*2.0f;
	VU_ID			cursorTgtID = FalconNullId;
	float			d;

	const float atx	= platform->dmx[0][0];
	const float aty	= platform->dmx[0][1];
	const float atz	= platform->dmx[0][2];


	// Prepare our rotation from world space into heading north space including feet->screen scaleing
	mlSinCos( &yaw, platform->Yaw() );
	scaledCosYaw = yaw.cos * invRangeFT;
	scaledSinYaw = yaw.sin * invRangeFT;


	// Prepare our range metrics if we're stepping targets
	if (lockedTarget) {
		newLock = lockedTarget->BaseData();
	} else {
		newLock = NULL;
	}
	if (lockCmd == AUTO) {
		bestSoFar = rangeFT;
		newLock = NULL;
	} else if (lockCmd == BORE) {
		bestSoFar = COS_RADAR_CONE_ANGLE;
		newLock = NULL;
	} else if (lockCmd == NEXT) {	// Want one further out
		if (lockedTarget) {
			dx = lockedTarget->BaseData()->XPos() - platform->XPos();
			dy = lockedTarget->BaseData()->YPos() - platform->YPos();
			lockedTarget->localData->range = (float)sqrt( dx*dx + dy*dy );
		}
		bestSoFar = 1e20f;
	} else if (lockCmd == PREV) {	// Want one closer in
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


	// Decide where to start walking the objects
	VuListIterator	vehicleWalker( SimDriver.combinedList );		// Vehicles
	VuListIterator	featureWalker( SimDriver.combinedFeatureList );	// Buildings
	object = (FalconEntity*)featureWalker.GetFirst();
	if (object) {
		walker = &featureWalker;
	} else {
		walker = &vehicleWalker;
		object = (FalconEntity*)vehicleWalker.GetFirst();
	}

	// Consider each potential target in our environment
	while (object) {

		// Skip air objects in AG mode
		if ( !object->OnGround() )
		{
			goto NextObject;
		}

		// Skip sleeping sim objects
		if (object->IsSim()) 
		{
			if (!((SimBaseClass*)object)->IsAwake())
			{
				goto NextObject;
			}
		}

		// Fow now we skip missles -- might want to display them eventually...
		if (object->IsMissile() || object->IsBomb() )
		{
			goto NextObject;
		}


		// Now, we need this stuff...
		dx		= object->XPos() - platform->XPos();
		dy		= object->YPos() - platform->YPos();
		dz		= object->ZPos() - platform->ZPos();
		range	= (float)sqrt( dx*dx + dy*dy );
		cosATA	= (atx*dx + aty*dy + atz*dz) / (float)sqrt(range*range+dz*dz);
		x		= dy*scaledCosYaw - dx*scaledSinYaw;	// Rotate into heading up plan view space
		y		= dy*scaledSinYaw + dx*scaledCosYaw;	// and scale from feet into viewport space


		// Mark the thing under the cursor
		d = CursorDelta(x,y);
		if (d < cursorDelta)
		{
			cursorDelta = d;
			cursorTgtID = object->Id();
		}


		// We're done unless we need to lock something up
		if (!lockCmd)
		{
			goto NextObject;
		}


		// Skip the object if it can't be locked
		if ( (range > rangeFT) || !InAGLockZone( cosATA, x, y ) ) {
			goto NextObject;
		}


		// Handle the lock command
		switch (lockCmd) {

		  case AUTO:
			// If this is the nearest "threat" object in front of us, pick it
			if (range <= bestSoFar)
			{
				if ( TeamInfo[platform->GetTeam()]->TStance(object->GetTeam()) == War )
				{
					bestSoFar = range;
					newLock = object;
				}
			}
			break;

		  case CURSOR:
			// We've been asked to lock a specific target, so find which one...
			if (object->Id() == targetUnderCursor)
			{
				newLock = object;
			}
			break;

		  case BORE:
			// We've been asked to lock the target nearest our nose
			if (cosATA > bestSoFar)
			{
				bestSoFar = cosATA;
				newLock = object;
			}
			break;

		  case NEXT:
			if (range < bestSoFar)
			{
				if ((!lockedTarget) || (range > lockedTarget->localData->range))
				{
					if ( TeamInfo[platform->GetTeam()]->TStance(object->GetTeam()) == War )
					{
						bestSoFar = range;
						newLock = object;
					}
				}
			}
			break;

		  case PREV:
			if (range > bestSoFar)
			{
				if ((!lockedTarget) || (range < lockedTarget->localData->range))
				{
					if ( TeamInfo[platform->GetTeam()]->TStance(object->GetTeam()) == War )
					{
						bestSoFar = range;
						newLock = object;
					}
				}
			}
			break;

		  default:
			ShiWarning( "Bad lock command" );
		}

NextObject:
		// Advance to the next object for consideration
		object = (FalconEntity*)walker->GetNext();
		if ((!object) && (walker == &featureWalker)) {
			walker = &vehicleWalker;
			object = (FalconEntity*)vehicleWalker.GetFirst();
		}
	}	// End of our target list traversal loop


	// Update our locked target
	SetSensorTargetHack( newLock );


	// Update our seeker center of attention
	if (lockedTarget)
	{
		CalcRelValues(	platform, lockedTarget->BaseData(),
						&lockedTarget->localData->az, 
						&lockedTarget->localData->el, 
						&lockedTarget->localData->ata, 
						&lockedTarget->localData->ataFrom, 
						&lockedTarget->localData->droll );
		SetSeekerPos( TargetAz(platform, lockedTarget), TargetEl(platform, lockedTarget) );
	}
	else
	{
		// Compute cursor location in world coordinates, but with us at the origin
		mlTrig yawTrig;
		
		mlSinCos (&yawTrig, platform->Yaw());

		float x = (cursorY*yawTrig.cos - cursorX*yawTrig.sin) * rangeFT;
		float y = (cursorY*yawTrig.sin + cursorX*yawTrig.cos) * rangeFT;

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

	// Publish the ID of the target under the cursor
	targetUnderCursor = cursorTgtID;
}


void Radar360Class::Display(VirtualDisplay *activeDisplay)
{
	float				scaledCosYaw, scaledSinYaw;
	char				string[24];
	Falcon4EntityClassType*	classPtr;
	int tmpColor = activeDisplay->Color();


	// Prepare our rotation from world space into heading north space including feet->screen scaleing
	scaledCosYaw = (float)cos( platform->Yaw() ) * invRangeFT;
	scaledSinYaw = (float)sin( platform->Yaw() ) * invRangeFT;


	// For now we have to do this silly thing to placate the SMS display routine --
	// we really should get ride of the display/privateDisplay dicotemy.
	display = activeDisplay;
//	display->SetColor( 0x0000FF00 );


	// Quit now if we're turned off
	if (!isEmitting) {
		return;
		display->TextCenter( 0.0f, 0.0f, "RADAR OFF" );
	}


	// Label our buttons
	DrawButtons();

	// Draw range circles
	display->Circle ( 0.0f, 0.0f, 0.5f );
	display->Circle ( 0.0f, 0.0f, 0.99f );

	// Draw the radar lock sector
	if (!AWACSMode) // JB 011213
	{
		display->Line( 0.0f, 0.0f,  SIN_RADAR_CONE_ANGLE, COS_RADAR_CONE_ANGLE );
		display->Line( 0.0f, 0.0f, -SIN_RADAR_CONE_ANGLE, COS_RADAR_CONE_ANGLE );
	}

	// Note if we're in auto target mode
	if (lockCmd == AUTO)
	{
		display->TextRight( 0.90f, -0.90f, "AUTO" );
	}


	// Mark where the locked target will be (if we have one)
	if (lockedTarget)
	{
		float		dx, dy;		// World space deltas		(x north)
		float		x, y;		// Screen space coordinates (x left/right)

		dx = (lockedTarget->BaseData()->XPos() - platform->XPos());
		dy = (lockedTarget->BaseData()->YPos() - platform->YPos());
		x = dy*scaledCosYaw - dx*scaledSinYaw;
		y = dy*scaledSinYaw + dx*scaledCosYaw;
		display->Circle ( x, y, BLIP_SIZE*2.0f );
	}


	// Now draw the radar cursors
	DrawCursor();
	DrawBullseyeData();


	// Now fill in the target blips
	if (mode == AA) {
		DisplayAATargets( scaledSinYaw, scaledCosYaw );
	} else {
		DisplayAGTargets( scaledSinYaw, scaledCosYaw );
	}


	// Put the display color back to green
	display->SetColor( tmpColor );

	// Target ID (NCTR)
	if (lockedTarget)
	{
		classPtr = (Falcon4EntityClassType*)lockedTarget->BaseData()->EntityType();
		if (lockedTarget->BaseData()->IsSim() && !((SimBaseClass*)lockedTarget->BaseData())->IsExploding())
		{
			if (classPtr->dataType == DTYPE_VEHICLE) {
				sprintf (string, "%s", ((VehicleClassDataType*)(classPtr->dataPtr))->Name);
			} else if (classPtr->dataType == DTYPE_FEATURE) {
				sprintf (string, "%s", "BLDG");
			} else {
				sprintf (string, "%s", "---");
			}
		} else {
			sprintf (string, "%s", "---");
		}
		ShiAssert( strlen(string) < sizeof(string) );
		display->TextCenter(0.0F, 0.75F, string);
	}
}


void Radar360Class::DisplayAATargets( float scaledSinYaw, float scaledCosYaw )
{
	SimObjectType*		object;
	float				dx, dy, dz;		// World space deltas		(x north)
	float				x, y;			// Screen space coordinates (x left/right)
	float				range;
	char				string[24];
	UInt32				color;


	// Consider each potential target for display
	for (object = platform->targetList; object; object = object->next)
	{

		// Only consider objects in the appropriate domain (air/land)
		if ( object->BaseData()->OnGround() )
		{
			// Skip ground objects in AA mode
			continue;
		}

		// Skip bombs
		if ( object->BaseData()->IsBomb() )
		{
			continue;
		}

		// Skip ejected pilots
		if ( object->BaseData()->IsEject() )
		{
			continue;
		}

		// Skip the object if it is out of range
		// NOTE:  SCR 9/17/98 -- it appears that "range" isn't always updated before we get here.
		// we either need to make sure that it IS updated, or else not rely on it here (and in AG?)
//		if (object->localData->range > rangeFT) {
//			continue;
//		}


		// Okay, it qualifies, so convert to display units and create an altitude label
		dx = object->BaseData()->XPos() - platform->XPos();
		dy = object->BaseData()->YPos() - platform->YPos();
		dz = object->BaseData()->ZPos() - platform->ZPos();
		range = (float)sqrt( dx*dx + dy*dy );
		x = dy*scaledCosYaw - dx*scaledSinYaw;	// Rotate into heading up plan view space
		y = dy*scaledSinYaw + dx*scaledCosYaw;	// and scale from feet into viewport space
		sprintf( string, "%1.0f", -0.001f*dz );	// Covert to 1000's of feet above
		ShiAssert( strlen(string) < sizeof(string) );


		// Skip the object if it is out of range
		// HERE BECAUSE LOCAL DATA WASN'T RELIABLE
		if (range > rangeFT) {
			continue;
		}


		// Choose the appropriate target color
		if ( TeamInfo[platform->GetTeam()]->TStance(object->BaseData()->GetTeam()) == War )
		{
			color = 0x000000FF;		// Red means at war
		}
		else if ( TeamInfo[platform->GetTeam()]->TStance(object->BaseData()->GetTeam()) == Allied )
		{
			color = 0x00FF0000;		// Blue means our team
		}
		else
		{
			color = 0x0000FF00;		// Green means everyone else
		}

		// Desaturate and brighten the target under the cursor
		if (object->BaseData()->Id() == targetUnderCursor)
		{
			color |= 0x00404040;	
		}

		// Draw the target symbol
		display->SetColor( color );
		display->AdjustOriginInViewport( x, y );
		display->TextLeft( BLIP_SIZE*2.5f, 0.0f, string );
		display->AdjustRotationAboutOrigin( object->BaseData()->Yaw() - platform->Yaw() );

		if (object->BaseData()->IsMissile()) {
			// Missile are just oriented lines
			display->Line( 0.0f, -BLIP_SIZE, 0.0f, BLIP_SIZE );
		} else {
			// Aircraft are oriented triangles
			display->Tri( -BLIP_SIZE, -BLIP_SIZE*1.2f, 0.0f, BLIP_SIZE*1.2f, BLIP_SIZE, -BLIP_SIZE*1.2f );
		}

		display->ZeroRotationAboutOrigin();
		display->CenterOriginInViewport();
	}
}


void Radar360Class::DisplayAGTargets( float scaledSinYaw, float scaledCosYaw){
	VuListIterator		*walker;
	FalconEntity*		object;
	float				dx, dy, dz;		// World space deltas		(x north)
	float				x, y;			// Screen space coordinates (x left/right)
	float				blipSize;
	UInt32				color;


	// Decide where to start walking the objects
	VuListIterator		featureWalker( SimDriver.combinedFeatureList );	// Buildings
	VuListIterator		vehicleWalker( SimDriver.combinedList );		// Vehicles
	object = (FalconEntity*)featureWalker.GetFirst();
	if (object) {
		walker = &featureWalker;
		blipSize = BLIP_SIZE;
	} else {
		walker = &vehicleWalker;
		blipSize = BLIP_SIZE / 2.0f;
		object = (FalconEntity*)vehicleWalker.GetFirst();
	}

	// Consider each potential target in our environment
	while (object) {

		// Skip air objects in AG mode
		if ( !object->OnGround() )
		{
			goto NextObject;
		}

		// Skip sleeping sim objects
		if (object->IsSim()) 
		{
			if (!((SimBaseClass*)object)->IsAwake())
			{
				goto NextObject;
			}
		}


		// Okay, it qualifies, so convert to display units
		dx = object->XPos() - platform->XPos();
		dy = object->YPos() - platform->YPos();
		dz = object->ZPos() - platform->ZPos();
		x = dy*scaledCosYaw - dx*scaledSinYaw;	// Rotate into heading up plan view space
		y = dy*scaledSinYaw + dx*scaledCosYaw;	// and scale from feet into viewport space

		// Skip if the object is off screen
		if ((fabs(x) > 1.0f) || (fabs(y) > 1.0f)) {
			goto NextObject;
		}

		// Choose the target's color based on their stance toward us
		switch(TeamInfo[platform->GetTeam()]->TStance(object->GetTeam())) {
		  case War:
			color = 0x000000FF;		// Red means at war
			break;
		  case Allied:
			color = 0x00FF0000;		// Blue means our team
			break;
		  default:
			color = 0x0000FF00;		// Green means everyone else
			break;
		}
		
		// Desaturate and brighten the target under the cursor
		if (object->Id() == targetUnderCursor)
		{
			color |= 0x00404040;	
		}

		// Draw the target symbol
		display->SetColor( color );
		display->AdjustOriginInViewport( x, y );
		display->Tri( -blipSize, -blipSize, blipSize, blipSize, blipSize, -blipSize );
		display->Tri( -blipSize, -blipSize, blipSize, blipSize, -blipSize, blipSize );
		display->CenterOriginInViewport();

NextObject:
		// Advance to the next object for consideration
		object = (FalconEntity*)walker->GetNext();
		if ((!object) && (walker == &featureWalker)) {
			walker = &vehicleWalker;
			blipSize = BLIP_SIZE / 2.0f;
			object = (FalconEntity*)vehicleWalker.GetFirst();
		}

	}	// End of our target list traversal loop
}


BOOL Radar360Class::InAALockZone( SimObjectType *object, float x, float y )
{
	// Take a quick path if the object is in front of the aircraft
	if (object->localData->ata < RADAR_CONE_ANGLE)
		return TRUE;

	// See if the object is within the God's eye pie slice about our heading
	if ( (y > 0.0f) && (fabs(x) < y*TAN_RADAR_CONE_ANGLE ) )
		return TRUE;

	// Not in either area, so not "lockable"
	return FALSE;
}


BOOL Radar360Class::InAGLockZone( float cosATA, float x, float y )
{
	// Take a quick path if the object is in front of the aircraft
	if (cosATA > COS_RADAR_CONE_ANGLE)
		return TRUE;

	// See if the object is within the God's eye pie slice about our heading
	if ( (y > 0.0f) && (fabs(x) < y*TAN_RADAR_CONE_ANGLE ) )
		return TRUE;

	// Not in either area, so not "lockable"
	return FALSE;
}


float Radar360Class::CursorDelta( float x, float y )
{
	return (float)max( fabs(x - cursorX), fabs(y - cursorY) );
}


void Radar360Class::DrawCursor(void)
{
	const float delta = BLIP_SIZE*2.0f;

	display->Line( cursorX-delta, cursorY-delta, cursorX-delta, cursorY+delta );
	display->Line( cursorX+delta, cursorY-delta, cursorX+delta, cursorY+delta );
}


void Radar360Class::DrawBullseyeData( void )
{
	float az, range;
	float cursX, cursY;
	char str[12];
   float bullseyeX, bullseyeY;
   mlTrig yawTrig;

   TheCampaign.GetBullseyeSimLocation (&bullseyeX, &bullseyeY);

   mlSinCos(&yawTrig, platform->Yaw());

	// Compute cursor location in world space
	cursX = (cursorY*yawTrig.cos - cursorX*yawTrig.sin) * rangeFT;
	cursY = (cursorY*yawTrig.sin + cursorX*yawTrig.cos) * rangeFT;
	cursX += platform->XPos();
	cursY += platform->YPos();

	// Compute azmuth and range from bullseye point
	az = RTD * (float)atan2 (cursY - bullseyeY, cursX - bullseyeX);
	range = (float)sqrt( (cursX-bullseyeX)*(cursX-bullseyeX) + (cursY-bullseyeY)*(cursY-bullseyeY) );
	if (az < -0.6f)
		az += 360.0f;

	// Present the data on the display
	sprintf( str, "%03.0f %02.0f", az, range*FT_TO_NM );
	ShiAssert (strlen (str) < sizeof(str));
	display->TextLeft( -0.95F, -0.75F, str );
	display->TextLeft( -0.95f, -0.65f, "BULLSEYE");
}


void Radar360Class::DrawButtons( void )
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
	LabelButton (3, "AA", "", mode == AA);

   LabelButton (4, "AG", "", mode == GM);
	LabelButton (13, "FCR", NULL, 1);
	LabelButton (14, "SWAP", "");
}


void Radar360Class::PushButton( int whichButton, int whichMFD )
{
	switch( whichButton ) {
	case 3:
		wantMode = AA;
		break;
	case 4:
		wantMode = GM;
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


void Radar360Class::NewRange( float newRange )
{
	// Round the new range to the nearest integer
	newRange = (float)floor( newRange + 0.5f );

	// Keep cursors at same range/bearing when display scale changes
//	cursorX *= ratio;
//	cursorY *= ratio;

	// Keep our various measures of range consistent
	rangeFT = newRange*NM_TO_FT;
	rangeNM = newRange;
	invRangeFT = 1.0f/rangeFT;
}


void Radar360Class::SetSRMOverride(void)
{
	prevMode = mode;
	prevRange = rangeNM;
	wantMode = AA;
	wantRange = 10.0f;
}


void Radar360Class::SetMRMOverride(void)
{
	prevMode = mode;
	prevRange = rangeNM;
	wantMode = AA;
	wantRange = 20.0f;
}


void Radar360Class::ClearOverride(void)
{
	wantMode = prevMode;
	wantRange = prevRange;
}

void Radar360Class::GetAGCenter (float *x, float *y)
{
	mlTrig yawTrig;

	mlSinCos(&yawTrig, platform->Yaw());

	// Compute cursor location in world space
	*x = (cursorY*yawTrig.cos - cursorX*yawTrig.sin) * rangeFT;
	*y = (cursorY*yawTrig.sin + cursorX*yawTrig.cos) * rangeFT;
	*x += platform->XPos();
	*y += platform->YPos();
}


void Radar360Class::SetMode (RadarMode cmd)
{
   wantMode = cmd;
}