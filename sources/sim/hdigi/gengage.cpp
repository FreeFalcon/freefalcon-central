#include "stdhdr.h"
#include "hdigi.h"
#include "simveh.h"
#include "object.h"
#include "airframe.h"
#include "missile.h"
#include "fcc.h"
#include "sms.h"
#include "Graphics\Include\drawobj.h"
#include "camp2sim.h"
#include "hardpnt.h"
#include "campbase.h"
#include "fakerand.h"
#include "guns.h"
#include "MsgInc\WeaponFireMsg.h"
#include "fsound.h"
#include "soundfx.h"

#define INIT_GUN_VEL   7000.0F
#define GUN_MAX_RANGE  8000.0F
#define GUN_MAX_ATA    (90.0F * DTR)
char debugbuf[256];

void HeliBrain::GunsEngageCheck(void)
{
   SimVehicleClass *theObject;

   // OutputDebugString("Entering Guns Engange Check\n");
   ClearFlag (MslFireFlag | GunFireFlag);

   // no target */
   if (!targetPtr)
   {
      if (curMode == GunsEngageMode)
	  {
       ClearFlag (MslFireFlag | GunFireFlag);
//		 MonoPrint("HELO BRAIN Exiting Guns Engange\n");
	  }
      return;
   }

   WeaponSelection();
   if ( !anyWeapons )
   {
      if (curMode == GunsEngageMode)
	  {
       ClearFlag (MslFireFlag | GunFireFlag);
//		 MonoPrint("HELO BRAIN Exiting Guns Engange\n");
	  }
      return;
   }

   // we're likely dealing with stale data for target, update range and
   // ata
   float xft, yft, zft;
   float rx;
   theObject = (SimVehicleClass *)targetPtr->BaseData();

   xft = theObject->XPos() - self->XPos();
   yft = theObject->YPos() - self->YPos();
   zft = theObject->ZPos() - self->ZPos();

   rx = self->dmx[0][0]*xft + self->dmx[0][1]*yft + self->dmx[0][2]*zft;

   targetData->range = (float)sqrt(xft*xft + yft*yft + zft*zft);
   targetData->range = max (targetData->range, 1.0F);
   // targetData->ata =  (float)acos(rx/targetData->range);

   // entry 
   if (curMode != GunsEngageMode)
   {
      if ( targetData->range <= GUN_MAX_RANGE * 8.0f )
      {
//         MonoPrint("HELO BRAIN Entering Guns Engange\n");
         mslCheckTimer = 0.0f;
         AddMode(GunsEngageMode);
      }
   }
   // exit 
   else if (targetData->range >= GUN_MAX_RANGE * 8.0f )
   {
//      MonoPrint("HELO BRAIN Exiting Guns Engange\n");
   }
   else // Already in, so stay there
   {
      AddMode(GunsEngageMode);
   }
}

