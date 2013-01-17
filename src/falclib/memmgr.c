/*
 *  MODULE:  MEMMGR.C
 * 
 *  PURPOSE: Memory Management Functions.           
 *                                                  
 *           Maintains a doubly linked list
 *           of memory descriptors that are
 *           inserted at the HEAD of every
 *           malloc.
 *
 *   Functions supported:
 *
 *    MEMMalloc - Allocate
 *    MEMFree   - Return
 *    MEMDump   - Display list & nodes
 *    MEMAvail  - Query the OS for specific memory availability
 *    MEMSanity - Traverse list looking for errors
 *    
 *    MEMFindCount - Find number of allocations
 *    MEMFindEqual - Find mallocs of size n
 *    MEMFindMin   - Find mallocs of size n or less
 *    MEMFindMax   - Find mallocs of size n or more
 *    MEMFindName  - Find simarly named mallocs
 *
 *    MEMFindUsage - Report sum of all mallocs
 *
 *    MEMStrDup - Replace stdlib strdup
 *
 *    new       - c++ overload
 *    new[]     - c++ overload   not allowed under msvc
 *    delete    - c++ overload
 *    delete[]  - c++ overload   not allowed under msvc
 *                                                  
 *
 *  MSVC4.x Note:
 *    Even though Microsoft Visual C++ 4.x supports wrapped allocations, there are several
 *    debugging features here that might be useful.  If this file is compiled with _DEBUG
 *    defined, the MSVC run-time debugging is available as well.
 *
 *
 *  AUTHORS:                                        
 *    KBR   3/29/94           Created.
 *    KBR   4/16/96           Win95 port 
 *                              renamed .c from .cpp
 *                              removed watcom specific comments
 *                              removed mono-windows support
 *
 *    (c) 1994, 1995, 1996 Spectrum Holobyte.
 *
 */

#include <malloc.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#include "memmgr.h"

#if( LIB_COMPILER == COMPILER_WATCOM )
#  include <dos.h>
#  define DPMI_INT        0x31
#endif
#define USE_LAZY_MALLOC_MACRO 1
#if( USE_LAZY_MALLOC_MACRO )    /* in case of precompiled header problems (paired at end-of-file) */
#  ifdef malloc
#     undef malloc
#  endif
#  ifdef calloc
#     undef calloc
#  endif
#  ifdef free
#     undef free
#  endif
#endif

#ifdef MEMMalloc
#  undef MEMMalloc
#endif

#ifdef MEMFree
#  undef MEMFree
#endif

#ifdef new
#  undef new
#endif

#ifdef delete
#  undef delete
#endif

#if(!COOKIE_COUNT_HEAD || !COOKIE_COUNT_TAIL)
#  error There must be at least one magic cookie at head & tail! (memmgr.h)
#endif

/* USE_DEBUG_MONO and USE_DEBUG_WIN are defined in debug.h */

typedef struct MEMCHECK
{
    struct MEMCHECK *nextptr;
    struct MEMCHECK *prevptr;
    char  name[16];
    char  file[16];
    long  time;
    size_t size;
    void *ptr;
    long  sentinel[COOKIE_COUNT_HEAD];
} MEMCHECK;



#define CHECK_COOKIE_HEAD(a,b)  { int kZi; b=TRUE;\
                                    for( kZi=0; kZi<COOKIE_COUNT_HEAD; kZi++ ) { \
                                       if( a[kZi] != MAGIC_COOKIE_HEAD ) {\
                                          b=FALSE;\
                                          break;\
                                       }\
                                    }\
                                 }

#define CHECK_COOKIE_TAIL(a,b)  { int kZi; b=TRUE;\
                                    for( kZi=0; kZi<COOKIE_COUNT_TAIL; kZi++ ) { \
                                       if( a[kZi] != MAGIC_COOKIE_TAIL ) {\
                                          b=FALSE;\
                                          break;\
                                       }\
                                    }\
                                 }

#define SET_COOKIE_HEAD(a)  { int kZi;\
                                for( kZi=0; kZi<COOKIE_COUNT_HEAD; kZi++ )\
                                   a[kZi] = MAGIC_COOKIE_HEAD;\
                            }

#define SET_COOKIE_TAIL(a)  { int kZi;\
                                for( kZi=0; kZi<COOKIE_COUNT_TAIL; kZi++ )\
                                   a[kZi] = MAGIC_COOKIE_TAIL;\
                            }


#define RESET_COOKIE_HEAD(a)  { int kZi;\
                                    for( kZi=0; kZi<COOKIE_COUNT_HEAD; kZi++ ) { \
                                       if( a[kZi] == MAGIC_COOKIE_HEAD )\
                                          a[kZi] = MAGIC_COOKIE_FREE;\
                                    }\
                              }


#define RESET_COOKIE_TAIL(a)  { int kZi;\
                                    for( kZi=0; kZi<COOKIE_COUNT_TAIL; kZi++ ) { \
                                       if( a[kZi] == MAGIC_COOKIE_TAIL )\
                                          a[kZi] = MAGIC_COOKIE_FREE;\
                                    }\
                              }

