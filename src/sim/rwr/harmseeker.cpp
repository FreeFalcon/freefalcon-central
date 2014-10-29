#include "stdhdr.h"
#include "Object.h"
#include "simbase.h"
#include "camp2sim.h"
#include "simmover.h"
#include "OTWdrive.h"
#include "Missile.h"
#include "HarmSeeker.h"
#include "simveh.h"
#include "fcc.h"


// How fast will the missile target point drift without a signal (ft/sec)
static const float MAX_DRIFT_RATE = 20.0f;


HarmSeekerClass::HarmSeekerClass(int type, SimMoverClass* parentPlatform) : RwrClass(type, parentPlatform)
{
    dataProvided = ExactPosition;

    couldGuide = TRUE;

    // Pick a random drift rate for this instance
    driftRateX = MAX_DRIFT_RATE * (float)((RAND_MAX >> 1) - rand()) / (RAND_MAX >> 1);
    driftRateY = MAX_DRIFT_RATE * (float)((RAND_MAX >> 1) - rand()) / (RAND_MAX >> 1);

    launched = false;
    launchedInPOS   = false;
    handedoff = false;
}


SimObjectType* HarmSeekerClass::Exec(SimObjectType* missileTarget)
{
    BOOL canGuide = false;
    float z;

    SimMoverClass* theParent;
    FireControlComputer* FCC;
    HarmTargetingPod* HTS;

    theParent = (SimMoverClass*)(((MissileClass*)platform)->parent.get());

    // FRB - CTD fix?
    if ( not theParent)
        return NULL;

    FCC = ((SimVehicleClass*)theParent)->GetFCC();
    HTS = (HarmTargetingPod*)FindSensor(theParent, SensorClass::HTS);

    if (HTS and ((MissileClass*)platform)->launchState not_eq MissileClass::PreLaunch and not launched)
    {
        launched = true;
        launchedInPOS = (HTS->GetPreHandoffMode() == HarmTargetingPod::Pos);
        handedoff = HTS->GetHandedoff();
    }


    // Adopt the missile's target if it is providing one and it is not the missile itself
    if (missileTarget)
    {
        if ( not platform->IsMissile() or ((MissileClass*)platform)->parent.get() not_eq missileTarget->BaseData())
            SetDesiredTarget(missileTarget);
    }

    // Validate our locked target
    CheckLockedTarget();

    // Decide if we can still guide on our locked target
    if (lockedTarget)
    {
        // RV - I-Hawk - If we are in a POS handoff, no sensor checks needed
        if (launchedInPOS)
        {
            canGuide = handedoff; // make sure target was already handedoff
        }

        else
        {
            // Can't guide if its outside our sensor cone
            if (CanSeeObject(lockedTarget))
            {
                // Can't guide if the signal is too weak or in the air
                if (CanDetectObject(lockedTarget) and lockedTarget->BaseData()->OnGround())
                {
                    canGuide = handedoff; // make sure target was already handedoff
                }
            }
        }
    }

    // If we can guide, update our data and return the target entity
    if (canGuide)
    {
        couldGuide = TRUE;

        // Tell the base class where we're looking
        seekerAzCenter = lockedTarget->localData->az;
        seekerElCenter = lockedTarget->localData->el;

        // Return the target entity for the missile to guide upon
        return lockedTarget;
    }

    // If we were guiding, but now we're not, have the missile "coast"
    if (couldGuide)
    {

        couldGuide = FALSE;

        // TODO:  Make sure the missile has a guide point even if the target dies...
        // for now...
        if (lockedTarget)
        {
            ShiAssert(platform);
            ShiAssert(platform->IsMissile());

            // Get the target's z height from the proper place (since campaign units lie)
            if (lockedTarget->BaseData()->IsSim())
            {
                z = lockedTarget->BaseData()->ZPos();
            }
            else
            {
                z = OTWDriver.GetGroundLevel(lockedTarget->BaseData()->XPos(), lockedTarget->BaseData()->YPos());
            }

            // Tell our parent missile where the target was when last seen
            ((MissileClass*)platform)->SetTargetPosition(lockedTarget->BaseData()->XPos(),
                    lockedTarget->BaseData()->YPos(),
                    z);

            // Give it a bogus speed so the target location drifts while the signal is lost
            ((MissileClass*)platform)->SetTargetVelocity(driftRateX, driftRateY, 0.0f);
        }
    }

    return NULL; // Indicate that we can't see our target
}
