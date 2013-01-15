/***************************************************************************\
    Display.cpp
    Scott Randolph
    February 1, 1995

    This class provides basic 2D drawing functions in a device independent fashion.
\***************************************************************************/
#include <math.h>
#include "Display.h"
#include "Render3D.h" // ASSO:

// COBRA - DX - DX Engine includes
#include "Graphics/DXEngine/DXVBManager.h"
#include "Graphics/DXEngine/DXEngine.h"
extern	bool g_bUse_DX_Engine;

// ASSO: BEGIN
extern int g_nGfxFix;
TextureHandle* VirtualDisplay::renderTexture = 0;
IDirectDrawSurface7* VirtualDisplay::oldTarget = 0;
Render3D* VirtualDisplay::r3d = 0;
float VirtualDisplay::oldTop = 0.0f; 
float VirtualDisplay::oldLeft = 0.0f; 
float VirtualDisplay::oldBottom = 0.0f; 
float VirtualDisplay::oldRight = 0.0f;
// ASSO: END

// An array of precomputed points on a unit circle to be shared by all instances of this class
// Element zero is for theta = 0, and each successive element adds 4 degrees to
// theta.  theta = 360 is a repeat of theta = 0.  The whole unit circle is
// represented without the need for reflecting points between quadrants.
float		CircleX[CircleSegments];
float		CircleY[CircleSegments];

// ASFO:
FontSet VirtualDisplay::Font2D;
FontSet VirtualDisplay::Font3D;;
FontSet* VirtualDisplay::pFontSet = NULL;
/*
FontDataType FontData[NUM_FONT_RESOLUTIONS][256] = {0};
int fontSpacing[NUM_FONT_RESOLUTIONS];
int FontNum = 0;
int TotalFont = 3;
*/

/***************************************************************************\
	Setup the rendering context for this display
\***************************************************************************/
void VirtualDisplay::Setup( void )
{
	ShiAssert( !IsReady() );
	
	// Setup the default viewport
	SetViewport( -1.0f, 1.0f, 1.0f, -1.0f );

	// Setup the default offset and rotation
	CenterOriginInViewport();
	ZeroRotationAboutOrigin();

	// Initialize the unit circle array (in case someone else hasn't done it already)
	double	angle;
	int		entry;
	for (entry = 0; entry < CircleSegments; entry++) {
 		angle = (entry * CircleStep) * PI / 180.0f;

		CircleX[entry] =  (float)cos( angle );
		CircleY[entry] = -(float)sin( angle );	// Account for the y axis flip
	}

    type = DISPLAY_GENERAL;

	ready = TRUE;
	ForceAlpha=false;
}


/***************************************************************************\
	Shutdown the rendering context for this display
\***************************************************************************/
void VirtualDisplay::Cleanup( void )
{
	ShiAssert( IsReady() );
	ready = FALSE;
}


/***************************************************************************\
	Set the dimensions and location of the viewport.
\***************************************************************************/
void VirtualDisplay::SetViewport( float l, float t, float r, float b )
{
	static const float	E = 0.01f;	// Eplsion value to ensure we stay within our pixel limits

	//sfr: some cropping
	if (l < -1.0f){ l = -1.0f; }
	if (t > 1.0f){ t = 1.0f; }
	if (r > 1.0f){ r = 1.0f; }
	if (b < -1.0f){ b = -1.0f; }

	left = l, top = t, 
	right = r, bottom = b;

	scaleX = (r-l)*xRes*0.25f - E;
	if (scaleX < 0.0f) scaleX = 0.0f;
	shiftX = (l+1.0f+(r-l)*0.5f)*xRes*0.5f;
	shiftX += tLeft;	// ASSO: for adjusted RTT viewport

	scaleY = (t-b)*yRes*0.25f - E;
	if (scaleY < 0.0f) scaleY = 0.0f;
	shiftY = yRes - ((b+1.0f+(t-b)*0.5f)*yRes*0.5f);
	shiftY += tTop;		// ASSO: for adjusted RTT viewport
	

	// Now store our pixel space boundries
	// (top/right inclusive, bottom/left exclusive)
	topPixel	= viewportYtoPixel(-1.0f );
	bottomPixel	= viewportYtoPixel( 1.0f );
	leftPixel	= viewportXtoPixel(-1.0f );
	rightPixel	= viewportXtoPixel( 1.0f );

	ShiAssert( floor(topPixel)   >= 0.0f );
	ShiAssert( ceil(bottomPixel) >= 0.0f );
	ShiAssert( floor(leftPixel)  >= 0.0f );
	ShiAssert( ceil(rightPixel)  >= 0.0f );
	// ASSO: changed xRes to txRes and yRes to tyRes
	ShiAssert( floor(topPixel)	 <= tyRes );
	ShiAssert( ceil(bottomPixel) <= tyRes );	
	ShiAssert( floor(leftPixel)	 <= txRes );
	ShiAssert( ceil(rightPixel)  <= txRes );
	TheDXEngine.SetViewport((DWORD)leftPixel, (DWORD)topPixel, (DWORD)rightPixel, (DWORD)bottomPixel);
}


/***************************************************************************\
	Set the dimensions and location of the viewport.  This one assumes
	the inputs are relative to the currently set viewport.
\***************************************************************************/
void VirtualDisplay::SetViewportRelative( float l, float t, float r, float b )
{
	float	w = right - left;
	float	h = top - bottom;

	float	topSide    = top    - (1.0f - t)/2.0f * h;
	float	bottomSide = bottom + (1.0f + b)/2.0f * h;
	float	leftSide   = left   + (1.0f + l)/2.0f * w;
	float	rightSide  = right  - (1.0f - r)/2.0f * w;

	SetViewport( leftSide, topSide, rightSide, bottomSide );
}


/***************************************************************************\
	Return the current normalized screen space dimensions of the viewport
	on the drawing target buffer.
\***************************************************************************/
void VirtualDisplay::GetViewport( float *leftSide, float *topSide, float *rightSide, float *bottomSide )
{
	*leftSide	= left;
	*topSide	= top;
	*rightSide	= right;
	*bottomSide	= bottom;
}


