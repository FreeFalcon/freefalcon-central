/***************************************************************************\
    TblkList.h
    Scott Randolph
    August 21, 1995

    This class manages a range sorted list of terrain blocks at a given
    level of detail.  There may be many of these lists for each level of
    detail in the underlying map (for different viewpoints).
\***************************************************************************/
#ifndef _TBLKLIST_H_
#define _TBLKLIST_H_

#include "Ttypes.h"

#ifdef USE_SH_POOLS
#include "SmartHeap\Include\smrtheap.h"
#endif

class TListEntry {
  public:
	TBlock		*block;
	int			virtualRow;
	int			virtualCol;
	TListEntry	*next;
	TListEntry	*prev;

#ifdef USE_SH_POOLS
  public:
	// Overload new/delete to use a SmartHeap fixed size pool
	void *operator new(size_t size) { ShiAssert( size == sizeof(TListEntry) ); return MemAllocFS(pool);	};
	void operator delete(void *mem) { if (mem) MemFreeFS(mem); };
	static void InitializeStorage()	{ pool = MemPoolInitFS( sizeof(TListEntry), 4, 0 ); };
	static void ReleaseStorage()	{ MemPoolFree( pool ); };
	static MEM_POOL	pool;
#endif
};


class TBlockList {
  public:
	TBlockList()	{};
	~TBlockList()	{};

	void Setup( TLevel *MapLevel, float PrefetchRange );
	void Cleanup( void );

	// Return the square range to the given post location from the center of our data
	int  RangeFromCenter( int levelPostRow, int levelPostCol );

	// Incrementally move the viewer and swap blocks as needed
  	void Update( float x, float y );

	// Return the maximum distance from the current postion in level 
	// post space at which all data is owned by this list or will ever be
	// available
	int  GetAvailablePostRange( void )		{ return availableRange; };
	int  GetMaxPostRange( void )			{ return interestRange; };

	// Return the min and max elevations within the current available range
	// (positive Z is DOWN)
	float	GetMaxZ(void)	{ return maxZ; };
	float	GetMinZ(void)	{ return minZ; };

	// Return a pointer to the requested post in our levels database.
	// The caller of this function must ensure that the post is within
	// the available range.  Also, the post pointer may become invalid
	// after a call to "Update"
	Tpost* GetPost( int levelPostRow, int levelPostCol );

  private:

	// Incrementally move the viewpoint (less than on LOD block in any direction)
	void UpdateBlockList( int vx, int vy );

	// Build the block list as required for a new position (including large moves)
	void RebuildBlockList( void );


	// Add a block to our active list
	void InsertBlock( int row, int col );

	// Release the most distant blocks in the list to make room for new ones
	void ReleaseDistantBlocks( void );

	// Compute the maximum distance from the current postion in global
	// post space at which all data is owned by this list
	void ComputeAvailableRange( void );

  private:
	// Pointer to the map level we're operating with
	TLevel *myLevelPtr;

	// The level post location of the center of attention of this list
	int		ourLevelPostRow;
	int		ourLevelPostCol;
	
	// The block row and column of the center of attention of this list
	int		ourBlockRow;
	int		ourBlockCol;
	
	// Max and min z values within all loaded blocks in the list
	// (positive Z is DOWN)
	float	maxZ;
	float	minZ;
	
	// linked list of all the blocks we own
	TListEntry	*head;
	TListEntry	*tail;

	// Level post space distance to which we are interested in data from this list
	int		interestRange;
	int		availableRange;

	// Block space distance at which to bring blocks in and out
	int		inBlockDistance;
	int		outBlockDistance;


#ifdef USE_SH_POOLS
  public:
	// Overload new/delete to use a SmartHeap fixed size pool
	void *operator new(size_t size) { ShiAssert( size == sizeof(TBlockList) ); return MemAllocFS(pool);	};
	void operator delete(void *mem) { if (mem) MemFreeFS(mem); };
	static void InitializeStorage()	{ pool = MemPoolInitFS( sizeof(TBlockList), 4, 0 ); };
	static void ReleaseStorage()	{ MemPoolFree( pool ); };
	static MEM_POOL	pool;
#endif
};


#endif // _TBLKLIST_H_
