/*----------------------------------------------------------------------
       John Robbins - Microsoft Systems Journal Bugslayer Column

A simpler way to figure out the OS.
----------------------------------------------------------------------*/

#include "PCH.h"
#include "BugslayerUtil.h"
#include "Internal.h"

/*//////////////////////////////////////////////////////////////////////
                           File Scope Globals
//////////////////////////////////////////////////////////////////////*/
// Indicates that the version information is valid.
static BOOL g_bHasVersion = FALSE ;
// Indicates NT or 95/98.
static BOOL g_bIsNT = TRUE ;

BOOL __stdcall IsNT(void)
{
    if (TRUE == g_bHasVersion)
    {
        return (TRUE == g_bIsNT) ;
    }

    g_bIsNT = TRUE ;
    g_bHasVersion = TRUE ;
    return (TRUE == g_bIsNT) ;
}


