/* ----------------------------------------------------------------------

    Resource Manager

 Version 2.05                   Released 03/26/97

 Written by Kevin Ray (x301)    (c) 1996 Spectrum Holobyte

   ----------------------------------------------------------------------

      The resource manager's main function is to provide a single entry
      point for file i/o from within an application.  Besides this
      obvious requirement, this implementation also provides these
      added functional blocks:

            1) file extraction from archive (zip) files.
            2) virtual file system that allows easy patching of data.
            3) caching beyond the minimal i/o buffers bitand mscdex.
            4) asynchronous read/writes.
            5) event logging (for general debugging as well as to
               provide a table from which to sort the contents of
               a CD to minimize seeks).
            6) Significant debugging options.

      For the most part, all of the exported functions are syntactically
      similiar to the low-level i/o functions (io.h).  Obvious exceptions
      to this rule are the functions for reading the contents of a
      directory.  Since the standard dos functions _findfirst, _findnext,
      _findclose have always been cumbersome and 'unnatural', this
      implementation uses the metaphor used by the UNIX opendir
      command (Roger Fujii's suggestion).  For more information, look
      at the comment above the ResOpenDirectory function.

      There are several compiler options that can be selected to build
      various flavors of the Resource Manager.  Debugging, obviously,
      can be included or excluded.  Within the general debugging
      category, Event Logging, Parameter Checking and use of a debug
      window can all be included or excluded.  Each of these latter
      options requires that the RES_DEBUG_VERSION be defined and
      non-zero (just use 'YES' in the header file).

      Since the bafoons at Microsoft have deemed multithreading to be
      such an awesome feature (like they invented it - it's not like
      there aren't multithreaded operating systems on even lowly 8-bit
      microcontrollers) this feature can be disabled.  Obviously this
      may seem somewhat rediculous considering that asynch i/o is the
      main feature you want threads for, but MS Visual C++ shows some
      terribly worrisome behaviors with threads.  For instance, the
      compiler doesn't recognize _beginthread() even with <process.h>
      included unless you have the link option /MT selected.  Why does
      the compiler care?  And there is no kill() so callbacks being
      instantiated from a thread are problematic since the callbacks
      context is always that of the thread.  Therefore, if you want
      to hide this functionality from the others using ResMgr, you
      can turn it off.

      In summary, here are the different buid options:

            debugging on or off
            multithreading on or off
            standalone version capability
            flat or hierarchical database (explained below)
            event logging
            parameter checking

      To use this library, you need to do just a few things.  Here is
      a synopsis of the functions you'll need to call and the order
      in which to call them:

         1) ResInit        initialize the resource manager

         2) ResCreatePath  give the resource manager somewhere to
                           look for files

         3) ResAttach      (optional) open an archive file

         4) ResOpenFile,
            ResReadFile,   perform file i/o
            ResCloseFile

         5) ResExit        shut-down the resource manager

 History:

 03/18/96 KBR   Created/Thieved (unzip/inflate from Fujii via gcc).
 04/01/96 KBR   Integrated unzip/inflate under virtual filesystem.

 TBD:  CD statistic caching (head movements, warning tracks, etc).

   ---------------------------------------------------------------------- */
#include <cISO646>
#include "lists.h"         /* list manipulation functions (+list.cpp)        */
#include "resmgr.h"        /* exported prototypes bitand type definitions         */
//#include "memmgr.h"
#include "omni.h"

#include <stdio.h>         /* low-level file i/o (+io.h)                     */
#include <string.h>
#include <memory.h>
#include <sys/stat.h>      /* _S_IWRITE                                      */
#include <stdarg.h>

#include <assert.h>

#if USE_WINDOWS
#  include <io.h>
#  include <direct.h>
#  include <process.h>       /* _beginthread()    MUST SET C++ OPTIONS UNDER 
MSVC SETTINGS                */

#  include <windows.h>       /* all this for MessageBox (may move to debug.cpp)*/
#endif /* USE_WINDOWS */

#include "unzip.h"

#ifdef USE_SH_POOLS
#undef MemFree
#undef MemFreePtr
#undef MemMalloc
#include "Smartheap/Include/smrtheap.h"
MEM_POOL gResmgrMemPool = NULL;
#endif

#define SHOULD_I_CALL(idx,retval)       if( RES_CALLBACK[(idx)] )\
                                            retval = ( *(RES_CALLBACK[(idx)]));

#define SHOULD_I_CALL_WITH(idx,param,retval)  if( RES_CALLBACK[(idx)] )\
                                                  retval = ((*(RES_CALLBACK[(idx)]))(param));

#if( RES_STREAMING_IO )

RES_EXPORT FILE *  __cdecl _getstream(void);
RES_EXPORT FILE *  __cdecl _openfile(const char *, const char *, int, FILE *);
RES_EXPORT void    __cdecl _getbuf(FILE *);
RES_EXPORT int     __cdecl _flush(FILE *);
RES_EXPORT long    __cdecl ftell(FILE *);
RES_EXPORT long    __cdecl _ftell_lk(FILE *);

extern void __cdecl _getbuf(FILE *);
extern int  __cdecl _flush(FILE * str);

#define EINVAL                  22

#define _IOYOURBUF              0x0100
#define _INTERNAL_BUFSIZ        4096
#define _IOARCHIVE              0x00010000
#define _IOLOOSE                0x00020000
#define _SMALL_BUFSIZ           512    /* from stdio.h */
#define _INTERNAL_BUFSIZ        4096   /* from stdio.h */
#define _IOSETVBUF              0x0400 /* from file2.h */
#define _SH_DENYNO              0x40   /* from share.h */

#define anybuf(s)               ((s)->_flag bitand (_IOMYBUF|_IONBF|_IOYOURBUF))
#define inuse(s)                ((s)->_flag bitand (_IOREAD|_IOWRT|_IORW))


#define SHOULD_I_CALL(idx,retval)       if( RES_CALLBACK[(idx)] )\
                                            retval = ( *(RES_CALLBACK[(idx)]));

#define SHOULD_I_CALL_WITH(idx,param,retval)  if( RES_CALLBACK[(idx)] )\
                                                  retval = ((*(RES_CALLBACK[(idx)]))(param));
#endif /* RES_STREAMING_IO */


#define RES_INIT_DIRECTORY_SIZE         8    /* initial size for buffer used in ResCountDirectory */
/* realloc'ed as needed */


/* ----------------------------------------------------------------------

      FLAT MODEL  VS.  HIERARCHICAL MODEL

   ----------------------------------------------------------------------

    There are two different implementation models that can be chosen from.
    The first model is one in which all files are peers of one another.
    Regardless of where the file resides, the flat model treats all files
    as if they were to be found in a single directory.  If there happens
    to be two files with the same name, but in two different directories,
    the file which resides in the directory which was LAST processed is the
    file that is entered into the database (in effect, it overrides the
    existence of the previous file).

    The second implementation model is hierarchical.  In this model
    directory paths are used to differentiate between identically named
    files.  This is the only model which supports wildcarding within a
    directory (\objects\*.3ds).  I'm not sure how usefull this model is, but
    considering what a pain it was to implement, I hope it somehow is.  Since
    one of the most important uses of the Resource Manager is to allow
    projects to be 'patched' by simply copying newer data to a higher
    priority directory, I exposed an 'override' function so    that this
    functionality is still present in the hierarchical model.  In essence, if
    there is a file c:\game\foo.dat and another file c:\object\foo.dat, if
    I override with a file called c:\game\patches\foo.dat I have replaced both
    \game\foo.dat AND \object\foo.dat with the file found in c:\game\patches --
    obviously this requires that more attention be paid to reducing these
 complexities and avoiding possible pitfalls.

      Flat model:       Single hash table
                        All filenames are peers
                        Last file entered takes precedence
                        'Override' is automatic

      Hierarchical model: Hash table for each directory
                        Supports multiple instances of a like-named file
                        'Override' files manually by calling override

    In the flat model, files within a compressed archive are globbed right
    along within the same table as all other files.  The hierarchical model
    will interpret any directory information within the archive file if it
    exists, if not all files will go into their own hash table (as if all the
    files within the archive were in the same directory).

    The flat model is the default.
   ---------------------------------------------------------------------- */




/* ----------------------------------------------------------------------

        U T I L I T Y   M A C R O S

   ---------------------------------------------------------------------- */

#ifndef MAX
#  define MAX(a,b)   ((a) > (b) ? (a) : (b))
#endif
#ifndef MIN
#  define MIN(a,b)   ((a) < (b) ? (a) : (b))
#endif

#define RES_STRING_SET( dst, src, ptr );    { strcpy( ptr, src );        \
                                              dst = ptr;                 \
                                              ptr += strlen(src)+1; }


#define NOTHING                  -1

#define FLAG_TEST(a,b)           ( a bitand b )
#define FLAG_SET(a,b)            ( a or_eq b )
#define FLAG_UNSET(a,b)          ( a xor_eq compl b )

#define HI_WORD(a)               ((a)>>16)
#define LO_WORD(a)               ((a)&0x0ffff)

#define SET_HIWORD(a,b)          { a or_eq ((b)<<16);   }
#define SET_LOWORD(a,b)          { a or_eq (b)&0x0ffff; }

#define WRITTEN_TO_FLAG          -1

#ifdef _MT
extern void __cdecl          _unlock_file(FILE *);
extern void __cdecl          _lock_file(FILE *);

extern void __cdecl          _lock_fhandle(int);
extern void __cdecl          _unlock_fhandle(int);

#   define LOCK_STREAM(a)        { _lock_file(a);   LOG( "+ %d\n", __LINE__ ); }
#   define UNLOCK_STREAM(a)      { _unlock_file(a); LOG( "- %d\n", __LINE__ ); }
#else
#   define LOCK_STREAM(a)
#   define UNLOCK_STREAM(a)
#endif



enum                                    /* compression methods                              */
{
    METHOD_STORED = 0,                  /* just stored in archive (no compression)          */
    METHOD_DEFLATED,                    /* pkzip style 'deflation'                          */
    METHOD_LOOSE                        /* loose file on harddrive                          */
};


#define EMPTY(a)                        ((a) == NOTHING)


/* ----------------------------------------------------------------------

    T Y P E   D E F I N I T I O N S

   ----------------------------------------------------------------------- */


typedef struct ASYNCH_DATA              /* created when spawning a read/write thread    */
{
    int         file;                   /* handle of file performing i/o on             */
    void      * buffer;                 /* ptr to buffer used to read to/write from     */
    size_t      size;                   /* bytes to read -or- bytes to write            */
    PFV         callback;               /* callback to issue upon completion            */

} ASYNCH_DATA;



/* ----------------------------------------------------------------------

   D E B U G   S P E C I F I C   I T E M S

   ---------------------------------------------------------------------- */

#if( RES_DEBUG_VERSION )

#    include "errno.h"


/* ---- DEBUG FUNCTIONS ---- */

void
dbg_analyze_hash(HASH_TABLE * hsh),
                 dbg_print(HASH_ENTRY * entry),
                 dbg_dir(HASH_TABLE * hsh),
                 dbg_device(DEVICE_ENTRY * dev);



STRING RES_ERR_OR_MSGS[] =
{
    "Not enough memory",                   /* Debug - Verbose Error Messages   */
    "Incorrect parameter",
    "Path not found",
    "File sharing (network error)",
    "There is no matching cd number",
    "File already closed",
    "File not found",
    "Can't attrib directory",
    "Can't attrib file",
    "Directory already exists",
    "Not a directory",
    "Could not delete",
    "Could not change directory",
    "Must create search path",
    "Could not spawn thread",
    "Could not open file",
    "Problem reading file",
    "Problem writing file",
    "System path has not been created yet",
    "Cannot interpret file/path",
    "Unknown archive",
    "Too many open files",
    "Illegal file handle",
    "Cannot delete file",
    "Cannot open archive file",
    "Corrupt archive file",
    "Unknown error",
    "Directory already in search path",
    "Destination directory not known (must add first)",
    "Cannot write to an archive",
    "Unsupported compression type",
    "Too many directories"
};

int
RES_ERR_COUNT = sizeof(RES_ERR_OR_MSGS) / sizeof(RES_ERR_OR_MSGS[0]);

void
_say_error(int error, const char * msg, int line, const char * filename);

#   define SAY_ERROR(a,b)   _say_error((a),(b), __LINE__, __FILE__ )

int RES_DEBUG_FLAG     = TRUE;           /* run-time toggle for debugging                  */
int RES_DEBUG_LOGGING  = FALSE;          /* are we currently logging events?               */
int RES_DEBUG_OPEN_LOG = FALSE;          /* is a log file open?                            */

int  RES_DEBUG_FILE     = -1;             /* file handle for logging events                 */

#if( RES_DEBUG_LOG )
#   define IF_LOG(a)               a
#   define LOG  ResDbgPrintf
#else
#   define IF_LOG(a)
#   define LOG
#endif

#   define IF_DEBUG(a)             a
#else /* RES_DEBUG_VERSION ? */
#   define IF_DEBUG(a)
#   define IF_LOG(a)
#   define LOG
#   define SAY_ERROR(a,b)          {RES_DEBUG_ERRNO=(a);}
#endif

#if( RES_DEBUG_VERSION == 0 )
#  if( RES_DEBUG_PARAMS )
#     error RES_DEBUG_VERSION must be TRUE to use RES_DEBUG_PARAMS
#  endif
#endif

#define CREATE_LOCK(a)      CreateMutex( NULL,  FALSE, a );
#define REQUEST_LOCK(a)     WaitForSingleObject(a, INFINITE);
#define RELEASE_LOCK(a)     ReleaseMutex(a);
#define DESTROY_LOCK(a)     CloseHandle(a);



/* ----------------------------------------------------------------------

    S T A T I C   D A T A

   ---------------------------------------------------------------------- */

#if (RES_MULTITHREAD)
static HANDLE  GLOCK = 0;
#endif

HASH_TABLE                              /* For a flat model, this is the only hash table,       */
* GLOBAL_HASH_TABLE = NULL;         /* for a hierarchical model, this is the hashed         */
/* version of a root directory                          */
FILE_ENTRY
* FILE_HANDLES = NULL;              /* Slots for open file handles                          */

PFI
RES_CALLBACK[ NUMBER_OF_CALLBACKS ];

PRIVATE
LIST
* ARCHIVE_LIST = NULL;

PRIVATE
LIST
* OPEN_DIR_LIST = NULL;

char
* RES_PATH[ RES_DIR_LAST ],
* GLOBAL_SEARCH_PATH[ MAX_DIRECTORIES ];

char
GLOBAL_INIT_PATH[ _MAX_PATH ],
                  GLOBAL_CURRENT_PATH[ _MAX_PATH ];

int
RESMGR_INIT = FALSE;

LIST
* GLOBAL_PATH_LIST = NULL;

DEVICE_ENTRY
* RES_DEVICES = NULL;

int
GLOBAL_INIT_DRIVE;

int
GLOBAL_CURRENT_DRIVE;

int
GLOBAL_CURRENT_CD;

int
GLOBAL_VOLUME_MASK = 0;             /* which drive volumes are available                */

int
GLOBAL_SEARCH_INDEX = 0;

int
GLOBAL_CD_DEVICE;

int
RES_DEBUG_ERRNO;                    /* the equivalent of an 'errno'                     */

static char resmgr_version[] = "[Version] ResMgr version 2.0";

static HWND RES_GLOBAL_HWND;



/* ----------------------------------------------------------------------

    L O C A L   P R O T O T Y P E S

   ---------------------------------------------------------------------- */


/* ---- ASYNCH I/O ---- */

void
asynch_write(void * thread_data),                       /* thread function to handle asynch writes      */
             asynch_read(void * thread_data);                        /* thread function to handle asynch reads       */


/* ---- HASH FUNCTIONS ---- */

int
hash(const char * string, int size);                    /* hash function                                */

int
hash_resize(HASH_TABLE * hsh),                          /* dynamically resize a hash table              */
            hash_delete(HASH_ENTRY * entry, HASH_TABLE * hsh);      /* delete an entry from a hash table            */

void
hash_destroy(HASH_TABLE * hsh),                         /* destroy a hash table                         */
             //    hash_purge( HASH_TABLE * hsh, char * archive, char * volume, char * directory, char * name ); /* purge hash entries  */
             hash_purge(HASH_TABLE * hsh, const char * archive, const char * volume, const int * directory, const char * name);   /* purge hash entries  */

HASH_TABLE
* hash_create(int size, char * name);                   /* create a new hash table                      */

HASH_ENTRY
* hash_find(const char * name, HASH_TABLE * hsh),         /* find an entry within a hash table          */
* hash_add(struct _finddata_t * data, HASH_TABLE * hsh),   /* add an entry to a hash table              */
* hash_find_table(const char * name, HASH_TABLE ** table);   /* find an entry within many tables        */

char
* hash_strcpy(HASH_TABLE * hsh, char * string);         /* strcpy that uses the hash table string pool  */


/* ---- MISCELLANEOUS ---- */

int
get_handle(void);                                       /* return an available file handle              */

void
split_path(const char * path, char * filename, char * dirpath),      /* cut a path string in two        */
           shut_down(void);                                        /* release allocations bitand reset Resource Mgr.    */


char
* res_fullpath(char * abs_buffer, const char * rel_buffer, int maxlen);

void
res_detach_ex(ARCHIVE * arc);                           /* allows func ptr to be passed to LIST_DESTROY */

void
sort_path(void);                                      /* forces cd-based paths to the bottom of the
                                                               search path */
int
get_dir_index(char * path);

/* From unzip.cpp */

extern
ARCHIVE
* archive_create(const char * attach_point, const char * filename, HASH_TABLE * table, int replace_flag);

extern
void
archive_delete(ARCHIVE * arc);


extern
int
process_local_file_hdr(local_file_hdr * lrec, char * buffer);


/* From inflate.cpp */
extern
int
inflate(COMPRESSED_FILE * cmp);

extern
int
inflate_free(void);


/* From MSVC CRT */
extern __cdecl _freebuf(FILE *);
extern __cdecl _fseek_lk(FILE *, long, int);
extern __cdecl _lseek_lk(int, long, int);
extern __cdecl _read_lk(int, char *, int);

/* ----------------------------------------------------------------------
   ----------------------------------------------------------------------

   R E S O U R C E   M A N A G E R   *   P U B L I C   F U N C T I O N S

   ----------------------------------------------------------------------
   ---------------------------------------------------------------------- */




/* =======================================================

   FUNCTION:   ResInit

   PURPOSE:    Initialize the resource manager.

   PARAMETERS: Parent window pointer

   RETURNS:    TRUE (pass) / FALSE (fail)

   ======================================================= */

RES_EXPORT int ResInit(HWND hwnd)
{
    DEVICE_ENTRY * dev;

    unsigned long  length,
             file;

    int            drive,
                   index;

    char root[] = "C:\\";       /* root dir mask used to query devices     */
    char string[_MAX_PATH];     /* dummy string to fill out parameter list */

#if USE_SH_POOLS

    if (gResmgrMemPool == NULL)
    {
        gResmgrMemPool = MemPoolInit(0);
    }

#endif


    /* if the user is calling ResInit to re-initialize the resource manager
       (since this is allowable), we need to free up any previous allocations. */

    if ( not RESMGR_INIT)
    {
        memset(GLOBAL_SEARCH_PATH, 0, sizeof(GLOBAL_SEARCH_PATH));
        memset(RES_PATH, 0, sizeof(char*) * RES_DIR_LAST);     /* reset system paths */
        GLOBAL_SEARCH_INDEX = 0;
    }

    shut_down();

    RES_GLOBAL_HWND = hwnd;

#ifdef USE_SH_POOLS
    FILE_HANDLES = (FILE_ENTRY *)MemAllocPtr(gResmgrMemPool, sizeof(FILE_ENTRY) * MAX_FILE_HANDLES, 0);
#else
    FILE_HANDLES = (FILE_ENTRY *)MemMalloc(sizeof(FILE_ENTRY) * MAX_FILE_HANDLES, "File handles");
#endif

    if ( not FILE_HANDLES)
    {
        SAY_ERROR(RES_ERR_NO_MEMORY, "ResInit");
        return(FALSE);
    }

    memset(FILE_HANDLES, 0, sizeof(FILE_ENTRY) * MAX_FILE_HANDLES);

    for (index = 0; index < MAX_FILE_HANDLES; index++)
        FILE_HANDLES[ index ].os_handle = -1;


    /* Save current drive. */

    GLOBAL_INIT_DRIVE = _getdrive();
    _getdcwd(GLOBAL_INIT_DRIVE, GLOBAL_INIT_PATH, _MAX_PATH);

    RES_PATH[ RES_DIR_CURR ] = MemStrDup(GLOBAL_INIT_PATH);


    /* -------------------------------------------------------------

        Minimal Hardware Query

       -------------------------------------------------------------

        We determine all devices in the host machine and store the
        volume name and volume serial number of the media that is
        current mounted on that device.  It there is ever a read
        failure, or ResCheckMedia is explicitly called, we use
        this information to determine if the end-user has swapped
        the media without our knowledge.

        Since it's not unimagineable that a user that is attached
        to a LAN has all his drive letters mapped to network drives,
        or, that under Win95 his CD letter is towards the high end
        of the alphabet because of network drives, it is recommended
        that you use 26 as the value for MAX_DEVICES.  It's not
        very much memory to give up, and you may be glad you did.
       ------------------------------------------------------------- */


    GLOBAL_VOLUME_MASK = 0;

#if 0

    for (drive = 1; drive <= MAX_DEVICES; drive++)
    {
        /* If we can switch to the drive, it exists. - not if there is a seriously
           damaged floppy in the drive, it is possible to crash VxD HFLOP just by
           switching to the device.  Of course, for this case, anytime the user
           double clicks on the floppy icon from the desktop will also cause this
           crash.  Caveat Emptor.   */
        if ( not _chdrive(drive))
        {
            GLOBAL_VOLUME_MASK or_eq (1 << drive);
        }
    }

#endif

    GLOBAL_VOLUME_MASK = GetLogicalDrives();
    GLOBAL_VOLUME_MASK <<= 1; /* 1 is drive A in ResMgr, GetLogicalDrives returns A equals 0 */

#ifdef USE_SH_POOLS
    RES_DEVICES = (DEVICE_ENTRY *)MemAllocPtr(gResmgrMemPool, MAX_DEVICES * sizeof(DEVICE_ENTRY), 0);
#else
    RES_DEVICES = (DEVICE_ENTRY *)MemMalloc(MAX_DEVICES * sizeof(DEVICE_ENTRY), "Devices");
#endif

    if ( not RES_DEVICES)
    {
        SAY_ERROR(RES_ERR_NO_MEMORY, "ResInit");
        return(FALSE);
    }

    GLOBAL_CD_DEVICE = -1;

    for (drive = 1; drive <= MAX_DEVICES; drive++)
    {
        dev = &RES_DEVICES[drive - 1];

        if (GLOBAL_VOLUME_MASK bitand (1 << drive))
        {
            root[0] = (char)('A' + (drive - 1));

            /* According to Microsoft, most of the parameters to GetVolumeInformation are optional, however
               this is not the case.  It is possible to completely destroy the file system on a floppy
               diskette by calling this seemingly innocuous function without all of the parameters */

            dev -> type = (char)(GetDriveType(root));
            dev -> letter = root[0];

            if ((dev -> type == DRIVE_FIXED) or
                (dev -> type == DRIVE_CDROM) or
                (dev -> type == DRIVE_RAMDISK))
            {

                GetVolumeInformation(root, dev -> name, 24, &dev -> serial, &length, &file, string, _MAX_PATH);
            }
            else
            {
                strcpy(dev -> name, "unknown");
                dev -> serial = 0L;
            }


            /* Initialize default entries into the system path tables */

            if ((dev -> type == DRIVE_CDROM) and not RES_PATH[ RES_DIR_CD ])
            {
                GLOBAL_CD_DEVICE = drive - 1; /* NEED A BETTER WAY */
                RES_PATH[ RES_DIR_CD ] = MemStrDup(root);
            }

            if ((drive == 3) and (dev -> type == DRIVE_FIXED))
                RES_PATH[ RES_DIR_HD ] = MemStrDup(root);
        }
        else
        {
            dev -> type = -1;
            dev -> letter = ASCII_DOT;
            strcpy(dev -> name, "unknown");
            dev -> serial = 0L;

        }
    }

    GetTempPath(_MAX_PATH, string);
    RES_PATH[ RES_DIR_TEMP ] = MemStrDup(string);

#if (RES_MULTITHREAD)

    if ( not GLOCK) GLOCK = CREATE_LOCK("multithread");

#endif

    IF_LOG(LOG("Resource Manager Initialized.\n"));

    RESMGR_INIT = TRUE; /* reinitialize the statics */


    return(TRUE);
}




/* =======================================================

   FUNCTION:   ResExit

   PURPOSE:    Shut down the resource manager.

   PARAMETERS: None.

   RETURNS:    None.

   ======================================================= */

RES_EXPORT void ResExit(void)
{
    IF_LOG(LOG("Resource Manager Exiting.\n"));

#if (RES_MULTITHREAD)

    if (GLOCK) DESTROY_LOCK(GLOCK);

    GLOCK = 0;
#endif

    shut_down();

    /* Restore original drive.*/

    _chdrive(GLOBAL_INIT_DRIVE);
    _chdir(GLOBAL_INIT_PATH);

#if( RES_DEBUG_VERSION )

    if (RES_DEBUG_LOGGING)
        ResDbgLogClose();

#   if( USE_MEMMGR )
    MemSanity();
#   endif /* USE_MEMMGR */

#endif /*RES_DEBUG_VERSION */

#if USE_SH_POOLS

    if (gResmgrMemPool not_eq NULL)
    {
        MemPoolFree(gResmgrMemPool);
        gResmgrMemPool = NULL;
    }

#endif
}



/* =======================================================

   FUNCTION:   ResMountCD

   PURPOSE:    Mount a cd.  If there is already a mounted
               disc, call the user callback and allow
               the user to resynch to the current cd.


   PARAMETERS: Parent window pointer

   RETURNS:    TRUE (pass) / FALSE (fail)

   ======================================================= */

RES_EXPORT int ResMountCD(int cd_number, int device)
{
    int resynch = FALSE;
    int retval = 1;

    IF_LOG(LOG("mounted cd %d\n", cd_number));

#if( RES_DEBUG_PARAMS )   /* parameter checking only with debug version */

    if (cd_number < 1 or cd_number > MAX_CD)
        SHOULD_I_CALL_WITH(CALLBACK_UNKNOWN_CD, RES_ERR_ILLEGAL_CD, retval);

    if ( not retval)
        return(FALSE);

#endif

    GLOBAL_CD_DEVICE = device;

    if (GLOBAL_CURRENT_CD not_eq cd_number)   /* we need this flag later */
        resynch = TRUE;

    /* has the user installed a handler for swap cd? */

    SHOULD_I_CALL_WITH(CALLBACK_SWAP_CD, cd_number, retval);

    if ( not retval)
        return(FALSE);

    GLOBAL_CURRENT_CD = cd_number;

    /* has the user installed a handler for resynch to the new cd? */

    SHOULD_I_CALL_WITH(CALLBACK_RESYNCH_CD, cd_number, retval);

    if ( not retval)
        return(FALSE);

    /* haven't failed so far... */

    return(TRUE);
}




