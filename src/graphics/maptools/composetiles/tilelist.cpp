/*******************************************************************************\
	Original texture tile manager class -- Used by the ComposeTiles tool.

	Scott Randolph
	Spectrum HoloByte
	May 14, 1997
\*******************************************************************************/
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "shi\ShiError.h"
#include "..\..\Terrain\Ttypes.h"
#include "..\..\3Dlib\Image.h"
#include "TileList.h"


void TileListManager::Setup( char *path )
{
	char			filename[256];
	FILE			*listFile;
	int				ret;
	int				id;
	char			basename[20];
	TileListEntry	*newTile;


	// Note that we don't yet have the shared palette loaded
	palette = NULL;

	// Construct the source file name
	strcpy( filename, path );
	strcat( filename, "TexCodes.txt" );

	// Open the list of textures codes and file names
	printf( "Reading from list file %s\n", filename );
	listFile = fopen( filename, "r" );
	ShiAssert( listFile );

	// Prime the pump...
	totalTiles = 0;
	tileListHead = NULL;
	newTile = new TileListEntry;

	// Loop until the input file is exhausted
	while ((ret = fscanf( listFile, "%X %s %*[^\n]", &id, &basename )) == 2) {

		// Convert to the "Tiny" file names
		basename[0] = 'T';

		// Store the tiles id and name
		newTile->texCode = id;
		strcpy( newTile->name, basename );

		// Read the associated image data
		if (id != 0xFFFF) {
			strcpy( filename, path );
			strcat( filename, basename );
			printf( "Fetching tile %s\n", basename );
			ReadImageData( filename, newTile->data, sizeof(newTile->data) );
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

	// Back off one tile (the last one is the end marker)
	totalTiles--;
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
	CImageFileMemory 	texFile;
	int					result;
	DWORD				width;
	DWORD				height;


	ShiAssert( filename );
	ShiAssert( target );


	// Make sure we recognize this file type
	texFile.imageType = CheckImageType( (GLbyte*)filename );
	ShiAssert( texFile.imageType != IMAGE_TYPE_UNKNOWN );

	// Open the input file
	result = texFile.glOpenFileMem( (GLbyte*)filename );
	if ( result != 1 ) {
		char	message[256];
		sprintf( message, "Failed to open %s", filename );
		ShiError( message );
	}

	// Read the image data (note that ReadTextureImage will close texFile for us)
	texFile.glReadFileMem();
	result = ReadTextureImage( &texFile );
	if (result != GOOD_READ) {
		ShiError( "Failed to read terrain texture.  CD Error?" );
	}


	// Store the image properties in our local storage
	width = texFile.image.width;
	height = texFile.image.height;
	ShiAssert(texFile.image.palette);

	// Copy the image palette indicies
	ShiAssert( size == width * height );
	memcpy( target, texFile.image.image, size );


	// If we don't already have the shared palette, save it
	if (!palette) {
		palette = (DWORD*)texFile.image.palette;
	} else {
		glReleaseMemory( texFile.image.palette );
	}

	ShiAssert( palette );

	// Release the image buffer
	glReleaseMemory( texFile.image.image );
}


void TileListManager::WriteSharedPaletteData( int TargetFile )
{
	int		result;

	ShiAssert( TargetFile != -1 );

	// Write the palette data verbatim
	result = write( TargetFile, palette, 256*sizeof(*palette) );
	ShiAssert( result == 256*sizeof(*palette) );
}
