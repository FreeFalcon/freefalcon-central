#include "Graphics/Include/drawbsp.h"
#include "Graphics/Include/drawsgmt.h"
#include "Graphics/Include/rviewpnt.h"
#include "stdhdr.h"
#include "Classtbl.h"
#include "sensclas.h"
#include "eyeball.h"
#include "irst.h"
#include "initdata.h"
#include "mavdisp.h"
#include "object.h"
#include "falcmesg.h"
#include "otwdrive.h"
#include "MsgInc/DamageMsg.h"
#include "campBase.h"
#include "simdrive.h"
#include "simveh.h"
#include "sfx.h"
#include "wpndef.h"
#include "fsound.h"
#include "soundfx.h"
#include "fakerand.h"
#include "radarMissile.h"
#include "BeamRider.h"
#include "acmi/src/include/acmirec.h"
#include "falcsess.h"
#include "camp2sim.h"
#include "playerop.h"
#include "Graphics/Include/terrtex.h"
#include "persist.h"
#include "campList.h"
#include "HarmSeeker.h"
#include "ground.h"
#include "battalion.h"
#include "navunit.h"
#include "missdata.h"
#include "missile.h"
#include "team.h"
#include "profiler.h" // MLR 5/21/2004 - 

// Marco Edit for AIM9s in Slave mode
#include "aircrft.h"
#include "fcc.h"
#include "MsgInc/RadioChatterMsg.h"
#include "flight.h"

// Gilman weapon count hack
extern int gNumWeaponsInAir;
// End gilman hack

ACMIMissilePositionRecord misPos;
void CalcTransformMatrix(SimBaseClass* theObject);

#ifdef USE_SH_POOLS
MEM_POOL MissileClass::pool;
MEM_POOL MissileInFlightData::pool;
#endif

extern int g_nMissileFix;

/*
** Constructor for MissileInFlightData
*/
MissileInFlightData::MissileInFlightData(void)
{
    // Gains
    kp01 = kp02 = kp03 = kp04 = kp05 = kp06 = kp07 = 0.0f;
    tp01 = tp02 = tp03 = tp04 = 0.0f;
    wp01 = 0.0f;
    zp01 = 0.0f;
    ky01 = ky02 = ky03 = ky04 = ky05 = ky06 = ky07 = 0.0f;
    ty01 = ty02 = ty03 = ty04 = 0.0f;
    wy01 = 0.0f;
    zy01 = 0.0f;

    // Geometry
    alpdot = betdot = 0.0f;
    gamma = sigma = mu = 0.0f;

    // Guidance

    // State
    burnIndex = 0;
    rstab = qstab = 0.0f;
    e1 = e2 = e3 = e4 = 0.0f;

    // Save Arrays
    oldalp = oldalpdt = oldbet = oldbetdt = 0;

    // Aero data
    lastmach = lastalpha = 0;
    qsom = qovt = qbar = 0.0f;

    // Accels
    xaero = yaero = zaero = 0.0f;
    xsaero = ysaero = zsaero = 0.0f;
    xwaero = ywaero = zwaero = 0.0f;
    xprop = yprop = zprop = 0.0f;
    xsprop = ysprop = zsprop = 0.0f;
    xwprop = ywprop = zwprop = 0.0f;
    nxcgw = nycgw = nzcgw = 0.0f;
    nxcgb = nycgb = nzcgb = 0.0f;
    nxcgs = nycgs = nzcgs = 0.0f;

    clalph = cybeta = 0.0f;

    // Closest Approach
    lastCMDeployed = 0;
    stage2gone = false;
}

MissileInFlightData::~MissileInFlightData()
{
}

MissileClass::MissileClass(VU_BYTE** stream, long *rem) : SimWeaponClass(stream, rem)
{
    InitLocalData();
}

MissileClass::MissileClass(FILE* filePtr) : SimWeaponClass(filePtr)
{
    InitLocalData();
}

MissileClass::MissileClass(int type) : SimWeaponClass(type)
{
    InitLocalData();
}

MissileClass::~MissileClass(void)
{
    CleanupLocalData();
}

void MissileClass::InitData()
{
    SimWeaponClass::InitData();
    InitLocalData();
}

void MissileClass::InitLocalData()
{
    ifd = NULL;
    TrailId = Trail = 0;
    engGlow = NULL;
    groundGlow = NULL;
    engGlowBSP1 = NULL;
    runTime = 0.0F;
    GuidenceTime = 0.0f;
    slaveTgt = NULL;
    targetPtr = NULL;
    targetList = NULL;
    display = NULL;
    isCaged = 0;
    isSpot = 0;
    isSlave = 0;
    isTD = 0;
    alpha = alphat = beta = 0;
    x = 0.0f;
    y = 0.0f;
    z = 0.0f;
    alt = 0;
    xdot = 0.0f;
    ydot = 0.0f;
    zdot = 0.0f;
    psi = 0.0f;
    theta = 0.0f;
    phi = 0.0f;
    groundZ = 0;
    weight = 0;
    wprop = 0;
    mass = 0;
    m0 = mp0 = mprop = 0;
    aeroData = NULL;
    inputData = NULL;
    rangeData = NULL;
    engineData = NULL;
    auxData = NULL; // JPO
    flags = 0;
    runTime = 0;
    vt = 0.0f;
    vtdot = 0.0f;
    timpct = 100.0F;
    p = q = r = 0.0f;
    ricept = FLT_MAX;
    ata = 0.0f;
    gimbal = 0.0f;
    gimdot = 0.0f;
    ataerr = 0.0f;
    targetX = -1.0F;
    targetY = -1.0F;
    targetZ = -1.0F;
    targetDX = 0.0F;
    targetDY = 0.0F;
    targetDZ = 0.0F;
    lastRmaxAta = 0;
    lastRmaxAlt = lastRmaxVt = 0;
    initAz = initEl = 0;

    if (IsLocal())
    {
        SetVuPosition();
    }

    //MI
    Covered = TRUE;
    HOC = TRUE;
    Pitbull = TRUE;
    SaidPitbull = FALSE;
    wentActive = false;
}

void MissileClass::CleanupData()
{
    CleanupLocalData();
    SimWeaponClass::CleanupData();
}

void MissileClass::CleanupLocalData()
{
    if (runTime not_eq 0.0F and not IsDead())
    {
        SetDead(TRUE);
    }

    DropTarget();

    if (display and inputData->displayType not_eq DisplayHTS)
    {
        delete display;
    }

    display = NULL;
}

void MissileClass::Init(SimInitDataClass* initData)
{
    if (initData == NULL)
    {
        Init();
    }
}