void HeliBrain::GunsEngage(void)
{
	float rng, desHeading;
	float rollLoad;
	float rollDir;
	float pedalLoad;
	float elerr;
	float rz;
	float desSpeed;
	float wpX, wpY, wpZ;
	float alt;
	float ataerror;
    SimVehicleClass *theObject;

   ClearFlag (MslFireFlag | GunFireFlag);
	mslCheckTimer += SimLibMajorFrameTime;

   	if (targetPtr == NULL)
   	{
//	  	MonoPrint("HELO BRAIN Exiting Guns Engange\n");
      	return;
   	}

   	WeaponSelection();
   	if ( !anyWeapons )
   	{
      	if (curMode == GunsEngageMode)
	  	{
       		ClearFlag (MslFireFlag | GunFireFlag);
//			 MonoPrint("HELO BRAIN Exiting Guns Engange\n");
	  	}
      	return;
   	}

    // we're likely dealing with stale data for target, update range and
    // ata
    float xft, yft, zft;
    float rx;

    theObject = (SimVehicleClass *)targetPtr->BaseData();

    xft = theObject->XPos() - self->XPos();
    yft = theObject->YPos() - self->YPos();

	// error for airplanes
	if ( theObject->IsAirplane() && !theObject->OnGround() )
	{
		xft += PRANDFloat() * 2500.0f;
		yft += PRANDFloat() * 2500.0f;
	}

	if (theObject->IsSim() && theObject->OnGround() && theObject->drawPointer )
	{
		Tpoint pos;
		theObject->drawPointer->GetPosition( &pos );
    	zft = pos.z - 20.0f - self->ZPos();
	}
	else
	{
    	zft = theObject->ZPos() - self->ZPos();
	}

    rx = self->dmx[0][0]*xft + self->dmx[0][1]*yft + self->dmx[0][2]*zft;

    targetData->range = (float)sqrt(xft*xft + yft*yft + zft*zft);
    targetData->range = max (targetData->range, 1.0F);
    targetData->ata =  (float)acos(rx/targetData->range);

	// if target is beyond shootable parameters, just track it
    if (targetData->range >= GUN_MAX_RANGE * 4.0f ||
        targetData->ata >= GUN_MAX_ATA * 1.5f )
    {
		trackX = theObject->XPos();
		trackY = theObject->YPos();
		trackZ = theObject->ZPos() - 500.0f;
		AutoTrack( 1.0f );
		return;
    }

	// fire error more strict with range....
	ataerror = (INIT_GUN_VEL - targetData->range)/INIT_GUN_VEL * 80.0f;

	if ( mslCheckTimer > 15.0f )
	{
		WeaponSelection();

		if ( curMissile )
		{
			FireControl();
		}
		mslCheckTimer = 0.0f;
	}
	// should we fire?
	else if ( mslCheckTimer < 4.0f && targetData->ata < ataerror * DTR && targetData->range < INIT_GUN_VEL)
	{
		float tof;
		float az, el;
		mlTrig tha, psi;

		SetFlag(GunFireFlag);
		// MonoPrint ("HELO Digi Firing %8ld   %4d -> %4d\n", SimLibElapsedTime,
		//    self->Id().num_, targetPtr->BaseData()->Id().num_);

      	// Guess TOF
      	tof = targetData->range / 3000.0f;

	  	// now get vector to where we're aiming
      	xft += theObject->XDelta() * tof;
      	yft += theObject->YDelta() * tof;
      	zft += theObject->ZDelta() * tof;

      	// Correct for gravity
      	zft += 0.5F * GRAVITY * tof * tof;

      	az = (float)atan2(yft,xft);
      	el = (float)atan(-zft/(float)sqrt(xft*xft + yft*yft +0.1F));


   		mlSinCos (&tha, el);
   		mlSinCos (&psi, az);

      	self->gunDmx[0][0] = psi.cos*tha.cos;
      	self->gunDmx[0][1] = psi.sin*tha.cos;
      	self->gunDmx[0][2] = -tha.sin;

      	self->gunDmx[1][0] = -psi.sin;
      	self->gunDmx[1][1] = psi.cos;
      	self->gunDmx[1][2] = 0.0f;

      	self->gunDmx[2][0] = psi.cos*tha.sin;
      	self->gunDmx[2][1] = psi.sin*tha.sin;
      	self->gunDmx[2][2] = tha.cos;
	}

	// position of target
   /*-----------------------------------------------------------------*/
   /* Project ahead target leadTof number of bullet times of flight  */
   /*-----------------------------------------------------------------*/
   wpX = targetPtr->BaseData()->XPos() + targetPtr->BaseData()->XDelta() * SimLibMajorFrameTime;
   wpY = targetPtr->BaseData()->YPos() + targetPtr->BaseData()->YDelta() * SimLibMajorFrameTime;
   wpZ = targetPtr->BaseData()->ZPos() + targetPtr->BaseData()->ZDelta() * SimLibMajorFrameTime;

   desSpeed = 0.0f;
   rollDir = 0.0f;
   rollLoad = 0.0f;
   pedalLoad = 0.0f;

	/*---------------------------*/
	/* Range to target
	/*---------------------------*/
	rng = (wpX - self->XPos()) * (wpX - self->XPos()) + (wpY - self->YPos()) *	(wpY - self->YPos());
	rz = wpZ - self->ZPos();

	/*------------------------------------*/
	/* Heading error for current waypoint */
	/*------------------------------------*/
	desHeading = (float)atan2 ( wpY - self->YPos(), wpX - self->XPos()) - self->Yaw();
	if (desHeading > 180.0F * DTR)
		desHeading -= 360.0F * DTR;
	else if (desHeading < -180.0F * DTR)
		desHeading += 360.0F * DTR;

	// rollLoad is normalized (0-1) factor of how far off-heading we are
	// to target
	rollLoad = desHeading / (90.0F * DTR);
	if (rollLoad < 0.0F)
		rollLoad = -rollLoad;
	rollLoad = min( rollLoad, 1.0F );
	if ( desHeading > 0.0f )
		rollDir = 1.0f;
	else
		rollDir = -1.0f;

	//MonoPrint ("%8.2f %8.2f\n", desHeading * RTD, desLoad);

	rng = (float)sqrt( rng );
	if ( rng != 0.0 )
	{
		elerr = (float)atan (rz/rng);
	}
	else
	{
		if ( rz < 0.0 )
			elerr = -90.0f * DTR;
		else
			elerr = 90.0f * DTR;
	}

	// ideally we want to be at about 6000ft away in x and y
	desSpeed = min( 1.0f, (float)fabs( rng - 6000.0f ) * 0.01f );

	if ( desSpeed < 0.2f )
		desSpeed = elerr / MAX_HELI_PITCH;

	// sprintf( debugbuf, "heading=%.3f, rollLoad=%.3f dir=%.3f, desSpeed=%.3f, elerr=%.3f\n",
	// 		 desHeading * RTD, rollLoad, rollDir, desSpeed, elerr );
	// OutputDebugString( debugbuf );

	if ( targetPtr->BaseData()->OnGround() )
	{
		if ( desSpeed > 0.2f && rng < 6000.0f )
		{
			rollLoad = 0.0f;
		}
		alt = wpZ - 200.0f - (rng * 0.1f);
	}
	else
	{
		if ( fabs(desSpeed) > 0.2f && rng < 6000.0f )
		{
			rollLoad = 0.0f;
		}

		if ( rng < 1000.0f )
			alt = wpZ + (1000.0f - rng) * 0.2f;
		else
			alt = wpZ - 100.0f - (rng - 1000.0f) * 0.2f;
	}

	LevelTurn (rollLoad, rollDir, TRUE);
    AltitudeHold(alt);
    // AltitudeHold(wpZ - 100.0 );
	MachHold(desSpeed, 0.0F, FALSE);

}
void HeliBrain::CoarseGunsTrack(float, float, float*)
{
}

