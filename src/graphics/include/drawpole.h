/***************************************************************************\
    DrawPole.h    Ed Goldman

	Based on DrawableShadowed, this class is used in ACMI to put altitude
	poles and extra labels on an object.
\***************************************************************************/
#ifndef _DRAWPOLE_H_
#define _DRAWPOLE_H_

#include "DrawBSP.h"


class DrawablePoled : public DrawableBSP {
#ifdef USE_SH_POOLS
  public:
      // Overload new/delete to use a SmartHeap fixed size pool
      void *operator new(size_t size)	{ ShiAssert( size == sizeof(DrawablePoled) ); return MemAllocFS(pool);	};
      void operator delete(void *mem)	{ if (mem) MemFreeFS(mem); };
      static void InitializeStorage()	{ pool = MemPoolInitFS( sizeof(DrawablePoled), 50, 0 ); };
      static void ReleaseStorage()		{ MemPoolFree( pool ); };
      static MEM_POOL	pool;
#endif
  public:
	DrawablePoled( int ID, const Tpoint *pos, const Trotation *rot, float s );
	virtual ~DrawablePoled()			{};

	virtual void Draw( class RenderOTW *renderer, int LOD );

	void SetDataLabel( DWORD type, char *labelString );
	char *DataLabel( DWORD type );
	#define		DP_LABEL_HEADING				0
	#define		DP_LABEL_ALT					1
	#define		DP_LABEL_SPEED					2
	#define		DP_LABEL_TURNRATE				3
	#define		DP_LABEL_TURNRADIUS				4
	#define		DP_LABEL_LOCK_RANGE				5

	void SetTarget( BOOL val )
	{
		isTarget = val;
	};

	void SetTargetBoxColor( DWORD val )
	{
		boxColor = val;
	};


	static BOOL			drawHeading;		// Shared by ALL drawable BSPs
	static BOOL			drawAlt;			// Shared by ALL drawable BSPs
	static BOOL			drawSpeed;			// Shared by ALL drawable BSPs
	static BOOL			drawTurnRadius;		// Shared by ALL drawable BSPs
	static BOOL			drawTurnRate;		// Shared by ALL drawable BSPs
	static BOOL			drawPole;			// Shared by ALL drawable BSPs
	static BOOL			drawlockrange;			// Shared by ALL drawable BSPs

  protected:

	void DrawTargetBox( class RenderOTW *renderer, ThreeDVertex *spos );

	char				heading[32];
	int					headingLen;
	char				alt[32];
	int					altLen;
	char				speed[32];
	int					speedLen;
	char				turnRadius[32];
	int					turnRadiusLen;
	char				turnRate[32];
	int					turnRateLen;
	char				lockrange[32];
	int					lockrangeLen;

	BOOL				isTarget;
	DWORD				boxColor;
};

#endif // _DRAWPOLE_H_