void MissileClass::Init(void)
{
    Falcon4EntityClassType* classPtr;
    WeaponClassDataType* wc;
    SimWeaponDataType* wpnDefinition;
    int dataIdx;

    SimWeaponClass::Init();
    classPtr = (Falcon4EntityClassType*)EntityType();
    // 2000-09-29 MODIFIED BY S.G. I NEED TO SET 'inputData' EVEN IF NON LOCAL SO incomingMissile HAS THE RIGHT DATA
    wc = (WeaponClassDataType*)classPtr->dataPtr;
    wpnDefinition = &SimWeaponDataTable[classPtr->vehicleDataIndex];
    dataIdx = wpnDefinition->dataIdx;
    ReadInput(dataIdx);
    // END OF ADDED SECTION



    // 2002-04-05 MN engineData was NULL once, causing a crash. So lets give all missiles on all machines their data, regardless if they're local or not...
    //   if (IsLocal())
    {
        done = FalconMissileEndMessage::NotDone;
        isCaged = TRUE;
        isSlave = TRUE;
        isSpot = TRUE;
        isTD = FALSE;


        /*----------------------------*/
        /* read in data and options   */
        /*----------------------------*/
        //      wpnDefinition = (SimWpnDefinition*)mvrDefinition;
        // 2000-09-29 COMMENTED OUT BY S.G. SINCE IT'S DONE ABOBE (THE LINE BEFORE WAS ALREADY COMMENTED OUT)
        //    wc = (WeaponClassDataType*)classPtr->dataPtr;
        //    wpnDefinition = &SimWeaponDataTable[classPtr->vehicleDataIndex];
        //    dataIdx = wpnDefinition->dataIdx;
        //    ReadInput(dataIdx);


        aeroData =   missileDataset[min(dataIdx, numMissileDatasets - 1)].aeroData;
        rangeData =  missileDataset[min(dataIdx, numMissileDatasets - 1)].rangeData;
        engineData = missileDataset[min(dataIdx, numMissileDatasets - 1)].engineData;
        auxData = missileDataset[min(dataIdx, numMissileDatasets - 1)].auxData; // JPO

        ShiAssert(aeroData);
        ShiAssert(rangeData);
        ShiAssert(engineData);
        ShiAssert(auxData);


        initXLoc = initYLoc = initZLoc = 0.0F;


#ifndef MISSILE_TEST_PROG

        switch (inputData->displayType)
        {
            case DisplayBW:
            case DisplayIR:
                display = new MaverickDisplayClass(this);
                break;

            case DisplayHTS:
                if (parent->IsCampaign())
                    display = NULL;
                else
                    display = FindSensor((SimMoverClass*)parent.get(), SensorClass::HTS);

                break;

            case DisplayNone:
            case DisplayColor:
            default:
                display = NULL;
                break;
        }

        // edg: total hack here.
        if (((VuEntityType *)classPtr)->classInfo_[VU_STYPE] == STYPE_MISSILE_SURF_SURF)
        {
            inputData->seekerType = SensorClass::Visual;
            inputData->seekerVersion = 0;
            inputData->gimlim = 90.0f * DTR;
        }

        SensorClass *snsr;

        switch (inputData->seekerType)
        {
            case SensorClass::IRST:
                snsr = new IrstClass(inputData->seekerVersion, this);
                break;

            case SensorClass::Visual:
                snsr = new EyeballClass(inputData->seekerVersion, this);
                break;

            case SensorClass::Radar:
                snsr = new BeamRiderClass(GetRadarType(), this);
                break;

            case SensorClass::RWR:
                snsr = new HarmSeekerClass(inputData->seekerVersion, this);
                break;

            case SensorClass::RadarHoming:
                snsr = new BeamRiderClass(inputData->seekerVersion, this);
                break;

            default:
                snsr = NULL;
                break;
        }

        if (snsr)
        {
            sensorArray = new SensorClass*[1];
            sensorArray[0] = snsr;
            numSensors = 1;
        }
        else
        {
            sensorArray = NULL;
            numSensors = 0;
        }

#else
        sensorArray = NULL;
        display = NULL;
        numSensors = 0;
#endif
    }

    //   else // not local
    if ( not IsLocal())
    {
        // 2002-04-05 MN not needed anymore...
        //      sensorArray = NULL;
        //      display = NULL;


#if 0 // SCR 11/6/98  PowerOutput is part of SimBase's high priority dirty data.  It should be propigated
        // to all machines, so this should not be necessary.  If this is a problem, figure out why dirty
        // data is handling it...

        // edg: set ir scale here.  I think this will work for doing the
        // missile trails.  If we're non local I think we're flying so we'll
        // want to start out with a full burn
        // could we also do launch cloud here?
        SetPowerOutput(1.0f);
#endif

        CalcTransformMatrix(this);
    }

    launchState = PreLaunch;

    if (drawPointer == (DrawableBSP*)0xbaadf00d)
        return;

    // prime the display
    if (drawPointer and ((DrawableBSP*)drawPointer)->GetNumSwitches() > 0)
    {
        if (ifd and ifd->stage2gone) // no 2nd stage
            ((DrawableBSP *)drawPointer)->SetSwitchMask(0, 0);
        else  // draw 2nd stage
            ((DrawableBSP *)drawPointer)->SetSwitchMask(0, 1);
    }

}