void  PrintMemHeading(void),
      PrintMemLine(int i, MEMCHECK * newptr);


#if( LIB_COMPILER == COMPILE_WATCOM )

struct meminfo
{
    unsigned LargestBlockAvail;
    unsigned MaxUnlockedPage;
    unsigned LargestLockablePage;
    unsigned LinAddrSpace;
    unsigned NumFreePagesAvail;
    unsigned NumPhysicalPagesFree;
    unsigned TotalPhysicalPages;
    unsigned FreeLinAddrSpace;
    unsigned SizeOfPageFile;
    unsigned Reserved[3];

} MemInfo;

#endif /* COMPILE_WATCOM */


#define  MEM_MARK_CHARACTER   0xDD  /* same as msvc runtime lib */


static
MEMCHECK
    * MEMCHECK_LIST = NULL;

long
      MEM_TOTAL_ALLOC = 0;

long
      MEM_TOTAL_MIN = 0,
      MEM_TOTAL_MAX = 0;


#if( USE_THREAD_SAFE )

   HANDLE MEMORY_MUTEX = 0;   /* If you want to bypass microsoft's very heavy
                                 mutex implementation, do so here. */

#  define WAIT_FOR_LOCK(a)     {if(a) WaitForSingleObject( a, INFINITE );}
#  define RELEASE_LOCK(a)      {if(a) ReleaseMutex( a );}
#  define CREATE_LOCK(a)       {if(!a) { a = CreateMutex( NULL, FALSE, NULL ); \
                                if(!a) KEVS_FATAL_ERROR( "Could not get mutex lock." ); }}
#else
#  define WAIT_FOR_LOCK(a)     
#  define RELEASE_LOCK(a)      
#  define CREATE_LOCK(a)       
#endif


int memmgr_linenum;
int memmgr_filename;




/*===================================================

   MEMMalloc :  Allocate memory - wrapper for malloc()

   INPUTS: size in bytes of request, an optional
           string descriptor (8 chars max).

   RETURNS: pointer to allocated memory**

   ** Actually allocates the amount requested AND
      enough for a linked list stucture and an
      extra magic cookie.  The struct contains
      pointers to the previous & next structures,
      the name, a time stamp, and cookie.

  ===================================================*/

MEM_EXPORT void * MEMMalloc( long req_size, char *name, char *filename, int linenum )
{
    void *ptr;
    long *tailptr;
    MEMCHECK *newptr;
    unsigned int req;

    CREATE_LOCK( MEMORY_MUTEX );
    WAIT_FOR_LOCK( MEMORY_MUTEX );

    if (!filename)
        filename = "Huh?";

    if (!req_size)
    {
        DBG(PF("Requested malloc size of 0!\n"));
        DBG(PF("FILE: %s\nLINE: %d\n", filename, linenum));

        DBG(if (filename))
            DBG(PF("File: %s  Line: %d \n", filename, linenum));
    }

#if( ALIGN_ALLOCATION )

    req_size = ALIGN_SIZE(req_size);

#endif

    req = req_size + sizeof(MEMCHECK) + (sizeof(long) * COOKIE_COUNT_TAIL);

#if( LIB_COMPILER == COMPILER_MSVC )
    ptr = (char *)calloc(req, sizeof(char));
//    ptr = (char *)__calloc_dbg(req, sizeof(char), _CRT_BLOCK, filename, linenum);
#else
    if ((ptr = (char *)malloc(req)) != NULL)
        memset(ptr, 0, req);    /* calloc was creating spurious results under watcom 9.5 */
#endif

    if (!ptr)
    {
        DBG(PF("Malloc() failed!  %d bytes, ref is %s.\n", req_size, filename));
        DBG(PF("file: %s ", filename));
        DBG(PF("line: %d ", linenum));
        DBG(PF("\n"));
        RELEASE_LOCK( MEMORY_MUTEX );
        return (NULL);
    }

    MEM_TOTAL_ALLOC += req_size;

    newptr = (MEMCHECK *) ptr;

    SET_COOKIE_HEAD(newptr->sentinel);

    newptr->size = req_size;
    newptr->time = (long)linenum;

    /* cast the pointer to BYTE so our arithmetic works */
    newptr->ptr = (memBYTE *) ptr + sizeof(MEMCHECK);

    if( strlen( filename ) > 15 ) {
        int length, i;

        length = strlen( filename );

        for( i=length; i>0; i-- ) {
            if( filename[i] == '\\' ) {
                if( i<length ) 
                    i++;
                break;
            }
        }

        if( i )
            filename += i;

        if( strlen(filename) > 15 )       
            filename += strlen(filename) - 15;
    }

    (void)strncpy(newptr->name, name, 15);
    newptr -> name[15] = '\0';

    (void)strncpy(newptr->file, filename, 15);
    newptr -> file[15] = '\0';

    if (MEMCHECK_LIST)
    {
        newptr->nextptr = MEMCHECK_LIST;
        newptr->prevptr = NULL;
        MEMCHECK_LIST->prevptr = newptr;
    }
    else
    {
        MEM_TOTAL_MAX = MEM_TOTAL_ALLOC;
        newptr->nextptr = NULL;
        newptr->prevptr = NULL;
    }

//   if( _TimerCount && !MEM_TOTAL_MIN )
//      MEM_TOTAL_MIN = MEM_TOTAL_ALLOC;

    tailptr = (long *)((char *)(newptr->ptr) + req_size);

    SET_COOKIE_TAIL(tailptr);

    MEMCHECK_LIST = newptr;

    if (MEM_TOTAL_ALLOC > MEM_TOTAL_MAX)
        MEM_TOTAL_MAX = MEM_TOTAL_ALLOC;

    if (MEM_TOTAL_ALLOC < MEM_TOTAL_MIN)
        MEM_TOTAL_MIN = MEM_TOTAL_ALLOC;

    RELEASE_LOCK( MEMORY_MUTEX );

    return ((void *)newptr->ptr);
}



