// Optimizer bad, debug good
// #pragma optimize( "", off )

/***************************************************************************\
  Tlevel.cpp
  Scott Randolph
  August 21, 1995

  Our terrain database is made up of multiple levels of detail.  Each level
  is composed of blocks of posts.  This class manges the collection of blocks
  which make up a level of detail.  It is responsible for paging the required
  off the disk a necessary.
  \***************************************************************************/
#include <io.h>
#include <fcntl.h>
#include <math.h>
#include "TimeMgr.h"
#include "TOD.h"
#include "TerrTex.h"
#include "FarTex.h"
#include "Tmap.h"
#include "Tlevel.h"
#include "Tblock.h"
#include "Tpost.h"
#include "TdskPost.h"
#include "Falclib/Include/IsBad.h"

//#define DEBUG_TLEVEL
extern bool g_bUseMappedFiles;

#ifdef USE_SH_POOLS
MEM_POOL gTPostMemPool = NULL;
#endif


// This memory block is used to fetch posts in disk format.  We can share it because
// only one block is handled at a time, and we're done with the data from one block
// before the next block is read in by the Loader.
static TdiskPost SharedPostIOBuffer[POSTS_PER_BLOCK];
static TNewdiskPost SharedNewPostIOBuffer[POSTS_PER_BLOCK];


void TLevel::Setup(int level, int width, int height, const char *mapPath)
{
    char filename[MAX_PATH];
    int offsetFile;
    DWORD bytes;

#ifdef USE_SH_POOLS
    // gTPostMemPool = MemPoolInitFS( sizeof(Tpost)*POSTS_PER_BLOCK, 24, 0 );
    gTPostMemPool = MemPoolInit(0);
#endif

    // Setup the properties of this level
    feet_per_post = LEVEL_POST_TO_WORLD(1, level);
    feet_per_block = LEVEL_BLOCK_TO_WORLD(1, level);
    myLevel = level;
    blocks_wide = width;
    blocks_high = height;


    // Create the synchronization object we'll need
    InitializeCriticalSection(&cs_blockArray);


    // Allocate memory for the block pointer array
    blocks = new tBlockAddress[ blocks_wide * blocks_high ];

    if ( not blocks)
    {
        ShiError("Failed to allocate memory for block pointer array");
    }

    // Open the block offset file for this level
    sprintf(filename, "%s\\Theater.o%0d", mapPath, level);

    offsetFile = open(filename, O_BINARY bitor O_RDONLY , 0);

    if (offsetFile >= 0)
    {

        // Read the file offsets into the post pointer array
        bytes = read(offsetFile, blocks, sizeof(TBlock*)*blocks_wide * blocks_high);

        if (bytes not_eq sizeof(TBlock*)*blocks_wide * blocks_high)
        {
            char message[120];
            sprintf(message, "%s:  Couldn't read block offset data", strerror(errno));
            ShiError(message);
        }

        close(offsetFile);

    }
    else
    {

        // We need to exit cleanly... since this path fails anyway
        return;

        // We couldn't open the offsets file, so we'll assume regular spacing
        for (unsigned i = 0; i < blocks_wide * blocks_high; i++)
        {
            blocks[i].offset = i * sizeof(TdiskPost) * POSTS_PER_BLOCK;
        }

    }

    // Walk through the offsets and shift them up to clear the low bit.
    for (unsigned i = 0; i < blocks_wide * blocks_high; i++)
    {

        // We're dropping the top bit, so no legal offset can have it set
        ShiAssert( not (blocks[i].offset bitand 0x80000000));
        blocks[i].offset = (blocks[i].offset << 1) bitor 1;
        ShiAssert((blocks[i].offset bitand 0x00000001));
    }


    // Open the post file for this level
    sprintf(filename, "%s\\Theater.l%0d", mapPath, level);

    if (postFileMap.Open(filename, FALSE, not g_bUseMappedFiles) == false)
        return;

    // Initialize the lighting conditions and register for future time of day updates
    TimeUpdateCallback(this);
    TheTimeManager.RegisterTimeUpdateCB(TimeUpdateCallback, this);
}



void TLevel::Cleanup(void)
{
    ShiAssert(IsReady());

    blocks_wide = 0;
    blocks_high = 0;

    feet_per_post = 0.0f;
    feet_per_block = 0.0f;

    // Wait for all outstanding TLoader requests to complete or be canceled.
    // Note: We only really have to wait for all _our_ requests to complete, but
    // waiting for an empty queue is easier, though _could_ starve us.
    TheLoader.WaitLoader();

    // Stop receiving time updates
    TheTimeManager.ReleaseTimeUpdateCB(TimeUpdateCallback, this);

    // Close the post file for this level
    postFileMap.Close();

    // Release the block pointer array memory
    delete[] blocks;
    blocks = NULL;

    // Release the sychronization objects we've been using
    DeleteCriticalSection(&cs_blockArray);

#ifdef USE_SH_POOLS

    if (gTPostMemPool)
    {
        MemPoolFree(gTPostMemPool);
        gTPostMemPool = NULL;
    }

#endif
}


