#ifndef _CPTEXT_H
#define _CPTEXT_H

#ifndef _WINDOWS_
#include <windows.h>
#endif
#include "cpcb.h"
#include "cpmanager.h"

#ifdef USE_SH_POOLS
extern MEM_POOL gCockMemPool;
#endif



class CPText : public CPObject
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(gCockMemPool,size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif

public:

	int mNumStrings;
	char** mpString;

	float mLeft;
	float mRight;
	float mTop;
	float mBottom;
	
	CPText(ObjectInitStr*, int);
	virtual ~CPText();

	void	Exec(SimBaseClass*);
	void	DisplayDraw(void);

};

#endif
