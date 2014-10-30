#include "stdhdr.h"
#include "Graphics/Include/drawparticlesys.h"
#include "classtbl.h"
#include "entity.h"
#include "simveh.h"
#include "otwdrive.h"
#include "initdata.h"
#include "simbrain.h"
#include "PilotInputs.h"
#include "object.h"
#include "sms.h"
#include "fcc.h"
#include "guns.h"
#include "misslist.h"
#include "bombfunc.h"
#include "gunsfunc.h"
#include "simvudrv.h"
#include "hardpnt.h"
#include "missile.h"
#include "mavdisp.h"
#include "hud.h"
#include "sensclas.h"
#include "simdrive.h"
#include "fsound.h"
#include "soundfx.h"
#include "MsgInc/DamageMsg.h"
#include "MsgInc/DeathMessage.h"
#include "MsgInc/WeaponFireMsg.h"
#include "MsgInc/RadioChatterMsg.h"
#include "campbase.h"
#include "sfx.h"
#include "wpndef.h"
#include "fakerand.h"
#include "eyeball.h"
#include "radarDigi.h"
#include "VehRwr.h"
#include "irst.h"
#include "acmi/src/include/acmirec.h"
#include "Graphics/Include/drawbsp.h"
#include "Graphics/Include/rviewpnt.h"
#include "playerop.h"
#include "camp2sim.h"
#include "falcsess.h"
#include "airframe.h"
#include "aircrft.h"
#include "camplist.h"
#include "Graphics/Include/terrtex.h"
#include "camp2sim.h"
#include "airunit.h"
#include "rules.h"
#include "falcsnd/voicemanager.h"
#include "campweap.h"
#include "drawable.h"
#include "aircrft.h"//Cobra test

extern SensorClass* FindLaserPod(SimMoverClass* theObject);
extern VehicleClassDataType* GetVehicleClassData(int index);
void GetCallsign(int id, int num, _TCHAR* callsign);  // from campui

extern bool g_bNewDamageEffects; // JB 000816
extern bool g_bDisableFunkyChicken; // JB 000820
extern bool g_bRealisticAvionics;
#include "ui/include/uicomms.h" // JB 010104
extern UIComms *gCommsMgr; // JB 010104


SimVehicleClass::SimVehicleClass(FILE* filePtr) : SimMoverClass(filePtr)
{
    InitLocalData();
}

SimVehicleClass::SimVehicleClass(VU_BYTE** stream, long *rem) : SimMoverClass(stream, rem)
{
    InitLocalData();
}

SimVehicleClass::SimVehicleClass(int type) : SimMoverClass(type)
{
    InitLocalData();
}

SimVehicleClass::~SimVehicleClass(void)
{
    CleanupLocalData();

    // sfr: @todo move to cleanup
    // Dispose waypoints
    WayPointClass* tmpWaypoint;
    curWaypoint = waypoint;

    while (curWaypoint)
    {
        tmpWaypoint = curWaypoint;
        curWaypoint = curWaypoint->GetNextWP();
        delete(tmpWaypoint);
    }
}


void SimVehicleClass::InitData()
{
    SimMoverClass::InitData();
    InitLocalData();
}

void SimVehicleClass::InitLocalData(void)
{
    Falcon4EntityClassType* classPtr = (Falcon4EntityClassType*)EntityType();
    VehicleClassDataType *vc;

    vc = (VehicleClassDataType *)classPtr->dataPtr;

    strength = maxStrength = (float)vc->HitPoints;
    pctStrength = 1.0;
    dyingTimer = -1.0f;
    dyingTime = 0; //RV - I-Hawk
    sfxTimer = 0.0f;
    ioPerturb = 0.0f;
    irOutput  = 0.0f;//me123
    // JB 000814
    rBias = 0.0f;
    pBias = 0.0f;
    yBias = 0.0f;
    // JB 000814

    theBrain = NULL;
    theInputs = NULL;
    deathMessage = NULL;
}

void SimVehicleClass::CleanupLocalData()
{
    delete(theBrain);
    theBrain = NULL;
    delete(theInputs);
    theInputs = NULL;
}


void SimVehicleClass::CleanupData()
{
    CleanupLocalData();
    // base class cleanup
    SimMoverClass::CleanupData();
}


void SimVehicleClass::Init(SimInitDataClass* initData)
{
    SimMoverClass::Init(initData);
    int i;

    if ( not IsAirplane())
    {
        // Create Sensors
        if (mvrDefinition)
        {
            numSensors = mvrDefinition->numSensors;
        }
        else
        {
            numSensors = 0;
        }

        // Note, until there is a vehicle definition for radar vehicles
        // do it as a special case here and below.
        if (IsGroundVehicle() and GetRadarType() not_eq RDR_NO_RADAR)
        {
            sensorArray = new SensorClass*[numSensors + 1];
        }
        else
        {
            sensorArray = new SensorClass*[numSensors];
        }

        for (i = 0; i < numSensors and mvrDefinition and mvrDefinition->sensorData; i++)
        {
            switch (mvrDefinition->sensorData[i * 2])
            {
                case SensorClass::Radar:
                    sensorArray[i] = new RadarDigiClass(GetRadarType(), this);
                    break;

                case SensorClass::RWR:
                    sensorArray[i] = new VehRwrClass(mvrDefinition->sensorData[i * 2 + 1], this);
                    break;

                case SensorClass::IRST:
                    sensorArray[i] = new IrstClass(mvrDefinition->sensorData[i * 2 + 1], this);
                    break;

                case SensorClass::Visual:
                    sensorArray[i] = new EyeballClass(mvrDefinition->sensorData[i * 2 + 1], this);
                    break;

                default:
                    ShiWarning("Unhandled sensor type during Init");
                    break;
            }
        }

        // Note, until there is a vehicle definition for radar vehicles
        // do it as a special case here and above.
        if (IsGroundVehicle() and GetRadarType() not_eq RDR_NO_RADAR)
        {
            numSensors ++;
            sensorArray[i] = new RadarDigiClass(GetRadarType(), this);
        }
    }
}

int SimVehicleClass::Wake(void)
{
    //int retval = 0;
    if (IsAwake())
    {
        return 0;
    }

    return SimMoverClass::Wake();
}

int SimVehicleClass::Sleep(void)
{
    int retval = 0;

    if ( not IsAwake())
    {
        return retval;
    }

    SimMoverClass::Sleep();

    if (theBrain)
    {
        theBrain->Sleep();
    }

    return retval;
}


void SimVehicleClass::MakeLocal()
{
    // Nothing particularly interesting here right now.
    SimMoverClass::MakeLocal();
}

void SimVehicleClass::MakeRemote()
{
    // Nothing particularly interesting here right now.
    SimMoverClass::MakeRemote();
}

WayPointClass *SimVehicleClass::GetWayPointNo(int n)
{
    WayPointClass *wp;

    //MI check it
    if (n <= 0)
        return NULL;

    int wpno = n - 1;

    for (wp = waypoint; wp; wp = wp->GetNextWP())
    {
        if (wpno <= 0)
            return wp;

        wpno --;
    }

    return NULL;
}

float SimVehicleClass::GetRCSFactor(void)
{
    VehicleClassDataType *vc = (VehicleClassDataType *)Falcon4ClassTable[Type() - VU_LAST_ENTITY_TYPE].dataPtr;
    ShiAssert(vc);

    return vc->RCSfactor;
}

