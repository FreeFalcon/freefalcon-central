/***************************************************************************\
    Tloader.cpp
    Scott Randolph
    August 24, 1995

    Asyncrhonus loader module.  This class is designed to be run on its
    own thread and respond to requests for data off the disk.
\***************************************************************************/
#include <io.h>
#include <fcntl.h>
#include <process.h>
#include "Loader.h"

#pragma warning(disable : 4127)
#pragma warning(disable : 4706)

// Provide the one and only terrain database object.  It will be up to the
// application to initialize and cleanup this object by calling Setup and Cleanup.
Loader	TheLoader;


#ifdef _DEBUG
#define LOADER_INSTRUMENT
#endif


#ifdef LOADER_INSTRUMENT
static	LoaderQ	lastActive;
static	DWORD	lastActiveCalledAt	= 0;
static	DWORD	lastSleptAt			= 0;
static	DWORD	lastEnquedAt		= 0;
static	DWORD	lastDequeueAt		= 0;
static	DWORD	lastCanceledAt		= 0;
static	int		forceWakeEvent		= 0;
#endif


void Loader::Setup( void )
{
	// Setup the starting state for the main loop
	head			      = NULL;
	tail			      = NULL;
	shutDown		      = FALSE;
	stopped			   = FALSE;
	paused			   = RUNNING;
	queueIsEmpty	   = TRUE;
   queueStatus       = QUEUE_FIFO;

#ifdef LOADER_INSTRUMENT
	memset( &lastActive, 0, sizeof(lastActive) );
	lastActiveCalledAt	= 0;
	lastSleptAt			= 0;
	lastEnquedAt		= 0;
	lastDequeueAt		= 0;
	lastCanceledAt		= 0;
	forceWakeEvent		= 0;
#endif


	// Create the synmchronization objects we'll need
	strcpy( WakeEventName, "LoaderWakeupCall" );
	WakeEventHandle = CreateEvent( NULL, FALSE, FALSE, WakeEventName );
	if ( !WakeEventHandle ) {
		ShiError( "Failed to create the loader wake up event" );
	} 
	InitializeCriticalSection( &cs_loaderQ );


	// Spawn the thread
	threadHandle = ( HANDLE ) _beginthreadex( NULL, 0, (unsigned int (__stdcall *)(void*))MainLoopWrapper, this, 0, ( unsigned * ) &threadID );	
	if ( !threadHandle ) {
		ShiError( "Failed to spawn loader" );
	} 
}


void Loader::Cleanup( void )
{
	DWORD retval;


	// Request that the loader shutdown
	EnterCriticalSection( &cs_loaderQ );
	shutDown = TRUE;
	LeaveCriticalSection( &cs_loaderQ );
	SetEvent( WakeEventHandle );

	// Wait for it to happen (5000 ms maximum)
	retval = WaitForSingleObject( threadHandle, 5000 );

	// While debugging, die if we failed to kill the thread.  Later, ignore it.
	ShiAssert( retval == WAIT_OBJECT_0 );

	// Close the handle to the thread so that it will be deallocated
	CloseHandle( threadHandle );

	// Release any entries remaining in the request queue
	while (GetNextRequest());

	// Release the sychronization objects we've been using
	CloseHandle( WakeEventHandle );
	DeleteCriticalSection( &cs_loaderQ );
}


// Dummy wrapper to get from C-Style Thread spawning back into C++ calling convention
DWORD Loader::MainLoopWrapper( LPVOID myself )
{
	return ( ((Loader*)myself)->MainLoop() );
}


// The actual main loader service loop.  Won't return until flagged to stop
DWORD Loader::MainLoop( void )
{
	LoaderQ	*Active;
	DWORD	retval;

	while (!shutDown) {

		// Wait for a request to be made (wake up and double check every 10 seconds just in case)
#ifdef LOADER_INSTRUMENT
		lastSleptAt = GetTickCount();
#endif
		retval = WaitForSingleObject( WakeEventHandle, 10000 );

#if 0
		// For debugging delayed block arrival
		if (retval == WAIT_TIMEOUT) {
			MessageBox( NULL, "Click OK to resume processing the request queue.", "Loader Idle", MB_OK );
		}
#endif

		// Process everything in our queue
		if (paused == RUNNING) {

			while ((Active = GetNextRequest()) && (!shutDown)) {
            // Check queue status
            if (queueStatus == QUEUE_FIFO) {
				
#ifdef LOADER_INSTRUMENT
				   lastActive			= *Active;
				   lastActiveCalledAt	= GetTickCount();
#endif
				   
				   // Make the callback to notify the requestor
				   // NOTE:  The callback is responsible for deleting the queue entry!
				   ShiAssert( (Active->callback != NULL) );
				   Active->callback( Active );
				   
   			} else if (queueStatus == QUEUE_SORTING) {
               SortLoaderQueue ();
            } else if (queueStatus == QUEUE_STORING) {
               ReplaceHeadEntry (Active);
            }
         }
			
			// We dropped out of the processing loop, so we're either shutting down, or
			// the queue is empty
			EnterCriticalSection( &cs_loaderQ );
			if (!head) {
				queueIsEmpty = TRUE;
			}
			LeaveCriticalSection( &cs_loaderQ );
		} else {
			// Note that we're truely paused now
			paused = PAUSED;
		}

	}

	
	// We've been asked to quit, so off we go!
	stopped = TRUE;

	return 0;
}


