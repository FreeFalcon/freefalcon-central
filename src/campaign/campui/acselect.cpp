#include <windows.h>
#include "Mesg.h"
#include "find.h"
#include "cmpclass.h"
#include "MsgInc/RequestAircraftSlot.h"
#include "uicomms.h"
#include "ui_cmpgn.h"
#include "FalcSess.h"
#include "DispCfg.h"
#include "Flight.h"
#include "ACSelect.h"


// ======================================================================
//
// KCK: The following functions are used by Dogfight, Tactical Engagement
// and Campaign to request a particular flight element for use by the
// player (ie: join a flight).
//
// ======================================================================

BOOL RequestACSlot (Flight flight, uchar team, uchar plane_slot, uchar skill, int ac_type, int player)
{
	UI_RequestAircraftSlot	*msg;
	VuTargetEntity			*target;

	target = (VuTargetEntity*) vuDatabase->Find(FalconLocalGame->OwnerId());

	if (flight)
	{
		msg=new UI_RequestAircraftSlot(flight->Id(),target);
		msg->dataBlock.team = flight->GetTeam();
	}
	else
	{
		msg=new UI_RequestAircraftSlot(FalconNullId,target);
		msg->dataBlock.team = team;
	}
	msg->dataBlock.requesting_session = FalconLocalSessionId;
	msg->dataBlock.game_type = FalconLocalGame->GetGameType();
	msg->dataBlock.requested_slot = plane_slot;
	msg->dataBlock.requested_type = ac_type;
	msg->dataBlock.requested_skill = skill;
	if (player)
	{
		msg->dataBlock.request_type = REQUEST_SLOT_JOIN_PLAYER;
		if (FalconLocalSession->GetPlayerFlight() == flight && FalconLocalSession->GetPilotSlot() < 255)
			msg->dataBlock.current_pilot_slot = FalconLocalSession->GetPilotSlot();
		else
			msg->dataBlock.current_pilot_slot = 255;
		msg->dataBlock.requested_skill = 0;
	}
	else
	{
		msg->dataBlock.request_type = REQUEST_SLOT_JOIN_AI;
		msg->dataBlock.current_pilot_slot = 255;
	}
	FalconSendMessage(msg,TRUE);
	return(TRUE);
}

void LeaveACSlot (Flight flight, uchar plane_slot)
	{
	UI_RequestAircraftSlot	*msg;
	VuTargetEntity			*target;

	if (!flight)
		return;

	target = (VuTargetEntity*) vuDatabase->Find(FalconLocalGame->OwnerId());
	msg=new UI_RequestAircraftSlot(flight->Id(),target);
	msg->dataBlock.requesting_session = FalconLocalSessionId;
	msg->dataBlock.game_type = FalconLocalGame->GetGameType();
	msg->dataBlock.requested_slot = plane_slot;
	msg->dataBlock.request_type = REQUEST_SLOT_LEAVE;
	FalconSendMessage(msg,TRUE);
	}

void RequestFlightDelete (Flight flight)
	{
	UI_RequestAircraftSlot	*msg;
	VuTargetEntity			*target;

	if (!flight)
		return;

	target = (VuTargetEntity*) vuDatabase->Find(FalconLocalGame->OwnerId());
	msg=new UI_RequestAircraftSlot(flight->Id(),target);
	msg->dataBlock.game_type = FalconLocalGame->GetGameType();
	msg->dataBlock.request_type = REQUEST_FLIGHT_DELETE;
	msg->dataBlock.requesting_session = FalconLocalSessionId;
	FalconSendMessage(msg,TRUE);
	}

void RequestTeamChange (Flight flight, int newteam)
	{
	UI_RequestAircraftSlot	*msg;
	VuTargetEntity			*target;

	if (!flight)
		return;

	target = (VuTargetEntity*) vuDatabase->Find(FalconLocalGame->OwnerId());
	msg=new UI_RequestAircraftSlot(flight->Id(),target);
	msg->dataBlock.game_type = FalconLocalGame->GetGameType();
	msg->dataBlock.team = newteam;
	msg->dataBlock.request_type = REQUEST_TEAM_CHANGE;
	msg->dataBlock.requesting_session = FalconLocalSessionId;
	FalconSendMessage(msg,TRUE);
	}

void RequestTypeChange (Flight flight, int newtype)
	{
	UI_RequestAircraftSlot	*msg;
	VuTargetEntity			*target;

	if (!flight)
		return;

	target = (VuTargetEntity*) vuDatabase->Find(FalconLocalGame->OwnerId());
	msg=new UI_RequestAircraftSlot(flight->Id(),target);
	msg->dataBlock.game_type = FalconLocalGame->GetGameType();
	msg->dataBlock.request_type = REQUEST_TYPE_CHANGE;
	msg->dataBlock.requesting_session = FalconLocalSessionId;
	msg->dataBlock.requested_type = newtype;
	FalconSendMessage(msg,TRUE);
	}

void RequestCallsignChange (Flight flight, int newcallsign)
	{
	UI_RequestAircraftSlot	*msg;
	VuTargetEntity			*target;

	if (!flight)
		return;

	target = (VuTargetEntity*) vuDatabase->Find(FalconLocalGame->OwnerId());
	msg=new UI_RequestAircraftSlot(flight->Id(),target);
	msg->dataBlock.game_type = FalconLocalGame->GetGameType();
	msg->dataBlock.team = flight->GetTeam();
	msg->dataBlock.request_type = REQUEST_CALLSIGN_CHANGE;
	msg->dataBlock.requesting_session = FalconLocalSessionId;
	msg->dataBlock.requested_type = newcallsign;
	FalconSendMessage(msg,TRUE);
	}

void RequestSkillChange (Flight flight, int plane_slot, int newskill)
	{
	UI_RequestAircraftSlot	*msg;
	VuTargetEntity			*target;

	if (!flight)
		return;

	target = (VuTargetEntity*) vuDatabase->Find(FalconLocalGame->OwnerId());
	msg=new UI_RequestAircraftSlot(flight->Id(),target);
	msg->dataBlock.game_type = FalconLocalGame->GetGameType();
	msg->dataBlock.team = flight->GetTeam();
	msg->dataBlock.request_type = REQUEST_SKILL_CHANGE;
	msg->dataBlock.requesting_session = FalconLocalSessionId;
	msg->dataBlock.requested_slot = plane_slot;
	msg->dataBlock.requested_skill = newskill;
	FalconSendMessage(msg,TRUE);
	}
