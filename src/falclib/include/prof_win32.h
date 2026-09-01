#ifndef Prof_INC_PROF_WIN32_H
#define Prof_INC_PROF_WIN32_H

#include <intrin.h>   // Artscout - 2026 (x64): __rdtsc intrinsic

typedef __int64 Prof_Int64;

#ifdef __cplusplus
inline
#elif _MSC_VER >= 1200
__forceinline
#else
static
#endif
    void Prof_get_timestamp(Prof_Int64 *result)
{
#if defined(_M_IX86)
    __asm
    {
        rdtsc;
        mov    ebx, result
        mov    [ebx], eax
        mov    [ebx+4], edx
    }
#else
    // Artscout - 2026 (x64): rdtsc asm is x86-only; use the __rdtsc intrinsic.
    *result = (Prof_Int64)__rdtsc();
#endif
}

#endif
