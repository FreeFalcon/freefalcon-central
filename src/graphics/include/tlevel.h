/***************************************************************************\
    Tlevel.cpp
    Scott Randolph
    August 21, 1995

    Our terrain database is made up of multiple levels of detail.  Each level
    is composed of blocks of posts.  This class manages the collection of blocks
    which make up a level of detail.  It pages off the disk as necessary.
\***************************************************************************/
#ifndef _TLEVEL_H_
#define _TLEVEL_H_

#include "Loader.h"
#include "Ttypes.h"
#include "Falclib/Include/FileMemMap.h"
#include "Tdskpost.h"

typedef union tBlockAddress {
	TBlock*	ptr;
	DWORD	offset;
} tBlockAddress;

class PostFile : public FileMemMap {
public:
    TdiskPost *GetDiskPost(DWORD offset) { 
	return (TdiskPost*)GetData(offset, sizeof(TdiskPost) * POSTS_PER_BLOCK);
    };
    TNewdiskPost *GetNewDiskPost(DWORD offset) {
	return (TNewdiskPost *)GetData(offset, sizeof(TNewdiskPost) * POSTS_PER_BLOCK);
    };
};


class TLevel {
  public:
	TLevel()	{ blocks = NULL; };
	~TLevel()	{};

	void Setup( int level, int width, int height, const char *mapPath );
	void Cleanup( void );


  	BOOL	IsReady( void )		{ return (blocks != NULL); };

	int		LOD( void )		{ return myLevel; }; 

 	float	FTperPOST()		{ return feet_per_post; };
  	float	FTperBLOCK()	{ return feet_per_block; };

	UINT	PostsWide( void ) { return blocks_wide*POSTS_PER_BLOCK; };	// How many posts across
	UINT	PostsHigh( void ) { return blocks_high*POSTS_PER_BLOCK; };	// How many posts high

	UINT	BlocksWide( void ) { return blocks_wide; };	 	// How many blocks across is this level
	UINT	BlocksHigh( void ) { return blocks_high; };		// How many blocks high is this level


	// This function loads and/or increments the reference count of the given block
	TBlock	*RequestBlockOwnership( int r, int c );

	// This function assumes the block is already owned by the caller
	TBlock	*GetOwnedBlock( int r, int c );
	
	// The following function decrements the reference count of the given block and frees it (if 0)
	void	ReleaseBlock( TBlock *block );


  protected:
	UINT	blocks_wide;	 	// How many blocks across is this level
	UINT	blocks_high;		// How many blocks high is this level

	tBlockAddress	*blocks;	// Point to an array of pointers to blocks (NULL means not loaded)

	float	feet_per_post;
 	float	feet_per_block;

	float	lightLevel;			// Light level from 0 to 1

	int		myLevel;			// (0 is highest detail, goes up from there by ones)

	PostFile	postFileMap;	    // mem mapped post file

	CRITICAL_SECTION	cs_blockArray;


	// Handle asychronous block loading
	static void LoaderCallBack( LoaderQ* request );		// Dummy front end
	void		PreProcessBlock( LoaderQ* request );	// Actual worker function

	// Handle time of day and lighting notifications
	static void TimeUpdateCallback( void *self );


	// Map from virutal block addresses (unbounded) to physical ones (one in the map)
	inline void	VirtualToPhysicalBlockAddress( int *r, int *c ) {
		if ((*r>=(int)blocks_high) || (*r<0) || (*c>=(int)blocks_wide) || (*c<0)) {
			*r = 0;
			*c = 0;
		}
	}

	// Set/get the block pointer associated with a physcial block address
	void	SetBlockPtr( UINT r, UINT c, TBlock *block );
	TBlock *GetBlockPtr( UINT r, UINT c );

  public:
	// The following functions should not be compiled into the final game...
	void 	SaveBlock( TBlock *block );
	static void	DebugDisplayInit( void );
	static void	DebugDisplayOutput( void );
	void	DebugDisplayLevel( void );

};

#endif // _TLEVEL_H_
