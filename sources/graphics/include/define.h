/***************************************************************************\
    Context.h
    Miro "Jammer" Torrielli
    06Oct03

	- Begin Major Rewrite
\***************************************************************************/
#ifndef	_3DEJ_DEFINE_H_
#define	_3DEJ_DEFINE_H_

#include "../../codelib//include/shi/SHIerror.h"
#include <ddraw.h>

#ifdef USE_SMART_HEAP
#include <stdlib.h>
#include "SmartHeap/Include/smrtheap.hpp"
#endif


typedef void			GLvoid;
typedef signed char		GLchar;
typedef unsigned char	GLuchar;
typedef signed short	GLshort;
typedef unsigned short	GLushort;
typedef signed int		GLint;
typedef unsigned int	GLuint;
typedef signed long		GLlong;
typedef unsigned long	GLulong;
typedef float			GLfloat;
typedef double			GLdouble;
typedef signed char		GLbyte;
typedef unsigned char	GLubyte;
typedef signed int		GLFixed0_14;

// color depth 
#define COLOR_256					0		// 256 color
#define COLOR_32K					1		// 32K color --  0rrrrrgggggbbbbb
#define COLOR_64K					2		// 64K color --  rrrrrggggggbbbbb
#define COLOR_16M					3		// 16M color --  8r8g8b

// image type
#define	IMAGE_TYPE_UNKNOWN					-1
#define	IMAGE_TYPE_GIF						1
#define	IMAGE_TYPE_LBM						2
#define	IMAGE_TYPE_PCX						3
#define	IMAGE_TYPE_BMP						4
#define	IMAGE_TYPE_APL						5
#define	IMAGE_TYPE_TGA						6
#define IMAGE_TYPE_DDS						7

struct GLImageInfo
{
	GLint	 		width;
	GLint	 		height;
	GLulong			*palette;
	GLubyte			*image;
	DDSURFACEDESC2	ddsd;
};

#endif
