/*
** Name: CANVAS3D.CPP
** Description:
**		Class description which allows 2d primitives to be used on a
**		"canvas" that exists somehwere in 3-space.  This class is derived
**		from Render3D and provides 2d operations
**		virtualized from the VirtualDisplay class.  The canvas is also a
**		kind of 3d object which has location and orientation in world
**		space.
** History:
**		3-nov-97 (edg)
**			We go marching in.....
*/
#include <math.h>
#include "Matrix.h"
#include "canvas3d.h"

/*
** Setup
** Calls parent setup and any additional work we need to do here
*/
void Canvas3D::Setup( Render3D *renderer )
{

	// canvas must have a parent 3d context
	r3d = renderer;

	xRes = r3d->GetImageBuffer()->targetXres();
	yRes = r3d->GetImageBuffer()->targetYres();
	 
	// ASSO: reset adjusted RTT viewport
	tLeft = 0;
	tTop = 0;
	tRight = xRes;
	tBottom = yRes;
	txRes = xRes;
	tyRes = yRes;

	canObjPos.x = canObjPos.y = canObjPos.z = 0.0f;
	canWorldPos.x = canWorldPos.y = canWorldPos.z = 0.0f;

	canObjDown.x = canWorldDown.x = 0.0f;
	canObjDown.y = canWorldDown.y = 0.0f;
	canObjDown.z = canWorldDown.z = 1.0f;

	canObjRight.x = canWorldRight.x = 0.0f;
	canObjRight.y = canWorldRight.y = 1.0f;
	canObjRight.z = canWorldRight.z = 0.0f;

	canScaleX = 1.0f;
	canScaleY = 1.0f;

	// Call our parents Setup code (win is available only after this call)
	VirtualDisplay::Setup();

	// we may want to be able to tell code elsewhere that we're a
	// 3d canvas
	type = DISPLAY_CANVAS;
	ForceAlpha=false;
}



/*
** Cleanup
** Just calls parent's cleanup.
*/
void Canvas3D::Cleanup( void )
{
	// Call our parent's cleanup
	VirtualDisplay::Cleanup();
}

/*
** SetCanvas
** Sets the object space rectangle and location from 3 points.
** Points are in order: upperleft, upperright, lowerleft.
** From this is calculated the object space center, down vector, right vector
** and XY scales.
*/
void Canvas3D::SetCanvas( Tpoint *ul, Tpoint *ur, Tpoint *ll )
{
	// right vector calc
	canObjRight.x = ur->x - ul->x;
	canObjRight.y = ur->y - ul->y;
	canObjRight.z = ur->z - ul->z;

	canScaleX = (float)sqrt( canObjRight.x * canObjRight.x +
					  canObjRight.y * canObjRight.y +
					  canObjRight.z * canObjRight.z );

	// normalize
	canObjRight.x /= canScaleX;
	canObjRight.y /= canScaleX;
	canObjRight.z /= canScaleX;

	canScaleX *= 0.5f;

	// down vector calc
	canObjDown.x = ll->x - ul->x;
	canObjDown.y = ll->y - ul->y;
	canObjDown.z = ll->z - ul->z;

	canScaleY = (float)sqrt( canObjDown.x * canObjDown.x +
					  canObjDown.y * canObjDown.y +
					  canObjDown.z * canObjDown.z );

	// normalize
	canObjDown.x /= canScaleY;
	canObjDown.y /= canScaleY;
	canObjDown.z /= canScaleY;

	canScaleY *= 0.5f;

	// center pos
	canObjPos.x = ul->x + canObjRight.x * canScaleX + canObjDown.x * canScaleY;
	canObjPos.y = ul->y + canObjRight.y * canScaleX + canObjDown.y * canScaleY;
	canObjPos.z = ul->z + canObjRight.z * canScaleX + canObjDown.z * canScaleY;

	canWorldPos = canObjPos;
	canWorldDown = canObjDown;
	canWorldRight = canObjRight;
}


