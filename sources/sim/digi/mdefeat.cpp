#include "stdhdr.h"
#include "simveh.h"
#include "simbase.h"
#include "digi.h"
#include "sensors.h"
#include "object.h"
#include "simbase.h"
#include "airframe.h"
#include "aircrft.h"
#include "Graphics\Include\tmap.h"
/* S.G. NEED TO KNOW WHICH WEAPON WE FIRED */#include "Missile.h"
/* S.G. NEED TO KNOW WHICH WEAPON WE FIRED */#include "vehrwr.h"
/* S.G. 2001-06-29 */#include "CampBase.h"
/* S.G. 2001-06-29 */#include "WingOrder.h"
#include "sms.h" // S.G. 2002-01-02
#include "flight.h" // MN 2002-03-27
#include "visual.h"//Cobra for the eyeball ;)

/*--------------------------*/
/* last ditch maneuver time */
/*--------------------------*/

#define LD_TIME  1.0


#define MISSILE_LETHAL_CONE    45.0F

extern float g_fAIDropStoreLauncherRange;

void DigitalBrain::MissileDefeatCheck(void)
{
	short edata[6];
	int	  response;
 
	//Cobra Moved here before jump out of function code.
	if  (targetPtr)//me123 defensive flare 
	   {int looks_scarry = 0;
		//me123 status test. let's drop a flare if we are threatened, even with no missile in the air. better safe then sory
		   if (targetData->ataFrom <MISSILE_LETHAL_CONE *DTR && targetData->range < 2.0f * 6000.0f)
			{//me123 status test. we are inside 2nm, somebody is pointing at us and they look like they can fire a missile.
				if ( targetData->ata < 30.0f* DTR ) 
				{looks_scarry = FALSE;}	

				else if ( targetData->ata > 30.0f * DTR && targetData->ata < 60.0f *DTR && targetData->range > 5000.0f) 
				{looks_scarry = TRUE;}

				else if ( targetData->ata > 60.0f * DTR && targetData->ata < 120.0f *DTR && targetData->range > 4000.0f) 
				{looks_scarry = TRUE;}

				else if ( targetData->ata > 120.0f * DTR && targetData->range > 3000.0f) 
				{looks_scarry = TRUE;}

			   if (looks_scarry == TRUE)
				{
				   if ( SimLibElapsedTime > self->FlareExpireTime() + 4 * SEC_TO_MSEC )
					 {//let's pump a flare if none is in the air.
					 ((AircraftClass*)self)->dropFlareCmd = TRUE;
					 }
				}
			}
	   }

// ADDED BY S.G.SINCE THIS ROUTINE IS CALLED OFTEN, WE CAN DEREFERENCE THE LAUNCHED MISSILE POINTER ONCE IT IS DEAD
// NO POINT WAITING FOR SOMEONE TO SHOOT AT US FOR THIS TO HAPPEN
	if (missileFiredEntity) {
		// Has the missile exploded yet?
		if (((SimWeaponClass *)missileFiredEntity)->IsDead()) {
			VuDeReferenceEntity((SimWeaponClass *)missileFiredEntity);
			missileFiredEntity = NULL;
		}
	}

// 2000-09-03 ADDED BY S.G. WE NEED TO STOP EVADING IF THE MISSILE IS NO LONGER TARGETING.
// ALSO, SINCE THE CHECK FOR self->incomingMissile[0] EXISTING IS BEING DONE IN BOTH IF AND ELSE CLAUSE, IT WILL BE MOVED HERE
	if ( !self->incomingMissile[0] )
		return;

	if ( self->incomingMissile[0]->IsDead() ) {
		self->SetIncomingMissile( NULL );
		return;
	}

	// RV - Biker - Allow to shoot them back
	if (((MissileClass *)self->incomingMissile[0])->parent && !((MissileClass *)self->incomingMissile[0])->parent->OnGround())
		missileShotTimer = 0;

	// 2000-09-05 this will make the range not target base but from us to the missile
	float dx, dy, dz, missileRange;

	dx = self->incomingMissile[0]->XPos() - self->XPos();
	dy = self->incomingMissile[0]->YPos() - self->YPos();
	dz = self->incomingMissile[0]->ZPos() - self->ZPos();
	missileRange = (float)sqrt( dx*dx + dy*dy + dz*dz );

	// If the missile's range is more than the previous missile range (that we kept), the missile has passed by us.
	if (missileRange > self->incomingMissileRange && self->incomingMissileRange) {
		if (self->incomingMissileEvadeTimer + (6 - SkillLevel()) * SEC_TO_MSEC < SimLibElapsedTime) {
			//We have spoofed the missile, now forget about it!
			// Cobra - Destroy the missile
			if (missileFiredEntity)
			{
				((SimWeaponClass *)missileFiredEntity)->SetFlag(MissileClass::SensorLostLock);
				((SimWeaponClass *)missileFiredEntity)->SetFlag(MissileClass::ClosestApprch);
				((SimWeaponClass *)missileFiredEntity)->SetExploding(TRUE);
				((SimWeaponClass *)missileFiredEntity)->SetDead(TRUE);
			}
			if (self->incomingMissile[0])
			{
				self->incomingMissile[0]->SetFlag(MissileClass::SensorLostLock);
				self->incomingMissile[0]->SetFlag(MissileClass::ClosestApprch);
				self->incomingMissile[0]->SetExploding(TRUE);
				self->incomingMissile[0]->SetDead(TRUE);
			}
			else if (self->incomingMissile[1])
			{
				self->incomingMissile[1]->SetFlag(MissileClass::SensorLostLock);
				self->incomingMissile[1]->SetFlag(MissileClass::ClosestApprch);
				self->incomingMissile[1]->SetExploding(TRUE);
				self->incomingMissile[1]->SetDead(TRUE);
			}
			self->SetIncomingMissile( NULL );
			self->incomingMissileRange = 500 * NM_TO_FT; //initialize
			return;
		}
	}
	else
		self->incomingMissileEvadeTimer = SimLibElapsedTime;

	// Keep this as the old missileRange
	self->incomingMissileRange = missileRange;
// END OF ADDED SECTION

   /*-------*/
   /* entry */
   /*-------*/
   if (curMode != MissileDefeatMode)
   {

// 2000-09-01 ADDED BY S.G. SO AI ARE NOT AWARE OF ARH MISSILE STILL COMMAND GUIDED (NOT ACTIVE) OR SARH OR ACTIVE ARH UNLESS IT'S GETTING CLOSE OR HAS A RWR, SKILL RELATED
			VehRwrClass::DetectListElement *rwrElement;
			SensorClass *rwrSensor;
/*
			// Must have a RWR or visual tally on the missile
			if (!((
					((MissileClass *)self->incomingMissile[0])->GetSeekerType() != SensorClass::IRST &&																							// Don't test for RWR if it's a IR missile...
					(rwrSensor = FindSensor(self, SensorClass::RWR)) &&																															// The plane has an RWR (without it, why bother)
					(
					( (MissileClass *)self->incomingMissile[0])->GetSeekerType() == SensorClass::RadarHoming ||																					// The real missile sensor is a SARH
					(((MissileClass *)self->incomingMissile[0])->sensorArray && ((MissileClass *)self->incomingMissile[0])->sensorArray[0]->Type() == SensorClass::Radar) ||					// The current sensor is a ARH (if it's a local entity)
					(!((MissileClass *)self->incomingMissile[0])->sensorArray && (rwrElement = ((VehRwrClass *)rwrSensor)->IsTracked(self->incomingMissile[0])) && rwrElement->missileLaunch)	// There is no sensor (non local enity) but the missile is on RWR with a launch
				   )) ||																																										// Don't forget we take the REVERSE of the test if (!(...
				    missileRange < ((float)SkillLevel() + 1.5f) * NM_TO_FT))																													// Or the missile has come to visual range
							return;*/

//Cobra Removed above so I can sort through this mess.  I want to know exactly how the AI
//Is "seeing" the missile

			//Cobra Test this Radar Detect stuff
			if (((MissileClass *)self->incomingMissile[0])->GetSeekerType() != SensorClass::IRST)
				{
				if (rwrSensor = FindSensor(self, SensorClass::RWR))
					{
					if ( ((MissileClass *)self->incomingMissile[0])->GetSeekerType() == SensorClass::RadarHoming)
						{
						int testme =1;
						}
					if (((MissileClass *)self->incomingMissile[0])->sensorArray && ((MissileClass *)self->incomingMissile[0])->sensorArray[0]->Type() == SensorClass::Radar)
						{
						int testme = 1;
						}
					if ( (!((MissileClass *)self->incomingMissile[0])->sensorArray) && (rwrElement = ((VehRwrClass *)rwrSensor)->IsTracked(self->incomingMissile[0])) && rwrElement->missileLaunch)
						{
						int testme = 1;
						}
					}
				}


			//end

			if ( ((MissileClass *)self->incomingMissile[0])->GetSeekerType() != SensorClass::IRST
				&& (rwrSensor = FindSensor(self, SensorClass::RWR))
				&&
				(( (MissileClass *)self->incomingMissile[0])->GetSeekerType() == SensorClass::RadarHoming
				|| (((MissileClass *)self->incomingMissile[0])->sensorArray && ((MissileClass *)self->incomingMissile[0])->sensorArray[0]->Type() == SensorClass::Radar) 
				|| (!((MissileClass *)self->incomingMissile[0])->sensorArray && (rwrElement = ((VehRwrClass *)rwrSensor)->IsTracked(self->incomingMissile[0])) && rwrElement->missileLaunch)))
				{
				int donothing = 1;
				}
			else if (missileRange < (10.0f * NM_TO_FT) && SimLibElapsedTime > visDetectTimer)
				{
				int donothing1 = 1;
				VisualClass *eyeball = (VisualClass*)FindSensor((SimMoverClass *)self, SensorClass::Visual );
				float az, el, ata, ataFrom, droll;	
				int canSee = 0;
				if (eyeball)
					{
					CalcRelValues ((SimBaseClass *)self, (MissileClass*)self->incomingMissile[0], &az, &el, &ata, &ataFrom, &droll);
					canSee = eyeball->CanSeeObject(az, el);
		
					if (!canSee)
						{
						return;
						}
					else
						{
						if (missileRange > (8.0f * NM_TO_FT) && SimLibElapsedTime > visDetectTimer
							&& rand()%100 < 3)
							{
							int testme = 0;
							}
						else if (missileRange > (6.0f * NM_TO_FT)&& missileRange < (8.0f * NM_TO_FT)
							&& SimLibElapsedTime > visDetectTimer && rand()%100 < 10)
							{
							int testme = 0;
							}
						else if (missileRange > (4.0f * NM_TO_FT) && missileRange < (6.0f * NM_TO_FT)
							&& SimLibElapsedTime > visDetectTimer && rand()%100 < 25)
							{
							int testme = 0;
							}
						else if (missileRange > (2.0f * NM_TO_FT) && missileRange < (4.0f * NM_TO_FT)
							&& SimLibElapsedTime > visDetectTimer && rand()%100 < 40)
							{
							int testme = 0;
							}
						else if (missileRange > (0.5f * NM_TO_FT) && missileRange < (2.0f * NM_TO_FT)
							&& SimLibElapsedTime > visDetectTimer && rand()%100 < 65)
							{
							int testme = 0;
							}
						else
							{
							visDetectTimer = SimLibElapsedTime + 5000;
							return;
							}
						}
					}
				}
			else
				{
				return;
				}

// If we get here, either we acquired the missile visually, or we have an RWR and the missile is either an SARH or an actively guiding ARH
// ADDED BY S.G. SO DIGI PILOT SHOOTING SARH WONT GET SCARED RIGHT AWAY
			if (missileFiredEntity) {
				// No need to check if its dead, we did that above already...
				// If we are providing radar guidance to hit (SARH), see if we should wait before defeating the incoming missile
				if (((SimWeaponClass *)missileFiredEntity)->sensorArray && ((SimWeaponClass *)missileFiredEntity)->sensorArray[0]->Type() == SensorClass::RadarHoming) {
					// Check if we are in our 'count down'. First bit will be 1 if so
					if (missileFiredTime & 0x1) {
						// Is our count down over? If not, that's it
						if (missileFiredTime > SimLibElapsedTime)
							return;
						// So it is over, start evading!
						else {
							VuDeReferenceEntity((SimWeaponClass *)missileFiredEntity);
							missileFiredEntity = NULL;
						}
					}
					// Ok we're just entering here for the first time
					else {
						//  So check how long ago the our missile was fired
						if (missileFiredTime  + SEC_TO_MSEC * (SkillLevel() + 1) <= SimLibElapsedTime) {
							// It was launched not long ago (skill dependant), Evade!
							VuDeReferenceEntity((SimWeaponClass *)missileFiredEntity);
							missileFiredEntity = NULL;
						}
						// It's in the air long enough, give us a count down timer, skill based. Also set the first bit to say we're in count down
						else {
							missileFiredTime =  (SimLibElapsedTime + rand() % (((SimLibElapsedTime - missileFiredTime) - SEC_TO_MSEC * SkillLevel()) + 1) + SEC_TO_MSEC * (SkillLevel() + 1)) | 0x1; // 2002-01-28 MODIFIED BY S.G. Added '+ 1' in the '%' section to prevent a rare divide by 0 CTD.
							// Don't go in MissileDefeatMode.
							return;
						}
					}
				}
				// It's not even (or anymore) a RadarHoming missile, so why should we bother?
				else {
					VuDeReferenceEntity((SimWeaponClass *)missileFiredEntity);
					missileFiredEntity = NULL;
				}
			}

// END OF ADDED SECTION
            AddMode(MissileDefeatMode);
            missileDefeatTtgo = -1.0F;

			// RV - Biker - This does not work (try something basic for now)			
			//if (((MissileClass *)self->incomingMissile[0])->parent && (!self->Sms->DidEmergencyJettison()))
			//{
			//	float launcherRange = 9999999.9F;
			//	dx = ((MissileClass *)self->incomingMissile[0])->parent->XPos() - self->XPos();
			//	dy = ((MissileClass *)self->incomingMissile[0])->parent->YPos() - self->YPos();
			//	dz = ((MissileClass *)self->incomingMissile[0])->parent->ZPos() - self->ZPos();
			//	launcherRange = SqrtF( dx*dx + dy*dy + dz*dz );
			//
			//	// 2002-01-01 ADDED BY S.G. From GunJinks.cpp, drop stores under some conditions...
			//	//Cobra we modified this - me too
			//	if (self->CombatClass() != MnvrClassBomber
			//	&& (((launcherRange < g_fAIDropStoreLauncherRange * NM_TO_FT) && (launcherRange > 0.0f))
			//	&& (((MissileClass *)self->incomingMissile[0])->parent
			//	&& (!((MissileClass *)self->incomingMissile[0])->parent->OnGround())
			//	&& (((MissileClass *)self->incomingMissile[0])->GetSeekerType() != SensorClass::IRST)))
			//	)
			//	{
			//		self->Sms->AGJettison();
			//		SelectGroundWeapon();
			//	}
				// END OF ADDED SECTION
			//} 
// Have wingman say something useful when he is engaged
		   edata[0]	= isWing;
		   response = rcENGDEFENSIVEC;
   		 AiMakeRadioResponse( self, response, edata );

// 2000-09-03 COMMENTED OUT BY S.G. SINCE THEIR BODY WERE COMMENTED OUT AS WELL
//       }
//	   }
   }
   /*------*/
   /* exit */
   /*------*/
   if (curMode == MissileDefeatMode)
   {
			AddMode(MissileDefeatMode);
   }
}