/***************************************************************************\
	Compound the current offset with the new one requested.
\***************************************************************************/
void VirtualDisplay::AdjustOriginInViewport( float horizontal, float vertical )
{
	dmatrix.translationX += horizontal;
	dmatrix.translationY += vertical;
}


/***************************************************************************\
	Compound the current rotation with the new one requested.
\***************************************************************************/
void VirtualDisplay::AdjustRotationAboutOrigin( float angle )
{
	float temp;
	float cosAng = (float)cos( angle );
	float sinAng = (float)sin( angle );

    temp	   = dmatrix.rotation00 * cosAng - dmatrix.rotation01 * sinAng;
    dmatrix.rotation01 = dmatrix.rotation00 * sinAng + dmatrix.rotation01 * cosAng;
    dmatrix.rotation00 = temp;

    temp       = dmatrix.rotation10 * cosAng - dmatrix.rotation11 * sinAng;
    dmatrix.rotation11 = dmatrix.rotation10 * sinAng + dmatrix.rotation11 * cosAng;
    dmatrix.rotation10 = temp;
}

// save restore context
void VirtualDisplay::SaveDisplayMatrix(DisplayMatrix *dm)
{
	*dm = dmatrix;
}

void VirtualDisplay::RestoreDisplayMatrix(DisplayMatrix *dm)
{
	dmatrix = *dm;
}


/***************************************************************************\
	Put a pixel on the display.
\***************************************************************************/
void VirtualDisplay::Point( float x1, float y1 )
{
	float x, y;

	// Rotation and translate this point based on the current settings
	x = x1 * dmatrix.rotation00 + y1 * dmatrix.rotation01 + dmatrix.translationX;
	y = x1 * dmatrix.rotation10 + y1 * dmatrix.rotation11 + dmatrix.translationY;

	// Clipping
	if ((x >= -1.0f) && (x <= 1.0f) && (y <= 1.0f) && (y >= -1.0f)) {

		// Convert to pixel coordinates and draw the point on the display
		Render2DPoint( viewportXtoPixel( x ), viewportYtoPixel( -y ) );

	}
}


/***************************************************************************\
	Put a one pixel wide line on the display
\***************************************************************************/
void VirtualDisplay::Line( float x1, float y1, float x2, float y2 )
{
	float	x;
	int		clipFlag = ON_SCREEN;

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

	Render2DLine( viewportXtoPixel(  x1 ),
		          viewportYtoPixel( -y1 ),
				  viewportXtoPixel(  x2 ),
				  viewportYtoPixel( -y2 ));
}

void VirtualDisplay::Line(float x1, float y1, float x2, float y2, float width)
{
	Tpoint a, b, c, d, normal, s, e, cross;
	// MD -- 20041028: new function for drawing wider lines:
	// mrivers figured out how to draw a line of greater than 1 pixel width in the
	// following manner -- I'm just the scribe here...

//	vector2 start(x1,y1);
//	vector2 end(x2,y2);
	s.x = x1;
	s.y = y1;
	e.x = x2;
	e.y = y2;

	normal.x = e.x - s.x;
	normal.y = e.y - s.y;

//	normal.norm();
	float l = (float) sqrt(normal.x * normal.x + normal.y * normal.y);
	if (l > 0.0000001)
	{
		normal.x /= l;
		normal.y /= l;
	}

//	vector2 cross(-normal.y,normal.x);
	cross.x = -normal.y;
	cross.y = normal.x;
	cross.x *= width / 2;
	cross.y *= width / 2;

// RV - Biker - Do the trigonometry properly
//	vector2 a=start+cross;
	a.x = s.x + cross.x;
	a.y = s.y + cross.y;

//	vector2 b=start-cross;
	//b.x = s.x + cross.x;
	//b.y = s.y + cross.y;
	b.x = s.x - cross.x;
	b.y = s.y - cross.y;

//	vector2 c=end+cross;
	c.x = e.x + cross.x;
	c.y = e.y + cross.y;

//	vector2 d=end-cross;
	//d.x = e.x + cross.x;
	//d.y = e.y + cross.y;
	d.x = e.x - cross.x;
	d.y = e.y - cross.y;

	Tri(a.x, a.y, b.x, b.y, c.x, c.y);
	Tri(c.x, c.y, d.x, d.y, b.x, b.y);

}

/***************************************************************************\
	Put a triangle on the display.  It is not filled (for now at least)
\***************************************************************************/
void VirtualDisplay::Tri( float x1, float y1, float x2, float y2, float x3, float y3 )
{
float	x;

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

   Render2DTri (
      viewportXtoPixel(x1), viewportYtoPixel(-y1),
      viewportXtoPixel(x2), viewportYtoPixel(-y2),
      viewportXtoPixel(x3), viewportYtoPixel(-y3));
}

void VirtualDisplay::Render2DTri( float x1, float y1, float x2, float y2, float x3, float y3 )
{
	Render2DLine( x1, y1, x2, y2 );
	Render2DLine( x2, y2, x3, y3 );
	Render2DLine( x3, y3, x1, y1 );
}


