/***************************************************************************\
	External.cpp
	Scott Randolph
	Jun 15, 1998

	Provides external camera functionality for the OTWdriver.
	It is a bit messy still, but at least its not one giant linear file
	anymore...
	\***************************************************************************/
#include "Graphics\Include\RViewPnt.h"
#include "Graphics\Include\RenderOW.h"
#include "Graphics\Include\DrawBSP.h"
#include "stdhdr.h"
#include "simbase.h"
#include "simdrive.h"
#include "otwdrive.h"
#include "simeject.h"
#include "aircrft.h"
#include "falclist.h"

float OTWDriverClass::GetDoppler (float x, float y, float z, float dx, float dy, float dz)
{ // MLR 12/1/2003 - Doppler effects are computed in the FalcSnd project
	Tpoint camPos;	// Camera position
	Tpoint camVec;	// Camera (listener) motion vector
	Tpoint dPos;	// Vector from camera to object
	float dist;
	float camClosure, objClosure;
	float closure;
	static const float APPROX_MACH	= 1000.0f;				// Ft/sec.  Use a constant mach approximation
	static const float SPEED_CLAMP	= 0.6f * APPROX_MACH;	// Ft/sec.  Limit the amount of doppler shift applied


	// This all works fine, but doesn't provide any perceivable benfit right now
	// -- Mostly because "FlyBy" mode doesn't stay in FlyBy mode long enough and
	// in nearly all other views, the sound falls off with distance too fast to
	// hear it anyway.  If any of this changes, we can let this code run.  For
	// now lets not waste cycles.  SCR 12/16/98
#if 1
	return 1.0f;
#endif


	// Construct the listener's location
	camPos.x = cameraPos.x + focusPoint.x;
	camPos.y = cameraPos.y + focusPoint.y;
	camPos.z = cameraPos.z + focusPoint.z;

	// Construct the vector from the listener to the emitter
	dPos.x = x - camPos.x;
	dPos.y = y - camPos.y;
	dPos.z = z - camPos.z;

	// Normalize the vector toward the emitter (making sure not to take the square root of zero)
	dist = dPos.x * dPos.x + dPos.y * dPos.y + dPos.z * dPos.z;
	if ( dist < 1.0f )
		return 1.0f;
	dist = (float)sqrt( dist );

	dPos.x /= dist;
	dPos.y /= dist;
	dPos.z /= dist;

	// Decide on the camera's motion vector
	if (otwPlatform.get() == NULL || GetOTWDisplayMode() == ModeFlyby )
	{
		camVec.x = 0.0f;
		camVec.y = 0.0f;
		camVec.z = 0.0f;
		camClosure = 0.0f;
	}
	else
	{
		camVec.x = otwPlatform->XDelta();
		camVec.y = otwPlatform->YDelta();
		camVec.z = otwPlatform->ZDelta();

		// Camera motion vector dotted with vector toward object
		camClosure = camVec.x * dPos.x + camVec.y * dPos.y + camVec.z * dPos.z;
	}

	// Object motion vector dotted with vector toward listener
	objClosure = -dx * dPos.x - dy * dPos.y - dz * dPos.z;

	// Sum the two closure terms
	closure = camClosure + objClosure;

	// Clamp the closure value at roughly 1/2 Mach
	if ( fabs(closure) > SPEED_CLAMP ) {
		if ( closure > 0.0f ) {
			closure = SPEED_CLAMP;
		} else {
			closure = -SPEED_CLAMP;
		}
	}

	return 1.0f + closure/APPROX_MACH;
}

#include "SimIO.h"		//	Retro 17Jan2004
#include "mouselook.h"	// Retro 18Jan2004

