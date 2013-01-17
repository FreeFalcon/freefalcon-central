/*
   MOVIE.C

   Exported functions:

      movieInit
      movieUnInit
      movieOpen
      movieClose
      movieStart
      movieStop
      movieIsPlaying
      movieCount
      movieGetLastError

   Programmed by Kuswara Pranawahadi               September 5, 1996
*/

#include <windows.h>
#include "fsound.h" //psound
#include "avimovie.h"
#include <process.h>
#include "debuggr.h"
#define  HICOLOR        16
#define  MAX_MOVIES     4
#define  DSB_SIZE       16384
#define  AUDIO_TIMEOUT  5000
#define PF						MonoPrint


#undef MEASURE_TIME
#ifdef MEASURE_TIME

	#include <sys/timeb.h>
	#include <time.h>

	struct _timeb	last;
	struct _timeb	now;
	BOOL			once;	
	double			average;
	double			total;
	long			count;

#endif


/* PRESPIN will fetch an arbitrarily small amount of data from the file.  The size is
   roughly the size of the internal cache of a cd drive or hd. */
#define PRESPIN_CD		1
#define CD_CACHE_SIZE	2048


/*
   Structure to support non-standard RGB mode.
*/
typedef struct tagBITMAPINFOHEADEREX
{
   BITMAPINFOHEADER  bmiHeader;
   DWORD             redMask;
   DWORD             greenMask;
   DWORD             blueMask;
} BITMAPINFOHEADEREX, *PBITMAPINFOHEADEREX;

typedef struct tagMOVIE
{
   HIC                  hIC;              // ICM handle
   LPVOID               ddSurface;        // surface
   int                  startX;           // x-coordinate for upper left
   int                  startY;           // y-coordinate for upper right
   BITMAPINFOHEADEREX   bihOut;           // output bitmap format
   LPVOID               surfaceBuffer;    // uncompressed data
   int                  videoMode;        // mode to display video
   int                  pixelSize;        // pixel size in bytes
   long                 totalFrames;      // total frames processed
   long                 dropFrames;       // total frames dropped
   int                  handle;           // movie handle
   int                  callBackID;       // ID to be used in callback
   int                  status;           // movie status
   int                  sbType;           // surface buffer type
   unsigned long        startTime;        // movie start time

   unsigned long        hMovieThread;     // handle to movie thread
   DWORD                movieThreadID;    // movie thread ID

   unsigned long        hFillerThread;    // handle to filler thread
   DWORD                fillerThreadID;   // filler thread ID

   void                 ( *callBack )( int, LPVOID, int, int,
                                       int, SURFACEACCESS * );
                                          // called issued every movie
                                          // frame
   AVISTREAMS           aviStreams;       // video and audio streams
//   AUDIO_CHANNEL        *audioChannel;    // audio channel
   int			        audioChannel;    // Stream ID
   int                  audioHandle;      // audio handle
   int                  lastError;        // last error code
   SURFACEACCESS        sa;
} MOVIE, *PMOVIE;

static unsigned __stdcall movieThread( void * );
static unsigned __stdcall fillerThread( void * );
/*
extern   unsigned int __stdcall movieThread( PMOVIE item );
extern   unsigned int __stdcall fillerThread( PMOVIE item );
*/
static   void doFrame ( PMOVIE item );
extern   void doFrame ( PMOVIE item );
//extern   int  fillSoundBuffer( int item_handle, void *soundBuffer,
//                                int length );

// make to work with psound
static DWORD fillSoundBuffer(void *,char *,DWORD);
/*
static DWORD fillSoundBuffer( void *me, void *soundBuffer, DWORD length );
*/

static   LPVOID            ddPointer;
static   PMOVIE            movie;
static   DWORD             numOfMovies = 0, abortMovie = 1;
static   CRITICAL_SECTION  movieCriticalSection;

/****************************************************************************

   doFrame

   Purpose:    To copy uncompressed movie bitmap to a surface.

   Arguments:  Movie item.

   Returns:    None.

****************************************************************************/



