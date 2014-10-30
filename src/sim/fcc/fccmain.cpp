#include "Graphics/Include/Render2D.h"
#include "Graphics/Include/DrawBsp.h"
#include "stdhdr.h"
#include "entity.h"
#include "PilotInputs.h"
#include "simveh.h"
#include "sms.h"
#include "airframe.h"
#include "object.h"
#include "fsound.h"
#include "soundfx.h"
#include "simdrive.h"
#include "mfd.h"
#include "radar.h"
#include "classtbl.h"
#include "playerop.h"
#include "navsystem.h"
#include "commands.h"
#include "hud.h"
#include "fcc.h"
#include "fault.h"
#include "fack.h"
#include "aircrft.h"
#include "smsdraw.h"
#include "airunit.h"
#include "handoff.h"
#include "otwdrive.h" //MI
#include "radardoppler.h" //MI
#include "missile.h" //MI
#include "misslist.h" //MLR
#include "getsimobjectdata.h" // MLR
#include "bomb.h" // MLR
#include "bombfunc.h" // MLR
#include "profiler.h"
#include "harmpod.h" // RV - I-Hawk



extern bool g_bUseRC135;
extern bool g_bEnableColorMfd;
extern bool g_bRealisticAvionics;
extern bool g_bEnableFCCSubNavCycle; // ASSOCIATOR 04/12/03: Enables you to cycle the Nav steerpoint modes modes with the FCC submodes key
extern bool g_bWeaponStepToGun; // MLR 3/13/2004 - optionally turns off weapon stepping to guns

extern bool g_bGreyMFD;
extern bool g_bGreyScaleMFD;
extern bool bNVGmode;

const int FireControlComputer::DATALINK_CYCLE = 20;//JPO = 20 seconds
const float FireControlComputer::MAXJSTARRANGESQ = 200 * 200; //JPO = 200 nm
const float FireControlComputer::EMITTERRANGE = 60;//JPO = 40 km

const float FireControlComputer::CursorRate = 0.15f; //MI added

FireControlComputer::FireControlComputer(SimVehicleClass* vehicle, int numHardpoints)
{
    // sfr: smartpointer
    //fccWeaponPtr = NULL; // MLR 3/16/2004 - simulated weapon
    fccWeaponId      = 0;
    //rocketPointer  = NULL; // MLR 3/5/2004 - For impact prediction
    rocketWeaponId = 0;

    platform = vehicle;
    airGroundDelayTime = 0.0F;
    airGroundRange     = 10.0F * NM_TO_FT;
    missileMaxTof = -1.0f;
    missileActiveTime = -1.0f;
    lastmissileActiveTime = -1.0f;
    bombPickle = FALSE;
    postDrop = FALSE;
    preDesignate = TRUE;
    tossAnticipationCue = NoCue;
    laddAnticipationCue = NoLADDCue; //MI
    lastMasterMode = Nav;
    // ASSOCIATOR
    lastNavMasterMode = Nav;
    lastAgMasterMode = (FCCMasterMode) - 1; // AirGroundBomb; // MLR 2/8/2004 - EnterAGMasterMode() will see this, and determine the default weapon

    // MLR 3/13/2004 - back to using hp ids
    lastAirAirHp          = -1;
    lastAirGroundHp       = -1;
    lastDogfightHp        = -1;
    lastMissileOverrideHp = -1;

    lastAirAirGunSubMode      = EEGS;  // MLR 2/7/2004 -
    lastAirGroundGunSubMode   = STRAF; // MLR 2/7/2004 -
    lastAirGroundLaserSubMode = SLAVE; // MLR 4/11/2004 -

    inAAGunMode = 0; // MLR 3/14/2004 -
    inAGGunMode = 0; // MLR 3/14/2004 -

    lastSubMode = ETE;
    //lastAirGroundSubMode = CCRP;//me123
    lastDogfightGunSubMode = EEGS; //MI
    lastAirAirSubMode = Aim9; // ASSOCIATOR
    strcpy(subModeString, "");
    playerFCC = FALSE;
    targetList = NULL;
    releaseConsent = FALSE;
    designateCmd = FALSE;
    dropTrackCmd = FALSE;
    targetPtr = NULL;
    missileCageCmd = FALSE;
    missileTDBPCmd = FALSE;
    missileSpotScanCmd = FALSE;
    missileSlaveCmd = FALSE;
    cursorXCmd = 0;
    cursorYCmd = 0;
    waypointStepCmd = 127; // Force an intial update (GM radar, at least, needs this)
    HSDRangeStepCmd = 0;
    HSDRange = 15.0F;
    HsdRangeIndex = 0; // JPO
    groundPipperAz = groundPipperEl = 0.0F;
    masterMode = Nav;
    subMode = ETE;
    dgftSubMode = Aim9; // JPO dogfight specific
    mrmSubMode = Aim120; // ASSOCIATOR 04/12/03: for remembering MRM mode missiles
    autoTarget = FALSE;
    missileWEZDisplayRange = 20.0F * NM_TO_FT;

    mSavedWayNumber = 0;
    mpSavedWaypoint = NULL;
    mStptMode = FCCWaypoint;
    mNewStptMode = mStptMode;
    bombReleaseOverride = FALSE;
    lastMissileShootRng = -1;
    missileLaunched = 0;
    lastMissileShootHeight = 0;
    lastMissileShootEnergy = 0;
    nextMissileImpactTime = -1.0F;
    lastMissileImpactTime = -1.0f;
    Height = 0;//me123
    targetspeed = 0;//me123
    hsdstates = 0; // JPO
    MissileImpactTimeFlash = 0; // JPO
    grndlist = NULL;
    BuildPrePlanned(); // JPO
    //MI
    LaserArm = FALSE;
    LaserFire = FALSE;
    ManualFire = FALSE;
    LaserWasFired = FALSE;
    CheckForLaserFire = FALSE;
    InhibitFire = FALSE;
    Timer = 0.0F;
    ImpactTime = 0.0F;
    LaserRange = 0.0F;
    SafetyDistance = 1 * NM_TO_FT;
    pitch = 0.0F;
    roll = 0.0F;
    yaw = 0.0F;
    time = 0;

    //MI SOI and HSD
    IsSOI = FALSE;
    CouldBeSOI = FALSE;
    HSDZoom = 0;
    HSDXPos = 0; //Wombat778 11-10-2003
    HSDYPos = 0; //Wombat778 11-10-2003
    HSDCursorXCmd = 0;
    HSDCursorYCmd = 0;
    xPos = 0; //position of the curson on the scope
    yPos = 0;
    HSDDesignate = 0;
    curCursorRate = CursorRate;
    DispX = 0;
    DispY = 0;
    missileSeekerAz = missileSeekerEl = 0;
}

FireControlComputer::~FireControlComputer(void)
{
    ClearCurrentTarget();
    ClearPlanned(); // JPO
}

void FireControlComputer::SetPlayerFCC(int flag)
{
    WayPointClass* tmpWaypoint;

    playerFCC = flag;
    Sms->SetPlayerSMS(flag);
    tmpWaypoint = platform->waypoint;
    TheHud->waypointNum = 0;

    while (tmpWaypoint and tmpWaypoint not_eq platform->curWaypoint)
    {
        tmpWaypoint = tmpWaypoint->GetNextWP();
        TheHud->waypointNum ++;
    }
}

// JPO - just note it is launched.
void FireControlComputer::MissileLaunch()
{
    if (subMode == Aim120)
    {
        missileLaunched = 1;
        lastMissileShootTime = SimLibElapsedTime;
    }

    if ( not Sms->GetCurrentWeapon())
    {
        switch (masterMode) // MLR 4/12/2004 - Even though this function only appears to be called in AA modes
        {
            case Missile:
            case MissileOverride:
                if ( not Sms->FindWeaponType(wtAim120))
                    Sms->FindWeaponType(wtAim9);

                SetMasterMode(masterMode);
                break;

            case Dogfight:
                if ( not Sms->FindWeaponType(wtAim9))
                    Sms->FindWeaponType(wtAim120);

                SetMasterMode(masterMode);
                break;
        }
    }

    UpdateLastData();
    // UpdateWeaponPtr();
}

SimObjectType* FireControlComputer::Exec(SimObjectType* curTarget, SimObjectType* newList,
        PilotInputs* theInputs)
{
#ifdef Prof_ENABLED
    Prof(FireControlComputer_Exec);
#endif


    //me123 overtake needs to be calgulated the same way in MissileClass::GetTOF
    static const float MISSILE_ALTITUDE_BONUS = 23.0f; //me123 addet here and in // JB 010215 changed from 24 to 23
    static const float MISSILE_SPEED = 1500.0f; // JB 010215 changed from 1300 to 1500

    if (playerFCC and 
        ((AircraftClass*)platform)->mFaults->GetFault(FaultClass::fcc_fault))
    {
        SetTarget(NULL);
    }
    else
    {
        if (SimDriver.MotionOn())
        {
            if ( not targetPtr)
            {
                MissileImpactTimeFlash = 0; // cancel flashing
                lastMissileImpactTime = 0;
                lastmissileActiveTime = 0;
            }

            if (targetPtr and missileLaunched)
            {
                lastMissileShootRng = targetPtr->localData->range;
                lastMissileImpactTime = nextMissileImpactTime;
                lastMissileShootHeight = Height;
                lastMissileShootEnergy = (platform->GetVt() * FTPSEC_TO_KNOTS - 150.0f) / 2 ; // JB 010215 changed from 250 to 150
            }

            missileLaunched = 0;

            if (lastMissileImpactTime > 0.0F and targetPtr)
            {
                //me123 addet this stuff

                //this is the missiles approximate overtake
                //missilespeed + altitude bonus + target closure

                float overtake = lastMissileShootEnergy + MISSILE_SPEED + (lastMissileShootHeight / 1000.0f * MISSILE_ALTITUDE_BONUS) + targetspeed  * (float)cos(targetPtr->localData->ataFrom);

                //this is the predicted range from the missile to the target
                lastMissileShootRng = lastMissileShootRng - (overtake / SimLibMajorFrameRate);

                lastMissileImpactTime = max(0.0F, lastMissileShootRng / overtake); // this is TOF.  Counting on silent failure of divid by 0.0 here...
                lastMissileImpactTime += -5.0f * (float) sin(.07f * lastMissileImpactTime); // JB 010215

                if (lastMissileImpactTime == 0.0f)
                {
                    // JPO - trigger flashing X
                    // 8 seconds steady, 5 seconds flash
                    MissileImpactTimeFlash = SimLibElapsedTime + (5 + 8) * CampaignSeconds;
                    lastMissileShootRng = -1.0f; // reset for next
                }
                else
                    MissileImpactTimeFlash = 0;
            }
        }

        SetTarget(curTarget);
        targetList = newList;

        NavMode();

        switch (masterMode)
        {
            case AGGun:
                //if (GetSubMode() == STRAF) {
                AirGroundMode();
                break;

            case ILS:
            case Nav:
                break;

            case Dogfight:
            case MissileOverride:
            case Missile:
                AirAirMode();
                lastCage = missileCageCmd;
                break;

            case AirGroundBomb:
                AirGroundMode();
                break;

            case AirGroundRocket:
                AirGroundMode();
                break;

            case AirGroundMissile:
            case AirGroundHARM:
                AirGroundMissileMode();
                break;

            case AirGroundLaser:
                //if( not playerFCC)
                TargetingPodMode();
                break;
        }

        // always run targeting pod for player
        // FRB - always run targeting pod for All
        //TargetingPodMode();
        //if(playerFCC) TargetingPodMode();
        // COBRA - RED - CCIP HUD FIX - make the Time To target equal to 0, targeting Pod mode is assigning a time to target
        // messing up the CCIP pipper in HudClas::DrawCCIP call
        //if(subMode==CCIP) airGroundDelayTime=0.0F;

        lastDesignate = designateCmd;
    }

    return (targetPtr);
}


