/***************************************************************************\
    TimeMgr.h
    Scott Randolph
    March 20, 1997

	Manage the visual world's clock and provide periodic callbacks to
	this modules which need to adjust with time of day changes.
\***************************************************************************/
#ifndef TIMEMGR_H
#define TIMEMGR_H

#include "grtypes.h"


#define MSEC_PER_DAY					86400000L	// 60sec * 60min * 24hr


typedef struct TimeCallBack {
	void(*fn)(void*);
	void *self;
} TimeCallBack;


// The one and only time manager object
extern class TimeManager TheTimeManager;


class TimeManager {
  public:
	TimeManager( void )	{ year = lastUpdateTime = currentTime = timeOfDay = today = 0; CBlist = NULL; };
	~TimeManager()		{ if (IsReady())	Cleanup (); };

	void	Setup (int startYear, int startDayOfYear);
	void	Cleanup (void);

	BOOL	IsReady( void )		{ return (CBlist != NULL); };

	void	SetTime( DWORD newTime );
	void	Refresh( void );

	void	RegisterTimeUpdateCB( void(*fn)(void *self), void *self );
	void	ReleaseTimeUpdateCB( void(*fn)(void *self), void *self );

	DWORD	GetYearAD( void )			{ return year; };
	DWORD	GetDayOfYear( void )		{ return today; };
	DWORD	GetDayOfLunarMonth( void )	{ return today & 31; };
	DWORD	GetTimeOfDay( void )		{ return timeOfDay; };
	DWORD	GetClockTime( void )		{ return currentTime; };
	DWORD	GetDeltaTime( void )		{ return deltaTime; };

protected:
	DWORD			deltaTime;					// milliseconds between the two most recent updates
	DWORD			currentTime;				// milliseconds since midnight, day 0
	DWORD			timeOfDay;					// milliseconds since midnight today
	DWORD			startDay;					// number of days from Jan 1 to day 0
	DWORD			today;						// number of days from Jan 1 to today
	DWORD			year;						// number of years AD

	DWORD			lastUpdateTime;				// currentTime at last callback execution
	int				nextCallToMake;				// index of next CB function to call
	TimeCallBack	*CBlist;
};


#endif // TIMEMGR_H