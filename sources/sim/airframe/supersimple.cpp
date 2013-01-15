#include "stdhdr.h"
#include "airframe.h"
#include "geometry.h"
#include "aircrft.h"

void AirframeClass::SuperSimpleFCS (void)
{
	float cosphiLimit;
	float fullThrow;

   cosphiLimit = max(0.0F,platform->platformAngles.cosphi);

   Axial(SimLibMajorFrameTime);
   Aerodynamics();

   // Roll 'dynamics'
   pstab = 0.75F * rstick*300.0F * DTR + 0.26F * pstab;
   if (rstick == 0.0F && fabs(pstab) < 0.1F)
      pstab = 0.0F;

   // Yaw rate
   rstab = 0.75F * ypedal*3.0F + 0.25F * rstab;
   if (ypedal == 0.0F && fabs(rstab) < 0.1F)
      rstab = 0.0F;

   // Pitch 'Dynamics
   if (pstick > 0.0F)
   {
	  fullThrow = maxGs - platform->platformAngles.cosgam*cosphiLimit;
      //fullThrow = 8.0F;
   }
   else
      fullThrow = 4.0F;

   nzcgs = nzcgb = platform->platformAngles.cosgam*cosphiLimit + pstick * fullThrow;
   alpha = nzcgb * GRAVITY / (qsom * cnalpha);
   if (alpha > aoamax)
   {
      alpha = aoamax;
      nzcgb = nzcgs = alpha * qsom * cnalpha / GRAVITY;
   }
   else if (alpha < aoamin)
   {
      alpha = aoamin;
      nzcgb = nzcgs = alpha * qsom * cnalpha / GRAVITY;
   }
   //alpdot = 0.0F;
}

void AirframeClass::SuperSimpleEngine (void)
{
   EngineModel(SimLibMinorFrameTime);
}

float AirframeClass::FuelBurn (int maxAB)
{
    float th1;
    float flow;
	BIG_SCALAR mz = -z;
    if (maxAB)
    {
		th1 = Math.TwodInterp (mz, mach, engineData->alt,
			engineData->mach, engineData->thrust[2], engineData->numAlt,
			engineData->numMach, &curEngAltBreak, &curEngMachBreak);
		flow = th1  * engineData->thrustFactor * auxaeroData->fuelFlowFactorAb / 3600.0F;
    }
    else
    {
		th1 = Math.TwodInterp (mz, mach, engineData->alt,
			engineData->mach, engineData->thrust[1], engineData->numAlt,
			engineData->numMach, &curEngAltBreak, &curEngMachBreak);
		flow = th1  * engineData->thrustFactor * auxaeroData->fuelFlowFactorNormal / 3600.0F;
    }

   return flow;
}

float AirframeClass::PsubS (int maxAB)
{
	float desThrust;
	float th1;
	float retval;
	BIG_SCALAR mz = -z;

	if (maxAB)
	{
      th1 = Math.TwodInterp (mz, mach, engineData->alt,
         engineData->mach, engineData->thrust[2], engineData->numAlt,
         engineData->numMach, &curEngAltBreak, &curEngMachBreak);
	}
	else
	{
      th1 = Math.TwodInterp (mz, mach, engineData->alt,
         engineData->mach, engineData->thrust[1], engineData->numAlt,
         engineData->numMach, &curEngAltBreak, &curEngMachBreak);
	}

	// Thrust Scaling
	desThrust = th1 * engineData->thrustFactor;

	retval = (desThrust + xwaero * mass) * vt / weight;

	return retval;
}

float AirframeClass::SustainedGs (int maxAB)
{
	float desThrust;
	float th1;
	float retval, alphaMax;
	float curDrag;
	int i = 0;
	BIG_SCALAR mz = -z;

	// Max sustained alpha when thrust = drag
	if (maxAB)
	{
      th1 = Math.TwodInterp (mz, mach, engineData->alt,
         engineData->mach, engineData->thrust[2], engineData->numAlt,
         engineData->numMach, &curEngAltBreak, &curEngMachBreak);
	}
	else
	{
      th1 = Math.TwodInterp (mz, mach, engineData->alt,
         engineData->mach, engineData->thrust[1], engineData->numAlt,
         engineData->numMach, &curEngAltBreak, &curEngMachBreak);
	}

   // Thrust Scaling
   desThrust = th1 * engineData->thrustFactor;

   // Normalize
   desThrust /= (qbar * area);

   // factor gear/speed brake/stores
   desThrust -= auxaeroData->CDSPDBFactor*dbrake + auxaeroData->CDLDGFactor*gearPos;

   alphaMax = -0.5F;
   do
   {
      alphaMax += 0.5F;
      curDrag = Math.TwodInterp(mach, alphaMax, aeroData->mach, aeroData->alpha,
         aeroData->cdrag, aeroData->numMach, aeroData->numAlpha, &curMachBreak, &i) *
         aeroData->cdFactor;;
   } while (alphaMax < aoamax && curDrag < desThrust);

   // Given alpha, find max gs
   retval = alphaMax * qsom * clalph0 / GRAVITY;
   return retval;
}