/*===================================================

   MEMFree :  Free a chunk of previously allocated
              memory.

   INPUTS: Pointer to the previously allocated
           memory.

   RETURNS: TRUE if successful, FALSE if not.

      This routine traverses the linked list looking
      for a pointer that matches a pointer contained
      in the node within the list.  If a match is
      found, the list is modified (node removed) and
      the memory is freed.

  ===================================================*/


MEM_EXPORT unsigned char MEMFree( void *ptr, char *filename, int linenum )
{
    MEMCHECK * newptr;
    long     * tailptr;
    memBOOL freed;
    memBOOL check_head,
            check_tail;

    static memBOOL ok = TRUE;

    CREATE_LOCK( MEMORY_MUTEX );
    WAIT_FOR_LOCK( MEMORY_MUTEX );

    if (!filename)
        filename = "Huh?";

    if (!MEMCHECK_LIST)
    {
        DBG(PF("Empty list!\n"));

        DBG(if(filename))
            DBG(PF("file: %s ", filename));

        DBG(if(linenum))
            DBG(PF("line: %d ", linenum));

        DBG(PF("\n"));

        RELEASE_LOCK( MEMORY_MUTEX );

        return (FALSE);
    }


#if 0
    if (!ptr)                   /* OK to do - just not polite! */
    {
        DBG(PF("Freeing NULL ptr!\n"));

        DBG(if (filename))
            DBG(PF("file: %s ", filename));

        DBG(if (linenum))
            DBG(PF("line: %d ", linenum));

        DBG(PF("\n"));
    }
#endif

    freed = FALSE;

    if (ptr)
    {
        if (ok)
        {
            for (newptr = MEMCHECK_LIST;
                 newptr;
                 newptr = newptr->nextptr)
            {
                if (newptr->ptr == ptr)
                {
                    tailptr = (long *)((char *)(newptr->ptr) + newptr->size);

                    CHECK_COOKIE_HEAD(newptr->sentinel, check_head);
                    CHECK_COOKIE_TAIL(tailptr, check_tail);

                    if (!check_head || !check_tail)
                    {
                        DBG(PF("Sentinel failed!\n"));
                        DBG(PF("Offending block -> %s\n", newptr->name));

                        DBG(if (filename))
                            DBG(PF("file: %s ", filename));

                        DBG(if (linenum))
                            DBG(PF("line: %d ", linenum));

                        DBG(PF("\n"));
                    }

                    if (newptr->prevptr)
                    {
                        newptr->prevptr->nextptr = newptr->nextptr;
                    }
                    else
                    {
                        if (newptr->nextptr)
                            newptr->nextptr->prevptr = NULL;
                    }

                    if (newptr->nextptr)
                    {
                        newptr->nextptr->prevptr = newptr->prevptr;
                    }
                    else
                    {
                        if (newptr->prevptr)
                            newptr->prevptr->nextptr = NULL;
                    }

                    if (!newptr->prevptr)
                    {
                        if (!newptr->nextptr)
                        {
                            MEMCHECK_LIST = NULL;
                        }
                        else
                        {
                            MEMCHECK_LIST = newptr->nextptr;
                        }
                    }

                    /* Reset cookies */

                    RESET_COOKIE_HEAD(newptr->sentinel);
                    RESET_COOKIE_TAIL(tailptr);

                    MEM_TOTAL_ALLOC -= newptr->size;

#if( !(LIB_COMPILER == COMPILE_MSVC) )        /* since msvc runtime library overwrites anyway ... */
                    MEMMark(newptr);
#endif

                    free(newptr);

                    freed = TRUE;

                    break;
                }
            }

            if (!freed)
            {
                DBG(PF("Couldn't find pointer in list! loc:%X \n", ptr));

                DBG(if (filename))
                    DBG(PF("file: %s ", filename));

                DBG(if (linenum))
                    DBG(PF("line: %d ", linenum));

                DBG(PF("\n"));

                if (ptr)
                {
                    newptr = (MEMCHECK *) ((char *)ptr - sizeof(MEMCHECK));

                    if (newptr->sentinel[0] == MAGIC_COOKIE_HEAD)
                    {
                        DBG(PF("Assuming it's my lost sheep. Freeing block!\n"));
                        free(newptr);
                    }
                    else
                    {
                        if (newptr->sentinel[0] == MAGIC_COOKIE_FREE)
                        {
                            DBG(PF("Looks like it has already been freed.\n"));
                            free(ptr);
                        }
                        else
                        {
                            DBG(PF("Doesn't look like one of mine, freeing blind!\n"));
                            free(ptr);
                        }
                    }
                }
                freed = TRUE;
            }
        }
        else
        {
            DBG(PF("Lists off. Freeing blindly!\n"));
            free(ptr);
            freed = TRUE;
        }
    }

    if (MEM_TOTAL_ALLOC > MEM_TOTAL_MAX)
        MEM_TOTAL_MAX = MEM_TOTAL_ALLOC;

    if (MEM_TOTAL_ALLOC < MEM_TOTAL_MIN)
        MEM_TOTAL_MIN = MEM_TOTAL_ALLOC;

    RELEASE_LOCK( MEMORY_MUTEX );

    return (freed);
}



