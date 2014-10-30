/***************************************************************************\
    Tblock.cpp
    Scott Randolph
    August 21, 1995

    This class manages all the posts which make up on block of terrain data.
    Each block has pointers to its parent and its four childer.  A block is
 the smallest piece of a map which may be indepently loaded and unloaded.
\***************************************************************************/
#include "Tblock.h"


#ifdef USE_SH_POOLS
MEM_POOL TBlock::pool;
#endif


// This must be called inside of Level's critical section
// Setup a block header.  This will only be called when the requested block
// needs to be loaded from disk.
void TBlock::Setup(TLevel *Level, UINT r, UINT c)
{
    ShiAssert( not IsOwned()); // Shouldn't be setting up an already owned block

    // Initialize the members of the block header structure
    level = Level;
    blockRow = r;
    blockCol = c;
    posts = NULL;
    minZ = 1e6f;
    maxZ = -1e6f;
    fileOffset = 0;

    // NOTE:  The posts pointer will be set when the data arrives
    // in the TLevel::PreProcessBlock() function
}


// This must be called inside of Level's critical section
void TBlock::Cleanup(void)
{
    ShiAssert( not IsOwned()); // Shouldn't be cleaning up a still owned block

    if (posts)
    {
#ifdef USE_SH_POOLS
        // MemFreeFS( posts );
        MemFreePtr(posts);
#else
        delete posts;
#endif
    }

    posts = NULL;
    level = NULL;
}


// This must be called inside of Level's critical section
void TBlock::Reference()
{
    ShiAssert(owners >= 0);
    owners++;
}


// This must be called inside of Level's critical section
int TBlock::Release()
{
    ShiAssert(IsOwned());
    owners--;

    // If this call returns 0, the caller should Cleanup() and then release this block's memory
    return owners;
}
