#ifndef _CPMACHASI_H
#define _CPMACHASI_H

#include "cpobject.h"

#ifdef USE_SH_POOLS
extern MEM_POOL gCockMemPool;
#endif


class CPObject;

#define MACH_WINDOW_HEIGHT			16
#define HALF_MACH_WINDOW_HEIGHT	MACH_WINDOW_HEIGHT / 2

//===========================================================
// Structures used for Initialization
//===========================================================

typedef struct {
					float	min_dial_value;							
					float	max_dial_value;
					float	dial_start_angle;
					float	dial_arc_length;
					int	needle_radius;
					float end_radius;
					float end_angle;
					long	color0;
					long	color1;
} MachAsiInitStr;



//===========================================================
// CPMachAsi Class Definition
//===========================================================

class CPMachAsi : public CPObject {
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(gCockMemPool,size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
private:

	void			CalculateDeflection(void);
	void			CalculateNeedlePosition(void);

public:

	//===========================================================
	// These memebers are used to gather Airframe Generated Data
	//===========================================================

	float			mAirSpeed;				// (unit: Knots indicated airspeed)					Calculated by Airframe

	//===========================================================
	// Location of this instrument
	//===========================================================

	int			mxCenter;				// (unit: pixels from top of screen)				x Center of this instrument
	int			myCenter;				// (unit: pixels from left of screen)				y center of this instrument

	//===========================================================
	// Members needed to draw ASI Needle
	//===========================================================

	float			mMinimumDialValue;	//	(unit: GetKias / 10)										Minimum airspeed for this instrument
	float			mMaximumDialValue;	//	(unit: GetKias / 10)										Maximum airspeed for this instrument
	float			mDialStartAngle;		// (unit: radians)										Angle measured from the positive x axis that intersects the instrument to the angle that intersects the instrument's minimum airspeed.
	float			mDialArcLength;		// (unit: radians)										Arc measured from mDialStartAngle to the angle that intersects the instruments maximum airspeed.
	float			mDeflection;			// (unit: radians)										Deflection of ASI needle, Measured from mDialStartAngle
	float       mEndLength;          // (unit: percentage)                           Percentage of needle length that goes back behind center
	float       mEndAngle;           // (unit: radians)                              Angle offset for back end of needle. 0 is straight back, PI/2 is straight forward

	int			mNeedleRadius;			// (unit: pixels)											Radius measured from instrument center to outermost vertex of the needle triangle.
	int			mxNeedlePos1;			// (unit: pixels relative to left of screen)		x location of outermost needle vertex
	int			myNeedlePos1;			// (unit: pixels relative to top of screen)		y location of outermost needle vertex
	int			mxNeedlePos2;			// (unit: pixels relative to left of screen)		x location of rightmost needle vertex, as viewed along the needle
	int			myNeedlePos2;			// (unit: pixels relative to top of screen)		y location of rightmost needle vertex, as viewed along the needle
	int			mxNeedlePos3;			// (unit: pixels relative to left of screen) 	x location of leftmost needle vertex, as viewed along the needle	
	int			myNeedlePos3;			// (unit: pixels relative to top of screen) 		y location of leftmost needle vertex, as viewed along the needle

	long			mColor[2][2];			// [0][?] daytime, [1][?] nvg, [?][0] color0, [?][1] color1

	//===========================================================
	// Runtime Member Functions
	//===========================================================

	void				Exec(void) {};
	virtual void	Exec(SimBaseClass*);
	virtual void	DisplayDraw(void);
	virtual void	HandleEvent(void){};

	//===========================================================
	// Constructors and Destructors
	//===========================================================

	CPMachAsi(ObjectInitStr *, MachAsiInitStr *);
	virtual ~CPMachAsi();
};

#endif
