#include "stdhdr.h"
#include "digi.h"
#include "simveh.h"
#include "missile.h"
#include "sms.h"
#include "fcc.h"
#include "object.h"
#include "f4error.h"
#include "simbase.h"
#include "aircrft.h"
#include "guns.h"
#include "cmpclass.h"
#include "ui/include/tac_class.h"
#include "ui/include/te_defs.h"
#include "simdrive.h"
#include "radar.h" // 2002-02-09 S.G.
/* S.G. SO ACES ARE NOT THAT SCARED */ #include "flight.h"

int CanEngage(AircraftClass *self, int combatClass, SimObjectType* targetPtr, int type);  // 2002-03-11 MODIFIED BY S.G. Added the 'type' parameter
FalconEntity* SpikeCheck(AircraftClass* self, FalconEntity *byHim = NULL, int *data = NULL); // 2002-02-10 S.G.

extern float g_fRAPDistance;

void DigitalBrain::MissileEngageCheck(void)
{

    float angLimit;
    radModeSelect = 3;//Default

    angLimit = 60.0f * DTR;

    /*-----------------------*/
    /* return if null target */
    /*-----------------------*/
    if (targetPtr == NULL)
    {
        if (curMode == MissileEngageMode)
        {
            self->FCC->SetTarget(NULL);

            if (curMissile)
                curMissile->SetTarget(NULL);
        }

    }

    /*-------*/
    /* entry */
    /*-------*/
    // To enter Missile engage you need a target, a missile within its dlz,
    // you need to be pointeing 'close' to the target, and outside of one
    // mile or be out of guns
    if (curMode not_eq MissileEngageMode)
    {
        if (targetPtr and curMissile and 
            targetData->range <= maxAAWpnRange * 1.05f  and 
            (targetData->range >= 3000.0f or not (((AircraftClass *)self)->Guns)) and 
            targetData->ata < angLimit * 1.05f and self->CombatClass() <= 7/*CanEngage (self, self->CombatClass(), targetPtr, BVRManeuver bitor BVRManeuver*/) // 2002-03-11 MODIFIED BY S.G. Added parameter "BVRManeuver bitor BVRManeuver"
        {
            if (targetPtr->BaseData()->IsSim() and ((SimBaseClass *)targetPtr->BaseData())->pctStrength <= 0.0f)
            {
                return;
            }
            else
            {
                AddMode(MissileEngageMode);
            }
        }
    }
    /*------*/
    /* exit */
    /*------*/
    else
    {
        if (targetData and targetData->range > 2000.0f) // JB 010208
        {
            if ( not targetPtr or
 not curMissile or
                targetData->range > maxAAWpnRange * 1.09F or
                // (targetData->range < 3000.0f * NM_TO_FT and (((AircraftClass *)self)->Guns)) and // Cobra - < 3000 NM??
                (targetData->range < 3000.0f and (((AircraftClass *)self)->Guns)) and 
                targetData->ata > angLimit * 1.09F)//me123 from 1.5
            {
                self->FCC->SetTarget(NULL);

                if (curMissile)
                    curMissile->SetTarget(NULL);
            }
            else
            {
                // Fire control
                FireControl();
                AddMode(MissileEngageMode);
            }
        }
        else if (targetPtr and targetData and ((AircraftClass *)self)->Guns and ((AircraftClass *)self)->Guns->numRoundsRemaining > 0) // JB 010208
            AddMode(GunsEngageMode); // JB 010208
    }
}