/* =======================================================

   FUNCTION:   ResDismountCD

   PURPOSE:    Dismount the currently mounted CD.

   PARAMETERS: None.

   RETURNS:    TRUE (pass) / FALSE (fail)

   ======================================================= */

RES_EXPORT int ResDismountCD(void)
{
    ARCHIVE    * archive;
    LIST       * list;
    int          hit;
    int         dir;   /* GFG change from char to int */

    if (GLOBAL_CD_DEVICE == -1)
        return(FALSE);

    if (ARCHIVE_LIST)
    {

        do
        {

            hit = 0;

            for (list = ARCHIVE_LIST; list; list = list -> next)
            {
                archive = (ARCHIVE *)list -> node;

                if (archive -> volume == (char)GLOBAL_CD_DEVICE)
                {
                    REQUEST_LOCK(archive -> lock);
                    dir = archive -> directory;
                    ResDetach(archive -> os_handle);
                    ResPurge(NULL, NULL, &dir, NULL);
                    hit = 1;
                    RELEASE_LOCK(archive -> lock);
                    break;
                }
            }

        }
        while (hit);
    }


    ResPurge(NULL, (char *)&GLOBAL_CD_DEVICE, NULL, NULL);

    return(TRUE);
}



/* =======================================================

   FUNCTION:   ResCheckMedia

   PURPOSE:    Determine if the end-user has swapped
               the media for the specified device.

   PARAMETERS: 0=A, 1=B, 2=C (drive ordinal).

   RETURNS:    0  - yes they changed the media
               1  - no they didn't change media
               -1 - error reading device (probably in
                    process of changing media.

   ======================================================= */

RES_EXPORT int ResCheckMedia(int device)
{
    int  drive;
    int  retval = 1;
    char root[] = "C:\\";
    char name[26],
         dummy[6];          /* possible bug in GetVolumeInformation */

    unsigned long serial,
             long1,    /* possible bug in GetVolumeInformation */
             long2;    /* possible bug in GetVolumeInformation */

    DEVICE_ENTRY * dev;

#if( RES_DEBUG_PARAMS )

    if (device < 0 or device > MAX_DEVICES)
    {
        SAY_ERROR(RES_ERR_INCORRECT_PARAMETER, "ResCheckMedia (use ordinals)");
        return(-1);
    }

#endif /* RES_DEBUG_PARAMS */

    drive = _getdrive();

    if (_chdrive(device + 1))
        return(-1);

    _chdrive(drive);

    dev = &RES_DEVICES[ device ];
    root[0] = (char)(device + 'A');

    if (GetVolumeInformation(root, name, 22, &serial, &long1, &long2, dummy, 5))
    {
        if (strcmp(name, dev -> name) or (serial not_eq dev -> serial))
        {
            strcpy(dev -> name, name);
            dev -> serial = serial;
            IF_DEBUG(LOG("Media has changed on volume %s\n", root));

            SHOULD_I_CALL_WITH(CALLBACK_SWAP_CD, GLOBAL_CURRENT_CD, retval);

            return(0);
        }

        return(1);
    }

    IF_DEBUG(LOG("Could not read media on volume %s\n", root));
    return(-1);
}



/* =======================================================

   FUNCTION:   ResAttach

   PURPOSE:    Open an archive file (zip).

   PARAMETERS: Directory to graft archive into, name of
               an archive file, replace flag.  If
               attact_point is NULL, the archive is
               grafted on to the current working
               directory.  If replace_flag is TRUE, any
               files contained within the archive that
               collide with files on the attach_point
               are replaced (the matching files in the
               archive file replace those at the
               destination attach point) otherwise
               the reverse is true (the files within
               the archive take precedence).

   RETURNS:    Handle if sucessful, otherwise -1.

   ======================================================= */

RES_EXPORT int ResAttach(const char * attach_point_arg, const char * filename, int replace_flag)
{
    ARCHIVE    * archive;
    HASH_TABLE * table = NULL;
    char         path[_MAX_PATH];
    char         attach_point_backup[_MAX_PATH];
    char       * attach_point;
    int          len, i;
    struct _finddata_t  info;



#if( not RES_USE_FLAT_MODEL )
    HASH_ENTRY * entry;
#endif

    //      _getcwd(old_cwd,MAX_PATH);

    if (attach_point_arg)
    {
        strcpy(attach_point_backup, attach_point_arg);
        attach_point = attach_point_backup;
    }
    else
    {
        attach_point = NULL;
    }


    IF_LOG(LOG("attach: %s %s\n", attach_point, filename));

#if( RES_DEBUG_PARAMS )

    if ( not filename or (strlen(filename) > _MAX_FNAME))
    {
        SAY_ERROR(RES_ERR_INCORRECT_PARAMETER, "ResAttach");
        return(-1);
    }

#endif

    if ( not GLOBAL_HASH_TABLE)
    {
        SAY_ERROR(RES_ERR_INCORRECT_PARAMETER, "ResAttach");
        return(-1);
    }

    if ( not attach_point)
        attach_point = GLOBAL_CURRENT_PATH;

#if( RES_COERCE_FILENAMES )
    len = strlen(attach_point);

    if (attach_point[len - 1] not_eq ASCII_BACKSLASH)
    {
        attach_point[len++] = ASCII_BACKSLASH;
        attach_point[len] = 0x00;
    }

    res_fullpath(path, attach_point, _MAX_PATH);

    attach_point = path;

#endif /* RES_COERCE_FILENAMES */

    info.size = 0;

#if( not RES_USE_FLAT_MODEL )
    entry = hash_find(attach_point, GLOBAL_HASH_TABLE);


    /* I used to force you to use a directory already in
       the resource manager for the attach point.  Roger
       Fujii asked for anything to be used (allowing you
       to create artificial directories).  If you want
       the original functionality, define RES_ALLOW_ALIAS
       to false. */


#if( not RES_ALLOW_ALIAS )

    if ( not entry)
    {
        SAY_ERROR(RES_ERR_PATH_NOT_FOUND, attach_point);
        return(-1);
    }

#else

    /* The attach point does not have to be either a directory -or-
       an 'added' directory (a directory that has been incorporated
       into the Resource Manager via ResAddPath or ResCreatePath */

    if ( not entry)
    {

#if( RES_DEBUG_VERSION )

        if (GLOBAL_SEARCH_INDEX >= (MAX_DIRECTORIES - 1))
        {
            assert( not "Exceeded MAX_DIRECTORIES as defined in omni.h");
            //            SAY_ERROR( RES_ERR_TOO_MANY_DIRECTORIES, "ResAddPath" );
            return(FALSE);
        }

#endif
        table = hash_create(ARCHIVE_TABLE_SIZE, attach_point);

        strcpy(info.name, attach_point);                  /* insert a dummy entry into the global hash table  */
        info.attrib = _A_SUBDIR bitor (unsigned int)FORCE_BIT;
        info.time_create = 0;
        info.time_access = 0;
        info.size = 0;

        entry = hash_add(&info, GLOBAL_HASH_TABLE);

        if ( not entry)
        {
            SAY_ERROR(RES_ERR_UNKNOWN, "ResAttach");
            return(-1);
        }

        entry -> archive       = -1; /* the actual directory existence should not be considered
                                        as part of the archive.  All of the contents found within
                                        the directory are.   This allows a hard disk based file to
                                        override a zip archive */

        entry -> volume = (char)(toupper(attach_point[0]) - 'A');
        entry -> directory = GLOBAL_SEARCH_INDEX;

        GLOBAL_SEARCH_PATH[ GLOBAL_SEARCH_INDEX++ ] = MemStrDup(attach_point);
        GLOBAL_PATH_LIST = LIST_APPEND(GLOBAL_PATH_LIST, table);
        strcpy(GLOBAL_CURRENT_PATH, attach_point);

        entry -> dir = table;
    }

#endif /* RES_ALLOW_ALIAS */
    archive = archive_create(attach_point, filename, (HASH_TABLE *)entry -> dir, replace_flag);
#else  /* RES_FLAT_MODEL  */
    archive = archive_create(attach_point, filename, GLOBAL_HASH_TABLE, replace_flag);
#endif /* RES_FLAT_MODEL  */

    if ( not archive)
    {
        SAY_ERROR(RES_ERR_CANT_OPEN_ARCHIVE, filename);
        return(-1);
    }

    for (i = 0; i < (GLOBAL_SEARCH_INDEX - 1); i++)
    {
        if ( not stricmp(GLOBAL_SEARCH_PATH[i], attach_point))
        {
            //            archive -> directory = (char)i;   /* GFG */
            archive -> directory = (char)(i);     /* GFG */
            break;
        }
    }

    if (i == GLOBAL_SEARCH_INDEX)
        DebugBreak();

    ARCHIVE_LIST = LIST_APPEND(ARCHIVE_LIST, archive);

    return(archive -> os_handle);
}



/* =======================================================

   FUNCTION:   ResDevice

   PURPOSE:    Returns information about a given drive.

   PARAMETERS: Id of device to query.

   RETURNS:    TRUE (pass) / FALSE (fail).

   ======================================================= */

RES_EXPORT int ResDevice(int device_id, DEVICE_ENTRY * dev)
{
#if( RES_DEBUG_PARAMS )

    if (device_id < 0 or device_id > MAX_DEVICES or not dev)
    {
        SAY_ERROR(RES_ERR_INCORRECT_PARAMETER, "ResDevice");
        return(FALSE);
    }

#endif /* RES_DEBUG_PARAMS */

    memcpy(dev, (void *)&RES_DEVICES[device_id], sizeof(DEVICE_ENTRY));

    return(TRUE);
}



/* =======================================================

   FUNCTION:   ResDetach

   PURPOSE:    Closes an open archive file (zip).

   PARAMETERS: Handle of an opened archive file.

   RETURNS:    None.

   ======================================================= */

RES_EXPORT void ResDetach(int handle)
{
    ARCHIVE    * archive = NULL;
    LIST       * list = NULL;

#if( RES_DEBUG_PARAMS )

    if ( not ARCHIVE_LIST)
    {
        //SAY_ERROR( RES_ERR_UNKNOWN_ARCHIVE, "ResDetach" );
        return;
    }

#endif /* RES_DEBUG_PARAMS */

    /* using the handle, search the list for the structure */

    for (list = ARCHIVE_LIST; list; list = list -> next)
    {
        archive = (ARCHIVE *)list -> node;

        if (archive -> os_handle == handle)
            break;
    }

    if ( not list)    /* couldn't find it, may already have been closed - or handle is incorrect */
    {
        SAY_ERROR(RES_ERR_UNKNOWN_ARCHIVE, "ResDetach");
        return;
    }

    IF_LOG(LOG("detach: %s\n", archive -> name));

    REQUEST_LOCK(archive -> lock);

    ResPurge((char *)&archive -> os_handle, NULL, NULL, NULL);

    /* remove the archive from out list */
    ARCHIVE_LIST = LIST_REMOVE(ARCHIVE_LIST, archive);


    /* The inflation code builds a table of constant values for decompressing
       files compressed with pkzip's FIXED compression mode.  The tables are
       dynamically created (if they don't already exist) when you decompress
       data via that method.  Therefore, we don't want to free it up until
       all the zips are detached - then we might as well to reclaim memory */

    if ( not ARCHIVE_LIST)
        inflate_free();

    /* close the actual archive file */
    _close(archive -> os_handle);

    RELEASE_LOCK(archive -> lock);
    DESTROY_LOCK(archive -> lock);

#ifdef USE_SH_POOLS
    MemFreePtr(archive);
#else
    MemFree(archive);
#endif
}



/* =======================================================

   FUNCTION:   ResOpenFile

   PURPOSE:    Open a file via the Resource Manager.

   PARAMETERS: Filename, mode to open with.
               Wildcards are not supported.

   RETURNS:    File int if sucessful, otherwise -1.

   ======================================================= */

RES_EXPORT int ResOpenFile(const char * name, int mode)
{
    HASH_TABLE * table = NULL;
    HASH_ENTRY * entry = NULL;
    FILE_ENTRY * file = NULL;

    LIST * list = NULL;

    ARCHIVE * archive = NULL;

    char dirpath[_MAX_PATH];
    char filename[_MAX_PATH];

    int   retval = 1;
    int   handle;
    int   dir_index;

    struct _finddata_t data; /* used to dummy-up a hash entry */

    char tmp[LREC_SIZE];
    local_file_hdr lrec;

    IF_LOG(LOG("open (%s):\n", name));


#if( RES_DEBUG_PARAMS )

    if ( not name)
    {
        SAY_ERROR(RES_ERR_INCORRECT_PARAMETER, "ResOpenFile");
        return(-1);
    }

#endif

#if (RES_MULTITHREAD)
    REQUEST_LOCK(GLOCK);
#endif


    /* get the next available file handle */
    handle = get_handle();

    if (handle == -1)    /* none left */
    {
        SAY_ERROR(RES_ERR_TOO_MANY_FILES, "ResOpenFile");
#if (RES_MULTITHREAD)
        RELEASE_LOCK(GLOCK); /* GFG */
#endif

        return(-1);
    }

    file = &FILE_HANDLES[ handle ];


    /* ----------------------------------------------------
       Find the hash entry for the given filename.  If we
       are building a hierarchical model, we need to search
       all of the tables in the order of our search path
       (hash_find_table), otherwise, we just need to search
       our main hash table (hash_find).

       If filename coercion is being used, the filename
       string is scanned for the occurence of a backslash.
       If there is one, the code forces a fullpath name
       conversion on this string in-case it is a relative
       pathname (eg; ..\windows\foo.c).
       ----------------------------------------------------  */

#if( not RES_USE_FLAT_MODEL )
    entry = hash_find_table(name, &table);
#else /* flat model */
    entry = hash_find(name, GLOBAL_HASH_TABLE);
#endif

    if ( not entry)      /* NOT FOUND */
    {
        /* if the user is trying to create a file on the harddrive,
           this is ok. */

        if ( not (mode bitand _O_CREAT))
        {
            SAY_ERROR(RES_ERR_FILE_NOT_FOUND, name);
#if (RES_MULTITHREAD)
            RELEASE_LOCK(GLOCK); /* GFG */
#endif

            return(-1);
        }
        else
        {
            /* CREATING A FILE */

            /* In this case, we need to make sure that the directory is already 'added'
               to the resource manager if we are using hierarchical model.  If we are
               using the flat model, we just need to create a dummy hash_entry for the
               file.  The file size member of the hash entry structure will be set
               in ResFileClose. */

            IF_LOG(LOG("creating file: %s\n", name));

#if( not RES_USE_FLAT_MODEL )

            if (strchr(name, ASCII_BACKSLASH))
            {
                split_path(name, filename, dirpath);
                entry = hash_find(dirpath, GLOBAL_HASH_TABLE);
            }
            else  /* current directory */
            {
                strcpy(filename, name);
                strcpy(dirpath, GLOBAL_CURRENT_PATH);
                entry = hash_find(GLOBAL_CURRENT_PATH, GLOBAL_HASH_TABLE);
            }

            if ( not entry or not entry -> dir)    /* directory is not already added */
            {
                SAY_ERROR(RES_ERR_UNKNOWN_WRITE_TO, name);
#if (RES_MULTITHREAD)
                RELEASE_LOCK(GLOCK); /* GFG */
#endif

                return(-1);
            }
            else
            {
                table = (HASH_TABLE *)entry -> dir;
            }

#else /* flat model */

            if (strchr(name, ASCII_BACKSLASH))
                split_path(name, filename, dirpath);
            else /* current directory */
                strcpy(filename, name);

            table = GLOBAL_HASH_TABLE;
#endif /* not RES_USE_FLAT_MODEL */

            strcpy(data.name, filename);

            data.attrib = (unsigned int)FORCE_BIT;
            data.time_create = 0;
            data.time_access = 0;
            data.size = 0;

            entry = hash_add(&data, table);

            if ( not entry)
            {
                SAY_ERROR(RES_ERR_UNKNOWN, "ResOpen - create");
#if (RES_MULTITHREAD)
                RELEASE_LOCK(GLOCK); /* GFG */
#endif

                return(-1);
            }

            for (dir_index = 0; dir_index <= GLOBAL_SEARCH_INDEX; dir_index++)
            {
                if ( not stricmp(dirpath, GLOBAL_SEARCH_PATH[ dir_index ]))
                {
                    entry -> directory = dir_index;
                    entry -> volume = (char)(toupper(dirpath[0]) - 'A');
                    break;
                }
            }

            if (dir_index > GLOBAL_SEARCH_INDEX)
            {
#if (RES_MULTITHREAD)
                RELEASE_LOCK(GLOCK); /* GFG */
#endif
                return(-1);
            }

            entry -> archive = -1;
        }
    }


    /* Make sure the user isn't trying to write to an archive file.
       Someday this may be possible, but not for a while. */

    if (entry -> archive not_eq -1)
    {
        int check;

        check = (_O_CREAT bitor _O_APPEND bitor _O_RDWR bitor _O_WRONLY);
        check and_eq mode;

        if (check)   /* don't known why had to do it broken out like this - ask MSVC */
        {
            SAY_ERROR(RES_ERR_CANT_WRITE_ARCHIVE, name);
#if (RES_MULTITHREAD)
            RELEASE_LOCK(GLOCK); /* GFG */
#endif
            return(-1);
        }
    }


    /* Initialize some common data */
    file -> current_pos = 0;
    file -> current_filbuf_pos = 0;


    /* Is this a loose file (not in an archive?) */

    if (entry -> archive == -1)
    {
        /* may seem redundant but there are too many pathological cases otherwise */

        if (mode bitand _O_CREAT)
            res_fullpath(filename, name, _MAX_PATH);    /* regardless of coercion state */
        else
            sprintf(filename, "%s%s", GLOBAL_SEARCH_PATH[ entry -> directory ], entry -> name);

        /* there is actually a third parameter to open() (MSVC just doesn't admit it)
           octal 666 ensures that stack-crap won't accidently create this file as
           read-only.  Thank to Roger Fujii for this fix */

        file -> os_handle = _open(filename, mode, 0x1b6 /* choked on O666 and O666L */);

        if (file -> os_handle == -1)
        {

            if (errno == EACCES)
            {
                SAY_ERROR(RES_ERR_FILE_SHARING, filename);
            }
            else
            {
                SAY_ERROR(RES_ERR_FILE_NOT_FOUND, filename);
            }

            ResCheckMedia(entry -> volume);
#if (RES_MULTITHREAD)
            RELEASE_LOCK(GLOCK); /* GFG */
#endif
            return(-1);
        }

        file -> seek_start  = 0;
        file -> size        = entry -> size;
        file -> csize       = 0;
        file -> attrib      = entry -> attrib;
        file -> mode        = mode;
        file -> location    = -1;
        file -> zip         = NULL;
        file -> filename    = MemStrDup(filename);
        file -> device      = entry -> volume;

        SHOULD_I_CALL_WITH(CALLBACK_OPEN_FILE, handle, retval);
#if (RES_MULTITHREAD)
        RELEASE_LOCK(GLOCK); /* GFG */
#endif
        return(handle);
    }
    else     /* in an archive */
    {
        /* using the handle, search the list for the structure */

        for (list = ARCHIVE_LIST; list; list = list -> next)
        {
            archive = (ARCHIVE *)list -> node;

            if (archive -> os_handle == entry -> archive)
                break;
        }

        if ( not list)
        {
            SAY_ERROR(RES_ERR_UNKNOWN, " ");   /* archive handle in hash entry is incorrect (or archive detached) */
#if (RES_MULTITHREAD)
            RELEASE_LOCK(GLOCK); /* GFG */
#endif
            return(-1);
        }

        sprintf(filename, "%s%s", GLOBAL_SEARCH_PATH[ entry -> directory ], entry -> name);

        lseek(archive -> os_handle, entry -> file_position + SIGNATURE_SIZE, SEEK_SET);

        _read(archive -> os_handle, tmp, LREC_SIZE);

        process_local_file_hdr(&lrec, tmp);      /* return PK-type error code */

        file -> seek_start = lseek(archive -> os_handle, lrec.filename_length + lrec.extra_field_length, SEEK_CUR);

        switch (entry -> method)
        {
            case STORED:
            {
                file -> os_handle   = archive -> os_handle;
                //file -> seek_start  = entry -> file_position;
                file -> csize       = 0;
                file -> size        = entry -> size;
                file -> filename    = MemStrDup(filename);
                file -> mode        = mode;
                file -> device      = entry -> volume;
                file -> zip         = NULL; /* only used if we need to deflate */

                SHOULD_I_CALL_WITH(CALLBACK_OPEN_FILE, handle, retval);
#if (RES_MULTITHREAD)
                RELEASE_LOCK(GLOCK); /* GFG */
#endif
                return(handle);
                break;
            }

            case DEFLATED:
            {
                COMPRESSED_FILE * zip;

#ifdef USE_SH_POOLS
                zip = (COMPRESSED_FILE *)MemAllocPtr(gResmgrMemPool, sizeof(COMPRESSED_FILE) + (entry -> size), 0);
#else
                zip = (COMPRESSED_FILE *)MemMalloc(sizeof(COMPRESSED_FILE) + (entry -> size), "Inflate");
#endif

                if ( not zip)
                {
                    SAY_ERROR(RES_ERR_NO_MEMORY, "Inflate");
#if (RES_MULTITHREAD)
                    RELEASE_LOCK(GLOCK); /* GFG */
#endif
                    return(-1);
                }

                file -> os_handle   = archive -> os_handle;
                //file -> seek_start  = entry -> file_position;
                file -> csize       = entry -> csize;
                file -> size        = entry -> size;
                file -> filename    = MemStrDup(filename);
                file -> mode        = mode;
                file -> device      = entry -> volume;

#ifdef USE_SH_POOLS
                zip -> slide      = (uch *)MemAllocPtr(gResmgrMemPool, UNZIP_SLIDE_SIZE + INPUTBUFSIZE, 0);   /* glob temporary allocations */
#else
                zip -> slide      = (uch *)MemMalloc(UNZIP_SLIDE_SIZE + INPUTBUFSIZE, "deflate");   /* glob temporary allocations */
#endif

                zip -> in_buffer  = (uch *)zip -> slide + UNZIP_SLIDE_SIZE;
                zip -> in_ptr     = (uch *)zip -> in_buffer;
                zip -> in_count   = 0;
                zip -> in_size    = file -> csize > INPUTBUFSIZE ? INPUTBUFSIZE : file -> csize;
                zip -> csize      = file -> csize;

                zip -> out_buffer = (char *)zip + sizeof(COMPRESSED_FILE);
                zip -> out_count  = 0;
                zip -> archive    = archive;

                file -> zip       = zip;    /* Future use: I may add incremental deflation */

                inflate(zip);

#ifdef USE_SH_POOLS
                MemFreePtr(zip -> slide);      /* Free temporary allocations */
#else
                MemFree(zip -> slide);      /* Free temporary allocations */
#endif

                SHOULD_I_CALL_WITH(CALLBACK_OPEN_FILE, handle, retval);
#if (RES_MULTITHREAD)
                RELEASE_LOCK(GLOCK); /* GFG */
#endif
                return(handle);
                break;
            }

            default:
                SAY_ERROR(RES_ERR_UNSUPPORTED_COMPRESSION, entry -> name);
#if (RES_MULTITHREAD)
                RELEASE_LOCK(GLOCK); /* GFG */
#endif
                return(-1);
                break;
        }
    }

#if (RES_MULTITHREAD)
    RELEASE_LOCK(GLOCK); /* GFG */
#endif
    return(-1);
}


/* =======================================================

   FUNCTION:   ResSizeFile

   PURPOSE:    Determine the size of a file

   PARAMETERS: File handle.

   RETURNS:    Size of the file in bytes.

   ======================================================= */

RES_EXPORT int ResSizeFile(int file)
{
#if( RES_DEBUG_PARAMS )

    if (file < 0 or file >= MAX_FILE_HANDLES)
    {
        SAY_ERROR(RES_ERR_INCORRECT_PARAMETER, "ResSizeFile");
        return(-1);
    }

    if (FILE_HANDLES[ file ].os_handle == -1)
    {
        SAY_ERROR(RES_ERR_ILLEGAL_FILE_HANDLE, "ResSizeFile");
        return(-1);
    }

#endif /* RES_DEBUG_PARAMS */

    return(FILE_HANDLES[ file ].size);
}


/* =======================================================

   FUNCTION:   ResReadFile

   PURPOSE:    Read from a file via the Resource Manager.

   PARAMETERS: File int, ptr to a buffer to place
               data, number of bytes to read.

   RETURNS:    Number of bytes actually read, -1 in case
               of an error.

   ======================================================= */

RES_EXPORT int ResReadFile(int handle, void * buffer, size_t count)
{
    FILE_ENTRY * file;
    int len;
    int retval = 1;

#if( RES_DEBUG_PARAMS )

    if ( not buffer or handle < 0 or handle > MAX_FILE_HANDLES)
    {
        SAY_ERROR(RES_ERR_INCORRECT_PARAMETER, "ResReadFile");
        return(-1);
    }

#endif /* RES_DEBUG_PARAMS */
#if (RES_MULTITHREAD)
    REQUEST_LOCK(GLOCK); /* GFG */
#endif
    file = &FILE_HANDLES[ handle ];

    if (file -> os_handle == -1)
    {
        SAY_ERROR(RES_ERR_ILLEGAL_FILE_HANDLE, "ResReadFile");
        return(-1);
    }

    IF_LOG(LOG("read (%s): (%d bytes)\n", file -> filename, count));

    SHOULD_I_CALL_WITH(CALLBACK_READ_FILE, handle, retval);

    if (file -> current_pos >= file -> size)
    {
#if (RES_MULTITHREAD)
        RELEASE_LOCK(GLOCK); /* GFG */
#endif
        return(0);    /* GFG NOV 18   was return (-1) */


    }

    if ( not file -> zip)
    {

        /* The only way to insure that the heads will be in the right place is
           to reseek on every read (since multiple threads may be reading this
           file).  This is actually not as expensive as it seems.  First of all,
           if the heads are in the same position, or the cache contains the byte
           stream from that offset, no seek will be done.  If that isn't so, a
           seek was going to happen anyway when the OS tries to do the read. */

        lseek(file -> os_handle, (file -> seek_start + file -> current_pos), SEEK_SET);

        len = _read(file -> os_handle, buffer, count);

        if (len < 0)  /* error, see if media has changed */
            ResCheckMedia(file -> device);
        else
            file -> current_pos += len;

        IF_LOG(LOG("read (%s): %d\n", file -> filename, len));
#if (RES_MULTITHREAD)
        RELEASE_LOCK(GLOCK); /* GFG */
#endif
        return(len);
    }
    else
    {

        if (count > (file -> size - file -> current_pos))
            count = file -> size - file -> current_pos;

        memcpy(buffer, file -> zip -> out_buffer + file -> current_pos, count);
        file -> current_pos += count;
#if (RES_MULTITHREAD)
        RELEASE_LOCK(GLOCK); /* GFG */
#endif
        return(count);
    }

    return(0);
}



