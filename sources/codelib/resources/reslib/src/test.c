/* -------------------------------------------------------------------------

    Test.cpp

    Console i/o interface to test resource manager

    Written by Kevin Ray    (c) 1996 Spectrum Holobyte          

   ------------------------------------------------------------------------ */

#include "resmgr.h"        /* exported prototypes & type definitions         */
#include "memmgr.h"

#include <stdio.h>         /* low-level file i/o (+io.h)                     */
#include <string.h>
#include <memory.h>
#include <sys/stat.h>      /* _S_IWRITE                                      */

#if USE_WINDOWS
#include <io.h>
#include <direct.h>
#include <process.h>       /* _beginthread()    MUST SET C++ OPTIONS UNDER 
                                                MSVC SETTINGS                */

#include <windows.h>       /* all this for MessageBox (may move to debug.cpp)*/
#endif

#include "unzip.h"

#define MAX_ARGS                25
#define MAX_COMMAND_LINE        255



#if( !RES_REPLACE_FTELL )
#   define FTELL(a)              ResFTell(a)
#else
#   define FTELL(a)              ftell(a)
#endif

#define HI_WORD(a)               ((a)>>16)
#define LO_WORD(a)               ((a)&0x0ffff)

#if( RES_DEBUG_VERSION )
#   define IF_DEBUG(a)           a
#else
#   define IF_DEBUG(a)
#endif

#if( RES_STANDALONE )
#   include <conio.h>      /* cgets                                          */
#   define GETS _cgets
#else
#   define GETS gets
#endif /*RES_STANDALONE */


#if( RES_STANDALONE )
char * command[] =
{
    "dir",
    "exit",
    "attach",
    "detach",
    "analyze",
    "dump",
    "find",
    "cd",
    "add",
    "read",
    "path",
    "map",
    "stream",
    "extract",
    "run",
    "help"
};

char * help[] =
{
    "Prints the contents of a directory that is in the search path.",
    "Exit the program.", 
    "Attach an archive file to the search path.",
    "Detach an archive file that has already been added to the\n\t\tsearch path",
    "Display an analysis of the hash table for the specified\n\t\tdirectory [blank pathname = GLOBAL_HASH_TABLE].",
    "Display a complete analysis of all hash tables and hash\n\t\tentries.",
    "Find an entry within the Resource Manager and give\n\t\tspecifics.",
    "Change the current directory path to a path already defined\n\t\twithin the search path (already added).",
    "Add a directory to the search path.",
    "Spawns an asynchronous read of specified file.  When finished\n\t\tnotification will appear.",
    "Display all of the directories in the search path.",
    "Display all of the devices on host computer.",
    "Read from a file using stdio streaming functions.",
    "Extract a file from an archive to c:\\.",
    "Execute a custom function.",
    "Any command followed by '?' will display an explanation of\n\t\tthat command."
};

char * syntax[] =
{
    "dir <directory name>",
    "exit",
    "attach [<attach point>] <zip filename> [<true | false>]",
    "detach",
    "analyze <pathname>",
    "dump",
    "find <filename>",
    "cd <pathname>",
    "add <pathname> [<true | false>]",
    "read <filename>",
    "path",
    "map <volume id ('A', 'B', etc)>",
    "stream <filename>",
    "extract <dst filename> <src filename>",
    "run <any number of parameters>",
    "help"
};

enum COMMAND_CODES
{
    COMMAND_ERROR = -1,
    COMMAND_DIR,
    COMMAND_EXIT,
    COMMAND_ATTACH,
    COMMAND_DETACH,
    COMMAND_ANALYZE,
    COMMAND_DUMP,
    COMMAND_FIND,
    COMMAND_CD,
    COMMAND_ADD,
    COMMAND_READ,
    COMMAND_PATH,
    COMMAND_MAP,
    COMMAND_STREAM,
    COMMAND_EXTRACT,
    COMMAND_RUN,
    COMMAND_HELP
};

#define COMMAND_COUNT   (sizeof(command)/sizeof(command[0]))

extern HASH_TABLE * GLOBAL_HASH_TABLE;                      /* root hash table                      */
extern LIST *       GLOBAL_PATH_LIST;                       /* search path list                     */
extern char         GLOBAL_CURRENT_PATH[];                  /* current working directory            */
extern char *       GLOBAL_SEARCH_PATH[ MAX_DIRECTORIES ];  /* directories in fixed order           */
extern int          GLOBAL_SEARCH_INDEX;                    /* number of entries in search path     */

