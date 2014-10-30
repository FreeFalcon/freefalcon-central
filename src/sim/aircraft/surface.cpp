#include "stdhdr.h"
#include "aircrft.h"
#include "airframe.h"
#include "acmi/src/include/acmirec.h"
#include "otwdrive.h"
#include "classtbl.h"
#include "ffeedbk.h"
#include "simdrive.h"
#include "dofsnswitches.h"
#include "simio.h"
#include "Graphics/Include/drawgrnd.h"
#include "soundfx.h"
#include "fsound.h"
#include "fakerand.h"
#include "sms.h"
#include "Graphics/Include/tod.h"
#include "TrackIr.h"

extern ACMISwitchRecord acmiSwitch;
extern ACMIDOFRecord DOFRec;
static int stallShake = FALSE;
extern bool g_bRealisticAvionics;
extern bool g_bNewFm;
extern bool g_bNewDamageEffects;
extern bool g_bRollLinkedNWSRudder;// ASSOCIATOR 30/11/03 Added for roll unlinked rudder on the ground
extern float g_fACMIAnimRecordTimer;

extern bool g_bAnimPilotHead; // Cobra - Animated Pilot's head
extern float g_fPilotActInterval; // Cobra - Pilot animation act interval (minutes)
extern float g_fPilotHeadMoveRate; // Cobra - Pilot animation act move rate
extern bool g_bEnableTrackIR; // Cobra - Animated Pilot's head
extern float g_fTIR2DPitchPercentage, g_fTIR2DYawPercentage;
extern TrackIR theTrackIRObject; // Retro 27/09/03

// MLR 2/22/2004 - these arrays make it easy to access the gear related DOFs bitand Switches
//                 because the ID numbers are out of order.
int ComplexGearDOF[] =
{
    COMP_GEAR_1,
    COMP_GEAR_2,
    COMP_GEAR_3,
    COMP_GEAR_4,
    COMP_GEAR_5,
    COMP_GEAR_6,
    COMP_GEAR_7,
    COMP_GEAR_8
};

int ComplexGearDoorDOF[] =
{
    COMP_GEAR_DR_1,
    COMP_GEAR_DR_2,
    COMP_GEAR_DR_3,
    COMP_GEAR_DR_4,
    COMP_GEAR_DR_5,
    COMP_GEAR_DR_6,
    COMP_GEAR_DR_7,
    COMP_GEAR_DR_8
};

int ComplexGearSwitch[] =
{
    COMP_GEAR_SW_1,
    COMP_GEAR_SW_2,
    COMP_GEAR_SW_3,
    COMP_GEAR_SW_4,
    COMP_GEAR_SW_5,
    COMP_GEAR_SW_6,
    COMP_GEAR_SW_7,
    COMP_GEAR_SW_8
};

int ComplexGearDoorSwitch[] =
{
    COMP_GEAR_DR_SW_1,
    COMP_GEAR_DR_SW_2,
    COMP_GEAR_DR_SW_3,
    COMP_GEAR_DR_SW_4,
    COMP_GEAR_DR_SW_5,
    COMP_GEAR_DR_SW_6,
    COMP_GEAR_DR_SW_7,
    COMP_GEAR_DR_SW_8
};

int ComplexGearHoleSwitch[] =
{
    COMP_GEAR_HOLE_1,
    COMP_GEAR_HOLE_2,
    COMP_GEAR_HOLE_3,
    COMP_GEAR_HOLE_4,
    COMP_GEAR_HOLE_5,
    COMP_GEAR_HOLE_6,
    COMP_GEAR_HOLE_7,
    COMP_GEAR_HOLE_8
};

int ComplexGearBrokenSwitch[] =
{
    COMP_GEAR_BROKEN_SW_1,
    COMP_GEAR_BROKEN_SW_2,
    COMP_GEAR_BROKEN_SW_3,
    COMP_GEAR_BROKEN_SW_4,
    COMP_GEAR_BROKEN_SW_5,
    COMP_GEAR_BROKEN_SW_6,
    COMP_GEAR_BROKEN_SW_7,
    COMP_GEAR_BROKEN_SW_8
};

//The ACMI is huge





// JPO
// works out flap and aileron angles. Sometimes these are linked as in F-16 flapperons.
void AircraftClass::CalculateAileronAndFlap(float qfactor, float *al, float *ar, float *fl, float *fr)
{
    float stabAngle;
    float flapdelta = 0;

    // elevators always working
    stabAngle = af->rstick * qfactor * af->auxaeroData->aileronMaxAngle * DTR;


    switch (af->auxaeroData->hasTef)
    {
        case AUX_LEFTEF_MANUAL: // nothing special here, just set to what given
            flapdelta = af->tefPos * DTR;
            break;

        case AUX_LEFTEF_AOA: // nothing uses this yet. So not sure what happens
        case AUX_LEFTEF_MACH: // dependendant on vcas (not MACH despite the name)
        {
            float gdelta;
            float speeddelta;

            if (TEFExtend) // forcibly extended
                gdelta = 1;
            else if (af->auxaeroData->flapGearRelative) // else dependent on gear deployment
                gdelta = max(0, af->gearPos);
            else gdelta = 1; // else always

            // work out how much flap we would have at this vcas
            speeddelta = (af->auxaeroData->maxFlapVcas - af->vcas) /
                         af->auxaeroData->flapVcasRange;
            speeddelta = max(0.0F, min(1.0F, speeddelta)); //limit to 0-1 range

            // max flaps * speed dependent factor (0-1) * gear delta (0-1)
            flapdelta = af->auxaeroData->tefMaxAngle  * DTR * speeddelta * gdelta;
            break;
        }
    }

    *al = stabAngle;
    *ar = -stabAngle;
    *fl = flapdelta;
    *fr = flapdelta;

}

// MLR - new function to implement spoilers and wing sweep
void AircraftClass::CalculateSweepAndSpoiler(float &sweep, float &sl1, float &sr1 , float &sl2, float &sr2)
{
    float ailangle, brakeangle = 0, spmax, cursweep;


    // Wing sweep for complex models //////////////////
    sweep = 0;
    cursweep = 0;

    if (acFlags bitand hasSwing)
    {
        if (af->auxaeroData->animSwingWingStages)
        {
            int l;

            if (af->auxaeroData->animSwingWingStages > 10 or af->auxaeroData->animSwingWingStages < 0)
            {
                MonoPrint("animSwingWingStages %d\n", af->auxaeroData->animSwingWingStages);
            }


            // scan in reverse
            for (l = (af->auxaeroData->animSwingWingStages - 1) ; l >= 0 ; l--)
            {
                if (af->mach >= af->auxaeroData->animSwingWingMach[l])
                {
                    sweep = af->auxaeroData->animSwingWingAngle[l];
                    break; // break for loop
                }
            }
        }

        sweep *= DTR;

        // need to retrieve current sweep position
        cursweep = GetDOFValue((IsComplex() ? COMP_SWING_WING : SIMP_SWING_WING_1));
        swingWingAngle = cursweep; // MLR 3/5/2004 -
    }


    // spoiler /////////////////////////////////

    ailangle = af->rstick;

    if (cursweep < af->auxaeroData->animSpoiler1OffAtWingSweep * DTR)
    {
        if (af->auxaeroData->animSpoiler1AirBrake)
            brakeangle = af->dbrake;


        sl1 = brakeangle - ailangle;
        sr1 = brakeangle + ailangle;

        // limit spoiler travel
        if (sl1 > 1) sl1 = 1.0;

        if (sr1 > 1) sr1 = 1.0;

        if (sl1 < 0) sl1 = 0.0;

        if (sr1 < 0) sr1 = 0.0;

        spmax = af->auxaeroData->animSpoiler1Max * DTR;
        sl1 *= spmax;
        sr1 *= spmax;
    }
    else
    {
        sl1 = sr1 = 0;
    }

    if (cursweep < af->auxaeroData->animSpoiler2OffAtWingSweep * DTR)
    {
        if (af->auxaeroData->animSpoiler2AirBrake)
            brakeangle = af->dbrake;

        sl2 = brakeangle - ailangle;
        sr2 = brakeangle + ailangle;

        // limit spoiler travel
        if (sl2 > 1) sl2 = 1.0;

        if (sr2 > 1) sr2 = 1.0;

        if (sl2 < 0) sl2 = 0.0;

        if (sr2 < 0) sr2 = 0.0;

        spmax = af->auxaeroData->animSpoiler2Max * DTR;
        sl2 *= spmax;
        sr2 *= spmax;
    }
    else
    {
        sl2 = sr2 = 0;
    }



}


void AircraftClass::CalculateLef(float qfactor)
{
    // LEF
    if (af->auxaeroData->hasLef == AUX_LEFTEF_MANUAL)
    {
        // use pilot input
        leftLEFAngle = af->lefPos * DTR;
        rightLEFAngle = af->lefPos * DTR;
    }
    else if (af->auxaeroData->hasLef == AUX_LEFTEF_TEF)   // LEF controlled by TEF
    {
        if (af->tefPos > 0)
            af->LEFMax();
        else af->LEFClose();

        leftLEFAngle = af->lefPos * DTR;
        rightLEFAngle = af->lefPos * DTR;
    }
    else
    {
        if ( not af->IsSet(AirframeClass::InAir))
        {
            leftLEFAngle = rightLEFAngle = af->auxaeroData->lefGround * DTR;
        }
        else if ( not g_bNewFm and af->mach > af->auxaeroData->lefMaxMach)
        {
            leftLEFAngle = rightLEFAngle = 0;;
        }
        else if (af->auxaeroData->hasLef == AUX_LEFTEF_AOA)
        {
            leftLEFAngle = max(min(af->alpha, af->auxaeroData->lefMaxAngle) * DTR, 0.0f); //me123 lef is controled my aoa not mach
            rightLEFAngle = leftLEFAngle;

            //MI additions
            if (g_bRealisticAvionics and g_bNewFm)
            {
                if (g_bNewDamageEffects)
                {
                    leftLEFAngle = CheckLEF(0); //left
                    rightLEFAngle = CheckLEF(1); //right
                }
            }
        }
        else
        {
            leftLEFAngle = min((af->auxaeroData->lefMaxMach - af->mach) / 0.2F, 1.0F) * af->auxaeroData->lefMaxAngle * DTR;
            rightLEFAngle = leftLEFAngle;
        }

        if (LEFLocked and g_bRealisticAvionics and g_bNewFm)
        {
            if (IsComplex())
            {
                leftLEFAngle = GetDOFValue(COMP_LT_LEF);
                rightLEFAngle = GetDOFValue(COMP_RT_LEF);
            }
            else
            {
                leftLEFAngle = GetDOFValue(SIMP_LT_LEF);
                rightLEFAngle = GetDOFValue(SIMP_RT_LEF);
            }

        }
    }
}

