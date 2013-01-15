#include "stdhdr.h"
#include "sms.h"
#include "object.h"
#include "missile.h"
#include "misldisp.h"
#include "misslist.h"
#include "fcc.h"
#include "hardpnt.h"
#include "simveh.h"
#include "otwdrive.h"
#include "classtbl.h"
#include "Simdrive.h"
#include "BeamRider.h"
#include "fcc.h"
#include "falcsess.h"
#include "Graphics\Include\drawbsp.h"
#include "Vehicle.h"
#include "aircrft.h"
#include "fack.h"
#include "falcmesg.h"
#include "MsgInc\TrackMsg.h"
#include "Fsound.h" // MN
#include "SoundFX.h" // MN

extern short NumRocketTypes;	// M.N.
extern int g_nMissileFix; // MN

void CreateDrawable (SimBaseClass* theObject, float objectScale);

int SMSClass::LaunchMissile (void)
{
	MissileClass* theMissile;
	int retval = FALSE;
	SimBaseClass* lastWeapon;
	FireControlComputer *FCC = ownship->GetFCC();
	SimObjectType* tmpTargetPtr;
	int isCaged, wpnStation, wpnNum, isSpot, isSlave, isTD;
	float x, y, z, az, el;
	VehicleClassDataType* vc;
	int visFlag;
	
	if (!FCC)
	{
		ClearFlag (Firing);
		return retval;
	}
	
	// Check for SMS Failure or other reason not to launch
	if (!CurStationOK() ||
       !curWeapon ||
       Ownship()->OnGround() ||
       MasterArm() != Arm)
	{
		return retval;
	}
	
	
	tmpTargetPtr = FCC->TargetPtr();
	if (curWeapon)
	{
		F4Assert(curWeapon->IsMissile());
		SetFlag (Firing);
		theMissile = (MissileClass *)curWeapon.get();

		//MI only fire Mav's if they are powered
		//M.N. these conditionals were also true for AA-2's for example, added Type and SType check
		//always use all three checks, as the same values of stype and sptype are not unique!
		//M.N. don't need to uncage or power when in combat AP mode
		if(g_bRealisticAvionics) 
		{
			if((curWeapon->parent &&
			((AircraftClass *)curWeapon->parent.get())->IsPlayer() &&
			!(((AircraftClass *)curWeapon->parent.get())->AutopilotType() == AircraftClass::CombatAP) &&
			!Powered) && curWeapon->GetType() == TYPE_MISSILE &&
			curWeapon->GetSType() == STYPE_MISSILE_AIR_GROUND &&
			(curWeapon->GetSPType() == SPTYPE_AGM65A ||
			curWeapon->GetSPType() == SPTYPE_AGM65B ||
			curWeapon->GetSPType() == SPTYPE_AGM65D ||
			curWeapon->GetSPType() == SPTYPE_AGM65G))
			return FALSE;
		}

		// RV - I-Hawk - Fire HARMs only if powered-up
		if(g_bRealisticAvionics) 
		{
			if((curWeapon->parent &&
			((AircraftClass *)curWeapon->parent.get())->IsPlayer() &&
			!(((AircraftClass *)curWeapon->parent.get())->AutopilotType() == AircraftClass::CombatAP) &&
			!GetHARMPowerState()) && curWeapon->GetType() == TYPE_MISSILE &&
			curWeapon->GetSType() == STYPE_MISSILE_AIR_GROUND &&
			(curWeapon->GetSPType() == SPTYPE_AGM88) )
         	{
                return FALSE;
			}
		}
	
		// JB 010109 CTD sanity check
		//if (theMissile->launchState == MissileClass::PreLaunch)
		if (theMissile->launchState == MissileClass::PreLaunch && curHardpoint != -1)
		// JB 010109 CTD sanity check
		{
			// Set the missile position on the AC
			wpnNum = min (hardPoint[curHardpoint]->NumPoints() - 1, curWpnNum);
			hardPoint[curHardpoint]->GetSubPosition(wpnNum, &x, &y, &z);
			hardPoint[curHardpoint]->GetSubRotation(wpnNum, &az, &el);

			MonoPrint("Platform %.4f %.4f %.4f\n",ownship->XPos(), ownship->YPos(), ownship->ZPos());
			MonoPrint("Launching Missile xyz:%.4f %.4f %.4f  az:%.4f  el:%.4f\n",x,y,z,az,el);
			
			// MLR 5/17/2004 - I was testing this addition below
			//theMissile->SetDelta(ownship->XDelta(), ownship->YDelta(), ownship->ZDelta());

			theMissile->SetLaunchPosition (x, y, z);
			theMissile->SetLaunchRotation (az, el);
			
// 2001-03-02 MOVED HERE BY S.G.
			if ( theMissile->sensorArray && theMissile->sensorArray[0]->Type() == SensorClass::RadarHoming )
			{
				// Have the missile use the launcher's radar for guidance
				((BeamRiderClass*)theMissile->sensorArray[0])->SetGuidancePlatform( ownship );
			}
// END OF MOVED SECTION

			// Don't hand off ground targets to radar guided air to air missiles
			if (curWeaponType != wtAim120 || (tmpTargetPtr && !tmpTargetPtr->BaseData()->OnGround()))
			{
				theMissile->Start (tmpTargetPtr);
			}
			else
			{
				theMissile->Start (NULL);
			}
			
/* 2001-03-02 MOVED BY S.G. ABOVE THE ABOVE IF. I NEED radarPlatform TO BE SET BEFORE I CAN CALL theMissile->Start SINCE I'LL CALL THE BeamRiderClass SendTrackMsg MESSAGE WHICH REQUIRES IT
			// Need to give beam riders a pointer to the illuminating radar platform
			if ( theMissile->sensorArray && theMissile->sensorArray[0]->Type() == SensorClass::RadarHoming )
			{
				// Have the missile use the launcher's radar for guidance
				((BeamRiderClass*)theMissile->sensorArray[0])->SetGuidancePlatform( ownship );
			}
*/			

// 2001-04-14 MN moved here from AircraftClass::DoWeapons - play bomb drop sound when flag is set and we are an AG missile



			// assume sounds are internal
			if(ownship)
			{
				int sfxid=0;
				
				//RV - I-Hawk - Include all missiles types here, AA and AG
				if (theMissile && theMissile->parent && FCC && (FCC->GetMasterMode() == FireControlComputer::AirGroundMissile ||
					FCC->GetMasterMode() == FireControlComputer::AirGroundHARM || 
					FCC->GetMasterMode() == FireControlComputer::Missile ||
					FCC->GetMasterMode() == FireControlComputer::MissileOverride||
					FCC->GetMasterMode() == FireControlComputer::Dogfight))
				{
					if (g_nMissileFix & 0x80)
					{
						Falcon4EntityClassType* classPtr;
						classPtr = (Falcon4EntityClassType*)theMissile->EntityType();
						WeaponClassDataType *wc = NULL;
						
						if (classPtr)
						{
							wc = (WeaponClassDataType*)classPtr->dataPtr; // this is important
						}
						
						if (wc && (wc->Flags & WEAP_BOMBDROPSOUND))	// for JSOW, JDAM...
						{
							sfxid = SFX_BOMBDROP;
						}

						//I-Hawk - get sound ID from missile FM file
						else
						{
							sfxid = theMissile->GetLaunchSound();
						}
					}
					//I-Hawk - get sound ID from missile FM file
					else
					{
						sfxid = theMissile->GetLaunchSound();
					}
				}
				else 
				{
					if (theMissile && !theMissile->parent)
					{
						sfxid = SFX_MISSILE2;
					}
				}

				//RV - I-Hawk - This sound call was commented before...
				//dono why, because this way AGMs never played any launch sound
				//Now fixed to play correct sound based on ID value set in missile FM data
				ownship->SoundPos.Sfx( sfxid, 0, 1, 0);
			}



			// Leonr moved here to avoid per frame database search
			// edg: moved insertion into vudatabse here.  I need
			// the missile to be retrievable from the db when
			// processing the weapon fire message
			vuDatabase->/*Quick*/Insert(theMissile);
			theMissile->Wake();
	
			FCC->lastMissileImpactTime = FCC->nextMissileImpactTime;
			
			lastWeapon = curWeapon.get();
			isCaged = theMissile->isCaged;
			isSpot = theMissile->isSpot;	// Marco Edit - SPOT/SCAN Support
			isSlave = theMissile->isSlave;	// Marco Edit - BORE/SLAVE Support
			isTD = theMissile->isTD;		// Marco Edit - TD/BP Support

			wpnStation = curHardpoint;
			wpnNum     = curWpnNum;
			WeaponStep(TRUE);
			if (lastWeapon == curWeapon)
			{
				SetCurrentWeapon(-1, NULL);
			}
			else
			{
				//MI if we launch one uncaged, the next one's caged again!
				if (curWeapon)
				{
					((MissileClass*)curWeapon.get())->isCaged = TRUE;
					((MissileClass*)curWeapon.get())->isSpot = isSpot;
					((MissileClass*)curWeapon.get())->isSlave = isSlave;
					((MissileClass*)curWeapon.get())->isTD = isTD;
				}
			}
			retval = TRUE;
			
			vc = GetVehicleClassData(ownship->Type() - VU_LAST_ENTITY_TYPE);
			visFlag = vc->VisibleFlags;
			
			if (visFlag & (1 << curHardpoint) && theMissile->drawPointer)
			{
				// Detach visual from parent
				hardPoint[wpnStation]->DetachWeaponBSP(theMissile); // MLR 2/21/2004 - 
				/*
				if (hardPoint[wpnStation]->GetRackOrPylon()) // MLR 2/20/2004 - added OrPylon
				{
					OTWDriver.DetachObject(hardPoint[wpnStation]->GetRackOrPylon(),	(DrawableBSP*)(theMissile->drawPointer), theMissile->GetRackSlot());// MLR 2/20/2004 - added OrPylon
				}
				else if (ownship->drawPointer && wpnStation)
				{
					OTWDriver.DetachObject((DrawableBSP*)(ownship->drawPointer), (DrawableBSP*)(theMissile->drawPointer), wpnStation-1);
				}
				*/
			}
			
			if (UnlimitedAmmo())
				ReplaceMissile(wpnStation, theMissile);
			else
			{
				DecrementStores(hardPoint[wpnStation]->GetWeaponClass(), 1);
				RemoveStore(wpnStation, hardPoint[wpnStation]->weaponId);
				hardPoint[wpnStation]->weaponCount --;
				if (hardPoint[wpnStation]->weaponCount >= hardPoint[wpnStation]->NumPoints())
				{
					ReplaceMissile(wpnStation, theMissile);
				}
				else
				{
					DetachWeapon(wpnStation, theMissile);

					if (ownship->IsLocal ())
					{
						FalconTrackMessage* trackMsg = new FalconTrackMessage (1,ownship->Id (), FalconLocalGame);
						trackMsg->dataBlock.trackType = Track_RemoveWeapon;
						trackMsg->dataBlock.hardpoint = wpnStation;
						trackMsg->dataBlock.id = ownship->Id ();
						
						// Send our track list
						FalconSendMessage (trackMsg, TRUE);
					}
				}
			}			
		}
		
		ClearFlag (Firing);
	}

	//MI SOI after firing Mav
	if(g_bRealisticAvionics) 
	{
		if(curWeapon && curWeapon->parent &&
		((AircraftClass *)curWeapon->parent.get())->IsPlayer() &&
		(curWeapon->GetSPType() == SPTYPE_AGM65A ||
		curWeapon->GetSPType() == SPTYPE_AGM65B ||
		curWeapon->GetSPType() == SPTYPE_AGM65D ||
		curWeapon->GetSPType() == SPTYPE_AGM65G))
		{
			AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
			if(playerAC)
			{
				playerAC->SOIManager(SimVehicleClass::SOI_WEAPON);
			}
		}
	}
	
	return (retval);
}

