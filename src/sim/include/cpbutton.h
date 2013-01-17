#ifndef _CPBUTTON_H
#define _CPBUTTON_H

#ifndef _WINDOWS_
#include <windows.h>
#endif
#include "cpmanager.h"
#include "cpobject.h"
#include "cplight.h"

#ifdef USE_SH_POOLS
extern MEM_POOL gCockMemPool;
#endif


//====================================================//
// Normal Button States
//====================================================//

#define CPBUTTON_OFF			0
#define CPBUTTON_ON			1

//====================================================//
// Button Types
//====================================================//

#define CPBUTTON_EXCLUSIVE	0
#define CPBUTTON_MOMENTARY	1
#define CPBUTTON_TOGGLE		2

#define MAX_MOMENTARY_COUNT	1  // i.e. momentary buttons stay down for two frames

//====================================================//
// CPLight Class Definition
//====================================================//

class CPButton : public CPObject {
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(gCockMemPool,size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
public:

	//====================================================//
	// State Information
	//====================================================//

	int		mStates;
	int		mState;
	int		mInitialState;
	int		mType;
	int		mMomentaryCount; // for momentary buttons only

	//====================================================//
	// Source Locations for Template Surface
	//====================================================//

	RECT			*mpSrcRect;

	//====================================================//
	// Pointer to Cursor Information
	//====================================================//

	int			mCursorId;
	int			mCursorIndex;

	//====================================================//
	// Runtime Member Functions
	//====================================================//

	void			DisplayBlit(void);
	BOOL			HandleEvent(int*, int, int, int);

	//====================================================//
	// Constructors and Destructors
	//====================================================//

	CPButton(ObjectInitStr*, LightButtonInitStr*);
	virtual ~CPButton();
};

#endif

