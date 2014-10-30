/*
 * Machine Generated source file for message "Missile Endgame".
 * NOTE: The functions here must be completed by hand.
 * Generated on 18-February-1997 at 15:12:33
 * Generated from file EVENTS.XLS by Leon Rosenshein
 */

#include "MsgInc/MissileEndMsg.h"
#include "mesg.h"
#include "otwdrive.h"
#include "sfx.h"
#include "fsound.h"
#include "soundfx.h"
#include "fakerand.h"
#include "simbase.h"
#include "classtbl.h"
#include "entity.h"
#include "campweap.h"
#include "Graphics/Include/terrtex.h"
#include "Graphics/Include/Renderow.h"
#include "falclib.h"
#include "falcmesg.h"
#include "falcgame.h"
#include "falcsess.h"
#include "InvalidBufferException.h"
#include "Graphics/Include/drawparticlesys.h"

static int random = 0;

FalconMissileEndMessage::FalconMissileEndMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) : FalconEvent(MissileEndMsg, FalconEvent::SimThread, entityId, target, loopback)
{
    // RequestReliableTransmit ();
    // RequestOutOfBandTransmit ();
    // Your Code Goes Here
    dataBlock.sfxPartSysName[0] = 0;
}

FalconMissileEndMessage::FalconMissileEndMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target) : FalconEvent(MissileEndMsg, FalconEvent::SimThread, senderid, target)
{
    // Your Code Goes Here
    type;
    dataBlock.sfxPartSysName[0] = 0;
}

FalconMissileEndMessage::~FalconMissileEndMessage(void)
{
    // Your Code Goes Here
}