void SMSBaseClass::ReplaceMissile(int station, MissileClass *theMissile)
{
	VuBin<SimWeaponClass> weapPtr, newMissile, lastPtr;
	int	visFlag;
	int	slotId;

	if ( IsSet( GunOnBoard ) ){
	    slotId = max(0,station - 1);
	}
	else {
		slotId = station;
	}
	
	newMissile.reset(InitAMissile (ownship, hardPoint[station]->weaponId, theMissile->GetRackSlot()));
	MissileClass *nm = static_cast<MissileClass*>(newMissile.get());
	nm->isCaged = TRUE;
	nm->isSpot = FALSE;
	nm->isSlave = TRUE;
	nm->isTD = FALSE;
	visFlag = GetVehicleClassData(ownship->Type() - VU_LAST_ENTITY_TYPE)->VisibleFlags;
	if (visFlag & (1 << station))
	{
		ShiAssert( slotId >= 0 );

		CreateDrawable(newMissile.get(), OTWDriver.Scale());
	
		// Fix up drawable parent/child relations
		hardPoint[station]->AttachWeaponBSP(newMissile.get()); // MLR 2/21/2004 - 

		/*
		if (hardPoint[station]->GetRackOrPylon())// MLR 2/20/2004 - added OrPylon
			OTWDriver.AttachObject(hardPoint[station]->GetRackOrPylon(), (DrawableBSP*)(newMissile->drawPointer), theMissile->GetRackSlot());// MLR 2/20/2004 - added OrPylon
		else if (ownship->drawPointer)
			OTWDriver.AttachObject((DrawableBSP*)(ownship->drawPointer), (DrawableBSP*)(newMissile->drawPointer), slotId );
			*/
	}
	
	weapPtr = hardPoint[station]->weaponPointer;
	while (weapPtr){
		if (weapPtr.get() == theMissile){
			if (lastPtr){
				lastPtr->nextOnRail = newMissile;
			}
			else {
				hardPoint[station]->weaponPointer = newMissile;
			}
			newMissile->nextOnRail = theMissile->nextOnRail;
			theMissile->nextOnRail.reset();
			return;
		}
		lastPtr = weapPtr;
		weapPtr = weapPtr->nextOnRail;
	}

	//edg: we neglected to account for the case where the replaced missile
	//is already detatched (as it is in SMS BAse Launch Weapon) and thereby
	// leak missile mem.  If we got here, we need to attach the missile at
	// the end of the chain
	newMissile->nextOnRail.reset();
	if (lastPtr){
		lastPtr->nextOnRail = newMissile;
	}
	else{
		hardPoint[station]->weaponPointer = newMissile;
	}
}

