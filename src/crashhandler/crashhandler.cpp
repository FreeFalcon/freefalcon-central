/*----------------------------------------------------------------------
 John Robbins - Microsoft Systems Journal Bugslayer Column - August '98

CONDITIONAL COMPILATION :
    WORK_AROUND_SRCLINE_BUG - Define this to work around the
                              SymGetLineFromAddr bug where PDB file
                              lookups fail after the first lookup.
----------------------------------------------------------------------*/

#include <cISO646>
#include "pch.h"
#include "BugslayerUtil.h"
#include "CrashHandler.h"

// The project internal header file.
#include "Internal.h"

/*//////////////////////////////////////////////////////////////////////
                      File Scope Global Variables
//////////////////////////////////////////////////////////////////////*/
// The filter function.
static PFNCHFILTFN g_pfnCallBack = NULL ;

// The original exception filter.
static LPTOP_LEVEL_EXCEPTION_FILTER g_pfnOrigFilt = NULL ;

// The array of modules to limit Crash Handler to.
static HMODULE * g_ahMod = NULL ;
// The size, in items, of g_ahMod.
static UINT g_uiModCount = 0 ;

// The static buffer returned by various functions.  This avoids putting
//  things on the stack.
#define BUFF_SIZE 1024
static TCHAR g_szBuff [ BUFF_SIZE ] ;

// The static symbol lookup buffer.  This gets casted to make it work.
#define SYM_BUFF_SIZE 512
static BYTE g_stSymbol [ SYM_BUFF_SIZE ] ;

// The static source and line structure.
static IMAGEHLP_LINE g_stLine ;

// The stack frame used in walking the stack.
static STACKFRAME g_stFrame ;

// The pointer to the SymGetLineFromAddr function I GetProcAddress out
//  of IMAGEHLP.DLL in case the user has an older version that does not
//  support the new extensions.
static PFNSYMGETLINEFROMADDR g_pfnSymGetLineFromAddr = NULL ;

// The flag that says if I have already done the GetProcAddress on
//  g_pfnSymGetLineFromAddr.
static BOOL g_bLookedForSymFuncs = FALSE ;

// The flag that indicates that the symbol engine as been initialized.
static BOOL g_bSymEngInit = FALSE ;

/*//////////////////////////////////////////////////////////////////////
                    File Scope Function Declarations
//////////////////////////////////////////////////////////////////////*/
// The exception handler.
LONG __stdcall CrashHandlerExceptionFilter(EXCEPTION_POINTERS *
        pExPtrs) ;

// Converts a simple exception to a string value.
LPCTSTR ConvertSimpleException(DWORD dwExcept) ;

// The internal function that does all the stack walking.
LPCTSTR __stdcall
InternalGetStackTraceString(DWORD                dwOpts  ,
                            EXCEPTION_POINTERS * pExPtrs);

// The internal SymGetLineFromAddr function.
BOOL InternalSymGetLineFromAddr(IN  HANDLE          hProcess        ,
                                IN  DWORD           dwAddr          ,
                                OUT PDWORD          pdwDisplacement ,
                                OUT PIMAGEHLP_LINE  Line);

// Initializes the symbol engine if needed.
void InitSymEng(void) ;

// Cleans up the symbol engine if needed.
void CleanupSymEng(void) ;

/*//////////////////////////////////////////////////////////////////////
                            Destructor Class
//////////////////////////////////////////////////////////////////////*/
// See the note in MemDumpValidator.cpp about automatic classes.
#pragma warning (disable : 4073)
#pragma init_seg(lib)
class CleanUpCrashHandler
{
public  :
    CleanUpCrashHandler(void)
    {
    }
    ~CleanUpCrashHandler(void)
    {
        // Is there any outstanding memory allocations?
        if (NULL not_eq g_ahMod)
        {
            VERIFY(HeapFree(GetProcessHeap() ,
                            0                  ,
                            g_ahMod)) ;
            g_ahMod = NULL ;
        }

        if (NULL not_eq g_pfnOrigFilt)
        {
            // Set the handler back to what it originally was.
            SetUnhandledExceptionFilter(g_pfnOrigFilt) ;
        }
    }
} ;

// The static class.
static CleanUpCrashHandler g_cBeforeAndAfter ;

/*//////////////////////////////////////////////////////////////////////
                 Crash Handler Function Implementation
//////////////////////////////////////////////////////////////////////*/

BOOL __stdcall SetCrashHandlerFilter(PFNCHFILTFN pFn)
{
    // It's OK to have a NULL parameter because this will unhook the
    //  callback.
    if (NULL == pFn)
    {
        if (NULL not_eq g_pfnOrigFilt)
        {
            // Put the original one back.
            SetUnhandledExceptionFilter(g_pfnOrigFilt) ;
            g_pfnOrigFilt = NULL ;

            if (NULL not_eq g_ahMod)
            {
                free(g_ahMod) ;
                g_ahMod = NULL ;
            }

            g_pfnCallBack = NULL ;
        }
    }
    else
    {
        g_pfnCallBack = pFn ;

        // If this is the first time that CrashHandler has been called
        //  set the exception filter and save off the previous handler.
        if (NULL == g_pfnOrigFilt)
        {
            g_pfnOrigFilt =
                SetUnhandledExceptionFilter(CrashHandlerExceptionFilter);
        }
    }

    return (TRUE) ;
}

BOOL __stdcall AddCrashHandlerLimitModule(HMODULE hMod)
{
    // Check the obvious cases.
    ASSERT(NULL not_eq hMod) ;

    if (NULL == hMod)
    {
        return (FALSE) ;
    }

    // TODO TODO
    //  Do the check that hMod really is a PE module.

    // Allocate a temporary version.  This must be allocated into memory
    //  that is guaranteed to be around even if the process is toasting.
    //  This means the RTL heap is probably already gone so I do it out
    //  of the process heap.
    HMODULE * phTemp = (HMODULE*)
                       HeapAlloc(GetProcessHeap()                 ,
                                 HEAP_ZERO_MEMORY |
                                 HEAP_GENERATE_EXCEPTIONS        ,
                                 (sizeof(HMODULE) * (g_uiModCount + 1))) ;
    ASSERT(NULL not_eq phTemp) ;

    if (NULL == phTemp)
    {
        TRACE0("Serious trouble in the house - malloc failed\n");
        return (FALSE) ;
    }

    if (NULL == g_ahMod)
    {
        g_ahMod = phTemp ;
        g_ahMod[ 0 ] = hMod ;
        g_uiModCount++ ;
    }
    else
    {
        // Copy the old values.
        CopyMemory(phTemp     ,
                   g_ahMod    ,
                   sizeof(HMODULE) * g_uiModCount) ;
        g_ahMod = phTemp ;
        g_ahMod[ g_uiModCount ] = hMod ;
        g_uiModCount++ ;
    }

    return (TRUE) ;
}

