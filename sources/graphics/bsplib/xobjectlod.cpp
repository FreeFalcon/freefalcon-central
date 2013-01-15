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


#include "RedProfiler.h"
#include "Graphics/DXEngine/DXDefines.h"
#include "Graphics/DXEngine/DXEngine.h"
#include "Graphics/DXEngine/DXVBManager.h"

extern	bool g_bUse_DX_Engine;

extern	DWORD	gDebugLodID;

#ifdef USE_SH_POOLS
extern MEM_POOL gBSPLibMemPool;
#endif

ObjectLOD			*TheObjectLODs = NULL;
int					TheObjectLODsCount = 0;
#ifdef _DEBUG
int		    ObjectLOD::lodsLoaded = 0;
#endif
FileMemMap  ObjectLOD::ObjectLodMap;
BYTE		*ObjectLOD::LodBuffer;
DWORD		ObjectLOD::LodBufferSize;
bool		ObjectLOD::RatedLoad;
short		*ObjectLOD::CacheLoad, *ObjectLOD::CacheRelease, ObjectLOD::LoadIn, ObjectLOD::LoadOut, ObjectLOD::ReleaseIn, ObjectLOD::ReleaseOut;


CRITICAL_SECTION	ObjectLOD::cs_ObjectLOD;
static 	int		maxTagList;
extern bool		g_bUseMappedFiles;


ObjectLOD::ObjectLOD()
{
	root		= NULL;
	refCount	= 0;
	OnRelease = OnOrder = false;
	RatedLoad = true;
	TexBank	  = NULL;

}


ObjectLOD::~ObjectLOD()
{
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

	// Allocate space for load buffer
	LodBuffer = (BYTE*) malloc( DEFAULT_BUFFER_SIZE );
	LodBufferSize = DEFAULT_BUFFER_SIZE;
	RatedLoad = true;

	// Allocte acche with a little safety margin
	CacheLoad = (short*) malloc(sizeof(short)*(numEntries + CACHE_MARGIN));
	CacheRelease = (short*) malloc(sizeof(short)*(numEntries + CACHE_MARGIN));
	LoadIn = LoadOut = ReleaseIn = ReleaseOut = 0;

	DWORD	  Count = TheObjectLODsCount;
	ObjectLOD *Lod = &TheObjectLODs[0];
	while(Count--){
		// next LOD
		Lod++;
	}

	// Init our critical section
	InitializeCriticalSection( &cs_ObjectLOD );
}

DWORD	LODsLoaded;



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
	memset(TheObjectLODs, 0, sizeof(ObjectLOD) * TheObjectLODsCount);

	// Allocate memory for the tag list buffer (read time use only)
	tagListBuffer = new BNodeType[maxTagList+1];
	ShiAssert( tagListBuffer );

	// Read the elements of the header array
	//result = read( file, TheObjectLODs, sizeof(*TheObjectLODs)*TheObjectLODsCount );

	// Open the data file we'll read from at run time as object LODs are required
	strcpy( filename, basename );
	strcat(filename, ".DXL");

	if (!ObjectLodMap.Open(filename, FALSE, !g_bUseMappedFiles)) {
		char message[256];
		sprintf( message, "Failed to open object LOD database %s", filename );
		ShiError( message );
	}

	// RED - Read serialization... finally... Stupid JAM...
	ObjectLOD *Lod = &TheObjectLODs[0];
	DWORD	  Count = TheObjectLODsCount;
	char	  Spare[12];
	while(Count--){
		// Spare unused data - 12 bytes
		read(file, &Spare, 12);
		// Read data serially
		read(file, &Lod->fileoffset, sizeof(Lod ->fileoffset));
		read(file, &Lod->filesize, sizeof(Lod ->filesize));

		// Make a copy of the Texture numbers used by the Model
		DxDbHeader rt;
		//read the model header
		if(ObjectLodMap.ReadDataAt(Lod->fileoffset, &rt, sizeof(DxDbHeader))){
			// if a bad version, skuip
			if((rt.Version & 0xffff) == (~rt.Version >>16)){
				// assign the number of textures to the model
				Lod->NrTextures = rt.dwTexNr;
				if(Lod->NrTextures){
					// allocate space and load the textures table from the model
					Lod->TexBank = (DWORD*)malloc(Lod->NrTextures * sizeof(DWORD));
					if(Lod->TexBank) ObjectLodMap.ReadDataAt(Lod->fileoffset + sizeof(DxDbHeader), Lod->TexBank, Lod->NrTextures * sizeof(DWORD));
				}
			}
		}
		// next LOD
		Lod++;
	}

	// Init our critical section
	InitializeCriticalSection( &cs_ObjectLOD );
	RatedLoad = true;
	// Allocte acche with a little safety margin
	CacheLoad = (short*) malloc(sizeof(short)*(TheObjectLODsCount + CACHE_MARGIN));
	CacheRelease = (short*) malloc(sizeof(short)*(TheObjectLODsCount + CACHE_MARGIN));
	LoadIn = LoadOut = ReleaseIn = ReleaseOut = 0;
	LODsLoaded = 0;
}


