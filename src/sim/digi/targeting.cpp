#include "stdhdr.h"
#include "digi.h"
#include "aircrft.h"
#include "unit.h"
#include "sensors.h"
#include "object.h"
#include "Graphics/Include/drawbsp.h"
#include "simobj.h"
#include "simdrive.h"
#include "sms.h"
/* S.G. */
#include "vehrwr.h"
/* S.G. */
#include "RadarDigi.h"
/* S.G. */
#include "visual.h"
/* S.G. */
#include "irst.h"
/* S.G. FOR UP TO TWO MISSILES ON ITS WAY TO A TARGET */
#include "aircrft.h"
/* S.G. FOR UP TO TWO MISSILES ON ITS WAY TO A TARGET */
#include "airframe.h"
/* S.G. FOR UP TO TWO MISSILES ON ITS WAY TO A TARGET */
#include "simWeapn.h"

#include "wingorder.h"//Cobra

extern int g_nLowestSkillForGCI; // 2002-03-12 S.G. Replaces the hardcoded '3' for skill test
extern float g_fSearchSimTargetFromRangeSqr; // 2002-03-15 S.G. Will lookup Sim target instead of using the campain target from this range

SimObjectType* MakeSimListFromVuList(AircraftClass *self, SimObjectType* targetList, VuFilteredList* vuList);
FalconEntity* SpikeCheck(AircraftClass* self, FalconEntity *byHim = NULL, int *data = NULL); // 2002-02-10 S.G.

