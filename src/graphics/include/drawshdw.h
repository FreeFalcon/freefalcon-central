/***************************************************************************\
    DrawShdw.h    Scott Randolph
    July 2, 1997

    Derived class to draw BSP'ed objects with shadows (specificly for aircraft)
\***************************************************************************/
#ifndef _DRAWSHDW_H_
#define _DRAWSHDW_H_

#include "DrawBSP.h"


class DrawableShadowed : public DrawableBSP {
#ifdef USE_SH_POOLS
  public:
      // Overload new/delete to use a SmartHeap fixed size pool
      void *operator new(size_t size)	{ ShiAssert( size == sizeof(DrawableShadowed) ); return MemAllocFS(pool);	};
      void operator delete(void *mem)	{ if (mem) MemFreeFS(mem); };
      static void InitializeStorage()	{ pool = MemPoolInitFS( sizeof(DrawableShadowed), 50, 0 ); };
      static void ReleaseStorage()		{ MemPoolFree( pool ); };
      static MEM_POOL	pool;
#endif
  public:
	DrawableShadowed( int ID, const Tpoint *pos, const Trotation *rot, float s, int ShadowID );
	virtual ~DrawableShadowed()			{};

	virtual void Draw( class RenderOTW *renderer, int LOD );

  protected:
	ObjectInstance		shadowInstance;
};

#endif // _DRAWSHDW_H_
