/***************************************************************************\
    Tviewpnt.cpp
    Scott Randolph
    August 21, 1995

    This class represents a single viewer of the terrain database.  There
    may be multiple viewers simultainiously, each represented by their
    own instance of this class.
\***************************************************************************/
#include <math.h>
#include "TerrTex.h"
#include "Edge.h"
#include "ttypes.h"
#include "TBlkList.h"
#include "TBlock.h"
#include "Tmap.h"
#include "Tviewpnt.h"

extern int g_nLowDetailFactor;
extern float g_fTexDetailFactor;
extern bool g_bDisableHighFartiles;

#pragma warning(disable:4706)

// X North, Y East, Z down
void TViewPoint::Setup(int minimumLOD, int maximumLOD, float *fetchRanges)
{
    ShiAssert(minimumLOD >= 0);
    ShiAssert(maximumLOD >= minimumLOD);

    minLOD  = minimumLOD;
    maxLOD  = maximumLOD;
    nLists  = maxLOD + 1; // Wastes extra array entries if minLOD not_eq 0 (compl 50 bytes per)

    maxRange = new float[(maxLOD + 1) ];

    if ( not maxRange)
    {
        ShiError("Failed memory allocation for viewer's range list");
    }

    memcpy(maxRange, fetchRanges, (maxLOD + 1) * sizeof(float));


    // Allocate memory for the array of block lists
    blockLists = new TBlockList[ nLists ];

    if ( not blockLists)
    {
        ShiError("Failed memory allocation for viewer's block list");
    }


    // Initialize the viewer's position to something rediculous to force a full update
    pos.x = -1e12f;
    pos.y = -1e12f;
    pos.z = -1e12f;
    // Initially enable all detail levels at once.  This will be adjusted by the first
    // call to UpdateViewpoint().
    highDetail = minLOD;
    lowDetail = maxLOD;

    Speed = 0.0; // JB 010610

    // Initialize each block list in turn
    for (int i = minLOD; i <= maxLOD; i++)
    {
        blockLists[i].Setup(TheMap.Level(i), fetchRanges[i]);
    }


    // Setup the critical section used to protect our active block lists
    InitializeCriticalSection(&cs_update);
}


void TViewPoint::Cleanup(void)
{
    // Ensure nobody is using this viewpoint while it is being destroyed
    EnterCriticalSection(&cs_update);

    // Release all the block lists
    ShiAssert(blockLists);

    for (int i = minLOD; i <= maxLOD; i++)
    {
        blockLists[i].Cleanup();
    }

    delete[] blockLists;

    // Release the range array memory
    delete[] maxRange;

    // Wipe out our private variables
    blockLists = NULL;
    maxRange = NULL;
    nLists = 0;

    // Release the critical section used to protect our active block lists
    LeaveCriticalSection(&cs_update);
    DeleteCriticalSection(&cs_update);
}

extern unsigned long vuxGameTime;
#include "SIM/INCLUDE/Phyconst.h"

// Move the viewer and swap blocks as needed (X North, Y East, Z Down)
void TViewPoint::Update(const Tpoint *position)
{
    float altAGL;
    int level;

    // Lock everyone else out of this viewpoint while it is being updated
    EnterCriticalSection(&cs_update);

    // JB 010608 for weather effects start
    static unsigned long prevvuxGameTime;

    if (vuxGameTime not_eq prevvuxGameTime)
    {
        float dist =
            (float)sqrt(((pos.x - position->x) * (pos.x - position->x) + (pos.y - position->y) * (pos.y - position->y)));
        dist = sqrt(dist * dist + (pos.z - position->z) * (pos.z - position->z));
        Speed = dist * FT_TO_NM / ((vuxGameTime - prevvuxGameTime) / (3600000.0F));
        prevvuxGameTime = vuxGameTime;
    }

    // JB 010608 for weather effects end

    // Store the viewer's position
    pos = *position;

    // Ask the list at each level to do any required flushing and/or prefetching of blocks
    for (level = maxLOD; level >= minLOD; level--)
    {
        blockLists[level].Update(X(), Y());
    }

    // TODO:  FIX THIS CRITICAL SECTION STUFF (NO NESTING)
    LeaveCriticalSection(&cs_update);

    // Figure out the viewer's height above the terrain
    // At this point convert from Z down to positive altitude up so
    // that the following conditional tree is less confusing
    altAGL = GetGroundLevelApproximation(X(), Y()) - Z();

    // TODO:  FIX THIS CRITICAL SECTION STUFF (NO NESTING)
    EnterCriticalSection(&cs_update);

    // Adjust the range of active LODs based on altitude
    // (THW: In a perfect world, we would also look at FOV and Screen Res...)
    // THW 2003-11-14 Let's be a bit more generous...this isn't 1998 anymore ;-)
    //THW 2003-11-14 Make it configurable
    if (altAGL < (500.0f * g_fTexDetailFactor))
    {
        highDetail = minLOD;
        lowDetail = maxLOD - g_nLowDetailFactor;
    }
    else if (altAGL < (6000.0f * g_fTexDetailFactor))
    {
        highDetail = minLOD;
        lowDetail = maxLOD;
    }
    else if (altAGL < (24000.0f * g_fTexDetailFactor))
    {
        highDetail = minLOD + 1;
        lowDetail = maxLOD;
    }
    else if (altAGL < (36000.0f * g_fTexDetailFactor))
    {
        highDetail = minLOD + 2;
        lowDetail = maxLOD;
    }
    else
    {
        if (g_bDisableHighFartiles)
        {
            highDetail = minLOD + 2;
        }
        else
        {
            highDetail = minLOD + 3;
        }

        lowDetail = maxLOD;
    }

    // Clamp the values to the avialable range of LODs
    if (lowDetail  < minLOD) 
        lowDetail = minLOD;

    if (lowDetail  > maxLOD) 
        lowDetail = maxLOD;

    if (highDetail > lowDetail) 
        highDetail = lowDetail;

    if (highDetail < minLOD) 
        highDetail = minLOD;

    // Unlock the viewpoint so others can query it
    LeaveCriticalSection(&cs_update);
}


