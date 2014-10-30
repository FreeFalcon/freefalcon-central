#include "stdhdr.h"
#include "Graphics/Include/drawbsp.h"
#include "Graphics/Include/drawsgmt.h"
#include "Graphics/Include/drawpuff.h"
#include "Graphics/Include/RenderOW.h"
#include "DrawParticleSys.h" //RV - I-Hawk - Added to support RV new trails
#include "falcmesg.h"
#include "aircrft.h"
#include "fsound.h"
#include "soundfx.h"
#include "MsgInc/DamageMsg.h"
#include "MsgInc/DeathMessage.h"
#include "campbase.h"
#include "simdrive.h"
#include "simstatc.h"
#include "hardpnt.h"
#include "camp2sim.h"
#include "digi.h"
#include "MsgInc/WingmanMsg.h"
#include "airframe.h"
#include "otwdrive.h"
#include "object.h"
#include "sms.h"
#include "sfx.h"
#include "fakerand.h"
#include "simeject.h"
#include "fault.h"
#include "fack.h"
#include "falcsess.h"
#include "Classtbl.h"
#include "SimEject.h"
#include "simweapn.h"
#include "playerop.h"
#include "weather.h"
#include "ffeedbk.h"
#include "dofsnswitches.h" //MI
#include "IvibeData.h"
#include "ACTurbulence.h"

extern void *gSharedIntellivibe;

extern bool g_bDisableFunkyChicken; // JB 010104
#include "ui/include/uicomms.h" // JB 010107
extern UIComms *gCommsMgr; // JB 010107
#include "terrtex.h" // JB carrier
extern float g_fCarrierStartTolerance; // JB carrier
extern bool g_bWakeTurbulence;


extern bool g_bNewDamageEffects; //MI

void CalcTransformMatrix(SimBaseClass* theObject);
void DecomposeMatrix(Trotation* matrix, float* pitch, float* roll, float* yaw);

//RV I-Hawk added local functions
float Get3DDistance(Tpoint &a, Tpoint &b);
Tpoint Get3DMiddle(Tpoint &a, Tpoint &b);
void AssignACOrientation(Trotation* &orientation, Tpoint &origin, Tpoint &position, Tpoint &worldPosition, bool right);

int flag_keep_smoke_trails = FALSE;
// wing tip vortex constants.
//static const float minwingvortexalt = 0; // where vortex conditions start
//static const float maxwingvortexalt = 10000; // where they end
//static const float wingvortexalpha = 15; // what alpha you need to exceed
//static const float wingvortexgs = 4; // what G you need to exceed

//ATARIBABY  wing vortex and vapor stuff
// wing tip vortex constants.
float minwingvortexalt = 0; // where vortex conditions start
float maxwingvortexalt = 25000; // where they end
float wingvortexAlpha = 12; // what alpha you need to exceed
float wingvortexgs = 3.5; // what G you need to exceed
float wingvortexAlphaHigh = 20; //RV - I-Hawk - AOA above this value enough to generate wingtip vortex trails
float wingvortexAlphaLimit = 29.5; //Above this value stop as probably AC is stalling
float wingvtxTrailSizeCx = 1;  //RV - I-Hawk added factors to suppoert RV new trails size/alpha changes
float wingvtxTrailAlphaCx = 1;

//RV - I-Hawk - Assigning 2 types of vortices trails, 1 is conditioned in high G maneuvers and 2nd
//is conditioned on high AOA (until passing the critical lift value), so there are different
//variables for each type. some ACs will use even 2 sets for trails to simulate the effect
//of smoke all over wing
float vortexAOA = 20; //RV - I-Hawk - low AOA where vapor trails start
float vortexAOALimit = 29.5; //Default value but is read from .dat file later
float vortexMinSpeedLimit = 150;
float vortexMaxaltLimit = 35000;

float vortexG = 5; //RV - I-Hawk - G value where vortex trails start
float vortex1AlphaCx;
float vortex1SizeCx;
float vortex2AlphaCx;
float vortex2SizeCx;

//RV -  I-Hawk - New variables to support RV new trails alpha/size change with altitude
float engineTrailSize = 1;
float engineTrailAlpha = 1;
float engineTrailMargin = 1; //I-Hawk - added to support fade in/out of engine smoke trails with altitude
float engineTrailRPMFactor = 1; //I-Hawk - added to support fade in/out of engine trails with RPM

float damageTrailSizeCx = 1;
float damageTrailAlphaCx = 1;

//I-Hawk - added factors to support fade in/out of contrails
float contrailMargin = 1;
float contrailLowValue = 0;
float contrailHighValue ;
float contrailLow10Percent ;
float contrailLow90Percent ;
float contrailHigh110Percent ;

Trotation* orientation;

void AircraftClass::ApplyDamage(FalconDamageMessage* damageMessage)
{
    float rx = 0.0F, ry = 0.0F, rz = 0.0F;
    float cosAta = 1.0F, sinAta = 0.0F;
    int octant = 0;
    long failuresPossible = 0;
    int failures = 0;
    int startStrength = 0, damageDone = 0;
    SimBaseClass *object = NULL;
    float dx = 0.0F;
    float dy = 0.0F;
    float dz = 0.0F;
    float range = 0.0F;

    if (IsDead() or IsExploding())
    {
        return;
    }

    object = (SimBaseClass *)vuDatabase->Find(damageMessage->dataBlock.fEntityID);

    if (object)
    {
        dx = object->XPos() - XPos();
        dy = object->YPos() - YPos();
        dz = object->ZPos() - ZPos();
        range = (float)(dx * dx + dy * dy + dz * dz);

        // Find relative position
        rx = dmx[0][0] * dx + dmx[0][1] * dy + dmx[0][2] * dz;
        ry = dmx[1][0] * dx + dmx[1][1] * dy + dmx[1][2] * dz;
        rz = dmx[2][0] * dx + dmx[2][1] * dy + dmx[2][2] * dz;

        // Find the octant
        octant = 0;

        if (rx > 0.0F) // In front
            octant = 1;

        if (ry > 0.0F) // On right
            octant += 2;

        if (rz > 0.0F) // below
            octant += 4;

        cosAta = rx / range;
        sinAta = (float)sqrt(range - rx * rx) / range;
    }
    else
    {
        cosAta = 1.0F;
        sinAta = 0.0F;
        octant = FloatToInt32(PRANDFloatPos() * 7.0F);
    }

    if (damageMessage->dataBlock.damageType == FalconDamageType::ObjectCollisionDamage)
    {
        Tpoint Objvec, Myvec, relVec;
        float relVel = 0.0F, objMass = 0.0F;

        float deltaSig = 0.0F, deltaGmma = 0.0F;



        Myvec.x = XDelta();
        Myvec.y = YDelta();
        Myvec.z = ZDelta();

        if (object and object->IsSim())
        {
            Objvec.x = object->XDelta();
            Objvec.y = object->YDelta();
            Objvec.z = object->ZDelta();
            objMass = object->Mass();

            if (object->IsAirplane())
                deltaSig = ((AircraftClass*)object)->af->sigma - af->sigma;
            else
                deltaSig = object->Yaw() - af->sigma;

            if (deltaSig * RTD < -180.0F)
                deltaSig += 360.0F * DTR;
            else if (deltaSig * RTD > 180.0F)
                deltaSig -= 360.0F * DTR;

            af->sigma -= deltaSig * 0.75F * sinAta * objMass / Mass();

            if (object->IsAirplane())
                deltaGmma = ((AircraftClass*)object)->af->gmma - af->gmma;
            else
                deltaGmma = object->Pitch() - af->gmma;

            if (deltaGmma * RTD < -90.0F)
                deltaGmma += 180.0F * DTR;
            else if (deltaGmma * RTD > 90.0F)
                deltaGmma -= 180.0F * DTR;

            af->gmma -= deltaGmma * 0.75F * sinAta * objMass / Mass();

            af->ResetOrientation();
        }
        else
        {
            Objvec.x = 0.0F;
            Objvec.y = 0.0F;
            Objvec.z = 0.0F;
            objMass = 2500.0F;
            cosAta = 1.0F;
        }

        relVec.x = Myvec.x - Objvec.x;
        relVec.y = Myvec.y - Objvec.y;
        relVec.z = Myvec.z - Objvec.z;

        relVel = (float)sqrt(relVec.x * relVec.x + relVec.y * relVec.y + relVec.z * relVec.z);

        // poke airframe data to simulate collisions
        af->x -= relVec.x * objMass / Mass() * cosAta;
        af->y -= relVec.y * objMass / Mass() * cosAta;
        af->z -= relVec.z * objMass / Mass() * cosAta;

        if (IsSetFalcFlag(FEC_INVULNERABLE))
        {
            af->vt *= 0.9F;
        }
        else
        {
            af->vt -= relVel * objMass / Mass() * cosAta;
            af->vt = max(0.0F, af->vt);
        }
    }
    else if (damageMessage->dataBlock.damageType == FalconDamageType::FeatureCollisionDamage)
    {
        af->x -= af->xdot * cosAta;
        af->y -= af->ydot * cosAta;
        af->z -= af->zdot * cosAta;

        if (IsSetFalcFlag(FEC_INVULNERABLE))
        {
            af->vt *= 0.9F;
        }
        else
        {
            af->vt = -0.01F;
        }
    }
    else if (damageMessage->dataBlock.damageType == FalconDamageType::MissileDamage)
    {
        if (this == SimDriver.GetPlayerEntity())
        {
            JoystickPlayEffect(JoyHitEffect, FloatToInt32((float)atan2(ry, rx) * RTD));
        }
    }

    startStrength = FloatToInt32(strength);
    // call generic vehicle damage stuff
    SimVehicleClass::ApplyDamage(damageMessage);
    damageDone = startStrength - FloatToInt32(strength);

    if (this == FalconLocalSession->GetPlayerEntity())
    {
        g_intellivibeData.lastdamage = octant + 1;
        g_intellivibeData.damageforce = (float)damageDone;
        g_intellivibeData.whendamage = SimLibElapsedTime;
        memcpy(gSharedIntellivibe, &g_intellivibeData, sizeof(g_intellivibeData));
    }

    // do any type specific stuff here:
    //   if (IsLocal() and not (this == (SimBaseClass*)SimDriver.GetPlayerEntity() and PlayerOptions.InvulnerableOn()))
    if (IsLocal() and not IsSetFalcFlag(FEC_INVULNERABLE))
    {
        // Randomly break something based on the damage inflicted
        // Find out who did it

        /* Fault types
           acmi - Air Combat maneuvering Instrumentation Pod, use when tape full?
           amux - Avionics Data Bus. If failed, fail all avionics (very rare)
            blkr - Interference Blanker. If failed, radio drives radar crazy
           cadc - Central Air Data Computer. If failed no baro altitude
            dmux - Weapon Data Bus. If failed, no weapons fire from given station
           dlnk - Improved Data Modem. If failed no data transfer in
            eng  - Engine. If failed usually means fire, but could mean hydraulics
            fcc  - Fire Control Computer. If failed, no weapons solutions
            fcr  - Fire Control Radar. If failed, no radar
           flcs - Digital Flight Control System. If failed one or more control surfaces stuck
            fms  - Fuel Measurement System. If failed, fuel gauge stuck
           gear - Landing Gear. If failed one or more wheels stuck
            hud  - Heads Up Display. If failed, no HUD
           iff  - Identification, Friend or Foe. If failed, no IFF
            ins  - Inertial Navigation System. If failed, no change in waypoint data
           mfds - Multi Function Display Set. If an MFD fails, it shows noise
            ralt - Radar Altimeter. If failed, no ALOW warning
           rwr  - Radar Warning Reciever. If failed, no rwr
            sms  - Stores Management System. If failed, no weapon or inventory display, and no launch
           tcn  - TACAN. If failed no TACAN data
            msl  - Missile Slave Loop. If failed, missile will not slave to radar
           ufc  - Up Front Controller. If failed, UFC/DED inoperative
        */
        switch (octant)
        {
            case 0: // Back, Left, Upper
                failuresPossible =
                    (1 << FaultClass::eng_fault) +
                    (1 << FaultClass::eng2_fault) +
                    (1 << FaultClass::hud_fault) +
                    (1 << FaultClass::fcc_fault) +
                    (1 << FaultClass::flcs_fault) +
                    (1 << FaultClass::mfds_fault) +
                    (1 << FaultClass::rwr_fault);

                failures = 7;
                //            MonoPrint ("Upper Left Back Damage\n");
                break;

            case 1: // Front, Left, Upper
                failuresPossible = 2 xor FaultClass::NumFaultListSubSystems - 1 -
                                   (1 << FaultClass::dmux_fault) -
                                   (1 << FaultClass::eng_fault) -
                                   (1 << FaultClass::eng2_fault) -
                                   (1 << FaultClass::gear_fault) -
                                   (1 << FaultClass::ralt_fault) -
                                   (1 << FaultClass::msl_fault);
                failures = 7;

                //            MonoPrint ("Upper Left Front Damage\n");
                //MI add LEF damage
                if (g_bRealisticAvionics and g_bNewDamageEffects)
                {
                    //random canopy damage
                    if (rand() % 100 < 10) //low chance
                        CanopyDamaged = TRUE;

                    if (rand() % 100 < 25 and not LEFState(LT_LEF_OUT)) //25% we're taking damage
                    {
                        if (LEFState(LT_LEF_DAMAGED))
                        {
                            LEFOn(LT_LEF_OUT);
                            LTLEFAOA = max(min(af->alpha, 25.0f) * DTR, 0.0f);
                        }
                        else
                        {
                            LEFOn(LT_LEF_DAMAGED);
                            LTLEFAOA = max(min(af->alpha, 25.0f) * DTR, 0.0f);

                            if (rand() % 100 > 20)
                                LEFOn(LT_LEF_OUT);
                        }
                    }
                }

                break;

            case 2:  // Back Right, Upper
                failuresPossible =
                    (1 << FaultClass::eng_fault) +
                    (1 << FaultClass::eng2_fault) +
                    (1 << FaultClass::hud_fault) +
                    (1 << FaultClass::fcc_fault) +
                    (1 << FaultClass::flcs_fault) +
                    (1 << FaultClass::mfds_fault) +
                    (1 << FaultClass::rwr_fault);
                failures = 7;
                //            MonoPrint ("Upper Right Back Damage\n");
                break;

            case 3:  // Front Right, Upper
                failuresPossible = 2 xor FaultClass::NumFaultListSubSystems - 1 -
                                   (1 << FaultClass::dmux_fault) -
                                   (1 << FaultClass::eng_fault) -
                                   (1 << FaultClass::eng2_fault) -
                                   (1 << FaultClass::gear_fault) -
                                   (1 << FaultClass::ralt_fault) -
                                   (1 << FaultClass::msl_fault);
                failures = 7;

                //            MonoPrint ("Upper Right Front Damage\n");
                //MI add LEF damage
                if (g_bRealisticAvionics and g_bNewDamageEffects)
                {
                    if (rand() % 100 < 10) //low chance
                        CanopyDamaged = TRUE;

                    if (rand() % 100 < 25 and not LEFState(RT_LEF_OUT)) //25% we're taking damage
                    {
                        if (LEFState(RT_LEF_DAMAGED))
                        {
                            LEFOn(RT_LEF_OUT);
                            RTLEFAOA = max(min(af->alpha, 25.0f) * DTR, 0.0f);
                        }
                        else
                        {
                            LEFOn(RT_LEF_DAMAGED);
                            RTLEFAOA = max(min(af->alpha, 25.0f) * DTR, 0.0f);

                            if (rand() % 100 > 20)
                                LEFOn(RT_LEF_OUT);
                        }
                    }
                }

                break;

            case 4: // Back, Left, Upper
                failuresPossible = 2 xor FaultClass::NumFaultListSubSystems - 1 -
                                   (1 << FaultClass::hud_fault) -
                                   (1 << FaultClass::mfds_fault) -
                                   (1 << FaultClass::ufc_fault);
                failures = 4;
                //            MonoPrint ("Lower Left Back Damage\n");
                break;

            case 5: // Front, Left, Lower
                failuresPossible = 2 xor FaultClass::NumFaultListSubSystems - 1 -
                                   (1 << FaultClass::eng_fault) -
                                   (1 << FaultClass::eng2_fault) -
                                   (1 << FaultClass::hud_fault) -
                                   (1 << FaultClass::mfds_fault) -
                                   (1 << FaultClass::ufc_fault);
                failures = 6;

                //            MonoPrint ("Lower Left Front Damage\n");
                //MI add LEF damage
                if (g_bRealisticAvionics and g_bNewDamageEffects)
                {
                    if (rand() % 100 < 25 and not LEFState(LT_LEF_OUT)) //25% we're taking damage
                    {
                        if (LEFState(LT_LEF_DAMAGED))
                        {
                            LEFOn(LT_LEF_OUT);
                            LTLEFAOA = max(min(af->alpha, 25.0f) * DTR, 0.0f);
                        }
                        else
                        {
                            LEFOn(LT_LEF_DAMAGED);
                            LTLEFAOA = max(min(af->alpha, 25.0f) * DTR, 0.0f);

                            if (rand() % 100 > 20)
                                LEFOn(LT_LEF_OUT);
                        }
                    }
                }

                break;

            case 6:  // Back Right, Lower
                failuresPossible = 2 xor FaultClass::NumFaultListSubSystems - 1 -
                                   (1 << FaultClass::hud_fault) -
                                   (1 << FaultClass::mfds_fault) -
                                   (1 << FaultClass::ufc_fault);
                failures = 4;
                //            MonoPrint ("Lower Right Back Damage\n");
                break;

            case 7:  // Front Right, Lower
            default:
                failuresPossible = 2 xor FaultClass::NumFaultListSubSystems - 1 -
                                   (1 << FaultClass::eng_fault) -
                                   (1 << FaultClass::eng2_fault) -
                                   (1 << FaultClass::hud_fault) -
                                   (1 << FaultClass::mfds_fault) -
                                   (1 << FaultClass::ufc_fault);
                failures = 6;

                //            MonoPrint ("Lower Right Front Damage\n");
                //MI add LEF damage
                if (g_bRealisticAvionics and g_bNewDamageEffects)
                {
                    if (rand() % 100 < 25 and not LEFState(RT_LEF_OUT)) //25% we're taking damage
                    {
                        if (LEFState(RT_LEF_DAMAGED))
                        {
                            LEFOn(RT_LEF_OUT);
                            RTLEFAOA = GetDOFValue(IsComplex() ? COMP_LT_LEF : SIMP_LT_LEF);
                        }
                        else
                        {
                            LEFOn(RT_LEF_DAMAGED);
                            RTLEFAOA = GetDOFValue(IsComplex() ? COMP_RT_LEF : SIMP_RT_LEF);

                            if (rand() % 100 > 20)
                                LEFOn(RT_LEF_OUT);
                        }
                    }
                }

                break;
        }

        AddFault(failures, failuresPossible, damageDone / 5, octant);
    }
}

