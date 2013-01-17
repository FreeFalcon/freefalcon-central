/******************************************************************************/
/*                                                                            */
/*  Unit Name : fcs.cpp                                                       */
/*                                                                            */
/*  Abstract  : Model the flight control laws                                 */
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
#include "simbase.h"
#include "limiters.h"

/********************************************************************/
/*                                                                  */
/* Routine: void AirframeClass::FlightControlSystem (void)         */
/*                                                                  */
/* Description:                                                     */
/*    Do each of the axis in turn.                                  */
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
void AirframeClass::FlightControlSystem(void)
{
	Limiter *limiter = NULL;

	limiter = gLimiterMgr->GetLimiter(PitchYawControlDamper,vehicleIndex);
	if(limiter)
	{
		ylsdamp = plsdamp = limiter->Limit(qbar);
	}
	else
	{
		ylsdamp = plsdamp = 1.0F;
	}
	
	limiter = gLimiterMgr->GetLimiter(RollControlDamper,vehicleIndex);
	if(limiter)
	{
		rlsdamp = limiter->Limit(qbar);
	}
	else
	{
		rlsdamp = 1.0F;
	}

   /*----------------------------*/
   /* gain schedules and filters */
   /*----------------------------*/
   Gains();

   SetStallConditions();

   /*--------------*/
   /* control laws */
   /*--------------*/
   Pitch();
   Roll();
   Yaw();
   Axial(SimLibMinorFrameTime);

   // This is probably unnecessary (it'll happen later)
   // AND it is VERY wasteful...  SCR 8/5/98
   //Trigenometry();
}
