#include "stdhdr.h"
#include "Graphics/Include/drawbsp.h"
#include "Graphics/Include/drawsgmt.h"
#include "Graphics/Include/rviewpnt.h"
#include "Graphics/Include/RenderOW.h"
#include "DrawParticleSys.h" //RV - I-Hawk - Added to support RV new trails
#include "ClassTbl.h"
#include "Entity.h"
#include "simobj.h"
#include "PilotInputs.h"
#include "hud.h"
#include "fcc.h"
#include "digi.h"
#include "facbrain.h"
#include "tankbrn.h"
#include "radar.h"
#include "radarAGOnly.h"
#include "radarDigi.h"
#include "radar360.h"
#include "radarSuper.h"
#include "radarDoppler.h"
#include "irst.h"
#include "eyeball.h"
#include "sms.h"
#include "guns.h"
#include "airframe.h"
#include "initdata.h"
#include "object.h"
#include "aircrft.h"
#include "fsound.h"
#include "FalcSnd/voicemanager.h"  // MLR 1/29/2004 - 
#include "soundfx.h"
#include "otwdrive.h"
#include "simdrive.h"
#include "hardpnt.h"
#include "campwp.h"
#include "missile.h"
#include "misldisp.h"
#include "f4error.h"
#include "cpmanager.h" //VWF added 2/18/97
#include "campbase.h"
#include "fack.h"
#include "caution.h"
#include "unit.h"
#include "playerop.h"
#include "camp2sim.h"
#include "sfx.h"
#include "acdef.h"
#include "simeject.h"
#include "MsgInc/EjectMsg.h" // PJW 12/3/97
#include "MsgInc/DamageMsg.h"
#include "acmi/src/include/acmirec.h"
#include "fakerand.h"
#include "navsystem.h"
#include "dogfight.h"
#include "falcsess.h"
#include "Graphics/Include/terrtex.h"
#include "laserpod.h"
#include "ui/include/logbook.h"
#include "MsgInc/TankerMsg.h"
#include "MsgInc/RadioChatterMsg.h"
#include "hardPnt.h"
#include "ptdata.h"
#include "atcbrain.h"
#include "ffeedbk.h"
#include "easyHts.h"
#include "VehRwr.h"
#include <mbstring.h>
#include "codelib/tools/lists/lists.h"
#include "acmi/src/include/acmitape.h"
#include "dofsnswitches.h"
#include "team.h"
#include "Graphics/Include/drawgrnd.h"
#include "Graphics/Include/matrix.h"
#include "Graphics/Include/tod.h"
#include "objectiv.h"
#include "lantirn.h"
#include "flight.h" /* MN added */
#include "Cmpclass.h" /* MN added */
#include "MissEval.h" /* MN added */
#include "profiler.h" // MLR 5/21/2004 - 
#include "ACTurbulence.h" // MLR 9/12/2004 - 
#include "dogfight.h" //TJL 08/01/04
#include "feature.h" //Cobra
#include "bomb.h"
#include "simbase.h"
// MD -- 20040110: added for analog cursor cupport
#include "SimIO.h"
#include "mavdisp.h"
#include "laserpod.h"
#include "HarmPod.h"

#define RESCALE(in,inmin,inmax,outmin,outmax) ( ((float)(in) - (inmin)) * ((outmax) - (outmin)) / ((inmax) - (inmin)) + (outmin))


extern SensorClass* FindLaserPod(SimMoverClass* theObject);

//MI for Radar Altimeter
extern bool g_bRealisticAvionics;
//static float Lasttime = SimLibElapsedTime * MSEC_TO_SEC; // 2002-01-29 MILOOK Not used but in case you decide to use it, shouldn't it be SEC_TO_MSEC?
bool OneSec = FALSE; //Did one second pass?

extern bool g_bDarkHudFix;
extern int g_nMPPowerXmitThreshold;
extern bool g_bMultiEngineSound;

extern bool g_bAllHaveIFF; // Cobra - Give all aircraft IFF interrogator

//MI
#include "PlayerRWR.h"
#include "commands.h"
extern bool g_bINS;
extern bool g_bNewEngineSounds;

extern short NumACDefEntries;


void ResetVoices(void);

ACMISwitchRecord acmiSwitch;
ACMIDOFRecord DOFRec;

extern int testFlag;
extern int RequestSARMission(FlightClass* flight);
int gPlayerExitMenuShown = FALSE;

#ifdef USE_SH_POOLS
MEM_POOL AircraftClass::pool;
#endif

#define FOLLOW_RATE 10.0F

extern float g_fCarrierStartTolerance; // JB carrier

extern float SimLibLastMajorFrameTime;

extern int g_nShowDebugLabels; // MN
extern bool g_bAIGloc; // MN
extern float g_fAIRefuelSpeed; // MN
extern bool g_bWakeTurbulence;
//static bool doOnce = FALSE;//Cobra

void CalcTransformMatrix(SimBaseClass* theObject);

#ifdef CHECK_PROC_TIMES
static ulong gPart1 = 0;
static ulong gPart2 = 0;
static ulong gPart3 = 0;
static ulong gPart4 = 0;
static ulong gPart5 = 0;
static ulong gPart6 = 0;
static ulong gPart7 = 0;
static ulong gPart8 = 0;
static ulong gWhole = 0;
extern int gameCompressionRatio;
#endif

AircraftClass::AircraftClass(int flag, VU_BYTE** stream, long *rem) : SimVehicleClass(stream, rem)
{
    InitLocalData(flag);
}

AircraftClass::AircraftClass(int flag, FILE* filePtr) : SimVehicleClass(filePtr)
{
    InitLocalData(flag);
}

AircraftClass::AircraftClass(int flag, int type) : SimVehicleClass(type)
{
    InitLocalData(flag);
}

void AircraftClass::InitData()
{
    SimVehicleClass::InitData();
    InitLocalData(IsDigital());
}

void AircraftClass::InitLocalData(int flag)
{
    turbulence = 0;// wake turb
    rVortex = 0; // wingtip turp
    lVortex = 0;

    acmiTimer = 0;

    // MLR 2/23/2004 - for recording the ACMI
#if 1
    // sfr: changed
    memset(acmiDOFValue, 0, sizeof(acmiDOFValue));
    memset(acmiSwitchValue, 0, sizeof(acmiSwitchValue));
#else

    for (int l = 0; l < COMP_MAX_DOF; l++)
    {
        acmiDOFValue[l] = 0;
    }

    for (int l = 0; l < COMP_MAX_SWITCH; l++)
    {
        acmiSwitchValue[l] = 0;
    }

#endif

    dustTrail = 0;  // MLR 1/4/2004 - new dust trail object
    dustConnect = FALSE; // MLR 1/4/2004 -
    //sfr: removed, using SetIsDigital
    //isDigital = flag;
    //autopilotType = APOff;//Cobra TEST
    SetIsDigital(flag);
    lastapType = ThreeAxisAP;
    theBrain = NULL;
    Guns = NULL;
    dropProgrammedStep  = 0;
    dropProgrammedTimer = 0;

    // Init targeting
    targetPtr = NULL;
    acFlags = 0;
    fireGun = FALSE;
    fireMissile = FALSE;
    lastPickle = FALSE;
    curWaypoint = 0;
    glocFactor = 0.0F;
    gLoadSeconds = 0.0F;
    playerSmokeOn = FALSE;
    // init JDAM
    JDAMtargetRange = -1.0f;
    strcpy(JDAMtargetName, "");
    strcpy(JDAMtargetName1, "");
    JDAMtarget = NULL;

    af = NULL;
    waypoint = NULL;
#if 1
    // sfr: changed
    memset(smokeTrail, 0, sizeof(smokeTrail));
    memset(conTrails, 0, sizeof(conTrails));
    memset(engineTrails, 0, sizeof(engineTrails));
#else

    for (int stn = 0; stn < TRAIL_MAX; stn++)
    {
        smokeTrail[stn] = NULL;
    }

    for (stn = 0; stn < MAXENGINES; stn++)
    {
        conTrails[stn] = NULL;
        engineTrails[stn] = NULL;
    }

#endif
    lwingvortex = rwingvortex = NULL;
    //wingvapor=NULL;

    //RV I-Hawk - Damage trail locations are 0 before being set in AircraftClass::ShowDamage
    damageTrailLocation0.x = damageTrailLocation1.x = 0.0f ;
    damageTrailLocation0.y = damageTrailLocation1.y = 0.0f ;
    damageTrailLocation0.z = damageTrailLocation1.z = 0.0f ;
    damageTrailLocationSet = FALSE;

    colorContrail = TRAIL_CONTRAIL; //RV - I-Hawk

    //RV - I-Hawk - determine if the burning position will follow damageLocation or not...
    if (rand() bitand 1)
        burnEffectPosition = true;
    else
        burnEffectPosition = false;

    //RV - I-Hawk - AC use random death time, and sometimes might explode right on death moment...
    dyingTime = PRANDInt5();

    dropChaffCmd = dropFlareCmd = NULL;
    ejectTriggered = FALSE;
    ejectCountdown = 1.0;
    doEjectCountdown = FALSE;

    //MI
    EmerJettTriggered = FALSE;
    JettCountown = 1.0;
    doJettCountdown = FALSE;
    //TJL 01/04/04
    wingSweep = 0.0F;

    bingoFuel = 1500;

    MPOCmd = FALSE;

    mFaults = NULL;
    lastStatus = 0;
    status_bits = 0;
    dirty_aircraft = 0;

    // timers for targeting
    nextGeomCalc = SimLibElapsedTime;
    nextTargetUpdate = SimLibElapsedTime;

    mCautionCheckTime = SimLibElapsedTime;

    if (isDigital)
    {
        geomCalcRate = FloatToInt32(0.5F * SEC_TO_MSEC);
        targetUpdateRate = (5 * SEC_TO_MSEC);/*FloatToInt32((1.0f + 3.0f * (float)rand()/RAND_MAX) * SEC_TO_MSEC);*/
    }
    else
    {
        geomCalcRate = 0;
        targetUpdateRate = 0;
    }

    // If we're flying dogfight, we mark this entity as nontransferable
    if (SimDriver.RunningDogfight())
    {
        share_.flags_.breakdown_.transfer_ = 0;
    }

    pLandLitePool = NULL;
    mInhibitLitePool = TRUE;

    //used for safe deletion of sensor array when making a player vehicle
    tempSensorArray = NULL;
    tempNumSensors = 0;
    powerFlags = 0; // JPO
    RALTStatus = ROFF; // JPO initialise
    RALTCoolTime = 5.0F;

    //MI EWS
    ManualECM = FALSE;
    EWSProgNum = 0;
    APFlag = 0;

    //MI Seat Arm
    SeatArmed = FALSE;

    // JPO electrics
    mainPower = MainPowerOff;
    currentPower = PowerNone;
    elecLights = ElecNone;
    interiorLight = LT_OFF;
    instrumentLight = LT_OFF;
    spotLight = LT_OFF;
    TheColorBank.PitLightLevel = 0;

    //MI Caution stuff
    NeedsToPlayCaution = FALSE;
    NeedsToPlayWarning = FALSE;
    WhenToPlayWarning = NULL;
    WhenToPlayCaution = NULL;

    attachedEntity = NULL; // JB carrier

    // 2001-10-21 ADDED BY S.G. INIT THESE SO AI REACTS TO THE FIRST MISSILE LAUNCH
    incomingMissile[0] = NULL;
    incomingMissile[1] = NULL;
    incomingMissileEvadeTimer = 10000000;
    incomingMissileRange = 500 * NM_TO_FT;

    //MI VMS stuff
    playBetty = TRUE;
    //MI RF Switch
    RFState = 0;

    //MI overSpeed and G stuff
    for (int i = 0; i < 3; i++)
    {
        switch (i)
        {
            case 0:
                //Level one, 60 - 90kts tolerance
                OverSpeedToleranceTanks[i] = 60 + rand() % 30;
                OverSpeedToleranceBombs[i] = 60 + rand() % 30;

                //0.5-1G's tolerance
                OverGToleranceTanks[i] = 5 + rand() % 5;

                //1.8 - 2.2 G tolerance, will be divided by 10
                OverGToleranceBombs[i] = 18 + rand() % 4;

                //assign them
                SpeedToleranceTanks = OverSpeedToleranceTanks[i];
                SpeedToleranceBombs = OverSpeedToleranceBombs[i];

                GToleranceTanks = float(OverGToleranceTanks[i]) / 10.0f;
                GToleranceBombs = float(OverGToleranceBombs[i]) / 10.0f;
                break;

            case 1:
                //level two,  90 - 120kts tolerance
                OverSpeedToleranceTanks[i] = 91 + rand() % 30;
                OverSpeedToleranceBombs[i] = 91 + rand() % 30;

                //1.1 - 1.6G's tolerance, will be divided by 10
                OverGToleranceTanks[i] = 11 + rand() % 5;

                //2.3 - 2.7 G tolerance
                OverGToleranceBombs[i] = 23 + rand() % 4;
                break;

            case 2:
                //level three, 120 - 150kts tolerance
                OverSpeedToleranceTanks[i] = 122 + rand() % 30;
                OverSpeedToleranceBombs[i] = 122 + rand() % 30;

                //1.7 - 2.2G's tolerance, will be divided by 10
                OverGToleranceTanks[i] = 17 + rand() % 5;

                //2.8 - 3.2 G tolerance
                OverGToleranceBombs[i] = 28 + rand() % 4;
                break;

            default:
                break;
        }
    }

    //MI autopilot
    //right switch to off (middle)
    ClearAPFlag(AircraftClass::AltHold);
    ClearAPFlag(AircraftClass::AttHold);
    //left switch to att hold (middle)
    //me123 middle is not rollhold realy..besides sometimes you can't stear
    //in mp after takeoff with this SetAPFlag(AircraftClass::RollHold);
    ClearAPFlag(AircraftClass::RollHold);
    ClearAPFlag(AircraftClass::HDGSel);
    ClearAPFlag(AircraftClass::StrgSel);

    //EWS init
    SetPGM(Off);

    //Exterior lights init
    //ExteriorLights = 0;
    CockpitWingLight = FALSE;
    CockpitWingLightFlash = FALSE; //martinv i have no idea about why these WERE residing under TGP cooling ??
    CockpitStrobeLight = FALSE;

    //MI emergency Jettison
    EmerJettTriggered = FALSE;
    JettCountown = 0;
    doJettCountdown = FALSE;

    //MI AVTR
    AVTRFlags = 0;
    AVTRCountown = 0; //AVTR auto
    doAVTRCountdown = FALSE;
    AVTROn(AVTR_OFF);

    //MI INS
    INSFlags = 0;
    INSOn(INS_PowerOff);
    INSAlignmentTimer = NULL;
    INSAlignmentStart = vuxGameTime;
    INSAlign = FALSE;
    HasAligned = FALSE;
    INSStatus = 100;
    INSLatDrift = 0.0F;
    INSLongDrift = 0.0F;
    INSTimeDiff = 0.0F;
    INS60kts = FALSE;
    CheckUFC = TRUE;
    INSLatOffset = 0.0F;
    INSLongOffset = 0.0F;
    INSAltOffset = 0.0F;
    INSHDGOffset = 0.0F;
    INSDriftLatDirection = 0;
    INSDriftLongDirection = 0;
    INSDriftDirectionTimer = 0.0F;
    BUPADIEnergy = 0.0F;
    GSValid = TRUE;
    LOCValid = TRUE;

    //MI MAL and Indicator lights test button and some other stuff
    CanopyDamaged = FALSE;
    TestLights = FALSE;
    LEFLocked = FALSE;
    LEFFlags = 0;
    LTLEFAOA = 0.0F;
    RTLEFAOA = 0.0F;
    leftLEFAngle = 0.0F;
    rightLEFAngle = 0.0F;
    TrimAPDisc = FALSE;
    TEFExtend = FALSE;
    MissileVolume = 8; //no volume
    ThreatVolume = 8;
    GunFire = FALSE;

    //TGP Cooling
    PodCooling = (float)((7 + rand() % 9) * 60); //7 - 15 Minutes to cool down.
    FCC = NULL;
    Sms = NULL;
    AWACSsaidAbort = false;

    NightLight = FALSE;
    WideView = FALSE;

    flareDispenser = 0;
    flareUsed = 0;
    chaffDispenser = 0;
    chaffUsed = 0;

    brakePos = 0;//TJL 02/28/04
    speedBrakeState = 0.0f;
    swingWingAngle = 0; // MLR 3/5/2004 -
    spawnpoint = 0; //RAS-11Nov01-for initial spawnpoint
    requestHotpitRefuel = FALSE; //TJL 08/16/04 //Cobra 10/30/04 TJL
    iffEnabled = FALSE;//Cobra 11/20/04
    interrogating = FALSE; //Cobra 11/21/04 True when TMS Left is Held
    runIFFInt = FALSE;//Cobra 11/21/04
    iffModeTimer = (float)SimLibElapsedTime;//Cobra 11/21/04
    iffModeChallenge = 99;//Cobra 11/21/04
    JDAMStep = 1; //Cobra - Start by finding a default target (for AI)
    JDAMtgtnum = -1; //Cobra
    JDAMsbc = NULL;
    JDAMtgtPos.x = -1;
    JDAMtgtPos.y = -1;
    JDAMAllowAutoStep = true; // RV - I-Hawk

    PulseTurbulenceTime = 0;
    memset(&PulseTurbulence, 0, sizeof(PulseTurbulence));
    memset(&TotalTurbulence, 0, sizeof(TotalTurbulence));
    memset(&StaticTurbulence, 0, sizeof(StaticTurbulence));

    //sfr: animwing timers
    animWingFlashTimer = 0;

    // RV - Biker
    carrierStartPosEngaged = 0;
    carrierInitTimer = 0.0f;
    takeoffSlot = 99;
}

