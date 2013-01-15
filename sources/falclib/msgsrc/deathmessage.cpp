/*
 * Machine Generated source file for message "Death Message".
 * NOTE: The functions here must be completed by hand.
 * Generated on 05-February-1997 at 18:30:20
 * Generated from file EVENTS.XLS by Leon Rosenshein
 */

#include "MsgInc/DeathMessage.h"
#include "mesg.h"
#include "Squadron.h"
#include "uicomms.h"
#include "msginc/radiochattermsg.h"
#include "falclib.h"
#include "falcmesg.h"
#include "falcgame.h"
#include "falcsess.h"
#include "team.h"
#include "MissEval.h"

//sfr: added here for checks
#include "InvalidBufferException.h"

extern bool g_bLogEvents;
extern uchar GetOwner (uchar* map_data, GridIndex x, GridIndex y);

void EvaluateKill (FalconDeathMessage *dtm, SimBaseClass *simShooter, CampBaseClass *campShooter, SimBaseClass *simTarget, CampBaseClass *campTarget);

FalconDeathMessage::FalconDeathMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) : FalconEvent (DeathMessage, FalconEvent::SimThread, entityId, target, loopback)
{
	dataBlock.dEntityID = FalconNullId;
	dataBlock.fEntityID = FalconNullId;
}

FalconDeathMessage::FalconDeathMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target) : FalconEvent (DeathMessage, FalconEvent::SimThread, senderid, target)
{
	type;
}

FalconDeathMessage::~FalconDeathMessage(void)
{
   // Your Code Goes Here
}

int FalconDeathMessage::Process(uchar autodisp)
{
	SimBaseClass*	target = (SimBaseClass*) vuDatabase->Find(dataBlock.dEntityID);
	SimBaseClass*	shooter = (SimBaseClass*) vuDatabase->Find(dataBlock.fEntityID);
	CampEntity		campTarget = (CampEntity) GetEntityByCampID(dataBlock.dCampID);
	CampEntity		campShooter = (CampEntity) GetEntityByCampID(dataBlock.fCampID);
	
	if (autodisp)
		return 0;

	// send the death message to the base obj
	if ( target )
		target->ApplyDeathMessage( this );

	// KCK: Chalk off a vehicle in the unit if the target is a Campaign Unit
	if (campTarget && campTarget->IsUnit())
		{
		if (!campTarget->IsAggregate() && target && !target->IsSetFalcFlag(FEC_REGENERATING))
			campTarget->GetComponents()->Remove(target);
		if (campTarget->IsLocal())
			campTarget->RecordCurrentState(NULL, FALSE);
		// Check for radar being killed
		// KCK NOTE: This will shut down radar for unit if ANY of our possibly multiple radar vehicles are killed
		if (target && campTarget->IsBattalion() && target->GetSlot() == ((Unit)campTarget)->GetUnitClassData()->RadarVehicle && campTarget->IsEmitting())
			campTarget->SetEmitting(0);
		}

	// KCK: update kill records for mission evaluator.
	EvaluateKill (this, shooter, campShooter, target, campTarget);
	
	if(target && (target->IsAirplane() || !(rand()%3)) )
	{
		if( shooter && shooter->IsAirplane() &&
			(GetTTRelations(shooter->GetTeam(), target->GetTeam()) >= Hostile) && rand()%2
			&& !shooter->IsPlayer() )
		{			
			FalconRadioChatterMessage *radioMessage = new FalconRadioChatterMessage( shooter->Id(), FalconLocalSession );
			radioMessage->dataBlock.from = shooter->Id();
			radioMessage->dataBlock.to = MESSAGE_FOR_TEAM;
			
			
			if(((CampBaseClass*)shooter->GetCampaignObject())->IsFlight())
			{
				radioMessage->dataBlock.voice_id = (uchar)((Flight)shooter->GetCampaignObject())->GetPilotVoiceID(((AircraftClass*)shooter)->vehicleInUnit);
				radioMessage->dataBlock.edata[0] = (short)shooter->GetCallsignIdx();
				radioMessage->dataBlock.edata[1] = (short)((Flight)shooter->GetCampaignObject())->GetPilotCallNumber(((AircraftClass*)shooter)->vehicleInUnit);
			}
			else
			{
				radioMessage->dataBlock.voice_id = (uchar)(((int)shooter) % 12); // JPO VOICEFIX
				radioMessage->dataBlock.edata[0] = -1;
				radioMessage->dataBlock.edata[1] = -1;					  
			}
			
			if(target->IsAirplane())
			{
				radioMessage->dataBlock.message = rcAIRBDA;
				//M.N. changed to 32767 which flexibly uses randomized values of available eval indexes
				radioMessage->dataBlock.edata[2] = 32767;
/*				if(rand()%2)
					radioMessage->dataBlock.edata[2] = 1;
				else
					radioMessage->dataBlock.edata[2] = 9;*/
			}
			else if(target->IsUnit())
			{
				radioMessage->dataBlock.message = rcMOVERBDA;
				radioMessage->dataBlock.edata[0] = 32767;
			}
			else
			{
				radioMessage->dataBlock.message = rcSTATICBDA;					  
				radioMessage->dataBlock.edata[2] = 32767;
			}
			
			
			
			FalconSendMessage(radioMessage, FALSE);
		}
	}

	return 0;
}

