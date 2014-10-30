/* ==========================================================================

    Unzip.cpp

    Interface into the inflation (decompression) of compressed archive files.
    These functions (as the name unzip.cpp implies) provide methods for
    extracting data from files created with either Phil Katz's PKZIP or
    Mark Adlers (GCC) unzip.

    Originally, this file was kept as similiar to the original dounzip.c
    that is part of the gcc unzip distribution.  The idea being that anyone
    could diff this file versus that file and see the changes that were
    made.  Unfortunately, the number have changes have become so great that
    there would be far more differences than similarities, making this
    process of little benefit.

    Inflate.cpp, inflate.h and unzip.h are exactly as they are in the
    gcc distribution.  **Touch them if you dare**.

    *06/12/96 - not any more, inflate is now completely thread safe, and
    therefore, looks nothing like it did before (not that its good - but
    it is better)*

    NOTE:  MSVC users - don't even bother trying to 'Update Dependencies'.
           Uggh  06/30/96 - This should work now

 Created     Mark Adler bitand Jean-loup Gailly (GCC)
    Modified    Roger Fujii (STTNG:AFU)
    Big Purge   Kevin Ray   (RESMGR)


 RESMGR Library component unzip.cpp (ver: 2.05 released 03/26/97)

   ========================================================================== */

#include <cISO646>
#include <ctype.h>
#include <fcntl.h>

#include "lists.h"
#include "unzip.h"                    /* includes, typedefs, macros, etc.        */
#include "resmgr.h"
#include "omni.h"

#include <assert.h>

#ifdef USE_SH_POOLS
#undef MemFree
#undef MemFreePtr
#undef MemMalloc
#include "Smartheap/Include/smrtheap.h"
extern MEM_POOL gResmgrMemPool;
#endif

/* -------------------------------------------------------------------------

    S T A T I C   D A T A

   ------------------------------------------------------------------------- */

static char seeklocked = FALSE;
int UNZIP_ERROR = 0; /* errno */

/* initialize signatures at runtime so unzip
   executable won't look like a zipfile */

static char near central_hdr_sig[5] = { 0, '\113', '\001', '\002' };
static char near local_hdr_sig[5]   = { 0, '\113', '\003', '\004' };
static char near end_central_sig[5] = { 0, '\113', '\005', '\006' };


/* -------------------------------------------------------------------------

    L O C A L   P R O T O T Y P E S

   ------------------------------------------------------------------------- */

int extract_or_test_member(int method, long ucsize, COMPRESSED_FILE * cmp);     /* return PK-type error code */
static int process_cdir_file_hdr(cdir_file_hdr * crec, ARCHIVE * arc);
static int do_string(unsigned int len, int option, char * filename, ARCHIVE * arc);
static int get_cdir_file_hdr(cdir_file_hdr * crec, ARCHIVE * arc);
int archive_size(ARCHIVE * arc);

int unzip_seek(LONGINT val, ARCHIVE * arc);
int process_local_file_hdr(local_file_hdr * lrec, char * buffer);

#define CREATE_LOCK(a)      CreateMutex( NULL,  FALSE, a );
#define REQUEST_LOCK(a)     WaitForSingleObject(a, INFINITE);
#define RELEASE_LOCK(a)     ReleaseMutex(a);
#define DESTROY_LOCK(a)     CloseHandle(a);


/* -------------------------------------------------------------------------

    E X T E R N A L   P R O T O T Y P E S bitand D A T A

   ------------------------------------------------------------------------- */

extern HASH_ENTRY * hash_add(struct _finddata_t * data, HASH_TABLE * table);
extern HASH_ENTRY * hash_find(const char * name, HASH_TABLE * hsh);                  /* find an entry within a hash table    */
extern HASH_TABLE * hash_create(int size, char * filename);
extern int          hash_resize(HASH_TABLE * table);

extern char       * res_fullpath(char * abs_buffer, const char * rel_buffer, int maxlen);

extern HASH_TABLE * GLOBAL_HASH_TABLE;
extern LIST *       GLOBAL_PATH_LIST;
extern char *       GLOBAL_SEARCH_PATH[];
extern int          GLOBAL_SEARCH_INDEX;
extern int          RES_DEBUG_ERRNO;

#if( RES_DEBUG_VERSION )
void
_say_error(int error, const char * msg, int line, const char * filename);

#   define SAY_ERROR(a,b)   _say_error((a),(b), __LINE__, __FILE__ )
#else
#   define SAY_ERROR(a,b)   {RES_DEBUG_ERRNO=(a);}
#endif /* RES_DEBUG_VERSION */




/* -------------------------------------------------------------------------

    C O M P I L E R   D E F I N I T I O N S

   ------------------------------------------------------------------------- */

#define MAKE_WORD(a)            makeword( &byterec[(a)] );
#define MAKE_LONG(a)            makelong( &byterec[(a)] );

#define WriteError(buf, len, strm) memcpy((void *)((int)arc -> out_buffer + arc -> out_count), buf, len); arc -> out_count += len;

#ifdef SFX
#  define UNKN_COMPR \
    (crec.compression_method not_eq STORED and crec.compression_method not_eq DEFLATED)
#else
#  define UNKN_COMPR \
    (crec.compression_method>IMPLODED and crec.compression_method not_eq DEFLATED)
#endif


#define UNZIP_LSEEK(a,b)        unzip_seek(a,b)





/* =======================================================

    FUNCTION:    archive_create

    PURPOSE:     Main interface into the unzip functions.
                 archive_create opens a compressed
                 archive file and adds the contents into
                 the hash table at the point specified.

    PARAMS:      Ptr to a directory to attach the
                 archive into, ptr to a complete filename
                 of the archive file, ptr to the hash
                 table for the attach point.

    RETURNS:     Ptr to the create archive structure.

   ======================================================= */