int FalconMissileEndMessage::Process(uchar autodisp)
{
    // Your Code Goes Here
    // SimBaseClass *weapon;
    Tpoint pos, vec;
    Falcon4EntityClassType *classPtr;
    char sptype = -1;
    char type = -1;
    char stype = -1;
    WeaponClassDataType *wc;

    //RV - I-Hawk - Added a 0 vector for RV new PS calls
    Tpoint PSvec;
    PSvec.x = 0;
    PSvec.y = 0;
    PSvec.z = 0;

    // MLR 5/16/2004 - Sound Id numbers
    static int sid = 0;

    if (++sid > 100)
    {
        sid = 0;
    }

    // prevent handling messages if the graphics isn't running
    if (autodisp or not OTWDriver.IsActive())
    {
        return FALSE;
    }

    // get type of weapon
    // weapon = (SimBaseClass*)(vuDatabase->Find (dataBlock.fWeaponUID));
    // if (weapon)
    {

        classPtr = &Falcon4ClassTable[dataBlock.wIndex - VU_LAST_ENTITY_TYPE];
        // classPtr = (Falcon4EntityClassType*)weapon->EntityType();
        sptype = classPtr->vuClassData.classInfo_[VU_SPTYPE];
        type = classPtr->vuClassData.classInfo_[VU_TYPE];
        stype = classPtr->vuClassData.classInfo_[VU_STYPE];
        wc = (WeaponClassDataType *)classPtr->dataPtr;
    }

    pos.x = dataBlock.x;
    pos.y = dataBlock.y;
    pos.z = dataBlock.z;

    if (dataBlock.sfxPartSysName[0])
    {
        vec.x = dataBlock.xDelta * 1.0f;
        vec.y = dataBlock.yDelta * 1.0f;
        vec.z = dataBlock.zDelta * 1.0f;

        // if we fail, we need to run the original sfx
        if (AddParticleEffect(dataBlock.sfxPartSysName, &pos, &vec))
        {
            return TRUE;
        }
    }

    // we decide what special effects and sounds to play based on
    // what happened to the missile (or bomb)

    // first check to see if it hit ground and if its a missile
    if (dataBlock.groundType >= 0 and (type == TYPE_MISSILE))
    {
        pos.z -= 40.0f;

        if ( not (dataBlock.groundType == COVERAGE_WATER or
              dataBlock.groundType == COVERAGE_RIVER))
        {
            F4SoundFXSetPos(SFX_IMPACTG1 + PRANDInt6(), TRUE, pos.x, pos.y, pos.z, 1.0f, 0, sid);
            /*
            OTWDriver.AddSfxRequest(
             new SfxClass (SFX_GROUND_STRIKE, // type
             &pos, // world pos
             2.0f, // time to live
             100.0f ) ); // scale
            */
            DrawableParticleSys::PS_AddParticleEx((SFX_GROUND_STRIKE + 1),
                                                  &pos,
                                                  &PSvec);


        }
        else
        {
            F4SoundFXSetPos(SFX_SPLASH, TRUE, pos.x, pos.y, pos.z, 1.0f, 0, sid);
            /*
            OTWDriver.AddSfxRequest(
             new SfxClass (SFX_WATER_STRIKE, // type
             &pos, // world pos
             2.0f, // time to live
             100.0f ) ); // scale
            */
            DrawableParticleSys::PS_AddParticleEx((SFX_WATER_STRIKE + 1),
                                                  &pos,
                                                  &PSvec);
        }

        return TRUE;
    }

    // now do separate out by type of weapon

    // MISSILES
    /*
    if ( type == TYPE_MISSILE and stype == STYPE_ROCKET  )
    {
     // try and "back up" the placement for explosion so that
     // it will sort before the feature
     pos.x -= dataBlock.xDelta * 0.22f;
     pos.y -= dataBlock.yDelta * 0.22f;
     pos.z -= dataBlock.zDelta * 0.22f;
     vec.x = dataBlock.xDelta * 1.0f;
     vec.y = dataBlock.yDelta * 1.0f;
     vec.z = dataBlock.zDelta * 1.0f;
     OTWDriver.AddSfxRequest(
     new SfxClass (SFX_ROCKET_BURST, // type
     0, // flags
     &pos, // world pos
     &vec, // movement
     1.5f, // time to live
     20.0f ) ); // scale
    }
    */
    if (type == TYPE_MISSILE)
    {
        switch (dataBlock.endCode)
        {
            case MissileKill:
                vec.x = dataBlock.xDelta * 1.0f;
                vec.y = dataBlock.yDelta * 1.0f;
                vec.z = dataBlock.zDelta * 1.0f;

                if (wc->DamageType == PenetrationDam)
                {
                    /*
                    OTWDriver.AddSfxRequest(
                     new SfxClass (SFX_AIR_PENETRATION, // type
                     &pos, // world pos
                     1.5f, // time to live
                     60.0f ) ); // scale
                    */
                    DrawableParticleSys::PS_AddParticleEx((SFX_AIR_PENETRATION + 1),
                                                          &pos,
                                                          &PSvec);
                }
                else
                {
                    /*
                    OTWDriver.AddSfxRequest(
                     new SfxClass (SFX_MISSILE_BURST, // type
                     0, // flags
                     &pos, // world pos
                     &vec, // movement
                     1.5f, // time to live
                     40.0f ) ); // scale
                     */
                    DrawableParticleSys::PS_AddParticleEx((SFX_MISSILE_BURST + 1),
                                                          &pos,
                                                          &vec);
                }

                // sound effect
                F4SoundFXSetPos(SFX_IMPACTA1 + PRANDInt5(), TRUE, pos.x, pos.y, pos.z, 1.0f, 0, sid);

                break;

            case FeatureImpact:
                // try and "back up" the placement for explosion so that
                // it will sort before the feature
                pos.x -= dataBlock.xDelta * 0.12f;
                pos.y -= dataBlock.yDelta * 0.12f;
                pos.z -= dataBlock.zDelta * 0.12f;
                /*
                OTWDriver.AddSfxRequest(
                 new SfxClass (SFX_AIR_PENETRATION, // type
                 &pos, // world pos
                 1.5f, // time to live
                 60.0f ) ); // scale
                 */
                DrawableParticleSys::PS_AddParticleEx((SFX_AIR_PENETRATION + 1),
                                                      &pos,
                                                      &PSvec);

                // sound effect
                F4SoundFXSetPos(SFX_IMPACTA1 + PRANDInt5(), TRUE, pos.x, pos.y, pos.z, 1.0f, 0, sid);
                break;

                //case MinSpeed:
                //case ExceedTime:
            case MinTime:
            case ArmingDelay: // added 2002-03-28 MN
                break;

            case BombImpact: // added 2002-03-28 MN
                switch (wc->DamageType)
                {
                    case HeaveDam:
                    case PenetrationDam:
                    case KineticDam:
                        /*
                        OTWDriver.AddSfxRequest(
                         new SfxClass(SFX_GROUND_PENETRATION, // type
                         &pos, // world pos
                         2.0f, // time to live
                         (float)wc->BlastRadius ) ); // scale
                         */
                        DrawableParticleSys::PS_AddParticleEx((SFX_GROUND_PENETRATION + 1),
                                                              &pos,
                                                              &PSvec);

                        break;

                    case NuclearDam:
                    case HighExplosiveDam:
                        /*
                        OTWDriver.AddSfxRequest(
                         new SfxClass(SFX_GROUND_EXPLOSION, // type
                         &pos, // world pos
                         2.0f, // time to live
                         (float)wc->BlastRadius ) ); // scale
                         */
                        DrawableParticleSys::PS_AddParticleEx((SFX_GROUND_EXPLOSION + 1),
                                                              &pos,
                                                              &PSvec);
                        break;

                    case IncendairyDam:
                        vec.x = dataBlock.xDelta * 2.0f;
                        vec.y = dataBlock.yDelta * 2.0f;
                        vec.z = 0.0f;
                        /*
                        OTWDriver.AddSfxRequest(
                         new SfxClass (SFX_NAPALM, // type
                         &pos, // world pos
                         &vec,
                         4, // # rings
                         0.2f ) ); // interval
                         */
                        DrawableParticleSys::PS_AddParticleEx((SFX_NAPALM + 1),
                                                              &pos,
                                                              &vec);
                        break;

                    case ProximityDam:
                    case HydrostaticDam:
                    case ChemicalDam:
                    case OtherDam:
                    case NoDamage:
                    default:
                        //RV - I-Hawk - Replaced type to SFX_SHAPED_FIRE_DEBRIS, this will be
                        //a missile default destruct in PS file, if none other explos is used
                        /*
                        OTWDriver.AddSfxRequest(
                         new SfxClass (SFX_SHAPED_FIRE_DEBRIS, // type
                         &pos, // world pos
                         1.5f, // time to live
                         40.0f ) ); // scale
                         */
                        DrawableParticleSys::PS_AddParticleEx((SFX_SHAPED_FIRE_DEBRIS + 1),
                                                              &pos,
                                                              &PSvec);
                        break;
                } // end switch

                break;

            case MinSpeed:
            case ExceedTime:
            case ExceedFOV:
            case ExceedGimbal:
            case GroundImpact:
            case Missed:
            default:
                //RV - I-Hawk - Replaced type to SFX_SHAPED_FIRE_DEBRIS, this will be
                //a missile default destruct in PS file, if none other explos is used
                /*
                OTWDriver.AddSfxRequest(
                 new SfxClass (SFX_SHAPED_FIRE_DEBRIS, // type
                 &pos, // world pos
                 1.5f, // time to live
                 40.0f ) ); // scale
                 */
                DrawableParticleSys::PS_AddParticleEx((SFX_SHAPED_FIRE_DEBRIS + 1),
                                                      &pos,
                                                      &PSvec);

                // sound effect
                F4SoundFXSetPos(SFX_IMPACTA1 + PRANDInt5(), TRUE, pos.x, pos.y, pos.z, 1.0f, 0, sid);

                break;
        }
    }

    // ROCKETS
    else if (type == TYPE_ROCKET)
    {
        // try and "back up" the placement for explosion so that
        // it will sort before the feature
        pos.x -= dataBlock.xDelta * 0.32f;
        pos.y -= dataBlock.yDelta * 0.32f;
        pos.z -= dataBlock.zDelta * 0.32f;
        vec.x = dataBlock.xDelta * 1.0f;
        vec.y = dataBlock.yDelta * 1.0f;
        vec.z = dataBlock.zDelta * 1.0f;
        /*
        OTWDriver.AddSfxRequest(
         new SfxClass (SFX_ROCKET_BURST, // type
         0, // flags
         &pos, // world pos
         &vec, // movement
         1.5f, // time to live
         20.0f ) ); //
         */
        DrawableParticleSys::PS_AddParticleEx((SFX_ROCKET_BURST + 1),
                                              &pos,
                                              &vec);
        /*
        switch( dataBlock.endCode )
        {
         case FeatureImpact:
         pos.x -= dataBlock.xDelta * 0.32f;
         pos.y -= dataBlock.yDelta * 0.32f;
         pos.z -= dataBlock.zDelta * 0.32f;
         case MissileKill:
         OTWDriver.AddSfxRequest(
         new SfxClass (SFX_AIR_PENETRATION, // type
         &pos, // world pos
         1.5f, // time to live
         60.0f ) ); // scale
         // sound effect
         F4SoundFXSetPos( SFX_IMPACTA1 + PRANDInt5(), TRUE, pos.x, pos.y, pos.z, 1.0f, 0, sid );
         break;
         case ExceedFOV:
         case ExceedGimbal:
         case GroundImpact:
         case MinTime:
         case Missed:
         case MinSpeed:
         case ExceedTime:
         default:
         OTWDriver.AddSfxRequest(
         new SfxClass (SFX_SMALL_HIT_EXPLOSION, // type
         &pos, // world pos
         1.5f, // time to live
         40.0f ) ); // scale

         // sound effect
         F4SoundFXSetPos( SFX_IMPACTA1 + PRANDInt5(), TRUE, pos.x, pos.y, pos.z, 1.0f, 0, sid );
         break;
        }
        */
    }
    // GUN SHELLS
    else if (type == TYPE_GUN)
    {
        // hit water?
        if ((dataBlock.groundType == COVERAGE_WATER or
             dataBlock.groundType == COVERAGE_RIVER))
        {
            F4SoundFXSetPos(SFX_SPLASH, TRUE, pos.x, pos.y, pos.z, 1.0f, 0, sid);
            F4SoundFXSetPos(SFX_IMPACTG4 + PRANDInt3(), TRUE, pos.x, pos.y, pos.z, 1.0f, 0, sid);
            /*
            OTWDriver.AddSfxRequest(
             new SfxClass (SFX_WATER_EXPLOSION, // type
             &pos, // world pos
             2.0f, // time to live
             50.0f ) ); // scale
             */
            DrawableParticleSys::PS_AddParticleEx((SFX_WATER_EXPLOSION + 1),
                                                  &pos,
                                                  &PSvec);
            return TRUE;
        }

        // otherwise, do it based on damage type
        switch (wc->DamageType)
        {
            case HeaveDam:
            case PenetrationDam:
            case KineticDam:
                if (dataBlock.groundType >= 0)
                {
                    /*
                    OTWDriver.AddSfxRequest(
                     new SfxClass(SFX_ARTILLERY_EXPLOSION, // type
                     &pos, // world pos
                     2.0f, // time to live
                     100.0f ) ); // scale
                     */
                    DrawableParticleSys::PS_AddParticleEx((SFX_ARTILLERY_EXPLOSION + 1),
                                                          &pos,
                                                          &PSvec);
                    // play sound
                    F4SoundFXSetPos(SFX_IMPACTG4 + PRANDInt3(), TRUE, pos.x, pos.y, pos.z, 1.0f, 0, sid);
                }

                // don't play any effect for air penetration misses...
                else // if ( dataBlock.endCode not_eq Missed )
                {
                    /*
                    OTWDriver.AddSfxRequest(
                     new SfxClass(SFX_AIR_PENETRATION, // type
                     &pos, // world pos
                     2.0f, // time to live
                     40.0f ) ); // scale

                    OTWDriver.AddSfxRequest(
                     new SfxClass(SFX_AIRBURST, // type
                     &pos, // world pos
                     10, // time to live
                     0.4f ) ); // scale

                    OTWDriver.AddSfxRequest(
                     new SfxClass(SFX_SPARKS, // type
                     &pos, // world pos
                     2.0f, // time to live
                     40.0f ) ); // scale
                    */
                    //RV - I-Hawk - Replaced type to SFX_SHAPED_FIRE_DEBRIS, this will be
                    //a missile default destruct in PS file, if none other explos is used

                    /*
                    OTWDriver.AddSfxRequest(
                     new SfxClass (SFX_SHAPED_FIRE_DEBRIS, // type
                     &pos, // world pos
                     1.5f, // time to live
                     40.0f ) ); // scale
                     */
                    DrawableParticleSys::PS_AddParticleEx((SFX_SHAPED_FIRE_DEBRIS + 1),
                                                          &pos,
                                                          &PSvec);

                    /*
                    OTWDriver.AddSfxRequest(
                     new SfxClass(SFX_LONG_HANGING_SMOKE2, // type
                     &pos, // world pos
                     20.0f, // time to live
                     14.0f ) ); // scale
                     */
                    DrawableParticleSys::PS_AddParticleEx((SFX_LONG_HANGING_SMOKE2 + 1),
                                                          &pos,
                                                          &PSvec);

                    // sound effect
                    F4SoundFXSetPos(SFX_IMPACTA1 + PRANDInt5(), TRUE, pos.x, pos.y, pos.z, 1.0f, 0, sid);
                }

                break;

            case NuclearDam:
            case HighExplosiveDam:
            case ProximityDam:
                if (dataBlock.groundType >= 0)
                {
                    /*
                    OTWDriver.AddSfxRequest(
                     new SfxClass(SFX_GROUND_STRIKE_NOFIRE, // type
                     &pos, // world pos
                     2.0f, // time to live
                     min( 100.0f, (float)wc->BlastRadius ) ) ); // scale
                     */
                    DrawableParticleSys::PS_AddParticleEx((SFX_GROUND_STRIKE_NOFIRE + 1),
                                                          &pos,
                                                          &PSvec);
                    // play sound
                    F4SoundFXSetPos(SFX_IMPACTG4 + PRANDInt3(), TRUE, pos.x, pos.y, pos.z, 1.0f, 0, sid);
                }
                else
                {
                    /*
                    OTWDriver.AddSfxRequest(
                     new SfxClass(SFX_AIRBURST, // type
                     &pos, // world pos
                     16, // time to live
                     0.3f ) ); // scale
                     */
                    DrawableParticleSys::PS_AddParticleEx((SFX_AIRBURST + 1),
                                                          &pos,
                                                          &PSvec);

                    // sound effect
                    F4SoundFXSetPos(SFX_IMPACTA1 + PRANDInt5(), TRUE, pos.x, pos.y, pos.z, 1.0f, 0, sid);
                }

                break;

            case IncendairyDam:
            case HydrostaticDam:
            case ChemicalDam:
            case OtherDam:
            case NoDamage:
            default:
                //RV - I-Hawk - Type here should be SMALL_HIT_EXPLOSION so we won't have
                //the large effects with AAA fire
                /*
                OTWDriver.AddSfxRequest(
                 new SfxClass (SFX_SMALL_HIT_EXPLOSION, // type
                 &pos, // world pos
                 1.5f, // time to live
                 40.0f ) ); // scale
                 */
                DrawableParticleSys::PS_AddParticleEx((SFX_SMALL_HIT_EXPLOSION + 1),
                                                      &pos,
                                                      &PSvec);
                // sound effect
                F4SoundFXSetPos(SFX_IMPACTA1 + PRANDInt5(), TRUE, pos.x, pos.y, pos.z, 1.0f, 0, sid);
                break;
        } // end switch

    }
    // EVERYTHING ELSE IS ASSUMED TO BE A BOMB
    else
    {
        //Cobra TJL add in SFX_NUKE and comment out all these.
        //MI Nuke explosions
        if (wc and wc->DamageType == NuclearDam)
        {
            /*
            OTWDriver.AddSfxRequest(
             new SfxClass(SFX_NUKE, // type
             &pos, // world pos
             50.0f, // time to live
             (float)wc->BlastRadius * 50.0F) ); // scale
             */
            DrawableParticleSys::PS_AddParticleEx((SFX_NUKE + 1),
                                                  &pos,
                                                  &PSvec);

            //Cobra TJL 11/06/04 comment out the old Nuke Effect per Steve and new PS
            /*OTWDriver.AddSfxRequest(
             new SfxClass(SFX_GROUND_EXPLOSION, // type
             &pos, // world pos
             50.0f, // time to live
             (float)wc->BlastRadius * 50.0F) ); // scale
            OTWDriver.AddSfxRequest(
             new SfxClass(SFX_GROUND_EXPLOSION, // type
             &pos, // world pos
             50.0f, // time to live
             (float)wc->BlastRadius * 50.0F) ); // scale
            OTWDriver.AddSfxRequest(
             new SfxClass(SFX_GROUND_EXPLOSION, // type
             &pos, // world pos
             50.0f, // time to live
             (float)wc->BlastRadius * 50.0F) ); // scale
            OTWDriver.AddSfxRequest(
             new SfxClass(SFX_GROUND_EXPLOSION, // type
             &pos, // world pos
             50.0f, // time to live
             (float)wc->BlastRadius * 50.0F) ); // scale
            OTWDriver.AddSfxRequest(
             new SfxClass(SFX_GROUND_EXPLOSION, // type
             &pos, // world pos
             50.0f, // time to live
             (float)wc->BlastRadius * 50.0F) ); // scale
            OTWDriver.AddSfxRequest(
             new SfxClass(SFX_LONG_HANGING_SMOKE2, // type
             &pos, // world pos
             2000.0f, // time to live
             20.0f ) ); // scale
            OTWDriver.AddSfxRequest(
             new SfxClass(SFX_LONG_HANGING_SMOKE2, // type
             &pos, // world pos
             2000.0f, // time to live
             20.0f ) ); // scale
            OTWDriver.AddSfxRequest(
             new SfxClass(SFX_LONG_HANGING_SMOKE2, // type
             &pos, // world pos
             2000.0f, // time to live
             20.0f ) ); // scale
            OTWDriver.AddSfxRequest(
             new SfxClass(SFX_LONG_HANGING_SMOKE2, // type
             &pos, // world pos
             2000.0f, // time to live
             20.0f ) ); // scale
            OTWDriver.AddSfxRequest(
             new SfxClass(SFX_LONG_HANGING_SMOKE2, // type
             &pos, // world pos
             2000.0f, // time to live
             20.0f ) ); // scale
            OTWDriver.AddSfxRequest(
             new SfxClass(SFX_GROUND_EXPLOSION, // type
             &pos, // world pos
             50.0f, // time to live
             (float)wc->BlastRadius * 50.0F) ); // scale

            OTWDriver.AddSfxRequest(
             new SfxClass(SFX_LONG_HANGING_SMOKE2, // type
             &pos, // world pos
             2000.0f, // time to live
             20.0f ) ); // scale
            OTWDriver.AddSfxRequest(
             new SfxClass(SFX_LONG_HANGING_SMOKE2, // type
             &pos, // world pos
             2000.0f, // time to live
             20.0f ) ); // scale
            OTWDriver.AddSfxRequest(
             new SfxClass(SFX_LONG_HANGING_SMOKE2, // type
             &pos, // world pos
             2000.0f, // time to live
             20.0f ) ); // scale
            OTWDriver.AddSfxRequest(
             new SfxClass(SFX_LONG_HANGING_SMOKE2, // type
             &pos, // world pos
             2000.0f, // time to live
             20.0f ) ); // scale
            OTWDriver.AddSfxRequest(
             new SfxClass(SFX_LONG_HANGING_SMOKE2, // type
             &pos, // world pos
             2000.0f, // time to live
             20.0f ) ); // scale
            OTWDriver.AddSfxRequest(
             new SfxClass(SFX_LONG_HANGING_SMOKE2, // type
             &pos, // world pos
             2000.0f, // time to live
             20.0f ) ); // scale
            OTWDriver.AddSfxRequest(
             new SfxClass(SFX_LONG_HANGING_SMOKE2, // type
             &pos, // world pos
             2000.0f, // time to live
             20.0f ) ); // scale
            OTWDriver.AddSfxRequest(
             new SfxClass(SFX_LONG_HANGING_SMOKE2, // type
             &pos, // world pos
             2000.0f, // time to live
             20.0f ) ); // scale
            OTWDriver.AddSfxRequest(
             new SfxClass(SFX_LONG_HANGING_SMOKE2, // type
             &pos, // world pos
             2000.0f, // time to live
             20.0f ) ); // scale
            OTWDriver.AddSfxRequest(
             new SfxClass(SFX_LONG_HANGING_SMOKE2, // type
             &pos, // world pos
             2000.0f, // time to live
             20.0f ) ); // scale
            vec.x = -dataBlock.xDelta * 1.0f;
            vec.y = dataBlock.yDelta * 1.0f;
            vec.z = dataBlock.zDelta * 1.0f;
            OTWDriver.AddSfxRequest(
             new SfxClass (SFX_NAPALM, // type
             &pos, // world pos
             &vec,
             20, // # rings
             0.2f ) ); // interval
            OTWDriver.AddSfxRequest(
             new SfxClass (SFX_NAPALM, // type
             &pos, // world pos
             &vec,
             20, // # rings
             0.2f ) ); // interval
            vec.x = dataBlock.xDelta * 1.0f;
            vec.y = dataBlock.yDelta * 1.0f;
            vec.z = dataBlock.zDelta * 1.0f;
            OTWDriver.AddSfxRequest(
             new SfxClass (SFX_NAPALM, // type
             &pos, // world pos
             &vec,
             20, // # rings
             0.2f ) ); // interval
            OTWDriver.AddSfxRequest(
             new SfxClass (SFX_NAPALM, // type
             &pos, // world pos
             &vec,
             20, // # rings
             0.2f ) ); // interval
            vec.x = dataBlock.xDelta * 1.0f;
            vec.y = -dataBlock.yDelta * 1.0f;
            vec.z = dataBlock.zDelta * 1.0f;
            OTWDriver.AddSfxRequest(
             new SfxClass (SFX_NAPALM, // type
             &pos, // world pos
             &vec,
             20, // # rings
             0.2f ) ); // interval
            OTWDriver.AddSfxRequest(
             new SfxClass (SFX_NAPALM, // type
             &pos, // world pos
             &vec,
             20, // # rings
             0.2f ) ); // interval
            vec.x = -dataBlock.xDelta * 1.0f;
            vec.y = -dataBlock.yDelta * 1.0f;
            vec.z = dataBlock.zDelta * 1.0f;
            OTWDriver.AddSfxRequest(
             new SfxClass (SFX_NAPALM, // type
             &pos, // world pos
             &vec,
             20, // # rings
             0.2f ) ); // interval
            OTWDriver.AddSfxRequest(
             new SfxClass (SFX_NAPALM, // type
             &pos, // world pos
             &vec,
             20, // # rings
             0.2f ) ); // interval*/

            // play sound
            F4SoundFXSetPos(SFX_IMPACTG4 + PRANDInt3(), TRUE, pos.x, pos.y, pos.z, 1.0f, 0, sid);
            return TRUE;
        }

        // hit water?
        if ((dataBlock.groundType == COVERAGE_WATER or
             dataBlock.groundType == COVERAGE_RIVER))
        {
            F4SoundFXSetPos(SFX_SPLASH, TRUE, pos.x, pos.y, pos.z, 1.0f);
            F4SoundFXSetPos(SFX_IMPACTG4 + PRANDInt3(), TRUE, pos.x, pos.y, pos.z, 1.0f, 0, sid);
            /*
            OTWDriver.AddSfxRequest(
             new SfxClass (SFX_WATER_EXPLOSION, // type
             &pos, // world pos
             2.0f, // time to live
             (float)wc->BlastRadius ) ); // scale
             */
            DrawableParticleSys::PS_AddParticleEx((SFX_WATER_EXPLOSION + 1),
                                                  &pos,
                                                  &PSvec);
            return TRUE;
        }

        // hit building
        if (dataBlock.endCode == FeatureImpact and wc->DamageType not_eq HighExplosiveDam)
        {
            pos.x -= dataBlock.xDelta * 0.32f;
            pos.y -= dataBlock.yDelta * 0.32f;
            pos.z -= dataBlock.zDelta * 0.32f;
            /*
            OTWDriver.AddSfxRequest(
             new SfxClass (SFX_AIR_PENETRATION, // type
             &pos, // world pos
             1.5f, // time to live
             60.0f ) ); // scale
             */
            DrawableParticleSys::PS_AddParticleEx((SFX_AIR_PENETRATION + 1),
                                                  &pos,
                                                  &PSvec);
            // sound effect
            F4SoundFXSetPos(SFX_IMPACTA1 + PRANDInt5(), TRUE, pos.x, pos.y, pos.z, 1.0f, 0, sid);
            return TRUE;
        }

        // check for durandal firing
        if (sptype == SPTYPE_DURANDAL)
        {
            if (dataBlock.groundType == -1)
            {
                vec.x = dataBlock.xDelta ;
                vec.y = dataBlock.yDelta ;
                vec.z = dataBlock.zDelta ;
                /*
                OTWDriver.AddSfxRequest(
                 new SfxClass (SFX_DURANDAL, // type
                 SFX_MOVES, // flags
                 &pos, // world pos
                 &vec,
                 15.0f, // time to live
                 20.1f ) ); // scale
                 */
                DrawableParticleSys::PS_AddParticleEx((SFX_DURANDAL + 1),
                                                      &pos,
                                                      &vec);
            }
            else
            {
                /*
                OTWDriver.AddSfxRequest(
                 new SfxClass(SFX_GROUND_PENETRATION, // type
                 &pos, // world pos
                 2.0f, // time to live
                 (float)wc->BlastRadius ) ); // scale
                 */
                DrawableParticleSys::PS_AddParticleEx((SFX_GROUND_PENETRATION + 1),
                                                      &pos,
                                                      &PSvec);
            }

            return TRUE;
        }
        // check for cluster bombs
        // at them moment we have no way to scale them
        else if (wc->Flags bitand WEAP_CLUSTER)
            //if (TRUE)
        {
            // if ground type is -1 we've exploded in air
            // do cluster bomb effect
            if (dataBlock.groundType == -1)
            {
                /*
                vec.x = dataBlock.xDelta * 2.0f;
                vec.y = dataBlock.yDelta * 2.0f;
                vec.z = 0.0f;
                OTWDriver.AddSfxRequest(
                 new SfxClass (SFX_GROUNDBURST, // type
                 &pos, // world pos
                 &vec,
                 8,
                 0.1f ) ); // interval
                */
                vec.x = dataBlock.xDelta ;
                vec.y = dataBlock.yDelta ;
                vec.z = dataBlock.zDelta ;
                /*
                OTWDriver.AddSfxRequest(
                 new SfxClass (SFX_CLUSTER_BOMB, // type
                 0, // flags
                 &pos, // world pos
                 &vec,
                 15.0f, // time to live
                 20.1f ) ); // scale
                 */
                DrawableParticleSys::PS_AddParticleEx((SFX_CLUSTER_BOMB + 1),
                                                      &pos,
                                                      &vec);
                return TRUE;
            }
            else
            {
                /*
                OTWDriver.AddSfxRequest(
                 new SfxClass(SFX_GROUND_PENETRATION, // type
                 &pos, // world pos
                 2.0f, // time to live
                 (float)wc->BlastRadius ) ); // scale
                 */
                DrawableParticleSys::PS_AddParticleEx((SFX_GROUND_PENETRATION + 1),
                                                      &pos,
                                                      &PSvec);
            }
        }
        // otherwise, do it based on damage type
        else switch (wc->DamageType)
            {
                case HeaveDam:
                case PenetrationDam:
                case KineticDam:
                    /*
                    OTWDriver.AddSfxRequest(
                     new SfxClass(SFX_GROUND_PENETRATION, // type
                     &pos, // world pos
                     2.0f, // time to live
                     (float)wc->BlastRadius ) ); // scale
                     */
                    DrawableParticleSys::PS_AddParticleEx((SFX_GROUND_PENETRATION + 1),
                                                          &pos,
                                                          &PSvec);
                    break;

                case NuclearDam:
                case HighExplosiveDam:
                    /*
                    OTWDriver.AddSfxRequest(
                     new SfxClass(SFX_GROUND_EXPLOSION, // type
                     &pos, // world pos
                     2.0f, // time to live
                     (float)wc->BlastRadius ) ); // scale
                     */
                    DrawableParticleSys::PS_AddParticleEx((SFX_GROUND_EXPLOSION + 1),
                                                          &pos,
                                                          &PSvec);
                    break;

                case IncendairyDam:
                    vec.x = dataBlock.xDelta * 2.0f;
                    vec.y = dataBlock.yDelta * 2.0f;
                    vec.z = 0.0f;
                    /*
                    OTWDriver.AddSfxRequest(
                     new SfxClass (SFX_NAPALM, // type
                     &pos, // world pos
                     &vec,
                     4, // # rings
                     0.2f ) ); // interval
                     */
                    DrawableParticleSys::PS_AddParticleEx((SFX_NAPALM + 1),
                                                          &pos,
                                                          &vec);
                    break;

                case ProximityDam:
                case HydrostaticDam:
                case ChemicalDam:
                case OtherDam:
                case NoDamage:
                default:
                    //RV - I-Hawk - Replaced type to SFX_SHAPED_FIRE_DEBRIS, this will be
                    //a default destruct effect in PS file, if none other explos is used

                    /*
                    OTWDriver.AddSfxRequest(
                     new SfxClass (SFX_SHAPED_FIRE_DEBRIS, // type
                     &pos, // world pos
                     1.5f, // time to live
                     40.0f ) ); // scale
                     */
                    DrawableParticleSys::PS_AddParticleEx((SFX_SHAPED_FIRE_DEBRIS + 1),
                                                          &pos,
                                                          &PSvec);
                    break;
            } // end switch

        // play sound
        F4SoundFXSetPos(SFX_IMPACTG4 + PRANDInt3(), TRUE, pos.x, pos.y, pos.z, 1.0f, 0, sid);
    } // end else bombs


    return TRUE;


}


void FalconMissileEndMessage::SetParticleEffectName(char *name)
{
    if (name)
    {
        if (name[0])
        {
            strncpy(dataBlock.sfxPartSysName, name, 19);
            dataBlock.sfxPartSysName[19] = 0;
        }
    }
}

