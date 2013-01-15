/***************************************************************************\
    FLTtoParent.cpp
    Scott Randolph
    January 29, 1998

    This handles reading a MultiGen FLT file into our parent
	object structure.  It normally expects to find only LOD
	distances and external references in its targets.
\***************************************************************************/
#include <stdlib.h>
#include <MgAPIall.h>
#include "shi\ShiError.h"
#include "StateStack.h"
#include "ObjectParent.h"
#include "LODBuildList.h"
#include "FLTreader.h"
#include "FLTerror.h"


typedef struct LODrecord2: public LODrecord {
	BuildTimeLODEntry	*buildTimeLOD;
} LODrecord2;

static const int	MAX_LODS_PER_PARENT = 20;
static LODrecord2	LODlist[MAX_LODS_PER_PARENT];
static int			LODlistLen;


void InitializeDefaultParent(char *filename)
{
	LODlistLen	= 1;

	LODlist[0].buildTimeLOD	= TheLODBuildList.AddReference( filename );
	LODlist[0].maxRange		= 600000.0f;
}


BOOL ProcessLOD(mgrec *rec)
{
	char	*fullpath = NULL;
	char	filename[_MAX_FNAME];
	char	ext[_MAX_EXT];
	double	dist = -1.0f;
	DWORD	retval;
	mgrec	*child;
	mgrec	*next;

	// Walk down the chain of nodes to the bottom
	next = mgGetChild( rec );
	do {
		if (mgGetNext(next)) {
			FLTwarning(rec, "LOD record should have one and only one child");
		}
		child = next;
	} while (next = mgGetChild(child));

	// If we got to the bottom, and its an external refernce, use it
	if (child && mgIsCode(child, fltXref)) {

		retval = mgGetAttList( rec, fltLodSwitchIn, &dist, MG_NULL );
		if (retval != MG_TRUE) {
			FLTwarning( rec, "Failed to get LOD switch range" );
		}
		retval = mgGetAttList( child, fltXrefFilename, &fullpath, MG_NULL );
		if (retval != MG_TRUE) {
			FLTwarning( rec, "Failed to get ext ref info" );
		}
		ShiAssert( dist >= 0.0f );
		if (!fullpath) {
			FLTwarning( rec, "Didn't get the ext ref filename");
			return FALSE;
		}
		_splitpath( fullpath, NULL, NULL, filename, ext );
		strcat( filename, ext );
		ShiAssert( strlen(filename) < sizeof(filename) );
		mgFree( fullpath );

		LODlist[LODlistLen].buildTimeLOD	= TheLODBuildList.AddReference( filename );
		LODlist[LODlistLen].maxRange		= (float)dist;
		LODlistLen++;

		return TRUE;
	} else {
		FLTwarning(rec, "LOD record didn't have an external reference as first child.  Skipping.");
		return FALSE;
	}

	return TRUE;
}


BOOL ExtractControlInfo( mgrec *rec )
{
	mgrec	*child;
	BNode	*masterNode = NULL;
	BNode	*node = NULL;
	BOOL	result = FALSE;

	if (!rec)  return FALSE;

	if (mgIsCode(rec, fltLod)) {
		return ProcessLOD( rec );
	}

	child = mgGetChild( rec );
	while(child) {
		result = result | ExtractControlInfo( child );
		child = mgGetNext( child );
	}

	return result;
}


BOOL ReadControlFlt(BuildTimeParentEntry *buildParent)
{
	mgrec	*db;		// top record of database file specified on command line
	int		i;
	ObjectParent *objParent = &TheObjectList[buildParent->id];

	// open the named database file
	if (!(db = mgOpenDb(buildParent->filename))) {
		char msgbuf [1024];
		mgGetLastError(msgbuf, 1024);
		printf("%s\n", msgbuf);
		mgExit();
		return FALSE;
	}

	// Start with no LODs for this parent object
	LODlistLen = 0;

	// Read the LOD switch distances and filenames
	if (!ExtractControlInfo( db )) {
		InitializeDefaultParent(buildParent->filename);
	}

	// Now move the accumulated data into the parent object
	objParent->nLODs = LODlistLen;
	objParent->pLODs = new LODrecord[LODlistLen];
	buildParent->pBuildLODs = new BuildTimeLODEntry*[LODlistLen];
	buildParent->nBuildLODs = LODlistLen;
	for (i=0; i<LODlistLen; i++) {
		objParent->pLODs[i] = LODlist[i];
		buildParent->pBuildLODs[i] = LODlist[i].buildTimeLOD;
	}

	// close the database file
	mgCloseDb(db);

	return TRUE;
}