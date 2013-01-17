/*******************************************************************\
	Simple tool to read in a 16 bit RAW file and scale its values
	to 8 bits.

	Scott Randolph		January 24, 1996		Spectrum HoloByte
\*******************************************************************/
#include <stdio.h>
#include <math.h>
#include <windows.h>
#include <assert.h>


int main(int argc, char **argv)
{
	HANDLE	inFile;
	HANDLE	outFile;
	DWORD	width;
	DWORD	height;
	DWORD	row;
	char	*inName;
	char	*outName;
	WORD	*buffer;
	WORD	*src;
	BYTE	*dst;
	short	maxValue;
	double	scale;
	DWORD	bytes;


	// Check for expected number of parameters
	if ((argc < 5) || (argc > 6)) {
		printf("Usage:  16to8 <input file> <output file> <width> <height> [maxelevation]\n");
		printf("   This tool reads in a 16 bit RAW file and scales it to 8 bits.\n");
		printf("   Negative values are clamped to 0, and the largest positive value\n");
		printf("   encountered in the 16 bit data is scaled to 0xFF in the output file.\n");
		printf("   It is assumed that the input file is 16 bit interleaved RAW data.  If\n");
		printf("   the maxelevation parameter is supplied, the initial scan of the RAW file\n");
		printf("   is skipped and the provided value is used to compute the scale factor.\n");

		return -1;
	}


	// Get the width and height of the RAW file from the command line
	inName	= argv[1];
	outName	= argv[2];
	width	= atoi( argv[3] );
	height	= atoi( argv[4] );
	if (argc == 6) {
		maxValue = atoi( argv[5] );
	} else {
		maxValue = 0;
	}


	// Allocate a buffer large enough to hold one row of the input
	buffer = (WORD*)malloc( width*2 );


	//
	// PART ONE:  Scan the input file and determine an appropriate scale factor
	//

	// Open the elevation information file
	printf( "Opening input file.\n");
    inFile = CreateFile( inName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	assert( inFile != INVALID_HANDLE_VALUE );


	// If we don't already have a max elevation, scan the file to determine it
	if (maxValue == 0 ) {

		// Do one row and a time
		printf("Scanning input file to find max height.\n");
		for (row = height; row > 0; row-- ) {
			
			// Read in the row of data
			if ( !ReadFile( inFile, buffer, width*2, &bytes, NULL ) )  bytes=1;
			assert( bytes == width*2 );

			// Process the row of data
			for (src = buffer; src<buffer+width; src++) {

				// Clamp negative values to zero
				if (*src & 0x8000)
					*src = 0;

				// Update the maximum value
				maxValue = max( *src, maxValue );

			}

		}
	}

	// Compute the scale factor based on the maximum altitude we need to represent
	scale = 256.0 / maxValue;
	printf( "Max value is %d resulting in a scale factor of %1.4f.\n", maxValue, scale );

	
	//
	// PART TWO:  Scan the input file and determine an appropriate scale factor
	//

	// Reposition ourselves to the start of the input file
	SetFilePointer( inFile, 0, NULL, FILE_BEGIN );	

	// Open the output file
	printf( "Opening output file.\n");
	outFile = CreateFile( outName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	assert( outFile != INVALID_HANDLE_VALUE );

	// Do one row and a time
	printf( "Converting data points from 16 to 8 bits.\n");
	for (row = height; row > 0; row-- ) {
		
		// Read in the row of data
		if ( !ReadFile( inFile, buffer, width*2, &bytes, NULL ) )  bytes=1;
		assert( bytes == width*2 );

		// Process the row of data
		for (src = buffer, dst = (BYTE*)buffer; src<buffer+width; src++, dst++) {

			// Clamp negative values to zero
			if (*src & 0x8000)
				*src = 0;

			// Apply the scale factor
			double value = scale * *src;
			if (value > 255.0) {
				*dst = 255;
			} else {
				*dst = (BYTE)floor( value );
			}
		}

		// Write out the 8 bit row
		WriteFile( outFile, buffer, width, &bytes, NULL );
		assert( bytes == width );

	}


	// Close the input and output files
	CloseHandle( inFile );
	CloseHandle( outFile );


	// Print out closing message
	printf("Conversion complete.\n");
	printf("1 bit in the output represents %0.2f bits from the input.\n", log(1.0/scale)/log(2.0));
	printf("The minimum representable change in the output is %0.4f units of the input.\n", 1.0/scale );

	return 0;
}