/* =======================================================

   FUNCTION:   ResLoadFile

   PURPOSE:    Load an entire file into memory.  This is
               really a convenience function that
               encorporates these components:

                  ResOpenFile
                  malloc
                  ResReadFile
                  ResCloseFile

   PARAMETERS: Filename, optional ptr to store number
               of bytes actually read, optional ptr to
               buffer you want file read to.

   RETURNS:    Ptr to buffer holding file or NULL (on error).

   ======================================================= */

RES_EXPORT char * ResLoadFile(const char * filename,  char * buffer, size_t * size)
{
    int file;
    int check;
    int s;
    char * alloc_buffer;

    IF_LOG(LOG("load (%s):\n", filename));

#if( RES_DEBUG_PARAMS )

    if ( not filename)
    {
        SAY_ERROR(RES_ERR_INCORRECT_PARAMETER, "ResLoadFile");
        return(NULL);
    }

#endif /* RES_DEBUG_PARAMS */

    file = ResOpenFile(filename, _O_RDONLY bitor _O_BINARY);

    if (EMPTY(file))
        return(NULL);   /* message will already have been printed if using the debug version */

    s = ResSizeFile(file);

    if ( not buffer)
    {
#ifdef USE_SH_POOLS
        alloc_buffer = (char *)MemAllocPtr(gResmgrMemPool, s, 0);
#else
        alloc_buffer = (char *)MemMalloc(s, filename);
#endif

        if ( not alloc_buffer)
        {
            SAY_ERROR(RES_ERR_NO_MEMORY, filename);
            ResCloseFile(file);

            if (size)
                *size = 0;

            return(NULL);
        }
    }
    else
        alloc_buffer = buffer;

    check = ResReadFile(file, alloc_buffer, s);

    ResCloseFile(file);

    if (check < 0)  /* error reading file */
    {
#ifdef USE_SH_POOLS
        MemFreePtr(alloc_buffer);
#else
        MemFree(alloc_buffer);
#endif
        alloc_buffer = NULL;
    }

    if (size)
        *size = check;

    return(alloc_buffer);
}



/* =======================================================

   FUNCTION:   ResUnloadFile

   PURPOSE:    Unloads a file that was allocated via
               ResLoadFile.  This function is here for
               two purposes, 1) provide a balance to
               the API for ResLoadFile, and 2) to provide
               a placeholder should ResLoadFile be expanded
               to allow some sort of caching.

   PARAMETERS: Memory pointer that was returned from
               ResLoadFile.

   RETURNS:    None.

   ======================================================= */

RES_EXPORT void ResUnloadFile(char * buffer)
{

#if( RES_DEBUG_PARAMS )

    if ( not buffer)
    {
        SAY_ERROR(RES_ERR_INCORRECT_PARAMETER, "ResUnloadFile");
        return;
    }

#endif /* RES_DEBUG_PARAMS */

#ifdef USE_SH_POOLS
    MemFreePtr(buffer);
#else
    MemFree(buffer);
#endif
}



/* =======================================================

   FUNCTION:   ResCloseFile

   PURPOSE:    Close a previously opened file.

   PARAMETERS: File int.

   RETURNS:    TRUE (pass) / FALSE (fail)

   ======================================================= */

RES_EXPORT int ResCloseFile(int file)
{
    HASH_ENTRY * entry;
    char         filename[_MAX_PATH],
                 dirpath[_MAX_PATH];
    long         size;
    int          retval = 1;


#if( RES_DEBUG_PARAMS )

    if (file < 0 or file >= MAX_FILE_HANDLES)
    {
        SAY_ERROR(RES_ERR_INCORRECT_PARAMETER, "ResCloseFile");
        return(FALSE);
    }

    if (FILE_HANDLES[ file ].os_handle == -1)
    {
        SAY_ERROR(RES_ERR_ILLEGAL_FILE_HANDLE, "ResCloseFile");
        return(FALSE);
    }

#endif /* RES_DEBUG_PARAMS */

#if (RES_MULTITHREAD)
    REQUEST_LOCK(GLOCK);
#endif

    IF_LOG(LOG("close (%s):\n", FILE_HANDLES[ file ].filename));

    SHOULD_I_CALL_WITH(CALLBACK_CLOSE_FILE, file, retval);

    if ( not FILE_HANDLES[file].zip)
    {
        /* if the file has been written to, recheck the size */

        if (FILE_HANDLES[file].csize == (unsigned int)WRITTEN_TO_FLAG)
        {
            /* flush to disk */
            _commit(FILE_HANDLES[file].os_handle);

            /* go to the end of the file */
            size = lseek(FILE_HANDLES[file].os_handle, 0, SEEK_END);

            split_path(FILE_HANDLES[file].filename, filename, dirpath);

#if( not RES_USE_FLAT_MODEL )
            entry = hash_find(dirpath, GLOBAL_HASH_TABLE);

            if (entry)
            {
                entry = hash_find(filename, (HASH_TABLE *)entry -> dir);

                if (entry)
                    entry -> size = size;
                else
                {
                    SAY_ERROR(RES_ERR_UNKNOWN, "set size");
                }
            }
            else
            {
                SAY_ERROR(RES_ERR_UNKNOWN, "set size");
            }

#else /* flat model */

            entry = hash_find(filename, GLOBAL_HASH_TABLE);

            if (entry)
                entry -> size = size;
            else
            {
                SAY_ERROR(RES_ERR_UNKNOWN, "set size");
            }

#endif /* not RES_USE_FLAT_MODEL */
        }

        if ( not FILE_HANDLES[ file ].seek_start)   /* don't close an archive */
            _close(FILE_HANDLES[ file ].os_handle);

#ifdef USE_SH_POOLS
        MemFreePtr(FILE_HANDLES[ file ].filename);
#else
        MemFree(FILE_HANDLES[ file ].filename);
#endif
        FILE_HANDLES[ file ].filename = NULL;
        FILE_HANDLES[ file ].os_handle = -1;
#if (RES_MULTITHREAD)
        RELEASE_LOCK(GLOCK);
#endif

        return(TRUE);
    }
    else
    {
#ifdef USE_SH_POOLS
        MemFreePtr(FILE_HANDLES[file].zip);
        MemFreePtr(FILE_HANDLES[file].filename);
#else
        MemFree(FILE_HANDLES[file].zip);
        MemFree(FILE_HANDLES[file].filename);
#endif
        FILE_HANDLES[ file ].zip = NULL;
        FILE_HANDLES[ file ].filename = NULL;
        FILE_HANDLES[ file ].os_handle = -1;
#if (RES_MULTITHREAD)
        RELEASE_LOCK(GLOCK);
#endif

        return(TRUE);
    }

#if (RES_MULTITHREAD)
    RELEASE_LOCK(GLOCK);
#endif

    return(FALSE);
}



/* =======================================================

   FUNCTION:   ResWriteFile

   PURPOSE:    Write to a file via the Resource Manager.

   PARAMETERS: File int, ptr to a buffer to get
               data from, number of bytes to write.

   RETURNS:    Number of bytes actually written.

   ======================================================= */

RES_EXPORT size_t ResWriteFile(int handle, const void * buffer, size_t count)
{
    FILE_ENTRY * file;
    int check;
    int retval = 1;

#if( RES_DEBUG_PARAMS )

    if (handle < 0 or handle >= MAX_FILE_HANDLES or not buffer)
    {
        SAY_ERROR(RES_ERR_INCORRECT_PARAMETER, "ResWriteFile");
        return(0);
    }

#endif /* RES_DEBUG_PARAMS */
#if (RES_MULTITHREAD)
    REQUEST_LOCK(GLOCK);
#endif

    file = &FILE_HANDLES[ handle ];

    if (file -> os_handle == -1)
    {
        SAY_ERROR(RES_ERR_ILLEGAL_FILE_HANDLE, "ResWriteFile");
#if (RES_MULTITHREAD)
        RELEASE_LOCK(GLOCK);
#endif

        return(0);
    }

    if ( not (file -> mode bitand (_O_CREAT bitor _O_APPEND bitor _O_RDWR bitor _O_WRONLY)))
    {
        SAY_ERROR(RES_ERR_PROBLEM_WRITING, file -> filename);
#if (RES_MULTITHREAD)
        RELEASE_LOCK(GLOCK);
#endif
        return(0);
    }

    SHOULD_I_CALL_WITH(CALLBACK_WRITE_FILE, handle, retval);

    /* Set a bit so we know to reestablish the file size on ResCloseFile()        */
    /* Use the csize field since we no this is not used for files we can write to */

    file -> csize = (unsigned int)WRITTEN_TO_FLAG;

    IF_LOG(LOG("write (%s): (%d bytes)\n", file -> filename, count));

    check = _write(file -> os_handle, buffer, count);

    if (check < 0)
        ResCheckMedia(file -> device);
    else
        file -> current_pos += count;

#if (RES_MULTITHREAD)
    RELEASE_LOCK(GLOCK);
#endif

    return(check);
}



/* =======================================================

   FUNCTION:   ResDeleteFile

   PURPOSE:    Remove a file from the r/w medium.

   PARAMETERS: Filename.

   RETURNS:    TRUE (pass) / FALSE (fail)

   ======================================================= */

RES_EXPORT int ResDeleteFile(const char * name)
{
    HASH_ENTRY * entry;
    HASH_TABLE * table;

    int check;

#if( RES_DEBUG_PARAMS )

    if ( not name)
    {
        SAY_ERROR(RES_ERR_INCORRECT_PARAMETER, "ResDeleteFile");
        return(FALSE);
    }

#endif
#if (RES_MULTITHREAD)
    REQUEST_LOCK(GLOCK);
#endif

    IF_LOG(LOG("delete: %s\n", name));

#if( not RES_USE_FLAT_MODEL )
    /* find both the entry bitand the table it resides in */
    entry = hash_find_table(name, &table);
#else
    entry = hash_find(name, GLOBAL_HASH_TABLE);

    table = GLOBAL_HASH_TABLE;
#endif

    if ( not entry)
    {
        SAY_ERROR(RES_ERR_FILE_NOT_FOUND, name);
#if (RES_MULTITHREAD)
        RELEASE_LOCK(GLOCK);
#endif
        return(FALSE);
    }

    if (entry -> file_position == -1)
    {
        chmod(entry -> name, _S_IWRITE);
        check = remove(entry -> name);

        if (check == -1)
        {
            SAY_ERROR(RES_ERR_CANT_DELETE_FILE, name);
        }

        /* don't return yet, remove file from hash
           table even if delete at os level fails */
    }

#if (RES_MULTITHREAD)
    RELEASE_LOCK(GLOCK);
#endif
    return(hash_delete(entry, table));
}



/* =======================================================

   FUNCTION:   ResModifyFile

   PURPOSE:    Modify the attributes of a file stored on
               r/w medium.

   PARAMETERS: Filename.

   RETURNS:    TRUE (pass) / FALSE (fail)

   ======================================================= */

RES_EXPORT int ResModifyFile(const char * name, int flags)
{
    HASH_ENTRY * entry;                        /* ptr to entry in hash table            */
    int          check;                        /* test return val from system calls    */

#if( RES_DEBUG_PARAMS )

    if ( not name)
    {
        SAY_ERROR(RES_ERR_INCORRECT_PARAMETER, "ResModifyFile");
        return(FALSE);
    }

#endif
#if (RES_MULTITHREAD)
    REQUEST_LOCK(GLOCK);
#endif

    IF_LOG(LOG("modify: %s %d\n", name, flags));

#if( not RES_USE_FLAT_MODEL )
    entry = hash_find_table(name, NULL);
#else
    entry = hash_find(name, GLOBAL_HASH_TABLE);
#endif

    if ( not entry)
    {
        SAY_ERROR(RES_ERR_FILE_NOT_FOUND, name);
#if (RES_MULTITHREAD)
        RELEASE_LOCK(GLOCK);
#endif
        return(FALSE);
    }

    if (entry -> file_position == -1)          /* is the file on the harddrive        */
    {
        check = chmod(entry -> name, flags);

        if (check == -1)
        {
            SAY_ERROR(RES_ERR_CANT_ATTRIB_FILE, name);
#if (RES_MULTITHREAD)
            RELEASE_LOCK(GLOCK);
#endif
            return(FALSE);
        }

        entry -> attrib = flags bitor FORCE_BIT;
    }
    else
    {
        SAY_ERROR(RES_ERR_CANT_ATTRIB_FILE, name);
#if (RES_MULTITHREAD)
        RELEASE_LOCK(GLOCK);
#endif
        return(FALSE);
    }

#if (RES_MULTITHREAD)
    RELEASE_LOCK(GLOCK);
#endif
    return(TRUE);
}



/* =======================================================

   FUNCTION:   ResMakeDirectory

   PURPOSE:    Create a directory.

   PARAMETERS: Filename.

   RETURNS:    TRUE (pass) / FALSE (fail)

   ======================================================= */

RES_EXPORT int ResMakeDirectory(char * pathname)
{
    int check;

#if( RES_DEBUG_PARAMS )

    if ( not pathname)
    {
        SAY_ERROR(RES_ERR_INCORRECT_PARAMETER, "ResMakeDirectory");
        return(FALSE);
    }

#endif

    check = _mkdir(pathname);

    IF_LOG(LOG("mkdir: %s\n", pathname));

    if (check == -1)
    {
#if( RES_DEBUG_PARAMS )

        if (errno == EACCES)
        {
            SAY_ERROR(RES_ERR_DIRECTORY_EXISTS, pathname);
        }
        else if (errno == ENOENT)
        {
            SAY_ERROR(RES_ERR_PATH_NOT_FOUND, pathname);
        }

#endif /* RES_DEBUG_PARAMS */

        return(FALSE);
    }

    ResAddPath(pathname, FALSE);

    return(TRUE);
}



/* =======================================================

   FUNCTION:   ResDeleteDirectory

   PURPOSE:    Remove a directory tree.

   PARAMETERS: Filename.

   RETURNS:    TRUE (pass) / FALSE (fail)

   =======================================================
   03/17/97    [GAB] - wrote.
   ======================================================= */

RES_EXPORT int ResDeleteDirectory(char * pathname, int forced)
{
    int  handle,
         check,
         status;

    char full_path[ MAX_PATH ],
         old_cwd[ MAX_PATH ];

    struct _finddata_t fileinfo;

    status = 0;

    handle = _findfirst(pathname, &fileinfo);

    if (handle == -1)
        return(FALSE);     /* couldn't find directory */

    IF_LOG(LOG("deltree: %s\n", pathname));

    if ( not (fileinfo.attrib bitand _A_SUBDIR))
    {
        SAY_ERROR(RES_ERR_IS_NOT_DIRECTORY, pathname);
        return(FALSE);
    }

    _findclose(handle);

    _getcwd(old_cwd, MAX_PATH);
    _chdir(pathname);

    sprintf(full_path, "%s\\*.*", pathname);
    handle = _findfirst(full_path, &fileinfo);

    while (status not_eq -1)
    {

        if ( not stricmp(fileinfo.name, ".") or not stricmp(fileinfo.name, ".."))
        {
            status = _findnext(handle, &fileinfo);
            continue;
        }

        if (fileinfo.attrib bitand _A_SUBDIR)
        {
            char recurse_path[MAX_PATH];
            sprintf(recurse_path, "%s\\%s", pathname, fileinfo.name);
            ResDeleteDirectory(recurse_path, TRUE);
            status = _findnext(handle, &fileinfo);
            continue;
        }

        check = remove(fileinfo.name);

        if (check == -1)
        {
            if (forced)
            {
                chmod(fileinfo.name, _S_IWRITE);
                check = remove(fileinfo.name);
            }

            if (check == -1)
            {
                SAY_ERROR(RES_ERR_COULD_NOT_DELETE, fileinfo.name);
                break;
            }
        }

        status = _findnext(handle, &fileinfo);
    }

    _findclose(handle);

    _chdir(old_cwd);

    _rmdir(pathname);

    if (handle == -1)
    {
        SAY_ERROR(RES_ERR_COULD_NOT_DELETE, pathname);
        return(FALSE);
    }

    return(TRUE);
}



/* =======================================================

   FUNCTION:   ResOpenDirectory

   PURPOSE:    Read the contents of a specified directory
               (based on the unix opendir).

               The idea is that the order of operations is
               1) open dir, 2) read dir, 3) close dir.

               _findfirst, _findnext and _findclose sort
               of do this but are uglier and more
               cumbersome.

               The ResOpenDirectory implementation is:
               ResOpen creates a table of filenames for
               the entire directory.  ResRead returns
               each filename in succession.  ResClose
               frees the allocation from ResOpen.  If
               you use the debug version of the Resource
               Manager, ResExit will report any open
               directories that have not been freed.

               ResOpen will fail if the directory does
 not exist or if there is not enough
               memory to allocate space for the whole
               directory.

               This function is only useful with the
               hierarchical model.

   PARAMETERS: Directory name.

   RETURNS:    Ptr to a RES_DIR struct or NULL (on error).

   ======================================================= */

RES_EXPORT RES_DIR * ResOpenDirectory(char * pathname)
{
    //    int count = 0;

#if( not RES_USE_FLAT_MODEL )

    HASH_TABLE * hsh;
    HASH_ENTRY * entry;
    RES_DIR    * dir;
    size_t       size;
    char         dirpath[_MAX_PATH];
    int          index, i;

#if( RES_DEBUG_PARAMS )

    if ( not GLOBAL_SEARCH_INDEX)
        return(NULL);

#endif

    IF_LOG(LOG("opendir: %s\n", pathname));

    res_fullpath(dirpath, pathname, _MAX_PATH);

    entry = hash_find(dirpath, GLOBAL_HASH_TABLE);

    if (entry and entry -> dir)
    {
        hsh = (HASH_TABLE *)entry -> dir;

        size = sizeof(RES_DIR);
        size += sizeof(char *) * (hsh -> num_entries);
        size += MAX_FILENAME * (hsh -> num_entries + 12);

#ifdef USE_SH_POOLS
        dir = (RES_DIR *)MemAllocPtr(gResmgrMemPool, size, 0);
#else
        dir = (RES_DIR *)MemMalloc(size, "RES_DIR");
#endif

        dir -> filenames = (char**)((char*)dir + sizeof(RES_DIR));
        dir -> string_pool = (char*)dir + sizeof(RES_DIR) + (sizeof(char *) * (hsh -> num_entries));

        if ( not dir -> filenames or not dir -> string_pool)
        {
            SAY_ERROR(RES_ERR_NO_MEMORY, "ResOpenDirectory");
            return(NULL);
        }

        dir -> string_ptr = dir -> string_pool;

        RES_STRING_SET(dir -> name, entry -> name, dir -> string_ptr);

        index = 0;

        for (i = 0; i < hsh->table_size; i++)
        {
            entry = &hsh -> table[i];

            if (entry -> next)
            {
                while (entry)
                {
                    RES_STRING_SET(dir -> filenames[ index++ ], entry -> name, dir -> string_ptr);
                    entry = entry -> next;
                }
            }
            else
            {
                if (entry -> attrib)
                {
                    RES_STRING_SET(dir -> filenames[ index++ ], entry -> name, dir -> string_ptr);
                }
            }
        }

        dir -> num_entries = index;
        dir -> current = 0;

#if( RES_DEBUG_VERSION )
        OPEN_DIR_LIST = LIST_APPEND(OPEN_DIR_LIST, dir);
#endif /* RES_DEBUG_VERSION */

        return(dir);
    }

#endif /* not RES_USE_FLAT_MODEL */

    return(NULL);   /* only usefull in the hierarchical version */
}



/* =======================================================

   FUNCTION:   ResReadDirectory

   PURPOSE:    Returns filenames sequentially.

   PARAMETERS: RES_DIR structure.

   RETURNS:    Ptr to filename string or NULL (on error).

   ======================================================= */

RES_EXPORT char * ResReadDirectory(RES_DIR * dir)
{
#if( RES_DEBUG_PARAMS )

    if ( not dir)
    {
        SAY_ERROR(RES_ERR_INCORRECT_PARAMETER, "ResReadDirectory");
        return(NULL);
    }

#endif /* RES_DEBUG_PARAMS */


    IF_LOG(LOG("readdir: %s\n", dir -> name));

    if (dir -> current >= dir -> num_entries)
        return(NULL);

    return((char *)(dir -> filenames[ dir -> current++ ]));
}



/* =======================================================

   FUNCTION:   ResCloseDirectory

   PURPOSE:    Frees the allocations of ResOpenDirectory.

   PARAMETERS: RES_DIR structure.

   RETURNS:    None.

   ======================================================= */

RES_EXPORT void ResCloseDirectory(RES_DIR * dir)
{
#if( RES_DEBUG_PARAMS )

    if ( not dir)
    {
        SAY_ERROR(RES_ERR_INCORRECT_PARAMETER, "ResCloseDirectory");
        return;
    }

#endif /* RES_DEBUG_PARAMS */

    IF_LOG(LOG("closedir: %s\n", dir -> name));

#ifdef USE_SH_POOLS
    MemFreePtr(dir);
#else
    MemFree(dir);
#endif

#if( RES_DEBUG_VERSION )
    OPEN_DIR_LIST = LIST_REMOVE(OPEN_DIR_LIST, dir);
#endif
}



/* =======================================================

   FUNCTION:   ResExistFile

   PURPOSE:    Determine if a file exists.

   PARAMETERS: Filename.

   RETURNS:    TRUE (yes) / FALSE (no).

   ======================================================= */

RES_EXPORT int ResExistFile(char * name)
{
    HASH_ENTRY * entry;

#if( RES_DEBUG_PARAMS )

    if ( not name)
    {
        SAY_ERROR(RES_ERR_INCORRECT_PARAMETER, "ResExistFile");
        return(FALSE);
    }

#endif
#if (RES_MULTITHREAD)
    REQUEST_LOCK(GLOCK);
#endif

    IF_LOG(LOG("exist file: %s\n", name));

#if( not RES_USE_FLAT_MODEL )
    entry = hash_find_table(name, NULL);
#else /* flat model */
    entry = hash_find(name, GLOBAL_HASH_TABLE);
#endif

#if (RES_MULTITHREAD)
    RELEASE_LOCK(GLOCK);
#endif

    if (entry)
        return(TRUE);

    return(FALSE);
}



/* =======================================================

   FUNCTION:   ResExistDirectory

   PURPOSE:    Determine if a directory already exists.

   PARAMETERS: Directory name.

   RETURNS:    TRUE (yes) / FALSE (no).

   ======================================================= */

RES_EXPORT int ResExistDirectory(char * pathname)
{
    char path[_MAX_PATH];
    int len;

#if( RES_DEBUG_PARAMS )

    if ( not pathname)
    {
        SAY_ERROR(RES_ERR_INCORRECT_PARAMETER, "ResExistDirectory");
        return(FALSE);
    }

#endif

    res_fullpath(path, pathname, _MAX_PATH);

#if( RES_COERCE_FILENAMES )
    len = strlen(path);

    if (path[len - 1] not_eq ASCII_BACKSLASH)
    {
        path[len++] = ASCII_BACKSLASH;
        path[len] = 0x00;
    }

#endif /* RES_COERCE_FILENAMES */

    IF_LOG(LOG("exist dir: %s\n", pathname));

    return((int)hash_find(path, GLOBAL_HASH_TABLE));
}



/* =======================================================

   FUNCTION:   ResTellFile

   PURPOSE:    Returns the current offset within a file
               at which point i/o is set to occur.

   PARAMETERS: File handle.

   RETURNS:    Size in bytes from the beginning of file.
               -1 indicates an error.

   ======================================================= */

RES_EXPORT long ResTellFile(int handle)
{
#if( RES_DEBUG_PARAMS )

    if (handle < 0 or handle >= MAX_FILE_HANDLES)
    {
        SAY_ERROR(RES_ERR_INCORRECT_PARAMETER, "ResTellFile");
        return(-1);
    }

#endif
#if (RES_MULTITHREAD)
    REQUEST_LOCK(GLOCK);
#endif

    if (FILE_HANDLES[ handle ].os_handle == -1)
    {
        SAY_ERROR(RES_ERR_ILLEGAL_FILE_HANDLE, "ResTellFile");
#if (RES_MULTITHREAD)
        RELEASE_LOCK(GLOCK);
#endif
        return(-1);
    }

#if (RES_MULTITHREAD)
    RELEASE_LOCK(GLOCK);
#endif

    return(FILE_HANDLES[ handle ].current_pos);
}



/* =======================================================

   FUNCTION:   ResSeekFile

   PURPOSE:    Moves the file pointer within an open
               file a number of bytes from an origin
               (either the current location, end of file,
               or beginning of file).

   PARAMETERS: File int, offset in bytes, origin.
               Origin can be one of:

                  SEEK_CUR - current location
                  SEEK_SET - beginning of file
                  SEEK_END - end of file

   RETURNS:    New position in file or -1 for error.

   ======================================================= */

RES_EXPORT int ResSeekFile(int handle, size_t offset, int origin)
{
#if( RES_DEBUG_PARAMS )

    if (handle < 0 or handle >= MAX_FILE_HANDLES)
    {
        SAY_ERROR(RES_ERR_INCORRECT_PARAMETER, "ResSeekFile");
        return(-1);
    }

    if ((origin not_eq SEEK_CUR) and (origin not_eq SEEK_SET) and (origin not_eq SEEK_END))
    {
        SAY_ERROR(RES_ERR_INCORRECT_PARAMETER, "ResSeekFile");
        return(-1);
    }

#endif /* RES_DEBUG_PARAMS */
#if (RES_MULTITHREAD)
    REQUEST_LOCK(GLOCK);
#endif

    IF_LOG(LOG("seek: %s\n", FILE_HANDLES[handle].filename));

    if (FILE_HANDLES[ handle ].os_handle == -1)
    {
        SAY_ERROR(RES_ERR_ILLEGAL_FILE_HANDLE, "ResReadFile");
#if (RES_MULTITHREAD)
        RELEASE_LOCK(GLOCK);
#endif
        return(-1);
    }

    /* If we are writing, do seek anyway */
    if (FILE_HANDLES[ handle ].mode bitand (O_WRONLY bitor O_RDWR))
    {
        FILE_HANDLES[ handle ].current_pos  = lseek(FILE_HANDLES[ handle ].os_handle, offset, origin);
    }
    else
    {
        /* cache the seek until we perform the read */

        switch (origin)
        {
            case SEEK_SET: /* 0 */
                FILE_HANDLES[ handle ].current_pos = offset;
                break;

            case SEEK_CUR: /* 1 */
                FILE_HANDLES[ handle ].current_pos += offset;
                break;

            case SEEK_END: /* 2 */
                FILE_HANDLES[ handle ].current_pos = FILE_HANDLES[ handle ].size + offset;
                break;
        }
    }

#if (RES_MULTITHREAD)
    RELEASE_LOCK(GLOCK);
#endif
    return(FILE_HANDLES[ handle ].current_pos);
}



