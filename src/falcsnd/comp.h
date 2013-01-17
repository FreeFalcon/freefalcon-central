#ifndef	_VOICE_H_
#define _VOICE_H_

unsigned char *ExpandCompandBuffer( compression_buf_t *compBuf, long	*actSize );
long ReadLZSSFile(unsigned long bytesToRead, BIT_FILE *input, unsigned char **buffer, unsigned char **fill_level);
long get_file_length( FILE *file );
unsigned char *ExpandBuffer( compression_buf_t *compBuf, long	*actSize );
long ExpandCompressionBuffer( long	bytesToRead, compression_buf_t *compbuf, unsigned char **buffer );
//long ExpandCompSilenceBuff( long	bytesRead, compression_buf_t *compBuf, AUDIO_STREAM_BUFFER *voice );
void CompressFile( FILE *input, BIT_FILE *output, int argc, char *argv[] );
void ExpandFile( BIT_FILE *input, FILE *output, int argc, char *argv[] );
void CompressCompandFile( FILE *input, BIT_FILE *output, int argc, char *argv[] );
void ExpandCompandFile( BIT_FILE *input, FILE *output, int argc, char *argv[] );
unsigned long CompressLZSSFile( FILE *input, BIT_FILE *output );
void ExpandLZSSFile( BIT_FILE *input, FILE *output );

#endif