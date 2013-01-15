//
// MakeThr.cpp
//
// Reads data from various sources and merges into the campaign theater (.thr) file
//

#include <stdio.h>
#include <conio.h>
#include <stddef.h>
#include <fcntl.h>
#include <io.h>
#include <stdlib.h>
#include "omni.h"
#include "MakeThr.h"
#include "CampTerr.h"
#include "CampCell.h"


int Max_Textures = 0;
char TheaterName[80];

// TextureEntry *ConversionTable;

extern	CellDataType 	*TheaterCells;

//	Reg	Road
// 0		16		Water
// 1		17		Bog										
// 2		18		Barren
// 3		19		Plain
// 4		20		Brush
// 5		21		LightForest
// 6		22		HeavyForest
// 7		23		Urban


// =============================================
// Campaign Terrain ADT - Private Implementation
// =============================================
                       
CellDataType 	*TheaterCells = NULL;
boolean  		EastLongitude;
boolean  		SouthLatitude;
float    		Latitude;
float    		Longitude;
float    		CellSizeInKilometers;

short			Map_Max_X;
short			Map_Max_Y;

#define			ROADMAP_SIZE		4				// Pixels per km of roadmap/rivermap

// -------------------------
// Local Function Prototypes
// =========================

// ----------------------------
// External Function Prototypes
// ============================

// ---------------------------------
// Global Function (ADT) Definitions
// =================================

// ========================
// Function Stubs
// ========================

FILE* OpenCampFile (char* name, char* ext, char *mode)
	{
	char	filename[MAX_PATH];

	sprintf(filename,"%s\\%s.%s",baseDirectory,name,ext);
	return fopen(filename,mode);
	}

// -----------------------------
// CampTerr functions
// =============================

void InitTheaterTerrain (void)
	{
	if (TheaterCells)
		FreeTheaterTerrain();
	TheaterCells = (CellDataType*) F4AllocMemory(sizeof(CellDataType)*Map_Max_X*Map_Max_Y);
	memset(TheaterCells,0,sizeof(CellDataType)*Map_Max_X*Map_Max_Y);
	}

void FreeTheaterTerrain (void)
	{
	if (TheaterCells)
		F4FreeMemory(TheaterCells);
	TheaterCells = NULL;
	}

int LoadTheaterTerrain (char* name)
	{
	FILE	*fp;

	FreeTheaterTerrain();
	if ((fp = OpenCampFile (name, "thr", "rb")) == NULL)
		return 0;
	fread(&Map_Max_X,sizeof(short),1,fp);
	fread(&Map_Max_Y,sizeof(short),1,fp);
	InitTheaterTerrain();
	fread(TheaterCells,sizeof(CellDataType),Map_Max_X*Map_Max_Y,fp);
	fclose(fp);
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
	fclose(fp);
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
	fclose(fp);
	return 1;
	}

   CellData GetCell (GridIndex x, GridIndex y)
 /*ษอออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออป
   บ Will return the CellData ADT element associated with the GridIndex  บ
   บ coordinates specified. See the CellData ADT (CampCell.H) for a list บ
   บ of the operations which can be performed on the returned CellData.  บ
   ศออออออออออออออออออออออออออออออออออฯออออออออออออออออออออออออออออออออออผ*/
      {
      return &TheaterCells[x*Map_Max_Y + y];
      }

	ReliefType GetRelief (GridIndex x, GridIndex y)
      {
      return (ReliefType)((TheaterCells[x*Map_Max_Y + y] & ReliefMask) >> ReliefShift);
      }

   CoverType GetCover (GridIndex x, GridIndex y)
      {
      return (CoverType)((TheaterCells[x*Map_Max_Y + y] & GroundCoverMask) >> GroundCoverShift);
      }

   char GetRoad (GridIndex x, GridIndex y)
      {
      return (char)((TheaterCells[x*Map_Max_Y + y] & RoadMask) >> RoadShift);
      }

   char GetRail (GridIndex x, GridIndex y)
      {
      return (char)((TheaterCells[x*Map_Max_Y + y] & RailMask) >> RailShift);
      }

