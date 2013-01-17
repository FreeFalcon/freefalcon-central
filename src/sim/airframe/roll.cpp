/******************************************************************************/
/*                                                                            */
/*  Unit Name : roll.cpp                                                      */
/*                                                                            */
/*  Abstract  : Roll axis control system                                      */
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
#include "debuggr.h"
#include "Simbase.h"
#include "limiters.h"
#include "aircrft.h"
#include "arfrmdat.h"
//TJL 12/11/03
#include "sms.h"
#include "entity.h"
#include "initdata.h"
#include "hardpnt.h"
#include "digi.h"
#include "fmath.h"

extern AeroDataSet *aeroDataset;

/********************************************************************/
/*                                                                  */
/* Routine: void AirframeClass::Roll(void)                         */
/*                                                                  */
/* Description:                                                     */
/*    Calculate roll rate based on max available rate and user      */
/*    intput.                                                       */
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
//TJL 01/06/04
extern bool g_bRollInertia;


void AirframeClass::Roll(void)
{
float pscmd, alphaError;
Limiter *limiter = NULL;
	
	if(IsSet(Planted))
		return;
	
	alphaError = 0.0F;

   /*--------------*/
   /* command path */
   /*--------------*/
   pscmd  = Math.Limit((rshape*kr01),-kr01,kr01);
	pscmd = min ( max (pscmd, -kr01), kr01);

	if( ((AircraftClass *)platform)->IsF16() )
	{
		alphaError = (float)(fabs(q*0.5F) + fabs(p))*0.1F*RTD;

		limiter = gLimiterMgr->GetLimiter(RollRateLimiter,vehicleIndex);
		if(limiter)
			pscmd *= limiter->Limit(alpha - alphaError);

		float speedswitch = 250.0f;
		
		if (g_bNewFm)
			speedswitch = 220.0f;

		if(vcas < speedswitch)
			pscmd *= (vcas/speedswitch);

		if(gearPos && g_bNewFm)
			pscmd *= 0.6f;
		else if (gearPos)
		   pscmd *= 0.5f;

		if(IsSet(CATLimiterIII))
		{
			limiter = gLimiterMgr->GetLimiter(CatIIIRollRateLimiter,vehicleIndex);
			if(limiter)
				pscmd = limiter->Limit(pscmd);
		}

		if(fabs(pscmd) < 0.001)
			pscmd = -p;
	}
	else 
	{
		limiter = gLimiterMgr->GetLimiter(RollRateLimiter,vehicleIndex);
		if(limiter)
			pscmd *= limiter->Limit(alpha);
	}

	
	if(!IsSet(Simplified))
	{		
		switch(stallMode)
		{
		case None:
			if(assymetry * platform->platformAngles.cosmu > 0.0F)
				pscmd +=  max((assymetry/weight)* 0.04F, 0.0F)*(nzcgs - 1.0F);
			else
				pscmd +=  min((assymetry/weight)* 0.04F, 0.0F)*(nzcgs - 1.0F);
			break;

		case DeepStall:
			if(platform->platformAngles.cosphi > 0.0F)
				pscmd = platform->platformAngles.sinphi*-5.0F*DTR;
			else
				pscmd = platform->platformAngles.sinphi*5.0F*DTR;

			pscmd += (oscillationTimer * 40.0F*max(0.0F,(0.4F - (float)fabs(r))*2.5F)*DTR*(max(0.0F,loadingFraction - 1.3F)) );
			break;

		case EnteringDeepStall:
			if(platform->platformAngles.cosphi > 0.0F)
				pscmd = platform->platformAngles.sinphi*-40.0F*DTR + rshape*kr01 *0.1F;
			else
				pscmd = platform->platformAngles.sinphi*40.0F*DTR + rshape*kr01 *0.1F;
			break;
		case Spinning:
			pscmd = oscillationTimer*DTR;
			break;
		case FlatSpin:
			pscmd = platform->platformAngles.sinphi*5.0F*DTR;
			break;
		}

	}

	//TJL 01/14/03 Multi-engine asymmetric thrust roll
   if (auxaeroData->nEngines == 2 && IsSet(InAir))
   {
	   float asymmRoll = 0.0f;
	   float engine1 = thrust1;
	   float engine2 = thrust2;
	   if (engine1 < 0.0f)
		   engine1 = 0.0f;
	   if (engine2 < 0.0f)
		   engine2 = 0.0f;

	  // asymmRoll = (float)Abs(thrust1 - thrust2);

	   //if (asymmRoll <= asymmRoll/2)
			//asymmRoll = (thrust1 - thrust2)*0.025f;
	   //else
	   asymmRoll = (engine1 - engine2)*0.05f;
	   
	   pscmd = (pscmd + asymmRoll);

   }

   /*---------------------------------*/
   /* closed loop roll response model */
   /*---------------------------------*/

   RollIt(pscmd, SimLibMinorFrameTime);
}

void AirframeClass::RollIt(float pscmd, float dt)
{
	//TJL 12/11/03
	float inertia = 0.0f;
	float addInertia;
   // Limit the command w/ roll limit
   if (maxRoll < aeroDataset[vehicleIndex].inputData[8])
   {
      if (phi > maxRoll)
      {
         pscmd = (maxRoll - phi) * kr01;
      }
      else if (phi < -maxRoll)
      {
         pscmd = (maxRoll - phi) * kr01;
      }

      // Scale cmd based on max delta;
      if (maxRollDelta == 0.0F)
         pscmd = 0.0F;
      else
         pscmd *= 1.0F - startRoll/maxRollDelta;
   }

   //TJL 01/06/04 Make Roll Inertia a config item
   if (g_bRollInertia)
   {

	//TJL 12/11/03 Call RollInertia
	RollInertia(inertia);
	addInertia = RollInertia(inertia);
	pstab  = Math.FLTust(pscmd,tr01 * (auxaeroData->rollMomentum + addInertia),dt,oldr01);
   }

   else
   {
	// JB 010714 mult by the momentum
	pstab  = Math.FLTust(pscmd,tr01 * auxaeroData->rollMomentum,dt,oldr01);
   }

}