// This function returns the block requested assuming it is KNOWN to already be owned.
TBlock* TLevel::GetOwnedBlock(int r, int c)
{
    TBlock *block;

    ShiAssert(IsReady());


    // Clamp the row and column address if required
    VirtualToPhysicalBlockAddress(&r, &c);

    // Look for a pointer to this block in memory
    block = GetBlockPtr(r, c);
    ShiAssert(block and block->IsOwned());

    return block;
}


// If the requested block is loaded, reference it and return a pointer.
// If it is not loaded, then request it and return NULL
// If the requested block is off the edge of the map, clamp the row/col
// address back onto the edge of the map (repeat the edge blocks)
TBlock* TLevel::RequestBlockOwnership(int r, int c)
{
    TBlock *block;

    ShiAssert(IsReady());


    EnterCriticalSection(&cs_blockArray);

    // Clamp the row and column address if required
    VirtualToPhysicalBlockAddress(&r, &c);

    // Look for a pointer to this block in memory -- reference it if found
    block = GetBlockPtr(r, c);

    if (block)
    {

        // Reference the block on behalf of our caller
        block->Reference();

    }
    else
    {

        if ((r < 0) or (r >= (int)BlocksHigh()) or (c < 0) or (c >= (int)BlocksWide()))
        {
            {
                // We're off the map, so don't return a block
                LeaveCriticalSection(&cs_blockArray);
                return NULL;
            }

        }
        else
        {

            // Block wasn't found, we we'll create one and ask the async loader to fetch the data.
            block = new TBlock;

            if ( not block)
            {
                ShiError("Failed to allocate memory for a block header");
            }

            block->Setup(this, r, c);

            // Mark this block as owned by the requestor
            block->Reference();

            // Put the block header into the block array to indicate that the data is on order
            SetBlockPtr(r, c, block);

            // Allocate space for the data transfer and the message requesting it
            LoaderQ *request = new LoaderQ;

            if ( not request)
            {
                ShiError("Failed to allocate memory for a block read request");
            }

            // Build the data transfer request to get the post data
            request->filename = NULL;
            request->fileoffset = block->fileOffset >> 1;
            request->callback = LoaderCallBack;
            request->parameter = block;

            // Submit the request to the asynchronous loader
            TheLoader.EnqueueRequest(request);
        }
    }


    LeaveCriticalSection(&cs_blockArray);

    ShiAssert(block);

    return block;
}



// Decrement the reference count of the given block and free it if we're
// the last user (refCount == 0)
void TLevel::ReleaseBlock(TBlock *block)
{
    int i;

    ShiAssert(IsReady());
    ShiAssert(block);

    EnterCriticalSection(&cs_blockArray);

    // Express disinterest in the block.  If no one else owns it, we may be able to free it
    if (block->Release() == 0)
    {

        // We can free the block if TheLoader is either already done, or hasn't started yet.
        if (block->Posts() or
            TheLoader.CancelRequest(LoaderCallBack, block, NULL, block->fileOffset >> 1))
        {

            SetBlockPtr(block->Row(), block->Col(), NULL);

            // If we actually have the block data already, free the associated textures
            if (block->Posts())
            {
                // Release any textures this block of posts requested
                if (LOD() <= TheMap.LastNearTexLOD())
                {
                    for (i = POSTS_PER_BLOCK - 1; i >= 0; i--)
                    {
                        TheTerrTextures.Release((block->Posts() + i)->texID);
                    }
                }
                else
                {
                    for (i = POSTS_PER_BLOCK - 1; i >= 0; i--)
                    {
                        TheFarTextures.Release((block->Posts() + i)->texID);
                    }
                }
            }

            // Cleanup the block header
            block->Cleanup();
            delete block;
        }

    }

    LeaveCriticalSection(&cs_blockArray);
}


// This function is static to allow the TLoader class to call it through a function pointer
// without knowledge of the specific class to which the data is to be delivered.
void TLevel::LoaderCallBack(LoaderQ* request)
{
    TBlock *block;
    TLevel *myself;

    ShiAssert(request);

    block = (TBlock*)request->parameter;
    ShiAssert(block);

    myself = block->Level();
    ShiAssert(myself);

    myself->PreProcessBlock(request);

    delete request;
}


