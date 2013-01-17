/* heapagnt.h -- HeapAgent (tm) public C/C++ header file
 *
 * Copyright (C) 1991-1997 Compuware Corporation.
 * All Rights Reserved.
 *
 * No part of this source code may be copied, modified or reproduced
 * in any form without retaining the above copyright notice.
 * This source code, or source code derived from it, may not be redistributed
 * without express written permission of the copyright owner.
 *
 */

#if !defined(_HEAPAGNT_H)
#define _HEAPAGNT_H

#include <limits.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _SMARTHEAP_H

#if !defined(macintosh) && !defined(THINK_C) && !defined(__MWERKS__) \
   && !defined(SHANSI) && UINT_MAX == 0xFFFFu \
   && (defined(_Windows) || defined(_WINDOWS) || defined(__WINDOWS__))
   #define MEM_WIN16
#endif

#if (UINT_MAX == 0xFFFFu) && (defined(MEM_WIN16) \
	|| defined(MSDOS) || defined(__MSDOS__) || defined(__DOS__))
   /* 16-bit X86 */
   #if defined(SYS_DLL)
      #if defined(_MSC_VER) && _MSC_VER <= 600
         #define MEM_ENTRY _export _loadds far pascal
      #else
         #define MEM_ENTRY _export far pascal
      #endif
   #else
      #define MEM_ENTRY far pascal
   #endif
   #ifdef __WATCOMC__
      #define MEM_ENTRY_ANSI __far
   #else
      #define MEM_ENTRY_ANSI far cdecl
   #endif
   #define MEM_FAR far
   #if defined(MEM_WIN16)
      #define MEM_ENTRY2 _export far pascal
   #elif defined(DOS16M) || defined(DOSX286)
      #define MEM_ENTRY2 _export _loadds far pascal
   #endif

#else  /* not 16-bit X86 */

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) \
    || defined(__WIN32__) || defined(__NT__)
   #if defined(_MSC_VER)
      #if defined(_SHI_Pool) && defined(SYS_DLL)
         #define MEM_ENTRY1 __declspec(dllexport)
         #define MEM_ENTRY4 __declspec(dllexport) extern 
      #elif !defined(_SHI_Pool) && (defined(MEM_DEBUG) || defined(MEM_DLL))
         #define MEM_ENTRY1 __declspec(dllimport)
         #if defined(_M_IX86) || defined(_X86_)
            #define MemDefaultPool shi_MemDefaultPool
            #define MEM_ENTRY4 __declspec(dllimport)
         #endif
      #endif
   #endif
   #if !defined(_MSC_VER) || defined(_M_IX86) || defined(_X86_)
     #define MEM_ENTRY __stdcall
   #else
     #define MEM_ENTRY __cdecl  /* for NT/RISC */
   #endif
   #ifndef __WATCOMC__
      #define MEM_ENTRY_ANSI __cdecl
   #endif

#elif defined(__OS2__)
   #if defined(__BORLANDC__) || defined(__WATCOMC__)
      #if defined(SYS_DLL)
         #define MEM_ENTRY __export __syscall
      #else
         #define MEM_ENTRY __syscall
      #endif /* SYS_DLL */
      #ifdef __BORLANDC__
         #define MEM_ENTRY_ANSI __stdcall
      #endif
   #elif defined(__IBMC__) || defined(__IBMCPP__)
      #if defined(SYS_DLL) && 0
         #define MEM_ENTRY _Export _System
      #else
         #define MEM_ENTRY _System
      #endif
      #define MEM_ENTRY_ANSI _Optlink
      #define MEM_ENTRY3 MEM_ENTRY
      #define MEM_CALLBACK MEM_ENTRY3
      #define MEM_ENTRY2
   #endif
#endif /* __OS2__ */

#if defined(__WATCOMC__) && defined(__SW_3S)
   /* Watcom stack calling convention */
#ifndef __OS2__
#ifdef __WINDOWS_386__
   #pragma aux syscall "*_" parm routine [eax ebx ecx edx fs gs] modify [eax];
#else
   #pragma aux syscall "*_" parm routine [eax ebx ecx edx] modify [eax];
#endif
#ifndef MEM_ENTRY
   #define MEM_ENTRY __syscall
#endif
#endif
#endif /* Watcom stack calling convention */

#endif /* end of system-specific declarations */

#ifndef MEM_ENTRY
   #define MEM_ENTRY
#endif
#ifndef MEM_ENTRY1
   #define MEM_ENTRY1
#endif
#ifndef MEM_ENTRY2
   #define MEM_ENTRY2 MEM_ENTRY
#endif
#ifndef MEM_ENTRY3
   #define MEM_ENTRY3
#endif
#ifndef MEM_ENTRY4
   #define MEM_ENTRY4 extern
#endif
#ifndef MEM_CALLBACK
#define MEM_CALLBACK MEM_ENTRY2
#endif
#ifndef MEM_ENTRY_ANSI
   #define MEM_ENTRY_ANSI
#endif
#ifndef MEM_FAR
   #define MEM_FAR
#endif

#ifdef applec
/* Macintosh: Apple MPW C/C++ passes char/short parms as longs (4 bytes),
 * whereas Semantec C/C++ for MPW passes these as words (2 bytes);
 * therefore, canonicalize all integer parms as 'int' for this platform.
 */
   #define MEM_USHORT unsigned
   #define MEM_UCHAR unsigned
#else
   #define MEM_USHORT unsigned short
   #define MEM_UCHAR unsigned char
#endif /* applec */

#endif /* _SMARTHEAP_H */

#define HA_MAJOR_VERSION 3
#define HA_MINOR_VERSION 1
#define HA_UPDATE_LEVEL 2

   
/*** Types ***/

#ifndef MEM_BOOL_DEFINED
#define MEM_BOOL_DEFINED
typedef int MEM_BOOL;
#endif