float AirframeClass::GetMaxCurrentRollRate()
{
	float maxcurrentrollrate = Math.TwodInterp (alpha, qbar, rollCmd->alpha, rollCmd->qbar, 
		rollCmd->roll, rollCmd->numAlpha, rollCmd->numQbar,
		&curRollAlphaBreak, &curRollQbarBreak);

	return maxcurrentrollrate;
}

//TJL 12/11/03 Adding Roll Inertia function
float AirframeClass::RollInertia(float inertia)
{
	//AircraftClass* self;
	//SMSClass* Sms;
	int station = 0;
	int i;
	int hasAGMissile = 0;
    int hasBomb = 0;
    int hasHARM = 0;
    int hasGun = 0;
    int hasCamera = 0;
    int hasRocket = 0;
    int hasGBU = 0;
	int hasSamWpn = 0;
	int haswcTank = 0;
	int wcTankCount = 0;
	float inertiaTank = 0.0f;


	for (i=0; i<platform->Sms->NumHardpoints(); i++)
	{
      // Check for various stores
      if (platform->Sms->hardPoint[i]->weaponPointer && platform->Sms->hardPoint[i]->GetWeaponClass() == wcAgmWpn)
      {
         hasAGMissile += 1;
      }
      else if (platform->Sms->hardPoint[i]->weaponPointer && platform->Sms->hardPoint[i]->GetWeaponClass() == wcHARMWpn)
      {
         hasHARM +=1;
      }
      else if (platform->Sms->hardPoint[i]->weaponPointer && platform->Sms->hardPoint[i]->GetWeaponClass() == wcBombWpn)
      {
		 hasBomb +=1;
	  }

      else if (platform->Sms->hardPoint[i]->weaponPointer && platform->Sms->hardPoint[i]->GetWeaponClass() == wcGbuWpn)
      {
         hasGBU +=1;
      }
      else if (platform->Sms->hardPoint[i]->weaponPointer && platform->Sms->hardPoint[i]->GetWeaponClass() == wcRocketWpn)
      {
         hasRocket +=1;
      }
      else if (platform->Sms->hardPoint[i]->weaponPointer && platform->Sms->hardPoint[i]->GetWeaponClass() == wcCamera)
      {
         hasCamera +=1;
      }
	  else if (platform->Sms->hardPoint[i]->weaponPointer && platform->Sms->hardPoint[i]->GetWeaponClass() == wcSamWpn)
      {
         hasSamWpn +=1;
      }
	  else if (platform->Sms->hardPoint[i]->weaponPointer && platform->Sms->hardPoint[i]->GetWeaponClass() == wcTank)
      {
         haswcTank +=1;
		 //Add this so we can divide the inertia by the number of tanks 
		 //since externalFuel is one value and fuel isn't recorded per tank
		 wcTankCount = haswcTank;
      }

   }

//TJL 12/12/03 We just counted the loadout, now assign inertia score based on loadout
	while (hasAGMissile > 0)
	{
		inertia += 0.2f;
		hasAGMissile --;
	}
		
	while (hasHARM > 0)
	{
		inertia += 0.1f;
		hasHARM--;
	}
		
	while (hasBomb > 0)
	{
		inertia += 0.3f;
		hasBomb--;
	}
		
	while (hasGBU > 0)
	{
		inertia += 0.3f;
		hasGBU--;
	}
	
	while (hasRocket > 0)
	{
		inertia += 0.1f;
		hasRocket--;
	}

	while (hasCamera > 0)
	{
		inertia += 0.1f;
		hasCamera--;
	}

	while (hasSamWpn > 0)
	{
		inertia += 0.1f;
		hasSamWpn--;
	}

	while (haswcTank > 0 && externalFuel > 4000)
	{
		inertiaTank += 0.5f;
		haswcTank--;
	}
	
	while (haswcTank > 0 && (externalFuel <= 4000 && externalFuel > 2000))
	{
		inertiaTank += 0.4f;
		haswcTank--;
	}

	while (haswcTank > 0 && (externalFuel <= 2000 && externalFuel > 500)  )
	{
		inertiaTank += 0.3f;
		haswcTank--;
	}

 	while (haswcTank > 0 && (externalFuel <= 500)  )
	{
		inertiaTank += 0.2f;
		haswcTank--;
	}

	// Now reduce inertia by number of fuel tanks since external Fuel is one value
	if (wcTankCount == 4)
	{
		inertiaTank = inertiaTank * 0.25f;
		inertia = inertia + inertiaTank;
	}

	else if (wcTankCount == 3)
	{
		inertiaTank = inertiaTank * 0.33f;
		inertia = inertia + inertiaTank;
	}

	else if (wcTankCount == 2)
	{
		inertiaTank = inertiaTank * 0.5f;
		inertia = inertia + inertiaTank;
	}
	else if (wcTankCount == 1)
	{
		inertia = inertia + inertiaTank;
	}


	return inertia;
}