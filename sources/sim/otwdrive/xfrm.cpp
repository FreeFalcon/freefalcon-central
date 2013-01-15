#include "stdhdr.h"
#include "otwdrive.h"
#include "Graphics/Include/grtypes.h"

void OTWDriverClass::BuildHeadMatrix(int doFlip, int order, float headYaw, float headPitch, float headRoll)
{
	Tpoint	at={1.0F}, up={1.0F}, rt={1.0F};
	float	scale=0.0F;
	mlTrig	trigYaw={0.0F}, trigPitch={0.0F};

#if 0
	if(headPitch < -90.0F * DTR) {
		headPitch = headPitch - (180.0F * DTR);
		if(headYaw >= 0.0F) {
			headYaw -= 180.0F * DTR;
		}
		else {
			headYaw += 180.0F * DTR;
		}
	}
#endif
	// at is a point on the unit sphere
	mlSinCos (&trigYaw,   headYaw);
	mlSinCos (&trigPitch, headPitch);

	if(order == YAW_PITCH) {	// front
		at.x = trigYaw.cos * trigPitch.cos;
		at.y = trigYaw.sin * trigPitch.cos;
		at.z = trigPitch.sin;
	}
	else if(order == PITCH_YAW) { // back
		at.x = trigYaw.cos * trigPitch.cos;
		at.y = trigYaw.sin;
		at.z = trigYaw.cos * trigPitch.sin;
	}
	else {
		ShiWarning("what order should we process in?");
		//do front so we have some data if given a bad order
		at.x = trigYaw.cos * trigPitch.cos;
		at.y = trigYaw.sin * trigPitch.cos;
		at.z = trigPitch.sin;
		ShiWarning( "Bad Rotation Order" );
	}

	scale = 1.0f / (float)sqrt(at.x * at.x + at.y * at.y + at.z * at.z);
	at.x *= scale;
	at.y *= scale;
	at.z *= scale;

	// (0, 0, 1) cross at [normalized]
	scale = 1.0f / (float)sqrt(at.x * at.x + at.y * at.y);
	rt.x = -at.y * scale;
	rt.y = at.x * scale;
	rt.z = 0.0f;

	// at cross rt [normalized]
	up.x = at.y * rt.z - at.z * rt.y;
	up.y = at.z * rt.x - at.x * rt.z;
	up.z = at.x * rt.y - at.y * rt.x;
	scale = 1.0f / (float)sqrt(up.x * up.x + up.y * up.y + up.z * up.z);
	up.x *= scale;
	up.y *= scale;
	up.z *= scale;

	if(doFlip) {
		doFlip = -1;
	}
	else if(headRoll == 0.0F){
		doFlip = 1;
	}
	else {
		CalculateHeadRoll(headRoll, &at, &up, &rt);
		doFlip = 1;
	}

	// Load the matrix
	headx = headMatrix.M11 = at.x;	headMatrix.M12 = rt.x * doFlip;	headMatrix.M13 = up.x * doFlip;
	heady = headMatrix.M21 = at.y;	headMatrix.M22 = rt.y * doFlip;	headMatrix.M23 = up.y * doFlip;
	headz = headMatrix.M31 = at.z;	headMatrix.M32 = rt.z * doFlip;	headMatrix.M33 = up.z * doFlip;
}



void OTWDriverClass::CalculateHeadRoll(float headRoll, Tpoint* p_at, Tpoint* p_up, Tpoint* p_rt)
{
	// Gillman was medicated when he requested this head roll...
	// Oh yeah simple, no problem.  Well just do a rotation in 3 space about
	// arbitrary line, piece of cake, not!
	// See the section entitled "Rotation Tools" in Graphics Gems, edited by Andrew S. Glassner
	// for the solution served on a silver platter.  Or see Mathematical Elements for
	// Computer Graphics, 2nd Ed. by Rodgers and Adams. Section 3-9 has a very nice explaination
	// to this very nontrivial problem.

	if(headRoll != 0.0F) {

		mlTrig		trigRoll;
		Trotation	R;
		Tpoint		p;
		float			c, s, t;
		float			x, y, z;
		float			scale;

		scale	= 1.0f / (float)sqrt(p_at->x * p_at->x + p_at->y * p_at->y + p_at->z * p_at->z);
		x		= p_at->x * scale;
		y		= p_at->y * scale;
		z		= p_at->z * scale;

	   mlSinCos(&trigRoll, headRoll);

		c		= trigRoll.cos;
		s		= trigRoll.sin;
		t		= 1 - c;

		R.M11 = t * x * x + c;		R.M12 = t * x * y + s * z;		R.M13 = t * x * z - s * y;
		R.M21 = t * x * y - s * z;	R.M22 = t * y * y + c;			R.M23 = t * y * z + s * x;
		R.M31 = t * x * z + s * y;	R.M32 = t * y * z - s * x;		R.M33 = t * z * z + c;

		MatrixMult (&R, p_rt, &p);
		p_rt->x	= p.x;
		p_rt->y	= p.y;
		p_rt->z	= p.z;

		MatrixMult (&R, p_up, &p);
		p_up->x	= p.x;
		p_up->y	= p.y;
		p_up->z	= p.z;
	}
}