// Return the tex ID of the tile at the provided position.
// The position is in units of texture tiles from lower left.
//  r = WORLD_TO_LEVEL_POST( x, LOD );
// c = WORLD_TO_LEVEL_POST( y, LOD );
int TViewPoint::GetTileID(int r, int c)
{
    Tpost *post;
    const int LOD = TheMap.LastNearTexLOD();
    int texID;

    // Lock everyone else out of this viewpoint while we're using it
    EnterCriticalSection(&cs_update);

    // See if we have the data we'll need
    if (blockLists[LOD].RangeFromCenter(r, c) >= blockLists[LOD].GetAvailablePostRange())
    {
        LeaveCriticalSection(&cs_update);
        return -1;
    }

    // Get the terrain post governing the area of interest
    post = blockLists[LOD].GetPost(r, c);
    ShiAssert(post);
    texID = post->texID;

    // Unlock the viewpoint
    LeaveCriticalSection(&cs_update);

    return texID;
}


// Return the Nth path segment of the required type for the tile type specified.
BOOL TViewPoint::GetPath(int texID, int type, int offset, TpathFeature *target)
{
    TexPath *path;
    const int LOD = TheMap.LastNearTexLOD();

    ShiAssert(texID >= 0);

    // Get the requested path feature in tile space
    path = TheTerrTextures.GetPath(static_cast<TextureID>(texID), type, offset);

    if ( not path)
    {
        return FALSE;
    }

    // If we got a feature, convert it to world space units with 0,0 at lower left of the tile
    target->width = path->width;
    target->x[0] = (path->x1) * TheMap.Level(LOD)->FTperPOST();
    target->x[1] = (path->x2) * TheMap.Level(LOD)->FTperPOST();
    target->y[0] = (path->y1) * TheMap.Level(LOD)->FTperPOST();
    target->y[1] = (path->y2) * TheMap.Level(LOD)->FTperPOST();

    // Return success
    return TRUE;
}


// Return the Nth area feature of the required type for the tile type specified.
BOOL TViewPoint::GetArea(int texID, int type, int offset, TareaFeature *target)
{
    TexArea *area;
    const int LOD = TheMap.LastNearTexLOD();

    ShiAssert(texID >= 0);

    // Get the requested area feature in tile space
    area = TheTerrTextures.GetArea(static_cast<TextureID>(texID), type, offset);

    if ( not area)
    {
        return FALSE;
    }

    // If we got a feature, convert it to world space units with 0,0 at lower left of the tile
    target->radius = area->radius;
    target->x = (area->x) * TheMap.Level(LOD)->FTperPOST();
    target->y = (area->y) * TheMap.Level(LOD)->FTperPOST();

    // Return success
    return TRUE;
}


// Return the highest and lowest terrain elevation within range of this viewpoint
// The values returned are distances upward from zero (ie:  POSITIVE UP)
void TViewPoint::GetAreaFloorAndCeiling(float *floor, float *ceiling)
{
    int level;
    float minZ;
    float maxZ;


    // Lock everyone else out of this viewpoint while we're using it
    EnterCriticalSection(&cs_update);

    // Ask the list at each level for its min/max Z values
    minZ = 1e6f;
    maxZ = -1e6f;

    for (level = maxLOD; level >= minLOD; level--)
    {
        minZ = min(minZ, blockLists[level].GetMinZ());
        maxZ = max(maxZ, blockLists[level].GetMaxZ());
    }

    // Unlock the viewpoint
    LeaveCriticalSection(&cs_update);

    // In case we don't have any terrain loaded yet...
    if (maxZ < minZ)
    {
        maxZ = minZ = 0.0f;
    }

    *floor = maxZ; // Remember, positive Z is downward in world space
    *ceiling = minZ; // Remember, positive Z is downward in world space
}


