#include "stdafx.h"
#include "stdhdr.h"
#include "Graphics/Include/display.h"
#include "airframe.h"
#include "campwp.h"
#include "simdrive.h"
#include "aircrft.h"
#include "cphsi.h"
#include "cpmanager.h"
#include "tacan.h"
#include "navsystem.h"
#include "dispopts.h"
#include "fack.h"
#include "otwdrive.h"
#include "Graphics/Include/renderow.h"
#include "UI/INCLUDE/tac_class.h"
#include "ui/include/te_defs.h"
#include "fakerand.h"
#include "dispcfg.h"

//MI 02/02/02
extern bool g_bRealisticAvionics;
extern bool g_bINS;
extern bool g_bCockpitAutoScale;

#include "Graphics/Include/grinline.h" //Wombat778 3-24-04
extern bool g_bFilter2DPit; //Wombat778 3-30-04



CPHsi::CPHsi()
{

    mpHsiStates[HSI_STA_CRS_STATE] = NORMAL_HSI_CRS_STATE;
    mpHsiStates[HSI_STA_HDG_STATE] = NORMAL_HSI_HDG_STATE;

    mpHsiFlags[HSI_FLAG_TO_TRUE] = FALSE;
    mpHsiFlags[HSI_FLAG_ILS_WARN] = FALSE;
    mpHsiFlags[HSI_FLAG_CRS_WARN] = FALSE;
    mpHsiFlags[HSI_FLAG_INIT] = FALSE;
    lastCheck = 0;
    lastResult = FALSE;

    ///VWF HACK: The following is a hack to make the Tac Eng Instrument
    // Landing Mission agree with the Manual

    if (SimDriver.RunningTactical() and 
        current_tactical_mission and 
        current_tactical_mission->get_type() == tt_training and 
        SimDriver.GetPlayerEntity() and 
 not strcmpi(current_tactical_mission->get_title(), "10 Instrument Landing"))
    {
        mpHsiValues[HSI_VAL_DESIRED_CRS] = 340.0F;
    }
    else
    {
        mpHsiValues[HSI_VAL_DESIRED_CRS] = 0.0F;
    }

    mpHsiValues[HSI_VAL_CRS_DEVIATION] = 0.0F;
    mpHsiValues[HSI_VAL_DISTANCE_TO_BEACON] = 0.0F;
    mpHsiValues[HSI_VAL_BEARING_TO_BEACON] = 0.0F;
    mpHsiValues[HSI_VAL_CURRENT_HEADING] = 0.0;
    mpHsiValues[HSI_VAL_DESIRED_HEADING] = 0.0F;
    mpHsiValues[HSI_VAL_DEV_LIMIT] = 10.0F;
    mpHsiValues[HSI_VAL_HALF_DEV_LIMIT] = mpHsiValues[HSI_VAL_DEV_LIMIT] * 0.5F;
    mpHsiValues[HSI_VAL_LOCALIZER_CRS] = 0.0F;
    mpHsiValues[HSI_VAL_AIRBASE_X] = 0.0F;
    mpHsiValues[HSI_VAL_AIRBASE_Y] = 0.0F;

    mLastWaypoint = NULL;
    mLastMode = NavigationSystem::TOTAL_MODES;

    //MI 02/02/02
    LastHSIHeading = 0.0F;
}


int CPHsi::GetState(HSIButtonStates stateIndex)
{

    return mpHsiStates[stateIndex];
}


float CPHsi::GetValue(HSIValues valueIndex)
{

    return mpHsiValues[valueIndex];
}


BOOL CPHsi::GetFlag(HSIFlags flagIndex)
{

    return mpHsiFlags[flagIndex];
}


void CPHsi::IncState(HSIButtonStates state, float step)
{

    HSIValues valueIndex;
    int normalState;

    if (state == HSI_STA_CRS_STATE)
    {
        valueIndex = HSI_VAL_DESIRED_CRS;
        normalState = NORMAL_HSI_CRS_STATE;
    }
    else
    {
        valueIndex = HSI_VAL_DESIRED_HEADING;
        normalState = NORMAL_HSI_HDG_STATE;
    }

    mpHsiValues[valueIndex] += step;  // MD -- 20040118: default 5 degrees

    if (mpHsiValues[valueIndex] >= 360.0F)
    {
        mpHsiValues[valueIndex] = 0.0F;
    }

    if (mpHsiStates[state] == normalState)
    {
        mpHsiStates[state] = 0;
    }
    else
    {
        mpHsiStates[state]++;
    }
}


void CPHsi::DecState(HSIButtonStates state, float step)
{

    HSIValues valueIndex;
    int normalState;

    if (state == HSI_STA_CRS_STATE)
    {
        valueIndex = HSI_VAL_DESIRED_CRS;
        normalState = NORMAL_HSI_CRS_STATE;
    }
    else
    {
        valueIndex = HSI_VAL_DESIRED_HEADING;
        normalState = NORMAL_HSI_HDG_STATE;
    }

    mpHsiValues[valueIndex] -= step;  // MD -- 20040118: default 5 degrees

    if (mpHsiValues[valueIndex] < 0.0F)
    {
        mpHsiValues[valueIndex] = 360.0F;
    }

    if (mpHsiStates[state] == 0)
    {
        mpHsiStates[state] = normalState;
    }
    else
    {
        mpHsiStates[state]--;
    }
}


void CPHsi::ExecNav(void)
{

    float x;
    float y;
    float z;
    WayPointClass *pcurrentWaypoint;
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    if (mpHsiFlags[HSI_FLAG_INIT] == FALSE)
    {

        mpHsiValues[HSI_VAL_DEV_LIMIT] = 10.0F;
        mpHsiValues[HSI_VAL_HALF_DEV_LIMIT] = mpHsiValues[HSI_VAL_DEV_LIMIT] * 0.5F;
        mpHsiFlags[HSI_FLAG_INIT] = TRUE;
        mpHsiFlags[HSI_FLAG_TO_TRUE] = FALSE;
    }

    if (playerAC == NULL)
    {
        return;
    }

    pcurrentWaypoint = playerAC->curWaypoint;

    // sfr TODO remove jb hack
    if (F4IsBadReadPtr(pcurrentWaypoint, sizeof * pcurrentWaypoint)) // JPO CTD check
    {
        mpHsiFlags[HSI_FLAG_CRS_WARN] = TRUE;
        ExecBeaconProximity(SimDriver.GetPlayerEntity()->XPos(), playerAC->YPos(), 0.0F, 0.0F);
        CalcTCNCrsDev(mpHsiValues[HSI_VAL_DESIRED_CRS]);
    }
    else
    {
        mpHsiFlags[HSI_FLAG_CRS_WARN] = FALSE;
        pcurrentWaypoint->GetLocation(&x, &y, &z);
        ExecBeaconProximity(playerAC->XPos(), playerAC->YPos(), x, y);
        CalcTCNCrsDev(mpHsiValues[HSI_VAL_DESIRED_CRS]);
    }

    if (mpHsiValues[HSI_VAL_CRS_DEVIATION] < 270 and mpHsiValues[HSI_VAL_CRS_DEVIATION] > 90)
    {
        mpHsiFlags[HSI_FLAG_TO_TRUE] = 2;
    }
    else
    {
        mpHsiFlags[HSI_FLAG_TO_TRUE] = TRUE;
    }


}