void MissileClass::Start(SimObjectType *tgt)
{
    // Make sure we have initialized the missile rotation properly
    F4Assert(initAz < 10.0F);
    F4Assert(initAz > -10.0F);
    F4Assert(initEl < 10.0F);
    F4Assert(initEl > -10.0F);

    // alloc the in flight data
    ifd = new MissileInFlightData;

    Init1();
    CommandGuide();

    // At this point, we need to make a copy of our target data - Since we need to start
    // Using relative geometry between us and the target - so force a target drop and
    // retarget now that we're 'Launching'

    // edg NOTE: fucking with the order we set launch state and targeting
    // could have deleterious effects on reference counting  We want to
    // DropTarget BEFORE we change state to Launching.  The launch state
    // effects how SimObjs are constructed and referenced for targeting
    // In a Prelaunch state the missile's target has no reference.  When
    // on rail or in flight the target is COPIED in SetTarget and ref'd.

    // If the missile is boresight, it gets to ignore the passed in target and keep its own
    // Marco Edit - actually - if it's boresighted or uncaged
    bool hasref = false; // JB 020109 CTD fix. Engage in safe referencing.  Shooting a breathing but unlocked Mav caused a CTD because DropTarget would delete the object that was later used.

    if ( not isSlave or not isCaged or tgt == NULL)// and sensorArray[0]->Type() == SensorClass::IRST) //me123 make sure only ir's are unchaged for now
    {
        tgt = targetPtr;

        if (tgt)
        {
            tgt->Reference();
            hasref = true;
        }
    }

    DropTarget();
    launchState = Launching;
    SetTarget(tgt);

    if (hasref)
        tgt->Release();

    // 2001-03-02 ADDED BY S.G. SO A LAUNCH IS DETECTED AT LAUNCH FOR SARH MISSILE
    if (targetPtr)
    {
        if (GetSeekerType() == SensorClass::RadarHoming and sensorArray)
            ((BeamRiderClass *)sensorArray[0])->SendTrackMsg(targetPtr, Track_Launch);
    }

    // END OF ADDED SECTION

    /*
    if ( targetPtr )
    {
       targetX = targetPtr->BaseData()->XPos();
       targetY = targetPtr->BaseData()->YPos();
       targetZ = targetPtr->BaseData()->ZPos();
    }
    */

    /*
    #ifndef MISSILE_TEST_PROG
    if (tgt)
       MonoPrint ("Missile %d Launch at %8ld %4d -> %4d\n", Id().num_,SimLibElapsedTime, parent->Id().num_, targetPtr->BaseData()->Id().num_);
    else
       MonoPrint ("Missile %d Launch at %8ld %4d -> No Target\n", Id().num_,SimLibElapsedTime, parent->Id().num_);
    #endif
       */

    if (inputData->displayType not_eq DisplayHTS and display)
    {
        display->DisplayExit();
        delete display;
        display = NULL;
    }

    /*
       if (targetPtr)
        MonoPrint ("Missile launched at %d\n", targetPtr->BaseData()->Id().num_);
       else
        MonoPrint ("Missile launched at nothing\n");
    */
}


int MissileClass::Exec(void)
{
    int i;

    if (IsDead())
    {
        return TRUE;
    }

#ifdef MLR_NEWSNDCODE
    SoundPos.UpdatePos(this);
#endif

    if (IsLocal())
    {
        if (IsExploding())
        {
            if ( not IsSetFlag(SHOW_EXPLOSION))
            {
                // edg note: all special effects moved to the MissileEndMessage
                // Process member function
                // make sure we don't do it again...
                SetFlag(SHOW_EXPLOSION);
                // we can now kill it immediately
                SetDead(TRUE);
            }
        }
        else
        {
            //if (((DrawableBSP*)drawPointer)->Label()[0] not_eq '\0')
            // MonoPrint ("ZPOS %.2f\n", ZPos());
#ifndef MISSILE_TEST_PROG
            if (sensorArray)
            {
                ShiAssert(sensorArray[0]);
                UpdateTargetData();
            }

#else
            /*
            if ( targetPtr ){
             targetX = targetPtr->BaseData()->XPos();
             targetY = targetPtr->BaseData()->YPos();
             targetZ = targetPtr->BaseData()->ZPos();
            }
            */
            CalcTransformMatrix(this);
            CalcRelGeom(this, targetPtr, NULL, 1.0F / SimLibMajorFrameTime);
            //MonoPrint ("%8.2f %8.2f %8d\n", targetPtr->localData->az*57.29F, targetPtr->localData->el*57.29F, SimLibElapsedTime);

            if (ifd)  // JB 010803
            {
                ifd->gimbal = min(max(targetPtr->localData->ata, -inputData->gimlim), inputData->gimlim);
                ataerr = targetPtr->localData->ata - ifd->gimbal;
            }

#endif

            if (SimDriver.MotionOn())
            {
                CommandGuide();

                for (i = 0; i < SimLibMinorPerMajor; i++)
                {
                    FlyMissile();
                }
            }

            // JPO - is it time to jettison 2nd stage?
            if (ifd and ifd->stage2gone == false and auxData and runTime > auxData->SecondStageTimer)
            {
                ifd->stage2gone = true;
            }

            if (((DrawableBSP*)drawPointer)->GetNumSwitches() > 0)
            {
                if (ifd and ifd->stage2gone)
                {
                    // 2002-03-04 MN CTD fix
                    ((DrawableBSP *)drawPointer)->SetSwitchMask(0, 0);
                }
                else
                {
                    ((DrawableBSP *)drawPointer)->SetSwitchMask(0, 1);
                }
            }

            ShiAssert(auxData); // MN which missile doesn't have aux data ??

            // A.S. Begin  deployable wings if "deployableWingsTime" is set in the .dat file of the missile
            if (auxData and auxData->deployableWingsTime > 0)
            {
                if (runTime > auxData->deployableWingsTime)
                {
                    if (((DrawableBSP*)drawPointer)->GetNumSwitches() > 0)
                    {
                        ((DrawableBSP *)drawPointer)->SetSwitchMask(0, 1);
                    }
                }
            }

            // A.S. end

            /*-----------------*/
            /* body axis accel */

            ClosestApproach();

            groundZ = OTWDriver.GetGroundLevel(x, y);

            SetStatus();

            /*--------------------*/
            /* Update shared data */
            /*--------------------*/
            SetPosition(x, y, z);
            SetDelta(xdot, ydot, zdot);
            SetYPR(psi, theta, phi);
            SetYPRDelta(r, q, p);
            // sfr: no more
            //SetVt(vt);

            if (launchState == InFlight)
            {
                // ACMI Output
                if (gACMIRec.IsRecording() and (SimLibFrameCount bitand 0x00000003) == 0)
                {
                    misPos.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
                    misPos.data.type = Type();
                    misPos.data.uniqueID = ACMIIDTable->Add(Id(), NULL, TeamInfo[GetTeam()]->GetColor()); //.num_;
                    misPos.data.x = XPos();
                    misPos.data.y = YPos();
                    misPos.data.z = ZPos();
                    misPos.data.roll = phi;
                    misPos.data.pitch = theta;
                    misPos.data.yaw = psi;
                    // remove: misPos.data.teamColor = TeamInfo[GetTeam()]->GetColor();
                    gACMIRec.MissilePositionRecord(&misPos);
                }
            }

            // edg: If a missle has status of missed, let it run for a while.
            // In general it's probably not a bad idea.  Specifically there
            // was a prob with the Mav's exploding almost immediately on launch
            // apparently they go quickly to a Missed status.  There may be another
            // problem there -- I'm just treating the symptom
            // 2002-04-04 MN only do that if we did not yet have closest approach on the target 
            // Hope this finally fixes floating missiles
            if (
                done == FalconMissileEndMessage::Missed and 
                runTime < 15.0f and 
 not ((g_nMissileFix bitand 0x10) and (flags bitand ClosestApprch))
            )
            {
#ifndef MISSILE_TEST_PROG
                done = FalconMissileEndMessage::NotDone;
#endif
            }

            if (done == FalconMissileEndMessage::Missed and ZPos() < groundZ - 10)   // MLR
            {
                done = FalconMissileEndMessage::NotDone;
            }

            if (done not_eq FalconMissileEndMessage::NotDone)
            {
#ifndef MISSILE_TEST_PROG
                EndMissile();
#endif
            }
        } // end motion on

        if (launchState == InFlight)
        {
            UpdateTrail();
        }
    } // end is local
    else
    {
#if 0
        // SCR 11/6/98  PowerOutput is part of SimBase's high priority dirty data.  It should be propigated
        // to all machines, so this should not be necessary.  If this is a problem, figure out why dirty
        // data is handling it...

        // not local, reduce IR Scale over time
        SetPowerOutput(PowerOutput() - 0.25 * SimLibMajorFrameTime);
#endif

        if (launchState == InFlight)
        {
            // ACMI Output
            if (gACMIRec.IsRecording() and (SimLibFrameCount bitand 0x00000003) == 0)
            {
                misPos.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
                misPos.data.type = Type();
                misPos.data.uniqueID = ACMIIDTable->Add(Id(), NULL, TeamInfo[GetTeam()]->GetColor()); //.num_;
                misPos.data.x = XPos();
                misPos.data.y = YPos();
                misPos.data.z = ZPos();
                misPos.data.roll = phi;
                misPos.data.pitch = theta;
                misPos.data.yaw = psi;
                // remove: misPos.data.teamColor = TeamInfo[GetTeam()]->GetColor();
                gACMIRec.MissilePositionRecord(&misPos);
            }
        }
        else
        {
            launchState = InFlight;
        }

        // Update the ground height for this missile (used by the engine glow stuff I think)
        groundZ = OTWDriver.GetGroundLevel(XPos(), YPos());

        // Update the trail
        UpdateTrail();
    }

    //MI 6/01/02
    if (Pitbull and parent and parent->IsAirplane() and not SaidPitbull)
    {
        float LastMissileImpactTime = 0.0F;
        float MissileActiveTime = 0.0F;
        FireControlComputer* theFCC = ((SimVehicleClass*)parent.get())->GetFCC();

        if (theFCC)
        {
            LastMissileImpactTime = theFCC->lastMissileImpactTime;
            MissileActiveTime = theFCC->missileActiveTime;
        }

        if (LastMissileImpactTime > 0.0F and (LastMissileImpactTime < MissileActiveTime))
        {
            AircraftClass *pa = static_cast<AircraftClass*>(parent.get());
            FalconRadioChatterMessage* radioMessage =
                new FalconRadioChatterMessage(parent->Id(), FalconLocalSession);
            radioMessage->dataBlock.from = parent->Id();
            radioMessage->dataBlock.to = MESSAGE_FOR_TEAM;
            radioMessage->dataBlock.voice_id = ((FlightClass*)
                                                (pa->GetCampaignObject()))->GetPilotVoiceID(
                                                   pa->GetCampaignObject()->GetComponentIndex(pa)
                                               )
                                               ;
            radioMessage->dataBlock.edata[0] = pa->GetCampaignObject()->GetComponentIndex(pa);
            radioMessage->dataBlock.message = rcFIREAMRAAM;
            radioMessage->dataBlock.edata[1] = 6; // Pit Bull
            FalconSendMessage(radioMessage, TRUE);
            SaidPitbull = TRUE;
        }
    }

#ifdef MLR_NEWSNDCODE

    if (PowerOutput() and auxData->EngineSound)
    {
        SoundPos.Sfx(auxData->EngineSound);
    }

#endif

    return TRUE;
}

