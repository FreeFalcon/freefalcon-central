#include "stdhdr.h"
#include "helo.h"
#include "missile.h"
#include "misslist.h"
#include "bomb.h"
#include "bombfunc.h"
#include "pilotinputs.h"
#include "fsound.h"
#include "soundfx.h"
#include "simsound.h"
#include "falcmesg.h"
#include "sms.h"
#include "fcc.h"
#include "guns.h"
#include "MsgInc\WeaponFireMsg.h"
#include "campbase.h"
#include "Simdrive.h"
#include "Graphics\Include\drawsgmt.h"
#include "otwdrive.h"
#include "sfx.h"
#include "falcsess.h"
#include "fakerand.h"
#include "camp2sim.h"
#include "object.h"

extern int tgtId;

void HelicopterClass::DoWeapons (void)
{
int fireFlag;
// MissileClass* theMissile;
Tpoint pos, vec;

   // Guns
   fireFlag = fireGun;
   if (Guns)
   {
      Guns->Exec (&fireFlag, gunDmx, platformAngles, targetPtr, FALSE);

      if (fireFlag)
      {
	     //F4SoundFXSetPos( SFX_MCGUN, 0, XPos(), YPos(), ZPos(), 1.0f , 0 , XDelta(),YDelta(),ZDelta());
	     SoundPos.Sfx( SFX_MCGUN );

		pos.x = XPos();
		pos.y = YPos();
		pos.z = ZPos() - 5.0f;

		vec.x = PRANDFloat() * 30.0f;
		vec.y = PRANDFloat() * 30.0f;
		vec.z = -PRANDFloatPos() * 30.0f;

		OTWDriver.AddSfxRequest(
			new SfxClass ( SFX_LIGHT_CLOUD,		// type
			SFX_MOVES,
			&pos,					// world pos
			&vec,					// vel vector
			2.3f,					// time to live
			2.0f ) );				// scale

         if (!IsFiring())
         {
            // KCK: This has been moved to GunClass::Exec, since that's where we generate
            // new bullets
//            SendFireMessage ((SimWeaponClass*)Guns, FalconWeaponsFire::GUN, TRUE, targetPtr);

			#ifdef _DEBUG
			SimVehicleClass *theObject;
			if ( targetPtr )
			{
				theObject = (SimVehicleClass *)targetPtr->BaseData();
/*				if ( theObject->IsAirplane() )
					MonoPrint( "HELO BRAIN Firing Guns at Air Unit\n" );
				else if ( theObject->IsHelicopter() )
					MonoPrint( "HELO BRAIN Firing Guns at Helo Unit\n" );
				else if ( theObject->IsGroundVehicle() )
					MonoPrint( "HELO BRAIN Firing Guns at Ground Unit\n" );
*/			}
			#endif
         }
         SetFiring(TRUE);
      }
      else
      {
         if (IsFiring())
         {
            SendFireMessage ((SimWeaponClass*)Guns, FalconWeaponsFire::GUN, FALSE, targetPtr);
         }
         SetFiring(FALSE);
      }
   }

   /*
   ** edg: no more FCC stuff here.  All done in hdigi now
   if ( FCC->releaseConsent && Sms->curWeapon )
   {
	   if (FCC->GetMasterMode() == FireControlComputer::Missile ||
		   FCC->GetMasterMode() == FireControlComputer::Dogfight ||
		   FCC->GetMasterMode() == FireControlComputer::MissileOverride)
	   {
			 theMissile = (MissileClass *)Sms->curWeapon;
			 if (Sms->LaunchMissile())
			 {
				SendFireMessage ((SimWeaponClass*)theMissile, FalconWeaponsFire::SRM, TRUE, targetPtr);
	
			   	fireMissile = FALSE;
			   	F4SoundFXSetPos( SFX_MISSILE1, 0, XPos(), YPos(), ZPos(), 1.0f );
			 }
	
	   } 
	   else if (FCC->GetMasterMode() == FireControlComputer::AirGroundMissile)
	   {
			 theMissile = (MissileClass *)Sms->curWeapon;
			 if (Sms->LaunchMissile())
			 {
				SendFireMessage ((SimWeaponClass*)theMissile, FalconWeaponsFire::AGM, TRUE, targetPtr);
	
				fireMissile = FALSE;
				F4SoundFXSetPos( SFX_MISSILE2, 0, XPos(), YPos(), ZPos(), 1.0f );
			 }
	
	   } 
      else if (FCC->GetMasterMode() == FireControlComputer::AirGroundBomb &&
         FCC->GetSubMode() == FireControlComputer::RCKT)
      {
         if (FCC->bombPickle && Sms->curWeapon && !OnGround())
         {
            // Play the sound
            F4SoundFXSetPos( SFX_RCKTLOOP, TRUE, XPos(), YPos(), ZPos(), 1.0f );

			theMissile = (MissileClass *)Sms->curWeapon;

            if (Sms->LaunchRocket())
            {

               // Stop firing
               FCC->bombPickle = FALSE;
               FCC->postDrop = TRUE;

               // Play the sound
               F4SoundFXSetPos( SFX_MISSILE3, TRUE, XPos(), YPos(), ZPos(), 1.0f );

               // Drop a message
               SendFireMessage (theMissile, FalconWeaponsFire::Rocket, TRUE, targetPtr);
            }
         }
      }
   }
   */

   /*
   else if (FCC->GetMasterMode() == FireControlComputer::AirGroundBomb)
   {
      curWeapon = Sms->curWeapon;
      if (FCC->bombPickle)
      {
         SendFireMessage ((SimWeaponClass*)curWeapon, FalconWeaponsFire::BMB, TRUE, targetPtr);
         if (Sms->DropBomb())
         {
            FCC->bombPickle = FALSE;
            FCC->postDrop = TRUE;
         }
      }
   }
   */

   // Handle missiles launched but still on the rail
	Sms->Exec();
}
