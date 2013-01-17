#include "stdhdr.h"
#include "missile.h"
#include "simmath.h"

void MissileClass::EquationsOfMotion(void)
{
float e1temp;         /* Temporary quaternions */
float e2temp;         /* before normalization */
float e3temp;
float e4temp;
float xwind;         /* x velocity, wind axis */
float e1dot, e2dot, e3dot, e4dot, enorm;

	if (!ifd)
		return; // JB 010720

   /*----------------------------------*/
   /* body axis roll rate and yaw rate */
   /*----------------------------------*/
   ifd->rstab  = (ifd->nycgw+ifd->geomData.cosgam*ifd->geomData.sinmu)*GRAVITY/vt-ifd->betdot;
   p =  ifd->rstab*ifd->geomData.sinalp;
   r =  ifd->rstab*ifd->geomData.cosalp;

   /*----------------------*/
   /* body axis pitch rate */
   /*----------------------*/
   q = (ifd->nzcgw - ifd->geomData.cosmu*ifd->geomData.cosgam)*GRAVITY/
         (vt*ifd->geomData.cosbet) + ifd->alpdot +
         (p*ifd->geomData.cosalp + r*ifd->geomData.sinalp)*ifd->geomData.tanbet;

   /*-----------------------------------*/
   /* quaternion differential equations */
   /*-----------------------------------*/
   e1dot = (-ifd->e4*p - ifd->e3*q - ifd->e2*r)*0.5F;
   e2dot = (-ifd->e3*p + ifd->e4*q + ifd->e1*r)*0.5F;
   e3dot = ( ifd->e2*p + ifd->e1*q - ifd->e4*r)*0.5F;
   e4dot = ( ifd->e1*p - ifd->e2*q + ifd->e3*r)*0.5F;

   /*-----------------------*/
   /* integrate quaternions */
   /*-----------------------*/
   e1temp = Math.FITust(e1dot,SimLibMinorFrameTime,ifd->olde1);
   e2temp = Math.FITust(e2dot,SimLibMinorFrameTime,ifd->olde2);
   e3temp = Math.FITust(e3dot,SimLibMinorFrameTime,ifd->olde3);
   e4temp = Math.FITust(e4dot,SimLibMinorFrameTime,ifd->olde4);

   /*--------------------------*/
   /* quaternion normalization */
   /*--------------------------*/
   enorm = (float)sqrt(e1temp*e1temp+e2temp*e2temp+e3temp*e3temp+e4temp*e4temp);
   ifd->e1    = e1temp/enorm;
   ifd->e2    = e2temp/enorm;
   ifd->e3    = e3temp/enorm;
   ifd->e4    = e4temp/enorm;

   /*------------------------------*/
   /* reset quaternion integrators */ 
   /*------------------------------*/
   ifd->olde1[0] = ifd->e1;
   ifd->olde2[0] = ifd->e2;
   ifd->olde3[0] = ifd->e3;
   ifd->olde4[0] = ifd->e4;

   /*-------------------*/
   /* direction cosines */
   /*-------------------*/
   dmx[0][0] = ifd->e1*ifd->e1 - ifd->e2*ifd->e2 - ifd->e3*ifd->e3 + ifd->e4*ifd->e4;
   dmx[0][1] = 2*(ifd->e3*ifd->e4 + ifd->e1*ifd->e2);
   dmx[0][2] = 2*(ifd->e2*ifd->e4 - ifd->e1*ifd->e3); 

   dmx[1][0] = 2*(ifd->e3*ifd->e4 - ifd->e1*ifd->e2);
   dmx[1][1] = ifd->e1*ifd->e1 - ifd->e2*ifd->e2 + ifd->e3*ifd->e3 - ifd->e4*ifd->e4;
   dmx[1][2] = 2*(ifd->e2*ifd->e3 + ifd->e4*ifd->e1);

   dmx[2][0] = 2*(ifd->e1*ifd->e3 + ifd->e2*ifd->e4);
   dmx[2][1] = 2*(ifd->e2*ifd->e3 - ifd->e1*ifd->e4);
   dmx[2][2] = ifd->e1*ifd->e1 + ifd->e2*ifd->e2 - ifd->e3*ifd->e3 - ifd->e4*ifd->e4; 

   /*--------------*/
   /* euler angles */
   /*--------------*/
   if(dmx[0][2] >  1.0)dmx[0][2] =  1.0F;
   if(dmx[0][2] < -1.0)dmx[0][2] = -1.0F;

   psi   =  (float)atan2(dmx[0][1],dmx[0][0]);

   theta = -(float)atan2(dmx[0][2],(float)sqrt(1.0f-dmx[0][2]*dmx[0][2]));
   phi   =  (float)atan2(dmx[1][2],dmx[2][2]);

   Trigenometry();

   /*-------------------*/
   /* velocity equation */
   /*-------------------*/
   xwind = ifd->xwaero + ifd->xwprop;

   vtdot = xwind - GRAVITY*ifd->geomData.singam;
   vt    = Math.FITust(vtdot,SimLibMinorFrameTime,ifd->oldvt);

   /*-------------------*/
   /* inertial velocity */
   /*-------------------*/
   xdot =  vt*ifd->geomData.cosgam*ifd->geomData.cossig;
   ydot =  vt*ifd->geomData.cosgam*ifd->geomData.sinsig;
   zdot = -vt*ifd->geomData.singam;

   /*-----------------*/
   /* inertial coords */
   /*-----------------*/
   if (done != FalconMissileEndMessage::GroundImpact)
   {
      x   = Math.FITust(xdot,SimLibMinorFrameTime,ifd->oldx);
      y   = Math.FITust(ydot,SimLibMinorFrameTime,ifd->oldy);
      z   = Math.FITust(zdot,SimLibMinorFrameTime,ifd->oldz);
      alt = -z;
   }
}