ARCHIVE * archive_create(const char * attach_point, const char * filename, HASH_TABLE * table, int replace_flag)
{
    ARCHIVE     * arc;
    HASH_ENTRY  * entry;
    struct _finddata_t data;     /* for hash_find */
    struct _finddata_t info;

    char    sig[5];
    char    vol_was;
    int     dir_was;
    int     error = 0,
            error_in_archive = 0,
            i, len;

    char    path_was[_MAX_PATH],
            path[_MAX_PATH];
    char *  fname;                  /* used to truncate path from filename */
    int     path_idx;

    ecdir_rec ecrec;                /* used in unzip.c, extract.c */

    //    int filnum=(-1);

    ush members_remaining;//,
    //        num_skipped = 0,
    //        num_bad_pwd = 0;

    char curfilename[FILNAMSIZ];
    cdir_file_hdr crec;             /* used in unzip.c, extract.c, misc.c */

#ifdef USE_SH_POOLS
    arc = (ARCHIVE *)MemAllocPtr(gResmgrMemPool, sizeof(ARCHIVE), 0);
#else
    arc = (ARCHIVE *)MemMalloc(sizeof(ARCHIVE), filename);
#endif

    if ( not arc)
    {
        UNZIP_ERROR = RES_ERR_NO_MEMORY;
        return(NULL);
    }

    strcpy(arc -> name, filename);

    arc -> lock = CREATE_LOCK("Archive");

    /*---------------------------------------------------------------------------
       Start by constructing the various PK signature strings.
      ---------------------------------------------------------------------------*/

    local_hdr_sig[0]   = '\120';   /* ASCII 'P', */
    central_hdr_sig[0] = '\120';
    end_central_sig[0] = '\120';   /* not EBCDIC */

    strcpy(path_was, attach_point);

#ifdef USE_SH_POOLS
    arc -> tmp_slide = (uch *)MemAllocPtr(gResmgrMemPool,  UNZIP_SLIDE_SIZE, 0);
#else
    arc -> tmp_slide = (uch *)MemMalloc(UNZIP_SLIDE_SIZE, "Slide");
#endif

    if (arc -> tmp_slide == NULL)
    {
        UNZIP_ERROR = RES_ERR_NO_MEMORY;
        return(NULL);      /* 4 extra for hold[] (below) */
    }

    {
        struct stat statbuf;

        if (SSTAT(filename, &statbuf) or (error = S_ISDIR(statbuf.st_mode)) not_eq 0)
        {
            ResCheckMedia(toupper(filename[0]) - 'A');   /* see if media has been swapped */

#ifdef USE_SH_POOLS
            MemFreePtr(arc -> tmp_slide);
            MemFreePtr(arc);
#else
            MemFree(arc -> tmp_slide);
            MemFree(arc);
#endif
            UNZIP_ERROR = RES_ERR_UNKNOWN;
            return(NULL);
        }

        arc -> length = statbuf.st_size;
    }

    if ((arc -> os_handle = _open(filename, O_RDONLY bitor O_BINARY)) < 0)
    {
        ResCheckMedia(toupper(filename[0]) - 'A');   /* see if media has been swapped */

#ifdef USE_SH_POOLS
        MemFreePtr(arc -> tmp_slide);
        MemFreePtr(arc);
#else
        MemFree(arc -> tmp_slide);
        MemFree(arc);
#endif
        UNZIP_ERROR = RES_ERR_NO_MEMORY;
        return(NULL);
    }

    res_fullpath(path, filename, _MAX_PATH);
    arc -> volume = (char)(toupper(path[0]) - 'A');

    arc -> start_buffer = 0;

    // Use 2048 size buffer for now, so we don't break things
    // But Input buffer is set to INPUTBUFSIZE >> 2048 + 4

    arc -> tmp_in_size = UNZIP_BUFFER_SIZE;

#ifdef USE_SH_POOLS

    if ((arc -> tmp_in_buffer = (uch *)MemAllocPtr(gResmgrMemPool, INPUTBUFSIZE, 0)) == NULL)
    {
        _close(arc -> os_handle);
        MemFreePtr(arc -> tmp_slide);
        MemFreePtr(arc);
        UNZIP_ERROR = RES_ERR_NO_MEMORY;
        return(NULL);
    }

#else

    if ((arc -> tmp_in_buffer = (uch *)MemMalloc(INPUTBUFSIZE, "input buffer")) == NULL)
    {
        _close(arc -> os_handle);
        MemFree(arc -> tmp_slide);
        MemFree(arc);
        UNZIP_ERROR = RES_ERR_NO_MEMORY;
        return(NULL);
    }

#endif

    arc -> tmp_hold = (uch *)(arc -> tmp_in_buffer + arc -> tmp_in_size);    /* to check for boundary-spanning signatures */

    arc -> tmp_in_ptr = (uch *)(arc -> tmp_in_buffer);

#if( RES_PREDETERMINE_SIZE )
    int sz;

    sz = archive_size(arc);

    /* printf( "archive size: %d\n", sz );*/

    if (ARCHIVE_TABLE_SIZE < (sz >> 1))
    {
        table -> num_entries = sz;

        if ( not hash_resize(table))
        {
            _close(arc -> os_handle);
#ifdef USE_SH_POOLS
            MemFreePtr(arc -> tmp_in_buffer);
            MemFreePtr(arc -> tmp_slide);
            MemFreePtr(arc);
#else
            MemFree(arc -> tmp_in_buffer);
            MemFree(arc -> tmp_slide);
            MemFree(arc);
#endif
            UNZIP_ERROR = RES_ERR_NO_MEMORY;
            return(NULL);
        }
    }

#endif /*RES_PREDETERMINE_SIZE */

    //****** (arc -> len) ?  66000L ? *****//
    if ((((error_in_archive = find_end_central_dir(MIN((arc -> length), 66000L), &ecrec, arc)) not_eq 0)))
    {
        _close(arc -> os_handle);
#ifdef USE_SH_POOLS
        MemFreePtr(arc -> tmp_in_buffer);
        MemFreePtr(arc -> tmp_slide);
        MemFreePtr(arc);
#else
        MemFree(arc -> tmp_in_buffer);
        MemFree(arc -> tmp_slide);
        MemFree(arc);
#endif
        UNZIP_ERROR = RES_ERR_NO_MEMORY;
        return(NULL);
    }

    /*-----------------------------------------------------------------------
        Compensate for missing or extra bytes, and seek to where the start
        of central directory should be.  If header not found, uncompensate
       and try again (necessary for at least some Atari archives created
        with STZIP, as well as archives created by J.H. Holm's ZIPSPLIT 1.1).
      ----------------------------------------------------------------------- */

    //LSEEK( ecrec.offset_start_central_directory );

    if (UNZIP_LSEEK(ecrec.offset_start_central_directory, arc))
    {
        _close(arc -> os_handle);
#ifdef USE_SH_POOLS
        MemFreePtr(arc -> tmp_in_buffer);
        MemFreePtr(arc -> tmp_slide);
        MemFreePtr(arc);
#else
        MemFree(arc -> tmp_in_buffer);
        MemFree(arc -> tmp_slide);
        MemFree(arc);
#endif
        UNZIP_ERROR = RES_ERR_NO_MEMORY;
        return(NULL);
    }


    /* -----------------------------------------------------------------------
        Seek to the start of the central directory one last time, since we
        have just read the first entry's signature bytes; then list, extract
        or test member files as instructed, and close the zipfile.
       -----------------------------------------------------------------------*/

    /* ---------------------------------------------------------------------------
        The basic idea of this function is as follows.  Since the central di-
        rectory lies at the end of the zipfile and the member files lie at the
        beginning or middle or wherever, it is not very desirable to simply
        read a central directory entry, jump to the member and extract it, and
        then jump back to the central directory.  In the case of a large zipfile
        this would lead to a whole lot of disk-grinding, especially if each mem-
        ber file is small.  Instead, we read from the central directory the per-
        tinent information for a block of files, then go extract/test the whole
        block.  Thus this routine contains two small(er) loops within a very
        large outer loop:  the first of the small ones reads a block of files
        from the central directory; the second extracts or tests each file; and
        the outer one loops over blocks.  There's some file-pointer positioning
        stuff in between, but that's about it.  Btw, it's because of this jump-
        ing around that we can afford to be lenient if an error occurs in one of
        the member files:  we should still be able to go find the other members,
        since we know the offset of each from the beginning of the zipfile.

        Begin main loop over blocks of member files.  We know the entire central
        directory is on this disk:  we would not have any of this information un-
        less the end-of-central-directory record was on this disk, and we would
 not have gotten to this routine unless this is also the disk on which
        the central directory starts.  In practice, this had better be the ONLY
        disk in the archive, but maybe someday we'll add multi-disk support.
       ---------------------------------------------------------------------------*/

    members_remaining = ecrec.total_entries_central_dir;

    //filelistcount = 0;
    //filelistmax = members_remaining + 30;
    //files = new direntry [ filelistmax ];

    while (members_remaining--)
    {
        if (readbuf(sig, 4, arc) <= 0)
        {
            error_in_archive = PK_EOF;
            SAY_ERROR(RES_ERR_BAD_ARCHIVE, filename);
            break;
        }

        if (strncmp(sig, central_hdr_sig, 4))       /* just to make sure                            */
        {
            error_in_archive = PK_BADERR;
            SAY_ERROR(RES_ERR_BAD_ARCHIVE, filename);
            break;
        }

        /* process_cdir_file_hdr() sets pInfo->hostnum, pInfo->lcflag */

        if ((error = process_cdir_file_hdr(&crec, arc)) not_eq PK_COOL)
        {
            error_in_archive = error;               /* only PK_EOF defined                          */
            SAY_ERROR(RES_ERR_BAD_ARCHIVE, filename);
            break;
        }

        if ((error = do_string(crec.filename_length, FILENAME, curfilename, arc)) not_eq PK_COOL)
        {
            if (error > error_in_archive)
                error_in_archive = error;

            if (error > PK_WARN)                    /* fatal:  no more left to do                   */
            {
                UNZIP_ERROR = RES_ERR_UNKNOWN;
                SAY_ERROR(RES_ERR_BAD_ARCHIVE, filename);
                break;
            }
        }

        if ((error = do_string(crec.extra_field_length, SKIP, NULL, arc)) not_eq PK_COOL)
        {
            if (error > error_in_archive)
                error_in_archive = error;

            if (error > PK_WARN)                    /* fatal: bail now                               */
            {
                UNZIP_ERROR = RES_ERR_UNKNOWN;
                SAY_ERROR(RES_ERR_BAD_ARCHIVE, filename);
                break;
            }
        }

        if ((error = do_string(crec.file_comment_length, SKIP, NULL, arc)) not_eq PK_COOL)
        {
            if (error > error_in_archive)
                error_in_archive = error;

            if (error > PK_WARN)                    /* fatal: bail now                               */
            {
                UNZIP_ERROR = RES_ERR_UNKNOWN;
                SAY_ERROR(RES_ERR_BAD_ARCHIVE, filename);
                break;
            }
        }

        fname = curfilename;
        path_idx = 0;                               /* see if directory has changed                 */

        if (attach_point[ strlen(attach_point) - 1 ] not_eq ASCII_BACKSLASH)
            sprintf(path, "%s\\%s", attach_point, curfilename);
        else
            sprintf(path, "%s%s", attach_point, curfilename);

        len = strlen(path);

        for (i = len; i > 2; i--)
        {
            if (path[i - 1] == ASCII_FORESLASH)
            {
                if ( not path_idx)
                {
                    path_idx = i - 1;
                    fname = &path[ path_idx + 1 ];
                }

                path[i - 1] = ASCII_BACKSLASH;
            }
        }

        strcpy(data.name, fname);   /* use a dummy to add entry to the hash table    */

        // (wrong -->) dir_was = GLOBAL_SEARCH_INDEX - 1;
        // KBR 9/4/97 - fixed.
        // dir_was was being set based on the CWD rather than the attach point.

        i = 0;
        dir_was = 0;

        do
        {

            if ( not stricmp(GLOBAL_SEARCH_PATH[i], attach_point))
            {
                dir_was = i;
                break;
            }

        }
        while (i++ < GLOBAL_SEARCH_INDEX);

        vol_was = (char)(toupper(path[0]) - 'A');

#if( not RES_USE_FLAT_MODEL )

        /* See if there is a new directory name.  If so, we need to create a new hash table,
           add this path into the global hash table, and continue add files into the new
           table.  This is assuming you're building the hierarchical model of course. */


        path[ path_idx + 1 ] = '\0';
        path[ path_idx + 2 ] = '\0';

        if (path_idx and strcmp(path, path_was))      /* new directory */
        {
            strcpy(path_was, path);

            /* see if it already exists */

            /*            RES_LOCK( GLOBAL_HASH_TABLE ); GFG */
            entry = hash_find(path, GLOBAL_HASH_TABLE);

            if (entry)
            {
                table = (HASH_TABLE *)entry -> dir;

                if ( not table)
                    break;
            }
            else
            {
#if( RES_DEBUG_VERSION )

                if (GLOBAL_SEARCH_INDEX >= (MAX_DIRECTORIES - 1))
                {
                    assert( not "Exceeded MAX_DIRECTORIES as defined in omni.h");
                    //                  SAY_ERROR( RES_ERR_TOO_MANY_DIRECTORIES, "ResAddPath" );
                    return(FALSE);
                }

#endif
                table = hash_create(ARCHIVE_TABLE_SIZE, path);

                strcpy(info.name, path);                  /* insert a dummy entry into the global hash table  */
                info.attrib = _A_SUBDIR bitor (unsigned int)FORCE_BIT;
                info.time_create = 0;
                info.time_access = 0;
                info.size = 0;

                entry = hash_add(&info, GLOBAL_HASH_TABLE);

                if ( not entry)
                    break;

                entry -> archive       = -1; /* the actual directory existence should not be considered
                                                as part of the archive.  All of the contents found within
                                                the directory are.   This allows a hard disk based file to
                                                override a zip archvie */

                entry -> volume        = vol_was;
                entry -> directory     = dir_was;

                GLOBAL_PATH_LIST = LIST_APPEND(GLOBAL_PATH_LIST, table);
                GLOBAL_SEARCH_PATH[ GLOBAL_SEARCH_INDEX ] = MemStrDup(path);
                dir_was = GLOBAL_SEARCH_INDEX++;

                entry -> dir = table;
            }

            /*            RES_UNLOCK( GLOBAL_HASH_TABLE );  GFG */
        }

#endif /* not RES_USE_FLAT_MODEL */

        if ( not (*data.name))
            continue;  /* this is usually a directory entry, which we'll decipher later */


#if( RES_REJECT_EMPTY_FILES )

        if ( not crec.csize)
            continue;

#endif
        data.attrib = (unsigned int)FORCE_BIT;
        data.time_create = 0;
        data.time_access = 0;
        data.size = crec.csize;

        /*        RES_LOCK( table );  GFG */
        entry = hash_find(data.name, table);    /* see if an entry already exists */

        if ( not entry)
            entry = hash_add(&data, table);     /* if not, create one             */
        else /* there is already a file with the same name here */
            if ( not replace_flag)
                continue;


        entry -> file_position = crec.relative_offset_local_header;

        //entry -> file_position = crec.relative_offset_local_header + 4 + LREC_SIZE + crec.filename_length;

        //if( crec.extra_field_length )
        //    entry -> file_position += crec.extra_field_length + 4 /* ?4? */;

        entry -> method        = crec.compression_method;
        entry -> size          = crec.ucsize;
        entry -> csize         = crec.csize;
        entry -> archive       = arc -> os_handle;
        entry -> volume        = vol_was;
        entry -> directory     = dir_was;
        /*        RES_UNLOCK( table );  GFG */
    }

    if (error > PK_WARN)    /* if error occurred, see if user ejected media during long inflation job */
        ResCheckMedia(toupper(filename[0]) - 'A');

#ifdef USE_SH_POOLS
    MemFreePtr(arc -> tmp_in_buffer);
    MemFreePtr(arc -> tmp_slide);
#else
    MemFree(arc -> tmp_in_buffer);
    MemFree(arc -> tmp_slide);
#endif

    return(arc);
}



