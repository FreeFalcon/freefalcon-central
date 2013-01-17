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

#include "avimovie.h"
//#include "sndmgr.h"
#include "fsound.h" //psound
#include "surface.h"
#include <process.h>

#define  HICOLOR        16
#define  MAX_MOVIES     4
#define  DSB_SIZE       16384

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
                                       int );
                                          // called issued every movie
                                          // frame
   AVISTREAMS           aviStreams;       // video and audio streams
//   AUDIO_CHANNEL        *audioChannel;    // audio channel
   int			        audioChannel;    // Stream ID
   int                  audioHandle;      // audio handle
   int                  lastError;        // last error code
} MOVIE, *PMOVIE;

extern   unsigned int __stdcall movieThread( PMOVIE item );
extern   unsigned int __stdcall fillerThread( PMOVIE item );
extern   void doFrame ( PMOVIE item );
//extern   int  fillSoundBuffer( int item_handle, void *soundBuffer,
//                                int length );

// make to work with psound
static DWORD fillSoundBuffer( void *me, void *soundBuffer, DWORD length );

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
   SURFACEACCESS  sa;

   if ( item->sbType == SURFACE_TYPE_SYSTEM )
   {
      width = item->aviStreams.bihIn.biWidth *
                         item->pixelSize;          // width in bytes
      height = item->aviStreams.bihIn.biHeight;
      buffer = ( char * ) item->surfaceBuffer;     // bitmap to render

      surfaceGetPointer( item->ddSurface, &sa );
				
      tempPtr = ( char * ) sa.surfacePtr;          // get pointer to the surface
      tempPtr += item->startY * sa.lPitch;         // get startY address
      tempPtr += item->startX * item->pixelSize;   // get upper left address
      padding = sa.lPitch -  width;                // offset to next scan line

      switch ( item->videoMode )
      {
         case MOVIE_MODE_V_DOUBLE:
            for ( i = 0; i < height; i++ )
            {
               memcpy( tempPtr, buffer, width );
               tempPtr += sa.lPitch;

               memcpy( tempPtr, buffer, width );
               tempPtr += sa.lPitch;
               buffer += width;
            }
            break;


         case MOVIE_MODE_INTERLACE:
            for ( i = 0; i < height; i++ )
            {
               memcpy( tempPtr, buffer, width );
               tempPtr += sa.lPitch + sa.lPitch;
               buffer += width;
            }
            break;


         default:
            if ( padding )
               for ( i = 0; i < height; i++ )
               {
                  memcpy( tempPtr, buffer, width );
                  tempPtr += sa.lPitch;
                  buffer += width;
               }
            else
               memcpy( tempPtr, buffer, width * height );
            break;
      }

      surfaceReleasePointer( item->ddSurface, &sa );
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

   movie = AVI_MALLOC( numMovies * sizeof( MOVIE ) );
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

   AVI_FREE( movie );
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
                                 int dropFlag ),
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

   if ( abortMovie )
   {
      LeaveCriticalSection( &movieCriticalSection );
      return MOVIE_ABORT;
   }      

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
      LeaveCriticalSection( &movieCriticalSection );
      return MOVIE_NO_SLOT;
   }

   /*
      Mark the slot 'in use'.
   */

   item = &movie[handle];
   item->status = MOVIE_STATUS_IN_USE;
   LeaveCriticalSection( &movieCriticalSection );

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

   if ( ddPointer && ( videoMode != MOVIE_MODE_INTERLACE ) )
   {
      item->sbType = SURFACE_TYPE_DDRAW;
      item->surfaceBuffer = surfaceCreate( ddPointer, dibWidth, dibHeight );
   }
   else
   {
      item->surfaceBuffer = AVI_MALLOC( item->pixelSize *
                                    item->aviStreams.bihIn.biWidth *
                                    item->aviStreams.bihIn.biHeight );
      item->sbType = SURFACE_TYPE_SYSTEM;
   }

   if ( !item->surfaceBuffer )
   {
      ICDecompressEnd ( item->hIC );
      aviClose( &( item->aviStreams ) );
      item->status = 0;
      return MOVIE_MALLOC_FAILED;
   }

   InitializeCriticalSection( &( item->aviStreams.criticalSection ) );

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
                                          0, &( item->fillerThreadID ) );

   if ( !item->hFillerThread )
      return MOVIE_UNABLE_TO_LAUNCH_THREAD;

   // Launch movie thread.

   item->hMovieThread = _beginthreadex( NULL, 0, movieThread, item,
                                          0, &( item->movieThreadID ) );

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