#if 0
// MLR 1/11/2004 - LaunchRocket() returns TRUE to stop the current Salvo.
int SMSClass::LaunchRocket (void)
{
	MissileClass* theMissile;
	int retval = FALSE;
	SimBaseClass* lastWeapon;
	FireControlComputer *FCC = ownship->GetFCC();
	SimObjectType* tmpTargetPtr;
	int wpnNum;
	static count = 0;
	float az, el, x, y, z;
	
	if (!FCC)
	{
		ClearFlag (Firing);
		return retval;
	}
	
	// Check for SMS Failure
	if (!CurStationOK() ||
		!curWeapon || 
        Ownship()->OnGround() ||
        MasterArm() != Arm)
	{
		return retval;
	}
	
	tmpTargetPtr = FCC->TargetPtr();
	ShiAssert(hardPoint[curHardpoint]->weaponPointer->IsMissile());

	// MLR 1/27/2004 - For gun pods, we won't allocate 1200 (or whatever) rounds of ammo
	//                 that would be silly.  We will allocate them as needed.
	if(hardPoint[curHardpoint]->weaponCount && !hardPoint[curHardpoint]->weaponPointer)
	{
		hardPoint[curHardpoint]->weaponPointer = InitWeaponList (ownship, hardPoint[curHardpoint]->weaponId,
						hardPoint[curHardpoint]->GetWeaponClass(), min( 10, hardPoint[curHardpoint]->weaponCount ), InitAMissile);

	}

	theMissile = (MissileClass*)(hardPoint[curHardpoint]->weaponPointer);
	// MLR 1/12/2004 - wtf, why fire every other call? (due to count)
	// I'm not going to change it.
	if (theMissile && count == 0)
	{
		if (hardPoint[curHardpoint]->GetRack()) // MLR 1/12/2004 - just incase the pylon is the pod (as in the IA).
			hardPoint[curHardpoint]->GetRack()->SetSwitchMask(0, 0); 

		// MLR 1/12/2004 - If the Pod is attached, set switch 0 to 0
		if(hardPoint[curHardpoint]->podPointer && hardPoint[curHardpoint]->podPointer->drawPointer)
		{
			((DrawableBSP*)(hardPoint[curHardpoint]->podPointer->drawPointer))->SetSwitchMask(0,0);
		}
		SetFlag(Firing);
		count = 2;
		wpnNum = min (hardPoint[curHardpoint]->NumPoints() - 1, curWpnNum);
		hardPoint[curHardpoint]->GetSubPosition(wpnNum, &x, &y, &z);
		hardPoint[curHardpoint]->GetSubRotation(wpnNum, &az, &el);
		theMissile->SetLaunchPosition (x, y, z);

		// maybe we want to add some random dispersion here later?
		theMissile->SetLaunchRotation (az, el);
		if (tmpTargetPtr)
		{
			theMissile->Start (tmpTargetPtr);
		}
		else
		{
			theMissile->Start (NULL);
		}
		
		// Leonr moved here to avoid per frame database search
		// edg: moved insertion into vudatabse here.  I need
		// the missile to be retrievable from the db when
		// processing the weapon fire message
		vuDatabase->QuickInsert(theMissile);
		// KCK: WARNING: these things already have drawables BEFORE wake is called.
		// Make Wake() not assign new drawables if one exists?
		theMissile->Wake();
		
		//		MonoPrint ("Inserting rocket %p\n", theMissile);
		FCC->lastMissileImpactTime = FCC->nextMissileImpactTime;
		
		// Remove it from the parent
		hardPoint[curHardpoint]->weaponCount --;
		DetachWeapon(curHardpoint, theMissile);
		/* // MLR 5/17/2004 - Was testing something
		if(theMissile->drawPointer)
		{
			vector3 pos;
			
			theMissile->drawPointer->GetPosition(&pos);
			theMissile->SetLaunchPosition (pos.x, pos.y, pos.z);

		}
		*/
		
		curWeapon = hardPoint[curHardpoint]->weaponPointer;
		// Only the last rocket has a nametag
		// MLR label all rockets
		//if (hardPoint[curHardpoint]->weaponPointer)
		//	theMissile->drawPointer->SetLabel("", 0xff00ff00);
		// End TYPE_ROCKET section
	}
	count --;
	
	// check to see if we've used up a hardpoint
	if (hardPoint[curHardpoint]->weaponCount == 0 )
	{
		count = 0;

		retval = TRUE;
		wpnNum = curHardpoint;
		
		// Reload if possible / wanted
		if (theMissile && UnlimitedAmmo())
			ReplaceRocket(wpnNum, theMissile);
		
		lastWeapon = curWeapon;
		WeaponStep(TRUE);
		
		if (lastWeapon == curWeapon)
		{
			ReleaseCurWeapon(-1);
		}
		
		ClearFlag (Firing);
	}
	else
	{
		// MLR 1/12/2004 - even if we haven't expended all the rockets on one harpoint
		// we may have expended one pod (incase multiple pods are loaded).
		// if so, we need to stop the salvo from firing.
		if(hardPoint[curHardpoint]->weaponsPerPod) // MLR 1/27/2004 - Div(Mod) by Zero fix 
		{
			if((hardPoint[curHardpoint]->weaponCount % hardPoint[curHardpoint]->weaponsPerPod) == 0)
			{
				// MLR 1/12/2004 - we've used a pod, shuffle the pointers so we set the 
				// switch on the next pod the next LaunchRocket() 
				// also return TRUE so that we stop firing this salvo
				count = 0;
				retval=TRUE;
				ClearFlag (Firing);
				if(hardPoint[curHardpoint]->podPointer)
				{
					SimWeaponClass *tempPod,**work;
					
					// remove the first pod, store in tempPod
					tempPod =  hardPoint[curHardpoint]->podPointer;
					hardPoint[curHardpoint]->podPointer=tempPod->nextOnRail;
					tempPod->nextOnRail = 0;
					
					// put tempPod on the end of the pod list
					work    = &hardPoint[curHardpoint]->podPointer;
					while(*work)
					{
						work= &((*work)->nextOnRail);
					}
					*work=tempPod;
				}
			}
			else
			{
				// finally, check to see if we've used a salvo, if so, stop firing.
				if(hardPoint[curHardpoint]->podSalvoSize>0)
					if((hardPoint[curHardpoint]->weaponCount % hardPoint[curHardpoint]->podSalvoSize) == 0)
					{
						count = 0;
						retval=TRUE;
						ClearFlag (Firing);
					}
			}
		}
	}
	return (retval);
}