/* =======================================================

    FUNCTION:    archive_delete

    PURPOSE:     Destructor for archive_create.  Destroy
                 an archive that was perviously created
                 using archive_create.

    PARAMS:      Ptr to an archive.

    RETURNS:     None.

   ======================================================= */

void archive_delete(ARCHIVE * arc)
{
    _close(arc -> os_handle);

    DESTROY_LOCK(arc -> lock);

#ifdef USE_SH_POOLS
    MemFreePtr(arc);
#else
    MemFree(arc);
#endif
}




/* =======================================================

    FUNCTION:    archive_size

    PURPOSE:     For large archives, its more efficient
                 to scan the entire file, counting the
                 number of entries, and then doing a
                 single hash_resize - than it is to
                 iteratively call hash_resize.

    PARAMS:      Archive ptr.

    RETURNS:     Number of entries in file.

   ======================================================= */

int archive_size(ARCHIVE * arc)
{
    char    sig[5];
    int     error = 0,
            error_in_archive = 0;
    int     count;

    ecdir_rec ecrec;                /* used in unzip.c, extract.c */

    //    int filnum=(-1);

    ush members_remaining;//,
    //        num_skipped = 0,
    //        num_bad_pwd = 0;

    char curfilename[FILNAMSIZ];
    cdir_file_hdr crec;             /* used in unzip.c, extract.c, misc.c */

    /*---------------------------------------------------------------------------
       Start by constructing the various PK signature strings.
      ---------------------------------------------------------------------------*/

    local_hdr_sig[0]   = '\120';   /* ASCII 'P', */
    central_hdr_sig[0] = '\120';
    end_central_sig[0] = '\120';   /* not EBCDIC */

    if ((((error_in_archive = find_end_central_dir(MIN((arc -> length), 66000L), &ecrec, arc)) not_eq 0)))
        return(-1);

    if (UNZIP_LSEEK(ecrec.offset_start_central_directory, arc))
        return(-1);

    members_remaining = ecrec.total_entries_central_dir;

    count = 0;

    while (members_remaining--)
    {
        if (readbuf(sig, 4, arc) <= 0)
        {
            error_in_archive = PK_EOF;
            break;
        }

        if (strncmp(sig, central_hdr_sig, 4))       /* just to make sure                            */
        {
            error_in_archive = PK_BADERR;
            break;
        }

        /* process_cdir_file_hdr() sets pInfo->hostnum, pInfo->lcflag */

        if ((error = process_cdir_file_hdr(&crec, arc)) not_eq PK_COOL)
        {
            error_in_archive = error;               /* only PK_EOF defined                          */
            break;
        }

        if ((error = do_string(crec.filename_length, FILENAME, curfilename, arc)) not_eq PK_COOL)
        {
            if (error > error_in_archive)
                error_in_archive = error;

            if (error > PK_WARN)                    /* fatal:  no more left to do                   */
            {
                UNZIP_ERROR = RES_ERR_UNKNOWN;
                break;
            }
        }

        if ((error = do_string(crec.file_comment_length, SKIP, NULL, arc)) not_eq PK_COOL)
        {
            if (error > error_in_archive)
                error_in_archive = error;

            if (error > PK_WARN)                    /* fatal: bail now                               */
            {
                UNZIP_ERROR = RES_ERR_UNKNOWN;
                break;
            }
        }

        if ( not (*curfilename))
            continue;  /* this is usually a directory entry, which we'll decipher later */

        count++;
    }

    if (error > PK_WARN)    /* if error occurred, see if user ejected media during long inflation job */
        return(-1);

    return(count);
}


