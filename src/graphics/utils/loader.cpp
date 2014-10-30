/***************************************************************************
    Tloader.cpp
    Scott Randolph
    August 24, 1995

    Asynchronus loader module.  This class is designed to be run on its
    own thread and respond to requests for data off the disk.
***************************************************************************/
#include <cISO646>
#include <io.h>
#include <fcntl.h>
#include <process.h>
#include "Loader.h"
#include "ObjectLOD.h"
#include "TexBank.h"
#pragma warning(disable : 4127)
#pragma warning(disable : 4706)

// Provide the one and only terrain database object.  It will be up to the
// application to initialize and cleanup this object by calling Setup and Cleanup.
Loader TheLoader;


#ifdef _DEBUG
#define LOADER_INSTRUMENT
#endif


#ifdef LOADER_INSTRUMENT
static LoaderQ lastActive;
static DWORD lastActiveCalledAt = 0;
static DWORD lastSleptAt = 0;
static DWORD lastEnquedAt = 0;
static DWORD lastDequeueAt = 0;
static DWORD lastCanceledAt = 0;
static int forceWakeEvent = 0;
#endif

// COBRA - DX - New DX Engine use Flag
extern bool g_bUse_DX_Engine;


void Loader::Setup()
{
    // Setup the starting state for the main loop
    head       = NULL;
    tail       = NULL;
    shutDown       = FALSE;
    stopped    = FALSE;
    paused    = RUNNING;
    queueIsEmpty    = TRUE;
    queueStatus       = QUEUE_FIFO;
    TickDelay  = DEFAULT_LOADER_DELAY;
    actionDone = false;

#ifdef LOADER_INSTRUMENT
    memset(&lastActive, 0, sizeof(lastActive));
    lastActiveCalledAt = 0;
    lastSleptAt = 0;
    lastEnquedAt = 0;
    lastDequeueAt = 0;
    lastCanceledAt = 0;
    forceWakeEvent = 0;
#endif


    // Create the synchronization objects we'll need
    strcpy(WakeEventName, "LoaderWakeupCall");
    WakeEventHandle = CreateEvent(NULL, FALSE, FALSE, WakeEventName);

    if ( not WakeEventHandle)
    {
        ShiError("Failed to create the loader wake up event");
    }

    InitializeCriticalSection(&cs_loaderQ);


    // Spawn the thread
    threadHandle = (HANDLE) _beginthreadex(
                       NULL, 0, (unsigned int (__stdcall *)(void*))MainLoopWrapper, this, 0, (unsigned *) &threadID
                   );

    if ( not threadHandle)
    {
        ShiError("Failed to spawn loader");
    }
}


void Loader::Cleanup()
{
    DWORD retval;

    // Request that the loader shutdown
    EnterCriticalSection(&cs_loaderQ);

    shutDown = TRUE;
    SetEvent(WakeEventHandle);

    //WakeUp();
    // RED - Need to wait it is off
    while ( not stopped);

    LeaveCriticalSection(&cs_loaderQ);
    SetEvent(WakeEventHandle);

    // Wait for it to happen (5000 ms maximum)
    // sfr: changed to infite. If we dont wait, CTD can happen
    retval = WaitForSingleObject(threadHandle, INFINITE/*5000*/);

    // While debugging, die if we failed to kill the thread.  Later, ignore it.
    ShiAssert(retval == WAIT_OBJECT_0);

    // Close the handle to the thread so that it will be deallocated
    CloseHandle(threadHandle);

    // Release any entries remaining in the request queue
    while (GetNextRequest());

    // Release the sychronization objects we've been using
    CloseHandle(WakeEventHandle);
    DeleteCriticalSection(&cs_loaderQ);
}


// Dummy wrapper to get from C-Style Thread spawning back into C++ calling convention
DWORD Loader::MainLoopWrapper(LPVOID myself)
{
    return (((Loader*)myself)->MainLoop());
}


// The actual main loader service loop.  Won't return until flagged to stop
DWORD Loader::MainLoop()
{
    LoaderQ *Active;
    actionDone = false;

    while ( not shutDown)
    {

        // Process everything in our queue
        if (paused == RUNNING)
        {
            actionDone = false;
            // The LOD updating run
            actionDone or_eq ObjectLOD::UpdateLods();
            actionDone or_eq TheTextureBank.UpdateBank();

            while ((Active = GetNextRequest()) and ( not shutDown))
            {
                // Check queue status
                if (queueStatus == QUEUE_FIFO)
                {
                    // Make the callback to notify the requestor
                    // NOTE:  The callback is responsible for deleting the queue entry
                    ShiAssert((Active->callback not_eq NULL));
                    Active->callback(Active);
                }
                else if (queueStatus == QUEUE_SORTING)
                {
                    SortLoaderQueue();
                }
                else if (queueStatus == QUEUE_STORING)
                {
                    ReplaceHeadEntry(Active);
                }
            }

            // We dropped out of the processing loop, so we're either shutting down, or
            // the queue is empty
            EnterCriticalSection(&cs_loaderQ);

            if ( not head)
            {
                queueIsEmpty = TRUE;
            }

            LeaveCriticalSection(&cs_loaderQ);
        }
        else
        {
            // Note that we're truely paused now
            paused = PAUSED;
        }

        // if any action done, restart after a delay, else go sleeping
        if (actionDone)
            Sleep(TickDelay);
        else
            WaitForSingleObject(WakeEventHandle, INFINITE);

        //WaitForSingleObject( WakeEventHandle, 10000 ); //INFINITE

    }

    // We've been asked to quit, so off we go
    stopped = TRUE;

    return 0;
}


