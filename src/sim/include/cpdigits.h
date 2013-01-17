#ifndef _CPDIGITS_H
#define _CPDIGITS_H

#include "cpobject.h"

#ifdef USE_SH_POOLS
extern MEM_POOL gCockMemPool;
#endif


class CPObject;
//====================================================//
// Structures used for Initialization
//====================================================//

//Wombat778 3-22-04 Added type to hold textures for rendered digits.
typedef struct {
	BYTE *digit;
	std::vector<TextureHandle *> m_arrTex;
	int mHeight;
	int mWidth;
} SourceDigitType;

typedef struct {
					RECT*	psrcRects;
					int	numDestDigits;
					RECT*	pdestRects;
					SourceDigitType *sourcedigits;	//Wombat778 3-22-04
} DigitsInitStr;

//====================================================//
// CPObject Class defintion
//====================================================//

class CPDigits : public CPObject {
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(gCockMemPool,size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
public:

	long				mValue;
	int				*mpValues;
	//====================================================//
	// Dimensions and Locations
	//====================================================//

	RECT				*mpDestRects;
	RECT				*mpSrcRects;
	int				mDestDigits;
	char				*mpDestString;

	//====================================================//
	// Pointers to Runtime Member Functions
	//====================================================//

	void				Exec(void) {};
	void				Exec(SimBaseClass*);
	void				DisplayBlit(void);
	void				DisplayBlit3D();			//Wombat778 3-22-04
	void				DisplayDraw(void){};
	void				SetDigitValues(long);
	//MI
	bool				active;

	//Wombat778 3-22-04 Stuff for rendered digits
	SourceDigitType		*mpSourceBuffer;
	virtual void CreateLit(void);
	virtual void DiscardLit(void);
	//Wombat778 End

	//====================================================//
	// Constructors and Destructors
	//====================================================//

	CPDigits(ObjectInitStr*, DigitsInitStr*);
	virtual ~CPDigits();
};

#endif
