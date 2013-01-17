#ifndef _CPKNEEVIEW_H
#define _CPKNEEVIEW_H

//#include <windows.h>

#include "cpobject.h"
#include "Graphics/Include/image.h"

#ifdef USE_SH_POOLS
extern MEM_POOL gCockMemPool;
#endif




//====================================================//
// CPLight Class Definition
//====================================================//

class CPKneeView : public CPObject, Render2D {
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(gCockMemPool,size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
public:

	// kneeboards are shared among all kneeviews
	KneeBoard* mpKneeBoard;

	//====================================================//
	// Runtime Member Functions
	//====================================================//

	virtual void	Exec(SimBaseClass*);
	virtual void	DisplayBlit(void);
	virtual void	DisplayDraw(void);
	virtual void Refresh( SimVehicleClass *platform );

	virtual void Setup( DisplayDevice *device, int top, int left, int bottom, int right );
	virtual void Cleanup( void );

	//====================================================//
	// Constructors and Destructors
	//====================================================//

	CPKneeView(ObjectInitStr*, KneeBoard*);
	virtual ~CPKneeView();

	// sfr: moved some information from kneeboard to kneeview to allow
	// multiple kneeview in the same pit
private:
	RECT		srcRect;
	RECT		dstRect;

	// image buffer from kneeboard map
	ImageBuffer	*mapImageBuffer;
	// Real world map dimensions (R/O externally)
	float		wsVcenter;	//vertical center
	float		wsHcenter;	//horizontal center
	float		wsHsize;	//horizontal size
	float		wsVsize;	//vertical size
	int			pixelMag;
	float m_pixel2nmX, m_pixel2nmY;

	// Internal worker functions for the text page
	void DrawMissionText( Render2D *renderer, SimVehicleClass *platform );

	// Internal worker funtions for the map page
	void RenderMap( SimVehicleClass *platform );
	void UpdateMapDimensions( SimVehicleClass *platform );
	void DrawMap( );
	void DrawWaypoints( SimVehicleClass *platform );
	void DrawCurrentPosition( ImageBuffer *targetBuffer, Render2D *renderer, SimVehicleClass *platform );
	void MapWaypointToDisplay( WayPointClass *curWaypoint, float *x, float *y );
//	void ApplyLighting(DWORD *inColor, DWORD *outColor);
};

#endif

