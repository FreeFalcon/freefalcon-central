/***************************************************************************\
    DrawGrnd.h    Scott Randolph
    July 10, 1996

    Derived class to do special position processing for vehicals on the
	ground.  (More precisly, any object which is to be placed on the 
	ground and reoriented so that it's "up" vector is aligned with the
	terrain normal.)
\***************************************************************************/
#ifndef _DRAWGRND_H_
#define _DRAWGRND_H_

#include "DrawBSP.h"
#include "DrawBrdg.h"


class DrawableGroundVehicle : public DrawableBSP {
#ifdef USE_SH_POOLS
  public:
      // Overload new/delete to use a SmartHeap fixed size pool
      void *operator new(size_t size) { ShiAssert( size == sizeof(DrawableGroundVehicle) ); return MemAllocFS(pool);	};
      void operator delete(void *mem) { if (mem) MemFreeFS(mem); };
      static void InitializeStorage()	{ pool = MemPoolInitFS( sizeof(DrawableGroundVehicle), 50, 0 ); };
      static void ReleaseStorage()	{ MemPoolFree( pool ); };
      static MEM_POOL	pool;
#endif

  public:
	DrawableGroundVehicle( int type, Tpoint *pos, float heading, float scale = 1.0f );
	virtual ~DrawableGroundVehicle()	{};

	virtual	void	SetParentList( ObjectDisplayList *list );
	void			Update( Tpoint *pos, float heading );
	void			SetUpon( DrawableBridge *bridge )		{ drivingOn = bridge; previousLOD = -1; };

	virtual void Draw( class RenderOTW *renderer, int LOD );

	float	GetHeading( void )	{ return yaw; };

  protected:
	int				previousLOD;
	float			yaw;
	float			cosYaw;
	float			sinYaw;
	DrawableBridge	*drivingOn;
};

#endif // _DRAWGRND_H_