void SMSBaseClass::ReplaceRocket(int station, MissileClass *theMissile)
{
/*
	hardPoint[station]->weaponCount = 19;
	hardPoint[station]->weaponPointer = InitWeaponList (ownship, hardPoint[station]->weaponId,
      hardPoint[station]->GetWeaponClass(), hardPoint[station]->weaponCount, InitAMissile);
*/


	// Change hacks to datafile: terrdata\objects\falcon4.rkt
	int entryfound = 0;
	for (int j=0; j<NumRocketTypes; j++)
	{
		if (hardPoint[station]->weaponId == RocketDataTable[j].weaponId)
		{
			hardPoint[station]->weaponCount = RocketDataTable[j].weaponCount;
			hardPoint[station]->weaponPointer = InitWeaponList (ownship, hardPoint[station]->weaponId,
			  hardPoint[station]->GetWeaponClass(), hardPoint[station]->weaponCount, InitAMissile);
			entryfound = 1;
			break;
		}
	}
	if (!entryfound)	// use generic 2.75mm rocket
	{
		hardPoint[station]->weaponCount = 19;
		hardPoint[station]->weaponPointer = InitWeaponList (ownship, hardPoint[station]->weaponId,
		  hardPoint[station]->GetWeaponClass(), hardPoint[station]->weaponCount, InitAMissile);
	}
}
#endif

