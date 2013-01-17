/***************************************************************************\
    ObjectLOD.h
    Scott Randolph
    February 9, 1998

    Provides structures and definitions for 3D objects.
\***************************************************************************/
#ifndef _OBJECTLOD_H_
#define _OBJECTLOD_H_

#include "BSPnodes.h"
#include "../../Falclib/Include/FileMemMap.h"

#define	MAX_LOD_LOAD_SIZE	(100 * 1024)	// The Max amount of data Loaded in single call Frame

extern class ObjectLOD	*TheObjectLODs;
extern int				TheObjectLODsCount;

typedef	short			TexSetRefType;

class ObjectLOD {
  public:
	ObjectLOD();
	~ObjectLOD();


	void				Reference(void);
	void				Release(void);
	BOOL				Fetch(void);			// True means ready to draw.  False means still waiting.
	void				Draw(void) const		{ ShiAssert( root ); root->Draw(); };
	void				ReferenceTexSet(DWORD TexSetNr = 0, DWORD TexSetMax = 1);
	void				ReleaseTexSet(DWORD TexSetNr = 0, DWORD TexSetMax = 1);

	static void			ReleaseLodList(void);
	static void			SetupEmptyTable( int numEntries );
	static void			SetupTable( int file, char *basename );
	static void			CleanupTable();
	static bool			UpdateLods( void );
	static void			WaitUpdates( void );
	static void			SetRatedLoad( bool Status ) { RatedLoad = false; }




	static				CRITICAL_SECTION	cs_ObjectLOD;

  protected:
	void				Unload( void );
	DWORD				Load(void);
	void				Free(void);

	bool				OnOrder, OnRelease;
	int					refCount;														// How many instances of this LOD are in use
	short				WhoAmI(void)			{ return static_cast<short>(this - TheObjectLODs); } // Return the Self LOD Id

	static FileMemMap   ObjectLodMap; // JPO - MMFILE
	static BNodeType	*tagListBuffer;
	static BYTE			*LodBuffer;
	static DWORD		LodBufferSize;
	static bool			RatedLoad;
	static	short		*CacheLoad, *CacheRelease, LoadIn, LoadOut, ReleaseIn, ReleaseOut;


  public:
	BRoot				*root;			// NULL until loaded, then pointer to node tree
	UInt32				fileoffset;		// Where in the disk file is this record's tree stored
	UInt32				filesize;		// How big the disk representation of this record's tree
	DWORD				*TexBank;		// The copy of the textures Bank of the Model
	DWORD				NrTextures;		// Nr of textures available in the model

#ifdef USE_SMART_HEAP
  public:
	static MEM_POOL	pool;
#endif
#ifdef _DEBUG
	static int lodsLoaded; // JPO - some stats
#endif

};


// The ObjectLod flags
#define	OBJLOD_ON_ORDER		0x0001
#define	OBJLOD_ON_TEXTURE	0x0002
#define	OBJLOD_ON_RELEASE	0x0004
#define	CACHE_MARGIN		32
#define	DEFAULT_BUFFER_SIZE	( 4 * 1024 * 1024 )		//The default buffer size

#endif // _OBJECTLOD_H_