UINT __stdcall GetLimitModuleCount(void)
{
    return (g_uiModCount) ;
}

int __stdcall GetLimitModulesArray(HMODULE * pahMod , UINT uiSize)
{
    int iRet ;

    __try
    {
        ASSERT(FALSE == IsBadWritePtr(pahMod ,
                                      uiSize * sizeof(HMODULE))) ;

        if (TRUE == IsBadWritePtr(pahMod ,
                                  uiSize * sizeof(HMODULE)))
        {
            iRet = GLMA_BADPARAM ;
            __leave ;
        }

        if (uiSize < g_uiModCount)
        {
            iRet = GLMA_BUFFTOOSMALL ;
            __leave ;
        }

        CopyMemory(pahMod     ,
                   g_ahMod    ,
                   sizeof(HMODULE) * g_uiModCount) ;

        iRet = GLMA_SUCCESS ;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        iRet = GLMA_FAILURE ;
    }

    return (iRet) ;
}

LONG __stdcall CrashHandlerExceptionFilter(EXCEPTION_POINTERS* pExPtrs)
{
    LONG lRet = EXCEPTION_CONTINUE_SEARCH ;

    __try
    {

        if (NULL not_eq g_pfnCallBack)
        {

            // The symbol engine has to be initialized here so that
            //  I can look up the base module information for the
            //  crash address as well as just get the symbol engine
            //  ready.
            InitSymEng() ;

            // Check the g_ahMod list.
            BOOL bCallIt = FALSE ;

            if (0 == g_uiModCount)
            {
                bCallIt = TRUE ;
            }
            else
            {
                HINSTANCE hBaseAddr = (HINSTANCE)
                                      SymGetModuleBase((HANDLE)GetCurrentProcessId() ,
                                                       (DWORD)pExPtrs->
                                                       ExceptionRecord->
                                                       ExceptionAddress);

                if (NULL not_eq hBaseAddr)
                {
                    for (UINT i = 0 ; i < g_uiModCount ; i ++)
                    {
                        if (hBaseAddr == g_ahMod[ i ])
                        {
                            bCallIt = TRUE ;
                            break ;
                        }
                    }
                }
            }

            if (TRUE == bCallIt)
            {
                // Check that the filter function still exists in memory
                //  before I call it.  The user might have forgotten to
                //  unregister and the filter function is invalid
                //  because it got unloaded.  Of course, if something
                //  loaded back into the same address, there is not much
                //  I can do.
                if (FALSE == IsBadCodePtr((FARPROC)g_pfnCallBack))
                {
                    lRet = g_pfnCallBack(pExPtrs) ;
                }
            }
            else
            {
                // Call the previous filter but only after it checks
                //  out.  I am just being a little paranoid.
                if (FALSE == IsBadCodePtr((FARPROC)g_pfnOrigFilt))
                {
                    lRet = g_pfnOrigFilt(pExPtrs) ;
                }
            }

            CleanupSymEng() ;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        lRet = EXCEPTION_CONTINUE_SEARCH ;
    }

    return (lRet) ;
}

/*//////////////////////////////////////////////////////////////////////
         EXCEPTION_POINTER Translation Functions Implementation
//////////////////////////////////////////////////////////////////////*/

LPCTSTR __stdcall GetFaultReason(EXCEPTION_POINTERS * pExPtrs)
{
    ASSERT(FALSE == IsBadReadPtr(pExPtrs ,
                                 sizeof(EXCEPTION_POINTERS))) ;

    if (TRUE == IsBadReadPtr(pExPtrs ,
                             sizeof(EXCEPTION_POINTERS)))
    {
        TRACE0("Bad parameter to GetFaultReasonA\n") ;
        return (NULL) ;
    }

    // The value that holds the return.
    LPCTSTR szRet ;

    __try
    {

        // Initialize the symbol engine in case it is not initialized.
        InitSymEng() ;

        // The current position in the buffer.
        int iCurr = 0 ;
        // A temp value holder.  This is to keep the stack usage to a
        //  minimum.
        DWORD dwTemp ;

        iCurr += BSUGetModuleBaseName(GetCurrentProcess() ,
                                      NULL                  ,
                                      g_szBuff              ,
                                      BUFF_SIZE) ;

        iCurr += wsprintf(g_szBuff + iCurr , _T(" caused a ")) ;

        dwTemp = (DWORD)
                 ConvertSimpleException(pExPtrs->ExceptionRecord->
                                        ExceptionCode);

        if (NULL not_eq dwTemp)
        {
            iCurr += wsprintf(g_szBuff + iCurr ,
                              _T("%s")      ,
                              dwTemp) ;
        }
        else
        {
            iCurr += (FormatMessage(FORMAT_MESSAGE_IGNORE_INSERTS |
                                    FORMAT_MESSAGE_FROM_HMODULE,
                                    GetModuleHandle(_T("NTDLL.DLL")) ,
                                    pExPtrs->ExceptionRecord->
                                    ExceptionCode,
                                    0                                 ,
                                    g_szBuff + iCurr                  ,
                                    BUFF_SIZE ,
                                    0)
                      * sizeof(TCHAR)) ;
        }

        ASSERT(iCurr < BUFF_SIZE) ;

        iCurr += wsprintf(g_szBuff + iCurr , _T(" in module ")) ;

        dwTemp =
            SymGetModuleBase((HANDLE)GetCurrentProcessId() ,
                             (DWORD)pExPtrs->ExceptionRecord->
                             ExceptionAddress) ;
        ASSERT(NULL not_eq dwTemp) ;

        if (NULL == dwTemp)
        {
            iCurr += wsprintf(g_szBuff + iCurr , _T("<UNKNOWN>"));
        }
        else
        {
            iCurr += BSUGetModuleBaseName(GetCurrentProcess() ,
                                          (HINSTANCE)dwTemp     ,
                                          g_szBuff + iCurr      ,
                                          BUFF_SIZE - iCurr) ;
        }

#ifdef _ALPHA_
        iCurr += wsprintf(g_szBuff + iCurr    ,
                          _T(" at %08X")   ,
                          pExPtrs->ExceptionRecord->ExceptionAddress);
#else
        iCurr += wsprintf(g_szBuff + iCurr                ,
                          _T(" at %04X:%08X")          ,
                          pExPtrs->ContextRecord->SegCs   ,
                          pExPtrs->ExceptionRecord->ExceptionAddress);
#endif

        ASSERT(iCurr < BUFF_SIZE) ;

        // Start looking up the exception address.
        //lint -e545
        PIMAGEHLP_SYMBOL pSym = (PIMAGEHLP_SYMBOL)&g_stSymbol ;
        //lint +e545
        FillMemory(pSym , NULL , SYM_BUFF_SIZE) ;
        pSym->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL) ;
        pSym->MaxNameLength = SYM_BUFF_SIZE - sizeof(IMAGEHLP_SYMBOL);

        DWORD dwDisp ;

        if (TRUE ==
            SymGetSymFromAddr((HANDLE)GetCurrentProcessId()     ,
                              (DWORD)pExPtrs->ExceptionRecord->
                              ExceptionAddress ,
                              &dwDisp                             ,
                              pSym))
        {
            iCurr += wsprintf(g_szBuff + iCurr , _T(", ")) ;

            // Copy no more than there is room for.
            dwTemp = lstrlen(pSym->Name) ;

            if ((int)dwTemp > (BUFF_SIZE - iCurr - 20))
            {
                lstrcpyn(g_szBuff + iCurr      ,
                         pSym->Name            ,
                         BUFF_SIZE - iCurr - 1) ;
                // Gotta leave now.
                szRet = g_szBuff ;
                __leave ;
            }
            else
            {
                if (dwDisp > 0)
                {
                    iCurr += wsprintf(g_szBuff + iCurr         ,
                                      _T("%s()+%d byte(s)") ,
                                      pSym->Name               ,
                                      dwDisp) ;
                }
                else
                {
                    iCurr += wsprintf(g_szBuff + iCurr ,
                                      _T("%s ")     ,
                                      pSym->Name) ;
                }
            }
        }
        else
        {
            // If the symbol was not found, the source and line will not
            //  be found either so leave now.
            szRet = g_szBuff ;
            __leave ;
        }

        ASSERT(iCurr < BUFF_SIZE) ;

        // Do the source and line lookup.
        ZeroMemory(&g_stLine , sizeof(IMAGEHLP_LINE)) ;
        g_stLine.SizeOfStruct = sizeof(IMAGEHLP_LINE) ;

        if (TRUE ==
            InternalSymGetLineFromAddr((HANDLE)
                                       GetCurrentProcessId()    ,
                                       (DWORD)pExPtrs->
                                       ExceptionRecord->
                                       ExceptionAddress ,
                                       &dwDisp                      ,
                                       &g_stLine))
        {
            iCurr += wsprintf(g_szBuff + iCurr , _T(", ")) ;

            // Copy no more than there is room for.
            dwTemp = lstrlen(g_stLine.FileName) ;

            if ((int)dwTemp > (BUFF_SIZE - iCurr - 25))
            {
                lstrcpyn(g_szBuff + iCurr      ,
                         g_stLine.FileName     ,
                         BUFF_SIZE - iCurr - 1) ;
                // Gotta leave now.
                szRet = g_szBuff ;
                __leave ;
            }
            else
            {
                if (dwDisp > 0)
                {
                    iCurr += wsprintf(g_szBuff + iCurr              ,
                                      _T("%s, line %d+%d byte(s)"),
                                      g_stLine.FileName             ,
                                      g_stLine.LineNumber           ,
                                      dwDisp);
                }
                else
                {
                    iCurr += wsprintf(g_szBuff + iCurr     ,
                                      _T("%s, line %d") ,
                                      g_stLine.FileName    ,
                                      g_stLine.LineNumber) ;
                }
            }
        }

        szRet = g_szBuff ;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        ASSERT(FALSE) ;
        szRet = NULL ;
    }

    return (szRet) ;
}

