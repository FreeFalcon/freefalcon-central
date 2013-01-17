/***************************************************************************\
    DrawRdbd.h    Scott Randolph
    July 29, 1997

    Derived class to do special position and containment processing for
	sections of bridges.
\***************************************************************************/
#ifndef _DRAWRDBD_H_
#define _DRAWRDBD_H_

#include "Edge.h"
#include "DrawBldg.h"


class DrawableRoadbed : public DrawableBuilding {
  public:
	DrawableRoadbed( int IDbase, int IDtop, Tpoint *pos, float heading, float height, float angle, float s = 1.0f );
	virtual ~DrawableRoadbed();

	virtual void Draw( class RenderOTW *renderer, int LOD );
	void         DrawSuperstructure( class RenderOTW *renderer, int LOD );
	void         DrawSuperstructure( class Render3D *renderer );

	BOOL OnRoadbed( Tpoint *pos, Tpoint *normal );

	// This one is for internal use only.  Don't use it or you'll break things...
	void ForceZ( float z )		{ position.z = z; if (superStructure) superStructure->ForceZ(z); };

  protected:
	DrawableBSP		*superStructure;

	float			start;
	float			length;

	float			cosInvYaw;
	float			sinInvYaw;

	float			tanRampAngle;
	Edge			ramp;

#ifdef USE_SH_POOLS
  public:
	// Overload new/delete to use a SmartHeap fixed size pool
	void *operator new(size_t size) { ShiAssert( size == sizeof(DrawableRoadbed) ); return MemAllocFS(pool);	};
	void operator delete(void *mem) { if (mem) MemFreeFS(mem); };
	static void InitializeStorage()	{ pool = MemPoolInitFS( sizeof(DrawableRoadbed), 10, 0 ); };
	static void ReleaseStorage()	{ MemPoolFree( pool ); };
	static MEM_POOL	pool;
#endif
};

#endif // _DRAWRDBD_H_
