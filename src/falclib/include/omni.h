#ifndef OMNI_HEADER
#   define  OMNI_HEADER 1
     
#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#endif

#ifndef YES
#  define YES   1
#endif
     
#ifndef NO
#  define NO    0
#endif
     
#ifndef TRUE
#  define TRUE  1
#endif
     
#ifndef FALSE
#  define FALSE 0
#endif
     
#define   COMPILER_WATCOM   1
#define   COMPILER_MSVC     2
#define   COMPILER_GCC      3
     
     
     
     
     
     
/* --------------------------------------------------------------- 
     
                     B U I L D    O P T I O N S
     
   --------------------------------------------------------------- */
     
     
/* ----- FOR ALL LIBRARIES ----- */
     
     
# define USE_THREAD_SAFE                YES
# define USE_WINDOWS                    YES
# define USE_WIN_DEBUG                  YES
# define USE_CPLUSPLUS                  NO
     
# define LIB_COMPILER                   COMPILER_MSVC   /* Choose a compiler */
     
     
     
     
/* ----- MEMORY MANAGER OPTIONS ----- */
     
# define MEM_ENABLED                    NO          /* Use the memory management
functions?         */
     
# define MEM_DEBUG_VERSION              YES         /* Use debug code? 
(maintains system metrics)   */
     
# define MEM_ARRAY_EXTENSION            NO          /* new[] delete[] ... 
doesn't work for msvc, but 
                                                    does for watcom, etc.       
                 */
# define MEM_ALIGN                      YES         /* ALIGN memory allocations 
                   */
     
# define MEM_DEBUG_PRINTF               YES         /* console i/o to print 
debug information       */
     
# define MEM_REPLACE_MALLOC             NO          /* redefine malloc & free as
MemMalloc, MemFree */
     
# define MEM_TRAP_MALLOC                NO          /* redefine malloc & free as
#error             */
     
/* ----- RESOURCE MANAGER GUIDE ----- */
/**************************************
1.  If you wish for any hard-drive file to override a zip file, regardless of the path of
    the file use RES_USE_FLAT_MODEL YES.

2.  If you want X:\path\..\filename on the hard drive to
    override the identical path in the ZIP use RES_USE_FLAT_MODEL NO

3.  In the hierarchical model, if you want an override directory to contain files to override
    any zip files, regardless of path, "ResAddPath( patch, true)" after the ResAttach call.

4.  The last AddedPath will be serach first.

5.  When calling ResAttach(), replace == FALSE means the zip files WILL NOT replace hard drive
    files of the same name.

6.  For Development, #define RES_NO_REPEATED_ADDPATHS YES, and any calls to ResAddPath with
    a path already added will be skipped. This can save load time.

7.  Caution: When RES_DEBUG_VERSION NO, checking against MAX_DIRECTORIES is disabled.
    It could be possible that after several Released Patches that MAX_DIRECTORIES could be
    exceeded if not redefined in the PATCHed code,
    so insure MAX_DIRECTORIES is large enough in released versions.

  
***************************************/     
/* ----- RESOURCE MANAGER OPTIONS ----- */
#define MAX_DIRECTORIES                 250         /* maximum number of directories in search path */

#define RES_NO_REPEATED_ADDPATHS        YES         /* if YES then redundant calls to ResAddPath are ignored*/
     
#define RES_USE_FLAT_MODEL              NO          /* flat (=YES) or hierarchical (=NO) database?  */
     
#define RES_REJECT_EMPTY_FILES          NO          /* reject any empty (size=0) files?             */
     
#define RES_STANDALONE                  NO          /* standalone test version?  */
     
#define RES_COERCE_FILENAMES            YES         /* perform filename coercion?                   */
     
#define RES_MULTITHREAD                 YES         /* use multithreaded version?                   */
     
#define RES_PREDETERMINE_SIZE           NO          /* count all entries before performing alloc    */
     
#define RES_STREAMING_IO                YES         /* allow the standard i/o streaming functions   */
     