void OTWDriverClass::SetExternalCameraPosition (float dT)
{
	Trotation	tilt;
	float		costha, sintha, cospsi, sinpsi;
	Tpoint		dPos;
	Tpoint		newPos;
	float		dRoll;
	float		camYaw, camPitch;
	float		dist;
	float		groundHeight;
	float		platRoll;

	// Display is outside the aircraft....
	ShiAssert( !DisplayInCockpit() );

	// Retro 17Jan2004
	if ((!actionCameraMode)&&(IO.AnalogIsUsed(AXIS_ZOOM) == true))
	{
		chaseRange = -IO.GetAxisValue(AXIS_ZOOM)/15000.f * 900.f;
		if ( otwPlatform && otwPlatform->drawPointer )
			// Retro: the "-4.f" is the max displacement (3.f) of the displacementcam with a bit of a buffer ;)
			chaseRange = min (-otwPlatform->drawPointer->Radius()-4.F, chaseRange);
		else
			min (-50.0F, chaseRange);
		chaseRange = max (-900.0F, chaseRange);
	}
	// Retro ends

	if (PlayerOptions.GetMouseLook() == true)	// Retro 18Jan2004
		theMouseView.Compute(slewRate * dT);

	if (!otwPlatform) {

		cameraVel.x=cameraVel.y=cameraVel.z=0; // MLR 12/2/2003 - 

		//Retro_dead 16Jan2004		chaseAz += azDir * slewRate * dT;
		//Retro_dead 16Jan2004		chaseEl += elDir * slewRate * dT;
		// Retro 16Jan2004
		if (PlayerOptions.GetMouseLook() == false)	// Retro 18Jan2004
		{
			chaseAz += azDir * slewRate * dT;
			chaseEl += elDir * slewRate * dT;
		}
		else	// Retro 18Jan2004
		{
			chaseAz = theMouseView.GetMouseAzim();
			chaseEl = theMouseView.GetMouseElev();
		}

		// Be around ownship, looking at it
		costha = (float)cos (chaseEl);
		sintha = (float)sin (chaseEl);
		cospsi = (float)cos (chaseAz);
		sinpsi = (float)sin (chaseAz);

		tilt.M11 = cospsi*costha;
		tilt.M12 = sinpsi*costha;
		tilt.M13 = -sintha;

		PositandOrientSetData ( chaseRange * tilt.M11, chaseRange * tilt.M12, chaseRange * tilt.M13,
				chaseEl, 0.0F, chaseAz,
				&cameraPos, &cameraRot);

		return;
	}


	cameraVel.x=otwPlatform->XDelta(); // MLR 12/2/2003 - 
	cameraVel.y=otwPlatform->YDelta();
	cameraVel.z=otwPlatform->ZDelta();

	UpdateCameraFocus();

	// orbit camera
	if(GetOTWDisplayMode() == ModeOrbit)
	{
		// absolute position chase mode
		//Retro_dead 16Jan2004		chaseAz += azDir * slewRate * dT;
		//Retro_dead 16Jan2004		chaseEl += elDir * slewRate * dT;
		// Retro 16Jan2004
		if (PlayerOptions.GetMouseLook() == false)	// Retro 18Jan2004
		{
			chaseAz += azDir * slewRate * dT;
			chaseEl += elDir * slewRate * dT;
		}
		else	// Retro 18Jan2004
		{
			chaseAz = theMouseView.GetMouseAzim();
			chaseEl = theMouseView.GetMouseElev();
		}

		// Be around ownship, looking at it
		costha = (float)cos (chaseEl);
		sintha = (float)sin (chaseEl);
		cospsi = (float)cos (chaseAz);
		sinpsi = (float)sin (chaseAz);

		tilt.M11 = cospsi*costha;
		tilt.M12 = sinpsi*costha;
		tilt.M13 = -sintha;

		PositandOrientSetData ( chaseRange * tilt.M11, chaseRange * tilt.M12, chaseRange * tilt.M13,
				chaseEl, 0.0F, chaseAz,
				&cameraPos, &cameraRot);

		// Retro 25Dec2003
		if ((otwPlatform->IsAirplane())&&(displaceCamera)&&(!otwPlatform->OnGround()))
		{
			DisplaceTheCamera(dT);
		}
		// ..ends
	}

	// satellite camera
	else if(GetOTWDisplayMode() == ModeSatellite)
	{
		// absolute position chase mode
		chaseAz += azDir * slewRate * dT;
		chaseEl += elDir * slewRate * dT;

		// put limiter or range of allowable adjustable motion.
		// it's supposed to be a satellite!
		if ( chaseEl > 75.0f * DTR )
			chaseEl = 75.0f * DTR;
		else if ( chaseEl < -75.0f * DTR )
			chaseEl = -75.0f * DTR;

		costha = (float)cos (-90.0f * DTR + chaseEl);
		sintha = (float)sin (-90.0f * DTR + chaseEl);
		cospsi = (float)cos (chaseAz);
		sinpsi = (float)sin (chaseAz);

		tilt.M11 = cospsi*costha;
		tilt.M12 = sinpsi*costha;
		tilt.M13 = -sintha;

		PositandOrientSetData (20.0f * chaseRange * tilt.M11,
				20.0f * chaseRange * tilt.M12,
				20.0f * chaseRange * tilt.M13,
				chaseEl - 90.0f * DTR,
				0.0F,
				chaseAz,
				&cameraPos,
				&cameraRot);

		// PositandOrientSetData (0.0f, 0.0f, chaseRange * 20.0f,
		// 	camPitch, chaseAz, 0.0f,
		// 	&cameraPos, &cameraRot);

	}

	// handle various cases of chase camera
	else if( GetOTWDisplayMode() == ModeChase ||
			(
			 ( GetOTWDisplayMode() == ModeAirFriendly ||
				 GetOTWDisplayMode() == ModeAirEnemy ||
				 GetOTWDisplayMode() == ModeGroundEnemy ||
				 GetOTWDisplayMode() == ModeGroundFriendly ||
				 GetOTWDisplayMode() == ModeTarget ||
				 GetOTWDisplayMode() == ModeTargetToSelf ||
				 GetOTWDisplayMode() == ModeTargetToWeapon ||
				 GetOTWDisplayMode() == ModeIncoming ||
				 GetOTWDisplayMode() == ModeWeapon ) &&
			 (otwTrackPlatform.get() == NULL || otwTrackPlatform == otwPlatform)
			) )
	{

		// "spring" constants for camera roll and move
#define KMOVE			0.59f
#define KROLL			0.55f

		// allow a relative slewing when in chase mode
		chaseAz += azDir * slewRate * dT;
		chaseEl += elDir * slewRate * dT;

		// put limiter or range of allowable adjustable motion.
		// it's actually quite disorienting to be in in "chase"
		// when in front of the plane.  Plus, rolling doesn't
		// work well
		if ( chaseAz > 45.0f * DTR )
			chaseAz = 45.0f * DTR;
		else if ( chaseAz < -45.0f * DTR )
			chaseAz = -45.0f * DTR;
		if ( chaseEl > 45.0f * DTR )
			chaseEl = 45.0f * DTR;
		else if ( chaseEl < -45.0f * DTR )
			chaseEl = -45.0f * DTR;

		costha = (float)cos (chaseEl + otwPlatform->Pitch());
		sintha = (float)sin (chaseEl + otwPlatform->Pitch());
		cospsi = (float)cos (chaseAz + otwPlatform->Yaw());
		sinpsi = (float)sin (chaseAz + otwPlatform->Yaw());

		tilt.M11 = cospsi*costha;
		tilt.M12 = sinpsi*costha;
		tilt.M13 = -sintha;

		// convert frame loop time to secs from ms
		dT = (float)frameTime * 0.001F;
		// clamp dT
		if ( dT < 0.01f )
			dT = 0.01f;
		else if ( dT > 0.5f )
			dT = 0.5f;

		// get the position we want the camera to go to
		if ( GetOTWDisplayMode() == ModeWeapon  )
		{
			if ( otwPlatform->IsBomb() )
			{
				chaseCamPos.x = 0.0f;
				chaseCamPos.y = 0.0f;
				chaseCamPos.z = chaseRange * 0.5f;
			}
			else
			{
				// chaseCamPos.x = otwPlatform->dmx[0][0] * chaseRange * 0.5f;
				// chaseCamPos.y = otwPlatform->dmx[0][1] * chaseRange * 0.5f;
				// chaseCamPos.z = otwPlatform->dmx[0][2] * chaseRange * 0.5f - 10.0f;
				chaseCamPos.x = tilt.M11 * chaseRange * 0.5f;
				chaseCamPos.y = tilt.M12 * chaseRange * 0.5f;
				chaseCamPos.z = tilt.M13 * chaseRange * 0.5f - 10.0f;
			}
		}
		else
		{
			// chaseCamPos.x = otwPlatform->dmx[0][0] * chaseRange;
			// chaseCamPos.y = otwPlatform->dmx[0][1] * chaseRange;
			// chaseCamPos.z = otwPlatform->dmx[0][2] * chaseRange - 10.0f;
			chaseCamPos.x = tilt.M11 * chaseRange ;
			chaseCamPos.y = tilt.M12 * chaseRange ;
			chaseCamPos.z = tilt.M13 * chaseRange  - 10.0f;
		}

		// check ground height of platform -- if near, raise the
		// cam position
		groundHeight = viewPoint->GetGroundLevel(otwPlatform->XPos(), otwPlatform->YPos());
		if ( otwPlatform->IsWeapon() )
		{
			if ( otwPlatform->ZPos() >= groundHeight - 200.0f )
				chaseCamPos.z  = groundHeight - 800.0f;
		}
		else if ( otwPlatform->ZPos() >= groundHeight - 20.0f )
		{
			chaseCamPos.z -= 50.0f;
		}

		// get the diff between desired and current camera pos
		dPos.x = chaseCamPos.x - cameraPos.x;
		dPos.y = chaseCamPos.y - cameraPos.y;
		dPos.z = chaseCamPos.z - cameraPos.z;


		// send the camera thataway
		newPos.x = cameraPos.x + dPos.x * dT * KMOVE;
		newPos.y = cameraPos.y + dPos.y * dT * KMOVE;
		newPos.z = cameraPos.z + dPos.z * dT * KMOVE;

		// "look at" vector
		dPos.x = -newPos.x;
		dPos.y = -newPos.y;
		dPos.z = -newPos.z;

		// get new camera roll
		// we need to check the quadrant of platform vs cam roll and
		// roll in the direction of the shortest distance
		if ( endFlightTimer == 0 )
			platRoll = otwPlatform->Roll();
		else
			platRoll = 0.0f;

		if ( platRoll < -90.0f * DTR && chaseCamRoll > 90.0f * DTR )
		{
			dRoll = platRoll + (360.0f * DTR) - chaseCamRoll;
		}
		else if ( platRoll > 90.0f * DTR && chaseCamRoll < -90.0f * DTR )
		{
			dRoll = platRoll - (360.0f * DTR) - chaseCamRoll;
		}
		else // same sign
		{
			dRoll = platRoll - chaseCamRoll;
		}

		// apply roll
		chaseCamRoll += dRoll * dT * KROLL;

		// keep chase cam roll with +/- 180
		if ( chaseCamRoll > 180.0f * DTR )
			chaseCamRoll -= 360.0f * DTR;
		else if ( chaseCamRoll < -180.0f * DTR )
			chaseCamRoll += 360.0f * DTR;

		// now get yaw and pitch based on look at vector
		dist = (float)sqrt( dPos.x * dPos.x + dPos.y * dPos.y + dPos.z * dPos.z );
		camPitch	= (float)-asin( dPos.z/dist );
		camYaw		= (float)atan2( dPos.y, dPos.x );

		PositandOrientSetData ( newPos.x, newPos.y, newPos.z,
				camPitch, chaseCamRoll, camYaw,
				&cameraPos, &cameraRot);


	}

	// various cameras (with tracked object)
	else if (GetOTWDisplayMode() == ModeWeapon ||
			GetOTWDisplayMode() == ModeAirFriendly ||
			GetOTWDisplayMode() == ModeAirEnemy ||
			GetOTWDisplayMode() == ModeTarget ||
			GetOTWDisplayMode() == ModeTargetToSelf ||
			GetOTWDisplayMode() == ModeTargetToWeapon ||
			GetOTWDisplayMode() == ModeIncoming ||
			GetOTWDisplayMode() == ModeGroundEnemy ||
			GetOTWDisplayMode() == ModeGroundFriendly )
	{
		// get look-at vector to tracked object
		dPos.x = otwTrackPlatform->XPos() - otwPlatform->XPos();
		dPos.y = otwTrackPlatform->YPos() - otwPlatform->YPos();
		dPos.z = otwTrackPlatform->ZPos() - otwPlatform->ZPos();

		dist = (float)sqrt( dPos.x * dPos.x + dPos.y * dPos.y + dPos.z * dPos.z );

		if(dist == 0.0F)
			dist = 1.0F;

		// place new position along the line of the look-at vector
		// with an upward adjustment
		if (GetOTWDisplayMode() == ModeWeapon )
		{
			newPos.x = (dPos.x/dist) * chaseRange * 0.5f;
			newPos.y = (dPos.y/dist) * chaseRange * 0.5f;
			newPos.z = (dPos.z/dist) * chaseRange * 0.5f - 5.0f;
		}
		else
		{
			if ( otwPlatform->IsGroundVehicle() )
			{
				newPos.x = (dPos.x/dist) * chaseRange * 0.5f;
				newPos.y = (dPos.y/dist) * chaseRange * 0.5f;
				newPos.z = GetGroundLevel( otwPlatform->XPos() + newPos.x ,
						otwPlatform->YPos() + newPos.y );
				newPos.z -= ( otwPlatform->ZPos() + 5.0f );
			}
			else
			{
				newPos.x = (dPos.x/dist) * chaseRange;
				newPos.y = (dPos.y/dist) * chaseRange;
				newPos.z = (dPos.z/dist) * chaseRange - 25.0f;
			}
		}

		// now we need to get the new look-at vector
		dPos.x = otwTrackPlatform->XPos() - (otwPlatform->XPos() + newPos.x);
		dPos.y = otwTrackPlatform->YPos() - (otwPlatform->YPos() + newPos.y);
		dPos.z = otwTrackPlatform->ZPos() - (otwPlatform->ZPos() + newPos.z);

		// now get yaw and pitch based on look at vector
		dist = (float)sqrt( dPos.x * dPos.x + dPos.y * dPos.y + dPos.z * dPos.z );
		if(dist == 0.0F)
			dist = 1.0F;
		camPitch	= (float)-asin( dPos.z/dist );
		camYaw		= (float)atan2( dPos.y, dPos.x );

		// set camera position and matrix
		PositandOrientSetData ( newPos.x, newPos.y, newPos.z,
				camPitch, 0.0f, camYaw,
				&cameraPos, &cameraRot);
	}
	else if(GetOTWDisplayMode() == ModeFlyby)
	{
		SetFlybyCameraPosition(dT);
	}
	else
	{
		// (If we never hit this assert, then the "else if" above could/should
		// become a simple "else".
		// ShiAssert( !"Illegal view mode.  Shouldn't get here.  SCR." );
	}
}
#include "profiler.h"
#include "fmath.h"			// needed for Jam´s new cos/sin/etc assembly stuff
#pragma warning (push, 4)
extern int targetCompressionRatio;
/*****************************************************************************/
// Retro 23Dec2003
//	This function moves the camera slighly to simulate the relative movement of
//	an imaginary formation-flying airplane that holds the camera in order to
//	heighten immersion (or such).
//	THIS function IMPLICITLY changes the cameraPos !!!	
/*****************************************************************************/
void OTWDriverClass::DisplaceTheCamera(float dT)
{
	Prof(DisplaceTheCamera);

	if ((!xDir)||(!yDir)||(!zDir))
		return;

	// only compute any additional displacement when NOT paused..
	if (targetCompressionRatio && SimDriver.MotionOn())	// Retro 29Feb2004 - hope that takes care of 'FREEZE' state too
	{
		// have to keep dT in check.. too long (or too short ?) values mess up the movement of the cam..
		// dT is in seconds.. be sure..
		if (dT > 1)
			dT = 1;

#define NUM_OF_DISPLACEMENT_AXIS 3
		CamDisplacement* dir[NUM_OF_DISPLACEMENT_AXIS] = { xDir, yDir, zDir };
		const float thisFramewobbleDistance = cameraDisplacementRate * dT;

		for (int i = 0; i < NUM_OF_DISPLACEMENT_AXIS; i++)
		{
			/*****************************************************************************/
			// muhahaha... errm..
			// 'scale' scales the movement speed of the cam, it should get slower as it
			// nears its maximum displacement.. for this, I elected to use a cos() function
			// as cos(90°) is 0 while cos(0°) is 1. So, if our displacement is equal to our
			// max displacement the cos returns 0 (cam moves very slow),  and if our
			// displacement is very small compared to the max displacement I get 1
			// (cam moves normally)
			// the 'offset' at the beginning makes sure that the cam NEVER ceases moving as
			// that would stop our displacement algorighm, and that would be a shame, no ?
			//
			// problem: hmm if maxDispl isn´t very big to begin with then the ratio
			// (displ/maxDispl) may not generate sufficient big (or small) changes to the
			// scale in order for the cam not to slow down enough as it nears the 'edges'
			// to be perceived as a 'smooth' direction change.. hmm
			/*****************************************************************************/
			float thisFrameScale = 0.1F + (float)FabsF((dir[i])->scale * Cos(dir[i]->Displ/dir[i]->maxDispl*HALF_PI));
			(dir[i])->Displ += (thisFramewobbleDistance * thisFrameScale * dir[i]->direction);
			if (FabsF((dir[i])->Displ) > FabsF((dir[i])->maxDispl))	// bumped into a limit.. limits are symmetric (duh!)
			{
				// so now we select a new cam-direction and speed (randomly)
				ReInitDisplacement(dir[i]);
			}
		}
	}

	// this here chunk is also done when paused, so that we don´t snap around when pressing 'p'
	// (== can stays displaced)
	cameraPos.x += xDir->Displ;
	cameraPos.y += yDir->Displ;
	cameraPos.z += zDir->Displ;
}
#ifdef USE_WING_SPAN	// doesn´t work properly now.
#include "Sim\Include\Airframe.h"	// needed for wing span
#endif
/*****************************************************************************/
// Retro 23Dec2003
// computes new speed and direction for the camera movement.
// Can be totally random (and nondeterministic) in principle..
/*****************************************************************************/
void OTWDriverClass::ReInitDisplacement(CamDisplacement* theDisp)
{
	ShiAssert(theDisp);

#ifdef USE_WING_SPAN
	// might want to take this as max displacement ?	
	// acft wingspan:
	float theSpan = ((AircraftClass*)otwPlatform)->af->GetAeroData(AeroDataSet::Span)/2;
	if (theSpan < 5)
		theSpan = 5;

	if (theDisp->direction > 1.F)
		theDisp->maxDispl = theSpan;
	else
		theDisp->maxDispl = -theSpan;

#else
	theDisp->Displ = theDisp->maxDispl;
#endif
	theDisp->direction *= -1.F;
	theDisp->maxDispl *= -1.F;

	theDisp->scale = 0.1F + ((float)(rand()%4))/4.F;

}