BOOL __stdcall GetFaultReasonVB(EXCEPTION_POINTERS * pExPtrs ,
                                LPTSTR               szBuff  ,
                                UINT                 uiSize)
{
    ASSERT(FALSE == IsBadWritePtr(szBuff , uiSize)) ;

    if (TRUE == IsBadWritePtr(szBuff , uiSize))
    {
        return (FALSE) ;
    }

    LPCTSTR szRet ;

    __try
    {

        szRet = GetFaultReason(pExPtrs) ;

        ASSERT(NULL not_eq szRet) ;

        if (NULL == szRet)
        {
            __leave ;
        }

        lstrcpyn(szBuff   ,
                 szRet    ,
                 min((UINT)lstrlen(szRet) + 1, uiSize)) ;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        szRet = NULL ;
    }

    return (NULL not_eq szRet) ;
}


LPCTSTR BUGSUTIL_DLLINTERFACE __stdcall
GetFirstStackTraceString(DWORD                dwOpts  ,
                         EXCEPTION_POINTERS * pExPtrs)
{
    // All of the error checking is in the InternalGetStackTraceString
    //  function.

    // Initialize the STACKFRAME structure.
    ZeroMemory(&g_stFrame , sizeof(STACKFRAME)) ;

#ifdef _X86_
    g_stFrame.AddrPC.Offset       = pExPtrs->ContextRecord->Eip ;
    g_stFrame.AddrPC.Mode         = AddrModeFlat                ;
    g_stFrame.AddrStack.Offset    = pExPtrs->ContextRecord->Esp ;
    g_stFrame.AddrStack.Mode      = AddrModeFlat                ;
    g_stFrame.AddrFrame.Offset    = pExPtrs->ContextRecord->Ebp ;
    g_stFrame.AddrFrame.Mode      = AddrModeFlat                ;
#else
    g_stFrame.AddrPC.Offset       = (DWORD)pExPtrs->ContextRecord->Fir ;
    g_stFrame.AddrPC.Mode         = AddrModeFlat ;
    g_stFrame.AddrReturn.Offset   =
        (DWORD)pExPtrs->ContextRecord->IntRa;
    g_stFrame.AddrReturn.Mode     = AddrModeFlat ;
    g_stFrame.AddrStack.Offset    =
        (DWORD)pExPtrs->ContextRecord->IntSp;
    g_stFrame.AddrStack.Mode      = AddrModeFlat ;
    g_stFrame.AddrFrame.Offset    =
        (DWORD)pExPtrs->ContextRecord->IntFp;
    g_stFrame.AddrFrame.Mode      = AddrModeFlat ;
#endif

    return (InternalGetStackTraceString(dwOpts , pExPtrs)) ;
}