/*
** Update:
** Set new world position and orientation of the canvas.
*/
void Canvas3D::Update( const Tpoint *loc, const Trotation *rot )
{
	// new position
	canWorldPos.x = canObjPos.x + loc->x;
	canWorldPos.y = canObjPos.y + loc->y;
	canWorldPos.z = canObjPos.z + loc->z;

	// reorient the down and right vectors
	canWorldDown.x = canObjDown.x * rot->M11 + canObjDown.y * rot->M12 + canObjDown.z * rot->M13;
	canWorldDown.y = canObjDown.x * rot->M21 + canObjDown.y * rot->M22 + canObjDown.z * rot->M23;
	canWorldDown.z = canObjDown.x * rot->M31 + canObjDown.y * rot->M32 + canObjDown.z * rot->M33;

	canWorldRight.x = canObjRight.x * rot->M11 + canObjRight.y * rot->M12 + canObjRight.z * rot->M13;
	canWorldRight.y = canObjRight.x * rot->M21 + canObjRight.y * rot->M22 + canObjRight.z * rot->M23;
	canWorldRight.z = canObjRight.x * rot->M31 + canObjRight.y * rot->M32 + canObjRight.z * rot->M33;
}

/***************************************************************************\
	Put a pixel on the display.
\***************************************************************************/
void Canvas3D::Point( float x1, float y1 )
{
	float x, y;
	Tpoint p;
	float xres, yres;

	// no 3d context, do dice...
	if ( !r3d )
		return;

	// Rotation and translate this point based on the current settings
	x = x1 * dmatrix.rotation00 + y1 * dmatrix.rotation01 + dmatrix.translationX;
	y = x1 * dmatrix.rotation10 + y1 * dmatrix.rotation11 + dmatrix.translationY;

	// Clipping
	if ((x >= -1.0f) && (x <= 1.0f) && (y <= 1.0f) && (y >= -1.0f))
	{
		xres = canScaleX * x;
		yres = -canScaleY * y;

		p.x = canWorldPos.x + canWorldRight.x * xres + canWorldDown.x * yres;
		p.y = canWorldPos.y + canWorldRight.y * xres + canWorldDown.y * yres;
		p.z = canWorldPos.z + canWorldRight.z * xres + canWorldDown.z * yres;
		r3d->ForceAlpha=ForceAlpha;												// COBRA - RED -Forced Alpha
		r3d->Render3DPoint( &p );
	}
}



