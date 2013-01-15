/***************************************************************************\
    PalBank.h
    Scott Randolph
    March 9, 1998

    Provides the bank of palettes used by all the BSP objects.
\***************************************************************************/
#ifndef _PALBANK_H_
#define _PALBANK_H_

#include "Palette.h"
#include "PolyLib.h"


// The one and only palette bank.  This would need to be replaced
// by pointers to instances of PaletteBankClass passed to each call
// if more than one color store were to be simultaniously maintained.
extern class PaletteBankClass		ThePaletteBank;


class PaletteBankClass {
  public:
	PaletteBankClass()	{ nPalettes = 0; PalettePool = NULL; };
	~PaletteBankClass()	{};

	// Management functions
	static void Setup( int nEntries );
	static void Cleanup( void );
	static void ReadPool( int file );
	static void FlushHandles( void );

	// Set the light level on the specified palette
	static void LightPalette( int id, Tcolor *light );
	static void LightBuildingPalette( int id, Tcolor *light );
	static void LightReflectionPalette( int id, Tcolor *light );

	// Check if palette ID is within the legal range of loaded palettes
	static BOOL IsValidIndex( int id );

  public:
	static Palette		*PalettePool;
	static int			nPalettes;
};
#endif // _PALBANK_H_