LPCTSTR BUGSUTIL_DLLINTERFACE __stdcall
GetNextStackTraceString(DWORD                dwOpts  ,
                        EXCEPTION_POINTERS * pExPtrs)
{
    // All error checking is in InternalGetStackTraceString.
    // Assume that GetFirstStackTraceString has already initialized the
    //  stack frame information.
    return (InternalGetStackTraceString(dwOpts , pExPtrs)) ;
}

BOOL __stdcall CH_ReadProcessMemory(HANDLE                      ,
                                    LPCVOID lpBaseAddress       ,
                                    LPVOID  lpBuffer            ,
                                    DWORD   nSize               ,
                                    LPDWORD lpNumberOfBytesRead)
{
    return (ReadProcessMemory(GetCurrentProcess() ,
                              lpBaseAddress         ,
                              lpBuffer              ,
                              nSize                 ,
                              lpNumberOfBytesRead)) ;
}

// The internal function that does all the stack walking.
LPCTSTR __stdcall
InternalGetStackTraceString(DWORD                dwOpts  ,
                            EXCEPTION_POINTERS * pExPtrs)
{

    ASSERT(FALSE == IsBadReadPtr(pExPtrs                      ,
                                 sizeof(EXCEPTION_POINTERS)));

    if (TRUE == IsBadReadPtr(pExPtrs                      ,
                             sizeof(EXCEPTION_POINTERS)))
    {
        TRACE0("GetStackTraceString - invalid pExPtrs\n") ;
        return (NULL) ;
    }

    // The value that is returned.
    LPCTSTR szRet ;
    // A temporary for all to use.  This saves stack space.
    DWORD dwTemp ;

    __try
    {
        // Initialize the symbol engine in case it is not initialized.
        InitSymEng() ;

#ifdef _ALPHA_
#define CH_MACHINE IMAGE_FILE_MACHINE_ALPHA
#else
#define CH_MACHINE IMAGE_FILE_MACHINE_I386
#endif
        // Note:  If the source and line functions are used, then
        //        StackWalk can access violate.
        BOOL bSWRet = StackWalk(CH_MACHINE                        ,
                                (HANDLE)GetCurrentProcessId()   ,
                                GetCurrentThread()              ,
                                &g_stFrame                        ,
                                pExPtrs->ContextRecord            ,
                                (PREAD_PROCESS_MEMORY_ROUTINE)
                                CH_ReadProcessMemory ,
                                SymFunctionTableAccess            ,
                                SymGetModuleBase                  ,
                                NULL) ;

        if ((FALSE == bSWRet) or (0 == g_stFrame.AddrFrame.Offset))
        {
            szRet = NULL ;
            __leave ;
        }

        int iCurr = 0 ;

        // At a minimum, put the address in.
#ifdef _ALPHA_
        iCurr += wsprintf(g_szBuff + iCurr        ,
                          _T("0x%08X")         ,
                          g_stFrame.AddrPC.Offset) ;
#else
        iCurr += wsprintf(g_szBuff + iCurr              ,
                          _T("%04X:%08X")            ,
                          pExPtrs->ContextRecord->SegCs ,
                          g_stFrame.AddrPC.Offset) ;
#endif

        // Do the parameters?
        if (GSTSO_PARAMS == (dwOpts bitand GSTSO_PARAMS))
        {
            iCurr += wsprintf(g_szBuff + iCurr          ,
                              _T(" (0x%08X 0x%08X "\
                                 "0x%08X 0x%08X)") ,
                              g_stFrame.Params[ 0 ]     ,
                              g_stFrame.Params[ 1 ]     ,
                              g_stFrame.Params[ 2 ]     ,
                              g_stFrame.Params[ 3 ]) ;
        }

        if (GSTSO_MODULE == (dwOpts bitand GSTSO_MODULE))
        {
            iCurr += wsprintf(g_szBuff + iCurr  , _T(" ")) ;

            dwTemp = SymGetModuleBase((HANDLE)GetCurrentProcessId(),
                                      g_stFrame.AddrPC.Offset);

            ASSERT(NULL not_eq dwTemp) ;

            if (NULL == dwTemp)
            {
                iCurr += wsprintf(g_szBuff + iCurr  ,
                                  _T("<UNKNOWN>")) ;
            }
            else
            {
                iCurr += BSUGetModuleBaseName(GetCurrentProcess() ,
                                              (HINSTANCE)dwTemp     ,
                                              g_szBuff + iCurr      ,
                                              BUFF_SIZE - iCurr);
            }
        }

        ASSERT(iCurr < BUFF_SIZE) ;
        DWORD dwDisp ;

        if (GSTSO_SYMBOL == (dwOpts bitand GSTSO_SYMBOL))
        {

            // Start looking up the exception address.
            //lint -e545
            PIMAGEHLP_SYMBOL pSym = (PIMAGEHLP_SYMBOL)&g_stSymbol ;
            //lint +e545
            ZeroMemory(pSym , SYM_BUFF_SIZE) ;
            pSym->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL) ;
            pSym->MaxNameLength = SYM_BUFF_SIZE -
                                  sizeof(IMAGEHLP_SYMBOL) ;

            if (TRUE ==
                SymGetSymFromAddr((HANDLE)GetCurrentProcessId() ,
                                  g_stFrame.AddrPC.Offset         ,
                                  &dwDisp                         ,
                                  pSym))
            {
                iCurr += wsprintf(g_szBuff + iCurr , _T(", ")) ;

                // Copy no more than there is room for.
                dwTemp = lstrlen(pSym->Name) ;

                if (dwTemp > (DWORD)(BUFF_SIZE - iCurr - 20))
                {
                    lstrcpyn(g_szBuff + iCurr      ,
                             pSym->Name            ,
                             BUFF_SIZE - iCurr - 1) ;
                    // Gotta leave now.
                    szRet = g_szBuff ;
                    __leave ;
                }
                else
                {
                    if (dwDisp > 0)
                    {
                        iCurr += wsprintf(g_szBuff + iCurr         ,
                                          _T("%s()+%d byte(s)") ,
                                          pSym->Name               ,
                                          dwDisp);
                    }
                    else
                    {
                        iCurr += wsprintf(g_szBuff + iCurr ,
                                          _T("%s")      ,
                                          pSym->Name) ;
                    }
                }
            }
            else
            {
                // If the symbol was not found, the source and line will
                // not be found either so leave now.
                szRet = g_szBuff ;
                __leave ;
            }

        }

        if (GSTSO_SRCLINE == (dwOpts bitand GSTSO_SRCLINE))
        {
            ZeroMemory(&g_stLine , sizeof(IMAGEHLP_LINE)) ;
            g_stLine.SizeOfStruct = sizeof(IMAGEHLP_LINE) ;

            if (TRUE ==
                InternalSymGetLineFromAddr((HANDLE)
                                           GetCurrentProcessId(),
                                           g_stFrame.AddrPC.Offset,
                                           &dwDisp                ,
                                           &g_stLine))
            {
                iCurr += wsprintf(g_szBuff + iCurr , _T(", ")) ;

                // Copy no more than there is room for.
                dwTemp = lstrlen(g_stLine.FileName) ;

                if (dwTemp > (DWORD)(BUFF_SIZE - iCurr - 25))
                {
                    lstrcpyn(g_szBuff + iCurr      ,
                             g_stLine.FileName     ,
                             BUFF_SIZE - iCurr - 1) ;
                    // Gotta leave now.
                    szRet = g_szBuff ;
                    __leave ;
                }
                else
                {
                    if (dwDisp > 0)
                    {
                        iCurr += wsprintf(g_szBuff + iCurr             ,
                                          _T("%s, line %d+%d byte(s)"),
                                          g_stLine.FileName            ,
                                          g_stLine.LineNumber          ,
                                          dwDisp);
                    }
                    else
                    {
                        iCurr += wsprintf(g_szBuff + iCurr     ,
                                          _T("%s, line %d") ,
                                          g_stLine.FileName    ,
                                          g_stLine.LineNumber) ;
                    }
                }
            }
        }

        szRet = g_szBuff ;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        ASSERT(FALSE) ;
        szRet = NULL ;
    }

    return (szRet) ;
}

