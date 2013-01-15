#include "stdhdr.h"
#include "f4vu.h"
#include "object.h"
#include "simbase.h"
#include "otwdrive.h"
#include "fakerand.h"
#include "Missile.h"
#include "Graphics\Include\tod.h"
#include "irst.h"
#include "entity.h" // MN

extern int g_nMissileFix;

// Angle off sun at which sun effect goes to zero
static const float	COS_SUN_EFFECT_HALF_ANGLE	= (float)cos( 20.0f * DTR );//me123 changed from 10 since the sun is so small in f4
//extern bool g_bHardCoreReal; //me123	MI replaced with g_bRealisticAvionics
extern bool g_bRealisticAvionics;

int IrstClass::CanSeeObject (SimObjectType* obj)
{
	if (obj == NULL) {//me123
		tracking = FALSE;
		return FALSE;
	}
	ShiAssert( platform );

// 2002-04-17 MN "GPS type" weapons can see and detect always
	if (g_nMissileFix & 0x100)
	{
		Falcon4EntityClassType* classPtr = NULL;
		classPtr = (Falcon4EntityClassType*)platform->EntityType();
		WeaponClassDataType *wc = NULL;

		ShiAssert(classPtr);

		if (classPtr)
			wc = (WeaponClassDataType*)classPtr->dataPtr; // this is important

		ShiAssert(wc);
		if (wc && (wc->Flags & WEAP_BOMBGPS))
			return true;
	}

	if ( platform->IsMissile() && ((MissileClass*)platform)->parent && ((MissileClass*)platform)->parent->OnGround() ) 
		{
		// TODO: Fix this case to deal with localData->az being heading instead of relative az
		return TRUE;
		} 
	//else if (!g_bHardCoreReal)	MI
	else if(!g_bRealisticAvionics)
	{
				if ( obj == lockedTarget && obj->localData->ata < typeData->GimbalLimitHalfAngle)
					{
						return TRUE;
					}
				else
					{
						return FALSE;
					}
	}
	else if (platform->IsMissile())//me123 status bad. make the heatseekers limited to 28 degree radar slew and 6 degree cone in bore.
	{
		// Marco Edit - if missile is both slaved and caged (ie. radar points it to target)
		if (((MissileClass*)platform)->isSlave == TRUE && ((MissileClass*)platform)->isCaged == TRUE)
			{		
			 tracking = FALSE ;
			//me123 slave mode
				if ( obj == lockedTarget && obj->localData->ata < (typeData->GimbalLimitHalfAngle/1.5))
				{
					return TRUE;
				}
				// If slaved with no radar target
				else if (obj->localData->ata < typeData->GimbalLimitHalfAngle)
				{
					return TRUE;
				}
				else
				{
					return FALSE;
				}
			}
		// Marco Edit - here it is either uncaged or boresighted
		else//me123 bore/uncaged
			{
				//RV - I-Hawk - use tracking factor from missile FMs. Default value is 1.0 so
				//nothing is changed from before. But if the missile requires high Off boresight
				//angles tracking ability, use such factor (between 0.5-1.0) to allow wide angle
				//tracking for advanced modern IR missiles, like IRIS-T, Python-4/5 etc
				float trackFactor = ((MissileClass*)platform)->GetGimbalTrack();
				float noneTrackingFactor = 3.0f * trackFactor;

				if (tracking)
				{//me123  tracking
					if (obj->localData->ata < typeData->GimbalLimitHalfAngle / trackFactor)
					{
						return TRUE;
					}
					else
					{
					tracking = FALSE ;
						return FALSE;
					} 
				}

				else 
				{
					if (fabs(obj->localData->el - seekerElCenter) <  (typeData->FOVHalfAngle / noneTrackingFactor) && 
						fabs(obj->localData->az - seekerAzCenter) <  (typeData->FOVHalfAngle / noneTrackingFactor) ) 
					{
						tracking = TRUE ;
						return TRUE;
					}
					else
					{
						return FALSE;
					}
				}
			}
		}
	else return FALSE;
}

float IrstClass::GetSignature (SimObjectType* obj)
{
	float	signal;
	float	ataFactor;
	// Get the IR output of the aircraft (relative to F16 at mil power)
	// TODO:  Make sure this uses real IR data from the class table and STRONGLY
	//        reflects afterburner usage...me123 done
	signal = (obj->BaseData()->GetIRFactor());

	// Get our angle off his nose
   if (!obj->BaseData()->OnGround())

if (0)
   { 
	if (obj->localData->ataFrom < 45*DTR)//me123 status test. scale ir signature ahead of 3/9 line
		{
	   ataFactor = (float)sin((obj->localData->ataFrom)*RADIANS_TO_DEG);
		ataFactor *= ataFactor;
	   // Scale for aspect
	   signal *= 0.03f + 0.08F*ataFactor;//me123 status test. changed from min (1.0f, 0.6f - 0.55F*ataFactor)
		}
	else if (obj->localData->ataFrom < 90*DTR)//me123 status test. 
		{
	   ataFactor = (float)sin((obj->localData->ataFrom)*RADIANS_TO_DEG);
		ataFactor *= ataFactor;
	   // Scale for aspect
	   signal *= 0.075f + 0.005F*ataFactor;
		}

	else if (obj->localData->ataFrom < 135*DTR)//me123 status test. scale  ir signature between 3/9 and 135 aspect line
		{
	   ataFactor = (float)cos((obj->localData->ataFrom)*RADIANS_TO_DEG);
		ataFactor *= ataFactor;
	   // Scale for aspect
	   signal *= 0.08f + 0.008F*ataFactor;
		}
	else   //me123 status test. scale  ir signature behind of 135 aspect line
	   {
		ataFactor = (float)cos((obj->localData->ataFrom)*RADIANS_TO_DEG);
		ataFactor *= ataFactor;
	   // Scale for aspect
	   signal *= 0.084F + 0.04F*ataFactor;
		}
   }
   else //sylvains stuff
	{ /* 0-180 DEGREES IS COS -1 TO 1. SINCE IT IS SQUARED, IT BECOMES 0 TO 1. 0 BEING FRONT OR BACK!
		I CHANGED IT TO 0-90 SO FRONT IS 0 AND BACK IS 1 LIKE IT SHOULD BE */
	   ataFactor = (float)cos(obj->localData->ataFrom/* S.G. TO BRING IT FROM 0-180 TO 0-90 */ / 2.0F);
		ataFactor *= ataFactor;
	   // Scale for aspect
	   signal *= min( 1.0f, 1.2f - 1.10F*ataFactor );//me123 if they screwed the squared thing, then they most likely didn't use it here either ! so changed from 0.6 - 0.55
   }

	return signal;
}


