#ifndef _F4THREADS_H
#define _F4THREADS_H

/** @file f4thread.h falcon 4 thread and mutex API. */

/** maximum number of threads created using this API */
#define F4T_MAX_THREADS 20

/** thread API return type */
typedef enum {
	F4T_RET_OK     = 0,
	F4T_RET_ERROR  = -1
} tret_e;

/** thread priority */
typedef enum {
	F4T_PRI_IDLE      = 0x02,
	F4T_PRI_NORMAL    = 0x04,
	F4T_PRI_HIGH      = 0x08,
	F4T_PRI_REALTIME  = 0x10
} tpri_e;

// Thread Crontrol
typedef int F4THREADHANDLE;
typedef int (*threadf_t)(void *);
#ifdef __cplusplus
extern "C" {
#endif
	/** creates a thread with the given thread function and arguments. Thread can be created
	* suspended and will be given the passed priority.
	* @param in tf thread function pointer
	* @param in arg1 thread arguments
	* @param in createSuspended boolean telling if the thread should be suspended on creation
	* @parma in p thread priority.
	* @return opaque handle to the created thread, -1 on error.
	*/
	F4THREADHANDLE F4CreateThread(threadf_t tf, void* arg1, int createSuspended, tpri_e p);

	/** Makes the current thread wait for the thread to end its execution.
	* @param in t thread to wait for
	*/
	void F4JoinThread(F4THREADHANDLE t);
#ifdef __cplusplus
}
#endif


// Critical Sections
struct F4CSECTIONHANDLE;
#ifdef __cplusplus
extern "C" {
#endif
	/** creates a critical section object (mutex) */
	F4CSECTIONHANDLE* F4CreateCriticalSection(const char *name);

	/** destroys a critical section object */
	void F4DestroyCriticalSection(F4CSECTIONHANDLE* theSection);

	/** locks the critical section or sleeps until the lock is released */
	void F4EnterCriticalSection(F4CSECTIONHANDLE* theSection);

	/** leaves the critical section */
	void F4LeaveCriticalSection(F4CSECTIONHANDLE* theSection);

	/** returns if the thread has the critical section locked. */
	int F4CheckHasCriticalSection(F4CSECTIONHANDLE* theSection);
#ifdef __cplusplus
}
#endif

// Barriers
struct F4BARRIERHANDLE;
#ifdef __cplusplus
extern "C" {
#endif
	/** creates a barrier object.
	* @param in name barrier name
	* @param in count number of calls to wait to resume waiting threads; Must be bigger than 1.
	* @return barrier handle
	*/
	F4BARRIERHANDLE* F4CreateBarrier(const char *name, unsigned int count);

	/** destroys a barrier. */
	void F4DestroyBarrier(F4BARRIERHANDLE *b);

	/** waits for the barrier to reach its count, putting the thread to sleep meanwhile. */
	void F4WaitBarrier(F4BARRIERHANDLE *b);
#ifdef __cplusplus
}
#endif



// Processor
#ifdef __cplusplus
extern "C" {
#endif
	int F4GetNumProcessors();
	int F4SetThreadProcessor(F4THREADHANDLE theThread, int theProcessor);
#ifdef __cplusplus
}
#endif




// C++ useful classes
#ifdef __cplusplus
/** smartpointer class for locking mutex in object scope */
class F4ScopeLock{
public:
	/** creates object locking muytex */
	F4ScopeLock(F4CSECTIONHANDLE *mutex) : mutex(mutex){
		if (mutex != 0){
			F4EnterCriticalSection(mutex);
		}	
	}
	/** destroys object unlocking mutex */
	~F4ScopeLock(){
		if (mutex != 0){
			F4LeaveCriticalSection(mutex);
		}	
	}

private:
	/** no copy constructor */
	F4ScopeLock(const F4ScopeLock &);

	/** the mutex we are holding */
	F4CSECTIONHANDLE *mutex;
};
#endif


#endif

