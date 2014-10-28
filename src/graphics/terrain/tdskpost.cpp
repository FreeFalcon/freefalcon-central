/***************************************************************************\
    TDskPost
    November 13, 1995

    Most basic element of the terrain map.  Posts contain all the
    information about a point on the ground.  There may in the future be
    multiple versions of posts (ie: textured vs. not textured).  For now,
    only one.

 This is version that is used to store the map in the disk file.
\***************************************************************************/

#include "Tpost.h"
#include "TdskPost.h"
#include "TMap.h"

void DiskblockToMemblock(Tpost *memPost, TdiskPost *diskPost,
                         int LOD, float,
                         float *minZ, float *maxZ)
{
    float minZvalue = 1e6f;
    float maxZvalue = -1e6f;

    ShiAssert(memPost);
    ShiAssert(diskPost);
    ShiAssert(minZ);
    ShiAssert(maxZ);
    ShiAssert(g_LargeTerrainFormat == false);

    // Convert the data from disk format to our current runtime format
    for (int i = 0; i < POSTS_PER_BLOCK; ++i)
    {
        // Scale from integer feet (Z up) to floating point feet (Z down)
        memPost->z = -(float)(diskPost->z);

        memPost->texID = diskPost->texID;

        // Compute the texture coordinates for this post
        // The "& 0x3" and 0.25 terms are because we have 4 posts (0,1,2,3) accross each
        // texture at LOD 0.

        static const float start = 0.00001f;
        static const float stop = 1.0f - start;
        static const float minStep = (stop - start) * 0.25f;

        memPost->u = start + ((i << LOD) bitand 0x3) * minStep;
        memPost->v = stop - (((i >> POST_OFFSET_BITS) << LOD) bitand 0x3) * minStep;
        memPost->d = LOD < TheMap.LastNearTexLOD() ? (1 << LOD) * minStep : stop;

        // Copy the color index
        memPost->colorIndex = diskPost->color;

        // See MapDice for the details of this conversion
        static const double thetaInStart = 0.0;
        static const double thetaInStop = PI * 2.0;
        static const double thetaInRange = thetaInStop - thetaInStart;
        static const double thetaOutScale = 255.99;
        memPost->theta = (float)((diskPost->theta * thetaInRange / thetaOutScale) + thetaInStart);

        static const double phiInStart = 0.0;
        static const double phiInStop = PI / 2.0;
        static const double phiInRange = phiInStop - phiInStart;
        static const double phiOutScale = 63.99;
        memPost->phi = (float)((diskPost->phi * phiInRange / phiOutScale) + phiInStart);

        // Request the texture used by this post (if any)
        if (LOD <= TheMap.LastNearTexLOD())
            TheTerrTextures.Request(memPost->texID);
        else TheFarTextures.Request(memPost->texID);

        // Accumulate the min and max Z values (These could be stored in the block on disk)
        minZvalue = min(minZvalue, memPost->z);
        maxZvalue = max(maxZvalue, memPost->z);

        // Move on to the next post
        ++memPost;
        ++diskPost;
    }

    *minZ = minZvalue;
    *maxZ = maxZvalue;
}

// larger sized stuff. common parts should be extracted, but I don't
// have time now to check it all out.
void LargeDiskblockToMemblock(Tpost *memPost, TNewdiskPost *diskPost, int LOD, float, float *minZ, float *maxZ)
{
    float minZvalue = 1e6f;
    float maxZvalue = -1e6f;

    ShiAssert(memPost);
    ShiAssert(diskPost);
    ShiAssert(minZ);
    ShiAssert(maxZ);
    ShiAssert(g_LargeTerrainFormat == true);

    // Convert the data from disk format to our current runtime format
    for (int i = 0; i < POSTS_PER_BLOCK; ++i)
    {
        // Scale from integer feet (Z up) to floating point feet (Z down)
        memPost->z = -(float)(diskPost->z);

        memPost->texID = (DWORD) diskPost->texID;

        // Compute the texture coordinates for this post
        // The "& 0x3" and 0.25 terms are because we have 4 posts (0,1,2,3) accross each
        // texture at LOD 0.

        static const float start = 0.00001f;
        static const float stop = 1.0f - start;
        static const float minStep = (stop - start) * 0.25f;

        memPost->u = start + ((i << LOD) bitand 0x3) * minStep;
        memPost->v = stop - (((i >> POST_OFFSET_BITS) << LOD) bitand 0x3) * minStep;
        memPost->d = LOD < TheMap.LastNearTexLOD() ? (1 << LOD) * minStep : stop;

        // Copy the color index
        memPost->colorIndex = diskPost->color;

        // See MapDice for the details of this conversion
        static const double thetaInStart = 0.0;
        static const double thetaInStop = PI * 2.0;
        static const double thetaInRange = thetaInStop - thetaInStart;
        static const double thetaOutScale = 255.99;
        memPost->theta = (float)((diskPost->theta * thetaInRange / thetaOutScale) + thetaInStart);

        static const double phiInStart = 0.0;
        static const double phiInStop = PI / 2.0;
        static const double phiInRange = phiInStop - phiInStart;
        static const double phiOutScale = 63.99;
        memPost->phi = (float)((diskPost->phi * phiInRange / phiOutScale) + phiInStart);

        // Request the texture used by this post (if any)
        if (LOD <= TheMap.LastNearTexLOD())
            TheTerrTextures.Request(memPost->texID);
        else TheFarTextures.Request(memPost->texID);

        // Accumulate the min and max Z values (These could be stored in the block on disk)
        minZvalue = min(minZvalue, memPost->z);
        maxZvalue = max(maxZvalue, memPost->z);

        // Move on to the next post
        ++memPost;
        ++diskPost;
    }

    *minZ = minZvalue;
    *maxZ = maxZvalue;
}
