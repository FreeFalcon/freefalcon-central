#include "stdhdr.h"
#include "sms.h"
#include "bomb.h"
#include "bombfunc.h"
#include "hardpnt.h"
#include "simveh.h"
#include "otwdrive.h"
#include "Graphics\Include\drawbsp.h"
#include "falcsess.h"
#include "entity.h"
#include "vehicle.h"
#include "aircrft.h"
#include "fack.h"
#include "MsgInc\WeaponFireMsg.h"
#include "MsgInc\TrackMsg.h"
#include "fcc.h"
#include "simdrive.h"
#include "ivibedata.h"
#include "digi.h" // 2002-02-26 S.G.
#include "grtypes.h"
#include "soundfx.h" // MLR 6/4/2004 - 

void CreateDrawable (SimBaseClass* theObject, float objectScale);

extern bool g_bWeaponLaunchUsesDrawPointerPos; // MLR 2/19/2004 

int SMSClass::DropBomb(int allowRipple)
{
	VuBin<BombClass> theBomb;
	SimBaseClass* lastWeapon;
	int retval = FALSE;
	vector pos, posDelta;
	float initXloc, initYloc, initZloc;
	int matchingStation, i;
	VehicleClassDataType* vc;
	int visFlag;
	float dragCoeff = 0.0f;
	int slotId;
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();


   // Check for SMS Failure or other reason not to drop
   if (!CurStationOK() || //Weapon Station failure
        ownship->OnGround() || // Weight on wheels inhibit
       !curWeapon || // No Weapon
       ownship->GetNz() < 0.0F || // Negative Gs
       MasterArm() != Arm) // Not in arm mode
   {
      ownship->GetFCC()->bombPickle = FALSE;
      return retval;
   }
   
	// edg: there was a problem.  Some ground vehicles do NOT have guns
	// at all.  All this code assumes a gun in Slot 0 and therefore subtracts
	// 1 from curhardpoint to get the right slot.  Don't do this if there
	// are no guns on board.
	if ( IsSet( GunOnBoard ) )
		slotId = max(0,curHardpoint - 1);
	else
		slotId = curHardpoint;

	// Verify curWeapon on curHardpoint
	if (curHardpoint < 0 || hardPoint[curHardpoint]->weaponPointer.get() != curWeapon)
	{
		for (i=0; i<numHardpoints; i++)
		{
			if(hardPoint[i]->weaponPointer.get() == curWeapon){
				curHardpoint = i;
				if ( IsSet( GunOnBoard ) ){
					slotId = max(0,curHardpoint - 1);
				}
				else{
					slotId = curHardpoint;
				}
				break;
			}
		}
	}

	if (curWeapon && fabs(ownship->Roll()) < 60.0F * DTR)
	{
		F4Assert(curWeapon->IsBomb());

		dragCoeff  = hardPoint[curHardpoint]->GetWeaponData()->cd;

		theBomb.reset((BombClass *)curWeapon.get());
		// edg: it's been observed that theBomb will be NULL at times?!
		// leonr: This happens when you ask for too many release pulses.
		if ( theBomb == NULL ){
			return retval;
		}
		SetFlag (Firing);
		hardPoint[curHardpoint]->GetSubPosition(curWpnNum, &initXloc, &initYloc, &initZloc);

		pos.x = 
			ownship->XPos() + 
			ownship->dmx[0][0]*initXloc + ownship->dmx[1][0]*initYloc + ownship->dmx[2][0]*initZloc
		;
		pos.y = 
			ownship->YPos() + 
			ownship->dmx[0][1]*initXloc + ownship->dmx[1][1]*initYloc + ownship->dmx[2][1]*initZloc
		;
		pos.z = 
			ownship->ZPos() + 
			ownship->dmx[0][2]*initXloc + ownship->dmx[1][2]*initYloc + ownship->dmx[2][2]*initZloc
		;
	 
		posDelta.x = ownship->XDelta();
		posDelta.y = ownship->YDelta();
		posDelta.z = ownship->ZDelta();
		theBomb->SetBurstHeight(burstHeight);
		SimObjectType *tgt = NULL;
		DigitalBrain *db = (DigitalBrain*)ownship->Brain();
		tgt = db == NULL ? NULL : db->GetGroundTarget();
		theBomb->Start(&pos, &posDelta, dragCoeff, tgt); 
		// 2002-02-26 MODIFIED BY S.G. Added the ownship... 
		// in case the target is aggregated and an AI is bombing it
	  
		//Wombat778 03-09-04 Copy the current ground designated point into the bomb
		AircraftClass *self = ((AircraftClass*)playerAC);
		//AircraftClass *self = ((AircraftClass*)ownship->DriveEntity);
		if (theBomb && ownship->GetFCC())
		{
			// Cobra - targets for all
   		if (theBomb->IsSetBombFlag(BombClass::IsGPS) 
						|| theBomb->IsSetBombFlag(BombClass::IsJSOW))
			{
				// Cobra - Dynamic select of target from list
				//TOO mode  //Cobra - !JDAMsbc = PB aggregated - Temp CTD fix
				if (((AircraftClass*)ownship)->GetSMS()->JDAMtargeting == SMSBaseClass::TOO 
							|| !(((AircraftClass*)ownship)->JDAMsbc))
				{
					theBomb->gpsx = ownship->GetFCC()->groundDesignateX;
					theBomb->gpsy = ownship->GetFCC()->groundDesignateY;
					theBomb->JSOWtgtID = ((AircraftClass*)ownship)->JDAMtgtnum;
					theBomb->JSOWtgtPos = ((AircraftClass*)ownship)->JDAMtgtPos;
					if (((AircraftClass*)ownship)->JDAMAllowAutoStep)
						((AircraftClass*)ownship)->JDAMStep = 1;
					else
						((AircraftClass*)ownship)->JDAMStep = 0;
					((AircraftClass*)ownship)->JDAMtgtnum = ((AircraftClass*)ownship)->GetJDAMPBTarget((AircraftClass*)ownship);
				}
				else 
				{
					//PB mode
					theBomb->gpsx = ((AircraftClass*)ownship)->JDAMsbc->XPos();
					theBomb->gpsy = ((AircraftClass*)ownship)->JDAMsbc->YPos();
					theBomb->JSOWtgtID = ((AircraftClass*)ownship)->JDAMtgtnum;
					theBomb->JSOWtgtPos = ((AircraftClass*)ownship)->JDAMtgtPos;
					if ( ((AircraftClass*)ownship)->JDAMAllowAutoStep )
						((AircraftClass*)ownship)->JDAMStep = 1;
					else
						((AircraftClass*)ownship)->JDAMStep = 0;
					((AircraftClass*)ownship)->JDAMtgtnum = ((AircraftClass*)ownship)->GetJDAMPBTarget((AircraftClass*)ownship);
				}
			}
			else 
			{
				theBomb->gpsx=ownship->GetFCC()->groundDesignateX;
				theBomb->gpsy=ownship->GetFCC()->groundDesignateY;
			}
			theBomb->gpsz=OTWDriver.GetGroundLevel(theBomb->gpsx,theBomb->gpsy);
		}

				  
		vc = GetVehicleClassData(ownship->Type() - VU_LAST_ENTITY_TYPE);
		visFlag = vc->VisibleFlags;

		if (visFlag & (1 << curHardpoint) && theBomb->drawPointer){
			// Detach visual from parent
			hardPoint[curHardpoint]->DetachWeaponBSP(theBomb.get()); // MLR 2/21/2004 - 
			/*
			if (hardPoint[curHardpoint]->GetRackOrPylon()) // MLR 2/20/2004 - added OrPylon
			{
				OTWDriver.DetachObject(hardPoint[curHardpoint]->GetRackOrPylon(),
				(DrawableBSP*)(theBomb->drawPointer), theBomb->GetRackSlot());// MLR 2/20/2004 - added OrPylon
			}
			else if (ownship->drawPointer && curHardpoint && theBomb->drawPointer)
			{
				OTWDriver.DetachObject((DrawableBSP*)(ownship->drawPointer), (DrawableBSP*)(theBomb->drawPointer), slotId);
			}
			*/
		}

		if (UnlimitedAmmo()){
			ReplaceBomb(curHardpoint, theBomb.get());
		}
		else {
			DecrementStores(hardPoint[curHardpoint]->GetWeaponClass(), 1);
			hardPoint[curHardpoint]->weaponCount --;
			if (hardPoint[curHardpoint]->weaponCount >= hardPoint[curHardpoint]->NumPoints()){
				ReplaceBomb(curHardpoint, theBomb.get());
			}
			else {
				DetachWeapon(curHardpoint, theBomb.get());
				if (ownship->IsLocal ()){
					FalconTrackMessage* trackMsg = new FalconTrackMessage (1,ownship->Id (), FalconLocalGame);
					trackMsg->dataBlock.trackType = Track_RemoveWeapon;
					trackMsg->dataBlock.hardpoint = curHardpoint;
					trackMsg->dataBlock.id = ownship->Id ();
					// Send our track list
					FalconSendMessage (trackMsg, TRUE);
				}
			}
		}

		RemoveStore(curHardpoint, hardPoint[curHardpoint]->weaponId);

		// Make it live
		vuDatabase->/*Quick*/Insert(theBomb.get());
		ownship->SoundPos.Sfx( SFX_BOMBDROP, 0, 1, 0); // MLR 6/4/2004 - 

		if (ownship == FalconLocalSession->GetPlayerEntity()){
			g_intellivibeData.BombDropped++;
		}
		// Note: The drawable for this object has already been created!
		theBomb->Wake();
		// Record the drop
		ownship->SendFireMessage (theBomb.get(), FalconWeaponsFire::BMB, TRUE, ownship->targetPtr);

		lastWeapon = curWeapon.get();
		matchingStation = curHardpoint;
		WeaponStep(TRUE);
		if (lastWeapon == curWeapon){
			curWeapon.reset();
		}

		// Want to drop a pair - No pairs w/ only one rack of bombs unless from centerline
		//if (pair && allowRipple && (curHardpoint != matchingStation || curHardpoint == (numHardpoints / 2 + 1)))
		// Cobra - 
		//if (GetAGBPair() && allowRipple && (curHardpoint != matchingStation || curHardpoint == (numHardpoints / 2 + 1)))
		if (GetAGBPair() && (curHardpoint != matchingStation || curHardpoint == (numHardpoints / 2 + 1)))
		{
			theBomb.reset((BombClass *)curWeapon.get());

			if (theBomb)
			{
				hardPoint[curHardpoint]->GetSubPosition(curWpnNum, &initXloc, &initYloc, &initZloc);
				pos.x = ownship->XPos() + ownship->dmx[0][0]*initXloc + ownship->dmx[1][0]*initYloc + ownship->dmx[2][0]*initZloc;
				pos.y = ownship->YPos() + ownship->dmx[0][1]*initXloc + ownship->dmx[1][1]*initYloc + ownship->dmx[2][1]*initZloc;
				pos.z = ownship->ZPos() + ownship->dmx[0][2]*initXloc + ownship->dmx[1][2]*initYloc + ownship->dmx[2][2]*initZloc;
				posDelta.x = ownship->XDelta();
				posDelta.y = ownship->YDelta();
				posDelta.z = ownship->ZDelta();
				theBomb->SetBurstHeight (burstHeight);
				SimObjectType *tgt = NULL;
				DigitalBrain *db = (DigitalBrain*)ownship->Brain();
				tgt = db == NULL ? NULL : db->GetGroundTarget();
				theBomb->Start(&pos, &posDelta, dragCoeff, tgt); 
				// 2002-02-26 MODIFIED BY S.G. Added the ownship... 
				// in case the target is aggregated and an AI is bombing it

				//Wombat778 03-09-04 Copy the current ground designated point into the bomb	  
				if (theBomb && ownship->GetFCC())
				{
					// Cobra - targets for all
					//if (theBomb->IsSetBombFlag(BombClass::IsGPS))
					//if (ownship->IsPlayer() && (theBomb->IsSetBombFlag(BombClass::IsGPS) 
					// FRB - for all
					if ((theBomb->IsSetBombFlag(BombClass::IsGPS) || theBomb->IsSetBombFlag(BombClass::IsJSOW)))
					{
						// Cobra - Dynamic select of target from list
						if (((AircraftClass*)ownship)->GetSMS()->JDAMtargeting == SMSBaseClass::TOO 
									|| !(((AircraftClass*)ownship)->JDAMsbc))
						{
							//TOO mode  //Cobra - !JDAMsbc = PB aggregated - Temp CTD fix
							theBomb->gpsx = ownship->GetFCC()->groundDesignateX;
							theBomb->gpsy = ownship->GetFCC()->groundDesignateY;
							theBomb->JSOWtgtID = ((AircraftClass*)ownship)->JDAMtgtnum;
							theBomb->JSOWtgtPos = ((AircraftClass*)ownship)->JDAMtgtPos;
							if ( ((AircraftClass*)ownship)->JDAMAllowAutoStep ){
								((AircraftClass*)ownship)->JDAMStep = 1;
							}
							else 
							{
								((AircraftClass*)ownship)->JDAMStep = 0;
							}
						}
						else//PB mode
						{
							theBomb->gpsx = ((AircraftClass*)ownship)->JDAMsbc->XPos();
							theBomb->gpsy = ((AircraftClass*)ownship)->JDAMsbc->YPos();
							theBomb->JSOWtgtID = ((AircraftClass*)ownship)->JDAMtgtnum;
							theBomb->JSOWtgtPos = ((AircraftClass*)ownship)->JDAMtgtPos;
							if ( ((AircraftClass*)ownship)->JDAMAllowAutoStep )
							{
								((AircraftClass*)ownship)->JDAMStep = 1;
							}
							else
							{
								((AircraftClass*)ownship)->JDAMStep = 0;
							}
						}
					}
					else
					{
						theBomb->gpsx=ownship->GetFCC()->groundDesignateX;
						theBomb->gpsy=ownship->GetFCC()->groundDesignateY;
					}
					theBomb->gpsz=OTWDriver.GetGroundLevel(theBomb->gpsx,theBomb->gpsy);
				}

				if ( IsSet( GunOnBoard ) )
					slotId = max(0,curHardpoint - 1);
				else
					slotId = curHardpoint;

				// Detach visual from parent
				hardPoint[curHardpoint]->DetachWeaponBSP(theBomb.get()); // MLR 2/21/2004 - 

				if (UnlimitedAmmo()){
					ReplaceBomb(curHardpoint, theBomb.get());
				}
				else
				{
					DecrementStores(hardPoint[curHardpoint]->GetWeaponClass(), 1);
					hardPoint[curHardpoint]->weaponCount --;
					if (hardPoint[curHardpoint]->weaponCount >= hardPoint[curHardpoint]->NumPoints()){
						ReplaceBomb(curHardpoint, theBomb.get());
					}
					else {
						DetachWeapon(curHardpoint, theBomb.get());
						if (ownship->IsLocal ()){
							FalconTrackMessage* trackMsg = new FalconTrackMessage (1,ownship->Id (), FalconLocalGame);
							trackMsg->dataBlock.trackType = Track_RemoveWeapon;
							trackMsg->dataBlock.hardpoint = curHardpoint;
							trackMsg->dataBlock.id = ownship->Id ();
							// Send our track list
							FalconSendMessage (trackMsg, TRUE);
						}
					}
				}

				RemoveStore(curHardpoint, hardPoint[curHardpoint]->weaponId);

				// Make it live
				vuDatabase->/*Quick*/Insert(theBomb.get());
				theBomb->Wake();

				// Record the drop
				ownship->SendFireMessage (theBomb.get(), FalconWeaponsFire::BMB, TRUE, ownship->targetPtr);

				// Step again
				lastWeapon = curWeapon.get();
				WeaponStep(TRUE);
				if (lastWeapon == curWeapon){
					curWeapon.reset();
				}
			}
		}

		retval = TRUE;

		if (!curRippleCount){
			// Can we do a ripple drop?
			if (allowRipple)
				//curRippleCount = max (min (rippleCount, numCurrentWpn - 1), 0);
				curRippleCount = max (min (GetAGBRippleCount(), numCurrentWpn - 1), 0);

			//nextDrop = SimLibElapsedTime + FloatToInt32((rippleInterval)/
			nextDrop = SimLibElapsedTime + FloatToInt32((GetAGBRippleInterval())/
			(float)sqrt(ownship->XDelta()*ownship->XDelta() + ownship->YDelta()*ownship->YDelta()) * SEC_TO_MSEC);

			if (!curRippleCount){
				ClearFlag (Firing);
			}
		}
		else {
			if (curRippleCount == 1){
				// Last bomb, ripple count will be decremented in ::Exec
				ClearFlag (Firing);
			}
		}
	}
	return (retval);
}