void OTWDriverClass::toggleDisplaceCamera(void)
{
	displaceCamera = !displaceCamera;
}
#pragma warning(pop)
// Retro 23Dec2003 end

void OTWDriverClass::SetFlybyCameraPosition (float dT)
{
	Tpoint	dPos;
	float	camYaw, camPitch;
	float	dist;
	float	groundHeight;


	// if we're in end flight cinematics, use otwplatform as focus point.
	// otherwise keep same focus point as last available and keep moving
	// the endflightpoint upwards

	if ( otwPlatform )
	{
		UpdateCameraFocus();
	}

	// convert frame loop time to secs from ms
	dT = (float)frameTime * 0.001F;
	// clamp dT
	if ( dT < 0.01f )
		dT = 0.01f;
	else if ( dT > 0.5f )
		dT = 0.5f;
	endFlightPoint.x += endFlightVec.x * 40.0f * dT;
	endFlightPoint.y += endFlightVec.y * 40.0f * dT;
	endFlightPoint.z += endFlightVec.z * 40.0f * dT;

	cameraVel.x = endFlightVec.x * 40.f;  // MLR 12/1/2003 - Set camera velocity
	cameraVel.y = endFlightVec.y * 40.f;
	cameraVel.z = endFlightVec.z * 40.f;

	// make sure camera never goes below the ground
	groundHeight = GetApproxGroundLevel(endFlightPoint.x, endFlightPoint.y);
	if ( endFlightPoint.z - groundHeight > -100.0f )
	{
		groundHeight = viewPoint->GetGroundLevel(endFlightPoint.x, endFlightPoint.y);
		if ( endFlightPoint.z >= groundHeight )
			endFlightPoint.z = groundHeight - 5.0f;
	}

	cameraPos.x = endFlightPoint.x - focusPoint.x;
	cameraPos.y = endFlightPoint.y - focusPoint.y;
	cameraPos.z = endFlightPoint.z - focusPoint.z;

	// "look at" vector
	dPos.x = -cameraPos.x;
	dPos.y = -cameraPos.y;
	dPos.z = -cameraPos.z;

	// now get yaw and pitch based on look at vector
	dist = (float)sqrt( dPos.x * dPos.x + dPos.y * dPos.y + dPos.z * dPos.z );
	camPitch	= (float)-asin( dPos.z/dist );
	camYaw		= (float)atan2( dPos.y, dPos.x );

	PositandOrientSetData ( cameraPos.x, cameraPos.y, cameraPos.z,
			camPitch, 0.0f, camYaw,
			&cameraPos, &cameraRot);
}


