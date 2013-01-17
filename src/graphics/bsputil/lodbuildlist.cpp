/***************************************************************************\
    LODBuildList.cpp
    Scott Randolph
    February 16, 1998

    Provides build time services for sharing LODs among parent
	objects.
\***************************************************************************/
#include <io.h>
#include "FLTreader.h"
#include "BSPnodeWriter.h"
#include "LODBuildList.h"


BuildTimeLODList	TheLODBuildList;


BuildTimeLODEntry* BuildTimeLODList::AddReference( char *filename )
{
	BuildTimeLODEntry	*entry;
	int					index;

	// See if we've already got an instance of this LOD
	index = 0;
	for (entry=head; entry; entry=entry->next) {
		if (strcmp(entry->filename, filename) == 0) {
			// We found a match so we're done
			return entry;
		}
		index++;
	}


	// Setup a new LOD entry for our list
	entry = new BuildTimeLODEntry;
	ShiAssert( strlen(filename) < sizeof(entry->filename) );
	strcpy(entry->filename, filename);
	entry->nSwitches				= -1;
	entry->nDOFs					= -1;
	entry->nSlots					= -1;
	entry->nDynamicCoords			= -1;
	entry->nTextureSets				= -1;
	entry->flags					= 0;
	entry->pSlotAndDynamicPositions	= NULL;
	entry->index					= index;
	entry->bflags					= 0;
	entry->next						= NULL;
	entry->prev						= tail;

	// Add the new entry to the list
	if (tail) {
		tail->next = entry;
		tail = entry;
	} else {
		head = tail = entry;
	}

	return entry;
}


BOOL BuildTimeLODList::BuildLODTable()
{
	BuildTimeLODEntry	*entry = head;

	// Quit right now if we don't have any parent objects in our list to process
	if (!head) {
		return FALSE;
	}

	// Create the contiguous array of LOD objects
	ObjectLOD::SetupEmptyTable( tail->index+1 );
	ShiAssert( TheObjectLODs );

	// Traverse our list of required LOD records
	while (entry) {

		// Make sure we didn't get surprised by an out of order index
		ShiAssert( entry->index <= tail->index );
		if (entry -> bflags == 0) {
		// Read this LOD record's FLT file and process it
		printf( "Child %s\n", entry->filename );
		fflush(NULL);
		TheObjectLODs[entry->index].root = ReadGeometryFlt(entry);
		if ( !TheObjectLODs[entry->index].root ) {
			printf("ERROR:  Failed to read subordinate FLT file\n");
		}

		// Copy the flags from the construction record into the real one...
		TheObjectLODs[entry->index].flags = entry->flags;
		}
		// Move to the next LOD record in the list
		entry = entry->next;
	}

	return TRUE;
}


// Write each LOD object to disk and record its position and size in the file
void BuildTimeLODList::WriteLODData( int file )
{
	BuildTimeLODEntry	*entry;
	int					index;

	entry = head;
	for (index=0; index<TheObjectLODsCount; index++) {

		// Make sure we're in sync (though all we use the entry for is displaying the filename)
		ShiAssert( entry->index == index );

		if (entry->bflags == 0) {
		// Record our starting position in the file
		TheObjectLODs[index].fileoffset = lseek( file, 0, SEEK_CUR );

		// Write our data
		printf("Writing %s\n", entry->filename );
		InitNodeStore();
		StoreBNode( TheObjectLODs[index].root );
		WriteNodeStore( file );

		// Record the size of the chunck we wrote
		TheObjectLODs[index].filesize = lseek( file, 0, SEEK_CUR ) - TheObjectLODs[index].fileoffset;
		}
		// Move to the next LOD record in the list
		entry = entry->next;
	}
}


// Write the LOD headers to disk
void BuildTimeLODList::WriteLODHeaders( int file )
{
	int		length;
	int		result;
	int		i;

	printf( "Writing LOD headers\n" );

	// Write the number of elements in the longest tag list we saw in the LOD data
	length = GetMaxTagListLength();
	result = write( file, &length, sizeof(length) );

	// Write the length of the LOD header array
	result = write( file, &TheObjectLODsCount, sizeof(TheObjectLODsCount) );

	// Write the elements of the header array
	for (i=0; i<TheObjectLODsCount; i++) {

		// Make sure we're saving in a clean state
//		ShiAssert( TheObjectLODs[i].root == NULL );

		result = write( file, &TheObjectLODs[i], sizeof(TheObjectLODs[i]) );
	}

	if (result < 0 ) {
		perror( "Writing LOD header list" );
	}
}


BuildTimeLODEntry * BuildTimeLODList::AddExisiting(ObjectLOD *op, ObjectParent *parent)
{
	BuildTimeLODEntry	*entry;
	int			index;

	// Count objects
	index = 0;
	for (entry=head; entry; entry=entry->next) {
		index++;
	}


	// Setup a new LOD entry for our list
	entry = new BuildTimeLODEntry;
	strcpy(entry->filename, "bogus");
	entry->radius = parent->radius;
	entry->minX = parent->minX;
	entry->maxX= parent->maxX;
	entry->minY= parent->minY;
	entry->maxY= parent->maxY;
	entry->minZ = parent->minZ;
	entry->maxZ= parent->maxZ;
	entry->nSwitches				= parent->nSwitches;
	entry->nDOFs					= parent->nDOFs;
	entry->nSlots					= parent->nSlots;
	entry->nDynamicCoords				= parent->nDynamicCoords;
	entry->nTextureSets				= parent->nTextureSets;
	entry->flags					= 0;
	entry->pSlotAndDynamicPositions = new Ppoint[entry->nDynamicCoords + entry->nSlots];
	memcpy (entry->pSlotAndDynamicPositions, parent->pSlotAndDynamicPositions,
		sizeof(Ppoint) * (entry->nDynamicCoords + entry->nSlots));

	entry->index					= index;
	entry->bflags					= 1;
	entry->next					= NULL;
	entry->prev					= tail;

	// Add the new entry to the list
	if (tail) {
		tail->next = entry;
		tail = entry;
	} else {
		head = tail = entry;
	}

	return entry;

}
