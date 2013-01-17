/*
 * Machine Generated source file for message "Laser Designate Msg".
 * NOTE: The functions here must be completed by hand.
 * Generated on 17-March-1998 at 22:13:19
 * Generated from file EVENTS.XLS by Microprose
 */

#include "MsgInc/LaserDesignateMsg.h"
#include "mesg.h"
#include "falclib.h"
#include "falcmesg.h"
#include "falcgame.h"
#include "falcsess.h"
#include "simbase.h"
#include "InvalidBufferException.h"

FalconLaserDesignateMsg::FalconLaserDesignateMsg(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) : FalconEvent (LaserDesignateMsg, FalconEvent::SimThread, entityId, target, loopback)
{
   // Your Code Goes Here
}

FalconLaserDesignateMsg::FalconLaserDesignateMsg(VU_MSG_TYPE type, VU_ID senderid, VU_ID target) : FalconEvent (LaserDesignateMsg, FalconEvent::SimThread, senderid, target)
{
   // Your Code Goes Here
	type;
}

FalconLaserDesignateMsg::~FalconLaserDesignateMsg(void)
{
   // Your Code Goes Here
}

int FalconLaserDesignateMsg::Process(uchar autodisp)
{
SimBaseClass	*theEntity;//,*shooter;

	if (autodisp)
		return 0;

	theEntity = (SimBaseClass*) vuDatabase->Find(dataBlock.target);
	if (theEntity)
   {
      if (dataBlock.state)
		   theEntity->SetFlag(IS_LASED);
      else
		   theEntity->UnSetFlag(IS_LASED);
   }
   return TRUE;
}

