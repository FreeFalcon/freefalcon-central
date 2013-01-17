/*
   RIFFIO.C

   Exported functions:

      aviOpen
      aviClose
      aviReadRecord
      waveReadBlock

   Programmed by Kuswara Pranawahadi               September 5, 1996
*/

#include <windows.h>
#include "fsound.h" //psound
#include "riffio.h"

/*
   MACRO to switch from big to little endian.
*/

#define  RIFFCODE(a,b,c,d) ((((long) d) << 24) | \
                           (((long) c) << 16) | \
                           (((long) b) << 8) | \
                           (((long) a)))

#define  RIFF_TOKEN_SIZE      4
#define  RING_BUFFER          65536
#define  ONE_SECOND           1000
#define  DEFAULT_BLOCK_SIZE   16384
#define  AUDIO_PADDING        4        // extra blocks to prevent
                                       // overflow

/****************************************************************************

   aviOpen

   Purpose:    To open 'AVI' and external 'WAV', and to allocate/read
               headers.

   Arguments:  Movie file name ( AVI file )
               Optional audio file name ( WAV file )
               AVISTREAMS ( pointer )

   Returns:    RIFF_OK if successful; negative number otherwise.


****************************************************************************/

int aviOpen( char *aviFileName, char *audioFileName,
               PAVISTREAMS streams )
{
   RIFFAVIHEADER     aviHeader;
   RIFFCHUNK         chunk;
   RIFFSUBCHUNK      subChunk;
   PVIDBLOCK         videoBlock;
   int               currentFilePointer;
   int               blockSize, i, remainder;
   long              token;

   if ( !aviFileName )
      return RIFF_BAD_FILENAME;

   streams->handle = AVI_OPEN( aviFileName, _O_RDONLY | _O_BINARY );
   if ( streams->handle == -1 )
      return RIFF_OPEN_FAILED;

   /*
      Read and verify AVI header.

      RIFF (   'AVI '
               LIST (   'hdrl'
                        'avih' ( <MainAVIHeader> )
   */

   if ( AVI_READ( streams->handle, &aviHeader,
                  RIFF_AVI_HEADER ) != RIFF_AVI_HEADER )
      return RIFF_BAD_FORMAT;

   if ( aviHeader.avi.chunkID != RIFFCODE( 'R', 'I', 'F', 'F' ) )
      return RIFF_BAD_FORMAT;

   if ( aviHeader.avi.chunkType != RIFFCODE( 'A', 'V', 'I', ' ' ) )
      return RIFF_BAD_FORMAT;

   if ( aviHeader.hdrl.chunkID != RIFFCODE( 'L', 'I', 'S', 'T' ) )
      return RIFF_BAD_FORMAT;

   if ( aviHeader.hdrl.chunkType != RIFFCODE( 'h', 'd', 'r', 'l' ) )
      return RIFF_BAD_FORMAT;

   if ( aviHeader.avih.subChunkID != RIFFCODE( 'a', 'v', 'i', 'h' ) )
      return RIFF_BAD_FORMAT;

   if ( AVI_READ( streams->handle, &( streams->mainAVIHeader ),
                  aviHeader.avih.subChunkLength ) !=
                  aviHeader.avih.subChunkLength )
      return RIFF_BAD_FORMAT;

   /*
      Read and verify a stream header.

      LIST (   'strl'
               'strh' ( <AVIStreamHeader> )
               'strf' ( <BIMAPINFOHEADER> )
   */

   currentFilePointer = AVI_SEEK( streams->handle, 0, SEEK_CUR );

   if ( AVI_READ( streams->handle, &chunk, RIFF_CHUNK ) !=
                  RIFF_CHUNK )
      return RIFF_BAD_FORMAT;

   if ( chunk.chunkID != RIFFCODE( 'L', 'I', 'S', 'T' ) )
      return RIFF_BAD_FORMAT;

   if ( chunk.chunkType != RIFFCODE( 's', 't', 'r', 'l' ) )
      return RIFF_BAD_FORMAT;

   if ( AVI_READ( streams->handle, &subChunk, RIFF_SUB_CHUNK ) !=
                  RIFF_SUB_CHUNK )
      return RIFF_BAD_FORMAT;

   if ( subChunk.subChunkID != RIFFCODE( 's', 't', 'r', 'h' ) )
      return RIFF_BAD_FORMAT;

   if ( AVI_READ ( streams->handle, &( streams->strh1 ),
                     subChunk.subChunkLength ) !=
                     subChunk.subChunkLength )
      return RIFF_BAD_FORMAT;

   if ( AVI_READ( streams->handle, &subChunk,
                  RIFF_SUB_CHUNK ) != RIFF_SUB_CHUNK )
      return RIFF_BAD_FORMAT;

   if ( subChunk.subChunkID != RIFFCODE( 's', 't', 'r', 'f' ) )
      return RIFF_BAD_FORMAT;

   if ( AVI_READ ( streams->handle, &( streams->bihIn ),
                  subChunk.subChunkLength ) !=
                  subChunk.subChunkLength )
      return RIFF_BAD_FORMAT;

   /*
      Read audio stream header.
   */

   if ( streams->audioFlag & STREAM_AUDIO_ON )
   {
      if (  streams->audioFlag & STREAM_AUDIO_EXTERNAL  )
      {

         /*
            External sound file...

            RIFF (   'WAVE'
                     'fmt ' ( <WaveFormat> )
                     'data' ( <audio data> )
         */

         streams->externalSoundHandle =
            AVI_OPEN ( audioFileName, _O_RDONLY | _O_BINARY );
         if ( streams->externalSoundHandle == -1 )
            return RIFF_OPEN_AUDIO_FAILED;

         if ( AVI_READ( streams->externalSoundHandle, &chunk, RIFF_CHUNK ) !=
                        RIFF_CHUNK )
            return RIFF_BAD_AUDIO_FORMAT;

         if ( chunk.chunkID != RIFFCODE( 'R', 'I', 'F', 'F' ) )
            return RIFF_BAD_AUDIO_FORMAT;

         if ( chunk.chunkType != RIFFCODE( 'W', 'A', 'V', 'E' ) )
            return RIFF_BAD_AUDIO_FORMAT;

         if ( AVI_READ( streams->externalSoundHandle, &subChunk, \
                        RIFF_SUB_CHUNK ) != RIFF_SUB_CHUNK )
            return RIFF_BAD_AUDIO_FORMAT;

         if ( subChunk.subChunkID != RIFFCODE( 'f', 'm', 't', ' ' ) )
            return RIFF_BAD_AUDIO_FORMAT;

         if ( AVI_READ( streams->externalSoundHandle, \
                        &( streams->waveFormat ), subChunk.subChunkLength )
                        != subChunk.subChunkLength )
            return RIFF_BAD_AUDIO_FORMAT;

         if ( AVI_READ( streams->externalSoundHandle, &subChunk, \
                        RIFF_SUB_CHUNK ) != RIFF_SUB_CHUNK )
            return RIFF_BAD_AUDIO_FORMAT;

         if ( subChunk.subChunkID != RIFFCODE( 'd', 'a', 't', 'a' ) )
            return RIFF_BAD_AUDIO_FORMAT;

         streams->totalAudioRemaining = subChunk.subChunkLength;
      }
      else
      {
         /*
            Check if the file contains multiple streams.
         */

         if ( streams->mainAVIHeader.dwStreams == 1 )
               streams->audioFlag &= ~STREAM_AUDIO_ON;
         else
         {
            /*
               Back to the beginning of stream header ( 'strl' ).
            */

            AVI_SEEK( streams->handle, currentFilePointer, SEEK_SET );

            /*
               Go to the beginning of the next stream.
            */

            AVI_SEEK( streams->handle, ( chunk.chunkLength + \
                     RIFF_SUB_CHUNK ), SEEK_CUR );

            /*
               LIST (   'strl'
                        'strh' ( <AVIStreamHeader> )
                        'strf' ( <WAVEFORMATEX> )
            */

            if ( AVI_READ( streams->handle, &chunk, RIFF_CHUNK ) !=
                           RIFF_CHUNK )
               return RIFF_BAD_FORMAT;

            if ( chunk.chunkID != RIFFCODE( 'L', 'I', 'S', 'T' ) )
               return RIFF_BAD_FORMAT;

            if ( chunk.chunkType != RIFFCODE( 's', 't', 'r', 'l' ) )
               return RIFF_BAD_FORMAT;

            if ( AVI_READ( streams->handle, &subChunk, RIFF_SUB_CHUNK ) !=
                        RIFF_SUB_CHUNK )
               return RIFF_BAD_FORMAT;

            if ( subChunk.subChunkID != RIFFCODE( 's', 't', 'r', 'h' ) )
               return RIFF_BAD_FORMAT;

            if ( AVI_READ ( streams->handle, &( streams->strh2 ),
                        subChunk.subChunkLength ) !=
                        subChunk.subChunkLength )
               return RIFF_BAD_FORMAT;

            if ( AVI_READ( streams->handle, &subChunk,
                        RIFF_SUB_CHUNK ) != RIFF_SUB_CHUNK )
               return RIFF_BAD_FORMAT;

            if ( subChunk.subChunkID != RIFFCODE( 's', 't', 'r', 'f' ) )
               return RIFF_BAD_FORMAT;

            if ( AVI_READ ( streams->handle, &( streams->waveFormat ),
                           subChunk.subChunkLength ) !=
                           subChunk.subChunkLength )
               return RIFF_BAD_FORMAT;
         }
      }
   }

   /*
      Back to the beginning of the file.
   */

   AVI_SEEK( streams->handle, 0, SEEK_SET );

   /*
      Look for the beginning of 'movi' chunk.
   */

   AVI_SEEK( streams->handle, \
               ( aviHeader.hdrl.chunkLength + RIFF_CHUNK +
               RIFF_SUB_CHUNK ), \
               SEEK_CUR );

   /*
      The next chunk can either be a LIST chunk or a JUNK chunk.

      JUNK
      LIST (   'movi'

      or

      LIST (   'movi'
               LIST (   'rec '
                           SubChunk1
   */

   if ( AVI_READ( streams->handle, &subChunk, RIFF_SUB_CHUNK ) !=
                  RIFF_SUB_CHUNK )
      return RIFF_BAD_FORMAT;

   while ( subChunk.subChunkID != RIFFCODE( 'L', 'I', 'S', 'T' ) )
   {
      if ( subChunk.subChunkID == RIFFCODE( 'J', 'U', 'N', 'K' ) )
      {
         AVI_SEEK( streams->handle, subChunk.subChunkLength, \
                     SEEK_CUR );
         if ( AVI_READ( streams->handle, &subChunk, \
                        RIFF_SUB_CHUNK ) != RIFF_SUB_CHUNK )
            return RIFF_BAD_FORMAT;
      }
      else
         return RIFF_BAD_FORMAT;
   }

   if ( AVI_READ( streams->handle, &token, RIFF_TOKEN_SIZE ) !=
                  RIFF_TOKEN_SIZE )
         return RIFF_BAD_FORMAT;

   if ( token != RIFFCODE( 'm', 'o', 'v', 'i' ) )
      return RIFF_BAD_FORMAT;

   /*
      Get rid of 'LIST' and chunk size if there are multiple streams.
   */

   if ( streams->mainAVIHeader.dwStreams > 1 )
   {
      if ( AVI_READ( streams->handle, &( streams->preRECSubChunk ),
                     RIFF_SUB_CHUNK ) != RIFF_SUB_CHUNK )
         return RIFF_BAD_FORMAT;

      if ( streams->preRECSubChunk.subChunkID !=
                     RIFFCODE( 'L', 'I', 'S', 'T' ) )
         return RIFF_BAD_FORMAT;
   }

   /*
      Allocate memory for video blocks.
   */

   videoBlock = &( streams->videoBlock[0] );
   blockSize = streams->strh1.dwSuggestedBufferSize;

   if ( !blockSize )
      blockSize = DEFAULT_BLOCK_SIZE;

   if ( streams->mainAVIHeader.dwTotalFrames > MAX_VIDBLOCKS )
      streams->numOfVidBlocks = MAX_VIDBLOCKS;
   else
      streams->numOfVidBlocks = streams->mainAVIHeader.dwTotalFrames;

   for ( i = 0; i < streams->numOfVidBlocks; i++ )
   {
      videoBlock[i].bufferSize = blockSize;
//      videoBlock[i].buffer = AVI_MALLOC( blockSize );
      videoBlock[i].buffer = new char[blockSize];
      if ( !videoBlock[i].buffer )
         return RIFF_MALLOC_FAILED;

      if ( i < ( streams->numOfVidBlocks - 1 ) )
         videoBlock[i].next = &videoBlock[i+1];
      else
         videoBlock[i].next = &videoBlock[0];
   }

   streams->currentBlock = videoBlock;
   streams->nextBlockToFill = videoBlock;

   streams->frameRate = ( streams->strh1.dwRate + streams->strh1.dwScale / 2 ) /
                        streams->strh1.dwScale;

   streams->timePerFrame = ( ONE_SECOND + streams->frameRate - 1 ) / streams->frameRate;
   streams->initialFrames = streams->mainAVIHeader.dwInitialFrames ;

   if ( streams->audioFlag & STREAM_AUDIO_ON )
   {
      if ( streams->audioFlag & STREAM_AUDIO_PRELOAD )
      {
         i = streams->audioToReadPerFrame = streams->totalAudioRemaining;

         streams->audioSizePerFrame = ( streams->waveFormat.nAvgBytesPerSec +
                                        streams->frameRate - 1 ) /
                                        streams->frameRate;

         /*
            Make audioSizePerFrame a multiple of sample size.
         */

         remainder = streams->audioSizePerFrame %
                     streams->waveFormat.nBlockAlign;

         if ( remainder )
         {
            streams->audioSizePerFrame += ( streams->waveFormat.nBlockAlign -
                                          remainder );
         }
      }
      else
      {
         if ( ( streams->audioFlag & STREAM_AUDIO_EXTERNAL ) &&
               !streams->initialFrames )
            streams->initialFrames = 1 + ( streams->frameRate * 3 / 4 );

         streams->audioSizePerFrame = ( streams->waveFormat.nAvgBytesPerSec +
                                        streams->frameRate - 1 ) /
                                        streams->frameRate;

         /*
            Make audioSizePerFrame a multiple of sample size.
         */

         remainder = streams->audioSizePerFrame %
                     streams->waveFormat.nBlockAlign;

         if ( remainder )
         {
            streams->audioSizePerFrame += ( streams->waveFormat.nBlockAlign -
                                          remainder );
         }

         streams->audioToReadPerFrame = streams->audioSizePerFrame;

         i = streams->audioSizePerFrame *
               ( streams->numOfVidBlocks + streams->initialFrames +
               AUDIO_PADDING );

         if ( i < RING_BUFFER )
            i = RING_BUFFER;
      }

//      streams->waveBuffer = AVI_MALLOC( i );
      streams->waveBuffer = new char[i];

      if ( !streams->waveBuffer )
         return RIFF_MALLOC_FAILED;

      streams->waveBufferLen = i;
      streams->waveBufferWrite = 0;
      streams->waveBufferRead = 0;
      streams->dataInWaveBuffer = 0;
   }

   return RIFF_OK;
}