AircraftClass::~AircraftClass()
{
    CleanupLocalData();
}

void AircraftClass::CleanupLocalData()
{
    if (af)
    {
        delete(af);
        af = NULL;
    }

    if (mFaults)
    {
        delete mFaults;
        mFaults = NULL;
    }

    if (SimDriver.GetPlayerEntity() == this)   //VWF
    {
        gNavigationSys->DeleteMissionTacans();
    }

    if (Sms)
    {
        delete Sms;
        Sms = NULL;
    }

    if (FCC)
    {
        delete FCC;
        FCC = NULL;
    }
}

void AircraftClass::CleanupData()
{
    CleanupLocalData();
    SimVehicleClass::CleanupData();
}

void AircraftClass::Init(SimInitDataClass* initData)
{
    SimVehicleClass::Init(initData);

    int i;
    WayPointClass* atWaypoint;
    float wp1X, wp1Y, wp1Z;
    float wp2X, wp2Y, wp2Z;
    Falcon4EntityClassType* classPtr = (Falcon4EntityClassType*)EntityType();
    int afIdx;
    long dt;

    if (initData)
    {
        if (GetSType() == STYPE_AIR_FIGHTER_BOMBER and GetSPType() == SPTYPE_F16C)
        {
            acFlags or_eq isF16 bitor isComplex;
            // Turn on the nozzle
            SetSwitch(COMP_EXH_NOZZLE, 1);

            if (gACMIRec.IsRecording())
            {
                acmiSwitch.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
                acmiSwitch.data.type = Type();
                acmiSwitch.data.uniqueID = Id();
                acmiSwitch.data.switchNum = COMP_EXH_NOZZLE;
                acmiSwitch.data.switchVal = TRUE;
                acmiSwitch.data.prevSwitchVal = TRUE;
                gACMIRec.SwitchRecord(&acmiSwitch);
            }
        }

        // Airframe Stuff
        af = new AirframeClass(this);
        af->SetFlag(AirframeClass::IsDigital);

        if (mFaults)
        {
            delete mFaults;
        }

        mFaults = new FackClass;

        if (isDigital)
        {
            if (SimDriver.RunningInstantAction())
            {
                af->SetFlag(AirframeClass::NoFuelBurn);
            }
        }
        else
        {
            // Unlimited fuel?
            if (PlayerOptions.UnlimitedFuel())
            {
                af->SetFlag(AirframeClass::NoFuelBurn);
            }

            // Flight model type
            if (PlayerOptions.GetFlightModelType() == FMSimplified)
            {
                af->SetFlag(AirframeClass::Simplified);
            }
        }

        af->initialX    = initData->x;
        af->initialY    = initData->y;
        af->initialZ    = initData->z;
        af->initialFuel = (float)initData->fuel;

        //KCK Kludge
        // sfr: dont need this anymore
        //af->x = af->initialX;
        //af->y = af->initialY;
        //af->z = af->initialZ;

        // END Kludge
        waypoint        = initData->waypointList;
        numWaypoints    = initData->numWaypoints;
        curWaypoint     = waypoint;
        atWaypoint      = waypoint;

        for (i = 0; i < numWaypoints; i++)
        {
            // Clear tactic for use as flags
            curWaypoint->GetLocation(&wp1X, &wp1Y, &wp1Z);

            if (wp1Z > (OTWDriver.GetGroundLevel(wp1X, wp1Y) - 500.0F))
                curWaypoint->SetLocation(wp1X, wp1Y, OTWDriver.GetGroundLevel(wp1X, wp1Y) + wp1Z);

            curWaypoint = curWaypoint->GetNextWP();
        }

        curWaypoint = waypoint;
        VU_TIME st = waypoint->GetWPArrivalTime();

        if (SimLibElapsedTime < st)
        {
            st = SimLibElapsedTime;
        }

        mFaults->SetStartTime(st); // set base time

        for (i = 0; i < initData->currentWaypoint; i++)
        {
            atWaypoint = curWaypoint;
            curWaypoint = curWaypoint->GetNextWP();
        }

        // Calculate current heading
        if (curWaypoint)
        {
            // Are we waiting for takeoff ?

            // RV - Biker - Don't initialize AI on carrier
            bool allowGroundInit = true;

            CampEntity ent = ((Flight)(initData->campBase))->GetUnitAirbase();

            if (ent and ent->IsTaskForce() and initData->playerSlot == NO_PILOT)
                allowGroundInit = false;

            if (curWaypoint->GetWPAction() == WP_TAKEOFF and allowGroundInit == true)
            {
                OnGroundInit(initData);
            }
            else
            {
                // RV - Biker - AI start at carrier pos with higher alt
                if (ent and ent->IsTaskForce())
                {
                    af->initialX = ent->XPos() + 1.0f * NM_TO_FT - rand() % 12000;
                    af->initialY = ent->YPos() + 1.0f * NM_TO_FT - rand() % 12000;
                    af->initialZ = min(af->initialZ, -5000 - rand() % 2000);
                }

                mFaults->AddTakeOff(waypoint->GetWPArrivalTime());

                if (curWaypoint == atWaypoint)
                    curWaypoint = curWaypoint->GetNextWP();

                atWaypoint->GetLocation(&wp1X, &wp1Y, &wp1Z);

                if (curWaypoint == NULL)
                {
                    wp1X = initData->x;
                    wp1Y = initData->y;
                    curWaypoint = atWaypoint;
                }

                if (SimDriver.RunningInstantAction() and not isDigital)
                {
                    af->initialPsi = 0.0F;
                    af->initialMach = 0.7F;
                }
                else
                {
                    curWaypoint->GetLocation(&wp2X, &wp2Y, &wp2Z);
                    af->initialPsi = (float)atan2(wp2Y - wp1Y, wp2X - wp1X);
                    dt = curWaypoint->GetWPArrivalTime() - atWaypoint->GetWPDepartureTime();

                    if (dt > 10 * SEC_TO_MSEC)
                    {
                        af->initialMach = (float)sqrt((wp2X - wp1X) * (wp2X - wp1X) + (wp2Y - wp1Y) * (wp2Y - wp1Y)) /
                                          (dt / SEC_TO_MSEC);
                        af->initialMach = max(af->initialMach, 200.0F);
                    }
                    else
                    {
                        af->initialMach = 0.7F;
                    }
                }

            }
        }
        else
        {
            af->initialPsi = 0.0F;
            af->initialMach = 0.7F;
        }

        // KCK Hack: To make EOM work ok...
        // REMOVE BEFORE FLIGHT
        if (af->initialMach < 0.01F)
        {
            af->initialMach = 0.01F; // Minimal speed to catch problems -
        }

        if (af->initialMach > 2.0F and af->initialMach < 10.0F)
        {
            af->initialMach = 2.0F; // max mach speed
        }

        //if (af->initialMach > 1000.0F)
        //af->initialMach = 1000.F; // max fps


        afIdx = SimACDefTable[classPtr->vehicleDataIndex].airframeIdx;

        // FRB - Garbage check
        if (afIdx > NumACDefEntries)
            afIdx = 1;

        af->InitData(afIdx);

        //TJL 12/20/03 This will allow for all F-16's to set the isF16 flag
        // without having to have the same specific type like the code above
        if (af->auxaeroData->typeAC == 1 or af->auxaeroData->typeAC == 2)
        {
            acFlags or_eq isF16;
        }

        //TJL 01/11/04 Set Two Engines
        if (af->auxaeroData->nEngines == 2)
        {
            acFlags or_eq hasTwoEngines;
        }


        // Weapons Stuff
        Sms = new SMSClass(this, initData->weapon, initData->weapons);
        FCC = new FireControlComputer(this, Sms->NumHardpoints());
        //FCC->Sms = Sms;
        FCC->SetSms(Sms); // MLR 4/10/2004 -

        if (Sms->NumHardpoints() > 0)
        {
            Guns = Sms->hardPoint[0]->GetGun();

            if (Guns)
            {
                // MLR 12/13/2003 - Set the guns trail ID
                Guns->SetTrailID(af->auxaeroData->gunTrailID);
            }
        }



        // edg: I moved this check down here to make ABSOLUTELY sure
        // that if we're in the air (ie haven't gone thru OnGroundInit() as
        // a result of being at a takeoff point), that we're above the terrain.
        // also the original check was inadequate.
        if ( not OnGround() and af->initialZ > OTWDriver.GetGroundLevel(af->x, af->y) - 500.0f)
        {
            af->initialZ = OTWDriver.GetGroundLevel(af->x, af->y) - 500.0F;
            af->z = af->initialZ;
        }

        // Get the controls
        af->Init(afIdx);


        if (af->auxaeroData->hasSwingWing)
        {
            acFlags or_eq hasSwing;
        }

        if (af->auxaeroData->isComplex)
        {
            acFlags or_eq isComplex;
            MakeComplex();
        }

        // Turn on the canopy.
        SetSwitch(COMP_CANOPY, TRUE);

        if (gACMIRec.IsRecording())
        {
            acmiSwitch.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
            acmiSwitch.data.type = Type();
            acmiSwitch.data.uniqueID = Id();
            acmiSwitch.data.switchNum = COMP_CANOPY;
            acmiSwitch.data.switchVal = TRUE;
            acmiSwitch.data.prevSwitchVal = TRUE;
            gACMIRec.SwitchRecord(&acmiSwitch);
        }

        // Add Laser designator and harm pod for F16's
        if (IsComplex())
        {
            SetSwitch(COMP_EXH_NOZZLE, 1);

            if (gACMIRec.IsRecording())
            {
                acmiSwitch.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
                acmiSwitch.data.type = Type();
                acmiSwitch.data.uniqueID = Id();
                acmiSwitch.data.switchNum = COMP_EXH_NOZZLE;
                acmiSwitch.data.switchVal = TRUE;
                acmiSwitch.data.prevSwitchVal = TRUE;
                gACMIRec.SwitchRecord(&acmiSwitch);
            }

            if (Sms->HasHarm())
            {
                //Flip the switch to show the HARM pod
                SetSwitch(COMP_HTS_POD, 1);
                //switchData[12] = 1;
                //switchChange[12] = TRUE;

                if (gACMIRec.IsRecording())
                {
                    acmiSwitch.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
                    acmiSwitch.data.type = Type();
                    acmiSwitch.data.uniqueID = Id();
                    acmiSwitch.data.switchNum = COMP_HTS_POD;
                    acmiSwitch.data.switchVal = TRUE;
                    acmiSwitch.data.prevSwitchVal = TRUE;
                    gACMIRec.SwitchRecord(&acmiSwitch);
                }
            }

            if (Sms->HasLGB())
            {
                // Flip the switch to show the laser pod
                SetSwitch(COMP_TIRN_POD, 1);

                //switchData[11] = 1;
                //switchChange[11] = TRUE;
                if (gACMIRec.IsRecording())
                {
                    acmiSwitch.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
                    acmiSwitch.data.type = Type();
                    acmiSwitch.data.uniqueID = Id();
                    acmiSwitch.data.switchNum = COMP_TIRN_POD;
                    acmiSwitch.data.switchVal = TRUE;
                    acmiSwitch.data.prevSwitchVal = TRUE;
                    gACMIRec.SwitchRecord(&acmiSwitch);
                }
            }
        }

        if (Guns)
        {
            float x, y, z;
            af->GetGunLocation(&x, &y, &z);
            Guns->SetPosition(x, y, z, af->auxaeroData->gunElevation * DTR, af->auxaeroData->gunAzimuth * DTR);
        }

        // Add Sensor
        // Quick, count the sensors
        {
            int sensorType, sensorIdx, hasHarm, hasLGB, needsRadar;

            numSensors = 0;
            needsRadar = TRUE;

            for (i = 0; i < 5; i++)
            {
                if (SimACDefTable[classPtr->vehicleDataIndex].sensorType[i] > 0)
                {
                    // MLR TODO, IRST is 0?
                    numSensors++;

                    //KLudge until new class table for bombers
                    if (SimACDefTable[classPtr->vehicleDataIndex].sensorType[i] == SensorClass::Radar)
                    {
                        needsRadar = FALSE;
                    }
                }
            }

            // Should we add a HARM pod?
            hasHarm = Sms->HasHarm();

            // Should we add a Targeting pod?
            hasLGB = Sms->HasLGB();
            numSensors += hasHarm + hasLGB;

            if (needsRadar)
            {
                numSensors ++;
            }

            sensorArray = new SensorClass*[numSensors];

            for (i = 0; i < numSensors - (hasHarm + hasLGB + needsRadar); i++)
            {
                sensorType = SimACDefTable[classPtr->vehicleDataIndex].sensorType[i];
                sensorIdx = SimACDefTable[classPtr->vehicleDataIndex].sensorIdx[i];

                switch (sensorType)
                {
                    case SensorClass::Radar:
                        if (RadarDataTable[GetRadarType()].LookDownPenalty < 0.0)
                            sensorArray[i] = new RadarAGOnlyClass(GetRadarType(), this);
                        else
                            sensorArray[i] = new RadarDigiClass(GetRadarType(), this);

                        break;

                    case SensorClass::RWR:
                        sensorArray[i] = new VehRwrClass(sensorIdx, this);
                        break;

                    case SensorClass::IRST:
                        sensorArray[i] = new IrstClass(sensorIdx, this);
                        break;

                    case SensorClass::Visual:
                        sensorArray[i] = new EyeballClass(sensorIdx, this);
                        break;

                    default:
                        ShiWarning("Unhandled sensor type during Init");
                        break;
                }
            }

            //Kludge until class table fixed
            if (needsRadar)
            {
                sensorArray[i] = new RadarAGOnlyClass(1, this);
                ++i;
            }

            if (hasHarm)
            {
                sensorArray[i] = new EasyHarmTargetingPod(5, this);
                // Cobra - new EasyHarmTargetingPod() does not set the sensorType.
                // New function added to set sensorType.
                sensorArray[i]->SetType(SensorClass::HTS);
                ++i;
            }

            if (hasLGB)
            {
                sensorArray[i] = new LaserPodClass(5, this);
                // Cobra - new LaserPodClass() does not set the sensorType.  New function added to set sensorType.
                sensorArray[i]->SetType(SensorClass::TargetingPod);
            }
        }

        // If I only had a brain
        // we need the waypoint data initialized before we build the brain
        switch (((UnitClass*)GetCampaignObject())->GetUnitMission())
        {
            case AMIS_FAC:
                theBrain = new FACBrain(this, af);
                break;

            case AMIS_TANKER:
                theBrain = new TankerBrain(this, af);
                SetFlag(I_AM_A_TANKER);
                break;

            default:
                theBrain = new DigitalBrain(this, af);
                break;
        }

        DBrain()->SetTaxiPoint(initData->ptIndex);
        DBrain()->SetSkill(initData->skill);
        //DBrain()->SetSkill(4);
        // Modify search time based on skill level
        // Rookies (level 0) has 9x ave time, top of the line has 1x
        geomCalcRate = geomCalcRate * ((4 - DBrain()->SkillLevel()) * 2 + 1);
        targetUpdateRate = (5 * SEC_TO_MSEC);/*targetUpdateRate * ((4 - DBrain()->SkillLevel()) * 2 + 1);*/

        // DO NOT MOVE THIS CODE OR I WILL BREAK YOUR DAMN FINGERS, VWF 3/11/97
        if (isDigital)
        {
            af->SetSimpleMode(SIMPLE_MODE_AF);
        }

        // END OF WARNING

        /*--------------------*/
        /* Update shared data */
        /*--------------------*/
        if ( not OnGround())
        {
            SetPosition(af->x, af->y, af->z);
        }
        else
        {
            Objective obj = (Objective)vuDatabase->Find(DBrain()->Airbase());
            int pt = initData->ptIndex;

            if (obj)
            {
                pt = FindBestSpawnPoint(obj, initData);
            }

            DBrain()->SetTaxiPoint(pt);
            SetPosition(af->x, af->y, OTWDriver.GetGroundLevel(af->x, af->y) - 5.0F);
            af->vt = 0.0f;
        }

        SetDelta(0.0F, 0.0F, 0.0F);
        SetYPR(af->psi, af->theta, af->phi);
        SetYPRDelta(0.0F, 0.0F, 0.0F);
        // sfr: no need for this anymore
        //SetVt(af->vt);
        //SetKias(af->vcas);
        // 2000-11-17 MODIFIED BY S.G.
        // SO OUR SetPowerOutput WITH ENGINE TEMP IS USED INSTEAD OF THE SimBase CLASS ONE
        SetPowerOutput(af->rpm);
        //me123 changed back SetPowerOutput(af->rpm, af->oldp01[0]);
        // END OF MODIFIED SECTION

        CalcTransformMatrix(this);
        theInputs   = new PilotInputs;

        if (Sms->NumHardpoints() and isDigital)
        {
            Sms->SetUnlimitedAmmo(FALSE);
            FCC->SetMasterMode(FireControlComputer::Missile);
            FCC->SetSubMode(FireControlComputer::Aim9);
        }

        if ( not g_bRealisticAvionics or not OnGround())  // JPO set up the systems
        {
            PreFlight();
        }
    }

    // FRB - Open canopy before entering 3D
    if (PlayerOptions.GetStartFlag() not_eq PlayerOptionsClass::START_RUNWAY and 
        OnGround() /* and (af->GetParkType() not_eq LargeParkPt)*/) // FRB = BigBoys do things too.
    {
        af->canopyState = true;

        if (TheTimeOfDay.GetLightLevel() < 0.65f)
        {
            SetSwitch(COMP_3DPIT_INTERIOR_LIGHTS, TRUE);
            SetAcStatusBits(ACSTATUS_PITLIGHT);
        }
    }
    else
    {
        af->canopyState = false;
        SetSwitch(COMP_3DPIT_INTERIOR_LIGHTS, FALSE);
        ClearAcStatusBits(ACSTATUS_PITLIGHT);
    }

    // Check unlimited guns in dogfight
    if (SimDriver.RunningDogfight() and SimDogfight.IsSetFlag(DF_UNLIMITED_GUNS))
    {
        Sms->SetUnlimitedGuns(TRUE);
    }

    // timers for targeting
    nextGeomCalc = SimLibElapsedTime;
    nextTargetUpdate = SimLibElapsedTime;
    SOIManager(SimVehicleClass::SOI_RADAR);

    CleanupLitePool();

    //MI Reset our soundflag
    SpeedSoundsWFuel = 0;
    SpeedSoundsNFuel = 0;
    GSoundsWFuel = 0;
    GSoundsNFuel = 0;
    StationsFailed = 0;

    //MI INS
    INSFlags = 0;
    INSOn(INS_PowerOff);
    INSAlignmentTimer = NULL;
    INSAlignmentStart = vuxGameTime;
    INSAlign = FALSE;
    HasAligned = FALSE;
    INSStatus = 100;
    INSLatDrift = 0.0F;
    INSLongDrift = 0.0F;
    INSTimeDiff = 0.0F;
    INS60kts = FALSE;
    CheckUFC = TRUE;
    INSLatOffset = 0.0F;
    INSLongOffset = 0.0F;
    INSAltOffset = 0.0F;
    INSHDGOffset = 0.0F;
    INSDriftLatDirection = 0;
    INSDriftLongDirection = 0;
    INSDriftDirectionTimer = 0.0F;
    BUPADIEnergy = 0.0F;
    GSValid = TRUE;
    LOCValid = TRUE;

    //MI MAL and Indicator lights test button and some other stuff
    CanopyDamaged = FALSE;
    TestLights = FALSE;
    LEFLocked = FALSE;
    LEFFlags = 0;
    LTLEFAOA = 0.0F;
    RTLEFAOA = 0.0F;
    leftLEFAngle = 0.0F;
    rightLEFAngle = 0.0F;
    TrimAPDisc = FALSE;
    TEFExtend = FALSE;
    MissileVolume = 8; //no volume
    ThreatVolume = 8;
    GunFire = FALSE;
    //Targeting pod cooling
    PodCooling = (float)((7 + rand() % 9) * 60); //7 - 15 Minutes to cool down.
    CockpitWingLight = FALSE;
    CockpitWingLightFlash = FALSE;
    CockpitStrobeLight = FALSE;

    NightLight = FALSE;
    WideView = FALSE;
}

