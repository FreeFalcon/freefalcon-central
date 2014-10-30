#include "stdhdr.h"
#include "digi.h"
#include "simveh.h"
#include "fcc.h"
#include "sms.h"
#include "smsdraw.h"
#include "campwp.h"
#include "PilotInputs.h"
#include "otwdrive.h"
#include "airframe.h"
#include "aircrft.h"
#include "missile.h"
#include "bomb.h"
#include "feature.h"
#include "f4vu.h"
#include "simbase.h"
#include "mavdisp.h"
#include "fsound.h"
#include "soundfx.h"
#include "simdrive.h"
#include "unit.h"
#include "flight.h"
#include "objectiv.h"
#include "classtbl.h"
#include "entity.h"
#include "MsgInc/WeaponFireMsg.h"
#include "object.h"
#include "guns.h"
#include "wingorder.h"
#include "team.h"
#include "radar.h"
#include "laserpod.h"
#include "harmpod.h"
#include "campwp.h"
/* S.G. FOR WEAPON SELECTION */
#include "simstatc.h"
/* S.G. 2001-06-16 FOR FINDING IF TARGET HAS MORE THAN ONE RADAR */
#include "battalion.h"

#include "fakerand.h"
extern SensorClass* FindLaserPod(SimMoverClass* theObject);

extern int g_nMissileFix;
extern float g_fBombMissileAltitude;
extern int g_nSeadAttackTime; // Cobra
extern int g_nStrikeAttackTime; // Cobra
extern int g_nGroundAttackTime; // Cobra
extern int g_nCASAttackTime; // Cobra
extern float g_fAIHarmMaxRange; // Cobra
extern float g_fAIJSOWMaxRange; // Cobra
extern int g_nAIshootLookShootTime; // Cobra
extern float g_fAGFlyoutRange; // Cobra
extern float g_fAGSlowFlyoutRange; // Cobra
extern float g_fAGSlowMoverSpeed; // Cobra
extern float g_fRocketPitchFactor; // Cobra
extern float g_fRocketPitchCorr; // Cobra


// 2001-06-28 ADDED BY S.G. HELPER FUNCTION TO TEST IF IT'S A SEAD MISSION ON ITS MAIN TARGET OR NOT
int DigitalBrain::IsNotMainTargetSEAD()
{
    // SEADESCORT has no main target
    if (missionType == AMIS_SEADESCORT)
        return TRUE;

    // Only for SEADS missions
    if (missionType not_eq AMIS_SEADSTRIKE)
        return FALSE;

    // If not ground target, we can't check if it's our main target, right? So assume it's not our main target
    if ( not groundTargetPtr)
        return TRUE;

    // Get the target campaign object if it's a sim
    CampBaseClass *campBaseTarget = ((CampBaseClass *)groundTargetPtr->BaseData());

    if (((SimBaseClass *)groundTargetPtr->BaseData())->IsSim())
        campBaseTarget = ((SimBaseClass *)groundTargetPtr->BaseData())->GetCampaignObject();

    // We should always have a waypoint but if we don't, can't be our attack waypoint right?
    if ( not self->curWaypoint)
        return TRUE;

    // If it's not our main target, it's ok to stop attacking it
    if (self->curWaypoint->GetWPTarget() not_eq campBaseTarget)
        return TRUE;

    // It's our main target, keep pounding it to death...
    return FALSE;
}
// END OF ADDED SECTION


SimObjectType **TP = NULL;