/* ------------------------------------------------------
   PURPOSE:  Since the sdtlib function strdup does its 
   own allocation, we need to overload it as 
   well.  Unfortunately, this is has not been 
   mimicked for all of the c++ functions that 
   perform their own allocations.  someday... 

   INPUTS: pointer to string to copy

   RETURNS: pointer to newly allocated duplicate

   -------------------------------------------------------- */

MEM_EXPORT char * MEMStrDup( const char *src_string, char *filename, int linenum )
{
    char *dst_string;
    size_t size;

    size = strlen(src_string);

    dst_string = (char *)MEMMalloc((size + 1), "strdup", filename, linenum );

    strcpy(dst_string, src_string);

    return (dst_string);
}




#ifdef __cplusplus

/* ----------------------------------------------------

   PURPOSE:  Replace the C++ new() operator with   
   something that is aware of the        
   memory manager.                       

   This process caused me great agony, and there-  
   fore is commented accordingly.                  

   As far as I can tell, overloading the new and delete
   operators should be quite trivial.

   In "C + C++ Programming Objects in C and C++"
   (Holub) writes that both of these operators can be
   overloaded [p.211].

   In "The C++ programming language" (Bjarne Stroustups) 
   mentions it several times [p. 498-500] [p. 592]

   Finally, in "Programming in C++" (Dewhurst & Stark)
   have several examples of the technique [p.155 - 157].

   Well, it seems as if Watcom is using the "vendor-specific
   -implementation-trump-card" on this.

   What works:

   Operator 'new' may be replaced.  It may be passed any
   number of parameters.  Once all of the code within
   the 'new' function { } has been executed, the global
   'new' function is executed.  The memory allocation code
   in the global function is *not* executed, only the
   initialization code.

   Operator 'delete' may be replaced.  Only one calling
   convention is allowed:  void delete( pointer to void );

   So my implementation of the memory management functions
   for new and delete are as follows:

   new provides the normal allocations via MEMMalloc();

   new may be called as it always has been in the past,
   or it may be called with two optional parameters,
   filename and line number.  This means that

   new fooCl;
   and
   new(__FILE__, __LINE__) fooCl;

   will both work.  The difference between the two
   being the second access method will store the filename
   in the memory structure, as well as dump the line number
   on fatal exit (no memory).  Accordingly:

   #define new    new(__FILE__, __LINE__)

   new fooCl;

   will cause all of your new operations to be substituted 
   for the improved version.

   The delete operator proved more troublesome.  Since the
   calling paramaters are restricted, I had to do some
   grubby little cheats to make things work.  When I receive
   a pointer I cast it to a long and treat it as an array.
   If you remember how the MEMMalloc function works, the
   first long before the pointer returned should be a magic
   cookie.  Therefore pointer[-1] should be a magic cookie.
   Also, pointer[size + 1] should also be a magic cookie.

   So with that in mind, I make a spit and a prayer, test 
   for cookies and decide whether to free a MEMMalloc'd
   block or a traditional malloc'd block.

   This is not as dangerous as it seems.  The only situations
   that will cause a failure are ones in which memory has been
   illegally overwritten (exceeds boundary) or a mashed
   (spurious) pointer has been passed.  Both of these situations
   would cause the program to choke like a rat on strychnine
   as well.

   ------------------------------------------------------------- */

MEM_EXPORT void * operator new( size_t size
#if !defined(_MSC_VER)
, char * memmgr_filename, int memmgr_linenum
#endif
)
{
    memANY_PTR any_ptr;
    unsigned int i;

    if (!memmgr_filename)
        memmgr_filename = "Huh?";

    if (strlen(memmgr_filename) > 12)
    {
        memmgr_filename = &memmgr_filename[strlen(memmgr_filename) - 12];

        for (i = 0; i < strlen(memmgr_filename); i++)
            if (memmgr_filename[i] == '\\' /* backslash 0x5C */ )
                memmgr_filename = &memmgr_filename[i + 1];
    }

    any_ptr = (memANY_PTR) MEMMalloc(size, memmgr_filename, "new()", memmgr_linenum);

    if (!any_ptr)
    {
        DBG(PF("new failed!"));

        DBG(if (memmgr_filename))
            DBG(PF("  file: %s", memmgr_filename));

        DBG(if (memmgr_linenum))
            DBG(PF("  line: %d", memmgr_linenum));

        DBG(PF("\n"));
    }

    return ((memANY_PTR) (any_ptr));
}