#define RES_REPLACE_STREAMING           YES         /* use ftell or ResFTell? (comment in RESMGR.C) */
     
#define RES_WILDCARD_PATHS              YES         /* allow wildcard explansion of directory paths?*/
     
#define RES_ALLOW_ALIAS                 YES         /* can you attach an archive to a 'fake' path?  */
     
#define RES_USE_FULLPATH                YES         /* use my _fullpath? (see comment in RESMGR.C)  */
     
#define RES_CPP_HYBRID                  YES         /* both c and cpp files in the project?         */
     
#define RES_DEBUG_VERSION               YES         /* include debug code? */
     
#define RES_DEBUG_PARAMS                YES         /* perform parameter checking?                  */
     
#define RES_DEBUG_LOG                   NO          /* perform event logging and error reporting?   */
     
                                                    /* NOTE: RES_DEBUG_PARAMS and RES_DEBUG_LOG both 
                                                       require RES_DEBUG_VERSION to be YES */


/* ----- LIST LIBRARY OPTIONS ----- */
     
#define   USE_LIST_ALLOCATIONS          YES
     
     
     
     
/* ----- SOUND MANAGER OPTIONS ----- */
     
#define SND_LINEAR_SCALE                YES         /* use linear volume scale? (opposed to db)
         */

#define SND_STANDALONE                  NO          /* standalone test version? 
         */
     
#define SND_DEBUG_VERSION               NO          /* include debug code?      
         */
     
#define SND_DEBUG_PARAMS                YES        /* perform parameter 
checking?        */
     
#define SND_DEBUG_WAVEIO                NO          /* extensive debug code in 
waveio.c   */
     
#define SND_USE_RESMGR                  YES         /* use the resource manager?
         */
                                                    /* make sure you're linking
properly  */
     
#define SND_CPP_HYBRID                  YES         /* some of your project 
files c++ ?   */
     
                                                    /* make sure you're linking
properly  */
     
     
     
     
     
     
     
     
     
     
     
     
     
#ifndef DBG
#   define DBG(a)           a
#endif
     
#ifndef PF
#   define PF               MonoPrint
#endif
     
#ifdef __cplusplus
#if USE_CPLUSPLUS
#   define CFUNC            extern 
#else
#   define CFUNC            extern "C" 
#endif
#else
#   define CFUNC            extern
#endif
     
#ifndef KEVS_FATAL_ERROR
#   define KEVS_FATAL_ERROR(a)   PF(a)
#endif
     
#if USE_WINDOWS
#   include <windows.h>
#   if USE_WIN_DEBUG
#       include <windebug.h>
#   endif
#endif
     
#if MEM_ENABLED
//#  include "memmgr.h"
#elif !defined _SMARTHEAP_H
#  define MemFree(p)        free(p)
#  define MemFreePtr        free
#  define MemMalloc(n, s)   malloc(n) 
#  define MemStrDup(s)      strdup(s) 
#  define MemRealloc(p, s)  realloc(p,s) 
#endif /* MEM_ENABLED */
     
#if( !SND_USE_RESMGR )
#   define RES_FOPEN        fopen
#   define RES_FCLOSE       fclose
#   define RES_FTELL        ftell
#   define RES_FSEEK        fseek
#endif
     
#ifndef _cplusplus
#   ifndef PRIVATE
#       define PRIVATE      static
#       define PUBLIC       extern
#   endif
#endif
     
     
     
     
typedef 
   char *                   STRING;
     
#if( USE_CPLUSPLUS )
   typedef 
      void                  (* PFV)(...);  /* Ptr to func returning VOID */
     
   typedef 
      int                   (* PFI)(...);  /* Ptr to func returning int  */
#else
   typedef 
      void                  (* PFV)();     /* Ptr to func returning VOID */
     
   typedef 
      int                   (* PFI)();     /* Ptr to func returning int  */
#endif 
     
     
#endif  /* OMNI_HEADER */
