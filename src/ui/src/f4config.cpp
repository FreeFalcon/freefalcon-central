////////////////////////////////////////////////
// Falcon4.cfg stuff

#include <cISO646>
#include <stdio.h>
#include <windows.h>
#include "../../SIM/INCLUDE/Phyconst.h" //JAM 19Sep03

template<class T>
class ConfigOption
{
public:
    char *Name;
    T *Value;

    bool CheckID(char *str)
    {
        return stricmp(&str[3], Name) == 0;
    }
};

extern char FalconDataDirectory[];
extern "C" int g_nBWMaxDeltaTime; // needed to link into C files (capi.c)
extern "C" int g_nBWCheckDeltaTime;

// #define OPENEYES //; disabled until futher
//bool g_bEnableCATIIIExtension = false; //MI replaced with g_bRealisticAvionics
bool g_bWakeTurbulence = true;
bool g_bDrawWakeTurbulence = false;
bool g_bWeaponLaunchUsesDrawPointerPos = false; // MLR 2/19/2004 - when weapons are launched, they are launched for
// where thier 3d graphics were positioned, this will allow the F-111's
// weapons to be launched from the right spot ** DOESN'T WORK

bool g_bExtViewOnGround = false; // RAS -5Dec04- allow ext view if on ground
bool g_bForceDXMultiThreadedCoopLevel = true; // JB 010401 Safer according to e
int g_nThrottleMode = 0;
bool g_bEnableABRelocation = false;
bool g_bEnableWeatherExtensions = true;
bool g_bEnableWindsAloft = false;
bool g_bEnableColorMfd = true;
bool g_bEPAFRadarCues = false;
bool g_bRadarJamChevrons = false;
bool g_bAWACSSupport = false;
bool g_bAWACSRequired = false;
bool g_bAWACSBackground = false;
bool g_bMLU = false;
bool g_bServer = false;
bool g_bServerHostAll = false;
bool g_bLogEvents = false;
bool g_bVoiceCom = true;
bool g_bwoeir = false;
int g_nPadlockBoxSize = 2;
int g_nDeagTimer = 0;
int g_nReagTimer = 0;
bool g_bShowFlaps = false;
bool g_bLowBwVoice = false;
float clientbwforupdatesmodifyer = 0.7f;
float hostbwforupdatesmodifyer = 0.7f;
int g_nMaxUIRefresh = 16; // 2002-02-23 S.G. To limit the UI refresh rate to prevent from running out of resources because it can't keep up with the icons (ie planes) to display on the map.
int g_nUnidentifiedInUI = 1; // 2002-02-24 S.G. To limit the UI refresh rate to prevent from running out of resources because it can't keep up with the icons (ie planes) to display on the map.
float g_fIdentFactor = 0.75f; // 2002-03-07 S.G. So identification is not at full detect range but a factor of it
bool g_bLimit2DRadarFight = true; // 2002-03-07 S.G. So 2D fights are limited in min altitude and range like their 3D counterpart
bool g_bAdvancedGroundChooseWeapon = true; // 2002-03-08 S.G. So 3D ground vehicle choose the best weapon based target min/max altitude and min/max range while the original code was just range (max range, not min range)
bool g_bUseNewCanEnage = true; // 2002-03-11 S.G. SensorFusion and CanEngage will use the 'GetIdentified' code instead of always knowing the combat type of the enemy
int g_nLowestSkillForGCI = 3; // 2002-03-12 S.G. Externalized the lowest skill that can use GCI
int g_nAIVisualRetentionTime  = 24 * 1000; // 2002-03-12 S.G. Time before AI looses sight of its target
int g_nAIVisualRetentionSkill =  2 * 1000; // 2002-03-12 S.G. Time before AI looses sight of its target (skill related)
float g_fBiasFactorForFlaks = 100000.0f; // 2002-03-12 S.G. Defaults bias for flaks. See guns.cpp
bool g_bUseSkillForFlaks = true; // 2002-03-12 S.G. If flaks uses the skill of the ground troop or not
float g_fTracerAccuracyFactor = 0.1f; // 2002-03-12 S.G. For tracers, multiply the dispersion (tracerError) by this value
bool g_bToggleAAAGunFlag = false; // 2002-03-12 S.G. RP5 have set the AAA flag for NONE AAA guns and have reset it for AAA guns This flag toggle the AAA gun check in the code
bool g_bUseComplexBVRForPlayerAI = false; // 2002-03-13 S.G. If false, Player's wingman will perform RP5 BVR code instead of the SP2 BVR code
float g_fFuelBaseProp = 100.0f; // 2002-03-14 S.G. For better fuel consomption tweaking
float g_fFuelMultProp = 0.008f; // 2002-03-14 S.G. For better fuel consomption tweaking
float g_fFuelTimeStep = 0.001f; // 2002-03-14 S.G. For better fuel consomption tweaking
bool g_bFuelUseVtDot = true; // 2002-03-14 S.G. For better fuel consomption tweaking
float g_fFuelVtClip = 5.0f; // 2002-03-14 S.G. For better fuel consomption tweaking
float g_fFuelVtDotMult = 5.0f; // 2002-03-14 S.G. For better fuel consomption tweaking
bool g_bFuelLimitBecauseVtDot = true; // 2002-03-14 S.G. For better fuel consomption tweaking
float g_fSearchSimTargetFromRangeSqr = (20.0F * NM_TO_FT) * (20.0F * NM_TO_FT); // 2002-03-15 S.G. Will lookup Sim target instead of using the campain target from this range
bool g_bUseAggresiveIncompleteA2G = true; // 2002-03-22 S.G. If false, AI on incomplete A2G missions will be defensive
float g_fHotNoseAngle = 50.0f;  // 2002-03-22 S.G. Default angle (in degrees) before considering the target pointing at us
float g_fMaxMARNoIdA = 10.0f;  // 2002-03-22 ADDED BY S.G. Max Start MAR for this type of aicraft when target is NOT ID'ed, fast
float g_fMinMARNoId5kA = 5.0f;  // 2002-03-22 ADDED BY S.G. MinMAR for this type of aicraft when target is NOT ID'ed, fast and below 5K
float g_fMinMARNoId18kA = 12.0f; // 2002-03-22 ADDED BY S.G. MinMAR for this type of aicraft when target is NOT ID'ed, fast and below 18K
float g_fMinMARNoId28kA = 17.0f; // 2002-03-22 ADDED BY S.G. MinMAR for this type of aicraft when target is NOT ID'ed, fast and below 28K
float g_fMaxMARNoIdB = 5.0f;  // 2002-03-22 ADDED BY S.G. Max Start MAR for this type of aicraft when target is NOT ID'ed, medium
float g_fMinMARNoId5kB = 3.0f;  // 2002-03-22 ADDED BY S.G. MinMAR for this type of aicraft when target is NOT ID'ed, medium and below 5K
float g_fMinMARNoId18kB = 5.0f;  // 2002-03-22 ADDED BY S.G. MinMAR for this type of aicraft when target is NOT ID'ed, medium and below 18K
float g_fMinMARNoId28kB = 8.0f;  // 2002-03-22 ADDED BY S.G. MinMAR for this type of aicraft when target is NOT ID'ed, medium and below 28K
float g_fMinMARNoIdC = 5.0f;  // 2002-03-22 ADDED BY S.G. MinMAR for this type of aicraft when target is NOT ID'ed
bool g_bOldStackDump = false;  // 2002-04-01 ADDED BY S.G. Also output the stack dump in the old format when generating a crashlog.
float g_fSSoffsetManeuverPoints1a = 5.0f; // 2002-04-07 ADDED BY S.G. Externalize the offset used in the AiInitSSOffset code
float g_fSSoffsetManeuverPoints1b = 5.0f; // 2002-04-07 ADDED BY S.G. Externalize the offset used in the AiInitSSOffset code
float g_fSSoffsetManeuverPoints2a = 4.0f; // 2002-04-07 ADDED BY S.G. Externalize the offset used in the AiInitSSOffset code
float g_fSSoffsetManeuverPoints2b = 5.0f; // 2002-04-07 ADDED BY S.G. Externalize the offset used in the AiInitSSOffset code
float g_fPinceManeuverPoints1a = 5.0f; // 2002-04-07 ADDED BY S.G. Externalize the offset used in the AiInitPince code
float g_fPinceManeuverPoints1b = 5.0f; // 2002-04-07 ADDED BY S.G. Externalize the offset used in the AiInitPince code
float g_fPinceManeuverPoints2a = 4.0f; // 2002-04-07 ADDED BY S.G. Externalize the offset used in the AiInitPince code
float g_fPinceManeuverPoints2b = 5.0f; // 2002-04-07 ADDED BY S.G. Externalize the offset used in the AiInitPince code
bool g_bUseDefinedGunDomain = FALSE; // 2002-04-17 ADDED BY S.G. Instead of 'fudging' the weapon domain, if it's set to true, use the weapon domain set in the data file

// 2000-11-24 ADDED BY S.G. FOR THE 'new padlock' code
#define PLockModeNormal 0
#define PLockModeNearLabelColor 1
#define PLockModeNoSnap 2
#define PLockModeBreakLock 4
#define PLockNoTrees 8
int g_nPadlockMode = PLockModeNoSnap bitor PLockModeBreakLock bitor PLockNoTrees;

// 2001-08-31 ADDED BY S.G. FOR AIRBASE RELOCATION CHOICE
#define AirBaseRelocTeamOnly 1
#define AirBaseRelocNoFar 2
int g_nAirbaseReloc = 2;

// 2002-01-30 ADDED BY S.G. Pitch limiter for AI activation variable
bool g_bPitchLimiterForAI = true;

// 2002-01-29 ADDED BY S.G. Default timeout for target bubbles
int g_nTargetSpotTimeout = 2 * 60 * 1000;

// 2001-09-07 ADDED BY S.G. FOR RP5 COMPATIBILITY DATA TEST
bool g_bRP5Comp = true;

extern int NumHats;
bool g_bEnableNonPersistentTextures = false;
bool g_bEnableStaticTerrainTextures = false;
//bool g_bEnableAircraftLimits = false; //MI replaced with g_bRealisticAvionics
//bool g_bArmingDelay = false; //MI replaced with g_bRealisticAvionics
//bool g_bHardCoreReal = false; //MI replaced with g_bRealisticAvionics
bool g_bCheckBltStatusBeforeFlip = true;
bool g_bForceSoftwareGUI = false;
//JAM 20Sep03 bool g_bUseMipMaps = false;
bool g_bShowMipUsage = false;
bool g_bUse3dSound = false; // JPO
//bool g_bUse3dSound = true; //       MLR 2003-11-17 Going to hardcode this on
bool g_bOldSoundAlg = true; // JPO // MLR 12/7/2003 - renamebled
int  g_nDynamicVoices = 16; // MLR 1/26/2004 - max # of dynamically allocated voices
bool g_bSoundSonicBoom = 1;  // MLR 3/16/2004 - Do sonic boom effect
bool g_bMultiEngineSound = 1; // MLR 5/18/2004 - play a sound for each engine?

bool g_bMFDHighContrast = false; // JPO
bool g_bPowerGrid = true; // JPO
bool g_bUseMappedFiles = false; // JPO
//bool g_bUserRadioVoice = false; // JPO // M.N. transferred into UI/PlayerOp structure
bool g_bNewRackData = true; // JPO
bool g_bBMSRackData = false; // MLR 2/13/2004 -
bool g_bNewAcmiHud = true; // JPO
int g_nLowDetailFactor = 0; // JPO - adjustment to the LOD show at low level
float g_fHUDonlySize = 0.0f; // FRB - % Size increase of HUD-Only view (% = decimal)

float g_fMipLodBias;
float g_fCloudMinHeight = -1.0f; // JPO
float g_fRadarScale = 1.0f; // JPO
float g_fCursorSpeed = 1.0f; // JPO
float g_fMinCloudWeather = 1500; // JPO
float g_fCloudThicknessFactor = 4000; //JPO

float g_fdwPortclient = 2937;
float g_fdwPorthost = 2936;

bool g_bEnableUplink = false;
char g_strMasterServerName[0x40];
int g_nMasterServerPort = 0;
char g_strServerName[0x40];
char g_strServerLocation[0x40];
char g_strServerAdmin[0x40];
char g_strServerAdminEmail[0x40];

char g_strVoiceHostIP[0x40];
char g_strWorldName[0x40] = {"FFVIPER"};
char g_strScrollUpFunction[0x40] = "FOVDecrease"; //Wombat778 11-16-2003 swapped FOVIncrease and Decrese.  It was backwards 10-07-2003
char g_strScrollDownFunction[0x40] = "FOVIncrease"; //Wombat778 10-07-2003
char g_strMiddleButtonFunction[0x40] = "FOVDefault"; //Wombat778 10-07-2003


