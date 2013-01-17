#include "stdhdr.h"
#include "radarData.h"
#include "simmath.h"
#include "object.h"
#include "simbase.h"
#include "otwdrive.h"
#include "rwr.h"
#include "simmover.h"

///////////////////////////////////////////////////////////////////////////////
// Is the object within my field of view?                                    //
///////////////////////////////////////////////////////////////////////////////
int RwrClass::CanSeeObject (SimObjectType* rwrObj)
{
	if (
      rwrObj->localData->az < typeData->right &&
      rwrObj->localData->az > typeData->left &&
		rwrObj->localData->el < typeData->top &&
      rwrObj->localData->el > typeData->bottom)
		return TRUE;
	else
		return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
// Is the object radiating in my direction                                   //
///////////////////////////////////////////////////////////////////////////////
int RwrClass::BeingPainted (SimObjectType* rwrObj)
{
	if (rwrObj->BaseData()->IsSim())
	{
// JB 010727 RP5 RWR start
// 2001-02-23 MODIFIED BY S.G.
// SINCE I'VE NOTICED THE AI DOESN'T UPDATE THEIR RdrAzCenter/RdrElCenter FREQUENTLY ENOUGH, IT'S POSSIBLE FOR THE RWR TO 'THINK' IT IS NO LONGER
// BEING LOCKED BECAUSE IT NOW FALLS OUTSIDE THE CURRENT RADAR CONE. TO FIX THIS, I'LL ALWAYS USE THE FULL RADAR COVERAGE, EVEN FOR LOCKED TARGET, UNLESS
// WE ARE *NOT* THAT LOCKED TARGET. IN THAT CASE, I'LL LIMIT THE SCAN TO THE CURRENT RADAR CONE. THAT WAY, PLANES NOT CURRENTLY BEING LOCKED ON WON'T GET
// SPIKED, UNLESS THEY HAPPEN TO ENTER THE RADAR CONE
// I'LL COMMENT THE CURRENT SIM CODING AND WRITE MY OWN
		float scanAz;
		float scanEl;

		// Since targetPtr belongs to movers only, make sure it is one. Not sure if required but just to be safe. I've been burned top many time :-(
		// Only use the true radar scan volume if the contact's target isn't us...
		if (rwrObj->BaseData()->IsMover() && ((SimMoverClass *)rwrObj->BaseData())->targetPtr && ((SimMoverClass *)rwrObj->BaseData())->targetPtr->BaseData() != platform) {
			scanAz = ((SimBaseClass*)rwrObj->BaseData())->RdrAz();
			scanEl = ((SimBaseClass*)rwrObj->BaseData())->RdrEl();
			if ((scanAz > fabs(rwrObj->localData->azFrom - ((SimBaseClass*)rwrObj->BaseData())->RdrAzCenter())) &&
				(scanEl > fabs(rwrObj->localData->elFrom - ((SimBaseClass*)rwrObj->BaseData())->RdrElCenter())))
				return TRUE;
			else
				return FALSE;
		}
		else {
			scanAz = scanEl = RadarDataTable[rwrObj->BaseData()->GetRadarType()].ScanHalfAngle;
			if ((scanAz > fabs(rwrObj->localData->azFrom)) && (scanEl > fabs(rwrObj->localData->elFrom)))
				return TRUE;
			else
				return FALSE;
		}
	}
	else
	{
		// TODO:  Can we use localData here?  Not sure, so lets be safe in the short term
		float dx = rwrObj->BaseData()->XPos() - platform->XPos();
		float dy = rwrObj->BaseData()->YPos() - platform->YPos();
		float brg = (float)atan2( dx, dy );//me123 switched x and y
		float angleOff = (float)fmod( fabs( brg - rwrObj->BaseData()->Yaw() ), PI );

		if (angleOff < RadarDataTable[rwrObj->BaseData()->GetRadarType()].ScanHalfAngle)
			return TRUE;
		else
			return FALSE;
	}
// JB 010727 RP5 RWR end
/*
	if (0) //me123 test (rwrObj->BaseData()->IsSim())
	{
		float scanAz = ((SimBaseClass*)rwrObj->BaseData())->RdrAz();
		float scanEl = ((SimBaseClass*)rwrObj->BaseData())->RdrEl();

		// TODO:  Use the search volumes correctly
		// RdrAzCenter()
		// RdrElCenter()

		if ((scanAz > fabs(rwrObj->localData->azFrom)) &&
			(scanEl > fabs(rwrObj->localData->elFrom)))
		{
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	} 
	else 
	{
		// TODO:  Can we use localData here?  Not sure, so lets be safe in the short term
		float dx = rwrObj->BaseData()->XPos() - platform->XPos();
		float dy = rwrObj->BaseData()->YPos() - platform->YPos();
		float brg = (float)atan2( dx, dy );//me123 switched x and y
		float angleOff = (float)fmod( fabs( brg - rwrObj->BaseData()->Yaw() ), PI );

		if (angleOff < RadarDataTable[rwrObj->BaseData()->GetRadarType()].ScanHalfAngle)
		{
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}
*/
}


///////////////////////////////////////////////////////////////////////////////
// Is the signal strong enough to detect and not occluded by terrain?        //
///////////////////////////////////////////////////////////////////////////////
int RwrClass::CanDetectObject (SimObjectType* rwrObj)
{
float radarRange;

	
	// Decide what the emitters strenth (expressed in feet of range) is.
	if (rwrObj->BaseData()->IsSim())
		// This gets set to 0 when the radar is off
		radarRange = ((SimBaseClass*)rwrObj->BaseData())->RdrRng();
	else {
		if (rwrObj->BaseData()->IsEmitting()) {
			radarRange = RadarDataTable[rwrObj->BaseData()->GetRadarType()].NominalRange;
		} else {
			radarRange = 0.0f;
		}
	}
	
	// If the signal is strong enough for detection, make sure we have line of sight
   // radarRange * nominalRange is the range that the object can be detected at
	//if (radarRange < typeData->nominalRange && CanDetectObject(rwrObj->BaseData()))
	if (
      (rwrObj->localData->range < radarRange * typeData->nominalRange) &&
      platform->CheckLOS(rwrObj))
	{
		return TRUE;
	}

	return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
// Is the signal not occluded by terrain?        //
///////////////////////////////////////////////////////////////////////////////
int RwrClass::CanDetectObject (FalconEntity* rwrObj)
{
float top, bottom;

	// See if the target is near the ground
	OTWDriver.GetAreaFloorAndCeiling (&bottom, &top);
	if (rwrObj->ZPos() > top)
	{
		// Check for occulsion by terrain
		if (OTWDriver.CheckLOS( platform, rwrObj )) {
			return TRUE;
		}
	}

	return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
