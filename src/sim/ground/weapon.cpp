#include "stdhdr.h"
#include "mesg.h"
#include "guns.h"
#include "Graphics/Include/drawsgmt.h"
#include "object.h"
#include "otwdrive.h"
#include "sfx.h"
#include "falcmesg.h"
#include "fcc.h"
#include "sms.h"
#include "MsgInc/WeaponFireMsg.h"
#include "hardpnt.h"
#include "battalion.h"
#include "Simdrive.h"
#include "missile.h"
#include "falcsess.h"
#include "fsound.h"
#include "soundfx.h"
#include "fakerand.h"
#include "camp2sim.h"
#include "vehicle.h"
#include "Unit.h"
#include "Radar.h"
#include "BeamRider.h"
#include "IRST.h"
#include "ground.h"
#include "dofsnswitches.h"
#include "missile.h"//me123
#include "missdata.h" // 2002-03-08 S.G.
#include "Graphics/Include/drawparticlesys.h"

extern float g_fTracerAccuracyFactor; // 2002-03-12
extern bool g_bToggleAAAGunFlag; // 2002-03-12
extern bool g_bFireOntheMove; // FRB - Test

/*
** WeaponKeepAlive is called for servicing weapons on non-execed
** frames.
*/

void GroundClass::WeaponKeepAlive (void)
{
	static const int	numToFly = 3;
	Tpoint				pos, vec;
	int					i;
	int					fire;
	int					whatWasHit;
	GunClass			*Gun;
	BOOL				keepAliveCheck = FALSE;

	// Rewritten by Kevin on 6/22
	// loop thru all weapons looking for guns
    for (i=0; i<Sms->NumHardpoints(); i++)
		{
		// point to Gun Class
		Gun = Sms->GetGun(i);
		if (Gun)
			{
			// make sure a relative position is set
			Gun->SetPosition( 2.0f, 0.0f, -4.0f, 0.0f, 0.0f );

			// if its a tracer....
			if ( Gun->IsTracer() )
			{
				// if the Z component of the X basis vector (dir of fire) is pointing
				// downward or level, allow only 1 shot to fly.  This will prevent
				// shots that go into the ground causing continual fire
				// if ( gunDmx[0][2] >= 0.0f )
				// 	numToFly = 1;
			
				// edg NOTE: I think it will work out better to keep tracer fire going
				// once they've started firing until the next process cycle.  This logic
				// will have to change once we have differentiated gun types.  The original
				// gun matrix (when started firing) is now kept in the ground class...
				if ( Gun->numFlying )
				{
					keepAliveCheck = TRUE;

					// allow a limited number of tracers to be firing in the keepalive stage
					if ( IsGunFiring(i) && Gun->numFlying < numToFly )
					{
						fire = TRUE;
						SoundPos.Sfx( SFX_MCGUN, 0, 1.0, 0);
			
						pos.x = XPos();
						pos.y = YPos();
						pos.z = ZPos() - 5.0f;

						// this is a hack right now -- it seems like the
						// unit aren't always set right on the ground
						SetPosition(
							XPos(),
							YPos(),
	  						OTWDriver.GetGroundLevel( XPos(), YPos() ) - 0.7f);
			
						vec.x = PRANDFloat() * 30.0f;
						vec.y = PRANDFloat() * 30.0f;
						vec.z = -PRANDFloatPos() * 30.0f;
			
						//RV - I-Hawk - changing all LIGHT_CLOUD weapons effects calls
						// to more appropriate GUNSMOKE effect
						/*
						OTWDriver.AddSfxRequest(
							new SfxClass ( SFX_GUNSMOKE,		// type
							SFX_MOVES,
							&pos,					// world pos
							&vec,					// vel vector
							2.3F,					// time to live
							2.0f ) );				// scale
							*/
						DrawableParticleSys::PS_AddParticleEx((SFX_GUNSMOKE + 1),
									&pos,
									&vec);
					}
					else
					{
						fire = FALSE;
						UnSetGunFiring( i );
					}

					// service the guns.   We need to keep any tracers that have been fired
					// flying and detecting collisions.  The 2nd 2 args aren't used when
					// we aren't actually starting new tracers.
					whatWasHit = Gun->Exec(&fire, gunDmx, &platformAngles, targetPtr, FALSE );

					// if we hit something, stop firing....
					if ( whatWasHit != TRACER_HIT_NOTHING )
					{
						UnSetGunFiring( i );

						// debug....
						if ( whatWasHit & TRACER_HIT_GROUND )
						{
							MonoPrint( "Tracer keepalive stopped.  Zpos = %3.3f Ground = %3.3f, GunZ = %3.3f\n",
										ZPos(),
	  									OTWDriver.GetGroundLevel( XPos(), YPos() ),
										gunDmx[0][2] );
						}
					}
			
				}
			}
			else // a shell
			{
				if ( Gun->shellTargetPtr )
				{
					// service the guns.   We need to keep any tracers that have been fired
					// flying and detecting collisions.  The 2nd 2 args aren't used when
					// we aren't actually starting new tracers.
					Gun->Exec(&fire, gunDmx, &platformAngles, targetPtr, FALSE );
					keepAliveCheck = TRUE;
				}
			}
        } // end is gun
    } // end for

	// set need for future keepalives
	needKeepAlive = keepAliveCheck;
}