void DigitalBrain::MissileDefeat()
{
   /*-------------------*/
   /* Shouldn't be here */
   /*-------------------*/
   if ( self->incomingMissile[0] )
   {
	    if ( self->incomingMissile[0]->IsDead() )
		{
			self->SetIncomingMissile( NULL );
			return;
		}
   }
   else
   {
		return;
   }

	// RV - Biker - Allow to shoot them back
	if (((MissileClass *)self->incomingMissile[0])->parent && !((MissileClass *)self->incomingMissile[0])->parent->OnGround())
		missileShotTimer = 0;

	// RV - Biker - Maybe check some extra conditions later
	if (!self->Sms->DidEmergencyJettison() && self->incomingMissileRange < 10.0f * NM_TO_FT) 
	{
		if (self->CombatClass() != MnvrClassBomber) 
		{
			if (rand()%100 < SkillLevel()*25) 
			{
				self->Sms->EmergencyJettison();
				SelectGroundWeapon();
			}
			else if (rand() & 1) 
			{
				self->Sms->AGJettison();
				SelectGroundWeapon();
			}
		}
	}

// 2001-06-29 ADDED BY S.G. I WANT LEAD TO ASK WINGS TO ATTACK IF HE IS ENGAGED OTHERWISE HE WON'T...
   if (/*!isWing &&*/ ((MissileClass *)self->incomingMissile[0])->parent && ((MissileClass *)self->incomingMissile[0])->parent->OnGround()) {
	   // Have we given the attack yet? Oh yeah, do we have some AG weapons and do we have someone to direct?
	   if (sentWingAGAttack != AG_ORDER_ATTACK && IsSetATC(HasAGWeapon) && self->GetCampaignObject()->NumberOfComponents() > 1) {
		   VU_ID targetId = FalconNullId;
		   // Only SEADS on target of opportunity changes to the target shooting at me...
			 // Cobra - changed to NOT (!) IsNotMainTargetSEAD() (double negatives :^( )
		   if (!IsNotMainTargetSEAD()) {
			   if (((MissileClass *)self->incomingMissile[0])->parent->IsSim())
				   targetId = ((SimBaseClass *)((MissileClass *)self->incomingMissile[0])->parent.get())->GetCampaignObject()->Id();
			   else
				   targetId = ((MissileClass *)self->incomingMissile[0])->parent->Id();
		   }
		   // Otherwise, tell them to engage our current target if they can (and we have one) while we ditch the missile
		   else if (groundTargetPtr) {
			   if (groundTargetPtr->BaseData()->IsSim())
				   targetId = ((SimBaseClass *)groundTargetPtr->BaseData())->GetCampaignObject()->Id();
			   else
				   targetId = groundTargetPtr->BaseData()->Id();
		   }

		   if (targetId != FalconNullId) {
			   SetGroundTargetPtr(NULL); // First clean our ground target so we can release the radar if we were targeting it
			   gndTargetHistory[0] = NULL; // Then remove our hold on the target so it can be retargeted

			   AiSendCommand (self, FalconWingmanMsg::WMAssignTarget, AiFlight, targetId);
			   AiSendCommand (self, FalconWingmanMsg::WMShooterMode, AiFlight, targetId);
			   sentWingAGAttack = AG_ORDER_ATTACK;
			   // 2002-01-20 ADDED BY S.G. Added the new nextAttackCommandToSend variable check to force the lead to reissue an attack in case wings went back into formation (can we say HACK?)
			   nextAttackCommandToSend = SimLibElapsedTime + 60 * SEC_TO_MSEC;
		   }
	   }
   }
// END OF ADDED SECTION

   /*------------------------------*/
   /* new missile, initialize data */
   /*------------------------------*/
   if (missileDefeatTtgo < 0.0F)
   {
      missileDefeatTtgo    = 1000.0F;
      missileFindDragPt    = TRUE;
      missileFinishedBeam  = FALSE;
      missileShouldDrag    = FALSE;
	}

	// calc approx threat time
	float dx, dy, dz, range, threatTime;

	dx = self->incomingMissile[0]->XPos() - self->XPos();
	dy = self->incomingMissile[0]->YPos() - self->YPos();
	dz = self->incomingMissile[0]->ZPos() - self->ZPos();
	

   range = (float)sqrt( dx*dx + dy*dy + dz*dz );
   threatTime = range / AVE_AIM9L_VEL;

  	/*--------------------------------------------*/
  	/* Use threat time for time to go if known.   */
  	/* Otherwise run down the clock in real time. */
  	/*--------------------------------------------*/
  	if (threatTime < MAX_THREAT_TIME)
  	{
     	missileDefeatTtgo = threatTime;
  	}
	else
   		missileDefeatTtgo -= SimLibMinorFrameTime;

  	if (missileDefeatTtgo < 0.0)
      missileDefeatTtgo = 0.0F;

  	/*--------------------*/
  	/* maneuver selection */
  	/*--------------------*/
	if (missileDefeatTtgo > LD_TIME)
  	{
		((AircraftClass*)self)->DropProgramed();

		/*if (((MissileClass *)self->incomingMissile[0])->targetPtr)
			if (((MissileClass *)self->incomingMissile[0])->targetPtr->localData->range > ((float)(6.0f - SkillLevel()) * NM_TO_FT) &&
				 ((MissileClass *)self->incomingMissile[0])->targetPtr->localData->range < ((float)(19.0f - SkillLevel()) * NM_TO_FT))
					missileShouldDrag = TRUE;

		if (!missileShouldDrag)
			MissileBeamManeuver();
		else
			MissileDragManeuver();*/

		//Cobra
		float closure = self->GetKias() - self->incomingMissile[0]->GetKias();
		if (((MissileClass *)self->incomingMissile[0])->targetPtr)
			{
			if (((MissileClass *)self->incomingMissile[0])->targetPtr->localData->range > 2.0f * NM_TO_FT
				|| closure < 400.0f)
				{
				MissileDragManeuver();
				}
			else
				{
				MissileBeamManeuver();
				}
			}

// 2000-09-08 ADDED BY S.G. SO WE DON'T GO IN AFTER BURNER WHEN AN IR MISSILE IS LAUNCHED AT US UNTIL WE TRY TO DITCH IT IF WE ARE VETERANS OR ACES
// WE DO THIS AFTER THE MANEUVERS SO WE OVERIDE THE THROTTLE SETTING BY THESE MANEUVERS
	    if (((MissileClass *)self->incomingMissile[0])->GetSeekerType() == SensorClass::IRST && SkillLevel() > 2)
//		    throtl = min (throtl, 0.0f);//me123 from 1.0 to 0 (we wanna go idle // S.G. RP4 COMPABILITY
		    throtl = min (throtl, 0.99f);
// END OF ADDED SECTION

  	}
  	else
		MissileLastDitch( dx, dy, dz);

}

