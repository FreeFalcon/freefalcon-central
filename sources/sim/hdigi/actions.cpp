#include "stdhdr.h"
#include "hdigi.h"
#include "simveh.h"
#include "object.h"

void HeliBrain::Actions(void)
{
   /*-----------------------------------------------*/ 
   /* stick/throttle commands based on current mode */
   /*-----------------------------------------------*/
   if ( lastMode != curMode )
   		onStation = NotThereYet;

   if ( self->flightLead != self && (targetPtr == NULL || !anyWeapons || targetPtr->localData->range > 5.0f * NM_TO_FT ) )
   {
	   /*
	   if ( modeStack.curMode == GunsEngageMode )
	   		GunsEngage();
	   else
	   */
	   FollowLead();
   }
   else switch (curMode)
   {
      /*----------------*/
      /* return to base */
      /*----------------*/
      case RTBMode:
//         GoHome();
      break;

      /*-------------------*/
      /* Do what your told */
      /*-------------------*/
      case FollowOrdersMode:
         FollowOrders();
      break;

      /*---------------------*/
      /* follow wingman lead */
      /*---------------------*/
      case WingyMode:
         FollowLead();
      break;

      /*------------------*/
      /* follow waypoints */
      /*------------------*/
      case WaypointMode:      
         FollowWaypoints();
      break;

      /*------------*/
      /* BVR engage */
      /*------------*/
      case BVREngageMode:      
         if (targetPtr == maxTargetPtr)
                RollAndPull();
      break;

      /*------------*/
      /* WVR engage */
      /*------------*/
      case WVREngageMode:
         if (targetPtr == maxTargetPtr)
                RollAndPull();
      break;

      case GunsEngageMode:
         if (targetPtr)
            GunsEngage();
      break;

      /*-----------------------------------------*/
      /* Inside missile range, try to line it up */
      /*-----------------------------------------*/
      case MissileEngageMode:
         MissileEngage();
      break;

      /*----------------*/
      /* missile defeat */
      /*----------------*/
      case MissileDefeatMode:
         MissileDefeat();
      break;

      /*-----------*/
      /* guns jink */
      /*-----------*/
      case GunsJinkMode:
         GunsJink();
      break;

      /*--------*/
      /* loiter */
      /*--------*/
      case LoiterMode:
         Loiter();
      break;

      /*----------*/
      /* run away */
      /*----------*/
      case RunAwayMode:
//         GoHome();
      break;

      /*-----------------*/
      /* collision avoid */
      /*-----------------*/
      case CollisionAvoidMode:
         CollisionAvoid();
      break;

      /*------------*/
      /* accelerate */
      /*------------*/
      case AccelerateMode:
//         Accelerate();
      break;

      /*-----------*/
      /* overshoot */
      /*-----------*/
      case OvershootMode:
//         OverShoot();
      break;

      /*----------*/
      /* Separate */
      /*----------*/
      case SeparateMode:
//         Separate();
      break;

      /*-------------------*/
      /* Roll Out of Plane */
      /*-------------------*/
      case RoopMode:
         RollOutOfPlane();
      break;

      /*-----------*/
      /* Over Bank */
      /*-----------*/
      case OverBMode:
         OverBank(30.0F*DTR);
      break;

      default:   
         SimLibPrintError("%s digi.w: Invalid digi mode %d\n",
                 self->Id().num_, curMode);
         FollowWaypoints();
      break;

   } /*switch*/

   /*-----------------------------------------------------------------*/
   /* Ground avoid now runs concurrently with whatever mode has been  */
   /* selected by the conflict resolver. Ground avoid modifies the    */
   /* current Pstick and Rstick commands.                             */
   /*-----------------------------------------------------------------*/
   GroundCheck();
   /*
   if (groundAvoidNeeded)
      PullUp();
   */
}
