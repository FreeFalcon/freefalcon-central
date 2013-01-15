/******************************************************************************/
/*                                                                            */
/*  Unit Name : gain.cpp                                                      */
/*                                                                            */
/*  Abstract  : Calculate the gains and parameters used by the axis systems   */
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
#include "aircrft.h"
#include "limiters.h"

/********************************************************************/
/*                                                                  */
/* Routine: void AirframeClass::Gains(void)                        */
/*                                                                  */
/* Description:                                                     */
/*    Calculate the paramenters for each axis, then do the          */
/*    feedback paramters need for the digi if needed.               */
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
extern bool g_bNewFm;
void AirframeClass::Gains(void)
{
float pcoef1, pcoef2, pradcl, pfreq1, pfreq2;
float ycoef1, ycoef2, yradcl, yfreq1, yfreq2;
float cosmuLim, cosphiLim;
float omegasp, omegasp1;
float psmax,nzalpha,ttheta2;
Limiter *limiter = NULL;
bool landingGains;

	if(vt == 0.0F)
		return;

   cosphiLim = max(0.0F,platform->platformAngles.cosphi);
   cosmuLim  = max(0.0F,platform->platformAngles.cosmu);

   //landingGains = gearPos != 0 || IsEngineFlag(FuelDoorOpen) || IsSet(Refueling);
   //TJL 10/20/03 Added TEFExtend. Per the F-16-1 ALT FLAPS sets Landing Gains
	landingGains = gearPos != 0 || IsEngineFlag(FuelDoorOpen) || IsSet(Refueling) || platform->TEFExtend;




   /*---------------------------------*/
   /* AOA bias for aoa command system */
   /*---------------------------------*/
   /*
   if (clalph0 == 0.0F || IsSet(Planted) )
	   aoabias = 0.0F;
   else
   {
	   aoabias = (GRAVITY * platform->platformAngles.cosgam *
					cosmuLim / qsom + 0.1F*gearPos - clift0 * (1.0F + tefFactor * 0.05F)) / clalph0 - tefFactor + lefFactor;
	   //aoabias = (GRAVITY * platform->platformAngles.costhe *
		//		cosphiLim / qsom + 0.1F*gearPos - clift0 * (1.0F + tefFactor * 0.05F)) / clalph0 - tefFactor + lefFactor;
	   
	   if(!IsSet(InAir))
	   {
		   float bleed = max(0.0F , min(aoabias*(vt - minVcas*KNOTS_TO_FTPSEC*0.5F)/(minVcas*KNOTS_TO_FTPSEC*0.25F), 1.0F));
		   aoabias = max(0.0F,min (bleed, aoamax));
	   }
	   else
		   aoabias = max(0.0F,min (aoabias, aoamax));
   }*/
   if(IsSet(InAir))
   {
	   if (clalph0 == 0.0F)
		   aoabias = 0.0F;
	   else 
	   {
#if 0 // JPO - think original is correct 
	       if (g_bNewFm)
				aoabias = (GRAVITY * platform->platformAngles.cosgam *
					cosmuLim / qsom + 0.1F*gearPos - clift0 * (1.0F + tefFactor * auxaeroData->CLtefFactor)) / clalph0 - tefFactor - lefFactor;
			 else
#endif
				aoabias = (GRAVITY * platform->platformAngles.cosgam *
					cosmuLim / qsom + 
					0.1F*gearPos - 
					clift0 * (1.0F + tefFactor * auxaeroData->CLtefFactor)) / 
					clalph0 - tefFactor + lefFactor;

			 if (g_bNewFm)
		   aoabias = max(0.0F,min (aoabias, aoamax/3));
			 else
		   aoabias = max(0.0F,min (aoabias, aoamax));
	   }
   }

   /*-------------------*/
   /* AOA or NZ command */
   /*-------------------*/
   gsAvail = aoamax * clalph * qsom / GRAVITY;
   if (IsSet(AutoCommand))
   {
	   limiter = gLimiterMgr->GetLimiter(CatIIICommandType,vehicleIndex);
	  if(IsSet(CATLimiterIII) && limiter)
	  {
		  if( alpha  < limiter->Limit(vcas) && (!gearPos || IsSet(GearBroken)) )
			ClearFlag(AOACmdMode);
		  else
			SetFlag(AOACmdMode);
	  }
	  else
	  {
		  limiter = gLimiterMgr->GetLimiter(CommandType,vehicleIndex);
		  if(limiter)
		  {
			  if (alpha < limiter->Limit(alpha) && (!gearPos || IsSet(GearBroken)) )
				 ClearFlag(AOACmdMode);
			  else
				 SetFlag(AOACmdMode);
		  }
		  else if (gsAvail > maxGs && (!gearPos || IsSet(GearBroken)) )
			 ClearFlag(AOACmdMode);
		  else
			 SetFlag(AOACmdMode);
	  }
   }
   else if (IsSet(GCommand))
      ClearFlag(AOACmdMode);
   else if (IsSet(AlphaCommand))
      SetFlag(AOACmdMode);

   /*---------------------------------------------------*/
   /* pitch rate transfer numerator time constant       */
   /*---------------------------------------------------*/
   nzalpha = clalph0*qsom*RTD/GRAVITY;
   ttheta2 = vt/(GRAVITY*nzalpha);
   ttheta2 = max (ttheta2, 0.1F);

   /*--------------------------------------*/
   /* pitch axis stick limiter and shaping */
   /*--------------------------------------*/
   pstick = max (-1.0F, min (1.0F, pstick));
   pshape = pstick*pstick;
   if (pstick < 0.0F)
        pshape *= -1.0F;

   /*----------------------------------------*/
   /* pitch axis gains and filter parameters */
   /*----------------------------------------*/
   tp01 = 0.200F;
   zp01 = 0.900F;

   if(!IsSet(Simplified) && simpleMode != SIMPLE_MODE_AF)
   {
	   //tp01 *= (1.0F + (loadingFraction - 1.3F) *0.1F);
		zp01 *=  (1.0F - 0.15F*(max(0.0F,1.0F - qbar/25.0F)) - zpdamp - max(0.0F,(loadingFraction - 1.3F) *0.01F) );
		zp01 = max(0.5F, zp01);
   }

   /*-----------------------------*/
   /* limit closed loop frequency */
   /*-----------------------------*/
   if (pshape > 0.0F)
      kp01 = maxGs - platform->platformAngles.costhe*cosphiLim;
   else
      kp01 = 4.0F + platform->platformAngles.costhe*cosphiLim;

   kp02 = 1.000F;
   kp03 = 2.000F;

   omegasp1 = 1.0F/(ttheta2 * 0.65F);
   omegasp1 = max (1.0F, omegasp1);
   omegasp = omegasp1 ; 

   if(stallMode > Recovering || !IsSet(InAir))
   {
		omegasp *= 2.0F;
   }
   else
   {
	   limiter = gLimiterMgr->GetLimiter(LowSpeedOmega,vehicleIndex);
		if(limiter)
			omegasp *= limiter->Limit(qbar);
   }

   wp01 = omegasp;

   /*----------------------------------------------*/
   /* calculate inner loop dynamics for pitch axis */
   /*----------------------------------------------*/
   pcoef1 =  tp01*wp01*wp01 -
             2.0F*zp01*wp01 - kp03;
   pcoef2 =  2.0F*zp01*wp01*kp03 -
             kp03*tp01*wp01*wp01;
   pradcl =  max((pcoef1*pcoef1 - 4.0F*pcoef2),0.0F);

   pfreq1 = ((float)sqrt(pradcl) - pcoef1) * 0.5F;
   pfreq2 = -pcoef1 - pfreq1;

   /*------------------------------------------*/
   /* time constants for pitch axis inner loop */
   /*------------------------------------------*/
   tp02   =  1/pfreq1;
   tp03   =  1/pfreq2;

   tp03   = max (tp03, 0.5F);

   if (IsSet(AOACmdMode) || !(qsom*cnalpha))
      kp05 = tp02*tp03*wp01*wp01;
   else
      kp05 = GRAVITY*tp02*tp03*wp01*wp01 /
                     (qsom*cnalpha);

   if(landingGains)
	   kp05 *= auxaeroData->pitchGearGain;

   if(!IsSet(InAir))
   {
		kp05 *= max(0.0f, min(1.0F, (qbar - 20.0F)/45.0F ) );
   }

   F4Assert (!_isnan(kp05));

   /*---------------------------------------*/
   /* roll axis gains and filter parameters */
   /*---------------------------------------*/
   if(qbar >= 250.0F)
      tr01 =  0.25F;
   else
      tr01 = -0.001111F*(qbar - 100.0F) + 0.416F;

   /*-----------------------------------------------*/
   /* roll command gain and stick limiter / shaping */
   /*-----------------------------------------------*/
   rstick = min ( max (rstick, -1.0F), 1.0F);
   rshape = rstick*rstick;
   if (rstick < 0.0F)
        rshape *= -1.0F;

   psmax = Math.TwodInterp (alpha, qbar, rollCmd->alpha, rollCmd->qbar, 
                     rollCmd->roll, rollCmd->numAlpha, rollCmd->numQbar,
                     &curRollAlphaBreak, &curRollQbarBreak);
   kr01  = psmax*DTR;

   if(landingGains)
	   kr01 *= auxaeroData->rollGearGain;

   kr02  = platform->platformAngles.cosalp;

   /*--------------------------------------*/
   /* yaw axis gains and filter parameters */
   /*--------------------------------------*/
   //zy01 = 0.50F;
   zy01 = 0.70F;
   
   //wy01 = (0.8F/tr01);
   wy01 = (0.3F/tr01);

   if(!IsSet(Simplified) && simpleMode != SIMPLE_MODE_AF)
	   wy01 *= (1.0F - loadingFraction * 0.1F);

   ky01 = 1.000F;
   ky02 = 1.000F;
   ky03 = 2.000F;

   /*--------------------------------------------*/
   /* calculate inner loop dynamics for yaw axis */
   /*--------------------------------------------*/
   ycoef1 = -2.0F*zy01*wy01 - ky03;
   ycoef2 =  2.0F*zy01*wy01*ky03;
   yradcl =  ycoef1*ycoef1 - 4.0F*ycoef2;
   if( yradcl < 0.0F ) yradcl = 0.0F;

   yfreq1 = ((float)sqrt(yradcl) - ycoef1)*0.5F;
   yfreq2 = -ycoef1 - yfreq1;

   /*----------------------------------------*/
   /* time constants for yaw axis inner loop */
   /*----------------------------------------*/
   ty01   =  1/yfreq1;
   ty02   =  1/yfreq2;

   if (cy != 0.0F)
   {
      ky05   = -GRAVITY*wy01*wy01 / (qsom*cy*yfreq1*yfreq2);
   }

   /*------------------------------------*/
   /* yaw axis pedal limiter and shaping */
   /*------------------------------------*/
   ypedal = min ( max (ypedal, -1.0F), 1.0F);
   yshape = ypedal * ypedal;
   if (ypedal < 0.0F)
        yshape *= -1.0F;

   if(landingGains)
	   ky05 *= auxaeroData->yawGearGain;
}
