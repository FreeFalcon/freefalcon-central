/***************************************************************************\
    ColorBank.cpp
    Scott Randolph
    February 17, 1998

    Provides the bank of colors used by all the BSP objects.
\***************************************************************************/
#include "stdafx.h"
#include <io.h>
#include "StateStack.h"
#include "ColorBank.h"

extern bool g_bGreyMFD;
extern bool bNVGmode;
// Color counts
int		ColorBankClass::nColors			= 0;	// Total number of colors in each set
int		ColorBankClass::nDarkendColors	= 0;	// Number of colors which are staticly lit

// This is the publicly used pointer to a color array
Pcolor	*ColorBankClass::ColorPool		= NULL;

// These are the color pools for each mode
Pcolor	*ColorBankClass::ColorBuffer	= NULL;	// Normal (original) colors
Pcolor	*ColorBankClass::DarkenedBuffer	= NULL;	// Processed for static lighting on some colors
Pcolor	*ColorBankClass::GreenIRBuffer	= NULL;	// Processed for green without lighting
Pcolor	*ColorBankClass::GreenTVBuffer	= NULL;	// Processed for green with static lighting on some colors
DWORD	ColorBankClass::TODcolor		= NULL; // JAM 12Oct03
int	ColorBankClass::PitLightLevel = 0;

#ifdef USE_SH_POOLS
extern MEM_POOL gBSPLibMemPool;
#endif


// Management functions
void ColorBankClass::Setup( int nclrs, int ndarkclrs )
{
	// Make sure we're not clobbering a pre-existing table
	ShiAssert( ColorBuffer == NULL );

	// Record how many colors we've got total and prelit
	nColors				= nclrs;
	nDarkendColors		= ndarkclrs;

	// Allocate space for the colors
	#ifdef USE_SH_POOLS
	ColorBuffer = (Pcolor *)MemAllocPtr(gBSPLibMemPool, sizeof(Pcolor)*4*(nclrs+MAX_VERTS_PER_POLYGON+MAX_CLIP_VERTS), 0 );
	#else
	ColorBuffer		= new Pcolor[4*(nclrs + MAX_VERTS_PER_POLYGON + MAX_CLIP_VERTS)];
	#endif
	DarkenedBuffer	= ColorBuffer    + (nclrs + MAX_VERTS_PER_POLYGON + MAX_CLIP_VERTS);
	GreenIRBuffer	= DarkenedBuffer + (nclrs + MAX_VERTS_PER_POLYGON + MAX_CLIP_VERTS);
	GreenTVBuffer	= GreenIRBuffer  + (nclrs + MAX_VERTS_PER_POLYGON + MAX_CLIP_VERTS);
}


void ColorBankClass::Cleanup( void )
{
	nColors				= 0;
	nDarkendColors		= 0;
	#ifdef USE_SH_POOLS
	MemFreePtr( ColorBuffer );
	#else
	delete[] ColorBuffer;
	#endif
	ColorBuffer			= NULL;
	DarkenedBuffer		= NULL;
	GreenIRBuffer		= NULL;
	GreenTVBuffer		= NULL;
}


void ColorBankClass::ReadPool( int file )
{
	int		result;
	Pcolor	*src;
	Pcolor	*end;
	Pcolor	*dst1;
	Pcolor  *dst2;
	Pcolor  *dst3;

	// Read our total color and darkened color count
	result = read( file, &nColors,			sizeof(nColors) );
	result = read( file, &nDarkendColors,	sizeof(nDarkendColors) );

	// Setup our internal storage
	Setup( nColors, nDarkendColors );

	// Read our color array
	result = read( file, ColorBuffer, nColors*sizeof(*ColorBuffer) );
	if (result < 0 ) {
		char message[256];
		sprintf( message, "Reading object color bank:  %s", strerror(errno) );
		ShiError( message );
	}

	// Setup the darkened and green TV and IR versions
	dst1 = DarkenedBuffer;
	dst2 = GreenTVBuffer;
	dst3 = GreenIRBuffer;
	src  = ColorBuffer;
	end  = ColorBuffer + nColors;
	while (src < end) {
		*dst1 = *src;
		dst2->g = src->g;
		dst2->a = src->a;
		dst2->r = 0.0f;
		dst2->b = 0.0f;

		// FRB - B&W display?
		if ((g_bGreyMFD) && (!bNVGmode))
			dst2->r = dst2->b = dst2->g;

		*dst3 = *dst2;
		src++;
		dst1++;
		dst2++;
		dst3++;
	}
}


void ColorBankClass::SetLight( float red, float green, float blue )
{
	Pcolor	*src;
	Pcolor	*end;
	Pcolor	*dst;
	Pcolor	*greenTv;

	ShiAssert( red   <= 1.0f );
	ShiAssert( green <= 1.0f );
	ShiAssert( blue  <= 1.0f );

	// Now darken the staticly lit colors
	src = ColorBuffer;
	end = ColorBuffer + nDarkendColors;
	dst = DarkenedBuffer;
	greenTv = GreenTVBuffer;
	while (src < end) {
		dst->r = red   * src->r;
		dst->g = green * src->g;
		dst->b = blue  * src->b;
		greenTv->g = dst->g;
		// FRB - B&W display?
		if ((g_bGreyMFD) && (!bNVGmode))
			greenTv->r = greenTv->b = greenTv->g;
		src++;
		dst++;
		greenTv++;
	}

	//JAM 12Oct03
	TODcolor = (0xFF<<24)+(FloatToInt32(red*255.f)<<16)+(FloatToInt32(green*255.f)<<8)+FloatToInt32(blue*255.f);
}


void ColorBankClass::SetColorMode( ColorMode mode )
{
	switch (mode) {
	  case NormalMode:
		ColorPool = DarkenedBuffer;
		break;
	  case UnlitMode:
		ColorPool = ColorBuffer;
		break;
	  case GreenMode:
		ColorPool = GreenTVBuffer;
		break;
	  case UnlitGreenMode:
		ColorPool = GreenIRBuffer;
		break;
	  default:
		ShiWarning( "Bad color mode!" );
	}
}


BOOL ColorBankClass::IsValidColorIndex( int i ) {
	return (i < nColors + MAX_VERTS_PER_POLYGON + MAX_CLIP_VERTS);
}
