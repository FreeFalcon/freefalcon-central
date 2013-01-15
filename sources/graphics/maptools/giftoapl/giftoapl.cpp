/*******************************************************************************\
	GIFtoAPL.cpp
	
	This tool reads a GIF file (with optional second alpha map) and writes
	an APL file.  An APL is a palettized image with the ability to store
	alpha values in the palette.

	Scott Randolph
	Spectrum HoloByte
	June 10, 1997
\*******************************************************************************/
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <shi\ShiError.h>
#include "..\..\3Dlib\Image.h"


#define NUM_ALPHAS	7		// This doesn't count the default fully transparent chroma color in slot 0
#define NUM_COLORS	(255/NUM_ALPHAS)



void ReadImage( char *filename, BYTE **image, DWORD **palette, WORD *width, WORD *height );

void main(int argc, char* argv[]) {
	char	inName[MAX_PATH];
	char	alphaName[MAX_PATH];
	char	outName[MAX_PATH];

	DWORD	*inPalette;
	DWORD	*alphaPalette;
	DWORD	*outPalette;

	BYTE	*inImage;
	BYTE	*alphaImage;
	BYTE	*outImage;

	int		a;
	int		i;

	int		outFile;
	int		result;
	WORD	width;
	WORD	height;
	DWORD	magic = 0x030870;


	// If we didn't get the right number of arguments, print a usage message
	if ((argc<2) || (argc>3)) {
		printf("Usage:  GIFtoAPL <colorfile.gif> [alphafile.gif]\n");
		printf("    Converts the input gif(s) into an APL image.\n");
		printf("    The output is stored in <colorfile.apl>\n");
		exit(-1);
	}

	// Open and read the input color image
	GetFullPathName( argv[1], sizeof( inName ), inName, NULL );
	ReadImage( inName, &inImage, &inPalette, &width, &height );
	ShiAssert( inImage );
	ShiAssert( inPalette );

	// Open and read the input alpha image (if we have one)
	if ( argc == 3) {
		WORD	w, h;
		GetFullPathName( argv[2], sizeof( alphaName ), alphaName, NULL );
		ReadImage( alphaName, &alphaImage, &alphaPalette, &w, &h );
		if ((w!=width) || (h!=height)) {
			printf("alpha map isn't the same size as the color map!\n");
			exit(-2);
		}
	} else {
		alphaImage = NULL;
		alphaPalette = NULL;
	}

	// Construct the output file name
	strcpy( outName, inName );
	char *p = strrchr( outName, '.' );
	if (!p) {
		p = &outName[strlen(outName)];
	}
	strcpy(p, ".APL");


	// Allocate memory for the output image and palette
	outPalette = new DWORD[256];
	outImage = new BYTE[ width * height ];


	// If we don't have an alpha map, just copy the data, otherwise do the hard stuff
	if (!alphaImage) {
		// Just copy the image data
		memcpy( outImage, inImage, width*height );

		// Set the alpha components to 1/2 to ensure they're
		// seen as different than a non-APL file without alpha information.
		outPalette[0] = inPalette[0] & 0x00FFFFFF;
		for (i=1; i<256; i++) {
			outPalette[i] = (inPalette[i] & 0x00FFFFFF) | 0x80000000;
		}
	} else {
		DWORD	alpha[NUM_ALPHAS];

		// Get the non-zero alpha values
		for (a=0; a<NUM_ALPHAS; a++) {
			alpha[a] = (alphaPalette[a+1] & 0xFF) << 24;
		}

		// Create the new palette
		outPalette[0] = inPalette[0] & 0x00FFFFFF;
		for (i=1; i<=NUM_COLORS; i++) {
			for (a=0; a<NUM_ALPHAS; a++) {
				outPalette[NUM_ALPHAS*(i-1)+a+1] = (inPalette[i] & 0x00FFFFFF) | alpha[a];
			}
		}
		
		// Update the image indicies to point to the appropriate new palette entries
		for (i=0; i<width*height; i++) {
			a = alphaImage[i] - 1;
			if (a < 0) {
				outImage[i] = 0;
			} else {
				outImage[i] = NUM_ALPHAS*(inImage[i]-1)+a+1;
			}
		}
	}


	// Open the output file
	printf("Opening output file %s\n", outName );
	outFile = open( outName, _O_CREAT | _O_TRUNC | _O_WRONLY | _O_BINARY, _S_IREAD | _S_IWRITE );
	ShiAssert( outFile != -1 );

	// Write the magic number and the width and height to the file
	result = write( outFile, &magic, sizeof(magic) );
	ShiAssert( result == sizeof(magic) );
	result = write( outFile, &width, sizeof(width) );
	ShiAssert( result == sizeof(width) );
	result = write( outFile, &height, sizeof(height) );
	ShiAssert( result == sizeof(height) );

	// Write the palette data
	result = write( outFile, outPalette, 1024 );
	ShiAssert( result == 1024 );

	// Write the image data
	result = write( outFile, outImage, width * height );
	ShiAssert( result == width * height );


	// Close the output file
	close( outFile );

	// Release the output image buffers
	delete[] outPalette;
	delete[] outImage;

	// Release the input image buffers
	glReleaseMemory( inImage );
	glReleaseMemory( inPalette );
	if ( alphaImage ) {
		glReleaseMemory( alphaImage );
		glReleaseMemory( alphaPalette );
	}

	exit (0);
}


void ReadImage( char *filename, BYTE **image, DWORD **palette, WORD *width, WORD *height )
{
	CImageFileMemory 	texFile;
	DWORD				result;

	*image = NULL;
	*palette = NULL;

	// Make sure we recognize this file type
	texFile.imageType = CheckImageType( filename );
	ShiAssert( texFile.imageType != IMAGE_TYPE_UNKNOWN );

	// Open the input file
	result = texFile.glOpenFileMem( filename );
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
	*width = texFile.image.width;
	*height = texFile.image.height;
	ShiAssert(texFile.image.palette);

	*image = texFile.image.image;
	*palette = (DWORD*)texFile.image.palette;
}
