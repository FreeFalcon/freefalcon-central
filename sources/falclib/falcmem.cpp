#include <stdio.h>

#ifdef USE_HEAPAGENT
#include "SmartHeap\Include\heapagnt.h"
#include "SmartHeap\Include\smrtheap.hpp"

MEM_BOOL MEM_CALLBACK errPrint(MEM_ERROR_INFO *errorInfo);
MEM_ERROR_FN lastErrorFn = NULL;

#endif

#include "debuggr.h"
#include "falcmem.h"

//unsigned MemDefaultPoolFlags = MEM_POOL_SERIALIZE;
#ifdef MEM_DEBUG
const unsigned dbgMemGuardSize = 16;

//#define USE_HEAPAGENT

#endif
#define MAIN_INIT_CHECK_POINT   2
#define MAIN_DEINIT_CHECK_POINT 3

falcon4LeakCheck::falcon4LeakCheck()
{
#ifdef USE_HEAPAGENT
  lastErrorFn = MemSetErrorHandler(errPrint);
//  dbgMemSetStackChecking (DBGMEM_STACK_CHECK_RETURN);
  dbgMemSetGuardSize(dbgMemGuardSize);
  dbgMemSetCheckpoint(MAIN_INIT_CHECK_POINT);
#endif
}

falcon4LeakCheck::~falcon4LeakCheck()
{
#ifdef USE_HEAPAGENT
  dbgMemSetCheckpoint(MAIN_DEINIT_CHECK_POINT);
  if (lastErrorFn)
  {
     dbgMemReportLeakage(NULL,MAIN_INIT_CHECK_POINT,MAIN_DEINIT_CHECK_POINT);
     MemSetErrorHandler(lastErrorFn);
  }
  dbgMemSetDefaultErrorOutput(DBGMEM_OUTPUT_FILE,"leak.log");
  dbgMemReportLeakage(NULL,MAIN_INIT_CHECK_POINT,MAIN_DEINIT_CHECK_POINT);
#endif
}

