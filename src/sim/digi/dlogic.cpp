#include "stdhdr.h"
#include "types.h"
#include "digi.h"
#include "sensors.h"
#include "falcmesg.h"
#include "simveh.h"
#include "fcc.h"
#include "sms.h"
#include "object.h"
#include "MsgInc/airaimodechange.h"
#include "campwp.h"
#include "aircrft.h"
#include "airframe.h"
#include "geometry.h"
#include "simdrive.h"
#include "missile.h"
#include "mission.h"
#include "unit.h"
#include "camp2sim.h"
#include "radar.h"
#include "irst.h"
#include "fakerand.h"
#include "team.h"
#include "flight.h"
#include "wingorder.h"
#include "playerop.h"
#include "classtbl.h"//Cobra

extern bool g_bRequestHelp;
extern bool g_bAIRefuelInComplexAF; // 2002-02-20 S.G.
extern int g_nAirbaseCheck; // 2002-03-11 MN
extern bool g_bAGNoBVRWVR; // 2002-04-12 MN
extern bool g_bCheckForMode; // 2002-04-14 MN

void DigitalBrain::SetCurrentTactic(void)
{
    DecisionLogic();

    af->SetSimpleMode(SelectFlightModel());
}

void DigitalBrain::DecisionLogic(void)
{
    UnitClass *campUnit = (UnitClass *)self->GetCampaignObject();
    WayPointClass *dwp = NULL;
    CampBaseClass *diverttarget = NULL;
    SimBaseClass *airtarget = NULL;

    if (curMode not_eq LandingMode and agApproach not_eq AGA_DIVE) // Cobra - Let rocket and strafing attacks take care of avoidance
        GroundCheck();
    else
        groundAvoidNeeded = FALSE;

    // MN Handle air divert waypoint here - set targetPtr to divert waypoint target
    // We have too many RequestIntercepts in CAMPAIGN code that sets flights to divert status while
    // there seems to be no code to really change a flight's mission to intercept a target


    if (g_bRequestHelp)
    {
        // 2002-01-14 MODIFIED BY S.G. pctStrength only belongs to SimBaseClass. Make sure it's one before checking
        if (airtargetPtr and (airtargetPtr->BaseData()->IsDead() or airtargetPtr->BaseData()->IsExploding() or
                             (airtargetPtr->BaseData()->IsSim() and ((SimBaseClass *)airtargetPtr->BaseData())->pctStrength <= 0.0f)))
        {
            airtargetPtr->Release();
            airtargetPtr = NULL;

            if (isWing)
                AiGoCover();
        }

        Flight flight = ((FlightClass *)campUnit);
        dwp = flight->GetOverrideWP();

        // only if we're not threatened...
        if (threatPtr == NULL and dwp and (dwp->GetWPFlags() bitand WPF_REQHELP)) // we've a divert waypoint from a help request
        {
            diverttarget = dwp->GetWPTarget();

            if (diverttarget and diverttarget->IsFlight())
            {
                airtarget = FindSimAirTarget((CampBaseClass*)diverttarget, ((CampBaseClass*)diverttarget)->NumberOfComponents(), 0);

                if ( not airtarget) // We've all targets assigned now, clear the divert waypoint
                    flight->SetOverrideWP(NULL);

                if (airtarget) // it's a new one no other flight member has chosen yet
                {
                    if (airtargetPtr not_eq NULL)
                    {
                        // release existing target data if different object
                        if (airtargetPtr->BaseData() not_eq airtarget)
                        {
                            airtargetPtr->Release();
                            airtargetPtr = NULL;
                        }
                        else
                        {
                            // already targeting this object
                            return;
                        }
                    }

#ifdef DEBUG
                    //airtargetPtr = new SimObjectType( OBJ_TAG, self, (FalconEntity*) airtarget );
#else
                    airtargetPtr = new SimObjectType((FalconEntity*) airtarget);
#endif
                    airtargetPtr->Reference();
                    SetTarget(airtargetPtr);

                    if (isWing) // let them loose...
                        AiGoShooter();

                    if ( not isWing) // make a radio call to the team
                    {
                        int flightIdx = self->GetCampaignObject()->GetComponentIndex(self);
                        FalconRadioChatterMessage* radioMessage = new FalconRadioChatterMessage(self->Id(), FalconLocalSession);
                        radioMessage->dataBlock.from = self->Id();
                        radioMessage->dataBlock.to = MESSAGE_FOR_TEAM;
                        radioMessage->dataBlock.voice_id = ((Flight)(self->GetCampaignObject()))->GetPilotVoiceID(self->vehicleInUnit);
                        radioMessage->dataBlock.message = rcREQHELPANSWER;
                        radioMessage->dataBlock.edata[0] = ((FlightClass*)self->GetCampaignObject())->callsign_id;
                        radioMessage->dataBlock.edata[1] = (((FlightClass*)self->GetCampaignObject())->callsign_num - 1) * 4 + flightIdx + 1;
                        radioMessage->dataBlock.edata[2] = 2 * (airtargetPtr->BaseData()->Type() - VU_LAST_ENTITY_TYPE);
                        radioMessage->dataBlock.edata[3] = (short) SimToGrid(airtargetPtr->BaseData()->YPos()); //THW 2003-11-14 Bugfix: Swapped X/Y
                        radioMessage->dataBlock.edata[4] = (short) SimToGrid(airtargetPtr->BaseData()->XPos());
                        radioMessage->dataBlock.edata[5] = (short) airtargetPtr->BaseData()->ZPos();
                        radioMessage->dataBlock.time_to_play = 4000; // 4 seconds
                        FalconSendMessage(radioMessage, FALSE);
                    }
                }
            }
        }
    }

    // Targeting
    // no new targeting while dealing with threat
    // RV - RED - Added check for AG Target (groundTargetPtr) being NULL
    // Fixes AI diverting to not assigned targets
    if (threatPtr == NULL and airtargetPtr == NULL and groundTargetPtr == NULL)  // M.N. only retarget if we aren't threatened and don't have a divert air target
    {
        // 2000-09-25 MODIFIED BY S.G. SO WEAPON FREE COMMAND WITH NO DESIGNATED TARGET MAKES THE AI GO AFTER THEIR TARGET...
        // if(isWing and mpActionFlags[AI_ENGAGE_TARGET]) {
        if (isWing and (mpActionFlags[AI_ENGAGE_TARGET] or mWeaponsAction == AI_WEAPONS_FREE))
        {
            AiRunTargetSelection();
        }
        else
        {
            TargetSelection();
        }
    }

    // calculate the relative geom on our target if we have one
    // always when defensive
    if (targetPtr)
    {
        // edg: check for dead targets

        // 2000-09-21 MODIFIED BY S.G. THIS IS NOT ENOUGH. IF THE PLANE IS *DYING*, STOP TARGETING IT. NOT IF IT'S EXPLODING
        // if ( targetPtr->BaseData()->IsExploding() )
        // 2002-01-14 MODIFIED BY S.G. targetPtr->BaseData CAN BE A *CAMPAIGN OBJECT* Don't assume it's a SimBaseClass
        //                             Campaign object do not even have a pctStrength variable which will returned in garbage being used
        if (targetPtr->BaseData()->IsSim() and ((SimBaseClass *)targetPtr->BaseData())->pctStrength <= 0.0f)  // Dying SIM target have a damage less than 0.0f
        {
            // 2000-09-21 MODIFIED BY S.G. SetTarget DOES TOO MUCH. NEED TO CALL ClearTarget INSTEAD WHICH SIMPLY CLEARS IT, NO MATTER WHAT
            // SetTarget( NULL );
            ClearTarget();
        }
        else
        {
            if (curMode <= DefensiveModes or curMode == GunsEngageMode or SimLibElapsedTime > self->nextGeomCalc)
            {
                self->nextGeomCalc += self->geomCalcRate;
                // hack to avoid traversing a list, set the targetPtr's next var
                // to NULL, then restore it
                SimObjectType *savenext;

                savenext = targetPtr->next;
                targetPtr->next = NULL;
                CalcRelGeom(self, targetPtr, ((AircraftClass *)self)->vmat, 1.0F / SimLibMajorFrameTime);
                targetPtr->next = savenext;

                // Monitor rates to check for stagnation
                if (targetPtr == lastTarget)
                {
                    ataddot = ataddot * 0.85F + ataDot * 0.15F;
                    rangeddot = rangeddot * 0.85F + targetPtr->localData->rangedot * 0.15F;
                }
                else
                {
                    ataddot = 10.0F;
                    rangeddot = 10.0F;
                }
            }
        }
    }

    // Maneuver control
    RunDecisionRoutines();

    // Select highest priority mode Resolve mode conflicts
    ResolveModeConflicts();

    // Print mode changes as they occur
    // PrtMode();

    // If I'm a leader or a wingman with permission to shoot and not defensive or in waypoint mode
    // MODIFIED BY S.G. SO AI CAN STILL DEFEND THEMSELF WHEN RETURNING TO BASE (ODDLY ENOUGH, LandingMode IS WHEN RTBing
    //  if(( not isWing or mWeaponsAction == AI_WEAPONS_FREE) and targetPtr and curMode > DefensiveModes and 
    if (( not isWing or mWeaponsAction == AI_WEAPONS_FREE) and targetPtr and (curMode > DefensiveModes or curMode == LandingMode) and 
        (curMode not_eq WaypointMode or agDoctrine == AGD_NONE))
    {
        // Weapon selection
        WeaponSelection();
    }
    else
    {
        // Never hold a missile over multiple frames
        if (curMissile)
        {
            if (curMissile->launchState == MissileClass::PreLaunch)
                curMissile->SetTarget(NULL);

            curMissile = NULL;
        }
    }

    // Now that we know what we are doing tell our wingmen if we have them
    if (CommandTest())
    {
        CommandFlight();
    }

    // 2002-02-20 ADDED BY S.G. Check if we should jettison our tanks...
    if (self->Sms and not self->Sms->DidJettisonedTank())
    {
        if (SkillLevel() > 2)   // Smart one will do it under most condition
        {
            if ((curMode >= GroundAvoidMode and curMode <= MissileDefeatMode) or (curMode >= MissileEngageMode and curMode <= BVREngageMode) or curMode == BugoutMode)
                self->Sms->TankJettison(); // will take care if tanks are empty
        }
        else if (SkillLevel() > 0)   // Not so smart will do it if threathened while dumb one won't do it...
        {
            if ((curMode >= GroundAvoidMode and curMode <= MissileDefeatMode))
                self->Sms->TankJettison(); // will take care if tanks are empty
        }
    }

    // RV - Biker - If low on fuel for sure drop tanks
    if (self->Sms and not self->Sms->DidJettisonedTank())
    {
        switch (SkillLevel())
        {
            case 4:
            case 3:
                if (af->ExternalFuel() <= 1.0f)
                {
                    self->Sms->TankJettison();
                }

                break;

            case 2:
            case 1:
                if (IsSetATC(SaidJoker))
                {
                    self->Sms->TankJettison();
                }

                break;

            case 0:
                if (IsSetATC(SaidBingo))
                {
                    self->Sms->TankJettison();
                }

                break;
        }

    }

    // RV - Biker - When no more fuel drop everything
    if ((IsSetATC(SaidFumes) or IsSetATC(SaidFlameout)) and not self->Sms->DidEmergencyJettison())
    {
        self->Sms->EmergencyJettison();
    }

    // 2002-02-20 ADDED BY S.G. When damaged and going home, why bring the bombs with us...
    if (self->Sms and not self->Sms->DidEmergencyJettison() and self->pctStrength < 0.50F)
    {
        curMissile = NULL;
        self->Sms->EmergencyJettison();
        // Cobra - Why???  Just dumped them all
        //SelectGroundWeapon();
    }
}

