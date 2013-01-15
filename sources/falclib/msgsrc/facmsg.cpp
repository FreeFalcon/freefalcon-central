/*
 * Machine Generated source file for message "FAC Message".
 * NOTE: The functions here must be completed by hand.
 * Generated on 27-January-1997 at 18:34:07
 * Generated from file EVENTS.XLS by Leon Rosenshein
 */

#include "MsgInc/FACMsg.h"
#include "mesg.h"
#include "facbrain.h"
#include "object.h"
#include "aircrft.h"
#include "falclib.h"
#include "falcmesg.h"
#include "falcgame.h"
#include "falcsess.h"
#include "simdrive.h"

//sfr: added here for checks
#include "InvalidBufferException.h"

FalconFACMessage::FalconFACMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) : FalconEvent (FACMsg, FalconEvent::SimThread, entityId, target, loopback)
{
	// Your Code Goes Here
}

FalconFACMessage::FalconFACMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target) : FalconEvent (FACMsg, FalconEvent::SimThread, senderid, target)
{
	// Your Code Goes Here
	type;
}

FalconFACMessage::~FalconFACMessage(void)
{
	// Your Code Goes Here
}

int FalconFACMessage::Process(uchar autodisp)
{
	SimVehicleClass* theEntity;
	SimVehicleClass* theFighter;

	if (autodisp)
		return 0;

	theEntity = (SimVehicleClass*)Entity();
	if (!theEntity)
		theEntity = (SimVehicleClass*)(vuDatabase->Find (EntityId()));
	theFighter = (SimVehicleClass*)(vuDatabase->Find (dataBlock.caller));

	if(!theFighter) // PJW: E3 Hack... make sure (theFighter) is valid
		return 0;

	switch (dataBlock.type)
	{
			case CheckIn:
					if (theEntity && theEntity->IsLocal())
						((FACBrain*)theEntity->Brain())->AddToQ(theFighter);

					// Play message here
					if (theFighter != SimDriver.GetPlayerEntity())
					{
					}
					break;

			case Wilco:
					// Play message here
					if (theFighter != SimDriver.GetPlayerEntity())
					{
					}
					break;

			case Unable:
					// Play message here
					if (theFighter != SimDriver.GetPlayerEntity())
					{
					}
					break;

			case In:
					// Play message here
					if (theFighter != SimDriver.GetPlayerEntity())
					{
					}
					break;

			case Out:
					// Play message here
					if (theFighter != SimDriver.GetPlayerEntity())
					{
					}
					break;

			case RequestMark:
					break;

			case RequestTarget:
					if (theEntity && theFighter && theEntity->IsLocal())
						((FACBrain*)theEntity->Brain())->RequestTarget(theFighter);
					break; 

			case RequestBDA:
					if (theEntity && theFighter && theEntity->IsLocal())
						((FACBrain*)theEntity->Brain())->RequestBDA(theFighter);
					break;

			case RequestLocation:
					if (theEntity && theEntity->IsLocal())
						((FACBrain*)theEntity->Brain())->RequestLocation();
					break;

			case RequestTACAN:
					if (theEntity && theEntity->IsLocal())
						((FACBrain*)theEntity->Brain())->RequestTACAN();
					break;

			case HoldAtCP:
					break;

			case FacSit:
					break;

			case Mark:
					break;

			case NoTargets:
					break;

			case GroundTargetBr:
					break;

			case BDA:
					break;

			case NoBDA:
					break;

			case ReattackQuery:
					if (theFighter && theFighter->IsLocal())
					{
					}
					break;

			case HartsTarget:
					break;

			case HartsOpen:
					if (theFighter && theFighter->IsLocal())
					{
					}
					break;

			case ScudLaunch:
					break;

			case SanitizeLZ:
					break;

			case AttackMyTarget:
					if (theFighter && theFighter->IsLocal())
					{
					}
					break;

			case SendChoppers:
					break;
	};

	return 0;
}