// NOTE:  This must only be called from inside the cs_loaderQ critical section
void Loader::Dequeue(LoaderQ *Old)
{
    ShiAssert(Old);

    if (Old->prev)
    {
        Old->prev->next = Old->next;
    }
    else
    {
        ShiAssert(head == Old);
        head = Old->next;
    }

    if (Old->next)
    {
        Old->next->prev = Old->prev;
    }
    else
    {
        ShiAssert(tail == Old);
        tail = Old->prev;
    }

#ifdef LOADER_INSTRUMENT
    lastDequeueAt = GetTickCount();
#endif

    if ( not head)
    {
        // Wake the main loop to ensure it sets the queueIsEmpty flag as appropriate
        SetEvent(WakeEventHandle);
    }
}


// NOTE:  This must only be called from inside the cs_loaderQ critical section
void Loader::Enqueue(LoaderQ *New)
{
    // This is debug code -- should go once I'm done testing.
#ifdef _DEBUG
    LoaderQ *p = head;

    while (p)
    {

        // See if this is a duplicate
        if ((p->fileoffset == New->fileoffset) and 
            (p->filename == New->filename) and 
            (p->parameter == New->parameter) and 
            (p->callback == New->callback))
        {

            return;
            ShiAssert( not "Caught trying to add duplicate request");
        }

        p = p->next;
    }

#endif

    // Link the new queue entry to the end of the Q
    New->next = NULL;
    New->prev = tail;

    if ( not tail)
    {
        head = New;
        queueIsEmpty = FALSE;
    }
    else
    {
        tail->next = New;
    }

    tail = New;

#ifdef LOADER_INSTRUMENT
    lastEnquedAt = GetTickCount();
#endif


    // Post the event to wake up the loader loop
    SetEvent(WakeEventHandle);
}


void Loader::EnqueueRequest(LoaderQ *New)
{
    EnterCriticalSection(&cs_loaderQ);

    // Link the new queue entry to the end of the Q
    if ( not shutDown)
    {
        Enqueue(New);
    }

    LeaveCriticalSection(&cs_loaderQ);
}


void Loader::ReplaceHeadEntry(LoaderQ *New)
{
    EnterCriticalSection(&cs_loaderQ);

    // Link the new queue entry to the HEAD of the Q
    New->next = head;
    New->prev = NULL;

    if (head)
        head->prev = New;

    if ( not tail)
        tail = New;

    LeaveCriticalSection(&cs_loaderQ);
}


LoaderQ* Loader::GetNextRequest(void)
{
    LoaderQ *request;


    EnterCriticalSection(&cs_loaderQ);

    // We always dequeue the head of the list
    request = head;

    if (head)
    {
        Dequeue(head);
    }

    LeaveCriticalSection(&cs_loaderQ);


    // return the requested entry
    return request;
}


// Cancel (if possible) the request to load data into the specified target buffer
BOOL Loader::CancelRequest(void(*callback)(LoaderQ*), void *parameter, char *filename, DWORD fileoffset)
{
    LoaderQ *p;

    EnterCriticalSection(&cs_loaderQ);

    p = head;

    while (p)
    {

        // See if this is the one we want to cancel
        if ((p->filename == filename) and 
            (p->fileoffset == fileoffset) and 
            (p->parameter == parameter) and 
            (p->callback == callback))
        {

            Dequeue(p);
            delete p;
            break;
        }

        p = p->next;
    }

#ifdef LOADER_INSTRUMENT
    lastCanceledAt = GetTickCount();
#endif

    LeaveCriticalSection(&cs_loaderQ);

    return (p not_eq NULL);
}


void Loader::SetPause(BOOL state)
{
    if (state)
    {
        if (paused == RUNNING)
        {
            paused = PAUSING;
        }
    }
    else
    {
        paused = RUNNING;
    }

    WakeUp();
}



// **** COBRA - RED - THIS FUNCTION IS NO MORE USED, SUBSTITUTED FROM FOLOWING ONE
//                    JUST KEPT TO LEAVE CALLS STILL THERE, FUNCTION NEEDING SUCH FEATURE MUST CALL
//                    THE FOLLOWING FUNCTION ...
// This call will block until the Loader queue becomes empty
void Loader::WaitForLoader(void)
{
    // Wait for the queue to become empty
    // CAUTION:  Could never happen if another thread keeps the loader busy

    //WaitLoader();

    // COBRA - DX - The DX VB MANAGER + ENGINE Manages itself readiness of models
    // Use of this function could lock up the Engine/System
    /*if(g_bUse_DX_Engine) return;

    while ( not queueIsEmpty ) {

    #ifdef LOADER_INSTRUMENT
     if (forceWakeEvent > 0) {
     SetEvent( WakeEventHandle );
     forceWakeEvent--;
     }
    #endif

     Sleep(100);
    }*/
}

void Loader::WaitLoader(void)
{

    while ( not queueIsEmpty)
    {

#ifdef LOADER_INSTRUMENT

        if (forceWakeEvent > 0)
        {
            SetEvent(WakeEventHandle);
            forceWakeEvent--;
        }

#endif
        Sleep(100);
    }
}



// Sort the queue by file. This should help on CD/HD access times
void Loader::SortLoaderQueue(void)
{
}


// Tell the loader to store all requests and not act on them
void Loader::SetQueueStatusStoring(void)
{
}


// Tell the loader to sort all current requests, then let it load them
// Note, this will block requests to the loader.
void Loader::SetQueueStatusSorting(void)
{
}

