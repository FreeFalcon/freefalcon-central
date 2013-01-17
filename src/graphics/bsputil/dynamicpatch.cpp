/***************************************************************************\
	DynamicPatch.cpp
    Scott Randolph
    April 8, 1998

    Keeps a list of polygon/vertex combinations which runtime moveable
	(ie: Dyanmic vertices).
\***************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include "shi\ShiError.h"
#include "DynamicPatch.h"


DynamicPatchClass		TheDynamicPatchList;


void DynamicPatchClass::Setup(void)
{
	PatchList = NULL;
}


void DynamicPatchClass::Load( char *filename )
{
	FILE	*file;
	char	line[256];
	char	name[256];
	int		vnum;
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

		argCnt = sscanf( line, "%d %s", &vnum, name );
		if (argCnt == 2) {
//			printf("  OLD STYLE VERTEX SPECIFICIATION:  %0d %s ==>", vnum, name );
			sprintf(name, "%s/v%0d", name, vnum+1);
//			printf("  NEW:  %s\n", name );
			AddPatch( name );
		} else {
			argCnt = sscanf( line, "%s", name );
			if (argCnt == 1) {
//				printf("  PATCH:  %s\n", name );
				AddPatch( name );
			}
		}

	}

	// Close the patch file
	fclose( file );
}


void DynamicPatchClass::Cleanup(void)
{
	DynamicPatchRecord *record;

	while (PatchList) {
		record = PatchList;
		PatchList = PatchList->next;
		delete record;
	}
}


void DynamicPatchClass::AddPatch( char *name )
{
	DynamicPatchRecord *record = new DynamicPatchRecord;

	ShiAssert( strlen(name) < sizeof(record->name) );
	strcpy( record->name, name );
	record->next	= PatchList;
	PatchList		= record;
}


BOOL DynamicPatchClass::IsDynamic( char *name )
{
	DynamicPatchRecord *record = PatchList;

	while(record) {
		if (stricmp( record->name, name ) == 0) {
			// We found a match!
			return TRUE;
		}
		record = record->next;
	}

	return FALSE;
}
