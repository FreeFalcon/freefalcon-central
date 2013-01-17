/***************************************************************************\
    ParentBuildList.cpp
    Scott Randolph
    February 16, 1998

    Provides build time services for accumulating and processing
	the list of parent objects.
\***************************************************************************/
#include <io.h>
#include <fcntl.h>
#include <SYS\STAT.H>
#include <MgAPIall.h>
#include "ObjectParent.h"
#include "FLTreader.h"
#include "ColorBuildList.h"
#include "PalBuildList.h"
#include "TexBuildList.h"
#include "LODBuildList.h"
#include "ParentBuildList.h"


BuildTimeParentList	TheParentBuildList;

#define MAX_ACCUMULATE( dst, src ) ((dst) = max( (dst), (src) ))
#define MIN_ACCUMULATE( dst, src ) ((dst) = min( (dst), (src) ))


void BuildTimeParentList::AddItem(int id, char *filename)
{
	BuildTimeParentEntry	*entry;
	BuildTimeParentEntry	*prevEntry;


	// Setup a new entry for our list of parents
	entry = new BuildTimeParentEntry;
	ShiAssert( strlen(filename) < sizeof(entry->filename) );
	strcpy(entry->filename, filename);
	entry->id	= id;
	entry->bflags = 0;
	// Find the right place to insert this entry to retain ID ordering
	prevEntry = tail;
	while (prevEntry && (prevEntry->id > id)) {
		prevEntry = prevEntry->prev;
	}

	// Add the new entry to the list just ahead of "nextEntry"
	entry->prev = prevEntry;
	if (prevEntry) {
		entry->next = prevEntry->next;
		if (prevEntry->next) {
			prevEntry->next->prev = entry;
		} else {
			prevEntry->next = entry;
			tail = entry;
		}
	} else {
		if (head) {
			head->prev = entry;
		} else {
			tail = entry;
		}
		entry->next = head;
		head = entry;
	}
}


