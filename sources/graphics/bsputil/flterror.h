/***************************************************************************\
	FLTerror.h
    Scott Randolph
    February 20, 1998

    Provides error reporting services while reading FLT files.
\***************************************************************************/
#ifndef _FLTERROR_H_
#define _FLTERROR_H_

#include <MgAPIall.h>
#include "shi\ShiError.h"

inline void FLTwarning( mgrec *rec, char *message ) {
	char *name = mgGetName(rec);
	char *dbname = mgRec2Filename(rec);
#if 1
	printf("%s %s: %s\n", dbname, name, message);
#else
	char	buffer[1024];
	strcpy( buffer, dbname );
	strcat( buffer, " " );
	strcat( buffer, name );
	strcat( buffer, ":  " );
	strcat( buffer, message );
	strcat( buffer, "\n" );
	OutputDebugString( buffer );
	ShiAssert( strlen(buffer) < sizeof(buffer) );
#endif
	mgFree( dbname );
	mgFree( name );
}


#endif //_FLTERROR_H_