void HeliBrain::FineGunsTrack(float, float*)
{
}

float HeliBrain::GunsAutoTrack(float, float, float, float*, float)
{
   return (0.0f);
}

void HeliBrain::FireControl (void)
{
   	SimVehicleClass *theObject;

   	theObject = (SimVehicleClass *)targetPtr->BaseData();

   	if ( targetData->ata > 50.0f * DTR )
   		return;

	// edg: this stuff is all kludged together -- I just can't seem to
	// get AG missiles to hit ground objects, but if they're close enough
	// it looks OK -- probably should be fixed.
	/*
	** Should work now....
   	if ( theObject->OnGround() )
   	{
	   if ( targetData->range < 1000.0f ||
	   		targetData->range > 10000.0f)
	   		return;
   	}
   	else
   	{
   		if ( targetData->range < self->FCC->missileRneMin ||
       		 targetData->range > self->FCC->missileRneMax  )
		   {
			   return;
		   }
   	}

   	if ( targetData->range < self->FCC->missileRneMin ||
       	 targetData->range > self->FCC->missileRneMax  )
	{
	   return;
	}
	*/

   if (self->FCC->GetMasterMode() == FireControlComputer::Missile ||
	   self->FCC->GetMasterMode() == FireControlComputer::AirGroundMissile )
   {
   		if ( targetData->range < 2000.0f ||
   			targetData->range > 15000.0f)
   			return;
   }
   else
   {
   		if ( targetData->range < 1000.0f ||
   			targetData->range > 6000.0f)
   			return;
   }

   	curMissile->SetTarget(targetPtr);
	self->FCC->SetTarget( targetPtr );

	float xft, yft, zft, az, el;

	#ifdef _DEBUG
/*	if ( theObject->IsAirplane() )
		MonoPrint( "HELO BRAIN Firing Missile at Air Unit\n" );
	else if ( theObject->IsHelicopter() )
		MonoPrint( "HELO BRAIN Firing Missile at Helo Unit\n" );
	else if ( theObject->IsGroundVehicle() )
		MonoPrint( "HELO BRAIN Firing Missile at Ground Unit\n" );
*/
	#endif

	// edg: get real az & el.  Since we don't update targets every frame
	// anymore, we should get a current val for more accuracy
	xft = theObject->XPos() - self->XPos();
	yft = theObject->YPos() - self->YPos();
	zft = theObject->ZPos() - self->ZPos();

	// az and el are relative from vehicles orientation so subtract
	// out yaw and pitch
	az = (float)atan2(yft,xft) - self->Yaw();
	el = (float)atan(-zft/(float)sqrt(xft*xft + yft*yft + 0.1F)) - self->Pitch();


	// SetFlag(MslFireFlag);
	if ( self->Sms->curWeapon )
	{
	   if (self->FCC->GetMasterMode() == FireControlComputer::Missile ||
		   self->FCC->GetMasterMode() == FireControlComputer::AirGroundMissile )
	   {
			 // rotate the missile on hardpoint towards target
			 self->Sms->hardPoint[self->Sms->CurHardpoint()]->SetSubRotation(self->Sms->curWpnNum, az, el);

			 if (self->Sms->LaunchMissile())
			 {
				self->SendFireMessage ((SimWeaponClass*)curMissile, FalconWeaponsFire::SRM, TRUE, targetPtr);
			   	//F4SoundFXSetPos( SFX_MISSILE1, 0, self->XPos(), self->YPos(), self->ZPos(), 1.0f , 0 , self->XDelta(),self->YDelta(),self->ZDelta());
			   	//self->SoundPos.Sfx( SFX_MISSILE1 ); // MLR 5/16/2004 - weapons have their own sounds
			 }
	
	   } 
       else //if (self->FCC->GetMasterMode() == FireControlComputer::AirGroundBomb &&
         	//   self->FCC->GetSubMode() == FireControlComputer::RCKT)
	   if (self->FCC->GetMasterMode() == FireControlComputer::AirGroundRocket)
       {
			// rotate the missile on hardpoint towards target
			self->Sms->curWpnNum = 0;
			self->Sms->hardPoint[self->Sms->CurHardpoint()]->SetSubRotation(0, az, el);
		 	if (self->Sms->LaunchRocket())
		 	{
		    	// Play the sound
		    	//F4SoundFXSetPos( SFX_MISSILE3, TRUE, self->XPos(), self->YPos(), self->ZPos(), 1.0f , 0 , self->XDelta(),self->YDelta(),self->ZDelta());
		    	//self->SoundPos.Sfx( SFX_MISSILE3 ); // MLR 5/16/2004 - weapons have their own sounds

		    	// Drop a message
		    	self->SendFireMessage (curMissile, FalconWeaponsFire::Rocket, TRUE, targetPtr);
		 	}
       }
   }
}