float IrstClass::GetSunFactor(SimObjectType* obj)
{
	Tpoint	sunRay;
	float	dx, dy, dz;
	float	scale;
	float	strength;

	// Skip out if no sun
	if (!TheTimeOfDay.ThereIsASun())
	{
		return 0.0f;
	}

	// get the vector to the target and vector toward the sun
	TheTimeOfDay.GetLightDirection( &sunRay );
	dx = obj->BaseData()->XPos() - platform->XPos();
	dy = obj->BaseData()->YPos() - platform->YPos();
	dz = obj->BaseData()->ZPos() - platform->ZPos();

	// Normalize the vector to target
	scale = 1.0f / obj->localData->range;
	dx *= scale;
	dy *= scale;
	dz *= scale;

	// Now do the dot product to get the cos of the angle between the rays
	strength = dx*sunRay.x + dy*sunRay.y + dz*sunRay.z;

	// Quit with 0.0 returned if we don't cross the threshhold
	if ( strength < COS_SUN_EFFECT_HALF_ANGLE )
		return 0.0f;

	// Normalize the result to 0 to 1 and return it
	return (strength - COS_SUN_EFFECT_HALF_ANGLE) / (1.0f - COS_SUN_EFFECT_HALF_ANGLE);
}


int IrstClass::CanDetectObject (SimObjectType* obj)
{
	float	signature;
	float	sunFactor;

// 2002-04-17 MN "GPS type" weapons can see and detect always
	if (g_nMissileFix & 0x100)
	{
		Falcon4EntityClassType* classPtr = NULL;
		classPtr = (Falcon4EntityClassType*)platform->EntityType();
		WeaponClassDataType *wc = NULL;

		ShiAssert(classPtr);

		if (classPtr)
			wc = (WeaponClassDataType*)classPtr->dataPtr; // this is important

		ShiAssert(wc);
		if (wc && (wc->Flags & WEAP_BOMBGPS))
			return true;
	}

	// Get Observer Signature
	signature = GetSignature (obj);
	
	// TODO:  Eliminate this once each vehicle has its own IR signal strength in the class table
	// Cut the signature for being on the ground
	if (obj->BaseData()->IsSim() && ((SimBaseClass*)obj->BaseData())->OnGround())
		signature *= typeData->GroundFactor;
	
	// Scale for range squared
	signature *= typeData->NominalRange * typeData->NominalRange;
	signature /= obj->localData->range  * obj->localData->range;

   // Bonus for being locked target

	//MI
   //if (g_bHardCoreReal && tracking == TRUE)//me123 we have uncaged on a target and it's tracking
	if(g_bRealisticAvionics && tracking == TRUE)
   {
      signature *= 1.2F;
   }

   //if (!g_bHardCoreReal && obj == lockedTarget)	MI
	if(!g_bRealisticAvionics && obj == lockedTarget)
   {
      signature *= 1.5F;
   }

	// Check line of sight (terrain and clouds)
	// TODO:  Store the LOS check results in local data and only refresh it periodicly
	//        Alos, share it among sensors (Radar, IR, Visual)
	//signature *= OTWDriver.CheckCompositLOS( platform, obj->BaseData() );
	
    signature *= platform->CheckCompositeLOS( obj );
	

	// Filter the computed signature for other's use (Aim9 growl, etc.)

	// Get the sun factor (if any)
	sunFactor = GetSunFactor( obj );


	// Keep the reported signal strength within reasonable bounds
	// NOTE:  We adding in sun factor so the seeker reports a strong signal when
	// looking at the sun.
	obj->localData->irSignature = max( min( signature+sunFactor, 2.0f ), 0.0f );
	// Decide if we think we can "see" it this time
	if (sunFactor > 0.5f)
   {
		// We took the sun, so tell the missile to go there (if thats what we are)
		if (platform->IsMissile())
      {
			Tpoint	sunRay;
			TheTimeOfDay.GetLightDirection( &sunRay );
			sunRay.x = sunRay.x * 1e6f + platform->XPos();
			sunRay.y = sunRay.y * 1e6f + platform->YPos();
			sunRay.z = sunRay.z * 1e6f + platform->ZPos();
			((MissileClass*)platform)->SetTargetPosition( sunRay.x, sunRay.y, sunRay.z );
		}

		return FALSE; 

	}
   else if ((signature > 0.75F)  && (signature > PRANDFloatPos()) )
   {
		return TRUE;
	}
   else
   {
		return FALSE;
	}
}