#include "bomb.h"
// MLR 1/11/2004 - LaunchRocket() returns TRUE to stop the current Salvo.
// launchers are bombclass objects now
// they fire missiles (and maybe other bombs later)
int SMSClass::LaunchRocket (void)
{
	BombClass *theLau;

	theLau = (BombClass *)curWeapon.get();
	if(!theLau)
	{
		WeaponStep(FALSE);
		theLau = (BombClass *)curWeapon.get();
	}

	if(theLau && !theLau->IsLauncher())
	{
		return 1;
	}

	int fire = 1;
	if( GetAGBPair() )
		fire = 2;
	//fire*=GetAGBRippleCount()

	int l;
	for(l=0;l<fire;l++)
	{
		theLau = (BombClass *)curWeapon.get();
		if(theLau && theLau->IsLauncher() && theLau->LauGetRoundsRemaining())
		{
			theLau->LauFireSalvo(); // tell lau to add a salvo to the firecount
			if(theLau->LauGetRoundsRemaining()==0)
			{
				if (!UnlimitedAmmo() && hardPoint && (curHardpoint >= 0))
				{
					hardPoint[curHardpoint]->weaponCount--;
					if(hardPoint[curHardpoint]->weaponCount<0)
						hardPoint[curHardpoint]->weaponCount=0;
				}
			}
		}
		WeaponStep();
	}

	// we use runRockets to determine if RunRockets() needs to do anything.
	runRockets = 1;

	// always stop firing now
	return(1);
}

