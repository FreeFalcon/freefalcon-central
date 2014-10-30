#include "stdhdr.h"
#include "simobj.h"
#include "digi.h"
#include "simveh.h"
#include "mesg.h"
#include "object.h"
#include "MsgInc/WingmanMsg.h"
#include "MsgInc/ATCMsg.h"
#include "mission.h"
#include "unit.h"
#include "airframe.h"
#include "simdrive.h"
#include "object.h"
#include "classtbl.h"
#include "aircrft.h"
#include "falcsess.h"
#include "camp2sim.h"
#include "Graphics/Include/drawbsp.h"
#include "simfile.h"
#include "entity.h"
#include "atcbrain.h"
#include "Find.h"
#include "tankbrn.h"
#include "navsystem.h"
#include "package.h"
#include "flight.h"
#include "sms.h"

#ifdef USE_SH_POOLS
MEM_POOL DigitalBrain::pool;
extern MEM_POOL gReadInMemPool;
#endif
extern float g_fAGSlowMoverSpeed; // Cobra
extern bool g_bwoeir; // FRB

#define MANEUVER_DATA_FILE   "sim\\acdata\\brain\\mnvrdata.dat"
DigitalBrain::ManeuverChoiceTable DigitalBrain::maneuverData[DigitalBrain::NumMnvrClasses][DigitalBrain::NumMnvrClasses] = {0, 0, 0, -1, -1, -1};
DigitalBrain::ManeuverClassData DigitalBrain::maneuverClassData[DigitalBrain::NumMnvrClasses] = {0};
FalconEntity* SpikeCheck(AircraftClass* self, FalconEntity *byHim = NULL, int *data = NULL); // 2002-02-10 S.G.

int WingmanTable[] = {1, 0, 3, 2};


int DigitalBrain::IsMyWingman(SimBaseClass* testEntity)
{
    return self->GetCampaignObject()->GetComponentNumber(WingmanTable[self->vehicleInUnit]) == testEntity;
}

SimBaseClass* DigitalBrain::MyWingman(void)
{
    return self->GetCampaignObject()->GetComponentNumber(WingmanTable[self->vehicleInUnit]);
}

int DigitalBrain::IsMyWingman(VU_ID testId)
{
    SimBaseClass *testEntity;
    testEntity = self->GetCampaignObject()->GetComponentNumber(WingmanTable[self->vehicleInUnit]);

    if (testEntity and testEntity->Id() == testId)
        return TRUE;

    return FALSE;
}

