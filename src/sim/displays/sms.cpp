#include "stdhdr.h"
#include "Graphics/Include/drawbsp.h"
#include "sms.h"
#include "missile.h"
#include "misldisp.h"
#include "misslist.h"
#include "bomb.h"
#include "bombfunc.h"
#include "fsound.h"
#include "soundfx.h"
#include "simsound.h"
#include "simveh.h"
#include "guns.h"
#include "object.h"
#include "otwdrive.h"
#include "hardpnt.h"
#include "camp2sim.h"
#include "sfx.h"
#include "SmsDraw.h"
#include "aircrft.h"
#include "airframe.h"
#include "fcc.h"
#include "entity.h"
#include "Classtbl.h"
#include "Gunsfunc.h"
#include "Misslist.h"
#include "Entity.h"
#include "wpndef.h"
#include "vehicle.h"
#include "Falcgame.h"
#include "FalcSess.h"
#include "Graphics/Include/objectparent.h"
#include "limiters.h"
#include "campweap.h"
#include "playerop.h"
#include "fack.h"
#include "ffeedbk.h"
#include "simdrive.h"
#include "falcmesg.h"
#include "MsgInc/TrackMsg.h"
#include "Unit.h"
#include "radardoppler.h" //MI
#include "missdata.h" // 2002-03-08 S.G.
#include "rdrackdata.h"
#include "harmPod.h" // RV - I-Hawk

SensorClass* FindLaserPod(SimMoverClass* theObject);
//extern VuAntiDatabase *vuAntiDB;

extern short gRackId_Single_Rack;
extern short gRackId_Triple_Rack;
extern short gRackId_Quad_Rack;
extern short gRackId_Six_Rack;
extern short gRackId_Two_Rack;
extern short gRackId_Single_AA_Rack;
extern short gRackId_Mav_Rack;
extern short gRocketId;
extern short NumWeaponTypes;
extern short NumRocketTypes; // Added by M.N.
extern bool g_bSMSPylonLoadingFix; // MLR 2003-10-16
extern bool g_bBMSRackData; // MLR 2/13/2004 -
//MI
//#define MAX_RIPPLE_COUNT    12
#define MAX_RIPPLE_COUNT    19

#ifdef USE_SH_POOLS
MEM_POOL SMSBaseClass::pool;
MEM_POOL SMSClass::pool;
#endif

//extern bool g_bEnableCATIIIExtension; MI
extern bool g_bRealisticAvionics;
extern bool g_bAdvancedGroundChooseWeapon; // 2002-03-07 S.G.
extern float g_fDragDilutionFactor; // JB 010707
extern bool g_bNewRackData; // JPO

extern bool g_bUseDefinedGunDomain; // 2002-04-17 S.G.

extern bool g_bEmergencyJettisonFix; // 2002-04-21 MN
extern bool g_bRealisticMavTime; // JPG 7 Dec 03

// ==================================================================
// SMSBaseClass
//
// By Kevin K, 6/23
//
// This holds the minimum needed to keep track of weapons
// ==================================================================

VU_TIME aim9LastRunTime;

SMSBaseClass::SMSBaseClass(SimVehicleClass *newOwnship, short *weapId, uchar *weapCnt, int advanced)
{
    int i;//,j;
    WeaponClassDataType *wd;
    Falcon4EntityClassType *classPtr;
    //
    VehicleClassDataType *vc;
    int rackFlag = 0, createCount;

    flags = 0;
    numCurrentWpn = 0;

    //MI change for default state
    if ( not g_bRealisticAvionics)
    {
        masterArm = Arm;
    }
    else
    {
        masterArm = Safe;
    }

    ownship = newOwnship;

    if (ownship->IsAirplane() and ((AircraftClass*)ownship)->IsF16())
    {
        numHardpoints = 10;
    }
    else
    {
        for (numHardpoints = HARDPOINT_MAX - 1; numHardpoints >= 0; numHardpoints--)
        {
            if (weapId[numHardpoints] not_eq 0)
            {
                break;
            }
        }

        if (numHardpoints >= 0)
        {
            numHardpoints ++;
        }
        else
        {
            numHardpoints = 0;
        }
    }

    curHardpoint = -1;

    if (numHardpoints)
    {
        hardPoint = new BasicWeaponStation*[numHardpoints];
    }
    else
    {
        hardPoint = NULL;
    }

    //
    vc = GetVehicleClassData(newOwnship->Type() - VU_LAST_ENTITY_TYPE);
    rackFlag = vc->RackFlags;

    //
    for (i = 0; i < numHardpoints; i++)
    {
        if (advanced)
        {
            hardPoint[i] = new AdvancedWeaponStation;
        }
        else
        {
            hardPoint[i] = new BasicWeaponStation;
        }

        hardPoint[i]->weaponPointer.reset();
        hardPoint[i]->weaponId = (unsigned short)(weapId[i]);
        hardPoint[i]->weaponCount = (unsigned short)(weapCnt[i]);

        if (hardPoint[i]->weaponId and hardPoint[i]->weaponCount)
        {
            wd = &WeaponDataTable[hardPoint[i]->weaponId];
            ShiAssert(wd);

            if (wd->Flags bitand WEAP_ONETENTH)
                hardPoint[i]->weaponCount *= 10;

            // sfr: in DF and IA we dont respect these hardpoint limitations
            // fixes DF bug where you select 8 missiles and after second youre out of ammo
            createCount = (
                              (FalconLocalGame->gameType == game_Dogfight) or
                              (FalconLocalGame->gameType == game_InstantAction) or
                              (rackFlag bitand (1 << i))
                          ) ? hardPoint[i]->weaponCount : 1;
            classPtr = &Falcon4ClassTable[wd->Index];

            if (classPtr->vuClassData.classInfo_[VU_TYPE] == TYPE_GUN and 
                classPtr->vuClassData.classInfo_[VU_CLASS] == CLASS_WEAPON)
            {
                // This is a gun, initialize some extra data
                hardPoint[i]->weaponPointer.reset(InitAGun(newOwnship, hardPoint[i]->weaponId, weapCnt[i]));
                SetFlag(GunOnBoard);
            }
            else if (
                classPtr->vuClassData.classInfo_[VU_TYPE] == TYPE_MISSILE or
                classPtr->vuClassData.classInfo_[VU_TYPE] == TYPE_ROCKET
            )
            {
                // This is a Missile, create one for each shot (linked list)
                hardPoint[i]->weaponPointer = InitWeaponList(
                                                  newOwnship, hardPoint[i]->weaponId,
                                                  hardPoint[i]->GetWeaponClass(), createCount, InitAMissile
                                              );
            }
            else if (
                classPtr->vuClassData.classInfo_[VU_TYPE] == TYPE_BOMB or
                (
                    classPtr->vuClassData.classInfo_[VU_TYPE] == TYPE_ELECTRONICS and 
                    classPtr->vuClassData.classInfo_[VU_CLASS] == CLASS_VEHICLE
                ) or
                (
                    classPtr->vuClassData.classInfo_[VU_TYPE] == TYPE_FUEL_TANK and 
                    classPtr->vuClassData.classInfo_[VU_STYPE] == STYPE_FUEL_TANK
                ) or
                (
                    classPtr->vuClassData.classInfo_[VU_TYPE] == TYPE_RECON and 
                    classPtr->vuClassData.classInfo_[VU_STYPE] == STYPE_CAMERA
                )
            )
            {
                // This is a Bomb or other dropable object, create one for each shot (linked list)
                hardPoint[i]->weaponPointer = InitWeaponList(
                                                  newOwnship, hardPoint[i]->weaponId, hardPoint[i]->GetWeaponClass(), createCount, InitABomb
                                              );
            }
            // MLR 3/5/2004 - launchers are "special" bombs
            else if (
                classPtr->vuClassData.classInfo_[VU_TYPE] == TYPE_LAUNCHER and 
                classPtr->vuClassData.classInfo_[VU_STYPE] == STYPE_ROCKET
            )
            {
                if (hardPoint[i]->weaponCount == 19) // MLR 1/20/2004 - Kludge for IA
                {
                    hardPoint[i]->weaponCount = 1;
                }

                // This is a Bomb or other dropable object, create one for each shot (linked list)
                hardPoint[i]->weaponPointer = InitWeaponList(
                                                  newOwnship, hardPoint[i]->weaponId,
                                                  hardPoint[i]->GetWeaponClass(), hardPoint[i]->weaponCount, InitABomb
                                              );
            }
            else
            {
                MonoPrint("Vehicle has unknown weapon type %d.\n", wd->Index);
            }
        }
    }
}

SMSBaseClass::~SMSBaseClass()
{
    int i;
    SimWeaponClass *tmpwpn;

    if (hardPoint)
    {
        // Clear out unfired weapons
        for (i = 0; i < numHardpoints; i++)
        {
            // edg bug fix hack (try): for some reason we're crashing
            // in here on ground unit cleanup.  I'm not sure what's
            // causing this, but I have seen the hardpoint weaponCount
            // be 0 and weaponPointer non-NULL.  Ths hack is to make
            // sure if weaponCount is 0 weaponPointer is NULL.
            // if ( hardPoint[i]->weaponCount == 0 )
            // hardPoint[i]->weaponPointer = NULL;
            while (hardPoint[i] and hardPoint[i]->weaponPointer)
            {
                tmpwpn = hardPoint[i]->weaponPointer.get();
                hardPoint[i]->weaponPointer.reset(tmpwpn->GetNextOnRail());
                //delete tmpwpn;
                //vuAntiDB->Remove(tmpwpn);
            }

            if (hardPoint[i])
            {
                delete hardPoint[i];
            }

            hardPoint[i] = NULL;
        }

        // sfr: [] since its a vector
        delete []hardPoint;
        hardPoint = NULL;
    }
}

GunClass* SMSBaseClass::GetGun(int station)
{
    if (hardPoint and station >= 0 and hardPoint[station]->weaponPointer and hardPoint[station]->weaponPointer->IsGun())
        return (GunClass*) hardPoint[station]->weaponPointer.get();

    return NULL;
}

MissileClass* SMSBaseClass::GetMissile(int hardpoint)
{
    if (hardPoint and hardPoint[hardpoint]->weaponPointer and hardPoint[hardpoint]->weaponPointer->IsMissile())
        return (MissileClass*) hardPoint[hardpoint]->weaponPointer.get();

    return NULL;
}

BombClass* SMSBaseClass::GetBomb(int hardpoint)
{
    if (hardPoint and hardPoint[hardpoint]->weaponPointer and hardPoint[hardpoint]->weaponPointer->IsBomb())
        return (BombClass*) hardPoint[hardpoint]->weaponPointer.get();

    return NULL;
}

SimWeaponClass* SMSBaseClass::GetCurrentWeapon(void)
{
    if (hardPoint and curHardpoint > -1)
    {
        return hardPoint[curHardpoint]->weaponPointer.get();
    }
    else
    {
        return NULL;
    }
}

short SMSBaseClass::GetCurrentWeaponIndex(void)
{
    if (curHardpoint > -1)
        return WeaponDataTable[hardPoint[curHardpoint]->weaponId].Index;

    return 0;
}


float SMSBaseClass::GetCurrentWeaponRangeFeet(void)
{
    if (curHardpoint > -1)
        return WeaponDataTable[hardPoint[curHardpoint]->weaponId].Range * KM_TO_FT;

    return 0.0F;
}

void SMSClass::RemoveWeapon(int hp)
{
    VuBin<SimWeaponClass> weapPtr;

    if (
        (hardPoint) and 
        (hp > -1) and 
        (hardPoint[hp]->weaponPointer) and 
        (hardPoint[hp]->weaponCount > 0)
    )
    {
        weapPtr = hardPoint[hp]->DetachFirstWeapon(); // removes BSP too

        if ( not weapPtr)
        {
            return;
        }

        if (weapPtr->IsMissile())
        {
            MissileClass *m = static_cast<MissileClass*>(weapPtr.get());
            m->SetTarget(NULL);
            m->ClearReferences();
        }
        else if (weapPtr->IsBomb())
        {
            BombClass *b = static_cast<BombClass*>(weapPtr.get());
            b->SetTarget(NULL);
        }

        RemoveStore(hp, hardPoint[hp]->weaponId);

        if (weapPtr->drawPointer)
        {
            OTWDriver.RemoveObject(weapPtr->drawPointer, TRUE);
            weapPtr->drawPointer = NULL;
        }
    }
}

// This function is intended only for the SMSBaseClass
// the advanced SMSClass should call LaunchMissile or otherwise appropriate function
void SMSBaseClass::LaunchWeapon(void)
{
    VuBin<SimWeaponClass> theWeapon;
    //SimWeaponClass *theWeapon;
    Tpoint simLoc;
    //float dx,dy,dz,xydist,yaw,pitch;
    SimObjectType *tmpTargetPtr = ownship->targetPtr;
    int visFlag;
    VehicleClassDataType* vc;
    int slotId;

    if ( not ownship->drawPointer or not tmpTargetPtr)
    {
        return;
    }

    // edg: there was a problem.  Some ground vehicles do NOT have guns
    // at all.  All this code assumes a gun in Slot 0 and therefore subtracts
    // 1 from curhardpoint to get the right slot.  Don't do this if there
    // are no guns on board.
    if (IsSet(GunOnBoard))
    {
        slotId = max(0, curHardpoint - 1);
    }
    else
    {
        slotId = curHardpoint;
    }

    vc = GetVehicleClassData(ownship->Type() - VU_LAST_ENTITY_TYPE);
    visFlag = vc->VisibleFlags;

    if (
        (hardPoint and curHardpoint > -1) and 
        (hardPoint[curHardpoint]->weaponPointer) and 
        (hardPoint[curHardpoint]->weaponCount > 0)
    )
    {
        theWeapon = hardPoint[curHardpoint]->weaponPointer;
        hardPoint[curHardpoint]->weaponCount--;
        DetachWeapon(curHardpoint, theWeapon.get());
        //DetachWeapon(curHardpoint, theWeapon);

        // Detach visual from parent
        if (theWeapon->drawPointer)
        {
            // KCK: Call detach directly, so we can use it's updated position/orientation
            ((DrawableBSP*)ownship->drawPointer)->DetachChild((DrawableBSP*)theWeapon->drawPointer, slotId);
        }

        if (theWeapon->IsMissile() and ownship->drawPointer)
        {
            if (visFlag bitand (1 << curHardpoint))
            {
                ((DrawableBSP*)ownship->drawPointer)->GetChildOffset(slotId, &simLoc);
            }
            else
            {
                simLoc.x = simLoc.y = simLoc.z = 0.0F;
            }

            // The weapon's position/orientation are being set relative to the parent's postion and orientation
            MissileClass *theMissile = static_cast<MissileClass*>(theWeapon.get());
            theMissile->SetLaunchPosition(simLoc.x, simLoc.y, simLoc.z);
            theMissile->SetLaunchRotation(ownship->GetDOFValue(0), ownship->GetDOFValue(1));

            //((MissileClass*)theWeapon.get())->SetLaunchPosition (simLoc.x, simLoc.y, simLoc.z);
            //((MissileClass*)theWeapon.get())->SetLaunchRotation (ownship->GetDOFValue(0), ownship->GetDOFValue(1));
        }
        else
        {
            // Just use parent object's position/orientation
            // KCK: These take world coodinates, but I can't really think of a non-missile ground weapon
            // which launches...
            theWeapon->SetPosition(ownship->XPos(), ownship->YPos(), ownship->ZPos());
            theWeapon->SetYPR(ownship->Yaw(), ownship->Pitch(), 0.0F);
        }

        theWeapon->SetDelta(ownship->XDelta(), ownship->YDelta(), ownship->ZDelta());

        // If we're direct mounted weapons and have more weapons on this hardpoint, but no weapon pointers,
        // replace the weapon with an identical copy
        if (
 not hardPoint[curHardpoint]->GetRackOrPylon() and // MLR 2/20/2004 - added OrPylon
            hardPoint[curHardpoint]->weaponCount and 
 not hardPoint[curHardpoint]->weaponPointer
        )
        {
            if (theWeapon->IsMissile())
            {
                //ReplaceMissile(curHardpoint, (MissileClass*)theWeapon.get());
                ReplaceMissile(curHardpoint, (MissileClass*)theWeapon.get());
            }

            if (theWeapon->IsBomb())
            {
                //ReplaceBomb(curHardpoint, (BombClass*)theWeapon.get());
                ReplaceBomb(curHardpoint, (BombClass*)theWeapon.get());
            }
        }

        // If next weapon is a visible weapon, attach it to the hardpoint (note: some delay to this would be cool)
        /*
        if (hardPoint[curHardpoint]->weaponPointer and visFlag bitand (1 << curHardpoint)){
         OTWDriver.CreateVisualObject(hardPoint[curHardpoint]->weaponPointer);
         OTWDriver.AttachObject(
         ownship->drawPointer, (DrawableBSP*)hardPoint[curHardpoint]->weaponPointer->drawPointer, curHardpoint
         );
        }
        */
    }
}

void SMSBaseClass::StepMasterArm(void)
{
    switch (masterArm)
    {
        case Safe:
            SetMasterArm(Sim);
            break;

        case Arm:
            SetMasterArm(Safe);
            break;

        default:
            SetMasterArm(Arm);
    }
}

// OW CAT III cockpit switch extension
void SMSBaseClass::StepCatIII()
{
    //if(g_bEnableCATIIIExtension) MI
    if (g_bRealisticAvionics)
    {
        if (((AircraftClass *)ownship)->af->IsSet(AirframeClass::CATLimiterIII))
            ((AircraftClass *)ownship)->af->ClearFlag(AirframeClass::CATLimiterIII);
        else
            ((AircraftClass *)ownship)->af->SetFlag(AirframeClass::CATLimiterIII);
    }
}
void SMSBaseClass::DetachWeapon(int hardpoint, SimWeaponClass *theWeapon)
{
    VuBin<SimWeaponClass> weapPtr = hardPoint[hardpoint]->weaponPointer;
    VuBin<SimWeaponClass> lastPtr;

    while (weapPtr)
    {
        if (weapPtr.get() == theWeapon)
        {
            if (lastPtr)
            {
                lastPtr->nextOnRail = theWeapon->nextOnRail;
            }
            else
            {
                hardPoint[hardpoint]->weaponPointer = theWeapon->nextOnRail;
            }

            theWeapon->nextOnRail.reset();
            return;
        }

        lastPtr = weapPtr;
        weapPtr = weapPtr->nextOnRail;
    }
}

float SMSBaseClass::GetWeaponRangeFeet(int hardpoint)
{
    ShiAssert(hardpoint >= 0 and hardpoint < numHardpoints);
    return WeaponDataTable[hardPoint[hardpoint]->weaponId].Range * KM_TO_FT;
}

