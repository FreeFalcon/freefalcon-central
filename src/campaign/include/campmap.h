//
// CampMap.h
//

#ifndef CAMPMAP_H
#define CAMPMAP_H

#include "vutypes.h"
#include "campterr.h"

// Map Types
#define	MAP_OWNERSHIP		1
#define MAP_SAMCOVERAGE		2
#define MAP_RADARCOVERAGE	3
#define MAP_PAK				4
#define MAP_PAK_BUILD		5					// PAK map builder - tool ONLY

#define MAP_RATIO			6					// Number of km to squeeze into one pixel
#define PAK_MAP_RATIO		4					// Same, but for the PAK map

// Map Ratio Globals
extern int MRX;
extern int MRY;

// Functions
extern uchar* MakeCampMap (int type, uchar* map_data, int csize);

extern uchar* UpdateCampMap (int type, uchar* map_data, GridIndex cx, GridIndex cy);

extern uchar GetOwner (uchar* map_data, GridIndex x, GridIndex y);

extern void FreeCampMap (uchar* map_data);

#endif