/***************************************************************************\
    DrawGrnd.cpp    Scott Randolph
    July 10, 1996

    Derived class to do special position processing for vehicles on the
	ground.  (More precisly, any object which is to be placed on the 
	ground and reoriented so that it's "up" vector is aligned with the
	terrain normal.)
\***************************************************************************/
#include "Matrix.h"
#include "RViewPnt.h"
#include "RenderOW.h"
#include "DrawGrnd.h"

#ifdef USE_SH_POOLS
MEM_POOL	DrawableGroundVehicle::pool;
#endif



/***************************************************************************\
    Initialize a container for a BSP object to be drawn
\***************************************************************************/
DrawableGroundVehicle::DrawableGroundVehicle( int ID, Tpoint *pos, float heading, float s )
: DrawableBSP( s, ID )
{
	// Store this objects properties
	drivingOn = NULL;
	drawClassID = GroundVehicle;

	// Initialize our position and orientation values
	Update( pos, heading );

	// insure our z position is initialized
	position.z = pos->z;

	// insure orientation has default value
	orientation = IMatrix;
}



/***************************************************************************\
    Add ourselves to our parent list and request callbacks
\***************************************************************************/
void DrawableGroundVehicle::SetParentList( ObjectDisplayList *list )
{
	DrawableBSP::SetParentList( list );

	// If we're changing lists, this will be invalid now...
	SetUpon( NULL );
}



/***************************************************************************\
    Update the object's position and heading.
\***************************************************************************/
void DrawableGroundVehicle::Update( Tpoint *pos, float heading )
{
	// Compute the sine and cosine of the object's desired heading
	yaw = heading;
	cosYaw = (float)cos( heading );
	sinYaw = (float)sin( heading );

	// Record our position (Z will be updated later, as will the orientation matrix)
	position.x = pos->x;
	position.y = pos->y;

	// Force a reevaluation of the terrain elevation and orientation
	previousLOD = -1;
}



/***************************************************************************\
    Make sure the object is placed on the ground then draw it.
\***************************************************************************/
void DrawableGroundVehicle::Draw( class RenderOTW *renderer, int LOD )
{
	// See if we need to update our ground position
	if (LOD != previousLOD) {

		Tpoint	normal;
		float	s;
		float	Nx, Ny, Nz;
		float	x, y, z;

		if (drivingOn) {
			// Get the normal and update our height to conform to the platform we're driving on
			// COBRA - RED - Little Offset to avoid ZBuffering conflicts
			position.z = drivingOn->GetGroundLevel( position.x, position.y, &normal ) - .1f;
		} 
		else {
			// Get the normal and update our height to reflect the terrain beneath us
			// COBRA - RED - Little Offset to avoid ZBuffering conflict
			position.z = renderer->viewpoint->GetGroundLevel( position.x, position.y, &normal ) - .1f;
		}
		previousLOD = LOD;

		// Construct the rotation matrix to orient the object correctly
		// The "old" axes are those of a pure rotation about Z (for heading).
		// The "new" axes include the alignment of "up" with the terrain normal.
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

	// Tell our parent class to draw us now
	DrawableBSP::Draw( renderer, LOD );
}
