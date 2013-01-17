/*******************************************************************************\
	Map Converter from BMP color and RAW elevation to Falcon 4.0 LOD format

	Scott Randolph
	Spectrum HoloByte
	November 14, 1995
\*******************************************************************************/
#include <stdio.h>
#include <math.h>
#include "..\..\Terrain\Ttypes.h"
#include "..\..\Terrain\TPost.h"
#include "..\..\Terrain\TDskPost.h"
#include "..\..\3Dlib\Image.h"


#define 		MAX_LEVELS				6		// How many levels of detail to generate
#define			LAST_TEX_LEVEL			2		// What is the number of the last level to be textured

const float		altScale	= 8.0f * FEET_PER_METER;	// Must match units in *-E.RAW file

float			FeetPerPost = (FEET_PER_KM / 4.0f);		// I've got 250m posts at the moment
const int		MEA_DOWNSAMPLE_SHIFT = 5;				// From 250m to 8km
const float		FeetToMEAcell = 1.0f / (FeetPerPost * (1<<MEA_DOWNSAMPLE_SHIFT));


static const WORD	INVALID_TEXID	= 0xFFFF;


int main(int argc, char* argv[]) {

	BYTE		*ColorIndexBuffer	= NULL;
	DWORD		*ColorPaletteBuffer	= NULL;
	BYTE		*ElevationBuffer	= NULL;
	WORD		*TexIDBuffer		= NULL;
	WORD		*FarFieldBuffer		= NULL;
	WORD		*NormalBuffer		= NULL;
	TdiskPost	*postBuffer			= NULL;
	TdiskPost	*postBufferPrev		= NULL;

	DWORD		ElevationBufferSize;
	DWORD		TexIDBufferSize;
	DWORD		FarFieldBufferSize;
	DWORD		NormalBufferSize;
	DWORD		postBufferSize;

	CImageFileMemory	colorFile;

	HANDLE		elevationFile;
	HANDLE		textureFile;
	HANDLE		farFieldFile;
	HANDLE		headerFile;
	HANDLE		postFile;
	HANDLE		offsetFile;

	int			bufferWidth;
	int			bufferHeight;

	Int16		MEAvalue;
	int			MEAwidth;
	int			MEAheight;

	int			texMapWidth;
	int			texMapHeight;

	int			farMapWidth;
	int			farMapHeight;

	OPENFILENAME dialogInfo;
	char		filename[256];
	char		dataRootDir[256];
	char		dataSet[256];
	char		texPath[256];

	int			row;
	int			col;
	int			r;
	int			c;

	int			top;
	int			left;
	int			bottom;
	int			right;

	int			LOD;
	int			blockRow;
	int			blockCol;
	DWORD		dataOffset;

	int			LastFarTexLevel;
	DWORD		fileOffset = 0xFFFFFFFF;
	DWORD		bytes;
	int			result;


	// See if we got a filename on the command line
	if ( argc == 2) {
		result = GetFullPathName( argv[1], sizeof( filename ), filename, NULL );
	} else {
		result = 0;
	}

	// If we didn't get it on the command line, ask the user
	// for the name of the BMP color file
	if (!result) {
		filename[0] = NULL;
		dialogInfo.lStructSize = sizeof( dialogInfo );
		dialogInfo.hwndOwner = NULL;
		dialogInfo.hInstance = NULL;
		dialogInfo.lpstrFilter = "24 Bit BMP\0*-C.GIF\0\0";
		dialogInfo.lpstrCustomFilter = NULL;
		dialogInfo.nMaxCustFilter = 0;
		dialogInfo.nFilterIndex = 1;
		dialogInfo.lpstrFile = filename;
		dialogInfo.nMaxFile = sizeof( filename );
		dialogInfo.lpstrFileTitle = NULL;
		dialogInfo.nMaxFileTitle = 0;
		dialogInfo.lpstrInitialDir = "J:\\TerrData";
		dialogInfo.lpstrTitle = "Select a base GIF file (*-C.GIF)";
		dialogInfo.Flags = OFN_FILEMUSTEXIST;
		dialogInfo.lpstrDefExt = "GIF";

 		if ( !GetOpenFileName( &dialogInfo ) ) {
			return -1;
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
	sprintf( filename, "%s\\terrain\\%s-C.GIF", dataRootDir, dataSet );
	printf( "Reading COLOR file %s\n", filename );
	colorFile.imageType = CheckImageType( filename );
	ShiAssert( colorFile.imageType != IMAGE_TYPE_UNKNOWN );
	result = colorFile.glOpenFileMem( filename );
	if ( result != 1 ) {
		char	message[256];
		sprintf( message, "Failed to open %s", filename );
		ShiError( message );
	}

	// Read the image data (note that ReadTextureImage will close texFile for us)
	colorFile.glReadFileMem();
	result = ReadTextureImage( &colorFile );
	if (result != GOOD_READ) {
		ShiError( "Failed to read terrain texture.  CD Error?" );
	}
	ShiAssert(colorFile.image.image);
	ShiAssert(colorFile.image.palette);

	// Store the image data
	bufferWidth			= colorFile.image.width;
	bufferHeight		= colorFile.image.height;
	ColorIndexBuffer	= colorFile.image.image;
	ColorPaletteBuffer	= (DWORD*)colorFile.image.palette;

	
	// Allocate space for the elevation buffer
	ElevationBufferSize = bufferWidth*bufferHeight*sizeof(*ElevationBuffer);
	ElevationBuffer = (BYTE*)malloc( ElevationBufferSize );
	ShiAssert( ElevationBuffer );

	// Open the elevation information file
	sprintf( filename, "%s\\terrain\\%s-E.RAW", dataRootDir, dataSet );
	printf( "Reading ELEVATION file %s\n", filename );
    elevationFile = CreateFile( filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( elevationFile == INVALID_HANDLE_VALUE ) {
		char string[256];
		PutErrorString( string );
		strcat( string, "Failed to open elevation file." );
		ShiError( string );
	}

	// Read in the data
	if ( !ReadFile( elevationFile, ElevationBuffer, ElevationBufferSize, &bytes, NULL ) )  bytes=0xFFFFFFFF;
	if ( bytes != ElevationBufferSize ) {
		char string[256];
		PutErrorString( string );
		strcat( string, "Couldn't read required elevation data." );
		ShiError( string );
	}
	CloseHandle( elevationFile );



	// Store the size of the MEA table we're going to build
	MEAwidth	= bufferWidth  >> MEA_DOWNSAMPLE_SHIFT;
	MEAheight	= bufferHeight >> MEA_DOWNSAMPLE_SHIFT;

	// Open the MEA table output file
	sprintf( filename, "%s\\terrain\\Theater.MEA", dataRootDir );
	printf( "Writing the MEA table %s\n", filename );
	headerFile = CreateFile( filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if( headerFile == INVALID_HANDLE_VALUE ) {
		char string[256];
		PutErrorString( string );
		strcat( string, "Failed to open MEA table output file." );
		ShiError( string );
	}

	// Store the maximum height encountered in each MEA cell on the map
	for (row=0; row < MEAheight; row++) {

		for (col=0; col < MEAwidth; col++) {
			MEAvalue = -32000;

			top		= row<<MEA_DOWNSAMPLE_SHIFT;
			left	= col<<MEA_DOWNSAMPLE_SHIFT;
			bottom	= ((row+1)<<MEA_DOWNSAMPLE_SHIFT) - 1;
			right	= ((col+1)<<MEA_DOWNSAMPLE_SHIFT) - 1;


			// Search for the maximum height within this MEA cell
			for (r = top; r <= bottom; r++) {
				for (c = left; c <= right; c++) {
		 			dataOffset = r*bufferWidth + c;
					MEAvalue = max( MEAvalue, (Int16)(ElevationBuffer[dataOffset] * altScale) );
				}
			}

			// Now we look one post outward (if we're not at an edge)
			if ((row>0) && (row<MEAheight-1) && (col>0) && (col<MEAwidth-1)) {
				for (c = left-1; c <= right+1; c++) {
		 			dataOffset = (top-1)*bufferWidth + c;
					MEAvalue = max( MEAvalue, (Int16)(ElevationBuffer[dataOffset] * altScale) );
		 			dataOffset = (top+1)*bufferWidth + c;
					MEAvalue = max( MEAvalue, (Int16)(ElevationBuffer[dataOffset] * altScale) );
				}
				for (r = top; r <= bottom; r++) {
	 				dataOffset = r*bufferWidth + (left-1);
					MEAvalue = max( MEAvalue, (Int16)(ElevationBuffer[dataOffset] * altScale) );
	 				dataOffset = r*bufferWidth + (left+1);
					MEAvalue = max( MEAvalue, (Int16)(ElevationBuffer[dataOffset] * altScale) );
				}
			}

			// Write out this element of the MEA table
			WriteFile( headerFile, &MEAvalue, sizeof(MEAvalue), &bytes, NULL );
			ShiAssert( bytes == sizeof(MEAvalue) );
		}
	}

	// Close the MEA table file
	CloseHandle( headerFile );



	// Allocate space for the surface normal buffer
	NormalBufferSize = bufferWidth*bufferHeight*sizeof(*NormalBuffer);
	NormalBuffer = (WORD*)malloc( NormalBufferSize );
	ShiAssert( NormalBuffer );

	// Compute the normal at each post based on its neighbors
	printf("Computing the surface normal at each post\n");
	for (r=0; r < bufferHeight; r++) {
		for (c=0; c < bufferWidth; c++) {

 			dataOffset = r*bufferWidth + c;
			ShiAssert( dataOffset < (DWORD)bufferWidth*bufferHeight );
			ShiAssert( dataOffset >= 0 );


			// Compute the cartesian components of the surface normal based on
			// the known post spacing and the height changes between the neighbors
			double Nx, Ny, Nz;
			double normalizer;

			// Start with the height changes (rise) in x direction and in the y direction
			// At the edges of the map, just use a normal pointing straight up for now.
			if ( (r == 0) || (c == 0) || (r == bufferHeight-1) || (c == bufferWidth-1) ) {
				Nx = 0.0f;
				Ny = 0.0f;
			} else {
				ShiAssert( dataOffset+bufferWidth < (DWORD)bufferWidth*bufferHeight );
				ShiAssert( dataOffset-bufferWidth >= 0 );
				Nx = altScale * (ElevationBuffer[dataOffset+bufferWidth] - ElevationBuffer[dataOffset-bufferWidth]);
				Ny = altScale * (ElevationBuffer[dataOffset-1]           - ElevationBuffer[dataOffset+1]);
			}
			Nz = GLOBAL_POST_TO_WORLD( 2 );

			// Now normalize the vector
			normalizer = 1.0 / sqrt(Nx*Nx + Ny*Ny + Nz*Nz);
			Nx *= normalizer;
			Ny *= normalizer;
			Nz *= normalizer;

	
			// Now store the normal in spherical coordinates (unit vector, so rho = 1)			
			double theta;
			double phi;

			// Convert from catesian to spherical coordinates
			phi		= asin( Nz );
			ShiAssert( phi <= PI/2.0 );
			ShiAssert( phi >= 0.0 );

			// BUG IN ATAN2 -- fails when both args are 0.0, therefore...
			if ( fabs(Nx) < 0.000001 ) {
				if ( Ny < 0.0 ) {
					theta = -PI / 2.0;
				} else {
					theta =  PI / 2.0;
				}
			} else {
				theta	= atan2( Ny, Nx );
			}
			if (theta < 0.0)  theta += PI*2.0;
			ShiAssert( theta < PI*2.0 );
			ShiAssert( theta >= 0.0 );

			// Scale theta from 0 - 2 PI (360 degrees) to 0 - 255
			static const double thetaInStart	= 0.0;
			static const double thetaInStop		= PI*2.0;
			static const double thetaInRange	= thetaInStop - thetaInStart;
			static const double thetaOutScale	= 255.99;
			theta = thetaOutScale * (theta - thetaInStart) / thetaInRange;

			// Scale phi from 1.3 - PI/2 to 0 - 63
			static const double phiInStart	= 1.3;
			static const double phiInStop	= PI/2.0;
			static const double phiInRange	= phiInStop - phiInStart;
			static const double phiOutScale	= 63.99;
			phi = phiOutScale * (phi - phiInStart) / phiInRange;
			if (phi < 0.0)  phi = 0.0;


			ShiAssert( theta <  256.0f );
			ShiAssert( phi   <   64.0f );
			ShiAssert( theta >=   0.0f );
			ShiAssert( phi   >=   0.0f );
 
 			// Store a compressed version of theta and phi for later use with this post
 			NormalBuffer[dataOffset] = (((BYTE)floor(phi)) << 8) | (BYTE)floor(theta);
		}
	}


	// Open a file to store the per-post normal information for use by the campaign engine
	sprintf( filename, "%s\\terrain\\%s-N.RAW", dataRootDir, dataSet );
	printf( "Writing the normal inspection file %s\n", filename );
	headerFile = CreateFile( filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if( headerFile == INVALID_HANDLE_VALUE ) {
		char string[256];
		PutErrorString( string );
		strcat( string, "Failed to open normal output file." );
		ShiError( string );
	}

	// Write out the encoded representations of the normals at each post
	WriteFile( headerFile, NormalBuffer, NormalBufferSize, &bytes, NULL );
	if( bytes != (DWORD)NormalBufferSize ) {
		char string[256];
		PutErrorString( string );
		strcat( string, "Failed to write normal output file." );
		ShiError( string );
	}
	ShiAssert( bytes == (DWORD)NormalBufferSize );

	// Close the normal file
	CloseHandle( headerFile );


	// Allocate space for the texture ID buffer
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
	CloseHandle( textureFile );


	// See how many LODs have far field texture information available
	LastFarTexLevel = LAST_TEX_LEVEL;
	for (LOD = LAST_TEX_LEVEL+1; LOD < MAX_LEVELS; LOD++) {
		
		// Try to open the texture ID file for this LOD
		sprintf( filename, "%s\\terrain\\FarTiles.%0d", dataRootDir, LOD );
		printf( "Checking for far field file %s\n", filename );
		farFieldFile = CreateFile( filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
		if( farFieldFile != INVALID_HANDLE_VALUE ) {
			printf( "Found far field data for LOD %0d\n", LOD );
			LastFarTexLevel = LOD;
			CloseHandle( farFieldFile );
		} else {
			break;
		}
	}


	// Open the map header file
	sprintf( filename, "%s\\terrain\\Theater.MAP", dataRootDir );
	printf( "Writing the map header file %s\n", filename );
	headerFile = CreateFile( filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if( headerFile == INVALID_HANDLE_VALUE ) {
		char string[256];
		PutErrorString( string );
		strcat( string, "Failed to open map heaader file." );
		ShiError( string );
	}


	// Write out the number of feet between map posts at the highest detail level
	LOD = MAX_LEVELS;
	WriteFile( headerFile, &FeetPerPost, sizeof(FeetPerPost), &bytes, NULL );
	ShiAssert( bytes == sizeof(FeetPerPost) );

	// Write out the width and height of the MEA table
	WriteFile( headerFile, &MEAwidth,  sizeof(MEAwidth),  &bytes, NULL );
	ShiAssert( bytes == sizeof(MEAwidth) );
	WriteFile( headerFile, &MEAheight, sizeof(MEAheight), &bytes, NULL );
	ShiAssert( bytes == sizeof(MEAheight) );
	WriteFile( headerFile, &FeetToMEAcell, sizeof(FeetToMEAcell), &bytes, NULL );
	ShiAssert( bytes == sizeof(FeetToMEAcell) );

	// Write out the number of levels we'll be generating for this map
	LOD = MAX_LEVELS;
	WriteFile( headerFile, &LOD, sizeof(LOD), &bytes, NULL );
	ShiAssert( bytes == sizeof(LOD) );

	// Write out the number of the last level which will have near format textures
	LOD = LAST_TEX_LEVEL;
	WriteFile( headerFile, &LOD, sizeof(LOD), &bytes, NULL );
	ShiAssert( bytes == sizeof(LOD) );

	// Write out the number of the last level which will have far format composite textures
	LOD = LastFarTexLevel;
	WriteFile( headerFile, &LOD, sizeof(LOD), &bytes, NULL );
	ShiAssert( bytes == sizeof(LOD) );

	// Write the map's color table
	WriteFile( headerFile, ColorPaletteBuffer, 256*sizeof(*ColorPaletteBuffer), &bytes, NULL );
	ShiAssert( bytes == 256*sizeof(*ColorPaletteBuffer) );


	// Allocate the memory for each disk block as it is constructed
	postBufferSize = POSTS_PER_BLOCK*sizeof(*postBuffer);
	postBuffer		= (TdiskPost*)malloc( postBufferSize );
	postBufferPrev	= (TdiskPost*)malloc( postBufferSize );
	ShiAssert( postBuffer );
	ShiAssert( postBufferPrev );



	// Write the post data for each level
	for (LOD=0; LOD < MAX_LEVELS; LOD++) {

		int blockStartRow;
		int blockStartCol;
		int blockSpan = POSTS_ACROSS_BLOCK << LOD;
		int postStep = 1 << LOD;

		ShiAssert( blockSpan < bufferWidth );
		ShiAssert( blockSpan < bufferHeight );


		// See if any far field texture offset information is available for this LOD
		if ((LOD > LAST_TEX_LEVEL) && (LOD <= LastFarTexLevel)) {
			free( FarFieldBuffer );
			FarFieldBuffer = NULL;
			sprintf( filename, "%s\\terrain\\FarTiles.%0d", dataRootDir, LOD );
			farFieldFile = CreateFile( filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
			if( farFieldFile == INVALID_HANDLE_VALUE ) {

				printf( "Failed to open %s\n", filename );

			} else {

				printf( "Reading far field data for LOD %0d\n", LOD );

				// Allocate space for the far field texture offset buffer
				farMapWidth			= bufferWidth >> LOD;
				farMapHeight		= bufferHeight >> LOD;
				FarFieldBufferSize	= farMapWidth*farMapHeight*sizeof(*FarFieldBuffer);
				FarFieldBuffer		= (WORD*)malloc( FarFieldBufferSize );
				ShiAssert( FarFieldBuffer );

				// Read in the data
				if ( !ReadFile( farFieldFile, FarFieldBuffer, FarFieldBufferSize, &bytes, NULL ) )  bytes=0xFFFFFFFF;
				if ( bytes != FarFieldBufferSize ) {
					char string[256];
					PutErrorString( string );
					strcat( string, "Couldn't read far field texture offset data." );
					ShiError( string );
				}
				CloseHandle( farFieldFile );
			}
		}

		
		// Open this level's post file
		sprintf( filename, "%s\\terrain\\Theater.L%0d", dataRootDir, LOD );
		printf( "Generating POST file %s\n", filename );
		postFile = CreateFile( filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
		if( postFile == INVALID_HANDLE_VALUE ) {
			char string[256];
			PutErrorString( string );
			strcat( string, "Failed to open post output file." );
			ShiError( string );
		}

		// Open this level's block offset file
		sprintf( filename, "%s\\terrain\\Theater.O%0d", dataRootDir, LOD );
		offsetFile = CreateFile( filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
		if( offsetFile == INVALID_HANDLE_VALUE ) {
			char string[256];
			PutErrorString( string );
			strcat( string, "Failed to open block offset file." );
			ShiError( string );
		}

		// For each row of blocks
		for ( blockStartRow = 0; blockStartRow + blockSpan <= bufferHeight; blockStartRow += blockSpan ) {

			blockRow = blockStartRow / blockSpan;

			// For each colomn of blocks
			for ( blockStartCol = 0; blockStartCol + blockSpan <= bufferWidth; blockStartCol += blockSpan ) {

				blockCol = blockStartCol / blockSpan;

				// Start at the head of the post buffer
				TdiskPost *post = postBuffer;
				
				// For each row in the block
				for (row=0; row < blockSpan; row+=postStep) {

					// For each column in the block
					for(col=0; col < blockSpan; col+=postStep) {

						ShiAssert( post-postBuffer < (int)(postBufferSize/sizeof(*post)) );

						dataOffset = ((bufferHeight-1)-(row+blockStartRow))*bufferWidth + (col+blockStartCol);

						// Store this point's data into the post array
			        	post->z			= (Int16)(ElevationBuffer[dataOffset] * altScale);
						post->color		= ColorIndexBuffer[dataOffset];
						post->theta		= NormalBuffer[dataOffset] & 0xFF;
						post->phi		= NormalBuffer[dataOffset] >> 8;

						// Fill in appropriate texture information
						if (LOD <= LAST_TEX_LEVEL) {
							int texOffset = (((bufferHeight-1)-(row+blockStartRow))>>LAST_TEX_LEVEL)*texMapWidth +
											((col+blockStartCol)>>LAST_TEX_LEVEL);

							// Figure out the right texture ID (includes Mipmap effects)
							post->texID	= TexIDBuffer[ texOffset ] | ((LAST_TEX_LEVEL-LOD)<<12);
						} else {
							int farFieldOffset = (((bufferHeight-1)-(row+blockStartRow))>>LOD)*farMapWidth +
												 ((col+blockStartCol)>>LOD);

							if (FarFieldBuffer) {
								post->texID	= FarFieldBuffer[farFieldOffset];
							} else {
								post->texID	= INVALID_TEXID;
							}
						}

						// Move on to the next post
						post++;

					}
				}


				// See if we can reuse the previously written block
				if ((fileOffset != 0xFFFFFFFF) && (memcmp(postBuffer, postBufferPrev, postBufferSize)==0)) {
					// Write the file offset at which a duplicate of this block is already stored
					WriteFile( offsetFile, &fileOffset, sizeof( fileOffset ), &bytes, NULL );
					if ( bytes != sizeof( fileOffset) ) {
						char string[256];
						PutErrorString( string );
						strcat( string, "Failed to write block offset information." );
						ShiError( string );
					}
				} else {
					// Get the file offset to which we're going to write the block
					fileOffset = SetFilePointer( postFile, 0, NULL, FILE_CURRENT );
					ShiAssert( fileOffset != 0xFFFFFFFF );

					// Write the file offset at which this block will be stored
					WriteFile( offsetFile, &fileOffset, sizeof( fileOffset ), &bytes, NULL );
					if ( bytes != sizeof( fileOffset) ) {
						char string[256];
						PutErrorString( string );
						strcat( string, "Failed to write block offset information." );
						ShiError( string );
					}

					// Now write the full block worth of posts to the post file
					WriteFile( postFile, postBuffer, postBufferSize, &bytes, NULL );
					if ( bytes != postBufferSize ) {
						char string[256];
						PutErrorString( string );
						strcat( string, "Failed to write posts data." );
						ShiError( string );
					}

					// Save the post data for next time
					TdiskPost *temp = postBufferPrev;
					postBufferPrev = postBuffer;
					postBuffer = temp;
				}
			}
		}

		// Close the post file
		CloseHandle( postFile );

		// Close the block offset file
		CloseHandle( offsetFile );

		// Write the dimensions of this level to the map header file
		int blocksWide = (bufferWidth)  / (blockSpan);
		int blocksHigh = (bufferHeight) / (blockSpan);

		WriteFile( headerFile, &blocksWide, sizeof(blocksWide), &bytes, NULL );
		ShiAssert( bytes == sizeof(blocksWide) );

		WriteFile( headerFile, &blocksHigh, sizeof(blocksHigh), &bytes, NULL );
		ShiAssert( bytes == sizeof(blocksHigh) );


		// TODO:  Down sample the color and elevation better than just decimation
	}


	// Close the header and post files currently open
	printf( "Closing down\n" );
	CloseHandle( headerFile );


	// Release the color and elevation buffers
	free( postBuffer );
	free( postBufferPrev );
	free( TexIDBuffer );
	free( FarFieldBuffer );
	free( NormalBuffer );
	free( ElevationBuffer );
	glReleaseMemory( ColorIndexBuffer );
	glReleaseMemory( ColorPaletteBuffer );


	// Say we're done
	printf( "Operation Complete:  Map Conversion Successful.\n" );
	return 0;
}
