/*---------------------------------------------------------------------------

  unzip.h

  This header file is used by all of the UnZip source files.  Its contents
  are divided into seven more-or-less separate sections:  predefined macros,
  OS-dependent includes, (mostly) OS-independent defines, typedefs, function 
  prototypes (or "forward declarations," in the case of non-ANSI compilers),
  macros, and global-variable declarations.

  created	Mark Adler & Jean-loup Gailly
  modified	Roger Fujii (STTNG:AFU)
  modified  Kevin Ray	(RESMGR)
  ---------------------------------------------------------------------------*/

#ifndef __unzip_h   /* prevent multiple inclusions */
#define __unzip_h

#include "resmgr.h"

/*****************************************/
/*  Predefined, Machine-specific Macros  */
/*****************************************/

#if defined(__GO32__) && defined(unix)   /* MS-DOS extender:  NOT Unix */
#  undef unix
#endif


/* define MSDOS for Turbo C (unless OS/2) and Power C as well as Microsoft C */
#ifdef __POWERC
#  define __TURBOC__
#  define MSDOS
#endif /* __POWERC */
#if defined(__MSDOS__) && (!defined(MSDOS))   /* just to make sure */
#  define MSDOS
#endif

/* use prototypes and ANSI libraries if __STDC__, or Microsoft or Borland C, or
 * Silicon Graphics, or Convex?, or IBM C Set/2, or GNU gcc/emx, or Watcom C,
 * or Macintosh, or Windows NT, or Sequent, or Atari.
 */
#if defined(__STDC__) || defined(MSDOS) || defined(sgi)
#  ifndef PROTO
#    define PROTO
#  endif
#  define MODERN
#endif

#if defined(__IBMC__) || defined(__EMX__) || defined(__WATCOMC__)
#  ifndef PROTO
#    define PROTO
#  endif
#  define MODERN
#endif

#if defined(THINK_C) || defined(MPW) || defined(WIN32) || defined(_SEQUENT_)
#  ifndef PROTO
#    define PROTO
#  endif
#  define MODERN
#endif


/* turn off prototypes if requested */
#if defined(NOPROTO) && defined(PROTO)
#  undef PROTO
#endif

/* used to remove arguments in function prototypes for non-ANSI C */
#ifdef PROTO
#  define OF(a) a
#else /* !PROTO */
#  define OF(a) ()
#endif /* ?PROTO */

/* stat() bug for Borland, Watcom, VAX C (also GNU?), and Atari ST MiNT on
 * TOS filesystems:  returns 0 for wildcards!  (returns 0xffffffff on Minix
 * filesystem or U: drive under Atari MiNT) */
#if (defined(__TURBOC__) || defined(__WATCOMC__) || defined(VMS))
#  define WILD_STAT_BUG
#endif

#define SSTAT stat
#define STRNICMP zstrnicmp




/***************************/
/*  OS-Dependent Includes  */
/***************************/

#ifndef MINIX            /* Minix needs it after all the other includes (?) */
#  include <stdio.h>
#endif

#include <ctype.h>       /* skip for VMS, to use tolower() function? */
#include <errno.h>       /* used in mapname() */
#include <string.h>      /* GRR:  EXPERIMENTAL! */

#ifdef MODERN
#  include <limits.h>    /* GRR:  EXPERIMENTAL!  (can be deleted) */
#endif

#ifdef EFT
#  define LONGINT off_t  /* Amdahl UTS nonsense ("extended file types") */
#else
#  define LONGINT long
#endif

#ifdef MODERN
#  ifndef NO_STDDEF_H
#    include <stddef.h>
#  endif
#  ifndef NO_STDLIB_H
#    include <stdlib.h>    /* standard library prototypes, malloc(), etc. */
#  endif
   typedef size_t extent;
   typedef void voidp;
#else /* !MODERN */
   LONGINT lseek();
   char *malloc();
   typedef unsigned int extent;
   typedef char voidp;
#  define void int
#endif /* ?MODERN */

/* this include must be down here for SysV.4, for some reason... */
#include <signal.h>      /* used in unzip.c, file_io.c */


/*---------------------------------------------------------------------------
    MS-DOS and OS/2 section:
  ---------------------------------------------------------------------------*/

#ifdef __IBMC__
#  define S_IFMT 0xF000
#  define timezone _timezone
#  define PIPE_ERROR (errno == EERRSET || errno == EOS2ERR)
#endif

#ifdef __WATCOMC__
#  define __32BIT__
#  undef far
#  define far
#  undef near
#  define near
#  define PIPE_ERROR (errno == -1)
#endif

#ifdef __EMX__
#  ifndef __32BIT__
#    define __32BIT__
#  endif
#  define far
#endif

