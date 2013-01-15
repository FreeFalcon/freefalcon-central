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
 * MATRX_LIB.C
 *
 * AMD3D 3D library code: Matrix math
 *
 *	BETA RELEASE
 *
 *	WARNING!  THE ROUTINES IN THIS FILE HAVE NOT BEEN TESTED!
 *
 *****************************************************************************/

#include <amatrix.h>

float det_3x3 (const float *m)
{
	return (m[0] * m[4] * m[8] +
			m[1] * m[5] * m[6] +
			m[2] * m[3] * m[7] -
			m[0] * m[5] * m[7] -
			m[1] * m[3] * m[8] -
			m[2] * m[4] * m[6]);
}

void inverse_3x3 (float *dst, const float *m)
{
	float tmp[9];
	const float dt = 1.0f / det_3x3 (m);

	// Compute to a temporary location in case dst == m
    tmp[0] =  (m[4] * m[8] - m[7] * m[5]) * dt;
    tmp[1] = -(m[1] * m[8] - m[7] * m[2]) * dt;
    tmp[2] =  (m[1] * m[5] - m[4] * m[2]) * dt;
    tmp[3] = -(m[3] * m[8] - m[6] * m[5]) * dt;
    tmp[4] =  (m[0] * m[8] - m[6] * m[2]) * dt;
    tmp[5] = -(m[0] * m[5] - m[3] * m[2]) * dt;
    tmp[6] =  (m[3] * m[7] - m[6] * m[4]) * dt;
    tmp[7] = -(m[0] * m[7] - m[6] * m[1]) * dt;
    tmp[8] =  (m[0] * m[4] - m[3] * m[1]) * dt;

	dst[0] = tmp[0];
	dst[1] = tmp[1];
	dst[2] = tmp[2];
	dst[3] = tmp[3];
	dst[4] = tmp[4];
	dst[5] = tmp[5];
	dst[6] = tmp[6];
	dst[7] = tmp[7];
	dst[8] = tmp[8];
}
