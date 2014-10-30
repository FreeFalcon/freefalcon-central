#ifndef _DIGI_H
#define _DIGI_H

#include "f4vu.h"
#include "simmath.h"
#include "simbrain.h"
#include "geometry.h"
#include "MsgInc/WingmanMsg.h"
#include "atcbrain.H"

// Forward declarations for class pointers
class SimVehicleClass;
class MissileClass;
class SimWeaponClass;
class BombClass;
class GunClass;
class RadarClass;
class AircraftClass;
class AirframeClass;
class SimObjectType;
class SimObjectLocalData;
class FlightClass; // 2002-02-27 S.G.
enum  MissionTypeEnum;

/*********************************************************/
/* include file for the digi aircraft                    */
/*********************************************************/

/*------------------------------------------------*/
/* defines for wingman formations   */
/*------------------------------------------------*/

#define SLOP_FACTOR 10 // degrees
#define HALF_SLOP_FACTOR SLOP_FACTOR / 2 // degrees
#define SLOT_RADIUS NM_TO_FT // feet

#define MinClassSpeed 200.0F //Knots
#define MaxClassSpeed 600.0F //Knots
#define MinSlotDist SLOT_RADIUS //Feet
#define MaxSlotDist MinSlotDist * 3 //Feet

#define TOTAL_MANEUVER_PTS 3

/*----------------------------------------------------------*/
/* Formation information.  All data is relative to the lead */
/*----------------------------------------------------------*/
class ACFormationData
{
public:
    typedef struct
    {
        int formNum; // Enum of the Wingman message
        float relAz;
        float relEl;
        float range;
    } PositionData;

    ACFormationData(void);
    ~ACFormationData(void);

    int numFormations;
    PositionData **positionData;
    PositionData *twoposData;
    int FindFormation(int);
};

class DigitalBrain : public BaseBrain
{
public:
    // Pete Style Maneuver Stuff
    enum BVRInterceptType
    {
        BvrFollowWaypoints,
        BvrFlyFormation,
        BvrSingleSideOffset,
        BvrPince,
        BvrPursuit,
        BvrNoIntercept,
        BvrPump,
        BvrCrank,
        BvrCrankRight,
        BvrCrankLeft,
        BvrNotch,
        BvrNotchRight,
        BvrNotchRightHigh,
        BvrNotchLeft,
        BvrNotchLeftHigh,
        BvrGrind,
        BvrCrankHi,//Cobra added hi/lo crankers
        BvrCrankLo,
        BvrCrankRightHi,
        BvrCrankRightLo,
        BvrCrankLeftHi,
        BvrCrankLeftLo,
        NumInterceptTypes
    };
    enum BVRProfileType
    {
        Pnone,
        Plevel1a,
        Plevel1b,
        Plevel1c,
        Plevel2a,
        Plevel2b,
        Plevel2c,
        Plevel3a,
        Plevel3b,
        Plevel3c,
        Pbeamdeploy,
        Pbeambeam,
        Pwall,
        Pgrinder,
        Pwideazimuth,
        Pshortazimuth,
        PwideLT,
        PShortLT,
        PSweep,
        PDefensive
    };


    enum WVRMergeManeuverType
    {
        WvrMergeHitAndRun,
        WvrMergeLimited,
        WvrMergeUnlimited,
        NumWVRMergeMnverTypes
    };

    enum SpikeReactionType
    {
        SpikeReactNone,
        SpikeReactECM,
        SpikeReactBeam,
        SpikeReactDrag,
        NumSpikeReactionTypes
    };

    enum ACMnverClass
    {
        MnvrClassF4,
        MnvrClassF5,
        MnvrClassF14,
        MnvrClassF15,
        MnvrClassF16,
        MnvrClassMig25,
        MnvrClassMig27,
        MnvrClassA10,
        MnvrClassBomber,
        NumMnvrClasses
    };

    enum ACMnverClassFlags
    {
        CanLevelTurn    = 0x1,
        CanSlice        = 0x2,
        CanUseVertical  = 0x4,
        CanOneCircle    = 0x10,
        CanTwoCircle    = 0x20,
        CanJinkSnake    = 0x100,
        CanJinkLoaded   = 0x200,
        CanJinkUnloaded = 0x400
    };

    enum OffsetDirs
    {
        offForward,
        offRight,
        offBack,
        offLeft,
        taxiLeft,
        taxiRight,
        downRunway,
        upRunway,
        rightRunway,
        leftRunway,
        centerRunway,
    };

    /*--------------*/
    /* Mnvr Element */
    /*--------------*/
    typedef struct
    {
        BVRInterceptType*     intercept;
        WVRMergeManeuverType* merge;
        SpikeReactionType*    spikeReact;
        char                  numIntercepts;
        char                  numMerges;
        char                  numReacts;
    } ManeuverChoiceTable;

    typedef struct
    {
        int flags;
    } ManeuverClassData;

    // 2002-03-11 ADDED BY S.G. Which ManeuverChoiceTable element to use in CanEngage
    enum MaveuverType
    {
        BVRManeuver = 0x01,
        WVRManeuver = 0x02
    };

    enum RocketMnvrType
    {
        RocketFlyToTgt,
        RocketFiring,
        RocketJink,
        RocketFlyout
    };