void MissileClass::FlyMissile(void)
{
    if (launchState == Launching)
    {
        Launch();
        Flight(); // MLR 1/5/2005 -
    }
    else
    {
        Flight();
    }

    runTime += SimLibMinorFrameTime;
}

float MissileClass::GetRMax(float alt, float vt, float az, float targetVt, float ataFrom)
{
#if 1
    // JPO CTD checks

    ShiAssert(FALSE == F4IsBadReadPtr(rangeData, sizeof(rangeData)));

    if ( not rangeData or F4IsBadReadPtr(rangeData, sizeof(rangeData)))   //Wombat778 3-23-04  Added CTD check
        // MLR 5/2/2004 - this bug was caused by heli brains firing the new rocket code.
        return 0.0F;


    // 2000-11-17 MODIFIED BY S.G. az CAN'T BE USED HERE BECAUSE IT CAN BE NEGATIVE AND THE DATA FILE DO NOT ACCOUNT FOR THAT. PLUS HEAD AND TAIL HAVE THE SAME VALUE (BAD)
    float rmax = 10.0f;

    if (rangeData)
    {
        // FRB - CTD's
        rmax = Math.ThreedInterp(alt, vt, ataFrom,
                                 rangeData->altBreakpoints, rangeData->velBreakpoints,
                                 rangeData->aspectBreakpoints, rangeData->data,
                                 rangeData->numAltBreakpoints, rangeData->numVelBreakpoints,
                                 rangeData->numAspectBreakpoints, &lastRmaxAlt, &lastRmaxVt, &lastRmaxAta);
    }

    //#else
    Falcon4EntityClassType* classPtr = (Falcon4EntityClassType*)EntityType();

    if (classPtr)
    {
        WeaponClassDataType* weaponData = (WeaponClassDataType*)classPtr->dataPtr;

        if (weaponData)
            rmax = weaponData->Range * KM_TO_FT;
    }

#endif

    // Scale for closure
    rmax *= (1500.0F + vt + targetVt * (float)cos(ataFrom)) / 1500.0F;

    // Scale for ataFrom
    // rmax *= (cos(ataFrom) + 1.5F) / 2.5F;

    return (rmax);
}

// This is used for time of flight and other computations (unfortunatly)
static const float MISSILE_SPEED = 1500.0f; //me123 ajusted from 2000 // Feet per second -- VERY WRONG, but easy... // JB 010215 changed from 1300 to 1500
static const float MISSILE_ALTITUDE_BONUS = 23.0f; //me123 addet here and in fccmain.cpp // JB 010215 changed from 24 to 23