#ifndef MEM_POOL_DEFINED
#define MEM_POOL_DEFINED
#ifdef _SHI_Pool
  typedef struct _SHI_Pool MEM_FAR *MEM_POOL;
  typedef struct _SHI_MovHandle MEM_FAR *MEM_HANDLE;
#else
  #ifdef THINK_C
    typedef void *MEM_POOL;
    typedef void **MEM_HANDLE;
  #else
    typedef struct _SHI_Pool { int reserved; } MEM_FAR *MEM_POOL;
    typedef struct _SHI_MovHandle { int reserved; } MEM_FAR *MEM_HANDLE;
  #endif
#endif
#endif /* MEM_POOL_DEFINED */
    

/* Error codes: errorCode field of MEM_ERROR_INFO */
#ifndef MEM_ERROR_CODE_DEFINED
#define MEM_ERROR_CODE_DEFINED
typedef enum
{
   MEM_NO_ERROR=0,
   MEM_INTERNAL_ERROR,
   MEM_OUT_OF_MEMORY,
   MEM_BLOCK_TOO_BIG,
   MEM_ALLOC_ZERO,
   MEM_RESIZE_FAILED,
   MEM_LOCK_ERROR,
   MEM_EXCEEDED_CEILING,
   MEM_TOO_MANY_PAGES,
   MEM_TOO_MANY_TASKS,
   MEM_BAD_MEM_POOL,
   MEM_BAD_BLOCK,
   MEM_BAD_FREE_BLOCK,
   MEM_BAD_HANDLE,
   MEM_BAD_POINTER,
   MEM_WRONG_TASK,
   MEM_NOT_FIXED_SIZE,
   MEM_BAD_FLAGS,
   MEM_BAD_BUFFER,
   MEM_DOUBLE_FREE,
   MEM_UNDERWRITE,
   MEM_OVERWRITE, 
   MEM_FREE_BLOCK_WRITE,
   MEM_READONLY_MODIFIED,
   MEM_NOFREE,
   MEM_NOREALLOC,
   MEM_LEAKAGE,
   MEM_FREE_BLOCK_READ,
   MEM_UNINITIALIZED_READ,
   MEM_UNINITIALIZED_WRITE,
   MEM_OUT_OF_BOUNDS_READ,
	MEM_UNDERWRITE_STACK,
	MEM_OVERWRITE_STACK,
	MEM_FREE_STACK_READ,
	MEM_UNINITIALIZED_READ_STACK,
	MEM_UNINITIALIZED_WRITE_STACK,
	MEM_OUT_OF_BOUNDS_READ_STACK,
   MEM_LASTOK,
   MEM_BREAKPOINT,
   MEM_ERROR_CODE_COUNT,
   MEM_ERROR_CODE_INT_MAX = INT_MAX  /* to ensure enum is full int in size */
} MEM_ERROR_CODE;
#endif /* MEM_ERROR_CODE_DEFINED */

/* HeapAgent Entry-Point API identifiers: errorAPI field of MEM_ERROR_INFO */
typedef enum
{
   MEM_NO_API,
   MEM_MEMVERSION,
   MEM_MEMREGISTERTASK,
   MEM_MEMUNREGISTERTASK,
   MEM_MEMPOOLINIT,
   MEM_MEMPOOLINITFS,
   MEM_MEMPOOLFREE,
   MEM_MEMPOOLSETPAGESIZE,
   MEM_MEMPOOLSETBLOCKSIZEFS,
   MEM_MEMPOOLSETFLOOR,
   MEM_MEMPOOLSETCEILING,
   MEM_MEMPOOLPREALLOCATE,
   MEM_MEMPOOLPREALLOCATEHANDLES,
   MEM_MEMPOOLSHRINK,
   MEM_MEMPOOLSIZE,
   MEM_MEMPOOLCOUNT,
   MEM_MEMPOOLINFO,
   MEM_MEMPOOLFIRST,
   MEM_MEMPOOLNEXT,
   MEM_MEMPOOLWALK,
   MEM_MEMPOOLCHECK,
   MEM_MEMALLOC,
   MEM_MEMREALLOC,
   MEM_MEMFREE,
   MEM_MEMLOCK,
   MEM_MEMUNLOCK,
   MEM_MEMFIX,
   MEM_MEMUNFIX,
   MEM_MEMLOCKCOUNT,
   MEM_MEMISMOVEABLE,
   MEM_MEMREFERENCE,
   MEM_MEMHANDLE,
   MEM_MEMSIZE,
   MEM_MEMALLOCPTR,
   MEM_MEMREALLOCPTR,
   MEM_MEMFREEPTR,
   MEM_MEMSIZEPTR,
   MEM_MEMCHECKPTR,
   MEM_MEMALLOCFS,
   MEM_MEMFREEFS,
   MEM_MEM_MALLOC,
   MEM_MEM_CALLOC,
   MEM_MEM_REALLOC,
   MEM_MEM_FREE,
   MEM_NEW,
   MEM_DELETE,
   MEM_DBGMEMPOOLSETCHECKFREQUENCY,
   MEM_DBGMEMPOOLDEFERFREEING,
   MEM_DBGMEMPOOLFREEDEFERRED,
   MEM_DBGMEMPROTECTPTR,
   MEM_DBGMEMREPORTLEAKAGE,
   MEM_MEMPOOLINITNAMEDSHARED,
   MEM_MEMPOOLINITNAMEDSHAREDEX,
   MEM_MEMPOOLATTACHSHARED,
   MEM_DBGMEMPOOLINFO,
   MEM_DBGMEMPTRINFO,
   MEM_DBGMEMSETTINGSINFO,
   MEM_DBGMEMCHECKPTR,
   MEM_DBGMEMPOOLSETNAME,
   MEM_DBGMEMPOOLSETDEFERQUEUELEN,
   MEM_DBGMEMFREEDEFERRED,
   MEM_DBGMEMCHECKALL,
   MEM_DBGMEMBREAKPOINT,
   MEM_MEMPOOLLOCK,
   MEM_MEMPOOLUNLOCK,
	MEM_MEMPOOLSETSMALLBLOCKSIZE,
   MEM_MEMSIZEREQUESTED,
	MEM_MSIZE,
	MEM_EXPAND,
	MEM_GETPROCESSHEAP,
	MEM_GETPROCESSHEAPS,
	MEM_GLOBALALLOC,
	MEM_GLOBALFLAGS,
	MEM_GLOBALFREE,
	MEM_GLOBALHANDLE,
	MEM_GLOBALLOCK,
	MEM_GLOBALREALLOC,
	MEM_GLOBALSIZE,
	MEM_GLOBALUNLOCK,
	MEM_HEAPALLOC,
	MEM_HEAPCOMPACT,
	MEM_HEAPCREATE,
	MEM_HEAPDESTROY,
	MEM_HEAPFREE,
	MEM_HEAPLOCK,
	MEM_HEAPREALLOC,
	MEM_HEAPSIZE,
	MEM_HEAPUNLOCK,
	MEM_HEAPVALIDATE,
	MEM_HEAPWALK,
	MEM_LOCALALLOC,
	MEM_LOCALFLAGS,
	MEM_LOCALFREE,
	MEM_LOCALHANDLE,
	MEM_LOCALLOCK,
	MEM_LOCALREALLOC,
	MEM_LOCALSIZE,
	MEM_LOCALUNLOCK,
	MEM_MEMPOOLINITREGION,
	MEM_TERMINATE,
   MEM_HEAPAGENT,
   MEM_USER_API,
   MEM_API_COUNT,
   MEM_API_INT_MAX = INT_MAX  /* to ensure enum is full int in size */
} MEM_API;

