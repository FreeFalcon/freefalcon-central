/******************************************************************************/
/*                                                                            */
/*  Unit Name : simmath.h                                                     */
/*                                                                            */
/*  Abstract  : Header file with class definition for SIMLIB_MATH_CLASS and   */
/*              defines used in its implementation.                           */
/*                                                                            */
/*  Dependencies : Auto-Generated                                             */
/*                                                                            */
/*  Operating System : MS-DOS 6.2, Windows 3.1                                */
/*                                                                            */
/*  Compiler : MSVC V1.5                                                      */
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
#ifndef _SIMMATH_H
#define _SIMMATH_H


typedef SIM_FLOAT SAVE_ARRAY[6];

class SIMLIB_MATH_CLASS
{
	private:

	public:
		SIM_SHORT  Limit (SIM_SHORT input, SIM_SHORT min, SIM_SHORT max);
		SIM_INT    Limit (SIM_INT input, SIM_INT min, SIM_INT max);
		SIM_LONG   Limit (SIM_LONG input, SIM_LONG min, SIM_LONG max);
		SIM_FLOAT  Limit (SIM_FLOAT input, SIM_FLOAT min, SIM_FLOAT max);
		SIM_FLOAT  RateLimit (SIM_FLOAT input, SIM_FLOAT cur, SIM_FLOAT maxRate,
			SIM_FLOAT *rate, SIM_FLOAT delt);
		SIM_FLOAT  DeadBand (SIM_FLOAT input, SIM_FLOAT minBreakout, SIM_FLOAT maxBreakout);
		SIM_FLOAT  Resolve (SIM_FLOAT input, SIM_FLOAT maxAngle);
		SIM_FLOAT  Resolve0 (SIM_FLOAT input, SIM_FLOAT maxAngle);
		SIM_FLOAT  ResolveWrap (SIM_FLOAT input, SIM_FLOAT maxAngle);
		SIM_DOUBLE Limit (SIM_DOUBLE input, SIM_DOUBLE min, SIM_DOUBLE max);
		SIM_DOUBLE RateLimit (SIM_DOUBLE input, SIM_DOUBLE cur, SIM_DOUBLE maxRate,
			SIM_DOUBLE *rate, SIM_DOUBLE delt);
		SIM_DOUBLE DeadBand (SIM_DOUBLE input, SIM_DOUBLE minBreakout, SIM_DOUBLE maxBreakout);
		SIM_DOUBLE Resolve (SIM_DOUBLE input, SIM_DOUBLE maxAngle);
		SIM_DOUBLE Resolve0 (SIM_DOUBLE input, SIM_DOUBLE maxAngle);
		SIM_DOUBLE ResolveWrap (SIM_DOUBLE input, SIM_DOUBLE maxAngle);

		// Tustin Filters
		SIM_FLOAT F1Tust(SIM_FLOAT ,SIM_FLOAT, SIM_FLOAT, SIM_FLOAT, SIM_FLOAT, SIM_FLOAT, SAVE_ARRAY, SIM_INT *);
		SIM_FLOAT F2Tust(SIM_FLOAT, SIM_FLOAT, SIM_FLOAT, SIM_FLOAT, SAVE_ARRAY);
		SIM_FLOAT F3Tust(SIM_FLOAT, SIM_FLOAT, SIM_FLOAT, SIM_FLOAT, SAVE_ARRAY);
		SIM_FLOAT F4Tust(SIM_FLOAT, SIM_FLOAT, SIM_FLOAT, SIM_FLOAT, SIM_FLOAT, SAVE_ARRAY,SIM_INT *);
		SIM_FLOAT F5Tust(SIM_FLOAT, SIM_FLOAT, SIM_FLOAT, SIM_FLOAT, SIM_FLOAT, SAVE_ARRAY,SIM_INT *);
		SIM_FLOAT F6Tust(SIM_FLOAT, SIM_FLOAT, SIM_FLOAT, SIM_FLOAT, SAVE_ARRAY,SIM_INT *);
		SIM_FLOAT F7Tust(SIM_FLOAT, SIM_FLOAT, SIM_FLOAT, SIM_FLOAT, SIM_FLOAT, SAVE_ARRAY,SIM_INT *);
		SIM_FLOAT F8Tust(SIM_FLOAT, SIM_FLOAT, SIM_FLOAT, SIM_FLOAT, SIM_FLOAT, SAVE_ARRAY,SIM_INT *);
		SIM_FLOAT FLTust(SIM_FLOAT, SIM_FLOAT, SIM_FLOAT, SAVE_ARRAY);
		SIM_FLOAT FLeadTust(SIM_FLOAT, SIM_FLOAT, SIM_FLOAT, SAVE_ARRAY);
		SIM_FLOAT FITust(SIM_FLOAT, SIM_FLOAT, SAVE_ARRAY);
		SIM_FLOAT FWTust(SIM_FLOAT, SIM_FLOAT, SIM_FLOAT, SAVE_ARRAY);

		//Adams-Bashforth Filters
		SIM_FLOAT SIMLIB_MATH_CLASS::FIAdamsBash(SIM_FLOAT in, SIM_FLOAT delt, SAVE_ARRAY save);

		// Interpolators
		SIM_FLOAT OnedInterp(SIM_FLOAT x, const SIM_FLOAT *xarray, const SIM_FLOAT *data, SIM_INT numx, SIM_INT *lastx);
		SIM_FLOAT TwodInterp(SIM_FLOAT x, SIM_FLOAT y, const SIM_FLOAT *xarray, const SIM_FLOAT *yarray,
		                  const SIM_FLOAT *data, SIM_INT numx, SIM_INT numy, SIM_INT *lastx, SIM_INT *lasty);
		SIM_FLOAT ThreedInterp(SIM_FLOAT x, SIM_FLOAT y, SIM_FLOAT z, const SIM_FLOAT *xarray, const SIM_FLOAT *yarray,
		                  const SIM_FLOAT *zarray, const SIM_FLOAT *data, SIM_INT numx, SIM_INT numy, SIM_INT numz,
		                  SIM_INT *lastx, SIM_INT *lasty, SIM_INT *lastz);

};

extern SIMLIB_MATH_CLASS Math;

#endif