void DigitalBrain::DoTargeting(void)
{
    FalconEntity* campTarget = NULL;
    float rngSqr = FLT_MAX;

    // Use parent target list for start
    targetList = self->targetList;

    /*-----------------------------*/
    /* Calculate relative geometry */
    /*-----------------------------*/
    if (targetPtr)
    {
        targetData = targetPtr->localData;
        ataDot  = (targetData->ata - lastAta) / SimLibMajorFrameTime;
        lastAta   = targetData->ata;
    }
    else
    {
        targetData = NULL;
    }

    // only flight leads update their target lists and scan.
    // furthermore they only do this when in waypoint mode and
    // we're in not in campaign or tactical
    // When they decide to engage, they'll fix a target (or flight)
    // and then begin continuous monitoring of that target only in
    // an engagement mode.  This will occur up until the engagement is
    // broken (or we die in engagement).  Returning to waypoint mode
    // will return to scanning.

    // Cobra - Not being set from human's 0 after going digi with CAP AP
    if (self->targetUpdateRate == 0)
        self->targetUpdateRate = (5 * SEC_TO_MSEC);

    if (SimLibElapsedTime > self->nextTargetUpdate)
    {
        // If we are lead, or lead is player, look for a target
        if ( not isWing or (mDesignatedObject == FalconNullId and flightLead and flightLead->IsSetFlag(MOTION_OWNSHIP)))
        {
            if (missionClass not_eq AAMission and not missionComplete and agDoctrine == AGD_NONE)
                SelectGroundWeapon();

            campTarget = CampTargetSelection();

            if (campTarget)
            {
                // Use campaign target
                if (campTarget->IsCampaign() and ((CampBaseClass*)campTarget)->GetComponents())
                {
                    self->targetList = MakeSimListFromVuList(self, self->targetList, ((CampBaseClass*)campTarget)->GetComponents());
                }
                // 2002-02-25 MODIFIED BY S.G. NO NO NO, AGGREGATED Campaign object should make it here as well otherwise AI will not target them until they enter the 20 NM limit below.
                // Campaign returned a sim entity, deal with it
                //          else if (campTarget->IsSim())
                else if (campTarget->IsSim() or (campTarget->IsCampaign() and ((CampBaseClass *)campTarget)->IsAggregate()))
                {
                    // Put it directly into our target list
                    SimObjectType *newTarg = new SimObjectType(campTarget);
                    self->targetList = InsertIntoTargetList(self->targetList, newTarg);
                }

                rngSqr = (campTarget->XPos() - self->XPos()) * (campTarget->XPos() - self->XPos()) +
                         (campTarget->YPos() - self->YPos()) * (campTarget->YPos() - self->YPos());
            }

            // If the campaign didn't give us a target or we're so close that campaign targeting isn't
            // reliable, then build our own target list.
            // TODO:  We should really put both the campaign suggested target and the local environment
            // objects into our target list and then make a weighted choice among all of the entities.
            // 2000-11-17 MODIFIED BY S.G. WILL TRY DIFFERENT VALUES AND SEE HOW IT AFFECTS GAMEPLAY/FPS
            //(5 NM IS THE DEFAULT). 20 NM SEEMS FINE (NO REAL FPS LOSS AND IMPROVED AI TARGET SORTING)
            //       if ( not campTarget or rngSqr < (5.0F * NM_TO_FT)*(5.0F * NM_TO_FT))
            // SYLVAINLOOK in your RP5 code, this was brought back to 5 NM...
            // 2002-03-15 MODIFIED BY S.G. Now uses g_fSearchSimTargetFromRangeSqr so we can tweak it
            // TJL 10/20/03 SearchSim is missing from F4config. Uncommented hard code and changed it to 8.0 miles.
            //if ( not campTarget or rngSqr < (20.0F * NM_TO_FT)*(20.0F * NM_TO_FT))
            //if ( not campTarget or rngSqr < (8.0F * NM_TO_FT)*(8.0F * NM_TO_FT))
            if ( not campTarget or rngSqr < g_fSearchSimTargetFromRangeSqr)
            {
                // Need a target list for threat checking
                self->targetList = UpdateTargetList(self->targetList, self, SimDriver.combinedList);
            }
        }
        else //wingman
        {
            if (mDesignatedObject not_eq FalconNullId)
            {
                campTarget = (FalconEntity*) vuDatabase->Find(mDesignatedObject); // Lookup target in database

                if (campTarget and campTarget->IsCampaign() and ((CampBaseClass*)campTarget)->GetComponents())
                {
                    self->targetList = MakeSimListFromVuList(self, self->targetList, ((CampBaseClass*)campTarget)->GetComponents());
                }
                else if (campTarget and campTarget->IsSim())
                {
                    // Put it directly into our target list
                    SimObjectType *newTarg = new SimObjectType(campTarget);
                    self->targetList = InsertIntoTargetList(self->targetList, newTarg);
                }
            }

            // Check for nearby threats and kill them
            // TODO:  Should we do a range check here like we do for leads above, or just trust the campaign?
            if ( not campTarget)
            {
                //me123 we need it for bvr reactions      if (missionClass == AAMission or missionComplete)
                self->targetList = UpdateTargetList(self->targetList, self, SimDriver.combinedList);
            }

            // edg: kruft check -- it has been observed that wingman's target
            // lists are holding refs to sim objects that are no longer awake.
            // This will remove them.
            SimObjectType *simobj = self->targetList;
            SimObjectType *tmpobj;

            while (simobj)
            {
                if (simobj->BaseData()->IsSim() and not ((SimBaseClass*)simobj->BaseData())->IsAwake())
                {
                    tmpobj = simobj->next;

                    // remove from chain
                    if (simobj->prev)
                        simobj->prev->next = simobj->next;
                    else
                        self->targetList = simobj->next;

                    if (simobj->next)
                        simobj->next->prev = simobj->prev;

                    simobj->next = NULL;
                    simobj->prev = NULL;
                    simobj->Release();
                    simobj = tmpobj;

                }
                else
                {
                    simobj = simobj->next;
                }
            }
        }

        //Don't go here if no Targetlist (nothing happens and it's a waste
        //Cobra
        if (self->targetList)
            CalcRelGeom(self, self->targetList, ((AircraftClass*)self)->vmat, 1.0F / SimLibMajorFrameTime);

        targetList = self->targetList;
        // Sensors
        ((AircraftClass *)self)->RunSensors();
        self->nextTargetUpdate = SimLibElapsedTime + self->targetUpdateRate;

        // This is a timed event, so lets check gas here
        FuelCheck();

        // Check for reaching IP
        if ( not IsSetATC(ReachedIP))
            IPCheck();
    }

    // Merge the data for each threat/target
    SensorFusion();
}