MEM_EXPORT void operator delete( memANY_PTR ptr )
{
    long *cookie;
    MEMCHECK *memcheck;
    memBOOL ok;

    ok = FALSE;

    if (ptr)                    /* we don't want to test the low end cookie on a null delete */
    {
        cookie = (long *)ptr;

        /* -------------------------------------------------------------------------------------------
           NOTE:

           By casting the pointer to a long and treating it as an array, we can see if the
           low end of the malloc'd area is bound by a magic cookie (sentinel value) by
           using cookie[-1].

           We could also use this method to check the high end value 
           (eg; cookie[ ( sizeof(long) / sizeof( BYTE ) ) * cookie -> size + sizeof(long) ]
           *but* that would be kind of repulsive - so we don't.
           ------------------------------------------------------------------------------------------- */

        if( cookie[-1] == MAGIC_COOKIE_HEAD ) {
            /* we found a cookie, so assume that there is one of our
               memcheck structures immediate preceding this pointer */

            memcheck = (MEMCHECK *) ((memBYTE *) ptr - sizeof(MEMCHECK));

            cookie = (long *)((memBYTE *) (memcheck->ptr) + memcheck->size);

            if (*cookie == MAGIC_COOKIE_TAIL) {
                ok = TRUE;      /* both cookies are present! yipee! */
            }
        }
    }

    if (!ok) {
        MEMFree(ptr, "BadDel()", 0);
    }
    else {
        MEMFree(ptr, "Del()", 0);
    }
}


/* ----------------------------------------------------

   Some compilers do not differentiate between an
   allocation of a single element and an array of
   elements (eg; new and new[] ).  For those compilers
   that do differentiate the two, the follow functions
   are necessary.

   Note:    Microsoft Visual C++ does not.
   Watcom C / C++ does.

   ---------------------------------------------------- */

#if( MEM_ARRAY_EXTENSION )

MEM_EXPORT void *operator new[] (size_t size, char *filename, int linenum)
{
    memANY_PTR any_ptr;

    if (!filename)
        filename = "Huh?";

    if (strlen(filename) > 12)
    {
        filename = &filename[strlen(filename) - 11];
    }

    any_ptr = (memANY_PTR) MEMMalloc(size, filename, "new[]", linenum);

    if (!any_ptr)
    {
        DBG(PF("new failed!"));

        DBG(if (filename))
            DBG(PF("  file: %s", filename));

        DBG(if (linenum))
            DBG(PF("  line: %d", linenum));

        DBG(PF("\n"));
    }

    return ((memANY_PTR) (any_ptr));
}


MEM_EXPORT void operator delete[] (memANY_PTR ptr)
{
    long *cookie;
    MEMCHECK *memcheck;
    memBOOL ok;

    ok = FALSE;

    if (ptr)                    /* we don't want to test the low end cookie on a null delete */
    {
        cookie = (long *)ptr;

        /* -------------------------------------------------------------------------------
           NOTE:

           By casting the pointer to a long and treating it as an array, we can see if 
           the low end of the malloc'd area is bound by a magic cookie (sentinel value) 
           by using cookie[-1].

           We could also use this method to check the high end value 
           (eg; cookie[ ( sizeof(long) / sizeof( BYTE ) ) * cookie -> size + sizeof(long) ]
           *but* that would be kind of repulsive - so we don't.
           ------------------------------------------------------------------------------- */

        if (cookie[-1] == MAGIC_COOKIE_HEAD)
        {
            /* we found a cookie, so assume that there is one of our
               memcheck structures immediate preceding this pointer */

            memcheck = (MEMCHECK *) ((memBYTE *) ptr - sizeof(MEMCHECK));

            cookie = (long *)((memBYTE *) memcheck->ptr + memcheck->size);

            if (*cookie == MAGIC_COOKIE_TAIL)
            {
                ok = TRUE;      /* both cookies are present! yipee! */
            }
        }
    }

    if (!ok)
    {
        MEMFree(ptr, "BadDel()", 0);
    }
    else
    {
        MEMFree(ptr, "Del()", 0);
    }
}
#endif /* MEM_ARRAY_EXTENSION */

#endif /* __cplusplus */



/*===================================================

   MEMCheckPointer :  Authenticates a pointer as valid.
              

   INPUTS: Pointer to the suspicious allocation.
           

   RETURNS: TRUE if successful, FALSE if not.

      This routine looks for the existence of a valid
      sentinel block immediately preceeding the supplied
      pointer.  If the allocation was created by the 
      memory manager, then if the pointer is treated
      as an array, array[-1] should be our magic cookie.

      To provide protection regardless of packing
      methods, there are four contiguous cookies.

  ===================================================*/