int unzip_seek(LONGINT val, ARCHIVE * arc)
{
    LONGINT request = val/*+extra_bytes*/,
            inbuf_offset = request % INBUFSIZ,
            bufstart = request - inbuf_offset;

    if (request < 0)
    {
        return(-1);
    }
    else
    {
        if (bufstart not_eq arc -> start_buffer)
        {
            arc -> start_buffer = lseek(arc -> os_handle, (LONGINT)bufstart, SEEK_SET);

            if ((arc -> tmp_in_count = read(arc -> os_handle, (char *)arc -> tmp_in_buffer, INBUFSIZ)) <= 0)
                return(-1);

            arc -> tmp_in_ptr = arc -> tmp_in_buffer + inbuf_offset;
            arc -> tmp_in_count -= (int)inbuf_offset;
        }
        else
        {
            arc -> tmp_in_count += (arc -> tmp_in_ptr - arc -> tmp_in_buffer) - inbuf_offset;
            arc -> tmp_in_ptr = arc -> tmp_in_buffer + inbuf_offset;
        }
    }

    return(0);   /* success (i guess) */
}



/* =======================================================

   FUNCTION:   getfiletomem()

   PURPOSE:    Unpack the file.

   PARAMETERS: ...

   RETURNS:    Error code if any.

   ======================================================= */