void AircraftClass::CalculateStab(float qfactor, float *sl, float *sr)
{
    if (af->auxaeroData->elevatorRolls)
    {
        *sl = max(min(af->pstick - af->rstick, 1.0F), -1.0F) * qfactor * af->auxaeroData->elevonMaxAngle * DTR;
        *sr = max(min(af->pstick + af->rstick, 1.0F), -1.0F) * qfactor * af->auxaeroData->elevonMaxAngle * DTR;
    }
    else
    {
        *sr = *sl = max(min(af->pstick, 1.0F), -1.0F) * qfactor * af->auxaeroData->elevonMaxAngle * DTR;
    }
}

float AircraftClass::CalculateRudder(float qfactor)
{
    return af->ypedal * af->auxaeroData->rudderMaxAngle * DTR * qfactor;
}

// JPO - routine to get the DOF to move to the desirted position
// happens at a given rate though. Optionally play SFX during the time.
void AircraftClass::MoveDof(int dof, float newval, float rate, int ssfx, int lsfx, int esfx)
{
    float changeval;
    float cdof = GetDOFValue(dof);
    bool doend = false;

    if (cdof == newval) return; // all done

    changeval = rate * DTR * SimLibMajorFrameTime;

    if (cdof > newval)
    {
        cdof -= changeval;

        if (cdof <= newval)
        {
            cdof = newval;
            doend = true;
        }

        SetDOF(dof, cdof);
    }
    else if (cdof < newval)
    {
        cdof += changeval;

        if (cdof >= newval)
        {
            cdof = newval;
            doend = true;
        }

        SetDOF(dof, cdof);
    }

    if (SFX_DEF and ssfx >= 0)
    {
        // something to play
        // MLR 12/30/2003 - reorganized for clarity
        if (doend)
        {
            SoundPos.Sfx(esfx);
        }
        else
        {
            if ( not SoundPos.IsPlaying(ssfx) and // MLR 12/30/2003 - changed IsPlaying sound call
 not SoundPos.IsPlaying(lsfx))
                SoundPos.Sfx(ssfx);
            else
                SoundPos.Sfx(lsfx);
        }
    }
}

void AircraftClass::DeployDragChute(int type)
{
    if (af->vcas < 20.0f and af->dragChute == AirframeClass::DRAGC_DEPLOYED)
        af->dragChute = AirframeClass::DRAGC_TRAILING;

    if (af->dragChute == AirframeClass::DRAGC_DEPLOYED and 
        af->vcas > af->auxaeroData->dragChuteMaxSpeed)
    {
        if ((af->vcas - af->auxaeroData->dragChuteMaxSpeed) / 100 > PRANDFloatPos())
            af->dragChute = AirframeClass::DRAGC_RIPPED;
    }

    if (af->dragChute not_eq GetSwitch(type))
    {
        if (af->dragChute == AirframeClass::DRAGC_DEPLOYED)
            SoundPos.Sfx(af->auxaeroData->sndDragChute); // MLR 5/16/2004 -

        //F4SoundFXSetPos( af->auxaeroData->sndDragChute, TRUE, af->x, af->y, af->z, 1.0f );

        SetSwitch(type, af->dragChute);
    }
}