    RocketMnvrType rocketMnvr;
    float rocketTimer;

    static ManeuverChoiceTable maneuverData[NumMnvrClasses][NumMnvrClasses];
    static ManeuverClassData maneuverClassData[NumMnvrClasses];
    static void ReadManeuverData(void);
    static void FreeManeuverData(void);

    //for now landing is a high priority mode so they won't get distracted
    enum DigiMode
    {
        TakeoffMode, GroundAvoidMode, CollisionAvoidMode, GunsJinkMode, MissileDefeatMode,
        LandingMode, DefensiveModes, RefuelingMode,
        SeparateMode, AccelMode, MergeMode, MissileEngageMode, GunsEngageMode,
        RoopMode, OverBMode, WVREngageMode, BVREngageMode,
        LoiterMode, FollowOrdersMode, RTBMode, WingyMode,
        BugoutMode, WaypointMode, GroundMnvrMode,
        LastValidMode, NoMode
    };

    enum WvrTacticType {WVR_NONE, WVR_RANDP, WVR_OVERB, WVR_ROOP, WVR_GUNJINK, WVR_STRAIGHT,
                        WVR_BUGOUT, WVR_AVOID, WVR_BEAM, WVR_BEAM_RETURN, WVR_RUNAWAY
                       };

    // engagement tactic from campaign
    int campTactic;

    DigiMode GetCurrentMode(void)
    {
        return curMode;
    }

    enum RefuelStatus
    {
        refNoTanker,
        refVectorTo,
        refWaiting,
        refRefueling,
        refDone,
    };
    int IsAtFirstTaxipoint();

protected:

    AircraftClass* self;
    AirframeClass* af;

    enum WaypointState
    {
        NotThereYet, Arrived, Stabalizing, OnStation, PreRoll, Departing, HoldingShort, HoldInPlace,
        TakeRunway, Takeoff, Taxi, Upwind, Crosswind, Downwind, Base, Final, Final1
    };

    // Controls
    SAVE_ARRAY pstickSave;
    float       autoThrottle;
    int         trackMode;
    float       maxGs, cornerSpeed;

    // Ground Avoid
    int groundAvoidNeeded;
    float groundAvoidPStick;
    float gaRoll, gaGs;
    float maxElevation; // Cobra
    int PullupNow; // Cobra

    // Mode Stuff
    int waypointMode;
    WaypointState onStation;
    DigiMode curMode;
    DigiMode lastMode;
    DigiMode nextMode;

    void AddMode(DigiMode);
    void ResolveModeConflicts(void);
    void PrtMode(void);
    void PrintOnline(char *str);
    void SetCurrentTactic(void);

    // Maneuvers
    float trackX, trackY, trackZ;
    float holdAlt, holdPsi;
    float gammaHoldIError;
    float reactiont;//me123 reaction time for ai
    int  MissileEvade(void);
    void ChoiceProfile(void);// me123 canned profiles
    void DoProfile(void);// me123 canned profiles
    void level1a(void);// me123 canned profiles
    void level1b(void);//me123
    void level1c(void);//me123
    void level2a(void);//me123
    void level2b(void);//me123
    void level2c(void);//me123
    void level3a(void);//me123
    void level3b(void);//me123
    void level3c(void);//me123
    void beamdeploy(void);//me123
    void beambeam(void);//me123
    void wall(void);//me123
    void grinder(void);//me123
    void wideazimuth(void);//me123
    void shortazimuth(void);//me123
    void wideLT(void);//me123
    void ShortLT(void);//me123
    void Defensive(void);//me123
    void Sweep(void);//me123
    void StickandThrottle(float DesiredSpeed, float DesiredAltitude); //me123
    int  MissileBeamManeuver(void);
    void MissileCrankManeuver(void);//me123
    void  BaseLineIntercept(void);//me123
    int  BeamManeuver(int direction, int Height);//me123
    void CrankManeuver(int direction, int Height);//me123 //Cobra add height
    void DragManeuver(void);//me123
    int  MachHold(float, float, int);
    void PullUp(void);
    void RollAndPull(void);
    void PullToCollisionPoint(void);
    void PullToControlPoint(void);
    void EnergyManagement(void);//me123
    void EagManage(void);//me123
    void MaintainClosure(void);
    void MissileDefeat(void);
    void MissileDragManeuver(void);
    void MissileLastDitch(float xft, float yft, float zft);
    void GunsEngage(void);
    void TrainableGunsEngage(void);
    void AccelManeuver(void);
    void GunsJink(void);
    void MissileEngage(void);
    void FollowWaypoints(void);
    void Loiter(void);
    void LevelTurn(float loadFactor, float turnDir, int newTurn);
    void GammaHold(float desGamma);
    int  AltitudeHold(float desAlt);
    int  HeadingAndAltitudeHold(float desHeading, float desAlt);
    float CollisionTime(void);
    void GoToCurrentWaypoint(void);
    void SelectNextWaypoint(void);
    void SetWaypointSpecificStuff(void);
    int  GetWaypointIndex(void);
    int  GetTargetWPIndex(void);
    void RollOutOfPlane(void);
    void OverBank(float delta);
    bool EvaluateTarget(DWORD Type);