static void doFrame( PMOVIE item )
{
   char           *tempPtr, *buffer;
   int            padding, width, height;
   int            i;
   SURFACEACCESS  *sa;
   AVISTREAMS     *streams;

#ifdef MEASURE_TIME

	double diff;

	if(!once) {

		_ftime(&now);
		once = TRUE;
	}
	else {

		last.time		= now.time;
		last.millitm	= now.millitm;

		_ftime(&now);

		diff			= (now.time + now.millitm / 1000.0) - (last.time + last.millitm / 1000.0);
		total			+= diff;
		count++;
	}

#endif

   streams = &( item->aviStreams );

   if ( item->sbType & SURFACE_TYPE_SYSTEM )
   {
      width = item->aviStreams.bihIn.biWidth *
                         item->pixelSize;          // width in bytes
      height = item->aviStreams.bihIn.biHeight;
      buffer = ( char * ) item->surfaceBuffer;     // bitmap to render

      sa = &( item->sa );

      /*
         Lock surface.
      */
      if (  ( sa->lockStatus == SURFACE_IS_UNLOCKED ) &&
            !( item->sbType & SURFACE_TRY_FAST ) )
      {
         surfaceGetPointer( item->ddSurface, sa );
         if ( sa->lockStatus == SURFACE_IS_UNLOCKED )
            return;
      }

      tempPtr = ( char * ) sa->surfacePtr;         // get pointer to the surface
      tempPtr += item->startY * sa->lPitch;        // get startY address
      tempPtr += item->startX * item->pixelSize;   // get upper left address
      padding = sa->lPitch -  width;               // offset to next scan line

      switch ( item->videoMode )
      {
         case MOVIE_MODE_V_DOUBLE:
            for ( i = 0; i < height; i++ )
            {
               memcpy( tempPtr, buffer, width );
               tempPtr += sa->lPitch;

               memcpy( tempPtr, buffer, width );
               tempPtr += sa->lPitch;
               buffer += width;
            }
            break;


         case MOVIE_MODE_INTERLACE:
            for ( i = 0; i < height; i++ )
            {
               memcpy( tempPtr, buffer, width );
               tempPtr += sa->lPitch + sa->lPitch;
               buffer += width;
            }
            break;

         default:
            if ( padding )
               for ( i = 0; i < height; i++ )
               {
                  memcpy( tempPtr, buffer, width );
                  tempPtr += sa->lPitch;
                  buffer += width;
               }
            else
               memcpy( tempPtr, buffer, width * height );
            break;
      }

      if ( sa->lockStatus == SURFACE_IS_LOCKED )
         surfaceReleasePointer( item->ddSurface, sa );
   }
   else
   {
      surfaceBlit( item->ddSurface, item->startX, item->startY,
            item->surfaceBuffer, item->aviStreams.bihIn.biWidth,
            item->aviStreams.bihIn.biHeight,
            ( item->videoMode == MOVIE_MODE_NORMAL ) ?
               BLIT_MODE_NORMAL : BLIT_MODE_DOUBLE_V );
   }
}

/****************************************************************************

   movieInit

   Purpose:    To initialize/allocate movie items.

   Arguments:  Number of movies to play simultaneously.
               NULL or DirectDraw object:
                  NULL: uses CPU to blit.
                  DirectDraw object: uses DirectDraw Blit.

   Returns:    MOVIE_OK if successful.

   Note:       DO NOT PLAY MORE THAN ONE MOVIE OFF A CD-ROM.

****************************************************************************/

int movieInit( int numMovies, LPVOID lpDD )
{


   if ( numOfMovies )
      return MOVIE_HAS_BEEN_INITIALIZED;

   if ( !numMovies || numMovies > MAX_MOVIES )
      return MOVIE_INVALID_NUMBER;

//   move = AVI_MALLOC( numMovies * sizeof( MOVIE ) );
   movie = new MOVIE[numMovies];
   if ( !movie )
      return MOVIE_MALLOC_FAILED;

   abortMovie = 0;
   memset( movie, 0, numMovies * sizeof( MOVIE ) );
   numOfMovies = numMovies;

   InitializeCriticalSection( &movieCriticalSection );

   ddPointer = lpDD;

   return MOVIE_OK;
}

/****************************************************************************

   movieUnInit

   Purpose:    To stop currently playing movies and to free up buffers
               allocated for all movies.

   Arguments:  None.

   Returns:    None.

****************************************************************************/

void movieUnInit( void )
{
   DWORD    i;

   if ( !numOfMovies )
      return;

   EnterCriticalSection( &movieCriticalSection );
   abortMovie++;
   LeaveCriticalSection( &movieCriticalSection );

   for ( i = 0; i < numOfMovies; i++ )
      if ( movie[i].status & ( MOVIE_STATUS_IN_USE | MOVIE_STATUS_PLAYING ) )
         movieClose( i );

   delete [] movie;
   ddPointer = NULL;
   numOfMovies = 0;
   DeleteCriticalSection( &movieCriticalSection );
}

/****************************************************************************

   movieOpen

   Purpose:    To prepare a movie to play.

   Arguments:  Movie file name ( AVI File ).
               Audio file name ( NULL if it does not use external audio ).
               Pointer to surface object.
               CallBack identification.
               CallBack function to be issued every movie frame.
               x-coordinate of movie upper left corner.
               y-coordinate of movie upper left corner.
               Mode used to render movie ( interlace, normal, etc. ).
               Audio flag.

   Returns:    Handle ( positive number ) if succesful.

****************************************************************************/