void SMSClass::RunRockets(void)
{
	if(runRockets)
	{
		SetFlag(Firing);
		int l;
		runRockets = 0;

		BombClass *theLau;

		for(l = 1;  l < numHardpoints; l++){
			theLau = (BombClass *)hardPoint[l]->weaponPointer.get();
			while(theLau){
				if(theLau->IsLauncher()){
					if(theLau->LauIsFiring()){
						FireRocket(l, theLau);
						runRockets = 1;
					}
				}
				theLau = (BombClass *)theLau->GetNextOnRail();
			}
		}
	}
	else {
		ClearFlag (Firing);
	}
}


void SMSClass::FireRocket (int hpId, BombClass *theLau)
{
	MissileClass* theMissile;
	FireControlComputer *FCC = ownship->GetFCC();
	int wpnNum;
	float az, el, x, y, z;
	
	if (theLau->LauCheckTimer())
	{
		// create a new rocket/missile/whatever
		theMissile = (MissileClass*)InitAMissile(ownship, theLau->LauGetWeaponId(), 0);

		theLau->LauRemFiredRound(); // update internal counters;

		if (UnlimitedAmmo())
		{
			theLau->LauAddRounds(1);
		}
		else
		{
			RemoveStore(hpId, theLau->LauGetWeaponId());
		}

		if (theLau->drawPointer)
		{
			((DrawableBSP *)(theLau->drawPointer))->SetSwitchMask(0, 0);
		}

		wpnNum = min (hardPoint[hpId]->NumPoints() - 1, theLau->GetRackSlot());

		MonoPrint("Firing Rocket hpId:%d  wpnNum:%d",hpId,wpnNum);

		hardPoint[hpId]->GetSubPosition(wpnNum, &x, &y, &z);
		hardPoint[hpId]->GetSubRotation(wpnNum, &az, &el);

		theMissile->SetLaunchPosition (x, y, z);
		theMissile->SetLaunchRotation (0, 0);

		theMissile->Start (FCC->TargetPtr());
		
		vuDatabase->/*Quick*/Insert(theMissile);
		theMissile->Wake();
		
		FCC->lastMissileImpactTime = FCC->nextMissileImpactTime;
	}


}

