#define MLR_NEWTRAILCODE

#ifdef MLR_NEWTRAILCODE
/***************************************************************************\
    DrawSgmt.h
    MLR
	12/10/2003

    Derived class to handle drawing segmented trails (like contrails).
\***************************************************************************/
#ifndef _DRAWSGMT_H_
#define _DRAWSGMT_H_

#include "DrawObj.h"
#include "RenderOW.h"

#include "context.h"
#include "Tex.h"
#include "falclib/include/alist.h"
#include "TimeMgr.h"


#ifdef USE_SH_POOLS
#include "SmartHeap\Include\smrtheap.h"
#endif

#include "context.h"

// Used to draw segmented trails (like missile trails)
//
//*************************************************************************************
//RV - I-Hawk - Changed some names of trails enum here, and all references in other files
//so now names correctly match data references 31/10/2007 
// #2 old TRAIL_AIM120 new TRAIL_SAM
// #3 old TRAIL_TRACER1 new TRAIL_AIM120
// #4 old TRAIL_TRACER2 new TRAIL_MAVERICK
// #6 old TRAIL_FIRE1 new TRAIL_MEDIUM_SAM
// #7 old TRAIL_EXPPIECE new TRAIL_LARGE_AGM
// #9 old TRAIL_DISTMISSILE new TRAIL_SARH_MISSILE
// #16 old TRAIL_MISLTRAIL new TRAIL_IR_MISSILE
// #21 old TRAIL_A10CANNON2 new TRAIL_ENGINE_SMOKE_LIGHT
// #38 old TRAIL_GROUNDVGUNSMOKE new TRAIL_B52H_ENGINE_SMOKE
// #39 New trail TRAIL_F4_ENGINE_SMOKE
// #40 New trail TRAIL_COLOR_0
// #41 New trail TRAIL_COLOR_1
// #42 New trail TRAIL_COLOR_2
// #43 New trail TRAIL_COLOR_3
// #8 old TRAIL_THINFIRE new TRAIL_GUN2
// #38 old TRAIL_DUST new TRAIL_GUN3
// #27 old TRAIL_MISSILEHIT_SMOKE new TRAIL_WING_COLOR

