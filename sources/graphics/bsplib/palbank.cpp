/***************************************************************************\
    PalBank.cpp
    Scott Randolph
    March 4, 1998

    Provides the bank of palettes used by all the BSP objects.
\***************************************************************************/
#include "stdafx.h"
#include <io.h>
#include "PalBank.h"

#ifdef USE_SH_POOLS
extern MEM_POOL gBSPLibMemPool;
#endif

// Static data members (used to avoid requiring "this" to be passed to every access function)
int				PaletteBankClass::nPalettes		= 0;
Palette*		PaletteBankClass::PalettePool	= NULL;

// Management functions
void PaletteBankClass::Setup( int nEntries )
{
	// Create our array of palettes
	nPalettes	= nEntries;
	if (nEntries) {
		#ifdef USE_SH_POOLS
		PalettePool = (Palette *)MemAllocPtr(gBSPLibMemPool, sizeof(Palette)*(nEntries), 0 );
		#else
		PalettePool	= new Palette[nEntries];
		#endif
	} else {
		PalettePool = NULL;
	}
}


void PaletteBankClass::Cleanup( void )
{
	// Clear our extra reference which was holding the data in memory
	for (int i=0; i<nPalettes; i++) {
		PalettePool[i].Release();
	}

	// Clean up our array of palettes
	#ifdef USE_SH_POOLS
	MemFreePtr( PalettePool );
	#else
	delete[] PalettePool;
	#endif
	PalettePool = NULL;
	nPalettes = 0;
}


void PaletteBankClass::ReadPool( int file )
{
	int result;

	// Read how many palettes we have
	result = read( file, &nPalettes, sizeof(nPalettes) );

	// Allocate memory for our palette list
	#ifdef USE_SH_POOLS
	PalettePool = (Palette *)MemAllocPtr(gBSPLibMemPool, sizeof(Palette)*(nPalettes), 0 );
	#else
	PalettePool = new Palette[nPalettes];
	#endif
	ShiAssert( PalettePool );

	// Read the data for each palette
	result = read( file, PalettePool, sizeof(*PalettePool)*nPalettes );
	if (result < 0 ) {
		char message[256];
		sprintf( message, "Reading object palette bank:  %s", strerror(errno) );
		ShiError( message );
	}
}


void PaletteBankClass::FlushHandles( void )
{
	int id;
	int	cnt;

	// Clear our extra reference which was holding the MPR instances of palettes
	for (id=0; id<nPalettes; id++) {
		cnt = PalettePool[id].Release();
		ShiAssert( cnt == 0 );
#if 0  // If a quick hack is required to clean up, this would be it...
		while (PalettePool[id].Release());
#endif
	}

	// Now put the extra reference back for next time (to hold the MPR palette once its loaded)
	for (id=0; id<nPalettes; id++) {
		PalettePool[id].Reference();
	}
}


// Set the light level on the specified palette
void PaletteBankClass::LightPalette( int id, Tcolor *light )
{
	if ( !IsValidIndex(id) ) {
		return;
	}

	PalettePool[id].LightTexturePalette( light );
}


// Set the light level on the specified palette (with special building lights)
void PaletteBankClass::LightBuildingPalette( int id, Tcolor *light )
{
	if ( !IsValidIndex(id) ) {
		return;
	}

	PalettePool[id].LightTexturePaletteBuilding( light );
}


// Set the light level on the specified palette (with special cockpit reflection alpha)
void PaletteBankClass::LightReflectionPalette( int id, Tcolor *light )
{
	if ( !IsValidIndex(id) ) {
		return;
	}

	PalettePool[id].LightTexturePaletteReflection( light );
}


BOOL PaletteBankClass::IsValidIndex( int id ) {
	return ((id >= 0) && (id < nPalettes));
}
