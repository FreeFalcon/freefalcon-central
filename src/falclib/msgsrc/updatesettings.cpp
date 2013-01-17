#if 0

#include "MsgInc/UpdateSettings.h"
#include "mesg.h"
#include "uicomms.h"
#include "falclib.h"
#include "falcmesg.h"
#include "falcgame.h"
#include "falcsess.h"
#include "InvalidBufferException.h"

/*
UI_UpdateSettings::UI_UpdateSettings(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) : FalconEvent (UpdateSettings, FalconEvent::UIThread, entityId, target, loopback)
{
	RequestOutOfBandTransmit();
}

UI_UpdateSettings::UI_UpdateSettings(VU_MSG_TYPE type, VU_ID senderid, VU_ID target) : FalconEvent (UpdateSettings, FalconEvent::UIThread, senderid, target)
{
   // Your Code Goes Here
}

UI_UpdateSettings::~UI_UpdateSettings(void)
{
   // Your Code Goes Here
}

int UI_UpdateSettings::Process(uchar autodisp)
{
	if(dataBlock.from != vuLocalSessionEntity->Id())
	{
		if(gCommsMgr == NULL)
			return(0);

		gCommsMgr->UpdateSetting(dataBlock.setting,dataBlock.value);
	}
	return(0);
}
*/
#endif