int movieOpen( char *aviFileName, char *audioFileName,
               LPVOID ddSurface,
               int callBackID,
               void ( *callBack )( int handle,
                                 LPVOID ddSurface,
                                 int frameNumber, int callBackID,
                                 int dropFlag, SURFACEACCESS * ),
               int startX, int startY, int videoMode, int audioFlag )
{
   DWORD                handle;
   int                  status;
   PMOVIE               item;
   SURFACEDESCRIPTION   sd;
   int                  screenWidth, screenHeight;
   int                  dibWidth, dibHeight;

   EnterCriticalSection( &movieCriticalSection );

   /*
      Movie system has been or is being uninitialized.
      Do not play new movies.
   */

	PF( "Movie open [ENTER].\n");

   if ( abortMovie )
   {
	  PF( "Movie aborted.\n" );
      LeaveCriticalSection( &movieCriticalSection );
      return MOVIE_ABORT;
   }


   /* Pre-spin the cd */

#ifdef PRESPIN_CD

	char * buf;
	int hnd;
	int retval;

	buf = (char *)malloc( CD_CACHE_SIZE );
	hnd = open( aviFileName, _O_BINARY | _O_RDONLY );
	retval = read( hnd, buf, CD_CACHE_SIZE );
	close(hnd);
	free( buf );

#endif

   /*
      Find an unused slot.
   */

   for ( handle = 0; handle < numOfMovies; handle++ )
      if ( !movie[handle].status )
         break;

   /*
      Exit if unable to find a slot.
   */

   if ( handle == numOfMovies )
   {
	  PF( "No slots available for movie.\n" );
      LeaveCriticalSection( &movieCriticalSection );
      return MOVIE_NO_SLOT;
   }

   /*
      Mark the slot 'in use'.
   */

   item = &movie[handle];
   item->status = MOVIE_STATUS_IN_USE;
   LeaveCriticalSection( &movieCriticalSection );

   if ( videoMode & MOVIE_MODE_HURRY )
      item->sbType = SURFACE_TRY_FAST;
   else
      item->sbType = 0;

   videoMode &= 0xffff;             // Clear high word

   if ( !( audioFlag & MOVIE_NO_AUDIO ) )
   {
      item->aviStreams.audioFlag |= STREAM_AUDIO_ON;

      if ( audioFileName )
      {
         item->aviStreams.audioFlag |= STREAM_AUDIO_EXTERNAL;
         if ( audioFlag & MOVIE_PRELOAD_AUDIO )
            item->aviStreams.audioFlag |= STREAM_AUDIO_PRELOAD;
      }
   }

   /*
      Open file(s) and read headers.
   */

   status = aviOpen( aviFileName, audioFileName,
                     &( item->aviStreams ) );

   if ( status != RIFF_OK )
   {
      aviClose( &( item->aviStreams ) );
      item->status = 0;
	  PF( "Movie open failed.\n" );
      return MOVIE_OPEN_FAILED;
   }

   /*
      Get the surface properties.
   */

   surfaceGetDescription( ddSurface, &sd );

   /*
      Movie player supports HIGH COLOR or TRUE COLOR only.
   */

   if ( sd.bitCount < HICOLOR )
   {
      aviClose( &( item->aviStreams ) );
      item->status = 0;
	  PF( "Unsupported bit depth.\n" );
      return MOVIE_UNSUPPORTED_BITDEPTH;
   }

   /*
      Set the decompression output format.

      If it's in 16 bit pixel-depth mode, check if it is 555 or 565
      and set the color format of the decompression output accordingly.
      Default is BI_RGB for 555.
   */

   item->bihOut.bmiHeader.biCompression = BI_RGB;
   item->bihOut.bmiHeader.biSize = sizeof( BITMAPINFOHEADER );

   if ( sd.bitCount == HICOLOR )
   {
      if ( ( sd.redMask & 0x8000 ) )
      {
         /*
            HICOLOR 565 mode.
         */

         item->bihOut.bmiHeader.biSize = sizeof( BITMAPINFOHEADEREX );
         item->bihOut.bmiHeader.biCompression = BI_BITFIELDS;
         item->bihOut.redMask =
                        sd.redMask;
         item->bihOut.greenMask =
                        sd.greenMask;
         item->bihOut.blueMask =
                        sd.blueMask;
      }
   }

   item->bihOut.bmiHeader.biPlanes = 1;
   item->bihOut.bmiHeader.biBitCount =
               ( WORD ) sd.bitCount;
   item->bihOut.bmiHeader.biWidth =
               item->aviStreams.bihIn.biWidth;

   /*
      Do inverted DIB.
   */

   item->bihOut.bmiHeader.biHeight =
               -item->aviStreams.bihIn.biHeight;

   /*
      Locate the decompress.
   */

   item->hIC = ICDecompressOpen( ICTYPE_VIDEO,
                  item->aviStreams.strh1.fccHandler,
                  &( item->aviStreams.bihIn ),
                  ( LPBITMAPINFOHEADER ) &( item->bihOut ) );

   /*
      Exit if unable to find the decompressor.
   */

   if ( !item->hIC )
   {
	  PF( "Compressor open() failed.\n" );
      aviClose( &( item->aviStreams ) );
      item->status = 0;
      return MOVIE_OPEN_COMPRESSOR_FAILED;
   }

   /*
      Check if both input and output are supported by codec.
   */

   if ( ICDecompressQuery( item->hIC, &( item->aviStreams.bihIn ),
                           ( LPBITMAPINFOHEADER )
                           &( item->bihOut ) ) != ICERR_OK )
   {
      aviClose( &( item->aviStreams ) );
      item->status = 0;
      return MOVIE_UNSUPPORTED_DIB;
   }

   if ( ICDecompressBegin( item->hIC, &( item->aviStreams.bihIn ),
                           ( LPBITMAPINFOHEADER )
                           &( item->bihOut ) ) != ICERR_OK )
   {
      aviClose( &( item->aviStreams ) );
      item->status = 0;
	  PF( "Unsupported DIB.\n" );
      return MOVIE_UNSUPPORTED_DIB;
   }

   /*
      Check if the movie is within a given surface.
   */

   screenWidth = sd.dwWidth;                    // surface width
   screenHeight = sd.dwHeight;                  // surface height
   dibWidth = item->aviStreams.bihIn.biWidth;   // movie width
   dibHeight = item->aviStreams.bihIn.biHeight; // movie height

   switch ( videoMode )
   {
      case MOVIE_MODE_V_DOUBLE:
      case MOVIE_MODE_INTERLACE:
		  break; // THIS LINE is to fix the MPR stuff NOT returning a screen width relative to the window
         if ( ( startX + dibWidth ) > screenWidth )
         {
            ICDecompressEnd ( item->hIC );
            aviClose( &( item->aviStreams ) );
            item->status = 0;
            return MOVIE_BAD_STARTING_COORDINATES;
         }
         if ( ( startY + dibHeight * 2 ) > screenHeight )
         {
            ICDecompressEnd ( item->hIC );
            aviClose( &( item->aviStreams ) );
            item->status = 0;
            return MOVIE_BAD_STARTING_COORDINATES;
         }
         break;

      default:
         if ( ( startX + dibWidth ) > screenWidth )
         {
            ICDecompressEnd ( item->hIC );
            aviClose( &( item->aviStreams ) );
            item->status = 0;
            return MOVIE_BAD_STARTING_COORDINATES;
         }
         if ( ( startY + dibHeight ) > screenHeight )
         {
            ICDecompressEnd ( item->hIC );
            aviClose( &( item->aviStreams ) );
            item->status = 0;
            return MOVIE_BAD_STARTING_COORDINATES;
         }
         break;
   }

   /*
      Initialized remaining movie variables.
   */

   item->videoMode = videoMode;        // render mode
   item->handle = handle;              // handle to be passed to callback
   item->startX = startX;              // upper left x of movie
   item->startY = startY;              // upper left y of movie
   item->totalFrames = 0;              // frames that have been processed
   item->dropFrames = 0;               // frames being dropped
   item->callBack = callBack;          // callback function
   item->callBackID = callBackID;      // ID to be passed to callback
   item->ddSurface = ddSurface;        // destination surface
   item->pixelSize = item->bihOut.bmiHeader.biBitCount / 8;
                                       // output pixel in bytes
   item->movieThreadID = 0;
   item->audioChannel = 0;
   item->lastError = MOVIE_OK;

   /*
      Buffer to hold decompressed data.
   */

// OW
//   if ( ddPointer && ( videoMode != MOVIE_MODE_INTERLACE ) )
   if ( 1 )
   {
      item->sbType |= SURFACE_TYPE_DDRAW;
      item->surfaceBuffer = surfaceCreate( ddPointer, dibWidth, dibHeight );
   }
   else
   {
//      item->surfaceBuffer = AVI_MALLOC( item->pixelSize *
//                                    item->aviStreams.bihIn.biWidth *
//                                    item->aviStreams.bihIn.biHeight );
      item->surfaceBuffer = new char[item->pixelSize *
                                    item->aviStreams.bihIn.biWidth *
                                    item->aviStreams.bihIn.biHeight];
      item->sbType |= SURFACE_TYPE_SYSTEM;
   }

   if ( !item->surfaceBuffer )
   {
      ICDecompressEnd ( item->hIC );
      aviClose( &( item->aviStreams ) );
      item->status = 0;
	  PF( "No memory for movie.\n" );
      return MOVIE_MALLOC_FAILED;
   }

   InitializeCriticalSection( &( item->aviStreams.criticalSection ) );

	PF( "Movie open [EXIT].\n");
   return ( int ) handle;
}