#define MEM_MAXCALLSTACK 16  /* maximum number of call stack frames recorded */

/* Error info, passed to error-handling callback routine */
#ifndef MEM_ERROR_INFO_DEFINED
#define MEM_ERROR_INFO_DEFINED

typedef struct _MEM_ERROR_INFO
{
   MEM_ERROR_CODE errorCode; /* error code identifying type of error      */
   MEM_POOL pool;            /* pool in which error occurred, if known    */

/* all fields below this are valid only for debugging lib                 */
   /* the following seven fields identify the call where error detected   */
   MEM_API errorAPI;         /* fn ID of entry-point where error detected */
   MEM_POOL argPool;         /* memory pool parameter, if applicable      */
   void MEM_FAR *argPtr;     /* memory pointer parameter, if applicable   */
   void MEM_FAR *argBuf;     /* result buffer parameter, if applicable    */
   MEM_HANDLE argHandle;     /* memory handle parameter, if applicable    */
   unsigned long argSize;    /* size parameter, if applicable             */
   unsigned long argCount;   /* count parameter, if applicable            */
   unsigned argFlags;        /* flags parameter, if applicable            */

   /* the following two fields identify the app source file and line      */
   const char MEM_FAR *file; /* app source file containing above call     */
   int line;                 /* source line in above file                 */

   /* the following two fields identify call instance of error detection  */
   unsigned long allocCount; /* enumeration of allocation since 1st alloc */
   unsigned long passCount;  /* enumeration of call at at above file/line */
   unsigned checkpoint;      /* group with which call has been tagged     */
   
   /* the following fields, if non-NULL, points to the address where an
      overwrite was detected and another MEM_ERROR_INFO structure
      identifying where the corrupted object was first created, if known  */
   void MEM_FAR *errorAlloc;  /* ptr to beginning of alloc related to error */
   void MEM_FAR *corruptAddr;
   struct _MEM_ERROR_INFO MEM_FAR *objectCreationInfo;

   unsigned long threadID;    /* ID of thread where error detected */
   unsigned long pid;         /* ID of process where error detected */

	void MEM_FAR *callStack[MEM_MAXCALLSTACK];
} MEM_ERROR_INFO;

/* Error handling callback function */
typedef MEM_BOOL (MEM_ENTRY2 * MEM_ENTRY3 MEM_ERROR_FN)
   (MEM_ERROR_INFO MEM_FAR *);

#endif /* MEM_ERROR_INFO_DEFINED */

/* Saftey Levels: parameter to dbgMemSetSafetyLevel */
typedef enum
{
   MEM_SAFETY_SOME=2,     /* fast: minimal debug performance degredation   */
   MEM_SAFETY_FULL,       /* slower: recommended during development        */
   MEM_SAFETY_DEBUG,      /* entire memory pool is checked each entrypoint */
   MEM_SAFETY_LEVEL_INT_MAX = INT_MAX  /* to ensure enum is full int in size */
} MEM_SAFETY_LEVEL;

#ifndef _SMARTHEAP_H

#define MEM_ERROR_RET ULONG_MAX

#endif /* _SMARTHEAP_H */

#ifndef MEM_BLOCK_TYPE_DEFINED
#define MEM_BLOCK_TYPE_DEFINED
/* Block Type: field of MEM_POOL_ENTRY, field of MEM_POOL_INFO,
 * parameter to MemPoolPreAllocate
 */
typedef enum
{
   MEM_FS_BLOCK               = 0x0001u,
   MEM_VAR_MOVEABLE_BLOCK     = 0x0002u,
   MEM_VAR_FIXED_BLOCK        = 0x0004u,
   MEM_EXTERNAL_BLOCK         = 0x0008u,
   MEM_BLOCK_TYPE_INT_MAX = INT_MAX  /* to ensure enum is full int in size */
} MEM_BLOCK_TYPE;
#endif /* MEM_BLOCK_TYPE_DEFINED */

