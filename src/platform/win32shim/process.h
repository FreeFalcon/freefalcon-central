// Artscout - 2026 (#104, Linux Ф1): <process.h> shim -- MSVC _beginthread(ex)/_endthread over the shim's
// CreateThread (pthreads). Returns a HANDLE-compatible value so WaitForSingleObject/CloseHandle work on it.
#ifndef FF_WIN32SHIM_PROCESS_H
#define FF_WIN32SHIM_PROCESS_H
#include <windows.h>

typedef unsigned(__stdcall* _beginthreadex_proc_type)(void*);
static inline uintptr_t _beginthreadex(void* sec, unsigned stack,
                                       _beginthreadex_proc_type start,
                                       void* arg, unsigned flags,
                                       unsigned* thrdaddr)
{
    DWORD tid = 0;
    HANDLE h = CreateThread(sec, (SIZE_T)stack, (LPTHREAD_START_ROUTINE)start,
                            arg, flags, &tid);
    if (thrdaddr)
        *thrdaddr = tid;
    return (uintptr_t)h;
}
static inline uintptr_t _beginthread(void (*start)(void*), unsigned stack,
                                     void* arg)
{
    // wrap the void-returning start in a DWORD-returning trampoline via CreateThread
    return (uintptr_t)CreateThread(
        nullptr, (SIZE_T)stack, (LPTHREAD_START_ROUTINE)start, arg, 0, nullptr);
}
static inline void _endthreadex(unsigned)
{
    pthread_exit(nullptr);
}
static inline void _endthread(void)
{
    pthread_exit(nullptr);
}

#endif