/****************************************************************************

   movieStart

   Purpose:    To start playing a movie.

   Arguments:  Handle to a movie.

   Returns:    MOVIE_OK if sucessful.

****************************************************************************/

int movieStart( int handle )
{
   int            i, status;
   PMOVIE         item;
   PAVISTREAMS    streams;


#ifdef MEASURE_TIME

	now.time	= 0;
	now.millitm	= 0;

	last.time	= 0;
	last.millitm	= 0;

	average		= 0.0;
	total		= 0.0;
	count		= 0;

	once = FALSE;

#endif


   if ( ( DWORD ) handle >= numOfMovies )
      return MOVIE_INVALID_HANDLE;

   item = &( movie[handle] );

   /*
      Cache in some audio and video data.
   */

   streams = &( item->aviStreams );

   if ( streams->audioFlag & STREAM_AUDIO_PRELOAD )
   {
      if ( waveReadBlock( streams ) < RIFF_OK )
         return MOVIE_BAD_AUDIO_FILE;
   }
   else
   {
      /*
         Get number of audio chunks ahead of video and read those chunks.
      */

      i = streams->initialFrames;

      while ( i-- )
      {
         /*
            Read from AVI file if interleaved.  Otherwise, read from an
            external sound file and discard interleaved audio data in AVI if
            any.
         */

         if ( !( streams->audioFlag & STREAM_AUDIO_EXTERNAL ) )
         {
            // Interleaved.

            status = aviReadRecord( streams );
            if ( status != RIFF_OK )
               return MOVIE_BAD_FILE;
         }
         else
         {
            // External audio file.

            if ( streams->mainAVIHeader.dwInitialFrames )
            {
               status = aviReadRecord( streams );
               if ( status != RIFF_OK )
                  return MOVIE_BAD_FILE;
            }

            status = waveReadBlock( streams );
            if ( status != RIFF_OK )
               return MOVIE_BAD_AUDIO_FILE;
         }
      }
   }

   /*
      Preload a few chunks to make sure there is ample audio data.
      ( preloading some video chunks is also necessary if streaming and
         decompressing/rendering are decoupled )
   */

   for ( i = 0; i < streams->numOfVidBlocks; i++ )
   {
      if ( aviReadRecord( streams ) != RIFF_OK )
         return MOVIE_BAD_FILE;

      if ( ( streams->audioFlag & STREAM_AUDIO_EXTERNAL ) &&
            !( streams->audioFlag & STREAM_AUDIO_PRELOAD ) )
         if ( waveReadBlock( streams ) != RIFF_OK )
            return MOVIE_BAD_AUDIO_FILE;
   }

   // Launch filler thread.

   item->hFillerThread = _beginthreadex( NULL, 0, fillerThread, item,
                                          0, (unsigned int *)&( item->fillerThreadID ) );

   if ( !item->hFillerThread )
      return MOVIE_UNABLE_TO_LAUNCH_THREAD;

   // Launch movie thread.

   item->hMovieThread = _beginthreadex( NULL, 0, movieThread, item,
                                          0, (unsigned int *)&( item->movieThreadID ) );

   if ( !item->hMovieThread )
   {
      /*
         Stop filler thread.
      */
      item->status |= MOVIE_STATUS_STOP_THREAD;
      WaitForSingleObject( ( HANDLE ) item->hFillerThread, INFINITE );
      CloseHandle( ( HANDLE ) item->hFillerThread );

      /*
         Reset movie thread ID.
      */
      item->movieThreadID = 0;            // reset thread ID
      return MOVIE_UNABLE_TO_LAUNCH_THREAD;
   }

   item->status |= MOVIE_STATUS_PLAYING;
   Sleep( 0 );                             // give up time slice

   return MOVIE_OK;
}