float SimVehicleClass::GetIRFactor(void)
{
    // 2000-11-24 REWRITTEN BY S.G. SO IT USES THE NEW IR FIELD AND THE NEW ENGINE TEMPERATURE. THIS ALSO MAKES IT COMPATIBLE WITH RP4 MP.
#if 1
    // if ( not g_bHardCoreReal) MI
    if ( not g_bRealisticAvionics)
    {
        VehicleClassDataType *vc = (VehicleClassDataType *)Falcon4ClassTable[Type() - VU_LAST_ENTITY_TYPE].dataPtr;
        ShiAssert(vc);

        return vc->RCSfactor;
    }
    else
    {
        VehicleClassDataType *vc = (VehicleClassDataType *)Falcon4ClassTable[Type() - VU_LAST_ENTITY_TYPE].dataPtr;
        ShiAssert(vc);
        // TODO:  THis needs to return IR data
        // The IR signature is stored as a byte at offset 0x9E of the falcon4.vcd structure.
        // This byte, as well as 0x9D and 0x9F are used for padding originally.
        // The value range will be 0 to 2 with increments of 0.0078125
        unsigned char *pIrSign = (unsigned char *)vc;
        int iIrSign = (unsigned)pIrSign[0x9E];
        float irSignature = 1.0f;

        if (iIrSign)
            irSignature = (float)iIrSign / 128.0f;

        if (PowerOutput() <= 1.0) //me123 status test. differensiate between idle, mil and ab
        {
            // Marco *** IR Fix is here 0.12 -> make it smaller to increase time for IR to drop (set to 0.04f)
            // 0.05 -> Make it larger to increase 'idle' IROutput (set to 0.20f)
            // Marco edit - * by 1.2 to help out IR missiles reach RPG status

            irOutput = (float)(irOutput + (((((PowerOutput() - 0.70) * 2) / irOutput) - 1.0f) * SimLibMajorFrameTime * 0.04f));
            irOutput = (max(min(0.6f, irOutput), 0.05f));    //,0.167f);

            // MonoPrint("IR output %d .\n",irOutput);
            return max((irOutput * irSignature) * 1.875f, 0.15f);
        }
        else
        {
            // irOutput = (PowerOutput()- 0.985f )*100.0f/2;// from 1.25 to 2.25 (me123'a Settings)
            irOutput = ((PowerOutput() - 1.0f) * 10.0f) + 1.4f;
            return min(5.0f, irOutput * irSignature);
            //return (max (2.0f, min(5.0f,irOutput * irSignature) ));
        }
    }

#else
    VehicleClassDataType *vc = (VehicleClassDataType *)Falcon4ClassTable[Type() - VU_LAST_ENTITY_TYPE].dataPtr;
    ShiAssert(vc);

    // TODO:  THis needs to return IR data
    // 2000-09-15 MODIFIED BY S.G. THIS GIVES IT THE IR DATA IT NEEDS
    // return vc->RCSfactor * PowerOutput();

    // The IR signature is stored as a byte at offset 0x9E of the falcon4.vcd structure.
    // This byte, as well as 0x9D and 0x9F are used for padding originally.
    // The value range will be 0 to 2 with increments of 0.0078125
    unsigned char *pIrSign = (unsigned char *)vc;
    int iIrSign = (unsigned)pIrSign[0x9E];
    float irSignature = 1.0f;

    if (iIrSign)
        irSignature = (float)iIrSign / 128.0f;

    return irSignature * EngineTempOutput();
#endif

}


int SimVehicleClass::GetRadarType(void)
{
    VehicleClassDataType *vc = (VehicleClassDataType *)((Falcon4EntityClassType*)EntityType())->dataPtr;
    ShiAssert(vc);

    return vc->RadarType;
}