void CPHsi::ExecTacan(void)
{

    float ownshipX;
    float ownshipY;
    float tacanX;
    float tacanY, tacanZ;
    float tacanRange;

    if (mpHsiFlags[HSI_FLAG_INIT] == FALSE)
    {

        mpHsiValues[HSI_VAL_DEV_LIMIT] = 10.0F;
        mpHsiValues[HSI_VAL_HALF_DEV_LIMIT] = mpHsiValues[HSI_VAL_DEV_LIMIT] * 0.5F;
        mpHsiFlags[HSI_FLAG_INIT] = TRUE;
        mpHsiFlags[HSI_FLAG_ILS_WARN] = TRUE;
        mpHsiFlags[HSI_FLAG_TO_TRUE] = TRUE;
    }

    if (SimDriver.GetPlayerEntity())
    {

        ownshipX = SimDriver.GetPlayerEntity()->XPos();
        ownshipY = SimDriver.GetPlayerEntity()->YPos();

        mpHsiFlags[HSI_FLAG_CRS_WARN] = not gNavigationSys->GetTCNPosition(&tacanX, &tacanY, &tacanZ);
        gNavigationSys->GetTCNAttribute(NavigationSystem::RANGE, &tacanRange);

        if (gNavigationSys->IsTCNTanker() or gNavigationSys->IsTCNAirbase() or gNavigationSys->IsTCNCarrier())   // now Carrier support
        {
            mpHsiFlags[HSI_FLAG_ILS_WARN] = FALSE;
        }
        else
        {
            mpHsiFlags[HSI_FLAG_ILS_WARN] =  mpHsiFlags[HSI_FLAG_CRS_WARN];
        }

        ExecBeaconProximity(ownshipX, ownshipY, tacanX, tacanY);

        if (BeaconInRange(mpHsiValues[HSI_VAL_DISTANCE_TO_BEACON], tacanRange))
        {
            CalcTCNCrsDev(mpHsiValues[HSI_VAL_DESIRED_CRS]);
        }
        else
        {
            mpHsiValues[HSI_VAL_BEARING_TO_BEACON] = 90; // fix at 90 deg off
            mpHsiValues[HSI_VAL_DISTANCE_TO_BEACON] = 0;
            mpHsiFlags[HSI_FLAG_ILS_WARN] = TRUE;
            mpHsiFlags[HSI_FLAG_CRS_WARN] = TRUE;
        }

        if (mpHsiValues[HSI_VAL_CRS_DEVIATION] < 270 and mpHsiValues[HSI_VAL_CRS_DEVIATION] > 90)
            mpHsiFlags[HSI_FLAG_TO_TRUE] = 2;
        else
            mpHsiFlags[HSI_FLAG_TO_TRUE] = TRUE;
    }
}


void CPHsi::ExecILSNav(void)
{
    float ownshipX;
    float ownshipY;
    WayPointClass *pcurrentWaypoint;
    float waypointX;
    float waypointY;
    float waypointZ;
    float gpDew;
    VU_ID ilsObj;
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    if (mpHsiFlags[HSI_FLAG_INIT] == FALSE)
    {

        mpHsiValues[HSI_VAL_DEV_LIMIT] = 10.0F;
        mpHsiValues[HSI_VAL_HALF_DEV_LIMIT] = mpHsiValues[HSI_VAL_DEV_LIMIT] * 0.5F;
        mpHsiFlags[HSI_FLAG_INIT] = TRUE;
        mpHsiFlags[HSI_FLAG_TO_TRUE] = FALSE;
    }

    if (playerAC == NULL)
    {
        return;
    }

    pcurrentWaypoint = playerAC->curWaypoint;
    ownshipX = playerAC->XPos();
    ownshipY = playerAC->YPos();

    if (pcurrentWaypoint == NULL)
    {
        mpHsiFlags[HSI_FLAG_CRS_WARN] = TRUE;
        ExecBeaconProximity(ownshipX, ownshipY, 0.0F, 0.0F);
    }
    else
    {
        pcurrentWaypoint->GetLocation(&waypointX, &waypointY, &waypointZ);
        ExecBeaconProximity(playerAC->XPos(), playerAC->YPos(), waypointX, waypointY);
        mpHsiFlags[HSI_FLAG_ILS_WARN] = not gNavigationSys->GetILSAttribute(NavigationSystem::GP_DEV, &gpDew);
        CalcILSCrsDev(gpDew);
    }

    if (mpHsiValues[HSI_VAL_CRS_DEVIATION] < 270 and mpHsiValues[HSI_VAL_CRS_DEVIATION] > 90)
    {
        mpHsiFlags[HSI_FLAG_TO_TRUE] = 2;
    }
    else
    {
        mpHsiFlags[HSI_FLAG_TO_TRUE] = TRUE;
    }
}



void CPHsi::ExecBeaconProximity(float x1, float y1, float x2, float y2)
{
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    if (playerAC == NULL)
    {
        return;
    }

    double xdiff, ydiff;
    double bearingToBeacon;

    xdiff = x2 - x1;
    ydiff = y2 - y1;

    bearingToBeacon = atan2(xdiff, ydiff); // radians +-pi, xaxis = 0deg

    mpHsiValues[HSI_VAL_BEARING_TO_BEACON] = ConvertRadtoNav((float)bearingToBeacon);
    mpHsiValues[HSI_VAL_DISTANCE_TO_BEACON] = (float)sqrt((float)(xdiff * xdiff + ydiff * ydiff)) * 0.0001666F; // in nautical miles (1 / 6000)

    if (mpHsiValues[HSI_VAL_DISTANCE_TO_BEACON] > 999.0F)
    {
        mpHsiValues[HSI_VAL_DISTANCE_TO_BEACON] = 999.0F;
    }
    else if ((mpHsiValues[HSI_VAL_DISTANCE_TO_BEACON] - (float)floor(mpHsiValues[HSI_VAL_DISTANCE_TO_BEACON])) > 0.5F
            )
    {
        mpHsiValues[HSI_VAL_DISTANCE_TO_BEACON] = (float)ceil(mpHsiValues[HSI_VAL_DISTANCE_TO_BEACON]);
    }

    mpHsiValues[HSI_VAL_CURRENT_HEADING] = playerAC->af->psi * RTD;

    //update our value
    if (playerAC->INSState(AircraftClass::INS_HSI_OFF_IN))
    {
        LastHSIHeading = mpHsiValues[HSI_VAL_CURRENT_HEADING];
    }

    if (g_bRealisticAvionics and g_bINS and not playerAC->INSState(AircraftClass::INS_HSI_OFF_IN))
    {
        mpHsiValues[HSI_VAL_CURRENT_HEADING] = LastHSIHeading;
    }

    if (mpHsiValues[HSI_VAL_CURRENT_HEADING] < 0.0F)
    {
        mpHsiValues[HSI_VAL_CURRENT_HEADING] += 360.0F;
    }
}