// JB
#ifdef _DEBUG
int g_nNearLabelLimit = 150; // nm M.N.
#else
int g_nNearLabelLimit = 20; // nm
#endif
int g_nACMIOptionsPopupHiResX = 666;
int g_nACMIOptionsPopupHiResY = 581;
int g_nACMIOptionsPopupLowResX = 500;
int g_nACMIOptionsPopupLowResY = 500;
int g_nMaxSimTimeAcceleration = 64; // JB 020315
bool g_bMPStartRestricted = false; // JB 0203111 Restrict takeoff/ramp options.
int g_nMPStartTime = 5; // JB 0203111 MP takeoff time if g_bMPStartRestricted enabled.
int g_nTaxiLaunchTime = 2; // Booster 2004/10/12 Taxi takeoff time option, default 2
int g_nFFEffectAutoCenter = -1; // JB 020306 Don't stop the centering FF effect (-1 disabled)
bool g_bMissionACIcons = true; // JB 020211 Load the correct mission icons for each type of aircraft
float g_fRecoveryAOA = 35.0F; // JB 020125 Specify the max AOA at which you can recover from a deep stall.
int g_nRNESpeed = 1; // JB 020123 More realistic No Escape DLZ.  Specify higher g_nRNESpeed to lower calculated RNE ranges.
bool g_bSlowButSafe = false; // JB 020115 Turn on extra ISBad CTD checks
float g_fCarrierStartTolerance = 20.0f; // JB 020117 How high can an aircraft be off the water to be "on" the carrier.
bool g_bNewDamageEffects = true;
bool g_bDisableFunkyChicken = true;
bool g_bSmartScaling = false; // JB 010112
bool g_bFloatingBullseye = false;// JB/Codec 010115
bool g_bDisableCrashEjectCourtMartials = true; // JB 010118
bool g_bSmartCombatAP = true; // JB 010224
//bool g_bVoodoo12Compatible = false; // JB 010330 Disables the cockpit kneemap to prevent CTDs on the Voodoo 1 and 2.
float g_fDragDilutionFactor = 1.0; // JB 010707
bool g_bRealisticAttrition = false; // JB 010710
bool g_bIFFRWR = false; // JB 010727
int g_nRelocationWait = 3; // JB 010728
int g_nLoadoutTimeLimit = 120; // JB 010729 Time limit in seconds before takeoff when you can change your loadout.
float g_fLatitude = 38.0f; // JB 010804 now set up by the theater
int g_nYear = 2004; // JB 010804;
int g_nDay = 135; // JB 010804
bool g_bSimpleFMUpdates = false; // JB 010805 // These update cause bad AI behaviour, see afsimple.cpp
bool g_b3dDynamicPilotHead = false; // JB 010804
// JB 010802
bool g_b3dCockpit = true;
bool g_b3dMFDLeft = true;
bool g_b3dMFDRight = true;
bool g_bRWR = true;
bool g_bBrightHUD = false;
#if 0
bool g_b3dHUD = true;
bool g_b3dRWR = true;
bool g_b3dICP = true;
bool g_b3dDials = true;
float g_f3dlMFDulx = 18.205f;
float g_f3dlMFDuly = -7.089f;
float g_f3dlMFDulz = 7.793f;

float g_f3dlMFDurx = 18.205f;
float g_f3dlMFDury = -2.770f;
float g_f3dlMFDurz = 7.793f;

float g_f3dlMFDllx = 17.501f;
float g_f3dlMFDlly = -7.089f;
float g_f3dlMFDllz = 11.816f;

float g_f3drMFDulx = 18.181f;
float g_f3drMFDuly = 2.834f;
float g_f3drMFDulz = 7.883f;

float g_f3drMFDurx = 18.181f;
float g_f3drMFDury = 6.994f;
float g_f3drMFDurz = 7.883f;

float g_f3drMFDllx = 17.475f;
float g_f3drMFDlly = 2.834f;
float g_f3drMFDllz = 11.978f;
#endif
// JB
int g_nNoPlayerPlay = 2; // JB 010926
bool g_bModuleList = false; // JB 010101

int g_npercentage_available_aircraft = 75; //me123
int g_nminimum_available_aircraft = 4; //me123
int g_nMaxVertexSpace = -1; // JPO - graphics option
int g_nMinTacanChannel = 70; // JPO - tacan variable for other theaters.
int g_nFlightVisualBonus = 1; // JPO - flight visual detection bonus

int g_nChatterInterval = 5; // FRB  - Interval between certain chatter messages (seconds).  Reduces chatter noise.
// Cobra - SFX display limits
int g_nSfxLODCutoff = 800;
int g_nSfxLODDistCutoff = 6000;
int g_nSfxLODTotCutoff = 8000;
// Cobra - PS list purge interval (msec)
int g_nPSPurgeInterval = 60000;
// Cobra - Use old dust trail SFX
bool g_bOldDustTrail = false; // 1 = use old dust trail (from trail.txt)
bool g_bHearThunder = true; // Cobra - 1 = Play thunder.wav 0 = no thunder sound
int g_nPSKillFPS = 0;  // Cobra - Stop PS effects when FPS drops below g_nPSKillFPS
bool g_bHighSFX = false; // Cobra - Switch between internal high-activity and low-activity PS effects
bool g_bAllHaveIFF = false; // Cobra - Give all a/c IFF interrogator
bool g_bAnimPilotHead = true; // Cobra - Animate the pilot's head
float g_fPilotActInterval = 0.5f; // Cobra - Pilot animation act interval (minutes)
float g_fPilotHeadMoveRate = 50.0f; // Cobra - Pilot animation head move rate (deg/sec)
bool g_bUseRC135 = false; // Cobra = FRB - Use the RC-135 for ELINT (radar) ID'ing
bool g_bFFDBC = false; // FF DB Control
bool g_bCATIIIDefault = false;
bool g_bRealisticAvionics = true; // M.N. now changed by UI, Avionics "Realistic" = true, "Enhanced" = false
bool g_bIFlyMirage = false; //MI support for a possible new mirage
bool g_bNoMFDsIn1View = false;
bool g_bGreyScaleMFD = false;
bool g_bGreyMFD = true;
bool g_bIFF = true;//Cobra 11/20/04
bool g_bINS = true;
bool g_bNoRPMOnHud = true;
bool g_bNoPadlockBoxes = false; //MI 18/01/02 removes the box around padlocked objects
bool g_bFallingHeadingTape = false; //28/02/02 let's the heading tape fall off of the hud, for those who want it.
bool g_bTFRFixes = true;
bool g_bCalibrateTFR_PitchCtrl = false;
bool g_bLantDebug = false;
bool g_bNewPitchLadder = true;
float g_fGroundImpactMod = 0.0F; // Grndfcc groundZ modification from S.G. / RP5
bool  g_bAGRadarFixes = true;
float g_fGMTMinSpeed = 3.0F; // min Vt to be displayed on GMT radar
float g_fGMTMaxSpeed = 100.0F;   // max Vt to be displayed on GMT radar
float g_fReconCameraHalfFOV = 8.4F;
float g_fReconCameraOffset = -8.0F;
float g_fBombTimeStep = 0.05F; //original 0.25F;
bool g_bBombNumLoopOnly = true;
float g_fHighDragGravFactor = 0.65F;
bool g_bTO_LDG_LightFix = true;
//MI
bool g_bNewFm = true;//me123 new flight model
bool g_bAIRefuelInComplexAF = false; // 2002-02-20 ADDED BY S.G. Test to see if the AI can refuel in complex AF

//M.N.
float g_fFormationBurnerDistance = 10.0F; // M.N. 2001-10-29 - allow burner distance to lead when not in formation
//float g_fHitChanceAir = 3.5F; // Only added to test out the best value. 6 seems to high (CampLIB/unit.cpp)
//float g_fHitChanceGround = 2.0F; // moved into Falcon4.aii in campaign\save folder
bool g_bHiResUI = true; // false = 800x600, true = 1024x768
bool g_bAWACSFuel = false; // for debug, shows fuel of flight in UI when AWACSSupport = true
//bool g_bShowManeuverLabels = true; // for debug, shows currently performed BVR/WVR maneuver in SIM
bool g_bFullScreenNVG = true; // a NVG makes tunnel vision, but a pilot can turn around his head...
bool g_bLogUiErrors = false; // debug UI
bool g_bLoadoutSquadStoreResupply = true; // code checked bitand working
bool g_bDisplayTrees = false; // if true, loads falcon4tree.fed/ocd instead of falcon4.fed/ocd. If tree version not available, loads falcon4.fed/ocd
bool g_bRequestHelp = true; // enable RequestHelp in DLOGIC.cpp
bool g_bLightsKC135 = true; // once we have the KC-135 with director lights...
float g_fPadlockBreakDistance = 6.0F; // nm
bool g_bOldSamActivity = false; // for switching 3D sams also by 2D code
bool g_bFireOntheMove = false; // FR - SAMs can fire while moving (testing switch)
// better keyboard  control support
float g_fAFRudderRight = 1.5f;
float g_fAFRudderLeft = 1.5f;
float g_fAFThrottleDown = 0.03f;
float g_fAFThrottleUp = 0.03f;
float g_fAFAileronLeft = 6.0f;
float g_fAFAileronRight = 6.0f;
float g_fAFElevatorDown = 3.0f;
float g_fAFElevatorUp = 3.0f;
float g_frollStickOffset = 0.0f;
float g_fpitchStickOffset = 0.9f;
float g_frudderOffset = 0.9f;
bool g_bRollLinkedNWSRudder = true; // ASSOCIATOR 30/11/03: Controls whether the Rudder and NWS are linked to the roll input on the ground when you don't have a Rudder control device
bool g_bEnableGRCursorBullseye = false; // ASSOCIATOR 03/12/03: Enables Bullseye cursor in Ground Radar modes
// MD -- 20040808: disabling this one -- see comments on why in fccmain.cpp
bool g_bEnableFCCSubNavCycle = false; // ASSOCIATOR 04/12/03: Enables you to cycle the Nav steerpoint modes modes with the FCC submodes key
bool g_bDisableCommsBorder = false; // ASSO: disable the radio comms menu border
bool g_bEcmOnHud = false; // ASSO:
bool g_bBrakeOnHud = false; // ASSO:
bool g_bGearOnHud = false; // ASSO:

bool g_bAddACSizeVisual = true; // adds drawpointer radius value to eyeball GetSignature()
float g_fVisualNormalizeFactor = 40.0F; // 40.0F = F-16 drawpointer radius
//bool g_bShowFuelLabel = false; // for debugging fuel consumption in 3D replaced by label debug stuff
bool g_bHelosReloc = true; // A.S. relocate helo squadrons faster
bool g_bNewPadlock = true;
int g_nlookAroundWaterTiles = 2; // we've 2 tile bridges, so use "2" here
float g_fPullupTime = 0.2f; // Cobra - pull up for 0.2 seconds before reevaluating
float g_fGALookAheadTime = 10.f; // Cobra - Look ahead deltaX-deltaY times 10.f for higher ground to avoid
int g_nCriticalPullup = 3; // Cobra - <= g_fGALookAheadTime tick full pStick pullup
float g_fAIMinWPAlt = 200.0F; // Cobra - Minimum alt AI will fly at while flying WP Nav
float g_fAIMinAlt = 200.0F; // Cobra - Minimum alt AI will fly at
float g_fGApStickFac = 0.25F; // Cobra - Smooth out Ground Avoidance pitch (pStick * g_fGApStickFac) g_fAIHarmMaxRange
float g_fAIHarmMaxRange = 50.0F; // Cobra - Limit AI Harm max firing range (NM).
float g_fAIJSOWMaxRange = 30.0F; // Cobra - Limit AI Harm max firing range (NM).
float g_fRocketPitchFactor = -0.5F; // Cobra - A/C fine pitch adjustment (deg) to correct rocket hit location
float g_fRocketPitchCorr = 1.5F; // Cobra - A/C pitch correction (deg) to correct rocket hit location
float g_fAGFlyoutRange = 8.0F; // Cobra - Distance (NM) AI flyout after A2G pass
float g_fAGSlowFlyoutRange = 4.0f; // Cobra - Distance (NM) slow-mover AI (e..g., A-10s) flyout after A2G pass
float g_fAGSlowMoverSpeed = 270.0f; // Cobra - Max A2G attack speed (Knots) used to identify slow-movers (e..g., A-10s)
float g_fAIRefuelRange = 10.0F; // range to the tanker at which AI asks for fuel
bool g_bNewRefuelHelp = true; // 2002-02-28 more refuel help for the player
bool g_bOtherGroundCheck = false; // try the old algorithm together with the new pullup timer
bool g_bAIGloc = false; // turns on/off AI GLoc prediction
float g_fAIDropStoreLauncherRange = 10.0F; // if launcher is outside 10 nm, don't drop stores
int g_nAirbaseCheck = 30; // each x seconds check distance to closest airbase at bingo states, RTB at fumes
bool g_bUseTankerTrack = true; // tanker flies track box 60 * 25 nm
float g_fTankerRStick = 0.2f; // RStick in wingmnvers.cpp in SimpleTrackTanker (to finetune turning rate)
float g_fTankerPStick = 0.01f; // PStick in wingmnvers.cpp in SimpleTrackTanker (to finetune turning rate)
float g_fTankerTrackFactor = 0.5f; // adds a distance in nm in front of the tanker track points if we need to start turn earlier
float g_fTankerHeadsupDistance = 2.5f; // this is the distance to trackpoint when "Heads up, tanker is entering turn" is called out
float g_fTankerBackupDistance = 3.0f; // this is the "backup turn distance" to keep the tanker from circling a trackpoint
float g_fHeadingStabilizeFactor = 0.004f; // this is the heading difference to the trackpoint at which rStick is set to zero to stabalize the tanker in leveled flight
float g_fAIRefuelSpeed = 1.0f; // If we want to speed up AI refueling later
float g_fClimbRatio = 0.3f; // Used in Camptask\Mission.cpp for fixing too steep climbs
float g_fNukeStrengthFactor = 0.2f; // modifier for proximity damage (Bombmain.cpp)
float g_fNukeDamageMod = 10000.0f; // range damage modifier in Bombmain.cpp for nukes
float g_fNukeDamageRadius = 10.0f; // radius of proximity damage for objectives in nm
int g_nNoWPRefuelNeeded = 2000; // amount of needed fuel which doesn't trigger tanker WP creation
bool g_bAddIngressWP = true; // add ingress waypoint if needed
bool g_bTankerWaypoints = true; // add tanker waypoints if needed
bool g_bPutAIToBoom = true; // hack: put AI sticking to the boom when close to it
float g_fWaypointBurnerDelta = 700.0f; // burnerdelta for WaypointMode and WingyMode
int g_nSkipWaypointTime = 30000; // time in milliseconds added to waypoint departure time at which flight switches to next waypoint
bool g_bLookCloserFix = true; // fixes look closer view through the cockpit
float g_fMavEXPLevel = 3.0f; // Wombat778 9-28-03 Sets the Maverick EXP zoom level to 3 degrees
float g_fMavFOVLevel = 6.0f; // Wombat778 9-28-03 Sets the Maverick FOV Zoom level to 6 degrees
bool g_bAnyWaypointTask = true; // Wombat778 9-27-03 allows selection of any task to any waypoint
bool g_bFFCenterFix = false; // Wombat778 9-29-03 causes FF sticks to retain standard constant centering force
bool g_bCockpitAutoScale = true; // Wombat778 10-06-03 allows the cockpit to auto scale
float g_fMouseLookSensitivity = 0.5f; // Wombat778 10-08-03 Sets the new Mouselook sensitivity to level to 0.5 ( 1 was too fast)
bool g_b3DClickableCockpitDebug = false;// Wombat778 10-10-03 Sets debug mode to true for the clickable cockpit (shows button locations);
bool g_b3DRTTCockpitDebug = false;// Wombat778 10-10-03 Sets debug mode to true for the clickable cockpit (shows button locations);
bool g_b3DClickableCursorChange = true;// Wombat778 10-15-03 When enabled, causes the 3d cockpit mouse cursor to change color over a button. Perf impact.
bool g_bDEDSpacingFix = true; // Wombat778 12-12-2003 changed to true 10-17-03 Enables Aeyes DED Spacing fix for high resolutions
int g_nForceCockpitResolution = 0; // Wombat778 4-02-04 Simplified method of forcing the cockpit resolution.  Just put in 640, 800, 1024, 1280 or 1600. 0 Disables
int g_n3DHeadPanRange = 0; // Wombat778 2-21-04 split expandedheadrange into two separate variables 10-23-03 Increases the range of tilt in the 3d cockpit, and takes FOV into account
int g_n3DHeadTiltRange = 1; // Wombat778 2-21-04 split expandedheadrange into two separate variables 10-23-03 Increases the range of tilt in the 3d cockpit, and takes FOV into account
bool g_bReconLatLong = true; // Wombat778 11-3-03 Displays the Latitude and Longitude in the recon view
bool g_bRatioHack = true; // Wombat778 11-4-03 Enables the 1.25 ratio black box (should always be enabled but added option in case of a 1280x1024 native pit)
bool g_bPrecisionWaypoints = false; // Wombat778 11-5-03 Allow waypoints to be placed with greater precision.  Needs testing, so disabled by default.
bool g_bSmallerBullseye = false; // Wombat778 11-12-03 Enable a smaller, more realistic bullseye.  May be too small at 1024 resolution, so defaults to off
bool g_bRealisticMavTime = false; // JPG 7 Dec 03 - Enables realistic Maverick missile gyro spool up time of 3 minutes instead of 5 seconds
bool g_bnewAMRAAMdlz = false; // JPG 3 Apr 03 - Enables the new AMRAAM DLZ w/ Raero, Ropt, RPI, Fpole/Apole, etc
bool g_bMLUM2TAinHUD = false; // JPG 4 Oct 04 - Enables the MLU M2 tape feature of showing the (valid Air-to-air) target's altitude in the HUD (in the previous ALOW field below the RALT box), moves the AL #### box up above the altitude scale
float g_fHSDSymbolSize = 0.05f; // Wombat778 11-13-03 Allows manual setting of the HSD Symbol size. Default is 0.05f
bool g_bAutoScaleFonts = true; // Wombat778 12-10-03 When set to true, cockpit fonts will be automatically increased one step at 1600.
int g_n6DOFTIR = 0; // Cobra - 0 = TIR Yaw, Pitch and Roll  1 = TIR Yaw, Pitch, Roll, X, Y Z  2 = TIR Yaw, Pitch, Roll and FOV zoom
float g_fTIRMinimumFOV = 5.0f; // Cobra - Sets the minimum allowed TIR FOV (forward) to 5 degrees
float g_fTIRMaximumFOV = 100.0f; // Cobra - Sets the maximum allowed TIR FOV (back) to 100 degrees
float g_fMinimumFOV = 5.0f; // Wombat778 1-15-04 Sets the minimum allowed FOV to 5 degrees
float g_fMaximumFOV = 100.0f; // Wombat778 1-15-03 changed from 80.0 to 100.0 10-11-03 Sets the maximum allowed FOV to 80 degrees
float g_fDefaultFOV = 65.0f; // Wombat778 10-31-03 Sets the default FOV to 60 degrees -- Cobra - Changed to 65 for 3D pit default
float g_fFOVIncrement = 5.0f; // Wombat778 9-27-03 Sets FOV increment to 5 degrees
float g_fNarrowFOV = 20.0f; // Wombat778 2-20-04 Sets the FOV of the "look closer" command
float g_fWideviewFOV = 0.0f; // Wombat778 2-20-04 Sets the FOV of the wide view cockpit.  When 0, auto switching is disabled.
float g_fMeanTimeBetweenFailures = 0.0f;// Wombat778 2-24-04 Sets the MTBF. If nonzero, this will override the setting in the aircraft dat file.
bool g_bEnableRandomFailures = false; // Wombat778 2-24-04 Enables random failures.  If set to 1, the MeanTimeBetweenFailures value in either the ac.dat or g_fMeanTimeBetweenFailures will be used
bool g_b2DPitWingFOVFix = true; // Wombat778 2-25-04 Makes the 3d wings/ordinance in the 2d pit exempt from FOV changes. This prevents the wings from becoming "disembodied" from the plane
float g_fJDAMLift = 31.9f; // Wombat778 3-12-04 Sets the amount of "lift" that JSOW's have.  This will increase their range.
float g_fAIJSOWmaxRange = 40.0f; // Cobra - Sets the maximum JSOW range (NM).
bool g_bOldFontTexelFix = false; // Wombat778 4-01-04 Adds the old texel alignment font fix for cards that dont work with the proper method
int g_nShow2DPitErrors = 0; // Wombat778 3-23-04 When false, all error message boxes about 2d pit errors will be supressed.  When 1, only critical issues will be shown. When 2, all errors will be shown. Errors only show in windowed mode.
bool g_bFilter2DPit = true; // Wombat778 3-30-04 Enable filtering in the 2D cockpit.  Improves smoothness
int g_nNewFPSCounter = 1; // Wombat778 3-24-04 Use accurate FPS counter that counts number of frames per time unit. When 0, old FPS counter is used. Number is the number of updates per second. 1 is the most accurate
bool g_bCrackFix = false; // Wombat778 4-02-04 Reenable the "old" crack fix, as it may be useful for some people with older card at high aa and pit filtering
bool g_bResizeUsesResMgr = false; // Wombat778 4-14-04 Make the resizer ignore the resource manager (skip cpdata.zip) when looking for a pit to resize. This allows rescaling to 1024
bool g_bPadlockHudColor = true; // Wombat778 4-28-04 Enables the new padlock box color that Unz implemented
int g_nMiniDump = 0; // Wombat778 5-01-04 When greater than -1, minidumps called "dumplog.dmp" will be created on a crash. The number denotes the type of dump that will be created. If < 0, no dump will be made.
bool g_bCheckFeatureIndex = false; // Wombat778 5-15-04 When enabled, there is a check to ensure that feature indexes in the PHD data are within range
bool g_bMachAsiDial = false; // Wombat778 7-09-04 When enabled, the MachASI callback is treated as a dial instead of a MachASI object
bool g_bNew2DTrackIR = true; // Wombat778 11-15-04 Enables the new 2D pit TrackIR code
bool g_bSync2D3DPit = false; // Wombat778 11-17-04 Causes the view to remain rougly the same when switching between 2d and 3d pits.  A prerequisite for the hybrid pit mode
float g_fHybridPitThreshold1 = 10.0f; // Wombat778 11-18-04 Sets the number of degrees of movement required before a pit mode switch (in hybrid pit mode) from 2D->3D pit
float g_fHybridPitThreshold2 = 2.5f; // Wombat778 11-18-04 Sets the number of degrees of movement required before a pit mode switch (in hybrid pit mode) from 3D->2D pit
int g_nHybridPitModeDelay = 1000; // Wombat778 11-18-04 Sets the number of milliseconds of movement below the threshold before a switch from 3d to 2d pit (in hybrid pit mode)
float g_fEXPfactor = 0.5f; // 50% cursorspeed in EXP
float g_fDBS1factor = 0.75f; // 35% cursorspeed in DBS1
float g_fDBS2factor = 0.85f; // 20% cursorspeed in DBS2
float g_fePropFactor = 40.0f; // Mnvers.cpp - to control restricted speed (curMaxStoreSpeed) for AI
float g_fSunPadlockTimeout = 1.5f; // After how many seconds look on a padlocked object into the sun break lock
int g_nGroundAttackTime = 6; // how many minutes after SetupAGMode the AI will continue to do a ground attack
int g_nSeadAttackTime = 4; // how many minutes after SetupAGMode the AI will continue to do a Sead ground attack
int g_nStrikeAttackTime = 6; // how many minutes after SetupAGMode the AI will continue to do a Strike ground attack
int g_nCASAttackTime = 6; // how many minutes after SetupAGMode the AI will continue to do a CAS ground attack
int g_nAIshootLookShootTime = 45; // Cobra - how many seconds between Wingman AG attack messages
bool g_bAGTargetWPFix = false; // stop skipping of target WP because of departure time for AGMissions if several conditions are met
bool g_bAlwaysAnisotropic = false; // if true, the "Anisotropic" button in gfx setup is always on (workaround for GF3)
float g_fTgtDZFactor = 0.0F; // factor to reduce targetDZ when track has been lost - for fixing ballistic missiles
bool g_bNoAAAEventRecords = false; // don't record AAA shots at the player to event list
int g_nATCTaxiOrderFix = 0; // 1 = fixes player (09:36 takeoff) behind AI planes (09:37 takeoff)
bool g_bEmergencyJettisonFix = true; // just check not to drop AA weapons and ECM for all
float g_fDBS1ScanRateFactor = 0.25f; //JAM 13Oct03
float g_fDBS2ScanRateFactor = 0.05f; //JAM 13Oct03
bool g_bACMIRecordMsgOff = true; // JPG 10 Jan 04 Turns off the "RECORDING: +++" stuff when ACMI is on
bool g_bF4CommsMTU = false ;              // Unz MTU switch activation
//extern "C" bool g_bF4CommsKillPL = false ; // Booster and Unz switch to Kill packet loss routine
/* Retro TrackIR stuff.. */
int g_nTrackIRSampleFreq = 0x200; // Retro 02/10/03 - how fast a 2d screen can change using TIR
float g_fTIR2DPitchPercentage = 0.7f; // Retro 02/10/03
float g_fTIR2DYawPercentage = 0.7f; // Retro 02/10/03
/* ..ends */

/* Retro RadioSubTitle stuff */
int g_nNumberOfSubTitles = 10; // Retro 20Dec2003 - max number of simultaneously drawn messages
int g_nSubTitleTTL = 10000; // Retro 20Dec2003 - time a message will be displayed on the screen, in ms
char g_strRadioflightCol[0x40] = ""; // Retro 27Dec2003
char g_strRadiotoPackageCol[0x40] = ""; // Retro 27Dec2003
char g_strRadioToFromPackageCol[0x40] = ""; // Retro 27Dec2003
char g_strRadioTeamCol[0x40] = ""; // Retro 27Dec2003
char g_strRadioProximityCol[0x40] = ""; // Retro 27Dec2003
char g_strRadioWorldCol[0x40] = ""; // Retro 27Dec2003
char g_strRadioTowerCol[0x40] = ""; // Retro 27Dec2003
char g_strRadioStandardCol[0x40] = ""; // Retro 27Dec2003
/* ..ends */

// Retro 25Dec2003 for the DisplacementCam
bool g_bEnableDisplacementCam = false;

// Retro 17Feb2004
int g_nSaturationSmall = 9500; // 5% axis saturation
int g_nSaturationMedium = 9000; // 10% axis saturation
int g_nSaturationLarge = 8500; // 15% axis saturation
int g_nDeadzoneSmall = 100; // 1% deadzone
int g_nDeadzoneMedium = 500; // 5% deadzone
int g_nDeadzoneLarge = 1000; // 10% deadzone
int g_nDeadzoneHuge = 2000; // 20% deadzone
// these values are the actual sensitivity * 1000 (slider needs ints, but the axisvals are in rads)
int g_nMouseLookSensMax = 1700;
int g_nMouseLookSensMin = 200;
// these values are "1:1" (no factor or such)
int g_nMouseWheelSensMax = 10;
int g_nMouseWheelSensMin = 1;
int g_nKeyPOVSensMax = 180; // 180 degrees per second
int g_nKeyPOVSensMin = 30; // 30 degrees per second
// Retro 17Feb2004 end

bool g_bUseNewSmoothing = false; // Retro 20Feb2004 - if enabled uses a new smoothing algorithm that however does not work
//  for bank/pitch/yaw/throttle(1+2) yet
bool g_bDisplayAxisValues = false; // Retro 25Feb2004 - shows the values of all analogue axis, for debugging purposes only