// Return the type of ground under the specified point on the ground
// (requires terrain data including textures to be loaded at that point,
//  otherwise, 0 is returned.)
int TViewPoint::GetGroundType(float x, float y)
{
    TexArea *area;
    TexPath *path;
    Tpost *post;
    TextureID texID;
    int row, col;
    float xPos, yPos;
    Edge segment;
    float dx, dy, d;
    float r;
    int i;
    int type = -1;
    const int LOD = TheMap.LastNearTexLOD();


    // Lock everyone else out of this viewpoint while we're using it
    EnterCriticalSection(&cs_update);


    // Compute our row and column address and our offset into the tile
    row = WORLD_TO_LEVEL_POST(x, LOD);
    col = WORLD_TO_LEVEL_POST(y, LOD);
    xPos = x - LEVEL_POST_TO_WORLD(row, LOD);
    yPos = y - LEVEL_POST_TO_WORLD(col, LOD);
    ShiAssert((xPos >= -0.5f) and (xPos <= LEVEL_POST_TO_WORLD(1, LOD) + 0.5f));
    ShiAssert((yPos >= -0.5f) and (yPos <= LEVEL_POST_TO_WORLD(1, LOD) + 0.5f));


    // See if we have the data we'll need
    if (blockLists[LOD].RangeFromCenter(row, col) >= blockLists[LOD].GetAvailablePostRange())
    {
        LeaveCriticalSection(&cs_update);
        return 0;
    }


    // Get the terrain post governing the area of interest
    post = blockLists[LOD].GetPost(row, col);
    ShiAssert(post);
    texID = post->texID;


    // Check all segment features for inclusion
    i = 0;
#define GET_NEXT_PATH path = TheTerrTextures.GetPath( texID, 0, i++ )


    GET_NEXT_PATH;

    while ((type == -1) and path)
    {

        r = path->width * 0.5f;

        // Skip this one if we're not inside its bounding box
        if (path->x2 < path->x1)
        {
            if ((path->x2 > xPos + r) or (path->x1 < xPos - r))
            {
                GET_NEXT_PATH;
                continue;
            }
        }
        else
        {
            if ((path->x1 > xPos + r) or (path->x2 < xPos - r))
            {
                GET_NEXT_PATH;
                continue;
            }
        }

        if (path->y2 < path->y1)
        {
            if ((path->y2 > yPos + r) or (path->y1 < yPos - r))
            {
                GET_NEXT_PATH;
                continue;
            }
        }
        else
        {
            if ((path->y1 > yPos + r) or (path->y2 < yPos - r))
            {
                GET_NEXT_PATH;
                continue;
            }
        }

        // Now check the line segment itself
        segment.SetupWithPoints(path->x1, path->y1, path->x2, path->y2);
        segment.Normalize();
        d = (float)fabs(segment.DistanceFrom(xPos, yPos));

        if (d < r)
        {
            type = path->type;
        }

        GET_NEXT_PATH;
    }

    // Check all area features for inclusion
    i = 0;
    area = TheTerrTextures.GetArea(texID, 0, i++);

    while ((type == -1) and area)
    {
        dx = xPos - area->x;
        dy = yPos - area->y;
        d = dx * dx + dy * dy;

        if (d < (area->radius * area->radius))
        {
            type = area->type;
        }

        area = TheTerrTextures.GetArea(texID, 0, i++);
    }

    // Get the basic terrain type of this tile
    if (type == -1)
    {
        type = TheTerrTextures.GetTerrainType(texID);
    }

    // Unlock the viewpoint
    LeaveCriticalSection(&cs_update);

    ShiAssert(type not_eq -1);

    return type;
}


// Return the maximum z value of the terrain near the specified point.  (positive Z down)
float TViewPoint::GetGroundLevelApproximation(float x, float y)
{
    int LOD;
    int row;
    int col;
    Tpost* post;
    float elevation;

    // Compute the level relative post address of interest at the highest available LOD
    LOD = minLOD;
    row = FloatToInt32(x / TheMap.Level(LOD)->FTperPOST());
    col = FloatToInt32(y / TheMap.Level(LOD)->FTperPOST());

    // Lock everyone else out of this viewpoint while we're using it
    EnterCriticalSection(&cs_update);

    // Figure out the highest detail level which has the required data available
    while (blockLists[LOD].RangeFromCenter(row, col) >= blockLists[LOD].GetAvailablePostRange())
    {
        row >>= 1;
        col >>= 1;
        LOD++;

        if (LOD > maxLOD)
        {
            // Unlock the viewpoint
            LeaveCriticalSection(&cs_update);

            return 0.0f;
        }
    }

    // Get the requested value
    post = blockLists[LOD].GetPost(row, col);

    if (post)
    {
        ShiAssert(post);
        elevation = post->z;
    }
    else
    {
        elevation = 0.0F;
    }

    // Unlock the viewpoint
    LeaveCriticalSection(&cs_update);

    return elevation;
}