    // ground attack stuff....
    void GroundAttackMode(void);
    void SetGroundTarget(FalconEntity *obj);
    void SetGroundTargetPtr(SimObjectType *obj);
    void SelectGroundTarget(int selectFilter);
    void SelectCampGroundTarget(void);
    void DoPickupAirdrop(void);
    void TakePicture(float, float);
    void DropBomb(float, float, RadarClass*);
    void DropGBU(float, float, RadarClass*);
    void FireAGMissile(float, float);
    int FireRocket(float, float);
    int GunStrafe(float, float);
    int  MaverickSetup(float, float, float, float, RadarClass* theRadar);
    int  HARMSetup(float, float, float, float);
    int  JSOWSetup(float, float, float, float);
    void AGflyOut();
    float rangeToIP;

    enum AG_TARGET_TYPE {TARGET_ANYTHING, TARGET_FEATURE, TARGET_UNIT};

    void SelectGroundWeapon(void);
    int hasBomb;
    int hasAGMissile;
    int hasHARM;
    int hasRocket;
    int hasGun;
    int hasCamera;
    int hasGBU;
    // Marco Edit - check for Clusters/Durandals specifically
    int hasCluster;
    int hasDurandal;

    int hasWeapons;

    SimObjectType* agThreatTargetPtr; //Cobra Let's target those pesky SAMs, yet keep a normal GTPtr
    SimObjectType* groundTargetPtr;
    SimObjectType* airtargetPtr; // for air diverts
    int sentWingAGAttack;
    BOOL  droppingBombs;
    BOOL  agImprovise;
    enum AG_ORDERS {AG_ORDER_NONE, AG_ORDER_FORMUP, AG_ORDER_ATTACK};
    enum AG_DOCTRINE {AGD_NONE, AGD_SHOOT_RUN, AGD_LOOK_SHOOT_LOOK, AGD_NEED_SETUP};
    enum AG_APPROACH {AGA_LOW, AGA_TOSS, AGA_HIGH, AGA_DIVE, AGA_BOMBER};

    void SetupAGMode(WayPointClass *, WayPointClass *);

    AG_DOCTRINE agDoctrine;
    AG_APPROACH agApproach;

    VU_TIME nextAGTargetTime;
    VU_TIME nextAttackCommandToSend; // 2002-01-20 ADDED BY S.G.
    BOOL madeAGPass;
    float AGattackAlt; // Cobra - holder for current weapon ground attack altitude
    float ipX, ipY, ipZ; // insertion point loc for AG run

public:
    // 2002-03-05 MODIFIED BY S.G. Added 'Enum' to differentiate it from the getter MissionClass
    enum MissionClassEnum {AGMission, AAMission, SupportMission, AirliftMission};
    //Public ATC Stuff
    VU_TIME lastVectorTypeCall; //RAS-3Oct04-timer for last vector type call

protected:
    MissionTypeEnum missionType;
    MissionClassEnum  missionClass;
    BOOL missionComplete;

    //ATC stuff
    VU_TIME createTime; //when this brain was created in terms of SimLibElapsedTime
    unsigned int atcFlags; //need to know what has happened to them already
    int rwIndex; //0 if none, else index of runway to use
    VU_ID airbase; //airbase to land at
    VU_TIME rwtime; //time scheduled to use runway
    VU_TIME updateTime; //time to update distance info
    float distAirbase; // how far are we from our desired airbase?

    AtcStatusEnum atcstatus; //at what point in the takeoff/landing process are you?
    int  curTaxiPoint; //index into PtDataTable, 0 means none
    float desiredSpeed; //speed at which we want to move so we arrive at next point on time
    float turnDist; //when distance to point is less than this we execute a
    // turn onto our new heading
    VU_TIME waittimer; //time at which we have been in this state too long

    //Refueling stuff
    RefuelStatus refuelstatus; //current status
    VU_ID tankerId; //which tanker we are using
    int tnkposition; //where are we in the queue to tank, 0 == currently tanking, -1 == don't know yet
    float LastVCas;

    void Land(void);
    void TakeOff(void);
    int PreFlight(void);
    void QuickPreFlight(void);
    void TaxiBack(ObjectiveClass *Airbase);
    SimBaseClass* CheckTaxiTrackPoint(void);
    SimBaseClass* CheckPoint(float x, float y);
    BOOL CheckTaxiCollision(void);
    BOOL SimpleGroundTrack(float);
    void FindParkingSpot(ObjectiveClass *Airbase);
    bool CloseToTrackPoint(void);
    bool AtFinalTaxiPoint(void);
    int BestParkSpot(ObjectiveClass *Airbase);
    void RandomStuff(SimBaseClass *inTheWay = NULL);

    void FuelCheck(void);
    void IPCheck(void);



    // Targeting
    float ataDot;
    float lastAta;
    SimObjectType* targetList;

    virtual void SetTarget(SimObjectType* newTarget);
    void DoTargeting(void);
    SimObjectType* InsertIntoTargetList(SimObjectType* root, SimObjectType* newObj);


    // Threat Avoid
    VU_ID missileDefeatId;
    int missileFindDragPt;
    int missileFinishedBeam;
    int missileShouldDrag;
    float missileDefeatTtgo;