Tpoint AircraftClass::GetTurbulence(void)
{
    return TotalTurbulence;
}

void AircraftClass::SetPulseTurbulence(float RateX, float RateY, float RateZ, float Time)
{
    PulseTurbulence.x += RateX;
    PulseTurbulence.y += RateY;
    PulseTurbulence.z += RateZ;

    if (Time > PulseTurbulenceTime) PulseTurbulenceTime = Time;
}


namespace
{
    /** randomizes a burn location. Used below. */
    void randomizeBurn(const AircraftClass &ac, unsigned int sfxType, Tpoint &pos, Tpoint &vec)
    {
        Trotation orientation = static_cast<DrawableBSP*>(ac.drawPointer)->orientation;

        if (ac.burnEffectPosition)
        {
            pos.x = ac.XPos() + orientation.M11 * ac.damageTrailLocation0.x +
                    orientation.M12 * ac.damageTrailLocation0.y +
                    orientation.M13 * ac.damageTrailLocation0.z
                    ;
            pos.y = ac.YPos() + orientation.M21 * ac.damageTrailLocation0.x +
                    orientation.M22 * ac.damageTrailLocation0.y +
                    orientation.M23 * ac.damageTrailLocation0.z
                    ;
            pos.z = ac.ZPos() + orientation.M31 * ac.damageTrailLocation0.x +
                    orientation.M32 * ac.damageTrailLocation0.y +
                    orientation.M33 * ac.damageTrailLocation0.z
                    ;
        }
        else
        {
            pos.x = ac.XPos();
            pos.y = ac.YPos();
            pos.z = ac.ZPos();
        }

        vec.x = ac.XDelta();
        vec.y = ac.YDelta();
        vec.z = ac.ZDelta();

        DrawableParticleSys::PS_AddParticleEx((sfxType + 1), &pos, &vec);
    }
}