#if 0
int getfiletomem(char * myfile, char **retbuf, long * size, COMPRESSED_FILE * cmp)      /* return PK-type error code */
{
    int error;
    struct direntry * entry;

    if ((entry = getentry(myfile)) == NULL)
        return -1;

    if ((int)entry -> file_position < 0)
    {
        int fd;
        char filename[_MAX_PATH];
        construct_path(local_file_dir, filename, myfile, entry->file_position);
        // sprintf(filename, "%s%s", local_file_dir, myfile);

        if ((fd = open(filename, O_BINARY bitor O_RDONLY)) >= 0)
        {
#ifdef USE_SH_POOLS

            if ((cmp -> out_buffer = (char *)MemAllocPtr(gResmgrMemPool, entry->file_size + strlen(myfile) + 1, 0)) == NULL)
                return -1;

#else

            if ((cmp -> out_buffer = (char *)MemMalloc(entry->file_size + strlen(myfile) + 1, myfile)) == NULL)
                return -1;

#endif
            read(fd, cmp -> out_buffer, entry->file_size);
            strcpy(cmp -> out_buffer + entry->file_size, myfile);
            close(fd);
        }
        else
        {
            return -1;
        }
    }
    else
    {
        if (entry->file_csize > INPUTBUFSIZE)
        {
            cmp -> in_size = INPUTBUFSIZE;
        }
        else
        {
            cmp -> in_size = entry->file_csize; /* alloced size should be either 2K or 10K already*/
        }

        cmp -> in_ptr = cmp -> in_buffer;
        cmp -> in_count = 0;

        /*
         * just about to extract file:  if extracting to disk, check if
         * already exists, and if so, take appropriate action according to
          * fflag/uflag/overwrite_all/etc. (we couldn't do this in upper
         * loop because we don't store the possibly renamed filename[] in
         * info[])
         */

#ifdef USE_SH_POOLS

        if ((cmp -> out_buffer = (char *)MemAllocPtr(gResmgrMemPool, entry->file_size + strlen(myfile) + 1, 0)) == NULL)
            return -1;

#else

        if ((cmp -> out_buffer = (char *)MemMalloc(entry->file_size + strlen(myfile) + 1, myfile)) == NULL)
            return -1;

#endif

        outcnt = 0;

        lseek(tmp -> archive -> os_handle, entry->file_position, SEEK_SET);

        if ((error = extract_or_test_member(entry->method, entry->file_csize, cmp)) not_eq PK_COOL)
        {
#ifdef USE_SH_POOLS
            MemFreePtr(tmp -> out_buffer);
#else
            MemFree(tmp -> out_buffer);
#endif
            return -1;        /* (unless disk full) */
        }

        strcpy(tmp -> out_buffer + entry -> file_size, myfile);
    }

    *retbuf = tmp -> out_buffer;

    if (size not_eq NULL)
        *size = entry->file_size;

    return(0);
}
#endif

/* =======================================================

   FUNCTION:   makeword

   PURPOSE:    Convert Intel style 'short' integer to
               non-Intel non-16-bit host format.  This
               routine also takes care of byte-ordering.

   PARAMETERS: Short (cast to unsigned char).

   RETURNS:    Unsigned Short.

   ======================================================= */

static ush makeword(uch *b)
{
    return (ush)((b[1] << 8) bitor b[0]);
}


/* =======================================================

   FUNCTION:   makelong

   PURPOSE:    Convert Intel style 'long' integer to
               non-Intel non-16-bit host format.  This
               routine also takes care of byte-ordering.

   PARAMETERS: Long (cast to unsigned char).

   RETURNS:    Unsigned Long.

   ======================================================= */

static ulg makelong(uch *sig)
{
    return (((ulg)sig[3]) << 24) bitor (((ulg)sig[2]) << 16) bitor (((ulg)sig[1]) << 8) bitor ((ulg)sig[0]);
}



/* =======================================================

   FUNCTION:   process_cdir_file_hdr()

   PURPOSE:    Get central directory info, save host and
               method numbers, and set flag for lowercase
               conversion of filename, depending on the
               OS from which the file is coming.

   PARAMETERS: ptr to ...

   RETURNS:    Error if any.

   ======================================================= */

static int process_cdir_file_hdr(cdir_file_hdr * crec, ARCHIVE * arc)
{
    int error;

    if ((error = get_cdir_file_hdr(crec, arc)) not_eq 0)
        return(error);

    return(PK_COOL);
}




/* =======================================================

   FUNCTION:   extract_or_test_member()

   PURPOSE:    Unpack the file.

   PARAMETERS: Compression method, compressed file size1,
               ptr to an archive.

   RETURNS:    Error code if any.

   ======================================================= */

