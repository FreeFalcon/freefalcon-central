#include "stdhdr.h"
#include "digi.h"
#include "otwdrive.h"
#include "PilotInputs.h"
#include "campwp.h"
#include "simveh.h"
#include "fcc.h"
#include "simdrive.h"
#include "facbrain.h"
#include "mission.h"
#include "object.h"
#include "MsgInc/FACMsg.h"
#include "MsgInc/ATCMsg.h"
#include "MsgInc/RadioChatterMsg.h"
#include "MsgInc/TankerMsg.h"
#include "falcmesg.h"
#include "falcsess.h"
#include "aircrft.h"
#include "airframe.h"
#include "unit.h"

#include "falcsnd/conv.h"
#include "mesg.h"
#include "airunit.h"
#include "rules.h"
#include "atcbrain.h"
#include "wingorder.h"

// Brain Choices
#define GENERIC_BRAIN     0
#define SEAD_BRAIN        1
#define STRIKE_BRAIN      2
#define INTERCEPT_BRAIN   3
#define AIR_CAP_BRAIN     4
#define AIR_SWEEP_BRAIN   5
#define ESCORT_BRAIN      6
#define WAYPOINTER_BRAIN  7

float get_air_speed(float speed, int altitude);

extern float g_fAIRefuelRange;
extern int g_nSkipWaypointTime;
extern bool g_bAGTargetWPFix;
extern float g_fAIMinWPAlt; // Cobra - Min alt AI will fly Nav WP

void DigitalBrain::FollowWaypoints(void)
{
    // edg double check groundAvoidNeeded if set -- could be stuck there
    if (groundAvoidNeeded and agApproach not_eq AGA_DIVE)  // Cobra - Let rocket and strafing attacks take care of avoidance
        GroundCheck();

    if (self->curWaypoint == NULL and self->FCC->GetStptMode() == FireControlComputer::FCCMarkpoint)
    {
        //   self->af->SetSimpleMode(SIMPLE_MODE_AF);
        self->SetAutopilot(AircraftClass::ThreeAxisAP);
        throtl = UserStickInputs.throttle;
        ThreeAxisAP();
        return;
    }

    if (self->curWaypoint == NULL)
    {
        AddMode(LoiterMode);
        return;
    }

    SimBaseClass* pobj = self->GetCampaignObject()->GetComponentEntity(0);

    if (isWing and (atcstatus not_eq noATC))
    {
        // if we are a wingman and we are taking off or landing
        mpActionFlags[AI_FOLLOW_FORMATION] = FALSE; // don't follow formation
    }
    else if (isWing and not pobj)
    {
        // if we are a wingman and we can't find the leader's pointer
        mpActionFlags[AI_FOLLOW_FORMATION] = FALSE; // don't follow formation
    }
    else if (isWing and (pobj and pobj->OnGround()))
    {
        // if we are a wingman and our leader is on the ground
        mpActionFlags[AI_FOLLOW_FORMATION] = FALSE; // don't follow formation
        mLeaderTookOff = FALSE; // reset leader take off flag

        if (self->curWaypoint->GetWPAction() == WP_ASSEMBLE and (onStation == Arrived or onStation == Stabalizing or onStation == OnStation))
        {
            // if we are at the assemble point
            if (SimLibElapsedTime < self->curWaypoint->GetWPDepartureTime() + 300000)
            {
                // if we have time to kill
                AddMode(LoiterMode); // hang out for a while
            }
        }
    }
    else if (isWing and mLeaderTookOff == FALSE)
    {
        // if i'm a wing and we think that the leader hasn't taken off
        mLeaderTookOff = TRUE; // tell ourselves that the leader tookoff.  to get here the leader must have taken off.
        mpActionFlags[AI_FOLLOW_FORMATION] = TRUE; // follow the leader
    }

    // check to see if we're heading at an IP waypoint
    // if we are, see if the next waypoint is a ground attack type.
    // if it is, setup a GA profile for the attack

    if (((self->curWaypoint->GetWPFlags() bitand WPF_IP) or (GetTargetWPIndex() >= 0 and GetWaypointIndex() == GetTargetWPIndex() - 1)) and 
        agDoctrine == AGD_NONE)
    {
        AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

        if ((self not_eq playerAC or (self->IsPlayer() and self->AutopilotType() == AircraftClass::CombatAP)) or playerAC->FCC->GetStptMode() == FireControlComputer::FCCWaypoint)
        {
            // VWF 5/25/98 for E3
            // get next Waypoint action
            switch (self->curWaypoint->GetNextWP()->GetWPAction())
            {
                case WP_GNDSTRIKE:
                case WP_NAVSTRIKE:
                case WP_STRIKE:
                case WP_BOMB:
                case WP_SAD:
                case WP_SEAD:
                case WP_CASCP:
                case WP_RECON:
                    SetupAGMode(self->curWaypoint, self->curWaypoint->GetNextWP());
                    break;
            }
        }
    }
    // check for on-the-fly ground attack
    // check for on-the-fly ground attack
    // 2001-06-02 MODIFIED BY S.G. DON'T DO IT IF WE'RE IN WEAPON HOLD (EXCEPT FOR THE LAST SECOND)
    // else if ( agDoctrine not_eq AGD_NONE  )
    else if (agDoctrine not_eq AGD_NONE and SimLibElapsedTime + 1000 >= missileShotTimer)
    {
        if (groundTargetPtr)
        {
            // ground attack
            GroundAttackMode();
            return;
        }

        // no ground target, reset the doctrine back to nothing
        agDoctrine = AGD_NONE;
    }

    // 2001-06-28 MODIFIED BY S.G. DO BOTH WPAction AND WPRouteAction IF NO SPECIFIC WAYPOINT ACTION
    // switch (self->curWaypoint->GetWPAction())
    switch (self->curWaypoint->GetWPAction() == WP_NOTHING ? self->curWaypoint->GetWPRouteAction() : self->curWaypoint->GetWPAction())
    {
        case WP_GNDSTRIKE:
        case WP_NAVSTRIKE:
        case WP_STRIKE:
        case WP_BOMB:
        case WP_SAD:
        case WP_SEAD:
        case WP_CASCP:
        case WP_RECON:

            // check to see if we've already got a profile
            // 2001-06-02 ADDED BY S.G. DON'T DO IT IF WE'RE IN WEAPON HOLD (EXCEPT FOR THE LAST SECOND)
            if (SimLibElapsedTime + 1000 < missileShotTimer)
            {
                self->theInputs->pickleButton = PilotInputs::Off;

                if (((AircraftClass*) self)->af->GetSimpleMode())
                {
                    SimpleGoToCurrentWaypoint();
                }
                else
                {
                    GoToCurrentWaypoint();
                }
            }
            // END OF ADDED SECTION EXCEPT FOR THE LINE INDENTS
            else
            {
                if (agDoctrine == AGD_NONE and onStation not_eq Downwind)
                {
                    SetupAGMode(self->curWaypoint, self->curWaypoint);

                    if (((AircraftClass*) self)->af->GetSimpleMode())
                    {
                        SimpleGoToCurrentWaypoint();
                    }
                    else
                    {
                        GoToCurrentWaypoint();
                    }
                }
                else
                {
                    // fly the ground attack profile
                    GroundAttackMode();
                }
            }

            break;


        case WP_LAND:
            Land();
            break;


        case WP_PICKUP:
        case WP_AIRDROP:
            DoPickupAirdrop();
            break;


        case WP_TAKEOFF:
            AddMode(TakeoffMode);
            TakeOff();
            break;


        case WP_REFUEL:
            self->theInputs->pickleButton = PilotInputs::Off;

            if (((AircraftClass*) self)->af->GetSimpleMode())
            {
                SimpleGoToCurrentWaypoint();
            }
            else
            {
                GoToCurrentWaypoint();
            }

            // If close, set Refuel Mode
            if (fabs(trackX - self->XPos()) < g_fAIRefuelRange * NM_TO_FT and 
                fabs(trackY - self->YPos()) < g_fAIRefuelRange * NM_TO_FT and 
                onStation == NotThereYet)
            {
                VU_ID TankerId = vuNullId;
                AircraftClass *theTanker = NULL;
                FalconTankerMessage *TankerMsg;
                FlightClass *flight;

                onStation = Arrived;

                if (TankerId == FalconNullId)
                    flight = SimDriver.FindTanker(self);
                else
                {
                    flight = (Flight)vuDatabase->Find(TankerId);

                    if ( not flight->IsFlight())
                    {
                        flight = SimDriver.FindTanker(SimDriver.GetPlayerEntity());
                    }
                }

                if (flight)
                    theTanker = (AircraftClass*) flight->GetComponentLead();

                if (theTanker)
                    TankerMsg = new FalconTankerMessage(theTanker->Id(), FalconLocalGame);
                else
                    TankerMsg = new FalconTankerMessage(FalconNullId, FalconLocalGame);

                TankerMsg->dataBlock.type = FalconTankerMessage::RequestFuel;
                TankerMsg->dataBlock.data1  = 1;
                TankerMsg->dataBlock.caller = self->Id();
                FalconSendMessage(TankerMsg);
            }

            break;

        default:
            self->theInputs->pickleButton = PilotInputs::Off;

            if (((AircraftClass*) self)->af->GetSimpleMode())
            {
                SimpleGoToCurrentWaypoint();
            }
            else
            {
                GoToCurrentWaypoint();
            }

            break;
    }
}