// Return the z value of the terrain at the specified point.  (positive Z down)
#if USE_GET_LOD_LEVEL
float TViewPoint::GetGroundLevel(float x, float y, Tpoint *normal, int *lod)
#else
float TViewPoint::GetGroundLevel(float x, float y, Tpoint *normal)
#endif
{
    int LOD;
    int row, col;
    float x_pos, y_pos;
    Tpost *p1, *p2, *p3;
    float Nx, Ny, Nz;


    // Compute the level post address of the point of interest
    LOD = minLOD;
    row = WORLD_TO_LEVEL_POST(x, LOD);
    col = WORLD_TO_LEVEL_POST(y, LOD);

    // Lock everyone else out of this viewpoint while we're using it
    EnterCriticalSection(&cs_update);

    // Figure out the highest detail level which has the required data available
    while (blockLists[LOD].RangeFromCenter(row, col) >= blockLists[LOD].GetAvailablePostRange())
    {
        row >>= 1;
        col >>= 1;
        LOD++;

        // See if we've run out of luck
        if (LOD > maxLOD)
        {

            // Unlock the viewpoint
            LeaveCriticalSection(&cs_update);

            // Return some default data
#if USE_GET_LOD_LEVEL

            if (lod)
            {
                *lod = LOD;
            }

#endif

            if (normal)
            {
                normal->x = 0.0f;
                normal->y = 0.0f;
                normal->z = 1.0f;
            }

            return 0.0f;
        }
    }

    // Compute the location of interest relative to the lower left bounding post
    x_pos = x - LEVEL_POST_TO_WORLD(row, LOD);
    y_pos = y - LEVEL_POST_TO_WORLD(col, LOD);

#if 0
    // We occasionally run into precision problems here which cause the asserts
    // to fail.  The basic algorithm seems to be sound, however, and this is a
    // non-fatal condition, so these have been disabled.
    ShiAssert(x_pos >= -0.05f);
    ShiAssert(x_pos <= TheMap.Level(LOD)->FTperPOST() + 0.05f);
    ShiAssert(y_pos >= -0.05f);
    ShiAssert(y_pos <= TheMap.Level(LOD)->FTperPOST() + 0.05f);
#endif


    // Compute the normal from the three posts which bound the point of interest
    p1 = blockLists[LOD].GetPost(row,   col);
    p3 = blockLists[LOD].GetPost(row + 1, col + 1);
    ShiAssert(p1);
    ShiAssert(p3);
    Nz = -TheMap.Level(LOD)->FTperPOST(); // (remember positive Z is down)

    if (x_pos >= y_pos
       and p1 and p3) // JB 011019 CTD fix
    {
        // upper left triangle
        p2 = blockLists[LOD].GetPost(row + 1, col);
        ShiAssert(p2);

        if (p2) // JB 011019 CTD fix
        {
            Nx = p2->z - p1->z; // (remember positive Z is down)
            Ny = p3->z - p2->z; // (remember positive Z is down)
        }
    }
    else if (p1 and p3)  // JB 011019 CTD fix
    {
        // lower right triangle
        p2 = blockLists[LOD].GetPost(row, col + 1);
        ShiAssert(p2);

        if (p2) // JB 011019 CTD fix
        {
            Nx = p3->z - p2->z; // (remember positive Z is down)
            Ny = p2->z - p1->z; // (remember positive Z is down)
        }
    }


    // Unlock the viewpoint
    LeaveCriticalSection(&cs_update);

#if USE_GET_LOD_LEVEL

    if (lod)
    {
        *lod = LOD;
    }

#endif

    // If the caller provided a place to store the normal, do it
    // NOTE: This vector is NOT a unit vector
    if (normal)
    {
        normal->x = Nx;
        normal->y = Ny;
        normal->z = Nz;
    }

    // Compute the z of the plane at the given x,y location using the
    // fact that the dot product between the normal and any vector in
    // the plane will be zero.
    // We choose (0,0,p1->z) and (xpos, ypos, Z) as two points to define
    // a line in the plane and dot that with the normal and solve for Z.
    if (p1) // JB 011019 CTD fix
        return p1->z - Nx / Nz * x_pos - Ny / Nz * y_pos;

    return 0; // JB 011019 CTD fix
}

int TViewPoint::GetLODLevel(float x, float y) const
{
    int lod;

    // Compute the level relative post address of interest at the highest available LOD
    lod = minLOD;
    int row = FloatToInt32(x / TheMap.Level(lod)->FTperPOST());
    int col = FloatToInt32(y / TheMap.Level(lod)->FTperPOST());

    // Lock everyone else out of this viewpoint while we're using it
    EnterCriticalSection(&cs_update);

    // Figure out the highest detail level which has the required data available
    while (
        lod <= maxLOD and 
        blockLists[lod].RangeFromCenter(row, col) >= blockLists[lod].GetAvailablePostRange()
    )
    {
        row >>= 1;
        col >>= 1;
        ++lod;
    }

    LeaveCriticalSection(&cs_update);
    return lod;
}