#ifdef __GO32__              /* note: MS-DOS compiler, not OS/2 */
#  ifndef __32BIT__
#    define __32BIT__
#  endif
#  include <sys/timeb.h>     /* for structure ftime */
   int setmode(int, int);    /* not in djgpp's include files */
#endif

#if defined(_MSC_VER) && (!defined(MSC))
#  define MSC                /* for old versions, MSC must be set explicitly */
#endif

#if defined(MSDOS) || defined(OS2)
#  include <sys/types.h>      /* off_t, time_t, dev_t, ... */
#  include <sys/stat.h>
#  include <io.h>             /* lseek(), open(), setftime(), dup(), creat() */
#  include <time.h>           /* localtime() */
#  include <fcntl.h>          /* O_BINARY for open() w/o CR/LF translation */
#  define DIR_END '\\'
#  if (defined(M_I86CM) || defined(M_I86LM))
#    define MED_MEM
#  endif
#  if (defined(__COMPACT__) || defined(__LARGE__) || defined(__HUGE__))
#    define MED_MEM
#  endif
#  ifndef __32BIT__
#    ifndef MED_MEM
#      define SMALL_MEM
#    endif
#    define USE_FWRITE        /* write() cannot write more than 32767 bytes */
#  endif
#  define DATE_FORMAT   dateformat()
#  define lenEOL        2
#  define PutNativeEOL  {*q++ = native(CR); *q++ = native(LF);}
#endif

/*---------------------------------------------------------------------------
    NT section:
  ---------------------------------------------------------------------------*/

#ifdef WIN32  /* NT */
#  include <sys/types.h>        /* off_t, time_t, dev_t, ... */
#  include <sys/stat.h>
#  include <io.h>               /* read(), open(), etc. */
#  include <time.h>
#  include <memory.h>
#  include <direct.h>           /* mkdir() */
#  include <fcntl.h>
#  if defined(FILE_IO_C)
#    include <conio.h>
#    include <sys\types.h>
#    include <sys\utime.h>
#    include <windows.h>
#  endif
#  define DATE_FORMAT   DF_MDY
#  define lenEOL        2
#  define PutNativeEOL  {*q++ = native(CR); *q++ = native(LF);}
#  define NT
#endif


/*************/
/*  Defines  */
/*************/

#define UNZIP
#define UNZIP_VERSION     20   /* compatible with PKUNZIP 2.0 */

#if defined(MSDOS) || defined(NT) || defined(OS2)
#  define DOS_NT_OS2
#endif

#if defined(MSDOS) || defined(OS2)
#  define DOS_OS2
#endif

#if defined(MSDOS) || defined(OS2) || defined(ATARI_ST)
#  define DOS_OS2_TOS
#endif

#if defined(MSDOS) || defined(ATARI_ST)
#  define DOS_TOS
#endif

#if defined(MSDOS) || defined(TOPS20) || defined(VMS)
#  define DOS_T20_VMS
#endif

/* GRR:  NT defines MSDOS?? */
#if (!defined(MSDOS) && !defined(__IBMC__)) || defined(NT)
#  define near
#  define far
#endif
#if defined(__GO32__) || defined(__EMX__)
#  define near
#  define far
#endif

/* clean up with a couple of defaults */
#ifndef DIR_END
#  define DIR_END '/'       /* last char before program name (or filename) */
#endif
#ifndef RETURN
#  define RETURN  return    /* only used in main() */
#endif

#define DIR_BLKSIZ  64      /* number of directory entries per block
                             *  (should fit in 4096 bytes, usually) */
#ifndef WSIZE
#  define WSIZE     0x8000  /* window size--must be a power of two, and */
#endif                      /*  at least 32K for zip's deflate method */

#ifndef INBUFSIZ
#  define INBUFSIZ  0x0800  /* 2K:  works for MS-DOS small model */
#endif

/* Logic for case of small memory, length of EOL > 1:  if OUTBUFSIZ == 2048,
 * OUTBUFSIZ>>1 == 1024 and OUTBUFSIZ>>7 == 16; therefore rawbuf is 1008 bytes
 * and transbuf 1040 bytes.  Have room for 32 extra EOL chars; 1008/32 == 31.5
 * chars/line, smaller than estimated 35-70 characters per line for C source
 * and normal text.  Hence difference is sufficient for most "average" files.
 * (Argument scales for larger OUTBUFSIZ.)
 */
