#ifndef _CPGAUGE_H
#define _CPGAUGE_H

#ifndef _WINDOWS_
#include <windows.h>
#endif
#include "cpobject.h"
#include "cpmanager.h"

#ifdef USE_SH_POOLS
extern MEM_POOL gCockMemPool;
#endif


typedef struct{
	int		widthTapeBitmap;
	int		heightTapeBitmap;
	float		maxTapeValue;
	int		maxValPosition;
	float		minTapeValue;
	int		minValPosition;
	DWORD*	TapeBitmapHandle;
} CPGaugeInitStruct;

 
class CPGauge : public CPObject{
public:

#ifdef USE_SH_POOLS
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(gCockMemPool,size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif

	CPGauge(CPObjectInitStruct *, CPGaugeInitStruct *);

	int		mWidthTapeBitmap;
	int		mHeightTapeBitmap;
	float		mMaxTapeValue;
	int		mMaxValPosition;
	float		mMinTapeValue;
	int		mMinValPosition;
	DWORD*	mpTapeBitmapHandle;
	float		mCurrentVal;

	virtual ~CPGauge();
	void	Exec(void);
	void	HandleEvent(int Event){};
	void	Display(Render2D*, BOOL);
	void	DrawTape(float, int, int, BOOL);

};

#endif

