/*******************************************************************\
	A tool to read in an 8 bit RAW texture template file and
	write out an 16 bit RAW texture ID map.  The input file uses a
	two by two pixel square to encode the required texture at each
	post.  As a result, the input file is 2x in width and height as
	compared to the output file.

	Scott Randolph		February 22, 1996		Spectrum HoloByte
\*******************************************************************/
#include <stdio.h>
#include <math.h>
#include <windows.h>
#include "shi\ShiError.h"
#include "texmunge.h"




// Used to accumulate the list of terrain textures in use by this map
codeListEntry	*codeList;
setListEntry	setList[256];
int				setListLen = 0;


// Convert an "Eric" tile code into a compressed (sequential) code
// for use by the run time engine.
WORD CompressedCode( WORD code )
{
	int i, j;

	BYTE set = code >> 4;
	BYTE tile = code & 0xF;

	// Look for the set in the list of already used sets
	for (i=0; i<setListLen; i++) {
		if (setList[i].setCode == set) {
			break;
		}
	}

	// i is now the new contiguous set id
	ShiAssert( i < 256 );
	if (i == setListLen) {
		setList[i].setCode = set;
		setListLen++;
	}
	

	// Look for the tile in the list of already used tiles in this set
	for (j=0; j<setList[i].numTiles; j++) {
		if (setList[i].tileCode[j] == tile) {
			break;
		}
	}

	// j is now the new contiguous tile id
	ShiAssert( j < 16 );
	if (j == setList[i].numTiles) {
		setList[i].tileCode[j] = tile;
		setList[i].numTiles++;
	}


	// Construct and return the new code
	return ((i & 0xFF) << 4) | (j & 0xF);
}


// Print a list of all the textures used and how many times they were referenced
// Return the number of textures actually applied to the terrain map
void PrintTextureList( void ) 
{
	codeListEntry	*entry = codeList;

	while ( entry ) {
		printf("%X\t %s\t | Used %0d times\n", entry->id, entry->name, entry->usageCount);
		entry = entry->next;
	}
}


// Function to build and maintain a list of all textures used by this map
// The first argument is the "compressed" id, and the second is "Eric's" code
void AddToCodeList( WORD id, WORD code )
{
	codeListEntry *entry;
	codeListEntry *newEntry;

	// If we don't have a code list already, start one with an end marker
	if (!codeList) {
		codeList = new codeListEntry;
		ShiAssert( codeList );
		codeList->usageCount = 0;
		codeList->prev = NULL;
		codeList->next = NULL;
		codeList->id = 0xFFFF;
		codeList->code = 0xFFFF;
		strcpy( codeList->name, "END MARK" );
	}
	entry = codeList;

	// Skip over lower valued codes
	while ( entry->id < id ) {
		entry = entry->next;
	}

	// If we found a match, return
	if (entry->id == id) {
		entry->usageCount++;
		return;
	}

	// Add this code to the list in its proper place
	newEntry = new codeListEntry;
	newEntry->prev = entry->prev;
	newEntry->next = entry;
	newEntry->id = id;
	newEntry->code = code;
	newEntry->usageCount = 1;
	NumberToName( code, newEntry->name );
	newEntry->next->prev = newEntry;
	if (newEntry->prev) {
		newEntry->prev->next = newEntry;
	} else {
		codeList = newEntry;
	}
}


// Function to release the memory used by the texture code list
void ReleaseCodeList( void )
{
	codeListEntry *entry = codeList;
	codeListEntry *d;

	// Delete all the list entries INCLUDING THE END MARKER
	while (entry) {
		d = entry;
		entry = entry->next;
		delete d;
	}
	codeList = NULL;
}


