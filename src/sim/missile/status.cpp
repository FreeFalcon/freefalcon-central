#include "stdhdr.h"
#include "missile.h"
#include "otwdrive.h"
#include "playerop.h"
#include "MsgInc\DamageMsg.h"
#include "camp2sim.h"
#include "object.h"
// Marco addition for missile arming delay
#include "entity.h"
//MI
#include "Classtbl.h"
#include "Hardpnt.h"

extern int g_nMissileFix;
extern float g_fLethalRadiusModifier;

// RV - Biker - New missile SetStatus...
void MissileClass::SetStatus(void)
{
	SimBaseClass *hitObj;
	float TToLive = 0.0f;
    bool bombwarhead = false;
	Falcon4EntityClassType* classPtr;
	classPtr = (Falcon4EntityClassType*)EntityType();
	WeaponClassDataType *wc = NULL;

	ShiAssert(classPtr);

	// this is important
	if (classPtr)
		wc = (WeaponClassDataType*)classPtr->dataPtr; 

	ShiAssert(wc);

	if (wc && (wc->Flags & WEAP_BOMBWARHEAD) && (g_nMissileFix & 0x01)) {
		bombwarhead = true;
	}

	ShiAssert(engineData);
	ShiAssert(inputData);

	// Check for min speed and max time for all missiles first
	if ((inputData && engineData) && (mach < inputData->mslVmin) &&	
		(runTime > engineData->times[engineData->numBreaks-1]))
	{
		done = FalconMissileEndMessage::MinSpeed;
		return;
	}
   
	if (inputData && runTime > inputData->maxTof) {
		done = FalconMissileEndMessage::ExceedTime;
		return;
	}

	// Also return if we are just launching
	if (launchState == Launching) {
		done = FalconMissileEndMessage::NotDone; 
		return;
	}

	// Do ground impact check for all missiles here
	if (z >= groundZ && !(this->GetSWD()->weaponType == wtSAM && runTime < 1.0f)) {
		done = FalconMissileEndMessage::GroundImpact;
		return;
	}

	// RV - Biker - Check what kind of missile
	switch (this->GetSWD()->weaponType)
	{
		case wtGuns:
			break;

		case wtAim9:
		case wtAim120:
			if (ricept*ricept <= lethalRadiusSqrd) {
				if (wc) {
					TToLive = (float)((((unsigned char *)wc)[45]) & 7);
					if (TToLive > 4.0)
						TToLive = TToLive * (float)2.0;
				}
	      
				// warhead didn't have time to fuse
				if (inputData && runTime < inputData->guidanceDelay) {
					done = FalconMissileEndMessage::MinTime;
				}

				// Marco Edit - Check for Time to Warhead Armed
				else if (runTime < TToLive) {
					if (g_nMissileFix & 0x04)
						done = FalconMissileEndMessage::ArmingDelay;
					else
						done = FalconMissileEndMessage::MinTime;
				}
	      
				// kill
				else {
					done = FalconMissileEndMessage::MissileKill;
				}
			}
			break;

		case wtAgm88:
			hitObj = FeatureCollision(groundZ);
				
			// Check for features
			if (hitObj) {
				done = FalconMissileEndMessage::FeatureImpact;
			}
			// Check for vehicles
			else {
				if (targetPtr && ricept*ricept < min(lethalRadiusSqrd, 10000.0f)) {
					done = FalconMissileEndMessage::MissileKill;
				}
			}
			break;

		case wtAgm65:
			// AGMs with large (bomb) warhead
			if (bombwarhead) {
				hitObj = FeatureCollision(groundZ);
				
				// Check for features
				if (hitObj) {
					done = FalconMissileEndMessage::BombImpact;
				}
				// Check for vehicles
				else {
					if (targetPtr && ricept*ricept < min(lethalRadiusSqrd, 10000.0f)) {
						done = FalconMissileEndMessage::BombImpact;
					}
				}
			}
			// Standard AGMs
			else {
				hitObj = FeatureCollision(groundZ);
				
				// Check for features
				if (hitObj) {
					done = FalconMissileEndMessage::FeatureImpact;
				}
				// Check for vehicles
				else {
					if (targetPtr && ricept*ricept < min(lethalRadiusSqrd, 10000.0f)) {
						done = FalconMissileEndMessage::MissileKill;
					}
				}
			}
			break;

		case wtMk82:
		case wtMk84:
		case wtGBU:
			break;

		case wtSAM:
			if (ricept*ricept <= lethalRadiusSqrd) {
				if (wc) {
					TToLive = (float)((((unsigned char *)wc)[45]) & 7);
					if (TToLive > 4.0)
						TToLive = TToLive * (float)2.0;
				}
	      
				// warhead didn't have time to fuse
				if (inputData && runTime < inputData->guidanceDelay) {
					done = FalconMissileEndMessage::MinTime;
				}

				// Marco Edit - Check for Time to Warhead Armed
				else if (runTime < TToLive) {
					if (g_nMissileFix & 0x04)
						done = FalconMissileEndMessage::ArmingDelay;
					else
						done = FalconMissileEndMessage::MinTime;
				}
	      
				// kill
				else {
					done = FalconMissileEndMessage::MissileKill;
				}
			}
			break;

		default:
			break;
	}
	return;
}

