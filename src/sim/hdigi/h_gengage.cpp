#include "stdhdr.h"
#include "hdigi.h"
#include "simveh.h"
#include "object.h"
#include "airframe.h"
#include "missile.h"
#include "fcc.h"
#include "sms.h"
#include "Graphics/Include/drawobj.h"
#include "camp2sim.h"
#include "hardpnt.h"
#include "campbase.h"
#include "fakerand.h"
#include "guns.h"
#include "MsgInc/WeaponFireMsg.h"
#include "fsound.h"
#include "soundfx.h"
#include "unit.h"

#define INIT_GUN_VEL   7000.0F
#define GUN_MAX_RANGE  8000.0F
#define GUN_MAX_ATA    (90.0F * DTR)
//char debugbuf[256];

void HeliBrain::GunsEngageCheck(void)
{
    SimVehicleClass *theObject;

    // OutputDebugString("Entering Guns Engange Check\n");
    ClearFlag(MslFireFlag bitor GunFireFlag);

    // no target
    if ( not targetPtr)
    {
        if (curMode == GunsEngageMode)
        {
            ClearFlag(MslFireFlag bitor GunFireFlag);
            //MonoPrint("HELO BRAIN Exiting Guns Engange\n");
        }

        return;
    }

    // RV - Biker - Abbort if damaged
    if (self->pctStrength < 1.0)
    {
        if (curMode == GunsEngageMode)
        {
            ClearFlag(MslFireFlag bitor GunFireFlag);
            SelectNextWaypoint();
        }

        return;
    }

    WeaponSelection();

    // RV - Biker - Check if we do have AG weapons
    int hasAgWeapons = 0;
    int hasAaWeapons = 0;

    if (self->Guns)
    {
        hasAgWeapons = hasAgWeapons + self->Guns->numRoundsRemaining;
    }

    for (int i = 1; i < self->Sms->NumHardpoints(); i++)
    {
        if (self->Sms->hardPoint[i]->GetWeaponType() not_eq wtAim9 and self->Sms->hardPoint[i]->GetWeaponType() not_eq wtAim120)
            hasAgWeapons = hasAgWeapons + self->Sms->hardPoint[i]->weaponCount;
        else
            hasAaWeapons = hasAaWeapons + self->Sms->hardPoint[i]->weaponCount;
    }

    // we're likely dealing with stale data for target, update range and ata
    float xft, yft, zft;
    float rx;
    theObject = (SimVehicleClass *)targetPtr->BaseData();

    xft = theObject->XPos() - self->XPos();
    yft = theObject->YPos() - self->YPos();
    zft = theObject->ZPos() - self->ZPos();

    if ( not theObject->OnGround() and hasAaWeapons == 0)
    {
        if (curMode == GunsEngageMode)
        {
            if (mslCheckTimer > 5.0f)
            {
                ClearFlag(MslFireFlag bitor GunFireFlag);

                if (hasAgWeapons == 0)
                    SelectNextWaypoint();

                return;
            }
        }
        else
        {
            return;
        }
    }

    if (theObject->OnGround() and hasAgWeapons == 0)
    {
        if (curMode == GunsEngageMode)
        {
            if (mslCheckTimer > 5.0f)
            {
                ClearFlag(MslFireFlag bitor GunFireFlag);

                if (hasAaWeapons == 0)
                    SelectNextWaypoint();

                return;
            }
        }
        else
        {
            return;
        }
    }

    // RV - Biker - Don't attack AC or chopper which are 5000 ft above us or are above 10000 ft in general
    if ( not theObject->OnGround())
    {
        if (theObject->ZPos() < -10000.0f or zft < -5000.0f)
        {
            targetPtr->Release();
            targetPtr = NULL;
            targetData = NULL;

            if (curMode == GunsEngageMode)
            {
                ClearFlag(MslFireFlag bitor GunFireFlag);
            }

            return;
        }
        else
        {
            // Never discard this target
            self->nextTargetUpdate = SimLibElapsedTime + 5;
        }
    }



    rx = self->dmx[0][0] * xft + self->dmx[0][1] * yft + self->dmx[0][2] * zft;

    targetData->range = (float)sqrt(xft * xft + yft * yft + zft * zft);
    targetData->range = max(targetData->range, 1.0F);
    // targetData->ata =  (float)acos(rx/targetData->range);

    // entry
    if (curMode not_eq GunsEngageMode)
    {
        if (targetData->range <= GUN_MAX_RANGE * 8.0f)
        {
            // MonoPrint("HELO BRAIN Entering Guns Engange\n");
            mslCheckTimer = 0.0f;
            AddMode(GunsEngageMode);
        }
    }

    // exit
    else if (targetData->range >= GUN_MAX_RANGE * 8.0f)
    {
        // MonoPrint("HELO BRAIN Exiting Guns Engange\n");
    }
    // Already in, so stay there
    else
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

    ClearFlag(MslFireFlag bitor GunFireFlag);
    mslCheckTimer += SimLibMajorFrameTime;

    if (targetPtr == NULL)
    {
        return;
    }

    if (self->OnGround())
    {
        self->UnSetFlag(ON_GROUND);
    }

    WeaponSelection();

    if (self->pctStrength < 1.0)
    {
        return;
    }

    // we're likely dealing with stale data for target, update range and ata
    float xft, yft, zft;
    float rx;

    theObject = (SimVehicleClass *)targetPtr->BaseData();

    xft = theObject->XPos() - self->XPos();
    yft = theObject->YPos() - self->YPos();

    // error for airplanes
    if (theObject->IsAirplane() and not theObject->OnGround())
    {
        xft += PRANDFloat() * 2500.0f;
        yft += PRANDFloat() * 2500.0f;
    }

    if (theObject->IsSim() and theObject->OnGround() and theObject->drawPointer)
    {
        Tpoint pos;
        theObject->drawPointer->GetPosition(&pos);
        zft = pos.z - 20.0f - self->ZPos();
    }
    else
    {
        zft = theObject->ZPos() - self->ZPos();
    }

    rx = self->dmx[0][0] * xft + self->dmx[0][1] * yft + self->dmx[0][2] * zft;

    targetData->range = (float)sqrt(xft * xft + yft * yft + zft * zft);
    targetData->range = max(targetData->range, 1.0F);
    targetData->ata = (float)acos(rx / targetData->range);

    // if target is beyond shootable parameters, just track it
    if (targetData->range >= GUN_MAX_RANGE * 4.0f or
        targetData->ata >= GUN_MAX_ATA * 1.5f)
    {
        trackX = theObject->XPos();
        trackY = theObject->YPos();
        trackZ = theObject->ZPos() - 500.0f;
        AutoTrack(1.0f);
        return;
    }

    // fire error more strict with range....
    // 2001-07-05 MODIFIED BY S.G. ataerror is an angle
    //ataerror = (INIT_GUN_VEL - targetData->range)/INIT_GUN_VEL * 80.0f;
    if (targetData->range < INIT_GUN_VEL)
        ataerror = (float)acos((double)(INIT_GUN_VEL - targetData->range) / INIT_GUN_VEL);

    // 2001-07-06 MODIDIED BY S.G. WE DON'T WANT THE ROCKETS TO FIRE ONE PER 15 SECONDS LAUNCH AT FRAME SPEED (JUST LIKE FOR PLANES)
    //if ( mslCheckTimer > 15.0f )
    //if ( mslCheckTimer > 15.0f or (mslCheckTimer > 0.25f and self->FCC->GetMasterMode() == FireControlComputer::AirGroundBomb and self->FCC->GetSubMode() == FireControlComputer::RCKT))
    if (mslCheckTimer > 15.0f or (mslCheckTimer > 0.25f and self->FCC->GetMasterMode() == FireControlComputer::AirGroundRocket))
    {
        WeaponSelection();

        if (curMissile)
        {
            FireControl();
        }

        mslCheckTimer = 0.0f;
    }
    // should we fire?
    // 2001-07-05 MODIFIED BY S.G. CHANGE THE ORDER
    //else if ( mslCheckTimer < 4.0f and targetData->ata < ataerror * DTR and targetData->range < INIT_GUN_VEL)
    else if (mslCheckTimer < 4.0f and targetData->range < INIT_GUN_VEL and targetData->ata < ataerror)
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
        //zft += 0.5F * GRAVITY * tof * tof;
        // RV - Biker - Do it the right way
        zft -= 0.5F * GRAVITY * tof * tof;

        az = (float)atan2(yft, xft);
        el = (float)atan(-zft / (float)sqrt(xft * xft + yft * yft + 0.1F));

        mlSinCos(&tha, el);
        mlSinCos(&psi, az);

        self->gunDmx[0][0] = psi.cos * tha.cos;
        self->gunDmx[0][1] = psi.sin * tha.cos;
        self->gunDmx[0][2] = -tha.sin;

        self->gunDmx[1][0] = -psi.sin;
        self->gunDmx[1][1] = psi.cos;
        self->gunDmx[1][2] = 0.0f;

        self->gunDmx[2][0] = psi.cos * tha.sin;
        self->gunDmx[2][1] = psi.sin * tha.sin;
        self->gunDmx[2][2] = tha.cos;
    }

    // position of target
    // Project ahead target leadTof number of bullet times of flight
    wpX = targetPtr->BaseData()->XPos() + targetPtr->BaseData()->XDelta() * SimLibMajorFrameTime;
    wpY = targetPtr->BaseData()->YPos() + targetPtr->BaseData()->YDelta() * SimLibMajorFrameTime;
    wpZ = targetPtr->BaseData()->ZPos() + targetPtr->BaseData()->ZDelta() * SimLibMajorFrameTime;

    desSpeed = 0.0f;
    rollDir = 0.0f;
    rollLoad = 0.0f;
    pedalLoad = 0.0f;

    // Range to target
    rng = (wpX - self->XPos()) * (wpX - self->XPos()) + (wpY - self->YPos()) * (wpY - self->YPos());
    rz = wpZ - self->ZPos();

    // Heading error for current waypoint
    desHeading = (float)atan2(wpY - self->YPos(), wpX - self->XPos()) - self->Yaw();

    if (desHeading > 180.0F * DTR)
        desHeading -= 360.0F * DTR;
    else if (desHeading < -180.0F * DTR)
        desHeading += 360.0F * DTR;

    // rollLoad is normalized (0-1) factor of how far off-heading we are to target
    rollLoad = desHeading / (90.0F * DTR);

    if (rollLoad < 0.0F)
        rollLoad = -rollLoad;

    rollLoad = min(rollLoad, 1.0F);

    if (desHeading > 0.0f)
        rollDir = 1.0f;
    else
        rollDir = -1.0f;

    //MonoPrint ("%8.2f %8.2f\n", desHeading * RTD, desLoad);

    rng = (float)sqrt(rng);

    if (rng not_eq 0.0f)
    {
        elerr = (float)atan(rz / rng);
    }
    else
    {
        if (rz < 0.0)
            elerr = -90.0f * DTR;
        else
            elerr = 90.0f * DTR;
    }

    // 2001-07-06 MODIFIED BY S.G. SO CHOPPERS STOPS WHEN IN FIRING PARAMETER
    // ideally we want to be at about 6000ft away in x and y
    //desSpeed = min( 1.0f, (float)fabs( rng - 6000.0f ) * 0.01f );
    //
    //if ( desSpeed < 0.2f )
    // desSpeed = elerr / MAX_HELI_PITCH;

    // Defaults to full speed ahead
    desSpeed = 1.0f;

    // Ground missiles
    // TJL 11/15/03 Hellfires/AGMs between 3 to 5 miles
    if (self->FCC->GetMasterMode() == FireControlComputer::Missile or self->FCC->GetMasterMode() == FireControlComputer::AirGroundMissile)
    {
        //if ( targetData->range >= 2000.0f and targetData->range <= 12000.0F)
        if (targetData->range >= 12000.0f and targetData->range <= 24000.0F)
            desSpeed = 0.0f;
    }
    // Rockets
    //TJL 11/15/03 Rockets have max range of 3 miles
    else //if ( self->FCC->GetMasterMode() == FireControlComputer::AirGroundBomb and self->FCC->GetSubMode() == FireControlComputer::RCKT ) {
        if (self->FCC->GetMasterMode() == FireControlComputer::AirGroundRocket)    // MLR 4/3/2004 -
        {
            //if ( targetData->range >= 1000.0f and targetData->range <= 10000.0F)
            if (targetData->range >= 6000.0f and targetData->range <= 15000.0F)
                desSpeed = 0.0f;
        }

    //sprintf( debugbuf, "heading=%.3f, rollLoad=%.3f dir=%.3f, desSpeed=%.3f, elerr=%.3f\n",  desHeading * RTD, rollLoad, rollDir, desSpeed, elerr );
    //OutputDebugString( debugbuf );

    if (targetPtr->BaseData()->OnGround())
    {
        if (desSpeed > 0.2f and rng < 6000.0f)
        {
            rollLoad = 0.0f;
        }

        alt = wpZ - 200.0f - (rng * 0.075f);
    }
    else
    {
        if (fabs(desSpeed) > 0.2f and rng < 6000.0f)
        {
            rollLoad = 0.0f;
        }

        if (rng < 1000.0f)
            alt = wpZ + (1000.0f - rng) * 0.2f;
        else
            alt = wpZ - 100.0f - (rng - 1000.0f) * 0.2f;
    }

    LevelTurn(rollLoad, rollDir, TRUE);
    MachHold(desSpeed, min(max(300.0f, -alt), 3500.0f), TRUE);
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

