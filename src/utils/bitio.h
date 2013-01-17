/************************** Start of BITIO.H *************************/

#ifndef _BITIO_H
#define _BITIO_H

#include <stdio.h>

typedef struct bit_file {
    FILE *file;
    unsigned char mask;
    int rack;
    long bytesRead;					/* uncompressed lzss bytes */
    int match_length;   
    int match_position;
    int current_position;
    long fileLength;				/* uncompressed file size, not actual file size */
    int pacifier_counter;			/* added for book bitio to work for file compressions */
} BIT_FILE;

/* 
 * Added the function which work from buffers instead of directly from files.
 * They are the same as below except they read from a buffer.
 */
typedef struct {
    unsigned char	*buf;			/* the compression buffer */
    unsigned char	*bufptr;		/* marker to move around the buffer */
    unsigned char	mask;
    int				rack;
	long			bufferSize;
    long			bytesRead;      /* bytesRead in relation to uncompressed file size */
    long			fileLength;     /* uncompressed file size, not actual file size */
	long			compFileLength; /* compressed file size */
    unsigned char	*fill_level;	/* last place in the buffer to be filled */
} compression_buf_t;

BIT_FILE     *OpenInputBitFile( char *name );
BIT_FILE     *OpenOutputBitFile( char *name );
void          OutputBit( BIT_FILE *bit_file, int bit );
void          OutputBits( BIT_FILE *bit_file,
                          unsigned long code, int count );
int           InputBit( BIT_FILE *bit_file );
unsigned long InputBits( BIT_FILE *bit_file, int bit_count );
void          CloseInputBitFile( BIT_FILE *bit_file );
void          CloseOutputBitFile( BIT_FILE *bit_file );
void          FilePrintBinary( FILE *file, unsigned int code, int bits );

/* Bitio on buffers */
BIT_FILE					*openTalkFile( char *name );
unsigned long				InputCompBits(compression_buf_t *compbuf, int bit_count);
void						initSegment( BIT_FILE *bit_file );
compression_buf_t			*FillCompressionBuffer( BIT_FILE *bit_file );

#endif  /* _BITIO_H */

/*************************** End of BITIO.H **************************/