void OTWDriverClass::BuildExternalNearList(void)
{
	SimBaseClass	*theObject;
	Tpoint			wsPos;
	float			referenceDepth;
	float			dx, dy, dz;
	float			depth;


	// We rebuild the near list each frame...
	FlushNearList();
	// Drop out if we have nothing to do...
	if (!SimDriver.objectList || DisplayInCockpit() || !otwPlatform || otwPlatform->OnGround() || !otwPlatform->drawPointer)
		return;

	// Compute the real world camera position
	wsPos.x = focusPoint.x + cameraPos.x;
	wsPos.y = focusPoint.y + cameraPos.y;
	wsPos.z = focusPoint.z + cameraPos.z;

	// Figure out the image space depth of the object of attention
	dx		= ownshipPos.x - wsPos.x;
	dy		= ownshipPos.y - wsPos.y;
	dz		= ownshipPos.z - wsPos.z;
	referenceDepth	= (float)fabs( dx*cameraRot.M11 + dy*cameraRot.M21 + dz*cameraRot.M31 );


	VuListIterator otwDrawWalker (SimDriver.objectList);
	// Consider each (sim) vehicle
	for (
		theObject = (SimBaseClass*)otwDrawWalker.GetFirst();
		theObject; 
		theObject = (SimBaseClass*)otwDrawWalker.GetNext()
	){
		// Skip things on the groud, without draw pointers, hidden, or exploding
		if (theObject->OnGround()					|| 
				theObject->IsSetLocalFlag( IS_HIDDEN )	|| 
				!theObject->drawPointer					|| 
				theObject->IsExploding()				||
				otwPlatform.get() == theObject) {
			continue;
		}

		// Compute its scene depth
		dx		= theObject->drawPointer->X() - wsPos.x;
		dy		= theObject->drawPointer->Y() - wsPos.y;
		dz		= theObject->drawPointer->Z() - wsPos.z;
		depth	= dx*cameraRot.M11 + dy*cameraRot.M21 + dz*cameraRot.M31;
		if (fabs( depth ) < referenceDepth)
		{
			// Put it into the "near" list for special treatment
			AddToNearList( theObject->drawPointer, depth );					
		}
	}
}