void AircraftClass::InitDamageStation(void)
{
}

//RV - I-Hawk - all engine trails (damage, contrails, colorTrails, engine smoke) are killed here
void AircraftClass::CleanupDamageStation()
{
    for (int i = 0; i < TRAIL_MAX; i++)
    {
        if (smokeTrail[i])
        {

            //RV - I-Hawk - RV new trails call changes
            //OTWDriver.RemoveObject(smokeTrail[i], TRUE);
            DrawableParticleSys::PS_KillTrail(smokeTrail_trail[i]);
            smokeTrail[i] = smokeTrail_trail[i] = NULL;
        }
    }

    for (int i = 0; i < MAXENGINES; i++)
    {
        if (conTrails[i])
        {
            //OTWDriver.RemoveObject(conTrails[i], TRUE);
            DrawableParticleSys::PS_KillTrail(conTrails_trail[i]);
            conTrails[i] = conTrails_trail[i] = NULL;
        }

        if (colorConTrails[i])
        {
            //OTWDriver.RemoveObject(conTrails[i], TRUE);
            DrawableParticleSys::PS_KillTrail(colorConTrails_trail[i]);
            colorConTrails[i] = colorConTrails_trail[i] = NULL;
        }


        if (engineTrails[i])
        {
            //OTWDriver.RemoveObject(engineTrails[i], TRUE);
            DrawableParticleSys::PS_KillTrail(engineTrails_trail[i]);
            engineTrails[i] = engineTrails_trail[i] = NULL;
        }
    }
}

//RV - I-Hawk - Killing of vortex trails and dusttrail is done here
void AircraftClass::CleanupVortex()
{
    if (dustTrail)
    {

        DrawableParticleSys::PS_KillTrail(dustTrail_trail);
        dustTrail = dustTrail_trail = NULL;
    }

    if (lwingvortex)
    {

        DrawableParticleSys::PS_KillTrail(lwingvortex_trail);
        lwingvortex = lwingvortex_trail = NULL;
    }

    if (rwingvortex)
    {

        DrawableParticleSys::PS_KillTrail(rwingvortex_trail);
        rwingvortex = rwingvortex_trail = NULL;
    }

    if (rvortex1)
    {

        DrawableParticleSys::PS_KillTrail(rvortex1_trail);
        rvortex1 = rvortex1_trail = NULL;
    }

    if (lvortex1)
    {

        DrawableParticleSys::PS_KillTrail(lvortex1_trail);
        lvortex1 = lvortex1_trail = NULL;
    }

    if (rvortex2)
    {

        DrawableParticleSys::PS_KillTrail(rvortex2_trail);
        rvortex2 = rvortex2_trail = NULL;
    }

    if (lvortex2)
    {

        DrawableParticleSys::PS_KillTrail(lvortex2_trail);
        lvortex2 = lvortex2_trail = NULL;
    }
}

//////////////////////////////////////
#define DAMAGEF16_NOSE_SLOTINDEX 0
#define DAMAGEF16_FRONT_SLOTINDEX 1
#define DAMAGEF16_BACK_SLOTINDEX 2
#define DAMAGEF16_RWING_SLOTINDEX 3
#define DAMAGEF16_LWING_SLOTINDEX 4
#define DAMAGEF16_LSTAB_SLOTINDEX 5
#define DAMAGEF16_RSTAB_SLOTINDEX 6

#define DAMAGEF16_RWING 0x01
#define DAMAGEF16_LWING 0x02
#define DAMAGEF16_LSTAB 0x04
#define DAMAGEF16_RSTAB 0x08
#define DAMAGEF16_BACK 0x10
#define DAMAGEF16_FRONT 0x20
#define DAMAGEF16_NOSE 0x40
#define DAMAGEF16_ALL 0x7f

#define DAMAGEF16_NOLEFT 0x79
#define DAMAGEF16_NOLEFTANDNOSE 0x39

#define DAMAGEF16_NORIGHT 0x76
#define DAMAGEF16_NORIGHTANDNOSE 0x36

#define DAMAGEF16_ONLYBODY 0x70
#define DAMAGEF16_NONOSE 0x3f
#define DAMAGEF16_BACKWITHWING 0x1f

#define DAMAGEF16_BACKWITHRIGHT 0x19
#define DAMAGEF16_BACKWITHLEFT 0x16

#define DAMAGEF16_TOLEFT 1
#define DAMAGEF16_TORIGHT 2
#define DAMAGEF16_TOLEFTRIGHT 3
#define DAMAGEF16_TOFRONT 4

#define DAMAGEF16_ID VIS_CF16A
#define DAMAGEF16_SWITCH 0
#define DAMAGEF16_NOSEBREAK_SWITCH 1
#define DAMAGEF16_FRONTBREAK_SWITCH 2
#define DAMAGEF16_LWINGBREAK_SWITCH 3
#define DAMAGEF16_RWINGBREAK_SWITCH 4
#define DAMAGEF16_CANOPYBREAK_SWITCH 5

int AircraftClass::SetDamageF16PieceType(DamageF16PieceStructure *piece, int type, int flag, int mask, float speed)
{
    if ( not (type bitand mask)) return 0;

    piece -> damage = MapVisId(DAMAGEF16_ID);
    piece -> mask = type;
    piece -> sfxtype = SFX_SMOKING_PART;
    piece -> sfxflag = SFX_F16CRASHLANDING bitor SFX_MOVES bitor SFX_BOUNCES;
    piece -> lifetime = 300.0f;

    piece -> pitch = 0.0f;
    piece -> roll = 0.0f;

    piece -> yd = 0.0f;
    piece -> pd = 0.0f;
    piece -> rd = 0.0f;

    piece -> dx = 1.0f;
    piece -> dy = 1.0f;
    piece -> dz = 1.0f;

    float angle = 0.0F, angle1 = 0.0F, angle2 = 0.0F, roll = 0.0F;

    if (PRANDFloatPos() > 0.5f)
        roll = 110.0f * DTR;
    else
        roll = 40.0f * DTR;

    angle1 = 0.0f;
    angle2 = 0.0f;

    switch (type)
    {
        case DAMAGEF16_NOLEFTANDNOSE:
        case DAMAGEF16_NOLEFT:
            piece -> sfxflag or_eq SFX_F16CRASH_OBJECT;
            piece -> roll = -roll;
            piece -> index = DAMAGEF16_FRONT_SLOTINDEX;
            speed += 20.0f;
            angle = 270.0f * DTR;
            piece -> dz = 2.0f;
            piece -> dx = 0.75f;
            break;

        case DAMAGEF16_NORIGHTANDNOSE:
        case DAMAGEF16_NORIGHT:
            piece -> sfxflag or_eq SFX_F16CRASH_OBJECT;
            piece -> roll = roll;
            piece -> index = DAMAGEF16_FRONT_SLOTINDEX;
            speed += 20.0f;
            angle = 270.0f * DTR;
            piece -> dz = 2.0f;
            piece -> dx = 0.75f;
            break;

        case DAMAGEF16_ONLYBODY:
            piece -> sfxflag or_eq SFX_F16CRASH_OBJECT;

            if (PRANDFloatPos() > 0.5f) roll = -roll;

            piece -> roll = roll;
            piece -> index = DAMAGEF16_FRONT_SLOTINDEX;
            speed += 20.0f;
            angle = 360.0f * DTR;
            piece -> dz = 1.5f;
            piece -> dx = 0.75f;
            break;

        case DAMAGEF16_NONOSE:
            piece -> sfxflag or_eq SFX_F16CRASH_OBJECT;
            piece -> index = DAMAGEF16_FRONT_SLOTINDEX;
            speed += 10.0f;
            angle = 270.0f * DTR;
            piece -> dz = 1.25f;
            piece -> dx = 0.5f;
            break;

        case DAMAGEF16_BACKWITHWING:
            piece -> index = DAMAGEF16_BACK_SLOTINDEX;
            speed += 15.0f;
            angle = 270.0f * DTR;
            piece -> dz = 2.0f;
            piece -> dx = 0.75f;
            break;

        case DAMAGEF16_BACKWITHLEFT:
            piece -> roll = roll;
            piece -> index = DAMAGEF16_BACK_SLOTINDEX;
            speed += 15.0f;
            angle = 270.0f * DTR;
            piece -> dz = 2.0f;
            piece -> dx = 0.75f;
            break;

        case DAMAGEF16_BACKWITHRIGHT:
            piece -> roll = -roll;
            piece -> index = DAMAGEF16_BACK_SLOTINDEX;
            speed += 15.0f;
            angle = 270.0f * DTR;
            piece -> dz = 2.0f;
            piece -> dx = 0.75f;
            break;

        case DAMAGEF16_BACK:
            if (PRANDFloatPos() > 0.5f) roll = -roll;

            piece -> roll = roll;
            piece -> index = DAMAGEF16_BACK_SLOTINDEX;
            speed += 25.0f;
            angle = 360.0f * DTR;
            piece -> dz = 1.25f;
            piece -> dx = 0.75f;
            break;

        case DAMAGEF16_FRONT:
            piece -> sfxflag or_eq SFX_F16CRASH_OBJECT;
            piece -> roll = 40.0f * DTR;
            piece -> index = DAMAGEF16_FRONT_SLOTINDEX;
            speed += 30.0f;
            angle = 360.0f * DTR;
            piece -> dz = 1.125f;
            break;

        case DAMAGEF16_NOSE:
            piece -> pitch = -5.0f * DTR;
            piece -> sfxflag or_eq SFX_BOUNCES;
            piece -> index = DAMAGEF16_NOSE_SLOTINDEX;
            speed += 50.0f;
            angle = 450.0f * DTR;
            piece -> dx = 1.5f;
            piece -> dx += PRANDFloatPos();

            if (flag bitand DAMAGEF16_TOLEFTRIGHT)
            {
                piece -> dy += PRANDFloatPos();
            }

            break;

        case DAMAGEF16_RWING:
            piece -> sfxflag or_eq SFX_BOUNCES;
            piece -> index = DAMAGEF16_RWING_SLOTINDEX;

            if (flag bitand DAMAGEF16_TOLEFT)
            {
                speed += 30.0f;
                piece -> dy = 0.75f;
            }
            else
            {
                speed += 40.0f;
                piece -> dy = 1.5f;
            }

            if (flag bitand DAMAGEF16_TOFRONT)
            {
                piece -> dx += PRANDFloatPos();
            }

            piece -> dy += PRANDFloatPos();
            angle = 360.0f * DTR;
            piece -> dz = 1.0625f;
            break;

        case DAMAGEF16_LWING:
            piece -> sfxflag or_eq SFX_BOUNCES;
            piece -> index = DAMAGEF16_LWING_SLOTINDEX;

            if (flag bitand DAMAGEF16_TORIGHT)
            {
                speed += 30.0f;
                piece -> dy = 0.75f;
            }
            else
            {
                speed += 40.0f;
                piece -> dy = 1.5f;
            }

            if (flag bitand DAMAGEF16_TOFRONT)
            {
                piece -> dx += PRANDFloatPos();
            }

            piece -> dy += PRANDFloatPos();
            angle = -360.0f * DTR;
            piece -> dz = 1.0625f;
            break;

        case DAMAGEF16_LSTAB:
            piece -> sfxflag or_eq SFX_BOUNCES;
            piece -> index = DAMAGEF16_LSTAB_SLOTINDEX;

            if (flag bitand DAMAGEF16_TORIGHT)
            {
                speed += 40.0f;
                piece -> dy = 0.75f;
            }
            else
            {
                speed += 50.0f;
                piece -> dy = 1.5f;
            }

            if (flag bitand DAMAGEF16_TOFRONT)
            {
                piece -> dx += PRANDFloatPos();
            }

            piece -> dy += PRANDFloatPos();
            angle = -450.0f * DTR;
            break;

        case DAMAGEF16_RSTAB:
            piece -> sfxflag or_eq SFX_BOUNCES;
            piece -> index = DAMAGEF16_RSTAB_SLOTINDEX;

            if (flag bitand DAMAGEF16_TOLEFT)
            {
                speed += 40.0f;
                piece -> dy = 0.75f;
            }
            else
            {
                speed += 50.0f;
                piece -> dy = 1.5f;
            }

            if (flag bitand DAMAGEF16_TOFRONT)
            {
                piece -> dx += PRANDFloatPos();
            }

            piece -> dy += PRANDFloatPos();
            angle = 450.0f * DTR;
            break;

        default:
            angle = 0.0F;
            break;
    }

    speed += 50.0f * PRANDFloatPos();

    angle1 = 180.0f * DTR;
    angle2 = 360.0f * DTR;
    float s = speed * 0.1f;

    if (s > 8.0f) s = 8.0f;

    s = 1.0f / (9.0f - s);
    angle *= s;
    angle1 *= s;
    angle2 *= s;

    if (flag bitand DAMAGEF16_TOLEFTRIGHT)
    {
        if (flag bitand DAMAGEF16_TOFRONT)
        {
            angle *= 2.5f;
            angle1 *= 2.0f;
            angle2 *= 1.5f;
        }

        if (flag bitand DAMAGEF16_TOLEFT)
        {
            angle = -angle;
            angle1 = -angle1;
            angle2 = -angle2;
        }
    }

    piece -> yd = angle;
    piece -> pd = angle1;
    piece -> rd = angle2;

    piece -> dx *= XDelta();
    piece -> dy *= YDelta();
    piece -> dz *= ZDelta();

    if (this not_eq SimDriver.GetPlayerEntity())
    {
        piece -> sfxflag and_eq compl SFX_F16CRASH_OBJECT;
    }

    return 1;
}

