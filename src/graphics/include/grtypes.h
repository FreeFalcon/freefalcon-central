/***************************************************************************\
    types.h
    Scott Randolph
    November 24, 1994

    Basic data types used by the Terrain Overflight application.
    This file is intended to be platform specific.
\***************************************************************************/
#ifndef _TYPES_H_
#define _TYPES_H_

//#include "mathlib\math.h"
//#include "mathlib\vector.h"
//#include "mathlib\matrix.h"
//#include "mathlib\color.h"
//#include "context.h"

// Convienient values to have arround

#include "../../codelib/include/shi/ConvFtoI.h"

//#include "shi\ConvFtoI.h"
#include "../../codelib/include/shi/ShiError.h"

#ifdef USE_SMART_HEAP
#include <stdlib.h>
#include "SmartHeap\Include\smrtheap.hpp"
#endif

#include "Constant.h"


#if 0	// I don't think these are used anymore....
// Useful basic data types
typedef signed char		INT8;
typedef unsigned char	UINT8;
typedef short			INT16;
typedef unsigned short	UINT16;
typedef long			INT32;
typedef unsigned long	UINT32;
typedef UINT8			BYTE;
typedef UINT16			WORD;
typedef UINT32			DWORD;
#endif


// Three by three rotation matrix
typedef struct Trotation {
	float	M11, M12, M13;
	float	M21, M22, M23;
	float	M31, M32, M33;
} Trotation;

// Three space point// Three space point
typedef struct Tpoint {
	float x, y, z;
} Tpoint;

//typedef struct vector3 {
//	float x, y, z;
//} vector3;

// RGB color
typedef struct Tcolor {
	float	r;
	float	g;
	float	b;

	// Pack Functions
	inline unsigned int Pack()
	{
		return (((long)((1.0f) * 255)) << 24) | (((long)((r) * 255)) << 16) | (((long)((g) * 255)) << 8) | (long)((b) * 255);
	}
	inline unsigned int Pack(float fScale)
	{
		return (((long)((1.0f) * 255)) << 24) | (((long)((r*fScale) * 255)) << 16) | (((long)((g*fScale) * 255)) << 8) | (long)((b*fScale) * 255);
	}

	// Static Pack Functions
	static unsigned int PackRGBA(float r,float g,float b, float)
	{
		return (((long)((1.0f) * 255)) << 24) | (((long)((r) * 255)) << 16) | (((long)((g) * 255)) << 8) | (long)((b) * 255);
	}
	static unsigned int PackRGBA(float r,float g,float b, float, float fScale)
	{
		return (((long)((1.0f) * 255)) << 24) | (((long)((r*fScale) * 255)) << 16) | (((long)((g*fScale) * 255)) << 8) | (long)((b*fScale) * 255);
	}

} Tcolor;


// Math constants
#ifndef PI
#define PI			3.14159265359f
#endif
#define TWO_PI		6.28318530718f
#define PI_OVER_2	1.570796326795f
#define PI_OVER_4	0.7853981633974f


#endif /* _TYPES_H_ */