// 2002-03-09 MODIFIED BY S.G. Added the alt_feet variable so it knows the altitude of the target as well as it range
void SMSBaseClass::SelectBestWeapon(uchar *dam, int mt, int range_km, int guns_only, int alt_feet)
{
    int i, str;
    int bhp = -1;
    int bw = 0, bs = 0;
    int wrange;

    for (i = 0; i < numHardpoints; i++)
    {
        if (hardPoint[i] and hardPoint[i]->weaponId and hardPoint[i]->weaponCount)
        {
            if (range_km >= 0 and not ((Unit)(ownship->GetCampaignObject()))->CanShootWeapon(hardPoint[i]->weaponId))
            {
                str = 0;
            } //JPO check
            else if (guns_only and hardPoint[i]->weaponPointer and not hardPoint[i]->weaponPointer->IsGun())
            {
                str = 0;
            }
            else if (range_km >= 0)
            {
                wrange = GetWeaponRange(hardPoint[i]->weaponId, mt);

                if (wrange > 2)
                {
                    // 2000-10-12 MODIFIED BY S.G. DIVIDER OF 11 INSTEAD IF IT'S A GUN.
                    // THAT WAY, RANGE * LowAirModifier MIGHT STILL SELECT THIS WEAPON IF THE RANGE IS 15 NM.
                    //if ( range_km < min(wrange/4, 2) or range_km > wrange )
                    // continue;
                    if (hardPoint[i]->weaponPointer->IsGun())
                    {
                        if (range_km < min(wrange / 11, 2) or range_km > wrange)
                            continue;
                    }
                    // 2002-03-09 MODIFIED BY S.G.
                    // New weapon selection code based on the target min/max alt and range.
                    // If g_bAdvancedGroundChooseWeapon is false or alt_feet is -1, use the original code
                    // else if ( range_km < min(wrange/4, 2) or range_km > wrange )
                    // continue;
                    else
                    {
                        // If we're shooting at an air thingy and this weapon is a ...
                        // STYPE_MISSILE_SURF_AIR, we might be restricted to a min/max engagement range/altitude
                        // if we asked for it
                        if (g_bAdvancedGroundChooseWeapon and alt_feet >= 0 and (mt == LowAir or mt == Air))
                        {
                            // If we're outside the weapon's range, no point going further,
                            // no matter what the weapon is...
                            if (range_km > wrange)
                            {
                                continue;
                            }

                            // If it's a SAM...
                            VU_BYTE *classInfoPtr = Falcon4ClassTable[WeaponDataTable[
                                                        hardPoint[i]->weaponId].Index
                                                                     ].vuClassData.classInfo_;

                            if (
                                classInfoPtr[VU_DOMAIN] == DOMAIN_AIR and 
                                classInfoPtr[VU_CLASS] == CLASS_VEHICLE and 
                                classInfoPtr[VU_TYPE] == TYPE_MISSILE and 
                                classInfoPtr[VU_STYPE] == STYPE_MISSILE_SURF_AIR
                            )
                            {
                                MissileAuxData *auxData = NULL;
                                SimWeaponDataType* wpnDefinition = &SimWeaponDataTable[
                                                                       Falcon4ClassTable[WeaponDataTable[hardPoint[i]->weaponId].Index].vehicleDataIndex
                                                                   ];

                                if (wpnDefinition->dataIdx < numMissileDatasets)
                                {
                                    auxData = missileDataset[wpnDefinition->dataIdx].auxData;
                                }

                                if (auxData)
                                {
                                    float minAlt = auxData->MinEngagementAlt;
                                    float minRange = auxData->MinEngagementRange;
                                    float maxAlt = (float)WeaponDataTable[hardPoint[i]->weaponId].MaxAlt * 1000.0f;

                                    // If our range is less than the min range,
                                    // don't consider this weapon (used range squared to save a FPU costly sqrt)
                                    if (range_km * KM_TO_FT < minRange)
                                        continue;

                                    // If we haven't entered the MinEngagementAlt yet,
                                    // use the one in the Falcon4.WCD file
                                    if (minAlt < 0.0f)
                                        minAlt = (float)(WeaponDataTable[hardPoint[i]->weaponId].Name[18]) * 32.0F;

                                    // If less than min altitude or more than max altitude
                                    // (in this case alt_feet is POSITIVE if we're below the target),
                                    // don't consider this weapon
                                    if (alt_feet < minAlt or alt_feet > maxAlt)
                                        continue;
                                }
                                // No auxiliary data, default to orinal min range test...
                                else if (range_km < min(wrange / 4, 2))
                                {
                                    continue;
                                }
                            }
                            // Not a SAM but was within wrange, check if too close
                            else if (range_km < min(wrange / 4, 2))
                            {
                                continue;
                            }
                        }
                        // Original line if we don't want advanced weapon
                        // selection or our target isn't in the air or alt_feet wasn't passed (ie, is -1)
                        else if (range_km < min(wrange / 4, 2) or range_km > wrange)
                        {
                            continue;
                        }
                    }

                    // END OF MODIFIED SECTION 2002-03-09
                    // END OF MODIFIED SECTION
                }

                str = GetWeaponScore(hardPoint[i]->weaponId, dam, mt, range_km, wrange);
            }
            else
            {
                //GetWeaponScore (hardPoint[i]->weaponId, dam, mt, 1);
                str = WeaponDataTable[hardPoint[i]->weaponId].HitChance[mt];
            }

            if (str > bs)
            {
                bw = hardPoint[i]->weaponId;
                bs = str;
                bhp = i;
            }
        }
    }

    curHardpoint = bhp;
}

// KCK NOTE: This function is SPECIFIC to Ground Vehicles.
// IE: 1) All weapons which are visible can place ONE visible weapon per hardpoint
// 2) All non-visable weapons refer to the last hardpoint (or first non-visible hardpoint)
void SMSBaseClass::AddWeaponGraphics(void)
{
    int i, visFlag;
    DrawableBSP *drawPtr = (DrawableBSP*) ownship->drawPointer;
    VehicleClassDataType* vc;

    if ( not hardPoint or not drawPtr)
        return;

    vc = GetVehicleClassData(ownship->Type() - VU_LAST_ENTITY_TYPE);
    visFlag = vc->VisibleFlags;

    for (i = 0; i < numHardpoints; i++)
    {
        if (hardPoint[i] and hardPoint[i]->weaponPointer)
        {
            if (visFlag bitand (1 << i))
            {
                // This is a visible weapon, however, only the first one should get a drawPointer
                OTWDriver.CreateVisualObject(hardPoint[i]->weaponPointer.get());
                OTWDriver.AttachObject(drawPtr, (DrawableBSP*)hardPoint[i]->weaponPointer->drawPointer, i);
            }
            else if (hardPoint[i]->weaponPointer->IsGun())
            {
                // Just init gun's tracers. We don't draw the gun
                ((GunClass*)hardPoint[i]->weaponPointer.get())->InitTracers();
            }

            // Otherwise, this is a non-visible weapon - it'll init it's draw pointer on launch
        }
    }
}

void SMSBaseClass::FreeWeaponGraphics(void)
{
    int i;
    SimWeaponClass *weaponPtr;
    DrawableBSP *drawPtr = (DrawableBSP*) ownship->drawPointer;

    if ( not hardPoint or not drawPtr)
        return;

    for (i = 0; i < numHardpoints; i++)
    {
        weaponPtr = NULL;

        if (hardPoint[i])
        {
            weaponPtr = hardPoint[i]->weaponPointer.get();
        }

        while (weaponPtr)
        {

            if (weaponPtr->IsMissile())
            {
                ((MissileClass *)weaponPtr)->SetTarget(NULL);
                ((MissileClass *)weaponPtr)->ClearReferences();
            }
            else if (weaponPtr->IsBomb())
                ((BombClass *)weaponPtr)->SetTarget(NULL);

            if (weaponPtr->drawPointer)
            {
                // Detach anything with a draw pointer from the vehicle's drawpointer
                OTWDriver.DetachObject(drawPtr, (DrawableBSP*)(weaponPtr->drawPointer), i);
                OTWDriver.RemoveObject(weaponPtr->drawPointer, TRUE);
                weaponPtr->drawPointer = NULL;
            }
            else if (GetGun(i))
                GetGun(i)->CleanupTracers();

            weaponPtr = weaponPtr->GetNextOnRail();
        }
    }
}

// JPO - generalise routine to report on any station.
int SMSBaseClass::StationOK(int n)
{
    int retval = TRUE;
    FackClass* mFaults;
    int broken;

    if (ownship->IsAirplane() and ((AircraftClass*)ownship)->mFaults)
    {
        mFaults = ((AircraftClass*)ownship)->mFaults;
        broken = mFaults->GetFault(FaultClass::sms_fault);

        if (broken bitand FaultClass::bus bitand FaultClass::fail)
        {
            retval = FALSE;
        }
        else if (n >= 1)
        {
            if (broken bitand (FaultClass::sta1 << (n - 1)) bitand FaultClass::fail)
            {
                retval = FALSE;
            }
        }
    }

    return retval;
}

// ==================================================================
// SMSClass
//
// This is Leon's origional class, now used only for aircraft/helos
// ==================================================================

SMSClass::SMSClass(SimVehicleClass *newOwnship, short *weapId, uchar *weapCnt) : SMSBaseClass(newOwnship, weapId, weapCnt, TRUE)
{
    int i;
    Falcon4EntityClassType* classPtr;
    SimWeaponDataType* wpnDefinition;
    int dataIndex;
    VehicleClassDataType* vc;
    WeaponClassDataType* wc;
    int rackFlag, visFlag;
    GunClass* gun;

    flash     = FALSE;
    curHardpoint = -1;
    curWpnNum = -1;
    curWeaponId = -1;
    //curWeapon = NULL;
    curWeaponType = wtNone;
    curWeaponClass = wcNoWpn;
    curWeaponDomain = wdNoDomain;
    flags = FALSE;
    //rippleCount = 0;
    //rippleInterval = 25;
    //rippleInterval = 125; // JB 010701
    curRippleCount = 0;
    //pair = FALSE;
    nextDrop = 0;
    // default burst height to optimum height...default for AI
    burstHeight  = 1000.0F;
    armingdelay = 480; //me123 status ok. addet
    aim120id = 0; // JPO added.
    aim9mode = WARM;
    aim9cooltime = 3.0F;
    aim9warmtime = 0.0F;
    aim9coolingtimeleft = 1.5 * 60 * 60;//1.5 * 60.0 * 60.0 * CLOCKS_PER_SEC;
    // Test - 10 seconds of coolant
    // aim9coolingtimeleft = 10.0F;// * CLOCKS_PER_SEC;
    aim9LastRunTime = 0;
    drawable = NULL;

    //MI
    //angle = 23;

    curProfile = 0;

    agbProfile[0].rippleCount     = 0;
    agbProfile[0].rippleInterval  = 175;
    agbProfile[0].fuzeNoseTail    = 0;
    agbProfile[0].burstAltitude   = 1000;
    agbProfile[0].releaseAngle    = 23;
    agbProfile[0].C1ArmDelay1     = 400;
    agbProfile[0].C1ArmDelay2     = 600;
    agbProfile[0].C2ArmDelay      = 150;
    agbProfile[0].releasePair     = FALSE;
    agbProfile[0].subMode   = FireControlComputer::CCRP;

    agbProfile[1].rippleCount     = 3;
    agbProfile[1].rippleInterval  = 25;
    agbProfile[1].fuzeNoseTail    = 1;
    agbProfile[1].burstAltitude   = 500;
    agbProfile[1].releaseAngle    = 23;
    agbProfile[1].C1ArmDelay1     = 400;
    agbProfile[1].C1ArmDelay2     = 600;
    agbProfile[1].C2ArmDelay      = 150;
    agbProfile[1].releasePair     = FALSE;
    agbProfile[1].subMode   = FireControlComputer::CCIP;

    runRockets = 0; // MLR 6/3/2004 -

    /*Prof1RP = 0;
    Prof2RP = 3;
    Prof1RS = 175;
    Prof2RS = 25;
    Prof1Pair = FALSE;
    Prof2Pair = FALSE;
    Prof1NSTL = 0;
    Prof2NSTL = 1;
    C1AD1 = 600;
    C1AD2 = 150;
    C2AD = 400;
    C2BA = 500;
    Prof1SubMode = FireControlComputer::CCRP;
    Prof2SubMode = FireControlComputer::CCIP;
    Prof1Pair = FALSE;
    Prof2Pair = TRUE;
    Prof1 = FALSE; */

    //MI
    BHOT = TRUE;
    GndJett = FALSE;
    FEDS = FALSE;
    DrawFEDS = FALSE;
    Powered = FALSE; //for Mav's

    if (g_bRealisticMavTime) MavCoolTimer = 180.0F;
    else MavCoolTimer = 5.0F; //5 seconds cooling time (a guess) // JPG 06 Dec 03 - changed to 3 mins for mav. gyro spool up

    MavSubMode = PRE;

    ShiAssert(weapCnt);

    for (i = 0; i <= wtNone; i++)
        numOnBoard[i] = 0;

    // Set up advanced hardpoint data
    vc = GetVehicleClassData(newOwnship->Type() - VU_LAST_ENTITY_TYPE);
    rackFlag = vc->RackFlags;
    visFlag = vc->VisibleFlags;

    for (i = 0; i < numHardpoints; i++)
    {
        if (hardPoint[i] and hardPoint[i]->weaponId and hardPoint[i]->weaponPointer)
        {
            wc = &WeaponDataTable[hardPoint[i]->weaponId];
            classPtr = &(Falcon4ClassTable[wc->Index]);
            dataIndex = classPtr->vehicleDataIndex;
            // 2002-03-21 MN catch data errors
            ShiAssert(wc);
            ShiAssert(classPtr);


            //LRKLUDGE
            if ((classPtr->vuClassData.classInfo_[VU_TYPE] == TYPE_RECON and 
                 classPtr->vuClassData.classInfo_[VU_STYPE] == STYPE_CAMERA))
            {
                dataIndex = Rpod_DEF;
            }

            // wpnDefinition = (SimWpnDefinition*)moverDefinitionData[dataIndex];
            wpnDefinition = &SimWeaponDataTable[classPtr->vehicleDataIndex];
            hardPoint[i]->SetWeaponClass((WeaponClass)wpnDefinition->weaponClass);
            hardPoint[i]->SetWeaponType((WeaponType)wpnDefinition->weaponType);
            hardPoint[i]->GetWeaponData()->domain = (WeaponDomain)wpnDefinition->domain;
            // edg kludge, look for durandal and set its drag to value
            // the weapon def files (at least for bombs) all seem to point
            // to the same thing -- mkxxx
            hardPoint[i]->GetWeaponData()->cd = wpnDefinition->cd;

            if (classPtr->vuClassData.classInfo_[VU_TYPE] == TYPE_BOMB)
            {
                if (classPtr->vuClassData.classInfo_[VU_STYPE] == STYPE_BOMB and 
                    classPtr->vuClassData.classInfo_[VU_SPTYPE] == SPTYPE_DURANDAL)
                {
                    hardPoint[i]->GetWeaponData()->cd = 1.0f;
                }
                // JB 010707 Why?? This makes the drag insignificant.  Make it configurable
                else if (hardPoint[i]->GetWeaponData()->cd < 1.0f)
                {
                    //hardPoint[i]->GetWeaponData()->cd *= 0.01f;
                    hardPoint[i]->GetWeaponData()->cd *= 0.2f * g_fDragDilutionFactor;
                }
                else
                {
                    // if it's not a durandal and it's got a value >= 1
                    // make it a a drag of 0.9 so that the bombclass doesn't
                    // think it's a durandal. did I hear kludge?
                    hardPoint[i]->GetWeaponData()->cd = 0.9f;
                }
            }

            //LRKLUDGE problem w/ AA7R's
            if ((classPtr->vuClassData.classInfo_[VU_TYPE] == TYPE_MISSILE and 
                 classPtr->vuClassData.classInfo_[VU_STYPE] == STYPE_MISSILE_AIR_AIR and 
                 classPtr->vuClassData.classInfo_[VU_SPTYPE] == SPTYPE_AA7R))
            {
                hardPoint[i]->SetWeaponType(wtAim120);
            }

            hardPoint[i]->GetWeaponData()->area = wpnDefinition->area;
            hardPoint[i]->GetWeaponData()->weight = wpnDefinition->weight;
            hardPoint[i]->GetWeaponData()->xEjection = wpnDefinition->xEjection;
            hardPoint[i]->GetWeaponData()->yEjection = wpnDefinition->yEjection;
            hardPoint[i]->GetWeaponData()->zEjection = wpnDefinition->zEjection;
            hardPoint[i]->GetWeaponData()->flags = wpnDefinition->flags;

            // Set CBU flag appropriatly
            if (wc->Flags bitand WEAP_CLUSTER)
                hardPoint[i]->GetWeaponData()->flags or_eq HasBurstHeight;

            strcpy(hardPoint[i]->GetWeaponData()->mnemonic, wpnDefinition->mnemonic);
            IncrementStores(hardPoint[i]->GetWeaponClass(), hardPoint[i]->weaponCount);
            gun = GetGun(i);

            if (gun)
            {
                // Special stuff for guns
                SetFlag(GunOnBoard);
                hardPoint[i]->SetGun(gun);
                hardPoint[i]->GetWeaponData()->xEjection = gun->initBulletVelocity;

                if ( not g_bUseDefinedGunDomain) // 2002-04-17 ADDED BY S.G. Why fudge the weapon domain of guns instead of relying on what's in the data file?
                    hardPoint[i]->GetWeaponData()->domain = gun->GetSMSDomain();
            }

            // Setup rack data
            //if (rackFlag bitand (1 << i))
            {
                // 2002-03-24 MN Helicopter also create a SMS class, but don't have auxaerodata, so add a check for Airplane
                if (ownship->IsAirplane() and ((AircraftClass*)ownship)->af and wc)
                {
                    hardPoint[i]->SetHPId(i);
                    hardPoint[i]->DetermineRackData(((AircraftClass*)ownship)->af->GetRackGroup(i), hardPoint[i]->weaponId, weapCnt[i]);

                    // MLR 3/20/2004 - use loadorder rackdata
                    int *lo;

                    if (lo = hardPoint[i]->GetLoadOrder())
                    {
                        int l = 0;
                        SimWeaponClass *weapPtr = hardPoint[i]->weaponPointer.get();

                        while (weapPtr and l < hardPoint[i]->NumPoints())
                        {
                            weapPtr->SetRackSlot(lo[l]);
                            l++;
                            weapPtr = weapPtr->GetNextOnRail();
                        }
                    }
                }
                else
                {
                    SetupHardpointImage(hardPoint[i], weapCnt[i]);
                }
            }
        }
    }

    // Check for HARM, LGB, ECM
    if (numOnBoard[wcHARMWpn] > 0)
    {
        SetFlag(HTSOnBoard);
    }

    if (numOnBoard[wcGbuWpn] > 0)
    {
        SetFlag(LGBOnBoard);
    }

    // 2000-11-17 MODIFIED BY S.G. SO INTERNAL ECMS ARE ACCOUNTING FOR
    // if (numOnBoard[wcECM] > 0)
    if (numOnBoard[wcECM] > 0 or vc->Flags bitand VEH_HAS_JAMMER)
    {
        SetFlag(SPJamOnBoard);
    }

    JDAMPowered = FALSE;
    JDAMInitTimer = 10.0f;
    JDAMtargeting = SMSBaseClass::PB; // 0 = PB, 1 = TOO

    HARMPowered = false;
    HARMInitTimer = 2.5f;
}

