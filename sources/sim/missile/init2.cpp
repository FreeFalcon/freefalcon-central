#include "stdhdr.h"
#include "missile.h"

void MissileClass::Init2(void)
{
float e10, e20, e30, e40;      /* temp. quaternions for init */
mlTrig trigPsi, trigPhi, trigTheta;

   mlSinCos (&trigPsi,   psi*0.5F);
   mlSinCos (&trigPhi,   phi*0.5F);
   mlSinCos (&trigTheta, theta*0.5F);

   /*------------------------*/
   /* initialize quaternions */
   /*------------------------*/
   e10 = trigPsi.cos*trigTheta.cos*trigPhi.cos
         + trigPsi.sin*trigTheta.sin*trigPhi.sin;

   e20 = trigPsi.sin*trigTheta.cos*trigPhi.cos
         - trigPsi.cos*trigTheta.sin*trigPhi.sin;

   e30 = trigPsi.cos*trigTheta.sin*trigPhi.cos
         + trigPsi.sin*trigTheta.cos*trigPhi.sin;

   e40 = trigPsi.cos*trigTheta.cos*trigPhi.sin
         - trigPsi.sin*trigTheta.sin*trigPhi.cos;

	if (!ifd)
		return; // JB 010803

   ifd->e1 = e10;
   ifd->e2 = e20;
   ifd->e3 = e30;
   ifd->e4 = e40;

   /*-------------------*/
   /* direction cosines */
   /*-------------------*/
   dmx[0][0] = e10*e10 - e20*e20 - e30*e30 + e40*e40;
   dmx[0][1] = 2*(e30*e40 + e10*e20);
   dmx[0][2] = 2*(e20*e40 - e10*e30); 
   dmx[1][0] = 2*(e30*e40 - e10*e20);
   dmx[1][1] = e10*e10 - e20*e20 + e30*e30 - e40*e40;
   dmx[1][2] = 2*(e20*e30 + e40*e10);
   dmx[2][0] = 2*(e10*e30 + e20*e40);
   dmx[2][1] = 2*(e20*e30 - e10*e40);
   dmx[2][2] = e10*e10 + e20*e20 - e30*e30 - e40*e40;

   ifd->olde1[0] = e10;
   ifd->olde1[1] = e10;
   ifd->olde1[2] = 0.0;
   ifd->olde1[3] = 0.0;

   ifd->olde2[0] = e20;
   ifd->olde2[1] = e20;
   ifd->olde2[2] = 0.0;
   ifd->olde2[3] = 0.0;

   ifd->olde3[0] = e30;
   ifd->olde3[1] = e30;
   ifd->olde3[2] = 0.0;
   ifd->olde3[3] = 0.0;

   ifd->olde4[0] = e40;
   ifd->olde4[1] = e40;
   ifd->olde4[2] = 0.0;
   ifd->olde4[3] = 0.0;

   /*--------------------------------*/
   /* velocity vector initialization */
   /*--------------------------------*/
   vtdot =  ifd->xwaero + ifd->xwprop - GRAVITY*ifd->geomData.singam;
   xdot  =  vt*ifd->geomData.cosgam*ifd->geomData.cossig;
   ydot  =  vt*ifd->geomData.cosgam*ifd->geomData.sinsig;
   zdot  = -vt*ifd->geomData.singam ;

   ifd->oldvt[0] =  vt;
   ifd->oldvt[1] =  vt;
   ifd->oldvt[2] =  0.0;
   ifd->oldvt[3] =  0.0;

   /*-----------------------------*/
   /* initialize pitch axis model */
   /*-----------------------------*/
   ifd->oldp01[0] = 0.0;
   ifd->oldp01[1] = 0.0;
   ifd->oldp01[2] = 0.0;
   ifd->oldp01[3] = 0.0;           

   ifd->oldp02[0] = alpha;
   ifd->oldp02[1] = alpha;
   ifd->oldp02[2] = 0.0;
   ifd->oldp02[3] = 0.0;

   ifd->oldp03[0] = alpha;
   ifd->oldp03[1] = alpha;
   ifd->oldp03[2] = alpha;
   ifd->oldp03[3] = alpha;
   ifd->oldp03[4] = alpha;
   ifd->oldp03[5] = alpha;

   ifd->oldp04[0] = ifd->alpdot*RTD;
   ifd->oldp04[1] = ifd->alpdot*RTD;
   ifd->oldp04[2] = ifd->alpdot*RTD;
   ifd->oldp04[3] = alpha;
   ifd->oldp04[4] = alpha;
   ifd->oldp04[5] = alpha;

   ifd->oldp05[0] = 0.0;
   ifd->oldp05[1] = 0.0;
   ifd->oldp05[2] = 0.0;
   ifd->oldp05[3] = 0.0;

   /*---------------------------*/
   /* initialize yaw axis model */
   /*---------------------------*/
   ifd->oldy01[0] = 0.0;
   ifd->oldy01[1] = 0.0;
   ifd->oldy01[2] = 0.0;
   ifd->oldy01[3] = 0.0;         


   ifd->oldy02[0] = beta; 
   ifd->oldy02[1] = beta ;
   ifd->oldy02[2] = 0.0;
   ifd->oldy02[3] = 0.0;

   ifd->oldy03[0] = beta;
   ifd->oldy03[1] = beta;
   ifd->oldy03[2] = beta;
   ifd->oldy03[3] = beta;
   ifd->oldy03[4] = beta;
   ifd->oldy03[5] = beta;

   ifd->oldy04[0] = ifd->betdot*RTD;
   ifd->oldy04[1] = ifd->betdot*RTD;
   ifd->oldy04[2] = ifd->betdot*RTD;
   ifd->oldy04[3] = beta;
   ifd->oldy04[4] = beta;
   ifd->oldy04[5] = beta;

   ifd->oldy05[0] = 0.0;
   ifd->oldy05[1] = 0.0;
   ifd->oldy05[2] = 0.0;
   ifd->oldy05[3] = 0.0;

   //Initialize position
   ifd->oldx[0] = x;
   ifd->oldx[1] = x;
   ifd->oldx[2] = xdot; // 0.0; // MLR 1/5/2005 - 
   ifd->oldx[3] = xdot; // 0.0; // MLR 1/5/2005 - 

   ifd->oldy[0] = y;
   ifd->oldy[1] = y;
   ifd->oldy[2] = ydot; // 0.0; // MLR 1/5/2005 - 
   ifd->oldy[3] = ydot; // 0.0; // MLR 1/5/2005 - 

   ifd->oldz[0] = z;
   ifd->oldz[1] = z;
   ifd->oldz[2] = zdot; // 0.0; // MLR 1/5/2005 - 
   ifd->oldz[3] = zdot; // 0.0; // MLR 1/5/2005 - 
}
