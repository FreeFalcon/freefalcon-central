/*----------------------------------------------------------------------
John Robbins
Microsoft Systems Journal, October 1997 - Bugslayer!
----------------------------------------------------------------------*/

#include "PCH.h"
#include "BugslayerUtil.h"
#include "CRTDBG_Internals.h"

//lint -e717

////////////////////////////////////////////////////////////////////////
// The main code is only for _DEBUG builds.
////////////////////////////////////////////////////////////////////////
#ifdef _DEBUG

// An internal typedef to just keep the source lines from being 600
//  characters long.
typedef int (*PFNCOMPARE) ( const void * , const void * ) ;

////////////////////////////////////////////////////////////////////////
// File static variables.
////////////////////////////////////////////////////////////////////////
// The variable that is checked to see if life is initialized.
static BOOL             g_bLibInit = FALSE          ;
// The actual pointer to the array of client block dumpers and
//  initializers.  This is allocated out of a private heap.
static LPDVINFO         g_pstDVInfo = NULL          ;
// The number of items in the g_pstDVInfo array.
static DWORD            g_dwDVCount = 0             ;
// The highest client subblock value used.  This is so we can add items
//  out of range of the highest seen so far.
static DWORD            g_dwMaxSubtype = 0          ;
// The private heap that this library will allocate out all data from.
static HANDLE           g_hHeap     = NULL          ;
// The critical section that everything will be protected with.
static CRITICAL_SECTION g_stCritSec                 ;
// The pointer to the previous dump client function.  If this library
//  does not have a dumper for the client block
static _CRT_DUMP_CLIENT g_pfnPrevDumpClient = NULL  ;

////////////////////////////////////////////////////////////////////////
// Internal file prototypes.
////////////////////////////////////////////////////////////////////////
// The library initializer.
static BOOL InitializeLibrary ( void ) ;
// The library shutdown function.
static BOOL ShutdownLibrary ( void ) ;
// The dump function that the actual RTL will call.
static void DumpFunction ( void * pData , size_t nSize ) ;
// The function that the RTL will call when doing all client blocks.
static void DoForAllFunction ( void * pData , void * pContext ) ;
// Compares DVINFO structures for qsort and bsearch.
static int CompareDVInfos ( LPDVINFO pDV1 , LPDVINFO pDV2 ) ;
// Finds the registered dumper-validator block for a particular piece of
//  memory.
static LPDVINFO FindRegisteredBlockType ( void * pData ) ;
// Keeps the maximum subtype straight.
static BOOL CheckMaxSubType ( void ) ;

/*//////////////////////////////////////////////////////////////////////

The AutoMatic class.

    This class is simply one that is used to create a static variable,
g_cBeforeAndAfter below.  Since all constructors are called before
main/WinMain and destructors are called after main/WinMain, it gives me
a chance to set up and tear down the library.
    Overall, there are some hacks in here you should be aware of.  The
first is that the initialization segment for the whole file is declared
as compiler.  This is not recommended at all.  However, there is no
other way of ensuring static initialization order other than ordering
things on the command line.  Since this can be rather difficult when
libraries like MFC use #pragma comment to automagically link themselves
in, this seems to be the only way.
    The second big hack is that the destructor does the memory leak
checking and then shuts it off.  This is because the RTL clears out the
dump client before calling _CrtDumpMemoryLeaks on shutdown.  While I
suspect that the reasoning for clearing out the dump function is good,
it makes it rather difficult to use the nice little architecture I set
up here.  My guess is that since the RTL is shutting down, and the
user's dump function could be using the RTL, it could lead to crashes.
I don't use any CRTL calls in here except to call the debug reporting
scheme.  Since _CrtDumpMemoryLeaks uses these calls, they are safe to be
called.
    Finally, in the constructor, I force all the debugging flags on
because what's the sense of having a debug build if you are not going to
take advantage of everything at your disposal?
//////////////////////////////////////////////////////////////////////*/
#pragma warning (disable : 4074)
#pragma init_seg(compiler)
class AutoMatic
{
public      :
    AutoMatic ( )
    {
        int iFlags = _CrtSetDbgFlag ( _CRTDBG_REPORT_FLAG ) ;
        iFlags |= _CRTDBG_CHECK_ALWAYS_DF ;
        iFlags |= _CRTDBG_DELAY_FREE_MEM_DF ;
        iFlags |= _CRTDBG_LEAK_CHECK_DF ;
        _CrtSetDbgFlag ( iFlags ) ;
    }
    ~AutoMatic ( )
    {
        if ( TRUE == g_bLibInit )
        {
            EnterCriticalSection ( &g_stCritSec ) ;
            // Do the leak checking, then turn off the option so the
            //  CRT does not show it.
            _CrtDumpMemoryLeaks ( ) ;
            int iFlags = _CrtSetDbgFlag ( _CRTDBG_REPORT_FLAG ) ;
            iFlags &= ~_CRTDBG_LEAK_CHECK_DF ;
            _CrtSetDbgFlag ( iFlags ) ;
            ShutdownLibrary ( ) ;
        }
    }
} ;