int AircraftClass::CreateDamageF16Piece(DamageF16PieceStructure *piece, int *mask)
{
    float p = (float)asin(dmx[0][2]);

    if (p > 70.0f * DTR) return 0;

    float r = (float)atan2(dmx[1][2], dmx[2][2]);
    float range = 5.0f * DTR;
    float range1 = 15.0f * DTR;
    float range2 = 50.0f * DTR;
    int damagetype = 0;

    if (r > range)
    {
        damagetype or_eq 0x2; // right

        if (r > range2) damagetype or_eq 0x10;
    }
    else if (r < -range)
    {
        damagetype or_eq 0x1; // left

        if (r < -range2) damagetype or_eq 0x10;
    }

    if (p > range)
    {
        if (p > range2) damagetype or_eq 0x1c;
        else if (p > range1) damagetype or_eq 0x8;
        else damagetype or_eq 0x4;
    }

    float speed = (float)sqrt(XDelta() * XDelta() + YDelta() * YDelta() + ZDelta() * ZDelta());

    if (damagetype)
    {
        if (speed > 200.0f and speed < 500.0f)
        {
            if (damagetype bitand 0x4) damagetype or_eq 8;
        }
        else if (speed > 500.0f) damagetype or_eq 0x10;
    }

    int dirflag = DAMAGEF16_TOFRONT;
    int numpiece = 0;

    if (damagetype bitand 0x10)
    {
        // full damage
        if (damagetype bitand 0xc)   // damage to the front
        {
            dirflag = DAMAGEF16_TOFRONT;
            numpiece += SetDamageF16PieceType(&(piece[numpiece]), DAMAGEF16_LWING, dirflag, *mask, speed);
            numpiece += SetDamageF16PieceType(&(piece[numpiece]), DAMAGEF16_LSTAB, dirflag, *mask, speed);
            numpiece += SetDamageF16PieceType(&(piece[numpiece]), DAMAGEF16_RWING, dirflag, *mask, speed);
            numpiece += SetDamageF16PieceType(&(piece[numpiece]), DAMAGEF16_RSTAB, dirflag, *mask, speed);
            numpiece += SetDamageF16PieceType(&(piece[numpiece]), DAMAGEF16_NOSE,  dirflag, *mask, speed);
            numpiece += SetDamageF16PieceType(&(piece[numpiece]), DAMAGEF16_FRONT, dirflag, *mask, speed);
            numpiece += SetDamageF16PieceType(&(piece[numpiece]), DAMAGEF16_BACK,  dirflag, *mask, speed);
            *mask and_eq DAMAGEF16_BACK;
        }
        else if (damagetype bitand 0x3)
        {
            // damage to the left or right
            if (damagetype bitand 0x1)
                dirflag = DAMAGEF16_TOLEFT;
            else
                dirflag = DAMAGEF16_TORIGHT;

            numpiece += SetDamageF16PieceType(&(piece[numpiece]), DAMAGEF16_LWING, dirflag, *mask, speed);
            numpiece += SetDamageF16PieceType(&(piece[numpiece]), DAMAGEF16_LSTAB, dirflag, *mask, speed);
            numpiece += SetDamageF16PieceType(&(piece[numpiece]), DAMAGEF16_RWING, dirflag, *mask, speed);
            numpiece += SetDamageF16PieceType(&(piece[numpiece]), DAMAGEF16_RSTAB, dirflag, *mask, speed);
            numpiece += SetDamageF16PieceType(&(piece[numpiece]), DAMAGEF16_ONLYBODY, dirflag, *mask, speed);
            *mask and_eq DAMAGEF16_ONLYBODY;
        }
    }
    else
    {
        if (damagetype bitand 0x1)
        {
            // damage to the left
            dirflag = DAMAGEF16_TOLEFT;

            if (damagetype bitand 0x4)   // damage to the nose
            {
                dirflag or_eq DAMAGEF16_TOFRONT;
                numpiece += SetDamageF16PieceType(&(piece[numpiece]), DAMAGEF16_LWING, dirflag, *mask, speed);
                numpiece += SetDamageF16PieceType(&(piece[numpiece]), DAMAGEF16_LSTAB, dirflag, *mask, speed);
                numpiece += SetDamageF16PieceType(&(piece[numpiece]), DAMAGEF16_NOSE, dirflag, *mask, speed);
                numpiece += SetDamageF16PieceType(&(piece[numpiece]), DAMAGEF16_NOLEFTANDNOSE, dirflag, *mask, speed);
                *mask and_eq DAMAGEF16_NOLEFTANDNOSE;
            }
            else if (damagetype bitand 0x8)   // damage to the front
            {
                dirflag or_eq DAMAGEF16_TOFRONT;
                numpiece += SetDamageF16PieceType(&(piece[numpiece]), DAMAGEF16_LWING, dirflag, *mask, speed);
                numpiece += SetDamageF16PieceType(&(piece[numpiece]), DAMAGEF16_LSTAB, dirflag, *mask, speed);
                numpiece += SetDamageF16PieceType(&(piece[numpiece]), DAMAGEF16_NOSE, dirflag, *mask, speed);
                numpiece += SetDamageF16PieceType(&(piece[numpiece]), DAMAGEF16_FRONT, dirflag, *mask, speed);
                numpiece += SetDamageF16PieceType(&(piece[numpiece]), DAMAGEF16_BACKWITHRIGHT, dirflag, *mask, speed);
                *mask and_eq DAMAGEF16_BACKWITHRIGHT;
            }
            else
            {
                numpiece += SetDamageF16PieceType(&(piece[numpiece]), DAMAGEF16_LWING, dirflag, *mask, speed);
                numpiece += SetDamageF16PieceType(&(piece[numpiece]), DAMAGEF16_LSTAB, dirflag, *mask, speed);
                numpiece += SetDamageF16PieceType(&(piece[numpiece]), DAMAGEF16_NOLEFT, dirflag, *mask, speed);
                *mask and_eq DAMAGEF16_NOLEFT;
            }
        }
        else if (damagetype bitand 0x2)
        {
            // damage to the right
            dirflag = DAMAGEF16_TORIGHT;

            if (damagetype bitand 0x4)   // damage to the nose
            {
                dirflag or_eq DAMAGEF16_TOFRONT;
                numpiece += SetDamageF16PieceType(&(piece[numpiece]), DAMAGEF16_RWING, dirflag, *mask, speed);
                numpiece += SetDamageF16PieceType(&(piece[numpiece]), DAMAGEF16_RSTAB, dirflag, *mask, speed);
                numpiece += SetDamageF16PieceType(&(piece[numpiece]), DAMAGEF16_NOSE, dirflag, *mask, speed);
                numpiece += SetDamageF16PieceType(&(piece[numpiece]), DAMAGEF16_NORIGHTANDNOSE, dirflag, *mask, speed);
                *mask and_eq DAMAGEF16_NORIGHTANDNOSE;
            }
            else if (damagetype bitand 0x8)   // damage to the front
            {
                dirflag or_eq DAMAGEF16_TOFRONT;
                numpiece += SetDamageF16PieceType(&(piece[numpiece]), DAMAGEF16_RWING, dirflag, *mask, speed);
                numpiece += SetDamageF16PieceType(&(piece[numpiece]), DAMAGEF16_RSTAB, dirflag, *mask, speed);
                numpiece += SetDamageF16PieceType(&(piece[numpiece]), DAMAGEF16_NOSE, dirflag, *mask, speed);
                numpiece += SetDamageF16PieceType(&(piece[numpiece]), DAMAGEF16_FRONT, dirflag, *mask, speed);
                numpiece += SetDamageF16PieceType(&(piece[numpiece]), DAMAGEF16_BACKWITHLEFT, dirflag, *mask, speed);
                *mask and_eq DAMAGEF16_BACKWITHLEFT;
            }
            else
            {
                numpiece += SetDamageF16PieceType(&(piece[numpiece]), DAMAGEF16_RWING, dirflag, *mask, speed);
                numpiece += SetDamageF16PieceType(&(piece[numpiece]), DAMAGEF16_RSTAB, dirflag, *mask, speed);
                numpiece += SetDamageF16PieceType(&(piece[numpiece]), DAMAGEF16_NORIGHT, dirflag, *mask, speed);
                *mask and_eq DAMAGEF16_NORIGHT;
            }
        }
        else if (damagetype bitand 0x4)
        {
            // damage to the nose
            dirflag = DAMAGEF16_TOFRONT;
            numpiece += SetDamageF16PieceType(&(piece[numpiece]), DAMAGEF16_NOSE, dirflag, *mask, speed);
            numpiece += SetDamageF16PieceType(&(piece[numpiece]), DAMAGEF16_NONOSE, dirflag, *mask, speed);
            *mask and_eq DAMAGEF16_NONOSE;
        }
        else if (damagetype bitand 0x8)
        {
            // damage to the front
            dirflag = DAMAGEF16_TOFRONT;
            numpiece += SetDamageF16PieceType(&(piece[numpiece]), DAMAGEF16_NOSE, dirflag, *mask, speed);
            numpiece += SetDamageF16PieceType(&(piece[numpiece]), DAMAGEF16_FRONT, dirflag, *mask, speed);
            numpiece += SetDamageF16PieceType(&(piece[numpiece]), DAMAGEF16_BACKWITHWING, dirflag, *mask, speed);
            *mask and_eq DAMAGEF16_BACKWITHWING;
        }
    }

    if ( not numpiece)   // if no damage, randomized damage parts
    {
        int i = DAMAGEF16_BACK;

        if (p > 0.0f)
        {
            if (PRANDFloatPos() > 0.5f)
            {
                dirflag or_eq DAMAGEF16_TOFRONT;
                i or_eq DAMAGEF16_FRONT;

                if (PRANDFloatPos() > 0.5f)
                {
                    i or_eq DAMAGEF16_NOSE;
                }
            }
        }

        if (r > 0.0f)
        {
            if (PRANDFloatPos() > 0.5f)
            {
                dirflag or_eq DAMAGEF16_TORIGHT;
                i or_eq DAMAGEF16_LWING bitor DAMAGEF16_LSTAB;
            }
            else i or_eq DAMAGEF16_RWING bitor DAMAGEF16_RSTAB;
        }
        else
        {
            if (PRANDFloatPos() > 0.5f)
            {
                dirflag or_eq DAMAGEF16_TOLEFT;
                i or_eq DAMAGEF16_RWING bitor DAMAGEF16_RSTAB;
            }
            else i or_eq DAMAGEF16_LWING bitor DAMAGEF16_LSTAB;
        }

        int j = i xor 0x7f;
        int k;
        int l = 1;

        for (k = 0; k < 7; k++)
        {
            if (l bitand j)
            {
                numpiece += SetDamageF16PieceType(&(piece[numpiece]), l, dirflag, *mask, speed);
            }

            l <<= 1;
        }

        numpiece += SetDamageF16PieceType(&(piece[numpiece]), i, dirflag, *mask, speed);
        *mask and_eq i;
    }

    return numpiece;
}

void AircraftClass::SetupDamageF16Effects(DamageF16PieceStructure *piece)
{
    Tpoint slot;
    Tpoint piececenter;
    piececenter.x = XPos();
    piececenter.y = YPos();
    piececenter.z = ZPos();

    SimBaseClass *tmpSimBase = new SimStaticClass(Type());//new SimBaseClass(Type());
    tmpSimBase -> SetYPR(Yaw(), 0.0f, 0.0f);
    CalcTransformMatrix(tmpSimBase);
    OTWDriver.CreateVisualObject(tmpSimBase, piece->damage, &piececenter, (Trotation *) &tmpSimBase->dmx, OTWDriver.Scale());

    DrawableBSP *ptr = (DrawableBSP *) tmpSimBase -> drawPointer;
    ptr -> SetSwitchMask(DAMAGEF16_SWITCH, piece->mask);

    if (piece -> mask bitand DAMAGEF16_BACK)
    {
        if (piece -> mask bitand DAMAGEF16_FRONT)
        {
            if ( not (piece -> mask bitand DAMAGEF16_NOSE))
            {
                ptr -> SetSwitchMask(DAMAGEF16_NOSEBREAK_SWITCH, 1);
            }

            // ptr -> SetSwitchMask(DAMAGEF16_CANOPYBREAK_SWITCH, 1);
        }
        else
        {
            ptr -> SetSwitchMask(DAMAGEF16_FRONTBREAK_SWITCH, 1);
        }

        if ( not (piece -> mask bitand DAMAGEF16_RWING))
        {
            ptr -> SetSwitchMask(DAMAGEF16_RWINGBREAK_SWITCH, 1);
        }

        if ( not (piece -> mask bitand DAMAGEF16_LWING))
        {
            ptr -> SetSwitchMask(DAMAGEF16_LWINGBREAK_SWITCH, 1);
        }
    }
    else if (piece -> mask bitand DAMAGEF16_FRONT)
    {
        if ( not (piece -> mask bitand DAMAGEF16_NOSE))
        {
            ptr -> SetSwitchMask(DAMAGEF16_NOSEBREAK_SWITCH, 1);
        }

        ptr -> SetSwitchMask(DAMAGEF16_FRONTBREAK_SWITCH, 1);
        // ptr -> SetSwitchMask(DAMAGEF16_CANOPYBREAK_SWITCH, 1);
    }
    else if (piece -> mask bitand DAMAGEF16_NOSE)
    {
        ptr -> SetSwitchMask(DAMAGEF16_NOSEBREAK_SWITCH, 1);
    }

    ptr -> GetChildOffset(piece->index, &slot);
    slot.x = -slot.x;
    slot.y = -slot.y;
    slot.z = -slot.z;

    tmpSimBase -> SetPosition(piececenter.x, piececenter.y, piececenter.z);
    tmpSimBase->SetDelta(piece->dx, piece->dy, piece->dz);
    tmpSimBase->SetYPRDelta(piece->yd, piece->pd, piece->rd);
    OTWDriver.AddSfxRequest(
        new SfxClass(
            piece->sfxtype, piece->sfxflag, tmpSimBase,
            piece->lifetime, OTWDriver.Scale(),
            &slot, piece -> pitch, piece -> roll
        )
    );
}

int AircraftClass::CreateDamageF16Effects()
{
    if ( not IsF16()) return 0;

    float groundZ = OTWDriver.GetApproxGroundLevel(XPos(), YPos());

    if (ZPos() - groundZ  < -500.0f) return 0; // not ground explosion

    DamageF16PieceStructure piece[7];
    int i = DAMAGEF16_ALL;
    int numpiece = CreateDamageF16Piece(piece, &i);

    if ( not numpiece) return 0;

    for (i = 0; i < numpiece; i++)
        SetupDamageF16Effects(&(piece[i]));

    return 1;
}
//////////////////////////////////////



void AircraftClass::RunExplosion(void)
{
    int i;
    Tpoint pos;
    SimBaseClass *tmpSimBase;
    Falcon4EntityClassType *classPtr;
    Tpoint tpo = Origin;
    Trotation tpim = IMatrix;

    //RV - I-Hawk - Added a 0 vector for RV new PS calls
    Tpoint PSvec;
    PSvec.x = 0;
    PSvec.y = 0;
    PSvec.z = 0;

    SoundPos.Sfx(SFX_BOOMA1 + PRANDInt5());


    ////////////////////
    if (this == SimDriver.GetPlayerEntity())
    {
        float az = (float)atan2(dmx[0][1], dmx[0][0]);
        OTWDriver.SetChaseAzEl(az, 0.0f);
    }

    if (CreateDamageF16Effects()) return;

    ////////////////////


    // 1st do primary explosion
    pos.x = XPos();
    pos.y = YPos();
    pos.z = ZPos();

    if (OnGround())
    {
        pos.z = OTWDriver.GetGroundLevel(pos.x, pos.y) - 4.0f;
        SetDelta(XDelta() * 0.1f, YDelta() * 0.1f, -50.0f);
        /*
        OTWDriver.AddSfxRequest(
         new SfxClass (SFX_GROUND_EXPLOSION, // type
         &pos, // world pos
         1.2f, // time to live
         100.0f ) ); // scale
         */

        DrawableParticleSys::PS_AddParticleEx(
            (SFX_GROUND_EXPLOSION + 1), &pos, &PSvec
        );

    }
    else
    {
        Tpoint vec = { XDelta(), YDelta(), ZDelta() }; //I-Hawk - Added vector for AC explosion effect

        /*
        OTWDriver.AddSfxRequest(
         new SfxClass (SFX_AC_AIR_EXPLOSION, // type
         &pos, // world pos
         2.0f, // time to live
         200.0f + 200 * PRANDFloatPos() ) ); // scale
         */

        DrawableParticleSys::PS_AddParticleEx(
            (SFX_AC_AIR_EXPLOSION + 1), &pos, &vec
        );
    }

    // Add the parts (appairently hardcoded at 4)
    // Recoded by KCK on 6/23 to remove damage station BS
    // KCK NOTE: Why are we creating SimBaseClass entities here?
    // Can't we just pass the stinking drawable object?
    // Ed seems to think not, if we want to keep the rotation deltas.
    for (i = 0; i < 4; i++)
    {
        tmpSimBase = new SimStaticClass(Type());//SimBaseClass(Type());
        classPtr = &Falcon4ClassTable[Type() - VU_LAST_ENTITY_TYPE];
        CalcTransformMatrix(tmpSimBase);
        OTWDriver.CreateVisualObject(tmpSimBase, classPtr->visType[i + 2], &tpo, &tpim, OTWDriver.Scale());
        tmpSimBase->SetPosition(pos.x, pos.y, pos.z);

        if ( not i)
        {
            tmpSimBase->SetDelta(XDelta(), YDelta(), ZDelta());
        }

        if ( not OnGround())
        {
            tmpSimBase->SetDelta(XDelta() + 50.0f * PRANDFloat(),
                                 YDelta() + 50.0f * PRANDFloat(),
                                 ZDelta() + 50.0f * PRANDFloat());
        }
        else
        {
            tmpSimBase->SetDelta(XDelta() + 50.0f * PRANDFloat(),
                                 YDelta() + 50.0f * PRANDFloat(),
                                 ZDelta() - 50.0f * PRANDFloatPos());
        }

        tmpSimBase->SetYPR(Yaw(), Pitch(), Roll());

        if ( not i)
        {
            // First peice is more steady and is flaming
            tmpSimBase->SetYPRDelta(0.0F, 0.0F, 10.0F + PRANDFloat() * 30.0F * DTR);
            /*
            OTWDriver.AddSfxRequest(
            new SfxClass (SFX_FLAMING_PART, // type
             SFX_MOVES bitor SFX_USES_GRAVITY bitor SFX_EXPLODE_WHEN_DONE,
             tmpSimBase, // sim base *
             3.0f + PRANDFloatPos() * 4.0F, // time to live
             1.0F ) ); // scale
             */
            pos.x = XPos();
            pos.y = YPos();
            pos.z = ZPos();

            DrawableParticleSys::PS_AddParticleEx((SFX_FLAMING_PART + 1), &pos, &PSvec);
        }
        else
        {
            // spin piece a random amount
            tmpSimBase->SetYPRDelta(PRANDFloat() * 30.0F * DTR,
                                    PRANDFloat() * 30.0F * DTR,
                                    PRANDFloat() * 30.0F * DTR);
            /*
            OTWDriver.AddSfxRequest(
             new SfxClass (SFX_SMOKING_PART, // type
             SFX_MOVES bitor SFX_USES_GRAVITY bitor SFX_BOUNCES bitor SFX_EXPLODE_WHEN_DONE,
             tmpSimBase, // sim base *
             4.0f * PRANDFloatPos() + (float)((i+1)*(i+1)), // time to live
             1.0 ) ); // scale
             */
            pos.x = XPos();
            pos.y = YPos();
            pos.z = ZPos();

            DrawableParticleSys::PS_AddParticleEx((SFX_SMOKING_PART + 1), &pos, &PSvec);
        }
    }
}