/****************************************************************************

   aviClose

   Purpose:    To close 'AVI' and external 'WAV', and to free buffers
               allocated for headers.

   Arguments:  AVISTREAMS ( pointer )

   Returns:    None.


****************************************************************************/

void aviClose( PAVISTREAMS streams )
{
   int   i;

   if ( streams->handle )
      AVI_CLOSE( streams->handle );

   if ( streams->externalSoundHandle )
      AVI_CLOSE( streams->externalSoundHandle );

   if ( streams->waveBuffer )
      delete [] streams->waveBuffer;

   for ( i = 0; i < streams->numOfVidBlocks; i++ )
   {
      if ( streams->videoBlock[i].buffer )
         delete [] streams->videoBlock[i].buffer;
   }

   memset ( streams, 0, sizeof( AVISTREAMS ) );
}

/****************************************************************************

   aviReadRecord

   Purpose:    To read video and audio chunks.

   Arguments:  AVISTREAMS ( pointer )

   Returns:    None.


****************************************************************************/

int aviReadRecord ( PAVISTREAMS streams )
{
   RIFFSUBCHUNK      subChunk;
   long              token;
   DWORD             streamCount;
   long              len, newPtr, size;
   long              ptr;
   long              recordLen;
   long              chunkLength;
   PVIDBLOCK         nextBlock;

   streamCount = streams->mainAVIHeader.dwStreams;

   if ( streamCount == 1 )
   {
      /*
         No 'rec ' header if single stream.
      */

      if ( AVI_READ( streams->handle, &subChunk, RIFF_SUB_CHUNK ) !=
                     RIFF_SUB_CHUNK )
         return RIFF_BAD_FORMAT;

      /*
         End of video stream.
      */

      if ( subChunk.subChunkID == RIFFCODE( 'i', 'd', 'x', '1' ) )
         return RIFF_END_FILE;

      nextBlock = streams->nextBlockToFill;

      if ( nextBlock->bufferSize < ( DWORD ) subChunk.subChunkLength )
      {
         delete [] nextBlock->buffer;
//         nextBlock->buffer = AVI_MALLOC( subChunk.subChunkLength );
         nextBlock->buffer = new char[subChunk.subChunkLength];

         if ( !( nextBlock->buffer ) )
            return RIFF_REALLOC_FAILED;

         nextBlock->bufferSize = subChunk.subChunkLength;
      }

      len = AVI_READ( streams->handle, \
                        nextBlock->buffer, \
                        subChunk.subChunkLength );

      if ( len != subChunk.subChunkLength )
         return RIFF_BAD_FORMAT;
      nextBlock->currentBlockSize = len;
      streams->nextBlockToFill = streams->nextBlockToFill->next;
   }
   else
   {
      /*
         LIST (   'rec '
                  '00id'   ( <Video> )
                  '01wb'   ( <Audio> )
         LIST (   'rec '
                  .
                  .
                  .

         LIST and size should be in 'preRECSubChunk'.
         Adjust length to exclude 'rec'.
      */

      recordLen = streams->preRECSubChunk.subChunkLength - 4;

      if ( AVI_READ( streams->handle, &token,
                     RIFF_TOKEN_SIZE ) != RIFF_TOKEN_SIZE )
         return RIFF_BAD_FORMAT;

      if ( token != RIFFCODE( 'r', 'e', 'c', ' ' ) )
         return RIFF_BAD_FORMAT;

      while ( streamCount-- )
      {
         if ( recordLen > RIFF_SUB_CHUNK )
         {
            if ( AVI_READ( streams->handle, &subChunk, \
                        RIFF_SUB_CHUNK ) != RIFF_SUB_CHUNK )
               return RIFF_BAD_FORMAT;
            token = subChunk.subChunkID;
            chunkLength = subChunk.subChunkLength;
         }
         else
         {
            AVI_SEEK( streams->handle, recordLen, SEEK_CUR );
            if ( AVI_READ( streams->handle, \
                        &( streams->preRECSubChunk ), \
                        RIFF_SUB_CHUNK ) != RIFF_SUB_CHUNK )
               return RIFF_BAD_FORMAT;

            token = streams->preRECSubChunk.subChunkID;
            chunkLength = streams->preRECSubChunk.subChunkLength;
         }

         if ( token == RIFFCODE( 'L', 'I', 'S', 'T' ) )
            return RIFF_OK;
         else
         {
            if ( token == RIFFCODE( 'i', 'd', 'x', '1' ) )
               return RIFF_END_FILE;
            else
            {
               token &= 0xffff;
            }
         }

         if ( token == RIFFCODE( '0', '1', 0, 0 ) )
         {
            /*
               Wave chunk.
            */

            recordLen -= ( chunkLength + 8 );

            if ( ( streams->audioFlag & STREAM_AUDIO_EXTERNAL ) ||
                  !( streams->audioFlag & STREAM_AUDIO_ON ) )
               AVI_SEEK( streams->handle, chunkLength, \
                           SEEK_CUR );
            else
            {
               ptr = ( long ) streams->waveBuffer;
               newPtr = streams->waveBufferWrite + chunkLength -
                        streams->waveBufferLen;

               if ( newPtr <= 0 )
               {
                  len = AVI_READ( streams->handle,
                           ( ( char * ) ( ptr + \
                           streams->waveBufferWrite ) ), \
                           chunkLength );

                  if ( len != chunkLength )
                     return RIFF_BAD_FORMAT;

                  streams->waveBufferWrite += chunkLength;
                  if ( streams->waveBufferWrite == streams->waveBufferLen )
                     streams->waveBufferWrite = 0;

                  EnterCriticalSection ( &( streams->criticalSection ) );

                  streams->dataInWaveBuffer += chunkLength;

                  LeaveCriticalSection ( &( streams->criticalSection ) );
               }
               else
               {
                  size = streams->waveBufferLen - streams->waveBufferWrite;
                  len = AVI_READ( streams->handle,
                                 ( ( char * ) ( ptr + \
                                 streams->waveBufferWrite ) ), \
                                 size );

                  if ( len != size )
                     return RIFF_BAD_FORMAT;

                  streams->waveBufferWrite = 0;

                  size = chunkLength - size;
                  len = AVI_READ( streams->handle,
                                 ( ( char * ) ( ptr + \
                                 streams->waveBufferWrite ) ), \
                                 size );

                  if ( len != size )
                     return RIFF_BAD_FORMAT;

                  streams->waveBufferWrite += size;

                  EnterCriticalSection ( &( streams->criticalSection ) );

                  streams->dataInWaveBuffer += chunkLength;

                  LeaveCriticalSection ( &( streams->criticalSection ) );
               }
            }
         }
         else
         {
            if ( token == RIFFCODE( '0', '0', 0, 0 ) )
            {
               /*
                  Video chunk.
               */
               recordLen -= ( chunkLength + 8 );

               nextBlock = streams->nextBlockToFill;

               if ( nextBlock->bufferSize < ( DWORD ) chunkLength )
               {
                  delete [] nextBlock->buffer;
//                  nextBlock->buffer = AVI_MALLOC( chunkLength );
                  nextBlock->buffer = new char[chunkLength];

                  if ( !( nextBlock->buffer ) )
                     return RIFF_REALLOC_FAILED;

                  nextBlock->bufferSize = chunkLength;
               }

               len = AVI_READ( streams->handle, \
                              nextBlock->buffer, \
                              chunkLength );

               if ( len != chunkLength )
                  return RIFF_BAD_FORMAT;

               nextBlock->currentBlockSize = len;
               streams->nextBlockToFill =
                        streams->nextBlockToFill->next;
            }
            else
            {
               recordLen -= ( chunkLength + 8 );

               AVI_SEEK( streams->handle, chunkLength, SEEK_CUR );
            }
         }
      }

      if ( recordLen )
         AVI_SEEK( streams->handle, recordLen, \
                     SEEK_CUR );

      if ( AVI_READ( streams->handle, &( streams->preRECSubChunk ),
                     RIFF_SUB_CHUNK ) != RIFF_SUB_CHUNK )
         return RIFF_BAD_FORMAT;

      if ( streams->preRECSubChunk.subChunkID !=
                     RIFFCODE( 'L', 'I', 'S', 'T' ) )
         return RIFF_BAD_FORMAT;
   }

   return RIFF_OK;
}