void ObjectLOD::CleanupTable( void )
{
	// Make sure all objects are freed
	for (int i=0; i<TheObjectLODsCount; i++) {
		ShiAssert( TheObjectLODs[i].refCount == 0 );
		if(TheObjectLODs[i].root){
			// Mark for release the LOD
			TheObjectLODs[i].OnRelease = true;
			// Put into Release List
			CacheRelease[ReleaseIn++] = i;
			// ring the in pointer
			if(ReleaseIn >= (TheObjectLODsCount + CACHE_MARGIN) ) ReleaseIn = 0;
		}
	}


	// Must wait until loader is done before we delete the object out from under it.
	WaitUpdates();

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

	if( LodBuffer ) free(LodBuffer);
	LodBufferSize = 0;

	if(CacheLoad) free(CacheLoad), CacheLoad = NULL;
	if(CacheRelease) free(CacheRelease), CacheRelease = NULL;
	LoadIn = LoadOut = ReleaseIn = ReleaseOut = 0;

	LODsLoaded = 0;

#ifdef USE_SMART_HEAP
	MemPoolFree( pool );
#endif
}



// RED - Rewritten Object Loader
// no more Mutex as they are causing stutters at load time
BOOL ObjectLOD::Fetch(void)
{
	
	ShiAssert( refCount > 0 );

	// if object is on release do nothing
	// DO NOT RETURN A ROOT THAT MAY BE VANISHING, deleted by loader task
	if(OnRelease || OnOrder) return false;

	// If we already have our data
	if (root) return true;

	// Mark as ordered
	OnOrder = true;
	// Put into Load List
	CacheLoad[LoadIn] = WhoAmI();
	// ring the in pointer, work on TEMP to avoid problems with ReleaseOut comparison in other thread
	short Temp = LoadIn + 1;
	if(Temp >= (TheObjectLODsCount + CACHE_MARGIN) ) Temp = 0;
	LoadIn = Temp;
	// Kick the Loader
	TheLoader.WakeUp();

	// Return a flag indicating whether or not the data is available for drawing
	return false;
}



// RED - Rewritten Object Loader
// no more Mutex as they are causing stutters at load time

void ObjectLOD::Unload( void )
{
    
	// if nothing loaded or already on release, return
	if(!root || OnRelease) return;
	// Setup the Flag about Release
	OnRelease = true;
	// Put into Release List
	CacheRelease[ReleaseIn] = WhoAmI();
	// ring the in pointer, work on TEMP to avoid problems with ReleaseOut comparison in other thread
	short Temp = ReleaseIn + 1;
	if(Temp >= (TheObjectLODsCount + CACHE_MARGIN) ) Temp = 0;
	ReleaseIn = Temp;
	// Kick the Loader
	TheLoader.WakeUp();

}





// Privatly used static members
//int			ObjectLOD::objectFile = -1;
BNodeType*	ObjectLOD::tagListBuffer = NULL;

#ifdef USE_SMART_HEAP
MEM_POOL	ObjectLOD::pool;
#endif


void ObjectLOD::ReleaseLodList(void)
{

	for(int a=0; a<TheObjectLODsCount; a++) TheObjectLODs[a].Unload();
	// wait for all done
	WaitUpdates();

}


