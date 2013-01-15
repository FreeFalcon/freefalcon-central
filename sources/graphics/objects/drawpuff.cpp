/***************************************************************************\
    DrawPuff.cpp
    Scott Randolph
    February 26, 1997

    Subclass for drawing puffy clouds.
\***************************************************************************/
#include <math.h>
#include "Matrix.h"
#include "DrawPuff.h"

#ifdef USE_SH_POOLS
MEM_POOL	DrawablePuff::pool;
#endif


/***************************************************************************\
    Do what little extra setup work we require.
\***************************************************************************/
DrawablePuff::DrawablePuff( int ID, int texSetNum, Tpoint *pos )
: DrawableBSP( ID, pos, &IMatrix, 1.0f )
{
	drawClassID = Puffy;
	
	// Store the texture set number we're to draw with
	SetTextureSet( texSetNum );
}



/***************************************************************************\
    Update the position of this overcast tile (called periodically to
	account for wind).
\***************************************************************************/
void DrawablePuff::UpdateForDrift( float x, float y )
{
	// Record our center point for list sorting purposes
	position.x = x;
	position.y = y;

	// KCK: Why arn't we doing the following?:
	// are puffy's drifting? I've not noticed them not.
//	position.x = x + RealWeather->xOffset * FEET_PER_KM
//	position.y = y + RealWeather->yOffset * FEET_PER_KM
}
