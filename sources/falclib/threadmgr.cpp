#include <stdio.h>
#include <stdlib.h>
#include "ThreadMgr.h"
#include "debuggr.h"
#include "F4Thread.h"
#include "Falclib.h"

int ThreadManager::initialized = FALSE;
HANDLE ThreadManager::campaign_wait_event;
HANDLE ThreadManager::sim_wait_event;
ThreadInfo ThreadManager::campaign_thread;
ThreadInfo ThreadManager::sim_thread;

void ThreadManager::setup(){
	if (initialized){
        return;
	}

    initialized = TRUE;

	// security descriptor = NULL, manual reset = false, initial state = unsignaled, name
	campaign_wait_event = CreateEvent(NULL, FALSE, FALSE, "campaign_wait_event");
	sim_wait_event = CreateEvent(NULL, FALSE, FALSE, "sim_wait_event");
    memset (&campaign_thread, 0, sizeof (campaign_thread));
    memset (&sim_thread, 0, sizeof (sim_thread));
}

void ThreadManager::start_campaign_thread(UFUNCTION function)
{
    ShiAssert( campaign_thread.handle == NULL );
    
	campaign_thread.status |= THREAD_STATUS_ACTIVE;
    
	campaign_thread.handle = ( HANDLE ) CreateThread(
		NULL,
		0,
		(unsigned long (__stdcall *)(void*))function,
		0,
		0,
        &campaign_thread.id
	);

	ShiAssert (campaign_thread.handle);

	fast_campaign();
}

bool ThreadManager::campaign_wait_for_sim(DWORD maxwait){
#if !NEW_SYNC
	ResetEvent(campaign_wait_event);
#endif

	return WaitForSingleObject(campaign_wait_event, maxwait) != WAIT_TIMEOUT ? true : false;
}


void ThreadManager::sim_signal_campaign()
{
	SetEvent(campaign_wait_event);
}

bool ThreadManager::sim_wait_for_campaign(DWORD maxwait){
#if !NEW_SYNC
	ResetEvent (sim_wait_event);
#endif

	return WaitForSingleObject(sim_wait_event, maxwait) != WAIT_TIMEOUT ? true : false;
}


void ThreadManager::campaign_signal_sim(){
	SetEvent(sim_wait_event);
}

void ThreadManager::start_sim_thread(UFUNCTION function){
	ShiAssert (sim_thread.handle == NULL);
	
	sim_thread.status |= THREAD_STATUS_ACTIVE;

	sim_thread.handle = (HANDLE)CreateThread(
		NULL,
		0,
		(unsigned long (__stdcall *)(void*))function,
		0,
		0,
		&sim_thread.id
	);
	
	ShiAssert (sim_thread.handle);
}


void ThreadManager::stop_campaign_thread()
{
    ShiAssert (campaign_thread.handle);

    campaign_thread.status &= ~THREAD_STATUS_ACTIVE;

    WaitForSingleObject (campaign_thread.handle, INFINITE);

    CloseHandle (campaign_thread.handle);

    campaign_thread.handle = NULL;
}


void ThreadManager::stop_sim_thread(){
    ShiAssert (sim_thread.handle);

    sim_thread.status &= ~THREAD_STATUS_ACTIVE;

    WaitForSingleObject (sim_thread.handle, INFINITE);

    CloseHandle (sim_thread.handle);

    sim_thread.handle = NULL;
}


void ThreadManager::slow_campaign(){
	SetThreadPriority (campaign_thread.handle, THREAD_PRIORITY_IDLE);
}


void ThreadManager::fast_campaign(){
	SetThreadPriority (campaign_thread.handle, THREAD_PRIORITY_NORMAL);
}


void ThreadManager::very_fast_campaign(){
	SetThreadPriority (campaign_thread.handle, THREAD_PRIORITY_ABOVE_NORMAL);
}
