/***************************************************************************\
    Ttypes.h
    Scott Randolph
    August 23, 1995

    This file provides forward declarations for all the classes used in the
    terrain database module.  This allows pointer declarations without
    the need for the full class definition.
\***************************************************************************/
#ifndef _TTYPES_H_
#define _TTYPES_H_

#include <math.h>
#include "grtypes.h"


// Number of feet between posts at highest detail
// (This global constant is provided by the TMap.CPP source file)
extern float FeetPerPost;


#define POST_OFFSET_BITS			4
#define POST_OFFSET_MASK			(~(((~1)>>POST_OFFSET_BITS)<<POST_OFFSET_BITS))
#define POSTS_ACROSS_BLOCK			(1<<POST_OFFSET_BITS)
#define POSTS_PER_BLOCK				(1<<(POST_OFFSET_BITS<<1))

#define WORLD_TO_FLOAT_GLOBAL_POST( distance )	((distance)/FeetPerPost)
#define WORLD_TO_GLOBAL_POST( distance )		(FloatToInt32((float)floor(WORLD_TO_FLOAT_GLOBAL_POST(distance))))
#define WORLD_TO_FLOAT_LEVEL_POST( distance, LOD )	((distance) / TheMap.Level(LOD)->FTperPOST())
#define WORLD_TO_LEVEL_POST( distance, LOD )	(WORLD_TO_GLOBAL_POST( distance ) >> (LOD))
#define WORLD_TO_LEVEL_BLOCK( distance, LOD )	(WORLD_TO_GLOBAL_POST( distance ) >> ((LOD)+POST_OFFSET_BITS))

#define GLOBAL_POST_TO_WORLD( distance )		((distance)*FeetPerPost)
#define LEVEL_POST_TO_WORLD( distance, LOD )	(GLOBAL_POST_TO_WORLD( (distance)<<(LOD) ))
#define LEVEL_BLOCK_TO_WORLD( distance, LOD )	(GLOBAL_POST_TO_WORLD( (distance)<<((LOD)+POST_OFFSET_BITS) ))

#define LEVEL_POST_TO_LEVEL_BLOCK( distance )	((distance) >> POST_OFFSET_BITS)
#define LEVEL_POST_TO_BLOCK_POST( distance )	((distance) & POST_OFFSET_MASK)

#define GLOBAL_POST_TO_LEVEL_POST( distance, LOD )	((distance) >> (LOD))
#define LEVEL_POST_TO_GLOBAL_POST( distance, LOD )	((distance) << (LOD))


class TMap;
class TLevel;
class TBlock;
class TViewer;
class TBlockList;
struct Tpost;

#endif // _TTYPES_H_
