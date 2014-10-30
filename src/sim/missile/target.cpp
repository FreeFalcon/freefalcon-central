#include "Graphics/Include/drawbsp.h"
#include "stdhdr.h"
#include "sensors.h"
#include "object.h"
#include "simdrive.h"
#include "sensclas.h"
#include "campbase.h"
#include "feature.h"
#include "missile.h"
#include "classtbl.h" // JB carrier


void MissileClass::UpdateTargetData(void)
{
    // Don't come here without a sensor
    ShiAssert(sensorArray[0]);

    // Don't run the seeker until we're established in flight
    if (runTime > inputData->guidanceDelay) //ME123 ADDET targetPtr->BaseData() TO AWOID CRASHIGN HERE AFTER SYLVAINS STUFF
    {
        if (targetPtr)
        {
            ShiAssert(targetPtr->BaseData()); // We used to drop lock in this case, but it should never happen, right?

            // This ensures that everybody agrees about where ground objects are even as they move
            // with terrain LOD changes.
            /* JB 010624 Why? Setting the position like this screws up multiplayer and entitys' movement
            if (targetPtr->BaseData()->GetDomain() not_eq DOMAIN_SEA) // JB carrier (otherwise ships stop when you fire a missile at them)
            {
             if (targetPtr->BaseData()->IsSim() and 
             ((SimBaseClass*)targetPtr->BaseData())->OnGround() and 
             ((SimBaseClass*)targetPtr->BaseData())->drawPointer)
             {
             Tpoint pos;
             ((SimBaseClass*)targetPtr->BaseData())->drawPointer->GetPosition (&pos);
             ((SimBaseClass*)targetPtr->BaseData())->SetPosition (pos.x, pos.y, pos.z);
             }
            }
            */
        }

        // Run the sensor model
        RunSeeker();
    }
}


void MissileClass::SetTarget(SimObjectType* newTarget)
{
    int rdrDetect = 0;
    float irSig = 0.0F;

    if (newTarget == targetPtr)
        return;


    ShiAssert( not newTarget or newTarget->BaseData() not_eq (FalconEntity*)0xDDDDDDDD);
    ShiAssert( not targetPtr or targetPtr->BaseData() not_eq (FalconEntity*)0xDDDDDDDD);


    if (targetPtr)
    {
        targetPtr->Release();
        targetPtr = NULL;
    }


    ShiAssert( not newTarget or newTarget->BaseData() not_eq (FalconEntity*)0xDDDDDDDD);
    ShiAssert( not targetPtr or targetPtr->BaseData() not_eq (FalconEntity*)0xDDDDDDDD);


    if (newTarget)
    {
        if (launchState == Launching or launchState == InFlight)
        {
            // We need to copy the data if this missile is on it's own.
            rdrDetect = newTarget->localData->rdrDetect;
            irSig = newTarget->localData->irSignature;
#ifdef DEBUG
            //newTarget = newTarget->Copy(OBJ_TAG, this);
#else
            newTarget = newTarget->Copy();
#endif
            newTarget->localData->rdrDetect = rdrDetect;
            newTarget->localData->irSignature = irSig;
        }
    }

    ricept = FLT_MAX;

    targetPtr = newTarget;

    if (targetPtr)
    {
        targetPtr->Reference();
        // 2001-05-10 ADDED BY S.G. WITHIN THIS, I ADDED A CalcRelGeom BECAUSE localData IS SOMETIME USED BEFORE THIS IS RAN. I'M CHECKING FOR range BEEING ZERO BECAUSE THIS IS LESS LIKELY TO BE IF CalRelGeom HAS BEEN RAN BEFORE...
        // 2001-05-25 MODIFIED BY S.G. WELL, AS IT TURNS OUT, SOMETIME IT IS NEVER CALLED SO BY DOING THIS HERE EVERY TIME AS WELL (SIGHT), I MAKE SURE IT *IS* RAN
        // 2001-10-20 REMOVED BY S.G. THIS SLOWS DOWN THE AIM-9 TARGET HUNTING. I'M MOVING IT TO THE FUNCTIONS NEEDING localData BEING SET CORRECTLY.
        // if (targetPtr->BaseData()->OnGround())
        // CalcRelGeom(this, targetPtr, NULL, 1.0F / SimLibMajorFrameTime);
    }

    // If we're still on the rail, update our seeker as well -- otherwise let it do its thing
    // TODO:  Might be nice if the missile always used the seeker's target instead of keeping its own.
    if (sensorArray and launchState not_eq InFlight)
    {
        ShiAssert(sensorArray[0]);

        if (newTarget)
        {
            rdrDetect = newTarget->localData->rdrDetect;
            irSig = newTarget->localData->irSignature;
        }

        sensorArray[0]->SetDesiredTarget(newTarget);

        if (newTarget)
        {
            newTarget->localData->rdrDetect = rdrDetect;
            newTarget->localData->irSignature = irSig;
        }
    }
}


void MissileClass::DropTarget(void)
{
    if (targetPtr)
    {
        targetPtr->Release();
        targetPtr = NULL;

        // Update our seeker as well
        // TODO:  Would be nice if the missile always used the seekers target instead of keeping its own.
        // I'll probably do that when I have the missile start maintaining a real target list for the seekers
        if (sensorArray)
        {
            ShiAssert(sensorArray[0]);
            sensorArray[0]->SetDesiredTarget(NULL);
        }
    }
}
