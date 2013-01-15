#include "stdhdr.h"
#include "aircrft.h"
#include "missile.h"
#include "misslist.h"
#include "bomb.h"
#include "bombfunc.h"
#include "PilotInputs.h"
#include "fsound.h"
#include "soundfx.h"
#include "simsound.h"
#include "falcmesg.h"
#include "sms.h"
#include "smsdraw.h"
#include "fcc.h"
#include "guns.h"
#include "MsgInc/WeaponFireMsg.h"
#include "campbase.h"
#include "Simdrive.h"
#include "Graphics/Include/drawsgmt.h"
#include "otwdrive.h"
#include "airframe.h"
#include "falcsess.h"
#include "hud.h"
#include "cpvbounds.h"
#include "Graphics/Include/renderow.h"
#include "ffeedbk.h"
#include "sms.h"
#include "flightdata.h"	//MI
#include "IvibeData.h"
#include "falclib/include/fakerand.h"//Cobra

/* S.G. SO I CAN ACCESS DIGI CLASS VARIABLE USED IN MDEFEAT */#include "digi.h"
/* S.G. SO I CAN HAVE AI AWARE OF UNCAGE IR MISSILE LAUNCH  */#include "sensclas.h"
/* S.G. SO I CAN HAVE AI AWARE OF UNCAGE IR MISSILE LAUNCH  */#include "object.h"
extern int tgtId;
static float vulcDist = 20000.0f;
extern ViewportBounds hudViewportBounds;

extern int g_nMissileFix;

// Start Gilman HACK
int gNumWeaponsInAir = 0;
static int gMaxIAWeaponsFired = 12;
// End Gilman weapon count hack

//MI CAT mod
//extern bool g_bEnableCATIIIExtension;	//MI replaced with g_bRealisticAvionics