void DigitalBrain::RunDecisionRoutines(void)
{
    // If you're on the ground, just taxi
    if ( not self->OnGround())
    {
        // Not done in AiRunDecisionRoutines and must be done by all flight members
        CollisionCheck();
        SeparateCheck();

        // 2002-03-11 MN added - if "SaidFumes", head for nearest friendly airbase
        if (g_nAirbaseCheck)
            AirbaseCheck();

        //Cobra select radar mode
        chooseRadarMode();

        if ( not isWing)
        {
            // Done in AiRunDecisionRoutines as well so limit it to lead in here
            GunsJinkCheck();
            MissileDefeatCheck();

            // Currently flight lead only
            MergeCheck();
            GunsEngageCheck();
            MissileEngageCheck();

            // if we're not on an air-air type mission -- no bvr
            //me123 bvr wil react defensive now too
            // 2002-04-12 MN put back in with config variable - if people prefer AG to not do any BVR/WVR checks
            // - switch might not be made public, but better have the hook in...
            // still look if we have still a weapon before checking for an engagement
            // 2002-04-14 MN saw a flight lead in TakeoffMode asking wingman to engage,
            //wingy taxied to the target...
            if (g_bCheckForMode and curMode not_eq TakeoffMode)
            {
                if ((g_bAGNoBVRWVR and ((missionClass == AAMission or missionComplete) and maxAAWpnRange not_eq 0.0F))
                    or maxAAWpnRange not_eq 0.0F)
                {
                    WvrEngageCheck();
                    BvrEngageCheck();
                }
            }
            else
            {
                if ((g_bAGNoBVRWVR and ((missionClass == AAMission or missionComplete) and maxAAWpnRange not_eq 0.0F))
                    or maxAAWpnRange not_eq 0.0F)
                {
                    WvrEngageCheck();
                    BvrEngageCheck();
                }
            }

            AccelCheck();
        }
        // END OF MODIFIED SECTION
        else
        {
            AiRunDecisionRoutines();
        }

        //if we decided to refuel and there isn't anything more important,
        //refuel
        if (IsSetATC(NeedToRefuel))
            AddMode(RefuelingMode);
    }

    // Check if I should be landing or taking off
    AiCheckLandTakeoff();

    /*------------------*/
    /* default behavior */
    /*------------------*/
    AddMode(WaypointMode);
}

