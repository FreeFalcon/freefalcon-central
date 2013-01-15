#include "stdhdr.h"
#include "simfile.h"
#include "object.h"
#include "eyeball.h"
#include "Graphics\Include\display.h"
#include "Graphics\Include\tod.h"
#include "simbase.h"
#include "entity.h"
#include "simmath.h"
#include "simbase.h"
#include "camp2sim.h"
#include "weather.h"
/* S.G. FOR AIRCRAFT DAMAGE */ #include "aircrft.h"
/* S.G. FOR AIRCRAFT DUST/WATER TRAIL */ #include "airframe.h"
/* S.G. FOR SKILL LEVEL */ #include "digi.h"
/* S.G. FOR SKILL LEVEL */ #include "Classtbl.h"

/* M.N. for draw radius */ #include "Graphics\Include\Drawobj.h"

extern bool g_bEnableWeatherExtensions;
extern bool g_bAddACSizeVisual;
extern float g_fVisualNormalizeFactor;

extern int g_nAIVisualRetentionTime; // 2002-03-12 S.G. How long before AI looses the lock.
extern int g_nAIVisualRetentionSkill; // 2002-03-12 S.G. How long before AI looses the lock (skill base)

EyeballClass::EyeballClass (int idx, SimMoverClass* self) : VisualClass (idx, self)
{
   visualType = EYEBALL;
}

SimObjectType* EyeballClass::Exec (SimObjectType* newTargetList)
{
SimObjectType	*newLock;
SimObjectType* tmpPtr = newTargetList;


	if (!lockedTarget)
		return lockedTarget;

	// Validate our locked target
	CheckLockedTarget();
	newLock = lockedTarget;
	
	// Decide if we can still see our locked target
	if (lockedTarget) {

		// Can't hold a lock if its outside our sensor cone
		if (!CanSeeObject( lockedTarget ))
		{
			newLock = NULL;
		}

		// Can't hold lock if the object is too far away or is occluded
		if ( !CanDetectObject (lockedTarget) )
		{
			newLock = NULL;
		}
	}

// ADDED BY S.G. SO PILOT WILL TRY TO REACQUIRE (ACTUALLY, STAY ON TARGET) A TARGET WHEN LOST. THIS TIME IS SKILL BASED.
	// If not a vehicle, defaults to 32 seconds, otherwise it's 24 (recruits) to 32 (ace) seconds, skill based
	int retentionTime = 32 * SEC_TO_MSEC;

	// Now look if we (ourself) are a vehicle. Only vehicle have a brain.
	Falcon4EntityClassType	*classPtr = (Falcon4EntityClassType*)platform->EntityType();
	// If we are, get our skill and from it, calculate the 'retention time'
	if (classPtr->dataType == DTYPE_VEHICLE && ((SimVehicleClass *)platform) && ((SimVehicleClass *)platform)->Brain())
		retentionTime = ((SimVehicleClass *)platform)->Brain()->SkillLevel() * g_nAIVisualRetentionSkill + g_nAIVisualRetentionTime;
// END OF ADDED SECTION

	// Look for a target
   while (tmpPtr)
   {
		// Can't hold a lock if its outside our sensor cone
		if (CanSeeObject( tmpPtr ) && CanDetectObject ( tmpPtr ) )
		{
			tmpPtr->localData->sensorState[Type()] = SensorTrack;
			tmpPtr->localData->sensorLoopCount[Type()] = SimLibElapsedTime;
			if (newLock == NULL && lockedTarget == NULL)
				{
				newLock = tmpPtr;//Cobra added this because new detects are being passedto newLock
				}
		}
      else
		{
// ADDED BY S.G. SO AI WILL 'LOOSE' SIGHT OF ITS TARGET IF IT 'DISAPPEARED' LONG ENOUGH
		  if ((unsigned int)tmpPtr->localData->sensorLoopCount[Type()] + retentionTime < SimLibElapsedTime)
// END OF ADDED SECTION
				tmpPtr->localData->sensorState[Type()] = NoTrack;
		}
		tmpPtr = tmpPtr->next;
   }

	// Update our lock
// ADDED BY S.G. THIS CODE WILL SET THE LOCK IF newLock IS NON NULL OR IF IT IS NULL AND THE TARGET WAS LOST LONG ENOUGH
    if (newLock == NULL && lockedTarget && (unsigned int)lockedTarget->localData->sensorLoopCount[Visual] + retentionTime >= SimLibElapsedTime)
	    SetSensorTarget( lockedTarget );
	else
// END OF ADDED SECTION
	    SetSensorTarget( newLock );

	// Tell the base class where we're looking
	if (lockedTarget)
	{
		// WARNING:  This is inconsistent with the rest of the sensors.  They all use
		// SetSeekerPos for this and pass in TargetAz and TargetEl which are NOT body rolled or pitched
		// I'm leaving this for now 'cause it might possibly break something if I "fix" it, but it really
		// should be fixed.
		seekerAzCenter = lockedTarget->localData->az;
		seekerElCenter = lockedTarget->localData->el;
		lockedTarget->localData->sensorState[Visual] = SensorTrack;
// ADDED BY S.G. ONLY UPDATE THE LOCK TIME IF WE HAVE A VISUAL LOCK ON HIM
		if (lockedTarget == newLock)
// END OF ADDED SECTION
	        lockedTarget->localData->sensorLoopCount[Visual] = SimLibElapsedTime;
	}

	return lockedTarget;
}