BOOL __stdcall
GetFirstStackTraceStringVB(DWORD                dwOpts  ,
                           EXCEPTION_POINTERS * pExPtrs ,
                           LPTSTR               szBuff  ,
                           UINT                 uiSize)
{
    ASSERT(FALSE == IsBadWritePtr(szBuff , uiSize)) ;

    if (TRUE == IsBadWritePtr(szBuff , uiSize))
    {
        return (FALSE) ;
    }

    LPCTSTR szRet ;

    __try
    {
        szRet = GetFirstStackTraceString(dwOpts , pExPtrs) ;

        if (NULL == szRet)
        {
            __leave ;
        }

        lstrcpyn(szBuff   ,
                 szRet    ,
                 min((UINT)lstrlen(szRet) + 1 , uiSize)) ;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        szRet = NULL ;
    }

    return (NULL not_eq szRet) ;
}

BOOL __stdcall
GetNextStackTraceStringVB(DWORD                dwOpts  ,
                          EXCEPTION_POINTERS * pExPtrs ,
                          LPTSTR               szBuff  ,
                          UINT                 uiSize)
{
    ASSERT(FALSE == IsBadWritePtr(szBuff , uiSize)) ;

    if (TRUE == IsBadWritePtr(szBuff , uiSize))
    {
        return (FALSE) ;
    }

    LPCTSTR szRet ;

    __try
    {
        szRet = GetNextStackTraceString(dwOpts , pExPtrs) ;

        if (NULL == szRet)
        {
            __leave ;
        }

        lstrcpyn(szBuff   ,
                 szRet    ,
                 min((UINT)lstrlen(szRet) + 1 , uiSize)) ;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        szRet = NULL ;
    }

    return (NULL not_eq szRet) ;
}

LPCTSTR __stdcall GetRegisterString(EXCEPTION_POINTERS * pExPtrs)
{
    // Check the parameter.
    ASSERT(FALSE == IsBadReadPtr(pExPtrs                      ,
                                 sizeof(EXCEPTION_POINTERS))) ;

    if (TRUE == IsBadReadPtr(pExPtrs                      ,
                             sizeof(EXCEPTION_POINTERS)))
    {
        TRACE0("GetRegisterString - invalid pExPtrs\n") ;
        return (NULL) ;
    }

#ifdef _ALPHA_
    // Do the ALPHA ones if needed.
    ASSERT(FALSE) ;
#else
    // This puts 48 bytes on the stack.  This could be a problem when
    //  the stack is blown.
    wsprintf(g_szBuff ,
             _T("EAX=%08X  EBX=%08X  ECX=%08X  EDX=%08X  ESI=%08X\r\n"\
                "EDI=%08X  EBP=%08X  ESP=%08X  EIP=%08X  FLG=%08X\r\n"\
                "CS=%04X   DS=%04X  SS=%04X  ES=%04X   "\
                "FS=%04X  GS=%04X") ,
             pExPtrs->ContextRecord->Eax      ,
             pExPtrs->ContextRecord->Ebx      ,
             pExPtrs->ContextRecord->Ecx      ,
             pExPtrs->ContextRecord->Edx      ,
             pExPtrs->ContextRecord->Esi      ,
             pExPtrs->ContextRecord->Edi      ,
             pExPtrs->ContextRecord->Ebp      ,
             pExPtrs->ContextRecord->Esp      ,
             pExPtrs->ContextRecord->Eip      ,
             pExPtrs->ContextRecord->EFlags   ,
             pExPtrs->ContextRecord->SegCs    ,
             pExPtrs->ContextRecord->SegDs    ,
             pExPtrs->ContextRecord->SegSs    ,
             pExPtrs->ContextRecord->SegEs    ,
             pExPtrs->ContextRecord->SegFs    ,
             pExPtrs->ContextRecord->SegGs) ;

#endif
    return (g_szBuff) ;

}

BOOL __stdcall GetRegisterStringVB(EXCEPTION_POINTERS * pExPtrs ,
                                   LPTSTR               szBuff  ,
                                   UINT                 uiSize)
{
    ASSERT(FALSE == IsBadWritePtr(szBuff , uiSize)) ;

    if (TRUE == IsBadWritePtr(szBuff , uiSize))
    {
        return (FALSE) ;
    }

    LPCTSTR szRet ;

    __try
    {
        szRet = GetRegisterString(pExPtrs) ;

        if (NULL == szRet)
        {
            __leave ;
        }

        lstrcpyn(szBuff   ,
                 szRet    ,
                 min((UINT)lstrlen(szRet) + 1 , uiSize)) ;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        szRet = NULL ;
    }

    return (NULL not_eq szRet) ;

}