void DigitalBrain::GroundAttackMode(void)
{
    FireControlComputer* FCC = self->FCC;
    SMSClass* Sms = self->Sms;
    RadarClass* theRadar = (RadarClass*) FindSensor(self, SensorClass::Radar);
    BombClass* theBomb;
    float approxRange, approxSlantRange, dx, dy;
    approxSlantRange = 0;
    float pitchDesired;
    float  desSpeed;
    float xft, yft, zft;
    float rx, ata, ry;
    BOOL shootMissile;
    BOOL diveOK;
    float curGroundAlt;

    // COBRA - RED - FIXING CTDs, u CAN NOT CAST what may not be a Bomb, more, u can not cast IF NO HP SELECTED..
    // theBomb = (BombClass*)Sms->hardPoint[Sms->GetCurrentHardpoint()]->weaponPointer;
    theBomb = FCC->GetTheBomb();

    if (groundTargetPtr)
    {
        //TP = &groundTargetPtr;
        //REPORT_VALUE("TARGET ID", groundTargetPtr->BaseData()->GetCampID());
    }

    // 2001-06-01 ADDED BY S.G. I'LL USE missileShotTimer TO PREVENT THIS ROUTINE FROM DOING ANYTHING.
    // THE SIDE EFFECT IS WEAPONS HOLD SHOULD DO SOMETHING AS WELL
    if (SimLibElapsedTime < missileShotTimer)
    {
        return;
    }

    if ((agmergeTimer not_eq 0 and SimLibElapsedTime > agmergeTimer) or
        (missionComplete and ( not groundTargetPtr or not (IsSetATC(HasAGWeapon)))))
    {
        if (groundTargetPtr)
            SetGroundTargetPtr(NULL);

        ClearATCFlag(HasCanUseAGWeapon);
        ClearATCFlag(HasAGWeapon);
        agmergeTimer = SimLibElapsedTime + 1;
        // Cobra - finsih up and go to next WP
        // go back to initial AG state
        agDoctrine = AGD_NONE;
        missionComplete = TRUE;

        if (GetWaypointIndex() == GetTargetWPIndex())
            SelectNextWaypoint();

        // if we're a wingie, rejoin the lead
        if (isWing)
        {
            mFormation = FalconWingmanMsg::WMWedge;
            AiRejoin(NULL, AI_REJOIN);   // Cobra - Try this hint
            // make sure wing's designated target is NULL'd out
            mDesignatedObject = FalconNullId;
        }

        //agmergeTimer = 0; // Cobra - reinitialize
        return;
    }

    curGroundAlt = OTWDriver.GetGroundLevel(self->XPos(), self->YPos());

    //================================================
    // Cobra - bypass attack profile stuff while attacking with rockets
    //================================================
    if ((Sms->curWeapon and Sms->curWeapon->IsLauncher() and hasRocket and onStation == Final) or
        rocketMnvr == RocketFiring or (self->Sms->IsFiringRockets() and (rocketTimer < 0)))
    {
        if (groundTargetPtr)
        {
            trackX = groundTargetPtr->BaseData()->XPos();
            trackY = groundTargetPtr->BaseData()->YPos();
        }

        dx = (self->XPos() - trackX);
        dy = (self->YPos() - trackY);
        approxRange = (float)sqrt(dx * dx + dy * dy);
        FireRocket(approxRange, 0.0f);
        return;
    }

    //================================================

    //================================================
    // Cobra - bypass targeting stuff while flying out
    //================================================
    if (onStation == Downwind)
        goto DownwindBypass;


    // Cobra - Having problems ID'ing JSOWs on HPs
    int hasJSOW = FALSE;

    if (
        (hasBomb == TRUE + 4) or
        (theBomb and (theBomb->IsSetBombFlag(BombClass::IsJSOW)))
    )
    {
        hasJSOW = TRUE;
    }



    // 2002-03-08 ADDED BY S.G. Turn off the lasing flag we were lasing (fail safe)
    if (SimLibElapsedTime > waitingForShot and (moreFlags bitand KeepLasing))
    {
        moreFlags and_eq compl KeepLasing;
    }

    // 2001-06-18 ADDED BY S.G. AI NEED TO RE-EVALUATE ITS TARGET FROM TIME TO TIME, UNLESS THE LEAD IS THE PLAYER
    // 2001-06-18 ADDED BY S.G. NOPE, AI NEED TO RE-EVALUATE THEIR WEAPONS ONLY
    // 2001-07-10 ADDED BY S.G. IF OUR TARGET IS A RADAR AND WE ARE NOT CARRYING HARMS, SEE IF SOMEBODY ELSE COULD DO THE JOB FOR US
    // 2001-07-15 S.G. This was required when an non emitting radar could be targeted. Now check every 5 seconds if the campaign target has changed if we're the lead
    // JB 011017 Only reevaluate when we've dropped our ripple.
    if ((SimLibElapsedTime > nextAGTargetTime) and (Sms->CurRippleCount() == 0))
    {
        // 2001-07-20 S.G. ONLY CHANGE 'onStation' IF OUR WEAPON STATUS CHANGED...
        int tempWeapon = (hasHARM << 24) bitor (hasAGMissile << 16) bitor (hasGBU << 8) bitor hasBomb;

        SelectGroundWeapon();

        if ( not groundTargetPtr or ( not IsSetATC(HasCanUseAGWeapon)) and ( not IsSetATC(HasAGWeapon)))
        {
            ClearATCFlag(HasCanUseAGWeapon);
            ClearATCFlag(HasAGWeapon);

            // Go to next WP
            if (GetWaypointIndex() == GetTargetWPIndex())
                SelectNextWaypoint();

            // go back to initial AG state
            agDoctrine = AGD_NONE;
            missionComplete = TRUE;
            agmergeTimer = SimLibElapsedTime + 1;

            // if we're a wingie, rejoin the lead
            if (isWing)
            {
                mFormation = FalconWingmanMsg::WMWedge;
                AiRejoin(NULL);
                // make sure wing's designated target is NULL'd out
                mDesignatedObject = FalconNullId;
            }

            onStation = NotThereYet;
            return;
        }

        // So if we choose a different weapon, we'll set our FCC
        if (((hasHARM << 24) bitor (hasAGMissile << 16) bitor (hasGBU << 8) bitor hasBomb) not_eq tempWeapon)
        {
            if (onStation == Final)
            {
                onStation = HoldInPlace;
            }
        }

        // 2001-07-12 S.G. Moved below the above code
        // 2001-07-12 S.G. Simply clear the target. Code below will do the selection (and a new attack profile for us)
        // See if we should change target if we're the lead of the flight (but first we must already got a target)
        if ( not isWing and lastGndCampTarget)
        {
            UnitClass *campUnit = (UnitClass *)self->GetCampaignObject();

            if (campUnit)
            {
                campUnit->UnsetChecked();

                if (campUnit->ChooseTarget())
                {
                    if (lastGndCampTarget not_eq campUnit->GetTarget())
                        SetGroundTargetPtr(NULL);
                }
            }

        }

        // 2001-07-15 S.G. This was required when an non emitting radar could be targeted. This is not the case anymore so skip the whole thing
        // Try again in 5 seconds.
        nextAGTargetTime = SimLibElapsedTime + 5000;
    }

    // 2001-07-12 S.G. Need to force a target reselection under some conditions. If we're in GroundAttackMode, it's because we are already in agDoctrine not_eq AGD_NONE which mean we already got a target originally
    // This was done is some 'onStation' modes but I moved it here instead so all modes runs it
    if (
        groundTargetPtr and 
        (
            (
                groundTargetPtr->BaseData()->IsSim() and // For a SIM entity
                (
                    groundTargetPtr->BaseData()->IsDead() or
                    groundTargetPtr->BaseData()->IsExploding() or
                    (
 not isWing and IsNotMainTargetSEAD() and 
 not ((SimBaseClass *)groundTargetPtr->BaseData())->GetCampaignObject()->IsEmitting()
                    )
                )
            ) or
            (
                groundTargetPtr->BaseData()->IsCampaign() and // For a CAMPAIGN entity
                (
 not isWing and IsNotMainTargetSEAD() and 
 not ((CampBaseClass *)groundTargetPtr->BaseData())->IsEmitting()
                )
            )
        )
    )
    {
        SetGroundTarget(NULL);
    }

    int reSetup = FALSE;

    if ( not groundTargetPtr)
    {
        // Wings run a limited version of the target selection (so they don't switch campaign target)
        if (isWing)
            AiRunTargetSelection();
        else
            SelectGroundTarget(TARGET_ANYTHING);

        // Force a run of SetupAGMode if we finally got a target
        if (groundTargetPtr)
        {
            if ((missionType == AMIS_SEADSTRIKE) and not hasHARM)
            {
                SetGroundTarget(NULL);
                agDoctrine = AGD_NONE;
                missionComplete = TRUE;

                if (GetWaypointIndex() == GetTargetWPIndex())
                    SelectNextWaypoint();

                // Cobra -  reset radar mode and Master Mode
                if (theRadar)
                    theRadar->SetMode(RadarClass::RWS);

                self->FCC->SetMasterMode(FireControlComputer::Nav);
                return;
            }
            else
                reSetup = TRUE;
        }
        else if ( not isWing) // If leader and no more target, move on
        {
            // Cobra - No more target.  Do something else
            ClearATCFlag(HasCanUseAGWeapon);
            ClearATCFlag(HasAGWeapon);
            agmergeTimer = SimLibElapsedTime + 1;
            // go back to initial AG state
            agDoctrine = AGD_NONE;
            missionComplete = TRUE;

            if (GetWaypointIndex() == GetTargetWPIndex())
                SelectNextWaypoint();

            // Cobra -  reset radar mode and Master Mode
            if (theRadar)
                theRadar->SetMode(RadarClass::RWS);

            self->FCC->SetMasterMode(FireControlComputer::Nav);
            // wingies rejoin the lead
            mFormation = FalconWingmanMsg::WMWedge;

            if (self->GetCampaignObject()->NumberOfComponents() > 1 and not isWing)
                // RED - FIXED CTD - AI should not use this function
                // AiSendPlayerCommand( FalconWingmanMsg::WMRejoin, AiFlight );
                AiSendCommand(self, FalconWingmanMsg::WMRejoin, AiFlight, FalconNullId);

            agmergeTimer = 0;
            onStation = NotThereYet;
            return; // Cobra
        }
    }

    // If we got a deaggregated campaign object, find a sim object within
    if (
        groundTargetPtr and 
        groundTargetPtr->BaseData()->IsCampaign() and 
 not ((CampBaseClass *)groundTargetPtr->BaseData())->IsAggregate()
    )
    {
        SelectGroundTarget(TARGET_ANYTHING);
        reSetup = TRUE;
    }

    if (reSetup)
    {
        nextAGTargetTime = 0;
        SetupAGMode(NULL, NULL);

        if (Sms->curWeapon and Sms->curWeapon->IsLauncher() and hasRocket and onStation == Final)
        {
            // leave onStation alone
        }
        else
        {
            onStation = NotThereYet;
        }
    }

    // END OF ADDED SECTION

    // NOTES ON MODES (in approximate order):
    // 1) NotThereYet: Initial setup for ground run.  Track to IP.
    //   Also determine if another run is to be made, or end ground run.
    // 2) CrossWind: head to IP
    // 3) HoldInPlace: Track Towards target
    // 4) Final: ready weapons and fire when within parms.
    // 5) Final1: release more weapoons if appropriate then go back to OnStation

    curGroundAlt = OTWDriver.GetGroundLevel(self->XPos(), self->YPos());

    if (agDoctrine == AGD_NEED_SETUP)
    {
        SetupAGMode(NULL, NULL);
    }

    // 2001-07-18 ADDED BY S.G. IF ON A LOOK_SHOOT_LOOK MODE WITH A TARGET, SEND AN ATTACK RIGHT AWAY AND DON'T WAIT TO BE CLOSE TO/ON FINAL TO DO IT BECAUSE YOU'LL MAKE SEVERAL PASS ANYWAY...

    if (groundTargetPtr and agDoctrine == AGD_LOOK_SHOOT_LOOK)
    {
        if (self->GetCampaignObject()->NumberOfComponents() > 1 and not isWing)
        {
            if (sentWingAGAttack == AG_ORDER_NONE)
            {
                AiSendCommand(self, FalconWingmanMsg::WMTrail, AiFlight, FalconNullId);
                sentWingAGAttack = AG_ORDER_FORMUP;
                // 2002-01-20 ADDED BY S.G. Added the new nextAttackCommandToSend variable check to force the lead to reissue an attack in case wings went back into formation (can we say HACK?)
                nextAttackCommandToSend = SimLibElapsedTime + g_nAIshootLookShootTime * SEC_TO_MSEC;
            }
            // 2002-01-20 ADDED BY S.G. Either we haven't sent the attack order or we sent it a while ago, check if we should send it again
            else if (sentWingAGAttack not_eq AG_ORDER_ATTACK or SimLibElapsedTime > nextAttackCommandToSend)
            {
                VU_ID targetId;

                if (groundTargetPtr->BaseData()->IsSim())
                {
                    targetId = ((SimBaseClass*)groundTargetPtr->BaseData())->GetCampaignObject()->Id();
                }
                else
                    targetId = groundTargetPtr->BaseData()->Id();

                if (targetId not_eq FalconNullId)
                {
                    // 2002-01-20 ADDED BY S.G. Check the wing's AI to see if they have weapon but in formation
                    int sendAttack = FALSE;

                    if (SimLibElapsedTime > nextAttackCommandToSend)
                    {
                        // timed out
                        int i;
                        int usComponents = self->GetCampaignObject()->NumberOfComponents();

                        for (i = 0; i < usComponents; i++)
                        {
                            AircraftClass *flightMember = (AircraftClass *)self->GetCampaignObject()->GetComponentEntity(i);

                            // This code is assuming the lead and the AI are on the same PC... Should be no problem unless another player is in Combat AP...
                            if (flightMember and flightMember->DBrain() and flightMember->DBrain()->IsSetATC(IsNotMainTargetSEAD() ? HasAGWeapon : HasCanUseAGWeapon) and flightMember->DBrain()->agDoctrine == AGD_NONE)
                            {
                                sendAttack = TRUE;
                                break;
                            }
                        }
                    }

                    // Cobra - Removed.  It make sendAttack always TRUE when SimLibElapsedTime <= nextAttackCommandToSend
                    //else
                    //sendAttack = TRUE;

                    if (self->GetCampaignObject()->NumberOfComponents() > 1 and sendAttack)
                    {
                        AiSendCommand(self, FalconWingmanMsg::WMAssignTarget, AiFlight, targetId);
                        AiSendCommand(self, FalconWingmanMsg::WMShooterMode, AiFlight, targetId);
                        sentWingAGAttack = AG_ORDER_ATTACK;
                    }

                    // 2002-01-20 ADDED BY S.G. Added the new nextAttackCommandToSend variable check to force the lead to reissue an attack in case wings went back into formation (can we say HACK?)
                    nextAttackCommandToSend = SimLibElapsedTime + g_nAIshootLookShootTime * SEC_TO_MSEC;
                }
            }
        }
    }

    // Cobra - bypass targeting stuff while flying out
DownwindBypass:

    // modes for ground attack
    switch (onStation)
    {
            // protect against invalid state by going to our start condition
            // then fall thru
        default:
            onStation = NotThereYet;

            // #1 setup....
            // We Should have an insertion point set up at ipX,Y and Z.....
            //==================================================
        case NotThereYet:

            //AI_MESSAGE(0, "AI - NOT YET THERE");
            // have we already made an AG Attack run?
            if (madeAGPass)
            {
                // 2001-05-13 ADDED BY S.G. SO AI DROPS SOME COUNTER MEASURE AFTER A PASS
                //self->DropProgramed();

                // clear weapons and target
                //Cobra tset
                // RV - Biker - Should fix wingmen to employ all Mavs
                //SetGroundTarget( NULL );
                self->FCC->preDesignate = TRUE;

                // if we've already made 1 pass and the doctrine is
                // shoot_run, then we're done with this waypoint go to
                // next.  Otherwise we're in look_shoot_look.   We'll remain
                // at the current waypoint and allow the campaign to retarget
                // for us
                if (agDoctrine == AGD_SHOOT_RUN or (agmergeTimer not_eq 0 and SimLibElapsedTime > agmergeTimer))  // Cobra - Get out of Dodge
                {
                    if (agImprovise == FALSE or (SimLibElapsedTime > agmergeTimer))
                    {
                        if (GetWaypointIndex() == GetTargetWPIndex())
                            SelectNextWaypoint();
                    }

                    ClearATCFlag(HasCanUseAGWeapon);
                    ClearATCFlag(HasAGWeapon);
                    // go back to initial AG state
                    self->FCC->SetMasterMode(FireControlComputer::Nav);
                    agDoctrine = AGD_NONE;
                    missionComplete = TRUE;
                    agmergeTimer = SimLibElapsedTime + 1;

                    // if we're a wingie, rejoin the lead
                    if (isWing)
                    {
                        // 2001-05-03 ADDED BY S.G. WE WANT WEDGE AFTER GROUND PASS
                        mFormation = FalconWingmanMsg::WMWedge;
                        // END OF ADDED SECTION
                        AiRejoin(NULL, AI_REJOIN);  // Cobra - Try this hint
                        // make sure wing's designated target is NULL'd out
                        mDesignatedObject = FalconNullId;
                    }

                    onStation = NotThereYet;
                    return;
                }

                // 2001-06-18 MODIFIED BY S.G. I'LL USE THIS TO HAVE THE AI FORCE A RETARGET AND 15 SECONDS IS TOO HIGH
                // nextAGTargetTime = SimLibElapsedTime + 15000;
                nextAGTargetTime = SimLibElapsedTime + 5000;

                // 2001-07-12 ADDED BY S.G. SINCE WE'RE GOING AFTER SOMETHING NEW, GET A NEW TARGET RIGHT NOW SO agDoctrine DOESN'T GET SET TO AGD_NONE WHICH WILL END UP SetupAGMode BEING CALLED BEFORE THE AI HAS A CHANCE TO REACH ITS TURN POINT
                if (isWing)
                    AiRunTargetSelection();
                else
                    SelectGroundTarget(TARGET_ANYTHING);

                // If we got a deaggregated campaign object, find a sim object within
                if (groundTargetPtr and groundTargetPtr->BaseData()->IsCampaign() and not ((CampBaseClass *)groundTargetPtr->BaseData())->IsAggregate())
                    SelectGroundTarget(TARGET_ANYTHING);

                // Cobra - No more radar targets?  Mission complete?  Added: or no more weapons
                // Cobra - Isn't this true for all missionTypes? ... no more targets, move on.
                // if ( not groundTargetPtr and missionType == AMIS_SEADSTRIKE)
                if ( not groundTargetPtr or ( not IsSetATC(HasCanUseAGWeapon)) or ( not IsSetATC(HasAGWeapon)))
                {
                    ClearATCFlag(HasCanUseAGWeapon);
                    ClearATCFlag(HasAGWeapon);

                    // Go to next WP
                    if (GetWaypointIndex() == GetTargetWPIndex())
                        SelectNextWaypoint();

                    // go back to initial AG state
                    agDoctrine = AGD_NONE;
                    missionComplete = TRUE;
                    agmergeTimer = SimLibElapsedTime + 1;

                    // if we're a wingie, rejoin the lead
                    if (isWing)
                    {
                        mFormation = FalconWingmanMsg::WMWedge;
                        AiRejoin(NULL);
                        // make sure wing's designated target is NULL'd out
                        mDesignatedObject = FalconNullId;
                    }

                    onStation = NotThereYet;
                    return;
                }

            } // end madeAGPass

            if (self->GetCampaignObject()->NumberOfComponents() > 1 and not isWing and sentWingAGAttack == AG_ORDER_NONE)
            {
                AiSendCommand(self, FalconWingmanMsg::WMTrail, AiFlight, FalconNullId);
                sentWingAGAttack = AG_ORDER_FORMUP;
                // 2002-01-20 ADDED BY S.G. Added the new nextAttackCommandToSend variable check to force the lead to reissue an attack in case wings went back into formation (can we say HACK?)
                nextAttackCommandToSend = SimLibElapsedTime + g_nAIshootLookShootTime * SEC_TO_MSEC;
            }

            // check to see if we're too close to target (if we've got one)
            // if so set up new IP
            if (groundTargetPtr and madeAGPass)
            {
                // Bail and try again
                AGflyOut();
                // Release current target and target history
                //SetGroundTarget( NULL );
                //gndTargetHistory[0] = NULL;
                onStation = Downwind;
                break;
            }

            // track to insertion point
            SetTrackPoint(ipX, ipY, AGattackAlt);

            dx = (float)fabs(self->XPos() - trackX);
            dy = (float)fabs(self->YPos() - trackY);
            approxRange = (float)sqrt(dx * dx + dy * dy);

            //  Cobra - SEAD attack mode is high to get the SAM radar to turn on
            // if ( agApproach == AGA_LOW or missionType == AMIS_SEADESCORT or missionType == AMIS_SEADSTRIKE)
            if (agApproach == AGA_LOW)
            {
                // see if we're too close to set up the ground run
                // if so, we're going to head to a new point perpendicular to
                // our current direction and make a run from there
                // this is kind of a sanity check
                trackZ = OTWDriver.GetGroundLevel(self->XPos() + self->XDelta(),
                                                  self->YPos() + self->YDelta());
                // if we're below track alt, kick us up a bit harder so we don't plow
                // into steeper slopes
                //Cobra we are going to try and do a sanity check on alt now
                float myAlt = self->ZPos();

                if (fabsf(myAlt - self->GetA2GGunRocketAlt()) < 2000.0f)
                {
                    trackZ = -self->GetA2GGunRocketAlt();
                }
                else
                {
                    trackZ = self->ZPos();
                }

                //TJL 11/09/03 Changed to Corner speed to allow for variety of AI
                desSpeed = cornerSpeed * 1.3f;
            }
            else if (missionType == AMIS_SEADESCORT or missionType == AMIS_SEADSTRIKE)
            {
                // see if we're too close to set up the ground run
                // if so, we're going to head to a new point perpendicular to
                // our current direction and make a run from there
                // this is kind of a sanity check
                //TJL 11/09/03 Changed to Corner speed to allow for variety of AI
                desSpeed = cornerSpeed * 1.3f;

                if (self->ZPos() - curGroundAlt > -500.0f)
                {
                    trackZ = OTWDriver.GetGroundLevel(self->XPos() + self->XDelta(),
                                                      self->YPos() + self->YDelta());

                    if (self->ZPos() - trackZ > -500.0f)
                        //trackZ = trackZ - 500.0f - ( self->ZPos() - trackZ + 500.0f ) * 2.0f;
                        trackZ = -self->GetA2GHarmAlt();

                    else if (agApproach == AGA_HIGH and trackZ > AGattackAlt)
                        trackZ = AGattackAlt;
                    else
                        trackZ = -self->GetA2GHarmAlt();
                }
            }
            else
            {
                // see if we're too close to set up the ground run
                // if so, we're going to head to a new point perpendicular to
                // our current direction and make a run from there
                // this is kind of a sanity check
                //TJL 11/09/03 Changed to Corner speed to allow for variety of AI
                desSpeed = cornerSpeed * 1.3f;

                if (self->ZPos() - curGroundAlt > -500.0f)
                {
                    trackZ = OTWDriver.GetGroundLevel(self->XPos() + self->XDelta(),
                                                      self->YPos() + self->YDelta());

                    if (self->ZPos() - trackZ > -500.0f)
                        //trackZ = trackZ - 500.0f - ( self->ZPos() - trackZ + 500.0f ) * 2.0f;
                        trackZ = -self->GetA2GDumbLDAlt();

                    else if (agApproach == AGA_HIGH and trackZ > AGattackAlt)
                        trackZ = AGattackAlt;
                    else
                        trackZ = AGattackAlt;
                }
            }

            // Cobra - one missile/harm at a time
            if ((agDoctrine == AGD_LOOK_SHOOT_LOOK) and 
                (missionType == AMIS_SEADESCORT or missionType == AMIS_SEADSTRIKE))
            {
                Sms->SetAGBPair(FALSE);//Cobra
            }
            else // bombs dropped in pairs
            {
                Sms->SetAGBPair(TRUE);
            }

            // 2001-05-13 ADDED BY S.G. WITHOUT THIS, I WOULD GO BACK REDO THE CROSSWIND AFTER THE MISSION IS OVER WAYPOINTS WILL OVERIDE trackXYZ SO DON'T WORRY ABOUT THEM
            //if ( not missionComplete and madeAGPass)
            if ( not missionComplete)
            {
                // head to IP
                onStation = Crosswind;
            }

            //TJL 11/09/03 Changed to Corner speed to allow for variety of AI
            desSpeed = cornerSpeed * 1.3f;
            //desSpeed = max(200.0F, min (desSpeed, 700.0F)); // Knots
            TrackPoint(0.0F, desSpeed);

            break;
            //======= End NotThereYet ===========================================

            // #2 heading towards target -- we've reached the IP and are heading in
            // set up our available weapons (pref to missiles) and head in
            // also set up our final approach tracking
            //==================================================
        case HoldInPlace:
            //AI_MESSAGE(0, "AI - HOLD IN PLACE");

            // 2001-07-12 S.G. Moved so a retargeting is not done
        FinalSG: // 2001-06-24 ADDED BY S.G. JUMPS BACK HERE IF TOO CLOSE FOR JSOWs or HARMS AND TARGET NOT EMITTING
            dx = (float)fabs(self->XPos() - trackX);
            dy = (float)fabs(self->YPos() - trackY);
            approxRange = (float)sqrt(dx * dx + dy * dy);

            if (groundTargetPtr)
            {
                SetTrackPoint(groundTargetPtr);
            }

            trackZ = OTWDriver.GetGroundLevel(trackX, trackY);

            // 2001-10-31 ADDED by M.N. hope this fixes the SEAD circling bug
            if (groundTargetPtr)
            {
                float approxTargetRange;
                xft = trackX - self->XPos();
                yft = trackY - self->YPos();
                zft = trackZ - self->ZPos();
                //zft = trackZ - self->ZPos();
                //approxTargetRange = (float)sqrt(xft*xft + yft*yft + zft*zft);
                approxTargetRange = (float)sqrt(xft * xft + yft * yft);
                approxTargetRange = max(approxTargetRange, 1.0F);

                dx = self->dmx[0][0] * xft + self->dmx[0][1] * yft + self->dmx[0][2] * zft;
                dy = self->dmx[1][0] * xft + self->dmx[1][1] * yft + self->dmx[1][2] * zft;
                ata = (float)acos(dx / approxTargetRange);


                //Cobra - pullout and away from target
                if ((approxTargetRange < 3.5f * NM_TO_FT) and (FabsF(ata) < 20.0f * DTR))//3.0 Cobra - was 6
                {
                    // Bail and try again
                    AGflyOut();
                    // Release current target and target history
                    //SetGroundTarget( NULL );
                    //gndTargetHistory[0] = NULL;
                    onStation = Downwind;
                    break;
                }
            }
            else // No target, so flyout and try again
            {
                // Bail and try again
                AGflyOut();
                // Release current target and target history
                //SetGroundTarget( NULL );
                //gndTargetHistory[0] = NULL;
                onStation = Downwind;
                break;
            }

            if (hasAGMissile)
            {
                self->FCC->SetMasterMode(FireControlComputer::AirGroundMissile);
                self->FCC->SetSubMode(FireControlComputer::SLAVE);
                //Cobra test
                // self->FCC->SetMasterMode (FireControlComputer::AirGroundMissile);
                // self->FCC->SetSubMode (FireControlComputer::SLAVE);
                FCC->preDesignate = TRUE;
                //MonoPrint ("Setup For Maverick\n");
            }
            else if (hasHARM)
            {
                //Cobra test
                self->FCC->SetMasterMode(FireControlComputer::AirGroundHARM);
                self->FCC->SetSubMode(FireControlComputer::HARM);  // RV- I-Hawk
                //MonoPrint ("Setup For HARM\n");
            }
            else if (hasGBU)
            {
                //Cobra test
                self->FCC->SetMasterMode(FireControlComputer::AirGroundLaser);
                self->FCC->SetSubMode(FireControlComputer::SLAVE);
                self->FCC->preDesignate = TRUE;
                self->FCC->designateCmd = TRUE;
                //MonoPrint ("Setup For GBU\n");
            }
            else if (hasBomb)
            {
                //Cobra JSOW use MANual submode
                self->FCC->SetMasterMode(FireControlComputer::AirGroundBomb);

                if (hasJSOW)
                    self->FCC->SetSubMode(FireControlComputer::MAN);
                else
                    self->FCC->SetSubMode(FireControlComputer::CCRP);

                self->FCC->groundDesignateX = trackX;
                self->FCC->groundDesignateY = trackY; // Cobra - was: groundDesignateY = trackX
                self->FCC->groundDesignateZ = trackZ;
                //MonoPrint ("Setup For Iron Bomb\n");
            }

            // Point the radar at the target
            if (theRadar)
            {
                // 2001-07-23 MODIFIED BY S.G. MOVERS ARE ONLY 3D ENTITIES WHILE BATTALIONS WILL INCLUDE 2D AND 3D VEHICLES...
                //          if (groundTargetPtr and groundTargetPtr->BaseData()->IsMover())
                if (groundTargetPtr and ((groundTargetPtr->BaseData()->IsSim() and ((SimBaseClass *)groundTargetPtr->BaseData())->GetCampaignObject()->IsBattalion()) or (groundTargetPtr->BaseData()->IsCampaign() and groundTargetPtr->BaseData()->IsBattalion())))
                    theRadar->SetMode(RadarClass::GMT);
                else
                    theRadar->SetMode(RadarClass::GM);

                theRadar->SetDesiredTarget(groundTargetPtr);
                theRadar->SetAGSnowPlow(TRUE);
            }

            //  Cobra - SEAD attack mode is high to get the SAM radar to turn on
            // if ( agApproach == AGA_LOW or missionType == AMIS_SEADESCORT or missionType == AMIS_SEADSTRIKE)
            if (agApproach == AGA_LOW)
            {
                trackZ = OTWDriver.GetGroundLevel(self->XPos() + self->XDelta(),
                                                  self->YPos() + self->YDelta());
                // if we're below track alt, kick us up a bit harder so we don't plow
                // into steeper slopes
                //Cobra we are going to try and do a sanity check on alt now
                float myAlt = self->ZPos();

                if (fabsf(myAlt - self->GetA2GGunRocketAlt()) < 2000.0f)
                    trackZ = -self->GetA2GGunRocketAlt();
                else
                    trackZ = self->ZPos();

                //TJL 11/09/03 Changed to Corner speed to allow for variety of AI
                desSpeed = cornerSpeed * 1.3f;
                //desSpeed = 650.0f;

            }
            else if (agApproach == AGA_DIVE)
            {
                trackZ = OTWDriver.GetGroundLevel(trackX, trackY);

                if (Sms->curWeapon)
                    trackZ -= 100.0f;

                //TJL 11/09/03 Changed to Corner speed to allow for variety of AI
                desSpeed = cornerSpeed * 1.3f;

                //desSpeed = 450.0f;
                if (self->ZPos() - curGroundAlt > -1000.0f)
                {
                    trackZ = OTWDriver.GetGroundLevel(self->XPos() + self->XDelta(),
                                                      self->YPos() + self->YDelta());

                    if (self->ZPos() - trackZ > -500.0f)
                        trackZ = trackZ - 500.0f - (self->ZPos() - trackZ + 500.0f) * 2.0f;
                    else
                        trackZ -= 500.0f;
                }
            }
            else
            {
                // 2002-03-28 MN fix for AI handling of JSOW/JDAM missiles - fire from defined altitude instead of 4000ft like Mavericks
                if (Sms and Sms->curWeapon)
                {
                    Falcon4EntityClassType* classPtr = NULL;
                    WeaponClassDataType *wc = NULL;

                    classPtr = (Falcon4EntityClassType*)Sms->curWeapon->EntityType();

                    if (classPtr)
                    {
                        wc = (WeaponClassDataType*)classPtr->dataPtr;

                        // Cobra - GPS weapons
                        if (wc and (wc->Flags bitand WEAP_BOMBGPS))
                        {
                            //if (wc->Flags bitand WEAP_CLUSTER) // Cobra - JSOW
                            if (hasJSOW)
                                ipZ = -self->GetA2GJSOWAlt();
                            else
                                ipZ = -self->GetA2GJDAMAlt(); // Cobra - JDAM
                        }
                    }
                }

                ipZ = trackZ = AGattackAlt;
                //TJL 11/09/03 Changed to Corner speed to allow for variety of AI
                desSpeed = cornerSpeed * 1.3f;

                //desSpeed = 450.0f;
                if (self->ZPos() - curGroundAlt > -500.0f)
                {
                    trackZ = OTWDriver.GetGroundLevel(self->XPos() + self->XDelta(),
                                                      self->YPos() + self->YDelta());

                    if (self->ZPos() - trackZ > -500.0f)
                        trackZ = trackZ - 500.0f - (self->ZPos() - trackZ + 500.0f) * 2.0f;
                    // 2001-06-18 ADDED S.G. WHY DO THIS IF WE'RE GOING UP ALREADY?
                    else if (trackZ > AGattackAlt) // Are we going up? (don't forget negative is UP)
                        trackZ = AGattackAlt;
                    // END OF ADDED SECTION
                    else
                        trackZ -= 500.0f;
                }
            }

            onStation = Final;

            // 2001-05-21 ADDED BY S.G. ONLY DO THIS IF NOT WITHIN TIMEOUT PERIOD. TO BE SAFE, I'LL SET waitingForShot TO 0 IN digimain SO IT IS INITIALIZED
            if (waitingForShot < SimLibElapsedTime - 5000)
                // Say you can fire when ready
                // END OF ADDED SECTION
                waitingForShot = SimLibElapsedTime - 1;

            TrackPoint(0.0F, desSpeed/* KNOTS_TO_FTPSEC*/); // Cobra - aerial TrackPoint speed is in knots
            // SimpleTrack(SimpleTrackSpd, desSpeed * KNOTS_TO_FTPSEC);
            break;
            //======= End HoldInPlace ===========================================

            // case 1a: head to good start location (IP)
            //==================================================
        case Crosswind:
            //AI_MESSAGE(0, "AI - CROSSWINDING");

            SetTrackPoint(ipX, ipY, AGattackAlt);

            dx = (float)fabs(self->XPos() - trackX);
            dy = (float)fabs(self->YPos() - trackY);
            approxRange = (float)sqrt(dx * dx + dy * dy);

            // Terrain follow around 1000 ft
            // 2001-07-12 MODIFIED BY S.G. SO SEAD STAY LOW UNTIL READY TO ATTACK
            //  Cobra - SEAD attack mode is high to get the SAM radar to turn on
            // if ( agApproach == AGA_LOW or missionType == AMIS_SEADESCORT or missionType == AMIS_SEADSTRIKE)
            if (agApproach == AGA_DIVE)
            {
                trackZ = OTWDriver.GetGroundLevel(self->XPos() + self->XDelta(),
                                                  self->YPos() + self->YDelta());
                // if we're below track alt, kick us up a bit harder so we don't plow
                // into steeper slopes
                //
                //Cobra we are going to try and do a sanity check on alt now
                float myAlt = self->ZPos();

                if (fabsf(myAlt - self->GetA2GGunRocketAlt()) < 2000.0f)
                    trackZ = -self->GetA2GGunRocketAlt();
                else
                    trackZ = self->ZPos();

                //TJL 11/09/03 Changed to Corner speed to allow for variety of AI
                desSpeed = cornerSpeed * 1.3f;

            }
            else
            {
                //TJL 11/09/03 Changed to Corner speed to allow for variety of AI
                desSpeed = cornerSpeed * 1.3f;

                //desSpeed = 450.0f;
                if (self->ZPos() - curGroundAlt > -500.0f)
                {
                    trackZ = OTWDriver.GetGroundLevel(self->XPos() + self->XDelta(),
                                                      self->YPos() + self->YDelta());

                    if (self->ZPos() - trackZ > -500.0f)
                        trackZ = trackZ - 500.0f - (self->ZPos() - trackZ + 500.0f) * 2.0f;
                    // 2001-06-18 ADDED S.G. WHY DO THIS IF WE'RE GOING UP ALREADY?
                    else if (agApproach == AGA_HIGH and trackZ > AGattackAlt) // Are we going up? (don't forget negative is UP)
                        trackZ = AGattackAlt;
                    // END OF ADDED SECTION
                    else
                        trackZ -= 500.0f;
                }
            }

            //Cobra trackpoint uses knots only
            TrackPoint(0.0F, desSpeed /* KNOTS_TO_FTPSEC*/); // Cobra - aerial TrackPoint speed is in knots

            // 2001-05-05 ADDED BY S.G. THIS IS TO MAKE THE AI PULL AGGRESIVELY AFTER A PASS. I WOULD HAVE LIKE TESTING madeAGPass BUT IT IS CLEARED BEFORE :-(
            // Increase the gains on crosswind turn
            rStick *= 3.0f;

            if (rStick > 1.0f)
                rStick = 1.0f;
            else if (rStick < -1.0f)
                rStick = -1.0f;

            // END OF ADDED SECTION

            // 2001-07-12 ADDED BY S.G. IF CLOSE TO THE FINAL POINT, SEND AN ATTACK COMMAND TO THE WINGS
            // tell our wing to attack
            if (groundTargetPtr and ((hasJSOW and approxRange < g_fAIJSOWMaxRange * NM_TO_FT) or (approxRange < 8.0f * NM_TO_FT))) // was 5.0
            {
                if (self->GetCampaignObject()->NumberOfComponents() > 1 and not isWing and sentWingAGAttack not_eq AG_ORDER_ATTACK)
                {
                    VU_ID targetId;

                    if (groundTargetPtr->BaseData()->IsSim())
                    {
                        targetId = ((SimBaseClass*)groundTargetPtr->BaseData())->GetCampaignObject()->Id();
                    }
                    else
                        targetId = groundTargetPtr->BaseData()->Id();

                    if (targetId not_eq FalconNullId)
                    {
                        AiSendCommand(self, FalconWingmanMsg::WMAssignTarget, AiFlight, targetId);
                        AiSendCommand(self, FalconWingmanMsg::WMShooterMode, AiFlight, targetId);
                        sentWingAGAttack = AG_ORDER_ATTACK;
                        // 2002-01-20 ADDED BY S.G. Added the new nextAttackCommandToSend variable check to force the lead to reissue an attack in case wings went back into formation (can we say HACK?)
                        nextAttackCommandToSend = SimLibElapsedTime + g_nAIshootLookShootTime * SEC_TO_MSEC;
                    }
                }
            }

            // END OF ADDED SECTION

            // are we about at our IP?
            // 2001-07-23 MODIFIED BY S.G. IF NO WEAPON AVAIL FOR TARGET, SWITCH TO FINAL RIGHT AWAY
            // if ( approxRange < 1.3f * NM_TO_FT)
            //cobra removed because SEAD was looping around target
            //if ( approxRange < 1.3f * NM_TO_FT /*or not IsSetATC(HasCanUseAGWeapon)*/)
            // Cobra - Added JSOW stand-off weapon launching
            //if ((hasJSOW and approxRange < g_fAIJSOWMaxRange * NM_TO_FT) or (approxRange < g_fAGFlyoutRange * NM_TO_FT)) // Cobra - was 5 NM
            if ((hasJSOW and approxRange < g_fAIJSOWMaxRange * NM_TO_FT) or
                (hasHARM and approxRange < g_fAIHarmMaxRange * NM_TO_FT) or
                (approxRange < 8.0f * NM_TO_FT /* and FabsF(ata) < 20.0f * DTR */)) // Cobra - was 5 NM
            {
                // 2001-07-15 ADDED BY S.G. IF madeAGPass IS TRUE, WE MADE AN A2G PASS AND WAS TURNING AWAY. REDO AN ATTACK PROFILE FOR A NEW PASS
                if (madeAGPass and ( not hasJSOW))
                {
                    AGflyOut();
                    // Release current target and target history
                    //SetGroundTarget( NULL );
                    //gndTargetHistory[0] = NULL;
                    onStation = Downwind;
                }
                else
                {
                    onStation = HoldInPlace;
                }
            }

            break;
            //======= End CrossWind ===========================================

            //==================================================
            //  Flyout for next pass
            //==================================================
        case Downwind:
        {
            float x, y, z;
            float approxTargetRange, approxRange;

            agDoctrine = AGD_LOOK_SHOOT_LOOK;
            ipZ = trackZ = AGattackAlt;

            if (groundTargetPtr)
            {
                dx = self->XPos() - groundTargetPtr->BaseData()->XPos();
                dy = self->YPos() - groundTargetPtr->BaseData()->YPos();
            }
            else
            {
                self->curWaypoint->GetLocation(&x, &y, &z);
                dx = self->XPos() - x;
                dy = self->YPos() - y;
            }

            approxTargetRange = (float)sqrt(dx * dx + dy * dy);

            dx = trackX - self->XPos();
            dy = trackY - self->YPos();
            approxRange = (float)sqrt(dx * dx + dy * dy);

            // Flying toward target??....Wrong
            if (FabsF(approxTargetRange) < 0.2f * NM_TO_FT)
            {
                AGflyOut();
            }

            desSpeed = cornerSpeed * 1.3f;

            if (self->ZPos() - curGroundAlt > -500.0f)
            {
                trackZ = OTWDriver.GetGroundLevel(self->XPos() + self->XDelta(),
                                                  self->YPos() + self->YDelta());
                // if ( self->ZPos() - trackZ > -500.0f )
                //trackZ = AGattackAlt;
                // else if (agApproach == AGA_HIGH and trackZ > AGattackAlt)
                //   trackZ = AGattackAlt;
                // else
                //trackZ = AGattackAlt;
            }

            TrackPoint(0.0F, desSpeed);

            // Cobra - get out far enough to make another pass
            if ((((slowMover) and (approxTargetRange > g_fAGSlowFlyoutRange * NM_TO_FT)) or
                 (approxTargetRange > g_fAGFlyoutRange * NM_TO_FT))) // or
                //(approxRange < 2.0f  * NM_TO_FT))
            {
                // Check for remaining AB weapons
                SelectGroundWeapon();

                if ( not (IsSetATC(HasCanUseAGWeapon)) and not (IsSetATC(HasAGWeapon)))
                {
                    ClearATCFlag(HasCanUseAGWeapon);
                    ClearATCFlag(HasAGWeapon);

                    // Go to next WP
                    if (GetWaypointIndex() == GetTargetWPIndex())
                        SelectNextWaypoint();

                    // go back to initial AG state
                    agDoctrine = AGD_NONE;
                    missionComplete = TRUE;
                    agmergeTimer = SimLibElapsedTime + 1;

                    // if we're a wingie, rejoin the lead
                    if (isWing)
                    {
                        mFormation = FalconWingmanMsg::WMWedge;
                        AiRejoin(NULL);
                        // make sure wing's designated target is NULL'd out
                        mDesignatedObject = FalconNullId;
                    }

                    onStation = NotThereYet;
                    return;
                }

                // Increase the gains on downwind turn
                rStick *= 3.0f;

                if (rStick > 1.0f)
                    rStick = 1.0f;
                else if (rStick < -1.0f)
                    rStick = -1.0f;

                if ( not groundTargetPtr)
                {
                    ipX = trackX = x;
                    ipY = trackY = y;
                    onStation = NotThereYet;
                }
                else
                {
                    ipX = trackX = groundTargetPtr->BaseData()->XPos();
                    ipY = trackY = groundTargetPtr->BaseData()->YPos();
                    onStation = HoldInPlace;
                }
            }
        }
        break;
        //======= End Downwind ===========================================

        //==================================================
        case Base:
            //TJL 11/09/03 Changed to Corner speed to allow for variety of AI
            desSpeed = cornerSpeed * 1.3f;
            //Cobra trackpoint uses knots only
            TrackPoint(0.0F, desSpeed /* KNOTS_TO_FTPSEC*/); // Cobra - aerial TrackPoint speed is in knots
            break;
            //======= End Base ===========================================

            // #3 -- final attack approach
            //==================================================
        case Final:
            //AI_MESSAGE(0, "AI - ON ATTACK FINAL");

            ClearFlag(GunFireFlag);

            if (groundTargetPtr)
            {
                SetTrackPoint(groundTargetPtr->BaseData()->XPos(), groundTargetPtr->BaseData()->YPos());

                // tell our wing to attack
                if (
                    self->GetCampaignObject()->NumberOfComponents() > 1 and 
 not isWing and sentWingAGAttack not_eq AG_ORDER_ATTACK
                )
                {
                    VU_ID targetId;

                    if (groundTargetPtr->BaseData()->IsSim())
                    {
                        targetId = ((SimBaseClass*)groundTargetPtr->BaseData())->GetCampaignObject()->Id();
                    }
                    else
                    {
                        targetId = groundTargetPtr->BaseData()->Id();
                    }

                    if (targetId not_eq FalconNullId)
                    {
                        AiSendCommand(self, FalconWingmanMsg::WMAssignTarget, AiFlight, targetId);
                        AiSendCommand(self, FalconWingmanMsg::WMShooterMode, AiFlight, targetId);
                        sentWingAGAttack = AG_ORDER_ATTACK;
                        // 2002-01-20 ADDED BY S.G. Added the new nextAttackCommandToSend
                        // variable check to force the lead to reissue an attack in case wings
                        // went back into formation (can we say HACK?)
                        nextAttackCommandToSend = SimLibElapsedTime + g_nAIshootLookShootTime * SEC_TO_MSEC;
                    }
                }
            }

            diveOK = FALSE;

            // Terrain follow around 1000 ft
            if (agApproach == AGA_LOW)
            {
                // if we're below track alt, kick us up a bit harder so we don't plow
                // into steeper slopes
                if (Sms->curWeapon and not Sms->curWeapon->IsMissile() and Sms->curWeapon->IsBomb())
                {
                    //TJL 10/28/03 Harm Altitude
                    if (self->ZPos() - curGroundAlt > -1000.0f)
                    {
                        trackZ = OTWDriver.GetGroundLevel(self->XPos() + self->XDelta(),
                                                          self->YPos() + self->YDelta());
                        //Cobra we are going to try and do a sanity check on alt now
                        float myAlt = self->ZPos();

                        if (fabsf(myAlt - self->GetA2GHarmAlt()) < 2000.0f)
                            trackZ = -self->GetA2GHarmAlt();
                        else
                            trackZ = self->ZPos();
                    }
                    else
                    {
                        //trackZ = OTWDriver.GetGroundLevel( trackX, trackY ) - 4000.0f;
                        trackZ = -self->GetA2GHarmAlt();
                    }
                }
                else
                {
                    trackZ = OTWDriver.GetGroundLevel(self->XPos() + self->XDelta(),
                                                      self->YPos() + self->YDelta());
                    //TJL 10/28/03

                    //Cobra we are going to try and do a sanity check on alt now
                    float myAlt = self->ZPos();

                    if (fabsf(myAlt - self->GetA2GHarmAlt()) < 2000.0f)
                        trackZ = -self->GetA2GHarmAlt();
                    else
                        trackZ = self->ZPos();
                }

                if (madeAGPass)
                    //TJL 11/09/03 Changed to Corner speed to allow for variety of AI
                    desSpeed = cornerSpeed * 1.3f;
                //desSpeed = 450.0f;
                else
                    //TJL 11/09/03 Changed to Corner speed to allow for variety of AI
                    desSpeed = cornerSpeed * 1.3f;

                //desSpeed = 650.0f;
                TrackPoint(0.0F, desSpeed);
            }
            else if (agApproach == AGA_DIVE)
            {
                if (self->ZPos() - curGroundAlt > -1000.0f)
                {
                    trackZ = OTWDriver.GetGroundLevel(self->XPos() + self->XDelta(),
                                                      self->YPos() + self->YDelta());

                    if (self->ZPos() - trackZ > -500.0f)
                        trackZ = trackZ - 500.0f - (self->ZPos() - trackZ + 500.0f) * 2.0f;
                    else
                        trackZ -= 500.0f;
                }
                else
                {
                    trackZ = OTWDriver.GetGroundLevel(trackX, trackY);
                    diveOK = TRUE;

                    if (Sms->curWeapon and not Sms->curWeapon->IsMissile() and Sms->curWeapon->IsBomb())
                        trackZ -= 2000.0f;
                    else if (Sms->curWeapon)
                        trackZ -= 50.0f;
                }

                //TJL 11/09/03 Changed to Corner speed to allow for variety of AI
                desSpeed = cornerSpeed * 1.3f;
                TrackPoint(0.0F, desSpeed);
            }
            else
            {
                trackZ = ipZ = AGattackAlt;
                //TJL 11/09/03 Changed to Corner speed to allow for variety of AI
                desSpeed = cornerSpeed * 1.3f;

                if (self->ZPos() - curGroundAlt > -1000.0f)
                {
                    trackZ = OTWDriver.GetGroundLevel(self->XPos() + self->XDelta(),
                                                      self->YPos() + self->YDelta());

                    if (self->ZPos() - trackZ > -500.0f)
                        trackZ = trackZ - 500.0f - (self->ZPos() - trackZ + 500.0f) * 2.0f;
                    // 2001-06-18 ADDED S.G. WHY DO THIS IF WE'RE GOING UP ALREADY?
                    else if (trackZ > AGattackAlt) // Are we going up? (don't forget negative is UP)
                        trackZ = AGattackAlt;
                    // END OF ADDED SECTION
                    else
                        trackZ -= 500.0f;
                }

                TrackPoint(0.0F, desSpeed);
            }

            // if we've got a missile get r mmax and min (which should be
            // fairly accurate now.
            // also we're going to predetermine if we'll take a missile shot
            // or not (mostly for harms).
            shootMissile = FALSE;
            droppingBombs = FALSE;

            // get accurate range and ata to target
            xft = trackX - self->XPos();
            yft = trackY - self->YPos();
            zft = trackZ - self->ZPos(); // Cobra - Ground range better
            approxRange = (float)sqrt(xft * xft + yft * yft);
            //approxSlantRange = (float)sqrt(xft*xft + yft*yft + zft*zft);
            //approxRange = max (approxRange, 1.0F);
            approxSlantRange = approxRange;

            // check for target
            if (groundTargetPtr == NULL)
            {
                if (approxRange < 1000.0f)
                    onStation = NotThereYet;

                break;
            }

            rx = self->dmx[0][0] * xft + self->dmx[0][1] * yft + self->dmx[0][2] * zft;
            ry = self->dmx[1][0] * xft + self->dmx[1][1] * yft + self->dmx[1][2] * zft;
            ata = (float)acos(rx / approxRange);

            // check for a photo mission
            if ((missionType == AMIS_BDA or missionType == AMIS_RECON) and hasCamera)
            {
                TakePicture(approxRange, ata);
            }
            // Might shoot a missile
            else
            {
                // preference given to stand-off missiles unless our
                // approach is high alt (bombing run)
                // 2001-07-18 ADDED BY S.G. DON'T GO TO THE TARGET BUT DO ANOTHER PASS FROM IP IF YOU HAVE NO WEAPONS LEFT
#if 0
                if ( not IsSetATC(HasCanUseAGWeapon))
                {
                    onStation = NotThereYet;
                    break;
                }

#else // FRB
                // Check for remaining AG weapons
                SelectGroundWeapon();

                if ( not (IsSetATC(HasCanUseAGWeapon)) and not (IsSetATC(HasAGWeapon)))
                {
                    ClearATCFlag(HasCanUseAGWeapon);
                    ClearATCFlag(HasAGWeapon);

                    // Go to next WP
                    if (GetWaypointIndex() == GetTargetWPIndex())
                        SelectNextWaypoint();

                    // go back to initial AG state
                    agDoctrine = AGD_NONE;
                    missionComplete = TRUE;
                    agmergeTimer = SimLibElapsedTime + 1;

                    // if we're a wingie, rejoin the lead
                    if (isWing)
                    {
                        mFormation = FalconWingmanMsg::WMWedge;
                        AiRejoin(NULL);
                        // make sure wing's designated target is NULL'd out
                        mDesignatedObject = FalconNullId;
                    }

                    onStation = NotThereYet;
                    break;
                }

#endif

                // END OF ADDED SECTION
                // Cobra - added JSOWs
                if ((hasAGMissile or hasHARM or (hasJSOW)) and groundTargetPtr)
                {
                    if (hasHARM)
                        shootMissile = HARMSetup(rx, ry, ata, approxRange);
                    else if (hasJSOW) // JSOW
                        shootMissile = JSOWSetup(rx, ry, ata, approxRange);
                    else
                        shootMissile = MaverickSetup(rx, ry, ata, approxRange, theRadar);

                    if (shootMissile == TRUE)
                    {
                        FireAGMissile(approxSlantRange, ata);
                    }
                    // 2001-06-24 ADDED BY S.G. IF shootMissile IS NOT FALSE (AND ALSO NOT TRUE), IF WE'RE CLOSE AND CAN'T LAUNCH HARM, SAY 'NO HARMS' or 'no JSOWs' AND TRY AGAIN RIGHT AWAY
                    else if (shootMissile not_eq FALSE)
                    {
                        // FRB
                        if (shootMissile == FALSE - 2) // Maverick - break away from target
                        {
                            // mark that we've completed an AG pass
                            madeAGPass = TRUE;
                            AGflyOut();
                            onStation = Downwind;
                            break;
                        }

                        // FRB - end
                        if (hasHARM)
                            hasHARM = FALSE;
                        else if (hasJSOW) // JSOW
                            hasBomb = FALSE;

                        // Cobra - get out of a possible forever loop
                        if (agmergeTimer not_eq 0 and SimLibElapsedTime > agmergeTimer)
                            break;

                        goto FinalSG;
                    }

                    // END OF ADDED SECTION
                }
                else if (hasGBU)
                {
                    DropGBU(approxSlantRange, ata, theRadar);
                }
                else if (hasBomb)
                {
                    DropBomb(approxSlantRange, ata, theRadar);  // AI bombing setup.  Bomb(s) dropped in Sms->DropBomb(int) in Doweapon.cpp
                }
                // rocket strafe attack
                else if (hasRocket)
                {
                    if (FireRocket(approxSlantRange, ata))
                    {
                        // fire rocket code is flying a/c
                        break;
                    }
                }
                // gun strafe attack
                else if (hasGun and agApproach == AGA_DIVE)
                {
                    GunStrafe(approxSlantRange, ata);
                }
            }

            // too close ?
            // we're within a certain range and our ATA is not good
            //dx = self->GetKias();
            //if (slowMover)
            //{
            // if (approxRange < 1.0f * NM_TO_FT)
            // {
            // waitingForShot = SimLibElapsedTime + 5000;
            // onStation = Final1;
            // }
            //}
            // FRB
            if (slowMover or agApproach == AGA_LOW)  // HD bombs
            {
                if (agApproach == AGA_LOW)
                {
                    if (approxRange < 0.0f * NM_TO_FT) // HD bombs
                    {
                        waitingForShot = SimLibElapsedTime + 5000;
                        onStation = Final1;
                    }
                }
                else if (approxRange < 1.0f * NM_TO_FT)
                {
                    waitingForShot = SimLibElapsedTime + 5000;
                    onStation = Final1;
                }
            }
            // FRB - end
            // COBRA - RED - FIXED WITH '0.1f' THE HD BOMBS UNDROPPED BUG... PROBLEM WAS GOING INTO FINAL1 TOO FAR FROM TARGET
            // IT WAS 2.5f
            else if (approxRange < 0.1f * NM_TO_FT) // was 1.5 bitand 75.0
            {
                //waitingForShot = SimLibElapsedTime + 5000;  // FRB
                onStation = Final1;
            }

            TrackPoint(0.0F, desSpeed/* KNOTS_TO_FTPSEC*/); // Cobra - aerial TrackPoint speed is in knots

            dx = (float)fabs(self->XPos() - trackX);
            dy = (float)fabs(self->YPos() - trackY);
            approxRange = (float)sqrt(dx * dx + dy * dy);

            // Increase the gains on final approach
            rStick *= 3.0f;

            if (rStick > 1.0f)
                rStick = 1.0f;
            else if (rStick < -1.0f)
                rStick = -1.0f;

            // pitch stick setting is based on our desired angle normalized to
            // 90 deg when in a dive
            if (agApproach == AGA_DIVE and diveOK)
            {
                // check hitting the ground and pull out of dive
                pitchDesired = (float)atan2(self->ZPos() - trackZ, approxRange);
                pitchDesired /= (90.0f * DTR);

                // keep stick at reasonable values.
                pStick = max(-0.7f, pitchDesired);
                pStick = min(0.7f, pStick);
            }

            break;
            //======= End Final ===========================================

            // #4 -- final attack approach hold for a sec and then head to next
            //==================================================
#if 1

            //==================================================
            // Cobra version
        case Final1:
        {
            if (Sms->CurRippleCount() > 0) // JB 011013
                break;

            ClearFlag(GunFireFlag);
            diveOK = FALSE;

            FCC->releaseConsent = PilotInputs::Off;

            // 2001-05-13 ADDED BY S.G. SO AI DROPS SOME COUNTER MEASURE AFTER A PASS
            self->DropProgramed();

            // mark that we've completed an AG pass
            madeAGPass = TRUE;

            // Terrain follow around 1000 ft
            if (agApproach == AGA_LOW)
            {
                trackZ = OTWDriver.GetGroundLevel(self->XPos() + self->XDelta(),
                                                  self->YPos() + self->YDelta());

                // if we're below track alt, kick us up a bit harder so we don't plow
                // into steeper slopes
                //TJL 10/28/03 Harm Alt
                if (self->ZPos() - trackZ > -1000.0f)
                    trackZ = -self->GetA2GGunRocketAlt();
                else
                    trackZ = -self->GetA2GGunRocketAlt();

                //TJL 11/09/03 Changed to Corner speed to allow for variety of AI
                desSpeed = cornerSpeed * 1.3f;
            }
            else if (agApproach == AGA_DIVE)
            {
                //TJL 11/09/03 Changed to Corner speed to allow for variety of AI
                desSpeed = cornerSpeed * 1.3f;

                if (self->ZPos() - curGroundAlt > -1000.0f)
                {
                    trackZ = OTWDriver.GetGroundLevel(self->XPos() + self->XDelta(),
                                                      self->YPos() + self->YDelta());

                    if (self->ZPos() - trackZ > -500.0f)
                        trackZ = trackZ - 500.0f - (self->ZPos() - trackZ + 500.0f) * 2.0f;
                    else
                        trackZ -= 500.0f;
                }
                else
                {
                    trackZ = OTWDriver.GetGroundLevel(trackX, trackY);
                    diveOK = TRUE;

                    if (Sms->curWeapon and not Sms->curWeapon->IsMissile() and Sms->curWeapon->IsBomb())
                        trackZ -= 2000.0f;
                    else if (Sms->curWeapon)
                        trackZ -= 50.0f;
                }
            }
            else
            {
                //TJL 11/09/03 Changed to Corner speed to allow for variety of AI
                desSpeed = cornerSpeed * 1.3f;

                if (self->ZPos() - curGroundAlt > -500.0f)
                {
                    trackZ = OTWDriver.GetGroundLevel(self->XPos() + self->XDelta(),
                                                      self->YPos() + self->YDelta());

                    if (self->ZPos() - trackZ > -500.0f)
                        trackZ = trackZ - 500.0f - (self->ZPos() - trackZ + 500.0f) * 2.0f;
                    else
                        trackZ -= 500.0f;
                }
            }

            SimpleTrackSpeed(desSpeed * KNOTS_TO_FTPSEC);

            dx = (float)fabs(self->XPos() - trackX);
            dy = (float)fabs(self->YPos() - trackY);
            approxRange = (float)sqrt(dx * dx + dy * dy);



            // pitch stick setting is based on our desired angle normalized to
            // 90 deg
            if (agApproach == AGA_DIVE and diveOK)
            {
                pitchDesired = (float)atan2(self->ZPos() - trackZ, approxRange);
                pitchDesired /= (90.0f * DTR);

                // keep stick at reasonable values.
                pStick = max(-0.7f, pitchDesired);
                pStick = min(0.7f, pStick);
            }

            if (missionType == AMIS_BDA or missionType == AMIS_RECON)
            {
                // clear and get new target next pass
                madeAGPass = TRUE;
                onStation = NotThereYet;
            }
            // Cobra - added JSOW
            else if ((hasAGMissile or hasHARM or (hasJSOW)) and not droppingBombs)
            {
                if (SimLibElapsedTime > waitingForShot)
                {
                    if (agDoctrine == AGD_SHOOT_RUN and groundTargetPtr)
                    {
                        // clear and get new target next pass
                        SetGroundTarget(NULL);

                        // this takes us back to missile fire
                        onStation = NotThereYet;
                        break;
                    }
                    //else if (hasAGMissile) // Mavericks
                    else if (hasAGMissile or hasHARM) // Mavericks bitand Harms
                    {
                        // Cobra - pull away from target
                        AGflyOut();
                        // Release current target and target history
                        SetGroundTarget(NULL);
                        gndTargetHistory[0] = NULL;
                        onStation = Downwind;
                        break;
                    }

                    // Check for remaining AG weapons
                    SelectGroundWeapon();

                    if ( not (IsSetATC(HasCanUseAGWeapon)) and not (IsSetATC(HasAGWeapon)))
                    {
                        ClearATCFlag(HasCanUseAGWeapon);
                        ClearATCFlag(HasAGWeapon);

                        // Go to next WP
                        if (GetWaypointIndex() == GetTargetWPIndex())
                            SelectNextWaypoint();

                        // go back to initial AG state
                        agDoctrine = AGD_NONE;
                        missionComplete = TRUE;
                        agmergeTimer = SimLibElapsedTime + 1;

                        // if we're a wingie, rejoin the lead
                        if (isWing)
                        {
                            mFormation = FalconWingmanMsg::WMWedge;
                            AiRejoin(NULL);
                            // make sure wing's designated target is NULL'd out
                            mDesignatedObject = FalconNullId;
                        }

                        onStation = NotThereYet;
                        return;
                    }

                    // this takes us back to 1st state
                    SetGroundTarget(NULL);
                    onStation = NotThereYet;
                    break;
                }
            }
            else if (droppingBombs == wcBombWpn)
            {
                DropBomb(approxSlantRange, 0.0F, theRadar);
            }
            else if (droppingBombs == wcGbuWpn)
            {
                DropGBU(approxSlantRange, 0.0F, theRadar);
            }

            /* else if (SimLibElapsedTime > waitingForShot or not hasWeapons)
                {
             // this takes us back to 1st state
             SetGroundTarget( NULL );
             onStation = NotThereYet;
             break;
                } */

            TrackPoint(0.0F, desSpeed/* KNOTS_TO_FTPSEC*/); // Cobra - aerial TrackPoint speed is in knots

            AGflyOut();
            // Release current target and target history
            SetGroundTarget(NULL);
            gndTargetHistory[0] = NULL;
            onStation = Downwind;

            // Check for remaining AB weapons
            SelectGroundWeapon();

            if ( not (IsSetATC(HasCanUseAGWeapon)) and not (IsSetATC(HasAGWeapon)))
            {
                ClearATCFlag(HasCanUseAGWeapon);
                ClearATCFlag(HasAGWeapon);

                // Go to next WP
                if (GetWaypointIndex() == GetTargetWPIndex())
                    SelectNextWaypoint();

                // go back to initial AG state
                agDoctrine = AGD_NONE;
                missionComplete = TRUE;
                agmergeTimer = SimLibElapsedTime + 1;

                // if we're a wingie, rejoin the lead
                if (isWing)
                {
                    mFormation = FalconWingmanMsg::WMWedge;
                    AiRejoin(NULL);
                    // make sure wing's designated target is NULL'd out
                    mDesignatedObject = FalconNullId;
                }

                onStation = NotThereYet;
                return;
            }

            break;
        }

        //======= End Final1 ===========================================
#else

        //==================================================
        // RV version
        case Final1:
            AI_MESSAGE(0, "AI - FINAL END");

            if (Sms->CurRippleCount() > 0) // JB 011013
                break;

            ClearFlag(GunFireFlag);
            diveOK = FALSE;

            FCC->releaseConsent = PilotInputs::Off;

            // 2001-05-13 ADDED BY S.G. SO AI DROPS SOME COUNTER MEASURE AFTER A PASS
            self->DropProgramed();

            // Cobra - pull away from target
            if (groundTargetPtr)
            {
                SetTrackPoint(groundTargetPtr->BaseData()->XPos(), groundTargetPtr->BaseData()->YPos());
            }

            dx = trackX - self->XPos();
            dy = trackY - self->YPos();
            approxRange = (float)sqrt(dx * dx + dy * dy);
            dir = 1.0f;

            if (rand() % 2)
                dir *= -1.0f;

            // Cobra - Slow-movers don't need to flyout too far
            if (slowMover)
            {
                if (dy < 0.0f)
                    ipX = trackX - dir * g_fAGSlowFlyoutRange * NM_TO_FT; // Cobra - added g_fAGFlyoutRange static range
                else
                    ipX = trackX - dir * g_fAGSlowFlyoutRange * NM_TO_FT; // Cobra - added g_fAGFlyoutRange static range

                if (dx < 0.0f)
                    ipY = trackY - dir * g_fAGSlowFlyoutRange * NM_TO_FT; // Cobra - added g_fAGFlyoutRange static range
                else
                    ipY = trackY + dir * g_fAGSlowFlyoutRange * NM_TO_FT;
            }
            else
            {
                if (dy < 0.0f)
                    ipX = trackX - dir * g_fAGFlyoutRange * NM_TO_FT; // Cobra - added g_fAGFlyoutRange static range
                else
                    ipX = trackX + dir * g_fAGFlyoutRange * NM_TO_FT;

                if (dx < 0.0f)
                    ipY = trackY - dir * g_fAGFlyoutRange * NM_TO_FT; // Cobra - added g_fAGFlyoutRange static range
                else
                    ipY = trackY + dir * g_fAGFlyoutRange * NM_TO_FT;
            }

            ipZ = trackZ = AGattackAlt;

            SetTrackPoint(ipX, ipY, AGattackAlt);

            // mark that we've completed an AG pass
            madeAGPass = TRUE;

            // Terrain follow around 1000 ft
            if (agApproach == AGA_LOW)
            {
                trackZ = OTWDriver.GetGroundLevel(self->XPos() + self->XDelta(),
                                                  self->YPos() + self->YDelta());

                // if we're below track alt, kick us up a bit harder so we don't plow
                // into steeper slopes
                //TJL 10/28/03 Harm Alt
                if (self->ZPos() - trackZ > -1000.0f)
                    trackZ = -self->GetA2GHarmAlt();
                else
                    trackZ = -self->GetA2GHarmAlt();

                //TJL 11/09/03 Changed to Corner speed to allow for variety of AI
                desSpeed = cornerSpeed * 1.3f;
            }
            else if (agApproach == AGA_DIVE)
            {
                //TJL 11/09/03 Changed to Corner speed to allow for variety of AI
                desSpeed = cornerSpeed * 1.3f;

                if (self->ZPos() - curGroundAlt > -1000.0f)
                {
                    trackZ = OTWDriver.GetGroundLevel(self->XPos() + self->XDelta(),
                                                      self->YPos() + self->YDelta());

                    if (self->ZPos() - trackZ > -500.0f)
                        trackZ = trackZ - 500.0f - (self->ZPos() - trackZ + 500.0f) * 2.0f;
                    else
                        trackZ -= 500.0f;
                }
                else
                {
                    trackZ = OTWDriver.GetGroundLevel(trackX, trackY);
                    diveOK = TRUE;

                    if (Sms->curWeapon and not Sms->curWeapon->IsMissile() and Sms->curWeapon->IsBomb())
                        trackZ -= 2000.0f;
                    else if (Sms->curWeapon)
                        trackZ -= 50.0f;
                }
            }
            else
            {
                //TJL 11/09/03 Changed to Corner speed to allow for variety of AI
                desSpeed = cornerSpeed * 1.3f;

                if (self->ZPos() - curGroundAlt > -500.0f)
                {
                    trackZ = OTWDriver.GetGroundLevel(self->XPos() + self->XDelta(),
                                                      self->YPos() + self->YDelta());

                    if (self->ZPos() - trackZ > -500.0f)
                        trackZ = trackZ - 500.0f - (self->ZPos() - trackZ + 500.0f) * 2.0f;
                    else
                        trackZ -= 500.0f;
                }
            }

            SimpleTrackSpeed(desSpeed * KNOTS_TO_FTPSEC);

            dx = (float)fabs(self->XPos() - trackX);
            dy = (float)fabs(self->YPos() - trackY);
            approxRange = (float)sqrt(dx * dx + dy * dy);



            // pitch stick setting is based on our desired angle normalized to
            // 90 deg
            if (agApproach == AGA_DIVE and diveOK)
            {
                pitchDesired = (float)atan2(self->ZPos() - trackZ, approxRange);
                pitchDesired /= (90.0f * DTR);

                // keep stick at reasonable values.
                pStick = max(-0.7f, pitchDesired);
                pStick = min(0.7f, pStick);
            }

            if (missionType == AMIS_BDA or missionType == AMIS_RECON)
            {
                // clear and get new target next pass
                madeAGPass = TRUE;
                onStation = NotThereYet;
            }
            // Cobra - added JSOW
            else if ((hasAGMissile or hasHARM or (hasJSOW)) and not droppingBombs)
            {
                if (SimLibElapsedTime > waitingForShot)
                {
                    if (agDoctrine == AGD_SHOOT_RUN and groundTargetPtr)
                    {
                        // clear and get new target next pass
                        SetGroundTarget(NULL);

                        // this takes us back to missile fire
                        onStation = NotThereYet;
                        break;
                    }
                    //else if (hasAGMissile) // Mavericks
                    else if (hasAGMissile or hasHARM) // Mavericks bitand Harms
                    {
                        // Cobra - pull away from target
                        AGflyOut();
                        // Release current target and target history
                        SetGroundTarget(NULL);
                        gndTargetHistory[0] = NULL;
                        onStation = Downwind;
                        break;
                    }

                    // Check for remaining AG weapons
                    SelectGroundWeapon();

                    if ( not (IsSetATC(HasCanUseAGWeapon)) and not (IsSetATC(HasAGWeapon)))
                    {
                        ClearATCFlag(HasCanUseAGWeapon);
                        ClearATCFlag(HasAGWeapon);

                        // Go to next WP
                        if (GetWaypointIndex() == GetTargetWPIndex())
                            SelectNextWaypoint();

                        // go back to initial AG state
                        agDoctrine = AGD_NONE;
                        missionComplete = TRUE;
                        agmergeTimer = SimLibElapsedTime + 1;

                        // if we're a wingie, rejoin the lead
                        if (isWing)
                        {
                            mFormation = FalconWingmanMsg::WMWedge;
                            AiRejoin(NULL);
                            // make sure wing's designated target is NULL'd out
                            mDesignatedObject = FalconNullId;
                        }

                        onStation = NotThereYet;
                        return;
                    }

                    // this takes us back to 1st state
                    SetGroundTarget(NULL);
                    onStation = NotThereYet;
                    break;
                }
            }
            else if (droppingBombs == wcBombWpn)
            {
                DropBomb(approxRange, 0.0F, theRadar);
            }
            else if (droppingBombs == wcGbuWpn)
            {
                DropGBU(approxRange, 0.0F, theRadar);
            }
            else if (SimLibElapsedTime > waitingForShot or not hasWeapons)
            {
                // this takes us back to 1st state
                SetGroundTarget(NULL);
                onStation = NotThereYet;
            }

            AGflyOut();
            // Release current target and target history
            SetGroundTarget(NULL);
            gndTargetHistory[0] = NULL;
            onStation = Downwind;

            // Check for remaining AB weapons
            SelectGroundWeapon();

            if ( not (IsSetATC(HasCanUseAGWeapon)) and not (IsSetATC(HasAGWeapon)))
            {
                ClearATCFlag(HasCanUseAGWeapon);
                ClearATCFlag(HasAGWeapon);

                // Go to next WP
                if (GetWaypointIndex() == GetTargetWPIndex())
                    SelectNextWaypoint();

                // go back to initial AG state
                agDoctrine = AGD_NONE;
                missionComplete = TRUE;
                agmergeTimer = SimLibElapsedTime + 1;

                // if we're a wingie, rejoin the lead
                if (isWing)
                {
                    mFormation = FalconWingmanMsg::WMWedge;
                    AiRejoin(NULL);
                    // make sure wing's designated target is NULL'd out
                    mDesignatedObject = FalconNullId;
                }

                onStation = NotThereYet;
                return;
            }

            break;
            //======= End Final1 ===========================================
#endif

            // after bombing run, we come here to pull up
            //==================================================
        case Stabalizing:
        {
            dx = (float)fabs(self->XPos() - trackX);
            dy = (float)fabs(self->YPos() - trackY);
            approxRange = (float)sqrt(dx * dx + dy * dy);

            if (approxRange < 2.0f * NM_TO_FT)
                onStation = NotThereYet;

            SetGroundTarget(NULL);

            //TJL 11/09/03 Changed to Corner speed to allow for variety of AI
            desSpeed = cornerSpeed * 1.3f;
            TrackPoint(0.0F, desSpeed/* KNOTS_TO_FTPSEC*/); // Cobra - aerial TrackPoint speed is in knots

            break;
        }

        //======= End Stabalizing ===========================================

        case Taxi:
            break;
    } // end switch (onStation)

    //======= End End End ===========================================

    // Been doing this long enough, go to the next waypoint
    //    if (groundTargetPtr and SimLibElapsedTime > agmergeTimer)
    if (agmergeTimer not_eq 0 and SimLibElapsedTime > agmergeTimer)
    {
        ClearATCFlag(HasCanUseAGWeapon);
        ClearATCFlag(HasAGWeapon);

        if (GetWaypointIndex() == GetTargetWPIndex())
            SelectNextWaypoint();

        // Cobra - finsih up and go to next WP
        // go back to initial AG state
        agDoctrine = AGD_NONE;
        missionComplete = TRUE;

        // if we're a wingie, rejoin the lead
        if (isWing)
        {
            mFormation = FalconWingmanMsg::WMWedge;
            AiRejoin(NULL, AI_REJOIN);   // Cobra - Try this hint
            // make sure wing's designated target is NULL'd out
            mDesignatedObject = FalconNullId;
        }

        agmergeTimer = SimLibElapsedTime + 1;
        //agmergeTimer = 0; // Cobra - reinitialize
    }
}