void FireControlComputer::SetSubMode(FCCSubMode newSubMode)
{
    if (newSubMode == CCRP and 
        Sms and 
        Sms->Ownship() and 
        Sms->Ownship()->IsAirplane() and // MLR not always owned by a/c
        ((AircraftClass *)(Sms->Ownship()))->af and 
        ( not ((AircraftClass *)Sms->Ownship())->af->IsSet(AirframeClass::IsDigital) or
         ( not (((AircraftClass *)(Sms->Ownship()))->AutopilotType() == AircraftClass::CombatAP))) and 
        platform and RadarDataTable[platform->GetRadarType()].NominalRange == 0.0) // JB 011018
    {
        newSubMode = CCIP;
    }

    // It has been stated (by Leon R) that changing modes while releasing weapons is bad, so...
    if (bombPickle)
    {
        return;
    }

    if (masterMode not_eq Dogfight and 
        masterMode not_eq MissileOverride)
    {
        lastSubMode = subMode;
    }

    if (lastSubMode == BSGT or
        lastSubMode == SLAVE or
        lastSubMode == HARM or
        lastSubMode == HTS)
    {
        platform->SOIManager(SimVehicleClass::SOI_RADAR);
    }

    subMode = newSubMode;
    // COBRA - RED - Default to immediate release Pickle Time
    PICKLE(DEFAULT_PICKLE);

    switch (subMode)
    {
        case SAM:
            strcpy(subModeString, "SAM");
            Sms->SetWeaponType(wtNone);
            Sms->FindWeaponClass(wcSamWpn);
            break;

        case Aim9:
            strcpy(subModeString, "SRM");

            if (Sms and Sms->Ownship() and ((AircraftClass *)Sms->Ownship())->AutopilotType() == AircraftClass::CombatAP)
            {
                if (Sms->GetCoolState() == SMSClass::WARM and Sms->MasterArm() == SMSClass::Arm)
                {
                    // JPO aim9 cooling
                    Sms->SetCoolState(SMSClass::COOLING);
                }
            }

            platform->SOIManager(SimVehicleClass::SOI_RADAR);
            break;

        case Aim120:
            strcpy(subModeString, "MRM");
            platform->SOIManager(SimVehicleClass::SOI_RADAR);
            // COBRA - RED - 1 Second Pickle for AIM 120
            PICKLE(SEC_1_PICKLE);
            break;

        case EEGS:
            strcpy(subModeString, "EEGS");
            platform->SOIManager(SimVehicleClass::SOI_RADAR);
            break;

            // ASSOCIATOR 03/12/03: Added the combined SnapShot LCOS Gunmode SSLC
        case SSLC:
            strcpy(subModeString, "SSLC");
            platform->SOIManager(SimVehicleClass::SOI_RADAR);
            break;

        case LCOS:
            strcpy(subModeString, "LCOS");
            platform->SOIManager(SimVehicleClass::SOI_RADAR);
            break;

        case Snapshot:
            strcpy(subModeString, "SNAP");
            platform->SOIManager(SimVehicleClass::SOI_RADAR);
            break;

        case CCIP:
            // MLR 4/1/2004 - rewrite based on Mirv's info.
            preDesignate = TRUE;
            strcpy(subModeString, "CCIP");
            platform->SOIManager(SimVehicleClass::SOI_RADAR);
            break;

        case CCRP:
            // MLR 4/1/2004 - rewrite based on Mirv's info.
            preDesignate = FALSE;
            strcpy(subModeString, "CCRP"); //me123 moved so we don't write this with no bombs
            platform->SOIManager(SimVehicleClass::SOI_RADAR);
            // 2001-04-18 ADDED BY S.G. I'LL SET MY RANDOM NUMBER FOR CCRP BOMBING INNACURACY NOW
            // Since autoTarget is a byte :-(  I'm limiting to just use the same value for both x and y offset :-(
            //  autoTarget = (rand() bitand 0x3f) - 32; // In RP5, I'm limited to the variables I can use
            xBombAccuracy = (rand() bitand 0x3f) - 32;
            yBombAccuracy = (rand() bitand 0x3f) - 32;
            // COBRA - RED - 1 Second Pickle for CCRP
            PICKLE(SEC_1_PICKLE);
            break;

        case DTOSS:
            preDesignate = TRUE;
            groundPipperAz = 0.0F;
            groundPipperEl = 0.0F;

            platform->SOIManager(SimVehicleClass::SOI_HUD);

            // MLR 4/1/2004 - rewrite based on Mirv's info.
            strcpy(subModeString, "DTOS");
            // COBRA - RED - 1 Second Pickle for DTOSS
            PICKLE(SEC_1_PICKLE);
            break;

        case LADD:
            preDesignate = FALSE;
            groundPipperAz = 0.0F;
            groundPipperEl = 0.0F;

            platform->SOIManager(SimVehicleClass::SOI_RADAR);

            // MLR 4/1/2004 - rewrite based on Mirv's info.
            strcpy(subModeString, "LADD");
            break;

        case MAN: // JPO
            preDesignate = TRUE;
            strcpy(subModeString, "MAN");
            platform->SOIManager(SimVehicleClass::SOI_RADAR);
            // COBRA - RED - 1 Second Pickle for MAN
            PICKLE(SEC_1_PICKLE);
            break;

        case OBSOLETERCKT:
            preDesignate = TRUE;
            strcpy(subModeString, "RCKT");
            platform->SOIManager(SimVehicleClass::SOI_RADAR);
            break;

        case STRAF:
            preDesignate = TRUE;
            strcpy(subModeString, "STRF");
            platform->SOIManager(SimVehicleClass::SOI_RADAR);
            break;

        case BSGT:
            preDesignate = TRUE;
            groundPipperAz = 0.0F;
            groundPipperEl = 0.0F;
            strcpy(subModeString, "BSGT");
            platform->SOIManager(SimVehicleClass::SOI_HUD);
            break;

        case SLAVE:
            preDesignate = TRUE;
            groundPipperAz = 0.0F;
            groundPipperEl = 0.0F;
            strcpy(subModeString, "SLAV");
            platform->SOIManager(SimVehicleClass::SOI_RADAR);
            // COBRA - RED - 1 Second Pickle for SLAVE
            PICKLE(SEC_1_PICKLE);
            break;

        case HARM:
        case HTS:
            preDesignate = TRUE;
            groundPipperAz = 0.0F;
            groundPipperEl = 0.0F;

            // RV - I-Hawk - on HUD it should read "HTS" for HTS and "HARM" for the HARM WPN mode usage
            if (subMode == HTS)
            {
                strcpy(subModeString, "HTS");
            }

            else
            {
                strcpy(subModeString, "HARM");
            }

            if (Sms->GetCurrentWeaponHardpoint() >= 0 and // JPO CTD fix
                Sms->CurHardpoint() >= 0 and // JB 010805 Possible CTD check curhardpoint
                Sms->hardPoint[Sms->CurHardpoint()] not_eq NULL) // Cobra - Sms->GetCurrentWeaponHardpoint() was causing a CTD (returned with a very large number)
            {
                Sms->SetWeaponType(Sms->hardPoint[Sms->CurHardpoint()]->GetWeaponType());

                // RV - I-Hawk - Don't auto pass SOI to HARM if we are using the advanced HARM systems
                HarmTargetingPod* harmPod = (HarmTargetingPod*)FindSensor(platform, SensorClass::HTS);

                if (harmPod and (harmPod->GetSubMode() == HarmTargetingPod::HAS or
                                harmPod->GetSubMode() == HarmTargetingPod::HAD))
                {
                    platform->SOIManager(SimVehicleClass::SOI_RADAR);
                }

                else
                {
                    platform->SOIManager(SimVehicleClass::SOI_WEAPON);
                }
            }

            else
                Sms->SetWeaponType(wtNone);

            // COBRA - RED - 1 Second Pickle for HTS
            PICKLE(SEC_1_PICKLE);
            break;

        case TargetingPod:
            preDesignate = TRUE;
            groundPipperAz = 0.0F;
            groundPipperEl = 0.0F;
            strcpy(subModeString, "GBU");

            if (Sms->GetCurrentWeaponHardpoint() >= 0 and // JPO CTD fix
                Sms->CurHardpoint() >= 0 and // JB 010805 Possible CTD check curhardpoint
                Sms->hardPoint[Sms->GetCurrentWeaponHardpoint()] not_eq NULL and 
                Sms->hardPoint[Sms->GetCurrentWeaponHardpoint()]->weaponPointer)
                Sms->SetWeaponType(Sms->hardPoint[Sms->CurHardpoint()]->GetWeaponType());
            else
                Sms->SetWeaponType(wtNone);

            platform->SOIManager(SimVehicleClass::SOI_RADAR);
            // COBRA - RED - 1 Second Pickle for TGP
            PICKLE(SEC_1_PICKLE);
            break;

        case TimeToGo:
        case ETE:
        case ETA:
            platform->SOIManager(SimVehicleClass::SOI_RADAR);
            break;
    }

    // Make sure string is correct in override modes
    switch (masterMode)
    {
        case Dogfight:
            //strcpy (subModeString, "DGFT"); //JPG 29 Apr 04 - This is no longer displayed in new software tapes
            break;

        case MissileOverride:
            if (Sms->curWeaponType == Aim9)
            {
                strcpy(subModeString, "SRM");
            }

            if (Sms->curWeaponType == Aim120)
            {
                strcpy(subModeString, "MRM");
            }

            break;
    }

    // MLR 4/1/2004 - Memorize SubMode
    switch (masterMode)
    {
        case ILS:
        case Nav:
            break;

        case Dogfight:
            lastDogfightGunSubMode = subMode;
            break;

        case MissileOverride:
            lastMissileOverrideSubMode = subMode;
            break;

        case AAGun:
            lastAirAirGunSubMode = subMode;
            break;

        case Missile:
            lastAirAirSubMode = subMode;
            break;

        case AGGun:
            lastAirGroundGunSubMode = subMode;
            break;

        case AirGroundBomb:
            Sms->SetAGBSubMode(subMode);
            {
                if (playerFCC and SimDriver.GetPlayerAircraft()->AutopilotType() not_eq AircraftClass::CombatAP)
                {
                    RadarDopplerClass* pradar = (RadarDopplerClass*) FindSensor(platform, SensorClass::Radar);

                    if (g_bRealisticAvionics and pradar)
                    {
                        if (subMode == CCRP or subMode == MAN)
                        {
                            pradar->SelectLastAGMode();
                        }
                        else
                        {
                            pradar->DefaultAGMode();
                        }

                        pradar->SetScanDir(1.0F);
                    }
                }
                else
                {
                    RadarClass* pradar = (RadarClass*) FindSensor(platform, SensorClass::Radar);
                    pradar->DefaultAGMode();
                }
            }
            break;

        case AirGroundMissile:
            //lastAirGroundMissileSubMode = subMode;
            break;

        case AirGroundHARM:
            //lastAirGroundHARMSubMode    = subMode;
            break;

        case AirGroundLaser:
            lastAirGroundLaserSubMode   = subMode;
            break;

        case AirGroundCamera:
            //lastAirGroundCameraSubMode  = subMode;
            break;
    }

}

