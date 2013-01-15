/***************************************************************************\
    DrawPuff.h
    Scott Randolph
    February 26, 1997

    Subclass for drawing puffy clouds.
\***************************************************************************/
#ifndef _DRAWPUFF_H_
#define _DRAWPUFF_H_

#include "DrawBSP.h"

class DrawablePuff : public DrawableBSP {
#ifdef USE_SH_POOLS
  public:
      // Overload new/delete to use a SmartHeap fixed size pool
      void *operator new(size_t size) { ShiAssert( size == sizeof(DrawablePuff) ); return MemAllocFS(pool);	};
      void operator delete(void *mem) { if (mem) MemFreeFS(mem); };
      static void InitializeStorage()	{ pool = MemPoolInitFS( sizeof(DrawablePuff), 50, 0 ); };
      static void ReleaseStorage()	{ MemPoolFree( pool ); };
      static MEM_POOL	pool;
#endif
  public:
	DrawablePuff( int ID, int texSetNum, Tpoint *pos );
	virtual ~DrawablePuff()	{};

	void UpdateForDrift( float x, float y );
};

#endif // _DRAWPUFF_H_