DWORD ObjectLOD::Load(void)
{
	DxDbHeader		*Header;
	DWORD			DxID;

////////////// COBRA - DX - The DX Load Procedure //////////////////////////////////////////

	gDebugLodID = WhoAmI();
	// check for buffer size... if smaller make a new Buffer
	if(filesize > LodBufferSize) free(LodBuffer), LodBufferSize = filesize, LodBuffer = (BYTE*)malloc(LodBufferSize);
	
	//Default the Root to null
	root=NULL;

	// if Memory allocated
	if( LodBuffer ){
		// if model is loaded
		if (ObjectLodMap.ReadDataAt(fileoffset, LodBuffer, filesize)){
			// Assign with casting to a pointer the header of data
			Header=(DxDbHeader*) LodBuffer;
			// Get the LOD ID
			DxID=Header->Id;
			// Request the VB Manager to add the model in buffer - if object Allocated
			if(TheVbManager.SetupModel(DxID, LodBuffer, Header->VBClass)){
				// Update the Root from VB Manager
				root=(BRoot*)TheVbManager.GetModelRoot(DxID);
				// update flags and assert as good
				TheVbManager.AssertValidModel(DxID);
				
				LODsLoaded++;
				gDebugLodID = -1;

				// return the file size
				return filesize;
			}
		}
	}

	gDebugLodID = -1;
	return 0;
}


void ObjectLOD::Free( void )
{
   	gDebugLodID = WhoAmI();

	// Release the textures
	//TheDXEngine.UnLoadTextures(((DxDbHeader*)root)->Id);
	// Release the model 
	TheVbManager.ReleaseModel(((DxDbHeader*)root)->Id);
	// clear the root
	root = NULL;
	
	gDebugLodID = -1;

	LODsLoaded--;
}


// RED - Rewritten Object Loader
// no more Mutex as they are causing stutters at load time
bool ObjectLOD::UpdateLods( void )
{

	while( LoadIn != LoadOut || ReleaseIn != ReleaseOut){
		// if something to Load
		if(LoadOut != LoadIn){
			// the amount of data loaded
			DWORD	LoadSize = 0;
//			while( LoadSize < MAX_LOD_LOAD_SIZE && LoadOut != LoadIn){
				ObjectLOD	&Lod = TheObjectLODs[CacheLoad[LoadOut++]];
				if(!Lod.root && Lod.OnOrder) LoadSize += Lod.Load(), Sleep(20);
				// Load is done IN ANY CASE
				Lod.OnOrder = false;
				// ring the done pointer
				if(LoadOut >= (TheObjectLODsCount + CACHE_MARGIN) ) LoadOut = 0;
//			}
			// * MANAGE ONLY 1 LOD PER CALL FRAME *
			if(RatedLoad) return true;
		}

		if(ReleaseIn != ReleaseOut){
			ObjectLOD	&Lod = TheObjectLODs[CacheRelease[ReleaseOut++]];
			if(Lod.root && Lod.OnRelease) Lod.Free();
			// Release is done IN ANY CASE
			Lod.OnRelease = false;
			// ring the done pointer
			if(ReleaseOut >= (TheObjectLODsCount + CACHE_MARGIN) ) ReleaseOut = 0;
			// * MANAGE ONLY 1 LOD PER CALL FRAME *
			if(RatedLoad) return true;
		}
	}

	// if here, nothing done, back is up to date
	return false;

}

void ObjectLOD::WaitUpdates( void )
{	
	// if no data to wait, exit here
	if(LoadIn == LoadOut && ReleaseIn == ReleaseOut) return;
	// Pause the Loader...
	TheLoader.SetPause(true);
	while(!TheLoader.Paused());
	// Not slow loading
	RatedLoad = false;
	// Parse all objects till any opration to do
	while(UpdateLods());
	// Restore rated loading
	RatedLoad = true;
	// Run the Loader again
	TheLoader.SetPause(false);
}
	

void	ObjectLOD::ReferenceTexSet(DWORD TexSetNr, DWORD TexSetMax)
{
	gDebugLodID = WhoAmI();
	// check for textures to reference
	if(NrTextures){
		// calculate the bank size
		DWORD	BankSize = NrTextures / TexSetMax;
		// reference the bank textures
		TheTextureBank.ReferenceTexSet(&TexBank[BankSize * TexSetNr], BankSize);
	}
	gDebugLodID = -1;
}

void	ObjectLOD::ReleaseTexSet(DWORD TexSetNr, DWORD TexSetMax)
{
	gDebugLodID = WhoAmI();


	// check for textures to dereference
	if(NrTextures){
		// calculate the bank size
		DWORD	BankSize = NrTextures / TexSetMax;
		// reference the bank textures
		TheTextureBank.ReleaseTexSet(&TexBank[BankSize * TexSetNr], BankSize);
	}

	gDebugLodID = -1;
}


void	ObjectLOD::Reference(void)
{
	// Reference the object
	refCount++;
}

void	ObjectLOD::Release(void){
	// Dereference the object, and eventually uload
	refCount--; if(refCount==0) Unload();
}
