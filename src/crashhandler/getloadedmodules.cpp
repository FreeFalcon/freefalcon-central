/*----------------------------------------------------------------------
       John Robbins - Microsoft Systems Journal Bugslayer Column
----------------------------------------------------------------------*/

#include "pch.h"
#include "BugslayerUtil.h"

// The project internal header file.
#include "Internal.h"

// The documentation for this function is in BugslayerUtil.h.
BOOL BUGSUTIL_DLLINTERFACE __stdcall
GetLoadedModules(DWORD     dwPID        ,
                 UINT      uiCount      ,
                 HMODULE * paModArray   ,
                 LPUINT    puiRealCount)
{
    // Do the debug checking.
    ASSERT(NULL not_eq puiRealCount) ;
    ASSERT(FALSE == IsBadWritePtr(puiRealCount , sizeof(UINT)));
#ifdef _DEBUG

    if (0 not_eq uiCount)
    {
        ASSERT(NULL not_eq paModArray) ;
        ASSERT(FALSE == IsBadWritePtr(paModArray                   ,
                                      uiCount *
                                      sizeof(HMODULE)));
    }

#endif

    // Do the parameter checking for real.  Note that I only check the
    //  memory in paModArray if uiCount is > 0.  The user can pass zero
    //  in uiCount if they are just interested in the total to be
    //  returned so they could dynamically allocate a buffer.
    if ((TRUE == IsBadWritePtr(puiRealCount , sizeof(UINT)))    or
        ((uiCount > 0) and 
         (TRUE == IsBadWritePtr(paModArray ,
                                uiCount * sizeof(HMODULE)))))
    {
        SetLastErrorEx(ERROR_INVALID_PARAMETER , SLE_ERROR) ;
        return (0) ;
    }

    // TOOLHELP32.
    return (TLHELPGetLoadedModules(dwPID,
                                   uiCount,
                                   paModArray,
                                   puiRealCount));
}