extern DEVICE_ENTRY * RES_DEVICES;                          /* array of device_entry structs        */

extern HASH_ENTRY * hash_find( const char *, HASH_TABLE * );

extern char * res_fullpath( char * abs_buffer, const char * rel_buffer, int maxlen );
extern void dbg_analyze_hash( HASH_TABLE * );
extern void dbg_device( DEVICE_ENTRY * ); 



/* =======================================================

   FUNCTION:   parse_args

   PURPOSE:    Parse a command line entry into its
               argc and argv components.

   PARAMETERS: Command line, char * array for argv[].

   RETURNS:    Count of arguments (argc).

   ======================================================= */

int parse_args( char * cmd, char ** argv )
{
    int quote_flag,
        writing_flag,
        argc;

    quote_flag = FALSE;
    writing_flag = FALSE;
    argc = 0;

    while( *cmd ) {

        if( quote_flag ) {
            if( *cmd == ASCII_QUOTE ) {
                quote_flag = 0;
                *cmd = 0x00;
            }
        } else {
            if( *cmd == ASCII_SPACE ) {
                *cmd = 0x00;
                writing_flag = 0;
            } else {
                if( !writing_flag ) {
                    if( *cmd == ASCII_QUOTE ) {
                        argv[ argc++ ] = ++cmd;
                        quote_flag = 1;
                    }
                    else
                        argv[ argc++ ] = cmd;
                    writing_flag = 1;
                }
            }
        }

        cmd++;
    }

    return( argc );
}


void print_bytes( char * buffer )
{
    int count;

    for( count = 0; count < 16; count++ )
        printf( "%02X ", (unsigned char)(buffer[ count ]));
    printf( "  " );

    for( count = 0; count < 16; count++ )
        if( (buffer[ count ] > 0x1f) && (buffer[ count ] < 0x7f))
            printf( "%c", (unsigned char)(buffer[ count ]));
        else
            printf( "." );
}

void print_heading( void )
{
    printf( "\n\n" );
    printf( "+-------------------------------------------------------------------------+\n" );
    printf( "|  COMMANDS :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::   |\n" );
    printf( "|                                                                         |\n" );
    printf( "|   dir  exit  attach  detach  analyze  dump  find  cd  add  read  path   |\n" );
    printf( "|   map  stream  extract  run  help                                       |\n" );
    printf( "+-------------------------------------------------------------------------+\n" );
}

void show_help( int cmd )
{
    printf( " syntax      :  %s\n", syntax[ cmd ] );
    printf( " description :  %s\n", help[ cmd ] );
}

int is_bool( char * string )
{
    if( !stricmp( string, "false" )) return( 0 );
    if( !stricmp( string, "true"  )) return( 1 );

    if( strlen( string ) == 1 ) {
        if( strchr( "nNfF0", *string )) return(0);
        if( strchr( "yYTt1", *string )) return(1);
    }        

    return( -1 ); /* can't determine */
}


/* =======================================================

   FUNCTION:   cmd_read   "read"

   PURPOSE:    Reads a file and displays it to stdout.

   PARAMETERS: Source file name.

   RETURNS:    None.

   ======================================================= */

void cmd_read( char * argv )
{
    unsigned int size;

    char * inbuf,
         * ptr;

    inbuf = ptr = ResLoadFile( argv, NULL, &size );

    if( !inbuf ) {
        printf( "File not found (%s).\n", argv );
        return;
    }

    fwrite( inbuf, 1, size, stdout );

    ResUnloadFile( inbuf );
}




/* =======================================================

   FUNCTION:   cmd_map  "map"

   PURPOSE:    Checks a device to see if the media has
               changed.

   PARAMETERS: Logical drive id ('A', 'B', etc ).

   RETURNS:    None.

   ======================================================= */


void cmd_map( char * argv )
{
    int id,
        check;

    if( !argv ) {
        printf( "No device id specified.\n" );
        return;
    }

    id = toupper(toupper(argv[0])) - 'A';

    if( id < 0 || id > 25 ) {
        printf( "Can't decipher drive.  Use A-Z\n" );
        return;
    }
    else {    
        check = ResCheckMedia( id );

        switch( check ) {
            case 0: 
#if( RES_DEBUG_VERSION )
               dbg_device( &RES_DEVICES[ id ]); 
#endif
               break;
            case 1: printf( "Media has not changed.\n" ); break;
            case -1: printf( "Media not available.\n" ); break;
        }                       
    }
}