int SimVehicleClass::Exec(void)
{
    Tpoint pos;

    SimMoverClass::Exec();
    // VP_changes for flak Nov 5, 2002
    // Here the function for explosion's gust can be added
    // update special effects timer
    // can be used in derived classes to control
    // various special effects they want to run
    // should be reset to 0 when effect has run
    sfxTimer += SimLibMajorFrameTime;

    //RV - I-Hawk - Added a 0 vector for RV new PS calls
    Tpoint PSvec;
    PSvec.x = 0;
    PSvec.y = 0;
    PSvec.z = 0;

    // ioPerturb will result in  temporary perturbances in controls
    // when damage occurs (i.e missile hit, etc...).  Reduce this
    // every frame if > 0.0
    if (ioPerturb > 0.0f)
    {
        // JB 010730
        if (ioPerturb > 0.5f and g_bDisableFunkyChicken and (rand() bitand 63) == 63)
        {
            float randamt  =   PRANDFloatPos();

            // run stuff here....
            pos.x = XPos();
            pos.y = YPos();
            pos.z = ZPos();
            /*
            OTWDriver.AddSfxRequest(
             new SfxClass( SFX_FIRE1, // type
             &pos, // world pos
             0.5f, // time to live
             20.0f + 50.0f * randamt ) ); // scale
             */
            DrawableParticleSys::PS_AddParticleEx((SFX_FIRE1 + 1),
                                                  &pos,
                                                  &PSvec);
        }

        ioPerturb -= 0.50f * SimLibMajorFrameTime;

        if (ioPerturb < 0.0f)
            ioPerturb = 0.0f;
    }

    // what's the strength?  if we're less than 0 we're dying.
    // bleed off strength when below 0.  This will allow time for
    // other effects (fire trails, ejection, etc...).  When we've reached
    // an absolute min, explode
    if (pctStrength <= 0.0f and not IsExploding())
    {
        // debug non local
        /*
        if ( not IsLocal() )
        {
         MonoPrint( "NonLocal Dying: Pct Strength now: %f\n", pctStrength );
        }
        else
        {
         MonoPrint( "Local Vehicle Dying: Pct Strength now: %f\n", pctStrength );
        }
        */

        // RV - Biker - This is for aircraft

        // RV - I-Hawk - if we are at DOMAIN_AIR, dying SFX is handled at AircraftClass or
        // HelicopterClass Exec functions
        if (gSfxLOD >= 0.5f and GetDomain() == DOMAIN_AIR)
        {
        }

        ///*
        //  update dying timer
        // dyingTimer += SimLibMajorFrameTime;
        //
        //  Do nothing for the 1st part of dying
        // if ( pctStrength > -0.07f )
        // {
        // }
        // RV - I-Hawk - Commenting some unnecessary stuff here...
        //
        //  just chuck out debris until a certain point in death sequence
        // else if ( pctStrength > -0.3f ) // I-Hawk - was -0.5f before
        // {
        // if ( dyingTimer > 0.3f ) //
        // {
        //    I-Hawk - better with the standard position...
        //
        // /*
        // pos.x = XPos() - XDelta() * SimLibMajorFrameTime;
        // pos.y = YPos() - YDelta() * SimLibMajorFrameTime;
        // pos.z = ZPos() - ZDelta() * SimLibMajorFrameTime;
        // */
        ///*
        // pos.x = XPos();
        // pos.y = YPos();
        // pos.z = ZPos();
        //
        // vec.x = XDelta();
        // vec.y = YDelta();
        // vec.z = ZDelta();
        //
        // DrawableParticleSys::PS_AddParticleEx((PSFX_AC_EARLY_BURNING + 1),
        // &pos,
        // &vec);
        //
        // Cobra 11/12/04 TJL Removed per Steve and stops Scrape sound during dying
        // /*
        // if( not AddParticleEffect(PSFX_AC_EARLY_BURNING,&pos, &vec))
        // {
        // /*
        // OTWDriver.AddSfxRequest(
        // new SfxClass (SFX_SPARKS, // type was SFX_SPARKS
        // &pos, // world pos
        // 1.0f, // time to live
        // 12.0f )); // scale
        // */
        // /*
        // DrawableParticleSys::PS_AddParticleEx((SFX_SPARKS + 1),
        // &pos,
        // &PSvec);
        // }
        // */
        //
        ///*
        // for ( int i = 0; i < 3; i++ )
        // {
        // vec.x = XDelta() * 0.5f + PRANDFloat() * 20.0f;
        // vec.y = YDelta() * 0.5f + PRANDFloat() * 20.0f;
        // vec.z = ZDelta() * 0.5f + PRANDFloat() * 20.0f;
        // /*
        // OTWDriver.AddSfxRequest(
        // new SfxClass (SFX_AC_DEBRIS, // type
        // SFX_MOVES bitor SFX_USES_GRAVITY, // flags
        // &pos, // world pos
        // &vec, // vector
        // 3.5f, // time to live
        // 2.0f )); // scale
        // */
        ///*
        // DrawableParticleSys::PS_AddParticleEx((SFX_AC_DEBRIS + 1),
        // &pos,
        // &vec);
        // }
        //
        //  zero out
        // dyingTimer = 0.0f;
        // }
        //    }
        //     if dyingTimer is greater than some value, we can run some
        //     sort of special effect such as tossing debris, ejection, etc...
        //
        // RV - I-Hawk - Commenting lots of stuff here... needed no more.
        //
        //    else switch( dyingType )
        //    {
        //    case 5:
        //    case SimVehicleClass::DIE_SMOKE:
        //    if ( dyingTimer > 0.10f + ( 1.0f - gSfxLOD ) * 0.3f )
        //    {
        //     run stuff here....
        //    pos.x = XPos();
        //    pos.y = YPos();
        //    pos.z = ZPos();
        //
        //     vec.x = XDelta();
        // vec.y = YDelta();
        // vec.z = ZDelta();
        //
        // DrawableParticleSys::PS_AddParticleEx((PSFX_VEHICLE_DIE_SMOKE + 1),
        // &pos,
        // &vec);
        //
        // /* I-Hawk
        //    if( not AddParticleEffect(PSFX_VEHICLE_DIE_SMOKE,&pos,&vec))
        //    {
        //        vec.x = PRANDFloat() * 40.0f;
        //    vec.y = PRANDFloat() * 40.0f;
        //    vec.z = PRANDFloat() * 40.0f;
        //
        //    switch ( PRANDInt5() )
        //    {
        //    case 0:
        //        /*
        //    OTWDriver.AddSfxRequest(
        //    new SfxClass (SFX_SPARKS, // type
        //    &pos, // world pos
        //    4.5f, // time to live
        //    7.5f )); // scale
        //    */
        // /*
        //    DrawableParticleSys::PS_AddParticleEx((SFX_SPARKS + 1),
        // &pos,
        // &PSvec);
        //    break;
        //    case 1:
        //    /*
        //    OTWDriver.AddSfxRequest(
        //    new SfxClass (SFX_TRAILSMOKE, // type
        //    SFX_MOVES, // flags
        //    &pos, // world pos
        //    &vec, // vector
        //    4.5f, // time to live
        //    20.5f )); // scale
        //    */
        // /*
        //    DrawableParticleSys::PS_AddParticleEx((SFX_TRAILSMOKE + 1),
        // &pos,
        // &vec);
        //    break;
        //    default:
        //    /*
        //    OTWDriver.AddSfxRequest(
        //    new SfxClass (SFX_AIR_SMOKECLOUD, // type
        //    SFX_MOVES, // flags
        //    &pos, // world pos
        //    &vec, // vector
        //    4.5f, // time to live
        //    10.5f )); // scale
        //    */
        // /*
        //    DrawableParticleSys::PS_AddParticleEx((SFX_AIR_SMOKECLOUD + 1),
        // &pos,
        // &vec);
        //    break;
        //    }
        //    }
        //    */
        ///*
        //     reset the timer
        //    dyingTimer = 0.0f;
        //    }
        //    break;
        //    case 6:
        //    case SimVehicleClass::DIE_SHORT_FIREBALL:
        //    if ( dyingTimer > 0.10f + ( 1.0f - gSfxLOD ) * 0.3f )
        //    {
        //    pos.x = XPos();
        //    pos.y = YPos();
        //    pos.z = ZPos();
        //
        //    vec.x = XDelta();
        //    vec.y = YDelta();
        //    vec.z = ZDelta();
        //
        //    DrawableParticleSys::PS_AddParticleEx((PSFX_VEHICLE_DIE_FIREBALL + 1),
        // &pos,
        // &vec);
        //    /* I-Hawk
        //    if( not AddParticleEffect(PSFX_VEHICLE_DIE_FIREBALL,&pos,&vec))
        //    {
        //    if ( pctStrength > -0.7f )
        //    {
        //    float randamt  =   PRANDFloatPos();
        //
        //     run stuff here....
        //    pos.x = XPos();
        //    pos.y = YPos();
        //    pos.z = ZPos();
        //
        //    vec.x = XDelta();
        //    vec.y = YDelta();
        //    vec.z = ZDelta();
        //
        //    /*
        //    OTWDriver.AddSfxRequest(
        //    new SfxClass( SFX_FIRE1, // type
        //    &pos, // world pos
        //    0.5f, // time to live
        //    20.0f + 50.0f * randamt ) ); // scale
        //    */
        //    /*
        //    DrawableParticleSys::PS_AddParticleEx((SFX_FIRE1 + 1),
        // &pos,
        // &PSvec);
        //    }
        //    else
        //    {
        //    vec.x = PRANDFloat() * 40.0f;
        //    vec.y = PRANDFloat() * 40.0f;
        //    vec.z = PRANDFloat() * 40.0f;
        //     run stuff here....
        //    pos.x = XPos() - XDelta() * SimLibMajorFrameTime;
        //    pos.y = YPos() - YDelta() * SimLibMajorFrameTime;
        //    pos.z = ZPos() - ZDelta() * SimLibMajorFrameTime;
        //    switch ( PRANDInt5() )
        //    {
        //    case 0:
        //    /*
        //    OTWDriver.AddSfxRequest(
        //    new SfxClass (SFX_SPARKS, // type
        //    &pos, // world pos
        //    4.5f, // time to live
        //    12.5f )); // scale
        //    */
        //    /*
        //    DrawableParticleSys::PS_AddParticleEx((SFX_SPARKS + 1),
        // &pos,
        // &PSvec);
        //    break;
        //    case 1:
        //    /*
        //    OTWDriver.AddSfxRequest(
        //    new SfxClass (SFX_SPARKS, // type
        //    &pos, // world pos
        //    1.5f, // time to live
        //    12.5f )); // scale
        //    */
        //    /*
        //    DrawableParticleSys::PS_AddParticleEx((SFX_SPARKS + 1),
        // &pos,
        // &PSvec);
        //    break;
        //    default:
        //    /*
        //    OTWDriver.AddSfxRequest(
        //    new SfxClass (SFX_TRAILSMOKE, // type
        //    SFX_MOVES, // flags
        //    &pos, // world pos
        //    &vec, // vector
        //    4.5f, // time to live
        //    20.5f )); // scale
        //    */
        //    /*
        //    DrawableParticleSys::PS_AddParticleEx((SFX_TRAILSMOKE + 1),
        // &pos,
        // &vec);
        //    break;
        //    }
        //    }
        //    }
        //    */
        //
        ///*    // reset the timer
        //    dyingTimer = 0.0f;
        //    }
        //    break;
        //    case SimVehicleClass::DIE_INTERMITTENT_SMOKE:
        //    if ( dyingTimer > 0.10f + ( 1.0f - gSfxLOD ) * 0.3f )
        //    {
        //
        //    RV - I-Hawk - No need for such random position and vector...
        //    /*
        // vec.x = PRANDFloat() * 40.0f;
        // vec.y = PRANDFloat() * 40.0f;
        // vec.z = PRANDFloat() * 40.0f;
        //  run stuff here....
        // pos.x = XPos() - XDelta() * SimLibMajorFrameTime;
        // pos.y = YPos() - YDelta() * SimLibMajorFrameTime;
        // pos.z = ZPos() - ZDelta() * SimLibMajorFrameTime;
        // */
        ///*
        //    pos.x = XPos();
        //    pos.y = YPos();
        //    pos.z = ZPos();
        //
        //    vec.x = XDelta();
        //    vec.y = YDelta();
        //    vec.z = ZDelta();
        //
        //    DrawableParticleSys::PS_AddParticleEx((PSFX_VEHICLE_DIE_SMOKE2 + 1),
        // &pos,
        // &vec);
        //
        //    /* I-Hawk
        // if( not AddParticleEffect(PSFX_VEHICLE_DIE_SMOKE2,&pos,&vec))
        // {
        // switch ( PRANDInt5() )
        // {
        // case 0:
        // case 1:
        // /*
        // OTWDriver.AddSfxRequest(
        // new SfxClass (SFX_SPARKS, // type
        // &pos, // world pos
        // 4.5f, // time to live
        // 12.5f )); // scale
        // */
        //    /*
        // DrawableParticleSys::PS_AddParticleEx((SFX_SPARKS + 1),
        // &pos,
        // &PSvec);
        // break;
        // case 2:
        // /*
        // OTWDriver.AddSfxRequest(
        // new SfxClass (SFX_SPARKS, // type
        // &pos, // world pos
        // 1.5f, // time to live
        // 12.5f )); // scale
        // */
        //    /*
        // DrawableParticleSys::PS_AddParticleEx((SFX_SPARKS + 1),
        // &pos,
        // &PSvec);
        // break;
        // default:
        // /*
        // OTWDriver.AddSfxRequest(
        // new SfxClass (SFX_TRAILSMOKE, // type
        // SFX_MOVES, // flags
        // &pos, // world pos
        // &vec, // vector
        // 4.5f, // time to live
        // 20.5f )); // scale
        // */
        //    /*
        // DrawableParticleSys::PS_AddParticleEx((SFX_TRAILSMOKE + 1),
        // &pos,
        // &vec);
        // break;
        // }
        // }
        // */
        ///*
        //  reset the timer
        // dyingTimer = 0.0f;
        //
        //    }
        //    break;
        //    case SimVehicleClass::DIE_FIREBALL:
        //    if ( dyingTimer > 0.10f + ( 1.0f - gSfxLOD ) * 0.3f )
        //    {
        //    float randamt  =   PRANDFloatPos();
        //
        //     run stuff here....
        //    pos.x = XPos();
        //    pos.y = YPos();
        //    pos.z = ZPos();
        //
        //    vec.x = XDelta();
        //    vec.y = YDelta();
        //    vec.z = ZDelta();
        //
        //    DrawableParticleSys::PS_AddParticleEx((PSFX_VEHICLE_DIE_FIREBALL2 + 1),
        // &pos,
        // &vec);
        //
        //    /* I-Hawk
        //    if( not AddParticleEffect(PSFX_VEHICLE_DIE_FIREBALL2,&pos,&vec))
        //    {
        //    /*
        //    OTWDriver.AddSfxRequest(
        // new SfxClass( SFX_FIRE1, // type
        // &pos, // world pos
        // 0.5f, // time to live
        // 40.0f + 30.0f * randamt ) ); // scale
        // */
        //    /*
        //    DrawableParticleSys::PS_AddParticleEx((SFX_FIRE1 + 1),
        // &pos,
        // &PSvec);
        //    }
        //    */
        ///*
        //     reset the timer
        //    dyingTimer = 0.0f;
        //    }
        //    break;
        //    case SimVehicleClass::DIE_INTERMITTENT_FIRE:
        //    default:
        //    if ( dyingTimer > 0.10f + ( 1.0f - gSfxLOD ) * 0.3f )
        //    {
        //    pos.x = XPos();
        //    pos.y = YPos();
        //    pos.z = ZPos();
        //
        //    vec.x = XDelta();
        //    vec.y = YDelta();
        //    vec.z = ZDelta();
        //
        //    DrawableParticleSys::PS_AddParticleEx((PSFX_VEHICLE_DIE_INTERMITTENT_FIRE + 1),
        // &pos,
        // &vec);
        //
        //    /* I-Hawk
        //    if( not AddParticleEffect(PSFX_VEHICLE_DIE_INTERMITTENT_FIRE,&pos,&vec))
        //    {
        //
        //    if ( pctStrength > -0.6f or
        //    pctStrength > -0.7f and pctStrength < -0.65 or
        //    pctStrength > -0.8f and pctStrength < -0.75 or
        //    pctStrength > -0.9f and pctStrength < -0.85 )
        //    {
        //    float randamt  =   PRANDFloatPos();
        //
        //     run stuff here....
        //    pos.x = XPos();
        //    pos.y = YPos();
        //    pos.z = ZPos();
        //
        //
        //    /*
        //    OTWDriver.AddSfxRequest(
        //    new SfxClass( SFX_FIRE1, // type
        //    &pos, // world pos
        //    0.5f, // time to live
        //    20.0f + 50.0f * randamt ) ); // scale
        //    */
        //    /*
        //    DrawableParticleSys::PS_AddParticleEx((SFX_FIRE1 + 1),
        // &pos,
        // &PSvec);
        //
        //    }
        //    else
        //    {
        //    vec.x = PRANDFloat() * 40.0f;
        //    vec.y = PRANDFloat() * 40.0f;
        //    vec.z = PRANDFloat() * 40.0f;
        //     run stuff here....
        //    pos.x = XPos() - XDelta() * SimLibMajorFrameTime;
        //    pos.y = YPos() - YDelta() * SimLibMajorFrameTime;
        //    pos.z = ZPos() - ZDelta() * SimLibMajorFrameTime;
        //    switch ( PRANDInt5() )
        //    {
        //    case 0:
        //    case 1:
        //    /*
        //    OTWDriver.AddSfxRequest(
        //    new SfxClass (SFX_SPARKS, // type
        //    &pos, // world pos
        //    4.5f, // time to live
        //    12.5f )); // scale
        //    */
        //    /*
        //    DrawableParticleSys::PS_AddParticleEx((SFX_SPARKS + 1),
        // &pos,
        // &PSvec);
        //    break;
        //    case 2:
        //    /*
        //                                   OTWDriver.AddSfxRequest(
        //    new SfxClass (SFX_SPARKS, // type
        //    &pos, // world pos
        //    1.5f, // time to live
        //    12.5f )); // scale
        //    */
        //    /*
        //    DrawableParticleSys::PS_AddParticleEx((SFX_SPARKS + 1),
        // &pos,
        // &PSvec);
        //    break;
        //    default:
        //    /*
        //    OTWDriver.AddSfxRequest(
        //    new SfxClass (SFX_TRAILSMOKE, // type
        //    SFX_MOVES, // flags
        //    &pos, // world pos
        //    &vec, // vector
        //    4.5f, // time to live
        //    20.5f )); // scale
        //    */
        //    /*
        //    DrawableParticleSys::PS_AddParticleEx((SFX_TRAILSMOKE + 1),
        // &pos,
        // &vec);
        //    break;
        //    }
        //    }
        //    }
        //    */
        ///*
        //     reset the timer
        //    dyingTimer = 0.0f;
        //    }
        //    break;
        // } // end switch
        // } // end if LOD
        //
        //  RV - I-Hawk - until here...
        //*/

        // RV - Biker - Effects for ships (maybe some special for tanker type)
        if (GetDomain() == DOMAIN_SEA)
        {
            dyingTimer += SimLibMajorFrameTime;

            if (dyingTimer > 2.5f and rand() bitand 0x03)
            {
                dyingTimer = 0.0f;
                pos.x = XPos();
                pos.y = YPos();
                pos.z = 0.0f;
                /*
                OTWDriver.AddSfxRequest(
                 new SfxClass (SFX_WATERTRAIL, // type
                 &pos, // world pos
                 3.0f, // time to live
                 0.1f ));
                 */
                DrawableParticleSys::PS_AddParticleEx((SFX_WATERTRAIL + 1),
                                                      &pos,
                                                      &PSvec);
            }
        }

        // RV - Biker - bleed off strength at some rate....
        switch (GetDomain())
        {
            case DOMAIN_AIR:

                //RV - I-Hawk - Use random death time...
                switch (dyingTime)
                {
                    case 0:
                    case 1:
                        strength -= maxStrength * 0.05f * SimLibMajorFrameTime;
                        break;

                    case 2:
                        strength -= maxStrength * 0.05f * SimLibMajorFrameTime * 5000.0f;
                        break;

                    case 3:
                        strength -= maxStrength * 0.05f * SimLibMajorFrameTime * 2.0f;
                        break;

                    case 4:
                        strength -= maxStrength * 0.05f * SimLibMajorFrameTime * 1.5f;
                        break;

                    default:
                        strength -= maxStrength * 0.05f * SimLibMajorFrameTime;
                        break;
                }

                break;

            case DOMAIN_LAND:
                // Ground vehicles go almost directly dead
                strength -= maxStrength * 0.05f * SimLibMajorFrameTime * 5000.0f;
                break;

                // Maybe switch for type also
            case DOMAIN_SEA:
                // Ship bleed off slower
                strength -= maxStrength * 0.05f * SimLibMajorFrameTime * 0.05f;
                break;

            default:
                strength -= maxStrength * 0.05f * SimLibMajorFrameTime * 5000.0f;
                break;
        }

        pctStrength = strength / maxStrength;

        // when we've bottomed out, kablooey
        // Have a look how AC behave when on ground
        if (pctStrength <= -1.0f /*or OnGround()*/)
        {
            if (GetDomain() == DOMAIN_LAND and GetType() not_eq TYPE_FOOT)
            {
                dyingTimer = 0.0f;
                pos.x = XPos();
                pos.y = YPos();
                pos.z = 0.0f;
                /*
                OTWDriver.AddSfxRequest(
                 new SfxClass (SFX_VEHICLE_BURNING, // type
                 &pos, // world pos
                 rand()%5+15.0f, // time to live
                 0.1f ));
                 */
                DrawableParticleSys::PS_AddParticleEx((SFX_VEHICLE_BURNING + 1),
                                                      &pos,
                                                      &PSvec);
            }

            if (GetDomain() == DOMAIN_SEA)
            {
                dyingTimer = 0.0f;
                pos.x = XPos();
                pos.y = YPos();
                pos.z = 0.0f;
                /*
                OTWDriver.AddSfxRequest(
                 new SfxClass (SFX_VEHICLE_BURNING,// type
                 &pos, // world pos
                 rand()%5+15.0f, // time to live
                 0.1f ));
                 */
                //RV - I-Hawk - Change from VEHICLE_BURNING to FIRE2 for ships special late burning
                DrawableParticleSys::PS_AddParticleEx((SFX_SHIP_BURNING_FIRE + 1),
                                                      &pos,
                                                      &PSvec);
            }

            // damage anything around us when we explode
            ApplyProximityDamage();

            SetExploding(TRUE);

            //           MonoPrint ("Vehicle %d DEAD Local: %s, Has Death Msg: %s\n",
            //     Id().num_,
            // IsLocal() ? "TRUE" : "FALSE",
            // deathMessage ? "TRUE" : "FALSE" );

            // local entity sends death message out
            if (deathMessage)
                FalconSendMessage(deathMessage, TRUE);
        }
    } // end if pctStr < 0

    return TRUE;
}