char dbg[256];


#if 0

/*
** edg: TODO:  I need to fix this: what's heli's loadout?  Also need
** to select rockets?  I think this can be simplified
*/
void HeliBrain::WeaponSelection (void)
{
MissileClass* theMissile;
float pctRange = 1.0F;
float thisPctRange;
float thisASE;
float thisRmax;
float thisRmin;
int startStation;

   curMissile = NULL;
   curMissileStation = -1;
   curMissileNum     = -1;

   if (targetPtr && self->Sms->FindWeaponClass (SMSClass::AimWpn))
   {
      // Find all weapons w/in parameters
      startStation = self->Sms->curWpnStation;
      do
      {
         if (self->Sms->curWeapon)
         {
            theMissile   = (MissileClass *)(self->Sms->curWeapon);

			thisRmax	 = theMissile->GetRMax(-self->ZPos(), self->Vt(), targetData->az, targetPtr->BaseData()->Vt(), targetData->ataFrom);
			thisRmin	 = 0.1F * thisRmax;		// Shouldn't we call GetRMin???
			thisPctRange = targetData->range / thisRmax;
			thisASE		 = DTR * theMissile->GetASE(-self->ZPos(), self->Vt(), targetData->ataFrom, targetPtr->BaseData()->Vt(), targetData->range);

            if (thisPctRange < pctRange && thisRmin < targetData->range)
            {
			   theMissile->SetTarget( targetPtr );
               pctRange = thisPctRange;
               curMissile = (MissileClass *)(self->Sms->curWeapon);
               curMissileStation = self->Sms->curWpnStation;
               curMissileNum     = self->Sms->curWpnNum;
//	  		   MonoPrint( "HELO BRAIN Missile Selected!\n" );
            }
         }
         self->Sms->FindWeaponClass (SMSClass::AimWpn);
      } while (self->Sms->curWpnStation != startStation);
   }
} 
#endif

