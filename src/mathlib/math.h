#ifndef FALCON_MATH_H
#define FALCON_MATH_H

/*
* math.h
*
* Author: Miro "Jammer" Torrielli
* sfr: adding some non win32 stuff to make it compile on linux. The implementation is not good though, specially ftoi functions.
*/

#include <math.h>
#include <stdlib.h>
#ifdef WIN32
#include <windows.h>
#else
#include <stdint.h>
#endif

#undef  PI
#define PI (3.1415926535897932384626433832795028841971693993751f)
#define PI_FRAC (PI / 180.0)
#define TINY (0.0000001)

#define MAXIMUM(a,b,c) ((a>b)?max(a,c):max(b,c))
#define MINIMUM(a,b,c) ((a<b)?min(a,c):min(b,c))

template<class T> inline T Abs(const T A)
{
    return (A >= (T)0) ? A : -A;
}

template<class T> inline T Sgn(const T A)
{
    return (A > 0) ? 1 : ((A < 0) ? -1 : 0);
}

template<class T> inline T Max(const T A, const T B)
{
    return (A >= B) ? A : B;
}

template<class T> inline T Min(const T A, const T B)
{
    return (A <= B) ? A : B;
}

template<class T> inline T Square(const T A)
{
    return A * A;
}

//template<class T> inline T Clamp( const T X, const T Min, const T Max )
//{
// return X<Min ? Min : X<Max ? X : Max;
//}

template<class T> inline T Align(const T Ptr, int Alignment)
{
#if WIN32
    return (T)(((DWORD)Ptr + Alignment - 1) & ~(Alignment - 1));
#else
    return (T)(((uint32_t)Ptr + Alignment - 1) & ~(Alignment - 1));
#endif
}

template<class T> inline void Exchange(T& A, T& B)
{
    const T Temp = A;
    A = B;
    B = Temp;
}

template< class T > T Lerp(T& A, T& B, float Alpha)
{
    return A + Alpha * (B - A);
}

#if _MSC_VER >= 1300

static inline float RsqrtSSE(float x)
{
    static int big = 0x7F7FFFFF;

    __asm
    {
        rsqrtss xmm0, x
        minss xmm0, [big]
        mulss xmm0, x
        movss x, xmm0
    }

    return x;
}

static inline float SqrtSSE(float x)
{
    __asm
    {
        sqrtss xmm0, x
        movss x, xmm0
    }

    return x;
}

#define sqrt SqrtSSE
#define sqrtf SqrtSSE

#else //_MSC_VER >= 1300

static inline float Rsqrt(float v)
{
    float v_half = v * .5f;
    long i = *(long *)&v;

    i = 0x5f3759df - (i >> 1);
    v = *(float *)&i;

    return v * (1.5f - v_half * v * v);
}

static inline float Sqrt(float x)
{
#if WIN32
    _asm
    {
        fld x;
        fsqrt;
        fstp x;
    }
#else
    x = sqrtf(x);
#endif
    return x;
}

#define sqrt Sqrt
#define sqrtf Sqrt

#endif //_MSC_VER >= 1300

static inline void SinCos(const float a, float *s, float *c)
{
#if WIN32
    _asm
    {
        push edx;
        push ebx;
        mov edx, s;
        fld a;
        fsincos;
        mov ebx, c;
        fstp [dword ptr ebx];
        fstp [dword ptr edx];
        pop ebx;
        pop edx;
    }
#else
    *s = sinf(a);
    *c = cosf(a);
#endif
}

const float L_2 = 0.693147180559945f;
static inline float Log2(float f)
{
    return logf(f) / L_2;
}

static inline bool Fequal(float f0, float f1, float tol)
{
    float f = f0 - f1;

    if ((f > (-tol)) && (f < tol)) return true;
    else                       return false;
}

static inline bool Fless(float f0, float f1, float tol)
{
    if ((f0 - f1) < tol) return true;
    else             return false;
}

static inline bool Fgreater(float f0, float f1, float tol)
{
    if ((f0 - f1) > tol) return true;
    else             return false;
}

#if WIN32
#pragma warning(disable : 4035) // Retro 29Apr2004 - suppress 'No return value' warning
#endif
static inline int FloatToInt32(float x)
{
#if WIN32
    __asm
    {
        fld dword ptr [x];
        fistp dword ptr [x];
        mov eax, dword ptr [x];
    }
#else
    return static_cast<int>(x);
#endif
}
#if WIN32
#pragma warning(default : 4035) // Retro 29Apr2004
#endif

static inline void FloatToInt32Store(int *a, float x)
{
#if WIN32
    __asm
    {
        fld dword ptr [x];
        mov eax, dword ptr[x];
        fistp dword ptr [eax];
    }
#else
    *a = FloatToInt32(x);
#endif
}

// Fast float to int conversion (always truncates) see http://www.stereopsis.com/FPU.html for a discussion.
static inline long Ftol(float val)
{
    double v = double(val) + (68719476736.0 * 1.5);
    return ((long*)&v)[0] >> 16;
}

static inline float Smooth(float newVal, float curVal, float maxChange)
{
    float diff = newVal - curVal;

    if (Abs(diff) > maxChange)
    {
        if (diff > 0.f)
        {
            curVal += maxChange;

            if (curVal > newVal)
            {
                curVal = newVal;
            }
        }
        else if (diff < 0.f)
        {
            curVal -= maxChange;

            if (curVal < newVal)
            {
                curVal = newVal;
            }
        }
    }
    else
    {
        curVal = newVal;
    }

    return curVal;
}

static inline float Clamp(float val, float lower, float upper)
{
    if (val < lower) return lower;
    else if (val > upper) return upper;
    else return val;
}

static inline float Clamp(float val)
{
    if (val < 0.f) return 0.f;
    else if (val > 1.f) return 1.f;
    else return val;
}

static inline float Rand()
{
    return float(rand()) / float(RAND_MAX);
}

static inline int Fchop(float f)
{
    // FIXME!
    return int(f);
}

static inline int Frnd(float f)
{
    return Fchop(f + 0.5f);
}

static inline float Lerp(float x, float y, float l)
{
    return x + l * (y - x);
}

static inline float Cot(float a)
{
    return cosf(a) / sinf(a);
}

#endif