void SMSBaseClass::ReplaceBomb(int station, BombClass *theBomb)
{
	VehicleClassDataType* vc;
	int visFlag;
	VuBin<SimWeaponClass> weapPtr, newBomb, lastPtr;

	newBomb.reset(InitABomb (ownship, hardPoint[station]->weaponId, theBomb->GetRackSlot()));
	vc = GetVehicleClassData(ownship->Type() - VU_LAST_ENTITY_TYPE);
	visFlag = vc->VisibleFlags;

	if (visFlag & 1 << curHardpoint)
	{
		CreateDrawable(newBomb.get(), OTWDriver.Scale());

		// Fix up drawable parent/child relations
		hardPoint[station]->AttachWeaponBSP(newBomb.get()); // MLR 2/21/2004 - 
		/*
	   if (hardPoint[station]->GetRackOrPylon()) // MLR 2/20/2004 - added OrPylon
		{
		   OTWDriver.AttachObject(hardPoint[station]->GetRackOrPylon(), (DrawableBSP*)(newBomb->drawPointer), theBomb->GetRackSlot()); // MLR 2/20/2004 - added OrPylon
		}
	   else if (ownship->drawPointer)
		{
		   OTWDriver.AttachObject((DrawableBSP*)(ownship->drawPointer), (DrawableBSP*)(newBomb->drawPointer), max (station - 1, 0));
		}*/
   }

	weapPtr = hardPoint[station]->weaponPointer;
	while (weapPtr){
		if (weapPtr.get() == theBomb){
			if (lastPtr){
				lastPtr->nextOnRail = newBomb;
			}
			else{
				hardPoint[station]->weaponPointer = newBomb;
			}
			newBomb->nextOnRail = theBomb->nextOnRail;
			theBomb->nextOnRail.reset();
			return;
		}
		lastPtr = weapPtr;
		weapPtr = weapPtr->nextOnRail;
	}
}

