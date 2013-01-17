// ====================
// Campaign Terrain ADT
// ====================

#ifndef CAMPTERR
#define CAMPTERR

#include "CmpGlobl.h"
#include "CampCell.h"

// ---------------------------------------
// Type and External Function Declarations
// ---------------------------------------

#define GroundCoverMask    0x0F // 0xF0
#define GroundCoverShift   0
#define ReliefMask         0x30 // 0xCF
#define ReliefShift        4
#define RoadMask           0x40 // 0xBF
#define RoadShift          6
#define RailMask           0x80 // 0x7F
#define RailShift          7

typedef Int16 GridIndex;

typedef struct gridloctype 
	{
	GridIndex		x;
	GridIndex		y;
	} GridLocation;

extern short				Map_Max_X;							// World Size, in grid coordinates
extern short				Map_Max_Y;

extern void InitTheaterTerrain (void);

extern void FreeTheaterTerrain (void);

extern int LoadTheaterTerrain (char* FileName);

extern int LoadTheaterTerrainLight (char* name);

extern int SaveTheaterTerrain (char* FileName);

extern CellData GetCell (GridIndex x, GridIndex y);

extern ReliefType GetRelief (GridIndex x, GridIndex y);

extern CoverType GetCover (GridIndex x, GridIndex y);

extern char GetRoad (GridIndex x, GridIndex y);

extern char GetRail (GridIndex x, GridIndex y);

#endif
