#include <windows.h>
#include "stdhdr.h"
#include "simdrive.h"
#include "falcmesg.h"
#include "camp2sim.h"
#include "simbase.h"
#include "f4thread.h"
#include "f4error.h"
#include "otwdrive.h"
#include "simio.h"
#include "sinput.h"
#include "ThreadMgr.h"
#include "falcsess.h"
#include "F4Comms.h"
#include "Campaign/Include/Campaign.h"
#include "TimerThread.h"

#include "Campaign/Include/Cmpclass.h"

extern int EndFlightFlag;
#define MAJOR_FRAME_RESOLUTION 50

HANDLE hLoopEvent;

extern FalconPrivateList *DanglingSessionsList;
extern int UpdateDanglingSessions();

/** sfr: this function is run every sim game cycle. It is the only thread safe place in whole code.
* This means every thread is (or should) be waiting for it before continuing.
* Also, it should be as simple as possible for performance reasons.
* One day I intend to make this a separe thread so that messages dont depend on simloop running.
*/
void RealTimeFunction(unsigned long, void*)
{
    static unsigned long update_time = 0, send_time = 0;

    // Check to see if some time events have occured (compression ratio change, etc)
    if ((vuxRealTime > send_time) and (FalconLocalGame))
    {
        ResyncTimes();
        send_time = vuxRealTime + RESYNC_TIME;
    }

    // Run VU Thread and handle messages
    if (vuxRealTime > update_time)
    {
        //CampEnterCriticalSection();
        // sfr: do this inside critical section because of UI shutdown
        VuEnterCriticalSection();
#if CAP_DISPATCH
        // 20 ms at most dispatching
        gMainThread->Update(20);
#else
        gMainThread->Update();
#endif
        VuExitCriticalSection();
        //CampLeaveCriticalSection();

        // sfr: 100 Hz always, since server can be in server mode
        update_time = vuxRealTime + 10;
        //if (FalconLocalSession->GetFlyState() == FLYSTATE_FLYING){
        // update_time = vuxRealTime + 10; // 100 Hz - in Sim
        //}
        //else {
        // update_time = vuxRealTime + 20; // 50 Hz - in UI
        //}
    }

    // KCK: handle messages from all of our 'dangling' sessions
    if (DanglingSessionsList)
    {
        UpdateDanglingSessions();
    }

    // LRKLUDGE to get out on ground impact
    if (EndFlightFlag)
    {
        EndFlightFlag = FALSE;
        OTWDriver.EndFlight();
    }

#if NEW_END_CAMPAIGN

    if (TheCampaign.Flags bitand CAMP_SHUTDOWN_REQUEST)
    {
        // sfr: this is the only safe place to really end the campaign
        TheCampaign.ReallyEndCampaign();
    }

#endif
}