#ifndef MEM_POOL_ENTRY_DEFINED
#define MEM_POOL_ENTRY_DEFINED
/* Pool Entry: parameter to MemPoolWalk */
typedef struct
{
   void MEM_FAR *entry;
   MEM_POOL pool;
   MEM_BLOCK_TYPE type;
   MEM_BOOL isInUse;
   unsigned long size;
   MEM_HANDLE handle;
   unsigned lockCount;
   void MEM_FAR *reserved_ptr;
} MEM_POOL_ENTRY;
#endif /* MEM_POOL_ENTRY_DEFINED */

#ifndef MEM_POOL_STATUS_DEFINED
#define MEM_POOL_STATUS_DEFINED
/* Pool Status: returned by MemPoolWalk, MemPoolFirst, MemPoolNext */
typedef enum
{
   MEM_POOL_OK            = 1,
   MEM_POOL_CORRUPT       = -1,
   MEM_POOL_CORRUPT_FATAL = -2,
   MEM_POOL_END           = 0,
   MEM_POOL_STATUS_INT_MAX = INT_MAX  /* to ensure enum is full int in size */
} MEM_POOL_STATUS;
#endif /* MEM_POOL_STATUS_DEFINED */

typedef enum
{
   MEM_POINTER_HEAP,
   MEM_POINTER_STACK,
   MEM_POINTER_STATIC,
   MEM_POINTER_CODE,
   MEM_POINTER_READONLY,
   MEM_POINTER_READWRITE
} MEM_POINTER_TYPE;

/* Debug Pool Info: parameter to dbgMemPoolInfo */
typedef struct
{
   MEM_POOL pool;
   const char MEM_FAR *name;
   MEM_API createAPI;
   const char MEM_FAR *createFile;
   int createLine;
   unsigned long createPass;
   unsigned long createAllocCount;
   unsigned createCheckpoint;
   int isDeferFreeing;
   unsigned long deferQueueLength;
   unsigned checkFrequency;
   MEM_ERROR_INFO lastOkInfo;
   unsigned long threadID;
   unsigned long pid;
	void *callStack[MEM_MAXCALLSTACK];
} DBGMEM_POOL_INFO;

/* Flags specifying heap properties */
#ifndef MEM_POOL_SHARED
#define MEM_POOL_SHARED       0x0001u /* == TRUE for SH 1.5 compatibility  */
#define MEM_POOL_SERIALIZE    0x0002u /* pool used in more than one thread */
#define MEM_POOL_VIRTUAL_LOCK 0x0004u /* pool is locked in physical memory */
#define MEM_POOL_ZEROINIT     0x0008u /* malloc/new from pool zero-inits   */
#define MEM_POOL_REGION       0x0010u /* store pool in user-supplied region*/
#define MEM_POOL_DEFAULT      0x8000u /* pool with default characteristics */
#endif /* MEM_POOL_SHARED */

/* Debug Ptr Info: parameter to dbgMemPtrInfo */
typedef struct
{
   void MEM_FAR *ptr;
   MEM_POOL pool;
   unsigned long argSize;
   MEM_BLOCK_TYPE blockType;
   MEM_BOOL isInUse;
   MEM_API createAPI;
   const char MEM_FAR *createFile;
   int createLine;
   unsigned long createPass;
   unsigned checkpoint;
   unsigned long allocCount;
   MEM_BOOL isDeferFreed;
   MEM_BOOL isFreeFillSuppressed;
   MEM_BOOL isReadOnly;
   MEM_BOOL isNoFree;
   MEM_BOOL isNoRealloc;
   unsigned long threadID;
   unsigned long pid;
	void MEM_FAR *callStack[MEM_MAXCALLSTACK];
} DBGMEM_PTR_INFO;

typedef enum
{
	DBGMEM_STACK_CHECK_NONE=1,
	DBGMEM_STACK_CHECK_ENTRY,
	DBGMEM_STACK_CHECK_RETURN,
	DBGMEM_STACK_CHECK_INT_MAX = INT_MAX  /* to ensure enum is full int size */
} DBGMEM_STACK_CHECKING;

/* Debug Settings Info: parameter to dbgMemSettingsInfo */
typedef struct
{
   MEM_SAFETY_LEVEL safetyLevel;
   unsigned checkFrequency;
   unsigned long allocCount;
   unsigned checkpoint;
   MEM_BOOL isDeferFreeing;
   unsigned long deferQueueLength;
   unsigned long deferSizeThreshold;
   MEM_BOOL isFreeFillSuppressed;
   MEM_BOOL isReallocAlwaysMoves;
   MEM_BOOL isWrongTaskRefReported;
   unsigned outputFlags;
   const char MEM_FAR *outputFile;
   unsigned guardSize;
	unsigned callstackChains;
	DBGMEM_STACK_CHECKING stackChecking;
   unsigned char guardFill;
   unsigned char freeFill;
   unsigned char inUseFill;
} DBGMEM_SETTINGS_INFO;

MEM_ENTRY4 MEM_POOL MemDefaultPool;

typedef void (MEM_ENTRY2 * MEM_ENTRY3 MEM_TRACE_FN)
   (MEM_ERROR_INFO MEM_FAR *, unsigned long);

/* define and initialize these variables at file scope to change defaults */
extern unsigned short MemDefaultPoolBlockSizeFS;
extern unsigned MemDefaultPoolPageSize;
extern unsigned MemDefaultPoolFlags;
extern const unsigned dbgMemGuardSize;
extern const unsigned char dbgMemGuardFill;
extern const unsigned char dbgMemFreeFill;
extern const unsigned char dbgMemInUseFill;

/* define SmartHeap_malloc at file scope if you
 * are intentionally _NOT_ linking in the HeapAgent malloc definition
 * ditto for HeapAgent operator new, and fmalloc etc.
 */