/*
 ** Name: SetGroundTarget
 ** Description:
 ** Creates a SimObjectType struct for the entity, sets the groundTargetPtr,
 ** References the target.  Any pre-existing target is dereferenced.
 */
void DigitalBrain::SetGroundTarget(FalconEntity *obj)
{
    if (obj not_eq NULL)
    {
        if (groundTargetPtr not_eq NULL)
        {
            // release existing target data if different object
            if (groundTargetPtr->BaseData() not_eq obj)
            {
                groundTargetPtr->Release();
                groundTargetPtr = NULL;
            }
            else
            {
                // already targeting this object
                return;
            }
        }

        // create new target data and reference it
        groundTargetPtr = new SimObjectType(obj);
        groundTargetPtr->Reference();
        // SetTarget( groundTargetPtr );
    }
    else  // obj is null
    {
        if (groundTargetPtr not_eq NULL)
        {
            groundTargetPtr->Release();
            groundTargetPtr = NULL;
        }
    }
}

/*
 ** Name: SetGroundTargetPtr
 ** Description:
 ** Sets GroundTargetPtr witch supplied SimObjectType
 */
void DigitalBrain::SetGroundTargetPtr(SimObjectType *obj)
{
    if (obj not_eq NULL)
    {
        if (groundTargetPtr not_eq NULL)
        {
            // release existing target data if different object
            if (groundTargetPtr not_eq obj)
            {
                groundTargetPtr->Release();
                groundTargetPtr = NULL;
            }
            else
            {
                // already targeting this object
                return;
            }
        }


        // set and reference
        groundTargetPtr = obj;
        groundTargetPtr->Reference();
        // SetTarget( groundTargetPtr );
    }
    else // obj is null
    {
        if (groundTargetPtr not_eq NULL)
        {
            groundTargetPtr->Release();
            groundTargetPtr = NULL;
        }
    }
}

