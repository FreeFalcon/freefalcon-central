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
 * DSP_LIB.C
 *
 * AMD3D 3D library code: Digital Signal Processing
 *
 *	BETA RELEASE
 *
 *****************************************************************************/

#ifdef _MSC_VER
// We don't need EMMS instruction warnings, we use FEMMS instead
#pragma warning(disable:4799)
#endif

#include <amath.h>
#include <adsp.h>
#include <amd3dx.h>

#define ITER 2
#define EPS  1.E-9f
#define PI	3.141592653589

static float zero[2] = {0.0f,0.0f};


float _rms (int n, float *v)
{
	float res;
	__asm {
		FEMMS
		push	ecx
		mov		eax,v
		mov		ecx,n
		call	a_rms
		pop		ecx
		movd	res,mm0
		FEMMS
	}
	return res;
}

float _average (int n, float *v)
{
	float res;
	__asm {
		FEMMS
		push	ecx
		mov		eax,v
		mov		ecx,n
		call	a_average
		pop		ecx
		movd	res,mm0
		FEMMS
	}
	return res;
}

void _v_mult (int n, float *input1, float *input2, float *output)
{
	__asm {
		FEMMS
		push	ecx
		push	ebx
		mov		ecx,n
		mov		eax,input1
		mov		ebx,input2
		mov		edi,output
		call	a_v_mult
		pop		ebx
		pop		ecx
		FEMMS
	}
}

void _minmax(int n, float *input, float *output)
{
	__asm {
		FEMMS
		push	ecx
		mov		ecx,n
		mov		eax,input
		call	a_minmax
		mov		eax,output
		pop		ecx
		movq	eax,mm0
		FEMMS
	}
}


float _bessel (float v)
{
	float res;
	__asm {
		FEMMS
		movd	mm0,v
		call	a_bessel
		movd	res,mm0
		FEMMS
	}
	return res;
}

/*	_a_iir - Compute the output of an infinite impulse response filter
 *      input   - the current sample
 *      n       - the number of coefficients (must be even)
 *      coef    - the filter coefficients (must have 4*n+1 elements)
 *      history - previous output values (must have 2*n elements), modified by call
 *      return  - the filtered sample value
 */
float _iir(float input,int n,float *coef,float *history)
{
	int j, h;
	float tmp[2];

	h = (n+1)/2; n = 2*h;
	coef += n-2;
	tmp[1] = *history;
	tmp[0] = input;
	__asm {
		FEMMS;
		movq	mm1, tmp;
		movq	mm0, zero;
		mov		eax, coef;
		mov		edx, history;
	}

	for(j=0; j<h; j++) {
		__asm {
			movq	mm2, [eax];
			sub		eax, 0x8;	

			punpckldq	mm3, mm2;
			punpckhdq 	mm2, mm3;

			movq	mm3, mm1;
			pfmul	(mm1,mm2)
			pfadd	(mm0,mm1)

			movq	mm1,[edx+4]
			movq	[edx],mm3
			add		edx,8
		}
	}

	__asm {
		pfacc	(mm0,mm0)
		movd	tmp, mm0;
		FEMMS;
	}
	return tmp[0];
}


/* _fftInit - initialize a FFT
 *      m	    -
 *      v       - values
 *      forward - direction of the transformation
 */
void _fftInit(int m, COMPLEX *w, int forward)
{
	const unsigned long negate[2] = { 0L, 0x80000000 };
	COMPLEX *xj;
	int n, le, i, j;
	float wr[2], wrx[2], wrm[2];
	double arg;

	n = 1 << m;
	le = n/2;

	xj = w;		
	arg = (forward*PI/(float)le);

	__asm femms
	_sincos((float) arg, wr);
	wrx[1] = wr[0];
	wrx[0] = -wr[1];
	wrm[0] = wr[0];
	wrm[1] = wr[1];

	for (i=0; i < le; i += ITER) {
//		_sincos((float) arg*(i+1), wr);
//		wr[1] *= -1;
//		__asm movq	mm0,wr

		__asm {
			mov		eax,i
			movd	mm0,arg
			add		eax,1
			pi2fd	(mm1,eax)	// really pi2fd mm1,[eax]
			pfmul	(mm0,mm1)
			call	a_sincos
			movq	mm1,negate	// Just the sign bit of the [1] value
			pxor	mm0,mm1		// Negate the number (very fast)
			mov		eax,xj;		
			movq	[eax],mm0;
			add		eax,8;
			movq	mm1,wrx;
			mov		xj,eax;
			movq	mm0,wrm;
		}
		for (j = 1; j < ITER; j++) {
			__asm {
				mov		eax,xj
				movq	mm3,[eax-8]
				movq	mm4,mm3
				pfmul	(mm3,mm0) 
				pfmul	(mm4,mm1) 
				pfacc	(mm3,mm3)	// real
				pfacc	(mm4,mm4)	// imag
				punpckldq	mm3,mm4
				movq	[eax],mm3

				add		eax,8
				mov		xj,eax
			}
		}
	} 
	__asm femms;
}


/* _fft - perform a FFT
 *      m       -
 *      v       -
 *      x       -
 *      forward - direction of transformation
 */
