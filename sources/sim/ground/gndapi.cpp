// GNDAPI, interface to the game code... seperate version needed for game and 
// windows test program.
//
// Note - Confession: Mostly derived from Leon code, with additions and mods by me (Mark)
// 
// Need to change logic so that a forced Next Way point occurs when cover is required
// TARGETING, I need to re-write (okay, modifiy Leon targeting code).
//
// By Mark McCubbin 
//
#include <windows.h>
#include <process.h>
#include <math.h>



#include "stdhdr.h"
#include "ground.h"
#include "mesg.h"
#include "otwdrive.h"
#include "initdata.h"
#include "waypoint.h"
#include "f4error.h"
#include "object.h"
#include "simobj.h"
#include "simdrive.h"
#include "Graphics\Include\drawsgmt.h"
#include "Graphics\Include\drawobj.h"
#include "entity.h"
#include "classtbl.h"
#include "sms.h"
#include "fcc.h"
#include "PilotInputs.h"
#include "MsgInc\DamageMsg.h"
#include "guns.h"
#include "hardpnt.h"
#include "campwp.h"
#include "sfx.h"
#include "Unit.h"
#include "fsound.h"
#include "soundfx.h"
#include "fakerand.h"
#include "gnddef.h"
#include "acmi\src\include\acmirec.h"
#include "gndunit.h"
#include "camp2sim.h"
#include "team.h"

#if 0  // TEST AI code
#include "simveh.h"
#include "gndai.h"
#include "simbase.h"
#endif

#define GNDAI_MAX_DIST		(2000.0F)
#define MIN_DIST			(100.0F)
#define AI_COLLISION_RANGE	(500.0F)
    
  
// Next_WayPoint - get the next way point..
//
// Input:	none.
//
// Output:	WayPoint * = tells whether we have a new way point or not
//
// By Mark McCubbin
// (c) 1997
WayPointClass *GNDAIClass::Next_WayPoint ( void )
{

	float		vel;
	float		distance,
				dx, 
				dy, 
				dz;

	// Used for the campaign movement..
	//
	BattalionClass	*theBattalion;
	WayPoint		waypoint = NULL;		
	int				dir;
	GridIndex		ox, 
					oy, 
					x, 
					y;
   mlTrig trig;

	//MonoPrint("GNDAI: %s, called going for next waypoint\n", RankText[rank] );

	// Determine whether we are follow way points, or a campaign path
	//
	if (self->curWaypoint)
	{

		// First find the location of the current way point
		//
		self->curWaypoint->GetLocation (&dx, &dy, &dz);
		distance = (float) sqrt ( ((self->XPos() - dx) * (self->XPos() - dx)) + 
									((self->YPos() - dy) * (self->YPos() - dy)) );

		// Assume we are at the current way point if we are very close (!)
		//
		if ( distance < MIN_DIST )
		{

			// Get the next way point, if one is available.
			//
			waypoint = self->curWaypoint = self->curWaypoint->GetNextWP ();
			// LRKLUDGE need to make sure there is a waypoint.
			if (self->curWaypoint)
			{
				self->curWaypoint->GetLocation (&dx, &dy, &dz);
				self->SetYPR ( (float)atan2(dy - self->YPos (), dx - self->XPos () ), 0.0F, 0.0F );
				distance = (float) sqrt ( ((self->XPos() - dx) * (self->XPos() - dx)) + 
											((self->YPos() - dy) * (self->YPos() - dy)) );
	
				// Setup the movement for the next frame
				//
				vel = distance / ((self->curWaypoint->GetWPArrivalTime() - SimLibElapsedTime) / SEC_TO_MSEC);
	 		}
			else
	 		{
		   		vel = 0.0F;
		   		self->SetYPR ( 0.0F, 0.0F, 0.0F );
	  		}

			// sfr: no setter on this anymore
			//self->SetVt ( vel );
			//self->SetKias ( vel * FTPSEC_TO_KNOTS);

			// Set the movement deltas
			//
			self->SetDelta (	vel * (float)cos(self->Yaw()),
								vel * (float)sin(self->Yaw()), 0.0F);
			self->SetYPRDelta (0.0F, 0.0F, 0.0F);
		}
	}
	else if ( self->GetCampaignObject() )
	{

		// Needs some changes here!
		// (MCC - UnitMove routines)
		// VP_changes this part should be modified
		theBattalion = (BattalionClass *)self->GetCampaignObject();

		theBattalion->GetLocation ( &ox, &oy);
		self->SetPosition (	self->XPos() + (self->XDelta() * SimLibMajorFrameTime),
								self->YPos() + (self->YDelta() * SimLibMajorFrameTime),
								OTWDriver.GetGroundLevel(self->XPos(), self->YPos() )
								);
		theBattalion->GetLocation ( &x, &y);

		// Check if the vehicle hasn't moved
		// (recalculate the velocity to get some movement
		// - can be speed up)
		//
		if ( (ox != x) || (oy != y) )
		{
			dir = theBattalion->GetNextMoveDirection();
			if (dir >= 0)
			{
				self->SetYPR (45.0F * DTR * dir, 0.0F, 0.0F);
				vel = theBattalion->GetUnitSpeed() * KPH_TO_FPS ;
			}
			else
			{
				self->SetYPR (0.0F, 0.0F, 0.0F);
				vel = 0.0F;
			}
         mlSinCos (&trig, self->Yaw());
			self->SetDelta (vel * trig.cos, vel * trig.sin, 0.0F);
			self->SetYPRDelta (0.0F, 0.0F, 0.0F);
		}
		waypoint = (WayPoint)&waypoint;	// MCC WARNING - this tells the AI code we 
														// are still moving.....!!!!!!!!
	}

	return ( waypoint );
}