#ifdef USE_HEAPAGENT
MEM_BOOL MEM_CALLBACK errPrint(MEM_ERROR_INFO *errorInfo)
{
#ifdef _DEBUG
  switch(errorInfo->errorCode)
    {
    case MEM_NO_ERROR:
      MonoPrint("\n");
      break;
    case MEM_INTERNAL_ERROR:
      MonoPrint("MEM_INTERNAL_ERROR\n");
      break;
    case MEM_OUT_OF_MEMORY:
      return 0;
      MonoPrint("MEM_OUT_OF_MEMORY\n");
      break;
    case MEM_BLOCK_TOO_BIG:
      MonoPrint("MEM_BLOCK_TOO_BIG\n");
      break;
    case MEM_ALLOC_ZERO:
      MonoPrint("MEM_ALLOC_ZERO\n");
      break;
    case MEM_RESIZE_FAILED:
      MonoPrint("MEM_RESIZE_FAILED\n");
      break;
    case MEM_LOCK_ERROR:
      MonoPrint("MEM_LOCK_ERROR\n");
      break;
    case MEM_EXCEEDED_CEILING:
      MonoPrint("MEM_EXCEEDED_CEILING\n");
      break;
    case MEM_TOO_MANY_PAGES:
      MonoPrint("MEM_TOO_MANY_PAGES\n");
      break;
    case MEM_TOO_MANY_TASKS:
      MonoPrint("MEM_TOO_MANY_TASKS\n");
      break;
    case MEM_BAD_MEM_POOL:
      MonoPrint("MEM_BAD_MEM_POOL\n");
      break;
    case MEM_BAD_BLOCK:
      MonoPrint("MEM_BAD_BLOCK\n");
      break;
    case MEM_BAD_FREE_BLOCK:
      MonoPrint("MEM_BAD_FREE_BLOCK\n");
      break;
    case MEM_BAD_HANDLE:
      MonoPrint("MEM_BAD_HANDLE\n");
      break;
    case MEM_BAD_POINTER:
      MonoPrint("MEM_BAD_POINTER\n");
      break;
    case MEM_WRONG_TASK:
      MonoPrint("MEM_WRONG_TASK\n");
      break;
    case MEM_NOT_FIXED_SIZE:
      MonoPrint("MEM_NOT_FIXED_SIZE\n");
      break;
    case MEM_BAD_FLAGS:
      MonoPrint("MEM_BAD_FLAGS\n");
      break;
    case MEM_BAD_BUFFER:
      MonoPrint("MEM_BAD_BUFFER\n");
      break;
    case MEM_DOUBLE_FREE:
      MonoPrint("MEM_DOUBLE_FREE\n");
      break;
    case MEM_UNDERWRITE:
      MonoPrint("MEM_UNDERWRITE\n");
      break;
    case MEM_OVERWRITE:
      MonoPrint("MEM_OVERWRITE\n");
      break;
    case MEM_FREE_BLOCK_WRITE:
      MonoPrint("MEM_FREE_BLOCK_WRITE\n");
      break;
    case MEM_READONLY_MODIFIED:
      MonoPrint("MEM_READONLY_MODIFIED\n");
      break;
    case MEM_NOFREE:
      MonoPrint("MEM_NOFREE\n");
      break;
    case MEM_NOREALLOC:
      MonoPrint("MEM_NOREALLOC\n");
      break;
    case MEM_LEAKAGE:
      if(errorInfo->objectCreationInfo)
        MonoPrint("FILE %s LINE %d SIZE %d LOCATION 0x%p\n",
errorInfo->objectCreationInfo->file, errorInfo->objectCreationInfo->line, errorInfo->objectCreationInfo->argSize, errorInfo->objectCreationInfo->argPtr);
      else
        MonoPrint("FILE %s LINE %d SIZE %d LOCATION 0x%p\n", errorInfo->file, errorInfo->line, errorInfo->argSize, errorInfo->argPtr);
      return 1;
    case MEM_FREE_BLOCK_READ:
      MonoPrint("MEM_FREE_BLOCK_READ\n");
      break;
    case MEM_UNINITIALIZED_READ:
      MonoPrint("MEM_UNINITIALIZED_READ\n");
      break;
    case MEM_UNINITIALIZED_WRITE:
      MonoPrint("MEM_UNINITIALIZED_WRITE\n");
      break;
    case MEM_OUT_OF_BOUNDS_READ:
      MonoPrint("MEM_OUT_OF_BOUNDS_READ\n");
      break;
    case MEM_UNDERWRITE_STACK:
      MonoPrint("MEM_UNDERWRITE_STACK\n");
      break;
    case MEM_OVERWRITE_STACK:
      MonoPrint("MEM_OVERWRITE_STACK\n");
      break;
    case MEM_FREE_STACK_READ:
      MonoPrint("MEM_FREE_STACK_READ\n");
      break;
    case MEM_UNINITIALIZED_READ_STACK:
      MonoPrint("MEM_UNINITIALIZED_READ_STACK\n");
      break;
    case MEM_UNINITIALIZED_WRITE_STACK:
      MonoPrint("MEM_UNINITIALIZED_WRITE_STACK\n");
      break;
    case MEM_OUT_OF_BOUNDS_READ_STACK:
      MonoPrint("MEM_OUT_OF_BOUNDS_READ_STACK\n");
      break;
    case MEM_LASTOK:
      MonoPrint("MEM_LASTOK\n");
      break;
    case MEM_BREAKPOINT:
      MonoPrint("MEM_BREAKPOINT\n");
      break;
    case MEM_ERROR_CODE_COUNT:
    MonoPrint("MEM_ERROR_CODE_COUNT\n");
      break;
    }
#endif
#if 0
  errorInfo->errorCode;         /* error code identifying type of error      */
  errorInfo->pool;              /* pool in which error occurred, if known    */

  /* all fields below this are valid only for debugging lib                 */
  /* the following seven fields identify the call where error detected   */
  errorInfo->errorAPI;          /* fn ID of entry-point where error detected */
  errorInfo->argPool;           /* memory pool parameter, if applicable      */
  errorInfo->argPtr;            /* memory pointer parameter, if applicable   */
  errorInfo->argBuf;            /* result buffer parameter, if applicable    */
  errorInfo->argHandle;         /* memory handle parameter, if applicable    */
  errorInfo->argSize;           /* size parameter, if applicable             */
  errorInfo->argCount;          /* count parameter, if applicable            */
  errorInfo->argFlags;          /* flags parameter, if applicable            */

  /* the following two fields identify the app source file and line      */
  errorInfo->file;              /* app source file containing above call     */
  errorInfo->line;              /* source line in above file                 */

  /* the following two fields identify call instance of error detection  */
  errorInfo->allocCount; /* enumeration of allocation since 1st alloc */
  errorInfo->passCount;  /* enumeration of call at at above file/line */
  errorInfo->checkpoint;      /* group with which call has been tagged     */
   
  /* the following fields, if non-NULL, points to the address where an
     overwrite was detected and another MEM_ERROR_INFO structure
     identifying where the corrupted object was first created, if known  */
  errorInfo->errorAlloc;        /* ptr to beginning of alloc related to err */
  errorInfo->corruptAddr;
  errorInfo->objectCreationInfo;

  errorInfo->threadID;          /* ID of thread where error detected */
  errorInfo->pid;               /* ID of process where error detected */

  errorInfo->callStack[0];      /* MEM_MAXCALLSTACK */
#endif
  return 0;
}
#endif