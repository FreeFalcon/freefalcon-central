/***************************************************************************\
	AlphaPatch.cpp
    Scott Randolph
    April 2, 1998

    Keeps a list of polygon/vertex combinations which have overriden
	alpha values.
\***************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include "shi\ShiError.h"
#include "AlphaPatch.h"


AlphaPatchClass		TheAlphaPatchList;


void AlphaPatchClass::Setup(void)
{
	PatchList = NULL;
}


void AlphaPatchClass::Load( char *filename )
{
	FILE	*file;
	char	line[256];
	char	name[256];
	int		vnum;
	float	alpha;
	float	r;
	float	g;
	float	b;
	int		argCnt;

	ShiAssert( PatchList == NULL );

	// Open the named patch file
	file = fopen( filename, "r" );
	if (!file) {
		printf( "Failed to open alpha patch file %s\n", filename );
		return;
	}

	// Read each line
	while (fgets( line, sizeof(line), file )) {

		argCnt = sscanf( line, "%d %s %f %f %f %f", &vnum, name, &r, &g, &b, &alpha );
		if (argCnt == 6) {
//			printf("  OLD STYLE PATCH LINE:  %0d %s %1.2f %1.2f %1.2f %1.2f ==>", vnum, name, r, g, b, alpha );
			sprintf(name, "%s/v%0d", name, vnum+1);
//			printf("  NEW:  %s %1.2f\n", name, alpha );
			AddPatch( name, alpha, r, g, b );
		} else {
			argCnt = sscanf( line, "%s %f", name, &alpha );
			if (argCnt == 2) {
//				printf("  PATCH:  %s %1.2f\n", name, alpha );
				AddPatch( name, alpha );
			}
		}

	}

	// Close the patch file
	fclose( file );
}


void AlphaPatchClass::Cleanup(void)
{
	AlphaPatchRecord *record;

	while (PatchList) {
		record = PatchList;
		PatchList = PatchList->next;
		delete record;
	}
}


void AlphaPatchClass::AddPatch( char *name, float alpha, float r, float g, float b )
{
	AlphaPatchRecord *record = new AlphaPatchRecord;

	ShiAssert( strlen(name) < sizeof(record->name) );
	strcpy( record->name, name );
	record->OLD		= TRUE;
	record->r		= r;
	record->g		= g;
	record->b		= b;
	record->alpha	= alpha;
	record->next	= PatchList;
	PatchList		= record;
}


void AlphaPatchClass::AddPatch( char *name, float alpha )
{
	AlphaPatchRecord *record = new AlphaPatchRecord;

	ShiAssert( strlen(name) < sizeof(record->name) );
	strcpy( record->name, name );
	record->OLD		= FALSE;
	record->alpha	= alpha;
	record->next	= PatchList;
	PatchList		= record;
}


AlphaPatchRecord* AlphaPatchClass::GetPatch( char *name )
{
	AlphaPatchRecord *record = PatchList;

	while(record) {
		if (stricmp( record->name, name ) == 0) {
			// We found a match!
			break;
		}
		record = record->next;
	}

	return record;
}
