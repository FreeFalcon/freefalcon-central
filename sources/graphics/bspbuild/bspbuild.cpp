/***************************************************************************\
    BSPbuild.cpp
    Scott Randolph
    January 29, 1998

    This is the tool which reads Multigen FLT files and writes out our 
	proprietary BSP file format.
\***************************************************************************/
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <SYS\STAT.H>
#include <stdlib.h>
#include <float.h>
#include "shi\ShiError.h"
#include "..\BSPutil\ParentBuildList.h"
#include "..\BSPutil\LODBuildList.h"
#include "..\BSPutil\ColorBuildList.h"
#include "..\BSPutil\PalBuildList.h"
#include "..\BSPutil\TexBuildList.h"
#include "grinline.h"
#include <Commdlg.h>
#include "PalBank.h"
#include "ObjectLod.h"
#include "PlayerOp.h"

// Time stamp from utility library
extern char FLTtoGeometryTimeStamp[];

#ifdef USE_SH_POOLS
extern MEM_POOL gBSPLibMemPool;
#endif
char g_CardDetails[1024];

// Unfortunatly, this is required by ShiError.h
int shiAssertsOn=1;
int shiWarningsOn = 1;
int shiHardCrashOn=0;
int NumHats;
PlayerOptionsClass PlayerOptions;

/*
 * Get a filename from the user
 */
BOOL GetNameFromUser( char *target, int targetSize )
{
	ShiAssert( target );
	ShiAssert( targetSize > 0 );
	ShiAssert( strlen(target) < (unsigned)targetSize );
#if 1
	OPENFILENAME dialogInfo;
	dialogInfo.lStructSize = sizeof( dialogInfo );
	dialogInfo.hwndOwner = NULL;
	dialogInfo.hInstance = NULL;
	dialogInfo.lpstrFilter = "ID List File\0*.Txt\0\0";
	dialogInfo.lpstrCustomFilter = NULL;
	dialogInfo.nMaxCustFilter = 0;
	dialogInfo.nFilterIndex = 1;
	dialogInfo.lpstrFile = target;
	dialogInfo.nMaxFile = targetSize;
	dialogInfo.lpstrFileTitle = NULL;
	dialogInfo.nMaxFileTitle = 0;
	dialogInfo.lpstrInitialDir = NULL;
	dialogInfo.lpstrTitle = "Select an ID list file";
	dialogInfo.Flags = OFN_FILEMUSTEXIST;
	dialogInfo.lpstrDefExt = "TXT";

	if ( !GetOpenFileName( &dialogInfo ) ) {
		return FALSE;
	}
#else
	printf("Input filename: ");
	fflush(stdout);
	fgets (target, targetSize, stdin);
#endif

	return TRUE;
}

static int sstr2arg (char *srcptr, int maxpf, char *argv[], const char *dlmstr)
{
    char gotquote;              /* currently parsing quoted string */
    register int ind;
    register char *destptr;
    char idex[256];
    const char *dp;
    
    if (srcptr == 0)
        return (-1);
    
    ZeroMemory (idex, sizeof idex);
    for (dp = dlmstr; *dp; dp++)
        idex[*(unsigned char *)dp] = 1;
    
    for (ind = 0, maxpf -= 2;; ind++) {
        if (ind >= maxpf) 
            return (-1);
	
        /* Skip leading white space */
        for (; *srcptr == ' ' || *srcptr == '\t'; srcptr++);
	
        argv [ind] = srcptr;
        destptr = srcptr;
	
	for (gotquote = 0; ; ) {
	    
            if (idex[*(unsigned char *)srcptr])
            {
                if (gotquote) { /* don't interpret the char */
                    *destptr++ = *srcptr++;
                    continue;
                }
		
                srcptr++;
                *destptr = '\0';
                goto nextarg;
            } else {
                switch (*srcptr) {
                default:        /* just copy it                     */
                    *destptr++ = *srcptr++;
                    break;
		    
                case '\"':      /* beginning or end of string       */
                    gotquote = (gotquote) ? 0 : 1 ;
                    srcptr++;   /* just toggle */
                    break;
		    
                case '\\':      /* quote next character     */
                    srcptr++;   /* skip the back-slash      */
                    switch (*srcptr) {
                        /* Octal character          */
                    case '0': case '1':
                    case '2': case '3': 
                    case '4': case '5':
                    case '6': case '7': 
                        *destptr = '\0';
                        do
			*destptr = (*destptr << 3) | (*srcptr++ - '0');
                        while (*srcptr >= '0' && *srcptr <= '7');
                        destptr++;
                        break;
                        /* C escape char            */
                    case 'b':
                        *destptr++ = '\b';
                        srcptr++;
                        break;
                    case 'n':
                        *destptr++ = '\n';
                        srcptr++;
                        break;
                    case 'r':
                        *destptr++ = '\r';
                        srcptr++;
                        break;
                    case 't':
                        *destptr++ = '\t';
                        srcptr++;
                        break;
			
                        /* Boring -- just copy ASIS */
                    default:
                        *destptr++ = *srcptr++;
                    }
                    break;
		    
		    case '\0':
			*destptr = '\0';
			ind++;
			argv[ind] = (char *) 0;
			return (ind);
                }
            }
        }
nextarg:
        continue;
    }
}

