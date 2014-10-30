#include "stdhdr.h"
#include "sensclas.h"
#include "object.h"
#include "camp2sim.h"
#include "radarMissile.h"
#include "missile.h"
#include "simobj.h"
#include "simdrive.h"
#include "sms.h"
#include "aircrft.h"
#include "MsgInc/RadioChatterMsg.h"
#include "flight.h" // Marco edit for 'Pitbull' call


//bool Pitbull = true ;

// sfr new runseekr function, fixes some leaks, using targetList instead of targetPtr
#define NEW_RUNSEEKER 1
#if NEW_RUNSEEKER
void MissileClass::RunSeeker()
{
    SimObjectType *lockedTarget;

    // Don't come here without a sensor
    // Shouldn't be necessary, but at this stage, lets be safe...
    // No seeker if in SAFE
    if (
 not sensorArray or not sensorArray[0] or
        launchState == PreLaunch and parent and parent->IsAirplane() and 
        ((AircraftClass*)parent.get())->Sms->MasterArm() == SMSBaseClass::Safe
    )
    {
        return;
    }

    // START_PROFILE("SEEKERS");
    // TODO:  This seems to do extra work to generate a matrix (vmat).
    // would it be worth a special CalcRelGeom call that avoids this???
    if (launchState == InFlight)
    {
        if (targetPtr)
        {
            // we have a target
            CalcRelGeom(this, targetPtr, NULL, 1.0F / SimLibMajorFrameTime);
        }
        else
        {
            // we dont have a target, create/update the target list
            targetList = UpdateTargetList(targetList, this, SimDriver.combinedList);
            CalcRelGeom(this, targetList, NULL, 1.0F / SimLibMajorFrameTime);
        }
    }

    // see if its time to go active
    float factor = ((targetPtr and targetPtr->BaseData()->IsSPJamming()) ? 1.5f : 1.0f);

    if (
        inputData->mslActiveTtg > 0 and 
        (
            timpct * factor < inputData->mslActiveTtg and 
            sensorArray[0]->Type() not_eq SensorClass::Radar
        ) or
        (
            launchState == InFlight and sensorArray[0]->Type() not_eq SensorClass::Radar and 
            ( not isSlave or not targetPtr) //I-Hawk - was missing the parentheses here, caused heat seeker locking problems
        )
    )
    {
        GoActive();
        // sfr shouldnt we finish here?
    }

    // exec the sensor and get locked target
    lockedTarget = sensorArray[0]->Exec(targetList);

    // Make sure we don't target ourselves
    if (lockedTarget and lockedTarget->BaseData() == parent)
    {
        lockedTarget = NULL;
    }

    // Deal with the case where the seeker lost lock on the intended target
    if (lockedTarget not_eq targetPtr)
    {
        if (
            (inputData->mslActiveTtg > 0) and 
            (sensorArray[0]->Type() not_eq SensorClass::Radar) and 
            launchState == InFlight
        )
        {
            // We can switch from passive to active guidance - go active if passive fails
            GoActive();
            // sfr: shouldnt we finish here
        }
        else if (targetPtr and targetPtr->localData->range * targetPtr->localData->range <= lethalRadiusSqrd)
        {
            // We were close enough to detonate, so do it
            flags or_eq ClosestApprch;
        }
        else
        {
            // Record that we lost lock on our original target
            flags or_eq SensorLostLock;

            // Update relative geometry on the new target (if any)
            if (lockedTarget)
            {
                SimObjectType* newNext = lockedTarget->next;
                lockedTarget->next = NULL;
                CalcRelGeom(this, lockedTarget, NULL, 1.0F / SimLibMajorFrameTime);
                lockedTarget->next = newNext;
            }
            // 2000-08-31 ADDED BY S.G. WHEN LOCK LOST, HAVE THE MISSILE SEEKER POINT WHERE IT WAS LAST SEEN
            else if (targetPtr)
            {
                SetRdrAzCenter(targetPtr->localData->az);
                SetRdrElCenter(targetPtr->localData->el);
            }
            else
            {
                SetRdrAzCenter(0.0f);
                SetRdrElCenter(0.0f);
            }

            // END OF ADDED SECTION
            // Take the new target
            SetTarget(lockedTarget);
        }
    }

    //STOP_PROFILE("SEEKERS");
}
#else
void MissileClass::RunSeeker()
{
    // COBRA - RED - To dentify locally created target lists
    bool localList = false;

    SimObjectType *newTarget = targetPtr, *lockedTarget = NULL;

    // Don't come here without a sensor
    ShiAssert(sensorArray);

    if ( not sensorArray) return; // Shouldn't be necessary, but at this stage, lets be safe...

    // Don't come here without a sensor
    ShiAssert(sensorArray[0]);

    if ( not sensorArray[0]) return; // Shouldn't be necessary, but at this stage, lets be safe...

    // No seeker if in SAFE
    if (
        launchState == PreLaunch and parent and parent->IsAirplane() and 
        ((AircraftClass*)parent.get())->Sms->MasterArm() == SMSBaseClass::Safe
    )
    {
        return;
    }

    // START_PROFILE("SEEKERS");
    // TODO:  This seems to do extra work to generate a matrix (vmat).
    // would it be worth a special CalcRelGeom call that avoids this???
    if (launchState == InFlight)
    {
        if (targetPtr)
        {
            CalcRelGeom(this, targetPtr, NULL, 1.0F / SimLibMajorFrameTime);
        }
        else
        {
            // COBRA - RED - OK, We r creating a local list
            localList = true;
            newTarget = UpdateTargetList(NULL, this, SimDriver.combinedList);
            CalcRelGeom(this, newTarget, NULL, 1.0F / SimLibMajorFrameTime);
        }
    }

    // For missiles that go active (AMRAAM) is it time to switch seekers?
    ShiAssert(timpct >= 0.0f); // If this fails, we might try to give ANY missile an active radar
    // KLUDGE: 2000-08-26 MODIFIED BY S.G. SO ARH MISSILE WAIT LONGER WHEN THE TARGET IS JAMMING
    // IT DOESN'T MATTER IF targetPtr IS NULL AND newTarget IS SET BECAUSE GoActive
    // WILL BE DONE LATER IN THE ROUTINE ANYHOW IN SUCH A SCENARIO
    // if ((timpct < inputData->mslActiveTtg) and (sensorArray[0]->Type() not_eq SensorClass::Radar))

    // ME123 WE GET A CTD HERE IF NOT MAKING THE ARH CHECK FIRST 
    // if (timpct *
    // ((targetPtr and targetPtr->BaseData()->IsSPJamming()) ? 1.5f : 1.0f) <
    // inputData->mslActiveTtg and sensorArray[0]->Type() not_eq SensorClass::Radar)
    // Marco edit - Also added in GoActive for 'Mad Dog' Launches
    //if (
    // inputData->mslActiveTtg > 0 and 
    // launchState == InFlight and 
    // sensorArray[0]->Type() not_eq SensorClass::Radar and 
    // not isSlave
    //){
    // Pitbull = false;
    //}

    float factor = ((targetPtr and targetPtr->BaseData()->IsSPJamming()) ? 1.5f : 1.0f);

    if (
        inputData->mslActiveTtg > 0 and 
        (
            timpct * factor < inputData->mslActiveTtg and 
            sensorArray[0]->Type() not_eq SensorClass::Radar
        ) or
        (
            inputData->mslActiveTtg > 0 and 
            launchState == InFlight and sensorArray[0]->Type() not_eq SensorClass::Radar and 
            ( not isSlave or not targetPtr)
        )
    )
    {
        GoActive();
    }


    // COBRA - RED - The memory Leakage Correction
    // **************************************** OLD CODE *****************************
    // newTarget = sensorArray[0]->Exec( newTarget );
    // ************************************* END OLD CODE ****************************
    // Exec the seeker
    // **************************************** NEW CODE *****************************
    // We r getting the locked target in a new variable avoiding loosing the target list
    // reference if the ssensor is not locked ( Exec() returns NULL )
    lockedTarget = sensorArray[0]->Exec(newTarget);
    // if NO LOCK and the list is created on flight ( i.e. Not the FCC List ), the created list is deleted
    // for such purpose I created ReleaseTargetList() function in 'simObj.cpp'
    // sfr: this prolly fix leak
#define ALWAYS_RELEASE_LOCAL_LIST 1
#if ALWAYS_RELEASE_LOCAL_LIST

    if (localList/* and ( not lockedTarget)*/)
    {
        ReleaseTargetList(newTarget);
        targetPtr = NULL;
    }

#else

    if (localList and ( not lockedTarget))
    {
        ReleaseTargetList(newTarget);
        targetPtr = NULL;
    }

#endif
    // Finally, the new Target is updated with whatever is locked by sensor
    newTarget = lockedTarget;
    // ********************************** NEW CODE - END *****************************

    // Make sure we don't target ourselves
    if (newTarget and newTarget->BaseData() == parent)
    {
#if ALWAYS_RELEASE_LOCAL_LIST
        // sfr: should we release targets??
        ReleaseTargetList(newTarget);
#endif
        newTarget = NULL;
    }

    // Deal with the case where the seeker lost lock on the intended target
    if (newTarget not_eq targetPtr)
    {
        if (
            (inputData->mslActiveTtg > 0) and 
            (sensorArray[0]->Type() not_eq SensorClass::Radar) and 
            launchState == InFlight
        )
        {
            // We can switch from passive to active guidance - go active if passive fails
            GoActive();
        }
        else if (targetPtr and targetPtr->localData->range * targetPtr->localData->range <= lethalRadiusSqrd)
        {
            // We were close enough to detonate, so do it
            flags or_eq ClosestApprch;
        }
        else
        {
            // Record that we lost lock on our original target
            flags or_eq SensorLostLock;

            // Update relative geometry on the new target (if any)
            if (newTarget)
            {
                SimObjectType* newNext = newTarget->next;
                newTarget->next = NULL;
                CalcRelGeom(this, newTarget, NULL, 1.0F / SimLibMajorFrameTime);
                newTarget->next = newNext;
            }
            // 2000-08-31 ADDED BY S.G. WHEN LOCK LOST, HAVE THE MISSILE SEEKER POINT WHERE IT WAS LAST SEEN
            else if (targetPtr)
            {
                SetRdrAzCenter(targetPtr->localData->az);
                SetRdrElCenter(targetPtr->localData->el);
            }
            else
            {
                SetRdrAzCenter(0.0f);
                SetRdrElCenter(0.0f);
            }

            // END OF ADDED SECTION
            // Take the new target
            SetTarget(newTarget);
        }
    }


    // TODO:  Kill this.  Only used by LimitSeeker below
    // sfr: ok, killed.
#if 0

    if (targetPtr)
    {
        //me123 check if the missile seekerhead can track
        //SetSeekerPos(&targetPtr->localData->az,&targetPtr->localData->el);
        ata = targetPtr->localData->ata;

        if (
            ata > inputData->gimlim or
            fabs(sensorArray[0]->SeekerAz() - targetPtr->localData->az) > inputData->atamax or
            fabs(sensorArray[0]->SeekerEl() - targetPtr->localData->el) > inputData->atamax
        )
        {
            //flags or_eq SensorLostLock;
            //SetTarget( NULL );
        }
    }

#endif
    //STOP_PROFILE("SEEKERS");
}
#endif