/* =======================================================

   FUNCTION:   cmd_dir  "dir"

   PURPOSE:    Display a familiar looking directory
               listing.

   PARAMETERS: Path to list (optional)

   RETURNS:    None.

   ======================================================= */

void cmd_dir( char * argv )
{
    RES_DIR * dir;
    char * file;
    char   fullpath[_MAX_PATH];
    int    ct = 0;

    if( !GLOBAL_SEARCH_INDEX ) {
        printf( "No path.\n" );
        return;
    }

    if( !argv )
        ResGetDirectory( fullpath );
    else
        res_fullpath( fullpath, argv, _MAX_PATH );
    
    dir = ResOpenDirectory( fullpath );

    if( !dir )
        printf( "Directory not found.\n" );
    else {
        printf( "Directory of %s\n", fullpath );

        file = ResReadDirectory( dir );

        for( ct = 0; ct < dir -> num_entries; ct++ ) {
            printf( "%-17s", file );
    
            if( !((ct+1) % 4 ))
                printf("\n");

            file = ResReadDirectory( dir );
        }
    }

    ResCloseDirectory( dir );
}



/* =======================================================

   FUNCTION:   cmd_find  "find"

   PURPOSE:    Find a file and display verbose statistics
               gathered about that file.

   PARAMETERS: Filename.

   RETURNS:    None.

   ======================================================= */

void cmd_find( char * fullpath )
{
    RES_STAT stat;
    DEVICE_ENTRY dev;
    int  flag, count;
    char path[_MAX_PATH],
         arcname[_MAX_PATH];

    if( ResStatusFile( fullpath, &stat )) {
        ResGetPath( stat.directory, path );
        ResGetArchive( stat.archive, arcname );

        printf( "\n----------------\n" );
        printf( "File Information\n" );
        printf( "----------------\n" );
        printf( "name      : %s\n", fullpath );
        printf( "size      : %d\n", stat.size );
        printf( "csize     : %d\n", stat.csize );
        printf( "directory : %s\n", path, stat.directory );
        printf( "volume    : %c\n", stat.volume + 'A' );
        if( *arcname )
            printf( "archive   : %s\n", arcname );
        else
            printf( "archive   : NOT IN AN ARCHIVE\n" );
        printf( "attributes: %0x ", stat.attributes );

        if( stat.attributes & _A_NORMAL ) printf( " NORMAL BIT," );
        if( stat.attributes & _A_RDONLY ) printf( " READ ONLY BIT," );
        if( stat.attributes & _A_HIDDEN ) printf( " HIDDEN BIT," );
        if( stat.attributes & _A_SYSTEM ) printf( " SYSTEM BIT," );
        if( stat.attributes & _A_SUBDIR ) printf( " DIRECTORY BIT," );
        if( stat.attributes & _A_ARCH   ) printf( " ARCHIVE BIT " );
        printf( "\n" );

        flag = ResWhereIs( fullpath, NULL );

        printf( "\n------------------\n" );
        printf( "Volume information\n" );
        printf( "------------------\n" );
        if( flag != -1 ) {
            printf( "media     : " );
            if( flag & RES_HD      ) printf( "HARD DRIVE, " );
            if( flag & RES_CD      ) printf( "CD-ROM, " );
            if( flag & RES_NET     ) printf( "NETWORK, " );
            if( flag & RES_ARCHIVE ) printf( "ARCHIVE FILE, " );
            if( flag & RES_FLOPPY  ) printf( "REMOVEABLE MEDIA " );
            printf( "\n" );
        }

        flag = ResDevice( stat.volume, &dev );

        if( flag ) {
            printf( "name      : %s\n", dev.name );
            printf( "serial    : %x-%x\n", HI_WORD(dev.serial), LO_WORD(dev.serial));
        }

        printf( "\n------------\n" );
        printf( "Header bytes\n" );
        printf( "------------\n" );

        flag = ResOpenFile( fullpath, _O_RDONLY | _O_BINARY );
        if( flag != -1 ) {
            ResReadFile( flag, path, _MAX_PATH );

            for( count = 0; count < 5; count++ ) {
                printf( "          : " );
                print_bytes( &path[16*count] );
                printf( "\n" );
            }
 
            ResCloseFile( flag );
        };
    }
    else
        printf( "File not found.\n" );
}



