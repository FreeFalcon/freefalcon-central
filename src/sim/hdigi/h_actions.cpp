#include "stdhdr.h"
#include "hdigi.h"
#include "simveh.h"
#include "object.h"
#include "unit.h"

void HeliBrain::Actions(void)
{
	// stick/throttle commands based on current mode
	if ( lastMode != curMode )
   		onStation = NotThereYet;

	// RV - Biker - Switch to next WP if flight lead already did
	if (self->flightLead->curWaypoint->GetWPArrivalTime() > self->curWaypoint->GetWPArrivalTime())
		SelectNextWaypoint();

	if (self->flightLead != self && (targetPtr == NULL || !anyWeapons || targetPtr->localData->range > 5.0f * NM_TO_FT)) {
		// if ( modeStack.curMode == GunsEngageMode )
	   	//	GunsEngage();
		// else
	   	FollowLead();
	}
	else {
		switch (curMode) {

			case RTBMode:
				// GoHome();
				break;

			case FollowOrdersMode:
				FollowOrders();
				break;

			case WingyMode:
				FollowLead();
				break;

			case WaypointMode:      
				FollowWaypoints();
				break;

			case BVREngageMode:      
				if (targetPtr == maxTargetPtr)
					RollAndPull();
				break;

			case WVREngageMode:
				if (targetPtr == maxTargetPtr)
					RollAndPull();
				break;

			case GunsEngageMode:
				if (targetPtr) {
					GunsEngage();
				}
				else {
					Loiter();
				}
				break;

			case MissileEngageMode:
				MissileEngage();
				break;

			case MissileDefeatMode:
				MissileDefeat();
				break;

			case GunsJinkMode:
				// GunsJink();
				break;

			case LoiterMode:
				Loiter();
				break;

			case RunAwayMode:
				// GoHome();
				break;

			case CollisionAvoidMode:
				CollisionAvoid();
				break;

			case AccelerateMode:
				// Accelerate();
				break;

			case OvershootMode:
				// OverShoot();
				break;

			case SeparateMode:
				// Separate();
				break;

			case RoopMode:
				RollOutOfPlane();
				break;

			case OverBMode:
				OverBank(30.0F*DTR);
				break;

			default:   
				//SimLibPrintError("%s digi.w: Invalid digi mode %d\n", self->Id().num_, curMode);
				FollowWaypoints();
				break;
		}
	}
	
	// RV - Biker - We don't need this anymore
	//GroundCheck();
}
