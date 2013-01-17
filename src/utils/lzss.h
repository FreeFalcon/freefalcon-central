#ifndef LZSS_H
#define LZSS_H

#include "utils\LZSSopt.h"

#ifdef INCLUDE_FILE_COMPRESSION
#include "bitio.h"
#endif

// This is the maxmimum number (actually, just a very high guess) of additional
// bytes which may be added to an already compressed data stream. 
// I would recomend adding this to your output_string's length before calling either 
// Compress or Expand in order to prevent overwritting the string.
#define MAX_POSSIBLE_OVERWRITE    100

#ifdef C_LINKAGE
#if __cplusplus
extern "C"
{
#endif
#endif

extern int LZSS_Compress(unsigned char *input_string, unsigned char *output_string, int uncompSize);

//sfr: added srcSize
extern int LZSS_Expand(unsigned char *input_string, int srcSize, unsigned char *output_string, int uncompSize);

#ifdef INCLUDE_FILE_COMPRESSION

extern unsigned long LZSS_CompressFile( FILE *input, BIT_FILE *output );

extern void LZSS_ExpandFile( BIT_FILE *input, FILE *output );

extern unsigned long LZSS_ReadFile(unsigned long bytesToRead, BIT_FILE *input, unsigned char **buffer, unsigned char **fill_level);

#endif

#ifdef C_LINKAGE
#if __cplusplus
} // extern "C"
#endif
#endif

#endif