int extract_or_test_member(int method, long fcsize, COMPRESSED_FILE * cmp)
{
    int    r,
           error = PK_COOL;

    char * pos;

    switch (method)
    {
        case STORED:
            pos  = (char *)cmp -> out_buffer;
            seeklocked = TRUE;

            do
            {
                r = (fcsize > INPUTBUFSIZE) ? INPUTBUFSIZE : fcsize;
                read(cmp -> archive -> os_handle, pos, r);
                pos += r;
                fcsize -= r;
            }
            while (fcsize > 0);

            seeklocked = FALSE;
            break;

#ifndef SFX

        case SHRUNK:
            break;

        case REDUCED1:
        case REDUCED2:
        case REDUCED3:
        case REDUCED4:
            break;

        case IMPLODED:
            break;
#endif /* not SFX */

        case DEFLATED:
            cmp -> csize = fcsize;
            seeklocked = TRUE;

            if ((r = inflate(cmp)) not_eq 0)
                error = (r == 3) ? PK_MEM2 : PK_ERR;

            /* free allocated memory */
            seeklocked = FALSE;
            inflate_free();
            break;

        default:   /* should never get to this point */
            /* close and delete file before return? */
            error = PK_WARN;
            break;
    } /* end switch (compression method) */

    /* NOTE THAT WE ****DISABLE**** crc checking on here..... and in fileio */

    return(error);
}



/* =======================================================

   FUNCTION:   readbuf()

   PURPOSE:    ...

   PARAMETERS: Ptr to buffer, num bytes.

   RETURNS:    Number of bytes read into buffer.

   ======================================================= */

int readbuf(char * buf, unsigned size, ARCHIVE * arc)
{
    register int count;
    int n;

    n = size;

    while (size)
    {
        if (arc -> tmp_in_count == 0)
        {
            if ((arc -> tmp_in_count = read(arc -> os_handle, (char *)arc -> tmp_in_buffer, arc -> tmp_in_size)) == 0)
            {
                //arc -> start_buffer += arc -> tmp_in_size;
                return((int)(n - size));
            }
            else
            {
                if (arc -> tmp_in_count < 0)
                {
                    //arc -> start_buffer += arc -> tmp_in_size;
                    return(-1);    /* discarding some data, but better than lockup */
                }
            }

            /* buffer ALWAYS starts on a block boundary:  */

            arc -> start_buffer += arc -> tmp_in_size; /* NUKE??? */
            arc -> tmp_in_ptr = arc -> tmp_in_buffer;
        }

        count = MIN(size, (unsigned)arc -> tmp_in_count);
        memcpy(buf, arc -> tmp_in_ptr, count);
        buf += count;
        arc -> tmp_in_ptr += count;
        arc -> tmp_in_count -= count;
        size -= count;
    }

    return(n);
}



/* =======================================================

   FUNCTION:   readbyte()

   PURPOSE:    Refill the input buffer and return a byte
               if available - otherwise return 'EOF'.

   PARAMETERS: Ptr to a compressed file structure

   RETURNS:    Byte read or EOF token.

   ======================================================= */

int readbyte(COMPRESSED_FILE * cmp)
{
    if ((cmp -> in_count = read(cmp -> archive -> os_handle, (char *)cmp -> in_buffer, cmp -> in_size)) <= 0)
        return(EOF);

    // nuke(?) cmp -> start_buffer += cmp -> in_size;   /* always starts on a block boundary */
    cmp -> in_ptr = cmp -> in_buffer;
    --cmp -> in_count;

    //   return( (*(int*)(cmp -> in_ptr))++ );
    return(*cmp->in_ptr++);
}


/* =======================================================

   FUNCTION:   flush()

   PURPOSE:    ...

   PARAMETERS: Ptr to buffer, size, ...

   RETURNS:    Byte read or EOF token.

   NOTE:       Only called from inflate.h

       --- archive_entry should be compress_file ---

   ======================================================= */

int flush(uch * rawbuf, ulg size, int unshrink, COMPRESSED_FILE * cmp)
{
    if (size == 0L)     /* testing or nothing to write:  all done    */
        return(0);

    unshrink = 0;        /* Keep compiler from complaining            */

    memcpy(cmp -> out_buffer + cmp -> out_count, rawbuf, size);
    cmp -> out_count += size;

    return(0);
}



/* =======================================================

   FUNCTION:   find_end_central_dir()

   PURPOSE:    ...

   PARAMETERS: Ptr to buffer, size, ...

   RETURNS:    Error code if any.

   ======================================================= */