// function interface
// serialization functions
int SimVehicleClass::SaveSize()
{
    return SimMoverClass::SaveSize();
}

int SimVehicleClass::Save(VU_BYTE **stream)
{
    int retval;

    retval = SimMoverClass::Save(stream);

    return retval;
}

int SimVehicleClass::Save(FILE *file)
{
    int retval;

    retval = SimMoverClass::Save(file);

    return (retval);
}

int SimVehicleClass::Handle(VuFullUpdateEvent *event)
{
    return (SimMoverClass::Handle(event));
}


int SimVehicleClass::Handle(VuPositionUpdateEvent *event)
{
    return (SimMoverClass::Handle(event));
}

int SimVehicleClass::Handle(VuTransferEvent *event)
{
    return (SimMoverClass::Handle(event));
}

VU_ERRCODE SimVehicleClass::InsertionCallback(void)
{
    // KCK: If you're relying on the code below, something is
    // VERY, VERY wrong. Please talk to me before re-adding this code again
    // if (IsLocal())
    // {
    // Wake();
    // }

    if (theBrain)
    {
        theBrain->PostInsert();
    }

    return SimMoverClass::InsertionCallback();
}

void SimVehicleClass::SetDead(int flag)
{
    SimMoverClass::SetDead(flag);
}

//void SimVehicleClass::InitWeapons (ushort *type, ushort *num)
void SimVehicleClass::InitWeapons(ushort*, ushort*)
{
    // KCK: Moved to SMSClass constructor
}