    // Missile Engage
    MissileClass* curMissile;
    int curMissileStation;
    int curMissileNum;
    // 2000-11-17 ADDED BY S.G. I NEED THIS TWO VARIABLE IN AIRCRAFT CLASS TO STORE THE LATEST MISSILE LAUNCHED. SINCE THEY'RE NOT USED, I'll RENAME THEM TO SOMETHING THAT MAKE MORE SENSE.
public:
    VuEntity *missileFiredEntity;
    VU_TIME missileFiredTime;
    VU_TIME visDetectTimer; //Cobra
    // S.G. BACK TO PROTECTED
    //MI taking this opportunity....
    float destRoll, destPitch, currAlt;
    int detRWR;
    int detRAD;
    int detVIS;
protected:
    // 2001-08-31 ADDED BY S.G. NEED TO KNOW THE LAST TWO GROUND TARGET
    // AN AI TARGETED SO OTHER AI IN THE FLIGHT CAN SKIP THEM
    SimBaseClass *gndTargetHistory[2];
    // 2001-08-31 ADDED BY S.G. NEED TO KNOW THE LAST GROUND CAMPAIGN TARGET IT GOT
    CampBaseClass *lastGndCampTarget;

    float jammertime; //me123
    VU_TIME holdlongrangeshot;//me123
    VU_TIME missileShotTimer;
    float          maxEngageRange;

    // 2002-02-24 added by MN do pullup for g_nPullupTime seconds before reevaluating
    VU_TIME pullupTimer;

    // used in actions.cpp, "AirbaseCheck" function
    int airbasediverted;
    VU_TIME nextFuelCheck;

    // Guns Engage
    VU_TIME jinkTime;
    VU_TIME waitingForShot;
    float ataddot, rangeddot, mnverTime;
    float newroll, pastAta, pastPstick, pastPipperAta;

    float GunsAutoTrack(float maxGs);
    void FineGunsTrack(float speed, float *lagAngle);
    void CoarseGunsTrack(float speed, float leadTof, float *newata);

    // Simple flight model
    enum SimpleTrackMode {SimpleTrackDist, SimpleTrackSpd, SimpleTrackTanker};
    void SimpleTrack(SimpleTrackMode, float);
    float SimpleTrackAzimuth(float, float, float);
    float SimpleTrackElevation(float, float);
    float SimpleTrackDistance(float, float);
    float SimpleTrackSpeed(float);
    float SimpleScaleThrottle(float);
    void SimpleGoToCurrentWaypoint(void);
    void SimplePullUp(void);
    float SimpleGroundTrackSpeed(float v);
    void CalculateRelativePos(float*, float*, float*, float*, float*, float*);

    VU_TIME mLastOrderTime;

    // Wing Radio Calls
    void RespondShortCallSign(int);

    // Controls for Simple flight Model
    int SelectFlightModel(void);

    // Decision Stuff
    void Actions(void);
    void DecisionLogic(void);
    void TargetSelection(void);
    FalconEntity* CampTargetSelection(void);
    void WeaponSelection(void);
    void FireControl(void);
    void RunDecisionRoutines(void);
    void GroundCheck(void);
    void GunsEngageCheck(void);
    void GunsJinkCheck(void);
    void CollisionCheck(void);
    void MissileDefeatCheck(void);
    void MissileEngageCheck(void);
    void MergeCheck(void);
    void AccelCheck(void);
    void MergeManeuver(void);
    void     SeparateCheck(void);
    void     SensorFusion(void);
    float    SetPstick(float, float, int);
    float    SetRstick(float);
    float    SetYpedal(float);
    float    SetRollCapture(float);
    void     SetMaxRoll(float);
    void     SetMaxRollDelta(float);
    void     ResetMaxRoll(void);
    void CollisionAvoid(void);
    float AutoTrack(float);
    float TrackPoint(float maxGs, float speed);
    float TrackPointLanding(float speed);
    float VectorTrack(float, int fineTrack = FALSE);
    int Stagnated(void);
    // 2002-03-11 MN Check for fuel state, head to closest airbase if SaidFumes
    void AirbaseCheck(void);
    //TJL 11/07/03
    VU_TIME  bugoutTimer;
    VU_TIME  targetTimer;//Cobra wait time if a/a targets are bombers
    void chooseRadarMode(void); //Cobra See function for details
    int radModeSelect;//Cobra works with chooseRadarMode

    // WVR Stuff
    void WvrEngageCheck(void);
    void WvrChooseTactic(void);
    void WvrEngage(void);
    void WvrRollOutOfPlane(void);
    void WvrStraight(void);
    void WvrBugOut(void);
    void WvrGunJink(void);
    void WvrOverBank(float delta);
    void WvrAvoid(void);
    void WvrBeam(void);
    void WvrRunAway(void);
    VU_TIME mergeTimer;
    VU_TIME wvrTacticTimer;
    VU_TIME engagementTimer;
    VU_TIME agmergeTimer;//Cobra for A/G missions
    int slowMover; // Cobra - A/G slow a/c (e.g., -10)
    WvrTacticType wvrCurrTactic;
    WvrTacticType wvrPrevTactic;
    float maxAAWpnRange;
    BOOL buggedOut;