BOOL GroundClass::DoWeapons (void){

	Tpoint pos;

	//RV - I-Hawk - Added a 0 vector for RV new PS calls
	Tpoint PSvec;
	PSvec.x = 0;
	PSvec.y = 0;
	PSvec.z = 0;

	// sfr: we need the bin to avoid weapon destruction when its detached
	VuBin<SimWeaponClass> theWeapon(Sms->GetCurrentWeapon());
	//SimWeaponClass *theWeapon;
	//theWeapon.reset(Sms->GetCurrentWeapon());
	//theWeapon.reset(Sms->GetCurrentWeapon());
	if (!theWeapon){ return FALSE; }

	// RV - Biker - Don't do missiles when heavy damaged
	//if (theWeapon->IsMissile() && pctStrength < 0.5f) { return FALSE; }

	// RV - Biker - Don't do guns when heavy damaged
	//if (theWeapon->IsGun() && pctStrength < 0.0f) { return FALSE; }

	if (theWeapon->IsGun()){
		// point to Gun Class
		GunClass *Gun = static_cast<GunClass*>(theWeapon.get());
		//GunClass *Gun = static_cast<GunClass*>(theWeapon);
		
		// make sure a relative position is set
		Gun->SetPosition( 2.0f, 0.0f, -4.0f, 0.0f, 0.0f );
		
		if (GunTrack()){
			if (!IsGunFiring(Sms->GetCurrentWeaponHardpoint()) || Gun->IsShell() ){
				//me123 disabled for now...its looking realy strange in mp 
				if (!Gun->FiremsgsendTime){
					SendFireMessage ((SimWeaponClass*)Gun, FalconWeaponsFire::GUN, TRUE, targetPtr);
				}
				Gun->FiremsgsendTime = vuxRealTime;
			}

			if ( Gun->IsTracer() ){
				SoundPos.Sfx( SFX_MCGUN);
				SetGunFiring(Sms->GetCurrentWeaponHardpoint());
				// 2000-10-12 MODIFIED BY S.G. THIS IS TO SAY WE TOOK A TRACER GUN SHOT
				// 2000-10-27 ADDED BY S.G. 
				// SO GUNS CAN HAVE DIFFERENT FIRE RATE... BUT NOT IN RP4 SO REMOVED FROM HERE AS WELL
				//if (Gun->wcPtr->Flags & 0x80)
					return TRUE + 1;			// Slow fire rate gun
				//else
				//	return TRUE + 2;			// Fast fire rate gun
				// END OF MODIFIED SECTION
			}
			return TRUE;	// We took a shot
		}
		else {
			if (1/*IsGunFiring(Sms->GetCurrentWeaponHardpoint())*/){
				if (Gun->FiremsgsendTime && vuxRealTime - Gun->FiremsgsendTime > 20000){
					//me123 disabled for now...its looking realy strange in mp	
					SendFireMessage ((SimWeaponClass*)Gun, FalconWeaponsFire::GUN, FALSE, targetPtr);
					Gun->FiremsgsendTime =0;
				}
			}
			UnSetGunFiring(Sms->GetCurrentWeaponHardpoint());
		}
	}
	else if (theWeapon->IsMissile()){
		// point to the missile
		MissileClass *theMissile = static_cast<MissileClass*>(theWeapon.get());
		//MissileClass *theMissile = static_cast<MissileClass*>(theWeapon);
		if (MissileTrack()){
			// 2002-02-28 MODIFIED BY S.G. 
			// Valid for campaign object as well, not just sim! 
			// We're testing inside for SIM or CAMPAIGN ANYWAY! Otherwise nextSamFireTime is NEVER adjusted
			if (
				gai->battalionCommand && 
				/* targetPtr->BaseData()->IsSim() &&*/ 
				!targetPtr->BaseData()->OnGround()
			){
				gai->battalionCommand->self->allowSamFire = FALSE;
				int numVeh = 1;
				// 2002-02-28 MODIFIED BY S.G. Different way of doing Joel's "011018" CTD fix. 
				// If it's not a sim object, it doesn't have a campaignObject member variable so that test isn't valid
				if (targetPtr->BaseData()->IsSim()) { 
					if (((SimBaseClass*)targetPtr->BaseData())->GetCampaignObject()){
						numVeh = max(
							1, ((Unit)((SimBaseClass*)targetPtr->BaseData())->GetCampaignObject())->GetTotalVehicles()/2
						);
					}
				}
				else {
					numVeh = max(1, ((Unit)targetPtr->BaseData())->GetTotalVehicles()/2);
				}
				
				int rate;
				// 2002-02-28 MODIFIED BY S.G. 
				// A GetRadarType() will return 0 (RDR_NO_RADAR) 
				// if it has no radar but radarDatFileTable has something there so it's being used. That's not right.
				//if (radarData->AirFireRate) rate = radarData->AirFireRate;
				// Default to MAX_SAM_FIRE_RATE
				
				// RV - Biker - For IR missiles we don't have a fire rate
				rate = MAX_SAM_FIRE_RATE;
				VehicleClassDataType* vc = GetVehicleClassData(Type() - VU_LAST_ENTITY_TYPE);
				ShiAssert (vc);
				// If the vehicle's VEH_USES_UNIT_RADAR flag is set, 
				// forget about your own (if you have one) and use the unit's radar vehicle
				if (vc->Flags & VEH_USES_UNIT_RADAR) 
				{
					RadarDataSet* radarData = &radarDatFileTable[GetCampaignObject()->GetRadarType()];
					if (radarData->AirFireRate && GetCampaignObject()->GetRadarType())
					{
						rate = radarData->AirFireRate;
					}
				}
				// Otherwise rate will be what our own radar AirFireRate says
				else 
				{
					RadarDataSet* radarData = &radarDatFileTable[GetRadarType()];
					if (radarData->AirFireRate && GetRadarType())
					{
						rate = radarData->AirFireRate;
					}
				}
				// rate is the value for aces, scale it down based on skill
				rate += rate - rate / (5 - gai->skillLevel);
				// END OF MODIFIED SECTION 2002-02-28

				gai->battalionCommand->self->nextSamFireTime = SimLibElapsedTime + rate/numVeh;
			}

			SendFireMessage (theMissile, FalconWeaponsFire::MRM, TRUE, targetPtr);
			
			// Special case for beam riders
			if ( theMissile->sensorArray && theMissile->sensorArray[0]->Type() == SensorClass::RadarHoming )
			{
				// Have the missile use the radar vehicle's radar for guidance
				((BeamRiderClass*)theMissile->sensorArray[0])->SetGuidancePlatform( battalionFireControl );
			}
			
			// Get it in the air
			Sms->LaunchWeapon();
			theMissile->Start(targetPtr);
			
			// It's Alive!
			vuDatabase->/*Quick*/Insert(theWeapon.get());
			//vuDatabase->/*Quick*/Insert(theWeapon);
			theMissile->Wake();
			SoundPos.Sfx( SFX_MISSILE1, 0, 1.0, 0);
			
			pos.x = XPos();
			pos.y = YPos();
			pos.z = ZPos();
			
			// Add a glow arround the launcher
			/*
			OTWDriver.AddSfxRequest(
				new SfxClass ( SFX_GROUND_FLASH,	// type
				&pos,								// world pos
				1.0f,								// time to live
				201.5f ) );							// scale
				*/
			DrawableParticleSys::PS_AddParticleEx((SFX_GROUND_FLASH + 1),	&pos, &PSvec);

			// RV - Biker - Add some smoke to the launcher
			Tpoint vec;
			vec.x = 0.0f;
			vec.y = 0.0f;
			vec.z = 0.0f;
			/*
			OTWDriver.AddSfxRequest( new SfxClass( SFX_SAM_LAUNCH,
								SFX_MOVES | SFX_NO_GROUND_CHECK,
								&pos,
								&vec,
								2.0f,
								1.0f ) );
								*/
			DrawableParticleSys::PS_AddParticleEx((SFX_SAM_LAUNCH + 1), &pos, &vec);

			return TRUE;	// We took a shot
		}
	}

	return FALSE;	// We didn't take a shot
}