// ======================
// Terrain setters
// ======================

void SetGroundCover (int i, int cover)
	{
	TheaterCells[i] &= ~GroundCoverMask;
	TheaterCells[i] |= (cover << GroundCoverShift) & GroundCoverMask;
	}

void SetRelief (int i, int relief)
	{
	TheaterCells[i] &= ~ReliefMask;
	TheaterCells[i] |= (relief << ReliefShift) & ReliefMask;
	}

void SetRoad (int i, int road)
	{
	TheaterCells[i] &= ~RoadMask;
	TheaterCells[i] |= (road << RoadShift) & RoadMask;
	}
//VP_changes Railroads
void SetRail (int i, int rail)
	{
	TheaterCells[i] &= ~RailMask;
	TheaterCells[i] |= (rail << RailShift) & RailMask;
	}

// ==========================
// Support functions
// ==========================

void ReadComments (FILE* fh)
	{
	int					c;

	c = fgetc(fh);
	while (c == '\n')
		c = fgetc(fh);
	while (c == '/' && !feof(fh))
		{
		c = fgetc(fh);
		while (c != '\n' && !feof(fh))
			c = fgetc(fh);
		while (c == '\n')
			c = fgetc(fh);
		}
	ungetc(c,fh);
	}

void ProcessTextureFile (void)
	{
	FILE		*fp;
	uchar		*texture;
	short		tex;
	int			x,y,i,j;
	char		name[80];

	printf("\nProcessing Texture file...  ");
	texture = (uchar*) malloc(Map_Max_X*Map_Max_Y*sizeof(short));
	sprintf(name,"%s-F",TheaterName);
	if ((fp = OpenCampFile(name,"COV","rb")) == NULL)
		{
		printf("Error reading texture file!\n");
		printf("Press a key to continue:");
		getch();
		exit(-1);
		}
	fread(texture,sizeof(uchar),Map_Max_X*Map_Max_Y,fp);
	fclose (fp);
	for (y=0; y<Map_Max_Y; y++)
		{
		for (x=0; x<Map_Max_X; x++)
			{
			j = ((Map_Max_Y-1-y)*Map_Max_X) + x;
			i = (x*Map_Max_Y) + y;
			tex = texture[j];
			if (tex < 8)
				SetGroundCover(i,tex);
			else if (tex < 16)
				{
				SetGroundCover(i,tex-8);
				SetRoad(i,1);
				}
			else if (tex < 24)
				{
				SetGroundCover(i,tex-16);
				SetRail(i,1);
				}
			else
				SetGroundCover(i,0);
			}
		}
	printf("\n");
	free(texture);
	}

void ProcessReliefFile (void)
	{
	FILE		*fp;
	uchar		*normals;
	char		name[80];
	int			x,y,ulx,uly,xx,yy,i,relief;

	printf("\nProcessing Relief file...");
	normals = (uchar*) malloc(Map_Max_X*Map_Max_Y*16*sizeof(short));
	sprintf(name,"%s-N",TheaterName);
	if ((fp = OpenCampFile(name,"RAW","rb")) == NULL)
		{
		printf("Error reading normals file!\n");
		printf("Press a key to continue:");
		getch();
		exit(-1);
		}
	fread(normals,sizeof(uchar),Map_Max_X*Map_Max_Y*16,fp);
	fclose (fp);
	for (y=0; y<Map_Max_Y; y++)
		{
		for (x=0; x<Map_Max_X; x++)
			{
			i = (x*Map_Max_Y) + y;
			relief = 0;
			ulx = x*4;
			uly = y*4;
			for (yy = uly; yy < uly+4; yy++)
				{
				for (xx = ulx; xx < ulx+4; xx++)
					relief += ((normals[(yy*Map_Max_X*4) + xx] & 0xE0) >> 5);
				}
			// relief is now the total relief of the km square region (0-7 * 16)
			// relief/16 would get average- I want a semi-average, plus a convert range 0-3
			relief /= 28;
			if (relief > 3)
				relief = 3;
			SetRelief(i,relief);
			}
		if (!(y%100))
			printf(".");
		}
	printf("\n");
	free(normals);
	}