// Return TRUE if the specified position in 3-space is under the terrain
BOOL TViewPoint::UnderGround(Tpoint *position)
{
    return (position->z >= GetGroundLevel(position->x, position->y));
}



//
// Helper functions for the LOS check below
// These return TRUE if the given Z value is below (less negative than)
// the relevant post or edge.
//
BOOL TViewPoint::TestVertex(int row, int col, float z, int LOD)
{
    // Get the relevant post
    Tpost *post = blockLists[LOD].GetPost(row, col);

    // Return true if the line crosses below the post (ie: is less negative)
    return (post->z < z);
}
BOOL TViewPoint::TestEast(int row, int col, float z, int LOD)
{
    // Get the relevant posts
    Tpost *left  = blockLists[LOD].GetPost(row, col);
    Tpost *right = blockLists[LOD].GetPost(row, col + 1);

    // Return true if the line crosses below the highest post (ie: is less negative)
    return (min(left->z, right->z) < z);
}
BOOL TViewPoint::TestNorth(int row, int col, float z, int LOD)
{
    // Get the relevant posts
    Tpost *bottom = blockLists[LOD].GetPost(row,   col);
    Tpost *top    = blockLists[LOD].GetPost(row + 1, col);

    // Return true if the line crosses below the highest post (ie: is less negative)
    return (min(bottom->z, top->z) < z);
}
BOOL TViewPoint::TestWest(int row, int col, float z, int LOD)
{
    return TestEast(row, col - 1, z, LOD);
}
BOOL TViewPoint::TestSouth(int row, int col, float z, int LOD)
{
    return TestNorth(row - 1, col, z, LOD);
}


// Return TRUE if the two specified points can see each other over the terrain
BOOL TViewPoint::LineOfSight(Tpoint *p1, Tpoint *p2)
{
    int Px = 0, Py = 0; // row and column to lower left of p1
    int Qx = 0, Qy = 0; // row and column to lower left of p2
    int LOD_P = 0; // Most detailed LOD which contains point P
    int LOD_Q = 0; // Most detailed LOD which contains point Q
    float z = 0.0F, dz = 0.0F; // Current LOS z and dz/per major step


    // Find the most detailed LOD which contains each point
    for (LOD_P = minLOD; LOD_P <= maxLOD; LOD_P++)
    {

        // Compute the coordinates of the lower left neighbor post
        Px = WORLD_TO_LEVEL_POST(p1->x, LOD_P);
        Py = WORLD_TO_LEVEL_POST(p1->y, LOD_P);

        // If the point lies within this level's range, stop looking
        if (blockLists[LOD_P].RangeFromCenter(Px, Py) <= blockLists[LOD_P].GetAvailablePostRange())
        {
            break;
        }
    }

    for (LOD_Q = minLOD; LOD_Q <= maxLOD; LOD_Q++)
    {

        // Compute the coordinates of the lower left neighbor post
        Qx = WORLD_TO_LEVEL_POST(p2->x, LOD_Q);
        Qy = WORLD_TO_LEVEL_POST(p2->y, LOD_Q);

        // If the point lies within this level's range, stop looking
        if (blockLists[LOD_Q].RangeFromCenter(Qx, Qy) <= blockLists[LOD_Q].GetAvailablePostRange())
        {
            break;
        }
    }

    // TODO:  make this work over multiple LODs
    // For now, use the highest detail which contains both points
    int LOD = max(LOD_Q, LOD_P);

    // TODO:  Clip the segment to the available map data
    // For now, if one or both of the points are entirely off our current map,
    // return FALSE (can't see between points)
    if (LOD > maxLOD)
    {
#define LOS_TRUE_IF_UNLOADED 1
#if LOS_TRUE_IF_UNLOADED
        // sfr: return TRUE if not loaded
        return TRUE;
#else
        return FALSE;
#endif

    }


    // Adjust the post coordinates as required
    if (LOD_P < LOD)
    {
        Px >>= LOD - LOD_P;
        Py >>= LOD - LOD_P;
    }

    if (LOD_Q < LOD)
    {
        Qx >>= LOD - LOD_Q;
        Qy >>= LOD - LOD_Q;
    }


    // Only check this LOD if at least one end point is "in" the terrain
    if ((p1->z > blockLists[LOD].GetMinZ()) or (p2->z > blockLists[LOD].GetMinZ()))
    {

        // Compute the z step rate
        z  = p1->z;
        dz = (p2->z - p1->z) / max(abs(Qx - Px), abs(Qy - Py));

        /****************************************************\
        Don't know why, buy Leon reports empirically better
        results with the following factor included.  I'll
        hopefully look into it later, but for now...
        \****************************************************/
        dz *= 0.2f;

        // Call the single LOD Line of Sight function
        if ( not SingleLODLineOfSight(Px, Py, Qx, Qy, z, dz, LOD))
        {
            return FALSE;
        }

    }

    return TRUE;
}


