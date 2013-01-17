/***************************************************************************\
    ClipFlags.h
    Scott Randolph
    February 27, 1998

    Provides the flags and arbitrary near clip distance for the BSPlib
	clipping routines.
\***************************************************************************/
#ifndef _CLIPFLAGS_H_
#define _CLIPFLAGS_H_

#include <math.h>

static const float NEAR_CLIP_DISTANCE = 1.0f;

static const UInt32	ON_SCREEN			= 0x00;
static const UInt32	CLIP_LEFT			= 0x01;
static const UInt32	CLIP_RIGHT			= 0x02;
static const UInt32	CLIP_TOP			= 0x04;
static const UInt32	CLIP_BOTTOM			= 0x08;
static const UInt32	CLIP_NEAR			= 0x10;
static const UInt32	CLIP_FAR			= 0x20;
static const UInt32	OFF_SCREEN			= 0xFF;


/* Helper functions to compute the horizontal, vertical, and near clip flags */
static inline UInt32 GetRangeClipFlags( float z, float )
{
	if ( z < NEAR_CLIP_DISTANCE ) {
		return CLIP_NEAR;
	}
	return ON_SCREEN;
}

static inline UInt32 GetHorizontalClipFlags( float x, float z )
{
	if ( fabs(x) > z ) {
		if ( x > z ) {
			return CLIP_RIGHT;
		} else {
			return CLIP_LEFT;
		}
	}
	return ON_SCREEN;
}

static inline UInt32 GetVerticalClipFlags( float y, float z )
{
	if ( fabs(y) > z ) {
		if ( y > z ) {
			return CLIP_BOTTOM;
		} else {
			return CLIP_TOP;
		}
	}
	return ON_SCREEN;
}
#endif // _CLIPFLAGS_H_