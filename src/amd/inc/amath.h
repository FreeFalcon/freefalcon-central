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
 * AMATH.H
 *
 * AMD3D 3D library code: Math routines
 *
 * BETA RELEASE
 *
 *****************************************************************************/

#ifndef _AMD_AMATH_H
#define _AMD_AMATH_H

#ifdef __cplusplus
extern "C" {
#endif

    float _atan(float);
    float _acos(float);
    float _asin(float);
    float   _log(float);
    float   _log10(float);
    void    _pow(float *, float *);
    float _exp(float);
    float   _cosh(float);
    float  _tanh(float);
    float   _sinh(float);
    float   _sqrt(float);
    float  _fabs(float);
    float   _ceil(float);
    float  _floor(float);
    float  _frexp(float, int *);
//  float _ldexp(float, int);
    float   _modf(float, float *);
    float _fmod(float, float);
    void _sincos(float, float *);
    float _sin(float);
    float _cos(float);
    float   _tan(float);

    // "fastcall" register called routines.
    void a_atan(); // mm0 -> mm0
    void a_acos(); // mm0 -> mm0
    void a_asin(); // mm0 -> mm0
    void a_log(); // mm0 -> mm0
    void a_log10(); // mm0 -> mm0
    void a_exp(); // mm0 -> mm0
    void a_cosh(); // mm0 -> mm0
    void  a_tanh(); // mm0 -> mm0
    void a_sinh(); // mm0 -> mm0
    void a_sqrt(); // mm0 -> mm0
    void  a_fabs(); // mm0 -> mm0
    void a_ceil(); // mm0 -> mm0
    void  a_floor(); // mm0 -> mm0
    void  a_frexp(); // mm0 -> mm0 (mantissa|exponent)
    void a_ldexp(); // mm0 * mm1 -> mm0
    void a_modf(); // mm0 -> mm0 (mod|rem)
    void a_fmod(); // mm0 * mm1 -> mm0
    void a_sincos(); // mm0 -> mm0 (cos|sin)
    void a_sin(); // mm0 -> mm0
    void a_cos(); // mm0 -> mm0
    void a_tan(); // mm0 -> mm0
#ifdef __cplusplus
}
#endif

#endif

/* eof - AMATH.H */
