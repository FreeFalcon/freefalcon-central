#include "stdhdr.h"
#include "airframe.h"
#include "aircrft.h"
#include "digi.h"
#include "sensors.h"
#include "campbase.h"
#include "object.h"
#include "wingorder.h"
#include "campterr.h"
#include "phyconst.h"
#include "find.h"
#include "msginc/atcmsg.h"
#include "atcbrain.h"
/* 2001-06-07 S.G. */ #include "Classtbl.h"
/* 2001-06-07 S.G. */ #include "simstatc.h"
/* 2001-06-07 S.G. */ #include "Ground.h"
/* 2001-06-24 S.G. */ #include "sms.h"
/* 2001-07-30 S.G. */ #include "flight.h"
#include "entity.h"
#include "feature.h"

// -------------------------------------------------
//
// DigitalBrain::AiRunDecisionRoutines
//
// -------------------------------------------------
FalconEntity* SpikeCheck(AircraftClass* self, FalconEntity *byHim = NULL, int *data = NULL); // 2002-02-10 S.G.

// COBRA - RED - A list of Targets of No-Interest for AI
const DWORD NonTargets[] = {TYPE_TREE, TYPE_FENCE, TYPE_VASI, TYPE_TAXI_SIGN, 0 };

// COBRA - RED - This function returns TRUE if the passed type of a target is a target of interest
bool DigitalBrain::EvaluateTarget(DWORD Type)
{
    const DWORD *ptr = NonTargets;

    // Look in the list
    while (*ptr and *ptr not_eq Type) ptr++;

    // Return the result
    return *ptr ? false : true;
}



void DigitalBrain::AiRunDecisionRoutines(void)
{
    //if (curMode == GunsJinkMode)
    //{
    GunsJinkCheck();
    //}

    //if (curMode == MissileDefeatMode)
    //{
    MissileDefeatCheck();
    //}

    // Check if I should be landing
    ShiAssert(flightLead)

    if (flightLead)
        AiCheckForUnauthLand(flightLead->Id());

    //AiCheckLand();

    // Check if I was ordered RTB
    AiCheckRTB();

    // Check If I was ordered to kill somthing
    AiCheckEngage();

    // Check if we have been ordered to perform a fancy maneuver
    AiCheckManeuvers();

    // Check if we should be in formation
    AiCheckFormation();


    // Always follow waypoints
    AddMode(WaypointMode);
}



// -------------------------------------------------
//
// DigitalBrain::AiRunTargetSelection
//
// -------------------------------------------------

void DigitalBrain::AiRunTargetSelection(void)
{
    VuEntity* pnewTarget;
    FalconEntity* curSpike = SpikeCheck(self);

    // 2001-06-27 ADDED BY S.G. SO WING WILL SET THEIR ECM AS WELL
    if (curSpike or (flightLead and flightLead not_eq self and flightLead->IsSPJamming()))
    {
        if (self->HasSPJamming())
        {
            self->SetFlag(ECM_ON);
        }
    }
    else
    {
        self->UnSetFlag(ECM_ON);
    }

    // END OF ADDED SECTION

    if (curSpike)
    {
        SetThreat(curSpike);
    }
    else
    {
        if (mDesignatedObject not_eq FalconNullId)
        {
            // If target has been designated by the leader

            pnewTarget = vuDatabase->Find(mDesignatedObject); // Lookup target in database

            if (pnewTarget)
            {
#if 0    // Not working yet, commented out in case it creates more problem than it solves...

                // 2002-03-15 ADDED BY S.G. Special case when everyone in the unit is dead... Should help the AI not targeting chutes when that's all is left...
                // If it's a NON aggregated UNIT CAMPAIGN object, it SHOULD have components... If it doesn't, clear it's designated target.
                if (((FalconEntity *)pnewTarget)->IsCampaign() and ((CampBaseClass *)pnewTarget)->IsUnit() and not ((CampBaseClass *)pnewTarget)->IsAggregate() and ((CampBaseClass *)pnewTarget)->NumberOfComponents() == 0)
                {
                    ShiAssert( not "Empty deaggregated object as a target??");
                    mDesignatedObject = FalconNullId;
                }
                else
                    // END OF ADDED SECTION
#endif
                    AiSearchTargetList(pnewTarget); // Run targeting for wingman with designated target
            }
            else
            {
                mDesignatedObject = FalconNullId;
            }
        }
        else if (mpSearchFlags[AI_SEARCH_FOR_TARGET] or
                 mDesignatedObject == FalconNullId)   // If we are ordered to scan for targets
        {
            TargetSelection(); // Run the full selection routine
        }
        else
        {
            ShiAssert(curMode not_eq GunsEngageMode);// Otherwise just chill out
            ClearTarget(); // No targets of interest
            AiRestoreWeaponState();
            mpActionFlags[AI_ENGAGE_TARGET] = AI_NONE; // 2002-03-04 MODIFIED BY S.G. Use new enum type
        }
    }
}



