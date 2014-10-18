#ifndef _CPMISC_H
#define _CPMISC_H

#include "entity.h"
#include "campwp.h"
#include "cpmanager.h"
#include "tacan.h"

#define MFD_BUTTONS 20


#ifdef USE_SH_POOLS
extern MEM_POOL gCockMemPool;
#endif


class CPObject;

class CPMisc
{
#ifdef USE_SH_POOLS
public:
    // Overload new/delete to use a SmartHeap pool
    void *operator new(size_t size)
    {
        return MemAllocPtr(gCockMemPool, size, FALSE);
    };
    void operator delete(void *mem)
    {
        if (mem) MemFreePtr(mem);
    };
#endif

public:
    CPMisc();
    CockpitManager *mpCPManager;

    // Chaff/Flare Buttons
    typedef enum type_ChaffFlareMode {none, chaff_only, flare_only, both};
    typedef enum type_ChaffFlareControl {automatic, manual};

    type_ChaffFlareMode mChaffFlareMode;
    type_ChaffFlareControl mChaffFlareControl;

    // Analog Clock
    float mHours;
    float mMinutes;
    float mSeconds;

    int mRefuelState;
    unsigned long mRefuelTimer;
    void SetRefuelState(int);

    // Radio stuff
    int mUHFPosition;
    void StepUHFPostion(void);
    void DecUHFPosition(void);


    // MFD Button States
    int MFDButtonArray[MFD_BUTTONS][4]; //Wombat778 4-12-04 Changed from 2 to 4 to accomodate new MFDs
    int GetMFDButtonState(int, int);
    void SetMFDButtonState(int, int, int);

    // Master Caution Stuff
    int mMasterCautionLightState;
    BOOL mMasterCautionEvent;
    void SetMasterCautionEvent(void);
    int GetMasterCautionLight(void);
    void StepMasterCautionLight(void);

    // Eject Handle
    BOOL mEjectState;
    void SetEjectButtonState(BOOL);
    BOOL GetEjectButtonState(void);
};

#endif