void FireControlComputer::ClearOverrideMode(void)
{
    if ((GetMasterMode() == Dogfight) or
        (GetMasterMode() == MissileOverride))
    {
        masterMode      = lastMasterMode; // MLR - little kludge so I can get the MM
        MASTERMODES mmm = GetMainMasterMode();

        masterMode = ClearOveride;//me123 to allow leaving an overide mode

        switch (mmm)
        {
            case MM_AA:
                EnterAAMasterMode();
                break;

            case MM_AG:
                EnterAGMasterMode();
                break;

            default:
                SetMasterMode(lastMasterMode);
                break;
        }

    }
}


void FireControlComputer::NextSubMode(void)
{
    // MLR 4/3/2004 - Added calls to SetSubMode instead of doing 'stuff' for ourselves.
    //MI
    RadarDopplerClass* pradar = (RadarDopplerClass*) FindSensor(platform, SensorClass::Radar);

    switch (masterMode)
    {
            // ASSOCIATOR 02/12/03: Now we can use the Cycle FCC Submodes key when in Dogfight Mode
            // ASSOCIATOR 03/12/03: Added the combined SnapShot LCOS Gunmode SSLC
        case Dogfight:
            switch (subMode)
            {
                case EEGS:
                    if (PlayerOptions.GetAvionicsType() not_eq ATEasy)
                    {
                        SetSubMode(SSLC);
                    }

                    break;

                case SSLC:
                    SetSubMode(LCOS);
                    break;

                case LCOS:
                    SetSubMode(Snapshot);
                    break;

                case Snapshot:
                    SetSubMode(EEGS);
                    break;
            }

            break;

            // ASSOCIATOR 02/12/03: Now we can use the Cycle FCC Submodes key when in MissileOverride
            // ASSOCIATOR 03/12/03: Added the combined SnapShot LCOS Gunmode SSLC
        case AAGun:
            switch (subMode)
            {
                case EEGS:
                    if (PlayerOptions.GetAvionicsType() not_eq ATEasy)
                    {
                        SetSubMode(SSLC);
                    }

                    break;

                case SSLC:
                    SetSubMode(LCOS);
                    break;

                case LCOS:
                    SetSubMode(Snapshot);
                    break;

                case Snapshot:
                    SetSubMode(EEGS);
                    break;
            }

            break;


        case Nav:

            // MD -- 20031203: removed this since sources seem to indicate that there is no such function in the real jet.
            //  ASSOCIATOR Added g_bEnableFCCSubNavCycle as an option and not g_bRealisticAvionics to not break the other modes
            if (g_bEnableFCCSubNavCycle bitor not g_bRealisticAvionics)
            {
                switch (subMode)
                {
                    case ETE:
                        if (PlayerOptions.GetAvionicsType() not_eq ATEasy)
                        {
                            SetSubMode(TimeToGo);
                        }

                        break;

                    case TimeToGo:
                        SetSubMode(ETA);
                        break;

                    case ETA:
                        SetSubMode(ETE);
                        break;
                }

                break;
            }
            else
                break;

        case AirGroundBomb:
            switch (subMode)
            {
                case CCIP:
                    if (PlayerOptions.GetAvionicsType() not_eq ATEasy)
                    {
                        SetSubMode(DTOSS);
                    }

                    break;

                case CCRP:
                    SetSubMode(CCIP);
                    break;

                case DTOSS:
                    SetSubMode(CCRP);
                    break;

                default: // catch LADD MAN etc
                    SetSubMode(CCRP);
                    break;
            }

            break;

        case AirGroundLaser:
            if (g_bRealisticAvionics)
            {
                SetSubMode(lastAirGroundLaserSubMode);
                SetSubMode(SLAVE);
                break;
            }

            // intentionally fall thru
        case AirGroundMissile:
            switch (subMode)
            {
                case SLAVE:
                    if (PlayerOptions.GetAvionicsType() not_eq ATEasy)
                    {
                        SetSubMode(BSGT);
                    }

                    break;

                case BSGT:
                    SetSubMode(SLAVE);
                    break;
            }

            break;


    }
}


void FireControlComputer::WeaponStep(void)
{
    RadarDopplerClass* pradar = (RadarDopplerClass*) FindSensor(platform, SensorClass::Radar);
    BombClass *TheBomb = GetTheBomb();

    switch (masterMode)
    {
        case Dogfight:
        case MissileOverride:
        case Missile:
        case AirGroundMissile:
        case AirGroundHARM:
            Sms->WeaponStep();

            if (pradar and (masterMode == AirGroundMissile or masterMode == AirGroundHARM)) //MI fix
            {
                pradar->SetScanDir(1.0F);
                pradar->SelectLastAGMode();
            }

            break;

        case AGGun:
            if (lastAgMasterMode == AirGroundBomb)
            {
                ToggleAGGunMode();
                SetSubMode(CCRP);
            }

            break;

        case AirGroundBomb:
            // COBRA - RED - FIXING POSSIBLE CTDs
            // CCIP -> DTOS -> STRAF -> CCRP
            /*if ((Sms->GetCurrentHardpoint() > 0) and (Sms->hardPoint[Sms->GetCurrentHardpoint()]->GetWeaponType()==wtGPS or  // Cobra - no rippling GPS
             (((BombClass*)Sms->hardPoint[Sms->GetCurrentHardpoint()]->weaponPointer) and 
             ((BombClass*)Sms->hardPoint[Sms->GetCurrentHardpoint()]->weaponPointer)->IsSetBombFlag(BombClass::IsJSOW))))*/
        {
            if (TheBomb and (TheBomb->IsSetBombFlag(BombClass::IsGPS) or TheBomb->IsSetBombFlag(BombClass::IsJSOW)))
                Sms->WeaponStep();

            break;
        }

        switch (subMode)
        {
            case CCIP:
                SetSubMode(DTOSS);
                break;

            case DTOSS:

                //Cobra TJL 11/17/04 Aircraft w/o guns get stuck in DTOSS
                //with missile step
                //ToggleAGGunMode();
                if (Sms->FindWeaponClass(wcGunWpn, TRUE))
                    ToggleAGGunMode();
                else
                    SetSubMode(CCRP);

                break;

            case CCRP:
                SetSubMode(CCIP);
                break;

            default: // catch LADD MAN etc
                SetSubMode(CCRP);
                break;
        }

        break;
    }
}