// This function is called when a new block has been read from disk and needs to be processed
// before being made available through this level.
void TLevel::PreProcessBlock(LoaderQ* request)
{
    Tpost *postArray;
    Tpost *memPost;
    float minZ;
    float maxZ;
    TBlock *block = (TBlock*)request->parameter;


    ShiAssert(IsReady());
    ShiAssert(block);
    ShiAssert( not block->posts);

    if (g_LargeTerrainFormat)
    {
        TNewdiskPost *post;

        if ( not g_bUseMappedFiles)
        {
            if ( not postFileMap.ReadDataAt(request->fileoffset, SharedNewPostIOBuffer,
                                        sizeof(SharedNewPostIOBuffer)))
            {
                char message[120];
                sprintf(message, "%s:  Bad loader read (%0d)", strerror(errno), request->fileoffset);
                ShiError(message);
            }

            post = SharedNewPostIOBuffer;
        }
        else
            post = postFileMap.GetNewDiskPost(request->fileoffset);

        if (post == NULL)
        {
            char message[120];
            sprintf(message, "Bad loader map for offset (%0d)", request->fileoffset);
            ShiError(message);

        }

        // Allocate space for the arriving post data and decompress it
#ifdef USE_SH_POOLS
        // postArray = (Tpost *)MemAllocFS( gTPostMemPool );
        postArray = (Tpost *)MemAllocPtr(gTPostMemPool, sizeof(Tpost) * POSTS_PER_BLOCK, 0);
#else
        postArray = new Tpost[ POSTS_PER_BLOCK ];
#endif

        if ( not postArray)
        {
            ShiError("Failed to allocate memory for an arriving post array");
        }

        LargeDiskblockToMemblock(postArray, post, LOD(), lightLevel, &minZ, &maxZ);
    }
    else
    {
        TdiskPost *post;

        if ( not g_bUseMappedFiles)
        {
            if ( not postFileMap.ReadDataAt(request->fileoffset, SharedPostIOBuffer, sizeof(SharedPostIOBuffer)))
            {
                char message[120];
                sprintf(message, "%s:  Bad loader read (%0d)", strerror(errno), request->fileoffset);
                ShiError(message);
            }

            post = SharedPostIOBuffer;
        }
        else
            post = postFileMap.GetDiskPost(request->fileoffset);

        if (post == NULL)
        {
            char message[120];
            sprintf(message, "Bad loader map for offset (%0d)", request->fileoffset);
            ShiError(message);

        }

        // Allocate space for the arriving post data and decompress it
#ifdef USE_SH_POOLS
        // postArray = (Tpost *)MemAllocFS( gTPostMemPool );
        postArray = (Tpost *)MemAllocPtr(gTPostMemPool, sizeof(Tpost) * POSTS_PER_BLOCK, 0);
#else
        postArray = new Tpost[ POSTS_PER_BLOCK ];
#endif

        if ( not postArray)
        {
            ShiError("Failed to allocate memory for an arriving post array");
        }

        DiskblockToMemblock(postArray, post, LOD(), lightLevel, &minZ, &maxZ);
    }

    // If the block is no longer needed, throw it away
    EnterCriticalSection(&cs_blockArray);

    if ( not block->IsOwned())
    {

        SetBlockPtr(block->Row(), block->Col(), NULL);
        block->Cleanup();
        delete block;

        // Release any textures this block of posts requested
        if (LOD() <= TheMap.LastNearTexLOD())
        {
            for (memPost = postArray + POSTS_PER_BLOCK - 1; memPost >= postArray; memPost--)
            {
                TheTerrTextures.Release(memPost->texID);
            }
        }
        else
        {
            for (memPost = postArray + POSTS_PER_BLOCK - 1; memPost >= postArray; memPost--)
            {
                TheFarTextures.Release(memPost->texID);
            }
        }

#ifdef USE_SH_POOLS
        // MemFreeFS( postArray );
        MemFreePtr(postArray);
#else
        delete postArray;
#endif
        postArray = NULL;

    }
    else
    {

        // Otherwise, give it it's post data
        block->posts = postArray;
        block->minZ = minZ;
        block->maxZ = maxZ;

    }

    LeaveCriticalSection(&cs_blockArray);
}


// Set the address of the block data for a specific terrain data block
// If the pointer provided is NULL, remove the pointer from the database
// and replace the file offset.
void TLevel::SetBlockPtr(UINT r, UINT c, TBlock *block)
{
    ShiAssert((c < blocks_wide) and (r < blocks_high));

    if (block)
    {
        ShiAssert(block->IsOwned());

        // Store the offset from which this block was retrieved
        block->fileOffset = blocks[ r * blocks_wide + c ].offset;

        // Durring debugging, make sure don't already have a pointer
        ShiAssert(block->fileOffset bitand 0x00000001);

        // Replace the file offset with a memory pointer
        blocks[ r * blocks_wide + c ].ptr = block;
    }
    else
    {
        block = blocks[ r * blocks_wide + c ].ptr;

        // Durring debugging, make sure don't already have an offset
        ShiAssert( not ((DWORD)block bitand 0x00000001));

        // Put the file offset back into the block pointer array
        if ( not F4IsBadReadPtr(block, sizeof(TBlock))) // JB 010408 CTD
            blocks[ r * blocks_wide + c ].offset = block->fileOffset;
        else
            blocks[ r * blocks_wide + c ].offset = NULL; // JB 010408 hmm... see what happens with this.
    }
}


// If the requested block is loaded, return a pointer to it.  Otherwise,
// return NULL
TBlock * TLevel::GetBlockPtr(UINT r, UINT c)
{
    tBlockAddress block;

    if ((c < blocks_wide) and (r < blocks_high))
    {
        block = blocks[ r * blocks_wide + c ];

        if ( not (block.offset bitand 0x00000001))
        {
            return block.ptr;
        }
    }

    return NULL;
};


// Update the lighting properties based on the time of day
void TLevel::TimeUpdateCallback(void *self)
{
    ((TLevel*)self)->lightLevel = TheTimeOfDay.GetLightLevel();
}