#ifdef SMALL_MEM          /* i.e., 16-bit OS's:  MS-DOS, OS/2 1.x, etc. */
#  define NO_ZIPINFO      /* GRR:  true until move all strings to far memory */
#  define OUTBUFSIZ INBUFSIZ
#  if (lenEOL == 1)
#    define RAWBUFSIZ (OUTBUFSIZ>>1)
#  else
#    define RAWBUFSIZ ((OUTBUFSIZ>>1) - (OUTBUFSIZ>>7))
#  endif
#  define TRANSBUFSIZ (OUTBUFSIZ-RAWBUFSIZ)
#else
#  ifdef MED_MEM
#    define OUTBUFSIZ 0xFF80     /* can't malloc arrays of 0xFFE8 or more */
#    define TRANSBUFSIZ 0xFF80
#  else
#    define OUTBUFSIZ (lenEOL*WSIZE)  /* more efficient text conversion */
#    define TRANSBUFSIZ (lenEOL*OUTBUFSIZ)
#  endif
#  define RAWBUFSIZ OUTBUFSIZ
#endif /* ?SMALL_MEM */

#if (defined(SFX) && !defined(NO_ZIPINFO))
#  define NO_ZIPINFO
#endif

#ifndef O_BINARY
#  define O_BINARY  0
#endif

#ifndef PIPE_ERROR
#  define PIPE_ERROR (errno == EPIPE)
#endif

#if defined(MODERN) || defined(AMIGA)
#  define FOPR "rb"
#  define FOPM "r+b"
#  ifdef TOPS20          /* TOPS-20 MODERN?  You kidding? */
#    define FOPW "w8"
#  else
#    define FOPW "wb"
#  endif
#else /* !MODERN */
#  define FOPR "r"
#  define FOPM "r+"
#  define FOPW "w"
#endif /* ?MODERN */

/*
 * If <limits.h> exists on most systems, should include that, since it may
 * define some or all of the following:  NAME_MAX, PATH_MAX, _POSIX_NAME_MAX,
 * _POSIX_PATH_MAX.
 */
#ifdef DOS_OS2_TOS
#  include <limits.h>
#endif

#ifndef PATH_MAX
#  ifdef MAXPATHLEN
#    define PATH_MAX      MAXPATHLEN    /* in <sys/param.h> on some systems */
#  else
#    ifdef _MAX_PATH
#      define PATH_MAX    _MAX_PATH
#    else
#      if FILENAME_MAX > 255
#        define PATH_MAX  FILENAME_MAX  /* used like PATH_MAX on some systems */
#      else
#        define PATH_MAX  1024
#      endif
#    endif /* ?_MAX_PATH */
#  endif /* ?MAXPATHLEN */
#endif /* !PATH_MAX */

#define FILNAMSIZ  (PATH_MAX+1)

#ifdef SHORT_SYMS                   /* Mark Williams C, ...? */
#  define extract_or_test_files     xtr_or_tst_files
#  define extract_or_test_member    xtr_or_tst_member
#endif

#ifdef REALLY_SHORT_SYMS            /* TOPS-20 linker:  first 6 chars */
#  define process_cdir_file_hdr     XXpcdfh
#  define process_local_file_hdr    XXplfh
#  define extract_or_test_files     XXxotf  /* necessary? */
#  define extract_or_test_member    XXxotm  /* necessary? */
#  define check_for_newer           XXcfn
#  define overwrite_all             XXoa
#  define process_all_files         XXpaf
#  define extra_field               XXef
#  define explode_lit8              XXel8
#  define explode_lit4              XXel4
#  define explode_nolit8            XXnl8
#  define explode_nolit4            XXnl4
#  define cpdist8                   XXcpdist8
#  define inflate_codes             XXic
#  define inflate_stored            XXis
#  define inflate_fixed             XXif
#  define inflate_dynamic           XXid
#  define inflate_block             XXib
#  define maxcodemax                XXmax
#endif

#define ZSUFX             ".zip"
#define CENTRAL_HDR_SIG   "\113\001\002"   /* the infamous "PK" signature */
#define LOCAL_HDR_SIG     "\113\003\004"   /*  bytes, sans "P" (so unzip */
#define END_CENTRAL_SIG   "\113\005\006"   /*  executable not mistaken for */
#define EXTD_LOCAL_SIG    "\113\007\010"   /*  zipfile itself) */

#define SKIP              0    /* choice of activities for do_string() */
#define DISPLAY           1
#define FILENAME          2
#define EXTRA_FIELD       3

#define DOES_NOT_EXIST    -1   /* return values for check_for_newer() */
#define EXISTS_AND_OLDER  0
#define EXISTS_AND_NEWER  1

#define ROOT              0    /* checkdir() extract-to path:  called once */
#define INIT              1    /* allocate buildpath:  called once per member */
#define APPEND_DIR        2    /* append a dir comp.:  many times per member */
#define APPEND_NAME       3    /* append actual filename:  once per member */
#define GETPATH           4    /* retrieve the complete path and free it */
#define END               5    /* free root path prior to exiting program */

