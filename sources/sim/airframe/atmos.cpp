/******************************************************************************/
/*                                                                            */
/*  Unit Name : atmos.cpp                                                     */
/*                                                                            */
/*  Abstract  : Calculates atmosphere at the current altitude                 */
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
#include "simdrive.h"
#include "ffeedbk.h"
#include "Graphics/Include/drawsgmt.h"


static float lastqBar = 0;  // Note: This limits us to 1 ownship/Force feedback stick per machine
static const float tropoAlt = 36089.0F, tropoAlt2 = 65617;
extern bool g_bFFCenterFix;

/********************************************************************/
/*                                                                  */
/* Routine: void AirframeClass::Atmosphere(void)                   */
/*                                                                  */
/* Description:                                                     */
/*    Calculates current state included pressure, mach, qbar and    */
/*    qsom for normalizing.                                         */
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
void AirframeClass::Atmosphere(void)
{
	float ttheta, rsigma;
	float pdelta, sound;
	float qpasl1, oper, pa, qc;
	pdelta = CalcPressureRatio(-z, &ttheta, &rsigma);
	
	sound	= (float)sqrt(ttheta) * AASL;
	rho		= rsigma * RHOASL;
	pa      = pdelta * PASL;
	
	if (IsSet(Trimming))
	{
		if (mach > 3.0)
			//vt = CalcMach(mach, pdelta) * sound;
			vt = mach * KNOTS_TO_FTPSEC;
		else
			vt = mach * sound;
	}
	
	/*----------------------------*/
	/* calculate dynamic pressure */
	/*----------------------------*/

	mach = vt / ((float)sqrt(ttheta) * AASL);//ME123 CALCULATE A TRUE MACH...NOT JUST VT/SPEEDOFSOUND
	
	if (fabs(vt) > 1.0F)
	{
		qbar   = 0.5F * rho * vt * vt;
		
		/*-------------------------------*/
		/* calculate calibrated airspeed */
		/*-------------------------------*/
		if (mach <= 1.0F)
			qc = ((float)pow((1.0F + 0.2F*mach*mach), 3.5F) - 1.0F)*pa;
		else
			qc = ((166.9F*mach*mach)/(float)(pow((7.0F - 1.0F/(mach*mach)), 2.5F)) - 1.0F)*pa;
		
		qpasl1 = qc/PASL + 1.0F;
		vcas = 1479.12F*(float)sqrt(pow(qpasl1, 0.285714F) - 1.0F);
		
		if (qc > 1889.64F)
		{
			oper = qpasl1 * (float)pow((7.0F - AASLK*AASLK/(vcas*vcas)), 2.5F);
			if (oper < 0.0F) oper = 0.1F;
			vcas = 51.1987F*(float)sqrt(oper);
		}
		
		/*------------------------*/
		/* normalizing parameters */
		/*------------------------*/
		qsom   = qbar*area/mass;
		qovt   = qbar/vt;
	}
	else
	{
		vcas = max (vt * FTPSEC_TO_KNOTS, 0.001F);
		qsom = 0.1F;
		qovt = 0.1F;
		qbar = 0.001F;
		mach = 0.001F;
	}

	//Wombat778 12-02-2003 Removed 
/*	
   if (platform == SimDriver.GetPlayerEntity())
   {
      if (g_bFFCenterFix) JoystickPlayEffect (JoyAutoCenter, 20000);		//Wombat778 11-26-2003 Changed method of FFCenterfix.  Added because of some reports that centering cuts out after an effect is played
	   {
         if (qbar < 250)
         {
            if (fabs (qbar - lastqBar) > 5.0F)
            {
			   if (!g_bFFCenterFix)				//Wombat778 9-29-2003  Allows user to have the fixed centering force for FF sticks
				   JoystickPlayEffect (JoyAutoCenter, FloatToInt32((qbar/250.0F * 0.5F + 0.5F) * 10000.0F));
               
               //lastqBar = qbar; // JB 010301 FF would cut out < 250
            }
						lastqBar = qbar; // JB 010301 FF would cut out < 250
         }
         else
         {
            if (fabs (qbar - lastqBar) > 5.0F)
				if (!g_bFFCenterFix)				//Wombat778 9-29-2003  Allows user to have the fixed centering force for FF sticks
					JoystickPlayEffect (JoyAutoCenter, 10000);
            lastqBar = qbar;
         }
      }
   }*/
	
	//Wombat778 12-02-2003 redid this section to be tighter.  Also, this should fix centering properly. g_bFFCenterFix now means that a constant instead of variable FF force is used      	

	if (platform == SimDriver.GetPlayerAircraft()){	  
	   if ( (fabs (qbar - lastqBar) > 5.0F) || !lastqBar ){
		   if ( (qbar > 250) || g_bFFCenterFix ){
			   JoystickPlayEffect (JoyAutoCenter, 10000);
		   }
		   else {
			   JoystickPlayEffect (JoyAutoCenter, FloatToInt32((qbar/250.0F * 0.5F + 0.5F) * 10000.0F));
		   }
		   lastqBar = qbar; 
	   }
	  
    }
}

