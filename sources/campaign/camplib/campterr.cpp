#include <stddef.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <fcntl.h>
#include <io.h>
#include <math.h>
#include "CmpGlobl.h"
#include "CampCell.h"
#include "CampTerr.h"
#include "F4Find.h"
#include "Entity.h"
#include "ASearch.h"
#include "Campaign.h"
//sfr: checks
#include "InvalidBufferException.h"

#ifdef DEBUG
#include "CmpClass.h"
#endif

// =============================================
// Campaign Terrain ADT - Private Implementation
// =============================================
                       
CellDataType 	*TheaterCells = NULL;
unsigned char	EastLongitude;
unsigned char	SouthLatitude;
float    		Latitude;
float    		Longitude;
float    		CellSizeInKilometers;

short			Map_Max_X = 0;
short			Map_Max_Y = 0;

// -------------------------
// Local Function Prototypes
// =========================

// -------------------------
// External Function Prototypes
// =========================

// ---------------------------------
// Global Function (ADT) Definitions
// =================================

void InitTheaterTerrain (void)
	{
	if (TheaterCells)
		FreeTheaterTerrain();
	TheaterCells = new CellDataType[Map_Max_X*Map_Max_Y];
	memset(TheaterCells,0,sizeof(CellDataType)*Map_Max_X*Map_Max_Y);
	}

void FreeTheaterTerrain (void)
	{
	if (TheaterCells)
		delete [] TheaterCells;
	TheaterCells = NULL;
	}

int LoadTheaterTerrain (char* name){
	//char *data, *data_ptr;
	
	FreeTheaterTerrain();

	CampaignData cd = ReadCampFile (name, "thr");
	if (cd.dataSize == -1){
		return 0;
	}

	long rem = cd.dataSize;
	VU_BYTE *data_ptr = (VU_BYTE*)cd.data;
	
	memcpychk(&Map_Max_X, &data_ptr, sizeof(short), &rem);
	memcpychk(&Map_Max_Y, &data_ptr, sizeof(short), &rem);
	
#ifdef DEBUG
	ShiAssert(Map_Max_X == TheCampaign.TheaterSizeX);
	ShiAssert(Map_Max_Y == TheCampaign.TheaterSizeY);
#endif

	InitTheaterTerrain();
	
	memcpychk(TheaterCells, &data_ptr, sizeof (CellDataType) * Map_Max_X * Map_Max_Y, &rem);
	
	delete cd.data;
	
	return 1;
}

int LoadTheaterTerrainLight (char* name)
{
	FILE	*fp;

	FreeTheaterTerrain();
	if ((fp = OpenCampFile (name, "thr", "rb")) == NULL)
		return 0;
	fread(&Map_Max_X,sizeof(short),1,fp);
	fread(&Map_Max_Y,sizeof(short),1,fp);
	CloseCampFile (fp);
	return 1;
}

int SaveTheaterTerrain (char* name)
	{
	FILE		*fp;

	if (!TheaterCells)
		return 0;
	if ((fp = OpenCampFile (name, "thr", "wb")) == NULL)
		return 0;
	fwrite(&Map_Max_X,sizeof(short),1,fp);
	fwrite(&Map_Max_Y,sizeof(short),1,fp);
	fwrite(TheaterCells,sizeof(CellDataType),Map_Max_X*Map_Max_Y,fp);
	CloseCampFile(fp);
	return 1;
	}

CellData GetCell (GridIndex x, GridIndex y)
	{
    ShiAssert(x >= 0 && x < Map_Max_X && y >= 0 && y < Map_Max_Y);
	return &TheaterCells[x*Map_Max_Y + y];
	}

ReliefType GetRelief (GridIndex x, GridIndex y)
	{
    ShiAssert(x >= 0 && x < Map_Max_X && y >= 0 && y < Map_Max_Y);
	return (ReliefType)((TheaterCells[x*Map_Max_Y + y] & ReliefMask) >> ReliefShift);
	}

CoverType GetCover (GridIndex x, GridIndex y)
	{
	if ((x< 0) || (x >= Map_Max_X) || (y < 0) || (y >= Map_Max_Y))
		return (CoverType) Water;
	else
		return (CoverType)((TheaterCells[x*Map_Max_Y + y] & GroundCoverMask) >> GroundCoverShift);
	}

char GetRoad (GridIndex x, GridIndex y)
	{
    ShiAssert(x >= 0 && x < Map_Max_X && y >= 0 && y < Map_Max_Y);
	return (char)((TheaterCells[x*Map_Max_Y + y] & RoadMask) >> RoadShift);
	}

char GetRail (GridIndex x, GridIndex y)
	{
    ShiAssert(x >= 0 && x < Map_Max_X && y >= 0 && y < Map_Max_Y);
	return (char)((TheaterCells[x*Map_Max_Y + y] & RailMask) >> RailShift);
	}