extern int SmartHeap_malloc;
extern int SmartHeap_far_malloc;
extern int SmartHeap_new;

#define DBGMEM_PTR_NOPROTECTION    0x0000u
#define DBGMEM_PTR_READONLY        0x0001u
#define DBGMEM_PTR_NOFREE          0x0002u
#define DBGMEM_PTR_NOREALLOC       0x0004u

#define DBGMEM_OUTPUT_PROMPT       0x0001u
#define DBGMEM_OUTPUT_CONSOLE      0x0002u
#define DBGMEM_OUTPUT_BEEP         0x0004u
#define DBGMEM_OUTPUT_FILE         0x0010u
#define DBGMEM_OUTPUT_FILE_APPEND  0x0020u



/*** Function Prototypes ***/

#ifdef MEM_DEBUG

#if defined(MEM_WIN16) || defined(MEM_WIN32)
#define SHI_MAJOR_VERSION HA_MAJOR_VERSION
#define SHI_MINOR_VERSION HA_MINOR_VERSION
#define SHI_UPDATE_LEVEL HA_UPDATE_LEVEL
#endif /* WINDOWS */

/* malloc, new, et al call these entry-point in debug lib */
MEM_ENTRY1 void MEM_FAR * MEM_ENTRY _dbgMemAllocPtr1(MEM_POOL, unsigned long,
   unsigned, MEM_API, const char MEM_FAR *, int);
MEM_ENTRY1 MEM_BOOL MEM_ENTRY _dbgMemFreePtr1(void MEM_FAR *, MEM_API,
   const char MEM_FAR *, int);
MEM_ENTRY1 void MEM_FAR * MEM_ENTRY _dbgMemReAllocPtr1(void MEM_FAR *,
   unsigned long, unsigned, MEM_API, const char MEM_FAR *, int);
MEM_ENTRY1 unsigned long MEM_ENTRY _dbgMemSizePtr1(void MEM_FAR *, MEM_API,
   const char MEM_FAR *, int);
void MEM_FAR * MEM_ENTRY _dbgMEM_alloc(size_t, unsigned, MEM_API,
                                       const char MEM_FAR *, int);
void MEM_FAR * MEM_ENTRY _dbgMEM_realloc(void MEM_FAR *, size_t,
                                         const char MEM_FAR *, int);
void MEM_ENTRY _dbgMEM_free(void MEM_FAR *, const char MEM_FAR *, int);
MEM_ENTRY1 void MEM_ENTRY _shi_deleteLoc(const char MEM_FAR *file, int line);


/* functions to control error detection */
MEM_ENTRY1 MEM_SAFETY_LEVEL MEM_ENTRY dbgMemSetSafetyLevel(MEM_SAFETY_LEVEL);
MEM_ENTRY1 MEM_BOOL MEM_ENTRY dbgMemSetGuardSize(unsigned);
MEM_ENTRY1 MEM_BOOL MEM_ENTRY dbgMemSetGuardFill(MEM_UCHAR);
MEM_ENTRY1 MEM_BOOL MEM_ENTRY dbgMemSetFreeFill(MEM_UCHAR);
MEM_ENTRY1 MEM_BOOL MEM_ENTRY dbgMemSetInUseFill(MEM_UCHAR);
MEM_ENTRY1 MEM_BOOL MEM_ENTRY dbgMemSetCallstackChains(unsigned);
MEM_ENTRY1 MEM_BOOL MEM_ENTRY dbgMemSetStackChecking(DBGMEM_STACK_CHECKING);
MEM_ENTRY1 unsigned MEM_ENTRY dbgMemSetCheckpoint(unsigned);
MEM_ENTRY1 unsigned MEM_ENTRY _dbgMemPoolSetCheckFrequency(MEM_POOL,
   unsigned, const char MEM_FAR *, int);
MEM_ENTRY1 MEM_BOOL MEM_ENTRY _dbgMemPoolDeferFreeing(MEM_POOL, int,
   const char MEM_FAR *, int);
MEM_ENTRY1 MEM_BOOL MEM_ENTRY _dbgMemPoolFreeDeferred(MEM_POOL,
   const char MEM_FAR *, int);
MEM_ENTRY1 MEM_BOOL MEM_ENTRY _dbgMemProtectPtr(void MEM_FAR *, unsigned,
   const char MEM_FAR *, int);

/* functions to control error reporting */
MEM_ENTRY1 unsigned MEM_ENTRY dbgMemFormatErrorInfo(MEM_ERROR_INFO MEM_FAR *,
   char MEM_FAR *, unsigned);
MEM_ENTRY1 unsigned MEM_ENTRY dbgMemFormatCall(MEM_ERROR_INFO MEM_FAR *,
   char MEM_FAR *, unsigned);
MEM_ENTRY1 MEM_BOOL MEM_ENTRY dbgMemSetDefaultErrorOutput(unsigned flags,
   const char MEM_FAR *file);
MEM_ENTRY1 MEM_TRACE_FN MEM_ENTRY dbgMemSetEntryHandler(MEM_TRACE_FN);
MEM_ENTRY1 MEM_TRACE_FN MEM_ENTRY dbgMemSetExitHandler(MEM_TRACE_FN);
MEM_ENTRY1 MEM_BOOL MEM_ENTRY _dbgMemReportLeakage(MEM_POOL,unsigned,unsigned,
   const char MEM_FAR *, unsigned);

MEM_ENTRY1 unsigned long MEM_ENTRY _dbgMemTotalCount(const char MEM_FAR *, int);
MEM_ENTRY1 unsigned long MEM_ENTRY _dbgMemTotalSize(const char MEM_FAR *, int);
MEM_ENTRY1 void MEM_ENTRY _dbgMemBreakpoint(const char MEM_FAR *, int);
MEM_ENTRY1 MEM_BOOL MEM_ENTRY _dbg_MemPoolInfo(MEM_POOL,
   DBGMEM_POOL_INFO MEM_FAR *, const char MEM_FAR *, int);