SMSClass::~SMSClass(void)
{
    FreeWeapons();

    delete drawable;
    drawable = NULL;
}


// JPO - make this into a routine, so we can call it as a fallback
// RV - Biker - This is used only for choppers
void SMSClass::SetupHardpointImage(BasicWeaponStation *hp, int count)
{
    ShiAssert(hp);

    if ( not hp)
        return;

    // RV - Biker - Hardcode chopper racks for now

    // Find the proper rack id and max points
    //if (hp->GetWeaponClass() == wcRocketWpn)
    //{
    // // The rack id should have already been set up in SMSBaseClass
    // hp->SetupPoints(1);
    //}
    //else if (count == 1)
    //{
    // if(hp->GetWeaponClass() == wcAimWpn)
    // {
    // hp->SetupPoints(1);
    // hp->SetRackId(gRackId_Single_AA_Rack);
    // }
    // else
    // {
    // hp->SetupPoints(1);
    // hp->SetRackId(gRackId_Single_Rack);
    // }
    //}
    //else if (count <= 2 and hp->GetWeaponClass() == wcAimWpn)
    //{
    // hp->SetupPoints(2);
    // hp->SetRackId(gRackId_Two_Rack);
    //}
    //else if (count <= 3)
    //{
    // if(hp->GetWeaponClass() == wcAgmWpn)
    // {
    // hp->SetupPoints(3);
    // hp->SetRackId(gRackId_Mav_Rack);
    // }
    // else
    // {
    // hp->SetupPoints(3);
    // hp->SetRackId(gRackId_Triple_Rack);
    // }
    //}
    //else if (count <= 4)
    //{
    // hp->SetupPoints(4);
    // hp->SetRackId(gRackId_Quad_Rack);
    //}
    //else
    //{
    // hp->SetupPoints(6);
    // hp->SetRackId(gRackId_Six_Rack);
    //}

    // RV - Biker - Without data adjustment this will cause CTD
    switch (count)
    {
        case 0:
            break;

        case 1:
            hp->SetupPoints(1);
            hp->SetRackId(747);
            break;

        case 2:
            hp->SetupPoints(2);
            hp->SetRackId(746);
            break;

        case 3:
            hp->SetupPoints(3);
            hp->SetRackId(745);
            break;

        case 4:
            hp->SetupPoints(4);
            hp->SetRackId(745);
            break;

        default:
            hp->SetupPoints(min(count, 4));
            hp->SetRackId(745);
            break;
    }
}

// MLR replacement for original
void SMSClass::AddWeaponGraphics(void)
{
    int i;
    Tpoint simView;
    float xOff, yOff, zOff;
    VehicleClassDataType* vc;
    int rackFlag, visFlag;
    SimWeaponClass *weapPtr;
    DrawableBSP *drawPtr = (DrawableBSP*) ownship->drawPointer;

    if ( not hardPoint)
        return;

    vc = GetVehicleClassData(ownship->Type() - VU_LAST_ENTITY_TYPE);
    rackFlag = vc->RackFlags;
    visFlag = vc->VisibleFlags;

    for (i = 0; i < numHardpoints; i++)
    {
        if (hardPoint[i] and hardPoint[i]->weaponPointer)
        {
            hardPoint[i]->SetSMS(this);
            hardPoint[i]->SetHPId(i);
            hardPoint[i]->SetParentDrawPtr(drawPtr);

            if (hardPoint[i]->weaponPointer->IsGun()/* and i==0*/)
            {
                // MLR 1/28/2004 - added i==0
                ((GunClass*)hardPoint[i]->weaponPointer.get())->InitTracers();
            }
            else
            {
                if (i > 0)
                {
                    if (visFlag bitand (1 << i))
                    {
                        drawPtr->GetChildOffset(i - 1, &simView);
                    }
                    else
                    {
                        simView.x = 0.0F;
                        simView.y = 0.0F;
                        simView.z = 0.0F;
                    }


                    hardPoint[i]->SetPosition(simView.x, simView.y, simView.z);


                    if (rackFlag bitand (1 << i))
                    {
                        // create the pylon
                        if (hardPoint[i]->GetPylonId())
                        {
                            hardPoint[i]->AttachPylonBSP();
                            AddStore(i, hardPoint[i]->GetPylonId(), (visFlag bitand (1 << i)));
                        }

                        // create the rack
                        if (hardPoint[i]->GetRackId())
                        {
                            hardPoint[i]->AttachRackBSP();
                            AddStore(i, hardPoint[i]->GetRackId(), (visFlag bitand (1 << i)));
                        }
                    }

                    // Use the rack offset in addition to our location offset
                    hardPoint[i]->GetPosition(&xOff, &yOff, &zOff);


                    /*
                    int ispod=0;
                    if(hardPoint[i]->podPointer)
                    {
                     ispod=1;
                     // since we're dealing with pods, we just need to add the
                     // weapon weight/drag whatever via AddStore()
                     weapPtr = hardPoint[i]->weaponPointer;
                     while(weapPtr)
                     {
                     AddStore(i, hardPoint[i]->weaponId, (visFlag bitand (1 << i)));
                     weapPtr=weapPtr->GetNextOnRail();
                     }

                     weapPtr = hardPoint[i]->podPointer;

                    }
                    else */
                    weapPtr = hardPoint[i]->weaponPointer.get();

                    int first = 1;

                    while (weapPtr)
                    {
                        if (visFlag bitand (1 << i) and 
                            (first or hardPoint[i]->GetRack()))

                        {
                            hardPoint[i]->AttachWeaponBSP(weapPtr);
                        }

                        AddStore(i, hardPoint[i]->weaponId, (visFlag bitand (1 << i)));

                        // MLR 3/15/2004 - Add Launchers munition
                        if (weapPtr->IsLauncher())
                        {
                            BombClass *lau = (BombClass *)weapPtr;
                            int wid  = lau->LauGetWeaponId();
                            int rnds = lau->LauGetRoundsRemaining();
                            int l;

                            for (l = 0; l < rnds; l++)
                            {
                                AddStore(i, wid, 0); // rockets are not "visible" - no extra drag
                            }
                        }

                        if (weapPtr->IsGun())  // init  gun pod // MLR 1/28/2004 -
                        {
                            ((GunClass*)weapPtr)->InitTracers();
                            //((GunClass*)weapPtr)->SetPosition(simView.x, simView.y, simView.z,0,0);
                        }

                        first = 0;
                        weapPtr = weapPtr->GetNextOnRail();
                    }
                }
            }
        }
    }
}



#if 0
void SMSClass::AddWeaponGraphics(void)
{
    int i, j;
    Tpoint simView;
    Trotation viewRot = IMatrix;
    float xOff, yOff, zOff;
    VehicleClassDataType* vc;
    int rackFlag, visFlag;
    SimWeaponClass *weapPtr, *firstPtr;
    DrawableBSP *drawPtr = (DrawableBSP*) ownship->drawPointer;

    if ( not hardPoint)
        return;

    vc = GetVehicleClassData(ownship->Type() - VU_LAST_ENTITY_TYPE);
    rackFlag = vc->RackFlags;
    visFlag = vc->VisibleFlags;

    for (i = 0; i < numHardpoints; i++)
    {
        if (hardPoint[i] and hardPoint[i]->weaponPointer)
        {
            if (hardPoint[i]->weaponPointer->IsGun()/* and i==0*/) // MLR 1/28/2004 - added i==0
                ((GunClass*)hardPoint[i]->weaponPointer)->InitTracers();
            else
            {
                if (i > 0)
                {
                    if (visFlag bitand (1 << i))
                    {
                        drawPtr->GetChildOffset(i - 1, &simView);
                    }
                    else
                    {
                        simView.x = 0.0F;
                        simView.y = 0.0F;
                        simView.z = 0.0F;
                    }

                    hardPoint[i]->SetPosition(simView.x, simView.y, simView.z);

                    DrawableBSP *pylonBSP = 0,
                                   *rackBSP = 0,
                                       *parentBSP = drawPtr; // parent is who the weapons will be attached too


                    if (rackFlag bitand (1 << i))
                    {
                        // create the pylon
                        int pylonid = hardPoint[i]->GetPylonId();

                        if (pylonid > 0 and pylonid < NumWeaponTypes and WeaponDataTable[pylonid].Index >= 0) // JB 010805 CTD
                        {
                            pylonBSP = new DrawableBSP(Falcon4ClassTable[WeaponDataTable[pylonid].Index].visType[0], &simView, &viewRot, OTWDriver.Scale());
                            hardPoint[i]->SetPylon(pylonBSP);
                            OTWDriver.AttachObject(parentBSP, pylonBSP, i - 1);
                            AddStore(i, pylonid, (visFlag bitand (1 << i)));
                            parentBSP = pylonBSP;

                            // update the HP's bogus "postion"
                            Tpoint o;
                            parentBSP->GetChildOffset(0, &o);
                            hardPoint[i]->SetPosition(simView.x + o.x, simView.y + o.y, simView.z + o.z);
                        }

                        // Create the proper type of rack (from rack Id)
                        int rackid = hardPoint[i]->GetRackId();

                        if (rackid > 0 and rackid < NumWeaponTypes and WeaponDataTable[rackid].Index >= 0) // JB 010805 CTD
                        {
                            rackBSP = new DrawableBSP(Falcon4ClassTable[WeaponDataTable[rackid].Index].visType[0], &simView, &viewRot, OTWDriver.Scale());
                            hardPoint[i]->SetRack(rackBSP);

                            // if the parent is the pylon, then attach the rack to slot 0
                            // otherwise, attach it to the proper slot on the a/c
                            if (parentBSP == pylonBSP)
                                OTWDriver.AttachObject(parentBSP, rackBSP, 0);
                            else
                                OTWDriver.AttachObject(parentBSP, rackBSP, i - 1);

                            AddStore(i, hardPoint[i]->GetRackId(), (visFlag bitand (1 << i)));
                            parentBSP = rackBSP;
                        }
                    }

                    // Use the rack offset in addition to our location offset
                    hardPoint[i]->GetPosition(&xOff, &yOff, &zOff);

                    // MLR 1/11/2004 - if we have a podPointer, then we need to load it
                    //                 onto the rack instead of the weaponPtr
                    //                 at some point, we might load the weaponPtr, and
                    //                 attach it's drawable to the pod.
                    /*
                    firstPtr = weapPtr = hardPoint[i]->podPointer;
                    while (weapPtr)
                    {
                     // determine the correct slot (j) to attach to, depending on who the parent BSP is.
                     if(rackBSP)
                     {
                     rackBSP->GetChildOffset(j, &simView);
                     j = weapPtr->GetRackSlot();
                     if (j >= hardPoint[i]->NumPoints())
                     j = hardPoint[i]->NumPoints()-1;
                     }
                     else
                     {
                     simView.x = simView.y = simView.z = 0.0F;
                     if(pylonBSP)
                     j=0;
                     else
                     j=i-1; // is a/c
                     }

                     // if we don't have a rackBSP, then we can only attach 1 weapon
                     if(  visFlag bitand (1 << i) and ( rackBSP or weapPtr == firstPtr)  )
                     {
                     OTWDriver.CreateVisualObject(weapPtr);
                     ((DrawableBSP*)(weapPtr->drawPointer))->SetSwitchMask(0,1);
                     OTWDriver.AttachObject(parentBSP, (DrawableBSP*)(weapPtr->drawPointer), j);
                     }

                     hardPoint[i]->SetSubPosition(j, xOff + simView.x, yOff + simView.y, zOff + simView.z);
                     hardPoint[i]->SetSubRotation(j, 0.0F, 0.0F);
                     AddStore(i, hardPoint[i]->weaponId, (visFlag bitand (1 << i)));
                     weapPtr = weapPtr->GetNextOnRail();
                    }
                    */

                    if (hardPoint[i]->podPointer)
                    {
                        // since we're dealing with pods, we just need to add the
                        // weapon weight/drag whatever via AddStore()
                        weapPtr = hardPoint[i]->weaponPointer;

                        while (weapPtr)
                        {
                            AddStore(i, hardPoint[i]->weaponId, (visFlag bitand (1 << i)));
                            weapPtr = weapPtr->GetNextOnRail();
                        }

                        firstPtr = weapPtr = hardPoint[i]->podPointer;

                    }
                    else
                        firstPtr = weapPtr = hardPoint[i]->weaponPointer;

                    while (weapPtr)
                    {
                        // determine the correct slot (j) to attach to,
                        // depending on which BSP pointers are valid.
                        if (rackBSP)
                        {
                            j = weapPtr->GetRackSlot();

                            if (j >= hardPoint[i]->NumPoints())
                                j = hardPoint[i]->NumPoints() - 1;

                            rackBSP->GetChildOffset(j, &simView);

                            hardPoint[i]->SetSubPosition(j, xOff + simView.x, yOff + simView.y, zOff + simView.z);
                            hardPoint[i]->SetSubRotation(j, 0.0F, 0.0F);
                        }
                        else
                        {
                            simView.x = simView.y = simView.z = 0.0F;

                            if (pylonBSP)
                                j = 0;
                            else
                                j = i - 1; // is a/c
                        }

                        // update the rack slot
                        weapPtr->SetRackSlot(j);

                        // if we don't have a rackBSP, then we can only attach 1 weapon
                        if (visFlag bitand (1 << i) and 
                            (rackBSP or weapPtr == firstPtr))
                        {
                            OTWDriver.CreateVisualObject(weapPtr);
                            OTWDriver.AttachObject(parentBSP, (DrawableBSP*)(weapPtr->drawPointer), j);

                            if (hardPoint[i]->podPointer)
                                ((DrawableBSP*)(weapPtr->drawPointer))->SetSwitchMask(0, 1);
                        }

                        AddStore(i, hardPoint[i]->weaponId, (visFlag bitand (1 << i)));

                        if (weapPtr->IsGun())  // init  gun pod // MLR 1/28/2004 -
                        {
                            ((GunClass*)weapPtr)->InitTracers();
                            //((GunClass*)weapPtr)->SetPosition(simView.x, simView.y, simView.z,0,0);
                        }

                        weapPtr = weapPtr->GetNextOnRail();
                    }
                }
            }
        }
    }
}
#endif

// MLR rewrite
void SMSClass::FreeWeaponGraphics(void)
{
    int i;
    DrawableBSP *drawPtr   = (DrawableBSP*) ownship->drawPointer;
    DrawableBSP *rackBSP   = NULL,
                 *pylonBSP  = NULL;
    DrawableBSP *parentBSP = drawPtr; // parent of the weapon
    SimWeaponClass *weapPtr;

    for (i = 0; i < numHardpoints; i++)
    {
        if (hardPoint[i])
        {
            // Detach all weapons from the rack and remove drawables
            weapPtr = hardPoint[i]->weaponPointer.get();

            while (weapPtr)
            {
                if (weapPtr->IsMissile())
                {
                    ((MissileClass *)weapPtr)->SetTarget(NULL);
                    ((MissileClass *)weapPtr)->ClearReferences();
                }
                else if (weapPtr->IsBomb())
                    ((BombClass *)weapPtr)->SetTarget(NULL);

                RemoveStore(i, hardPoint[i]->weaponId);

                if (weapPtr->drawPointer)
                {
                    hardPoint[i]->DetachWeaponBSP(weapPtr);
                    //OTWDriver.DetachObject(parentBSP, (DrawableBSP*)(weapPtr->drawPointer), weapPtr->GetRackSlot());
                    OTWDriver.RemoveObject(weapPtr->drawPointer, TRUE); // MLR is this needed?
                    weapPtr->drawPointer = NULL;
                }

                weapPtr = weapPtr->GetNextOnRail();
            }

            // MLR 1/11/2004 - Free the pods, if any
            /*
            weapPtr = hardPoint[i]->podPointer;
            while (weapPtr)
            {
             if ( weapPtr->IsMissile() ) // this code could be trimmed out
             {
             ((MissileClass *)weapPtr)->SetTarget( NULL );
             ((MissileClass *)weapPtr)->ClearReferences();
             }
             else if ( weapPtr->IsBomb() )
             ((BombClass *)weapPtr)->SetTarget( NULL );

             RemoveStore(i, hardPoint[i]->weaponId);
             if (weapPtr->drawPointer)
             {
             hardPoint[i]->DetachWeaponBSP(weapPtr);
             //OTWDriver.DetachObject(parentBSP, (DrawableBSP*)(weapPtr->drawPointer), weapPtr->GetRackSlot());
             OTWDriver.RemoveObject(weapPtr->drawPointer, TRUE); // MLR is this needed?
             weapPtr->drawPointer = NULL;
             }
             weapPtr = weapPtr->GetNextOnRail();
            }
            */

            // remove the pylon bitand rack

            // 1st determine what the rack is attached to
            parentBSP = drawPtr;
            int rackslotid = i - 1;

            if (pylonBSP)
            {
                // set these up for the rack
                parentBSP  = pylonBSP;
                rackslotid = 0;
            }

            rackBSP = hardPoint[i]->DetachRackBSP();

            if (rackBSP)
            {
                //OTWDriver.DetachObject(parentBSP, rackBSP, rackslotid);
                RemoveStore(i, hardPoint[i]->GetRackId());
                OTWDriver.RemoveObject(rackBSP, TRUE); // deletes object too
                //hardPoint[i]->SetRack(NULL);
            }

            pylonBSP = hardPoint[i]->DetachPylonBSP();

            if (pylonBSP)
            {
                //OTWDriver.DetachObject(drawPtr, pylonBSP, i-1);
                RemoveStore(i, hardPoint[i]->GetPylonId());
                OTWDriver.RemoveObject(pylonBSP, TRUE); // deletes object too
                //hardPoint[i]->SetPylon(NULL);
            }

            if (GetGun(i))
                GetGun(i)->CleanupTracers();
        }
    }
}

