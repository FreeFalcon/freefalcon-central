
/***************************************************************************\
    Tblock.h
    Scott Randolph
    August 21, 1995

    This class manages all the posts which make up on block of terrain data.
    A block is the smallest piece of a map which may be indepently loaded
	and unloaded.
\***************************************************************************/
#ifndef _TBLOCK_H_
#define _TBLOCK_H_

#include "Ttypes.h"
#include "Tpost.h"
#include "TLevel.h"
#include "TMap.h"


class TBlock {
  public:
	TBlock()	{ owners = 0; };
	~TBlock()	{ ShiAssert( owners == 0 ); };

  	TLevel*	Level()	{ return level; };

	float	GetMaxZ(void)	{ return maxZ; };
	float	GetMinZ(void)	{ return minZ; };

	// Return the row and column address of this physical block in the map grid
	UINT	Row()	{ return blockRow; };
	UINT	Col()	{ return blockCol; };

	// Return a pointer to the South West post in this block (array element 0)
	Tpost*	Posts()	{ return posts; };

	// Return a pointer to the specified post (in local row/col coordinates)
	Tpost*	Post( UINT r, UINT c )		{	ShiAssert(r<(UINT)POSTS_ACROSS_BLOCK);
											ShiAssert(c<(UINT)POSTS_ACROSS_BLOCK);
											return posts+(r*POSTS_ACROSS_BLOCK+c);	};
  
  protected:
	// Block coordinates of this block within its level
	UINT	blockRow;
	UINT	blockCol;

	// Offset of this block's data in the disk file
	DWORD	fileOffset;

	// Max and min z values within this block
	float	maxZ;
	float	minZ;

	// The level to which this block belongs
	TLevel	*level;

	// Pointer to the array of posts (NULL when not loaded in memory)
	Tpost	*posts;

	// How many viewers are using this block -- if none, this block should be removed
	int		owners;


	// Intialize and release block headers - only allowed from within a TLevel's critical section.
	void Setup( TLevel *Level, UINT r, UINT c );
	void Cleanup();

	// Set and check ownership of this block (block may belong to multiple viewers)
	// These should only be called from within the level's critical section
	void	Reference( void );							// Request use of this block
	int		Release( void );							// Express disinterest in this block
	BOOL	IsOwned( void )  { return (owners > 0); };	// Are we in use by any entity?


	// Allow the level which owns each block to get at the private functions of this class.
	// Specificly, the reference and release functions.
	friend TLevel;


#ifdef USE_SH_POOLS
  public:
	// Overload new/delete to use a SmartHeap fixed size pool
	void *operator new(size_t size) { ShiAssert( size == sizeof(TBlock) ); return MemAllocFS(pool);	};
	void operator delete(void *mem) { if (mem) MemFreeFS(mem); };
	static void InitializeStorage()	{ pool = MemPoolInitFS( sizeof(TBlock), 4, 0 ); };
	static void ReleaseStorage()	{ MemPoolFree( pool ); };
	static MEM_POOL	pool;
#endif
};

#endif // _TBLOCK_H_