MEM_ENTRY1 MEM_BOOL MEM_ENTRY _dbgMemPtrInfo(void MEM_FAR *,
   DBGMEM_PTR_INFO MEM_FAR *, const char MEM_FAR *, int);
MEM_ENTRY1 MEM_BOOL MEM_ENTRY _dbgMemSettingsInfo(
   DBGMEM_SETTINGS_INFO MEM_FAR *, const char MEM_FAR *, int);
MEM_ENTRY1 unsigned MEM_ENTRY dbgMemSetCheckFrequency(unsigned);
MEM_ENTRY1 MEM_BOOL MEM_ENTRY dbgMemDeferFreeing(MEM_BOOL);
MEM_ENTRY1 MEM_BOOL MEM_ENTRY _dbgMemFreeDeferred(const char MEM_FAR *, int);
MEM_ENTRY1 MEM_BOOL MEM_ENTRY dbgMemReallocMoves(MEM_BOOL);
MEM_ENTRY1 MEM_BOOL MEM_ENTRY dbgMemSuppressFreeFill(MEM_BOOL);
MEM_ENTRY1 MEM_BOOL MEM_ENTRY dbgMemReportWrongTaskRef(MEM_BOOL);
MEM_ENTRY1 unsigned long MEM_ENTRY dbgMemSetDeferQueueLen(unsigned long);
MEM_ENTRY1 unsigned long MEM_ENTRY dbgMemSetDeferSizeThreshold(unsigned long);
MEM_ENTRY1 MEM_BOOL MEM_ENTRY _dbgMemPoolSetName(MEM_POOL,
   const char MEM_FAR *, const char MEM_FAR *, int);
MEM_ENTRY1 unsigned long MEM_ENTRY _dbgMemPoolSetDeferQueueLen(MEM_POOL,
   unsigned long, const char MEM_FAR *, int);
MEM_ENTRY1 MEM_BOOL MEM_ENTRY _dbgMemCheckAll(const char MEM_FAR *, int);
MEM_POOL_STATUS MEM_ENTRY _dbgMemWalkHeap(
   MEM_POOL_ENTRY MEM_FAR *, const char MEM_FAR *, int);
MEM_ENTRY1 MEM_BOOL MEM_ENTRY _dbg_MemCheckPtr(void MEM_FAR *,
   MEM_POINTER_TYPE, unsigned long, const char MEM_FAR *, int);
MEM_ENTRY1 MEM_BOOL MEM_ENTRY dbgMemScheduleChecking(MEM_BOOL, int, unsigned);
#endif
   

/* Error Handling Functions */
MEM_ENTRY1 MEM_ERROR_FN MEM_ENTRY MemSetErrorHandler(MEM_ERROR_FN);
MEM_ENTRY1 MEM_BOOL MEM_ENTRY MemDefaultErrorHandler(MEM_ERROR_INFO MEM_FAR*);
MEM_ENTRY1 void MEM_ENTRY MemErrorUnwind(void);

/* internal routines */
MEM_ENTRY1 MEM_BOOL MEM_ENTRY _shi_enterCriticalSection(void);
MEM_ENTRY1 void MEM_ENTRY _shi_leaveCriticalSection(void);


/* Wrapper macros for passing file/line info */

#ifndef _SHI_dbgMacros
#ifdef MEM_DEBUG
#ifndef MALLOC_MACRO
#define MEM_malloc(s_) _dbgMEM_alloc(s_, 0, MEM_MEM_MALLOC, __FILE__, __LINE__)
#define MEM_calloc(s_, c_) _dbgMEM_alloc((s_)*(c_), 1 /*MEM_ZEROINIT*/, \
   MEM_MEM_CALLOC, __FILE__, __LINE__)
#define MEM_realloc(p_, s_) _dbgMEM_realloc(p_, s_, __FILE__, __LINE__)
#define MEM_free(p_) _dbgMEM_free(p_, __FILE__, __LINE__)
#endif
#if !defined(NO_MALLOC_MACRO)
#ifdef malloc
#undef malloc
#endif
#ifdef calloc
#undef calloc
#endif
#ifdef realloc
#undef realloc
#endif
#ifdef free
#undef free
#endif
#define malloc(s_) MEM_malloc(s_)
#define calloc(s_, c_) MEM_calloc(s_, c_)
#define realloc(p_, s_) MEM_realloc(p_, s_)
#define free(p_) MEM_free(p_)
#endif /* NO_MALLOC_MACRO */

#define dbgMemPoolSetCheckFrequency(p, b) \
     _dbgMemPoolSetCheckFrequency(p, b, __FILE__, __LINE__)
#define dbgMemPoolDeferFreeing(p, b) \
     _dbgMemPoolDeferFreeing(p, b, __FILE__, __LINE__)
#define dbgMemPoolFreeDeferred(p) _dbgMemPoolFreeDeferred(p,__FILE__,__LINE__)
#define dbgMemProtectPtr(p, b)    _dbgMemProtectPtr(p, b, __FILE__, __LINE__)
#define dbgMemReportLeakage(p, c1, c2) \
     _dbgMemReportLeakage(p, c1, c2, __FILE__, __LINE__)

#define dbgMemTotalCount() _dbgMemTotalCount(__FILE__, __LINE__)
#define dbgMemTotalSize() _dbgMemTotalSize(__FILE__, __LINE__)
#define dbgMemPoolInfo(p, b) _dbg_MemPoolInfo(p, b, __FILE__, __LINE__)
#define dbgMemPtrInfo(p, b) _dbgMemPtrInfo(p, b, __FILE__, __LINE__)
#define dbgMemSettingsInfo(b) _dbgMemSettingsInfo(b, __FILE__, __LINE__)
#define dbgMemPoolSetName(p, n) _dbgMemPoolSetName(p, n, __FILE__, __LINE__)
#define dbgMemPoolSetDeferQueueLen(p, l) \
   _dbgMemPoolSetDeferQueueLen(p, l, __FILE__, __LINE__)
