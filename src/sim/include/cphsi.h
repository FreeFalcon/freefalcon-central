#ifndef _CPHSI_H
#define _CPHSI_H

#include "cpobject.h"
#include "navsystem.h"


#ifdef USE_SH_POOLS
extern MEM_POOL gCockMemPool;
#endif


const float COMPASS_X_CENTER = 0.0F;
const float COMPASS_Y_CENTER = -0.1F;
const float HALFPI = 1.5708F;
const float TWOPI = 6.2832F;
const int NORMAL_HSI_CRS_STATE = 7;
const int NORMAL_HSI_HDG_STATE = 7;

enum HSIColors
{
    HSI_COLOR_ARROWS,
    HSI_COLOR_ARROWGHOST,
    HSI_COLOR_COURSEWARN,
    HSI_COLOR_HEADINGMARK,
    HSI_COLOR_STATIONBEARING,
    HSI_COLOR_COURSE,
    HSI_COLOR_AIRCRAFT,
    HSI_COLOR_CIRCLES,
    HSI_COLOR_DEVBAR,
    HSI_COLOR_ILSDEVWARN,
    HSI_COLOR_TOTAL
};

//====================================================
// Predeclarations
//====================================================

class CPObject;
class CPHsi;

//====================================================
// Structures used for HSI Initialization
//====================================================

typedef struct
{
    int compassTransparencyType;
    RECT compassSrc;
    RECT compassDest;
    RECT devSrc;
    RECT devDest;
    RECT warnFlag;
    CPHsi *pHsi;
    long colors[HSI_COLOR_TOTAL];
    BYTE *sourcehsi; //Wombat778 3-24-04

} HsiInitStr;



//====================================================
// CPHsi Class Definition
//====================================================

class CPHsi
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

    //====================================================
    // Button States
    //====================================================

    typedef enum HSIButtonStates
    {
        HSI_STA_CRS_STATE,
        HSI_STA_HDG_STATE,
        HSI_STA_TOTAL_STATES
    };

    //====================================================
    // Horizontal Situaton Geometry
    //====================================================

    typedef enum HSIValues
    {
        HSI_VAL_CRS_DEVIATION,
        HSI_VAL_DESIRED_CRS,
        HSI_VAL_DISTANCE_TO_BEACON,
        HSI_VAL_BEARING_TO_BEACON,
        HSI_VAL_CURRENT_HEADING,
        HSI_VAL_DESIRED_HEADING,
        HSI_VAL_DEV_LIMIT,
        HSI_VAL_HALF_DEV_LIMIT,
        HSI_VAL_LOCALIZER_CRS,
        HSI_VAL_AIRBASE_X,
        HSI_VAL_AIRBASE_Y,
        HSI_VAL_TOTAL_VALUES
    };

    //====================================================
    // Control Flags
    //====================================================

    typedef enum HSIFlags
    {
        HSI_FLAG_TO_TRUE,
        HSI_FLAG_ILS_WARN,
        HSI_FLAG_CRS_WARN,
        HSI_FLAG_INIT,
        HSI_FLAG_TOTAL_FLAGS
    };

    //====================================================
    // Constructors and Destructors
    //====================================================

    CPHsi();

    //====================================================
    // Runtime Public Member Functions
    //====================================================

    void Exec(void);

    //====================================================
    // Access Functions
    //====================================================

    void IncState(HSIButtonStates, float step = 5.0F);  // MD -- 20040118: add default arg
    void DecState(HSIButtonStates, float step = 5.0F);  // MD -- 20040118: add default arg
    int GetState(HSIButtonStates);
    float GetValue(HSIValues);
    BOOL GetFlag(HSIFlags);

    // long mColor[2][HSI_COLOR_TOTAL];

    //MI 02/02/02
    float LastHSIHeading;

private:

    //====================================================
    // Internal Data
    //====================================================

    CockpitManager *mpCPManager;


    int mpHsiStates[HSI_STA_TOTAL_STATES];
    float mpHsiValues[HSI_VAL_TOTAL_VALUES];
    BOOL mpHsiFlags[HSI_FLAG_TOTAL_FLAGS];

    NavigationSystem::Instrument_Mode mLastMode;
    WayPointClass* mLastWaypoint;
    VU_TIME lastCheck;
    BOOL lastResult;

    //====================================================
    // Calculation Routines
    //====================================================

    void ExecNav(void);
    void ExecTacan(void);
    void ExecILSNav(void);
    void ExecILSTacan(void);
    void ExecBeaconProximity(float, float, float, float);
    void CalcTCNCrsDev(float course);
    void CalcILSCrsDev(float);
    BOOL BeaconInRange(float rangeToBeacon, float nominalBeaconRange);
};



//====================================================
// Class Definition for CPHSIView
//====================================================

class CPHsiView: public CPObject
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

private:

    //====================================================
    // Pointers to the Outside World
    //====================================================

    CPHsi *mpHsi;

    //====================================================
    // Viewport Dimension Data
    //====================================================

    float mTop;
    float mLeft;
    float mBottom;
    float mRight;

    //====================================================
    // Compass Dial Data
    //====================================================

    int mRadius;

    RECT mDevSrc;
    RECT mDevDest;

    RECT mCompassSrc;
    RECT mCompassDest;
    RECT mWarnFlag;

    int mCompassTransparencyType;

    int *mpCompassCircle;
    int mCompassXCenter;
    int mCompassYCenter;
    int mCompassWidth;
    int mCompassHeight;

    ImageBuffer *CompassBuffer; //Wombat778 10-06-2003   Added to hold temporary buffer for HSI autoscaling/rotation

    ulong mColor[2][HSI_COLOR_TOTAL];

    //====================================================
    // Position Routines
    //====================================================

    void MoveToCompassCenter(void);

    //====================================================
    // Draw Routines for Needles
    //====================================================

    void DrawCourse(float, float);
    void DrawStationBearing(float);
    void DrawAircraftSymbol(void);

    //====================================================
    // Draw Routines for Flags
    //====================================================

    void DrawHeadingMarker(float);
    void DrawCourseWarning(void);
    void DrawToFrom(void);

public:

    //====================================================
    // Runtime Public Draw Routines
    //====================================================

    virtual void DisplayDraw(void);
    virtual void DisplayBlit(void);
    virtual void Exec(SimBaseClass*);

    //Wombat778 3-24-04 Stuff for rendered hsi
    void DisplayBlit3D();
    GLubyte *mpSourceBuffer;
    virtual void CreateLit(void);
    //Wombat778 End

    //====================================================
    // Constructors and Destructors
    //====================================================

    CPHsiView(ObjectInitStr*, HsiInitStr*);
    virtual ~CPHsiView();
};


#endif
