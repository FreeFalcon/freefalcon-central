#ifndef _DED_H
#define _DED_H

#ifndef _WINDOWS_
#include <windows.h>
#endif
#include "stdhdr.h"

#include "cpobject.h"

#ifdef USE_SH_POOLS
extern MEM_POOL gCockMemPool;
#endif


//====================================================//
// Structures used for Initialization
//====================================================//
enum DedType {
    DEDT_DED,
	DEDT_PFL,
};

typedef struct { // jpo - maybe this will grow
    long	color0; // color ded is displayed in


    DedType dedtype;
} DedInitStr;

class CPDed : public CPObject
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(gCockMemPool,size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif

	// DED in viewport coords and pixel coords
	//MI changed for ICP Stuff
	//RECT					mpLinePos[3];
	//RECT					mpLinePos[4];
	RECT					mpLinePos[5];

	float					mTop;
	float					mLeft;
	float					mBottom;
	float					mRight;
	long					mColor[2]; // night and day colours
	DedType					mDedType;

public:

	CPDed(ObjectInitStr *genericInit, DedInitStr *dedInit);
	virtual ~CPDed();
	void	Exec(SimBaseClass*);
	void DisplayDraw(void);
};

#endif