void DigitalBrain::SimpleGoToCurrentWaypoint(void)
{
    float xerr, yerr;
    float rng;
    float desSpeed; //desSpeedAlt TJL 02/20/04
    float ttg = 1.0;
    WayPointClass* tmpWaypoint;
    long timeDelta;
    float gainCtrl;
    int vehInFlight;
    int flightIdx;
    ACFormationData::PositionData *curPosition;
    float rangeFactor;
    //int thisWP;
    //int nextWP;

    // prevent too much gain in controls
    // except when we're too close to ground
    gainCtrl = 0.25f;

    // Cobra - Make sure groundAvoidNeeded is not needed
    // if (pullupTimer < SimLibElapsedTime)
    // {
    // groundAvoidNeeded = FALSE;
    // pullupTimer = 0;
    // }

    // see if we're in a ag attack mode
    if (agDoctrine == AGD_NONE)
    {
        float tx, ty, tz;
        self->curWaypoint->GetLocation(&tx, &ty, &tz);
        SetTrackPoint(tx, ty, tz);

        // Adjust position to avoid collision near waypoint
        vehInFlight = ((FlightClass*)self->GetCampaignObject())->GetTotalVehicles();
        flightIdx = ((FlightClass*)self->GetCampaignObject())->GetComponentIndex(self);

        if (flightIdx not_eq 0)
        {
            if (flightIdx == AiFirstWing and vehInFlight == 2)
            {
                curPosition = &(acFormationData->twoposData[mFormation]); // The four ship #2 slot position is copied in to the 2 ship formation array.
            }
            else if (flightIdx == AiSecondWing and mSplitFlight)
            {
                curPosition = &(acFormationData->twoposData[mFormation]);
            }
            else
            {
                curPosition = &(acFormationData->positionData[mFormation][flightIdx - 1]);
            }

            rangeFactor = curPosition->range * (2.0F * mFormLateralSpaceFactor);

            trackX += rangeFactor * (float)cos(curPosition->relAz * mFormSide + self->Yaw());
            trackY += rangeFactor * (float)sin(curPosition->relAz * mFormSide + self->Yaw());

            if (curPosition->relEl)
            {
                trackZ += rangeFactor * (float)sin(-curPosition->relEl);
            }
            else
            {
                trackZ += (flightIdx * -100.0F);
            }
        }
    }
    else
    {
        SetTrackPoint(ipX, ipY, ipZ);
    }

    // edg: according to Kevin, waypoint Z's set for <= 5000.0f indicate
    // terrain following, we'll modify our trackZ accordingly by looking
    // 1 second and getting ground alt then setting ourselves to follow
    // at 500ft.
    // Cobra  - externalized WP min altitude
    if (trackZ >= -5000.0f)
    {
        trackZ = OTWDriver.GetGroundLevel(self->XPos() + self->XDelta(),
                                          self->YPos() + self->YDelta());

        // if we're below track alt, kick us up a bit harder so we don't plow
        // into steeper slopes
        if (self->ZPos() - trackZ > -g_fAIMinWPAlt)
        {
            trackZ = trackZ - g_fAIMinWPAlt - (self->ZPos() - trackZ + g_fAIMinWPAlt) * 2.0f;
            gainCtrl = 1.5f;
        }
        else
            trackZ -= g_fAIMinWPAlt;
    }
    else
    {
        // check to make sure we're above everything around
        float tfloor, tceil;

        OTWDriver.GetAreaFloorAndCeiling(&tfloor, &tceil);

        if (trackZ > tceil - g_fAIMinWPAlt)
        {
            gainCtrl = 1.5f;
            trackZ = tceil - g_fAIMinWPAlt;
        }
    }

    if (curMode not_eq lastMode)
    {
        onStation = NotThereYet;
    }

    /*---------------------------*/
    /* Range to current waypoint */
    /*---------------------------*/
    xerr = trackX - self->XPos();
    yerr = trackY - self->YPos();
    rng = sqrtf(xerr * xerr + yerr * yerr) * FT_TO_NM;

    /*---------------------------*/
    /* Reached the next waypoint? */
    /*---------------------------*/
    // Cobra - Change rng to 2 nm (was 1 nm) to give AI a little more leeway in hitting the WP.
    // Cobra - Added loitering timer check (agmergeTimer)
    if (rng < 2.0f or (onStation not_eq NotThereYet) or (SimLibElapsedTime > self->curWaypoint->GetWPDepartureTime()))
    {
        // Should we repeat?
        if (self and self->curWaypoint and self->curWaypoint->GetWPFlags() bitand (WPF_REPEAT bitor WPF_REPEAT_CONTINUOUS))
        {
            if ((self->curWaypoint->GetWPFlags() bitand WPF_REPEAT_CONTINUOUS) or
                SimLibElapsedTime < self->curWaypoint->GetWPDepartureTime())
            {
                // Find prev waypoint
                tmpWaypoint = self->curWaypoint->GetPrevWP();

                // Get travel time between points
                timeDelta = self->curWaypoint->GetWPArrivalTime() - tmpWaypoint->GetWPDepartureTime();

                // Reset arrival and departure points for first waypoint
                tmpWaypoint->SetWPArrive(self->curWaypoint->GetWPArrivalTime() + timeDelta);
                tmpWaypoint->SetWPDepartTime(self->curWaypoint->GetWPArrivalTime() + timeDelta);

                // reset arrival time for end waypoint
                self->curWaypoint->SetWPArrive(self->curWaypoint->GetWPArrivalTime() + 2 * timeDelta);

                // If continuous, reset current departure time
                if (self->curWaypoint->GetWPFlags() bitand WPF_REPEAT_CONTINUOUS)
                {
                    self->curWaypoint->SetWPDepartTime(self->curWaypoint->GetWPArrivalTime() + 2 * timeDelta);
                }

                // set current waypoint to prev
                self->curWaypoint = tmpWaypoint;

                onStation = NotThereYet;
                SetWaypointSpecificStuff();
            }
            else
            {
                SelectNextWaypoint();
            }

            SimpleTrack(SimpleTrackDist, 0.0F);
        }
        else if (onStation == NotThereYet)
        {
            onStation = Arrived;
            SimpleTrack(SimpleTrackDist, 0.0F);
        }
        else if (rng < 2.0F and onStation == Arrived)
        {
            if (GetTargetWPIndex() >= 0 and GetWaypointIndex() <= GetTargetWPIndex())
            {
                SelectNextWaypoint();
                SimpleTrack(SimpleTrackDist, 0.0F);
            }
            else if (GetWaypointIndex() not_eq GetTargetWPIndex())
            {
                SelectNextWaypoint();
                SimpleTrack(SimpleTrackDist, 0.0F);
            }
        }
        // edg: if we're within 30 secs just go to next ....
        else if (SimLibElapsedTime + g_nSkipWaypointTime > self->curWaypoint->GetWPDepartureTime())
        {
            /* 2002-04-05 MN
            We have a problem here. With the new code that also allows AG missions to engage BVR/WVR, and the new flightmodels,
            it can easily happen that a flight is too late at its target, especially for AG missions. So if we're on an AG mission,
            have not yet completed the mission, are not in RTB mode or landing mode, don't skip the target waypoint.

            A better fix would be having the AI increase speed if they are behind scheduled waypoint times, but for FalconSP3 this is
            beyond our scope. */

            //Cobra Caught the AI stuck in Crosswind with NULL groundTarget
            // Cobra - Sead AI wouldn't move on when no targets left or exceeded loitering timer
            //Adding in onStation == Crosswind; if we make it here, we are past our waypoint time
            //already so this shouldn't be a problem?
            if (((onStation == Crosswind or (onStation == NotThereYet and missionComplete)) and 
                 groundTargetPtr == NULL) or onStation == OnStation or not (g_bAGTargetWPFix and 
                         self->curWaypoint->GetWPFlags() bitand WPF_TARGET and not missionComplete and missionClass == AGMission and 
                         curMode not_eq RTBMode and curMode not_eq LandingMode))
            {
                // 2002-04-08 MN removed again - this stops AI from going to landing mode at all...
                // JB 020315 Don't skip to the last waypoint unless we're OnStation. Otherwise we may go into landing mode too early.
                // if (onStation == OnStation or self->curWaypoint->GetNextWP() and self->curWaypoint->GetNextWP()->GetWPAction() not_eq WP_LAND)

                // Cobra - Don't skip the WP after target WP
                if (GetTargetWPIndex() >= 0 and GetWaypointIndex() <= GetTargetWPIndex())
                    SelectNextWaypoint();
            }

            SimpleTrack(SimpleTrackDist, 0.0F);
        }
        else
        {
            Loiter();
        }
    }
    else
    {
        // Time left to target
        if (self and self->curWaypoint)
            ttg = (self->curWaypoint->GetWPArrivalTime() - SimLibElapsedTime) / (float)SEC_TO_MSEC;

        //TJL 11/09/03 Make speed aircraft friendly. What about the A-10, it's not seeing 700.0f
        // Changed to from 700.0F to 1.3 of corner
        if (ttg < 0.0F)
        {
            // If we're late
            desSpeed = 1.3F * cornerSpeed;
        }
        else
        {
            //TJL 02/20/04 TEST of new range/speed code
            //desSpeed = (float) sqrt(rng) / ttg * FTPSEC_TO_KNOTS;
            rng = sqrtf(xerr * xerr + yerr * yerr);
            desSpeed = ((rng / ttg) * FTPSEC_TO_KNOTS);
        }

        //TJL 02/20/04 We don't need this. MinVcas should be set in ac.dat file
        //as min speed Sea level.  Sim will take into account that speed at altitude
        //Mnvers.cpp will catch speed below min vcas

        /*      desSpeedAlt = get_air_speed (desSpeed, -1*FloatToInt32(self->ZPos()));
              while (desSpeedAlt < af->MinVcas() * 0.85F)
              {
                 desSpeed *= 1.1F;
                 desSpeedAlt = get_air_speed (desSpeed, -1*FloatToInt32(self->ZPos()));
              }
        */

        if (curMode == RTBMode)
            desSpeed = cornerSpeed;

        //TJL 02/20/04 Speed is in KNOTS
        //TrackPoint(0.0F, desSpeed * KNOTS_TO_FTPSEC);
        TrackPoint(0.0F, desSpeed);
    }

    // modify p and r stick settings by gain control
    pStick *= gainCtrl;
    // rStick *= gainCtrl;
}


