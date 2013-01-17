#ifndef _3DEJ_INLINE_H_
#define _3DEJ_INLINE_H_

//___________________________________________________________________________

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "mathlib\math.h"
#include "shi\ConvFtoI.h"
#include "define.h"

#ifdef USE_SH_POOLS
extern MEM_POOL glMemPool;
#endif

//___________________________________________________________________________

// default is clear memory allocated
inline void *glAllocateMemory (int totalbytes, GLint clearit=1)
{
#ifdef USE_SH_POOLS
	char *buf = (char *)MemAllocPtr( glMemPool, totalbytes, 0 );
#else
	char *buf = new char[totalbytes];
#endif
	if (buf && clearit) memset (buf, 0, totalbytes);
	return buf;
}

inline void glReleaseMemory (void *memptr)
{
#ifdef USE_SH_POOLS
	if (memptr)
	{
		MemFreePtr( memptr );
	}
#else
	if (memptr)
	{
		delete[] memptr;
	}
#endif
}


/*
+---------------------------------------------------------------------------+
|    glConvertToRadian                                                      |
+---------------------------------------------------------------------------+
|    Description:  Convert angle to radian representation                   |
|                                                                           |
|    Parameters:   angle (16384 = 360 degrees)                              |
|                                                                           |
|    Returns:      angle in radian                                          |
+---------------------------------------------------------------------------+
|    Programmed by Erick Jap                           November 29, 1993    |
+---------------------------------------------------------------------------+
*/
inline GLfloat glConvertToRadian (GLFixed0_14 deg)
{
	return (deg * (GLfloat) 0.000383495197f);
}	/* glConvertToRadian */

/*
+---------------------------------------------------------------------------+
|    glConvertToDegree                                                      |
+---------------------------------------------------------------------------+
|    Description:  Convert angle (0-16384) to degree (0-360) representation |
|                                                                           |
|    Parameters:   angle (16384 = 360 degrees)                              |
|                                                                           |
|    Returns:      angle in degree (0-360)                                  |
+---------------------------------------------------------------------------+
|    Programmed by Erick Jap                           November 29, 1993    |
+---------------------------------------------------------------------------+
*/
inline GLfloat glConvertToDegree (GLFixed0_14 deg)
{
	return (deg * 0.021972656f);
}	/* glConvertToDegree */

/*
+---------------------------------------------------------------------------+
|    glConvertFromDegree                                                    |
+---------------------------------------------------------------------------+
|    Description:  Convert degree (0-360) to angle (0-16384) representation |
|                                                                           |
|    Parameters:   angle in degree (0-360)                                  |
|                                                                           |
|    Returns:      angle (16384 = 360 degrees)                              |
+---------------------------------------------------------------------------+
|    Programmed by Erick Jap                           November 29, 1993    |
+---------------------------------------------------------------------------+
*/
inline GLFixed0_14 glConvertFromDegree (GLfloat deg)
{
	return ( FloatToInt32(deg * 45.511111111f + 0.5f) );
}	/* glConvertFromDegree */

inline GLfloat glConvertFromDegreef (GLfloat deg)
{
	return (deg * 45.511111111f + 0.5f);
}	/* glConvertFromDegreef */

/*
+---------------------------------------------------------------------------+
|    glConvertFromRadian                                                    |
+---------------------------------------------------------------------------+
|    Description:  Convert to angle from radian representation              |
|                                                                           |
|    Parameters:   angle in radian                                          |
|                                                                           |
|    Returns:      angle (16384 = 360 degrees)                              |
+---------------------------------------------------------------------------+
|    Programmed by Erick Jap                           November 29, 1993    |
+---------------------------------------------------------------------------+
*/
inline GLFixed0_14 glConvertFromRadian (GLfloat deg)
{
	return ( FloatToInt32(deg * 2607.594588f + 0.5f) );
}	/* glConvertFromRadian */



inline void glGetFileExtension (const char *file, char *ext)
{
#if 0
	char	prevchar, currchar, nextchar;

	prevchar = '.';
	while (*file != 0) {
		currchar = *file++;
		nextchar = *file;
		if (currchar == '.' && 
			((prevchar != '.' && prevchar != '\\') || 
			 (nextchar != '.' && nextchar != '\\'))) break;
		prevchar = currchar;
	}
#else


// Visual C/C++ 2010 migration fix
//pmvstrm this the change
	const char *lastdot;

//pmvstrm this is the original
//char *lastdot;

	lastdot = strrchr(file, '.');

	if(lastdot) {
		file = lastdot + 1;
	} else {
		file = file + strlen(file);
	}
#endif

	while (*file) *ext++ = *file++;
	*ext = 0;
}

//___________________________________________________________________________

#endif