// The main routine which handles parameter interpretation and the main read/write loop
int main(int argc, char **argv)
{
	HANDLE			inFile;
	HANDLE			outFile;
	int				width;
	int				height;
	int				row;
	int				col;
	char			*inName;
	char			*outName;
	BYTE			*buffer;
	PIXELCLUSTER	sourceSamples;
	WORD			*IDbuffer;
	DWORD			bytes;
	WORD			code;
	int				i, j;


	// Initialize the list of tile and set codes as empty
	for(i=0; i<256; i++) {
		setList[i].setCode = -1;
		setList[i].numTiles = 0;
		for(j=0; j<16; j++) {
			setList[i].tileCode[j] = -1;
		}
	}


	// Check for expected number of parameters
	if (argc != 5) {
		printf("Usage:  TexMunge <input file> <output file> <width> <height>\n");
		printf("   Reads in an 8 bit RAW texture template file and\n");
		printf("   writes out a 16 bit RAW texture ID map.  The input file uses a\n");
		printf("   two by two pixel sqaure to encode the required texture at each\n");
		printf("   post.  As a result, the input file is 2x in width and height as\n");
		printf("   compared to the output file.  The width and height provided on\n");
		printf("   the command line are the width and height of the output file.\n");

		return -1;
	}


	// Get the width and height of the RAW file from the command line
	inName	= argv[1];
	outName	= argv[2];
	width	= atoi( argv[3] );
	height	= atoi( argv[4] );


	// Allocate a buffer large enough to hold our input file
	buffer = (BYTE*)malloc( width*2*height*2*sizeof( *buffer ) );
	ShiAssert( buffer );

	// Allocate a buffer large enough to hold a row of our output file
	IDbuffer = (WORD*)malloc( width*sizeof( *IDbuffer ) );
	ShiAssert( IDbuffer );


	// Open the input file
//	printf( "Opening input file.\n");
    inFile = CreateFile( inName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	ShiAssert( inFile != INVALID_HANDLE_VALUE );

	// Open the output file
//	printf( "Opening output file.\n");
	outFile = CreateFile( outName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	ShiAssert( outFile != INVALID_HANDLE_VALUE );


	// Read in the input data
	if ( !ReadFile( inFile, buffer, width*2*height*2*sizeof(*buffer), &bytes, NULL ) )  bytes=1;
	ShiAssert( bytes == width*2*height*2*sizeof(*buffer) );


	// Write out a row of zeros at the top (skip the first row since we don't have neighbors)
	for ( col = 0; col < width; col++ ) {
		IDbuffer[ col ] = 0;
	}
	WriteFile( outFile, IDbuffer, width*sizeof(*IDbuffer), &bytes, NULL );
	ShiAssert( bytes == width*sizeof(*IDbuffer) );


	// Do one row and a time
//	printf( "Converting data points.\n");
	for ( row = 1; row < height-1; row++ ) {

		
		// We are skipping the edge columns to simplicity's sake
		IDbuffer[ 0 ]		= 0;
		IDbuffer[ width-1 ]	= 0;


		// Fill in the row of output data with appropriatly chosen texture IDs
		for ( col = 1; col < width-1; col++ ) {

			if (col == 231 && row == 230)
			{
				col = col;
				row = row + col - col;
			}


			sourceSamples.p1 = buffer[ ((row<<1))*(width<<1)   + ((col<<1))   ];
			sourceSamples.p2 = buffer[ ((row<<1))*(width<<1)   + ((col<<1)+1) ];
			sourceSamples.p3 = buffer[ ((row<<1)+1)*(width<<1) + ((col<<1))   ];
			sourceSamples.p4 = buffer[ ((row<<1)+1)*(width<<1) + ((col<<1)+1) ];

			sourceSamples.p1A = buffer[ ((row<<1))*(width<<1)   + ((col<<1)-1) ];
			sourceSamples.p1B = buffer[ ((row<<1)-1)*(width<<1) + ((col<<1)-1) ];
			sourceSamples.p1C = buffer[ ((row<<1)-1)*(width<<1) + ((col<<1))   ];

			sourceSamples.p2A = buffer[ ((row<<1)-1)*(width<<1) + ((col<<1)+1) ];
			sourceSamples.p2B = buffer[ ((row<<1)-1)*(width<<1) + ((col<<1)+2) ];
			sourceSamples.p2C = buffer[ ((row<<1))*(width<<1)   + ((col<<1)+2) ];

			sourceSamples.p3A = buffer[ ((row<<1)+1)*(width<<1) + ((col<<1)-1) ];
			sourceSamples.p3B = buffer[ ((row<<1)+2)*(width<<1) + ((col<<1)-1) ];
			sourceSamples.p3C = buffer[ ((row<<1)+2)*(width<<1) + ((col<<1))   ];

			sourceSamples.p4A = buffer[ ((row<<1)+2)*(width<<1) + ((col<<1)+1) ];
			sourceSamples.p4B = buffer[ ((row<<1)+2)*(width<<1) + ((col<<1)+2) ];
			sourceSamples.p4C = buffer[ ((row<<1)+1)*(width<<1) + ((col<<1)+2) ];

			code = DecodeCluster( sourceSamples, row, col );
//			assert( code != 0xFFFF );

			IDbuffer[ col ] = CompressedCode( code );
			AddToCodeList( IDbuffer[ col ], code );
		}


		// Write out the row of texture IDs
		WriteFile( outFile, IDbuffer, width*sizeof(*IDbuffer), &bytes, NULL );
		ShiAssert( bytes == width*sizeof(*IDbuffer) );

	}


	// Write out a row of zeros at the bottom (skip the last row since we don't have neighbors)
	for ( col = 0; col < width; col++ ) {
		IDbuffer[ col ] = 0;
	}
	WriteFile( outFile, IDbuffer, width*sizeof(*IDbuffer), &bytes, NULL );
	ShiAssert( bytes == width*sizeof(*IDbuffer) );

	
	// Close the input and output files
	CloseHandle( inFile );
	CloseHandle( outFile );


	// Print out closing message
	PrintTextureList();

	ReleaseCodeList();

	return 0;
}