int AircraftClass::Exec(void)
{
    //START_PROFILE("AC_EXEC");
    bool EvenFrame = true;
    StaticTurbulence.x = StaticTurbulence.y = StaticTurbulence.z = 0.0f;

    if (this == SimDriver.GetPlayerEntity())
    {
        if (EvenFrame)
        {
            //* SPEED BRAKES *
            float Turb = af->dbrake * 0.005f * sqrtf(af->vcas);
            static float LastMach = 0;

            // * MAX CAS *
            // Calculate based on Max Cas for the vehicle and removing a 70%
            float TempTurb = af->vcas / af->MaxVcas() - 0.7f;

            // if we r over 70% of Max Cas Apply Turbulence
            if (TempTurb > 0.0f) Turb += 6.0f * (TempTurb * TempTurb);

            // * SHOCK WAVE *
            // Calculate removing a 90%
            TempTurb = sqrtf(XDelta() * XDelta() + ZDelta() * ZDelta() + YDelta() * YDelta());
            TempTurb = TempTurb / 1100 - 0.9f;

            // if we r over 90% of Mach Apply Turbulence
            if (TempTurb > 0.0f and TempTurb <= 0.1f) Turb += 10.0f * (TempTurb * TempTurb);

            // if Passing Sonic Wave, Bang
            if (TempTurb >= 0.1f and LastMach < 0.1f) SetPulseTurbulence(0.3f, 0.3f, 0.3f, 1.0f);

            LastMach = TempTurb;


            // * LOADED G's*
            // Calculate removing a 70%
            TempTurb = af->nzcgs / af->MaxGs() - 0.7f;

            // if we r over 70% of Max Load Apply Turbulence
            if (TempTurb > 0.0f) Turb += 5.0f * TempTurb * TempTurb;

            StaticTurbulence.x = StaticTurbulence.y = StaticTurbulence.z = Turb;

            // COBRA - RED - Update Turbulence for the Pit, the Decrement is calculates with a rate of 100 mSecs
            if (PulseTurbulenceTime)
            {
                float Div = PulseTurbulenceTime / SimLibMajorFrameTime;
                // Decrease it with present elapsed time rate
                PulseTurbulence.x -= PulseTurbulence.x / Div;
                PulseTurbulence.y -= PulseTurbulence.y / Div;
                PulseTurbulence.z -= PulseTurbulence.z / Div;
                PulseTurbulenceTime -= SimLibMajorFrameTime;

                if (PulseTurbulenceTime < 0.0f) PulseTurbulenceTime = 0.0f;
            }
            else
            {
                // No time, no Turbulence
                PulseTurbulence.x = PulseTurbulence.y = PulseTurbulence.z = 0.0f;
            }

            TotalTurbulence.x = (PulseTurbulence.x + StaticTurbulence.x);
            TotalTurbulence.y = (PulseTurbulence.y + StaticTurbulence.y);
            TotalTurbulence.z = (PulseTurbulence.z + StaticTurbulence.z);

            // Setup the Value for sound stuff
            Turb = max(TotalTurbulence.x, max(TotalTurbulence.y, TotalTurbulence.z));
            Turb = log(Turb) * 1000.0f;

            // The turbulence Noise
            if (af->nzcgs > 5.5f)
            {
                float v;

                //I-Hawk - different calculation for volume based on G...

                v = -2000.0f;

                v += (af->nzcgs - 5.5f) * 250.0f ;
                //F4SoundFXSetDist(af->auxaeroData->sndOverSpeed2, TRUE, v, ( (GetKias() - af->curMaxStoreSpeed)) / 25);
                SoundPos.Sfx(af->auxaeroData->sndOverSpeed2, 0, 2 , v); // MLR 5/16/2004 -
            }

            //if (Turb) SoundPos.Sfx(af->auxaeroData->sndOverSpeed2, 0, 2, min( Turb , 0.0f));

            // Apply randomness to be used
            TotalTurbulence.x *= PRANDFloat() * 0.01f;
            TotalTurbulence.y *= PRANDFloat() * 0.01f;
            TotalTurbulence.z *= PRANDFloat() * 0.01f;
        }

        EvenFrame xor_eq 1;
    }

    SoundPos.UpdatePos(this);

    if (turbulence)
    {
        // tubulence trails
        // wingtip vortices are done in damage.cpp
        Tpoint turb;

        //if(GetKias() > 50)
        if (af->IsSet(AirframeClass::InAir))
        {
            Trotation *orientation = &((DrawableBSP*)drawPointer)->orientation;

            // place turb 20' behind jet

            turb.x = XPos() + orientation->M11 * -20; //thrx + orientation->M12*thry + orientation->M13*thrz;
            turb.y = YPos() + orientation->M21 * -20; //thrx + orientation->M22*thry + orientation->M23*thrz;
            turb.z = ZPos() + orientation->M31 * -20; //thrx + orientation->M32*thry + orientation->M33*thrz;

            float acweight = af->weight * 0.00001f;
            float acaoa = af->alpha * 0.1f;
            //float strength = 1; //af->weight * af->nxcgb;
            float strength = acweight * acaoa;

            turbulence->RecordPosition(strength, turb.x, turb.y, turb.z); // TODO compute a strength value

            // the wingtip vortices are in Damage.cpp because the wingtip position is calculated there, and
            // it takes into account wing sweep and all that jazz.
        }
        else
        {
            turbulence->BreakRecord();
        }
    }

    //float groundZ = 0.0f;
    //Falcon4EntityClassType* classPtr;
    ACMIAircraftPositionRecord airPos;
    Flight flight = NULL;
    int flightIdx;

    if (g_nShowDebugLabels bitand 0x2000)
    {
        if (drawPointer)
        {
            char tmpchr[1024];
            sprintf(tmpchr, "%f", af->Fuel() + af->ExternalFuel());
            ((DrawableBSP*)drawPointer)->SetLabel(tmpchr, ((DrawableBSP*)drawPointer)->LabelColor());
        }
    }

    if (IsDead())
    {
        return TRUE;
    }

    // only do ejection on local entity
    if (IsLocal())
    {
        // When doing a QuickPreflight, it can happen that the HUD is not yet setup and
        // it stays dark when entering the simulation.
        if (g_bDarkHudFix and TheHud and this == SimDriver.GetPlayerEntity() and 
            DBrain() and not (DBrain()->moreFlags bitand DigitalBrain::HUDSetup) and 
            PlayerOptions.GetStartFlag() not_eq PlayerOptionsClass::START_RAMP)
        {
            TheHud->SymWheelPos = 1.0F;
            TheHud->SetLightLevel();
            DBrain()->moreFlags or_eq DigitalBrain::HUDSetup; // set the flag so we don't go in here again
        }


        if (doEjectCountdown and not ejectTriggered and ejectCountdown < 0.0 and fabs(glocFactor) < 0.95F)
        {
            ejectTriggered = TRUE;
            //Make sure the autopilot is off;
            // sfr: moved below to test
            //SetAutopilot(APOff);
            //no radio if you eject :)
            ResetVoices();
        }

        ejectCountdown -= SimLibMajorFrameTime;

        // ejection: use a small pctStrength damage window to trigger;
        if
        (
            (
                (
                    pctStrength < -0.4f and pctStrength > -0.6f and 
                    (
                        dyingType == SimVehicleClass::DIE_INTERMITTENT_SMOKE or
                        dyingType == SimVehicleClass::DIE_SHORT_FIREBALL or
                        dyingType == 5 or
                        dyingType == 6 or
                        dyingType == SimVehicleClass::DIE_SMOKE
                    ) and 
                    isDigital
                ) or
                (
                    ejectTriggered
                )
            ) and 
            (
 not IsAcStatusBitsSet(ACSTATUS_PILOT_EJECTED)
            )
        )
        {
            Eject();
            ResetVoices();
            SoundPos.Sfx(af->auxaeroData->sndEject, 0, 1.0, 0);
        }

        //MI Emergency jettison
        if (g_bRealisticAvionics)
        {
            if (doJettCountdown and not EmerJettTriggered and JettCountown < 0.0)
            {
                EmerJettTriggered = TRUE;
            }

            // MN fix: reset EmerJettTriggered
            if (EmerJettTriggered and Sms)
            {
                Sms->EmergencyJettison();
                EmerJettTriggered = false;
                doJettCountdown = false; // Unz and Saint -- This was missing causing MP flooding.
            }

            JettCountown -= SimLibMajorFrameTime;

            //AVTR
            if (AVTRState(AVTR_AUTO) and SimDriver.AVTROn())
            {
                if (AVTRCountown > 0.0F)
                    AVTRCountown -= SimLibMajorFrameTime;
                else if (AVTRCountown <= 0.0F) //turn it off
                {
                    ACMIToggleRecording(0, KEY_DOWN, NULL);
                    SimDriver.SetAVTR(FALSE);
                }
            }

            //MI INS
            if (g_bINS and not isDigital)
            {
                RunINS();

                //Backup ADI engergy
                if (MainPower() == MainPowerMain)
                {
                    if (BUPADIEnergy < 540.0F) //9 minutes
                        BUPADIEnergy += (SimLibMajorFrameTime * 9 * 2); //full in 1 min
                }
                else if (BUPADIEnergy > 0.0F)
                    BUPADIEnergy -= SimLibMajorFrameTime;

                if (BUPADIEnergy > 0.0F) //goes away immediately
                    INSOn(BUP_ADI_OFF_IN);
                else
                    INSOff(BUP_ADI_OFF_IN); //visibile

                //TargetingPod Cooling
                if (HasPower(AircraftClass::RightHptPower) and PodCooling > -1.0F)
                    PodCooling -= SimLibMajorFrameTime;
                else //warm it up if we don't have power
                    PodCooling += SimLibMajorFrameTime;
            }

            //RALT stuff
            //Cobra Hack attempt to keep RALT ON and CoolTime down when Combat AI is engaged
            //and disengaged.
            if (autopilotType == CombatAP and RALTCoolTime not_eq -1.0f)
            {
                RALTCoolTime = -1.0f;
                RALTStatus = RON;
            }
            //made this an else if, the rest is orig code.
            else if (HasPower(RaltPower) and RALTCoolTime > -1.0F and RALTStatus not_eq ROFF)
                RALTCoolTime -= SimLibMajorFrameTime;
            else
                RALTCoolTime += SimLibMajorFrameTime;
        }
    }

    //START_PROFILE("AC_EXEC_VEH");
    // Base Class Exec

    //RV - I-Hawk - Removed the burning SFX stuff here as not handled in simveh.cpp anymore...
    if (pctStrength <= 0.0f and not IsExploding() and (DrawableBSP*)drawPointer)
    {
        Trotation *orientation = &((DrawableBSP*)drawPointer)->orientation;
        Tpoint pos, vec;

        if (gSfxLOD >= 0.5f)
        {
            // update dying timer
            dyingTimer += SimLibMajorFrameTime;

            // Do nothing for the 1st part of dying
            if (pctStrength > -0.07f)
            {
            }
            else if (pctStrength > -0.3f)   // I-Hawk - was -0.5f before
            {
                if (dyingTimer > 0.3f)   //
                {
                    //RV - I-Hawk - Randomized burning position...
                    randomizeBurn(*this, PSFX_AC_EARLY_BURNING, pos, vec);

                    for (int i = 0; i < 3; i++)
                    {
                        vec.x = XDelta() * 0.5f + PRANDFloat() * 20.0f;
                        vec.y = YDelta() * 0.5f + PRANDFloat() * 20.0f;
                        vec.z = ZDelta() * 0.5f + PRANDFloat() * 20.0f;

                        DrawableParticleSys::PS_AddParticleEx(
                            (SFX_AC_DEBRIS + 1), &pos, &vec
                        );
                    }

                    // zero out
                    dyingTimer = 0.0f;
                }
            }
            else switch (dyingType)
                {
                    case 5:
                    case SimVehicleClass::DIE_SMOKE:
                        if (dyingTimer > 0.10f + (1.0f - gSfxLOD) * 0.3f)
                        {
                            // run stuff here....
                            //RV - I-Hawk - Randomized burning position...
                            randomizeBurn(*this, PSFX_AC_BURNING_1, pos, vec);
                            dyingTimer = 0;
                        }

                        break;

                    case 6:
                    case SimVehicleClass::DIE_SHORT_FIREBALL:
                        if (dyingTimer > 0.10f + (1.0f - gSfxLOD) * 0.3f)
                        {
                            //RV - I-Hawk - Randomized burning position...
                            randomizeBurn(*this, PSFX_AC_BURNING_3, pos, vec);
                            // reset the timer
                            dyingTimer = 0.0f;
                        }

                        break;

                    case SimVehicleClass::DIE_INTERMITTENT_SMOKE:
                        if (dyingTimer > 0.10f + (1.0f - gSfxLOD) * 0.3f)
                        {
                            //RV - I-Hawk - Randomized burning position...
                            randomizeBurn(*this, PSFX_AC_BURNING_2, pos, vec);
                            // reset the timer
                            dyingTimer = 0.0f;
                        }

                        break;

                    case SimVehicleClass::DIE_FIREBALL:
                        if (dyingTimer > 0.10f + (1.0f - gSfxLOD) * 0.3f)
                        {
                            // run stuff here....
                            //RV - I-Hawk - Randomized burning position...
                            randomizeBurn(*this, PSFX_AC_BURNING_4, pos, vec);
                            // reset the timer
                            dyingTimer = 0.0f;
                        }

                        break;

                    case SimVehicleClass::DIE_INTERMITTENT_FIRE:
                    default:
                        if (dyingTimer > 0.10f + (1.0f - gSfxLOD) * 0.3f)
                        {
                            randomizeBurn(*this, PSFX_AC_BURNING_6, pos, vec);
                            // reset the timer
                            dyingTimer = 0.0f;
                        }

                        break;
                } // end switch
        } // end if LOD
    }

    SimVehicleClass::Exec();
    //STOP_PROFILE("AC_EXEC_VEH");

    if (IsExploding())
    {
        if ( not IsSetFlag(SHOW_EXPLOSION))
        {
            RunExplosion();
            SetFlag(SHOW_EXPLOSION);

            if (this == SimDriver.GetPlayerEntity())
            {
                JoystickPlayEffect(JoyHitEffect, 0);
            }

            SetDead(TRUE);
        }

        return TRUE;
    }
    else if ( not IsDead())
    {
        // Sound effects....
        if (pctStrength > 0.0f)
        {
            VehicleClassDataType *vc = GetVehicleClassData(Type() - VU_LAST_ENTITY_TYPE);
            ShiAssert(FALSE == F4IsBadReadPtr(vc, sizeof * vc));
            int inckpt = OTWDriver.DisplayInCockpit(); // JPO save this

            // JPO - if special case...
            // easter egg: need to check af NULL since we may be non-local
            if (af and af->GetSimpleMode() == SIMPLE_MODE_HF)
            {
                // MLR this appears to not be used anymore
                SoundPos.Sfx(SFX_ENGHELI, 0, 0.5f + af->rpm / 3.0F, 0);
            }
            else
            {
                if (g_bNewEngineSounds)
                {
                    float p, p1, v; // pitch, vol
                    int t;
                    int eng;
                    int engines;
                    float rpm[4];
                    float pwr;

                    if (af->auxaeroData->sndInt == -1)
                    {
                        af->auxaeroData->sndInt = vc->EngineSound;
                    }

                    if ( not IsLocal()) // MLR 2/29/2004 - for MP engine sounds
                    {
                        af->rpm  = specialData.powerOutput;
                        af->rpm2 = specialData.powerOutput2;
                    }

                    if (af->auxaeroData->nEngines == 2)
                    {
                        engines = 2;
                        rpm[0] = af->rpm;
                        rpm[1] = af->rpm2;
                    }
                    else
                    {
                        if (g_bMultiEngineSound)
                        {
                            engines = min(af->auxaeroData->nEngines, 4);
                        }
                        else
                        {
                            engines = 1;
                        }

                        rpm[0] = af->rpm;
                        rpm[1] = af->rpm;
                        rpm[2] = af->rpm;
                        rpm[3] = af->rpm;
                    }


                    for (eng = 0; eng < engines; eng++)
                    {
                        pwr = rpm[eng];

                        if (pwr > 0.01f)
                        {
                            p = pwr / .7f;

                            if ( not isDigital and inckpt and IsLocal())
                            {
                                // internal sounds
                                v = (1.0f - af->auxaeroData->sndIntChart.Lookup(pwr)) * -10000;
                                p1 = af->auxaeroData->sndIntPitchChart.Lookup(pwr);
                                SoundPos.Sfx(af->auxaeroData->sndInt, eng, p1, v);

                                if (af->auxaeroData->sndAbInt)
                                {
                                    v = (1.0f - af->auxaeroData->sndAbIntChart.Lookup(pwr)) * -10000;
                                    p1 = af->auxaeroData->sndAbIntPitchChart.Lookup(pwr);
                                    SoundPos.Sfx(af->auxaeroData->sndAbInt, eng, p1, v);
                                }

                                if (af->auxaeroData->sndInt2)
                                {
                                    v = (1.0f - af->auxaeroData->sndInt2Chart.Lookup(pwr)) * -10000;
                                    p1 = af->auxaeroData->sndInt2PitchChart.Lookup(pwr);
                                    SoundPos.Sfx(af->auxaeroData->sndInt2, eng, p1, v);
                                }
                            }

                            // external sounds
                            v = (1.0f - af->auxaeroData->sndExtChart.Lookup(pwr)) * -10000;
                            p1 = af->auxaeroData->sndExtPitchChart.Lookup(pwr);
                            SoundPos.SfxRel(af->auxaeroData->sndExt, eng, p1, v, af->auxaeroData->engineLocation[eng]);

                            v = (1.0f - af->auxaeroData->sndAbExtChart.Lookup(pwr)) * -10000;
                            p1 = af->auxaeroData->sndAbExtPitchChart.Lookup(pwr);
                            SoundPos.SfxRel(af->auxaeroData->sndAbExt, eng, p1, v, af->auxaeroData->engineLocation[eng]);

                            v = (1.0f - af->auxaeroData->sndExt2Chart.Lookup(pwr)) * -10000;
                            p1 = af->auxaeroData->sndExt2PitchChart.Lookup(pwr);
                            SoundPos.SfxRel(af->auxaeroData->sndExt2, eng, p1, v, af->auxaeroData->engineLocation[eng]);
                        }

                        pwr = af->rpm2;
                    }

                    for (t = 0; t < 4 and af->auxaeroData->sndAero[t]; t++)
                    {
                        v = (af->auxaeroData->sndAeroAOAChart[t].Lookup(af->alpha) *
                             af->auxaeroData->sndAeroSpeedChart[t].Lookup(af->mach) - 1.0f)
                            * 10000;
                        SoundPos.Sfx(af->auxaeroData->sndAero[t], t, 1, v);
                    }

                    // RV - I-Hawk - Using Airspeed to manage Envicontrolsys.wav sound emission
                    // in cockpit. If using the constant breathing .wav file, it'll be managed
                    // properly
                    if (af->mach >= 0.55f)
                    {
                        float breathingPitch = 0.0f * (af->nzcgs / 4.5f) ;

                        float v;

                        //base volume on Mach speed
                        v = ((af->mach - 1.0f) * (5500.0f));

                        //no need for more volume if it's above 0...
                        if (v > 0)
                        {
                            v = 0;
                        }

                        //if entering high G levels, reduce volume here as the other sound is
                        // starting...
                        if (af->nzcgs > 5.5f)
                        {
                            v += ((5.5f - af->nzcgs) * 1250.0f);
                        }

                        //if velocity gets above stores maximum velocity, reduce volume here as
                        //the other sound is starting...


                        if (GetKias() > af->curMaxStoreSpeed)
                        {
                            v += (((af->curMaxStoreSpeed - GetKias()) / 50.0f) * 1250.0f);
                        }


                        //SoundPos.Sfx( SFX_ENGINEA, 0, GetKias()/450.0f, 0 );
                        SoundPos.Sfx(SFX_ENGINEA, 0, 2, v);
                    }
                }
                else
                {
                    // old engine style
                    if ( not isDigital and inckpt and IsLocal())
                    {
                        // inside sounds
                        if (PowerOutput() > 0.01f)
                        {
                            if (af->auxaeroData->sndInt > 0)
                            {
                                // MLR 2003-11-10 internal engine sound
                                SoundPos.Sfx(af->auxaeroData->sndInt, 0, 0.01f + PowerOutput() , 0);
                            }
                            else
                            {
                                SoundPos.Sfx(vc->EngineSound, 0, 0.01f + PowerOutput() , 0);
                            }
                        }

                        if (PowerOutput() > 1.0F)
                            SoundPos.Sfx(af->auxaeroData->sndAbInt, 0, PowerOutput() - 0.25f , 0);

                        // wind noise
                        SoundPos.Sfx(SFX_ENGINEA, 0, GetKias() / 450.0f, 0);
                    }

                    //else
                    {
                        // outside sounds
                        if (PowerOutput() > 0.01f)
                            SoundPos.Sfx(af->auxaeroData->sndExt, 0, 0.01f + PowerOutput() , 0);

                        if (PowerOutput() > 1.0F)
                            SoundPos.Sfx(af->auxaeroData->sndAbExt, 0, (PowerOutput() - 0.25f) , 0);
                    }
                }

                // stored the original code at the end of file
            }

        }
        else
        {
            // plane is going down: strength < 0
            SoundPos.Sfx(SFX_FIRELOOP, 0, 1.0f + pctStrength * 0.2f , 0);

            if (af)
                af->SetSimpleMode(SIMPLE_MODE_OFF);
        }

        // COBRA - RED - Check for Lite Pool
        if (
            af->gearPos == 1.0F and not (af->gear[1].flags bitand GearData::GearBroken) and 
            IsAcStatusBitsSet(ACSTATUS_EXT_LANDINGLIGHT) and IsAcStatusBitsSet(ACSTATUS_EXT_LIGHTS)
        )
        {
            if ( not pLandLitePool)
            {
                Tpoint pos;
                pos.x = XPos();
                pos.y = YPos();
                pos.z = ZPos();
                pLandLitePool = new DrawableGroundVehicle(MapVisId(VIS_LITEPOOL), &pos, Yaw());
                mInhibitLitePool = TRUE;
                OTWDriver.InsertObject(pLandLitePool);
            }
        }
        else
        {
            CleanupLitePool();
        }

        if (pLandLitePool)
        {

            // COBRA - RED - SWITCH TAXI light on to use real lights
            SetSwitch(9, 1);

            Trotation rot;
            Tpoint relPos;
            Tpoint rotPos;
            float scale;
            float groundLevel;

            rot.M11 = dmx[0][0];
            rot.M21 = dmx[0][1];
            rot.M31 = dmx[0][2];

            rot.M12 = dmx[1][0];
            rot.M22 = dmx[1][1];
            rot.M32 = dmx[1][2];

            rot.M13 = dmx[2][0];
            rot.M23 = dmx[2][1];
            rot.M33 = dmx[2][2];

            // Set the position of the pool relative
            // to the aircraft with a normalized vector

            relPos.x = 1.0F;
            relPos.y = 0.0F;
            relPos.z = 0.23F; // tangent of 13 degrees (down)

            MatrixMult(&rot, &relPos, &rotPos);


            groundLevel = OTWDriver.GetGroundLevel(XPos(), YPos());

            // COBRA - RED - NEW TAXILIGHT
            // The difference btw vehicle and ground
            float ZDiff = ZPos() - groundLevel;
            // The spot light Z is the middle of Nose Gear Height
            float LiteZ = af->GetAeroData(AeroDataSet::NosGearZ) / 2;
            /// calculate the light projection at given angle
            scale = -(ZDiff - LiteZ) / rotPos.z;
            //float z=aeroDataset[af->VehicleIndex()].inputData[AeroDataSet::NosGearY]

            if (rotPos.z <= 0.0F)
            {
                mInhibitLitePool = TRUE;
            }
            else
            {
                mInhibitLitePool = FALSE;

                rotPos.x = XPos() + rotPos.x * (scale + 25.0F);
                rotPos.y = YPos() + rotPos.y * (scale + 25.0F);
                rotPos.z = ZPos() + rotPos.z * scale;

                pLandLitePool->SetScale(1 + (2 - (LiteZ * 4) / -ZDiff));
                pLandLitePool->Update(&rotPos, Yaw());
            }

            pLandLitePool->SetInhibitFlag(mInhibitLitePool);
        }
        else
        {
            // COBRA - RED - SWITCH TAXI light off to disable 3D light
            SetSwitch(9, 0);
        }

        // ACMI Output
        if (gACMIRec.IsRecording() and (SimLibFrameCount bitand 0x0000000f) == 0)
        {
            airPos.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
            airPos.data.type = Type();
            airPos.data.uniqueID = ACMIIDTable->Add(
                                       Id(), (char *)((DrawableBSP*)drawPointer)->Label(), TeamInfo[GetTeam()]->GetColor()
                                   );//.num_;
            airPos.data.x = XPos();
            airPos.data.y = YPos();
            airPos.data.z = ZPos();
            airPos.data.roll = Roll();
            airPos.data.pitch = Pitch();
            airPos.data.yaw = Yaw();
            RadarClass *radar = (RadarClass*)FindSensor(this, SensorClass::Radar);
#if NO_REMOTE_BUGGED_TARGET

            if (radar and radar->CurrentTarget())
            {
                airPos.RadarTarget = ACMIIDTable->Add(radar->CurrentTarget()->BaseData()->Id(), NULL, 0); //.num_;
            }

#else

            if (radar and radar->RemoteBuggedTarget)
            {
                //me123 add record for online targets
                airPos.RadarTarget = ACMIIDTable->Add(radar->RemoteBuggedTarget->Id(), NULL, 0); //.num_;
            }
            else if (radar and radar->CurrentTarget())
            {
                airPos.RadarTarget = ACMIIDTable->Add(radar->CurrentTarget()->BaseData()->Id(), NULL, 0); //.num_;
            }

#endif
            else
            {
                airPos.RadarTarget = -1;
            }

            gACMIRec.AircraftPositionRecord(&airPos);
        }

        if ( not IsLocal())
        {
            ShowDamage();
            af->RemoteUpdate();
            return FALSE;
        }

        //START_PROFILE("AC_EXEC_5d1");
        // Increment the SendStatus Timer
        requestCount ++;

        // edg note: this seems to have no ill effects as far as I can
        // tell.  However I'm commenting it out for the time being until
        // a better solution for targetlist updates is done.  Also, it should
        // be done on some sort of timer anyway....

        // edg note: For AI's I've moved the target list stuff into the digi
        // code.  I want to synchronize doing this and geom calcs with running
        // sensor fusion
        //if ( IsSetFlag( MOTION_OWNSHIP ) )
        if (autopilotType not_eq CombatAP and HasPilot())
        {
            targetList = UpdateTargetList(targetList, this, SimDriver.combinedList);

            // edg: all aircraft now get combined list
            CalcRelGeom(this, targetList, vmat, 1.0F / SimLibMajorFrameTime);

            // Sensors
            RunSensors();

            // electrics
            // Have Missiles?
            if (Sms->HasWeaponClass(wcAimWpn))
            {
                SetFlag(HAS_MISSILES);
            }
            else
            {
                UnSetFlag(HAS_MISSILES);
            }
        }

        DoElectrics(); // JPO - well it has to go somewhere
        //STOP_PROFILE("AC_EXEC_5d1");

        if (OTWDriver.IsActive() // JPO - practice safe viewpoints people :-)
           and not OTWDriver.IsShutdown()) // JB - come on, practice safer viewpoints ;-)
        {
            // JB carrier start
            RViewPoint* viewp = OTWDriver.GetViewpoint();

            if (
                ZPos() > -g_fCarrierStartTolerance and 
                af and af->vcas < .01 and viewp and viewp->GetGroundType(XPos(), YPos()) == COVERAGE_WATER
            )
            {
                // We just started inside the carrier
                if (isDigital)
                {
                    af->z = (float)(-500 + rand() % 200);
                    af->vcas = (float)(250 + rand() % 100);
                    af->vt = (float)(367 + rand() % 100);
                    af->ClearFlag(AirframeClass::OverRunway);
                    af->ClearFlag(AirframeClass::OnObject);
                    af->ClearFlag(AirframeClass::Planted);
                    af->platform->UnSetFlag(ON_GROUND);
                    af->SetFlag(AirframeClass::InAir);
                    PreFlight();
                    af->ClearFlag(AirframeClass::EngineOff);
                    //TJL 01/22/04 multi-engine
                    af->ClearFlag(AirframeClass::EngineOff2);
                }
                else
                {
                    targetList = UpdateTargetList(targetList, this, SimDriver.combinedList);
                }
            }
            // JB carrier end
            else if (isDigital and DBrain() and not DBrain()->IsSetATC(DigitalBrain::DonePreflight))
            {
                //in multiplay this sometimes gets fucked up for the player flight
                // JPO - changed > to < here --- otherwise everything start started up
                if (curWaypoint and curWaypoint->GetWPArrivalTime() - 2 * CampaignMinutes < TheCampaign.CurrentTime)
                {
                    //me123 this should never happen, but it does sometimes in MP
                    PreFlight();

                    //MI if we don't tell our brain that we've preflighted, we keep on looping in that function.....
                    if (isDigital and DBrain() and not DBrain()->IsSetATC(DigitalBrain::DonePreflight))
                    {
                        DBrain()->SetATCFlag(DigitalBrain::DonePreflight);
                    }

                    // Turn on the canopy.
                    SetSwitch(COMP_CANOPY, TRUE);

                    if (gACMIRec.IsRecording())
                    {
                        acmiSwitch.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
                        acmiSwitch.data.type = Type();
                        acmiSwitch.data.uniqueID = Id();
                        acmiSwitch.data.switchNum = COMP_CANOPY;
                        acmiSwitch.data.switchVal = TRUE;
                        acmiSwitch.data.prevSwitchVal = TRUE;
                        gACMIRec.SwitchRecord(&acmiSwitch);
                    }
                }
            }
        }

        //START_PROFILE("AC_EXEC_5d3");
        // after geometry has been calculated, we can check collisions
        // digitals don't check collisions.  they just avoid.
        // this shouldn't really matter much, but it can be easily turned on

        //if ( not isDigital and PlayerOptions.CollisionsOn() ) // JB carrier
        if ( not isDigital)
        {
            // JB carrier
            CheckObjectCollision();
        }

        // Get the controls
        GatherInputs();

        if (tempSensorArray)
        {
            //when we need to change the sensors for the player we just set tempSensorArray equal
            //to sensorarray to save them for deletion at this time, and then
            for (int i = 0; i < tempNumSensors; i++)
            {
                delete tempSensorArray[i];
                tempSensorArray[i] = NULL;
            }

            delete [] tempSensorArray;
            tempSensorArray = NULL;
            tempNumSensors = 0;
        }

        // Predict GLOC
        // 2002-01-27 MN AI can't handle GLOC at all which causes them to lawndart at full afterburner
        // for now we remove GLOC for AI and only apply it on the player's aircraft
        // 2002-03-08 make AI Gloc configurable for testing
        // RV - Biker - Don't blackout for AI -> they cannot handle atm
        if (SimDriver.GetPlayerEntity() == this /*or g_bAIGloc*/)
        {
            if ( not PlayerOptions.BlackoutOn())
            {
                glocFactor = 0.0F;
            }
            else
            {
                glocFactor = GlocPrediction();
            }
        }

        // 2002-02-12 MN Check if our mission target has been occupied by our ground troops and let AWACs say something useful
        // 2002-03-03 MN fix, only check strike mission flights
        flight = (Flight)GetCampaignObject();

        if (flight and (flight->GetUnitMission() > AMIS_SEADESCORT and flight->GetUnitMission() < AMIS_FAC))
        {
            flightIdx = flight->GetComponentIndex(this);

            if ( not AWACSsaidAbort and not flightIdx) // only for flight leaders of course
            {
                WayPoint w = flight->GetFirstUnitWP();

                while (w)
                {
                    if ( not (w->GetWPFlags() bitand WPF_TARGET))
                    {
                        w = w->GetNextWP();
                        continue;
                    }

                    CampEntity target;
                    target = w->GetWPTarget();

                    if (target and (target->GetTeam() == GetTeam()))
                    {
                        AWACSsaidAbort = true;
                        FalconRadioChatterMessage *radioMessage;
                        radioMessage = CreateCallFromAwacs(flight, rcAWACSTARGETOCCUPIED);
                        // let AWACS bring the message that we've occupied the target ;)
                        FalconSendMessage(radioMessage, FALSE);
                        TheCampaign.MissionEvaluator->Register3DAWACSabort(flight);
                    }

                    break;
                }
            }
        }

        //Cobra AI JDAMs need a target.  Go through PB target list and select a target
        // Cobra - bypass HP = 0 (guns) and HP= -1 (CTD)
        if ((isDigital or Sms->JDAMtargeting == SMSBaseClass::PB) and Sms->GetCurrentHardpoint() > 0)
        {
            SimWeaponClass *sw = static_cast<BombClass*>(Sms->hardPoint[Sms->GetCurrentHardpoint()]->weaponPointer.get());

            if (((Sms->hardPoint[Sms->GetCurrentHardpoint()]->GetWeaponType() == wtGPS)
                 or (sw and (((BombClass *)sw)->IsSetBombFlag(BombClass::IsJSOW)))))
            {
                if (this)
                    JDAMtgtnum = GetJDAMPBTarget((AircraftClass*)this);
            }
        }

        //End


        // 2002-01-29 ADDED BY S.G. Run our targetSpot update code...
        if ((SimDriver.GetPlayerEntity() == this) and DBrain())
        {
            if (DBrain()->targetSpotWing)
            {
                // First See if the timer has elapsed or the target is died
                if (
                    SimLibElapsedTime > DBrain()->targetSpotWingTimer or
 not DBrain()->targetSpotWingTarget or DBrain()->targetSpotWingTarget->IsDead()
                )
                {
                    // 2002-03-07 MODIFIDED BY S.G.
                    //Added 'or not DBrain()->targetSpotWingTarget' but should NOT be required
                    // If so, kill the camera and clear out everything associated with this targetSpot
                    // sfr: cleanup camera mess
                    FalconLocalSession->RemoveCamera(DBrain()->targetSpotWing);
                    //for (unsigned char i = 0; i < FalconLocalSession->CameraCount(); ++i) {
                    // if (FalconLocalSession->GetCameraEntity(i) == DBrain()->targetSpotWing) {
                    // FalconLocalSession->RemoveCamera(DBrain()->targetSpotWing);
                    // break;
                    // }
                    //}

                    // Takes care of deleting the allocated memory and the driver allocation as well.
                    vuDatabase->Remove(DBrain()->targetSpotWing);

                    // 2002-03-07 ADDED BY S.G. In case it's NULL. Shouldn't happen but happened
                    if (DBrain()->targetSpotWingTarget)
                    {
                        VuDeReferenceEntity(DBrain()->targetSpotWingTarget);
                    }

                    DBrain()->targetSpotWing = NULL;
                    DBrain()->targetSpotWingTarget = NULL;
                    DBrain()->targetSpotWingTimer = 0;
                }
                else
                {
                    // Update the position and exec the driver
                    DBrain()->targetSpotWing->SetPosition(
                        DBrain()->targetSpotWingTarget->XPos(),
                        DBrain()->targetSpotWingTarget->YPos(),
                        DBrain()->targetSpotWingTarget->ZPos()
                    );
                    DBrain()->targetSpotWing->EntityDriver()->Exec(vuxGameTime);
                }
            }

            if (DBrain()->targetSpotElement)
            {
                // First See if the timer has elapsed or the target is died
                if (
                    SimLibElapsedTime > DBrain()->targetSpotElementTimer or
 not DBrain()->targetSpotElementTarget or
                    DBrain()->targetSpotElementTarget->IsDead()
                )
                {
                    // 2002-03-07 MODIFIDED BY S.G. Added 'or
                    // not DBrain()->targetSpotElementTarget' but should NOT be required
                    // sfr: cleanup camera mess
                    FalconLocalSession->RemoveCamera(DBrain()->targetSpotElement);
                    // If so, kill the camera and clear out everything associated with this targetSpot
                    //for (unsigned char i = 0; i < FalconLocalSession->CameraCount(); i++) {
                    // if (FalconLocalSession->GetCameraEntity(i) == DBrain()->targetSpotElement) {
                    // FalconLocalSession->RemoveCamera(DBrain()->targetSpotElement);
                    // break;
                    // }
                    //}
                    // Takes care of deleting the allocated memory and the driver allocation as well.
                    vuDatabase->Remove(DBrain()->targetSpotElement);

                    if (DBrain()->targetSpotElementTarget)
                    {
                        // 2002-03-07 ADDED BY S.G. In case it's NULL. Shouldn't happen but happened
                        VuDeReferenceEntity(DBrain()->targetSpotElementTarget);
                    }

                    DBrain()->targetSpotElement = NULL;
                    DBrain()->targetSpotElementTarget = NULL;
                    DBrain()->targetSpotElementTimer = 0;
                }
                else
                {
                    // Update the position and exec the driver
                    DBrain()->targetSpotElement->SetPosition(
                        DBrain()->targetSpotElementTarget->XPos(),
                        DBrain()->targetSpotElementTarget->YPos(),
                        DBrain()->targetSpotElementTarget->ZPos()
                    );
                    DBrain()->targetSpotElement->EntityDriver()->Exec(vuxGameTime);
                }
            }

            if (DBrain()->targetSpotFlight)
            {
                // First See if the timer has elapsed or the target is died
                if (SimLibElapsedTime > DBrain()->targetSpotFlightTimer or
 not DBrain()->targetSpotFlightTarget or
                    DBrain()->targetSpotFlightTarget->IsDead()
                   )
                {
                    // 2002-03-07 MODIFIDED BY S.G.
                    // Added 'or not DBrain()->targetSpotFlightTarget' but should NOT be required
                    // If so, kill the camera and clear out everything associated with this targetSpot
                    // sfr: cleanup camera mess
                    FalconLocalSession->RemoveCamera(DBrain()->targetSpotFlight);
                    //for (int i = 0; i < FalconLocalSession->CameraCount(); i++) {
                    // if (FalconLocalSession->GetCameraEntity(i) == DBrain()->targetSpotFlight) {
                    // FalconLocalSession->RemoveCamera(DBrain()->targetSpotFlight);
                    // break;
                    // }
                    //}
                    // Takes care of deleting the allocated memory and the driver allocation as well.
                    vuDatabase->Remove(DBrain()->targetSpotFlight);

                    if (DBrain()->targetSpotFlightTarget)
                    {
                        // 2002-03-07 ADDED BY S.G. In case it's NULL. Shouldn't happen but happened
                        VuDeReferenceEntity(DBrain()->targetSpotFlightTarget);
                    }

                    DBrain()->targetSpotFlight = NULL;
                    DBrain()->targetSpotFlightTarget = NULL;
                    DBrain()->targetSpotFlightTimer = 0;
                }
                else
                {
                    // Update the position and exec the driver
                    DBrain()->targetSpotFlight->SetPosition(
                        DBrain()->targetSpotFlightTarget->XPos(),
                        DBrain()->targetSpotFlightTarget->YPos(),
                        DBrain()->targetSpotFlightTarget->ZPos()
                    );
                    DBrain()->targetSpotFlight->EntityDriver()->Exec(vuxGameTime);
                }
            }
        }

        //START_PROFILE("AC_EXEC_5g");

        /*------------------------*/
        /* Fly the airframe.      */
        /*------------------------*/

        // edg TODO: we're calculating ground level in too many places (the
        // airframe does this too).  There should be some place this is
        // done 1 time every frame to optimize.  Att the moment I'm just trying
        // to move LandingCheck out of here and into the airframe class
        //af->groundZ = OTWDriver.GetGroundLevel(af->x, af->y, &af->gndNormal);

        // bb pullup goes here?
        // factors from manual:
        // greater than 50ft AGL
        // descent rate > 960 fpm
        // landing gear up
        if ( not isDigital and 
            ZPos() - af->groundZ < -50.0f and 
            af->gearPos <= 0.0f and 
            af->zdot > 960.0f / 60.0f and 
            Pitch() < 0.0f
           )
        {
            float turnRadius, denom, gs, num, alt;
            float groundAlt;
            float clearanceBuffer;

            groundAlt = -af->groundZ;

            /*--------------------------------*/
            /* Find current state turn radius */
            /*--------------------------------*/
            gs = af->MaxGs() - 0.5F;
            gs = min(gs, 7.0F);
            num = gs * gs - 1.0F;

            if (num <= 0.0)
                denom = 0.1F;
            else
                denom = GRAVITY * (float)sqrt(num);

            // get turn radius length nased on our speed and gs
            float vt = GetVt();
            turnRadius = vt * vt / denom;

            // turnRadius should be equivalent to the drop in altitude
            // we'd make assuming we were pointed straight down and trying
            // to reach level flight.  modify the radius based on our
            // current pitch.  Remember our pitch should be negative at this
            // point
            turnRadius *= -Pitch() / (90.0f * DTR);

            // clearance buffer depends on vcas
            if (af->vcas <= 325.0f)
            {
                clearanceBuffer = 150.0f;
            }
            else if (af->vcas >= 375.0f)
            {
                clearanceBuffer = 50.0f;
            }
            else // between 325 and 375
            {
                clearanceBuffer = 150.0f - 2.0f * (af->vcas - 325.0f);
            }

            // get alt AGL (positive)
            alt = (-ZPos()) - groundAlt - clearanceBuffer;

            // num = alt / (2.0F*turnRadius);
            // if (num > 1.0F)

            // if our AGL is greater than our predicted turn radius plus some
            // cushion,
            // the clear the fault.  otherwise issue warning...
            if (alt > turnRadius * 1.15f)
            {
                // no pullup needed
                mFaults->ClearFault(alt_low);
            }
            else
            {
                if ( not SoundPos.IsPlaying(af->auxaeroData->sndBBPullup))
                {
                    SoundPos.Sfx(af->auxaeroData->sndBBPullup);
                }

                mFaults->SetFault(alt_low);

            }
        }
        else if ( not isDigital)
        {
            mFaults->ClearFault(alt_low);
        }

        // check for collision with feature
        if (af->vt > 0.0f and not isDigital and PlayerOptions.CollisionsOn())
            GroundFeatureCheck(af->groundZ);
        else
            FeatureCollision(af->groundZ);   //need onFlatFeature to be correct for landings

        //onFlatFeature = TRUE;

        if (OnGround())
        {

            // sfr: this is causing problems in server mode
            // RED - Floting/sunked Ground park fix - if planted, refresh Z ground position
            //if (isDigital and not af->IsSet(AirframeClass::OnObject) and af->IsSet(AirframeClass::Planted)){
            // SetPosition (af->x, af->y, OTWDriver.GetGroundLevel(af->x, af->y));
            //}

            //MI
            if (IsPlayer() and g_bRealisticAvionics)
            {
                UnSetFlag(ECM_ON);
            }

            // Check for special IA end condition
            if (SimDriver.RunningInstantAction() and this == SimDriver.GetPlayerEntity())
            {
                if (af->vt < 5.0F and af->throtl < 0.1F)
                {
                    if ( not gPlayerExitMenuShown)
                    {
                        gPlayerExitMenuShown = TRUE;
                        OTWDriver.EndFlight();
                    }
                }
                else
                {
                    gPlayerExitMenuShown = FALSE;
                }
            }
        }
        else
        {
            // fun stuff: if between 10 and 80 ft of ground, stirr up
            // dust or water
            // JAM - FIXME
            //if ( OTWDriver.renderer and OTWDriver.renderer->GetAlphaMode() )
            //{
            int connected = 0;

            // MLR 1/3/2004 - this is a retarded effect.
            if (ZPos() - af->groundZ >= -40.0f and 
                ZPos() - af->groundZ <= -10.0f)
            {
                //if we are over a runway, we don't want to kick up dust
                if ( not af->IsSet(AirframeClass::OverRunway) and af->gearPos <= 0.7F)
                {
                    int groundType;
                    // Check ground type
                    groundType = OTWDriver.GetGroundType(XPos(), YPos());

                    if (groundType not_eq COVERAGE_ROAD)
                    {
                        connected = 1;
                        dustConnect = 1;
                        Tpoint mvec;
                        Tpoint pos;
                        int theSFX;

                        pos.x = XPos();
                        pos.y = YPos();
                        pos.z = af->groundZ - 3.0f;

                        mvec.x = 20.0f * PRANDFloat();
                        mvec.y = 20.0f * PRANDFloat();
                        mvec.z = -30.0f;

                        //RV - I-Hawk - Not using the dust trail anymore

                        ///*
                        //if ( not dustTrail){
                        // /*
                        // if (dustTrail=new DrawableTrail(TRAIL_DUSTCLOUD))
                        // {
                        // OTWDriver.InsertObject(dustTrail);
                        // }*/
                        // dustTrail = TRAIL_DUSTCLOUD;
                        //}
                        //*/

                        //I-Hawk - Use PS for dust/mist effect
                        if ( not (groundType == COVERAGE_WATER or groundType == COVERAGE_RIVER))
                        {
                            theSFX = SFX_DUSTCLOUD;
                        }

                        else
                        {
                            theSFX = SFX_WATERCLOUD;
                        }

                        //dustTrail->AddPointAtHead(&pos,0);
                        //dustTrail->SetHeadVelocity(&mvec);

                        /*
                        dustTrail_trail = DrawableParticleSys::PS_EmitTrail(
                         dustTrail_trail, dustTrail, pos.x, pos.y, pos.z
                         );
                         */

                        DrawableParticleSys::PS_AddParticleEx(
                            (theSFX + 1), &pos, &mvec);

                    }
                }
            }

            if ( not connected and dustConnect and dustTrail)
            {
                //dustTrail->TrimTrail(0);
                dustConnect = FALSE;
                DrawableParticleSys::PS_KillTrail(dustTrail_trail);
                dustTrail = dustTrail_trail = NULL;
            }
        }

        if (SimDriver.MotionOn())
        {
            af->Exec();
        }

        //STOP_PROFILE("AC_EXEC_5g");

        //START_PROFILE("AC_EXEC_5h");

        // JPO - current home of lantirn execution
        // JB 010325 Only do this for the player
        //if (theLantirn and theLantirn->IsEnabled() and IsSetFlag(MOTION_OWNSHIP))
        if (theLantirn and theLantirn->IsEnabled() and not isDigital)
        {
            theLantirn->Exec(this);
        }

        //TJL 02/28/04 Speedbrake specific coding
        SetSpeedBrake();

        // Spped brake sound
        if (af->dbrake > 0.1f and this == SimDriver.GetPlayerEntity() and GetKias() > 100.0F)
        {
            SoundPos.Sfx(af->auxaeroData->sndSpdBrakeWind, 0, GetKias() / 450.0f, (1.0F - af->speedBrake) * -100.0F);
        }

        if (af->IsSet(AirframeClass::Refueling) and DBrain() and not DBrain()->IsTanker())
        {
            AircraftClass *tanker = NULL;
            tanker = (AircraftClass*)vuDatabase->Find(DBrain()->Tanker());
            float refuelRate = af->GetRefuelRate();
            float refuelHelp = (float)PlayerOptions.GetRefuelingMode();

            if (this not_eq SimDriver.GetPlayerEntity() and g_fAIRefuelSpeed)
                refuelHelp = g_fAIRefuelSpeed;

            if ( not tanker or not tanker->IsAirplane() or
 not af->AddFuel(refuelRate * SimLibMajorFrameTime * refuelHelp * refuelHelp)
               )
            {
                FalconTankerMessage *TankerMsg;

                if (tanker)
                {
                    TankerMsg = new FalconTankerMessage(tanker->Id(), FalconLocalGame);
                }
                else
                {
                    TankerMsg = new FalconTankerMessage(FalconNullId, FalconLocalGame);
                }

                TankerMsg->dataBlock.type = FalconTankerMessage::DoneRefueling;
                TankerMsg->dataBlock.caller = Id();
                TankerMsg->dataBlock.data1  = 0;
                FalconSendMessage(TankerMsg);
            }
        }

        /*--------------------*/
        /* Update shared data */
        /*--------------------*/

        SetPosition(af->x, af->y, af->z);
        SetDelta(af->xdot, af->ydot, af->zdot);
        SetYPR(af->psi, af->theta, af->phi);
        SetYPRDelta(af->r, af->q, af->p);
        // sfr: no need for this anymore
        //SetVt (af->vt);
        //SetKias(af->vcas);
        // 2000-11-17 MODIFIED BY S.G.
        // SO OUR SetPowerOutput WITH ENGINE TEMP IS USED INSTEAD OF THE SimBase CLASS ONE
        SetPowerOutput(af->rpm);
        //me123 back SetPowerOutput(af->rpm, af->oldp01[0]);
        // END OF MODIFIED SECTION

        // Weapons
        if (autopilotType not_eq CombatAP)
        {
            SetTarget(FCC->Exec(targetPtr, targetList, theInputs));
        }
        else
        {
            // edg: emperical hack.  The FCC is exec'd in digi anyway
            // when bombing run.  Doing it here as well also screws up
            // digi's releasing bombs
            SimObjectType* tmpTarget = targetPtr;

            if (FCC->GetMasterMode() == FireControlComputer::AirGroundMissile and DBrain())
            {
                tmpTarget = DBrain()->GetGroundTarget();
            }
            else if (FCC->GetMasterMode() == FireControlComputer::AirGroundLaser)
            {
                tmpTarget = NULL;
            }

            //    if (FCC->GetMasterMode() not_eq FireControlComputer::AirGroundBomb)
            // TODO
            // COBRA - RED - WE HAD A CTD from this following call just entering 3D, still dunno why...
            SetTarget(FCC->Exec(tmpTarget, targetList, theInputs));
        }

        DoWeapons();

        // Handle missiles launched but still on the rail
        Sms->Exec();

        if (TheHud and TheHud->Ownship() == this and Guns not_eq NULL)
        {
            TheHud->SetEEGSData(af->x, af->y, af->z, af->gmma, af->sigma,
                                af->theta, af->psi, af->vt);
        }

        if (IsDying())
            SetDead(TRUE);

        // Do countermeasures
        DoCountermeasures();

        // KCK: Really, this is the only safe way to set campaign positions -
        UnitClass *campObj = (UnitClass*) GetCampaignObject();

        if (campObj and campObj->IsLocal() and campObj->GetComponentLead() == this)
        {
            campObj->SimSetLocation(af->x, af->y, af->z);

            if (campObj->IsFlight())
                campObj->SimSetOrientation(af->psi, af->theta, af->phi);
        }

        //Check cautions
        CautionCheck();
        //STOP_PROFILE("AC_EXEC_5h");
    }

    //START_PROFILE("AC_EXEC_6");

    // If ownship update control surfaces
    if ( not IsExploding())
        MoveSurfaces();

    if ( not IsDead())
        ShowDamage();

    //MI no parking brake while in Air
    if (af and af->IsSet(AirframeClass::InAir) and af->PBON)
        af->PBON = FALSE;

    //TJL 08/01/04 Refuel in DF //Cobra 10/30/04 TJL
    HotPitRefuel();

    //Cobra 11/20/04 IFF Do we have it? Is it on? Is it working?
    if (
        (af->auxaeroData->hasIFF == 1 or g_bAllHaveIFF) and 
        HasPower(AircraftClass::IFFPower) and ( not mFaults->GetFault(FaultClass::iff_fault))
    )
    {
        iffEnabled = TRUE;
    }
    else
    {
        iffEnabled = FALSE;
    }

    //Cobra 11/21/04 Run Interrogation through modes
    if (runIFFInt)
    {
        if (iffModeChallenge == 99)
        {
            iffModeChallenge = 4;
            iffModeTimer = SimLibElapsedTime + 1000.0f;
        }
        else if ((iffModeChallenge <= 4 and iffModeChallenge > 0) and (iffModeTimer < SimLibElapsedTime))
        {
            iffModeChallenge --;
            iffModeTimer = SimLibElapsedTime + 1000.0f;
        }
        else if (iffModeChallenge == 0)
        {
            interrogating = TRUE;
            runIFFInt = FALSE;
        }

    }
    else
    {
        runIFFInt = FALSE;
        iffModeTimer = 0.0f;
        iffModeChallenge = 99;
        interrogating = FALSE;
    }

    //Cobra CombatAP doesn't turn on Master Arm for human pilot .. So, let's fix that.
    // sfr: TODO must go to initialization. fucking hack
    if (Sms)
    {
        if (
            (this == SimDriver.GetPlayerEntity()) and (autopilotType == CombatAP) and 
            (Sms->MasterArm() == SMSBaseClass::Safe) and not OnGround()
        )
        {
            Sms->SetMasterArm(SMSBaseClass::Arm);
        }
    }

    // RV - Biker - Set carrierInitTimer to 35 if we're moving
    if (ZPos() < -150.0f or af->Thrust() > 5.0f)
        carrierInitTimer = 35.0f;

    //Cobra lights
    // sfr: check light stuff here
    if (isDigital and curWaypoint)
    {
        if (
            curWaypoint->GetWPAction() not_eq WP_TAKEOFF
           and curWaypoint->GetWPAction() not_eq WP_LAND
           and (IsAcStatusBitsSet(ACSTATUS_EXT_LIGHTS))
        )
        {
            ExtlOff(Extl_Main_Power);
            ExtlOff(Extl_Anti_Coll);
            ExtlOff(Extl_Wing_Tail);
            ExtlOff(Extl_Flash);
        }
        else if (curWaypoint->GetWPAction() == WP_LAND and not (IsAcStatusBitsSet(ACSTATUS_EXT_LIGHTS)))
        {
            ExtlOn(Extl_Main_Power);
            ExtlOn(Extl_Anti_Coll);
            ExtlOn(Extl_Wing_Tail);
            ExtlOn(Extl_Flash);
        }
    }

    //STOP_PROFILE("AC_EXEC_6");
    //STOP_PROFILE("AC_EXEC");
    return TRUE;
}

