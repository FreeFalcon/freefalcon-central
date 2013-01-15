#ifndef _CPDIAL_H
#define _CPDIAL_H

#include "cpobject.h"

#ifdef USE_SH_POOLS
extern MEM_POOL gCockMemPool;
#endif


//====================================================//
// Structures used for Initialization
//====================================================//

typedef struct {
					int	radius0;
					int	radius1;
					int	radius2;
					long	color0;
					long	color1;
					long	color2;
					int	endPoints;
					float	*pvalues;
					float	*ppoints;
					RECT	srcRect;					
					int IsRendered;			//Wombat778 3-26-04
					GLubyte *sourcedial;	//Wombat778 3-26-04

					} DialInitStr;

//====================================================//
// CPDial Class defintion
//====================================================//

class CPDial : public CPObject {
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(gCockMemPool,size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
public:

   virtual ~CPDial (void);
	float				mDialValue;
	//====================================================//
	// Dimensions and Locations
	//====================================================//

	int				mxCenter;
	int				myCenter;
	int				mRadius0;
	int				mRadius1;
	int				mRadius2;
	long				mColor[2][3];
//	long				mColor0;
//	long				mColor1;
//	long				mColor2;
	int				mEndPoints;
	float				*mpValues;
	float				*mpPoints;
	float				*mpCosPoints;
	float				*mpSinPoints;
	RECT				mSrcRect;
	int				mxPos0, mxPos1, mxPos2, mxPos3;
	int				myPos0, myPos1, myPos2, myPos3;
	float			angle;				//Wombat778 3-26-04


	//====================================================//
	// Pointers to Runtime Member Functions
	//====================================================//

	void				Exec(SimBaseClass*);
	void				DisplayDraw(void);

	
	//Wombat778 3-22-04 Stuff for rendered dials
	void			DisplayBlit3D();
	GLubyte			*mpSourceBuffer;
	virtual void CreateLit(void);	
	int				IsRendered;
	//Wombat778 End



	//====================================================//
	// Constructors and Destructors
	//====================================================//

	CPDial(ObjectInitStr*, DialInitStr*);
};

#endif
