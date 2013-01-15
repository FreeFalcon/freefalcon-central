#include <string>
#include <vector>

#include <windows.h>
#include <process.h>
#include "f4thread.h"

using namespace std;

/** thread internal structure */
typedef struct {
   int inUse;
   HANDLE handle;
   DWORD id;
} F4THREAD_TYPE;

// static namespace
namespace {
	/** our array of threads */
	F4THREAD_TYPE F4Thread[F4T_MAX_THREADS] = {0};
}


F4THREADHANDLE F4CreateThread(threadf_t tf, void *args, int createSuspended, tpri_e p){
	int i;

	// Find unused thread slot
	for (i=0; i<F4T_MAX_THREADS; ++i){
		if (F4Thread[i].inUse == FALSE){
			break;
		}
	}
	if (i == F4T_MAX_THREADS){
		return F4T_RET_ERROR;
	}

	// create the thread
	if (!(F4Thread[i].handle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)tf, args, 0, &F4Thread[i].id))){
		return F4T_RET_ERROR;
	}

	if (F4Thread[i].handle){
		DWORD wp = 0;
		switch (p){
			case F4T_PRI_NORMAL:    wp = THREAD_PRIORITY_NORMAL; break;
			case F4T_PRI_HIGH:      wp = THREAD_PRIORITY_ABOVE_NORMAL; break;
			case F4T_PRI_REALTIME:  wp = THREAD_PRIORITY_TIME_CRITICAL; break;
			case F4T_PRI_IDLE:
			default:                wp = THREAD_PRIORITY_IDLE; break;
		}
		F4Thread[i].inUse = TRUE;
		SetThreadPriority(F4Thread[i].handle, wp);
	}

	return (i);
}

void F4JoinThread(F4THREADHANDLE t){
	if (t < 0 || t >= F4T_MAX_THREADS || !F4Thread[t].inUse){ return; }
	WaitForSingleObject(F4Thread, INFINITE);
	F4Thread[t].inUse = 0;
}


/** critical section internal structure */
struct F4CSECTIONHANDLE {
   CRITICAL_SECTION criticalSection;
   HANDLE owningThread;
   //PVOID owningThread;
   int count;
   string name;
#ifdef _DEBUG
   DWORD time;
#endif
};


F4CSECTIONHANDLE* F4CreateCriticalSection(const char *name){
	F4CSECTIONHANDLE* theSection;
	theSection = new F4CSECTIONHANDLE;

	if (theSection && (theSection != (F4CSECTIONHANDLE*)0xfeeefeee) && (theSection != (F4CSECTIONHANDLE*)0xbaadf00d)){
	//if (theSection){
		memset (&(theSection->criticalSection), 0, sizeof (CRITICAL_SECTION));
		InitializeCriticalSection (&(theSection->criticalSection));
		theSection->owningThread = (HANDLE)-1;
		theSection->count = 0;
		theSection->name = name;
	}
	else
		theSection = NULL;

	return (theSection);
}

void F4DestroyCriticalSection (F4CSECTIONHANDLE* theSection){
	if (theSection && (theSection != (F4CSECTIONHANDLE*)0xfeeefeee) && (theSection != (F4CSECTIONHANDLE*)0xbaadf00d)){
	//if (theSection){
		DeleteCriticalSection (&(theSection->criticalSection));
		delete theSection;
	}
	else
		theSection = NULL;
}

#include <stdio.h>
void F4EnterCriticalSection (F4CSECTIONHANDLE* theSection){
	if (theSection && (theSection != (F4CSECTIONHANDLE*)0xfeeefeee) && (theSection != (F4CSECTIONHANDLE*)0xbaadf00d)){
	//if (theSection){
		DWORD now = GetTickCount();
		EnterCriticalSection (&(theSection->criticalSection));
		DWORD aft = GetTickCount();
		if (aft - now > 200){
			printf("stutter");
		}
		theSection->count ++;
		theSection->owningThread = (HANDLE)GetCurrentThreadId();
	}
	else
		theSection = NULL;
}

BOOL F4TryEnterCriticalSection (F4CSECTIONHANDLE* theSection){
   HANDLE tid = (HANDLE)GetCurrentThreadId();

	if (theSection && (theSection != (F4CSECTIONHANDLE*)0xfeeefeee) && (theSection != (F4CSECTIONHANDLE*)0xbaadf00d)){
	//if (theSection){
      if ( (int)theSection->owningThread < 0 || theSection->owningThread == tid ){
      	 EnterCriticalSection (&(theSection->criticalSection));
      	 theSection->count ++;
      	 theSection->owningThread = tid;
		 return TRUE;
	  }
   }
	else
		theSection = NULL;

   return FALSE;
}

// JPO - utility routine to check that a routine that
// expects to be in critical section really is.
int F4CheckHasCriticalSection(F4CSECTIONHANDLE* theSection){
    HANDLE tid = (HANDLE)GetCurrentThreadId();
 	if (theSection && (theSection != (F4CSECTIONHANDLE*)0xfeeefeee) && (theSection != (F4CSECTIONHANDLE*)0xbaadf00d)){
   if (theSection && theSection->owningThread == tid && theSection->count > 0){
			return true;
		}
	}
	else
		theSection = NULL;

    return false;
}

