/***************************************************************************\
    DrawGuys.h    Scott Randolph
    April 6, 1998

    Derived class to do special position processing for foot soldiers on the
	ground.
\***************************************************************************/
#ifndef _DRAWGUYS_H_
#define _DRAWGUYS_H_

#include "DrawGrnd.h"


class DrawableGuys : public DrawableGroundVehicle {
  public:
	DrawableGuys( int type, Tpoint *pos, float heading, int numGuys, float scale = 1.0f );
	virtual ~DrawableGuys()	{};

	virtual void Draw( class RenderOTW *renderer, int LOD );

	void	SetSquadMoving( BOOL state )	{ moving = state; };
	void	SetNumInSquad( int n )			{ numInSquad = n; };
	int		GetNumInSquad( void )			{ return numInSquad; };

  protected:
    BOOL			moving;
	int				numInSquad;

#ifdef USE_SH_POOLS
  public:
	// Overload new/delete to use a SmartHeap fixed size pool
	void *operator new(size_t size) { ShiAssert( size == sizeof(DrawableGuys) ); return MemAllocFS(pool);	};
	void operator delete(void *mem) { if (mem) MemFreeFS(mem); };
	static void InitializeStorage()	{ pool = MemPoolInitFS( sizeof(DrawableGuys), 50, 0 ); };
	static void ReleaseStorage()	{ MemPoolFree( pool ); };
	static MEM_POOL	pool;
#endif
};

#endif // _DRAWGUYS_H_