void AircraftClass::RunSensors(void)
{
    SimObjectType* object;
    SensorClass *sensor = NULL;
    int i;

    // RV - MASTERFIX - if It's a player and there is no more the player entity, avoid all this stuff
    if (( not isDigital) and ( not SimDriver.GetPlayerAircraft())) return;

    // MD -- 20040110: if player is using analog cursor controls, grab values now if player
    // is not using CombatAP.  Both X and Y must be mapped for analog to work at all.
    if ((IO.AnalogIsUsed(AXIS_CURSOR_X) == true) and (IO.AnalogIsUsed(AXIS_CURSOR_Y) == true))
    {
        if ( not isDigital or autopilotType == CombatAP)
        {
            SetCursorCmdsByAnalog();
        }
    }

    // Now run all our sensors
    for (i = 0; i < numSensors; i++)
    {
        //if we are in the middle of resetting our sensors bail
        if (numSensors)
        {
            sensor = sensorArray[i];
        }
        else
        {
            return;
        }

        // Do player control processing
        //if ( not isDigital){
        if ( not isDigital or autopilotType == CombatAP)
        {
            sensor->ExecModes(FCC->designateCmd, FCC->dropTrackCmd);
            sensor->UpdateState(FCC->cursorXCmd, FCC->cursorYCmd);
        }

        // 2000-09-25 ADDED BY S.G. SO 'LockedTarget' IS AT LEAST FOLLOWING OR 'targetPtr'
        // Before we run our sensor, we 'push' our targetPtr as its
        // lockedTarget if we are in CombatAP, its lockedTarget is NULL
        // and it's not the targeting pod (from SetTarget code) and we HAVE a target
        // I had to do this because once the radar gimbals out,
        // AI won't reacquire its lockedTarget once it can see it again...
        // If it cannot see it, it will be reset anyway...
        // TODO: Do we need to check if it's an AI? Will it affect the player's radar? Update: Nope
        if (
            autopilotType == CombatAP and 
            sensor->CurrentTarget() == NULL and 
            sensor->Type() not_eq SensorClass::TargetingPod and 
            targetPtr not_eq NULL
        )
        {
            sensor->SetDesiredTarget(targetPtr);
        }

        // END OF ADDED SECTION

        // Run the sensor model
        object = sensor->Exec(targetList);

        //Cobra for some reason Visdetect isn't setting target
        if (object and sensor->Type() == SensorClass::Visual and sensor->Type() not_eq SensorClass::TargetingPod)
        {
            SetTarget(object);
        }

        // Update the system target if necessary
        // NOTE:  This is only part of the story.  Many FCC modes also set the system
        // target directly.  Sad but true...
        if (sensor->Type() == SensorClass::Radar)
        {
            if (FCC->GetMasterMode() not_eq FireControlComputer::AirGroundBomb and 
                FCC->GetMasterMode() not_eq FireControlComputer::AirGroundRocket and // MLR 4/3/2004 -
                FCC->GetMasterMode() not_eq FireControlComputer::AirGroundMissile and 
                FCC->GetMasterMode() not_eq FireControlComputer::AirGroundLaser and 
                FCC->GetMasterMode() not_eq FireControlComputer::AirGroundHARM
               )
            {
                // Our system target becomes our radar target
                SetTarget(object);
            }
            else if (PlayerOptions.GetAvionicsType() == ATEasy and FCC->GetSubMode() == FireControlComputer::CCIP)
            {
                // Let easy mode players get a TD box even in CCIP
                SetTarget(object);
            }
        }
    }
}