/*
 ** Name: SelectGroundTarget
 ** Description:
 */
void DigitalBrain::SelectGroundTarget(int targetFilter)
{
    CampBaseClass* campTarg;
    Tpoint pos;
    VuGridIterator* gridIt = NULL;
    UnitClass *campUnit = (UnitClass *)self->GetCampaignObject();
    WayPointClass *dwp, *cwp;
    int relation;

    // No targeting when RTB
    if (curMode == RTBMode)
    {
        SetGroundTarget(NULL);
        return;
    }

    // 1st let camp select target
    SelectCampGroundTarget();


    // if we've got one we're done
    if (groundTargetPtr)
        return;

    // if we're not on interdiction type mission, return....
    //Cobra: We want to let the AI target as necessary when weapons free
    /*if ( missionType not_eq AMIS_SAD and missionType not_eq AMIS_INT and missionType not_eq AMIS_BAI )
      return;*/

    if ( not campUnit)
        return;

    // divert waypoint overrides everything else
    dwp = ((FlightClass *)campUnit)->GetOverrideWP();

    if ( not dwp)
        cwp = self->curWaypoint;
    else
        cwp = dwp;

    /*if (not cwp or cwp->GetWPAction() not_eq WP_SAD )
      return;*/ //Cobra, remove this so that we can attack regardless


    // choose how we are going to attack and whom....
    cwp->GetLocation(&pos.x, &pos.y, &pos.z);

#ifdef VU_GRID_TREE_Y_MAJOR
    gridIt = new VuGridIterator(RealUnitProxList, pos.y, pos.x, 5.0F * NM_TO_FT);
#else
    gridIt = new VuGridIterator(RealUnitProxList, pos.x, pos.y, 5.0F * NM_TO_FT);
#endif


    // get the 1st objective that contains the bomb
    campTarg = (CampBaseClass*)gridIt->GetFirst();

    while (campTarg not_eq NULL)
    {
        if (campTarg->IsAirplane() or campTarg->IsFlight()) //Cobra
        {
            campTarg = (CampBaseClass*)gridIt->GetNext();
            continue;
        }

        relation = TeamInfo[self->GetTeam()]->TStance(campTarg->GetTeam());

        if (relation == Hostile or relation == War)
        {
            break;
        }

        campTarg = (CampBaseClass*)gridIt->GetNext();
    }

    SetGroundTarget(campTarg);

    if (gridIt)
        delete gridIt;

    return;
}

/*
 ** Name: SelectGroundWeapon
 ** Description:
 */
void DigitalBrain::SelectGroundWeapon(void)
{
    int i;
    Falcon4EntityClassType* classPtr;
    int runAway = FALSE;
    SMSClass* Sms = self->Sms;
    BombClass *theBomb;

    hasAGMissile = FALSE;
    hasBomb = FALSE;
    hasHARM = FALSE;
    hasGun = FALSE;
    hasCamera = FALSE;
    hasRocket = FALSE;
    hasGBU = FALSE;

    // always make sure the FCC is in a weapons neutral mode when a
    // weapon selection is made.  Potentially we may be out of missiles
    // and have a SMS current bomb selected by this function and be in
    // FCC air-air mode which will cause a crash.
    //Cobra Reset to Nav messes up SOI for human player.
    //self->FCC->SetMasterMode (FireControlComputer::Nav);
    self->FCC->preDesignate = TRUE;


    // Set no AG weapon, set true if found
    ClearATCFlag(HasAGWeapon);

    // look for a bomb and/or a missile
    for (i = 0; i < self->Sms->NumHardpoints(); i++)
    {
        theBomb = self->FCC->GetTheBomb();

        // Check for AG Missiles
        if ( not hasAGMissile and Sms->hardPoint[i]->weaponPointer and Sms->hardPoint[i]->GetWeaponClass() == wcAgmWpn)
        {
            hasAGMissile = TRUE;
        }
        else if ( not hasHARM and Sms->hardPoint[i]->weaponPointer and Sms->hardPoint[i]->GetWeaponClass() == wcHARMWpn)
        {
            hasHARM = TRUE;
        }
        else if ( not hasBomb and Sms->hardPoint[i]->weaponPointer and Sms->hardPoint[i]->GetWeaponClass() == wcBombWpn)
        {
            // 2001-07-11 ADDED BY S.G. SO DURANDAL AND CLUSTER and JDAM and JSOW ARE ACCOUNTED FOR DIFFERENTLY THAN NORMAL BOMBS
            hasBomb = TRUE;

            // JDAM or JSOW
            if (Sms->hardPoint[i]->GetWeaponType() == wtGPS or (theBomb and theBomb->IsSetBombFlag(BombClass::IsJSOW)))
            {
                if (theBomb and theBomb->EntityType()->classInfo_[VU_STYPE] == STYPE_BOMB_JSOW)
                    hasBomb = TRUE + 4;
                else  // JDAM
                    hasBomb = TRUE + 3;
            }
            else if (Sms->hardPoint[i]->GetWeaponData()->cd >= 0.9f) // S.G. used edg kludge: drag coeff >= 1.0 is a durandal (w/chute) BUT 0.9 is hardcode for high drag :-(
                hasBomb = TRUE + 1;
            else if (Sms->hardPoint[i]->GetWeaponData()->flags bitand SMSClass::HasBurstHeight) // S.G. If it has burst height, it's a cluster bomb
            {
                if (theBomb and theBomb->EntityType()->classInfo_[VU_STYPE] == STYPE_BOMB_JSOW)
                    hasBomb = TRUE + 4;
                else
                    hasBomb = TRUE + 2;
            }
        }
        else if ( not hasGBU and Sms->hardPoint[i]->weaponPointer and Sms->hardPoint[i]->GetWeaponClass() == wcGbuWpn)
        {
            hasGBU = TRUE;
        }
        else if ( not hasRocket and 
                 Sms->hardPoint[i]->GetWeaponClass() == wcRocketWpn  and 
                 Sms->hardPoint[i]->GetWeaponType()  == wtLAU)
        {
            SimWeaponClass *weap = Sms->hardPoint[i]->weaponPointer.get();

            while (weap and not weap->IsUseable())
            {
                weap = weap->GetNextOnRail();
            }

            if (weap)
            {
                hasRocket = TRUE;

                if (rocketMnvr not_eq RocketFlyout and rocketMnvr not_eq RocketJink and rocketMnvr not_eq RocketFiring)
                {
                    if (rocketMnvr not_eq RocketFlyToTgt)
                    {
                        rocketMnvr = RocketFlyToTgt; // Cobra Interrupts RocketFlyOut before far enough from target to get good approach.
                        rocketTimer = 60; // 1 minute for something to happen
                    }
                }
            }
        }
        else if ( not hasCamera and Sms->hardPoint[i]->weaponPointer and Sms->hardPoint[i]->GetWeaponClass() == wcCamera)
        {
            hasCamera = TRUE;
        }
    }


    // finally look for guns
    // only the A-10 and SU-25 are guns-capable for A-G
    classPtr = (Falcon4EntityClassType*)self->EntityType();

    if (classPtr->vuClassData.classInfo_[VU_STYPE] == STYPE_AIR_ATTACK and 
        (classPtr->vuClassData.classInfo_[VU_SPTYPE] == SPTYPE_A10 or
         classPtr->vuClassData.classInfo_[VU_SPTYPE] == SPTYPE_SU25))
    {
        if (self->Guns and 
            self->Guns->numRoundsRemaining)
        {
            hasGun = TRUE;
        }
    }

    if (hasAGMissile bitor hasBomb bitor hasHARM bitor hasRocket bitor hasGBU)
    {
        SetATCFlag(HasAGWeapon);

        // 2001-05-27 ADDED BY S.G. IF WE HAVE HARMS AND OUR TARGET IS NOT EMITTING, CLEAR hasHARM ONLY IF WE HAVE SOMETHING ELSE ON BOARD
        // 2001-06-20 MODIFIED BY S.G. EVEN IF ONLY HAVE HARMS, DO THIS. HOPEFULLY WING WILL REJOIN AND LEAD WILL TERMINATE IF ON SEAD STRIKES
        //   if (hasHARM and groundTargetPtr and (hasAGMissile bitor hasBomb bitor hasRocket bitor hasGBU)) {
        if (hasHARM and groundTargetPtr)
        {
            // If it's a sim entity, look at its radar type)
            if (groundTargetPtr->BaseData()->IsSim())
            {
                // 2001-06-20 MODIFIED BY S.G. IT DOESN'T MATTER AT THIS POINT IF IT'S EMITTING OR NOT. ALL THAT MATTERS IS - IS IT A RADAR VEHICLE/FEATURE FOR A SIM OR DOES IT HAVE A RADAR VEHICLE IF A CAMPAIGN ENTITY
                // No need to check if they exists because if they got selected, it's because they exists. Only check if they have a radar
                if ( not ((groundTargetPtr->BaseData()->IsVehicle() and ((SimVehicleClass *)groundTargetPtr->BaseData())->GetRadarType() not_eq RDR_NO_RADAR) or // It's a vehicle and it has a radar
                      (groundTargetPtr->BaseData()->IsStatic() and ((SimStaticClass *)groundTargetPtr->BaseData())->GetRadarType() not_eq RDR_NO_RADAR))) // It's a feature and it has a radar
                    hasHARM = FALSE;
            }
            // NOPE - It's a campaign object, if it's aggregated, can't use HARM unless no one has chosen it yet.
            else if (((CampBaseClass *)groundTargetPtr->BaseData())->IsAggregate())
            {
                // 2002-01-20 ADDED BY S.G. Unless it's an AAA since it has more than one radar.
                if ( not groundTargetPtr->BaseData()->IsBattalion() or ((BattalionClass *)groundTargetPtr->BaseData())->class_data->RadarVehicle < 16)
                {
                    // END OF ADDED SECTION 2002-01-20
                    if (((FlightClass *)self->GetCampaignObject())->shotAt not_eq groundTargetPtr->BaseData())
                    {
                        ((FlightClass *)self->GetCampaignObject())->shotAt = groundTargetPtr->BaseData();
                        ((FlightClass *)self->GetCampaignObject())->whoShot = self;
                    }
                    else if (((FlightClass *)self->GetCampaignObject())->whoShot not_eq self)
                        hasHARM = FALSE;
                }
            }
        }

        // END OF ADDED SECTION
    }

    // 2001-06-30 ADDED BY S.G. SO THE TRUE WEAPON STATE IS KEPT...
    if (hasAGMissile bitor hasBomb bitor hasHARM bitor hasRocket bitor hasGBU)
        SetATCFlag(HasCanUseAGWeapon);
    else
        ClearATCFlag(HasCanUseAGWeapon);

    // END OF ADDED SECTION

    hasWeapons = hasAGMissile bitor hasBomb bitor hasHARM bitor hasRocket bitor hasGun bitor hasGBU;

    // 2001-06-20 ADDED BY S.G. LEAD WILL TAKE ITS WING WEAPON LOADOUT INTO CONSIDERATION BEFORE ABORTING
    // 2001-06-30 MODIFIED BY S.G. IF NOT A ENROUTE SEAD TARGET, SKIP HARMS AS AVAILABLE IF IT CAN'T BE FIRED
    int ret;

    if ( not hasWeapons and not isWing and ((ret = IsNotMainTargetSEAD()) or sentWingAGAttack not_eq AG_ORDER_ATTACK))
    {
        int i;
        int usComponents = self->GetCampaignObject()->NumberOfComponents();

        for (i = 0; i < usComponents; i++)
        {
            AircraftClass *flightMember = (AircraftClass *)self->GetCampaignObject()->GetComponentEntity(i);

            if (flightMember and flightMember->DBrain() and flightMember->DBrain()->IsSetATC(ret ? HasAGWeapon : HasCanUseAGWeapon))
            {
                hasWeapons = TRUE;
                SetATCFlag(HasAGWeapon);
                break;
            }
        }
    }

    // END OF ADDED SECTION
    // make sure, if we're guns or rockets only, that are approach is
    // a dive
    if ( not hasBomb and not hasGBU and not hasAGMissile and not hasHARM and (hasGun or hasRocket))
    {
        agApproach = AGA_DIVE;
        ipZ = -self->GetA2GGunRocketAlt();
    }

    // Check for run-away case
    if (missionType == AMIS_BDA or missionType == AMIS_RECON)
    {
        if ( not hasCamera)
        {
            runAway = TRUE;
        }
    }
    // 2001-06-20 MODIFIED BY S.G. SO AI DO NOT RUN AWAY IF YOU STILL HAVE HARMS AND ON A SEAD TYPE MISSION
    // else if ( not hasWeapons)
    else if ( not hasWeapons and not (IsSetATC(HasAGWeapon) and IsNotMainTargetSEAD()))
    {
        runAway = TRUE;
    }

    if (missionType == AMIS_AIRCAV)
    {
        runAway = false;
    }

    // 2002-03-08 ADDED BY S.G. Don't run away if designating...
    if ((moreFlags bitand KeepLasing) and runAway == TRUE)
        runAway = FALSE;

    // END OF ADDED SECTION 2002-03-08

    // Cobra - Check for over-staying welcome.  missionClass was changed to AAMission by SelectNextWaypoint()
    if (runAway or ((SimLibElapsedTime > agmergeTimer or agmergeTimer == 0) and missionClass == AAMission))//
    {
        // no AG weapons, next waypoint....
        agDoctrine = AGD_NONE;
        // 2001-08-04 MODIFIED BY S.G. SET missionComplete ONLY ONCE WE TEST IT (ADDED THAT TEST FOR THE IF AS WELL)
        // missionComplete = TRUE;
        self->FCC->SetMasterMode(FireControlComputer::Nav);
        self->FCC->preDesignate = TRUE;
        SetGroundTarget(NULL);

        if (missionClass == AGMission and not missionComplete and agImprovise == FALSE and not self->OnGround())
        {
            // JB 020315 Only skip to the waypoint after the target waypoint. Otherwise we may go into landing mode too early.
            WayPointClass* tmpWaypoint = self->curWaypoint;

            while (tmpWaypoint)
            {
                if (tmpWaypoint->GetWPFlags() bitand WPF_TARGET)
                {
                    tmpWaypoint = tmpWaypoint->GetNextWP();
                    break;
                }

                tmpWaypoint = tmpWaypoint->GetNextWP();
            }

            if (tmpWaypoint)
                SelectNextWaypoint();
        }

        missionComplete = TRUE; /*S.G.*/

        // if we're a wingie, rejoin the lead
        if (isWing)
        {
            // 2001-05-03 ADDED BY S.G. WE WANT WEDGE AFTER GROUND PASS
            mFormation = FalconWingmanMsg::WMWedge;
            // END OF ADDED SECTION
            AiRejoin(NULL);
            // make sure wing's designated target is NULL'd out
            mDesignatedObject = FalconNullId;
        }
    }
}

