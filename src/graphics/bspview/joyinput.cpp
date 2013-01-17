/***************************************************************************\
	JoyInput.cpp
	Scott Randolph
	February 18, 1998

	Provide interaction through a joystick.
\***************************************************************************/
#include <stdio.h>
#include <math.h>
#include "JoyInput.h"


#define SIM_SPEED		1000.0f		// Feet per second
#define SIM_ANG_RATE	2.0f		// Radians per second
#define	HEAD_ANG_RATE	1.0f		// Radians per second
#define	IIR_RATE		0.5f		// Percent new value added in (1.0 = no filter)
#define PI				3.14159265359f


JoyInputClass	TheJoystick;



/***************************************************************************\
	Detect the joystick and initialize our internal structures.
\***************************************************************************/
void JoyInputClass::Setup( void )
{   
	ShiAssert( !IsReady() );
	
	// Initialize our outputs to default values
	throttleIIR		= 1.0f;
	throttle		= 0.003f;
	pitchRate		= 0.0f;
	yawRate			= 0.0f;
	rollRate		= 0.0f;
	deltaMatrix		= IMatrix;
	headPitchRate	= 0.0f;
	headYawRate		= 0.0f;
	headDeltaMatrix	= IMatrix;
	buttons			= 0;

	// Ask for the joystick properties
	DWORD ret = joyGetDevCaps( JOYSTICKID1,	&joyCaps, sizeof(joyCaps) );
	if (ret != JOYERR_NOERROR) {
		haveJoystick = FALSE;
	} else {
		haveJoystick = TRUE;
	}

	// Initialize the joystick information structure for use in polling
	memset( &joyInfoEx, 0, sizeof(joyInfoEx) );
	joyInfoEx.dwSize = sizeof( joyInfoEx );
	joyInfoEx.dwFlags = JOY_RETURNALL;

	lastTime = 0;

	ready = TRUE;
}



/***************************************************************************\
	Clean up when the simulation loop is no longer needed.
\***************************************************************************/
void JoyInputClass::Cleanup( void )
{ 
	ShiAssert( IsReady() );

	ready = FALSE;
	return;
}



/***************************************************************************\
	Do the computations for one time step in the simulation.
\***************************************************************************/
void JoyInputClass::Update( DWORD time )
{
	float deltaTime		= 0.0f;


	ShiAssert( IsReady() );

	// Determine how long its been since the last sim update
	if ( time > lastTime ) {
		deltaTime = (float)(time - lastTime)/1000.0f;
	}
	lastTime = time;


	// Quit now if we don't have a joystick
	if (!haveJoystick) {
		return;
	}

	
	// Update our joystick information
	joyGetPosEx(JOYSTICKID1, &joyInfoEx);
	buttons = joyInfoEx.dwButtons;


	// Based on the joystick position, set the roll and pitch angles
	// Positive pitch is upward
	pitchRate = ((int)joyInfoEx.dwYpos-32768) / 32768.0f * SIM_ANG_RATE * deltaTime;

	// Positive roll is "right" roll (ie: clockwise)
	rollRate  = ((int)joyInfoEx.dwXpos-32768) / 32768.0f * SIM_ANG_RATE * deltaTime;

	// Positive yaw is "right"
	if (joyCaps.wCaps & JOYCAPS_HASR) {
		yawRate  = ((int)joyInfoEx.dwRpos-32768) / 32768.0f * SIM_ANG_RATE * deltaTime;
	}

	// Positive Z is decrease in throttle
	if (joyCaps.wCaps & JOYCAPS_HASZ) {
#if 0
		throttle  = (65535-(int)joyInfoEx.dwZpos) / 65535.0f;
#else
		throttleIIR *= 1.0f - IIR_RATE;
		throttleIIR += IIR_RATE * (65535-(int)joyInfoEx.dwZpos) / 65535.0f;
		throttle = throttleIIR;
#endif
		throttle  = throttle * throttle * throttle;	// Use a polynomial curve to get better low end response
	}

	// Update the viewing rotation
	ConstructDeltaMatrix( pitchRate, rollRate, yawRate, &deltaMatrix );


	// Update the view direction if the joystick provides POV information
	if (joyCaps.wCaps & JOYCAPS_HASPOV) {
		if (joyInfoEx.dwPOV == JOY_POVCENTERED) {
			headPitchRate	= 0.0f;
			headYawRate		= 0.0f;
		} else {
			// Extract the head motion from the POV data
			float controlAngle = joyInfoEx.dwPOV * 0.01f * PI / 180.0f;
			headPitchRate = HEAD_ANG_RATE * cos( controlAngle ) * deltaTime;
			headYawRate = HEAD_ANG_RATE * sin( controlAngle ) * deltaTime;
		}

		ConstructDeltaMatrix( headPitchRate, 0.0f, headYawRate, &headDeltaMatrix );
	}
}



/***************************************************************************\
	Update the rotation matrix to account for the user's control inputs.
	NOTE:  This is an approximation and will gimbal lock as well...
\***************************************************************************/
void JoyInputClass::ConstructDeltaMatrix( float p, float r, float y, Trotation *T )
{                    
	Tpoint	at, up, rt;
	float	mag;

	// TODO:  Add in roll component.

	// at is a point on the unit sphere
	at.x = cos(y) * cos(p);
	at.y = sin(y) * cos(p);
	at.z = -sin(p);

	// (0, 0, 1) rolled and crossed with at [normalized]
	up.x = 0.0f;
	up.y = -sin(r);
	up.z = cos(r);
	rt.x = up.y * at.z - up.z * at.y;
	rt.y = up.z * at.x - up.x * at.z;
	rt.z = up.x * at.y - up.y * at.x;
	mag = sqrt(rt.x * rt.x + rt.y * rt.y + rt.z * rt.z);
	rt.x /= mag;
	rt.y /= mag;
	rt.z /= mag;

	// at cross rt [normalized]
	up.x = at.y * rt.z - at.z * rt.y;
	up.y = at.z * rt.x - at.x * rt.z;
	up.z = at.x * rt.y - at.y * rt.x;
	mag = sqrt(up.x * up.x + up.y * up.y + up.z * up.z);
	up.x /= mag;
	up.y /= mag;
	up.z /= mag;

	// Load the matrix
	T->M11 = at.x;	T->M12 = rt.x;	T->M13 = up.x;
	T->M21 = at.y;	T->M22 = rt.y;	T->M23 = up.y;
	T->M31 = at.z;	T->M32 = rt.z;	T->M33 = up.z;
}