//float MissileClass::GetTOF (float alt, float vt, float ataFrom, float targetVt, float range)
//me123 overtake needs to be calvulated the same way in Fccmain.cpp
float MissileClass::GetTOF(float alt, float vt, float ataFrom, float targetVt, float range)
{
    // TODO:  Get this from a table like the max range stuff above...
    float overtake = MISSILE_SPEED  + (alt / 1000.0f * MISSILE_ALTITUDE_BONUS) + targetVt * (float)cos(ataFrom); //me123 + vt taken out and "alt" addet due to aim120 tof problems
    overtake += (vt * FTPSEC_TO_KNOTS - 150.0f) / 2 ; //me123 platform speed bonus // JB 010215 changed from 250 to 150
    float tof = range / overtake;
    tof += -5.0f * (float) sin(.07 * tof); // JB 010215

    return max(0.0F, tof); // Counting on silent failure of divid by 0.0 here...
}
//me123 overtake needs to be calvulated the same way in Fccmain.cpp
float MissileClass::GetActiveRange(float alt, float vt, float ataFrom, float targetVt, float range)
{
    // Well, I don't much like this, but it'll be consistent with the other bogus data...
    float overtake = MISSILE_SPEED  + (alt / 1000.0f * MISSILE_ALTITUDE_BONUS) + targetVt * (float)cos(ataFrom); //me123 + vt taken out and "alt" addet due to aim120 tof problems
    float rangeToGo = overtake * GetActiveTime(alt, vt, ataFrom, targetVt, range);
    return rangeToGo; //me123 the missile goes active when there's a serden sec TOF left, not after a serden TOF // range - rangeToGo;
}

//float MissileClass::GetActiveTime(float alt, float vt, float ataFrom, float targetVt, float range)
float MissileClass::GetActiveTime(float, float, float, float, float)
{
    // This could be a variable, but for now its a constant per missile type
    return inputData->mslActiveTtg;
}

float MissileClass::GetASE(float alt, float vt, float ataFrom, float targetVt, float range)
{
    Falcon4EntityClassType* classPtr = (Falcon4EntityClassType*)EntityType();
    WeaponClassDataType* weaponData = (WeaponClassDataType*)classPtr->dataPtr;

    float ase;
    float rmax = weaponData->Range * KM_TO_FT;

    alt = vt = ataFrom = targetVt = range;
    ase = inputData->gimlim * RTD * (1.0F - range / rmax);

    return (max(ase, 0.0F));
}

void MissileClass::GetTransform(TransformMatrix vmat)
{
    memcpy(vmat, dmx, sizeof(TransformMatrix));
}

int MissileClass::SetSeekerPos(float* az, float* el)
{
    int isLimited = FALSE;


    ShiAssert(parent->IsSim()); // SCR 9/22/98:  I think this is only called on behalf of the player.
    ShiAssert(parent); // A missile should always have a parent
    // if (parent and parent->IsSim() )
    // {
    memcpy(dmx, ((SimBaseClass*)parent.get())->dmx, sizeof(TransformMatrix));
    // }

    ShiAssert(sensorArray); // SCR 10/9/98:  Who's calling this on a missile with no seeker???

    if (sensorArray == NULL)   // VWF I'm hoping that this will help.
    {
        return FALSE;
    }

    sensorArray[0]->SetSeekerPos(*az, *el);
    LimitSeeker(*az, *el);
    *az = sensorArray[0]->SeekerAz();
    *el = sensorArray[0]->SeekerEl();

    if (fabs(*az) == inputData->gimlim or
        fabs(*el) == inputData->gimlim)
    {
        sensorArray[0]->SetSeekerPos(*az, *el);
        isLimited = TRUE;

        if (inputData->displayType not_eq DisplayHTS and display)
        {
            ((MaverickDisplayClass*)display)->SetSeekerPos(*az / inputData->gimlim,
                    *el / inputData->gimlim);
        }

        return isLimited;
    }
    else
    {
        return FALSE;
    }
}

void MissileClass::GetSeekerPos(float* az, float* el)
{
    if (sensorArray)
    {
        *az = sensorArray[0]->SeekerAz();
        *el = sensorArray[0]->SeekerEl();
    }
    else
    {
        *az = 0.0F;
        *el = 0.0F;
    }
}

void MissileClass::UpdatePosition()
{
    Tpoint newPos;

    if ( not ifd)
        return; // JB 010803

    drawPointer->GetPosition(&newPos);
    ifd->oldx[0] = newPos.x;
    ifd->oldx[1] = newPos.x;
    ifd->oldx[2] = 0.0;
    ifd->oldx[3] = 0.0;

    ifd->oldy[0] = newPos.y;
    ifd->oldy[1] = newPos.y;
    ifd->oldy[2] = 0.0;
    ifd->oldy[3] = 0.0;

    ifd->oldz[0] = newPos.z;
    ifd->oldz[1] = newPos.z;
    ifd->oldz[2] = 0.0;
    ifd->oldz[3] = 0.0;
}

void MissileClass::SetDead(int flag)
{
    if (IsAwake())
    {
        Sleep();
    }

    // delete in flight data if any
    if (ifd)
    {
        delete ifd;
        ifd = NULL;
    }

    // clear out target if any
    DropTarget();

    SimWeaponClass::SetDead(flag);
}

int MissileClass::Sleep()
{
    int retval = 0;

    if ( not IsAwake())
    {
        return retval;
    }

    /* if (trail)
     RemoveTrail();*/

    if (TrailId)
    {
        RemoveTrail();
    }

    // Drop the sensors platform if beamrider
    ClearReferences();

    // make sure parent is no longer referenced
    // Tell the parent this missile doesn't need guidance anymore
    if (parent.get() not_eq NULL)
    {
        if (parent->IsGroundVehicle())
        {
            CampBaseClass *campobj = ((GroundClass*)parent.get())->GetCampaignObject();

            if (campobj->IsBattalion())
            {
                ((BattalionClass*)campobj)->DecrementMissileCount();
            }
            else if (campobj->IsTaskForce())
            {
                // add in naval stuff
                ((TaskForceClass*)campobj)->DecrementMissileCount();
            }
        }
        else if (parent->IsBattalion())
        {
            ((BattalionClass*)parent.get())->DecrementMissileCount();
        }
        else if (parent->IsTaskForce())
        {
            ((TaskForceClass*)parent.get())->DecrementMissileCount();
        }
    }

    SimWeaponClass::Sleep();
    return retval;
}

void MissileClass::ClearReferences()
{
    if (sensorArray and sensorArray[0] and sensorArray[0]->Type() == SensorClass::RadarHoming)
    {
        ((BeamRiderClass*)sensorArray[0])->SetGuidancePlatform(NULL);
    }
}