/****************************************************************************

   movieClose

   Purpose:    To close a movie and to free all buffers.

   Arguments:  Handle to a movie.

   Returns:    MOVIE_OK if sucessful.

****************************************************************************/

#ifdef MEASURE_TIME

#include <stdio.h>
#endif


int movieClose( int handle )
{
   PMOVIE         item;


#ifdef MEASURE_TIME

	average		= count / total; //fps
	FILE* fp	= fopen("measure.dat", "a");
	fprintf(fp, "average framerate = %f fps\n", average);
	fclose(fp);

#endif



   if ( ( DWORD ) handle >= numOfMovies )
      return MOVIE_INVALID_HANDLE;

   item = &( movie[handle] );

   EnterCriticalSection( &movieCriticalSection );
   if ( !( item->status & MOVIE_STATUS_IN_USE ) )
   {
      LeaveCriticalSection( &movieCriticalSection );
      return MOVIE_NOT_IN_USE;
   }

   if ( item->movieThreadID == GetCurrentThreadId( ) )
   {
      LeaveCriticalSection( &movieCriticalSection );
      return MOVIE_CLOSE_THREAD_ERROR;
   }

   if ( ( item->status & MOVIE_STATUS_PLAYING ) ||
            ( item->status & MOVIE_STATUS_THREAD_RUNNING ) )
   {
      item->status |= MOVIE_STATUS_QUIT;
      WaitForSingleObject( ( HANDLE ) item->hMovieThread, INFINITE );
      CloseHandle( ( HANDLE ) item->hMovieThread );
      CloseHandle( ( HANDLE ) item->hFillerThread );
   }

   ICDecompressEnd( item->hIC );
   DeleteCriticalSection( &( item->aviStreams.criticalSection ) );

   if ( item->surfaceBuffer )
   {
      if ( item->sbType & SURFACE_TYPE_SYSTEM )
         delete [] item->surfaceBuffer;
      else
         surfaceRelease( item->surfaceBuffer );

      item->surfaceBuffer = NULL;
   }
   aviClose ( &( item->aviStreams ) );
   item->status = 0;
   LeaveCriticalSection( &movieCriticalSection );

   return MOVIE_OK;
}

