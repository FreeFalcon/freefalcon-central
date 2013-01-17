#ifndef  _RIFFIO_
#define  _RIFFIO_

#include <windows.h>
#include <vfw.h>
#include <process.h>
#include "include\avifile.h"
#include "include\avimem.h"

#ifdef    __cplusplus
extern   "C"
{
#endif

#define  RIFF_OK              0
#define  RIFF_END_FILE        1
#define  RIFF_AUDIO_END_FILE  2

#define  RIFF_OPEN_FAILED           -1
#define  RIFF_BAD_FORMAT            -2
#define  RIFF_BAD_FILENAME          -3
#define  RIFF_MALLOC_FAILED         -4
#define  RIFF_REALLOC_FAILED        -5
#define  RIFF_NO_AUDIO              -6
#define  RIFF_BAD_AUDIO_FORMAT      -7
#define  RIFF_OPEN_AUDIO_FAILED     -8

#define  MAX_VIDBLOCKS           24

/*
   AUDIO FLAG BITFIELDS
*/

#define  STREAM_AUDIO_ON         1
#define  STREAM_AUDIO_EXTERNAL   2
#define  STREAM_AUDIO_PRELOAD    4

typedef struct tagRIFFCHUNK
{
   long  chunkID;
   long  chunkLength;
   long  chunkType;
} RIFFCHUNK, *PRIFFCHUNK;

typedef struct tagRIFFSUBCHUNK
{
   long  subChunkID;
   long  subChunkLength;
} RIFFSUBCHUNK, *PRIFFSUBCHUNK;

typedef struct tagRIFFAVIHEADER
{
   RIFFCHUNK      avi;
   RIFFCHUNK      hdrl;
   RIFFSUBCHUNK   avih;
} RIFFAVIHEADER, *PRIFFAVIHEADER;

#define  RIFF_CHUNK        sizeof( RIFFCHUNK )
#define  RIFF_SUB_CHUNK    sizeof( RIFFSUBCHUNK )
#define  RIFF_AVI_HEADER   sizeof( RIFFAVIHEADER )   

typedef struct tagVIDBLOCK
{
   DWORD                bufferSize;
   DWORD                currentBlockSize;
   char                 *buffer;
   struct tagVIDBLOCK   *next;
} VIDBLOCK, *PVIDBLOCK;

typedef struct tagAVISTREAMS
{
   int                  handle;                 // AVI file handle
   int                  externalSoundHandle;    // optional WAV file handle
   int                  audioFlag;              // variable to hold audio
                                                // bitfields
   int                  initialFrames;          // number of audio chunks
                                                // at the beginning of
                                                // 'movi' body
   int                  frameRate;              // number of frames per
                                                // second
   int                  timePerFrame;           // duration of a frame in
                                                // milliseconds
   long                 audioSizePerFrame;      // number of bytes
                                                // per frame
   long                 totalAudioRemaining;    // number of bytes to process
   long                 numOfVidBlocks;         // number of video blocks
   long                 audioToReadPerFrame;    // number of bytes to read
                                                // per frame - it is the
                                                // size of the WAV file if
                                                // preloaded

   MainAVIHeader        mainAVIHeader;
   AVIStreamHeader      strh1;                  // video stream header
   BITMAPINFOHEADER     bihIn;
   AVIStreamHeader      strh2;                  // audio stream header
   WAVEFORMATEX         waveFormat;
   char                 *waveBuffer;            // audio buffer
   DWORD                dataInWaveBuffer;       // audio data to process
   DWORD                waveBufferLen;          // length of audio buffer
   DWORD                waveBufferWrite;        // write offset
   DWORD                waveBufferRead;         // read offset
   VIDBLOCK             videoBlock[MAX_VIDBLOCKS];
   PVIDBLOCK            currentBlock;           // video block to process
   PVIDBLOCK            nextBlockToFill;        // video block to fill
   RIFFSUBCHUNK         preRECSubChunk;
   CRITICAL_SECTION     criticalSection;
} AVISTREAMS, *PAVISTREAMS;

extern   int   aviOpen( char *aviFileName, char *audioFileName,
                        AVISTREAMS *aviStreams );
extern   void  aviClose( AVISTREAMS *aviStreams );
extern   int   aviReadRecord( AVISTREAMS *aviStreams );
extern   int   waveReadBlock( AVISTREAMS *aviStreams );

#ifdef __cplusplus
}
#endif

#endif   /* _RIFFIO_ */
