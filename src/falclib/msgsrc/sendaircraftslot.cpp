#include "MsgInc/SendAircraftSlot.h"
#include "Flight.h"
#include "mesg.h"
#include "userids.h"
#include "CmpClass.h"
#include "Dispcfg.h"
#include "falclib.h"
#include "falcmesg.h"
#include "falcgame.h"
#include "falcsess.h"
#include "MissEval.h"
#include "InvalidBufferException.h"


extern void LeaveDogfight();
extern void UI_Refresh (void);
extern void CheckForNewPlayer (FalconSessionEntity *session);

UI_SendAircraftSlot::UI_SendAircraftSlot(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) : FalconEvent (SendAircraftSlot, FalconEvent::SimThread, entityId, target, loopback)
{
	RequestOutOfBandTransmit();
	RequestReliableTransmit ();
	dataBlock.got_pilot_skill = 0;
}

UI_SendAircraftSlot::UI_SendAircraftSlot(VU_MSG_TYPE type, VU_ID senderid, VU_ID target) : FalconEvent (SendAircraftSlot, FalconEvent::SimThread, senderid, target)
{
	RequestOutOfBandTransmit();
	RequestReliableTransmit ();
	// Your Code Goes Here
	type;
}

UI_SendAircraftSlot::~UI_SendAircraftSlot(void)
{
	// Your Code Goes Here
}

int UI_SendAircraftSlot::Process(uchar autodisp){
	if (autodisp){ 
		return 0; 
	}

	VuGameEntity		*game = (VuGameEntity*) vuDatabase->Find(dataBlock.game_id);
	FalconSessionEntity	*session = (FalconSessionEntity*) vuDatabase->Find(dataBlock.requesting_session);
	Flight				oldflight,flight = (Flight) Entity();

	if (flight == 0)
	{
		MonoPrint ("Got a Send Aircraft Slot on a flight that doesn't exist\n");
		return 0;
	}

	MonoPrint ("SendAircraftSlot %08x %d\n", flight, dataBlock.got_slot);

	VuGameEntity *g = FalconLocalGame;
	if (game != FalconLocalGame || !session){
		return FALSE;
	}

	ShiAssert (session->GetFlyState() == FLYSTATE_IN_UI);

	oldflight = session->GetPlayerFlight();
	session->SetPlayerFlight(flight);
	session->SetAircraftNum(dataBlock.got_slot);
	session->SetPilotSlot(dataBlock.got_pilot_slot);

	if (!flight){
		return FALSE;
	}

	// For Campaign and TE games, if the player switches flights, we need to redo PreMissionEval
	if (session == FalconLocalSession && oldflight != flight && FalconLocalGame->GetGameType() != game_Dogfight)
		TheCampaign.MissionEvaluator->PreMissionEval(flight,dataBlock.got_pilot_slot);

	CheckForNewPlayer (session);

	if (dataBlock.game_type == game_Dogfight)
		session->SetCountry(dataBlock.team);

	// Update flight stats
	if (FalconLocalGame->GetGameType() == game_Dogfight)
		flight->pilots[dataBlock.got_slot] = dataBlock.got_pilot_skill;
	if (dataBlock.got_pilot_slot > 0 && dataBlock.got_pilot_slot < 255)
		flight->player_slots[dataBlock.got_slot] = dataBlock.got_pilot_slot;

	UI_Refresh();

	return TRUE;
}

int UI_SendAircraftSlot::Decode (VU_BYTE **buf, long *rem) {
	long init = *rem;

	FalconEvent::Decode (buf, rem);
	memcpychk(&dataBlock, buf, sizeof (dataBlock), rem);
	return init - *rem;
}

int UI_SendAircraftSlot::Encode (VU_BYTE **buf){
	int size;

	size = FalconEvent::Encode (buf);
	memcpy (*buf, &dataBlock, sizeof (dataBlock));
	*buf += sizeof (dataBlock);
	size += sizeof (dataBlock);
	return size;
}









