// *************************************************************************
// * Cobra definitions & Options for Falcon4                               *
// * COBRA -RED -                                                          *
// *************************************************************************














#define	_USE_COBRA_SSE_

// ************************** MACROS **************************************
#define	FLOAT_FAST_MORE_THAN_ZERO(x)	(*((signed long*)&x)>0)
#include "xmmintrin.h"
/*
#define	FAST_FACE_FRONT_CHECK(poly)	__asm{
					mov eax, poly
					movups xmm0, poly\
					mulps	xmm0,	xmm7\
					movups	SSECc,	xmm0\
					addss	xmm0,SSECc[2]\
					addss	xmm0,SSECc[3]\
					addss	xmm0,SSECc[4]\
					comiss	xmm0,SSEZero\
					jl		_Exit_}
*/



