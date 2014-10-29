/***************************************************************************\
    TblkList.cpp
    Scott Randolph
    August 21, 1995

    This class manages a range sorted list of terrain blocks at a given
    level of detail.  There may be many of these lists for each level of
    detail in the underlying map (for different viewpoints).
\***************************************************************************/
#include <limits.h>
#include <math.h>
#include "Tmap.h"
#include "TblkList.h"
#include "Tlevel.h"
#include "Tblock.h"


#ifdef USE_SH_POOLS
MEM_POOL TListEntry::pool;
MEM_POOL TBlockList::pool;
#endif


// Construct an empty range sorted list manager for the specified map level
void TBlockList::Setup(TLevel *MapLevel, float SwapInRange)
{
    // Store our setup values we'll need later
    myLevelPtr = MapLevel;
    head = tail = NULL;


    // Retain a block of samples with a minimum radius of "SwapInRange"
    // This number is how many blocks ahead of the viewer to request blocks
    // (0 means only the block the viewer is over is requested)
    interestRange   = FloatToInt32((float)ceil(SwapInRange / myLevelPtr->FTperPOST()));
    inBlockDistance = FloatToInt32((float)ceil(SwapInRange / myLevelPtr->FTperBLOCK()));

    // Store the distance (in blocks) at which we want to throw out blocks
    // (should be at least inBlockDistance + 1 to avoid thrashing)
    outBlockDistance = inBlockDistance + 1;

    // Initialize our position to something rediculous to force a full update
    ourLevelPostRow = -5000;
    ourLevelPostCol = -5000;
    ourBlockRow = -5000;
    ourBlockCol = -5000;
    availableRange = -1;
}


// Cleanup and shutdown a range sorted list -- Loader thread MUST already be stopped
void TBlockList::Cleanup(void)
{
    // Block until all my outstanding requests have been filled (to avoid delivery to a destroyed object)
    // SCR 4-23-98  I think this is unnecessary and may needlessly slow shutdown.
    // TheLoader.WaitForLoader();

    // Release all the entries in the list and the blocks that go with them
    inBlockDistance = 0;
    outBlockDistance = 0;
    ReleaseDistantBlocks();
}


// Master function to move the center of attention of this block list
// (X North, Y East, Z Down)
void TBlockList::Update(float x, float y)
{
    int col, row;
    int vx, vy;

    // Store the viewer's new position and convert it into block coordinates
    ourLevelPostRow = WORLD_TO_LEVEL_POST(x, myLevelPtr->LOD());
    ourLevelPostCol = WORLD_TO_LEVEL_POST(y, myLevelPtr->LOD());
    row = LEVEL_POST_TO_LEVEL_BLOCK(ourLevelPostRow);
    col = LEVEL_POST_TO_LEVEL_BLOCK(ourLevelPostCol);

    // Compute which direction(s) we're moving
    vx = row - ourBlockRow;
    vy = col - ourBlockCol;

    // Store the new block row and column for next time
    ourBlockRow = row;
    ourBlockCol = col;

    // Skip the block managment stuff if we didn't cross a block boundry
    if (vx or vy)
    {
        // Call the appropriate worker function based on how far we moved
        if ((abs(vx) <= 1) and (abs(vy) <= 1))
        {
            UpdateBlockList(vx, vy);
        }
        else
        {
            RebuildBlockList();
        }
    }

    // Recompute the available range of posts and store it
    ComputeAvailableRange();
}


// Incrementally move the viewpoint (less than on LOD block in any direction)
// (X North, Y East, Z Down)
void TBlockList::UpdateBlockList(int vx, int vy)
{
    int r, c;
    int start, stop, same;


    // Recompute the block ranges and release distant blocks
    ReleaseDistantBlocks();


    // Request the loading of data coming into range ahead of us
    if (vy)
    {
        if (vy > 0) same = ourBlockCol + inBlockDistance;
        else same = ourBlockCol - inBlockDistance;

        for (r = ourBlockRow - inBlockDistance; r <= ourBlockRow + inBlockDistance; r++)
        {
            InsertBlock(r, same);
        }
    }

    if (vx)
    {
        if (vx > 0) same = ourBlockRow + inBlockDistance;
        else same = ourBlockRow - inBlockDistance;

        // Special case for diagonal motion -- to avoid double loading the corner block
        start = ourBlockCol - inBlockDistance;
        stop = ourBlockCol + inBlockDistance;

        if (vy)
        {
            if (vy > 0) stop--;
            else start++;
        }

        for (c = start; c <= stop; c++)
        {
            InsertBlock(same, c);
        }
    }
}


