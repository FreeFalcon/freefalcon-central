/***************************************************************************\
    DrawPlat.cpp
    Scott Randolph
    April 10, 1997

    Derived class from DrawableBSP which handles large flat objects which
	can lie beneath other objects (ie: runways, carries, bridges).
\***************************************************************************/
#ifndef _DRAWPLAT_H_
#define _DRAWPLAT_H_

#include "ObjList.h"
#include "DrawObj.h"


class DrawablePlatform : public DrawableObject {
  public:
	DrawablePlatform( float scale = 1.0f );
	virtual ~DrawablePlatform();

	virtual void Draw( class RenderOTW *renderer, int LOD );
	virtual void Draw( class Render3D *renderer );

	void	InsertStaticSurface( class DrawableBuilding *object );
	void	InsertStaticObject( class DrawableObject *object );

  protected:
	float					InclusionRadiusSquared;
	float					maxX, minX, maxY, minY;

	ObjectDisplayList		flatStaticObjects;
	ObjectDisplayList		tallStaticObjects;
	ObjectDisplayList		dynamicObjects;

	UpdateCallBack			updateCBstruct;
	SortCallBack			sortCBstruct;

  protected:
	BOOL			ObjectInside( DrawableObject *object );
	virtual	void	SetParentList( ObjectDisplayList *list );

	static void UpdateMetrics( void *self, long listNo, const Tpoint *pos, TransportStr *transList );
	void	UpdateMetrics( long listNo, const Tpoint *pos, TransportStr *transList );

	static void SortForViewpoint( void *self );
	void	SortForViewpoint( void );

#ifdef USE_SH_POOLS
  public:
      // Overload new/delete to use a SmartHeap fixed size pool
      void *operator new(size_t size) { ShiAssert( size == sizeof(DrawablePlatform) ); return MemAllocFS(pool);	};
      void operator delete(void *mem) { if (mem) MemFreeFS(mem); };
      static void InitializeStorage()	{ pool = MemPoolInitFS( sizeof(DrawablePlatform), 10, 0 ); };
      static void ReleaseStorage()	{ MemPoolFree( pool ); };
      static MEM_POOL	pool;
#endif
};

#endif // _DRAWPLAT_H_