bool g_bPilotEntertainment = false; // Retro 3Jan2004 - enable winamp interface
int g_nWinAmpInitVolume = 204; // Retro 3Jan2004 - inital playback volume (valid between 0 and 255)

int g_nBWMaxDeltaTime = 1; // true = use maximum value restriction, false = set 0 and return
int g_nBWCheckDeltaTime = 5000; // maximum value of delta_time in capi.c
int g_nVUMaxDeltaTime = 5000; // maximum value of delta_time in vuevent.cpp
bool g_bCampSavedMenuHack = true;
#ifdef _DEBUG
bool g_bActivateDebugStuff = false;
#else
bool g_bActivateDebugStuff = false; // to activate .label and .fuel chat line switch
#endif
bool g_bActivateMissileDebug = false; // FRB
float g_fMoverVrValue = 450.0f; // bogus Vr value in Radar.cpp - seems a bit too high - must test how change effects the AI
bool g_bEmptyFilenameFix = true; // fixes savings of "no name" files
float g_fRAPDistance = 3.0F; // used in MissileEngage() function to decide at which distance we start to roll and pull
bool g_bLabelRadialFix = true; // Fix for label display at screen edges
bool g_bLabelShowDistance = false; // If wanted, also show the distance to the target
bool g_bCheckForMode = true; // saw lead at takeoff asking wingman to do bvrengagement
bool g_bRQDFix = true; // Fix RQD C/S readout in ICP CRUS page
int g_nSessionTimeout = 30; // 30 seconds to timeout a disconnected session (might be a bit too high...)
int g_nSessionUpdateRate = 15; // 15 seconds session update
int g_nMaxInterceptDistance = 60; // only divert flights within 60 nm distance to the target
bool g_bNewSensorPrecision = true;
bool g_bSAM2D3DHandover = false; // 2D-3D target handover to SAMs doesn't really work this way - turn off
int g_nChooseBullseyeFix = 0; // theater fix for finding best bullseye position
/* 0x01 = use bullseye central position from campaign trigger files
   0x02 = change bullseye at each new day (should be tested before activated - what happens in flight, Multiplayer ?)
*/
int g_nSoundSwitchFix = 0x03;
/* 0x01 = activate "DoSoundSetup" in TheaterDef.cpp
   0x02 = add a fix in VM::ResetVoices - I think that garbage in this variable after sound switching
          can cause chatter messages not to be processed in PlayRadioMessage...*/

int g_nDFRegenerateFix = 0x03; // fix for DF regenerations
/* 0x01 = Fix in RegenerateMessage.cpp
 0x02 = Fix in CampUpd\Dogfight.cpp (not sure if this is really needed - but we let it in */
bool g_bAllowOverload = true;   // Allow takeoff even when overloaded - the player may decide...
bool g_bACPlayerCTDFix = true; // When a player CTD's, put aircraft back to host's AI control
bool g_bSetWaypointNumFix = false; // Older fix from S.G. in Navfcc.cpp - must still be tested as AI uses this function, too
float g_fLethalRadiusModifier = 1.5f; // used in 0x20 condition
int g_nMissileFix = 0x7f; // several missile fixes:
/*

 0x01 "Bomb missile" flag support -> do a ground/feature impact missile end instead
     of lethalradius detonation
 0x02 also check if range*range > lethalRadiusSqrd at "closestApprch" flag
 0x04 Use ArmingDelay MissileEndMessage instead of MinTime if warhead is
 not armed (fixes missiles being able to apply proximity damage while
 warhead is unarmed)
 0x08 Do Proximity damage to the missile's target if we didn't hit it directly
     (for example if missile lost seeker track and hits the dirt)
 0x10 Don't do the change to "NotDone" in Missmain.cpp when we have "closestApprch" flag already set
         hope this finally fixes floating missiles on the ground...
 0x20 bring missile to an end if missile sensor lost lock on target and we are outside 2 times
 of lethalrange to last known position
 0x40 Change targetDZ when target lost by g_fTgtDZFactor
 0x80 Use correct missile/bomb drop sound (+ flag 0x01)
 0x100   Fix for JDAM - have always cloud LOS if weapon flag 0x400 is set
*/

bool g_bDarkHudFix = true; // fix for the host player getting a dark HUD in TAKEOFF/TAXI mode

float g_fBombMissileAltitude = 13000.0f;// altitude at which "bomb-like" missiles are being released
int g_nFogRenderState = 0x01; // 0x01 turn on the D3D call m_pD3DD->SetRenderState in context.cpp
// 0x02 turns on StateStack::SetFog call to context->SetState
// which seems, according to comment, to be only some test code...
bool g_bTankerFMFix = true; // fix for tankers simple af flightmodel
//M.N.
bool g_bAppendToBriefingFile = false;
int g_nPrintToFile = 0x00; // MNLOOK set to 00 for release
/*  0x00 = print out
 0x01 = only print to file
    0x02 = print out and to file*/

int g_nGfxFix = 0x00; // turn all fixes off by default


/*
 0x01 activates 2D clipping fix
 0x02 activates ScatterPlot clipping fix
 0x04 activates 3D OTWSky clipping fix (sun, moon)
 0x08 does new clip code only for 2D drawables (removed, causes AG radar not to work anymore)
 */
bool g_bExitCampSelectFix = true;
bool g_bAGNoBVRWVR = false; // stops AG missions from doing any BVR/WVR checks
// Refuel debugging:
unsigned long gFuelState = 0;   // to set SimDriver.GetPlayerEntity()'s fuel by ".fuel" chat command

// DEBUG LABELING:

int g_nShowDebugLabels = 0; // give each label type a bit
int g_nMaxDebugLabel = 0x40000000; // 2002-04-01 MODIFIED BY S.G. Bumped up to highest without being negative

/*
 DEBUG LABELS:

 0x01 Aircraft current mode
 0x02 Missile mode (boost, sustaion, terminal) speed and altitude, Active state
 0x04 SAM radar modes
 0x08 BVR modes
 0x10 WVR modes
 0x20 Energy state modes
 0x40 add current radar mode (RWS, TWS, STT) to 0x08 and 0x10
 0x80 Aircraft Pstick, Rstick, Throttle, Pedal
 0x100 Reset all aircraft labels
 0x200 Radar Digi Emitting/Not Emitting
 0x400 SAM sensor track
 0x800 Refueling: Tanker speed/altitude, refueling aircraft speed, tankerdistance, fuel level
 0x1000 Aircraft Identified/NotIdentified
 0x2000 Fuel level of aircraft
 0x4000 IL78 totalrange value - for fixing IL78Factor in aircraft data files
 0x8000 add flight model (SIMPLE/COMPLEX) to labels 0x01, 0x08, 0x10, 0x20, 0x80
 0x10000 Mnvers.cpp - debugging of afterburner stuff
 0x20000 Refuel.cpp - Relative position to boom
 0x40000 Actions.cpp - show label if aborting in AirbaseCheck
 0x80000 landme.cpp - current taxi point and type
 0x100000 Show aircrafts speed, heading, altitude
 0x200000 Show trackpoint location of aircraft (trackX/trackY/trackZ)
 0x400000 Show player's wing current maneuver (wingactions.cpp)
 0x800000 Show aircrafts "self" pointer address
 0x1000000 Free to use
 0x2000000 Free to use
 0x4000000 Free to use
 0x8000000 Free to use
 0x10000000 Free to use
 0x20000000 Free to use
    0x40000000 Max debug (Resets all aircraft labels)
*/

// a.s. begin
bool g_bEnableMfdColors = true; // enables transparent and colored Mfds
float g_fMfdTransparency = 50; // set transparence of Mfds as a percentage value, e.g. 100 means no transparency (255), 80 means 20% transparency
float g_fMfdRed = 0; // set brightness of red as a percentage value for Mfds, e.g. 100 means brightness of 255
float g_fMfdGreen = 30; // set brightness of green as a percentage value for Mfds, e.g. 100 means brightness of 255
float g_fMfdBlue = 0; // set brightness of blue as a percentage value for Mfds, e.g. 100 means brightness of 255
bool g_bEnableMfdSize = true; // enables resizing of Mfds
float g_fMfd_p_Size = 90; // set size of Mfds as a percentage value of normal size (154)
// a.s. end
bool g_bMavFixes = true;  // a.s. New code for slewing MAVs.
bool g_bMavFix2 = false;  // MN When designating inside the 40 nm distance in BORE, and slewing outside, the HUD designation box got stuck
bool g_bLgbFixes = true;  // a.s. New code for slewing LGBs.
bool g_brebuildbobbleFix = true;
bool g_bMPFix = true;
bool g_bMPFix2 = true;
bool g_bMPFix3 = true;
bool g_bMPFix4 = true;
int  g_nMPPowerXmitThreshold = 1; // MLR 3/22/2004 - This is the threshold for sending the RPM value to other players, lower values are more fluid, but eat more bandwidth


float MinBwForOtherData = 1000.0f;
float g_fclientbwforupdatesmodifyerMAX = 0.8f;
float g_fclientbwforupdatesmodifyerMIN = 0.7f;
float g_fReliablemsgwaitMAX = 60000;

bool g_bDisableMissleEngGlow = true;  // MLR 2003-10-11 disable craptastic engine glow.
bool g_bSMSPylonLoadingFix   = false; // MLR 2003-10-16 enable the code that load 2 slot pylons CORRECTLY.
// disabled by default because it breaks existing models that work around the problem.
bool g_bWeaponStepToGun      = false;  // MLR 3/13/2004 - allow the gun to be removed from the normal weapon step
bool g_bEnableDopplerSound   = false; // MLR 2003-10-17
float g_fSoundDopplerFactor  = 1.0;   // MLR 2003-10-17
float g_fSoundRolloffFactor  = 1.0;   // MLR affects how sounds are attenuated.
//float g_fSoundDopplerBlend   = 30.0;  // MLR it's really dependent on framerate methinks // MLR 12/3/2003 - Obsolete
bool  g_bSoundDistanceEffect = false; // MLR simulate the effect of distance on some sounds.
int g_nSoundUpdateMS         = 50;    // MLR how many milliseconds must elapse before the sound code updates.
bool g_bNewEngineSounds      = false; // MLR new style engine sounds
bool g_bSoundHearVMSExternal = true;  // MLR enable/disable VMS sounds when in the external view.
// bool g_bSoundDynamicAllocate = false;  // MLR 2003-11-30 enable allocating sound buffers on the fly  // MLR 12/3/2003 - Not implemented yet

float g_fACMIAnimRecordTimer = .1f;  // MLR 3/2/2004 - seconds between recording a/c animation data

bool g_bHUDFix = true; //Smeghead 14-Oct-2003: HUD_Fixes.pdf changes on/off switch.


// TJL 10/24/03
bool g_bLargeStrike = false; // TJL 10/24/03 Allows for campaign to generate > 4 ship strike packages
bool g_bTakeoffSound = false; // TJL 10/26/03 Allows for removal of annoying takeoff.wav
bool g_bTurb = true;//TJL 03/14/04 Turbulence Code

// MLR 2/4/2004 - these are the defaults for when the ac.dat file does not contain the data
float g_fA2GJDAMAlt = 20000.0f; //TJL 10/27/03 Sets AI JDAM attack altitude
float g_fA2GJSOWAlt = 25000.0f; //TJL 10/27/03 Sets AI JSOW attack altitude
float g_fA2GHarmAlt = 20000.0f; //TJL 10/27/03 Sets AI HARM attack altitude (all set to SP3 defaults)
float g_fA2GAGMAlt = 4000.0f; //TJL 10/27/03 Sets AI AGM attack altitude
float g_fA2GGBUAlt = 13000.0f; //TJL 10/27/03 Sets AI GBU attack altitude
float g_fA2GDumbHDAlt = 250.0f; //TJL 10/27/03 Sets AI Durandal attack altitude
float g_fA2GClusterAlt = 5000.0f; //TJL 10/27/03 Sets AI Cluster Bomb attack altitude
float g_fA2GDumbLDAlt = 25000.0f; //TJL 10/27/03 Sets AI Generic attack altitude
float g_fA2GGenericBombAlt = 15000.0f; //TJL 10/27/03 Sets AI Generic Bomb attack altitude
float g_fA2GGunRocketAlt = 1000.0f; //TJL 10/27/03 Sets AI Gun and Rocket altitude
float g_fA2GCameraAlt = 7000.0f; //TJL 10/27/03 Sets AI BDA/Recon altitude

bool g_bScramble = true; // TJL 11/02/03 Enable Scramble missions
bool g_bhudAOA = true; // TJL 11/09/03 Enable HUD AOA indicator
bool g_bnoRadStutter = true; //TJL 11/25/03 Stops airplane 0.5 second stutter when in AG mode.
bool g_bRollInertia = true; //TJL 01/06/04 Do Roll Inertia code

bool g_bLensFlare = true; // THW 2003-11-10 Disable Lens Flare
bool g_bDisableHighFartiles = true; //THW 2003-11-14 Never let the fartiles kick in at high altitudes (default: 1)
float g_fTexDetailFactor = 2.0f; //THW 2003-11-14 Use Higher-Res textures at higher altitudes (BMS default: 2 / Original MPS: 1)
bool g_bBriefHTML = true; // THW 2003-12-07 Generate Briefing using HTML tags
//int g_nSeason = 0; //THW 2004-01-02 Select Season (0=Summer/default, 1= Autumn, 2=Winter)