/* =======================================================

   FUNCTION:   ResSetDirectory

   PURPOSE:    Sets the current working directory
               Only useful is using the hierarchical
               model.

   PARAMETERS: Directory name.

   RETURNS:    TRUE (pass) / FALSE (fail).

   ======================================================= */

RES_EXPORT int ResSetDirectory(const char * pathname)
{
    HASH_ENTRY * entry;
#if( RES_COERCE_FILENAMES )
    char full[_MAX_PATH];
    int  len;
#endif

#if( RES_DEBUG_PARAMS )

    if ( not pathname or not (*pathname))
    {
        SAY_ERROR(RES_ERR_INCORRECT_PARAMETER, "ResSetDirectory");
        return(FALSE);
    }

    if ( not GLOBAL_PATH_LIST)
    {
        SAY_ERROR(RES_ERR_MUST_CREATE_PATH, "ResSetDirectory");
        return(FALSE);
    }

#endif

#if( RES_COERCE_FILENAMES )
    res_fullpath(full, pathname, (_MAX_PATH - 2));

    len = strlen(full);

    if (full[len - 1] not_eq ASCII_BACKSLASH)
    {
        full[len++] = ASCII_BACKSLASH;
        full[len++] = '\0';
    }

    pathname = full;
#endif /* RES_COERCE_FILENAMES */

    entry = hash_find(pathname, GLOBAL_HASH_TABLE);

    if ( not entry or not entry -> dir)
    {
        SAY_ERROR(RES_ERR_PATH_NOT_FOUND, pathname);
        return(FALSE);
    }

    sort_path();    /* sort path BEFORE forcing one of the entries
                       to the top.  Since we subjigate all of the
                       paths that are based on the CD, this allows
                       the caller to force a CD path to be on top
                       of the search path.  All of the other CD
                       paths, however, are still at the bottom. */

#if( not RES_USE_FLAT_MODEL )
    /* Force to the head of the list */
    GLOBAL_PATH_LIST = LIST_REMOVE(GLOBAL_PATH_LIST, entry -> dir);
    GLOBAL_PATH_LIST = LIST_APPEND(GLOBAL_PATH_LIST, entry -> dir);
#endif /* RES_USE_FLAT_MODEL */

    strcpy(GLOBAL_CURRENT_PATH, pathname);

    /* If we allow alias attach points ('fake' directories to
       attach archive's onto) we should disable the error
       reporting. */


#if 0   // GFG May 05/98
#if( not RES_ALLOW_ALIAS )

    if (_chdir(pathname))
    {
        SAY_ERROR(errno, "ResSetDirectory");
    }

#else
    _chdir(pathname);
#endif
#endif

    GLOBAL_CURRENT_DRIVE = pathname[0];

    IF_LOG(LOG("set dir: %s\n", pathname));
    return(TRUE);
}



/* =======================================================

   FUNCTION:   ResGetDirectory

   PURPOSE:    Returns the current working directory

   PARAMETERS: Buffer to copy the current working
               directory name in to.

   RETURNS:    Number of characters written.

   ======================================================= */

RES_EXPORT int ResGetDirectory(char * buffer)
{
    char * check;

#if( RES_DEBUG_PARAMS )

    if ( not buffer)
    {
        SAY_ERROR(RES_ERR_INCORRECT_PARAMETER, "ResGetDirectory");
        return(0);
    }

    if ( not GLOBAL_PATH_LIST)
    {
        SAY_ERROR(RES_ERR_MUST_CREATE_PATH, "ResSetDirectory");
        return(0);
    }

#endif

#if( not RES_USE_FLAT_MODEL )
    check = strcpy(buffer, ((HASH_TABLE *)(GLOBAL_PATH_LIST -> node)) -> name);
#else
    check = strcpy(buffer, GLOBAL_CURRENT_PATH);
#endif /* not RES_USE_FLAT_MODEL */

    IF_LOG(LOG("get dir: %s\n", buffer));

    if (check)
        return(strlen(buffer));

    return(0);      /* nothing was copied into buffer */
}



/* =======================================================

   FUNCTION:   ResGetPath

   PURPOSE:    Returns the nth entry in the search path.

   PARAMETERS: Index, buffer to copy directory name in to.

   RETURNS:    Number of characters written to buffer.

   ======================================================= */

RES_EXPORT int ResGetPath(int idx, char * buffer)
{
    LIST * list;
    char * check = NULL;

#if( RES_DEBUG_PARAMS )

    if ( not buffer or idx < 0)
    {
        return(0);
    }

#endif /* RES_DEBUG_PARAMS */

    list = LIST_NTH(GLOBAL_PATH_LIST, idx);

    if (list)
        check = strcpy(buffer, ((HASH_TABLE *)(list -> node)) -> name);
    else
        *buffer = 0x00;

    return(check ? strlen(buffer) : 0);
}



/* =======================================================

   FUNCTION:   ResGetArchive

   PURPOSE:    Returns the archive name for the given
               handle.

   PARAMETERS: Archive handle, buffer to copy directory
               name in to.

   RETURNS:    Number of characters written to buffer.

   ======================================================= */

RES_EXPORT int ResGetArchive(int handle, char * buffer)
{
    ARCHIVE    * archive = NULL;
    LIST       * list = NULL;

#if( RES_DEBUG_PARAMS )

    if ( not ARCHIVE_LIST)
    {
        *buffer = 0x00;
        return(0);
    }

#endif /* RES_DEBUG_PARAMS */

    /* using the handle, search the list for the structure */

    for (list = ARCHIVE_LIST; list; list = list -> next)
    {
        archive = (ARCHIVE *)list -> node;

        if (archive -> os_handle == handle)
            break;
    }

    if ( not list)    /* couldn't find it, may already have been closed - or handle is incorrect */
    {
        *buffer = 0x00;
        return(0);
    }

    strcpy(buffer, archive -> name);

    return(strlen(buffer));
}



/* =======================================================

   FUNCTION:   ResWhereIs

   PURPOSE:    Find the location of a specified file.

   PARAMETERS: Filename, optional ptr to buffer for
               storage of directory path.

   RETURNS:    Bit pattern made up of:

                    RES_HD          0x00010000
                    RES_CD          0x00020000
                    RES_NET         0x00040000
                    RES_ARCHIVE     0x00080000
                    RES_FLOPPY      0x00100000

                    -and-

                    CD number stored in low word
                    First cd is 1.  0 indicates
                    cd is unknown or not
                    applicable.

               or -1 indicating an error.

   ======================================================= */

RES_EXPORT int ResWhereIs(char * filename, char * path)
{
    HASH_ENTRY * entry;

    int retval = 0,
        type;

#if( RES_DEBUG_PARAMS )

    if ( not filename)
    {
        SAY_ERROR(RES_ERR_INCORRECT_PARAMETER, "ResGetDirectory");
        return(-1);
    }

#endif
#if (RES_MULTITHREAD)
    REQUEST_LOCK(GLOCK);
#endif

#if( not RES_USE_FLAT_MODEL )
    entry = hash_find_table(filename, NULL);
#else /* flat model */
    //    entry = hash_find( file, GLOBAL_HASH_TABLE );  /* GFG  31/01/98 */
    entry = hash_find(filename, GLOBAL_HASH_TABLE);
#endif /* not RES_USE_FLAT_MODEL */

    if ( not entry)
    {
#if (RES_MULTITHREAD)
        RELEASE_LOCK(GLOCK);
#endif
        return(-1);
    }

    if (entry -> archive not_eq -1)
        retval or_eq RES_ARCHIVE;

    type = RES_DEVICES[ entry -> volume ].type;

    if (type == DRIVE_CDROM)
    {
        retval or_eq RES_CD;
        retval or_eq RES_DEVICES[ entry -> volume ].id;
    }

    if (type == DRIVE_REMOTE)
        retval or_eq RES_NET;

    if (type == DRIVE_FIXED)
        retval or_eq RES_HD;

    if (type == DRIVE_REMOVABLE)
        retval or_eq RES_FLOPPY;

    if (path)
        strcpy(path, GLOBAL_SEARCH_PATH[ entry -> directory ]);

    IF_LOG(LOG("where is: %s\n", filename));
#if (RES_MULTITHREAD)
    RELEASE_LOCK(GLOCK);
#endif

    return(retval);
}



/* =======================================================

   FUNCTION:   ResWhichCD

   PURPOSE:    Find out which cd is currently mounted.

   PARAMETERS: None.

   RETURNS:    CD Number.
               1 is the first cd
               CD_MAX is the last cd
               -1 is no cd is currently mounted

   ======================================================= */

RES_EXPORT int ResWhichCD(void)
{
    IF_LOG(LOG("which cd: %d\n", GLOBAL_CURRENT_CD));
    return(GLOBAL_CURRENT_CD);
}



/* =======================================================

   FUNCTION:   ResWriteTOC

   PURPOSE:    Writes the Unified Table of Contents
               file.  This file is the global table of
               contents of all files contained on cd-roms
               for a given project.

   PARAMETERS: Filename to write file to.

   RETURNS:    TRUE (pass) / FALSE (fail).

   ======================================================= */

RES_EXPORT int ResWriteTOC(char * filename)
{
    filename;

    IF_LOG(LOG("write t.o.c.: %s\n", filename));

    return(FALSE);
}

/* =======================================================

   FUNCTION:   ResStatusFile

   PURPOSE:    Query statistics about an opened file.

   PARAMETERS: File handle, buffer to copy statistics.

   RETURNS:    TRUE (pass) / FALSE (fail).

   ======================================================= */

RES_EXPORT int ResStatusFile(const char * filename, RES_STAT * stat_buffer)
{
    HASH_ENTRY * entry;
    LIST * list;
    int hit;

    char * src;

#if( RES_DEBUG_PARAMS )

    if ( not filename or not stat_buffer)
    {
        SAY_ERROR(RES_ERR_INCORRECT_PARAMETER, "ResStatusFile");
        return(FALSE);
    }

#endif /* RES_DEBUG_PARAMS */

    if ( not GLOBAL_SEARCH_INDEX)
    {
        SAY_ERROR(RES_ERR_MUST_CREATE_PATH, "ResStatusFile");
        return(FALSE);
    }

#if (RES_MULTITHREAD)
    REQUEST_LOCK(GLOCK);
#endif

    entry = hash_find_table(filename, NULL);

    if ( not entry)
    {
#if (RES_MULTITHREAD)
        RELEASE_LOCK(GLOCK);
#endif
        return(FALSE);
    }

    stat_buffer -> size       = entry -> size;
    stat_buffer -> csize      = entry -> csize;
    stat_buffer -> volume     = entry -> volume;
    stat_buffer -> attributes = entry -> attrib;
    stat_buffer -> archive    = entry -> archive;

    stat_buffer -> directory  = -1;

    src = GLOBAL_SEARCH_PATH[ entry -> directory ];

    hit = 0;

    for (list = GLOBAL_PATH_LIST; list; list = list -> next)
    {
        if ( not (strcmp(src, ((HASH_TABLE *)(list -> node)) -> name)))
        {
            stat_buffer -> directory = hit;
            break;
        }

        hit++;
    }

    IF_LOG(LOG("stat: %s\n", filename));

#if (RES_MULTITHREAD)
    RELEASE_LOCK(GLOCK);
#endif
    return(TRUE);
}



/* =======================================================

   FUNCTION:   ResSetCallback

   PURPOSE:    Set one of the user-defined callback ptrs.

   PARAMETERS: Which callback, pointer to a function
               that returns a integer value (PFI).

   RETURNS:    Any previously set callback, or NULL
               (on error).

   ======================================================= */

RES_EXPORT PFI ResSetCallback(int which, PFI func)
{
    PFI old_ptr;

#if( RES_DEBUG_PARAMS )

    if ((which < 0) or (which >=  NUMBER_OF_CALLBACKS))
    {
        SAY_ERROR(RES_ERR_INCORRECT_PARAMETER, "ResSetCallback");
        return(NULL);
    }

#endif /* RES_DEBUG_PARAMS */

    old_ptr = RES_CALLBACK[ which ];
    RES_CALLBACK[ which ] = func;

    IF_LOG(LOG("set callback: %d\n", which));

    return(old_ptr);
}



/* =======================================================

   FUNCTION:   ResGetCallback

   PURPOSE:    Query for a specified user defined
               callback.

   PARAMETERS: Which callback.

   RETURNS:    Any previously set callback ptr, or
               NULL (on error).

   ======================================================= */

RES_EXPORT PFI ResGetCallback(int which)
{
#if( RES_DEBUG_PARAMS )

    if ((which < 0) or (which >= NUMBER_OF_CALLBACKS))
    {
        SAY_ERROR(RES_ERR_INCORRECT_PARAMETER, "ResGetCallback");
        return(NULL);
    }

#endif /* RES_DEBUG_PARAMS */

    IF_LOG(LOG("get callback: %d\n", which));

    return(RES_CALLBACK[ which ]);
}



/* =======================================================

    FUNCTION:   ResCreatePath

    PURPOSE:    Initialize a search path for the resource
                manager.  The resource manager maintains
                a separate hash    table for each directory's
                contents.

    PARAMS:     Ptr to pathname to parse.

    RETURNS:    TRUE (pass) / FALSE (fail)

                A return value of false indicates that the
                filename was not interprettable.

   ======================================================= */

RES_EXPORT int ResCreatePath(char * path, int recurse)
{
    LIST * list;

#if( RES_DEBUG_VERSON )

    if ( not path)
    {
        SAY_ERROR(RES_ERR_INCORRECT_PARAMETER, "ResCreatePath");
        return(FALSE);
    }

#endif /* RES_DEBUG_VERSON */
#if (RES_MULTITHREAD)
    REQUEST_LOCK(GLOCK);
#endif

    IF_LOG(LOG("create path: %s\n", path));

    if (GLOBAL_HASH_TABLE)
        hash_destroy(GLOBAL_HASH_TABLE);

    if (GLOBAL_PATH_LIST)
    {
        for (list = GLOBAL_PATH_LIST; list; list = list -> next)
        {
#if( RES_USE_FLAT_MODEL )
#ifdef USE_SH_POOLS
            MemFreePtr(list -> node);
#else
            MemFree(list -> node);
#endif
#else
            hash_destroy((HASH_TABLE *)list -> node);
#endif /* RES_USE_FLAT_MODEL */
        }

        LIST_DESTROY(GLOBAL_PATH_LIST, NULL);
    }

    GLOBAL_PATH_LIST = NULL;

    GLOBAL_HASH_TABLE = hash_create(HASH_TABLE_SIZE, "Global");
#if (RES_MULTITHREAD)
    RELEASE_LOCK(GLOCK);
#endif

    if ( not GLOBAL_HASH_TABLE)
        return(FALSE);

    return(ResAddPath(path, recurse));
}



/* =======================================================

    FUNCTION:   ResAddPath

    PURPOSE:    Add a search path to the resource manager.
                This process includes hashing all of the
                directory names found in the indicated
                directory and either a) adding them to
                the global hash directory if using the
                flat model -or- b) adding them to a hash
                table created specifically for this
                directory.

    PARAMS:     Ptr to pathname to parse.

    RETURNS:    TRUE (pass) / FALSE (fail)

                A return value of false indicates that
                the filename was not interprettable.

   ======================================================= */

RES_EXPORT int ResAddPath(char * path, int recurse)
{
    struct _finddata_t data;

    int  length = 0,
         count = 0,
         done = 0,
         refresh = FALSE,
         full_yet = FALSE,
         retval = TRUE,
         filenum = 0, filecount = 0;

    long directory = 0;

    char tmp_path[ _MAX_PATH ] = {0};
    char buffer[ _MAX_PATH ] = {0};

    HASH_TABLE * local_table = NULL;
    HASH_ENTRY * entry = NULL;

    char vol_was = 0;
    int  dir_was = 0;

    struct _finddata_t  *file_data = NULL;

    int currentDrive;
    char currentPath[ _MAX_PATH];

    /* Save original drive/path.*/
    currentDrive = _getdrive();
    _getdcwd(currentDrive, currentPath, _MAX_PATH);



#ifdef RES_NO_REPEATED_ADDPATHS

    if (get_dir_index(path) not_eq -1)
    {
        return TRUE;
    }

#endif

    IF_LOG(LOG("adding path: %s\n", path));

#if( RES_DEBUG_PARAMS )

    if ( not path)
    {
        SAY_ERROR(RES_ERR_INCORRECT_PARAMETER, "ResAddPath");
        return(FALSE);
    }

    if ( not GLOBAL_HASH_TABLE)
    {
        printf("You must first call ResCreatePath()\n");
        return(FALSE);
    }

#endif /* RES_DEBUG_VERSON */

#if( RES_DEBUG_VERSION )

    if (GLOBAL_SEARCH_INDEX >= (MAX_DIRECTORIES - 1))
    {
        assert( not "Exceeded MAX_DIRECTORIES as defined in omni.h");
        //        SAY_ERROR( RES_ERR_TOO_MANY_DIRECTORIES, "ResAddPath" );
        return(FALSE);
    }

#endif




    /* The hash index must be determined from hashing the full pathname */

    res_fullpath(tmp_path, path, (_MAX_PATH - 2));   /* non-portable */

#if( not RES_USE_FLAT_MODEL )
    entry = hash_find(tmp_path, GLOBAL_HASH_TABLE);

    if (entry)
        refresh = TRUE;

#endif

    /* we need a trailing backslash */

    length = strlen(tmp_path);

    if (tmp_path[ length - 1 ] not_eq ASCII_BACKSLASH)
    {
        tmp_path[ length++ ] = ASCII_BACKSLASH;
        tmp_path[ length ] = '\0';
    }

    /* the size of the hash table for this directory will be defined by
       the macro HASH_OPTIMAL_RATIO times the number of files in the
       directory (a small price to pay) */

    filecount = count = ResCountDirectory(tmp_path, &file_data);

    if (count == -1)
    {
#ifdef USE_SH_POOLS

        if (file_data) MemFreePtr(file_data);

#else

        if (file_data) MemFree(file_data);

#endif
        return FALSE;
    }

    count = (int)((float)count * (float)(HASH_OPTIMAL_RATIO));

    count = MAX(count, 10);   /* minimum of 10 entries set aside for a directory */

    strcpy(buffer, tmp_path);

    length = strlen(tmp_path);      /* append wildcard to the filename (*.*) */
    tmp_path[ length++ ] = ASCII_ASTERISK;
    tmp_path[ length++ ] = ASCII_PERIOD;
    tmp_path[ length++ ] = ASCII_ASTERISK;
    tmp_path[ length ]   = '\0';

#if( not RES_USE_FLAT_MODEL )

    /* for the hierarchical model, we create a new hash table for the directory to
       be added.  Then we insert a hashed entry into the GLOBAL_HASH_TABLE which
       points to this new table. */

#if 1
    memcpy(&data, file_data, sizeof(struct _finddata_t));
#else
    directory = _findfirst(tmp_path, &data);        /* make sure it exists                              */

    if (directory == -1)
    {
        SAY_ERROR(RES_ERR_PATH_NOT_FOUND, tmp_path);
        return(FALSE);
    }

    _findclose(directory);

#endif

    if ( not entry or not entry -> dir)
        local_table = hash_create(count, buffer);   /* create a new table                               */
    else if (entry)
        local_table = entry -> dir;

    if ( not local_table)
        return(FALSE);

    strcpy(data.name, buffer);                      /* insert a dummy entry into the global hash table  */
    data.attrib = _A_SUBDIR bitor (unsigned int)FORCE_BIT;
    data.time_create = 0;
    data.time_access = 0;
    data.size = 0;

    if ( not entry)
        entry = hash_add(&data, GLOBAL_HASH_TABLE);

    if ( not entry)
        return(FALSE);

    if ( not entry -> dir)
        entry -> dir = local_table;

    if ( not refresh)
    {
        entry -> offset = 0;
        entry -> csize  = 0;
        entry -> method = 0;
        entry -> archive = -1;
        entry -> file_position = -1;
        entry -> volume = (char)(toupper(tmp_path[0]) - 'A');
        entry -> directory = GLOBAL_SEARCH_INDEX;

        GLOBAL_PATH_LIST = LIST_APPEND(GLOBAL_PATH_LIST, local_table);
    }

#else
    local_table = GLOBAL_HASH_TABLE;                /* flat mode - all entries go into the root         */
#endif  /* not RES_USE_FLAT_MODEL */

    /* enter the files into the local hash table, keeping count of the total
       number of entries. */

    done = 0;

    vol_was = (char)(toupper(tmp_path[0]) - 'A');



    if ( not refresh)
    {
        dir_was = GLOBAL_SEARCH_INDEX;

        GLOBAL_SEARCH_PATH[ GLOBAL_SEARCH_INDEX++ ] = MemStrDup(buffer);

        strcpy(GLOBAL_CURRENT_PATH, buffer);
    }
    else
        dir_was = get_dir_index(path);

    //    if( _chdir( buffer ))        // GFG MAY 05 / 98
    //        SAY_ERROR( errno, "ResAddPath" );

#if 1
    memcpy(&data, file_data, sizeof(struct _finddata_t));
    directory = 1;
    filenum = 0;
#else
    directory = _findfirst(tmp_path, &data);
#endif

    if (directory not_eq -1)
    {
        /* integral volume id bitand path index */

        while ( not done)
        {

            /* don't add directories to the hash table,
               ResTreeAdd is used to recurse directories */

            if ( not (data.attrib bitand _A_SUBDIR))
            {

                /* Reject empty files before calling hash_add.  This
                   allows file creation to still be able to use hash_add. */

#if( RES_REJECT_EMPTY_FILES )
                if (data.size == 0)
                {
                    IF_LOG(LOG("empty file rejected: %s\n", data.name));
#if 1

                    if (filenum < filecount - 1)
                    {
                        filenum++;
                        memcpy(&data, file_data + filenum, sizeof(struct _finddata_t));
                    }
                    else
                        done = filenum;

#else
                    done = _findnext(directory, &data);
#endif
                    continue;
                }

#endif /* not RES_ALLOW_EMPTY_FILES */

                if (refresh)
                {
                    if (hash_find(data.name, local_table))
                    {
#if 1

                        if (filenum < filecount - 1)
                        {
                            filenum++;
                            memcpy(&data, file_data + filenum, sizeof(struct _finddata_t));
                        }
                        else
                            done = filenum;

#else

                        done = _findnext(directory, &data);
#endif
                        continue;
                    }

                    entry = hash_find(data.name, local_table);

                    if ( not entry)
                        entry = hash_add(&data, local_table);

                    if (entry)
                    {
                        entry -> size = data.size;
                        entry -> attrib = data.attrib bitor FORCE_BIT;
                    }
                }
                else
                {
                    entry = hash_add(&data, local_table);
                }


                if (entry)    /* empty files may be rejected from hash_add */
                {

                    IF_LOG(LOG("add: %s\n", entry -> name));

                    entry -> offset = 0;
                    entry -> csize  = 0;
                    entry -> method = 0;
                    entry -> volume = vol_was;
                    entry -> directory = dir_was;
                    entry -> archive = -1;
                    entry -> file_position = -1;
                }
            }
            else      /* if we want to add an entire directory tree, recurse is TRUE */
            {
                if (recurse and not full_yet)
                {
                    if (strcmp(data.name, ".") and strcmp(data.name, ".."))
                    {
                        int idx, ln;

                        ln = strlen(tmp_path);

                        for (idx = ln - 1; idx, tmp_path[idx] not_eq ASCII_BACKSLASH; idx--) ;

                        strncpy(buffer, tmp_path, idx);
                        sprintf(&buffer[idx], "\\%s\\", data.name);

                        if ( not ResAddPath(buffer, TRUE))     /* Recursively call this function. */
                        {
                            full_yet = TRUE;               /* We want to continue adding this directory, and THEN */
                            retval = FALSE;                /* trickle up a return flag.                           */
                        }
                    }
                }
            }

#if 1

            if (filenum < filecount - 1)
            {
                filenum++;
                memcpy(&data, file_data + filenum, sizeof(struct _finddata_t));
            }
            else
                done = filenum;

#else

            done = _findnext(directory, &data);
#endif

        }

#if 0
        _findclose(directory);   /* done */
#endif

    }

    sort_path();

    if (file_data)
    {
#ifdef USE_SH_POOLS
        MemFreePtr(file_data);
#else
        MemFree(file_data);
#endif
        file_data = NULL;
    }

    /* Restore original drive.*/  // GFG MAY 05 /98
    _chdrive(currentDrive);
    _chdir(currentPath);


    return(retval);
}



/* =======================================================

    FUNCTION:   ResBuildPathname

    PURPOSE:    Builds a complex pathname.  Whether the
                input is a relative path, missing the
                trailing slash, or needs to be built off
                of a system directory, the output is
                always a full pathname.

    PARAMS:     Optional enum representing a special
                system path.

                RES_DIR_INSTALL    - path program was installed.
                RES_DIR_WINDOWS - windows directory.
                RES_DIR_CURR    - current path.
                RES_DIR_CD        - path to the cd-rom
                RES_DIR_HD        - path to the boot hd
                RES_DIR_TEMP    - system temp directory
                RES_DIR_SAVE    - save game directory
                RES_DIR_NONE    - none

    RETURNS:    Number of characters written to path_out.

   ======================================================= */

RES_EXPORT int ResBuildPathname(int index, char * path_in, char * path_out)
{
    char tmp[ _MAX_PATH ] = {0};
    int  len = 0;
    char * ptr = NULL;

#if( RES_DEBUG_PARAMS )

    if ((index < RES_DIR_NONE)  or
        (index >= RES_DIR_LAST) or
        ( not path_out))
    {
        SAY_ERROR(RES_ERR_INCORRECT_PARAMETER, "ResBuildPathname");
        return(-1);
    }


    if ((index not_eq RES_DIR_NONE) and (RES_PATH[ index ] == NULL))
    {
        SAY_ERROR(RES_ERR_NO_SYSTEM_PATH, "ResBuildPathname");
        *path_out = '\0';
        return(-1);
    }

#endif /* RES_DEBUG_PARAMS */


    /* path_in is optional, but, of course without it you get the same
       effect as strcpy( path_out, RES_PATH[ index ]) */

    if (path_in)
    {
        if (index not_eq RES_DIR_NONE)                     /* since all these end with a slash  */
            while (*path_in == ASCII_BACKSLASH)     /* trim leading backslashes             */
                path_in++;

        strcpy(tmp, path_in);
        ptr = tmp;

        while (*ptr)
        {
            if (*ptr == ASCII_FORESLASH)     /* substitute backslash for forward slashes */
                *ptr = ASCII_BACKSLASH;

            *ptr = (char)(toupper(*ptr));           /* force to upper case */

            ptr++;
        }

        /* trailing backslash */

        len = strlen(tmp);

        if (len and (tmp[len - 1] not_eq ASCII_BACKSLASH))
        {
            tmp[len++] = ASCII_BACKSLASH;
            tmp[len] = '\0';
        }
    }
    else
    {
        tmp[0] = '\0';
        tmp[1] = '\0';
    }


    /* if there is a system path index */

    if (index not_eq RES_DIR_NONE)
        sprintf(path_out, "%s%s", RES_PATH[ index ], tmp);
    else
        strcpy(path_out, tmp);

    return(len);
}