int MissileClass::Wake()
{
    if (IsAwake())
    {
        return 0;
    }

    int retval = SimWeaponClass::Wake();

    // Tell our campaign parent he's got missiles in the air
    // (So that SAM battalions keep their radars on)
    if (parent)
    {
        //Campaign launched vehicles don't necessarily have a parent
        if (parent->IsGroundVehicle())
        {
            // JPO more checks please
            GroundClass *pg = static_cast<GroundClass*>(parent.get());
            CampBaseClass *campobj = pg->GetCampaignObject();

            if (campobj -> IsBattalion())
            {
                ((BattalionClass*)campobj)->IncrementMissileCount();
            }
            else if (campobj -> IsTaskForce())
            {
                // add in naval stuff
                ((TaskForceClass*)campobj)->IncrementMissileCount();
            }
        }
        else if (parent->IsBattalion())
        {
            ((BattalionClass*)parent.get())->IncrementMissileCount();
        }
        else if (parent->IsTaskForce())   // and again
        {
            ((TaskForceClass*)parent.get())->IncrementMissileCount();
        }
    }

    return retval;
}

void MissileClass::SetVuPosition()
{
    SetPosition(x, y, z);
    SetDelta(xdot, ydot, zdot);
    SetYPR(psi, theta, phi);
    SetYPRDelta(r, q, p);
    // sfr: no more
    //SetVt(vt);
}


void
MissileClass::EndMissile(void)
{
    FalconMissileEndMessage* endMessage;


    if (done not_eq FalconMissileEndMessage::MissileKill and 
        done not_eq FalconMissileEndMessage::GroundImpact and 
        done not_eq FalconMissileEndMessage::FeatureImpact and 
        done not_eq FalconMissileEndMessage::BombImpact and // "bomb warhead" missiles hit SIM target
        done not_eq FalconMissileEndMessage::ArmingDelay) // when the warhead is not yet armed, do nothing here
    {
        if (flags bitand SensorLostLock)
            done = FalconMissileEndMessage::ExceedFOV;

        // 2002-02-26 ADDED BY S.G. This is the best place to handle aggregated campaign object as target that got missed...
        if (targetPtr)
        {
            // First get the campaign object if it's still a sim entity
            CampBaseClass *campBaseObj;

            if (targetPtr->BaseData()->IsSim()) // If we're a SIM object, get our campaign object
                campBaseObj = ((SimBaseClass*)targetPtr->BaseData())->GetCampaignObject();
            else
                campBaseObj = (CampBaseClass *)targetPtr->BaseData();

            // Now find out if our campaign object is aggregated
            if (campBaseObj and campBaseObj->IsAggregate())
            {
                // Yes, replace the current target by its aggregated campaign object
                if (campBaseObj not_eq targetPtr->BaseData())
                {
                    if (targetPtr)
                        targetPtr->Release();

#ifdef DEBUG
                    /*    targetPtr = new SimObjectType( OBJ_TAG, NULL, campBaseObj );*/
#else
                    targetPtr = new SimObjectType(campBaseObj);
#endif
                    targetPtr->Reference();
                }

                // Now say we've killed the target and let the 2D code handle the damage
                done = FalconMissileEndMessage::MissileKill;
            }
        }
    }


    // MonoPrint ("Missile %d done: Time %8ld code %d\n", Id().num_, SimLibElapsedTime, done);
    SetExploding(TRUE);

    ShiAssert(parent);

    if ( not parent)
        return;

    // SCR 11/29/98
    // We skip doing damage if a surface launched missile blew up on launch
    // (cause it hit a building or something silly like that)

    // 2002-03-28 MN if end message is ArmingDelay, don't apply damage at all
    if ( not (done == FalconMissileEndMessage::ArmingDelay) and inputData and (runTime > inputData->guidanceDelay or not parent->OnGround()))
        ApplyProximityDamage();

    endMessage = new FalconMissileEndMessage(Id(), FalconLocalGame);
    endMessage->RequestReliableTransmit();
    endMessage->RequestOutOfBandTransmit();
    endMessage->dataBlock.fEntityID  = parent->Id();
    endMessage->dataBlock.fCampID    = parent->GetCampID();
    endMessage->dataBlock.fSide      = parent->GetCountry();
    endMessage->dataBlock.fIndex     = parent->Type();

    if (parent->IsSim())
        endMessage->dataBlock.fPilotID   = shooterPilotSlot;
    else
        endMessage->dataBlock.fPilotID   = 0; // Flight leads get all kills for now...

    if (targetPtr)
    {
        if (targetPtr->BaseData()->IsSim())
        {
            endMessage->dataBlock.dCampSlot  = (char)((SimBaseClass*)targetPtr->BaseData())->GetSlot();
        }
        else
        {
            endMessage->dataBlock.dCampSlot  = 0;
        }

        endMessage->dataBlock.dEntityID  = targetPtr->BaseData()->Id();
        endMessage->dataBlock.dCampID    = targetPtr->BaseData()->GetCampID();
        endMessage->dataBlock.dSide      = targetPtr->BaseData()->GetCountry();
        endMessage->dataBlock.dIndex     = targetPtr->BaseData()->Type();
        endMessage->dataBlock.dPilotID   = 0;
    }
    else
    {
        endMessage->dataBlock.dEntityID  = FalconNullId;
        endMessage->dataBlock.dCampID    = 0;
        endMessage->dataBlock.dSide      = 0;
        endMessage->dataBlock.dIndex     = 0;
        endMessage->dataBlock.dPilotID   = 0;
    }

    endMessage->dataBlock.fWeaponUID = Id();
    endMessage->dataBlock.wIndex = Type();

    endMessage->dataBlock.endCode    = done;
    endMessage->dataBlock.x    = XPos();
    endMessage->dataBlock.y    = YPos();
    endMessage->dataBlock.z    = ZPos();
    endMessage->dataBlock.xDelta    = XDelta();
    endMessage->dataBlock.yDelta    = YDelta();
    endMessage->dataBlock.zDelta    = ZDelta();
    endMessage->dataBlock.groundType    = -1;

    if (done == FalconMissileEndMessage::GroundImpact or ZPos() > groundZ)
    {
        endMessage->dataBlock.z  = groundZ;
        endMessage->dataBlock.groundType = (char)OTWDriver.GetGroundType(XPos(), YPos());
    }

    switch (done) // particle effects
    {
        case FalconMissileEndMessage::GroundImpact:
            endMessage->SetParticleEffectName(auxData->psGroundImpact);
            break;

        case FalconMissileEndMessage::MissileKill:
            endMessage->SetParticleEffectName(auxData->psMissileKill);
            break;

        case FalconMissileEndMessage::FeatureImpact:
            endMessage->SetParticleEffectName(auxData->psFeatureImpact);
            break;

        case FalconMissileEndMessage::BombImpact:
            endMessage->SetParticleEffectName(auxData->psBombImpact);
            break;

        case FalconMissileEndMessage::ArmingDelay:
            endMessage->SetParticleEffectName(auxData->psArmingDelay);
            break;

        case FalconMissileEndMessage::ExceedFOV:
            endMessage->SetParticleEffectName(auxData->psExceedFOV);
            break;
    }

    // Can't send the end message until all the damage messages are gone.
    FalconSendMessage(endMessage, FALSE);
}