void _fft(int m, COMPLEX *w, COMPLEX *x, int forward)
{
	static int n = 1;           

	COMPLEX u,ux;
	COMPLEX *xi,*xip, *xj, *wptr;
	float pm[] = {+1,-1};
	int i,j,k,l,le,windex;

	n = 1 << m;
	le = n;
	windex = 1;
	__asm {
		FEMMS;
		movq	mm6,pm;
	}
	for (l = 0 ; l < m ; l++)
	{
		le = le/2;

		for(i = 0 ; i < n ; i = i + 2*le)
		{
			xi = x + i;
			xip = xi + le;

			__asm {
				mov		eax,xi
				mov		edi,xip
				movq	mm0,[eax]
				movq	mm1,[edi]
				movq	mm2,mm0
				pfadd	(mm0,mm1)
				pfsub	(mm2,mm1)
				movq	[eax],mm0
				movq	[edi],mm2
			}
		}

		wptr = w + windex - 1;
		for (j = 1 ; j < le ; j++) 
		{
			u = *wptr;
			ux.real = wptr->imag;
			ux.imag = wptr->real;	
			__asm movq	mm7,ux;
			for (i = j ; i < n ; i = i + 2*le) 
			{
				xi = x + i;
				xip = xi + le;
				__asm {
					mov		eax,xi
					mov		edi,xip
					movq	mm0,[eax]
					movq	mm1,[edi]
					movq	mm2,mm0
					pfadd	(mm0,mm1)
					pfsub	(mm2,mm1)
					movq	mm3,u
					movq	[eax],mm0
					movq	mm4,mm3
					pfmul	(mm3,mm2)
					pfmul	(mm3,mm6)
					pfacc	(mm3,mm3)
					movd	[edi],mm3

					pfmul	(mm2,mm7)
					pfacc	(mm2,mm2)
					movd	[edi+4],mm2
				}
			}
			wptr = wptr + windex;
		}
		windex <<= 1;
	}            

	j = 0;
	for (i = 1 ; i < (n-1) ; i++) 
	{
		k = n/2;
		while(k <= j) 
		{
			j = j - k;
			k = k/2;
		}
		j = j + k;
		if (i < j) {
			xi = x + i;
			xj = x + j;
			__asm {
				mov		eax,xi;
				movq	mm0,[eax];
				mov		ecx,xj;
				movq	mm1,[ecx];
				movq	[ecx],mm0;
				movq	[eax],mm1;
			}
		}
	}

	__asm femms;

	if(forward == 1) {
		for(i=0; i<n; i++) {
			x[i].real /= (float)n;
			x[i].imag /= (float)n;
		}
	}
}



float _fir(float input,int n,float *coef,float *history)
{
	int j, h;
	float tmp[2];

	h = (n+1)/2; n = 2*h;
	coef += n-2;
	tmp[1] = *history;
	tmp[0] = input;
	__asm {
		FEMMS;
		movq	mm1, tmp;
		movq	mm0, zero;
		mov		eax, coef;
		mov		ebx, history;
	}

	for(j=0; j<h; j++) {
		__asm {
			movq	mm2,[eax]
			sub		eax,8

			punpckldq	mm3,mm2
			punpckhdq 	mm2,mm3

			movq	mm3, mm1
			pfmul	(mm1,mm2)
			pfadd	(mm0,mm1)

			movq	mm1,[ebx+4]
			movq	[ebx],mm3
			add		ebx,8
		}
	}

	__asm {
		pfacc	(mm0,mm0)
		movd	tmp,mm0
		FEMMS
	}
	return tmp[0];
}


void _firBlock(int n, float *input, float *output, int m, float *coef)
{
	int i, j, h;
	float *tmpy = input, *tmpx = coef+m-2;

	h = (m+1)/2;
	m = 2*h;

	__asm {
		FEMMS;
		movq	mm0, zero;
	}
	for(i=0; i < h; i++) {
		__asm {
			mov		eax, output;
			movq	[eax], mm0;
			add		eax, 0x8;
			mov		output, eax;
		}
	}

	for (i=m; i<n; i++, tmpy++) {
		input = tmpy;
		coef = tmpx;
		__asm {
			movq	mm0, zero;
		}

		for(j=0; j<h; j++) {
			__asm {
				mov		eax,input
				mov		ebx,coef
				movq	mm1,[eax]
				movq	mm2,[ebx]

				punpckldq	mm3,mm2
				punpckhdq 	mm2,mm3

				add		eax,8
				pfmul	(mm1,mm2)
				sub		ebx,8
				pfadd	(mm0,mm1)
				mov		input,eax
				mov		coef,ebx
			}
		}

		__asm {
			pfacc	(mm0,mm0)
			mov		eax,output
			movd	[eax],mm0
			add		eax,4
			mov		output,eax
		}
	}
	__asm femms;
}


void _convolve(int n, float *input1, float *input2, float *output) 
{
	int i, j;	
	float *tmpy = input1, *tmpx = input2-n/2;
	float tmp;

	if(n & 1) {
		n++;
	}
	for(i = 0; i < n/2; i++) {
		tmp = input2[i];
		input2[i] = input2[n/2-i-1];
		input2[n/2-i-1] = tmp;
	}

	__asm femms;
	for(i=0; i<n; i++, tmpx++) {
		input1 = tmpy;
		input2 = tmpx;
		__asm {
			movq	mm0, zero;
		}

		for(j=0; j<n/4; j++) {
			__asm {
				mov		eax,input1
				mov		ebx,input2
				movq	mm1,[eax]
				movq	mm2,[ebx]
				add		eax,8
				add		ebx,8
				pfmul	(mm1,mm2)
				mov		input1,eax
				mov		input2,ebx
				pfadd	(mm0,mm1)
			}
		}

		__asm {
			pfacc	(mm0,mm0)
			mov		eax,output
			movd	[eax],mm0
			add		eax,4
			mov		output,eax
		}
	}
	__asm femms;
}

// eof