void HeliBrain::FireControl(void)
{
    SimVehicleClass *theObject;

    theObject = (SimVehicleClass *)targetPtr->BaseData();

    if (targetData->ata > 50.0f * DTR)
        return;

    // edg: this stuff is all kludged together -- I just can't seem to
    // get AG missiles to hit ground objects, but if they're close enough
    // it looks OK -- probably should be fixed.
    /*
    ** Should work now....
    if ( theObject->OnGround() )
    {
       if ( targetData->range < 1000.0f or
        targetData->range > 10000.0f)
        return;
    }
    else
    {
     if ( targetData->range < self->FCC->missileRneMin or
         targetData->range > self->FCC->missileRneMax  )
        {
        return;
        }
    }

    if ( targetData->range < self->FCC->missileRneMin or
         targetData->range > self->FCC->missileRneMax  )
    {
       return;
    }
    */

    if (self->FCC->GetMasterMode() == FireControlComputer::Missile or
        self->FCC->GetMasterMode() == FireControlComputer::AirGroundMissile)
    {
        if (targetData->range < 2000.0f or targetData->range > maxWpnRange)
            return;
    }
    // 2001-07-06 ADDED BY S.G. SO ROCKETS WORKS
    else //if ( self->FCC->GetMasterMode() == FireControlComputer::AirGroundBomb and 

        //  self->FCC->GetSubMode() == FireControlComputer::RCKT )
        if (self->FCC->GetMasterMode() == FireControlComputer::AirGroundRocket) // MLR 4/3/2004 -
        {
            if (targetData->range < 1000.0f or targetData->range > maxWpnRange)
                return;
        }
    // END OF ADDED SECTION
        else
        {
            if (targetData->range < 1000.0f or targetData->range > 6000.0f)
                return;
        }

    curMissile->SetTarget(targetPtr);
    self->FCC->SetTarget(targetPtr);

    float xft, yft, zft, az, el;

#ifdef DEBUG

    if (theObject->IsAirplane())
        MonoPrint("HELO BRAIN Firing Missile at Air Unit\n");
    else if (theObject->IsHelicopter())
        MonoPrint("HELO BRAIN Firing Missile at Helo Unit\n");
    else if (theObject->IsGroundVehicle())
        MonoPrint("HELO BRAIN Firing Missile at Ground Unit\n");
    else
    {
        MonoPrint("HELO BRAIN Firing Missile at unknown target\n");

        if (theObject->IsAirplane())
            MonoPrint("Airplane");

        if (theObject->IsAwake())
            MonoPrint("Awake");

        if (theObject->IsBattalion())
            MonoPrint("Battalion");

        if (theObject->IsBomb())
            MonoPrint("Bomb");

        if (theObject->IsBrigade())
            MonoPrint("Brigade");

        if (theObject->IsCamera())
            MonoPrint("Camera");

        if (theObject->IsCampaign())
            MonoPrint("CampaignObject");

        if (theObject->IsCollidable())
            MonoPrint("Collidable");

        if (theObject->IsDead())
            MonoPrint("Dead");

        if (theObject->IsDying())
            MonoPrint("Dying");

        if (theObject->IsEmitting())
            MonoPrint("Emitting");

        if (theObject->IsExploding())
            MonoPrint("Exploding");

        if (theObject->IsFiring())
            MonoPrint("Firing");

        if (theObject->IsFlight())
            MonoPrint("Flight");

        if (theObject->IsGroup())
            MonoPrint("Group");

        if (theObject->IsAreaJamming())
            MonoPrint("AreaJamming");

        if (theObject->IsLocal())
            MonoPrint("Local");

        if (theObject->IsMissile())
            MonoPrint("Missile");

        if (theObject->IsMover())
            MonoPrint("Mover");

        if (theObject->IsPackage())
            MonoPrint("Package");

        if (theObject->IsPersistant())
            MonoPrint("Persistant");

        if (theObject->IsPersistent())
            MonoPrint("Persistent");

        if (theObject->IsPlayer())
            MonoPrint("Player");

        if (theObject->IsPrivate())
            MonoPrint("Private");

        if (theObject->IsSim())
            MonoPrint("SimObject");

        if (theObject->IsSquadron())
            MonoPrint("Squadron");

        if (theObject->IsUnit())
            MonoPrint("Unit");

        if (theObject->IsVehicle())
            MonoPrint("Vehicle");

        if (theObject->IsWeapon())
            MonoPrint("Weapon");

        if (theObject->OnGround())
            MonoPrint("OnGround");

        int g = ((CampBaseClass*)theObject)->NumberOfComponents();
        MonoPrint("Number Of Elements: %d targetPtr: %08x BaseDataPointer: %08x", g, targetPtr, theObject);
    }

    if (theObject->IsSim())
    {
        Falcon4EntityClassType *classPtr = (Falcon4EntityClassType*)theObject->EntityType();
        //MonoPrint("Name: %s",((VehicleClassDataType*)(classPtr->dataPtr))->Name);
    }

#endif
    // edg: get real az bitand el.  Since we don't update targets every frame
    // anymore, we should get a current val for more accuracy
    xft = theObject->XPos() - self->XPos();
    yft = theObject->YPos() - self->YPos();
    zft = theObject->ZPos() - self->ZPos();

    // az and el are relative from vehicles orientation so subtract
    // out yaw and pitch
    az = (float)atan2(yft, xft) - self->Yaw();
    el = (float)atan(-zft / (float)sqrt(xft * xft + yft * yft + 0.1F)) - self->Pitch();

    self->Sms->SetMasterArm(SMSBaseClass::Arm); // M.N. This prevented helicopters from firing their missiles

    if (curMissile->targetPtr)
        CalcRelGeom(self, curMissile->targetPtr, NULL, 1.0F / SimLibMajorFrameTime);

    // SetFlag(MslFireFlag);
    if (self->Sms->curWeapon)
    {
        if (self->FCC->GetMasterMode() == FireControlComputer::Missile or
            self->FCC->GetMasterMode() == FireControlComputer::AirGroundMissile)
        {
            // rotate the missile on hardpoint towards target
            self->Sms->hardPoint[self->Sms->CurHardpoint()]->SetSubRotation(self->Sms->curWpnNum, az, el);

            if (self->Sms->LaunchMissile())
            {
                self->SendFireMessage((SimWeaponClass*)curMissile, FalconWeaponsFire::SRM, TRUE, targetPtr);
                // MLR 1/25/2004 - we'll just let the missile engine sound suffice
                //F4SoundFXSetPos( SFX_MISSILE1, 0, self->XPos(), self->YPos(), self->ZPos(), 1.0f , 0 , self->XDelta(),self->YDelta(),self->ZDelta());
            }
        }
        else //if (self->FCC->GetMasterMode() == FireControlComputer::AirGroundBomb and 

            //  self->FCC->GetSubMode() == FireControlComputer::RCKT)
            if (self->FCC->GetMasterMode() == FireControlComputer::AirGroundRocket) // MLR 4/3/2004 -
            {
                // rotate the missile on hardpoint towards target
                self->Sms->curWpnNum = 0;

                // This just causes the rockets to fly all over the place
                //self->Sms->hardPoint[self->Sms->CurHardpoint()]->SetSubRotation(0, az, el);

                if (self->Sms->LaunchRocket())
                {
                    // Play the sound
                    // MLR 1/25/2004 - we'll just let the missile engine sound suffice
                    //F4SoundFXSetPos( SFX_MISSILE3, TRUE, self->XPos(), self->YPos(), self->ZPos(), 1.0f , 0 , self->XDelta(),self->YDelta(),self->ZDelta());

                    // Drop a message
                    self->SendFireMessage(curMissile, FalconWeaponsFire::Rocket, TRUE, targetPtr);
                }
            }
    }
}

