#ifndef _VDIAL_H
#define _VDIAL_H

#include "cpcb.h"
#include "simbase.h"

#ifdef USE_SH_POOLS
extern MEM_POOL gCockMemPool;
#endif

class RenderOTW;
class Canvas3D;

//====================================================//
// Structures used for Initialization
//====================================================//

typedef struct {
					int				callback;
					Tpoint			*pUL;
					Tpoint			*pUR;
					Tpoint			*pLL;
					RenderOTW		*pRender;
					float				radius;
					long				color;
					int				endPoints;
					float				*pvalues;
					float				*ppoints;
} VDialInitStr;

//====================================================//
// CPDial Class defintion
//====================================================//

class VDial {

#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(gCockMemPool,size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif

public:

	float				mRadius;
	long				mColor;
	long				mNVGColor;
	float				mDialValue;

	Canvas3D*		mpCanvas;
	RenderOTW*		renderer;
		
	int				mCallback;
	CPCallback		mExecCallback;

	SimBaseClass*	mpOwnship;

	//====================================================//
	// Dimensions and Locations
	//====================================================//

	int				mEndPoints;
	float				*mpValues;
	float				*mpPoints;
	float				*mpCosPoints;
	float				*mpSinPoints;

	void				Exec(SimBaseClass*);

	//====================================================//
	// Constructors and Destructors
	//====================================================//

	VDial(VDialInitStr*);
  	~VDial (void);
};

#endif