SimObjectType* FireControlComputer::TargetStep(SimObjectType* startObject, int checkFeature)
{
    VuEntity* testObject = NULL;
    VuEntity* groundTarget = NULL;
    SimObjectType* curObject = NULL;
    SimObjectType* retObject = NULL;
    float angOff;

    // Starting in the object list
    if (startObject == NULL or startObject not_eq targetPtr)
    {
        // Start at next and go on
        if (startObject)
        {
            curObject = startObject->next;

            while (curObject)
            {
                if (curObject->localData->ata < 60.0F * DTR)
                {
                    retObject = curObject;
                    break;
                }

                curObject = curObject->next;
            }
        }

        // Did we go off the End of the objects?
        if ( not retObject and checkFeature)
        {
            // Check features
            {
                VuListIterator featureWalker(SimDriver.featureList);
                testObject = featureWalker.GetFirst();

                while (testObject)
                {
                    angOff = (float)atan2(testObject->YPos() - platform->YPos(),
                                          testObject->XPos() - platform->XPos()) - platform->Yaw();

                    if (fabs(angOff) < 60.0F * DTR)
                    {
                        break;
                    }

                    testObject = featureWalker.GetNext();
                }
            }

            if (testObject)
            {
                groundTarget = testObject;
                // KCK NOTE: Uh.. why are we doing this?
                //if (retObject)
                //{
                //Tpoint pos;
                //targetPtr->BaseData()->drawPointer->GetPosition (&pos);
                //targetPtr->BaseData()->SetPosition (pos.x, pos.y, pos.z);
                //}
                groundDesignateX = testObject->XPos();
                groundDesignateY = testObject->YPos();
                groundDesignateZ = testObject->ZPos();
            }
        }

        // Did we go off the end of the Features?
        if ( not retObject and not groundTarget)
        {
            // Check the head of the object list
            curObject = targetList;

            if (curObject)
            {
                if (curObject->localData->ata < 60.0F * DTR)
                {
                    retObject = curObject;
                }
                else
                {
                    while (curObject not_eq startObject)
                    {
                        curObject = curObject->next;

                        if (curObject and curObject->localData->ata < 60.0F * DTR)
                        {
                            retObject = curObject;
                            break;
                        }
                    }
                }
            }
        }
    }
    else if (startObject == targetPtr)
    {
        // Find the current object
        if (checkFeature)
        {
            {
                VuListIterator featureWalker(SimDriver.featureList);
                testObject = featureWalker.GetFirst();

                // Iterate up to our position in the list.
                while (testObject and testObject not_eq targetPtr->BaseData())
                    testObject = featureWalker.GetNext();

                // And then get the next object
                if (testObject)
                    testObject = featureWalker.GetNext();

                // Is there anything after the current object?
                while (testObject)
                {
                    angOff = (float)atan2(testObject->YPos() - platform->YPos(),
                                          testObject->XPos() - platform->XPos()) - platform->Yaw();

                    if (fabs(angOff) < 60.0F * DTR)
                    {
                        break;
                    }

                    testObject = featureWalker.GetNext();
                }
            }

            // Found one, so use it
            if (testObject)
            {
                groundTarget = testObject;
                // KCK: Why are we doing this?
                // Tpoint pos;
                //          targetPtr->BaseData()->drawPointer->GetPosition (&pos);
                //          targetPtr->BaseData()->SetPosition (pos.x, pos.y, pos.z);
                groundDesignateX = testObject->XPos();
                groundDesignateY = testObject->YPos();
                groundDesignateZ = testObject->ZPos();
            }
        }

        // Off the end of the feature list?
        if ( not retObject and not groundTarget)
        {
            // Check the head of the object list
            curObject = targetList;

            while (curObject)
            {
                if (curObject->localData->ata < 60.0F * DTR)
                {
                    retObject = curObject;
                    break;
                }

                curObject = curObject->next;
            }
        }

        // Of the End of the object list ?
        if ( not retObject and checkFeature and not groundTarget)
        {
            // Check features
            VuListIterator featureWalker(SimDriver.featureList);
            testObject = featureWalker.GetFirst();

            while (testObject and testObject not_eq targetPtr->BaseData())
            {
                angOff = (float)atan2(testObject->YPos() - platform->YPos(),
                                      testObject->XPos() - platform->XPos()) - platform->Yaw();

                if (fabs(angOff) < 60.0F * DTR)
                {
                    break;
                }

                testObject = featureWalker.GetNext();
            }

            if (testObject)
            {
                groundTarget = testObject;
                groundDesignateX = testObject->XPos();
                groundDesignateY = testObject->YPos();
                groundDesignateZ = testObject->ZPos();
            }
        }
    }

    if (groundTarget)
    {
        // We're targeting a feature thing - make a new SimObjectType
#ifdef DEBUG
        //retObject = new SimObjectType(OBJ_TAG, platform, (SimBaseClass*)groundTarget);
#else
        retObject = new SimObjectType((SimBaseClass*)groundTarget);
#endif
        retObject->localData->ataFrom = 180.0F * DTR;
    }

    SetTarget(retObject);

    return retObject;
}

void FireControlComputer::ClearCurrentTarget(void)
{
    if (targetPtr)
        targetPtr->Release();

    targetPtr = NULL;
}

void FireControlComputer::SetTarget(SimObjectType* newTarget)
{
    if (newTarget == targetPtr)
        return;

    /* MLR debugging stuff
    MonoPrint("  FCC::SetTarget - prev:%08x/%08x, new: %08x/%08x\n",
     targetPtr,(targetPtr?targetPtr->BaseData():0),
     newTarget,(newTarget?newTarget->BaseData():0));

    if( not newTarget)
     int stop=0;
    */
    ClearCurrentTarget();

    if (newTarget)
    {
        ShiAssert(newTarget->BaseData() not_eq (FalconEntity*)0xDDDDDDDD);
        newTarget->Reference();
    }

    targetPtr = newTarget;
}

void FireControlComputer::DisplayInit(ImageBuffer* image)
{
    DisplayExit();

    privateDisplay = new Render2D;
    ((Render2D*)privateDisplay)->Setup(image);

    if ((g_bGreyMFD) and ( not bNVGmode))
        privateDisplay->SetColor(GetMfdColor(MFD_WHITE));
    else
        privateDisplay->SetColor(0xff00ff00);
}

void FireControlComputer::Display(VirtualDisplay* newDisplay)
{
    display = newDisplay;

    // JPO intercept for now FCC power...
    if ( not ((AircraftClass*)platform)->HasPower(AircraftClass::FCCPower))
    {
        BottomRow();
        display->TextCenter(0.0f, 0.2f, "FCC");
        int ofont = display->CurFont();
        display->SetFont(2);
        display->TextCenterVertical(0.0f, 0.0f, "OFF");
        display->SetFont(3);
        return;
    }

    NavDisplay();
}

void FireControlComputer::PushButton(int whichButton, int whichMFD)
{
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
    ShiAssert(whichButton < 20);
    ShiAssert(whichMFD < 4);

    if (IsHsdState(HSDCNTL))
    {
        if (hsdcntlcfg[whichButton].mode not_eq HSDNONE)
        {
            ToggleHsdState(hsdcntlcfg[whichButton].mode);
            return;
        }
    }

    //MI
    if (g_bRealisticAvionics)
    {
        if (playerAC)
        {
            if (IsSOI and (whichButton >= 11 and whichButton <= 13))
                platform->StepSOI(2);
        }
    }

    switch (whichButton)
    {
        case 0: // DEP - JPO
            if (g_bRealisticAvionics)
            {
                ToggleHsdState(HSDCEN);
            }

            break;

        case 1: // DCPL - JPO
            if (g_bRealisticAvionics)
            {
                ToggleHsdState(HSDCPL);
            }

            break;

            //MI
        case 2:
            if (g_bRealisticAvionics)
                ToggleHSDZoom();

            break;

        case 4: // CTRL
            if (g_bRealisticAvionics)
            {
                ToggleHsdState(HSDCNTL);
            }

            break;

        case 6: // FRZ - JPO
            if (g_bRealisticAvionics)
            {
                frz_x = platform->XPos();
                frz_y = platform->YPos();
                frz_dir = platform->Yaw();
                ToggleHsdState(HSDFRZ);
            }

            break;

        case 10:
            if (g_bRealisticAvionics)
            {
                MfdDrawable::PushButton(whichButton, whichMFD);
            }

            break;

        case 11: // SMS
            if (g_bRealisticAvionics)
                MfdDrawable::PushButton(whichButton, whichMFD);
            else
                MfdDisplay[whichMFD]->SetNewMode(MFDClass::SMSMode);

            break;

        case 12: // jpo
            if (g_bRealisticAvionics)
                MfdDrawable::PushButton(whichButton, whichMFD);

            break;

        case 13: // HSD
            if (g_bRealisticAvionics)
                MfdDrawable::PushButton(whichButton, whichMFD);
            else
                MfdDisplay[whichMFD]->SetNewMode(MFDClass::MfdMenu);

            break;

        case 14: // SWAP
            if (g_bRealisticAvionics)
                MfdDrawable::PushButton(whichButton, whichMFD);
            else
                MFDSwapDisplays();

            break;

        case 18: // Down
            if ( not g_bRealisticAvionics)
                SimHSDRangeStepDown(0, KEY_DOWN, NULL);
            else
            {
                //MI
                if (HSDZoom == 0)
                    SimHSDRangeStepDown(0, KEY_DOWN, NULL);
            }

            break;

        case 19: // UP
            if ( not g_bRealisticAvionics)
                SimHSDRangeStepUp(0, KEY_DOWN, NULL);
            else
            {
                //MI
                if (HSDZoom == 0)
                    SimHSDRangeStepUp(0, KEY_DOWN, NULL);
            }

            break;
    }
}

// STUFF Copied from Harm - now merged. JPO
// JPO
// This routine builds the initial list of preplanned targets.
// this will remain unchanged if there is no dlink/jstar access
void FireControlComputer::BuildPrePlanned()
{
    FlightClass* theFlight = (FlightClass*)(platform->GetCampaignObject());
    FalconPrivateList* knownEmmitters = NULL;
    GroundListElement* tmpElement;
    GroundListElement* curElement = NULL;
    FalconEntity* eHeader;

    // this is all based around waypoints.
    if (SimDriver.RunningCampaignOrTactical() and theFlight)
        knownEmmitters = theFlight->GetKnownEmitters();

    if (knownEmmitters)
    {
        {
            VuListIterator elementWalker(knownEmmitters);
            eHeader = (FalconEntity*)elementWalker.GetFirst();

            while (eHeader)
            {
                tmpElement = new GroundListElement(eHeader);

                if (grndlist == NULL)
                    grndlist = tmpElement;
                else
                    curElement->next = tmpElement;

                curElement = tmpElement;
                eHeader = (FalconEntity*)elementWalker.GetNext();
            }
        }
        knownEmmitters->Unregister();
        delete knownEmmitters;
    }

    nextDlUpdate = SimLibElapsedTime + CampaignSeconds * DATALINK_CYCLE;
}