/****************************************************************************

   movieStop

   Purpose:    To stop playing a movie.

   Arguments:  Handle to a movie.

   Returns:    MOVIE_OK if sucessful.

****************************************************************************/

int movieStop( int handle )
{
   PMOVIE         item;

   if ( ( DWORD ) handle >= numOfMovies )
      return MOVIE_INVALID_HANDLE;

   item = &( movie[handle] );

   if ( !( item->status & MOVIE_STATUS_IN_USE ) )
      return MOVIE_NOT_IN_USE;

   item->status |= MOVIE_STATUS_QUIT;

   return MOVIE_OK;
}

/****************************************************************************

   movieIsPlaying

   Purpose:    To check if a given movie handle is still active.

   Arguments:  Handle to a movie.

   Returns:    Non-zero if a movie is still active.

****************************************************************************/

int movieIsPlaying( int handle )
{
   if ( ( DWORD ) handle >= numOfMovies )
      return FALSE;

   return ( ( movie[handle].status &
            MOVIE_STATUS_PLAYING ) ? TRUE : FALSE );
}

/****************************************************************************

   movieCount

   Purpose:    To count the number of movie slots currently in use.

   Arguments:  None.

   Returns:    Number of movie slots currently in use.

****************************************************************************/

int movieCount( void )
{
   DWORD i, count;

   EnterCriticalSection( &movieCriticalSection );

   if ( abortMovie )
      count = 0;
   else
      for ( i = count = 0; i < numOfMovies; i++ )
         if ( movie[i].status & MOVIE_STATUS_IN_USE )
            count++;

   LeaveCriticalSection( &movieCriticalSection );

   return count;
}

/****************************************************************************

   fillerThread

   Purpose:    To stream video and audio data.

   Arguments:  Movie item.

   Returns:    None.

****************************************************************************/

static unsigned int __stdcall fillerThread( void* itemIn )
{
   int            status;
   int            exitCode;
   PAVISTREAMS    streams;
   PMOVIE item = (PMOVIE)itemIn;

   exitCode = MOVIE_OK;
   streams = &( item->aviStreams );

   while ( !( ( item->status & MOVIE_STATUS_QUIT ) ||
            ( item->status & MOVIE_STATUS_STOP_THREAD ) ) )
   {
      /*
         Read only if there are free blocks.
      */

      if ( !streams->nextBlockToFill->currentBlockSize )
      {
         if ( item->status & MOVIE_STATUS_EOF )
            break;

         status = aviReadRecord( streams );

         if ( status < RIFF_OK )
         {
            item->lastError = exitCode = MOVIE_THREAD_BAD_FILE;
            item->status |= MOVIE_STATUS_STOP_THREAD;
            break;
         }

         if ( status == RIFF_END_FILE )
            item->status |= MOVIE_STATUS_EOF;

         /*
            Read audio data from an external sound file.
         */

         if ( ( streams->audioFlag & STREAM_AUDIO_EXTERNAL ) &&
               !( item->status & MOVIE_STATUS_AUDIO_EOF ) &&
               !( streams->audioFlag & STREAM_AUDIO_PRELOAD ) )
         {
            status = waveReadBlock( streams );

            if ( status < RIFF_OK )
            {
               item->lastError = exitCode = MOVIE_THREAD_BAD_AUDIO_FILE;
               item->status |= MOVIE_STATUS_STOP_THREAD;
               break;
            }

            if ( status == RIFF_AUDIO_END_FILE )
               item->status |= MOVIE_STATUS_AUDIO_EOF;
         }
      }
      else
         Sleep( 0 );
   }

   return ( unsigned int ) exitCode;
}