int find_end_central_dir(long searchlen, ecdir_rec *ecrec, ARCHIVE * arc)
{
    int i,
        numblks,
        found = FALSE;

    LONGINT tail_len;
    ec_byte_rec byterec;

    LONGINT real_ecrec_offset,
            expect_ecrec_offset;

    /*---------------------------------------------------------------------------
        Treat case of short zipfile separately.
      ---------------------------------------------------------------------------*/

    if ((arc -> length) <= arc -> tmp_in_size)
    {
        lseek(arc -> os_handle, 0L, SEEK_SET);

        if ((arc -> tmp_in_count = read(arc -> os_handle, (char *)arc -> tmp_in_buffer, (unsigned int)(arc -> length))) == (int)(arc -> length))
        {
            /* 'P' must be at least 22 bytes from end of zipfile */

            for (arc -> tmp_in_ptr = arc -> tmp_in_buffer + (arc -> length - 22);
                 arc -> tmp_in_ptr >= arc -> tmp_in_buffer;
                 arc -> tmp_in_ptr = arc -> tmp_in_ptr - 1 /* was: --inptr  Uggh */
                )
            {
                //                if((native(*((int*)arc -> tmp_in_ptr)) == 'P') and 
                // not strncmp((char *)arc -> tmp_in_ptr, end_central_sig, 4))  /* GFG 31/01/98
                if (((*(char*)(arc -> tmp_in_ptr)) == 'P') and not strncmp((char *)arc -> tmp_in_ptr, end_central_sig, 4))

                {
                    arc -> tmp_in_count -= (int)arc -> tmp_in_ptr - (int)arc -> tmp_in_buffer;
                    found = TRUE;
                    break;
                }
            }
        }
    }
    else    /* --------------------------------------------------------------------------- */
    {
        /*  Zipfile is longer than inbufsiz:  may need to loop.  Start with short      */
        /*  block at end of zipfile (if not TOO short).                                */
        /* --------------------------------------------------------------------------- */

        if ((tail_len = (arc -> length) % arc -> tmp_in_size) > ECREC_SIZE)
        {
            arc -> start_buffer = lseek(arc -> os_handle, (arc -> length) - tail_len, SEEK_SET);
            arc -> tmp_in_count = read(arc -> os_handle, (char *)arc -> tmp_in_buffer, (unsigned int)tail_len);

            if (arc -> tmp_in_count not_eq (int)tail_len)
                goto fail;      /* shut up; it's expedient */

            /* 'P' must be at least 22 bytes from end of zipfile */
            for (arc -> tmp_in_ptr = arc -> tmp_in_buffer + (tail_len - 22);
                 arc -> tmp_in_ptr >= arc -> tmp_in_buffer;
                 arc -> tmp_in_ptr = arc -> tmp_in_ptr - 1
                )
            {
                if (((*(char*)(arc -> tmp_in_ptr)) == 'P') and not strncmp((char *)arc -> tmp_in_ptr, end_central_sig, 4))
                {
                    arc -> tmp_in_count -= (int)arc -> tmp_in_ptr - (int)arc -> tmp_in_buffer;
                    found = TRUE;
                    break;
                }
            }

            /* sig may span block boundary: */
            strncpy((char *)arc -> tmp_hold, (char *)arc -> tmp_in_buffer, 3);
        }
        else
            arc -> start_buffer = (arc -> length) - tail_len;


        /*-----------------------------------------------------------------------
            Loop through blocks of zipfile data, starting at the end and going
            toward the beginning.  In general, need not check whole zipfile for
            signature, but may want to do so if testing.
          -----------------------------------------------------------------------*/

        numblks = (int)((searchlen - tail_len + (arc -> tmp_in_size - 1)) / arc -> tmp_in_size);

        /*   ==amount=   ==done==   ==rounding==    =blksiz=  */

        for (i = 1; not found and (i <= numblks);  ++i)
        {
            arc -> start_buffer -= arc -> tmp_in_size;
            lseek(arc -> os_handle, arc -> start_buffer, SEEK_SET);

            if ((arc -> tmp_in_count = read(arc -> os_handle, (char *)arc -> tmp_in_buffer, arc -> tmp_in_size)) not_eq arc -> tmp_in_size)
                break;   /* fall through and fail */

            for (arc -> tmp_in_ptr = arc -> tmp_in_buffer + arc -> tmp_in_size - 1;
                 arc -> tmp_in_ptr >= arc -> tmp_in_buffer;
                 arc -> tmp_in_ptr = arc -> tmp_in_ptr - 1
                )
            {
                if ((*(char *)arc -> tmp_in_ptr == 'P') and not strncmp((char *)arc -> tmp_in_ptr, end_central_sig, 4))
                {
                    arc -> tmp_in_count -= ((int)arc -> tmp_in_ptr - (int)arc -> tmp_in_buffer);
                    found = TRUE;
                    break;
                }
            }

            /* sig may span block boundary: */
            strncpy((char *)arc -> tmp_hold, (char *)arc -> tmp_in_buffer, 3);
        }

    }

    /*---------------------------------------------------------------------------
        Searched through whole region where signature should be without finding
        it.  Print informational message and die a horrible death.
      ---------------------------------------------------------------------------*/


fail:

    if ( not found)
    {
#ifdef MSWIN
        MessageBeep(1);
#endif
        //  if (qflag or (zipinfo_mode and not hflag))
        //        fprintf(stderr, "[%s]\n", zipfn);
        //  fprintf(stderr, "\
        //    End-of-central-directory signature not found.  Either this file is not\n\
        //    a zipfile, or it constitutes one disk of a multi-part archive.  In the\n\
        //    latter case the central directory and zipfile comment will be found on\n\
        //    the last disk(s) of this archive.\n\n");

        return PK_ERR;   /* failed */
    }

    /*---------------------------------------------------------------------------
        Found the signature, so get the end-central data before returning.  Do
        any necessary machine-type conversions (byte ordering, structure padding
        compensation) by reading data into character array and copying to struct.
      ---------------------------------------------------------------------------*/

    real_ecrec_offset = (int)arc -> start_buffer + ((int)arc -> tmp_in_ptr - (int)arc -> tmp_in_buffer);

    if (readbuf((char *)byterec, ECREC_SIZE + 4, arc) <= 0)
        return(PK_EOF);

    ecrec -> number_this_disk                   = MAKE_WORD(NUMBER_THIS_DISK);
    ecrec -> num_disk_with_start_central_dir    = MAKE_WORD(NUM_DISK_WITH_START_CENTRAL_DIR);
    ecrec -> num_entries_centrl_dir_ths_disk    = MAKE_WORD(NUM_ENTRIES_CENTRL_DIR_THS_DISK);
    ecrec -> total_entries_central_dir          = MAKE_WORD(TOTAL_ENTRIES_CENTRAL_DIR);
    ecrec -> size_central_directory             = MAKE_LONG(SIZE_CENTRAL_DIRECTORY);
    ecrec -> offset_start_central_directory     = MAKE_LONG(OFFSET_START_CENTRAL_DIRECTORY);
    ecrec -> zipfile_comment_length             = MAKE_WORD(ZIPFILE_COMMENT_LENGTH);

    expect_ecrec_offset = ecrec -> offset_start_central_directory + ecrec -> size_central_directory;

    return(PK_COOL);
}




/********************************/
/* Function get_cdir_file_hdr() */
/********************************/

static int get_cdir_file_hdr(cdir_file_hdr *crec, ARCHIVE * arc)     /* return PK-type error code */
{
    cdir_byte_hdr byterec;

    /*---------------------------------------------------------------------------
        Read the next central directory entry and do any necessary machine-type
        conversions (byte ordering, structure padding compensation--do so by
        copying the data from the array into which it was read (byterec) to the
        usable struct (crec)).
      ---------------------------------------------------------------------------*/

    if (readbuf((char *)byterec, CREC_SIZE, arc) <= 0)
        return(PK_EOF);

    crec->version_made_by[0] = byterec[C_VERSION_MADE_BY_0];
    crec->version_made_by[1] = byterec[C_VERSION_MADE_BY_1];
    crec->version_needed_to_extract[0] = byterec[C_VERSION_NEEDED_TO_EXTRACT_0];
    crec->version_needed_to_extract[1] = byterec[C_VERSION_NEEDED_TO_EXTRACT_1];

    crec->general_purpose_bit_flag = makeword(&byterec[C_GENERAL_PURPOSE_BIT_FLAG]);
    crec->compression_method = makeword(&byterec[C_COMPRESSION_METHOD]);
    crec->last_mod_file_time = makeword(&byterec[C_LAST_MOD_FILE_TIME]);
    crec->last_mod_file_date = makeword(&byterec[C_LAST_MOD_FILE_DATE]);
    crec->crc32 = makelong(&byterec[C_CRC32]);
    crec->csize = makelong(&byterec[C_COMPRESSED_SIZE]);
    crec->ucsize = makelong(&byterec[C_UNCOMPRESSED_SIZE]);
    crec->filename_length = makeword(&byterec[C_FILENAME_LENGTH]);
    crec->extra_field_length = makeword(&byterec[C_EXTRA_FIELD_LENGTH]);
    crec->file_comment_length = makeword(&byterec[C_FILE_COMMENT_LENGTH]);
    crec->disk_number_start = makeword(&byterec[C_DISK_NUMBER_START]);
    crec->internal_file_attributes = makeword(&byterec[C_INTERNAL_FILE_ATTRIBUTES]);
    crec->external_file_attributes = makelong(&byterec[C_EXTERNAL_FILE_ATTRIBUTES]);  /* LONG, not word */
    crec->relative_offset_local_header = makelong(&byterec[C_RELATIVE_OFFSET_LOCAL_HEADER]);

    return(PK_COOL);
}