/* version_made_by codes (central dir):  make sure these */
/*  are not defined on their respective systems!! */
#define FS_FAT_           0    /* filesystem used by MS-DOS, OS/2, NT */
#define AMIGA_            1
#define VMS_              2
#define UNIX_             3
#define VM_CMS_           4
#define ATARI_            5    /* what if it's a minix filesystem? [cjh] */
#define FS_HPFS_          6    /* filesystem used by OS/2, NT */
#define MAC_              7
#define Z_SYSTEM_         8
#define CPM_              9
#define TOPS20_           10
#define FS_NTFS_          11   /* filesystem used by Windows NT */
/* #define QDOS_          12?  */
#define NUM_HOSTS         12   /* index of last system + 1 */

#define STORED            0    /* compression methods */
#define SHRUNK            1
#define REDUCED1          2
#define REDUCED2          3
#define REDUCED3          4
#define REDUCED4          5
#define IMPLODED          6
#define TOKENIZED         7
#define DEFLATED          8
#define NUM_METHODS       9    /* index of last method + 1 */
/* don't forget to update list_files() appropriately if NUM_METHODS changes */

#define PK_OK             0    /* no error */
#define PK_COOL           0    /* no error */
#define PK_GNARLY         0    /* no error */
#define PK_WARN           1    /* warning error */
#define PK_ERR            2    /* error in zipfile */
#define PK_BADERR         3    /* severe error in zipfile */
#define PK_MEM            4    /* insufficient memory */
#define PK_MEM2           5    /* insufficient memory */
#define PK_MEM3           6    /* insufficient memory */
#define PK_MEM4           7    /* insufficient memory */
#define PK_MEM5           8    /* insufficient memory */
#define PK_NOZIP          9    /* zipfile not found */
#define PK_PARAM          10   /* bad or illegal parameters specified */
#define PK_FIND           11   /* no files found */
#define PK_DISK           50   /* disk full */
#define PK_EOF            51   /* unexpected EOF */

#define IZ_DIR            76   /* potential zipfile is a directory */
#define IZ_CREATED_DIR    77   /* directory created: set time and permissions */
#define IZ_VOL_LABEL      78   /* volume label, but can't set on hard disk */

#define DF_MDY            0    /* date format 10/26/91 (USA only) */
#define DF_DMY            1    /* date format 26/10/91 (most of the world) */
#define DF_YMD            2    /* date format 91/10/26 (a few countries) */

/*---------------------------------------------------------------------------
    True sizes of the various headers, as defined by PKWARE--so it is not
    likely that these will ever change.  But if they do, make sure both these
    defines AND the typedefs below get updated accordingly.
  ---------------------------------------------------------------------------*/
#define LREC_SIZE     26    /* lengths of local file headers, central */
#define CREC_SIZE     42    /*  directory headers, and the end-of-    */
#define ECREC_SIZE    18    /*  central-dir record, respectively      */

#define MAX_BITS      13                 /* used in unshrink() */
#define HSIZE         (1 << MAX_BITS)    /* size of global work area */

#define LF      10    /* '\n' on ASCII machines; must be 10 due to EBCDIC */
#define CR      13    /* '\r' on ASCII machines; must be 13 due to EBCDIC */
#define CTRLZ   26    /* DOS & OS/2 EOF marker (used in file_io.c, vms.c) */

#ifdef EBCDIC
#  define native(c)   ebcdic[(c)]
#  define NATIVE      "EBCDIC"
#endif

#ifdef MPW
#  define FFLUSH(f)   putc('\n',f)
#else
#  define FFLUSH      fflush
#endif

#ifdef ZMEM     /* GRR:  THIS IS AN EXPERIMENT... (seems to work) */
#  undef ZMEM
#  define memcpy(dest,src,len)   bcopy(src,dest,len)
#  define memzero                bzero
#else
#  define memzero(dest,len)      memset(dest,0,len)
#endif

#define ENV_UNZIP     "UNZIP"
#define ENV_ZIPINFO   "ZIPINFO"

#if !defined(QQ) && !defined(NOQQ)
#  define QQ
#endif

#ifdef QQ                         /* Newtware version:  no file */
#  define QCOND     (!qflag)      /*  comments with -vq or -vqq */
#else                             /* Bill Davidsen version:  no way to */
#  define QCOND     (which_hdr)   /*  kill file comments when listing */
#endif

#ifdef OLD_QQ
#  define QCOND2    (qflag < 2)
#else
#  define QCOND2    (!qflag)
#endif

#ifndef TRUE
#  define TRUE      1   /* sort of obvious */
#endif
#ifndef FALSE
#  define FALSE     0
#endif

#ifndef SEEK_SET
#  define SEEK_SET  0
#  define SEEK_CUR  1
#  define SEEK_END  2
#endif

#if (defined(UNIX) && defined(S_IFLNK) && !defined(MTS))
#  define SYMLINKS
#  ifndef S_ISLNK
#    define S_ISLNK(m)  (((m) & S_IFMT) == S_IFLNK)
#  endif
#endif /* UNIX && S_IFLNK && !MTS */