void DigitalBrain::PrtMode(void)
{
    unsigned long tid;

    // for only checking ownship....
    if (self not_eq SimDriver.GetPlayerEntity())
        return;

    if (curMode not_eq lastMode)
    {
        if (targetPtr)
            tid = targetPtr->BaseData()->Id();
        else
            tid = 0;

        switch (curMode)
        {
            case RTBMode:
                PrintOnline("DIGI RTB");
                break;

            case WingyMode:
                PrintOnline("DIGI Wingman");
                break;

            case WaypointMode:
                PrintOnline("DIGI Waypoint");
                break;

            case GunsEngageMode:
                PrintOnline("DIGI Guns Engage");
                break;

            case MergeMode:
                PrintOnline("DIGI Merge");
                break;

            case BVREngageMode:
                PrintOnline("DIGI BVR Engage");
                break;

            case WVREngageMode:
                PrintOnline("DIGI WVR Engage");
                break;

            case MissileDefeatMode:
                PrintOnline("DIGI Missile Defeat");
                break;

            case MissileEngageMode:
                PrintOnline("DIGI Missile Engage");
                break;

            case GunsJinkMode:
                PrintOnline("DIGI Guns Jink");
                break;

            case GroundAvoidMode:
                PrintOnline("DIGI Ground Avoid");
                break;

            case LoiterMode:
                PrintOnline("DIGI Loiter");
                break;

            case CollisionAvoidMode:
                PrintOnline("DIGI Collision");
                break;

            case SeparateMode:
                PrintOnline("DIGI Separate");
                break;

            case BugoutMode:
                PrintOnline("DIGI Bug Out");
                break;

            case RoopMode:
                PrintOnline("DIGI Roop");
                break;

            case OverBMode:
                PrintOnline("DIGI Overb");
                break;

            case AccelMode:
                PrintOnline("DIGI Accelerate");
                break;
        }

        /*
        AirAIModeMsg* modeMsg;

           modeMsg = new AirAIModeMsg (self->Id(), FalconLocalGame);
           modeMsg->dataBlock.gameTime = SimLibElapsedTime;
           modeMsg->dataBlock.whoDidIt = self->Id();
           modeMsg->dataBlock.newMode = curMode;
           FalconSendMessage (modeMsg,FALSE);
              */
    }
}