//JAM - FIXME
void OTWDriverClass::DrawExternalViewTarget (void)
{
	Tpoint			worldPosition;
	Tpoint			viewPos;
	DrawableBSP		*drawable;
	Tpoint			objOrigin = Origin;
	float 			groundZ, bottom, top;
	drawPtrList		*entry;
	static int		LODlie = 0;				// A way to force an object to update its terrain data every frame.

	// Make sure we don't get in here when we shouldn't.
	if ( otwPlatform.get() == NULL || otwPlatform->drawPointer == NULL ){
		return;
	}

	// Get our cast pointer
	drawable = (DrawableBSP*)otwPlatform->drawPointer;

	// RED - check for visibility stuff
	if(drawable->SetupVisibility( renderer )){

		// Save the object's real world space position
		drawable->GetPosition( &worldPosition );

		// Save the camera's real world space position
		viewPos.x = focusPoint.x + cameraPos.x;
		viewPos.y = focusPoint.y + cameraPos.y;
		viewPos.z = focusPoint.z + cameraPos.z;

		// NOTE:  The camera position is clamped at 5 feet above ground, so
		// if it would have broken that, it will not be pointing at the object
		// of interest.  In this case we shift the object appropriatly to keep
		// it anchored in the world.
		GetAreaFloorAndCeiling(&bottom, &top);
		if (viewPos.z > top)
		{
			groundZ = min( viewPos.z, GetGroundLevel(viewPos.x, viewPos.y) - 5.0F );
			objOrigin.z = viewPos.z - groundZ;
		}

		// Setup the local coordinate system.
		// This is world space orientation, but with the focusPoint (usually the object) at the origin.
		renderer->SetCamera( &cameraPos, &cameraRot );
		drawable->Update( &objOrigin, &ownshipRot );

		// Draw the object of interest
		drawable->Draw( renderer );

		// Put the object back into world space
		drawable->Update( &worldPosition, &ownshipRot );

		// restore camera
		renderer->SetCamera( &viewPos, &cameraRot );

	}
	// remove the inhibit
	otwPlatform->drawPointer->SetInhibitFlag( FALSE );


	// Now deal with the objects which were inhibited because they were closer than the object of interest we just drew
	while (nearObjectRoot)
	{
		// Get the first entry
		entry = nearObjectRoot;

		// Turn the object back on
		entry->drawPointer->SetInhibitFlag( FALSE );

		// Draw it
		entry->drawPointer->Draw( renderer, LODlie );

		// Remove it from the list
		nearObjectRoot = nearObjectRoot->next;
		delete entry;
	}
}


