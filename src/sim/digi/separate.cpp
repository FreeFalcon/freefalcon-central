#include "stdhdr.h"
#include "digi.h"
#include "object.h"
#include "airframe.h"
#include "aircrft.h"
#include "campwp.h"
#include "MsgInc/WingmanMsg.h"
#include "wingorder.h"
#include "simdrive.h"
#include "ui/include/tac_class.h"
#include "ui/include/te_defs.h"
/* S.G. */ #include "unit.h"
/* S.G. */ #include "campbase.h"
/* S.G. */ #include "flight.h"
/* MN */   #include "MissEval.h"

float RangeAtTailChase(AircraftClass* tgt, SimObjectType* launcher);
float TailChaseRMaxNe(AircraftClass* tgt, SimObjectType* launcher, int flag);





void DigitalBrain::SeparateCheck(void)
{
    float turnRadius, rMaxNe, sepRange;
    float gs;
    WayPointClass* tmpWaypoint = self->waypoint;
    short edata[10];
    char inTraining = FALSE;
    char aaAbort = FALSE;
    char agAbort = FALSE;
    char campAbort = FALSE;
    char damageAbort = FALSE;



    //Cobra test we want to stop AI from aborting in fictional dogfight scenario
    if (FalconLocalGame->GetGameType() == game_Dogfight)
        return;

    // Check for RTB
    if (((SimDriver.RunningTactical() and current_tactical_mission and 
          current_tactical_mission->get_type() == tt_training)))
    {
        inTraining = TRUE;
    }

    // 2001-08-31 BROUGHT BACK TO LIKE IT IS WAS ORIGINALLY (ALSO LIKE IN RP4/5)
    if (missionClass == AAMission and not IsSetATC(AceGunsEngage))
        aaAbort = FALSE;

    // 2002-02-12 added by MN - Aircraftclass checks for target being occupied and sets AWACSsaidAbort
    // Do an agAbort here if so
    if (self->AWACSsaidAbort)
    {
        SetGroundTarget(NULL);
        agAbort = TRUE;
    }

    // 2001-12-28 MN this prevents FAC aircraft from aborting missions in 3D (they have no weapons..)
    /* if (missionClass == AGMission and not IsSetATC(HasAGWeapon) and missionType not_eq AMIS_FAC)
     {
        if ((missionType not_eq AMIS_BDA and missionType not_eq AMIS_RECON) or not hasCamera)
    // 2001-05-12 ADDED BY S.G. ABORT ONLY WHEN THE MISSION IS NOT COMPLETED OR WE ARE AT THE ATTACK WAYPOINT, OTHERWISE FOLLOW WAYPOINTS HOME
    // 2001-06-21 MODIFIED BY S.G. BROUGHT BACK TO WHAT IS RELEASED
    //    if ( not missionComplete or not self->curWaypoint or self->curWaypoint->GetWPFlags() bitand WPF_TARGET)
        if ( not missionComplete)
    // END OF ADDED SECTION
             agAbort = TRUE;
     }*/

    if (missionType == AMIS_ABORT)
        campAbort = TRUE;



    if (self->pctStrength < 0.50F)
        damageAbort = TRUE;

    if ((aaAbort or agAbort or campAbort or damageAbort) and not inTraining)
    {
        // If pre IP go to landing, else step past target
        if (curMode not_eq RTBMode and curMode not_eq LandingMode and curMode not_eq TakeoffMode)
        {
            // Drop ground target if any
            if (groundTargetPtr)
            {
                SetGroundTarget(NULL);
            }

            // 2001-05-13 MODIFIED BY S.G. TO MAKE IT SIMILAR TO THE ABOVE agAbort CODE
            // 2001-06-21 RESTATED BY S.G. BROUGHT BACK TO WHAT IS RELEASED
            if ( not IsSetATC(ReachedIP))
                //      if ( not missionComplete or not self->curWaypoint or self->curWaypoint->GetWPFlags() bitand WPF_TARGET)
            {
                // Find the landing waypoint, and make it the current one
                while (tmpWaypoint)
                {
                    if (tmpWaypoint->GetWPAction() == WP_LAND)
                    {
                        break;
                    }

                    tmpWaypoint = tmpWaypoint->GetNextWP();
                }

                if (tmpWaypoint and not isWing)
                {
                    self->curWaypoint = tmpWaypoint;
                    SetWaypointSpecificStuff();
                }

                if ( not IsSetATC(SaidRTB))
                {
                    SetATCFlag(SaidRTB);
                    // Call going home
                    edata[0] = isWing;
                    AiMakeRadioResponse(self, rcIMADOT, edata);
                }
            }
            else
            {
                // Find the waypoint after the target, and make it the current one if
                // we haven't gotten there yet
                tmpWaypoint = self->curWaypoint;

                while (tmpWaypoint)
                {
                    // 2001-05-13 MODIFIED BY S.G. WPF_TARGET IS A FLAG WITHIN MANY, DON'T TEST FOR EQUALITY
                    //  if (tmpWaypoint->GetWPFlags() == WPF_TARGET)
                    if (tmpWaypoint->GetWPFlags() bitand WPF_TARGET)
                    {
                        tmpWaypoint = tmpWaypoint->GetNextWP();
                        break;
                    }

                    tmpWaypoint = tmpWaypoint->GetNextWP();
                }

                if (tmpWaypoint and not isWing)
                {
                    self->curWaypoint = tmpWaypoint;
                    SetWaypointSpecificStuff();
                }

                if ( not IsSetATC(SaidRTB))
                {
                    SetATCFlag(SaidRTB);
                    // Call going home
                    edata[0] = isWing;
                    AiMakeRadioResponse(self, rcIMADOT, edata);
                }
            }
        }

        if ( not isWing)
        {
            AddMode(RTBMode);
        }
    }

    if (( not isWing or IsSetATC(SaidBingo)) and curMode == RTBMode)
        AddMode(RTBMode);

    if (isWing and mpActionFlags[AI_RTB])
        AddMode(RTBMode);

    if ( not targetPtr)
        return;

    // If you can't be offensive, and you have a target/threat, run away
    /*   if (targetPtr->localData->range < 10.0F * targetData->ataFrom / (180.0F * DTR))
          AddMode (BugoutMode);
    *///me123



    // go no further unless separation is desired
    if (IsSetATC(SaidBingo)  or
        curMode == WVREngageMode and (aaAbort or agAbort or campAbort or damageAbort))
    {
        // Entry
        if (curMode not_eq SeparateMode and targetData->range < 2.0f * NM_TO_FT)
        {
            // Find range where tail chase would begin
            sepRange = RangeAtTailChase(self, targetPtr);

            // Find missile Rmax for a tail chase
            // Final flag True = MRM, False = SRM
            // Skip the check if target has no missiles
            if (targetPtr->BaseData()->IsSim() and 
                (((SimBaseClass*)targetPtr->BaseData())->IsSetFlag(HAS_MISSILES) or targetPtr->localData->range > 2.0F * NM_TO_FT))
                rMaxNe = TailChaseRMaxNe(self, targetPtr, FALSE);
            else
                rMaxNe = 6000.0F;

            // Enter if separation range > rne
            if (rMaxNe < sepRange and IsSetATC(SaidJoker))
            {
                // If inside one turn radius threat needs to be ahead of 3/9 line
                // else behind 3/9 line
                gs = af->SustainedGs(TRUE);
                turnRadius = self->GetVt() * self->GetVt() / ((float)sqrt(gs * gs - 1.0F) * GRAVITY);

                if (targetData->range < turnRadius)
                {
                    if (targetData->ata < 90.0 * DTR) AddMode(SeparateMode);
                }
                else
                {
                    if (targetData->ata > 90.0 * DTR) AddMode(SeparateMode);
                }
            }
        }
        else if (targetData->range < 6.0f * NM_TO_FT and 
                 targetPtr->localData->rangedot > 0.0f or
                 targetData->range > 6000)// last mode was seperate
        {
            AddMode(SeparateMode);
        }


    }

    //TJL 11/08/03 Bugout code courtesy of Jam/Mike
    // Is the AI deep six? ataFrom is from target nose.
    if (targetData->ataFrom > 135.0F * DTR and FalconLocalGame->GetGameType() not_eq game_Dogfight)
    {
        if ( not bugoutTimer)
        {
            //Set 90 second timer
            bugoutTimer = SimLibElapsedTime + 90000;
        }

        //If not 90 seoncds, do nothing
        if (bugoutTimer > SimLibElapsedTime)
        {
            return;
        }
        else
        {
            //If deep six > 90 seconds, disengage.
            AddMode(BugoutMode);
        }
    }
    else
    {
        //reset our timer
        bugoutTimer = 0;
    }

}