void SMSClass::Exec(void)
{
    int i;
    GunClass* gun;

    RunRockets();

    // 2002-01-27 ADDED BY S.G. Reset curRippleCount if the current weapon is NOT a bomb...
    if (curRippleCount and curWeapon and not curWeapon->IsBomb())
    {
        curRippleCount = 0;
        nextDrop = 0;
    }

    // END OF ADDED SECTION

    // Do ripple stuff here
    if (curRippleCount and SimLibElapsedTime > nextDrop)
    {
        if (GetAGBPair() and numCurrentWpn <= 1)
            curRippleCount = 1;
        else
            DropBomb();

        curRippleCount--;

        if (curRippleCount)
            // 2001-04-27 MODIFIED BY S.G. SO THE RIPPLE DISTANCE IS INCREASED WITH ALTITUDE (SINCE BOMBS SLOWS DOWN)
            //         nextDrop = SimLibElapsedTime + FloatToInt32((rippleInterval)/(float)sqrt(ownship->XDelta()*ownship->XDelta() + ownship->YDelta()*ownship->YDelta()) * SEC_TO_MSEC + (float)sqrt(ownship->ZPos() * -4.0f));
            // JB 011017 Use old nextDrop ripple code instead of altitude dependant code.  I don't see why bombs slowing down would have an effect on ripple distance
            // If anything is effected it would cause bombs to fall short which is not happening.
            // Besides this really screws up the code that decides when to start the ripple, and all the bombs don't fall off properly.
            //nextDrop = SimLibElapsedTime + FloatToInt32((rippleInterval)/
            nextDrop = SimLibElapsedTime + FloatToInt32((GetAGBRippleInterval()) /
                       (float)sqrt(ownship->XDelta() * ownship->XDelta() + ownship->YDelta() * ownship->YDelta()) * SEC_TO_MSEC);
        else
        {
            nextDrop = 0;

            // Make sure we can fire in the future
            ClearFlag(Firing);
        }

    }

    //MI Mav cooling
    if (g_bRealisticAvionics)
    {
        if (Powered and MavCoolTimer >= -1.0F)
            MavCoolTimer -= SimLibMajorFrameTime;
        else if ( not Powered and MavCoolTimer <= 5.0F)
            MavCoolTimer += SimLibMajorFrameTime;
    }

    if (g_bRealisticAvionics)//Cobra for the JDAM
    {
        if (JDAMPowered and JDAMInitTimer >= -1.0f)
            JDAMInitTimer -= SimLibMajorFrameTime;
        else if ( not JDAMPowered and JDAMInitTimer < 10.0f)
            JDAMInitTimer = 10.0f;
    }

    // RV - I-Hawk - For HARM initial power-up
    if (g_bRealisticAvionics)
    {
        if (GetHARMPowerState() and GetHARMInitTimer() >= -1.0f)
        {
            HARMInitTimer -= SimLibMajorFrameTime;
        }
        else if ( not GetHARMPowerState() and GetHARMInitTimer() < 5.0f)
        {
            HARMInitTimer = 2.5f;
        }
    }

    // Marco Edit - Check overall cooling amount left
    if (GetCoolState() == COOLING)
    {
        // JPO aim9 is cooling down
        aim9cooltime -= SimLibMajorFrameTime;

        if (aim9cooltime <= 0.0F)
        {
            SetCoolState(COOL);
            aim9cooltime = 0.0F;
        }
    }

    if (GetCoolState() == COOL or GetCoolState() == COOLING)
    {
        aim9coolingtimeleft -= SimLibMajorFrameTime;
        aim9warmtime -= SimLibMajorFrameTime;

        if (aim9warmtime <= 0.0F)
            aim9warmtime = 0.0F;
    }

    // Marco Edit - Warming up back to normal
    if (GetCoolState() == WARMING)
    {
        aim9warmtime += SimLibMajorFrameTime;
        aim9cooltime += SimLibMajorFrameTime; //reset our cooling timer.

        if (aim9cooltime > 3.0F)
            aim9cooltime = 3.0F;

        if (aim9warmtime >= 60.0F)
        {
            SetCoolState(WARM);
            aim9cooltime = 3.0F; //reset our cooling timer.
        }
    }

    // We're out of coolant We start to warm up....
    if (aim9coolingtimeleft <= 0 and GetCoolState() not_eq WARM)
        SetCoolState(WARMING);

    // Okay, we've warmed up enough....
    // can you say dead weight???
    /*else if(aim9coolingtimeleft == 0 and SimLibElapsedTime > aim9warmtime)
    {
     SetCoolState(WARM);
     aim9warmtime = 0;
     aim9cooltime = 0;
    }*/

    if (drawable)
    {
        drawable->UpdateGroundSpot();

        if (ownship and // JB 010710 CTD?
 not ownship->OnGround())
        {
            drawable->frameCount += FloatToInt32(SimLibMajorFrameTime * SEC_TO_MSEC * 0.1F);
        }
    }

    // Count how many of these we have
    if (curHardpoint >= 0)
    {
        numCurrentWpn = 0;

        if (curWeaponClass == wcGunWpn)
        {
            gun = GetGun(curHardpoint);

            if (gun)
                numCurrentWpn = gun->numRoundsRemaining / 10;
        }
        else
        {
            for (i = 0; i < numHardpoints; i++)
            {
                if (hardPoint[i] and hardPoint[i]->weaponId == curWeaponId)
                {
                    numCurrentWpn += hardPoint[i]->weaponCount;
                }
            }
        }
    }
    else
    {
        numCurrentWpn = -1;
    }
}

void SMSClass::SetPlayerSMS(int flag)
{
    if (flag and not drawable)
    {
        drawable = new SmsDrawable(this);
    }

    else if ( not flag)
    {
        delete drawable;
        drawable = NULL;
    }
}

void SMSClass::FreeWeapons(void)
{
    int i;

    // KCK: The actual deletion of this stuff is done in the base class destructor
    ReleaseCurWeapon(-1);

    for (i = 0; i < numHardpoints; i++)
        if (hardPoint[i])
            hardPoint[i]->SetGun(NULL);
}

void SMSClass::SetWeaponType(WeaponType newType)
{
    curWeaponType = newType;
}

void SMSClass::IncrementStores(WeaponClass wClass, int count)
{
    ShiAssert(wClass <= wcNoWpn and wClass >= 0);
    numOnBoard[wClass] += count;
    ShiAssert(numOnBoard[wClass] >= 0);

    if (numOnBoard[wClass] < 0)
        numOnBoard[wClass] = 0;
}

void SMSClass::DecrementStores(WeaponClass wClass, int count)
{
    ShiAssert(wClass <= wcNoWpn and wClass >= 0);
    numOnBoard[wClass] -= count;
    ShiAssert(numOnBoard[wClass] >= 0);

    if (numOnBoard[wClass] < 0)
        numOnBoard[wClass] = 0;
}


void SMSClass::SelectiveJettison(void)
{
    int curStation;
    int jettSuccess = 0;

    if (drawable)
    {
        for (curStation = numHardpoints - 1; curStation > 0; curStation--)
        {
            //if(drawable->hardPointSelected bitand (1 << curStation) and MasterArm() not_eq Safe)
            if (drawable->sjSelected[curStation] not_eq JettisonNone and MasterArm() not_eq Safe)
            {
                MonoPrint("Jettison station %d at %ld\n", curStation, SimLibElapsedTime);
                ReleaseCurWeapon(-1);
                jettSuccess = JettisonStation(curStation, drawable->sjSelected[curStation]);  // MLR 3/2/2004 -

                if (jettSuccess)
                    drawable->hardPointSelected -= (1 << curStation);

                if (ownship->IsLocal() and jettSuccess)
                {
                    // Create and fill in the message structure
                    FalconTrackMessage* trackMsg = new FalconTrackMessage(1, ownship->Id(), FalconLocalGame);
                    trackMsg->dataBlock.trackType = Track_JettisonWeapon;
                    trackMsg->dataBlock.hardpoint = (ushort)curStation;
                    trackMsg->dataBlock.id = ownship->Id();

                    // Send our track list
                    FalconSendMessage(trackMsg, TRUE);
                }
            }
        }
    }

    /*   // Can't jettison the gun
     if (curHardpoint > 0)
     {
    // MonoPrint ("Jettison station %d\n", curStation);
     ReleaseCurWeapon (-1);
     JettisonStation (curHardpoint);
     }
       */
}

void SMSClass::JettisonWeapon(int hp)
{
    ReleaseCurWeapon(-1);
    JettisonStation(hp, SelectiveRack);  // MLR 3/2/2004 -
}

void SMSClass::EmergencyJettison(void)
{
    int curStation;
    int jettSuccess = 0;

    //me123 make sure we don't keep doing this...a mp messages is tranmitted every time.
    if (flags bitand EmergencyJettisonFlag) return;

    //MI
    if ( not g_bRealisticAvionics)
    {
        if (ownship->OnGround())
            return;
    }
    else
    {
        if (ownship->OnGround() and not GndJett)
            return;
    }

    for (curStation = numHardpoints - 1; curStation > 0; curStation--)
    {
        // OW Jettison fix
        /*
         if( not (((AircraftClass *)ownship)->IsF16() and 
                 (curStation == 1 or curStation == 9 or hardPoint[curStation]->GetWeaponClass() == wcECM)) and 
                 hardPoint[curStation]->GetRack())
        */
        // 2002-04-21 MN this fixes release of AA weapons for F-16's, but other aircraft drop all the stuff -> crap
        // new code by Pogo - just check not to drop station 1 and 9 and no AA and ECM for F-16's, no AA and ECM for all

        if ( not g_bEmergencyJettisonFix)
        {
            if (hardPoint[curStation] and ( not (((AircraftClass *)ownship)->IsF16() and 
                                            (curStation == 1 or curStation == 9 or hardPoint[curStation]->GetWeaponClass() == wcECM or hardPoint[curStation]->GetWeaponClass() == wcAimWpn or hardPoint[curStation]->GetWeaponClass() == wcHARMWpn)) and 
                                          (hardPoint[curStation]->GetRack() or curStation == 5 and hardPoint[curStation]->GetWeaponClass() == wcTank)))//me123 in the line above addet a check so we don't emergency jettison a-a missiles

            {
                MonoPrint("Jettison station %d at %ld\n", curStation, SimLibElapsedTime);
                ReleaseCurWeapon(-1);
                jettSuccess = JettisonStation(curStation, Emergency);  // MLR 3/2/2004 -
            }
        }
        else
        {

            if ( not (hardPoint[curStation]->GetRackDataFlags() bitand RDF_BMSDEFINITION))
            {
                if (((AircraftClass *)ownship)->IsF16())
                {
                    if ( not (curStation == 1 or curStation == 9 or hardPoint[curStation]->GetWeaponClass() == wcECM or hardPoint[curStation]->GetWeaponClass() == wcAimWpn or hardPoint[curStation]->GetWeaponClass() == wcHARMWpn) and 
                        (hardPoint[curStation]->GetRack() or curStation == 5 and hardPoint[curStation]->GetWeaponClass() == wcTank))
                    {
                        ReleaseCurWeapon(-1);
                        jettSuccess = JettisonStation(curStation, Emergency);  // MLR 3/2/2004 -
                    }
                }
                else
                {
                    if (hardPoint[curStation] and not (hardPoint[curStation]->GetWeaponClass() == wcECM or
                                                   hardPoint[curStation]->GetWeaponClass() == wcAimWpn or hardPoint[curStation]->GetWeaponClass() == wcHARMWpn))
                    {
                        ReleaseCurWeapon(-1);
                        jettSuccess = JettisonStation(curStation, Emergency);  // MLR 3/2/2004 -
                    }

                }
            }
            else
            {
                ReleaseCurWeapon(-1);
                jettSuccess = JettisonStation(curStation, Emergency);  // MLR 3/2/2004 -
            }
        }
    }

    if (ownship->IsLocal())
    {
        // Create and fill in the message structure
        FalconTrackMessage* trackMsg = new FalconTrackMessage(1, ownship->Id(), FalconLocalGame);
        trackMsg->dataBlock.trackType = Track_JettisonAll;
        trackMsg->dataBlock.hardpoint = 0;
        trackMsg->dataBlock.id = ownship->Id();

        // Send our track list
        FalconSendMessage(trackMsg, TRUE);
    }

    // Set a permanent flag indicating that we've done the deed
    if (jettSuccess)
        flags or_eq EmergencyJettisonFlag;
}


void SMSClass::AGJettison(void)
{
    int curStation;
    int jettSuccess = 0;


    for (curStation = numHardpoints - 1; curStation > 0; curStation--)
    {
        // FRB - SEAD a/c need to keep their Harms
        if (hardPoint[curStation]->GetWeaponClass() == wcHARMWpn)
            continue;

        // MLR-NOTE GetRack??? should prevent A-10 from Jetting???
        if (hardPoint[curStation] and hardPoint[curStation]->GetRack() and (hardPoint[curStation]->Domain() bitand wdGround))
        {
            // MonoPrint ("Jettison station %d at %ld\n", curStation, SimLibElapsedTime);
            ReleaseCurWeapon(-1);
            jettSuccess = JettisonStation(curStation, SelectiveRack);  // MLR 3/2/2004 -
        }
    }

    // Set a permanent flag indicating that we've done the deed
    if (jettSuccess)
        flags or_eq EmergencyJettisonFlag;
}

// 2002-02-20 ADDED BY S.G. Will jettison the tanks if empty.
void SMSClass::TankJettison(void)
{
    int curStation;
    int jettSuccess = 0;

    if ( not (flags bitand TankJettisonFlag) and ownship->IsAirplane() and ((AircraftClass*)ownship)->af->ExternalFuel() < 0.1f)   // We're an airplane and our external fuel is almost zero (to trap floating point precision error), then jettison the tanks
    {
        for (curStation = numHardpoints - 1; curStation > 0; curStation--)
        {
            if (hardPoint[curStation] and hardPoint[curStation]->GetWeaponClass() == wcTank)
            {
                // MonoPrint ("Jettison station %d at %ld\n", curStation, SimLibElapsedTime);
                jettSuccess = JettisonStation(curStation, SelectiveRack);  // MLR 3/2/2004 -
            }
        }

        // Set a permanent flag indicating that we've done the deed
        if (jettSuccess)
            flags or_eq TankJettisonFlag;
    }
}

void SMSClass::ResetCurrentWeapon(void)
{
    curHardpoint = lastWpnStation;
    curWpnNum    = lastWpnNum;

    if (curHardpoint >= 0)
    {
        curWeapon = hardPoint[curHardpoint]->weaponPointer;

        // sfr: test
        if (curWeapon and not curWeapon->IsMissile())
        {
            printf("bug bug bug");
        }

        curWeaponType  = hardPoint[curHardpoint]->GetWeaponType();
        curWeaponClass = hardPoint[curHardpoint]->GetWeaponClass();
        curWeaponId    = hardPoint[curHardpoint]->weaponId;
        curWeaponDomain = hardPoint[curHardpoint]->Domain();

        if (curWeapon and curWeaponClass == wcHARMWpn)
        {
            ((MissileClass*)curWeapon.get())->display = FindSensor(ownship, SensorClass::HTS);
        }
    }
    else
    {
        curWeapon.reset();
        curWeaponType  = wtNone;
        curWeaponClass = wcNoWpn;
        curWeaponId    = -1;
        curWeaponDomain = wdNoDomain;
        // 2001-08-04 ADDED BY S.G. I THINK WE WANT TO SET curHardpoint TO -1 SINCE WE ARE CLEARING IT, RIGHT?
        //   curHardpoint = -1;
        //   lastWpnStation = -1;
    }
}

// This allows us to look at a specific weapon on a hardpoint
void SMSClass::SetCurrentWeapon(int station, SimWeaponClass *weapon)
{
    lastWpnStation = curHardpoint;
    lastWpnNum     = curWpnNum;

    if (station >= 0)
    {
        if ( not weapon) // MLR 2/1/2004 -
        {
            weapon = hardPoint[station]->weaponPointer.get();

            while (weapon and not weapon->IsUseable()) // MLR 6/4/2004 -
            {
                weapon = weapon->GetNextOnRail();
            }
        }

        curHardpoint   = station;

        // COBRA - RED - A NEW CORRECTION, RESET WEAPON PARAMETERS IF NO WEAPON...
        if (weapon)
        {
            // sfr: test
            if (weapon and not weapon->IsWeapon())
            {
                printf("bug bug bug");
            }

            curWpnNum   = weapon->GetRackSlot();
            curWeapon.reset(weapon);
            curWeaponType  = hardPoint[station]->GetWeaponType();
            curWeaponClass = hardPoint[station]->GetWeaponClass();
            curWeaponId    = hardPoint[station]->weaponId;
            curWeaponDomain = hardPoint[station]->Domain();
        }
        else
        {
            curWpnNum   = -1;
            curWeapon.reset();
            curWeaponType  = wtNone;
            curWeaponClass = wcNoWpn;
            curWeaponId    = -1;
            curWeaponDomain = wdNoDomain;
        }

        if (curWeapon and (curWeaponClass == wcHARMWpn))
        {
            ((MissileClass*)curWeapon.get())->display = FindSensor(ownship, SensorClass::HTS);
        }
    }
    else
    {
        curWeapon.reset();
        curWeaponType  = wtNone;
        curWeaponClass = wcNoWpn;
        curWeaponId    = -1;
        curWeaponDomain = wdNoDomain;
    }

    //MI since this get's called AFTER the ChooseLimiterMode() function above when launching AG
    //missiles, we're not checking with the correct loadout. So let's just check here again.
    //This fixes the bug with AG missiles and OverG when we only have the rack left
    ChooseLimiterMode(1);
}

// Find the next weapon of the class and type.  Start with
// the next point on the current station, and wrap around
// back to the current point on the current station.


int NextACHp(int curHp, int hpCount)
{
    // assumes 0 is gun
    float middleHp;
    int   newHp;

    //           |
    // 0 1 2 3 4 5 6 7 8 9
    // 5 9 8 7 6 4 3 2 1 0

    // 0 1 2 3 4|5 6 7 8
    // 4 8 7 6 5 3 4 1 0

    middleHp = (hpCount) / 2.0f;

    if (curHp == 0)
    {
        return((int)middleHp);
    }

    if (curHp == middleHp)
    {
        newHp = (int)(middleHp - 1);

        if (newHp >= 0)
            return newHp;

        return 0;
    }

    if (curHp < middleHp) // goto opposite Hp
        return(hpCount - curHp);

    if (curHp > middleHp) // goto opposite Hp
        return(hpCount - curHp - 1);

    return(1); // start over
}