/*
** Name: ApplyProximityDamage
** Description:
** At this point the missile has reached the end of its life.
** We send out an end message.  If state is missile kill, we send
** a damage message to its target.  Otherwise, we look for any ground
** objects that may have been impacted by the missile exploding.
*/
void
MissileClass::ApplyProximityDamage(void)
{
    float tmpX, tmpY, tmpZ;
    float rangeSquare;
    FalconEntity* testObject;
    CampBaseClass* objective;
    ACMIStationarySfxRecord acmiStatSfx;
    //float normBlastDist;
#ifdef VU_GRID_TREE_Y_MAJOR
    VuGridIterator gridIt(ObjProxList, YPos(), XPos(), NM_TO_FT * 2.0F);
#else
    VuGridIterator gridIt(ObjProxList, XPos(), YPos(), NM_TO_FT * 2.0F);
#endif

    // 2002-03-28 MN added BombImpact for "bomb-like" missiles (JSOW...)
    // RV - Biker - For AGMs we use FeatureImpact also
    if (done == FalconMissileEndMessage::MissileKill or done == FalconMissileEndMessage::BombImpact or done == FalconMissileEndMessage::FeatureImpact)
    {
        //TJ_Changes .... how about we check for another object in the vicinity and apply damage to that ?
        //Instead of just attacking object you are targeting we will now loop through all aircraft ....

        //JAM 03Nov03 - This block is causing HARMS and many AA missiles to not cause any damage. Restoring SP3 code for now.
        /* if( domain bitand wdAir ) {
         SimBaseClass* testObject;

         if ( not SimDriver.objectList) return;

         VuListIterator objectWalker(SimDriver.objectList);
         testObject = (SimBaseClass*) objectWalker.GetFirst();

         while (testObject) {
         //TJ_changes
         //Only check against planes ....
         //removed this -> dont check against plane that we already were targeting that is handled abouve ... for now .. this could become only check
         if ( not (testObject->IsAirplane())/* or targetPtr and ( targetPtr->BaseData()->Id() == testObject->Id() ) */ /* ) {
 testObject = (SimBaseClass*) objectWalker.GetNext();
 continue;
 }
 tmpX = testObject->XPos() - XPos();
 tmpY = testObject->YPos() - YPos();
 tmpZ = testObject->ZPos() - ZPos();

 rangeSquare = tmpX*tmpX + tmpY*tmpY + tmpZ*tmpZ;

 if (rangeSquare < lethalRadiusSqrd ) {
 // edg: calculate a normalized blast Dist
 normBlastDist = ( lethalRadiusSqrd - rangeSquare )/( lethalRadiusSqrd );

 // quadratic dropoff
 normBlastDist *= normBlastDist;
 SendDamageMessage( targetPtr->BaseData(), rangeSquare, FalconDamageType::MissileDamage ); // 2002-02-26 MODIFIED BY S.G. Removed '(SimBaseClass*)' from targetPtr->BaseData() since it can be a campaign object anyway (bad practice but no harm was done).
 }
 testObject = (SimBaseClass*) objectWalker.GetNext();
 }
}
*/ if (targetPtr)
        {
            // F4Assert(targetPtr->BaseData()->IsSim());
            SendDamageMessage(targetPtr->BaseData(), 0, FalconDamageType::MissileDamage); // 2002-02-26 MODIFIED BY S.G. Removed '(SimBaseClass*)' from targetPtr->BaseData() since it can be a campaign object anyway (bad practice but no harm was done).
        }
    }

    if (done == FalconMissileEndMessage::GroundImpact or ZPos() > groundZ)
    {
        int groundType;

        // check for water b4 placing crater
        groundType = OTWDriver.GetGroundType(XPos(), YPos());

        if ( not (groundType == COVERAGE_WATER or groundType == COVERAGE_RIVER))
        {
            AddToTimedPersistantList(VIS_CRATER2 + PRANDInt3(), Camp_GetCurrentTime() + CRATER_REMOVAL_TIME, XPos(), YPos());

            // add crater to ACMI as special effect
            if (gACMIRec.IsRecording())
            {

                acmiStatSfx.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
                acmiStatSfx.data.type = SFX_CRATER2 + PRANDInt3();
                acmiStatSfx.data.x = XPos();
                acmiStatSfx.data.y = YPos();
                acmiStatSfx.data.z = ZPos();
                acmiStatSfx.data.timeToLive = 180.0f;
                acmiStatSfx.data.scale = 1.0f;
                gACMIRec.StationarySfxRecord(&acmiStatSfx);
            }
        }


        // now we need to check objects
        // only do this on ground impact
        if (SimDriver.combinedList)
        {
            VuListIterator objectWalker(SimDriver.combinedList);
            // Check vs vehicles
            testObject = (FalconEntity*) objectWalker.GetFirst();

            while (testObject)
            {
                if (testObject not_eq this)
                {
                    tmpX = testObject->XPos() - XPos();
                    tmpY = testObject->YPos() - YPos();
                    tmpZ = testObject->ZPos() - ZPos();

                    // for ground units, don't use Z diff
                    if (testObject->OnGround())
                        rangeSquare = tmpX * tmpX + tmpY * tmpY;
                    else
                        rangeSquare = tmpX * tmpX + tmpY * tmpY + tmpZ * tmpZ;

                    if (rangeSquare < lethalRadiusSqrd)
                    {
                        SendDamageMessage(testObject, rangeSquare, FalconDamageType::ProximityDamage);
                    }
                }

                testObject = (FalconEntity*) objectWalker.GetNext();
            }
        }
    }

    // relative height above ground (Z = 0)
    float agl = ZPos() - groundZ;

    // no point in checking proximity if too high above ground
    // This needs to be >= the check for building impact ala FeatureCollision()
    if (agl > -900.0f)
    {
        // get the 1st objective that contains the bomb
        objective = (CampBaseClass*) gridIt.GetFirst();

        // main loop through objectives
        while (objective)
        {
            if (objective->GetComponents())
            {
                // loop thru each element in the objective
                VuListIterator featureWalker(objective->GetComponents());
                testObject = (FalconEntity*) featureWalker.GetFirst();

                while (testObject)
                {
                    // don't test the original target ('cause we already dealt with him)
                    // 2002-03-28 MN apply proximity damage if it IS the target,
                    // but we have got no missile kill or BombImpact -
                    // or the target can not be damaged by proximity damage
                    if (
 not targetPtr or testObject not_eq targetPtr->BaseData() or
                        (g_nMissileFix bitand 0x08) and testObject == targetPtr->BaseData() and 
                        done not_eq FalconMissileEndMessage::MissileKill and 
                        done not_eq FalconMissileEndMessage::BombImpact
                    )
                    {
                        tmpX = testObject->XPos() - XPos();
                        tmpY = testObject->YPos() - YPos();

                        rangeSquare = tmpX * tmpX + tmpY * tmpY + agl * agl;

                        if (rangeSquare < lethalRadiusSqrd)
                            SendDamageMessage(testObject, rangeSquare, FalconDamageType::ProximityDamage);
                    }

                    testObject = (FalconEntity*) featureWalker.GetNext();
                }
            }
            else
            {
                // Apply damage to an aggregated objective
                tmpX = objective->XPos() - XPos();
                tmpY = objective->YPos() - YPos();

                rangeSquare = tmpX * tmpX + tmpY * tmpY + agl * agl;

                if (rangeSquare < lethalRadiusSqrd)
                    SendDamageMessage(objective, rangeSquare, FalconDamageType::ProximityDamage);
            }

            // get the next objective that contains the bomb
            objective = (CampBaseClass*) gridIt.GetNext();

        } // end objective loop
    }
}