// MD -- 20031221 - knob elevation control independent of mode and always followed for RWS/SAM and no-target TWS
bool g_bAntElevKnobFix = true;
// MD -- 20040209: enforce analog idle/cutoff value; affects startup and shutdown ops to be more
// like the real jet for players that have actual throttles with an idle/cutoff gate
bool g_bUseAnalogIdleCutoff = false;
// MD -- 20040209: a pad value to make sure joystick jitter/repeatability problems don't shutoff your engine by accident
int g_nIdleCutoffPad = 500;
// MD -- 20040613: enable per weapon pickle hold to release timers
bool g_bWeaponPickleDelays = true;
// MD -- 20040727: enable HSD to always draw steerpoint course line regardless of NAV mode/submode
bool g_bHsdStptFix = true;

bool g_bUnlimitedAmmo = false;//Cobra name says it all ;)
bool g_bUseNew3dpit = false; //ATARIBABY Use new 3dpit code - needs new 3d pit model
bool g_bStartIn3Dpit = false;  // Cobra - start in 3D cockpit
//bool g_bUse6DOFTir = false; // Retro 24Dez2004
float g_f3DHeadTilt = 15.0f; //Cobra - Head tilt when entering the 3D cockpit
float g_f3DPitFOV = 60.0f; //Cobra - FOV when entering the 3D cockpit

float g_fButtonScaler = 56.9f;
float g_fButtonZScaler = 56.9f;

//ATARIBABY Externalised parameters and multipliers for 3D pit dynamic head movement
float g_fDyn_Head_TiltMul = 1.0f; //Controls how much pitch change influence head tilt
float g_fDyn_Head_TiltRndGMul = 1.0f; //Controls random addition to tilt under G's
float g_fDyn_Head_RollMul = 1.0f; //Controls how much roll change influence head roll
float g_fDyn_Head_PanMul = 1.0f; //Controls how much yaw change influence head pan
float g_fDyn_Head_TiltRateMul = 1.0f; //Controls speed of head tilt change to new pos
float g_fDyn_Head_TiltGRateMul = 0.5; //Controls influence of Gs on speed of head tilt change to new pos
float g_fDyn_Head_RollRate = 2.0f; //Controls speed of head roll change to new pos
float g_fDyn_Head_PanRate = 1.0f; //Controls speed of head pan change to new pos

float g_fdpitStart = 0600; //Cobra TJL 11/08/04
float g_fdpitStop = 1800; //Cobra TJL 11/08/04
float g_fTaxiEarly = 75.0f; // RAS - Early taxi time in seconds  - Cobra - changed from 100 to 75


// COBRA - Red - NEW DX ENGINE
bool g_bUse_DX_Engine = true;

static ConfigOption<bool> BoolOpts[] =
{
    // { "EnableCATIIIExtension", &g_bEnableCATIIIExtension }, MI
    { "UseDxEngine", &g_bUse_DX_Engine },
    { "ForceDXMultiThreadedCoopLevel", &g_bForceDXMultiThreadedCoopLevel },
    { "EnableABRelocation", &g_bEnableABRelocation },
    { "EnableWeatherExtensions", &g_bEnableWeatherExtensions },
    { "EnableWindsAloft", &g_bEnableWindsAloft },
    { "EnableNonPersistentTextures", &g_bEnableNonPersistentTextures },
    { "EnableStaticTerrainTextures", &g_bEnableStaticTerrainTextures },
    // { "EnableAircraftLimits", &g_bEnableAircraftLimits }, MI
    // { "EnableArmingDelay", &g_bArmingDelay }, MI
    // { "EnableHardCoreReal", &g_bHardCoreReal }, MI
    { "CheckBltStatusBeforeFlip", &g_bCheckBltStatusBeforeFlip },
    { "EnableUplink", &g_bEnableUplink },
    { "EnableColorMfd", &g_bEnableColorMfd },
    { "NewDamageEffects", &g_bNewDamageEffects },
    { "DisableFunkyChicken", &g_bDisableFunkyChicken },
    //JAM 01Dec03 { "ForceSoftwareGUI", &g_bForceSoftwareGUI },
    { "SmartScaling", &g_bSmartScaling },
    { "FloatingBullseye", &g_bFloatingBullseye },
    { "DisableCrashEjectCourtMartials", &g_bDisableCrashEjectCourtMartials },
    //JAM 20Sep03  { "UseMipMaps", &g_bUseMipMaps },
    { "ShowMipUsage", &g_bShowMipUsage },
    { "SmartCombatAP", &g_bSmartCombatAP },
    { "NoRPMOnHUD", &g_bNoRPMOnHud },
    { "CATIIIDefault", &g_bCATIIIDefault },
    // { "RealisticAvionics", &g_bRealisticAvionics },
    { "EPAFRadarCues", &g_bEPAFRadarCues},
    { "RadarJamChevrons", &g_bRadarJamChevrons},
    { "AWACSSupport", &g_bAWACSSupport},
    // { "Voodoo12Compatible", &g_bVoodoo12Compatible},
    { "AWACSRequired", &g_bAWACSRequired},
    { "Use3dSound", &g_bUse3dSound}, // MLR 2003-11-17 Going to hardcode this on // MLR 12/7/2003 - renabled
    { "OldSoundAlg", &g_bOldSoundAlg}, // MLR 2003-11-17 Going to hardcode this on // MLR 12/7/2003 - renabled
    { "MultiEngineSound", &g_bMultiEngineSound}, // MLR 5/18/2004 -
    { "MFDHighContrast", &g_bMFDHighContrast},
    { "IFlyMirage", &g_bIFlyMirage},
    { "PowerGrid", &g_bPowerGrid},
    { "UseMappedFiles", &g_bUseMappedFiles },
    // { "UserRadioVoice", &g_bUserRadioVoice },
    { "NewFm", &g_bNewFm },
    { "RealisticAttrition", &g_bRealisticAttrition },
    { "GreyScaleMFD", &g_bGreyScaleMFD },
    { "IFFRWR", &g_bIFFRWR },
    { "3dCockpit", &g_b3dCockpit },
    { "RWR", &g_bRWR },
    { "BrightHUD", &g_bBrightHUD },

#if 0 // JPO
    { "3dHUD", &g_b3dHUD },
    { "3dRWR", &g_b3dRWR },
    { "3dICP", &g_b3dICP },
    { "3dDials", &g_b3dDials },
#endif
    { "3dMFDLeft", &g_b3dMFDLeft },
    { "3dMFDRight", &g_b3dMFDRight },
    { "3dDynamicPilotHead", &g_b3dDynamicPilotHead },
    { "SimpleFMUpdates", &g_bSimpleFMUpdates },
    { "NoMFDsIn1View", &g_bNoMFDsIn1View},
    { "Server", &g_bServer},
    { "ServerHostAll", &g_bServerHostAll},
    { "LogEvents", &g_bLogEvents},
    { "VoiceCom", &g_bVoiceCom},
    //#if 0
    { "woeir", &g_bwoeir},
    { "MLU", &g_bMLU},
    { "IFF", &g_bIFF}, //MI disabled until further notice
    //#endif
    { "INS", &g_bINS},
    { "RP5DataCompatiblity", &g_bRP5Comp },
    { "ModuleList", &g_bModuleList },
    { "HiResUI", &g_bHiResUI},
    { "AWACSFuel", &g_bAWACSFuel},
    // { "ShowManeuverLabels", &g_bShowManeuverLabels},
    { "FullScreenNVG", &g_bFullScreenNVG},
    { "LogUiErrors", &g_bLogUiErrors},
    { "LoadoutSquadStoreResupply", &g_bLoadoutSquadStoreResupply},
    { "DisplayTrees", &g_bDisplayTrees},
    { "RequestHelp", &g_bRequestHelp},
    { "LightsKC135", &g_bLightsKC135},
    { "AddACSizeVisual", &g_bAddACSizeVisual},
    // { "ShowFuelLabel", &g_bShowFuelLabel},
    { "NewRackData", &g_bNewRackData},
    { "BMSRackData", &g_bBMSRackData},
    { "HelosReloc", &g_bHelosReloc},
    { "ShowFlaps", &g_bShowFlaps},
    { "SlowButSafe", &g_bSlowButSafe},
    { "NewPadlock", &g_bNewPadlock},
    { "NoPadlockBoxes", &g_bNoPadlockBoxes},
    { "PitchLimiterForAI", &g_bPitchLimiterForAI},
    { "MissionACIcons", &g_bMissionACIcons},
    { "EnableMfdColors", &g_bEnableMfdColors}, // a.s.
    { "EnableMfdSize", &g_bEnableMfdSize}, // a.s.
    { "AIRefuelInComplexAF", &g_bAIRefuelInComplexAF}, // 2002-02-20 S.G.
    { "NewAcmiHud", &g_bNewAcmiHud},
    { "AWACSBackground", &g_bAWACSBackground},
    { "MavFixes", &g_bMavFixes},  // a.s.
    { "LgbFixes", &g_bLgbFixes},  // a.s.
    { "FallingHeadingTape", &g_bFallingHeadingTape},
    { "NewRefuelHelp", &g_bNewRefuelHelp}, // MN
    { "OtherGroundCheck", &g_bOtherGroundCheck }, // MN
    { "Limit2DRadarFight", &g_bLimit2DRadarFight}, // 2002-03-07 S.G.
    { "AdvancedGroundChooseWeapon", &g_bAdvancedGroundChooseWeapon}, // 2002-03-09 S.G.
    { "TFRFixes", &g_bTFRFixes}, //MI TFR Fixes
    { "AIGloc", &g_bAIGloc }, // MN
    { "CalibrateTFR_PitchCtrl", &g_bCalibrateTFR_PitchCtrl},
    { "LantDebug", &g_bLantDebug},
    { "UseNewCanEnage", &g_bUseNewCanEnage}, // 2002-03-11 S.G.
    { "MPStartRestricted", &g_bMPStartRestricted},
    { "UseSkillForFlaks", &g_bUseSkillForFlaks}, // 2002-03-12 S.G.
    { "ToggleAAAGunFlag", &g_bToggleAAAGunFlag}, // 2002-03-12 S.G.
    { "UseTankerTrack", &g_bUseTankerTrack},// 2002-03-13 MN
    { "UseComplexBVRForPlayerAI", &g_bUseComplexBVRForPlayerAI}, // 2002-03-13 S.G.
    { "FuelUseVtDot", &g_bFuelUseVtDot }, // 2002-03-14 S.G.
    { "FuelLimitBecauseVtDot", &g_bFuelLimitBecauseVtDot }, // 2002-03-14 S.G.
    { "UseAggresiveIncompleteA2G", &g_bUseAggresiveIncompleteA2G }, // 2002-03-22 S.G.
    { "NewPitchLadder", &g_bNewPitchLadder},
    { "AddIngressWP", &g_bAddIngressWP}, // 2002-03-25 MN
    { "TankerWaypoints", &g_bTankerWaypoints}, // 2002-03-25 MN
    { "PutAIToBoom", &g_bPutAIToBoom}, // 2002-03-28 MN
    { "OldStackDump", &g_bOldStackDump}, // 2002-04-01 S.G.
    { "TankerFMFix", &g_bTankerFMFix}, // 2002-04-02 MN
    { "AGRadarFixes", &g_bAGRadarFixes}, //MI 2002-03-28
    { "LookCloserFix", &g_bLookCloserFix}, // 2002-04-05 MN by dpc
    { "AnyWaypointTask", &g_bAnyWaypointTask}, // Wombat778 09-27-2003
    { "FFCenterFix", &g_bFFCenterFix}, // Wombat778 09-29-2003
    { "CockpitAutoScale", &g_bCockpitAutoScale}, // Wombat778 10-06-2003
    { "3DClickableCockpitDebug", &g_b3DClickableCockpitDebug}, // Wombat778 10-10-2003
    { "3DRTTCockpitDebug", &g_b3DRTTCockpitDebug}, // Wombat778 10-10-2003
    { "3DClickableCursorChange", &g_b3DClickableCursorChange}, // Wombat778 10-15-2003
    { "DEDSpacingFix", &g_bDEDSpacingFix}, // Wombat778 10-17-2003
    { "ReconLatLong", &g_bReconLatLong}, // Wombat778 11-3-2003
    { "RatioHack", &g_bRatioHack}, // Wombat778 11-4-2003
    { "PrecisionWaypoints", &g_bPrecisionWaypoints}, // Wombat778 11-5-2003
    { "SmallerBullseye", &g_bSmallerBullseye}, // Wombat778 11-12-2003
    { "AutoScaleFonts", &g_bAutoScaleFonts}, // Wombat778 12-11-2003
    { "EnableRandomFailures", &g_bEnableRandomFailures}, // Wombat778 12-11-2003
    { "2DPitWingFOVFix", &g_b2DPitWingFOVFix}, // Wombat778 2-25-2003
    { "OldFontTexelFix", &g_bOldFontTexelFix}, // Wombat778 4-01-04
    { "Filter2DPit", &g_bFilter2DPit}, // Wombat778 3-30-2004
    { "CrackFix", &g_bCrackFix}, // Wombat778 4-02-2004
    { "ResizeUsesResMgr", &g_bResizeUsesResMgr}, // Wombat778 4-14-2004
    { "PadlockHudColor", &g_bPadlockHudColor}, // Wombat778 4-28-2004
    { "CheckFeatureIndex", &g_bCheckFeatureIndex}, // Wombat778 5-15-2004
    { "MachAsiDial", &g_bMachAsiDial}, // Wombat778 7-9-2004
    { "New2DTrackIR", &g_bNew2DTrackIR}, // Wombat778 11-15-2004
    { "Sync2D3DPit", &g_bSync2D3DPit}, // Wombat778 11-17-2004
    { "LowBwVoice", &g_bLowBwVoice},
    { "AGTargetWPFix", &g_bAGTargetWPFix}, // 2002-04-06 MN
    { "BombNumLoopOnly", &g_bBombNumLoopOnly}, //MI 2002-07-04 fix for missing bombs in CCIP
    { "AlwaysAnisotropic", &g_bAlwaysAnisotropic }, // 2002-04-07 MN
    { "NoAAAEventRecords", &g_bNoAAAEventRecords }, // 2002-04-07 MN
    { "ActivateDebugStuff", &g_bActivateDebugStuff }, // 2002-04-12 MN
    { "ActivateMissileDebug", &g_bActivateMissileDebug }, // FRB
    { "NewSensorPrecision", &g_bNewSensorPrecision },
    { "AGNoBVRWVR", &g_bAGNoBVRWVR },
    { "TO_LDG_LightFix", &g_bTO_LDG_LightFix}, //MI 2002-04-13
    { "AppendToBriefingFile", &g_bAppendToBriefingFile},
    { "DarkHudFix", &g_bDarkHudFix},
    { "RQDFix", &g_bRQDFix}, // MN 2002-04-13
    { "ACPlayerCTDFix", &g_bACPlayerCTDFix},
    { "CheckForMode", &g_bCheckForMode}, // MN 2002-04-14
    { "AllowOverload", &g_bAllowOverload},
    { "UseDefinedGunDomain", &g_bUseDefinedGunDomain}, // 2002-04-17 S.G.
    { "LabelRadialFix", &g_bLabelRadialFix},
    { "LabelShowDistance", &g_bLabelShowDistance},
    { "SetWaypointNumFix", &g_bSetWaypointNumFix}, // 2002-04-18 MN a fix from S.G. which he didn't put in because it was not really tested yet..
    { "EmptyFilenameFix", &g_bEmptyFilenameFix},
    { "RebuildbobbleFix", &g_brebuildbobbleFix},
    { "MPFix", &g_bMPFix},
    { "MPFix2", &g_bMPFix2},
    { "MPFix3", &g_bMPFix3},
    { "MPFix4", &g_bMPFix4},
    { "ExitCampSelectFix", &g_bExitCampSelectFix},
    { "CampSavedMenuHack", &g_bCampSavedMenuHack},
    { "EmergencyJettisonFix", &g_bEmergencyJettisonFix},
    { "OldSamActivity", &g_bOldSamActivity}, // no LOS check and stuff - just the old code
    { "FireOntheMove", &g_bFireOntheMove},
    { "SAM2D3DHandover", &g_bSAM2D3DHandover},
    { "MavFix2", &g_bMavFix2},
    { "DisableMissileEngGlow", &g_bDisableMissleEngGlow}, // MLR 2003/10/11 - get rid of that ugly star
    { "HUDFix", &g_bHUDFix}, //Smeghead 14-Oct-2003
    { "SMSPylonLoadingFix", &g_bSMSPylonLoadingFix}, // MLR 2003/10/16

    // MLR 12/14/2003 -  All these are control in the UI setup now
    //{ "EnableDopplerSound", &g_bEnableDopplerSound}, // MLR 2003/10/17
    //{ "NewEngineSounds" , &g_bNewEngineSounds},      // MLR 2003/11/21
    //{ "SoundHearVMSExternal", &g_bSoundHearVMSExternal}, // MLR 2003/11/23
    //{ "SoundDistanceEffect", &g_bSoundDistanceEffect}, // MLR 2003/11/02

    // { "SoundDynamicAllocate", &g_bSoundDynamicAllocate}, // MLR 11/30/2003 - Allocate DSOund buffers on the fly // MLR 12/3/2003 - Not implemented yet
    { "LargeStrike", &g_bLargeStrike}, //TJL 10/24/03 Enables Campaign to generate large strike packages >4 aircraft
    { "TakeoffSound", &g_bTakeoffSound}, // TJL 10/26/03 Enables Takeoff.wav in Camp UI. Defaults to off.
    { "Scramble", &g_bScramble}, //TJL 11/02/03 Enable Scramble missions
    { "hudAOA" , &g_bhudAOA}, //TJL 11/09/03 Enable HUD AOA indicator
    { "LensFlare", &g_bLensFlare}, //THW 2003-11-10 Toggle Lens Flare
    { "DisableHighFartiles", &g_bDisableHighFartiles}, //THW 2003-11-14 Never let the fartiles kick in at high altitudes (default: 1)
    { "noRadStutter", &g_bnoRadStutter}, //TJL 11/25/03 Stops 0.5 second stutter on aircraft when in A/G radar.
    { "RollLinkedNWSRudder", &g_bRollLinkedNWSRudder }, // ASSOCIATOR 30/11/03 Controls whether the Rudder and NWS are linked to the roll input when on the ground
    { "EnableGRCursorBullseye", &g_bEnableGRCursorBullseye }, // ASSOCIATOR 03/12/03: Enables Bullseye cursor in Ground Radar modes
    { "BriefHTML", &g_bBriefHTML }, // THW 2003-12-07 Generate Briefing using HTML tags
    // { "EnableFCCSubNavCycle", &g_bEnableFCCSubNavCycle },  // ASSOCIATOR 04/12/03: Enables you to cycle the Nav steerpoint modes modes with the FCC submodes key
    { "RealisticMavTime", &g_bRealisticMavTime }, // JPG 7 Dec 03 - Enables realistic Maverick missile gyro spool up time of 3 minutes instead of 5 seconds
    { "newAMRAAMdlz", &g_bnewAMRAAMdlz }, // JPG 2 Apr 04 - Enables new AMRAAM DLZ - see above
    { "EnableDisplacementCam", &g_bEnableDisplacementCam }, // Retro 25Dec2003
    { "AntElevKnobFix", &g_bAntElevKnobFix }, // MD -- 20031231: see above
    { "PilotEntertainment", &g_bPilotEntertainment },// Retro 3Jan2004
    { "RollInertia", &g_bRollInertia }, //TJL 01/06/04 Roll Inertia
    { "ACMIRecordMsgOff", &g_bACMIRecordMsgOff }, // JPG 10 Jan 04 Turns off the ACMI RECORDING msg
    { "UseAnalogIdleCutoff", &g_bUseAnalogIdleCutoff }, // MD -- 20040209: see above
    { "WeaponPickleDelays", &g_bWeaponPickleDelays}, // MD -- 20040613: see above
    { "HsdStptFix", &g_bHsdStptFix}, // MD -- 20040727: see above
    { "F4CommsMTU", &g_bF4CommsMTU },     //  Unz-- MTU swtich with Wombats help
    // { "F4CommsKillPL", &g_bF4CommsKillPL },     //  Booster and Unz to Kill packet loss
    { "UseNew3dpit", &g_bUseNew3dpit}, //ATARIBABY Use new 3dpit code - needs new 3d pit model
    { "StartIn3Dpit", &g_bStartIn3Dpit},  // Cobra - start in 3D cockpit
    { "UseNewSmoothing", &g_bUseNewSmoothing}, // Retro 21Feb2004, look above for expl.
    { "DisplayAxisValues", &g_bDisplayAxisValues}, // Retro 25Feb2004
    { "WeaponLaunchUsesDrawPointerPos", &g_bWeaponLaunchUsesDrawPointerPos},  // MLR 2/19/2004 -
    { "UseNewSmoothing", &g_bUseNewSmoothing}, // Retro 21Feb2004, look above for expl.
    { "DisplayAxisValues", &g_bDisplayAxisValues}, // Retro 25Feb2004
    { "WeaponStepToGun", &g_bWeaponStepToGun},  // MLR 3/13/2004 - See above
    { "Turb", &g_bTurb}, //TJL 03/14/04 Turbulence Code
    { "SoundSonicBoom", &g_bSoundSonicBoom},
    { "DisableCommsBorder", &g_bDisableCommsBorder}, // ASSO: disable the radio comms menu border
    { "EcmOnHud", &g_bEcmOnHud}, // ASSO:
    { "BrakeOnHud", &g_bBrakeOnHud}, // ASSO:
    { "GearOnHud", &g_bGearOnHud}, // ASSO:
    { "WakeTurbulence", &g_bWakeTurbulence },
    { "DrawWakeTurbulence", &g_bDrawWakeTurbulence },
    { "OldDustTrail", &g_bOldDustTrail}, // Cobra - Use old dust trail sfx
    { "HearThunder", &g_bHearThunder}, // Cobra - Play thunder.wav
    { "HighSFX", &g_bHighSFX}, // Cobra - Switch internal PS effects levels
    { "AllHaveIFF", &g_bAllHaveIFF}, // Cobra - Give all a/c IFF interrogator
    { "UseRC135", &g_bUseRC135}, // Cobra = FRB - Use the RC-135 for ELINT (radar) ID'ing
    { "FFDBC", &g_bFFDBC},
    { "ExtViewOnGround", &g_bExtViewOnGround },// RAS -5Dec04- allow ext view on ground
    { "UnlimitedAmmo", &g_bUnlimitedAmmo }, //Cobra
    { "AnimPilotHead", &g_bAnimPilotHead}, // Cobra - Animate the pilot's head
    { NULL, NULL }
};

