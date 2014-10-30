#include "stdhdr.h"
#include "hdigi.h"
#include "sensors.h"
#include "falcmesg.h"
#include "simveh.h"
#include "fcc.h"
#include "sms.h"
#include "object.h"
#include "MsgInc/airaimodechange.h"
#include "campwp.h"
#include "falcsess.h"
#include "simdrive.h"
#include "camp2sim.h"
#include "campbase.h"
#include "team.h"
#include "unit.h"

void HeliBrain::DecisionLogic(void)
{
    /*--------------------*/
    /* ground avoid check */
    /*--------------------*/
    // GroundCheck();

    // if (self->flightLead == self)
    {
        RunDecisionRoutines();
    }
    /*
    else
    {
       CheckOrders();
    }
    */

    /*------------------------*/
    /* resolve mode conflicts */
    /*------------------------*/
    ResolveModeConflicts();

    /*----------------------------------*/
    /* print mode changes as they occur */
    /*----------------------------------*/
    // PrtMode();

    /*------------------*/
    /* weapon selection */
    /*------------------*/
    // if (targetPtr)
    //     WeaponSelection();

    /*--------------*/
    /* fire control */
    /*--------------*/
    // if (targetPtr and shooting)
    //     FireControl();
}


void HeliBrain::TargetSelection(void)
{
    UnitClass *campUnit = (UnitClass *)self->GetCampaignObject();
    FalconEntity *target;
    SimBaseClass *simTarg;
    int campTactic;

    // sanity check
    if ( not campUnit)
        return;

    // check to see if our current ground target is a sim and exploding or
    // dead, if so let's get a new target from the campaign
    if (targetPtr and 
        targetPtr->BaseData()->IsSim() and 
        (targetPtr->BaseData()->IsExploding() or not ((SimBaseClass *)targetPtr->BaseData())->IsAwake()))
    {
        ClearTarget();
    }

    // see if we've already got a target
    /* if ( targetPtr )
     {
     target = targetPtr->BaseData();

     // is it a campaign object? if not we can return....
     if (target->IsSim() )
     {
     return;
     }

     // itsa campaign object.  Check to see if its deagg'd
     if (((CampBaseClass*)target)->IsAggregate() )
     {
     // still aggregated, return
     return;
     }

     // the campaign object is now deaggregated, choose a sim entity
     // to target on it

     // M.N. use S.G.'s FindSimGroundTarget function to choose a sim entity

     simTarg = FindSimGroundTarget((CampBaseClass*)target, ((CampBaseClass*)target)->NumberOfComponents(), 0);

     if ( not simTarg) // another sanity check
     return;

     if ( not simTarg->IsExploding() and not simTarg->IsDead() and simTarg->pctStrength > 0.0f) // still alive?
     SetTargetEntity( simTarg );

     return;

     } // end if already targetPtr
    */

    // at this point we have no target, we're going to ask the campaign
    // to find out what we're supposed to hit

    // tell unit we haven't done any checking on it yet
    campUnit->UnsetChecked();

    // choose target.  I assume if this returns 0, no target....
    if ( not campUnit->ChooseTarget())
    {
        ClearTarget();
        // alternately try and choose the waypoint's target
        // SettargetPtr( self->curWaypoint->GetWPTarget() );
        return;
    }

    // get the target
    target = campUnit->GetTarget();

    // get tactic -- not doing anything with it now
    campUnit->ChooseTactic();
    campTactic = campUnit->GetUnitTactic();

    // sanity check and make sure its on ground, what to do if not?...
    if ( not target or
        campTactic == ATACTIC_RETROGRADE or
        campTactic == ATACTIC_IGNORE or
        campTactic == ATACTIC_AVOID or
        campTactic == ATACTIC_ABORT or
        campTactic == ATACTIC_REFUEL)
    {
        ClearTarget();
        return;
    }

    if (((CampBaseClass*)target)->IsAggregate())
    {
        // still aggregated, return
        SetTargetEntity(target);
        return;
    }

    // we've a SIM target, go get a component

    // M.N. use S.G.'s FindSimGroundTarget function to choose a sim entity
    if (target->IsSim() and target->OnGround())
    {
        simTarg = FindSimGroundTarget((CampBaseClass*)target, ((CampBaseClass*)target)->NumberOfComponents(), 0);

        if ( not simTarg) // another sanity check
            return;

        // set it as our target
        if ( not simTarg->IsExploding() and not simTarg->IsDead() and simTarg->pctStrength > 0.0f) // still alive?
            SetTargetEntity(simTarg);

        return;
    }

    SetTargetEntity(target);

}