void SimVehicleClass::SOIManager(SOI newSOI)
{
    SMSClass *Sms = static_cast<SMSClass*>(GetSMS());
    SimWeaponClass *weapon = NULL;
    MissileDisplayClass *mslDisplay = NULL;
    SensorClass *tPodDisplay = NULL;
    RadarClass *theRadar = (RadarClass*) FindSensor(this, SensorClass::Radar);
    FireControlComputer* FCC = ((SimVehicleClass*)this)->GetFCC();

    if (Sms)
    {
        weapon = Sms->GetCurrentWeapon();
    }

    if (weapon and weapon->IsMissile())
    {
        MissileClass* theMissile = static_cast<MissileClass*>(weapon);
        int dType = theMissile->GetDisplayType();

        if (
            dType == MissileClass::DisplayBW or
            dType == MissileClass::DisplayIR or
            dType == MissileClass::DisplayHTS
        )
        {
            mslDisplay = static_cast<MissileDisplayClass*>(theMissile->display);
        }
    }

    tPodDisplay = FindLaserPod(this);

    curSOI = newSOI;

    switch (curSOI)
    {
        case SOI_HUD:
            if (theRadar)
                theRadar->SetSOI(FALSE);

            if (TheHud and this == SimDriver.GetPlayerEntity())//Cobra test
                TheHud->SetSOI(TRUE);

            if (mslDisplay)
                mslDisplay->SetSOI(FALSE);

            if (tPodDisplay)
                tPodDisplay->SetSOI(FALSE);

            if (FCC) //MI
                FCC->ClearSOIAll();

            break;

        case SOI_RADAR:
            if (TheHud and this == SimDriver.GetPlayerEntity())//Cobra test
                TheHud->SetSOI(FALSE);

            if (theRadar)
                theRadar->SetSOI(TRUE);

            if (mslDisplay)
                mslDisplay->SetSOI(FALSE);

            if (tPodDisplay)
                tPodDisplay->SetSOI(FALSE);

            if (FCC) //MI
                FCC->ClearSOIAll();

            break;

        case SOI_WEAPON:
            if (theRadar)
                theRadar->SetSOI(FALSE);

            if (TheHud and this == SimDriver.GetPlayerEntity())//Cobra test // JB/JPO 010614 CTD
                TheHud->SetSOI(FALSE);

            if (mslDisplay)
                mslDisplay->SetSOI(TRUE);

            if (tPodDisplay)
                tPodDisplay->SetSOI(TRUE);

            if (FCC) //MI
                FCC->ClearSOIAll();

            break;

            //MI
        case SOI_FCC:
        {
            if (theRadar)
                theRadar->SetSOI(FALSE);

            if (mslDisplay)
                mslDisplay->SetSOI(FALSE);

            if (tPodDisplay)
                tPodDisplay->SetSOI(FALSE);

            if (TheHud and this == SimDriver.GetPlayerEntity())//Test // JB/JPO 010614 CTD
                TheHud->SetSOI(FALSE);

            if (FCC)
            {
                //reset our cursor position
                FCC->xPos = 0.0F;
                FCC->yPos = 0.0F;
                FCC->IsSOI = TRUE;
            }
        }
        break;
    }
}

