#include "stdhdr.h"
#include "missile.h"
#include "simmath.h"

void MissileClass::Engine(void)
{
float thrust, impulse;

	if (!ifd)
		return; // JB 010720

   /*---------------*/
   /* Thrust lookup */
   /*---------------*/
   thrust = Math.OnedInterp(runTime, engineData->times,
                        engineData->thrust, engineData->numBreaks,
                        &ifd->burnIndex );

   if (runTime > engineData->times[engineData->numBreaks-1])
   {
      thrust = 0.0F;
      ifd->xprop = 0.0F;
   }

   /*-----------------*/
   ifd->xprop = (thrust * (1.0F + (PASL - ps)/PASL))/ mass;
   ifd->yprop = 0.0F;
   ifd->zprop = 0.0F;

   /*----------------------*/
   /* stability axis accel */
   /*----------------------*/
   ifd->xsprop = ifd->xprop * ifd->geomData.cosalp + ifd->zprop * ifd->geomData.sinalp;
   ifd->ysprop = ifd->yprop;    
   ifd->zsprop = ifd->zprop*ifd->geomData.cosalp - ifd->xprop*ifd->geomData.sinalp;

   /*-----------------*/
   /* wind axis accel */
   /*-----------------*/
   ifd->xwprop =  ifd->xsprop * ifd->geomData.cosbet + ifd->ysprop * ifd->geomData.sinbet;
   ifd->ywprop = -ifd->xsprop * ifd->geomData.sinbet + ifd->ysprop * ifd->geomData.cosbet;
   ifd->zwprop =  ifd->zsprop;


   /*-------------------------*/
   /* impulse of rocket motor */
   /*-------------------------*/
   impulse = Math.FITust(thrust,SimLibMinorFrameTime,ifd->oldimp);

   /*-----------------------*/
   /* new propellant weight */
   /*-----------------------*/
   wprop  = inputData->wp0*(1.0F - impulse/inputData->totalImpulse);
   if(wprop <=0.0F)
      wprop = 0.0F;

   /*---------------------------------------------*/
   /* new mass and weight of vehicle / propellant */
   /*---------------------------------------------*/
   weight = inputData->wm0 + wprop;
   if (ifd->stage2gone) // subtract the weight
	   weight -= auxData->SecondStageWeight;
   mass   = weight/GRAVITY;
   mprop  = wprop/GRAVITY;

   if (ifd->xprop > 0.0F)//me123 smoke as logs as there is thrust
      SetPowerOutput(1.0F);
   else
      SetPowerOutput(0.0F);
}
