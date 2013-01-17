/***************************************************************************\
    Tviewpnt.h
    Scott Randolph
    August 21, 1995

    This class represents a single viewer of the terrain database.  There
    may be multiple viewers simultainiously, each represented by their
    own instance of this class.
\***************************************************************************/
#ifndef _TVIEWPNT_H_
#define _TVIEWPNT_H_

#include "Matrix.h"
#include "Ttypes.h"
#include "TBlkList.h"


typedef struct TpathFeature {
	float x[2];
	float y[2];
	float width;
} TpathFeature;

typedef struct TareaFeature {
	float x, y;
	float radius;
} TareaFeature;


class TViewPoint {
  public:
	TViewPoint()	{ nLists = 0; };
	~TViewPoint()	{ if (nLists != 0)  Cleanup(); };

	void Setup( int minimumLOD, int maximumLOD, float *fetchRanges );
	virtual void Cleanup( void );

  	BOOL IsReady(void)		{ return (nLists != 0 ); };

	// Move the viewer and swap blocks as needed
  	void	Update(const Tpoint *position);

	// Return the Nth feature of the required type from the tile containing the viewpoint
	int		GetTileID( int r, int c );
	BOOL	GetPath( int TileID, int type, int offset, TpathFeature *target );
	BOOL	GetArea( int TileID, int type, int offset, TareaFeature *target );

	// Return the min and max LODs ever useable by this viewpoint
	int		GetMinLOD(void)		{ return minLOD; };
	int		GetMaxLOD(void)		{ return maxLOD; };

	// Return the highest and lowest terrain elevation within range of this viewpoint
	void	GetAreaFloorAndCeiling( float *floor, float *ceiling );

	// Return the low and high detail levels to be used for drawing	the next frame
	int		GetHighLOD(void)	{ return highDetail; };
	int		GetLowLOD(void)		{ return lowDetail; };

	// Return the largest distance from the viewer a post will ever want to be drawn
	// in world space and level post space
	float	GetMaxRange( void )			{ return maxRange[maxLOD]; };
	float	GetMaxRange( int LOD )		{ return maxRange[LOD]; };
	int		GetMaxPostRange( int LOD )	{ return blockLists[LOD].GetMaxPostRange(); };

	// Return the maximum distance from the current postion at which all
	// data is owned by the specified LOD list in world space and in level posts
	float	GetAvailableRange( void )			{ return LEVEL_POST_TO_WORLD( blockLists[maxLOD].GetAvailablePostRange(), maxLOD ); };
	float	GetAvailableRange( int LOD )		{ return LEVEL_POST_TO_WORLD( blockLists[LOD].GetAvailablePostRange(), LOD ); };
	int		GetAvailablePostRange( int LOD )	{ return blockLists[LOD].GetAvailablePostRange(); };

	// Return the distance to the farthest piece of terrain that will be drawn in the next frame
	// (The .65 factor is a magic number that seems to work to account for the fact that
	//  the terrain engine leaves a safty margin of undrawn posts arround the viewpoint).
	float	GetDrawingRange( void )		{ return LEVEL_POST_TO_WORLD( blockLists[lowDetail].GetAvailablePostRange(), lowDetail ) * 0.65f; };

	// Return a pointer to the requested post in the given level.
	// The caller of this function must ensure that the post is within
	// the available range.  Also, the post pointer may become invalid
	// after a call to "Update"
	Tpost* GetPost( int levelPostRow, int levelPostCol, int LOD ) { 
		return blockLists[LOD].GetPost( levelPostRow, levelPostCol );
	};

	// Return the type of ground under the specified point on the ground
	// (requires terrain data including textures to be loaded at that point,
	//  otherwise, 0 is returned.)
	int		GetGroundType( float x, float y );

	// Return the z value of the terrain at the specified point.  (positive Z down)
	// If the third argument is provided to the exact version, then the normal
	// will also be returned
	float	GetGroundLevelApproximation( float x, float y );
	// sfr: new prototype with LOD level
#define USE_GET_LOD_LEVEL 1
#if USE_GET_LOD_LEVEL
	/** returns the ground level at the highest possible LOD for the given spot. 
	* normal and the LOD level are also returned if not NULL.
	*/
	float	GetGroundLevel( float x, float y, Tpoint *normal=NULL, int *lod = NULL );
#else
	float	GetGroundLevel( float x, float y, Tpoint *normal=NULL);
#endif
	
	// Return TRUE if the given point is on or under the terrain
	BOOL	UnderGround( Tpoint *position );

	// Return TRUE if the two specified points can see each other over the terrain
	BOOL	LineOfSight( Tpoint *p1, Tpoint *p2 );

	// Find the intersection with the terrain (return FALSE if there isn't one)
	BOOL	GroundIntersection( Tpoint *dir, Tpoint *intersection );


	// Get the position and orientation matrix for this viewpoint
	float	X( void )	{ return pos.x; };
	float	Y( void )	{ return pos.y; };
	float	Z( void )	{ return pos.z; };

	void	GetPos( Tpoint *p )		{ *p = pos; };

	float Speed; // JB 010608 for weather effects

private:
#if USE_GET_LOD_LEVEL
	/** sfr: gets the LOD available for retrieving the point. Returns maxLOD+1 if none. */
	int GetLODLevel(float x, float y) const;
#endif

	// Line of Sight helpers
	BOOL SingleLODLineOfSight( int Px, int Py, int Qx, int Qy, float z, float dz, int LOD );
	BOOL TestVertex(int row, int col, float z, int LOD);
	BOOL TestEast(  int row, int col, float z, int LOD);
	BOOL TestNorth( int row, int col, float z, int LOD);
	BOOL TestWest(  int row, int col, float z, int LOD);
	BOOL TestSouth( int row, int col, float z, int LOD);

	// Ground Intersection helpers
	BOOL horizontalEdgeTest( int row, int col, float x, float y, float z, int LOD );
	BOOL verticalEdgeTest(   int row, int col, float x, float y, float z, int LOD );
	void LineSquareIntersection( int row, int col, Tpoint *dir, Tpoint *intersection, int LOD );


protected:
	// Last reported world space location of the viewer (X north, Y east, Z down)
	Tpoint		pos;

	// Range sorted lists of pointers to data blocks at each map level
	// (level 0 is highest level of detail)
	int					nLists;
	TBlockList			*blockLists;
	mutable CRITICAL_SECTION cs_update;


	// The farest into the distance this viewer should ever see (in world space)
	float	*maxRange;
	
	// The lowest and highest detail levels ever to be used by this viewpoint
	int		minLOD;
	int		maxLOD;

	// The lowest and highest detail levels currently turned on for drawing
	int		highDetail;		// 0 <= highDetail <= lowDetail
	int		lowDetail;		// higheDetail <= lowDetail <= nLevels-1;
};


#endif // _TVIEWPNT_H_