DigitalBrain::DigitalBrain(AircraftClass *myPlatform, AirframeClass* myAf) : BaseBrain()
{
    rocketMnvr = RocketFlyToTgt;
    rocketTimer = 2 * 60; // two minutes for something to happen
    af = myAf;
    self = myPlatform;
    flightLead = NULL;
    targetPtr = NULL;
    targetList = NULL;
    isWing = FALSE; // JPO initialise
    mCurFormation = -1; // JPO
    SetLeader(self);
    onStation = NotThereYet;
    mLastOrderTime = 0;
    wvrCurrTactic = WVR_NONE;
    wvrPrevTactic = WVR_NONE;
    wvrTacticTimer = 0;
    lastAta = 0;
    engagementTimer = 0;
    //TJL 11/08/03 initialize and set here so it stays set
    bugoutTimer = 0;
    //RAS-3Oct04-Timer for Vector Type Calls (i.e. ...left base or right base)
    lastVectorTypeCall = 0;

    // ADDED BY S.G. SO DIGI KNOWS
    // IT HASN'T LAUNCHED A MISSILE YET (UNUSED BEFORE - NAME IS MEANINGLESS BUT WHAT THE HECK)
    missileFiredEntity = NULL;
    missileFiredTime = 0;

    // sfr: using this instead of doing all by hand. remove old code if reliable
#define NEW_INIT 0
#if NEW_INIT
    ResetATC();
#else
    rwIndex = 0;
#endif

    if (self->OnGround())
    {
        airbase = self->TakeoffAirbase(); // we take off from this base
    }
    else
    {
        airbase = self->HomeAirbase(); // original code.
    }

    if (airbase == FalconNullId)
    {
        GridIndex x, y;
        vector pos;
        pos.x = self->XPos();
        pos.y = self->YPos();
        //find nearest airbase
        ConvertSimToGrid(&pos, &x, &y);
        Objective obj = FindNearestFriendlyAirbase(self->GetTeam(), x, y);

        //if(obj)
        // sfr: @todo remove JB check
        if (obj and not F4IsBadReadPtr(obj, sizeof(ObjectiveClass)))
        {
            // JB 010326 CTD
            airbase = obj->Id();
        }
    }

#if not NEW_INIT
    atcstatus = noATC;
    curTaxiPoint = 0;
    rwtime = 0;
    desiredSpeed = 0.0F;
    turnDist = 0.0F;
    atcFlags = 0;
    waittimer = 0;
#endif
    distAirbase = 0.0F;
    updateTime = 0;
    createTime = SimLibElapsedTime;

    if (self->OnGround())
    {
        SetATCFlag(Landed);
        SetATCFlag(RequestTakeoff);
        ClearATCFlag(PermitTakeoff);
        SetATCFlag(StopPlane);
    }
    else
    {
        SetATCFlag(PermitTakeoff);
        ClearATCFlag(Landed);
        ClearATCFlag(RequestTakeoff);
    }

    autoThrottle = 0.0F;
    velocitySlope = 0.0F;

    tankerId = FalconNullId;
    tnkposition = -1;
    refuelstatus = refNoTanker;
    tankerRelPositioning.x = 0.0F;
    tankerRelPositioning.y = 0.0F;
    tankerRelPositioning.z = -3.0F;
    lastBoomCommand = 0;

    Package package;
    Flight flight;
    escortFlightID = FalconNullId; // 2002-02-27 S.G.

    flight = (Flight)self->GetCampaignObject();

    if (flight)
    {
        package = flight->GetUnitPackage();

        if (package)
        {
            tankerId = package->GetTanker();

            // 2002-02-27 ADDED BY S.G.
            // Lets save our escort flight pointer if we have one.
            // It's going to come handy in BvrEngageCheck...
            for (UnitClass *un = package->GetFirstUnitElement(); un; un = package->GetNextUnitElement())
            {
                if (un->IsFlight())
                {
                    if (((FlightClass *)un)->GetUnitMission() == AMIS_ESCORT)
                        escortFlightID = un->Id(); // We got one
                }
            }

            // END OF ADDED SECTION 2002-02-27
        }
    }


    SetTrackPoint(self->XPos(), self->YPos(), self->ZPos());

    agDoctrine = AGD_NONE;
    agImprovise = FALSE;
    nextAGTargetTime = SimLibElapsedTime + (5 * SEC_TO_MSEC);

    // sfr: commenting this out. Causing AI to not fire missiles.
    // RV - Biker - Init this at 5 hours from now
    // missileShotTimer = SimLibElapsedTime + 5 * 60 * 60 * SEC_TO_MSEC;

    missileShotTimer = SimLibElapsedTime; // Cobra

    curMissile       = NULL;
    sentWingAGAttack = AG_ORDER_NONE;
    nextAttackCommandToSend = 0; // 2002-01-20 ADED BY S.G. Make sure it's initialized to something we can handle
    jinkTime         = 0;
    jammertime  = FALSE;//ME123
    holdlongrangeshot = 0; //0.0f;
    cornerSpeed      = af->CornerVcas();
    rangeToIP        = FLT_MAX;
    madeAGPass       = FALSE;
    AGattackAlt = self->GetA2GDumbLDAlt();
    // 2001-05-21 ADDED BY S.G. INIT waitingForShot SO IT'S NOT GARBAGE TO START WITH
    waitingForShot = 0;
    // END OF ADDED SECTION
    pastAta = 0;
    pastPipperAta = 0;


    // 2001-10-12 ADDED BY S.G. INIT gndTargetHistory[2] SO IT'S NOT GARBAGE TO START WITH
    gndTargetHistory[0] = gndTargetHistory[1] = NULL;
    // END OF ADDED SECTION

    // WingAi inits
    // Cobra - For A2G attack profiles
    if (af->CornerVcas() <= g_fAGSlowMoverSpeed)
    {
        slowMover = TRUE;
    }
    else
    {
        slowMover = FALSE;
    }

    if (self->OnGround())
    {
        mpActionFlags[AI_FOLLOW_FORMATION] = FALSE;
    }
    else
    {
        mpActionFlags[AI_FOLLOW_FORMATION] = TRUE;
    }

    mLeaderTookOff = FALSE;
    mpActionFlags[AI_ENGAGE_TARGET] = AI_NONE;
    mpActionFlags[AI_EXECUTE_MANEUVER] = FALSE;
    mpActionFlags[AI_USE_COMPLEX]          = FALSE;
    mpActionFlags[AI_RTB]    = FALSE;
    mpActionFlags[AI_LANDING]          = FALSE;

    mpSearchFlags[AI_SEARCH_FOR_TARGET] = FALSE;
    mpSearchFlags[AI_MONITOR_TARGET] = FALSE;
    mpSearchFlags[AI_FIXATE_ON_TARGET] = FALSE;

    mCurrentManeuver = FalconWingmanMsg::WMTotalMsg;
    mDesignatedObject = FalconNullId;
    mFormation = acFormationData->FindFormation(FalconWingmanMsg::WMWedge);
    mDesignatedType = AI_NO_DESIGNATED;
    mSearchDomain = DOMAIN_ABSTRACT;
    mWeaponsAction = AI_WEAPONS_HOLD;
    // RV - Biker - Better init this too
    mSavedWeapons = AI_WEAPONS_HOLD;
    mInPositionFlag = FALSE;

    mFormRelativeAltitude = 0.0F;
    mFormSide = 1;
    mFormLateralSpaceFactor = 1.0F;
    mSplitFlight = FALSE;

    mLastReportTime = 0;
    mpLastTargetPtr = NULL;

    mAzErrInt = 0.0F;
    mLeadGearDown = -1;
    groundAvoidNeeded = FALSE;

    groundTargetPtr = NULL;
    airtargetPtr = NULL;
    preThreatPtr = NULL;
    threatPtr = NULL;
    threatTimer = 0.0f;

    pStick = 0.0F;
    rStick = 0.0F;
    throtl = 0.0F; // jpo - from 1 - start at 0
    yPedal = 0.0F;

    // Modes and rules
    maxGs = af->MaxGs();
    maxGs = max(maxGs, 2.5F);

    curMode = WaypointMode;
    lastMode = WaypointMode;
    nextMode = NoMode;
    trackMode = 0;
    // Init Target data
    targetPtr = NULL;
    lastTarget = NULL;

    // Setup BVR Stuff
    bvrCurrTactic = BvrNoIntercept;
    bvrPrevTactic = BvrNoIntercept;
    bvrTacticTimer = 0;
    spiked = 0;
    MAR = 0.0f;
    TGTMAR = 0.0f;
    DOR = 0.0f;
    Isflightlead = false;
    IsElementlead = false;
    bvractionstep = 0;
    bvrCurrProfile = Pnone;
    offsetdir = 0;
    spikeseconds = 0;
    spikesecondselement = 0;
    spikeseconds = 0;
    missilelasttime = NULL;
    spiketframetime  = NULL;
    lastspikeent = NULL;
    spiketframetimewingie  = NULL;
    lastspikeentwingie = NULL;
    gammaHoldIError = 0;
    reactiont = 0;
    // start out assuming we at least have guns
    maxAAWpnRange = 6000.0f;

    // once we've bugged out of WVR our tactics will change
    buggedOut = FALSE;

    // edg missionType and missionClass are now in the digi class since
    // they're used a lot (rather than being locals everywhere)
    missionType = ((UnitClass*)(self->GetCampaignObject()))->GetUnitMission();

    switch (missionType)
    {
        case AMIS_BARCAP:
        case AMIS_BARCAP2:
            // 2002-03-05 ADDED BY S.G.
            //These need to attack bombers as well and if OnSweep isn't set, SensorFusion will ignore them
            maxEngageRange = 45.0F * NM_TO_FT;//me123 from 20
            missionClass = AAMission;
            SetATCFlag(OnSweep);
            break;

            // END OF ADDED SECTION 2002-03-05
        case AMIS_HAVCAP:
        case AMIS_TARCAP:
        case AMIS_RESCAP:
        case AMIS_AMBUSHCAP:
        case AMIS_NONE:
            maxEngageRange = 45.0F * NM_TO_FT;//me123 from 20
            missionClass = AAMission;
            ClearATCFlag(OnSweep);
            break;

        case AMIS_SWEEP:
            maxEngageRange = 60.0F * NM_TO_FT;//me123 from 80
            missionClass = AAMission;
            SetATCFlag(OnSweep);
            break;

        case AMIS_ALERT:
        case AMIS_INTERCEPT:
        case AMIS_ESCORT:
            maxEngageRange = 45.0F * NM_TO_FT;//me123 from 30
            missionClass = AAMission;
            ClearATCFlag(OnSweep);
            break;

        case AMIS_AIRLIFT:
            maxEngageRange = 40.0F * NM_TO_FT;//me123 from 10 bvrengage will now crank beam and drag defensive
            missionClass = AirliftMission;
            ClearATCFlag(OnSweep);
            break;

        case AMIS_TANKER:
        case AMIS_AWACS:
        case AMIS_JSTAR:
        case AMIS_ECM:
        case AMIS_SAR:
            maxEngageRange = 40.0F * NM_TO_FT;//me123 from 10bvrengage will now crank beam and drag defensive
            missionClass = SupportMission;
            ClearATCFlag(OnSweep);
            break;

        default:
            maxEngageRange = 40.0F * NM_TO_FT;//me123 from 10
            missionClass = AGMission;
            ClearATCFlag(OnSweep);
            // Engage sooner after mission complete
            // JB 010719 missionComplete has not been initialized yet.
            //if (missionComplete)
            //   maxEngageRange *= 1.2F;//me123 from 2.0
            break;
    }

    // 2002-02-27 ADDED BY S.G.
    // What about flights on egress that deaggregates?
    // Look up their Eval flags and set missionComplete accordingly...
    if (((FlightClass *)self->GetCampaignObject())->GetEvalFlags() bitand FEVAL_GOT_TO_TARGET)
    {
        missionComplete = TRUE;
    }
    else
    {
        // END OF ADDED SECTION 2002-02-27
        missionComplete = FALSE;
    }

    // Check for trainable guns
    if (self->Sms->HasTrainable())
    {
        SetATCFlag(HasTrainable);
    }

    moreFlags = 0; // 2002-03-08 ADDED BY S.G. (Before SelectGroundWeapon which will query it

    // Check for AG weapons
    if (missionClass == AGMission)
    {
        SelectGroundWeapon();
    }

    // Check for AA Weapons
    SetATCFlag(AceGunsEngage);
    // JPO - for startup actions.
    mActionIndex = 0;
    mNextPreflightAction = 0;
    lastGndCampTarget = NULL;

    destRoll = 0;
    destPitch = 0;
    currAlt = 0;

    // 2002-01-29 ADDED BY S.G. Init our targetSpot and associated
    targetSpotFlight = NULL;
    targetSpotFlightTarget = NULL;
    targetSpotFlightTimer = 0;
    targetSpotElement = NULL;
    targetSpotElementTarget = NULL;
    targetSpotElementTimer = 0;
    targetSpotWing = NULL;
    targetSpotWingTarget = NULL;
    targetSpotWingTimer = 0;
    // END OF ADDED SECTION
    radarModeTest = 0; // 2002-02-10 ADDED BY S.G.
    // 2002-02-24 MN
    pullupTimer = 0;//0.0f;
    tiebreaker = 0;
    nextFuelCheck = SimLibElapsedTime; // 2002-03-02 MN fix airbase check NOT 0 - set the time here.. aargh...
    airbasediverted = 0;
    //agmergeTimer = SimLibElapsedTime * 60 * SEC_TO_MSEC;
    // RV - Biker - uint should not be -1
    //agmergeTimer = -1; // Cobra - -1 = needs initializing
    agmergeTimer = 0;
    mergeTimer = 0;
    visDetectTimer = SimLibElapsedTime;//Cobra
    detRWR = 0;
    detRAD = 0;
    detVIS = 0;
    targetTimer = 0;//Cobra
    radModeSelect = 3;
}

