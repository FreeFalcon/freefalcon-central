#include "stdhdr.h"
#include "Graphics/Include/drawparticlesys.h"
#include "f4error.h"
#include "aircrft.h"
#include "airframe.h"
#include "otwdrive.h"
#include "simdrive.h"
#include "entity.h"
#include "cmpclass.h"
#include "initData.h"
#include "find.h"
#include "flight.h"
#include "ptdata.h"
#include "feature.h"
#include "fsound.h"
#include "soundfx.h"
#include "MsgInc/LandingMessage.h"
#include "MsgInc/DamageMsg.h"
#include "fakerand.h"
#include "falcsess.h"
#include "persist.h"
#include "Graphics/Include/terrtex.h"
#include "fack.h"
#include "playerop.h"
#include "sms.h"
#include "digi.h"
#include "sfx.h"
#include "dofsnswitches.h"
#include "Sim/Include/SimVuDrv.h"
#include "ui/include/uicomms.h" // JB 010107
extern UIComms *gCommsMgr; // JB 010107

void CalcTransformMatrix (SimBaseClass* theObject);

void AircraftClass::OnGroundInit(SimInitDataClass* initData)
{
	float nextX, nextY;
	float psi;
	CampEntity ent;
	SimBaseClass *carrier;
	
	// set lights and gear
	SetAcStatusBits(
		ACSTATUS_EXT_LIGHTS | ACSTATUS_GEAR_DOWN | ACSTATUS_EXT_NAVLIGHTS | ACSTATUS_EXT_NAVLIGHTSFLASH
	);

	//curGroundPt = initData->ptIndex - 1;
	af->ClearFlag(AirframeClass::InAir);
	af->SetFlag (AirframeClass::Planted);
	af->SetFlag (AirframeClass::ThrottleCheck);
	af->SetFlag (AirframeClass::EngineOff);
	af->SetFlag(AirframeClass::EngineOff2);//TJL 01/22/04 multi-engine
	af->zdot = 0.0F;
	af->gearPos = 1.0f;
	SetFlag (ON_GROUND);
	
	// Find our airbase;
	ent = ((Flight)(initData->campBase))->GetUnitAirbase();

	// check to see if our entity airbase is really a task force
	if ((ent) && (ent->IsTaskForce())){
		// try and get the carrier's position, otherwise just use the
		// task force position
		if ( !ent->IsAggregate() && ent->GetComponents() && (carrier = ent->GetComponentLead()) != NULL ){
			psi = carrier->Yaw();
			af->initialX = nextX = af->x = initData->x = carrier->XPos();
			af->initialY = nextY = af->y = initData->y = carrier->YPos();
	   		initData->z = OTWDriver.GetGroundLevel(initData->x, initData->y) -20.0f;
		}
		else {
			psi = ent->Yaw();
			af->initialX = nextX = af->x = initData->x = ent->XPos();
			af->initialY = nextY = af->y = initData->y = ent->YPos();
	   		initData->z = OTWDriver.GetGroundLevel(initData->x, initData->y) -20.0f;
		}
	}
	else {
		// Set the heading
		//TranslatePointData(ent, initData->ptIndex - 1, &nextX, &nextY);
		//psi = atan2 (nextY-initData->y, nextX-initData->x);
		psi = initData->heading;
	}
	
	// Set position and velocity
	//   initData->z = OTWDriver.GetGroundLevel(initData->x, initData->y);
	af->initialPsi = psi;
	af->initialMach = 0.0F;
	// sfr: try to fix sink
	af->initialZ    = initData->z - af->CheckHeight();
	SetPosition (initData->x, initData->y, af->initialZ);
	SetYPR(psi, 0.0F, 0.0F);
	SetDelta (0.0f, 0.0F, 0.0F);
	af->xdot = XDelta();
	af->ydot = YDelta();
	af->zdot = ZDelta();
	af->vt = 0.0F;
	if (isDigital){
		af->initialMach = (float)sqrt(XDelta()*XDelta() + YDelta()*YDelta());
	}
	SetYPRDelta (0.0F, 0.0F, 0.0F);
}

