#include "MsgInc/UnitAssignmentMsg.h"
#include "mesg.h"
#include "Unit.h"
#include "Find.h"
#include "falclib.h"
#include "falcmesg.h"
#include "falcgame.h"
#include "falcsess.h"
#include "InvalidBufferException.h"


FalconUnitAssignmentMessage::FalconUnitAssignmentMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) : FalconEvent (UnitAssignmentMsg, FalconEvent::CampaignThread, entityId, target, loopback)
	{
   // Your Code Goes Here
	}

FalconUnitAssignmentMessage::FalconUnitAssignmentMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target) : FalconEvent (UnitAssignmentMsg, FalconEvent::CampaignThread, senderid, target)
	{
   // Your Code Goes Here
	type;
	}

FalconUnitAssignmentMessage::~FalconUnitAssignmentMessage(void)
	{
   // Your Code Goes Here
	}

int FalconUnitAssignmentMessage::Process(uchar autodisp)
	{
	Unit		u = FindUnit(EntityId());

	if (autodisp || !u)
		return -1;

#ifdef KEV_DEBUG
	char			name1[80],name2[80],name3[80];		// For testing
	Objective		po,so,ao;							// For testing
	po = FindObjective(dataBlock.poid);
	if (po)
		sprintf(name1,po->GetName(name1,80));
	else
		sprintf(name1,"<none>");
	so = FindObjective(dataBlock.soid);
	if (so)
		sprintf(name2,so->GetName(name2,80));
	else
		sprintf(name2,"<none>");
	ao = FindObjective(dataBlock.soid);
	if (ao)
		sprintf(name3,ao->GetName(name3,80));
	else
		sprintf(name3,"<none>");
	if (so && po)
		MonoPrint("Assigning Unit %d to %s, %s, objective: %s\n",u->GetCampID(),name2,name1,name3);
#endif

	// Assign objectives
	u->SetUnitPrimaryObj(dataBlock.poid);
	if (dataBlock.soid != FalconNullId)
		u->SetUnitSecondaryObj(dataBlock.soid);
	u->SetAssigned(1);

	// Set orders
	if (dataBlock.orders >= 0)
		u->SetUnitOrders(dataBlock.orders,dataBlock.roid);

	// For now, assume children are owned by same machine. If not, probably have
	// To forward the message
	Unit	e;
	int		en;
	for (en=0; en<MAX_UNIT_CHILDREN; en++)
		{
		e = u->GetUnitElement(en);
		if (e)
			{
			e->SetUnitPrimaryObj(dataBlock.poid);
			if (dataBlock.soid != FalconNullId)
				e->SetUnitSecondaryObj(dataBlock.soid);
			e->SetAssigned(1);
			}
		}
	return 0;
	}