/***************************************************************************\
	Put a one pixel wide line on the display
\***************************************************************************/
void Canvas3D::Line( float x1, float y1, float x2, float y2 )
{
	float	x;
	int		clipFlag;
	Tpoint  p1, p2;
	float   xres, yres;

	// no 3d context, do dice...
	if ( !r3d )
		return;

	// Rotation and translate this point based on the current settings
	x  = x1 * dmatrix.rotation00 + y1 * dmatrix.rotation01 + dmatrix.translationX;
	y1 = x1 * dmatrix.rotation10 + y1 * dmatrix.rotation11 + dmatrix.translationY;
	x1 = x;

	x  = x2 * dmatrix.rotation00 + y2 * dmatrix.rotation01 + dmatrix.translationX;
	y2 = x2 * dmatrix.rotation10 + y2 * dmatrix.rotation11 + dmatrix.translationY;
	x2 = x;


	// Clip point 1
	clipFlag = ON_SCREEN;
	if ( x1 < -1.0f ) {
		 y1 = y2 + (y1-y2)*( (x2+1.0f)/(x2-x1) );
		 x1 = -1.0f;
		 clipFlag = CLIP_LEFT;
	} else if ( x1 > 1.0f ) {
		 y1 = y2 + (y1-y2)*( (x2-1.0f)/(x2-x1) );
		 x1 = 1.0f;
		 clipFlag = CLIP_RIGHT;
	}
	if ( y1 < -1.0f ) {
		 x1 = x2 + (x1-x2)*( (y2+1.0f)/(y2-y1) );
		 y1 = -1.0f;
		 clipFlag |= CLIP_BOTTOM;
	} else if ( y1 > 1.0f ) {
		 x1 = x2 + (x1-x2)*( (y2-1.0f)/(y2-y1) );
		 y1 = 1.0f;
		 clipFlag |= CLIP_TOP;
	}

	// Clip point 2
	if ( x2 < -1.0f ) {
		 y2 = y1 + (y2-y1)*( (x1+1.0f)/(x1-x2) );
		 x2 = -1.0f;
		 if (clipFlag & CLIP_LEFT)  return;
	} else if ( x2 > 1.0f ) {
		 y2 = y1 + (y2-y1)*( (x1-1.0f)/(x1-x2) );
		 x2 = 1.0f;
		 if (clipFlag & CLIP_RIGHT)  return;
	}
	if ( y2 < -1.0f ) {
		 x2 = x1 + (x2-x1)*( (y1+1.0f)/(y1-y2) );
		 y2 = -1.0f;
		 if (clipFlag & CLIP_BOTTOM)  return;
	} else if ( y2 > 1.0f ) {
		 x2 = x1 + (x2-x1)*( (y1-1.0f)/(y1-y2) );
		 y2 = 1.0f;
		 if (clipFlag & CLIP_TOP)  return;
	}

	xres = canScaleX * x1;
	yres = -canScaleY * y1;

	p1.x = canWorldPos.x + canWorldRight.x * xres + canWorldDown.x * yres;
	p1.y = canWorldPos.y + canWorldRight.y * xres + canWorldDown.y * yres;
	p1.z = canWorldPos.z + canWorldRight.z * xres + canWorldDown.z * yres;

	xres = canScaleX * x2;
	yres = -canScaleY * y2;

	p2.x = canWorldPos.x + canWorldRight.x * xres + canWorldDown.x * yres;
	p2.y = canWorldPos.y + canWorldRight.y * xres + canWorldDown.y * yres;
	p2.z = canWorldPos.z + canWorldRight.z * xres + canWorldDown.z * yres;

	r3d->ForceAlpha=ForceAlpha;												// COBRA - RED -Forced Alpha
	r3d->Render3DLine( &p1, &p2 );
}



/***************************************************************************\
	Put a triangle on the display.  It is not filled (for now at least)
\***************************************************************************/
void Canvas3D::Tri( float x1, float y1, float x2, float y2, float x3, float y3 )
{
	float	x;
	Tpoint  p1, p2, p3;
	float   xres, yres;

	// no 3d context, do dice...
	if ( !r3d )
		return;


	// Line( x1, y1, x2, y2 );
	// Line( x2, y2, x3, y3 );
	// Line( x3, y3, x1, y1 );


	// Rotation and translate this point based on the current settings
	x  = x1 * dmatrix.rotation00 + y1 * dmatrix.rotation01 + dmatrix.translationX;
	y1 = x1 * dmatrix.rotation10 + y1 * dmatrix.rotation11 + dmatrix.translationY;
	x1 = x;

	x  = x2 * dmatrix.rotation00 + y2 * dmatrix.rotation01 + dmatrix.translationX;
	y2 = x2 * dmatrix.rotation10 + y2 * dmatrix.rotation11 + dmatrix.translationY;
	x2 = x;

	x  = x3 * dmatrix.rotation00 + y3 * dmatrix.rotation01 + dmatrix.translationX;
	y3 = x3 * dmatrix.rotation10 + y3 * dmatrix.rotation11 + dmatrix.translationY;
	x3 = x;

	// stoopid clip at the moment
	if ( x1 < -1.0f )
		x1 = -1.0f;
	else if ( x1 >  1.0f )
		x1 = 1.0f;

	if ( x2 < -1.0f )
		x2 = -1.0f;
	else if ( x2 >  1.0f )
		x2 = 1.0f;

	if ( x3 < -1.0f )
		x3 = -1.0f;
	else if ( x3 >  1.0f )
		x3 = 1.0f;

	if ( y1 < -1.0f )
		y1 = -1.0f;
	else if ( y1 >  1.0f )
		y1 = 1.0f;

	if ( y2 < -1.0f )
		y2 = -1.0f;
	else if ( y2 >  1.0f )
		y2 = 1.0f;

	if ( y3 < -1.0f )
		y3 = -1.0f;
	else if ( y3 >  1.0f )
		y3 = 1.0f;

	xres = canScaleX * x1;
	yres = -canScaleY * y1;

	p1.x = canWorldPos.x + canWorldRight.x * xres + canWorldDown.x * yres;
	p1.y = canWorldPos.y + canWorldRight.y * xres + canWorldDown.y * yres;
	p1.z = canWorldPos.z + canWorldRight.z * xres + canWorldDown.z * yres;

	xres = canScaleX * x2;
	yres = -canScaleY * y2;

	p2.x = canWorldPos.x + canWorldRight.x * xres + canWorldDown.x * yres;
	p2.y = canWorldPos.y + canWorldRight.y * xres + canWorldDown.y * yres;
	p2.z = canWorldPos.z + canWorldRight.z * xres + canWorldDown.z * yres;

	xres = canScaleX * x3;
	yres = -canScaleY * y3;

	p3.x = canWorldPos.x + canWorldRight.x * xres + canWorldDown.x * yres;
	p3.y = canWorldPos.y + canWorldRight.y * xres + canWorldDown.y * yres;
	p3.z = canWorldPos.z + canWorldRight.z * xres + canWorldDown.z * yres;

	// r3d->Render3DLine( &p1, &p2 );
	// r3d->Render3DLine( &p2, &p3 );
	// r3d->Render3DLine( &p3, &p1 );

	// Draw the triangle
	r3d->ForceAlpha=ForceAlpha;												// COBRA - RED -Forced Alpha
	r3d->Render3DFlatTri( &p1, &p2, &p3 );

	/*
	*/

}


