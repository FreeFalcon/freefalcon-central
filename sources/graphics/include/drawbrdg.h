/***************************************************************************\
    DrawBrdg.h    Scott Randolph
    July 29, 1997

    Derived class to do special position and containment processing for
	bridges (platforms upon which ground vehicles can drive).
\***************************************************************************/
#ifndef _DRAWBRDG_H_
#define _DRAWBRDG_H_

#include "ObjList.h"
#include "DrawRdbd.h"
#include "DrawBldg.h"


class DrawableBridge : public DrawableObject {
public:
	DrawableBridge( float scale = 1.0f );
	virtual ~DrawableBridge();

	void AddSegment( DrawableRoadbed *piece );
	void ReplacePiece( DrawableRoadbed *oldPiece, DrawableRoadbed *newPiece );

	float GetGroundLevel( float x, float y, Tpoint *normal );

	virtual void Draw( class RenderOTW *renderer, int LOD );
	virtual void Draw( class Render3D *renderer );

protected:
	int						previousLOD;
	float					InclusionRadiusSquared;
	float					maxX, minX, maxY, minY;

	ObjectDisplayList		roadbedObjects;
	ObjectDisplayList		dynamicObjects;

	UpdateCallBack			updateCBstruct;
	SortCallBack			sortCBstruct;

protected:
	BOOL			ObjectInside( DrawableObject *object );
	virtual	void	SetParentList( ObjectDisplayList *list );

	static void UpdateMetrics( void *self, long listNo, const Tpoint *pos, TransportStr *transList );
	void		UpdateMetrics( long listNo, const Tpoint *pos, TransportStr *transList );

	static void SortForViewpoint( void *self );
	void		SortForViewpoint( void );

#ifdef USE_SH_POOLS
public:
      // Overload new/delete to use a SmartHeap fixed size pool
      void *operator new(size_t size) { ShiAssert( size == sizeof(DrawableBridge) ); return MemAllocFS(pool);	};
      void operator delete(void *mem) { if (mem) MemFreeFS(mem); };
      static void InitializeStorage()	{ pool = MemPoolInitFS( sizeof(DrawableBridge), 20, 0 ); };
      static void ReleaseStorage()	{ MemPoolFree( pool ); };
      static MEM_POOL	pool;
#endif
};

#endif // _DRAWBRDG_H_