// NOTE:  This must only be called from inside the cs_loaderQ critical section
void Loader::Dequeue( LoaderQ *Old )
{
	ShiAssert( Old );

	if (Old->prev) {
		Old->prev->next = Old->next;
	} else {
		ShiAssert( head == Old );
		head = Old->next;
	}

	if (Old->next) {
		Old->next->prev = Old->prev;
	} else {
		ShiAssert( tail == Old );
		tail = Old->prev;
	}

#ifdef LOADER_INSTRUMENT
			lastDequeueAt	= GetTickCount();
#endif

	if (!head) {
		// Wake the main loop to ensure it sets the queueIsEmpty flag as appropriate
		SetEvent( WakeEventHandle );
	}
}


// NOTE:  This must only be called from inside the cs_loaderQ critical section
void Loader::Enqueue( LoaderQ *New )
{
	// This is debug code -- should go once I'm done testing.
#ifdef _DEBUG
	LoaderQ	*p = head;
	while (p) {

		// See if this is a duplicate
		if ((p->fileoffset	== New->fileoffset) &&
			(p->filename	== New->filename  ) &&
			(p->parameter	== New->parameter ) &&
			(p->callback	== New->callback  )) {

			return;
			ShiAssert( !"Caught trying to add duplicate request" );
		}

		p = p->next;
	}
#endif

	// Link the new queue entry to the end of the Q
	New->next = NULL;
	New->prev = tail;

	if (!tail) {
		head = New;
		queueIsEmpty = FALSE;
	} else {
		tail->next = New;
	}

	tail = New;

#ifdef LOADER_INSTRUMENT
	lastEnquedAt = GetTickCount();
#endif


	// Post the event to wake up the loader loop
	SetEvent( WakeEventHandle );
}


void Loader::EnqueueRequest( LoaderQ *New )
{
	EnterCriticalSection( &cs_loaderQ );

	// Link the new queue entry to the end of the Q
	if (!shutDown) {
    	Enqueue( New );
    }

	LeaveCriticalSection( &cs_loaderQ );
}


void Loader::ReplaceHeadEntry ( LoaderQ *New )
{
	EnterCriticalSection( &cs_loaderQ );

	// Link the new queue entry to the HEAD of the Q
   New->next = head;
   New->prev = NULL;

   if (head)
      head->prev = New;

   if (!tail)
      tail = New;

   LeaveCriticalSection( &cs_loaderQ );
}


LoaderQ* Loader::GetNextRequest( void )
{
	LoaderQ	*request;

	
	EnterCriticalSection( &cs_loaderQ );

 	// We always dequeue the head of the list
	request = head;
	if (head) {
		Dequeue( head );
	}

	LeaveCriticalSection( &cs_loaderQ );


	// return the requested entry
	return request;
}


// Cancel (if possible) the request to load data into the specified target buffer
BOOL Loader::CancelRequest( void(*callback)(LoaderQ*), void *parameter, char *filename, DWORD fileoffset )
{
	LoaderQ	*p;

	EnterCriticalSection( &cs_loaderQ );

	p = head;
	while (p) {

		// See if this is the one we want to cancel
		if ((p->filename	== filename  ) &&
			(p->fileoffset	== fileoffset) &&
			(p->parameter	== parameter ) &&
			(p->callback	== callback  )) {

			Dequeue( p );
			delete p;
			break;
		}

		p = p->next;
	}

#ifdef LOADER_INSTRUMENT
	lastCanceledAt = GetTickCount();
#endif

	LeaveCriticalSection( &cs_loaderQ );

	return (p != NULL);
}


void Loader::SetPause( BOOL state )
{
	if (state) {
		if (paused == RUNNING) {
			paused = PAUSING;
			SetEvent( WakeEventHandle );
		}
	} else {
		paused = RUNNING;
	}
}


// This call will block until the Loader queue becomes empty
void Loader::WaitForLoader( void )
{
	// Wait for the queue to become empty
	// CAUTION:  Could never happen if another thread keeps the loader busy
	while ( !queueIsEmpty ) {

#ifdef LOADER_INSTRUMENT
		if (forceWakeEvent > 0) {
			SetEvent( WakeEventHandle );
			forceWakeEvent--;
		}
#endif

		Sleep(100);
	}
}

// Sort the queue by file. This should help on CD/HD access times
void Loader::SortLoaderQueue (void)
{
}


// Tell the loader to store all requests and not act on them
void Loader::SetQueueStatusStoring (void)
{
}


// Tell the loader to sort all current requests, then let it load them
// Note, this will block requests to the loader.
void Loader::SetQueueStatusSorting (void)
{
}