static ConfigOption<int> IntOpts[] =
{
    { "ThrottleMode", &g_nThrottleMode },
    { "PadlockBoxSize", &g_nPadlockBoxSize },
    { "PadlockMode", &g_nPadlockMode },
    { "NumDefaultHatSwitches", &NumHats },
    { "NearLabelLimit", &g_nNearLabelLimit },
    { "percentage_available_aircraft", &g_npercentage_available_aircraft },
    { "minimum_available_aircraft", &g_nminimum_available_aircraft },
    { "MasterServerPort", &g_nMasterServerPort },
    { "MaxVertexSpace", &g_nMaxVertexSpace },
    // { "MinTacanChannel", &g_nMinTacanChannel}, -> Theater definition file
    { "FlightVisualBonus", &g_nFlightVisualBonus},
    { "RelocationWait", &g_nRelocationWait},
    { "LoadoutTimeLimit", &g_nLoadoutTimeLimit},
    { "Year", &g_nYear},
    { "Day", &g_nDay},
    { "AirbaseReloc", &g_nAirbaseReloc},
    { "NoPlayerPlay", &g_nNoPlayerPlay},
    { "DeagTimer", &g_nDeagTimer},
    { "ReagTimer", &g_nReagTimer},
    { "RNESpeed", &g_nRNESpeed},
    { "TargetSpotTimeout", &g_nTargetSpotTimeout},
    { "MaxUIRefresh", &g_nMaxUIRefresh}, // 2002-02-23 S.G.
    { "UnidentifiedInUI", &g_nUnidentifiedInUI}, // 2002-02-24 S.G.
    { "lookAroundWaterTiles", &g_nlookAroundWaterTiles},
    { "FFEffectAutoCenter", &g_nFFEffectAutoCenter},
    { "MPStartTime", &g_nMPStartTime},
    { "LowestSkillForGCI", &g_nLowestSkillForGCI}, // 2002-03-12 S.G.
    { "AIVisualRetentionTime", &g_nAIVisualRetentionTime}, // 2002-03-12 S.G.
    { "AIVisualRetentionSkill", &g_nAIVisualRetentionSkill}, // 2002-03-12 S.G.
    { "MaxSimTimeAcceleration", &g_nMaxSimTimeAcceleration},
    // { "ShowDebugLabels", &g_nShowDebugLabels},// only by .label chatline input
    { "LowDetailFactor", &g_nLowDetailFactor},
    { "NoWPRefuelNeeded", &g_nNoWPRefuelNeeded}, // 2002-03-25 MN
    { "AirbaseCheck", &g_nAirbaseCheck}, // 2002-03-11 MN
    { "FogRenderState", &g_nFogRenderState},
    { "MissileFix", &g_nMissileFix}, // 2002-03-28 MN
    { "SkipWaypointTime", &g_nSkipWaypointTime}, // 2002-04-05 MN
    { "GroundAttackTime", &g_nGroundAttackTime}, // Cobra
    { "SeadAttackTime", &g_nSeadAttackTime}, // Cobra
    { "StrikeAttackTime", &g_nStrikeAttackTime}, // Cobra
    { "CASAttackTime", &g_nCASAttackTime}, // Cobra
    { "AIshootLookShootTime", &g_nAIshootLookShootTime}, // Cobra
    { "GfxFix", &g_nGfxFix}, // 2002-04-06 MN
    { "ATCTaxiOrderFix", &g_nATCTaxiOrderFix},// 2002-04-08 MN
    { "DFRegenerateFix", &g_nDFRegenerateFix},// 2002-04-09 MN
    { "BWMaxDeltaTime", &g_nBWMaxDeltaTime}, // 2002-04-12 MN
    { "BWCheckDeltaTime", &g_nBWCheckDeltaTime}, // 2002-04-12 MN
    { "VUMaxDeltaTime", &g_nVUMaxDeltaTime}, // 2002-04-12 MN
    { "ACMIOptionsPopupHiResX", &g_nACMIOptionsPopupHiResX},
    { "ACMIOptionsPopupHiResY", &g_nACMIOptionsPopupHiResY},
    { "ACMIOptionsPopupLowResX", &g_nACMIOptionsPopupLowResX},
    { "ACMIOptionsPopupLowResY", &g_nACMIOptionsPopupLowResY},
    { "ChooseBullseyeFix", &g_nChooseBullseyeFix }, // 2002-04-12 MN
    { "SoundSwitchFix", &g_nSoundSwitchFix},
    { "PrintToFile", &g_nPrintToFile},
    { "SessionTimeout", &g_nSessionTimeout},
    { "SessionUpdateRate", &g_nSessionUpdateRate},
    { "MaxInterceptDistance", &g_nMaxInterceptDistance}, // 2002-04-14 MN
    { "TrackIRSampleFreq", &g_nTrackIRSampleFreq}, // Retro 02/10/03
    { "SoundUpdatems", &g_nSoundUpdateMS }, // MLR 03/11/03
    { "NumberOfSubTitles", &g_nNumberOfSubTitles }, // Retro 20Dec2003
    { "SubTitleTTL", &g_nSubTitleTTL }, // Retro 20Dec2003
    // { "Season", &g_nSeason}, //THW 2004-01-02 Select Season (0=Summer/default, 1= Autumn, 2=Winter)
    { "WinAmpInitVolume", &g_nWinAmpInitVolume }, // Retro 3Jan2004
    { "DynamicVoices", &g_nDynamicVoices},
    { "IdleCutoffPad", &g_nIdleCutoffPad }, // MD -- 20040209: see above
    { "SaturationSmall", &g_nSaturationSmall}, // Retro 17Feb2004
    { "SaturationMedium", &g_nSaturationMedium}, // Retro 17Feb2004
    { "SaturationLarge", &g_nSaturationLarge}, // Retro 17Feb2004
    { "DeadzoneSmall", &g_nDeadzoneSmall}, // Retro 17Feb2004
    { "DeadzoneMedium", &g_nDeadzoneMedium}, // Retro 17Feb2004
    { "DeadzoneLarge", &g_nDeadzoneLarge}, // Retro 17Feb2004
    { "DeadzoneHuge", &g_nDeadzoneHuge}, // Retro 17Feb2004
    { "MouseLookSensMax", &g_nMouseLookSensMax}, // Retro 17Feb2004
    { "MouseLookSensMin", &g_nMouseLookSensMin}, // Retro 17Feb2004
    { "MouseWheelSensMax", &g_nMouseWheelSensMax}, // Retro 17Feb2004
    { "MouseWheelSensMin", &g_nMouseWheelSensMin}, // Retro 17Feb2004
    { "KeyPOVSensMax", &g_nKeyPOVSensMax}, // Retro 17Feb2004
    { "KeyPOVSensMin", &g_nKeyPOVSensMin}, // Retro 17Feb2004
    { "3DHeadPanRange", &g_n3DHeadPanRange}, // Wombat778 10-23-2003 split expandedheadrange
    { "3DHeadTiltRange", &g_n3DHeadTiltRange}, // Wombat778 10-23-2003 split expandedheadrange
    { "6DOFTIR", &g_n6DOFTIR}, // Cobra - 0 = TIR Yaw, Pitch and Roll  1 = TIR Yaw, Pitch, Roll, X, Y Z  2 = TIR Yaw, Pitch, Roll and FOV zoom
    { "NewFPSCounter", &g_nNewFPSCounter}, // Wombat778 3-24-2004
    { "ForceCockpitResolution", &g_nForceCockpitResolution}, // Wombat778 4-02-04
    { "Show2DPitErrors", &g_nShow2DPitErrors}, // Wombat778 3-22-2004
    { "MiniDump", &g_nMiniDump}, // Wombat778 5-01-2004
    { "HybridPitModeDelay", &g_nHybridPitModeDelay}, // Wombat778 5-01-2004
    { "MPPowerXmitThreshold", &g_nMPPowerXmitThreshold},  // MLR 3/22/2004 - see declaration
    { "TaxiLaunchTime", &g_nTaxiLaunchTime}, // Booster 2004/10/12 Taxi takeoff time option
    { "ChatterInterval", &g_nChatterInterval}, // FRB - chatter noise control
    { "SfxLODCutoff", &g_nSfxLODCutoff}, // Cobra - SFX display limits
    { "SfxLODDistCutoff", &g_nSfxLODDistCutoff}, // Cobra - SFX display limits
    { "SfxLODTotCutoff", &g_nSfxLODTotCutoff}, // Cobra - SFX display limits
    { "PSPurgeInterval", &g_nPSPurgeInterval}, // Cobra - PS list purge interval
    { "PSKillFPS", &g_nPSKillFPS}, //Cobra
    { "CriticalPullup", &g_nCriticalPullup}, // Cobra - AI ground avoidance full pullup theshold
    { NULL, NULL }
};

