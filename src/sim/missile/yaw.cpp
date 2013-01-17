#include "stdhdr.h"
#include "missile.h"

void MissileClass::Yaw(void)
{
float eintg,eprop,error, error1,eintg1,betdt1,betcmd;
float nycmd;

 	if (!ifd)
		return; // JB 010720
//me123 added the /1.41 which is approimatly sqrt(2)
//total Gz^2 = yaw^2 +  pitch^2 so
// yaw max = gZ/sqrt(2)
// this is still not totaly true since it will be limited to less then max g only in the yaw direction, but better imo
   nycmd = -ifd->augCommand.yaw;
    if (!(flags & EndGame))
	nycmd = min ( max (nycmd, -auxData->maxGNormal/1.41f), auxData->maxGNormal/1.41f);
    else
	nycmd = min ( max (nycmd, -auxData->maxGTerminal/1.41f), auxData->maxGTerminal/1.41f);

   /*-----------------------------*/
   /* ny load factor loop closure */
   /*-----------------------------*/
   error1 = nycmd + ifd->nycgb;
   error  = error1*ifd->ky05;
   eprop  = ifd->ky02*error;
   eintg1 = ifd->ky03*error;
   eintg  = Math.FITust(eintg1,SimLibMinorFrameTime,ifd->oldy02);

   /*--------------*/
   /* beta limiter */
   /*--------------*/
   betcmd = eprop + eintg;
   if(betcmd >= inputData->betmax)
   {
      ifd->oldy02[0] = inputData->betmax;
      ifd->oldy02[1] = inputData->betmax;
      ifd->oldy02[2] = 0.0;
      ifd->oldy02[3] = 0.0;
      betcmd     = inputData->betmax;
   }   

   if(betcmd <= inputData->betmin)
   {
      ifd->oldy02[0] = inputData->betmin;
      ifd->oldy02[1] = inputData->betmin;
      ifd->oldy02[2] =  0.0;
      ifd->oldy02[3] =  0.0;
      betcmd     = inputData->betmin;
   }

   beta   = Math.F7Tust(betcmd,ifd->ty02,ifd->ty03,ifd->ty04,SimLibMinorFrameTime,ifd->oldy03,&ifd->oldbet);
   betdt1 = Math.F8Tust(betcmd,ifd->ty02,ifd->ty03,ifd->ty04,SimLibMinorFrameTime,ifd->oldy04,&ifd->oldbetdt);
   ifd->betdot = betdt1*DTR;
}
