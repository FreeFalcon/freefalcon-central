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
 * AVECTOR.H
 *
 * AMD3D 3D library code: Vector Math
 *
 *      BETA RELEASE
 *
 *****************************************************************************/

#ifndef _AMD_AVECTOR_H
#define _AMD_AVECTOR_H

#ifdef __cplusplus
extern "C" {
#endif

void _add_vect(float *res, float *a, float *b);
void _add_vects(float *res, float *a, float b);
void _sub_vect(float *res, float *a, float *b);
void _sub_vects(float *res, float *a, float b);
void _mult_vect(float *res, float *a, float *b);
void _mult_vects(float *res, float *a, float b);
void _norm_vect(float *res, float *a);
float _mag_vect(float *a);
float _dot_vect(float *a, float *b);
void _cross_vect(float *res, float *a, float *b);

/* fastcall routines */
void a_mag_vect();      /* eax -> mm0		*/
void a_dot_vect();      /* eax * edx -> mm0	*/

#ifdef __cplusplus
}
#endif

#endif

/* eof - AVECTOR.H */
