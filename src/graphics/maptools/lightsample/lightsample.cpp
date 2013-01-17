/*******************************************************************************\
	LightSample.cpp
	
	This tool inserts night lights into the multi-resolution terrain tiles
	for Falcon 4.0

	Scott Randolph
	MicroProse
	June 1, 1998
\*******************************************************************************/
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <shi\ShiError.h>
#include "..\..\3Dlib\Image.h"


#define NUM_LIGHT_COLORS	4


void  ReadImage(  char *filename, BYTE **image, DWORD **palette, WORD *width, WORD *height );
void  WriteImage( char *filename, BYTE *image,  DWORD  *palette, WORD width,  WORD height );
void  AddLights( BYTE *texImage, BYTE *lightImage, WORD texWidth, WORD texHeight );
BYTE* DownSampleLights( BYTE *lightImage, WORD lightWidth, WORD lightHeight );


void main(int argc, char* argv[]) {
	char	baseName[_MAX_PATH];
	char	fileName[_MAX_PATH];

	WORD	lightWidth;
	WORD	lightHeight;
	BYTE	*lightImage;
	DWORD	*lightPalette;

	char	texPrefix;

	WORD	texWidth;
	WORD	texHeight;
	BYTE	*texImage;
	DWORD	*texPalette;


	// If we didn't get the right number of arguments, print a usage message
	if (argc != 3) {
		printf("Usage:  LightSample <name> <lightName>\n");
		printf("    Reads <lightName>.pcx to get light positions.  Edits them into\n");
		printf("    H<name>.pcx, M<name>.pcx, L<name>.pcx, and T<name>.pcx\n");
		printf("	NOTE:  This tool requires that the input and output images be\n");
		printf("	       in the current working directory.\n");
		exit(-1);
	}

	// Construct the name of the texture images
	strcpy( fileName, argv[2] );
	strcat( fileName, ".PCX" );

	// Read the light image file
	ReadImage( fileName, &lightImage, &lightPalette, &lightWidth, &lightHeight );
	ShiAssert( lightImage );
	ShiAssert( lightPalette );

	// Start the loop that handles each texture size
	texPrefix = 'H';
	while (texPrefix != '\0') {

		// Construct the name of the texture image to edit
		strcpy( baseName, argv[1] );
		fileName[0] = texPrefix;
		fileName[1] = '\0';
		strcat( fileName, baseName );
		strcat( fileName, ".PCX" );

		// Read the texture image file
		ReadImage( fileName, &texImage, &texPalette, &texWidth, &texHeight );
		ShiAssert( texImage );
		ShiAssert( texPalette );

		// Call the light insertion function
		ShiAssert( texWidth  == lightWidth );
		ShiAssert( texHeight == lightHeight );
		AddLights( texImage, lightImage, texWidth, texHeight );

		// Write the edited version of the texture image
		WriteImage( fileName, texImage, texPalette, texWidth, texHeight );
	
		// Decide on what the next texture prefix is to be
		switch( texPrefix ) {
		case 'H':
			texPrefix = 'M';
			lightImage = DownSampleLights( lightImage, lightWidth, lightHeight );
			lightWidth  /= 2;
			lightHeight /= 2;
			break;
		case 'M':
			texPrefix = 'L';
			lightImage = DownSampleLights( lightImage, lightWidth, lightHeight );
			lightWidth  /= 2;
			lightHeight /= 2;
			break;
		case 'L':
			texPrefix = 'T';
			lightImage = DownSampleLights( lightImage, lightWidth, lightHeight );
			lightWidth  /= 2;
			lightHeight /= 2;
			break;
		default:
			texPrefix = '\0';
			break;
		}
	}


	// Release the light image buffers
	glReleaseMemory( lightImage );
	glReleaseMemory( lightPalette );

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
		ShiError( "Failed to read terrain texture." );
	}


	// Store the image properties in our local storage
	*width = texFile.image.width;
	*height = texFile.image.height;
	ShiAssert(texFile.image.palette);

	*image = texFile.image.image;
	*palette = (DWORD*)texFile.image.palette;
}


void WriteImage( char *filename, BYTE *image,  DWORD  *palette, WORD width,  WORD height )
{
	int			fileHandle;
	GLImageInfo	imageInfo;

	// Make sure we recognize this file type
	ShiAssert( CheckImageType( filename ) != IMAGE_TYPE_UNKNOWN );

	// Load up the input data structure
	imageInfo.width = width;
	imageInfo.height = height;
	imageInfo.image = image;
	imageInfo.palette = (GLuint*)palette;

	// Create the output file
	printf("Opening output file %s\n", filename );
	fileHandle = open( filename, _O_CREAT | _O_TRUNC | _O_WRONLY | _O_BINARY, _S_IREAD | _S_IWRITE );
	ShiAssert( fileHandle != -1 );

	// Write the data
	WritePCX( fileHandle, &imageInfo );

	// Close the output file
	close( fileHandle );
}


void AddLights( BYTE *texImage, BYTE *lightImage, WORD texWidth, WORD texHeight )
{
	int		r;
	int		c;

	for (r=0; r<texHeight; r++) {
		for (c=0; c<texWidth; c++) {
			if (lightImage[r*texWidth+c] > 255 - NUM_LIGHT_COLORS) {
				texImage[r*texWidth+c] = lightImage[r*texWidth+c];
			}
		}
	}
}


// Take the current image and reduce its size in x and y by a factor of two
BYTE* DownSampleLights( BYTE *lightImage, WORD lightWidth, WORD lightHeight )
{
	int		w;
	int		h;
	int		r;
	int		c;
	BYTE	val;
	BYTE	*newImage;

	// Reduce the size of the light image by two in each dimension
	w = lightWidth  / 2;
	h = lightHeight / 2;

	// Allocate space for the new image
	newImage = new BYTE[w*h];

	// Down sample the lights
	for (r=0; r<h; r++) {
		for (c=0; c<w; c++) {

			// For now we pick the highest valued pixel as the new downsampled value.
			val = max( max( lightImage[r*2*lightWidth + c*2],     lightImage[(r*2+1)*lightWidth + c*2] ),
					   max( lightImage[r*2*lightWidth + (c*2+1)], lightImage[(r*2+1)*lightWidth + (c*2+1)] ));

			newImage[r*w+c] = val;
		}
	}

	// Release the old image
	glReleaseMemory( lightImage );

	// Return the new image
	return newImage;
}
