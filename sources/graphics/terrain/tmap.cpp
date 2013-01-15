/***************************************************************************\
    Tmap.cpp
    Scott Randolph
    August 21, 1995

    Top level class which manages the various levels of detail representing
    our map of terrain information.
\***************************************************************************/
#include "TimeMgr.h"
#include "TOD.h"
#include "TMap.h"
#include "falclib\include\openfile.h"

// Provide the one and only terrain database object.  It will be up to the
// application to initialize and cleanup this object by calling Setup and Cleanup.
TMap	TheMap;

void SetLatLong (float latitude, float longitude);
void ResetLatLong (void);
void GetLatLong (float *latitude, float *longitude);

// Number of feet between posts at highest detail - since we have one map, we can have on of these
float	FeetPerPost;
bool g_LargeTerrainFormat;
float g_MaximumTheaterAltitude;
bool g_LargeTheater;

extern float g_fLatitude;

int TMap::Setup( const char *mapPath )
{
	int		i=0;
	int		width=0, height=0;
	char	filename[MAX_PATH]={0};
	HANDLE	headerFile=0;
	DWORD	bytesRead=0;
	BOOL	retval=0;


	// Construct the filename for the map description file and open it
	strcpy( filename, mapPath );
	strcat( filename, "\\Theater.map" );
	headerFile = CreateFile_Open( filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if (headerFile == INVALID_HANDLE_VALUE) {
		//char	string[80];
		//char	message[120];
		//PutErrorString( string );
		//sprintf( message, "%s:  Couldn't open terrain header - disk error?", string );
		//int len = strlen( message );
		//ShiError( message );
		// We need to exit the game if they select cancel/abort from the file open dialog
		return(0);
	}


	// Read the number of feet between the highest detail posts in this map
	retval = ReadFile( headerFile, &FeetPerPost, sizeof(FeetPerPost), &bytesRead, NULL );
	if (( !retval ) || ( bytesRead != sizeof(FeetPerPost) )) {
		char	string[80];
		char	message[120];
		PutErrorString( string );
		sprintf( message, "%s:  Bad terrain header read (feet) - CD Error?", string );
		ShiError( message );
	}
	ShiAssert( (FeetPerPost > 0.0f) && (FeetPerPost < 50000.0f) );		// 50,000 is arbitrary, just want to check reasonableness here.


	// Read the width and height of the MEA table
	retval = ReadFile( headerFile, &MEAwidth,    sizeof(MEAwidth),    &bytesRead, NULL );
	retval = ReadFile( headerFile, &MEAheight,   sizeof(MEAheight),   &bytesRead, NULL );
	retval = ReadFile( headerFile, &FTtoMEAcell, sizeof(FTtoMEAcell), &bytesRead, NULL );
	if (( !retval ) || ( bytesRead != sizeof(FTtoMEAcell) )) {
		char	string[80];
		char	message[120];
		PutErrorString( string );
		sprintf( message, "%s:  Bad terrain header read (MEAsize) - CD Error?", string );
		ShiError( message );
	}


	// Read the number of levels we have available from the map header file
	retval = ReadFile( headerFile, &nLevels, sizeof(nLevels), &bytesRead, NULL );
	if (( !retval ) || ( bytesRead != sizeof(nLevels) )) {
		char	string[80];
		char	message[120];
		PutErrorString( string );
		sprintf( message, "%s:  Bad terrain header read (levels)", string );
		ShiError( message );
	}
	ShiAssert( (nLevels > 0) && (nLevels < 9) );		// 9 is arbitrary, just want to check reasonableness here.


	// Read the number of the last level which has conventional textures applied
	retval = ReadFile( headerFile, &lastNearTexturedLOD, sizeof(lastNearTexturedLOD), &bytesRead, NULL );
	if (( !retval ) || ( bytesRead != sizeof(lastNearTexturedLOD) )) {
		char	string[80];
		char	message[120];
		PutErrorString( string );
		sprintf( message, "%s:  Bad terrain header read (near textured LOD)", string );
		ShiError( message );
	}
	ShiAssert( (lastNearTexturedLOD >= 0) && (lastNearTexturedLOD < nLevels) );


	// Read the number of the last level which has far textures applied
	retval = ReadFile( headerFile, &lastFarTexturedLOD, sizeof(lastFarTexturedLOD), &bytesRead, NULL );
	if (( !retval ) || ( bytesRead != sizeof(lastFarTexturedLOD) )) {
		char	string[80];
		char	message[120];
		PutErrorString( string );
		sprintf( message, "%s:  Bad terrain header read (far textured LOD)", string );
		ShiError( message );
	}
	ShiAssert( (lastFarTexturedLOD >= lastNearTexturedLOD) && (lastFarTexturedLOD < nLevels) );


	// Read the map's color table
	LoadColorTable( headerFile );


	// Allocate memory for the map level objects
	Levels = new TLevel[ nLevels ];
	if (!Levels) {
		ShiError("Failed to allocate memory for map levels");
	}


	// Setup each level of the map
	for (i=0; i<nLevels; i++) {

		// Read the width and height in blocks of this level
		retval = ReadFile( headerFile, &width, sizeof(width), &bytesRead, NULL );
		if (( retval ) && ( bytesRead == sizeof(nLevels) )) {
			retval = ReadFile( headerFile, &height, sizeof(height), &bytesRead, NULL );
		}
		if (( !retval ) || ( bytesRead != sizeof(nLevels) )) {
			char	string[80];
			char	message[120];
			PutErrorString( string );
			sprintf( message, "%s:  Bad terrain header read - CD Error?", string );
			ShiError( message );
		}
		ShiAssert( (width  > 0) && (width  < 5000) );	// 1000 is arbitrary, just want to check reasonableness here.
		ShiAssert( (height > 0) && (height < 5000) );	// 1000 is arbitrary, just want to check reasonableness here.

		// Setup the level
		Levels[i].Setup( i, width, height, mapPath );
	}

	// extra stuff
	float latitude, longitude;
	retval = ReadFile( headerFile, &flags, sizeof(flags), &bytesRead, NULL );
	if (!retval  || bytesRead != sizeof(flags)) {
	    flags = 0;
	    ResetLatLong();
	}
	else {
	    retval = ReadFile( headerFile, &longitude, sizeof(longitude), &bytesRead, NULL );
	    retval = ReadFile( headerFile, &latitude, sizeof(latitude), &bytesRead, NULL );
	    if (retval && bytesRead == sizeof(latitude)) {
		SetLatLong (latitude, longitude);
	    }
	    else
		ResetLatLong();
	}
	GetLatLong(&latitude,&longitude);
	if (flags & TMAP_LARGETERRAIN) { // big indexes in use
	    g_LargeTerrainFormat = true;
	}
	else g_LargeTerrainFormat = false;
	if (flags & TMAP_LARGEUIMAP) { // 128x128 theater
		g_LargeTheater = true;
	}
	else g_LargeTheater = false;
	
	float maxtheateralt;
	retval = ReadFile( headerFile, &maxtheateralt, sizeof(maxtheateralt), &bytesRead, NULL );
	if (!retval  || bytesRead != sizeof(maxtheateralt)) {
	    g_MaximumTheaterAltitude = 12000.0F;
	}

	g_MaximumTheaterAltitude = -g_MaximumTheaterAltitude; // ZPos is negative

	// Close the map description file
	CloseHandle( headerFile );
	

	// Load the course height table
	LoadMEAtable( mapPath );


	// Initialize the lighting conditions and register for future time of day updates
	TimeUpdateCallback( this );
	TheTimeManager.RegisterTimeUpdateCB( TimeUpdateCallback, this );


	// Now we're ready to run
	initialized = TRUE;
	return nLevels;
}


void TMap::Cleanup( void )
{
	// Note that we're shutting down
	initialized = FALSE;

	// Stop receiving time updates
	TheTimeManager.ReleaseTimeUpdateCB( TimeUpdateCallback, this );

	// Wait for the loader queue to be empty so it won't send us more data
	TheLoader.WaitForLoader();

	// Cleanup each level of the map
	while (nLevels-- > 0) {
		Levels[nLevels].Cleanup();
	}

	// Now release the memory used by the levels
	delete[] Levels;
	Levels = NULL;

	// Free the rought height table (MEA)
	delete[] MEAarray;
	MEAarray = NULL;
}


void TMap::LoadColorTable( HANDLE inputFile )
{
	UInt32	bytesRead;
	BOOL	retval;
	UInt32	*packedSrc;
	Tcolor	*src, *dst, *end;
	UInt32	palette[256];

	// Read the original color data
	retval = ReadFile( inputFile, palette, sizeof(palette), &bytesRead, NULL );
	if (( !retval ) || ( bytesRead != sizeof(palette) )) {
		char	string[80];
		char	message[120];
		PutErrorString( string );
		sprintf( message, "%s:  Bad terrain header read (color table)", string );
		ShiError( message );
	}

	// Convert from 4 byte packed to floating format
	packedSrc = palette;
	dst = ColorTable;
	end = ColorTable+256;
	while (dst < end) {
		dst->r = ((*packedSrc)       & 0xFF) / 255.0f;
		dst->g = ((*packedSrc >> 8)  & 0xFF) / 255.0f;
		dst->b = ((*packedSrc >> 16) & 0xFF) / 255.0f;
		packedSrc++;
		dst++;
	}

	// Construct the green version
	src = ColorTable;
	dst = GreenTable;
	end = ColorTable+256;
	while (src < end) {
		dst->r = 0.0f;
		dst->g = src->r*0.25f + src->g*0.5f + src->b*0.25f;
		dst->b = 0.0f;
		src++;
		dst++;
	}

	// The lit versions will be created on the first time update callback...
}


void TMap::LoadMEAtable( const char *mapPath )
{
	char	filename[MAX_PATH];
	HANDLE	dataFile;
	DWORD	bytesRead;
	BOOL	retval;
	Int16	*target;
	int		row;

	
	// Constuct the storage for the array
	MEAarray = new Int16[ MEAwidth * MEAheight ];
	ShiAssert( MEAarray );

	// Open the MEA data file
	strcpy( filename, mapPath );
	strcat( filename, "\\Theater.MEA" );
	dataFile = CreateFile_Open( filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if (dataFile == INVALID_HANDLE_VALUE) {
		//char	string[80];
		//char	message[120];
		//PutErrorString( string );
		//sprintf( message, "%s:  Couldn't open MEA table - disk error?", string );
		//int len = strlen( message );
		//ShiError( message );
		// We need to exit the game if they select cancel/abort from the file open dialog
	}


	// Read in the MEA data (vertical flip in the process)
	for (row=MEAheight-1; row>=0; row--) {
		target = &MEAarray[row*MEAwidth];
		retval = ReadFile( dataFile, target, MEAwidth*sizeof(*MEAarray), &bytesRead, NULL );
		if (( !retval ) || ( bytesRead != MEAwidth*sizeof(*MEAarray) )) {
			char	string[80];
			char	message[120];
			PutErrorString( string );
			sprintf( message, "%s:  Bad MEA table read - CD Error?", string );
			ShiError( message );
		}
	}

	// Close the input file
	CloseHandle( dataFile );
}


float TMap::GetMEA( float FTnorth, float FTeast )
{
	int r, c;


	ShiAssert( IsReady() );

	// Convert to the units used by the MEA array
	r = FloatToInt32( FTnorth * FTtoMEAcell );
	c = FloatToInt32( FTeast  * FTtoMEAcell );

	// Snap onto the map
	r = min( max( r, 0 ), MEAheight-1);
	c = min( max( c, 0 ), MEAwidth-1);

	// Return the requested value
	return MEAarray[ r*MEAwidth + c ];
}


// Update the sky colors and sun/moon position based on the current time of day
void TMap::TimeUpdateCallback( void *self ) {
	((TMap*)self)->UpdateLighting();
}
void TMap::UpdateLighting( void )
{
	Tcolor	lightColor;
	Tcolor	*src, *dst, *end;

	// Get the light level from the time of day manager
	TheTimeOfDay.GetTextureLightingColor( &lightColor );

	// Light the color table
	src = ColorTable;
	dst = DarkColorTable;
	end = ColorTable+256;
	while (src < end) {
		dst->r = src->r * lightColor.r;
		dst->g = src->g * lightColor.g;
		dst->b = src->b * lightColor.b;
		src++;
		dst++;
	}

	// Light the green table
	src = GreenTable;
	dst = DarkGreenTable;
	end = GreenTable+256;
	while (src < end) {
		dst->g = src->g * lightColor.g;
		src++;
		dst++;
	}
}