void DigitalBrain::MissileEngage(void)
{
    float desiredClosure, rdes, rngdot, desSpeed;
    float xDot, yDot, zDot;
    float tof, rMax;

    // check for exit condition
    if ( not targetPtr or not curMissile)
    {
        return;
    }

    // Set up for missile engage
    if (curMode not_eq lastMode)//only go into missile mode the first time
    {
        FireControlComputer::FCCSubMode newSubMode;

        self->FCC->SetMasterMode(FireControlComputer::Missile);

        switch (self->Sms->curWeaponType)
        {
            case wtAim9:
                newSubMode = FireControlComputer::Aim9;
                break;

            case wtAim120:
                newSubMode = FireControlComputer::Aim120;
                break;

            default:
                newSubMode = FireControlComputer::Aim9;
                break;
        }

        if (newSubMode not_eq self->FCC->GetSubMode())
            self->FCC->SetSubMode(newSubMode);
    }

    // 2002-02-09 ADDED BY S.G. Change our radarMode to digiSTT if we have been detected and we have a radar to start with or if we are shooting a SARH missile
    /*if (SimLibElapsedTime > radarModeTest) {
      RadarClass* theRadar = (RadarClass*)FindSensor(self, SensorClass::Radar);
      if (theRadar) {
       RadarDataSet* radarData = &radarDatFileTable[self->GetRadarType()];
       if (radarData->MaxTwstargets == 0) // Must be equipped with a radar capable of doing TWS...
       theRadar->digiRadarMode = RadarClass::DigiSTT;
       else if (SpikeCheck(self, targetPtr->BaseData())) // If our target is locked onto us, do the same to him
       theRadar->digiRadarMode = RadarClass::DigiSTT;
       else if (self->GetCampaignObject()->GetSpotted(self->GetTeam()) or (curMissile and curMissile and curMissile->sensorArray and curMissile->sensorArray[0]->Type() == SensorClass::RadarHoming))
       theRadar->digiRadarMode = RadarClass::DigiSTT;
       else
       theRadar->digiRadarMode = RadarClass::DigiTWS;
      }
      radarModeTest = SimLibElapsedTime + (4 + (4 - SkillLevel())) * SEC_TO_MSEC;
    }
    // END OF ADDED SECTION 2002-02-09*/


    if (targetData->range <= g_fRAPDistance * NM_TO_FT)//me123 bwrengage will fly the jet outside g_fRAPDistance
        RollAndPull();
    // 2002-03-15 MODIFIED BY S.G. If asked to perform maneuver like chainsaw, do this without doing the BVR stuff. Added the 'mpActionFlags[AI_EXECUTE_MANEUVER] == TRUE+1' condition
    else if ((self->Sms->curWeaponType == wtAim120 and not missileFiredEntity) or
             mpActionFlags[AI_EXECUTE_MANEUVER] == TRUE + 1)
    {
        SetYpedal(0.0F);

        /*---------------------------------------*/
        /* Find current missile's tof, rmax, etc */
        /*---------------------------------------*/
        tof = curMissile->GetTOF((-self->ZPos()), self->GetVt(), targetData->ataFrom, targetPtr->BaseData()->GetVt(),
                                 targetData->range);
        rMax = curMissile->GetRMax((-self->ZPos()), self->GetVt(), targetData->az, targetPtr->BaseData()->GetVt(), targetData->ataFrom);

        /*---------------------------------*/
        /* Put a deadband on target's zdot */
        /*---------------------------------*/
        xDot = targetPtr->BaseData()->XDelta();
        yDot = targetPtr->BaseData()->YDelta();
        zDot = targetPtr->BaseData()->ZDelta() * 0.1F;
        zDot = Math.DeadBand(zDot, -100.0F, 100.0F);

        if (holdlongrangeshot)//Cobra, deprecated removed from dlogic
        {
            /*----------------------*/
            /* Find the track point */
            /*----------------------*/
            SetTrackPoint(
                targetPtr->BaseData()->XPos() + xDot * tof,
                targetPtr->BaseData()->YPos() + yDot * tof
            );

            if (targetData->range > 15 * NM_TO_FT)
            {
                trackZ = targetPtr->BaseData()->ZPos();

                if (SkillLevel() >= 3)
                {
                    trackZ -= targetData->range / 3.0f;
                }
            }
            else
            {
                trackZ = targetPtr->BaseData()->ZPos();
            }

            // TJL 11/09/03 1.7 is way too high. Changed to 1.3
            desSpeed = 1.3F * cornerSpeed;
        }
        /*----------------------------------*/
        /* Ownship ahead of target 3/9 line */
        /*----------------------------------*/
        else if (fabs(targetData->azFrom) < 90.0F * DTR)
        {
            /*----------------------*/
            /* Find the track point */
            /*----------------------*/
            SetTrackPoint(
                targetPtr->BaseData()->XPos() + xDot * tof,
                targetPtr->BaseData()->YPos() + yDot * tof,
                targetPtr->BaseData()->ZPos() + zDot * tof
            );

            desSpeed = 1.3F * cornerSpeed;
        }
        /*-----------------------------------*/
        /* Ownship behind of target 3/9 line */
        /*-----------------------------------*/
        else
        {
            /*-------------------------------------------------------------*/
            /* Calculate desired closure                                   */
            /* desired closure is 10% of range in feet, expressed as knots */
            /* -100.0 Kts <= desired closure <= 300 Kts                    */
            /*-------------------------------------------------------------*/
            rdes = 0.40F * rMax;
            //   desiredClosure = 0.1F * (targetData->range - rdes);
            desiredClosure = ((targetData->range - 3000  / 1000.0F) * 50.0F); //me123
            desiredClosure = min(max(desiredClosure, -100.0F),
                                 2300.0F);

            /*-----------------*/
            /* Closing to fast */
            /*-----------------*/
            if (-targetData->rangedot * FT_TO_NM > desiredClosure)
            {
                /*---------------------------------------*/
                /* Find the track point - Lag the target */
                /*---------------------------------------*/
                SetTrackPoint(
                    targetPtr->BaseData()->XPos() + xDot * tof * 0.9F,
                    targetPtr->BaseData()->YPos() + yDot * tof * 0.9F,
                    targetPtr->BaseData()->ZPos() + zDot * tof * 0.9F
                );

                if (targetData->range > 10 * NM_TO_FT)
                {
                    trackZ -= 10000;
                }
            }
            else
                /*-------------------------*/
                /* Not closing fast enough */
                /*-------------------------*/
            {
                /*----------------------*/
                /* Find the track point */
                /*----------------------*/
                SetTrackPoint(
                    targetPtr->BaseData()->XPos() + xDot * tof,
                    targetPtr->BaseData()->YPos() + yDot * tof,
                    targetPtr->BaseData()->ZPos() + zDot * tof
                );

            }

            /*--------------------------------------*/
            /* Set the throttle for desired closure */
            /*--------------------------------------*/
            rngdot = (targetData->rangedot) * FTPSEC_TO_KNOTS;
            desSpeed = self->GetKias() + desiredClosure ;//me123 - rngdot;

            //me123   if (desSpeed < cornerSpeed) desSpeed = cornerSpeed;
        }

        /*----------*/
        /* Track It */
        /*----------*/
        trackZ = min(trackZ , -4000.0f);
        TrackPoint(5.0f, desSpeed);  // desSpeed = knots
    }
    else BvrEngage();
}