// Build the block list as required for a new position (including large moves)
// (X North, Y East, Z Down)
void TBlockList::RebuildBlockList(void)
{
    int r, c;
    int rStart, rStop;
    int cStart, cStop;
    TListEntry* oldHead;
    TListEntry* discard;


    // Save a copy of the current list, then empty it
    oldHead = head;
    head = tail = NULL;


    // Compute the bounds of the region we need to fetch
    cStart = ourBlockCol - inBlockDistance;
    cStop = ourBlockCol + inBlockDistance;
    rStart = ourBlockRow - inBlockDistance;
    rStop = ourBlockRow + inBlockDistance;


    // Fetch the area around the viewer's starting position
    for (c = cStart; c <= cStop; c++)
    {
        for (r = rStart; r <= rStop; r++)
        {
            InsertBlock(r, c);
        }
    }


    // Now release the blocks in the old list
    while (oldHead)
    {

        ShiAssert(oldHead->block);

        // Remove this entry from the list
        discard = oldHead;
        oldHead = discard->next;

        // Tell the owning level we're done with this block
        myLevelPtr->ReleaseBlock(discard->block);
        discard->block = NULL;

        // Release this list entry
        delete discard;
    }
}


// Return the square range to the given post location from the center of our data
int TBlockList::RangeFromCenter(int levelPostRow, int levelPostCol)
{
    return max(abs(levelPostRow - ourLevelPostRow),
               abs(levelPostCol - ourLevelPostCol));
}


// Return a pointer to the requested post in our levels database.
// The caller of this function must ensure that the post is within
// the available range.  Also, the post pointer should be considered
// invalid after a call to "Update"
Tpost* TBlockList::GetPost(int levelPostRow, int levelPostCol)
{
    ShiAssert(RangeFromCenter(levelPostRow, levelPostCol) <= GetAvailablePostRange());

    int blockRow = LEVEL_POST_TO_LEVEL_BLOCK(levelPostRow);
    int blockCol = LEVEL_POST_TO_LEVEL_BLOCK(levelPostCol);

    TBlock* block = myLevelPtr->GetOwnedBlock(blockRow, blockCol);

    ShiAssert(block->Posts());

    return block->Post(LEVEL_POST_TO_BLOCK_POST(levelPostRow),
                       LEVEL_POST_TO_BLOCK_POST(levelPostCol));
}


