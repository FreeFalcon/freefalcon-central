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
 * ATRAN.H
 *
 * AMD3D 3D library code: 3D Transformation
 *
 *	BETA RELEASE
 *
 *****************************************************************************/

#ifndef _AMD_ATRAN_H
#define _AMD_ATRAN_H

#ifdef __cplusplus
extern "C" {
#endif
 
void _trans_v1x4(float *res, float *mtx, float *pt);
void _trans_v4x1(float *res, float *mtx, float *pt);
void _trans_v1x3(float *res, float *mtx, float *pt);
void _trans_v3x1(float *res, float *mtx, float *pt);
void _trans_va3(float *res, float *mtx, float *pt, int npt);
void _trans_va3r(float *res, float *mtx, float *pt, int npt);
void _trans_va4(float *res, float *mtx, float *pt, int npt);
void _trans_va4r(float *res, float *mtx, float *pt, int npt);
void _jpeg_fdct(float *);

#ifdef __cplusplus
}
#endif

#endif

/* eof - ATRANS.H */
