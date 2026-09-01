// Artscout - 2026 (#104, Linux Ф1): <intrin.h> shim -- MSVC compiler intrinsics -> clang/GCC builtins. SSE/AVX
// come from <x86intrin.h>; the MSVC-only ones (_BitScan*, __cpuid, __rdtsc, barriers, __debugbreak) are mapped.
//
// IMPORTANT: clang under -fms-compatibility ALREADY implements most MSVC intrinsics as real builtins, and DEFINING
// one of those is a hard error ("definition of builtin function '__cpuid'") -- which is what this header used to do,
// breaking every TU that reached it. So each definition below is gated on __has_builtin: supply only what the
// compiler lacks. Note #undef cannot help there -- a builtin is not a macro; the gate is the fix.
#ifndef FF_WIN32SHIM_INTRIN_H
#define FF_WIN32SHIM_INTRIN_H
#include <x86intrin.h>
#include <cpuid.h>
#include <csignal>
// <cpuid.h> defines these as 5-arg MACROS that would clash with the 2-arg MSVC signatures. Drop the macros; whether a
// *builtin* of that name then exists is what __has_builtin answers below.
#undef __cpuid
#undef __cpuidex

#ifndef __has_builtin
#define __has_builtin(x) 0
#endif

#if !__has_builtin(_BitScanForward)
static inline unsigned char _BitScanForward(unsigned long* idx,
                                            unsigned long mask)
{
    if (!mask)
        return 0;
    *idx = (unsigned long)__builtin_ctzl(mask);
    return 1;
}
#endif
#if !__has_builtin(_BitScanReverse)
static inline unsigned char _BitScanReverse(unsigned long* idx,
                                            unsigned long mask)
{
    if (!mask)
        return 0;
    *idx = (unsigned long)(31 - __builtin_clzl(mask));
    return 1;
}
#endif
#if !__has_builtin(_BitScanForward64)
static inline unsigned char _BitScanForward64(unsigned long* idx,
                                              unsigned long long mask)
{
    if (!mask)
        return 0;
    *idx = (unsigned long)__builtin_ctzll(mask);
    return 1;
}
#endif
#if !__has_builtin(_BitScanReverse64)
static inline unsigned char _BitScanReverse64(unsigned long* idx,
                                              unsigned long long mask)
{
    if (!mask)
        return 0;
    *idx = (unsigned long)(63 - __builtin_clzll(mask));
    return 1;
}
#endif
#if !__has_builtin(__cpuid)
static inline void __cpuid(int info[4], int leaf)
{
    unsigned a, b, c, d;
    __get_cpuid((unsigned)leaf, &a, &b, &c, &d);
    info[0] = a;
    info[1] = b;
    info[2] = c;
    info[3] = d;
}
#endif
#if !__has_builtin(__cpuidex)
static inline void __cpuidex(int info[4], int leaf, int sub)
{
    unsigned a, b, c, d;
    __get_cpuid_count((unsigned)leaf, (unsigned)sub, &a, &b, &c, &d);
    info[0] = a;
    info[1] = b;
    info[2] = c;
    info[3] = d;
}
#endif
#if !__has_builtin(__rdtsc)
static inline unsigned long long __rdtsc(void)
{
    return __builtin_ia32_rdtsc();
}
#endif
#if !__has_builtin(_ReadWriteBarrier)
static inline void _ReadWriteBarrier(void)
{
    __atomic_thread_fence(__ATOMIC_SEQ_CST);
}
#endif
#if !__has_builtin(_ReadBarrier)
static inline void _ReadBarrier(void)
{
    __atomic_thread_fence(__ATOMIC_ACQUIRE);
}
#endif
#if !__has_builtin(_WriteBarrier)
static inline void _WriteBarrier(void)
{
    __atomic_thread_fence(__ATOMIC_RELEASE);
}
#endif
static inline void _mm_pause_shim(void)
{
    __builtin_ia32_pause();
}
#if !__has_builtin(__debugbreak)
static inline void __debugbreak(void)
{
    ::raise(SIGTRAP);
}
#endif
#if !__has_builtin(_rotl)
static inline unsigned int _rotl(unsigned int v, int s)
{
    return (v << s) | (v >> (32 - s));
}
#endif
#if !__has_builtin(_rotr)
static inline unsigned int _rotr(unsigned int v, int s)
{
    return (v >> s) | (v << (32 - s));
}
#endif
#endif