/* =======================================================

    FUNCTION:   ResCountDirectory

    PURPOSE:    Count the number of files within a
                directory.

    PARAMS:     Ptr to a pathname.

    RETURNS:    Number of files found within given
                directory.

   ======================================================= */

RES_EXPORT int ResCountDirectory(char * path , struct _finddata_t **file_data)
{
    //    HASH_ENTRY * entry=NULL;

    char fullpath[ _MAX_PATH ];

    //    struct _finddata_t data;
    struct _finddata_t *data;

    int dir,
        len,
        count = -1,
        data_count = 0,
        ret;


    IF_LOG(LOG("count: %s\n", path));

#if( RES_DEBUG_PARAMS )

    if ( not path)
    {
        SAY_ERROR(RES_ERR_INCORRECT_PARAMETER, "ResCountDirectory");
        return(-1);
    }

#endif /* RES_DEBUG_PARAMS */



    /* we need to make sure we have an absolute (full) path */

    res_fullpath(fullpath, path, (_MAX_PATH - 3));

#if( not RES_USE_FLAT_MODEL )

#if 0
    entry = hash_find(fullpath, GLOBAL_HASH_TABLE);

    if (entry and entry -> dir)
        return(((HASH_TABLE *)entry -> dir) -> num_entries);

#endif

#endif /* RES_USE_FLAT_MODEL */

    /* add a wildcard to the end of the path */

    len = strlen(fullpath);
    fullpath[ len++ ] = ASCII_ASTERISK;
    fullpath[ len++ ] = ASCII_PERIOD;
    fullpath[ len++ ] = ASCII_ASTERISK;
    fullpath[ len ] = '\0';

    data_count = RES_INIT_DIRECTORY_SIZE;
#ifdef USE_SH_POOLS
    *file_data = data = MemAllocPtr(gResmgrMemPool, data_count * sizeof(struct _finddata_t), 0);
#else
    *file_data = data = MemMalloc(data_count * sizeof(struct _finddata_t), "ResCountDirectory");
#endif

    if ( not data)
    {
        SAY_ERROR(RES_ERR_NO_MEMORY, "ResCountDirectory");
        return (-1);
    }


    /* try to open the directory */

    //    dir = _findfirst( fullpath, &data );
    dir = _findfirst(fullpath, data);

    if (dir not_eq -1)
    {
        count = 1;

        do
        {
            if (count >= data_count)
            {
                data_count += RES_INIT_DIRECTORY_SIZE;
#ifdef USE_SH_POOLS
                *file_data =  MemReAllocPtr(*file_data, data_count * sizeof(struct _finddata_t), 0);
#else
                *file_data =  MemRealloc(*file_data, data_count * sizeof(struct _finddata_t));
#endif
                data = *file_data + (count - 1);
            }

            data++;
            ret =  _findnext(dir, data);

            if ( not ret)
            {
                count++;
            }
        }
        while ( not ret);


        _findclose(dir);
    }
    else
    {
        IF_LOG(LOG("Could not open directory %s\n", fullpath));
    }

    return(count);
}





/* =======================================================

    FUNCTION:   ResAssignPath

    PURPOSE:    What C++ was invented for.  ResAssignPath
                allows a public method to initialize
                private data.

    PARAMS:     Enumerated value that represents one of
                the slots open for system paths, ptr to
                a pathname to assign to that slot.

    RETURNS:    None.

   ======================================================= */

RES_EXPORT void ResAssignPath(int index, char * path)
{
    char * ptr;
    int len;

#if( RES_DEBUG_PARAMS )

    if ((index <= RES_DIR_NONE) or
        (index >= RES_DIR_LAST) or
        ( not path))
    {
        SAY_ERROR(RES_ERR_INCORRECT_PARAMETER, "ResAssignPath");
        return;
    }

#endif /* RES_DEBUG_PARAMS */

    if (RES_PATH[ index ])
#ifdef USE_SH_POOLS
        MemFreePtr(RES_PATH[ index ]);

#else
        MemFree(RES_PATH[ index ]);
#endif

#ifdef USE_SH_POOLS
    ptr = (char *)MemAllocPtr(gResmgrMemPool, _MAX_PATH, 0);
#else
    ptr = (char *)MemMalloc(_MAX_PATH, path);
#endif

    if ( not ptr)
    {
        SAY_ERROR(RES_ERR_NO_MEMORY, "Assign path");
        return;
    }

    /* All of the pathnames within the RES_PATH array should end with
       a backslash, therefore, we want to trim any leading backslashes
       from the path here. */

    while (*path == ASCII_BACKSLASH)
        path++;

    strcpy(ptr, path);

    len = strlen(ptr);

    if (len and (ptr[ len - 1 ] not_eq ASCII_BACKSLASH))
    {
        ptr[len++] = ASCII_BACKSLASH;
        ptr[len] = '\0';
    }

    RES_PATH[ index ] = ptr;

    IF_LOG(LOG("assign: %s [%d]\n", ptr, index));
}



/* =======================================================

    FUNCTION:   ResAsynchRead

    PURPOSE:    Perform an asynchronous read.

    PARAMETERS: File int, buffer to read data in to,
                function ptr for callback on completion.

    RETURNS:    TRUE (pass) / FALSE (fail).

                True means the reader thread was
                spawned to int the task - not that
                the read was actually completed.

                Confirmation that the read was actually
                performed is intd by the callback.

                IMPORTANT NOTE:  Remember that the callback
                is executed by the reader thread.  Since
                this thread is necessarily a low priority
                thread - AND - that the callback will
                exist within the context of this thread
                (it will be the reader threads instruction
                pointer - not the main instruction
                pointer that executes the callback), you
                should take care that the callback function
                is short, simple and finite

   ======================================================= */

RES_EXPORT int ResAsynchRead(int file, void * buffer, PFV callback)
{
    ASYNCH_DATA * data;
    int thread_id;

#if( RES_DEBUG_PARAMS )

    if ( not buffer or (file < 0) or (file > MAX_FILE_HANDLES))
    {
        SAY_ERROR(RES_ERR_INCORRECT_PARAMETER, "ResAsynchRead");
        return(FALSE);
    }

#endif

    IF_LOG(LOG("asynch read %s\n", FILE_HANDLES[file].filename));

#ifdef USE_SH_POOLS
    data = (ASYNCH_DATA *)MemAllocPtr(gResmgrMemPool, sizeof(ASYNCH_DATA), 0);
#else
    data = (ASYNCH_DATA *)MemMalloc(sizeof(ASYNCH_DATA), "Asynch data");
#endif

    if ( not data)
    {
        SAY_ERROR(RES_ERR_NO_MEMORY, NULL);
        return(FALSE);
    }

    data -> file = file;
    data -> buffer = buffer;
    data -> callback = callback;

    thread_id = _beginthread(asynch_read, 128 /*stack size*/, (void *)(data));

    if (thread_id == -1)
    {
        SAY_ERROR(RES_ERR_COULDNT_SPAWN_THREAD, NULL);
        return(FALSE);
    }

    return(TRUE);
}



/* =======================================================

    FUNCTION:   ResAsynchWrite

    PURPOSE:    Perform an asynchronous write.

    PARAMETERS: File int, buffer to read data from,
                function ptr for callback on completion.

    RETURNS:    TRUE (pass) / FALSE (fail).

                True means the reader thread was
                spawned to int the task - not that
                the write was actually completed.

                Confirmation that the write was actually
                performed is intd by the callback.

                IMPORTANT NOTE:  Remember that the callback
                is executed by the writer thread.  Since
                this thread is necessarily a low priority
                thread - AND - that the callback will
                exist within the context of this thread
                (it will be the writer threads instruction
                pointer - not the main instruction
                pointer that executes the callback), you
                should take care that the callback function
                is short, simple and finite

   ======================================================= */

RES_EXPORT int ResAsynchWrite(int file, void * buffer, PFV callback)
{
    ASYNCH_DATA * data;
    int thread_id;

#if( RES_DEBUG_PARAMS )

    if ( not buffer or (file < 0) or (file > MAX_FILE_HANDLES))
    {
        SAY_ERROR(RES_ERR_INCORRECT_PARAMETER, "ResAsynchWrite");
        return(FALSE);
    }

#endif

    IF_LOG(LOG("asynch write %s\n", FILE_HANDLES[file].filename));

#ifdef USE_SH_POOLS
    data = (ASYNCH_DATA *)MemAllocPtr(gResmgrMemPool, sizeof(ASYNCH_DATA), 0);
#else
    data = (ASYNCH_DATA *)MemMalloc(sizeof(ASYNCH_DATA), "Asynch data");
#endif

    if ( not data)
    {
        SAY_ERROR(RES_ERR_NO_MEMORY, NULL);
        return(FALSE);
    }

    data -> file = file;
    data -> buffer = buffer;
    data -> callback = callback;

    thread_id = _beginthread(asynch_write, 128 /*stack size*/, (void *)(data));

    //SetThreadPriority( thread_id, THREAD_PRIORITY_LOWEST );
    //#error PROTOTYPE THIS


    if (thread_id == -1)
    {
        SAY_ERROR(RES_ERR_COULDNT_SPAWN_THREAD, NULL);
        return(FALSE);
    }

    IF_DEBUG(LOG("Write thread (%d) spawned.", thread_id));
    return(TRUE);
}



/* =======================================================

    FUNCTION:   ResExtractFile

    PURPOSE:    Convenience function for extacting
                members of an archive.

    PARAMETERS: Ptr to destination filename, ptr to
                source filename.

    RETURNS:    Number of bytes written, or -1 in
                case of an error.

   ======================================================= */

RES_EXPORT int ResExtractFile(const char * dst, const char * src)
{
    char * buffer;
    int    handle;
    unsigned int size;

    const char * fdst = dst; /* expanded pathnames */
    const char * fsrc = src;

#if( RES_COERCE_FILENAMES )
    char   fulldst[_MAX_PATH],
           fullsrc[_MAX_PATH];
#endif /*RES_COERCE_FILENAMES */

#if( RES_DEBUG_PARAMS )

    if ( not dst or not src)
    {
        SAY_ERROR(RES_ERR_INCORRECT_PARAMETER, "ResExtract");
        return(-1);
    }

#endif /* RES_DEBUG_PARAMS */

#if( RES_COERCE_FILENAMES )

    if (strchr(dst, ASCII_BACKSLASH))
    {
        res_fullpath(fulldst, dst, _MAX_PATH);
        fdst = fulldst;
    }

    if (strchr(src, ASCII_BACKSLASH))
    {
        res_fullpath(fullsrc, src, _MAX_PATH);
        fsrc = fullsrc;
    }

#endif /* RES_COERCE_FILENAMES */

    buffer = ResLoadFile(fsrc, NULL, &size);

    if (buffer)
    {
        handle = ResOpenFile(fdst, _O_CREAT bitor _O_WRONLY bitor _O_BINARY);

        if (handle not_eq -1)
        {
            size = ResWriteFile(handle, buffer, size);
            ResCloseFile(handle);
        }

        ResUnloadFile(buffer);
    }
    else
        return(-1);

    return(size);
}



/* =======================================================

    FUNCTION:   ResPurge

    PURPOSE:    Purge entries from the Resource Manager
                based on identical archive handle,
                volume id, and/or directory path.

    PARAMETERS: Ptr to archive handle, volume id,
                directory handle.  Pass NULL for any
                unused parameter.

    RETURNS:    None.

   ======================================================= */

//RES_EXPORT void ResPurge( char * archive, char * volume, char * directory, char * filename )
RES_EXPORT void ResPurge(const char * archive, const char * volume, const int * directory, const char * filename)
{
    LIST * list;

    if ( not archive and not volume and not directory and not filename)
    {
        SAY_ERROR(RES_ERR_INCORRECT_PARAMETER, "ResPurge");
        return;
    }

    if ( not GLOBAL_PATH_LIST)
        return;

    for (list = GLOBAL_PATH_LIST; list; list = list -> next)
        hash_purge((HASH_TABLE*)list -> node, archive, volume, directory, filename);
}





#if( RES_STREAMING_IO )

/* -----------------------------------------------------------------------------------------------

    STREAM I/O Functions

    All of the functions listed here should work.

    Basically, if you want to perform streaming i/o, use ResFOpen the same way you would
    use fopen.  The returned value is a lightweight pointer to a _iob struct (FILE *).

    From this point on, you can use this file pointer as if it were truly returned from
    fopen.  When you're ready to close the file, use ResFClose.

    example:

    {
        FILE * file;
        char   name[255],
               buffer[1024];
        int    date,
               time;

        file = ResFOpen( "test.txt", "r" );

        if( not file )
            return;

        fscanf( file, "%s", &name );
        fscanf( file, "%d", &date );
        fscanf( file, "%d", &time );

        printf( "name: %s  date: %d  time: %d\n", name, date, time );

        fseek( file, 1024, SEEK_CUR );

        while( (t = fread( buffer, 1, 64, file )) == 64 )
            printf( "%s\n", buffer );

        ResFClose( file );
    }

    Regardless of whether this file is located on a hard-drive, cd-rom drive, or floppy --
    or whether the file is stored within a zip file -- or whether the file is compressed
    within a zip file -- all the stdio streaming functions should work seamlessly using the
    same FILE pointer.

   -----------------------------------------------------------------------------------------------

    These stream i/o functions are TOTALLY SUPPORTED

    clearerr        Clear error indicator for stream
    fclose            Close stream
    feof            Test for end of file on stream
    ferror            Test for error on stream
    fflush            Flush stream to buffer or storage device
    fgetc           Read character from stream (function versions of getc and getwc)
    fgets           Read string from stream
    fopen           Open stream
    fprintf         Write formatted data to stream
    fputc           Write a character to a stream (function versions of putc and putwc)
    fputs           Write string to stream
    fread            Read unformatted data from stream
    fscanf          Read formatted data from stream
    fwrite            Write unformatted data items to stream
    _getw            Read binary int from stream
    puts            Write line to stream
    _putw            Write binary int to stream
    vfprintf        Write formatted data to stream

    These stream i/o functions are NOT SUPPORTED

    _fdopen         Associate stream with handle to open file
    _flushall        Flush all streams to buffer or storage device


    These stream i/o functions are PARTIALLY SUPPORTED

                    These three functions act slightly differently from their normal
                    stdio behavior.  The file may be flushed, and ungetc will be
                    cleared on archive files.  All other behaviors are the same.

    fseek            Move file position to given location
    fsetpos            Set position indicator of stream
    rewind            Move file position to beginning of stream


                    This returns you a ResMgr handle if the file is found within
                    an archive, and an OS handle if the file is found loose on
                    hard-drive, cd, or floppy.

    _fileno            Get file handle associated with stream


                    These functions do not work on archive based files (but do on loose files):

    ftell            Get current file position
    fgetpos            Get position indicator of stream

                    This function is not guaranteed to work on archive files (but sometimes will):

    ungetc          Push character back onto stream
                    ungetc may work most of the time for all types of files, but
                    is not guarenteed to work on an archive file if:
                          ungetc is called on the first character (_cnt == _size),
                          ungetc is called twice in a row,
                          ungetc returned character is EOF.

   -------------------------------------------------------------------------------------------- */


/* the flags field within an _iob struct (internal version of FILE struct)
   is masked with the bit-fields found within stdio.h.  The highest value
   bit-field is 0x0200, and embarrassingly, I've munged my bit-fields into
   this same member, starting at 0x00010000 */

#define _IOARCHIVE  0x00010000
#define _IOLOOSE    0x00020000



/* =======================================================

    FUNCTION:   ResSetBuf

    PURPOSE:    Controls stream buffer location and size.
                Synonimous with setvbuf().

    PARAMETERS: FILE ptr, ptr to an allocated buffer, mode
                used for file buffering, size of buffer.

    RETURNS:    None.

                NOT CURRENTLY SUPPORTED

   ======================================================= */
#if 0
RES_EXPORT void ResSetbuf(FILE * file, void * buffer, int mode, size_t size)
{
}
#endif




/* =======================================================

    FUNCTION:   fopen / ResFOpen

    PURPOSE:    Open a file for streaming i/o

    PARAMETERS: Filename,

    RETURNS:    None.

   ======================================================= */



RES_EXPORT FILE * RES_FOPEN(const char * name, const char * mode)
{
    FILE * stream;
    int    write_flag = FALSE;

    HASH_ENTRY * entry;
    HASH_TABLE * table = NULL;

    char filename[_MAX_PATH],
         dirpath[_MAX_PATH];

    int  retval = 1;     /* to test return value of callbacks */
    int  dir_index;
    struct _finddata_t data;

    char tmp[LREC_SIZE];
    local_file_hdr lrec;

#if( RES_DEBUG_PARAMS )

    if ( not name or not mode)
    {
        SAY_ERROR(RES_ERR_INCORRECT_PARAMETER, "ResFOpen");
        return(FALSE);
    }

#endif /* RES_DEBUG_PARAMS */
#if (RES_MULTITHREAD)
    REQUEST_LOCK(GLOCK);
#endif


    /* if the mode string contains either a 'w' or a '+' we
       need to return an error if the file being operated on
       is an archive file (eventually, this may be otherwise) */

    if (strchr(mode, 'w') or strchr(mode, 'a'))
        write_flag = TRUE;


    /* find the file */

#if( not RES_USE_FLAT_MODEL )
    entry = hash_find_table(name, &table);          /* look through tables in search path order */
#else
    entry = hash_find(name, GLOBAL_HASH_TABLE);     /* look in the root hash table (flat model) */
#endif

    if ( not entry and table and not write_flag)
    {
        SAY_ERROR(RES_ERR_FILE_NOT_FOUND, name);
        SHOULD_I_CALL_WITH(CALLBACK_OPEN_FILE, -1, retval);
#if (RES_MULTITHREAD)
        RELEASE_LOCK(GLOCK);
#endif
        return(NULL);
    }

    /* -------------------------------------
           Creating a file for writing
       ------------------------------------- */

    if ( not entry and write_flag)     /* FILE NOT FOUND */
    {

        /* if the user is trying to create a file on the harddrive,
           this is ok (entry not found), but if they are not even
           openning the file for any writing, we can return with
           an error now. */

#if( not RES_USE_FLAT_MODEL )

        /* see if the destination directory exists */

        if (strchr(name, ASCII_BACKSLASH))
        {
            split_path((char *)name, filename, dirpath);
            entry = hash_find(dirpath, GLOBAL_HASH_TABLE);
        }
        else    /* current directory */
        {
            strcpy(filename, name);
            strcpy(dirpath, GLOBAL_CURRENT_PATH);
            entry = hash_find(GLOBAL_CURRENT_PATH, GLOBAL_HASH_TABLE);
        }


        /* if the directory does not exist, this is an error.  Otherwise,
           we get the ptr to the hash table for the destination directory */

        if ( not entry or not entry -> dir)    /* directory not found in resmgr */
        {
            SAY_ERROR(RES_ERR_UNKNOWN_WRITE_TO, name);
#if (RES_MULTITHREAD)
            RELEASE_LOCK(GLOCK);
#endif
            return(NULL);
        }
        else
        {
            table = (HASH_TABLE *)entry -> dir;
        }

#else /* flat model */

        /* if this is flat-mode, just look in the root hash table for
           existance of the file, and set the table ptr to be the global
           hash table (the sole hash table in this case). */

        if (strchr(name, ASCII_BACKSLASH))
            split_path(name, filename, dirpath);
        else
            strcpy(filename, name);

        table = GLOBAL_HASH_TABLE;

#endif /* not RES_USE_FLAT_MODEL */


        /* We use a dummy _finddata_t struct to stuff an entry for
           this file into the hash table */

        strcpy(data.name, filename);
        data.attrib = (unsigned int)FORCE_BIT;
        data.time_create = 0;
        data.time_access = 0;
        data.size = 0;

        entry = hash_add(&data, table);

        if ( not entry)
        {
            SAY_ERROR(RES_ERR_UNKNOWN, "ResFOpen - create");
#if (RES_MULTITHREAD)
            RELEASE_LOCK(GLOCK);
#endif
            return(NULL);
        }

        /* Look through the array of directory names comparing these to
           our destination directory.  The index value of the match is
           set in our hash entry for later use.  If no match is found
           ( this should never occur ), there is a big problem in the
           hash tables */

        for (dir_index = 0; dir_index <= GLOBAL_SEARCH_INDEX; dir_index++)
        {
            if ( not stricmp(dirpath, GLOBAL_SEARCH_PATH[ dir_index ]))
            {
                entry -> directory = dir_index;
                entry -> volume = (char)(toupper(dirpath[0]) - 'A');
                break;
            }
        }

        /* oops.  big problem. */

        if (dir_index > GLOBAL_SEARCH_INDEX)
        {
            SAY_ERROR(RES_ERR_UNKNOWN, "ResFOpen - create");
#if (RES_MULTITHREAD)
            RELEASE_LOCK(GLOCK);
#endif
            return(NULL);
        }
    }


    /* Make sure the user isn't trying to write to an archive file.
       Someday this may be possible, but not for a while. */

    if (entry and (entry -> archive not_eq -1) and write_flag)
    {
        SAY_ERROR(RES_ERR_CANT_WRITE_ARCHIVE, "ResFOpen");
#if (RES_MULTITHREAD)
        RELEASE_LOCK(GLOCK);
#endif
        return(NULL);
    }


    /* we want to use the same allocation scheme that the
       visual c++ run-time uses because a) it isn't that
       bad, b) it assures the highest integration with the
       stream i/o functions, and c) it may keep fclose(file)
       from thrashing your system. */

    stream = _getstream(); /* taken from open.c */

    if ( not stream)
    {
        SAY_ERROR(RES_ERR_TOO_MANY_FILES, "ResFOpen");
#if (RES_MULTITHREAD)
        RELEASE_LOCK(GLOCK);
#endif
        return(NULL);
    }


    /* these initialization values may change */

    stream -> _ptr     = NULL;
    stream -> _cnt     = 0;
    stream -> _base    = NULL;
    stream -> _flag    = _IOREAD; /* *MUST* have this for inuse to think it's full */
    stream -> _file    = 0;
    stream -> _charbuf = 0;
    stream -> _bufsiz  = 0;
    stream -> _tmpfname = NULL;

    if ( not entry or (entry -> archive == -1))
    {

        /* ----- Loose file ----- */


        /* If the file is loose (not in an archive) we will want
           _filbuf to work as normal - therefore the file handle
           should be the OS handle of the open file.  If the file
           is in an archive, the handle is our file handle which
           we use to access the archive and read the file. */

        if ( not entry)  /* assume it's a 'create' acceptable mode */
            res_fullpath(filename, name, _MAX_PATH);    /* regardless of coercion state */
        else
            sprintf(filename, "%s%s", GLOBAL_SEARCH_PATH[ entry -> directory ], entry -> name);


        /* call the same low-level open file that fopen uses */

        if ( not _openfile(filename, mode, _SH_DENYNO, stream))
        {

            if (errno == EACCES)
            {
                SAY_ERROR(RES_ERR_FILE_SHARING, filename);
            }
            else
            {
                SAY_ERROR(RES_ERR_PROBLEM_READING, filename);
            }

            /* Don't forget to free the stream handle, duh */
            stream -> _flag = 0;
            stream -> _ptr = NULL;
            stream -> _cnt = 0;

            UNLOCK_STREAM(stream);
#if (RES_MULTITHREAD)
            RELEASE_LOCK(GLOCK);
#endif
            return(NULL);
        }


        SHOULD_I_CALL_WITH(CALLBACK_OPEN_FILE, -1, retval);


        /* tag the structure as our own flavor (specifically 'loose') */

        stream -> _flag or_eq _IOLOOSE;

        UNLOCK_STREAM(stream);

#if (RES_MULTITHREAD)
        RELEASE_LOCK(GLOCK);
#endif
        return(stream);
    }
    else
    {

        /* ----- Archive File ----- */


        /* This is the case that we're doing all the work for.
           If the file being read is a member of an archive, we
           treat it as if we were using ResOpenFile at this
           point.  Later, during the _filbuf() function, we'll
           use this data to simulate the stream i/o filling
           routine. */


        /* We need one of our special file descriptors */

        int           handle = 0;
        FILE_ENTRY  * file = NULL;
        LIST        * list = NULL;
        ARCHIVE     * archive = NULL;

        handle = get_handle();

        if (handle == -1)    /* none left */
        {
            SAY_ERROR(RES_ERR_TOO_MANY_FILES, "ResOpenFile");
            UNLOCK_STREAM(stream);
#if (RES_MULTITHREAD)
            RELEASE_LOCK(GLOCK);
#endif
            return(NULL);
        }

        file = &FILE_HANDLES[ handle ];

        /* Find the archive file from which this file is found */

        for (list = ARCHIVE_LIST; list; list = list -> next)
        {
            archive = (ARCHIVE *)list -> node;

            if (archive -> os_handle == entry -> archive)
                break;
        }


        /* oops.  big problem. */

        if ( not list)
        {
            SAY_ERROR(RES_ERR_UNKNOWN, "ResFOpen");   /* archive handle in hash entry is incorrect (or archive detached) */
            UNLOCK_STREAM(stream);
#if (RES_MULTITHREAD)
            RELEASE_LOCK(GLOCK);
#endif
            return(NULL);
        }


        /* Use our own file descriptor here.  This is where we do two cheats, one
           obvious, one subtle.  We stuff our own descriptor into the file member
           of the stream structure so we can use our own access methods within our
           modified _filbuf routine.  However, we also use a sleazy little VC++
           uniqueness by setting a flag that this is a _IOSTRG (stream is really
           a string).  This ensures that if the user accidently passes the FILE
           ptr to fclose, nothing nasty will occur.  It also ensures that should
           the run-time library decide to go south, the clean-up code won't
           exaserbate the problem, possibly allowing you to debug the original
           problem. */

        stream -> _file = handle;

        /* Tag the structure as our own flavor (specifically 'archive'), as well
           as use a vc++ uniqueness. */

        stream -> _flag or_eq (_IOARCHIVE bitor _IOSTRG bitor _IOREAD);


        /* ---------------------------------------------------------------------------

                                          UGGGH

           Microsoft morons didn't implement any of their stdio streaming functions
           like everyone else in the fucking world, so this is a sordid fix.  The
           problem is that fread will bypass _filbuf if the requested read size is
           larger than the buffer.  Now, this alone is not a bad optimization, but
           it would have been a hell of lot easier if they had done it like everyone
           else and just put a function ptr for read/write within the iobuf struct.

           --------------------------------------------------------------------------- */

        stream -> _bufsiz = 0xffffffff;

        /* --------------------------------------------------------------------------- */



        UNLOCK_STREAM(stream);

        REQUEST_LOCK(archive -> lock);

        sprintf(filename, "%s%s", GLOBAL_SEARCH_PATH[ entry -> directory ], entry -> name);

        lseek(archive -> os_handle, entry -> file_position + SIGNATURE_SIZE, SEEK_SET);

        _read(archive -> os_handle, tmp, LREC_SIZE);

        process_local_file_hdr(&lrec, tmp);      /* return PK-type error code */

        file -> seek_start = lseek(archive -> os_handle, lrec.filename_length + lrec.extra_field_length, SEEK_CUR);


        /* Initialize some common data */
        file -> current_pos = 0;
        file -> current_filbuf_pos = 0;


        switch (entry -> method)
        {
            case STORED:
            {
                file -> os_handle   = archive -> os_handle;
                //file -> seek_start  = entry -> file_position;
                file -> csize       = 0;
                file -> size        = entry -> size;
                file -> filename    = MemStrDup(filename);
                file -> mode        = _O_RDONLY bitor _O_BINARY;
                file -> device      = entry -> volume;
                file -> zip         = NULL; /* only used if we need to deflate */

                SHOULD_I_CALL_WITH(CALLBACK_OPEN_FILE, handle, retval);

                RELEASE_LOCK(archive -> lock);
#if (RES_MULTITHREAD)
                RELEASE_LOCK(GLOCK);
#endif
                return(stream);
                break;
            }

            case DEFLATED:
            {
                COMPRESSED_FILE * zip;

#ifdef USE_SH_POOLS
                zip = (COMPRESSED_FILE *)MemAllocPtr(gResmgrMemPool, sizeof(COMPRESSED_FILE) + (entry -> size), 0);
#else
                zip = (COMPRESSED_FILE *)MemMalloc(sizeof(COMPRESSED_FILE) + (entry -> size), "Inflate");
#endif

                if ( not zip)
                {
                    SAY_ERROR(RES_ERR_NO_MEMORY, "Inflate");
                    RELEASE_LOCK(archive -> lock);
#if (RES_MULTITHREAD)
                    RELEASE_LOCK(GLOCK);
#endif
                    return(NULL);
                }

                file -> os_handle   = archive -> os_handle;
                //file -> seek_start  = entry -> file_position;
                file -> csize       = entry -> csize;
                file -> size        = entry -> size;
                file -> filename    = MemStrDup(filename);
                file -> mode        = _O_RDONLY bitor _O_BINARY;
                file -> device      = entry -> volume;

#ifdef USE_SH_POOLS
                zip -> slide      = (uch *)MemAllocPtr(gResmgrMemPool, UNZIP_SLIDE_SIZE + INPUTBUFSIZE, 0);   /* glob temporary allocations */
#else
                zip -> slide      = (uch *)MemMalloc(UNZIP_SLIDE_SIZE + INPUTBUFSIZE, "deflate");   /* glob temporary allocations */
#endif

                zip -> in_buffer  = (uch *)zip -> slide + UNZIP_SLIDE_SIZE;
                zip -> in_ptr     = (uch *)zip -> in_buffer;
                zip -> in_count   = 0;
                zip -> in_size    = file -> csize > INPUTBUFSIZE ? INPUTBUFSIZE : file -> csize;
                zip -> csize      = file -> csize;

                zip -> out_buffer = (char *)zip + sizeof(COMPRESSED_FILE);
                zip -> out_count  = 0;
                zip -> archive    = archive;

                file -> zip       = zip;    /* Future use: I may add incremental deflation */

                //lseek( file -> os_handle, file -> seek_start, SEEK_SET );
                inflate(zip);

#ifdef USE_SH_POOLS
                MemFreePtr(zip -> slide);      /* Free temporary allocations */
#else
                MemFree(zip -> slide);      /* Free temporary allocations */
#endif

                SHOULD_I_CALL_WITH(CALLBACK_OPEN_FILE, handle, retval);

                RELEASE_LOCK(archive -> lock);
#if (RES_MULTITHREAD)
                RELEASE_LOCK(GLOCK);
#endif
                return(stream);
                break;
            }

            default:
                SAY_ERROR(RES_ERR_UNSUPPORTED_COMPRESSION, entry -> name);
#if (RES_MULTITHREAD)
                RELEASE_LOCK(GLOCK);
#endif
                return(NULL);
                break;
        }

        RELEASE_LOCK(archive -> lock);
    }

#if (RES_MULTITHREAD)
    RELEASE_LOCK(GLOCK);
#endif

    return(NULL);
}





