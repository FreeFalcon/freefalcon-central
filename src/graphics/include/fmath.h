/***************************************************************************\
    fmath.h
    Miro "Jammer" Torrielli
    29Dec03

 Fast Math
\***************************************************************************/

#ifndef _FMATH_H_
#define _FMATH_H_

// Artscout - 2026 (x64): the x87 inline asm below is x86-only. On x64 use the standard
// <math.h> functions (the compiler emits SSE scalar ops); identical results, builds on both.
#include <math.h>

inline float Sin(float a)
{
#if defined(_M_IX86)
    _asm
    {
        fld a;
        fsin;
        fstp a;
    }

    return a;
#else
    return sinf(a);
#endif
}

inline float Cos(float a)
{
#if defined(_M_IX86)
    _asm
    {
        fld a;
        fcos;
        fstp a;
    }

    return a;
#else
    return cosf(a);
#endif
}

inline float FabsF(float f)
{
#if defined(_M_IX86)
    _asm
    {
        fld f;
        fabs;
        fstp f;
    }

    return f;
#else
    return fabsf(f);
#endif
}

inline float SqrtF(float f)
{
#if defined(_M_IX86)
    _asm
    {
        fld f;
        fsqrt;
        fstp f;
    }

    return f;
#else
    return sqrtf(f);
#endif
}

inline float Tan(const float a)
{
#if defined(_M_IX86)
    float r = a;
    _asm
    {
        fld r;
        fptan;
        fstp r;
    }

    return r;
#else
    return tanf(a);
#endif
}

inline float Atan(const float o, float a)
{
#if defined(_M_IX86)
    _asm
    {
        fld o;
        fld a;
        fpatan;
        fstp a;
    }

    return a;
#else
    return atan2f(o, a);
#endif
}
/*
inline void SinCos(const float a, float *s, float *c)
{
 _asm
 {
 push edx;
 push ebx;
 mov edx,s;
 fld a;
 fsincos;
 mov ebx,c;
 fstp [dword ptr ebx];
 fstp [dword ptr edx];
 pop ebx;
 pop edx;
 }
}
*/
#endif // _FMATH_H_