// -------------------------------------------------
//
// DigitalBrain::AiSearchTargetList
//
// -------------------------------------------------

void DigitalBrain::AiSearchTargetList(VuEntity* pentity)
{
    CampBaseClass* theTargetGroup = (CampBaseClass*)pentity;
    FalconEntity* theTarget = NULL;
    SimObjectType* objectPtr = NULL;

    if ( not pentity)
        return;

    if (((FalconEntity*)pentity)->IsSim())
    {
        theTarget = (FalconEntity*)pentity;

        // 2002-04-02 ADDED BY S.G. If it's dead or a chute, leave it alone...
        if ( not theTarget->OnGround() and (theTarget->IsDead() or theTarget->IsEject()))
            theTarget = NULL;

        // END OF ADDED SECTION 2002-04-02
    }
    else
    {
        if (theTargetGroup->NumberOfComponents())
            // 2001-06-04 MODIFIED BY S.G. NEED TO ACCOUNT FOR GROUND TARGETS DIFFERENTLY THAN AIR TARGETS
            //       theTarget = theTargetGroup->GetComponentEntity (isWing % theTargetGroup->NumberOfComponents());
        {
            if (theTargetGroup->OnGround())
            {
                // If we already have a ground target and it is part of the unit we are being asked to target and it's still alive, keep using it (ie, don't switch)
                if (groundTargetPtr and groundTargetPtr->BaseData()->IsSim() and 
                    ((SimBaseClass *)groundTargetPtr->BaseData())->GetCampaignObject() == theTargetGroup and 
 not groundTargetPtr->BaseData()->IsExploding() and not groundTargetPtr->BaseData()->IsDead() and 
                    ((SimBaseClass *)groundTargetPtr->BaseData())->pctStrength > 0.0f)
                    return;

                // Use our hep function
                theTarget = FindSimGroundTarget(theTargetGroup, theTargetGroup->NumberOfComponents(), 0);
            }
            // Target is not on the ground, select your opponent in the flight, wrapping if the opponent has less planes than us
            else
                theTarget = theTargetGroup->GetComponentEntity(isWing % theTargetGroup->NumberOfComponents());

            // 2002-04-02 ADDED BY S.G. If it's dead or a chute, leave it alone...
            if (theTarget and (theTarget->IsDead() or theTarget->IsEject()))
                theTarget = NULL;

            // END OF ADDED SECTION 2002-04-02
        }
        else
            theTarget = theTargetGroup;
    }

    if (theTarget)
    {
        // 2001-05-10 MODIFIED BY S.G. IF WE HAVE A GROUND TARGET AND IT'S OUR TARGET, AVOID DOING THIS AS WELL (WE'RE FINE)
        //    if ( not targetPtr or targetPtr->BaseData() not_eq theTarget)
        if (( not targetPtr or targetPtr->BaseData() not_eq theTarget) and ( not groundTargetPtr or groundTargetPtr->BaseData() not_eq theTarget))
        {
#ifdef DEBUG
            /*     objectPtr = new SimObjectType (OBJ_TAG, self, theTarget);*/
#else
            objectPtr = new SimObjectType(theTarget);
#endif
            SetTarget(objectPtr);
            // 2000-09-18 ADDED BY S.G. SO AI STARTS SHOOTING RIGHT NOW AND STOP WAITING THAT STUPID 30 SECONDS
            missileShotTimer = 0;
            // END OF ADDED SECTION
        }
    }
    else
    {
        ShiAssert(curMode not_eq GunsEngageMode);
        ClearTarget();
        mDesignatedObject = FalconNullId; // 2002-04-04 ADDED BY S.G. If we are clearing the target, we might as well clear the designated target as well so we leave it alone...
        AddMode(WaypointMode);
    }
}

