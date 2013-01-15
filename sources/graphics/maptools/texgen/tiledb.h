/*******************************************************************************\
	TileDB.h
	Provides an interface to the tile database written by the tile tool.  At 
	present this is only used by the TexGen tool.

	Scott Randolph
	Spectrum HoloByte
	October 8, 1996
\*******************************************************************************/
#ifndef _TILEDB_H_
#define	_TILEDB_H_
#include "windows.h"


typedef struct AreaRecord {
	int		type;
	float	size;
	float	x;
	float	y;
} AreaRecord;

typedef struct PathRecord {
	int		type;
	float	size;
	float	x1, y1;
	float	x2, y2;
} PathRecord;


#pragma pack( push, 1 )

typedef struct PointRecord {
	float		y;			// Matches file format on disk                           
	float		x;			// (size = 8)
} PointRecord;


typedef struct FeatureRecord {
	char		tag[4];		// 
	short		featureID;	//                         
	BYTE		type;		// Matches file format on disk 
	float		size;		// (size = 13);
	short		numPoints;	//                       

	PointRecord	*points;
} FeatureRecord;


typedef struct colorRecord {
	BYTE		alpha;		// 
	BYTE		red;		// Matches file format on disk
	BYTE		green;		// (size = 4)
	BYTE		blue;		//
} colorRecord;


typedef struct elevationRecord {
	short		value;		// Matches file format on disk
	BYTE		absolute;	// (size = 3)
} elevationRecord;


typedef struct TileRecord {
	char			tag[4];			// 
	short			tileID;			//
	char			basename[8];	// Matches file format on disk
	BYTE			type;			// (size = 17)
	short			numFeatures;	// 

	colorRecord		*colors;
	elevationRecord *elevations;
	FeatureRecord	*features;
	FeatureRecord	**sortedFeatures;
	int				nareas;
	AreaRecord		*areas;
	int				npaths;
	PathRecord		*paths;
	WORD			code;
} TileRecord;


typedef struct HeaderRecord {
	char		title[15];		//                            
	char		version[8];		//                            
	short		gridXsize;		// Matches file format on disk 
	short		gridYsize;		// (size = 29)
	short		numTiles;		//
	
	TileRecord	*tiles;
} HeaderRecord;

#pragma pack( pop )


class TileDatabase {
  public:
	TileDatabase()	{};
	~TileDatabase()	{};

	void Load( char *filename );
	void Free( void );

	TileRecord *GetTileRecord( WORD code );

	BYTE GetTerrainType( TileRecord *pTile )	{ if (pTile) return pTile->type;   else return 0; };
	int  GetNAreas( TileRecord *pTile )			{ if (pTile) return pTile->nareas; else return 0; };
	int  GetNPaths( TileRecord *pTile )			{ if (pTile) return pTile->npaths; else return 0; };

	AreaRecord *GetArea( TileRecord *pTile, int area );
	PathRecord *GetPath( TileRecord *pTile, int path );

  protected:
	void ReadTile( HANDLE inputFile, TileRecord *tile );
	void ReadFeature( HANDLE inputFile, FeatureRecord *feature );
	void SortArray( FeatureRecord **array, int numElements );
	BOOL typeIsPath( BYTE featureType );


	HeaderRecord	header;
};




#endif // _TILEDB_H_

