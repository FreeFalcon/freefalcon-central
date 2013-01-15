/***************************************************************************\
    DrawPNT.h

	Derived calss from DrawableObject which will just draw a pixel at its
	given location and will draw a label for it (if turned on).  In general,
	this can be used to draw an object that's very far away.
\***************************************************************************/
#ifndef _DRAWPNT_H_
#define _DRAWPNT_H_

#include "DrawObj.h"

#ifdef USE_SH_POOLS
#include "SmartHeap\Include\smrtheap.h"
#endif


class DrawablePoint : public DrawableObject {
#ifdef USE_SH_POOLS
  public:
      // Overload new/delete to use a SmartHeap fixed size pool
      void *operator new(size_t size) { ShiAssert( size == sizeof(DrawablePoint) ); return MemAllocFS(pool);	};
      void operator delete(void *mem) { if (mem) MemFreeFS(mem); };
      static void InitializeStorage()	{ pool = MemPoolInitFS( sizeof(DrawablePoint), 50, 0 ); };
      static void ReleaseStorage()	{ MemPoolFree( pool ); };
      static MEM_POOL	pool;
#endif
  public:
	DrawablePoint( DWORD color, BOOL grnd, const Tpoint *pos, float scale = 1.0f );
	virtual ~DrawablePoint();

	void Update( const Tpoint *pos );
	void SetLabel( char *labelString, DWORD color );
	char *Label()					{ return label; };
	DWORD LabelColor()				{ return labelColor; };
	DWORD PointColor()				{ return pointColor; };
	void SetOnGround( BOOL grnd )	{ onGround = grnd; } ;
	void SetPointColor( DWORD c )	{ pointColor = c; } ;

	virtual void Draw( class RenderOTW *renderer, int LOD );

  public:
	static BOOL			drawLabels;		// Shared by ALL drawable points (now just labels, of course)

  protected:
	char				label[32];
	int					labelLen;
	DWORD				labelColor;
	DWORD				pointColor;
	int					previousLOD;
	BOOL				onGround;
};

#endif // _DRAWPNT_H_