void AircraftClass::MoveSurfaces(void)
{
    float qFactor, stabAngle;
    int gFact;
    int i, stage;

    qFactor = 100.0F / (max(af->vcas, 100.0F));

    // are we in easter-egg heli mode?
    if (af and af->GetSimpleMode() == SIMPLE_MODE_HF)
    {
        /*
        Switch and Dof settings for Helo
        sw0 bit1=fast moving blades; bit2=slow moving blades
        sw2 makes Muzzle flash visible
        dof2 main rotor
        dof3 Muzzle Flash
        dof4 tail or secondary rotor
         */
        // animate rotors
        if (GetSwitch(HELI_ROTORS) not_eq 1)
        {
            SetSwitch(HELI_ROTORS, 1);
            SetDOF(HELI_MAIN_ROTOR, 0.0f);
            SetDOF(HELI_TAIL_ROTOR, 0.0f);
        }

        // blades rotate at around 300 RPM which is 1800 DPS
        float curDOF = GetDOFValue(HELI_MAIN_ROTOR);
        float deltaDOF = 1800.0f * DTR * SimLibMajorFrameTime;
        curDOF += deltaDOF;

        if (curDOF > 360.0f * DTR)
            curDOF -= 360.0f * DTR;

        SetDOF(HELI_MAIN_ROTOR, curDOF);
        SetDOF(HELI_TAIL_ROTOR, curDOF);
        return;
    }

    float rpm[2];

    if (IsLocal())
    {
        rpm[0] = af->rpm;
        rpm[1] = af->rpm2;
    }
    else
    {
        rpm[0] = specialData.powerOutput;
        rpm[1] = specialData.powerOutput2;
    }


    {
        // MLR 2003-10-12 Throttle DOF (works on complex bitand simple - same ID)
        //SetDOF(COMP_THROTTLE,af->Throtl()); // simple eh?
        //ATARIBABY change for 2 engine ACs to be correct, help from Mike
        if (af->auxaeroData->nEngines == 2)
        {
            SetDOF(COMP_THROTTLE, af->engine1Throttle);
            // SetDOF(COMP_THROTTLE2,af->engine2Throttle); i will add later with second engine needles too
        }
        else
        {
            SetDOF(COMP_THROTTLE, af->Throtl()); // simple eh?
        }

        SetDOF(COMP_RPM, rpm[0]);

        // MLR 2003-10-12 Animated refuel probe
        MoveDof(COMP_REFUEL, (af->IsEngineFlag(AirframeClass::FuelDoorOpen) ? af->auxaeroData->animRefuelAngle * DTR : 0), af->auxaeroData->animRefuelRate);
    }

    {
        // MLR - 2003-09-30 Prop code // MLR 1/14/2004 - made available for simple models
        // note, the scripts will be modified to read this dof, and supply it to the appropriate
        // dofs
        int l;
        float curDOF;
        // MLR 1/22/2004 - I forgot to use the RPMMult
        float deltaDOF = rpm[0] * af->auxaeroData->animEngineRPMMult * DTR; // replace the .01 with a global value

        curDOF = GetDOFValue(COMP_PROPELLOR);
        curDOF = fmod((curDOF + deltaDOF) , 6.28318531f);  // 2 * PI
        SetDOF(COMP_PROPELLOR, curDOF);

        if (IsComplex())
        {
            if (af->auxaeroData->nEngines == 2)
            {
                l = 31;
                curDOF = GetDOFValue(l);
                curDOF = fmod((curDOF + deltaDOF) , 6.28318531f);  // 2 * PI
                SetDOF(l , curDOF);

                l = 32;
                deltaDOF = rpm[1] * af->auxaeroData->animEngineRPMMult * DTR; // replace the .01 with a global value
                curDOF = GetDOFValue(l);
                curDOF = fmod((curDOF + deltaDOF) , 6.28318531f);  // 2 * PI
                SetDOF(l , curDOF);
            }
            else
            {
                for (l = 31; l < 37; l++)
                {
                    curDOF = GetDOFValue(l);
                    curDOF = fmod((curDOF + deltaDOF) , 6.28318531f);  // 2 * PI
                    SetDOF(l , curDOF);
                }
            }
        }
    }


    if (IsComplex())
    {
        // Tail Surface
        float leftStab, rightStab;
        CalculateStab(qFactor, &leftStab, &rightStab);
        MoveDof(COMP_LT_STAB, -leftStab,  af->auxaeroData->elevRate);
        MoveDof(COMP_RT_STAB, -rightStab, af->auxaeroData->elevRate);

        float aileronleft, aileronrt;
        float flapleft, flaprt;

        // RV - Biker
        float doorAngle = 0.0f;

        // RV - Biker - Use DOFs defined in DAT files for opening gun and bomb bay doors
        if (GunFire or fireGun)
            doorAngle = 90.0f * DTR;
        else
            doorAngle =  0.0f * DTR;

        if (af->GetGunDofType() >= COMP_WEAPON_BAY_0 and 
            af->GetGunDofType() <= COMP_WEAPON_BAY_4 and 
            af->GetGunDofRate() > 0.0f)
        {

            MoveDof(af->GetGunDofType(), doorAngle, af->GetGunDofRate());
        }

        if (af->GetGunSwitchType() >= COMP_WEAPON_BAY_0_SW and 
            af->GetGunSwitchType() <= COMP_WEAPON_BAY_4_SW)
        {

            if (GetDOFValue(af->GetGunDofType()) > 0.0f)
                SetSwitch(af->GetGunSwitchType(), 1);
            else
                SetSwitch(af->GetGunSwitchType(), 0);
        }

        if (Sms)
        {
            // RV - Biker - Which DOF and switch to use
            int curHP = Sms->CurHardpoint();
            int curDOF = af->GetHpDofType(curHP);

            MoveDof(af->GetHpDofType(curHP), 90 * DTR, af->GetHpDofRate(curHP));

            for (int l = 0; l < Sms->NumHardpoints(); l++)
            {
                if (af->GetHpDofType(l) not_eq curDOF)
                    MoveDof(af->GetHpDofType(l), 0 * DTR, af->GetHpDofRate(l));

            }

            /* if (af->GetHpSwitchType(l) >= COMP_WEAPON_BAY_0_SW and 
             af->GetHpSwitchType(l) <= COMP_WEAPON_BAY_4_SW) {

             if(GetDOFValue(af->GetHpDofType(l)) > 0.0f)
             SetSwitch(af->GetHpSwitchType(l), 1);
             else
             SetSwitch(af->GetHpSwitchType(l), 0);
             }
            }*/
        }

        CalculateAileronAndFlap(qFactor, &aileronleft, &aileronrt, &flapleft, &flaprt);

        if (af->auxaeroData->hasFlapperons)
        {
            MoveDof(COMP_LT_FLAP, aileronleft + flapleft, af->auxaeroData->tefRate);
            MoveDof(COMP_RT_FLAP, aileronrt + flaprt,  af->auxaeroData->tefRate);
        }
        else
        {
            MoveDof(COMP_LT_FLAP, aileronleft, af->auxaeroData->animAileronRate); // MLR 2003-09-30 change because tefRate is to damn slow
            MoveDof(COMP_RT_FLAP, aileronrt, af->auxaeroData->animAileronRate);   // MLR 2003-09-30 same as above

            if (af->auxaeroData->hasTef == AUX_LEFTEF_MANUAL)
            {
                MoveDof(COMP_LT_TEF, flapleft, af->auxaeroData->tefRate, af->auxaeroData->sndFlapStart, af->auxaeroData->sndFlapLoop, af->auxaeroData->sndFlapEnd);
                MoveDof(COMP_RT_TEF, flaprt, af->auxaeroData->tefRate, af->auxaeroData->sndFlapStart, af->auxaeroData->sndFlapLoop, af->auxaeroData->sndFlapEnd);
            }
            else
            {
                MoveDof(COMP_LT_TEF, flapleft, af->auxaeroData->tefRate);
                MoveDof(COMP_RT_TEF, flaprt, af->auxaeroData->tefRate);
            }
        }

        // Thrust Reverser - FRB
        if (af->auxaeroData->hasThrRev)
        {
            if (OnGround() and af->thrustReverse == 2)
            {
                float thrpos = af->auxaeroData->animThrRevAngle * DTR;
                MoveDof(COMP_REVERSE_THRUSTER, thrpos, af->auxaeroData->animThrRevRate);
            }
            else // Close it
                MoveDof(COMP_REVERSE_THRUSTER, 0.0f, af->auxaeroData->animThrRevRate);
        }

        // Cobra - FRB animated pilot's head
        if ((g_bAnimPilotHead) and (( not IsPlayer()) or ((IsPlayer()) and ( not g_bEnableTrackIR))
                                   or ((IsPlayer()) and (g_bEnableTrackIR)
                                      and ((PlayerOptions.Get3dTrackIR() == false)
                                           or (OTWDriver.GetOTWDisplayMode() not_eq OTWDriverClass::Mode2DCockpit
                                              and OTWDriver.GetOTWDisplayMode() not_eq OTWDriverClass::Mode3DCockpit)))))
        {
            long  nHoldSec = 3;

            // Pilot animation timer
            if (SimLibElapsedTime > static_cast<SIM_ULONG>(af->AnimPilotTime))
            {
                if ( not af->IsSet(AirframeClass::InAir))
                {
                    af->AnimPilotScenario = 1;
                }

                af->AnimPilotAct ++;

                if (af->AnimPilotAct > af->maxAnimPilotActs)
                {
                    af->AnimPilotAct = af->PA_End;
                }

                af->AnimPilotTime = SimLibElapsedTime + (CampaignSeconds * nHoldSec);
            }

            // WSO/RIO/Copilot animation timer
            if (SimLibElapsedTime > static_cast<SIM_ULONG>(af->AnimWSOTime))
            {
                if ( not af->IsSet(AirframeClass::InAir))
                {
                    af->AnimWSOScenario = 1;
                }

                af->AnimWSOAct ++;

                if (af->AnimWSOAct > af->maxAnimPilotActs)
                {
                    af->AnimWSOAct = af->PA_End;
                }

                af->AnimWSOTime = SimLibElapsedTime + (CampaignSeconds * nHoldSec * 2);
            }

            if (af->AnimPilotAct)
            {
                switch (af->TheRoutine[af->AnimPilotScenario][af->AnimPilotAct])
                {
                    case af->PA_None:
                    case af->PA_End:
                        MoveDof(COMP_HEAD_LR, (0.0f * DTR), g_fPilotHeadMoveRate, -1);
                        MoveDof(COMP_HEAD_UD, (0.0f * DTR), g_fPilotHeadMoveRate, -1);
                        af->AnimPilotTime = static_cast<int>(SimLibElapsedTime + (g_fPilotActInterval * CampaignMinutes * PRANDFloatPos()));
                        af->AnimPilotAct = 0;
                        af->AnimPilotScenario = rand() % (af->maxAnimPilotScenarios - 1);

                        if (af->AnimPilotScenario >= af->maxAnimPilotScenarios)
                            af->AnimPilotScenario = 0;

                        break;

                    case af->PA_Forward:
                        MoveDof(COMP_HEAD_LR, (0.0f * DTR), g_fPilotHeadMoveRate, -1);
                        MoveDof(COMP_HEAD_UD, (0.0f * DTR), g_fPilotHeadMoveRate, -1);
                        break;

                    case af->PA_ForwardUp:
                        MoveDof(COMP_HEAD_LR, (0.0f * DTR), g_fPilotHeadMoveRate, -1);
                        MoveDof(COMP_HEAD_UD, (-40.0f * DTR), g_fPilotHeadMoveRate, -1);
                        break;

                    case af->PA_ForwardDown:
                        MoveDof(COMP_HEAD_LR, (0.0f * DTR), g_fPilotHeadMoveRate, -1);
                        MoveDof(COMP_HEAD_UD, (20.0f * DTR), g_fPilotHeadMoveRate, -1);
                        break;

                    case af->PA_Left:
                        MoveDof(COMP_HEAD_LR, (40.0f * DTR), g_fPilotHeadMoveRate, -1);
                        MoveDof(COMP_HEAD_UD, (0.0f * DTR), g_fPilotHeadMoveRate, -1);
                        break;

                    case af->PA_Right:
                        MoveDof(COMP_HEAD_LR, (-40.0f * DTR), g_fPilotHeadMoveRate, -1);
                        MoveDof(COMP_HEAD_UD, (0.0f * DTR), g_fPilotHeadMoveRate, -1);
                        break;

                    case af->PA_LeftBack:
                        MoveDof(COMP_HEAD_LR, (70.0f * DTR), g_fPilotHeadMoveRate, -1);
                        MoveDof(COMP_HEAD_UD, (0.0f * DTR), g_fPilotHeadMoveRate, -1);
                        break;

                    case af->PA_RightBack:
                        MoveDof(COMP_HEAD_LR, (-70.0f * DTR), g_fPilotHeadMoveRate, -1);
                        MoveDof(COMP_HEAD_UD, (0.0f * DTR), g_fPilotHeadMoveRate, -1);
                        break;

                    case af->PA_LeftBackUp:
                        MoveDof(COMP_HEAD_LR, (80.0f * DTR), g_fPilotHeadMoveRate, -1);
                        MoveDof(COMP_HEAD_UD, (-90.0f * DTR), g_fPilotHeadMoveRate, -1);
                        break;

                    case af->PA_RightBackUp:
                        MoveDof(COMP_HEAD_LR, (-80.0f * DTR), g_fPilotHeadMoveRate, -1);
                        MoveDof(COMP_HEAD_UD, (-90.0f * DTR), g_fPilotHeadMoveRate, -1);
                        break;

                    case af->PA_BackUp:
                        MoveDof(COMP_HEAD_LR, (-90.0f * DTR), g_fPilotHeadMoveRate, -1);
                        MoveDof(COMP_HEAD_UD, (-45.0f * DTR), g_fPilotHeadMoveRate, -1);
                        break;
                }
            } // end Pilot animation

            if (af->AnimWSOAct)
            {
                switch (af->TheRoutine[af->AnimWSOScenario][af->AnimWSOAct])
                {
                    case af->PA_None:
                    case af->PA_End:
                        MoveDof(COMP_HEAD2_LR, (0.0f * DTR), g_fPilotHeadMoveRate, -1);
                        MoveDof(COMP_HEAD2_UD, (0.0f * DTR), g_fPilotHeadMoveRate, -1);
                        af->AnimWSOTime = static_cast<int>(SimLibElapsedTime + (g_fPilotActInterval * CampaignMinutes * PRANDFloatPos()));
                        af->AnimWSOAct = 0;
                        af->AnimWSOScenario = rand() % (af->maxAnimPilotScenarios - 1);

                        if (af->AnimWSOScenario >= af->maxAnimPilotScenarios)
                            af->AnimWSOScenario = 0;

                        break;

                    case af->PA_Forward:
                        MoveDof(COMP_HEAD2_LR, (0.0f * DTR), g_fPilotHeadMoveRate, -1);
                        MoveDof(COMP_HEAD2_UD, (0.0f * DTR), g_fPilotHeadMoveRate, -1);
                        break;

                    case af->PA_ForwardUp:
                        MoveDof(COMP_HEAD2_LR, (20.0f * DTR), g_fPilotHeadMoveRate, -1);
                        MoveDof(COMP_HEAD2_UD, (0.0f * DTR), g_fPilotHeadMoveRate, -1);
                        break;

                    case af->PA_ForwardDown:
                        MoveDof(COMP_HEAD2_LR, (0.0f * DTR), g_fPilotHeadMoveRate, -1);
                        MoveDof(COMP_HEAD2_UD, (-10.0f * DTR), g_fPilotHeadMoveRate, -1);
                        break;

                    case af->PA_Left:
                        MoveDof(COMP_HEAD2_LR, (-20.0f * DTR), g_fPilotHeadMoveRate, -1);
                        MoveDof(COMP_HEAD2_UD, (0.0f * DTR), g_fPilotHeadMoveRate, -1);
                        break;

                    case af->PA_Right:
                        MoveDof(COMP_HEAD2_LR, (0.0f * DTR), g_fPilotHeadMoveRate, -1);
                        MoveDof(COMP_HEAD2_UD, (-30.0f * DTR), g_fPilotHeadMoveRate, -1);
                        break;

                    case af->PA_LeftBack:
                        MoveDof(COMP_HEAD2_LR, (40.0f * DTR), g_fPilotHeadMoveRate, -1);
                        MoveDof(COMP_HEAD2_UD, (0.0f * DTR), g_fPilotHeadMoveRate, -1);
                        break;

                    case af->PA_RightBack:
                        MoveDof(COMP_HEAD2_LR, (-40.0f * DTR), g_fPilotHeadMoveRate, -1);
                        MoveDof(COMP_HEAD2_UD, (-30.0f * DTR), g_fPilotHeadMoveRate, -1);
                        break;

                    case af->PA_LeftBackUp:
                        MoveDof(COMP_HEAD2_LR, (40.0f * DTR), g_fPilotHeadMoveRate, -1);
                        MoveDof(COMP_HEAD2_UD, (-20.0f * DTR), g_fPilotHeadMoveRate, -1);
                        break;

                    case af->PA_RightBackUp:
                        MoveDof(COMP_HEAD2_LR, (-20.0f * DTR), g_fPilotHeadMoveRate, -1);
                        MoveDof(COMP_HEAD2_UD, (-40.0f * DTR), g_fPilotHeadMoveRate, -1);
                        break;

                    case af->PA_BackUp:
                        MoveDof(COMP_HEAD2_LR, (80.0f * DTR), g_fPilotHeadMoveRate, -1);
                        MoveDof(COMP_HEAD2_UD, (-20.0f * DTR), g_fPilotHeadMoveRate, -1);
                        break;
                }
            } // end WSO animation
        }

        // Pilot animation tied to stick
        if (g_bAnimPilotHead and af->AnimPilotAct == 0)
        {

            if (( not g_bEnableTrackIR) or ( not IsPlayer()) or ((g_bEnableTrackIR) and ( not PlayerOptions.Get3dTrackIR())))
            {
                if ((af->rstick > -0.1f) and (af->rstick < 0.1f))
                {
                    MoveDof(COMP_HEAD_LR, (0.0f * DTR), g_fPilotHeadMoveRate, -1);
                    MoveDof(COMP_HEAD2_LR, (0.0f * DTR), g_fPilotHeadMoveRate, -1);
                }
                else
                {
                    MoveDof(COMP_HEAD_LR, (-af->rstick * 120.0f * DTR), g_fPilotHeadMoveRate, -1);
                    MoveDof(COMP_HEAD2_LR, (-af->rstick * 120.0f * DTR), g_fPilotHeadMoveRate, -1);
                }

                if ((af->pstick > -0.1f) and (af->pstick < 0.1f))
                {
                    MoveDof(COMP_HEAD_UD, (0.0f * DTR), g_fPilotHeadMoveRate, -1);
                    MoveDof(COMP_HEAD2_UD, (0.0f * DTR), g_fPilotHeadMoveRate, -1);
                }
                else
                {
                    MoveDof(COMP_HEAD_UD, (-af->pstick * 100.0f * DTR), g_fPilotHeadMoveRate, -1);
                    MoveDof(COMP_HEAD2_UD, (-af->pstick * 100.0f * DTR), g_fPilotHeadMoveRate, -1);
                }
            }
            else
            {
                // Head TRACK IR Control
                //SetDOF( COMP_HEAD_LR, -theTrackIRObject.getYaw());
                //SetDOF( COMP_HEAD2_LR, -theTrackIRObject.getYaw());
                //SetDOF( COMP_HEAD_UD, theTrackIRObject.getPitch());
                //SetDOF( COMP_HEAD2_UD, theTrackIRObject.getPitch());
                MoveDof(COMP_HEAD_LR, theTrackIRObject.getYaw(), g_fPilotHeadMoveRate, -1);
                MoveDof(COMP_HEAD2_LR, theTrackIRObject.getYaw(), g_fPilotHeadMoveRate, -1);
                MoveDof(COMP_HEAD_UD, theTrackIRObject.getPitch(), g_fPilotHeadMoveRate, -1);
                MoveDof(COMP_HEAD2_UD, theTrackIRObject.getPitch(), g_fPilotHeadMoveRate, -1);

            }
        }

        // end Pilot head animation

        {
            // MLR - 2003-10-04 Tail hook
            float hookpos = 0.0f;

            if (af->IsSet(AirframeClass::Hook))
            {
                // RV - Biker - Do not lower hook when on carrier deck
                if (af->IsSet(AirframeClass::OnObject))
                    hookpos = 0.0f;
                else
                    hookpos = af->auxaeroData->animHookAngle * DTR;
            }

            MoveDof(COMP_TAILHOOK, hookpos, af->auxaeroData->animHookRate);
        }

        {
            // MLR - 2003-09-30 Spoiler code
            //TJL 01/04/04 adding wingSweep; //Cobra 10/30/04 TJL
            float spoiler1l, spoiler1r, spoiler2l, spoiler2r, sweep;
            CalculateSweepAndSpoiler(sweep, spoiler1l, spoiler1r, spoiler2l, spoiler2r);
            MoveDof(COMP_LT_SPOILER1, spoiler1l, af->auxaeroData->animSpoiler1Rate);
            MoveDof(COMP_RT_SPOILER1, spoiler1r, af->auxaeroData->animSpoiler1Rate);
            MoveDof(COMP_LT_SPOILER2, spoiler2l, af->auxaeroData->animSpoiler2Rate);
            MoveDof(COMP_RT_SPOILER2, spoiler2r, af->auxaeroData->animSpoiler2Rate);
            MoveDof(COMP_SWING_WING,  sweep,     af->auxaeroData->animSwingWingRate);
            wingSweep = sweep;
        }
        //Cobra 10/30/04 TJL
        {
            // intake ramps
            int l;

            for (l = 0; l < 3; l++)
            {
                MoveDof(COMP_INTAKE_1_RAMP_1 + l, af->auxaeroData->animIntakeRamp[0].table.Lookup(af->alpha, af->mach), af->auxaeroData->animIntakeRamp[0].Rate * DTR);
            }

            if (af->auxaeroData->nEngines == 2)
            {
                for (l = 0; l < 3; l++)
                {
                    MoveDof(COMP_INTAKE_2_RAMP_1 + l, af->auxaeroData->animIntakeRamp[0].table.Lookup(af->alpha, af->mach), af->auxaeroData->animIntakeRamp[0].Rate * DTR);
                }
            }

        }


        // Rudder
        stabAngle = -CalculateRudder(qFactor);
        SetDOF(COMP_RUDDER, stabAngle);

        // LEF
        if (af->auxaeroData->hasLef)
        {

            CalculateLef(qFactor);

            MoveDof(COMP_LT_LEF, leftLEFAngle, af->auxaeroData->lefRate);
            MoveDof(COMP_RT_LEF, rightLEFAngle, af->auxaeroData->lefRate);
        }

        // set light stuff
        RunLightSurfaces();
        // Set gear stuff
        RunGearSurfaces();


        {
            // MLR - Exhaust Nozzle DOF
            float nozpos;

            if (rpm[0] < 1.0F) // Mil or less
            {
                float diff;
                nozpos = (rpm[0] - 0.8f) * 5.f;

                if (nozpos < 0)
                    nozpos = 0;

                // noz is 0 - 1.0
                diff = af->auxaeroData->animExhNozMil - af->auxaeroData->animExhNozIdle;
                nozpos = (af->auxaeroData->animExhNozIdle + diff * nozpos) * DTR;
            }
            else
            {
                float diff;
                nozpos = (rpm[0] - 1.0f) * 50.f;

                if (nozpos > 1.0)
                    nozpos = 1.0;

                // noz is 0 - 1.0
                diff = af->auxaeroData->animExhNozAB - af->auxaeroData->animExhNozMil;
                nozpos = (af->auxaeroData->animExhNozMil + diff * nozpos) * DTR;
            }

            MoveDof(COMP_EXH_NOZ, nozpos, af->auxaeroData->animExhNozRate);
        }

        {
            // MLR This DOF is intended to be used with the BScaleNode
            float abscale = 0;

            if (rpm[0] > 1.0f)
            {
                abscale = (rpm[0] - 1.0f) * 33.0f;

                if (abscale > 1.0)
                    abscale = 1.0;
            }

            SetDOF(COMP_ABDOF, abscale); // only goes from 0 - 1.0
        }




        //need to reimplement using the GetAfterburnerStage()
        //   since there isn't enough granularity in the powerOutput to show the noz
        //   correctly
        // Nozzle position
        {
            int ab = 0;

            if (rpm[0] < 1.0F) // Mil or less
            {
                stage = FloatToInt32((rpm[0] - 0.7F) * 13.3333F);
            }
            else
            {
                stage = FloatToInt32((rpm[0] - 1.0F) * 233.0F) + 4;
                ab = 11;
            }

            stage = max(0, stage);


            if ((rpm[0] > 1.0F) not_eq GetSwitch(COMP_AB))
            {

                SetSwitch(COMP_AB, (rpm[0] > 1.0F));
            }

            // Afterbuner cone
            if (rpm[0] >= 1.0F)
            {
                for (i = 0; i < 6; i++)
                {
                    VertexData[i] = max(0.0F, 18.0F - 600.0F * (rpm[0] - 1.0F));
                }
            }

            if (1 << stage not_eq GetSwitch(COMP_EXH_NOZZLE))
            {
                SetSwitch(COMP_EXH_NOZZLE, 1 << stage);
            }

            if (af->auxaeroData->nEngines == 2)
            {
                // MLR - Exhaust Nozzle DOF

                float nozpos;

                if (rpm[1] < 1.0F) // Mil or less
                {
                    float diff;
                    nozpos = (rpm[1] - 0.8f) * 5.0f;

                    if (nozpos < 0)
                        nozpos = 0;

                    // noz is 0 - 1.0
                    diff = af->auxaeroData->animExhNozMil - af->auxaeroData->animExhNozIdle;
                    nozpos = (af->auxaeroData->animExhNozIdle + diff * nozpos) * DTR;
                }
                else
                {
                    float diff;
                    nozpos = (rpm[1] - 1.0f) * 50.0f;

                    if (nozpos > 1.0f)
                        nozpos = 1.0f;

                    // noz is 0 - 1.0
                    diff = af->auxaeroData->animExhNozAB - af->auxaeroData->animExhNozMil;
                    nozpos = (af->auxaeroData->animExhNozMil + diff * nozpos) * DTR;
                }

                MoveDof(COMP_EXH_NOZ2, nozpos, af->auxaeroData->animExhNozRate);

                {
                    // MLR This DOF is intended to be used with the BScaleNode
                    float abscale = 0;

                    if (rpm[1] > 1.0f)
                    {
                        abscale = (rpm[1] - 1.0f) * 33.0f;

                        if (abscale > 1.0f)
                            abscale = 1.0f;
                    }

                    SetDOF(COMP_ABDOF2, abscale); // only goes from 0 - 1.0
                }

                SetSwitch(COMP_AB2, (rpm[1] > 1.0F));

                if (rpm[1] < 1.0F) // Mil or less
                {
                    stage = FloatToInt32((rpm[1] - 0.7F) * 13.3333F);
                }
                else
                {
                    stage = FloatToInt32((rpm[1] - 1.0F) * 233.0F) + 4;
                }

                stage = max(0, stage);

                SetSwitch(COMP_EXH_NOZZLE2, 1 << stage);
            }
            else
            {
                SetDOF(COMP_EXH_NOZ2, GetDOFValue(COMP_EXH_NOZ));
                SetDOF(COMP_ABDOF2, GetDOFValue(COMP_ABDOF));
                SetSwitch(COMP_AB2, GetSwitch(COMP_AB));
                SetSwitch(COMP_EXH_NOZZLE2, GetSwitch(COMP_EXH_NOZZLE));
            }
        }


        if (IsLocal())
        {
            if (af->dbrake < 0.4F)
            {
                UnSetFlag(AIR_BRAKES_OUT);
            }
            else if (af->dbrake > 0.6F)
            {
                SetFlag(AIR_BRAKES_OUT);
            }
        }
        else
        {
            if (IsSetFlag(AIR_BRAKES_OUT))
            {
                af->dbrake = 1.0;
            }
            else
            {
                af->dbrake = 0.0;
            }
        }

        stabAngle = af->dbrake * af->auxaeroData->airbrakeMaxAngle * DTR;

        SetDOF(COMP_LT_AIR_BRAKE_TOP, stabAngle);
        SetDOF(COMP_LT_AIR_BRAKE_BOT, stabAngle);
        SetDOF(COMP_RT_AIR_BRAKE_TOP, stabAngle);
        SetDOF(COMP_RT_AIR_BRAKE_BOT, stabAngle);

        gFact = min(7, max(0, FloatToInt32((af->nzcgb - 4.0F) * 1.5F)));

        if (gFact not_eq GetSwitch(COMP_WING_VAPOR))
        {
            // RV - I-Hawk - the switch is used only if new vortex code isn't used
            // or if the y position of the first (and basic) vortex trail is set to 0
            // (or not defind as for sure if defined correctly, y position must be not_eq then 0)

            bool vortexTrailsCondition = false;

#if NEW_VORTEX_TRAILS //I-Hawk, if Defined at aircraft.h

            vortexTrailsCondition = true;

#endif

            float vortexTrailsUsed = af->auxaeroData->vortex1Location.y;

            if (( not vortexTrailsCondition) or ( not vortexTrailsUsed))
            {
                SetSwitch(COMP_WING_VAPOR, gFact);
            }
        }

        if (af->IsEngineFlag(AirframeClass::FuelDoorOpen) not_eq GetSwitch(COMP_REFUEL_DR))
        {

            SetSwitch(COMP_REFUEL_DR, af->IsEngineFlag(AirframeClass::FuelDoorOpen));
        }

        if (af->auxaeroData->dragChuteCd > 0)
            DeployDragChute(COMP_DRAGCHUTE);

        if (IsLocal())
        {
            if (af->canopyState)
            {
                // canopy open
                MoveDof(COMP_CANOPY_DOF, af->auxaeroData->canopyMaxAngle * DTR, af->auxaeroData->canopyRate,
                        af->auxaeroData->sndCanopyOpenStart,
                        af->auxaeroData->sndCanopyLoop, af->auxaeroData->sndCanopyOpenEnd
                       );
                SetAcStatusBits(ACSTATUS_CANOPY); //2004-03-23 Booster
            }
            else
            {
                // canopy shut
                MoveDof(
                    COMP_CANOPY_DOF, 0, af->auxaeroData->canopyRate,
                    af->auxaeroData->sndCanopyCloseStart,
                    af->auxaeroData->sndCanopyLoop, af->auxaeroData->sndCanopyCloseEnd
                );
                ClearAcStatusBits(ACSTATUS_CANOPY);  //2004-03-23 Booster
            }
        }
        else
        {
            if (IsAcStatusBitsSet(ACSTATUS_CANOPY))  // 2004-03-23 Booster - Canopy open/close
            {
                MoveDof(COMP_CANOPY_DOF, af->auxaeroData->canopyMaxAngle * DTR, af->auxaeroData->canopyRate);
            }
            else
            {
                MoveDof(COMP_CANOPY_DOF, 0, af->auxaeroData->canopyRate);
            }
        }


    }
    else // Simple Model
    {
        float leftStab, rightStab;
        CalculateStab(qFactor, &leftStab, &rightStab);
        SetDOF(SIMP_LT_STAB, -leftStab);
        SetDOF(SIMP_RT_STAB, -rightStab);

        float aileronleft, aileronrt;
        float flapleft, flaprt;
        CalculateAileronAndFlap(qFactor, &aileronleft, &aileronrt, &flapleft, &flaprt);

        //      stabAngle = af->rstick * qFactor * af->auxaeroData->aileronMaxAngle * DTR;
        if (af->auxaeroData->hasFlapperons)
        {
            SetDOF(SIMP_LT_AILERON, aileronleft + flapleft);
            SetDOF(SIMP_RT_AILERON, aileronrt + flaprt);
        }
        else
        {
            SetDOF(SIMP_LT_AILERON, aileronleft);
            SetDOF(SIMP_RT_AILERON, aileronrt);

            if (af->auxaeroData->hasTef == AUX_LEFTEF_MANUAL)
            {
                MoveDof(SIMP_LT_TEF, flapleft, af->auxaeroData->tefRate, af->auxaeroData->sndFlapStart, af->auxaeroData->sndFlapLoop, af->auxaeroData->sndFlapEnd);
                MoveDof(SIMP_RT_TEF, flaprt, af->auxaeroData->tefRate, af->auxaeroData->sndFlapStart, af->auxaeroData->sndFlapLoop, af->auxaeroData->sndFlapEnd);
            }
            else
            {
                MoveDof(SIMP_LT_TEF, flapleft, af->auxaeroData->tefRate);
                MoveDof(SIMP_RT_TEF, flaprt, af->auxaeroData->tefRate);
            }
        }

        stabAngle = CalculateRudder(qFactor);
        SetDOF(SIMP_RUDDER_1, stabAngle);
        SetDOF(SIMP_RUDDER_2, stabAngle);

        stabAngle = af->dbrake * af->auxaeroData->airbrakeMaxAngle * DTR;
        SetDOF(SIMP_AIR_BRAKE, stabAngle);

        if (af->auxaeroData->hasLef)
        {
            CalculateLef(qFactor);
            MoveDof(SIMP_LT_LEF, leftLEFAngle, af->auxaeroData->lefRate, -1);
            MoveDof(SIMP_RT_LEF, rightLEFAngle, af->auxaeroData->lefRate, -1);
        }

        if ((rpm[0] > 1.0F) not_eq GetSwitch(SIMP_AB))
        {

            SetSwitch(SIMP_AB, rpm[0] > 1.0F);
        }

        if ((af->gearPos > 0.5F) not_eq GetSwitch(SIMP_GEAR))
        {

            SetSwitch(SIMP_GEAR, af->gearPos > 0.5F);
        }

        gFact = min(7, max(0, FloatToInt32((af->nzcgb - 4.0F) * 1.5F)));

        if (gFact not_eq GetSwitch(SIMP_WING_VAPOR))
        {

            SetSwitch(SIMP_WING_VAPOR, gFact);
        }





        {
            // MLR - 2003-09-30 Spoiler bitand Wing Sweep code
            //TJL 01/04/04 adding wingSweep;
            float spoiler1l, spoiler1r, spoiler2l, spoiler2r, sweep;
            CalculateSweepAndSpoiler(sweep, spoiler1l, spoiler1r, spoiler2l, spoiler2r);
            MoveDof(SIMP_LT_SPOILER1, spoiler1l, af->auxaeroData->animSpoiler1Rate);
            MoveDof(SIMP_RT_SPOILER1, spoiler1r, af->auxaeroData->animSpoiler1Rate);
            MoveDof(SIMP_LT_SPOILER2, spoiler2l, af->auxaeroData->animSpoiler2Rate);
            MoveDof(SIMP_RT_SPOILER2, spoiler2r, af->auxaeroData->animSpoiler2Rate);
            wingSweep = sweep;

            if (acFlags bitand hasSwing)
            {
                static const int swdofs[] =
                {
                    SIMP_SWING_WING_1, SIMP_SWING_WING_2, SIMP_SWING_WING_3,
                    SIMP_SWING_WING_4, SIMP_SWING_WING_5, SIMP_SWING_WING_6,
                    SIMP_SWING_WING_7, SIMP_SWING_WING_8
                };

                MoveDof(SIMP_SWING_WING_1, sweep, 5.0f);

                for (int i = 1; i < sizeof(swdofs) / sizeof(swdofs[0]); i++)
                    SetDOF(swdofs[i], GetDOFValue(SIMP_SWING_WING_1));
            }
        }




        if (af->auxaeroData->dragChuteCd > 0)
            DeployDragChute(SIMP_DRAGCHUTE);

    }

    // Check for stick shake
    if (this == SimDriver.GetPlayerEntity())
    {
        if (GetAlpha() > 15.0F and GetAlpha() < 20.0F)
        {
            if ( not stallShake)
            {
                if (this->AutopilotType() not_eq AircraftClass::CombatAP) // Retro 20Feb2004.. it can STOP regardless of AP status however..
                {
                    stallShake = TRUE;
                    JoystickPlayEffect(JoyStall1, 0);
                    JoystickPlayEffect(JoyStall2, 0);
                }
            }
        }
        else if (stallShake)
        {
            stallShake = FALSE;
            JoystickStopEffect(JoyStall1);
            JoystickStopEffect(JoyStall2);
        }
    }


    // this could be optimized
    if (gACMIRec.IsRecording())
    {
        acmiTimer -= SimLibMajorFrameTime;

        if (acmiTimer <= 0)
        {
            acmiTimer = g_fACMIAnimRecordTimer;

            int l, dofs, switches;
            dofs = ((DrawableBSP *)this->drawPointer)->GetNumDOFs();
            switches = ((DrawableBSP *)this->drawPointer)->GetNumSwitches();

            if (dofs >= COMP_MAX_DOF)
                dofs = COMP_MAX_DOF - 1;

            if (switches >= COMP_MAX_SWITCH)
                switches = COMP_MAX_SWITCH - 1;

            acmiSwitch.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
            acmiSwitch.data.type = Type();
            acmiSwitch.data.uniqueID = ACMIIDTable->Add(Id(), NULL, 0); //.num_;

            DOFRec.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
            DOFRec.data.type = Type();
            DOFRec.data.uniqueID = ACMIIDTable->Add(Id(), NULL, 0); //.num_;

            for (l = 0; l < COMP_MAX_DOF; l++)
            {
                float d;

                d = GetDOFValue(l);

                if (((int)(acmiDOFValue[l] * 360)) not_eq ((int)(d * 360)))
                {
                    DOFRec.data.DOFNum = l;
                    DOFRec.data.DOFVal = d;
                    DOFRec.data.prevDOFVal = acmiDOFValue[l];
                    acmiDOFValue[l] = d;
                    gACMIRec.DOFRecord(&DOFRec);
                }
            }

            for (l = 0; l < switches; l++)
            {
                int s;

                s = GetSwitch(l);

                if (acmiSwitchValue[l] not_eq s)
                {
                    acmiSwitch.data.switchNum   = l;
                    acmiSwitch.data.switchVal   = s;
                    acmiSwitch.data.prevSwitchVal = acmiSwitchValue[l];
                    acmiSwitchValue[l] = s;
                    gACMIRec.SwitchRecord(&acmiSwitch);
                }
            }
        }
    }


}