int DigitalBrain::MissileBeamManeuver(){
	int retval = FALSE;
	float nh1,nh2,az;
	float at;
	mlTrig trig;

	/*-----------------*/
	/* missile heading */
	/*-----------------*/
	az = self->incomingMissile[0]->Yaw();

	/*--------------------*/
	/* pick a new heading */
	/*--------------------*/
	nh1 = az + PI * 0.5F;
	if (nh1 > PI)
		nh1 -= PI;

   nh2 = az - PI * 0.5F;
	if (nh2 < -PI)
		nh2 += PI;

   if (fabs(self->Yaw() - nh1) < fabs(self->Yaw() - nh2))
		az = nh1;
   else
		az = nh2;

   /*------------------------------------*/
   /* find a stationary point 10 NM away */
   /* in the nh? direction.              */
   /*------------------------------------*/
	mlSinCos(&trig, az);
	// Cobra - Use local max elevation to try and keep AI from lawndarting
	float tz = TheMap.GetMEA(((AircraftClass*) self)->XPos(), ((AircraftClass*) self)->YPos());
	//Cobra
	if (self->ZPos() < -20000.0f) {
	  tz = self->ZPos() - 10000.0f - trackZ;
	}
	else {
	  tz += -2000.0f;
	}
	SetTrackPoint(
		self->XPos() + 0.5F*NM_TO_FT*trig.cos, 
		self->YPos() + 0.5F*NM_TO_FT*trig.sin,
		tz
	);


   /*----------------------------------------------------------*/
   /* use AUTO_TRACK to steer toward point, override gs limits */
   /*----------------------------------------------------------*/
   at = AutoTrack(maxGs);
	//MachHold(2*cornerSpeed,self->GetKias(), TRUE);//Cobra We want corner speed on beams
   MachHold(cornerSpeed, self->GetKias(), TRUE);

   if (at < 5.0F)
		retval = TRUE;
	return (retval);
}