void AircraftClass::AddEngineTrails(int ttype, DWORD *tlist, DWORD *tlist_trail)
{
    // 2002-02-16 ADDED BY S.G. It's been seen that drawPointer is NULL here and &((DrawableBSP*)drawPointer)->orientation is simply drawPointer+0x2C hence why orientation is never NULL
    if ( not drawPointer)
        return;

    Tpoint pos, vec;

    Trotation *orientation = &((DrawableBSP*)drawPointer)->orientation;

    mlTrig noz;
    mlSinCos(&noz, af->nozzlePos);

    float thrx = -noz.cos ;
    float thry = 0;
    float thrz =  noz.sin ;

    vec.x = orientation->M11 * thrx + orientation->M12 * thry + orientation->M13 * thrz;
    vec.y = orientation->M21 * thrx + orientation->M22 * thry + orientation->M23 * thrz;
    vec.z = orientation->M31 * thrx + orientation->M32 * thry + orientation->M33 * thrz;

    ShiAssert(orientation);

    if ( not orientation)
        return;

    /*
    RV - I-Hawk

    If trails are contrails or CTRL-S trails we will unite
    trails emmiters locations based on engines locations.
    if 2 engines are close enough, no need for a trail for each engine but enough to have one
    trail for both...
    */

    int nTrails;
    int nEngines = min(MAXENGINES, af->auxaeroData->nEngines);
    int nContrails;
    Tpoint ContrailsLocation[MAXENGINES];

    //if this are the regular engine trails, don't bother getting in here...
    if (tlist == conTrails or tlist == colorConTrails)
    {
        switch (af->auxaeroData->nEngines)      //how many engines?
        {
            case 1:
                nContrails = 1;
                ContrailsLocation[0] = af->auxaeroData->engineLocation[0];
                break;

            case 2:
                if (fabs(af->auxaeroData->engineLocation[0].y - af->auxaeroData->engineLocation[1].y) < 9.0f)
                {
                    ContrailsLocation[0].x = af->auxaeroData->engineLocation[0].x;
                    ContrailsLocation[0].y = (af->auxaeroData->engineLocation[0].y + af->auxaeroData->engineLocation[1].y) / 2.0f ;
                    ContrailsLocation[0].z = af->auxaeroData->engineLocation[0].z;
                    nContrails = 1;
                }
                else
                {
                    nContrails = nEngines;

                    for (int i = 0; i < MAXENGINES; i++)
                        ContrailsLocation[i] = af->auxaeroData->engineLocation[i];
                }

                break;

                //
                //I-Hawk
                //in case of 3 engines I assume the second is set as the middle one...
                //3 engines A/C will always have one at the middle and the other two
                //on simetrical positions on the sides... so enough to check distance
                //from middle to one side
                //
            case 3:
                if (fabs(af->auxaeroData->engineLocation[1].y - af->auxaeroData->engineLocation[2].y) < 4.5f)
                {
                    if ((Get3DDistance(af->auxaeroData->engineLocation[1], af->auxaeroData->engineLocation[2])) < 4.5f)
                    {
                        nContrails = 1;
                        ContrailsLocation[0] = af->auxaeroData->engineLocation[1];
                    }
                }
                else
                {
                    nContrails = nEngines;

                    for (int i = 0; i < MAXENGINES; i++)
                        ContrailsLocation[i] = af->auxaeroData->engineLocation[i];
                }

                break;

                //I-Hawk
                //4 engine, assuming ordered in data from left to right...
                //
            case 4:
                nContrails = 0; //make sure we get this changed in one of the conditions below...

                if (fabs(af->auxaeroData->engineLocation[0].y - af->auxaeroData->engineLocation[1].y) < 9.0f)
                {
                    if ((Get3DDistance(af->auxaeroData->engineLocation[0], af->auxaeroData->engineLocation[1])) < 9.0f)
                    {
                        nContrails = 2;
                        ContrailsLocation[0] = Get3DMiddle(af->auxaeroData->engineLocation[0], af->auxaeroData->engineLocation[1]);
                        ContrailsLocation[1] = Get3DMiddle(af->auxaeroData->engineLocation[2], af->auxaeroData->engineLocation[3]);
                    }
                }

                if (fabs(af->auxaeroData->engineLocation[1].y - af->auxaeroData->engineLocation[2].y) < 9.0f)
                {
                    if ((Get3DDistance(af->auxaeroData->engineLocation[1], af->auxaeroData->engineLocation[2])) < 9.0f)
                    {
                        if ((Get3DDistance(af->auxaeroData->engineLocation[0], af->auxaeroData->engineLocation[3])) < 9.0f)
                        {
                            nContrails = 1;
                            ContrailsLocation[0] = Get3DMiddle(af->auxaeroData->engineLocation[1], af->auxaeroData->engineLocation[2]);
                        }
                        else
                        {
                            nContrails = 3;
                            ContrailsLocation[0] = af->auxaeroData->engineLocation[0];
                            ContrailsLocation[1] = Get3DMiddle(af->auxaeroData->engineLocation[1], af->auxaeroData->engineLocation[2]);
                            ContrailsLocation[2] = af->auxaeroData->engineLocation[3];
                        }
                    }
                }

                if ( not nContrails)    //engines are not close enough to unite, so keep a trail for each one
                {
                    nContrails = nEngines;

                    for (int i = 0; i < MAXENGINES; i++)
                        ContrailsLocation[i] = af->auxaeroData->engineLocation[i];
                }

                break;

            case 5:
            case 6:
            case 7:
            case 8:
            default:
                nContrails = nEngines;

                for (int i = 0; i < MAXENGINES; i++)
                    ContrailsLocation[i] = af->auxaeroData->engineLocation[i];

                break;
        }

        nTrails = nContrails;
    }
    else
        nTrails = nEngines;  // in case of regular engine smoke, we keep a trail for each engine

    //
    for (int i = 0; i < nTrails; i++)
    {
        //RV - I-Hawk - RV new trails call changes
        if (tlist[i] == NULL)
        {
            //tlist[i] = new DrawableTrail(ttype);
            //OTWDriver.InsertObject(tlist[i]);
            tlist[i] = ttype;
        }

        Tpoint *tp;

        if (tlist not_eq conTrails and tlist not_eq colorConTrails)
        {
            tp = &af->auxaeroData->engineLocation[i];
        }
        else
        {
            tp = &ContrailsLocation[i];
        }

        ShiAssert(tp)

        if ( not tp)
            return;

        float offset = -10.0f;

        pos.x = orientation->M11 * (tp->x + offset) + orientation->M12 * tp->y + orientation->M13 * tp->z;
        pos.y = orientation->M21 * (tp->x + offset) + orientation->M22 * tp->y + orientation->M23 * tp->z;
        pos.z = orientation->M31 * (tp->x + offset) + orientation->M32 * tp->y + orientation->M33 * tp->z;

#if 1// MLR_NEWTRAILCODE
        Tpoint smokeVel;
        smokeVel.x = vec.x * af->thrust * 2.5f ;
        smokeVel.y = vec.y * af->thrust * 2.5f ;
        smokeVel.z = vec.z * af->thrust * 2.5f ;
        //tlist[i]->SetHeadVelocity(&smokeVel);
#endif
        pos.x += XPos();
        pos.y += YPos();
        pos.z += ZPos();

        //OTWDriver.AddTrailHead (tlist[i], pos.x, pos.y, pos.z );
        //tlist[i]->KeepStaleSegs (flag_keep_smoke_trails);
        //
        //RV - I-Hawk - engine trails size/alpha change with altitude
        engineTrailMargin = 1;

        if (tlist not_eq conTrails)
        {
            if (tlist not_eq colorConTrails)
            {

                // if trail is engine trail, use 1K feet margin between 11K and 10K for trail fade-in/out...
                if (-ZPos() > 10000.0f)
                {
                    engineTrailMargin *= (11000.0f - (-ZPos())) / 1000.0f;
                }

                //RV - I-Hawk
                //Engine trails are fading in with RPM, at 80% the
                //engineTrailRPMFactor = 0.25, and slowly increasing based on RPM until it gets to
                //be 1 at 100% RPM. Always multiplying the difference by 3.75 and adding 0.25 because maximum difference
                //in RPM is: 1 - 0.8 = 0.2 -> 0.2 * 3.75 +0.25 = 1... so factor run between 0.25 to 1.
                //
                engineTrailRPMFactor = (PowerOutput() - 0.8f) * 3.75f + 0.25f;
                engineTrailAlpha = (((5000.0f - (-ZPos())) / 5000.0f) * 0.25f + 1) * engineTrailMargin * engineTrailRPMFactor;
                engineTrailSize = ((5000.0f - (-ZPos())) / 5000.0f) * 0.25f + 1;
            }
        }
        //trail is contrail... use 10% of ContrailLow value as margin for fade-in/out
        else
        {
            if (-ZPos() > (contrailLow90Percent) and -ZPos() < contrailLowValue)
            {
                engineTrailMargin *= (-ZPos() - contrailLow90Percent) / contrailLow10Percent;
            }

            if (-ZPos() > (contrailHighValue) and -ZPos() < contrailHigh110Percent)
            {
                engineTrailMargin *= (fabs(-ZPos() - contrailHigh110Percent)) / contrailLow10Percent;
            }

            engineTrailAlpha = engineTrailMargin; //Really means (1 * EngineTrailMargin) ...
            engineTrailSize = 1;
        }

        if (tlist == colorConTrails)
        {
            engineTrailAlpha = 1;
            engineTrailSize = 1;
        }

        tlist_trail[i] = DrawableParticleSys::PS_EmitTrail(tlist_trail[i], tlist[i], pos.x, pos.y, pos.z, engineTrailAlpha, engineTrailSize);
    }
}

void AircraftClass::CancelEngineTrails(DWORD *tlist, DWORD *tlist_trail)
{
    int nEngines = min(MAXENGINES, af->auxaeroData->nEngines);

    for (int i = 0; i < nEngines; i++)
    {
        if (tlist[i])
        {
            /* RV - I-Hawk - RV new trails call changes
            OTWDriver.AddSfxRequest(
            new SfxClass (
            21.2f, // time to live
            tlist[i]) ); // scale
            */
            DrawableParticleSys::PS_KillTrail(tlist_trail[i]);
            tlist[i] = tlist_trail[i] = NULL;
        }
    }
}