// 2001-06-04 ADDED BY S.G. HELP FUNCTION TO SEARCH FOR A GROUND TARGET
SimBaseClass *DigitalBrain::FindSimGroundTarget(CampBaseClass *targetGroup, int targetNumComponents, int startPos)
{
    int i, gotRadar = FALSE;
    int usComponents = self->GetCampaignObject()->NumberOfComponents();
    int haveHARMS = FALSE;
    int otherHaveHARMS = FALSE;
    FeatureClassDataType *fc = NULL;
    SimBaseClass *simTarg = NULL;
    SimBaseClass *firstSimTarg = NULL;
    AircraftClass *flightMember[4] =  { 0 }; // Maximum of 4 planes per flight with no target as default

    // Get the flight aircrafts (once per call instead of once per target querried)
    for (i = 0; i < usComponents; i++)
    {
        // I onced tried to get the player's current target so it could be skipped by the AI but
        // all the player's are not on the same PC as the AI so this is not valid.
        // Therefore, only get this from digital planes, or the player if he is local
        // 2002-03-08 MODIFIED BY S.G. Code change so I'm only calling GetComponentEntity once and checking if it returns non NULL
        flightMember[i] = (AircraftClass *)self->GetCampaignObject()->GetComponentEntity(i);

        if (flightMember[i] and (flightMember[i]->IsDigital() or flightMember[i]->IsLocal()))
        {
            // Now that we know it's local (hopefully, digitals are also local), see if they are carrying HARMS
            // By first looking at their 'hasHARM' status
            if (flightMember[i]->DBrain()->hasHARM)
            {
                if (i == isWing)
                    haveHARMS = TRUE;
                else
                    otherHaveHARMS = TRUE;
            }
            // Can't rely on 'hasHARM' so check the loadout of each plane yourself...
            else
            {
                // Try the loadout as long I didn't find an HARM on mine or another plane higher than us has an HARM
                for (int j = 0; not haveHARMS and not (otherHaveHARMS and i > isWing) and flightMember[i]->Sms and j < flightMember[i]->Sms->NumHardpoints(); j++)
                {
                    if (flightMember[i]->Sms->hardPoint[j]->weaponPointer and flightMember[i]->Sms->hardPoint[j]->GetWeaponClass() == wcHARMWpn)
                    {
                        if (i == isWing)
                            haveHARMS = TRUE;
                        else
                            otherHaveHARMS = TRUE;
                    }
                }
            }
        }
        else
            flightMember[i] = NULL;
    }

    // Check each sim entity in the campaign entity in succession, starting at startPos.
    // When incrementing i, use 0 if we had a 'startPos' but it wasn't valid

    //#define SG_TEST_NO_RANDOM_TARGET // Define this to make sure an AI always goes at the same target. Makes it easier to debug with constant behaviour
#ifndef SG_TEST_NO_RANDOM_TARGET

    // 2001-10-19 ADDED BY S.G. IF WE ARE USING AN HARM OR A MAVERICK, DON'T RANDOMIZE
    if ( not haveHARMS and not hasAGMissile and targetNumComponents)
    {
        // JB 011014 Target "randomly" if its just a long line of vehicles
        if (startPos == 0)
        {
            startPos = rand() % targetNumComponents;

            for (i = startPos; i < targetNumComponents and startPos > 0; i++)
                if ( not targetGroup->GetComponentEntity(i)->IsVehicle() or not targetGroup->GetComponentEntity(i)->OnGround())
                    startPos = 0;
        }

        // JB 011014
    }
    else
        startPos = 0;

#endif

    for (i = startPos; i < targetNumComponents; i = startPos ? 0 : i + 1, startPos = 0)
    {
        // Get the sim object associated to this entity number
        simTarg = targetGroup->GetComponentEntity(i);

        if ( not simTarg or F4IsBadReadPtr(simTarg, sizeof(simTarg)))  //sanity check
            continue;

        // Is it alive?
        if (simTarg->IsExploding() or simTarg->IsDead() or simTarg->pctStrength <= 0.0f)  // Cobra - add priority filter?
            continue; // Dead thing, ignore it.


        // COBRA - RED - skip if a target of no-interest
        if ( not EvaluateTarget(simTarg->GetType()))
            continue;

        // FRB - Put it back in
        //* // Cobra - Don't target low priority features (trees, fences, sheds)
        if (simTarg->IsStatic()) // It's a feature
        {
            fc = GetFeatureClassData(((Objective)simTarg)->GetFeatureID(i));

            if (fc and not F4IsBadReadPtr(fc, sizeof(fc)) and fc->Priority > 2)  // higher priority number = lower priority
                continue;

            if (((Objective)simTarg)->GetFeatureStatus(i) == VIS_DESTROYED)
                continue;
        }

        //*/
        // Are flight members already using it (was using it) as a target?#
        int j = 0;

        for (j = 0; j < usComponents; j++)
        {
            if (flightMember[j])
            {
                if (flightMember[j]->vehicleInUnit == self->vehicleInUnit)
                {
                    continue;
                }

                if (
                    flightMember[j]->DBrain() and 
                    (
                        (
                            flightMember[j]->DBrain()->groundTargetPtr and 
                            flightMember[j]->DBrain()->groundTargetPtr->BaseData() == simTarg
                        ) or
                        flightMember[j]->DBrain()->gndTargetHistory[0] == simTarg or
                        flightMember[j]->DBrain()->gndTargetHistory[1] == simTarg
                    )
                )
                {
                    break;  // Yes, ignore it.
                }
            }
        }

        // If we didn't reach the end, someone else is using it so skip it.
        if (j not_eq usComponents)
            continue;

        // Mark this sim entity as the first target with a match (in case no emitting targets are left standing, or it's a feature)
        if ( not firstSimTarg)
            firstSimTarg = simTarg;

        // Is it an objective and we are not carrying HARMS (HARMS will go for the radar feature)? If so, stop right now and use that feature
        // Cobra - take out radar with whatever AG weapon you have
        // RED -  Code Restored, was causing continuous hunting for radars with any weapon

        // FRB - So?
        //if (targetGroup->IsObjective() and not hasHARM) break;

        // If I have HARMS or no one has any and the entity has a radar, choose it
        // 2001-07-12 S.G. Testing if radar first so it's not becoming an air defense if i have no harms
        if ((simTarg->IsVehicle() and ((SimVehicleClass *)simTarg)->GetRadarType() not_eq RDR_NO_RADAR) or // It's a vehicle and it has a radar
            (simTarg->IsStatic() and ((SimStaticClass *)simTarg)->GetRadarType() not_eq RDR_NO_RADAR))  // It's a feature and it has a radar
        {
            // 2001-07-29 S.G. If I was shooting at the campaign object, then I stick to it
            if ((((FlightClass *)self->GetCampaignObject())->shotAt == targetGroup and ((FlightClass *)self->GetCampaignObject())->whoShot == self) or ((((FlightClass *)self->GetCampaignObject())->whoShot == NULL) and (haveHARMS or not otherHaveHARMS)))
            {
                gotRadar = TRUE;
                firstSimTarg = simTarg; // Yes, use it for a target
                break; // and stop looking
            }
        }
        // 2001-07-12 S.G. DON'T CHECK IF AIR DEFENSE IF IT'S A RADAR
        else
        {
            // Prioritize air defense thingies...
            if (simTarg->IsGroundVehicle() and ((GroundClass *)simTarg)->isAirDefense)
            {
                gotRadar = TRUE;
                firstSimTarg = simTarg; // Yes, use it for a target

                // 2001-07-12 S.G. DON'T CONTINUE IF I HAVE NO HARMS
                if ( not haveHARMS)
                    break;
            }
        }

        // Look for the next one...
    }

    if (startPos < targetNumComponents)
    {
        // Now after all this, see if the AI is too dumb for selecting a valid target...
        // If he is, select at random
        if ( not gotRadar and (unsigned)((unsigned)rand() % (unsigned)32) > (unsigned)((unsigned)SkillLevel() + (unsigned)28))
            firstSimTarg = targetGroup->GetComponentEntity(rand() % targetNumComponents);

        // Keep track of the last two targets but only if we have one, otherwise, leave our previous targets alone
        if (firstSimTarg)
        {
            gndTargetHistory[1] = gndTargetHistory[0];
            gndTargetHistory[0] = firstSimTarg;
        }
    }
    else
        firstSimTarg = 0;

    // 2001-07-29 S.G. If I was shooting at a (not necessarely THIS one) campaign object, say I'm not anymore since the object deaggregated.
    if (((FlightClass *)self->GetCampaignObject())->whoShot == self)
    {
        ((FlightClass *)self->GetCampaignObject())->shotAt = NULL;
        ((FlightClass *)self->GetCampaignObject())->whoShot = NULL;
    }

    // JB 011017 from Schumi if targetNumComponents is less than usComponents, then of course there is no target anymore for the wingmen to bomb, and firstSimTarg is NULL.
    if (firstSimTarg == NULL and targetNumComponents and targetNumComponents < usComponents)
        firstSimTarg = targetGroup->GetComponentEntity(rand() % targetNumComponents);

    return firstSimTarg;
}
// END OF ADDED SECTION