BOOL BuildTimeParentList::BuildParentTable()
{
	BuildTimeParentEntry	*entry;
	ObjectParent			*objParent;
	int						maxID = -1;
	int						i;


	// Quit right now if we don't have any parent objects in our list to process
	if (!head) {
		return FALSE;
	}


	// Find the largest object id in use
	entry = head;
	while (entry) {
		maxID = max( maxID, entry->id );
		entry = entry->next;
	}

	// Create the contiguous array of parent objects
	if (TheObjectList != NULL) { // we're appending
	    ShiAssert(TheObjectListLength < maxID);
	    ObjectParent *temp = TheObjectList;
	    TheObjectList = new ObjectParent[maxID + 1];
	    ShiAssert( TheObjectList != NULL );
	    memset( TheObjectList, 0, (maxID+1)*sizeof(*TheObjectList) );
	    memcpy( TheObjectList, temp, TheObjectListLength*sizeof(*TheObjectList) );
	    TheObjectListLength = maxID+1;
	    delete []temp;
	}
	else {
	    ShiAssert( TheObjectList == NULL );
	    TheObjectListLength = maxID+1;
	    TheObjectList = new ObjectParent[TheObjectListLength];
	    ShiAssert( TheObjectList != NULL );
	    memset( TheObjectList, 0, TheObjectListLength*sizeof(*TheObjectList) );
	}

	// Initialize the MultiGen API libraries
	mgInit(0, NULL);
	mgSetMessagesEnabled(MMSG_ERROR, MG_TRUE);
	mgSetMessagesEnabled(MMSG_WARNING, MG_FALSE);
	mgSetMessagesEnabled(MMSG_STATUS, MG_FALSE);

	// Traverse the list of parents we've accumulated
	entry = head;
	while (entry) {
	    if (entry -> bflags == 0) {
		// Read the parent FLT file (this will add to TheLODBuildList for each child encountered)
		printf( "Parent %s\n", entry->filename );
		fflush(NULL);
		if ( !ReadControlFlt(entry) ) {
			printf("ERROR:  Failed to read subordinate FLT file\n");
		}
	    }
		entry = entry->next;
	}

	// Now that we've seen all the parents, read the children
	if ( !TheLODBuildList.BuildLODTable() ) {
		return FALSE;
	}

	// Clean up the MultiGen API libraries
	mgExit();


	// Construct the color and texture and palette banks
	TheColorBuildList.BuildPool();
	TheTextureBuildList.BuildPool();
	ThePaletteBuildList.BuildPool();


	// Now accumulate the statistics for each parent
	entry = head;
	while (entry) {
		objParent = &TheObjectList[entry->id];
//		printf ("Process ID %d\n", entry->id);
		ShiAssert(objParent != NULL);
		objParent->radius			= -1.0f;
		objParent->maxX				= -1e12f;
		objParent->maxY				= -1e12f;
		objParent->maxZ				= -1e12f;
		objParent->minX				= 1e12f;
		objParent->minY				= 1e12f;
		objParent->minZ				= 1e12f;
		objParent->nSwitches		= 0;
		objParent->nDOFs			= 0;
		objParent->nSlots			= 0;
		objParent->nDynamicCoords	= 0;
		objParent->nTextureSets		= 0;

		for (i=objParent->nLODs-1; i>=0; i--) {
			ShiAssert (entry->pBuildLODs[i] != NULL);
			objParent->pLODs[i].objLOD = &TheObjectLODs[ entry->pBuildLODs[i]->index ];

			MAX_ACCUMULATE( objParent->radius,	entry->pBuildLODs[i]->radius );
			MAX_ACCUMULATE( objParent->maxX,	entry->pBuildLODs[i]->maxX );
			MAX_ACCUMULATE( objParent->maxY,	entry->pBuildLODs[i]->maxY );
			MAX_ACCUMULATE( objParent->maxZ,	entry->pBuildLODs[i]->maxZ );
			MIN_ACCUMULATE( objParent->minX,	entry->pBuildLODs[i]->minX );
			MIN_ACCUMULATE( objParent->minY,	entry->pBuildLODs[i]->minY );
			MIN_ACCUMULATE( objParent->minZ,	entry->pBuildLODs[i]->minZ );

			MAX_ACCUMULATE( objParent->nSwitches,		entry->pBuildLODs[i]->nSwitches,);
			MAX_ACCUMULATE( objParent->nDOFs,			entry->pBuildLODs[i]->nDOFs );
			MAX_ACCUMULATE( objParent->nSlots,			entry->pBuildLODs[i]->nSlots );
			MAX_ACCUMULATE( objParent->nDynamicCoords,	entry->pBuildLODs[i]->nDynamicCoords );

			if (entry->pBuildLODs[i]->nTextureSets != objParent->nTextureSets) {
				if (objParent->nTextureSets) {
					char string[256];
					sprintf( string, "ERROR: %s LOD %0d has a different texture set count (this %0d, lower %0d, ).", entry->filename, i, objParent->nTextureSets, entry->pBuildLODs[i]->nTextureSets );
					printf("%s\n", string);
					MIN_ACCUMULATE( objParent->nTextureSets, entry->pBuildLODs[i]->nTextureSets );
				} else {
					objParent->nTextureSets = entry->pBuildLODs[i]->nTextureSets;
				}
			}
		}
		// Now that we're all done checking, make ALL objects have at least 1 texture set to keep later math happy
		if (objParent->nTextureSets == 0) {
			objParent->nTextureSets = 1;
		}

		// Create the postion array and load it from the highest LOD
		if (objParent->nSlots + objParent->nDynamicCoords) {
			objParent->pSlotAndDynamicPositions = new Ppoint[objParent->nSlots + objParent->nDynamicCoords];
			memset( objParent->pSlotAndDynamicPositions, 0, 
					sizeof(*objParent->pSlotAndDynamicPositions)*(objParent->nSlots + objParent->nDynamicCoords) );

			// Get the positions from the highest detail LOD.  All others _assumed_ to be the same.
			for (i=0; i<entry->pBuildLODs[0]->nSlots + entry->pBuildLODs[0]->nDynamicCoords; i++) {
				objParent->pSlotAndDynamicPositions[i] = entry->pBuildLODs[0]->pSlotAndDynamicPositions[i];
			}
		} else {
			objParent->pSlotAndDynamicPositions = NULL;
		}


		// Since the data is loaded, keep things consistent by adding a phantom reference
		if (objParent->nLODs > 0)
		    objParent->Reference();

		entry = entry->next;
	}

	return TRUE;
}