void AircraftClass::ShowDamage(void)
{
    BOOL hasMilSmoke = FALSE;
    float radius;
    Tpoint pos, rearOffset;

    //RV - I-Hawk - set Contrails margin band variables for appropraite contrails insertion checks later...
    if ( not contrailLowValue) // Make sure I assigned contrails weather values only once
    {
        contrailLowValue = ((WeatherClass*)realWeather)->contrailLow;
        contrailHighValue = ((WeatherClass*)realWeather)->contrailHigh;
        contrailLow10Percent = (((WeatherClass*)realWeather)->contrailLow * 0.1f);
        contrailLow90Percent = (((WeatherClass*)realWeather)->contrailLow * 0.9f);
        contrailHigh110Percent = (((WeatherClass*)realWeather)->contrailHigh + contrailLow10Percent);
    }

    //RV - I-Hawk - Set damage trails locations based on how many engines there are, can be set only once
    if ( not damageTrailLocationSet)
    {
        switch (af->auxaeroData->nEngines)
        {
                // 1 - damage point will be on that engine
            case 1:
                damageTrailLocation0 = damageTrailLocation1 = af->auxaeroData->engineLocation[0];
                break;

                // 2- damage location depends on space between the engines
            case 2:
                if (rand() bitand 1)
                {
                    damageTrailLocation0 = af->auxaeroData->engineLocation[0];
                    damageTrailLocation1 = af->auxaeroData->engineLocation[1];
                }
                else
                {
                    damageTrailLocation0 = af->auxaeroData->engineLocation[1];
                    damageTrailLocation1 = af->auxaeroData->engineLocation[0];
                }

                break;

                // 3- damage points will be selected randomly
            case 3:
                switch (PRANDInt3())
                {
                    case 0:
                        damageTrailLocation0 = af->auxaeroData->engineLocation[0];
                        damageTrailLocation1 = af->auxaeroData->engineLocation[1];
                        break;

                    case 1:
                        damageTrailLocation0 = af->auxaeroData->engineLocation[1];
                        damageTrailLocation1 = af->auxaeroData->engineLocation[2];

                    case 2:
                        damageTrailLocation0 = af->auxaeroData->engineLocation[2];
                        damageTrailLocation1 = af->auxaeroData->engineLocation[0];
                        break;
                }

                break;

                // 4+ engines - damage points will be selected randomly
            case 4:
            case 5:
            case 6:
            case 7:
            case 8:
                switch (PRANDInt5())
                {
                    case 0:
                        damageTrailLocation0 = af->auxaeroData->engineLocation[0];
                        damageTrailLocation1 = af->auxaeroData->engineLocation[2];
                        break;

                    case 1:
                        damageTrailLocation0 = af->auxaeroData->engineLocation[1];
                        damageTrailLocation1 = af->auxaeroData->engineLocation[3];

                    case 2:
                        damageTrailLocation0 = af->auxaeroData->engineLocation[2];
                        damageTrailLocation1 = af->auxaeroData->engineLocation[1];
                        break;

                    case 3:
                        damageTrailLocation0 = af->auxaeroData->engineLocation[0];
                        damageTrailLocation1 = af->auxaeroData->engineLocation[1];
                        break;

                    case 4:
                        damageTrailLocation0 = af->auxaeroData->engineLocation[3];
                        damageTrailLocation1 = af->auxaeroData->engineLocation[2];
                        break;
                }

                break;
        }

        damageTrailLocationSet = true; //do not try to set damage location more than once...
    }

    // handle case when moving slow and/or on ground -- no trails
    // MLR 12/14/2003 - The trail code will handle this
#if 1 // MLR_NEWTRAILCODE

    if (GetVt() < 40.0f)
    {
        for (int i = 0; i < TRAIL_MAX; i++)
        {
            if (smokeTrail[i])
            {
                /* RV - I-Hawk - RV new trails call changes
                OTWDriver.AddSfxRequest(
                new SfxClass (
                15.2f, // time to live
                smokeTrail[i]) ); // scale
                */
                DrawableParticleSys::PS_KillTrail(smokeTrail_trail[i]);
                smokeTrail[i] = smokeTrail_trail[i] = NULL;
            }
        }

        CancelEngineTrails(conTrails, conTrails_trail);
        CancelEngineTrails(engineTrails, engineTrails_trail);

        // no do puffy smoke if damaged
        if (pctStrength < 0.50f)
        {
            rearOffset.x = PRANDFloat() * 20.0f;
            rearOffset.y = PRANDFloat() * 20.0f;
            rearOffset.z = -PRANDFloatPos() * 20.0f;

            pos.x = XPos();
            pos.y = YPos();
            pos.z = ZPos();

            /*
            OTWDriver.AddSfxRequest(
             new SfxClass (SFX_TRAILSMOKE, // type
             SFX_MOVES bitor SFX_NO_GROUND_CHECK, // flags
             &pos, // world pos
             &rearOffset, // vector
             3.5f, // time to live
             4.5f )); // scale
             */
            DrawableParticleSys::PS_AddParticleEx((SFX_TRAILSMOKE + 1),
                                                  &pos,
                                                  &rearOffset);
        }

        return;
    }

#endif

    // mig29's, f4's and f5's all smoke when at MIL power
    // it's not damage, but since we handle the smoke here anyway.....

    if ( not OnGround() and 
        af->EngineTrail() >= 0)
    {
        //       if ( OTWDriver.renderer and OTWDriver.renderer->GetAlphaMode() )
        hasMilSmoke = af->EngineTrail();
    }

    // get rear offset behind the plane
    if (drawPointer)
        radius = drawPointer->Radius();
    else
        radius = 25.0f;

    rearOffset.x = -dmx[0][0] * radius;
    rearOffset.y = -dmx[0][1] * radius;
    rearOffset.z = -dmx[0][2] * radius;

    // check for contrail alt
    // also this can be toggled by the player
    //JAM 24Nov03
    //RV I-Hawk - contrails use 10% margin band for fading in/out
    if (-ZPos() > contrailLow90Percent and 
        -ZPos() < contrailHigh110Percent)
    {
        AddEngineTrails(TRAIL_CONTRAIL, conTrails, conTrails_trail);
    }
    else
    {
        CancelEngineTrails(conTrails, conTrails_trail);
    }

    //RV I-Hawk - Separated player smoke from contrails... but emmit trail only if not declared damaged
    if (playerSmokeOn and pctStrength > 0.50f)
    {
        //Pick different color based on UI munition screen setting
        AddEngineTrails(colorContrail, colorConTrails, colorConTrails_trail);
    }
    else
    {
        CancelEngineTrails(colorConTrails, colorConTrails_trail);
    }

    // JPO - maybe some wing tip vortexes
    if (af->auxaeroData->wingTipLocation.y not_eq 0   and 
        af->auxaeroData->wingTipLocation.y < 500 and 
        GetKias() > 80)
    {

        //RV - I-Hawk - define variables for trails generation conditions
        int dotrails = 0;
        int doturb = 0;
        int doGVortex = 0;
        int doAOAVortex = 0;

        //Read here from data
        float currentG = af->nzcgb;
        float currentAOA = af->alpha;
        float currentVCAS = af->vcas;
        bool vortexCondition = false;
        wingvortexAlphaLimit = vortexAOALimit = af->auxaeroData->vortexAOALimit;

        //check here if vortex trail (the first basic one) is defined
        if (af->auxaeroData->vortex1Location.y)
        {
            vortexCondition = true;
        }

        //RV - I-Hawk - do wingtip vortex trails based on G bitand AOA or based on AOA only if above certain value...
        if ((OTWDriver.renderer and 
             /*OTWDriver.renderer->GetAlphaMode() and */
             -ZPos() > minwingvortexalt and 
             -ZPos() < maxwingvortexalt) and 

            ((currentAOA > wingvortexAlpha and currentG > wingvortexgs) or
             (currentAOA > wingvortexAlphaHigh and currentAOA < wingvortexAlphaLimit and 
              currentVCAS > vortexMinSpeedLimit)))
        {
            dotrails = 1;
        }

        //RV - I-Hawk - adding new Vortex trails and PS instead of the 3D model switch
        if (OTWDriver.renderer and currentG > vortexG and vortexCondition and -ZPos() < vortexMaxaltLimit)
        {
            doGVortex = 1;
        }

        if (OTWDriver.renderer and currentAOA > vortexAOA and 
            currentAOA < vortexAOALimit and 
            vortexCondition and 
            -ZPos() < vortexMaxaltLimit)
        {
            doAOAVortex = 1;
        }

        if (g_bWakeTurbulence)
        {
            if (lVortex and rVortex)
            {
                //if(GetKias() > 40)
                if (af->IsSet(AirframeClass::InAir))
                    // MLR - IMHO this isn't correct, a wing can generate lift with the a/c still on the ground.
                    //       even though the lift may not be enough to allow for lift off.
                {
                    doturb = 1;
                }
                else
                {
                    lVortex->BreakRecord();
                    rVortex->BreakRecord();
                }
            }
        }


        if (dotrails or  doturb)
        {
            Trotation *orientation = 0L;

            // only do this if we have to
            if ((DrawableBSP*)drawPointer)
                orientation = &((DrawableBSP*)drawPointer)->orientation;

            if (orientation)
            {
                Tpoint tp = af->auxaeroData->wingTipLocation;
                Tpoint pos, ltip, rtip;

                pos.x = XPos();
                pos.y = YPos();
                pos.z = ZPos();

                if (af->auxaeroData->hasSwingWing)
                {
                    mlTrig trig;
                    mlSinCos(&trig, swingWingAngle);

                    float x, y;
                    x = tp.x - af->auxaeroData->swingWingHinge.x;
                    y = tp.y - af->auxaeroData->swingWingHinge.y;
                    tp.x = x * cos(swingWingAngle) - y * sin(swingWingAngle) + af->auxaeroData->swingWingHinge.x;
                    tp.y = x * sin(swingWingAngle) + y * cos(swingWingAngle) + af->auxaeroData->swingWingHinge.y;
                }

                AssignACOrientation(orientation, tp, ltip, pos, false);
                AssignACOrientation(orientation, tp, rtip, pos, true);

                if (dotrails)
                {
                    //RV - I-Hawk - RV new trails call changes
                    if (lwingvortex == NULL)
                    {
                        //lwingvortex = new DrawableTrail(TRAIL_WINGTIPVTX);
                        lwingvortex = TRAIL_WINGTIPVTX;
                        //OTWDriver.InsertObject(lwingvortex);
                    }

                    //OTWDriver.AddTrailHead (lwingvortex, ltip.x, ltip.y, ltip.z );

                    //RV - I-Hawk - Added alpha/size change with altitude, speed and G for wingtip vortex
                    wingvtxTrailAlphaCx = wingvtxTrailSizeCx = 1;

                    if (currentG > 3.5)
                    {
                        wingvtxTrailAlphaCx += (currentG - 5) * 0.1f;
                    }

                    if (currentVCAS < 250 and currentVCAS > vortexMinSpeedLimit)
                    {
                        wingvtxTrailAlphaCx *= (currentVCAS - vortexMinSpeedLimit) / (250.0f - vortexMinSpeedLimit);
                    }

                    float wingtipTrailAltCx = 1.0f - (-ZPos() / maxwingvortexalt);
                    wingvtxTrailAlphaCx *= wingtipTrailAltCx;
                    wingvtxTrailSizeCx *=  wingtipTrailAltCx;

                    //Never let the sizeCx get below 0.6 as it would cause the trail to start
                    //with a puff smaller than 0.5, may cause freezes.
                    if (wingvtxTrailSizeCx < 0.6f)
                    {
                        wingvtxTrailSizeCx = 0.6f;
                    }

                    //Never let the alphaCx get below 0
                    if (wingvtxTrailAlphaCx < 0)
                    {
                        wingvtxTrailAlphaCx = 0;
                    }

                    lwingvortex_trail = DrawableParticleSys::PS_EmitTrail(lwingvortex_trail, lwingvortex, ltip.x, ltip.y, ltip.z, wingvtxTrailAlphaCx, wingvtxTrailSizeCx);

                    if (rwingvortex == NULL)
                    {
                        //rwingvortex = new DrawableTrail(TRAIL_WINGTIPVTX);
                        rwingvortex = TRAIL_WINGTIPVTX;
                        //OTWDriver.InsertObject(rwingvortex);
                    }

                    //OTWDriver.AddTrailHead (rwingvortex, rtip.x, rtip.y, rtip.z );

                    rwingvortex_trail = DrawableParticleSys::PS_EmitTrail(rwingvortex_trail, rwingvortex, rtip.x, rtip.y, rtip.z, wingvtxTrailAlphaCx, wingvtxTrailSizeCx);
                }


                if (doturb)
                {
                    float acweight = af->weight * 0.00001f;
                    float acaoa = currentAOA * 0.1f;
                    float strength = acweight * acaoa;

                    lVortex->RecordPosition(strength, ltip.x, ltip.y, ltip.z);
                    rVortex->RecordPosition(strength, rtip.x, rtip.y, rtip.z);
                }

#if NEW_VORTEX_TRAILS

                //RV - I-Hawk - Vortex stuff starting here
                if (doGVortex or doAOAVortex)
                {
                    //Get the vortex locations from data. vortexAOALimit was already taken
                    //since it was used above with the wingtip trails

                    Tpoint vtx1 = af->auxaeroData->vortex1Location;
                    Tpoint vtx2 = af->auxaeroData->vortex2Location;
                    Tpoint vtx3 = af->auxaeroData->vortexPS1Location;
                    Tpoint vtx4 = af->auxaeroData->vortexPS2Location;
                    Tpoint vtx5 = af->auxaeroData->vortexPS3Location;
                    int largeVortex = af->auxaeroData->largeVortex;

                    Tpoint pos, PSvec;
                    Tpoint lvtx1, rvtx1;
                    int theSFX;
                    float Xoffset, Zoffset, currentMach = af->mach;

                    //Get world position of AC
                    pos.x = XPos();
                    pos.y = YPos();
                    pos.z = ZPos();

                    //Get the AC heading ( the Delta() direction )
                    PSvec.x = XDelta();
                    PSvec.y = YDelta();
                    PSvec.z = ZDelta();

                    AssignACOrientation(orientation, vtx1, lvtx1, pos, false);
                    AssignACOrientation(orientation, vtx1, rvtx1, pos, true);

                    vortex1AlphaCx = vortex1SizeCx = 1;

                    //normalize the alpha and size Cxs by G
                    if (doGVortex)
                    {
                        vortex1AlphaCx += (currentG - 8.5f) * 0.2f;
                        vortex1SizeCx += (currentG - 7.5f) * 0.1f;
                    }

                    //normalize by AOA
                    //If both G and AOA conditions are filled, use only G values to calculate Cxs
                    if (doAOAVortex and not doGVortex)
                    {
                        vortex1AlphaCx += (currentAOA - vortexAOALimit) * (1.0f / vortexAOALimit);
                        vortex1SizeCx += (currentAOA - vortexAOA) * 0.03f;
                    }

                    //normalize by altitude, only the alpha Cx, not size Cx
                    float trailAltCx = 1.0f - (-ZPos() / vortexMaxaltLimit);
                    vortex1AlphaCx *= trailAltCx;

                    //make sure we never get the Cxs below 0
                    if (vortex1AlphaCx < 0 or vortex1SizeCx < 0)
                    {
                        vortex1AlphaCx = vortex1SizeCx = 0;
                    }

                    //Use the larger trails/PS when specified in .dat file (for larger ACs)
                    //largeVortex is a bitwise used variable. Deciding size/types of the trails/PS
                    //according to the appropriate bits as mentioned below

                    //bit 0 is to decide for the 1st vortex trail
                    //bit 1 is to decide for the 2nd vortex trail
                    //bits 2,3,4 are to decide for the 3 PS generators

                    if (lvortex1 == NULL or rvortex1 == NULL)
                    {
                        if (largeVortex bitand 1)   //check if bit 0 of largeVortex is set
                        {
                            lvortex1 = rvortex1 = TRAIL_VORTEX_LARGE;
                        }
                        else
                        {
                            lvortex1 = rvortex1 = TRAIL_VORTEX;
                        }
                    }

                    lvortex1_trail = DrawableParticleSys::PS_EmitTrail(lvortex1_trail, lvortex1, lvtx1.x, lvtx1.y, lvtx1.z, vortex1AlphaCx, vortex1SizeCx);
                    rvortex1_trail = DrawableParticleSys::PS_EmitTrail(rvortex1_trail, rvortex1, rvtx1.x, rvtx1.y, rvtx1.z, vortex1AlphaCx, vortex1SizeCx);

                    //if AC has a second trail location defined

                    //using bits 5,6,7 of largeVortex to decide if this position will be used
                    //as a trail or as PS. This leavs option for some ACs to have
                    //wide vortex over the entire body and wings.
                    if (vtx2.y and not (largeVortex bitand 32) and not (largeVortex bitand 64) and not (largeVortex bitand 128))
                    {
                        Tpoint lvtx2, rvtx2;

                        AssignACOrientation(orientation, vtx2, lvtx2, pos, false);
                        AssignACOrientation(orientation, vtx2, rvtx2, pos, true);

                        //Use same alpha Cx for the second trail as the first trail
                        vortex2AlphaCx = vortex1AlphaCx;
                        vortex2SizeCx = vortex1SizeCx;

                        if (lvortex2 == NULL or rvortex2 == NULL)
                        {
                            if (largeVortex bitand 2)   //check if bit 1 of largeVortex is set
                            {
                                lvortex2 = rvortex2 = TRAIL_VORTEX_LARGE;
                            }
                            else
                            {
                                lvortex2 = rvortex2 = TRAIL_VORTEX;
                            }
                        }

                        lvortex2_trail = DrawableParticleSys::PS_EmitTrail(lvortex2_trail, lvortex2, lvtx2.x, lvtx2.y, lvtx2.z, vortex2AlphaCx, vortex2SizeCx);
                        rvortex2_trail = DrawableParticleSys::PS_EmitTrail(rvortex2_trail, rvortex2, rvtx2.x, rvtx2.y, rvtx2.z, vortex2AlphaCx, vortex2SizeCx);
                    }

                    //are we doing the PS vortex?
                    if (((currentG > 7.0f) or (currentAOA > ((vortexAOALimit + vortexAOA) / 2.0f))) and 
                        (vtx3.y or vtx4.y or vtx5.y))
                    {

                        //local position variables for the PS vortex effects
                        Tpoint lvtxPS1, rvtxPS1, lvtxPS2, rvtxPS2, lvtxPS3, rvtxPS3;

                        //determine if the PS is used and how (which type of PS, strong, weak etc)
                        int enablePS1, enablePS2, enablePS3;
                        enablePS1 = enablePS2 = enablePS3 = 0;

                        //fix Z positioning 1 feet up, as it's always when pulling up,
                        //and we are using the ZDelta() for the PS vector
                        vtx3.z -= 1.0f;
                        vtx4.z -= 1.0f;
                        vtx5.z -= 1.0f;

                        // if we are using the larger vortex PS, give it another feet up correction
                        if (largeVortex bitand 4)
                        {
                            vtx3.z -= 1.0f;
                        }

                        if (largeVortex bitand 8)
                        {
                            vtx4.z -= 1.0f;
                        }

                        if (largeVortex bitand 16)
                        {
                            vtx5.z -= 1.0f;
                        }

                        Xoffset = 0;

                        //fix X positioning as using the XDelta() for vector, so if AC is too fast
                        //need to reposition the PS origin
                        if (currentMach > 0.9)
                        {
                            Xoffset = (currentMach - 0.9f) * -7.0f;
                            vtx3.x += Xoffset;
                            vtx4.x += Xoffset;
                            vtx5.x += Xoffset;
                        }

                        //fix Z positioning when pulling high G, to make the vortex seen below the
                        //wing as less as possible
                        if (currentG > 7.0f)
                        {
                            if (largeVortex bitand 4)
                            {
                                Zoffset = (currentG - 7.0f) * 1.0f;
                            }

                            else
                            {
                                Zoffset = (currentG - 7.0f) * - 0.75f;
                            }

                            vtx3.z += Zoffset;
                            vtx4.z += Zoffset;
                            vtx5.z += Zoffset;

                            // check if bits 5,6 or 7 are set, means that vortex2 is used as a PS
                            // so fix it as well...
                            if (largeVortex bitand 224)
                            {
                                vtx2.z += Zoffset;
                            }
                        }

                        //Start to determine a Cx for the PS effects, I'm using this Cx
                        //to decide when the PS will be rendered and which type for each position

                        float PSCx = 0;

                        //Get a G or AOA factor
                        if (doGVortex)
                            PSCx += currentG / 9.0f;

                        if (doAOAVortex and not doGVortex)
                            PSCx += currentAOA / vortexAOALimit;

                        //Get altitude factor as vortex should get thinner with altitude
                        float PSaltCx = 1.0f - (-ZPos() / vortexMaxaltLimit);

                        //Multiply the Cx with the altitude Factor
                        PSCx *= PSaltCx;

                        //Now I decide how to set the enablePSx (x=1,2,3) according to the PSCx
                        //enablePSx = 0 means No PS
                        //enablePSx = 1 means weak PS
                        //enablePSx = 2 means medium PS
                        //enablePSx = 3 means strong PS
                        //
                        //size will be decide according to the vortexLarge bits

                        if (PSCx > 0.8f)
                        {
                            enablePS1 = 3;
                            enablePS2 = 3;
                            enablePS3 = 2;
                        }

                        if (PSCx <= 0.8f and PSCx > 0.6f)
                        {
                            enablePS1 = 3;
                            enablePS2 = 2;
                            enablePS3 = 2;
                        }

                        if (PSCx <= 0.6f and PSCx > 0.4f)
                        {
                            enablePS1 = 2;
                            enablePS2 = 2;
                            enablePS3 = 1;
                        }

                        if (PSCx <= 0.4f and PSCx > 0.2f)
                        {
                            enablePS1 = 2;
                            enablePS2 = 1;
                            enablePS3 = 1;
                        }

                        if (PSCx < 0.2f)
                        {
                            enablePS1 = 1;
                            enablePS2 = 0;
                            enablePS3 = 0;
                        }

                        //Execute the PS

                        if (enablePS1 and vtx3.y)
                        {

                            AssignACOrientation(orientation, vtx3, lvtxPS1, pos, false);
                            AssignACOrientation(orientation, vtx3, rvtxPS1, pos, true);

                            switch (enablePS1)
                            {
                                case 1:
                                    theSFX = SFX_VORTEX_WEAK;

                                    if (largeVortex bitand 4)   //check if bit 2 of largeVortex is set
                                    {
                                        theSFX = SFX_VORTEX_LARGE_WEAK;
                                    }

                                    break;

                                case 2:
                                    theSFX = SFX_VORTEX_MEDIUM;

                                    if (largeVortex bitand 4)
                                        theSFX = SFX_VORTEX_LARGE_MEDIUM;

                                    break;

                                case 3:
                                    theSFX = SFX_VORTEX_STRONG;

                                    if (largeVortex bitand 4)
                                        theSFX = SFX_VORTEX_LARGE_STRONG;

                                    break;

                                default:
                                    theSFX = SFX_VORTEX_MEDIUM;
                                    break;
                            }

                            DrawableParticleSys::PS_AddParticleEx(
                                (theSFX + 1), &rvtxPS1, &PSvec);

                            DrawableParticleSys::PS_AddParticleEx(
                                (theSFX + 1), &lvtxPS1, &PSvec);

                            //the second trail position is used as a PS, so add a PS1 type there...
                            if (vtx2.y and largeVortex bitand 32)
                            {
                                Tpoint lvtx2, rvtx2;

                                AssignACOrientation(orientation, vtx2, lvtx2, pos, false);
                                AssignACOrientation(orientation, vtx2, rvtx2, pos, true);

                                DrawableParticleSys::PS_AddParticleEx(
                                    (theSFX + 1), &rvtx2, &PSvec);

                                DrawableParticleSys::PS_AddParticleEx(
                                    (theSFX + 1), &lvtx2, &PSvec);
                            }
                        }

                        if (enablePS2 and vtx4.y)
                        {

                            AssignACOrientation(orientation, vtx4, lvtxPS2, pos, false);
                            AssignACOrientation(orientation, vtx4, rvtxPS2, pos, true);

                            switch (enablePS2)
                            {
                                case 1:
                                    theSFX = SFX_VORTEX_WEAK;

                                    if (largeVortex bitand 8)   //check if bit 3 of largeVortex is set
                                        theSFX = SFX_VORTEX_LARGE_WEAK;

                                    break;

                                case 2:
                                    theSFX = SFX_VORTEX_MEDIUM;

                                    if (largeVortex bitand 8)
                                        theSFX = SFX_VORTEX_LARGE_MEDIUM;

                                    break;

                                case 3:
                                    theSFX = SFX_VORTEX_STRONG;

                                    if (largeVortex bitand 8)
                                        theSFX = SFX_VORTEX_LARGE_STRONG;

                                    break;

                                default:
                                    theSFX = SFX_VORTEX_MEDIUM;
                                    break;
                            }

                            DrawableParticleSys::PS_AddParticleEx(
                                (theSFX + 1), &rvtxPS2, &PSvec);

                            DrawableParticleSys::PS_AddParticleEx(
                                (theSFX + 1), &lvtxPS2, &PSvec);

                            //the second trail position is used as a PS, so add a PS2 type there...
                            if (vtx2.y and largeVortex bitand 64)
                            {
                                Tpoint lvtx2, rvtx2;

                                AssignACOrientation(orientation, vtx2, lvtx2, pos, false);
                                AssignACOrientation(orientation, vtx2, rvtx2, pos, true);

                                DrawableParticleSys::PS_AddParticleEx(
                                    (theSFX + 1), &rvtx2, &PSvec);

                                DrawableParticleSys::PS_AddParticleEx(
                                    (theSFX + 1), &lvtx2, &PSvec);
                            }
                        }

                        if (enablePS3 and vtx5.y)
                        {

                            AssignACOrientation(orientation, vtx5, lvtxPS3, pos, false);
                            AssignACOrientation(orientation, vtx5, rvtxPS3, pos, true);

                            switch (enablePS3)
                            {
                                case 1:
                                    theSFX = SFX_VORTEX_WEAK;

                                    if (largeVortex bitand 16)   //check if bit 4 of largeVortex is set
                                        theSFX = SFX_VORTEX_LARGE_WEAK;

                                    break;

                                case 2:
                                    theSFX = SFX_VORTEX_MEDIUM;

                                    if (largeVortex bitand 16)
                                        theSFX = SFX_VORTEX_LARGE_MEDIUM;

                                    break;

                                case 3:
                                    theSFX = SFX_VORTEX_STRONG;

                                    if (largeVortex bitand 16)
                                        theSFX = SFX_VORTEX_LARGE_STRONG;

                                    break;

                                default:
                                    theSFX = SFX_VORTEX_MEDIUM;
                                    break;
                            }

                            DrawableParticleSys::PS_AddParticleEx(
                                (theSFX + 1), &rvtxPS3, &PSvec);

                            DrawableParticleSys::PS_AddParticleEx(
                                (theSFX + 1), &lvtxPS3, &PSvec);

                            //the second trail position is used as a PS, so add a PS3 type there...
                            if (vtx2.y and largeVortex bitand 64)
                            {
                                Tpoint lvtx2, rvtx2;

                                AssignACOrientation(orientation, vtx2, lvtx2, pos, false);
                                AssignACOrientation(orientation, vtx2, rvtx2, pos, true);

                                DrawableParticleSys::PS_AddParticleEx(
                                    (theSFX + 1), &rvtx2, &PSvec);

                                DrawableParticleSys::PS_AddParticleEx(
                                    (theSFX + 1), &lvtx2, &PSvec);
                            }
                        }
                    }
                }
                else //Kill the trails, PS needs no killing
                {
                    if (lvortex1)
                    {
                        DrawableParticleSys::PS_KillTrail(lvortex1_trail);
                        lvortex1 = lvortex1_trail = NULL;
                    }

                    if (rvortex1)
                    {
                        DrawableParticleSys::PS_KillTrail(rvortex1_trail);
                        rvortex1 = rvortex1_trail = NULL;
                    }

                    if (lvortex2)
                    {
                        DrawableParticleSys::PS_KillTrail(lvortex2_trail);
                        lvortex2 = lvortex2_trail = NULL;
                    }

                    if (rvortex2)
                    {
                        DrawableParticleSys::PS_KillTrail(rvortex2_trail);
                        rvortex2 = rvortex2_trail = NULL;
                    }
                }

#endif
            }
        }

#if 0  // MLR 10/3/2004 - This is handled in ::Sleep() now.
        {
            // make the wingtip vortices (smoke trails) independent of the a/c
            if (lwingvortex)
            {

                /* RV - I-Hawk - RV new trails call changes
                   OTWDriver.AddSfxRequest(
                    new SfxClass (
                    21.2f, // time to live
                    lwingvortex) ); // scale*/

                DrawableParticleSys::PS_KillTrail(lwingvortex_trail);
                lwingvortex = lwingvortex_trail = NULL;

            }

            if (rwingvortex)
            {
                /*
                    OTWDriver.AddSfxRequest(
                    new SfxClass (
                    21.2f, // time to live
                    rwingvortex) ); // scale
                   */
                DrawableParticleSys::PS_KillTrail(rwingvortex_trail);
                rwingvortex = rwingvortex_trail = NULL;
            }
        }
#endif
    }

    // do military power smoke if its that type of craft
    // PowerOutput() runs from 0.7 (flight idle) to 1.5 (max ab) with 1.0 being mil power
    // M.N. added Poweroutput > 0.65, stops smoke trails when engine is shut down.
    //if ( not OnGround() and af->EngineTrail() >= 0 and OTWDriver.renderer /* and OTWDriver.renderer->GetAlphaMode()*/) {
    /* if (PowerOutput() <= 1.0f and PowerOutput() > 0.65f)
     {
     AddEngineTrails(af->EngineTrail(), engineTrails); // smoke
     }
     else CancelEngineTrails(engineTrails);
    }*/
    //Cobra TJL 11/12/04 Allow engine smoke on ground and make it between 80% to 100% power
    //Cobra limit smoke to under 10000 feet
    //RV I-Hawk - allow engine smoke till 11000, 1K feet as a fade in/out margin band...
    if (af->EngineTrail() >= 0 and OTWDriver.renderer)
    {
        if (PowerOutput() <= 1.0f and PowerOutput() > 0.80f and -ZPos() < 11000.0f)
        {
            AddEngineTrails(af->EngineTrail(), engineTrails, engineTrails_trail); // smoke
        }
        else
        {
            CancelEngineTrails(engineTrails, engineTrails_trail);
        }
    }

    if (pctStrength > 0.50f)
    {
        if ( not hasMilSmoke and smokeTrail[TRAIL_DAMAGE] /* and smokeTrail[TRAIL_DAMAGE]->InDisplayList(*/)
        {
            /* RV - I-Hawk - RV new trails call changes
            OTWDriver.AddSfxRequest(
            new SfxClass (
            11.2f, // time to live
            smokeTrail[TRAIL_DAMAGE]) ); // scale
            */
            DrawableParticleSys::PS_KillTrail(smokeTrail_trail[TRAIL_DAMAGE]);
            smokeTrail[TRAIL_DAMAGE] = smokeTrail_trail[TRAIL_DAMAGE] = NULL;
        }

        return;
    }

    // if we're dying, just maitain the status quo...
    if (pctStrength < 0.0f)
    {
        radius = 3.0f;

        //RV - I-Hawk - RV new trails call changes
        if (smokeTrail[TRAIL_DAMAGE])
        {
            Trotation *orientation = &((DrawableBSP*)drawPointer)->orientation;

            if (orientation)
            {
                Tpoint tp = damageTrailLocation0;
                float offset = 2.0f;

                //If it's the bigger trail, set larger offset.
                if (smokeTrail[TRAIL_DAMAGE] == TRAIL_BURNING_SMOKE2)
                    offset = 5.0f;

                pos.x = orientation->M11 * (tp.x + offset) + orientation->M12 * tp.y + orientation->M13 * tp.z + XPos();
                pos.y = orientation->M21 * (tp.x + offset) + orientation->M22 * tp.y + orientation->M23 * tp.z + YPos();
                pos.z = orientation->M31 * (tp.x + offset) + orientation->M32 * tp.y + orientation->M33 * tp.z + ZPos();

                //OTWDriver.AddTrailHead (smokeTrail[TRAIL_DAMAGE], pos.x, pos.y, pos.z );

                //RV - I-Hawk added check so damage trail alpha/size change with altitude
                // but never goes too low...
                if (-ZPos() < 25000.0f)
                {
                    damageTrailAlphaCx = ((15000.0f - (-ZPos())) / 15000.0f) * 0.25f + 1;
                    damageTrailSizeCx = ((15000.0f - (-ZPos())) / 15000.0f) * 0.15f + 1;
                }
                else
                {
                    damageTrailAlphaCx = ((15000.0f - (25000.0f)) / 15000.0f) * 0.25f + 1;
                    damageTrailSizeCx = ((15000.0f - (25000.0f)) / 15000.0f) * 0.15f + 1;
                }

                smokeTrail_trail[TRAIL_DAMAGE] = DrawableParticleSys::PS_EmitTrail(smokeTrail_trail[TRAIL_DAMAGE], smokeTrail[TRAIL_DAMAGE], pos.x, pos.y, pos.z, damageTrailAlphaCx, damageTrailSizeCx);
            }
        }

        //RV I-Hawk - Removing this as no need for additional trails when damaged...
        if (smokeTrail[TRAIL_ENGINE1])
        {
            Trotation *orientation = &((DrawableBSP*)drawPointer)->orientation;

            if (orientation)
            {
                Tpoint tp = damageTrailLocation1;
                float offset = 5.0f;

                pos.x = orientation->M11 * (tp.x + offset) + orientation->M12 * tp.y + orientation->M13 * tp.z + XPos();
                pos.y = orientation->M21 * (tp.x + offset) + orientation->M22 * tp.y + orientation->M23 * tp.z + YPos();
                pos.z = orientation->M31 * (tp.x + offset) + orientation->M32 * tp.y + orientation->M33 * tp.z + ZPos();

                //OTWDriver.AddTrailHead (smokeTrail[TRAIL_ENGINE1], pos.x, pos.y, pos.z );

                //RV - I-Hawk - damage trails size/alpha change with altitude
                if (-ZPos() < 25000.0f)
                {
                    damageTrailAlphaCx = ((15000.0f - (-ZPos())) / 15000.0f) * 0.25f + 1;
                    damageTrailSizeCx = ((15000.0f - (-ZPos())) / 15000.0f) * 0.15f + 1;
                }
                else
                {
                    damageTrailAlphaCx = ((15000.0f - (25000.0f)) / 15000.0f) * 0.25f + 1;
                    damageTrailSizeCx = ((15000.0f - (25000.0f)) / 15000.0f) * 0.15f + 1;
                }

                smokeTrail_trail[TRAIL_ENGINE1] = DrawableParticleSys::PS_EmitTrail(
                                                      smokeTrail_trail[TRAIL_ENGINE1],
                                                      smokeTrail[TRAIL_ENGINE1], pos.x, pos.y, pos.z, damageTrailAlphaCx, damageTrailSizeCx);
            }
        }

        return;
    }

    // at this point we know we've got enough damage for 1 trail in
    // the center

    //RV - I-Hawk - RV new trails call changes
    if ( not smokeTrail[TRAIL_DAMAGE])
    {

        //smokeTrail[TRAIL_DAMAGE] = new DrawableTrail(TRAIL_SMOKE); // smoke
        //OTWDriver.InsertObject(smokeTrail[TRAIL_DAMAGE]);
        //
        //Set smaller/larger damage trail based on the number of engines...
        // the large planes 3/4 engines and distanced 2 engines, will have larger trails...

        //I-Hawk - Set damage trails size based on the AC radius
        float radius = drawPointer->Radius();

        if (radius > 50.0f)
        {
            smokeTrail[TRAIL_DAMAGE] = TRAIL_BURNING_SMOKE2;
        }

        else
        {
            smokeTrail[TRAIL_DAMAGE] = TRAIL_BURNING_SMOKE;
        }
    }

    //RV I-Hawk - Get orientation of location
    Trotation *orientation = &((DrawableBSP*)drawPointer)->orientation;

    if (orientation)
    {
        Tpoint tp = damageTrailLocation0;
        float offset = 2.0f;

        //If it's the bigger trail, set larger offset.
        if (smokeTrail[TRAIL_DAMAGE] == TRAIL_BURNING_SMOKE2)
        {
            offset = 5.0f;
        }

        pos.x = orientation->M11 * (tp.x + offset) + orientation->M12 * tp.y + orientation->M13 * tp.z + XPos();
        pos.y = orientation->M21 * (tp.x + offset) + orientation->M22 * tp.y + orientation->M23 * tp.z + YPos();
        pos.z = orientation->M31 * (tp.x + offset) + orientation->M32 * tp.y + orientation->M33 * tp.z + ZPos();

        //OTWDriver.AddTrailHead (smokeTrail[TRAIL_DAMAGE], pos.x, pos.y, pos.z );
        //
        //RV - I-Hawk added check so damage trail alpha/size change with altitude
        // but never goes too low...

        if (-ZPos() < 25000.0f)
        {
            damageTrailAlphaCx = ((15000.0f - (-ZPos())) / 15000.0f) * 0.25f + 1;
            damageTrailSizeCx = ((15000.0f - (-ZPos())) / 15000.0f) * 0.15f + 1;
        }
        else
        {
            damageTrailAlphaCx = ((15000.0f - (25000.0f)) / 15000.0f) * 0.25f + 1;
            damageTrailSizeCx = ((15000.0f - (25000.0f)) / 15000.0f) * 0.15f + 1;
        }
    }

    smokeTrail_trail[TRAIL_DAMAGE] = DrawableParticleSys::PS_EmitTrail(
                                         smokeTrail_trail[TRAIL_DAMAGE], smokeTrail[TRAIL_DAMAGE],
                                         pos.x, pos.y, pos.z, damageTrailAlphaCx, damageTrailSizeCx);

    // now test for additional damage and add smoke trails on the left and right
    if (pctStrength < 0.25f)
    {

        //RV I-Hawk - Get orientation of location
        Trotation *orientation = &((DrawableBSP*)drawPointer)->orientation;

        if (orientation)
        {
            if (af->auxaeroData->nEngines >= 3 or (af->auxaeroData->nEngines == 2
                                                  and (fabs(af->auxaeroData->engineLocation[0].y - af->auxaeroData->engineLocation[1].y) > 15.0f))
               )
            {
                radius = 3.0f;

                Tpoint tp = damageTrailLocation1;

                float offset = 2.0f;

                //If it's the bigger trail, set larger offset.
                if (smokeTrail[TRAIL_DAMAGE] == TRAIL_BURNING_SMOKE2)
                    offset = 5.0f;

                pos.x = orientation->M11 * (tp.x + offset) + orientation->M12 * tp.y + orientation->M13 * tp.z + XPos();
                pos.y = orientation->M21 * (tp.x + offset) + orientation->M22 * tp.y + orientation->M23 * tp.z + YPos();
                pos.z = orientation->M31 * (tp.x + offset) + orientation->M32 * tp.y + orientation->M33 * tp.z + ZPos();

                //RV - I-Hawk - RV new trails call changes
                if ( not smokeTrail[TRAIL_ENGINE1])
                {

                    //smokeTrail[TRAIL_ENGINE1] = new DrawableTrail(TRAIL_SMOKE); // smoke
                    //OTWDriver.InsertObject(smokeTrail[TRAIL_ENGINE1]);

                    //If we are here, it's a big bird, so use bigger dmage trail
                    smokeTrail[TRAIL_ENGINE1] = TRAIL_BURNING_SMOKE2;
                }

                //OTWDriver.AddTrailHead (smokeTrail[TRAIL_ENGINE1],  pos.x, pos.y, pos.z );

                //RV - I-Hawk - damage trails size/alpha change with altitude
                if (-ZPos() < 25000.0f)
                {
                    damageTrailAlphaCx = ((15000.0f - (-ZPos())) / 15000.0f) * 0.25f + 1;
                    damageTrailSizeCx = ((15000.0f - (-ZPos())) / 15000.0f) * 0.15f + 1;
                }
                else
                {
                    damageTrailAlphaCx = ((15000.0f - (25000.0f)) / 15000.0f) * 0.25f + 1;
                    damageTrailSizeCx = ((15000.0f - (25000.0f)) / 15000.0f) * 0.15f + 1;
                }

                smokeTrail_trail[TRAIL_ENGINE1] = DrawableParticleSys::PS_EmitTrail(
                                                      smokeTrail_trail[TRAIL_ENGINE1], smokeTrail[TRAIL_ENGINE1],
                                                      pos.x, pos.y, pos.z, damageTrailAlphaCx, damageTrailSizeCx
                                                  );
            }
        }
    }

    if (pctStrength < 0.15f)
    {

        // RV I-Hawk - Do nothing here... no need for more than 2 damage trails max...

        //
        //pos.x = -dmx[1][0]*radius + XPos() + rearOffset.x * 0.75f;
        //pos.y = -dmx[1][1]*radius + YPos() + rearOffset.y * 0.75f;
        //pos.z = -dmx[1][2]*radius + ZPos() + rearOffset.z * 0.75f;

        ////RV - I-Hawk - RV new trails call changes
        //if ( not smokeTrail[TRAIL_ENGINE2])
        //{
        ////smokeTrail[TRAIL_ENGINE2] = new DrawableTrail(TRAIL_SMOKE); // smoke
        ////smokeTrail[TRAIL_ENGINE2] = new DrawableTrail(TRAIL_BURNING_SMOKE); // smoke
        ////OTWDriver.InsertObject(smokeTrail[TRAIL_ENGINE2]);
        //smokeTrail[TRAIL_ENGINE2] = TRAIL_BURNING_SMOKE;
        //}
        ////OTWDriver.AddTrailHead (smokeTrail[TRAIL_ENGINE2],  pos.x, pos.y, pos.z );
        ////
        ////RV - I-Hawk - damage trails size/alpha change with altitude
        //if ( -ZPos() < 25000.0f)
        //{
        //damageTrailAlpha = (( 10000.0f - (-ZPos())) / 10000.0f) * 0.25f + 1;
        //damageTrailSize =  (( 10000.0f - (-ZPos())) / 10000.0f) * 0.25f + 1;
        //}
        //else
        //{
        //damageTrailAlpha = (( 10000.0f - (25000.0f)) / 10000.0f) * 0.25f + 1;
        //damageTrailSize =  (( 10000.0f - (25000.0f)) / 10000.0f) * 0.25f + 1;
        //}
        //smokeTrail_trail[TRAIL_ENGINE2] = DrawableParticleSys::PS_EmitTrail(smokeTrail_trail[TRAIL_ENGINE2], smokeTrail[TRAIL_ENGINE2], pos.x, pos.y, pos.z, damageTrailAlpha, damageTrailSize);

        //// occasionalyy add a smoke cloud
        ///*
        //if ( sfxTimer > 0.2f and (rand() bitand 0x3) == 0x3 )
        //{
        //sfxTimer = 0.0f;
        //OTWDriver.AddSfxRequest(
        // new SfxClass (SFX_TRAILSMOKE, // type
        // &pos, // world pos
        // 2.5f, // time to live
        // 2.0f ) ); // scale
        //}
        //
    }

    // occasionally, perturb the controls
    // JB 010104
    //if ( pctStrength < 0.5f and (rand() bitand 0xf) == 0xf)
    if ( not g_bDisableFunkyChicken and pctStrength < 0.5f and (rand() bitand 0xf) == 0xf)
    {
        // JB 010104
        ioPerturb = 0.5f + (1.0f - pctStrength);
        // good place also to stick in a damage, clunky sound....
    }
}