void DoConnections (uchar *data, int i, int n, int s, int e, int w, int val, int nodia)
	{
	if (w == val)
		{
		// West catigory
		if (e == val)
			{
			// East/west + others
			data[i] = val;
			data[i+1] = val;
			if (s == val)
				{
				// East/West/South
				data[i+1+(Map_Max_X*ROADMAP_SIZE)] = val;
				}
			}
		else if (s == val)
			{
			// West/south + others
			if (n == val)
				{
				// West/South/North
				data[i] = val;
				data[i+1] = val;
				data[i+1+(Map_Max_X*ROADMAP_SIZE)] = val;
				}
			else
				{
				if (nodia)
					data[i+(Map_Max_X*ROADMAP_SIZE)] = val;
				else
					{
					data[i] = val;
					data[i+1+(Map_Max_X*ROADMAP_SIZE)] = val;
					}
				}
			}
		else if (n == val)
			{
			// West/north 
			data[i] = val;
			}
		else
			{
			// West only
			data[i] = val;
			}
		}
	else if (e == val)
		{
		// East catigory
		if (n== val)
			{
			// East/North + others
			if (s == val)
				{
				// East/North/South
				data[i+1] = val;
				data[i+1+(Map_Max_X*ROADMAP_SIZE)] = val;
				}
			else
				{
				}
			}
		else if (s == val)
			{
			// East/South 
			data[i+1+(Map_Max_X*ROADMAP_SIZE)] = val;
			}
		else
			{
			// East only
			data[i+1] = val;
			}
		}
	else if (s == val)
		{
		// South catigory
		data[i+1+(Map_Max_X*ROADMAP_SIZE)] = val;
		if (n == val)
			{
			// North/South 
			data[i+1] = val;
			}
		}
	else if (n == val)
		{
		// North catigory
		data[i+1] = val;
		}
	}

void BuildMapData (char* name)
	{
	uchar		*MapData;
	GridIndex	x,y,rx,ry;
	CoverType	cov,wcov,ncov,ecov,scov;
	int			i;
	char		fname[80];
	FILE		*fp;

	printf("\nBuilding map data...");
	MapData = (uchar*)malloc(Map_Max_X*Map_Max_Y*ROADMAP_SIZE*ROADMAP_SIZE);
	memset(MapData,0,Map_Max_X*Map_Max_Y*ROADMAP_SIZE*ROADMAP_SIZE);
	for (x=0; x<Map_Max_X; x++)
		{
		for (y=0; y<Map_Max_Y; y++)
			{
			rx = x*ROADMAP_SIZE;
			ry = (Map_Max_Y-y-1)*ROADMAP_SIZE;
			i = (ry*Map_Max_X*ROADMAP_SIZE) + rx;
			cov = wcov = ecov = ncov = scov = GetCover(x,y);
			MapData[i] = cov;
			MapData[i+1] = cov;
			MapData[i+(Map_Max_X*ROADMAP_SIZE)] = cov;
			MapData[i+1+(Map_Max_X*ROADMAP_SIZE)] = cov;
			if (x > 0)
				wcov = GetCover(x-1,y);
			if (y < Map_Max_Y-1)
				ncov = GetCover(x,y+1);
			if (x < Map_Max_X-1)
				ecov = GetCover(x+1,y);
			if (y > 0)
				scov = GetCover(x,y-1);
			if ((wcov == Plain || wcov == Brush) && (ncov == Plain || ncov == Brush))
				MapData[i] = wcov;
			if ((ncov == Plain || ncov == Brush) && (ecov == Plain || ecov == Brush))
				MapData[i+1] = ncov;
			if ((ecov == Plain || ecov == Brush) && (scov == Plain || scov == Brush))
				MapData[i+1+(Map_Max_X*ROADMAP_SIZE)] = ecov;
			if ((scov == Plain || scov == Brush) && (wcov == Plain || wcov == Brush))
				MapData[i+(Map_Max_X*ROADMAP_SIZE)] = scov;
			if (cov == Water)
				{
				if (wcov != Water)
					{
					MapData[i] = wcov;
					MapData[i+(Map_Max_X*ROADMAP_SIZE)] = wcov;
					}
				if (ncov != Water)
					{
					MapData[i] = ncov;
					MapData[i+1] = ncov;
					}
				if (ecov != Water)
					{
					MapData[i+1] = ecov;
					MapData[i+1+(Map_Max_X*ROADMAP_SIZE)] = ecov;
					}
				if (scov != Water)
					{
					MapData[i+(Map_Max_X*ROADMAP_SIZE)] = scov;
					MapData[i+1+(Map_Max_X*ROADMAP_SIZE)] = scov;
					}
				int tot = 0;
				if (ncov == Water)	tot++;
				if (scov == Water)	tot++;
				if (wcov == Water)	tot++;
				if (ecov == Water)	tot++;
				if (tot < 3)
					DoConnections(MapData,i,ncov,scov,ecov,wcov,Water,1);
				}
			}
		}
	sprintf(fname,"%s-M",name);
	if ((fp = OpenCampFile (fname, "raw", "wb")) == NULL)
		return;
	fwrite(MapData,sizeof(uchar),Map_Max_X*Map_Max_Y*ROADMAP_SIZE*ROADMAP_SIZE,fp);
	fclose(fp);
	printf("\n");
	}

