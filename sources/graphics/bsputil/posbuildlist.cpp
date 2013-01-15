/***************************************************************************\
	PosBuildList.cpp
    Scott Randolph
    April 8, 1998

    Provides build time services for sharing positions among all polygons
	in an object.
\***************************************************************************/
#include "PosBuildList.h"


BuildTimePosList::BuildTimePosList()
{
	head			= NULL;
	
	numTotal		= 0;
	numStatic		= 0;
	numDynamic		= 0;

	SizeInfo.radiusSquared	= -1.0f;
	SizeInfo.maxX			= -1e12f;
	SizeInfo.maxY			= -1e12f;
	SizeInfo.maxZ			= -1e12f;
	SizeInfo.minX			= 1e12f;
	SizeInfo.minY			= 1e12f;
	SizeInfo.minZ			= 1e12f;
}


void BuildTimePosList::AddReference( int *target, float x, float y, float z, BuildTimePosType type )
{
	BuildTimePosEntry		*entry;
	BuildTimePosReference	*ref;

	// See if we've already got a matching color to share
	for (entry=head; entry; entry=entry->next) {
		if ((entry->pos.x == x) && 
			(entry->pos.y == y) &&
			(entry->pos.z == z) &&
			(entry->type == type)) {

			// Found a match, so add our reference to it and quit
			for (ref=entry->refs; ref->next; ref=ref->next);
			ref->next = new BuildTimePosReference;
			ref = ref->next;
			ref->target	= target;
			ref->next	= NULL;
			*target		= -1;
			return;
		}
	}

	// Setup a new entry for our list
	entry = new BuildTimePosEntry;
	entry->pos.x		= x;
	entry->pos.y		= y;
	entry->pos.z		= z;
	entry->type			= type;
	entry->index		= -1;
	entry->refs			= new BuildTimePosReference;
	entry->refs->target	= target;
	entry->refs->next	= NULL;
	*target = -1;

	// Add the new entry to the head of the list
	entry->next		= head;
	head = entry;

	// Keep a count of how many static and dynamic positions we've got
	numTotal++;
	switch (type) {
	  case Static:
		numStatic++;
		break;
	  case Dynamic:
		numDynamic++;
		break;
	  default:
		printf("Illegal position type requested\n");
		ShiAssert( !"Illegal position type requested" );
	}

	// Maintain statistics about all our positions
	SizeInfo.radiusSquared	= max( SizeInfo.radiusSquared, x*x+y*y+z*z ); 
	SizeInfo.maxX			= max( SizeInfo.maxX, x );
	SizeInfo.maxY			= max( SizeInfo.maxY, y );
	SizeInfo.maxZ			= max( SizeInfo.maxZ, z );
	SizeInfo.minX			= min( SizeInfo.minX, x );
	SizeInfo.minY			= min( SizeInfo.minY, y );
	SizeInfo.minZ			= min( SizeInfo.minZ, z );
}


void BuildTimePosList::AddReference( int *target, int *source )
{
	BuildTimePosEntry		*entry;
	BuildTimePosReference	*ref;

	// Find the reference to the source position to which we want another reference
	for (entry=head; entry; entry=entry->next) {
		for (ref=entry->refs; ref; ref=ref->next) {
			if (ref->target == source) {
				break;
			}
		}
		if (ref) {
			break;
		}
	}

	if (entry) {
		// Found a match, so add our reference to it and quit
		for (ref=entry->refs; ref->next; ref=ref->next);
		ref->next = new BuildTimePosReference;
		ref = ref->next;
		ref->target	= target;
		ref->next	= NULL;
		*target = -1;
		return;
	} else {
		ShiError( "Asked to copy a position which wasn't found in the list" );
	}
}


Ppoint*	BuildTimePosList::GetPosFromTarget(int *target)
{
	BuildTimePosEntry		*entry;
	BuildTimePosReference	*ref;

	// Find the reference to the target position we want to retrieve
	for (entry=head; entry; entry=entry->next) {
		for (ref=entry->refs; ref; ref=ref->next) {
			if (ref->target == target) {
				break;
			}
		}
		if (ref) {
			break;
		}
	}

	if (entry && ref) {
		// Found a match, so return the value
		return &entry->pos;
	} else {
		ShiError( "Asked to retrieve a position which wasn't found in the list" );
		return NULL;
	}
}

Ppoint* BuildTimePosList::GetPool()
{
	Ppoint					*pool;
	Ppoint					*staticPtr;
	Ppoint					*dynamicPtr;
	BuildTimePosReference	*ref;
	BuildTimePosEntry		*entry = head;


	// Construct the position array and get pointers into it
	pool		= new Ppoint[ numStatic + numDynamic ];
	staticPtr	= pool;
	dynamicPtr	= pool + numStatic;
	ShiAssert( staticPtr );
	ShiAssert( dynamicPtr );

	// Copy the positions into their appropriate slots
	while (entry) {

		switch (entry->type) {
		  case Static:
			entry->index = staticPtr - pool;
			*staticPtr++ = entry->pos;
			break;
		  case Dynamic:
			entry->index = dynamicPtr - pool;
			*dynamicPtr++ = entry->pos;
			break;
		  default:
			printf("Illegal position type encountered in list\n");
			ShiAssert( !"Illegal position type encountered in list" );
		}

		// Resolve the dangling color references we've accumulated
		for(ref = entry->refs; ref; ref=ref->next) {
			*ref->target = entry->index;
		}

		// Move to the next LOD record in the list
		entry = entry->next;
	}

	// return a pointer to the table we built
	return pool; 
}
