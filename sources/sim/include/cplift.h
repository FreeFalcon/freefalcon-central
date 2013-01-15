#ifndef _CPLIFT_H
#define _CPLIFT_H

#ifndef _WINDOWS_
#include <windows.h>
#endif
#include "stdhdr.h"
#include "cpobject.h"

#ifdef USE_SH_POOLS
extern MEM_POOL gCockMemPool;
#endif


typedef struct {
	float pan;
	float tilt;
	int	panLabel;
	int	tiltLabel;
	BOOL	doLabel;
} LiftInitStr;

//====================================================//
// Structures used for Initialization
//====================================================//

class CPLiftLine : public CPObject
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(gCockMemPool,size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	float pan, tilt;
	BOOL	mDoLabel;
	float mCheveron[12][2];
	float mLineSegment[6][2];

	int	mLineSegments;
	int	mCheverons;
#define CPLIFT_USE_STRING 1
#if CPLIFT_USE_STRING
	std::string mString1;
	std::string mString2;
#else
	char	mString1[20];
	char	mString2[20];
#endif

public:

	CPLiftLine(ObjectInitStr*, LiftInitStr*);
	virtual ~CPLiftLine();
	virtual void DisplayDraw(void);

};

#endif