int SMSClass::WeaponStep(int symFlag)
{
    int i, idDesired;
    int stationUnderTest;
    WeaponClass classDesired;
    WeaponType typeDesired;
    SimWeaponClass *weapPtr, *found = NULL;

    if (ownship->IsAirplane())
    {
        if (curHardpoint > 0)
        {
            int initialHp = curHardpoint;
            int nextHp    = NextACHp(initialHp, numHardpoints);

            while (1)
            {
                if (hardPoint[nextHp]->weaponId == hardPoint[initialHp]->weaponId)
                {
                    SimWeaponClass *weap = hardPoint[nextHp]->weaponPointer.get();

                    // skip unuseable weapons
                    while (weap and not weap->IsUseable())
                    {
                        weap = weap->GetNextOnRail();
                    }

                    if (weap)
                    {
                        SetCurrentWeapon(nextHp, weap);
                        return 1;
                    }
                }

                if (nextHp == initialHp)
                {
                    // we searched all Hps, and didn't find a match.
                    SetCurrentWeapon(nextHp);
                    return 0;
                }

                nextHp = NextACHp(nextHp, numHardpoints);
            }
        }

        return 0;
    }

    // old code follows

    if (curHardpoint < 0)
    {
        // Do nothing if no station is currently selected
        return 0;
    }

    // Symetric or same?
    if ( not symFlag)
    {
        stationUnderTest = curHardpoint;
        i = curWpnNum;
    }
    else
    {
        stationUnderTest = numHardpoints - 1;
        i = -1;
    }

    // Otherwise, start with next weapon on this hardpoint
    ReleaseCurWeapon(curHardpoint);

    // KCK: Why are we returning here? I guess we can't step to our next gun for
    // multiple gun vehicles...
    if ( not hardPoint[curHardpoint] or hardPoint[curHardpoint]->GetWeaponClass() == wcGunWpn)
    {
        return 0;
    }

    classDesired = hardPoint[curHardpoint]->GetWeaponClass();
    typeDesired = hardPoint[curHardpoint]->GetWeaponType();
    idDesired = hardPoint[curHardpoint]->weaponId;

    // First try and find next weapon on the current hardpoint
    if (i >= 0)
    {
        weapPtr = hardPoint[curHardpoint]->weaponPointer.get();

        while (weapPtr)
        {
            if (weapPtr->GetRackSlot() > i and weapPtr->IsUseable())
            {
                // MLR 3/6/2004 - added IsUseable (for rockets at this point)
                found = weapPtr;
                break;
            }

            weapPtr = weapPtr->GetNextOnRail();
        }
    }

    // Next look for anything w/ the same weapon Id
    if ( not found)
    {
        if ( not symFlag)
        {
            for (i = 0; i < numHardpoints; i++)
            {
                stationUnderTest = (i + 1 + curHardpoint) % numHardpoints;

                if (hardPoint[stationUnderTest] and hardPoint[stationUnderTest]->weaponId == idDesired)
                {
                    if (hardPoint[stationUnderTest]->weaponPointer and hardPoint[stationUnderTest]->weaponPointer->IsUseable())
                    {
                        // MLR 3/6/2004 - added IsUseable (for rockets at this point)
                        found = hardPoint[stationUnderTest]->weaponPointer.get();
                        break;
                    }
                }
            }
        }
        else
        {
            if (curHardpoint > numHardpoints / 2)
            {
                for (i = 0; i < numHardpoints; i++)
                {
                    stationUnderTest = i;

                    if (hardPoint[stationUnderTest] and hardPoint[stationUnderTest]->weaponId == idDesired)
                    {
                        if (hardPoint[stationUnderTest]->weaponPointer and hardPoint[stationUnderTest]->weaponPointer->IsUseable())
                        {
                            // MLR 3/6/2004 - added IsUseable (for rockets at this point)
                            found = hardPoint[stationUnderTest]->weaponPointer.get();
                            break;
                        }
                    }
                }
            }
            else
            {
                for (i = numHardpoints - 1; i >= 0; i--)
                {
                    stationUnderTest = i;

                    if (hardPoint[stationUnderTest] and hardPoint[stationUnderTest]->weaponId == idDesired)
                    {
                        if (hardPoint[stationUnderTest]->weaponPointer and hardPoint[stationUnderTest]->weaponPointer->IsUseable())
                        {
                            found = hardPoint[stationUnderTest]->weaponPointer.get();
                            break;
                        }
                    }
                }
            }
        }
    }

    // Next try and find first weapon of the same class on any other hardpoint
    if ( not found and classDesired not_eq wcGbuWpn)
    {
        for (i = 0; i < numHardpoints; i++)
        {
            stationUnderTest = (i + 1 + curHardpoint) % numHardpoints;

            if (hardPoint[stationUnderTest] and hardPoint[stationUnderTest]->GetWeaponClass() == classDesired)
            {
                if (hardPoint[stationUnderTest]->GetWeaponType() == typeDesired and 
                    hardPoint[stationUnderTest]->weaponPointer and 
                    hardPoint[stationUnderTest]->weaponPointer->IsUseable())
                {
                    // MLR 3/6/2004 - added IsUseable (for rockets at this point)
                    found = hardPoint[stationUnderTest]->weaponPointer.get();
                    break;
                }
            }
        }
    }

    if (found or classDesired == wcHARMWpn)
    {
        // JB 010726 Stick with the HARM
        SetCurrentWeapon(stationUnderTest, found);
    }
    else
    {
        SetCurrentWeapon(-1, NULL);
    }

    return found ? 1 : 0;
}


#if 0
void SMSClass::WeaponStep(int symFlag)
{
    int i, idDesired;
    int stationUnderTest;
    WeaponClass classDesired;
    WeaponType typeDesired;
    SimWeaponClass *weapPtr, *found = NULL;

    if (curHardpoint < 0)
    {
        // Do nothing if no station is currently selected
        return;
    }

    // Symetric or same?
    if ( not symFlag)
    {
        stationUnderTest = curHardpoint;
        i = curWpnNum;
    }
    else
    {
        stationUnderTest = numHardpoints - 1;
        i = -1;
    }

    // Otherwise, start with next weapon on this hardpoint
    ReleaseCurWeapon(curHardpoint);

    // KCK: Why are we returning here? I guess we can't step to our next gun for
    // multiple gun vehicles...
    if ( not hardPoint[curHardpoint] or hardPoint[curHardpoint]->GetWeaponClass() == wcGunWpn)
        return;

    classDesired = hardPoint[curHardpoint]->GetWeaponClass();
    typeDesired = hardPoint[curHardpoint]->GetWeaponType();
    idDesired = hardPoint[curHardpoint]->weaponId;

    // First try and find next weapon on the current hardpoint
    if (i >= 0)
    {
        weapPtr = hardPoint[curHardpoint]->weaponPointer;

        while (weapPtr)
        {
            if (weapPtr->GetRackSlot() > i and weapPtr->IsUseable())
                // MLR 3/6/2004 - added IsUseable (for rockets at this point)
            {
                found = weapPtr;
                break;
            }

            weapPtr = weapPtr->GetNextOnRail();
        }
    }

    // Next look for anything w/ the same weapon Id
    if ( not found)
    {
        if ( not symFlag)
        {
            for (i = 0; i < numHardpoints; i++)
            {
                stationUnderTest = (i + 1 + curHardpoint) % numHardpoints;

                if (hardPoint[stationUnderTest] and hardPoint[stationUnderTest]->weaponId == idDesired)
                {
                    if (hardPoint[stationUnderTest]->weaponPointer and hardPoint[stationUnderTest]->weaponPointer->IsUseable())
                        // MLR 3/6/2004 - added IsUseable (for rockets at this point)
                    {
                        found = hardPoint[stationUnderTest]->weaponPointer;
                        break;
                    }
                }
            }
        }
        else
        {
            if (curHardpoint > numHardpoints / 2)
            {
                for (i = 0; i < numHardpoints; i++)
                {
                    stationUnderTest = i;

                    if (hardPoint[stationUnderTest] and hardPoint[stationUnderTest]->weaponId == idDesired)
                    {
                        if (hardPoint[stationUnderTest]->weaponPointer and hardPoint[stationUnderTest]->weaponPointer->IsUseable())
                            // MLR 3/6/2004 - added IsUseable (for rockets at this point)
                        {
                            found = hardPoint[stationUnderTest]->weaponPointer;
                            break;
                        }
                    }
                }
            }
            else
            {
                for (i = numHardpoints - 1; i >= 0; i--)
                {
                    stationUnderTest = i;

                    if (hardPoint[stationUnderTest] and hardPoint[stationUnderTest]->weaponId == idDesired)
                    {
                        if (hardPoint[stationUnderTest]->weaponPointer and hardPoint[stationUnderTest]->weaponPointer->IsUseable())
                        {
                            found = hardPoint[stationUnderTest]->weaponPointer;
                            break;
                        }
                    }
                }
            }
        }
    }

    // Next try and find first weapon of the same class on any other hardpoint
    if ( not found and classDesired not_eq wcGbuWpn)
    {
        for (i = 0; i < numHardpoints; i++)
        {
            stationUnderTest = (i + 1 + curHardpoint) % numHardpoints;

            if (hardPoint[stationUnderTest] and hardPoint[stationUnderTest]->GetWeaponClass() == classDesired)
            {
                if (hardPoint[stationUnderTest]->GetWeaponType() == typeDesired and 
                    hardPoint[stationUnderTest]->weaponPointer and 
                    hardPoint[stationUnderTest]->weaponPointer->IsUseable()) // MLR 3/6/2004 - added IsUseable (for rockets at this point)
                {
                    found = hardPoint[stationUnderTest]->weaponPointer;
                    break;
                }
            }
        }
    }

    if (found or classDesired == wcHARMWpn) // JB 010726 Stick with the HARM
        SetCurrentWeapon(stationUnderTest, found);
    else
        SetCurrentWeapon(-1, NULL);
}
#endif

// This looks like it'll find the next weapon of the type passed (description index)
int SMSClass::FindWeapon(int indexDesired)
{
    int i = 0;
    int stationUnderTest = 0;
    SimWeaponClass *weapPtr = NULL, *found = NULL;

    if (curHardpoint >= 0)
    {
        ReleaseCurWeapon(curHardpoint);

        if (hardPoint[curHardpoint]->weaponPointer and hardPoint[curHardpoint]->weaponPointer->Type() == indexDesired)
        {
            // Try and get the next weapon on current hardpoint
            weapPtr = hardPoint[curHardpoint]->weaponPointer->GetNextOnRail();

            if (weapPtr)
            {
                stationUnderTest = curHardpoint;
                found = weapPtr;
            }
        }
    }

    // Try and get the first weapon on any other hardpoint
    if ( not found)
    {
        for (i = 0; i < numHardpoints; i++)
        {
            stationUnderTest = (i + 1 + curHardpoint) % numHardpoints;

            if (hardPoint[stationUnderTest] and hardPoint[stationUnderTest]->weaponPointer and 
                hardPoint[stationUnderTest]->weaponPointer->Type() == indexDesired and 
                hardPoint[stationUnderTest]->weaponPointer->IsUseable()) // MLR 3/6/2004 - IsUseable()
            {
                found = hardPoint[stationUnderTest]->weaponPointer.get();
                break;
            }
        }
    }

    if (found)
        SetCurrentWeapon(stationUnderTest, found);
    else
        SetCurrentWeapon(-1, NULL);

    if (found)
        return 1;

    return 0;
}

int SMSClass::HasWeaponClass(WeaponClass classDesired)
{
    int i;

    for (i = 0; i < numHardpoints; i++)
    {
        if (hardPoint[i] and hardPoint[i]->GetWeaponClass() == classDesired and hardPoint[i]->weaponPointer)
        {
            break;
        }
    }

    return (i < numHardpoints ? TRUE : FALSE);
}

// Find the next weapon of the desired class.  Start with
// the next point on the current station, and wrap around
// back to the current point on the current station.
int SMSClass::FindWeaponClass(WeaponClass weaponDesired, int needWeapon)
{
    int i, retval;
    int stationUnderTest;
    SimWeaponClass *weapPtr, *found = NULL;
    int notNeedStation = -1;
    int foundStation = -1;

    // Release the current weapon;
    if (curHardpoint >= 0)
    {
        ReleaseCurWeapon(curHardpoint);

        // Do we have the right thing on the current station?
        weapPtr = hardPoint[curHardpoint]->weaponPointer.get();

        if (hardPoint[curHardpoint]->GetWeaponClass() == weaponDesired and weapPtr and weapPtr->IsUseable())
        {
            foundStation = curHardpoint;
            found = weapPtr;
        }
    }

    // Try and get the first weapon on any other hardpoint
    if ( not found)
    {
        for (i = 0; i < numHardpoints; i++)
        {
            stationUnderTest = (i + 1 + curHardpoint) % numHardpoints;

            if (hardPoint[stationUnderTest] and hardPoint[stationUnderTest]->GetWeaponClass() == weaponDesired)
            {
                if ( not found)
                {
                    weapPtr = hardPoint[stationUnderTest]->weaponPointer.get();

                    while (weapPtr and not weapPtr->IsUseable()) // skip unusable stores
                    {
                        weapPtr = weapPtr->GetNextOnRail();
                    }

                    found  = weapPtr;
                    foundStation = stationUnderTest;
                }
                else if (notNeedStation < 0)
                {
 notNeedStation = stationUnderTest;
                }
            }
        }
    }

    // Report status
    if (found)
    {
        // Found one
        SetCurrentWeapon(foundStation, found);
        retval = TRUE;
    }
    else if ( not needWeapon and notNeedStation not_eq -1)
    {
        // Found where one was, and thats good enough
        SetCurrentWeapon(notNeedStation, NULL);
        retval = TRUE;
    }
    else
    {
        // Never had one.
        SetCurrentWeapon(-1, NULL);
        retval = FALSE;
    }

    return retval;
}

// Find the *FIRST* weapon of the desired type.
int SMSClass::FindWeaponType(WeaponType weaponDesired)
{
    int newHp = 0;

    if ( not hardPoint)
        return 0;

    do
    {
        if (hardPoint[newHp]->GetWeaponType() == weaponDesired)
        {
            SimWeaponClass *weap;
            weap = hardPoint[newHp]->weaponPointer.get();

            // find a useable weapon
            while (weap and not weap->IsUseable())
            {
                weap = weap->GetNextOnRail();
            }

            if (weap)
            {
                ReleaseCurWeapon(curHardpoint);
                SetCurrentWeapon(newHp, weap);
                return 1;
            }
        }

        newHp = NextACHp(newHp, this->numHardpoints);
    }
    while (newHp not_eq 0);

    ReleaseCurWeapon(curHardpoint);

    return 0;
}


#if 0
// Find the next weapon of the desired type.  Start with
// the next point on the current station, and wrap around
// back to the current point on the current station.
// Favor a station w/ a weapon

int SMSClass::FindWeaponType(WeaponType weaponDesired)
{
    int i;
    int stationUnderTest;
    SimWeaponClass *weapPtr, *found = NULL;
    int foundStation = -1;
    int emptyStation = -1;

    if (curHardpoint >= 0)
    {
        ReleaseCurWeapon(curHardpoint);

        if (hardPoint[curHardpoint]->GetWeaponType() == weaponDesired and hardPoint[curHardpoint]->weaponPointer)
        {
            // Try and get the next weapon on current hardpoint
            // weapPtr = hardPoint[curHardpoint]->weaponPointer->GetNextOnRail();   // MLR 1/20/2004 - Original
            weapPtr = hardPoint[curHardpoint]->weaponPointer;   // MLR 1/20/2004 - Why get the second weapon on the hardpoint?

            if (weapPtr)
            {
                //stationUnderTest = curHardpoint; // MLR 1/20/2004 -
                foundStation = curHardpoint; // MLR 1/20/2004 -
                found = weapPtr;
            }
        }
    }

    // Try and get the first weapon on any other hardpoint
    if ( not found)
    {
        for (i = 0; i < numHardpoints; i++)
        {
            stationUnderTest = (i + 1 + curHardpoint) % numHardpoints;

            if (hardPoint[stationUnderTest] and hardPoint[stationUnderTest]->GetWeaponType() == weaponDesired)
            {
                if ( not found)
                {
                    found = hardPoint[stationUnderTest]->weaponPointer;
                    foundStation = stationUnderTest;
                }
                else if (emptyStation == -1)
                {
                    emptyStation = stationUnderTest;
                }
                else
                {
                    break;
                }
            }
        }
    }

    if (found)
        SetCurrentWeapon(foundStation, found);
    else if (emptyStation >= 0)
        SetCurrentWeapon(emptyStation, NULL);
    else
        SetCurrentWeapon(-1, NULL);

    if (found)
        return 1;

    return 0;
}
#endif

void SMSClass::ReleaseCurWeapon(int newStation)
{
    MissileClass* theMissile;

    if (curWeapon)
    {
        switch (curWeaponType)
        {
            case wtAgm65:
            case wtAgm88:
                theMissile = (MissileClass*)curWeapon.get();

                if (theMissile->display)
                {
                    theMissile->display->DisplayExit();
                }

            case wtGBU:
            {
                // MN blind shot from JPO - does this fix the LGB crash/hardlock ?
                // MN commented back in - I think not performing the DisplayExit will result in memory leaks,
                // as each missile seems to have its own display initialised
                SensorClass* laserPod = FindLaserPod(ownship);

                if (laserPod)
                {
                    if (laserPod->GetDisplay())
                    {
                        laserPod->DisplayExit();
                    }
                }
            }
            break;
        }
    }

    curHardpoint = newStation;
    curWpnNum = -1;
    curWeapon.reset();
    curWeaponType = wtNone;
    curWeaponClass = wcNoWpn;
    curWeaponId   = -1;
}