//Added 10 more wing trails, 5 for each side for later implemention of wingtip color trails 
//Added a VORTEX_LARGE trail for larger vortex for large fighters
//*************************************************************************************
enum TrailType {
    TRAIL_CONTRAIL, //0				White contrails
	TRAIL_VORTEX, //1				Not in use
	TRAIL_SAM, //2					Large SAMs trail
	TRAIL_AIM120, //3				ARH missiles weak trail      
	TRAIL_MAVERICK, //4				Maverick type missiles trail
	TRAIL_SMOKE, //5				Old trail, not in use
	TRAIL_MEDIUM_SAM, //6			For smaller SAMs  
	TRAIL_LARGE_AGM, //7			For large AGMs 
	TRAIL_GUN2, //8					Second type gun trail 
	TRAIL_SARH_MISSILE, //9			SARH missiles trail
	TRAIL_GUN3, //10				Third type gun trail
	TRAIL_GUN, //11					default gun trail
	TRAIL_LWING, //12				ACMI trail (still using old trail.txt trail)
	TRAIL_RWING, //13				ACMI trail (still using old trail.txt trail)
	TRAIL_ROCKET, //14				Rockets trail
	TRAIL_MISLSMOKE, //15			Old trail, not in use
	TRAIL_IR_MISSILE, //16			Small IR missiles trail (usually IR)
	TRAIL_DARKSMOKE, //17			Engien trail
	TRAIL_FIRE2, //18				Old trail, not in use	
	TRAIL_WINGTIPVTX, //19			Wingtip vortex trail
	TRAIL_A10CANNON1, //20			A-10 Avenger cannon trail
	TRAIL_ENGINE_SMOKE_LIGHT, //21  Engien trail   
	TRAIL_FEATURESMOKE, //22		Old trail, not in use
	TRAIL_BURNINGDEBRIS, // 23      Old trail, not in use
	TRAIL_FLARE, // 24				Flare trail (not used really, as it's called by PS script...)
	TRAIL_FLAREGLOW, // 25			Old trail, not in use
	TRAIL_CONTAIL_SHORT, // 26      Old trail, not in use
	TRAIL_WING_COLOR, // 27         Saved for wing color trail (if will be implemented later...)
	TRAIL_MISSILEHIT_FIRE, // 28    Old trail, not in use
	TRAIL_BURNING_SMOKE, // 29		AC damaged trail
	TRAIL_BURNING_SMOKE2, // 30     Old trail, not in use
	TRAIL_BURNING_FIRE,  // 31      Old trail, not in use
	TRAIL_ENGINE_SMOKE_SHARED, //32 Engien trail
	TRAIL_GROUND_EXP_SMOKE, //33    Old trail, not in use
	TRAIL_DUSTCLOUD, // 34			Stirred up dust when A/C close to ground
	TRAIL_MISTCLOUD, // 35			Stirred up mist when A/C close to water
	TRAIL_LINGERINGSMOKE, // 36		Old trail, not in use
	TRAIL_GROUNDVDUSTCLOUD, // 37	Old trail, not in use
	TRAIL_B52H_ENGINE_SMOKE, // 38	Engine trail to be used for all modern engine big birds 
	TRAIL_F4_ENGINE_SMOKE, // 39    F-4 engine trail 
	TRAIL_COLOR_0, // 40			Color trail (for CTRL-S trail) this one is default in PS as RED
	TRAIL_COLOR_1, //41				Color trail (for CTRL-S trail) this one is default in PS as GREEN	
	TRAIL_COLOR_2, //42				Color trail (for CTRL-S trail) this one is default in PS as BLUE
	TRAIL_COLOR_3, //43				Color trail (for CTRL-S trail) this one is default in PS as YELLOW
	TRAIL_RWING_COLOR_0, // 44		Right wing Color trail this one is default in PS as WHITE
	TRAIL_RWING_COLOR_1, //45		Right wing Color trail this one is default in PS as RED	
	TRAIL_RWING_COLOR_2, //46		Right wing Color trail this one is default in PS as GREEN
	TRAIL_RWING_COLOR_3, //47		Right wing Color trail this one is default in PS as BLUE
	TRAIL_RWING_COLOR_4, //48		Right wing Color trail this one is default in PS as YELLOW
	TRAIL_LWING_COLOR_0, // 49		Left wing Color trail this one is default in PS as WHITE
	TRAIL_LWING_COLOR_1, //50		Left wing Color trail this one is default in PS as RED	
	TRAIL_LWING_COLOR_2, //51		Left wing Color trail this one is default in PS as GREEN
	TRAIL_LWING_COLOR_3, //52		Left wing Color trail this one is default in PS as BLUE
	TRAIL_LWING_COLOR_4, //53		Left wing Color trail this one is default in PS as YELLOW
	TRAIL_VORTEX_LARGE, //54	    Large vortex trail for large fighters
	TRAIL_MAX = 150
};

class ChunkNode;

class DrawableTrail : public DrawableObject {
#ifdef USE_SH_POOLS
  public:
      // Overload new/delete to use a SmartHeap fixed size pool
      void *operator new(size_t size) { ShiAssert( size == sizeof(DrawableTrail) ); return MemAllocFS(pool);	};
      void operator delete(void *mem) { if (mem) MemFreeFS(mem); };
      static void InitializeStorage()	{ pool = MemPoolInitFS( sizeof(DrawableTrail), 40, 0 ); };
      static void ReleaseStorage()	{ MemPoolFree( pool ); };
      static MEM_POOL	pool;
#endif
  public:
	DrawableTrail( int trailType, float scale = 1.0f );
	virtual ~DrawableTrail();

	void	AddPointAtHead( Tpoint *p, DWORD now );
	int		RewindTrail( DWORD now );
	void	TrimTrail( int len );
	void	KeepStaleSegs( BOOL val ) { keepStaleSegs = val; };
	void    SetType(int trailType);


	virtual void Draw( class RenderOTW *renderer, int LOD );
	static void SetGreenMode(BOOL state);
	static void SetCloudColor(Tcolor *color);

	void    SetHeadVelocity(Tpoint *FPS);
	void    ReleaseToSfx(void);
	int IsTrailEmpty(void);
	
  protected:
	AList List;
	DrawableTrail *children;
	Texture *TrailTexture;
	static BOOL	greenMode;
	static Tcolor litCloudColor;

	int					type;
	BOOL				keepStaleSegs;	// for ACMI
  private:
	  void   DrawNode(class RenderOTW *renderer, int LOD, class TrailNode *);
	  void   DrawSegment(class RenderOTW *renderer, int LOD, class TrailNode *start, class TrailNode *end);
	  struct TrailTypeEntry *Type;
	  int	 Something;
	  class  DrawableTrail *Link;
	  Tpoint headFPS;
	  ThreeDVertex v0,v1,v2,v3; // so we can precompute the colors
  public:
	static void SetupTexturesOnDevice( DXContext *rc );
	static void ReleaseTexturesOnDevice( DXContext *rc );

};

