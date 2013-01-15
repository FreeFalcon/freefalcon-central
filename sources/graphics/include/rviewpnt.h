/***************************************************************************\
    RViewPnt.h
    Scott Randolph
    August 20, 1996

    Manages information about a world space location and keeps the weather,
	terrain, and object lists in synch.
\***************************************************************************/
#ifndef _RVIEWPNT_H_
#define _RVIEWPNT_H_


#include "TViewPnt.h"
#include "ObjList.h"
#include "Tex.h"

typedef struct ObjectListRecord {
	ObjectDisplayList	displayList;
	float				Ztop;
} ObjectListRecord;

class RViewPoint : public TViewPoint {
  public:
	RViewPoint()	{ objectLists = NULL; nObjectLists = 0; weather = NULL; bZBuffering = FALSE; }; //JAM 10Nov03
	~RViewPoint()	{ ShiAssert( !IsReady() ); };

	void	Setup(float gndRange,int maxDetail,int minDetail,bool isZBuffer);
	void	Cleanup( void );

	void	SetGroundRange( float gndRange, int maxDetail, int minDetail );

	BOOL	IsReady( void )	{ return (objectLists != NULL) && (TViewPoint::IsReady()); };

	// Add/remove drawable objects from this viewpoint's display list
	void	InsertObject( DrawableObject *object );
	void	RemoveObject( DrawableObject *object );

	// Query terrain and weather properties for the area arround this viewpoint
	float	GetTerrainFloor( void )		{ return terrainFloor; };
	float	GetTerrainCeiling( void )	{ return terrainCeiling; };
	float	GetCloudBase( void )		{ return cloudBase; };
	float	GetCloudTops( void )		{ return cloudTops; };
//	float	GetWeatherRange( void )		{ return weather ? weather->GetMaxRange() : 0.0f; };
//	float	GetRainFactor(void)		{ return weather ? weather->GetRainFactor() : 0.0f; };
//	float	GetVisibility(void)		{ return weather ? weather->GetVisibility() : 1.0f; };
//	bool	GetLightning(void)		{ return weather ? weather->HasLightning() : false; };
//	float	GetLocalCloudTops(void)		{ return weather ? weather->GetLocalCloudTops() : 0.0f; };

	// Cloud properties at this viewpoint's exact position in space
	float	CloudOpacity( void )		{ return cloudOpacity; };
	Tcolor	CloudColor( void )			{ return cloudColor; };

	// Ask if a line of sight exists between two points with respect to both clouds and terrain
	float	CompositLineOfSight( Tpoint *p1, Tpoint *p2 );
	int		CloudLineOfSight( Tpoint *p1, Tpoint *p2 );

	void	Update(const Tpoint *pos);
	void	UpdateMoon();

	void	ResetObjectTraversal( void );
	int		GetContainingList( float zValue );

	ObjectDisplayList*	ObjectsInTerrain( void )	{ return &objectLists[0].displayList; };
	ObjectDisplayList*	ObjectsBelowClouds( void )	{ return &objectLists[1].displayList; };
	ObjectDisplayList*	Clouds( void )				{ return &cloudList; };
	ObjectDisplayList*	ObjectsInClouds( void )		{ return &objectLists[2].displayList; };
	ObjectDisplayList*	ObjectsAboveClouds( void )	{ return &objectLists[3].displayList; };
	ObjectDisplayList*	ObjectsAboveRoof( void )	{ return &objectLists[4].displayList; };

	Texture		SunTexture, GreenSunTexture;
	Texture		MoonTexture, GreenMoonTexture, OriginalMoonTexture;

  protected:
	int			lastDay;				// Used to decide when the moon needs updating

	class LocalWeather	*weather;

	int					nObjectLists;
	ObjectListRecord	*objectLists;
	ObjectDisplayList	cloudList;
	float				cloudOpacity;  // 0.0 for no effect, 1.0 when inCloud is TRUE
	Tcolor				cloudColor;

	bool	bZBuffering;			//JAM 13Dec03

	float	terrainFloor;
	float	terrainCeiling;
	float	cloudBase;
	float	cloudTops;
	float	roofHeight;


  protected:
	void		SetupTextures(void);
	void		ReleaseTextures(void);
	static void	TimeUpdateCallback( void *self );
};

#endif // _RVIEWPNT_H_