//lint -e527
LPCTSTR ConvertSimpleException(DWORD dwExcept)
{
    switch (dwExcept)
    {
        case EXCEPTION_ACCESS_VIOLATION         :
            return (_T("EXCEPTION_ACCESS_VIOLATION")) ;
            break ;

        case EXCEPTION_DATATYPE_MISALIGNMENT    :
            return (_T("EXCEPTION_DATATYPE_MISALIGNMENT")) ;
            break ;

        case EXCEPTION_BREAKPOINT               :
            return (_T("EXCEPTION_BREAKPOINT")) ;
            break ;

        case EXCEPTION_SINGLE_STEP              :
            return (_T("EXCEPTION_SINGLE_STEP")) ;
            break ;

        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED    :
            return (_T("EXCEPTION_ARRAY_BOUNDS_EXCEEDED")) ;
            break ;

        case EXCEPTION_FLT_DENORMAL_OPERAND     :
            return (_T("EXCEPTION_FLT_DENORMAL_OPERAND")) ;
            break ;

        case EXCEPTION_FLT_DIVIDE_BY_ZERO       :
            return (_T("EXCEPTION_FLT_DIVIDE_BY_ZERO")) ;
            break ;

        case EXCEPTION_FLT_INEXACT_RESULT       :
            return (_T("EXCEPTION_FLT_INEXACT_RESULT")) ;
            break ;

        case EXCEPTION_FLT_INVALID_OPERATION    :
            return (_T("EXCEPTION_FLT_INVALID_OPERATION")) ;
            break ;

        case EXCEPTION_FLT_OVERFLOW             :
            return (_T("EXCEPTION_FLT_OVERFLOW")) ;
            break ;

        case EXCEPTION_FLT_STACK_CHECK          :
            return (_T("EXCEPTION_FLT_STACK_CHECK")) ;
            break ;

        case EXCEPTION_FLT_UNDERFLOW            :
            return (_T("EXCEPTION_FLT_UNDERFLOW")) ;
            break ;

        case EXCEPTION_INT_DIVIDE_BY_ZERO       :
            return (_T("EXCEPTION_INT_DIVIDE_BY_ZERO")) ;
            break ;

        case EXCEPTION_INT_OVERFLOW             :
            return (_T("EXCEPTION_INT_OVERFLOW")) ;
            break ;

        case EXCEPTION_PRIV_INSTRUCTION         :
            return (_T("EXCEPTION_PRIV_INSTRUCTION")) ;
            break ;

        case EXCEPTION_IN_PAGE_ERROR            :
            return (_T("EXCEPTION_IN_PAGE_ERROR")) ;
            break ;

        case EXCEPTION_ILLEGAL_INSTRUCTION      :
            return (_T("EXCEPTION_ILLEGAL_INSTRUCTION")) ;
            break ;

        case EXCEPTION_NONCONTINUABLE_EXCEPTION :
            return (_T("EXCEPTION_NONCONTINUABLE_EXCEPTION")) ;
            break ;

        case EXCEPTION_STACK_OVERFLOW           :
            return (_T("EXCEPTION_STACK_OVERFLOW")) ;
            break ;

        case EXCEPTION_INVALID_DISPOSITION      :
            return (_T("EXCEPTION_INVALID_DISPOSITION")) ;
            break ;

        case EXCEPTION_GUARD_PAGE               :
            return (_T("EXCEPTION_GUARD_PAGE")) ;
            break ;

        case EXCEPTION_INVALID_HANDLE           :
            return (_T("EXCEPTION_INVALID_HANDLE")) ;
            break ;

        default :
            return (NULL) ;
            break ;
    }
}
//lint +e527


BOOL InternalSymGetLineFromAddr(IN  HANDLE          hProcess        ,
                                IN  DWORD           dwAddr          ,
                                OUT PDWORD          pdwDisplacement ,
                                OUT PIMAGEHLP_LINE  Line)
{
    // Have I already done the GetProcAddress?
    if (FALSE == g_bLookedForSymFuncs)
    {
        g_bLookedForSymFuncs = TRUE ;
        g_pfnSymGetLineFromAddr = (PFNSYMGETLINEFROMADDR)
                                  GetProcAddress(GetModuleHandle(_T("IMAGEHLP.DLL")),
                                          "SymGetLineFromAddr");
    }

    if (NULL not_eq g_pfnSymGetLineFromAddr)
    {
#ifdef WORK_AROUND_SRCLINE_BUG

        // The problem is that the symbol engine only finds those source
        //  line addresses (after the first lookup) that fall exactly on
        //  a zero displacement.  I will walk backwards 100 bytes to
        //  find the line and return the proper displacement.
        DWORD dwTempDis = 0 ;

        while (FALSE == g_pfnSymGetLineFromAddr(hProcess        ,
                                                dwAddr -
                                                dwTempDis      ,
                                                pdwDisplacement ,
                                                Line))
        {
            dwTempDis += 1 ;

            if (100 == dwTempDis)
            {
                return (FALSE) ;
            }
        }

        // It was found and the source line information is correct so
        //  change the displacement if it was looked up multiple times.
        if (0 not_eq dwTempDis)
        {
            *pdwDisplacement = dwTempDis ;
        }

        return (TRUE) ;

#else  // WORK_AROUND_SRCLINE_BUG
        return (g_pfnSymGetLineFromAddr(hProcess         ,
                                        dwAddr           ,
                                        pdwDisplacement  ,
                                        Line)) ;
#endif
    }

    return (FALSE) ;
}

// Initializes the symbol engine if needed.
void InitSymEng(void)
{
    if (FALSE == g_bSymEngInit)
    {
        // Set up the symbol engine.
        DWORD dwOpts = SymGetOptions() ;

        // Turn on load lines.
        SymSetOptions(dwOpts                |
                      SYMOPT_LOAD_LINES) ;

        // Initialize the symbol engine.
        VERIFY(SymInitialize((HANDLE)GetCurrentProcessId() ,
                             NULL                            ,
                             FALSE)) ;
        UINT uiCount ;
        // Find out how many modules there are.
        VERIFY(GetLoadedModules(GetCurrentProcessId() ,
                                0                       ,
                                NULL                    ,
                                &uiCount)) ;
        // Allocate something big enough to hold the list.
        HMODULE * paMods = new HMODULE[ uiCount ] ;

        // Get the list for real.
        if (FALSE == GetLoadedModules(GetCurrentProcessId() ,
                                      uiCount                 ,
                                      paMods                  ,
                                      &uiCount))
        {
            ASSERT(FALSE) ;
            // Free the memory that I allocated earlier.
            delete [] paMods ;
            // There's not much I can do here...
            g_bSymEngInit = FALSE ;
            return ;
        }

        // The module filename.
        TCHAR szModName [ MAX_PATH ] ;

        for (UINT uiCurr = 0 ; uiCurr < uiCount ; uiCurr++)
        {
            // Get the module's filename.
            VERIFY(GetModuleFileName(paMods[ uiCurr ]     ,
                                     szModName            ,
                                     sizeof(szModName))) ;

            // In order to get the symbol engine to work outside a
            //  debugger, it needs a handle to the image.  Yes, this
            //  will leak but the OS will close it down when the process
            //  ends.
            HANDLE hFile = CreateFile(szModName       ,
                                      GENERIC_READ    ,
                                      FILE_SHARE_READ ,
                                      NULL            ,
                                      OPEN_EXISTING   ,
                                      0               ,
                                      0) ;

            VERIFY(SymLoadModule((HANDLE)GetCurrentProcessId() ,
                                 hFile                           ,
                                 szModName                       ,
                                 NULL                            ,
                                 (DWORD)paMods[ uiCurr ]         ,
                                 0));
        }

        delete [] paMods ;
    }

    g_bSymEngInit = TRUE ;
}