/* =======================================================

    FUNCTION:   fclose / ResFClose

    PURPOSE:    Closes a file that has previously been
                opened using ResFOpen().

    PARAMETERS: File ptr.

    RETURNS:    None.

   ======================================================= */

int __cdecl RES_FCLOSE(FILE * file)
{
    int handle,
        result;

#if( RES_DEBUG_PARAMS )
    /* check to see if it's one of our two flavors of FILE ptrs */

    if ( not file or not (file -> _flag, (_IOARCHIVE bitor _IOLOOSE)))
    {
        SAY_ERROR(RES_ERR_INCORRECT_PARAMETER, "ResFClose");
        return(EOF); /* error */
    }

#endif

#if (RES_MULTITHREAD)
    REQUEST_LOCK(GLOCK);
#endif

    if (FLAG_TEST(file -> _flag, _IOARCHIVE))
    {
        handle = file -> _file;

        if (FILE_HANDLES[ handle ].zip)
#ifdef USE_SH_POOLS
            MemFreePtr(FILE_HANDLES[ handle ].zip);

#else
            MemFree(FILE_HANDLES[ handle ].zip);
#endif

#ifdef USE_SH_POOLS
        MemFreePtr(FILE_HANDLES[ handle ].filename);
#else
        MemFree(FILE_HANDLES[ handle ].filename);
#endif

        FILE_HANDLES[ handle ].zip = NULL;
        FILE_HANDLES[ handle ].filename = NULL;
        FILE_HANDLES[ handle ].os_handle = -1;

        /* since microsoft doesn't use have symmetry with it's _getstream()
           function (eg; _freestream()), we just set the _flag field to 0
          and assume that's all there is to do (seems like this is true
           after looking at close.c and fclose.c */

        /* Actually, not quite. If the streaming io functions are used then
           a call to _freebuf is needed. Looking closely at fclose.c and
           _freebuf.c it seems safe to do all the time. LRKUDGE
        */
        LOCK_STREAM(file);

        _freebuf(file);
        file -> _flag = 0;
        file -> _ptr = NULL;
        file -> _cnt = 0;

        UNLOCK_STREAM(file);
#if (RES_MULTITHREAD)
        RELEASE_LOCK(GLOCK);
#endif

        return(0);

    }
    else
    {
        FLAG_UNSET(file -> _flag, _IOLOOSE);    /* we want to unset our unique flags before */
        FLAG_UNSET(file -> _flag, _IOSTRG);     /* calling any CRT functions.               */

        /* this is basically all that fclose does   */
        LOCK_STREAM(file);

        result = _flush(file);

        _freebuf(file);

        if (_close(_fileno(file)) < 0)
            result = EOF;

        UNLOCK_STREAM(file);

        file -> _flag = 0;                      /* now we clear all flags                   */

#if (RES_MULTITHREAD)
        RELEASE_LOCK(GLOCK);
#endif
        return(result);
    }
}





#define bigbuf(s)       ((s)->_flag bitand (_IOMYBUF|_IOYOURBUF))
#define _osfile(i)      ( _pioinfo(i)->osfile
#define FCRLF           0x04    /* CR-LF across read buffer (in text mode) */
#define _IOCTRLZ        0x2000
#define FTEXT           0x80    /* file handle is in text mode */

/* =======================================================

    FUNCTION:   ftell / ResFTell

    PURPOSE:    Replaces the stdio ftell function to be
                able to correctly handle streaming i/o
                from within the Resource Manager.

                NOTE:  THIS IS WRAPPED BECAUSE...

                The Microsoft source for ftell is
                surprisingly long and ugly and I'm
                way to lazy to want to implement all
                the special case crap they have.

                However, for the sake of consistency
                with the API for streaming functions,
               and because I think this will work
                for 99.9999% of our projects, I did
                the skinny solution.

                And, having acknowledged that there
                may be several pathalogic cases where
                this version will fail, (eg; files
                opened with fopen instead of ResFOpen,
                in text-mode, without buffering) I'll
                let you choose which version to use.

                If ftell doesn't work for you as it is
                here (first of all, call me because
                I'll be amazed), define
                RES_REPLACE_FTELL to be FALSE and call
                ResFTell.  This will always work.

    PARAMETERS: File ptr.

    RETURNS:    File position, or -1 in case of error.

    NOTE:       Based on the vc++ run-time source file
                ftell.c

   ======================================================= */

long __cdecl RES_FTELL(FILE * stream)
{
    unsigned int offset;
    long filepos;
    register char * p;
    char * max;
    int fd;
    unsigned int rdcnt;

    int handle,
        count;

#if( RES_DEBUG_PARAMS )

    if ( not stream)
    {
        SAY_ERROR(RES_ERR_INCORRECT_PARAMETER, "ftell");
        return(-1);
    }

#endif /* RES_DEBUG_PARAMS */

#if (RES_MULTITHREAD)
    REQUEST_LOCK(GLOCK);
#endif

    /* --------- File within a compressed archive --------- */


    LOCK_STREAM(stream);

    if ((stream -> _flag) bitand _IOARCHIVE)
    {
        handle = stream -> _file;

        /* GFG_NOV06        count = (int)( stream -> _ptr - stream -> _base ); *//* should be safe (key word: SHOULD) */

        if (handle < 0 or handle > MAX_FILE_HANDLES or (FILE_HANDLES[ handle ].os_handle == -1 and not (stream -> _flag bitand _IOLOOSE)))
        {
            SAY_ERROR(RES_ERR_ILLEGAL_FILE_HANDLE, "ftell");
            UNLOCK_STREAM(stream);
#if (RES_MULTITHREAD)
            RELEASE_LOCK(GLOCK);
#endif
            return(-1);
        }

        /***  GFG_NOV06
               if( stream -> _flag bitand _IOARCHIVE )
                   count = FILE_HANDLES[ handle ].current_pos - stream -> _cnt;
               else
                   count += FILE_HANDLES[ handle ].current_pos;
        ***/
        count = FILE_HANDLES[ handle ].current_pos;;

        UNLOCK_STREAM(stream);
#if (RES_MULTITHREAD)
        RELEASE_LOCK(GLOCK);
#endif
        return(count);
    }


    /* ------------------- Loose file ------------------- */


    /* Init stream pointer and file descriptor */

    fd = _fileno(stream);

    if (stream->_cnt < 0)
        stream->_cnt = 0;

    UNLOCK_STREAM(stream);

    if ((filepos = _lseek(fd, 0L, SEEK_CUR)) < 0L)
    {
#if (RES_MULTITHREAD)
        RELEASE_LOCK(GLOCK);
#endif
        return(-1L);
    }

    if ( not bigbuf(stream))           /* _IONBF or no buffering designated */
    {
#if (RES_MULTITHREAD)
        RELEASE_LOCK(GLOCK);
#endif
        return(filepos - stream->_cnt);
    }

    LOCK_STREAM(stream);

    offset = stream->_ptr - stream->_base;

    if (stream->_flag bitand (_IOWRT bitor _IOREAD))
    {
        if (stream -> _flag bitand _O_TEXT)
            for (p = stream->_base; p < stream->_ptr; p++)
                if (*p == '\n')  /* adjust for '\r' */
                    offset++;
    }
    else if ( not (stream->_flag bitand _IORW))
    {
        errno = EINVAL;
        UNLOCK_STREAM(stream);
#if (RES_MULTITHREAD)
        RELEASE_LOCK(GLOCK);
#endif
        return(-1L);
    }

    if (filepos == 0L)
    {
        UNLOCK_STREAM(stream);
#if (RES_MULTITHREAD)
        RELEASE_LOCK(GLOCK);
#endif
        return((long)offset);
    }

    if (stream->_flag bitand _IOREAD)    /* go to preceding sector */
    {

        if (stream->_cnt == 0)      /* filepos holds correct location */
        {
            UNLOCK_STREAM(stream);
            offset = 0;
        }
        else
        {
            /* Subtract out the number of unread bytes left in the
               buffer. [We can't simply use _iob[]._bufsiz because
               the last read may have hit EOF and, thus, the buffer
               was not completely filled.] */

            rdcnt = stream->_cnt + (stream->_ptr - stream->_base);

            /* If text mode, adjust for the cr/lf substitution. If
               binary mode, we're outta here. */

            if (stream -> _flag bitand _O_TEXT)
            {
                /* (1) If we're not at eof, simply copy _bufsiz
                   onto rdcnt to get the # of untranslated
                   chars read. (2) If we're at eof, we must
                   look through the buffer expanding the '\n'
                   chars one at a time. */

                /* [NOTE: Performance issue -- it is faster to
                   do the two _lseek() calls than to blindly go
                   through and expand the '\n' chars regardless
                   of whether we're at eof or not.] */

                UNLOCK_STREAM(stream);

                if (_lseek(fd, 0L, 2) == filepos)
                {

                    LOCK_STREAM(stream);

                    max = stream->_base + rdcnt;

                    for (p = stream->_base; p < max; p++)
                        if (*p == '\n')                     /* adjust for '\r' */
                            rdcnt++;

                    /* If last byte was ^Z, the lowio read
                       didn't tell us about it.  Check flag
                      and bump count, if necessary. */

                    if (stream->_flag bitand _IOCTRLZ)
                        ++rdcnt;

                    UNLOCK_STREAM(stream);
                }
                else
                {

                    _lseek(fd, filepos, 0);

                    /* We want to set rdcnt to the number
                       of bytes originally read into the
                       stream buffer (before crlf->lf
                       translation). In most cases, this
                       will just be _bufsiz. However, the
                       buffer size may have been changed,
                       due to fseek optimization, at the
                       END of the last _filbuf call. */

                    LOCK_STREAM(stream);

                    if ((rdcnt <= _SMALL_BUFSIZ) and 
                        (stream->_flag bitand _IOMYBUF) and 
 not (stream->_flag bitand _IOSETVBUF))
                    {
                        /* The translated contents of
                           the buffer is small and we
                           are not at eof. The buffer
                           size must have been set to
                           _SMALL_BUFSIZ during the
                           last _filbuf call. */

                        rdcnt = _SMALL_BUFSIZ;
                    }
                    else
                        rdcnt = stream->_bufsiz;


                    /* If first byte in untranslated buffer
                       was a '\n', assume it was preceeded
                       by a '\r' which was discarded by the
                       previous read operation and count
                       the '\n'. */
                    if (*stream->_base == '\n')
                        ++rdcnt;

                    UNLOCK_STREAM(stream);
                }

            } /* end if FTEXT */
            else
                UNLOCK_STREAM(stream);

            filepos -= (long)rdcnt;

        } /* end else stream->_cnt not_eq 0 */
    }
    else
        UNLOCK_STREAM(stream);

#if (RES_MULTITHREAD)
    RELEASE_LOCK(GLOCK);
#endif
    return(filepos + (long)offset);
}










/* ==================================================================================

    R E P L A C E M E N T       F R E A D

   ================================================================================== */



/* define the normal version */

size_t __cdecl RES_FREAD(void *buffer, size_t size, size_t num, FILE *stream)
{
    char *data;                     /* point to where should be read next */
    unsigned total;                 /* total bytes to read */
    unsigned count;                 /* num bytes left to read */
    unsigned bufsize;               /* size of stream buffer */
    unsigned nbytes;                /* how much to read now */
    unsigned nread;                 /* how much we did read */
    int c;                          /* a temp char */


    /* initialize local vars */
    data = buffer;


    if ((count = total = size * num) == 0)
        return 0;

#if (RES_MULTITHREAD)
    REQUEST_LOCK(GLOCK);
#endif

    LOCK_STREAM(stream);

    if (anybuf(stream)) /* already has buffer, use its size */
        bufsize = stream->_bufsiz;
    else
#if defined (_M_M68K) or defined (_M_MPPC)
        bufsize = BUFSIZ;           /* assume will get BUFSIZ buffer */

#else  /* defined (_M_M68K) or defined (_M_MPPC) */
        bufsize = _INTERNAL_BUFSIZ; /* assume will get _INTERNAL_BUFSIZ buffer */
#endif  /* defined (_M_M68K) or defined (_M_MPPC) */

    /* here is the main loop -- we go through here until we're done */
    while (count not_eq 0)
    {
        /* if the buffer exists and has characters, copy them to user
           buffer */
        if (anybuf(stream) and stream->_cnt not_eq 0)
        {
            /* how much do we want? */
            nbytes = (count < (unsigned)stream->_cnt) ? count : stream->_cnt;
            memcpy(data, stream->_ptr, nbytes);

            /* update stream and amt of data read */
            count -= nbytes;
            stream->_cnt -= nbytes;
            stream->_ptr += nbytes;
            data += nbytes;

            /* GFG_NOV06 */
            if (stream -> _flag bitand _IOARCHIVE)
                FILE_HANDLES[ stream -> _file ].current_pos += nbytes;




        }              //          |<---------- MODIFIED ----------->|
        else if ((count >= bufsize) and not (stream -> _flag bitand _IOARCHIVE))
        {
            //          |<---------- MODIFIED ----------->|
            /* If we have more than bufsize chars to read, get data
               by calling read with an integral number of bufsiz
               blocks.  Note that if the stream is text mode, read
               will return less chars than we ordered. */

            /* calc chars to read -- (count/bufsize) * bufsize */
            nbytes = (bufsize ? (count - count % bufsize) :
                      count);

            UNLOCK_STREAM(stream);
            nread = _read(_fileno(stream), data, nbytes);
            LOCK_STREAM(stream);

            if (nread == 0)
            {
                /* end of file -- out of here */
                stream->_flag or_eq _IOEOF;
                UNLOCK_STREAM(stream);
#if (RES_MULTITHREAD)
                RELEASE_LOCK(GLOCK);
#endif
                return (total - count) / size;
            }
            else if (nread == (unsigned) - 1)
            {
                /* error -- out of here */
                stream->_flag or_eq _IOERR;
                UNLOCK_STREAM(stream);
#if (RES_MULTITHREAD)
                RELEASE_LOCK(GLOCK);
#endif
                return (total - count) / size;
            }

            /* update count and data to reflect read */
            count -= nread;
            data += nread;
        }
        else
        {
            /* less than bufsize chars to read, so call _filbuf to
               fill buffer */
            if ((c = _filbuf(stream)) == EOF)
            {
                /* error or eof, stream flags set by _filbuf */
                UNLOCK_STREAM(stream);
#if (RES_MULTITHREAD)
                RELEASE_LOCK(GLOCK);
#endif
                return (total - count) / size;
            }

            /* _filbuf returned a char -- store it */
            *data++ = (char) c;
            --count;

            /* GFG_NOV06 */
            if (stream -> _flag bitand _IOARCHIVE)
                FILE_HANDLES[ stream -> _file ].current_pos++;

            /* update buffer size */
            bufsize = stream->_bufsiz;
        }
    }

    UNLOCK_STREAM(stream);
#if (RES_MULTITHREAD)
    RELEASE_LOCK(GLOCK);
#endif

    /* we finished successfully, so just return num */
    return num;
}


/* ==================================================================================

    R E P L A C E M E N T       F S E E K

   ================================================================================== */


/* =======================================================

    FUNCTION:   fseek

    PURPOSE:    Replaces the stdio fseek function to be
                able to correctly handle streaming i/o.

    PARAMETERS: File ptr, offset, enumerated token
                defining the starting location.

    RETURNS:    0 if succeccful, or -1 in case of error.

    NOTE:       Based on the vc++ run-time source file
                fseek.c

   ======================================================= */

int __cdecl RES_FSEEK(FILE * stream, long offset, int whence)
{
    unsigned int pos;

#if( RES_DEBUG_PARAMS )

    if ( not stream)
    {
        SAY_ERROR(RES_ERR_INCORRECT_PARAMETER, "fseek");
        return(-1);
    }

#endif /* RES_DEBUG_PARAMS */
#if (RES_MULTITHREAD)
    REQUEST_LOCK(GLOCK);
#endif

    LOCK_STREAM(stream);

    if (stream -> _flag bitand _IOARCHIVE)
    {
        pos = FILE_HANDLES[ stream -> _file ].current_pos;

        switch (whence)
        {
            case SEEK_SET: /* 0 */
                pos = offset;
                break;

            case SEEK_CUR: /* 1 */
                pos += offset;
                break;

            case SEEK_END: /* 2 */
                pos = FILE_HANDLES[ stream -> _file ].size + offset;
                break;
        }

        stream -> _cnt = 0; /* force next read to replenish buffers */
        stream -> _ptr = stream -> _base;

        UNLOCK_STREAM(stream);

        if (pos > FILE_HANDLES[ stream -> _file ].size)
        {
#if (RES_MULTITHREAD)
            RELEASE_LOCK(GLOCK);
#endif
            return(-1);
        }

        FILE_HANDLES[ stream -> _file ].current_pos = pos;


#if (RES_MULTITHREAD)
        RELEASE_LOCK(GLOCK);
#endif

        return(0);
    }
    else
    {
        if ( not inuse(stream) or
            ((whence not_eq SEEK_SET) and 
             (whence not_eq SEEK_CUR) and 
             (whence not_eq SEEK_END)))
        {
            errno = EINVAL;
            UNLOCK_STREAM(stream);
#if (RES_MULTITHREAD)
            RELEASE_LOCK(GLOCK);
#endif
            return(-1);
        }

        /* Clear EOF flag */

        stream -> _flag and_eq compl _IOEOF;

        /* If seeking relative to current location, then convert to
           a seek relative to beginning of file.  This accounts for
           buffering, etc. by letting fseek() tell us where we are. */

        if (whence == SEEK_CUR)
        {
            offset += ftell(stream);
            whence = SEEK_SET;
        }

        /* Flush buffer as necessary */

        _flush(stream);

        /* If file opened for read/write, clear flags since we don't know
           what the user is going to do next. If the file was opened for
           read access only, decrease _bufsiz so that the next _filbuf
           won't cost quite so much */

        if (stream->_flag bitand _IORW)
            stream->_flag and_eq compl (_IOWRT bitor _IOREAD);
        else
        {
            if ((stream->_flag bitand _IOREAD) and 
                (stream->_flag bitand _IOMYBUF) and 
 not (stream->_flag bitand _IOSETVBUF))
            {
                stream->_bufsiz = _SMALL_BUFSIZ;
            }
        }


        /* Seek to the desired locale and return. */

#ifdef _MT
        pos = _lseek(stream -> _file, offset, whence);
#else
        pos = _lseek_lk(stream -> _file, offset, whence);
#endif


        stream -> _ptr = stream -> _base;

        // There is no file handle assosciated with a streaming 'loose'
        // file.  Therefore... the following fix was actually scribling
        // memory.

        // if( pos not_eq -1 )
        //     FILE_HANDLES[ stream -> _file ].current_pos = pos; [KBR SEPT 10 96]

        if (pos == -1)
        {
#if (RES_MULTITHREAD)
            RELEASE_LOCK(GLOCK);
#endif
            return(-1);
        }


        if ((stream -> _flag bitand _IOARCHIVE) and (pos not_eq -1))
            FILE_HANDLES[ stream -> _file ].current_pos = pos;
    }

    UNLOCK_STREAM(stream);
#if (RES_MULTITHREAD)
    RELEASE_LOCK(GLOCK);
#endif

    return(0);
}

#endif /* RES_STREAMING_IO */


/* ==================================================================================

    R E P L A C E M E N T       _ F I L B U F

   ================================================================================== */



/* =======================================================

    FUNCTION:   _filbuf

    PURPOSE:    Low-level read routine used by stdio
                streaming functions.  _flsbuf is the
                low-level write routine, but since write
                is not allowed on a compressed archive,
                we don't need to replace this function.

    PARAMETERS: File ptr.

    RETURNS:    None.

    NOTE:       Originally I was using the _fillbuf
                routine that is included with the
                Free Software Foundation's gcc
                distribution.  Then I grabbed Microsoft's
                version (on the MSDEV cd with the library
                source code).  This version is a
                unique (bastard) version of the two.

   ======================================================= */