void AircraftClass::ReceiveOrders(FalconEvent* theEvent)
{
    if (theBrain)
    {
        theBrain->ReceiveOrders(theEvent);
    }
}

void AircraftClass::RemovePilot(void)
{
    SetAcStatusBits(ACSTATUS_PILOT_EJECTED);
    SetAutopilot(APOff);
}

void AircraftClass::Eject(void)
{
    int i;
    EjectedPilotClass *epc;
    FalconEjectMessage *ejectMessage; // PJW... Yell at me for this :) 12/3/97
    FalconDeathMessage *deathMessage; // SCR/KCK 12/10/98
    FalconEntity *lastToHit;

    if (IsSetFalcFlag(FEC_REGENERATING))
    {
        // In Dogfight - eject is a suicide - regenerates
        // Send an eject message and a death message right away
        ejectMessage = new FalconEjectMessage(Id(), FalconLocalGame);
        ShiAssert(ejectMessage);
        ejectMessage->dataBlock.ePlaneID = Id();
        ejectMessage->dataBlock.eCampID = ((CampBaseClass*)GetCampaignObject())->GetCampID();
        ejectMessage->dataBlock.eFlightID = GetCampaignObject()->Id();
        ejectMessage->dataBlock.ePilotID = pilotSlot;
        ejectMessage->dataBlock.hadLastShooter = (uchar)(LastShooter() not_eq FalconNullId);
        FalconSendMessage(ejectMessage, TRUE);

        deathMessage = new FalconDeathMessage(Id(), FalconLocalGame);
        deathMessage->dataBlock.dEntityID = Id();
        deathMessage->dataBlock.dCampID = ((CampBaseClass*)GetCampaignObject())->GetCampID();
        deathMessage->dataBlock.dSide = ((CampBaseClass*)GetCampaignObject())->GetOwner();
        deathMessage->dataBlock.dPilotID = pilotSlot;
        deathMessage->dataBlock.dIndex = Type();
        deathMessage->dataBlock.fWeaponID = Type();
        deathMessage->dataBlock.fWeaponUID = FalconNullId;
        deathMessage->dataBlock.deathPctStrength = pctStrength;

        lastToHit = (SimVehicleClass*)vuDatabase->Find(LastShooter());

        if (lastToHit and not lastToHit->IsEject())
        {
            deathMessage->dataBlock.damageType = FalconDamageType::OtherDamage;

            if (lastToHit->IsSim())
            {
                SimVehicleClass *lh = static_cast<SimVehicleClass*>(lastToHit);
                deathMessage->dataBlock.fPilotID = lh->pilotSlot;
                deathMessage->dataBlock.fCampID = lh->GetCampaignObject()->GetCampID();
                deathMessage->dataBlock.fSide = lh->GetCampaignObject()->GetOwner();
            }
            else
            {
                CampBaseClass *lh = static_cast<CampBaseClass*>(lastToHit);
                deathMessage->dataBlock.fPilotID = 0;
                deathMessage->dataBlock.fCampID = lh->GetCampID();
                deathMessage->dataBlock.fSide = lh->GetOwner();
            }

            deathMessage->dataBlock.fEntityID = lastToHit->Id();
            deathMessage->dataBlock.fIndex = lastToHit->Type();
        }
        else
        {
            // If aircraft is undamaged, the death message we send is 'ground collision',
            // since the aircraft would probably get there eventually
            deathMessage->dataBlock.damageType = FalconDamageType::OtherDamage;
            deathMessage->dataBlock.fPilotID = pilotSlot;
            deathMessage->dataBlock.fCampID = GetCampaignObject()->GetCampID();
            deathMessage->dataBlock.fSide = GetCampaignObject()->GetOwner();
            deathMessage->dataBlock.fEntityID = Id();
            deathMessage->dataBlock.fIndex = Type();
        }

        deathMessage->RequestOutOfBandTransmit();
        FalconSendMessage(deathMessage, TRUE);
        SetDead(TRUE);
    }
    else
    {
        RemovePilot();

        // make sure we're no longer in simple model
        if (af)
        {
            af->SetSimpleMode(SIMPLE_MODE_OFF);
        }

        VehicleClassDataType *vc = GetVehicleClassData(Type() - VU_LAST_ENTITY_TYPE);

        for (int pilotno = 0; pilotno < vc->NumberOfPilots; pilotno++)
        {
            // Create a new ejected pilot sim object.
            epc = NULL;
            epc = new EjectedPilotClass(this, EM_F16_MODE1, pilotno);

            if (epc not_eq NULL)
            {
                epc->SetSendCreate(VuEntity::VU_SC_SEND_OOB);
                vuDatabase->/*Quick*/Insert(epc);
                epc->Wake();
                // KCK: We should really Wake() this with an overloaded wake function,
                // but the Insertion callback will do most of our waking stuff.

                // Play with this.
                epc->SetTransmissionTime(
                    vuxRealTime + (unsigned long)((float)rand() / RAND_MAX * epc->UpdateRate())
                );
            }

            if ((pilotno == 0) and IsLocal())
            {
                // PJW 12/3/97... added Eject message broadcast
                ejectMessage = new FalconEjectMessage(epc->Id(), FalconLocalGame);

                if (ejectMessage not_eq NULL)
                {
                    ejectMessage->dataBlock.ePlaneID = Id();
                    // ejectMessage->dataBlock.eEjectID = epc->Id();
                    ejectMessage->dataBlock.eCampID = ((CampBaseClass*)GetCampaignObject())->GetCampID();
                    ejectMessage->dataBlock.eFlightID       = GetCampaignObject()->Id();
                    // ejectMessage->dataBlock.eSide = ((CampBaseClass*)GetCampaignObject())->GetOwner();
                    ejectMessage->dataBlock.ePilotID = pilotSlot;
                    // ejectMessage->dataBlock.eIndex = Type();
                    ejectMessage->dataBlock.hadLastShooter = (uchar)(LastShooter() not_eq FalconNullId);
                    FalconSendMessage(ejectMessage, TRUE);
                }

                // If there is a wingman that is not the player send the airman down message, and ask for sar
                for (i = 0; i < GetCampaignObject()->NumberOfComponents(); i++)
                {
                    if (GetCampaignObject()->GetComponentEntity(i) not_eq this and 
 not ((SimBaseClass*)GetCampaignObject()->GetComponentEntity(i))->IsSetFlag(MOTION_OWNSHIP))
                    {
                        SimBaseClass* sender = GetCampaignObject()->GetComponentEntity(i);
                        FalconRadioChatterMessage* msg;

                        msg = new FalconRadioChatterMessage(sender->Id(), FalconLocalGame);
                        msg->dataBlock.message = rcAIRMANDOWNA;
                        msg->dataBlock.from = Id();
                        msg->dataBlock.to = MESSAGE_FOR_TEAM;
                        msg->dataBlock.voice_id = (uchar)sender->GetPilotVoiceId();
                        msg->dataBlock.edata[0] = (short)sender->GetCallsignIdx();
                        msg->dataBlock.edata[1] = (short)sender->GetSlot();
                        msg->dataBlock.time_to_play = 2 * CampaignSeconds;
                        FalconSendMessage(msg, FALSE);
                        // Another...
                        // was rcAIRMANDOWNE - totally wrong  Is now: "setup RESCAP"
                        msg = new FalconRadioChatterMessage(sender->Id(), FalconLocalGame);
                        msg->dataBlock.message = rcAIRMANDOWNF;
                        msg->dataBlock.from = sender->Id();
                        msg->dataBlock.to = MESSAGE_FOR_TEAM;
                        msg->dataBlock.voice_id = (uchar)sender->GetPilotVoiceId();
                        msg->dataBlock.time_to_play = 4 * CampaignSeconds;
                        msg->dataBlock.edata[0] = -1;
                        msg->dataBlock.edata[1] = -1;
                        // Using X and Y position doesn't work...we need bearing and distance...
                        //GetCampaignObject()->GetLocation(&msg->dataBlock.edata[0],&msg->dataBlock.edata[1]);
                        FalconSendMessage(msg, FALSE); // Added - why not send when we've set it up ?

                        // RV - Biker - No SAR missions at all with this
                        // if ( not (rand()%5) and RequestSARMission ((FlightClass*)GetCampaignObject()))
                        if (RequestSARMission((FlightClass*)GetCampaignObject()))
                        {
                            // Generate a SAR radio call from awacs
                            FalconRadioChatterMessage* radioMessage;
                            radioMessage = CreateCallFromAwacs((FlightClass*)GetCampaignObject(), rcSARENROUTE);
                            radioMessage->dataBlock.time_to_play = (5 + rand() % 5) * CampaignSeconds;
                            FalconSendMessage(radioMessage, FALSE);
                        }

                        break;
                    }
                }
            }
        }
    }
}