float RangeAtTailChase(AircraftClass* target, SimObjectType* launcher)
{
    float initialRange, finalRange;
    float vLaunch, vTarget;
    float aspectLaunch, aspectTarget;
    float pSubSLaunch, pSubSTarget;
    float radiusLaunch, radiusTarget;
    float rateLaunch, rateTarget;
    float xLaunch, xTarget;
    float xLaunch1, xTarget1;
    float xLaunch2, xTarget2;
    float yLaunch, yTarget;
    float yLaunch1, yTarget1;
    float yLaunch2, yTarget2;
    float thetaLaunch1, thetaTarget1;
    float thetaLaunch2, thetaTarget2;
    float timeLaunch, timeTarget;
    float speedLaunch, speedTarget;
    float rc;
    float thetac1, thetac2;
    float gammac1, gammac2;
    float tMax;
    int left = FALSE;

    // Initializations
    initialRange = launcher->localData->range;
    vLaunch = launcher->BaseData()->GetVt();
    vTarget = target->GetVt();

    // Current aspect
    if (launcher->localData->az > 0.0F)
        aspectTarget = (180.0F * DTR) - launcher->localData->ata;
    else
        aspectTarget = launcher->localData->ata - (180.0F * DTR);

    if (launcher->localData->azFrom > 0.0F)
        aspectLaunch = (180.0F * DTR) - launcher->localData->ataFrom;
    else
        aspectLaunch = launcher->localData->ataFrom - (180.0F * DTR);

    // Excess energy. Assume he's in better shape than me
    pSubSTarget = target->af->PsubS(TRUE);
    pSubSLaunch = 1.2F * pSubSTarget;

    // Turn Radii (assume 5g turn)
    radiusLaunch = vLaunch * vLaunch / (5.0F * GRAVITY);
    radiusTarget = vTarget * vTarget / (target->af->SustainedGs(TRUE) * GRAVITY);

    // Turn rate
    rateLaunch = vLaunch / radiusLaunch;
    rateTarget = vTarget / radiusTarget;

    // Go Left
    xLaunch1 = -radiusLaunch * (float)sin(aspectLaunch);
    yLaunch1 =  radiusLaunch * (float)cos(aspectLaunch);

    xTarget1 =  initialRange + radiusTarget * (float)sin(aspectTarget);
    yTarget1 = -radiusTarget * (float)cos(aspectTarget);

    rc = (float)sqrt((xLaunch1 - xTarget1) * (xLaunch1 - xTarget1) + (yLaunch1 - yTarget1) * (yLaunch1 - yTarget1));
    thetac1 = (float)asin(max(-1.0F, min(1.0F, (radiusTarget + radiusLaunch) / rc)));
    gammac1 = (float)asin(max(-1.0F, min(1.0F, (yTarget1 - yLaunch1) / rc)));

    thetaLaunch1 = -(aspectLaunch - gammac1 - thetac1);
    thetaTarget1 = aspectTarget - gammac1 - thetac1;

    // Go Right
    xLaunch2 =  radiusLaunch * (float)sin(aspectLaunch);
    yLaunch2 = -radiusLaunch * (float)cos(aspectLaunch);

    xTarget2 =  initialRange + radiusTarget * (float)sin(aspectTarget);
    yTarget2 = -radiusTarget * (float)cos(aspectTarget);

    rc = (float)sqrt((xLaunch2 - xTarget2) * (xLaunch2 - xTarget2) + (yLaunch2 - yTarget2) * (yLaunch2 - yTarget2));
    thetac2 = (float)asin(max(-1.0F, min(1.0F, (radiusTarget - radiusLaunch) / rc)));
    gammac2 = (float)asin(max(-1.0F, min(1.0F, (yTarget2 - yLaunch2) / rc)));

    thetaLaunch2 = aspectLaunch - gammac2 - thetac2;
    thetaTarget2 = aspectTarget - gammac2 - thetac2;

    // Choose between left and right
    if (thetaLaunch2 < 0.0F or thetaLaunch1 > thetaLaunch2)
        left = TRUE;

    // if left use the 1 case, otherwise use the 2 case
    if (left)
    {
        timeLaunch = thetaLaunch1 / rateLaunch;
        timeTarget = thetaTarget1 / rateTarget;
    }
    else
    {
        timeLaunch = thetaLaunch2 / rateLaunch;
        timeTarget = thetaTarget2 / rateTarget;
    }

    tMax = max(timeLaunch, timeTarget);

    speedLaunch = vLaunch * (tMax - timeLaunch) +
                  pSubSLaunch * GRAVITY * 0.5F * (tMax - timeLaunch) * (tMax - timeLaunch);
    speedTarget = vTarget * (tMax - timeTarget) +
                  pSubSTarget * GRAVITY * 0.5F * (tMax - timeTarget) * (tMax - timeTarget);


    // Find range when stern chase begins
    if (left)
    {
        xLaunch = xLaunch1 + radiusLaunch * (float)sin(aspectLaunch + thetaLaunch1) +
                  speedLaunch * (float)cos(aspectLaunch + thetaLaunch1);
        yLaunch = yLaunch1 - radiusLaunch * (float)cos(aspectLaunch + thetaLaunch1) +
                  speedLaunch * (float)sin(aspectLaunch + thetaLaunch1);
        xTarget = xTarget1 - radiusTarget * (float)sin(aspectTarget - thetaTarget1) +
                  speedTarget * (float)cos(aspectTarget - thetaTarget1);
        yTarget = yTarget1 + radiusTarget * (float)cos(aspectTarget - thetaTarget1) +
                  speedTarget * (float)sin(aspectTarget - thetaTarget1);
    }
    else
    {
        xLaunch = xLaunch2 - radiusLaunch * (float)sin(aspectLaunch - thetaLaunch2) +
                  speedLaunch * (float)cos(aspectLaunch - thetaLaunch2);
        yLaunch = yLaunch2 + radiusLaunch * (float)cos(aspectLaunch - thetaLaunch2) +
                  speedLaunch * (float)sin(aspectLaunch - thetaLaunch2);
        xTarget = xTarget2 - radiusTarget * (float)sin(aspectTarget - thetaTarget2) +
                  speedTarget * (float)cos(aspectTarget - thetaTarget2);
        yTarget = yTarget2 + radiusTarget * (float)cos(aspectTarget - thetaTarget2) +
                  speedTarget * (float)sin(aspectTarget - thetaTarget2);
    }

    finalRange = (float)sqrt((xLaunch - xTarget) * (xLaunch - xTarget) + (yLaunch - yTarget) * (yLaunch - yTarget));

    return finalRange;
}

