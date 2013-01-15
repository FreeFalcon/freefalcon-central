/* ------------------------------------------------------------------------

    silence.cpp

    Used to decompressed conversation files, compile with the
	conv.dll, sndmgr.lib

    STANDALONE application.

    Version 0.01

    Written by Jim DiZoglio (x257)       (c) 1996 Spectrum Holobyte

   ------------------------------------------------------------------------ */


#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "fsound.h"
#include "FalcVoice.h"
#include "Utils\bitio.h"


/*
 * These two strings are used by MAIN-C.C and MAIN-E.C to print
 * messages of importance to the user of the program.
 */
/*
 * These macros define the parameters used to compress the silent
 * sequences.  SILENCE_LIMIT is the maximum size of a signal that can
 * be considered silent, in terms of offset from the center point.
 * START_THRESHOLD gives the number of consecutive silent codes that
 * have to be seen before a run is started.  STOP_THRESHOLD tells how
 * many non-silent codes need to be seen before a run is considered to
 * be over.  SILENCE_CODE is the special code output to the compressed
 * file to indicate that a run has been detected.  SILENCE_CODE is always
 * followed by a single byte indicating how many consecutive silence
 * bytes are to follow.
 */

#define SILENCE_LIMIT   4
#define START_THRESHOLD 5
#define STOP_THRESHOLD  2
#define SILENCE_CODE    0xff
#define IS_SILENCE( c ) ( (c) > ( 0x7f - SILENCE_LIMIT ) && \
                          (c) < ( 0x80 + SILENCE_LIMIT ) )

/*
 * BUFFER_SIZE is the size of the look ahead buffer.  BUFFER_MASK is
 * the mask applied to a buffer index when performing index math.
 */
#define BUFFER_SIZE 8
#define BUFFER_MASK 7

/*
 * Local function prototypes.
 */

int silence_run( int buffer[], int index );
int end_of_silence( int buffer[], int index );


#define COMPRESS5BITS

#ifdef COMPRESS5BITS
#define COMPRESSION_BITS     5
int expand[32] = 
	{
	5,   15,  25,  35,  45,  54,  62,  70,  78,  86,  93,  100, 106, 112, 118, 
	124, 131, 137, 143, 149, 155, 162, 169, 177, 185, 193, 201, 210, 220, 230, 
	240, 250, 
	};
#endif /* 5-bit compresssion */

/*
 * The compression routine has the hard job here.  It has to detect when
 * a silence run has started, and when it is over.  It does this by keeping
 * up and coming bytes in a look ahead buffer.  The buffer along with the
 * current index is passed ahead to routines that check to see if a run
 * has started, or if it has ended.
 */

//void CompressFile( FILE *input, BIT_FILE *output, int argc, char *argv[] )
void CompressFile( FILE *input, BIT_FILE *output, int, char*[])
{
    int look_ahead[ BUFFER_SIZE ];
    int index;
    int i;
    int run_length;

	//Update so Silence and Compand do not conflict
    int silence_match[6] = { 31, 0, 31, 0, 30, 0 };

	// Compand addition
    int compress[ 256 ];
    int steps;
    int bits;
    int value;
    int k;
    int j;

	bits = 5;
    steps = ( 1 << ( bits - 1 ) );

  //  OutputBits( output, (unsigned long) bits, 8 );
  //  OutputBits( output, (unsigned long) get_file_length( input ), 32 );

    for ( k = steps ; k > 0; k-- ) {
        value = (int)
           ( 128.0 * ( pow( 2.0, (double) k  /  steps ) - 1.0 ) + 0.5 );
        for ( j = value ; j > 0 ; j-- ) {
            compress[ j + 127 ] = k + steps - 1;
            compress[ 128 - j ] = steps - k;
        }
    }

    for ( i = 0 ; i < BUFFER_SIZE ; i++ )
        look_ahead[ i ] = getc( input );
    index = 0;
    for ( ; ; ) 
		{
        if ( look_ahead[ index ] == EOF )
            break;
		/*
		 * If a run has started, I handle it here.   I sit in the do loop until
		 * the run is complete, loading new characters all the while.
		 */
        if ( silence_run( look_ahead, index ) ) 
			{
            run_length = 0;

            do {
                look_ahead[ index++ ] = getc( input );
                index &= BUFFER_MASK;
                if ( ++run_length == 255 ) 
					{
					for( i=0; i<6; i++ )
						{
						OutputBits( output, (unsigned long) silence_match[i], bits );
						}
					OutputBits( output, (unsigned long) run_length, 8 );
					}
            } while ( !end_of_silence( look_ahead, index ) );
            if ( run_length > 0 ) 
				{
				for( i=0; i<6;i++ )
					{
					OutputBits( output, (unsigned long) silence_match[i], bits );
					}
				OutputBits( output, (unsigned long) run_length, 8 );
				}
			}
		/*
		 * Eventually, any run of silence is over, and I output some plain codes.
		 * Any code that accidentally matches the silence code gets silently changed.
		 */
		OutputBits( output, (unsigned long) compress[ look_ahead[ index ] ], bits );

        look_ahead[ index++ ] = getc( input );
        index &= BUFFER_MASK;
		}

	}