void DigitalBrain::SetTarget(SimObjectType* newTarget)
{
    short edata[6];
    int response, navAngle;
    float rz;

    // 2000-10-09 REMOVED BY S.G. BECAUSE OF THIS, AI WON'T SWITCH TARGET WHEN ASKED
    // No targeting when on ground attack run(i.e. After IP)
    // 2001-05-05 MODIFIED BY S.G. LETS TRY SOMETHING ELSE INSTEAD
    //if (agDoctrine not_eq AGD_NONE and not madeAGPass)
    if (newTarget and newTarget->BaseData()->GetTeam() == self->GetTeam() and (agDoctrine not_eq AGD_NONE or missionComplete))
    {
        return;
    }

    // 2000-09-21 ADDED BY S.G. DON'T CHANGE TARGET IF WE ARE SUPPORTING OUR SARH MISSILE DAMN IT
    // TODO: Check if 'HandleThreat' is not screwing stuff since it calls WVREngage DIRECTLY
    if (newTarget and // Assigning a target
        newTarget not_eq targetPtr and // It's a new target
        missileFiredEntity and // we launched a missile already
 not ((SimWeaponClass *)missileFiredEntity)->IsDead() and // it's not dead
        ((SimWeaponClass *)missileFiredEntity)->targetPtr and // it's still homing to a target
        ((SimWeaponClass *)missileFiredEntity)->sensorArray and // the missile is local (it has a sensor array)
        (((SimWeaponClass *)missileFiredEntity)->sensorArray[0]->Type() == SensorClass::RadarHoming and 
         ((SimWeaponClass *)missileFiredEntity)->GetSPType() not_eq SPTYPE_AIM120)) // It's still being guided by us
    {
        return; // That's it, don't change target (support your missile)
    }

    // END OF ADDED SECTION

    // Tell someone we're enaging/want to engage an air target of our own volition
    if (newTarget and // Assigning a target
        newTarget not_eq targetPtr and // It's a new target
 not newTarget->BaseData()->OnGround() and // It's not on the ground
        ( not mpActionFlags[AI_ENGAGE_TARGET] and missionClass == AAMission or missionComplete) and // We're not busy doing A/G stuff
        newTarget not_eq threatPtr and // It's not a threat we're reacting to
        isWing and // We're a wingy
        mDesignatedObject == FalconNullId and // We're not being directed
 not self->OnGround()) // We're in the air
    {
        //F4Assert ( not newTarget->BaseData()->IsHelicopter()); // 2002-03-05 Choppers are fare game now under some conditions

        // Ask for permission?
        // 2000-09-25 MODIFIED BY S.G. WHY ASK PERMISSION IF WE HAVE WEAPON FREE?
        if ( not mpActionFlags[AI_ENGAGE_TARGET] and mWeaponsAction == AI_WEAPONS_HOLD)
        {
            if ( not IsSetATC(AskedToEngage))
            {
                SetATCFlag(AskedToEngage);
                edata[0] = ((FlightClass*)self->GetCampaignObject())->callsign_id;
                edata[1] = (((FlightClass*)self->GetCampaignObject())->callsign_num - 1) * 4 + isWing;
                response = rcREQUESTTOENGAGE;
                AiMakeRadioResponse(self, response, edata);

                // sfr: back to 0 so AI can fire
                // RV - Biker
                if (missileShotTimer < SimLibElapsedTime)
                {
                    missileShotTimer = 0;
                    //SimLibElapsedTime + FloatToInt32(5.0f - PRANDFloatPos() * 10.0f + 30.0F * SEC_TO_MSEC);
                }
            }
            else if (SimLibElapsedTime > missileShotTimer)
            {
                // We've waited long enough, go kill something
                ClearATCFlag(AskedToEngage);
                missileShotTimer = 0;
                AiGoShooter();
            }

            return;
        }
        else if (newTarget and (targetPtr == NULL or (newTarget->BaseData() not_eq targetPtr->BaseData())) and newTarget->localData->range < 2.0F * NM_TO_FT)
        {
            ClearATCFlag(AskedToEngage);
            // 2000-09-25 ADDED BY S.G. NEED TO FORCE THE AI TO SHOOT RIGHT AWAY
            missileShotTimer = 0;

            // END OF ADDED SECTION
            if (PlayerOptions.BullseyeOn())
            {
                response = rcENGAGINGA;
            }
            else
            {
                response = rcENGAGINGB;
            }

            edata[0] = ((FlightClass*)self->GetCampaignObject())->callsign_id;
            edata[1] = (((FlightClass*)self->GetCampaignObject())->callsign_num - 1) * 4 + isWing;
            edata[2] = 2 * (newTarget->BaseData()->Type() - VU_LAST_ENTITY_TYPE);
            edata[3] = (short) SimToGrid(newTarget->BaseData()->YPos()); //THW 2003-11-14 Bugfix: Swapped X/Y
            edata[4] = (short) SimToGrid(newTarget->BaseData()->XPos());
            edata[5] = (short) newTarget->BaseData()->ZPos();
            // 2000-09-25 MODIFIED BY S.G. SO AI SAY WHAT IT IS SUPPOSED TO SAY INSTEAD OF 'HELP'
            // AiMakeRadioResponse( self, rcHELPNOW, edata );
            AiMakeRadioResponse(self, response, edata);
        }
        else
        {
            // 2000-09-25 ADDED BY S.G. NEED TO FORCE THE AI TO SHOOT RIGHT AWAY
            missileShotTimer = 0;
            // END OF ADDED SECTION
            edata[0] = 2 * (newTarget->BaseData()->Type() - VU_LAST_ENTITY_TYPE);
            navAngle =  FloatToInt32(RTD * TargetAz(self, newTarget->BaseData()));

            if (navAngle < 0)
            {
                navAngle = 360 + navAngle;
            }

            edata[1] = navAngle / 30; // scale compass angle for radio eData

            if (edata[1] >= 12)
            {
                edata[1] = 0;
            }

            rz = newTarget->BaseData()->ZPos() - self->ZPos();

            if (rz < 300.0F and rz > -300.0F)   // check relative alt and select correct frag
            {
                edata[2] = 1;
            }
            else if (rz < -300.0F and rz > -1000.0F)
            {
                edata[2] = 2;
            }
            else if (rz < -1000.0F)
            {
                edata[2] = 3;
            }
            else
            {
                edata[2] = 0;
            }

            response = rcENGAGINGC;
            // 2000-09-25 MODIFIED BY S.G. SO AI SAY WHAT IT IS SUPPOSED TO SAY INSTEAD OF 'HELP'
            // AiMakeRadioResponse( self, rcHELPNOW, edata );
            AiMakeRadioResponse(self, response, edata);
        }
    }

    // edg: don't set ground targets via this mechanism, ground targeting
    // should always use groundTargetPtr
    // We now divert to SetGroundTargetPtr if target on ground.  Potentially
    // if don't do something with the target is may cause a memory leak of
    // SimObjectTypes.
    if (newTarget and newTarget->BaseData()->OnGround())
    {
        // presumably if we're setting to a new target here, we want to
        // clear the air target (?)
        ClearTarget();
        //ShiAssert(curMode not_eq GunsEngageMode);
        SetGroundTargetPtr(newTarget);
        return;
    }

    // sfr: back to 0 so AI can fire weapons
    // RV - Biker - Don't think this is good idea to do without check for weapons hold
    if (newTarget not_eq targetPtr)
    {
        missileShotTimer = 0;//SimLibElapsedTime + 30 * SEC_TO_MSEC;
    }

    BaseBrain::SetTarget(newTarget);

    // Make sure the radar is pointed at the desired target
    // Special case, people w/ heaters and an IRST and sometimes ACE level w/ heaters
    RadarClass* theRadar = (RadarClass*)FindSensor(self, SensorClass::Radar);

    if (theRadar)
    {
        if (SkillLevel() < 3)
        {
            theRadar->SetDesiredTarget(newTarget);
        }
        else
        {
            IrstClass* theIrst = (IrstClass*)FindSensor(self, SensorClass::IRST);

            if ( not theIrst)
            {
                theRadar->SetDesiredTarget(newTarget);
            }
            else if (
 not curMissile or
                (curMissile->sensorArray and curMissile->sensorArray[0]->Type() not_eq SensorClass::IRST)
            )
            {
                theRadar->SetDesiredTarget(newTarget);
            }
        }
    }

    if (targetPtr)
    {
        // Get all our sensors tracking this guy
        for (int i = 0; i < self->numSensors; i++)
        {
            ShiAssert(self->sensorArray[i]);

            if (self->sensorArray[i]->Type() not_eq SensorClass::TargetingPod)
            {
                self->sensorArray[i]->SetDesiredTarget(targetPtr);
            }
        }
    }
    //edg: don't we want to clear sensor targets when no target?
    else
    {
        for (int i = 0; i < self->numSensors; i++)
        {
            ShiAssert(self->sensorArray[i]);
            self->sensorArray[i]->ClearSensorTarget();
        }
    }
}