// update - every so often from a JSTAR platform if available.
void FireControlComputer::UpdatePlanned()
{
    if (nextDlUpdate > SimLibElapsedTime)
        return;

    //Cobra This function was killing SAMs and threat rings on HSD
    //without it, everything is appearing as normal on the HSD.
    nextDlUpdate = SimLibElapsedTime + 5000/*CampaignSeconds * DATALINK_CYCLE*/;
    /*if (((AircraftClass*)platform)->mFaults->GetFault(FaultClass::dlnk_fault) or
 not ((AircraftClass*)platform)->HasPower(AircraftClass::DLPower))
    return;*/

    // RV version
    // nextDlUpdate = SimLibElapsedTime + CampaignSeconds * DATALINK_CYCLE;
    //if (((AircraftClass*)platform)->mFaults->GetFault(FaultClass::dlnk_fault) or not ((AircraftClass*)platform)->HasPower(AircraftClass::DLPower))
    // return;

    FlightClass* theFlight = (FlightClass*)(platform->GetCampaignObject());

    if ( not theFlight)
        return;

    CampEntity e;
    VuListIterator myit(EmitterList);

    // see if we have a jstar.
    Flight jstar = theFlight->GetJSTARFlight();

    Team us = theFlight->GetTeam();

    // If we didn't find JSTAR do other search
    if ( not jstar)
    {
        Unit nu, cf;
        VuListIterator new_myit(AllAirList);
        nu = (Unit) new_myit.GetFirst();

        while (nu and not jstar)
        {
            cf = nu;
            nu = (Unit) new_myit.GetNext();

            if ( not cf->IsFlight() or cf->IsDead())
                continue;

            if (cf->GetUnitMission() == AMIS_JSTAR and cf->GetTeam() == us and cf->GetUnitTOT() + 5 * CampaignMinutes < Camp_GetCurrentTime() and cf->GetUnitTOT() + 95 * CampaignMinutes > Camp_GetCurrentTime())
            {
                // Check if JSTAR is in range for communication
                if (Distance(platform->XPos(), platform->YPos(), cf->XPos(), cf->YPos()) * FT_TO_NM <= 250.0f)
                {
                    jstar = (Flight)cf;
                }
            }
        }
    }

    if (( not jstar) or ( not g_bUseRC135))
    {

        // completely new list please
        //cobra added here
        float myx = platform->XPos();
        float myy = platform->YPos();
        ClearPlanned();

        GroundListElement* tmpElement;
        GroundListElement* curElement = NULL;

        for (e = (CampEntity) myit.GetFirst(); e; e = (Unit) myit.GetNext())
        {
            if (e->GetTeam() not_eq us /* and e->GetSpotted(us) and 
 ( not e->IsUnit() or not ((Unit)e)->Moving()) and e->GetElectronicDetectionRange(Air)*/)
            {
                float ex = e -> XPos();
                float ey = e -> YPos();

                if (Distance(ex, ey, myx, myy) * FT_TO_NM < 150/*EMITTERRANGE*/)
                {
                    tmpElement = new GroundListElement(e);
                    tmpElement->SetFlag(GroundListElement::DataLink);//Cobra nothing is using this...
                    tmpElement->SetFlag(GroundListElement::RangeRing);//Cobra set the ring???

                    if (grndlist == NULL)
                    {
                        grndlist = tmpElement;
                    }
                    else
                    {
                        curElement->next = tmpElement;
                    }

                    curElement = tmpElement;
                }
            }
        }

        return;
    }

    //cobra added here

    int jstarDetectionChance = 0;

    if (jstar)
    {
        // This is a ELINT flight (e.g. RC-135)
        if (jstar->class_data->Role == ROLE_ELINT)
            jstarDetectionChance = 75;
        else

            // FRB - Give JSTAR SAM finder capabilities
            if ( not g_bUseRC135)
                jstarDetectionChance = 75;
            else
                jstarDetectionChance = 25;
    }

    //CampEntity e;
    //   VuListIterator myit(EmitterList);

    for (e = (CampEntity) myit.GetFirst(); e; e = (CampEntity) myit.GetNext())
    {
        if (e->IsUnit() and e->IsBattalion() and rand() % 100 > jstarDetectionChance)
            continue;

        if (e->IsGroundVehicle())
        {
            GroundListElement* tmpElement = GetFirstGroundElement();

            while (tmpElement)
            {
                if (tmpElement->BaseObject() == e)
                    break;

                tmpElement = tmpElement->GetNext();
            }

            if ( not tmpElement)
            {
                tmpElement = new GroundListElement(e);
                AddGroundElement(tmpElement);
            }
        }
    }
}

// every so often - remove dead targets
void FireControlComputer::PruneList()
{
    GroundListElement  **gpp;

    for (gpp = &grndlist; *gpp;)
    {
        if ((*gpp)->BaseObject() == NULL)   // delete this one
        {
            GroundListElement *gp = *gpp;
            *gpp = gp -> next;
            delete gp;
        }
        else gpp = &(*gpp)->next;
    }
}

GroundListElement::GroundListElement(FalconEntity* newEntity)
{
    F4Assert(newEntity);

    baseObject = newEntity;
    VuReferenceEntity(newEntity);
    symbol = RadarDataTable[newEntity->GetRadarType()].RWRsymbol;

    if (newEntity->IsCampaign())
        range = (float)((CampBaseClass*)newEntity)->GetAproxWeaponRange(Air);
    else range = 0;

    flags = RangeRing;
    next = NULL;
    lastHit = SimLibElapsedTime;
}

GroundListElement::~GroundListElement()
{
    VuDeReferenceEntity(baseObject);
}

void GroundListElement::HandoffBaseObject()
{
    FalconEntity *newBase;

    if (baseObject == NULL) return;

    newBase = SimCampHandoff(baseObject, HANDOFF_RADAR);

    if (newBase not_eq baseObject)
    {
        VuDeReferenceEntity(baseObject);

        baseObject = newBase;

        if (baseObject)
        {
            VuReferenceEntity(baseObject);
        }
    }
}

void FireControlComputer::ClearPlanned()
{
    GroundListElement* tmpElement;

    while (grndlist)
    {
        tmpElement = grndlist;
        grndlist = tmpElement->next;
        delete tmpElement;
    }
}

MASTERMODES FireControlComputer::GetMainMasterMode()
{
    switch (masterMode)
    {
        case AAGun:
        case Missile:
            return MM_AA;

        case ILS:
        case Nav:
        default:
            return MM_NAV;

        case AirGroundBomb:
        case AirGroundRocket:
        case AirGroundMissile:
        case AirGroundHARM:
        case AirGroundLaser:
        case AirGroundCamera:
        case AGGun:
            return MM_AG;

            //case Gun:
            //if (subMode == STRAF)
            //    return MM_AG;
            //else return MM_AA;
        case Dogfight:
            return MM_DGFT;

        case MissileOverride:
            return MM_MSL;
    }
}

int FireControlComputer::LastMissileWillMiss(float range)
{
    /* if (lastMissileImpactTime >0 and range > missileRMax )
       {
            return 1;
       }
     else return 0;
      */  //me123 if the predicted total TOF is over xx seconds the missile is considered out of energy

    if (lastMissileImpactTime > 0 and //me123 let's make sure there is a missile in the air
        lastMissileImpactTime - ((lastMissileShootTime - SimLibElapsedTime) / 1000)  >= 80)
        return 1;

    return 0;
}

float FireControlComputer::Aim120ASECRadius(float range)
{
    float asecradius = 0.6f;
    static const float bestmaxrange = 0.8f; // upper bound

    if ( not g_bRealisticAvionics) return asecradius;

    if (range > bestmaxrange * missileRMax)
    {
        // above best range
        float dr = (range - bestmaxrange * missileRMax) / (0.2f * missileRMax);
        dr = 1.0f - dr;
        asecradius *= dr;
    }
    else if (range < (missileRneMax - missileRneMin) / 2.0f)
    {
        float dr = (range - missileRMin);
        dr /= (missileRneMax - missileRneMin) / 2.0f - missileRMin;
        asecradius *= dr;
    }

    //MI make the size dependant on missile mode
    if (Sms and Sms->curWeapon and ((MissileClass*)Sms->GetCurrentWeapon())->isSlave)
        asecradius = max(min(0.3f, asecradius), 0.1f);
    else
        asecradius = max(min(0.6f, asecradius), 0.1f);

    return asecradius;
}

