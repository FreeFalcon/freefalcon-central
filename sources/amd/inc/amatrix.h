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
 * AMATRIX.H
 *
 * AMD3D 3D library code: Maxtrix Manipulation
 *
 *	BETA RELEASE
 *
 *****************************************************************************/

#ifndef _AMD_AMATRIX_H
#define _AMD_AMATRIX_H

#ifdef __cplusplus
extern "C" {
#endif
 
void _add_m3x3(float *res, float *a, float *b);
void _add_m4x4(float *res, float *a, float *b);
void _sub_m3x3(float *res, float *a, float *b);
void _sub_m4x4(float *res, float *a, float *b);
void _mul_m3x3(float *res, float *a, float *b);
void _mul_m4x4(float *res, float *a, float *b);
void _add_m3x3s(float *res, float *a, float b);
void _add_m4x4s(float *res, float *a, float b);
void _sub_m3x3s(float *res, float *a, float b);
void _sub_m4x4s(float *res, float *a, float b);
void _mul_m3x3s(float *res, float *a, float b);
void _mul_m4x4s(float *res, float *a, float b);

#ifdef __cplusplus
}
#endif

#endif

/* eof - AMATRIX.H */