/***************************************************************************\
	Put a mono-colored string of text on the display.
	(The location given is used as the lower left corner of the text)
\***************************************************************************/
void Canvas3D::TextLeft( float x1, float y1, const char *string, int boxed )
{
	float	x, y;
	ThreeDVertex ps1, ps2;
	Tpoint p1;
	float xres, yres;
	float xStart, yStart;
	int height;

	if ( !*string )
		return;

	r3d->ForceAlpha=ForceAlpha;												// COBRA - RED -Forced Alpha

	// Rotation and translate this point based on the current settings
	x = x1 * dmatrix.rotation00 + y1 * dmatrix.rotation01 + dmatrix.translationX;
	y = x1 * dmatrix.rotation10 + y1 * dmatrix.rotation11 + dmatrix.translationY;

	// Clipping
	if ((x >= -1.0f) && (x <= 1.0f) && (y <= 1.0f) && (y >= -1.0f))
	{
		xres = canScaleX * x;
		yres = -canScaleY * y;
	
		p1.x = canWorldPos.x + canWorldRight.x * xres + canWorldDown.x * yres;
		p1.y = canWorldPos.y + canWorldRight.y * xres + canWorldDown.y * yres;
		p1.z = canWorldPos.z + canWorldRight.z * xres + canWorldDown.z * yres;
	
		// Transform the point from world space to window space
		r3d->TransformPoint( &p1, &ps1 );
		if ( ps1.clipFlag != ON_SCREEN )  return;

		// make sure the text won't go off the edge of our canvas
		// assumption: the canvas doesn't roll so that text travels in the
		// +X direction only
		xres = canScaleX;
	
		p1.x = canWorldPos.x + canWorldRight.x * xres + canWorldDown.x * yres;
		p1.y = canWorldPos.y + canWorldRight.y * xres + canWorldDown.y * yres;
		p1.z = canWorldPos.z + canWorldRight.z * xres + canWorldDown.z * yres;

		r3d->TransformPoint( &p1, &ps2 );

		// now do the slope for the characters
		int numChars;
		int i, width;
		char s[2];
		float slope;

		numChars = strlen( string );
		slope =  (ps2.x - ps1.x);
		if (slope != 0.0f )
			slope = ( ps2.y - ps1.y )/ slope;
		else
			slope = 0.0f;

		// do it letter by letter
		s[1] = 0;

		xStart = ps1.x;
		yStart = ps1.y;

		for ( i = 0; i < numChars; i++ )
		{
			s[0] = string[i];

			// specifically check for blanks (DED text has a lot)
			if ( *s == ' ' && !boxed )
			{
				ps1.x += 3.0f;
				ps1.y += slope * 3.0f;
				continue;
			}

			width = r3d->ScreenTextWidth(s) + 1;
			if ( ps1.x + width >= ps2.x )
				return;
			r3d->ScreenText( ps1.x, ps1.y, s, (boxed & 0x2));
			ps1.x += width;
			ps1.y += slope * width;
		}

		if (boxed == 1) //Surround
		{
			height = r3d->ScreenTextHeight();
			Render2DLine (xStart-3, yStart, ps1.x, ps1.y);
			Render2DLine (xStart-3, yStart+height, ps1.x, ps1.y+height);
			Render2DLine (xStart-3, yStart, xStart-3, yStart+height);
			Render2DLine (ps1.x, ps1.y, ps1.x, ps1.y+height);
		}
		else if (boxed == 4) // Left Arrow
		{
			height = r3d->ScreenTextHeight();
			Render2DLine (xStart, yStart, ps1.x, ps1.y);
			Render2DLine (xStart, yStart+height, ps1.x, ps1.y+height);
			Render2DLine (ps1.x, ps1.y, ps1.x, ps1.y+height);

			Render2DLine (xStart, yStart, xStart - 5, yStart+height/2);
			Render2DLine (xStart, yStart+height, xStart - 5, yStart+height/2);
		}
		else if (boxed == 8) // Right Arrow
		{
			height = r3d->ScreenTextHeight();
			Render2DLine (xStart-3, yStart, ps1.x, ps1.y);
			Render2DLine (xStart-3, yStart+height, ps1.x, ps1.y+height);
			Render2DLine (xStart-3, yStart, xStart-3, yStart+height);

			Render2DLine (ps1.x, ps1.y, ps1.x+5, ps1.y+height/2);
			Render2DLine (ps1.x, ps1.y+height, ps1.x+5, ps1.y+height/2);
		}
	}
}