void CPHsi::CalcTCNCrsDev(float course)
{

    mpHsiValues[HSI_VAL_CRS_DEVIATION] = course - mpHsiValues[HSI_VAL_BEARING_TO_BEACON]; // in degrees

    if (mpHsiValues[HSI_VAL_CRS_DEVIATION] < 0.0F)
    {
        mpHsiValues[HSI_VAL_CRS_DEVIATION] += 360.0F;
    }
}


BOOL CPHsi::BeaconInRange(float rangeToBeacon, float nominalBeaconrange)
{
    // we check every 5 seconds, to see if in range, else use the last result
    if (lastCheck < SimLibElapsedTime)
    {
        lastCheck = SimLibElapsedTime + 5 * CampaignSeconds;
    }
    else
    {
        return lastResult;
    }

    // RAS - 18 Dec 03 - Added Radio Horizon Code
    // Purpose:  reception of TACAN based on altitude of aircraft
    // Note:  This added code combines both the range code and altitude code

    int alt = 0; // Aircrafts altitude
    double radioHorizon = 0.0; // Set radio horizon to 0
    float rndNum = PRANDFloatPos(); // number picked from 4x4 array to add intermitant reception

    alt = SimDriver.GetPlayerEntity()->GetAltitude(); // Get current altitude (AGL)

    if (alt <= 0)  alt = 1; // Prevent taking the sqrt of 0 (may not be needed)

    radioHorizon = 1.06f * sqrt((float)alt); // Simplified equation for radio line-of-site  //JPG 18 Apr 04 - Replaced sqrt(2*alt) w/ better equation

    // Ensure there is no divide by 0
    if (nominalBeaconrange == 0) nominalBeaconrange = 1;

    if (rangeToBeacon == 0) rangeToBeacon = 1;

    // used to add random reception at edge of max range and lower altitudes
    float detectionChanceRange = 1.0f - (rangeToBeacon - nominalBeaconrange)  * 2.0f / nominalBeaconrange;
    float detectionChanceAlt = (float)(0.5f - (rangeToBeacon - radioHorizon) * 2.0f / radioHorizon);

    //MonoPrint("detectionChanceRange = %f\n", detectionChanceRange);
    //MonoPrint("detectionChanceAlt = %f\n", detectionChanceAlt);
    //MonoPrint("radioHorizon = %f\n", radioHorizon);
    //MonoPrint("rangeToBeacon = %f\n", rangeToBeacon);
    //MonoPrint("alt = %i\n\n", alt);

    // above radio horizon and within detection range - good reception
    if ((rangeToBeacon < radioHorizon) and (detectionChanceRange > 1) and (detectionChanceAlt > 1)) // definite receive
        lastResult = TRUE;
    // outside of radio horizon or detection range or altitude detection range to low for reception - no reception
    else if ((rangeToBeacon > radioHorizon) or (detectionChanceRange <= 0) or (detectionChanceAlt <= 0.5))
        lastResult = FALSE;
    // detection range at max or detection altitude near min (intermitant reception)
    else if ((detectionChanceRange > rndNum) and (detectionChanceAlt > rndNum))
        lastResult = TRUE;
    else lastResult = FALSE;

    // End of new code



    // ***** Old Range Code ****
    //if (nominalBeaconrange ==0) nominalBeaconrange = 1;
    //float detectionChance = 1.0f -  (rangeToBeacon - nominalBeaconrange)  * 2.0f / nominalBeaconrange;
    //if (detectionChance > 1) lastResult = TRUE; // definite receive
    //else if (detectionChance <= 0) lastResult = FALSE; // definite no receive
    //else if (detectionChance > PRANDFloatPos()) { // somewhere in between
    //lastResult = TRUE;
    //}
    //else lastResult = FALSE;

    return lastResult;
}


void CPHsi::CalcILSCrsDev(float dev)
{

    mpHsiValues[HSI_VAL_CRS_DEVIATION] = -dev * RTD;

    if (mpHsiValues[HSI_VAL_CRS_DEVIATION] < 0.0F)
    {
        mpHsiValues[HSI_VAL_CRS_DEVIATION] += 360.0F;
    }
}


void CPHsi::Exec(void)
{

    NavigationSystem::Instrument_Mode mode;
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    if (playerAC == NULL)
    {
        return;
    }

    // Check for HSI Failure
    if (playerAC->mFaults and playerAC->mFaults->GetFault(FaultClass::tcn_fault))
    {
        return;
    }

    if (gNavigationSys)
    {
        mode = gNavigationSys->GetInstrumentMode();
    }
    else
    {
        return;
    }

    if (mode not_eq mLastMode)
    {
        mpHsiFlags[HSI_FLAG_INIT] = FALSE;
    }

    mLastMode = mode;

    switch (mode)
    {

        case NavigationSystem::NAV:
            ExecNav();
            break;

        case NavigationSystem::ILS_NAV:
            ExecILSNav();
            break;

        case NavigationSystem::TACAN:
            ExecTacan();
            break;

        case NavigationSystem::ILS_TACAN:
            ExecILSTacan();
            break;

        default:
            break;

    }
}