/*
 ** Name: SelectCampGroundTarget
 ** Description:
 ** Uses the campaign functions to set ground targets
 */
void DigitalBrain::SelectCampGroundTarget(void)
{
    UnitClass *campUnit = (UnitClass *)self->GetCampaignObject();
    FalconEntity *target = NULL;
    // int i, numComponents;
    SimBaseClass *simTarg;
    WayPointClass *dwp;

    // 2001-07-15 REMOVED BY S.G. THIS IS CLEARED IN THE 'Final' STAGE ONLY
    // madeAGPass = FALSE;

    agImprovise = FALSE;

    // sanity check
    if ( not campUnit)
        return;

    // divert waypoint overrides everything else
    dwp = ((FlightClass *)campUnit)->GetOverrideWP();

    // check to see if our current ground target is a sim and exploding or
    // dead, if so let's get a new target from the campaign
    if (
        groundTargetPtr and 
        groundTargetPtr->BaseData()->IsSim() and 
        (
            groundTargetPtr->BaseData()->IsExploding() or
            groundTargetPtr->BaseData()->IsDead() or
 not ((SimBaseClass *)groundTargetPtr->BaseData())->IsAwake()
        )
    )
    {
        SetGroundTarget(NULL);
    }

    // see if we've already got a target
    if (groundTargetPtr)
    {
        target = groundTargetPtr->BaseData();

        // is it a campaign object? if not we can return....
        if (target->IsSim())
        {
            return;
        }

        // itsa campaign object.  Check to see if its deagg'd
        if (((CampBaseClass*)target)->IsAggregate())
        {
            // still aggregated, return
            return;
        }

        // the campaign object is now deaggregated, choose a sim entity
        // to target on it
        // 2001-04-11 MODIFIED BY S.G. SO LEAD USES THE ASSIGNED TARGET IF IT'S AN OBJECTIVE AND MAKES A BETTER SELECTION ON MOVERS
        /* numComponents = ((CampBaseClass*)target)->NumberOfComponents();

         for ( i = 0; i < numComponents; i++ )
         {
         simTarg = ((CampBaseClass*)target)->GetComponentEntity( rand() % numComponents );
         if ( not simTarg ) //sanity check
         continue;

        // don't target runways (yet)
        if ( // not simTarg->IsSetCampaignFlag (FEAT_FLAT_CONTAINER) and 
 not simTarg->IsExploding() and 
 not simTarg->IsDead() and 
        simTarg->pctStrength > 0.0f )
        {
        SetGroundTarget( simTarg );
        break;
        }
        } // end for # components
        */

        // RV - RED - Wrong Building Fix - Default to Assigned Building
        int targetNum = self->curWaypoint->GetWPTargetBuilding();
        // int targetNum = 0;

        // First, the lead will go for the assigned target, if any...
        if ( not isWing and target->IsObjective())
        {
            FalconEntity *wpTarget = NULL;
            WayPointClass *twp = self->curWaypoint;

            // First prioritize the divert waypoint target
            if (dwp)
                wpTarget = dwp->GetWPTarget();

            // If wpTarget is not NULL, our waypoint will be the divert waypoint
            if (wpTarget)
                twp = dwp;
            else
            {
                // Our target will be the current waypoint target if any
                if (self->curWaypoint)
                    wpTarget = self->curWaypoint->GetWPTarget();
            }

            // If we have a waypoint target and it is our current target
            if (wpTarget and wpTarget == target)
                // Our feature is the one assigned to us by the mission planner
                targetNum = twp->GetWPTargetBuilding();
        }

        // Use our helper function
        simTarg = FindSimGroundTarget((CampBaseClass*)target, ((CampBaseClass*)target)->NumberOfComponents(), targetNum);

        // Hopefully, we have one...
        SetGroundTarget(simTarg);
        // END OF ADDED SECTION

        return;

    } // end if already targetPtr

    // priority goes to the waypoint target
    if (dwp)
    {
        target = dwp->GetWPTarget();

        if ( not target)
        {
            dwp = NULL;

            if (self->curWaypoint)
            {
                target = self->curWaypoint->GetWPTarget();
            }
        }
    }
    else if (self->curWaypoint)
    {
        target = self->curWaypoint->GetWPTarget();
    }

    if (target and target->OnGround())
    {
        SetGroundTarget(target);
        return;
    }


    // at this point we have no target, we're going to ask the campaign
    // to find out what we're supposed to hit

    // tell unit we haven't done any checking on it yet
    campUnit->UnsetChecked();

    // choose target.  I assume if this returns 0, no target....
    // 2001-06-09 MODIFIED BY S.G. NEED TO SEE IF WE ARE CHANGING CAMPAIGN TARGET AND ON A SEAD ESCORT MISSION. IF SO, DEAL WITH IT
    /* if ( not campUnit->ChooseTarget() )
     {
    // alternately try and choose the waypoint's target
    // SetGroundTarget( self->curWaypoint->GetWPTarget() );
    return;
    }

    // get the target
    target = campUnit->GetTarget();
    */
    // Choose and get this target
    int ret;
    ret = campUnit->ChooseTarget();
    target = campUnit->GetTarget();

    // If ChooseTarget returned FALSE or we changed campaign target and we're the lead  (but we must had a previous campaign target first)
    if ( not isWing and lastGndCampTarget and ( not ret or target not_eq lastGndCampTarget))
    {
        agDoctrine = AGD_NONE; // Need to setup our next ground attack
        onStation = NotThereYet; // Need to do a new pass next time
        sentWingAGAttack = AG_ORDER_NONE; // Next time, direct wingmen on target
        lastGndCampTarget = NULL; // No previous campaign target

        if (self->GetCampaignObject()->NumberOfComponents() > 1)
        {
            AiSendCommand(self, FalconWingmanMsg::WMWedge, AiFlight, FalconNullId); // Ask wingmen to resume a wedge formation
            AiSendCommand(self, FalconWingmanMsg::WMRejoin, AiFlight, FalconNullId); // Ask wingmen to rejoin
            AiSendCommand(self, FalconWingmanMsg::WMCoverMode, AiFlight, FalconNullId); // And stop what they were doing
        }
    }
    else
        // Keep track of this campaign target
        lastGndCampTarget = (CampBaseClass *)target;

    if ( not ret)
        return;

    // END OF MODIFIED SECTION

    // get tactic -- not doing anything with it now
    campUnit->ChooseTactic();
    campTactic = campUnit->GetUnitTactic();

    // sanity check and make sure its on ground, what to do if not?...
    if ( not target or
 not target->OnGround() or
        (campTactic not_eq ATACTIC_ENGAGE_STRIKE and 
         campTactic not_eq ATACTIC_ENGAGE_SURFACE and 
         campTactic not_eq ATACTIC_ENGAGE_DEF and 
         campTactic not_eq ATACTIC_ENGAGE_NAVAL))
        return;


    // set it as our target
    SetGroundTarget(target);

}

/*
 ** Name: SetupAGMode
 ** Description:
 ** Prior to entering an air to ground attack mode, we do some
 ** setup to determine what our attack doctrine is going to be and
 ** how we will approach the target.
 ** When agDoctrine is set to other than AGD_NONE, this signals that
 ** Setup has been completed for the AG attack.
 ** SEAD and CASCP waypoints are special cases in that targets aren't
 ** necessarily tied to the waypoint but the entire flight route -- we
 ** check for these targets periodically.
 **
 ** The 1st arg is current waypoint (should be an IP)
 ** 2nd arg is next WP (presumably where action is)
 */
// 2001-07-11 REDONE BY S.G.
// This routine is called until an attack profile can be established, then it is no longer call unless agDoctrine is reset to AGD_NONE
void DigitalBrain::SetupAGMode(WayPointClass *cwp, WayPointClass *wp)
{
    int wpAction;
    UnitClass *campUnit = (UnitClass *)self->GetCampaignObject();
    CampBaseClass *campBaseTarg;
    float dx, dy, dz, range;
    Falcon4EntityClassType* classPtr;
    WayPointClass *dwp;
    SMSClass* Sms = self->Sms;

    //================================================
    // Cobra - bypass Setup while attacking with rockets
    //================================================
    if ((Sms->curWeapon and Sms->curWeapon->IsLauncher() and hasRocket and onStation == Final) or rocketMnvr == RocketFiring)
    {
        return;
    }

    //================================================

    // Cobra - Initialize the loitering timer
    if (agmergeTimer < 0)
    {
        int LoiterTime = g_nGroundAttackTime;

        switch (self->curWaypoint->GetWPAction())
        {
            case WP_GNDSTRIKE:
            case WP_NAVSTRIKE:
            case WP_STRIKE:
            case WP_BOMB:
                LoiterTime = g_nStrikeAttackTime;
                break;

            case WP_SEAD:
                LoiterTime = g_nSeadAttackTime;
                break;

            case WP_SAD:
            case WP_CASCP:
            case WP_RECON:
                LoiterTime = g_nCASAttackTime;
                break;

            default:
                LoiterTime = g_nGroundAttackTime;
                break;
        }

        agmergeTimer = SimLibElapsedTime + LoiterTime * 60 * SEC_TO_MSEC;


        // Cobra - Try turning your AI wingmen loose
        if (self->GetCampaignObject()->NumberOfComponents() > 1 and not isWing)
            // RED - FIXED CTD - AI should not use this function
            // AiSendPlayerCommand( FalconWingmanMsg::WMWeaponsFree, AiFlight );
            AiSendCommand(self, FalconWingmanMsg::WMWeaponsFree, AiFlight, FalconNullId);
    }

    // So we don't think our mission is complete and forget to go to CrossWind from 'NotThereYet'
    // Cobra - if we have been attacking long enough, go home
    if (agmergeTimer not_eq 0 and SimLibElapsedTime > agmergeTimer)
    {
        ClearATCFlag(HasCanUseAGWeapon);
        ClearATCFlag(HasAGWeapon);
        agmergeTimer = SimLibElapsedTime + 1;
        SetGroundTarget(NULL);
        agDoctrine = AGD_NONE;
        missionComplete = TRUE;
        return;
    }
    else
        missionComplete = FALSE;

    agImprovise = FALSE;

    // First, lets get a target if we're the lead, otherwise use the target provided by the lead...

    if ( not isWing)
    {
        dwp = NULL;

        if (campUnit)
            dwp = ((FlightClass *)campUnit)->GetOverrideWP();

        // If we were passed a target wayp
        if (wp)
        {
            // First have the lead fly toward the IP waypoint until he can see a target
            if (cwp not_eq wp)
            {
                cwp->GetLocation(&ipX, &ipY, &ipZ);

                // Next waypoint is our target waypoint
                SelectNextWaypoint();

                // If we have HARM or JSOW on board (even if we can't use them), start your attack from here
                // Cobra - JSOWs start attack here also
                if ((hasHARM) or (hasBomb == TRUE + 4))
                {
                    ipX = self->XPos();
                    ipY = self->YPos();
                }
            }
            else
            {
                if (dwp)
                    dwp->GetLocation(&ipX, &ipY, &ipZ);
                else
                    wp->GetLocation(&ipX, &ipY, &ipZ);

            }

            wpAction = wp->GetWPAction();

            // If we have nothing, look at our enroute action
            if (wpAction == WP_NOTHING)
                wpAction = wp->GetWPRouteAction();

            // If it's a SEAD or CASCP waypoint, do the following...
            if ((wpAction == WP_SEAD or wpAction == WP_CASCP) and cwp == wp)
            {
                // But only if it is time to retarget, otherwise stay quiet
                if (SimLibElapsedTime > nextAGTargetTime)
                {
                    // Next retarget in 5 seconds
                    nextAGTargetTime = SimLibElapsedTime + 5000;

                    // First, lets release our current target and target history
                    SetGroundTarget(NULL);
                    gndTargetHistory[0] = NULL;

                    // The first call should get a campaign entity while the second one will fetch a sim entity within
                    SelectGroundTarget(TARGET_ANYTHING);

                    if (groundTargetPtr == NULL)
                        return;

                    if (groundTargetPtr->BaseData()->IsCampaign() and not ((CampBaseClass *)groundTargetPtr->BaseData())->IsAggregate())
                        SelectGroundTarget(TARGET_ANYTHING);
                }
                else
                    return;
            }
            else
            {
                // divert waypoint overrides everything else
                if (dwp)
                {
                    campBaseTarg = dwp->GetWPTarget();

                    if ( not campBaseTarg)
                    {
                        dwp = NULL;
                        campBaseTarg = wp->GetWPTarget();
                    }
                }
                else
                    campBaseTarg = wp->GetWPTarget();

                // See if we got a target waypoint target, if not, try and see if we can select one by using the campaign target selector
                if (campBaseTarg == NULL)
                {
                    if (SimLibElapsedTime > nextAGTargetTime)
                    {
                        // Next retarget in 5 seconds
                        nextAGTargetTime = SimLibElapsedTime + 5000;

                        // First, lets release our current target and target history
                        SetGroundTarget(NULL);
                        gndTargetHistory[0] = NULL;

                        // The first call should get a campaign entity while the second one will fetch a sim entity within
                        SelectGroundTarget(TARGET_ANYTHING);

                        if (groundTargetPtr == NULL)
                            return;

                        if (groundTargetPtr->BaseData()->IsCampaign() and not ((CampBaseClass *)groundTargetPtr->BaseData())->IsAggregate())
                            SelectGroundTarget(TARGET_ANYTHING);
                    }
                    else
                        return;
                }
                // set ground target to camp base if ground target is NULL at this point
                else if (groundTargetPtr == NULL)
                {
                    SetGroundTarget(campBaseTarg);

                    if (groundTargetPtr == NULL)
                        return;

                    // 2001-10-26 ADDED by M.N. If player changed mission type in TE planner, and below the target WP
                    // we find a package object, it will become a ground target. If a package is engaged, CTD.
                    if (groundTargetPtr->BaseData()->IsPackage())
                    {
                        SetGroundTarget(NULL);
                        gndTargetHistory[0] = NULL;
                        SelectGroundTarget(TARGET_ANYTHING); // choose something else
                    }

                    // END of added section

                    // Then get a sim entity from it
                    if (groundTargetPtr->BaseData()->IsCampaign() and not ((CampBaseClass *)groundTargetPtr->BaseData())->IsAggregate())
                        SelectGroundTarget(TARGET_ANYTHING);
                }
            }
        }
        // It's not from a waypoint action (could be a target of opportunity, even from A2A mission if it has A2G weapons as well)
        else  if (SimLibElapsedTime > nextAGTargetTime)
        {
            // Next retarget in 5 seconds
            nextAGTargetTime = SimLibElapsedTime + 5000;

            // First, lets release our current target and target history
            SetGroundTarget(NULL);
            gndTargetHistory[0] = NULL;

            // The first call should get a campaign entity while the second one will fetch a sim entity within
            SelectGroundTarget(TARGET_ANYTHING);

            if (groundTargetPtr == NULL)
                return;

            SelectGroundTarget(TARGET_ANYTHING);

            // Don't ask me, that's how they had it in the orininal code
            agImprovise = TRUE;
        }
        // Not the time to retarget, so get out
        else
            return;

    }
    // Don't ask me, that's how they had it in the orininal code
    else
    {
        agImprovise = TRUE;
    }

    // No ground target? do nothing
    if ( not groundTargetPtr)
        return;

    // After all this, make sure we have a sim target if we can
    if (groundTargetPtr->BaseData()->IsCampaign() and not ((CampBaseClass *)groundTargetPtr->BaseData())->IsAggregate())
        SelectGroundTarget(TARGET_ANYTHING);

    // do we have any ground weapons?
    /* SelectGroundWeapon();
     if ( not IsSetATC(HasAGWeapon))
     {
     // Nope, can't really attack can't we? so bail out
     SetGroundTarget(NULL);
     agDoctrine = AGD_NONE;
     return;
     }
    */
    // Better be safe than sory...
    if (groundTargetPtr == NULL)
    {
        // Nope, somehow we lost our target so bail out...
        agDoctrine = AGD_NONE;
        return;
    }

    // Tell the AI it hasn't done a ground pass yet so it can redo its attack profile
    madeAGPass = FALSE;

    // set doctrine and approach to default value, calc an insertion point loc
    agDoctrine = AGD_LOOK_SHOOT_LOOK;

    if (missionType == AMIS_SEADESCORT or missionType == AMIS_SEADSTRIKE)
    {
        // Cobra - SEAD escort or strike do not fly in low.  They want the radars to see them and turn on.
        agApproach = AGA_HIGH;
        // agApproach = AGA_LOW;
        //ipZ = 0.0f;
        //TJL 10/28/03 HARM Altitude
        ipZ = -self->GetA2GHarmAlt();
    }
    else
    {
        // Cobra - kind of a radical default profile for non-Harm missions
        //agApproach = AGA_DIVE;
        //ipZ = -self->GetA2GGunRocketAlt();
        agApproach = AGA_HIGH;
        ipZ = -self->GetA2GDumbLDAlt();
    }

    dx = groundTargetPtr->BaseData()->XPos() - self->XPos();
    dy = groundTargetPtr->BaseData()->YPos() - self->YPos();
    dz = groundTargetPtr->BaseData()->ZPos() - self->ZPos();

    // x-y get range
    range = (float)sqrt(dx * dx + dy * dy) + 0.1f;

    // normalize the x and y vector
    dx /= range;
    dy /= range;

    // see if we're too close in and set ipX/ipY accordingly
    if (range < 2.0f * NM_TO_FT)   // was 5.0
    {
        AGflyOut();
        SetGroundTarget(NULL);
        onStation = Downwind;
    }
    else
    {
        // get point between us and target
        ipX = groundTargetPtr->BaseData()->XPos(); // + dy * g_fAGFlyoutRange * NM_TO_FT;//previously 7.0 Cobra
        ipY = groundTargetPtr->BaseData()->YPos(); // - dx * g_fAGFlyoutRange * NM_TO_FT;
        ShiAssert(ipX > 0.0F);
    }

    // Depending on the type of plane, adjust our attack profile
    classPtr = (Falcon4EntityClassType*)self->EntityType();

    if (classPtr->vuClassData.classInfo_[VU_STYPE] == STYPE_AIR_BOMBER)
    {
        agApproach = AGA_BOMBER;
        agDoctrine = AGD_SHOOT_RUN;
        ipZ = self->ZPos();
    }
    else
    {
        // Cobra - Check for AG weapons to make sure we have the correct info
        SelectGroundWeapon();

        // Order of fire is: HARMs, AGMissiles, GBUs, bombs, rockets then gun so do it similarely
        //TJL 10/27/03 Porting out all attack altitudes to spconfig variables
        if (hasHARM)
        {
            // Cobra - SEAD escort or strike do not fly in low.  They want the radars to see them and turn on.
            agApproach = AGA_HIGH;
            // agApproach = AGA_LOW;
            agDoctrine = AGD_LOOK_SHOOT_LOOK;
            ipX = self->XPos();
            ipY = self->YPos();
            ipZ = -self->GetA2GHarmAlt();
            //ipZ = 0.0f;
        }
        else if (hasAGMissile)
        {
            agApproach = AGA_HIGH;
            agDoctrine = AGD_LOOK_SHOOT_LOOK;

            // Wings shoots Mavericks as soon as asked.
            ipX = self->XPos();
            ipY = self->YPos();
            ipZ = -self->GetA2GAGMAlt();
            //ipZ = -4000.0f;
        }
        else if (hasGBU)
        {
            agApproach = AGA_HIGH;
            agDoctrine = AGD_SHOOT_RUN;
            ipZ = -self->GetA2GGBUAlt();
            //ipZ = -13000.0f;
        }
        else if (hasBomb)
        {
            if (hasBomb == TRUE + 1)  // It's a durandal
            {
                agApproach = AGA_HIGH; // Because if 'low', he will 'pop up' on final...
                ipZ = -self->GetA2GGenericBombAlt();
                //ipZ = -250.0f;
            }
            else if (hasBomb == TRUE + 2)  // It's a cluster bomb
            {
                agApproach = AGA_HIGH;
                ipZ = -self->GetA2GClusterAlt();
                //ipZ = -5000.0f;
            }
            else if (hasBomb == TRUE + 3)  // It's a JDAM bomb
            {
                agApproach = AGA_HIGH;
                ipZ = -self->GetA2GJDAMAlt();
                //ipZ = -5000.0f;
            }
            else if (hasBomb == TRUE + 4)  // It's a JSOW bomb
            {
                ipX = self->XPos();
                ipY = self->YPos();
                agApproach = AGA_HIGH;
                ipZ = -self->GetA2GJSOWAlt();
                //ipZ = -5000.0f;
            }
            else
            {
                // It's any other type of bombs
                agApproach = AGA_HIGH;
                ipZ = -self->GetA2GDumbLDAlt();
                //ipZ = -11000.0f;
            }

            // Now check if we are within the plane's min/max altitude and snap if not...
            VehicleClassDataType *vc = GetVehicleClassData(self->Type() - VU_LAST_ENTITY_TYPE);
            ipZ = max(vc->HighAlt * -100.0f, ipZ);
            ipZ = min(vc->LowAlt * -100.0f, ipZ);

            agDoctrine = AGD_SHOOT_RUN;
        }
        else if (hasGun or hasRocket)
        {
            agDoctrine = AGD_LOOK_SHOOT_LOOK;
            agApproach = AGA_DIVE;
            ipZ = -self->GetA2GGunRocketAlt();
            //ipZ = -7000.0f;
        }

        // For these, it's always a LOOK SHOOT LOOK
        if (missionType == AMIS_SAD or missionType == AMIS_INT or missionType == AMIS_BAI or hasAGMissile or hasHARM or IsNotMainTargetSEAD())
            agDoctrine = AGD_LOOK_SHOOT_LOOK;

        // Just in case it was changed by a weapon type earlier
        if (missionType == AMIS_SEADESCORT or missionType == AMIS_SEADSTRIKE)
        {
            agApproach = AGA_HIGH;
            //ipZ = 0.0f;
            //TJL 10/28/03 Harm Alt
            ipZ = -self->GetA2GHarmAlt();
        }

        // Then if we have a camera,
        if ((missionType == AMIS_BDA or missionType == AMIS_RECON) and hasCamera)
        {
            ipZ = -self->GetA2GCameraAlt();
            //ipZ = -7000.0f;
            agDoctrine = AGD_SHOOT_RUN;
            agApproach = AGA_DIVE;
        }
    }

    // Cobra - Save current weapon attack altitude
    AGattackAlt = ipZ;
}