void AircraftClass::RunLightSurfaces(void)
{
    //SetSwitch(COMP_NAV_LIGHTS, light == 1);
    //SetSwitch(COMP_NAV_LIGHTSFLASH, light == 2);
    //SetSwitch(COMP_TAIL_STROBE, light == 3);
    //SetSwitch(COMP_LAND_LIGHTS, light == 4);

    if ( not ExtlState(Extl_Main_Power))
    {
        // lights off
        SetSwitch(COMP_NAV_LIGHTS, FALSE);
        SetSwitch(COMP_TAIL_STROBE, FALSE);
        SetSwitch(COMP_LAND_LIGHTS, FALSE);
        //ExtlOff(AircraftClass::ExtlLightFlags::Extl_Anti_Coll);
        //ExtlOff(AircraftClass::ExtlLightFlags::Extl_Wing_Tail);
    }
    else
    {
        // check flags
        // gear: landing lights on and gear down
        SetSwitch(COMP_LAND_LIGHTS, IsAcStatusBitsSet(ACSTATUS_EXT_LANDINGLIGHT) and (af->gearPos == 1.0F));

        //----------------------------
        // animWingFlashOnTime  0.4 * 1000.0f = 400
        // animWingFlashOffTime  0.5 * 1000.0f = 500
        //----------------------------
        if (af->auxaeroData->animWingFlashOnTime == 0.0f)
            af->auxaeroData->animWingFlashOnTime = 0.4f;

        if (af->auxaeroData->animWingFlashOffTime == 0.0f)
            af->auxaeroData->animWingFlashOffTime = 0.5f;

        // nav: if steady, set on
        int navState = FALSE;

        if (ExtlState(Extl_Wing_Tail))
        {
            VU_TIME FlashOff = (VU_TIME)(af->auxaeroData->animWingFlashOffTime * 1000.0f);

            navState = TRUE;

            if (ExtlState(Extl_Flash))
            {
                navState = GetSwitch(COMP_NAV_LIGHTS);

                // if timer not initialized or timed out, give it a random start
                if (SimLibElapsedTime >= animWingFlashTimer)
                {
                    //if(SimLibElapsedTime - animWingFlashTimer > 3000) animWingFlashTimer = SimLibElapsedTime + (rand() % 3000);
                    if ((SimLibElapsedTime - animWingFlashTimer) > FlashOff)
                        animWingFlashTimer = SimLibElapsedTime + (rand() % FlashOff);
                }
                else
                {
                    //if(animWingFlashTimer - SimLibElapsedTime > 3000) animWingFlashTimer = SimLibElapsedTime + (rand() % 3000);
                    if ((animWingFlashTimer - SimLibElapsedTime) > FlashOff)
                        animWingFlashTimer = SimLibElapsedTime + (rand() % FlashOff);
                }

                // make it blink
                if (SimLibElapsedTime > animWingFlashTimer)
                {
                    // blink time is small. random factor to avoid all blinking together
                    navState = not navState;

                    if (navState)
                    {
                        //animWingFlashTimer += 75;
                        animWingFlashTimer += (VU_TIME)(af->auxaeroData->animWingFlashOnTime * 1000.0f);
                    }
                    else
                    {
                        //animWingFlashTimer += 3000;
                        animWingFlashTimer += FlashOff;
                    }
                }
            } // Flashing
            else
            {
                navState = TRUE; // Steady
            }

            // nav is controlling wing
            SetSwitch(COMP_NAV_LIGHTS, navState);
        }
        else
            SetSwitch(COMP_NAV_LIGHTS, 0); // Wing lights OFF

        //********************************************************
        // FRB - Tail strobe support
        // animStrobeOnTime  0.08 * 1000.0f = 80
        // animStrobeOffTime  2.0 * 1000.0f = 2000
        //-----------------------------
        if (ExtlState(Extl_Anti_Coll)) // Tail strobe always flashing
        {
            if (af->auxaeroData->animStrobeOnTime == 0.0f)
                af->auxaeroData->animStrobeOnTime = 0.08f;

            if (af->auxaeroData->animStrobeOffTime == 0.0f)
                af->auxaeroData->animStrobeOffTime = 2.0f;

            VU_TIME FlashOff = (VU_TIME)(af->auxaeroData->animStrobeOffTime * 1000.0f);
            int strobeState = FALSE;
            strobeState = GetSwitch(COMP_TAIL_STROBE);

            if (SimLibElapsedTime >= animStrobeTimer)
            {
                //if(SimLibElapsedTime - animStrobeTimer > 1500) animStrobeTimer = SimLibElapsedTime + (rand() % 1500);
                if ((SimLibElapsedTime - animStrobeTimer) > FlashOff)
                    animStrobeTimer = SimLibElapsedTime + (rand() % FlashOff);
            }
            else
            {
                //if(animStrobeTimer - SimLibElapsedTime > 1500) animStrobeTimer = SimLibElapsedTime + (rand() % 1500);
                if ((animStrobeTimer - SimLibElapsedTime) > FlashOff)
                    animStrobeTimer = SimLibElapsedTime + (rand() % FlashOff);
            }

            // make it blink
            if (SimLibElapsedTime > animStrobeTimer)
            {
                // blink time is small. random factor to avoid all blinking together
                strobeState = not strobeState;

                if (strobeState)
                {
                    animStrobeTimer += (VU_TIME)(af->auxaeroData->animStrobeOnTime * 1000.0f);
                }
                else
                {
                    animStrobeTimer += FlashOff;
                }
            }

            SetSwitch(COMP_TAIL_STROBE, strobeState);
        }
        else
            SetSwitch(COMP_TAIL_STROBE, 0); // Tail strobe OFF
    } // Light Power
}