void CPHsi::ExecILSTacan(void)
{

    float ownshipX;
    float ownshipY;
    float tacanX;
    float tacanY, tacanZ;
    float gpDew;
    float range;

    if (mpHsiFlags[HSI_FLAG_INIT] == FALSE)
    {

        mpHsiValues[HSI_VAL_DEV_LIMIT] = 10.0F;
        mpHsiValues[HSI_VAL_HALF_DEV_LIMIT] = mpHsiValues[HSI_VAL_DEV_LIMIT] * 0.5F;
        mpHsiFlags[HSI_FLAG_INIT] = TRUE;
        mpHsiFlags[HSI_FLAG_ILS_WARN] = FALSE;
        mpHsiFlags[HSI_FLAG_TO_TRUE] = FALSE;
    }

    if (SimDriver.GetPlayerEntity())
    {

        ownshipX = SimDriver.GetPlayerEntity()->XPos();
        ownshipY = SimDriver.GetPlayerEntity()->YPos();

        mpHsiFlags[HSI_FLAG_CRS_WARN] = not gNavigationSys->GetTCNPosition(&tacanX, &tacanY, &tacanZ);
        gNavigationSys->GetTCNAttribute(NavigationSystem::RANGE, &range);

        mpHsiFlags[HSI_FLAG_ILS_WARN] = not gNavigationSys->GetILSAttribute(NavigationSystem::GP_DEV, &gpDew);
        ExecBeaconProximity(ownshipX, ownshipY, tacanX, tacanY);

        if (BeaconInRange(mpHsiValues[HSI_VAL_DISTANCE_TO_BEACON], range))
        {
            CalcILSCrsDev(gpDew);
        }
        else
        {
            mpHsiFlags[HSI_FLAG_CRS_WARN] = TRUE;
            mpHsiFlags[HSI_FLAG_ILS_WARN] = TRUE;
            mpHsiValues[HSI_VAL_DISTANCE_TO_BEACON] = 0;
            // mpHsiValues[HSI_VAL_CRS_DEVIATION] = 0; // last value?
        }

        if (mpHsiValues[HSI_VAL_CRS_DEVIATION] < 270 and mpHsiValues[HSI_VAL_CRS_DEVIATION] > 90)
            mpHsiFlags[HSI_FLAG_TO_TRUE] = 2;
        else
            mpHsiFlags[HSI_FLAG_TO_TRUE] = TRUE;
    }
}

/////////////
// HSIVIEW //
/////////////
CPHsiView::CPHsiView(ObjectInitStr *pobjectInitStr, HsiInitStr *phsiInitStr) : CPObject(pobjectInitStr)
{

    int radiusSquared;
    int arraySize, halfArraySize;
    int i;
    float x, y;
    int halfHeight;
    int halfWidth;

    mWarnFlag.top = (long)(phsiInitStr->warnFlag.top * mVScale);
    mWarnFlag.left = (long)(phsiInitStr->warnFlag.left * mHScale);
    mWarnFlag.bottom = (long)(phsiInitStr->warnFlag.bottom * mVScale);
    mWarnFlag.right = (long)(phsiInitStr->warnFlag.right * mHScale);

    mpHsi = phsiInitStr->pHsi;

    mCompassTransparencyType = phsiInitStr->compassTransparencyType;

    mCompassSrc = phsiInitStr->compassSrc;

    mCompassDest = phsiInitStr->compassDest;
    mCompassDest.top = (long)(mCompassDest.top * mVScale);
    mCompassDest.left = (long)(mCompassDest.left * mHScale);
    mCompassDest.bottom = (long)(mCompassDest.bottom * mVScale);
    mCompassDest.right = (long)(mCompassDest.right * mHScale);

    mDevSrc = phsiInitStr->devSrc;

    mDevDest = phsiInitStr->devDest;
    mDevDest.top = (long)(mDevDest.top * mVScale);
    mDevDest.left = (long)(mDevDest.left * mHScale);
    mDevDest.bottom = (long)(mDevDest.bottom * mVScale);
    mDevDest.right = (long)(mDevDest.right * mHScale);

    mCompassWidth = mCompassSrc.right - mCompassSrc.left;
    mCompassHeight = mCompassSrc.bottom - mCompassSrc.top;

    // mCompassWidth = mCompassDest.right - mCompassDest.left;
    // mCompassHeight = mCompassDest.bottom - mCompassDest.top;

    mCompassXCenter = mCompassDest.left + mCompassWidth / 2;
    mCompassYCenter = mCompassDest.top + mCompassHeight / 2;

    // Setup the compass circle limits
    mRadius = max(mCompassWidth, mCompassHeight);
    mRadius = (mRadius + 1) / 2;
    arraySize = mRadius * 4;

#ifdef USE_SH_POOLS
    mpCompassCircle = (int *)MemAllocPtr(gCockMemPool, sizeof(int) * arraySize, FALSE);
#else
    mpCompassCircle = new int[arraySize];
#endif

    radiusSquared = mRadius * mRadius;

    halfArraySize = arraySize / 2;

    for (i = 0; i < halfArraySize; i++)
    {

        y = (float)fabs((float)i - mRadius);
        x = (float)sqrt(radiusSquared - y * y);
        mpCompassCircle[i * 2 + 1] = mRadius + (int)x; //right
        mpCompassCircle[i * 2 + 0] = mRadius - (int)x; //left
    }

    halfHeight = DisplayOptions.DispHeight / 2;
    halfWidth = DisplayOptions.DispWidth / 2;
    mTop = (float)(halfHeight - mDevDest.top) / halfHeight;
    mLeft = (float)(mDevDest.left - halfWidth) / halfWidth;
    mBottom = (float)(halfHeight - mDevDest.bottom) / halfHeight;
    mRight = (float)(mDevDest.right - halfWidth) / halfWidth;

    for (i = 0; i < HSI_COLOR_TOTAL; i++)
    {
        mColor[0][i] = phsiInitStr->colors[i];
        mColor[1][i] = CalculateNVGColor(mColor[0][i]);
    }



    //Wombat778 3-22-04  Added for rendered (rather than blitted hsi)
    if (DisplayOptions.bRender2DCockpit)
    {
        mpSourceBuffer = phsiInitStr->sourcehsi;
    }
    //Wombat778 10-06-2003 Added following lines to set up a temporary buffer for the HSI
    //this is unnecessary in using rendered pit
    else if (g_bCockpitAutoScale and ((mVScale not_eq 1.0f) or (mHScale not_eq 1.0f)))
    {

        CompassBuffer = new ImageBuffer;
        CompassBuffer->Setup(&FalconDisplay.theDisplayDevice, mCompassWidth, mCompassHeight, SystemMem, None, FALSE); //Wombat778 10-06 2003 Setup a new imagebuffer the size of the Compass
        CompassBuffer->SetChromaKey(0xFFFF0000);
    }

    //Wombat778 10-06-2003 End of added code
}