void DigitalBrain::GoToCurrentWaypoint(void)
{
    float rng, desHeading;
    float desLoad;
    float ttg, desSpeed;
    float wpX, wpY, wpZ;
    WayPointClass* tmpWaypoint;
    long timeDelta;
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    if (self == playerAC and playerAC->FCC->GetStptMode() not_eq FireControlComputer::FCCWaypoint)
    {
        return;  // VWF 5/25/98 for E3
    }

    self->curWaypoint->GetLocation(&wpX, &wpY, &wpZ);
    SetTrackPoint(wpX, wpY, wpZ);

    if (curMode not_eq lastMode)
    {
        onStation = NotThereYet;
        holdAlt = -wpZ;
        waypointMode = 1;
    }

    /*---------------------------*/
    /* Range to current waypoint */
    /*---------------------------*/
    rng = (wpX - self->XPos()) * (wpX - self->XPos()) + (wpY - self->YPos()) * (wpY - self->YPos());

    /*------------------------------------*/
    /* Heading error for current waypoint */
    /*------------------------------------*/
    desHeading = (float)atan2(wpY - self->YPos(), wpX - self->XPos()) - af->sigma;

    if (desHeading > 180.0F * DTR)
        desHeading -= 360.0F * DTR;
    else if (desHeading < -180.0F * DTR)
        desHeading += 360.0F * DTR;

    /*------------------------------------*/
    /* Radius of the turn at the waypoint */
    /*------------------------------------*/
    desLoad = 2.0F * maxGs * desHeading / (180.0F * DTR);

    if (desLoad < 0.0F)
        desLoad = -desLoad;

    desLoad = min(max(1.25F, desLoad), 2.0F);

    /*---------------------------*/
    /* Reached the next waypoint */
    /*---------------------------*/
    //            0.83 NM
    // if (rng < (5000.0F * 5000.0F) or (onStation not_eq NotThereYet) or
    // Cobra - Give AI plenty of leeway to reach WP
    //            1.66 NM
    if (rng < (10000.0F * 10000.0F) or (onStation not_eq NotThereYet) or
        SimLibElapsedTime > self->curWaypoint->GetWPDepartureTime())
    {
        // Should we repeat?
        if (self->curWaypoint->GetWPFlags() bitand (WPF_REPEAT bitor WPF_REPEAT_CONTINUOUS))
        {
            if ((self->curWaypoint->GetWPFlags() bitand WPF_REPEAT_CONTINUOUS) or
                SimLibElapsedTime < self->curWaypoint->GetWPDepartureTime())
            {
                // Find prev waypoint
                tmpWaypoint = self->curWaypoint->GetPrevWP();

                // Get travel time between points
                timeDelta = self->curWaypoint->GetWPArrivalTime() - tmpWaypoint->GetWPDepartureTime();

                // Reset arrival and departure points for first waypoint
                tmpWaypoint->SetWPArrive(self->curWaypoint->GetWPArrivalTime() + timeDelta);
                tmpWaypoint->SetWPDepartTime(self->curWaypoint->GetWPArrivalTime() + timeDelta);

                // reset arrival time for end waypoint
                self->curWaypoint->SetWPArrive(self->curWaypoint->GetWPArrivalTime() + 2 * timeDelta);

                // If continuous, reset current departure time
                if (self->curWaypoint->GetWPFlags() bitand WPF_REPEAT_CONTINUOUS)
                {
                    self->curWaypoint->SetWPDepartTime(self->curWaypoint->GetWPArrivalTime() + 2 * timeDelta);
                }

                // set current waypoint to prev
                self->curWaypoint = tmpWaypoint;

                onStation = NotThereYet;
                SetWaypointSpecificStuff();
            }
            else
            {
                SelectNextWaypoint();
            }
        }
        else if (onStation == NotThereYet)
        {
            holdAlt = -wpZ;
            gammaHoldIError = 0.0F;
            onStation = Arrived;
        }
        else if (SimLibElapsedTime + g_nSkipWaypointTime > self->curWaypoint->GetWPDepartureTime())
        {
            /* 2002-04-05 MN
            We have a problem here. With the new code that also allows AG missions to engage BVR/WVR, and the new flightmodels,
            it can easily happen that a flight is too late at its target, especially for AG missions. So if we're on an AG mission,
            have not yet completed the mission, are not in RTB mode or landing mode, don't skip the target waypoint. */

            // mind the  check here 
            if (onStation == OnStation or not (g_bAGTargetWPFix and 
                                            self->curWaypoint->GetWPFlags() bitand WPF_TARGET and 
 not missionComplete and missionClass == AGMission and 
                                            curMode not_eq RTBMode and curMode not_eq LandingMode))
            {
                // JB 020315 Don't skip to the last waypoint unless we're OnStation. Otherwise we may go into landing mode too early.
                if (onStation == OnStation or self->curWaypoint->GetNextWP() and self->curWaypoint->GetNextWP()->GetWPAction() not_eq WP_LAND)
                {
                    // Cobra - Don't skip the WP after target WP
                    if (GetTargetWPIndex() >= 0 and GetWaypointIndex() <= GetTargetWPIndex())
                        SelectNextWaypoint();
                }
            }
        }
        // Cobra - Give AI plenty of leeway to reach WP
        //            1.66 NM
        else if (rng < (10000.0F * 10000.0F))
            SelectNextWaypoint();

        desSpeed = cornerSpeed;
    }
    else
    {
        /*---------------------*/
        /* Time left to target */
        /*---------------------*/
        ttg = (self->curWaypoint->GetWPArrivalTime() - SimLibElapsedTime) / (float)SEC_TO_MSEC;

        if (ttg < 0.0)
            // TJL 11/09/03 2.0? changed to something reasonable 1.3.  Should help stop AI flameouts if late.
            desSpeed = 1.3F * cornerSpeed;
        else
            desSpeed = (sqrtf(rng) / ttg) * FTPSEC_TO_KNOTS;

        desSpeed = max(desSpeed, 200.0F);
    }

    if (curMode == RTBMode)
        desSpeed = cornerSpeed;

    /*--------------*/
    /* On Station ? */
    /*--------------*/
    if (onStation not_eq NotThereYet)
    {
        if (self->GetKias() < 0.8F * cornerSpeed and onStation == Arrived)
        {
            AltitudeHold(-wpZ);
            MachHold(cornerSpeed, self->GetKias(), TRUE);
        }
        else
        {
            if (onStation == Arrived)
            {
                onStation = Stabalizing;
            }

            if (onStation == Stabalizing)
            {
                if (MachHold(cornerSpeed, self->GetKias(), TRUE))
                {
                    onStation = OnStation;
                    LevelTurn(2.0F, 1.0F, waypointMode);
                }

                AltitudeHold(holdAlt);
            }
            else
            {
                LevelTurn(2.0F, 1.0F, FALSE);
            }
        }
    }
    /*-------------------------------------*/
    /* Level turn if we are far off course */
    /*-------------------------------------*/
    else if (waypointMode not_eq 2)
    {
        if (waypointMode not_eq 0)
        {
            holdAlt = -self->ZPos();
        }

        if (desHeading > 3.0F * DTR)
        {
            LevelTurn(desLoad, 1.0F, waypointMode);
            waypointMode = 0;
        }
        else if (desHeading < -3.0F * DTR)
        {
            LevelTurn(desLoad, -1.0F, waypointMode);
            waypointMode = 0;
        }
        else
        {
            waypointMode = 2;
            holdAlt = -wpZ;
        }
    }
    else
    {
        /*--------------------------*/
        /* Skid to nail the heading */
        /*--------------------------*/
        AltitudeHold(-trackZ);
        SetYpedal(desHeading * 0.05F * RTD * self->GetVt() / cornerSpeed);

        if (fabs(desHeading) > 10.0F * DTR)
        {
            waypointMode = 1;
        }
    }

    MachHold(desSpeed, self->GetKias(), FALSE);
}

