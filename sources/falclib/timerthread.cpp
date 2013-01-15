#include <windows.h>
#include <process.h>
#include "TimerThread.h"
#include "FalcSess.h"
#include "Cmpclass.h"
#include "ui/include/uicomms.h"
#include "sim/include/simdrive.h"

// Time compression globals
unsigned long       lastStartTime;
int                 gameCompressionRatio = 0;
int                 targetGameCompressionRatio = 0;
int                 targetCompressionRatio = 0;

// Requests bit array
int					remoteCompressionRequests;

#ifndef NO_TIMER_THREAD

#define     RT_FUNCTION_INTERVAL    20

static HANDLE       timerHandle;
static BOOL         timerRunning = FALSE;
static unsigned     timerThreadID;

static unsigned __stdcall timerThread( void )
{
    DWORD   delta;
    DWORD   timerRTFunction;

    timerRTFunction = GetTickCount( );

    while( timerRunning ){
        vuxRealTime = GetTickCount( );
        delta = (vuxRealTime - lastStartTime);
        if (delta > MAX_TIME_DELTA)
            delta = MAX_TIME_DELTA;
		//ShiAssert(vuxGameTime + SimLibMajorFrameTime >= SimLibElapsedTime);
        if (!gCompressTillTime || vuxGameTime + delta * gameCompressionRatio < gCompressTillTime){
			vuxGameTime += delta * gameCompressionRatio;		// Normal time advance
		}
		else if (vuxGameTime < gCompressTillTime)
            vuxGameTime = gCompressTillTime;					// We don't want to advance time past here
		//ShiAssert(vuxGameTime + SimLibMajorFrameTime >= SimLibElapsedTime);
        lastStartTime = vuxRealTime;

        if ( timerRTFunction < vuxRealTime ){
            timerRTFunction += RT_FUNCTION_INTERVAL;
            RealTimeFunction( vuxRealTime, NULL );
        }

        Sleep( THREAD_TIME_SLICE );
    }

    return 0;
}

void beginTimer( void ){
    ShiAssert( timerRunning == FALSE );
    timerRunning = TRUE;
    timerHandle = ( HANDLE ) _beginthreadex(
		NULL, 0, ( unsigned int ( __stdcall * )( void * ) ) timerThread, NULL, 0, &timerThreadID 
	);
    ShiAssert( timerHandle );
    SetThreadPriority( timerHandle, THREAD_PRIORITY_ABOVE_NORMAL );
}

void endTimer(void)
{
    timerRunning = FALSE;
    WaitForSingleObject(timerHandle, INFINITE);
    CloseHandle(timerHandle);
}

#endif 

void ResyncTimes();

// This is the main routine to change our time compression ratio
void SetTimeCompression(int newComp)
	{
    if (newComp < 0)
        newComp = 0;
    if (newComp > 1024)
        newComp = 1024;
	// Force zero compression when game is over
	if (TheCampaign.EndgameResult)
		newComp = 0;

	if (gCommsMgr && gCommsMgr->Online() && FalconLocalGame)
		{
		// For online games, we only set our session's requested time compression.
		FalconLocalSession->SetReqCompression((short)newComp);
		// The resync is now done is SetReqCompression
		// ResyncTimes();		// Resync immediately
		return;
		}
	else
		{
		// Otherwise, set our compression directly
	    lastStartTime = vuxRealTime;
		if(!gameCompressionRatio && newComp)
			SimDriver.lastRealTime = vuxGameTime;
		gameCompressionRatio = newComp;
		targetCompressionRatio = newComp;
		FalconLocalSession->SetReqCompression((short)newComp);
		}
	}

// This is the way we set time compression from remote (without effecting our
// requested compression, essentially.
void SetOnlineTimeCompression(int newComp)
{
	if (FalconLocalSession->GetFlyState () == FLYSTATE_FLYING)
	{
		if (newComp < 0)
			newComp = 0;
		if (newComp > 4)
			newComp = 4;
	}
	else
	{
		if (newComp < 0)
			newComp = 0;
		if (newComp > 128)
			newComp = 128;
	}
    lastStartTime = vuxRealTime;
	if(!gameCompressionRatio && newComp)
		SimDriver.lastRealTime = vuxGameTime;
    gameCompressionRatio = newComp;
    targetCompressionRatio = newComp;
}

// This routine will temporarily change our time compression
// Used to override player's requested compression in time critical places
void SetTemporaryCompression(int newComp)
	{
    if (newComp < 0)
        newComp = 0;
    if (newComp > 1024)
        newComp = 1024;
    lastStartTime = vuxRealTime;
	if(!gameCompressionRatio && newComp)
		SimDriver.lastRealTime = vuxGameTime;
    gameCompressionRatio = newComp;
	}

// This is the one and only way to change time
void SetTime( unsigned long currentTime )
{
    vuxGameTime = currentTime;
	vuxDeadReconTime = 0;
	vuxLastTargetGameTime = 0;
	vuxTargetGameTime = currentTime;

	//ShiAssert(vuxGameTime + SimLibMajorFrameTime >= SimLibElapsedTime);
    lastStartTime = vuxRealTime;
	TheCampaign.CurrentTime = currentTime;
	SimLibFrameElapsed = (float)currentTime;
	SimLibElapsedTime = currentTime;
	UPDATE_SIM_ELAPSED_SECONDS;											// COBRA - RED - Scale Elapsed Seconds
	SimDriver.lastRealTime = currentTime;
	
//	ShiAssert
//	(
//		(gameCompressionRatio == 0) ||
//		(TheCampaign.IsSuspended ())
//	);
}