// Cleans up the symbol engine if needed.
void CleanupSymEng(void)
{
    if (TRUE == g_bSymEngInit)
    {
        VERIFY(SymCleanup((HANDLE)GetCurrentProcessId())) ;
    }

    g_bSymEngInit = FALSE ;
}



//Wombat778 5-01-04 New Minidump code

/*----------------------------------------------------------------------
"Debugging Applications" (Microsoft Press)
Copyright (c) 1997-2001 John Robbins -- All rights reserved.
----------------------------------------------------------------------*/

/*//////////////////////////////////////////////////////////////////////
                                Includes
//////////////////////////////////////////////////////////////////////*/


// The project internal header file.

// The minidump definitions.
#include "minidump.h"
#include "DBGHELP_MINDUMP.h"
#include <process.h>

/*//////////////////////////////////////////////////////////////////////
          File Specific Defines, Typdefs, Constants, and Structs
//////////////////////////////////////////////////////////////////////*/
// The typedef for the MiniDumpWriteDump function.
typedef BOOL
(WINAPI * PFNMINIDUMPWRITEDUMP)(
    IN HANDLE hProcess,
    IN DWORD ProcessId,
    IN HANDLE hFile,
    IN MINIDUMP_TYPE DumpType,
    IN CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam, OPTIONAL
    IN CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam, OPTIONAL
    IN CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam OPTIONAL) ;

// The structure I can package the data necessary to the dump writer
// thread.
typedef struct tag_DUMPTHREADPARAMS
{
    MINIDUMP_TYPE        eType                        ;
    wchar_t *            szFileName                   ;
    DWORD                dwThreadID                   ;
    EXCEPTION_POINTERS * pExceptInfo                  ;
    BSUMDRET             eReturnValue                 ;
    DWORD                dwMiniDumpWriteDumpLastError ;
} DUMPTHREADPARAMS , * LPDUMPTHREADPARAMS ;

// Ye ol' DBGHELP.DLL name.
static const TCHAR * k_DBGHELPDLLNAME = _T("DBGHELP.DLL") ;
// The function name for MiniDumpWriteDump.  Note, this is ANSI as
// that's what GetProcAddress wants.
static const char * k_MINIDUMPWRITEDUMP = "MiniDumpWriteDump" ;

/*//////////////////////////////////////////////////////////////////////
                        File Specific Prototypes
//////////////////////////////////////////////////////////////////////*/
// The dumper function.
unsigned WINAPI DumpThread(LPVOID pData) ;

/*//////////////////////////////////////////////////////////////////////
                          File Specific Globals
//////////////////////////////////////////////////////////////////////*/
// The MiniDumpWriteDump function.  I don't want to link to this
// directly as it requires a version if DBGHELP.DLL that most people
// don't have on their machines.
static PFNMINIDUMPWRITEDUMP g_pfnMDWD = NULL ;

// The last error value for IsMiniDumpFunctionAvailible so I don't have
// to go through doing lookup operations all over again.
static BSUMDRET g_eIMDALastError = eINVALID_ERROR ;

/*//////////////////////////////////////////////////////////////////////
                             Implementation
//////////////////////////////////////////////////////////////////////*/

BOOL BUGSUTIL_DLLINTERFACE __stdcall IsMiniDumpFunctionAvailable(void)
{
    // If this is the first time through, always do it.
    if (NULL == g_pfnMDWD)
    {
        // Find DBGHELP.DLL in memory.
        HINSTANCE hInstDBGHELP = GetModuleHandle(k_DBGHELPDLLNAME) ;

        // BugslayerUtil.DLL has it in memory, but someone might excise
        // this file for their own purposes so try and load it.
        if (NULL == hInstDBGHELP)
        {
            hInstDBGHELP = LoadLibrary(k_DBGHELPDLLNAME) ;
        }

        if (NULL not_eq hInstDBGHELP)
        {
            // At least I have DBGHELP.DLL's handle.  Get the exported
            // function.
            g_pfnMDWD = (PFNMINIDUMPWRITEDUMP)
                        GetProcAddress(hInstDBGHELP , k_MINIDUMPWRITEDUMP) ;


            if (NULL not_eq g_pfnMDWD)
            {
                // It's good so set the last error for this function.
                g_eIMDALastError = eDUMP_SUCCEEDED ;
            }
            else
            {
                // Ain't got the export.
                g_eIMDALastError = eDBGHELP_MISSING_EXPORTS ;
            }

        }
        else
        {
            // Can't find DBGHELP.DLL  Save this for the
            // CreateCurrentProcessMiniDump function to return.
            g_eIMDALastError = eDBGHELP_NOT_FOUND ;
        }
    }

    // If g_pfnMDWD is not NULL, I found it.
    return (NULL not_eq g_pfnMDWD) ;
}