static ConfigOption<char> StringOpts[] =
{
    { "MasterServerName", &g_strMasterServerName[0] },
    { "ServerName", &g_strServerName[0] },
    { "ServerLocation", &g_strServerLocation[0] },
    { "ServerAdmin", &g_strServerAdmin[0] },
    { "ServerAdminEmail", &g_strServerAdminEmail[0] },
    { "VoiceHostIP", &g_strVoiceHostIP[0] },
    { "WorldName", &g_strWorldName[0] },
    { "ScrollUpFunction", &g_strScrollUpFunction[0] }, //Wombat778 10-07-2003
    { "ScrollDownFunction", &g_strScrollDownFunction[0]}, //Wombat778 10-07-2003
    { "MiddleButtonFunction", &g_strMiddleButtonFunction[0] }, //Wombat778 10-07-2003
    { "RadioflightCol", &g_strRadioflightCol[0] }, // Retro 27Dec2003
    { "RadiotoPackageCol", &g_strRadiotoPackageCol[0] }, // Retro 27Dec2003
    { "RadioToFromPackageCol", &g_strRadioToFromPackageCol[0] }, // Retro 27Dec2003
    { "RadioTeamCol", &g_strRadioTeamCol[0] }, // Retro 27Dec2003
    { "RadioProximityCol", &g_strRadioProximityCol[0] }, // Retro 27Dec2003
    { "RadioWorldCol", &g_strRadioWorldCol[0] }, // Retro 27Dec2003
    { "RadioTowerCol", &g_strRadioTowerCol[0] }, // Retro 27Dec2003
    { "RadioStandardCol", &g_strRadioStandardCol[0] }, // Retro 27Dec2003
    { NULL, NULL }
};