// This finds, selects, and returns the next type of weapon of the desired domain inclusive
WeaponType SMSClass::GetNextWeapon(WeaponDomain domainDesired)
{
    WeaponType newType = curWeaponType;
    FireControlComputer *FCC = ownship->GetFCC();
    int i = 0;
    int stationUnderTest = 0;

    // Don't do this if we're in the process of firing a weapon
    if (IsSet(Firing))
    {
        return curWeaponType;
    }

    // Find the next hardpoint with a weapon of the desired type on it
    for (i = 0; i < numHardpoints; i++)
    {
        stationUnderTest = (i + 1 + curHardpoint) % numHardpoints;

        // Marco edit - non-zero weapon check due to problems with weapon cycling
        if (hardPoint[stationUnderTest] and (hardPoint[stationUnderTest]->GetWeaponData()->domain bitand domainDesired) and 
            (hardPoint[stationUnderTest]->weaponCount not_eq 0 or
             hardPoint[stationUnderTest]->GetWeaponType() == wtGuns
             or hardPoint[stationUnderTest]->GetWeaponType() == wtAgm88
             or hardPoint[stationUnderTest]->GetWeaponType() == wtGBU)) // JB 010726 Allow HTS/LaserPod to be selected even when out of weapons
        {
            newType = hardPoint[stationUnderTest]->GetWeaponType();

            // Set the current hardpoint so that this weapon is actually used
            //MI
            if (g_bRealisticAvionics)
            {
                if (newType == wtAgm65 and curWeaponType not_eq wtAgm65)
                    StepMavSubMode(TRUE);
            }

            ReleaseCurWeapon(stationUnderTest);
            SetCurrentWeapon(stationUnderTest, hardPoint[stationUnderTest]->weaponPointer.get());
            break;
        }
        // ASSOCIATOR 03/12/03: Added this check so that we can now properly cycle AG guns without relying on the
        // buggy g_bUseDefinedGunDomain that causes AI to crash
        else if (hardPoint[stationUnderTest] and (hardPoint[stationUnderTest]->GetWeaponData()->domain bitor wdBoth) and 
                 (hardPoint[stationUnderTest]->weaponCount not_eq 0 and hardPoint[stationUnderTest]->GetWeaponType() == wtGuns))
        {
            newType = hardPoint[stationUnderTest]->GetWeaponType();
            break;
        }
    }

    SelectWeapon(newType, domainDesired);

    //MI
    if (g_bRealisticAvionics and domainDesired == wdGround)
    {
        RadarDopplerClass* pradar = (RadarDopplerClass*) FindSensor(ownship, SensorClass::Radar);

        if (pradar)
            pradar->SetScanDir(1.0F);
    }

    return (newType);
}

// This finds, selects, and returns the next type of weapon of the desired domain only
#if 0
WeaponType SMSClass::GetNextWeaponSpecific(WeaponDomain domainDesired)
{
    WeaponType newType = curWeaponType;
    FireControlComputer *FCC = ownship->GetFCC();
    int i = 0;
    int stationUnderTest = 0;

    // Don't do this if we're in the process of firing a weapon
    if (IsSet(Firing))
    {
        return curWeaponType;
    }

    // Find the next hardpoint with a weapon of the desired type on it
    for (i = 0; i < numHardpoints; i++)
    {
        stationUnderTest = (i + 1 + curHardpoint) % numHardpoints;

        if (hardPoint[stationUnderTest] and (hardPoint[stationUnderTest]->GetWeaponData()->domain == domainDesired))
        {
            newType = hardPoint[stationUnderTest]->GetWeaponType();

            // Set the current hardpoint so that this weapon is actually used
            ReleaseCurWeapon(stationUnderTest);
            SetCurrentWeapon(stationUnderTest, hardPoint[stationUnderTest]->weaponPointer);
            break;
        }
    }

    SelectWeapon(newType, domainDesired);
    return (newType);
}
#endif

//JPO break into separate routine.
void SMSClass::SelectWeapon(WeaponType newtype, WeaponDomain domainDesired)
{
    FireControlComputer *FCC = ownship->GetFCC();
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
    HarmTargetingPod* harmPod = (HarmTargetingPod*) FindSensor(ownship, SensorClass::HTS);

    // Tell the FCC about the new weapon selection
    switch (newtype)
    {
        case wtNone:
            FCC->SetMasterMode(FireControlComputer::Nav);
            break;

            // ASSOCIATOR 3/12/03: Added some checks so that it cycles correctly
        case wtAim9:
            if (FCC->GetMasterMode() == FireControlComputer::Dogfight)
            {
                FCC->SetDgftSubMode(FireControlComputer::Aim9);
            }
            else if (FCC->GetMasterMode() == FireControlComputer::MissileOverride)
            {
                FCC->SetMrmSubMode(FireControlComputer::Aim9); // ASSOCIATOR 04/12/03: for remembering MRM mode missiles
            }
            else
            {
                FCC->SetMasterMode(FireControlComputer::Missile);
                FCC->SetSubMode(FireControlComputer::Aim9);
                FCC->lastAirAirSubMode = FireControlComputer::Aim9;
            }

            break;

            // ASSOCIATOR 3/12/03: Added some checks so that it cycles correctly
        case wtAim120:
            if (FCC->GetMasterMode() == FireControlComputer::Dogfight)
            {
                FCC->SetDgftSubMode(FireControlComputer::Aim120);
            }
            else if (FCC->GetMasterMode() == FireControlComputer::MissileOverride)
            {
                FCC->SetMrmSubMode(FireControlComputer::Aim120); // ASSOCIATOR 04/12/03: for remembering MRM mode missiles
            }
            else
            {
                FCC->SetMasterMode(FireControlComputer::Missile);
                FCC->SetSubMode(FireControlComputer::Aim120);
                FCC->lastAirAirSubMode = FireControlComputer::Aim120;
            }

            break;

        case wtGuns:
        {
            // ASSOCIATOR 04/12/03: In DGFT we are already in Gun mode
            if (playerAC->FCC->GetMasterMode() == FireControlComputer::Dogfight)
                return;

            if (domainDesired == wdAir)
            {
                FCC->SetMasterMode(FireControlComputer::AAGun);
                FCC->SetSubMode(FCC->lastAirAirGunSubMode);   // ASSOCIATOR 3/12/03: changed default EEGS to lastAirAirGunSubMode
            }
            else
            {
                FCC->SetMasterMode(FireControlComputer::AGGun);
                FCC->SetSubMode(FireControlComputer::STRAF);
            }
        }
        break;

        case wtAgm88:
            // RV - I-Hawk - Submode will be set in HARM display classes...
            FCC->SetMasterMode(FireControlComputer::AirGroundHARM);
            break;

        case wtAgm65:
            FCC->SetMasterMode(FireControlComputer::AirGroundMissile);

            //MI done elsewhere
            if ( not g_bRealisticAvionics)
            {
                // M.N. no ATRealisticAV check needed here as in AtRealisticAV mode: g_bRealisticAvionics == TRUE
                if (PlayerOptions.GetAvionicsType() == ATRealistic)
                    FCC->SetSubMode(FireControlComputer::BSGT);
                else
                    FCC->SetSubMode(FireControlComputer::SLAVE);
            }

            break;

        case wtMk82:
        case wtMk84:
            if (FCC->GetMasterMode() not_eq FireControlComputer::AirGroundBomb) // or // MLR 4/3/2004 -
                //FCC->GetSubMode() == FireControlComputer::STRAF or
                //FCC->GetSubMode() == FireControlComputer::OBSOLETERCKT)
            {
                FCC->SetMasterMode(FireControlComputer::AirGroundBomb);
                FCC->SetSubMode(FireControlComputer::CCRP);  //me123 don't go to ccip everytime we select a mk82/84 hmm this might be ai stuff
            }

            break;

        case wtLAU:
            // FCC->SetMasterMode( FireControlComputer::AirGroundBomb ); // MLR 4/3/2004 -
            // FCC->SetSubMode( FireControlComputer::OBSOLETERCKT );
            FCC->SetMasterMode(FireControlComputer::AirGroundRocket);   // MLR 4/3/2004 -
            break;

        case wtGBU:
            FCC->SetMasterMode(FireControlComputer::AirGroundLaser);

            //MI in realistic we start in slave
            if ( not g_bRealisticAvionics)
            {
                if (PlayerOptions.GetAvionicsType() == ATRealistic)
                    FCC->SetSubMode(FireControlComputer::BSGT);
                else
                    FCC->SetSubMode(FireControlComputer::SLAVE);
            }
            else
            {
                // M.N. added full realism mode
                if (PlayerOptions.GetAvionicsType() not_eq ATRealistic and PlayerOptions.GetAvionicsType() not_eq ATRealisticAV)
                    FCC->SetSubMode(FireControlComputer::BSGT);
                else
                    FCC->SetSubMode(FireControlComputer::SLAVE);
            }

            break;

        case wtFixed:
            if (hardPoint[curHardpoint]->GetWeaponClass() == wcCamera)
            {
                FCC->SetMasterMode(FireControlComputer::AirGroundCamera);
            }

            break;
    }

}

int SMSClass::HasTrainable(void)
{
    int i;
    int retval = FALSE;

    for (i = 0; i < numHardpoints; i++)
    {
        if (hardPoint[i] and hardPoint[i]->GetWeaponData()->flags bitand SMSClass::Trainable)
        {
            retval = TRUE;
            break;
        }
    }

    return retval;
}

void SMSClass::SetUnlimitedGuns(int flag)
{
    int i;

    for (i = 0; i < numHardpoints; i++)
    {
        if (hardPoint[i] and hardPoint[i]->GetGun())
        {
            hardPoint[i]->GetGun()->unlimitedAmmo = flag;
        }
    }
}

void SMSClass::SetUnlimitedAmmo(int newFlag)
{
    if (newFlag)
        flags or_eq UnlimitedAmmoFlag;
    else
        flags and_eq compl UnlimitedAmmoFlag;

    SetUnlimitedGuns(newFlag);
}

#if 0
void SMSClass::SetPair(int flag)
{
    if (curWeaponClass == wcBombWpn)
    {
        pair = flag;
    }

    //MI
    if (g_bRealisticAvionics)
    {
        if (curWeaponClass == wcGbuWpn)
            pair = flag;

        SetAGBPair(pair ? 1 : 0); // MLR 4/3/2004 -
        /*
        if(Prof1)
           Prof1Pair = pair ? 1 : 0;
        else
           Prof2Pair = pair ? 1 : 0;
        */
    }
}
#endif

// 2000-08-31 ADDED BY S.G. SO BOMBER HAVE A HIGHER RIPPLE COUNT AVAILABLE
#define MAX_RIPPLE_COUNT_BOMBER 48

// END OF ADDED SECTION
void SMSClass::IncrementRippleCount(void)
{
    if (curWeaponClass == wcBombWpn or (g_bRealisticAvionics and curWeaponClass == wcGbuWpn))
    {
        if (ownship->GetSType() == STYPE_AIR_BOMBER)
            SetAGBRippleCount((GetAGBRippleCount() + 1) % MAX_RIPPLE_COUNT_BOMBER);
        else
            SetAGBRippleCount((GetAGBRippleCount() + 1) % MAX_RIPPLE_COUNT_BOMBER);
    }


#if 0

    if (curWeaponClass == wcBombWpn)
    {
        // 2000-08-31 ADDED BY S.G. SO BOMBER HAVE A HIGHER RIPPLE COUNT AVAILABLE
        if (ownship->GetSType() == STYPE_AIR_BOMBER)
            //rippleCount = (rippleCount + 1) % MAX_RIPPLE_COUNT_BOMBER; // MLR 4/3/2004 -
            SetAGBRippleCount((GetAGBRippleCount() + 1) % MAX_RIPPLE_COUNT_BOMBER);
        else
            // END OF ADDED SECTION
            //rippleCount = (rippleCount + 1) % MAX_RIPPLE_COUNT; // MLR 4/3/2004 -
            SetAGBRippleCount((GetAGBRippleCount() + 1) % MAX_RIPPLE_COUNT_BOMBER);
    }

    //MI
    if (g_bRealisticAvionics)
    {
        if (curWeaponClass == wcGbuWpn)
        {
            // 2000-08-31 ADDED BY S.G. SO BOMBER HAVE A HIGHER RIPPLE COUNT AVAILABLE
            if (ownship->GetSType() == STYPE_AIR_BOMBER)
                rippleCount = (rippleCount + 1) % MAX_RIPPLE_COUNT_BOMBER;
            else
                // END OF ADDED SECTION
                rippleCount = (rippleCount + 1) % MAX_RIPPLE_COUNT;
        }


        SetAGBRippleCount(rippleCount); // MLR 4/3/2004 -
        /*if(Prof1)
           Prof1RP = rippleCount;
        else
           Prof2RP = rippleCount;
           */
    }

#endif
}

void SMSClass::DecrementRippleCount(void)
{
    if (curWeaponClass == wcBombWpn or (g_bRealisticAvionics and curWeaponClass == wcGbuWpn))
    {
        if (ownship->GetSType() == STYPE_AIR_BOMBER)
            SetAGBRippleCount((GetAGBRippleCount() - 1) % MAX_RIPPLE_COUNT_BOMBER);
        else
            SetAGBRippleCount((GetAGBRippleCount() - 1) % MAX_RIPPLE_COUNT_BOMBER);
    }

#if 0

    if (curWeaponClass == wcBombWpn)
    {
        rippleCount --;

        if (rippleCount < 0)

            // 2000-08-31 ADDED BY S.G. SO BOMBER HAVE A HIGHER RIPPLE COUNT AVAILABLE
            if (ownship->GetSType() == STYPE_AIR_BOMBER)
                rippleCount = MAX_RIPPLE_COUNT_BOMBER - 1;
            else
                // END OF ADDED SECTION
                rippleCount = MAX_RIPPLE_COUNT - 1;
    }

    //MI
    if (g_bRealisticAvionics)
    {
        if (curWeaponClass == wcGbuWpn)
        {
            rippleCount --;

            if (rippleCount < 0)

                // 2000-08-31 ADDED BY S.G. SO BOMBER HAVE A HIGHER RIPPLE COUNT AVAILABLE
                if (ownship->GetSType() == STYPE_AIR_BOMBER)
                    rippleCount = MAX_RIPPLE_COUNT_BOMBER - 1;
                else
                    // END OF ADDED SECTION
                    rippleCount = MAX_RIPPLE_COUNT - 1;
        }

        SetAGBRippleCount(rippleCount); // MLR 4/3/2004 -
        /*
        if(Prof1)
           Prof1RP = rippleCount;
        else
           Prof2RP = rippleCount;
           */
    }

#endif
}

void SMSClass::IncrementRippleInterval(void)
{
    if (curWeaponClass == wcBombWpn or (g_bRealisticAvionics and curWeaponClass == wcGbuWpn))
    {
        SetAGBRippleInterval((GetAGBRippleInterval() + 50) % 200);
    }

#if 0

    if (curWeaponClass == wcBombWpn)
    {
        //rippleInterval = (rippleInterval + 50) % 200;
        SetAGBRippleInterval((GetAGBRippleInterval() + 50) % 200);
    }

    //MI
    if (g_bRealisticAvionics)
    {
        if (curWeaponClass == wcGbuWpn)
        {
            //rippleInterval = (rippleInterval + 50) % 200;
            SetAGBRippleInterval((GetAGBRippleInterval() + 50) % 200);

        }

        SetAGBRippleInterval(rippleInterval); // MLR 4/3/2004 -
        /*
            if(Prof1)
            Prof1RS = rippleInterval;
            else
            Prof2RS = rippleInterval;
            */
    }

#endif
}

void SMSClass::DecrementRippleInterval(void)
{
    if (curWeaponClass == wcBombWpn or (g_bRealisticAvionics and curWeaponClass == wcGbuWpn))
    {
        SetAGBRippleInterval((GetAGBRippleInterval() - 50) % 200);
    }

#if 0

    if (curWeaponClass == wcBombWpn)
    {
        rippleInterval -= 50;

        if (rippleInterval < 0)
            rippleInterval = 175;
    }

    //MI
    if (g_bRealisticAvionics)
    {
        if (curWeaponClass == wcGbuWpn)
        {
            rippleInterval -= 50;

            if (rippleInterval < 0)
                rippleInterval = 175;
        }

        SetAGBRippleInterval(rippleInterval); // MLR 4/3/2004 -

        /*
        if(Prof1)
           Prof1RS = rippleInterval;
        else
           Prof2RS = rippleInterval;
        */
    }

#endif
}

void SMSClass::SetRippleInterval(int rippledistance)
{
    // MLR 4/3/2004 -
    if (curWeaponClass == wcBombWpn or (g_bRealisticAvionics and curWeaponClass == wcGbuWpn))
    {
        if (rippledistance > 999)
            rippledistance = 999;

        if (rippledistance < 10)
            rippledistance = 10;

        SetAGBRippleInterval(rippledistance);
    }

#if 0

    if (curWeaponClass == wcBombWpn)
    {
        rippleInterval = rippledistance;
    }

    if (rippleInterval > 175)
    {
        rippleInterval = 175;
    }

    if (rippleInterval < 0)
    {
        rippleInterval = 0;
    }

#endif
}

void SMSClass::Incrementarmingdelay(void) //me123 status ok. addet this subclass "Incrementarmingdelay"
{
    if (armingdelay >= 0)
    {
        if (armingdelay < 300)
            armingdelay += 20;
        else if (armingdelay < 500)
            armingdelay += 30;
        else if (armingdelay < 1000)
            armingdelay += 40;
        else
            armingdelay = 75;
    }
}
void SMSClass::IncrementBurstHeight(void)
{
    if (curHardpoint >= 0 and hardPoint[curHardpoint]->GetWeaponData()->flags bitand SMSClass::HasBurstHeight)
    {
        if (burstHeight < 900)
            burstHeight += 200;
        else if (burstHeight < 1800)
            burstHeight += 300;
        else if (burstHeight < 3000)
            burstHeight += 400;
        else
            burstHeight = 300;

        // Cobra - limit max BH
        if (burstHeight > 6000)
            burstHeight = 6000;
    }

    if (g_bRealisticAvionics)
    {
        SetAGBBurstAlt((int)burstHeight);
    }
}

void SMSClass::DecrementBurstHeight(void)
{
    if (curHardpoint >= 0 and hardPoint[curHardpoint]->GetWeaponData()->flags bitand SMSClass::HasBurstHeight)
    {
        if (burstHeight > 1800)
            burstHeight -= 400;
        else if (burstHeight > 900)
            burstHeight -= 300;
        else if (burstHeight > 300)
            burstHeight -= 200;
        else
            burstHeight = 3000;

        // Cobra - limit min BH
        if (burstHeight < 0)
            burstHeight = 0;
    }

    if (g_bRealisticAvionics)
    {
        SetAGBBurstAlt((int)burstHeight);
    }
}