// 2001-12-17 Added by M.N. - for air diverts
SimBaseClass *DigitalBrain::FindSimAirTarget(CampBaseClass *targetGroup, int targetNumComponents, int startPos)
{
    int i, gotRadar = FALSE;
    int usComponents = self->GetCampaignObject()->NumberOfComponents();
    int haveHARMS = FALSE;
    int otherHaveHARMS = FALSE;
    SimBaseClass *simTarg = NULL;
    SimBaseClass *firstSimTarg = NULL;
    AircraftClass *flightMember[4] =  { 0 }; // Maximum of 4 planes per flight with no target as default

    // Get the flight aircrafts (once per call instead of once per target querried)
    for (i = 0; i < usComponents; i++)
    {
        // I onced tried to get the player's current target so it could be skipped by the AI but
        // all the player's are not on the same PC as the AI so this is not valid.
        // Therefore, only get this from digital planes, or the player if he is local
        // 2002-03-08 MODIFIED BY S.G. Code change so I'm only calling GetComponentEntity once and checking if it returns non NULL
        flightMember[i] = (AircraftClass *)self->GetCampaignObject()->GetComponentEntity(i);

        if (flightMember[i] and ( not flightMember[i]->IsDigital() and not flightMember[i]->IsLocal()))
        {
            flightMember[i] = NULL;
        }
    }

    // Check each sim entity in the campaign entity in succession, starting at startPos.
    // When incrementing i, use 0 if we had a 'startPos' but it wasn't valid

    for (i = startPos; i < targetNumComponents; i = startPos ? 0 : i + 1, startPos = 0)
    {
        // Get the sim object associated to this entity number
        simTarg = targetGroup->GetComponentEntity(i);

        if ( not simTarg) //sanity check
            continue;

        // Is it alive?
        if (simTarg->IsExploding() or simTarg->IsDead() or simTarg->pctStrength <= 0.0f)
            continue; // Dead thing, ignore it.

        // Are flight members already using it (was using it) as a target?
        int j = 0;

        for (j = 0; j < usComponents; j++) // MN - use the gndtargethistory for divert air targets, too.
        {
            // Cobra - You will always get a match with my last target...skip me
            if (flightMember[j]->vehicleInUnit == self->vehicleInUnit)
                continue;

            if (flightMember[j] and flightMember[j]->DBrain() and ((flightMember[j]->DBrain()->airtargetPtr and flightMember[j]->DBrain()->airtargetPtr->BaseData() == simTarg) or flightMember[j]->DBrain()->gndTargetHistory[0] == simTarg or flightMember[j]->DBrain()->gndTargetHistory[1] == simTarg))
                break;  // Yes, ignore it.
        }

        // If we didn't reach the end, someone else is using it so skip it.
        if (j not_eq usComponents)
            continue;

        // Mark this sim entity as the first target with a match (in case no emitting targets are left standing, or it's a feature)
        if ( not firstSimTarg)
            firstSimTarg = simTarg;

        // Look for the next one...
    }

    if (startPos < targetNumComponents)
    {
        // Keep track of the last two targets but only if we have one, otherwise, leave our previous targets alone
        if (firstSimTarg)
        {
            gndTargetHistory[1] = gndTargetHistory[0];
            gndTargetHistory[0] = firstSimTarg;
        }
    }
    else
        firstSimTarg = 0;

    // JB 011017 from Schumi if targetNumComponents is less than usComponents, then of course there is no target anymore for the wingmen to bomb, and firstSimTarg is NULL.
    if (firstSimTarg == NULL and targetNumComponents and targetNumComponents < usComponents)
        firstSimTarg = targetGroup->GetComponentEntity(rand() % targetNumComponents);

    return firstSimTarg;
}
// END of added section