/*
 * Initialization, message loop
 */
int main(int argc, char *argv[])
{   
	char	basename[_MAX_PATH];
	char	filename[_MAX_PATH];
	char	drive[_MAX_DRIVE];
	char	path[_MAX_DIR];
	char	fname[_MAX_FNAME];
	char	ext[_MAX_EXT];
	char	msgbuf [1024];
	char linebuf[1024*4];
	char *preload = NULL;
	FILE	*IDSfile;
	int		result;
	int		id;
	int		file;
	UInt32	startTime;
	int lastid = 0;

	// Set the FPU to 24 bit precision
	_controlfp( _PC_24,   MCW_PC );
    _controlfp( _RC_CHOP, MCW_RC);
	#ifdef USE_SH_POOLS
	glMemPool = MemPoolInit( 0 );
	Palette::InitializeStorage ();
#endif
	// Display are startup banner
	printf( "BSPbuild compiled %s.  FLT reader %s\n", __TIMESTAMP__, FLTtoGeometryTimeStamp );
	while (argc > 1 && argv[1][0] == '-') {
	    switch (argv[1][1]) {
	    case '-': // end of args
		break;
	    case 'l':
		preload = argv[2];
		argc --;
		argv ++;
		break;
	    }
	    argv ++;
	    argc --;
	    break;
	}

	// See if we got a filename on the command line
	if ( argc == 2) {
		result = GetFullPathName( argv[1], sizeof( filename ), filename, NULL );
	} else {
		result = 0;
	}

	// If we didn't get it on the command line, ask the user
	if (result == 0) {
		strcpy( filename, "ids.txt" );
		if ( !GetNameFromUser( filename, sizeof(filename) )) {
			return -1;
		}
	}
	if (preload) {
	    printf ("Step 0: Preloading file %s\n", preload);
	    ObjectParent::SetupTable(preload);
	    //lastid = TheObjectListLength + 1;
//	    TheParentBuildList.SetStartPoint(lastid);
//	    TheParentBuildList.MergeEntries();
	    TheColorBuildList.MergeColors();
	    ColorBankClass::Cleanup();
	    ThePaletteBuildList.MergePalette();
	    ThePaletteBank.Cleanup();
	    ObjectLOD::CleanupTable();
	}


	// Mark our start time
	startTime = GetTickCount();


	// Open the object id input file
	printf("Step 1:  Reading %s.  (Time %0.0fmin)\n", filename, (GetTickCount()-startTime)/60000.0 );
	fprintf(stderr, "Step 1:  Reading %s.  (Time %0.0fmin)\n", filename, (GetTickCount()-startTime)/60000.0 );
	fflush(NULL);
	IDSfile = fopen( filename, "r" );
	if (!IDSfile) {
		sprintf( msgbuf, "Failed to open input file %s", filename );
		ShiError( msgbuf );
	}

	#ifdef USE_SH_POOLS
	if ( gBSPLibMemPool == NULL )
	{
		gBSPLibMemPool = MemPoolInit( 0 );
	}
	#endif

	// Read each line from the input ID file
	while (fgets (linebuf, sizeof linebuf, IDSfile) != NULL) {
	    char *av[100];
	    int ac;
	    char *cp;

	    if (linebuf[0] == '#' || linebuf[0] == '\n' || linebuf[0] == '\r')
		continue;
	    if (cp = strchr (linebuf, '\n'))
		*cp = '\0';

	    ac = sstr2arg(linebuf, 100, av, " \t,");
	    if (ac < 2) continue;
	    id = atoi(av[0]);
	    if (id == -1) id = lastid ++;
	    
	    TheParentBuildList.AddItem( id, av[1]);
	}

	// Close the id file
	fclose( IDSfile );

	
	// Now construct the object array and read each object
	printf("Step 2:  Reading FLT files.  (Time %0.0fmin)\n", (GetTickCount()-startTime)/60000.0 );
	fprintf(stderr, "Step 2:  Reading FLT files.  (Time %0.0fmin)\n", (GetTickCount()-startTime)/60000.0 );
	fflush(NULL);
	if ( !TheParentBuildList.BuildParentTable() ) {
		printf("ERROR:  We got no objects to process.  (Time %0.0fmin)\n", (GetTickCount()-startTime)/60000.0 );
		fprintf(stderr, "ERROR:  We got no objects to process.  (Time %0.0fmin)\n", (GetTickCount()-startTime)/60000.0 );
		fflush(NULL);
		exit( -1 );
	}


	// Write the data to disk in a packed format
	_splitpath( filename, drive, path, fname, ext );
	strcpy( basename, drive );
	strcat( basename, path );
	strcat( basename, "KoreaObj" );


	// Create the object LOD file
	printf("Step 3:  Writing object LODs.  (Time %0.0fmin)\n", (GetTickCount()-startTime)/60000.0 );
	fprintf(stderr, "Step 3:  Writing object LODs.  (Time %0.0fmin)\n", (GetTickCount()-startTime)/60000.0 );
	fflush(NULL);
	strcpy( filename, basename );
	strcat( filename, ".LOD" );
	printf("\nCreating %s\n", filename);
	file = open( filename,  _O_WRONLY | _O_BINARY | _O_TRUNC | _O_CREAT, _S_IWRITE );
	if (file < 0) {
		printf("Failed to create object LOD file %s\n", filename);
		exit(-1);
	}

	// Write the object LOD data file
	TheLODBuildList.WriteLODData( file );

	// Close the object LOD file
	close(file);


	// Create the object texture file
	printf("Step 4:  Writing object textures.  (Time %0.0fmin)\n", (GetTickCount()-startTime)/60000.0 );
	fprintf(stderr, "Step 4:  Writing object textures.  (Time %0.0fmin)\n", (GetTickCount()-startTime)/60000.0 );
	fflush(NULL);
	strcpy( filename, basename );
	strcat( filename, ".TEX" );
	printf("\nCreating %s\n", filename);
	file = open( filename,  _O_WRONLY | _O_BINARY | _O_TRUNC | _O_CREAT, _S_IWRITE );
	if (file < 0) {
		printf("Failed to create object texture file %s\n", filename);
		exit(-1);
	}

	// Write the object texture file
	TheTextureBuildList.WriteTextureData( file );

	// Close the object texture file
	close(file);


	// Free the LOD data now that its on disk
	TheParentBuildList.ReleasePhantomReferences();


	// Create the master object file
	printf("Step 5:  Writing object headers.  (Time %0.0fmin)\n", (GetTickCount()-startTime)/60000.0 );
	fprintf(stderr, "Step 4:  Writing object headers.  (Time %0.0fmin)\n", (GetTickCount()-startTime)/60000.0 );
	fflush(NULL);
	strcpy( filename, basename );
	strcat( filename, ".HDR" );
	printf("\nCreating %s\n", filename);
	file = open( filename, _O_WRONLY | _O_BINARY | _O_TRUNC | _O_CREAT | _O_SEQUENTIAL, _S_IWRITE );
	if (file < 0) {
		printf("Failed to create master file %s\n", filename);
		exit(-1);
	}

	// Write the object format version to the master file
	TheParentBuildList.WriteVersion( file );

	// Write the Color Table to the master file
	TheColorBuildList.WritePool( file );

	// Write the Palette Table to the master file
	ThePaletteBuildList.WritePool( file );

	// Write the Texture Table to the master file
	TheTextureBuildList.WritePool( file );

	// Write the object LOD headers
	TheLODBuildList.WriteLODHeaders( file );

	// Write the parent object records to the master file
	TheParentBuildList.WriteParentTable( file );

	// Close the master file
	close(file);


	printf("Step 6:  Reporting.  (Time %0.0fmin)\n", (GetTickCount()-startTime)/60000.0 );
	fprintf(stderr, "Step 5:  Reporting.  (Time %0.0fmin)\n", (GetTickCount()-startTime)/60000.0 );
	fflush(NULL);


	TheColorBuildList.Report();
	ThePaletteBuildList.Report();
	TheTextureBuildList.Report();


	printf("Finished.  (Time %0.0fmin)\n", (GetTickCount()-startTime)/60000.0 );
	fprintf(stderr, "Finished.  (Time %0.0fmin)\n", (GetTickCount()-startTime)/60000.0 );
	fprintf(stderr, "Press ENTER to end.\n");
	getchar();
	return 0;
}
