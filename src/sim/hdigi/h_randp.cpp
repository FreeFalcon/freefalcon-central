#include "stdhdr.h"
#include "hdigi.h"
#include "object.h"
#include "simveh.h"

#define CONTROL_POINT_DISTANCE      2750.0F
#define CONTROL_POINT_ELEVATION         25.0F
#define MAGIC_NUMBER                     5.0F

void HeliBrain::RollAndPull(void)
{
   /*-------------------*/
   /* bail if no target */
   /*-------------------*/
   if (targetPtr == NULL) 
   {
      return;
   }

   /*-------------------------------------------*/
   /* roll and pull logic is geometry dependent */
   /*-------------------------------------------*/
   if (targetData->ata <= 90.0F * DTR)
   {
      /*--------------*/
      /* me -> <- him */
      /*--------------*/
      if (targetData->ataFrom <= 90.0F * DTR)
      {
         fCount = 0;

         PullToCollisionPoint();
         MachHold(1.1F*CORNER_SPEED, self->GetWPalt(), TRUE);
         //MachHold(1.1F*CORNER_SPEED, self->GetKias(), TRUE);
      }
      /*--------------*/
      /* me -> him -> */
      /*--------------*/
      else if (targetData->ataFrom >= 90.0F * DTR)
      {
         /*-------------------*/
         /* energy management */
         /*-------------------*/
         MaintainClosure();
         PullToCollisionPoint();

         /*------------------------------------------*/
         /* if overshoot situation roll out of plane */
         /*------------------------------------------*/
//         if (FLIGHT_PATH_OVERSHOOT() && roop.losDiff > 25.0) ADD_MODE(ROOP);
//         if (WING_LINE_OVERSHOOT()) ADD_MODE(ROOP);

         /*--------------------------------------------------*/
         /* if stagnated for X consecutive seconds, overbank */
         /*--------------------------------------------------*/
//         if (Stagnated());
//            AddMode(OverBMode);
      }
   }
   /*--------------*/
   /* him -> me -> */
   /*--------------*/
   else if (targetData->ata > 90.0F * DTR)
   {
      fCount = 0;

      /*-----------------------------*/
      /* inside of the control point */
      /*-----------------------------*/
      if (targetData->range < CONTROL_POINT_DISTANCE &&
                   targetData->ataFrom > 90.0 * DTR) PullToCollisionPoint();
      /*-----------*/
      /* otherwise */
      /*-----------*/
      else PullToControlPoint();
   }
}

void HeliBrain::PullToControlPoint(void)
{
float az,el;

   az = maxTargetPtr->BaseData()->Yaw();
   el = maxTargetPtr->BaseData()->Pitch();

   trackX = maxTargetPtr->BaseData()->XPos();
   trackY = maxTargetPtr->BaseData()->YPos();
   trackZ = maxTargetPtr->BaseData()->ZPos();
   trackX -= 2750.0F*(float)cos(az)*(float)cos(el);
   trackY += 2750.0F*(float)sin(az)*(float)cos(el);
   trackZ += 2750.0F*(float)sin(el);

   AutoTrack(100.0f);
	 MachHold(1.1F*CORNER_SPEED, self->GetWPalt(), TRUE);
   //MachHold(1.1F*CORNER_SPEED, self->GetKias(), TRUE);
}

void HeliBrain::PullToCollisionPoint(void)
{
float tc;

   /*------------------------*/
   /* find time to collision */
   /*------------------------*/
   tc = CollisionTime();

   /*------------------------------*/
   /* If collision time is defined */
   /* extrapolate targets position */
   /*------------------------------*/
   if (tc > 0.0 && targetData->range < 2.0*NM_TO_FT)
   {
      trackX = maxTargetPtr->BaseData()->XPos();
      trackY = maxTargetPtr->BaseData()->YPos();
      trackZ = maxTargetPtr->BaseData()->ZPos();
      trackX += maxTargetPtr->BaseData()->XDelta() * tc;
      trackY += maxTargetPtr->BaseData()->YDelta() * tc;
      trackZ += maxTargetPtr->BaseData()->ZDelta() * tc;
   }
   /*-----------------------------*/
   /* Collision point not defined */
   /* track a point X seconds     */
   /* ahead of the target.        */
   /*-----------------------------*/
   else
   {
      trackX = maxTargetPtr->BaseData()->XPos();
      trackY = maxTargetPtr->BaseData()->YPos();
      trackZ = maxTargetPtr->BaseData()->ZPos();
      trackX += maxTargetPtr->BaseData()->XDelta() * MAGIC_NUMBER;
      trackY += maxTargetPtr->BaseData()->YDelta() * MAGIC_NUMBER;
      trackZ += maxTargetPtr->BaseData()->ZDelta() * MAGIC_NUMBER;
   }
   AutoTrack(100.0f);
}

void HeliBrain::MaintainClosure(void)
{
float rng,closure,rngdot;

   /*------------------------*/
   /* range to control point */
   /*------------------------*/
   rng = targetData->range - CONTROL_POINT_DISTANCE;

   /*---------------------------------------*/
   /* desired in kts closure based on range */
   /*---------------------------------------*/
   closure = (rng / 500.0F) * 50.0F; /* farmer range*closure function */
   closure = min (max (closure, -100.0F), 300.0F);

   /*------------------------*/
   /* current closure in kts */
   /*------------------------*/
   rngdot = -rangedot * FTPSEC_TO_KNOTS;

   /*-------------------*/
   /* mach hold command */
   /*-------------------*/
   if (closure - rngdot > 0.0) 
      MachHold((self->GetKias() + (closure - rngdot)), self->GetWPalt(), TRUE);
   else
      MachHold((self->GetKias() + (closure - rngdot)), self->GetWPalt(), FALSE);
}

float HeliBrain::CollisionTime(void)
{
   return(targetData->range / -rangedot);
}

int HeliBrain::Stagnated(void)
{
	return FALSE;
}
