#include "MsgInc/DLinkMsg.h"
#include "simdrive.h"
#include "mesg.h"
#include "navsystem.h"
#include "falclib.h"
#include "falcmesg.h"
#include "falcgame.h"
#include "falcsess.h"
#include "simbase.h"
#include "icp.h"
#include "OTWDrive.h"
#include "cpmanager.h"

//sfr: added here for checks
#include "InvalidBufferException.h"


char* DLink_Type_Str[5] = {
		"NODINK",
		"IP",
		"TGT",
		"EGR",
		"CP"
	};

FalconDLinkMessage::FalconDLinkMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) : FalconEvent (DLinkMsg, FalconEvent::SimThread, entityId, target, loopback)
{
   // Your Code Goes Here
}

FalconDLinkMessage::FalconDLinkMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target) : FalconEvent (DLinkMsg, FalconEvent::SimThread, senderid, target)
{
   // Your Code Goes Here
	type;
}

FalconDLinkMessage::~FalconDLinkMessage(void)
{
   // Your Code Goes Here
}

int FalconDLinkMessage::Process(uchar autodisp){
	SimBaseClass* theEntity;
	theEntity = (SimBaseClass*)(vuDatabase->Find (EntityId()));
	if (
		theEntity && theEntity->IsLocal() && 
		theEntity == (void*)SimDriver.GetPlayerEntity()
	){
		gNavigationSys->SetDataLinks(
			dataBlock.numPoints,
			dataBlock.targetType,
			dataBlock.threatType,
			(FalconDLinkMessage::DLinkPointType *) dataBlock.ptype,
			dataBlock.px,
			dataBlock.py,
			dataBlock.pz,
			dataBlock.arrivalTime
		);
		OTWDriver.pCockpitManager->mpIcp->SetICPUpdateFlag(DLINK_UPDATE);
	}
	return 0;
}
