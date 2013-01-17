#ifndef  _MOVIE_
#define  _MOVIE_

#include <windows.h>
#include <vfw.h>
#include "riffio.h"
#include "include\avimem.h"
#include "surface.h"

#ifdef   __cplusplus
extern   "C"
{
#endif

#ifndef  TRUE
   TRUE  1
   FALSE 0
#endif

#define  AUDIO_ON                         1  // 0 - don't call sound
                                             // manager
                                             
/*
   Movie return codes
*/
#define  MOVIE_OK                         0
#define  MOVIE_MALLOC_FAILED              -1
#define  MOVIE_OPEN_FAILED                -2
#define  MOVIE_UNSUPPORTED_BITDEPTH       -3
#define  MOVIE_OPEN_COMPRESSOR_FAILED     -4
#define  MOVIE_UNSUPPORTED_DIB            -5
#define  MOVIE_BAD_STARTING_COORDINATES   -6
#define  MOVIE_NO_SLOT                    -7
#define  MOVIE_HAS_BEEN_INITIALIZED       -8
#define  MOVIE_INVALID_NUMBER             -9
#define  MOVIE_ABORT                      -10
#define  MOVIE_THREAD_BAD_FILE            -11
#define  MOVIE_THREAD_BAD_AUDIO_FILE      -12
#define  MOVIE_THREAD_BAD_DECOMPRESS      -13
#define  MOVIE_INVALID_HANDLE             -14
#define  MOVIE_CLOSE_THREAD_ERROR         -15
#define  MOVIE_NOT_IN_USE                 -16
#define  MOVIE_UNABLE_TO_LAUNCH_THREAD    -17
#define  MOVIE_BAD_FILE                   -18
#define  MOVIE_BAD_AUDIO_FILE             -19
#define  MOVIE_THREAD_AUDIO_TIMEOUT       -20
#define  MOVIE_BUFFER_LOCK_FAIL           -21

/*
   Movie status bitfields
*/
#define  MOVIE_STATUS_PLAYING          1
#define  MOVIE_STATUS_EOF              2
#define  MOVIE_STATUS_QUIT             4
#define  MOVIE_STATUS_AUDIO_EOF        8
#define  MOVIE_STATUS_IN_USE           16
#define  MOVIE_STATUS_THREAD_RUNNING   32
#define  MOVIE_STATUS_STOP_THREAD      64

/*
   Movie render modes ( moviePlay argument )
*/
#define  MOVIE_MODE_NORMAL             0           // single pixel
#define  MOVIE_MODE_V_DOUBLE           1           // double pixel vertical
#define  MOVIE_MODE_INTERLACE          2           // every-other-scan-line
#define  MOVIE_MODE_HURRY              ( 1 << 16 )

/*
   Surface type bit flags
*/
#define  SURFACE_TYPE_SYSTEM           1
#define  SURFACE_TYPE_DDRAW            2
#define  SURFACE_TRY_FAST              4

/*
   Audio bitfields ( moviePlay argument )
*/
#define  MOVIE_USE_AUDIO               0     // audio on
#define  MOVIE_NO_AUDIO                1     // audio off
#define  MOVIE_PRELOAD_AUDIO           2     // load the entire audio file

/*
   Function prototypes
*/
extern   int   movieInit( int numMovies, LPVOID lpDD );
extern   void  movieUnInit( void );
extern   int   movieOpen( char *aviFileName,
                           char *audioFileName,
                           LPVOID ddSurface,
                           int callBackID,
                           void ( *callBack )( int handle,
                                             LPVOID ddSurface,
                                             int frameNumber,
                                             int callBackID,
                                             int dropFlag,
                                             SURFACEACCESS *sa ),
                           int startX, int startY, int videoMode,
                           int audioFlag );

extern   int   movieClose( int handle );
extern   int   movieStart( int handle );
extern   int   movieStop( int handle );
extern   int   movieIsPlaying( int handle );
extern   int   movieCount( void );
extern   int   movieGetLastError( int handle );

#ifdef   __cplusplus
}
#endif

#endif   /* _MOVIE_ */