// MLR - SetMasterMode() no longer finds matching weapons if the currently
//       selected weapon doesn't match the mastermode.
//       However - SMM() will change HPs when Master Modes change.
void FireControlComputer::SetMasterMode(FCCMasterMode newMode)
{
    RadarClass* theRadar = (RadarClass*) FindSensor(platform, SensorClass::Radar);
    FCCMasterMode oldMode;
    HarmTargetingPod* harmPod = (HarmTargetingPod*) FindSensor(platform, SensorClass::HTS);

    /* appears to not be needed anymore
    if( playerFCC                                                  and 
     (masterMode == Dogfight or masterMode == MissileOverride)  and 
     newMode not_eq MissileOverride                                 and 
     newMode not_eq Dogfight                                        and 
     (Sms->curWeaponType == wtAim9 or Sms->curWeaponType == wtAim120 ))  // MLR 1/19/2004 - put these two in parenthesis

    {
     if ( masterMode == MissileOverride )
     weaponMisOvrdMode = Sms->curWeaponType;
     else
     if ( masterMode == Dogfight )
     weaponDogOvrdMode = Sms->curWeaponType;
     else
     weaponNoOvrdMode = Sms->curWeaponType;
    }
    */


    // Nav only if Amux and Bmux failed, no change if FCC fail
    if (playerFCC and 
        (
            (
                ((AircraftClass*)platform)->mFaults->GetFault(FaultClass::fcc_fault)
            )
            or
            (
                ((AircraftClass*)platform)->mFaults->GetFault(FaultClass::amux_fault) and 
                ((AircraftClass*)platform)->mFaults->GetFault(FaultClass::bmux_fault) and 
                newMode not_eq Nav
            )
        )
       )
        return;

    // It has been stated (by Leon R) that changing modes while releasing weapons is bad, so...
    if (bombPickle)
    {
        return;
    }

    switch (masterMode)
    {
        case ClearOveride:
            if (theRadar)
                theRadar->ClearOverride();

            break;

            // Clear any holdouts from previous modes
        case AirGroundHARM:
            ((AircraftClass*)platform)->SetTarget(NULL);
            break;
    }

    oldMode = masterMode;

    if (masterMode not_eq Dogfight and masterMode not_eq MissileOverride) masterMode = newMode;//me123

    int isAI = not playerFCC or
               (playerFCC and ((AircraftClass *)Sms->Ownship())->AutopilotType() == AircraftClass::CombatAP) ;


    switch (masterMode)
    {
        case Dogfight:

            // Clear out any non-air-to-air targets we had locked.
            if (oldMode not_eq Dogfight and oldMode not_eq Missile and oldMode not_eq MissileOverride)
                ClearCurrentTarget();

            //if (oldMode not_eq Dogfight)// MLR 4/11/2004 -  I'ld like to remove these, but the AI still calls SMM() directly
            //Sms->SetCurrentHpByWeaponId(lastDogfightWId);
            // Sms->SetCurrentHardPoint(lastDogfightHp);

            postDrop = FALSE;

            //MI changed so it remembers last gun submode too
            if ( not g_bRealisticAvionics)
                SetSubMode(EEGS);
            else
                SetSubMode(lastDogfightGunSubMode);

            if (isAI and not WeaponClassMatchesMaster(Sms->curWeaponClass))
                if ( not Sms->FindWeaponType(wtAim9))
                    Sms->FindWeaponType(wtAim120);

            switch (Sms->curWeaponType)
            {
                case wtAim9:
                    SetDgftSubMode(Aim9);
                    break;

                case wtAim120:
                    SetDgftSubMode(Aim120);
                    break;
            }

            if (theRadar and oldMode not_eq Dogfight)
            {
                theRadar->SetSRMOverride();
            }

            if (TheHud and playerFCC)
            {
                TheHud->headingPos = HudClass::Low;
            }

            break;

        case MissileOverride://me123 multi changes here

            //strcpy (subModeString, "MSL");  // JPG 20 Jan 04
            // Clear out any non-air-to-air targets we had locked.
            if (oldMode not_eq Dogfight and oldMode not_eq Missile and oldMode not_eq MissileOverride)
                ClearCurrentTarget();

            postDrop = FALSE;

            //if (oldMode not_eq MissileOverride)// MLR 4/11/2004 -  I'ld like to remove these, but the AI still calls SMM() directly
            // Sms->SetCurrentHardPoint(lastMissileOverrideHp);
            // Sms->SetCurrentHpByWeaponId(lastMissileOverrideWId);


            if (isAI and not WeaponClassMatchesMaster(Sms->curWeaponClass))
                if ( not Sms->FindWeaponType(wtAim120))
                    Sms->FindWeaponType(wtAim9);

            switch (Sms->curWeaponType)
            {
                case wtAim9:
                    SetSubMode(Aim9);
                    SetMrmSubMode(Aim9);
                    break;

                case wtAim120:
                    SetSubMode(Aim120);
                    SetMrmSubMode(Aim120);
                    break;
            }

            if (theRadar)
            {
                theRadar->SetMRMOverride();
            }

            if (TheHud and playerFCC)
                TheHud->headingPos = HudClass::Low;

            break;

        case Missile:

            // Clear out any non-air-to-air targets we had locked.
            if (oldMode not_eq Dogfight and oldMode not_eq MissileOverride)
                ClearCurrentTarget();

            postDrop = FALSE;

            //if(oldMode not_eq masterMode) // MLR 4/11/2004 -  I'ld like to remove these, but the AI still calls SMM() directly
            // Sms->SetCurrentHardPoint(lastAirAirHp);

            //MonoPrint("FCC:SetMasterMode - 1. CurrentWeaponType=%d\n",Sms->GetCurrentWeaponType());

            // make sure the AI get a proper weapon
            if (isAI and not WeaponClassMatchesMaster(Sms->curWeaponClass))
                if ( not Sms->FindWeaponType(wtAim120))
                    Sms->FindWeaponType(wtAim9);


            switch (Sms->GetCurrentWeaponType())
            {
                case wtAim120:
                    SetSubMode(Aim120);
                    break;

                case wtAim9:
                    SetSubMode(Aim9);
                    break;
            }

            if (TheHud and playerFCC)
                TheHud->headingPos = HudClass::Low;

            break;

        case ILS:
            // Clear out any previous targets we had locked.
            ClearCurrentTarget();
            Sms->SetWeaponType(wtNone);
            Sms->FindWeaponClass(wcNoWpn);

            if (TheHud and playerFCC)
                TheHud->headingPos = HudClass::High;

            platform->SOIManager(SimVehicleClass::SOI_RADAR);
            break;

        case Nav:
            // Clear out any previous targets we had locked.
            strcpy(subModeString, "NAV");
            ClearCurrentTarget();
            Sms->SetWeaponType(wtNone);
            Sms->FindWeaponClass(wcNoWpn);
            SetSubMode(ETE);
            releaseConsent = FALSE;
            postDrop = FALSE;
            preDesignate = TRUE;
            postDrop = FALSE;
            bombPickle = FALSE;

            // Find currentwaypoint
            if (TheHud and playerFCC)
            {
                TheHud->headingPos = HudClass::Low;
            }

            // SOI is RADAR in NAV
            //Cobra test Double here since ETE above sets SOI_RADAR
            //platform->SOIManager (SimVehicleClass::SOI_RADAR);
            break;

        case AirGroundBomb:
            // Clear out any previous targets we had locked.
            ClearCurrentTarget();
            preDesignate = TRUE;
            postDrop = FALSE;
            inRange = TRUE;

            if (isAI and not WeaponClassMatchesMaster(Sms->curWeaponClass))
                Sms->FindWeaponClass(wcBombWpn);

            if (g_bRealisticAvionics)
            {
                SetSubMode(Sms->GetAGBSubMode());
            }
            else
            {
                SetSubMode(CCIP);
            }

            //if(playerFCC)
            //{
            // Sms->drawable->SetDisplayMode(SmsDrawable::Wpn);
            ///}

            if (TheHud and playerFCC)
                TheHud->headingPos = HudClass::High;

            break;

        case AirGroundRocket:
            // Clear out any previous targets we had locked.
            ClearCurrentTarget();
            preDesignate = TRUE;
            postDrop = FALSE;
            inRange = TRUE;

            if (isAI and not WeaponClassMatchesMaster(Sms->curWeaponClass))
                Sms->FindWeaponClass(wcRocketWpn);

            SetSubMode(OBSOLETERCKT);
            /*
            if( not playerFCC)
            {
             SetSubMode (OBSOLETERCKT);
            }
            else
            {
             if(g_bRealisticAvionics)
             {
             SetSubMode (Sms->GetAGBSubMode());
             }
             else
             {
             SetSubMode (OBSOLETERCKT);
             }
            }
            */

            if (TheHud and playerFCC)
                TheHud->headingPos = HudClass::High;

            break;

        case AirGroundMissile:
            // Clear out any previous targets we had locked.
            ClearCurrentTarget();
            preDesignate = TRUE;
            postDrop = FALSE;
            inRange = TRUE;
            missileTarget = FALSE;

            if (isAI and not WeaponClassMatchesMaster(Sms->curWeaponClass))
                Sms->FindWeaponType(wtAgm65);

            if (WeaponClassMatchesMaster(Sms->curWeaponClass))
            {

                if (playerFCC)
                {
                    this->
                    Sms->StepMavSubMode(TRUE); // TRUE means initial step
                }

                /*
                if (PlayerOptions.GetAvionicsType() == ATRealistic or
                 PlayerOptions.GetAvionicsType() == ATRealisticAV)
                 SetSubMode (BSGT);
                else
                 SetSubMode (SLAVE);
                */
            }
            else
            {
                if (playerFCC)
                {
                    Sms->drawable->SetDisplayMode(SmsDrawable::Wpn);
                }
            }


            /*
            if (Sms->curWeaponClass not_eq wcAgmWpn)
            {
             if (Sms->FindWeaponClass (wcAgmWpn) and Sms->CurHardpoint() >= 0) // JB 010805 Possible CTD check curhardpoint
             {
             Sms->SetWeaponType(Sms->hardPoint[Sms->CurHardpoint()]->GetWeaponType());
             switch (Sms->curWeaponType)
             {
             case wtAgm65:
             // M.N. added full realism mode
             if (PlayerOptions.GetAvionicsType() == ATRealistic or PlayerOptions.GetAvionicsType() == ATRealisticAV)
             SetSubMode (BSGT);
             else
             SetSubMode (SLAVE);
             break;
             }
             }
             else
             {
             Sms->SetWeaponType (wtNone);

             if( playerFCC )
             {
             Sms->GetNextWeapon(wdGround);
             Sms->drawable->SetDisplayMode(SmsDrawable::Wpn);
             }
             }
            }
            */
            if (TheHud and playerFCC)
                TheHud->headingPos = HudClass::High;

            break;

        case AirGroundHARM:
            // Clear out any previous targets we had locked.
            ClearCurrentTarget();
            preDesignate = TRUE;
            postDrop = FALSE;

            // RV - I-Hawk - Get into the right HARM modes
            if (isAI and not WeaponClassMatchesMaster(Sms->curWeaponClass))
            {
                Sms->FindWeaponType(wtAgm88);
            }

            if (isAI)
            {
                harmPod->SetSubMode(HarmTargetingPod::HAS);
                harmPod->SetHandedoff(true);    // AI doesn't need any target hadnoff delay
            }

            else
            {
                harmPod->SetSubMode(HarmTargetingPod::HarmModeChooser);
            }

            /*
            if (Sms->curWeaponClass not_eq wcHARMWpn)
            {
             if (Sms->FindWeaponClass (wcHARMWpn, FALSE) and Sms->CurHardpoint() >= 0) // JB 010805 Possible CTD check curhardpoint
             {
             Sms->SetWeaponType(Sms->hardPoint[Sms->CurHardpoint()]->GetWeaponType());
             switch (Sms->curWeaponType)
             {
             case wtAgm88:
             SetSubMode (HTS);
             break;
             }
             }
             else
             {
             Sms->SetWeaponType (wtNone);

             if( playerFCC )
             {
             Sms->GetNextWeapon(wdGround);
             Sms->drawable->SetDisplayMode(SmsDrawable::Wpn);
             }
             }
            }
            */

            if (TheHud and playerFCC)
            {
                TheHud->headingPos = HudClass::High;
            }

            break;

        case AirGroundLaser:
            // Clear out any previous targets we had locked.
            ClearCurrentTarget();
            preDesignate = TRUE;
            postDrop = FALSE;

            if (isAI and not WeaponClassMatchesMaster(Sms->curWeaponClass))
                Sms->FindWeaponType(wtGBU);

            if (WeaponClassMatchesMaster(Sms->curWeaponClass))
            {
                if ( not g_bRealisticAvionics)
                {
                    //Mi this isn't true... doc states you start off in SLAVE
                    if (PlayerOptions.GetAvionicsType() == ATRealistic)
                        SetSubMode(BSGT);
                    else
                        SetSubMode(SLAVE);
                }
                else
                {
                    InhibitFire = FALSE;

                    // M.N. added full realism mode
                    if (PlayerOptions.GetAvionicsType() not_eq ATRealistic and 
                        PlayerOptions.GetAvionicsType() not_eq ATRealisticAV)
                        SetSubMode(BSGT);
                    else
                        SetSubMode(SLAVE);
                }
            }
            else
            {
                if (playerFCC)
                {
                    Sms->drawable->SetDisplayMode(SmsDrawable::Wpn);
                }
            }


            /*
            if (Sms->curWeaponClass not_eq wcGbuWpn)
            {
             if (Sms->FindWeaponClass (wcGbuWpn, FALSE) and Sms->CurHardpoint() >= 0) // JB 010805 Possible CTD check curhardpoint
             {
             Sms->SetWeaponType(Sms->hardPoint[Sms->CurHardpoint()]->GetWeaponType());
             switch (Sms->curWeaponType)
             {
             case wtGBU:
             //Mi this isn't true... doc states you start off in SLAVE
             if( not g_bRealisticAvionics)
             {
             if (PlayerOptions.GetAvionicsType() == ATRealistic)
             SetSubMode (BSGT);
             else
             SetSubMode (SLAVE);
             }
             else
             {
             InhibitFire = FALSE;
             // M.N. added full realism mode
             if(PlayerOptions.GetAvionicsType() not_eq ATRealistic and PlayerOptions.GetAvionicsType() not_eq ATRealisticAV)
             SetSubMode (BSGT);
             else
             SetSubMode (SLAVE);
             }
             break;
             }
             }
             else
             {
             Sms->SetWeaponType (wtNone);

             if( playerFCC )
             {
             Sms->GetNextWeapon(wdGround);
             Sms->drawable->SetDisplayMode(SmsDrawable::Wpn);
             }
             }
            }
            */
            if (TheHud and playerFCC)
                TheHud->headingPos = HudClass::High;

            break;

        case AirGroundCamera:
            // Clear out any previous targets we had locked.
            ClearCurrentTarget();
            preDesignate = TRUE;
            postDrop = FALSE;

            if (isAI and not WeaponClassMatchesMaster(Sms->curWeaponClass))
                Sms->FindWeaponClass(wcCamera);

            if (WeaponClassMatchesMaster(Sms->curWeaponClass))
            {
                SetSubMode(PRE); // MLR 2/14/2004 - who knows if this it correct
            }
            else
            {
                Sms->drawable->SetDisplayMode(SmsDrawable::Wpn);
            }

            /*
            if (Sms->curWeaponClass not_eq wcCamera)
            {
             if (Sms->FindWeaponClass (wcCamera, FALSE) and Sms->CurHardpoint() >= 0) // JB 010805 Possible CTD check curhardpoint
             {
             Sms->SetWeaponType(Sms->hardPoint[Sms->CurHardpoint()]->GetWeaponType());
             }
             else
             {
             Sms->SetWeaponType (wtNone);

             if( playerFCC )
             {
             Sms->GetNextWeapon(wdGround);
             Sms->drawable->SetDisplayMode(SmsDrawable::Wpn);
             }
             }
            }
            */
            if (TheHud and playerFCC)
                TheHud->headingPos = HudClass::High;

            strcpy(subModeString, "RPOD");
            platform->SOIManager(SimVehicleClass::SOI_RADAR);
            break;
    }

    // MLR 2/1/2004 - we check this last, because sometimes we fall back to gun mode in the switch above
    if (masterMode == AAGun)
    {
        if (g_bRealisticAvionics)
            SetSubMode(lastAirAirGunSubMode);
        else
            SetSubMode(EEGS);

        if ( not WeaponClassMatchesMaster(Sms->curWeaponClass))
            Sms->FindWeaponType(wtGuns);

        // we only want to store this as the previous weapon if g_bWeaponStepToGun is TRUE
        //if(g_bWeaponStepToGun)
        // lastAirAirHp = Sms->GetCurrentWeaponHardpoint();
        //lastAirAirWId=Sms->GetCurrentWeaponId();

        if (TheHud and playerFCC)
            TheHud->headingPos = HudClass::Low;
    }

    if (masterMode == AGGun)
    {
        {
            SetSubMode(STRAF);
        }

        if ( not WeaponClassMatchesMaster(Sms->curWeaponClass))
            Sms->FindWeaponType(wtGuns);

        // we only want to store this as the previous weapon if g_bWeaponStepToGun is TRUE
        //if(g_bWeaponStepToGun)
        // lastAirGroundHp = Sms->GetCurrentWeaponHardpoint();
        //lastAirGroundWId=Sms->GetCurrentWeaponId(); //>GetCurrentWeaponHardpoint();

        if (TheHud and playerFCC)
            TheHud->headingPos = HudClass::High;
    }

    // store master mode, used when leaving DF or MO.
    switch (masterMode)
    {
        case Dogfight:
        case MissileOverride:
            break;

        default:
            lastMasterMode = masterMode;
    }

    UpdateWeaponPtr();
    UpdateLastData();
}