void GroundClass::RotateTurret(void)
{
	float newAz, newEl;

	if ( isFootSquad )
	{
		float newAz = targetPtr->localData->az - Yaw();	
		float newEl = targetPtr->localData->el - Pitch();
		//Let foot soldiers have instant aiming ability
		SetDOF(AIRDEF_AZIMUTH, newAz);
		SetDOF(AIRDEF_ELEV, newEl);
	}
	else
	{
		if(!targetPtr->BaseData()->OnGround() && !isAirDefense)
		{
			newAz = 0.0F;
			newEl = 0.0F;
		}
		else
		{
			newAz = targetPtr->localData->az - Yaw();	

			if(isAirDefense)
				newEl = min(targetPtr->localData->el, 85.0F*DTR) - Pitch();
			else
				newEl = min(targetPtr->localData->el, 45.0F*DTR) - Pitch();
		}
		// Rotate our DOFs towards our target, if any - or toward front
		// KCK: This stuff could be LODed out with ed's IsHidden() check to save time
		float maxAz = TURRET_ROTATE_RATE * SimLibMajorFrameTime;
		float maxEl = TURRET_ELEVATE_RATE * SimLibMajorFrameTime;
		
		float delta = newEl - GetDOFValue(AIRDEF_ELEV);
		if(delta > 180.0F*DTR)
			delta -= 360.0F*DTR;
		else if(delta < -180.0F*DTR)
			delta += 360.0F*DTR;

		// Do elevation adjustments
		if (delta > maxEl)
		    SetDOFInc(AIRDEF_ELEV, maxEl);
		else if (delta < -maxEl)
		    SetDOFInc(AIRDEF_ELEV, -maxEl);
		else
		    SetDOF(AIRDEF_ELEV, newEl);

		SetDOF(AIRDEF_ELEV, min(85.0F*DTR, max(GetDOFValue(AIRDEF_ELEV), 0.0F)));
		SetDOF(AIRDEF_ELEV2, GetDOFValue(AIRDEF_ELEV));
		
		// RV - Biker - For vehicles which have radar not looking forward by default read offset from FueFlow data
		VehicleClassDataType* vc = GetVehicleClassData(Type() - VU_LAST_ENTITY_TYPE);

		delta = newAz - GetDOFValue(AIRDEF_AZIMUTH) - min(max(0.0F, vc->FuelEcon*DTR), 360.0F*DTR);

		if(delta > 180.0F*DTR)
			delta -= 360.0F*DTR;
		else if(delta < -180.0F*DTR)
			delta += 360.0F*DTR;

		// Now do the azmuth adjustments
		if (delta > maxAz)
		    SetDOFInc(AIRDEF_AZIMUTH, maxAz);
		else if (delta < -maxAz)
		    SetDOFInc(AIRDEF_AZIMUTH, -maxAz);
		// RV - Biker - Don't do this
		//else
		//	SetDOF(AIRDEF_AZIMUTH, newAz);
	}
	
}