void SimVehicleClass::ApplyDamage(FalconDamageMessage* damageMessage)
{
    int soundIdx = -1;
    VehicleClassDataType *vc;
    WeaponClassDataType *wc;
    FalconEntity *lastToHit;
    float hitPoints = 0.0f;
    int groundType;
    int i;
    Tpoint pos, mvec;

    //RV - I-Hawk - Added a 0 vector for RV new PS calls
    Tpoint PSvec;
    PSvec.x = 0;
    PSvec.y = 0;
    PSvec.z = 0;

    // RV - Biker - No more kamikaze on carriers allowed
    if (GetDomain() == DOMAIN_SEA and GetType() == TYPE_CAPITAL_SHIP and damageMessage->dataBlock.damageType == FalconDamageType::ObjectCollisionDamage)
    {
        return;
    }

    // call base class damage function
    SimBaseClass::ApplyDamage(damageMessage);

    switch (damageMessage->dataBlock.damageType)
    {
            // Collisions
        case FalconDamageType::GroundCollisionDamage:
        case FalconDamageType::FeatureCollisionDamage:
        case FalconDamageType::ObjectCollisionDamage:
        case FalconDamageType::CollisionDamage:
        case FalconDamageType::DebrisDamage:
            hitPoints = damageMessage->dataBlock.damageStrength;
            break;

            // Auto-death
        case FalconDamageType::FODDamage:
            hitPoints = maxStrength;
            break;

            // Proximity Damage
        case FalconDamageType::ProximityDamage:
            // Same as weapon damage, but we pre-calculate the damage strength
            // get vehicle data
            vc = (VehicleClassDataType *)Falcon4ClassTable[ Type() - VU_LAST_ENTITY_TYPE].dataPtr;
            // get strength of weapon and calc damage based on vehicle mod
            hitPoints = (float)damageMessage->dataBlock.damageStrength * ((float)vc->DamageMod[HighExplosiveDam]) / 100.0f;
            break;

            // Weapon Hits
        default:
            // get vehicle data
            vc = (VehicleClassDataType *)Falcon4ClassTable[ Type() - VU_LAST_ENTITY_TYPE].dataPtr;
            // get weapon data
            wc = (WeaponClassDataType *)Falcon4ClassTable[ damageMessage->dataBlock.fWeaponID - VU_LAST_ENTITY_TYPE].dataPtr;
            // get strength of weapon and calc damage based on vehicle mod
            // also, check for ownship and invincibiliy flag
            hitPoints = (float)wc->Strength * ((float)vc->DamageMod[ wc->DamageType ]) / 100.0f;
            break;
    }

    //VP_changes this is important for Damage Model
    switch (damageMessage->dataBlock.damageType)
    {
        case FalconDamageType::BulletDamage:
            soundIdx = SFX_RICOCHET1 + PRANDInt5();
            ioPerturb = 0.5f;
            hitPoints -= hitPoints * 0.7f * damageMessage->dataBlock.damageRandomFact;
            break;

        case FalconDamageType::MissileDamage:
            soundIdx = SFX_BOOMA1 + PRANDInt5();
            ioPerturb = 2.0f;
            hitPoints += hitPoints * 0.25f * damageMessage->dataBlock.damageRandomFact;
            break;

        case FalconDamageType::BombDamage:
            soundIdx = SFX_BOOMG1 + PRANDInt5();
            ioPerturb = 2.0f;
            hitPoints += hitPoints * 0.25f * damageMessage->dataBlock.damageRandomFact;
            break;

        case FalconDamageType::ProximityDamage:
            // KCK Question: Should we have secondary explosion sound effects here?
            ioPerturb = 2.0f;
            hitPoints += hitPoints * 0.25f * damageMessage->dataBlock.damageRandomFact;
            break;

        case FalconDamageType::GroundCollisionDamage:
            // if we're exploding at this point we've smacked into a
            // large brown,green,blue globe....
            hitPoints += hitPoints * 0.25f * damageMessage->dataBlock.damageRandomFact;
            ioPerturb = 2.0f;

            if (OTWDriver.GetViewpoint())
                groundType = (OTWDriver.GetViewpoint())->GetGroundType(XPos(), YPos());
            else
                groundType = COVERAGE_PLAINS;

            // are we blowing up?
            if (hitPoints > strength)
            {
                // death sounds
                if ( not (groundType == COVERAGE_WATER or groundType == COVERAGE_RIVER))
                    soundIdx = SFX_DIRTDART;
                else
                    soundIdx = SFX_H2ODART;
            }
            else
            {
                // damage sounds and effects

                pos.x = XPos();
                pos.y = YPos();
                pos.z = ZPos();
                mvec.x = XDelta();
                mvec.y = YDelta();
                mvec.z = ZDelta() - 40.0f;

                if ( not (groundType == COVERAGE_WATER or groundType == COVERAGE_RIVER))
                {
                    soundIdx = SFX_BOOMA1;
                    /*
                    OTWDriver.AddSfxRequest(
                     new SfxClass( SFX_GROUND_DUSTCLOUD, // type
                     SFX_MOVES,
                     &pos, // world pos
                     &mvec, // vel vector
                     3.0, // time to live
                     8.0f ) ); // scale
                     */
                    DrawableParticleSys::PS_AddParticleEx((SFX_GROUND_DUSTCLOUD + 1),
                                                          &pos,
                                                          &mvec);

                }
                else
                {
                    soundIdx = SFX_SPLASH;
                    /*
                    OTWDriver.AddSfxRequest(
                     new SfxClass( SFX_WATER_CLOUD, // type
                     SFX_MOVES,
                     &pos, // world pos
                     &mvec, // vel vector
                     3.0, // time to live
                     10.0f ) ); // scale
                     */
                    DrawableParticleSys::PS_AddParticleEx((SFX_WATER_CLOUD + 1),
                                                          &pos,
                                                          &mvec);
                }
            }

            // trail sparks when hit ground
            for (i = 0; i < 5; i++)
            {
                pos.x = XPos() - XDelta() * SimLibMajorFrameTime * PRANDFloatPos() * 2.0f;
                pos.y = YPos() - YDelta() * SimLibMajorFrameTime * PRANDFloatPos() * 2.0f;
                pos.z = ZPos();
                mvec.x = XDelta();
                mvec.y = YDelta();
                mvec.z = 0;

                if ( not AddParticleEffect(PSFX_AC_EARLY_BURNING, &pos, &mvec))
                {
                    /*
                    OTWDriver.AddSfxRequest(
                     new SfxClass( SFX_SPARKS, // type
                     &pos, // world pos
                     2.9f, // time to live
                     14.3f ) ); // scale
                     */
                    DrawableParticleSys::PS_AddParticleEx((SFX_SPARKS + 1),
                                                          &pos,
                                                          &PSvec);
                }
            }

            break;

        case FalconDamageType::CollisionDamage:
        case FalconDamageType::FeatureCollisionDamage:
        case FalconDamageType::ObjectCollisionDamage:
            hitPoints += hitPoints * 0.25f * damageMessage->dataBlock.damageRandomFact;
            soundIdx = SFX_BOOMG1 + PRANDInt5();
            ioPerturb = 2.0f;
            break;

        case FalconDamageType::DebrisDamage:
            // we probably want thump sounds here.....
            soundIdx = SFX_BOOMA1 + PRANDInt5();
            hitPoints += hitPoints * 0.25f * damageMessage->dataBlock.damageRandomFact;
            ioPerturb = 2.0f;
            break;

        default:
            break;
    }

    // what percent strength is left?
    // NOTE: A missile strike effect to an aircraft looks like a missile
    // burst with the plane flying thru it OK for a little while and trailing
    // some debris.  For missile damage, don't let pctStrength hit neg val
    // until it's been set to 0.  This will allow time to generate debris
    if (strength == maxStrength and 
        hitPoints > strength and 
        IsAirplane() and 
        damageMessage->dataBlock.damageType == FalconDamageType::MissileDamage)
    {
        hitPoints = strength;
    }

    // check for ownship and invincibiliy flag
    if (IsSetFalcFlag(FEC_INVULNERABLE))
    {
        hitPoints = 0;
    }

    strength -= hitPoints;
    pctStrength = strength / maxStrength;

    // JB 000816
    // If damaged has been sustained then:
    // If this is our own aircraft and if we have disabled the damage jitter or a 25% chance
    // Or otherwise a 13% chance (AI controlled aircraft won't be able to fly with a bias so we want the % to be low)
    if (hitPoints > 0 and 
        ((IsSetFlag(MOTION_OWNSHIP) and (g_bDisableFunkyChicken or (rand() bitand 0x3) == 0x3)) or
         (rand() bitand 0x7) == 0x7))
    {
        rBias += ioPerturb / 2 * PRANDFloat();
        pBias += ioPerturb / 2 * PRANDFloat();
        yBias += ioPerturb / 2 * PRANDFloat();
    }

    // JB 010121 adjusted to work in MP
    if (g_bNewDamageEffects and IsSetFlag(MOTION_OWNSHIP) and // hitPoints > 0 and 2002-04-11 REMOVED BY S.G. Done below after ->af since it's now externalized
        // SimDriver.GetPlayerEntity() and SimDriver.GetPlayerEntity()->AutopilotType() not_eq AircraftClass::CombatAP and 
        // not (gCommsMgr and gCommsMgr->Online()) and 
        IsAirplane() and 
        // (rand() bitand 0x7) == 0x7 and // 13% chance 2002-04-11 MOVED BY S.G. After the ->af and used the external var now
        ((AircraftClass*)this)->af and 
        ((AircraftClass*)this)->af->GetEngineDamageHitThreshold() < hitPoints and // 2002-04-11 ADDED BY S.G. hitPoints 'theshold' is no longer 1 or above but externalized
        ((AircraftClass*)this)->af->GetEngineDamageStopThreshold() > rand() % 100 and // 2002-04-11 ADDED BY S.G. instead of a fixed 13%, now uses an aiframe aux var
        ((AircraftClass*)this)->AutopilotType() not_eq AircraftClass::CombatAP
       )
    {
        ((AircraftClass*)this)->af->SetFlag(AirframeClass::EngineStopped);
        // ((AircraftClass*)this)->af->SetFlag(AirframeClass::EpuRunning);

        // MODIFIED BY S.G. Instead of 50%, now uses an aiframe aux var
        // if ((rand() bitand 0x1) == 0x1) // JB 010115 half the time (13%/2) you won't be able to restart.
        if (((AircraftClass*)this)->af->GetEngineDamageNoRestartThreshold() > rand() % 100)
            ((AircraftClass*)this)->af->jfsaccumulator = -3600;
    }

    // JB 000816

    // debug non local
    if ( not IsLocal())
    {
        //    MonoPrint( "NonLocal Apply Damage: Pct Strength now: %f\n", pctStrength );
    }

    // BIGASSCOW - We need to make this a local sound so it can be heard inside the cockpit.
    if (soundIdx not_eq -1)
        SoundPos.Sfx(soundIdx, 0, 1, 0);

    // send out death message if strength below zero
    if (pctStrength <= 0.0F and dyingTimer < 0.0F)
    {
        // ground vehicles die immediately
        if (IsGroundVehicle())
            pctStrength = -1.0f;

        // only the local entity sends the death message
        if (IsLocal())
        {
            deathMessage = new FalconDeathMessage(Id(), FalconLocalGame);
            deathMessage->dataBlock.damageType = damageMessage->dataBlock.damageType;

            deathMessage->dataBlock.dEntityID  = Id();
            ShiAssert(GetCampaignObject())
            deathMessage->dataBlock.dCampID = ((CampBaseClass*)GetCampaignObject())->GetCampID();
            deathMessage->dataBlock.dSide = ((CampBaseClass*)GetCampaignObject())->GetOwner();
            deathMessage->dataBlock.dPilotID   = pilotSlot;
            deathMessage->dataBlock.dIndex     = Type();

            lastToHit = (SimVehicleClass*)vuDatabase->Find(LastShooter());

            if (lastToHit and not lastToHit->IsEject() and lastToHit->Id() not_eq damageMessage->dataBlock.fEntityID)
            {
                deathMessage->dataBlock.fEntityID = lastToHit->Id();
                deathMessage->dataBlock.fIndex = lastToHit->Type();

                if (lastToHit->IsSim())
                {
                    deathMessage->dataBlock.fCampID = ((SimVehicleClass*)lastToHit)->GetCampaignObject()->GetCampID();
                    deathMessage->dataBlock.fPilotID = ((SimVehicleClass*)lastToHit)->pilotSlot;
                    deathMessage->dataBlock.fSide = ((SimVehicleClass*)lastToHit)->GetCampaignObject()->GetOwner();
                }
                else
                {
                    deathMessage->dataBlock.fCampID = ((CampBaseClass*)lastToHit)->GetCampID();
                    deathMessage->dataBlock.fPilotID = 0;
                    deathMessage->dataBlock.fSide = ((CampBaseClass*)lastToHit)->GetOwner();
                }

                // We really don't know what weapon did the killing. Pick a non-existant one
                deathMessage->dataBlock.fWeaponID  = 0xffff;
                deathMessage->dataBlock.fWeaponUID = FalconNullId;
            }
            else
            {
                // If aircraft is undamaged, the death message we send is 'ground collision',
                // since the aircraft would probably get there eventually
                deathMessage->dataBlock.fEntityID  = damageMessage->dataBlock.fEntityID;
                deathMessage->dataBlock.fIndex     = damageMessage->dataBlock.fIndex;
                deathMessage->dataBlock.fCampID    = damageMessage->dataBlock.fCampID;
                deathMessage->dataBlock.fPilotID   = damageMessage->dataBlock.fPilotID;
                deathMessage->dataBlock.fSide      = damageMessage->dataBlock.fSide;
                deathMessage->dataBlock.fWeaponID  = damageMessage->dataBlock.fWeaponID;
                deathMessage->dataBlock.fWeaponUID = damageMessage->dataBlock.fWeaponUID;
            }

            deathMessage->dataBlock.deathPctStrength = pctStrength;
            // me123 deathMessage->RequestOutOfBandTransmit ();
        }

        // this is now done in the exec() function when hitpoints reaches
        // the appropriate value
        // FalconSendMessage (deathMessage,FALSE);

        // short delay before vehicle starts death throw effects
        dyingTimer = 0.0f;
        // dyingType = PRANDInt5();
        dyingType = rand() bitand 0x07;

        //MonoPrint ("Vehicle %d in death throws at %8ld\n", Id().num_, SimLibElapsedTime);
        //MonoPrint ("Dying Type = %d\n", dyingType);
    }
}