DigitalBrain::~DigitalBrain(void)
{
    SetGroundTarget(NULL);
    SetTarget(NULL);

    if (preThreatPtr)
    {
        preThreatPtr->Release();
        preThreatPtr = NULL;
    }

    // 2002-01-29 ADDED BY S.G.
    // Clear our targetSpot and associated if we have created one (see AiSendPlayerCommand)
    if (targetSpotFlight)
    {
        // Kill the camera and clear out everything associated with this targetSpot
        // sfr: cleanup camera mess
        FalconLocalSession->RemoveCamera(targetSpotFlight);
        //for (int i = 0; i < FalconLocalSession->CameraCount(); i++) {
        // if (FalconLocalSession->GetCameraEntity(i) == targetSpotFlight) {
        // FalconLocalSession->RemoveCamera(targetSpotFlight);
        // break;
        // }
        //}
        vuDatabase->Remove(targetSpotFlight); // Takes care of deleting the allocated memory and the driver allocation as well.

        if (targetSpotFlightTarget)
        {
            // 2002-03-07 ADDED BY S.G. In case it's NULL. Shouldn't happen but happened
            VuDeReferenceEntity(targetSpotFlightTarget);
        }

        targetSpotFlight = NULL;
        targetSpotFlightTarget = NULL;
        targetSpotFlightTimer = 0;
    }

    if (targetSpotElement)
    {
        // Kill the camera and clear out everything associated with this targetSpot
        // sfr: cleanup camera mess
        FalconLocalSession->RemoveCamera(targetSpotElement);
        //for (int i = 0; i < FalconLocalSession->CameraCount(); i++) {
        // if (FalconLocalSession->GetCameraEntity(i) == targetSpotElement) {
        // FalconLocalSession->RemoveCamera(targetSpotElement);
        // break;
        // }
        //}
        vuDatabase->Remove(targetSpotElement);  // Takes care of deleting the allocated memory and the driver allocation as well.

        if (targetSpotElementTarget)  // 2002-03-07 ADDED BY S.G. In case it's NULL. Shouldn't happen but happened
        {
            VuDeReferenceEntity(targetSpotElementTarget);
        }

        targetSpotElement = NULL;
        targetSpotElementTarget = NULL;
        targetSpotElementTimer = 0;
    }

    if (targetSpotWing)
    {
        // Kill the camera and clear out everything associated with this targetSpot
        // sfr: cleanup camera mess
        FalconLocalSession->RemoveCamera(targetSpotWing);
        //for (int i = 0; i < FalconLocalSession->CameraCount(); i++) {
        // if (FalconLocalSession->GetCameraEntity(i) == targetSpotWing) {
        // FalconLocalSession->RemoveCamera(targetSpotWing);
        // break;
        // }
        //}
        vuDatabase->Remove(targetSpotWing);  // Takes care of deleting the allocated memory and the driver allocation as well.

        if (targetSpotWingTarget)  // 2002-03-07 ADDED BY S.G. In case it's NULL. Shouldn't happen but happened
        {
            VuDeReferenceEntity(targetSpotWingTarget);
        }

        targetSpotWing = NULL;
        targetSpotWingTarget = NULL;
        targetSpotWingTimer = 0;
    }

    // END OF ADDED SECTION 2002-01-29

    // 2002-03-13 ADDED BY S.G. Must dereference it or it will cause memory leak...
    if (missileFiredEntity)
    {
        VuDeReferenceEntity(missileFiredEntity);
    }

    missileFiredEntity = NULL;
}