    // RV - Biker - Air mobile stuff
    bool doneAirdrop;
    int numParatroopers;

    // RV - Biker - Check for VTOL AC
    int isVTOL;

    // Bvr Stuff
    void BvrEngageCheck(void);
    void BvrChooseTactic(void);
    void BvrEngage(void);
    int IsSupportignmissile(void);// are self or our wingie suporting a missile
    int WhoIsSpiked(void);//me123
    int WhoIsHotnosed(void);//me123
    int HowManySpiked(void);//me123
    int HowManyHotnosed(void);//me123
    int HowManyTargetet(void);//me123
    int IsSplitup(void);//me123
    void CalculateMAR(void);//me123
    void AiFlyBvrFOrmation(void); //me123
    VU_TIME bvrTacticTimer;//me123
    bool spiked;//me123
    int offsetdir;//me123
    VU_TIME spikeseconds;//me123
    VU_TIME spikesecondselement;//me123
    float MAR;//me123
    float TGTMAR ;//me123
    float DOR ;//me123
    bool Isflightlead;//me123
    bool IsElementlead;//me123
    int bvractionstep;//me123
    BVRProfileType bvrCurrProfile;//me123
    BVRInterceptType bvrCurrTactic;
    BVRInterceptType bvrPrevTactic;
    VuEntity *missilelasttime  ;
    VU_TIME spiketframetime  ;
    FalconEntity* lastspikeent ;
    VU_TIME spiketframetimewingie ;
    FalconEntity* lastspikeentwingie ;

    // Threat handling
    SimObjectType* threatPtr;
    SimObjectType* preThreatPtr;
    float threatTimer;
    void SetThreat(FalconEntity *obj);
    BOOL HandleThreat(void);

public:
    DigitalBrain(AircraftClass *myPlatform, AirframeClass* myAf);
    virtual ~DigitalBrain(void);
    virtual void Sleep(void);
    void           ClearCurrentMissile(void)
    {
        curMissile = NULL;
    };
    void SetBvrCurrProfile(BVRProfileType newProfile)
    {
        bvrCurrProfile = newProfile;
    };
    float PIDLoop(
        float error, float K, float KD, float KI,
        float Ts, float *lastErr, float *MX, float Output_Top,
        float Output_Bottom, bool LimitMX
    );

    void ReSetLabel(SimBaseClass *theObject);

    void ReceiveOrders(FalconEvent* theEvent);
    void JoinFlight(void);
    void CheckLead(void);
    void SetLead(int flag);
    void FrameExec(SimObjectType*, SimObjectType*);
    void ThreeAxisAP(void);
    void WaypointAP(void);
    void LantirnAP(void);
    void RealisticAP(void);
    void HDGSel(void);
    void PitchRollHold(void);
    void RollHold(void);
    void PitchHold(void);
    void AltHold(void);
    void FollowWP(void);
    void CheckForTurn(void);
    bool APAutoDisconnect(void);
    int CheckAPParameters(void);
    void AcceptManual(void);
    float HeadingDifference, bank;
    void SetHoldAltitude(float newAlt)
    {
        holdAlt = newAlt;
    };
    float GetHoldAltitude()
    {
        return holdAlt;
    };
    void SetHoldHeading(float newPsi)
    {
        holdPsi = newPsi;
    };
    float GetHoldHeading()
    {
        return holdPsi;
    };
    SimObjectType* GetGroundTarget(void)
    {
        return groundTargetPtr;
    };
    MissionTypeEnum MissionType(void)
    {
        return missionType;
    }
    // 2002-03-05 ADDED BY S.G. Need it public
    MissionClassEnum MissionClass(void)
    {
        return missionClass;
    }

#ifdef DAVE_DBG
    void SetDebugLabel(ObjectiveClass*);
#else
    void SetDebugLabel(ObjectiveClass*) {};
#endif

    VU_TIME CreateTime(void)
    {
        return createTime;
    }
    int CalcWaitTime(ATCBrain *Atc);
    void ResetTaxiState(void);
    void SetTaxiPoint(int pt)
    {
        curTaxiPoint = pt;
    }
    int GetTaxiPoint(void)
    {
        return curTaxiPoint;
    }
    void UpdateTaxipoint(void);
    AircraftClass* GetLeader(void);
    int FindDesiredTaxiPoint(ulong takeoffTime, int rwindx);
    int FindDesiredTaxiPoint(ulong takeoffTime);
    // sfr: changed to reference and added multiple set tracks
    void GetTrackPoint(float &x, float &y, float &z);
    void SetTrackPoint(float x, float y)
    {
        trackX = x;
        trackY = y;
    }
    void SetTrackPoint(float x, float y, float z)
    {
        SetTrackPoint(x, y);
        trackZ = z;
    }
    void SetTrackPoint(SimObjectType *obj); //sfr: added
    void ChooseNextPoint(ObjectiveClass *Airbase);
    void DealWithBlocker(SimBaseClass *inTheWay, ObjectiveClass *Airbase);
    int WingmanTakeRunway(ObjectiveClass *Airbase, AircraftClass *flightLead, AircraftClass *leader);
    int WingmanTakeRunway(ObjectiveClass *Airbase);
    void OffsetTrackPoint(float dist, int dir);
    void ResetTimer(int delta);
    void SetATCFlag(int flag)
    {
        atcFlags or_eq flag;
    }
    void ClearATCFlag(int flag)
    {
        atcFlags and_eq compl flag;
    }
    int IsSetATC(int flag)
    {
        return (atcFlags bitand flag) and TRUE;
    }
    void ResetATC(void);
    void FlightMemberWantsFuel(int state);