/*
 * The expansion routine used here has a very easy time of it.  It just
 * has to check for the run code, and when it finds it, pad out the
 * output file with some silence bytes.
 */
//void ExpandFile( BIT_FILE *input, FILE *output, int argc, char *argv[] )
void ExpandFile( BIT_FILE *input, FILE *output, int, char*[] )
{
    int		c;
    int		run_count;
    int		bits;
    long	count;

	//Update so Silence and Compand do not conflict
    int silence_match[6] = { 31, 0, 31, 0, 30, 0 };
	int inputBuff[6];
	int i;

	bits = (int) InputBits( input, 8 );

	count = InputBits( input, 32 );

    while ( ( c = (int) InputBits( input, bits ) ) != EOF ) 
		{
		if ( c == 31 )
			{
			inputBuff[0] = c;

			for(i = 1; i < 6; i++ )
				{
				inputBuff[i] = (int) InputBits( input, bits );
				}

			if( memcmp( &inputBuff, &silence_match, 6 ) )
				{
				for(i = 0; i < 6; i++ )
					{
					c = inputBuff[i];
					putc( expand[ inputBuff[i] ], output );
					}
				}
			else
				{
				run_count = (int) InputBits( input, 8 );
				while ( run_count-- > 0 )
					putc( 0x80, output );
				}
			}
		else
			putc( expand[ c ], output );
		}

}

unsigned char *ExpandBuffer( compression_buf_t *compBuf, long	*actSize )
	{
	unsigned char	*ptr,
					*ouputBuffer;
    int				bits,
     				c;
    long			count;
    int				run_count;

	//Update so Silence and Compand do not conflict
    int silence_match[6] = { 31, 0, 31, 0, 30, 0 };
	int inputBuff[6];
	int i;

	bits = 5;//(int) InputCompBits( compBuf, 8 );

	count = *actSize = 0x2800;//InputCompBits( compBuf, 32 );//387576
//	ptr = ouputBuffer = ( unsigned char * )malloc( *actSize );
	ptr = ouputBuffer = new unsigned char[*actSize];
	memset( ptr, 0x80, *actSize );

    while ( count > 0 ) 
		{
		c = (int) InputCompBits( compBuf, bits );

		if ( c == 31 )
			{
			inputBuff[0] = c;

			for(i = 1; i < 6; i++ )
				{
				inputBuff[i] = (int) InputCompBits( compBuf, bits );
				}

			if( memcmp( &inputBuff, &silence_match, 6 ) )
				{
				for(i = 0; i < 6; i++ )
					{
					*ptr = ( unsigned char )expand[ inputBuff[i] ];
					ptr++;
					count--;
					}
				}
			else
				{
				run_count = (int) InputCompBits( compBuf, 8 );
				memset( ptr, 0x80, run_count );
				ptr+=run_count;
				count-=run_count;
				run_count = 0;
				}
			}
		else
			{
			*ptr = ( unsigned char )expand[ c ];
			ptr++;
			count--;
			}
		}

	return( ouputBuffer );
	}