void DigitalBrain::FrameExec(SimObjectType* curTargetList, SimObjectType* curTarget)
{
    maxGs = af->MaxGs();
    maxGs = max(maxGs, 2.5F);

    // 2002-03-15 MN if we've flamed out
    // and our speed is below minvcas, put the wheels out so that there is a chance to land
    if (self->af->Fuel() <= 0.0F and self->af->vcas < self->af->MinVcas())
    {
        // Set Landed flag now, so that RegroupAircraft can be called in Eom.cpp -
        // have the maintenance crew tow us back to the airbase ;-)
        //TJL 02/19/04 Flame out = Eject for Digital pilots
        //SetATCFlag(Landed);
        //af->gearHandle = 1.0F;

        self->Eject();
        rStick = -0.3f;//Roll while the plane dies
    }
    else
    {
        // make sure the wheels are up after takeoff
        if (self->curWaypoint and self->curWaypoint->GetWPAction() not_eq WP_TAKEOFF)
        {
            af->gearHandle = -1.0F;
        }
        else if ( not self->OnGround())
        {
            //Cobra stop Naval aircraft flying around with gear down
            af->gearHandle = -1.0F;
        }
    }

    // assume we're not going to be firing in this frame
    ClearFlag(MslFireFlag bitor GunFireFlag);

    // check to see if our leader is dead and if not set leader to next in
    // flight (and/or ourself)
    CheckLead();

    // FRB - Keep radar sweeping while in CombatAP
    if ((self->IsPlayer()) and (g_bwoeir))
        self->targetUpdateRate = (VU_TIME)(.01 * SEC_TO_MSEC);
    else
        self->targetUpdateRate = (VU_TIME)(5 * SEC_TO_MSEC);

    // Find a threat/target
    DoTargeting();

    // Make Decisions
    SetCurrentTactic();

    // Set the controls
    Actions();

    // RV - Biker - Enable shooting missiles if flight lead is AI
    if (flightLead and not flightLead->IsPlayer() and missileShotTimer >= SimLibElapsedTime + 4.9 * 60 * 60 * SEC_TO_MSEC)
    {
        missileShotTimer = SimLibElapsedTime;
    }

    // has the target changed?
    if (targetPtr not_eq lastTarget)
    {
        lastTarget = targetPtr;
        ataddot = 10.0F;
        rangeddot = 10.0F;
    }

    // edg: check stick settings for bad values
    if (rStick < -1.0f)
        rStick = -1.0f;
    else if (rStick > 1.0f)
        rStick = 1.0f;

    if (pStick < -1.0f)
        pStick = -1.0f;
    else if (pStick > 1.0f)
        pStick = 1.0f;

    //me123 unload if roll input is 1 (to rool faster and eleveate the FF bug)
    if (fabs(rStick) > 0.9f and groundAvoidNeeded == FALSE)
        pStick = 0.0f;

    if (throtl < 0.0f)
        throtl = 0.0f;
    else if (throtl > 1.5f)
        throtl = 1.5f;

    // RV - Biker - Don't allow AB when low on fuel or on ground
    if (IsSetATC(SaidFumes) or IsSetATC(SaidFlameout) or (self->OnGround() and GetCurrentMode() not_eq TakeoffMode))
    {
        throtl = min(1.0f, throtl);
    }
}
void DigitalBrain::Sleep(void)
{
    SetLeader(NULL);
    ClearTarget();

    // NOTE: This is only legal if the platorms target list is already cleared.
    // Currently, SimVehicle::Sleep call SimMover::Sleep which clears the list,
    // then it calls theBrain::Sleep.  As long as this doesn't change this will
    // not cause a leak.
    if (groundTargetPtr)
    {
#ifdef DEBUG

        if (groundTargetPtr->prev or groundTargetPtr->next)
        {
            MonoPrint("Ground target still in list at sleep\n");
        }

#endif
        groundTargetPtr->prev = NULL;
        groundTargetPtr->next = NULL;
    }

    SetGroundTarget(NULL);
}

