/***************************************************************************\
    Loader.h
    Scott Randolph
    August 24, 1995

    Asyncrhonus loader module.  This class is designed to be run on its
    own thread and respond to requests for data off the disk.
\***************************************************************************/
#ifndef _LOADER_H_
#define _LOADER_H_

#include "grtypes.h"



#define	DEFAULT_LOADER_DELAY	100				// Default value of delay in mSeconds
#define	FAST_LOADER_DELAY		5				// Fast value of delay in mSeconds


// The global asynchronous loader used by everyone
extern class Loader	TheLoader;



// The structure used to request a data transfer
typedef struct LoaderQ {
    char            *filename;
	DWORD			fileoffset;
	void			(*callback)( LoaderQ* );
	void			*parameter;
	LoaderQ		*prev;		// Modified by loader
	LoaderQ		*next;		// Modified by loader
} LoaderQ;


typedef enum { RUNNING = 0, PAUSING, PAUSED } LoaderPauseMode;
typedef enum { QUEUE_FIFO, QUEUE_SORTING, QUEUE_STORING } QueueMode;


class Loader {
  public:
  	Loader()	{};
	~Loader()	{};

	void Setup( void );
	void Cleanup( void );
	void SetDelay ( DWORD Delay ) { TickDelay = Delay; }
	void WakeUp(void)			  { SetEvent( WakeEventHandle ); }

	void EnqueueRequest( LoaderQ *New );
	BOOL CancelRequest( void(*callback)(LoaderQ*), void *parameter, char *filename, DWORD fileoffset );

	void SetPause( BOOL state );

	BOOL Paused( void )		{ return paused == PAUSED; };
	BOOL Stopped( void )	{ return stopped; };

	BOOL LoaderQueueEmpty( void )	{ return queueIsEmpty; };
	void WaitForLoader( void );										// no more used - use WaitLoader instead
	void WaitLoader( void );

   void SetQueueStatusStoring (void);
   void SetQueueStatusSorting (void);
   QueueMode QueueStatus (void) {return queueStatus;};

  private:
	HANDLE	threadHandle;
	DWORD	threadID;
	bool	actionDone;

	char	WakeEventName[30];
	HANDLE	WakeEventHandle;

	CRITICAL_SECTION	cs_loaderQ;

	LoaderQ*	head;
	LoaderQ*	tail;

  	volatile BOOL				shutDown;
	volatile BOOL				stopped;
	volatile LoaderPauseMode	paused;
	volatile BOOL				queueIsEmpty;
   volatile QueueMode      queueStatus;
   volatile	DWORD			TickDelay;

   DWORD	static	MainLoopWrapper( LPVOID myself );
 	DWORD			MainLoop( void );
   void SortLoaderQueue (void);
	LoaderQ*		GetNextRequest( void );

	void			Enqueue( LoaderQ *New );
	void			Dequeue( LoaderQ *Old );
   void        ReplaceHeadEntry (LoaderQ *New);
};


#endif // _LOADER_H_
