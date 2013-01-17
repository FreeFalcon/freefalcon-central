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
 * MATH_LIB.C
 *
 * AMD3D 3D library code: Math routines (wrap fastcall ASM routines in math.asm)
 *
 *	BETA RELEASE
 *
 *****************************************************************************/

#ifdef _MSC_VER
// We don't need EMMS instruction warnings
#pragma warning(disable:4799)
#endif

#include <amath.h>
#include <amd3dx.h>


/*---------------------------------------------------------*/
float _atan (float x)
{
	float res=0.0f;
	__asm
	{
		FEMMS
		movd	mm0,x
		call	a_atan
		movd	res,mm0
		FEMMS
	}
	return res;
}

/*---------------------------------------------------------*/
float _acos(float x)
{
	float fval;
		
	__asm
	{
		FEMMS
		movd	mm0,x
		push	ecx
		call	a_acos
		pop		ecx
		movd	fval,mm0
		FEMMS
	}
	return fval;
}



/*---------------------------------------------------------------*/
float	_asin(float x)
{
	float fval;
		
	__asm
	{
		FEMMS
		movd	mm0,x
		push	ecx
		call	a_asin
		pop		ecx
		movd	fval,mm0
		FEMMS
	}
	return fval;
}

	
/*---------------------------------------------------------*/
float _log(float x)
{
	float fval;
		
	__asm
	{
		FEMMS
		movd	mm0,x
		push	ecx
		call	a_log
		pop		ecx
		movd	fval,mm0
		FEMMS
	}
	return fval;
}


/*---------------------------------------------------------*/
float _log10(float x)
{
	float fval;
		
	__asm
	{
		FEMMS
		movd	mm0,x
		push	ecx
		call	a_log10
		pop		ecx
		movd	fval,mm0
		FEMMS
	}
	return fval;
}

/*---------------------------------------------------------*/
float _cosh(float x)
{
	float fval;
		
	__asm
	{
		FEMMS
		movd	mm0,x
		push	ecx
		call	a_cosh
		pop		ecx
		movd	fval,mm0
		FEMMS
	}
	return fval;
}

/*---------------------------------------------------------*/
float _sinh(float x)
{
	float fval;
		
	__asm
	{
		FEMMS
		movd	mm0,x
		push	ecx
		call	a_sinh
		pop		ecx
		movd	fval,mm0
		FEMMS
	}
	return fval;
}	

/*---------------------------------------------------------*/
float _tanh(float x)
{
	float fval;
		
	__asm
	{
		FEMMS
		movd	mm0,x
		push	ecx
		call	a_tanh
		pop		ecx
		movd	fval,mm0
		FEMMS
	}
	return fval;
}
	
		
/*---------------------------------------------------------*/
float _exp(float x)
{
	float fval;
		
	__asm
	{
		FEMMS
		movd	mm0,x
		push	ecx
		call	a_exp
		pop		ecx
		movd	fval,mm0
		FEMMS
	}
	return fval;
}	

/*---------------------------------------------------------*/
float _sqrt(float x)
{
	float fval;
		
	__asm
	{
		FEMMS
		movd	mm0,x
		call	a_sqrt
		movd	fval,mm0
		FEMMS
	}
	return fval;
}

/*---------------------------------------------------------*/
float _fabs (float x)
{
	float fval;
		
	__asm
	{
		mov		eax,x
		and		eax,0x7fffffff
		mov		fval,eax
	}
	return fval;
}
		
/*---------------------------------------------------------*/
float _ceil(float x)
{
	float fval;
		
	__asm
	{
		FEMMS
		movd	mm0,x
		call	a_ceil
		movd	fval,mm0
		FEMMS
	}
	return fval;
} 

/*---------------------------------------------------------*/
float _floor(float x)
{
	float fval;
		
	__asm
	{
		FEMMS
		movd	mm0,x
		call	a_floor
		movd	fval,mm0
		FEMMS
	}
	return fval;
}		
		
/*---------------------------------------------------------*/
float _frexp(float x, int *y)
{
	float fval[2];
		
	__asm
	{
		FEMMS
		movd	mm0,x
		call	a_frexp
		movq	fval,mm0
		FEMMS
	}
	*y = *(int *)&fval[1];
	return fval[0];
}

/*---------------------------------------------------------*/
float _ldexp(float x, int exp)
{
	float	res;
	__asm
	{
		FEMMS
		movd		mm0,x
		movd		mm1,exp
		push		ebx
		push		ecx
		call		a_ldexp
		pop 		ecx
		pop 		ebx
		movd		res,mm0
		FEMMS
	}
	return res;
}

/*---------------------------------------------------------*/
float _modf(float x, float *iptr)
{
	float res[2];
	__asm
	{
		FEMMS
		movd		mm0,x
		pf2id		(mm1,mm0)
		pi2fd		(mm1,mm1)
		pfsub		(mm0,mm1)	// (I+F) - I = F
		punpckldq	mm0,mm1		// mm0 = res:iptr
		movd		res,mm0
		FEMMS
	}
	*iptr = res[1];
	return res[0];
}

/*---------------------------------------------------------*/
float _fmod(float x, float y)
{
	float res;
	
	__asm {
		FEMMS
		movd		mm0,x
		movd		mm1,y
		call		a_fmod
		movd		res,mm0
		FEMMS
	}

	return res;
}	

/*---------------------------------------------------------*/
void _sincos(float x, float *v)
{
	__asm
	{
		FEMMS
		movd	mm0,x
		push	ebx
		push	ecx
		push	esi
		push	edi
		call	a_sincos
		mov		eax,v
		pop		edi
		pop		esi
		pop		ecx
		pop		ebx
		movq	qword ptr [eax],mm0
		FEMMS
	}
}

/*---------------------------------------------------------*/
float _sin(float x)
{
	float fval;
		
	__asm
	{
		FEMMS
		movd	mm0,x
		push	ebx
		push	ecx
		push	esi
		push	edi
		call	a_sin
		pop		edi
		pop		esi
		pop		ecx
		pop		ebx
		movd	fval,mm0
		FEMMS
	}
	return fval;
}

/*---------------------------------------------------------*/
float _cos(float x)
{
	float fval;
		
	__asm
	{
		FEMMS
		movd	mm0,x
		push	ebx
		push	ecx
		push	esi
		push	edi
		call	a_cos
		pop		edi
		pop		esi
		pop		ecx
		pop		ebx
		movd	fval,mm0
		FEMMS
	}
	return fval;
}

/*---------------------------------------------------------*/
float _tan(float x)
{
	float fval;
		
	__asm
	{
		FEMMS
		movd	mm0,x
		push	ebx
		push	ecx
		push	esi
		push	edi
		call	a_tan
		pop		edi
		pop		esi
		pop		ecx
		pop		ebx
		movd	fval,mm0
		FEMMS
	}
	return fval;
}


// eof