// Cobra help function to search for a JSOW ground target
SimBaseClass *DigitalBrain::FindJSOWGroundTarget(CampBaseClass *targetGroup, int targetNumComponents, int feaNo)
{
    int usComponents = self->GetCampaignObject()->NumberOfComponents();
    int FeatureNumber = feaNo;
    FeatureClassDataType *fc = NULL;
    SimBaseClass *simTarg = NULL;
    SimBaseClass *firstSimTarg = NULL;
    AircraftClass *flightMember[4] =  { 0 }; // Maximum of 4 planes per flight with no target as default

    // Get the flight aircrafts (once per call instead of once per target querried)
    for (int i = 0; i < usComponents; i++)
    {
        flightMember[i] = (AircraftClass *)self->GetCampaignObject()->GetComponentEntity(i);
    }

    // Check each sim entity in the campaign entity in succession, starting at startPos = 0.
    int i = 0;

    for (i = 0; i < targetNumComponents; i++)
    {
        // Get the sim object associated to this entity number
        simTarg = targetGroup->GetComponentEntity(i);

        if ( not simTarg or F4IsBadReadPtr(simTarg, sizeof(simTarg)))  //sanity check
            continue;

        // Is it alive?
        if (simTarg->IsExploding() or simTarg->IsDead() or simTarg->pctStrength <= 0.0f)  // Cobra - add priority filter?
            continue; // Dead thing, ignore it.

        // COBRA - RED - skip if a target of no-interest
        if ( not EvaluateTarget(simTarg->GetType()))
            continue;

        // RED - TODO -
        // FRB - Leave it
        ///* // FRB - Cobra - Don't target low priority features (trees, fences, sheds)
        if (simTarg->IsStatic()) // It's a feature
        {
            fc = GetFeatureClassData(((Objective)simTarg)->GetFeatureID(i));

            if (fc and not F4IsBadReadPtr(fc, sizeof(fc)) and fc->Priority > 2)  // higher priority number = lower priority
                continue;

            if (((Objective)simTarg)->GetFeatureStatus(i) == VIS_DESTROYED)
                continue;
        }//*/


        // Are flight members already using it (was using it) as a target?
        int j = 0;

        for (j = 0; j < usComponents; j++)
        {
            if ( not flightMember[j])
                continue;

            // Cobra - You will always get a match with my last target...skip me
            if (flightMember[j]->vehicleInUnit == self->vehicleInUnit)
                continue;

            if (flightMember[j] and flightMember[j]->DBrain() and ((flightMember[j]->DBrain()->groundTargetPtr and 
                    flightMember[j]->DBrain()->groundTargetPtr->BaseData() == simTarg) or
                    flightMember[j]->DBrain()->gndTargetHistory[0] == simTarg or
                    flightMember[j]->DBrain()->gndTargetHistory[1] == simTarg))
                break;  // Yes, ignore it.
        }

        // If we didn't reach the end, someone else is using it so skip it.
        if (j not_eq usComponents)
            continue;

        // Mark this sim entity as the first target with a match (in case no emitting targets are left standing, or it's a feature)
        if ( not firstSimTarg)
        {
            FeatureNumber = i;
            firstSimTarg = simTarg;
        }

        // Look for the next one...
    }

    if (i < targetNumComponents)
    {
        // Now after all this, see if the AI is too dumb for selecting a valid target...
        // If he is, select at random
        if ((unsigned)((unsigned)rand() % (unsigned)32) > (unsigned)((unsigned)SkillLevel() + (unsigned)28))
        {
            FeatureNumber = (rand() % targetNumComponents);
            firstSimTarg = targetGroup->GetComponentEntity(rand() % targetNumComponents);
        }

        // Keep track of the last two targets but only if we have one, otherwise, leave our previous targets alone
        if (firstSimTarg)
        {
            gndTargetHistory[1] = gndTargetHistory[0];
            gndTargetHistory[0] = firstSimTarg;
        }
    }
    else
        firstSimTarg = 0;

    // 2001-07-29 S.G. If I was shooting at a (not necessarely THIS one) campaign object, say I'm not anymore since the object deaggregated.
    if (((FlightClass *)self->GetCampaignObject())->whoShot == self)
    {
        ((FlightClass *)self->GetCampaignObject())->shotAt = NULL;
        ((FlightClass *)self->GetCampaignObject())->whoShot = NULL;
    }

    // JB 011017 from Schumi if targetNumComponents is less than usComponents, then of course there is no target anymore for the wingmen to bomb, and firstSimTarg is NULL.
    if (firstSimTarg == NULL and targetNumComponents and targetNumComponents < usComponents)
    {
        FeatureNumber = (rand() % targetNumComponents);
        firstSimTarg = targetGroup->GetComponentEntity(rand() % targetNumComponents);
    }

    return firstSimTarg;
}


