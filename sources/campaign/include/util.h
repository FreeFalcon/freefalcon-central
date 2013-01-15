
#ifndef _UTIL_H
#define _UTIL_H

#include <dos.h>
#include <stdio.h>


/*----------------------------------------------------------------------------
 * Defines for CPU Types, this is used in GameInfo but we'll probably
 * pass this about and do comparisons on these values but don't necessarily
 * have access or want access to the GameInfo Struct (like in sound.c)
 ---------------------------------------------------------------------------*/
#define CPU486          0
#define CPUPENTIUM      1

#define MONO_NORMAL  0x07                  
#define MONO_INTENSE 0x08
#define MONO_UNDER   0x01
#define MONO_REVERSE 0x70
#define MONO_BLINK   0x80

#define MONO_TEXT 0xB0000

#ifdef NDEBUG
#define MonoPrint(x)    ((void) 0)
#define MonoLocate(x, y) ((void) 0)
#define MonoCls() ((void) 0)
#define MonoScroll() ((void) 0)
#define MonoColor( attribute ) ((void) 0)
#else
#define MonoPrint(x)    MonPrint x
extern void MonPrint( const char *string, ... );
extern void MonoLocate( unsigned char x, unsigned char y );
extern void MonoCls( void );
extern void MonoScroll( void );
extern void MonoColor( char attribute );
#endif

#define cos(x) (((float) IGetCosine((short) (((x))*2607.5))) * 0.000000059604)
#define sin(x) (((float) IGetSine((short) (((x))*2607.5))) * 0.000000059604)

int UnzipFile(char *zipfile, char *filename, char **retbuf, long *size,
        int *zipID);
void FreeZipFile(int zipID, char *retbuf);
int OpenZipFile(char *zipfile, int *zipID);
void CloseZipFile(int zipID);
int UnzipMemFile(int zipID, char *filename, char **retbuf, long *size);
void ReturnBuffer(char *buf);
#define FreeMemFile ReturnBuffer
void GetUnzipInfo(int zipID);
void InitZipFile(void);
void ShutdownZipFile(void);
int OpenMemFile(int zipID, char *filename, FILE *fptr);
void CloseMemFile(FILE *fptr);
char *fgetsMem(char *linebuff, size_t n, FILE *fptr);
size_t freadMem (void *buffer, size_t elsize, size_t nelem, FILE *fptr);
int fseekMem(FILE *fptr, long offset, int where);
long ftellMem(FILE *fptr);
void rewindMem(FILE *fptr);
long flengthMem(FILE *fptr);
#if 0
int fscanfMem(FILE *fptr, const char *format, ...);
int fgetwordMem(FILE *fptr, char *bufptr);
#endif
#endif /* _UTIL_H */
