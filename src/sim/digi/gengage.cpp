#include "stdhdr.h"
#include "digi.h"
#include "simveh.h"
#include "object.h"
#include "PilotInputs.h"
#include "airframe.h"
#include "aircrft.h"
#include "guns.h"
#include "simdrive.h"
#include "sms.h"
#include "guns.h"
#include "classtbl.h"
#include "MsgInc\WeaponFireMsg.h"
#include "radar.h" // 2002-02-10 S.G.
/* S.G. for WeaponClassDataType */#include "entity.h"
#include "campbase.h" // 2002-02-10 S.G.

#define GUNS_LEAD      (3.0F * DTR)

FalconEntity* SpikeCheck (AircraftClass* self, FalconEntity *byHim = NULL, int *data = NULL);// 2002-02-10 S.G.

void DigitalBrain::GunsEngageCheck(void)
{
float angleLimit;

   if ((!mpActionFlags[AI_ENGAGE_TARGET] != AI_AIR_TARGET && missionClass != AAMission && !missionComplete) || curMode == RTBMode) // 2002-03-04 MODIFIED BY S.G. Use new enum type
      angleLimit = 15.0f * DTR;
   else
      angleLimit = 35.0f * DTR;

   /*-----------*/
   /* no target */
   /*-----------*/
   if (targetPtr == NULL) 
   {
      return;
   }

   /*-------*/
   /* entry */
   /*-------*/
   if (curMode != GunsEngageMode)
   {
      if (targetPtr && targetData->range <= 3500.0F && //targetData->range >= 1000.0F &&//me123 let's bfm to 4000 before gunning changed from 10000
// JB		!(self->YawDelta() > 0 && targetPtr->BaseData()->YawDelta() < 0 ||// not nose to nose
// JB		self->YawDelta() < 0 && targetPtr->BaseData()->YawDelta() > 0) &&
		  ((AircraftClass *)self)->Guns &&
   		((AircraftClass *)self)->Guns->numRoundsRemaining > 0 &&
          targetData->ata < angleLimit && IsSetATC (AceGunsEngage))
      {
         AddMode(GunsEngageMode);
      }
   }
   /*------*/
   /* exit */
   /*------*/
   else if (curMode == GunsEngageMode)
   {
      if (targetData->range < 3500.0f && //targetData->range > 1000.0f &&
// JB		!(self->YawDelta() > 0 && targetPtr->BaseData()->YawDelta() < 0 ||// not nose to nose
// JB		self->YawDelta() < 0 && targetPtr->BaseData()->YawDelta() > 0) &&
         ((AircraftClass *)self)->Guns->numRoundsRemaining > 0 && 
		 (targetPtr->BaseData()->IsAirplane() || targetPtr->BaseData()->IsHelicopter()) // 2002-03-05 MODIFIED BY S.G. airplane, choppers and fligth are ok in here (choppers only makes it here if it passed the SensorFusion test first)
		 //&& targetData->ata < 135.0f * DTR) 
           && targetData->ata < 1.25 * angleLimit)
      {
         AddMode(GunsEngageMode);
      }
   }
}