void DigitalBrain::SelectNextWaypoint(void)
{
    WayPointClass* tmpWaypoint = self->curWaypoint;
    WayPointClass* wlist = self->waypoint;
    UnitClass *campUnit = NULL;
    WayPointClass *campCurWP = NULL;
    int waypointIndex, i;

    ShiAssert( not self->OnGround());

    // first get our current waypoint index in the list
    for (waypointIndex = 0;
         wlist and wlist not_eq tmpWaypoint;
         wlist = wlist->GetNextWP(), waypointIndex++);

    // see if we're running in tactical or campaign.  If so, we want to
    // synch the campaign's waypoints with ours
    // if ( SimDriver.RunningCampaignOrTactical() )
    {
        // get the pointer to our campaign unit
        campUnit = (UnitClass *)self->GetCampaignObject();

        if (campUnit)   // sanity check
        {
            campCurWP = campUnit->GetFirstUnitWP();

            // now get the camp waypoint that corresponds to our next in the
            // list by index
            for (i = 0; i <= waypointIndex; i++)
            {
                if (campCurWP)   // sanity check
                    campCurWP = campCurWP->GetNextWP();
            }
        }
    }

    onStation = NotThereYet;

    if (self->curWaypoint) // JB 011016 CTD fix
        self->curWaypoint = self->curWaypoint->GetNextWP();

    // KCK: This isn't working anyway - so I'm commentting it out in order to prevent bugs
    // in the ATC and Campaign
    // edg: this should be OK now that an obsolute waypoint index is used to
    // synch the current WP between sim and camp units.
    if (campCurWP)
    {
        campUnit->SetCurrentUnitWP(campCurWP);
    }

    waypointMode = 0;

    if ( not self->curWaypoint)
    {
        // No current waypoint, so go home

        // JB 010715 If the Digi has a ground target it may switch
        // between the second to last and last waypoint continuously.
        groundTargetPtr = NULL;

        wlist = self->waypoint;
        waypointIndex = 0;

        while (wlist)
        {
            if (wlist->GetWPAction() == WP_LAND)
            {
                break;
            }

            waypointIndex ++;
            wlist = wlist->GetNextWP();
        }

        if (wlist)
        {
            // get the pointer to our campaign unit
            campUnit = (UnitClass *)self->GetCampaignObject();

            if (campUnit)   // sanity check
            {
                campCurWP = campUnit->GetFirstUnitWP();

                // now get the camp waypoint that corresponds to our next in the
                // list by index
                for (i = 0; i <= waypointIndex; i++)
                {
                    if (campCurWP)   // sanity check
                        campCurWP = campCurWP->GetNextWP();
                }
            }

            self->curWaypoint = wlist;
        }
        else
        {
            // go back to the beginning
            self->curWaypoint = self->waypoint;
            campUnit->SetCurrentUnitWP(campUnit->GetFirstUnitWP());
        }
    }
    else if ( not (tmpWaypoint->GetWPFlags() bitand WPF_REPEAT) and 
             (self->curWaypoint->GetWPFlags() bitand WPF_REPEAT))
    {
        if (self->curWaypoint->GetWPFlags() bitand WPF_IP)
        {
            SetATCFlag(ReachedIP);
        }

        if ( not (moreFlags bitand SaidSunrise)) // only say sunrise once and only insert once into FAC list
        {
            moreFlags or_eq SaidSunrise;

            switch (tmpWaypoint->GetWPAction())
            {
                case WP_FAC:
                    SimDriver.facList->ForcedInsert(self);
                    break;

                case WP_ELINT:
                    FalconRadioChatterMessage* radioMessage = new FalconRadioChatterMessage(self->Id(), FalconLocalSession);
                    radioMessage->dataBlock.from = self->Id();
                    radioMessage->dataBlock.to = MESSAGE_FOR_TEAM;
                    radioMessage->dataBlock.voice_id = ((Flight)(self->GetCampaignObject()))->GetPilotVoiceID(self->vehicleInUnit);
                    radioMessage->dataBlock.message = rcAWACSON;
                    radioMessage->dataBlock.edata[0] = -1;
                    radioMessage->dataBlock.edata[1] = -1;
                    radioMessage->dataBlock.edata[2] = self->GetCallsignIdx();
                    radioMessage->dataBlock.edata[3] = self->vehicleInUnit + 1;
                    FalconSendMessage(radioMessage, FALSE);
                    // PlayRadioMessage (rcAWACSON)
                    // self is a pointer to the AC that is going on-line
                    break;
            }
        }
    }
    else if ((tmpWaypoint->GetWPFlags() bitand WPF_REPEAT) and 
 not (self->curWaypoint->GetWPFlags() bitand WPF_REPEAT))
    {
        switch (tmpWaypoint->GetWPAction())
        {
            case WP_FAC:
                SimDriver.facList->Remove(self);
                break;

            case WP_ELINT:
                if (((Flight)self->GetCampaignObject())->GetFlightLeadSlot() == self->vehicleInUnit)
                {
                    FalconRadioChatterMessage* radioMessage = new FalconRadioChatterMessage(self->Id(), FalconLocalSession);
                    radioMessage->dataBlock.from = self->Id();
                    radioMessage->dataBlock.to = MESSAGE_FOR_TEAM;
                    radioMessage->dataBlock.voice_id = ((Flight)(self->GetCampaignObject()))->GetPilotVoiceID(self->vehicleInUnit);
                    radioMessage->dataBlock.message = rcAWACSOFF;
                    radioMessage->dataBlock.edata[0] = -1;
                    radioMessage->dataBlock.edata[1] = -1;
                    radioMessage->dataBlock.edata[2] = self->GetCallsignIdx();
                    radioMessage->dataBlock.edata[3] = self->vehicleInUnit + 1;
                    FalconSendMessage(radioMessage, FALSE);
                }

                // PlayRadioMessage (rcAWACSOFF)
                // self is a pointer to the AC that is going on-line
                break;
        }
    }

    // 2001-07-04 ADDED BY S.G. RE_EVALUATE YOUR GROUND WEAPONS WHEN SWITCHING WAYPOINT...
    if ( not IsSetATC(HasAGWeapon) and (missionClass == AGMission))
    {
        MissionClassEnum tmpMission = missionClass;
        //missionClass = AAMission; // Without this, SelectGroundWeapon might call SelectNextWaypoint which will result in a stack overflow
        SelectGroundWeapon();
        missionClass = tmpMission;
    }

    // END OF ADDED SECTION

    SetWaypointSpecificStuff();
}