void HeliBrain::WeaponSelection (void)
{
	int i;
    SimVehicleClass *target=NULL;
	MissileClass *curAA=NULL, *curAG=NULL, *curRock=NULL;
	int curAAStation=0, curAGStation=0, curRockStation=0;
	int curAANum=0, curAGNum=0, curRockNum=0;
	GunClass *curGun=NULL;

	anyWeapons = FALSE;

	curAA = NULL;
	curAG = NULL;
	curRock = NULL;
	curGun = NULL;

   	curMissile = NULL;
   	curMissileStation = -1;
   	curMissileNum     = -1;

	if ( targetPtr )
    	target = (SimVehicleClass *)targetPtr->BaseData();
	else
    	target = NULL;

	for (i=0; i<self->Sms->NumHardpoints(); i++)
	{
		// Do I have AA Missiles?
		if ( curAA == NULL && self->Sms->hardPoint[i]->GetWeaponClass() == wcAimWpn )
		{
			self->Sms->curWeapon = NULL;
			self->Sms->SetCurHardpoint(-1);
			self->Sms->curWpnNum = 0;
			self->Sms->WeaponStep();
			if (self->Sms->curWeapon)
			{
				MonoPrint( "Helo found AA Missile.  Class = %d\n",
							self->Sms->hardPoint[i]->GetWeaponClass() );

				curAA = (MissileClass *)(self->Sms->curWeapon);
				curAAStation = self->Sms->CurHardpoint();
				curAANum     = self->Sms->curWpnNum;
				if (curAA->launchState != MissileClass::PreLaunch )
				{
					curAA = NULL;
					continue;
				}
			}
		}
		else if ( curAG == NULL && self->Sms->hardPoint[i]->GetWeaponClass() == wcAgmWpn )
		{
			self->Sms->curWeapon = NULL;
			self->Sms->SetCurHardpoint(-1);
			self->Sms->curWpnNum = 0;
			self->Sms->WeaponStep();
			if (self->Sms->curWeapon)
			{
//				MonoPrint( "Helo found AG Missile.  Class = %d\n",
//							self->Sms->hardPoint[i]->GetWeaponClass() );

				curAG = (MissileClass *)(self->Sms->curWeapon);
				curAGStation = self->Sms->CurHardpoint();
				curAGNum     = self->Sms->curWpnNum;
				if (curAG->launchState != MissileClass::PreLaunch )
				{
					curAG = NULL;
					continue;
				}
			}
		}
		/*
		** edg: forget about rockets.   The SMS is fucked up and
		** there's no time to fix since it seems o cause a crash.
		*/
		else if ( curRock == NULL &&
				  (self->Sms->hardPoint[i]->GetWeaponClass() == wcRocketWpn &&
				  self->Sms->hardPoint[i]->weaponCount >= 0 ) )
		{
			self->Sms->curWeapon = NULL;
			self->Sms->SetCurHardpoint(-1);
			self->Sms->curWpnNum = 0;
			self->Sms->WeaponStep();
			if (self->Sms->curWeapon)
			{
//				MonoPrint( "Helo found Rocket.  Class = %d\n",
//							self->Sms->hardPoint[i]->GetWeaponClass() );

				curRock = (MissileClass *)(self->Sms->curWeapon);
				curRockStation = self->Sms->CurHardpoint();
				curRockNum     = self->Sms->curWpnNum;
				if (curRock->launchState != MissileClass::PreLaunch )
				{
					curRock = NULL;
					continue;
				}
			}
		}
	} // hardpoint loop

	// finally look for guns
	if ( self->Guns &&
		 self->Guns->numRoundsRemaining )
	{
		curGun = self->Guns;
	}

	if ( target )
	{
		if ( !target->OnGround() )
		{
			if ( curAA )
			{
				anyWeapons = TRUE;
   			curMissile = curAA;
   			curMissileStation = curAAStation;
   			curMissileNum     = curAANum;
				self->Sms->curWeapon = curAA;
				self->Sms->SetCurHardpoint(curAAStation);
				self->Sms->curWpnNum = curAANum;
				self->FCC->SetMasterMode(FireControlComputer::Missile);
				/*
				if (self->Sms->hardPoint[curMissileStation]->GetWeaponType() == wtAim120 )
					self->FCC->SetSubMode(FireControlComputer::Aim120);
				else
					self->FCC->SetSubMode(FireControlComputer::Aim9);
				*/
				// self->FCC->Exec(targetPtr, self->targetList, self->theInputs);
			}
			else
			{
				self->Sms->curWeapon = NULL;
				self->Sms->SetCurHardpoint(-1);
				self->Sms->curWpnNum = -1;
				self->FCC->SetMasterMode(FireControlComputer::Nav);
   				curMissile = NULL;
   				curMissileStation = -1;
   				curMissileNum     = -1;
				if ( curGun )
					anyWeapons = TRUE;
			}
		}
		else
		{
			if ( curAG )
			{
				anyWeapons = TRUE;
   				curMissile = curAG;
   				curMissileStation = curAGStation;
   				curMissileNum     = curAGNum;
				self->Sms->curWeapon = curAG;
				self->Sms->SetCurHardpoint(curAGStation);
				self->Sms->curWpnNum = curAGNum;
				self->FCC->SetMasterMode (FireControlComputer::AirGroundMissile);
				self->FCC->SetSubMode (FireControlComputer::SLAVE);
			}
			else if ( curRock )
			{
				anyWeapons = TRUE;
   				curMissile = curRock;
   				curMissileStation = curRockStation;
   				curMissileNum     = curRockNum;
				self->Sms->curWeapon = curRock;
				self->Sms->SetCurHardpoint(curRockStation);
				self->Sms->curWpnNum = curRockNum;
				self->FCC->SetMasterMode (FireControlComputer::AirGroundRocket);
				//self->FCC->SetMasterMode (FireControlComputer::AirGroundBomb);
				//self->FCC->SetSubMode (FireControlComputer::RCKT);
			}
			else
			{
				self->Sms->curWeapon = NULL;
				self->Sms->SetCurHardpoint(-1);
				self->Sms->curWpnNum = -1;
				self->FCC->SetMasterMode(FireControlComputer::Nav);
   				curMissile = NULL;
   				curMissileStation = -1;
   				curMissileNum     = -1;
				if ( curGun )
					anyWeapons = TRUE;
			}
		}
	}
	else
	{
		self->Sms->curWeapon = NULL;
		self->Sms->SetCurHardpoint(-1);
		self->Sms->curWpnNum = 0;
		self->FCC->SetMasterMode(FireControlComputer::Nav);
   		curMissile = NULL;
   		curMissileStation = -1;
   		curMissileNum     = -1;
	}

	/*
	if ( curAA || curAG || curRock || curGun )
		anyWeapons = TRUE;
	*/

}