//char dbg[256];
//
//
//#if 0
//
///*
//** edg: TODO:  I need to fix this: what's heli's loadout?  Also need
//** to select rockets?  I think this can be simplified
//*/
//void HeliBrain::WeaponSelection (void)
//{
//MissileClass* theMissile;
//float pctRange = 1.0F;
//float thisPctRange;
//float thisASE;
//float thisRmax;
//float thisRmin;
//int startStation;
//
//   curMissile = NULL;
//   curMissileStation = -1;
//   curMissileNum     = -1;
//
//   if (targetPtr and self->Sms->FindWeaponClass (SMSClass::AimWpn))
//   {
//      // Find all weapons w/in parameters
//      startStation = self->Sms->curWpnStation;
//      do
//      {
//         if (self->Sms->curWeapon)
//         {
//            theMissile   = (MissileClass *)(self->Sms->curWeapon);
//
// thisRmax  = theMissile->GetRMax(-self->ZPos(), self->GetVt(), targetData->az, targetPtr->BaseData()->GetVt(), targetData->ataFrom);
// thisRmin  = 0.1F * thisRmax; // Shouldn't we call GetRMin???
// thisPctRange = targetData->range / thisRmax;
// thisASE  = DTR * theMissile->GetASE(-self->ZPos(), self->GetVt(), targetData->ataFrom, targetPtr->BaseData()->GetVt(), targetData->range);
//
//            if (thisPctRange < pctRange and thisRmin < targetData->range)
//            {
//    theMissile->SetTarget( targetPtr );
//               pctRange = thisPctRange;
//               curMissile = (MissileClass *)(self->Sms->curWeapon);
//               curMissileStation = self->Sms->curWpnStation;
//               curMissileNum     = self->Sms->curWpnNum;
////       MonoPrint( "HELO BRAIN Missile Selected\n" );
//            }
//         }
//         self->Sms->FindWeaponClass (SMSClass::AimWpn);
//      } while (self->Sms->curWpnStation not_eq startStation);
//   }
//}
//#endif
//
//#if 0
//
//void HeliBrain::WeaponSelection (void)
//{
// int i;
//    SimVehicleClass *target=NULL;
// MissileClass *curAA=NULL, *curAG=NULL, *curRock=NULL, *theMissile=NULL;
// int curAAStation=0, curAGStation=0, curRockStation=0;
// int curAANum=0, curAGNum=0, curRockNum=0;
// GunClass *curGun=NULL;
// float rmax;
//
// anyWeapons = FALSE;
//
// curAA = NULL;
// curAG = NULL;
// curRock = NULL;
// curGun = NULL;
//
//    curMissile = NULL;
//    curMissileStation = -1;
//    curMissileNum     = -1;
//
// if ( targetPtr )
//     target = (SimVehicleClass *)targetPtr->BaseData();
// else
//     target = NULL;
//
// for (i=0; i<self->Sms->NumHardpoints(); i++)
// {
// // Do I have AA Missiles?
// if ( curAA == NULL and self->Sms->hardPoint[i]->GetWeaponClass() == wcAimWpn )
// {
// self->Sms->curWeapon = NULL;
//// 2001-07-05 MODIFIED BY S.G. SO THEY ACTUALLY CHOOSE A WEAPON
//// self->Sms->SetCurHardpoint(-1);
//// self->Sms->curWpnNum = 0;
// self->Sms->SetCurHardpoint(i);
// self->Sms->curWpnNum = -1;
//
// self->Sms->WeaponStep();
// if (self->Sms->curWeapon)
// {
// //MonoPrint( "Helo found AA Missile.  Class = %d\n",
// self->Sms->hardPoint[i]->GetWeaponClass() );
//
// curAA = (MissileClass *)(self->Sms->curWeapon);
// curAAStation = self->Sms->CurHardpoint();
// curAANum     = self->Sms->curWpnNum;
// if (curAA->launchState not_eq MissileClass::PreLaunch )
// {
// curAA = NULL;
// continue;
// }
// }
// }
// else if ( curAG == NULL and self->Sms->hardPoint[i]->GetWeaponClass() == wcAgmWpn )
// {
// self->Sms->curWeapon = NULL;
//// 2001-07-05 MODIFIED BY S.G. SO THEY ACTUALLY CHOOSE A WEAPON
//// self->Sms->SetCurHardpoint(-1);
//// self->Sms->curWpnNum = 0;
// self->Sms->SetCurHardpoint(i);
// self->Sms->curWpnNum = -1;
//
// self->Sms->WeaponStep();
// if (self->Sms->curWeapon)
// {
//// MonoPrint( "Helo found AG Missile.  Class = %d\n",
//// self->Sms->hardPoint[i]->GetWeaponClass() );
//
// curAG = (MissileClass *)(self->Sms->curWeapon);
// curAGStation = self->Sms->CurHardpoint();
// curAGNum     = self->Sms->curWpnNum;
// if (curAG->launchState not_eq MissileClass::PreLaunch )
// {
// curAG = NULL;
// continue;
// }
// }
// }
// /*
// ** edg: forget about rockets.   The SMS is fucked up and
// ** there's no time to fix since it seems o cause a crash.
// */
// else if ( curRock == NULL and 
//   (self->Sms->hardPoint[i]->GetWeaponClass() == wcRocketWpn and 
//   self->Sms->hardPoint[i]->weaponCount >= 0 ) )
// {
// self->Sms->curWeapon = NULL;
//// 2001-07-05 MODIFIED BY S.G. SO THEY ACTUALLY CHOOSE A WEAPON
//// self->Sms->SetCurHardpoint(-1);
//// self->Sms->curWpnNum = 0;
// self->Sms->SetCurHardpoint(i);
// self->Sms->curWpnNum = -1;
//
// self->Sms->WeaponStep();
// if (self->Sms->curWeapon)
// {
//// MonoPrint( "Helo found Rocket.  Class = %d\n",
//// self->Sms->hardPoint[i]->GetWeaponClass() );
//
// curRock = (MissileClass *)(self->Sms->curWeapon);
//
// curRockStation = self->Sms->CurHardpoint();
// curRockNum     = self->Sms->curWpnNum;
// if (curRock->launchState not_eq MissileClass::PreLaunch )
// {
// curRock = NULL;
// continue;
// }
// }
// }
// } // hardpoint loop
//
//
// maxWpnRange = 6000.0F;
// if (curRock)
// theMissile = curRock;
// else if (curAA)
// theMissile = curAA;
// else if (curAG)
// theMissile = curAG;
//
// if ( theMissile )
// {
// // get maximum range
// rmax = theMissile->GetRMax(-self->ZPos(),
// self->GetVt(),
// targetData->az,
//      targetPtr->BaseData()->GetVt(),
// targetData->ataFrom);
//
// // set our max weapon range
// if ( rmax > maxWpnRange )
// maxWpnRange = rmax;
// }
//
//
// // finally look for guns
// if ( self->Guns and 
//  self->Guns->numRoundsRemaining )
// {
// curGun = self->Guns;
// }
//
// if ( target )
// {
// if ( not target->OnGround() )
// {
// if ( curAA )
// {
// anyWeapons = TRUE;
//    curMissile = curAA;
//    curMissileStation = curAAStation;
//    curMissileNum     = curAANum;
// self->Sms->curWeapon = curAA;
// self->Sms->SetCurHardpoint(curAAStation);
// self->Sms->curWpnNum = curAANum;
// self->FCC->SetMasterMode(FireControlComputer::Missile);
// /*
// if (self->Sms->hardPoint[curMissileStation]->GetWeaponType() == wtAim120 )
// self->FCC->SetSubMode(FireControlComputer::Aim120);
// else
// self->FCC->SetSubMode(FireControlComputer::Aim9);
// */
// // self->FCC->Exec(targetPtr, self->targetList, self->theInputs);
// }
// else
// {
// self->Sms->curWeapon = NULL;
// self->Sms->SetCurHardpoint(-1);
// self->Sms->curWpnNum = -1;
// self->FCC->SetMasterMode(FireControlComputer::Nav);
//    curMissile = NULL;
//    curMissileStation = -1;
//    curMissileNum     = -1;
// if ( curGun )
// anyWeapons = TRUE;
// }
// }
// else
// {
// if ( curAG )
// {
// anyWeapons = TRUE;
//    curMissile = curAG;
//    curMissileStation = curAGStation;
//    curMissileNum     = curAGNum;
// self->Sms->curWeapon = curAG;
// self->Sms->SetCurHardpoint(curAGStation);
// self->Sms->curWpnNum = curAGNum;
// self->FCC->SetMasterMode (FireControlComputer::AirGroundMissile);
// self->FCC->SetSubMode (FireControlComputer::SLAVE);
// }
// else if ( curRock )
// {
// anyWeapons = TRUE;
//    curMissile = curRock;
//    curMissileStation = curRockStation;
//    curMissileNum     = curRockNum;
// self->Sms->curWeapon = curRock;
// self->Sms->SetCurHardpoint(curRockStation);
// self->Sms->curWpnNum = curRockNum;
// self->FCC->SetMasterMode (FireControlComputer::AirGroundRocket); // MLR 4/3/2004 -
// //self->FCC->SetMasterMode (FireControlComputer::AirGroundBomb);
// //self->FCC->SetSubMode (FireControlComputer::RCKT);
// }
// else
// {
// self->Sms->curWeapon = NULL;
// self->Sms->SetCurHardpoint(-1);
// self->Sms->curWpnNum = -1;
// self->FCC->SetMasterMode(FireControlComputer::Nav);
//    curMissile = NULL;
//    curMissileStation = -1;
//    curMissileNum     = -1;
// if ( curGun )
// anyWeapons = TRUE;
// }
// }
// }
// else
// {
// self->Sms->curWeapon = NULL;
// self->Sms->SetCurHardpoint(-1);
// self->Sms->curWpnNum = 0;
// self->FCC->SetMasterMode(FireControlComputer::Nav);
//    curMissile = NULL;
//    curMissileStation = -1;
//    curMissileNum     = -1;
// }
//
// /*
// if ( curAA or curAG or curRock or curGun )
// anyWeapons = TRUE;
// */
//
//}
//
//#endif