void DigitalBrain::SetLead(int flag)
{
    if (flag == TRUE)
    {
        isWing = FALSE;
        SetLeader(self);
    }
    else
    {
        isWing = self->GetCampaignObject()->GetComponentIndex(self);
        SetLeader(self->GetCampaignObject()->GetComponentLead());
    }
}

// Make sure our leader hasn't gone away without telling us.
void DigitalBrain::CheckLead(void)
{
    SimBaseClass *pobj;
    SimBaseClass* newLead = NULL;

    BOOL done = FALSE;
    int i = 0;

    if (flightLead and 
        flightLead->VuState() == VU_MEM_ACTIVE and 
 not flightLead->IsDead()
       )
    {
        return;
    }

    {
        VuListIterator cit(self->GetCampaignObject()->GetComponents());
        pobj = (SimBaseClass*)cit.GetFirst();

        while ( not done)
        {
            if (pobj and pobj->VuState() == VU_MEM_ACTIVE and not pobj->IsDead())
            {
                done = TRUE;
                newLead = pobj;
            }
            else if (i > 3)
            {
                done = TRUE;
                newLead = self;
            }

            ++i;
            pobj = (SimBaseClass*)cit.GetNext();
        }
    }

    SetLeader(newLead);
}

void DigitalBrain::SetLeader(SimBaseClass* newLead)
{
    // edg: I've encountered some over-referencing problems that I think
    // is associated with flight lead setting.  So what I put in was a
    // check for self and not doing the reference -- shouldn;t be needed in
    // this case, right?
    if (flightLead not_eq newLead)
    {
        if (flightLead and flightLead not_eq self)
        {
            VuDeReferenceEntity(flightLead);
        }

        flightLead = newLead;

        if (flightLead and flightLead not_eq self)
        {
            VuReferenceEntity(flightLead);
        }

        // edg: what a confusing mess... Why do we have SetLead and
        // SetLeader?
        // anyway, overreferencing has been occurring on Sleep(), when
        // SetLeader( NULL ) is called.  We then called SetLead() below,
        // which resulted in a new flight lead
        // check for NULL flight lead and just return
        if (flightLead not_eq NULL)
            SetLead(flightLead == self ? TRUE : FALSE);
    }
}