void F4LeaveCriticalSection (F4CSECTIONHANDLE* theSection)
{
	if (theSection && (theSection != (F4CSECTIONHANDLE*)0xfeeefeee) && (theSection != (F4CSECTIONHANDLE*)0xbaadf00d)){
	//if (theSection){
      theSection->count --;
      if (theSection->count == 0){
         //F4Assert (theSection->owningThread == (HANDLE)GetCurrentThreadId());
         theSection->owningThread = (HANDLE)-2;
      }
      LeaveCriticalSection (&(theSection->criticalSection));
#ifdef _TIMEDEBUG
      static int interesttime = 100;
      DWORD endt = GetTickCount();
      int time = endt - theSection->time;
	  
      if (theSection->count == 0 &&time > interesttime) {
		  MonoPrint ("Has held critical section %s for %d\n", theSection->name, time);
      }
#endif

   }
	else
		theSection = NULL;
}


/** barrier internal structure */
struct F4BARRIERHANDLE {
	F4CSECTIONHANDLE *criticalSection; ///< barrier critical section
	vector<HANDLE> countReached;       ///< array of events to be notified when barrier reachs count
	vector<HANDLE> notifierGo;         ///< notifier can only notify if this is set
	unsigned int ccount;               ///< current count
	unsigned int rcount;               ///< count to reach to release all
};

F4BARRIERHANDLE* F4CreateBarrier(const char *name, unsigned int count){
	if (count < 2){ return NULL; }

	F4BARRIERHANDLE *b = new F4BARRIERHANDLE;
	b->countReached.resize(count-1);
	b->notifierGo.resize(count-1);
	for (unsigned int i=0;i<count-1;++i){
		b->countReached[i] = CreateEvent(NULL, FALSE, FALSE, name);
		// this one must be reset manually otherwise we can lose the event
		b->notifierGo[i] = CreateEvent(NULL, TRUE, FALSE, name);
	}
	b->criticalSection = F4CreateCriticalSection(name);
	b->ccount = 0;
	b->rcount = count;

	return b;
}

void F4DestroyBarrier(F4BARRIERHANDLE *b){
	for (unsigned int i=0;i<b->rcount-1;++i){
		CloseHandle(b->countReached[i]);
		CloseHandle(b->notifierGo[i]);
	}
	delete b;
}

void F4WaitBarrier(F4BARRIERHANDLE *b){
	F4EnterCriticalSection(b->criticalSection);
	if (++b->ccount == b->rcount){
		// count reached its limit, wake up everyone
		b->ccount = 0;
		// run all notifying
		for (unsigned int i=0;i<b->rcount-1;++i){
			// wait the other thread to be waiting
			WaitForSingleObject(b->notifierGo[i], INFINITE);
			// here the other thread is waiting, we can signal safely
			SetEvent(b->countReached[i]);
		}
		F4LeaveCriticalSection(b->criticalSection);
	}
	else {
		// get our wait index
		unsigned int idx = b->ccount-1;
		F4LeaveCriticalSection(b->criticalSection);
		// say notifier can go ahead (if its waiting) and wait its notification
	/*	SignalObjectAndWait(b->notifierGo[idx], b->countReached[idx], INFINITE, FALSE);*/
	}
}


int F4GetNumProcessors(){
	int i;
	DWORD pmask, smask;

   GetProcessAffinityMask(GetCurrentProcess(), &pmask, &smask);

	smask = 0;
	for (i=0; i<32; i++){
		if (pmask & (1 << i)){
			smask ++;
		}
	}

	return (smask);
}

int F4SetThreadProcessor (F4THREADHANDLE theThread, int theProcessor){
	if (SetThreadAffinityMask (F4Thread[theThread].handle, (DWORD)(1 << (theProcessor - 1)))){
		return (F4T_RET_OK);
	}
	else {
		return (F4T_RET_ERROR);
	}
}








#if 0
/* old stuff
int F4SuspendThread (F4THREADHANDLE theThread)
{
   //F4Assert( "Don't call F4SuspendThread silly!" == NULL );
	int retval = F4THREAD_ERROR;
	if (F4Thread[theThread].inUse){
		if (SuspendThread(F4Thread[theThread].handle) != 0xFFFFFFFF){
			retval = F4THREAD_OK;
		}
	}
	return (retval);
}

int F4ResumeThread (F4THREADHANDLE theThread)
{
	//F4Assert( "Don't call F4ResumeThread silly!" == NULL );
	int retval = F4THREAD_ERROR;
	if (F4Thread[theThread].inUse){
		if (ResumeThread(F4Thread[theThread].handle) != 0xFFFFFFFF){
			retval = F4THREAD_OK;
		}
	}
	return (retval);
}

int F4DestroyThread (F4THREADHANDLE theThread, Int32 exitCode)
{
	int retval = F4THREAD_ERROR;

	//   F4Assert (FALSE);
	if (F4Thread[theThread].inUse){
		if (TerminateThread(F4Thread[theThread].handle, exitCode)){
			F4Thread[theThread].inUse = FALSE;
			retval = F4THREAD_OK;
		}
	}
	return (retval);
}

int F4DestroyAllThreads (void)
{
	int i;
	int retval = F4THREAD_OK;

	for (i=0; i<F4MAX_THREADS; i++){
		if (F4Thread[i].inUse){
			if (TerminateThread(F4Thread[i].handle, 0)){
				F4Thread[i].inUse = FALSE;
			}
			else{
				retval = F4THREAD_OK;
			}
		}
	}

	return (retval);
}
*/
#endif
