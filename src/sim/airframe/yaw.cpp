/******************************************************************************/
/*                                                                            */
/*  Unit Name : yaw.cpp                                                       */
/*                                                                            */
/*  Abstract  : Yaw axis control path                                         */
/*                                                                            */
/*  Dependencies : Auto-Generated                                             */
/*                                                                            */
/*  Operating System : MS-DOS 6.2,                                            */
/*                                                                            */
/*  Compiler : WATCOM C/C++ V10                                               */
/*                                                                            */
/*  Naming Conventions :                                                      */
/*                                                                            */
/*      Public Functions    : Mixed case with no underscores                  */
/*      Private Functions   : Mixed case with no underscores                  */
/*      Public Functions    : Mixed case with no underscores                  */
/*      Global Variables    : Mixed case with no underscores                  */
/*      Classless Functions : Mixed case with no underscores                  */
/*      Classes             : All upper case seperated by an underscore       */
/*      Defined Constants   : All upper case seperated by an underscore       */
/*      Macros              : All upper case seperated by an underscore       */
/*      Structs/Types       : All upper case seperated by an underscore       */
/*      Private Variables   : All lower case seperated by an underscore       */
/*      Public Variables    : All lower case seperated by an underscore       */
/*      Local Variables     : All lower case seperated by an underscore       */
/*                                                                            */
/*  Development History :                                                     */
/*  Date      Programer           Description                                 */
/*----------------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                               */
/*                                                                            */
/******************************************************************************/
#include "stdhdr.h"
#include "airframe.h"
#include "Simbase.h"
#include "limiters.h"
#include "aircrft.h"
#include "simdrive.h"//TJL 01/14/04
#include "fmath.h" //TJL 01/24/04

/********************************************************************/
/*                                                                  */
/* Routine: AirframeClass::Yaw(void)                               */
/*                                                                  */
/* Description:                                                     */
/*    Loop closure for aircraft yaw axis.  Uses beta and Ny         */
/*    feedback to close the ruder loop.                             */
/*                                                                  */
/* Inputs:                                                          */
/*    None                                                          */
/*                                                                  */
/* Outputs:                                                         */
/*    None                                                          */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
void AirframeClass::Yaw(void)
{
float error, error1, eprop, eintg, eintg1;
//float betdt1,  beta1, lastBeta;
float betcmd, nycmd, gsAvail, alphaError;
Limiter *limiter = NULL;

	//if( IsSet(Planted) || (IsSet(NoseSteerOn) && !(gear[0].flags & GearData::GearStuck)) )
	if(!IsSet(InAir))
		return;

	if(platform->IsF16())
		alphaError = (float)(fabs(q) + fabs(p)*0.5F)*0.4F*RTD;
	else
		alphaError = 0.0F;

   /*--------------*/
   /* command path */
   /*--------------*/

   nycmd = yshape*2.0F;

   nycmd = min ( max ( nycmd, -2.0F), 2.0F);
   gsAvail = betmax * 0.05F * qsom / GRAVITY;

   nycmd *= min (gsAvail/2.0F, 1.0F);

   if(IsSet(CATLimiterIII))
   {
	   limiter = gLimiterMgr->GetLimiter(CatIIIYawAlphaLimiter,vehicleIndex);
		if(limiter)
			nycmd *= limiter->Limit(alpha - alphaError);

		limiter = gLimiterMgr->GetLimiter(CatIIIYawRollRateLimiter,vehicleIndex);
	   if(limiter)
			nycmd *= limiter->Limit(p*RTD);
   }
   else
   {
	   limiter = gLimiterMgr->GetLimiter(YawAlphaLimiter,vehicleIndex);
	   if(limiter)
			nycmd *= limiter->Limit(alpha - alphaError);

	   limiter = gLimiterMgr->GetLimiter(YawRollRateLimiter,vehicleIndex);
	   if(limiter)
			nycmd *= limiter->Limit(p*RTD);
   }
   
   limiter = gLimiterMgr->GetLimiter(CatIIICommandType,vehicleIndex);
   if( fabs(yshape) >= 0.1F && (limiter) )
	   nycmd *= (5.0F/limiter->Limit(vcas*0.8F));

   limiter = gLimiterMgr->GetLimiter(PitchYawControlDamper,vehicleIndex);
   if(limiter)
	   nycmd *= limiter->Limit((350.0F - vcas)*0.2F);
   //error1 = nycmd + nycgb;
   //if(platform->isF16)
//		error1 = nycmd + (nycgs + platform->platformAngles.sinmu * platform->platformAngles.cosgam);
  // else
   if(IsSet(InAir))
		error1 = nycmd + nycgs;
   else
		error1 = (nycmd + nycgs)*0.8F;
   error  = error1*ky05;
   eprop  = ky02*error;
   eintg1 = ky03*error;
   
   //eintg  = Math.FITust(eintg1,SimLibMinorFrameTime,oldy01);
   eintg  = Math.FIAdamsBash(eintg1,SimLibMinorFrameTime,oldy01);



   /*--------------*/
   /* beta limiter */
   /*--------------*/
   if (eintg > betmax)
   {
      oldy01[0]    = betmax;
      oldy01[1]    = betmax;
      oldy01[2]    = 0.0;
      oldy01[3]    = 0.0;
   }

   if (eintg < betmin)
   {
      oldy01[0]    = betmin;
      oldy01[1]    = betmin;
      oldy01[2]    = 0.0;
      oldy01[3]    = 0.0;
   }

   betcmd = max(min(eprop + eintg,betmax),betmin);
   betcmd *= ylsdamp;

   //TJL 01/14/03 Multi-engine asymmetric thrust yaw
   if (auxaeroData->nEngines == 2)
   {
	   float asymmYaw = 0.0f;
	   float asymmYpedal = 0.0f;
	   float engine1 = thrust1;
	   float engine2 = thrust2;
	   if (engine1 < 0.0f)
		   engine1 = 0.0f;
	   if (engine2 < 0.0f)
		   engine2 = 0.0f;
	   //asymmYaw = (float)Abs(thrust1 - thrust2);
	
		asymmYaw = (engine2 - engine1)*1.5f;
		betcmd = (betcmd + asymmYaw);
		asymmYpedal = (engine2 - engine1)*0.08f;

		if ((float)fabs(asymmYpedal + ypedal) <= 1.0f)
			{
				ypedal = asymmYpedal + ypedal;
				yshape = ypedal * ypedal; 
			}

   }


   YawIt(betcmd, SimLibMinorFrameTime);

   if(!IsSet(InAir))
   {
	   oldy03[0] *= 0.8F;
	   oldy03[1] *= 0.8F;
	   oldy03[2] *= 0.8F;
	   oldy03[3] *= 0.8F;
	   beta *= 0.8F;
   }

}

void AirframeClass::YawIt(float betcmd, float dt)
{
	// JB 010714 mult by the momentum
	beta   = Math.FLTust(betcmd ,ty02 * auxaeroData->yawMomentum,dt,oldy03);

	if(beta < -180.0F)
	{
		oldy03[0] += 360.0F;
		oldy03[1] += 360.0F;
		oldy03[2] += 360.0F;
		oldy03[3] += 360.0F;
	}
	else if(beta > 180.0F)
	{
		oldy03[0] -= 360.0F;
		oldy03[1] -= 360.0F;
		oldy03[2] -= 360.0F;
		oldy03[3] -= 360.0F;
	}


	ShiAssert(!_isnan(beta));
}