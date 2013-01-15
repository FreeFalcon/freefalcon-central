#include "stdhdr.h"
#include "hdigi.h"
#include "simveh.h"
#include "missile.h"
#include "sms.h"
#include "fcc.h"
#include "object.h"
#include "f4error.h"
#include "simbase.h"

void HeliBrain::MissileEngageCheck(void)
{
   /*-----------------------*/
   /* return if null target */
   /*-----------------------*/
   if (targetPtr == NULL)
   {
      if (curMode == MissileEngageMode) 
      {
//		 MonoPrint( "HELO BRAIN Exiting Missile Engange 1\n" );
      }
      return;
   }

   /*-------*/
   /* entry */
   /*-------*/
   if (curMode != MissileEngageMode && curMissile)
   {
//	  MonoPrint( "HELO BRAIN ENTERING Missile Engange 1\n" );
      AddMode(MissileEngageMode);
   }
   /*------*/
   /* exit */
   /*------*/
   else if (curMissile == NULL)
   {
//	  MonoPrint( "HELO BRAIN Exiting Missile Engange 2\n" );
   }
}

void HeliBrain::MissileEngage(void)
{
float desiredClosure, rdes,rngdot, desSpeed;
float xDot, yDot, zDot;
float tof, rMax;

   if (!targetPtr || !curMissile)
   {
//	  MonoPrint( "HELO BRAIN Exiting Missile Engange 3\n" );
      return;
   }

   // Set up for missile engage
   if (curMode != lastMode)
   {
   FireControlComputer::FCCSubMode newSubMode;

      self->FCC->SetMasterMode (FireControlComputer::Missile);
      switch (self->Sms->hardPoint[curMissileStation]->GetWeaponType())
      {
         case wtAim9:
            newSubMode = FireControlComputer::Aim9;
         break;

         case wtAim120:
            newSubMode = FireControlComputer::Aim120;
         break;

         default:
            newSubMode = FireControlComputer::Aim9;
         break;
      }
      self->FCC->SetSubMode (newSubMode);
   }

	/*---------------------------------------*/
	/* Find current missile's tof, rmax, etc */
	/*---------------------------------------*/

   if(curMissile->IsMissile())
   {
		tof  = ((MissileClass *)curMissile)->GetTOF((-self->ZPos()), self->GetVt(), targetData->ataFrom,
			targetPtr->BaseData()->GetVt(), targetData->range);

		rMax = ((MissileClass *)curMissile)->GetRMax((-self->ZPos()), self->GetVt(), targetData->az,
			targetPtr->BaseData()->GetVt(), targetData->ataFrom);
		
   }
   else
   {
	   // MLR just in case
		tof = 10;
		rMax = 6000;
   }


   /*---------------------------------*/
   /* Put a deadband on target's zdot */
   /*---------------------------------*/
	xDot = targetPtr->BaseData()->XDelta();
	yDot = targetPtr->BaseData()->YDelta();
	zDot = targetPtr->BaseData()->ZDelta();
	zDot = Math.DeadBand(zDot, -10.0F, 10.0F);

   /*----------------------------------*/
   /* Ownship ahead of target 3/9 line */
   /*----------------------------------*/
   if (fabs (targetData->azFrom) < 90.0F)
   {
      /*----------------------*/
      /* Find the track point */
      /*----------------------*/
      trackX = targetPtr->BaseData()->XPos() + xDot * tof;
      trackY = targetPtr->BaseData()->YPos() + yDot * tof;
      trackZ = targetPtr->BaseData()->ZPos() + zDot * tof;

      desSpeed = 2.0F * CORNER_SPEED;
   }
   /*-----------------------------------*/
   /* Ownship behind of target 3/9 line */
   /*-----------------------------------*/
   else
   {
      /*-------------------------------------------------------------*/
      /* Calculate desired closure                                   */
      /* desired closure is 10% of range in feet, expressed as knots */
      /* -100.0 Kts <= desired closure <= 300 Kts                    */
      /*-------------------------------------------------------------*/
      rdes = 0.40F * rMax;
      desiredClosure = 0.1F * (targetData->range - rdes);
      desiredClosure = min (max (desiredClosure, -100.0F * KNOTS_TO_FTPSEC),
            300.0F * KNOTS_TO_FTPSEC);

      /*-----------------*/
      /* Closing to fast */
      /*-----------------*/
      if (-targetData->rangedot > desiredClosure)
      {
         /*---------------------------------------*/
         /* Find the track point - Lag the target */
         /*---------------------------------------*/
      	trackX = targetPtr->BaseData()->XPos() + xDot * tof * 0.9F;
      	trackY = targetPtr->BaseData()->YPos() + yDot * tof * 0.9F;
      	trackZ = targetPtr->BaseData()->ZPos() + zDot * tof * 0.9F;
      }
      else
      /*-------------------------*/
      /* Not closing fast enough */
      /*-------------------------*/
      {

         /*----------------------*/
         /* Find the track point */
         /*----------------------*/
      	trackX = targetPtr->BaseData()->XPos() + xDot * tof;
      	trackY = targetPtr->BaseData()->YPos() + yDot * tof;
      	trackZ = targetPtr->BaseData()->ZPos() + zDot * tof;
      }

      /*--------------------------------------*/
      /* Set the throttle for desired closure */
      /*--------------------------------------*/
      rngdot = (targetData->rangedot) * FTPSEC_TO_KNOTS;
      desSpeed = self->GetKias() + desiredClosure - rngdot;

      if (desSpeed < 200.0) desSpeed = 200.0F;
   }

   /*----------*/
	/* Track It */
	/*----------*/
   AutoTrack (5.0F);
}