CPHsiView::~CPHsiView(void)
{

    delete mpCompassCircle;

    //Wombat778 3-22-04 clean up buffers
    if (DisplayOptions.bRender2DCockpit)
    {
        glReleaseMemory((char*) mpSourceBuffer);
    }
    //Wombat778 10-06-2003 Added following lines to destroy the temporary imagebuffer;
    //unnecessary if using rendered hsi
    else if (g_bCockpitAutoScale and ((mVScale not_eq 1.0f) or (mHScale not_eq 1.0f)))
    {
        if (CompassBuffer)
        {
            CompassBuffer->Cleanup();
            delete CompassBuffer;
        }
    }

    //Wombat778 10-06-2003 End of added code
}

////////////////////////////////////////////////////////



//------------------------------------------------------
// CPHsiView::DisplayBlit
//------------------------------------------------------

void CPHsiView::DisplayBlit()
{

    mDirtyFlag = TRUE;

    if ( not mDirtyFlag or DisplayOptions.bRender2DCockpit)
    {
        return;
    }

    float angle;
    float currentHeading = mpHsi->GetValue(CPHsi::HSI_VAL_CURRENT_HEADING);

    angle = currentHeading + 90.0F;

    if (angle > 360.0F)
    {
        angle -= 360.0F;
    }

    // Make the rotating blt call

    //Wombat778 10-06-2003, modified following lines. allows HSI to scale properly when using cockpit auto scaling
    if (g_bCockpitAutoScale and ((mVScale not_eq 1.0f) or (mHScale not_eq 1.0f)))   //dont run this code if the var is set but no scaling is occuring
    {

        RECT temprect;

        temprect.top = 0;
        temprect.left = 0;
        temprect.right = mCompassWidth;
        temprect.bottom = mCompassHeight;

        CompassBuffer->Clear(0xFFFF0000); //clear the temp buffer with chromakey blue;
        CompassBuffer->ComposeRoundRot(mpTemplate, &mCompassSrc, &temprect, ConvertNavtoRad(angle), mpCompassCircle); //Rotate the image from template to temp buffer
        mpOTWImage->ComposeTransparent(CompassBuffer, &temprect, &mCompassDest);

    }
    else
    {
        mpOTWImage->ComposeRoundRot(mpTemplate, &mCompassSrc, &mCompassDest, ConvertNavtoRad(angle), mpCompassCircle);
    }

    //Wombat778 10-06-2003  End of modified lines

    mDirtyFlag = FALSE;
}

//Wombat778 3-23-04 Add support for rendered hsi.  Much faster than blitting.
//Wombat778 3-22-04 helper function to keep the displayblit3d tidy.
void RenderHSIPoly(tagRECT *srcrect, tagRECT *destrect, GLint alpha, TextureHandle *pTex, float angle)
{
    int entry;

    const float angsin = sin(angle);
    const float angcos = cos(angle);

    float x = destrect->left + ((float)(destrect->right - destrect->left) / 2.0f);
    float y = destrect->top + ((float)(destrect->bottom - destrect->top) / 2.0f);
    float Radius = (float)(destrect->bottom - destrect->top) / 2.0f;
    float hw_rate = OTWDriver.pCockpitManager->mHScale / OTWDriver.pCockpitManager->mVScale;


    OTWDriver.renderer->CenterOriginInViewport();
    OTWDriver.renderer->SetViewport(-1.0F, 1.0F, 1.0F, -1.0F);

    // Setup vertices
    float fStartU = 0;
    float fMaxU = (float) pTex->m_nWidth / (float) pTex->m_nActualWidth;
    fMaxU -= fStartU;
    float fStartV = 0;
    float fMaxV = (float) pTex->m_nHeight / (float) pTex->m_nActualHeight;
    fMaxV -= fStartV;


    TwoDVertex pVtx[90];
    ZeroMemory(pVtx, sizeof(pVtx));

    for (entry = 0; entry <= CircleSegments - 2; entry++)
    {
        float x1, y1;
        // Compute the end point of this next segment
        x1 = (x + Radius * CircleX[entry]);
        y1 = (y + Radius * CircleY[entry]);

        // Draw the segment
        pVtx[entry].x = x1;
        pVtx[entry].y = y1;

        float tempx = pVtx[entry].x;
        pVtx[entry].x = x + ((angcos * (pVtx[entry].x - x)) + (-angsin * (pVtx[entry].y - y)))  * hw_rate;
        pVtx[entry].y = y + (angsin * (tempx - x)) + (angcos * (pVtx[entry].y - y));
        x1 = (x + Radius * CircleX[entry] * hw_rate);
        pVtx[entry].u = fStartU +
                        (((float)(x1 - destrect->left) / (float)(destrect->right - destrect->left)) * (fMaxU - fStartU));
        pVtx[entry].v = fStartV +
                        (((float)(y1 - destrect->top) / (float)(destrect->bottom - destrect->top)) * (fMaxV - fStartV));

        pVtx[entry].r = pVtx[entry].g = pVtx[entry].b = pVtx[entry].a = 1.0f;
    }

    OTWDriver.renderer->context.RestoreState(alpha);
    OTWDriver.renderer->context.SelectTexture1((GLint) pTex);
    OTWDriver.renderer->context.DrawPrimitive(MPR_PRM_TRIFAN, MPR_VI_COLOR bitor MPR_VI_TEXTURE, 90, pVtx, sizeof(pVtx[0]));
}


void CPHsiView::DisplayBlit3D()
{

    mDirtyFlag = TRUE;

    if ( not mDirtyFlag or not DisplayOptions.bRender2DCockpit)
    {
        return;
    }

    float angle;
    float currentHeading = mpHsi->GetValue(CPHsi::HSI_VAL_CURRENT_HEADING);

    angle = currentHeading + 90.0F;

    if (angle > 360.0F)
    {
        angle -= 360.0F;
    }

    RECT DestRect = mCompassDest;
    // COBRA - RED - Pit Vibrations
    int OffsetX = (int)OTWDriver.pCockpitManager->PitTurbulence.x;
    int OffsetY = (int)OTWDriver.pCockpitManager->PitTurbulence.y;
    DestRect.top += OffsetY;
    DestRect.bottom += OffsetY;
    DestRect.left += OffsetX;
    DestRect.right += OffsetX;


    if (g_bFilter2DPit)
    {
        //Wombat778 3-30-04 Added option to filter
        RenderHSIPoly(&mCompassSrc, &DestRect, STATE_TEXTURE, m_arrTex[0], ConvertNavtoRad(angle));
    }
    else
    {
        RenderHSIPoly(&mCompassSrc, &DestRect, STATE_TEXTURE_NOFILTER, m_arrTex[0], ConvertNavtoRad(angle));
    }

    mDirtyFlag = FALSE;
}

//Wombat778 3-24-04 end


void CPHsiView::Exec(SimBaseClass*)
{

    OTWDriver.pCockpitManager->mpHsi->Exec();
}


