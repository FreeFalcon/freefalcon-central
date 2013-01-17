/***************************************************************************\
    DrawGuys.cpp    Scott Randolph
    April 6, 1998

    Derived class to do special position processing for foot soldiers on the
	ground.
\***************************************************************************/
#include "TimeMgr.h"
#include "Matrix.h"
#include "RViewPnt.h"
#include "RenderOW.h"
#include "DrawGuys.h"

#ifdef USE_SH_POOLS
MEM_POOL	DrawableGuys::pool;
#endif

// table of offsets for squads
static Tpoint sSquadOffsets[] =
{
	{  0.0f, 	0.0f,		0.0f },
	{ -5.0f,    5.0f,		0.0f },
	{  5.0f,   12.0f,		0.0f },
	{  8.0f,   -5.0f,		0.0f },
	{ -6.0f,  -10.0f,		0.0f },
	{  0.0f,   20.0f,		0.0f },
	{ 20.0f,    0.0f,		0.0f },
	{  0.0f,  -20.0f,		0.0f },
	{-20.0f,    0.0f,		0.0f },
};
/* KCK: Randomized more. Would be nice to specify this ourselves..
static Tpoint sSquadOffsets[] =
{
	{  0.0f, 	0.0f,		0.0f },
	{-15.0f,   15.0f,		0.0f },
	{ 15.0f,   15.0f,		0.0f },
	{ 15.0f,  -15.0f,		0.0f },
	{-15.0f,  -15.0f,		0.0f },
	{  0.0f,   20.0f,		0.0f },
	{ 20.0f,    0.0f,		0.0f },
	{  0.0f,  -20.0f,		0.0f },
	{-20.0f,    0.0f,		0.0f },
};
*/

/***************************************************************************\
    Initialize a container for a BSP object to be drawn
\***************************************************************************/
DrawableGuys::DrawableGuys( int ID, Tpoint *pos, float heading, int numGuys, float s )
: DrawableGroundVehicle( ID, pos, heading, s )
{
	drawClassID = Guys;

	numInSquad = numGuys;
	moving = FALSE;
}



/***************************************************************************\
    Make sure the object is placed on the ground then draw it.
\***************************************************************************/
void DrawableGuys::Draw( class RenderOTW *renderer, int LOD )
{
	int i;
	Tpoint savePosition;
	Tpoint *poff;
	int mask;
	BOOL saveLabels;
	float relYaw;
	int	animFrame;
	int animShift;


	// See if we need to update our ground position because the terrain under us changed
	if (LOD != previousLOD) {

		Tpoint	normal;
		float	s;
		float	Nx, Ny, Nz;
		float	x, y, z;

		if (drivingOn) {
			// Get the normal and update our height to conform to the platform we're driving on
			position.z = drivingOn->GetGroundLevel( position.x, position.y, &normal );
		} else {
			// Get the normal and update our height to reflect the terrain beneath us
			position.z = renderer->viewpoint->GetGroundLevel( position.x, position.y, &normal );
		}
		previousLOD = LOD;

		// Construct the rotation matrix to orient the object correctly
		// The "old" axes are those of a pure rotation about Z (for heading).
		// The "new" axes include the alignment of "up" with the terrain normal.
		// Store the matrix in transposed form to work with Erick's row vector based library
		// New Z axis (Inverted Terrain Normal)
		Nx = -normal.x,
		Ny = -normal.y,
		Nz = -normal.z;
		s =  1.0f / (float)sqrt( Nx*Nx + Ny*Ny + Nz*Nz );
		orientation.M13 = Nx*s, orientation.M23 = Ny*s, orientation.M33 = Nz*s;

		// New X axis (New Z axis cross negative old Y axis)
		x =  Nz*cosYaw,
		y =  Nz*sinYaw,
		z = -Nx*cosYaw-Ny*sinYaw;
		s =  1.0f / (float)sqrt( x*x + y*y + z*z );
		orientation.M11 = x*s, orientation.M21 = y*s, orientation.M31 = z*s;

		// New Y axis (New Z axis cross old X axis)
		x = -Nz*sinYaw,				   
		y =  Nz*cosYaw,				   
		z =  Nx*sinYaw-Ny*cosYaw;
		s =  1.0f / (float)sqrt( x*x + y*y + z*z );
		orientation.M12 = x*s, orientation.M22 = y*s, orientation.M32 = z*s;
	}


	//	1) determine which switch to set based on our relative heading
	//	   with view heading.
	//	2) If moving, determine which switch to set to cycle animation
	//	3) If more than 1 unit in squad position and draw several of them

	// figure out which orientation to use....
	// calculate the relative headings
	relYaw = yaw - renderer->Yaw();
	if ( relYaw < 0.0f )
		relYaw += 2.0f * PI;


	// there are a set of 8 different orientations each with 4 bits
	// for the animation sequence.  Therefore, get a number from 0-7
	// for the shift position and mult by 4 to get the starting bit
	// position of the 1st animation frame for the orientation.
	animShift = FloatToInt32( 8.0f * relYaw / ( 2.0f * PI ) );
	animShift &= 0x00000007;

	// set appropriate switch
	if ( moving )
	{
		// cycle animation at about 4 fps
		animFrame = (TheTimeManager.GetClockTime() >> 8) & 0x3;

		// switch 0 is for the moving case
		mask = ( 1 << ((animShift << 2) + animFrame) );
		SetSwitchMask( 0, mask );
		SetSwitchMask( 1, 0 );
	}
	else
	{
		// switch 1 is for the shooting case
		mask = ( 1 << (animShift) );
		SetSwitchMask( 0, 0 );
		SetSwitchMask( 1, mask );
	}

	// save our actual position
	savePosition = position;

	// save labeling state since we don't want to draw labels
	// for all units
	saveLabels = drawLabels;


	for ( i = 0; i < numInSquad; i++ )
	{
		// we don't need to do any special positioning of 1st unit
		if ( i > 0 )
		{
			// get pointer to offsets
			poff = &sSquadOffsets[i];

			position.x = savePosition.x +
					poff->x * orientation.M11 +
					poff->y * orientation.M21;
			position.y = savePosition.y +
					poff->x * orientation.M12 +
					poff->y * orientation.M22;
			position.z = savePosition.z +
					poff->x * orientation.M13 +
					poff->y * orientation.M23;
			drawLabels = FALSE;
		}

		// Tell our grandparent class to draw one instance of us
		DrawableBSP::Draw( renderer, LOD );
	}

	// restore position
	position = savePosition;
	drawLabels = saveLabels;
}