// The static class.
static AutoMatic g_cBeforeAndAfter ;

////////////////////////////////////////////////////////////////////////
// Public Implementation starts here.
////////////////////////////////////////////////////////////////////////

int __stdcall AddClientDV ( LPDVINFO lpDVInfo )
{
    BOOL bRet = TRUE ;

    __try
    {
        __try
        {
            ASSERT ( NULL != lpDVInfo ) ;
            ASSERT ( FALSE ==
                          IsBadCodePtr ( (FARPROC)lpDVInfo->pfnDump) ) ;
            ASSERT ( FALSE ==
                       IsBadCodePtr ( (FARPROC)lpDVInfo->pfnValidate ));

            if ( ( NULL == lpDVInfo )                                ||
                 ( TRUE ==
                      IsBadCodePtr((FARPROC)lpDVInfo->pfnDump     ) )||
                 ( TRUE ==
                      IsBadCodePtr((FARPROC)lpDVInfo->pfnValidate ) )  )
            {
                _RPT0 ( _CRT_WARN , "Bad parameters to AddClientDV\n" );
                return ( FALSE ) ;
            }

            // Has everything been initialized?
            if ( FALSE == g_bLibInit )
            {
                InitializeLibrary ( ) ;
            }

            // Block access to the library.
            EnterCriticalSection ( &g_stCritSec ) ;

            // Do the trivial case add first.
            if ( 0 == g_dwDVCount )
            {
                ASSERT ( NULL == g_pstDVInfo ) ;

                g_dwDVCount = 1 ;
                // Check if we are supposed to make up the new client
                //  value.
                if ( 0 == CLIENT_BLOCK_SUBTYPE ( lpDVInfo->dwValue ) )
                {
                    lpDVInfo->dwValue =
                                    CLIENT_BLOCK_VALUE ( g_dwDVCount ) ;
                    g_dwMaxSubtype = 1 ;
                }
                else
                {
                    g_dwMaxSubtype =
                            CLIENT_BLOCK_SUBTYPE ( lpDVInfo->dwValue ) ;
                    if ( FALSE == CheckMaxSubType ( ) )
                    {
                        g_dwDVCount = 0 ;
                        __leave ;
                    }
                }
                g_pstDVInfo =
                       (LPDVINFO)HeapAlloc ( g_hHeap                  ,
                                             HEAP_GENERATE_EXCEPTIONS |
                                              HEAP_ZERO_MEMORY        ,
                                             sizeof ( DVINFO )        );
                g_pstDVInfo[ 0 ] = *lpDVInfo ;
                __leave ;
            }
            // Is this a specific value add?
            else if ( 0 != CLIENT_BLOCK_SUBTYPE ( lpDVInfo->dwValue ) )
            {
                LPDVINFO lpDV ;

                // Make sure that this value is not already in the list.
                lpDV = (LPDVINFO)bsearch ( lpDVInfo                  ,
                                           g_pstDVInfo               ,
                                           g_dwDVCount               ,
                                           sizeof ( DVINFO )         ,
                                          (PFNCOMPARE)CompareDVInfos  );
                ASSERT ( NULL == lpDV ) ;

                if ( NULL != lpDV )
                {
                    _RPT1 ( _CRT_WARN   ,
                            "%08X is already in the MemDumperValidate "
                            "list!\n"   ,
                            lpDVInfo->dwValue             );
                    bRet = FALSE ;
                    __leave ;
                }
                // Check the block subtype against the highest we have
                //  seen so far.  If this is the new high, save the
                //  value off.
                if ( g_dwMaxSubtype <
                     CLIENT_BLOCK_SUBTYPE ( lpDVInfo->dwValue ) )
                {
                    g_dwMaxSubtype =
                            CLIENT_BLOCK_SUBTYPE ( lpDVInfo->dwValue ) ;
                    if ( FALSE == CheckMaxSubType ( ) )
                    {
                        __leave ;
                    }
                }
            }
            // Bump up the count.
            g_dwDVCount++ ;

            BOOL bTackOnEnd = FALSE ;
            // If the user wants to just get a value assigned, then
            //  the returned number is one larger than the last one in
            //  the list.
            if ( 0 == CLIENT_BLOCK_SUBTYPE ( lpDVInfo->dwValue ) )
            {
                // Bump up the max subtype that we have seen.
                if ( FALSE == CheckMaxSubType ( ) )
                {
                    g_dwDVCount-- ;
                    __leave ;
                }
                bTackOnEnd = TRUE ;
                lpDVInfo->dwValue =
                                 CLIENT_BLOCK_VALUE ( g_dwMaxSubtype ) ;
            }

            // Reallocate the array.
            g_pstDVInfo =
                (LPDVINFO)HeapReAlloc ( g_hHeap                     ,
                                        HEAP_GENERATE_EXCEPTIONS |
                                            HEAP_ZERO_MEMORY        ,
                                        g_pstDVInfo                 ,
                                        g_dwDVCount * sizeof ( DVINFO));

            if ( TRUE == bTackOnEnd )
            {
                g_pstDVInfo[ g_dwDVCount - 1 ] = *lpDVInfo ;
            }
            else
            {
                // Bummer, pound through the array and look for the slot
                //  to insert it.
                DWORD iCurr = 0 ;
                DWORD dwToFind =
                              CLIENT_BLOCK_SUBTYPE( lpDVInfo->dwValue );
                while ( dwToFind >
                       CLIENT_BLOCK_SUBTYPE(g_pstDVInfo[iCurr].dwValue))
                {
                    iCurr++ ;
                    if ( iCurr == g_dwDVCount )
                    {
                        // Tack it on the end.
                        g_pstDVInfo[ g_dwDVCount - 1 ] = *lpDVInfo ;
                        __leave ;
                    }
                }
                DWORD dwDest = (DWORD)g_pstDVInfo +
                                ( (iCurr+1) * sizeof ( DVINFO ) );

                DWORD dwSrc = (DWORD)g_pstDVInfo +
                               ( iCurr * sizeof ( DVINFO ) ) ;

                DWORD dwCount = ( ( g_dwDVCount - 1 ) - iCurr) *
                                sizeof ( DVINFO ) ;

                // Move memory to allow the insertion.
                memmove ( (void*)dwDest , (void*)dwSrc , dwCount ) ;

                // Insert the element.
                g_pstDVInfo[ iCurr ] = *lpDVInfo ;
            }
        }
        __except ( EXCEPTION_EXECUTE_HANDLER )
        {
            ASSERT ( FALSE ) ;
            bRet = FALSE ;
        }
    }
    __finally
    {
        LeaveCriticalSection ( &g_stCritSec ) ;
    }
    return ( bRet ) ;
}