EyeballClass::~EyeballClass (void)
{
}


float EyeballClass::GetSignature (SimObjectType* obj)
{
float bonus = 1.25F;
float objAlt = -obj->BaseData()->ZPos() * 0.001F;

FalconEntity *object = obj->BaseData();
SimBaseClass *theObject = NULL;
SimObjectLocalData* localData=NULL;
localData = obj->localData;

float bogeyAngle = fabsf(localData->ataFrom*RTD);
float bogeyRange = localData->range*FT_TO_NM;

	//
	// This section handles visibilty based on visual artifacts like contrail, smoke and external lights.
	//

//Cobra  We are going to remake this using the F-16 and NM as the standard unit
//The previous code below is unintelligble doing a signature
//The standard will be an 1) F-16 2)Daytime (noon)
//We then add or subtract from this NM distance based on various conditions
//This function will return the detection distance in NM

	float visDetNM = 5.0f; //The standard Cobra
	float light = TheTimeOfDay.GetLightLevel();
	float visDetMod = 0.0f; //This adds or subtracts

	// Must be an airplane sim object
	if (object->IsSim())
		{
		if (object->IsAirplane())
			{
		    AircraftClass *aircraft = (AircraftClass *)object;
			Falcon4EntityClassType* classPtr = (Falcon4EntityClassType*)aircraft->EntityType();
			//aircraft->dropFlareCmd == TRUE;
			//Night
			if (light < 0.3)
				{
				// Are the exterior lights turned on?
 				if (
					aircraft->IsAcStatusBitsSet(AircraftClass::ACSTATUS_EXT_LIGHTS) ||
					aircraft->IsAcStatusBitsSet(AircraftClass::ACSTATUS_EXT_NAVLIGHTS) || 
					aircraft->IsAcStatusBitsSet(AircraftClass::ACSTATUS_EXT_TAILSTROBE) ||
					aircraft->IsAcStatusBitsSet(AircraftClass::ACSTATUS_EXT_LANDINGLIGHT)
				){
					visDetMod = max (visDetMod, 30); 
				}
				//Are we in Afterburner?
				if (aircraft->af->GetHasAB() && aircraft->PowerOutput() > 1.0f && bogeyAngle > 20.0f)
					{
						visDetMod = max (visDetMod, 30);
					}

				if (aircraft->dropFlareCmd == TRUE)
					{
						visDetMod = max (visDetMod, 30);
					}

				//Vis penalty for night
				if (visDetMod == 0.0f)
					{
						visDetNM = 0.5f;
					}
				else 
					{
						visDetNM = visDetMod;
					}
				//we return 1 for detected
				if (bogeyRange <= visDetNM)
					{
					return (1.0f);
					}
				else 
					{
					return (0.0f);
					}

				}
			//DAY
			else
				{
			// Contrails
			if (objAlt > ((WeatherClass*)realWeather)->contrailLow &&
			   objAlt < ((WeatherClass*)realWeather)->contrailHigh)
			{
					visDetMod = 10.0f;
			}
			// Damaged smoking
			if (platform->pctStrength < 0.5f)
				{
				visDetMod = max(visDetMod, 10.0f);
				}
				// Damaged smoking
				if (platform->pctStrength < 0.5f)
				{
					visDetMod = max(visDetMod, 10.0f);
				}
				// MIL smoking aircraft
				if (!aircraft->OnGround() &&
					aircraft->PowerOutput() <= 1.0f && aircraft->PowerOutput() > 0.90f)
				{
					float smoke = aircraft->af->EngineSmokeFactor();
					if (smoke == 2)
					{
						visDetMod = max (visDetMod, 7);
					}
					else if (smoke == 4)
					{
						visDetMod = max (visDetMod, 10);
					}
				}

				// 2000-10-16 ADDED BY S.G. SO DUST/WATER TRAIL IMPROVES VISIBILITY, BUT NOT AS MUCH AS CONTRAILS OR ENGINE SMOKE...
				// Altitude is within 10 to 80 feet off the ground and we're not over a runway
				//if (aircraft->ZPos() - aircraft->af->groundZ >= -80.0f && object->ZPos() - ((AircraftClass *)object)->af->groundZ <= -10.0f && !aircraft->af->IsSet(AirframeClass::OverRunway))
				//bonus *= 2.0F;
				// END OF ADDED SECTION (WITHIN CODE I ADDED)

				// Are the exterior lights turned on?
				if (aircraft->IsAcStatusBitsSet(AircraftClass::ACSTATUS_EXT_LIGHTS)){
					visDetMod = max(visDetMod, 10);
				}

				if (aircraft->IsAcStatusBitsSet(AircraftClass::ACSTATUS_EXT_NAVLIGHTS)){
					visDetMod = max(visDetMod, 10);
				}
				
				if (aircraft->IsAcStatusBitsSet(AircraftClass::ACSTATUS_EXT_TAILSTROBE)){
					visDetMod = max(visDetMod, 10);
				}

				if (aircraft->IsAcStatusBitsSet(AircraftClass::ACSTATUS_EXT_LANDINGLIGHT)){
					visDetMod = max(visDetMod, 10);
				}

				if (aircraft->dropFlareCmd == TRUE)	{
					visDetMod = max (visDetMod, 30);
				}	

				//Relative Size to F-16
				//Length * Span = 1504
				float afSize = (aircraft->af->GetAeroData(AeroDataSet::Length) *
								aircraft->af->GetAeroData(AeroDataSet::Span));
				float afCompPercent = afSize/1504;
				float afVisDetNM = visDetNM * afCompPercent;
				afVisDetNM = min(afVisDetNM, 10); //Cap at 10 miles vis
				if (visDetMod == 5 && afVisDetNM < visDetMod)
				{
					visDetMod = afVisDetNM;
				}
				else
				{
					visDetMod = max (visDetMod, afVisDetNM);
				}
			}
		}
	}
	if (bogeyRange <= visDetMod)
	{
		//We return 1 for detected
		return (1.0f);
	}
	else 
	{
		return (0.0f);
	}

	//
	// This section gets the visual signature of the target
	//

	// skill defaults to 5 (ace + 1)
	/*int skill = 5;
	float visualSignature = 1.0f;

	// Now look if we (ourself) are a vehicle. Only vehicle have a brain.
	Falcon4EntityClassType	*classPtr = (Falcon4EntityClassType*)platform->EntityType();
	// If we are, get our skill + 1
	if (classPtr->dataType == DTYPE_VEHICLE && 
		// S.G. SHOULDN'T BE REQUIRED, ALL VEHICLES ARE ASSIGNED A BRAIN...
		// O.W. WRONG ASSUMPTION DUDE >:)
		((SimVehicleClass *)platform) && ((SimVehicleClass *)platform)->Brain())//me123 addet brain check to avoid CTD
		skill = ((SimVehicleClass *)platform)->Brain()->SkillLevel() + 1;

	// Now if we have a locked target and that target is a vehicle, get his signature.
	// The visual signature is stored as a byte at offset 0x9D of the falcon4.vcd structure.
	// This byte, as well as 0x9E and 0x9F are used for padding originally.
	// The value range will be 0 to 4 with increments of 0.015625
	if (lockedTarget) {
		classPtr = (Falcon4EntityClassType*)lockedTarget->BaseData()->EntityType();
		if (classPtr->dataType == DTYPE_VEHICLE) {
			unsigned char *pVisSign = (unsigned char *)classPtr->dataPtr;
			int iVisSign = (unsigned)pVisSign[0x9D];
			if (iVisSign)
				visualSignature = (float)iVisSign / 64.0f;
		}
	}


	visualSignature *= sqrtf((float)skill) / 1.2f; //JAM 06Oct03
	// factor in prevailing vis conditions
	//JAM 20Nov03
	*/
	

/*	if (g_bEnableWeatherExtensions) {
	    int wx = realWeather->WorldToTile( obj->BaseData()->XPos() - realWeather->xOffset);
	    int wy = realWeather->WorldToTile( obj->BaseData()->YPos() - realWeather->yOffset);
	    if (obj->BaseData()->ZPos() > realWeather->TopsAt(wx, wy) )
		visualSignature *= realWeather->VisRangeAt(wx, wy);
	}
*/
	// M.N. Factor in the radius value of the draw pointer as representation of overall aircraft size
	/*if (g_bAddACSizeVisual)
	{
		if (object->IsSim() && object->IsAirplane())
		{
			theObject = (SimBaseClass*) vuDatabase->Find(object->Id());
			if (theObject && theObject->drawPointer)
			{
				float radius = theObject->drawPointer->Radius();
				visualSignature *= (radius / g_fVisualNormalizeFactor);	// normalize on F-16 drawpointer radius
			}
		}
	}

	// Visual acuity is proportional to light level
	return (lightBonus + TheTimeOfDay.GetLightLevel() * bonus) * visualSignature;*/
}