// Fire - fire on current target
//
// Input:	none.
//
// Output:	BOOL Yes, NO.
//
// TODO: Change to use local target pointer.
//

#ifdef CHECK_PROC_TIMES
ulong gDoWeapons = 0;
ulong gSelWeapon = 0;
ulong gRotTurr = 0;
ulong gTurrCalc = 0;
ulong gKeepAlive = 0;
ulong gFireTime = 0;
#endif

void GNDAIClass::Fire ( void )
{
	int xtraFireTime;
	ulong nextFire;
	// Do Weapons Control
	// Here we decide if it is time to fire
	if (self->targetPtr){
		// If a unit is hidden, increase its fire time
		if ( self->IsSetLocalFlag( IS_HIDDEN ) )
			xtraFireTime = 10000;
		else
			xtraFireTime = 0;
		
		if (self->targetPtr->BaseData()->OnGround())
			nextFire = nextGroundFire + xtraFireTime;
		else
			nextFire = nextAirFire + xtraFireTime;
		
		// RV - Biker - Chech if we do have any weapons (radar units should not have)
		VehicleClassDataType* vc = GetVehicleClassData(self->Type() - VU_LAST_ENTITY_TYPE);

		int hasWeapons = 0;

		for (int i = 0; i < HARDPOINT_MAX; i++) {
			if (vc->Weapon[i] && vc->Weapons[i]) {
				hasWeapons = 1;
				break;
			}
		}

		// RV - Biker - Radar vehicles shouldn't do this
		if (SimLibElapsedTime > nextFire && !(self->isEmitter && !hasWeapons)) {
		// FRB - The weapns search above seems to break the SAM firing (decreases it or stops it)
		//if (SimLibElapsedTime > nextFire && !(self->isEmitter && !self->Sms->GetCurrentWeapon())) {
			nextTurretCalc = SimLibElapsedTime + TURRET_CALC_RATE;

			if(!self->targetPtr->BaseData()->OnGround())
				self->SelectWeapon(!battalionCommand->self->allowSamFire);
			else
				self->SelectWeapon(FALSE);
			// 2000-10-12 MODIFIED BY S.G. WE NEED TO KNOW IF IT IS A GUN SHOT. IF SO, SHOOT AGAIN VERY SOON
			//if (self->DoWeapons()) {
			//	nextAirFire	= SimLibElapsedTime + airFireRate;	// Shot -- wait a while
			int ret;
			if (ret = self->DoWeapons()) {
				// If it's just TRUE, it's a missile launch, wait 'airFireRate' otherwise wait half a second
				if (ret == TRUE)
					nextAirFire	= SimLibElapsedTime + airFireRate;	// Shot -- wait a while
				else {
					// 2000-10-27 ADDED BY S.G. 
					// SO GUNS CAN HAVE DIFFERENT FIRE RATE... NOT IN RP4, SIMPLY ADD 500 TO MAKE IT LIKE RP4
					//if (ret == TRUE + 1) // It's a slow fire rate
					//	nextAirFire	= SimLibElapsedTime + 2000;
					//else
						nextAirFire	= SimLibElapsedTime + 500;
				}
				// END OF MODIFIED SECTION
			} else {
				if (SimLibElapsedTime > nextFire + 2000) {
					// No shot, exceeded window -- Try again soon
					nextAirFire	= SimLibElapsedTime + 2000;
				}
				else {
					// No shot, still in window -- Try again next frame
				}
			}
			nextGroundFire	= SimLibElapsedTime + gndFireRate;	// Shot or not shot, wait a while
		}
		else
		{
			if ( self->needKeepAlive ){
				self->WeaponKeepAlive();
			}
			// RV - Biker - Radar vehicles without weapons should do this also
			//if(SimLibElapsedTime > nextTurretCalc) {
			if (SimLibElapsedTime > nextTurretCalc || (self->isEmitter && !self->Sms->GetCurrentWeapon())) {
				float xft, yft, zft;
				float realRange, tof;
				FalconEntity	*target;

				SimWeaponClass *theWeapon = self->Sms->GetCurrentWeapon();

				// RV - Biker - Adjust this
				//if(!theWeapon)
				if (!theWeapon && !self->isEmitter)
					return;

				target = self->targetPtr->BaseData();
				if (!target)
					return;

				nextTurretCalc = SimLibElapsedTime + TURRET_CALC_RATE;
				
				xft = target->XPos() - self->XPos();
				yft = target->YPos() - self->YPos();
				zft = target->ZPos() - self->ZPos() + 4.0f;
				realRange = (float)sqrt( xft * xft + yft * yft + zft * zft );

				// RV - Biker - Check if we have a weapon first
				//if(theWeapon->IsGun()) {
				if(theWeapon && theWeapon->IsGun()) {
					GunClass	*Gun = (GunClass*)theWeapon;

					// Guess TOF
					tof = realRange / Gun->initBulletVelocity + 0.25F;

					// now get vector to where we're aiming
					xft += target->XDelta() * tof;
					yft += target->YDelta() * tof;
					zft += target->ZDelta() * tof;

					// Correct for gravity
					zft += 0.5F * GRAVITY * tof * tof;
				}

				// KCK : Copy these values into our targetPtr.
				// Note: This factors our induced error and target's vector into the Rel Geometry data,
				// but as we use this only for aiming and targetting, it shouldn't matter.
				self->targetPtr->localData->az = (float)atan2(yft,xft);
				self->targetPtr->localData->el = (float)atan(-zft/(float)sqrt(xft*xft + yft*yft +0.1F));
				self->targetPtr->localData->range = realRange;
			}
			self->RotateTurret();
		}
	}
	else {
		// Wait until we have a target before we even start counting
		if ( self->needKeepAlive )
			self->WeaponKeepAlive();
		if ( SimLibElapsedTime > nextAirFire + 2000 )
			// We'll wait from 0 to airFireRate to shoot when we get a targetPtr.
			nextAirFire = SimLibElapsedTime + airFireRate;		
		if ( SimLibElapsedTime > nextGroundFire + 2000 )
			// We'll wait from 0 to gndFireRate to shoot when we get a targetPtr.
			nextGroundFire = SimLibElapsedTime + gndFireRate;	
	}
	return;
}