/***************************************************************************\
	Draw a circle in the viewport.  The radius is given indpendently
	in the x and y direction.
\***************************************************************************/
void VirtualDisplay::Oval (float x, float y, float xRadius, float yRadius)
{
	int entry;

	float x1, y1;
	float x2, y2;

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
void VirtualDisplay::OvalArc (float x, float y, float xRadius, float yRadius, float start, float stop)
{
	int		entry, startEntry, stopEntry;

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
	Put a mono-colored string of text on the display.
	(The location given is used as the lower left corner of the text)
\***************************************************************************/
void VirtualDisplay::TextLeft( float x1, float y1, const char *string, int boxed )
{
	float	x, y;

	// Rotation and translate this point based on the current settings
	x = x1 * dmatrix.rotation00 + y1 * dmatrix.rotation01 + dmatrix.translationX;
	y = x1 * dmatrix.rotation10 + y1 * dmatrix.rotation11 + dmatrix.translationY;

	// Convert from viewport coordiants to screen space and draw the string
	ScreenText( viewportXtoPixel(x), viewportYtoPixel(-y), string, boxed );
}

void VirtualDisplay::TextLeftVertical( float x1, float y1, const char *string, int boxed )
{
	float	x, y;

	// Rotation and translate this point based on the current settings
	x = viewportXtoPixel(x1 * dmatrix.rotation00 + y1 * dmatrix.rotation01 + dmatrix.translationX);
	y = viewportYtoPixel( -x1 * dmatrix.rotation10 - y1 * dmatrix.rotation11 - dmatrix.translationY );

	y -= ScreenTextHeight() / 2;

	// Convert from viewport coordiants to screen space and draw the string
	ScreenText( x, y, string, boxed );
}


/***************************************************************************\
	Put a mono-colored string of text on the display.
	(The location given is used as the lower right corner of the text)
\***************************************************************************/
void VirtualDisplay::TextRight( float x1, float y1, const char *string, int boxed )
{
	float	xPixel, yPixel;

	// Rotation and translate this point based on the current settings
	// Convert from viewport coordiants to screen space
	xPixel = viewportXtoPixel(  x1 * dmatrix.rotation00 + y1 * dmatrix.rotation01 + dmatrix.translationX );
	yPixel = viewportYtoPixel( -x1 * dmatrix.rotation10 - y1 * dmatrix.rotation11 - dmatrix.translationY );

	// Adjust our starting point in screen space to get proper alignment
	xPixel -= ScreenTextWidth (string);

	// Draw the string on the screen
	ScreenText( xPixel, yPixel, string, boxed );
}

void VirtualDisplay::TextRightVertical( float x1, float y1, const char *string, int boxed )
{
	float	xPixel, yPixel;

	// Rotation and translate this point based on the current settings
	// Convert from viewport coordiants to screen space
	xPixel = viewportXtoPixel(  x1 * dmatrix.rotation00 + y1 * dmatrix.rotation01 + dmatrix.translationX );
	yPixel = viewportYtoPixel( -x1 * dmatrix.rotation10 - y1 * dmatrix.rotation11 - dmatrix.translationY );

	// Adjust our starting point in screen space to get proper alignment
	xPixel -= ScreenTextWidth (string);
	yPixel -= ScreenTextHeight() / 2;

	// Draw the string on the screen
	ScreenText( xPixel, yPixel, string, boxed );
}


/***************************************************************************\
	Put a mono-colored string of text on the display.
	(The location given is used as the horizontal center and vertical lower
	 edge of the text)
\***************************************************************************/
void VirtualDisplay::TextCenter( float x1, float y1, const char *string, int boxed )
{
	float	xPixel, yPixel;

	// Rotation and translate this point based on the current settings
	// Convert from viewport coordiants to screen space
	xPixel = viewportXtoPixel(  x1 * dmatrix.rotation00 + y1 * dmatrix.rotation01 + dmatrix.translationX );
	yPixel = viewportYtoPixel( -x1 * dmatrix.rotation10 - y1 * dmatrix.rotation11 - dmatrix.translationY );

	// Adjust our starting point in screen space to get proper alignment
	xPixel -= ScreenTextWidth (string) / 2;

	// Draw the string on the screen
	ScreenText( xPixel, yPixel, string, boxed );
}


/***************************************************************************\
	Put a mono-colored string of text on the display.
	(The location given is used as the horizontal center and vertical lower
	 edge of the text)
\***************************************************************************/
void VirtualDisplay::TextCenterVertical( float x1, float y1, const char *string, int boxed )
{
	float	xPixel, yPixel;

	// Rotation and translate this point based on the current settings
	// Convert from viewport coordiants to screen space
	xPixel = viewportXtoPixel(  x1 * dmatrix.rotation00 + y1 * dmatrix.rotation01 + dmatrix.translationX );
	yPixel = viewportYtoPixel( -x1 * dmatrix.rotation10 - y1 * dmatrix.rotation11 - dmatrix.translationY );

	// Adjust our starting point in screen space to get proper alignment
	xPixel -= ScreenTextWidth (string) / 2;
	yPixel -= ScreenTextHeight() / 2;

	// Draw the string on the screen
	ScreenText( xPixel, yPixel, string, boxed );
}


/***************************************************************************\
	Print the string on multiple lines.  h,v is the starting point.
	spacing is the line height and width is the line width.
	THIS ASSUMES that no single word is longer than will fit in the
	specified width.
\***************************************************************************/
int VirtualDisplay::TextWrap( float h, float v, const char *s, float spacing, float width )
{
	char *string = strdup(s);
	int pixelsLeft;
	char *lineBreak;
	char prevChar;
	int line = 0;

	// While we have more to display
	while (*string) {

		// We start from the end of the string
		lineBreak = string+strlen(string);
		ShiAssert( *lineBreak == '\0' );	
		prevChar = '\0';
		pixelsLeft = FloatToInt32(width*scaleX) - ScreenTextWidth( string );

		// While we need to shorten the string...
		while (pixelsLeft < 0) {

			// Take the synthetic NULL back out
			*lineBreak = prevChar;

			// Find the next word to slice off
			do {
				lineBreak--;
				ShiAssert( lineBreak >= string );
			} while (*lineBreak != ' ');
			do {
				lineBreak--;
				ShiAssert( lineBreak >= string );
			} while (*lineBreak == ' ');
			lineBreak++;	// Step back to the first space after the word

			// Save the existing character, then insert NULL terminator
			prevChar = *lineBreak;
			*lineBreak = '\0';

			// Recompute the length of the trimmed string
			pixelsLeft = FloatToInt32(width*scaleX) - ScreenTextWidth( string );
		}

		// Now print the trimmed string
		TextLeft( h, v-line*spacing, string );

		// Take the synthetic NULL back out and advance
		*lineBreak = prevChar;
		string = lineBreak;
		line++;

		// Skip any extra white space
		while (*string == ' ') {
			string++;
		}
	}


	free(string);
	return line;
}


/***************************************************************************\
	Get the width of a text string about to be placed onto the display
\***************************************************************************/
int VirtualDisplay::ScreenTextWidth(const char *string)
{
#ifndef USE_TEXTURE_FONT
	unsigned	num;
	int			width;

	width = 0;

	while (*string)
	{
		num = FontLUT[*(unsigned char *)string];
		ShiAssert( num < FontLength );

		width += Font[num][8] + 1;

		string++;
	}

	return width - 1;
#else
	int width = 0;

	while (*string)
	{
		width += FloatToInt32(pFontSet->fontData[pFontSet->fontNum][*string].pixelWidth);
		string++;
	}

	return width;
#endif
}

//JAM 22Dec03 - DELME
/***************************************************************************\
	Get the width of a text string about to be placed onto the display
\***************************************************************************/
int VirtualDisplay::ScreenTextHeight(void)
{
#ifndef USE_TEXTURE_FONT
	// Right now we have only one font.  It draws 6 pixels high with one
	// pixel each above and below for spacing and reverse video effects.
	return 8;
#else
	return FloatToInt32(pFontSet->fontData[pFontSet->fontNum][32].pixelHeight);
#endif
}


/***************************************************************************\
	This is the font data used to draw text.  For now, it uses an 8x8 cell.
	NOTE:  The last number in each character it the actual width of the
	character for proportional spacing.
\***************************************************************************/

static unsigned char Space[] = {
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 3
};
static unsigned char OpenParen[] = {
	0x00,0x00,0x40,0x80,0x80,0x80,0x40,0x00, 2
};
static unsigned char CloseParen[] = {
	0x00,0x00,0x80,0x40,0x40,0x40,0x80,0x00, 2
};
static unsigned char Asterisk[] = {
	0x00,0x00,0xe0,0xa0,0xe0,0x00,0x00,0x00, 3
};												// Note: This is the slot for the ascii asterisk, but I'm mapping a degree symbol to it.
static unsigned char Plus[] = {
	0x00,0x00,0x40,0xe0,0x00,0xe0,0x40,0x00, 3
};												// Note: This is the slot for the ascii plus symbol, but I'm mapping a 'roll' symbol to it.
static unsigned char Comma[] = {
	0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x80, 2
};
static unsigned char Minus[] = {
	0x00,0x00,0x00,0x00,0xe0,0x00,0x00,0x00, 3
};
static unsigned char Period[] = {
	0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x00, 2
};
static unsigned char Slash[] = {
	0x00,0x00,0x20,0x20,0x40,0x80,0x80,0x00, 3
};
static unsigned char Number0[] = {
	0x00,0x00,0xe0,0xa0,0xa0,0xa0,0xe0,0x00, 3
};
static unsigned char Number1[] = {
	0x00,0x00,0x40,0xc0,0x40,0x40,0xe0,0x00, 3
};
static unsigned char Number2[] = {
	0x00,0x00,0xe0,0x20,0xe0,0x80,0xe0,0x00, 3
};
static unsigned char Number3[] = {
	0x00,0x00,0xe0,0x20,0x60,0x20,0xe0,0x00, 3
};
static unsigned char Number4[] = {
	0x00,0x00,0x80,0xa0,0xe0,0x20,0x20,0x00, 3
};
static unsigned char Number5[] = {
	0x00,0x00,0xe0,0x80,0xc0,0x20,0xc0,0x00, 3
};
static unsigned char Number6[] = {
    0x00,0x00,0x80,0x80,0xe0,0xa0,0xe0,0x00, 3
};
static unsigned char Number7[] = {
    0x00,0x00,0xe0,0x20,0x20,0x20,0x20,0x00, 3
};
static unsigned char Number8[] = {
    0x00,0x00,0xe0,0xa0,0xe0,0xa0,0xe0,0x00, 3
};
static unsigned char Number9[] = {
    0x00,0x00,0xe0,0xa0,0xe0,0x20,0x20,0x00, 3
};
static unsigned char Colon[] = {
    0x00,0x00,0x00,0x80,0x00,0x80,0x00,0x00, 1
};
static unsigned char SemiColon[] = {
    0x00,0x00,0x00,0x40,0x00,0x40,0x40,0x80, 2
};
static unsigned char Less[] = {
    0x00,0x00,0x20,0x40,0x80,0x40,0x20,0x00, 3
};
static unsigned char Equal[] = {
    0x00,0x00,0x00,0xe0,0x00,0xe0,0x00,0x00, 3
};
static unsigned char More[] = {
    0x00,0x00,0x80,0x40,0x20,0x40,0x80,0x00, 3
};
static unsigned char Quest[] = {
    0x40,0x00,0xc0,0x20,0x40,0x00,0x40,0x00, 3
};
static unsigned char Each[] = {
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 3
};
static unsigned char LetterA[] = {
    0x00,0x00,0x40,0xa0,0xa0,0xe0,0xa0,0x00, 3
};
static unsigned char LetterB[] = {
    0x00,0x00,0xc0,0xa0,0xc0,0xa0,0xc0,0x00, 3
};
static unsigned char LetterC[] = {
    0x00,0x00,0x40,0xa0,0x80,0xa0,0x40,0x00, 3
};
static unsigned char LetterD[] = {
    0x00,0x00,0xc0,0xa0,0xa0,0xa0,0xc0,0x00, 3
};
static unsigned char LetterE[] = {
    0x00,0x00,0xe0,0x80,0xc0,0x80,0xe0,0x00, 3
};
static unsigned char LetterF[] = {
    0x00,0x00,0xe0,0x80,0xc0,0x80,0x80,0x00, 3
};
static unsigned char LetterG[] = {
    0x00,0x00,0x60,0x80,0xa0,0xa0,0x60,0x00, 3
};
static unsigned char LetterH[] = {
    0x00,0x00,0xa0,0xa0,0xe0,0xa0,0xa0,0x00, 3
};
static unsigned char LetterI[] = {
    0x00,0x00,0xe0,0x40,0x40,0x40,0xe0,0x00, 3
};
static unsigned char LetterJ[] = {
    0x00,0x00,0x20,0x20,0x20,0xa0,0x40,0x00, 3
};
static unsigned char LetterK[] = {
    0x00,0x00,0xa0,0xa0,0xc0,0xa0,0xa0,0x00, 3
};
static unsigned char LetterL[] = {
    0x00,0x00,0x80,0x80,0x80,0x80,0xe0,0x00, 3
};
static unsigned char LetterM[] = {
    0x00,0x00,0x88,0xd8,0xa8,0xa8,0x88,0x00, 5
};
static unsigned char LetterN[] = {
    0x00,0x00,0x90,0xd0,0xb0,0x90,0x90,0x00, 4
};
static unsigned char LetterO[] = {
    0x00,0x00,0x60,0x90,0x90,0x90,0x60,0x00, 4
};
static unsigned char LetterP[] = {
    0x00,0x00,0xe0,0xa0,0xe0,0x80,0x80,0x00, 3
};
static unsigned char LetterQ[] = {
    0x00,0x00,0x60,0x90,0x90,0xa0,0xd0,0x00, 4
};
static unsigned char LetterR[] = {
    0x00,0x00,0xc0,0xa0,0xc0,0xa0,0xa0,0x00, 3
};
static unsigned char LetterS[] = {
    0x00,0x00,0x60,0x80,0x40,0x20,0xc0,0x00, 3
};
static unsigned char LetterT[] = {
    0x00,0x00,0xe0,0x40,0x40,0x40,0x40,0x00, 3
};
static unsigned char LetterU[] = {
    0x00,0x00,0xa0,0xa0,0xa0,0xa0,0xe0,0x00, 3
};
static unsigned char LetterV[] = {
    0x00,0x00,0xa0,0xa0,0xa0,0xe0,0x40,0x00, 3
};
static unsigned char LetterW[] = {
    0x00,0x00,0x88,0x88,0xa8,0xa8,0x50,0x00, 5
};
static unsigned char LetterX[] = {
    0x00,0x00,0xa0,0xa0,0x40,0xa0,0xa0,0x00, 3
};
static unsigned char LetterY[] = {
    0x00,0x00,0xa0,0xa0,0xe0,0x40,0x40,0x00, 3
};
static unsigned char LetterZ[] = {
    0x00,0x00,0xe0,0x20,0x40,0x80,0xe0,0x00, 3
};
static unsigned char Apostrophe[] = {
	0x00,0x00,0x40,0x80,0x00,0x00,0x00,0x00, 2
};
static unsigned char LetterAumlaut[] = {
    0x00,0xa0,0x40,0xa0,0xa0,0xe0,0xa0,0x00, 3
};
static unsigned char LetterOumlaut[] = {
    0x00,0x90,0x60,0x90,0x90,0x90,0x60,0x00, 4
};
static unsigned char LetterUumlaut[] = {
    0x00,0xa0,0x00,0xa0,0xa0,0xa0,0xe0,0x00, 3
};
static unsigned char LetterBeta[] = {
    0x00,0x00,0xf0,0x90,0xb8,0x88,0xf8,0x00, 5
};
static unsigned char Degree[] = {
	0x00,0x00,0xe0,0xa0,0xe0,0x00,0x00,0x00, 3
};
static unsigned char Mu[] = {
	0x00,0x00,0x50,0x50,0x50,0x60,0x80,0x00, 4
};
static unsigned char Exclaim[] = {
	0x00,0x00,0x80,0x80,0x80,0x00,0x80,0x00, 1
};
static unsigned char Quote[] = {
	0x00,0x00,0xa0,0xa0,0x00,0x00,0x00,0x00, 3
};
static unsigned char And[] = {
	0x00,0x00,0x40,0xa0,0x40,0xa0,0x50,0x00, 4
};
static unsigned char LetterAaccent[] = {
    0x10,0x20,0x40,0x60,0x90,0xf0,0x90,0x00, 4
};
static unsigned char LetterAbackaccent[] = {
    0x40,0x20,0x10,0x60,0x90,0xf0,0x90,0x00, 4
};
static unsigned char LetterAsquiggle[] = {
    0x50,0xa0,0x00,0x60,0x90,0xf0,0x90,0x00, 4
};
static unsigned char LetterAhat[] = {
    0x60,0x90,0x00,0x60,0x90,0xf0,0x90,0x00, 4
};
static unsigned char LetterEaccent[] = {
    0x20,0x40,0xe0,0x80,0xc0,0x80,0xe0,0x00, 3
};
static unsigned char LetterEhat[] = {
    0x40,0xa0,0xe0,0x80,0xc0,0x80,0xe0,0x00, 3
};
static unsigned char LetterIaccent[] = {
    0x20,0x40,0xe0,0x40,0x40,0x40,0xe0,0x00, 3
};
static unsigned char LetterNsquiggle[] = {
    0x50,0xa0,0x00,0x90,0xd0,0xb0,0x90,0x00, 4
};
static unsigned char LetterOsquiggle[] = {
    0x50,0xa0,0x00,0xf0,0x90,0x90,0xf0,0x00, 4
};
static unsigned char LetterOaccent[] = {
    0x10,0x20,0x40,0xf0,0x90,0x90,0xf0,0x00, 4
};
static unsigned char LetterUaccent[] = {
    0x10,0x20,0x40,0x90,0x90,0x90,0xf0,0x00, 4
};
static unsigned char LetterCstem[] = {
    0x00,0x00,0xe0,0x80,0x80,0xe0,0x40,0xc0, 3
};


const unsigned char *VirtualDisplay::Font[] = {
	Space, OpenParen, CloseParen, Asterisk, Plus,				/* Index 0 through 4 */
	Comma, Minus, Period, Slash,								/* Index 5 through 8 */
	Number0,Number1,Number2,Number3,Number4,					/* Index 9 through 13 */
	Number5,Number6,Number7,Number8,Number9,					/* Index 14 through 18 */
	Colon, SemiColon,Less,Equal,More,Quest,Each,				/* Index 19 through 25 */
	LetterA,LetterB,LetterC,LetterD,LetterE,LetterF,LetterG,	/* Index 26 through 32 */
	LetterH,LetterI,LetterJ,LetterK,LetterL,LetterM,LetterN,	/* Index 33 through 39 */
	LetterO,LetterP,LetterQ,LetterR,LetterS,LetterT,LetterU,	/* Index 40 through 46 */
	LetterV,LetterW,LetterX,LetterY,LetterZ, Apostrophe,		/* Index 47 through 52 */
	LetterAumlaut,LetterOumlaut,LetterUumlaut,LetterBeta,		/* Index 53 through 56 */
	Degree,	Mu,	Exclaim, Quote, And,							/* Index 57 through 61 */
	LetterAaccent,		// 62
	LetterAsquiggle,	// 63
	LetterEaccent,		// 64
	LetterEhat,			// 65
	LetterIaccent,		// 66
	LetterNsquiggle,	// 67
	LetterOsquiggle,	// 68
	LetterOaccent,		// 69
	LetterUaccent,		// 70
	LetterCstem,		// 71
	LetterAhat,			// 72
	LetterAbackaccent,	// 73
};

const unsigned int  VirtualDisplay::FontLength = sizeof(Font)/sizeof(Font[0]);

const unsigned char VirtualDisplay::FontLUT[256] =
{
	  0, /* ASCII   0 */	  0, /* ASCII   1 */	  0, /* ASCII   2 */      0,  /* ASCII   3 */
	  0, /* ASCII   4 */	  0, /* ASCII   5 */	  0, /* ASCII   6 */      0,  /* ASCII   7 */
	  0, /* ASCII   8 */	  0, /* ASCII   9 */	  0, /* ASCII  10 */      0,  /* ASCII  11 */
	  0, /* ASCII  12 */	  0, /* ASCII  13 */	  0, /* ASCII  14 */      0,  /* ASCII  15 */
	  0, /* ASCII  16 */	  0, /* ASCII  17 */	  0, /* ASCII  18 */      0,  /* ASCII  19 */
	  0, /* ASCII  20 */	  0, /* ASCII  21 */	  0, /* ASCII  22 */      0,  /* ASCII  23 */
	  0, /* ASCII  24 */	  0, /* ASCII  25 */	  0, /* ASCII  26 */      0,  /* ASCII  27 */
	  0, /* ASCII  28 */	  0, /* ASCII  29 */	  0, /* ASCII  30 */      0,  /* ASCII  31 */
	  0, /* ASCII  32 */	 59, /* ASCII  33 */	 60, /* ASCII  34 */      0,  /* ASCII  35 */
	  0, /* ASCII  36 */	  0, /* ASCII  37 */	 61, /* ASCII  38 */     52,  /* ASCII  39 */
	  1, /* ASCII  40 */	  2, /* ASCII  41 */	  3, /* ASCII  42 */      4,  /* ASCII  43 */
	  5, /* ASCII  44 */	  6, /* ASCII  45 */	  7, /* ASCII  46 */      8,  /* ASCII  47 */
	  9, /* ASCII  48 */	 10, /* ASCII  49 */	 11, /* ASCII  50 */     12,  /* ASCII  51 */
	 13, /* ASCII  52 */	 14, /* ASCII  53 */	 15, /* ASCII  54 */     16,  /* ASCII  55 */
	 17, /* ASCII  56 */	 18, /* ASCII  57 */	 19, /* ASCII  58 */     20,  /* ASCII  59 */
	 21, /* ASCII  60 */	 22, /* ASCII  61 */	 23, /* ASCII  62 */     24,  /* ASCII  63 */
	 25, /* ASCII  64 */	 26, /* ASCII  65 */	 27, /* ASCII  66 */     28,  /* ASCII  67 */
	 29, /* ASCII  68 */	 30, /* ASCII  69 */	 31, /* ASCII  70 */     32,  /* ASCII  71 */
	 33, /* ASCII  72 */	 34, /* ASCII  73 */	 35, /* ASCII  74 */     36,  /* ASCII  75 */
	 37, /* ASCII  76 */	 38, /* ASCII  77 */	 39, /* ASCII  78 */     40,  /* ASCII  79 */
	 41, /* ASCII  80 */	 42, /* ASCII  81 */	 43, /* ASCII  82 */     44,  /* ASCII  83 */
	 45, /* ASCII  84 */	 46, /* ASCII  85 */	 47, /* ASCII  86 */     48,  /* ASCII  87 */
	 49, /* ASCII  88 */	 50, /* ASCII  89 */	 51, /* ASCII  90 */     52,  /* ASCII  91 */
	  0, /* ASCII  92 */	  0, /* ASCII  93 */	  0, /* ASCII  94 */      0,  /* ASCII  95 */
	  0, /* ASCII  96 */	 26, /* ASCII  97 */	 27, /* ASCII  98 */     28,  /* ASCII  99 */
	 29, /* ASCII 100 */	 30, /* ASCII 101 */	 31, /* ASCII 102 */	 32,  /* ASCII 103 */
	 33, /* ASCII 104 */	 34, /* ASCII 105 */	 35, /* ASCII 106 */	 36,  /* ASCII 107 */
	 37, /* ASCII 108 */	 38, /* ASCII 109 */	 39, /* ASCII 110 */	 40,  /* ASCII 111 */
	 41, /* ASCII 112 */	 42, /* ASCII 113 */	 43, /* ASCII 114 */	 44,  /* ASCII 115 */
	 45, /* ASCII 116 */	 46, /* ASCII 117 */	 47, /* ASCII 118 */	 48,  /* ASCII 119 */
	 49, /* ASCII 120 */	 50, /* ASCII 121 */	 51, /* ASCII 122 */	  0,  /* ASCII 123 */
	  0, /* ASCII 124 */	  0, /* ASCII 125 */	  0, /* ASCII 126 */	  0,  /* ASCII 127 */
	 28, /* ASCII 128 */	 46, /* ASCII 129 */	 30, /* ASCII 130 */	 26,  /* ASCII 131 */
	 26, /* ASCII 132 */	 26, /* ASCII 133 */	 26, /* ASCII 134 */	 28,  /* ASCII 135 */
	 30, /* ASCII 136 */	 30, /* ASCII 137 */	 30, /* ASCII 138 */	 37,  /* ASCII 139 */
	 37, /* ASCII 140 */	 37, /* ASCII 141 */	 26, /* ASCII 142 */	 26,  /* ASCII 143 */
	 30, /* ASCII 144 */	 26, /* ASCII 145 */	 26, /* ASCII 146 */	 40,  /* ASCII 147 */
	 40, /* ASCII 148 */	 40, /* ASCII 149 */	 46, /* ASCII 150 */	 46,  /* ASCII 151 */
	 50, /* ASCII 152 */	 40, /* ASCII 153 */	 46, /* ASCII 154 */	  0,  /* ASCII 155 */
	  0, /* ASCII 156 */	  0, /* ASCII 157 */	  0, /* ASCII 158 */	  0,  /* ASCII 159 */
	 26, /* ASCII 160 */	 37, /* ASCII 161 */	 40, /* ASCII 162 */	 46,  /* ASCII 163 */
	 39, /* ASCII 164 */	 39, /* ASCII 165 */	  0, /* ASCII 166 */	  0,  /* ASCII 167 */
	  0, /* ASCII 168 */	  0, /* ASCII 169 */	  0, /* ASCII 170 */	  0,  /* ASCII 171 */
	  0, /* ASCII 172 */	  0, /* ASCII 173 */	  0, /* ASCII 174 */	  0,  /* ASCII 175 */
	 57, /* ASCII 176 */	  0, /* ASCII 177 */	  0, /* ASCII 178 */	  0,  /* ASCII 179 */
	  0, /* ASCII 180 */	 58, /* ASCII 181 */	  0, /* ASCII 182 */	  0,  /* ASCII 183 */
	  0, /* ASCII 184 */	  0, /* ASCII 185 */	  0, /* ASCII 186 */	  0,  /* ASCII 187 */
	  0, /* ASCII 188 */	  0, /* ASCII 189 */	  0, /* ASCII 190 */	  0,  /* ASCII 191 */
	 73, /* ASCII 192 */	 62, /* ASCII 193 */	 72, /* ASCII 194 */	 63,  /* ASCII 195 */
	 53, /* ASCII 196 */	 26, /* ASCII 197 */	 26, /* ASCII 198 */	 71,  /* ASCII 199 */
	 30, /* ASCII 200 */	 64, /* ASCII 201 */	 65, /* ASCII 202 */	 30,  /* ASCII 203 */
	 34, /* ASCII 204 */	 66, /* ASCII 205 */	 34, /* ASCII 206 */	 34,  /* ASCII 207 */
	 29, /* ASCII 208 */	 67, /* ASCII 209 */	 40, /* ASCII 210 */	 69,  /* ASCII 211 */
	 40, /* ASCII 212 */	 68, /* ASCII 213 */	 54, /* ASCII 214 */	  0,  /* ASCII 215 */
	 40, /* ASCII 216 */	 46, /* ASCII 217 */	 70, /* ASCII 218 */	 46,  /* ASCII 219 */
	 55, /* ASCII 220 */	  0, /* ASCII 221 */	  0, /* ASCII 222 */	 56,  /* ASCII 223 */
	 73, /* ASCII 224 */	 62, /* ASCII 225 */	 72, /* ASCII 226 */	 63,  /* ASCII 227 */
	 53, /* ASCII 228 */	 26, /* ASCII 229 */	 26, /* ASCII 230 */	 71,  /* ASCII 231 */
	 30, /* ASCII 232 */	 64, /* ASCII 233 */	 65, /* ASCII 234 */	 30,  /* ASCII 235 */
	 37, /* ASCII 236 */	 66, /* ASCII 237 */	 37, /* ASCII 238 */	 37,  /* ASCII 239 */
	  0, /* ASCII 240 */	 67, /* ASCII 241 */	 40, /* ASCII 242 */	 69,  /* ASCII 243 */
	 40, /* ASCII 244 */	 68, /* ASCII 245 */	 54, /* ASCII 246 */	 40,  /* ASCII 247 */
	  0, /* ASCII 248 */	 46, /* ASCII 249 */	 70, /* ASCII 250 */	 46,  /* ASCII 251 */
	 55, /* ASCII 252 */	 50, /* ASCII 253 */	  0, /* ASCII 254 */	 50,  /* ASCII 255 */
};


unsigned char VirtualDisplay::InvFont[VirtualDisplay::FontLength][8];


void VirtualDisplay::InitializeFonts( void )
{
	int c, r;

	for (c=0; c<VirtualDisplay::FontLength; c++) {
		// Shift each row right one to make room for the edging
		for (r=0; r<8; r++) {
			InvFont[c][r] = (unsigned char)(~(Font[c][r] >> 1));
		}
	}
}

void VirtualDisplay::SetFont(int newfont)
{ 
    ShiAssert(newfont >= 0 && newfont < NUM_FONT_RESOLUTIONS);
    if (newfont >= pFontSet->totalFont) 
	newfont = pFontSet->totalFont-1; 
    else pFontSet->fontNum = newfont;
}
// ASSO: BEGIN ---------------------------------------------------------------------------------------------
bool VirtualDisplay::SetupRttTarget( int tXres_, int tYres_, int tBpp_ )
{
	// Initialize the shared renderTexture only once
	if( !renderTexture )
	{
		int tBpp;
		switch( tBpp_ )
		{
		  case 16: 
			tBpp = MPR_TI_RGB16;
			break;
		  case 24: 
			tBpp = MPR_TI_RGB24;
			break;
		  case 32: 
			tBpp = MPR_TI_ARGB32;
			break;
		  default:
			tBpp = MPR_TI_ARGB32;
		}

		renderTexture = new TextureHandle;
		renderTexture->Create( "RttTarget", tBpp, 0, tXres_, tYres_, 
				TextureHandle::FLAG_RENDERTARGET | TextureHandle::FLAG_HINT_DYNAMIC );

		return true;
	}
	return false;
}

bool VirtualDisplay::CleanupRttTarget()
{
	if( renderTexture ) {
		delete renderTexture;
		renderTexture = NULL;
		return true;
	}
	return false;
}

void VirtualDisplay::SetRttCanvas( Tpoint* ul_, Tpoint* ur_, Tpoint* ll_, char blendMode_, float alpha_ )
{
	canUL = *ul_;
	canUR = *ur_;
	canLL = *ll_;
	
	switch( blendMode_ )
	{
	  case 'a':
	  	rttBlendMode = STATE_ALPHA_TEXTURE_GOURAUD;
		break;
	  case 'c':
		rttBlendMode = STATE_CHROMA_TEXTURE_GOURAUD2;
		break;
	  case 'g':
		rttBlendMode = STATE_TEXTURE_GOURAUD;
		break;
	  case 't':
		rttBlendMode = STATE_TEXTURE;
		break;
	  default:
		rttBlendMode = STATE_TEXTURE_GOURAUD;
	}
	
	rttAlpha = alpha_;
}

void VirtualDisplay::SetRttRect( int tLeft_, int tTop_, int tRight_, int tBottom_, bool rt_ )
{
	//oldXRes = xRes;
	//oldYRes = yRes;
	tLeft = tLeft_;
	tTop = tTop_;
	tRight = tRight_;
	tBottom = tBottom_;

	xRes = tRight - tLeft;
	yRes = tBottom - tTop;

	if( rt_ && renderTexture) {
		txRes = renderTexture->m_nActualWidth;
		tyRes = renderTexture->m_nActualHeight;
		//context.SetViewportAbs( 0, 0, renderTexture->m_nActualWidth, renderTexture->m_nActualHeight );
	}
	else {
		xRes = txRes = image->targetXres();
		yRes = tyRes = image->targetYres();
		tLeft = 0;
		tTop = 0;
		tRight = xRes;
		tBottom = yRes;
		//context.SetViewportAbs( 0, 0, xRes, yRes );
	}
	context.ZeroViewport();
}

void VirtualDisplay::StartRtt( Render3D* r3d_ )
{
	r3d = r3d_;
	//oldXRes = xRes;
	//oldYRes = yRes;
	GetViewport( &oldLeft, &oldTop, &oldRight, &oldBottom ); // save the current viewport
	oldTarget = context.m_pRenderTarget;
	context.m_pRenderTarget = renderTexture->m_pDDS;
	context.m_pCtxDX->SetRenderTarget( context.m_pRenderTarget );
	SetRttRect( 0, 0, renderTexture->m_nActualWidth, renderTexture->m_nActualHeight );
	SetViewport(-1.0f, 1.0f, 1.0f, -1.0f);
}

void VirtualDisplay::FinishRtt()
{
	context.m_pRenderTarget = image->targetSurface();
	context.m_pCtxDX->SetRenderTarget( context.m_pRenderTarget );
	SetRttRect( 0, 0, 0, 0, false );
	//SetViewport(-1.0f, 1.0f, 1.0f, -1.0f);
	SetViewport( oldLeft, oldTop, oldRight, oldBottom );	// restore viewport	
}

void VirtualDisplay::AdjustRttViewport()
{
	context.m_pRenderTarget = renderTexture->m_pDDS;
	//context.SetViewportAbs( 0, 0, renderTexture->m_nActualWidth, renderTexture->m_nActualHeight );
	SetViewport(-1.0f, 1.0f, 1.0f, -1.0f);
}

void VirtualDisplay::ResetRttViewport()
{
	// ASSO: reset adjusted RTT viewport
	context.m_pRenderTarget = image->targetSurface();
	context.RestoreState( STATE_SOLID );
	
	xRes = txRes = image->targetXres();
	yRes = tyRes = image->targetYres();
	tLeft = 0;
	tTop = 0;
	tRight = xRes;
	tBottom = yRes;
	
	// Setup the default viewport
	SetViewport( -1.0f, 1.0f, 1.0f, -1.0f );

	// Setup the default offset and rotation
	CenterOriginInViewport();
	ZeroRotationAboutOrigin();
}



void VirtualDisplay::DrawRttQuad()
{
	r3d->context.RestoreState( rttBlendMode );
	r3d->context.SelectTexture1( (DWORD)renderTexture );

	Tpoint os;
	ThreeDVertex v0,v1,v2,v3;
	
	os.x = canUL.x;
	os.y = canUL.y;
	os.z = canUL.z;
	r3d->TransformPoint(&os, &v0);	
	
	os.x = canUR.x;
	os.y = canUR.y;
	os.z = canUR.z;
	r3d->TransformPoint(&os, &v1);	
		
	os.x =  canLL.x;
	os.y =  canUR.y;
	os.z =  canLL.z;
	r3d->TransformPoint(&os, &v2);	
		
	os.x = canLL.x; 
	os.y = canLL.y; 
	os.z = canLL.z;
	r3d->TransformPoint(&os, &v3);	

	v0.u = (float)tLeft / (float)renderTexture->m_nActualWidth;  
	v0.v = (float)tTop / (float)renderTexture->m_nActualHeight; 
	v0.q = v0.csZ * Q_SCALE;
	
	v1.u = (float)tRight / (float)renderTexture->m_nActualWidth; 
	v1.v = (float)tTop / (float)renderTexture->m_nActualHeight; 
	v1.q = v1.csZ * Q_SCALE;
	
	v2.u = (float)tRight / (float)renderTexture->m_nActualWidth; 
	v2.v = (float)tBottom / (float)renderTexture->m_nActualHeight; 
	v2.q = v2.csZ * Q_SCALE;
	
	v3.u = (float)tLeft / (float)renderTexture->m_nActualWidth; 
	v3.v = (float)tBottom / (float)renderTexture->m_nActualHeight; 
	v3.q = v3.csZ * Q_SCALE;

	v0.r = v1.r = v2.r = v3.r = 1.0f;
	v0.g = v1.g = v2.g = v3.g = 1.0f;
	v0.b = v1.b = v2.b = v3.b = 1.0f;
	v0.a = v1.a = v2.a = v3.a = rttAlpha;
	
	r3d->DrawSquare(&v0,&v1,&v2,&v3,CULL_ALLOW_ALL,false);
}

// ASSO: END ---------------------------------------------------------------------------------------------
