/***************************************************************************\
    Context.h
    Miro "Jammer" Torrielli
    06Oct03

 - Begin Major Rewrite
\***************************************************************************/
#ifndef _3DEJ_DEFINE_H_
#define _3DEJ_DEFINE_H_

#include "../../codelib//include/shi/shierror.h"
#include "d3d7compat.h"

#ifdef USE_SMART_HEAP
#include <stdlib.h>
#include "smartheap/include/smrtheap.hpp"
#endif


typedef void GLvoid;
typedef signed char GLchar;
typedef unsigned char GLuchar;
typedef signed short GLshort;
typedef unsigned short GLushort;
typedef signed int GLint;
typedef unsigned int GLuint;
// Artscout - 2026 (Linux port): GL 32-bit integer types. The width must be 32-bit on both platforms (a palette is
// 256 4-byte entries), but the TYPE must also stay assignment-compatible with DWORD, because palette pointers are
// passed around as both GLulong* and DWORD* (e.g. cpkneeview.cpp). On Win32 DWORD is `unsigned long` (32-bit), so
// GLulong must be `unsigned long` there -- `unsigned int` is the same width but a DIFFERENT type, and `unsigned
// int*` will not convert to `DWORD*` (C2440). Under LP64-Linux `long` is 64-bit, so there GLulong must be `unsigned
// int` to stay 32-bit; the Linux DWORD shim is uint32_t, so they match. Hence the split.
#ifdef _WIN32
typedef signed long GLlong;
typedef unsigned long
    GLulong; // == DWORD on Win32 (32-bit); keeps GLulong* assignable to DWORD*
#else
typedef signed int
    GLlong; // 32-bit under LP64 (Linux `long` is 64-bit; DWORD shim is uint32_t == unsigned int)
typedef unsigned int GLulong;
#endif
typedef float GLfloat;
typedef double GLdouble;
typedef signed char GLbyte;
typedef unsigned char GLubyte;
typedef signed int GLFixed0_14;

// color depth
#define COLOR_256 0 // 256 color
#define COLOR_32K 1 // 32K color --  0rrrrrgggggbbbbb
#define COLOR_64K 2 // 64K color --  rrrrrggggggbbbbb
#define COLOR_16M 3 // 16M color --  8r8g8b

// image type
#define IMAGE_TYPE_UNKNOWN -1
#define IMAGE_TYPE_GIF 1
#define IMAGE_TYPE_LBM 2
#define IMAGE_TYPE_PCX 3
#define IMAGE_TYPE_BMP 4
#define IMAGE_TYPE_APL 5
#define IMAGE_TYPE_TGA 6
#define IMAGE_TYPE_DDS 7

struct GLImageInfo
{
    GLint width;
    GLint height;
    GLulong *palette;
    GLubyte *image;
    DDSURFACEDESC2 ddsd;
};

#endif
