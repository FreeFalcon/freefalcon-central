#include "stdhdr.h"
#include "missile.h"

void MissileClass::Pitch(void)
{
float aoacmd;               /* commanded Angle of Attack */
float error, error1;
float eintg1, eprop;
float eintg;
float alpdt1;
float nzcmd;

 	if (!ifd)
		return; // JB 010720

	 nzcmd = -ifd->augCommand.pitch;
//me123 added the /1.41 which is approimatly sqrt(2)
//total Gz^2 = yaw^2 +  pitch^2 so
// yaw max = gZ/sqrt(2)
// this is still not totaly true since it will be limited to less then max g only in the yaw direction, but better imo

   // Limit commands to +/- 40 Gs (except at endgame where anything goes)
   if (!(flags & EndGame))
		nzcmd = min ( max (nzcmd, -auxData->maxGNormal/1.41f), auxData->maxGNormal/1.41f);
    else
	nzcmd = min ( max (nzcmd, -auxData->maxGTerminal/1.41f), auxData->maxGTerminal/1.41f);
   error1 = nzcmd - ifd->nzcgb;

   /*-----------------------------*/
   /* nz load factor loop closure */
   /*-----------------------------*/
   error  = error1*ifd->kp05;
   eprop  = ifd->kp02*error;
   eintg1 = ifd->kp03*error;
   eintg  = Math.FITust(eintg1,SimLibMinorFrameTime,ifd->oldp02);

   /*-------------*/
   /* aoa limiter */
   /*-------------*/
   aoacmd = eprop + eintg;
   if(aoacmd >= inputData->aoamax)
   {
      ifd->oldp02[0] = inputData->aoamax;
      ifd->oldp02[1] = inputData->aoamax;
      ifd->oldp02[2] = 0.0;
      ifd->oldp02[3] = 0.0;
      aoacmd    = inputData->aoamax;
   }   
   else if(aoacmd <= inputData->aoamin)
   {
      ifd->oldp02[0] = inputData->aoamin;
      ifd->oldp02[1] = inputData->aoamin;
      ifd->oldp02[2] = 0.0;
      ifd->oldp02[3] = 0.0;
      aoacmd    = inputData->aoamin;
   }

   alpha  = Math.F7Tust(aoacmd,ifd->tp02,ifd->tp03,ifd->tp04, SimLibMinorFrameTime,ifd->oldp03,&ifd->oldalp);
   alpdt1 = Math.F8Tust(aoacmd,ifd->tp02,ifd->tp03,ifd->tp04, SimLibMinorFrameTime,ifd->oldp04,&ifd->oldalpdt);
   ifd->alpdot = alpdt1* DTR;
}
