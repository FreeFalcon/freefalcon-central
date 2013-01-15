#if 0

/*
 * Machine Generated source file for message "Update Position in UI".
 * NOTE: The functions here must be completed by hand.
 * Generated on 12-January-1998 at 21:13:25
 * Generated from file EVENTS.XLS by Peter
 */
/*
#include "MsgInc/UpdateMapPosition.h"
#include "mesg.h"
#include "falclib.h"
#include "falcmesg.h"
#include "falcgame.h"
#include "falcsess.h"
#include "uicomms.h"

//sfr: added here for checks
#include "InvalidBufferException.h"

//extern data
extern UIComms *gCommsMgr;

UI_UpdateMapPosition::UI_UpdateMapPosition(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) :
	FalconEvent (UpdateMapPosition, FalconEvent::UIThread, entityId, target, loopback)
{
   // Your Code Goes Here
}

UI_UpdateMapPosition::UI_UpdateMapPosition(VU_MSG_TYPE type, VU_ID senderid, VU_ID target) : FalconEvent (UpdateMapPosition, FalconEvent::UIThread, senderid, target)
{
   // Your Code Goes Here
}

UI_UpdateMapPosition::~UI_UpdateMapPosition(void)
{
   // Your Code Goes Here
}

int UI_UpdateMapPosition::Process(uchar autodisp)
{
	if(autodisp)
		return(0);

	if(gCommsMgr){
		gCommsMgr->UpdateLocation(dataBlock.x,dataBlock.y);
	}
   return 0;
}

*/

#endif