/*
typedef enum {	Water,                           // Cover types
				Bog,										
				Barren,
				Plain,
				Brush,
				LightForest,
				HeavyForest,
				Urban } CoverType;
*/

int SaveRoadData (char* name)
	{
	FILE		*fp;
	uchar		*RoadData;
	GridIndex	x,y,rx,ry;
	int			i;
	char		fname[80];

	printf("\nBuilding road data...");
	RoadData = (uchar*)malloc(Map_Max_X*Map_Max_Y*ROADMAP_SIZE*ROADMAP_SIZE);
	memset(RoadData,0,Map_Max_X*Map_Max_Y*ROADMAP_SIZE*ROADMAP_SIZE);
	for (x=1; x<Map_Max_X-1; x++)
		{
		for (y=1; y <Map_Max_Y-1; y++)
			{
			if (GetRoad(x,y))
				{
				rx = x*ROADMAP_SIZE;
				ry = (Map_Max_Y-y-1)*ROADMAP_SIZE;
				i = (ry*Map_Max_X*ROADMAP_SIZE) + rx;
				if (ROADMAP_SIZE == 4)
					{
					if (GetRoad(x-1,y))
						{
						// Road to west catigory
						if (GetRoad(x+1,y))
							{
							// East west road + others
							RoadData[i+(Map_Max_X*4)] = 1;
							RoadData[i+1+(Map_Max_X*4)] = 1;
							RoadData[i+2+(Map_Max_X*4)] = 1;
							RoadData[i+3+(Map_Max_X*4)] = 1;
							if (GetRoad(x,y-1))
								{
								// South branch
								RoadData[i+2+2*(Map_Max_X*4)] = 1;
								RoadData[i+2+3*(Map_Max_X*4)] = 1;
								}
							if (GetRoad(x,y+1))
								RoadData[i+2] = 1;
							}
						else if (GetRoad(x,y-1))
							{
							// West/south road + others
							if (GetRoad(x,y+1))
								{
								// 3 way intersection
								RoadData[i+(Map_Max_X*4)] = 1;
								RoadData[i+1+(Map_Max_X*4)] = 1;
								RoadData[i+2+(Map_Max_X*4)] = 1;
								RoadData[i+2] = 1;
								RoadData[i+2+2*(Map_Max_X*4)] = 1;
								RoadData[i+2+3*(Map_Max_X*4)] = 1;
								}
							else
								{
								RoadData[i+2*(Map_Max_X*4)] = 1;
								RoadData[i+1+3*(Map_Max_X*4)] = 1;
								}
							}
						else if (GetRoad(x,y+1))
							{
							// West/north road only
							RoadData[i+(Map_Max_X*4)] = 1;
							RoadData[i+1] = 1;
							}
						else
							{
							// West road only
							RoadData[i+(Map_Max_X*4)] = 1;
							RoadData[i+1+(Map_Max_X*4)] = 1;
							}
						}
					else if (GetRoad(x+1,y))
						{
						// Road to east catigory
						if (GetRoad(x,y+1))
							{
							// East/North road + others
							if (GetRoad(x,y-1))
								{
								// 3 way intersection
								RoadData[i+2] = 1;
								RoadData[i+2+(Map_Max_X*4)] = 1;
								RoadData[i+2+2*(Map_Max_X*4)] = 1;
								RoadData[i+2+3*(Map_Max_X*4)] = 1;
								RoadData[i+3+(Map_Max_X*4)] = 1;
								}
							else
								{
								RoadData[i+2] = 1;
								RoadData[i+3+(Map_Max_X*4)] = 1;
								}
							}
						else if (GetRoad(x,y-1))
							{
							// East/South road only
							RoadData[i+3+2*(Map_Max_X*4)] = 1;
							RoadData[i+2+3*(Map_Max_X*4)] = 1;
							}
						else
							{
							// East road only
							RoadData[i+2+(Map_Max_X*4)] = 1;
							RoadData[i+3+(Map_Max_X*4)] = 1;
							}
						}
					else if (GetRoad(x,y-1))
						{
						// Road to south catigory
						RoadData[i+2+2*(Map_Max_X*4)] = 1;
						RoadData[i+2+3*(Map_Max_X*4)] = 1;
						if (GetRoad(x,y+1))
							{
							// North/South road
							RoadData[i+2] = 1;
							RoadData[i+2+(Map_Max_X*4)] = 1;
							}
						}
					else if (GetRoad(x,y+1))
						{
						// Road to north catigory
						RoadData[i+2] = 1;
						RoadData[i+2+(Map_Max_X*4)] = 1;
						}
					}
				else if (ROADMAP_SIZE == 2)
					DoConnections(RoadData,i,GetRoad(x,y+1),GetRoad(x,y-1),GetRoad(x+1,y),GetRoad(x-1,y),1,0);
				}
			}
		}

	sprintf(fname,"%s-R",name);
	if ((fp = OpenCampFile (fname, "raw", "wb")) == NULL)
		return 0;
	fwrite(RoadData,sizeof(uchar),Map_Max_X*Map_Max_Y*ROADMAP_SIZE*ROADMAP_SIZE,fp);
	fclose(fp);
	printf("\n");
	return 1;
	}