//////////////////////////////////////////

#if 0
int SMSClass::SubLaunchRocket (int hpId)
{
	BombClass *theLau;
	MissileClass* theMissile;
	int retval = FALSE;
	SimBaseClass* lastWeapon;
	FireControlComputer *FCC = ownship->GetFCC();
	SimObjectType* tmpTargetPtr;
	int wpnNum;
	float az, el, x, y, z;
	
	if (!FCC)
	{
		ClearFlag (Firing);
		return TRUE; // MLR 5/30/2004 - was false
	}
	
	// Check for SMS Failure
	if (!CurStationOK() ||
		!curWeapon || 
        Ownship()->OnGround() ||
        MasterArm() != Arm)
	{
		return TRUE; // MLR 5/30/2004 - was false
	}

	theLau = (BombClass *)hardPoint[hpId]->weaponPointer;
	if( theLau->IsLauncher()==0 ) 
		return 1;

	while(theLau && theLau->LauGetRoundsRemaining()==0)
	{
		theLau = (BombClass *)theLau->GetNextOnRail();
	}
	

	if(!theLau)
		return 1;

	if( theLau->LauGetWeaponId()==0 )
		return 1;

	tmpTargetPtr = FCC->TargetPtr();
//	ShiAssert(hardPoint[hpId]->weaponPointer->IsMissile());

	// MLR 1/27/2004 - For gun pods, we won't allocate 1200 (or whatever) rounds of ammo
	//                 that would be silly.  We will allocate them as needed.
	/*
	if(hardPoint[hpId]->weaponCount && !hardPoint[hpId]->weaponPointer)
	{
		hardPoint[hpId]->weaponPointer = InitWeaponList (ownship, hardPoint[hpId]->weaponId,
						hardPoint[hpId]->GetWeaponClass(), min( 10, hardPoint[hpId]->weaponCount ), InitAMissile);

	}
	*/

	if (theLau->LauCheckTimer())
	{
		// create a new rocket/missile/whatever
		theMissile = (MissileClass*)InitAMissile(ownship, theLau->LauGetWeaponId(), 0);
		theLau->LauAddRounds(-1);
		// MLR 3/15/2004 - remove the weapon weight
		RemoveStore(hpId, theLau->LauGetWeaponId());

		if (theLau->drawPointer)
		{
			((DrawableBSP *)(theLau->drawPointer))->SetSwitchMask(0, 0);
		}

		SetFlag(Firing);
		wpnNum = min (hardPoint[hpId]->NumPoints() - 1, curWpnNum);
		hardPoint[hpId]->GetSubPosition(wpnNum, &x, &y, &z);
		hardPoint[hpId]->GetSubRotation(wpnNum, &az, &el);
		theMissile->SetLaunchPosition (x, y, z);
		//theMissile->SetLaunchRotation (az, el);
		theMissile->SetLaunchRotation (0, 0);
		if (tmpTargetPtr)
		{
			theMissile->Start (tmpTargetPtr);
		}
		else
		{
			theMissile->Start (NULL);
		}
		
		// Leonr moved here to avoid per frame database search
		// edg: moved insertion into vudatabse here.  I need
		// the missile to be retrievable from the db when
		// processing the weapon fire message
		vuDatabase->QuickInsert(theMissile);
		// KCK: WARNING: these things already have drawables BEFORE wake is called.
		// Make Wake() not assign new drawables if one exists?
		theMissile->Wake();
		
		//		MonoPrint ("Inserting rocket %p\n", theMissile);
		FCC->lastMissileImpactTime = FCC->nextMissileImpactTime;
	}



	if(theLau->LauGetRoundsRemaining()==0)
	{
		retval = 1;
		ClearFlag (Firing);
		if (UnlimitedAmmo())
		{
			int rnds = theLau->LauGetRoundsRemaining();
			int max = theLau->LauGetMaxRounds();
			int wid = theLau->LauGetWeaponId();

			for(;rnds<max;rnds++)
			{
				AddStore(hpId, wid, 0); // rockets are not "visible" - no extra drag
			}

			theLau->LauSetRoundsRemaining(max);
			//ReplaceRocket(hpId);
		}
		else
		{
			hardPoint[hpId]->weaponCount --;
			lastWeapon = theLau;
			WeaponStep(TRUE);
			
			if (lastWeapon == curWeapon)
			{
				ReleaseCurWeapon(-1);
			}
		}
	}
	else
	{
		// stop firing if we've fired a salvo
		int ss = theLau->LauGetSalvoSize();
		if(ss > 0)
		{
			int exp = theLau->LauGetMaxRounds() - theLau->LauGetRoundsRemaining(); // expended rounds
			if((exp % ss) == 0)
			{
				retval = 1;
				ClearFlag (Firing);
			}
		}
	}
	return (retval);
}
#endif

void SMSBaseClass::ReplaceRocket(int station)
{
	BombClass *theLau;

	theLau = (BombClass *)hardPoint[station]->weaponPointer.get();
	if(theLau && theLau->IsLauncher()){
		while(theLau){
			theLau->LauSetRoundsRemaining(theLau->LauGetMaxRounds());
			theLau = (BombClass *)theLau->GetNextOnRail();
		}
	}
}