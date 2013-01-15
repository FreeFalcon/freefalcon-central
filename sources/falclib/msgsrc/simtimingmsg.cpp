#if 0

/*
 * Machine Generated source file for message "Sim Timing".
 * NOTE: The functions here must be completed by hand.
 * Generated on 23-April-1997 at 12:45:31
 * Generated from file EVENTS.XLS by Leon Rosenshein
 */

/*
#include "MsgInc\SimTimingMsg.h"
#include "mesg.h"
#include "simdrive.h"
#include "falclib.h"
#include "falcmesg.h"
#include "falcgame.h"
#include "falcsess.h"
#include "InvalidBufferException.h"

void ToggleSimPause(int level = -1, int mode = SimulationDriver::DebugMode);

FalconSimTiming::FalconSimTiming(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) : FalconEvent (SimTimingMsg, FalconEvent::SimThread, entityId, target, loopback)
{
   // Your Code Goes Here
}

FalconSimTiming::FalconSimTiming(VU_MSG_TYPE type, VU_ID senderid, VU_ID target) : FalconEvent (SimTimingMsg, FalconEvent::SimThread, senderid, target)
{
   // Your Code Goes Here
}

FalconSimTiming::~FalconSimTiming(void)
{
   // Your Code Goes Here
}

int FalconSimTiming::Process(uchar autodisp)
{
   if (SimDriver.IsRunning() && SimDriver.isPaused != dataBlock.paused)
      ToggleSimPause();
   // Your Code Goes Here
   return 0;
}

*/
#endif