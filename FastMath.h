///////////////////////////////////////// FAST MATH FUNCTIONS ///////////////////////////////////////////
// RED - 2007

// Artscout - 2026 (x64): x87 inline asm is x86-only; on x64 use standard <math.h> equivalents.
#include <math.h>

// Float to Int32
inline	DWORD	F_I32(float x)
{	DWORD	r;
#if defined(_M_IX86)
	_asm{
			fld		x
			fistp	r
	}
	return r;
#else
	return (DWORD)lrintf(x);   // round-to-nearest, like fistp
#endif
}



// Absolute Value
inline	float	F_ABS(float x)
{
#if defined(_M_IX86)
	float r;

	_asm{
			fld		x
			fabs
			fstp	r
	}
	return r;
#else
	return fabsf(x);
#endif
}