void CPHsiView::DisplayDraw()
{
    static BOOL monoYes;
    static int init = 0;

    if ( not init)
    {
        monoYes = 0;
        init = 1;
    }

    mDirtyFlag = TRUE;

    if ( not mDirtyFlag)
    {
        return;
    }

    float desiredCourse = mpHsi->GetValue(CPHsi::HSI_VAL_DESIRED_CRS);
    float desiredHeading = mpHsi->GetValue(CPHsi::HSI_VAL_DESIRED_HEADING);
    float courseDeviation = mpHsi->GetValue(CPHsi::HSI_VAL_CRS_DEVIATION);
    float bearingToBeacon = mpHsi->GetValue(CPHsi::HSI_VAL_BEARING_TO_BEACON);
    BOOL crsWarnFlag = mpHsi->GetFlag(CPHsi::HSI_FLAG_CRS_WARN);


    if (monoYes)
    {
        MonoPrint("bearingToBeacon = %f\n", bearingToBeacon);
    }


    OTWDriver.renderer->SetViewport(mLeft, mTop, mRight, mBottom);

    DrawCourse(desiredCourse, courseDeviation);
    DrawStationBearing(bearingToBeacon);
    DrawHeadingMarker(desiredHeading);

    DrawAircraftSymbol();

    if (crsWarnFlag)
    {
        DrawCourseWarning();
    }

    mDirtyFlag = FALSE;
}


void CPHsiView::DrawToFrom(void)
{

    static const float toArrow[3][2] =
    {
        { 0.055F,  0.12F - 0.25f},
        { 0.157F,  0.00F - 0.25f},
        { 0.055F, -0.12F - 0.25f},
    };
    BOOL crsToTrueFlag = mpHsi->GetFlag(CPHsi::HSI_FLAG_TO_TRUE);

    //MI
    if (g_bRealisticAvionics)
    {
        if (gNavigationSys)
        {
            if (gNavigationSys->GetInstrumentMode() == NavigationSystem::NAV)
                crsToTrueFlag = FALSE;
        }
    }

    //sfr: took the colors out of the ifs
    DWORD colorArrows = OTWDriver.pCockpitManager->ApplyLighting(mColor[0][HSI_COLOR_ARROWS], true);
    DWORD colorArrowGhost = OTWDriver.pCockpitManager->ApplyLighting(mColor[0][HSI_COLOR_ARROWGHOST], true);

    if (crsToTrueFlag == TRUE)
    {
        // Draw To-From Arrows
        OTWDriver.renderer->SetColor(colorArrows);
        OTWDriver.renderer->Tri(toArrow[0][0], toArrow[0][1], toArrow[1][0], toArrow[1][1],
                                toArrow[2][0], toArrow[2][1]);
        // draw ghost arrow
        OTWDriver.renderer->SetColor(colorArrowGhost);
        OTWDriver.renderer->Line(-toArrow[0][0], toArrow[0][1], -toArrow[1][0], toArrow[1][1]);
        OTWDriver.renderer->Line(-toArrow[1][0], toArrow[1][1], -toArrow[2][0], toArrow[2][1]);
        OTWDriver.renderer->Line(-toArrow[2][0], toArrow[2][1], -toArrow[0][0], toArrow[0][1]);

    }
    else if (crsToTrueFlag == 2)
    {
        // from
        OTWDriver.renderer->SetColor(colorArrows);
        OTWDriver.renderer->Tri(-toArrow[0][0], toArrow[0][1], -toArrow[1][0], toArrow[1][1],
                                -toArrow[2][0], toArrow[2][1]);
        // draw ghost arrow
        OTWDriver.renderer->SetColor(colorArrowGhost);
        OTWDriver.renderer->Line(toArrow[0][0], toArrow[0][1], toArrow[1][0], toArrow[1][1]);
        OTWDriver.renderer->Line(toArrow[1][0], toArrow[1][1], toArrow[2][0], toArrow[2][1]);
        OTWDriver.renderer->Line(toArrow[2][0], toArrow[2][1], toArrow[0][0], toArrow[0][1]);
    }
    else
    {
        OTWDriver.renderer->SetColor(colorArrowGhost);
        // draw ghost arrow
        OTWDriver.renderer->Line(-toArrow[0][0], toArrow[0][1], -toArrow[1][0], toArrow[1][1]);
        OTWDriver.renderer->Line(-toArrow[1][0], toArrow[1][1], -toArrow[2][0], toArrow[2][1]);
        OTWDriver.renderer->Line(-toArrow[2][0], toArrow[2][1], -toArrow[0][0], toArrow[0][1]);
        // draw ghost arrow
        OTWDriver.renderer->Line(toArrow[0][0], toArrow[0][1], toArrow[1][0], toArrow[1][1]);
        OTWDriver.renderer->Line(toArrow[1][0], toArrow[1][1], toArrow[2][0], toArrow[2][1]);
        OTWDriver.renderer->Line(toArrow[2][0], toArrow[2][1], toArrow[0][0], toArrow[0][1]);
    }
}


void CPHsiView::DrawCourseWarning(void)
{

    OTWDriver.renderer->SetViewport(-1.0F, 1.0F, 1.0F, -1.0F);
    DWORD color = OTWDriver.pCockpitManager->ApplyLighting(mColor[0][HSI_COLOR_COURSEWARN], true);
    OTWDriver.renderer->SetColor(color);
    OTWDriver.renderer->Render2DTri((float)mWarnFlag.left, (float)mWarnFlag.top,
                                    (float)mWarnFlag.right, (float)mWarnFlag.top,
                                    (float)mWarnFlag.right, (float)mWarnFlag.bottom);
    OTWDriver.renderer->Render2DTri((float)mWarnFlag.left, (float)mWarnFlag.top,
                                    (float)mWarnFlag.left, (float)mWarnFlag.bottom,
                                    (float)mWarnFlag.right, (float)mWarnFlag.bottom);

}


void CPHsiView::DrawHeadingMarker(float desiredHeading)
{

    static const float headingMarker[2][2] =
    {
        {0.56F, 0.04F},
        {0.61F, 0.04F},
    };
    float currentHeading = mpHsi->GetValue(CPHsi::HSI_VAL_CURRENT_HEADING);

    MoveToCompassCenter();


    OTWDriver.renderer->AdjustRotationAboutOrigin(-ConvertNavtoRad(360.0F - currentHeading + desiredHeading));
    DWORD color = OTWDriver.pCockpitManager->ApplyLighting(mColor[0][HSI_COLOR_HEADINGMARK], true);
    OTWDriver.renderer->SetColor(color);

    // draw tail
    OTWDriver.renderer->Tri(headingMarker[0][0], headingMarker[0][1], headingMarker[1][0], headingMarker[1][1],
                            headingMarker[1][0], -headingMarker[1][1]);
    OTWDriver.renderer->Tri(headingMarker[0][0], headingMarker[0][1], headingMarker[1][0], -headingMarker[1][1],
                            headingMarker[0][0], -headingMarker[0][1]);

}