void FireControlComputer::UpdateLastData(void)
{
    switch (masterMode)
    {
        case ILS:
        case Nav:
            lastNavMasterMode = masterMode;
            break;

        case Dogfight:
            lastDogfightHp        = Sms->GetCurrentWeaponHardpoint();
            break;

        case MissileOverride:
            lastMissileOverrideHp = Sms->GetCurrentWeaponHardpoint();
            break;

        case AAGun:
            break;

            //if( not g_bWeaponStepToGun)
            //{
            // break;
            //}
            // intentionally fall thru
        case Missile:
            //lastAaMasterMode = masterMode;
            lastAirAirHp = Sms->GetCurrentWeaponHardpoint();
            break;

        case AGGun:
            break;

            //if( not g_bWeaponStepToGun)
            //{
            // break;
            //}
            // intentionally fall thru
        case AirGroundBomb:
        case AirGroundRocket:
        case AirGroundMissile:
        case AirGroundHARM:
        case AirGroundLaser:
        case AirGroundCamera:
            lastAgMasterMode = masterMode;
            lastAirGroundHp = Sms->GetCurrentWeaponHardpoint();
            break;
    }
}


int FireControlComputer::WeaponClassMatchesMaster(WeaponClass wc)
{
    switch (masterMode)
    {
        case Missile:
            if (wc == wcAimWpn or wc == wcGunWpn)
                return 1;

            return 0;

        case Dogfight:
        case MissileOverride:
            if (wc == wcAimWpn)
                return 1;

            return 0;

        case AAGun:
        case AGGun:
            if (wc == wcGunWpn)
                return 1;

            return 0;

        case AirGroundMissile:
            if (wc == wcAgmWpn)
                return 1;

            return 0;
            break;

        case AirGroundBomb:
            if (wc == wcBombWpn) // or wc==wcRocketWpn)
                return 1;

            return 0;
            break;

        case AirGroundRocket:
            if (wc == wcRocketWpn) // or wc==wcRocketWpn)
                return 1;

            return 0;
            break;

        case AirGroundHARM:
            if (wc == wcHARMWpn)
                return 1;

            return 0;
            break;

        case AirGroundLaser:
            if (wc == wcGbuWpn)
                return 1;

            return 0;
            break;

        case AirGroundCamera:
            if (wc == wcCamera)
                return 1;

            return 0;
            break;
            /*
            case ClearOveride:
            if(wc==)
             return 1;
            return 0;
            break;
            */
    }

    return 0;
}

