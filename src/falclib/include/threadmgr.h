#ifndef _THREAD_MANAGER_H_
#define _THREAD_MANAGER_H_

#include <iso646.h>
#include <windows.h>
#include <process.h>

extern "C" struct F4BARRIERHANDLE;

// sfr: new synchronization
#define NEW_SYNC 1

typedef unsigned(__stdcall *UFUNCTION)(void);

// Thread status
#define THREAD_STATUS_ACTIVE 1
#define THREAD_STATUS_FRAME_RUNNING 2

struct ThreadInfo
{
    HANDLE handle;
    DWORD status;
    // Artscout - 2026 (Linux port): thread id is a Win32 DWORD (32-bit); CreateThread's out-param is
    // DWORD*. `unsigned long` is 64-bit on LP64, so &id would not convert to DWORD*.
    DWORD id;
};

class ThreadManager
{
public:
    static void setup(void);

    static bool campaign_wait_for_sim(DWORD maxtime);
    static void sim_signal_campaign(void);

    static bool sim_wait_for_campaign(DWORD maxtime);
    static void campaign_signal_sim(void);

    static void start_campaign_thread(UFUNCTION function);
    static void start_sim_thread(UFUNCTION function);

    static void stop_campaign_thread(void);
    static void stop_sim_thread(void);

    static BOOL campaign_active(void)
    {
        return campaign_thread.status bitand THREAD_STATUS_ACTIVE;
    }

    static void slow_campaign(void);
    static void fast_campaign(void);
    static void very_fast_campaign(void);

private:
    static ThreadInfo campaign_thread, sim_thread;

    static HANDLE campaign_wait_event;
    static HANDLE sim_wait_event;

    static int initialized;
};

#endif // _THREAD_MANAGER_H_