void CPHsiView::DrawStationBearing(float bearing)   // in nav units
{

    static const float bearingArrow[2][2] =
    {
        {0.69F, 0.05F},
        {0.80F, 0.0F},
    };
    static const float bearingTail[2][2] =
    {
        { -0.80F, 0.02F},
        { -0.69F, 0.02F},
    };
    float currentHeading = mpHsi->GetValue(CPHsi::HSI_VAL_CURRENT_HEADING);


    MoveToCompassCenter();

    OTWDriver.renderer->AdjustRotationAboutOrigin(-ConvertNavtoRad(360.0F - currentHeading + bearing));

    DWORD color = OTWDriver.pCockpitManager->ApplyLighting(mColor[0][HSI_COLOR_STATIONBEARING], true);
    OTWDriver.renderer->SetColor(color);

    // draw arrow
    OTWDriver.renderer->Tri(bearingArrow[0][0], bearingArrow[0][1], bearingArrow[1][0], bearingArrow[1][1],
                            bearingArrow[0][0], -bearingArrow[0][1]);

    // draw tail
    //MI this is not here in real
    // if( not g_bRealisticAvionics) //Wombat778 10-19-2003   Re-enabled in all avionics modes for realism as per MIRV
    // {
    OTWDriver.renderer->Tri(bearingTail[0][0], bearingTail[0][1], bearingTail[1][0], bearingTail[1][1],
                            bearingTail[1][0], -bearingTail[1][1]);
    OTWDriver.renderer->Tri(bearingTail[0][0], bearingTail[0][1], bearingTail[1][0], -bearingTail[1][1],
                            bearingTail[0][0], -bearingTail[0][1]);
    // }
}

void CPHsiView::DrawCourse(float desiredCourse, float deviaiton)
{

    static const float courseArrow[4][2] =
    {
        {0.610F, 0.000F},
        {0.50F, 0.050F},
        {0.50F, 0.020F},
        {0.25F, 0.020F},
    };
    static const float courseTail[2][2] =
    {
        { -0.40F, 0.020F},
        { -0.60F, 0.020F},
    };
    static const float courseDevScale[3] = {0.0000F, 0.24F, 0.04F };
    static const float courseDevBar[2][2] =
    {
        { -0.39F, 0.020F},
        { 0.24F, 0.020F},
    };
    static const float courseDevWarn[2] = { -0.04F, 0.08F};

    float devBarCenter[2];
    float startPoint;
    float r;
    float theta;
    int i;
    float currentHeading = mpHsi->GetValue(CPHsi::HSI_VAL_CURRENT_HEADING);
    float deviationLimit = mpHsi->GetValue(CPHsi::HSI_VAL_DEV_LIMIT);
    float halfDeviationLimit = mpHsi->GetValue(CPHsi::HSI_VAL_HALF_DEV_LIMIT);
    BOOL ilsWarnFlag = mpHsi->GetFlag(CPHsi::HSI_FLAG_ILS_WARN);
    mlTrig         trig;

    MoveToCompassCenter();
    desiredCourse = 360.0F - currentHeading + desiredCourse;

    if (desiredCourse > 360.0F)
    {
        desiredCourse -= 360.0F;
    }

    desiredCourse = ConvertNavtoRad(desiredCourse);


    // Draw Arrow
    OTWDriver.renderer->AdjustRotationAboutOrigin(-desiredCourse);

    // BOOL toFromFlag = mpHsi->GetFlag(CPHsi::HSI_FLAG_TO_TRUE);

    DrawToFrom();

    DWORD colorCourse = OTWDriver.pCockpitManager->ApplyLighting(mColor[0][HSI_COLOR_COURSE], true);
    OTWDriver.renderer->SetColor(colorCourse);

    OTWDriver.renderer->Tri(courseArrow[1][0], courseArrow[1][1], courseArrow[1][0], -courseArrow[1][1],
                            courseArrow[0][0], courseArrow[0][1]);
    OTWDriver.renderer->Tri(courseArrow[3][0], courseArrow[3][1], courseArrow[2][0], -courseArrow[2][1],
                            courseArrow[2][0], courseArrow[2][1]);
    OTWDriver.renderer->Tri(courseArrow[3][0], courseArrow[3][1], courseArrow[3][0], -courseArrow[3][1],
                            courseArrow[2][0], -courseArrow[2][1]);

    // Draw Tail
    OTWDriver.renderer->Tri(courseTail[0][0], courseTail[0][1], courseTail[1][0], courseTail[1][1],
                            courseTail[1][0], -courseTail[1][1]);
    OTWDriver.renderer->Tri(courseTail[0][0], courseTail[0][1], courseTail[1][0], -courseTail[1][1],
                            courseTail[0][0], -courseTail[0][1]);

    DWORD colorCircle = OTWDriver.pCockpitManager->ApplyLighting(mColor[0][HSI_COLOR_CIRCLES], true);
    OTWDriver.renderer->SetColor(colorCircle);

    // Draw Scale start drawing the course
    // deviation scale from bottom circle,
    // draw four equally spaced circles
    startPoint = -2.0F * courseDevScale[1];

    for (i = 0; i < 2; i++)
    {
        OTWDriver.renderer->Circle(courseDevScale[0], -startPoint, courseDevScale[2]);
        OTWDriver.renderer->Circle(courseDevScale[0], startPoint, courseDevScale[2]);
        startPoint += courseDevScale[1];
    }

    // JPO - normalise to account for flying away.
    if (deviaiton > 90) deviaiton = 180 - deviaiton;

    if (deviaiton < -90) deviaiton = - (180 + deviaiton);

    // If the deviaion is > 10% or < -10%, the bar will be pinned
    if (deviaiton > deviationLimit)
    {
        startPoint = 2.0F * courseDevScale[1];
    }
    else if (deviaiton < -deviationLimit)
    {
        startPoint = -2.0F * courseDevScale[1];
    }
    else
    {
        startPoint = deviaiton / halfDeviationLimit * courseDevScale[1];
    }

    mlSinCos(&trig, HALFPI + desiredCourse);
    devBarCenter[0] = startPoint * trig.cos;
    devBarCenter[1] = startPoint * trig.sin;

    MoveToCompassCenter();

    OTWDriver.renderer->AdjustOriginInViewport(devBarCenter[0], devBarCenter[1]);
    OTWDriver.renderer->AdjustRotationAboutOrigin(-desiredCourse);

    // Draw the course dev bar
    DWORD colorDevBar = OTWDriver.pCockpitManager->ApplyLighting(mColor[0][HSI_COLOR_DEVBAR], true);
    OTWDriver.renderer->SetColor(colorDevBar);
    OTWDriver.renderer->Tri(courseDevBar[0][0], courseDevBar[0][1], courseDevBar[1][0], courseDevBar[1][1],
                            courseDevBar[1][0], -courseDevBar[1][1]);
    OTWDriver.renderer->Tri(courseDevBar[0][0], courseDevBar[0][1], courseDevBar[1][0], -courseDevBar[1][1],
                            courseDevBar[0][0], -courseDevBar[0][1]);


    if (ilsWarnFlag) // and (gNavigationSys->GetInstrumentMode() == NavigationSystem::ILS_TACAN or gNavigationSys->GetInstrumentMode() == NavigationSystem::ILS_NAV)) {
    {

        r = (float) sqrt(courseDevScale[1] * courseDevScale[1] + 0.2f * 0.2f);
        theta = (float)atan2(courseDevScale[1], 0.2F);
        MoveToCompassCenter();
        mlSinCos(&trig, theta + desiredCourse);
        OTWDriver.renderer->AdjustOriginInViewport(r * trig.cos, r * trig.sin);
        OTWDriver.renderer->AdjustRotationAboutOrigin(-desiredCourse);

        DWORD colorWarning = OTWDriver.pCockpitManager->ApplyLighting(mColor[0][HSI_COLOR_ILSDEVWARN], true);
        OTWDriver.renderer->SetColor(colorWarning);
        OTWDriver.renderer->Tri(courseDevWarn[0], courseDevWarn[1], -courseDevWarn[0], courseDevWarn[1],
                                -courseDevWarn[0], -courseDevWarn[1]);
        OTWDriver.renderer->Tri(courseDevWarn[0], courseDevWarn[1], -courseDevWarn[0], -courseDevWarn[1],
                                courseDevWarn[0], -courseDevWarn[1]);
    }
}

