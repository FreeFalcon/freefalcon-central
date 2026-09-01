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
#include <cstdarg>

static void TerrLog(const char *fmt, ...); // defined above PreProcessBlock
#include "timemgr.h"
#include "tod.h"
#include "terrtex.h"
#include "fartex.h"
#include "tmap.h"
#include "tlevel.h"
#include "tblock.h"
#include "tpost.h"
#include "tdskpost.h"
#include "falclib/include/isbad.h"

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
    blocks = new tBlockAddress[blocks_wide * blocks_high];

    if (not blocks)
    {
        ShiError("Failed to allocate memory for block pointer array");
    }

    // Open the block offset file for this level
    sprintf(filename, "%s/Theater.o%0d", mapPath, level);

    // #104 (Linux): use the case-insensitive-resolving _open (the raw ::open is case-sensitive and the
    // shipped tree is lowercase "theater.o<n>", so "Theater.o<n>" missed). A missed .o file used to
    // fall through to the early `return` below, leaving the freshly-new'd blocks[] array UNINITIALISED
    // -> GetBlockPtr() returned random heap garbage -> CTD in TBlock::Reference() during streaming.
    offsetFile = _open(filename, O_BINARY bitor O_RDONLY, 0);

    if (offsetFile >= 0)
    {

        // Artscout - 2026 (x64): the .o file stores 32-bit (4-byte) block offsets, but tBlockAddress
        // is a union with a pointer -> 8 bytes on x64. The old bulk read sized by sizeof(TBlock*)
        // read twice the file on x64 and failed. Read the 32-bit offsets through a temp buffer and
        // expand them into the union's .offset (the low bits; .ptr cleared so the high bits are 0).
        unsigned blkCount = blocks_wide * blocks_high;
        DWORD *tmpOff = new DWORD[blkCount];

        bytes = read(offsetFile, tmpOff, (unsigned)(sizeof(DWORD) * blkCount));

        if (bytes not_eq (DWORD)(sizeof(DWORD) * blkCount))
        {
            char message[120];
            sprintf(message, "%s:  Couldn't read block offset data",
                    strerror(errno));
            delete[] tmpOff;
            ShiError(message);
        }

        for (unsigned bi = 0; bi < blkCount; bi++)
        {
            blocks[bi].ptr = NULL; // zero all bytes (8 on x64)
            blocks[bi].offset = tmpOff[bi]; // low 32 bits = on-disk offset
        }

        delete[] tmpOff;

        close(offsetFile);
    }
    else
    {
        // #104 (Linux): the offset file is missing/unreadable. Zero the whole block array so every
        // slot reads back as a NULL pointer (offset 0, low bit clear) rather than uninitialised heap
        // garbage -- GetBlockPtr() then returns NULL and the streamer treats the block as not-loaded,
        // instead of dereferencing a random pointer. Then exit cleanly (this level can't stream posts).
        memset(blocks, 0,
               sizeof(tBlockAddress) * (size_t)blocks_wide *
                   (size_t)blocks_high);
        return;
    }

    // Walk through the offsets and shift them up to clear the low bit.
    for (unsigned i = 0; i < blocks_wide * blocks_high; i++)
    {

        // We're dropping the top bit, so no legal offset can have it set
        ShiAssert(not(blocks[i].offset bitand 0x80000000));
        blocks[i].offset = (blocks[i].offset << 1) bitor 1;
        ShiAssert((blocks[i].offset bitand 0x00000001));
    }


    // Open the post file for this level
    sprintf(filename, "%s/Theater.l%0d", mapPath, level);

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
TBlock *TLevel::GetOwnedBlock(int r, int c)
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
TBlock *TLevel::RequestBlockOwnership(int r, int c)
{
    TBlock *block;

    ShiAssert(IsReady());


    EnterCriticalSection(&cs_blockArray);

    // Clamp the row and column address if required
    VirtualToPhysicalBlockAddress(&r, &c);

    // Look for a pointer to this block in memory -- reference it if found
    block = GetBlockPtr(r, c);

    // #104 (Linux) DIAGNOSTIC: GetBlockPtr has been seen returning a bogus pointer here -> CTD in
    // TBlock::Reference(). Two shapes observed: ASCII garbage (0x35..) and a 32-bit-TRUNCATED pointer
    // (high 32 bits == 0, e.g. 0x47f641ca). Real x64 heap pointers are always > 4GB, so treat any
    // sub-4GB or unreadable result as corrupt: dump the raw slot and treat the block as not-loaded so
    // the async loader re-fetches it.
    {
        unsigned long long pv = (unsigned long long)(uintptr_t)block;
        // Platform-correct plausibility range. On Linux x64 heap/mmap pointers are always > 4GB, so
        // sub-4GB means truncation. ON WINDOWS SUB-4GB HEAP POINTERS ARE PERFECTLY LEGAL -- the old
        // Linux-tuned `pv < 4GB` test rejected LIVE blocks here, and the not-found branch below then
        // built a fresh TBlock over the occupied slot: SetBlockPtr read the slot's POINTER bits as the
        // file OFFSET (e.g. 331067360 into a 426KB Theater.l4) -> every read failed -> flat sea-level
        // blocks ("water instead of terrain"), empty GM map, and wild post pointers. Only reject the
        // null page and non-canonical addresses on Windows.
#ifdef _WIN64
        const unsigned long long loBound = 0x10000ULL; // null page / tiny garbage
#else
        const unsigned long long loBound = 0x100000000ULL; // Linux: heap is always above 4GB
#endif
        if (block and (pv < loBound or pv >= 0x800000000000ULL))
        {
            unsigned long long raw = 0;
            if ((unsigned)c < blocks_wide and (unsigned) r < blocks_high)
                memcpy(&raw, &blocks[(unsigned)r * blocks_wide + (unsigned)c],
                       sizeof(raw));
            static int _bbCount = 0;
            if (_bbCount++ < 40)
                TerrLog("[FF] BADBLOCK lvl=%d r=%d c=%d wide=%u high=%u ptr=%p "
                        "slotRaw=0x%016llx off&1=%llu\n",
                        myLevel, r, c, blocks_wide, blocks_high, (void *)block,
                        raw, raw & 1ULL);
            block = NULL;
        }
    }

    if (block)
    {

        // Reference the block on behalf of our caller
        block->Reference();
    }
    else
    {

        if ((r < 0) or (r >= (int)BlocksHigh()) or (c < 0) or
            (c >= (int)BlocksWide()))
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

            if (not block)
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

            if (not request)
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
            TheLoader.CancelRequest(LoaderCallBack, block, NULL,
                                    block->fileOffset >> 1))
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
void TLevel::LoaderCallBack(LoaderQ *request)
{
    TBlock *block;
    TLevel *myself;

    ShiAssert(request);

    block = (TBlock *)request->parameter;
    ShiAssert(block);

    myself = block->Level();
    ShiAssert(myself);

    myself->PreProcessBlock(request);

    delete request;
}


// This function is called when a new block has been read from disk and needs to be processed
// before being made available through this level.
// Artscout - 2026: loader diagnostics must be VISIBLE on Windows (stderr goes nowhere in a windowed
// build) -- mirror to OutputDebugStringA, same pattern as [TERR-NULLPOST].
static void TerrLog(const char *fmt, ...)
{
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    fprintf(stderr, "%s", buf);
    fflush(stderr);
    OutputDebugStringA(buf);
}

void TLevel::PreProcessBlock(LoaderQ *request)
{
    Tpost *postArray;
    Tpost *memPost;
    float minZ;
    float maxZ;
    TBlock *block = (TBlock *)request->parameter;


    ShiAssert(IsReady());
    ShiAssert(block);
    ShiAssert(not block->posts);

    if (g_LargeTerrainFormat)
    {
        TNewdiskPost *post;

        if (not g_bUseMappedFiles)
        {
            // #104 robustness: a TRANSIENT read failure on the background loader thread must not kill the game --
            // ShiError here calls exit() from the loader thread, which then crashes again in the CRT onexit chain
            // (ANode::Remove). Seen intermittently at first stream-in ("failed twice, then fine"). Retry the read;
            // if it still fails, substitute a ZEROED (flat sea-level) block and log -- one flat terrain tile is a
            // far better failure mode than taking the whole sim down.
            bool _rdOk = false;
            for (int _att = 0; _att < 3 && !_rdOk; ++_att)
            {
                if (_att)
                    Sleep(1);
                _rdOk = postFileMap.ReadDataAt(request->fileoffset,
                                               SharedNewPostIOBuffer,
                                               sizeof(SharedNewPostIOBuffer)) ?
                            true :
                            false;
            }
            if (not _rdOk)
            {
                TerrLog(
                        "[TERRAIN] %s: Bad loader read (%d) after 3 tries -> "
                        "flat block substituted\n",
                        strerror(errno), (int)request->fileoffset);
                memset(SharedNewPostIOBuffer, 0, sizeof(SharedNewPostIOBuffer));
            }

            post = SharedNewPostIOBuffer;
        }
        else
            post = postFileMap.GetNewDiskPost(request->fileoffset);

        if (post == NULL)
        {
            // Same policy for the memory-mapped path: log + flat block, never a loader-thread ShiError/exit.
            TerrLog(
                    "[TERRAIN] Bad loader map for offset (%d) -> flat block "
                    "substituted\n",
                    (int)request->fileoffset);
            memset(SharedNewPostIOBuffer, 0, sizeof(SharedNewPostIOBuffer));
            post = SharedNewPostIOBuffer;
        }

        // Allocate space for the arriving post data and decompress it
#ifdef USE_SH_POOLS
        // postArray = (Tpost *)MemAllocFS( gTPostMemPool );
        postArray = (Tpost *)MemAllocPtr(gTPostMemPool,
                                         sizeof(Tpost) * POSTS_PER_BLOCK, 0);
#else
        postArray = new Tpost[POSTS_PER_BLOCK];
#endif

        if (not postArray)
        {
            ShiError("Failed to allocate memory for an arriving post array");
        }

        LargeDiskblockToMemblock(postArray, post, LOD(), lightLevel, &minZ,
                                 &maxZ);
    }
    else
    {
        TdiskPost *post;

        if (not g_bUseMappedFiles)
        {
            // #104 robustness: retry-then-flat, same policy as the large-format branch above (never ShiError/exit
            // from the background loader thread on a transient read).
            bool _rdOk = false;
            for (int _att = 0; _att < 3 && !_rdOk; ++_att)
            {
                if (_att)
                    Sleep(1);
                _rdOk = postFileMap.ReadDataAt(request->fileoffset,
                                               SharedPostIOBuffer,
                                               sizeof(SharedPostIOBuffer)) ?
                            true :
                            false;
            }
            if (not _rdOk)
            {
                TerrLog(
                        "[TERRAIN] %s: Bad loader read (%d) after 3 tries -> "
                        "flat block substituted\n",
                        strerror(errno), (int)request->fileoffset);
                memset(SharedPostIOBuffer, 0, sizeof(SharedPostIOBuffer));
            }

            post = SharedPostIOBuffer;
        }
        else
            post = postFileMap.GetDiskPost(request->fileoffset);

        if (post == NULL)
        {
            // Same policy for the memory-mapped path: log + flat block, never a loader-thread ShiError/exit.
            TerrLog(
                    "[TERRAIN] Bad loader map for offset (%d) -> flat block "
                    "substituted\n",
                    (int)request->fileoffset);
            memset(SharedPostIOBuffer, 0, sizeof(SharedPostIOBuffer));
            post = SharedPostIOBuffer;
        }

        // Allocate space for the arriving post data and decompress it
#ifdef USE_SH_POOLS
        // postArray = (Tpost *)MemAllocFS( gTPostMemPool );
        postArray = (Tpost *)MemAllocPtr(gTPostMemPool,
                                         sizeof(Tpost) * POSTS_PER_BLOCK, 0);
#else
        postArray = new Tpost[POSTS_PER_BLOCK];
#endif

        if (not postArray)
        {
            ShiError("Failed to allocate memory for an arriving post array");
        }

        DiskblockToMemblock(postArray, post, LOD(), lightLevel, &minZ, &maxZ);
    }

    // If the block is no longer needed, throw it away
    EnterCriticalSection(&cs_blockArray);

    if (not block->IsOwned())
    {

        SetBlockPtr(block->Row(), block->Col(), NULL);
        block->Cleanup();
        delete block;

        // Release any textures this block of posts requested
        if (LOD() <= TheMap.LastNearTexLOD())
        {
            for (memPost = postArray + POSTS_PER_BLOCK - 1;
                 memPost >= postArray; memPost--)
            {
                TheTerrTextures.Release(memPost->texID);
            }
        }
        else
        {
            for (memPost = postArray + POSTS_PER_BLOCK - 1;
                 memPost >= postArray; memPost--)
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
        block->fileOffset = blocks[r * blocks_wide + c].offset;

        // Durring debugging, make sure don't already have a pointer
        ShiAssert(block->fileOffset bitand 0x00000001);
        // Artscout - 2026: if the slot holds a live POINTER (bit0 clear), reading it as a file offset
        // produces a garbage seek (pointer bits as offset -> reads far past EOF -> flat blocks). This
        // is the release-mode net for the class of bug the plausibility guard above caused; log loudly
        // and fall back to offset 0|1 (block 0's data: wrong terrain for one block, never garbage).
        if (not(block->fileOffset bitand 0x00000001))
        {
            TerrLog("[TERRAIN] SetBlockPtr: slot r=%u c=%u holds a live pointer "
                    "(0x%llx) -- refusing to use it as a file offset\n",
                    r, c, (unsigned long long)block->fileOffset);
            block->fileOffset = 1; // offset 0, tagged
        }

        // Replace the file offset with a memory pointer
        blocks[r * blocks_wide + c].ptr = block;
    }
    else
    {
        block = blocks[r * blocks_wide + c].ptr;

        // Durring debugging, make sure don't already have an offset
        ShiAssert(not((DWORD_PTR)block bitand 0x00000001));

        // Artscout - 2026 (x64 fix): tBlockAddress is a UNION { TBlock* ptr; DWORD offset; }. On 32-bit both
        // members were 4 bytes and FULLY overlapped, so writing .offset overwrote the whole pointer. On x64
        // ptr is 8 bytes but offset is 4, so writing only .offset leaves the HIGH 4 bytes of the pointer stale.
        // The `offset = NULL` branch then yields ptr = 0x<staleHigh>00000000 with bit0 == 0, which GetBlockPtr()
        // mistakes for a live block -> returns a bogus 0x1_00000000-style pointer -> CTD in TBlock::Reference
        // during terrain streaming. Zero the full 8-byte ptr FIRST (matching Setup()'s "zero all bytes" init),
        // then stamp the low 32 bits with the on-disk offset.
        blocks[r * blocks_wide + c].ptr = NULL;

        // Put the file offset back into the block pointer array
        if (not F4IsBadReadPtr(block, sizeof(TBlock))) // JB 010408 CTD
            blocks[r * blocks_wide + c].offset = block->fileOffset;
        // else: leave the slot fully NULL (empty); GetBlockPtr returns NULL for it.
    }
}


// If the requested block is loaded, return a pointer to it.  Otherwise,
// return NULL
TBlock *TLevel::GetBlockPtr(UINT r, UINT c)
{
    tBlockAddress block;

    if ((c < blocks_wide) and (r < blocks_high))
    {
        block = blocks[r * blocks_wide + c];

        if (not(block.offset bitand 0x00000001))
        {
            return block.ptr;
        }
    }

    return NULL;
};


// Update the lighting properties based on the time of day
void TLevel::TimeUpdateCallback(void *self)
{
    ((TLevel *)self)->lightLevel = TheTimeOfDay.GetLightLevel();
}