// Cobra help function to search for a JDAM ground target
int DigitalBrain::FindJDAMGroundTarget(CampBaseClass *targetGroup, int targetNumComponents, int feaNo)
{
    int usComponents = self->GetCampaignObject()->NumberOfComponents();
    int FeatureNumber = feaNo;
    FeatureClassDataType *fc = NULL;
    SimBaseClass *simTarg = NULL;
    SimBaseClass *firstSimTarg = NULL;
    AircraftClass *flightMember[4] =  { 0 }; // Maximum of 4 planes per flight with no target as default

    if (usComponents <= 1)
        return FeatureNumber;

    // Get the flight aircrafts (once per call instead of once per target querried)
    for (int i = 0; i < usComponents; i++)
    {
        flightMember[i] = (AircraftClass *)self->GetCampaignObject()->GetComponentEntity(i);
    }

    // Check each sim entity in the campaign entity in succession, starting at startPos = 0.
    int i = 0;

    for (i = 0; i < targetNumComponents; i++)
    {
        // Get the sim object associated to this entity number
        simTarg = targetGroup->GetComponentEntity(i);

        if ( not simTarg or F4IsBadReadPtr(simTarg, sizeof(simTarg)))  //sanity check
            continue;

        // Is it alive?
        if (simTarg->IsExploding() or simTarg->IsDead() or simTarg->pctStrength <= 0.0f)  // Cobra - add priority filter?
            continue; // Dead thing, ignore it.

        // COBRA - RED - skip if a target of no-interest
        //if( not EvaluateTarget(simTarg->GetType()))
        // continue;
        // RED - TODO -
        // FRB - Cobra - Don't target low priority features (trees, fences, sheds)
        if (simTarg->IsStatic()) // It's a feature
        {
            if ((Objective)simTarg)
                fc = GetFeatureClassData(((Objective)simTarg)->GetFeatureID(i));

            if (fc and not F4IsBadReadPtr(fc, sizeof(fc)) and fc->Priority > 2)  // higher priority number = lower priority
                continue;

            if (((Objective)simTarg)->GetFeatureStatus(i) == VIS_DESTROYED)
                continue;
        }

        // Are flight members already using it (was using it) as a target?
        int j = 0;

        for (j = 0; j < usComponents; j++)
        {
            if ( not flightMember[j])
                continue;

            // Cobra - You will always get a match with my last target...skip me
            if (flightMember[j]->vehicleInUnit == self->vehicleInUnit)
                continue;

            if (flightMember[j] and flightMember[j]->DBrain() and ((flightMember[j]->DBrain()->groundTargetPtr and 
                    flightMember[j]->DBrain()->groundTargetPtr->BaseData() == simTarg) or
                    flightMember[j]->DBrain()->gndTargetHistory[0] == simTarg or
                    flightMember[j]->DBrain()->gndTargetHistory[1] == simTarg))
                break;  // Yes, ignore it.
        }

        // If we didn't reach the end, someone else is using it so skip it.
        if (j not_eq usComponents)
            continue;

        // Mark this sim entity as the first target with a match (in case no emitting targets are left standing, or it's a feature)
        if ( not firstSimTarg)
        {
            FeatureNumber = i;
            firstSimTarg = simTarg;
        }

        // Look for the next one...
    }

    if (i < targetNumComponents)
    {
        // Now after all this, see if the AI is too dumb for selecting a valid target...
        // If he is, select at random
        if ((unsigned)((unsigned)rand() % (unsigned)32) > (unsigned)((unsigned)SkillLevel() + (unsigned)28))
        {
            FeatureNumber = (rand() % targetNumComponents);
            firstSimTarg = targetGroup->GetComponentEntity(rand() % targetNumComponents);
        }

        // Keep track of the last two targets but only if we have one, otherwise, leave our previous targets alone
        if (firstSimTarg)
        {
            gndTargetHistory[1] = gndTargetHistory[0];
            gndTargetHistory[0] = firstSimTarg;
        }
    }
    else
        firstSimTarg = 0;

    // 2001-07-29 S.G. If I was shooting at a (not necessarely THIS one) campaign object, say I'm not anymore since the object deaggregated.
    if (((FlightClass *)self->GetCampaignObject())->whoShot == self)
    {
        ((FlightClass *)self->GetCampaignObject())->shotAt = NULL;
        ((FlightClass *)self->GetCampaignObject())->whoShot = NULL;
    }

    // JB 011017 from Schumi if targetNumComponents is less than usComponents, then of course there is no target anymore for the wingmen to bomb, and firstSimTarg is NULL.
    if (firstSimTarg == NULL and targetNumComponents and targetNumComponents < usComponents)
    {
        FeatureNumber = (rand() % targetNumComponents);
        firstSimTarg = targetGroup->GetComponentEntity(rand() % targetNumComponents);
    }

    return FeatureNumber;
}