// Generic fire message used by all platforms
void SimVehicleClass::SendFireMessage(SimWeaponClass* curWeapon, int type, int startFlag, SimObjectType* targetPtr, VU_ID targetId)
{
    FalconWeaponsFire* fireMsg;
#ifdef DEBUG
    static int fireCount = 0;

    // MonoPrint ("Firing %d at %d. mesg count %d\n", type, SimLibElapsedTime, fireCount);
    fireCount ++;
#endif

    fireMsg = new FalconWeaponsFire(Id(), FalconLocalGame);
    fireMsg->dataBlock.fEntityID  = Id();
    fireMsg->dataBlock.weaponType  = type;
    ShiAssert(GetCampaignObject());
    fireMsg->dataBlock.fCampID = ((CampBaseClass*)GetCampaignObject())->GetCampID();
    fireMsg->dataBlock.fSide = ((CampBaseClass*)GetCampaignObject())->GetOwner();
    fireMsg->dataBlock.fPilotID   = pilotSlot;
    fireMsg->dataBlock.fIndex     = Type();

    // For rockets, get the pod id instead of the weapon ID
    // JB 010104 Marco Edit
    //if (IsAirplane() and type == FalconWeaponsFire::Rocket)
    //   fireMsg->dataBlock.fWeaponID = static_cast<ushort>(((AircraftClass*)this)->Sms->hardPoint[((AircraftClass*)this)->Sms->CurHardpoint()]->GetRackId());
    //else
    // JB 010104 Marco Edit
    fireMsg->dataBlock.fWeaponID  = curWeapon->Type();

    fireMsg->dataBlock.dx = 0.0f;
    fireMsg->dataBlock.dy = 0.0f;
    fireMsg->dataBlock.dz = 0.0f;

    if (type == FalconWeaponsFire::GUN)
    {
        if (((GunClass *)curWeapon)->IsTracer())
        {
            if (startFlag)
            {
                // MonoPrint (" startFlag\n" );
                ((GunClass*)curWeapon)->NewBurst();
                fireMsg->dataBlock.fWeaponUID.num_ = ((GunClass*)curWeapon)->GetCurrentBurst();
                fireMsg->dataBlock.dx = ((GunClass*)curWeapon)->bullet[0].xdot;
                fireMsg->dataBlock.dy = ((GunClass*)curWeapon)->bullet[0].ydot;
                fireMsg->dataBlock.dz = ((GunClass*)curWeapon)->bullet[0].zdot;
            }
            else
            {
                // MonoPrint (" endFlag\n" );
                // Null Id means were stopping gun fire.
                fireMsg->dataBlock.fWeaponUID.num_ = 0;
                fireMsg->RequestReliableTransmit();
            }
        }
        else
        {
            // MonoPrint ("GUN BURST\n" );
            ((GunClass*)curWeapon)->NewBurst();
            fireMsg->dataBlock.fWeaponUID.num_ = ((GunClass*)curWeapon)->GetCurrentBurst();
        }

    }
    else
    {
        fireMsg->RequestReliableTransmit();
        fireMsg->RequestOutOfBandTransmit();
        //  MonoPrint ("OTHER WEAPON \n" );
        // Make sure the weapon's shooter pilot slot is the same as the current pilot
        curWeapon->shooterPilotSlot = pilotSlot;
        fireMsg->dataBlock.fWeaponUID = curWeapon->Id();
    }

    if (targetPtr)
        fireMsg->dataBlock.targetId   = targetPtr->BaseData()->Id();
    else if (targetId not_eq vuNullId)
        fireMsg->dataBlock.targetId   = targetId;
    else
        fireMsg->dataBlock.targetId   = vuNullId;

    FalconSendMessage(fireMsg);
}

