#ifndef _CPCURSOR_H
#define _CPCURSOR_H

#ifdef USE_SH_POOLS
extern MEM_POOL gCockMemPool;
#endif


//====================================================//
// Initialization Structures
//====================================================//

typedef struct {
					int				idNum;
					RECT				srcRect;
					int				xhotSpot;
					int				yhotSpot;
					ImageBuffer*	pOtwImage;
					ImageBuffer*	pTemplate;
					} CursorInitStr;

//====================================================//
// CPCursor Class Defintion
//====================================================//

class CPCursor {
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(gCockMemPool,size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
public:

	//====================================================//
	// ID Tag
	//====================================================//

	int				mIdNum;

	//====================================================//
	// Source Location in Template Buffer and Hotspot Offsets
	//====================================================//

	RECT				mSrcRect;
	int				mxHotspot;
	int				myHotspot;

	//====================================================//
	// Pointer's to Outside Image Buffers
	//====================================================//

	ImageBuffer		*mpOTWImage;
	ImageBuffer		*mpTemplate;	

	//====================================================//
	// Runtime Member Functions
	//====================================================//

	void Display(void);

	//====================================================//
	// Constructors and Destructors
	//====================================================//
	
	CPCursor(CursorInitStr*);
};

#endif
