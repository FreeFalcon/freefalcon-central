#ifndef Prof_INC_PROF_POSIX_H
#define Prof_INC_PROF_POSIX_H

// Artscout - 2026 (Linux port): POSIX equivalent of prof_win32.h.
// Provides the platform primitives prof_gather.h requires (Prof_Int64 and
// Prof_get_timestamp). On x86-64 we use the __rdtsc intrinsic just like the
// Windows path; elsewhere we fall back to clock_gettime(CLOCK_MONOTONIC).

#include <stdint.h>

typedef int64_t Prof_Int64;

#if defined(__x86_64__) || defined(__i386__)
#include <x86intrin.h>
#else
#include <time.h>
#endif

#ifdef __cplusplus
inline
#else
static
#endif
    void Prof_get_timestamp(Prof_Int64 *result)
{
#if defined(__x86_64__) || defined(__i386__)
    *result = (Prof_Int64)__rdtsc();
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    *result = (Prof_Int64)ts.tv_sec * 1000000000LL + (Prof_Int64)ts.tv_nsec;
#endif
}

#endif // Prof_INC_PROF_POSIX_H