void DigitalBrain::GunsEngage(void)
{
float desiredClosure,actualClosure,lagAngle;

   if (targetPtr == NULL) 
   {
      return;
   }

   /*-----------------------------*/
   /* if ahead of target 3/9 line */
   /*-----------------------------*/
   if(targetData->ataFrom < 90.0F * DTR)//me123 this looks like a snapshot
   {
      // Pointing near, so look for a shot
      FineGunsTrack(cornerSpeed,&lagAngle); 
   }         
   else 
   /*---------------------------*/
   /* if behind target 3/9 line */
   /*---------------------------*/
   {
	float CONTROL_POINT_DISTANCE = 1400.0f;
	float rngdot;
	float rng;
	float closure;
   /*------------------------*/
   /* range to control point */
   /*------------------------*/
	if (targetData->ataFrom >= 120 *DTR)
	   {rng = targetData->range - CONTROL_POINT_DISTANCE;}

	else {rng = -targetData->range + 3000.0f - CONTROL_POINT_DISTANCE  ;}

   /*------------------------*/
   /* current closure in kts */
   /*------------------------*/
   rngdot = -targetData->rangedot * FTPSEC_TO_KNOTS;

   /*---------------------------------------*/
   /* desired in kts closure based on range */
   /*---------------------------------------*/
    closure = (((rng - rngdot *5) / 1000.0F) * 50.0F); /* farmer range*closure function */ //me123
    closure = min (max (closure, -350.0F), 1000.0F);
	closure = min (closure, targetPtr->BaseData()->GetKias() + 50.0f);
	desiredClosure = closure;


      /*--------------------------------------------------------------------*/
      /* use one dimensional interpolation to find desired closure in knots */
      /*--------------------------------------------------------------------*/
  //    desiredClosure = (100.0F * targetData->range / 2000.0F) - 100.0F; //me123
      /*--------------------*/
      /* get actual closure */
      /*--------------------*/
      actualClosure = -targetData->rangedot * FTPSEC_TO_KNOTS;

      /*-------------------------------------------------------*/
      /* if too close and too fast, bail out on the high side  */
      /*-------------------------------------------------------*/
      if( targetData->range < 2000.0F)//me123 changed from 2000
      {
         if( actualClosure > desiredClosure)
         {
			
				//          AddMode (RoopMode);
			 //me123           FineGunsTrack(self->GetKias()+(desiredClosure - actualClosure),&lagAngle);
						//      FineGunsTrack(min (targetPtr->BaseData()->GetKias() -250,self->GetKias()+(desiredClosure - actualClosure)),&lagAngle);//me123 erhh don't use corner here, it's not good you might overshoot.(max (cornerSpeed, self->GetKias()+(desiredClosure - actualClosure)),&lagAngle);
						MachHold(targetPtr->BaseData()->GetKias()-100.0f, self->GetKias(), FALSE);//me123 addet
						if (targetData->range < 800.0F) // JB 010212 1000.0F)
						//{ // JB 010212
							RollAndPull();
						//	return; // JB 010212
						//} // JB 010212
						FineGunsTrack(min (targetPtr->BaseData()->GetKias(),self->GetKias()+(desiredClosure - actualClosure)),&lagAngle);//me123 erhh don't use corner here, it's not good you might overshoot.(max (cornerSpeed, self->GetKias()+(desiredClosure - actualClosure)),&lagAngle); // JB 010212

			//            MonoPrint ("too close and too fast, let's BFM");
         }

         /*---------------------------------------*/
         /* if too close and slow, point to shoot */
         /* if lagging behind target.             */
         /*---------------------------------------*/
         else
         {
            FineGunsTrack(min (targetPtr->BaseData()->GetKias(),self->GetKias()+(desiredClosure - actualClosure)),&lagAngle);//me123 erhh don't use corner here, it's not good you might overshoot.(max (cornerSpeed, self->GetKias()+(desiredClosure - actualClosure)),&lagAngle);
            if (Stagnated())
            {
               MonoPrint ("Help me, I'm Stuck in a Luffberry\n");
            }
         }
      }
      else
      /*-------------------------------------------------------*/
      /* if too far and too fast, point to shoot and slow      */
      /*-------------------------------------------------------*/
      {
         if( actualClosure > desiredClosure )
         {
						FineGunsTrack(min (targetPtr->BaseData()->GetKias(), self->GetKias()+(desiredClosure - actualClosure)),&lagAngle);//me123 erhh don't use corner here, it's not good you might overshoot.(max (cornerSpeed, self->GetKias()+(desiredClosure - actualClosure)),&lagAngle);
         }
         /*-------------------------------------------------------*/
         /* if too far and slow, point to shoot, and overbank     */
         /* if lagging behind target.                             */
         /*-------------------------------------------------------*/
         else
         {
            FineGunsTrack(min (targetPtr->BaseData()->GetKias() + 30.0f,self->GetKias()+(desiredClosure - actualClosure)),&lagAngle);//me123 erhh don't use corner here, it's not good you might overshoot.(max (cornerSpeed, self->GetKias()+(desiredClosure - actualClosure)),&lagAngle);
            if( lagAngle > 2.0F * DTR)
               AddMode (OverBMode);
         }
      } /* check on range desired */
   } /* behind 3/9 line */
}

