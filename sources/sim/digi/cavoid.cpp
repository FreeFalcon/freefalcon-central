#include "stdhdr.h"
#include "classtbl.h"
#include "geometry.h"
#include "digi.h"
#include "simveh.h"
#include "object.h"
#include "Entity.h"
#include "Aircrft.h"

#define GS_LIMIT 9.0F

void DigitalBrain::CollisionCheck(void)
{
float relAz, relEl, range, reactTime;
float hRange    = 200.0F; /* range to miss a hostile tgt / fireball */
float hRangeSq = 40000.0F; /* square of hRange */
float reactFact = 0.55F; /* fudge factor for reaction time *///me123 from .75
float timeToImpact,rngSq,dt,pastRngSq;
float ox,oy,oz,tx,ty,tz;
int    collision;
Falcon4EntityClassType* classPtr;
SimObjectLocalData* localData;

	if ( !targetPtr )
	{
		return;
	}

   /*----------------------------------------------------------*/
   /* Reaction time is a function of gs available and          */
   /* agression level (gs allowed). 2 seconds is the bare      */
   /* minimum for most situations. Modify with reaction factor */
   /*----------------------------------------------------------*/
   reactTime = (GS_LIMIT / maxGs) * (0.0F + reactFact);//me123 0.0 from 1.0

   collision = FALSE;

   /*---------------*/
   /* check objects */
   /*---------------*/
   if ( !targetPtr->BaseData()->IsSim() )
   {
      return;
   }

   localData = targetPtr->localData;

   /*-----------------------*/
   /* aircraft objects only */
   /*-----------------------*/
   classPtr = (Falcon4EntityClassType*)targetPtr->BaseData()->EntityType();
   if (classPtr->vuClassData.classInfo_[VU_TYPE] == TYPE_AIRPLANE)
   {
      /*----------------*/
      /* time to impact */
      /*----------------*/
      timeToImpact = (localData->range - hRange) / -localData->rangedot;

      /*---------------*/
      /* not a problem */
      /*---------------*/
      if (timeToImpact > reactTime &&
          localData->range > hRange) 
      {
         return;
      }

      /*-------------------------------------------------------------*/
      /* check for close approach. Linearly extrapolate out velocity */
      /* vector for react time.                                      */
      /*-------------------------------------------------------------*/
      pastRngSq = 10.0e7F;
      dt = 0.05F;

      while (dt < reactTime)
      {
         ox = self->XPos() + self->XDelta()*dt;
         oy = self->YPos() + self->YDelta()*dt;
         oz = self->ZPos() + self->ZDelta()*dt;

         tx = targetPtr->BaseData()->XPos() + targetPtr->BaseData()->XDelta()*dt;
         ty = targetPtr->BaseData()->YPos() + targetPtr->BaseData()->YDelta()*dt;
         tz = targetPtr->BaseData()->ZPos() + targetPtr->BaseData()->ZDelta()*dt;

         rngSq = (ox-tx)*(ox-tx) + (oy-ty)*(oy-ty) + (oz-tz)*(oz-tz);

         /*------------------------------------------------*/
         /* collision possible if within hRange of target */
         /*------------------------------------------------*/
         if (rngSq <= hRangeSq)
         {
            collision = TRUE;
            break;
         }
         /*----------------------------------------------*/
         /* break out of loop if range begins to diverge */
         /*----------------------------------------------*/
         if (rngSq > pastRngSq) break;   
         pastRngSq = rngSq;

         dt += 0.1F;
      }

      /*------------------------------*/
      /* take action or bite the dust */
      /*------------------------------*/
      if (collision)
      {
         if (curMode != CollisionAvoidMode)
         {
            /*------------------------------------*/
            /* Find a point in the maneuver plane */
            /*------------------------------------*/
            relEl = 45.0F * DTR;
            if (localData->droll > 0.0)
               relAz = -45.0F * DTR;
            else
               relAz = 45.0F * DTR;

            range  = 10000.0F;

			float tx, ty, tz;
            GetXYZ (self, relAz, relEl, range, &tx, &ty, &tz);
			SetTrackPoint(tx, ty, tz);
         }

         AddMode(CollisionAvoidMode);
      }
   }
}

void DigitalBrain::CollisionAvoid(void)
{
   TrackPoint (maxGs, cornerSpeed /* KNOTS_TO_FTPSEC*/); // Cobra - aerial TrackPoint speed is in knots
}