#define dbgMemFreeDeferred() _dbgMemFreeDeferred(__FILE__, __LINE__)
#define dbgMemWalkHeap(b) _dbgMemWalkHeap(b, __FILE__, __LINE__)
#define dbgMemCheckAll() _dbgMemCheckAll(__FILE__, __LINE__)
#define dbgMemCheckPtr(p, t, s) _dbg_MemCheckPtr(p, t, s, __FILE__, __LINE__)
#define dbgMemBreakpoint() _dbgMemBreakpoint(__FILE__, __LINE__)

#else /* MEM_DEBUG */

/* MEM_DEBUG not defined: define dbgMemXXX as no-op macros
 * each macro returns "success" value when MEM_DEBUG not defined
 */

#define dbgMemBreakpoint() ((void)0)
#define dbgMemCheckAll() 1
#define dbgMemCheckPtr(p, f, s) 1
#define dbgMemDeferFreeing(b) 1
#define dbgMemFormatCall(i, b, s) 0
#define dbgMemFormatErrorInfo(i, b, s) 0
#define dbgMemPoolDeferFreeing(p, b) 1
#define dbgMemFreeDeferred() 1
#define dbgMemPoolFreeDeferred(p) 1
#define dbgMemPoolInfo(p, b) 1
#define dbgMemPoolSetCheckFrequency(p, f) 1
#define dbgMemPoolSetDeferQueueLen(p, b) 1
#define dbgMemPoolSetName(p, n) 1
#define dbgMemProtectPtr(p, f) 1
#define dbgMemPtrInfo(p, b) 1
#define dbgMemReallocMoves(b) 1
#define dbgMemReportLeakage(p, c1, c2) 1
#define dbgMemReportWrongTaskRef(b) 1
#define dbgMemScheduleChecking(b, p, i) 1
#define dbgMemSetCheckFrequency(f) 1
#define dbgMemSetCheckpoint(c) 1
#define dbgMemSetDefaultErrorOutput(x, f) 1
#define dbgMemSetDeferQueueLen(l) 1
#define dbgMemSetDeferSizeThreshold(s) 1
#define dbgMemSetEntryHandler(f) 0
#define dbgMemSetExitHandler(f) 0
#define dbgMemSetFreeFill(c) 1
#define dbgMemSetGuardFill(c) 1
#define dbgMemSetGuardSize(s) 1
#define dbgMemSetInUseFill(c) 1
#define dbgMemSetCallstackChains(s) 1
#define dbgMemSetStackChecking(s) 1
#define dbgMemSetSafetyLevel(s) 1
#define dbgMemSettingsInfo(b) 1
#define dbgMemSuppressFreeFill(b) 1
#define dbgMemTotalCount() 1
#define dbgMemTotalSize() 1
#define dbgMemWalkHeap(b) MEM_POOL_OK

#endif /* MEM_DEBUG */
#endif /* _SHI_dbgMacros */

#if defined(__WATCOMC__) && defined(__SW_3S)
/* Watcom stack calling convention */
   #pragma aux MemDefaultPool "_*";
   #pragma aux MemDefaultPoolBlockSizeFS "_*";
   #pragma aux MemDefaultPoolPageSize "_*";
   #pragma aux MemDefaultPoolFlags "_*";
   #pragma aux SmartHeap_malloc "_*";
   #pragma aux SmartHeap_far_malloc "_*";
   #pragma aux SmartHeap_new "_*";
#ifdef MEM_DEBUG
   #pragma aux dbgMemGuardSize "_*";
   #pragma aux dbgMemGuardFill "_*";
   #pragma aux dbgMemFreeFill "_*";
   #pragma aux dbgMemInUseFill "_*";
#endif
#endif  /* __WATCOMC__ */


       
#ifdef __cplusplus
}

#ifndef __BORLANDC__
/* Borland C++ does not treat extern "C++" correctly */
extern "C++"
{
#endif /* __BORLANDC__ */

#if defined(_MSC_VER) && _MSC_VER >= 900
#pragma warning(disable : 4507)
#endif

#if defined(new)
#if defined(MEM_DEBUG)
#undef new
#define DEFINE_NEW_MACRO 1
#endif
#endif
#include <new.h>

#if ((defined(__BORLANDC__) && (__BORLANDC__ >= 0x450)) \
   || (defined(__WATCOMC__) && __WATCOMC__ >= 1000) \
   || (defined(__IBMCPP__) && __IBMCPP__ >= 250))
#define SHI_ARRAY_NEW 1
#endif

#ifdef DEBUG_NEW
#undef DEBUG_NEW
#endif
#ifdef DEBUG_DELETE
#undef DEBUG_DELETE
#endif
   
#ifdef MEM_DEBUG
#define DBG_FORMAL , const char MEM_FAR *file, int line
#define DBG_ACTUAL , file, line


/* both debug and non-debug versions are both defined so that calls
 * to shi_New from inline versions of operator new in modules
 * that were not recompiled with MEM_DEBUG will resolve correctly
 */
void MEM_FAR * MEM_ENTRY_ANSI shi_New(unsigned long DBG_FORMAL, unsigned=0,
   MEM_POOL=0);

/* operator new variants: */

/* compiler-specific versions of new */
#if UINT_MAX == 0xFFFFu
#if defined(__BORLANDC__)
#if (defined(__LARGE__) || defined(__COMPACT__) || defined(__HUGE__))
inline void far *operator new(unsigned long sz DBG_FORMAL)
   { return shi_New(sz DBG_ACTUAL); }
