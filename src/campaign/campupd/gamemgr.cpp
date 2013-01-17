#include "stdhdr.h"
#include "entity.h"
#include "classtbl.h"
#include "falcsess.h"
#include "F4Vu.h"
#include "Flight.h"
#include "CampList.h"
#include "FalcMesg.h"
#include "SimBase.h"
#include "SimMover.h"
#include "SimDrive.h"
#include "OTWDrive.h"
#include "MsgInc/PlayerStatusMsg.h"
#include "GameMgr.h"
#include "PlayerOp.h"
#include "TimerThread.h"
#include "aircrft.h"
#include "MsgInc/RadioChatterMsg.h"
#include "fsound.h"
#include "FalcSnd/LHSP.h"
#include "FalcSnd/FalcVoice.h"
#include "FalcSnd/VoiceManager.h"
#include "digi.h"
#include "atcbrain.h"
#include "dogfight.h"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern void RebuildFLOTList (void);
extern void SetLabel (SimBaseClass* theObject);

extern int gRebuildBubbleNow;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

ulong gBumpTime = 0;
int	gBumpFlag = FALSE;
GameManagerClass	GameManager;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Returns 1 if all players in the game are ready to fly.

int GameManagerClass::AllPlayersReady (VuGameEntity *game)
{
	VuSessionsIterator		sessionWalker(game);
	FalconSessionEntity		*session;
	FalconEntity			*sessent;
	
	int
		ok = TRUE;
	
	session = (FalconSessionEntity*)sessionWalker.GetFirst();
	while (session)
	{
		sessent = (FalconEntity*)session->GetPlayerEntity();
		
		MonoPrint ("APR %s %d\n", session->GetPlayerCallsign (), session->GetFlyState ());
		
		if ((session->GetFlyState() != FLYSTATE_WAITING) && (session->GetFlyState() != FLYSTATE_FLYING))
		{
			ok = FALSE;
		}
		
		session = (FalconSessionEntity*)sessionWalker.GetNext();
	}
	
	return ok;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// Returns 1 if all players in the game are in the UI (or heading there)
int GameManagerClass::NoMorePlayers (VuGameEntity *game)
{
	VuSessionsIterator		sessionWalker(game);
	FalconSessionEntity		*session;
	
	int
		ok = TRUE;
	
	session = (FalconSessionEntity*)sessionWalker.GetFirst();
	while (session)
	{
//		MonoPrint ("NMP %s %d\n", session->GetPlayerCallsign (), session->GetFlyState ());
		
		if (session->GetFlyState() != FLYSTATE_IN_UI)
		{
			ok = FALSE;
		}
		
		session = (FalconSessionEntity*)sessionWalker.GetNext();
	}
	
	return ok;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// Checks and sets player status of this entity 
// (i.e: is there one or more players attached to this?)
int GameManagerClass::CheckPlayerStatus (FalconEntity *entity)
{
	VuSessionsIterator		sessionWalker(FalconLocalGame);
	FalconSessionEntity		*session;
	int						player=0;
	
	if (!entity){
		return 0;
	}
	
	session = (FalconSessionEntity*)sessionWalker.GetFirst();
	while (session && !player){
		if (entity->IsSim() && entity == session->GetPlayerEntity()){
			player = 1;
		}
		else if (
			(entity->IsCampaign()) &&
			(
				(entity == session->GetPlayerFlight()) ||
				(entity == session->GetPlayerSquadron())
			)
		){
			player = 1;
		}
		session = (FalconSessionEntity*)sessionWalker.GetNext();
	}
	
	if (player){
		entity->SetFalcFlag(FEC_HASPLAYERS);
	}
	else {
		entity->UnSetFalcFlag(FEC_HASPLAYERS);
	}
	
	return player;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void GameManagerClass::AnnounceEntry()
{
	// Announce our entry to the other players
	FalconPlayerStatusMessage	*msg = new FalconPlayerStatusMessage(FalconLocalSessionId, FalconLocalGame);
	SimBaseClass				*playerEntity = (SimBaseClass*) FalconLocalSession->GetPlayerEntity();

	_tcscpy(msg->dataBlock.callsign, FalconLocalSession->GetPlayerCallsign());
	msg->dataBlock.playerID         = playerEntity->Id();
	msg->dataBlock.campID           = ((CampBaseClass*)(playerEntity->GetCampaignObject()))->GetCampID();
	msg->dataBlock.side             = ((CampBaseClass*)(playerEntity->GetCampaignObject()))->GetOwner();
	msg->dataBlock.pilotID          = FalconLocalSession->GetPilotSlot();
	msg->dataBlock.vehicleID		= FalconLocalSession->GetAircraftNum();
	msg->dataBlock.state            = PSM_STATE_ENTERED_SIM;
	FalconSendMessage(msg, TRUE);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void GameManagerClass::AnnounceExit()
{
	// Announce our exit to the other players
	FalconPlayerStatusMessage	*msg = new FalconPlayerStatusMessage(FalconLocalSessionId, FalconLocalGame);
	SimBaseClass				*playerEntity = (SimBaseClass*) FalconLocalSession->GetPlayerEntity();
	
	if (FalconLocalSession->GetPlayerFlight())
	{
		msg->dataBlock.campID		= FalconLocalSession->GetPlayerFlight()->GetCampID();
	}
	else
	{
		msg->dataBlock.campID		= 0;
	}
	
	if (playerEntity)
	{
		msg->dataBlock.playerID         = playerEntity->Id();
	}
	
	_tcscpy(msg->dataBlock.callsign,FalconLocalSession->GetPlayerCallsign()); 
	
	msg->dataBlock.side             = FalconLocalSession->GetCountry();
	msg->dataBlock.pilotID          = FalconLocalSession->GetPilotSlot();
	msg->dataBlock.vehicleID		= FalconLocalSession->GetAircraftNum();
	msg->dataBlock.state            = PSM_STATE_LEFT_SIM;
	
	if (FalconLocalGame->GetGameType() == game_Dogfight)
	{
		if (SimDogfight.GetGameType () == dog_TeamMatchplay)
		{
			SimDogfight.localGameStatus = dog_Waiting;
		}
	}
	
	FalconSendMessage(msg, TRUE);
	FalconLocalSession->SetFlyState(FLYSTATE_IN_UI);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void GameManagerClass::AnnounceTransfer (SimBaseClass *oldObj, SimBaseClass *newObj)
{
	// Announce our transfer of entities to the other players
	FalconPlayerStatusMessage	*msg = new FalconPlayerStatusMessage(FalconLocalSessionId, FalconLocalGame);
	
	_tcscpy(msg->dataBlock.callsign, FalconLocalSession->GetPlayerCallsign());
	msg->dataBlock.playerID         = newObj->Id();
	msg->dataBlock.oldID			= oldObj->Id();

	if (oldObj->GetCampaignObject())
	{
		msg->dataBlock.campID		= ((CampBaseClass*)(oldObj->GetCampaignObject()))->GetCampID();
	}
	else
	{
		msg->dataBlock.campID		= 0;
	}

	msg->dataBlock.pilotID          = FalconLocalSession->GetPilotSlot();
	msg->dataBlock.vehicleID		= FalconLocalSession->GetAircraftNum();
	msg->dataBlock.state            = PSM_STATE_TRANSFERED;
	FalconSendMessage(msg, TRUE);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// ===============================================================
// Here are our attachment functions (How the player gets a plane)
// ===============================================================

// Find a specific vehicle inside campEntity
SimMoverClass* GameManagerClass::FindPlayerVehicle (UnitClass *campEntity, int vehSlot) {
	SimMoverClass* simEntity = NULL;

	if ((!campEntity) || (!campEntity->GetComponents())){
		return NULL;
	}

	VuListIterator	flit(campEntity->GetComponents());
	if ((simEntity = static_cast<SimMoverClass*>( flit.GetFirst() )) == NULL){
		return NULL;
	}
	while (simEntity->vehicleInUnit != vehSlot) {
		simEntity = (SimMoverClass*) flit.GetNext();
	}
	return simEntity;

/*	int		count;
	
	SimMoverClass* simEntity = NULL;
	
	if (!campEntity)
		return NULL;
	
	if (campEntity->GetComponents())
	{
		VuListIterator	flit(campEntity->GetComponents());
		
		// Hack Hack Hack Hack HACK - This is a HACK - RH
		count = 0;
		
		while ((!simEntity) && (count < 100))
		{
			simEntity = (SimMoverClass*) flit.GetFirst();

			while (simEntity && simEntity->vehicleInUnit != vehSlot)
			{
				simEntity = (SimMoverClass*) flit.GetNext();
			}
			
			count ++;
			Sleep (100);
		}
		
		ShiAssert(simEntity);
	}
	else
	{
		MonoPrint ("No Components\n");
	}

	return simEntity;

*/
}


// Attach passed player to this sim entity
SimMoverClass* GameManagerClass::AttachPlayerToVehicle (FalconSessionEntity *player, SimMoverClass *simEntity, int playerSlot)
{
	Unit		campEntity;
	
	//simEntity->ChangeOwner(player->Id());
	player->SetPlayerEntity(simEntity);
	if (player == FalconLocalSession){
		simEntity->MakePlayerVehicle();
#define NEW_ATTACH_ORDER 1
#if NEW_ATTACH_ORDER
		// sfr: set it here, since start loop calls this too
		OTWDriver.SetGraphicsOwnship(simEntity);
#endif
		// sfr: this will need changing if one day players are not aircraft anymore
		SimDriver.SetPlayerEntity(static_cast<AircraftClass*>(simEntity));
#if !NEW_ATTACH_ORDER
		// sfr: set it here, since start loop calls this too
		OTWDriver.SetGraphicsOwnship(simEntity);
#endif
		RebuildFLOTList();
		simEntity->ConfigurePlayerAvionics();
	}
	else {
		simEntity->MakePlayerVehicle();
	}

	simEntity->pilotSlot = playerSlot;
	
	LockPlayer(player);
	
	CheckPlayerStatus(simEntity);
	campEntity = (UnitClass*)simEntity->GetCampaignObject();
	CheckPlayerStatus(campEntity);
	
	SetLabel(simEntity);
	
	gRebuildBubbleNow = TRUE;
	
	return simEntity;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int GameManagerClass::DetachPlayerFromVehicle(FalconSessionEntity *player, SimMoverClass* simEntity){
	Unit campEntity;
	
	if (simEntity){
		// JB 010617 (Fixes MP bug where ejected planes can't be destroyed)
		simEntity->UnSetFalcFlag(FEC_INVULNERABLE); 
		simEntity->MakeNonPlayerVehicle();
		simEntity->pilotSlot = simEntity->vehicleInUnit;

		if (player){
			player->SetPlayerEntity(NULL);
		}
		
		// Check it's the local player
		if (player == FalconLocalSession){
			SimDriver.SetPlayerEntity(NULL);
			OTWDriver.SetGraphicsOwnship(NULL);
		}
		
		// Update player status of entity and it's campaign parent
		CheckPlayerStatus(simEntity);
		campEntity = (UnitClass*)simEntity->GetCampaignObject();
		CheckPlayerStatus(campEntity);
		
		SetLabel(simEntity);
		
		gRebuildBubbleNow = TRUE;
	}
	return 1;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void GameManagerClass::ReassignPlayerVehicle (FalconSessionEntity *player, SimMoverClass *oldEntity, SimMoverClass *newEntity)
{
	DetachPlayerFromVehicle(player, oldEntity);
	AttachPlayerToVehicle(player, newEntity, player->GetPilotSlot());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void GameManagerClass::LockPlayer (FalconSessionEntity *player)
{
	SimMoverClass* simEntity = (SimMoverClass*) player->GetPlayerEntity();
	
	if (simEntity){
		MonoPrint ("Locking up the hounds %08x - setting invulnerable\n", simEntity);
		
		// Set our entry flags
		simEntity->SetFalcFlag(FEC_INVULNERABLE);
		simEntity->SetFalcFlag(FEC_PLAYER_ENTERING);
		simEntity->SetFalcFlag(FEC_HOLDSHORT);
		
		gRebuildBubbleNow = TRUE;
	}
	else {
		MonoPrint("Not locking hounds - no sim entity\n");
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// The player is ready to go.. We're going to set our invulnerability countdown and release the aircraft from motion pause 
void GameManagerClass::ReleasePlayer(FalconSessionEntity *player)
{
	SimMoverClass* simEntity = (SimMoverClass*) player->GetPlayerEntity();
	
	ShiAssert ( player != FalconLocalSession || simEntity );
	
	if (simEntity){
		simEntity->UnSetFalcFlag(FEC_PLAYER_ENTERING);
		simEntity->UnSetFalcFlag(FEC_HOLDSHORT);
		MonoPrint ("Releasing the player\n");
		if (!PlayerOptions.InvulnerableOn()){
			MonoPrint ("Releasing the hounds %08x - not invulnerable\n", simEntity);
			simEntity->UnSetFalcFlag(FEC_INVULNERABLE);
		}
		else {
			MonoPrint ("Releasing the hounds %08x\n", simEntity);
		}
		
		simEntity->MakeFlagsDirty();
		
		simEntity->ChangeOwner(player->Id());
		OTWDriver.SetGraphicsOwnship(NULL);
		simEntity->MakePlayerVehicle();
		simEntity->ConfigurePlayerAvionics();
		OTWDriver.SetGraphicsOwnship(simEntity);
		
		player->SetFlyState(FLYSTATE_FLYING);

		// sfr: why OnGround its not disabled ???
		// this is causing players to begin with AP on. 
		// commentted out
		if (simEntity->IsAirplane()/* && !simEntity->OnGround()*/){
			((AircraftClass *)simEntity)->SetAutopilot(AircraftClass::APOff);
		}

		SetTimeCompression(1);
		TheCampaign.Resume();
		OTWDriver.SetOTWDisplayMode(OTWDriverClass::Mode2DCockpit);

		if(simEntity->OnGround() && simEntity->IsAirplane()){
			gBumpFlag = TRUE;
			gBumpTime = SimLibElapsedTime + FalconLocalGame->GetRules()->BumpTimer;
			if(((AircraftClass *)simEntity)->DBrain()->CreateTime() + 2*CampaignSeconds < SimLibElapsedTime){
				FalconRadioChatterMessage	*radioMessage = NULL;
				ObjectiveClass *atc = (ObjectiveClass*)vuDatabase->Find(
					((AircraftClass*)simEntity)->DBrain()->Airbase()
				);
				
				if(((AircraftClass*)simEntity)->DBrain()->IsSetATC(DigitalBrain::PermitTakeoff)){
					radioMessage = CreateCallFromATC(
						atc, (AircraftClass*)simEntity, rcCLEAREDONRUNWAY, FalconLocalSession
					);
					radioMessage->dataBlock.edata[3] = atc->brain->GetRunwayName(
						((AircraftClass*)simEntity)->DBrain()->Runway()
					);
				}
				else if( ((AircraftClass*)simEntity)->DBrain()->IsSetATC(DigitalBrain::PermitRunway) ){
					radioMessage = CreateCallFromATC(
						atc, (AircraftClass*)simEntity, rcPOSITIONANDHOLD, FalconLocalSession
					);
					radioMessage->dataBlock.edata[3] = atc->brain->GetRunwayName(
						((AircraftClass*)simEntity)->DBrain()->Runway()
					);
				}
				else {
					radioMessage = CreateCallFromATC(
						atc, (AircraftClass*)simEntity, rcCLEARTOTAXI, FalconLocalSession
					);
					radioMessage->dataBlock.edata[2] = atc->brain->GetRunwayName(
						((AircraftClass*)simEntity)->DBrain()->Runway()
					);
				}
				radioMessage->dataBlock.time_to_play = 2 * CampaignSeconds;
				FalconSendMessage(radioMessage, FALSE);
			}
		}
		
		gRebuildBubbleNow = TRUE;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