int GroundClass::GunTrack (void)
{
	float xft, yft, zft;
	float az, el, tof;
	FalconEntity	*target;
	int fire = FALSE;
	float realRange, delta;
	Tpoint pos, vec;
	mlTrig trigtha,trigpsi;
	SimWeaponClass	*theWeapon;
	WeaponClassDataType *wc;

	//RV - I-Hawk - Added a 0 vector for RV new PS calls
	Tpoint PSvec;
	PSvec.x = 0;
	PSvec.y = 0;
	PSvec.z = 0;

	theWeapon = Sms->GetCurrentWeapon();

	ShiAssert( targetPtr );
	ShiAssert( theWeapon );
	ShiAssert( theWeapon->IsGun() );


	// point to Gun Class
	GunClass	*Gun = (GunClass*)theWeapon;

	wc = &WeaponDataTable[Sms->hardPoint[Sms->CurHardpoint()]->weaponId];

	target = targetPtr->BaseData();
	if (!target)
		return FALSE;

	// make guns less accurate by randomizing target position when in air, unless its AAA
// 2000-10-19 MODIFIED BY S.G. NEW TEST FOR AAA AND REGULAR GUNS. IF FIRST BIT OF NEW FIELD IS A 1, IT'S AN AAA GUN.
//	if (!target->OnGround() && !isAirDefense)
	float tracerError; // JB 010106
	if ((((unsigned char *)Gun->wcPtr)[31] & 1) == (g_bToggleAAAGunFlag & 1) && !target->OnGround()) // 2002-03-12 MODIFIED BY S.G. Added the 'g_bToggleAAAGunFlag' check to possibly reverse the check. RP5 reversed the check so this deals with it
	{
		//float tracerError; // JB 010106
		tracerError = PRANDFloatPos() / (float)(gai->skillLevel+1);
// 2000-10-19 MODIFIED BY S.G. THE TRACE ERROR WILL BE BASED ON THE GUN'S PERFORMANCE WHICH IS STORED IN THE LAST BYTE OF THE NAME OF THE WEAPON IN VAL / 4 FORMAT
//		tracerError = (5.0f + tracerError) * 360.0f;
//		tracerError = (5.0f + tracerError) * (float)(((unsigned int)(((unsigned char *)Gun->wcPtr)[31])) << 2);
// 2002-03-12 MODIFIED BY S.G. Looking at the assembly of RP5, this was coded wrong here :-(
		tracerError = 5.0f + (tracerError * (float)(((unsigned int)(((unsigned char *)Gun->wcPtr)[31])) << 2));

		tracerError *= g_fTracerAccuracyFactor; // JB 010104 hack to make AAA more accurate 2002-03-12 MODIFIED BY S.G. Use the exported g_fTracerAccuracyFactor variable instead so we can tweak it without editing the code

		xft = target->XPos() + tracerError * PRANDFloat();
		yft = target->YPos() + tracerError * PRANDFloat();
	}
	else
	{
// 2000-10-12 MODIFIED BY S.G. isAirDefense VEHICLE NEEDS TO VARIATE A BIT AS WELL
//		xft = target->XPos();
//		yft = target->YPos();
// 2000-10-19 MODIFIED BY S.G. THE TRACE ERROR WILL BE BASED ON THE GUN'S PERFORMANCE WHICH IS STORED IN THE LAST BYTE OF THE NAME OF THE WEAPON IN VAL / 4 FORMAT
		tracerError = (float)(((unsigned int)(((unsigned char *)Gun->wcPtr)[31])) << 2); // JB 010106

		tracerError *= g_fTracerAccuracyFactor; // JB 010106 hack to make AAA more accurate 2002-03-12 MODIFIED BY S.G. Use the exported g_fTracerAccuracyFactor variable instead so we can tweak it without editing the code

		xft = target->XPos() + tracerError * PRANDFloat();
		yft = target->YPos() + tracerError * PRANDFloat();
	}

	xft -= XPos();
	yft -= YPos();
	zft = target->ZPos() - ZPos() + 4.0f;
	realRange = (float)sqrt( xft * xft + yft * yft + zft * zft );

	// Guess TOF
	tof = realRange / Gun->initBulletVelocity;

	// now get vector to where we're aiming
	xft += target->XDelta() * tof;
	yft += target->YDelta() * tof;
	zft += target->ZDelta() * tof;

	// Correct for gravity
// 2000-10-24 I'LL CALCULATE THE GRAVITY THE 'RIGHT' WAY (NOT IN RP4 SO REMOVED - IN FACT, zft IS NOT ADJUSTED IN RP4)
//	zft += 0.5F * GRAVITY * tof * tof;
//	zft -= (tof * tof + tof) / 2 * GRAVITY;

	// KCK : Copy these values into our targetPtr.
	// Note: This factors our induced error and target's vector into the Rel Geometry data,
	// but as we use this only for aiming and targetting, it shouldn't matter.
	targetPtr->localData->az = (float)atan2(yft,xft);
	targetPtr->localData->el = (float)atan(-zft/(float)sqrt(xft*xft + yft*yft +0.1F));
	targetPtr->localData->range = realRange;

	az = targetPtr->localData->az - Yaw();
	el = targetPtr->localData->el - Pitch();

	RotateTurret();

	// if we are not an air defense unit, but we're shooting guns at something in
	// the air, it will be small arms so it is irrelevant which way our main gun
	// is pointing
	if(target->OnGround() || isAirDefense)
	{
		// Check our limits
		if (el > 85*DTR || (el < 5*DTR && !target->OnGround()))
			return FALSE;

		delta = (float)fabs(GetDOFValue(AIRDEF_AZIMUTH) - az);
		if(delta > 180.0F*DTR)
			delta -= 180.0F*DTR;
		if ( delta > 15*DTR )
			return FALSE;

		delta = (float)fabs(GetDOFValue(AIRDEF_ELEV) - el);
		if(delta > 180.0F*DTR)
			delta -= 180.0F*DTR;
		if ( delta > 15*DTR )
			return FALSE;
	}

	// KCK: I've noticed some vehicles (ie: artillery) don't use/have DOFs. 
	// An idea to think about is physically rotating the vehicle instead. 
	// Just food for thought right now
	mlSinCos (&trigtha, targetPtr->localData->el);
	mlSinCos (&trigpsi, targetPtr->localData->az);

	gunDmx[0][0] = trigpsi.cos*trigtha.cos;
	gunDmx[0][1] = trigpsi.sin*trigtha.cos;
	gunDmx[0][2] = -trigtha.sin;

	gunDmx[1][0] = -trigpsi.sin;
	gunDmx[1][1] = trigpsi.cos;
	gunDmx[1][2] = 0.0f;

	gunDmx[2][0] = trigpsi.cos*trigtha.sin;
	gunDmx[2][1] = trigpsi.sin*trigtha.sin;
	gunDmx[2][2] = trigtha.cos;

	fire = TRUE;

	if ( Gun->IsTracer() )
		{

		Gun->Exec(&fire, gunDmx, &platformAngles, targetPtr, FALSE );
		if ( fire )
			{
			needKeepAlive = TRUE;
			SoundPos.Sfx( SFX_MCGUN, 0, 1.0, 0);
//			MonoPrint( "Ground Unit Firing tracers at %s, flying = %d, rounds = %d, unlim = %d\n",
//					   target->OnGround() ? "Ground Unit" : "Air Unit",
//					   Gun->numFlying,
//					   Gun->numRoundsRemaining,
//					   Gun->unlimitedAmmo );
			}
		else
			{
//			MonoPrint( "Ground Unit unable to fire tracer flying = %d, rounds = %d, unlim = %d\n",
//					   Gun->numFlying,
//					   Gun->numRoundsRemaining,
//					   Gun->unlimitedAmmo );
			}
		}
	// RV - Biker check max firing height for AAA (flak)
	else {
		if (!targetPtr->BaseData()->OnGround()) {
			WeaponClassDataType* wc = Gun->GetWCD();
			float maxFireRange;

			if (battalionFireControl || rand()%100 < 10)
				maxFireRange = float(wc->Range*KM_TO_FT);
			else
				maxFireRange = float(wc->MaxAlt*KM_TO_FT*0.666f);

			if (targetPtr->localData->range > maxFireRange)
				return FALSE;
		}

		// edg: not really the best place for this, but....
		// we need to prevent flak from firing when aircraft are flying
		// low -- it should be a gameplay feature.  SelectWeapon isn't
		// being granular enough....
		if ( !targetPtr->BaseData()->OnGround() &&
			 targetPtr->BaseData()->IsSim() &&
			 targetPtr->BaseData()->ZPos() - ZPos() > -2000.0f )
	 	{
			return FALSE;
		}

		needKeepAlive = TRUE;
		Gun->FireShell( targetPtr );

//		MonoPrint( "Ground Unit Firing shell at %s\n",
//				   target->OnGround() ? "Ground Unit" : "Air Unit" );

		// sound effect
		if ( rand() & 1 )
			SoundPos.Sfx( SFX_BIGGUN1, 0, 1.0, 0);
		else
			SoundPos.Sfx( SFX_BIGGUN2, 0, 1.0, 0);

		pos.x = XPos() + gunDmx[0][0] * 5.0f;
		pos.y = YPos() + gunDmx[0][1] * 5.0f;
		pos.z = ZPos() - 5.0f + gunDmx[0][2] * 5.0f;

		vec.x = XDelta();
		vec.y = YDelta();
		vec.z = 0.0f;
//Cobra TJL Change this to SFX_GUN_SMOKE
		/*
		OTWDriver.AddSfxRequest(
			new SfxClass ( SFX_GUN_SMOKE,		// type
			SFX_MOVES,
			&pos,					// world pos
			&vec,					// world pos
			1.3f,					// time to live
			7.0f ) );				// scale
			*/
		DrawableParticleSys::PS_AddParticleEx((SFX_GUN_SMOKE + 1),
									&pos,
									&vec);
/*
		pos.x = XPos() + gunDmx[0][0] * 8.0f;
		pos.y = YPos() + gunDmx[0][1] * 8.0f;
		pos.z = ZPos() - 5.0f + gunDmx[0][2] * 8.0f;

		vec.z = -10.0f;*/
/* Cobra TJL 11/06/04 Removed per Steve for new PS file.
		OTWDriver.AddSfxRequest(
			new SfxClass ( SFX_LIGHT_CLOUD,		// type
			SFX_MOVES,
			&pos,					// world pos
			&vec,					// vel vector
			4.3f,					// time to live
			0.5f ) );				// scale

		pos.x = XPos() + gunDmx[0][0] * 13.0f;
		pos.y = YPos() + gunDmx[0][1] * 13.0f;
		pos.z = ZPos() - 5.0f + gunDmx[0][2] * 13.0f;

		OTWDriver.AddSfxRequest(
			new SfxClass ( SFX_LIGHT_CLOUD,		// type
			SFX_MOVES,
			&pos,					// world pos
			&vec,					// vel vector
			4.3f,					// time to live
			1.0f ) );				// scale

		pos.x = XPos() + gunDmx[0][0] * 20.0f;
		pos.y = YPos() + gunDmx[0][1] * 20.0f;
		pos.z = ZPos() - 5.0f + gunDmx[0][2] * 20.0f;

		OTWDriver.AddSfxRequest(
			new SfxClass ( SFX_LIGHT_CLOUD,		// type
			SFX_MOVES,
			&pos,					// world pos
			&vec,					// vel vector
			4.3f,					// time to live
			1.5f ) );				// scale

		pos.x = XPos();
		pos.y = YPos();
		pos.z = ZPos();
		
		OTWDriver.AddSfxRequest(
			new SfxClass ( SFX_GROUND_FLASH,		// type
			&pos,					// world pos
			4.3f,					// time to live
			201.5f ) );				// scale */
		}
	return (fire);
}

