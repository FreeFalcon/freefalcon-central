/***************************************************************************\
	sim.c
	Scott Randolph
	November 25, 1994

	Provide the simulations calcuations and interaction with the user.
\***************************************************************************/
#include <stdio.h>
#include <math.h>
#include "Sim.h"


#define SIM_SPEED		10000.0f	// Feet per second
#define SIM_ANG_RATE	2.0f		// Radians per second
#define	HEAD_ANG_RATE	1.0f		// Radians per second
#define PI				3.14159265359f


/***************************************************************************\
	Prepare the simulation for running by intialize required state and other
	modules.
\***************************************************************************/
void SimClass::Setup( void )
{   
	ShiAssert( !IsReady() );
	
	// Initialize the viewing rotation matrix
	rotation = IMatrix;
	headMatrix = IMatrix;

	// Put the head at its rest point
	headPitch	= 0.0f;
	headYaw		= 0.0f;
    headPos.x = 0.0f;
    headPos.y = 0.0f;
    headPos.z = 0.0f;

	// Initialize the quaternians which control viewer orientation
    e1 = 1.0f;
	e2 = 0.0f;
    e3 = 0.0f;
    e4 = 0.0f;
     
    // Start at a reasonable default location
    position.x = 0.0f;
    position.y = 0.0f;
    position.z = 0.0f;

	// Set our initial rates to zero
	pitchRate	= 0.0f;
	rollRate	= 0.0f;
	yawRate		= 0.0f;
	speed		= 0.0f;

	// Ask for the joystick properties
	UInt32 ret = joyGetDevCaps( JOYSTICKID1,	&joyCaps, sizeof(joyCaps) );
	if (ret != JOYERR_NOERROR) {
		haveJoystick = FALSE;
	} else {
		haveJoystick = TRUE;
	}

	// Initialize the joystick information structure for use in polling
	joyInfoEx.dwSize = sizeof( joyInfoEx );
	joyInfoEx.dwFlags = JOY_RETURNALL;

	lastTime = 0;

	ready = TRUE;
}



/***************************************************************************\
	Clean up when the simulation loop is no longer needed.
\***************************************************************************/
void SimClass::Cleanup( void )
{ 
	ShiAssert( IsReady() );

	ready = FALSE;
	return;
}



/***************************************************************************\
	Do the computations for one time step in the simulation.  Call the
	various other modules to get input and generate output.  Return TRUE
	until the end condition for the simulation is detected.
\***************************************************************************/
UInt32 SimClass::Update( UInt32 time )
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
		return TRUE;
	}

	
	// Update our joystick information
	joyGetPosEx(JOYSTICKID1, &joyInfoEx);
	buttons = joyInfoEx.dwButtons;


#if 1
	// Based on the joystick position, set the roll and pitch angles
	// Positive pitch is upward
	pitchRate = ((int)joyInfoEx.dwYpos-32768) / 32768.0f * SIM_ANG_RATE;
#endif
#if 1
	// Positive roll is "right" roll (ie: clockwise)
	rollRate  = ((int)joyInfoEx.dwXpos-32768) / 32768.0f * SIM_ANG_RATE;
#endif
#if 1
	// Positive yaw is "right"
	yawRate  = ((int)joyInfoEx.dwRpos-32768) / 32768.0f * SIM_ANG_RATE;
	if (!(joyCaps.wCaps & JOYCAPS_HASR)) {
		yawRate = 0.0f;				// Default to no yaw rate
	}
#endif
#if 1
	// Positive Z is decrease in throttle
	speed  = (65535-(int)joyInfoEx.dwZpos) / 65535.0f;
	speed  = speed * speed * speed;	// Use a polynomial curve to get better low end response
	speed  *= SIM_SPEED;			// Scale from 0:1 to 0:SIM_SPEED
	if (!(joyCaps.wCaps & JOYCAPS_HASZ)) {
		speed = SIM_SPEED * 0.25f;	// Default to 1/4 max speed
	}
#endif

	// If button 2 is pressed, reverse direction (go backward)
	if ( buttons & JOY_BUTTON2 ) {
		// Move backward
		speed = -speed;
	}

    // Don't move at all unless joystick button 1 is pushed.
    if ( buttons & JOY_BUTTON1 ) {               

	    // Update the viewing rotation
		UpdateRotation( rollRate*deltaTime, pitchRate*deltaTime, yawRate*deltaTime );


		// Move along the DOV
		// Compute the sample space components of the DOV vector (along the x axis)
		// using the inverse of the viewing rotation (for rotation matricies, 
		// inverse = transpose)
		position.x += speed * deltaTime * rotation.M11;
		position.y += speed * deltaTime * rotation.M21;
		position.z += speed * deltaTime * rotation.M31;
	}

	// Update the view direction if the joystick provides POV information
	if ((joyCaps.wCaps & JOYCAPS_HASPOV) && (joyInfoEx.dwPOV != JOY_POVCENTERED)) {

		// Extract the head motion from the POV data
		float controlAngle = joyInfoEx.dwPOV * 0.01f * PI / 180.0f;
		headPitchRate = HEAD_ANG_RATE * cos( controlAngle );
		headYawRate = HEAD_ANG_RATE * sin( controlAngle );

		// Update the head angles 
		headYaw += headYawRate * deltaTime;
		if (headYaw > 2.0f * PI) {
			headYaw -= 2.0f * PI;
		}
		headPitch += headPitchRate * deltaTime;
		if (headPitch > 2.0f * PI) {
			headPitch -= 2.0f * PI;
		}

		// Construct the head matrix
		Tpoint	at, up, rt;
		float	mag;

		// at is a point on the unit sphere
		at.x = cos(headYaw) * cos(headPitch);
		at.y = sin(headYaw) * cos(headPitch);
		at.z = sin(headPitch);

		// (0, 0, 1) cross at [normalized]
		mag = sqrt(at.x * at.x + at.y * at.y);
		rt.x = -at.y / mag;
		rt.y = at.x / mag;
		rt.z = 0.0f;

		// at cross rt [normalized]
		up.x = at.y * rt.z - at.z * rt.y;
		up.y = at.z * rt.x - at.x * rt.z;
		up.z = at.x * rt.y - at.y * rt.x;
		mag = sqrt(up.x * up.x + up.y * up.y + up.z * up.z);
		up.x /= mag;
		up.y /= mag;
		up.z /= mag;

		// Load the matrix
		headMatrix.M11 = at.x;	headMatrix.M12 = rt.x;	headMatrix.M13 = up.x;
		headMatrix.M21 = at.y;	headMatrix.M22 = rt.y;	headMatrix.M23 = up.y;
		headMatrix.M31 = at.z;	headMatrix.M32 = rt.z;	headMatrix.M33 = up.z;
	}

	return buttons;
}