//make me compatible with new rack code
int SMSClass::JettisonStation(int stationNum, JettisonMode mode)
{
    VuBin<SimWeaponClass> tempPtr, weapPtr;

    // only works for valid stations with positive G's on a local machine
    if (stationNum < 1) // MLR 3/2/2004 - changed from 0 - which is the gun
    {
        return 0;
    }

    //MI
    if ( not g_bRealisticAvionics)
    {
        if ((ownship->IsLocal()) and (((AircraftClass*)ownship)->af->nzcgb <= 0.0F))
        {
            return 0;
        }
    }
    else
    {
        if (ownship->IsLocal() and not ownship->OnGround())
        {
            // RV - Biker - Only allow jettison when in plane flight
            if (mode not_eq RippedOff)
            {
                if (fabs(ownship->Roll()) > 35.0f * DTR)
                    return 0;

                if (((AircraftClass*)ownship)->Pitch() > 45.0f * DTR or ((AircraftClass*)ownship)->Pitch() < -60.0f * DTR)
                    return 0;

                if (((AircraftClass*)ownship)->af->MaxVcas() / 1.5f > ((AircraftClass*)ownship)->GetVt())
                    return 0;

                if (((AircraftClass*)ownship)->af->nzcgb < 0.5F or ((AircraftClass*)ownship)->af->nzcgb > 5.0f)
                    return 0;
            }
        }
    }

    if (hardPoint[stationNum]->weaponPointer or hardPoint[stationNum]->GetRack())
    {
        SimWeaponClass *weapptr;
        Tpoint pos , vec;
        int rdflags = hardPoint[stationNum]->GetRackDataFlags();

        int jettpylon = ((rdflags bitand RDF_EMERGENCY_JETT_PYLON) and (mode == Emergency)) or
                        ((rdflags bitand RDF_SELECTIVE_JETT_PYLON) and (mode == SelectivePylon));

        int jettrack  = jettpylon or
                        ((rdflags bitand RDF_EMERGENCY_JETT_RACK) and (mode == Emergency)) or
                        ((rdflags bitand RDF_SELECTIVE_JETT_RACK) and (mode == SelectiveRack));

        int jettweapon = jettrack or
                         ((rdflags bitand RDF_EMERGENCY_JETT_WEAPON) and (mode == Emergency)) or
                         ((rdflags bitand RDF_SELECTIVE_JETT_WEAPON) and (mode == SelectiveWeapon));

        MonoPrint("JettisonStation(%d,%d) : rdflags=%8x  jettpylon=%d  jettrack=%d jettweapon=%d",
                  stationNum, mode, rdflags, jettpylon, jettrack, jettweapon);




        if (jettweapon)
        {
            weapptr = hardPoint[stationNum]->weaponPointer.get();

            while (weapptr)
            {
                //RemoveStore(stationNum, hardPoint[stationNum]->weaponId);
                // remove the BSP
                DrawableBSP *bsp;
                bsp = (DrawableBSP *)weapptr->drawPointer;

                hardPoint[stationNum]->DetachWeaponBSP(weapptr);



                weapptr->drawPointer = NULL;

                if (bsp)
                {
                    bsp->GetPosition(&pos);

                    /*
                    pos.x = ownship->XPos() + ownship->dmx[0][0]*lpos.x + ownship->dmx[1][0]*lpos.y + ownship->dmx[2][0]*lpos.z;
                    pos.y = ownship->YPos() + ownship->dmx[0][1]*lpos.x + ownship->dmx[1][1]*lpos.y + ownship->dmx[2][1]*lpos.z;
                    pos.z = ownship->ZPos() + ownship->dmx[0][2]*lpos.x + ownship->dmx[1][2]*lpos.y + ownship->dmx[2][2]*lpos.z;
                    */

                    vec.x = ownship->XDelta();
                    vec.y = ownship->YDelta();
                    vec.z = ownship->ZDelta();

                    // Create and add the "SFX" container
                    bsp->SetLabel("", 0xff00ff00);
                    OTWDriver.AddSfxRequest(new SfxClass(
                                                SFX_MOVING_BSP, // type
                                                &pos, // world pos
                                                &vec, // vector
                                                bsp, // BSP
                                                30.0f, // time to live
                                                1.0f)); // scale
                }

                weapptr = weapptr->GetNextOnRail();
            }

            // If it's  fuel tank and it has anything in it, remove it
            if (hardPoint[stationNum]->GetWeaponClass() == wcTank and ownship->IsAirplane())
            {
                // float lostFuel = ((AircraftClass*)ownship)->af->ExternalFuel() / numOnBoard[hardPoint[stationNum]->GetWeaponClass()];
                // JPO redo with ne fuel stuff
                int center = (numHardpoints - 1) / 2 + 1;

                if (stationNum < center)
                    ((AircraftClass*)ownship)->af->DropTank(AirframeClass::TANK_LEXT); // XXX DROP
                else if (stationNum > center)
                    ((AircraftClass*)ownship)->af->DropTank(AirframeClass::TANK_REXT); // XXX DROP
                else ((AircraftClass*)ownship)->af->DropTank(AirframeClass::TANK_CLINE); // XXX DROP

                // ((AircraftClass*)ownship)->af->AddExternalFuel (-lostFuel);
            }

            // Iterate all the weapons on this station and clean them up
            weapPtr = hardPoint[stationNum]->weaponPointer;

            while (weapPtr)
            {
                if (FalconLocalGame->GetGameType() not_eq game_InstantAction)
                {
                    tempPtr.reset(weapPtr->GetNextOnRail());
                }

                RemoveStore(stationNum, hardPoint[stationNum]->weaponId);
                weapPtr = tempPtr;
            }

            hardPoint[stationNum]->weaponPointer.reset();
            numOnBoard[hardPoint[stationNum]->GetWeaponClass()] -= hardPoint[stationNum]->weaponCount;
            hardPoint[stationNum]->weaponCount = 0;

        }

        if (jettrack)
        {
            DrawableBSP *rack = hardPoint[stationNum]->DetachRackBSP();

            if (rack)
            {
                rack->GetPosition(&pos);

                RemoveStore(stationNum, hardPoint[stationNum]->GetRackId());

                vec.x = ownship->XDelta();
                vec.y = ownship->YDelta();
                vec.z = ownship->ZDelta();

                // Create and add the "SFX" container
                rack->SetLabel("", 0xff00ff00);
                OTWDriver.AddSfxRequest(new SfxClass(
                                            SFX_MOVING_BSP, // type
                                            &pos, // world pos
                                            &vec, // vector
                                            rack, // BSP
                                            30.0f, // time to live
                                            1.0f)); // scale
            }
        }

        if (jettpylon)
        {
            DrawableBSP *pylon = hardPoint[stationNum]->DetachPylonBSP();

            if (pylon)
            {
                pylon->GetPosition(&pos);

                RemoveStore(stationNum, hardPoint[stationNum]->GetPylonId());

                vec.x = ownship->XDelta();
                vec.y = ownship->YDelta();
                vec.z = ownship->ZDelta();

                // Create and add the "SFX" container
                pylon->SetLabel("", 0xff00ff00);
                OTWDriver.AddSfxRequest(new SfxClass(
                                            SFX_MOVING_BSP, // type
                                            &pos, // world pos
                                            &vec, // vector
                                            pylon, // BSP
                                            30.0f, // time to live
                                            1.0f)); // scale
            }
        }

        //if(ownship and not rippedOff)  // MLR 3/2/2004 - never used ???
        // ownship->SoundPos.Sfx( SFX_JETTISON, 0, 1, 0, pos.x, pos.y, pos.z);





#if 0

        if (hardPoint[stationNum]->GetRack())
        {
            droppedThing = hardPoint[stationNum]->GetRack();
            weapPtr = hardPoint[stationNum]->weaponPointer;
            RemoveStore(stationNum, hardPoint[stationNum]->GetRackId());
            hardPoint[stationNum]->SetRack(NULL);
        }
        else
        {
            droppedThing = (DrawableBSP *)hardPoint[stationNum]->weaponPointer->drawPointer;
            hardPoint[stationNum]->weaponPointer->drawPointer = NULL;
            weapPtr = NULL;
            RemoveStore(stationNum, hardPoint[stationNum]->weaponId);
        }

        if (droppedThing)
        {
            Tpoint pos, vec;

            // Detach the thing from the parent object
            OTWDriver.DetachObject((DrawableBSP*)(ownship->drawPointer), (DrawableBSP*)droppedThing, stationNum - 1);

            // Get initial location and velocity for the "SFX" container
            droppedThing->GetPosition(&pos);
            vec.x = ownship->XDelta();
            vec.y = ownship->YDelta();
            vec.z = ownship->ZDelta();

            // Create and add the "SFX" container
            droppedThing->SetLabel("", 0xff00ff00);
            OTWDriver.AddSfxRequest(new SfxClass(
                                        SFX_MOVING_BSP, // type
                                        &pos, // world pos
                                        &vec, // vector
                                        droppedThing, // BSP
                                        30.0f, // time to live
                                        1.0f)); // scale

            // Play the jettison sound unless the thing is being ripped off, in which case it takes care of itself
            if ( not rippedOff)
                if (ownship) ownship->SoundPos.Sfx(SFX_JETTISON, 0, 1, 0, pos.x, pos.y, pos.z);

            // If this was a rack, we will drop all its children seperatly to ensure proper cleanup.
            // SCR:  It would be nice if this could be handled by a sub-class of SfxClass.
            while (weapPtr)
            {

                // Get the child drawable (if any)
                DrawableBSP *child = (DrawableBSP*)weapPtr->drawPointer;

                if (child)
                {

                    // Detach the child from the parent rack
                    OTWDriver.DetachObject(droppedThing, child, weapPtr->GetRackSlot());
                    weapPtr->drawPointer = NULL;

                    // Get initial location for the "SFX" container
                    child->GetPosition(&pos);

                    // Create and add the "SFX" container
                    droppedThing->SetLabel("", 0xff00ff00);
                    OTWDriver.AddSfxRequest(new SfxClass(
                                                SFX_MOVING_BSP, // type
                                                &pos, // world pos
                                                &vec, // vector
                                                child, // BSP
                                                30.0f, // time to live
                                                1.0f)); // scale
                }

                // Get the next weapon
                weapPtr = weapPtr->GetNextOnRail();
            }
        }

#endif
        /*
         // If it's  fuel tank and it has anything in it, remove it
         if (hardPoint[stationNum]->GetWeaponClass() == wcTank and ownship->IsAirplane())
         {
        // float lostFuel = ((AircraftClass*)ownship)->af->ExternalFuel() / numOnBoard[hardPoint[stationNum]->GetWeaponClass()];
           // JPO redo with ne fuel stuff
             int center = (numHardpoints - 1) / 2 + 1;
         if (stationNum < center)
             ((AircraftClass*)ownship)->af->DropTank(AirframeClass::TANK_LEXT); // XXX DROP
         else if (stationNum > center)
             ((AircraftClass*)ownship)->af->DropTank(AirframeClass::TANK_REXT); // XXX DROP
         else ((AircraftClass*)ownship)->af->DropTank(AirframeClass::TANK_CLINE); // XXX DROP

        // ((AircraftClass*)ownship)->af->AddExternalFuel (-lostFuel);
         }

         // Iterate all the weapons on this station and clean them up
         weapPtr = hardPoint[stationNum]->weaponPointer;
         tempPtr = NULL;
         while (weapPtr)
         {
         ShiAssert( weapPtr->drawPointer == NULL );
         if(FalconLocalGame->GetGameType() not_eq game_InstantAction)
         tempPtr = weapPtr->GetNextOnRail();
         RemoveStore(stationNum, hardPoint[stationNum]->weaponId);
         // 2002-02-08 ADDED BY S.G. Before we delete it, we must check if the drawable->thePrevMissile is pointing to it but NOT referenced so we can clear it as well otherwise it will CTD in UpdateGroundSpot
         if (drawable and drawable->thePrevMissile and drawable->thePrevMissile == weapPtr and not drawable->thePrevMissileIsRef)
         drawable->thePrevMissile = NULL; // Clear it as well
         delete weapPtr;
         // vuAntiDB->Remove(weapPtr);
         weapPtr = tempPtr;
         }
         hardPoint[stationNum]->weaponPointer = NULL;
         numOnBoard[hardPoint[stationNum]->GetWeaponClass()] -= hardPoint[stationNum]->weaponCount;
         hardPoint[stationNum]->weaponCount = 0;
         */
    }



    {
        ChooseLimiterMode(1); // (me1234 lets check airframe_g-limit after stores jettison)
    }

    return 1;
}

void SMSClass::RipOffWeapons(float noseAngle)
{
    //we have only a gun or no weapons
    if (numHardpoints <= 1)
        return;

    int lwing = 1;
    int rwing = numHardpoints - 1;
    int center = 0;
    int count = numHardpoints - 1;
    int i;

    if (count % 2)
    {
        center = count / 2 + 1;
    }

    if (fabs(noseAngle) < 0.2588F)
    {
        //remove left wingtip weapon
        if (ownship->platformAngles.sinphi < -0.5F)
        {
            if (curHardpoint == lwing)
            {
                ReleaseCurWeapon(-1);
            }

            JettisonStation(lwing, RippedOff);
        }

        //remove left wing stores
        if (ownship->Roll() > -30.0F * DTR and ownship->Roll() < -10.0F * DTR)
        {
            for (i = lwing + 1; i < count / 2 + 1; i++)
            {
                if (curHardpoint == i)
                {
                    ReleaseCurWeapon(-1);
                }

                JettisonStation(i, RippedOff);
            }
        }

        //remove centerline stores
        if (center and ownship->Roll() > -20.0F * DTR and ownship->Roll() < 20.0F * DTR)
        {
            if (curHardpoint == center)
            {
                ReleaseCurWeapon(-1);
            }

            JettisonStation(center, RippedOff);
        }

        //remove right wing stores
        if (ownship->Roll() > 10.0F * DTR and ownship->Roll() < 30.0F * DTR)
        {
            for (i = count / 2 + 1 + (center ? 1 : 0); i < rwing; i++)
            {
                if (curHardpoint == i)
                {
                    ReleaseCurWeapon(-1);
                }

                JettisonStation(i, RippedOff);
            }
        }

        //remove right wingtip weapon
        if (rwing and ownship->platformAngles.sinphi > 0.5F)
        {
            if (curHardpoint == rwing)
            {
                ReleaseCurWeapon(-1);
            }

            JettisonStation(rwing, RippedOff);
        }
    }
}

void SMSClass::AddStore(int station, int storeId, int visible)
{
    float x, y, z;

    if (ownship->IsAirplane() and not UnlimitedAmmo())
    {
        ShiAssert(((AircraftClass *)ownship)->af);

        hardPoint[station]->GetPosition(&x, &y, &z);

        if (((AircraftClass *)ownship)->IsF16() and (station == 1 or station == 9))
            ((AircraftClass *)ownship)->af->AddWeapon(WeaponDataTable[storeId].Weight, 0.0F, y);
        else if (visible)
            ((AircraftClass *)ownship)->af->AddWeapon(WeaponDataTable[storeId].Weight,
                    WeaponDataTable[storeId].DragIndex,
                    y);
        else
            ((AircraftClass *)ownship)->af->AddWeapon(WeaponDataTable[storeId].Weight, 0.0F, 0.0F);

        if (gLimiterMgr->HasLimiter(CatIIICommandType, ((AircraftClass *)ownship)->af->VehicleIndex()))
        {
            if (hardPoint[station]->GetWeaponClass() == wcRocketWpn or
                hardPoint[station]->GetWeaponClass() == wcBombWpn or
                hardPoint[station]->GetWeaponClass() == wcTank or
                hardPoint[station]->GetWeaponClass() == wcAgmWpn or
                hardPoint[station]->GetWeaponClass() == wcHARMWpn or
                hardPoint[station]->GetWeaponClass() == wcSamWpn or
                hardPoint[station]->GetWeaponClass() == wcGbuWpn)
            {

                // OW CATIII Fix
                //if( not g_bEnableCATIIIExtension) MI
                if ( not g_bRealisticAvionics)
                {
                    ((AircraftClass *)ownship)->af->SetFlag(AirframeClass::CATLimiterIII);

                    if (hardPoint[station]->GetWeaponClass() == wcTank)
                    {
                        if (((AircraftClass *)ownship)->af->curMaxGs > 7.5F)
                            ((AircraftClass *)ownship)->af->curMaxGs = 7.5F;
                    }
                    else
                    {
                        if (((AircraftClass *)ownship)->af->curMaxGs > 6.0F)
                            ((AircraftClass *)ownship)->af->curMaxGs = 6.0F;
                    }
                }

                else
                {
                    if (((AircraftClass *)ownship)->af->IsSet(AirframeClass::IsDigital))//me123 let's only set the cat switch for ai
                        ((AircraftClass *)ownship)->af->SetFlag(AirframeClass::CATLimiterIII);

                    if (hardPoint[station]->GetWeaponClass() == wcTank)
                    {
                        // MD -- 20040531: quick fix for JPO...if there's no fuel on the centerline, don't
                        // set the limiter
                        if ((station == numHardpoints / 2) and (((AircraftClass *)ownship)->af->m_tanks[AirframeClass::TANK_CLINE] > 0.0F))
                            //if ((station == 5) and (((AircraftClass *)ownship)->af->m_tanks[AirframeClass::TANK_CLINE] > 0.0F))
                            //if (station == 5) // centerline tank
                            //if (station == numHardpoints /2) // centerline tank
                        {
                            if (((AircraftClass *)ownship)->af->curMaxGs > 7.0F) //me123 from 7.5
                            {
                                ((AircraftClass *)ownship)->af->curMaxGs = 7.0F;//me123 from 7.5
                                ((AircraftClass *)ownship)->af->curMaxStoreSpeed = 600.0f;//me123
                                ((AircraftClass *)ownship)->af->ClearFlag(AirframeClass::CATLimiterIII);
                            }
                        }
                        else // dollys
                        {
                            if (((AircraftClass *)ownship)->af->curMaxGs > 6.5F)
                            {
                                ((AircraftClass *)ownship)->af->curMaxGs = 6.5F;
                                ((AircraftClass *)ownship)->af->curMaxStoreSpeed = 600.0f;//me123
                                ((AircraftClass *)ownship)->af->SetFlag(AirframeClass::CATLimiterIII);
                            }
                        }
                    }
                    else
                    {
                        if (((AircraftClass *)ownship)->af->curMaxGs > 5.5F) //me123 from 6.0
                        {
                            ((AircraftClass *)ownship)->af->curMaxGs = 5.5F;//me123 from 6.0
                            ((AircraftClass *)ownship)->af->curMaxStoreSpeed = 550.0f;//me123
                        }
                    }
                }
            }
        }

    }
}