BOOL AircraftClass::LandingCheck( float noseAngle, float impactAngle, int groundType )
{
	VehicleClassDataType* vc;
	FalconDamageMessage* message;
	float sinkRate = af->auxaeroData->sinkRate; // JPO
	// Allow 14 degree error cos(14) = 0.9702 sin(14) = 0.2419
	float impactTest = 0.2419f;
	float gearAbsorption;
	Tpoint pos, vec;
	
	FeatureCollision(af->groundZ); // JPO force a check for onFlatFeature now!
	// Check for landing
	MonoPrint ("LandingCheck onFlatFeature %s\n", onFlatFeature == TRUE ? "TRUE" : "FALSE");
	if (af->IsSet(AirframeClass::Simplified))
	{
		sinkRate *= 2.0F;
	}
	else if(af->IsSet(AirframeClass::IsDigital))
	{
		impactTest *= 2.0F;
		sinkRate *= 2.0F;
	}

	if(IsSetFalcFlag(FEC_INVULNERABLE))
		sinkRate *= 2.0F;
	
	if (groundType == COVERAGE_WATER ||
	    groundType == COVERAGE_RIVER ||
	    ( !onFlatFeature && 
	    !af->IsSet(AirframeClass::OverRunway) &&
	    (groundType == COVERAGE_THINFOREST || 
	    groundType == COVERAGE_THICKFOREST || 
	    groundType == COVERAGE_ROCKY ||
	    groundType == COVERAGE_URBAN)) )
	{
		message = CreateGroundCollisionMessage(this, FloatToInt32(maxStrength));

		//we are not allowed to come to a stop on the water
		if(af->vt < 0.5F && (groundType == COVERAGE_WATER || groundType == COVERAGE_RIVER ))
			message->dataBlock.damageStrength = maxStrength;
		else if((float)rand()/(float)RAND_MAX < 0.01F)
		{
			vc = GetVehicleClassData(Type() - VU_LAST_ENTITY_TYPE);

			if( (float)rand()/(float)RAND_MAX < af->vt * impactAngle * 0.02F * af->Fuel()/vc->FuelWt )
				message->dataBlock.damageStrength = maxStrength;
		}
		else
		{
			if(groundType == COVERAGE_WATER	|| groundType == COVERAGE_RIVER)
				message->dataBlock.damageStrength = min(1000.0F, af->vt * impactAngle * 0.1F + 
												af->vt * af->vt * impactAngle * impactAngle * 0.02F);
			else
				message->dataBlock.damageStrength = min(1000.0F, af->vt * impactAngle * 0.1F + 
												af->vt * af->vt * impactAngle * impactAngle * 0.05F);

			if(message->dataBlock.damageStrength < 1.0F && (float)rand()/(float)RAND_MAX < 0.2F)
				message->dataBlock.damageStrength = 1.0F;
		}

		if(!IsSetFalcFlag(FEC_INVULNERABLE))
		{
			af->SetFlag(AirframeClass::EngineOff);
			af->SetFlag(AirframeClass::EngineOff2);//TJL 01/22/04 multi-engine
			mFaults->SetFault(FaultClass::eng_fault,
								FaultClass::fl_out, FaultClass::fail, FALSE);
			Sms->RipOffWeapons(noseAngle);
		}
		
		message->dataBlock.damageRandomFact = 0.0F;
		
		FalconSendMessage (message,TRUE);		
		return FALSE;
	}
	
	
	
	if (fabs(noseAngle) >= impactTest || 
		(!af->IsSet(AirframeClass::OnObject) && platformAngles.sinthe < 0.0F || platformAngles.sinthe < -0.01F) || // JB carrier
		platformAngles.cosphi < 0.94F)
	{
		// planted right into ground
		// edg note: don't set exploding any more.
		// all collision problems must now go thru ApplyDamage
		message = CreateGroundCollisionMessage(this, FloatToInt32(maxStrength));
		message->dataBlock.damageStrength =  min(1000.0F, af->vt * impactAngle * 0.1F + 
											af->vt * af->vt * impactAngle * impactAngle * 0.02F);
		if(message->dataBlock.damageStrength < 1.0F && (float)rand()/(float)RAND_MAX < 0.2F)
			message->dataBlock.damageStrength = 1.0F;
		
		vc = GetVehicleClassData(Type() - VU_LAST_ENTITY_TYPE);

		if((float)rand()/(float)RAND_MAX < 0.02F)
		{
			if( (float)rand()/(float)RAND_MAX < af->vt * impactAngle * 0.02F * af->Fuel()/vc->FuelWt )
				message->dataBlock.damageStrength = 1000.0F;
		}
		FalconSendMessage (message,TRUE);

		if(!IsSetFalcFlag(FEC_INVULNERABLE))
		{
			af->SetFlag(AirframeClass::EngineOff);
			af->SetFlag(AirframeClass::EngineOff2);//TJL 01/22/04 multi-engine
			mFaults->SetFault(FaultClass::eng_fault,
								FaultClass::fl_out, FaultClass::fail, FALSE);
			Sms->RipOffWeapons(noseAngle);
		}
		return FALSE;
	}
	
	if( af->gearPos > 0.8F && af->IsSet(AirframeClass::IsDigital) )
	{
		
//		if(!af->auxaeroData->animWheelRadius[0]) // only play the sound of the radius is 0, otherwise it'll be played in airframe::RunLandingGear()
			SoundPos.Sfx(af->auxaeroData->sndTouchDown);

		pos.x = XPos();
		pos.y = YPos();
		pos.z = ZPos() + 4.0f;
		vec.x = 0.0f;
		vec.y = 0.0f;
		vec.z = -12.0f;

		/*
		OTWDriver.AddSfxRequest(
				new SfxClass ( SFX_LANDING_SMOKE,		// type
				SFX_MOVES | SFX_NO_GROUND_CHECK | SFX_NO_DOWN_VECTOR | SFX_USES_GRAVITY,
				&pos,					// world pos
				&vec,					// vel vector
				5.3f,					// time to live
				5.0f ) );				// scale
				*/
		DrawableParticleSys::PS_AddParticleEx((SFX_LANDING_SMOKE + 1),
												&pos,
												&vec);
		return TRUE;
	}

	if (af->vt * impactAngle < sinkRate *(1.25F - (!
		(af->IsSet(AirframeClass::OverRunway)
		|| af->IsSet(AirframeClass::OnObject)) // JB carrier
		&& 
		!onFlatFeature && groundType != COVERAGE_ROAD) * 0.5F) && af->gearPos > 0.8F)
	{
		// ok touchdown
		
       if (af->vt * impactAngle > 1.0F)
    		//if(!af->auxaeroData->animWheelRadius[0])
			   SoundPos.Sfx( af->auxaeroData->sndTouchDown );

		pos.x = XPos();
		pos.y = YPos();
		pos.z = ZPos() + 4.0f;
		vec.x = 0.0f;
		vec.y = 0.0f;
		vec.z = -12.0f;

		/*
		OTWDriver.AddSfxRequest(
				new SfxClass ( SFX_LANDING_SMOKE,		// type
				SFX_MOVES | SFX_NO_GROUND_CHECK | SFX_NO_DOWN_VECTOR | SFX_USES_GRAVITY,
				&pos,					// world pos
				&vec,					// vel vector
				5.3f,					// time to live
				5.0f ) );				// scale
				*/
		DrawableParticleSys::PS_AddParticleEx((SFX_LANDING_SMOKE + 1),
												&pos,
												&vec);

		SetPulseTurbulence(0.2f, 0.2f, 1.0f * af->vt * impactAngle, 1.0f);

		return TRUE;
	}
	else if(af->vt * impactAngle < sinkRate * 1.75F *(1.0F - (!
		(af->IsSet(AirframeClass::OverRunway) 
		|| af->IsSet(AirframeClass::OnObject)) // JB carrier
		&& !onFlatFeature && groundType != COVERAGE_ROAD) * 0.5F) && af->gearPos > 0.8F)
	{
		//bounce
		
		//if(!af->auxaeroData->animWheelRadius[0])
			SoundPos.Sfx( af->auxaeroData->sndTouchDown);
		

		pos.x = XPos();
		pos.y = YPos();
		pos.z = ZPos() + 4.0f;;
		vec.x = 0.0f;
		vec.y = 0.0f;
		vec.z = -12.0f;

		/*
		OTWDriver.AddSfxRequest(
				new SfxClass ( SFX_LANDING_SMOKE,		// type
				SFX_MOVES | SFX_NO_GROUND_CHECK | SFX_NO_DOWN_VECTOR | SFX_USES_GRAVITY,
				&pos,					// world pos
				&vec,					// vel vector
				5.3f,					// time to live
				5.0f ) );				// scale
				*/
		DrawableParticleSys::PS_AddParticleEx((SFX_LANDING_SMOKE + 1),
												&pos,
												&vec);

		if (groundType == COVERAGE_OBJECT) // JB carrier
			return TRUE; // JB carrier

		SetPulseTurbulence(0.2f, 0.2f, 1.0f * af->vt * impactAngle, 1.0f);

		return FALSE;
	}
	else if(af->vt * impactAngle < sinkRate * 3.0F *(1.0F - (!af->IsSet(AirframeClass::OverRunway) && !onFlatFeature && groundType != COVERAGE_ROAD) * 0.5F) && af->gearPos > 0.8F)
	{
		//we hit too hard for the landing gear, crunch!		
		
		//if(!af->auxaeroData->animWheelRadius[0])
			SoundPos.Sfx( af->auxaeroData->sndTouchDown );
		
		SoundPos.Sfx( SFX_IMPACTG3 + rand() % 4);
		
		// edg note: I would have liked to put this stuff in faults, but
		// as far as I can tell we don't know xyz position there.
		if ( !IsSetFalcFlag(FEC_INVULNERABLE) && !af->IsSet( AirframeClass::GearBroken ) ){				    
			af->SetFlag(AirframeClass::EngineOff);
			af->SetFlag(AirframeClass::EngineOff2);//TJL 01/22/04 multi-engine
			mFaults->SetFault(FaultClass::eng_fault, FaultClass::fl_out, FaultClass::fail, FALSE);

			if(!af->IsSet( AirframeClass::GearBroken )){
				af->gearPos = 0.2F;
				for(int i = 0; i < af->NumGear(); i++){
					af->gear[i].flags |= GearData::GearProblem;
					SetDOF(ComplexGearDOF[i]/*COMP_NOS_GEAR + i*/, 0.0F);
				}

				mFaults->SetFault(FaultClass::gear_fault,
					FaultClass::ldgr, FaultClass::fail, TRUE);

				// gear breaks sound
				SoundPos.Sfx( SFX_BIND );
				
				af->SetFlag (AirframeClass::GearBroken);
			}
		}
		SetPulseTurbulence(0.5f, 0.5f, 3.0f * af->vt * impactAngle, 3.0f);
		return TRUE;
	}
	else
	{		
		SoundPos.Sfx( SFX_IMPACTG3 + rand() % 4 );
		
		if(!IsSetFalcFlag(FEC_INVULNERABLE))
		{
			// we're taking damage.....		
			message = CreateGroundCollisionMessage(this, FloatToInt32(maxStrength));
			
			gearAbsorption = sinkRate * 3.0F * !af->IsSet(AirframeClass::GearBroken) * af->gearPos;
			
			message->dataBlock.damageStrength = min(1000.0F, af->vt * impactAngle * 0.1F + 
												(af->vt * impactAngle - gearAbsorption) * 
												(af->vt * impactAngle - gearAbsorption) * 0.006F );

			if(message->dataBlock.damageStrength < 1.0F && (float)rand()/(float)RAND_MAX < 0.2F)
			{
				message->dataBlock.damageStrength = 1.0F;
			}

			vc = GetVehicleClassData(Type() - VU_LAST_ENTITY_TYPE);

			if((float)rand()/(float)RAND_MAX < 0.03F)
			{
				if( (float)rand()/(float)RAND_MAX < af->vt * impactAngle * 0.02F * af->Fuel()/vc->FuelWt )
					message->dataBlock.damageStrength = 1000.0F;
			}

			// RV - Biker - Damage for AI when crashed into gound
			if(af->IsSet(AirframeClass::IsDigital))
				message->dataBlock.damageStrength = max(message->dataBlock.damageStrength, maxStrength*(1.0f+(rand()%50)/100.0f));
			
			FalconSendMessage (message,TRUE);

			if (message->dataBlock.damageStrength > 20.0F)
			{
				af->SetFlag(AirframeClass::EngineOff);
				af->SetFlag(AirframeClass::EngineOff2);//TJL 01/22/04 multi-engine
				mFaults->SetFault(FaultClass::eng_fault,
								FaultClass::fl_out, FaultClass::fail, FALSE);
			}
			
			if (!af->IsSet( AirframeClass::GearBroken ) &&  af->gearPos > 0.0F){				    
				af->gearPos = 0.2F;
				for(int i = 0; i < af->NumGear(); i++){
					af->gear[i].flags |= GearData::GearProblem;
				}
				// JPO - change to only play for us.
				mFaults->SetFault(FaultClass::gear_fault,
					FaultClass::ldgr, FaultClass::fail, this == SimDriver.GetPlayerEntity());
				
				// gear breaks sound
				SoundPos.Sfx( af->auxaeroData->sndWheelBrakes );
				
				af->SetFlag (AirframeClass::GearBroken);
			}
			
			Sms->RipOffWeapons(noseAngle);
		}

		SetPulseTurbulence(0.5f, 0.5f, 3.0f * af->vt * impactAngle, 3.0f);

		if(af->vt * impactAngle < sinkRate * 2.0F)			
			return TRUE;
		else
			return FALSE;
	}
}