void DigitalBrain::JoinFlight(void)
{
    SimBaseClass* newLead = self->GetCampaignObject()->GetComponentLead();

    if (newLead == self)
    {
        SetLead(TRUE);
    }
    else
    {
        SetLead(FALSE);
    }
}

void DigitalBrain::ReadManeuverData(void)
{
    SimlibFileClass *mnvrFile;
    char fileType;

    // Open formation file
    mnvrFile = SimlibFileClass::Open(MANEUVER_DATA_FILE, SIMLIB_READ);
    F4Assert(mnvrFile);
    mnvrFile->Read(&fileType, 1);

    // ASCII file
    // Either change the file structure, or use this compromised reading method.
    // Comments at the top of the file are preventing proper detection, thus '#'
    if (fileType == '#')
    {
        for (int i = 0; i < NumMnvrClasses; ++i)
        {
            // read a line to get rid of the comments
            char temp[3000];
            mnvrFile->ReadLine(temp, 3000);

            // Get the limits for this Maneuver type
            sscanf(mnvrFile->GetNext(), "%x", &maneuverClassData[i]);

            for (int j = 0; j < NumMnvrClasses; ++j)
            {
                maneuverData[i][j].numIntercepts = (char)atoi(mnvrFile->GetNext());

                if (maneuverData[i][j].numIntercepts)
                {
                    maneuverData[i][j].intercept =
#ifdef USE_SH_POOLS
                        (BVRInterceptType*)MemAllocPtr(
                            gReadInMemPool,
                            sizeof(BVRInterceptType) *maneuverData[i][j].numIntercepts,
                            0
                        );
#else
                        new BVRInterceptType[maneuverData[i][j].numIntercepts];
#endif
                }
                else maneuverData[i][j].intercept = NULL;

                maneuverData[i][j].numMerges = (char)atoi(mnvrFile->GetNext());

                if (maneuverData[i][j].numMerges)
                {
                    maneuverData[i][j].merge =
#ifdef USE_SH_POOLS
                        (WVRMergeManeuverType*)MemAllocPtr(
                            gReadInMemPool,
                            sizeof(WVRMergeManeuverType) *maneuverData[i][j].numMerges,
                            0
                        );
#else
                        new WVRMergeManeuverType[maneuverData[i][j].numMerges];
#endif
                }
                else maneuverData[i][j].merge = NULL;

                maneuverData[i][j].numReacts = (char)atoi(mnvrFile->GetNext());

                if (maneuverData[i][j].numReacts)
                {
                    maneuverData[i][j].spikeReact =
#ifdef USE_SH_POOLS
                        (SpikeReactionType*)MemAllocPtr(
                            gReadInMemPool,
                            sizeof(SpikeReactionType) *maneuverData[i][j].numReacts,
                            0
                        );
#else
                        new SpikeReactionType[maneuverData[i][j].numReacts];
#endif
                }
                else maneuverData[i][j].spikeReact = NULL;

                for (int k = 0; k < maneuverData[i][j].numIntercepts; ++k)
                    maneuverData[i][j].intercept[k] =
                        (BVRInterceptType)(atoi(mnvrFile->GetNext()) - 1);

                for (int k = 0; k < maneuverData[i][j].numMerges; ++k)
                    maneuverData[i][j].merge[k] =
                        (WVRMergeManeuverType)(atoi(mnvrFile->GetNext()) - 1);

                for (int k = 0; k < maneuverData[i][j].numReacts; ++k)
                    maneuverData[i][j].spikeReact[k] =
                        (SpikeReactionType)(atoi(mnvrFile->GetNext()) - 1);
            }
        }
    }
    // Allow binary, but otherwise throw a warning
    else if (fileType not_eq 'B')
        ShiWarning("Bad Maneuver Data File Format");

    mnvrFile->Close();
    delete mnvrFile;
    mnvrFile = NULL;
}