MEM_EXPORT int MEMCheckPointer( char * ptr )
{
    long *tailptr;
    MEMCHECK *newptr;
    memBOOL check_head,
          check_tail;

    newptr = (MEMCHECK *) ((memBYTE *) ptr - sizeof(MEMCHECK));

    CHECK_COOKIE_HEAD(newptr->sentinel, check_head);

    tailptr = (long *)((char *)(newptr->ptr) + newptr->size);

    CHECK_COOKIE_TAIL(tailptr, check_tail);

    if (check_head && check_tail)
        return (TRUE);

    DBG(if (newptr->sentinel[0] == MAGIC_COOKIE_FREE))
        DBG(PF("Pointer has previously been freed.\n"));

    DBG(PF("MEMCheckPointer FAILED!\n"));

    return (FALSE);
}



/*===================================================
   MEMFindCount : Return the number of allocations
              being monitored by the memory
              manager.

   INPUTS: None.
   
   RETURNS: The number of allocations.

  =================================================== */

MEM_EXPORT int MEMFindCount( void )
{
    MEMCHECK *newptr;
    int   count = 0;

    WAIT_FOR_LOCK( MEMORY_MUTEX );

    for (newptr = MEMCHECK_LIST;
         newptr;
         newptr = newptr->nextptr)
        count++;

    RELEASE_LOCK( MEMORY_MUTEX );

    return (count);
}

/*===================================================

   MEMFindSize :  Return the size of an allocation,
                  given it's pointer.

   INPUTS: Pointer to allocation.

   RETURNS: Size of allocation. (-1 if not found).

  ===================================================*/

MEM_EXPORT long MEMFindSize( void * ptr )
{
    MEMCHECK *newptr;

    WAIT_FOR_LOCK( MEMORY_MUTEX );

    for (newptr = MEMCHECK_LIST;
         newptr;
         newptr = newptr->nextptr)
    {
        if (newptr->ptr == ptr) {
            RELEASE_LOCK( MEMORY_MUTEX );
            return (newptr->size);
        }
    }

    RELEASE_LOCK( MEMORY_MUTEX );

    return (-1);
}




/*===================================================

   MEMDump  :  Display all of the information 
               contained within the memory-list.

   INPUTS: None.

   RETURNS: None.

  ===================================================*/

MEM_EXPORT void MEMDump( void )
{
    MEMCHECK *newptr;
    int   i;


    long  total_size = 0;

    DBG(PF("Total Memory Allocations\n\n"));


    PrintMemHeading();

    i = 0;


    WAIT_FOR_LOCK( MEMORY_MUTEX );

    for (newptr = MEMCHECK_LIST;
         newptr != NULL;
         newptr = newptr->nextptr)
    {
        ++i;

        PrintMemLine(i, newptr);
        
        if( !(i % 20))
            PrintMemHeading();

        total_size += newptr->size;
    }

    RELEASE_LOCK( MEMORY_MUTEX );

    DBG(if(!i))
        DBG(PF("No allocations (all freed)\n" ));
}



/*===================================================

   MEMFindLevels :  Display the statically held 
                    values of:

                      1) current memory
                      2) minimum memory
                      3) maximum memory

   INPUTS: None.

   RETURNS: None.

  ===================================================*/

MEM_EXPORT void MEMFindLevels( void )
{
    DBG(PF("Current memory usage:  %ld\n", MEM_TOTAL_ALLOC));
    DBG(PF("Minimum memory usage:  %ld\n", MEM_TOTAL_MIN));
    DBG(PF("Maximum memory usage:  %ld\n", MEM_TOTAL_MAX));
}


#if( LIB_COMPILER == COMPILER_WATCOM )
MEM_EXPORT unsigned long MEMAvail( void )
{
    union REGS regs;
    struct SREGS sregs;

    regs.x.eax = 0x00000500;
    memset(&sregs, 0, sizeof(sregs));
    sregs.es = FP_SEG(&MemInfo);
    regs.x.edi = FP_OFF(&MemInfo);

    int386x(DPMI_INT, &regs, &regs, &sregs);

    DBG(PF("Largest available block (in bytes):     %lu\n", MemInfo.LargestBlockAvail));
    DBG(PF("Maximum unlocked page allocation:       %lu\n", MemInfo.MaxUnlockedPage));
    DBG(PF("Pages that can be allocated and locked: %lu\n", MemInfo.LargestLockablePage));
    DBG(PF("Total linear address space:             %lu\n", MemInfo.LinAddrSpace));
    DBG(PF("Number of free pages available:         %lu\n", MemInfo.NumFreePagesAvail));
    DBG(PF("Number of physical pages not in use:    %lu\n", MemInfo.NumPhysicalPagesFree));
    DBG(PF("Total physical pages managed by host:   %lu\n", MemInfo.TotalPhysicalPages));
    DBG(PF("Free linear address space (pages):      %lu\n", MemInfo.FreeLinAddrSpace));
    DBG(PF("Size of paging/file partition (pages):  %lu\n", MemInfo.SizeOfPageFile));

    return (MemInfo.MaxUnlockedPage * 4);
}