    enum ATCFlags
    {
        Landed    = 0x01,
        PermitRunway      = 0x02,
        PermitTakeoff     = 0x04,
        HoldShort         = 0x08,
        EmerStop          = 0x10,
        TakeoffAborted    = 0x20,
        MissionCanceled   = 0x40,
        RequestTakeoff    = 0x80,
        Refueling         = 0x100,
        NeedToRefuel      = 0x200,
        ClearToLand = 0x400,
        PermitTakeRunway  = 0x800,
        WingmanReady      = 0x1000,
        AceGunsEngage     = 0x2000,
        SaidJoker         = 0x4000,
        SaidBingo         = 0x8000,
        SaidFumes         = 0x10000,
        SaidFlameout      = 0x20000,
        HasTrainable      = 0x40000,
        FireTrainable     = 0x80000,
        AskedToEngage     = 0x100000,
        ReachedIP         = 0x200000,
        HasAGWeapon       = 0x400000,
        OnSweep           = 0x800000,
        InShootShoot      = 0x1000000,
        CheckTaxiBack = 0x2000000,
        WaitingPermission = 0x4000000,
        StopPlane = 0x8000000,
        SaidRTB     = 0x10000000,
        // 2001-08-31 MODIFIED BY S.G. I USED 0x80000000 BUT JULIAN ALREADY USED IT. InhibitDefensive IS NOT USED ANYMORE SO I'LL REUSE IT INSTEAD
        //    InhibitDefensive  = 0x20000000,
        HasCanUseAGWeapon  = 0x20000000,
        // 2000-11-17 ADDED BY S.G. SO AI WILL WAIT FOR TARGET TO BE IN RANGE BEFORE ASKING TO ENGAGE
        WaitForTarget     = 0x40000000,
        DonePreflight     = 0x80000000 // JPO - done all preflight checks


    };

    enum MoreFlags
    {
        KeepTryingRejoin = 0x00000001,
        KeepTryingAttack = 0x00000002,
        KeepLasing = 0x00000004,
        SaidImADot = 0x00000008, // MN
        NewHomebase     = 0x00000010, // MN
        SaidSunrise = 0x00000020, // MN
        HUDSetup = 0x00000040
    };

    void SendATCMsg(AtcStatusEnum msg);
    void SetATCStatus(AtcStatusEnum status)
    {
        atcstatus = status;
    }
    AtcStatusEnum ATCStatus(void)
    {
        return atcstatus;
    }
    void SetWaitTimer(VU_TIME timer)
    {
        waittimer = timer;
    }
    VU_TIME WaitTime(void)
    {
        return waittimer;
    }

    void SetRunwayInfo(VU_ID Airbase, int rwindex, unsigned long time);
    VU_ID Airbase(void)
    {
        return airbase;
    }
    int Runway(void)
    {
        return rwIndex;
    }
    VU_TIME RwTime(void)
    {
        return rwtime;
    }
    float CalculateNextTurnDistance(void);
    float TurnDistance(void)
    {
        return turnDist;
    }
    int ReadyToGo(void);
    float CalculateTaxiSpeed(float time);

    void StartRefueling(void);
    void DoneRefueling(void);
    void SetRefuelStatus(RefuelStatus newstatus)
    {
        refuelstatus = newstatus;
    }
    RefuelStatus RefuelStatus(void)
    {
        return refuelstatus;
    }
    void SetTanker(VU_ID tanker)
    {
        tankerId = tanker;
    }
    void SetTnkPosition(int pos)
    {
        tnkposition = pos;
    }
    VU_ID Tanker(void)
    {
        return tankerId;
    }
    void HelpRefuel(AircraftClass *tanker);
    Tpoint tankerRelPositioning; // JB 020311 respond to tanker "commands"
    unsigned long lastBoomCommand;

    ///////////////////////////////////////////////////////////////////////
    // Begin Wingman Stuff


    // Wingman Stuff
    float velocitySlope;
    float velocityIntercept;
    void SetLeader(SimBaseClass*);
    int IsMyWingman(SimBaseClass* testEntity);
    int IsMyWingman(VU_ID testId);
    SimBaseClass* MyWingman(void);
    SimBaseClass* flightLead;
    TransformMatrix threatAxis;
    int pointCounter;
    void FollowOrders(void);
    void CommandFlight(void);
    BOOL CommandTest(void);
    float mAzErrInt; // Azimuth error integrator

    //for tankers
    virtual int IsTanker(void)
    {
        return FALSE;
    }
    virtual void InitBoom(void) {};

public:
    int mLeadGearDown; //1 = true, 0 = false, -1 = uninitalized, -2 = waiting for message

    typedef enum AiActionModeTypes
    {
        AI_RTB,
        AI_LANDING,
        AI_FOLLOW_FORMATION,
        AI_ENGAGE_TARGET,
        AI_EXECUTE_MANEUVER,
        AI_USE_COMPLEX,
        AI_TOTAL_ACTION_TYPES
    };