VU_ID AircraftClass::HomeAirbase(void)
{
    WayPointClass* tmpWaypoint = waypoint;
    VU_ID airbase = FalconNullId;

    while (tmpWaypoint)
    {
        if (tmpWaypoint->GetWPAction() == WP_LAND)
        {
            airbase = tmpWaypoint->GetWPTargetID();
            break;
        }

        tmpWaypoint = tmpWaypoint->GetNextWP();
    }

    return airbase;
}

VU_ID AircraftClass::TakeoffAirbase(void)
{
    WayPointClass* tmpWaypoint = waypoint;
    VU_ID airbase = FalconNullId;

    while (tmpWaypoint)
    {
        if (tmpWaypoint->GetWPAction() == WP_TAKEOFF)
        {
            airbase = tmpWaypoint->GetWPTargetID();
            break;
        }

        tmpWaypoint = tmpWaypoint->GetNextWP();
    }

    return airbase;
}

VU_ID AircraftClass::LandingAirbase(void)
{
    WayPointClass* tmpWaypoint = waypoint;
    VU_ID airbase = FalconNullId;

    while (tmpWaypoint)
    {
        if (tmpWaypoint->GetWPAction() == WP_LAND)
        {
            airbase = tmpWaypoint->GetWPTargetID();
            break;
        }

        tmpWaypoint = tmpWaypoint->GetNextWP();
    }

    return airbase;
}

VU_ID AircraftClass::DivertAirbase(void)
{
    WayPointClass* tmpWaypoint = waypoint;
    VU_ID airbase = FalconNullId;

    while (tmpWaypoint)
    {
        if (tmpWaypoint->GetWPFlags() bitand WPF_ALTERNATE)
        {
            airbase = tmpWaypoint->GetWPTargetID();
            break;
        }

        tmpWaypoint = tmpWaypoint->GetNextWP();
    }

    return airbase;
}

int AircraftClass::CombatClass(void)
{
    return SimACDefTable[((Falcon4EntityClassType*)EntityType())->vehicleDataIndex].combatClass;
}

float AircraftClass::Mass(void)
{
    return af->Mass();
}


void AircraftClass::CleanupLitePool(void)
{
    if (pLandLitePool)
    {
        OTWDriver.RemoveObject(pLandLitePool);
        delete pLandLitePool;
        mInhibitLitePool = TRUE;
        pLandLitePool = NULL;
    }
}

// 2000-11-17 ADDED BY S.G. SO THE ENGINE TEMPERATURE IS SENT AS WELL AS THE RPM
//me123 changed back to original
void AircraftClass::SetPowerOutput(float)
{
    int send = FALSE, diff, value;

    if (af->auxaeroData->nEngines not_eq 2 or af->rpm == af->rpm2)
    {
        // xmit as a single event
        // RPM
        value = FloatToInt32(af->rpm / 1.5f * 255.0f);

        ShiAssert(value >= 0);
        ShiAssert(value <= 0xFF);

        specialData.powerOutput = af->rpm;

        diff = specialData.powerOutputNet - value;

        if ((diff < -g_nMPPowerXmitThreshold) or (diff > g_nMPPowerXmitThreshold) or
 not value and specialData.powerOutputNet) // Xmit if RPM just changed to 0
        {
            //MonoPrint ("%08x SPO %f\n", this, powerOutput);

            specialData.powerOutputNet = static_cast<uchar>(value);
            send = TRUE;
        }

        /*// disabled me123
         // ENGINE TEMP
         value = FloatToInt32(engineTempOutput / 1.6f * 255.0f);

         ShiAssert( value >= 0    );
         ShiAssert( value <= 0xFF );

         specialData.engineHeatOutput = engineTempOutput;

         diff = specialData.engineHeatOutputNet - value;

         if ((diff < -16) or (diff > 16))
         {
         //MonoPrint ("%08x SPO %f\n", this, powerOutput);

         specialData.engineHeatOutputNet = static_cast<uchar>(value);
         send = TRUE;
         }
        */
        // Send if one of the value changed
        if (send)
            //MakeSimBaseDirty (DIRTY_SIM_POWER_OUTPUT, DDP[162].priority);
            MakeSimBaseDirty(DIRTY_SIM_POWER_OUTPUT, SEND_SOMETIME);
    }
    else
    {
        // xmit rpm1
        value = FloatToInt32(af->rpm / 1.5f * 255.0f);

        ShiAssert(value >= 0);
        ShiAssert(value <= 0xFF);

        specialData.powerOutput = af->rpm;

        diff = specialData.powerOutputNet - value;

        if ((diff < -g_nMPPowerXmitThreshold) or (diff > g_nMPPowerXmitThreshold) or
 not value and specialData.powerOutputNet) // Xmit if RPM just changed to 0
        {

            specialData.powerOutputNet = static_cast<uchar>(value);
            MakeSimBaseDirty(DIRTY_SIM_POWER_OUTPUT1, DDP[162].priority);
        }

        // xmit rpm2
        value = FloatToInt32(af->rpm2 / 1.5f * 255.0f);

        ShiAssert(value >= 0);
        ShiAssert(value <= 0xFF);

        specialData.powerOutput2 = af->rpm2;

        diff = specialData.powerOutputNet2 - value;

        if ((diff < -g_nMPPowerXmitThreshold) or (diff > g_nMPPowerXmitThreshold) or
 not value and specialData.powerOutputNet2) // Xmit if RPM just changed to 0
        {
            specialData.powerOutputNet2 = static_cast<uchar>(value);
            MakeSimBaseDirty(DIRTY_SIM_POWER_OUTPUT2, DDP[162].priority);
        }
    }
}
// END OF ADDED SECTION

// JPO all relevant things to on.
void AircraftClass::PreFlight()
{
    RALTCoolTime = -1.0F;
    RALTStatus = RON;
    af->ClearEngineFlag(AirframeClass::MasterFuelOff);
    af->SetFuelSwitch(AirframeClass::FS_NORM);
    af->SetFuelPump(AirframeClass::FP_NORM);
    af->SetAirSource(AirframeClass::AS_NORM);

    // Cobra - Start with canopy open if on parking spot
    if (
        PlayerOptions.GetStartFlag() not_eq PlayerOptionsClass::START_RUNWAY and 
        OnGround() and (af->GetParkType() not_eq LargeParkPt)
    )
    {
        af->canopyState = true;

        if (TheTimeOfDay.GetLightLevel() < 0.65f)
        {
            SetSwitch(COMP_3DPIT_INTERIOR_LIGHTS, TRUE);
            SetAcStatusBits(ACSTATUS_PITLIGHT);
        }
    }
    else
    {
        af->canopyState = false;
        SetSwitch(COMP_3DPIT_INTERIOR_LIGHTS, FALSE);
        ClearAcStatusBits(ACSTATUS_PITLIGHT);
    }

    af->QuickEngineStart();

    //Cobra moved the lights here.  If we start on the ground we can have lights on :)
    // sfr: check light stuff here
    if (OnGround())
    {
        af->TEFTakeoff();
        af->LEFTakeoff();
        //Lights on the ground
        ExtlOn(Extl_Main_Power);
        ExtlOn(Extl_Anti_Coll);
        ExtlOn(Extl_Wing_Tail);
        ExtlOn(Extl_Flash);
    }
    else
    {
        ExtlOff(Extl_Main_Power);
        ExtlOff(Extl_Anti_Coll);
        ExtlOff(Extl_Wing_Tail);
        ExtlOff(Extl_Flash);
    }

    //MI
    SeatArmed = TRUE;
    mainPower = MainPowerMain;
    currentPower = PowerNonEssentialBus;
    EWSPgm = Man;

    //ATARIBABY ground check added - Removed by JPG - was causing AI problems
    //if(Sms)
    // Sms->SetMasterArm(SMSBaseClass::Arm); //set it to arm
    //ATARIBABY Fixed second try to test
    if (Sms)
    {
        if (this == SimDriver.GetPlayerEntity() and OnGround())
            Sms->SetMasterArm(SMSBaseClass::Safe);   //if palyer and on ground then set it to safe
        else
            Sms->SetMasterArm(SMSBaseClass::Arm);    //set it to arm
    }

    // RV - Biker - Only give power to data link if AC is capable
    if (af->GetDataLinkCapLevel() > 0)
    {
        PowerOn(AllPower);
    }
    else
    {
        PowerOn(AvionicsPowerFlags(AllPower bitand compl DLPower));
        PowerOff(DLPower);
    }


    // MLR 2003-10-10
    animStrobeTimer = 0;
    animWingFlashTimer = 0;
    MPWingFlashTimer = 0; //martinv

    //MI INS
    INSOn(INS_Nav);
    INSOff(INS_PowerOff);
    INSOff(INS_AlignNorm);

    if ( not g_bINS)
    {
        INSOn(AircraftClass::INS_ADI_OFF_IN);
        INSOn(AircraftClass::BUP_ADI_OFF_IN);
        INSOn(AircraftClass::INS_ADI_AUX_IN);
        INSOn(AircraftClass::INS_HUD_STUFF);
        INSOn(AircraftClass::INS_HSI_OFF_IN);
        INSOn(AircraftClass::INS_HSD_STUFF);
    }

    HasAligned = TRUE;
    INSStatus = 10;
    INSAlignmentTimer = 480.0F;
    INSLatDrift = 0.0F;
    INSAlignmentStart = vuxGameTime;
    BUPADIEnergy = 540.0F; //9 minutes useable
    //Set the missile and threat volume
    MissileVolume = 0; //max vol
    ThreatVolume = 0;
    //Targeting Pod cooled
    PodCooling = 0.0F;
    //MI HUD
    // if(TheHud and this == SimDriver.GetPlayerEntity() and ( not OnGround() or
    // PlayerOptions.GetStartFlag() not_eq PlayerOptionsClass::START_RAMP))
    //ATARIBABY/WOMBAT Hud sym wheel ramp start fix
    // MD -- 20041216: this has to move to MakePlayerVehicle().  For MP games, it turns out that
    // the AircraftClass instances are preflighted before a player vehicle is marked as such so
    // this code is never called here because by the time you reach MakePlayerVehicle, the DonePreflight
    // flag is already set so PreFlight isn't called from there.  For some reason in single player games
    // the same is not true and this code here gets invoked always.  Strange but true.
    //if(TheHud and this == SimDriver.GetPlayerEntity())
    //{
    // TheHud->SymWheelPos = 1.0F;
    // TheHud->SetLightLevel();
    //}


    if (PlayerOptions.GetStartFlag() not_eq PlayerOptionsClass::START_RAMP and OnGround())
        af->SetFlag(AirframeClass::NoseSteerOn);

    //MI voice volume stuff
    // MD -- 20041216: this has to move to MakePlayerVehicle().  For MP games, it turns out that
    // the AircraftClass instances are preflighted before a player vehicle is marked as such so
    // this code is never called here because by the time you reach MakePlayerVehicle, the DonePreflight
    // flag is already set so PreFlight isn't called from there.  For some reason in single player games
    // the same is not true and this code here gets invoked always.  Strange but true.
    // if(IsPlayer() and OTWDriver.pCockpitManager and OTWDriver.pCockpitManager->mpIcp)
    // {
    //// OTWDriver.pCockpitManager->mpIcp->Comm1Volume = 4; // MLR 2003-10-20 Med vol //was 0, max vol
    //// OTWDriver.pCockpitManager->mpIcp->Comm2Volume = 4;
    //
    // // MD -- 20041211: turn on the radios and set the appropriate default knob settings
    // if (g_bManualRadioTuning)
    // {
    // OTWDriver.pCockpitManager->mpIcp->UfcComm1()->SetPowerOn(TRUE);
    // OTWDriver.pCockpitManager->mpIcp->UfcComm1()->SetSecondaryPowerOn(TRUE);
    // OTWDriver.pCockpitManager->mpIcp->UfcComm1()->SetGuardMonitor(TRUE);
    // OTWDriver.pCockpitManager->mpIcp->UfcComm1()->SetActive(TRUE);
    //
    // OTWDriver.pCockpitManager->mpIcp->BupComm1()->SetPowerOn(TRUE);
    // OTWDriver.pCockpitManager->mpIcp->BupComm1()->SetSecondaryPowerOn(TRUE);
    // OTWDriver.pCockpitManager->mpIcp->BupComm1()->SetGuardMonitor(TRUE);
    // OTWDriver.pCockpitManager->mpIcp->BupComm1()->SetActive(FALSE);
    //
    // OTWDriver.pCockpitManager->mpIcp->UfcComm2()->SetPowerOn(TRUE);
    //
    // if (gNavigationSys)
    // gNavigationSys->SetControlSrc(NavigationSystem::ICP);
    // }
    //
    // if(VM) // MLR 1/29/2004 -
    // {
    // OTWDriver.pCockpitManager->mpIcp->Comm1Volume=RESCALE(PlayerOptions.GroupVol[COM1_SOUND_GROUP],-2000,0,7,0);
    // OTWDriver.pCockpitManager->mpIcp->Comm2Volume=RESCALE(PlayerOptions.GroupVol[COM2_SOUND_GROUP],-2000,0,7,0);
    // if(PlayerOptions.GroupVol[COM1_SOUND_GROUP]<-2000) OTWDriver.pCockpitManager->mpIcp->Comm1Volume=8;
    // if(PlayerOptions.GroupVol[COM2_SOUND_GROUP]<-2000) OTWDriver.pCockpitManager->mpIcp->Comm2Volume=8;
    // F4SetStreamVolume(VM->VoiceHandle(0),PlayerOptions.GroupVol[COM1_SOUND_GROUP]);
    // F4SetStreamVolume(VM->VoiceHandle(1),PlayerOptions.GroupVol[COM2_SOUND_GROUP]);
    // }
    //
    //
    // }



}
void AircraftClass::StepSeatArm(void)
{
    if (SeatArmed)
        SeatArmed = FALSE;
    else
        SeatArmed = TRUE;
}

void AircraftClass::IncMainPower()
{
    switch (mainPower)
    {
        case MainPowerOff:
            mainPower = MainPowerBatt;

            //MI
            if ( not g_bINS)
            {
                INSOn(AircraftClass::INS_ADI_OFF_IN);
                INSOn(AircraftClass::INS_ADI_AUX_IN);;
                INSOn(AircraftClass::INS_HUD_STUFF);
                INSOn(AircraftClass::INS_HSI_OFF_IN);
                INSOn(AircraftClass::INS_HSD_STUFF);
                INSOn(AircraftClass::BUP_ADI_OFF_IN);
                GSValid = TRUE;
                LOCValid = TRUE;
            }

            break;

        case MainPowerBatt:
            mainPower = MainPowerMain;
            break;

        case MainPowerMain: // cant go any further
            break;
    }
}

void AircraftClass::DecMainPower()
{
    switch (mainPower)
    {
        case MainPowerOff:
            break;

        case MainPowerBatt:
            mainPower = MainPowerOff;

            //MI
            if ( not g_bINS)
            {
                INSOff(AircraftClass::INS_ADI_OFF_IN);
                INSOff(AircraftClass::INS_ADI_AUX_IN);;
                INSOff(AircraftClass::INS_HUD_STUFF);
                INSOff(AircraftClass::INS_HSI_OFF_IN);
                INSOff(AircraftClass::INS_HSD_STUFF);
                INSOff(AircraftClass::BUP_ADI_OFF_IN);
                GSValid = FALSE;
                LOCValid = FALSE;
            }

            break;

        case MainPowerMain:
            mainPower = MainPowerBatt;
            break;
    }
}
void AircraftClass::IncEWSPGM()
{
    PlayerRwrClass* theRwr = (PlayerRwrClass*)FindSensor(this, SensorClass::RWR);

    switch (EWSPgm)
    {
        case Off:
            EWSPgm = Stby;

            if (theRwr)
            {
                theRwr->InEWSLoop = FALSE;
                theRwr->ReleaseManual = FALSE;
            }

            break;

        case Stby:
            EWSPgm = Man;

            if (theRwr)
            {
                theRwr->InEWSLoop = FALSE;
                theRwr->ReleaseManual = FALSE;
            }

            break;

        case Man:
            EWSPgm = Semi;
            break;

        case Semi:
            EWSPgm = Auto;
            break;

        case Auto: // cant go any further
            break;
    }
}

void AircraftClass::DecEWSPGM()
{
    PlayerRwrClass* theRwr = (PlayerRwrClass*)FindSensor(this, SensorClass::RWR);

    switch (EWSPgm)
    {
        case Off:
            if (theRwr)
            {
                theRwr->InEWSLoop = FALSE;
                theRwr->ReleaseManual = FALSE;
            }

            break;

        case Auto:
            EWSPgm = Semi;
            break;

        case Semi:
            EWSPgm = Man;
            break;

        case Man:
            EWSPgm = Stby;

            if (theRwr)
            {
                theRwr->InEWSLoop = FALSE;
                theRwr->ReleaseManual = FALSE;
            }

            break;

        case Stby:
            EWSPgm = Off;

            if (theRwr)
            {
                theRwr->InEWSLoop = FALSE;
                theRwr->ReleaseManual = FALSE;
            }

            break;
    }
}
void AircraftClass::IncEWSProg()
{
    PlayerRwrClass* theRwr = (PlayerRwrClass*)FindSensor(this, SensorClass::RWR);

    if (theRwr)
    {
        theRwr->InEWSLoop = FALSE;
        theRwr->ReleaseManual = FALSE;
    }

    if (EWSProgNum < 3)
        EWSProgNum++;
}