void DigitalBrain::TargetSelection(void)
{
    //float threatTime = MAX_THREAT_TIME - 1;
    //float targetTime = MAX_TARGET_TIME - 1;
    int baseScore = 0;

    FalconEntity* curSpike = NULL;
    SimObjectType* objectPtr;
    SimObjectType* maxTargetPtr[2];
    SimObjectType* maxThreatPtr;
    BOOL foundSpike = FALSE;
    RadarClass* theRadar = (RadarClass*)FindSensor(self, SensorClass::Radar);

    // stay on current target
    if (targetPtr and (
            targetPtr->BaseData()->IsExploding() or targetPtr->BaseData()->IsDead() or
            (
                targetPtr->BaseData()->IsAirplane() and 
                ((AircraftClass*)targetPtr->BaseData())->IsAcStatusBitsSet(
                    AircraftClass::ACSTATUS_PILOT_EJECTED
                )
            )
        )
       )
    {
        ClearTarget();
    }

    maxTargetPtr[0] = NULL;
    maxTargetPtr[1] = NULL;
    maxThreatPtr = NULL;

    curSpike = SpikeCheck(self);

    objectPtr = targetList;

    // sfr: removed JB check
    // and not F4IsBadReadPtr(objectPtr, sizeof(SimObjectType))) // JB 010224 CTD
    while (objectPtr)
    {
        FalconEntity *baseData = objectPtr->BaseData();

        if (
            (baseData == NULL) or (baseData->VuState() not_eq VU_MEM_ACTIVE) or
            //F4IsBadCodePtr((FARPROC) objectPtr->BaseData()) or // JB 010224 CTD
            baseData->IsSim() and (
                baseData->IsWeapon() or baseData->IsEject() or (
                    baseData->IsAirplane() and ((AircraftClass*)baseData)->IsAcStatusBitsSet(
                        AircraftClass::ACSTATUS_PILOT_EJECTED
                    )
                )
            ) or
            (objectPtr->localData->range > maxEngageRange)
        )
        {
            objectPtr = objectPtr->next;
            continue;
        }

        // Cobra add this to clear out dead missiles?
        if (
            baseData->IsSim() and 
            ((SimBaseClass *)baseData)->incomingMissile[0] and 
            ((SimBaseClass *)baseData)->incomingMissile[0]->IsDead()
        )
        {
            ((SimBaseClass *)baseData)->incomingMissile[0] = NULL;
        }

        // S.G.ADDED SECTION. MAKE SUR OUR SENSOR IS SEEING HIM BEFORE WE DO ANYTHING WITH HIM...
        // WILL HOLD THE LOCKED OBJECT IF THE SENSOR COULD LOCK IT
        SimObjectType* tmpLock = NULL;

        for (int i = 0; i < self->numSensors; i++)
        {
            ShiAssert(self->sensorArray[i]);

            if (objectPtr->localData->sensorState[self->sensorArray[i]->Type()] > SensorClass::NoTrack)
            {
                // CAN SEE AND DETECT IT
                tmpLock = objectPtr;
                break;
            }
        }

        if ( not tmpLock)
        {
            // IF NO SENSOR IS SEEING THIS GUY, HOW CAN WE TRACK HIM?
            // 2001-03-16 ADDED BY S.G. THIS IS OUR GCI IMPLEMENTATION...
            // EVEN IF NO SENSORS ON HIM, ACE AND VETERAN GETS TO USE GCI
            if (/*SkillLevel() < g_nLowestSkillForGCI or*/
                objectPtr->localData->range >= 60.0F * NM_TO_FT or
 not (
                    (
                        baseData->IsSim() and 
                        ((SimBaseClass*)baseData)->GetCampaignObject()->GetSpotted(self->GetTeam())
                    ) or
                    (
                        baseData->IsCampaign() and 
                        ((CampBaseClass*)baseData)->GetSpotted(self->GetTeam())
                    )
                )
            )
            {
                objectPtr = objectPtr->next;
                continue;
            }
        }

        // 2001-08-04 MODIFIED BY S.G. objectPtr CAN BE A CAMPAIGN OBJECT. NEED TO ACCOUNT FOR THIS
        if (baseData->IsSim() and ((SimBaseClass *)objectPtr->BaseData())->pctStrength <= 0.0f)
        {
            // Dying target have a damage less than 0.0f
            objectPtr = objectPtr->next;
            continue;
        }
        // Increase priority of spike
        else if (baseData == curSpike)
        {
            //objectPtr->localData->threatTime *= 0.8F + (1.0F - SkillLevel()/4.0F) * 0.8F;
            objectPtr->localData->threatScore += 30;
            foundSpike = TRUE;
        }

        // 2000-09-12 ADDED BY S.G.
        // IF THE TARGET ALREADY HAS A MISSILES ON ITS WAY AND
        // WE DIDN'T SHOOT IT, DECREASE ITS PRIORITY BY 4. THIS WILL MAKE THE CURRENT LESS LIKEABLE
        /*else if (objectPtr->BaseData()->IsSim() and ((SimBaseClass *)objectPtr->BaseData())->incomingMissile[0] and ((SimWeaponClass *)((SimBaseClass *)objectPtr->BaseData())->incomingMissile[0])->parent not_eq self)
         //objectPtr->localData->targetTime *= 4.0f;
         //objectPtr->localData->threatScore -= 20;*/
        else if (baseData->IsSim() and ((SimBaseClass *)baseData)->incomingMissile[0])
        {
            if (theRadar->digiRadarMode == RadarClass::DigiSTT)
            {
                int doNothing = 1; //Cobra just keep going since we are guiding a missile in this mode
            }
            else
            {
                //objectPtr = objectPtr->next;
                //continue;
                objectPtr->localData->threatScore = 1; //we give it one so if there is nothing else it will
                //at least target this aircraft
            }
        }

        // Increase priority of current target
        /* if (targetPtr and objectPtr->BaseData() == targetPtr->BaseData()) {
            //objectPtr->localData->targetTime *= 0.5f;
         //objectPtr->localData->threatTime *= 0.5f;
          objectPtr->localData->threatScore += 30;

         }*/

        /*if (objectPtr->localData->threatTime < threatTime)
        {
         threatTime = objectPtr->localData->threatTime;
         maxThreatPtr = objectPtr;
        }

        if (objectPtr->localData->targetTime < targetTime)
        {
         targetTime = objectPtr->localData->targetTime;
         maxTargetPtr = objectPtr;
        }*/
        //Cobra test we want to force a retarget
        //TODO don't do this if you have to support a missile
        if (objectPtr->localData->threatScore > 0 and objectPtr->localData->threatScore >= baseScore)
        {
            //if (objectPtr->BaseData()->IsSim())
            //{
            //if ( not ((SimBaseClass *)objectPtr->BaseData())->incomingMissile[0]/* and ((SimWeaponClass *)((SimBaseClass *)objectPtr->BaseData())->incomingMissile[0])->parent not_eq self*/)
            //{
            baseScore = objectPtr->localData->threatScore;

            if (maxTargetPtr[0] == NULL)
                maxTargetPtr[0] = objectPtr;

            if (maxTargetPtr[0] not_eq NULL)
            {
                maxTargetPtr[1] = maxTargetPtr[0];
                maxTargetPtr[0] = objectPtr;
            }


            //}
            //}
        }

        objectPtr = objectPtr->next;
    }

    //Cobra we want to hold on targeting bombers for 15 seconds to be sure we don't miss a fighter target
    if (baseScore <= 5 and targetTimer == 0)
    {
        targetTimer = SimLibElapsedTime + 60000;
    }

    if (missionType not_eq AMIS_AIRCAV)
    {
        /*if (threatTime < targetTime and maxThreatPtr and not maxThreatPtr->BaseData()->OnGround() )
          SetTarget(maxThreatPtr);
        else if (targetTime < MAX_TARGET_TIME and maxTargetPtr and not maxTargetPtr->BaseData()->OnGround() )
          SetTarget(maxTargetPtr);*/
        if (baseScore <= 5 and targetTimer < SimLibElapsedTime and maxTargetPtr[0] and not maxTargetPtr[0]->BaseData()->OnGround())
        {
            SetTarget(maxTargetPtr[0]);
            targetTimer = 0;
        }
        else if (baseScore > 5 and maxTargetPtr[0] and not maxTargetPtr[0]->BaseData()->OnGround())
        {
            SetTarget(maxTargetPtr[0]);
        }
        else if (curSpike and not curSpike->OnGround() and not foundSpike)
        {
            SetThreat(curSpike);
        }
        else if (curSpike and curSpike->OnGround())
        {
            SetGroundTarget(curSpike);
        }
        else if (baseScore <= 5 and targetTimer < SimLibElapsedTime)
        {
            ClearTarget();
            AddMode(WaypointMode);
        }
    }
    else
    {
        int testme = 0;

        // airground mission, if pickup and drop off completely ignore
        // threats
        if (missionType == AMIS_AIRCAV)
        {
            ClearTarget();
            AddMode(WaypointMode);
        }

        /*
           else if (threatTime < targetTime and 
               maxThreatPtr and 
            maxThreatPtr->BaseData()->IsAirplane() and 
 not maxThreatPtr->BaseData()->OnGround() and 
            maxThreatPtr->localData->range < 5.0f * NM_TO_FT )
           {
           SetTarget(maxThreatPtr);
           }
           else if (targetTime < MAX_TARGET_TIME and 
                    maxTargetPtr and 
         maxTargetPtr->BaseData()->IsAirplane() and 
 not maxTargetPtr->BaseData()->OnGround() and 
         maxTargetPtr->localData->range < 5.0f * NM_TO_FT )
           {
           SetTarget(maxTargetPtr);
           }
           else if (curSpike and 
            curSpike->IsAirplane() and 
 not curSpike->OnGround() and 
 not foundSpike)
           {
           float dx, dy, dist;

           dx = self->XPos() - curSpike->XPos();
           dy = self->YPos() - curSpike->YPos();

           dist = dx * dx + dy * dy;

           if ( dist < 5.0f * NM_TO_FT * 5.0f * NM_TO_FT )
            SetThreat (curSpike);
           else
           {
           ClearTarget();
               AddMode (WaypointMode);
           }
           }
           else
           {
           ClearTarget();
           AddMode (WaypointMode);
           }*/
    }

    // Turn on jamming if possible
    if (curSpike and not jammertime or (flightLead and flightLead->IsSPJamming()))
    {
        if (self->HasSPJamming())
        {
            self->SetFlag(ECM_ON);
            jammertime = SimLibElapsedTime + 60000.0f;
        }
    }
    else if (jammertime and jammertime < SimLibElapsedTime)
    {
        jammertime = 0;
        self->UnSetFlag(ECM_ON);
    }
}