int GroundClass::MissileTrack(void)
{
	float				zft;
	float				az, el;
	FalconEntity		*target;
	MissileClass		*theMissile;
	WeaponClassDataType	*wc;
	float				minAlt, maxAlt;

	theMissile = (MissileClass*)Sms->GetCurrentWeapon();

	ShiAssert( targetPtr );
	ShiAssert( theMissile );
	ShiAssert( theMissile->IsMissile() );
	RadarDataSet* radarData = &radarDatFileTable[GetCampaignObject()->GetRadarType()];


	// We don't want to shoot missiles while we're on the move
	// TODO:  Would be nice to track "Set up" as a state and have some
	//        delay between stop of motion and ready to fire (several 
	//        minutes at least)

	// RV - Biker - Think here is a problem
	// FRB - Increased VT = 1 to VT = 3, same as GMT threshold
	if (!isShip && (GetVt() > 3.0f && !g_bFireOntheMove))
		return FALSE;

	// check for radar-guided missiles
	if (theMissile->sensorArray) {
		switch ( theMissile->sensorArray[0]->Type() )
		{
		  case SensorClass::RadarHoming:
			{
				// Make sure this battalion at least has a radar assigned to it (though it could be dead)
				ShiAssert( GetCampaignObject()->GetRadarType() );
				
				// if we don't have a fire control radar, don't launch
				if ( !battalionFireControl)
					return FALSE;
				
				// Shoot at our fire control radar's target
				// (Kinda annoying to go to the trouble of picking a target for this vehicle,
				// then ignoring the choice here, but oh well.)
				RadarClass *radar = (RadarClass*)FindSensor( battalionFireControl, SensorClass::Radar );
				ShiAssert( radar );
				if(!radar->CurrentTarget() || targetPtr->BaseData() != radar->CurrentTarget()->BaseData())
					SetTarget( radar->CurrentTarget() );

// ADDED BY S.G. SO SAM DO NOT NORMALLY FIRE WHEN JAMMED. DEPENDING ON THE SKILL, THEY MIGHT FIRE THOUGH
				if (radar->CurrentTarget() && radar->CurrentTarget()->localData->sensorState[SensorClass::Radar] != SensorClass::SensorTrack && (rand() % 1000 >= (4 - gai->skillLevel) * (4 - gai->skillLevel) * 10))
					return FALSE;
// END OF ADDED SECTION				

				// Make sure we still have a target after all the above contortions
				if ( !targetPtr )
					return FALSE;
			}
			break;
			
		  case SensorClass::IRST:
			{
				// Don't launch until the seeker sees the target
				theMissile->SetPosition( XPos(), YPos(), ZPos() );
				if ( !((IrstClass*)theMissile->sensorArray[0])->CanDetectObject( targetPtr ) )
				{
					return FALSE;
				}
			}
			break;
		}
	}
	
	// get target entity
	target = targetPtr->BaseData();
	ShiAssert( target );



	
	if (radarData->AverageSpeed)//me123
	{
	vector collPoint;
	FindCollisionPoint ((SimBaseClass*)targetPtr->BaseData(), (SimVehicleClass*)this, &collPoint, radarData->AverageSpeed*NM_TO_FT);
	float xft,yft,zft,rx_t;
	float range;
	FalconEntity* theObject;
	mlTrig	psi;

	theObject = targetPtr->BaseData();

	xft = collPoint.x - this->XPos();
	yft = collPoint.y - this->YPos();
	zft = collPoint.z - this->ZPos();
	
	// This is the only place CalcTransformMatrix() happens (other than Init()) for 
	// ground vehicles, I think. It's quite possible we can do away with it entirely,
	// as Ed isn't using it anywhere any how.
	// CalcTransformMatrix();
	
	mlSinCos (&psi, theObject->Yaw());
	rx_t = -(psi.cos*xft + psi.sin*yft);

	range = (float)(xft*xft + yft*yft + zft*zft);

	targetPtr->localData->range	= (float)sqrt(range);
	targetPtr->localData->ataFrom	= (float)atan2(sqrt(range-rx_t*rx_t),rx_t);
	targetPtr->localData->az		= (float)atan2(yft,xft);
	targetPtr->localData->el		= (float)atan2(-zft,sqrt(range-zft*zft));
	}
	else 	// KCK: Do our ground version of CalcRelGeometry to get needed info
	CalcRelAzElRangeAta (this, targetPtr);
	// edg: get real range.  Since we don't update targets every frame
	// anymore, we should get a current range for more accuracy
	// KCK: all but zft are calculated in the CalRelAzElRangeAta function, so this doesn't
	// need to be done anymore
//	xft = target->XPos() - XPos();
//	yft = target->YPos() - YPos();
	zft = target->ZPos() - ZPos();
//	realRange = sqrt( xft * xft + yft * yft + zft * zft );

	wc = theMissile->GetWCD(); // MLR
	//wc = &WeaponDataTable[Sms->hardPoint[Sms->CurHardpoint()]->weaponId];
	
	// 2002-03-08 ADDED BY S.G. Will use the new missile auxiliary data file for MinEngagementAlt and MinEngagementRange
	SimWeaponDataType* wpnDefinition = &SimWeaponDataTable[Falcon4ClassTable[wc->Index].vehicleDataIndex];
	MissileAuxData *auxData = NULL;
	if (wpnDefinition->dataIdx < numMissileDatasets)
		auxData = missileDataset[wpnDefinition->dataIdx].auxData;
	// END OF ADDED SECTION 2002-03-08

	// edg: wc->maxAlt is a value in the range of 0 - 100
	// I'm going to assume:
	//		1) 100 is equivalent to 100000 ft
	//		2) 0 has no air capability
	//		3) not 0 has no surf-surf capability
	maxAlt = -1000.0f * wc->MaxAlt;
// 2001-02-18 MODIFIED BY S.G. SO minAlt IS NO LONGER BASED ON maxAlt
//  minAlt = max(-1500.0F, maxAlt * 0.1f);
//	minAlt = (float)(wc->Name[18]) * -32.0F;
// 2002-02-26 MN the radar data now decides on that
// 2002-03-09 MN mea culpa - minAlt above put back in - this makes is weapon dependant !!
	//	minAlt = (float)-(radarData->MinEngagementAlt);
// 2002-03-09 MODIFIED BY S.G. Uses the new missile auxiliary data file instead of the radar file so it's more granular
	if (auxData)
		minAlt = (float)-auxData->MinEngagementAlt;
	else
		minAlt = 1.0f; // Because I do a '-' in front of the previous statement, I need to make it 1.0 instead of -1.0

	// If we haven't entered the MinEngagementAlt yet, use the one in the Falcon4.WCD file
	if (minAlt > 0.0f)
		minAlt = (float)(wc->Name[18]) * -32.0F;

	if (!target->OnGround() )
	{
		if ( maxAlt == 0.0f )
			return FALSE;

		// edg: I'm leaving these in for now, however this should all be
		// moved into weapon selection.  I've commented them out in guns.
		if (zft < maxAlt || zft > minAlt)
			return FALSE;
//		if (targetPtr->localData->range > wc->Range*KM_TO_FT /*|| targetPtr->localData->range < wc->Range*KM_TO_FT*0.1F */)
//			return FALSE;
		if (targetPtr->localData->range > theMissile->GetRMax(-target->ZPos(), 0, 
			targetPtr->localData->az, targetPtr->BaseData()->GetVt(), targetPtr->localData->ataFrom))
			return FALSE;

		if (auxData)
		{
			if(targetPtr->localData->range < auxData->MinEngagementRange) // 2002-03-09 MODIFIED BY S.G. Uses the MISSILES data file, more granular than the radar data file
			return FALSE;
		}

// SCR 11/20/98  Lets let the seeker and kinematics deal with this...
/*
		// Check for aspect (KCK WARNING: This assumes ataFrom is -PI to PI, not 0 to 2*PI
		if (wc->Flags & WEAP_FRONT_ASPECT && fabs(targetPtr->localData->ataFrom) > 90*DTR)
			return FALSE;
		if (wc->Flags & WEAP_REAR_ASPECT && fabs(targetPtr->localData->ataFrom) < 90*DTR)
			return FALSE;
*/
	}
	// target on ground
	else if (maxAlt != 0.0f || targetPtr->localData->range > wc->Range*KM_TO_FT)
		return FALSE;

	// az and el are relative from our vehicles orientation so subtract
	// out yaw and pitch
	az = targetPtr->localData->az - Yaw();
	el = targetPtr->localData->el - Pitch();

	// Bump up elevation to account for gravity, etc.
/*	static const float BUMP_AMOUNT = 30.0f*DTR;
	if (el < 45.0f*DTR - BUMP_AMOUNT) {
		el = 45.0f*DTR;
	}
	else if (el < 60.0f*DTR)
	{
		el += BUMP_AMOUNT;
	}*/

	//static const float BUMP_AMOUNT = 15.0f*DTR;
	//if (el < 45.0f*DTR) {
	float BUMP;
	if (	GetCampaignObject()		&&  // MLR 5/27/2004 - CTD ?
		 /*!((BattalionClass*)GetCampaignObject())->GetMissilesFlying())*/
		 //Cobra TJL 10/30/04
		 !GetCampaignObject()->GetMissilesFlying()){

		BUMP = static_cast<float>(radarData->Elevationbumpamounta);
	}
	else {
		BUMP = static_cast<float>(radarData->Elevationbumpamountb);
	}
	if (BUMP > 90) BUMP = (BUMP -90) * -1.0f;
	BUMP *= DTR;
		el = max(5.0F*DTR, el + targetPtr->localData->range/(wc->Range*KM_TO_FT)*BUMP);
	//}

	// if we're within an acceptable tolerance, just snap to target and return TRUE,
	// otherwise, update our target data and keep rotating our DOFs
	/*
	** edg: I dunno about this.  It's fine in theory, however I think its
	** defeating the fire rate by also making them wait to get their turrets
	** around (and therefore surf-air not firing enough).  Just snap.
	** Also, I think there's a bug here in that at doesn't account for 360
	** deg wrap.
	*/
#if 0
	if ( fabs(DOFData[0] - az) > 45*DTR )
		return FALSE;
	if ( fabs(DOFData[1] - el) > 45*DTR )
		return FALSE;
#else
	SetDOF(AIRDEF_ELEV, el);
	SetDOF(AIRDEF_AZIMUTH, az);
#endif

	// Set the missile's target
	theMissile->SetTarget( targetPtr );

	return TRUE;
}