static ConfigOption<float> FloatOpts[] =
{
    { "MipLodBias", &g_fMipLodBias },
    { "CloudMinHeight", &g_fCloudMinHeight}, // JPO
    { "RadarScale", &g_fRadarScale}, // JPO
    { "CursorSpeed", &g_fCursorSpeed}, // JPO
    { "MinCloudWeather",  &g_fMinCloudWeather}, //JPO
    { "CloudThicknessFactor", &g_fCloudThicknessFactor}, //JPO
    { "DragDilutionFactor", &g_fDragDilutionFactor},
    // { "Latitude", &g_fLatitude}, // is now set by the theater.map readout
    { "dwPorthost", &g_fdwPorthost},
    { "dwPortclient", &g_fdwPortclient},
#if 0
    {"3dlMFDulx", &g_f3dlMFDulx},
    {"3dlMFDuly", &g_f3dlMFDuly},
    {"3dlMFDulz", &g_f3dlMFDulz},
    {"3dlMFDurx", &g_f3dlMFDurx},
    {"3dlMFDury", &g_f3dlMFDury},
    {"3dlMFDurz", &g_f3dlMFDurz},
    {"3dlMFDllx", &g_f3dlMFDllx},
    {"3dlMFDlly", &g_f3dlMFDlly},
    {"3dlMFDllz", &g_f3dlMFDllz},
    {"3drMFDulx", &g_f3drMFDulx},
    {"3drMFDuly", &g_f3drMFDuly},
    {"3drMFDulz", &g_f3drMFDulz},
    {"3drMFDurx", &g_f3drMFDurx},
    {"3drMFDury", &g_f3drMFDury},
    {"3drMFDurz", &g_f3drMFDurz},
    {"3drMFDllx", &g_f3drMFDllx},
    {"3drMFDlly", &g_f3drMFDlly},
    {"3drMFDllz", &g_f3drMFDllz},
#endif
    {"FormationBurnerDistance" , &g_fFormationBurnerDistance},
    {"PadlockBreakDistance", &g_fPadlockBreakDistance},
    {"AFRudderRight", &g_fAFRudderRight},
    {"AFRudderLeft", &g_fAFRudderLeft},
    {"AFThrottleDown", &g_fAFThrottleDown},
    {"AFThrottleUp", &g_fAFThrottleUp},
    {"AFAileronLeft", &g_fAFAileronLeft},
    {"AFAileronRight", &g_fAFAileronRight},
    {"AFElevatorDown", &g_fAFElevatorDown},
    {"AFElevatorUp", &g_fAFElevatorUp},
    {"rollStickOffset", &g_frollStickOffset},
    {"pitchStickOffset", &g_fpitchStickOffset},
    {"rudderOffset", &g_frudderOffset},
    {"VisualNormalizeFactor", &g_fVisualNormalizeFactor},
    {"RecoveryAOA", &g_fRecoveryAOA},
    {"clientbwforupdatesmodifyer", &clientbwforupdatesmodifyer},
    {"hostbwforupdatesmodifyer", &hostbwforupdatesmodifyer},
    {"MfdTransparency", &g_fMfdTransparency}, // a.s. begin
    {"MfdRed", &g_fMfdRed},
    {"MfdBlue", &g_fMfdBlue},
    {"MfdGreen", &g_fMfdGreen},
    {"Mfd_p_Size", &g_fMfd_p_Size}, // a.s. end
    {"PullupTime", &g_fPullupTime}, // 2002-02-24 M.N.
    {"GALookAheadTime", &g_fGALookAheadTime}, // Cobra
    {"AIMinAlt", &g_fAIMinAlt}, // Cobra
    {"AIMinWPAlt", &g_fAIMinWPAlt}, // Cobra
    {"GApStickFac", &g_fGApStickFac}, // Cobra
    {"AIHarmMaxRange", &g_fAIHarmMaxRange}, // Cobra
    {"AIJSOWMaxRange", &g_fAIJSOWMaxRange}, // Cobra
    {"AGFlyoutRange", &g_fAGFlyoutRange}, // Cobra
    {"AGSlowFlyoutRange", &g_fAGSlowFlyoutRange}, // Cobra
    {"AGSlowMoverSpeed", &g_fAGSlowMoverSpeed}, // Cobra
    {"AIRefuelRange", &g_fAIRefuelRange}, // 2002-02-28 MN
    {"IdentFactor", &g_fIdentFactor}, // 2002-03-07 S.G.
    {"AIDropStoreLauncherRange", &g_fAIDropStoreLauncherRange}, // 2002-03-08 MN
    {"BiasFactorForFlaks", &g_fBiasFactorForFlaks}, // 2002-03-12 S.G.
    { "TracerAccuracyFactor", &g_fTracerAccuracyFactor }, // 2002-03-12 S.G.
    { "TankerRStick", &g_fTankerRStick }, // 2003-03-13 MN
    { "TankerPStick", &g_fTankerPStick }, // 2003-03-13 MN
    { "TankerTrackFactor", &g_fTankerTrackFactor }, // 2003-03-13 MN
    { "TankerHeadsupDistance", &g_fTankerHeadsupDistance}, // 2003-04-07 MN
    { "TankerBackupDistance", &g_fTankerBackupDistance}, // 2003-04-07 MN
    { "HeadingStabilizeFactor", &g_fHeadingStabilizeFactor}, // 2003-04-07 MN
    { "RocketPitchFactor", &g_fRocketPitchFactor}, // Cobra
    { "RocketPitchCorr", &g_fRocketPitchCorr}, // Cobra
    { "FuelBaseProp", &g_fFuelBaseProp }, // 2002-03-14 S.G.
    { "FuelMultProp", &g_fFuelMultProp }, // 2002-03-14 S.G.
    { "FuelTimeStep", &g_fFuelTimeStep }, // 2002-03-14 S.G.
    { "FuelVtClip", &g_fFuelVtClip }, // 2002-03-14 S.G.
    { "FuelVtDotMult", &g_fFuelVtDotMult }, // 2002-03-14 S.G.
    { "AIRefuelSpeed", &g_fAIRefuelSpeed }, // 2002-03-15 MN
    { "SearchSimTargetFromRangeSqr", &g_fSearchSimTargetFromRangeSqr}, // 2002-03-15 S.G.
    { "NukeStrengthFactor", &g_fNukeStrengthFactor }, // 2002-03-22 MN
    { "NukeDamageMod", &g_fNukeDamageMod }, // 2002-03-25 MN
    { "NukeDamageRadius", &g_fNukeDamageRadius }, // 2002-03-25 MN
    { "ClimbRatio", &g_fClimbRatio }, // 2002-03-25 MN
    { "HotNoseAngle", &g_fHotNoseAngle }, // 2002-03-22 S.G.
    { "MaxMARNoIdA", &g_fMaxMARNoIdA}, // 2002-03-22 S.G.
    { "MinMARNoId5kA", &g_fMinMARNoId5kA }, // 2002-03-22 S.G.
    { "MinMARNoId18kA", &g_fMinMARNoId18kA }, // 2002-03-22 S.G.
    { "MinMARNoId28kA", &g_fMinMARNoId28kA }, // 2002-03-22 S.G.
    { "MaxMARNoIdB", &g_fMaxMARNoIdB}, // 2002-03-22 S.G.
    { "MinMARNoId5kB", &g_fMinMARNoId5kB }, // 2002-03-22 S.G.
    { "MinMARNoId18kB", &g_fMinMARNoId18kB }, // 2002-03-22 S.G.
    { "MinMARNoId28kB", &g_fMinMARNoId28kB }, // 2002-03-22 S.G.
    { "MinMARNoIdC", &g_fMinMARNoIdC }, // 2002-03-22 S.G.
    { "WaypointBurnerDelta", &g_fWaypointBurnerDelta }, // 2002-03-28 MN
    { "GroundImpactMod", &g_fGroundImpactMod}, //MI 2002-03-28
    { "BombMissileAltitude", &g_fBombMissileAltitude}, // 2002-03-28 MN
    { "GMTMaxSpeed", &g_fGMTMaxSpeed}, // 2002-04-03 MN
    { "GMTMinSpeed", &g_fGMTMinSpeed}, // 2002-04-03 MN
    { "ReconCameraHalfFOV", &g_fReconCameraHalfFOV}, //MI 2002-04-03 Recon camera stuff
    { "ReconCameraOffset", &g_fReconCameraOffset}, //MI 2002-04-03 Recon camera stuff
    { "EXPfactor", &g_fEXPfactor}, // 2002-04-05 MN cursor speed reduction in EXP
    { "DBS1factor", &g_fDBS1factor}, // 2002-04-05 MN cursor speed reduction in DBS1
    { "DBS2factor", &g_fDBS2factor}, // 2002-04-05 MN cursor speed reduction in DBS2
    { "HUDonlySize", &g_fHUDonlySize}, // FRB - % Size increase of HUD-Only view (% = decimal)
    { "ePropFactor", &g_fePropFactor}, // 2002-04-05 MN
    { "SunPadlockTimeout", &g_fSunPadlockTimeout}, // 2002-04-06 MN
    { "CarrierStartTolerance", &g_fCarrierStartTolerance},
    { "BombTimeStep", &g_fBombTimeStep}, //MI 2002-04-07 fix for missing bombs
    { "HighDragGravFactor", &g_fHighDragGravFactor}, //MI 2002-04-07 externalised var to allow tweaking afterwards
    { "TgtDZFactor", &g_fTgtDZFactor}, // MN 2002-04-07 fix for ballistic missiles
    { "SSoffsetManeuverPoints1a", &g_fSSoffsetManeuverPoints1a}, // 2002-04-07 S.G.
    { "SSoffsetManeuverPoints1b", &g_fSSoffsetManeuverPoints1b}, // 2002-04-07 S.G.
    { "SSoffsetManeuverPoints2a", &g_fSSoffsetManeuverPoints2a}, // 2002-04-07 S.G.
    { "SSoffsetManeuverPoints2b", &g_fSSoffsetManeuverPoints2b}, // 2002-04-07 S.G.
    { "PinceManeuverPoints1a", &g_fPinceManeuverPoints1a}, // 2002-04-07 S.G.
    { "PinceManeuverPoints1b", &g_fPinceManeuverPoints1b}, // 2002-04-07 S.G.
    { "PinceManeuverPoints2a", &g_fPinceManeuverPoints2a}, // 2002-04-07 S.G.
    { "PinceManeuverPoints2b", &g_fPinceManeuverPoints2b}, // 2002-04-07 S.G.
    { "LethalRadiusModifier", &g_fLethalRadiusModifier}, // 2002-04-14 MN
    { "RAPDistance", &g_fRAPDistance}, // 2002-04-18 MN RollAndPull triggering in MissileEngage
    { "MoverVrValue", &g_fMoverVrValue},
    { "MinBwForOtherData", &MinBwForOtherData},
    { "clientbwforupdatesmodifyerMIN", &g_fclientbwforupdatesmodifyerMIN},
    { "clientbwforupdatesmodifyerMAX", &g_fclientbwforupdatesmodifyerMAX},
    { "ReliablemsgwaitMAX", &g_fReliablemsgwaitMAX},
    { "FOVIncrement", &g_fFOVIncrement}, // Wombat778 09-27-2003
    { "MavEXPLevel", &g_fMavEXPLevel}, // Wombat778 09-27-2003
    { "MavFOVLevel", &g_fMavFOVLevel}, // Wombat778 09-27-2003
    { "MouseLookSensitivity", &g_fMouseLookSensitivity}, // Wombat778 10-08-2003
    { "DefaultFOV", &g_fDefaultFOV}, // Wombat778 10-31-2003
    { "HSDSymbolSize", &g_fHSDSymbolSize}, // Wombat778 11-13-2003
    { "TIRMinimumFOV", &g_fTIRMinimumFOV}, // Cobra
    { "TIRMaximumFOV", &g_fTIRMaximumFOV}, // Cobra
    { "MinimumFOV", &g_fMinimumFOV}, // Wombat778 1-15-2004
    { "MaximumFOV", &g_fMaximumFOV}, // Wombat778 10-11-2003
    { "NarrowFOV", &g_fNarrowFOV}, // Wombat778 2-21-2004
    { "WideviewFOV", &g_fWideviewFOV}, // Wombat778 2-21-2004
    { "MeanTimeBetweenFailures", &g_fMeanTimeBetweenFailures}, // Wombat778 2-21-2004
    { "JDAMLift", &g_fJDAMLift}, // Wombat778 2-21-2004
    { "AIJSOWmaxRange", &g_fAIJSOWmaxRange}, // Cobra
    { "ButtonScaler", &g_fButtonScaler},
    { "ButtonZScaler", &g_fButtonZScaler},
    { "HybridPitThreshold1", &g_fHybridPitThreshold1}, // Wombat778 11-19-2004
    { "HybridPitThreshold2", &g_fHybridPitThreshold2}, // Wombat778 11-19-2004
    { "TIR2DYawPercentage", &g_fTIR2DYawPercentage}, // Retro 02/10/03
    { "TIR2DPitchPercentage", &g_fTIR2DPitchPercentage}, // Retro 02/10/03
    { "DBS1ScanRateFactor", &g_fDBS1ScanRateFactor}, //JAM 13Oct03
    { "DBS2ScanRateFactor", &g_fDBS2ScanRateFactor}, //JAM 13Oct03
    //{ "SoundDopplerFactor", &g_fSoundDopplerFactor }, // MLR 2003-10-17
    //{ "SoundRolloffFactor", &g_fSoundRolloffFactor }, // MLR 2003-10-18
    //    { "SoundDopplerBlend", &g_fSoundDopplerBlend}, // MLR 12/3/2003 - OBSOLETE

    { "A2GJDAMAlt", &g_fA2GJDAMAlt}, //TJL 10/27/03 Sets AI JDAM attack altitude
    { "A2GJSOWAlt", &g_fA2GJSOWAlt}, //TJL 10/27/03 Sets AI JSOW attack altitude
    { "A2GHarmAlt", &g_fA2GHarmAlt}, //TJL 10/27/03 Sets AI HARM attack altitude (all set to SP3 defaults)
    { "A2GAGMAlt", &g_fA2GAGMAlt}, //TJL 10/27/03 Sets AI AGM attack altitude
    { "A2GGBUAlt", &g_fA2GGBUAlt}, //TJL 10/27/03 Sets AI GBU attack altitude
    { "A2GDumbHDAlt", &g_fA2GDumbHDAlt}, //TJL 10/27/03 Sets AI Durandal attack altitude
    { "A2GClusterAlt", &g_fA2GClusterAlt}, //TJL 10/27/03 Sets AI Cluster Bomb attack altitude
    { "A2GDumbLDAlt", &g_fA2GDumbLDAlt}, //TJL 10/27/03 Sets AI Generic attack altitude
    { "A2GGenericBombAlt", &g_fA2GGenericBombAlt}, //TJL 10/27/03 Sets AI Generic Bomb attack altitude
    { "A2GGunRocketAlt", &g_fA2GGunRocketAlt}, //TJL 10/27/03 Sets AI Gun and Rocket altitude
    { "A2GCameraAlt", &g_fA2GCameraAlt}, //TJL 10/27/03 Sets AI BDA/Recon altitude

    { "TexDetailFactor", &g_fTexDetailFactor}, ////THW 2003-11-14 Use Higher-Res textures at higher altitudes

    { "ACMIAnimRecordTimer", &g_fACMIAnimRecordTimer}, // MLR 3/2/2004 - seconds between recording a/c animation data
    { "dpitStart", &g_fdpitStart}, //Cobra TJL 11/08/04 in 24 hr clock
    { "dpitStop", &g_fdpitStop}, //Cobra TJL 11/08/04 in 24 hr clock
    { "TaxiEarly", &g_fTaxiEarly}, //RAS Amount of seconds to taxi early

    { "Dyn_Head_TiltMul", &g_fDyn_Head_TiltMul}, //ATARIBABY externalised 3d pit dynamic head params
    { "Dyn_Head_TiltRndGMul", &g_fDyn_Head_TiltRndGMul},
    { "Dyn_Head_RollMul", &g_fDyn_Head_RollMul},
    { "Dyn_Head_PanMul", &g_fDyn_Head_PanMul},
    { "Dyn_Head_TiltRateMul", &g_fDyn_Head_TiltRateMul},
    { "Dyn_Head_TiltGRateMul", &g_fDyn_Head_TiltGRateMul},
    { "Dyn_Head_RollRate", &g_fDyn_Head_RollRate},
    { "Dyn_Head_PanRate", &g_fDyn_Head_PanRate},

    { "PilotActInterval", &g_fPilotActInterval}, //Cobra - Pilot animation act interval (minutes)
    { "PilotHeadMoveRate", &g_fPilotHeadMoveRate}, //Cobra - Pilot animation move rate (deg/sec)
    { "3DHeadTilt", &g_f3DHeadTilt}, //Cobra - Head tilt when entering the 3D cockpit
    { "3DPitFOV", &g_f3DPitFOV}, //Cobra - FOV when entering the 3D cockpit
    { NULL, NULL }
};

void ParseFalcon4Config(FILE *file)
{
    char strLine[0x100];
    char strID[0x100];
    int nIntVal;
    float fFloatVal;
    char strVal[0x100];
    // float nFloatVal;

NextLine:

    while (fgets(strLine, sizeof(strLine) / sizeof(strLine[0]), file))
    {
        if ((strlen(strLine) <= 1) or (strstr(strLine, "//") == strLine))
            continue;

        if (sscanf(strLine, "set %s \"%s\"", strID, &strVal) == 2)
        {
            if (strstr(strID, "g_s") == strID)
            {
                // Integer value
                ConfigOption<char> *pOpts = StringOpts;

                while (pOpts->Name)
                {
                    if (pOpts->CheckID(strID))
                    {
                        char *p = strstr(strLine, "\"") + 1;
                        char *p2 = strstr(p, "\"");

                        if (p2)
                            strncpy(pOpts->Value, p, p2 - p);

                        goto NextLine;
                    }

                    pOpts++;
                }
            }
        }

        if (sscanf(strLine, "set %s %f", strID, &fFloatVal) == 2)
        {
            if (strstr(strID, "g_f") == strID)
            {
                // Boolean value
                ConfigOption<float> *pOpts = FloatOpts;

                while (pOpts->Name)
                {
                    if (pOpts->CheckID(strID))
                    {
                        *pOpts->Value = fFloatVal;
                        goto NextLine;
                    }

                    pOpts++;
                }
            }
        }

        if (sscanf(strLine, "set %s %d", strID, &nIntVal) == 2)
        {
            if (strstr(strID, "g_b") == strID)
            {
                // Boolean value
                ConfigOption<bool> *pOpts = BoolOpts;

                while (pOpts->Name)
                {
                    if (pOpts->CheckID(strID))
                    {
                        *pOpts->Value = nIntVal ? true : false;
                        goto NextLine;
                    }

                    pOpts++;
                }
            }

            else if (strstr(strID, "g_n") == strID)
            {
                // Integer value
                ConfigOption<int> *pOpts = IntOpts;

                while (pOpts->Name)
                {
                    if (pOpts->CheckID(strID))
                    {
                        *pOpts->Value = nIntVal;
                        goto NextLine;
                    }

                    pOpts++;
                }
            }
        }
    }
}

void ReadFalcon4Config()
{
    int nBufLen = 1024;
    char *strAppPath = new char[nBufLen];

    if ( not strAppPath) return;

    char *strDir = new char[nBufLen];

    if ( not strDir) return;

    // sprintf(strDir, "%s\\FalconBMS.cfg", FalconDataDirectory);
    // sprintf(strDir, "%s\\Cobra.cfg", FalconDataDirectory);
    sprintf(strDir, "%s\\FFViper.cfg", FalconDataDirectory);
    FILE *file = fopen(strDir, "r");

    if ( not file)
    {
        // strcpy(strDir, "FalconBMS.cfg");
        // strcpy(strDir, "Cobra.cfg");
        strcpy(strDir, "FFViper.cfg");
        file = fopen(strDir, "r");
    }

    if ( not file)
    {
        // Investigate program directory
        HMODULE Module = ::GetModuleHandle(NULL);

        if ( not ::GetModuleFileName(Module, strAppPath, nBufLen)) return;

        int nAppPathLen = strlen(strAppPath);

        if (nAppPathLen < 2) return;

        char *p = &strAppPath[nAppPathLen - 1];

        while (p > strAppPath and *p not_eq '\\') p--;

        if (p == strAppPath) return;

        p++;
        int nDirLen = p - strAppPath;
        memcpy(strDir, strAppPath, nDirLen);
        strDir[nDirLen] = '\0';
        // strcat(strDir, "FalconBMS.cfg");
        strcat(strDir, "FFViper.cfg");
        file = fopen(strDir, "r");
    }

    if (file)
    {
        ParseFalcon4Config(file);
        fclose(file);
    }

    // JB 010104 Second config file overrides the first and can be CRC checked by anti-cheat programs.
    // strcat(strDir, "FalconBMSServer.cfg");
    strcat(strDir, "FFViperServer.cfg");
    file = fopen(strDir, "r");

    if (file)
    {
        ParseFalcon4Config(file);
        fclose(file);
    }

    //JB 010104 Cobra 11/20/04
    /*if ( not g_bwoeir)
    { g_bMLU = false;
      g_bIFF = false;}*/

    delete[] strDir;
    delete[] strAppPath;
}