void DigitalBrain::WeaponSelection(void)
{
    MissileClass* theMissile;
    MissileClass* lastMissile = curMissile;
    // MODIFIED BY S.G. OUR CODE FOR BEST WEAPON WANTS ZERO//float pctRange = 10000000.0F;
    float pctRange = 10000000.0F;
    float thisPctRange;
    // float thisASE;
    float thisRmin;
    float rmax;
    int i;
    float skillMod;


    curMissile = NULL;
    curMissileStation = -1;
    curMissileNum     = -1;
    self->Sms->SetCurHardpoint(-1);
    self->UnSetFlagSilent(HAS_MISSILES);

    // as we select missiles we're going to determine what our max
    // range is.  This will be usefull for determining other tactics
    // start off by checking guns availability
    if (((AircraftClass *)self)->Guns and 
        ((AircraftClass *)self)->Guns->numRoundsRemaining > 0)
        maxAAWpnRange = 6000.0f;
    else
        maxAAWpnRange = 0.0f;

    // Adjust firing range base on skill level
    // Aces fire at 80% max range, rookies go out to 110% max range
    //   skillMod = 0.8F + (5 - SkillLevel()) * 0.06F;
    // MODIFIED BY S.G. SO SKILL DOESN'T AFFECT THE WEAPON SELECTION.
    // WE'LL USE 110% SO RECRUIT CAN FIRE WHILE OUTSIDE RMAX IN DLOGIN.CPP FCC CODE
    skillMod = 0.8F + 5 * 0.06F;

    // Find all weapons w/in parameters
    for (i = 0; i < self->Sms->NumHardpoints(); i++)
    {
        // 2002-03-26 MN A data bug (weapon type = Guns, weapon Class = wcAimWpn) caused a crash in GetRMax below.
        // Just make sure this doesn't happen again
#ifdef DEBUG
        if (self->Sms->hardPoint[i]->GetWeaponType() == wtGuns and not (self->Sms->hardPoint[i]->GetWeaponClass() == wcGunWpn or self->Sms->hardPoint[i]->GetWeaponClass() == wcTank))
            ShiAssert(false);

#endif

        if (self->Sms->hardPoint[i]->GetWeaponType() == wtGuns)
            continue;

        // Is this an aim missile?
        if (self->Sms->hardPoint[i]->GetWeaponClass() not_eq wcAimWpn)
            continue;

        if (targetPtr->BaseData()->IsHelicopter() and 
            self->Sms->hardPoint[i]->GetWeaponType() not_eq wtAim9)
            continue;

        self->Sms->curWeapon.reset();// = NULL;
        self->Sms->SetCurHardpoint(i);
        self->Sms->curWpnNum = -1;
        self->Sms->WeaponStep();

        if (self->Sms->curWeapon == NULL)
        {
            continue;
        }

        theMissile   = (MissileClass *)(self->Sms->GetCurrentWeapon());

        if (theMissile)
        {
            // get maximum range
            rmax = theMissile->GetRMax(-self->ZPos(),
                                        self->GetVt(),
                                        targetData->az,
                                        targetPtr->BaseData()->GetVt(),
                                        targetData->ataFrom);

            // set our max weapon range
            if (rmax > maxAAWpnRange)
                maxAAWpnRange = rmax;

            self->SetFlagSilent(HAS_MISSILES);

            /*
            ** edg: we don't seem to need this
            thisASE      = DTR *
             theMissile->GetASE(-self->ZPos(),
             self->GetVt(),
                 targetData->ataFrom,
             targetPtr->BaseData()->GetVt(),
             targetData->range);
            */
            if (targetPtr)
            {


                // we want to choose the missile that's the closest, without going over
                // to 80% (modified by pilot level) max range of the missile on the target
                thisPctRange = rmax * 0.8f - targetData->range; //me123 rmax * skillMod - targetData->range;



                // leon's kludge
                thisRmin = 2000;//me123 0.01F * rmax;

                //me123 if ir missile pick the missile with greatest range becourse it also also best seeker)
                if (theMissile->sensorArray and theMissile->sensorArray[0]->Type() == SensorClass::IRST and 
                     curMissile and curMissile->sensorArray[0]->Type() == SensorClass::IRST and 
                     thisPctRange > pctRange and thisPctRange > 0.0F
                   )
                {
                    pctRange = thisPctRange;
                    curMissile = (MissileClass *)(self->Sms->GetCurrentWeapon());
                    curMissileStation = self->Sms->CurHardpoint();
                    curMissileNum     = self->Sms->curWpnNum;
                }
                //me123 pick the radar missile if no current missile or
                //we have a radar or radarhoming missile but this one has father range or
                // outside 3nm and curmissile is a irmissile
                else if (
                    theMissile->sensorArray and theMissile->sensorArray[0]->Type() not_eq SensorClass::IRST and not curMissile
                    or
                    (curMissile and 
                     (curMissile->sensorArray[0]->Type() == SensorClass::Radar or
                      curMissile->sensorArray[0]->Type() == SensorClass::RadarHoming) and 
                     thisPctRange > pctRange and thisPctRange > 0.0F)
                    or
                    curMissile and curMissile->sensorArray[0]->Type() == SensorClass::IRST and 
                    targetPtr->localData->range > 3 * NM_TO_FT)
                {
                    pctRange = thisPctRange;
                    curMissile = (MissileClass *)(self->Sms->GetCurrentWeapon());
                    curMissileStation = self->Sms->CurHardpoint();
                    curMissileNum     = self->Sms->curWpnNum;
                }
                //me123 pick a ir missile if no cur missile.
                else if ( not curMissile and 
                         theMissile->sensorArray and 
                         theMissile->sensorArray[0]->Type() == SensorClass::IRST)
                {
                    pctRange = thisPctRange;
                    curMissile = (MissileClass *)(self->Sms->GetCurrentWeapon());
                    curMissileStation = self->Sms->CurHardpoint();
                    curMissileNum     = self->Sms->curWpnNum;
                }

            } // if targetPtr
        } // end if a missile
    }  //end for # hardpoints

    if (curMissile)
    {
        self->Sms->curWeapon.reset();// = NULL;
        self->Sms->SetCurHardpoint(curMissileStation);
        self->Sms->curWpnNum = -1;
        self->Sms->WeaponStep();

        // Assert the weapon still exists
        F4Assert(self->Sms->curWeapon);
    }

    if (lastMissile and lastMissile->launchState == MissileClass::PreLaunch)
        lastMissile->SetTarget(NULL);

    // Guns only check
    // MODIFIED BY S.G. SO ACES ARE NOT SCARY CATS (A2A ABORTS)
    // THE REAL A2A ABORT TAKES PLACE IN Separate.cpp (if (missionClass == AAMission and not IsSetATC(AceGunsEngage)) aaAbort = TRUE;) BUT THIS IS THE SOURCE OF THE PROBLEM
    // Same as before (ie. ace and missile can't reach) but we adds more condition.
    // 1. Must be out of everything (maxAAWpnRange == 0)
    // 2. Range to target needs to be more than 7 NM (otherwise we're too close so we commit)
    // 3. There are less than 2 vehicles in our flight (ie, by ourself)
    //   if (SkillLevel() == 4 and ((AircraftClass *)self)->Guns and not curMissile)
    if (SkillLevel() == 4 and ((AircraftClass *)self)->Guns and not curMissile and 
        maxAAWpnRange == 0.0f and targetData->range > 7.0f * NM_TO_FT and ((FlightClass*)self->GetCampaignObject())->GetTotalVehicles() < 2)
    {
        if (SimDriver.RunningCampaign())
        {
            ClearATCFlag(AceGunsEngage);
        }
        else if (SimDriver.RunningTactical() and current_tactical_mission and 
                 current_tactical_mission->get_type() == tt_engagement)
        {
            ClearATCFlag(AceGunsEngage);
        }
        else
        {
            SetATCFlag(AceGunsEngage);
        }
    }
    else
    {
        SetATCFlag(AceGunsEngage);
    }
}