void __stdcall ValidateAllBlocks ( void * pContext )
{
    __try
    {
        // Block access to the library.
        EnterCriticalSection ( &g_stCritSec ) ;

        // The first thing to do is to let the CRT do it's normal stuff.
        _CrtCheckMemory ( ) ;

        // Now, have the CRT call our library validator for all client
        //  blocks.
        _CrtDoForAllClientObjects ( DoForAllFunction ,
                                    pContext          ) ;
    }
    __finally
    {
        LeaveCriticalSection ( &g_stCritSec ) ;
    }
}

////////////////////////////////////////////////////////////////////////
// Private Implementation starts here.
////////////////////////////////////////////////////////////////////////
/*----------------------------------------------------------------------
FUNCTION        :   DumpFunction
DISCUSSION      :
    The function the the RTL will call for all client blocks.  It
calculates which of the user's functions will be called.
PARAMETERS      :
RETURNS         :
----------------------------------------------------------------------*/
static void DumpFunction ( void * pData , size_t nSize )
{

    __try
    {
        __try
        {
            // Block access to the library.
            EnterCriticalSection ( &g_stCritSec ) ;

            ASSERT ( NULL != pData ) ;

            LPDVINFO lpDV = FindRegisteredBlockType ( pData ) ;

            if ( ( NULL != lpDV ) && ( NULL != lpDV->pfnDump ) )
            {
                // Call the dumper registered for this block.
                lpDV->pfnDump ( pData ) ;
            }
            // This is either a normal client block (not one that the
            //  user added to this list, or does not have a registered
            //  dumper so pass it on to the previous dumper function.
            else if ( NULL != g_pfnPrevDumpClient )
            {
                g_pfnPrevDumpClient ( pData , nSize ) ;
            }
            else
            {
                // I just lifted the _printMemBlockData function out of
                //  DBGHEAP.C and put it here.
                _CrtMemBlockHeader * pHead ;
                pHead = pHdr ( pData ) ;
                ASSERT ( NULL != pHead ) ;

                #define MAXPRINT 16
                int           i                             ;
                unsigned char ch                            ;
                unsigned char printbuff[ MAXPRINT + 1 ]     ;
                unsigned char valbuff[ MAXPRINT * 3 + 1 ]   ;

                for ( i = 0 ;
                      i < min( (int)pHead->nDataSize , MAXPRINT ) ;
                      i++     )
                {
                    ch = pbData(pHead)[ i ] ;
                    printbuff[ i ] = isprint( ch ) ? ch : ' ' ;
                    wsprintf ( (char*)&valbuff[ i * 3 ] , "%.2X " , ch);
                }
                printbuff[ i ] = '\0' ;

                _RPT2 ( _CRT_WARN           ,
                         " Data: <%s> %s\n" ,
                         printbuff          ,
                         valbuff             ) ;
            }
        }
        __except ( EXCEPTION_EXECUTE_HANDLER )
        {
            ASSERT ( FALSE ) ;
            _RPT0 ( _CRT_WARN , "There was a crash in DumpFunction!\n");
        }
    }
    __finally
    {
        LeaveCriticalSection ( &g_stCritSec ) ;
    }
}