void AircraftClass::RunGearSurfaces(void)
{
    // sfr: this is called in move surface, responsible for moving
    // surfaces related to landing gears.
    // the run landing gear function makes the computations needed
    int numgear;
    //this function is only valid for airplanes with complex gear
    int i;

    // MLR 2003-10-04  must limit the original MPS code to animating 3 gear bitand door sets,
    // there are no DOFs for the rest
    numgear = af->NumGear();

    if (numgear > 8) numgear = 8; // MLR 2/22/2004 - the limit is now 8

    if (IsLocal())
    {
        if ((af->gearHandle > 0.0F or OnGround()) and not af->IsSet(AirframeClass::GearBroken))
        {
            SetAcStatusBits(ACSTATUS_GEAR_DOWN);
        }
        else
        {
            ClearAcStatusBits(ACSTATUS_GEAR_DOWN);
        }
    }

    if (af->auxaeroData->animWheelRadius[0])
    {
        // MLR 2003-10-04 animate wheels.
        // FF only supports 3 gears currently (2003-10-04)
        int ng = af->NumGear();

        if (ng > 8) ng = 8;

        for (i = 0; i < ng; i++)
        {
            //af->gear[i].WheelAngle+=af->gear[i].WheelRPS * SimLibMajorFrameTime;
            //af->gear[i].WheelAngle=fmod(af->gear[i].WheelAngle,PI*2); // MLR 1/6/2004 -
            // sfr: does not belong here, moved to RunLangindGear
            //af->gear[i].WheelRPS *= (1 - .4f * SimLibMajorFrameTime); // slows wheel down
            SetDOF(COMP_WHEEL_1 + i, af->gear[i].WheelAngle);
            SetDOF(COMP_GEAREXTENSION_1 + i, af->gear[i].StrutExtension);
        }
    }

    for (i = 0; i < numgear; i++)
    {
        //move the door
        if ( not (af->gear[i].flags bitand GearData::DoorStuck) and not (af->gear[i].flags bitand GearData::DoorBroken))
        {
            float pos = af->gearPos * 2;

            if (pos > 1.0)
                pos = 1.0;

            // MLR 2/22/2004 -
            SetDOF(ComplexGearDoorDOF[i], pos * af->GetAeroData(AeroDataSet::NosGearRng + i * 4) * DTR);
        }
        else
            SetDOF(ComplexGearDoorDOF[i], af->GetAeroData(AeroDataSet::NosGearRng) * DTR);

        //move the gear
        if ( not (af->gear[i].flags bitand GearData::GearStuck) and not (af->gear[i].flags bitand GearData::GearBroken))
        {
            float pos = (af->gearPos - .5f) * 2;

            if (pos < 0.0)
                pos = 0.0;

            // MLR 2/22/2004 -
            SetDOF(ComplexGearDOF[i], pos * af->GetAeroData(AeroDataSet::NosGearRng + i * 4) * DTR);
        }
        else
            SetDOF(ComplexGearDOF[i], af->GetAeroData(AeroDataSet::NosGearRng) * 0.6f * DTR);

        if (af->gearPos >= 0.9F and ((af->gear[i].flags bitand GearData::DoorBroken)
                                    or (af->gear[i].flags bitand GearData::DoorStuck)
                                    or (af->gear[i].flags bitand GearData::GearStuck)
                                    or (af->gear[i].flags bitand GearData::GearBroken)))
        {
            if ((af->gear[i].flags bitand GearData::DoorBroken) or (af->gear[i].flags bitand GearData::DoorStuck))
                SetSwitch(ComplexGearDoorSwitch[i], TRUE);

            if ((af->gear[i].flags bitand GearData::DoorBroken) or (af->gear[i].flags bitand GearData::DoorStuck))
                SetSwitch(ComplexGearHoleSwitch[i], TRUE);

            if ((af->gear[i].flags bitand GearData::GearBroken) or (af->gear[i].flags bitand GearData::GearStuck))
                SetSwitch(ComplexGearSwitch[i], TRUE);
        }
        else
        {
            if (GetDOFValue(ComplexGearDoorDOF[i]) < 5.0F * DTR)
                SetSwitch(ComplexGearDoorSwitch[i], FALSE);
            else
                SetSwitch(ComplexGearDoorSwitch[i], TRUE);

            if (GetDOFValue(ComplexGearDoorDOF[i]) > 5.0F * DTR)
                SetSwitch(ComplexGearHoleSwitch[i], TRUE);
            else
                SetSwitch(ComplexGearHoleSwitch[i], FALSE);

            if (GetDOFValue(ComplexGearDOF[i]) > 5.0F * DTR)
                SetSwitch(ComplexGearSwitch[i], TRUE);
            else
                SetSwitch(ComplexGearSwitch[i], FALSE);
        }
    }

    if (af->IsSet(AirframeClass::NoseSteerOn))
    {
        if ( not (af->gear[0].flags bitand GearData::GearStuck))
        {
            // ASSOCIATOR 30/11/03 Added g_bRollLinkedNWSRudder for roll unlinked rudder on the ground
            // RAS 05Apr04 chanded ypedal to lastYPedal and rstick to lastRStick so that nosewheel will track movement of plane
            // lastRStick and lastYPedal defined in EOM.cpp
            // RAS 06Apr04 changed 30.0F to 50.0F to make graphical nose wheel match rate of turn.  Acutal turn radius needs to be
            // looked at.  Real F-16 nose wheel turns 32.0 degrees
            if (IO.AnalogIsUsed(AXIS_YAW) and not af->IsSet(AirframeClass::IsDigital) or not g_bRollLinkedNWSRudder)  // Retro 31Dec2003
            {
                SetDOF(COMP_NOS_GEAR_ROT, -af->lastYPedal * 50.0F * DTR * (0.5F + (80.0F * KNOTS_TO_FTPSEC - af->vt) / (160.0F * KNOTS_TO_FTPSEC)));
            }
            else
            {
                if (fabs(af->lastRStick) > fabs(af->lastYPedal))   // ASSOCIATOR: Added check so that we can use rudder keys and stick
                {
                    SetDOF(COMP_NOS_GEAR_ROT, af->lastRStick * 50.0F * DTR * (0.5F + (80.0F * KNOTS_TO_FTPSEC - af->vt) / (160.0F * KNOTS_TO_FTPSEC)));
                }
                else
                {
                    SetDOF(COMP_NOS_GEAR_ROT, -af->lastYPedal * 50.0F * DTR * (0.5F + (80.0F * KNOTS_TO_FTPSEC - af->vt) / (160.0F * KNOTS_TO_FTPSEC)));
                }
            }
        }
    }
    else
        SetDOF(COMP_NOS_GEAR_ROT, GetDOFValue(COMP_NOS_GEAR_ROT) * 0.9F);

    if (GetDOFValue(ComplexGearDOF[0]) == af->GetAeroData(AeroDataSet::NosGearRng)*DTR and not (af->gear[0].flags bitand GearData::DoorBroken))
        SetSwitch(COMP_NOS_GEAR_ROD, TRUE);
    else
        SetSwitch(COMP_NOS_GEAR_ROD, FALSE);


#if 0

    if (gACMIRec.IsRecording())
    {
        acmiSwitch.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
        acmiSwitch.data.type = Type();
        acmiSwitch.data.uniqueID = ACMIIDTable->Add(Id(), NULL, 0); //.num_;
        DOFRec.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
        DOFRec.data.type = Type();
        DOFRec.data.uniqueID = ACMIIDTable->Add(Id(), NULL, 0); //.num_;

        if (switch1 not_eq GetSwitch(COMP_NOS_GEAR_SW))
        {
            acmiSwitch.data.switchNum = COMP_NOS_GEAR_SW;
            acmiSwitch.data.switchVal = GetSwitch(COMP_NOS_GEAR_SW);
            acmiSwitch.data.prevSwitchVal = switch1;
            gACMIRec.SwitchRecord(&acmiSwitch);
        }

        if (switch2 not_eq GetSwitch(COMP_LT_GEAR_SW))
        {
            acmiSwitch.data.switchNum = COMP_LT_GEAR_SW;
            acmiSwitch.data.switchVal = GetSwitch(COMP_LT_GEAR_SW);
            acmiSwitch.data.prevSwitchVal = switch2;
            gACMIRec.SwitchRecord(&acmiSwitch);
        }

        if (switch3 not_eq GetSwitch(COMP_RT_GEAR_SW))
        {
            acmiSwitch.data.switchNum = COMP_RT_GEAR_SW;
            acmiSwitch.data.switchVal = GetSwitch(COMP_RT_GEAR_SW);
            acmiSwitch.data.prevSwitchVal = switch3;
            gACMIRec.SwitchRecord(&acmiSwitch);
        }

        if (switch4 not_eq GetSwitch(COMP_NOS_GEAR_ROD))
        {
            acmiSwitch.data.switchNum = COMP_NOS_GEAR_ROD;
            acmiSwitch.data.switchVal = GetSwitch(COMP_NOS_GEAR_ROD);
            acmiSwitch.data.prevSwitchVal = switch4;
            gACMIRec.SwitchRecord(&acmiSwitch);
        }

        if (switch7 not_eq GetSwitch(COMP_TAIL_STROBE))
        {
            acmiSwitch.data.switchNum = COMP_TAIL_STROBE;
            acmiSwitch.data.switchVal = GetSwitch(COMP_TAIL_STROBE);
            acmiSwitch.data.prevSwitchVal = switch7;
            gACMIRec.SwitchRecord(&acmiSwitch);
        }

        if (switch8 not_eq GetSwitch(COMP_NAV_LIGHTS))
        {
            acmiSwitch.data.switchNum = COMP_NAV_LIGHTS;
            acmiSwitch.data.switchVal = GetSwitch(COMP_NAV_LIGHTS);
            acmiSwitch.data.prevSwitchVal = switch8;
            gACMIRec.SwitchRecord(&acmiSwitch);
        }

        if (switch9 not_eq GetSwitch(COMP_LAND_LIGHTS))
        {
            acmiSwitch.data.switchNum = COMP_LAND_LIGHTS;
            acmiSwitch.data.switchVal = GetSwitch(COMP_LAND_LIGHTS);
            acmiSwitch.data.prevSwitchVal = switch9;
            gACMIRec.SwitchRecord(&acmiSwitch);
        }

        if (switch14 not_eq GetSwitch(COMP_NOS_GEAR_DR_SW))
        {
            acmiSwitch.data.switchNum = COMP_NOS_GEAR_DR_SW;
            acmiSwitch.data.switchVal = GetSwitch(COMP_NOS_GEAR_DR_SW);
            acmiSwitch.data.prevSwitchVal = switch14;
            gACMIRec.SwitchRecord(&acmiSwitch);
        }

        if (switch15 not_eq GetSwitch(COMP_LT_GEAR_DR_SW))
        {
            acmiSwitch.data.switchNum = COMP_LT_GEAR_DR_SW;
            acmiSwitch.data.switchVal = GetSwitch(COMP_LT_GEAR_DR_SW);
            acmiSwitch.data.prevSwitchVal = switch15;
            gACMIRec.SwitchRecord(&acmiSwitch);
        }

        if (switch16 not_eq GetSwitch(COMP_RT_GEAR_DR_SW))
        {
            acmiSwitch.data.switchNum = COMP_RT_GEAR_DR_SW;
            acmiSwitch.data.switchVal = GetSwitch(COMP_RT_GEAR_DR_SW);
            acmiSwitch.data.prevSwitchVal = switch16;
            gACMIRec.SwitchRecord(&acmiSwitch);
        }

        if (switch17 not_eq GetSwitch(COMP_NOS_GEAR_HOLE))
        {
            acmiSwitch.data.switchNum = COMP_NOS_GEAR_HOLE;
            acmiSwitch.data.switchVal = GetSwitch(COMP_NOS_GEAR_HOLE);
            acmiSwitch.data.prevSwitchVal = switch17;
            gACMIRec.SwitchRecord(&acmiSwitch);
        }

        if (switch18 not_eq GetSwitch(COMP_LT_GEAR_HOLE))
        {
            acmiSwitch.data.switchNum = COMP_LT_GEAR_HOLE;
            acmiSwitch.data.switchVal = GetSwitch(COMP_LT_GEAR_HOLE);
            acmiSwitch.data.prevSwitchVal = switch18;
            gACMIRec.SwitchRecord(&acmiSwitch);
        }

        if (switch19 not_eq GetSwitch(COMP_RT_GEAR_HOLE))
        {
            acmiSwitch.data.switchNum = COMP_RT_GEAR_HOLE;
            acmiSwitch.data.switchVal = GetSwitch(COMP_RT_GEAR_HOLE);
            acmiSwitch.data.prevSwitchVal = switch19;
            gACMIRec.SwitchRecord(&acmiSwitch);
        }

        /* if ( switch20 not_eq GetSwitch(COMP_BROKEN_NOS_GEAR) )
        {
        acmiSwitch.data.switchNum = COMP_BROKEN_NOS_GEAR;
        acmiSwitch.data.switchVal = GetSwitch(COMP_BROKEN_NOS_GEAR);
        acmiSwitch.data.prevSwitchVal = switch20;
        gACMIRec.SwitchRecord( &acmiSwitch );
        }
        if ( switch21 not_eq GetSwitch(COMP_BROKEN_LT_GEAR) )
        {
        acmiSwitch.data.switchNum = COMP_BROKEN_LT_GEAR;
        acmiSwitch.data.switchVal = GetSwitch(COMP_BROKEN_LT_GEAR);
        acmiSwitch.data.prevSwitchVal = switch21;
        gACMIRec.SwitchRecord( &acmiSwitch );
        }
        if ( switch22 not_eq GetSwitch(COMP_BROKEN_RT_GEAR) )
        {
        acmiSwitch.data.switchNum = COMP_BROKEN_RT_GEAR;
        acmiSwitch.data.switchVal = GetSwitch(COMP_BROKEN_RT_GEAR);
        acmiSwitch.data.prevSwitchVal = switch22;
        gACMIRec.SwitchRecord( &acmiSwitch );
        }*/

        if (dof19 not_eq GetDOFValue(COMP_NOS_GEAR))
        {
            DOFRec.data.DOFNum = COMP_NOS_GEAR;
            DOFRec.data.DOFVal = GetDOFValue(COMP_NOS_GEAR);
            DOFRec.data.prevDOFVal = dof19;
            gACMIRec.DOFRecord(&DOFRec);
        }

        if (dof20 not_eq GetDOFValue(COMP_LT_GEAR))
        {
            DOFRec.data.DOFNum = COMP_LT_GEAR;
            DOFRec.data.DOFVal = GetDOFValue(COMP_LT_GEAR);
            DOFRec.data.prevDOFVal = dof20;
            gACMIRec.DOFRecord(&DOFRec);
        }

        if (dof21 not_eq GetDOFValue(COMP_RT_GEAR))
        {
            DOFRec.data.DOFNum = COMP_RT_GEAR;
            DOFRec.data.DOFVal = GetDOFValue(COMP_RT_GEAR);
            DOFRec.data.prevDOFVal = dof21;
            gACMIRec.DOFRecord(&DOFRec);
        }

        if (dof22 not_eq GetDOFValue(COMP_NOS_GEAR_DR))
        {
            DOFRec.data.DOFNum = COMP_NOS_GEAR_DR;
            DOFRec.data.DOFVal = GetDOFValue(COMP_NOS_GEAR_DR);
            DOFRec.data.prevDOFVal = dof22;
            gACMIRec.DOFRecord(&DOFRec);
        }

        if (dof23 not_eq GetDOFValue(COMP_LT_GEAR_DR))
        {
            DOFRec.data.DOFNum = COMP_LT_GEAR_DR;
            DOFRec.data.DOFVal = GetDOFValue(COMP_LT_GEAR_DR);
            DOFRec.data.prevDOFVal = dof23;
            gACMIRec.DOFRecord(&DOFRec);
        }

        if (dof24 not_eq GetDOFValue(COMP_RT_GEAR_DR))
        {
            DOFRec.data.DOFNum = COMP_RT_GEAR_DR;
            DOFRec.data.DOFVal = GetDOFValue(COMP_RT_GEAR_DR);
            DOFRec.data.prevDOFVal = dof24;
            gACMIRec.DOFRecord(&DOFRec);
        }
    }

#endif
}
//MI
float AircraftClass::CheckLEF(int side)
{
    float rLEF;
    float lLEF;

    if (IsComplex())
    {
        rLEF = GetDOFValue(COMP_RT_LEF);
        lLEF = GetDOFValue(COMP_LT_LEF);
    }
    else
    {
        rLEF = GetDOFValue(SIMP_RT_LEF);
        lLEF = GetDOFValue(SIMP_LT_LEF);
    }

    //side 0 = left
    switch (side)
    {
        case 0:

            //Left LEF
            if (LEFState(LT_LEF_OUT) or LEFState(LEFSASYNCH)) //can't work anymore
                leftLEFAngle = LTLEFAOA;
            else //normal operation
            {
                if (LEFLocked)
                    leftLEFAngle = lLEF;
            }

            return leftLEFAngle;
            break;

        case 1:

            //Right LEF
            if (LEFState(RT_LEF_OUT) or LEFState(LEFSASYNCH)) //can't work anymore
                rightLEFAngle = RTLEFAOA; //got set when we took hit
            else //normal operation
            {
                if (LEFLocked)
                    rightLEFAngle = rLEF;
            }

            return rightLEFAngle;
            break;

        default:
            return 0.0F;
            break;
    }

    return 0.0F;
}