// Return TRUE if the two specified points can see each other over the terrain
// This version works only against a single LOD of the terrain
BOOL TViewPoint::SingleLODLineOfSight(int Px, int Py, int Qx, int Qy, float z, float dz, int LOD)
{
    int nr; // remainder
    int deltax, deltay; // Q.x - P.x, Q.y - P.y
    int k; // loop invariant constant
    int row, col; // Current row and column being checked
    BOOL hit; // Flag to indicate a terrain hit


    // Initialize values used in the following interations
    deltax = Qx - Px;
    deltay = Qy - Py;
    hit = FALSE;


    // Lock everyone else out of this viewpoint while we're using it
    EnterCriticalSection(&cs_update);


#define OCTANT(f1, f2, f3, f4, f5, i1, s1, r1, r2) \
 for (f1, f2, f3, nr=0; ((f4) and ( not hit)); f5) { \
 z += dz; \
   if (nr < k) { \
 if (i1) { \
 hit = r1(row,col,z,LOD); \
 } else { \
 hit = TestVertex(row,col,z,LOD); \
 } \
 } else { \
 s1; \
 if (nr -= k) { \
 hit  = r2(row,col,z,LOD); \
 if ( not hit) { \
 hit = r1(row,col,z,LOD); \
 } \
 } else { \
 hit = TestVertex(row,col,z,LOD); \
 } \
 } \
 }


    // For reference purposes, let theta be the angle from P to Q
    if ((deltax >= 0) and (deltay >= 0) and (deltay < deltax))   // theta < 45
    {
        OCTANT(row = Px + 1, col = Py, k = deltax - deltay,  row < Qx, row++, nr += deltay, col++, TestEast, TestSouth);
    }
    else if ((deltax > 0) and (deltay >= 0) and (deltay >= deltax))   // 45 <= theta < 90
    {
        OCTANT(col = Py + 1, row = Px, k = deltay - deltax,  col < Qy, col++, nr += deltax, row++, TestNorth, TestWest);
    }
    else if ((deltax <= 0) and (deltay >= 0) and (deltay > -deltax))  // 90 <= theta < 135
    {
        OCTANT(col = Py + 1, row = Px, k = deltay + deltax,  col < Qy, col++, nr -= deltax, row--, TestSouth, TestWest);
    }
    else if ((deltax <= 0) and (deltay > 0) and (deltay <= -deltax))  // 135 <= theta < 180
    {
        OCTANT(row = Px - 1, col = Py, k = -deltax - deltay, row > Qx, row--, nr += deltay, col++, TestEast, TestNorth);
    }
    else if ((deltax <= 0) and (deltay <= 0) and (deltay > deltax))   // 180 <= theta < 225
    {
        OCTANT(row = Px - 1, col = Py, k = -deltax + deltay, row > Qx, row--, nr -= deltay, col--, TestWest, TestNorth);
    }
    else if ((deltax < 0) and (deltay <= 0) and (deltay <= deltax))   // 225 <= theta < 270
    {
        OCTANT(col = Py - 1, row = Px, k = -deltay + deltax, col > Qy, col--, nr -= deltax, row--, TestSouth, TestEast);
    }
    else if ((deltax >= 0) and (deltay <= 0) and (-deltay > deltax))   // 270 <= theta < 315
    {
        OCTANT(col = Py - 1, row = Px, k = -deltay - deltax, col > Qy, col--, nr += deltax, row++, TestNorth, TestWest);
    }
    else if ((deltax >= 0) and (deltay < 0) and (-deltay <= deltax))   // 315 <= theta < 360
    {
        OCTANT(row = Px + 1, col = Py, k = deltax + deltay,  row < Qx, row++, nr -= deltay, col--, TestWest, TestSouth);
    }
    else   // P == Q
    {
    }


    // Unlock the viewpoint
    LeaveCriticalSection(&cs_update);

    return ( not hit);
}


