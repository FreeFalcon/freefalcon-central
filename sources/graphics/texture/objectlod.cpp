/***************************************************************************\
    ObjectLOD.cpp
    Scott Randolph
    February 9, 1998

    Provides structures and definitions for 3D objects.
\***************************************************************************/
#include "stdafx.h"
#include <io.h>
#include <fcntl.h>
#include "Loader.h"
#include "ObjectLOD.h"

#ifdef USE_SH_POOLS
extern MEM_POOL gBSPLibMemPool;
#endif

ObjectLOD			*TheObjectLODs = NULL;
int					TheObjectLODsCount = 0;
#ifdef _DEBUG
int		    ObjectLOD::lodsLoaded = 0;
#endif
FileMemMap  ObjectLOD::ObjectLodMap;

CRITICAL_SECTION	ObjectLOD::cs_ObjectLOD;
static 	int		maxTagList;
extern bool g_bUseMappedFiles;


ObjectLOD::ObjectLOD()
{
	root		= NULL;
	flags		= 0;
	refCount	= 0;
	onOrder		= 0;
}


ObjectLOD::~ObjectLOD()
{
	ShiAssert( !onOrder );
	ShiAssert( !root );
}


void ObjectLOD::SetupEmptyTable( int numEntries )
{
#ifdef USE_SMART_HEAP
	pool = MemPoolInit( 0 );
#endif

	// Create the contiguous array of (unitialized) LOD records
	ShiAssert( TheObjectLODs == NULL );
	#ifdef USE_SH_POOLS
	TheObjectLODs = (ObjectLOD *)MemAllocPtr(gBSPLibMemPool, sizeof(ObjectLOD)*(numEntries), 0 );
	#else
	TheObjectLODs = new ObjectLOD[numEntries];
	#endif
	TheObjectLODsCount = numEntries;

	// Init our critical section
	InitializeCriticalSection( &cs_ObjectLOD );
}


void ObjectLOD::SetupTable( int file, char *basename )
{
	char	filename[_MAX_PATH];
	int		result;


#ifdef USE_SMART_HEAP
	pool = MemPoolInit( 0 );
#endif


	// Read the number of elements in the longest tag list we saw in the LOD data
	result = read( file, &maxTagList, sizeof(maxTagList) );

	// Read the length of the LOD header array
	result = read( file, &TheObjectLODsCount, sizeof(TheObjectLODsCount) );


	// Allocate memory for the LOD array
	TheObjectLODs = new ObjectLOD[TheObjectLODsCount];
	ShiAssert( TheObjectLODs );

	// Allocate memory for the tag list buffer (read time use only)
	tagListBuffer = new BNodeType[maxTagList+1];
	ShiAssert( tagListBuffer );

	// Read the elements of the header array
	result = read( file, TheObjectLODs, sizeof(*TheObjectLODs)*TheObjectLODsCount );
	if (result < 0 ) {
		char message[256];
		sprintf( message, "Reading object LOD headers:  %s", strerror(errno) );
		ShiError( message );
	}


	// Open the data file we'll read from at run time as object LODs are required
	strcpy( filename, basename );
	strcat( filename, ".LOD" );
	if (!ObjectLodMap.Open(filename, FALSE, !g_bUseMappedFiles)) {
		char message[256];
		sprintf( message, "Failed to open object LOD database %s", filename );
		ShiError( message );
	}

	// Init our critical section
	InitializeCriticalSection( &cs_ObjectLOD );
}


void ObjectLOD::CleanupTable( void )
{
	// Make sure all objects are freed
	for (int i=0; i<TheObjectLODsCount; i++) {
		ShiAssert( TheObjectLODs[i].refCount == 0 );

		// Must wait until loader is done before we delete the object out from under it.
		while ( TheObjectLODs[i].onOrder );
	}

	// Free our array of object LODs
	delete[] TheObjectLODs;
	TheObjectLODs = NULL;
	TheObjectLODsCount = 0;

	// Free our tag list buffer
	delete[] tagListBuffer;
	tagListBuffer = NULL;

	// Close our data file
	ObjectLodMap.Close();
	//close( objectFile );

	// Free our critical section
	DeleteCriticalSection( &cs_ObjectLOD );

#ifdef USE_SMART_HEAP
	MemPoolFree( pool );
#endif
}


BOOL ObjectLOD::Fetch(void)
{
	BOOL	result;

    EnterCriticalSection( &cs_ObjectLOD );

	ShiAssert( refCount > 0 );

	// If we already have our data, we can draw
	if (root) {
		result = TRUE;
	} else {
		if (onOrder == 1) {
			// Normal case, do nothing but wait
		} else if (onOrder == 0) {
			// Not requested yet, so go get it
			RequestLoad();
			onOrder = 1;
		} else {
			// Requested, dropped, and now requested again all before the IO completed
			ShiAssert( onOrder == -1 );
			onOrder = 1;
		}
		result = FALSE;
	}

    LeaveCriticalSection( &cs_ObjectLOD );

	// Return a flag indicating whether or not the data is available for drawing
	return result;
}


// This is called from inside our critical section...
void ObjectLOD::RequestLoad( void )
{
	LoaderQ		*request;

	// Allocate space for the async IO request
	request = new LoaderQ;
	if (!request) {
		ShiError( "Failed to allocate memory for an object read request" );
	}

	// Build the data transfer request to get the required object data
	request->filename	= NULL;
	request->fileoffset	= fileoffset;
	request->callback	= LoaderCallBack;
	request->parameter	= this;

	// Submit the request to the asynchronous loader
	TheLoader.EnqueueRequest( request );
}