/*
** Name: CampTargetSelection
** Description:
** When running in the campaign, we get our marching orders (targets)
** from the campaign entities.
** NOTE: This function is intended only for Air-Air.  Air-Ground is
** another function using a different target ptr
*/
FalconEntity* DigitalBrain::CampTargetSelection(void)
{
    UnitClass *campUnit = (UnitClass *)self->GetCampaignObject();
    FalconEntity *target;

    // at this point we have no target, we're going to ask the campaign
    // to find out what we're supposed to hit
    // tell unit we haven't done any checking on it yet
    campUnit->UnsetChecked();

    // choose target.  I assume if this returns 0, no target....
    if ( not campUnit->ChooseTarget())
        return NULL;

    // get the target
    target = campUnit->GetTarget();

    // do we have a target?
    if ( not target)
        return NULL;


    // get tactic -- not doing anything with it now
    campUnit->ChooseTactic();
    campTactic = campUnit->GetUnitTactic();

    // set ground target pointer if on ground
    // never, ever set targetPtr to ground object
    // 2000-09-27 MODIFIED BY S.G. AI NEED TO SET ITS TARGET POINTER IF IT HAS REACHED ITS IP WAYPOINT AS WELL
    // if ( target->OnGround() and (missionClass == AAMission or missionComplete) and hasWeapons)
    if (target->OnGround() and (missionClass == AAMission or missionComplete or IsSetATC(ReachedIP)) and hasWeapons)
    {
        if ( not groundTargetPtr)
        {
            SetGroundTarget(target);
            SetupAGMode(NULL, NULL);
        }

        return NULL;
    }

#ifdef DEBUG_TARGETING
    {
        float rng;

        rng = (float)sqrt((campBaseTarg->XPos() - self->XPos()) * (campBaseTarg->XPos() - self->XPos()) +
                          (campBaseTarg->YPos() - self->YPos()) * (campBaseTarg->YPos() - self->YPos()));
        MonoPrint("%s-%d set camp target %s : range = %.2f\n", ((DrawableBSP*)self->drawPointer)->Label(), isWing + 1,
                  ((UnitClass*)targetPtr->BaseData())->GetUnitClassName(), rng * FT_TO_NM);
    }
#endif

    // Return the selected campaign target
    return target;
}