/****************************************************************************

   movieThread

   Purpose:    To decompress and to put movie up on the screen.

   Arguments:  Movie item.

   Returns:    None.

****************************************************************************/

static unsigned int __stdcall movieThread( void* itemIn )
{
   int            timeFrames, dropFlag;
   int            exitCode;
   unsigned long  timeElapsed, firstTime;
   DWORD          errorCode;
   SURFACEACCESS  sa;
   PAVISTREAMS    streams;
   PMOVIE item = (PMOVIE)itemIn;

   firstTime = TRUE;
   exitCode = MOVIE_OK;
   item->status |= MOVIE_STATUS_THREAD_RUNNING;
   streams = &( item->aviStreams );

   if (  ( item->sbType & SURFACE_TYPE_SYSTEM ) &&
         ( item->sbType & SURFACE_TRY_FAST ) )
   {
      surfaceGetPointer( item->ddSurface, &( item->sa ) );
      if ( item->sa.lockStatus == SURFACE_IS_UNLOCKED )
      {
         item->status |= MOVIE_STATUS_STOP_THREAD;
         item->lastError = exitCode = MOVIE_BUFFER_LOCK_FAIL;
         WaitForSingleObject( ( HANDLE ) item->hFillerThread, INFINITE );
         item->status &= ~MOVIE_STATUS_PLAYING;
      }
      surfaceReleasePointer( item->ddSurface, &( item->sa ) );
   }

   while ( !( item->status & MOVIE_STATUS_QUIT ) )
   {
      /*
         Process a frame.
      */

      if ( streams->currentBlock->currentBlockSize )
      {
         if ( firstTime )
            timeFrames = 0;
         else
         {
            if ( streams->audioFlag & STREAM_AUDIO_ON)
            {
#if   AUDIO_ON
               /*
                  Use audio to synch up if it's on.
               */


//               timeElapsed = AudioCount( item->audioHandle,
//                                       AUDIO_UNIT_SIZE, AUDIO_SIZE_PLAYED );
               timeElapsed=F4StreamPlayed(item->audioChannel); // psound
               timeFrames = timeElapsed / streams->audioSizePerFrame;

#else

               timeElapsed = timeGetTime( ) - item->startTime;
               timeFrames = timeElapsed / streams->timePerFrame;
#endif
            }
            else
            {
               /*
                  Use timer to synch up.
               */

               timeElapsed = timeGetTime( ) - item->startTime;
               timeFrames = timeElapsed / streams->timePerFrame;
            }

            timeFrames -= item->totalFrames;
         }

         if ( timeFrames > -1 )
         {
            /*
               Decompress a frame if audio/timer is behind or on time.
            */

            if ( item->sbType & SURFACE_TYPE_SYSTEM )
            {
               errorCode = ICDecompress( item->hIC, 0, &( streams->bihIn ),
                              streams->currentBlock->buffer,
                              ( LPBITMAPINFOHEADER )
                              &( item->bihOut ),
                              item->surfaceBuffer );
            }
            else
            {
               surfaceGetPointer( item->surfaceBuffer, &sa );
               if ( sa.lockStatus == SURFACE_IS_LOCKED )
               {
                  errorCode = ICDecompress( item->hIC, 0, &( streams->bihIn ),
                                 streams->currentBlock->buffer,
                                 ( LPBITMAPINFOHEADER )
                                 &( item->bihOut ),
                                 sa.surfacePtr );
                  surfaceReleasePointer( item->surfaceBuffer, &sa );
               }
               else
               {
                  item->status |= MOVIE_STATUS_STOP_THREAD;
                  item->lastError = exitCode = MOVIE_BUFFER_LOCK_FAIL;
                  break;
               }
            }

            if ( errorCode != ICERR_OK )
            {
               item->status |= MOVIE_STATUS_STOP_THREAD;
               item->lastError = exitCode = MOVIE_THREAD_BAD_DECOMPRESS;
               break;
            }

            dropFlag = 0;

            if ( timeFrames > 1 )
            {
               /*
                  Skip copy if behind.
               */

               item->dropFrames++;
               dropFlag++;
            }
            else
               doFrame( item );

            /*
               Issue a callback.
            */

            if ( item->callBack )
                item->callBack( item->handle, item->ddSurface,
                                 item->totalFrames, item->callBackID,
                                 dropFlag, &( item->sa ) );

            item->totalFrames++;          // increment frame number

            if ( firstTime )
            {
				firstTime = FALSE;
#if   AUDIO_ON
					if ( streams->audioFlag & STREAM_AUDIO_ON )
					{
					unsigned long   timeBegin;
					DWORD           bytesProcessed;

						item->audioChannel=(int)F4CreateStream(&streams->waveFormat,0.5f);
						if(item->audioChannel != 0)
						{
							void *test;
							test=&item->handle;
							F4StartCallbackStream(item->audioChannel,test,fillSoundBuffer);
							F4SetStreamVolume(item->audioChannel,0);
						}

						/*
							Wait until audio starts
						*/
						timeBegin = timeGetTime( );
						bytesProcessed = F4StreamPlayed( item->audioChannel );
						while( !bytesProcessed )
						{
							if ( ( timeGetTime( ) - timeBegin ) > AUDIO_TIMEOUT )
							{
								item->status |= MOVIE_STATUS_STOP_THREAD;
								item->lastError = exitCode = MOVIE_THREAD_AUDIO_TIMEOUT;
								break;
							}
							bytesProcessed = F4StreamPlayed( item->audioChannel );
						}

						if ( !bytesProcessed )
						{
							break;                                 // time out
						}
					}

#endif
               item->startTime = timeGetTime( );            // record start time
            }

            // Free video block.
            streams->currentBlock->currentBlockSize = 0;
            streams->currentBlock = streams->currentBlock->next;
         }
         else
            Sleep( 10 );
      }
      else
         if ( ( item->status & MOVIE_STATUS_EOF ) ||
               ( item->status & MOVIE_STATUS_STOP_THREAD ) )
            break;
   }

   WaitForSingleObject( ( HANDLE ) item->hFillerThread, INFINITE );

#if   AUDIO_ON
       F4StopStream(item->audioChannel);
//
//   if ( item->audioChannel )
//      SoundReleaseChannel( item->audioChannel );
//
#endif

   item->status &= ~MOVIE_STATUS_PLAYING;
   return ( unsigned int ) exitCode;
}

