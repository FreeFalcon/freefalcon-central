/*****************************************************************************
 *
 * Copyright (c) 1996-1999 ADVANCED MICRO DEVICES, INC. All Rights reserved.
 *
 * This software is unpublished and contains the trade secrets and
 * confidential proprietary information of AMD. Unless otherwise
 * provided in the Software Agreement associated herewith, it is
 * licensed in confidence "AS IS" and is not to be reproduced in
 * whole or part by any means except for backup. Use, duplication,
 * or disclosure by the Government is subject to the restrictions
 * in paragraph(b)(3)(B)of the Rights in Technical Data and
 * Computer Software clause in DFAR 52.227-7013(a)(Oct 1988).
 * Software owned by Advanced Micro Devices Inc., One AMD Place
 * P.O. Box 3453, Sunnyvale, CA 94088-3453.
 *
 *****************************************************************************
 *
 * VECT_LIB.C
 *
 * AMD3D 3D library code: Vector math
 *	The majority of these routines are in vect.asm - this file only
 *	provides a C wrapper for functions needing to return a float value.
 *
 *	BETA RELEASE
 *
 *****************************************************************************/

#include <amd3dx.h>
#include <avector.h>

#ifdef _MSC_VER
#pragma warning(disable:4799)
#endif

/* _mag_vect - find the magnitude of a vector
 *      a       - input vector
 *      return  - the magnitude of 'a'
 */
float _mag_vect (float *a)
{
	float r;
	__asm {
		femms
		mov			eax,a
		movq		mm0,[eax]
		movd		mm1,[eax+8]
		pfmul		(mm0,mm0)
		pfmul		(mm1,mm1)
		pfacc		(mm0,mm0)
		pfadd		(mm0,mm1)
		pfrsqrt		(mm1,mm0)
		movq		mm2,mm1
		pfmul		(mm1,mm1)
		pfrsqit1	(mm1,mm0)
		pfrcpit2	(mm1,mm2)
		pfmul		(mm0,mm1)
		movd		r,mm0
		femms
	}
	return r;
}


/* _dot_vect - compute the dot product of two vectors
 *      a       - input vector 1
 *      b       - input vector 2
 *      return  - the dot product
 */
float _dot_vect (float *a, float *b)
{
	float r;
	__asm {
		femms
		mov			eax,a
		mov			edx,b
		movq		mm0,[eax]
		movd		mm1,[eax+8]
		pfmul		(mm0,edx)
		pfacc		(mm0,mm0)
		pfmulm		(mm1,edx,0x8)
		pfadd		(mm0,mm1)
		movd		r,mm0
		femms
	}
	return r;
}

// eof