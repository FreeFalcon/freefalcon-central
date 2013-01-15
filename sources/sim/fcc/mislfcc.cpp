#include "stdhdr.h"
#include "fcc.h"
#include "missile.h"
#include "sms.h"
#include "fsound.h"
#include "soundfx.h"
#include "object.h"
#include "camp2sim.h"
#include "simveh.h"
#include "otwdrive.h"
#include "radar.h"
#include "fack.h"
#include "aircrft.h"
//extern bool g_bHardCoreReal; //me123		MI replaced with g_bRealisticAvionics
#include "campbase.h"
#include "classtbl.h"
#include "simdrive.h" // MLR to give access to SimDriver for Aim9 volume


// Angle off sun at which sun effect goes to zero
static const float	COS_SUN_EFFECT_HALF_ANGLE	= (float)cos( 20.0f * DTR );//me123 changed from 10 since the sun is so small in f4

extern bool g_bRealisticAvionics;
extern int g_nRNESpeed;

#include "SimIO.h"	// Retro 3Jan2004

void FireControlComputer::AirAirMode(void)
{
	MissileClass* theMissile;
	float irSig;
	SimObjectType* curTarget;
	RadarClass* theRadar = (RadarClass*)FindSensor(platform, SensorClass::Radar);
	static float lastIrSig = 0.0F;
	static const float MISSILE_ALTITUDE_BONUS = 24.0f;//me123 addet and in missmain.cpp
	static bool bElReversed = false; // Marco - is diamond for aim9 uncaged reversed movement in HUD?
	static bool bAzReversed = false;
	// Marco - set if there is a target under the diamond but missile slave/caged and radar not locked up
	bool bCageSound = false ; 
	float irSigTDBP = 0.0F ;	// Marco - storage for irSig
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

	if (!(Sms->curWeaponType == wtAim120 || Sms->curWeaponType ==wtAim9)) return;//me123

	SimWeaponClass *cw = Sms->GetCurrentWeapon();
	if (cw){
		theMissile = static_cast<MissileClass *>(cw);
		if (theMissile->launchState == MissileClass::PreLaunch){
			theMissile->SetPosition(platform->XPos(), platform->YPos(), platform->ZPos());
			
			// Draw the DLZ
			if (targetPtr){
				if (theRadar){
					if (targetPtr->localData->range < 4.5F * NM_TO_FT)
						missileWEZDisplayRange = 5.0F * NM_TO_FT;
					else if (targetPtr->localData->range > 4.75F * NM_TO_FT) 
						missileWEZDisplayRange = theRadar->GetRange() * NM_TO_FT;
				}
				else
					missileWEZDisplayRange = 20.0F * NM_TO_FT;
			}
			else {
				if (theRadar){
					missileWEZDisplayRange = theRadar->GetRange() * NM_TO_FT;
				}
				else {
					missileWEZDisplayRange = 20.0F * NM_TO_FT;
				}
			}
			
			// Marco Edit - Slave/Bore Mode
			if(missileSlaveCmd){
				theMissile->isSlave = 1 - theMissile->isSlave;
				missileSlaveCmd = FALSE;
			}
			// Marco Edit - AIM9 Spot/Scan
			if (
				Sms->curWeaponType == wtAim9 && missileSpotScanCmd && 
				cw->GetSPType() != SPTYPE_AIM9P
			){
				theMissile->isSpot = 1 - theMissile->isSpot;
				if (!theMissile->isSpot){
					missileSeekerAz = 0.00f;
					missileSeekerEl = -0.06f;
				}
				missileSpotScanCmd = FALSE;
			}

			// Marco Edit - AIM9 Cage/Uncaged
			if (Sms->curWeaponType == wtAim9 && missileCageCmd)
			{
				theMissile->isCaged = 1 - theMissile->isCaged;
				missileCageCmd = FALSE;
			}
			// Marco Edit - AIM9 Auto Uncage
			if (Sms->curWeaponType == wtAim9 && missileTDBPCmd)
			{
				theMissile->isTD = 1 - theMissile->isTD;
				missileTDBPCmd = FALSE;
			}

			//MI 02/02/02 make sure we don't get an "unallowed" mode for the P's
			if (Sms->curWeaponType == wtAim9 && cw->GetSPType() == SPTYPE_AIM9P)
			{
				if(!theMissile->isSpot)
					theMissile->isSpot = TRUE;
				if(!theMissile->isSlave)
					theMissile->isSlave = TRUE;
				if(theMissile->isTD)
					theMissile->isTD = FALSE;
			}
			
			if (
				(subMode == Aim9 || mrmSubMode == Aim9 || masterMode == Dogfight ) && 
				Sms->curWeaponType == wtAim9
			){
				// ASSOCIATOR: Added mrmSubMode
				// If slave failure, force boresight
				if (playerFCC && ((AircraftClass*)platform)->mFaults->GetFault(FaultClass::msl_fault)){
					theMissile->isSlave = FALSE;
				}
				
				// Marco Edit - rewritten the sidewinder header stuff
				// to support spot/scan, cage/uncage, slav/bore
				
				// First off - if uncaged with a target then
				// track it

				// 2001-01-31 MODIFIED BY S.G. SO IN HMS PLANE, 
				// UNCAGED IR MISSILE DOESN'T GO FIND A TARGET BY THEMSELF.
				// THIS IS FOUND BY QUERYING BIT 20000000 
				// OF Flags OF THE VEHICLE. A 1 THERE MEANS THIS VEHICLE IS HMS EQUIPPED AND SHOULD SKIP THIS CODE
				VehicleClassDataType	*vc	= 
					(VehicleClassDataType *)Falcon4ClassTable[platform->Type() - VU_LAST_ENTITY_TYPE].dataPtr;
				if (!playerFCC || ( // don't do this check for ai
					vc && (!(vc->Flags & 0x20000000) || 
					// JB 010712 Normal behavior for the 2d cockpit
					OTWDriver.GetOTWDisplayMode() == OTWDriverClass::Mode2DCockpit) && 
					!theMissile->isCaged)
				){
					// We don't have a target so find one
					if (!theMissile->targetPtr){
						theMissile->SetSeekerPos (&missileSeekerAz, &missileSeekerEl);
						curTarget = targetList;
						while (curTarget)
						{
							if (curTarget->BaseData()->IsSim() &&
								!curTarget->BaseData()->IsWeapon())
							{
								theMissile->SetTarget(curTarget);
								theMissile->RunSeeker();
								if (theMissile->targetPtr)
								{
									break;
								}
							}
							curTarget = curTarget->next;
						}
					}
					else
					// We have a target so track it
					{
						theMissile->SetSeekerPos (&theMissile->targetPtr->localData->az, &theMissile->targetPtr->localData->el);
						theMissile->RunSeeker();
					}
				}
				
				// Missile is Caged so bore/slave becomes important
				if (theMissile->isCaged)
				{
					// First, drop the current target
					if (theMissile->targetPtr)
						theMissile->DropTarget();

					// Deal if we have both a radar tgt and missile is slaved to radar
// M.N. 2001-12-13 fix - we didn't run the seeker, which didn't update the irSignature, which didn't give us a release consent from the FCC
					if (targetPtr && theMissile->isSlave)
					{
						theMissile->SetTarget(targetPtr);
						theMissile->SetSeekerPos (&theMissile->targetPtr->localData->az, &theMissile->targetPtr->localData->el);
						theMissile->RunSeeker();	// We still need to run the seeker even if slaved to radar ! How to get new irSig otherwise ??
/*** old code				irSig = targetPtr->localData->irSignature;
						theMissile->SetTarget(targetPtr);
						targetPtr->localData->irSignature = irSig;
						theMissile->SetSeekerPos (&theMissile->targetPtr->localData->az, &theMissile->targetPtr->localData->el);*/
					}
					else
					{
						// Here either we don't care about the actual target
						// or we are caged and won't 'lock' on to it
						float oldAz = missileSeekerAz;
						float oldEl = missileSeekerEl;
						curTarget = targetList;
						while (curTarget)
						{
							if (curTarget->BaseData()->IsSim() && !curTarget->BaseData()->IsWeapon())
							{
								theMissile->SetTarget(curTarget);
								theMissile->RunSeeker();
								theMissile->SetSeekerPos (&oldAz, &oldEl);// reset the seeker
								if (theMissile->targetPtr)
								{
									// We assume we have an IRST seeker - so check if it's near the diamond
									// if (theMissile->targetPtr->localData->ata < 1.0 * DTR)
									//IrstClass*  Irst = (IrstClass *)theMissile->sensorArray[0];
	
									if (theMissile->isSpot && fabs(theMissile->targetPtr->localData->el - missileSeekerEl) <  0.03f && 
										fabs(theMissile->targetPtr->localData->az - missileSeekerAz) <  0.03f ) 
									{
										// nb. Exec checks LOS and stuff like that
										break;
									}
									if (!theMissile->isSpot && fabs(theMissile->targetPtr->localData->el - missileSeekerEl) <  0.06f && 
										fabs(theMissile->targetPtr->localData->az - missileSeekerAz) <  0.06f ) 
									{
										break;
									}
								}
							}
							curTarget = curTarget->next;
						}
						// Now get the IR Signature for the sound
						if (curTarget)
						{
							bCageSound = true;
							irSig = theMissile->targetPtr->localData->irSignature 
								* ((0.02f - (float)fabs(theMissile->targetPtr->localData->el - missileSeekerEl)) / 0.01f) 
								* ((0.02f - (float)fabs(theMissile->targetPtr->localData->az - missileSeekerAz)) / 0.01f)
								/ 2.0f + 0.75f;
							
//							missileSeekerAz = theMissile->targetPtr->localData->az;
//							missileSeekerEl = theMissile->targetPtr->localData->el;
//							theMissile->SetSeekerPos (&missileSeekerAz, &missileSeekerEl);
						}
						else
						{
							if (theMissile->targetPtr)
								theMissile->DropTarget();
						}
					}

				}



				// Do a check - if seekerhead is not cool then
				// we can't track any target(s)
				//MI 02/02/02 added check for PlayerFCC and or AIM9P
				bool IsCAP = true;
				if (playerAC && playerAC->AutopilotType() != AircraftClass::CombatAP)
					IsCAP = false;
				if(playerFCC && !IsCAP 
						&& (Sms->GetCoolState() != SMSClass::COOL && Sms->GetCoolState() != SMSClass::WARMING)
						&& theMissile->targetPtr && Sms->GetCurrentWeapon() && Sms->curWeaponType == wtAim9 
						&& cw->GetSPType() != SPTYPE_AIM9P)
				{
					theMissile->DropTarget();
					irSig = 0.0;
				}
				
				if (theMissile->targetPtr)
				{
					irSig = theMissile->targetPtr->localData->irSignature;
				}
				else if (!bCageSound)
				{
					irSig = 0.0f;
				}

				if (irSig < 0.0f)
				{
					irSig = 0.0f;
				}
				
				if (irSig > 1.05F) 
				{
					inRange = TRUE;
				}
				else if (irSig < 1.01F || theMissile->targetPtr != targetPtr)
				{
					inRange = FALSE;
				}
				
				//if (playerFCC && (playerAC && playerAC->AutopilotType() != AircraftClass::CombatAP)) 
				IsCAP = true;
				if (playerAC && playerAC->AutopilotType() != AircraftClass::CombatAP)
					IsCAP = false;
				if (playerFCC && !IsCAP) 
				{
					float aim9Vol=0;
					// Marco Edit
					irSigTDBP = irSig;
					if (!theMissile->isTD)
					{
						irSigTDBP = 0.0f;
					}

					if(playerAC) // CTD exiting mission
					{
						if (IO.AnalogIsUsed(AXIS_MSL_VOLUME) == false)						// Retro 3Jan2004
						{
							if(playerAC->MissileVolume == 8)
								aim9Vol=-10000;
							else
								aim9Vol=-(float)playerAC->MissileVolume * 250;
						}
						else																// Retro 3Jan2004
						{
							// Retro 26Jan2004 - the axis is now reversed on default and scales linear to the axis
							//					- the user will have to shape it to logarithmic to use the throw efficiently
							aim9Vol = -(float)(/*15000-*/IO.GetAxisValue(AXIS_MSL_VOLUME))/1.5F;		// Retro 26Jan2004
						}
					}


					if (g_bRealisticAvionics && !irSig)
					{
						mlTrig trig;
						float yaw   = platform->Yaw();
						float pitch = platform->Pitch();
						float tmpX, tmpY, tmpZ;
						float aim9Pitch ;

						mlSinCos (&trig, platform->Roll());

						switch (Sms->GetCoolState())
						{
							case SMSClass::WARM:
							{
								aim9Pitch = 0.5f;
								break;
							}
							case SMSClass::COOLING:
							{
								aim9Pitch = (float)(Sms->aim9cooltime - SimLibMajorFrameTime);
								aim9Pitch /= (3 * CampaignSeconds) / 2 ;
								aim9Pitch = 1.0f - aim9Pitch;
								break;
							}
							case SMSClass::WARMING:
							{
								aim9Pitch = (float)(Sms->aim9warmtime - SimLibMajorFrameTime);
								aim9Pitch /= (60 * CampaignSeconds) / 2 ;
								break;
							}
							default:
							{
								aim9Pitch = 1.0f;
								break;
							}
						}
						//MI 02/02/02 9P cooling works differently, no button on SMS
						if (cw->GetSPType() == SPTYPE_AIM9P){
							aim9Pitch = (float)(Sms->aim9cooltime - SimLibMajorFrameTime) ;
							aim9Pitch /= (3 * CampaignSeconds) / 2 ;
							aim9Pitch = 1.0f - aim9Pitch;
						}
						// Ensure it's between right values
						//ShiAssert(aim9Pitch <= 1.0f);
						//ShiAssert(aim9Pitch >= 0.5f);
						aim9Pitch = min( max(aim9Pitch, 0.5f), 1.0f);

						if (FindGroundIntersection (pitch, yaw, &tmpX, &tmpY, &tmpZ))
						{
							Aim9AtGround = true ;
							if (OTWDriver.DisplayInCockpit ())
							{
								//F4SoundFXSetDist (SFX_AIM9_ENVIRO_GND, 0, aim9Vol, aim9Pitch);
								F4SoundFXSetDist (theMissile->GetSndAim9EnviroGround(), 0, aim9Vol, aim9Pitch);
							}
						}
						else
						{
							Aim9AtGround = false ;
							if (OTWDriver.DisplayInCockpit ())
							{
								//F4SoundFXSetDist (SFX_AIM9_ENVIRO_SKY, 0, aim9Vol, aim9Pitch);
								F4SoundFXSetDist (theMissile->GetSndAim9EnviroSky(), 0, aim9Vol, aim9Pitch);
							}
						}
					}
					else if (irSig && theMissile->isCaged == TRUE)
					{
						irSig -= 0.75F;
						irSig *= 2.0F;
						irSig += 2.00F;
						irSig = max(2.0F, min (irSig, 4.5F));
						
						// IIR filtered
						static const float TC = 1.0f;	// Seconds...
						
						if (SimLibMajorFrameTime < TC)
						{
							float m  = SimLibMajorFrameTime/TC;
							float im = 1.0f - m;
							lastIrSig = irSig*m + lastIrSig*im;
						}
						lastIrSig = min (irSig, 4.5F);//me123 changed from 4.5
						
						if (OTWDriver.DisplayInCockpit ())
						{
//							MonoPrint ("%f\n", lastIrSig);
							// 1000000.0f = max distance
							// lastIrSig between 2.0 and 4.5
							//F4SoundFXSetDist (SFX_GROWL, 0, aim9Vol, lastIrSig / 4.5F);
							F4SoundFXSetDist (theMissile->GetSndAim9Growl(), 0, aim9Vol, lastIrSig / 4.5F);
						}
					}

					else if (irSig)
					{
						irSig = 16;
						static const float TC = 1.0f;	// Seconds...
						
						if (SimLibMajorFrameTime < TC)
						{
							float m  = SimLibMajorFrameTime/TC;
							float im = 1.0f - m;
							lastIrSig = irSig*m + lastIrSig*im;
						}
						lastIrSig = min (irSig, 16.0F);//me123 changed from 4.5
						
						if (OTWDriver.DisplayInCockpit ())
						{
//							MonoPrint ("%f\n", lastIrSig);
							//MI Changed for new sound
							//F4SoundFXSetDist (SFX_GROWL, 0, 0.0f, lastIrSig / 4.5F);
//							F4SoundFXSetDist (SFX_NO_CAGE, 0, aim9Vol, lastIrSig / 16);
							F4SoundFXSetDist (theMissile->GetSndAim9Uncaged(), 0, aim9Vol, lastIrSig / 16);

						}
					}

					else
					{
						irSig = 2.0f;
						
						// IIR filtered
						static const float TC = 1.0f;	// Seconds...
						
						if (SimLibMajorFrameTime < TC)
						{
							float m  = SimLibMajorFrameTime/TC;
							float im = 1.0f - m;
							lastIrSig = irSig*m + lastIrSig*im;
						}
						lastIrSig = min (irSig, 4.5F);//me123 changed from 4.5
						
						if (OTWDriver.DisplayInCockpit ())
						{

//							MonoPrint ("%f\n", lastIrSig);
//							F4SoundFXSetDist (SFX_GROWL, 0, aim9Vol, lastIrSig / 4.5f);
							F4SoundFXSetDist (theMissile->GetSndAim9Growl(), 0, aim9Vol, lastIrSig / 4.5f);

						}

					}
					if(theMissile->targetPtr && ((SimBaseClass*)theMissile->targetPtr->BaseData())->OnGround())
					{
						theMissile->DropTarget() ;
						irSig = 0.0;
					}
				}
				else //me123 digi cage stuff
				{
					irSig = 0.0f;		   
				}
			}
			else	//not DF or Aim9
			{
				if (playerFCC)
				{
					lastIrSig = 0.0F;
				}
				
				inRange = TRUE;
			}
			
			if (targetPtr && theMissile->isCaged && theMissile->isSlave)
			{
				missileTarget   = TRUE;
				missileRMax   = theMissile->GetRMax (-platform->ZPos(), platform->GetVt(),
				targetPtr->localData->az, targetPtr->BaseData()->GetVt(),
				targetPtr->localData->ataFrom);
				//me123
				missileMaxTof = theMissile->GetmaxTof()  ;//me123
				theMissile->SetSeekerPos(&targetPtr->localData->az, &targetPtr->localData->el);
				//if (g_bHardCoreReal && (	MI
				if(g_bRealisticAvionics && (
					theMissile->sensorArray[0]->Type() == SensorClass::Radar
					//|| theMissile->sensorArray[0]->Type() == SensorClass::RadarHoming
					))
				{
					static const float MISSILE_ALTITUDE_BONUS = 23.0f; // JB 010215 changed from 24 to 23
					static const float	MISSILE_SPEED = 1500.0f; // JB 010215 changed from 1300 to 1500
					static const float MISSILE_TEORY_SPEED = 2900.0f;
					float missileTeoryRMax   = theMissile->GetRMax (30000.0f, 400.0f*KNOTS_TO_FTPSEC ,0.0f, 400.0f*KNOTS_TO_FTPSEC, 0.0f);

					float overtake = MISSILE_SPEED +  (-targetPtr->BaseData()->ZPos()/1000.0f * MISSILE_ALTITUDE_BONUS)+ 
									targetPtr->BaseData()->GetVt()* (float)cos(targetPtr->localData->ataFrom);
					overtake = overtake + ((platform->GetVt() * FTPSEC_TO_KNOTS - 150.0f)/2 );//me123 platform speed bonus // JB 010215 changed from 250 to 150

					float missileteoryMaxTof = min (missileMaxTof-8.0f,missileTeoryRMax / MISSILE_TEORY_SPEED);
					missileteoryMaxTof += -5.0F * (float) sin(.07F * missileteoryMaxTof); // JB 010215
					// digi's don't shoot semi's if agregate now, so this is ok
					/*if (((AircraftClass *)theMissile->parent)->isDigital &&
							(theMissile->sensorArray[0]->Type() == SensorClass::Radar ||
							theMissile->sensorArray[0]->Type() == SensorClass::RadarHoming &&
							overtake * missileteoryMaxTof < missileRMax)) 
						missileRMax   = overtake * missileteoryMaxTof;// missileMaxTof;

					else if (!((AircraftClass *)theMissile->parent)->isDigital) */
					 missileRMax   = overtake * missileteoryMaxTof;
				}
				missileActiveRange = theMissile->GetActiveRange (-platform->ZPos(), platform->GetVt(), targetPtr->localData->ataFrom, 0.0F, targetPtr->localData->range);
				missileActiveTime  = theMissile->GetActiveTime  (-platform->ZPos(), platform->GetVt(), targetPtr->localData->ataFrom, 0.0F, targetPtr->localData->range);
				missileTOF         = theMissile->GetTOF(-platform->ZPos(), platform->GetVt(),targetPtr->localData->ataFrom, targetPtr->BaseData()->GetVt(), targetPtr->localData->range);				
				//LRKLUDGE
				missileRMin   = (platform->GetVt()+targetPtr->BaseData()->GetVt() * ((float)cos(targetPtr->localData->ataFrom)))*2.05f + 1400 +
								 (1000 * targetPtr->localData->ata  * RTD / 10.0f)+
								 (2000 * (float)sin(targetPtr->localData->ataFrom)); //me123 status test. changed from  missileRMax;
				
				// JB 020123 More realistic RneMax value HACK
				// The real calculation of RneMax should be for an aircraft to make 6.5G turn to zero aspect 
				// (headed straight away) accelerating at 1G to 300 knots over their current speed.
				if (g_nRNESpeed)
				{
					missileRneMax = missileRMax * 1500 / (1500.0F + platform->GetVt() + targetPtr->BaseData()->GetVt() * (float)cos(targetPtr->localData->az));
					missileRneMax = missileRneMax * (1500.0F + platform->GetVt() + -(targetPtr->BaseData()->GetVt() + g_nRNESpeed * KNOTS_TO_FTPSEC)) / 1500.0F;
					missileRneMin = 0.26F * missileRneMax;
				}
				else
				{
					missileRneMax = 0.75F * missileRMax; 
					missileRneMin = 0.2F * missileRMax;
				}

				if(missileRMin > missileRneMin) 
					missileRneMin = missileRMin;//me123  addet.
			}
			else if (!theMissile->isCaged)
			{
				if (theMissile->targetPtr)
				{
					missileTarget = TRUE;
					missileActiveRange = theMissile->GetActiveRange (-platform->ZPos(), platform->GetVt(),
						theMissile->targetPtr->localData->ataFrom, 0.0F, theMissile->targetPtr->localData->range);
					missileActiveTime  = theMissile->GetActiveTime  (-platform->ZPos(), platform->GetVt(),
						theMissile->targetPtr->localData->ataFrom, 0.0F, theMissile->targetPtr->localData->range);
					missileTOF         = theMissile->GetTOF(-platform->ZPos(), platform->GetVt(),
						theMissile->targetPtr->localData->ataFrom, theMissile->targetPtr->BaseData()->GetVt(), theMissile->targetPtr->localData->range);
					theMissile->RunSeeker();
				}
				// Marco Edit - only want IR missiles to 'search'
				else if (Sms->curWeaponType == wtAim9 && theMissile->sensorArray)
				{
					missileTarget = FALSE;
					missileTOF      = 0.0F;
					float Az = theMissile->sensorArray[0]->SeekerAz() + ((0.1F  - 0.31f*(float)rand()/(float)RAND_MAX) * DTR);//me123 dangle 0.0F;
					Az = theMissile->sensorArray[0]->SeekerAz()- ((0.1F - 0.33f*(float)rand()/(float)RAND_MAX) * DTR);//me123 dangle 0.0F;
					float El = theMissile->sensorArray[0]->SeekerEl()+ ((0.1F-6.0F * DTR - 0.24F*(float)rand()/(float)RAND_MAX) * DTR);//me123 dangle-6.0F * DTR;
					El = theMissile->sensorArray[0]->SeekerEl()- ((0.1F-6.0F * DTR - 0.28F*(float)rand()/(float)RAND_MAX) * DTR);//me123 dangle-6.0F * DTR;
					theMissile->SetSeekerPos (&Az,&El);						

				}
				else
				{
					missileTarget = FALSE;
					missileTOF    = 0.0f;
					float a = 0;
					theMissile->SetSeekerPos (&a, &a);
				}
				

			}
			// Nutating Seekerhead
			else if (g_bRealisticAvionics && theMissile->isCaged && !theMissile->isSpot)
			{
				if (missileSeekerAz > 0.01)
					bAzReversed = true;
				if (missileSeekerAz < -0.01)
					bAzReversed = false;
				if (missileSeekerEl > -0.05)
					bElReversed = true;
				if (missileSeekerEl < -0.07)
					bElReversed = false;

				if (bAzReversed == false)
					missileSeekerAz += 0.2f * SimLibMajorFrameTime;
				else
					missileSeekerAz -= 0.2f * SimLibMajorFrameTime;
				if (bElReversed == false)
					missileSeekerEl += 0.2f * SimLibMajorFrameTime;
				else
					missileSeekerEl -= 0.2f * SimLibMajorFrameTime;

				missileTarget = FALSE;
				missileTOF = 0.0F;
				theMissile->SetSeekerPos (&missileSeekerAz, &missileSeekerEl);
				theMissile->DropTarget();
			}
			// End Marco Edit
			else
			{
				missileTarget = FALSE;
				missileSeekerAz = 0.0F;
				missileSeekerEl = -6.0F * DTR;
				missileTOF      = 0.0F;
				theMissile->SetSeekerPos (&missileSeekerAz, &missileSeekerEl);
			}
		}
		else	//not prelaunch
		{
			missileTarget   = FALSE;
			if (theMissile->isSlave)
			{
				if (targetPtr)
				{
					missileSeekerAz = targetPtr->localData->az;
					missileSeekerEl = targetPtr->localData->el;
				}
				else
				{
					missileSeekerAz = 0.0F;
					missileSeekerEl = -6.0F * DTR;
				}
			}
			//LRKLUDGE
			missileRMax   = 10000.0F;
			missileActiveRange = 0.0F;
			missileActiveTime = -1.0F;
			missileRMin   = 0.075F * missileRMax;
			missileRneMax = 0.8F * missileRMax;
			missileRneMin = 0.2F * missileRMax;
			missileTOF = 0.0F;
			missileMaxTof = -1.0f;//me123
		}

		if (theMissile && theMissile->sensorArray && theMissile->sensorArray[0])
		{// make sure the display is showing where we are actualy looking
				missileSeekerAz = theMissile->sensorArray[0]->SeekerAz();
				missileSeekerEl = theMissile->sensorArray[0]->SeekerEl();
		}
	}
	else
	{
		if (!releaseConsent)
		{
			// Check for regeneration of weapon
			if (postDrop && Sms->curWeapon == NULL)
			{
	            Sms->ResetCurrentWeapon();
				Sms->WeaponStep();
				ClearCurrentTarget();
			}
			postDrop = FALSE;
		}
	
		missileTOF = 0.0F;
		missileActiveRange = 0.0F;
		missileActiveTime = -1.0F;
		missileTarget   = FALSE;
		missileSeekerAz = 0.0F;
		missileSeekerEl = -6.0F * DTR;
		//LRKLUDGE
		missileRMax   = 10000.0F;
		missileRMin   = 0.075F * missileRMax;
		missileRneMax = 0.8F * missileRMax;
		missileRneMin = 0.2F * missileRMax;
	}

	if (missileTOF > 0.0F) 
	{
		nextMissileImpactTime = missileTOF;
		if (theMissile&&targetPtr)
		lastmissileActiveTime = theMissile->GetActiveTime  (-platform->ZPos(), platform->GetVt(),
						targetPtr->localData->ataFrom, 0.0F, lastMissileShootRng);
		else lastmissileActiveTime = -1.0f;

		if (targetPtr) 
			targetspeed = targetPtr->BaseData()->GetVt();
		else 
			targetspeed = 0.0f;
		Height = -platform->ZPos();
	}
	else
		nextMissileImpactTime = -1.0F;
	
	if (!releaseConsent)
	{
		postDrop = FALSE;
	}
	if (playerFCC && irSigTDBP > 0.79f)/* && theMissile && theMissile->isTD)*/
	{
		theMissile->isCaged = false;
	}
}