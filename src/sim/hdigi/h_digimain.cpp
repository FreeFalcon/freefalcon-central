#include "stdhdr.h"
#include "simobj.h"
#include "hdigi.h"
#include "simveh.h"
#include "mesg.h"
#include "object.h"
#include "MsgInc/WingmanMsg.h"
#include "campBase.h"

#ifdef USE_SH_POOLS
MEM_POOL		HeliBrain::pool;
#endif

HeliBrain::HeliBrain (SimVehicleClass *myPlatform)
{
	self = (HelicopterClass *)myPlatform;
	side = myPlatform->GetTeam();
	self->flightLead = self;
	underOrders = FALSE;
	curFormation = FalconWingmanMsg::WMWedge;
	curMode = WaypointMode;
	lastMode = WaypointMode;
	nextMode = NoMode;
	onStation = NotThereYet;

	flags = 0;
	isWing = 0;

	// Init Target data
	lastTarget = NULL;
	targetList = NULL;
	targetPtr = NULL;
	targetData = NULL;

	campMission = 0;
	trackMode = 0;
	hf = NULL;
	saveMode = NoMode;
	waypointMode = NoMode;
	rangedot = 0.0f;
	lastAta = 0.0f;
	lastRange = 0.0f;
	maxThreatPtr = NULL;
	maxTargetPtr = NULL;
	gunsJinkPtr = NULL;
	curMissile = NULL;
	curMissileStation = -1;
	curMissileNum = -1;
	mslCheckTimer = 0.0f;
	jinkTime = 0;
	waitingForShot = 0;
	rangeddot = 0.0f;
	mnverTime = 0.0f;
	newroll = 0.0f;
	pastAta = 0.0f;
	pastPstick = 0.0f;
	fCount = 0;
	trackX = 0.0f;
	trackY = 0.0f;
	trackZ = 0.0f;
	pointCounter = 0;
	curOrder = 0;
	headingOrdered = 0.0f;
	altitudeOrdered = 0.0f;
	pStick = 0.0f;
	rStick = 0.0f;
	yPedal = 0.0f;
	throtl = 0.0f;

	// 2001-11-29 ADDED BY S.G. INIT gndTargetHistory[2] SO IT'S NOT GARBAGE TO START WITH // M.N.
	targetHistory[0] = targetHistory[1] = NULL;
	// END OF ADDED SECTION
	nextTargetUpdate = 0.0f;

	// RV - Biker - Integrator for altitude PI-controler
	powerI = 0.0f;
}

HeliBrain::~HeliBrain (void)
{
	CleanupLanding();
}

void HeliBrain::FrameExec(SimObjectType* curTargetList, SimObjectType* curTarget)
{
	// targetList = curTargetList;
	// SetTarget(curTarget);

	// Calculate relative geometry
	if (targetPtr) {
		rangedot = (targetData->range - lastRange) / SimLibMajorFrameTime;
		lastRange = targetData->range;
	}

	// Integrate the data 
	//
	// edg: no longer do this for helos, just select a target in TargetSelection
	// which is in decision logic
	// SensorFusion ();

	DecisionLogic();

	Actions();

	if (targetPtr != lastTarget) {
		lastTarget = targetPtr;
		ataddot = 10.0F;
		rangeddot = 10.0F;
	}
	curTarget;curTargetList;
}

void HeliBrain::SetLead (int flag)
{
	if (flag) {
		isWing = FALSE;
	}
	else {
		isWing = self->GetCampaignObject()->GetComponentIndex(self);
	}
}

void HeliBrain::JoinFlight (void)
{
	self->flightIndex = self->GetCampaignObject()->GetComponentIndex(self);
	if (self->flightIndex != 0) {
		SetLead (FALSE);
		self->flightLead = (HelicopterClass *)self->GetCampaignObject()->GetComponentLead();
	}
	else {
		SetLead (TRUE);
	}
}

void HeliBrain::SetTarget (SimObjectType* newTarget)
{
	if (newTarget == targetPtr)
		return;

	ClearTarget();
	if (newTarget) {
        ShiAssert( newTarget->BaseData() != (FalconEntity*)0xDDDDDDDD );
		newTarget->Reference(  );
		targetData = newTarget->localData;
	}
	targetPtr = newTarget;
}

void HeliBrain::ClearTarget (void)
{
	if (targetPtr) {
		targetPtr->Release();
	}
	targetPtr = NULL;
    targetData = NULL;
}