float AirframeClass::CalcTASfromCAS(float cas)
{
	float ttheta, rsigma;
	float sound, pdelta, desMach;

	pdelta = CalcPressureRatio(-z, &ttheta, &rsigma);
	desMach = CalcMach(cas, pdelta);

	sound	= (float)sqrt(ttheta) * AASL;

	return desMach*sound*3600.0F/6080.0F;
}


float AirframeClass::CalcMach (float GetKias, float press_ratio)
{
	float a0 = 661.4785F;
	float kiasa, qcp0, qcpa;
	float u, fu, fpu;
	
	kiasa = GetKias / a0;
	qcp0  = (float)pow ((1.0 + 0.2 * kiasa * kiasa), 3.5) - 1.0F;
	if (kiasa >= 1.0) 
		qcp0 = 166.921F*(float)pow(kiasa,7.0F) / (float)pow((7.0F*kiasa*kiasa - 1.0F),2.5F) - 1.0F;
	
	qcpa = qcp0 / press_ratio;
	
	if (qcpa >= 0.892929F)
	{
		u = 2.0F;
		do
		{
			fu = 166.921F * (float)pow (u,7.0F) / (float)pow((7.0F*u*u - 1.0F), 2.5F) - (1.0F + qcpa);
			fpu = 7.0F*166.921F*(float)pow(u,6.0F)*(2.0F*u*u-1.0F)/(float)pow((7.0F*u*u-1.0F), 3.5F);
			u -= fu/fpu;
		}
		while (fabs(fu) > 0.001F);
		return (u);
	}
	else
	{
		return ((float)sqrt((pow((qcpa + 1.0F), (1.0F/3.5F)) - 1.0F) / 0.2F));
	}
}


float AirframeClass::CalcPressureRatio(float alt, float* ttheta, float* rsigma) {
	
	/*-----------------------------------------------*/
	/* calculate temperature ratio and density ratio */
	/*-----------------------------------------------*/
	if (alt <= tropoAlt)
	{
		*ttheta = 1.0F - 0.000006875F * alt;
		*rsigma = (float)pow (*ttheta, 4.255876F);
	}
	else if (alt < tropoAlt2)
	{
		*ttheta = 0.751865F;
		*rsigma = 0.297076F * (float)exp(0.00004806 * (tropoAlt - alt));
	}
	else {
	    *ttheta = 0.682457f + alt/945374.0f;
	    *rsigma = (float)pow( 0.978261+alt/659515.0, -35.16319 );
	}
	
	return (*ttheta) * (*rsigma);
}

// sort of atmospheric
float AirframeClass::EngineSmokeFactor()
{
    switch(auxaeroData->engineSmokes) {
    default:
	return 2;
    case 1: // vortex
    case 0: // nothing
	return 1;
    case 2: // light smoke
	return 2;
    case 3:
	return 4;
    }
}

int AirframeClass::EngineTrail()
{
    switch(auxaeroData->engineSmokes) {
    case 0: // nothing
	return -1;
    case 1: // vortex
	return TRAIL_VORTEX;
    case 2: // light smoke
	return TRAIL_SMOKE;
    case 3:
	return TRAIL_DARKSMOKE;
    default:
	/*if (auxaeroData->engineSmokes > 3 && auxaeroData->engineSmokes - 3 < TRAIL_MAX)
	    return auxaeroData->engineSmokes - 3;*/
		if (auxaeroData->engineSmokes > 3)
			return auxaeroData->engineSmokes;//Cobra Allow for more engine trails 
		else return -1;
    }
}