//void DigitalBrain::CoarseGunsTrack(float speed,float leadTof,float *newata)
void DigitalBrain::CoarseGunsTrack(float, float leadTof,float *newata)
{
float ata, gunFactor;
float multiplier;

/*-----------------------------------------------------------------*/
/* Project ahead target leadTof number of bullet times of flight  */
/*-----------------------------------------------------------------*/

gunFactor = leadTof * targetData->range/(self->Guns->initBulletVelocity + self->GetVt());
	if (targetData->ata > 45.0F * DTR){
		multiplier = max (1.0F - ((targetData->ata - 45.0F * DTR) / (45.0F * DTR)), 0.0F);
		gunFactor *= multiplier;
	}

	float tx = targetPtr->BaseData()->XPos() + targetPtr->BaseData()->XDelta() * gunFactor;
	float ty = targetPtr->BaseData()->YPos() + targetPtr->BaseData()->YDelta() * gunFactor;
	float tz = targetPtr->BaseData()->ZPos() + targetPtr->BaseData()->ZDelta() * gunFactor;
	tz -= 0.5F * GRAVITY * gunFactor * gunFactor * 4.0F;
	SetTrackPoint(tx, ty, tz);
	ata = GunsAutoTrack(maxGs);
	*newata = ata;
}

void DigitalBrain::FineGunsTrack(float speed, float *lagAngle)
{
float rx=0.0F,ry=0.0F,rz=0.0F,pipperEl=0.0F,pipperAz=0.0F;
float ata=0.0F,dx=0.0F,dy=0.0F,dz=0.0F,tf=0.0F,rangeEst=0.0F,elerr=0.0F;
float azerr=0.0F,cata=0.0F,atadot=0.0F,pipperRate=0.0F,pipperAta=0.0F;
float lastStick=0.0F, leadTime=0.0F;

   /*----------------------------------------------------------*/
   /* start by seeing if the target is flying upder our pipper */
   /* and fire at him if he should be so unlucky.              */
   /*----------------------------------------------------------*/

   /*----------------*/
   /* range estimate */
   /*----------------*/
   rangeEst = min (targetData->range,2000.0F);//me123 from 10000

	SimObjectType* localTarget;
	if (targetPtr)
		localTarget = targetPtr;
	else
		localTarget = threatPtr;

   tf = rangeEst / (self->Guns->initBulletVelocity + self->GetVt());
	 if (localTarget && localTarget->localData)
		 tf = rangeEst / (self->Guns->initBulletVelocity + self->GetVt() - localTarget->localData->rangedot); // JB 010211
	 else
		 tf = rangeEst / (self->Guns->initBulletVelocity + self->GetVt());

   /*------------------------------*/
   /* Ownship Speed + gravity Drop */
   /*------------------------------*/
   dx = self->GetVt() * self->platformAngles.cosgam * self->platformAngles.cossig * tf;
   dy = self->GetVt() * self->platformAngles.cosgam * self->platformAngles.sinsig * tf;
   dz = -(self->GetVt() * self->platformAngles.singam * tf + 0.5F * GRAVITY * tf * tf);

   /*------------*/
   /* Muzzle Vel */
   /*------------*/
   dx += self->Guns->initBulletVelocity * self->platformAngles.costhe * self->platformAngles.cospsi * tf;
   dy += self->Guns->initBulletVelocity * self->platformAngles.costhe * self->platformAngles.sinpsi * tf;
   dz -= self->Guns->initBulletVelocity * self->platformAngles.sinthe * tf;

   /*----------------------------*/
   /* find pipper body az and el */
   /*----------------------------*/
   rx = self->dmx[0][0]*dx + self->dmx[0][1]*dy + self->dmx[0][2]*dz;
   ry = self->dmx[1][0]*dx + self->dmx[1][1]*dy + self->dmx[1][2]*dz;
   rz = self->dmx[2][0]*dx + self->dmx[2][1]*dy + self->dmx[2][2]*dz;

   pipperAz = (float)atan2 (ry,rx);
   pipperEl = (float)atan2(-rz,(float)sqrt(rx*rx+ry*ry+.1f));

   pipperAta = (float)sqrt(pipperEl*pipperEl + pipperAz*pipperAz);
   pipperRate = (pipperAta - pastPipperAta)*SimLibMajorFrameRate;
   pastPipperAta = pipperAta;
   *lagAngle = pipperEl;

   /*-----------------------------------------------*/
   /* find error between pipper and target location */
   /*-----------------------------------------------*/
   azerr = targetData->az - pipperAz;
   elerr = targetData->el - pipperEl;
   ata = (float)sqrt(azerr*azerr + elerr*elerr);
   atadot = (ata - pastAta)*SimLibMajorFrameRate;
   pastAta = ata;

   /*-------------------------------------------------------------*/
   /* shoot gun if pipper close, pipper moving twords target, etc */
   /*-------------------------------------------------------------*/

   /*-----------------------------------------------------------*/
   /* begin steering commands by pointing 1.2 bullet times of   */
   /* ahead of the target. When we get near there, unload the   */
   /* aircraft and see if the target will fly under the pipper. */
   /* If he doesn't, start again.                               */
   /*-----------------------------------------------------------*/

   if (ata > 60.0F * DTR)
      speed = cornerSpeed;

   if( !waitingForShot )
   {
//      leadTime = 1.5F + 5.0F * targetData->ataFrom / (180.0F * DTR);
// MODIFIED TO ACCOUNT FOR DIFFERENT PULL OF THE TARGET
//      leadTime = 1.5F;
      leadTime = 2.0F + 2.0F * (float)sin(targetData->ataFrom);
      CoarseGunsTrack(speed, leadTime, &cata);

// MODIFIED BY S.G. TO MAKE IT MORE PRECISE AND WE NEED TO TAKE THE abs OF elerr
//      if( fabs (azerr) < 1.5 * DTR && elerr < 1.0F * DTR)
//me123      if( fabs (azerr) < 0.3 * DTR && elerr < 0.5F * DTR && elerr > -0.5F * DTR)      {
// JB      if( fabs (azerr) < 3.5 * DTR && fabs(elerr) < 3.5F * DTR)      {
      if( fabs (azerr) < 2.0 * DTR && elerr < .5F * DTR && elerr > -2.0F * DTR)      {
			  waitingForShot = TRUE;
         pastPstick = af->nzcgb;
      }
   }
   else
   {
// MODIFIED BY S.G. TO MAKE IT MORE PRECISE
//      if (elerr < 10.0F * DTR && elerr > -15.0F * DTR && fabs(azerr) < 10.0F /* ADDED BY S.G. - IT'S IN RADIAN! */ * DTR &&
//ajusted by me123      if (elerr < 1.0F * DTR && elerr > -1.5F * DTR && fabs(azerr) < 1.0F /* ADDED BY S.G. - IT'S IN RADIAN! */ * DTR &&
// JB     if (elerr < 3.0F * DTR && fabs(elerr) < 2.5F * DTR && fabs(azerr) < 3.0F /* ADDED BY S.G. - IT'S IN RADIAN! */ * DTR &&
      if( fabs (azerr) < 1.5 * DTR && elerr < .5F * DTR && elerr > -1.5F * DTR &&
          atadot < 50.0F* DTR && targetData->range < 2.0F * self->Guns->initBulletVelocity)//ME123 FROM 0.1 TO 0.2
      {
         SetFlag (GunFireFlag);
         //MonoPrint ("Digi Firing %8ld   %4d -> %4d\n", SimLibElapsedTime,
            //self->Id().num_, targetPtr->BaseData()->Id().num_);
      }

       /*---------------------------------*/
       /* Relax stick and neutralize roll */
       /*---------------------------------*/
      SetPstick (max (pastPstick - 1.0F, 0.0F), maxGs, AirframeClass::GCommand);
      SetRstick( 0.0F );         
      if( fabs (azerr) < 3.5 * DTR  && elerr < .5F * DTR && elerr > -1.5F * DTR &&
      fabs(pipperRate) < 10.0F * DTR && targetData->range < 3000.0F)//me123 from 6000 
      {
         waitingForShot = (ataDot < 0.01F ? TRUE : FALSE);
      }
      else
         waitingForShot = 0;
   }

   lastStick = pStick;
   MachHold(speed, self->GetKias(), FALSE);//me123 true

   // Check for full pull
   if (!waitingForShot && cata < 5.0F * DTR * (af->GsAvail() - af->nzcgb))
      pStick = lastStick;
}