void DigitalBrain::IPCheck(void)
{
    WayPointClass* tmpWaypoint = self->waypoint;
    SMSClass* Sms = self->Sms;
    float wpX, wpY, wpZ;
    //TJL 10/20/03 Added back in dZ
    float dX, dY, dZ;
    float rangeSq, tgtrangeSq;
    short edata[6];
    int response;

    // Only for the player's wingmen
    // JB 020315 All aircraft will now all check to see if they have passed the IP.
    // Only checking for the IP if the lead is a player is silly.
    // Things depend on the ReachedIP flag being set properly.
    //if (flightLead and flightLead->IsSetFlag(MOTION_OWNSHIP))
    {
        // Periodically check for IP and if so, ask for permission to engage
        //if ( not IsSetATC(ReachedIP))
        //TJL 10/20/03 Put this back to check FL lead
        if ( not IsSetATC(ReachedIP) and flightLead->IsSetFlag(MOTION_OWNSHIP))
        {
            // Find the IP waypoint
            while (tmpWaypoint)
            {
                if (tmpWaypoint->GetWPFlags() bitand WPF_TARGET)
                    break;

                tmpWaypoint = tmpWaypoint->GetNextWP();
            }

            // From the target, find the IP
            if (tmpWaypoint)
            {
                tmpWaypoint->GetLocation(&wpX, &wpY, &wpZ);
                dX = self->XPos() - wpX;
                dY = self->YPos() - wpY;
                dZ = self->ZPos() - wpZ;
                tgtrangeSq = dX * dX + dY * dY + dZ * dZ;
                tmpWaypoint = tmpWaypoint->GetPrevWP();
            }

            // Have an IP
            // TJL 10/20/03 Added back Zpos
            if (tmpWaypoint)
            {
                tmpWaypoint->GetLocation(&wpX, &wpY, &wpZ);
                dX = self->XPos() - wpX;
                dY = self->YPos() - wpY;
                dZ = self->ZPos() - wpZ;

                // Original code was looking for the SLANT distance, I'm not...
                // Check for within 5 NM of IP
                // TJL 10/20/03 Added back Z slant and change 5.0F to 10.0F
                rangeSq = dX * dX + dY * dY + dZ * dZ;

                //Cobra TJL Attempt to let SEAD flights target earlier
                if (missionType == AMIS_SEADESCORT or missionType == AMIS_SEADSTRIKE)
                {
                    if ((FabsF(rangeSq) < (25.0F * NM_TO_FT) * (25.0F * NM_TO_FT)) and 
                        (FabsF(tgtrangeSq) < (g_fAIHarmMaxRange * NM_TO_FT) * (g_fAIHarmMaxRange * NM_TO_FT)))
                    {
                        SetATCFlag(ReachedIP);

                        // JB 020315 Only set the WaitForTarget flag if the lead is a player.
                        if (flightLead and flightLead->IsSetFlag(MOTION_OWNSHIP))
                            SetATCFlag(WaitForTarget);
                    }

                }
                //Cobra - Attempt to let JSOW flights target earlier
                //else if (((BombClass *)Sms->hardPoint[Sms->CurHardpoint()]->weaponPointer) and 
                // (((BombClass *)Sms->hardPoint[Sms->CurHardpoint()]->weaponPointer)->IsSetBombFlag(BombClass::IsJSOW)) or
                else if (hasBomb == TRUE + 4)
                {
                    if ((FabsF(rangeSq) < FabsF((25.0F * NM_TO_FT) * (25.0F * NM_TO_FT))) and 
                        (FabsF(tgtrangeSq) < (g_fAIJSOWMaxRange * NM_TO_FT) * (g_fAIJSOWMaxRange * NM_TO_FT)))
                    {
                        SetATCFlag(ReachedIP);

                        // JB 020315 Only set the WaitForTarget flag if the lead is a player.
                        if (flightLead and flightLead->IsSetFlag(MOTION_OWNSHIP))
                            SetATCFlag(WaitForTarget);
                    }
                }
                else if (rangeSq < (10.0F * NM_TO_FT) * (10.0F * NM_TO_FT))
                {
                    // The range is INCREASING so we assume (may be wronly if we turned away from the IP waypoint) we've reached it, say so and wait for a target
                    SetATCFlag(ReachedIP);

                    // JB 020315 Only set the WaitForTarget flag if the lead is a player.
                    if (flightLead and flightLead->IsSetFlag(MOTION_OWNSHIP))
                        SetATCFlag(WaitForTarget);
                }
            }
        }
        // We have reached our IP waypoint, are we waiting for a target?
        else if (IsSetATC(WaitForTarget))
        {
            // Yes, ask for permission if we are an incomplete AGMission that doesn't already have a designated target and it holds a ground target
            if (missionClass == AGMission and not missionComplete and not mpActionFlags[AI_ENGAGE_TARGET] and groundTargetPtr)
            {
                // Flag we are waiting for permission and we have a ground target to shoot at
                SetATCFlag(WaitingPermission);
                ClearATCFlag(WaitForTarget);

                // Ask for permission to engage
                SetATCFlag(AskedToEngage);
                edata[0] = ((FlightClass*)self->GetCampaignObject())->callsign_id;
                edata[1] = (((FlightClass*)self->GetCampaignObject())->callsign_num - 1) * 4 + isWing + 1;
                response = rcREQUESTTOENGAGE;
                AiMakeRadioResponse(self, response, edata);
            }
        }
    }
}

void DigitalBrain::TakePicture(float approxRange, float ata)
{
    FireControlComputer* FCC = self->FCC;

    // Go to camera mode
    if (FCC->GetMasterMode() not_eq FireControlComputer::AirGroundCamera)
    {
        //Cobra test
        self->FCC->SetMasterMode(FireControlComputer::AirGroundCamera);
    }

    if (groundTargetPtr == NULL)
    {
        onStation = Final1;
    }
    else if (approxRange < 3.0f * NM_TO_FT and ata < 10.0f * DTR)
    {
        // take picture
        waitingForShot = SimLibElapsedTime;
        onStation = Final1;
        SetFlag(MslFireFlag);
    }
}

void DigitalBrain::DropBomb(float approxRange, float ata, RadarClass* theRadar)
{
    FireControlComputer* FCC = self->FCC;
    SMSClass* Sms = self->Sms;
    int wt;

    F4Assert( not Sms->curWeapon or Sms->curWeapon->IsBomb());

    // COBRA - RED - FIXING POSSIBLE CTDs
    BombClass *TheBomb = FCC->GetTheBomb();

    if (TheBomb and TheBomb->GetWeaponId() /*Sms->CurHardpoint() and Sms->hardPoint*/)
        //wt = (WeaponDataTable[Sms->hardPoint[Sms->CurHardpoint()]->weaponId].Weight);
        wt = (WeaponDataTable[TheBomb->GetWeaponId()].Weight);


    // Check for JDAM/JSOW ready-to-drop
    /* if ((self->IsPlayer() and self->AutopilotType() not_eq AircraftClass::CombatAP) and (Sms->GetCurrentHardpoint() > 0) and 
     ((Sms->hardPoint[Sms->GetCurrentHardpoint()]->GetWeaponType()==wtGPS) or
     (((BombClass *)Sms->hardPoint[Sms->CurHardpoint()]->weaponPointer) and 
     ((BombClass *)Sms->hardPoint[Sms->CurHardpoint()]->weaponPointer)->IsSetBombFlag(BombClass::IsJSOW))) and 
     ( not Sms->JDAMPowered))
     return;*/

    if ((self->IsPlayer() and self->AutopilotType() not_eq AircraftClass::CombatAP) and TheBomb and 
        (TheBomb->IsSetBombFlag(BombClass::IsGPS) or TheBomb->IsSetBombFlag(BombClass::IsJSOW)) and ( not Sms->JDAMPowered))
        return;


    // Make sure the FCC is in the right mode/sub mode
    if (FCC->GetMasterMode() not_eq FireControlComputer::AirGroundBomb or
        FCC->GetSubMode() not_eq FireControlComputer::CCRP)
    {
        //Cobra test
        self->FCC->SetMasterMode(FireControlComputer::AirGroundBomb);
        self->FCC->SetSubMode(FireControlComputer::CCRP);
    }


    if ( not Sms->curWeapon or not Sms->curWeapon->IsBomb())
    {
        if (Sms->FindWeaponClass(wcBombWpn))
        {
            Sms->SetWeaponType(Sms->hardPoint[Sms->CurHardpoint()]->GetWeaponType());
            // COBRA - RED - FIXING POSSIBLE CTDs
            TheBomb = FCC->GetTheBomb();
        }
        else
        {
            Sms->SetWeaponType(wtNone);
            TheBomb = NULL;
        }
    }

    // Cobra - Set JSOW to MANual submode
    if (TheBomb and TheBomb->IsSetBombFlag(BombClass::IsJSOW)) self->FCC->SetSubMode(FireControlComputer::MAN);

    // Point the radar at the target
    if (theRadar)
    {
        if (groundTargetPtr and groundTargetPtr->BaseData()->IsMover())
            theRadar->SetMode(RadarClass::GMT);
        else
            theRadar->SetMode(RadarClass::GM);

        theRadar->SetDesiredTarget(groundTargetPtr);
        theRadar->SetAGSnowPlow(TRUE);
    }

    // Mode the SMS
    //Sms->SetPair(TRUE); // MLR 4/3/2004 -

    // Give the FCC permission to release if in parameters
    SetFlag(MslFireFlag);

    // Adjust for wind/etc
    if (fabs(FCC->airGroundBearing) < 5.0F * DTR)
    {
        SetTrackPoint(
            2.0F * FCC->groundDesignateX - FCC->groundImpactX,
            2.0F * FCC->groundDesignateY - FCC->groundImpactY
        );
    }

    if (agApproach == AGA_BOMBER)
    {
        if ( not droppingBombs)
        {
            // Try to put the middle drop on target
            //Sms->SetRippleCount (int(Sms->NumCurrentWpn() / 2.0F + 0.5F) - 1);
            Sms->SetAGBRippleCount(int(Sms->NumCurrentWpn() / 2.0F + 0.5F) - 1);

            //int rcount = Sms->RippleCount() + 1;
            int rcount = Sms->GetAGBRippleCount() + 1;

            if ( not (rcount bitand 1)) // If not odd
                rcount--;

            //if (FCC->airGroundRange < Sms->NumCurrentWpn() * 2.0F * Sms->RippleInterval())
            //if (FCC->airGroundRange < (rcount * Sms->RippleInterval()) / 2) // JB 010408 010708 Space the ripples correctly over the target
            if (FCC->airGroundRange < (rcount * Sms->GetAGBRippleInterval()) / 2) // JB 010408 010708 Space the ripples correctly over the target
            {
                droppingBombs = wcBombWpn;
                FCC->SetBombReleaseOverride(TRUE);
                onStation = Final1;
            }
        }
        else
        {

            if (Sms->NumCurrentWpn() == 0)
            {
                FCC->SetBombReleaseOverride(FALSE);
                // Out of this weapon, find another/get out of dodge
                agDoctrine = AGD_LOOK_SHOOT_LOOK;
                hasRocket = FALSE;
                hasGun = FALSE;
                hasBomb = FALSE;
                hasGBU = FALSE;

                // Start over again
                madeAGPass = TRUE;
                onStation = NotThereYet;
                return;
            }
            // too close ?
            // we're within a certain range and our ATA is not good
            else if (approxRange < 1.2f * NM_TO_FT and ata > 75.0f * DTR)
            {
                waitingForShot = SimLibElapsedTime + 5000;
                AGflyOut();
                onStation = Downwind;
                //onStation = Final1;
                return;
            }
        }

    }
    else
    {
        // JB 010708 start Drop all your dumb bombs of the current type or if
        // the lead is a player (not in autopilot) use the lead's ripple setting.
        if ( not droppingBombs)
        {
            //if (flightLead and ((AircraftClass*)flightLead)->AutopilotType() not_eq AircraftClass::CombatAP and ((AircraftClass*)flightLead)->Sms)
            // FRB
            if (flightLead and flightLead == self and self->IsPlayer() and self->AutopilotType() not_eq AircraftClass::CombatAP and ((AircraftClass*)flightLead)->Sms)
            {
                Sms->SetAGBRippleCount(min(((AircraftClass*)flightLead)->Sms->GetAGBRippleCount(), int(Sms->NumCurrentWpn() / 2.0F + 0.5F) - 1));

                // Cobra - set AI CBU Burst Height to the human leader's
                if (hasBomb == TRUE + 2)
                    Sms->SetAGBBurstAlt((((AircraftClass*)flightLead)->Sms->GetAGBBurstAlt()));
            }
            else
            {
                // COBRA - RED - FIXING POSSIBLE CTDs
                // Cobra - rippling pair only for GPS JDAM/JSOW
                /* if ((Sms->GetCurrentHardpoint() > 0) and ((Sms->hardPoint[Sms->GetCurrentHardpoint()]->GetWeaponType()==wtGPS) or
                 (((BombClass *)Sms->hardPoint[Sms->CurHardpoint()]->weaponPointer) and 
                 ((BombClass *)Sms->hardPoint[Sms->CurHardpoint()]->weaponPointer)->IsSetBombFlag(BombClass::IsJSOW))))*/
                if (TheBomb and (TheBomb->IsSetBombFlag(BombClass::IsJSOW) or TheBomb->IsSetBombFlag(BombClass::IsGPS)))
                    Sms->SetAGBRippleCount(2);
                else if (wt >= 1900) // 2000 lb'ers one at a time
                    Sms->SetAGBRippleCount(1);
                else
                    Sms->SetAGBRippleCount(int(Sms->NumCurrentWpn() / 2.0F + 0.5F) - 1);
            }

            int rcount = Sms->GetAGBRippleCount() + 1;

            if ( not (rcount bitand 1)) // If not odd
                rcount--;

            if (Sms->GetAGBRippleCount() > 0 and FCC->airGroundRange < (rcount * Sms->GetAGBRippleInterval()) / 2)
            {
                droppingBombs = wcBombWpn;
                FCC->SetBombReleaseOverride(TRUE);
                onStation = Final1;
            }

            // 2001-10-24 ADDED BY M.N. Planes can start to circle around their target if we don't do
            // a range bitand ata check to the target here.

            if (approxRange < 1.2f * NM_TO_FT and ata > 75.0f * DTR)
            {
                // Bail and try again
                AGflyOut();
                onStation = Downwind;

                //dx = ( self->XPos() - trackX );
                //dy = ( self->YPos() - trackY );
                //approxRange = (float)sqrt( dx * dx + dy * dy );
                //dx /= approxRange;
                //dy /= approxRange;
                //ipX = trackX + dy * 5.5f * NM_TO_FT;
                //ipY = trackY - dx * 5.5f * NM_TO_FT;
                //ShiAssert (ipX > 0.0F);

                //// Try bombing run again
                //onStation = Crosswind;
            }

            // End of added section
        }
        // JB 010708 end
        else
        {
            // if ( FCC->postDrop)
            if (FCC->postDrop and Sms->CurRippleCount() == 0)  // JB 010708
            {
                FCC->SetBombReleaseOverride(FALSE); // JB 010708
                droppingBombs = wcBombWpn;

                // Out of this weapon, find another/get out of dodge
                agDoctrine = AGD_LOOK_SHOOT_LOOK;
                hasRocket = FALSE;
                hasGun = FALSE;
                hasBomb = FALSE;
                hasGBU = FALSE;

                // Start over again
                //madeAGPass = TRUE;
                //onStation = NotThereYet;
                madeAGPass = TRUE;
                AGflyOut();
                // Release current target and target history
                SetGroundTarget(NULL);
                gndTargetHistory[0] = NULL;
                onStation = Downwind;
            }

            // too close ?
            // we're within a certain range and our ATA is not good
            if (approxRange < 1.2f * NM_TO_FT and ata > 75.0f * DTR)
            {
                waitingForShot = SimLibElapsedTime + 5000;
                // Bail and try again
                AGflyOut();
                // Release current target and target history
                //SetGroundTarget( NULL );
                //gndTargetHistory[0] = NULL;
                onStation = Downwind;
                //onStation = Final1;
            }
        }
    }
}

