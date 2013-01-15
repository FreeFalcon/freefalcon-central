/*******************************************************************************\
	Read in all in use 2x2 combinations of terrain texture tiles, and generate
	aggregate 32x32 pixel bitmaps and the associated ID assignment map.

	Scott Randolph
	Spectrum HoloByte
	November 14, 1995
\*******************************************************************************/
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <math.h>
#include <crtdbg.h>
#include "shi\ShiError.h"
#include "..\..\Terrain\Ttypes.h"
#include "..\..\Terrain\TPost.h"
#include "..\..\Terrain\TDskPost.h"
#include "..\..\3Dlib\Image.h"
#include "IDlist.h"
#include "TileList.h"


static const unsigned LAST_TEX_LEVEL	= 2;


void main(int argc, char* argv[]) {

	int			bufferWidth;
	int			bufferHeight;

	int			texMapWidth;
	int			texMapHeight;
	DWORD		TexIDBufferSize;
	WORD		*TexIDBuffer;
	DWORD		texCornerOffset;	

	int			outMapWidth;
	int			outMapHeight;
	DWORD		outBufferSize;
	WORD		*outBuffer;

	OPENFILENAME dialogInfo;
	char		filename[256];
	char		dataRootDir[256];
	char		dataSet[256];
	char		texPath[256];

	HANDLE		colorFile;
	HANDLE		textureFile;
	int			palFile;

	int			row;
	int			col;

	IDListManager	IDList;
	TileListManager	TileList;

	__int64		code;
	DWORD		bytes;
	int			result;


	// See if we got a filename on the command line
	if ( argc == 2) {
		result = GetFullPathName( argv[1], sizeof( filename ), filename, NULL );
	} else {
		result = 0;
	}

	// If we didn't get it on the command line, ask the user
	// for the name of the master BMP color file
	if (!result) {
		filename[0] = NULL;
		dialogInfo.lStructSize = sizeof( dialogInfo );
		dialogInfo.hwndOwner = NULL;
		dialogInfo.hInstance = NULL;
		dialogInfo.lpstrFilter = "24 Bit BMP\0*-C.BMP\0\0";
		dialogInfo.lpstrCustomFilter = NULL;
		dialogInfo.nMaxCustFilter = 0;
		dialogInfo.nFilterIndex = 1;
		dialogInfo.lpstrFile = filename;
		dialogInfo.nMaxFile = sizeof( filename );
		dialogInfo.lpstrFileTitle = NULL;
		dialogInfo.nMaxFileTitle = 0;
		dialogInfo.lpstrInitialDir = "J:\\TerrData";
		dialogInfo.lpstrTitle = "Select a base BMP file (*-C.BMP)";
		dialogInfo.Flags = OFN_FILEMUSTEXIST;
		dialogInfo.lpstrDefExt = "BMP";

 		if ( !GetOpenFileName( &dialogInfo ) ) {
			return;
		}
	}


	// Extract the path to the directory ONE above the one containing the selected file
	// (the "root" of the data tree)
	char *p = &filename[ strlen(filename)-1 ];
	while ( (*p != ':') && (*p != '\\') && (p != filename) ) {
		if (*p == '.')  *p = '\0';
		p--;
	}
	*p = '\0';
	char *base = p+1;
	char *dir = filename;
	while ( (*p != ':') && (*p != '\\') && (p != filename) ) {
		p--;
	}
	*p = '\0';
	strcpy( dataRootDir, dir );
	strcpy( dataSet, base );
	dataSet[strlen(dataSet)-2] = '\0';	// Get rid of the "-C"
	strcpy( texPath, dir );
	strcat( texPath, "\\texture\\" );


/************************************************************************************\
	Got all input args -- Begin Setup
\************************************************************************************/

	// Open the color input file
	sprintf( filename, "%s\\terrain\\%s-C.BMP", dataRootDir, dataSet );
	printf( "Reading COLOR file %s\n", filename );
    colorFile = CreateFile( filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( colorFile == INVALID_HANDLE_VALUE ) {
		char string[256];
		PutErrorString( string );
		strcat( string, "Failed to open color file." );
		ShiError( string );
	}


	// Read the image header
#pragma pack (1)	// Force tightly packed structure to match the file format
	struct {
		BITMAPFILEHEADER	header;
		BITMAPINFOHEADER	info;
	} bm;
#pragma pack ()		// Go back to the default structure alignment scheme
	if ( !ReadFile( colorFile, &bm, sizeof(bm), &bytes, NULL ) )  bytes=0xFFFFFFFF;
	if ( bytes != sizeof(bm) ) {
		char string[256];
		PutErrorString( string );
		strcat( string, "Couldn't read required color BMP header." );
		ShiError( string );
	}
	if (( bm.header.bfType != 0x4D42		) ||
		( bm.info.biCompression != BI_RGB	) ||
		( bm.info.biPlanes != 1				) ||
		( bm.info.biBitCount != 24			)) {
		ShiError( "Invalid or unsupported BMP format" );
	}

	// Store the image size
	bufferWidth		= bm.info.biWidth;
	bufferHeight	= bm.info.biHeight;
	ShiAssert( (bufferWidth & 0x3) == 0 );
	ShiAssert( (bufferHeight & 0x3) == 0 );

	// Close the bitmap file now that we've gotten all the header info we need
	CloseHandle(colorFile);


	// Allocate space for the incomming texture ID buffer
	texMapWidth		= bufferWidth >> LAST_TEX_LEVEL;
	texMapHeight	= bufferHeight >> LAST_TEX_LEVEL;
	TexIDBufferSize = texMapWidth*texMapHeight*sizeof(*TexIDBuffer);
	TexIDBuffer = (WORD*)malloc( TexIDBufferSize );
	ShiAssert( TexIDBuffer );

	// Open the texture id layout file
	sprintf( filename, "%s\\terrain\\%s-T.RAW", dataRootDir, dataSet );
	printf( "Reading TEXTURE ID file %s\n", filename );
    textureFile = CreateFile( filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if( textureFile == INVALID_HANDLE_VALUE ) {
		char string[256];
		PutErrorString( string );
		strcat( string, "Failed to open texture id input file." );
		ShiError( string );
	}

	// Read in the data
	if ( !ReadFile( textureFile, TexIDBuffer, TexIDBufferSize, &bytes, NULL ) )  bytes=0xFFFFFFFF;
	if ( bytes != TexIDBufferSize ) {
		char string[256];
		PutErrorString( string );
		strcat( string, "Couldn't read required texture data." );
		ShiError( string );
	}

	// Close the texture ID input file
	CloseHandle( textureFile );
	

	// Allocate space for the output texture offset buffer
	outMapWidth		= texMapWidth >> 1;
	outMapHeight	= texMapHeight >> 1;
	outBufferSize	= outMapWidth*outMapHeight*sizeof(*outBuffer);
	outBuffer		= (WORD*)malloc( outBufferSize );
	ShiAssert( outBuffer );

	// Open the texture offset output file
	sprintf( filename, "%s\\terrain\\FarTiles.%0d", dataRootDir, LAST_TEX_LEVEL+1 );
	printf( "Opening output file %s\n", filename );
    textureFile = CreateFile( filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if( textureFile == INVALID_HANDLE_VALUE ) {
		char string[256];
		PutErrorString( string );
		strcat( string, "Failed to open texture offset output file." );
		ShiError( string );
	}


	// Setup our internal database structures
	TileList.Setup( texPath );
	IDList.Setup( texPath, &TileList );


/************************************************************************************\
	Startup complete -- Begin Main Loop
\************************************************************************************/

	// Scan the texture layout file and identify all unique 2x2 combinations
	for (row = 0; row <outMapHeight; row++) {
		for (col = 0; col < outMapWidth; col++) {

			texCornerOffset = (row<<1)*texMapWidth + (col<<1);

			code  = ((__int64)TexIDBuffer[ texCornerOffset ])				<< 48;	// Top left
			code |= ((__int64)TexIDBuffer[ texCornerOffset+1 ])				<< 32;	// Top right
			code |= ((__int64)TexIDBuffer[ texCornerOffset+texMapWidth ])	<< 16;	// Bottom left
			code |= ((__int64)TexIDBuffer[ texCornerOffset+texMapWidth+1 ])	<<  0;	// Bottom right

			outBuffer[row*outMapWidth + col] = IDList.GetIDforCode( code );	

		}
	}


/************************************************************************************\
	Main loop complete -- Shutdown
\************************************************************************************/

	// Write out the tile offset map
	printf( "Writing texture offset map.\n" );
	if ( !WriteFile( textureFile, outBuffer, outBufferSize, &bytes, NULL ) )  bytes=0xFFFFFFFF;
	if ( bytes != outBufferSize ) {
		char string[256];
		PutErrorString( string );
		strcat( string, "Couldn't write texture offset map." );
		ShiError( string );
	}
	CloseHandle( textureFile );


	// Create a new composite tile palette file
	printf( "Creating tile palette file.\n" );
	sprintf( filename, "%s%s", texPath, "FarTiles.PAL" );
	palFile = open( filename, _O_CREAT | _O_TRUNC | _O_WRONLY | _O_APPEND | _O_BINARY, _S_IREAD | _S_IWRITE );
	ShiAssert( palFile != -1 );

	// Write out the shared palette
	printf( "Writing shared palette.\n" );
	TileList.WriteSharedPaletteData( palFile );

	// Write out the total number of tiles in use by this first "far" LOD
	printf( "Writing tile count.\n" );
	DWORD numCodes = IDList.GetNumCodes();
	result = write( palFile, &numCodes, sizeof(numCodes) );
	ShiAssert( result == sizeof(numCodes) );

	// Could write out representative tile type for each composite tile here.

	// Close the palette file
	close( palFile );


	// Report our results
	printf( "Closing down after issueing %0d new IDs (%0d were possible)\n", 
			IDList.GetNumCodes(),
			outMapWidth*outMapHeight);

	// Release the tex ID buffer and other resources
	free( outBuffer );
	free( TexIDBuffer );
	IDList.Cleanup();
	TileList.Cleanup();
}