void Canvas3D::TextLeftVertical( float x1, float y1, const char *string, int boxed )
{
	if ( !*string )
		return;

	y1 += r3d->ScreenTextHeight()/((float)yRes) * canScaleY;

	r3d->ForceAlpha=ForceAlpha;												// COBRA - RED -Forced Alpha
	TextLeft (x1, y1, string, boxed);
}


/***************************************************************************\
	Put a mono-colored string of text on the display.
	(The location given is used as the lower right corner of the text)
\***************************************************************************/
void Canvas3D::TextRight( float x1, float y1, const char *string, int boxed )
{
	if ( !*string )
		return;

	x1 -= r3d->ScreenTextWidth(string)/((float)xRes)* 2.0F * canScaleX;

	r3d->ForceAlpha=ForceAlpha;												// COBRA - RED -Forced Alpha
	TextLeft (x1, y1, string, boxed);
}

/***************************************************************************\
	Put a mono-colored string of text on the display.
	(The location given is used as the lower right corner of the text)
\***************************************************************************/
void Canvas3D::TextRightVertical( float x1, float y1, const char *string, int boxed )
{
	if ( !*string )
		return;

	x1 -= r3d->ScreenTextWidth(string)/((float)xRes) * 2.0F * canScaleX;
	y1 += r3d->ScreenTextHeight()/((float)yRes) * canScaleY;

	r3d->ForceAlpha=ForceAlpha;												// COBRA - RED -Forced Alpha
	TextLeft (x1, y1, string, boxed);
}


/***************************************************************************\
	Put a mono-colored string of text on the display.
	(The location given is used as the horizontal center and vertical lower
	 edge of the text)
\***************************************************************************/
void Canvas3D::TextCenter( float x1, float y1, const char *string, int boxed )
{
	if ( !*string )
		return;

	x1 -= r3d->ScreenTextWidth(string)/((float)xRes) * canScaleX;

	r3d->ForceAlpha=ForceAlpha;												// COBRA - RED -Forced Alpha
	TextLeft (x1, y1, string, boxed);
}