void OTWDriverClass::StartEjectCam(EjectedPilotClass *ejectedPilot, int startChaseMode)
{
	// F4Assert(ejectCam == 0);
	F4Assert(ejectedPilot != NULL);

	// Set ownship to the ejected pilot
	lastotwPlatform = otwPlatform;
	if ( lastotwPlatform ){
		InsertObjectIntoDrawList(lastotwPlatform.get());
	}
	SetGraphicsOwnship((SimBaseClass*)ejectedPilot);

	// edg note: I don't think we need eject cam anymore
	ejectCam = 1;

	// Set us to the cockpit view
	SetEjectCamChaseMode(ejectedPilot, startChaseMode);
}


void OTWDriverClass::SetEjectCamChaseMode(EjectedPilotClass *ejectedPilot, int chaseMode)
{
	SimBaseClass *airCraft;

	if(otwPlatform.get() == ejectedPilot)
	{
		airCraft = ejectedPilot->GetParentAircraft();
		/*
			 SetOTWDisplayMode(ModeOrbit);
		 ** edg note: I don't particularly think starting out in 1st person
		 ** and then going to 3rd person is so great.  We'll just do 3rd
		 ** person in orbit mode.
		 */

		// saveCameraPos = cameraPos;

		if(chaseMode == 0)
		{
			SetOTWDisplayMode(ModeFlyby);
			flybyTimer = SimLibElapsedTime + 5000;

			if ( airCraft )
			{
				endFlightPoint.x = airCraft->XPos() - airCraft->dmx[2][0] * 500.0f + airCraft->XDelta() * 1.0f;
				endFlightPoint.y = airCraft->YPos() - airCraft->dmx[2][1] * 500.0f + airCraft->YDelta() * 1.0f;
				endFlightPoint.z = airCraft->ZPos() - airCraft->dmx[2][2] * 500.0f + airCraft->ZDelta() * 1.0f;
			}
			else
			{
				endFlightPoint.x = otwPlatform->XPos() + otwPlatform->XDelta() * 2.0f;
				endFlightPoint.y = otwPlatform->YPos() + otwPlatform->YDelta() * 2.0f;
				endFlightPoint.z = otwPlatform->ZPos() + otwPlatform->ZDelta() * 2.0f;
			}
		}
		/*
			 else if(chaseMode == 1) {
			 SetOTWDisplayMode(ModeChase);
			 }
			 else if(chaseMode == 2) {
			 SetOTWDisplayMode(ModeOrbit);
			 }
			 else {
		// F4Assert(0);
		SetOTWDisplayMode(ModeChase);
		}
		 */

		// cameraPos = saveCameraPos;
	}
}