// MLR 2003-10-12 This is used to copy animations from the aircraft to the 3d pit
// maps Simple to Complex as needed
void AircraftClass::CopyAnimationsToPit(DrawableBSP *PitBSP)
{
    if (IsComplex())
    {
        // don't waste cycles on range checking.
        // Get/SetDOFangle() does so itself. same with Switches
        int i;

        for (i = 0; i < COMP_MAX_DOF; i++)
        {
            PitBSP->SetDOFangle(i, GetDOFValue(i));
        }

        // some of the switches had to be reordered because the pits have conflicting switches
        // copy 0 - 3
        for (i =  0; i < 4; i++)
            PitBSP->SetSwitchMask(i + COMP_PIT_AB, GetSwitch(i));

        // copy 4 - 6
        for (i = 4; i < 7; i++)
            PitBSP->SetSwitchMask(i, GetSwitch(i));

        // copy 7
        PitBSP->SetSwitchMask(COMP_PIT_TAIL_STROBE, GetSwitch(COMP_TAIL_STROBE));

        // copy 8-24
        for (i = 8; i < COMP_PIT_AB; i++)
            PitBSP->SetSwitchMask(i, GetSwitch(i));


#if COMP_MAX_SWITCH > 30 // just in case more switches are added.

        // copy 30 - COMP_MAX_SWITCH
        for (i = 30; i < COMP_MAX_SWITCH; i++)
        {
            PitBSP->SetSwitchMask(i, GetSwitch(i));
        }

#endif


    }
    else
    {
        // map Simple model DOFs to complex model DOFs;
        static const int dmap[] =
        {
            // PIT DOF             SIMPLE
            COMP_LT_STAB, SIMP_LT_STAB,
            COMP_RT_STAB, SIMP_RT_STAB,
            COMP_LT_FLAP, SIMP_LT_AILERON,
            COMP_RT_FLAP, SIMP_RT_AILERON,
            COMP_RUDDER, SIMP_RUDDER_1,
            COMP_LT_AIR_BRAKE_TOP, SIMP_AIR_BRAKE,
            COMP_LT_AIR_BRAKE_BOT, SIMP_AIR_BRAKE,
            COMP_RT_AIR_BRAKE_TOP, SIMP_AIR_BRAKE,
            COMP_RT_AIR_BRAKE_BOT, SIMP_AIR_BRAKE,
            COMP_SWING_WING, SIMP_SWING_WING_1,
            COMP_RT_TEF, SIMP_RT_TEF,
            COMP_LT_TEF, SIMP_LT_TEF,
            COMP_RT_LEF, SIMP_RT_LEF,
            COMP_LT_LEF, SIMP_LT_LEF,
            COMP_CANOPY_DOF, SIMP_CANOPY_DOF,
            COMP_PROPELLOR, SIMP_PROPELLOR,
            COMP_LT_SPOILER1, SIMP_LT_SPOILER1,
            COMP_RT_SPOILER1, SIMP_RT_SPOILER1,
            COMP_LT_SPOILER2, SIMP_LT_SPOILER2,
            COMP_RT_SPOILER2, SIMP_RT_SPOILER2,
            COMP_THROTTLE, SIMP_THROTTLE,
        };

        static const int dmap_size = sizeof(dmap) / sizeof(dmap[0]) / 2;

        for (int i = 0; i < dmap_size; i++)
        {
            PitBSP->SetDOFangle(dmap[i * 2], GetDOFValue(dmap[i * 2 + 1]));
        }


        // map Simple model Switches to complex model Switches;
        static const int smap[] =
        {
            // PIT SWITCH          SIMPLE
            COMP_PIT_AB, SIMP_AB,
            COMP_PIT_NOS_GEAR_SW, SIMP_GEAR,
            COMP_PIT_LT_GEAR_SW, SIMP_GEAR,
            COMP_PIT_RT_GEAR_SW, SIMP_GEAR,
            COMP_WING_VAPOR, SIMP_WING_VAPOR,
            COMP_CANOPY, SIMP_CANOPY,
            COMP_DRAGCHUTE, SIMP_DRAGCHUTE,
            COMP_HOOK, SIMP_HOOK
        };
        static const int smap_size = sizeof(smap) / sizeof(smap[0]) / 2;

        for (int i = 0; i < smap_size; i++)
        {
            PitBSP->SetSwitchMask(smap[i * 2], GetSwitch(smap[i * 2 + 1]));
        }
    }
}