void DigitalBrain::PrintOnline(char *str)
{
    int att = self->Id().num_;
    int tid = 0;

    if (targetPtr)
    {
        tid = targetPtr->BaseData()->Id().num_;
    }

    MonoPrint("%8ld %-25s %3d - %-3d -> %3d - %-3d\n", SimLibElapsedTime, str,
              att bitand 0xFFFF, att >> 16, tid bitand 0xFFFF, tid >> 16);
}

// Check to see if we are just going around in circles, and not getting anywhere
int DigitalBrain::Stagnated(void)
{
    int retval = FALSE;

    if (fabs(ataddot) < 4.0F * DTR and fabs(rangeddot) < 50.0F and 
        fabs(self->YawDelta()) > 8.0F * DTR)
    {
        retval = TRUE;
        ataddot = 10.0F;
        rangeddot = 10.0F;
    }

    return (retval);
}

void DigitalBrain::AddMode(DigiMode newMode)
{
    // 2000-11-17 ADDED BY S.G. SO AI CAN BE MORE AGRESSIVE WHEN RTBing
    // Now if the new mode asked is 'LandingMode', and the mode we are asked to go to is a defensive or engagement mode, leave it alone
    if (newMode == LandingMode and (nextMode == DefensiveModes or (nextMode >= MissileEngageMode and nextMode <= WVREngageMode)))


        return;

    //TJL 11/08/03
    // Keep BugoutMode set, but allow for MissileDefeat to override.

    if (nextMode == BugoutMode and newMode not_eq MissileDefeatMode)
        return;




    //ME123  if this is not done you will suffer severe floodign becourse resolvemodeconflict funktion
    // will send an atcstatus = NOATC when entering wvr engage and it will alternate between landing and wvrengage each frame in some situations.
    if (curMode == LandingMode and newMode == WVREngageMode) return;

    // So we're not asking to land but are we in 'LandingMode' already? If so, check if we are engaged or should engage
    if (nextMode == LandingMode and newMode >= MissileEngageMode and newMode <= WVREngageMode)
    {
        nextMode = newMode;
        return;
    }

    // None of the above, to the normal coding
    // END OF ADDED SECTION
    if (newMode < nextMode)
        nextMode = newMode;
}