/*----------------------------------------------------------------------
FUNCTION        :   DoForAllFunction
DISCUSSION      :
    The function that ValidateAllBlocks will call to have run over the
all the client blocks.  When called for a block, it will look at the
list of registered types and if there is a validator function, then
it will be called for the block.
PARAMETERS      :
    pData    - The data to validate.
    pContext - The context data originally passed to ValudateAllBlocks.
RETURNS         :
    None.
----------------------------------------------------------------------*/
static void DoForAllFunction ( void * pData , void * pContext )
{
    __try
    {
        __try
        {
            // Block access to the library.
            EnterCriticalSection ( &g_stCritSec ) ;

            ASSERT ( NULL != pData ) ;

            LPDVINFO lpDV = FindRegisteredBlockType ( pData ) ;

            // Only call the validator if there is one.  If there is not
            //  one, then there is nothing to do.
            if ( ( NULL != lpDV ) && ( NULL != lpDV->pfnValidate ) )
            {
                // Call the validator registered for this block.
                lpDV->pfnValidate ( pData , pContext ) ;
            }
        }
        __except ( EXCEPTION_EXECUTE_HANDLER )
        {
            ASSERT ( FALSE ) ;
            _RPT0 ( _CRT_WARN                                  ,
                    "There was a crash in DoForAllFunction!\n"  ) ;
        }
    }
    __finally
    {
        LeaveCriticalSection ( &g_stCritSec ) ;
    }
}

/*----------------------------------------------------------------------
FUNCTION        :   InitializeLibrary
DISCUSSION      :
    Completely initializes the library.  All of the file static
variables are ready to be used.
PARAMETERS      :
    None.
RETURNS         :
    TRUE  - The library was initialized.
    FALSE - There was a problem.
----------------------------------------------------------------------*/
static BOOL InitializeLibrary ( void )
{
    ASSERT ( FALSE == g_bLibInit ) ;

    // Always start by initializing the critical section.
    InitializeCriticalSection ( &g_stCritSec ) ;
    // Create the private heap for this library.
    g_hHeap = HeapCreate ( HEAP_GENERATE_EXCEPTIONS , 0 , 0 ) ;

    // Now hook our dump function up.
    g_pfnPrevDumpClient =
            _CrtSetDumpClient ( (_CRT_DUMP_CLIENT)DumpFunction ) ;

    g_bLibInit = TRUE ;

    return ( TRUE ) ;
}

