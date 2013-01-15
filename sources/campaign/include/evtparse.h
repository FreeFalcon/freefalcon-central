
#ifndef EVTPARSE_H
#define EVTPARSE_H

#include "FalcMesg.h"

// ===========================================================================
// This event structure has gone through many iterations.
// Currently it is being used as a linked list of events for mission 
// debriefings. Basically, the event string is the text version of the event, 
// the VU_ID is used to link the event to a particular entity, and the time is
// when the event occured.
//
// The EVTPARSE filename is an artifact of when this structure was used to
// write events to disk (originally for ACMI), and included parsing functions
// to build the event strings on demand.
//
// Kevin Klemmick, April 1999
// ===========================================================================

// ============================
// Defines
// ============================

#define MAX_EVENT_STRING_LEN		128			// Maximum string length of an event string

// ============================
// Event Structure
// ============================

class EventElement
{
#ifdef USE_SH_POOLS
   public:
      // Overload new/delete to use a SmartHeap fixed size pool
      void *operator new(size_t size)	{ ShiAssert( size == sizeof(EventElement) ); return MemAllocFS(pool);	};
      void operator delete(void *mem)	{ if (mem) MemFreeFS(mem); };
      static void InitializeStorage()	{ pool = MemPoolInitFS( sizeof(EventElement), 50, 0 ); };
      static void ReleaseStorage()		{ MemPoolFree( pool ); };
      static MEM_POOL	pool;
#endif

public:
	EventElement (void)			{};
	~EventElement (void)		{};

	// This data is used to build event lists
	VU_ID			vuIdData1;
	VU_ID			vuIdData2;
	CampaignTime	eventTime;
	_TCHAR			eventString[MAX_EVENT_STRING_LEN];
	EventElement	*next;
};

// ============================
// Functions
// ============================

void InsertEventToList (EventElement* theEvent, EventElement* baseEvent);

void DisposeEventList (EventElement* rootEvent);

#endif