/****************************************************************************

   waveReadRecord

   Purpose:    To read audio chunk.

   Arguments:  AVISTREAMS ( pointer )

   Returns:    None.


****************************************************************************/

int waveReadBlock ( PAVISTREAMS streams )
{
   long           ptr, newPtr, len, size;
   long           blockLen;

   if ( !( streams->audioFlag & STREAM_AUDIO_ON ) )
      return RIFF_OK;

   blockLen = streams->audioToReadPerFrame;

   if ( streams->totalAudioRemaining < blockLen )
      blockLen = streams->totalAudioRemaining;

   if ( !blockLen )
      return RIFF_AUDIO_END_FILE;

   streams->totalAudioRemaining -= blockLen;

   ptr = ( long ) streams->waveBuffer;
   newPtr = streams->waveBufferWrite + blockLen -
               streams->waveBufferLen;

   if ( newPtr <= 0 )
   {
      len = AVI_READ( streams->externalSoundHandle,
               ( ( char * ) ( ptr + \
               streams->waveBufferWrite ) ), \
               blockLen );
      if ( len != blockLen )
         return RIFF_BAD_AUDIO_FORMAT;

      streams->waveBufferWrite += blockLen;
      if ( streams->waveBufferWrite == streams->waveBufferLen )
         streams->waveBufferWrite = 0;

      EnterCriticalSection ( &( streams->criticalSection ) );

      streams->dataInWaveBuffer += blockLen;

      LeaveCriticalSection ( &( streams->criticalSection ) );
   }
   else
   {
      size = streams->waveBufferLen - streams->waveBufferWrite;
      len = AVI_READ( streams->externalSoundHandle,
                     ( ( char * ) ( ptr + \
                     streams->waveBufferWrite ) ), \
                     size );

      if ( len != size )
         return RIFF_BAD_AUDIO_FORMAT;

      streams->waveBufferWrite = 0;

      size = blockLen - size;
      len = AVI_READ( streams->externalSoundHandle,
                     ( ( char * ) ( ptr + \
                     streams->waveBufferWrite ) ), \
                     size );

      if ( len != size )
         return RIFF_BAD_AUDIO_FORMAT;

      streams->waveBufferWrite += size;

      EnterCriticalSection ( &( streams->criticalSection ) );

      streams->dataInWaveBuffer += blockLen;

      LeaveCriticalSection ( &( streams->criticalSection ) );
   }

   return RIFF_OK;
}
