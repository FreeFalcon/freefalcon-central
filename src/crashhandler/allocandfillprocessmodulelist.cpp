/*----------------------------------------------------------------------
       John Robbins - Microsoft Systems Journal Bugslayer Column
----------------------------------------------------------------------*/

#include "pch.h"
#include "BugslayerUtil.h"

// The project internal header file.
#include "Internal.h"

HMODULE * /*BUGSUTIL_DLLINTERFACE*/ __stdcall
                      AllocAndFillProcessModuleList ( HANDLE hHeap    ,
                                                      LPUINT puiCount  )

{
    ASSERT ( FALSE == IsBadWritePtr ( puiCount , sizeof ( LPUINT ) ) ) ;
    if ( TRUE == IsBadWritePtr ( puiCount , sizeof ( LPUINT ) ) )
    {
        SetLastError ( ERROR_INVALID_PARAMETER ) ;
        return ( NULL ) ;
    }

    ASSERT ( NULL != hHeap ) ;
    if ( NULL == hHeap )
    {
        SetLastError ( ERROR_INVALID_PARAMETER ) ;
        return ( NULL ) ;
    }

    // First, ask how many modules are really loaded.
    UINT uiQueryCount ;

    BOOL bRet = GetLoadedModules ( GetCurrentProcessId ( ) ,
                                   0                       ,
                                   NULL                    ,
                                   &uiQueryCount            ) ;
    ASSERT ( TRUE == bRet ) ;
    ASSERT ( 0 != uiQueryCount ) ;

    if ( ( FALSE == bRet ) || ( 0 == uiQueryCount ) )
    {
        return ( NULL ) ;
    }

    // The HMODULE array.
    HMODULE * pModArray ;

    // Allocate the buffer to hold the returned array.
    pModArray = (HMODULE*)HeapAlloc ( hHeap             ,
                                      HEAP_ZERO_MEMORY  ,
                                      uiQueryCount *
                                                   sizeof ( HMODULE ) );

    ASSERT ( NULL != pModArray ) ;
    if ( NULL == pModArray )
    {
        return ( NULL ) ;
    }

    // bRet holds BOOLEAN return values.
    bRet = GetLoadedModules ( GetCurrentProcessId ( ) ,
                              uiQueryCount            ,
                              pModArray               ,
                              puiCount                 ) ;
    // Save off the last error so that the assert can still fire and
    //  not change the value.
    DWORD dwLastError = GetLastError ( ) ;
    ASSERT ( TRUE == bRet ) ;

    if ( FALSE == bRet )
    {
        // Get rid of the last buffer.
        free ( pModArray ) ;
        pModArray = NULL ;
        SetLastError ( dwLastError ) ;
    }
    else
    {
        SetLastError ( ERROR_SUCCESS ) ;
    }
    // All OK, Jumpmaster!
    return ( pModArray ) ;
}