void DigitalBrain::DropGBU(float approxRange, float ata, RadarClass* theRadar)
{
    LaserPodClass* targetingPod = NULL;
    FireControlComputer* FCC = self->FCC;
    SMSClass* Sms = self->Sms;
    float dir, dx, dy, angle;
    mlTrig trig;

    F4Assert( not Sms->curWeapon or Sms->curWeapon->IsBomb());

    // Don't stop in the middle
    droppingBombs = wcGbuWpn;

    // Make sure the FCC is in the right mode/sub mode
    if (FCC->GetMasterMode() not_eq FireControlComputer::AirGroundLaser or
        FCC->GetSubMode() not_eq FireControlComputer::SLAVE)
    {
        //Cobra test
        self->FCC->SetMasterMode(FireControlComputer::AirGroundLaser);
        self->FCC->SetSubMode(FireControlComputer::SLAVE);
    }

    if ( not Sms->curWeapon or not Sms->curWeapon->IsBomb())
    {
        if (Sms->FindWeaponClass(wcGbuWpn, FALSE))
        {
            Sms->SetWeaponType(Sms->hardPoint[Sms->CurHardpoint()]->GetWeaponType());
        }
        else
        {
            Sms->SetWeaponType(wtNone);
        }
    }


    // Get the targeting pod locked on to the target
    targetingPod = (LaserPodClass*) FindLaserPod(self);

    if (targetingPod and targetingPod->CurrentTarget())
    {
        if ( not targetingPod->IsLocked())
        {
            // Designate needs to go down then up then down to make it work
            if (FCC->designateCmd)
                FCC->designateCmd = FALSE;
            else
                FCC->designateCmd = TRUE;
        }
        else
        {
            FCC->designateCmd = FALSE;
        }

        FCC->preDesignate = FALSE;
    }

    // Point the radar at the target
    if (theRadar)
    {
        if (groundTargetPtr and groundTargetPtr->BaseData()->IsMover())
            theRadar->SetMode(RadarClass::GMT);
        else
            theRadar->SetMode(RadarClass::GM);

        theRadar->SetDesiredTarget(groundTargetPtr);
        theRadar->SetAGSnowPlow(TRUE);
    }

    // Mode the SMS
    //Sms->SetPair(FALSE);
    Sms->SetAGBPair(FALSE);
    //Sms->SetRippleCount(0);
    Sms->SetAGBRippleCount(0);

    // Adjust for wind/etc
    if (fabs(FCC->airGroundBearing) < 10.0F * DTR)
    {
        SetTrackPoint(
            2.0F * FCC->groundDesignateX - FCC->groundImpactX,
            2.0F * FCC->groundDesignateY - FCC->groundImpactY
        );
    }

    // Give the FCC permission to release if in parameters
    if (onStation == Final)
    {
        if (SimLibElapsedTime > waitingForShot)
        {
            // 2001-08-31 REMOVED BY S.G. NOT USED ANYWAY AND I NEED THE FLAG FOR SOMETHING ELSE
            // if (approxRange < 1.2F * FCC->airGroundRange)
            // SetATCFlag(InhibitDefensive);

            // Check for too close
            if (approxRange < 0.5F * FCC->airGroundRange)
            {
                // Bail and try again
                dx = (self->XPos() - trackX);
                dy = (self->YPos() - trackY);
                approxRange = (float)sqrt(dx * dx + dy * dy);
                dir = 1.0f;

                if (rand() % 2)
                    dir *= -1.0f;

                // Cobra - Slow-movers don't need to flyout too far
                if (slowMover)
                {
                    if (dy < 0.0f)
                        ipX = trackX - dir * g_fAGSlowFlyoutRange * NM_TO_FT; // Cobra - added g_fAGFlyoutRange static range
                    else
                        ipX = trackX - dir * g_fAGSlowFlyoutRange * NM_TO_FT; // Cobra - added g_fAGFlyoutRange static range

                    if (dx < 0.0f)
                        ipY = trackY - dir * g_fAGSlowFlyoutRange * NM_TO_FT; // Cobra - added g_fAGFlyoutRange static range
                    else
                        ipY = trackY + dir * g_fAGSlowFlyoutRange * NM_TO_FT;
                }
                else
                {
                    if (dy < 0.0f)
                        ipX = trackX - dir * g_fAGFlyoutRange * NM_TO_FT; // Cobra - added g_fAGFlyoutRange static range
                    else
                        ipX = trackX + dir * g_fAGFlyoutRange * NM_TO_FT;

                    if (dx < 0.0f)
                        ipY = trackY - dir * g_fAGFlyoutRange * NM_TO_FT; // Cobra - added g_fAGFlyoutRange static range
                    else
                        ipY = trackY + dir * g_fAGFlyoutRange * NM_TO_FT;
                }

                ipZ = trackZ = self->ZPos();
                ShiAssert(ipX > 0.0F);

                // Start again
                onStation = Crosswind;
                //MonoPrint ("Too close to GBU, head to IP and try again\n");
            }

            SetFlag(MslFireFlag);

            if (FCC->postDrop)
            {
                // 1/2 second between bombs, 10 seconds after last bomb - Cobra - 1/4 second to get the pair closer to target
                if (Sms->NumCurrentWpn() % 2 not_eq 0)
                {
                    waitingForShot = SimLibElapsedTime + (SEC_TO_MSEC / 4);
                }
                else
                {
                    // Keep Lasing
                    madeAGPass = TRUE;
                    onStation = Final1;
                    waitingForShot = SimLibElapsedTime;
                }
            }
        }
        else // Are we too close?
        {
            if (onStation == Final and approxRange < 0.7F * FCC->airGroundRange or ata > 60.0F * DTR)
            {
                // Bail and try again
                dx = (self->XPos() - trackX);
                dy = (self->YPos() - trackY);
                approxRange = (float)sqrt(dx * dx + dy * dy);
                dir = 1.0f;

                if (rand() % 2)
                    dir *= -1.0f;

                // Cobra - Slow-movers don't need to flyout too far
                if (slowMover)
                {
                    if (dy < 0.0f)
                        ipX = trackX - dir * g_fAGSlowFlyoutRange * NM_TO_FT; // Cobra - added g_fAGFlyoutRange static range
                    else
                        ipX = trackX - dir * g_fAGSlowFlyoutRange * NM_TO_FT; // Cobra - added g_fAGFlyoutRange static range

                    if (dx < 0.0f)
                        ipY = trackY - dir * g_fAGSlowFlyoutRange * NM_TO_FT; // Cobra - added g_fAGFlyoutRange static range
                    else
                        ipY = trackY + dir * g_fAGSlowFlyoutRange * NM_TO_FT;
                }
                else
                {
                    if (dy < 0.0f)
                        ipX = trackX - dir * g_fAGFlyoutRange * NM_TO_FT; // Cobra - added g_fAGFlyoutRange static range
                    else
                        ipX = trackX + dir * g_fAGFlyoutRange * NM_TO_FT;

                    if (dx < 0.0f)
                        ipY = trackY - dir * g_fAGFlyoutRange * NM_TO_FT; // Cobra - added g_fAGFlyoutRange static range
                    else
                        ipY = trackY + dir * g_fAGFlyoutRange * NM_TO_FT;
                }

                ipZ = trackZ = self->ZPos();
                ShiAssert(ipX > 0.0F);

                // Start again
                onStation = Crosswind;
                //MonoPrint ("Too close to bomb, head to IP and try again\n");
            }
        }
    }

    // Out of this weapon, find another/get out of dodge
    if (onStation == Final1)
    {
        if (SimLibElapsedTime > waitingForShot) // Bomb has had time to fall.
        {
            agDoctrine = AGD_LOOK_SHOOT_LOOK;
            hasRocket = FALSE;
            hasGun = FALSE;
            hasBomb = FALSE;
            hasGBU = FALSE;
            droppingBombs = FALSE;

            // Force a weapon/target selection
            madeAGPass = TRUE;
            onStation = NotThereYet;
            moreFlags and_eq compl KeepLasing; // 2002-03-08 ADDED BY S.G. Not lasing anymore
        }
        else if (SimLibElapsedTime == waitingForShot) // Turn but keep designating
        {
            dx = (trackX - self->XPos());
            dy = (trackY - self->YPos());
            angle = 45.0F * DTR + (float)atan2(dy, dx);
            mlSinCos(&trig, angle);
            ipX = trackX + trig.cos * 7.5f * NM_TO_FT;
            ipY = trackY + trig.sin * 7.5f * NM_TO_FT;
            SetTrackPoint(ipX, ipY);
            ipZ = self->ZPos();
            waitingForShot = SimLibElapsedTime + 27 * SEC_TO_MSEC;
            ShiAssert(ipX > 0.0F);
            moreFlags or_eq KeepLasing; // 2002-03-08 ADDED BY S.G. Flag this AI as lasing so he sticks to it
        }
        else
        {
            SetTrackPoint(ipX, ipY, ipZ);
        }
    }

    //MI NULL out our Target
    if (groundTargetPtr and 
        groundTargetPtr->BaseData()->IsSim() and 
        (groundTargetPtr->BaseData()->IsDead() or
         groundTargetPtr->BaseData()->IsExploding()))
    {
        SetGroundTarget(NULL);
    }

}

void DigitalBrain::FireAGMissile(float approxRange, float ata)
{
    SMSClass* Sms = self->Sms;

    //F4Assert ( not Sms->curWeapon or Sms->curWeapon->IsMissile());

    // Check Timer
    // 2001-07-12 MODIFIED BY S.G. DON'T LAUNCH UNTIL CLOSE TO OUR ATTACK ALTITUDE
    // if ( SimLibElapsedTime >= waitingForShot )
    if (SimLibElapsedTime >= waitingForShot and self->ZPos() - trackZ >= -500.0f)
    {
        SetFlag(MslFireFlag);

        if (hasHARM)
        {
            if (approxRange * FT_TO_NM > g_fAIHarmMaxRange) // Cobra - was 20.0f nm
            {
                waitingForShot = SimLibElapsedTime + 180000;
            }
            else
            {
                waitingForShot = SimLibElapsedTime + 60000;
            }
        }
        else if (hasBomb == TRUE + 4) // Cobra - JSOW
        {
            if (approxRange * FT_TO_NM > g_fAIJSOWMaxRange) // Cobra - was 20.0f nm
            {
                waitingForShot = SimLibElapsedTime + 60000;
            }
            else
            {
                waitingForShot = SimLibElapsedTime + 2000;
            }
        }
        else//AGMs
        {
            waitingForShot = SimLibElapsedTime + 5000;
        }

        // if we're out of missiles and bombs and our
        // doctrine is look shoot look, we don't want to
        // continue with guns/rockets only -- reset
        // agDoctrine if this is the case
        // 2001-05-03 MODIFIED BY S.G. WE STOP FIRING WHEN WE HAVE AN ODD NUMBER OF MISSILE LEFT (MEANT WE FIRED ONE ALREADY) THIS WILL LIMIT IT TO 2 MISSILES PER TARGET
        // if (Sms->NumCurrentWpn() == 1 )

        if ((hasBomb not_eq TRUE + 4) and Sms->NumCurrentWpn() bitand 1)
        {
            hasRocket = FALSE;
            hasGun = FALSE;
            hasHARM = FALSE;
            hasAGMissile = FALSE;

            // Force a weapon/target selection
            madeAGPass = TRUE;
            onStation = NotThereYet;

            // 2001-06-01 ADDED BY S.G. THAT WAY, AI WILL KEEP GOING STRAIGHT FOR A SECOND BEFORE PULLING
            missileShotTimer = SimLibElapsedTime + 1000;

            // END OF ADDED SECTION
            // 2001-06-16 ADDED BY S.G. THAT WAY, AI WILL NOT RETURN TO THEIR IP
            // ONCE THEY FIRED BUT WILL GO "PERPENDICULAR FOR 2 NM"...
            // 2001-07-16 MODIFIED BY S.G. DEPENDING IF WE HAVE WEAPONS LEFT, PULL MORE OR LESS
            if (groundTargetPtr)
            {
                float dx = groundTargetPtr->BaseData()->XPos() - self->XPos();
                float dy = groundTargetPtr->BaseData()->YPos() - self->YPos();

                // x-y get range
                float range = (float)sqrt(dx * dx + dy * dy) + 0.1f;

                // normalize the x and y vector
                dx /= range;
                dy /= range;

                if (Sms->NumCurrentWpn() == 1)
                {
                    ipX = self->XPos() + dy * 5.0f * NM_TO_FT - dx * 1.5f * NM_TO_FT;
                    ipY = self->YPos() - dx * 5.0f * NM_TO_FT - dy * 1.5f * NM_TO_FT;
                }
                else
                {
                    ipX = self->XPos() + dy * 3.0f * NM_TO_FT - dx * 1.5f * NM_TO_FT;
                    ipY = self->YPos() - dx * 3.0f * NM_TO_FT - dy * 1.5f * NM_TO_FT;
                }
            }

            // END OF ADDED SECTION
        }

        // determine if we shoot and run or not
        //waitingForShot = SimLibElapsedTime + 5000; //Cobra
    }
    // too close or already fired, try again
    // we're within a certain range and our ATA is not good
    else if (approxRange < 1.2f * NM_TO_FT and ata > 75.0f * DTR)
    {
        waitingForShot = SimLibElapsedTime + 5000;
        onStation = Final1;
    }
}

void MoveStick(float &stick, float desired, float dps)
{
    if (stick > desired)
    {
        stick -= dps * SimLibMajorFrameTime;

        if (stick < desired) stick = desired;

    }
    else
    {
        if (stick < desired)
        {
            stick += dps * SimLibMajorFrameTime;

            if (stick > desired) stick = desired;
        }
    }
}

int DigitalBrain::FireRocket(float approxRange, float ata)
{
    // Cobra test
    static FILE *fp = NULL;
    //if (fp == NULL)
    //fp = fopen("G:\\RocketFCC.txt", "w");


    FireControlComputer* FCC = self->FCC;
    SMSClass* Sms = self->Sms;

    if (FCC->GetMasterMode() not_eq FireControlComputer::AirGroundRocket) // MLR 4/3/2004 -
    {
        //self->Sms->FindWeaponClass(wcRocketWpn);
        //FCC->SetAGMasterModeForCurrentWeapon();
        //Cobra test
        self->FCC->SetMasterMode(FireControlComputer::AirGroundRocket);
    }

    // put pipper on target
    if (groundTargetPtr)// and 
        //ata < 45 * DTR)
    {
        static float azRough = 10 * DTR;
        int pmode = 0;
        int rmode = 0;

        float dx = groundTargetPtr->BaseData()->XPos() - self->FCC->groundImpactX;
        float dy = groundTargetPtr->BaseData()->YPos() - self->FCC->groundImpactY;
        float dz = groundTargetPtr->BaseData()->ZPos() - self->FCC->groundImpactZ;
        float deltaEl = (float)atan2(-dz, sqrt(dx * dx + dy * dy));

        float taz, tel; // target
        float tx = groundTargetPtr->BaseData()->XPos();
        float ty = groundTargetPtr->BaseData()->YPos();
        float tz = groundTargetPtr->BaseData()->ZPos();

        TargetAzEl(self, tx, ty, tz, taz, tel);

        float tgtAltDelta = self->ZPos() - groundTargetPtr->BaseData()->ZPos();
        float selfAlt   = self->ZPos() - OTWDriver.GetGroundLevel(self->XPos(), self->YPos());


        float iaz, iel; // predicted impact
        float ix = self->FCC->groundImpactX;
        float iy = self->FCC->groundImpactY;
        float iz = OTWDriver.GetGroundLevel(ix, iy);   // returned iz is zero when tz is much higher

        TargetAzEl(self, ix, iy, iz, iaz, iel);

        float dxx = tx - self->XPos();
        float dyy = ty - self->YPos();
        float dtaz = (float)atan2(dyy, dxx) * RTD;
        dtaz -= (self->Yaw() * RTD);

        dxx = ix - self->XPos();
        dyy = iy - self->YPos();
        float diaz = (float)atan2(dyy, dxx) * RTD;

        diaz = 0.0f; // Cobra - missile projected impact should always be off the nose

        float atel;
        atel = tel / 2.0f;
        // Pitch correction
        atel -= g_fRocketPitchFactor * DTR;

        float diffel = FabsF(deltaEl);
        float diffaz = FabsF(dtaz - diaz) * DTR;
        float angle = SqrtF(diffel * diffel + diffaz * diffaz);

        if (selfAlt > -400)      // REALLY avoid ground
        {
            if (fp and self->IsPlayer())
                fprintf(fp, "Way Too Low\n");

            MoveStick(pStick, 45 * DTR, 45 * DTR);
            MoveStick(rStick, 0,        20 * DTR);
            return 1;
        }

        if (tgtAltDelta > -1500   or  // atleast 500" above target
            selfAlt     > -1500)      // avoid ground
        {
            if (fp and self->IsPlayer())
                fprintf(fp, "Too Low\n");

            MoveStick(pStick, 45 * DTR, 20 * DTR);
            MoveStick(rStick, 0,        20 * DTR);
            return 1;
        }

        dx = (float)fabs(self->XPos() - tx);
        dy = (float)fabs(self->YPos() - ty);
        approxRange = (float)sqrt(dx * dx + dy * dy);

        switch (rocketMnvr)
        {
            case RocketFiring:
                if (fp and self->IsPlayer())
                    fprintf(fp, "RocketFiring\n");

                rocketTimer -= SimLibMajorFrameTime;

                // wait for the SMS to finish the salvo, then Jink
                if ( not self->Sms->IsFiringRockets() and (rocketTimer < 0))
                {
                    rocketMnvr = RocketJink;
                    rocketTimer = 10; // was 5 sec
                }
                else
                    return 1; // Cobra - let's finish firing this salvo.

                break;

            case RocketJink:
                if (fp and self->IsPlayer())
                    fprintf(fp, "RocketJink\n");

                //if(SimLibElapsedTime bitand 4096)
                //MoveStick(rStick, 45 * DTR , 45 * DTR); // Cobra - was 90
                //else
                //MoveStick(rStick, -45 * DTR , 45 * DTR); // Cobra - was -90
                if (SimLibElapsedTime bitand 8192)
                    MoveStick(rStick, 40 * DTR , 10 * DTR);
                else
                    MoveStick(rStick, -40 * DTR , 10 * DTR);

                MoveStick(pStick, 10 * DTR, 5 * DTR);
                //MoveStick(pStick, 30 * DTR, 20 * DTR); // Cobra - was 0

                rocketTimer -= SimLibMajorFrameTime;

                if (rocketTimer < 0)
                {
                    rocketMnvr = RocketFlyout;
                    rocketTimer = 60; // Cobra - was 10
                }

                return 1;
                break;

            case RocketFlyout:

                // just fly level for a few seconds
                if (fp and self->IsPlayer())
                    fprintf(fp, "RocketFlyOut Alt: %f  Range: %f\n", -selfAlt, approxRange * FT_TO_NM);

                MoveStick(rStick, 0 * DTR , 20 * DTR);

                if (FabsF(selfAlt) >= self->GetA2GGunRocketAlt())
                    MoveStick(pStick, 0, 10 * DTR);
                else
                    MoveStick(pStick, 20 * DTR, 20 * DTR); // Cobra - was 0

                rocketTimer -= SimLibMajorFrameTime;
                float flyout;

                if (slowMover)
                    flyout = g_fAGSlowFlyoutRange;
                else
                    flyout = g_fAGFlyoutRange;

                if (((approxRange > flyout * NM_TO_FT) and (FabsF(selfAlt) > self->GetA2GGunRocketAlt())))
                {
                    SetGroundTarget(NULL);   // Cobra - Get another target

                    if ( not isWing)
                        SelectGroundTarget(TARGET_ANYTHING);
                    else
                        AiRunTargetSelection();

                    //else
                    //SelectGroundTarget(TARGET_ANYTHING);
                    rocketMnvr = RocketFlyToTgt;
                    rocketTimer = 60; // 1 minute for something to happen
                    waitingForShot = SimLibElapsedTime + 1;
                }

                return 1;
                break;

            case  RocketFlyToTgt:
                rocketTimer -= SimLibMajorFrameTime;

                if (rocketTimer < 0)
                {
                    if (fabs(taz) > 4 * DTR or approxRange < 6000.0f)
                    {
                        rocketMnvr = RocketFlyout;
                        rocketTimer = 60;
                    }
                    else
                    {
                        rocketTimer = 5;
                    }
                }

                break;
        }


        MonoPrint("noSolution = %d", self->FCC->noSolution);

        if (fabs(taz) > azRough)  // 10 degrees
        {
        }
        else
        {
            rmode = 1; // align pipper

            if (approxRange < 34000)  // Cobra - was 15000
            {
                pmode = 1; // align pipper
            }
            else if (approxRange < 75000)
            {
                pmode = 0; // basic guidance
            }
        }

        if (pmode == 0 or rocketMnvr == RocketFlyout)
        {
            if (fp and self->IsPlayer())
                fprintf(fp, "Pitch: To Attack Alt %f\n", -selfAlt);

            // move to attack alt
            float stick = 0;
            float diff = (-selfAlt) - self->GetA2GGunRocketAlt();

            if (fabs(selfAlt) >  self->GetA2GGunRocketAlt())
                stick = 0;
            else if (diff < -250.0f) // too low
                stick = 35 * DTR; // Cobra - was 10
            else if (diff < -150.0f) // too low
                stick = 5 * DTR;
            else if (diff > 1500.0f)
                stick = -30 * DTR; // Cobra - 45
            else if (diff > 250.0f) // to high
                stick = -10 * DTR;
            else if (diff > 150.0f) // to high
                stick = -5 * DTR;
            else if (diff > 0.0f) // close
                stick = -1 * DTR;
            else if (diff < 0.0f) // close
                stick = 1 * DTR;

            diff = fabsf(diff) - 250.0f;
            diff = min(diff, 1000.0f) / 1000.0f;

            stick *= diff;

            MoveStick(pStick, stick, 20 * DTR); // Cobra - was 20
        }

        if (pmode == 1)
        {
            //fprintf(fp,"Pitch: Pipper To Tgt\n");

            if (fp)
            {
                if (self->IsPlayer())
                    fprintf(fp, "**ME** hdg %f atel %f pitch %f tel %f  taz %f  dtaz %f  iel %f  iaz %f  diaz %f  ang %f  Rng %f  rStk %f  pStk %f \n",
                            self->Yaw()*RTD, atel * RTD, self->Pitch()*RTD, tel * RTD, taz * RTD, dtaz, iel * RTD, iaz * RTD, diaz, angle * RTD, approxRange / NM_TO_FT, rStick * RTD, pStick * RTD);

                //else
                //fprintf(fp,"*WING* hdg %f ata %f pitch %f tel %f  taz %f  dtaz %f  iel %f  iaz %f  diaz %f  ang %f  Rng %f  rStk %f  pStk %f \n",
                // self->Yaw()*RTD, ata*RTD, self->Pitch()*RTD, tel*RTD, taz*RTD, dtaz, iel*RTD, iaz*RTD, diaz, angle*RTD, approxRange / NM_TO_FT, rStick*RTD, pStick*RTD );
                fflush(fp);
            }

            if (FabsF(selfAlt) < 1500.0f and rocketMnvr == RocketFlyToTgt) // Cobra - was 1500
            {
                if (fp and self->IsPlayer())
                    fprintf(fp, "Too Close\n");

                rocketMnvr = RocketFlyout;
                rocketTimer = 60; // Cobra - was 10 sec.
                rmode = 0;
                pmode = 0;
            }
            else
            {
                if (atel < 0.0f)
                    MoveStick(pStick, atel - (g_fRocketPitchCorr * DTR), 20 * DTR); // g_fRocketPitchCorr was 1.7
                else if (atel > 0.0f)
                    MoveStick(pStick, atel + (g_fRocketPitchCorr * DTR), 20 * DTR);
                else
                    MoveStick(pStick, atel, 20 * DTR);
            }
        }

        if (rmode == 0 and rocketMnvr == RocketFlyToTgt)
        {
            if (fp and self->IsPlayer())
                fprintf(fp, "Roll: To Tgt Direction\n");

            if (taz > azRough)
            {
                MoveStick(rStick, 60 * DTR, 45 * DTR);
            }
            else if (taz < -azRough)
            {
                MoveStick(rStick, -60 * DTR, 45 * DTR);
            }
        }

        if (rmode == 1)
        {
            // Yaw pipper to tgt
            if (fp and self->IsPlayer())
                fprintf(fp, "Roll: Pipper To Tgt taz: %f\n", taz * RTD);

            if (taz < 0.0f)
                MoveStick(rStick, taz - (0.5f * DTR), 90 * DTR);
            else if (taz > 0.0f)
                MoveStick(rStick, taz + (0.5f * DTR), 90 * DTR);
        }

        // only fire or a bad pass can occur in FlyToTgt mode
        if (rocketMnvr == RocketFlyToTgt)
        {
            if (angle < 5.0f * DTR and 
                waitingForShot <= SimLibElapsedTime and 
                approxRange < 2.5f * NM_TO_FT)  // Cobra - was 2
            {
                MonoPrint("Firing");
                SetFlag(MslFireFlag);
                rocketMnvr = RocketFiring;
                rocketTimer = 6;
                waitingForShot = SimLibElapsedTime + 60000;

                if (fp)
                    fprintf(fp, "**** Fire Missiles **** Angle %f\n", angle * RTD);

                return 1;
            }

            // bad pass, flyout and re-lineup
            if (approxRange < 1500 and rocketMnvr == RocketFlyToTgt) // and 
                //fabs(taz) > 45 * DTR )
            {
                if (fp and self->IsPlayer())
                    fprintf(fp, "Too Close\n");

                rocketMnvr = RocketFlyout;
                rocketTimer = 60; // Cobra - was 10 sec.
            }
        }

        return 1;
    }

    MonoPrint("Rocket-Fail");
    return 0;

}