/* ================================================

   FUNCTION:   cmd_run   "run"

   PURPOSE:    Use to test code.

   PARAMETERS: argc, argv[]

   RETURNS:    None.

   NOTE:   If you want to test your own throw-away,
           do so here.

           Within the the console app, if you 
           type 'run <params ...>', you'll execute 
           this function with the familiar arguments 
           from main().

   ================================================ */ 

void cmd_run( int argc, char ** argv )
{
    FILE * fp;
	FILE *fp1;

    char buffer[1024];
    //  ALL OF THE CODE WITHIN THIS FUNCTION CAN BE
    //  (AND SHOULD BE) DELETED AND REPLACED WITH
    //  WHATEVER THROW-AWAY CODE YOU WANT TO TEST!!
 
    /* here's a simple example */

    /* --- Show arguments --- */
    int i;
	int ret;

    for( i=0; i<argc; i++ )
        printf( "%02d: %s\n", i, argv[i] );


    /* here's a more likely example */



    /* --- Test fseek, ftell --- */

    if( argc < 3 ) {
        printf( "useage: RUN <file> <mode>\n" );
        return;
    }

    fp = fopen( argv[1], argv[2] );

    if( !fp ) {
        printf( "Could not open %s\n", argv[1] );
        return;
    }

    fread( buffer, 1, 1, fp );
    printf( "buffer[0] = %d\n", buffer[0] );
    
    fseek( fp, 64, SEEK_SET );
    fread( buffer, 1, 1, fp );
    printf( "buffer[64] = %d\n", buffer[0] );

    fseek( fp, 512, SEEK_SET );
    fread( buffer, 18, 1, fp );
    printf( "buffer[512] = %s\n", buffer );

    printf( "pos = %d\n", RES_FTELL(fp));

    fseek( fp, -15, SEEK_END );
    fread( buffer, 1, 1, fp );
    printf( "buffer[1009] = %d\n", buffer[0] );

    printf( "pos = %d\n", RES_FTELL(fp));

    fseek( fp, -1024, SEEK_END );
    fread( buffer, 1, 1, fp );
    printf( "buffer[0] = %d\n", buffer[0] );

    printf( "pos = %d\n", RES_FTELL(fp));

    fseek( fp, -896, SEEK_END );
    fread( buffer, 1, 1, fp );
    printf( "buffer[128] = %d\n", buffer[0] );

    printf( "pos = %d\n", RES_FTELL(fp));

    fseek( fp, 0, SEEK_SET );
    fscanf( fp, "%s", buffer );
    printf( "fscanf = %s\n", buffer );

    printf( "size = %d\n", fseek( fp, 0, SEEK_END ));



	if(argc > 3 )
	{
	/* open second file */
    fp1 = fopen( argv[3], argv[2] );

    if( !fp1 ) {
        printf( "Could not open %s %s\n", argv[3],argv[2] );
        return;
    }

    fread( buffer, 1, 1, fp1 );
    printf( "buffer[0] = %d\n", buffer[0] );
    

    fseek( fp1, 512, SEEK_SET );
    fread( buffer, 18, 1, fp1 );
    printf( "buffer[512] = %s\n", buffer );

    printf( "pos = %d\n", RES_FTELL(fp));

    fseek( fp1, -15, SEEK_END );
    fread( buffer, 1, 1, fp1 );
    printf( "buffer[1009] = %d\n", buffer[0] );

    printf( "pos = %d\n", RES_FTELL(fp));

    fseek( fp1, -1024, SEEK_END );
    fread( buffer, 1, 1, fp1 );
    printf( "buffer[0] = %d\n", buffer[0] );

    printf( "pos = %d\n", RES_FTELL(fp));


    fseek( fp1, 0, SEEK_SET );
    fscanf( fp1, "%s", buffer );
    printf( "fscanf = %s\n", buffer );

    printf( "size = %d\n", fseek( fp, 0, SEEK_END ));
	fclose(fp1);
	}



	/* open third file */
	if(argc > 4)
	{
    fp1 = fopen( argv[4], argv[2] );

    if( !fp1 ) {
        printf( "Could not open %s\n", argv[4] );
        return;
    }

    fread( buffer, 1, 1, fp1 );
    printf( "buffer[0] = %d\n", buffer[0] );
    

    fseek( fp1, 512, SEEK_SET );
    fread( buffer, 18, 1, fp1 );
    printf( "buffer[512] = %s\n", buffer );

    printf( "pos = %d\n", RES_FTELL(fp));

    fseek( fp1, -15, SEEK_END );
    fread( buffer, 1, 1, fp1 );
    printf( "buffer[1009] = %d\n", buffer[0] );

    printf( "pos = %d\n", RES_FTELL(fp));

    fseek( fp1, -1024, SEEK_END );
    fread( buffer, 1, 1, fp1 );
    printf( "buffer[0] = %d\n", buffer[0] );

    printf( "pos = %d\n", RES_FTELL(fp));


    fseek( fp1, 0, SEEK_SET );
    fscanf( fp1, "%s", buffer );
    printf( "fscanf = %s\n", buffer );

    printf( "size = %d\n", fseek( fp, 0, SEEK_END ));
	fclose(fp1);

	}

    ret = fseek( fp, 0, SEEK_SET );
    printf( "fseek: ret = %d\n", ret );
    fscanf( fp, "%s", buffer );
    printf( "fscanf = %s\n", buffer );

    printf( "size = %d\n", fseek( fp, 0, SEEK_END ));



    fclose(fp);
}


