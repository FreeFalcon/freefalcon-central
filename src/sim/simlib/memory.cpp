/******************************************************************************/
/*                                                                            */
/*  Unit Name : memory.cpp                                                    */
/*                                                                            */
/*  Abstract  : Source file for functions implementing the classless memory   */
/*              utility functions.                                            */
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
#include "stdhdr.h"

/********************************************************************/
/*                                                                  */
/* Routine: SIM_VOID *SimLibAlloc (SIM_INT)                         */
/*                                                                  */
/* Description:                                                     */
/*    Allocate len bytes of memory.                                 */
/*                                                                  */
/* Inputs:                                                          */
/*    SIM_INT len - Number of bytes to allocate.                    */
/*                                                                  */
/* Outputs:                                                         */
/*    Pointer to allocated memory.                                  */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
SIM_VOID *SimLibAlloc(SIM_INT len)
{
    return (new char[len]);
}

/********************************************************************/
/*                                                                  */
/* Routine: SIM_INT SimLibFree (SIM_VOID *)                         */
/*                                                                  */
/* Description:                                                     */
/*    Release previously allocated memory                           */
/*                                                                  */
/* Inputs:                                                          */
/*    SIM_VOID *mem_ptr - Pointer to memory to be freed             */
/*                                                                  */
/* Outputs:                                                         */
/*    SIMLIB_OK for success, SIMLIB_ERR for failure.                */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
SIM_INT SimLibFree(SIM_VOID *mem_ptr)
{
    delete mem_ptr;

    return (SIMLIB_OK);
}