void DigitalBrain::FreeManeuverData(void)
{
    int i, j;

    for (i = 0; i < DigitalBrain::NumMnvrClasses; i++)
    {
        for (j = 0; j < DigitalBrain::NumMnvrClasses; j++)
        {
            delete maneuverData[i][j].intercept;
            delete maneuverData[i][j].merge;
            delete maneuverData[i][j].spikeReact;
            maneuverData[i][j].intercept  = NULL;
            maneuverData[i][j].merge      = NULL;
            maneuverData[i][j].spikeReact = NULL;
        }
    }

}

void DigitalBrain::GetTrackPoint(float &x, float &y, float &z)
{
    x = trackX;
    y = trackY;
    z = trackZ;
}
// sfr: inlined
/*
void DigitalBrain::SetTrackPoint(float x, float y)
{
 trackX = x;
 trackY = y;
}

void DigitalBrain::SetTrackPoint(float x, float y, float z)
{
 SetTrackPoint(x, y);
 trackZ = z;
}
*/

void DigitalBrain::SetTrackPoint(SimObjectType *object)
{
    SetTrackPoint(object->BaseData()->XPos(), object->BaseData()->YPos(), object->BaseData()->ZPos());
}


void DigitalBrain::SetRunwayInfo(VU_ID Airbase, int rwindex, unsigned long time)
{

    airbase = Airbase;
    rwIndex = rwindex;
    rwtime = time;

    // MD -- 20040605: make this a noop now for players -- the ILS info is set from
    // TACAN functions now.
    //if(self == SimDriver.GetPlayerEntity()) { // vwf
    // gNavigationSys->SetIlsData(airbase, rwIndex);
    //}
}