int movieClose( int handle )
{
   PMOVIE         item;

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
      if ( item->sbType == SURFACE_TYPE_SYSTEM )
         AVI_FREE( item->surfaceBuffer );
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

static unsigned int __stdcall fillerThread( PMOVIE item )
{
   int            status;
   int            exitCode;
   PAVISTREAMS    streams;

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

static unsigned int __stdcall movieThread( PMOVIE item )
{
   int            timeFrames, dropFlag;
   int            exitCode;
   unsigned long  timeElapsed, firstTime;
   DWORD          errorCode;
   SURFACEACCESS  sa;
   PAVISTREAMS    streams;

   firstTime = TRUE;
   exitCode = MOVIE_OK;
   item->status |= MOVIE_STATUS_THREAD_RUNNING;
   streams = &( item->aviStreams );

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

            if ( item->sbType == SURFACE_TYPE_SYSTEM )
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
               errorCode = ICDecompress( item->hIC, 0, &( streams->bihIn ),
                              streams->currentBlock->buffer,
                              ( LPBITMAPINFOHEADER )
                              &( item->bihOut ),
                              sa.surfacePtr );
               surfaceReleasePointer( item->surfaceBuffer, &sa );
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
               streams->currentBlock->currentBlockSize = 0;
               streams->currentBlock = streams->currentBlock->next;
               dropFlag++;
            }
            else
            {
               doFrame( item );
               streams->currentBlock->currentBlockSize = 0;
               streams->currentBlock = streams->currentBlock->next;
            }

            /*
               Issue a callback.
            */

            if ( item->callBack )
              item->callBack( item->handle, item->ddSurface,
                                 item->totalFrames, item->callBackID,
                                 dropFlag );

            item->totalFrames++;          // increment frame number

            if ( firstTime )
            {
				firstTime = FALSE;
#if   AUDIO_ON
                if ( streams->audioFlag & STREAM_AUDIO_ON )
                {
					item->audioChannel=(int)F4CreateStream(&streams->waveFormat,1.0f);
					if(item->audioChannel != 0)
					{
						void *test;
						test=&item->handle;
						F4StartCallbackStream(item->audioChannel,test,fillSoundBuffer);
					}
				}

//                if ( streams->audioFlag & STREAM_AUDIO_ON )
//                {
//                  int            dummyHandle;
//                  AUDIO_ITEM     *audio;
//
//                     dummyHandle = AudioCreate( &( streams->waveFormat ) );
//                     item->audioChannel = SoundRequestChannel( dummyHandle,
//                                             &( item->audioHandle ), DSB_SIZE );
//
//                     audio = AudioGetItem( item->audioHandle );
//                     audio->user_data[0] = item->handle;
//                     AudioDestroy( dummyHandle );
//                     if ( item->audioChannel )
//                        SoundStreamChannel( item->audioChannel, fillSoundBuffer );
//                  }
//               }
#endif
               item->startTime = timeGetTime( );            // record start time
            }
         }
         else
            Sleep( 0 );
      }
      else
         if ( ( item->status & MOVIE_STATUS_EOF ) ||
               ( item->status & MOVIE_STATUS_STOP_THREAD ) )
            break;
   }

   WaitForSingleObject( ( HANDLE ) item->hFillerThread, INFINITE );

#if   AUDIO_ON
	   F4RemoveStream(item->audioChannel);
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
static DWORD fillSoundBuffer( int *me, void *soundBuffer, DWORD length )
{
//   AUDIO_ITEM  *audio;
   int         movieHandle, fillerSize, dataToCopy, filler;
   int         size;
   PMOVIE      item;
   PAVISTREAMS streams;
   char        *ptr, *dsb;

//   audio = AudioGetItem( item_handle );
//   movieHandle = audio->user_data[0];

   movieHandle=(int)(*me);
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