MEM_EXPORT void MEMCheckVMM( void )
{
    union REGS regs;
    struct SREGS sregs;

    regs.x.eax = 0x00000500;
    memset(&sregs, 0, sizeof(sregs));
    sregs.es = FP_SEG(&MemInfo);
    regs.x.edi = FP_OFF(&MemInfo);

    int386x(DPMI_INT, &regs, &regs, &sregs);
}
#endif /* COMPILER_WATCOM */





/*===================================================

   MEMFindEqual :  Display statistics of all 
                   allocations matching the 
                   specified size.

   INPUTS: Allocation size.

   RETURNS: None.

  ===================================================*/

MEM_EXPORT void MEMFindEqual( size_t size )
{
    MEMCHECK *newptr;
    int   i;

    long  total_size = 0;

    DBG(PF("Memory Allocations for blocks of size %ld\n\n", size));

    PrintMemHeading();

    i = 0;

    WAIT_FOR_LOCK( MEMORY_MUTEX );

    for (newptr = MEMCHECK_LIST;
         newptr != NULL;
         newptr = newptr->nextptr)
    {
        if (newptr->size == size)
        {
            ++i;
            PrintMemLine(i, newptr);
            total_size += newptr->size;
        }
    }

    RELEASE_LOCK( MEMORY_MUTEX );

    DBG(PF("Total size : %ld \n", total_size));
    DBG(PF("Total items: %d \n", i));
}


/*===================================================

   MEMFindMin :  Display statistics of all allocations 
                 equal to, or smaller than, the
                 specified size.

   INPUTS: Allocation size.

   RETURNS: None.

  ===================================================*/

MEM_EXPORT void MEMFindMin( size_t size )
{
    MEMCHECK *newptr;
    int   i;

    long  total_size = 0;

    DBG(PF("Memory Allocations for blocks of size %ld\n\n", size));

    PrintMemHeading();

    i = 0;

    WAIT_FOR_LOCK( MEMORY_MUTEX );

    for (newptr = MEMCHECK_LIST;
         newptr != NULL;
         newptr = newptr->nextptr)
    {
        if (newptr->size <= size)
        {
            ++i;
            PrintMemLine(i, newptr);
            total_size += newptr->size;
        }
    }

    RELEASE_LOCK( MEMORY_MUTEX );
    
    DBG(PF("Total size : %ld \n", total_size));
    DBG(PF("Total items: %d \n", i));
}



/*===================================================

   MEMFindMax :  Display statistics of all allocations 
                 equal to, or greater than, the 
                 specified size.

   INPUTS: Allocation size.

   RETURNS: None.

  ===================================================*/

MEM_EXPORT void MEMFindMax( size_t size )
{
    MEMCHECK *newptr;
    int   i;

    long  total_size = 0;

    DBG(PF("Memory Allocations for blocks of size %ld\n\n", size));

    PrintMemHeading();

    i = 0;

    WAIT_FOR_LOCK( MEMORY_MUTEX );

    for (newptr = MEMCHECK_LIST;
         newptr != NULL;
         newptr = newptr->nextptr)
    {
        if ((size_t) newptr->size >= size)
        {
            ++i;
            PrintMemLine(i, newptr);
            total_size += newptr->size;
        }
    }

    RELEASE_LOCK( MEMORY_MUTEX );

    DBG(PF("Total size : %ld \n", total_size));
    DBG(PF("Total items: %d \n", i));
}



/*===================================================

   MEMFindUsage :  Display the total size (in bytes)
                   of allocations handled by the
                   memory manager.
                   
   INPUTS: None.

   RETURNS: None.

  ===================================================*/

MEM_EXPORT void MEMFindUsage( void )
{
    MEMCHECK *newptr;

    long  total_size = 0;

    DBG(PF("Total Memory Allocations = "));


    WAIT_FOR_LOCK( MEMORY_MUTEX );

    for (newptr = MEMCHECK_LIST;
         newptr != NULL;
         newptr = newptr->nextptr)
    {
        total_size += newptr->size;
    }

    RELEASE_LOCK( MEMORY_MUTEX );

    MEM_TOTAL_ALLOC = total_size;

    DBG(PF("%ld \n", total_size));
}



/*===================================================

   MEMFindName :  Display all of the allocations
                  with user strings matching the
                  specified string.
                  
                   
   INPUTS: String to match.

   RETURNS: Number of entries matching.

  ===================================================*/