    typedef enum AiSearchModeTypes
    {
        AI_SEARCH_FOR_TARGET,
        AI_MONITOR_TARGET,
        AI_FIXATE_ON_TARGET,
        AI_TOTAL_SEARCH_TYPES
    };

    typedef enum AiDesignatedTypes
    {
        AI_NO_DESIGNATED,
        AI_TARGET,
        AI_GROUP,
        AI_TOTAL_DESIGNATED_TYPES
    };

    typedef enum AiWeaponsAction
    {
        AI_WEAPONS_HOLD,
        AI_WEAPONS_FREE,
        AI_WEAPONS_ACTION_TOTAL
    };

    typedef enum AiThreatPosition
    {
        AI_THREAT_NONE,
        AI_THREAT_LEFT,
        AI_THREAT_RIGHT,
        AI_TOTAL_THREAT_POSITIONS
    };
    enum AiHint   // JPO some hints for what were doing.
    {
        AI_NOHINT, AI_TAKEOFF, AI_REJOIN, AI_ATTACK
    };

    enum AiTargetType   // 2002-03-04 ADDED BY S.G. Need to know what kind of target we're going after...
    {
        AI_NONE = FALSE, AI_AIR_TARGET, AI_GROUND_TARGET
    };

private:

    // Mode Arrays
    // 2002-03-04 MODIFIED BY S.G. Used to be type BOOL and now needs
    // 0, 1 or 2 so for best practice, make it an int
    int mpActionFlags[AI_TOTAL_ACTION_TYPES];
    BOOL mpSearchFlags[AI_TOTAL_SEARCH_TYPES];
    int mCurrentManeuver;

    // Assigned Target
    // SimObjectType* mpDesignatedTarget;
    VU_ID mDesignatedObject;
    int mFormation;
    // Marco Edit - for Current Formation
    int mCurFormation;
    BOOL mLeaderTookOff;

    // Mode Basis
    AiDesignatedTypes mDesignatedType;
    char mSearchDomain;
    AiWeaponsAction mWeaponsAction;
    AiWeaponsAction mSavedWeapons;

    // Saved mode Basis
    // AiDesignatedTypes mSaveDesignatedType;
    char mSaveSearchDomain;
    // AiWeaponsAction mSaveWeaponsAction;

    BOOL mInPositionFlag;

    float mHeadingOrdered;
    float mAltitudeOrdered;
    float mSpeedOrdered;

    int mPointCounter;
    float mpManeuverPoints[3][2];

    // Last time wingman gave a scheduled report
    VU_TIME mLastReportTime;
    SimObjectType* mpLastTargetPtr;

    // Formation Offsets
    BOOL mSplitFlight;
    float mFormRelativeAltitude;
    int mFormSide;
    float mFormLateralSpaceFactor;

    VU_TIME mNextPreflightAction; // JPO when to do the next action
    int mActionIndex; // what to do.

    // Breaking the flight
    void AiSplitFlight(int, VU_ID, int);
    void AiGlueFlight(int, VU_ID, int);

    // Targeting
    void AiRunTargetSelection(void);
    void AiSearchTargetList(VuEntity*);

    // Wingman Decision Fuctions
    void AiRunDecisionRoutines(void);
    void AiCheckManeuvers(void);
    void AiCheckFormation(void);
    void AiCheckEngage(void);
    void AiCheckRTB(void);
    void AiCheckLandTakeoff(void);
    void AiCheckForUnauthLand(VU_ID);

    // Wingman Actions
    void AiPerformManeuver(void);
    void AiFollowLead(void);

    // Wingman Monitor for targets
    void AiMonitorTargets();

    // Wingman Utility Functions
    void AiSaveWeaponState(void);
    void AiRestoreWeaponState(void);

    void AiSaveSetSearchDomain(char);
    void AiRestoreSearchDomain(void);

    void AiSetManeuver(int);
    void AiClearManeuver(void);

    // Commands that modify Action and Search States
    void AiClearLeadersSix(FalconWingmanMsg*);
    void AiCheckOwnSix(FalconWingmanMsg*);
    void AiEngageThreatAtSix(VU_ID);
    void AiBreakLeft(void);
    void AiBreakRight(void);
    void AiGoShooter(void);
    void AiGoCover(void);
    void AiSearchForTargets(char);
    void AiResumeFlightPlan(FalconWingmanMsg*);
    void AiRejoin(FalconWingmanMsg*, AiHint hint = AI_NOHINT);
    void AiSetRadarActive(FalconWingmanMsg*);
    void AiSetRadarStby(FalconWingmanMsg*);
    void AiBuddySpikeReact(FalconWingmanMsg*);
    void AiRaygun(FalconWingmanMsg*);

    void AiRTB(FalconWingmanMsg*);

    // Commands that modify the state basis
    void AiDesignateTarget(FalconWingmanMsg*);
    void AiDesignateGroup(FalconWingmanMsg*);
    void AiSetWeaponsAction(FalconWingmanMsg*, AiWeaponsAction);