#ifndef S_ISDIR
#  define S_ISDIR(m)  (((m) & S_IFMT) == S_IFDIR)
#endif

#ifndef IS_VOLID
#  define IS_VOLID(m)  ((m) & 0x08)
#endif


#define SIGNATURE_SIZE  4



/**************/
/*  Typedefs  */
/**************/

typedef char              bule;
typedef unsigned char     uch;  /* code assumes unsigned bytes; these type-  */
typedef unsigned short    ush;  /*  defs replace byte/UWORD/ULONG (which are */
typedef unsigned long     ulg;  /*  predefined on some systems) & match zip  */

typedef struct min_info {
    long offset;
    ulg compr_size;          /* compressed size (needed if extended header) */
    ulg crc;                 /* crc (needed if extended header) */
    int hostnum;
    unsigned file_attr;      /* local flavor, as used by creat(), chmod()... */
    unsigned encrypted : 1;  /* file encrypted: decrypt before uncompressing */
    unsigned ExtLocHdr : 1;  /* use time instead of CRC for decrypt check */
    unsigned textfile : 1;   /* file is text (according to zip) */
    unsigned textmode : 1;   /* file is to be extracted as text */
    unsigned lcflag : 1;     /* convert filename to lowercase */
    unsigned vollabel : 1;   /* "file" is an MS-DOS volume (disk) label */
} min_info;

typedef struct VMStimbuf {
    char *revdate;           /* (both correspond to Unix modtime/st_mtime) */
    char *credate;
} VMStimbuf;

/*---------------------------------------------------------------------------
    Zipfile work area declarations.
  ---------------------------------------------------------------------------*/

#ifdef MALLOC_WORK
   union work {
     struct {
       short *Prefix_of;            /* (8193 * sizeof(short)) */
       uch *Suffix_of;
       uch *Stack;
     } shrink;                      /* unshrink() */
     uch *Slide;                    /* explode(), inflate(), unreduce() */
   };

#else /* !MALLOC_WORK */
   union work {
     struct {
#ifdef HSIZE2    /* needed to avoid errors on some machines? */
       short Prefix_of[HSIZE + 2];  /* (8194 * sizeof(short)) */
       uch Suffix_of[HSIZE + 2];    /* also s-f length_nodes (smaller) */
       uch Stack[HSIZE + 2];        /* also s-f distance_nodes (smaller) */
#else /* !HSIZE2 */
       short Prefix_of[HSIZE];      /* (8192 * sizeof(short)) */
       uch Suffix_of[HSIZE];        /* also s-f length_nodes (smaller) */
       uch Stack[HSIZE];            /* also s-f distance_nodes (smaller) */
#endif /* ?HSIZE2 */
     } shrink;
     uch Slide[WSIZE];
   };
#endif /* ?MALLOC_WORK */

//#define prefix_of   area.shrink.Prefix_of
//#define suffix_of   area.shrink.Suffix_of
//#define stack       area.shrink.Stack
//#define slide       area.Slide

/*---------------------------------------------------------------------------
    Zipfile layout declarations.  If these headers ever change, make sure the
    xxREC_SIZE defines (above) change with them!
  ---------------------------------------------------------------------------*/

   typedef uch   local_byte_hdr[ LREC_SIZE ];
#      define L_VERSION_NEEDED_TO_EXTRACT_0     0
#      define L_VERSION_NEEDED_TO_EXTRACT_1     1
#      define L_GENERAL_PURPOSE_BIT_FLAG        2
#      define L_COMPRESSION_METHOD              4
#      define L_LAST_MOD_FILE_TIME              6
#      define L_LAST_MOD_FILE_DATE              8
#      define L_CRC32                           10
#      define L_COMPRESSED_SIZE                 14
#      define L_UNCOMPRESSED_SIZE               18
#      define L_FILENAME_LENGTH                 22
#      define L_EXTRA_FIELD_LENGTH              24

   typedef uch   cdir_byte_hdr[ CREC_SIZE ];
#      define C_VERSION_MADE_BY_0               0
#      define C_VERSION_MADE_BY_1               1
#      define C_VERSION_NEEDED_TO_EXTRACT_0     2
#      define C_VERSION_NEEDED_TO_EXTRACT_1     3
#      define C_GENERAL_PURPOSE_BIT_FLAG        4
#      define C_COMPRESSION_METHOD              6
#      define C_LAST_MOD_FILE_TIME              8
#      define C_LAST_MOD_FILE_DATE              10
#      define C_CRC32                           12
#      define C_COMPRESSED_SIZE                 16
#      define C_UNCOMPRESSED_SIZE               20
#      define C_FILENAME_LENGTH                 24
#      define C_EXTRA_FIELD_LENGTH              26
#      define C_FILE_COMMENT_LENGTH             28
#      define C_DISK_NUMBER_START               30
#      define C_INTERNAL_FILE_ATTRIBUTES        32
#      define C_EXTERNAL_FILE_ATTRIBUTES        34
#      define C_RELATIVE_OFFSET_LOCAL_HEADER    38

   typedef uch   ec_byte_rec[ ECREC_SIZE+4 ];
