/***************************************************************************\
	DataBase.cpp
	Scott Randolph
	November 18, 1998

	Read in and store missile test data.
\***************************************************************************/
#include <io.h>
#include <fcntl.h>
#include "DataBase.h"


DataBaseClass	TheDataBase;


void DataBaseClass::ReadData( char *filename )
{
	int			file = -1;
	DataPoint	tempData;

	file = open( filename, _O_RDONLY | _O_BINARY, 0000666 );
	if (file < 0) {
		char	message[80];
		sprintf( message, "Failed to open input file %s.", filename );
		ShiError( message );
	}

	TheDataLength = 0;
	while ( read(file, &tempData, sizeof(tempData)) == sizeof(tempData) ) { 
		TheDataLength++;
	}

	if (TheDataLength < 1) {
		char	message[80];
		sprintf( message, "No data in input file %s.", filename );
		ShiError( message );
	}

	TheData = new DataPoint[ TheDataLength ];
	ShiAssert( TheData );

	lseek( file, 0, SEEK_SET );

	read(file, TheData, sizeof(tempData)*TheDataLength);
}



void DataBaseClass::FreeData( void )
{
	delete[] TheData;
	TheData = NULL;
	TheDataLength = 0;
}


void DataBaseClass::Process( void(*fn)( DataPoint *arg ), unsigned startAt, unsigned stopBefore )
{
	int		i;

	// Skip the ones too early
	for (i=0; i<TheDataLength; i++) {
		if (TheData[i].time >= startAt)
			break;
	}

	// Process the needed ones
	for (; i<TheDataLength; i++) {
		if (TheData[i].time < stopBefore)
			// Make the callback
			fn( &TheData[i] );
		else
			// We reached our end time
			break;
	}
}
