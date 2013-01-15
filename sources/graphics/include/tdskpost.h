/***************************************************************************\
    TDskPost
    Scott Randolph
    November 9, 1995

    Most basic element of the terrain map.  Posts contain all the
    information about a point on the ground.  There may in the future be
    multiple versions of posts (ie: textured vs. not textured).  For now,
    only one.

	This is version that is used to store the map in the disk file.
\***************************************************************************/
#ifndef _TDSKPOST_H_
#define _TDSKPOST_H_

#include "grTypes.h"


#pragma pack (push, 1)		// Force 1 byte alignment
typedef struct TdiskPost {

	// Texture information
	UInt16		texID;		// Max 64k textures

	// Position information  	
	Int16		z;			// Feet above sea level

	// Color index
	UInt8		color;	// Needs 8 bits

	// normal information
	UInt8		theta;	// Needs 8 bits
	UInt8		phi;	// Needs 6 bits (8 now for efficient access)

} TdiskPost;

typedef struct TNewdiskPost { // newer larger structure

	// Texture information
	UInt32		texID;		// Max 4 billion textures
	// Position information  	
	Int16		z;			// Feet above sea level

	// Color index
	UInt8		color;	// Needs 8 bits

	// normal information
	UInt8		theta;	// Needs 8 bits
	UInt8		phi;	// Needs 6 bits (8 now for efficient access)

} TNewdiskPost;
#pragma pack (pop)				// Force 1 byte alignment


// Declarations for conversion functions between this type and the one used in memory at run time
void DiskblockToMemblock( Tpost *memPost, TdiskPost *diskPost, int LOD, float lightLevel, float *minZ, float *maxZ );
void LargeDiskblockToMemblock( Tpost *memPost, TNewdiskPost *diskPost, int LOD, float lightLevel, float *minZ, float *maxZ );

extern bool g_LargeTerrainFormat;

#endif // _TDSKPOST_H_
