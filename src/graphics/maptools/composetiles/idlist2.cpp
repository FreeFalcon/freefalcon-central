/*******************************************************************************\
	Unique ID list manager class -- Used by the ComposeTiles tool.

	Scott Randolph
	Spectrum HoloByte
	May 12, 1997
\*******************************************************************************/
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <shi\ShiError.h>
#include "..\..\Terrain\Ttypes.h"
#include "IDlist.h"


static const int TILE_SIZE = 16;


void IDListManager::Setup( char *path, TileListManager *tileListmgr )
{
	char	filename[256];

	// Save the pointer to our helper class
	pTileListMgr = tileListmgr;

	// Create a new composite tile output file
	strcpy( filename, path );
	strcat( filename, "FarTiles.raw" );
	tileFile = open( filename, _O_WRONLY | _O_APPEND | _O_BINARY, _S_IREAD | _S_IWRITE );
	ShiAssert( tileFile != -1 );

	// Get the next available code from the tile manager
	startID = pTileListMgr->GetTileCount();
}


void IDListManager::Cleanup( void )
{
	// Close the composite tile output file
	close( tileFile );
}


WORD IDListManager::GetIDforCode( __int64 code )
{
	DWORD	i;
	WORD	ID;

	// Look for an existing match in the code list
	for (i=0; i<numCodes; i++) {
		if (code == Codes[i]) {
			ShiAssert( i+startID < 0xFFFF );
			ID = (WORD)(i+startID);
			return ID;
		}
	}

	// New code we haven't seen yet
	numCodes++;
	Codes[i] = code;
	ShiAssert( i+startID < 0xFFFF );
	ID = (WORD)(i+startID);

	// Create the composite image in the tileFile
	WORD	NWcode = (WORD)((code>>48) & 0xFFFF);
	WORD	NEcode = (WORD)((code>>32) & 0xFFFF);
	WORD	SWcode = (WORD)((code>>16) & 0xFFFF);
	WORD	SEcode = (WORD)((code>>0)  & 0xFFFF);

	printf( "Compositing image at offset %0d (code %4X %4X %4X %4X)\n", ID, NWcode, NEcode, SWcode, SEcode );
	WriteCompositeTile( NWcode, NEcode, SWcode, SEcode );

	return ID;
}


void IDListManager::WriteCompositeTile( WORD NWcode, WORD NEcode, WORD SWcode, WORD SEcode )
{
	BYTE		outBuffer[32*32];
	BYTE		*out = outBuffer;
	const BYTE	*NWin = pTileListMgr->GetImageData( NWcode );
	const BYTE	*NEin = pTileListMgr->GetImageData( NEcode );
	const BYTE	*SWin = pTileListMgr->GetImageData( SWcode );
	const BYTE	*SEin = pTileListMgr->GetImageData( SEcode );
	int			row;
	int			result;


	ShiAssert( tileFile != -1 );
	ShiAssert( out && NWin && NEin && SWin && SEin );
	ShiAssert( out = &outBuffer[32*0] );

	// Construct top rows of output image
	for (row=0; row < 16; row++) {
		memcpy( out, NWin, 16 );
		out += 16;
		NWin += 16;
		memcpy( out, NEin, 16 );
		out += 16;
		NEin += 16;
	}
	ShiAssert( out = &outBuffer[32*16] );

	// Construct bottom rows of output image
	for (row; row < 32; row++) {
		memcpy( out, SWin, 16 );
		out += 16;
		SWin += 16;
		memcpy( out, SEin, 16 );
		out += 16;
		SEin += 16;
	}
	ShiAssert( out = &outBuffer[32*32] );


	// Write the generated image to the composite file
	result = write( tileFile, outBuffer, sizeof(outBuffer) );
	ShiAssert( result == sizeof(outBuffer) );
}