/*
** Name: GroundFeatureCheck
** Description:
**		Checks to see if we've hit a building (or something).
**		Ignores runways (groundcheck should handle this?)
**		If hit, then we're dead and send a message
**
**		TODO: we probably also need to send the feature a damage
**		message saying its been hit.
*/
void
AircraftClass::GroundFeatureCheck(float groundZ)
{
	FalconDamageMessage* message;
	SimBaseClass *hitFeature;
	
	if (OnGround() && af->vcas <= 50.0f  && gCommsMgr && gCommsMgr->Online()) // JB 010107
		return; // JB 010107

	// Decide if we hit anything
	hitFeature = FeatureCollision( groundZ );
	if (!hitFeature)
	{
		if ( onFlatFeature == TRUE )
			CheckPersistantCollision();
		return;
	}
	
	// send message to self
	// VuTargetEntity *owner_session = (VuTargetEntity*)vuDatabase->Find(OwnerId());
	message = new FalconDamageMessage (Id(), FalconLocalGame );
	message->dataBlock.fEntityID  = hitFeature->Id();
	ShiAssert(hitFeature->GetCampaignObject());
	message->dataBlock.fCampID = ((CampBaseClass*)(hitFeature->GetCampaignObject()))->GetCampID();
	message->dataBlock.fSide   = ((CampBaseClass*)(hitFeature->GetCampaignObject()))->GetOwner();
	message->dataBlock.fPilotID   = 255;
	message->dataBlock.fIndex     = hitFeature->Type();
	message->dataBlock.fWeaponID  = hitFeature->Type();
	message->dataBlock.fWeaponUID = hitFeature->Id();
	
	message->dataBlock.dEntityID  = Id();
	ShiAssert(GetCampaignObject());
	message->dataBlock.dCampID = ((CampBaseClass*)(GetCampaignObject()))->GetCampID();
	message->dataBlock.dSide   = ((CampBaseClass*)(GetCampaignObject()))->GetOwner();
	message->dataBlock.dPilotID   = pilotSlot;
	message->dataBlock.dIndex     = Type();
	message->dataBlock.damageType = FalconDamageType::FeatureCollisionDamage;	
	message->dataBlock.damageStrength = min(1000.0F, af->vt * 0.5F + af->vt * af->vt * 0.02F );	
	message->dataBlock.damageRandomFact = 1.0f;
	message->RequestOutOfBandTransmit ();
	FalconSendMessage (message,TRUE);
	
	// send message to feature
	// owner_session = (VuTargetEntity*)vuDatabase->Find(hitFeature->OwnerId());
	message = new FalconDamageMessage (hitFeature->Id(), FalconLocalGame );
	message->dataBlock.fEntityID  = Id();
	message->dataBlock.fCampID = ((CampBaseClass*)(GetCampaignObject()))->GetCampID();
	message->dataBlock.fSide   = ((CampBaseClass*)(GetCampaignObject()))->GetOwner();
	message->dataBlock.fPilotID   = pilotSlot;
	message->dataBlock.fIndex     = Type();
	message->dataBlock.fWeaponID  = Type();
	message->dataBlock.fWeaponUID = hitFeature->Id();
	
	message->dataBlock.dEntityID  = hitFeature->Id();
	message->dataBlock.dCampID = ((CampBaseClass*)(hitFeature->GetCampaignObject()))->GetCampID();
	message->dataBlock.dSide   = ((CampBaseClass*)(hitFeature->GetCampaignObject()))->GetOwner();
	message->dataBlock.dPilotID   = 255;
	message->dataBlock.dIndex     = hitFeature->Type();
	message->dataBlock.damageType = FalconDamageType::ObjectCollisionDamage;	
	message->dataBlock.damageStrength = min(1000.0F, af->vt * 0.5F + af->vt * af->vt * 0.02F );
	message->dataBlock.damageRandomFact = 1.0f;
	message->RequestOutOfBandTransmit ();
	FalconSendMessage (message,TRUE);

	if ( OnGround() )
	{
		// poke airframe data so we don't go through features
		af->xdot = -af->xdot;
		af->ydot = -af->ydot;
		af->x += af->xdot * 3.0F * SimLibMinorFrameTime;
		af->y += af->ydot * 3.0F * SimLibMinorFrameTime;
		af->vt = 0.0f;
	}	
}