void DigitalBrain::SetWaypointSpecificStuff(void)
{
    WayPointClass* wlist;
    int waypointIndex;



    if (self->curWaypoint)
    {
        switch (self->curWaypoint->GetWPAction())
        {
            case WP_AIRDROP:
                if (RuleMode not_eq rINSTANT_ACTION and RuleMode not_eq rDOGFIGHT)
                {
                    if (((Flight)self->GetCampaignObject())->GetFlightLeadSlot() == self->vehicleInUnit)
                    {
                        FalconRadioChatterMessage* radioMessage = new FalconRadioChatterMessage(self->Id(), FalconLocalSession);
                        radioMessage->dataBlock.from = self->Id();
                        radioMessage->dataBlock.to = MESSAGE_FOR_TEAM;
                        radioMessage->dataBlock.voice_id = ((Flight)(self->GetCampaignObject()))->GetPilotVoiceID(self->vehicleInUnit);
                        radioMessage->dataBlock.message = rcAIRDROPAPPROACH;
                        radioMessage->dataBlock.edata[0] = self->GetCallsignIdx();
                        radioMessage->dataBlock.edata[1] = self->vehicleInUnit + 1;
                        FalconSendMessage(radioMessage, FALSE);
                    }
                }

                // PlayRadioMessage (rcAIRDROPDONE)
                // self is a pointer to the AC that has dropped troops/cargo
                break;

            case WP_ELINT:
            case WP_NOTHING:
            case WP_TAKEOFF:
            case WP_ASSEMBLE:

                //TJL 11/10/03 Adding sounds?

            case WP_POSTASSEMBLE:
            case WP_REFUEL:
            case WP_REARM:
            case WP_LAND:
            case WP_RECON:
            case WP_RESCUE:
            case WP_ASW:
            case WP_TANKER:
            case WP_JAM:
            case WP_FAC:
                break;

            case WP_ESCORT:
                break;

            case WP_CA:
                break;

            case WP_CAP:
                break;

            case WP_INTERCEPT:
                break;

            case WP_GNDSTRIKE:
            case WP_NAVSTRIKE:
            case WP_STRIKE:
            case WP_BOMB:
            case WP_SAD:
                //TJL 11/10/03 Sounds?
#if 0 // Retro 20May2004 - fixed logic
                if (missionType == (AMIS_OCASTRIKE or AMIS_INTSTRIKE or AMIS_STRIKE or AMIS_DEEPSTRIKE or AMIS_STSTRIKE or AMIS_STRATBOMB))
#else
                if ((missionType == AMIS_OCASTRIKE) or
                    (missionType == AMIS_INTSTRIKE) or
                    (missionType == AMIS_STRIKE) or
                    (missionType == AMIS_DEEPSTRIKE) or
                    (missionType == AMIS_STSTRIKE) or
                    (missionType == AMIS_STRATBOMB))
#endif // Retro 20May2004 - end
                {
                    FalconRadioChatterMessage* radioMessage = new FalconRadioChatterMessage(self->Id(), FalconLocalSession);
                    radioMessage->dataBlock.from = self->Id();
                    radioMessage->dataBlock.to = MESSAGE_FOR_PACKAGE;
                    radioMessage->dataBlock.voice_id = ((Flight)(self->GetCampaignObject()))->GetPilotVoiceID(self->vehicleInUnit);
                    radioMessage->dataBlock.message = rcFLIGHTIN;
                    radioMessage->dataBlock.edata[0] = ((Flight)this)->callsign_id;
                    radioMessage->dataBlock.edata[1] = ((Flight)this)->GetFlightLeadCallNumber();
                    radioMessage->dataBlock.edata[2] = rand() % 12;
                }

                //End


                if (missionType == AMIS_CAS)
                {
                    FalconFACMessage* facMsg;

                    facMsg = new FalconFACMessage(self->Id(), FalconLocalGame);
                    facMsg->dataBlock.type = FalconFACMessage::CheckIn;
                    FalconSendMessage(facMsg, FALSE);
                }

                break;

            case WP_SEAD:
                break;

            default:
                //            MonoPrint ("Why am I here (Digi GetBrain)\n");
                break;
        }
    }
    else
    {
    }

    // For a player, make sure the HUD and ICP know the current waypoint
    if (self == SimDriver.GetPlayerEntity())
    {
        for (waypointIndex = 0, wlist = self->waypoint;
             wlist and wlist not_eq self->curWaypoint;
             wlist = wlist->GetNextWP(), waypointIndex++);

        self->FCC->SetWaypointNum(waypointIndex);
    }

    // Marco edit - set Formation depending on waypoint selected
    if (SimDriver.GetPlayerEntity() not_eq self and not isWing and self->curWaypoint->GetWPFormation() not_eq mCurFormation)
    {
        mCurFormation = self->curWaypoint->GetWPFormation() ;
        AiSendCommand(self, mCurFormation, AiFlight, FalconNullId);
    }

    // End Marco Edit
}

