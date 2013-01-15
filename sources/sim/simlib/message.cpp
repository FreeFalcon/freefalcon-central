/******************************************************************************/
/*                                                                            */
/*  Unit Name : message.cpp                                                   */
/*                                                                            */
/*  Abstract  : Source file for functions implementing the classless          */
/*              diagnostic output functions.                                  */
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
#include <windows.h>
#include "stdhdr.h"
#include "dispcfg.h"

char debstr[10][120];

/********************************************************************/
/*                                                                  */
/* Routine: SIM_INT SimLibPrintError (char *, ...)                  */
/*                                                                  */
/* Description:                                                     */
/*    Dump an error message to the screen                           */
/*                                                                  */
/* Inputs:                                                          */
/*    char *fmt - Format string ala printf                          */
/*    ...       - Whatever is to be displayed                       */
/*                                                                  */
/* Outputs:                                                         */
/*    SIMLIB_OK                                                     */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
SIM_INT SimLibPrintError (char *fmt, ...)
{
va_list ap;
char new_str[1023];

        /* This comment is for LINT */
        /*CONSTCOND*/
        va_start (ap, fmt);

        vsprintf (new_str, fmt, ap);

        va_end (ap);

        MessageBox (FalconDisplay.appWin, new_str, "ERROR", MB_OK | MB_ICONSTOP);
        return (SIMLIB_OK);
}

/********************************************************************/
/*                                                                  */
/* Routine: SIM_INT SimLibPrintMessage (char *, ...)                */
/*                                                                  */
/* Description:                                                     */
/*    Dump an error message to the screen                           */
/*                                                                  */
/* Inputs:                                                          */
/*    char *fmt - Format string ala printf                          */
/*    ...       - Whatever is to be displayed                       */
/*                                                                  */
/* Outputs:                                                         */
/*    SIMLIB_OK                                                     */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
SIM_INT SimLibPrintMessage (char *fmt, ...)
{
va_list ap;
char new_str[1023];

        /* This comment is for LINT */
        /*CONSTCOND*/
        va_start (ap, fmt);

        vsprintf (new_str, fmt, ap);

        va_end (ap);

        MessageBox (FalconDisplay.appWin, new_str, "Message", MB_OK);
        return (SIMLIB_OK);
}