void CPHsiView::MoveToCompassCenter(void)
{
    OTWDriver.renderer->CenterOriginInViewport();
    OTWDriver.renderer->ZeroRotationAboutOrigin();
    OTWDriver.renderer->AdjustOriginInViewport(COMPASS_X_CENTER, COMPASS_Y_CENTER);
}

void CPHsiView::DrawAircraftSymbol(void)
{

    static const float aircraftSymbol[10][2] =
    {
        {0.017F, 0.133F},
        {0.133F, 0.017F},
        {0.067F, -0.133F},
        {0.067F, -0.158F},
        {0.017F, -0.2F},
    };


    MoveToCompassCenter();

    //transform the color using cockpit light
    DWORD color = OTWDriver.pCockpitManager->ApplyLighting(mColor[0][HSI_COLOR_AIRCRAFT], true);
    OTWDriver.renderer->SetColor(color);

    // draw the aircraft symbol
    OTWDriver.renderer->Tri(-aircraftSymbol[0][0], aircraftSymbol[0][1], aircraftSymbol[0][0], aircraftSymbol[0][1],
                            aircraftSymbol[4][0], aircraftSymbol[4][1]);
    OTWDriver.renderer->Tri(-aircraftSymbol[0][0], aircraftSymbol[0][1], aircraftSymbol[4][0], aircraftSymbol[4][1],
                            -aircraftSymbol[4][0], aircraftSymbol[4][1]);
    OTWDriver.renderer->Tri(-aircraftSymbol[1][0], aircraftSymbol[1][1], aircraftSymbol[1][0], aircraftSymbol[1][1],
                            aircraftSymbol[1][0], -aircraftSymbol[1][1]);
    OTWDriver.renderer->Tri(-aircraftSymbol[1][0], aircraftSymbol[1][1], aircraftSymbol[1][0], -aircraftSymbol[1][1],
                            -aircraftSymbol[1][0], -aircraftSymbol[1][1]);
    OTWDriver.renderer->Tri(-aircraftSymbol[2][0], aircraftSymbol[2][1], aircraftSymbol[2][0], aircraftSymbol[2][1],
                            aircraftSymbol[3][0], aircraftSymbol[3][1]);
    OTWDriver.renderer->Tri(-aircraftSymbol[2][0], aircraftSymbol[2][1], aircraftSymbol[3][0], aircraftSymbol[3][1],
                            -aircraftSymbol[3][0], aircraftSymbol[3][1]);
}

//Wombat778 3-24-04 Additional function for rendering the image.
// Discardlit is not necessary because the CPObject one is fine.
void CPHsiView::CreateLit(void)
{
    if (DisplayOptions.bRender2DCockpit)
    {
        try
        {
            const DWORD dwMaxTextureWidth =
                mpOTWImage->GetDisplayDevice()->GetDefaultRC()->m_pD3DHWDeviceDesc->dwMaxTextureWidth;
            const DWORD dwMaxTextureHeight =
                mpOTWImage->GetDisplayDevice()->GetDefaultRC()->m_pD3DHWDeviceDesc->dwMaxTextureHeight;
            m_pPalette = new PaletteHandle(mpOTWImage->GetDisplayDevice()->GetDefaultRC()->m_pDD, 32, 256);

            if ( not m_pPalette)
            {
                throw _com_error(E_OUTOFMEMORY);
            }

            // Check if we can use a single texture
            if (
                (dwMaxTextureWidth >= (unsigned int)mCompassWidth) and 
                (dwMaxTextureHeight >= (unsigned int)mCompassHeight)
            )
            {
                TextureHandle *pTex = new TextureHandle;

                if ( not pTex)
                {
                    throw _com_error(E_OUTOFMEMORY);
                }

                m_pPalette->AttachToTexture(pTex);

                if ( not pTex->Create("CPHsi", MPR_TI_PALETTE bitor MPR_TI_CHROMAKEY, 8, mCompassWidth, mCompassHeight))
                {
                    throw _com_error(E_FAIL);
                }

                if ( not pTex->Load(0, 0xFFFF0000, (BYTE*)mpSourceBuffer, true, true))
                {
                    // soon to be re-loaded by CPSurface::Translate3D
                    throw _com_error(E_FAIL);
                }

                m_arrTex.push_back(pTex);
            }
        }
        catch (_com_error e)
        {
            MonoPrint("CPHsi::CreateHsiView - Error 0x%X (%s)\n", e.Error(), e.ErrorMessage());
            DiscardLit();
        }
    }
}

//Wombat778 end