// -------------------------------------------------
//
// DigitalBrain::AiCheckEngage
//
// -------------------------------------------------

void DigitalBrain::AiCheckEngage(void)
{
    // 2000-09-18 MODIFIED BY S.G. NEED THE WINGMEN TO DO ITS DUTY EVEN WHILE EXECUTING MANEUVERS...
    // 2000-09-25 MODIFIED BY S.G. NEED THE WINGMEN TO DO ITS STUFF WHEN HE HAS WEAPON FREE AS WELL
    // 2002-03-15 MODIFIED BY S.G. Perform this if mpActionFlags[AI_EXECUTE_MANEUVER] is NOT TRUE+1, since TRUE+1 mean we are doing a maneuver that's limiting the AI's ACTION to specific functions
    // if(mpActionFlags[AI_ENGAGE_TARGET] /* REMOVED BY S.G. and not mpActionFlags[AI_EXECUTE_MANEUVER] */) {
    // if(mpActionFlags[AI_ENGAGE_TARGET] or mWeaponsAction == AI_WEAPONS_FREE) {
    if ((mpActionFlags[AI_ENGAGE_TARGET] or mWeaponsAction == AI_WEAPONS_FREE) and mpActionFlags[AI_EXECUTE_MANEUVER] not_eq TRUE + 1)
    {
        MergeCheck();
        BvrEngageCheck();
        GunsEngageCheck();
        WvrEngageCheck();
        MissileEngageCheck();
        AccelCheck();
    }
}



// -------------------------------------------------
//
// DigitalBrain::AiCheckRTB
//
// -------------------------------------------------

void DigitalBrain::AiCheckRTB(void)
{
    // set waypoint to home
    // check distance to home, contact tower it necessary

    if (mpActionFlags[AI_RTB])   // If we are ordered to do a maneuver
    {
        AddMode(RTBMode); // Add maneuvers to stack

        // if(mpActionFlags[AI_LANDING] == FALSE and distance to airbase < 15 nm, and no atc) {
        // contact atc
        // mpActionFlags[AI_LANDING] = TRUE;
        //}
    }
}


// -------------------------------------------------
//
// DigitalBrain::AiCheckLand
//
// -------------------------------------------------