/*     define SIGNATURE                         0   space-holder only */
#      define NUMBER_THIS_DISK                  4
#      define NUM_DISK_WITH_START_CENTRAL_DIR   6
#      define NUM_ENTRIES_CENTRL_DIR_THS_DISK   8
#      define TOTAL_ENTRIES_CENTRAL_DIR         10
#      define SIZE_CENTRAL_DIRECTORY            12
#      define OFFSET_START_CENTRAL_DIRECTORY    16
#      define ZIPFILE_COMMENT_LENGTH            20


   typedef struct local_file_header {                 /* LOCAL */
       uch version_needed_to_extract[2];
       ush general_purpose_bit_flag;
       ush compression_method;
       ush last_mod_file_time;
       ush last_mod_file_date;
       ulg crc32;
       ulg csize;
       ulg ucsize;
       ush filename_length;
       ush extra_field_length;
   } local_file_hdr;

   typedef struct central_directory_file_header {     /* CENTRAL */
       uch version_made_by[2];
       uch version_needed_to_extract[2];
       ush general_purpose_bit_flag;
       ush compression_method;
       ush last_mod_file_time;
       ush last_mod_file_date;
       ulg crc32;
       ulg csize;
       ulg ucsize;
       ush filename_length;
       ush extra_field_length;
       ush file_comment_length;
       ush disk_number_start;
       ush internal_file_attributes;
       ulg external_file_attributes;
       ulg relative_offset_local_header;
   } cdir_file_hdr;

   typedef struct end_central_dir_record {            /* END CENTRAL */
       ush number_this_disk;
       ush num_disk_with_start_central_dir;
       ush num_entries_centrl_dir_ths_disk;
       ush total_entries_central_dir;
       ulg size_central_directory;
       ulg offset_start_central_directory;
       ush zipfile_comment_length;
   } ecdir_rec;





/*************************/
/*  Function Prototypes  */
/*************************/

#ifndef __
#  define __   OF
#endif

/*---------------------------------------------------------------------------
    Functions in unzip.c (main initialization/driver routines):
  ---------------------------------------------------------------------------*/

int    uz_opts                   __((int *pargc, char ***pargv));
int    usage                     __((int error));
int    process_zipfiles          __((void));
int    do_seekable               __((int lastchance));
int    uz_end_central            __((void));
//int    process_cdir_file_hdr     __((void));
//int    process_local_file_hdr    __((void));

/*---------------------------------------------------------------------------
    Functions in zipinfo.c (zipfile-listing routines):
  ---------------------------------------------------------------------------*/

int    zi_opts                   __((int *pargc, char ***pargv));
int    zi_end_central            __((void));
int    zipinfo                   __((void));
/* static int    zi_long         __((void)); */
/* static int    zi_short        __((void)); */
/* static char  *zi_time         __((ush *datez, ush *timez)); */
ulg    SizeOfEAs                 __((void *extra_field));  /* also in os2.c? */
int    list_files                __((void));
/* static int    ratio           __((ulg uc, ulg c)); */

/*---------------------------------------------------------------------------
    Functions in file_io.c:
  ---------------------------------------------------------------------------*/

int      open_input_file    __((char *));
int      open_outfile       __((void));                        /* also vms.c */
int      readbuf            __((char *buf, register unsigned len, ARCHIVE * arc));
int      FillBitBuffer      __((void));
int      readbyte           __((COMPRESSED_FILE * cmp));
#ifdef FUNZIP
   int   flush              __((ulg size));
#else
   int   flush              __((uch *buf, ulg size, int unshrink, COMPRESSED_FILE * cmp));
#endif
void     handler            __((int signal));
time_t   dos_to_unix_time   __((unsigned ddate, unsigned dtime));
int      check_for_newer    __((char *filename));       /* also os2.c, vms.c */
int      find_end_central_dir __((long searchlen, ecdir_rec *, ARCHIVE * arc ));        /* find_ecrec */
//int      get_cdir_file_hdr  __((void));                    /* get_cdir_ent */
//int      do_string          __((unsigned int len, int option, char *buffer));
//ush      makeword           __((uch *b));
//ulg      makelong           __((uch *sig));
int      zstrnicmp __((register char *s1, register char *s2, register int n));

#ifdef ZMEM   /* MUST be ifdef'd because of conflicts with the standard def. */
   char *memset __((register char *, register char, register unsigned int));
   char *memcpy __((register char *, register char *, register unsigned int));
#endif

