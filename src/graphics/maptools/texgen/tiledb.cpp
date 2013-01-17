/*******************************************************************************\
	TileDB.cpp
	Provides an interface to the tile database written by the tile tool.  At 
	present this is only used by the TexGen tool.

	Scott Randolph
	Spectrum HoloByte
	October 8, 1996
\*******************************************************************************/
#include <stdio.h>
#include <math.h>
#include "TileDB.h"


// Terrain and feature types defined in the the visual basic tile tool
// (Not all are features, but we have the whole list here for completeness sake)
const int COVERAGE_WATER		= 1;
const int COVERAGE_RIVER		= 2;
const int COVERAGE_SWAMP		= 3;
const int COVERAGE_PLAINS		= 4;
const int COVERAGE_BRUSH		= 5;
const int COVERAGE_THINFOREST	= 6;
const int COVERAGE_THICKFOREST	= 7;
const int COVERAGE_ROCKY		= 8;
const int COVERAGE_URBAN		= 9;
const int COVERAGE_ROAD			= 10;
const int COVERAGE_RAIL			= 11;
const int COVERAGE_BRIDGE		= 12;
const int COVERAGE_RUNWAY		= 13;
const int COVERAGE_STATION		= 14;


void TileDatabase::Load( char *filename ) {
	HANDLE		inputFile;
	DWORD		bytes;
	char		message[80];
	int			tile;


	// Open the texture tile database for reading
	inputFile = CreateFile( filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if (inputFile == INVALID_HANDLE_VALUE) {
		PutErrorString( message );
		strcat( message, ":  Failed tile database open" );
		ShiError( message );
	}


	// Read the file header
	ReadFile( inputFile, &header, 29, &bytes, NULL );
	if ( bytes != 29 ) {
		PutErrorString( message );
		strcat( message, ":  Failed file header read" );
		ShiError( message );
	}


	// Verify the file header
	if ( strncmp( header.title, "TILE DATABASE O", sizeof(header.title) ) != 0 ) {
		ShiError( "Unrecognized file header" );
	}
	if ( strncmp( header.version, "1.1     ", sizeof(header.version) ) != 0 ) {
		ShiError( "Unrecognized file version" );
	}


	// Allocate memory for the tile list
	header.tiles = (TileRecord*)malloc( header.numTiles * sizeof(*header.tiles) );


	// Loop for each tile
	for (tile = 0; tile < header.numTiles; tile++ ) {
		ReadTile( inputFile, &header.tiles[tile] );
	}


	// Close the tile database
	CloseHandle( inputFile );
}


void TileDatabase::Free( void )
{
	int tile, feature;

	for ( tile = 0; tile < header.numTiles; tile++ ) {
		for ( feature = 0; feature < header.tiles[tile].numFeatures; feature++ ) {
			free ( header.tiles[tile].features[feature].points );
			header.tiles[tile].features[feature].points = NULL;
		}
		free ( header.tiles[tile].colors );
		free ( header.tiles[tile].elevations );
		free ( header.tiles[tile].features );
		free ( header.tiles[tile].paths );
		free ( header.tiles[tile].areas );
		header.tiles[tile].colors = NULL;
		header.tiles[tile].elevations = NULL;
		header.tiles[tile].features = NULL;
		header.tiles[tile].areas = NULL;
		header.tiles[tile].paths = NULL;
	}
	free ( header.tiles );
	header.tiles = NULL;

	memset ( &header, 0, sizeof(header) );
}


void TileDatabase::ReadTile( HANDLE inputFile, TileRecord *tile )
{
	DWORD	bytes;
	char	string[8];
	char	message[80];
	char	codeString[8];
	int		feature;
	DWORD	arraySize;


	// Read the tile record header
	ReadFile( inputFile, tile, 17, &bytes, NULL );
	if ( bytes != 17 ) {
		PutErrorString( message );
		strcat( message, ":  Failed the tile header read" );
		ShiError( message );
	}


	// Extract the original tile code from the file name
	strcpy( codeString, "0x" );
	strncpy( &codeString[2], &tile->basename[5], 3 );
	sscanf( codeString, "%hx", &tile->code );


	// Read and verify the color record header
	ReadFile( inputFile, &string, 4, &bytes, NULL );
	if (strncmp( string, "CLR ", 4 ) != 0) {
		PutErrorString( message );
		strcat( message, ":  Failed the color tag read/verify" );
		ShiError( message );
	}

	// Allocate the memory for the color array
	arraySize = header.gridXsize * header.gridYsize * 4;
	tile->colors = (colorRecord*)malloc( arraySize );
	ShiAssert( tile->colors );

	// Read the color array
	ReadFile( inputFile, tile->colors, arraySize, &bytes, NULL );
	if ( bytes != arraySize ) {
		PutErrorString( message );
		strcat( message, ":  Failed the color read" );
		ShiError( message );
	}


	// Read and verify the elevation record header
	ReadFile( inputFile, &string, 4, &bytes, NULL );
	if (strncmp( string, "ELEV", 4 ) != 0) {
		PutErrorString( message );
		strcat( message, ":  Failed the color tag read/verify" );
		ShiError( message );
	}

	// Allocate the memory for the elevation array
	arraySize = header.gridXsize * header.gridYsize * 3;
	tile->elevations = (elevationRecord*)malloc( arraySize );
	ShiAssert( tile->elevations );

	// Read the elevation array
	ReadFile( inputFile, tile->elevations, arraySize, &bytes, NULL );
	if ( bytes != arraySize ) {
		PutErrorString( message );
		strcat( message, ":  Failed the elevation read" );
		ShiError( message );
	}


	// Read and verify the feature record header
	ReadFile( inputFile, &string, 4, &bytes, NULL );
	if (strncmp( string, "FEAT", 4 ) != 0) {
		PutErrorString( message );
		strcat( message, ":  Failed the feature group tag read/verify" );
		ShiError( message );
	}


	// Allocate memory for the features in this tile
	tile->features = (FeatureRecord*)malloc( tile->numFeatures * sizeof( *tile->features ) );
	tile->sortedFeatures = (FeatureRecord**)malloc( tile->numFeatures * sizeof(void*) );
	tile->nareas = 0;
	tile->npaths = 0;

	
	// Read each feature description and count types
	for( feature = 0; feature < tile->numFeatures; feature++ ) {
		ReadFeature( inputFile, &tile->features[feature] );
		tile->sortedFeatures[feature] = &tile->features[feature];

		if ( typeIsPath( tile->features[feature].type ) ) {

			// Make sure each path has at least two defining points
			ShiAssert( tile->features[feature].numPoints >= 2 );
			tile->npaths += tile->features[feature].numPoints - 1;

		} else {

			// Make sure each area has exactly one center point
			ShiAssert( tile->features[feature].numPoints == 1 );
			tile->nareas++;

		}
	}

	
	// Sort the feature descriptions by type
	SortArray( tile->sortedFeatures, tile->numFeatures );


	// Build the path and area records as appropriate for each feature
	tile->areas = (AreaRecord*)malloc( tile->nareas * sizeof( AreaRecord ) );
	tile->paths = (PathRecord*)malloc( tile->npaths * sizeof( PathRecord ) );
	ShiAssert( tile->areas );
	ShiAssert( tile->paths );
	int areaIndex = 0;
	int pathIndex = 0;

	for( feature = 0; feature < tile->numFeatures; feature++ ) {

		if ( typeIsPath( tile->features[feature].type ) ) {
			int pntIndex = 1;

			while ( pntIndex < tile->features[feature].numPoints ) {
				ShiAssert( pathIndex < tile->npaths );
				tile->paths[pathIndex].type = tile->features[feature].type;
				tile->paths[pathIndex].size = tile->features[feature].size;
				tile->paths[pathIndex].x1 = tile->features[feature].points[pntIndex-1].x;
				tile->paths[pathIndex].y1 = tile->features[feature].points[pntIndex-1].y;
				tile->paths[pathIndex].x2 = tile->features[feature].points[pntIndex].x;
				tile->paths[pathIndex].y2 = tile->features[feature].points[pntIndex].y;
				pathIndex++;
				pntIndex++;
			}

		} else {
			ShiAssert( areaIndex < tile->nareas );
			tile->areas[areaIndex].type = tile->features[feature].type;
			tile->areas[areaIndex].size = tile->features[feature].size;
			tile->areas[areaIndex].x = tile->features[feature].points[0].x;
			tile->areas[areaIndex].y = tile->features[feature].points[0].y;
			areaIndex++;
		}
	}
}


void TileDatabase::ReadFeature( HANDLE inputFile, FeatureRecord *feature )
{
	DWORD	bytes;
	char	message[80];


	// Read the feature record header
	ReadFile( inputFile, feature, 13, &bytes, NULL );
	if (bytes != 13) {
		PutErrorString( message );
		strcat( message, ":  Failed feature record read" );
		ShiError( message );
	}

	// Verify the feature tag
	if ( strncmp( feature->tag, "1FTR", 4 ) != 0 ) {
		ShiError( "Failed feature tag check" );
	}


	// Allocate memory for the list of points
	feature->points = (PointRecord*)malloc( feature->numPoints * sizeof( *feature->points ) );
	ShiAssert( feature->points );

	// Read the point records
	ReadFile( inputFile, feature->points, 8*feature->numPoints, &bytes, NULL );
	if (bytes != (DWORD)8*feature->numPoints) {
		PutErrorString( message );
		strcat( message, ":  Failed read of a point record" );
		ShiError( message );
	}
}


void TileDatabase::SortArray( FeatureRecord **array, int numElements )
{
	int				i, j, k;
	FeatureRecord	*p;


	for (i=1; i<numElements; i++) {

		// Decide where to place this element in the list
		j = i;
		while ((j>0) && (array[j-1]->type > array[i]->type)) {
			j--;
		}

		// Only adjust the list if we need to
		if ( j != i ) {

			// Remove the element under consideration (i) from its current location
			p = array[i];
			for (k=i; k<numElements-1; k++) {
				array[k] = array[k+1];
			}

			// Insert the element under consideration in front of the identified element
			for (k=numElements-1; k>j; k--) {
				array[k] = array[k-1];
			}
			array[j] = p;
		}
	}
}


TileRecord* TileDatabase::GetTileRecord( WORD code )
{
	int tile;

	for ( tile = 0; tile < header.numTiles; tile++ ) {

		if (header.tiles[tile].code == code) {
			return &header.tiles[tile];
		}
	}

	// We didn't find a match
	return NULL;
}


AreaRecord* TileDatabase::GetArea( TileRecord *pTile, int area )
{
	if (pTile == NULL) {
		return NULL;
	}

	ShiAssert( area < pTile->nareas );

	return &pTile->areas[area];
}


PathRecord* TileDatabase::GetPath( TileRecord *pTile, int path )
{
	if (pTile == NULL) {
		return NULL;
	}

	ShiAssert( path < pTile->npaths );

	return &pTile->paths[path];
}


BOOL TileDatabase::typeIsPath( BYTE featureType )
{
	return ( (featureType == COVERAGE_RIVER) ||
			 (featureType == COVERAGE_ROAD)  ||
		     (featureType == COVERAGE_RAIL)  ||
		     (featureType == COVERAGE_BRIDGE)||
		     (featureType == COVERAGE_RUNWAY) );
}