void AircraftClass::CleanupDamage()
{
    for (int i = 0; i < TRAIL_MAX; i++)
    {
        if (smokeTrail[i])
        {

            /* RV - I-Hawk - RV new trails call changes
            OTWDriver.AddSfxRequest(
             new SfxClass (
             11.2f, // time to live
             smokeTrail[i]) ); // scale
             */
            DrawableParticleSys::PS_KillTrail(smokeTrail_trail[i]);
            smokeTrail[i] = smokeTrail_trail[i] = NULL;
        }
    }

    return;
}


/*
** Name: CheckObjectCollision
** Description:
** Crawls thru the target list for the aircraft.  Checks the range
** to the target and compares it with the radii of both objects as well
** as the rangedot.  if rangedot is moving closer and range is less
** than the sum of the 2 radii, damage messages are sent to both.
**
** A caveat, since we're using targetList, is that non-ownships only
** detect collisions with the enemy.
*/
// VP_changes
//This is a problem that should be fixed, since we're using targetList, is that non-ownships only
//detect collisions with the enemy.
// Here the function for explosion's gust can be added
void AircraftClass::CheckObjectCollision(void)
{

    SimObjectType *obj;
    SimBaseClass *theObject;
    SimObjectLocalData *objData;
    FalconDamageMessage *message;
    bool setOnObject = false; // JB carrier

    // no detection on ground when not moving
    //if ( not af->IsSet(AirframeClass::OnObject) and // JB carrier
    // OnGround() and af->vt == 0.0f )
    //{
    // return;
    //}
    //
    //if ( not af->IsSet(AirframeClass::OnObject) and // JB carrier
    // OnGround() and af->vcas <= 50.0f and gCommsMgr and gCommsMgr->Online()) // JB 010107
    //{
    // return; // JB 010107
    //} //Cobra Test removal

    // loop thru all targets
    for (obj = targetList; obj; obj = obj->next)
    {
        objData = obj->localData;
        theObject = (SimBaseClass*)obj->BaseData();

        if (theObject == NULL)
        {
            continue;
        }

        // RV - Biker - Don't collide with weapons belonging to us
        if (theObject->IsWeapon() and ((SimWeaponClass*)theObject)->Parent() == this)
        {
            continue;
        }

        if (F4IsBadReadPtr(theObject, sizeof(SimBaseClass)) or not theObject->IsSim() or (OnGround() and not theObject->OnGround()))
        {
            continue;
        }

        if (theObject->drawPointer == NULL)
        {
            continue;
        }

        // RV - Biker - We cannot collide with something that far away
        if (objData->range > 1.0f * NM_TO_FT)
        {
            continue;
        }

        // Stop now if the spheres don't overlap
        // special case the tanker -- we want to be able to get in closer
        // JB carrier
        if (af->IsSet(AirframeClass::OnObject) or not OnGround())
        {
            if (IsSetFlag(I_AM_A_TANKER) or theObject->IsSetFlag(I_AM_A_TANKER))
            {
                //if ( objData->range >  0.1f * theObject->GetGfx()->Radius()){// + GetGfx()->Radius() ) // PJW
                // continue;
                //}
                if (objData->range > 0.2F * theObject->drawPointer->Radius() + theObject->drawPointer->Radius())
                {
                    continue;
                }

                Tpoint org, vec, pos;

                org.x = XPos();
                org.y = YPos();
                org.z = ZPos();
                vec.x = XDelta() - theObject->XDelta();
                vec.y = YDelta() - theObject->YDelta();
                vec.z = ZDelta() - theObject->ZDelta();

                // we're within the range of the object's radius.
                // for ships, this may be a BIG radius -- longer than
                // high.  Let's also check the bounding box
                // scale the box so we might possibly miss it
                if ( not theObject->drawPointer->GetRayHit(&org, &vec, &pos, 0.1f))
                    continue;


            }
            else
            {
                if (objData->range > theObject->drawPointer->Radius() + drawPointer->Radius())
                    if (theObject->GetDomain() not_eq DOMAIN_SEA or theObject->GetClass() not_eq CLASS_VEHICLE or theObject->GetType() not_eq TYPE_CAPITAL_SHIP or carrierInitTimer >= 30.0f)
                    {
                        continue;
                    }

                //else if ( theObject->OnGround() )
                //{
                Tpoint org, vec, pos;

                org.x = XPos();
                org.y = YPos();

                if (af->IsSet(AirframeClass::OnObject)) // JB carrier
                    org.z = ZPos() + 5; // Make sure its detecting that we are on top // JB carrier
                else // JB carrier
                    org.z = ZPos();

                vec.x = XDelta() - theObject->XDelta();
                vec.y = YDelta() - theObject->YDelta();
                vec.z = ZDelta() - theObject->ZDelta();
                // we're within the range of the object's radius.
                // for ships, this may be a BIG radius -- longer than
                // high.  Let's also check the bounding box

                if ( not theObject->drawPointer->GetRayHit(&org, &vec, &pos, 1.0f))
                {
                    if (theObject->GetDomain() not_eq DOMAIN_SEA or theObject->GetClass() not_eq CLASS_VEHICLE or theObject->GetType() not_eq TYPE_CAPITAL_SHIP or carrierInitTimer >= 30.0f)
                        continue;
                }

                //}
            }
        }
        else // on ground
        {
            if (objData->range > theObject->drawPointer->Radius())  // + drawPointer->Radius() ) { // PJW
            {
                continue;
            }
        }

        // Don't collide ejecting pilots with their aircraft
        if (theObject->IsEject())
        {
            if (((EjectedPilotClass*)theObject)->GetParentAircraft() == this)
                continue;
        }

        //***********************************************
        // If we get here, we've decided we've collided
        //***********************************************

        // JB carrier start
        if (IsAirplane() and theObject->GetDomain() == DOMAIN_SEA and theObject->GetClass() == CLASS_VEHICLE and theObject->GetType() == TYPE_CAPITAL_SHIP)
        {
            if (carrierInitTimer < 30.0f)
                carrierInitTimer += SimLibMajorFrameTime;

            // RV - Biker - Carrier hitbox
            Tpoint minB;
            Tpoint maxB;
            ((DrawableBSP*) theObject->drawPointer)->GetBoundingBox(&minB, &maxB);

            // JB 010731 Hack for unfixed hitboxes.
            //if (minB.z < -193.0f and minB.z > -194.0f)
            // minB.z = -72.0f;

            // RV - Biker - Try to get the values from slot data
            Tpoint startPos;
            Tpoint tmpPos;
            Tpoint startAngle;

            startPos.x = 0.0f;
            startPos.y = 0.0f;
            startPos.z = 0.0f;

            startAngle.x = 0.0f;
            startAngle.y = 0.0f;
            startAngle.z = 0.0f;

            int numSlots = ((DrawableBSP*) theObject->drawPointer)->GetNumSlots();
            int numDynamics = ((DrawableBSP*) theObject->drawPointer)->GetNumDynamicVertices();

            // RV - Biker - Choose take-off slot randomly
            if (numSlots > 0 and takeoffSlot > numSlots - 1)
            {
                takeoffSlot = rand() % numSlots;
            }

            if (numSlots > 0)
            {
                ((DrawableBSP*) theObject->drawPointer)->GetChildOffset(takeoffSlot, &tmpPos);
            }
            else
            {
                tmpPos.x = max(minB.x * 0.75f, -200.0f);
                tmpPos.y = 0.0f;
                tmpPos.z = 0.0f;
            }

            // RV - Biker - Read takeoff angle from dynamic data
            if (numDynamics > 0)
            {
                ((DrawableBSP*) theObject->drawPointer)->GetDynamicCoords(min(takeoffSlot, numDynamics - 1), &startAngle.x, &startAngle.y, &startAngle.z);
            }

            startAngle.x = max(min(startAngle.x, 45.0f), -45.0f) * DTR;

            // RV - Biker - Rotate AC with carrier
            if (af->vt <= 1.0f and af->Thrust() <= 1.0f and (carrierInitTimer < 10.0f or theObject->YawDelta() not_eq 0.0f))
            {
                af->sigma = theObject->Yaw() + startAngle.x;
                af->ResetOrientation();
            }

            if (theObject->Yaw() >= 178.0f * DTR or theObject->Yaw() <= -178.0f * DTR)
            {
                startPos.x = -tmpPos.x;
                startPos.y = -tmpPos.y;
                startPos.z =  tmpPos.z;
            }
            else
            {
                MatrixMult(&((DrawableBSP*) theObject->drawPointer)->orientation, &tmpPos, &startPos);
                startPos.z = tmpPos.z;
            }

            if (startPos.z not_eq 0.0f and numSlots > 0)
                minB.z = startPos.z;

            float deltaX = abs(af->x - theObject->XPos());
            float deltaY = abs(af->y - theObject->YPos());

            // RV - Biker - Move AC in correct position
            if (af->vt <= 1.0f and af->Thrust() <= 1.0f and carrierInitTimer < 25.0f)
            {
                if ((deltaX < abs(startPos.x) - 2.5f or deltaX > abs(startPos.x) + 2.5f))
                {
                    af->x = theObject->XPos() + startPos.x;
                }

                if ((deltaY < abs(startPos.y) - 2.5f or deltaY > abs(startPos.y) + 2.5f))
                {
                    af->y = theObject->YPos() + startPos.y;
                }
            }

            // RV - Biker - Check if AC is in correct position otherwise reject "CTRL K"
            bool onCatPosition = false;

            if (deltaX >= abs(startPos.x) - 5.0f and 
                deltaX <= abs(startPos.x) + 5.0f and 
                deltaY >= abs(startPos.y) - 5.0f and 
                deltaY <= abs(startPos.y) + 5.0f and 
                abs(Yaw() - startAngle.x - theObject->Yaw()) <= 5.0f * DTR)
            {
                onCatPosition = true;
            }
            else if (numSlots > 0 and af->vt <= 1.0f)
            {
                af->ClearFlag(AirframeClass::Hook);
            }

            // RV - Biker - Deploy catapult deflector
            if (numSlots > 0)
            {
                float changeval;
                float cdof = ((DrawableBSP*) theObject->drawPointer)->GetDOFangle(takeoffSlot + 10);

                if (af->IsSet(AirframeClass::Hook) and onCatPosition == true)
                {
                    if (cdof < 55.0f * DTR)
                    {
                        changeval = 30.0f * DTR * SimLibMajorFrameTime;
                    }
                    else
                    {
                        changeval = 10.0f * DTR * SimLibMajorFrameTime;
                    }

                    if (cdof < 65.0f * DTR)
                    {
                        cdof += changeval;

                        if (cdof >= 65.0f * DTR)
                        {
                            cdof = 65.0f * DTR;
                        }

                        ((DrawableBSP*) theObject->drawPointer)->SetDOFangle(takeoffSlot + 10, cdof);
                    }

                    //RV - I-Hawk - Add Cat steam effects when trapped

                    Tpoint pos, randPos1, randPos2, randPos3, randPos4, noseGear, vec;

                    noseGear.x = af->GetAeroData(AeroDataSet::NosGearX) + XPos();
                    noseGear.y = af->GetAeroData(AeroDataSet::NosGearY) + YPos();
                    noseGear.z = af->GetAeroData(AeroDataSet::NosGearZ) - 5.0f + ZPos();

                    pos.x = theObject->XPos() + startPos.x;
                    pos.y = theObject->YPos() + startPos.y;
                    pos.z = theObject->ZPos() + startPos.z;

                    randPos1.x = pos.x + 120.0f * PRANDFloat();
                    randPos1.y = pos.y + 35.0f * PRANDFloat();

                    randPos2.x = pos.x + 85.0f * PRANDFloat();
                    randPos2.y = pos.y - 35.0f * PRANDFloat();

                    randPos3.x = pos.x - 45.0f * PRANDFloat();
                    randPos3.y = pos.y + 25.0f * PRANDFloat();

                    randPos4.x = pos.x - 60.0f * PRANDFloat();
                    randPos4.y = pos.y + 60.0f * PRANDFloat();

                    randPos1.z = randPos2.z = randPos3.z = randPos4.z = pos.z;

                    //Randomize the vector a bit... Yaw pushes the smoke away more
                    //quickly, Delta make it hover a bit longer over the deck...
                    if ((rand() bitand 3) not_eq 3)
                    {
                        vec.x = theObject->Yaw();
                        vec.y = theObject->Yaw();
                    }

                    else
                    {
                        vec.x = theObject->XDelta();
                        vec.y = theObject->YDelta();
                    }

                    vec.z = 0;

                    //The main steam SFX
                    DrawableParticleSys::PS_AddParticleEx(
                        (SFX_CAT_STEAM + 1), &pos, &vec);

                    //Some random deck "leaks" steam SFX
                    DrawableParticleSys::PS_AddParticleEx(
                        (SFX_CAT_RANDOM_STEAM + 1), &randPos1, &vec);

                    DrawableParticleSys::PS_AddParticleEx(
                        (SFX_CAT_RANDOM_STEAM + 1), &randPos2, &vec);

                    DrawableParticleSys::PS_AddParticleEx(
                        (SFX_CAT_RANDOM_STEAM + 1), &randPos3, &vec);

                    DrawableParticleSys::PS_AddParticleEx(
                        (SFX_CAT_RANDOM_STEAM + 1), &randPos4, &vec);
                }
                else
                {
                    if (cdof > 55.0f * DTR)
                    {
                        changeval = 10.0f * DTR * SimLibMajorFrameTime;
                    }
                    else
                    {
                        changeval = 120.0f * DTR * SimLibMajorFrameTime;
                    }

                    if (cdof > 0.0f * DTR)
                    {
                        cdof -= changeval;

                        if (cdof <= 0.0f * DTR)
                        {
                            cdof = 0.0f * DTR;
                        }

                        ((DrawableBSP*) theObject->drawPointer)->SetDOFangle(takeoffSlot + 10, cdof);
                    }
                }
            }

            // RV - Biker - Give nose gear offset when carrier takeoff
            float gearOffset = min(5.0f, af->GetAeroData(AeroDataSet::NosGearZ) - 5.0f);

            minB.z = minB.z - gearOffset;

            // RV - I-Hawk - Add AC launch smoke effect when doing the take-off run
            Tpoint noseGear, PSvec;

            noseGear.x = af->GetAeroData(AeroDataSet::NosGearX) + XPos();
            noseGear.y = af->GetAeroData(AeroDataSet::NosGearY) + YPos();
            noseGear.z = af->GetAeroData(AeroDataSet::NosGearZ) - gearOffset + ZPos();

            PSvec.x = (theObject->XDelta());
            PSvec.y = (theObject->YDelta());
            PSvec.z = 0;

            if (af->vcas > 40.0f and af->vcas < 180.0f and af->IsSet(AirframeClass::OnObject) and not (af->IsSet(AirframeClass::Hook)))
            {
                DrawableParticleSys::PS_AddParticleEx(
                    (SFX_CAT_LAUNCH + 1), &noseGear, &PSvec);
            }

            //if(/*(ZPos() <= minB.z * /*.96*/.90f and ZPos() > minB.z * /*1.02*/1.1f or */(ZPos() > -g_fCarrierStartTolerance and af->vcas < 10.0f /*.01*/))//Cobra aircraft coming in at 6.5 knots and thus failing this check
            if ((ZPos() <= minB.z * 0.96f and ZPos() > minB.z - 0.5f) or (ZPos() > -g_fCarrierStartTolerance and af->vcas < 10.0f))
            {
                // the eagle has landed
                attachedEntity = theObject;
                af->SetFlag(AirframeClass::OnObject);
                af->SetFlag(AirframeClass::OverRunway);
                setOnObject = true;

                // Set our anchor so that when we're moving slowly we can accumulate our position in high precision
                // RV - Biker - Do the offset if desired
                if (carrierStartPosEngaged == 0 and abs(theObject->YawDelta()) < 0.25f and carrierInitTimer > 1.0f)
                {
                    carrierStartPosEngaged = 1;
                    af->x = af->x + startPos.x;
                    af->y = af->y + startPos.y;
                }

                af->groundAnchorX = af->x;
                af->groundAnchorY = af->y;
                af->groundDeltaX = 0.0f;
                af->groundDeltaY = 0.0f;
                af->platform->SetFlag(ON_GROUND);

                float gndGmma, relMu;

                af->CalculateGroundPlane(&gndGmma, &relMu);

                af->stallMode = AirframeClass::None;
                af->slice = 0.0F;
                af->pitch = 0.0F;

                if (af->IsSet(af->GearBroken) or af->gearPos <= 0.1F)
                {
                    if (af->platform->DBrain() and not af->platform->IsSetFalcFlag(FEC_INVULNERABLE))
                    {
                        af->platform->DBrain()->SetATCFlag(DigitalBrain::Landed);
                        af->platform->DBrain()->SetATCStatus(lCrashed);

                        // KCK NOTE:: Don't set timer for players
                        if (af->platform not_eq SimDriver.GetPlayerEntity())
                            af->platform->DBrain()->SetWaitTimer(SimLibElapsedTime + 1 * CampaignMinutes);
                    }
                }

                Tpoint velocity;
                Tpoint noseDir;
                float impactAngle, noseAngle;
                float tmp;

                float vt = GetVt();
                velocity.x = af->xdot / vt;
                velocity.y = af->ydot / vt;
                velocity.z = af->zdot / vt;

                noseDir.x = af->platform->platformAngles.costhe * af->platform->platformAngles.cospsi;
                noseDir.y = af->platform->platformAngles.costhe * af->platform->platformAngles.sinpsi;
                noseDir.z = -af->platform->platformAngles.sinthe;
                tmp = (float)sqrt(noseDir.x * noseDir.x + noseDir.y * noseDir.y + noseDir.z * noseDir.z);
                noseDir.x /= tmp;
                noseDir.y /= tmp;
                noseDir.z /= tmp;

                noseAngle = af->gndNormal.x * noseDir.x + af->gndNormal.y * noseDir.y + af->gndNormal.z * noseDir.z;
                impactAngle = af->gndNormal.x * velocity.x + af->gndNormal.y * velocity.y + af->gndNormal.z * velocity.z;

                impactAngle = (float)fabs(impactAngle);

                if (ZPos() > -g_fCarrierStartTolerance)
                {
                    // We just started inside the carrier
                    SetAutopilot(APOff);
                    af->onObjectHeight = minB.z;
                    noseAngle = 0;
                    impactAngle = 0;
                }
                else
                    af->onObjectHeight = ZPos();

                if (vt == 0.0f)
                    continue;

                // do the landing check (no damage)
                if ( not af->IsSet(AirframeClass::InAir) or af->platform->LandingCheck(noseAngle, impactAngle, COVERAGE_OBJECT))
                {
                    af->ClearFlag(AirframeClass::InAir);
                    continue;
                }
            }
            else if (ZPos() <= minB.z * 0.96f)
                continue;
        }

        if (isDigital or not PlayerOptions.CollisionsOn())
            continue;

        // JB carrier end

        //Cobra Lest Dewdog whine forever
        //This should keep online players from "colliding"
        if ( not af->IsSet(AirframeClass::OnObject) and OnGround() and af->vcas <= 50.0f
           and gCommsMgr and gCommsMgr->Online())
        {
            continue;
        }

        // RV - Biker - If we are in initialization don't crash
        if (carrierInitTimer < 30.0f)
        {
            continue;
        }

        // 2002-04-17 MN fix for killer chaff / flare
        if (theObject->GetType() == TYPE_BOMB and 
            (theObject->GetSType() == STYPE_CHAFF or theObject->GetSType() == STYPE_FLARE1) and 
            (theObject->GetSPType() == SPTYPE_CHAFF1 or theObject->GetSPType() == SPTYPE_CHAFF1 + 1))
            continue;

        if ( not isDigital)
            g_intellivibeData.CollisionCounter++;

        // send message to self
        // VuTargetEntity *owner_session = (VuTargetEntity*)vuDatabase->Find(OwnerId());
        message = new FalconDamageMessage(Id(), FalconLocalGame);
        message->dataBlock.fEntityID  = theObject->Id();

        message->dataBlock.fCampID    = theObject->GetCampID();
        message->dataBlock.fSide      = theObject->GetCountry();

        if (theObject->IsAirplane())
            message->dataBlock.fPilotID   = ((SimMoverClass*)theObject)->pilotSlot;
        else
            message->dataBlock.fPilotID   = 255;

        message->dataBlock.fIndex     = theObject->Type();
        message->dataBlock.fWeaponID  = theObject->Type();
        message->dataBlock.fWeaponUID = theObject->Id();

        message->dataBlock.dEntityID  = Id();
        ShiAssert(GetCampaignObject())
        message->dataBlock.dCampID = GetCampID();
        message->dataBlock.dSide   = GetCountry();

        if (IsAirplane())
            message->dataBlock.dPilotID   = pilotSlot;
        else
            message->dataBlock.dPilotID   = 255;

        message->dataBlock.dIndex     = Type();
        message->dataBlock.damageType = FalconDamageType::ObjectCollisionDamage;

        Tpoint Objvec, Myvec, relVec;
        float relVel;

        Myvec.x = XDelta();
        Myvec.y = YDelta();
        Myvec.z = ZDelta();

        Objvec.x = theObject->XDelta();
        Objvec.y = theObject->YDelta();
        Objvec.z = theObject->ZDelta();

        relVec.x = Myvec.x - Objvec.x;
        relVec.y = Myvec.y - Objvec.y;
        relVec.z = Myvec.z - Objvec.z;

        relVel = (float)sqrt(relVec.x * relVec.x + relVec.y * relVec.y + relVec.z * relVec.z);

        // for now use maxStrength as amount of damage.
        // later we'll want to add other factors into the equation --
        // on ground, speed, etc....

        message->dataBlock.damageRandomFact = 1.0f;

        message->dataBlock.damageStrength = min(1000.0F, relVel * theObject->Mass() * 0.0001F + relVel * relVel * theObject->Mass() * 0.000002F);

        message->RequestOutOfBandTransmit();

        if (message->dataBlock.damageStrength > 0.0f) // JB carrier
            FalconSendMessage(message, TRUE);

        // send message to other ship
        // owner_session = (VuTargetEntity*)vuDatabase->Find(theObject->OwnerId());
        message = new FalconDamageMessage(theObject->Id(), FalconLocalGame);
        message->dataBlock.fEntityID  = Id();
        ShiAssert(GetCampaignObject())
        message->dataBlock.fCampID = GetCampID();
        message->dataBlock.fSide   = GetCountry();
        message->dataBlock.fPilotID   = pilotSlot;
        message->dataBlock.fIndex     = Type();
        message->dataBlock.fWeaponID  = Type();
        message->dataBlock.fWeaponUID = theObject->Id();

        message->dataBlock.dEntityID  = theObject->Id();
        message->dataBlock.dCampID = theObject->GetCampID();
        message->dataBlock.dSide   = theObject->GetCountry();

        if (theObject->IsAirplane())
            message->dataBlock.dPilotID   = ((SimMoverClass*)theObject)->pilotSlot;
        else
            message->dataBlock.dPilotID   = 255;

        message->dataBlock.dIndex     = theObject->Type();
        // for now use maxStrength as amount of damage.
        // later we'll want to add other factors into the equation --
        // on ground, speed, etc....

        message->dataBlock.damageRandomFact = 1.0f;
        message->dataBlock.damageStrength = min(1000.0F, relVel * Mass() * 0.0001F + relVel * relVel * Mass() * 0.000002F);

        message->dataBlock.damageType = FalconDamageType::ObjectCollisionDamage;
        message->RequestOutOfBandTransmit();

        if (message->dataBlock.damageStrength > 0.0f) // JB carrier
            FalconSendMessage(message, TRUE);
    } // end target list loop

    // JB carrier start
    if ( not setOnObject and af->IsSet(AirframeClass::OnObject))
    {
        attachedEntity = NULL;
        af->ClearFlag(AirframeClass::OverRunway);
        af->ClearFlag(AirframeClass::OnObject);
        af->ClearFlag(AirframeClass::Planted);
        af->platform->UnSetFlag(ON_GROUND);
        af->SetFlag(AirframeClass::InAir);
    }

    // JB carrier end
}