float TailChaseRMaxNe(AircraftClass*, SimObjectType*, int missiletype)
{
    return 6.0F * NM_TO_FT;
}

void DigitalBrain::FuelCheck(void)
{
    //Cobra no need for all this ... ;)

    //WayPointClass* tmpWaypoint = self->waypoint;
    //float xPos, yPos, zPos, dist;
    //float ps, fuelNeeded, fuelRemain, fuelBurn;
    float fuelRemain = 0.0f;

    //we don't care if we need fuel on the ground :)
    if (self->OnGround())
        return;

    // Find airbase
    /*while (tmpWaypoint)
    {
     if (tmpWaypoint->GetWPAction() == WP_LAND)
     {
     break;
     }
     tmpWaypoint = tmpWaypoint->GetNextWP();
    }

    if ( not tmpWaypoint)
      tmpWaypoint = self->waypoint;

    if (tmpWaypoint)
    {
      // Find fuel needed
    tmpWaypoint->GetLocation (&xPos, &yPos, &zPos);
      dist = (float)sqrt ((self->XPos()-xPos)*(self->XPos()-xPos) + (self->YPos()-yPos)*(self->YPos()-yPos));
      fuelBurn = af->FuelBurn(FALSE);
      ps = af->PsubS(FALSE);
      // Acceleration = G*Ps/vt
      fuelNeeded = dist / (self->GetVt() + 2.0F * GRAVITY * ps/self->GetVt()) * fuelBurn / 3600.0F;

      // Fuel left at home
      fuelRemain = af->Fuel() - fuelNeeded;
      */ //End of removed code

    //me123 arhh we don't want the wingmen to think :), just say the prebrefed fuelstate joker, bingo
    fuelRemain = af->Fuel();//me123 hack addet

    // Check fuel state
    if (fuelRemain < af->GetJoker())//me123 from 1000
    {
        if ( not IsSetATC(SaidJoker))
        {
            // Say Joker
            //            MonoPrint ("Digi joker fuel\n");
            SetATCFlag(SaidJoker);
            AiSendCommand(self, FalconWingmanMsg::WMJokerFuel, AiAllButSender);
        }
        else if (fuelRemain < af->GetBingo())
        {
            if ( not IsSetATC(SaidBingo))
            {
                // Say Bingo
                //               MonoPrint ("Digi bingo fuel\n");
                SetATCFlag(SaidBingo);
                AiSendCommand(self, FalconWingmanMsg::WMBingoFuel, AiAllButSender);
            }
            else if (fuelRemain < af->GetFumes() and not IsSetATC(SaidFumes))
            {
                // Say Fumes
                //               MonoPrint ("Digi fumes fuel\n");
                SetATCFlag(SaidFumes);
                AiSendCommand(self, FalconWingmanMsg::WMFumes, AiAllButSender);
            }
            else if (af->Fuel() == 0.0F and not IsSetATC(SaidFlameout))
            {
                // Say Flameout
                //               MonoPrint ("Digi flameout\n");
                SetATCFlag(SaidFlameout);
                AiSendCommand(self, FalconWingmanMsg::WMFlameout, AiAllButSender);
            }
        }
    }
}
