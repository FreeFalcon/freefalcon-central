#ifndef _CPADI_H
#define _CPADI_H

#include "cpobject.h"
#include "navsystem.h"

#ifdef USE_SH_POOLS
extern MEM_POOL gCockMemPool;
#endif

#define NUM_AC_BAR_POINTS	7
#define NUM_ILS_BAR_POINTS 2
#define VERTICAL_SCALE		(2.0F * DTR)
#define HORIZONTAL_SCALE	(10.0F * DTR)

typedef struct {
	RECT backSrc;
	RECT backDest;
	BOOL doBackRect;
	RECT srcRect;
	RECT ilsLimits;
	BYTE* pBackground;
	long color0, color1, color2, color3, color4;
	BYTE *sourceadi;	//Wombat778 3-24-04
} ADIInitStr;


class CPObject;

class CPAdi : public CPObject {
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(gCockMemPool,size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
public:

	//====================================================//
	// Aircraft Parameters
	//====================================================//

	float		mPitch;
	float		mRoll;
	float		mMaxPitch;
	float		mMinPitch;
	float		mTanVisibleBallHalfAngle;

	long		mColor[2][5];
	//====================================================//
	// ADI Dimensions, BitTape Dimensions and Positioning Data
	//====================================================//

	int		mRadius;
	float		mSlide;
	RECT		mSrcRect;
	int		mSrcHalfHeight;
	RECT		mILSLimits;
	RECT		mBackSrc;
	RECT		mBackDest;
	BOOL		mDoBackRect;

	//====================================================//
	// Line Drawing Data, Aircraft Bar, Viewport
	//====================================================//
	
	float			mLeft;
   float			mRight;
   float			mTop;
	float			mBottom;

	unsigned		mpAircraftBar[NUM_AC_BAR_POINTS][2];
	float			mpAircraftBarData[NUM_AC_BAR_POINTS][2]; //x, y
	
	int      		mTopLimit;
	int      		mLeftLimit;
	int      		mBottomLimit;
	int      		mRightLimit;

	float			mHorizScale;
	float			mVertScale;

	int      		mHorizCenter;
	int      		mVertCenter;

	int      		mHorizBarPos;
	int      		mVertBarPos;

	//MI
	int Persistant;
	float LastMainADIPitch;
	float LastMainADIRoll;
	float LastBUPPitch;
	float LastBUPRoll;

	//====================================================//
	// Pointer to Rotated Blit Data
	//====================================================//

	int*				mpADICircle;
	GLubyte*			mpSourceBuffer;
	ImageBuffer*	mpSurfaceBuffer;
	ImageBuffer*	ADIBuffer;					//Wombat778 10-06-2003		temporary buffer for ADI for use when scaling


	//====================================================//
	// Runtime Member Functions
	//====================================================//

	virtual void Exec(SimBaseClass*);
	void			 ExecILS(void);
	void			 ExecILSNone(void);
	virtual void DisplayBlit(void);
	virtual void DisplayDraw(void);
	virtual void CreateLit(void);
	virtual void DiscardLit(void);
	virtual void Translate(WORD*);
	virtual void Translate(DWORD*);

	// OW
	virtual void DisplayBlit3D();	

	//====================================================//
	// Constructors and Destructors
	//====================================================//

	CPAdi(ObjectInitStr *, ADIInitStr *);
	virtual ~CPAdi();
};

#endif
