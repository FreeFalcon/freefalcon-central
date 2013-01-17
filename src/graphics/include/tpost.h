/***************************************************************************\
    Tpost.h
    Scott Randolph
    August 21, 1995

    Most basic element of the terrain map.  Posts contain all the
    information about a point on the ground.  There may in the future be
    multiple versions of posts (ie: textured vs. not textured).  For now,
    only one.

	This is version that is used at runtime in memory.
\***************************************************************************/
#ifndef _TPOST_H_
#define _TPOST_H_

#include "TerrTex.h"
#include "FarTex.h"
#include "Ttypes.h"


typedef struct Tpost {

	// X can be obtained from:  	x = LEVEL_POST_TO_WORLD( levelRow, LOD );
	// Y can be obtained from:  	y = LEVEL_POST_TO_WORLD( levelCol, LOD );
	float		z;		// Units of floating point feet -- positive z axis points DOWN

	// Color information
	int			colorIndex;	// (could shrink to a byte)

	// Texture information
	float		u;		// u is East/West         
	float		v;		// v is North/South       
	float		d;   
	TextureID	texID;	// 16 bit value (see TerrTex.h)	   

	// normal information
	float		theta;
	float		phi;

} Tpost;


#endif // _TPOST_H_