#if __BORLANDC__ >= 0x450
inline void MEM_FAR *operator new[](unsigned long sz DBG_FORMAL)
   { return shi_New(sz DBG_ACTUAL); }
#endif /* __BORLANDC__ >= 0x450 */
#endif /* __LARGE__ */

#elif defined(_MSC_VER)
#if (defined(M_I86LM) || defined(M_I86CM) || defined(M_I86HM))
#ifndef MEM_HUGE
#define MEM_HUGE 0x8000u
#endif /* MEM_HUGE */
inline void __huge * operator new(unsigned long count, size_t sz DBG_FORMAL)
   { return (void __huge *)shi_New(count * sz DBG_ACTUAL, MEM_HUGE); }
#endif /* M_I86LM */
#endif /* _MSC_VER */

#endif /* compiler-specific versions of new */

/* version of new that passes memory allocation flags */
inline void MEM_FAR *operator new(size_t sz DBG_FORMAL, unsigned flags)
   { return shi_New(sz DBG_ACTUAL, flags); }

/* version of new that allocates from a specified memory pool with alloc flags*/
inline void MEM_FAR *operator new(size_t sz DBG_FORMAL, MEM_POOL pool)
   { return shi_New(sz DBG_ACTUAL, 0, pool); }
inline void MEM_FAR *operator new(size_t sz DBG_FORMAL, MEM_POOL pool, 
                                  unsigned flags)
   { return shi_New(sz DBG_ACTUAL, flags, pool); }
#ifdef SHI_ARRAY_NEW
inline void MEM_FAR *operator new[](size_t sz DBG_FORMAL, MEM_POOL pool)
   { return shi_New(sz DBG_ACTUAL, 0, pool); }
inline void MEM_FAR *operator new[](size_t sz DBG_FORMAL, MEM_POOL pool, 
                                  unsigned flags)
   { return shi_New(sz DBG_ACTUAL, flags, pool); }
#endif /* SHI_ARRAY_NEW */

/* version of new that changes the size of a memory block */
inline void MEM_FAR *operator new(size_t new_sz DBG_FORMAL,
                                  void MEM_FAR *lpMem, unsigned flags)
   { return _dbgMemReAllocPtr1(lpMem, new_sz, flags, MEM_NEW DBG_ACTUAL); }
#ifdef SHI_ARRAY_NEW
inline void MEM_FAR *operator new[](size_t new_sz DBG_FORMAL,
                                    void MEM_FAR *lpMem, unsigned flags)
   { return _dbgMemReAllocPtr1(lpMem, new_sz, flags, MEM_NEW DBG_ACTUAL); }
#endif /* SHI_ARRAY_NEW */

/* To have HeapAgent track file/line of C++ allocations,
 * define new/delete as macros:
 * #define new DEBUG_NEW
 * #define delete DEBUG_DELETE
 *
 * In cases where you use explicit placement syntax, or in modules that define
 * operator new/delete, you must undefine the new/delete macros, e.g.:
 * #undef new
 *    void *x = new(placementArg) char[30];  // cannot track file/line info
 * #define new DEBUG_NEW
 *    void *y = new char[20];                // resume tracking file/line info
 */

#if (!(defined(_AFX) && defined(_DEBUG)) \
	  && !(defined(_MSC_VER) && _MSC_VER >= 900))
/* this must be defined out-of-line for _DEBUG MFC and MEM_DEBUG VC++/Win32 */
inline void MEM_FAR *operator new(size_t sz DBG_FORMAL)
   { return shi_New(sz DBG_ACTUAL); }
#else
void MEM_FAR * MEM_ENTRY_ANSI operator new(size_t sz DBG_FORMAL);
#endif
#ifdef SHI_ARRAY_NEW
inline void MEM_FAR *operator new[](size_t sz DBG_FORMAL)
   { return shi_New(sz DBG_ACTUAL); }
#endif /* SHI_ARRAY_NEW */

#if !(defined(__IBMCPP__) && defined(__DEBUG_ALLOC__))
/* debug new/delete built in for IBM Set C++ and Visual Age C++ */

#define DEBUG_NEW new(__FILE__, __LINE__)
#define DEBUG_NEW1(x_) new(__FILE__, __LINE__, x_)
#define DEBUG_NEW2(x_, y_) new(__FILE__, __LINE__, x_, y_)
#define DEBUG_NEW3(x_, y_, z_) new(__FILE__, __LINE__, x_, y_, z_)

#define DEBUG_DELETE _shi_deleteLoc(__FILE__, __LINE__), delete

#ifdef DEFINE_NEW_MACRO
#ifdef macintosh  /* MPW C++ bug precludes new --> DEBUG_NEW --> new(...) */
#define new new(__FILE__, __LINE__)
#define delete _shi_deleteLoc(__FILE__, __LINE__), delete
#else
#define new DEBUG_NEW
#define delete DEBUG_DELETE
#endif /* macintosh */
#endif /* DEFINE_NEW_MACRO */
#endif /* __IBMCPP__ */
#endif /* MEM_DEBUG */

#if !defined(MEM_DEBUG) || (defined(__IBMCPP__) && __DEBUG_ALLOC__)

#ifndef MEM_DEBUG
#define DBG_FORMAL
#define DBG_ACTUAL
#endif
#define DEBUG_NEW new
#define DEBUG_NEW1(x_) new(x_)
#define DEBUG_NEW2(x_, y_) new(x_, y_)
#define DEBUG_NEW3(x_, y_, z_) new(x_, y_, z_)
#define DEBUG_DELETE delete

#endif /* MEM_DEBUG */

#if defined(_MSC_VER) && _MSC_VER >= 900
#pragma warning(default : 4507)
#endif

#ifndef __BORLANDC__
}
#endif /* __BORLANDC__ */

#endif /* __cplusplus */

#endif /* !defined(_HEAPAGNT_H) */

