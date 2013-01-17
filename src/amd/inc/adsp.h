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
 * ADSP.H
 *
 * AMD3D 3D library code: Digital Signal Processing
 *
 *	BETA RELEASE
 *
 *****************************************************************************/

#ifndef _AMD_ADSP_H
#define _AMD_ADSP_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct X {
	float real;
	float imag;
} COMPLEX, *COMPLEX_PTR;

float	_iir(float input, int n, float *coef, float *history);
void	_fftInit(int, COMPLEX *, int);
void	_fft(int, COMPLEX *, COMPLEX *, int);
float	_average(int, float *);
float	_rms(int, float *);
void	_bias(int n, float *data, float bias);
void	_v_mult(int n, float *input1, float *input2, float *output);
void	_minmax(int n, float *input, float *output);
float	_bessel(float);
float	_fir(float input, int n, float *coef, float *history);
void	_firBlock(int n, float *input, float *output, int m, float *history);
void	_convolve(int n, float *input1, float *input2, float *output);

// fastcall routines
// (registers are in same order as corresponding parameters above)
void a_average();	// ecx * eax -> mm0
void a_rms();		// ecx * eax -> mm0
void a_v_mult();	// ecx * eax * ebx * edi -> ()
void a_minmax();	// ecx * eax -> mm0 (max|min)
void a_bessel();	// mm0 -> mm0

#ifdef __cplusplus
}
#endif

#endif

/* eof - ADSP.H */