// 2001-11-29 ADDED BY S.G. HELP FUNCTION TO SEARCH FOR A GROUND TARGET Modified for helis by M.N.
SimBaseClass *HeliBrain::FindSimGroundTarget(CampBaseClass *targetGroup, int targetNumComponents, int startPos)
{
    int i;
    int usComponents = self->GetCampaignObject()->NumberOfComponents();
    SimBaseClass *simTarg = NULL;
    SimBaseClass *firstSimTarg = NULL;
    HelicopterClass *flightMember[4] =  { 0 }; // Maximum of 4 planes per flight with no target as default

    // Get the flight helis (once per call instead of once per target querried)
    for (i = 0; i < usComponents; i++)
    {
        // I onced tried to get the player's current target so it could be skipped by the AI but
        // all the player's are not on the same PC as the AI so this is not valid.
        // Therefore, only get this from digital planes, or the player if he is local
        if (((HeliMMClass *)self->GetCampaignObject()->GetComponentEntity(i))->isDigital or ((HelicopterClass *)self->GetCampaignObject()->GetComponentEntity(i))->IsLocal())
        {
            flightMember[i] = (HelicopterClass *)self->GetCampaignObject()->GetComponentEntity(i);

            // Sanity check
            if ( not flightMember[i])
                continue;
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

        for (j = 0; j < usComponents; j++)
            if (flightMember[j]
               and flightMember[j]->hBrain
               and ((flightMember[j]->hBrain->targetPtr
                    and flightMember[j]->hBrain->targetPtr->BaseData() == simTarg)
                    or flightMember[j]->hBrain->targetHistory[0] == simTarg
                    or flightMember[j]->hBrain->targetHistory[1] == simTarg))
                break;  // Yes, ignore it.

        // If we didn't reach the end, someone else is using it so skip it.
        if (j not_eq usComponents)
            continue;

        // Mark this sim entity as the first target with a match (in case no emitting targets are left standing, or it's a feature)
        if ( not firstSimTarg)
            firstSimTarg = simTarg;

        // Is it an objective, break out
        if (targetGroup->IsObjective())
            break;

        // Look for the next one...
    }

    if (startPos < targetNumComponents)
    {
        // Keep track of the last two targets but only if we have one, otherwise, leave our previous targets alone
        if (firstSimTarg)
        {
            targetHistory[1] = targetHistory[0];
            targetHistory[0] = firstSimTarg;
        }
    }
    else
        firstSimTarg = 0;

    // JB 011017 from Schumi if targetNumComponents is less than usComponents, then of course there is no target anymore for the wingmen to bomb, and firstSimTarg is NULL.
    if (firstSimTarg == NULL and targetNumComponents and targetNumComponents < usComponents)
        firstSimTarg = targetGroup->GetComponentEntity(rand() % targetNumComponents);

    return firstSimTarg;
}



/*
** Name: SetTargetEntity
** Description:
** Creates a SimObjectType struct for the entity, sets the targetPtr,
** References the target.  Any pre-existing target is dereferenced.
*/
void HeliBrain::SetTargetEntity(FalconEntity *obj)
{
    if (obj not_eq NULL)
    {
        if (targetPtr not_eq NULL)
        {
            // release existing target data if different object
            if (targetPtr->BaseData() not_eq obj)
            {
                targetPtr->Release();
                targetPtr = NULL;
                targetData = NULL;
            }
            else
            {
                // already targeting this object
                return;
            }
        }

        // create new target data and reference it
#ifdef DEBUG
        //targetPtr = new SimObjectType( OBJ_TAG, self, obj );
#else
        targetPtr = new SimObjectType(obj);
#endif
        targetPtr->Reference();
        targetData = targetPtr->localData;
        // SetTarget( targetPtr );
    }
    else // obj is null
    {
        if (targetPtr not_eq NULL)
        {
            targetPtr->Release();
            targetPtr = NULL;
            targetData = NULL;
        }
    }
}

#if 0

void HeliBrain::TargetSelection(SimObjectType *tlist)
{

    float tmpAirRange = FLT_MAX;
    float tmpGndRange = FLT_MAX;
    SimObjectType *tmpObj;
    SimObjectType *tmpAirObj = NULL;
    SimObjectType *tmpGndObj = NULL;
    SimBaseClass *theObject;
    Team b;
    float range;

    // Loop until we find a target that is valid,
    // I now use the air flag to tell me whether the target is valid
    //

    // edg TODO: rather than check the class table, just use MOTION type flag
    // to determine air or ground.

    tmpObj = targetList = tlist;
    SetTarget(NULL);

    while (tmpObj)
    {

        // Targeting should be based in type
        theObject = (SimBaseClass*)tmpObj->BaseData();

        // edg : ERROR
        if ( not theObject or not tmpObj->localData)
        {
            // get next object
            tmpObj = tmpObj->next;

            continue;
        }

        // get team of potential target
        b = theObject->GetTeam();

        // ground targets
        if (theObject->OnGround())
        {

            if (GetRoE(side, b, ROE_GROUND_FIRE) == ROE_ALLOWED or
                GetRoE(side, b, ROE_GROUND_CAPTURE) == ROE_ALLOWED)
            {
                // favor ground units
                range = tmpObj->localData->range * 0.4f;

                // find closest
                if (range < tmpGndRange)
                {
                    tmpGndRange = range;
                    tmpGndObj = tmpObj;
                }
            }
        }
        // air targets
        else
        {
            // special case instant action -- only target ownship
            /*
            if ( SimDriver.RunningInstantAction() and theObject->IsSetFlag( MOTION_OWNSHIP ) )
            {
             SetTarget( tmpObj );
             return;
            }
            else
            */
            if (GetRoE(side, b, ROE_AIR_FIRE) == ROE_ALLOWED or
                GetRoE(side, b, ROE_AIR_ENGAGE) == ROE_ALLOWED or
                GetRoE(side, b, ROE_AIR_ATTACK) == ROE_ALLOWED)
            {
                // if the object is another helicopter, favor it
                if (theObject->IsSim() and theObject->IsHelicopter())
                    range = tmpObj->localData->range * 0.5f;
                else
                    range = tmpObj->localData->range * 2.0f;

                // find closest
                if (range < tmpAirRange)
                {
                    tmpAirRange = range;
                    tmpAirObj = tmpObj;
                }
            }
        }

        // get next object
        tmpObj = tmpObj->next;
    }

    // decide on either air or ground to target -- base on range

    // scale air range
    // tmpAirRange *= 0.75f;

    // at the moment at least, instant action only targets air
    /*
    if ( SimDriver.RunningInstantAction() )
    {
     SetTarget( tmpAirObj );
    }
    */
    // if they're both less than a certain range, flip a coin
    if (tmpAirRange < tmpGndRange)
    {
        SetTarget(tmpAirObj);
    }
    else
    {
        SetTarget(tmpGndObj);
    }
}
#endif

void HeliBrain::RunDecisionRoutines(void)
{
    /*-----------------------*/
    /* collision avoid check */
    /*-----------------------*/
    CollisionCheck();

    /*-----------*/
    /* Guns Jink */
    /*-----------*/
    GunsJinkCheck();
    // checks for missiles too
    GunsEngageCheck();

    /* Special cases for close in combat logic.                 */
    /* These maneuvers are started from within other maneuvers, */
    /* eg. "rollAndPull" and are self-terminating.            */

    /*------------------*/
    /* default behavior */
    /*------------------*/
    // if (isWing)
    //    AddMode (WingyMode);
    AddMode(WaypointMode);
}

void HeliBrain::PrtMode(void)
{
    AirAIModeMsg* modeMsg;

    if (curMode not_eq lastMode)
    {
        switch (curMode)
        {
            case RTBMode:
                PrintOnline("DIGI RTBMode");
                break;

            case WingyMode:
                PrintOnline("DIGI WINGMAN");
                break;

            case WaypointMode:
                PrintOnline("DIGI WaypointMode");
                break;

            case GunsEngageMode:
                PrintOnline("DIGI GUNS ENGAGE");
                break;

            case BVREngageMode:
                PrintOnline("DIGI BVR ENGAGE");
                break;

            case WVREngageMode:
                PrintOnline("DIGI WVR ENGAGE");
                break;

            case MissileDefeatMode:
                PrintOnline("DIGI MISSILE DEFEAT");
                break;

            case MissileEngageMode:
                PrintOnline("DIGI MSSLE ENGAGE");
                break;

            case GunsJinkMode:
                PrintOnline("DIGI GUNS JINK");
                break;

            case GroundAvoidMode:
                PrintOnline("DIGI GROUND AVOID");
                break;

            case LoiterMode:
                PrintOnline("DIGI LoiterMode");
                break;

            case RunAwayMode:
                PrintOnline("DIGI DISENGAGE");
                break;

            case OvershootMode:
                PrintOnline("DIGI OvershootMode");
                break;

            case CollisionAvoidMode:
                PrintOnline("DIGI COLLISION");
                break;

            case AccelerateMode:
                PrintOnline("DIGI AccelerateMode");
                break;

            case SeparateMode:
                PrintOnline("DIGI SeparateMode");
                break;

            case RoopMode:
                PrintOnline("DIGI RoopMode");
                break;

            case OverBMode:
                PrintOnline("DIGI OVERBANK");
                break;
        }

        modeMsg = new AirAIModeMsg(self->Id(), FalconLocalGame);
        modeMsg->dataBlock.whoDidIt = self->Id();
        modeMsg->dataBlock.newMode = curMode;
        FalconSendMessage(modeMsg, FALSE);
    }
}

void HeliBrain::PrintOnline(char *)
{
}

void HeliBrain::AddMode(DigiMode newMode)
{
    if (newMode < nextMode)
        nextMode = newMode;
}

void HeliBrain::ResolveModeConflicts(void)
{
    /*--------------------*/
    /* What were we doing */
    /*--------------------*/
    lastMode = curMode;
    curMode  = nextMode;
    nextMode = NoMode;
}
