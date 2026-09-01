// Artscout - 2026 (#104, Linux Ф1): Win32 timing -> POSIX clock_gettime. Header-only (inline), no .cpp needed.
// Covers the grep surface: GetTickCount, timeGetTime, QueryPerformanceCounter/Frequency, GetSystemTime.
#ifndef FF_WIN32SHIM_TIME_H
#define FF_WIN32SHIM_TIME_H
#include <ctime>
#include <sys/time.h>

// Monotonic milliseconds since an arbitrary origin -- exactly GetTickCount's contract (used only for deltas).
static inline DWORD GetTickCount(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (DWORD)((uint64_t)ts.tv_sec * 1000ull +
                   (uint64_t)ts.tv_nsec / 1000000ull);
}
static inline unsigned long long GetTickCount64(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000ull + (uint64_t)ts.tv_nsec / 1000000ull;
}
// winmm timeGetTime is the same monotonic-ms clock for this codebase's purposes.
static inline DWORD timeGetTime(void)
{
    return GetTickCount();
}

// High-resolution counter: nanoseconds, so QPF is 1e9. The code only ever forms ratios (counter/frequency), so any
// consistent (counter, frequency) pair is correct; nanoseconds keeps full precision.
static inline BOOL QueryPerformanceCounter(LARGE_INTEGER* p)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    if (p)
        p->QuadPart = (LONGLONG)((uint64_t)ts.tv_sec * 1000000000ull +
                                 (uint64_t)ts.tv_nsec);
    return TRUE;
}
static inline BOOL QueryPerformanceFrequency(LARGE_INTEGER* p)
{
    if (p)
        p->QuadPart = 1000000000LL; // nanoseconds
    return TRUE;
}

static inline void GetSystemTime(SYSTEMTIME* st)
{
    if (!st)
        return;
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    struct tm tmv;
    gmtime_r(&ts.tv_sec, &tmv);
    st->wYear = (WORD)(tmv.tm_year + 1900);
    st->wMonth = (WORD)(tmv.tm_mon + 1);
    st->wDayOfWeek = (WORD)tmv.tm_wday;
    st->wDay = (WORD)tmv.tm_mday;
    st->wHour = (WORD)tmv.tm_hour;
    st->wMinute = (WORD)tmv.tm_min;
    st->wSecond = (WORD)tmv.tm_sec;
    st->wMilliseconds = (WORD)(ts.tv_nsec / 1000000);
}
static inline void GetLocalTime(SYSTEMTIME* st)
{
    if (!st)
        return;
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    struct tm tmv;
    localtime_r(&ts.tv_sec, &tmv);
    st->wYear = (WORD)(tmv.tm_year + 1900);
    st->wMonth = (WORD)(tmv.tm_mon + 1);
    st->wDayOfWeek = (WORD)tmv.tm_wday;
    st->wDay = (WORD)tmv.tm_mday;
    st->wHour = (WORD)tmv.tm_hour;
    st->wMinute = (WORD)tmv.tm_min;
    st->wSecond = (WORD)tmv.tm_sec;
    st->wMilliseconds = (WORD)(ts.tv_nsec / 1000000);
}

// FILETIME: 100-nanosecond intervals since 1601-01-01 UTC. The Unix epoch (1970-01-01) is
// 11644473600 seconds later, i.e. 116444736000000000 in FILETIME units.
static inline unsigned long long ff_filetime_from_unix(long long sec, long nsec)
{
    return (unsigned long long)(sec + 11644473600LL) * 10000000ull +
           (unsigned long long)(nsec / 100);
}
static inline void GetSystemTimeAsFileTime(FILETIME* ft)
{
    if (!ft)
        return;
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    unsigned long long v = ff_filetime_from_unix(ts.tv_sec, ts.tv_nsec);
    ft->dwLowDateTime = (DWORD)(v & 0xFFFFFFFFull);
    ft->dwHighDateTime = (DWORD)(v >> 32);
}
// Convert a UTC FILETIME to local time by adding the current UTC offset.
static inline BOOL FileTimeToLocalFileTime(const FILETIME* in, FILETIME* out)
{
    if (!in || !out)
        return FALSE;
    unsigned long long v =
        ((unsigned long long)in->dwHighDateTime << 32) | in->dwLowDateTime;
    time_t unixSec = (time_t)(v / 10000000ull) - 11644473600LL;
    struct tm tmv;
    localtime_r(&unixSec, &tmv);
    v += (unsigned long long)((long long)tmv.tm_gmtoff * 10000000LL);
    out->dwLowDateTime = (DWORD)(v & 0xFFFFFFFFull);
    out->dwHighDateTime = (DWORD)(v >> 32);
    return TRUE;
}
// Pack a FILETIME into DOS date/time words (date: year-1980<<9|month<<5|day; time: hour<<11|min<<5|sec/2).
static inline BOOL FileTimeToDosDateTime(const FILETIME* in, WORD* fatDate,
                                         WORD* fatTime)
{
    if (!in)
        return FALSE;
    unsigned long long v =
        ((unsigned long long)in->dwHighDateTime << 32) | in->dwLowDateTime;
    time_t unixSec = (time_t)(v / 10000000ull) - 11644473600LL;
    struct tm tmv;
    gmtime_r(&unixSec, &tmv);
    int year = tmv.tm_year + 1900;
    if (year < 1980)
        year = 1980;
    if (fatDate)
        *fatDate = (WORD)(((year - 1980) << 9) | ((tmv.tm_mon + 1) << 5) |
                          tmv.tm_mday);
    if (fatTime)
        *fatTime =
            (WORD)((tmv.tm_hour << 11) | (tmv.tm_min << 5) | (tmv.tm_sec / 2));
    return TRUE;
}

#endif // FF_WIN32SHIM_TIME_H
