/******************************************************************************/
/*                                                                            */
/*  Unit Name : cruise.cpp                                                    */
/*                                                                            */
/*  Abstract  : Finds aerodynamic forces                                      */
/*                                                                            */
/*                                                                            */
/*  Development History :                                                     */
/*  Date      Programer           Description                                 */
/*----------------------------------------------------------------------------*/
/*  20-Oct-97 VF                  Initial Write                               */
/*                                                                            */
/******************************************************************************/

#include "stdhdr.h"
#include "airframe.h"
#include "simmath.h"


float AirframeClass::GetOptimumCruise(void) {

	float Cl;
	float OptimumAirspeed;
	float	OptimumMach;
	float pressureRatio;
	
	Cl = Math.TwodInterp(0.7F,
								OPTIMUM_ALPHA+(dragIndex/60-1.4f),// me123 take drag into acount 
								aeroData->mach,
								aeroData->alpha, 
								aeroData->clift,
								aeroData->numMach, 
								aeroData->numAlpha,
								&curMachBreak, 
								&curAlphaBreak) * aeroData->clFactor;
													//me13 RHOASL from rho
	OptimumAirspeed	= (float)sqrt (2.0F * weight / (RHOASL * area * Cl)) * FTPSEC_TO_KNOTS;
	pressureRatio		= rho / RHOASL;
	OptimumMach			= CalcMach(OptimumAirspeed, pressureRatio);

	return OptimumMach;
}

float	AirframeClass::GetOptimumAltitude(void) {

	float optimum_altitude;

	// note: must find the drag index for either aim120 or f16
	optimum_altitude =	OPTIMUM_ALT_M1 * dragIndex*3 +// me123 take drag into acount 
								OPTIMUM_ALT_M2 * weight + 
								OPTIMUM_ALT_B;
	
	return optimum_altitude;
}	
//MI
float AirframeClass::GetOptKias(int mode)
{

	// mode 0 =climb 1 =end  2 =rng
	float Cl;
	float OptimumAirspeed;
	
	Cl = Math.TwodInterp(0.7F,
								OPTIMUM_ALPHA+(dragIndex/60-1.4f),// me123 take drag into acount 
								aeroData->mach,
								aeroData->alpha, 
								aeroData->clift,
								aeroData->numMach, 
								aeroData->numAlpha,
								&curMachBreak, 
								&curAlphaBreak) * aeroData->clFactor;
													//me13 RHOASL from rho
	OptimumAirspeed	= (float)sqrt (2.0F * weight / (RHOASL * area * Cl)) * FTPSEC_TO_KNOTS;

	if (mode == 0) OptimumAirspeed	+= 100.0f;
	if (mode == 2) OptimumAirspeed	+= 30.0f;

	if (1)
	{
		float ttheta, rsigma;
		float mach, vcas, pa;
		float qpasl1, oper, qc;

			if (mode == 0) mach = 0.86f;
			if (mode == 1) mach = 0.78f;
			if (mode == 2) mach = 0.85f;
		   
			CalcPressureRatio(-z, &ttheta, &rsigma);
			pa   = ttheta * rsigma * PASL;

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
		   OptimumAirspeed = min (OptimumAirspeed,vcas);
	}

	return OptimumAirspeed;
}