/* ================================================

                    M  A  I  N

   ================================================ */

int main( int argc, char ** argv )
{
    char path[_MAX_PATH];

    char buffer[ MAX_COMMAND_LINE + 2 ] = { (char)MAX_COMMAND_LINE };  /* Used with _cgets() - maximum number of characters in must be set in 1st byte */

    int    _argc;
    char * _argv[ MAX_ARGS ],
         * result,
         * fullpath;

    int archive_handle = -1;         /* last attached archive */

    int i, cmd;

    if( !ResInit( NULL )) 
        return( 1 );            /* ResInit failed */

    
    /* these would be called after parsing an .ini file or similar */

    IF_DEBUG( ResDbgLogOpen( "resource.log" ));

    print_heading();

    _getcwd( path, _MAX_PATH );
    printf( "PATH: %s\n", path );

    do {
        do {
#if( !RES_USE_FLAT_MODEL )
            printf( "\n%s> ", GLOBAL_CURRENT_PATH );
#else
            printf( "\nRoot > " );
#endif
    _getcwd( path, _MAX_PATH );
    printf( "[SD: %s]>", path );

            result = GETS( buffer );  /* Input a line of text */
            
        } while( !buffer[1] );


        if( !stricmp( result, "exit" ))
            break;

        _argc = parse_args( result, _argv );

        cmd = COMMAND_ERROR;

        for( i=0; i<COMMAND_COUNT; i++ ) {
            if( !stricmp( result, command[i] )) {
                cmd = i;
                break;
            }
        }

        if( cmd == COMMAND_EXIT && !_argc )
            break;

        if( _argc > 1 ) {
            if( !stricmp( _argv[1], "?" ) || !stricmp( _argv[1], "help" )) {
                show_help( cmd );
                continue;
            }
            else
                if( strchr( _argv[1], ASCII_BACKSLASH )) {
                    res_fullpath( path, _argv[1], _MAX_PATH );
                    fullpath = path;
                }
                else
                    fullpath = _argv[1];
        }

        switch( cmd ) 
        {
            case COMMAND_DIR:
#if( RES_USE_FLAT_MODEL )
                printf( "This function is only meaningful when using the hierarchical model.\n" );
                break;
#endif
                if( _argc > 1 )
                    cmd_dir( _argv[1] );
                else
                    cmd_dir( NULL );
                break;


            case COMMAND_ANALYZE:
            {
#if( !RES_USE_FLAT_MODEL )
                HASH_ENTRY * entry;

                if( _argc == 1 )
                    entry = hash_find( GLOBAL_CURRENT_PATH, GLOBAL_HASH_TABLE );
                else
                    entry = hash_find( fullpath, GLOBAL_HASH_TABLE );

                if( entry ) {
#if( RES_DEBUG_VERSION )
                    if( entry -> dir )
                        dbg_analyze_hash( (HASH_TABLE *)entry -> dir );
                    else
                        printf( "No directory table for this directory.\n" );
#endif /* RES_DEBUG_VERSION */
                }
                else
                    printf( "Directory not found.\n" );
#else
                printf( "This command only meaningful when using the hierarchical model!\n" );
#endif
                break;
            }

            case COMMAND_RUN:
                cmd_run( _argc, _argv );
                break;

            case COMMAND_CD:
                if( _argc > 1 ) {
                    if( !ResSetDirectory( fullpath ))
                        printf( "Error changing to directory %s\n", fullpath );
                }
                else
                    printf( "Current directory is: %s\n", GLOBAL_CURRENT_PATH );

                break;


            case COMMAND_ADD:
                if( _argc > 1 ) {
                    int test = FALSE,
                        flag = -1;

                    if( _argc > 2 )
                        flag = is_bool( _argv[2] );
                    
                    if( flag == -1 )
                        flag = TRUE;

                    if( !GLOBAL_SEARCH_INDEX )
                        test = ResCreatePath( fullpath, flag );
                    else
                        test = ResAddPath( fullpath, flag );

                    if( !test )
                        printf( "Error adding %s to search path\n", fullpath );
                }
                else
                    show_help(cmd);
                break;


            case COMMAND_STREAM:
            {
                FILE * fptr;
                char   c;
                int    test;

                if( _argc < 2 ) {
                    show_help(cmd);
                    break;
                }

                fptr = fopen( _argv[1], "r" );

                if( fptr ) {
                    while( (test = fscanf( fptr, "%c", &c )) != EOF )
                        printf( "%c", c );

                    printf( "\n\n\n\n ************** REWINDING ****************** \n\n\n\n\n\n\n" );

                    fseek( fptr, 0, SEEK_SET );

                    while( (test = fscanf( fptr, "%c", &c )) != EOF )
                        printf( "%c", c );

                    fclose( fptr );

                } else {
                    printf( "Error opening file %s\n", _argv[1] );
                }

                break;
            }


            case COMMAND_PATH:
            {
                int x=0;
                char b[_MAX_PATH];

                if( GLOBAL_PATH_LIST ) {

                    while( ResGetPath( x++, b ))
                        printf( "%s\n", b );
                }
                else
                    printf( "No path created.\n" );

                break;
            }

            case COMMAND_EXTRACT:
            {
                /* extracts the archive to the local directory with the same filename */

                if( _argc < 3 )
                    show_help(cmd);
                else              
                    ResExtractFile( _argv[1], _argv[2] );
                break;
            }            

            case COMMAND_READ:
                if( _argc >= 2 )
                    cmd_read( _argv[1] );
                else
                    show_help(cmd);
                break;


            case COMMAND_ATTACH:
            {
                char dst[_MAX_PATH];
                int  flag = -1;

                if( _argc < 2 ) {
                    show_help(cmd);
                    break;
                }

                if( _argc >= 3 )
                    flag = is_bool( _argv[ _argc - 1 ] );

                if( _argc >= 3 ) res_fullpath( dst, _argv[2], _MAX_PATH );

                if( _argc == 2 )
                    archive_handle = ResAttach( GLOBAL_CURRENT_PATH, fullpath, FALSE );
                else
                    if( _argc == 3 ) {
                        if( flag != -1 )
                            archive_handle = ResAttach( GLOBAL_CURRENT_PATH, fullpath, flag );
                        else
                            archive_handle = ResAttach( fullpath, dst, FALSE );
                    } else
                        if( _argc == 4 )
                            archive_handle = ResAttach( fullpath, dst, flag == -1 ? 0 : flag );
                
                if( archive_handle == -1 )
                    printf( "Error attaching zip file %s\n", fullpath );

                break;
            }

            case COMMAND_DUMP:
                ResDbgDump();
                MemDump();          printf("\n");
                MemFindLevels();    printf("\n");
                MemFindUsage();
                break;

            case COMMAND_DETACH:
                if( archive_handle != -1 )
                {
                    ResDetach( archive_handle );
                    archive_handle = -1;
                }
                else
                    printf( "No archives currently attached.\n" );
                break;

            case COMMAND_MAP:
                if( _argc < 2 )
                    show_help(cmd);
                else
                    cmd_map( _argv[1] );
                break;

            case COMMAND_FIND:
                if( _argc < 2 )
                    show_help(cmd);
                else
                    cmd_find( fullpath );
                break;

            case COMMAND_HELP:
                print_heading();
                show_help(cmd);
                break;

            case COMMAND_ERROR:
            default:
                printf( "Syntax error\n" );
                break;
        }

    } while( TRUE );
    
    ResExit();
    MemDump();
    _getcwd( path, _MAX_PATH );
    printf( "PATH: %s\n", path );

    return(0);
}
#endif /* RES_STANDALONE    */
