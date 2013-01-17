#include "MsgInc/DamageMsg.h"
#include "MsgInc/CampWeaponFireMsg.h"
#include "mesg.h"
#include "sim/include/simbase.h"
#include "sim/include/SimDrive.h"
#include "Find.h"
#include "Flight.h"
#include "MissEval.h"
#include "falclib.h"
#include "falcmesg.h"
#include "falcgame.h"
#include "falcsess.h"
#include "InvalidBufferException.h"

#include "IVibeData.h"
extern IntellivibeData g_intellivibeData;

FalconDamageMessage::FalconDamageMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) : FalconEvent (DamageMsg, FalconEvent::SimThread, entityId, target, loopback)
{
}

FalconDamageMessage::FalconDamageMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target) : FalconEvent (DamageMsg, FalconEvent::SimThread, senderid, target)
{
}

FalconDamageMessage::~FalconDamageMessage(void)
{
}

int FalconDamageMessage::Process(uchar autodisp)
{
	if (autodisp){
		return 0;
	}

	FalconEntity	*theEntity,*shooter;
	theEntity = (FalconEntity*) vuDatabase->Find(dataBlock.dEntityID);
	if (theEntity){
		if (theEntity->IsSim()){
			((SimBaseClass*)theEntity)->ApplyDamage(this);
			// Record any hits directly
			if (TheCampaign.MissionEvaluator && !(theEntity->IsSetFalcFlag(FEC_INVULNERABLE))){
				TheCampaign.MissionEvaluator->RegisterHit(this);
			}
		}
		else if (theEntity->IsCampaign()){
			CampEntity		campTarget = (CampEntity)theEntity;
			CampEntity		campShooter = NULL;
	
			shooter = (FalconEntity*)vuDatabase->Find(dataBlock.fEntityID);

			// ShiAssert (!"This is a bad thing I think");
			if (!shooter){
				return TRUE;
			}

			if (shooter->IsSim()){
				campShooter = ((SimBaseClass*)shooter)->GetCampaignObject();
			}
			else {
				campShooter = (CampEntity)shooter;
			}

			if (!campTarget->IsAggregate()){
				// This thing is actually deaggregated (probably happened while
				// the missile was in flight). Chalk it up as a miss if it
				// doesn't happen very often.
				// ShiAssert (!"This probably shouldn't happen.");
			}
			else if (campShooter->IsUnit()){
				// Otherwise, apply the damage to the campaign unit (we have to convert
				// the the campaign's parameter expectations).
				campShooter->ReturnToSearch();
				if (campTarget->IsLocal()){
					FalconCampWeaponsFire	*cwfm = new FalconCampWeaponsFire(campTarget->Id(), FalconLocalGame);
					cwfm->dataBlock.shooterID = campShooter->Id();
					cwfm->dataBlock.fPilotId = dataBlock.fPilotID;
					cwfm->dataBlock.dPilotId = dataBlock.dPilotID;
					cwfm->dataBlock.weapon[0] = (short)GetWeaponIdFromDescriptionIndex(dataBlock.fWeaponID-VU_LAST_ENTITY_TYPE);
					cwfm->dataBlock.weapon[1] = 0;
					cwfm->dataBlock.shots[0] = 1;
					cwfm->dataBlock.fWeaponUID = dataBlock.fWeaponUID;
					campTarget->ApplyDamage(cwfm, 200);
				}
			}
			// KCK: Currently Apply Damage calls register hit (sometimes multiple times). Theoretically,
			// it should be possible to call it here, like we do for sim entities - but this is a task
			// for another time.
//			if (TheCampaign.MissionEvaluator && (!theEntity || !theEntity->IsSetFalcFlag(FEC_INVULNERABLE)))
//				TheCampaign.MissionEvaluator->RegisterHit(this);
		}
	}

	return TRUE;
}

FalconDamageMessage *CreateGroundCollisionMessage(SimVehicleClass* vehicle, int damage, VuTargetEntity *target)
{
	ShiAssert(vehicle);

	if (FalconLocalSession && vehicle == FalconLocalSession->GetPlayerEntity())
		g_intellivibeData.CollisionCounter++;

	FalconEntity	*lastToHit = (SimVehicleClass*)vuDatabase->Find(vehicle->LastShooter());
	CampBaseClass	*campUnit = NULL;

	FalconDamageMessage* message;
	message = new FalconDamageMessage (vehicle->Id(), target );
	
	if (lastToHit && !lastToHit->IsEject() ){
		message->dataBlock.fEntityID  = lastToHit->Id();
		message->dataBlock.fIndex     = lastToHit->Type();
		if (lastToHit->IsSim()){
			message->dataBlock.fPilotID	= ((SimVehicleClass*)lastToHit)->pilotSlot;
			message->dataBlock.fCampID	= ((SimVehicleClass*)lastToHit)->GetCampaignObject()->GetCampID();
			message->dataBlock.fSide	= ((SimVehicleClass*)lastToHit)->GetCampaignObject()->GetOwner();
			campUnit = ((SimBaseClass*)lastToHit)->GetCampaignObject();
		}
		else {
			message->dataBlock.fPilotID	= 0;
			message->dataBlock.fCampID	= ((CampBaseClass*)lastToHit)->GetCampID();
			message->dataBlock.fSide	= ((CampBaseClass*)lastToHit)->GetOwner();
			campUnit =  (CampBaseClass*)lastToHit;
		}
		message->dataBlock.fCampID	= campUnit->GetCampID();
		message->dataBlock.fSide	= campUnit->GetOwner();
	}
	else {
		message->dataBlock.fEntityID	= vehicle->Id();
		message->dataBlock.fCampID		= vehicle->GetCampaignObject()->GetCampID();
		message->dataBlock.fSide		= vehicle->GetCampaignObject()->GetOwner();
		message->dataBlock.fPilotID		= vehicle->pilotSlot;
		message->dataBlock.fIndex		= vehicle->Type();
	}
	
	message->dataBlock.fWeaponID  = vehicle->Type();
	message->dataBlock.fWeaponUID.num_ = 0;
	message->dataBlock.damageType = FalconDamageType::GroundCollisionDamage;
	message->dataBlock.dEntityID  = vehicle->Id();
	message->dataBlock.dCampID = vehicle->GetCampaignObject()->GetCampID();
	message->dataBlock.dSide   = vehicle->GetCampaignObject()->GetOwner();
	message->dataBlock.dPilotID   = vehicle->pilotSlot;
	message->dataBlock.dIndex     = vehicle->Type();
	
	message->dataBlock.damageStrength = (float)damage;
	message->dataBlock.damageRandomFact = 0.0F;
	message->RequestOutOfBandTransmit ();
					
	return message;
}
