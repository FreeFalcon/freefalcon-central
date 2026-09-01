// Artscout - 2026 (#104, Linux Ф1): Win32 synchronisation/threads -> POSIX. The trivial primitives (CRITICAL_
// SECTION, Interlocked*, Sleep) are inline here; the handle-based ones (CreateThread, events, WaitForSingleObject)
// are declared here and implemented over pthreads + a HANDLE table in win32compat.cpp.
#ifndef FF_WIN32SHIM_SYNC_H
#define FF_WIN32SHIM_SYNC_H
#include <pthread.h>
#include <time.h>

// ---- CRITICAL_SECTION -> recursive pthread mutex (Win CS is recursive on the same thread) ----
typedef struct _CRITICAL_SECTION
{
    pthread_mutex_t m;
    int initialised;
} CRITICAL_SECTION, *LPCRITICAL_SECTION;

static inline void InitializeCriticalSection(CRITICAL_SECTION* cs)
{
    pthread_mutexattr_t a;
    pthread_mutexattr_init(&a);
    pthread_mutexattr_settype(&a, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&cs->m, &a);
    pthread_mutexattr_destroy(&a);
    cs->initialised = 1;
}
static inline BOOL InitializeCriticalSectionAndSpinCount(CRITICAL_SECTION* cs,
                                                         DWORD)
{
    InitializeCriticalSection(cs);
    return TRUE;
}
static inline void EnterCriticalSection(CRITICAL_SECTION* cs)
{
    pthread_mutex_lock(&cs->m);
}
static inline void LeaveCriticalSection(CRITICAL_SECTION* cs)
{
    pthread_mutex_unlock(&cs->m);
}
static inline BOOL TryEnterCriticalSection(CRITICAL_SECTION* cs)
{
    return pthread_mutex_trylock(&cs->m) == 0 ? TRUE : FALSE;
}
static inline void DeleteCriticalSection(CRITICAL_SECTION* cs)
{
    if (cs->initialised)
    {
        pthread_mutex_destroy(&cs->m);
        cs->initialised = 0;
    }
}

// ---- Interlocked -> GCC/clang atomics ----
static inline LONG InterlockedIncrement(volatile LONG* v)
{
    return __atomic_add_fetch(v, 1, __ATOMIC_SEQ_CST);
}
static inline LONG InterlockedDecrement(volatile LONG* v)
{
    return __atomic_sub_fetch(v, 1, __ATOMIC_SEQ_CST);
}
static inline LONG InterlockedExchange(volatile LONG* t, LONG val)
{
    return __atomic_exchange_n(t, val, __ATOMIC_SEQ_CST);
}
static inline LONG InterlockedExchangeAdd(volatile LONG* t, LONG val)
{
    return __atomic_fetch_add(t, val, __ATOMIC_SEQ_CST);
}
static inline LONG InterlockedCompareExchange(volatile LONG* t, LONG ex,
                                              LONG cmp)
{
    __atomic_compare_exchange_n(t, &cmp, ex, false, __ATOMIC_SEQ_CST,
                                __ATOMIC_SEQ_CST);
    return cmp;
}
static inline void* InterlockedExchangePointer(void* volatile* t, void* val)
{
    return __atomic_exchange_n(t, val, __ATOMIC_SEQ_CST);
}

// ---- Sleep -> nanosleep ----
static inline void Sleep(DWORD ms)
{
    struct timespec ts;
    ts.tv_sec = (time_t)(ms / 1000);
    ts.tv_nsec = (long)(ms % 1000) * 1000000L;
    nanosleep(&ts, nullptr);
}
static inline DWORD SleepEx(DWORD ms, BOOL)
{
    Sleep(ms);
    return 0;
}

// ---- handle-based waitables (thread / event / mutex): implemented in win32compat.cpp over pthreads ----
#define WAIT_OBJECT_0 0x00000000
#define WAIT_TIMEOUT 0x00000102
#define WAIT_FAILED 0xFFFFFFFF
#define WAIT_ABANDONED 0x00000080
#define MAXIMUM_WAIT_OBJECTS 64

// Artscout - 2026 (Linux port): thread procs are cast to `unsigned long (*)(void*)`
// throughout the source (matching real Windows where DWORD == unsigned long). The shim
// keeps DWORD as uint32_t for on-disk struct layout, so the thread-proc return type is
// spelled `unsigned long` explicitly here to match those casts.
typedef unsigned long(WINAPI* LPTHREAD_START_ROUTINE)(LPVOID);

extern "C"
{
    HANDLE CreateThread(void* attrs, SIZE_T stack, LPTHREAD_START_ROUTINE start,
                        LPVOID param, DWORD flags, DWORD* tid);
    DWORD WaitForSingleObject(HANDLE h, DWORD ms);
    DWORD WaitForMultipleObjects(DWORD count, const HANDLE* h, BOOL waitAll,
                                 DWORD ms);
    BOOL CloseHandle(HANDLE h);
    HANDLE CreateEventA(void* sec, BOOL manualReset, BOOL initialState,
                        LPCSTR name);
    BOOL SetEvent(HANDLE h);
    BOOL ResetEvent(HANDLE h);
    // Mutex: modelled as an auto-reset event -- WaitForSingleObject acquires (and auto-resets), ReleaseMutex signals.
    HANDLE CreateMutexA(void* sec, BOOL initialOwner, LPCSTR name);
    BOOL ReleaseMutex(HANDLE h);
    DWORD GetCurrentThreadId(void);
    DWORD GetCurrentProcessId(void);
    HANDLE GetCurrentThread(void);
    BOOL SetThreadPriority(HANDLE h, int pri);
    DWORD_PTR SetThreadAffinityMask(HANDLE h, DWORD_PTR mask);
}
#define CreateEvent CreateEventA
#define CreateMutex CreateMutexA
// DebugBreak: the Win32 debugger trap. In error paths on Linux, do nothing (don't SIGTRAP a release run).
static inline void DebugBreak(void)
{
}
#define THREAD_PRIORITY_NORMAL 0
#define THREAD_PRIORITY_ABOVE_NORMAL 1
#define THREAD_PRIORITY_BELOW_NORMAL (-1)
#define THREAD_PRIORITY_HIGHEST 2
#define THREAD_PRIORITY_TIME_CRITICAL 15

#endif // FF_WIN32SHIM_SYNC_H