// Return the maximum distance (in level posts) from the current postion in level post space
// at which all data is owned by this list (but not greater than requested SwapInRange)
void TBlockList::ComputeAvailableRange(void)
{
    int row, col;
    TBlock *block;
    int Hrange, Vrange;


    // Get the block rows and columns which bound our area of immediate interest
    int startRow = LEVEL_POST_TO_LEVEL_BLOCK(ourLevelPostRow - interestRange);
    int startCol = LEVEL_POST_TO_LEVEL_BLOCK(ourLevelPostCol - interestRange);
    int stopRow  = LEVEL_POST_TO_LEVEL_BLOCK(ourLevelPostRow + interestRange);
    int stopCol  = LEVEL_POST_TO_LEVEL_BLOCK(ourLevelPostCol + interestRange);


    // Start with the default range.  We'll reduce it as necessary
    availableRange = interestRange;
    minZ = 1e6f;
    maxZ = -1e6f;

    // Step through the blocks in the area of interest
    for (row = startRow; row <= stopRow; row++)
    {
        for (col = startCol; col <= stopCol; col++)
        {

            // Get a pointer to the current block we're checking
            block = myLevelPtr->GetOwnedBlock(row, col);
            ShiAssert(block);

            if (block->Posts())
            {

                // This block's data is loaded, so take its min and max elevations into account
                minZ = min(minZ, block->GetMinZ());
                maxZ = max(maxZ, block->GetMaxZ());


            }
            else
            {

                // This block's data isn't loaded, so we have to reduce our range to exclude it
                if (row == ourBlockRow)
                {
                    Vrange = 0;
                }
                else if (row < ourBlockRow)
                {
                    Vrange = LEVEL_POST_TO_BLOCK_POST(ourLevelPostRow) +
                             POSTS_ACROSS_BLOCK * (ourBlockRow - row - 1);
                }
                else
                {
                    Vrange = POSTS_ACROSS_BLOCK - 1 - LEVEL_POST_TO_BLOCK_POST(ourLevelPostRow) +
                             POSTS_ACROSS_BLOCK * (row - ourBlockRow - 1);
                }

                if (col == ourBlockCol)
                {
                    Hrange = 0;
                }
                else if (col < ourBlockCol)
                {
                    Hrange = LEVEL_POST_TO_BLOCK_POST(ourLevelPostCol) +
                             POSTS_ACROSS_BLOCK * (ourBlockCol - col - 1);
                }
                else
                {
                    Hrange = POSTS_ACROSS_BLOCK - 1 - LEVEL_POST_TO_BLOCK_POST(ourLevelPostCol) +
                             POSTS_ACROSS_BLOCK * (col - ourBlockCol - 1);
                }

                availableRange = min(availableRange, max(Hrange, Vrange));
                ShiAssert(availableRange >= 0);

            }
        }
    }


    // Return the value in units of global posts (posts at the highest detail level)
    ShiAssert(availableRange >= 0);
}


// Release blocks going out of range behind us
// THIS CALL SHOULD ONLY BE MADE WHILE PROTECTED BY A CRITICAL SECTION
void TBlockList::ReleaseDistantBlocks()
{
    int dx, dy;
    TListEntry* entry;
    TListEntry* discard;


    // Recompute ranges for all loaded (active) tiles and throw out distant ones
    entry = head;

    while (entry)
    {

        ShiAssert(entry->block);
        dx = abs(entry->virtualRow - ourBlockRow);
        dy = abs(entry->virtualCol - ourBlockCol);

        if ((dx < outBlockDistance) and (dy < outBlockDistance))
        {

            // This ones okay, so move on to the next entry in the list
            entry = entry->next;

        }
        else
        {

            // Remove this entry from the list as it is out of range
            if (entry->prev)
            {
                entry->prev->next = entry->next;
            }
            else
            {
                head = entry->next;
            }

            if (entry->next)
            {
                entry->next->prev = entry->prev;
            }
            else
            {
                tail = entry->prev;
            }

            // Tell the owning level we're done with this block
            myLevelPtr->ReleaseBlock(entry->block);

            // Release this list entry
            discard = entry;
            discard->block = NULL;
            entry = entry->next;
            delete discard;
        }
    }
}


// This call takes care or requesting and storing the blocks needed in our active list
// THIS CALL SHOULD ONLY BE MADE WHILE PROTECTED BY A CRITICAL SECTION
void TBlockList::InsertBlock(int row, int col)
{
    TListEntry* entry;


    // Allocate memory for the new block list entry
    entry = new TListEntry;

    if ( not entry)
    {
        ShiError("Failed to allocate memory for terrain block list entry");
    }


    // Request the block from our level
    entry->virtualRow = row;
    entry->virtualCol = col;
    entry->block = myLevelPtr->RequestBlockOwnership(row, col);

    if ( not entry->block)
    {
        ShiError("I failed to find a terrain block");
    }


    // Add this block to the head of this list
    entry->prev = NULL;
    entry->next = head;

    if (head)
    {
        head->prev = entry;
    }

    head = entry;

    if ( not tail)
    {
        tail = entry;
    }
}
