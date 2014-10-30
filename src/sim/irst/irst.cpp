#include "stdhdr.h"
#include "simfile.h"
#include "object.h"
#include "object.h"
#include "simMover.h"
#include "SimMath.h"
#include "entity.h"
#include "Graphics/Include/display.h"
#include "irst.h"
#include "Simbase.h"
#include "missile.h"

IRSTDataType* IRSTDataTable = NULL;
short NumIRSTEntries = 0;

static const float CM_EFFECTIVE_ANGLE = 30.0f * DTR; // If used, should be in class table data... me123 status test changed from 30

IrstClass::IrstClass(int idx, SimMoverClass* self) : SensorClass(self)
{
    dataProvided = RangeAndBearing;
    typeData = &(IRSTDataTable[min(max(idx, 0), NumIRSTEntries - 1)]);
    sensorType = IRST;
    tracking = 0;
    lastFlareID = FalconNullId;
    ShiAssert(self->IsMissile());

    // TODO:  Use class table data instead of private text files
    ShiAssert(IRSTDataTable);
}


void IrstClass::SetDesiredTarget(SimObjectType* newTarget)
{
    SensorClass::SetDesiredTarget(newTarget);

    // Each time we take a new locked target, start with 0 signature
    // so the IIR filter works correctly.
    // NOTE:  This might cause a problem if we ever have a platform
    // with two IR sensors looking at the same target.
    if (newTarget)
    {
        ShiAssert(newTarget->localData);
        newTarget->localData->irSignature = 0.0f;
    }

}

SimObjectType* IrstClass::Exec(SimObjectType*)
{
    SimObjectType *newLock;

    // Validate our locked target
    CheckLockedTarget();
    newLock = lockedTarget;

    // Decide if we can still see our locked target (based on last Exec)
    if (lockedTarget)
    {
        ShiAssert(lockedTarget->IsReferenced() > 0);// JPO - trying to catch the problem child
        // Consider taking a decoy
        lockedTarget = ConsiderDecoy(lockedTarget);

        // Can't hold a lock if its outside our sensor cone
        if ( not CanSeeObject(lockedTarget))
        {
            newLock = NULL;
        }

        // Can't hold lock if the signal is too weak or blocked
        if ( not CanDetectObject(lockedTarget))
        {
            newLock = NULL;
        }
    }
    else CanSeeObject(NULL);//me123 to reset the tracking bit

    // Update our lock
    SetSensorTarget(newLock);

    // Tell the base class where we're looking
    if (lockedTarget)
    {
        // SetSeekerPos (lockedTarget->localData->az,lockedTarget->localData->el);
        // seekerAzCenter = lockedTarget->localData->az;
        // seekerElCenter = lockedTarget->localData->el;
        lockedTarget->localData->sensorState[IRST] = SensorTrack;
        lockedTarget->localData->sensorLoopCount[IRST] = SimLibElapsedTime;
    }

    return lockedTarget;
}


// This controls how effective countermeasures are as a function of seeker range from target
// 2000-11-17 MODIFIED BY S.G. SO FLARES ARE EFFECTIVE MORE REALISTICALLY
// static const float cmRangeArray[] = {0.0F,  5500.0f,  11000.0f,  16500.0f,  27500.0f};
static const float cmRangeArray[] = {0.0F,   451.0f,   4500.0f,  16500.0f,  38000.0f};
// END OF MODIFIED DATA
static const float cmBiteChanceArray[] = {0.0F,     0.0F,      1.0F,      1.0F,      0.0F};
// static const float cmBiteChanceArray[] = {1.0F,     1.0F,      1.0F,      1.0F,      1.0F};//me123 status ok. S.G. TO BRING IT TO RP4 STATUS
static const int cmArrayLength = sizeof(cmRangeArray) / sizeof(cmRangeArray[0]);


SimObjectType* IrstClass::ConsiderDecoy(SimObjectType *target)
{
    VU_ID id;
    FalconEntity *cm;
    float chance;
    int dummy = 0;

    // No counter measures deployed by campaign things
    if ( not target or not target->BaseData()->IsSim())
    {
        return target;
    }

    // Get the ID of the most recently launched counter measure from our target
    id = ((SimBaseClass*)target->BaseData())->NewestFlareID();

    // If we have a new chaff bundle to deal with
    if (id not_eq lastFlareID)
    {
        // Stop here if there isn't a counter measure in play
        if (id == FalconNullId)
        {
            lastFlareID = id;
            return target;
        }

        // Try to find the counter measure entity in the database
        cm = (FalconEntity*)vuDatabase->Find(id);

        if ( not cm)
        {
            // We'll have to wait until next time
            // (probably because the create event hasn't been processed locally yet)
            return target;
        }

        // Start with the suceptability of this seeker to counter measures
        chance = typeData->FlareChance;

        // Adjust with a range to target based chance of an individual countermeasure working
        chance *= Math.OnedInterp(target->localData->range, cmRangeArray, cmBiteChanceArray, cmArrayLength, &dummy);
        chance /= max(0.3f , target->localData->irSignature); //me123 flares works better if the irsignature on the target is low

        // Player countermeasures work better
        // if (target->BaseData()->IsPlayer()) { //me123 status ok. don't differentiate between ai/human here
        // chance *= 1.15F;
        // }
        // Marco Edit - AB reduces chance of flares working quite a bit ......
        if (target->localData->irSignature > 1.29f)
        {
            chance /= 2.0f;
        }

        // Roll the dice
        if (chance > (float)rand() / RAND_MAX)
        {

            // Compute some relative geometry stuff
            const float atx = platform->dmx[0][0];
            const float aty = platform->dmx[0][1];
            const float atz = platform->dmx[0][2];
            const float dx = cm->XPos() - platform->XPos();
            const float dy = cm->YPos() - platform->YPos();
            const float dz = cm->ZPos() - platform->ZPos();
            const float range = (float)sqrt(dx * dx + dy * dy);
            const float cosATA = (atx * dx + aty * dy + atz * dz) / (float)sqrt(range * range + dz * dz);

            // Only take the bait if we can see the thing
            if (cosATA >= cos(CM_EFFECTIVE_ANGLE))
            {
#ifdef DEBUG
                /* target = new SimObjectType(OBJ_TAG, platform, cm);*/
#else
                target = new SimObjectType(cm);
#endif
                target->localData->irSignature = 20.0;//me123
                SetSensorTarget(target); // JPO - do it now
            }
        }

        // Note that we've considered this countermeasure
        lastFlareID = id;
    }

    return target;
}