void ObjectLOD::LoaderCallBack( LoaderQ* request )
{
	int			size;
	int			tagCount;
	BYTE		*nodeTreeData;
	BRoot		*root;
	BNodeType	*tagList = tagListBuffer;
	ObjectLOD	*objLOD = (ObjectLOD*)request->parameter;
	DWORD offset = objLOD->fileoffset;

	// TODO:  Decompress in here...

	// Seek to the required data on disk
	ShiAssert( request->fileoffset == objLOD->fileoffset );

	if (g_bUseMappedFiles) {
	    BYTE *data = ObjectLodMap.GetData(offset, objLOD->filesize);
	    if (data == NULL) {
		char	message[120];
		sprintf( message, "%s:  Bad object map", strerror( errno ));
		ShiError( message );
	    }
	    tagCount = *(DWORD *)data;;
	    data += sizeof(DWORD);
	    ShiAssert(tagCount <= maxTagList);
	    memcpy(tagListBuffer, data, tagCount*sizeof(*tagListBuffer));
	    data += tagCount*sizeof(*tagListBuffer);

	    size = objLOD->filesize - sizeof(tagCount) - tagCount*sizeof(*tagListBuffer);
#ifdef USE_SMART_HEAP
	    nodeTreeData = (BYTE*)MemAllocPtr(pool, size, 0);
#else
	    nodeTreeData = (BYTE*)malloc( size );
#endif
	    ShiAssert( nodeTreeData );
	    memcpy(nodeTreeData, data, size);
	}
	else {
	    // Read the tag list length
	    if (!ObjectLodMap.ReadDataAt(offset, &tagCount, sizeof(tagCount))) {
		char	message[120];
		sprintf( message, "%s:  Bad object seek", strerror( errno ));
		ShiError( message );
	    }
	    offset += sizeof(tagCount);
	    ShiAssert(tagCount <= maxTagList);
	    // Read the tag list
	    if (!ObjectLodMap.ReadDataAt(offset, tagListBuffer, tagCount*sizeof(*tagListBuffer))) {
		char	message[120];
		sprintf( message, "%s:  Bad taglist read", strerror( errno ));
		ShiError( message );
	    }
	    offset += tagCount*sizeof(*tagListBuffer);
	    // Compute the size of the node tree
	    size = objLOD->filesize - sizeof(tagCount) - tagCount*sizeof(*tagListBuffer);
	    
	    // Allocate memory for the node tree
#ifdef USE_SMART_HEAP
	    nodeTreeData = (BYTE*)MemAllocPtr(pool, size, 0);
#else
	    nodeTreeData = (BYTE*)malloc( size );
#endif
	    ShiAssert( nodeTreeData );
	    
	    // Read the BSP tree node data
	    if (!ObjectLodMap.ReadDataAt(offset, nodeTreeData, size)) {
		char	message[120];
		sprintf( message, "%s:  Bad Lod read", strerror( errno ));
		ShiError( message );
	    }
	}
	// Restore the virtual function tables and pointer connectivity of the node tree
	root = (BRoot*)BNode::RestorePointers( nodeTreeData, 0, &tagList );
	ShiAssert( root->Type() == tagBRoot );
	ShiAssert( (BYTE*)root == nodeTreeData );	// Ensure it will be legal to use "root" to delete the whole buffer later...

	// Load all our textures
	root->LoadTextures();

	// Mark ourselves no longer queued for IO
    EnterCriticalSection( &cs_ObjectLOD );
#ifdef _DEBUG
	objLOD->lodsLoaded ++;
#endif
	if (objLOD->onOrder == 1) {
		objLOD->root = root;
	} else {
		ShiAssert(objLOD->onOrder == -1);	// We must have been Unloaded before IO completed
		root->UnloadTextures();
#ifdef _DEBUG
		objLOD->lodsLoaded --;
#endif
		delete root;
	}
	objLOD->onOrder = 0;
    LeaveCriticalSection( &cs_ObjectLOD );

	// Free the request queue entry
	delete request;
}


void ObjectLOD::Unload( void )
{
	BYTE		*nodeTreeData;
    
	EnterCriticalSection( &cs_ObjectLOD );

	if (!onOrder) {
		if (root) {
			root->UnloadTextures();
			nodeTreeData = (BYTE*)root;
#ifdef USE_SMART_HEAP
			MemFreePtr( nodeTreeData );
#else
			free( nodeTreeData );
#endif
			root = NULL;
#ifdef _DEBUG
			lodsLoaded --;
#endif
		}
	} else {
		if (TheLoader.CancelRequest( LoaderCallBack, this, NULL, fileoffset )) {
			// Normal case, we canceled the request, so we're done.
			onOrder = 0;
		} else {
			// Couldn't cancel the request, so set a flag to drop the object when it finally arrives.
			ShiAssert(onOrder == 1);
			onOrder = -1;
		}
	}

    LeaveCriticalSection( &cs_ObjectLOD );
}



// Privatly used static members
//int			ObjectLOD::objectFile = -1;
BNodeType*	ObjectLOD::tagListBuffer = NULL;

#ifdef USE_SMART_HEAP
MEM_POOL	ObjectLOD::pool;
#endif