// This function is used to straighten things out during cleanup when we built objects
// from scratch and added phantom references to hold the data in memory without associated
// instances.
void BuildTimeParentList::ReleasePhantomReferences( void ) 
{
	BuildTimeParentEntry	*entry;
	ObjectParent			*objParent;

	entry = head;
	while (entry) {
		objParent = &TheObjectList[entry->id];
		objParent->Release();
		entry = entry->next;
	}
}


void BuildTimeParentList::WriteVersion( int file )
{
	int				result;
	DWORD			fileVersion = FORMAT_VERSION;

	// Write the magic number at the head of the file
	result = write( file, &fileVersion, sizeof(fileVersion) );
	if (result < 0 ) {
		perror( "Writing object format version" );
	}
}


void BuildTimeParentList::WriteParentTable( int file )
{
	int						result;
	BuildTimeParentEntry	*entry;
	ObjectParent			*objParent;
	int						objIdx;
	int						i;
	

	printf( "Writing parent data\n" );

	// Write the length of the parent object array
	result = write( file, &TheObjectListLength, sizeof(TheObjectListLength) );


	// Now write the elements of the parent array
	result = write( file, TheObjectList, sizeof(*TheObjectList)*TheObjectListLength );
	if (result < 0 ) {
		perror( "Writing object list" );
	}

	// Finally, write the reference and slot position arrays for each parent in order
	entry = head;
	for (objIdx=0; objIdx < TheObjectListLength; objIdx++) {

		objParent = &TheObjectList[objIdx];

		// Skip any unused entries
		if (objParent->nLODs == 0) {
			printf("ID %0d is empty.\n", objIdx);
			if (objIdx < startpoint-1)
			    entry = entry -> next;
			continue;
		}

		// Detect any problems -- could avoid this requirement by sorting the entry list...
		if (entry->id != objIdx) {
			printf("IDS.TXT must currently be sorted in ID order! %d != %d\n", entry->id, objIdx);
			ShiError("IDS.TXT must currently be sorted in ID order!");
		}

		// Process the slot and dynamic position array
		if (objParent->nSlots) {
			result = write( file, 
							objParent->pSlotAndDynamicPositions, 
							(objParent->nSlots+objParent->nDynamicCoords)*sizeof(*objParent->pSlotAndDynamicPositions) );
		}

		// Process each LOD reference in this parent object
		for (i=0; i<objParent->nLODs; i++) {
	
			// Copy the LOD reference data
			LODrecord p = objParent->pLODs[i];

			// Replace the pointer with the offset of the LOD in TheObjectLOD array.
			// NOTE:  We're shifting the offset left 1 bit and setting the bottom bit
			// to distinguish it from a legal pointer (which would be 4 byte aligned).
			*((DWORD*)&p.objLOD) = (entry->pBuildLODs[i]->index << 1) | 1;
				
			// Write this reference record
			result = write( file, &p, sizeof(p) );
		}

		// Move to the next build list entry
		entry=entry->next;
	}

	printf("Last ID processed was %0d\n", objIdx-1);
}

void BuildTimeParentList::MergeEntries()
{
   for (int i = 0; i < TheObjectListLength; i++ ) {
	AddExisiting (&TheObjectList[i], i);
    }
}

void BuildTimeParentList::AddExisiting(ObjectParent *op, int id)
{
	BuildTimeParentEntry	*entry;
	BuildTimeParentEntry	*prevEntry;

	// Setup a new entry for our list of parents
	entry = new BuildTimeParentEntry;
	strcpy(entry->filename, "bogus");
	entry->id	= id;
	entry->bflags = 1;
	//ShiAssert(op->nLODs > 0);
	entry->pBuildLODs = new BuildTimeLODEntry*[op->nLODs];
	for (int i = 0; i < op->nLODs; i++)
	    entry->pBuildLODs[i] = 
		TheLODBuildList.AddExisiting (op->pLODs[i].objLOD, op);
	// Find the right place to insert this entry to retain ID ordering
	prevEntry = tail;
	while (prevEntry && (prevEntry->id > id)) {
		prevEntry = prevEntry->prev;
	}

	// Add the new entry to the list just ahead of "nextEntry"
	entry->prev = prevEntry;
	if (prevEntry) {
		entry->next = prevEntry->next;
		if (prevEntry->next) {
			prevEntry->next->prev = entry;
		} else {
			prevEntry->next = entry;
			tail = entry;
		}
	} else {
		if (head) {
			head->prev = entry;
		} else {
			tail = entry;
		}
		entry->next = head;
		head = entry;
	}
}