/*
long ExpandCompressionBuffer( long	bytesRead, compression_buf_t *compBuf, unsigned char *buffer )
	{
	unsigned char	*ptr;
    int				bits,
     				c;
    long			count;
    int				run_count;

	//Update so Silence and Compand do not conflict
    int silence_match[6] = { 31, 0, 31, 0, 30, 0 };
	int inputBuff[6];
	int i;

	bits = 5;

	count = bytesRead;
	ptr = buffer;
	memset( ptr, 0x80, count );

    while ( count > 0 ) 
		{
		c = (int) InputCompBits( compBuf, bits );

		if ( c == 31 )
			{
			inputBuff[0] = c;

			for(i = 1; i < 6; i++ )
				{
				inputBuff[i] = (int) InputCompBits( compBuf, bits );
				}

			if( memcmp( &inputBuff, &silence_match, 6 ) )
				{
				for(i = 0; i < 6; i++ )
					{
					*ptr = ( unsigned char )expand[ inputBuff[i] ];
					ptr++;
					count--;
					}
				}
			else
				{
				run_count = (int) InputCompBits( compBuf, 8 );
				memset( ptr, 0x80, run_count );
				ptr+=run_count;
				count-=run_count;
				run_count = 0;
				}
			}
		else
			{
			*ptr = ( unsigned char )expand[ c ];
			ptr++;
			count--;
			}
		}
	return( count );
	}
*/
long ExpandCompressionBuffer( long	bytesToRead, compression_buf_t *compbuf, unsigned char **buffer )
	{
	unsigned char	*ptr;
    int				bits = COMPRESSION_BITS,
     				c;
    long			count;
    long			bytesLeft;
    long			byteMax;
	//Update so Silence and Compand do not conflict
    int				silence_match[6] = { 31, 0, 31, 0, 30, 0 };
	int				inputBuff[6];
	int				i;
    short			silence_used;
    static short	silence_run = 0;


    if (compbuf->fileLength && (compbuf->fileLength == compbuf->bytesRead)) {
        return 0L;
    }

    ptr = *buffer;
    bytesLeft = compbuf->fileLength - compbuf->bytesRead;

    /* Calculate how much of the file we can read */
    if (bytesToRead <= bytesLeft)
        byteMax = bytesToRead;
    else
        byteMax = bytesLeft;
    
    count = 0;

    if (silence_run > 0) 
		{
		memset( ptr, SILENCE_KEY, silence_run );
		ptr += silence_run;
        count = count + silence_run;
        silence_run = 0;
		}

    while (count < byteMax) 
		{
		c = (int) InputCompBits( compbuf, bits );

		if ( c == 31 )
			{
			inputBuff[0] = c;

			for(i = 1; i < 6; i++ )
				{
				inputBuff[i] = (int) InputCompBits( compbuf, bits );
				}

			if( memcmp( &inputBuff, &silence_match, 6 ) )
				{
				for(i = 0; i < 6; i++ )
					{
					c = inputBuff[i];
					*ptr = ( unsigned char )expand[ inputBuff[i] ];
					ptr++;
					count++;
					}
				}
			else
				{
				silence_run = (short) InputCompBits( compbuf, 8 );

				silence_run += 1;
                if (silence_run + count > byteMax) {
                    silence_used = (short) (byteMax - count);
                    silence_run = (short) ((silence_run + count) - byteMax);
                }
                else {
                    silence_used = silence_run;
                    silence_run = 0;
                }

				memset( ptr, 0x80, silence_used );
				ptr+=silence_used;
				count+=silence_used;
				}
			}
		else
			{
			*ptr = ( unsigned char )expand[ c ];
			ptr++;
			count++;
			}
		}

    compbuf->bytesRead = compbuf->bytesRead + byteMax;

    if (compbuf->fileLength == compbuf->bytesRead) 
        return bytesLeft;
    else 
        return bytesToRead;
	}