////static int miscount = 0;
//
//// 2002-03-28 MN Modified SetStatus function, fixes floating missiles and adds support for "bombwarhead" missiles like JSOW/JDAM
//void MissileClass::SetStatus(void)
//{
//SimBaseClass *hitObj;
//float TToLive = 0.0f;
//bool bombwarhead = false;
//Falcon4EntityClassType* classPtr;
//classPtr = (Falcon4EntityClassType*)EntityType();
//WeaponClassDataType *wc = NULL;
//
//	ShiAssert(classPtr);
//
//	if (classPtr)
//		wc = (WeaponClassDataType*)classPtr->dataPtr; // this is important
//
//	ShiAssert(wc);
//
//	if (wc && (wc->Flags & WEAP_BOMBWARHEAD) && (g_nMissileFix & 0x01))
//		bombwarhead = true;
//
//	ShiAssert(engineData);
//	ShiAssert(inputData);
//
//   /*--------------------------------------*/
//   /* don't check status until off the rail*/  
//   /*--------------------------------------*/
//   if (launchState == Launching)
//   {
//      done = FalconMissileEndMessage::NotDone; 
//   }
//   /*----------------------*/
//   /* within lethal radius */
//   /*----------------------*/
//// 2002-03-28 MN "Bomb" warheads on missiles with a big blast radius shall not 
//// detonate when the target is within blast range (like JSOW)....
//   //Cobra test  since ricept might not get updated enough, lets add range as well
//   // COBRA - RED - SARH BUG - removed, causing unklocked missiles to explode as 'range' is 0 for them
//   else if ( (ricept*ricept <= lethalRadiusSqrd/* || range * range < lethalRadiusSqrd*/) && !bombwarhead) 
//   {
//	  // Marco Edit - check for our missile arming delay
//	  if (wc)
//	  {
//		  TToLive = (float)((((unsigned char *)wc)[45]) & 7) ;  //  + 1) * 2)) ;
//		  if (TToLive > 4.0)
//			TToLive = TToLive * (float)2.0;
//	  }
//      /*----------------------------------*/
//      /* warhead didn't have time to fuse */
//      /*----------------------------------*/
//      if (inputData && runTime < inputData->guidanceDelay) // possible CTD fix
//      {
//         done = FalconMissileEndMessage::MinTime;
//      }
//	  // Marco Edit - Check for Time to Warhead Armed
//	  else if (runTime < TToLive)
//	  {
//		  if (g_nMissileFix & 0x04)
//			done = FalconMissileEndMessage::ArmingDelay;
//		  else
//			done = FalconMissileEndMessage::MinTime;
//	  }
//      /*------*/
//      /* kill */
//      /*------*/
//      else
//      {
//         done = FalconMissileEndMessage::MissileKill;
//      }
//   }
//// 2002-04-06 MN if the missile sensor has lost track, we have no targetptr, and our range to interpolated
//// target position is inside the lethal radius OR missile is higher than its maxalt,
//// bring missile to an end. When we have missiles going high ballistic, intercept them at max altitude
//// in case of lethalRadius, they might apply a bit of proximity damage to the target...
//	else if (runTime > 1.50f && (flags & SensorLostLock) && !targetPtr && ((g_nMissileFix & 0x20) && 
//		range * range < lethalRadiusSqrd || (wc && wc->MaxAlt && (fabs(z) > fabsf(wc->MaxAlt*1000.0f))))) //JAM 27Sep03 - Should be fabsf
//	{
//		done = FalconMissileEndMessage::NotDone; //ExceedFOV;//Cobra we can't use Missed because it is out of range
//
//// 2002-04-14 MN Try a modification here. This code could result in missing mavericks and Aim-9 and others.
//// So when our last target lock was really close to lethalRadius - let's still do a MissileKill...
//		if (range*range < lethalRadiusSqrd && ricept*ricept < lethalRadiusSqrd*g_fLethalRadiusModifier)
//			done = FalconMissileEndMessage::MissileKill;
//		else 
//			done = FalconMissileEndMessage::NotDone;
//			//done = FalconMissileEndMessage::ExceedFOV;//Cobra we can't use Missed because it is out of range
//	}
//   /*-------------------*/
//   /* Missed the target */
//   /*-------------------*/
//// 2002-03-28 MN added range^2 check, as ricept is not updated anymore if we lost our target.
//// Fixes floating missiles on the ground
//   else if ((flags & ClosestApprch) && (ricept*ricept > lethalRadiusSqrd || ((g_nMissileFix & 0x02) && range*range > lethalRadiusSqrd)))
//   {
//      //done = FalconMissileEndMessage::Missed;
//		done = FalconMissileEndMessage::ExceedFOV;//Cobra we can't use Missed because it is out of range
//   }
//   /*---------------*/
//   /* ground impact */
//   /*---------------*/
//   else if ( z > groundZ && !bombwarhead) // bombwarhead missiles are handled below
//   {
//   	  // edg: this is somewhat of a hack, but I haven't had much luck fixing it
//      // otherwise... if the parent is a ground unit and the missile has just
//   	  // been launched( runTime ), don't do a groundimpact check yet
//	  if ( (parent && parent->OnGround() && runTime < 1.50f) )
//	  {
//	  }
//	  else
//      	done = FalconMissileEndMessage::GroundImpact;
//   }
//   /*----------------*/
//   /* feature impact */
//   /*----------------*/
//// 2002-03-28 MN handle bombwarhead missiles here
//#ifndef MISSILE_TEST_PROG
//   else if ( z - groundZ > -800.0f)	// only do feature impact if we are not a bomb
//   {
//		if (runTime > 1.50f) // JB 000819 //+ SAMS sitting on runways will hit the runway
//		{ // JB 000819 //+
//			hitObj = FeatureCollision( groundZ );
//			if (hitObj)
//			{
//				if (bombwarhead)
//				{
//					// bombwarhead missile - we've not hit our target on our way:
//					if (targetPtr && hitObj != targetPtr->BaseData())	// CTD fix
//					{
//						done = FalconMissileEndMessage::FeatureImpact;
//						// might as well send the msg now....
//						// apply proximity damage may send it more damage.  However this is
//						// only from the base!
//						SendDamageMessage(hitObj,0,FalconDamageType::MissileDamage);
//					}
//					else // bombwarhead missile has hit assigned target or hit something else
//					{
//						if (targetPtr && ricept*ricept < lethalRadiusSqrd)
//						  done = FalconMissileEndMessage::BombImpact;
//						else
//					      done = FalconMissileEndMessage::GroundImpact;
//					}
//				}
//						
//				else // we're not a bombwarhead missile, do normal feature impact
//				{
//					done = FalconMissileEndMessage::FeatureImpact;
//					// might as well send the msg now....
//					// apply proximity damage may send it more damage.  However this is
//					// only from the base!
//					SendDamageMessage(hitObj,0,FalconDamageType::MissileDamage);
//				}
//			}
//			if (z > groundZ)	// now we've hit the dust and not a feature (needed in case of g_bMissileFix == true...)
//			{
//				done = FalconMissileEndMessage::GroundImpact;
//			}
//
//
//		} // JB 000819 //+
//   }
//#endif
//// 2002-03-28 MN changed "else if" to plain "if" for the next two conditions. 
//// Min velocity and max life time can happen in *each* situation, so always check for them
//// If we should still have floating missiles on the ground, this will bring them to an end some time for sure...
//   /*--------------------------------*/
//   /* min velocity only after burnout*/
//   /*--------------------------------*/
//   if (inputData && engineData && mach < inputData->mslVmin &&	// 2002-04-05 MN CTD fix
//         runTime > engineData->times[engineData->numBreaks-1])
//   {
////      done = FalconMissileEndMessage::MinSpeed;
//			done = FalconMissileEndMessage::ExceedFOV; //Cobra MinSpeed doesn't seem to kill it
//   }
//   /*----------*/
//   /* max time */
//   /*----------*/
//   if (inputData && runTime > inputData->maxTof) // 2002-04-05 MN possible CTD fix
//   {
////      done = FalconMissileEndMessage::ExceedTime;
//			done = FalconMissileEndMessage::ExceedFOV; //Cobra ExceedTime doesn't seem to kill it
//   }
//}