void AircraftClass::DoWeapons()
{
	int fireFlag, wasPostDrop;
	SimWeaponClass* curWeapon = Sms->GetCurrentWeapon();
	WayPointClass* tmpWp;
	VuEntity* entity = NULL;
	VU_ID tgtId = FalconNullId,tmpId = FalconNullId;

	// Guns
	//MI
	if(!g_bRealisticAvionics || isDigital)
	{
		fireFlag = fireGun && !OnGround() && (Sms->MasterArm() == SMSBaseClass::Arm);
	}
	else 
	{
		//Gun can only be fired if it's selected as actual weapon
		fireFlag = !OnGround() && (Sms->MasterArm() == SMSBaseClass::Arm) && (GunFire || fireGun);
	}
	
	if (Guns)
	{
		Guns->Exec(&fireFlag, dmx, &platformAngles, targetList, !isDigital );
		
		if (fireFlag)
		{
			if ( isDigital )
				//TJL 11/08/03 Say good bye to the annoying bump sound ;)
				SoundPos.Sfx(af->auxaeroData->sndGunLoop);
			
			else if ( IsFiring() )
			{
				if (!SoundPos.IsPlaying(af->auxaeroData->sndGunStart) ) // MLR 2003-11-19
				{
					// MonoPrint("Vulcan Loop Sound: Playing\n" );
					SoundPos.Sfx(af->auxaeroData->sndGunLoop);
				}
				else
				{
					// this is a hack to keep the start sound contiguous with
					// the loop sound (otherwise there's a descernable pause).
					// while the start sound is playing, gradually decrease the
					// distance so it gets louder
					// basically the start sound is .5 sec.
					// the loop sound max dist is 20000 ft.
					// so, we get the following line....
					vulcDist -= 60000.0f * SimLibMajorFrameTime;
					vulcDist = max( 0.0f, vulcDist );
					SoundPos.Sfx(af->auxaeroData->sndGunLoop);
				}
			}
			
			if (!IsFiring())
			{
				if ( !isDigital )
				{
					SoundPos.Sfx(af->auxaeroData->sndGunStart);
					// F4PlaySound( SFX_DEF[ SFX_VULCAN_START ].handle );
					// MonoPrint("Vulcan Start Sound: Playing\n" );
				}
				// Here we can add effect of Gun firing for A-10, VP_changes
				SendFireMessage (Guns, FalconWeaponsFire::GUN, TRUE, targetPtr);
				if (this == SimDriver.GetPlayerEntity())
					JoystickPlayEffect (JoyFireEffect, 0);
			}
			SetFiring(TRUE);
		}
		else // fireflag is false
		{
			// spin down ownship vulcan?
			if ( IsFiring())
			{
				if ( !isDigital )
				{
					vulcDist = 20000.0f;
					SoundPos.Sfx(af->auxaeroData->sndGunEnd);
				}
				
				// send stop firing message
				SendFireMessage (Guns, FalconWeaponsFire::GUN, FALSE, targetPtr);
				if (this == SimDriver.GetPlayerEntity())
					JoystickStopEffect (JoyFireEffect);
			}
			SetFiring(FALSE);
		}
	}

	// COBRA - RED - Fixing the Bombing differences btw Human and AI
	bool isPlayer = (this == SimDriver.GetPlayerEntity());
	if ((this->autopilotType == CombatAP)) // FRB
		isPlayer = false;
	
	if (!SimDriver.RunningInstantAction() || gNumWeaponsInAir < gMaxIAWeaponsFired)
	{
		wasPostDrop = FCC->postDrop;
		
		// If in Selective jettison mode, stop here, since the pickle button has been usurped
		if (
			Sms->drawable && Sms->drawable->DisplayMode() == SmsDrawable::SelJet &&
			Sms->drawable->IsDisplayed()
		){
			if (FCC->releaseConsent && !OnGround())
				Sms->SelectiveJettison();
		}
		// Firing A-A Missiles
		else if (FCC->GetMasterMode() == FireControlComputer::Missile ||
			FCC->GetMasterMode() == FireControlComputer::Dogfight ||
			FCC->GetMasterMode() == FireControlComputer::MissileOverride)
		{
			if (FCC->releaseConsent && !FCC->postDrop)
			{
				// 2002-04-07 MN CTD fix - if in AA mode and gun is selected, pressing trigger will crash
				if (Sms->curWeapon && Sms->curWeaponClass != wcGunWpn){
					if (Sms->LaunchMissile()){
						if (this == FalconLocalSession->GetPlayerEntity())
						{
							g_intellivibeData.AAMissileFired++;
						}
						
						// ADDED BY S.G. SO DIGI KNOWS ABOUT THE MISSILE THEY HAVE LAUNCHED
						// 2000-10-04 UPDATED BY S.G. Forgot to 
						// dereference our previous missile before setting another one
						
						// Only if we are a digital entity
						if (isDigital || !isPlayer) // FRB
						{
							// If we currenly have a missile in our variable, dereference it first
							if (DBrain()->missileFiredEntity)
								VuDeReferenceEntity((VuEntity *)(DBrain()->missileFiredEntity));
							DBrain()->missileFiredEntity = curWeapon;
							// Let F4 know we are using this object...
							VuReferenceEntity((VuEntity *)(DBrain()->missileFiredEntity));
							// Clear the lsb so we know we have just set it 
							// (we'll be off by a milisecond, so what!)
							DBrain()->missileFiredTime = SimLibElapsedTime & 0xfffffffe;
						}
						// END OF ADDED SECTION
						// JPO - two cases- dogfight mode has its own sub mode.
						// all others use the regular.
						if ((FCC->GetMasterMode() != FireControlComputer::Dogfight &&
							FCC->GetSubMode() == FireControlComputer::Aim9) ||
							(FCC->GetMasterMode() == FireControlComputer::Dogfight &&
							FCC->GetDgftSubMode() == FireControlComputer::Aim9)  ||
							(FCC->GetMasterMode() == FireControlComputer::MissileOverride && 
							// ASSOCIATOR: Added MissileOverride here to get remembered mode
							FCC->GetMrmSubMode() == FireControlComputer::Aim9) )
						{
							// 2000-09-08 MODIFIED BY S.G. SO AI ARE MADE AWARE OF UNCAGED 
							// IR MISSILE LAUNCHED IF THEY HAVE A TALLY ON THE LAUNCHING AIRCRAFT
							// 2000-10-02 UPDATED BY S.G. WILL DO A SendFireMessage AND 
							// LET THE MISSILE SetIncomingMissile ROUTINE DECIDE IF IT CAN SEE IT OR NOT...
							//SendFireMessage (curWeapon, FalconWeaponsFire::SRM, TRUE, targetPtr);
							SimObjectType* tmpTargetPtr = targetPtr;
							// Must be an IR missile
							if (
								curWeapon->IsMissile() && 
								((MissileClass *)curWeapon)->GetSeekerType() == SensorClass::IRST
							){
								// If we do not have a target, get the missile's target
								if (!targetPtr){
									tmpTargetPtr = curWeapon->targetPtr;
								}
							}
							
							SendFireMessage (curWeapon, FalconWeaponsFire::SRM, TRUE, tmpTargetPtr);
						}
						
						// END OF ADDED SECTION
						
						else if (
							FCC->GetSubMode() == FireControlComputer::Aim120 ||
							(FCC->GetMasterMode() == FireControlComputer::Dogfight &&	
							FCC->GetDgftSubMode() == FireControlComputer::Aim120) || 
							(FCC->GetMasterMode() == FireControlComputer::MissileOverride && 
							// ASSOCIATOR: Added MissileOverride here to get remembered mode
							FCC->GetMrmSubMode() == FireControlComputer::Aim120) 
						){
							//me123 addet next line
							SendFireMessage (curWeapon, FalconWeaponsFire::MRM, TRUE, targetPtr);
						}
						else {
							SendFireMessage (curWeapon, FalconWeaponsFire::MRM, TRUE, targetPtr);
						}
					
						FCC->MissileLaunch(); //me123 used for "TOF" que modifed JPO

						fireMissile = FALSE;
						// MLR Note: Maybe we should have the missle emit the sound (new & looped!)

						//RV - I-Hawk - commenting this call. all missiles launches sound
						//now handled in SMSClass::LaunchMissile()
						//SoundPos.Sfx(SFX_MISSILE1); 
					}
					
					FCC->postDrop = TRUE;
					//Cobra bomb shake?
					if (this == SimDriver.GetPlayerEntity() && Sms->MasterArm() == SMSBaseClass::Arm){
   						ioPerturb = 0.5f;
						}
				}
			}
		} 
		// Firing A-G Missiles
		else if (FCC->GetMasterMode() == FireControlComputer::AirGroundMissile ||
			FCC->GetMasterMode() == FireControlComputer::AirGroundHARM)
		{
			if (FCC->releaseConsent && !FCC->postDrop)
			{
				if (Sms->LaunchMissile())
				{
					if (this == FalconLocalSession->GetPlayerEntity())
						g_intellivibeData.AGMissileFired++;
					
					if (FCC->GetMasterMode() == FireControlComputer::AirGroundMissile)
						SendFireMessage (curWeapon, FalconWeaponsFire::AGM, TRUE, targetPtr);
					else
						SendFireMessage (curWeapon, FalconWeaponsFire::ARM, TRUE, targetPtr);
					
					fireMissile = FALSE;
					
					// 2002-04-14 MN moved into MislSms::LaunchMissile - if WEAP_BOMBDROPSOUND is set, play it, if not, missile launch sound
					// do that only if we don't want the above...
					if (!(g_nMissileFix & 0x80))
						SoundPos.Sfx(SFX_MISSILE2);
				}
				
				FCC->postDrop = TRUE;
			//Cobra bomb shake?
				if (this == SimDriver.GetPlayerEntity() && Sms->MasterArm() == SMSBaseClass::Arm)
					{
   					ioPerturb = 0.5f;
					}
			}
		} 
		// Firing rockets
		else 
			//if (FCC->GetMasterMode() == FireControlComputer::AirGroundBomb && // MLR 4/3/2004 - 
			//FCC->GetSubMode() == FireControlComputer::RCKT)
			if (FCC->GetMasterMode() == FireControlComputer::AirGroundRocket
						&& FCC->GetSubMode() == FireControlComputer::OBSOLETERCKT) // MLR 4/3/2004 - 
			{
				if (FCC->bombPickle)
				{
					// Play the sound
					/* New sound for rockets, no longer need this.
					SoundPos.Sfx(SFX_RCKTLOOP);
					*/
					
					if (!(Sms->IsSet(SMSBaseClass::Firing)) && curWeapon) // MLR 3/8/2004 - CTD
					{
						// Drop a message
						SendFireMessage (curWeapon, FalconWeaponsFire::Rocket, TRUE, targetPtr);
					}
					
					if (Sms->LaunchRocket())
					{
						if (this == FalconLocalSession->GetPlayerEntity())
							g_intellivibeData.AGMissileFired++;
						
						// Stop firing
						FCC->bombPickle = FALSE;
						FCC->postDrop = TRUE;
						
						// Play the sound
						/*
						SoundPos.Sfx(SFX_MISSILE3);
						*/
					}
				}
			}
			// Droping dumb bombs
			else if (FCC->GetMasterMode() == FireControlComputer::AirGroundBomb)
			{
				// COBRA - RED - Rewritten in a decent and WORKING WAY..!!!!

				// Ok, look for a Bomb
				BombClass	*TheBomb=FCC->GetTheBomb();
				// And if the Bomb is present and released
				if(TheBomb && FCC->bombPickle)
				{
					float wt = 0.0f;
					if (TheBomb->GetWeaponId())
						wt = (float)(WeaponDataTable[TheBomb->GetWeaponId()].Weight);
						//float wt = (Sms->hardPoint[Sms->CurHardpoint()]) ? (WeaponDataTable[Sms->hardPoint[Sms->CurHardpoint()]->weaponId].Weight) : 0.0f;
					// RED - Turbulence based on Weight...
					float		wtTurb=sqrtf((float)wt)/100.0f;

					// *** JDAM STUFF *** Check if it's powered
					if ( TheBomb->IsSetBombFlag(BombClass::IsGPS) && Sms->JDAMPowered){
						// Drop it, enabling ripple if Player
						if (Sms->DropBomb(isPlayer)){
							FCC->bombPickle = FALSE;
							FCC->postDrop = TRUE;
							ioPerturb = wtTurb;
						}
						// End here
						return;
					}
					
					// *** JSOW STUFF *** Check if it's powered
					if ( TheBomb->IsSetBombFlag(BombClass::IsJSOW && Sms->JDAMPowered)){
						// if AI Pair enabled based on weight							
						if(!isPlayer){
							if (wt < 1999) Sms->SetAGBPair(TRUE);
							else Sms->SetAGBPair(FALSE);
						}
						// Drop it, enabling ripple if Player
						if (Sms->DropBomb(isPlayer)){
							Sms->JDAMPowered = TRUE;
							FCC->bombPickle = FALSE;
							FCC->postDrop = TRUE;
   							ioPerturb = wtTurb;
						}
						// End here
						return;
					}
					
					// * From here, code for bumb bombs *
					// *** CLUSTERS STUFF *** it has burnt height
					if (Sms->hardPoint[Sms->CurHardpoint()]->GetWeaponData()->flags & SMSClass::HasBurstHeight){
						// If not Player drop in Couples
						if (!isPlayer) Sms->SetAGBPair(TRUE);
						// Drop it, enabling ripple if Player
       			if (Sms->DropBomb(isPlayer)){
							FCC->bombPickle = FALSE;
							FCC->postDrop = TRUE;
   							ioPerturb = wtTurb;
						}
						// End here
						return;
					}
					
					bool	Ripple=true;
					// *** GENERIC BOMBS ***
					// Checks for AI
					if(!isPlayer){
						// Drop light Bombs in pair else as single
						if (wt < 2000){ Sms->SetAGBPair(FALSE); Ripple=true; }
						else { Sms->SetAGBPair(TRUE); Ripple=false; }
					}
					if (Sms->DropBomb(Ripple)){
						FCC->bombPickle = FALSE;
						FCC->postDrop = TRUE;
	   					ioPerturb = wtTurb;
					}
					// End here
					return;
				}
			}
			// *** LASERS STUFF *** it has burnt height
			else if (FCC->GetMasterMode() == FireControlComputer::AirGroundLaser)
			{	
				// COBRA - RED - Rewritten in a decent and WORKING WAY..!!!!

				// Ok, look for a Bomb
				BombClass	*TheBomb=FCC->GetTheBomb();

				// And if the Bomb is present and released
				if(TheBomb && FCC->bombPickle)
				{
					float wt = 0.0f;
					if (TheBomb->GetWeaponId())
						wt = (float)(WeaponDataTable[TheBomb->GetWeaponId()].Weight);
						//float wt = (Sms->hardPoint[Sms->CurHardpoint()]) ? WeaponDataTable[Sms->hardPoint[Sms->CurHardpoint()]->weaponId].Weight : 0.0f;
					// RED - Turbulence based on Weight...
					float	wtTurb=sqrtf((float)wt)/100.0f;		
					// Set the target position 
					TheBomb->SetTarget(targetPtr);

					//MI ripple for GBU's is there in real
					if(!g_bRealisticAvionics){            
						if (Sms->DropBomb(FALSE)) 
						{
							FCC->bombPickle = FALSE;
							FCC->postDrop = TRUE;
							ioPerturb = wtTurb;
						}
					} else {
						if (Sms->DropBomb(isPlayer)){
							FCC->bombPickle = FALSE;
							FCC->postDrop = TRUE;
							ioPerturb = wtTurb;
						}
					}
				}
			}
			// Just taking pictures for the family
			else if (FCC->GetMasterMode() == FireControlComputer::AirGroundCamera)
			{
				if (FCC->releaseConsent && !OnGround() && Sms->curWeapon)
				{
					CampBaseClass		*campEntity;
					float vpLeft, vpTop, vpRight, vpBottom;
					
					// Store the current viewport
					OTWDriver.renderer->GetViewport (&vpLeft, &vpTop, &vpRight, &vpBottom);
					
					// Set the HUD viewport
					OTWDriver.renderer->SetViewport (hudViewportBounds.left, hudViewportBounds.top,
						hudViewportBounds.right, hudViewportBounds.bottom);
					
					// Find the 'official target
					tmpWp = waypoint;
					while (tmpWp && tmpWp->GetWPAction() != WP_RECON)
						tmpWp = tmpWp->GetNextWP();
					if (tmpWp)
						tgtId = tmpWp->GetWPTargetID();
					
					// Check the assigned target, if any
					if (tgtId != FalconNullId)
					{
						campEntity = (CampBaseClass*) vuDatabase->Find(tgtId);
						
						// Check vs the assigned target's components
						entity = NULL;
						if (campEntity && campEntity->GetComponents())
						{
							VuListIterator		cit(campEntity->GetComponents());
							
							//				   if (campEntity->GetComponents())
							//					   {
							entity = cit.GetFirst();
							while (entity && !TheHud->CanSeeTarget(curWeapon->Type(),entity, this))
								entity = cit.GetNext();
							//					   }
						}
						if (entity)
							tgtId = entity->Id();
						else
							tgtId = FalconNullId;
					}
					
					// If we didn't find something at our target site, check for something else we may have seen
					if (!entity)
					{
						// Check features first
						VuListIterator featWalker (SimDriver.combinedFeatureList);
						entity = featWalker.GetFirst();
						while (entity && !TheHud->CanSeeTarget(curWeapon->Type(),entity, this))
						{
							entity = featWalker.GetNext();
						}
						
						// No features, check for vehicles
						if (!entity)
						{
							VuListIterator objWalker (SimDriver.combinedList);
							entity = objWalker.GetFirst();
							while (entity && !TheHud->CanSeeTarget(curWeapon->Type(),entity, this)){
								entity = objWalker.GetNext();
							}
						}
						
						if (entity)
							tgtId = entity->Id();
						else
							tgtId = FalconNullId;
					}
					
					// Need to find target here
					SendFireMessage (curWeapon, FalconWeaponsFire::Recon, TRUE, NULL, tgtId);
					
					// Restore the viewport
					OTWDriver.renderer->SetViewport (vpLeft, vpTop, vpRight, vpBottom);
				}
			}
   }
   else
   {
	   // Special case for firing a load of rockets
	   //      if (FCC->GetMasterMode() == FireControlComputer::AirGroundBomb && // MLR 4/3/2004 - 
	   //          FCC->GetSubMode() == FireControlComputer::RCKT  &&
	   //	             Sms->IsSet(SMSBaseClass::Firing) &&
	   //          FCC->bombPickle)
	   
	   if (FCC->GetMasterMode() == FireControlComputer::AirGroundRocket && // MLR 4/3/2004 - 
		   Sms->IsSet(SMSBaseClass::Firing) &&
		   FCC->bombPickle)
	   {
		   // Play the sound
		   /*
		   SoundPos.Sfx(SFX_RCKTLOOP);
		   */
		   
		   if (!(Sms->IsSet(SMSBaseClass::Firing)) && curWeapon)  // MLR 3/8/2004 - CTD
		   {
			   // Drop a message
			   SendFireMessage (curWeapon, FalconWeaponsFire::Rocket, TRUE, targetPtr);
		   }
		   
		   if (Sms->LaunchRocket())
		   {
			   if (this == FalconLocalSession->GetPlayerEntity())
				   g_intellivibeData.AGMissileFired++;
			   
			   // Stop firing
			   FCC->bombPickle = FALSE;
			   FCC->postDrop = TRUE;
			   
			   // Play the sound
			   /*
			   SoundPos.Sfx(SFX_MISSILE3);
			   */
		   }
	   }
	   else
	   {
		   FCC->bombPickle = FALSE;
	   }
   }
}