/****************************************************************************

   fillSoundBuffer

   Purpose:    To fill audio buffer.

   Arguments:  Handle to the audio item.
               Pointer to the audio buffer to fill.
               Number of bytes to copy to the audio buffer.

   Returns:    Number of bytes actually copied to the audio buffer.

****************************************************************************/

//static int fillSoundBuffer( int item_handle, void *soundBuffer, int length )
static DWORD fillSoundBuffer( void *me, char *soundBuffer, DWORD length )
{
//   AUDIO_ITEM  *audio;
   int         movieHandle, fillerSize, dataToCopy, filler;
   int         size;
   PMOVIE      item;
   PAVISTREAMS streams;
   char        *ptr, *dsb;

//   audio = AudioGetItem( item_handle );
//   movieHandle = audio->user_data[0];

   movieHandle=*((int*)me);
   item = &( movie[movieHandle] );
   if ( !( item->status & MOVIE_STATUS_PLAYING ) )
      return 0;

   streams = &( item->aviStreams );

   ptr = streams->waveBuffer + streams->waveBufferRead;
   dsb = ( char * ) soundBuffer;

   EnterCriticalSection( &( streams->criticalSection ) );

   if ( streams->dataInWaveBuffer > ( DWORD ) length )
   {
      fillerSize = 0;
      dataToCopy = length;
   }
   else
   {
      fillerSize = length - streams->dataInWaveBuffer;
      dataToCopy = streams->dataInWaveBuffer;
   }

   streams->dataInWaveBuffer -= dataToCopy;

   LeaveCriticalSection( &( streams->criticalSection ) );

   if ( dataToCopy )
   {
      size = streams->waveBufferRead + dataToCopy;
      size -= streams->waveBufferLen;

      if ( size <= 0 )
      {
         memcpy( dsb, ptr, dataToCopy );
         ptr += dataToCopy;
         dsb += dataToCopy;
         streams->waveBufferRead += dataToCopy;
         if ( !size )
            streams->waveBufferRead = 0;
      }
      else
      {
         dataToCopy = streams->waveBufferLen -
                        streams->waveBufferRead;
         memcpy( dsb, ptr, dataToCopy );
         ptr = streams->waveBuffer;
         dsb += dataToCopy;
         memcpy( dsb, ptr, size );
         streams->waveBufferRead = size;
         dsb += size;
      }
   }

   if ( fillerSize )
   {
      filler = 0x80;
      if ( streams->waveFormat.wBitsPerSample != 8 )
         filler = 0;

      memset( dsb, filler, fillerSize );
   }

   return length;
}

/****************************************************************************

   movieGetLastError

   Purpose:    Return last error.

   Arguments:  Handle to a movie.

   Returns:    Last error.

****************************************************************************/

int movieGetLastError( int handle )
{
   if ( ( DWORD ) handle >= numOfMovies )
      return MOVIE_INVALID_HANDLE;

   return movie[handle].lastError;
}