MEM_EXPORT int MEMFindName( char * string )
{
    MEMCHECK *newptr;
    int   i;
    long  total_size = 0;

    i = strlen(string);

    if ((i == 0) || (i >= 16))
    {
        DBG(PF("Bad string in MEMFindName() \n"));
        return(0);
    }

    DBG(PF("Memory Allocations for blocks named %s\n\n", string));

    PrintMemHeading();

    i = 0;

    WAIT_FOR_LOCK( MEMORY_MUTEX );

    for (newptr = MEMCHECK_LIST;
         newptr != NULL;
         newptr = newptr->nextptr)
    {
        if (!strcmp(string, newptr->name))
        {
            ++i;
            PrintMemLine(i, newptr);
            total_size += newptr->size;
        }
    }

    RELEASE_LOCK( MEMORY_MUTEX );

    DBG(PF("Total size : %ld \n", total_size));
    DBG(PF("Total items: %d \n", i));

    return(i);
}


/*===================================================

   MEMSanity:  Check all of the sentinel dwords.
               Report any failures.

   INPUTS:     None.

   RETURNS:    TRUE (all ok) / FALSE (corrupt).

  ===================================================*/

MEM_EXPORT int MEMSanity( void )
{
    MEMCHECK *newptr;
    int   i;
    long *tailptr;
    memBOOL check_head,
          check_tail;
    int   retval = TRUE;

    long  total_size = 0;

    i = 0;

    WAIT_FOR_LOCK( MEMORY_MUTEX );

    DBG(if (!MEMCHECK_LIST))
        DBG(PF("Empty list!\n"));

    for (newptr = MEMCHECK_LIST;
         newptr != NULL;
         newptr = newptr->nextptr)
    {
        ++i;

        tailptr = (long *)((char *)(newptr->ptr) + newptr->size);

        CHECK_COOKIE_HEAD(newptr->sentinel, check_head);
        CHECK_COOKIE_TAIL(tailptr, check_tail);

        if (!check_head || !check_tail)
        {
            retval = FALSE;
            PrintMemHeading();
            PrintMemLine(i, newptr);
        }
    }

    RELEASE_LOCK( MEMORY_MUTEX );

    return( retval );
}


/*===================================================

   MEMMark:  Writes '*' into memory block

   INPUTS: Pointer to block.

   RETURNS: None.

  ===================================================*/

MEM_EXPORT void MEMMark( void * ptr )
{
    long  size;

    size = MEMFindSize(ptr);

    if (size > 0)
        memset(ptr, MEM_MARK_CHARACTER, size);
}



/* ----------------------------------------
   printing utility function 
   ---------------------------------------- */

MEM_EXPORT void PrintMemHeading( void )
{
#if( USE_DEBUG_WIN || USE_DEBUG_MONO || MEM_DEBUG_PRINTF )
#  if( USE_DEBUG_MONO )
      MonoColor( MONO_REVERSE | MONO_INTENSE );
#  endif

#  if( MEM_DEBUG_PRINTF )
      DBG(PF( "\n" ));
#  endif

    DBG(PF(" BLOCK LOCATION   SIZE  COOKIES     NAME            LINE  FILE  "));

#  if( MEM_DEBUG_PRINTF )
      DBG(PF( "\n" ));
#  endif

#  if( USE_DEBUG_MONO )
      MonoColor( MONO_NORMAL );
#  endif
#endif
}


/* ----------------------------------------
   printing utility function 
   ---------------------------------------- */

MEM_EXPORT void PrintMemLine(int i, MEMCHECK * newptr)
{
    long    * tailptr;
    memBOOL check_head,
            check_tail;

#if( USE_DEBUG_WIN || USE_DEBUG_MONO || MEM_DEBUG_PRINTF )
    DBG(PF("%4d %9X %7ld ", i, (long)newptr->ptr, (long)newptr->size));
#endif

    tailptr = (long *)((char *)newptr + sizeof(MEMCHECK) + newptr->size);

    CHECK_COOKIE_HEAD(newptr->sentinel, check_head);
    CHECK_COOKIE_TAIL(tailptr, check_tail);

    if (check_head, check_tail)
    {
        DBG(PF("PASSED "));
    }
    else
    {
#if( USE_DEBUG_WIN || USE_DEBUG_MONO || MEM_DEBUG_PRINTF )
        #if( USE_DEBUG_MONO )
            MonoColor( MONO_NORMAL | MONO_BLINK );
        #endif

        DBG(PF("FAILED "));

        #if( USE_DEBUG_MONO )
            dbgPause();
            MonoColor( MONO_NORMAL );
        #endif
#endif
    }

    DBG(PF("  %-16s  %5ld %s\n", newptr->name, newptr->time, newptr->file));


    /* if you are using monowindows (the monochrome monitor debugging windowing library) these
       lines should be used */

#if( USE_DEBUG_MONO )
    /* stdio window is a simple console output onto the mono monitor, otherwise, full windows
       are used on the mono monitor. */

    if (STDIO_WINDOW)
    {
        if (!(i % (STDIO_WINDOW->h - 2)))
        {
            MonoColor(MONO_NORMAL | MONO_BLINK);
            DBG(PF("Press any key to continue!"));
            dbgPause();

            PrintMemHeading();
        }
    }
    else
    {
        if (!(i % 23))
        {
            MonoColor(MONO_NORMAL | MONO_BLINK);
            DBG(PF("Press any key to continue!"));
            dbgPause();

            PrintMemHeading();
        }
    }
#endif
}
