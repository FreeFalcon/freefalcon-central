#include "stdhdr.h"
#include "missile.h"

void MissileClass::Gains(void)
{
float pcoef1;        /* Used in calculating pitch axis gains */
float pcoef2;
float pfreq1;
float pfreq2;
float pradcl;
float ycoef1;        /* Used in calculating yaw axis gains */
float ycoef2;
float yfreq1;
float yfreq2;
float yradcl;

	if (!ifd)
		return; // JB 010720

   /*----------------------------------------*/
   /* pitch axis gains and filter parameters */
   /*----------------------------------------*/
   ifd->tp01 = 0.100F;
   ifd->tp02 = 0.200F;
   ifd->zp01 = 0.950F;
   ifd->wp01 = 3.0F*(ifd->qovt - 0.12F) + 0.5F;

   /*-----------------------------*/
   /* limit closed loop frequency */
   /*-----------------------------*/
	ifd->wp01 = min ( max (ifd->wp01, 0.5F), 5.0F);
   ifd->kp02 = 1.000F;
   ifd->kp03 = 2.000F;
   ifd->kp04 = 2.085F;

   /*----------------------------------------------*/
   /* calculate inner loop dynamics for pitch axis */
   /*----------------------------------------------*/
   pcoef1 =  ifd->tp02*ifd->kp04*ifd->wp01*ifd->wp01/ifd->kp03 -
               2*ifd->zp01*ifd->wp01 - ifd->kp04;
   pcoef2 = (1 - ifd->kp04*(1/ifd->kp03 + ifd->tp02))*
            ifd->wp01*ifd->wp01 + 2*ifd->zp01*ifd->wp01*ifd->kp04;
   pradcl =  pcoef1*pcoef1 - 4.0F*pcoef2;

   pfreq1 = ((float)sqrt(pradcl) - pcoef1)/2.0F;
   pfreq2 = -pcoef1 - pfreq1;

   /*------------------------------------------*/
   /* time constants for pitch axis inner loop */
   /*------------------------------------------*/
   ifd->tp03   =  1.0F/pfreq1;
   ifd->tp04   =  1.0F/pfreq2;
   ifd->kp05   =  GRAVITY*ifd->kp04*ifd->wp01*ifd->wp01/
            (ifd->qsom*ifd->clalph*ifd->kp03*pfreq1*pfreq2);
   ifd->kp06   =  0.002F*ifd->qbar;
   if(ifd->kp06 >= 20.0) ifd->kp06 = 20.0F;
   ifd->kp07   =  2.00F;

   /*--------------------------------------*/
   /* yaw axis gains and filter parameters */
   /*--------------------------------------*/
   ifd->ty01 = 0.10F;
   ifd->ty02 = 0.200F;
   ifd->zy01 = 0.950F;
   ifd->wy01 = 3.0F*(ifd->qovt - 0.12F) + 0.5F;

   /*-----------------------------*/
   /* limit closed loop frequency */
   /*-----------------------------*/
	ifd->wy01 = min ( max (ifd->wy01, 0.5F), 5.0F);
   ifd->ky01 = 200.00F - ifd->geomData.costhe*ifd->geomData.sinphi;
   ifd->ky02 = 1.000F;
   ifd->ky03 = 2.000F;
   ifd->ky04 = 2.085F;

   /*--------------------------------------------*/
   /* calculate inner loop dynamics for yaw axis */
   /*--------------------------------------------*/
   ycoef1 =  ifd->ty02*ifd->ky04*ifd->wy01*ifd->wy01/ifd->ky03 -
               2*ifd->zy01*ifd->wy01 - ifd->ky04;
   ycoef2 = (1 - ifd->ky04*(1/ifd->ky03 + ifd->ty02))*
               ifd->wy01*ifd->wy01 + 2*ifd->zy01*ifd->wy01*ifd->ky04;
   yradcl =  ycoef1*ycoef1 - 4.0F*ycoef2;
   yfreq1 = ((float)sqrt(yradcl) - ycoef1)/2.0F;
   yfreq2 = -ycoef1 - yfreq1;

   /*----------------------------------------*/
   /* time constants for yaw axis inner loop */
   /*----------------------------------------*/
   ifd->ty03   =  1/yfreq1;
   ifd->ty04   =  1/yfreq2;
   ifd->ky05   = -GRAVITY*ifd->ky04*ifd->wy01*ifd->wy01/
            (ifd->qsom*ifd->cybeta*ifd->ky03*yfreq1*yfreq2);
   ifd->ky06   =  0.002F*ifd->qbar;
   if(ifd->ky06 >= 20.0)ifd->ky06 = 20.0F;
   ifd->ky07   =  2.00F;
}
