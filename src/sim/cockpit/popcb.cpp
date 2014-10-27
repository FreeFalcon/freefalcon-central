#include "stdafx.h"
#include "popcbproto.h"
#include "wingorder.h"
#include "simdrive.h"

MenuCallback MenuCallbackArray[TOTAL_POPCALLBACK_SLOTS] =
{
    CBPopTestFalse,
    CBPopTestTrue,
    CBTestForTarget,
    CBCheckExtent,
    CBTestTwoShip,
    CBTestOnGround,
    CBTestAWACS,
    CBTestNotOnGround,
    NULL,
    NULL
};


BOOL CBCheckExtent(int callerIdx, int numInFlight, int extent, BOOL isPolling, VU_ID tgtId)
{
    BOOL retVal = FALSE;

    switch (callerIdx)
    {
        case AiFlightLead:

            if (numInFlight > 2 or (numInFlight == 2 and extent == AiWingman))
            {
                retVal = TRUE;
            }

            break;

        case AiElementLead:

            if (numInFlight == 4 and extent == AiWingman)
            {
                retVal = TRUE;
            }

            break;

        default:
            break;
    }

    return retVal;
}

BOOL CBPopTestTrue(int callerIdx, int numInFlight, int extent, BOOL isPolling, VU_ID tgtId)
{
    return TRUE;
}

BOOL CBPopTestFalse(int callerIdx, int numInFlight, int extent, BOOL isPolling, VU_ID tgtId)
{
    return FALSE;
}


BOOL CBTestForTarget(int callerIdx, int numInFlight, int extent, BOOL isPolling, VU_ID tgtId)
{
    BOOL retVal = FALSE;

    if (CBCheckExtent(callerIdx, numInFlight, extent, isPolling, tgtId))
    {
        if (tgtId not_eq FalconNullId)
        {
            retVal = TRUE;
        }
    }

    return retVal;
}


BOOL CBTestTwoShip(int callerIdx, int numInFlight, int extent, BOOL isPolling, VU_ID tgtId)
{
    BOOL retVal = FALSE;

    if (CBCheckExtent(callerIdx, numInFlight, extent, isPolling, tgtId))
    {
        if (numInFlight > 2)
        {
            retVal = TRUE;
        }
    }

    return retVal;
}

BOOL CBTestOnGround(int, int, int, BOOL, VU_ID)
{
    return false;
}

BOOL CBTestAWACS(int, int, int, BOOL, VU_ID)
{
    return false;
}

BOOL CBTestNotOnGround(int, int, int, BOOL, VU_ID)
{
    return false;
}