void Canvas3D::TextCenterVertical( float x1, float y1, const char *string, int boxed )
{
	if ( !*string )
		return;

	x1 -= r3d->ScreenTextWidth(string)/((float)xRes) * canScaleX;
	y1 += r3d->ScreenTextHeight()/((float)yRes) * canScaleY;

	r3d->ForceAlpha=ForceAlpha;												// COBRA - RED -Forced Alpha
	TextLeft (x1, y1, string, boxed);
}


/***************************************************************************\
	Draw a circle in the viewport.  The radius given is in
	the X direction.  The Y direction will be scaled to account for the
	aspect ratio of the display and viewport.
\***************************************************************************/
void Canvas3D::Circle (float x, float y, float xRadius)
{
	int entry;

	float x1, y1;
	float x2, y2;
	float yRadius;

	r3d->ForceAlpha=ForceAlpha;												// COBRA - RED -Forced Alpha
	yRadius = xRadius * canScaleX / canScaleY;

	// Prime the pump
	x1 = x + xRadius*CircleX[0];
	y1 = y + yRadius*CircleY[0];

	for (entry = 1; entry <= CircleSegments-1; entry++) {

		// Compute the end point of this next segment
		x2 = (x + xRadius*CircleX[entry]);
		y2 = (y + yRadius*CircleY[entry]);

		// Draw the segment
		Line( x1, y1, x2, y2 );

		// Save the end point of this one to use as the start point of the next one
		x1 = x2;
		y1 = y2;
	}
}



/***************************************************************************\
	Draw a portion of a circle in the viewport.  The radius given is in
	the X direction.  The Y direction will be scaled to account for the
	aspect ratio of the display and viewport.  The start and stop angles
	will be adjusted lie between 0 and 2PI.
\***************************************************************************/
void Canvas3D::Arc (float x, float y, float xRadius, float start, float stop)
{
	int		entry, startEntry, stopEntry;
	float 	yRadius;

	r3d->ForceAlpha=ForceAlpha;												// COBRA - RED -Forced Alpha
	yRadius = xRadius * canScaleX / canScaleY;

	// Find the first and last segment end point of interest
	startEntry	= (int)(fmod(start, 2.0f*PI) / PI * 180.0) / CircleStep;
	stopEntry	= (int)(fmod(stop,  2.0f*PI) / PI * 180.0) / CircleStep;

	// Make sure we aren't overrunning the precomputed array
	ShiAssert( startEntry >= 0 );
	ShiAssert( stopEntry >= 0 );
	ShiAssert( startEntry < CircleSegments );
	ShiAssert( stopEntry < CircleSegments );

	if ( startEntry <= stopEntry) {
		for (entry = startEntry; entry < stopEntry; entry++) {
			Line( x + xRadius*CircleX[entry],   y + yRadius*CircleY[entry],
				  x + xRadius*CircleX[entry+1], y + yRadius*CircleY[entry+1] );
		}
	} else {
		for (entry = startEntry; entry < CircleSegments-1; entry++) {
			Line( x + xRadius*CircleX[entry],   y + yRadius*CircleY[entry],
				  x + xRadius*CircleX[entry+1], y + yRadius*CircleY[entry+1] );
		}
		for (entry = 0; entry < stopEntry; entry++) {
			Line( x + xRadius*CircleX[entry],   y + yRadius*CircleY[entry],
				  x + xRadius*CircleX[entry+1], y + yRadius*CircleY[entry+1] );
		}
	}
}

/***************************************************************************\
	Put a pixel on the display.
\***************************************************************************/
void Canvas3D::Render2DPoint( float x1, float y1 )
{
	r3d->ForceAlpha=ForceAlpha;												// COBRA - RED -Forced Alpha
	r3d->Render2DPoint( x1, y1 );
	// MPRVtx_t	vert;

	// Package up the point's coordinates
	// vert.x = (float)x1;
	// vert.y = (float)y1;

	// Draw the point
	// context.RestoreState( STATE_SOLID );
	// context.Primitive( MPR_PRM_POINTS, 0, 1, sizeof(vert), (unsigned char*)&vert );
}