//void AircraftClass::AddFault(int failures, unsigned int failuresPossible, int numToBreak, int sourceOctant)
void AircraftClass::AddFault(int failures, unsigned int failuresPossible, int, int)
{
    int i;

    // failures = numToBreak * (float)rand() / (float)RAND_MAX;

    for (i = 0; i < failures; i++)
    {
        mFaults->SetFault(failuresPossible, not isDigital);
    }

    // JPO - break hydraulics occasionally
    if ((failuresPossible bitand FaultClass::eng_fault) and 
        (mFaults->GetFault(FaultClass::eng_fault) bitand FaultClass::hydr))
    {
        if (rand() % 100 < 20)   // 20% failure chance of A system
        {
            af->HydrBreak(AirframeClass::HYDR_A_SYSTEM);
        }

        if (rand() % 100 < 20)   // 20% failure chance of B system
        {
            af->HydrBreak(AirframeClass::HYDR_B_SYSTEM);
        }
    }

    // also break the generators now and then
    if (failuresPossible bitand FaultClass::eng_fault)
    {
        if (rand() % 7 == 1)
            af->GeneratorBreak(AirframeClass::GenStdby);

        if (rand() % 7 == 1)
            af->GeneratorBreak(AirframeClass::GenMain);
    }

#if 0
    int failedThing;
    int i, j = 0;
    int failedThings[FaultClass::NumFaultListSubSystems];
    BOOL Found;
    int canFail;
    int numFunctions = 0;
    int failedFunc;

    failures = numToBreak * (float)rand() / (float)RAND_MAX;

    for (i = 0; i < FaultClass::NumFaultListSubSystems; i++)
    {
        if (failuresPossible bitand (1 << i))
        {
            failedThings[j] = i;
            j++;
        }
    }

    for (i = 0; i < failures; i++)
    {
        Found = FALSE;

        do
        {
            failedThing = j * (float)rand() / (float)RAND_MAX;
            numFunctions = 0;

            if (failedThings[failedThing] not_eq -1)
            {
                // FLCS is a special case, as it has 1 informational fault which MUST happen first
                if ((FaultClass::type_FSubSystem)failedThings[failedThing] == FaultClass::flcs_fault and 
 not mFaults->GetFault(FaultClass::flcs_fault))
                {
                    failedFunc = 1;
                    numFunctions = -1;

                    while (failedFunc)
                    {
                        numFunctions ++;

                        if (FaultClass::sngl bitand (1 << numFunctions))
                            failedFunc --;
                    }
                }
                else
                {
                    canFail   = mFaults->Breakable((FaultClass::type_FSubSystem)failedThings[failedThing]);

                    // How many functions?
                    while (canFail)
                    {
                        if (canFail bitand 0x1)
                            numFunctions ++;

                        canFail = canFail >> 1;
                    }

                    // pick 1 of canFail things
                    failedFunc = numFunctions * (float)rand() / (float)RAND_MAX + 1;

                    // Find that function
                    canFail   = mFaults->Breakable((FaultClass::type_FSubSystem)failedThings[failedThing]);
                    numFunctions = -1;

                    while (failedFunc)
                    {
                        numFunctions ++;

                        if (canFail bitand (1 << numFunctions))
                            failedFunc --;
                    }
                }

                mFaults->SetFault((FaultClass::type_FSubSystem)failedThings[failedThing],
                                  (FaultClass::type_FFunction)(1 << numFunctions),
                                  (FaultClass::type_FSeverity) FaultClass::fail,
 not isDigital); // none, fail for now

                if (failedThings[failedThing] == FaultClass::eng_fault and 
                    (1 << numFunctions) == hydr)
                {
                    if (rand() % 100 < 20)   // 20% failure chance of A system
                    {
                        af->HydrBreak(Airframe::HYDR_A_SYSTEM);
                    }

                    if (rand() % 100 < 20)   // 20% failure chance of B system
                    {
                        af->HydrBreak(Airframe::HYDR_B_SYSTEM);
                    }
                }

                MonoPrint("Failed %s %s\n", FaultClass::mpFSubSystemNames[failedThings[failedThing]],
                          FaultClass::mpFFunctionNames[numFunctions + 1]);
                failedThings[failedThing] = -1;
                Found = TRUE;
            }
        }
        while (Found = FALSE);
    }

#endif
}