void DigitalBrain::AiCheckLandTakeoff(void)
{
    if (self->curWaypoint == NULL)
    {
        return;
    }

    if (SimLibElapsedTime > updateTime)
    {
        ObjectiveClass *Airbase = (ObjectiveClass *)vuDatabase->Find(airbase);

        if (Airbase)
        {
            float dx = self->XPos() - Airbase->XPos();
            float dy = self->YPos() - Airbase->YPos();
            distAirbase = (float)sqrt(dx * dx + dy * dy);
        }

        updateTime = SimLibElapsedTime + 15 * CampaignSeconds;
    }

    if (atcstatus >= tReqTaxi and atcstatus <= tTaxiBack)
    {
        mpActionFlags[AI_FOLLOW_FORMATION] = FALSE;
        AddMode(TakeoffMode);
    }
    else if (mpActionFlags[AI_LANDING]
             or (atcstatus >= lReqClearance and atcstatus <= lCrashed))
    {
        if (atcstatus > lIngressing)
            mpActionFlags[AI_FOLLOW_FORMATION] = FALSE;

        AddMode(LandingMode);
    }
    else if (self->curWaypoint->GetWPAction() == WP_LAND and not self->OnGround() and distAirbase < 30.0F * NM_TO_FT
            and (missionComplete or IsSetATC(SaidRTB) or IsSetATC(SaidBingo) or mpActionFlags[AI_RTB])) // don't land if one of these conditions isn't met
    {
        if (atcstatus > lIngressing)
            mpActionFlags[AI_FOLLOW_FORMATION] = FALSE;

        AddMode(LandingMode);
    }
    else if (self->curWaypoint->GetWPAction() == WP_TAKEOFF and self->OnGround())
    {
        mpActionFlags[AI_FOLLOW_FORMATION] = FALSE;
        AddMode(TakeoffMode);
    }
    else if (self->OnGround())
    {
        atcstatus = lTaxiOff;
        mpActionFlags[AI_FOLLOW_FORMATION] = FALSE;
        AddMode(LandingMode);
    }
}

// -------------------------------------------------
//
// DigitalBrain::AiCheckManeuvers
//
// -------------------------------------------------

void DigitalBrain::AiCheckManeuvers(void)
{

    if (mpActionFlags[AI_EXECUTE_MANEUVER])   // If we are ordered to do a maneuver
    {
        AddMode(FollowOrdersMode); // Add maneuvers to stack
    }
}


// -------------------------------------------------
//
// DigitalBrain::AiCheckFormation
//
// -------------------------------------------------

void DigitalBrain::AiCheckFormation(void)
{
    //temp Hack until I can talk to Vince about a better way. DSP
    // if(mpActionFlags[AI_FOLLOW_FORMATION] and self->curWaypoint->GetWPAction() not_eq WP_LAND) { // If we are ordered to fly in formation

    // edg: if the wingy was told to engage a ground target, we must make sure that they
    // will continue on waypoint mode so that they can go thru the ground attack logic
    if (mpActionFlags[AI_ENGAGE_TARGET] and not mpActionFlags[AI_EXECUTE_MANEUVER] and (agDoctrine not_eq AGD_NONE or groundTargetPtr))
    {
        AddMode(WaypointMode);
    }
    else if (mpActionFlags[AI_FOLLOW_FORMATION])
    {
        AddMode(WingyMode); // Add formation mode to stack
    }
}



// -------------------------------------------------
//
// DigitalBrain::AiSplitFlight
//
// -------------------------------------------------

void DigitalBrain::AiSplitFlight(int extent, VU_ID from, int idx)
{
    if (vuDatabase->Find(from) == self->GetCampaignObject()->GetComponentLead())   // if from the flight lead
    {
        if (extent == AiElement or extent == AiFlight)
        {
            if (idx == AiElementLead or idx == AiSecondWing)
            {
                mSplitFlight = TRUE;
            }
        }
    }
    else   // Otherwise the order is coming from the element lead, we want to follow him
    {
        if (extent == AiWingman and idx == AiSecondWing)
        {
            mSplitFlight = TRUE;
        }
    }
}




// -------------------------------------------------
//
// DigitalBrain::AiGlueFlight
//
// -------------------------------------------------

void DigitalBrain::AiGlueFlight(int extent, VU_ID from, int idx)
{
    if (vuDatabase->Find(from) == self->GetCampaignObject()->GetComponentLead())   // if from the flight lead
    {
        if (extent == AiElement or extent == AiFlight)
        {
            if (idx == AiElementLead or idx == AiSecondWing)
            {
                mSplitFlight = FALSE;
            }
        }
    }
    else   // Otherwise the order is coming from the element lead, we want to follow him
    {
        if (extent == AiWingman and idx == AiSecondWing)
        {
            mSplitFlight = TRUE;
        }
    }
}

void DigitalBrain::AiCheckForUnauthLand(VU_ID lead)
{
    GridIndex x, y;
    vector pos;

    AircraftClass* leader = (AircraftClass*) vuDatabase->Find(lead);

    if ( not self->OnGround() and leader and mpActionFlags[AI_FOLLOW_FORMATION] == TRUE and leader->DBrain()->ATCStatus() < tReqTaxi and leader->OnGround() and atcstatus == noATC)
    {

        pos.x = leader->XPos();
        pos.y = leader->YPos();
        ConvertSimToGrid(&pos, &x, &y);

        Objective obj = FindNearestFriendlyAirbase(self->GetTeam(), x, y);

        if (obj)
        {
            airbase = obj->Id();
            atcstatus = lReqClearance;
            SendATCMsg(atcstatus);
            mpActionFlags[AI_FOLLOW_FORMATION] = FALSE;
        }
    }
}

