/******************************************************************************/
/*                                                                            */
/*  Unit Name : meters.cpp                                                    */
/*                                                                            */
/*  Abstract  : Calculate G loading in different axes.                        */
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

/********************************************************************/
/*                                                                  */
/* Routine: void AirframeClass::Accelerometers(void)               */
/*                                                                  */
/* Description:                                                     */
/*    Calculate G loading in the three axis systems.                */
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
void AirframeClass::Accelerometers(){
	oldnzcgs = nzcgs;

   /*--------------------------*/
   /* body axis accelerometers */
   /*--------------------------*/
   nxcgb =  (xaero + xprop)/GRAVITY;
   nycgb =  (yaero + yprop)/GRAVITY;
   nzcgb = -(zaero + zprop)/GRAVITY;
   if (platform->OnGround())
       nzcgb = max(1, nzcgb);

   /*-----------------------*/
   /* stability axis accels */
   /*-----------------------*/
   nxcgs =  (xsaero + xsprop)/GRAVITY;
   nycgs =  (ysaero + ysprop)/GRAVITY;
   nzcgs = -(zsaero + zsprop)/GRAVITY;

   /*------------------*/
   /* wind axis accels */
   /*------------------*/
   nxcgw =  (xwaero + xwprop)/GRAVITY;
   nycgw =  (ywaero + ywprop)/GRAVITY;
   nzcgw = -(zwaero + zwprop)/GRAVITY;
}