BSUMDRET BUGSUTIL_DLLINTERFACE __stdcall
CreateCurrentProcessMiniDumpA(MINIDUMP_TYPE        eType      ,
                              char *               szFileName ,
                              DWORD                dwThread   ,
                              EXCEPTION_POINTERS * pExceptInfo)
{
    // Check the string parameter because I am paranoid.
    ASSERT(FALSE == IsBadStringPtrA(szFileName , MAX_PATH)) ;

    if (TRUE == IsBadStringPtrA(szFileName , MAX_PATH))
    {
        return (eBAD_PARAM) ;
    }

    // The return value.
    BSUMDRET eRetVal = eDUMP_SUCCEEDED ;

    // Allocate enough space to hold the converted string.
    int iLen = (lstrlenA(szFileName) + 1) * sizeof(wchar_t) ;
    wchar_t * pWideFileName = (wchar_t*)
                              HeapAlloc(GetProcessHeap()         ,
                                        HEAP_GENERATE_EXCEPTIONS |
                                        HEAP_ZERO_MEMORY         ,
                                        iLen);

    int iRet = MultiByteToWideChar(CP_ACP          ,
                                   MB_PRECOMPOSED  ,
                                   szFileName      ,
                                   -1              ,
                                   pWideFileName   ,
                                   iLen) ;
    ASSERT(iRet not_eq 0) ;

    if (iRet not_eq 0)
    {
        // The conversion worked, call the wide function.
        eRetVal = CreateCurrentProcessMiniDumpW(eType         ,
                                                pWideFileName ,
                                                dwThread      ,
                                                pExceptInfo) ;
    }
    else
    {
        eRetVal = eBAD_PARAM ;
    }

    if (NULL not_eq pWideFileName)
    {
        HeapFree(GetProcessHeap() , 0 , pWideFileName) ;
    }

    return (eRetVal) ;
}


BSUMDRET BUGSUTIL_DLLINTERFACE __stdcall
CreateCurrentProcessMiniDumpW(MINIDUMP_TYPE        eType      ,
                              wchar_t *            szFileName ,
                              DWORD                dwThread   ,
                              EXCEPTION_POINTERS * pExceptInfo)
{
    // Check the string parameter because I am paranoid.  I can't check
    // the eType as that might change in the future.
    ASSERT(FALSE == IsBadStringPtrW(szFileName , MAX_PATH)) ;

    if (TRUE == IsBadStringPtrW(szFileName , MAX_PATH))
    {
        return (eBAD_PARAM) ;
    }

    // If an exception pointer blob was passed in.
    if (NULL not_eq pExceptInfo)
    {
        ASSERT(FALSE ==
               IsBadReadPtr(pExceptInfo , sizeof(EXCEPTION_POINTERS)));

        if (TRUE ==
            IsBadReadPtr(pExceptInfo , sizeof(EXCEPTION_POINTERS)))
        {
            return (eBAD_PARAM) ;
        }
    }

    // Have I even tried to get the exported MiniDumpWriteDump function
    // yet?
    if ((NULL == g_pfnMDWD) and (eINVALID_ERROR == g_eIMDALastError))
    {
        if (FALSE == IsMiniDumpFunctionAvailable())
        {
            return (g_eIMDALastError) ;
        }
    }

    // If the MiniDumpWriteDump function pointer is NULL, return
    // whatever was in g_eIMDALastError.
    if (NULL == g_pfnMDWD)
    {
        return (g_eIMDALastError) ;
    }

    // Package up the data for the dump writer thread.
    DUMPTHREADPARAMS stParams ;
    stParams.eReturnValue = eINVALID_ERROR  ;
    stParams.eType        = eType           ;
    stParams.pExceptInfo  = pExceptInfo     ;
    stParams.dwThreadID   = dwThread        ;
    stParams.szFileName   = szFileName      ;
    stParams.dwMiniDumpWriteDumpLastError = ERROR_SUCCESS ;

    // Crank the writer thread.
    unsigned dwTID ;
    HANDLE hThread = (HANDLE)_beginthreadex(NULL        ,
                                            0           ,
                                            DumpThread  ,
                                            &stParams   ,
                                            0           ,
                                            &dwTID) ;
    ASSERT((HANDLE) - 1 not_eq hThread) ;

    if ((HANDLE) - 1 not_eq hThread)
    {
        // The thread is running.  Block until the thread ends.
        WaitForSingleObject(hThread , INFINITE) ;

        // Close the handle.
        VERIFY(CloseHandle(hThread)) ;

    }
    else
    {
        stParams.dwMiniDumpWriteDumpLastError = GetLastError() ;
        stParams.eReturnValue = eDEATH_ERROR ;
    }

    // Set the last error code based so it looks like this thread made
    // the call to MiniDumpWriteDump.
    SetLastError(stParams.dwMiniDumpWriteDumpLastError) ;

    return (stParams.eReturnValue) ;
}


/*----------------------------------------------------------------------
FUNCTION        :   DumpThread
DISCUSSION      :
    The function that does all the work.
PARAMETERS      :
    pData - A pointer to a DUMPTHREADPARAMS structure.
RETURNS         :
    FALSE - There was a problem dumping the data.  The DUMPTHREADPARAMS
            struct contains the problem.
    TRUE  - All OK, Jumpmaster
----------------------------------------------------------------------*/
unsigned WINAPI DumpThread(LPVOID pData)
{
    LPDUMPTHREADPARAMS pParams = (LPDUMPTHREADPARAMS)pData ;

    // Create the file first.
    HANDLE hFile = CreateFileW(pParams->szFileName             ,
                               GENERIC_READ bitor GENERIC_WRITE    ,
                               0                              ,
                               NULL                           ,
                               CREATE_ALWAYS                  ,
                               FILE_ATTRIBUTE_NORMAL          ,
                               NULL) ;
    ASSERT(INVALID_HANDLE_VALUE not_eq hFile) ;

    if (INVALID_HANDLE_VALUE not_eq hFile)
    {
        MINIDUMP_EXCEPTION_INFORMATION   stMDEI ;
        MINIDUMP_EXCEPTION_INFORMATION * pMDEI = NULL ;

        if (NULL not_eq pParams->pExceptInfo)
        {
            stMDEI.ThreadId = pParams->dwThreadID ;
            stMDEI.ExceptionPointers = pParams->pExceptInfo ;
            stMDEI.ClientPointers = TRUE ;
            pMDEI = &stMDEI ;
        }

        // Got the file open.  Write it.
        BOOL bRet = g_pfnMDWD(GetCurrentProcess()   ,
                              GetCurrentProcessId() ,
                              hFile                   ,
                              pParams->eType          ,
                              pMDEI                   ,
                              NULL                    ,
                              NULL) ;
        ASSERT(TRUE == bRet) ;

        if (TRUE == bRet)
        {
            pParams->eReturnValue = eDUMP_SUCCEEDED ;
        }
        else
        {
            // Oops.
            pParams->eReturnValue = eMINIDUMPWRITEDUMP_FAILED ;
        }

        // Close the open file.
        VERIFY(CloseHandle(hFile)) ;
    }
    else
    {
        // Could not open the file
        pParams->eReturnValue = eOPEN_DUMP_FAILED ;
    }

    // Always save the last error value so I can set it in the original
    // thread.
    pParams->dwMiniDumpWriteDumpLastError = GetLastError() ;
    return (eDUMP_SUCCEEDED == pParams->eReturnValue) ;
}
