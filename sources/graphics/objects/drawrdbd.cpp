/***************************************************************************\
    DrawRdbd.cpp    Scott Randolph
    July 29, 1997

    Derived class to do special position and containment processing for
	sections of bridges.
\***************************************************************************/
#include "DrawRdbd.h"


#ifdef USE_SH_POOLS
MEM_POOL	DrawableRoadbed::pool;
#endif


/***************************************************************************\
    Initialize an object to represent a piece of bridge roadbed.
\***************************************************************************/
DrawableRoadbed::DrawableRoadbed( int IDbase, int IDtop, Tpoint *pos, float heading, float height, float angle, float s )
: DrawableBuilding( IDbase, pos, heading, s )
{
	drawClassID		= Roadbed;
	start			= position.x + instance.BoxBack();
	length			= instance.BoxFront() - instance.BoxBack();

	cosInvYaw		= (float)cos( -heading );
	sinInvYaw		= (float)sin( -heading );
	tanRampAngle	= (float)tan( angle );
	ramp.SetupWithVector( 0.0f, height, 1.0f, tanRampAngle );

	// 0 is a legal VIS ID, but is currently "VIS_NOTHING", so exclude it.
	// If this ever changes, then (IDtop >= 0) would be appropriate.
	if (IDtop > 0) {
		superStructure = new DrawableBSP( IDtop, &position, &orientation, s );
		ShiAssert( superStructure );
	} else {
		superStructure = NULL;
	}
}



/**************************************************************************
    Initialize an object to represent a piece of bridge roadbed.
***************************************************************************/
DrawableRoadbed::~DrawableRoadbed()
{
	if (superStructure) {
		delete superStructure;
		superStructure = NULL;
	}
};



/**************************************************************************
    See if the given point is within our perview.  If so, provide the
	height and normal of the surface at that point.
***************************************************************************/
BOOL DrawableRoadbed::OnRoadbed( Tpoint *pos, Tpoint *normal )
{
	float	x, y, d;


	// First see if the point is too far away
	if ( (fabs(pos->x - position.x) > radius) || (fabs(pos->y - position.y) > radius) ) {
		return FALSE;
	}

	// See if we can do a simple case for level segments
	if (tanRampAngle == 0.0f) {
		if (normal) {
			normal->x =  0.0f;
			normal->y =  0.0f;
			normal->z = -1.0f;
		}

		pos->z = position.z - ramp.Y(0.0f);		// -Z is up, but height is positive up
	} else {
		// Tranlate into object space
		x = pos->x - position.x;
		y = pos->y - position.y;

		// Rotate into object space (only worried about distance along bridge axis)
		d = x * cosInvYaw - y * sinInvYaw;

		// Construct the worldspace rotated normal to the ramp
		if (normal) {
			normal->x = -tanRampAngle*cosInvYaw;
			normal->y =  tanRampAngle*sinInvYaw;
			normal->z = -1.0f;
		}

		// Return the altitude of the ramp at the given point
		pos->z = position.z - ramp.Y(d);		// -Z is up, but height is positive up
	}

	return TRUE;
}



/***************************************************************************\
    Just do a simple draw on the base piece.
\***************************************************************************/
void DrawableRoadbed::Draw( class RenderOTW *renderer, int LOD )
{
	// Tell our grand-parent class to draw us now (we don't want our parent to update our Z)
	DrawableBSP::Draw( renderer, LOD );
}



/***************************************************************************\
    Just do a simple draw on the superstructure piece.
\***************************************************************************/
void DrawableRoadbed::DrawSuperstructure( class RenderOTW *renderer, int LOD )
{
	// Draw the superstructure (if we have one)
	if (superStructure) {
		superStructure->Draw( renderer, LOD );
	}
}



/***************************************************************************\
    Just do a simple draw on the superstructure piece.
\***************************************************************************/
void DrawableRoadbed::DrawSuperstructure( class Render3D *renderer )
{
	// Draw the superstructure (if we have one)
	if (superStructure) {
		superStructure->Draw( renderer );
	}
}