/*
** Name: ApplyProximityDamage
** Description:
** Cycles thru objectList check for proximity.
** Cycles thru all objectives, and checks vs individual features
**        if it's within the objective's bounds.
*/
void
SimVehicleClass::ApplyProximityDamage(void)
{
    float tmpX, tmpY, tmpZ;
    float rangeSquare;
    SimBaseClass* testObject;
    float damageRadiusSqrd;
#ifdef VU_GRID_TREE_Y_MAJOR
    VuGridIterator gridIt(ObjProxList, YPos(), XPos(), NM_TO_FT * 3.5F);
#else
    VuGridIterator gridIt(ObjProxList, XPos(), YPos(), NM_TO_FT * 3.5F);
#endif

    FalconDamageMessage* message;
    float normBlastDist;

    // when on ground, damage radius is smaller
    if (OnGround())
    {
        damageRadiusSqrd = 150.0f * 150.0f;
    }
    else
    {
        damageRadiusSqrd = 300.0f * 300.0f;
    }

    if ( not SimDriver.objectList)
    {
        return;
    }

    VuListIterator objectWalker(SimDriver.objectList);

    // Check vs vehicles
    testObject = (SimBaseClass*) objectWalker.GetFirst();

    while (testObject)
    {
        // don't send msg to objects already dead or dying
        // no damge to camp units
        if (testObject->IsCampaign() or testObject->pctStrength < 0.0f)
        {
            testObject = (SimBaseClass*) objectWalker.GetNext();
            continue;
        }

        if (testObject not_eq this)
        {
            tmpX = testObject->XPos() - XPos();
            tmpY = testObject->YPos() - YPos();
            tmpZ = testObject->ZPos() - ZPos();

            rangeSquare = tmpX * tmpX + tmpY * tmpY + tmpZ * tmpZ;

            if (rangeSquare < damageRadiusSqrd)
            {
                // edg: calculate a normalized blast Dist
                normBlastDist = (damageRadiusSqrd - rangeSquare) / (damageRadiusSqrd);

                // quadratic dropoff
                normBlastDist *= normBlastDist;

                message = new FalconDamageMessage(testObject->Id(), FalconLocalGame);
                message->dataBlock.fEntityID  = Id();
                message->dataBlock.fCampID = GetCampID();
                message->dataBlock.fSide   = static_cast<uchar>(GetCountry());
                message->dataBlock.fPilotID   = pilotSlot;
                message->dataBlock.fIndex     = Type();
                message->dataBlock.fWeaponID  = Type();
                message->dataBlock.fWeaponUID = Id();

                message->dataBlock.dEntityID  = testObject->Id();
                message->dataBlock.dCampID = testObject->GetCampID();
                message->dataBlock.dSide   = static_cast<uchar>(testObject->GetCountry());

                if (testObject->IsSimObjective())
                    message->dataBlock.dPilotID   = 255;
                else
                    message->dataBlock.dPilotID   = ((SimMoverClass*)testObject)->pilotSlot;

                message->dataBlock.dIndex     = testObject->Type();

                message->dataBlock.damageRandomFact = 1.0f;
                message->dataBlock.damageStrength = normBlastDist * 120.0f;
                message->dataBlock.damageType = FalconDamageType::ProximityDamage;
                // me123 message->RequestOutOfBandTransmit ();
                FalconSendMessage(message, TRUE);

            }
        }

        testObject = (SimBaseClass*) objectWalker.GetNext();
    }
}
//MI
void SimVehicleClass::StepSOI(int dir)
{
    SMSClass *Sms = (SMSClass*)GetSMS();
    MissileClass* theMissile = NULL;
    MissileDisplayClass* mslDisplay = NULL;
    SensorClass* tPodDisplay = NULL;
    RadarClass* theRadar = (RadarClass*) FindSensor(this, SensorClass::Radar);
    //MI
    FireControlComputer* FCC = ((SimVehicleClass*)this)->GetFCC();

    if ( not theRadar or not FCC or not Sms or not SimDriver.GetPlayerEntity())
        return;

    if (Sms)
        theMissile = (MissileClass*)Sms->GetCurrentWeapon();



    if (theMissile and theMissile->IsMissile())
    {
        if (theMissile->GetDisplayType() == MissileClass::DisplayBW or
            theMissile->GetDisplayType() == MissileClass::DisplayIR or
            theMissile->GetDisplayType() == MissileClass::DisplayHTS)
        {
            mslDisplay = (MissileDisplayClass*)theMissile->display;
        }
    }

    tPodDisplay = FindLaserPod(this);

    //dir 1 = up
    //dir 2 = down
    switch (dir)
    {
        case 1:

            //Can the SOI move to the HUD?
            //ok let's see... AGR can't be the SOI
            //info from Snako
            /*o If CCIP, CCIP rockets, STRF, DTOS, or EO-VIS is selected, the forward position moves the
            SOI to the HUD and makes it the SOI.  When LADD, EO PRE, or CCRP submode is selected along with
            the IP or RP, the SOI designations will move to the HUD when the DMS is moved forward.  When TWS
             is selected, TWS AUTO or MAN submode will be selected.
            o The UP direction wouldn't have any effect on A2A master modes.
            o If the aft (down) position is selected, the SOI moves to the MFD of the highest priority.
            Subsequent aft depressions move the SOI to the opposite MFD.
            */
            if (FCC->GetSubMode() == FireControlComputer::CCIP or
                FCC->GetSubMode() == FireControlComputer::DTOSS or
                //FCC->GetSubMode() == FireControlComputer::RCKT or
                FCC->GetMasterMode() == FireControlComputer::AirGroundRocket or // MLR 4/3/2004 -
                FCC->GetSubMode() == FireControlComputer::STRAF or
                FCC->GetSubMode() == FireControlComputer::CCRP or
                FCC->GetSubMode() == FireControlComputer::LADD)
            {
                SOIManager(SOI_HUD);
            }

            break;

        case 2:

            //Toggles between the MFD's.
            //Check if we are currently in HSD SOI
            if (FCC->IsSOI)
            {
                //Yes, can our radar be the SOI?
                //if we are in AG MasterMode, there's not many possibilities
                if (FCC->IsAGMasterMode())
                {
                    if (FCC->GetSubMode() not_eq FireControlComputer::CCIP and FCC->GetSubMode() not_eq FireControlComputer::DTOSS)
                    {
                        SOIManager(SOI_RADAR);
                    }
                }
                else if (FCC->IsAAMasterMode() or FCC->IsNavMasterMode() or FCC->GetMasterMode() ==
                         FireControlComputer::MissileOverride or FCC->GetMasterMode() ==
                         FireControlComputer::Dogfight)
                {
                    SOIManager(SOI_RADAR);
                }
            }
            else
            {
                //No, currently not SOI. If we have the HSD visible, we're going to set it there
                if (FCC->CouldBeSOI)
                    SOIManager(SOI_FCC);
                else if ((mslDisplay and not mslDisplay->IsSOI()) or (tPodDisplay and not tPodDisplay->IsSOI()))
                    SOIManager(SOI_WEAPON);
                else
                    SOIManager(SOI_RADAR);
            }

            break;

        default:
            ShiWarning("Wrong SOI Direction");
            break;
    }
}