int DigitalBrain::GunStrafe(float approxRange, float ata)
{
    FireControlComputer* FCC = self->FCC;

    if (FCC->GetMasterMode() not_eq FireControlComputer::AGGun or
        FCC->GetSubMode() not_eq FireControlComputer::STRAF)
    {
        FCC->SetMasterMode(FireControlComputer::AGGun);
        FCC->SetSubMode(FireControlComputer::STRAF);
    }

    if (ata < 5.0f * DTR and approxRange < 1.2f * NM_TO_FT)
    {
        SetFlag(GunFireFlag);
    }
    else if (approxRange < 1.2f * NM_TO_FT and ata > 75.0f * DTR)
    {
        waitingForShot = SimLibElapsedTime + 5000;
        onStation = Final1;
    }

    return 0;
}

int DigitalBrain::MaverickSetup(float rx, float ry, float ata, float approxRange, RadarClass* theRadar)
{
    FireControlComputer* FCC = self->FCC;
    SMSClass* Sms = self->Sms;
    float az, rMin, rMax;
    int retval = TRUE;

    if (FCC->postDrop)
    {
        // Force a retarget
        if (groundTargetPtr and groundTargetPtr->BaseData()->IsSim())
        {
            SetGroundTarget(((SimBaseClass*)groundTargetPtr->BaseData())->GetCampaignObject());
        }

        SelectGroundTarget(TARGET_ANYTHING);
        retval = FALSE;
    }
    else
    {
        // Point the radar at the target
        if (theRadar)
        {
            // 2001-07-23 MODIFIED BY S.G. MOVERS ARE ONLY 3D ENTITIES WHILE BATTALIONS WILL INCLUDE 2D AND 3D VEHICLES...
            //       if (groundTargetPtr and groundTargetPtr->BaseData()->IsMover())
            if (groundTargetPtr and (groundTargetPtr->BaseData()->IsSim() and ((SimBaseClass *)groundTargetPtr->BaseData())->GetCampaignObject()->IsBattalion()) or (groundTargetPtr->BaseData()->IsCampaign() and groundTargetPtr->BaseData()->IsBattalion()))
                theRadar->SetMode(RadarClass::GMT);
            else
                theRadar->SetMode(RadarClass::GM);

            theRadar->SetDesiredTarget(groundTargetPtr);
            theRadar->SetAGSnowPlow(TRUE);
        }

        F4Assert( not Sms->curWeapon or Sms->curWeapon->IsMissile());


        // Set up FCC for maverick shot
        if (FCC->GetMasterMode() not_eq FireControlComputer::AirGroundMissile)
        {
            //Cobra test
            self->FCC->SetMasterMode(FireControlComputer::AirGroundMissile);
            self->FCC->SetSubMode(FireControlComputer::SLAVE);
            self->FCC->designateCmd = FALSE;
            self->FCC->preDesignate = TRUE;
        }

        // 2001-07-23 REMOVED BY S.G. DO LIKE THE HARMSetup. ONCE SET, DO YOUR STUFF
        //    else
        {
            // Has the current weapon locked?
            if (Sms->curWeapon)
            {
                if (Sms->curWeapon->targetPtr)
                {
                    if (self->curSOI not_eq SimVehicleClass::SOI_WEAPON)
                    {
                        FCC->designateCmd = TRUE;
                    }
                    else
                    {
                        FCC->designateCmd = FALSE;
                    }

                    FCC->preDesignate = FALSE;
                }
                // 2001-07-23 ADDED BY S.G. DON'T LAUNCH IF OUR MISSILE DO NOT HAVE A LOCK
                else
                    retval = FALSE;

                // fcc target needs to be set cuz that's the target
                // that will be used in sms launch missile
                FCC->SetTarget(groundTargetPtr);

                az = (float)atan2(ry, rx);
                ShiAssert(Sms->curWeapon->IsMissile());
                rMax = ((MissileClass*)Sms->GetCurrentWeapon())->GetRMax(-self->ZPos(), self->GetVt(), az, 0.0f, 0.0f);

                // 2001-08-31 REMOVED BY S.G. NOT USED ANYWAY AND I NEED THE FLAG FOR SOMETHING ELSE
                // if (approxRange < rMax)
                // SetATCFlag(InhibitDefensive);

                // rmin is just a percent of rmax
                rMin = rMax * 0.1f;
                // get the sweet spot
                rMax *= 0.8f;

                // Check for firing solution
                if ( not (ata < 15.0f * DTR and approxRange > rMin and approxRange < rMax))
                {
                    retval = FALSE;
                }
                // 2001-07-23 ADDED BY S.G. MAKE SURE WE CAN SEE THE TARGET
                else if (Sms->curWeapon)
                {
                    // First make sure the relative geometry is valid
                    if (Sms->curWeapon->targetPtr)
                    {
                        CalcRelGeom(self, Sms->curWeapon->targetPtr, NULL, 1.0F / SimLibMajorFrameTime);
                        // Then run the seeker if we already have a target
                        ((MissileClass *)Sms->GetCurrentWeapon())->RunSeeker();
                    }

                    // If we have no target, don't shoot
                    if ( not Sms->curWeapon or not Sms->curWeapon->targetPtr)
                        retval = FALSE;
                }

                // END OF ADDED SECTION

                // Check for Min Range
                //if (approxRange < 1.1F * rMin or ata > 165.0F * DTR)
                //{
                // // Bail and try again
                // dx = ( self->XPos() - trackX );
                // dy = ( self->YPos() - trackY );
                // approxRange = (float)sqrt( dx * dx + dy * dy );
                // dx /= approxRange;
                // dy /= approxRange;

                // // COBRA - RED - Calculate minimum range for a nice ATA
                // float r=rMax * 0.75f + (rMax) * PRANDFloatPos() * 0.25f;
                // ipX = trackX + dx * r ;
                // ipY = trackY + dy * r ;

                // ShiAssert (ipX > 0.0F);

                // // Start again
                // onStation = Crosswind;
                // //MonoPrint ("Too close to Maverick, head to IP and try again\n");
                // retval = FALSE;
                //}

                // FRB
                //if (approxRange < 1.1F * rMin or ata > 165.0F * DTR)
                //if (approxRange < 1.0F or ata > 165.0F * DTR)
                if (approxRange < 4.1F)
                {
                    // Release current target and target history
                    SetGroundTarget(NULL);
                    gndTargetHistory[0] = NULL;
                    onStation = Downwind;
                    retval = FALSE - 2;
                }

                // FRB - end
            }
            else
            {
                retval = FALSE;
            }
        }
    }

    return retval;
}

int DigitalBrain::HARMSetup(float rx, float ry, float ata, float approxRange)
{
    FireControlComputer* FCC = self->FCC;
    SMSClass* Sms = self->Sms;
    float /*dx, dy,*/ az, rMin, rMax;
    int retval = TRUE;
    HarmTargetingPod *theHTS;

    theHTS = (HarmTargetingPod*)FindSensor(self, SensorClass::HTS);

    F4Assert( not Sms->curWeapon or Sms->curWeapon->IsMissile());

    // Set up FCC for harm shot
    if (FCC->GetMasterMode() not_eq FireControlComputer::AirGroundHARM)
    {
        //Cobra test
        self->FCC->SetMasterMode(FireControlComputer::AirGroundHARM);
        self->FCC->SetSubMode(FireControlComputer::HARM);  // RV- I-Hawk
    }

    // fcc target needs to be set cuz that's the target
    // that will be used in sms launch missile
    FCC->SetTarget(groundTargetPtr);

    az = (float)atan2(ry, rx);

    // Got this null in multiplayer - RH
    ShiAssert(Sms->curWeapon);

    if (Sms->curWeapon)
    {
        ShiAssert(Sms->curWeapon->IsMissile());
        rMax = ((MissileClass*)Sms->GetCurrentWeapon())->GetRMax(-self->ZPos(), self->GetVt(), az, 0.0f, 0.0f);
    }
    else
    {
        rMax = 0.1F;
    }

    // 2002-01-21 ADDED BY S.G. GetRMax is not enough, need to see if the HARM seeker will see the target as well
    //                          Adjust rMax accordingly.
    float radarRange;//Cobra changed from int to float to match RdrRng

    if (groundTargetPtr->BaseData()->IsSim())
        radarRange = ((SimBaseClass*)groundTargetPtr->BaseData())->RdrRng();
    else
    {
        if (groundTargetPtr->BaseData()->IsEmitting())
            radarRange = RadarDataTable[groundTargetPtr->BaseData()->GetRadarType()].NominalRange;
        else
            radarRange = 0.0f;
    }

    rMax = min(rMax, radarRange);
    rMax = max(rMax, 0.1f);
    // END OF ADDED SECTION 2002-01-21

    //Cobra removed RMax Rmin in favor of this
    rMin = 0.0f; // HARM's don't have a min

    // Make sure the HTS has the target
    if (theHTS)
    {
        theHTS->SetDesiredTarget(groundTargetPtr);

        if ( not theHTS->CurrentTarget())
            retval = FALSE;

        // 2001-06-18 ADDED BY S.G. JUST DO A LOS CHECK :-( I CAN'T RELIABLY GET THE POD LOCK STATUS
        /*if ( not self->CheckLOS(groundTargetPtr))
          retval = FALSE;*///Cobra removed this
        // END OF ADDED SECTION
    }

    //Cobra HARMs are all aspect but we will limit AI to 1/2 range if target behind 3/9 line
    if (ata > 90.0f * DTR and approxRange > rMax * 0.5f)
    {
        retval = FALSE;
    }

    if (approxRange > g_fAIHarmMaxRange * NM_TO_FT) // Cobra limit AI max firing range to g_fAIHarmMaxRange
    {
        retval = FALSE;
    }

    // we want to see what the target campaign
    // entity is doing
    if (groundTargetPtr->BaseData()->IsSim())
    {
        // 2001-06-25 ADDED BY S.G. IF I HAVE SOMETHING IN shotAt, IT COULD MEAN SOMEONE SHOT WHILE THE TARGET WAS AGGREGATED. DEAL WITH THIS
        // If shotAt has something, someone is/was targeting the aggregated entity. If it wasn't me, don't fire at it once it is deaggregated as well.
        if (((FlightClass *)self->GetCampaignObject())->shotAt == ((SimBaseClass *)groundTargetPtr->BaseData())->GetCampaignObject() and ((FlightClass *)self->GetCampaignObject())->whoShot not_eq self)
            retval = FALSE - 1;

        // END OF ADDED SECTION
        // 2001-05-27 MODIFIED BY S.G. LAUNCH AT A CLOSER RANGE IF NOT EMITTING (AND IT'S THE ONLY WEAPONS ON BOARD - TESTED SOMEWHERE ELSE)
        // if ( not ((SimBaseClass *)groundTargetPtr->BaseData())->GetCampaignObject()->IsEmitting() and approxRange > 0.5F * rMax)
        // retval = FALSE;
        // 2002-01-20 MODIFIED BY S.G. If RdrRng() is zero, this means the radar is off. Can't fire at it if it's off (only valid for sim object)
        // if ( not ((SimBaseClass *)groundTargetPtr->BaseData())->GetCampaignObject()->IsEmitting())
        if ( not ((SimBaseClass *)groundTargetPtr->BaseData())->GetCampaignObject()->IsEmitting() or ((SimBaseClass *)groundTargetPtr->BaseData())->RdrRng() == 0)
        {
            retval = FALSE;

            // 2001-07-02 MODIFIED BY S.G. IT'S NOW 0.25 SO TWICE AS CLOSE AS BEFORE
            // if (approxRange < 0.5F * rMax)
            if (approxRange < 0.25F * rMax)
            {
                // 2001-06-24 ADDED BY S.G. TRY WITH SOMETHING ELSE IF YOU CAN
                if (hasAGMissile bitor hasBomb bitor hasRocket bitor hasGun bitor hasGBU)
                    retval = FALSE - 1;
                else
                {
                    // END OF ADDED SECTION
                    ClearATCFlag(HasCanUseAGWeapon);
                    ClearATCFlag(HasAGWeapon);
                    agDoctrine = AGD_NONE;
                    missionComplete = TRUE;
                    self->FCC->SetMasterMode(FireControlComputer::Nav);
                    self->FCC->preDesignate = TRUE;
                    SetGroundTarget(NULL);

                    if (GetWaypointIndex() == GetTargetWPIndex())
                        SelectNextWaypoint();

                    // if we're a wingie, rejoin the lead
                    if (isWing)
                    {
                        mFormation = FalconWingmanMsg::WMWedge;
                        AiRejoin(NULL);
                        // make sure wing's designated target is NULL'd out
                        mDesignatedObject = FalconNullId;
                    }
                    else // So the player's wingmen still know they have something
                        hasWeapons = FALSE; // Got here so nothing else than HARMS was available anyway
                }
            }
        }

        // END OF MODIFIED SECTION
    }
    else
    {
        // campaign entity
        // 2001-06-25 ADDED BY S.G. IF IT IS AGGREGATED, ONLY ONE PLANE CAN SHOOT AT IT WITH HARMS
        if (((CampBaseClass *)groundTargetPtr->BaseData())->IsAggregate())
        {
            // 2002-01-20 ADDED BY S.G. Unless it's an AAA since it has more than one radar.
            if ( not groundTargetPtr->BaseData()->IsBattalion() or ((BattalionClass *)groundTargetPtr->BaseData())->class_data->RadarVehicle < 16)
            {
                // END OF ADDED SECTION 2002-01-20
                // If it's not at what we shot last, then it's valid
                if (((FlightClass *)self->GetCampaignObject())->shotAt not_eq groundTargetPtr->BaseData())
                {
                    ((FlightClass *)self->GetCampaignObject())->shotAt = groundTargetPtr->BaseData();
                    ((FlightClass *)self->GetCampaignObject())->whoShot = self;
                }
                // If one of us is shooting, make sure it's me, otherwise no HARMS for me please.
                else if (((FlightClass *)self->GetCampaignObject())->whoShot not_eq self)
                    retval = FALSE - 1;
            }
        }

        // END OF ADDED SECTION
        // 2001-05-27 MODIFIED BY S.G. LAUNCH AT A CLOSER RANGE IF NOT EMITTING (AND IT'S THE ONLY WEAPONS ON BOARD - TESTED SOMEWHERE ELSE)
        // if ( not groundTargetPtr->BaseData()->IsEmitting() and approxRange > 0.5F * rMax)
        // retval = FALSE;
        // 2001-06-05 MODIFIED BY S.G. THAT'S IT IF YOU CAN CONNECT WITH IT...
        // 2001-06-21 MODIFIED BY S.G. EVEN IF EMITTING, IF IT'S NOT AGGREGATED, DON'T FIRE (IE retval = FALSE)
        if ( not groundTargetPtr->BaseData()->IsEmitting() or not ((CampBaseClass *)groundTargetPtr->BaseData())->IsAggregate())
        {
            retval = FALSE;

            // 2001-07-02 MODIFIED BY S.G. IT'S NOW 0.25 SO TWICE AS CLOSE AS BEFORE
            // if (approxRange < 0.5F * rMax) {
            if (approxRange < 0.25F * rMax)
            {
                // 2001-06-24 ADDED BY S.G. TRY WITH SOMETHING ELSE IF YOU CAN
                if (hasAGMissile bitor hasBomb bitor hasRocket bitor hasGun bitor hasGBU)
                    retval = FALSE - 1;
                else
                {
                    // END OF ADDED SECTION
                    ClearATCFlag(HasCanUseAGWeapon);
                    ClearATCFlag(HasAGWeapon);
                    agDoctrine = AGD_NONE;
                    missionComplete = TRUE;
                    self->FCC->SetMasterMode(FireControlComputer::Nav);
                    self->FCC->preDesignate = TRUE;
                    SetGroundTarget(NULL);

                    if (GetWaypointIndex() == GetTargetWPIndex())
                        SelectNextWaypoint();

                    // if we're a wingie, rejoin the lead
                    if (isWing)
                    {
                        mFormation = FalconWingmanMsg::WMWedge;
                        AiRejoin(NULL);
                        // make sure wing's designated target is NULL'd out
                        mDesignatedObject = FalconNullId;
                    }
                    else // So the player's wingmen still know they have something
                        hasWeapons = FALSE; // Got here so nothing else than HARMS was available anyway
                }
            }
        }

        // END OF MODIFIED SECTION
    }

    // if we use missiles we don't drop bombs
    // unless we shot a harm
    if (agDoctrine not_eq AGD_LOOK_SHOOT_LOOK)
    {
        hasBomb = FALSE;
        hasGBU = FALSE;
        hasRocket = FALSE;
    }

    return retval;
}

int DigitalBrain::JSOWSetup(float rx, float ry, float ata, float approxRange)
{
    FireControlComputer* FCC = self->FCC;
    SMSClass* Sms = self->Sms;
    float /*dx, dy,*/ az, rMin, rMax;
    int retval = TRUE;

    // Set up FCC for JSOW shot
    if (FCC->GetMasterMode() not_eq FireControlComputer::AirGroundBomb)
    {
        //Cobra test
        self->FCC->SetMasterMode(FireControlComputer::AirGroundBomb);
        self->FCC->SetSubMode(FireControlComputer::MAN);
    }

    // fcc target needs to be set cuz that's the target
    // that will be used in sms launch
    FCC->SetTarget(groundTargetPtr);

    az = (float)atan2(ry, rx);

    // Got this null in multiplayer - RH
    ShiAssert(Sms->curWeapon);

    if (Sms->curWeapon)
    {
        rMax = g_fAIJSOWMaxRange;
    }
    else
    {
        rMax = 0.1F;
    }

    //Cobra removed RMax Rmin in favor of this
    rMin = 0.0f; // JSOW's don't have a min

    //Cobra JSOWs have a guidance limit
    if (fabs(ata) > 30.0f * DTR)
    {
        retval = FALSE;
    }

    if (approxRange > g_fAIJSOWMaxRange * NM_TO_FT) // Cobra limit AI max firing range to g_fAIHarmMaxRange
    {
        retval = FALSE;
    }

    // we want to see what the target campaign
    // entity is doing
    if (groundTargetPtr->BaseData()->IsSim())
    {
        // 2001-06-25 ADDED BY S.G. IF I HAVE SOMETHING IN shotAt, IT COULD MEAN SOMEONE SHOT WHILE THE TARGET WAS AGGREGATED. DEAL WITH THIS
        // If shotAt has something, someone is/was targeting the aggregated entity. If it wasn't me, don't fire at it once it is deaggregated as well.
        if (((FlightClass *)self->GetCampaignObject())->shotAt == ((SimBaseClass *)groundTargetPtr->BaseData())->GetCampaignObject() and ((FlightClass *)self->GetCampaignObject())->whoShot not_eq self)
            retval = FALSE - 1;
    }
    else
    {
        // campaign entity
        if (((CampBaseClass *)groundTargetPtr->BaseData())->IsAggregate())
        {
            // If it's not at what we shot last, then it's valid
            if (((FlightClass *)self->GetCampaignObject())->shotAt not_eq groundTargetPtr->BaseData())
            {
                ((FlightClass *)self->GetCampaignObject())->shotAt = groundTargetPtr->BaseData();
                ((FlightClass *)self->GetCampaignObject())->whoShot = self;
            }
        }

        if ( not ((CampBaseClass *)groundTargetPtr->BaseData())->IsAggregate())
        {
            //retval = FALSE; //????
            //It's close to target so try some other weapon
            if (approxRange < 0.25F * rMax)
            {
                if (hasAGMissile bitor hasBomb bitor hasRocket bitor hasGun bitor hasGBU)
                    retval = FALSE - 1;
                else
                {
                    ClearATCFlag(HasCanUseAGWeapon);
                    ClearATCFlag(HasAGWeapon);
                    agDoctrine = AGD_NONE;
                    missionComplete = TRUE;
                    self->FCC->SetMasterMode(FireControlComputer::Nav);
                    self->FCC->preDesignate = TRUE;
                    SetGroundTarget(NULL);

                    if (GetWaypointIndex() == GetTargetWPIndex())
                        SelectNextWaypoint();

                    // if we're a wingie, rejoin the lead
                    if (isWing)
                    {
                        mFormation = FalconWingmanMsg::WMWedge;
                        AiRejoin(NULL);
                        // make sure wing's designated target is NULL'd out
                        mDesignatedObject = FalconNullId;
                    }
                    else // So the player's wingmen still know they have something
                        hasWeapons = FALSE; // Got here so nothing else than HARMS was available anyway
                }
            }
        }
    }

    return retval;
}

void DigitalBrain::AGflyOut()
{
    float dir, dx, dy, x, y, z, approxRange;
    mlTrig trig;

    if (groundTargetPtr)
    {
        trackX = groundTargetPtr->BaseData()->XPos();
        trackY = groundTargetPtr->BaseData()->YPos();
    }
    else
    {
        self->curWaypoint->GetLocation(&x, &y, &z);
        trackX = x;
        trackY = y;
    }

    dx = trackX - self->XPos();
    dy = trackY - self->YPos();
    approxRange = (float)sqrt(dx * dx + dy * dy);

    dir = 1.0f;

    if (rand() % 2)
        dir *= -1.0f;

    float angle = 90.0F * DTR * dir + (float)atan2(dy, dx);
    mlSinCos(&trig, angle);

    // Cobra - Slow-movers don't need to flyout too far
    if (slowMover)
    {
        ipX = trackX + trig.cos * g_fAGSlowFlyoutRange * NM_TO_FT;
        ipY = trackY + trig.sin * g_fAGSlowFlyoutRange * NM_TO_FT;
    }
    else
    {
        ipX = trackX + trig.cos * g_fAGFlyoutRange * NM_TO_FT;
        ipY = trackY + trig.sin * g_fAGFlyoutRange * NM_TO_FT;
    }

    dx = ipX - trackX;
    dy = ipY - trackY;
    approxRange = (float)sqrt(dx * dx + dy * dy);

    if (slowMover)
    {
        if (approxRange < g_fAGSlowFlyoutRange * NM_TO_FT)
        {
            if (dy < 0.0f)
                ipX = trackX - dir * g_fAGSlowFlyoutRange * NM_TO_FT;
            else
                ipX = trackX + dir * g_fAGSlowFlyoutRange * NM_TO_FT;

            if (dx < 0.0f)
                ipY = trackY - dir * g_fAGSlowFlyoutRange * NM_TO_FT;
            else
                ipY = trackY + dir * g_fAGSlowFlyoutRange * NM_TO_FT;
        }
    }
    else if (approxRange < g_fAGFlyoutRange * NM_TO_FT)
    {
        if (dy < 0.0f)
            ipX = trackX - dir * g_fAGFlyoutRange * NM_TO_FT;
        else
            ipX = trackX + dir * g_fAGFlyoutRange * NM_TO_FT;

        if (dx < 0.0f)
            ipY = trackY - dir * g_fAGFlyoutRange * NM_TO_FT;
        else
            ipY = trackY + dir * g_fAGFlyoutRange * NM_TO_FT;
    }

    trackX = ipX;
    trackY = ipY;
    trackZ = ipZ;
}