    // Commands that modify formation
    void AiSetFormation(FalconWingmanMsg*);
    void AiKickout(FalconWingmanMsg*);
    void AiCloseup(FalconWingmanMsg*);
    void AiToggleSide(void);
    void AiIncreaseRelativeAltitude(void);
    void AiDecreaseRelativeAltitude(void);

    // Transient Commands
    void AiGiveBra(FalconWingmanMsg*);
    void AiGiveStatus(FalconWingmanMsg*);
    void AiGiveDamageReport(FalconWingmanMsg*);
    void AiGiveFuelStatus(FalconWingmanMsg*);
    void AiGiveWeaponsStatus(void);

    // Other Commands
    void AiRefuel(void);
    void AiPromote(void);
    void AiCheckInPositionCall(float trX, float trY, float trZ);
    void AiCheckPosition(void);
    void AiCheckFormStrip(void);
    void AiSmokeOn(FalconWingmanMsg*);
    void AiSmokeOff(FalconWingmanMsg*);
    void AiSendUnauthNotice(void);
    void AiHandleUnauthNotice(FalconWingmanMsg*);
    // void AiQueryLeadGear(void);
    // void AiHandleLeadGearUp(FalconWingmanMsg*);
    // void AiHandleLeadGearDown(FalconWingmanMsg*);
    void AiGlueWing(void);
    void AiSplitWing(void);
    void AiDropStores(FalconWingmanMsg*); // M.N. 2001-12-19
    void AiLightsOn(FalconWingmanMsg*); // MN 2002-02-11
    void AiLightsOff(FalconWingmanMsg*);
    void AiECMOn(FalconWingmanMsg*);
    void AiECMOff(FalconWingmanMsg*);
public:
    void AiCheckPlayerInPosition(void);
    void AiSetInPosition(void);
    // void AiHandleGearQuery(void);
private:

    // Execution for Maneuvers
    void AiInitSSOffset(FalconWingmanMsg*);
    void AiInitPince(FalconWingmanMsg*, int doSplit);
    void AiInitFlex(void);
    void AiInitChainsaw(FalconWingmanMsg*);
    void AiInitPosthole(FalconWingmanMsg*);
    void AiInitTrig(mlTrig*, mlTrig*);

    void AiExecBreakRL(void);
    void AiExecPosthole(void);
    void AiExecChainsaw(void);
    void AiExecPince(void);
    void AiExecFlex(void);
    void AiExecClearSix();
    void AiExecSearchForTargets(char);

    // End Wingman Stuff


#ifdef USE_SH_POOLS
public:
    // Overload new/delete to use a SmartHeap fixed size pool
    void *operator new(size_t size)
    {
        ShiAssert(size == sizeof(DigitalBrain));
        return MemAllocFS(pool);
    };
    void operator delete(void *mem)
    {
        if (mem) MemFreeFS(mem);
    };
    static void InitializeStorage()
    {
        pool = MemPoolInitFS(sizeof(DigitalBrain), 200, 0);
    };
    static void ReleaseStorage()
    {
        MemPoolFree(pool);
    };
    static MEM_POOL pool;
#endif

    // 2001-06-04 ADDED BY S.G. HELP FUNCTION TO SEARCH FOR A GROUND TARGET
    SimBaseClass *FindSimGroundTarget(CampBaseClass *targetGroup, int targetNumComponents, int startPos);
    // 2001-06-28 ADDED BY S.G. HELPER FUNCTION TO TEST IF IT'S A SEAD MISSION ON ITS MAIN TARGET OR NOT
    int IsNotMainTargetSEAD(void);
    // 2001-12-17 Added by M.N. To get a sim object from a divert target
    SimBaseClass *FindSimAirTarget(CampBaseClass *targetGroup, int targetNumComponents, int startPos);
    // 2002-01-29 ADDED BY S.G. To set the various target spot used to keep target deaggregated after the player issue an Attack target command
public:
    VuEntity* targetSpotWing;
    VuEntity* targetSpotElement;
    VuEntity* targetSpotFlight;
    FalconEntity *targetSpotWingTarget;
    FalconEntity *targetSpotElementTarget;
    FalconEntity *targetSpotFlightTarget;
    VU_TIME targetSpotWingTimer;
    VU_TIME targetSpotElementTimer;
    VU_TIME targetSpotFlightTimer;
    VU_TIME radarModeTest; // 2002-02-10 ADDED BY S.G.
    int tiebreaker; // JPO - for ground manuveur
    VU_ID escortFlightID; // 2002-02-27 ADDED BY S.G. Instead of calculating it everytime in BVR, let's find it here...
    AG_DOCTRINE GetAGDoctrine(void)
    {
        return agDoctrine;    // 2002-03-08 MN so leader can check if wingmen are still engaging a ground target
    }
    unsigned int moreFlags; // 2002-03-08 ADDED BY S.G. atcFlags ran out of bits...

    // Cobra help function to search for a JDAM ground target
    int FindJDAMGroundTarget(CampBaseClass *targetGroup, int targetNumComponents, int feaNo);
    SimBaseClass *FindJSOWGroundTarget(CampBaseClass *targetGroup, int targetNumComponents, int feaNo);
    /////////////////////////////////////////////////////////
};

extern int WingmanTable[4];

extern ACFormationData *acFormationData;
#endif /* _DIGI_H */