/*----------------------------------------------------------------------
FUNCTION        :   ShutdownLibrary
DISCUSSION      :
    Takes care of freeing all the memory and shutting down the library.
This MUST be called after the critical section has already been grabbed
because it will take care of releasing it and destroying it.
PARAMETERS      :
    None.
RETURNS         :
    TRUE  - Everything was kosher.
    FALSE - Danger, Will Robinson.
----------------------------------------------------------------------*/
static BOOL ShutdownLibrary ( void )
{
    ASSERT ( TRUE == g_bLibInit ) ;

    // Get rid of all the private memory in one fell swoop.
    BOOL bRet = HeapDestroy ( g_hHeap ) ;
    ASSERT ( TRUE == bRet ) ;
    g_hHeap = NULL ;

    // Set the previous dump client back.
    if ( NULL != g_pfnPrevDumpClient )
    {
        _CrtSetDumpClient ( (_CRT_DUMP_CLIENT)g_pfnPrevDumpClient ) ;
    }

    // Reset all of the global variables.
    g_pstDVInfo         = NULL  ;
    g_dwDVCount         = 0     ;
    g_dwMaxSubtype      = 0     ;
    g_hHeap             = NULL  ;
    g_pfnPrevDumpClient = NULL  ;
    g_bLibInit          = FALSE ;

    // Remember, the critical section is blocked here so clear it then
    //  get rid of it because we are done.
    LeaveCriticalSection ( &g_stCritSec ) ;
    DeleteCriticalSection ( &g_stCritSec ) ;

    return ( TRUE ) ;
}

/*----------------------------------------------------------------------
FUNCTION        :   CompateDVInfos
DISCUSSION      :
    Compares the dwValue parameters for two DVINFO structures.
PARAMETERS      :
    pDV1, pDV2 - The structures to compare.
RETURNS         :
    < 0 - pDV1 < pDV2
    0   - pDV1 = pDV2
    > 0 - pDV1 > pDV2
----------------------------------------------------------------------*/
static int CompareDVInfos ( LPDVINFO pDV1 , LPDVINFO pDV2 )
{
    ASSERT ( NULL != pDV1 ) ;
    ASSERT ( NULL != pDV2 ) ;

    if ( pDV1->dwValue < pDV2->dwValue )
    {
        return ( -1 ) ;
    }
    if ( pDV1->dwValue > pDV2->dwValue )
    {
        return ( 1 ) ;
    }
    return ( 0 ) ;
}

/*----------------------------------------------------------------------
FUNCTION        :   CheckMaxSubType
DISCUSSION      :
    A simple function that keeps the maximum subtype straight.
PARAMETERS      :
    None.
RETURNS         :
    TRUE  - g_dwMaxSubtype is properly updated and ready for use.
    FALSE - g_dwMaxSubtype is out of range.
----------------------------------------------------------------------*/
static BOOL CheckMaxSubType ( )
{
    ASSERT ( (WORD)g_dwMaxSubtype < (WORD)0xFFFF ) ;

    if ( g_dwMaxSubtype >= 0xFFFF )
    {
        _RPT0 ( _CRT_WARN                        ,
                "Running max subtype value in "
                "MemDumperValidator is to high!"  ) ;
        return ( FALSE ) ;
    }
    g_dwMaxSubtype++ ;
    return ( TRUE ) ;
}

static LPDVINFO FindRegisteredBlockType ( void * pData )
{
    // Get at the header for the block to get the client type.
    _CrtMemBlockHeader * pHead ;
    pHead = pHdr ( pData ) ;
    ASSERT ( NULL != pHead ) ;

    DVINFO   stDVFind   ;
    LPDVINFO lpDV       ;

    stDVFind.dwValue = (DWORD)pHead->nBlockUse ;

    // Try and find the value.
    lpDV = (LPDVINFO)bsearch ( &stDVFind                    ,
                               g_pstDVInfo                  ,
                               g_dwDVCount                  ,
                               sizeof ( DVINFO )            ,
                               (PFNCOMPARE)CompareDVInfos    ) ;
    return ( lpDV ) ;
}

#else  // ! _DEBUG

// This is a release build so put in two stub functions to keep the
//  exports happy.
int __stdcall AddClientDV ( void * )
{
    return ( 0 ) ;
}

void __stdcall ValidateAllBlocks ( void * )
{
}

#endif

//lint +e717