/***************************************************************************\
	Update the rotation matrix to account for the user's control inputs.
\***************************************************************************/
void SimClass::UpdateRotation( float rollChange, float pitchChange, float yawChange )
{                    
	float e1dot, e2dot, e3dot, e4dot;
	float enorm;

	float	p = rollChange;
	float	q = pitchChange;
	float	r = yawChange;


   /*-----------------------------------*/
   /* quaternion differential equations */
   /*-----------------------------------*/
   e1dot = (-e4*p - e3*q - e2*r)*0.5F;
   e2dot = (-e3*p + e4*q + e1*r)*0.5F;
   e3dot = ( e2*p + e1*q - e4*r)*0.5F;
   e4dot = ( e1*p - e2*q + e3*r)*0.5F;

   /*-----------------------*/
   /* integrate quaternions */
   /*-----------------------*/
   e1 += e1dot;
   e2 += e2dot;
   e3 += e3dot;
   e4 += e4dot;

   /*--------------------------*/
   /* quaternion normalization */
   /*--------------------------*/
   enorm = (float)sqrt(e1*e1 + e2*e2 + e3*e3 + e4*e4);
   e1 /= enorm;
   e2 /= enorm;
   e3 /= enorm;
   e4 /= enorm;

   /*-------------------*/
   /* direction cosines */
   /*-------------------*/
   rotation.M11 = e1*e1 - e2*e2 - e3*e3 + e4*e4;
   rotation.M21 = 2.0F*(e3*e4 + e1*e2);
   rotation.M31 = 2.0F*(e2*e4 - e1*e3);

   rotation.M12 = 2.0F*(e3*e4 - e1*e2);
   rotation.M22 = e1*e1 - e2*e2 + e3*e3 - e4*e4;
   rotation.M32 = 2.0F*(e2*e3 + e4*e1);

   rotation.M13 = 2.0F*(e1*e3 + e2*e4);
   rotation.M23 = 2.0F*(e2*e3 - e1*e4);
   rotation.M33 = e1*e1 + e2*e2 - e3*e3 - e4*e4;
}



/***************************************************************************\
	Update the rotation matrix to account for the user's control inputs.
\***************************************************************************/
void SimClass::Save( char *filename )
{
	HANDLE	fileID;
	UInt32	bytes;

	// Create a new data file
    fileID = CreateFile( filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if( fileID == INVALID_HANDLE_VALUE ) {
		char string[256];
		PutErrorString( string );
		strcat( string, "Failed to open sim state file." );
		MessageBox( NULL, "Failed to save sim state", string, MB_OK );
	}

	// Write ourselves to disk
	if ( !WriteFile( fileID, this, sizeof(*this), &bytes, NULL ) )  bytes=0xFFFFFFFF;
	if ( bytes != sizeof(*this) ) {
		char string[256];
		PutErrorString( string );
		strcat( string, "Couldn't write sim state." );
		MessageBox( NULL, "Failed to save sim state", string, MB_OK );
	}

	// Close the output file
	CloseHandle( fileID );
}



/***************************************************************************\
	Update the rotation matrix to account for the user's control inputs.
\***************************************************************************/
void SimClass::Restore( char *filename )
{
	HANDLE	fileID;
	UInt32	bytes;
	UInt32	result;

	// Create a new data file
    fileID = CreateFile( filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if( fileID == INVALID_HANDLE_VALUE ) {
		char string[256];
		PutErrorString( string );
		strcat( string, "Failed to open sim state file." );
		MessageBox( NULL, "Failed to load sim state", string, MB_OK );
	}

	// Read ourselves from disk
	result = ReadFile( fileID, this, sizeof(*this), &bytes, NULL );
	if (!result) {
		char	string[80];
		char	message[120];
		PutErrorString( string );
		sprintf( message, "%s:  Couldn'd read far texture palette.", string );
		MessageBox( NULL, "Failed to load sim state", string, MB_OK );
	}

	// Close the output file
	CloseHandle( fileID );
}