void DigitalBrain::MissileDragManeuver(void)
{
	float az;
	mlTrig trig;
	az = self->incomingMissile[0]->Yaw();//Cobra

	if (missileFindDragPt)
	{
		/*--------------------*/
		/* pick a new heading */
		/*--------------------*/
		// az = self->incomingMissile[0]->Yaw();

		/*-------------------------------------------*/
		/* find a stationary point 20 NM away in the */
		/* correct direction to chase.               */
		/*-------------------------------------------*/
		mlSinCos(&trig, az);

		// Cobra - Use local max elevation to try and keep AI from lawndarting
		float tz = TheMap.GetMEA(((AircraftClass*) self)->XPos(), ((AircraftClass*) self)->YPos()); 
		//Cobra
		if (self->ZPos() < -20000.0f){
			tz = self->ZPos() - 10000.0f - trackZ;
		}
		else {
			tz += -2000.0f;
		}
		SetTrackPoint(self->XPos() + 20.0F*NM_TO_FT*trig.cos, self->YPos() + 20.0F*NM_TO_FT*trig.sin, tz);
		missileFindDragPt = FALSE;
	}

	/*----------------------------------------------------------*/
	/* use AUTO_TRACK to steer toward point, override gs limits */
	/*----------------------------------------------------------*/
	AutoTrack(maxGs);
	MachHold(3*cornerSpeed,self->GetKias(), TRUE);
}