#define TIME_TO_RUN_IMPACT 30.0f

/*
** Name: FindRocketGroundImpact
** Description:
** Test flies a rocket (although should work for any missile, but
** guidance isn't tested ) until it impacts ground.
** Returns TRUE if resolution found and sets impact x,y,z
*/
BOOL
MissileClass::FindRocketGroundImpact(float *impactX, float *impactY, float *impactZ, float *impactTime)
{
    //float saveMinorFrameTime; // FRB
    float lastx = x, lasty = y, lastz = z;

    // Cobra test
    static FILE *fp = NULL;
    //if (fp == NULL)
    // fp = fopen("G:\\RocketImpact.txt", "w");

    /*
     // edg: yuck.  Unfortuantely we have to do this
     ifd = new MissileInFlightData;

     // init stuff and save vars we need to set back at end....
     runTime = 0.0f;
     saveMinorFrameTime = SimLibMinorFrameTime;

     initXLoc = 0.0f;
     initYLoc = 0.0f;
     initZLoc = 0.0f;
     initAz = 0.0f;
     initEl = 0.0f;

     SimLibMinorFrameTime = 0.05f;

     // Start stuff....
       Init1();
     launchState = Launching;

     // Exec Stuff
     // runtime updated in fly missile
     // keep flying up until a max time or until the ground is hit
     while ( runTime < TIME_TO_RUN_IMPACT )
     {
      // not sure if this is needed
             //CalcTransformMatrix(this);

     //MI fix for rocket recticle bouncing... these 3 lines where commented....
     lastx = x;
     lasty = y;
     lastz = z;

      // flies the thing
             CommandGuide(); // TODO: Avoid this -- all it does for rockets is set the G commands to 1.0

      flags or_eq FindingImpact; // MLR 1/9/2004 - added to prevent the rocket's launch smoke puff trail when selected
             FlyMissile();
      flags and_eq compl FindingImpact;

             ClosestApproach(); // TODO: Avoid this -- it's only really meaningful for proximity fuzed weapons
             SetPosition (x, y, z);

      if ( launchState == InFlight )
      {
                SetDelta(xdot, ydot, zdot);
                SetYPR(psi, theta, phi);
                SetYPRDelta (r, q, p);
     // sfr: no more
                //SetVt(vt);

     groundZ = OTWDriver.GetGroundLevel( x, y );
     if ( z > groundZ )
       break;
      }
     }

     *impactTime = runTime;

     float degpsi = psi * RTD;
     float degphi = phi * RTD;
     float degtheta = theta * RTD;


     // restore stuff
     runTime = 0.0f;
     GuidenceTime = 0.0f;
     SimLibMinorFrameTime = saveMinorFrameTime;
        launchState = PreLaunch;
     delete ifd;
     ifd = NULL;

     // check for no resolution
     if ( *impactTime >= TIME_TO_RUN_IMPACT )
     return FALSE;

     // at this point we've hit the ground
      // Interpolate for the time
      float delta = (groundZ - lastz) / (z - lastz);
      *impactX = lastx + delta * (x - lastx);
      *impactY = lasty + delta * (y - lasty);
      *impactZ = OTWDriver.GetGroundLevel(*impactX, *impactY);

    */

    float rng = fabs((static_cast<SimVehicleClass*>(parent.get())->ZPos() - OTWDriver.GetGroundLevel(x, y))
                     / (tan(static_cast<SimVehicleClass*>(parent.get())->Pitch() - 0.01f)));
    float dx = sin(static_cast<SimVehicleClass*>(parent.get())->Yaw()) * rng;
    float dy = cos(static_cast<SimVehicleClass*>(parent.get())->Yaw()) * rng;
    *impactX = dx + static_cast<SimVehicleClass*>(parent.get())->XPos();
    *impactY = dy + static_cast<SimVehicleClass*>(parent.get())->YPos();
    *impactZ = OTWDriver.GetGroundLevel(*impactX, *impactY);

    if ((fp) and (rng < 11000.f))
    {
        float dz = *impactZ - static_cast<SimVehicleClass*>(parent.get())->ZPos();
        float pel = static_cast<SimVehicleClass*>(parent.get())->Pitch() * RTD;
        float paz = static_cast<SimVehicleClass*>(parent.get())->Yaw() * RTD;
        float PipAz = ((float)atan2(dx, dy) * RTD) - paz;
        float PipEl = ((float)atan(-dz / (float)sqrt(dx * dx + dy * dy + .1F)) * RTD) - pel;

        //fprintf(fp,"**--** Rng %f Imp X %f Imp Y %f Imp Z %f Pitch %f Yaw %f pel %f paz %f PipEl %f PipAz %f dx %f dy %f dz %f \n",
        fprintf(fp, "**--** Rng %f Imp X %f Imp Y %f Imp Z %f pel %f paz %f PipEl %f PipAz %f dx %f dy %f dz %f \n",
                rng, dx, dy, dz, pel, paz, PipEl, PipAz, dx, dy, dz);
        //rng, *impactX, *impactY, *impactZ, degtheta, degpsi, pel, paz, PipEl, PipAz, dx, dy, dz);
        fflush(fp);
    }

    return TRUE;
}