// okay -- this is ugly....
// used by persistant list collision check
extern SimPersistantClass*	PersistantObjects;
extern int persistantListTail;

/*
** Name: CheckPersistantCollision
** Description:
**  Walks the persistant list and checks to see if we've collided
**  any persistant objects (ie craters)
*/
void AircraftClass::CheckPersistantCollision()
{
	int i;
	SimPersistantClass *testP;
	float radius;
	Tpoint fpos;
	FalconDamageMessage* message;
	
	
	for (i = 0; i < MAX_PERSISTANT_OBJECTS; i++){
		// get the object
		if (!PersistantObjects[i].InUse() || !PersistantObjects[i].drawPointer){
			continue;
		}
		
		testP = &PersistantObjects[i];
		
		// get feature's radius and position
		radius = testP->drawPointer->Radius();
		testP->drawPointer->GetPosition( &fpos );
		
		// test with gross level bounds of object
		if (fabs (XPos() - fpos.x) < radius  &&
			fabs (YPos() - fpos.y) < radius  &&
			fabs (ZPos() - fpos.z) < radius  )
		{
            message = new FalconDamageMessage (Id(), FalconLocalGame );
            message->dataBlock.fEntityID  = Id();
			ShiAssert(GetCampaignObject());
            message->dataBlock.fCampID = ((CampBaseClass*)(GetCampaignObject()))->GetCampID();
            message->dataBlock.fSide   = ((CampBaseClass*)(GetCampaignObject()))->GetOwner();
            message->dataBlock.fPilotID   = pilotSlot;
            message->dataBlock.fIndex     = Type();
            message->dataBlock.fWeaponID  = Type();
            message->dataBlock.fWeaponUID = Id();
			
            message->dataBlock.dEntityID  = Id();
            message->dataBlock.dCampID = ((CampBaseClass*)(GetCampaignObject()))->GetCampID();
            message->dataBlock.dSide   = ((CampBaseClass*)(GetCampaignObject()))->GetOwner();
            message->dataBlock.dPilotID   = pilotSlot;
            message->dataBlock.dIndex     = Type();
            message->dataBlock.damageType = FalconDamageType::GroundCollisionDamage;
			
            // for now use maxStrength as amount of damage.
            // later we'll want to add other factors into the equation --
            // on ground, speed, etc....
            message->dataBlock.damageStrength = maxStrength * af->vt/250.0f;
            message->dataBlock.damageRandomFact = PRANDFloat();
			message->RequestOutOfBandTransmit ();
            FalconSendMessage (message,TRUE);
			
            // "bounce" the aircraft
            af->z -= 5.0f;
			
			return;
		}
	}
	
}