float DigitalBrain::GunsAutoTrack(float trackGs)
{
float xft,yft,zft,rx,ry,rz,ata,droll;
float pullFact;

   /*-----------------------------*/
   /* calculate relative position */
   /*-----------------------------*/
   xft = trackX - self->XPos();
   yft = trackY - self->YPos();
   zft = trackZ - self->ZPos();

	// JB 010210 Start
	SimObjectType* localTarget;
	FalconEntity	*target;
	if (targetPtr)
		localTarget = targetPtr;
	else
		localTarget = threatPtr;
	float realRange, tof;
	realRange = (float)sqrt( xft * xft + yft * yft + zft * zft );

	// Guess TOF
	if (localTarget && localTarget->localData)
		tof = realRange / (self->Guns->initBulletVelocity + self->GetVt() - localTarget->localData->rangedot);
	else
		tof = realRange / (self->Guns->initBulletVelocity + self->GetVt());

	if (localTarget)
	{
		target = localTarget->BaseData();
		if (target)
		{
			// now get vector to where we're aiming
			xft += (target->XDelta() - self->XDelta()) * tof;
			yft += (target->YDelta() - self->YDelta()) * tof;
			zft += (target->ZDelta() - self->ZDelta()) * tof - 4.0f;
		}
	}

	// Correct for gravity
	zft -= GRAVITY * tof * tof;
	// JB 010210 End

   rx = self->dmx[0][0]*xft + self->dmx[0][1]*yft + self->dmx[0][2]*zft;
   ry = self->dmx[1][0]*xft + self->dmx[1][1]*yft + self->dmx[1][2]*zft;
   rz = self->dmx[2][0]*xft + self->dmx[2][1]*yft + self->dmx[2][2]*zft;

   // Bias for lead
   rz     = -(rz * 2.0F);
   droll  = (float)atan2(ry,rz);

   // Bias X degrees upward

   // Ata including elevation bias
   ata    = (float)atan2(sqrt(ry*ry+rz*rz),rx);

   // Scale pull based on roll error
   pullFact = min ((25.0F * DTR) / (float)fabs(droll), 1.0F);

   SetPstick( ata * RTD * 2.0F * pullFact, trackGs, AirframeClass::GCommand);
	SetRstick( droll*RTD);
   SetYpedal( 0.0F);

   SetMaxRoll ((float)fabs(self->Roll() + droll) * RTD);
   SetMaxRollDelta (droll * RTD);

   return (ata);
}

