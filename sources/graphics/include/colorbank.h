/***************************************************************************\
    ColorBank.h
    Scott Randolph
    February 17, 1998

    Provides the bank of colors used by all the BSP objects.
\***************************************************************************/
#ifndef _COLORBANK_H_
#define _COLORBANK_H_

#include "PolyLib.h"


// The one and only color bank.  This would need to be replaced
// by pointers to instances of ColorBankClass passed to each call
// if more than one color store were to be simultaniously maintained.
extern class ColorBankClass		TheColorBank;


class ColorBankClass {
  public:
	ColorBankClass()	{ nColors = nDarkendColors = 0; ColorPool = ColorBuffer = NULL; };
	~ColorBankClass()	{};

	enum ColorMode { NormalMode, UnlitMode, GreenMode, UnlitGreenMode };

	// Management functions
	static void Setup( int nclrs, int ndarkclrs );
	static void Cleanup( void );
	static void ReadPool( int file );
	static void	SetLight( float red, float green, float blue );
	static void SetColorMode( ColorMode mode );

	// Debug parameter validation
	static BOOL IsValidColorIndex( int i );


  public:
	// Publicly used color array (set when color mode is chosen)
	static Pcolor		*ColorPool;

	// Color counts
	static int			nColors;			// Total number of colors in each set
	static int			nDarkendColors;		// Number of colors which are staticly lit

	// These are the color pools for each mode
	static Pcolor		*ColorBuffer;		// Normal (original) colors
	static Pcolor		*DarkenedBuffer;	// Processed for static lighting on some colors
	static Pcolor		*GreenIRBuffer;		// Processed for green without lighting
	static Pcolor		*GreenTVBuffer;		// Processed for green with static lighting on some colors

	static DWORD		TODcolor;			//JAM 12Oct03

	static int PitLightLevel;		// Cobra - 3D cockpit light level (0, 1, 2)

	// Light levels for the staticly lit colors
//	float				redLevel;
//	float				greenLevel;
//	float				blueLevel;
};
#endif // _COLORBANK_H_