// Convert from unspecified passive to active radar guidance
void MissileClass::GoActive(void)
{
    // Can't go active is we're already active
    ShiAssert(sensorArray[0]->Type() not_eq SensorClass::Radar)

    // Get rid of the old sensor
    sensorArray[0]->SetPower(FALSE);
    delete(sensorArray[0]);

    wentActive = true; // MN this is currently only used for debug labels

    // Construct and initialize the new active radar sensor
    ShiAssert(GetRadarType()); // Type 0 is RDR_NO_RADAR -- we'd better not be going active without a radar...
    sensorArray[0] = new RadarMissileClass(GetRadarType(), this);
    ShiAssert(sensorArray[0]);
    sensorArray[0]->SetDesiredTarget(targetPtr);
}


// TODO:  This is still used in a few places (indirectly by the FCC, I think)
// to present sensor limits to the player (for Mavericks only I think).
// The missile itself ignores this stuff in favor of the sensors knowing their
// own limits.  This should go.  SCR  10/9/98
void MissileClass::LimitSeeker(float az, float el)
{
    /*-----------------*/
    /* Local variables */
    /*-----------------*/
    float gimbalCmd;        /* Gimbal command */
    float azCmd, elCmd;

    //  if (inputData->seekerType not_eq SensorClass::RadarHoming)
    {
        /*     if ( targetPtr == NULL)// launchState == PreLaunch or
             {
                azCmd = sensorArray[0]->SeekerAz();
                elCmd = sensorArray[0]->SeekerEl();
                gimbalCmd = (float)acos (cos (azCmd) * cos (elCmd));
                ata = gimbalCmd;
             }
             else*/
        {
            /*---------------------------*/
            /* antenna train angle limit */
            /*---------------------------*/
            if (launchState == InFlight and runTime <= inputData->guidanceDelay)
            {
                gimbalCmd = ata;
                azCmd = 0.0F;
                elCmd = 0.0F;
            }
            else
            {
                gimbalCmd = Math.RateLimit(ata, gimbal,
                                           inputData->gmdmax, &gimdot, SimLibMinorFrameTime);

                azCmd = Math.RateLimit(az, sensorArray[0]->SeekerAz(),
                                       inputData->gmdmax, &gimdot, SimLibMinorFrameTime);
                elCmd = Math.RateLimit(el, sensorArray[0]->SeekerEl(),
                                       inputData->gmdmax, &gimdot, SimLibMinorFrameTime);
            }
        }

        gimbal = min(max(gimbalCmd, -inputData->gimlim), inputData->gimlim);
        sensorArray[0]->SetSeekerPos(
            min(max(azCmd, -inputData->gimlim), inputData->gimlim),
            min(max(elCmd, -inputData->gimlim), inputData->gimlim));
        ataerr = ata - gimbal;
    }
}