void SMSClass::ChooseLimiterMode(int hardpoint)
{
    int i;
    float gLimit;
    float storespeed;//me123

    gLimit = 9.0f;//me123
    storespeed = 800.0f;//me123

    if (ownship->IsAirplane() and ((AircraftClass *)ownship)->af and 
 not ((AircraftClass *)ownship)->af->IsSet(AirframeClass::IsDigital))
    {
        gLimit = ((AircraftClass *)ownship)->af->MaxGs();

        //MI
        //if( not g_bEnableCATIIIExtension or ((AircraftClass *)ownship)->af->IsSet(AirframeClass::IsDigital))
        if ( not g_bRealisticAvionics or ((AircraftClass *)ownship)->af->IsSet(AirframeClass::IsDigital))
            ((AircraftClass *)ownship)->af->ClearFlag(AirframeClass::CATLimiterIII);

        if (gLimiterMgr->HasLimiter(CatIIICommandType, ((AircraftClass *)ownship)->af->VehicleIndex()))
        {
            for (i = 0; i < numHardpoints; i++)
            {

                if (hardPoint[i] and hardPoint[i]->weaponPointer and 
                    hardPoint[i]->weaponCount > 0 and 
                    (hardPoint[i]->GetWeaponClass() == wcRocketWpn or
                     hardPoint[i]->GetWeaponClass() == wcBombWpn or
                     hardPoint[i]->GetWeaponClass() == wcTank or
                     hardPoint[i]->GetWeaponClass() == wcAgmWpn or
                     hardPoint[i]->GetWeaponClass() == wcHARMWpn or
                     hardPoint[i]->GetWeaponClass() == wcSamWpn or
                     hardPoint[i]->GetWeaponClass() == wcGbuWpn))
                {
                    // OW CATIII Fix
                    //if( not g_bEnableCATIIIExtension) MI
                    if ( not g_bRealisticAvionics)
                    {
                        if (hardPoint[i]->GetWeaponClass() == wcTank)
                        {
                            ((AircraftClass *)ownship)->af->SetFlag(AirframeClass::CATLimiterIII);

                            if (gLimit > 7.5F)
                                gLimit = 7.5F;
                        }
                        else
                        {
                            ((AircraftClass *)ownship)->af->SetFlag(AirframeClass::CATLimiterIII);

                            if (gLimit > 6.0F)
                                gLimit = 6.0F;
                        }
                    }

                    else
                    {
                        if (hardPoint[i]->GetWeaponClass() == wcTank)
                        {
                            if (((AircraftClass *)ownship)->af->IsSet(AirframeClass::IsDigital))//me123 don't change cat for player check addet
                            {
                                ((AircraftClass *)ownship)->af->SetFlag(AirframeClass::CATLimiterIII);
                            }

                            /*if(gLimit > 7.0F)//me123 addet fuel check
                            {
                             storespeed = 600.0f;//me123
                             if (((AircraftClass *)ownship)->af->ExternalFuel() >=1.0f)
                             gLimit = 7.0F;//me123 tanks g limit is 7Gz
                            }*/
                            //MI me123... we need to check which tank we have here too
                            if (i == numHardpoints / 2) // centerline tank
                            {
                                // MD -- 20040531: quick fix for JPO...if there's no fuel on the centerline, don't
                                // set the limiter
                                if (((AircraftClass *)ownship)->af->m_tanks[AirframeClass::TANK_CLINE] > 0.0F)
                                {
                                    if (gLimit > 7.0F) //me123 from 7.5
                                    {
                                        gLimit = 7.0F;//me123 from 7.5
                                        storespeed = 600.0f;//me123
                                    }
                                }
                            }
                            else // dollys
                            {
                                if (gLimit > 6.5F)
                                {
                                    gLimit = 6.5F;
                                    storespeed = 600.0f;//me123
                                }
                            }
                        }
                        else
                        {
                            if (((AircraftClass *)ownship)->af->IsSet(AirframeClass::IsDigital))//me123 don't change cat for player check addet
                            {
                                ((AircraftClass *)ownship)->af->SetFlag(AirframeClass::CATLimiterIII);
                            }

                            if (gLimit > 5.5F)
                            {
                                gLimit = 5.5F;//me123 stores in general has 5.5 g limit
                                storespeed = 550.0f;//me123
                            }
                        }
                    }
                }
            }
        }

        ((AircraftClass *)ownship)->af->curMaxGs = gLimit;
        ((AircraftClass *)ownship)->af->curMaxStoreSpeed = storespeed;//me123
    }

}

void SMSClass::RemoveStore(int station, int storeId)
{
    VehicleClassDataType* vc;
    float x, y, z;
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    //ShiAssert(hardPoint[station]->weaponCount > 0);
    if (hardPoint[station]->weaponCount >= 0)
    {
        if (ownship->IsAirplane() and not UnlimitedAmmo())
        {
            vc = GetVehicleClassData(ownship->Type() - VU_LAST_ENTITY_TYPE);
            ShiAssert(((AircraftClass *)ownship)->af);

            hardPoint[station]->GetPosition(&x, &y, &z);

            if (((AircraftClass *)ownship)->IsF16() and (station == 2 or station == 8) and storeId == gRackId_Single_Rack)
                ((AircraftClass *)ownship)->af->RemoveWeapon(WeaponDataTable[storeId].Weight - 283.0F, WeaponDataTable[storeId].DragIndex - 11.0F, y);
            else if (((AircraftClass *)ownship)->IsF16() and (station == 1 or station == 9))
                ((AircraftClass *)ownship)->af->RemoveWeapon(WeaponDataTable[storeId].Weight, 0.0F, y);
            else if (vc->VisibleFlags bitand (1 << station))
                ((AircraftClass *)ownship)->af->RemoveWeapon(WeaponDataTable[storeId].Weight,
                        WeaponDataTable[storeId].DragIndex,
                        y);
            else
                ((AircraftClass *)ownship)->af->RemoveWeapon(WeaponDataTable[storeId].Weight, 0.0F, 0.0F);

            ChooseLimiterMode(station);
        }

        if (ownship == playerAC)
        {
            if (station < numHardpoints / 2)
                JoystickPlayEffect(JoyLeftDrop, 0);
            else
                JoystickPlayEffect(JoyRightDrop, 0);
        }
    }
}

// =========================================
// Supporting functions
// =========================================

VuBin<SimWeaponClass> InitWeaponList(
    FalconEntity* parent, ushort weapid, int weapClass, int num,
    SimWeaponClass* initFunc(FalconEntity* parent, ushort type, int slot),
    int *loadOrder
)
{
    VuBin<SimWeaponClass> weapPtr;
    VuBin<SimWeaponClass> lastPtr;
    int i = 0, rackSize = 1;

    // Find the real weapon class
    weapClass = SimWeaponDataTable[Falcon4ClassTable[WeaponDataTable[weapid].Index].vehicleDataIndex].weaponClass;

    // Determine rack size;
    if (num)
    {
        // RV - Biker - This is why AGMs don't load nice on 2 slot rack
        // MLR 2003-10-16 - make this optional, FF crew has gone mad. :)
        if (g_bSMSPylonLoadingFix or parent->IsHelicopter())
        {
            rackSize = num; // MLR fixes issue with 2 bitand 5 slotted A2G racks not being loaded correctly
        }
        else
        {
            if (num > 6)
                rackSize = num;
            else if (num > 4)
                rackSize = 6;
            else if (num == 4)
                rackSize = 4;
            else if (num == 2 and weapClass == wcAimWpn)
                rackSize = 2;
            else if (num == 1 and weapClass == wcAimWpn)
                rackSize = 1;
            else if (num > 1)
                rackSize = 3;
            else  if (num > 0)
                rackSize = 1;
        }


        for (i = 0; i < num; i++)
        {
            // Load from back of rack to front (ie, 2 missiles on a tri-rack will
            // load into slot 1 and 2, not 0 and 1)
            weapPtr.reset(initFunc(parent, weapid, rackSize - (i + 1)));

            if (lastPtr)
            {
                weapPtr->nextOnRail = lastPtr;
            }

            lastPtr = weapPtr;
        }
    }

    return weapPtr;
}

//MI
// MLR 2/20/2004 - this can only be called in the players SMS
// MLR 3/20/2004 - this should be a FCC class call.
void SMSBaseClass::StepMavSubMode(bool init)
{
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
    RadarDopplerClass* theRadar = (RadarDopplerClass*)FindSensor(playerAC, SensorClass::Radar);
    FireControlComputer *FCC = ownship->GetFCC();

    if ( not theRadar or not FCC)
        return;

    if ( not FCC->PlayerFCC())
        return; // MLR just in case

    if ( not g_bRealisticAvionics) // MLR 7/17/2004 - //Cobra 10/31/04 TJL
    {
        MavSubMode = SMSBaseClass::PRE;
        FCC->SetSubMode(FireControlComputer::SLAVE);
        return;
    }

    if (init)
    {
        MavSubMode = SMSBaseClass::PRE;
        theRadar->SelectGM();
        theRadar->SetScanDir(1.0F);
        FCC->SetSubMode(FireControlComputer::SLAVE);
        return;
    }

    if (MavSubMode == SMSBaseClass::PRE)
    {
        MavSubMode = SMSBaseClass::VIS;
        theRadar->SelectAGR();
        theRadar->SetScanDir(1.0F);
        FCC->SetSubMode(FireControlComputer::BSGT);
    }
    else if (MavSubMode == SMSBaseClass::VIS)
    {
        MavSubMode = SMSBaseClass::BORE;
        theRadar->SelectGM();
        theRadar->SetScanDir(1.0F);
        FCC->SetSubMode(FireControlComputer::BSGT);
    }
    else
    {
        MavSubMode = SMSBaseClass::PRE;
        theRadar->SelectGM();
        theRadar->SetScanDir(1.0F);
        FCC->SetSubMode(FireControlComputer::SLAVE);
    }
}
//MI


#include <alist.h>

class WeaponStepNode : public ANode
{
    int CompareWith(ANode *a)
    {
        int i = stricmp(name, ((WeaponStepNode *)a)->name);

        if (i == 0)
        {
            if (weaponCount == 0 and ((WeaponStepNode *)a)->weaponCount)
            {
                return 1;
            }
            else
            {
                return -1;
            }
        }

        return i;
    }
public:
    char *name;
    int weaponId,
        weaponCount;
};

// MLR 2/8/2004 - StepWeaponClass renamed to StepAAWeapon
void SMSClass::StepAAWeapon(void)
{
    FireControlComputer *fcc;
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    if (playerAC and 
        playerAC->FCC)
    {
        fcc = playerAC->FCC;

        switch (fcc->GetMasterMode())
        {
            case FireControlComputer::MissileOverride:
            case FireControlComputer::Dogfight:
            case FireControlComputer::Missile:
            case FireControlComputer::AAGun:
                StepWeaponByID(); // only step if we are already in an AA or gun mode
                fcc->SetAAMasterModeForCurrentWeapon(); // let the FCC figure out the best master/sub mode
                break;

            default:
                fcc->EnterAAMasterMode();
                // fcc->SetMasterMode(FireControlComputer::Missile); // otherwise, change master mode
                break;
        }
    }
}

// MLR 2/8/2004 - StepAGWeapon
void SMSClass::StepAGWeapon(void)
{
    FireControlComputer *fcc;
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    if (playerAC and 
        playerAC->FCC)
    {
        fcc = playerAC->FCC;

        //switch(fcc->GetMasterMode())
        switch (fcc->GetMainMasterMode())
        {
            case MM_AG:
                /* After Jettison, we are still in the AGMM, but the curHardpoint is -1 */
                /* this really screws things up in SetAGMasterModeForCurrentWeapon()    */
                StepWeaponByID(); // only step if we are already in an AG or gun mode
                fcc->SetAGMasterModeForCurrentWeapon(); // let the FCC figure out the best master/sub mode
                break;

            default:
                fcc->EnterAGMasterMode();
                //fcc->SetMasterMode(FireControlComputer::AGGun); // otherwise, change master mode
        }
    }
}


// MLR 1/19/2004 - This has been totally redone so that it Steps to differing weapons, in alphabetical order.
// it keys off of the weaponId, and not the weaponType in order to find the next weapon.
#if 0
void SMSClass::StepWeaponByID(void)
{
    static looped = 0;
    int newHp  = -1;
    int i, t;
    int found;

    // 2002-02-08 ADDED BY S.G. From SMSClass::WeaponStep, other it will CTD. This will fix the CTD but not the original cause which is why curHardpoint is -1.
    if (curHardpoint < 0 or looped)
    {
        return; // Do nothing if no station is currently selected
    }

    if (hardPoint[curHardpoint]  and 
        hardPoint[curHardpoint]->weaponCount > 0)
    {
        // these will be fallback in case we don't find something else
        newHp = curHardpoint;  // MLR 1/20/2004 - just in case there's nothing else to choose from
    }
    else
    {
        newHp = -1;
    }

    found = 0;

    for (i = curHardpoint + 1; i not_eq curHardpoint and not found; i++)
    {
        i = i % numHardpoints;

        if (hardPoint[i]                 and 
            playerAC and 
            playerAC->FCC and 
            playerAC->FCC->CanStepToWeaponClass(hardPoint[i]->GetWeaponData()->weaponClass)
           )
        {
            // found a valid hp
            found = 1;

            // now make sure there's not a same weapon on a lower hp (avoid selecting duplicates)
            for (t = i - 1 ; t > 0 ; t--)
            {
                if (hardPoint[i]->weaponId == hardPoint[t]->weaponId)
                {
                    if (hardPoint[t]->weaponCount > 0)
                    {
                        found = 0;
                    }
                }
            }

            // now check to see if we have 0 qty but another hp has >0 qty
            if (hardPoint[i]->weaponCount == 0)
            {
                for (t = i + 1 ; t < numHardpoints; t++)
                {
                    if (hardPoint[i]->weaponId == hardPoint[t]->weaponId)
                    {
                        if (hardPoint[t]->weaponCount > 0)
                        {
                            found = 0;
                        }
                    }
                }
            }
        }
    }

    if (found)
        newHp = i - 1;


    looped = 1;

    if (newHp >= 0       and // if >-1 we found something
        hardPoint[newHp])// and // MLR 1/21/2004 - Just incase there's no HP
        //hardPoint[newHp]->weaponPointer )   // MLR 1/21/2004 - Just incase there's no weapon (causes CTD if NULL/Empty)
    {
        SetCurrentWeapon(newHp, hardPoint[newHp]->weaponPointer);
    }
    else
        SetCurrentWeapon(-1, NULL);

    looped = 0;
}
#endif

#if 1

#define SWC_DEBUG
void SMSClass::StepWeaponByID(void)
{
    static int looped = 0;
    int i;
    int useEmpty = 1;
    short newWeaponId;
    AList list;
    WeaponStepNode *n;
    SimWeaponClass *found = NULL;
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    // 2002-02-08 ADDED BY S.G.
    // From SMSClass::WeaponStep, other it will CTD.
    // This will fix the CTD but not the original cause which is why curHardpoint is -1.
    if (curHardpoint < 0  or looped or not (playerAC and playerAC->FCC))
    {
        return; // Do nothing if no station is currently selected
    }

    if (curHardpoint == -1)
    {
        curHardpoint = 0;
    }

    // needs to be set because if qty is 0, it is -1
    curWeaponId = hardPoint[curHardpoint]->weaponId;

    newWeaponId = curWeaponId;   // MLR 1/20/2004 - just in case there's nothing else to choose from

    WeaponStepNode *curWeapNode = 0;

    for (i = 0; i < numHardpoints; i++)
    {
        if (playerAC->FCC->CanStepToWeaponClass(hardPoint[i]->GetWeaponData()->weaponClass))
        {
            int skip = 0;

            // see if we already have a matching node
            // if so, tally WeaponCount
            n = (WeaponStepNode *)list.GetHead();

            while (n)
            {
                if (n->weaponId == hardPoint[i]->weaponId)
                {
                    n->weaponCount += hardPoint[i]->weaponCount;
                    skip = 1;
                }

                n = (WeaponStepNode *)n->GetSucc();
            }

            if ( not skip)
            {
                // create a new node
                n = new WeaponStepNode;
                n->name = hardPoint[i]->GetWeaponData()->mnemonic;
                n->weaponId = hardPoint[i]->weaponId;
                n->weaponCount = hardPoint[i]->weaponCount;

                if (n->weaponId == curWeaponId)
                {
                    // this is our current weapon, store for logic later.
                    curWeapNode = n;
                }

                list.AddSorted(n);
            }
        }
    }

    // logic to find the next weaponId
    n = NULL;

    if (curWeapNode)
    {
        n = (WeaponStepNode *)curWeapNode->GetSucc();
    }

    if ( not n)
    {
        n = (WeaponStepNode *)list.GetHead();
    }

    if (n)
    {
        newWeaponId = n->weaponId;

        if (n->weaponCount)
        {
            useEmpty = 0;
        }
    }

    // cleanup nodes
    while (n = (WeaponStepNode *)list.RemHead())
    {
        delete n;
    }


    // search for a hardpoint with a matching WeaponId
    // search inboard to outboard
    // 0 1 2 3 4 5
    // 0 1 2 3 4

    int startHp = (numHardpoints) / 2;
    int newHp = startHp;

    looped = 1;

    int l;

    while (startHp >= 0)
    {
        newHp = startHp;

        for (l = 0; l < 2; l++)
        {
            MonoPrint("Comparing Hp %d\n", newHp);

            if (newHp < numHardpoints and hardPoint[newHp]->weaponId == newWeaponId)
            {
                SimWeaponClass *weap = hardPoint[newHp]->weaponPointer.get();

                while (weap and not weap->IsUseable())
                {
                    // MLR 6/3/2004 - Skip unuseable weapons (LAUs)
                    weap = weap->GetNextOnRail();
                }

                if (weap or useEmpty)
                {
                    MonoPrint("Match Hp %d\n", newHp);
                    SetCurrentWeapon(newHp, weap);
                    looped = 0;
                    return;
                }
            }

            newHp = numHardpoints - newHp;
        }

        startHp--;
    }

    SetCurrentWeapon(-1, NULL);
    looped = 0;
}
#endif

WeaponType SMSBaseClass::GetCurrentWeaponType(void)
{
    if (curHardpoint < 0)
    {
        return wtNone;
    }

    return hardPoint[curHardpoint]->GetWeaponType();
}


int SMSClass::SetCurrentHpByWeaponId(int Id)
{
    int i;

    for (i = 0; i < numHardpoints; i++)
    {
        if (hardPoint[i]->weaponId == Id and 
            hardPoint[i]->weaponCount > 0)
        {
            SetCurrentWeapon(i);
            return(1);
        }
    }

    SetCurrentWeapon(-1);
    return(0);
}

int SMSClass::GetCurrentWeaponId(void)
{
    if (curHardpoint > -1)
        return hardPoint[curHardpoint]->weaponId;

    return 0;
}

// MLR 3/13/2004 - Set the current HP to hpId, if the hp is empty,
//                 find a similar HP that has stores
int SMSClass::SetCurrentHardPoint(int hpId, int findSimilar)
{

    SetCurrentWeapon(hpId);

    if (curWeapon)
        return TRUE;

    if (hpId < 0)
        return FALSE;

    if (findSimilar)
    {
        if (SetCurrentHpByWeaponId(hardPoint[hpId]->weaponId))
            return TRUE;
        else
        {
            // we have to do this because SetCurrentHpByWeaponId() will set the hp to -1
            SetCurrentWeapon(hpId);
            return FALSE;
        }
    }

    return FALSE;
}