void AircraftClass::RunCautionChecks(void)
{
}

void AircraftClass::SetColorContrail(int color)
{
    switch (color)
    {
        case 0:
            colorContrail = TRAIL_CONTRAIL;

        case 1:
            colorContrail = TRAIL_COLOR_0;

        case 2:
            colorContrail = TRAIL_COLOR_1;

        case 3:
            colorContrail = TRAIL_COLOR_2;

        case 4:
            colorContrail = TRAIL_COLOR_3;

        default:
            colorContrail = TRAIL_CONTRAIL;
    }

    return;
}
float Get3DDistance(Tpoint &a, Tpoint &b)
{
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float dz = a.z - b.z;

    return sqrtf(dx * dx + dy * dy + dz * dz);
}

Tpoint Get3DMiddle(Tpoint &a, Tpoint &b)
{
    Tpoint middle;

    middle.x = (a.x + b.x) / 2.0f;
    middle.y = (a.y + b.y) / 2.0f;
    middle.z = (a.z + b.z) / 2.0f;

    return middle;
}

//RV - I-Hawk - local function for assigning AC orientation to trails/PS positions
void AssignACOrientation(Trotation* &orientation, Tpoint &origin, Tpoint &position, Tpoint &worldPosition, bool right)
{
    // FRB - not now
    if ( not orientation)
        return;

    if (right)
    {
        position.x = orientation->M11 * origin.x + orientation->M12 * origin.y + orientation->M13 * origin.z + worldPosition.x;
        position.y = orientation->M21 * origin.x + orientation->M22 * origin.y + orientation->M23 * origin.z + worldPosition.y;
        position.z = orientation->M31 * origin.x + orientation->M32 * origin.y + orientation->M33 * origin.z + worldPosition.z;
    }
    else
    {
        position.x = orientation->M11 * origin.x + orientation->M12 * -origin.y + orientation->M13 * origin.z + worldPosition.x;
        position.y = orientation->M21 * origin.x + orientation->M22 * -origin.y + orientation->M23 * origin.z + worldPosition.y;
        position.z = orientation->M31 * origin.x + orientation->M32 * -origin.y + orientation->M33 * origin.z + worldPosition.z;
    }
}