/************************/
/* Function do_string() */
/************************/

static int do_string(unsigned int len, int option, char * filename, ARCHIVE * arc)       /* return PK-type error code */
{
    int error = PK_OK;
    ush extra_len;

    /*---------------------------------------------------------------------------
      This function processes arbitrary-length (well, usually) strings.  Three
      options are allowed:  SKIP, wherein the string is skipped (pretty logical,
      eh?); DISPLAY, wherein the string is printed to standard output after un-
      dergoing any necessary or unnecessary character conversions; and FILENAME,
      wherein the string is put into the filename[] array after undergoing ap-
      propriate conversions (including case-conversion, if that is indicated:
      see the global variable pInfo->lcflag).  The latter option should be OK,
      since filename is now dimensioned at 1025, but we check anyway.

      The string, by the way, is assumed to start at the current file-pointer
      position; its length is given by len.  So start off by checking length
      of string:  if zero, we're already done.
      ---------------------------------------------------------------------------*/

    if ( not len)
        return(PK_COOL);

    switch (option)
    {

            /*
             * First case:  print string on standard output.  First set loop vari-
             * ables, then loop through the comment in chunks of OUTBUFSIZ bytes,
             * converting formats and printing as we go.  The second half of the
             * loop conditional was added because the file might be truncated, in
             * which case comment_bytes_left will remain at some non-zero value for
             * all time.  outbuf is used as a scratch buffer because it is avail-
             * able (we should be either before or in between any file processing).
             * [The typecast in front of the MIN() macro was added because of the
             * new promotion rules under ANSI C; readbuf() wants an int, but MIN()
             * returns a signed long, if I understand things correctly.  The proto-
             * type should handle it, but just in case...]
             */

        case DISPLAY:
        case FILENAME:
            extra_len = 0;

            if (len >= FILNAMSIZ)
            {
                // fprintf(stderr, "warning:  filename too long--truncating.\n");
                error = PK_WARN;
                extra_len = (ush)(len - FILNAMSIZ + 1);
                len = FILNAMSIZ - 1;
            }

            if (readbuf(filename, len, arc) <= 0)
                return(PK_EOF);

            filename[ len ] = '\0';        /* terminate w/zero:  ASCIIZ    */

            A_TO_N(filename);            /* translate string to native    */

            if ( not extra_len)             /* we're done here                */
                break;

            /*
             * We truncated the filename, so print what's left and then fall
             * through to the SKIP routine.
             */

            // fprintf(stderr, "[ %s ]\n", filename);

            len = extra_len;

            /* =========== FALL THROUGH =========== */

            /*
             * Third case:  skip string, adjusting readbuf's internal variables
             * as necessary (and possibly skipping to and reading a new block of
             * data).
             */

        case SKIP:
            if (UNZIP_LSEEK((int)(arc -> start_buffer) + ((int)(arc -> tmp_in_ptr) - (int)(arc -> tmp_in_buffer)) + len, arc))
                return(PK_WARN);

            break;

            /*
             * Fourth case:  assume we're at the start of an "extra field"; malloc
             * storage for it and read data into the allocated space.
             */

    } /* switch */

    return(error);
}



/***************************************/
/*  Function process_local_file_hdr()  */
/***************************************/

int process_local_file_hdr(local_file_hdr * lrec, char * buffer)      /* return PK-type error code */
{
    local_byte_hdr byterec;


    /*---------------------------------------------------------------------------
      Read the next local file header and do any necessary machine-type con-
      versions (byte ordering, structure padding compensation--do so by copy-
      ing the data from the array into which it was read (byterec) to the
      usable struct (lrec)).
      ---------------------------------------------------------------------------*/

    //if( readbuf((char *)byterec, LREC_SIZE) <= 0)
    //    return PK_EOF;

    memcpy(byterec, buffer, LREC_SIZE);

    lrec->version_needed_to_extract[0] = byterec[L_VERSION_NEEDED_TO_EXTRACT_0];
    lrec->version_needed_to_extract[1] = byterec[L_VERSION_NEEDED_TO_EXTRACT_1];

    lrec->general_purpose_bit_flag = makeword(&byterec[L_GENERAL_PURPOSE_BIT_FLAG]);
    lrec->compression_method = makeword(&byterec[L_COMPRESSION_METHOD]);
    lrec->last_mod_file_time = makeword(&byterec[L_LAST_MOD_FILE_TIME]);
    lrec->last_mod_file_date = makeword(&byterec[L_LAST_MOD_FILE_DATE]);
    lrec->crc32 = makelong(&byterec[L_CRC32]);
    lrec->csize = makelong(&byterec[L_COMPRESSED_SIZE]);
    lrec->ucsize = makelong(&byterec[L_UNCOMPRESSED_SIZE]);
    lrec->filename_length = makeword(&byterec[L_FILENAME_LENGTH]);
    lrec->extra_field_length = makeword(&byterec[L_EXTRA_FIELD_LENGTH]);

    if ((lrec->general_purpose_bit_flag bitand 8) not_eq 0)
    {
        /* can't trust local header, use central directory
           If this is the problem, you should probably extract the
           entire zip and then rebuild it.  The local header should
           always be more reliable than the global, and in your case
           it is not (sorry, but you're fucked) */
        SAY_ERROR(RES_ERR_BAD_ARCHIVE, "Big problemo, read comment");
        return(PK_ERR);
    }

    return(PK_COOL);

} /* end function process_local_file_hdr() */