int __cdecl _filbuf(FILE * stream)
{
    int retval = FALSE;     /* used for the callback */
    int handle = 0;

    FILE_ENTRY * file = NULL;      /* my file descriptor */

    int compressed_flag = TRUE;


#if( RES_DEBUG_PARAMS )

    if ( not stream)
    {
        SAY_ERROR(RES_ERR_INCORRECT_PARAMETER, "_filbuf");
        return(EOF);
    }

    // if( not (stream -> _flag bitand ( _IOARCHIVE bitor _IOLOOSE )) ) {
    //    /* You can actually remove this error trap if you want fopen
    //       as well as ResFOpen */
    //    SAY_ERROR( RES_ERR_UNKNOWN, "Stream not created with ResFOpen" );
    //    stream -> _flag or_eq _IOREAD;
    //    return( EOF );
    // }
#endif

    //LRKLUDGE
    // If its a string return
    if ( not inuse(stream) or
        ((stream->_flag bitand _IOSTRG) and 
 not (stream->_flag bitand (_IOLOOSE bitor _IOARCHIVE))))
        return(EOF);

    /* if stream is opened as WRITE ONLY, set error and return */
    if (stream -> _flag bitand _IOWRT)
    {
        stream -> _flag or_eq _IOERR;
        return(EOF);
    }

    /* force flag */

    stream -> _flag or_eq _IOREAD;

    /* Get a buffer, if necessary. (taken from _filbuf.c) */

    if ( not (stream -> _base))
        _getbuf(stream);
    else
        stream -> _ptr = stream -> _base;

    /* if the callback routine does the fill it should return TRUE,
       designating that this routine can exit immediately */

    SHOULD_I_CALL_WITH(CALLBACK_FILL_STREAM, stream, retval);

    if (retval)
        return(0xff bitand retval);

    /* READ OR DECOMPRESS ? */

    /* if a file is loose on the hard-drive, we will want to replenish
       the buffer by simply reading the file directly.  If a file is
       'stored' (not compressed) within an archive file, we replenish
       the buffer by seeking within the archive, and then doing a
       simple read.  Finally, if the file is compressed within an
       archive, we assume we already have a decomressed buffer from
       which to copy bytes. */

    if ( not (stream -> _flag bitand _IOARCHIVE))
    {
        compressed_flag = FALSE;
        handle = stream -> _file;
    }
    else
    {

        file = &FILE_HANDLES[ stream -> _file ];

        /*        if( file -> current_pos >= file -> size )  was GFG */
        if (file -> current_filbuf_pos >= file -> size)
        {
            return(EOF);
        }

        if (file -> os_handle == -1)
        {
            SAY_ERROR(RES_ERR_ILLEGAL_FILE_HANDLE, "_filbuf internal error");
            stream -> _flag or_eq _IOERR;
            return(EOF);
        }

        if ( not file -> zip)      /* file is just stored */
        {
            compressed_flag = FALSE;
            handle = file -> os_handle;
#ifdef _MT
            /*_lseek_lk( handle, (file -> seek_start + file -> current_pos), SEEK_SET );*/
#else
            lseek(handle, (file -> seek_start + file -> current_pos), SEEK_SET);
#endif
        }
        else      /* end of file check ? */
        {

            int count;

            count = stream -> _bufsiz;

            if (count > (int)(file -> size - file -> current_filbuf_pos))    /* was current_pos */
            {
                memset(stream -> _base, 0, stream -> _bufsiz);
                count = file -> size - file -> current_filbuf_pos;    /* was current_pos */
            }

            memcpy(stream -> _base, file -> zip -> out_buffer + file -> current_filbuf_pos, count);  /* was current_pos */
            file -> current_filbuf_pos += count;       /* GFG_NOV06 */
            stream -> _cnt = count;
        }
    }

    if ( not compressed_flag)
    {

        stream -> _cnt = _read(handle, stream -> _base, stream -> _bufsiz);

        if (file)    /* stored in an archive */
        {

            if (stream -> _cnt < 0)       /* error reading */
            {
                stream -> _flag or_eq _IOERR;

                if (stream -> _flag bitand (_IOARCHIVE bitor _IOLOOSE))  /* make sure this is an fopen() file */
                    ResCheckMedia(file -> device);              /* if not, has media changed?        */

                return(EOF);
            }

            /****    GFG_NOV06
                        else
                            file -> current_pos += stream -> _cnt;
            ***/
        }

        if ((stream -> _cnt == 0) or (stream -> _cnt == -1))
        {
            stream -> _flag or_eq stream -> _cnt ? _IOERR : _IOEOF;
            stream -> _cnt = 0;
            return(EOF);
        }

        //  Don't think I need this, but... _osfile_safe(i) expands to (_pioinfo_safe(i)->osfile)
        //  if( not (stream -> _flag bitand ( _IOWRT bitor _IORW )) and ((_osfile_safe(_fileno(stream)) bitand (FTEXT|FEOFLAG)) == (FTEXT|FEOFLAG)))
        //      stream -> _flag or_eq _IOCTRLZ;

        /* Check for small _bufsiz (_SMALL_BUFSIZ). If it is small and
           if it is our buffer, then this must be the first _filbuf after
           an fseek on a read-access-only stream. Restore _bufsiz to its
           larger value (_INTERNAL_BUFSIZ) so that the next _filbuf call,
           if one is made, will fill the whole buffer. */

        if ((stream -> _bufsiz == _SMALL_BUFSIZ) and 
            (stream -> _flag bitand _IOMYBUF) and 
 not (stream -> _flag bitand _IOSETVBUF))
        {
            stream -> _bufsiz = _INTERNAL_BUFSIZ;
        }
    }

    stream -> _cnt--;
    return(0xff bitand *stream -> _ptr++);
}





#if( RES_DEBUG_VERSION )
/* =======================================================

    FUNCTION:   ResDbg

    PURPOSE:    Toggle the state of debugging at runtime.

    PARAMETERS: Boolean state TRUE (on) / FALSE (off).

    RETURNS:    None.

   ======================================================= */

RES_EXPORT void ResDbg(int on)
{
    RES_DEBUG_FLAG = on;
}



/* =======================================================

    FUNCTION:   ResDbgLogOpen

    PURPOSE:    Open a file for event logging.

    PARAMETERS: Filename to write data to.

    RETURNS:    TRUE (pass) / FALSE (fail).

   ======================================================= */

RES_EXPORT int ResDbgLogOpen(char * filename)
{
    if (RES_DEBUG_LOGGING)
        ResDbgLogClose();

    /* there is actually a third parameter to open() (MSVC just doesn't admit it)
       octal 666 ensures that stack-crap won't accidently create this file as
       read-only.  Thank to Roger Fujii for this fix */

    RES_DEBUG_FILE = _open(filename, _O_RDWR bitor _O_CREAT bitor _O_TEXT, 0x1b6 /* Choked on O666L and O666 */);

    if (RES_DEBUG_FILE == -1)
    {
        SAY_ERROR(RES_ERR_COULDNT_OPEN_FILE, filename);
        RES_DEBUG_OPEN_LOG = FALSE;
        RES_DEBUG_LOGGING = FALSE;
        return(FALSE);
    }

    RES_DEBUG_OPEN_LOG = TRUE;
    RES_DEBUG_LOGGING = TRUE;

    IF_DEBUG(LOG("Log file opened.\n\n"));

    return(TRUE);
}



/* =======================================================

    FUNCTION:   ResDbgLogClose

    PURPOSE:    Closes a previously opened log file.

    PARAMETERS: None.

    RETURNS:    None.

   ======================================================= */

RES_EXPORT void ResDbgLogClose(void)
{
    if (RES_DEBUG_OPEN_LOG)
        _close(RES_DEBUG_FILE);

    RES_DEBUG_OPEN_LOG = FALSE;

    RES_DEBUG_LOGGING = FALSE;
}



/* =======================================================

    FUNCTION:   ResDbgPrintf

    PURPOSE:    Handle stdio output.

    PARAMETERS: Same as printf().

    RETURNS:    None.

   ======================================================= */

RES_EXPORT void ResDbgPrintf(char * msg, ...)
{
    va_list data;                            /* c sucks                      */
    char buffer[255];
    int length;

    va_start(data, msg);                     /* init variable args           */

    length = vsprintf(buffer, msg, data);

    if (RES_DEBUG_OPEN_LOG and RES_DEBUG_LOGGING)
        _write(RES_DEBUG_FILE, buffer, length);

#ifdef RES_DEBUG_STDIO
    RES_DEBUG_STDIO(buffer, length);         /* external func for dumping text msg's to the console */
#endif /*RES_DEBUG_STDIO */

    va_end(data);                            /* reset variable args          */
}



/* =======================================================

    FUNCTION:   ResDbgLogPause

    PURPOSE:    Sets the state of event logging ( a log
                file obviously must already be opened).

    PARAMETERS: None.

    RETURNS:    TRUE (pass) / FALSE (fail).

   ======================================================= */

RES_EXPORT void ResDbgLogPause(int on)
{
    RES_DEBUG_LOGGING = on;
}



/* =======================================================

    FUNCTION:   ResDbgLogDump

    PURPOSE:    Dump statistics to the log file.

    PARAMETERS: None.

    RETURNS:    None.

   ======================================================= */

RES_EXPORT void ResDbgDump(void)
{
    HASH_TABLE   * table;
    DEVICE_ENTRY * dev;
    LIST         * list;
    int            i;

    if (RES_DEBUG_LOGGING and RES_DEBUG_OPEN_LOG)
    {

        IF_LOG(LOG("\n\n-------------\n"));
        IF_LOG(LOG("Statistics...\n"));
        IF_LOG(LOG("-------------\n"));

        for (list = GLOBAL_PATH_LIST; list; list = list -> next)
        {
            table = (HASH_TABLE *)list -> node;

            dbg_analyze_hash(table);
            dbg_dir(table);
        }

        dev = RES_DEVICES;

        for (i = 0; i < MAX_DEVICES; i++)
            if (dev -> serial)
                dbg_device(dev++);
    }
    else
        IF_LOG(LOG("Either no open log file, or event logging paused.\n"));
}
#endif /* RES_DEBUG_VERSION */















/* -----------------------------------------------------------------------
   -----------------------------------------------------------------------

   R E S O U R C E   M A N A G E R   *   P R I V A T E   F U N C T I O N S

   -----------------------------------------------------------------------
   ----------------------------------------------------------------------- */



/* =======================================================

    FUNCTION:   asynch_write (local only)  THREAD FUNCTION

    PURPOSE:    Asynchronously write to a file.

    PARAMETERS: Thread data (ASYNCH_DATA struct).

    RETURNS:    None. (callback on completion)

   ======================================================= */

void
asynch_write(void * thread_data)
{
    int check;
    ASYNCH_DATA * data;

    data = (ASYNCH_DATA *)thread_data;

    check = _write(data->file, data->buffer, data->size);

    if (data->callback)
        (*(data->callback))(data->file);

#ifdef USE_SH_POOLS
    MemFreePtr(data);
#else
    MemFree(data);
#endif

    IF_LOG(LOG("Write thread exited."));
}



/* =======================================================

    FUNCTION:   asynch_read (local only)  THREAD FUNCTION

    PURPOSE:    Asynchronously write to a file.

    PARAMETERS: Thread data (ASYNCH_DATA struct).

    RETURNS:    None. (callback on completion)

   ======================================================= */

void
asynch_read(void * thread_data)
{
    ASYNCH_DATA * data;

    data = (ASYNCH_DATA *)thread_data;

    _read(data->file, data->buffer, data->size);

    if (data->callback)
        (*(data->callback))(data->file);

#ifdef USE_SH_POOLS
    MemFreePtr(data);
#else
    MemFree(data);
#endif

    IF_LOG(LOG("Read thread exited."));
}






/* =======================================================

   FUNCTION:   shut_down

   PURPOSE:    Shut down the resource manager.  Since
               the user is allowed to re-ResInit the
               manager, this function is called by both
               ResInit and ResExit.

   PARAMETERS: None.

   RETURNS:    None.

   ======================================================= */

void shut_down(void)
{
    LIST    * list;
    int       index;

    /* Reset system paths.  The paths were stored with strdup, so we
       free them here. */

    for (index = 0; index < RES_DIR_LAST; index++)
        if (RES_PATH[ index ])
#ifdef USE_SH_POOLS
            MemFreePtr(RES_PATH[ index ]);

#else
            MemFree(RES_PATH[ index ]);
#endif

    if (GLOBAL_SEARCH_INDEX)
        for (index = 0; index < GLOBAL_SEARCH_INDEX; index++)
#ifdef USE_SH_POOLS
            MemFreePtr(GLOBAL_SEARCH_PATH[ index ]);

#else
            MemFree(GLOBAL_SEARCH_PATH[ index ]);
#endif

    memset(GLOBAL_SEARCH_PATH, 0, sizeof(GLOBAL_SEARCH_PATH));
    GLOBAL_SEARCH_INDEX = 0;

    memset(RES_PATH, 0, sizeof(char*) * RES_DIR_LAST);     /* reset system paths */


    /* If there are any files that are still open (regardless of whether the
       media has been ejected by the user or not) allocations will still
       exist.  Close any of them here. */

    if (FILE_HANDLES)    /* close any open file handles, clear for heck of it */
    {
        for (index = 0; index < MAX_FILE_HANDLES; index++)
            if (FILE_HANDLES[ index ].os_handle not_eq -1)
                ResCloseFile(index);

#ifdef USE_SH_POOLS
        MemFreePtr(FILE_HANDLES);
#else
        MemFree(FILE_HANDLES);
#endif

        FILE_HANDLES = NULL;
    }


    /* If there are any archive files that haven't been closed, do
       so now. */

    if (ARCHIVE_LIST)
    {
        for (list = ARCHIVE_LIST; list; list = list -> next)
            res_detach_ex((ARCHIVE *)list -> node);

        LIST_DESTROY(ARCHIVE_LIST, NULL);

        //UGGH --> Why won't this compile? LIST_DESTROY( ARCHIVE_LIST, res_detach_ex );

        ARCHIVE_LIST = NULL;

        inflate_free();
    }

#if( RES_DEBUG_VERSION )
    /* If you've called any ResOpenDirectory()'s without calling the
       destructor ResCloseDirectory, the debug version takes care of
       it for you.  Do this using the Release version and you'll
       leak.                                                            */

    if (OPEN_DIR_LIST)
    {
        RES_DIR * dir;

        for (list = OPEN_DIR_LIST; list; list = list -> next)
        {
            dir = (RES_DIR *)list -> node;
            ResCloseDirectory(dir);
            IF_LOG(LOG("directory leak prevented: %s\n", dir -> name));
        }

        LIST_DESTROY(OPEN_DIR_LIST, NULL);
        OPEN_DIR_LIST = NULL;
    }

#endif /* RES_DEBUG_VERSION */

    if (RES_DEVICES)
#ifdef USE_SH_POOLS
        MemFreePtr(RES_DEVICES);

#else
        MemFree(RES_DEVICES);
#endif

    RES_DEVICES = NULL;

    if (GLOBAL_PATH_LIST)
    {
        for (list = GLOBAL_PATH_LIST; list; list = list -> next)
            hash_destroy((HASH_TABLE *)list -> node);

        LIST_DESTROY(GLOBAL_PATH_LIST, NULL);
        GLOBAL_PATH_LIST = NULL;
    }

    if (GLOBAL_HASH_TABLE)
    {
        hash_destroy(GLOBAL_HASH_TABLE);
        GLOBAL_HASH_TABLE = NULL;
    }
}





/* =======================================================

   FUNCTION:   get_dir_index

   PURPOSE:    Returns the path index given a path name.

   PARAMETERS: Pathname.

   RETURNS:    Index or -1 if not found.

   ======================================================= */

int get_dir_index(char * path)
{
    int dir_index;

    for (dir_index = 0; dir_index < GLOBAL_SEARCH_INDEX; dir_index++)
    {
        if ( not stricmp(path, GLOBAL_SEARCH_PATH[ dir_index ]))
            return(dir_index);
    }

    return(-1);
}





/* =======================================================

   FUNCTION:   sort_path

   PURPOSE:    Force cd based paths to be at the end of
               the search path.

   PARAMETERS: None.

   RETURNS:    None.


   NOTE:       Given a path similar to: (c-hard drive, f-cd)

                  c:\game\
                  f:\game\objects\
                  c:\
                  f:\game\data
                  f:\
                  c:\game\sound\

               New path would be

                  c:\game\
                  c:\
                  c:\game\sound\
                  f:\game\objects\
                  f:\game\data
                  f:\

              None of the members are sub-sorted.

   ======================================================= */

void sort_path(void)
{
    char   cd_id;

    HASH_TABLE * ht;

    LIST * hd_list = NULL,
           * cd_list = NULL;

    LIST * list;

    cd_id = (char)(GLOBAL_CD_DEVICE + 'A');

    /* ugly - look at the first character in the pathname to
       determine which volume is referenced.  If this volume
       is the CD-drive, subjigate this to the cd list.  The
       new search path is hard-drive list + cd list. */

    for (list = GLOBAL_PATH_LIST; list; list = list -> next)
    {

        ht = (HASH_TABLE *)list -> node;

        if (cd_id == toupper(ht -> name[0]))
            cd_list = LIST_APPEND_END(cd_list, ht);
        else
            hd_list = LIST_APPEND_END(hd_list, ht);
    }

    LIST_DESTROY(GLOBAL_PATH_LIST, NULL);

    GLOBAL_PATH_LIST = LIST_CATENATE(hd_list, cd_list);
}



/* =======================================================

   FUNCTION:   res_detach_ex

   PURPOSE:    Closes an open archive file (zip).

   PARAMETERS: Ptr to an archive.

   RETURNS:    None.

   ======================================================= */

void res_detach_ex(ARCHIVE * archive)
{
    HASH_ENTRY * entry;

    int i;

#if( not RES_USE_FLAT_MODEL )
    HASH_TABLE * table;

    for (i = 0; i < archive -> num_entries; i++)
    {
        entry = hash_find_table(&archive -> name[i], &table);

        if ( not entry)
        {
            SAY_ERROR(RES_ERR_FILE_NOT_FOUND, &archive -> name[i]);
            continue;
        }

        if (((ARCHIVE*)entry -> archive) -> os_handle ==             archive -> os_handle)  /* make sure the entry wasn't overridden */

            hash_delete(entry, table);
    }

#else

    for (i = 0; i < archive -> num_entries; i++)
    {
        entry = hash_find(&archive -> name[i], GLOBAL_HASH_TABLE);

        if ( not entry)
        {
            SAY_ERROR(RES_ERR_FILE_NOT_FOUND, &archive -> name[i]);
            continue;
        }

        if (((ARCHIVE *)entry -> archive) -> os_handle == archive -> os_handle)  /* make sure the entry wasn't overridden */
            hash_delete(entry, GLOBAL_HASH_TABLE);
    }

#endif /* not USE_FLAT_MODEL */

    _close(archive -> os_handle);

    DESTROY_LOCK(archive -> lock);

#ifdef USE_SH_POOLS
    MemFreePtr(archive);
#else
    MemFree(archive);
#endif
}


/* =======================================================

    FUNCTION:   hash

    PURPOSE:    Hashing function.  This was taken from
                Sedgewicks' Algorithms in C.

    PARAMS:        ASCII string to hash.

    RETURNS:    Hashed value (guarenteed to be less than
                the size of the hash table).

   ======================================================= */

int hash(const char * string, int size)
{
    int i;

#if( USE_SEDGEWICK )

    for (i = 0; *string not_eq '\0'; string++)
        i = (HASH_CONST * i + (toupper(*string))) % size;

#   error   DO NOT USE SEDGEWICK - RH
#endif

#if( USE_AFU )
    int  res = 0;
    int  pos = 1;

    while (*string)
    {
        res += toupper(*string) * pos;

        pos++;
        string++;
    }

    i = (res bitand 0xffffff) % size;
#endif

    return(i);
}

#if( USE_AFU and USE_SEDGEWICK )
#   error   You cannot use both hashing algorithms
#endif /* USE_BOTH? */



/* =======================================================

    FUNCTION:    hash_create

    PURPOSE:    Create a hash table.  The hash table used
                to be allocated with the wrapper, and the
                table ptr set to be base + sizeof(wrapper),
                but this made RESIZE difficult.

    PARAMS:        Size of hash table to create.

    RETURNS:    Ptr to a hash structure.

   ======================================================= */

HASH_TABLE * hash_create(int size, char * name)
{
    HASH_TABLE * hsh;
    char * string_space;
    int    sizeb;

#ifdef USE_SH_POOLS
    hsh = (HASH_TABLE *)MemAllocPtr(gResmgrMemPool, sizeof(HASH_TABLE), 0);
#else
    hsh = (HASH_TABLE *)MemMalloc(sizeof(HASH_TABLE), "Hash wrapper");
#endif

    if ( not hsh)
    {
        SAY_ERROR(RES_ERR_NO_MEMORY, "hash_create");
        return(NULL);
    }

#ifdef USE_SH_POOLS
    hsh -> table = (HASH_ENTRY *)MemAllocPtr(gResmgrMemPool, size * sizeof(HASH_ENTRY), 0);
#else
    hsh -> table = (HASH_ENTRY *)MemMalloc(size * sizeof(HASH_ENTRY), name);
#endif

    if ( not hsh -> table)
    {
        SAY_ERROR(RES_ERR_NO_MEMORY, "hash_create");
        return(NULL);
    }

    memset(hsh -> table, 0, size * sizeof(HASH_ENTRY));

    sizeb = size * MAX_FILENAME;

#ifdef USE_SH_POOLS
    string_space = (char *)MemAllocPtr(gResmgrMemPool, sizeb, 0);
#else
    string_space = (char *)MemMalloc(sizeb, "Strings");
#endif

    if ( not string_space)
    {
        SAY_ERROR(RES_ERR_NO_MEMORY, "hash_create");
        return(NULL);
    }

    hsh -> table_size = size;
    hsh -> num_entries = 0;
    hsh -> ptr_in = string_space;

    size = (int)(STRING_SAFETY_SIZE * (float)sizeb); /* safety buffer on string pool */
    hsh -> ptr_end = string_space + size;
    hsh -> str_list = NULL;
    hsh -> str_list = LIST_APPEND(hsh->str_list, string_space);

    hsh -> name = hash_strcpy(hsh, name);

    return(hsh);
}



/* =======================================================

    FUNCTION:    hash_destroy

    PURPOSE:    Destroy an entire hash table.

    PARAMS:        Ptr to a hash table structure.

    RETURNS:    None.

   ======================================================= */

void hash_destroy(HASH_TABLE * hsh)
{
    int index;
    HASH_ENTRY * entry,
               * prev,
               * curr;

    for (index = 0; index < hsh -> table_size; index++)
    {
        entry = &hsh -> table[index];

        entry -> attrib = 0;

        if ( not entry -> next)
            continue;

        prev = entry -> next;
        curr = prev -> next;

        while (curr)
        {
#ifdef USE_SH_POOLS
            MemFreePtr(prev);
#else
            MemFree(prev);
#endif

            prev = curr;
            curr = curr -> next;
        }

#ifdef USE_SH_POOLS
        MemFreePtr(prev);
#else
        MemFree(prev);
#endif
    }

#if( RES_DEBUG_VERSION )
    hsh -> num_entries = 0;
    hsh -> table_size = 0;
#endif

#ifdef USE_SH_POOLS
    LIST_DESTROY(hsh->str_list, free);
#else
    LIST_DESTROY(hsh->str_list, MemFreePtr);
#endif

#ifdef USE_SH_POOLS
    MemFreePtr(hsh -> table);
    MemFreePtr(hsh);
#else
    MemFree(hsh -> table);
    MemFree(hsh);
#endif
}


char * hash_strcpy(HASH_TABLE * hsh, char * string)
{
    char * string_space;
    char * ptr_out;
    int    size;

    ptr_out = hsh -> ptr_in;

    strcpy(hsh -> ptr_in, string);

    hsh -> ptr_in += strlen(string);
    *hsh -> ptr_in = '\0'; /*safety*/
    hsh -> ptr_in++;

    if (hsh -> ptr_in > hsh -> ptr_end)
    {
        size = HASH_TABLE_SIZE * MAX_FILENAME;

#ifdef USE_SH_POOLS
        string_space = (char *)MemAllocPtr(gResmgrMemPool, size, 0);
#else
        string_space = (char *)MemMalloc(size, "Strings");
#endif

        hsh -> str_list = LIST_APPEND(hsh -> str_list, string_space);
        hsh -> ptr_in = string_space;

        size = (int)((float)STRING_SAFETY_SIZE * (float)size);
        hsh -> ptr_end = string_space + size;
    }

    return(ptr_out);
}

/* =======================================================

    FUNCTION:   hash_add

    PURPOSE:    Add an entry into a hash table.

    PARAMS:     Ptr to a file data structure, ptr to a
                hash table wrapper struct.

    RETURNS:    Ptr to the hash entry.

   ======================================================= */

HASH_ENTRY * hash_add(struct _finddata_t * data, HASH_TABLE * hsh)
{
    int   hash_val;

    HASH_ENTRY * entry = NULL;


    /* If we need to do a resize, we want to do it when there
       are no HASH_ENTRY pointers being used.  Currently, the
       HASH_TABLE is NOT being checked for a lock before
       a resize is performed.  This is not safe. */

    /* Resize before searching (and returning) a HASH_ENTRY ptr */

    if (hsh -> num_entries)
    {
        /* efficiency ratio of the hash table (entries/num slots)   */
        float ratio = ((float)hsh -> table_size / (float)hsh -> num_entries);

        if (ratio < HASH_MINIMAL_RATIO)
            hash_resize(hsh);               /* WARNING: THIS IS STILL NOT THREAD SAFE */
    }

    hash_val = hash(data -> name, hsh -> table_size);

    if (hsh -> table[ hash_val ].attrib)      /* an entry already exists here                    */
    {
#if( RES_USE_FLAT_MODEL )
        entry = hash_find(data -> name, hsh);

        if (entry)   /* override automatically if this is the flat model */
        {
            IF_LOG(LOG("Override %s\n", data -> name));

            entry -> name = hash_strcpy(hsh, data -> name);
            entry -> offset = 0;
            entry -> size = data -> size;
            entry -> attrib = data -> attrib bitor FORCE_BIT; /* FORCE_BIT ensures the field will be non-zero */
            /* entry -> next stays the same         */
            /* hsh -> num_entries stays the same    */
            return(entry);
        }

#endif /* RES_USE_FLAT_MODEL */

        entry = &hsh -> table[ hash_val ];

        while (entry -> next)                         /* go to the end of the list                    */
            entry = entry -> next;

#ifdef USE_SH_POOLS
        entry -> next = (HASH_ENTRY *)MemAllocPtr(gResmgrMemPool, sizeof(HASH_ENTRY), 0);
#else
        entry -> next = (HASH_ENTRY *)MemMalloc(sizeof(HASH_ENTRY), "Hash entry");
#endif

        if ( not entry -> next)                           /* malloc failed                                */
        {
            SAY_ERROR(RES_ERR_NO_MEMORY, "hash_add");
            return(NULL);
        }

        memset(entry ->next, 0, sizeof(HASH_ENTRY)); // OW BC

        entry = entry -> next;
    }
    else
    {
        entry = &hsh -> table[ hash_val ];
    }

    entry -> name = hash_strcpy(hsh, data -> name);
    entry -> offset = 0;
    entry -> size = data -> size;
    entry -> attrib = data -> attrib bitor FORCE_BIT; /* FORCE_BIT ensures the field will be non-zero */
    entry -> next = NULL;
    entry -> archive = -1; // Changed on AUG30th  [KBR]
#if( not RES_USE_FLAT_MODEL )
    entry -> dir = NULL;
#endif /* not RES_USE_FLAT_MODEL */

    hsh -> num_entries++;

    return(entry);
}



/* =======================================================

    FUNCTION:   hash_copy

    PURPOSE:    Save some typing.

    PARAMS:     Ptr to a source HASH_ENTRY, ptr to
                a destination HASH_ENTRY.

    RETURNS:    True / False.  False implies error.

   ======================================================= */


void hash_copy(HASH_ENTRY * dst, HASH_ENTRY * src)
{
    if (dst -> attrib)
    {
        while (dst -> next)                         /* go to the end of the list                    */
            dst = dst -> next;

#ifdef USE_SH_POOLS
        dst -> next = (HASH_ENTRY *)MemAllocPtr(gResmgrMemPool, sizeof(HASH_ENTRY), 0);
#else
        dst -> next = (HASH_ENTRY *)MemMalloc(sizeof(HASH_ENTRY), "Hash entry");
#endif

        if (dst->next) memset(dst->next, 0, sizeof(HASH_ENTRY)); // OW BC

        dst = dst -> next;
    }

    dst -> name   = src -> name;
    dst -> offset = src -> offset;
    dst -> size   = src -> size;
    dst -> csize  = src -> csize;
    dst -> method = src -> method;
    dst -> volume = src -> volume;
    dst -> directory = src -> directory;
    dst -> file_position = src -> file_position;
    dst -> archive = src -> archive;
    dst -> attrib = src -> attrib;
    dst -> dir    = src -> dir;
    dst -> next   = NULL;
}