// Return TRUE and fill in the "intersection" structure if a vector from the viewer's
// position in the given direction hit the terrain.  Return FALSE otherwise
BOOL TViewPoint::GroundIntersection(Tpoint *dir, Tpoint *intersection)
{
    int LOD;
    int range;
    BOOL stepUp, stepRt;
    int hStep, vStep;
    int endRow, endCol;
    int prevRow, prevCol;
    float dzdx, dzdy, dydx, dxdy;
    int row, col;
    int rowt, colt;
    float x, y, z;
    float xt, yt, zt;


    // Store parameters of the vector we're testing
    dzdx = dir->z / dir->x;
    dzdy = dir->z / dir->y;
    dydx = dir->y / dir->x;
    dxdy = dir->x / dir->y;
    stepUp = (dir->x > 0.0f);
    stepRt = (dir->y > 0.0f);
    hStep = stepRt ? 1 : -1;
    vStep = stepUp ? 1 : -1;

    // Lock everyone else out of this viewpoint while we're using it
    EnterCriticalSection(&cs_update);


    // Start at the eyepoint at the highest drawn detail level
    row = WORLD_TO_LEVEL_POST(X(), highDetail);
    col = WORLD_TO_LEVEL_POST(Y(), highDetail);


    // Walk through all the LODs being drawn
    for (LOD = highDetail; LOD <= lowDetail; LOD++)
    {

        // Get the available data range at this LOD -- skip if we don't have anything available
        range = blockLists[LOD].GetAvailablePostRange() - 1;

        if (range <= 0)
        {
            row >>= 1;
            col >>= 1;
            continue;
        }

        // Decide if we need to use x or y as the control variable
        if (fabs(dir->x) > fabs(dir->y))
        {
            // North/South dominant vector

            // Set up the stepping parameters
            if (stepUp)
            {
                endRow = WORLD_TO_LEVEL_POST(X(), LOD) + range;
                row += 1;
            }
            else
            {
                endRow = WORLD_TO_LEVEL_POST(X(), LOD) - range;
            }

            // Walk the line
            while (row not_eq endRow)
            {

                // Compute row/col for next check
                x = LEVEL_POST_TO_WORLD(row, LOD);
                prevCol = col;
                y = Y() + dydx * (x - X());
                z = Z() + dzdx * (x - X());
                col = WORLD_TO_LEVEL_POST(y, LOD);

                // Do vertical edge check if we've changed columns
                if (col not_eq prevCol)
                {

                    // Compute row/col for the check
                    rowt = min(row, row - vStep);
                    colt = max(col, prevCol);
                    yt = LEVEL_POST_TO_WORLD(colt, LOD);
                    xt = X() + dxdy * (yt - Y());
                    zt = Z() + dzdy * (yt - Y());

                    // Check vertical edge we crossed
                    if (verticalEdgeTest(rowt, colt, xt, yt, zt, LOD))
                    {
                        LineSquareIntersection(rowt, colt - stepRt, dir, intersection, LOD);

                        // Unlock the viewpoint
                        LeaveCriticalSection(&cs_update);

                        return TRUE;
                    }

                }

                // Check horizontal edge between (row,col) and (row,col+1)
                if (horizontalEdgeTest(row, col, x, y, z, LOD))
                {
                    LineSquareIntersection(row - stepUp, col, dir, intersection, LOD);

                    // Unlock the viewpoint
                    LeaveCriticalSection(&cs_update);

                    return TRUE;
                }


                // Take on vertical step
                row += vStep;
            }
        }
        else
        {
            // East/West dominant vector

            // Set up the stepping parameters
            if (stepRt)
            {
                endCol = WORLD_TO_LEVEL_POST(Y(), LOD) + range;
                col += 1;
            }
            else
            {
                endCol = WORLD_TO_LEVEL_POST(Y(), LOD) - range;
            }

            // Walk the line
            while (col not_eq endCol)
            {

                // Compute row/col for next check
                y = LEVEL_POST_TO_WORLD(col, LOD);
                prevRow = row;
                x = X() + dxdy * (y - Y());
                z = Z() + dzdy * (y - Y());
                row = WORLD_TO_LEVEL_POST(x, LOD);

                // Do horizontal edge check if we've changed rows
                if (row not_eq prevRow)
                {

                    // Compute row/col for next check
                    rowt = max(row, prevRow);
                    colt = min(col, col - hStep);
                    xt = LEVEL_POST_TO_WORLD(rowt, LOD);
                    yt = Y() + dydx * (xt - X());
                    zt = Z() + dzdx * (xt - X());

                    // Check horizontal edge between we crossed
                    if (horizontalEdgeTest(rowt, colt, xt, yt, zt, LOD))
                    {
                        LineSquareIntersection(rowt - stepUp, colt, dir, intersection, LOD);

                        // Unlock the viewpoint
                        LeaveCriticalSection(&cs_update);

                        return TRUE;
                    }

                }

                // Check vertical edge between (row,col) and (row+1,col)
                if (verticalEdgeTest(row, col, x, y, z, LOD))
                {
                    LineSquareIntersection(row, col - stepRt, dir, intersection, LOD);

                    // Unlock the viewpoint
                    LeaveCriticalSection(&cs_update);

                    return TRUE;
                }


                // Take a horizontal step
                col += hStep;
            }
        }


        // TODO:  Don't restart search at eyepoint each time
#if 0
        // Convert the row/col address we were about to check into the next
        // lowest detail level for the next loop iteration
        // (backup one post for good measure)
        row = (row - vStep) >> 1;
        col = (col - hStep) >> 1;
#else
        // Restart the search at the eyepoint to avoid the case where
        // we might be above ground at one LOD, but under at the next.
        row = WORLD_TO_LEVEL_POST(X(), LOD + 1);
        col = WORLD_TO_LEVEL_POST(Y(), LOD + 1);
#endif
    }


    // Unlock the viewpoint
    LeaveCriticalSection(&cs_update);

    // If we got here, we didn't find an intersection with the ground
    return FALSE;
}