// =============================================
// Support Functions
// =============================================

// Anytime something is killed, we need to credit the kill to the appropriate sources.
void EvaluateKill (FalconDeathMessage *dtm, SimBaseClass *simShooter, CampBaseClass *campShooter, SimBaseClass *simTarget, CampBaseClass *campTarget)
	{
	int			kill_type=-1,tid,ps=PILOT_KIA;
	Squadron	sq;

	if (!campShooter || !campTarget)
		return;

	// Determine type of kill
	// KCK: This check for being a player will probably not work
	if (simTarget && simTarget->IsSetFalcFlag(FEC_HASPLAYERS))
		kill_type = ASTAT_PKILL;
	else if (campTarget->IsObjective())
		kill_type = ASTAT_ASKILL;
	else if (campTarget->IsFlight())
		kill_type = ASTAT_AAKILL;
	else if (campTarget->IsBattalion())
		kill_type = ASTAT_AGKILL;
	else if (campTarget->IsTaskForce())
		kill_type = ASTAT_ANKILL;

	// Credit kill if shooter was a flight && target was not on our team (not nessisarily ok for RoE)
	if (campShooter->IsFlight() && campShooter->GetTeam() != campTarget->GetTeam())
		{
		int			pilot,squadron_pilot;

		// Find the pilot who did the killing (or pick one)
		sq = (Squadron) ((Flight)campShooter)->GetUnitSquadron();
		// JB 010107
		//if (simShooter)
		if (simShooter && (void*) simShooter != (void*) campShooter) // JB 010107 CTD Sanity check
		// JB 010107
			pilot = ((SimMoverClass*)simShooter)->pilotSlot;
		else
			pilot = ((Flight)campShooter)->PickRandomPilot(campTarget->Id().num_);

		if (pilot < PILOTS_PER_FLIGHT)
			squadron_pilot = ((Flight)campShooter)->pilots[pilot];
		else
			squadron_pilot = 255;		// Player kill, probably

		// Update squadron records
		if (sq && squadron_pilot >= 0 && squadron_pilot < PILOTS_PER_SQUADRON&& kill_type > -1)
			sq->ScoreKill(squadron_pilot, kill_type);
		}

	// Determine pilot status and chalk up a loss
	if (campTarget->IsFlight())
		{
		GridIndex	x,y;
		PilotClass	*pc;
		int			pilot,squadron_pilot;

		sq = (Squadron) ((Flight)campTarget)->GetUnitSquadron();

		// Find the target pilot
		if (simTarget)
			pilot = ((SimMoverClass*)simTarget)->pilotSlot;
		else if (dtm)
			pilot = dtm->dataBlock.dPilotID;
		else
			pilot = ((Flight)campTarget)->PickRandomPilot(campTarget->GetCampID());

		if (pilot < PILOTS_PER_FLIGHT)
			squadron_pilot = ((Flight)campTarget)->pilots[pilot];
		else
			squadron_pilot = 255;		// Player kill, probably

		if (sq && squadron_pilot >= 0 && squadron_pilot < PILOTS_PER_SQUADRON)
			{
			pc = sq->GetPilotData(squadron_pilot);

			if (pc && pc->pilot_status != PILOT_RESCUED)
				{
				pc->pilot_status = PILOT_KIA;
				// For campaign spawned deaths, we use the aircraft status, and assume rescued if over friendly
				// territory.
				// For sim spawned deaths, we assume KIA. Choose between MIA and rescued if and when we get an
				// eject message
				if (!dtm || dtm->dataBlock.dEntityID != FalconNullId)
					{
					if (((Flight)campTarget)->plane_stats[pilot] == AIRCRAFT_MISSING)
						{
						campTarget->GetLocation(&x,&y);
						if (GetRoE(GetOwner(TheCampaign.CampMapData,x,y),campTarget->GetTeam(),ROE_AIR_USE_BASES) == ROE_ALLOWED)
							ps = pc->pilot_status = PILOT_RESCUED;
						else
							ps = pc->pilot_status = PILOT_MIA;
						}
					}
				if (pc->pilot_id == 1)
					pc->pilot_status = PILOT_AVAILABLE;
				}
			}
		}

	// Update mission evaluation records if either target or shooter is in our package
	if (dtm && ((campTarget && (campTarget->InPackage()||g_bLogEvents)) || (campShooter && (campShooter->InPackage()||g_bLogEvents))))
		TheCampaign.MissionEvaluator->RegisterKill(dtm, kill_type, ps);

	// Update some status flags as well for all hits
	if (simTarget)
		tid = simTarget->GetSlot();
	else
		tid = 255;
	TheCampaign.MissionEvaluator->RegisterKill(campShooter,campTarget,tid);
	}