// ===========================
// Main program
// ===========================

int main( int argc, char **argv )
	{
	int     i,x,y;
	char	*args;
	char	*baseDirectory, *baseFileName;

	if (argc >= 4)
		{
		for(i=1; i<argc; i++)
			{
			args = argv[i];
			switch ( args[1] )
				{
				case 'n':
					sprintf(TheaterName,args + 2);
					break;
				case 'w':
					Map_Max_X = atoi(args + 2);
					break;
				case 'h':
					Map_Max_Y = atoi(args + 2);
					break;
				case 'd':
					baseDirectory = args + 2;
					break;
				case 'f':
					baseFileName = args + 2;
					break;
				default:
					break;
				}
			}
		}
	else
		{
		printf("Theater Size (x,y): ");
		scanf("%d,%d",&x,&y);
		fflush(stdin);
		printf("Theater Name: ");
		gets(TheaterName);
		Map_Max_X = x;
		Map_Max_Y = y;
		}
	if (Map_Max_X < 10 || Map_Max_Y < 10)
		return -1;
	InitTheaterTerrain();
	ProcessTextureFile();
	ProcessReliefFile();
	SaveTheaterTerrain(TheaterName);
	SaveRoadData(TheaterName);
	BuildMapData(TheaterName);
	return 0;
	}

