/***************************************************************************\
    TimeMgr.cpp
    Scott Randolph
    March 20, 1997

	Manage the visual world's clock and provide periodic callbacks to
	this modules which need to adjust with time of day changes.
\***************************************************************************/
#include "TimeMgr.h"


// The one and only time manager object
TimeManager TheTimeManager;


static const int	MAX_TOD_CALLBACKS	= 64;		// Max number of requestors
static const long	CALLBACK_CYCLE_TIME	= 60000L;	// Approx time to update all requestors
static const long	CALLBACK_TIME_STEP	= CALLBACK_CYCLE_TIME / MAX_TOD_CALLBACKS;



void TimeManager::Setup (int startYear, int startDayOfYear)
{
	ShiAssert( !IsReady() );

	// Store the day the clock started
	year		= startYear;
	startDay	= startDayOfYear;

	// Allocate and intialize the callback list.
	CBlist = new TimeCallBack[MAX_TOD_CALLBACKS];
	memset( CBlist, 0, sizeof(CBlist[0]) * MAX_TOD_CALLBACKS );
	nextCallToMake = 0;
}


void TimeManager::Cleanup()
{
	ShiAssert( IsReady() );

#ifdef _DEBUG	
	for (nextCallToMake = 0; nextCallToMake < MAX_TOD_CALLBACKS; nextCallToMake++) {
		if ( CBlist[nextCallToMake].fn != NULL ) {
			ShiWarning( "TimeManager dieing with callbacks still registered!" );
			break;
		}
	}
#endif

	delete[] CBlist;
	CBlist = NULL;
}


// Add a callback function to the list of those to be periodically called
// as time advances
void TimeManager::RegisterTimeUpdateCB( void(*fn)(void*), void *self )
{
	ShiAssert( IsReady() );
	ShiAssert( fn );

	if (CBlist)
	{
		for (int i=0; i<MAX_TOD_CALLBACKS; i++)
		{
			if (CBlist[i].fn == NULL)
			{
				CBlist[i].self	= self;
				CBlist[i].fn	= fn;
				break;
			}
		}
		// If we fell out the bottom, we ran out of room.
		ShiAssert( i<MAX_TOD_CALLBACKS );
	}
}


// Remove a previously added callback from the list of those which are
// periodically called
void TimeManager::ReleaseTimeUpdateCB( void(*fn)(void*), void *self )
{
	ShiAssert( IsReady() );
	ShiAssert( fn );

	for (int i=0; i<MAX_TOD_CALLBACKS; i++) {
		if (CBlist[i].fn == fn) {
			if (CBlist[i].self == self) {
				CBlist[i].fn	= NULL;
				CBlist[i].self	= NULL;
				break;
			}
		}
	}

	// Squawk if someone tried to remove a callback that wasn't in the list
	ShiAssert( i<MAX_TOD_CALLBACKS );
}


// newTime = millliseconds since game clock start (which was assumed to be midnight)
void TimeManager::SetTime ( DWORD newTime )
{
	ShiAssert( IsReady() );

	// We're in trouble if the clock rolls over (approximatly 49 days after start)
//	ShiAssert(newTime >= lastUpdateTime);

	// Update all our measures of time
	deltaTime = newTime - currentTime;
	currentTime = newTime;
	DWORD day = currentTime / MSEC_PER_DAY;
	timeOfDay = currentTime - day * MSEC_PER_DAY;
	today = day + startDay;
	// TODO:  Deal with leap years???
	if (today >= 365) {
		year += 1;
		today = 0;
	}
	
	// Quit now unless enough time has passed to make it worth while
	// (for now we're set for 60 seconds)
	if (newTime - lastUpdateTime < CALLBACK_TIME_STEP) {
		return;
	}

	// Decide how many steps to take (in case we had a large time step)
	int steps = (newTime - lastUpdateTime) / CALLBACK_TIME_STEP;
	if (steps >= MAX_TOD_CALLBACKS) {
		// Start back at the first callback to make sure things happen in order
		nextCallToMake = 0;
		steps = MAX_TOD_CALLBACKS;
	}
	
	// Note the new time
	lastUpdateTime = newTime;

	// Make the callbacks
	while (steps--) {
		// Make the callback if we have one in this slot
		if (CBlist[nextCallToMake].fn) {
			CBlist[nextCallToMake].fn( CBlist[nextCallToMake].self ); 
		}

		// Advance to the next slot for next time
		nextCallToMake++;
		if (nextCallToMake == MAX_TOD_CALLBACKS) {
			nextCallToMake = 0;
		}
	}
}


// This call is used to force a lighting reset (as when switch to/from NVG mode)
void TimeManager::Refresh( void )
{
	ShiAssert( IsReady() );

	for ( int i=0; i<MAX_TOD_CALLBACKS; i++ ) {
		// Make the callback if we have one in this slot
		if (CBlist[i].fn) {
			CBlist[i].fn( CBlist[i].self ); 
		}
	}

	// Reset our callback control variables
	nextCallToMake = 0;
	lastUpdateTime = currentTime;
}