int FireControlComputer::CanStepToWeaponClass(WeaponClass wc)
{
    switch (masterMode)
    {
        case AAGun:
            if ((wc == wcAimWpn and g_bWeaponStepToGun) or
                wc == wcGunWpn)
                return 1;

            return 0;

        case Missile:
            if (wc == wcAimWpn or
                (wc == wcGunWpn and g_bWeaponStepToGun))
                return 1;

            return 0;

        case MissileOverride:
        case Dogfight:
            if (wc == wcAimWpn)
                return 1;

            return 0;

        case AGGun:
            if (((wc == wcAgmWpn or wc == wcBombWpn or
                  wc == wcRocketWpn or wc == wcHARMWpn or
                  wc == wcGbuWpn or wc == wcCamera) and g_bWeaponStepToGun) or
                wc == wcGunWpn)
                return 1;

            return 0;

        case AirGroundBomb:
        case AirGroundRocket:
        case AirGroundMissile:
        case AirGroundHARM:
        case AirGroundLaser:
        case AirGroundCamera:
            if (wc == wcAgmWpn or wc == wcBombWpn or
                wc == wcRocketWpn or wc == wcHARMWpn or
                wc == wcGbuWpn or wc == wcCamera  or
                (wc == wcGunWpn and g_bWeaponStepToGun))
                return 1;

            return 0;
    }

    return 0;

    /*
     switch(GetMainMasterMode())
    {
    case MM_AA:
     if(g_bWeaponStepToGun)
     {
     if( wc==wcAimWpn or wc==wcGunWpn )
     return 1;
     }
     else
     {

     }
     return 0;
    case MM_DGFT:
    case MM_MSL:
     if(wc==wcAimWpn )
     return 1;
     return 0;
    case MM_AG:
     if( wc==wcAgmWpn or wc==wcBombWpn or
     wc==wcRocketWpn or wc==wcHARMWpn or
     wc==wcGbuWpn or wc==wcCamera  or
     ( wc==wcGunWpn and g_bWeaponStepToGun ) )
     return 1;
     return 0;
    }
    return 0;
    */
}

void FireControlComputer::SetAAMasterModeForCurrentWeapon(void)
{
    //UpdateWeaponPtr();

    FCCMasterMode newmode = masterMode;

    if (Sms->GetCurrentWeaponHardpoint() == -1)
    {
        // maybe we've jetted the weapons, and the Sms curHardpoint is -1
        // anyhow, find what we were using before
        Sms->SetCurrentHardPoint(lastAirAirHp, 1);
    }

    inAAGunMode = 0;

    switch (Sms->curWeaponClass)
    {
        case wcAimWpn:
            newmode = Missile;
            break;

        case wcGunWpn:
            inAAGunMode = 1;
            newmode = AAGun;
            break;
    }

    SetMasterMode(newmode);
}

void FireControlComputer::SetAGMasterModeForCurrentWeapon(void)
{
    if (Sms->GetCurrentWeaponHardpoint() == -1)
    {
        // maybe we've jetted the weapons, and the Sms curHardpoint is -1
        // anyhow, find what we were using before
        Sms->SetCurrentHardPoint(lastAirGroundHp, 1);
    }

    //UpdateWeaponPtr();

    if (Sms->CurHardpoint() < 0)
    {
        // whoops
        return;
    }
    else if ( not Sms->hardPoint[Sms->CurHardpoint()])
        return;

    FCCMasterMode newmode = masterMode;

    inAGGunMode = 0;

    switch (Sms->hardPoint[Sms->CurHardpoint()]->GetWeaponClass())
    {
        case wcRocketWpn:
            newmode = AirGroundRocket;
            break;

        case wcBombWpn:
            newmode = AirGroundBomb;
            break;

        case wcGunWpn:
            inAGGunMode = 1;
            newmode = AGGun;
            break;

        case wcAgmWpn:
            newmode = AirGroundMissile;
            break;

        case wcHARMWpn:
            newmode = AirGroundHARM;
            break;

        case wcGbuWpn:
            newmode = AirGroundLaser;
            break;

        case wcCamera:
            newmode = AirGroundCamera;
            break;

        default:
            newmode = AirGroundBomb;
            /*
            case wcSamWpn:
             break;
            case wcNoWpn:
             break;
            case wcECM:
             break;
            case wcTank:
             break;
             */
    }

    SetMasterMode(newmode);
}


int  FireControlComputer::IsInAAMasterMode(void)
{
    return(GetMainMasterMode() == MM_AA);
}

int  FireControlComputer::IsInAGMasterMode(void)
{
    return(GetMainMasterMode() == MM_AG);
}

void FireControlComputer::EnterAAMasterMode(void)
{
    if (inAAGunMode)
    {
        // go to gun mode
        if (Sms->FindWeaponClass(wcGunWpn, TRUE))
            SetMasterMode(AAGun);
    }
    else
    {
        Sms->SetCurrentHardPoint(lastAirAirHp);
        //SetMasterMode(lastAaMasterMode);
        SetMasterMode(Missile);
    }
}

void FireControlComputer::EnterAGMasterMode(void)
{
    if (inAGGunMode)
    {
        // go to gun mode
        if (Sms->FindWeaponClass(wcGunWpn, TRUE))
            SetMasterMode(AGGun);
    }
    else
    {
        Sms->SetCurrentHardPoint(lastAirGroundHp);
        SetMasterMode(lastAgMasterMode);
    }
}

void FireControlComputer::EnterMissileOverrideMode(void)
{
    Sms->SetCurrentHardPoint(lastMissileOverrideHp);
    SetMasterMode(MissileOverride);
}

void FireControlComputer::EnterDogfightMode(void)
{
    Sms->SetCurrentHardPoint(lastDogfightHp);
    SetMasterMode(Dogfight);
}



void FireControlComputer::ToggleAAGunMode(void)
{
    inAAGunMode = not inAAGunMode;
    EnterAAMasterMode();
}

void FireControlComputer::ToggleAGGunMode(void)
{
    inAGGunMode = not inAGGunMode;
    EnterAGMasterMode();
}


void FireControlComputer::UpdateWeaponPtr(void)
{
    // the fccWeaponPointer is ONLY used to access weapon data, impact prediction etc, it is NEVER fired.
    int wid;

    wid = Sms->GetCurrentWeaponId();

    if (wid not_eq fccWeaponId)
    {
        fccWeaponPtr.reset();
        fccWeaponId = wid;

        if (fccWeaponId)
        {
            Falcon4EntityClassType *classPtr = GetWeaponF4CT(fccWeaponId);

            if (classPtr)
            {
                if (
                    classPtr->vuClassData.classInfo_[VU_TYPE] == TYPE_MISSILE or
                    classPtr->vuClassData.classInfo_[VU_TYPE] == TYPE_ROCKET
                )
                {
                    fccWeaponPtr.reset(InitAMissile(Sms->Ownship(), fccWeaponId, 0));
                }
                else
                {
                    if (
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
                        ) or
                        (
                            classPtr->vuClassData.classInfo_[VU_TYPE] == TYPE_LAUNCHER and 
                            classPtr->vuClassData.classInfo_[VU_STYPE] == STYPE_ROCKET
                        )
                    )
                    {
                        fccWeaponPtr.reset(InitABomb(Sms->Ownship(), fccWeaponId, 0));
                    }
                }
            }

            if (fccWeaponPtr and fccWeaponPtr->IsLauncher())
            {
                wid = ((BombClass *)fccWeaponPtr.get())->LauGetWeaponId();

                if (wid not_eq rocketWeaponId)
                {
                    if (wid)
                    {
                        rocketPointer.reset((MissileClass *)InitAMissile(Sms->Ownship(), wid, 0));
                    }
                    else
                    {
                        rocketPointer.reset(0);
                    }

                    rocketWeaponId = wid;
                }
            }
        }
    }
}

void FireControlComputer::SetSms(SMSClass *SMS)
{
    Sms = SMS;

    // setup default HPs for MMs

    // dogfight
    if ( not Sms->FindWeaponType(wtAim9))
        Sms->FindWeaponType(wtAim120);

    lastDogfightHp = Sms->CurHardpoint();

    // missile override bitand aamm
    if ( not Sms->FindWeaponType(wtAim120))
        Sms->FindWeaponType(wtAim9);

    lastMissileOverrideHp = Sms->CurHardpoint();
    lastAirAirHp   = Sms->CurHardpoint();

    // agmm
    /*if( not Sms->FindWeaponType (wtAgm88))
     if( not Sms->FindWeaponType (wtAgm65))
     if( not Sms->FindWeaponType (wtGBU))
     if( not Sms->FindWeaponType (wtGPS))
     if( not Sms->FindWeaponType (wtMk84))
     if( not Sms->FindWeaponType (wtMk82))
     Sms->FindWeaponClass (wcRocketWpn); // used to be: Sms->FindWeaponType (wtLAU); but jammers are marks as wtLAU :rolleyes:*/

    //Cobra
    if ( not Sms->FindWeaponType(wtAgm88))
        if ( not Sms->FindWeaponType(wtAgm65))
            if ( not Sms->FindWeaponType(wtGBU))
                if ( not Sms->FindWeaponType(wtGPS))
                    if ( not Sms->FindWeaponType(wtMk84))
                        if ( not Sms->FindWeaponType(wtMk82))
                            if ( not Sms->FindWeaponClass(wcRocketWpn))
                                Sms->FindWeaponClass(wcGunWpn);

    //end

    if (Sms->CurHardpoint() > 0)
    {
        switch (Sms->hardPoint[Sms->CurHardpoint()]->GetWeaponClass())
        {
            case wcRocketWpn:
                lastAgMasterMode = AirGroundRocket;
                break;

            case wcBombWpn:
                lastAgMasterMode = AirGroundBomb;
                break;

            case wcAgmWpn:
                lastAgMasterMode = AirGroundMissile;
                break;

            case wcHARMWpn:
                lastAgMasterMode = AirGroundHARM;
                break;

            case wcGbuWpn:
                lastAgMasterMode = AirGroundLaser;
                break;

            case wcCamera:
                lastAgMasterMode = AirGroundCamera;
                break;

            default:
                lastAgMasterMode = AirGroundBomb;
        }
    }

    lastAirGroundHp =  Sms->CurHardpoint();

    // Set us up in Gun mode if we have no AA or AG stores
    if (lastAirAirHp == -1)
    {
        inAAGunMode = 1;
    }

    //Cobra change to HP 0 for Gun from -1
    if (lastAirGroundHp == 0)
    {
        inAGGunMode = 1;
    }
}

// RV - I-Hawk - Added function
bool FireControlComputer::AllowMaddog()
{
    if (Sms and Sms->GetCurrentWeaponType() == wtAim120)
    {
        MissileClass* currMissile = (MissileClass*)Sms->GetCurrentWeapon();

        if ( not targetPtr and currMissile and currMissile->isSlave)
        {
            return false;
        }
    }

    return true;
}