/* =======================================================

    FUNCTION:   hash_resize

    PURPOSE:    Resize a hash table.  This is
                unfortunately, not a trivial task.  All
                entries in the original table must be
                rehashed into the new table since
                altering the size dramatically alters
                the hash algorithm.

    PARAMS:     Ptr to a hash table wrapper struct.

    RETURNS:    True / False.  False implies error.

   ======================================================= */

int hash_resize(HASH_TABLE * hsh)
{
    int size;
    int i, val;

    HASH_ENTRY * entry;
    HASH_ENTRY * src,
               * dst,
               * prev;

    IF_LOG(LOG("resizing hash table %s\n", hsh -> name));

    /* calc the size of the new hash table  */
    size = (int)((float)hsh -> num_entries * HASH_OPTIMAL_RATIO);

    /* create the new hash entries          */

#ifdef USE_SH_POOLS
    entry = (HASH_ENTRY *)MemAllocPtr(gResmgrMemPool, size * sizeof(HASH_ENTRY), 0);
#else
    entry = (HASH_ENTRY *)MemMalloc(size * sizeof(HASH_ENTRY), "Hash resized");
#endif

    if ( not entry)
    {
        SAY_ERROR(RES_ERR_NO_MEMORY, "hash_resize");
        return(FALSE);
    }

    memset(entry, 0, size * sizeof(HASH_ENTRY));

    /* we have to rehash ALL of the entries in the old table into the new table */

    for (i = 0; i < hsh -> table_size; i++)
    {
        src = &hsh -> table[i];

        if (src -> attrib)
        {
            val = hash(src -> name, size);
            dst = &entry[ val ];
            hash_copy(dst, src);
        }

        if (src -> next)
        {
            for (src = src -> next; src; src = src -> next)
            {
                val = hash(src -> name, size);
                dst = &entry[ val ];
                hash_copy(dst, src);
            }
        }
    }

    for (i = 0; i < hsh -> table_size; i++)
    {
        src = &hsh -> table[i];

        src -> attrib = 0;

        if ( not src -> next)
            continue;

        prev = src -> next;
        dst = prev -> next;

        while (dst)
        {
            prev -> next = NULL;

#ifdef USE_SH_POOLS
            MemFreePtr(prev);
#else
            MemFree(prev);
#endif

            prev = dst;
            dst = dst -> next;
        }

#ifdef USE_SH_POOLS
        MemFreePtr(prev);
#else
        MemFree(prev);
#endif

        src -> next = NULL;
    }

#ifdef USE_SH_POOLS
    MemFreePtr(hsh->table);
#else
    MemFree(hsh->table);
#endif

    hsh -> table_size = size;
    hsh -> table = entry;

    return(TRUE);
}



/* =======================================================

    FUNCTION:   hash_find

    PURPOSE:    Find an entry within a hash table.

    PARAMS:     Ptr to a filename to find, ptr to a hash
                table wrapper struct.

    RETURNS:    Ptr to the found entry or NULL (if not found).

   ======================================================= */

HASH_ENTRY * hash_find(const char * name, HASH_TABLE * hsh)
{
    int hash_val;
    HASH_ENTRY * entry;

    if ( not GLOBAL_HASH_TABLE)
        return(NULL);

    hash_val = hash(name, hsh -> table_size);               /* calc the hash value for the given string */

    if ( not hsh -> table[ hash_val ].attrib)                   /* no hash entry found                        */
        return(NULL);

    if (hsh -> table[ hash_val ].next == NULL)
        if ( not stricmp(hsh -> table[ hash_val ].name, name))
            return(&hsh -> table[ hash_val ]);              /* just one entry found in the hash position */

    /* found imperfect hash entry                    */
    for (entry = &hsh -> table[ hash_val ]; entry; entry = entry -> next)
        if ( not stricmp(entry -> name, name))
            return(entry);                                  /* assumes only one occurrence of a given name    */

    return(NULL);                                           /* not found                                    */
}



/* =======================================================

    FUNCTION:   hash_delete

    PURPOSE:    Removes an entry from a hash table.

    PARAMS:        Ptr to a file data structure, ptr to a
                hash table wrapper.

    RETURNS:    None.

   ======================================================= */

int hash_delete(HASH_ENTRY * hash_entry, HASH_TABLE * hsh)
{
    int i;

    HASH_ENTRY * entry,
               * prev;

    if (hash_entry -> dir)
        hash_destroy((HASH_TABLE *)hash_entry -> dir);

    for (i = 0; i < hsh -> table_size; i++)
    {
        entry = &hsh -> table[i];

        if (entry == hash_entry)
        {
            if (hash_entry -> next)                 /* pop the chain of hash collisions */
            {
                entry = hash_entry -> next;

                memcpy(hash_entry, entry, sizeof(HASH_ENTRY));

#ifdef USE_SH_POOLS
                MemFreePtr(entry);
#else
                MemFree(entry);
#endif
                hsh -> num_entries--;
            }
            else
            {
                entry -> attrib = 0;                /* no chain to pop.  just flag this entry as being empty */
                hsh -> num_entries--;
            }

            return(TRUE);
        }

        if (entry -> next)                          /* look for hash entry on a chain */
        {
            prev = entry;
            entry = entry -> next;

            while (entry)
            {
                if (entry == hash_entry)
                {
                    prev -> next = entry -> next;   /* cut from chain */

#ifdef USE_SH_POOLS
                    MemFreePtr(entry);
#else
                    MemFree(entry);
#endif
                    hsh -> num_entries--;

                    return(TRUE);
                }

                prev = entry;
                entry = entry -> next;
            }
        }
    }

    return(FALSE);
}



/* =======================================================

    FUNCTION:   hash_purge

    PURPOSE:    Purge entries from a single hash table
                based on identical archive handle, volume
                id, and/or directory handle.

    PARAMETERS: Ptr to archive handle, volume id,
                directory handle and filename.
                Pass NULL for any unused parameter.

    RETURNS:    None.

   ======================================================= */

//void hash_purge( HASH_TABLE * hsh, char * archive, char * volume, char * directory, char * filename )
void hash_purge(HASH_TABLE * hsh, const char * archive, const char * volume, const int * directory, const char * filename)
{
    int index;

    HASH_ENTRY * entry,
               * prev,
               * curr;

    if (archive)
    {
        IF_LOG(LOG("purging entries from archive %d\n", *archive));
    }

    if (volume)
    {
        IF_LOG(LOG("purging entries from volume %d\n", *volume));
    }

    if (directory)
    {
        IF_LOG(LOG("purging entries from directory %d\n", *directory));
    }

    if (filename)
    {
        IF_LOG(LOG("purging entries named %s\n", filename));
    }

    for (index = 0; index < hsh -> table_size; index++)
    {
        entry = &hsh -> table[ index ];

        if ( not entry -> attrib)
            continue; /* empty hash entry */

        if (entry -> next)
        {
            prev = entry;
            curr = entry -> next;

            while (curr)
            {
                if ((volume and (curr -> volume == *volume)) or
                    (archive and (curr -> archive == *archive)) or
                    (directory and (curr -> directory == *directory)) or
                    (filename and not strcmp(entry -> name, filename)))
                {
                    if (curr -> dir)
                        hash_destroy((HASH_TABLE*)prev -> dir);

                    prev -> next = curr -> next;

#ifdef USE_SH_POOLS
                    MemFreePtr(curr);
#else
                    MemFree(curr);
#endif
                }
                else
                    prev = curr;

                curr = prev -> next;
            }
        }

        if ((volume and (entry -> volume == *volume)) or
            (archive and (entry -> archive == *archive)) or
            (directory and (entry -> directory == *directory)) or
            (filename and not strcmp(entry -> name, filename)))
        {
            if (entry -> dir)
            {
                hash_destroy((HASH_TABLE *)entry -> dir);
                entry -> dir = NULL;
            }

            if (entry -> next)
            {
                prev = entry -> next;

                memcpy(entry, prev, sizeof(HASH_ENTRY));

                if (prev -> dir)
                    hash_destroy((HASH_TABLE *)prev -> dir);  // navio: (408)328-0630

                entry -> next = prev -> next;

#ifdef USE_SH_POOLS
                MemFreePtr(prev);
#else
                MemFree(prev);
#endif
            }
            else
            {
                entry -> attrib = 0;

                if (entry -> dir)
                {
                    hash_destroy((HASH_TABLE *) entry -> dir);
                    entry -> dir = NULL;
                }
            }
        }
    }
}



/* =======================================================

   FUNCTION:   hash_find_table (local only)

   PURPOSE:    Find a file wherever it exist.

   PARAMETERS: Filename with or without path, ptr to
               a hash table ptr.  The last parameter
               can be NULL if this information is not
               needed.

   RETURNS:    Ptr to an entry from the hash table, or
               NULL if file not found or error
               encountered.

   ======================================================= */

HASH_ENTRY * hash_find_table(const char * name, HASH_TABLE ** table)
{
    int  path_used = FALSE,
         wild_path = FALSE;

    HASH_TABLE * ht = NULL;


    int  wild_len = 0,
         len = 0,
         i;

    char fullpath[ _MAX_PATH ] = {0},
                                 filename[ _MAX_FNAME ] = {0};

    HASH_ENTRY * entry = NULL;

#if( not RES_USE_FLAT_MODEL )
    LIST       * list = NULL;
#endif /* not RES_USE_FLAT_MODEL */

    if ( not GLOBAL_HASH_TABLE)
        return(NULL);


#if( RES_WILDCARD_PATHS )

    /* wildcard directory */


    /* wild carding a directory means that you can specify a neo-absolute path.  Where
       an absolute path is really a shortcut to typing a path relative to your current
       working directory, wildcarding a path allows you to specify a branch from a directory
       tree, and that you want to look for files in a file in all directories that have
       the same branch.  For instance,

            given this:

                *\objects\foo.dat

            all of these would be found:

                c:\game\install\objects\foo.dat
                f:\objects\foo.dat
                <archive>\data\objects\foo.dat

        Obviously this is primarily to find files that can be found on multiple volumes,
        rather than multiple locations on one volume.  The other way of doing this would
        have been to simply leave the volume identifier out of the hash lookup function.
        This however, would have made it impossible to explicitly specify a volume to
        differentiate between two files. */


    /* It may not be a good idea to implement this here (at a point so low in the code).
       I haven't made up my mind yet whether this should be done at a higher level
       (like the way unix expands argv[] wildcards) or whether it should be so
       cancerously ingrained here.  For now, here it goes. */

    if (name[0] == ASCII_ASTERISK)
    {

        /* strip the filename out of 'name' and then stuff what path information we
           do have into fullpath.  'wild_len' is the number of characters that we will
           compare of each of the directory entries in our search path with the path
           we have here. */

        wild_path = TRUE;
        path_used = TRUE;

        name++;
        strcpy(fullpath, name);
        wild_len = strlen(name);

        for (wild_len; (name[wild_len] not_eq ASCII_BACKSLASH) and wild_len; wild_len--) ;

        if (wild_len)
        {
            strcpy(filename, &name[ wild_len + 1 ]);
            fullpath[wild_len + 1] = 0x00;
        }
        else
            return(NULL);   /* improper use of wildcard directory */
    }

#endif /* RES_WILDCARD_PATHS */

    /* check to see if a directory name is specified */

    if ( not wild_path and (strchr(name, ASCII_BACKSLASH) or (name[0] == ASCII_DOT)))
    {
        /* utterly non-portable */
        /* create a full path name from what could be a partial path */
        if (res_fullpath(fullpath, name, _MAX_PATH) == NULL)
        {
            SAY_ERROR(RES_ERR_CANT_INTERPRET, name);
            return(NULL);
        }

        /* split the full path name into components */

        len = strlen(fullpath);

        for (i = len; i >= 0; i--)
        {
            if (fullpath[i] == ASCII_BACKSLASH)
            {
                strcpy(filename, &fullpath[i + 1]);
                fullpath[i + 1] = 0x00;
                break;
            }
        }

#if( RES_USE_FLAT_MODEL )

        if (filename[0] == 0x00)
        {
            SAY_ERROR(RES_ERR_CANT_INTERPRET, name);

            if (table)
                *table = NULL;

            return(NULL);
        }

#endif /* RES_USE_FLAT_MODEL */

        path_used = TRUE;
    }

#if( not RES_USE_FLAT_MODEL ) /* HIERARCHICAL MODEL */

    if (path_used)
    {

#if( RES_WILDCARD_PATHS )

        if (wild_path)
        {

            for (list = GLOBAL_PATH_LIST; list; list = list -> next)
            {

                ht = (HASH_TABLE *)list -> node;

                len = strlen(ht -> name);

                if (wild_len > len)
                    continue;

                if ( not strnicmp(&ht -> name[ len - wild_len - 1 ], fullpath, wild_len))
                {
                    entry = hash_find(filename, ht);

                    if (table)
                        *table = ht;

                    if (entry)
                        return(entry);
                }

            }

            if (table)
                *table = NULL;

            return(NULL);
        }
        else
        {
#endif /* RES_WILDCARD_PATHS */

            entry = hash_find(fullpath, GLOBAL_HASH_TABLE);

            if (entry)
            {
                if (entry -> dir)
                {
                    if (table)
                        *table = (HASH_TABLE *)entry -> dir;

                    return(hash_find(filename, (HASH_TABLE *)entry -> dir));
                }
            }
            else
            {
                if (table)
                    *table = NULL;

                return(NULL);
            }
        }

    }
    else   /* path_used == FALSE */
    {

        /* look in order */
        for (list = GLOBAL_PATH_LIST; list; list = list -> next)
        {
            entry = hash_find(name, (HASH_TABLE *)list -> node);

            if (entry)
            {
                if (table)
                    *table = (HASH_TABLE *)list -> node;

                return(entry);
            }
        }

        if (table)
            *table = NULL;

        return(NULL);
    }

#else /* not RES_USE_FLAT_MODEL */

    if (path_used)
        entry = hash_find(filename, GLOBAL_HASH_TABLE);
    else
        entry = hash_find(name, GLOBAL_HASH_TABLE);

    if ( not entry)
    {
        if (table)
            *table = NULL;
    }
    else if (table)
        *table = GLOBAL_HASH_TABLE;

    return(entry);
#endif

    if (table)
        *table = NULL;

    return(NULL);
}





/* =======================================================

    FUNCTION:    get_handle

    PURPOSE:    Returns the next available file handle.

    PARAMS:        None.

    RETURNS:    File handle or -1 on an error.

   ======================================================= */

int get_handle(void)
{
    int i;

    for (i = 0; i < MAX_FILE_HANDLES; i++)
        if (FILE_HANDLES[i].os_handle == -1)
            return(i);

    return(-1);
}



/* =======================================================

    FUNCTION:    split_handle

    PURPOSE:    Returns the filename and the path for
                a given full file desription.

    PARAMS:     Full filename (path bitand filename), ptr to
                a buffer for filename, ptr to a buffer
                for path.

    RETURNS:    None.

    NOTES:      Ex:  c:\object\ships\klingon.dat yeilds
                filename = klingon.dat
                path = c:\object\ships\

   ======================================================= */

void split_path(const char * in_name, char * out_filename, char * out_dirpath)
{
    char fullpath[ _MAX_PATH ];
    int  len,
         i;

    if (res_fullpath(fullpath, in_name, _MAX_PATH) == NULL)
    {
        if (out_filename) *out_filename = '\0';

        if (out_dirpath) *out_dirpath = '\0';

        return;
    }

    len = strlen(fullpath);

    for (i = len; i >= 0; i--)
    {
        if ((fullpath[i] == ASCII_BACKSLASH) or
            (fullpath[i] == ASCII_COLON))
        {
            if (out_filename) strcpy(out_filename, &fullpath[i + 1]);

            fullpath[i + 1] = 0x00;

            if (out_dirpath) strcpy(out_dirpath, fullpath);

            return;
        }
    }

    if (out_filename) strcpy(out_filename, fullpath);

    if (out_dirpath) *out_dirpath = '\0';
}



/* =======================================================

    FUNCTION:   say_error (local only bitand debug only)

    PURPOSE:    Display dialog box with error message.

    PARAMETERS: Error number, optional string.

    RETURNS:    None.

   ======================================================= */

#if( RES_DEBUG_VERSION )

void _say_error(int error, const char * msg, int line, const char * filename)
{
    int err_code;
    char buffer[ 255 ];
    char title[] = "Resource Manager Error";
    char blank[] = "???";
    int  retval = 1;

    IF_LOG(LOG("ERROR (line: %d  file: %s):\n", line, filename));

    if ( not msg)
        msg = blank;

    RES_DEBUG_ERRNO = error;    /* set the equiv. of an errno */
    err_code = error;

    switch (error)
    {
            /* from erno.h */

        case EACCES:
            sprintf(buffer, "Tried to open read-only file (%s) for writing.", msg);
            break;

        case EEXIST:
            sprintf(buffer, "Create flag specified, but filename (%s) already exists.", msg);
            break;

        case ENOENT:
            sprintf(buffer, "File or path not found. (%s).", msg);
            break;

        default:    /* an error that is specific to this file */
            if ((error > RES_ERR_FIRST_ERROR) and (error < RES_ERR_LAST_ERROR))
            {
                /* error values run from -5000 up, so error will always be negative,
                  and so will (RES_ERR_OR_FIRST+1).  We want to normalize this to
                   0,1,2,3... */

                error = error + (-RES_ERR_FIRST_ERROR - 1);

                sprintf(buffer, "%s (%s)\n\n\nFile: %s\nLine: %d",  RES_ERR_OR_MSGS[ error ], msg, filename, line);
            }
            else
            {
                sprintf(buffer, "Unknown error encountered with file. (%s)\n\n\nFile: %s\nLine: %d\n", msg, filename, line);
            }
    }

    IF_LOG(LOG("---> %s\n", buffer));

    SHOULD_I_CALL_WITH(CALLBACK_ERROR, err_code, retval);

    //    MessageBox( NULL, buffer, title, MB_OK bitor MB_ICONEXCLAMATION );

    if ( not retval)
        MessageBox(RES_GLOBAL_HWND, buffer, title, MB_OK bitor MB_ICONEXCLAMATION);
}



/* =======================================================

    FUNCTION:   dbg_print

    PURPOSE:    Print a familiar looking file description.

    PARAMS:     Ptr to a file data structure.

    RETURNS:    None.

   ======================================================= */

void dbg_print(HASH_ENTRY * data)
{
    int attrib;

    attrib = data -> attrib;

    printf("%-17s", data -> name);
    printf("size: %5d ", data -> size);
    printf("csize: %5d ", data -> csize);
    printf("method: %1d ", data -> method);

    if (attrib bitand _A_RDONLY)  printf("R");
    else   printf(" ");

    if (attrib bitand _A_HIDDEN)  printf("H");
    else   printf(" ");

    if (attrib bitand _A_SYSTEM)  printf("S");
    else   printf(" ");

    if (attrib bitand _A_ARCH)    printf("A");
    else   printf(" ");

    if (attrib bitand _A_SUBDIR)  printf("\t\t<DIR>");
    else  printf("\t\t     ");

    printf("\n");
}



/* =======================================================

    FUNCTION:   dbg_device

    PURPOSE:    Print a device description.

    PARAMS:     Ptr to a device structure.

    RETURNS:    None.

   ======================================================= */

void dbg_device(DEVICE_ENTRY * dev)
{
    IF_LOG(LOG("Drive letter:  %c\n", dev -> letter));
    IF_LOG(LOG("Volume name:   %s\n", dev -> name));
    IF_LOG(LOG("Serial num:    %x-%x\n", HI_WORD(dev->serial), LO_WORD(dev -> serial)));
    IF_LOG(LOG("Type:          [%d] ", dev -> type));

    switch (dev -> type)
    {
        case 1:
            IF_LOG(LOG("The root directory does not exist.\n"));
            break;

        case DRIVE_REMOVABLE:
            IF_LOG(LOG("The drive can be removed from the drive.\n"));
            break;

        case DRIVE_FIXED:
            IF_LOG(LOG("The disk cannot be removed from the drive.\n"));
            break;

        case DRIVE_REMOTE:
            IF_LOG(LOG("The drive is a remote (network) drive.\n"));
            break;

        case DRIVE_CDROM:
            IF_LOG(LOG("The drive is a CD-ROM drive.\n"));
            break;

        case DRIVE_RAMDISK:
            IF_LOG(LOG("The drive is a RAM disk.\n"));
            break;

        case 0:
        default:
            IF_LOG(LOG("The drive type cannot be determined.\n"));
    }

    IF_LOG(LOG("----------------\n"));
}

/* =======================================================

    FUNCTION:    dbg_analyze_hash

    PURPOSE:     Print statistics regarding hash
                 performance bitand contents

    PARAMS:      Ptr to a hash table wrapper structure.

    RETURNS:     None.

   ======================================================= */

void dbg_analyze_hash(HASH_TABLE * hsh)
{
    int i,
        len,
        max_len,
        hits = 0;

    HASH_ENTRY * entry;

    IF_LOG(LOG("Table name............. %s\n", hsh -> name));
    IF_LOG(LOG("Hash size.............. %d\n", hsh -> table_size));
    IF_LOG(LOG("Num entries............ %d\n", hsh -> num_entries));
    IF_LOG(LOG("Ratio.................. %-3.0f%%\n", ((float)hsh -> table_size / (float)hsh -> num_entries) * 100.0));

    max_len = 0;

    for (i = 0; i < hsh->table_size; i++)
    {
        if (hsh -> table[i].next)
        {
            hits++;
            entry = &hsh -> table[i];
            len = 0;

            while (entry)
            {
                entry = entry -> next;
                len++;
            }

            if (len > max_len)
                max_len = len;
        }
    }

    IF_LOG(LOG("Hash collisions........ %d\n", hits));
    IF_LOG(LOG("Hash peformance........ %-3.0f%%\n", (1.0 - ((float)hits / (float)hsh->num_entries)) * 100.0));
    IF_LOG(LOG("Maximum chain length... %d\n", max_len));
}




/* =======================================================

    FUNCTION:    dbg_dir

    PURPOSE:     Print the contents of a directory

    PARAMS:      Ptr to a hash table wrapper structure.

    RETURNS:     None.

   ======================================================= */

void dbg_dir(HASH_TABLE * hsh)
{
    int i,
        count,
        hits = 0;

    HASH_ENTRY * entry;

    count = 0;

    IF_LOG(LOG("\n\n"));

    for (i = 0; i < hsh->table_size; i++)
    {
        if (hsh -> table[i].next)
        {
            hits++;
            entry = &hsh -> table[i];

            while (entry)
            {
                IF_LOG(LOG("%-14s ", entry -> name));
                count++;

                if ( not (count % 4)) IF_LOG(LOG("\n"));

                entry = entry -> next;
            }
        }
        else if (hsh -> table[i].attrib)
        {
            IF_LOG(LOG("%-14s ", hsh -> table[i].name));
            count++;

            if ( not (count % 4)) IF_LOG(LOG("\n"));
        }
    }

    IF_LOG(LOG("\n\n"));
}

#endif /* RES_DEBUG_VERSION */


/* -----------------------------------------------------------------------

    Unfortunately there was a pathologic problem using the VC++ runtime
    function _fullpath().  If you attach an archive to a virtual attach
    point, and then try to open the file using a relative pathname, and
    have RES_COERCE_FILENAMES true, you would always get File Not Found.

    This fixes the problem, however I am worried about two things:
    1) its late.
    2) this change affects a lot of functions

    Therefore, if things appear flaky define RES_USE_FULLPATH to be
    false, and go back to the old way.

    NOTE: If you continue using res_fullpath, be advised that current
    working directories of OTHER VOLUMES will not be maintained

    (eg F:readme.txt will parse to the literal string, not
    F:\some_dir\readme.txt).  This will not be fixed (I doubt it will
    affect anyone anyway).

    PS: Five days later and all is well with this function.  good sign.
   ------------------------------------------------------------------------ */


/* =======================================================

    FUNCTION:    res_fullpath

    PURPOSE:     Convert relative path names into
                 absolute path names.

    PARAMS:      Ptr to a buffer to store the absolute
                 pathname, ptr to a relative pathname,
                 maximum length of absolute pathname
                 (parameter is unused, but I include it
                 to maintain consistency with _fullpath().

    RETURNS:     Ptr to abs_buffer or NULL on error.

    NOTE:        All of these forms are supported:

                 ..\file.ext
                 \file.ext
                 ..\dir\dir\file.ext
                 \dir\file.ext
                 file.ext
                 c:\dir\file.ext

   ======================================================= */
char * res_fullpath(char * abs_buffer, const char * rel_buffer, int maxlen)
{
#if( RES_USE_FULLPATH )

    char  * colon;
    char  * rel;

    char    drive[4];
    char    tmp_buffer[ _MAX_PATH ];
    char    current_path[ _MAX_PATH ];

    int     chop = 0,
            len,
            i;

    maxlen;

    if ( not GLOBAL_PATH_LIST)
        strcpy(current_path, "c:\\");
    else
        strcpy(current_path, ((HASH_TABLE *)(GLOBAL_PATH_LIST -> node)) -> name);

    rel = (char *)rel_buffer;

    colon = strchr(rel, ASCII_COLON);

    if (colon)
    {
        if (rel_buffer[1] not_eq ASCII_COLON)
        {
            *abs_buffer = 0x00;
            return(NULL);
        }

        if ( not strstr(rel_buffer, ".."))
            return(strcpy(abs_buffer, rel_buffer));

        strncpy(drive, rel_buffer, 2);
        drive[2] = ASCII_BACKSLASH;
        drive[3] = 0x00;

        rel += 2;
    }
    else
    {
        strncpy(drive, current_path, 2);
        drive[2] = ASCII_BACKSLASH;
        drive[3] = 0x00;

        if (rel_buffer[0] == ASCII_BACKSLASH)
        {
            sprintf(abs_buffer, "%s%s", drive, &rel_buffer[1]);
            return(abs_buffer);
        }
    }

    if (*rel == ASCII_BACKSLASH)
    {
        sprintf(abs_buffer, "%s%s", drive, rel);
        return(abs_buffer);
    }

    while ( not memcmp(rel, "..", 3))
    {
        chop++;
        rel += 2;

        if (*rel == ASCII_BACKSLASH)
            *rel++;
    }

    len = strlen(current_path) - 2;

    for (i = len; i and chop; i--)
        if (current_path[i] == ASCII_BACKSLASH)
            chop--;

    if ( not i)
        i = 1;

    strncpy(tmp_buffer, current_path, i + 2);

    strcpy(&tmp_buffer[i + 2], rel);

    return(strcpy(abs_buffer, tmp_buffer));

#else

    return(_fullpath(abs_path, rel_path, maxlen));

#endif
}