void DigitalBrain::ReSetLabel(SimBaseClass* theObject)
{
    Falcon4EntityClassType *classPtr = (Falcon4EntityClassType*)theObject->EntityType();
    CampEntity campObj;
    char label[40] = {0};
    long labelColor = 0xff0000ff;

    if ( not theObject->IsExploding() and not theObject->IsDead())
    {
        if (classPtr->dataType == DTYPE_VEHICLE)
        {
            FlightClass *flight;
            flight = FalconLocalSession->GetPlayerFlight();
            campObj = theObject->GetCampaignObject();

            if (campObj and campObj->IsFlight() /* and not campObj->IsAggregate() and campObj->InPackage()*/
                // 2001-10-31 M.N. show flight names of our team
               and flight and flight->GetTeam() == campObj->GetTeam())
            {
                char temp[40];
                GetCallsign(((Flight)campObj)->callsign_id, ((Flight)campObj)->callsign_num, temp);
                sprintf(label, "%s%d", temp, ((SimVehicleClass*)theObject)->vehicleInUnit + 1);
            }
            else
                sprintf(label, "%s", ((VehicleClassDataType*)(classPtr->dataPtr))->Name);
        }
        else if (classPtr->dataType == DTYPE_WEAPON)
            sprintf(label, "%s", ((WeaponClassDataType*)(classPtr->dataPtr))->Name);
    }

    // Change the label to a player, if there is one
    if (theObject->IsSetFalcFlag(FEC_HASPLAYERS))
    {
        // Find the player's callsign
        VuSessionsIterator sessionWalker(FalconLocalGame);
        FalconSessionEntity *session;

        session = (FalconSessionEntity*)sessionWalker.GetFirst();

        while (session)
        {
            if (session->GetPlayerEntity() == theObject)
                sprintf(label, "%s", session->GetPlayerCallsign());

            session = (FalconSessionEntity*)sessionWalker.GetNext();
        }
    }

    // Change the label color by team color
    //ShiAssert(TeamInfo[theObject->GetTeam()]);


    // KCK: This uses the UI's colors. For a while these didn't work well in Sim
    // They may be ok now, though - KCK: As of 10/25, still looked bad
    // labelColor = TeamColorList[TeamInfo[theObject->GetTeam()]->GetColor()];

    if (theObject->drawPointer)
        theObject->drawPointer->SetLabel(label, ((DrawableBSP*)theObject->drawPointer)->LabelColor());

}