void DigitalBrain::MissileLastDitch(float xft, float yft, float zft)
{
//TJL 11/14/03. The roll code was causing the AI to never pull. It's been removed and now the AI pull to evade.
   //float tDroll,roll_offset,newroll,eroll;

   
   /*--------------*/
   /* target droll */
   /*--------------*/
   /*
   float rx = self->dmx[0][0]*xft + self->dmx[0][1]*yft + self->dmx[0][2]*zft;
   float ry = self->dmx[1][0]*xft + self->dmx[1][1]*yft + self->dmx[1][2]*zft;
   float rz = self->dmx[2][0]*xft + self->dmx[2][1]*yft + self->dmx[2][2]*zft;
   float droll =  (float)atan2 (ry,-rz);

   tDroll = droll*RTD;

  */
   /*------------------------------------------*/
   /* offset required to put wings on attacker */
   /*------------------------------------------*/

   /*
   if (tDroll >= 0.0)
		roll_offset = tDroll - 90.0F;
   else
		roll_offset = tDroll + 90.0F;
*/
   /*------------------------*/
   /* generate new phi angle */
   /*------------------------*/
//   newroll = self->Roll()*RTD + roll_offset;

   /*------------------------------------------*/
   /* roll the aircraft the shortest direction */
   /*------------------------------------------*/
   /*
   if (newroll > 180.0)
		newroll -= 360.0F;
   else if (newroll < -180.0)
		newroll += 360.0F;

   eroll = newroll - self->Roll();

   if (eroll > 180.0)
		eroll -= 360.0F;
   else if (eroll < -180.0)
		eroll += 360.0F;
*/
   /*------------------------------*/
   /* generate roll stick commands */
   /*------------------------------*/
  // SetRstick( eroll );
//   SetMaxRoll ((self->Roll() + eroll) * RTD);

   /*-----------*/
   /* max pitch */
   /*-----------*/
   SetPstick (maxGs, maxGs, AirframeClass::GCommand);

   /*----------------------*/
   /* center rudder pedals */
   /*----------------------*/
   SetYpedal( 0.0F );

   /*-------------------*/
   /* hold corner speed */
   /*-------------------*/
   MachHold(cornerSpeed,self->GetKias(), TRUE);

	/*------*/
	/* Pray */
	/*------*/


   if (missileDefeatTtgo > LD_TIME * 0.25F && missileDefeatTtgo < LD_TIME * 0.8F &&
      ((AircraftClass*)self)->HasPilot())
   {
      if (SimLibElapsedTime > self->ChaffExpireTime() )
      {
         ((AircraftClass*)self)->dropChaffCmd = TRUE;
      }
      if (SimLibElapsedTime > self->FlareExpireTime() )
      {
         ((AircraftClass*)self)->dropFlareCmd = TRUE;
      }
   }
}

int DigitalBrain::MissileEvade (void)
{
	return TRUE;
}