void DigitalBrain::TrainableGunsEngage(void)
{
int i, angles = FALSE;
int fireFlag = TRUE;
GunClass* theGun = NULL;
TransformMatrix gMat;
SimObjectType* localTarget;
//float az, el, xft, yft, zft, azErr, dt; // JB 010210
float xft, yft, zft, azErr;
mlTrig trigAz, trigEl;

   if (targetPtr)
      localTarget = targetPtr;
   else
      localTarget = threatPtr;

   // Don't shoot if far away
   if (localTarget->localData->range > 2.0F * NM_TO_FT)//me123 from 5
      return;

   // Find the trainable gun
   for (i=0; i<self->Sms->NumHardpoints(); i++)
   {
	   //!
      if ((self->Sms->hardPoint[i]->GetWeaponData()->flags & SMSClass::Trainable) &&
          (theGun = self->Sms->hardPoint[i]->GetGun()))
      {
         // Tail guns point out the rear (obviously)
         if (theGun->EntityType()->classInfo_[VU_STYPE] == STYPE_TAIL_GUN && !localTarget->BaseData()->OnGround())
         {
            theGun->unlimitedAmmo = TRUE;
            if (localTarget->localData->az > 0.0F)
               azErr = 180.0F*DTR - localTarget->localData->az;
            else
               azErr = -180.0F*DTR - localTarget->localData->az;

            // Within 30 degree box and 2 NM, fire that weapon
            if (fabs (azErr) < 30.0F * DTR && fabs(localTarget->localData->el) < 30.0F * DTR &&
               localTarget->localData->range < 2.0F * NM_TO_FT)
            {
               angles = TRUE;
            }
         }
         else if (localTarget->BaseData()->OnGround())
         {
            // For now assume all other guns point out the left side

            // Within 15 degree box and 2 NM, fire that weapon
            if (fabs (-90.0F * DTR - localTarget->localData->az) < 15.0F * DTR &&
               localTarget->localData->range < 2.0F * NM_TO_FT)
            {
               angles = TRUE;
            }
         }

         if (angles && SimLibElapsedTime % 2000 < 500)
         {
						// JB 010210 Start
						FalconEntity	*target;
						float realRange;
						float az, el, tof;

						target = localTarget->BaseData();
						xft = target->XPos() - self->XPos();
						yft = target->YPos() - self->YPos();
						zft = target->ZPos() - self->ZPos() + 4.0f;
						realRange = (float)sqrt( xft * xft + yft * yft + zft * zft );

						// Guess TOF
						tof = realRange / (theGun->initBulletVelocity + self->GetVt() - localTarget->localData->rangedot);

						// now get vector to where we're aiming
						xft += (target->XDelta() - self->XDelta()) * tof;
						yft += (target->YDelta() - self->YDelta()) * tof;
						zft += (target->ZDelta() - self->ZDelta()) * tof;

						// Correct for gravity
						zft -= GRAVITY * tof * tof;

						localTarget->localData->az = (float)atan2(yft,xft);
						localTarget->localData->el = (float)atan(-zft/(float)sqrt(xft*xft + yft*yft +0.1F));
						localTarget->localData->range = realRange;

						az = localTarget->localData->az - self->Yaw();
						el = localTarget->localData->el - self->Pitch();

						mlSinCos (&trigEl, localTarget->localData->el);
						mlSinCos (&trigAz, localTarget->localData->az);

						/*

												dt = localTarget->localData->range / (3000.0F - localTarget->localData->rangedot);

												xft = localTarget->BaseData()->XPos() + localTarget->BaseData()->XDelta() * dt - self->XPos();
												yft = localTarget->BaseData()->YPos() + localTarget->BaseData()->YDelta() * dt - self->YPos();
												zft = localTarget->BaseData()->ZPos() + localTarget->BaseData()->ZDelta() * dt - self->ZPos();
												zft += 0.5F * GRAVITY * dt * dt;
            
												az = (float)atan2(yft,xft);
												// sqrt returns positive, so this is cool
												el = (float)atan2(-zft,sqrt(xft*xft+yft*yft));

												mlSinCos (&trigAz, az);
												mlSinCos (&trigEl, el);
						*/
						// JB 010210 End

            // Gun pointing matrix - NOTE roll = 0.0F
            gMat[0][0] = trigAz.cos * trigEl.cos;
            gMat[0][1] = trigAz.sin * trigEl.cos;
            gMat[0][2] = -trigEl.sin;

            gMat[1][0] = -trigAz.sin;
            gMat[1][1] = trigAz.cos;
            gMat[1][2] = 0.0F;

            gMat[2][0] = trigAz.cos * trigEl.sin;
            gMat[2][1] = trigAz.sin * trigEl.sin;
            gMat[2][2] = trigEl.cos;

            fireFlag = TRUE;
            if (!IsSetATC(FireTrainable))
            {
               self->SendFireMessage (theGun, FalconWeaponsFire::GUN, TRUE, localTarget);
               SetATCFlag (FireTrainable);
            }
         }
         else
         {
            fireFlag = FALSE;
            if (IsSetATC(FireTrainable))
            {
               self->SendFireMessage (theGun, FalconWeaponsFire::GUN, FALSE, localTarget);
               ClearATCFlag (FireTrainable);
            }
         }

         // Keep the tracers alive
         theGun->Exec(&fireFlag, gMat, &self->platformAngles, targetList, FALSE );
      }
   }
}