void DigitalBrain::ResolveModeConflicts(void)
{
    if (threatPtr == NULL and curMode not_eq WVREngageMode)
    {
        wvrCurrTactic = WVR_NONE;
        wvrPrevTactic = WVR_NONE;
    }

    /*--------------------*/
    /* What were we doing */
    /*--------------------*/


    lastMode = curMode;
    curMode = nextMode;
    nextMode = NoMode;



    //we appear to be getting distracted while landing
    //ShiAssert( (atcstatus == noATC) or (curMode == LandingMode or curMode == TakeoffMode or curMode == WaypointMode) );
    if (atcstatus not_eq noATC and curMode not_eq LandingMode and curMode not_eq TakeoffMode and curMode not_eq WaypointMode)
    {
        SendATCMsg(noATC);
        ResetATC();
    }
}

void DigitalBrain::FireControl(void)
{
    float shootShootPct = 0.0F, pct = 0.0F;

    // basic check for firing, time to shoot, have a missile, have a target
    if (SimLibElapsedTime < missileShotTimer or
 not curMissile or not targetPtr
        or F4IsBadReadPtr(curMissile, sizeof(MissileClass)) // JB 010223 CTD
        or F4IsBadReadPtr(self->FCC, sizeof(FireControlComputer)) // JB 010326 CTD
        or F4IsBadReadPtr(self->Sms, sizeof(SMSClass)) // JB 010326 CTD
        or F4IsBadReadPtr(targetPtr, sizeof(SimObjectType)) // JB 010326 CTD
        or F4IsBadReadPtr(targetPtr->localData, sizeof(SimObjectLocalData)) // JB 010326 CTD
        or not curMissile->sensorArray or F4IsBadReadPtr(curMissile->sensorArray, sizeof(SensorClass*)) // M.N. 011114 CTD
       )
    {
        return;
    }

    // Are we cleared to fire?
    if (curMode not_eq MissileEngageMode and not mWeaponsAction == AI_WEAPONS_FREE)
    {
        return;
    }

    // 2000-09-20 S.G. I CHANGED THE CODE SO ONLY ONE AIRPLANE CAN LAUNCH AT ANOTHER AIRPLANE (SAME CODE I ADDED TO 'TargetSelection')
    // me123 commented out for now. it seems the incomign missiles are not getting cleared 
    // 2001-08-31 S.G. FIXED PREVIOUS CODE WAS ASSUMING targetPtr WAS ALWAYS A SIM. IT CAN BE A CAMPAIGN OBJECT AS WELL, HENCE THE CTD.
    // if ((((SimBaseClass *)targetPtr->BaseData())->incomingMissile and ((SimWeaponClass *)((SimBaseClass *)targetPtr->BaseData())->incomingMissile)->parent not_eq self))
    if ((targetPtr->BaseData()->IsAirplane() and ((SimBaseClass *)targetPtr->BaseData())->incomingMissile[1]) or ( not targetPtr->BaseData()->IsAirplane() and ((SimBaseClass *)targetPtr->BaseData())->incomingMissile[0]))
        return;

    //END OF ADDED SECTION

    // Check firing parameters
    // MODIFIED BY S.G. SO IR MISSILE HAVE A VARIABLE ATA
    //   if ( targetData->ata > 20.0f * DTR or

    if (self->FCC->inRange == FALSE or targetData->range < self->FCC->missileRMin or
        targetData->range > self->FCC->missileRMax)
        return;

    //Cobra JB is a crackhead
    /*if (curMissile->sensorArray[0] and 
     (curMissile->sensorArray[0]->Type() == SensorClass::RadarHoming or curMissile->sensorArray[0]->Type() == SensorClass::Radar) and 
     ( targetData->range >
     (self->FCC->missileRMax * (((0.99F - isWing * 0.05f)) *(1.30-1.00f * min(((((float)SkillLevel()/2 )/ ((float)self->Sms->numOnBoard[wcAimWpn]))), 1.0f) *
     ((float)cos(targetPtr->localData->ataFrom/2) * (float)cos(targetPtr->localData->ataFrom/2)))))))
     return;*/

    if // stuff like mavs has 20 degree off bore cabability
    (
        curMissile->sensorArray[0]->Type() not_eq SensorClass::RadarHoming and 
        curMissile->sensorArray[0]->Type() not_eq SensorClass::Radar and 
        curMissile->sensorArray[0]->Type() not_eq SensorClass::IRST and 
        targetData->ata > 20.0f * DTR
    )
        return;

    if // off bore or getting closer to bore
    (curMissile->sensorArray[0]->Type() == SensorClass::RadarHoming and (targetData->ata > 35.0f * DTR/* or  targetData->atadot < 0.0f*/)) // 2002-03-12 MODIFIED BY S.G. and has HIGHER precedence than or
        return;

    // if // don't shoot semis if agregated
    // (curMissile->sensorArray[0]->Type() == SensorClass::RadarHoming and 
    // ((CampBaseClass*)curMissile->parent)->IsAggregate())
    // return;

    if // off bore or getting closer to bore
    (curMissile->sensorArray[0]->Type() == SensorClass::Radar and (targetData->ata > 35.0f * DTR /*or  targetData->atadot < 0.0f*/))  // 2002-03-12 MODIFIED BY S.G. and has HIGHER precedence than or
        return;

    if // irst iff bore
    (
        curMissile->sensorArray[0]->Type() == SensorClass::IRST and targetData->ata >
        ((IrstClass *)curMissile->sensorArray[0])->GetTypeData()->GimbalLimitHalfAngle * 0.95f
    )
        return;

    // ADDED BY S.G. TO MAKE SURE WE DON'T FIRE BEAM RIDER IF THE MAIN RADAR IS JAMMED (NEW: USES SensorTrack INSTEAD of noTrack)
    if (curMissile->sensorArray and curMissile->sensorArray[0]->Type() == SensorClass::RadarHoming
        /* and curMissile->GetSPType() not_eq SPTYPE_AIM120*/ or
        curMissile->sensorArray[0]->Type() == SensorClass::Radar)
    {
        // Find the radar attached to us
        if (targetPtr->localData->sensorState[SensorClass::Radar] not_eq SensorClass::SensorTrack)
        {
            return;
        }

    }

    // WARNING: MIGHT HAVE TO DEAL WITH ARH MISSILE (LIKE AIM120)
    // SINCE GetSeekerType WOULD RETURN SensorClass::Radar
    // END OF ADDED SECTIION
    curMissile->SetTarget(targetPtr);
    self->FCC->SetTarget(targetPtr);

    // Set the flag
    SetFlag(MslFireFlag);

    // Check doctrine
    switch (curMissile->GetSeekerType())
    {
        case SensorClass::Radar:
        case SensorClass::RWR:
        case SensorClass::HTS:
            shootShootPct = TeamInfo[self->GetCountry()]->GetDoctrine()->RadarShootShootPct();
            break;

        case SensorClass::IRST:
        case SensorClass::Visual:
        default:
            shootShootPct = TeamInfo[self->GetCountry()]->GetDoctrine()->HeatShootShootPct();
            break;
    }

    // Roll the 'dice'
    pct = ((float)rand()) / RAND_MAX * 100.0F;

    if (pct < shootShootPct and not IsSetATC(InShootShoot))
    {
        missileShotTimer = SimLibElapsedTime + 4 * SEC_TO_MSEC;
        //MonoPrint ("DIGI BRAIN Firing Missile at Air Unit rng = %.0F: Shoot Shoot\n", targetData->range);
        SetATCFlag(InShootShoot);
    }
    else
    {
        float delay;

        delay = curMissile->GetTOF(
                    (-self->ZPos()), self->GetVt(), targetData->ataFrom, targetPtr->BaseData()->GetVt(),
                    targetData->range
                ) + 5.0F;
        delay += min(delay * 0.5F, 5.0F);
        missileShotTimer = SimLibElapsedTime + FloatToInt32(delay * SEC_TO_MSEC);
        //MonoPrint ("DIGI BRAIN Firing Missile at Air Unit rng =
        // %.0f: Shoot Look next %.2f\n", targetData->range, delay);
        ClearATCFlag(InShootShoot);
    }

    if ( not IsSetATC(InShootShoot)) holdlongrangeshot = FALSE;
}


