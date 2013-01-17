/******************************************************************************/
/*                                                                            */
/*  Unit Name : trim.cpp                                                      */
/*                                                                            */
/*  Abstract  : Routine for triming the A/C to it's initial state             */
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

/********************************************************************/
/*                                                                  */
/* Routine: void AirframeClass::TrimModel (void)                   */
/*                                                                  */
/* Description:                                                     */
/*    Itterates pitch and throttle commands to find a steady state  */
/*    condition.                                                    */
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
void AirframeClass::TrimModel(void)
{
	register int i=0;
	int ii = 0;
	float error=0.0F;
	float alph1=0.0F, thr1=0.0F, accx1=0.0F, accz1=0.0F;
	float alph2=0.0F, thr2=0.0F, accx2=0.0F, accz2=0.0F;
	int isTrimmed;
	
	/*---------------*/
	/* set trim flag */
	/*---------------*/
	SetFlag (Trimming);
	error  = 1.0e-04F;
	
	isTrimmed = FALSE;
	/*---------------------------------------------------------*/
	/* trim loop: aoa to trim alphadot, throttle to trim vtdot */
	/*---------------------------------------------------------*/
	do
	{
		if (!isTrimmed)
		{
			/*-----------------*/
			/* initial guesses */
			/*-----------------*/
			alpha  = 0.0F;
			theta  = alpha * DTR;
			throtl = 0.0F;
			alph1  = alpha;
			thr1   = throtl;
			Trigenometry();
			Aerodynamics();
			EngineModel(SimLibMinorFrameTime);
			accx1 = xsaero + xsprop;
			accz1 = zsaero + zsprop + GRAVITY;
			
			alpha  = 5.0F;
			theta  = alpha*DTR;
			throtl = 1.5F;
			alph2  = alpha;
			thr2   = throtl;
			Trigenometry();
			Aerodynamics();
			EngineModel(SimLibMinorFrameTime);
			accx2 = xsaero + xsprop;
			accz2 = zsaero + zsprop + GRAVITY;
		}
		
		for (i=0; i<50; i++)
		{
			if (fabs(accx2) < error && fabs(accz2) < error)
			{
				isTrimmed = TRUE;
				break;
			}
			alpha  = Predictor(alph1,alph2,accz1,accz2);
			throtl = Predictor(thr1,thr2,accx1,accx2);
			
			if (alpha > 13.0F) 
				alpha = 13.0F;
			else if (alpha < 0.0F)
				alpha = 0.0F;
			else if (zaero > 0.0F)
				alpha += 1.0F;
			
			if (throtl > 1.5F)
			{
				throtl = 1.5F;
				//         thr1 = 0.0F;
			}
			else
			{
				thr1   = thr2;
				accx1  = accx2;
			}
			
			theta  = alpha*DTR;
			alph1  = alph2;
			accz1  = accz2;
			
			Trigenometry();
			Aerodynamics();
			EngineModel(SimLibMinorFrameTime);
			
			accx2 = xsaero + xsprop;
			accz2 = zsaero + zsprop + GRAVITY;
			alph2 = alpha;
			thr2  = throtl;
		}
		
		// Too slow for weight, make it faster
		if (!isTrimmed && (_isnan(accz2) || fabs(accz2) > error))
		{
			ii ++;
			
			if (ii > 5)
				isTrimmed = TRUE;
			
			mach *= 1.1F;
			Atmosphere();
			
//			MonoPrint ("Trimmed to slow, trying %.2f\n", mach);
		}
		else
		{
			isTrimmed = TRUE;
		}
		
	} while (!isTrimmed);
	
	//   F4Assert (i<50);
	
	Trigenometry();
	Aerodynamics();
	EngineModel(SimLibMinorFrameTime);
	Accelerometers();
	
	ClearFlag (Trimming);
}

/********************************************************************/
/*                                                                  */
/* Routine: float AirframeClass::Predictor(float, float, float     */
/*    float)                                                        */
/*                                                                  */
/* Description:                                                     */
/*    Calculates the X intercept of the given line.                 */
/*                                                                  */
/* Inputs:                                                          */
/*    float x1 - X value of the first point                         */
/*    float y1 - Y value of the first point                         */
/*    float x2 - X value of the second point                        */
/*    float y2 - Y value of the second point                        */
/*                                                                  */
/* Outputs:                                                         */
/*    float - X value where y = 0.0                                 */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
float AirframeClass::Predictor(float x1, float x2, float y1, float y2)
{
float ret_val;

	/*------------------------------------------*/
	/* If they're close the split the differenc */
	/*------------------------------------------*/
   if (fabs (y2-y1) <= 1.e-6)
      ret_val = (x1+x2) * 0.5F;
   else
      ret_val = x2 - y2*(x2-x1)/(y2-y1);

   return (ret_val);
}
