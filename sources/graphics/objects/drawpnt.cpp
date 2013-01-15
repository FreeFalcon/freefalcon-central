/***************************************************************************\
    DrawPNT.cpp

	Derived calss from DrawableObject which will just draw a pixel at its
	given location and will draw a label for it (if turned on).  In general,
	this can be used to draw an object that's very far away.
\***************************************************************************/
#include "Matrix.h"
#include "TimeMgr.h"
#include "TOD.h"
#include "RenderOW.h"
#include "RViewPnt.h"
#include "DrawBSP.h"
#include "DrawPNT.h"

#ifdef USE_SH_POOLS
MEM_POOL	DrawablePoint::pool;
#endif

BOOL	DrawablePoint::drawLabels = FALSE;		// Shared by ALL drawable points (campaing labels)
extern int g_nNearLabelLimit;	// JB 000807
extern bool g_bLabelRadialFix;
extern bool g_bLabelShowDistance;
extern BOOL renderACMI;

/***************************************************************************\
    Initialize a container for a Point object to be drawn
\***************************************************************************/
DrawablePoint::DrawablePoint( DWORD color, BOOL grnd, const Tpoint *pos, float s )
: DrawableObject( s )
{
	// Initialize our member variables
	pointColor = color;
	onGround = grnd;
	labelLen = 0;
	drawClassID = Default;
	radius = 1.0f;
	
	// Record our position
	position = *pos;

	// Force a reevaluation of the terrain elevation
	previousLOD = -1;
}



/***************************************************************************\
    Remove an instance of a Point object.
\***************************************************************************/
DrawablePoint::~DrawablePoint( void )
{
}



/***************************************************************************\
    Update the position and orientation of this object.
\***************************************************************************/
void DrawablePoint::Update( const Tpoint *pos )
{
	// Update the location of this object
	position.x = pos->x;
	position.y = pos->y;

	// if we're on ground, z position will be adjusted in draw
	if ( onGround == FALSE )
		position.z = pos->z;

	// Force a reevaluation of the terrain elevation and orientation
	previousLOD = -1;
}



/***************************************************************************\
    Store the labeling information for this object instance.
\***************************************************************************/
void DrawablePoint::SetLabel( char *labelString, DWORD color )
{
	ShiAssert( strlen( labelString ) < sizeof( label ) );

	strncpy( label, labelString, 31 );
	label[31] = 0;
	labelColor = color;
	labelLen = VirtualDisplay::ScreenTextWidth( labelString ) >> 1;
}


/***************************************************************************\
\***************************************************************************/
void DrawablePoint::Draw( RenderOTW *renderer, int LOD )
{
	ThreeDVertex	labelPoint;

	if ( DrawablePoint::drawLabels && labelLen ) {

		// See if we need to update our ground position
		if (onGround == TRUE && LOD != previousLOD)
		{
			// Update our height to reflect the terrain beneath us
			position.z = renderer->viewpoint->GetGroundLevel( position.x, position.y, NULL );
			previousLOD = LOD;
		}

	// Now print our label text, distance coded color
		renderer->TransformPoint( &position, &labelPoint ); // JB 010112
		float x,y;
		// RV - RED - If ACMI force Label Limit to 150 nMiles
		long limit = (renderACMI?150:g_nNearLabelLimit) * 6076 + 8,limitcheck;
		if (!DrawablePoint::drawLabels)
			limitcheck = (renderACMI?150:g_nNearLabelLimit) * 6076 + 8;
		else limitcheck = 300 * 6076+8; // 

//dpc LabelRadialDistanceFix
//First check if Z distance is below "limitcheck" and only if it is then do additional
//radial distance check (messes up .csZ value - but it shouldn't matter
// since labelPoint is local and .csZ is not used afterwards)
// Besides no need to calculate radial distance is Z distance is already greater
		if (g_bLabelRadialFix)
			if (labelPoint.clipFlag == ON_SCREEN &&
				labelPoint.csZ < limitcheck)			//Same condition as below!!!
			{
				float dx = position.x - renderer->X();
				float dy = position.y - renderer->Y();
				float dz = position.z - renderer->Z();
				labelPoint.csZ = (float)sqrt(dx*dx + dy*dy + dz*dz);
			}
//end LabelRadialDistanceFix

		if (labelPoint.clipFlag == ON_SCREEN &&
			labelPoint.csZ < limitcheck)
		{
			int colorsub = int((labelPoint.csZ / (limit >> 3))) << 5;
			if (colorsub > 180)	// let's not reduce brightness too much, keep a glimpse of the original color
				colorsub = 180;
			int red = (labelColor & 0x000000ff);
			red -= min(red, colorsub);				// minimum red = 100
			int green = (labelColor & 0x0000ff00) >> 8;
			green -= min(green, colorsub+30);		// minimum green = 70, 100 is too light
			int blue = (labelColor & 0x00ff0000) >> 16;
			blue -= min(blue, colorsub);			// minimum blue = 100

			long newlabelColor = blue << 16 | green << 8 | red;

			x = labelPoint.x - renderer->ScreenTextWidth(label) / 2;		// Centers text
			y = labelPoint.y - 12;				// Place text above center of object
			renderer->SetColor( newlabelColor );
			renderer->ScreenText( x, y, label );
//dpc LabelRadialDistanceFix
			if (g_bLabelShowDistance)
			{
				char label2[32];
				sprintf(label2, "%4.1f nm", labelPoint.csZ / 6076);	// convert from ft to nm
				float x2 = labelPoint.x - renderer->ScreenTextWidth(label2) / 2;	// Centers text
				float y2 = labelPoint.y + 12; // Distance below center object
				renderer->ScreenText( x2, y2, label2);
			}
//end LabelRadialDistanceFix
		}
// JB 000807
	}
/*				original code
	// Transform the point to screen space
		renderer->TransformPoint( &position, &labelPoint );

		// Print the text if it is on screen
		if ( labelPoint.clipFlag == ON_SCREEN )
		{
			labelPoint.x -= labelLen;		// Centers text
			labelPoint.y -= 12;				// Place text above center of object
			renderer->SetColor( labelColor );
			renderer->ScreenText( labelPoint.x, labelPoint.y, label );
		}
	}
	*/
}