/***************************************************************************\
	Put a line on the display.
\***************************************************************************/
void Canvas3D::Render2DLine( float x1, float y1, float x2, float y2 )
{
	r3d->ForceAlpha=ForceAlpha;												// COBRA - RED -Forced Alpha
	r3d->Render2DLine( x1, y1, x2, y2 );
	// MPRVtx_t	vert;

	// Package up the point's coordinates
	// vert.x = (float)x1;
	// vert.y = (float)y1;

	// Draw the point
	// context.RestoreState( STATE_SOLID );
	// context.Primitive( MPR_PRM_POINTS, 0, 1, sizeof(vert), (unsigned char*)&vert );
}

/*
** Calculate how high a line is on the canvas normalized to pixel height
** of canvas
*/
float Canvas3D::NormalizedLineHeight( void )
{
	float	scaley;
	ThreeDVertex ps1, ps2;
	Tpoint p1;

	
	// xform top and bottom points of canvas to screen
	p1.x = canWorldPos.x + canWorldDown.x * canScaleY;
	p1.y = canWorldPos.y + canWorldDown.y * canScaleY;
	p1.z = canWorldPos.z + canWorldDown.z * canScaleY;
	
	r3d->TransformPoint( &p1, &ps1 );

	p1.x = canWorldPos.x - canWorldDown.x * canScaleY;
	p1.y = canWorldPos.y - canWorldDown.y * canScaleY;
	p1.z = canWorldPos.z - canWorldDown.z * canScaleY;

	r3d->TransformPoint( &p1, &ps2 );

	scaley = ps1.y - ps2.y;

	if ( scaley <= 0.0f )
		return 0.0f;

	return 8.0f/scaley;
}

/*
** 	How wide is string s in normalized screen space
*/
float Canvas3D::TextWidth( char *s )
{
	float	scalex;
	ThreeDVertex ps1, ps2;
	Tpoint p1;

	
	// xform top and bottom points of canvas to screen
	p1.x = canWorldPos.x + canWorldRight.x * canScaleX;
	p1.y = canWorldPos.y + canWorldRight.y * canScaleX;
	p1.z = canWorldPos.z + canWorldRight.z * canScaleX;
	
	r3d->TransformPoint( &p1, &ps1 );

	p1.x = canWorldPos.x - canWorldRight.x * canScaleX;
	p1.y = canWorldPos.y - canWorldRight.y * canScaleX;
	p1.z = canWorldPos.z - canWorldRight.z * canScaleX;

	r3d->TransformPoint( &p1, &ps2 );

	scalex = ps1.x - ps2.x;

	if ( scalex <= 0.0f )
		return 0.0f;

	return r3d->ScreenTextWidth(s)/scalex;
}

/*
** 	How high is char  in normalized screen space
*/
float Canvas3D::TextHeight( void  )
{
	float	scaley;
	ThreeDVertex ps1, ps2;
	Tpoint p1;

	
	// xform top and bottom points of canvas to screen
	p1.x = canWorldPos.x + canWorldDown.x * canScaleY;
	p1.y = canWorldPos.y + canWorldDown.y * canScaleY;
	p1.z = canWorldPos.z + canWorldDown.z * canScaleY;
	
	r3d->TransformPoint( &p1, &ps1 );

	p1.x = canWorldPos.x - canWorldDown.x * canScaleY;
	p1.y = canWorldPos.y - canWorldDown.y * canScaleY;
	p1.z = canWorldPos.z - canWorldDown.z * canScaleY;

	r3d->TransformPoint( &p1, &ps2 );

	scaley = ps1.y - ps2.y;

	if ( scaley <= 0.0f )
		return 0.0f;

	return r3d->ScreenTextHeight()/scaley;
}