#endif // _DRAWSGMT_H_

#else
/***************************************************************************\
    DrawSgmt.h
    Scott Randolph
    May 3, 1996

    Derived class to handle drawing segmented trails (like contrails).
\***************************************************************************/
#ifndef _DRAWSGMT_H_
#define _DRAWSGMT_H_

#include "DrawObj.h"
#include "Falclib\Include\IsBad.h"

#ifdef USE_SH_POOLS
#include "SmartHeap\Include\smrtheap.h"
#endif

#include "context.h"

// Used to draw segmented trails (like missile trails)
class TrailElement {
#ifdef USE_SH_POOLS
  public:
      // Overload new/delete to use a SmartHeap fixed size pool
      void *operator new(size_t size) { ShiAssert( size == sizeof(TrailElement) ); return MemAllocFS(pool);	};
      void operator delete(void *mem) { if (mem) MemFreeFS(mem); };
      static void InitializeStorage()	{ pool = MemPoolInitFS( sizeof(TrailElement), 100, 0 ); };
      static void ReleaseStorage()	{ MemPoolFree( pool ); };
      static MEM_POOL	pool;
#endif
  public:
	TrailElement( Tpoint *p, DWORD now )	{ point = *p; time = now; };
	~TrailElement()							
	{ 
		if (!F4IsBadWritePtr(next, sizeof(TrailElement))) // JB 010304 CTD
		{	delete next; next = NULL; }
	};

	Tpoint			point;		// World space location of this point on the trail
	DWORD			time;		// For head this is insertion time, for others its deltaT

	TrailElement	*next;
};

enum TrailType {
    TRAIL_CONTRAIL, //0
	TRAIL_VORTEX, //1
	TRAIL_SAM, //2
	TRAIL_AIM120, //3
	TRAIL_MAVERICK, //4
	TRAIL_SMOKE, //5
	TRAIL_MEDIUM_SAM, //6
	TRAIL_LARGE_AGM, //7
	TRAIL_GUN2, //8
	TRAIL_SARH_MISSILE, //9
	TRAIL_GUN3, //10
	TRAIL_GUN, //11
	TRAIL_LWING, //12
	TRAIL_RWING, //13
	TRAIL_ROCKET, //14
	TRAIL_MISLSMOKE, //15
	TRAIL_IR_MISSILE, //16
	TRAIL_DARKSMOKE, //17
	TRAIL_FIRE2, //18
	TRAIL_WINGTIPVTX, //19
	TRAIL_MAX
};

class DrawableTrail : public DrawableObject {
#ifdef USE_SH_POOLS
  public:
      // Overload new/delete to use a SmartHeap fixed size pool
      void *operator new(size_t size) { ShiAssert( size == sizeof(DrawableTrail) ); return MemAllocFS(pool);	};
      void operator delete(void *mem) { if (mem) MemFreeFS(mem); };
      static void InitializeStorage()	{ pool = MemPoolInitFS( sizeof(DrawableTrail), 40, 0 ); };
      static void ReleaseStorage()	{ MemPoolFree( pool ); };
      static MEM_POOL	pool;
#endif
  public:
	DrawableTrail( int trailType, float scale = 1.0f );
	virtual ~DrawableTrail();

	void	AddPointAtHead( Tpoint *p, DWORD now );
	int		RewindTrail( DWORD now );
	void	TrimTrail( int len );
	void	KeepStaleSegs( BOOL val ) { keepStaleSegs = val; };

	virtual void Draw( class RenderOTW *renderer, int LOD );

	TrailElement*	GetHead()	{ return head; };

  protected:
	int					type;
	TrailElement		*head;
	BOOL				keepStaleSegs;	// for ACMI

  protected:
	void ConstructSegmentEnd( RenderOTW *renderer, 
							  Tpoint *start, Tpoint *end, 
							  struct ThreeDVertex *xformLeft, ThreeDVertex *xformRight );

	// Handle time of day notifications
	static void TimeUpdateCallback( void *unused );

  public:
	static void SetupTexturesOnDevice( DXContext *rc );
	static void ReleaseTexturesOnDevice( DXContext *rc );
};

#endif // _DRAWSGMT_H_

#endif