// MLR 4/23/2004 - Mike was here
void HeliBrain::WeaponSelection(void)
{
    SimVehicleClass *target = NULL;

    if (targetPtr)
    {
        target = (SimVehicleClass *)targetPtr->BaseData();

        if (target->OnGround())
        {
            // would be nice to pick weapons based on what the target is
            if ( not self->Sms->FindWeaponClass(wcAgmWpn))
                self->Sms->FindWeaponClass(wcRocketWpn);

            // self->Sms->FindWeaponClass(wcGunWpn);
        }
        else
        {
            // airborne
            self->Sms->FindWeaponClass(wcAimWpn);
            //self->Sms->FindWeaponClass(wcGunWpn);
        }

        maxWpnRange = 6000;

        if (self->Sms->curWeapon)
        {
            anyWeapons = TRUE;
            curMissile = self->Sms->GetCurrentWeapon();
            curMissileStation = self->Sms->GetCurrentWeaponHardpoint();
            curMissileNum = 0; // wtf, who cares?

            if (target->OnGround())
                self->FCC->SetAGMasterModeForCurrentWeapon();
            else
                self->FCC->SetAAMasterModeForCurrentWeapon();

            if (self->Sms->curWeapon->IsMissile())
            {
                maxWpnRange = ((MissileClass *)curMissile)->GetRMax(-self->ZPos(),
                              self->GetVt(),
                              targetData->az,
                              targetPtr->BaseData()->GetVt(),
                              targetData->ataFrom);
            }
            else
            {
                if (self->Sms->curWeapon->IsLauncher())
                {
                    // the FCC has a rocket missile pointer, we need access to it though...
                    maxWpnRange = 6000;
                }
                else
                {
                    maxWpnRange = 6000;
                }
            }
        }
    }

    if (lastRange > maxWpnRange or
 not self->Sms->curWeapon  or
 not targetPtr)
    {
        self->FCC->SetMasterMode(FireControlComputer::Nav);
        curMissile = NULL;
        curMissileStation = -1;
        curMissileNum     = -1;
    }
}