// Insert 1 target into a sorted target list. Maintain sort order
SimObjectType* DigitalBrain::InsertIntoTargetList(SimObjectType* root, SimObjectType* newObj)
{
    SimObjectType *tmpPtr;
    SimObjectType *last = NULL;

    // This new object had better NOT be in someone elses list
    ShiAssert(newObj->next == NULL);
    ShiAssert(newObj->prev == NULL);

    // Stuff the new sim thing into the list
    tmpPtr = root;

    // No list, so make it the list
    if (tmpPtr == NULL)
    {
        root = newObj;
        newObj->Reference();
    }
    else
    {
        while (tmpPtr and SimCompare(tmpPtr->BaseData(), newObj->BaseData()) < 0)
        {
            last = tmpPtr;
            tmpPtr = tmpPtr->next;
        }

        if ( not last and (tmpPtr->BaseData() not_eq newObj->BaseData()))
        {
            F4Assert(tmpPtr not_eq newObj);
            // Goes at the front
            newObj->next = root;
            root->prev = newObj;
            root = newObj;
            newObj->Reference();
        }
        // Goes at the end
        else if (tmpPtr == NULL)
        {
            last->next = newObj;
            newObj->prev = last;
            newObj->Reference();
        }
        // Somewhere in the middle, but not already in there
        else if (tmpPtr->BaseData() not_eq newObj->BaseData())
        {
            last->next = newObj;
            newObj->prev = last;

            tmpPtr->prev = newObj;
            newObj->next = tmpPtr;
            newObj->Reference();
        }
        // Must already be in the list
        else if (tmpPtr not_eq newObj)
        {
            F4Assert(tmpPtr->BaseData() == newObj->BaseData());

            if ( not tmpPtr->BaseData()->OnGround())
                SetTarget(tmpPtr);

            // we don't need this any more -- and it shouldn't have any refs
            newObj->Reference();
            newObj->Release();

        }
        else
        {
            // how did we get here?
            // we don't need this any more
            newObj->Reference();
            newObj->Release();
        }
    }

    return root;
}