/*
long ExpandCompressionBuffer( long	bytesToRead, compression_buf_t *compbuf, unsigned char **buffer )
	{
	unsigned char	*ptr;
    int				bits = COMPRESSION_BITS,
     				c;
    long			count;
    long			bytesLeft;
    long			byteMax;
	//Update so Silence and Compand do not conflict
    int				silence_match[6] = { 31, 0, 31, 0, 30, 0 };
	int				inputBuff[6];
	int				i;
    int				silence_used;
    static int		silence_run = 0;


    if (compbuf->fileLength && (compbuf->fileLength == compbuf->bytesRead)) 
		{
        return 0L;
		}

    ptr = *buffer;
    bytesLeft = compbuf->fileLength - compbuf->bytesRead;

    if (bytesToRead <= bytesLeft)
        byteMax = bytesToRead;
    else
        byteMax = bytesLeft;
    
    count = 0;

    if (silence_run > 0) 
		{
		memset( ptr, SILENCE_KEY, silence_run );
		ptr += silence_run;
        count = count + silence_run;
        silence_run = 0;
		}

    while (count < byteMax) 
		{
		c = (int) InputCompBits( compbuf, bits );

		if ( c == 31 )
			{
			inputBuff[0] = c;

			for(i = 1; i < 6; i++ )
				{
				inputBuff[i] = (int) InputCompBits( compbuf, bits );
				}

			if( memcmp( &inputBuff, &silence_match, 6 ) )
				{
				for(i = 0; i < 6; i++ )
					{
					c = inputBuff[i];
					*ptr = ( unsigned char )expand[ inputBuff[i] ];
					ptr++;
					count++;
					}
				}
			else
				{
				silence_run = (int) InputCompBits( compbuf, 8 );
			//	silence_run += 1;
				            
                if (silence_run + count > (int)byteMax) {
                    silence_used = (int)byteMax - count;
                    silence_run = (silence_run + count) - (int)byteMax;
                }
                else {
                    silence_used = silence_run;
                    silence_run = 0;
                }

				memset( ptr, 0x80, silence_used );
				ptr+=silence_used;
				count+=silence_used;
				}
			}
		else
			{
			*ptr = ( unsigned char )expand[ c ];
			ptr++;
			count++;
			}
		}

    compbuf->bytesRead = compbuf->bytesRead + byteMax;

    if (compbuf->fileLength == compbuf->bytesRead) 
        return bytesLeft;
    else 
        return bytesToRead;
	}
*/
long ExpandCompSilenceBuff( long	bytesRead, compression_buf_t *compBuf, VOICE_STREAM_BUFFER *voice )
	{
	unsigned char	*ptr;
    int				bits,
     				c;
    long			count;
    int				run_count;

    int silence_match[6] = { 31, 0, 31, 0, 30, 0 };
	int inputBuff[6];
	int i;

	bits = 5;

	count = bytesRead;
	ptr = voice->waveBuffer;
	memset( ptr, 0x80, count );

    while ( count > 0 ) 
		{
		c = (int) InputCompBits( compBuf, bits );

		if ( c == 31 )
			{
			inputBuff[0] = c;

			for(i = 1; i < 6; i++ )
				{
				inputBuff[i] = (int) InputCompBits( compBuf, bits );
				}

			if( memcmp( &inputBuff, &silence_match, 6 ) )
				{
				for(i = 0; i < 6; i++ )
					{
					*ptr = ( unsigned char )expand[ inputBuff[i] ];
					ptr++;
					count--;
					}
				}
			else
				{
				run_count = (int) InputCompBits( compBuf, 8 );
				memset( ptr, 0x80, run_count );
				ptr+=run_count;
//				count-=run_count;
				run_count = 0;
				}
			}
		else
			{
			*ptr = ( unsigned char )expand[ c ];
			ptr++;
			count--;
			}
		}

//	voice->waveBufferRead += bytesRead;
	voice->dataInWaveBuffer = ptr - voice->waveBuffer;//bytesRead;
	voice->waveBufferLen = ptr - voice->waveBuffer;//bytesRead;
//	voice->fill_level = ptr;
	return( count );
	}

/*
 * This support routine checks to see if the look ahead buffer contains
 * the start of a run, which by definition is START_THRESHOLD consecutive
 * silence characters.
 */

int silence_run( int buffer[], int index )
{
    int i;

    for ( i = 0 ; i < START_THRESHOLD ; i++ )
        if ( !IS_SILENCE( buffer[ ( index + i ) & BUFFER_MASK ] ) )
            return( 0 );
    return( 1 );
}

/*
 * This support routine is called while we are in the middle of a run of
 * silence.  It checks to see if we have reached the end of the run.
 * By definition this occurs when we see STOP_THRESHOLD consecutive
 * non-silence characters.
 */

int end_of_silence( int buffer[], int index )
{
    int i;

    for ( i = 0 ; i < STOP_THRESHOLD ; i++ )
        if ( IS_SILENCE( buffer[ ( index + i ) & BUFFER_MASK ] ) )
            return( 0 );
    return( 1 );
}


/*************************** End of SILENCE.C **************************/
