/***************************************************************************\
    DrawPole.cpp    Ed Goldman

	Based on DrawableShadowed, this class is used in ACMI to put altitude
	poles and extra labels on an object.
\***************************************************************************/
#include "Matrix.h"
#include "RViewPnt.h"
#include "RenderOW.h"
#include "Drawpole.h"

#ifdef USE_SH_POOLS
MEM_POOL	DrawablePoled::pool;
#endif

BOOL	DrawablePoled::drawHeading = FALSE;			// Shared by ALL drawable BSPs
BOOL	DrawablePoled::drawAlt = FALSE;				// Shared by ALL drawable BSPs
BOOL	DrawablePoled::drawSpeed = FALSE;			// Shared by ALL drawable BSPs
BOOL	DrawablePoled::drawTurnRadius = FALSE;		// Shared by ALL drawable BSPs
BOOL	DrawablePoled::drawTurnRate = FALSE;		// Shared by ALL drawable BSPs
BOOL	DrawablePoled::drawPole = TRUE;				// Shared by ALL drawable BSPs
BOOL	DrawablePoled::drawlockrange = TRUE;		// Shared by ALL drawable BSPs


/***************************************************************************\
    Initialize a container for a BSP object to be drawn
\***************************************************************************/
DrawablePoled::DrawablePoled( int ID, const Tpoint *pos, const Trotation *rot, float s )
: DrawableBSP( ID, pos, rot, s )
{
	// init our label stuff
	headingLen = 0;
	altLen = 0;
	speedLen = 0;
	turnRadiusLen = 0;
	turnRateLen = 0;

	// turn off target box
	isTarget = FALSE;
}



/***************************************************************************\
    Make sure the object is placed on the ground then draw it.
\***************************************************************************/
void DrawablePoled::Draw( class RenderOTW *renderer, int LOD )
{
	ThreeDVertex	labelPoint;
	float			x, y;

	// draw pole 1st if enabled
	if ( drawPole == TRUE )
	{
		Tpoint		pos;

		pos.x = position.x;
		pos.y = position.y;
		pos.z = renderer->viewpoint->GetGroundLevel( position.x, position.y );

		// blue poles
		renderer->SetColor (0xffff0000);
		renderer->Render3DLine( &pos, &position);
	}

	// Tell our parent class to draw us now
	DrawableBSP::Draw( renderer, LOD );

	renderer->TransformPoint( &position, &labelPoint );

	// only if object center is on screen
	if ( labelPoint.clipFlag == ON_SCREEN )
	{
		y = labelPoint.y + 12;
		renderer->SetColor( 0xffff0000 );  //0xff00ffff

		// now we draw the data labels if enabled
		if (drawSpeed && speedLen)
		{
			x = labelPoint.x - speedLen;		// Centers text
			renderer->ScreenText( x, y, speed );
			y += 10.0f;
		}


		if (drawAlt && altLen)
		{
			x = labelPoint.x - altLen;		// Centers text
			renderer->ScreenText( x, y, alt );
			y += 10.0f;
		}

		if (drawHeading && headingLen)
		{
			x = labelPoint.x - headingLen;		// Centers text
			renderer->ScreenText( x, y, heading );
			y += 10.0f;
		}

		if (drawlockrange && lockrangeLen)
		{
			x = labelPoint.x - lockrangeLen;		// Centers text
			renderer->ScreenText( x, y, lockrange );
			y += 10.0f;
		}

		if (drawTurnRate && turnRateLen)
		{
			x = labelPoint.x - turnRateLen;		// Centers text
			renderer->ScreenText( x, y, turnRate );
			y += 10.0f;
		}

		if (drawTurnRadius && turnRadiusLen)
		{
			x = labelPoint.x - turnRadiusLen;		// Centers text
			renderer->ScreenText( x, y, turnRadius );
			y += 10.0f;
		}

		if ( isTarget )
			DrawTargetBox( renderer, &labelPoint );
	}
}


/***************************************************************************\
    Store the labeling information for this object instance.
\***************************************************************************/
void DrawablePoled::SetDataLabel( DWORD type, char *labelString )
{
	ShiAssert( strlen( labelString ) < sizeof( label ) );

	switch ( type )
	{
		case DP_LABEL_HEADING:
			strcpy( heading, labelString );
			headingLen = VirtualDisplay::ScreenTextWidth( labelString ) >> 1;
			break;
		case DP_LABEL_ALT:
			strcpy( alt, labelString );
			altLen = VirtualDisplay::ScreenTextWidth( labelString ) >> 1;
			break;
		case DP_LABEL_SPEED:
			strcpy( speed, labelString );
			speedLen = VirtualDisplay::ScreenTextWidth( labelString ) >> 1;
			break;
		case DP_LABEL_TURNRADIUS:
			strcpy( turnRadius, labelString );
			turnRadiusLen = VirtualDisplay::ScreenTextWidth( labelString ) >> 1;
			break;
		case DP_LABEL_TURNRATE:
			strcpy( turnRate, labelString );
			turnRateLen = VirtualDisplay::ScreenTextWidth( labelString ) >> 1;
			break;
		case DP_LABEL_LOCK_RANGE:
			strcpy( lockrange, labelString );
			lockrangeLen = VirtualDisplay::ScreenTextWidth( labelString ) >> 1;
		default:
			break;
	}
}


/***************************************************************************\
	Return the specified data label
\***************************************************************************/
char * DrawablePoled::DataLabel( DWORD type )
{
	switch ( type )
	{
		case DP_LABEL_HEADING:
			return heading;
			break;
		case DP_LABEL_ALT:
			return alt;
			break;
		case DP_LABEL_SPEED:
			return speed;
			break;
		case DP_LABEL_TURNRADIUS:
			return turnRadius;
			break;
		case DP_LABEL_TURNRATE:
			return turnRate;
			break;
		case DP_LABEL_LOCK_RANGE://me123
			return lockrange;
			break;
		default:
			break;
	}

	return NULL;
}

/***************************************************************************\
	Draw Target box on the object
\***************************************************************************/
void DrawablePoled::DrawTargetBox( class RenderOTW *renderer, ThreeDVertex *spos  )
{
	DWORD clip = 0;
	float x1, y1, x2, y2;

	float xres = (float)renderer->GetXRes();
	float yres = (float)renderer->GetYRes();

	x1 = spos->x - 7.0f;
	y1 = spos->y - 7.0f;
	x2 = spos->x + 7.0f;
	y2 = spos->y + 7.0f;

	// not sure if clipping needed, to be safe....
	if ( x1 < 0.0f )
	{
		clip |= CLIP_LEFT;
		x1 = 0.0f;
	}
	if ( y1 < 0.0f )
	{
		clip |= CLIP_TOP;
		y1 = 0.0f;
	}
	if ( x2 >= xres )
	{
		clip |= CLIP_RIGHT;
		x2 = xres - 1.0f;
	}
	if ( y2 >= yres )
	{
		clip |= CLIP_BOTTOM;
		y2 = yres - 1.0f;
	}

	// yellow
	renderer->SetColor (boxColor);

	// draw box
	if ( !( clip & CLIP_LEFT ) )
	{
		renderer->Render2DLine( x1, y1, x1, y2 );
	}
	if ( !( clip & CLIP_TOP ) )
	{
		renderer->Render2DLine( x1, y1, x2, y1 );
	}
	if ( !( clip & CLIP_RIGHT ) )
	{
		renderer->Render2DLine( x2, y1, x2, y2 );
	}
	if ( !( clip & CLIP_BOTTOM ) )
	{
		renderer->Render2DLine( x2, y2, x1, y2 );
	}
}