/*---------------------------------------------------------------------------
    Functions in extract.c:
  ---------------------------------------------------------------------------*/

int    extract_or_test_files     __((char *file, char *outbuf, long *size));
/* static int   store_info               __((void)); */
/* static int   extract_or_test_member   __((void)); */
int    memextract                __((uch *, ulg, uch *, ulg));
int    FlushMemory               __((void));
int    ReadMemoryByte            __((ush *x));

/*---------------------------------------------------------------------------
    Decompression functions:
  ---------------------------------------------------------------------------*/

int    explode                   __((void));                    /* explode.c */
int    inflate                   __((COMPRESSED_FILE *));                    /* inflate.c */
int    inflate_free              __((void));                    /* inflate.c */
void   unreduce                  __((void));                   /* unreduce.c */
/* static void  LoadFollowers    __((void));                    * unreduce.c */
int    unshrink                  __((void));                   /* unshrink.c */
/* static void  partial_clear    __((void));                    * unshrink.c */

/*---------------------------------------------------------------------------
    MSDOS-only functions:
  ---------------------------------------------------------------------------*/

#if (defined(__GO32__) || (defined(MSDOS) && defined(__EMX__)))
   void _dos_setftime(int, unsigned short, unsigned short);       /* msdos.c */
   void _dos_setfileattr(char *, int);                            /* msdos.c */
   unsigned _dos_creat(char *, unsigned, int *);                  /* msdos.c */
   void _dos_getdrive(unsigned *);                                /* msdos.c */
   unsigned _dos_close(int);                                      /* msdos.c */
#endif


/*---------------------------------------------------------------------------
    Miscellaneous/shared functions:
  ---------------------------------------------------------------------------*/

int      match             __((char *s, char *p, int ic));        /* match.c */
int      iswild            __((char *p));                         /* match.c */

void     envargs           __((int *, char ***, char *));       /* envargs.c */
void     mksargs           __((int *, char ***));               /* envargs.c */

int      dateformat        __((void));
int      mapattr           __((void));                              /* local */
int      mapname           __((int renamed));                       /* local */
int      checkdir          __((char *pathcomp, int flag));          /* local */
char    *do_wild           __((char *wildzipfn));                   /* local */
#ifndef MTS /* macro in MTS */
   void  close_outfile     __((void));                              /* local */
#endif




/************/
/*  Macros  */
/************/

#ifndef MAX
#  define MAX(a,b)   ((a) > (b) ? (a) : (b))
#endif
#ifndef MIN
#  define MIN(a,b)   ((a) < (b) ? (a) : (b))
#endif

#ifdef DEBUG
#  define Trace(x)   fprintf x
#else
#  define Trace(x)
#endif

#if defined(UNIX) || defined(T20_VMS)   /* generally old systems */
#  define ToLower(x)   ((char)(isupper((int)x)? tolower((int)x) : x))
#else
#  define ToLower      tolower          /* assumed "smart"; used in match() */
#endif


#define LSEEK(abs_offset) {LONGINT request=(abs_offset)/*+extra_bytes*/,\
   inbuf_offset=request%INBUFSIZ, bufstart=request-inbuf_offset;\
   if(request<0) {/*fprintf(stderr, SeekMsg, ReportMsg); */return(3);}\
   else if(bufstart!=cur_zipfile_bufstart)\
   {cur_zipfile_bufstart=lseek(zipfd,(LONGINT)bufstart,SEEK_SET);\
   if((incnt=read(zipfd,(char *)inbuf,INBUFSIZ))<=0) return(51);\
   inptr=inbuf+(int)inbuf_offset; incnt-=(int)inbuf_offset;} else\
   {incnt+=(inptr-inbuf)-(int)inbuf_offset; inptr=inbuf+(int)inbuf_offset;}}

/*
 *  Seek to the block boundary of the block which includes abs_offset,
 *  then read block into input buffer and set pointers appropriately.
 *  If block is already in the buffer, just set the pointers.  This macro
 *  is used by process_end_central_dir (unzip.c) and do_string (file_io.c).
 *  A slightly modified version is embedded within extract_or_test_files
 *  (unzip.c).  ReadByte and readbuf (file_io.c) are compatible.
 *
 *  macro LSEEK(abs_offset)
 *      ulg abs_offset;
 *  {
 *      LONGINT   request = abs_offset #+ extra_bytes;
 *      LONGINT   inbuf_offset = request % INBUFSIZ;
 *      LONGINT   bufstart = request - inbuf_offset;
 *
 *      if (request < 0) {
 *          fprintf(stderr, SeekMsg, ReportMsg);
 *          return(3);             /-* 3:  severe error in zipfile *-/
 *      } else if (bufstart != cur_zipfile_bufstart) {
 *          cur_zipfile_bufstart = lseek(zipfd, (LONGINT)bufstart, SEEK_SET);
 *          if ((incnt = read(zipfd,inbuf,INBUFSIZ)) <= 0)
 *              return(51);        /-* 51:  unexpected EOF *-/
 *          inptr = inbuf + (int)inbuf_offset;
 *          incnt -= (int)inbuf_offset;
 *      } else {
 *          incnt += (inptr-inbuf) - (int)inbuf_offset;
 *          inptr = inbuf + (int)inbuf_offset;
 *      }
 *  }
 *
 */