int DigitalBrain::SelectFlightModel(void)
{

    int simplifiedModel;

    // edg: if we're on the ground, we always want to be in simple
    // mode,  Otherwise we may cause a qnan crash in the flight model.
    // observed: simple model off, digi in separate mode and plane
    // was on ground -- x and y were qnan.  This doesn't fix the root of
    // the prob.
    if (self->OnGround())
        return SIMPLE_MODE_AF;


    // turn off simple mode if pilot has ejected or dying....
    if (self->IsAcStatusBitsSet(AircraftClass::ACSTATUS_PILOT_EJECTED) or self->pctStrength <= 0.0f)
    {
        return SIMPLE_MODE_OFF;
    }

    // override if we're deling with a threat
    if (threatPtr not_eq NULL)
    {
        return SIMPLE_MODE_OFF;
    }

    switch (curMode)
    {

        case FollowOrdersMode:
        case WingyMode:
            if (mpActionFlags[AI_USE_COMPLEX])
                simplifiedModel = SIMPLE_MODE_OFF;
            else
                simplifiedModel = SIMPLE_MODE_AF;

            break;

        case WaypointMode:
        case LoiterMode:
        case LandingMode:
        case TakeoffMode:
            simplifiedModel = SIMPLE_MODE_AF;
            break;

        case RefuelingMode:

            // 2002-02-20 ADDED BY S.G. Have the AI use complex flight model if in refuel
            if (g_bAIRefuelInComplexAF)
                simplifiedModel = SIMPLE_MODE_OFF;
            else
                // END OF ADDED SECTION
                simplifiedModel = SIMPLE_MODE_AF;

            break;

        case RTBMode:
        case BVREngageMode:
        case GunsEngageMode:
        case MissileEngageMode:
        case GunsJinkMode:
        case CollisionAvoidMode:
        case OverBMode:
        case RoopMode:
        case WVREngageMode:
        default:
            simplifiedModel = SIMPLE_MODE_OFF;
            break;
    }



    return simplifiedModel;
}

BOOL DigitalBrain::CommandTest(void)
{
    int flightIdx;

    // 2000-09-21 S.G. NO NEED TO USE THIS FUNCTION CALL, isWing ALREADY HAS THAT VALUE FOR US...
    // flightIdx = ((FlightClass*)self->GetCampaignObject())->GetComponentIndex(self);
    flightIdx = isWing;

    // If Leader, issue orders to wingmen

    if (flightIdx == AiFlightLead or (flightIdx == AiElementLead and mSplitFlight and mpActionFlags[AI_ENGAGE_TARGET] and mCurrentManeuver == FalconWingmanMsg::WMTotalMsg))   // VWF or rtb should be added
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}