SimObjectType* MakeSimListFromVuList(AircraftClass *self, SimObjectType* targetList, VuFilteredList* vuList)
{
    SimObjectType* rootObject;
    SimObjectType* curObject;
    SimObjectType* tmpObject;

    if (vuList)
    {
        // If we have a list then make the targetList has the same objects

        SimObjectType* lastObject;
        VuEntity* curEntity;

        curObject = targetList;
        rootObject = targetList;
        lastObject = NULL;

        VuListIterator updateWalker(vuList);
        curEntity = updateWalker.GetFirst();

        while (curEntity)
        {
            if (curObject)
            {
                if (curObject->BaseData() == curEntity)
                {
                    // Step both lists
                    lastObject = curObject;
                    curObject = curObject->next;
                    curEntity = updateWalker.GetNext();
                }
                else
                {
                    // Delete current object
                    if (curObject->prev)
                    {
                        curObject->prev->next = curObject->next;
                    }
                    else
                    {
                        rootObject = curObject->next;
                    }

                    if (curObject->next)
                    {
                        curObject->next->prev = curObject->prev;
                    }

                    tmpObject = curObject;
                    curObject = curObject->next;
                    tmpObject->next = NULL;
                    tmpObject->prev = NULL;
                    tmpObject->Release();
                    tmpObject = NULL;
                }
            }
            else   // FRB - ALERT
            {
                // sfr: no dead or sleeping entities
                FalconEntity *feEntity = static_cast<FalconEntity*>(curEntity);

                if (
                    feEntity->IsDead() or
                    (feEntity->IsSim() and not static_cast<SimBaseClass*>(feEntity)->IsAwake())
                )
                {
                    curEntity = updateWalker.GetNext();
                    continue;
                }

                // Append new entry to sim list and increment vu list
                tmpObject = new SimObjectType((FalconEntity*)curEntity);
                tmpObject->Reference();
                tmpObject->localData->range = 0.0F;
                tmpObject->localData->ataFrom = 180.0F * DTR;
                tmpObject->localData->aspect = 0.0F;
                tmpObject->next = NULL;
                tmpObject->prev = lastObject;

                if (lastObject)
                    lastObject->next = tmpObject;

                lastObject = tmpObject;

                // Set head if needed
                if ( not rootObject)
                    rootObject = tmpObject;

                // Step vu list
                curEntity = updateWalker.GetNext();
            }
        }
    }
    else
    {
        // PETER HACK
        // PETER: curObject has NOT been set by anything (garbage off the stack)
        // I am assuming this is what you meant :)
        // HOWEVER, this causes crashes later on so I am just returning
        // return(targetList);
        curObject = targetList;
        rootObject = targetList;
        // PETER END HACK

        // We have no list, so we should have no objects
        while (curObject)
        {
            // Delete current object
            if (curObject->prev)
            {
                curObject->prev->next = curObject->next;
            }
            else
            {
                rootObject = curObject->next;
            }

            if (curObject->next)
            {
                curObject->next->prev = curObject->prev;
            }

            tmpObject = curObject;
            curObject = curObject->next;
            tmpObject->next = NULL;
            tmpObject->prev = NULL;
            tmpObject->Release();
            tmpObject = NULL;
        }

        rootObject = NULL;
    }

    return rootObject;
}