/*
** Name: FindBattalionFireControl
** Description:
**		Find vehicle in unit who can emit (preferentially ourselves), and
**		set them as our fire controller
*/
void
GroundClass::FindBattalionFireControl( void )
{
	GroundClass		*firectl;
	CampBaseClass	*batt;
	int				battRadarType;

	// do we already have one?
	if ( battalionFireControl )
	{
		if ( !battalionFireControl->IsDead() )
		{
			// Can still use the one we've got
			return;
		} else {
			VuDeReferenceEntity( battalionFireControl );
			battalionFireControl = NULL;
		}
	}

	// are we an emitter?
	if ( isEmitter )
	{
		battalionFireControl = this;
		VuReferenceEntity( battalionFireControl );
		return;
	}


	// Get the type of radar the battalion thinks it has
	batt = GetCampaignObject();
	ShiAssert( batt );					// We'de better belong to a battalion
	ShiAssert( batt->GetComponents() );	// If there aren't components, then WHAT ARE WE???
	if (!batt || batt->GetComponents() == NULL)  // Should never have been changed...
		return;
	
	battRadarType = batt->GetRadarType();

	// If the battalion is blind, so are we...
	if (!battRadarType) {
		ShiAssert( battalionFireControl == NULL );	// Should never have been changed...
		return;
	}


	// Get the list of sim entities in our battalion
	VuListIterator	battalionIterator(batt->GetComponents());
	firectl = (GroundClass*) battalionIterator.GetFirst();
	// Search the list for a battalion radar vehicle
	while ( firectl )
	{
		if ( firectl->GetRadarType() == battRadarType && !firectl->IsDead() )
		{
			battalionFireControl = firectl;
			VuReferenceEntity( battalionFireControl );
			break;
		}
		firectl = (GroundClass*) battalionIterator.GetNext();
	}
}