//
// Helper functions used for the Ground Intersection and Line of Sight tests.
// Return TRUE if the given point is below (less negative than) the edge
// anchored at given row,col address.
//
//BOOL TViewPoint::horizontalEdgeTest( int row, int col, float x, float y, float z, int LOD )
BOOL TViewPoint::horizontalEdgeTest(int row, int col, float, float y, float z, int LOD)
{
    float t, height;
    Tpost *left, *right;

    // Get the relevant posts
    left = blockLists[LOD].GetPost(row, col);
    right = blockLists[LOD].GetPost(row, col + 1);

    // Compute the height of the edge at the point the line crosses it
    t = WORLD_TO_FLOAT_LEVEL_POST(y, LOD) - col;
    ShiAssert((t > -0.1f) and (t < 1.1f));   // Make it more tolerant since it actually works anyway
    height = left->z + t * (right->z - left->z);

    // Return true if the line crosses below the edge (ie: is less negative)
    return (height < z);
}

//BOOL TViewPoint::verticalEdgeTest( int row, int col, float x, float y, float z, int LOD )
BOOL TViewPoint::verticalEdgeTest(int row, int col, float x, float, float z, int LOD)
{
    float t, height;
    Tpost *top, *bottom;

    // Get the relevant posts
    top = blockLists[LOD].GetPost(row + 1, col);
    bottom = blockLists[LOD].GetPost(row,   col);

    // Compute the height of the edge at the point the line crosses it
    t = WORLD_TO_FLOAT_LEVEL_POST(x, LOD) - row;

    // OW
    //ShiAssert( (t >= -0.1f) and (t <= 1.1f) );
    height = bottom->z + t * (top->z - bottom->z);

    // Return true if the line crosses below the edge (ie: is less negative)
    return (height < z);
}


//
// Helper function used for the Ground Intersection test.
// Assume a line from the viewpoint in the given direction intersects
// the square whose lower left corner post is at (row,col).  Return
// the exact location of the intersection in the provided Tpoint structure.
//
void TViewPoint::LineSquareIntersection(int row, int col, Tpoint *dir, Tpoint *intersection, int LOD)
{
    Tpost *SW, *NW, *NE, *SE;
    float Nx, Ny, Nz;
    float SWx, SWy, SWz;
    float PQdotN;
    float NdotDIR;
    float t;

    // Get the posts which bound this square
    SW = blockLists[LOD].GetPost(row,   col);
    NW = blockLists[LOD].GetPost(row + 1, col);
    NE = blockLists[LOD].GetPost(row + 1, col + 1);
    SE = blockLists[LOD].GetPost(row,   col + 1);
    ShiAssert(SW and NW and NE and SE);

    // Store the world space location of the upper left and lower right corner posts
    SWx = LEVEL_POST_TO_WORLD(row, LOD);
    SWy = LEVEL_POST_TO_WORLD(col, LOD);
    SWz = SW->z;

    // Compute the normal from the three posts which bound the point of interest
    // (don't forget - positive Z is DOWN)
    Nz = -TheMap.Level(LOD)->FTperPOST();

    // upper left triangle (remember positive Z is down)
    Nx = NW->z - SW->z;
    Ny = NE->z - NW->z;

    // Only check the first triangle if it is front facing relative to the test ray
    if (Nx * dir->x + Ny * dir->y + Nz * dir->z < 0.0f)
    {

        // Compute the intersection of the line with the plane
        PQdotN = (SWx - X()) * Nx + (SWy - Y()) * Ny + (SWz - Z()) * Nz;
        NdotDIR = Nx * dir->x     + Ny * dir->y      + Nz * dir->z;
        t = PQdotN / NdotDIR;

        intersection->x = X() + t * dir->x;
        intersection->y = Y() + t * dir->y;
        intersection->z = Z() + t * dir->z;

        // Return now if the intersection is within the upper left half space
        if ((intersection->x - SWx) >= (intersection->y - SWy))
        {
            return;
        }

    }


    // upper right triangle (remember positive Z is down)
    Nx = NE->z - SE->z;
    Ny = SE->z - SW->z;

    // Compute the intersection of the line with the plane
    PQdotN = (SWx - X()) * Nx + (SWy - Y()) * Ny + (SWz - Z()) * Nz;
    NdotDIR = Nx * dir->x     + Ny * dir->y      + Nz * dir->z;
    t = PQdotN / NdotDIR;

    intersection->x = X() + t * dir->x;
    intersection->y = Y() + t * dir->y;
    intersection->z = Z() + t * dir->z;

    // Make sure the intersection we found is within the lower right half space
    // Rounding errors could make this assertion fail occasionally
    // ShiAssert( (intersection->x-SWx) <= (intersection->y-SWy) );

}
