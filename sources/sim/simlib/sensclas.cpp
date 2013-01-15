#include "stdhdr.h"
#include "sensclas.h"
#include "object.h"
#include "geometry.h"
#include "handoff.h"
#include "Graphics/Include/display.h"
#include "simmover.h"

SensorClass::SensorClass(SimMoverClass* self)
{
	platform = self;
	lockedTarget = NULL;
	seekerAzCenter = 0.0f;
	seekerElCenter = 0.0f;
	isOn = TRUE;
#if !NO_REMOTE_BUGGED_TARGET
	RemoteBuggedTarget = NULL;
#endif
}


SensorClass::~SensorClass(void)
{
	ClearSensorTarget();
}


void SensorClass::SetDesiredTarget( SimObjectType* newTarget )
{
	SetSensorTarget( newTarget );
}


void SensorClass::SetSensorTarget( SimObjectType* newTarget )
{
	if (newTarget == lockedTarget){
		return;
	}
	
	// Drop our previously locked target
	if (lockedTarget){
		lockedTarget->Release();
	}
	
	// Aquire the new one
	if (newTarget){
		newTarget->Reference();
	}

	lockedTarget = newTarget;
}


void SensorClass::SetSensorTargetHack( FalconEntity* newTarget )
{
	SimObjectType	*tgt;

	// Aquire the new one (up to others to keep relative geometry up to date)
	if (newTarget){
		tgt = new SimObjectType (newTarget);
	} 
	else {
		tgt = NULL;
	}

	SetSensorTarget( tgt );
}


void SensorClass::ClearSensorTarget(){
	// Drop our previously locked target
	if (lockedTarget) 
	{
		lockedTarget->Release();
		lockedTarget = NULL;
	}
}


/*
** Name: CheckLockedTarget
** Description:
**		If we have a current locked target do some checks on it: Is at
**		a campaign unit?, has it been removed from the bubble?  has it
**		been deagg'd/agg'd?.....  We may have to change the lock back and
**		forth between a camp unit and its components.....
*/
void SensorClass::CheckLockedTarget()
{
	SimObjectType	*newTarget;

	// if no target, nothing to validate
	if (lockedTarget == NULL )
	{
		return;
	}

	// Run the handoff routine
	if (sensorType == HTS || sensorType == RWR) {
		newTarget = SimCampHandoff( lockedTarget, platform->targetList, HANDOFF_RADAR );
	}
	else {
		newTarget = SimCampHandoff( lockedTarget, platform->targetList, HANDOFF_RANDOM );
	}

	// Stop now if our current target is still fine
	if (newTarget == lockedTarget) {
		return;
	}

	// If we get here, we need to exchange targets
	SetSensorTarget( newTarget );
}


//Helper Function to find a given sensor
SensorClass* FindSensor (SimMoverClass* theObject, int sensorType)
{
	int i;
	SensorClass* retval = NULL;

	ShiAssert (theObject);

	//JAM 25Nov03 - CTD Fix (Why am I getting spiked by an object with no sensor array, and an insane number of sensors???)
	// MLR 1/26/2004 - something is bent somewhere, the sensorArray may be NULL with numSensors>0
	if (theObject && theObject->sensorArray && theObject->numSensors && theObject->numSensors < 12)
	{
		for (i=0; theObject && i < theObject->numSensors; i++)
		{
			ShiAssert( theObject->sensorArray[i] );
			if (((theObject->sensorArray[i]) == (SensorClass*)0xbaadf00d)||((theObject->sensorArray[i]) == (SensorClass*)0xfeeefeee))
				continue;
			if (theObject && (theObject->sensorArray[i]) && (theObject->sensorArray[i]->Type() == sensorType))
			{
				retval = theObject->sensorArray[i];
				break;
			}
		}
	}

	return retval;
}


