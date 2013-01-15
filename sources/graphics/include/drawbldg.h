/***************************************************************************\
    DrawBldg.h    Scott Randolph
    July 10, 1996

    Derived class to do special position processing for buildings on the
	ground.  (More precisly, any object which is to be placed on the 
	ground but not reoriented.)
\***************************************************************************/
#ifndef _DRAWBLDG_H_
#define _DRAWBLDG_H_

#include "DrawBSP.h"


class DrawableBuilding : public DrawableBSP {
  public:
	DrawableBuilding( int type, Tpoint *pos, float heading, float scale = 1.0f );
	virtual ~DrawableBuilding()	{};

	virtual void Draw( class RenderOTW *renderer, int LOD );

  protected:
	int		previousLOD;

#ifdef USE_SH_POOLS
  public:
	// Overload new/delete to use a SmartHeap fixed size pool
	void *operator new(size_t size) { ShiAssert( size == sizeof(DrawableBuilding) ); return MemAllocFS(pool);	};
	void operator delete(void *mem) { if (mem) MemFreeFS(mem); };
	static void InitializeStorage()	{ pool = MemPoolInitFS( sizeof(DrawableBuilding), 50, 0 ); };
	static void ReleaseStorage()	{ MemPoolFree( pool ); };
	static MEM_POOL	pool;
#endif
};

#endif // _DRAWBLDG_H_
