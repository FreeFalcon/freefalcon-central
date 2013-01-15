#include "stdhdr.h"
#include "hdigi.h"
#include "sensors.h"
#include "falcmesg.h"
#include "simveh.h"
#include "fcc.h"
#include "sms.h"
#include "object.h"
#include "MsgInc\airaimodechange.h"
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
   // if (targetPtr && shooting)
   //     FireControl();
}


void HeliBrain::TargetSelection( void )
{
	UnitClass *campUnit = (UnitClass *)self->GetCampaignObject();
	FalconEntity *target;
	int i, numComponents;
	SimBaseClass *simTarg;
	int campTactic;

	// sanity check
	if ( !campUnit )
		return;

	// check to see if our current ground target is a sim and exploding or
	// dead, if so let's get a new target from the campaign
	if ( targetPtr &&
		 targetPtr->BaseData()->IsSim() &&
		 ( targetPtr->BaseData()->IsExploding() ||
		 !((SimBaseClass *)targetPtr->BaseData())->IsAwake() ) )
	{
		ClearTarget( );
	}

	// see if we've already got a target
	if ( targetPtr )
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
		numComponents = ((CampBaseClass*)target)->NumberOfComponents();

		for ( i = 0; i < numComponents; i++ )
		{
			simTarg = ((CampBaseClass*)target)->GetComponentEntity( rand() % numComponents );
			if ( !simTarg ) //sanity check
				continue;

			// don't target runways (yet)
			if ( // !simTarg->IsSetCampaignFlag (FEAT_FLAT_CONTAINER) &&
				!simTarg->IsExploding() &&
				!simTarg->IsDead() &&
				simTarg->pctStrength > 0.0f )
			{
				SetTargetEntity( simTarg );
				break;
			}
		} // end for # components


		return;

	} // end if already targetPtr


	// at this point we have no target, we're going to ask the campaign
	// to find out what we're supposed to hit

	// tell unit we haven't done any checking on it yet
	campUnit->UnsetChecked();

	// choose target.  I assume if this returns 0, no target....
	if ( !campUnit->ChooseTarget() )
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

	// sanity check and make sure its on ground, what to do if not?!...
	if ( !target ||
		  campTactic == ATACTIC_RETROGRADE ||
		  campTactic == ATACTIC_IGNORE ||
		  campTactic == ATACTIC_AVOID ||
		  campTactic == ATACTIC_ABORT ||
		  campTactic == ATACTIC_REFUEL  )
	{
		ClearTarget();
		return;
	}


	// set it as our target
	SetTargetEntity( target );

}

/*
** Name: SetTargetEntity
** Description:
**		Creates a SimObjectType struct for the entity, sets the targetPtr,
**		References the target.  Any pre-existing target is dereferenced.
*/
void HeliBrain::SetTargetEntity( FalconEntity *obj )
{
	if ( obj != NULL )
	{
		if ( targetPtr != NULL )
		{
			// release existing target data if different object
			if ( targetPtr->BaseData() != obj )
			{
				targetPtr->Release( SIM_OBJ_REF_ARGS );
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
		targetPtr = new SimObjectType( OBJ_TAG, self, obj );
		#else
		targetPtr = new SimObjectType( obj );
		#endif
		targetPtr->Reference( SIM_OBJ_REF_ARGS );
		targetData = targetPtr->localData;
		// SetTarget( targetPtr );
	}
	else // obj is null
	{
		if ( targetPtr != NULL )
		{
			targetPtr->Release( SIM_OBJ_REF_ARGS );
			targetPtr = NULL;
			targetData = NULL;
		}
	}
}

#if 0

void HeliBrain::TargetSelection(SimObjectType *tlist)
{

	float					tmpAirRange = FLT_MAX;
	float					tmpGndRange = FLT_MAX;
	SimObjectType			*tmpObj;
	SimObjectType			*tmpAirObj = NULL;
	SimObjectType			*tmpGndObj = NULL;
	SimBaseClass			*theObject;
	Team					b;
	float					range;

	// Loop until we find a target that is valid, 
	// I now use the air flag to tell me whether the target is valid
	//

	// edg TODO: rather than check the class table, just use MOTION type flag
	// to determine air or ground.

	tmpObj = targetList = tlist;
	SetTarget( NULL );
	while (tmpObj)
	{

		// Targeting should be based in type
		theObject = (SimBaseClass*)tmpObj->BaseData();

		// edg : ERROR!
		if ( !theObject || !tmpObj->localData )
		{
			// get next object
			tmpObj = tmpObj->next;

			continue;
		}

		// get team of potential target
		b = theObject->GetTeam();

		// ground targets
		if ( theObject->OnGround() )
		{

			if ( GetRoE( side, b, ROE_GROUND_FIRE ) == ROE_ALLOWED ||
				 GetRoE( side, b, ROE_GROUND_CAPTURE ) == ROE_ALLOWED )
			{
				// favor ground units
				range = tmpObj->localData->range * 0.4f;

				// find closest
				if ( range < tmpGndRange )
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
			if ( SimDriver.RunningInstantAction() && theObject->IsSetFlag( MOTION_OWNSHIP ) )
			{
				SetTarget( tmpObj );
				return;
			}
			else
			*/
			if ( GetRoE( side, b, ROE_AIR_FIRE ) == ROE_ALLOWED ||
				 GetRoE( side, b, ROE_AIR_ENGAGE ) == ROE_ALLOWED ||
				 GetRoE( side, b, ROE_AIR_ATTACK ) == ROE_ALLOWED )
			{
				// if the object is another helicopter, favor it
				if ( theObject->IsSim() && theObject->IsHelicopter() )
					range = tmpObj->localData->range * 0.5f;
				else
					range = tmpObj->localData->range * 2.0f;

				// find closest
				if ( range < tmpAirRange )
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
	if ( tmpAirRange < tmpGndRange )
	{
		SetTarget( tmpAirObj );
	}
	else
	{
		SetTarget( tmpGndObj );
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

   if (curMode != lastMode)
   {
      switch(curMode)
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

      modeMsg = new AirAIModeMsg (self->Id(), FalconLocalGame);
      modeMsg->dataBlock.whoDidIt = self->Id();
      modeMsg->dataBlock.newMode = curMode;
      FalconSendMessage (modeMsg,FALSE);
   }
}

void HeliBrain::PrintOnline(char *)
{
}

void HeliBrain::AddMode (DigiMode newMode)
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
