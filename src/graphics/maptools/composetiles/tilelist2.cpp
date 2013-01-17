/*******************************************************************************\
	Composite texture tile manager class -- Used by the ComposeTilesMore tool.

	Scott Randolph
	Spectrum HoloByte
	July 1, 1997
\*******************************************************************************/
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "shi\ShiError.h"
#include "..\..\Terrain\Ttypes.h"
#include "..\..\3Dlib\Image.h"
#include "TileList.h"

// Read in all the previous LOD images and catalog them
void TileListManager::Setup( char *path )
{
	char			filename[256];
	int				listFile;
	int				ret;
	TileListEntry	*newTile;
	BYTE			imageData[32*32];
	int				r, c;


	// Note that we don't yet have the shared palette loaded
	// (in the case of the composite tile manager, we never will)
	palette = NULL;

	// Open the linear array of composite tiles
	strcpy( filename, path );
	strcat( filename, "FarTiles.RAW" );
	printf( "Reading from list file %s\n", filename );
	listFile = open( filename, _O_RDONLY | _O_BINARY, _S_IREAD | _S_IWRITE );
	ShiAssert( listFile != -1 );

	// Prime the pump...
	totalTiles = 0;
	tileListHead = NULL;
	newTile = new TileListEntry;

	// Loop until the input file is exhausted
	while ((ret = read( listFile, imageData, sizeof(imageData))) == sizeof(imageData)) {

		// Store the tiles id
		newTile->texCode = totalTiles;

		// Downsample the image data into the tile record
		for (r=0; r<16; r++) {
			for (c=0; c<16; c++) {
				newTile->data[r*16+c] = imageData[(r*32+c)*2];
			}
		}

		// Add the tile to the list
		newTile->next = tileListHead;
		tileListHead = newTile;

		// Get a new blank record for next time
		newTile = new TileListEntry;

		// Increment out count of tiles
		totalTiles++;
	}

	// Clean up after ourselves (we didn't use the last "newTile" we allocated)
	delete newTile;
	close( listFile );
}


void TileListManager::Cleanup( void )
{
	TileListEntry	*p;

	while (tileListHead) {
		p = tileListHead->next;
		delete tileListHead;
		tileListHead = p;
	}

	totalTiles = 0;
	delete[] palette;
}


const char* TileListManager::GetFileName( WORD texCode )
{
	TileListEntry	*pTileRecord;

	for (pTileRecord=tileListHead; pTileRecord != NULL; pTileRecord = pTileRecord->next) {
		if ( pTileRecord->texCode == texCode ) {
			return pTileRecord->name;
		}
	}

	printf( "tile coded %0X was requested, but not found in TileList!\n", texCode );
	return NULL;
}


const BYTE*	TileListManager::GetImageData( WORD texCode )
{
	TileListEntry	*pTileRecord;

	for (pTileRecord=tileListHead; pTileRecord != NULL; pTileRecord = pTileRecord->next) {
		if ( pTileRecord->texCode == texCode ) {
			return pTileRecord->data;
		}
	}

	printf( "tile coded %0X was requested, but not found in TileList!\n", texCode );
	return NULL;
}


void TileListManager::ReadImageData( char *filename, BYTE *target, DWORD size )
{
	ShiAssert( !"Don't do this -- data at this point is in a linear RAW file." );
}


void TileListManager::WriteSharedPaletteData( int TargetFile )
{
	ShiAssert( !"Shouldn't do this for second and subsequent "far" tile sets." );
}