// Cobra - Utility to get current WP index
int DigitalBrain::GetWaypointIndex(void)
{
    WayPointClass* tmpWaypoint = self->curWaypoint;
    WayPointClass* wlist = self->waypoint;
    int waypointIndex;

    // get our current waypoint index in the list
    for (waypointIndex = 0;
         wlist and wlist not_eq tmpWaypoint;
         wlist = wlist->GetNextWP(), waypointIndex++);

    return waypointIndex;
}

// Cobra - Utility to get target WP index
int DigitalBrain::GetTargetWPIndex(void)
{
    WayPointClass* wlist = self->waypoint;
    int waypointIndex = 0;

    // get the target waypoint index in the list
    while (wlist)
    {
        if (wlist->GetWPFlags() bitand WPF_TARGET)
            break;

        wlist = wlist->GetNextWP();
        waypointIndex++ ;
    }

    if (wlist)
        return waypointIndex;
    else
        return -1;
}


/*
** DoPickupAirdrop: This is a cheesy routine to allow troops to be picked
** up or airdropped.  Given time constraints we're just going to fly in
** low and slow and pick them up or drop them off with the campaign calls.
*/
void
DigitalBrain::DoPickupAirdrop(void)
{
    float xerr, yerr;
    float rng;
    float desSpeed, desAlt;
    float gainCtrl;
    Unit cargo, unit;
    GridIndex x, y;

    // prevent too much gain in controls
    // except when we're too close to ground
    gainCtrl = 0.25f;
    onStation = NotThereYet; // jpo - force it to keep going to pickup
    // get where we're supposed to go
    float tx, ty, tz;
    self->curWaypoint->GetLocation(&tx, &ty, &tz);
    SetTrackPoint(tx, ty, tz);

    // range info
    xerr = trackX - self->XPos();
    yerr = trackY - self->YPos();
    rng = xerr * xerr + yerr * yerr;

    // if we're still far away just use the regular waypoint stuff
    if (rng > 10000.0f * 10000.0f)
    {
        if (((AircraftClass*) self)->af->GetSimpleMode())
        {
            SimpleGoToCurrentWaypoint();
        }
        else
        {
            GoToCurrentWaypoint();
        }

        return;
    }

    // now pick our speed based on final approach range
    if (rng < 5000.0f * 5000.0f)
    {
        desSpeed = 50.0f * KNOTS_TO_FTPSEC;
        desAlt = 40.0f;
    }
    else
    {
        desSpeed = 150.0f * KNOTS_TO_FTPSEC;
        desAlt = 500.0f;
    }

    // terrain follwing....
    trackZ = OTWDriver.GetGroundLevel(self->XPos() + self->XDelta(),
                                      self->YPos() + self->YDelta());

    // if we're below track alt, kick us up a bit harder so we don't plow
    // into steeper slopes
    if (self->ZPos() - trackZ > -desAlt)
    {
        trackZ = trackZ - 500.0f - (self->ZPos() - trackZ + 500.0f) * 2.0f;
        gainCtrl = 1.5f;
    }
    else
        trackZ -= desAlt;

    // are we there yet?
    if (rng < 500.0f * 500.0f)
    {
        if (self->curWaypoint->GetWPAction() == WP_PICKUP)
        {
            // Load the airborne battalion.
            cargo = (Unit) self->curWaypoint->GetWPTarget();
            unit = (Unit)self->GetCampaignObject();

            if (cargo and unit)
            {
                unit->SetCargoId(cargo->Id());
                cargo->SetCargoId(unit->Id());
                cargo->SetInactive(1);
                unit->LoadUnit(cargo);

                if (RuleMode not_eq rINSTANT_ACTION and RuleMode not_eq rDOGFIGHT)
                {
                    if (((Flight)self->GetCampaignObject())->GetFlightLeadSlot() == self->vehicleInUnit)
                    {
                        FalconRadioChatterMessage* radioMessage = new FalconRadioChatterMessage(self->Id(), FalconLocalSession);
                        radioMessage->dataBlock.from = self->Id();
                        radioMessage->dataBlock.to = MESSAGE_FOR_TEAM;
                        radioMessage->dataBlock.voice_id = ((Flight)(self->GetCampaignObject()))->GetPilotVoiceID(self->vehicleInUnit);
                        radioMessage->dataBlock.message = rcPACKJOINED; // best I can find - JPO
                        radioMessage->dataBlock.edata[0] = self->GetCallsignIdx();
                        radioMessage->dataBlock.edata[1] = self->vehicleInUnit + 1;
                        FalconSendMessage(radioMessage, FALSE);
                    }
                }
            }
        }
        else if (self->curWaypoint->GetWPAction() == WP_AIRDROP)
        {
            // Load the airborne battalion.
            cargo = (Unit) self->curWaypoint->GetWPTarget();
            unit = (Unit)self->GetCampaignObject();

            if (cargo and unit and unit->Cargo())
            {
                unit->UnloadUnit();
                cargo->SetCargoId(FalconNullId);
                cargo->SetInactive(0);
                self->curWaypoint->GetWPLocation(&x, &y);
                cargo->SetLocation(x, y);

                if (RuleMode not_eq rINSTANT_ACTION and RuleMode not_eq rDOGFIGHT)
                {
                    if (((Flight)self->GetCampaignObject())->GetFlightLeadSlot() == self->vehicleInUnit)
                    {
                        FalconRadioChatterMessage* radioMessage = new FalconRadioChatterMessage(self->Id(), FalconLocalSession);
                        radioMessage->dataBlock.from = self->Id();
                        radioMessage->dataBlock.to = MESSAGE_FOR_TEAM;
                        radioMessage->dataBlock.voice_id = ((Flight)(self->GetCampaignObject()))->GetPilotVoiceID(self->vehicleInUnit);
                        radioMessage->dataBlock.message = rcAIRDROPDONE;
                        radioMessage->dataBlock.edata[0] = self->GetCallsignIdx();
                        radioMessage->dataBlock.edata[1] = self->vehicleInUnit + 1;
                        FalconSendMessage(radioMessage, FALSE);
                    }
                }
            }
        }

        SelectNextWaypoint();
    }


    // head to track point
    TrackPoint(0.0F, desSpeed);

    // modify p and r stick settings by gain control
    pStick *= gainCtrl;
    // rStick *= gainCtrl;
}

