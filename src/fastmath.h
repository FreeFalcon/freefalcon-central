/** Fast Math Functions **/

// Artscout - 2026 (x64): x87 inline asm is x86-only; on x64 use standard <math.h> equivalents.
#include <math.h>

// Float to 32-bit integer
inline DWORD F_I32(float x)
{
#if defined(_M_IX86)
    DWORD r;

    _asm
    {
        fld x
        fistp r
    }

    return r;
#else
    return (DWORD)lrintf(x);   // round-to-nearest, like fistp
#endif
}

// Absolute value
inline float F_ABS(float x)
{
#if defined(_M_IX86)
    float r;

    _asm
    {
        fld x
        fabs
        fstp r
    }

    return r;
#else
    return fabsf(x);
#endif
}