void AircraftClass::DecEWSProg()
{
    PlayerRwrClass* theRwr = (PlayerRwrClass*)FindSensor(this, SensorClass::RWR);

    if (theRwr)
    {
        theRwr->InEWSLoop = FALSE;
        theRwr->ReleaseManual = FALSE;
    }

    if (EWSProgNum > 0)
        EWSProgNum--;
}
void AircraftClass::ToggleBetty()
{
    playBetty = not playBetty;
}
//MI
void AircraftClass::AddAVTRSeconds(void)
{
    if (AVTRState(AVTR_AUTO))
        AVTRCountown = 30.0F;
}
void AircraftClass::SetCockpitWingLight(bool state)
{
    CockpitWingLight = state;
}
void AircraftClass::SetCockpitWingLightFlash(bool state)
{
    CockpitWingLightFlash = state;
}
void AircraftClass::SetCockpitStrobeLight(bool state)
{
    CockpitStrobeLight = state;
}
// Cobra - rewrite
int AircraftClass::FindBestSpawnPoint(Objective obj, SimInitDataClass* initData)
{
    int pt = initData->ptIndex;
    int pktype = af->GetParkType();
    int npt = 0;
    int rwindex = initData->rwIndex;
    //ulong takeoffTime;
    runwayQueueStruct *info = 0;

    if (PlayerOptions.GetStartFlag() == PlayerOptionsClass::START_RUNWAY)
    {
        af->vt = 0.0f;
        spawnpoint = initData->ptIndex; //RAS-11Nov04-store initial spawn point
        return initData->ptIndex;
    }

    // FRB - CTD's here
    if (GetNextTaxiPt(initData->ptIndex) == 0 or initData->ptIndex >= -2)
    {
        // means were at first point, try and find a parking spot
        info = 0;

        if (obj->brain and initData->campBase->IsUnit())
        {
            info = obj->brain->InList(initData->campBase->Id());
        }

        rwindex = 0;

        if (info)
        {
            rwindex = info->rwindex;
        }

        if (initData->ptIndex < 0)
        {
            pt = GetFirstPt(initData->rwIndex) + 1;
        }

        pt = DBrain()->FindDesiredTaxiPoint(waypoint->GetWPDepartureTime(), (int) rwindex);

        if (pt)
        {
            // found somewhere
            initData->ptIndex = pt;
            TranslatePointData(obj, pt, &af->x, &af->y);
            npt = GetPrevTaxiPt(pt); // this is next place to go to
            float x, y;
            TranslatePointData(obj, npt, &x, &y);
            af->initialPsi = af->psi = af->sigma = (float)atan2((y - af->y), (x - af->x));
            af->initialX = af->groundAnchorX = af->x;
            af->initialY = af->groundAnchorY = af->y;
            spawnpoint = pt; //RAS-11Nov04-store initial spawn point
            return pt;
        }
    }
    else
    {
        PtDataTable[initData->ptIndex].flags or_eq PT_OCCUPIED; // 02JAN04 - FRB - Reserve parking spot
        spawnpoint = initData->ptIndex; //RAS-11Nov04-store initial spawn point
        return initData->ptIndex;
    }

    spawnpoint = initData->ptIndex; //RAS-11Nov04-store initial spawn point
    PtDataTable[initData->ptIndex].flags or_eq PT_OCCUPIED; // 02JAN04 - FRB - Reserve parking spot
    return initData->ptIndex;
}

// MD -- 20040110: Adding support routine for analog HOTAS cursor control

void AircraftClass::SetCursorCmdsByAnalog(void)
{
    // When analog axis support is mapped for the HOTAS cursor controls, there is no key press event
    // to trigger calculation of the cursor commands for the current frame.  Instead, before updating
    // all the sensors, come here and check for movement commands as represented by non-zero values
    // of the microstick X/Y axis values.  The logic here is pretty much the same as that used in
    // the keyboard routine however.

    if ((IO.AnalogIsUsed(AXIS_CURSOR_X) == false) or (IO.AnalogIsUsed(AXIS_CURSOR_Y) == false))
        return;  // shouldn't be needed but this code only makes sense if both are mapped

    int xValue = IO.GetAxisValue(AXIS_CURSOR_X);
    int yValue = IO.GetAxisValue(AXIS_CURSOR_Y);

    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    if (playerAC not_eq NULL and playerAC->IsSetFlag(MOTION_OWNSHIP))
    {
        if ((xValue not_eq 0) or (yValue not_eq 0))
        {
            // we're moving
            if (g_bRealisticAvionics)
            {

                RadarDopplerClass* theRadar = (RadarDopplerClass*)FindSensor(playerAC, SensorClass::Radar);
                LaserPodClass* laserPod = (LaserPodClass*)FindLaserPod(playerAC);
                MaverickDisplayClass* mavDisplay = NULL;
                HarmTargetingPod *theHTS = (HarmTargetingPod*)FindSensor(playerAC, SensorClass::HTS);

                if (playerAC->Sms->curWeaponType == wtAgm65 and playerAC->Sms->curWeapon)
                {
                    mavDisplay = (MaverickDisplayClass*)((MissileClass*)playerAC->Sms->GetCurrentWeapon())->display;
                }

                if ((theRadar and theRadar->IsSOI()) or (mavDisplay and mavDisplay->IsSOI()) or
                    (laserPod and laserPod->IsSOI()) or (TheHud and TheHud->IsSOI()) or
                    (theHTS and playerAC->GetSOI() == SimVehicleClass::SOI_WEAPON))
                {
                    FCC->cursorXCmd = xValue;
                    FCC->cursorYCmd = yValue;
                }

                else if (FCC->IsSOI)
                {
                    FCC->HSDCursorXCmd = xValue;
                    FCC->HSDCursorYCmd = yValue;
                }
            }
            else
            {
                FCC->cursorXCmd = xValue;
                FCC->cursorYCmd = yValue;
            }
        }
        else  // we're not moving
        {
            FCC->cursorXCmd = 0;
            FCC->cursorYCmd = 0;
            FCC->HSDCursorXCmd = 0;
            FCC->HSDCursorYCmd = 0;
        }
    }
}

float AircraftClass::GetA2GJDAMAlt(void)
{
    return af->auxaeroData->A2GJDAMAlt;
}
float AircraftClass::GetA2GJSOWAlt(void)
{
    return af->auxaeroData->A2GJSOWAlt;
}
float AircraftClass::GetA2GHarmAlt(void)
{
    return af->auxaeroData->A2GHarmAlt;
}
float AircraftClass::GetA2GAGMAlt(void)
{
    return af->auxaeroData->A2GAGMAlt;
}
float AircraftClass::GetA2GGBUAlt(void)
{
    return af->auxaeroData->A2GGBUAlt;
};
float AircraftClass::GetA2GDumbHDAlt(void)
{
    return af->auxaeroData->A2GDumbHDAlt;
}
float AircraftClass::GetA2GClusterAlt(void)
{
    return af->auxaeroData->A2GClusterAlt;
}
float AircraftClass::GetA2GDumbLDAlt(void)
{
    return af->auxaeroData->A2GDumbLDAlt;
}
float AircraftClass::GetA2GGenericBombAlt(void)
{
    return af->auxaeroData->A2GGenericBombAlt;
}
float AircraftClass::GetA2GGunRocketAlt(void)
{
    return af->auxaeroData->A2GGunRocketAlt;
}
float AircraftClass::GetA2GCameraAlt(void)
{
    return af->auxaeroData->A2GCameraAlt;
}
float AircraftClass::GetA2GBombMissileAlt(void)
{
    return af->auxaeroData->A2GBombMissileAlt;
}

//TJL 02/28/04 Speedbrake specific coding
void AircraftClass::SetSpeedBrake(void)
{
    //F15A/B, C/D, E
    if (af->auxaeroData->typeAC == 3 or af->auxaeroData->typeAC == 4 or af->auxaeroData->typeAC == 5)
    {
        if (af->dbrake > 0 and (brakePos == 0 or brakePos == 3))
        {
            brakePos = 1;
            speedBrakeState = af->dbrake;
        }

        if (af->alpha > 25.0f and brakePos == 1)
        {
            af->speedBrake = -1.0f;
            brakePos = 2;
        }

        if (af->alpha < 25.0f and brakePos == 2)
        {
            af->speedBrake = 1.0f;
            brakePos = 1;
        }


    }

    //F14A-D Speedbrake retracts when MIL power or greater
    if (af->auxaeroData->typeAC == 6 or af->auxaeroData->typeAC == 7)
    {
        if (af->rpm >= 1.0f or af->rpm2 >= 1.0f)
        {
            af->speedBrake = -1.0f;
        }
        else if (af->vcas > 400.0f)
        {
            af->speedBrake = -1.0f;
        }


    }

    //F-18A-D if g +6, AOA 28, or tef/down and vcas < 250 retract
    if (af->auxaeroData->typeAC == 8 or af->auxaeroData->typeAC == 9)
    {
        if (af->nzcgb > 6.0f or af->alpha > 28.0f or (af->gearPos > 0.5f and af->vcas < 250.0f))
        {
            af->speedBrake = -1.0f;
        }

    }

}

void AircraftClass::HotPitRefuel()
{
    if (SimDriver.RunningDogfight() and af->vt <= 0.0f and OnGround())
    {
        float refuelRate = af->GetRefuelRate();
        af->AddFuel(refuelRate * SimLibMajorFrameTime);
    }
    else if (af->vt <= 0.0f and OnGround() and not af->IsSet(AirframeClass::OverRunway))
    {
        // sfr: fixing xy order
        GridIndex gx, gy;
        ::vector pos = { af->x, af->y };
        ConvertSimToGrid(&pos, &gx, &gy);
        ObjectiveClass* nearest = FindNearestFriendlyRunway(
                                      FalconLocalSession->GetTeam(),
                                      /*SimToGrid(af->y), SimToGrid(af->x),*/
                                      gx, gy
                                  );

        if (nearest)
        {
            float dist = 0.0f;
            BIG_SCALAR xd = nearest->XPos() - af->x;
            BIG_SCALAR yd = nearest->YPos() - af->y;
            dist = (sqrt(xd * xd + yd * yd) * FT_TO_NM);

            if (dist < 1.0f and requestHotpitRefuel == TRUE)
            {
                float refuelRate = af->GetRefuelRate();
                af->AddFuel(refuelRate * SimLibMajorFrameTime);

                // Turn it off when full
                if (af->AddFuel(refuelRate * SimLibMajorFrameTime) == FALSE)
                {
                    requestHotpitRefuel = FALSE;
                }
            }
        }

    }
}

int AircraftClass::GetJDAMPBTarget(AircraftClass* aircraft)
{
    Flight flight = NULL;
    int flightIdx = 0;
    float x, y, z, xx, yy, zz;
    FalconEntity *wpTarget = NULL;
    ObjClassDataType* oc;
    uchar tgtnum1 = 0;
    flight = (Flight)GetCampaignObject();
    flightIdx = flight->GetComponentIndex(aircraft);
    WayPoint w = flight->GetFirstUnitWP();

    JDAMtargetRange = -1.0f;
    strcpy(JDAMtargetName, "");
    strcpy(JDAMtargetName1, "");

    while (w)
    {
        if ( not (w->GetWPFlags() bitand WPF_TARGET))
        {
            w = w->GetNextWP();
            continue;
        }
        else
            break;
    }

    // FRB - Something is wrong
    if ( not w)
        return -1;

    JDAMtarget = NULL;
    JDAMtarget = w->GetWPTarget();
    wpTarget = w->GetWPTarget();

    if (JDAMtgtnum == -1) // Initialize target selection
    {
        JDAMtgtnum = w->GetWPTargetBuilding() + flightIdx; // Cobra - different starting point for each element
        JDAMStep = 1;
    }

    // RV - Biker, CTD Fix if target is not a possible objective
    if (JDAMtarget and JDAMtarget->IsObjective())
    {
        FeatureClassDataType *fc = NULL;
        oc = ((Objective)JDAMtarget)->GetObjectiveClassData();
        int featnum = oc->Features;
        int Priority = 99;

        if (JDAMStep == 1 and JDAMtgtnum < (featnum - 1))
        {
            while (Priority > 2)
            {
                if (JDAMtgtnum + 1 < featnum)
                {
                    JDAMtgtnum ++;

                    if (((Objective)JDAMtarget)->GetFeatureStatus(JDAMtgtnum) == VIS_DESTROYED)
                        continue;

                    fc = GetFeatureClassData(((Objective)JDAMtarget)->GetFeatureID(JDAMtgtnum));

                    if (fc and not F4IsBadReadPtr(fc, sizeof(fc)))  // higher priority number = lower priority
                        Priority = fc->Priority;
                }
                else
                    break;
            }

            JDAMStep = 0;
        }
        else if (JDAMStep == 1)
        {
            JDAMtgtnum = 0;
            JDAMStep = 0;
        }

        if (JDAMStep == -1 and JDAMtgtnum > 1)
        {
            while (Priority > 2)
            {
                if (JDAMtgtnum - 1 > 0)
                {
                    JDAMtgtnum --;

                    if (((Objective)JDAMtarget)->GetFeatureStatus(JDAMtgtnum) == VIS_DESTROYED)
                        continue;

                    fc = GetFeatureClassData(((Objective)JDAMtarget)->GetFeatureID(JDAMtgtnum));

                    if (fc and not F4IsBadReadPtr(fc, sizeof(fc)))  // higher priority number = lower priority
                        Priority = fc->Priority;
                }
                else
                    break;
            }

            JDAMStep = 0;
        }
        else if (JDAMStep == -1)
        {
            JDAMtgtnum = featnum;
            JDAMStep = 0;
        }

        if ( not ((Objective)JDAMtarget)->IsAggregate() and isDigital and DBrain())
            JDAMtgtnum = DBrain()->FindJDAMGroundTarget((CampBaseClass*)JDAMtarget, featnum, 0);

        JDAMsbc = JDAMtarget->GetComponentEntity(JDAMtgtnum);

        if (JDAMsbc)
        {
            float dx = JDAMsbc->XPos() - XPos();
            float dy = JDAMsbc->YPos() - YPos();
            JDAMtargetRange = SqrtF(dx * dx + dy * dy) * FT_TO_NM;
            JDAMtgtPos.x = JDAMsbc->XPos();
            JDAMtgtPos.y = JDAMsbc->YPos();
            JDAMtgtPos.z = OTWDriver.GetGroundLevel(JDAMtgtPos.x, JDAMtgtPos.y);
        }
        else
        {
            //========================================================================
            // Not Deagg'ed, so calculate where the Feature is placed in the Objective
            fc = GetFeatureClassData(((Objective)JDAMtarget)->GetFeatureID(JDAMtgtnum));

            if (fc and not F4IsBadReadPtr(fc, sizeof(fc)) and not (fc->Flags bitand FEAT_VIRTUAL))
            {
                ((Objective)JDAMtarget)->GetFeatureOffset(JDAMtgtnum, &yy, &xx, &zz);
                w->GetLocation(&x, &y, &z);
                JDAMtgtPos.x = xx + x;
                JDAMtgtPos.y = yy + y;
                JDAMtgtPos.z = OTWDriver.GetGroundLevel(JDAMtgtPos.x, JDAMtgtPos.y);
            }

            //========================================================================
            float dx = x - XPos();
            float dy = y - YPos();
            JDAMtargetRange = SqrtF(dx * dx + dy * dy) * FT_TO_NM;
            JDAMsbc = NULL;
        }

        if ( not fc)
        {
            fc = GetFeatureClassData(((Objective)JDAMtarget)->GetFeatureID(JDAMtgtnum));
        }

        if (fc and not F4IsBadReadPtr(fc, sizeof(fc)))
            strcpy(JDAMtargetName1, fc->Name);
        else
            strcpy(JDAMtargetName1, "xxxx");

        JDAMtarget->GetName(JDAMtargetName, 79, FALSE);

        if ( not JDAMsbc) // Aggregated
            strcat(JDAMtargetName1, " >AGG");
    }
    else
    {
        JDAMtargetRange = -1.0f;
    }

    return JDAMtgtnum;
}

void AircraftClass::MakeAircraftDirty(DirtyAircraft bits, Dirtyness score)
{
    if ( not IsLocal())
    {
        return;
    }

    dirty_aircraft or_eq bits;
    MakeDirty(DIRTY_AIRCRAFT, score);
}

void AircraftClass::ReadDirty(unsigned char **stream, long *rem)
{
    memcpychk(&dirty_aircraft, stream, sizeof(dirty_aircraft), rem);

    if (dirty_aircraft bitand DIRTY_ACSTATUS_BITS)
    {
        memcpychk(&status_bits, stream, sizeof(status_bits), rem);
    }

    // do post processing
    if (IsAcStatusBitsSet(ACSTATUS_PILOT_EJECTED))
    {
        RemovePilot();
    }
}

void AircraftClass::WriteDirty(unsigned char **sAdd)
{
    unsigned char *stream = *sAdd;
    memcpy(stream, &dirty_aircraft, sizeof(dirty_aircraft));
    stream += sizeof(dirty_aircraft);

    if (dirty_aircraft bitand DIRTY_ACSTATUS_BITS)
    {
        memcpy(stream, &status_bits, sizeof(status_bits));
        stream += sizeof(status_bits);
    }

    dirty_aircraft = 0;
    *sAdd = stream;
}

void AircraftClass::SetAcStatusBits(int bits)
{
    if ( not IsAcStatusBitsSet(bits))
    {
        status_bits or_eq bits;
        MakeAircraftDirty(DIRTY_ACSTATUS_BITS, SEND_NOW);
    }
}

void AircraftClass::ClearAcStatusBits(int bits)
{
    if (IsAcStatusBitsSet(bits))
    {
        status_bits and_eq compl bits;
        MakeAircraftDirty(DIRTY_ACSTATUS_BITS, SEND_NOW);
    }
}