#define SKIP_(length) if(length&&((error=do_string(length,SKIP,NULL))!=0))\
  {error_in_archive=error; if(error>1) return error;}

/*
 *  Skip a variable-length field, and report any errors.  Used in zipinfo.c
 *  and unzip.c in several functions.
 *
 *  macro SKIP_(length)
 *      ush length;
 *  {
 *      if (length && ((error = do_string(length, SKIP)) != 0)) {
 *          error_in_archive = error;   /-* might be warning *-/
 *          if (error > 1)              /-* fatal *-/
 *              return (error);
 *      }
 *  }
 *
 */


#ifdef FUNZIP
#  define FLUSH    flush
#  define NEXTBYTE getc(in)   /* redefined in crypt.h if full version */
#else
#  define FLUSH(w) /* if (mem_mode) outcnt=(w); else */ flush( cmp -> slide, (ulg)w, 0, cmp )
#  define NEXTBYTE (cmp -> csize-- <= 0 ? EOF : (--cmp -> in_count >= 0 ? /* (*((int*)cmp -> in_ptr))++ */ *cmp -> in_ptr++: readbyte(cmp)))
#endif


/*
 *  Remove all the ASCII carriage returns from buffer buf (length len),
 *  shortening as necessary (note that len gets modified in the process,
 *  so it CANNOT be an expression).  This macro is intended to be used
 *  *before* A_TO_N(); hence the check for CR instead of '\r'.  NOTE:  The
 *  if-test gets performed one time too many, but it doesn't matter.
 */
#define NUKE_CRs(buf,len) \
{ \
    register int i, j; \
    for (i = j = 0;  j < len;  (buf)[i++] = (buf)[j++]) \
        if ((buf)[j] == CR) \
            ++j; \
    len = i; \
}


/* GRR:  should change name to STRLOWER and use StringLower if possible */

/*
 *  Copy the zero-terminated string in str1 into str2, converting any
 *  uppercase letters to lowercase as we go.  str2 gets zero-terminated
 *  as well, of course.  str1 and str2 may be the same character array.
 */
#ifdef __human68k__
#  define TOLOWER(str1, str2) \
   { \
       char *p=(str1), *q=(str2); \
       uch c; \
       while ((c = *p++) != '\0') { \
           if (iskanji(c)) { \
               if (*p == '\0') \
                   break; \
               *q++ = c; \
               *q++ = *p++; \
           } else \
               *q++ = isupper(c) ? tolower(c) : c; \
       } \
       *q = '\0'; \
   }
#else
#  define TOLOWER(str1, str2) \
   { \
       char  *p, *q; \
       p = (str1) - 1; \
       q = (str2); \
       while (*++p) \
           *q++ = (char)(isupper((int)(*p))? tolower((int)(*p)) : *p); \
       *q = '\0'; \
   }
#endif
/*
 *  NOTES:  This macro makes no assumptions about the characteristics of
 *    the tolower() function or macro (beyond its existence), nor does it
 *    make assumptions about the structure of the character set (i.e., it
 *    should work on EBCDIC machines, too).  The fact that either or both
 *    of isupper() and tolower() may be macros has been taken into account;
 *    watch out for "side effects" (in the C sense) when modifying this
 *    macro.
 */


#ifndef native
#  define native(c)   (c)
#  define A_TO_N(str1)
#else
#  ifndef NATIVE
#    define NATIVE     "native chars"
#  endif
#  define A_TO_N(str1) {register unsigned char *p;\
     for (p=str1; *p; p++) *p=native(*p);}
#endif

/*
 *  Translate the zero-terminated string in str1 from ASCII to the native
 *  character set. The translation is performed in-place and uses the
 *  "native" macro to translate each character.
 *
 *  macro A_TO_N( str1 )
 *  {
 *      register unsigned char *p;
 *
 *      for (p = str1;  *p;  ++p)
 *          *p = native(*p);
 *  }
 *
 *  NOTE:  Using the "native" macro means that is it the only part of unzip
 *    which knows which translation table (if any) is actually in use to
 *    produce the native character set.  This makes adding new character set
 *    translation tables easy, insofar as all that is needed is an appropriate
 *    "native" macro definition and the translation table itself.  Currently,
 *    the only non-ASCII native character set implemented is EBCDIC, but this
 *    may not always be so.
 */

#endif /* !__unzip_h */
