#include "stdhdr.h"
#include "missile.h"

void MissileClass::Trigenometry(void)
{
	float t1,t2;
	mlTrig trig;

	if (!ifd)
		return; // JB 010803

   mlSinCos (&trig, psi);
   ifd->geomData.cospsi = trig.cos;
   ifd->geomData.sinpsi = trig.sin;
   mlSinCos (&trig, phi);
   ifd->geomData.cosphi = trig.cos;
   ifd->geomData.sinphi = trig.sin;
   mlSinCos (&trig, theta);
   ifd->geomData.costhe = trig.cos;
   ifd->geomData.sinthe = trig.sin;
   mlSinCos (&trig, alpha*DTR);
   ifd->geomData.cosalp = trig.cos;
   ifd->geomData.sinalp = trig.sin;
   mlSinCos (&trig, beta*DTR);
   ifd->geomData.cosbet = trig.cos;
   ifd->geomData.sinbet = trig.sin;

   ifd->geomData.tanalp = (float)tan(alpha*DTR);
   ifd->geomData.tanbet = (float)tan(beta*DTR);

   /*-----------------------------*/
   /* velocity vector orientation */
   /*-----------------------------*/
   /*-------*/
   /* gamma */
   /*-------*/
   ifd->geomData.singam = (ifd->geomData.sinthe*ifd->geomData.cosalp - ifd->geomData.costhe*
            ifd->geomData.cosphi*ifd->geomData.sinalp)*ifd->geomData.cosbet -
            ifd->geomData.costhe*ifd->geomData.sinphi*ifd->geomData.sinbet;
   ifd->geomData.cosgam = (float)sqrt(1.0f - (ifd->geomData.singam*ifd->geomData.singam));

   ifd->gamma  = (float)atan2(ifd->geomData.singam,ifd->geomData.cosgam);

   /*----*/
   /* mu */
   /*----*/
   t1 = ifd->geomData.costhe*ifd->geomData.sinphi*ifd->geomData.cosbet +
            (ifd->geomData.sinthe*ifd->geomData.cosalp - ifd->geomData.costhe*ifd->geomData.cosphi*
            ifd->geomData.sinalp)*ifd->geomData.sinbet;

   t2 = ifd->geomData.costhe*ifd->geomData.cosphi*ifd->geomData.cosalp +
            ifd->geomData.sinthe*ifd->geomData.sinalp;

   ifd->mu     = (float)atan2(t1,t2);
   ifd->geomData.sinmu  = t1;
   ifd->geomData.cosmu  = t2;



   /*-------*/
   /* sigma */
   /*-------*/
   t1 = ( -ifd->geomData.sinphi * ifd->geomData.sinalp * ifd->geomData.cosbet +
            ifd->geomData.cosphi * ifd->geomData.sinbet) * ifd->geomData.cospsi +
            (( ifd->geomData.costhe*ifd->geomData.cosalp +
            ifd->geomData.sinthe*ifd->geomData.cosphi*ifd->geomData.sinalp)*ifd->geomData.cosbet
         +    ifd->geomData.sinthe*ifd->geomData.sinphi*ifd->geomData.sinbet)*ifd->geomData.sinpsi;

   t2 =  ((ifd->geomData.costhe*ifd->geomData.cosalp + ifd->geomData.sinthe*
              ifd->geomData.cosphi*ifd->geomData.sinalp)*ifd->geomData.cosbet
         +    ifd->geomData.sinthe*ifd->geomData.sinphi*ifd->geomData.sinbet)*ifd->geomData.cospsi
         +   (ifd->geomData.sinphi*ifd->geomData.sinalp*ifd->geomData.cosbet - ifd->geomData.cosphi
         *   ifd->geomData.sinbet)*ifd->geomData.sinpsi;

   ifd->sigma = (float)atan2(t1,t2);
   ifd->geomData.sinsig = t1;
   ifd->geomData.cossig = t2;
 
   /*-----------------------*/
   /* total angle of attack */
   /*-----------------------*/

#if 1
   alphat = RTD*(float)acos(ifd->geomData.cosalp*ifd->geomData.cosbet);
#else
   {
     float foo = (ifd->geomData.cosalp*ifd->geomData.cosbet);
     alphat = RTD*(float)atan2(sqrt(1.0-foo*foo),foo);
   }
#endif

}
