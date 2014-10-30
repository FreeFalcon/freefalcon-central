#include "stdhdr.h"
#include "digi.h"
#include "simveh.h"
#include "object.h"
#include "aircrft.h"
#include "airframe.h"
#include "simdrive.h"
#include "classtbl.h"
#include "entity.h"
#include "fakerand.h"
#include "flight.h"
#include "wingorder.h"
#include "playerop.h"
#include "radar.h" // 2002-02-09 S.G.

// #define DEBUG_WVR_ENGAGE
#define MANEUVER_DEBUG
#ifdef MANEUVER_DEBUG
#include "Graphics/include/drawbsp.h"
extern int g_nShowDebugLabels;
#endif

extern bool g_bUseNewCanEnage; // 2002-03-11 S.G.

#ifdef DEBUG_WVR_ENGAGE
#define PrintWVRMode(a) MonoPrint(a)
#else
#define PrintWVRMode(a)
#endif

int CanEngage(AircraftClass *self, int combatClass, SimObjectType* targetPtr, int type);  // 2002-03-11 MODIFIED BY S.G. Added the 'type' parameter
FalconEntity* SpikeCheck(AircraftClass* self, FalconEntity *byHim = NULL, int *data = NULL); // 2002-02-10 S.G.

/* Check for Entry/Exit condition into WVR */
void DigitalBrain::WvrEngageCheck(void)
{
    float engageRange;
    engageRange = 3.0F * NM_TO_FT;

    /*---------------------*/
    /* return if no target */
    /*---------------------*/
    if (targetPtr == NULL or (mpActionFlags[AI_ENGAGE_TARGET] not_eq AI_AIR_TARGET and missionClass not_eq AAMission and not missionComplete) or curMode == RTBMode) // 2002-03-04 MODIFIED BY S.G. Use new enum type
    {
        //me123     ClearTarget();
        engagementTimer = 0;
    }
    /*-------*/
    /* entry */
    /*-------*/
    else if (curMode not_eq WVREngageMode and targetData->range < engageRange and 
             (targetPtr->BaseData()->IsAirplane() or targetPtr->BaseData()->IsFlight() or targetPtr->BaseData()->IsHelicopter()) and // 2002-03-05 MODIFIED BY S.G. airplane, choppers and fligth are ok in here (choppers only makes it here if it passed the SensorFusion test first)
             SimLibElapsedTime > engagementTimer and 
             CanEngage(self, self->CombatClass(), targetPtr, WVRManeuver))  // 2002-03-11 MODIFIED BY S.G. Added parameter WVRManeuver
    {
        AddMode(WVREngageMode);
    }
    else if (curMode == WVREngageMode and 
             targetPtr->localData->range < 1.5F * engageRange and 
             CanEngage(self, self->CombatClass(), targetPtr, WVRManeuver)) // 2002-03-11 MODIFIED BY S.G. Added parameter WVRManeuver
    {
        AddMode(WVREngageMode);
    }
    else
    {
        engagementTimer = 0;
    }
}

/*
** Name: WVREngage
** Description:
** Main function for within visual range engagement.
** Basically this function just checks the tactics timer to determiine
** when we next need to eval our BFM stuff.  It then runs the current
** tactic.
*/
void DigitalBrain::WvrEngage(void)
{
    int lastTactic = wvrCurrTactic;
    float left = 0.0F, right = 0.0F, cur = 0.0F;
    int myCombatClass = self->CombatClass();
    int hisCombatClass = 0;
    ManeuverChoiceTable *theIntercept = NULL;
    int numChoices = 0, myChoice = 0;

#ifdef MANEUVER_DEBUG
    char tmpchr[40];
    strcpy(tmpchr, "WvrEngage");
#endif
    radModeSelect = 3;//default

    // 2002-01-27 MN No need to go through all the stuff if we need to avoid the ground
    if (groundAvoidNeeded)
        return;

    int mergeTime = -1 * (int)(SimLibElapsedTime + 2);

    /*-------------------*/
    /* bail if no target */
    /*-------------------*/
    if (targetPtr == NULL)
    {
        wvrCurrTactic = WVR_NONE;
#ifdef MANEUVER_DEBUG
        strcpy(tmpchr, "Wvr NoTarget");

        if (g_nShowDebugLabels bitand 0x10)
        {
            if (g_nShowDebugLabels bitand 0x8000)
            {
                if (((AircraftClass*) self)->af->GetSimpleMode())
                    strcat(tmpchr, " SIMP");
                else
                    strcat(tmpchr, " COMP");
            }

            if (self->drawPointer)
                ((DrawableBSP*)self->drawPointer)->SetLabel(tmpchr, ((DrawableBSP*)self->drawPointer)->LabelColor());
        }

#endif
        return;
    }

    // do we need to evaluate our position?
    if (SimLibElapsedTime > wvrTacticTimer or wvrCurrTactic == WVR_NONE)
    {
        // 2002-02-09 ADDED BY S.G. Change our radarMode to digiSTT if we have been detected and we have a radar to start with

        // Look up intercept type for all A/C
        if (targetPtr->BaseData()->IsAirplane() or targetPtr->BaseData()->IsFlight() or targetPtr->BaseData()->IsHelicopter())
        {
            // Find the data table for these two types of A/C
            hisCombatClass = targetPtr->BaseData()->CombatClass(); // 2002-02-26 MODIFIED BY S.G. Removed the AircraftClass cast

            // 2002-03-27 MN CTD fix - hisCombatClass is 999 in case of a FalcEnt. Need to add CombatClass() return call to helo.h
            F4Assert(hisCombatClass < NumMnvrClasses);

            if (hisCombatClass >= NumMnvrClasses)
                hisCombatClass = NumMnvrClasses - 1;

            theIntercept = &(maneuverData[myCombatClass][hisCombatClass]);

            numChoices = theIntercept->numMerges;

            if (numChoices)
            {
                myChoice =   rand() % numChoices;// changed back WvrMergeUnlimited;// me123 status test. lets do smart merge for now.

                switch (theIntercept->merge[myChoice])
                {
                    case WvrMergeHitAndRun:
                        mergeTime = 15 * SEC_TO_MSEC;
                        break;

                    case WvrMergeLimited:
                        mergeTime = 60 * SEC_TO_MSEC;
                        break;

                    case WvrMergeUnlimited:
                        mergeTime = -1 * (int)(SimLibElapsedTime + 2);
                        break;
                }
            }
        }

        // run logic for next tactic
        WvrChooseTactic();

        // When do we get out of the fight
        if (SimLibElapsedTime > engagementTimer)
        {
            engagementTimer = SimLibElapsedTime + mergeTime;
        }

        // chooose next time to check
        // based on current tactic
        switch (wvrCurrTactic)
        {
            case WVR_RANDP:
                PrintWVRMode("DIGI WVR RANDP\n");
                wvrTacticTimer = SimLibElapsedTime + 1000;//me123 status test chenged from 10
                break;

            case WVR_ROOP:
                PrintWVRMode("DIGI WVR ROOP\n");
                wvrTacticTimer = SimLibElapsedTime + 250;//me123 from 5000
                break;

            case WVR_OVERB:
                PrintWVRMode("DIGI WVR OVERB\n");
                wvrTacticTimer = SimLibElapsedTime + 3000;//me123 from 5000
                break;

            case WVR_GUNJINK:
                PrintWVRMode("DIGI WVR GUN JINK\n");
                wvrTacticTimer = SimLibElapsedTime + 500;//me123 status test chenged from 6
                break;

            case WVR_STRAIGHT:
                // note: we'll likely want to set how long we go straight
                // for by pilot skill level
                PrintWVRMode("DIGI WVR STRAIGHT\n");
                wvrTacticTimer = SimLibElapsedTime + 1500;

                if (lastTactic not_eq wvrCurrTactic)
                {
                    if (af->theta > 0.0F)
                        holdAlt = max(5000.0F, -self->ZPos());
                    else
                        holdAlt = 0.0F;
                }

                break;

            case WVR_AVOID:
                PrintWVRMode("DIGI WVR AVOID\n");
                wvrTacticTimer = SimLibElapsedTime + 1500;

                if (lastTactic not_eq wvrCurrTactic)
                {
                    holdAlt = max(5000.0F, -self->ZPos());
                }

                holdPsi = targetPtr->BaseData()->Yaw() + 180.0F * DTR;

                if (holdPsi > 180.0F * DTR)
                    holdPsi -= 360.0F * DTR;

                break;

            case WVR_BEAM_RETURN:
                if (lastTactic not_eq wvrCurrTactic)
                {
                    PrintWVRMode("DIGI WVR BEAM RETURN\n");
                    wvrTacticTimer = SimLibElapsedTime + 10000;
                    holdAlt = max(5000.0F, -self->ZPos());

                    // Find left and right beam angles
                    // Note: Yaw is 0 - 359.9999
                    left  = targetPtr->BaseData()->Yaw() - 90.0F * DTR;
                    right = targetPtr->BaseData()->Yaw() + 90.0F * DTR;

                    // Normalize
                    if (right > 180.0F * DTR)
                        right -= 360.0F * DTR;

                    if (left > 180.0F * DTR)
                        left -= 360.0F * DTR;

                    cur = self->Yaw();

                    if (cur >  180.0F * DTR)
                        cur -= 360.0F * DTR;

                    // Go to the closer
                    if (fabs(left - cur) < fabs(right - cur))
                    {
                        holdPsi = left;
                    }
                    else
                    {
                        holdPsi = right;
                    }
                }

                break;

            case WVR_BEAM:
                PrintWVRMode("DIGI WVR BEAM\n");
                wvrTacticTimer = SimLibElapsedTime + 1500;

                if (lastTactic not_eq wvrCurrTactic)
                {
                    holdAlt = max(5000.0F, -self->ZPos());
                }

                // Find left and right beam angles
                // Note: Yaw is 0 - 359.9999
                left  = targetPtr->BaseData()->Yaw() - 90.0F * DTR;
                right = targetPtr->BaseData()->Yaw() + 90.0F * DTR;

                // Normalize
                if (right > 180.0F * DTR)
                    right -= 360.0F * DTR;

                if (left > 180.0F * DTR)
                    left -= 360.0F * DTR;

                cur = self->Yaw();

                if (cur >  180.0F * DTR)
                    cur -= 360.0F * DTR;

                // Go to the closer
                if (fabs(left - cur) < fabs(right - cur))
                {
                    holdPsi = left;
                }
                else
                {
                    holdPsi = right;
                }

                break;

            case WVR_BUGOUT:
                PrintWVRMode("DIGI WVR BUGOUT\n");
                wvrTacticTimer = SimLibElapsedTime + 5000;

                if (lastTactic not_eq wvrCurrTactic)
                {
                    holdAlt = max(5000.0F, -self->ZPos());
                }

                // Find a heading directly away from the target
                cur = TargetAz(self, targetPtr);

                if (cur > 0.0F)
                    cur = self->Yaw() - (180.0F * DTR - cur);
                else
                    cur = self->Yaw() - (-180.0F * DTR - cur);

                // Normalize
                if (cur >  180.0F * DTR)
                    cur -= 360.0F * DTR;

                holdPsi = cur;
                break;

            default:
                PrintWVRMode("DIGI NO MODE\n");
                wvrTacticTimer = SimLibElapsedTime + 500;//me123 from 5000
        }
    }


    // run the tactic
    switch (wvrCurrTactic)
    {
        case WVR_RANDP:
            RollAndPull();
#ifdef MANEUVER_DEBUG
            sprintf(tmpchr, "Wvr Roll&Pull");
#endif
            break;

        case WVR_ROOP:
            WvrRollOutOfPlane();
#ifdef MANEUVER_DEBUG
            sprintf(tmpchr, "Wvr RollOutOfPlane");
#endif
            break;

        case WVR_BUGOUT:
            WvrBugOut();
#ifdef MANEUVER_DEBUG
            sprintf(tmpchr, "Wvr Bugout");
#endif
            break;

        case WVR_STRAIGHT:
            WvrStraight();
#ifdef MANEUVER_DEBUG
            sprintf(tmpchr, "Wvr straight");
#endif
            break;

        case WVR_GUNJINK:
            WvrGunJink();
#ifdef MANEUVER_DEBUG
            sprintf(tmpchr, "Wvr Gunjink");
#endif
            break;

        case WVR_OVERB:
            WvrOverBank(45.0f * DTR);
#ifdef MANEUVER_DEBUG
            sprintf(tmpchr, "Wvr overbank");
#endif
            break;

        case WVR_AVOID:
            WvrAvoid();
#ifdef MANEUVER_DEBUG
            sprintf(tmpchr, "Wvr avoid");
#endif
            break;

        case WVR_BEAM:
            WvrBeam();
#ifdef MANEUVER_DEBUG
            sprintf(tmpchr, "Wvr beam");
#endif
            break;

        case WVR_BEAM_RETURN:
            if (SimLibElapsedTime > wvrTacticTimer)
            {
                RollAndPull();
#ifdef MANEUVER_DEBUG
                sprintf(tmpchr, "Wvr RandP");
#endif
            }
            else
            {
                WvrBeam();
#ifdef MANEUVER_DEBUG
                sprintf(tmpchr, "Wvr Beam");
#endif
            }

            break;

        default:
            RollAndPull();
#ifdef MANEUVER_DEBUG
            sprintf(tmpchr, "Wvr Roll&Pull");
#endif
            break;
    }

    // last tactic executed
    wvrPrevTactic = wvrCurrTactic;
#ifdef MANEUVER_DEBUG

    if (g_nShowDebugLabels bitand 0x10)
    {
        if (g_nShowDebugLabels bitand 0x50) // add radar mode
        {
            RadarClass* theRadar = (RadarClass*)FindSensor(self, SensorClass::Radar);

            if (theRadar)
            {
                if (theRadar->digiRadarMode = RadarClass::DigiSTT)
                    strcat(tmpchr, " STT");
                else if (theRadar->digiRadarMode = RadarClass::DigiSAM)
                    strcat(tmpchr, " SAM");
                else if (theRadar->digiRadarMode = RadarClass::DigiTWS)
                    strcat(tmpchr, " TWS");
                else if (theRadar->digiRadarMode = RadarClass::DigiRWS)
                    strcat(tmpchr, " RWS");
                else if (theRadar->digiRadarMode = RadarClass::DigiOFF)
                    strcat(tmpchr, " OFF");
                else strcat(tmpchr, " UNKNOWN");
            }
        }

        if (g_nShowDebugLabels bitand 0x8000)
        {
            if (((AircraftClass*) self)->af->GetSimpleMode())
                strcat(tmpchr, " SIMP");
            else
                strcat(tmpchr, " COMP");
        }

        if (self->drawPointer)
            ((DrawableBSP*)self->drawPointer)->SetLabel(tmpchr, ((DrawableBSP*)self->drawPointer)->LabelColor());
    }

#endif

}

/*
** Name: WvrChooseTactic
** Description:
** Try and make some not completely idiotic decisions....
*/
void DigitalBrain::WvrChooseTactic(void)
{
    int myCombatClass = self->CombatClass();
    int hisCombatClass;
    ManeuverChoiceTable *theIntercept;
    int aceAvoid = FALSE;

    if ( not IsSetATC(AceGunsEngage) and SkillLevel() >= 3)
    {
        if (maxAAWpnRange > 0 and maxAAWpnRange < 1.0F * NM_TO_FT)
            aceAvoid = TRUE;
    }

    //TJL 12/06/03 Change to targetPtr
    //if (threatPtr)
    if (targetPtr)
    {
        // Look up intercept type for all A/C
        //    if (targetPtr->BaseData()->IsSim() and targetPtr->BaseData()->IsAirplane())
        if (targetPtr->BaseData()->IsAirplane() or targetPtr->BaseData()->IsFlight() or targetPtr->BaseData()->IsHelicopter()) // 2002-03-05 MODIFIED BY S.G. airplane, choppers and fligth are ok in here (choppers only makes it here if it passed the SensorFusion test first)
        {
            // Find the data table for these two types of A/C
            hisCombatClass = targetPtr->BaseData()->CombatClass(); // 2002-02-26 MODIFIED BY S.G. Removed the AircraftClass cast
            theIntercept = &(maneuverData[myCombatClass][hisCombatClass]);

            // No intercepts or Choose not to, or no weapons, or Guns only and ACE in campaign
            if (theIntercept->numIntercepts == 0 or
                theIntercept->intercept[0] == BvrNoIntercept or
                maxAAWpnRange == 0 or aceAvoid)
            {
                // Can't go offensive, should we be defensive, or just keep running?
                if (targetData->range > 1.5F * NM_TO_FT or targetData->ataFrom > 25.0F * DTR)
                    wvrCurrTactic = WVR_BUGOUT;
                else
                    wvrCurrTactic = WVR_GUNJINK;

                // Check for reload
                WeaponSelection();
            }
            else
            {
                // hmm this beaming stuff should be bwr tactics, we dont' do that when at visual range
                // it takes place outside 10nm which is not within this rutine's paremeters so it's kinda disregarded
                if (targetData->range > 25.0F * NM_TO_FT)
                    wvrCurrTactic = WVR_AVOID;
                else if (targetData->range > 15.0F * NM_TO_FT)
                    wvrCurrTactic = WVR_BEAM ;//me123 status test chenged from WVR_BEAM_RETURN
                else if (targetData->range > 10.0F * NM_TO_FT and wvrCurrTactic not_eq WVR_BEAM_RETURN)//me123 status test chenged from >3
                {
                    wvrCurrTactic = WVR_BEAM_RETURN;
                }
                else
                {
                    wvrCurrTactic = WVR_RANDP;
                }
            }
        }
        else
        {
            wvrCurrTactic = WVR_AVOID;
        }
    }
    else

        /*-----------------------------*/
        /* logic is geometry dependent */
        /*-----------------------------*/
        //ME123 WE DEFINATLY NEED TO THINK ABOUT NOSE TO NOSE OR NOSE TO TAIL FIGHT HERE 
        // AT THE MOMENT WE JUST ROLL AND PULL NOMATTER WHAT :-(


        // we are pointing at him and him at us and we are too slow
        // so let's exploit that we are not emidiatly threatened and get some energy
        //TJL 12/06/03 Appears someone forgot a * DTR
        //if (targetData->ata <= 90.0F * DTR and targetData->ataFrom <= 90.0F)
        if (targetData->ata <= 90.0F * DTR and targetData->ataFrom <= 90.0F * DTR)
        {
            // how stupid are we?
            // MODIFIED BY S.G. af->vt is in feet/second. cornerSpeed is in knot/hour
            // NEEDS TO BE DONE IN THE 1.08 EXE FIRST TO BE CONSISTANT
            if (af->vt < cornerSpeed * 0.3F)
                //      if ( self->GetKias() < cornerSpeed * 0.3F )
            {
                wvrCurrTactic = WVR_STRAIGHT;
            }
            /*--------------*/
            /* me -> <- him */
            /*--------------*/
            else if (targetData->ataFrom <= 90.0F * DTR)
            {
                wvrCurrTactic = WVR_RANDP;
            }
            /*--------------*/
            /* me -> him -> */
            /*--------------*/
            else if (targetData->ataFrom >= 90.0F * DTR)
            {
                wvrCurrTactic = WVR_RANDP;
            }
        }
    /*--------------*/
    /* him -> me -> */
    /*--------------*/
        else if (targetData->ata > 90.0F * DTR)
        {
            if (self->GetKias() < cornerSpeed * 0.6f)  //me123 status test. accelerate if we are below 210cas changed from 0.3
            {
                wvrCurrTactic = WVR_RANDP;//me123 WVR_STRAIGHT;
            }
            else
            {
                wvrCurrTactic = WVR_RANDP;
            }
        }
}

void DigitalBrain::WvrRollOutOfPlane(void)
{
    float eroll;

    /*-----------------------*/
    /* first pass, save roll */
    /*-----------------------*/
    if (wvrPrevTactic not_eq WVR_ROOP)
    {
        /*----------------------------------------------------*/
        /* want to roll toward the vertical but limit to keep */
        /* droll < 45 degrees.                                */
        /*----------------------------------------------------*/
        if (self->Roll() >= 0.0)
        {
            newroll = self->Roll() - 45.0F * DTR; //me123 from 45. smaller repossition
        }
        else
        {
            newroll = self->Roll() + 45.0F * DTR; //me123 from 45. smaller repossition
        }
    }

    /*------------*/
    /* roll error */
    /*------------*/
    eroll = newroll - self->Roll();

    /*-----------------------------*/
    /* roll the shortest direction */
    /*-----------------------------*/
    /*
    if (eroll < -180.0F*DTR)
       eroll += 360.0F*DTR;
    else if (eroll > 180.0F*DTR)
       eroll -= 360.0F*DTR;
    */

    // me123 hmm nonono we are too fast that's why we need to do this  MachHold(cornerSpeed, self->GetKias(), TRUE);
    MachHold(150.0f, self->GetKias(), TRUE);
    SetPstick(af->GsAvail(), maxGs, AirframeClass::GCommand);
    SetRstick(eroll * RTD);

}

void DigitalBrain::WvrOverBank(float delta)
{
    float eroll;

    //-----------------------*/
    // Find a new roll angle */
    // relative to target
    //-----------------------*/
    if (wvrPrevTactic not_eq WVR_OVERB)
    {
        if (self->Roll() > 0.0F)
            newroll = targetData->droll + delta;
        else
            newroll = targetData->droll - delta;

        if (newroll > 180.0F * DTR)
            newroll -= 360.0F * DTR;
        else if (newroll < -180.0F * DTR)
            newroll += 360.0F * DTR;
    }

    eroll = newroll - self->Roll();

    MachHold(cornerSpeed, self->GetKias(), TRUE);
    SetRstick(eroll * RTD);
    SetPstick(af->GsAvail(), maxGs, AirframeClass::GCommand);
}




/*
** Name: WvrStraight
** Description:
** Level out and fly straight at high speed.
*/
void DigitalBrain::WvrStraight(void)
{
    if (holdAlt < 0.0F)
        AltitudeHold(holdAlt);
    else
        pStick = 0.0F;

    MachHold(2.0F * cornerSpeed, self->GetKias(), TRUE);

    // If alpha > alphamax/2 unload to accel
    if (af->alpha > 15.0F)
        SetPstick(-1.0F, maxGs, AirframeClass::GCommand);
}


/*
** Name: WvrAvoid
** Description:
** Try to fly around the threat
*/
void DigitalBrain::WvrAvoid(void)
{
    HeadingAndAltitudeHold(holdPsi, holdAlt);
    MachHold(2.0F * cornerSpeed, self->GetKias(), TRUE);
}


/*
** Name: WvrBeam
** Description:
** Keep aspect at 90
*/
void DigitalBrain::WvrBeam(void)
{
    HeadingAndAltitudeHold(holdPsi, holdAlt);
    MachHold(2.0F * cornerSpeed, self->GetKias(), TRUE);

    // We be chaff'n and flare'n
    if (((AircraftClass*)self)->HasPilot())
    {
        if (SimLibElapsedTime > self->ChaffExpireTime() + 500)
        {
            ((AircraftClass*)self)->dropChaffCmd = TRUE;
        }

        if (SimLibElapsedTime > self->FlareExpireTime() + 500)
        {
            ((AircraftClass*)self)->dropFlareCmd = TRUE;
        }
    }
}


/*
** Name: WvrBugOut
** Description:
** Level out and fly straight at high speed.
** Head for the hills
*/
void DigitalBrain::WvrBugOut(void)
{
    HeadingAndAltitudeHold(holdPsi, holdAlt);
    MachHold(2.0F * cornerSpeed, self->GetKias(), TRUE);
}


/*
** Name: Guns Jink
** Description:
** Jink around
*/
void DigitalBrain::WvrGunJink(void)
{
    GunsJink();
}


/*
** Name: SetThreat
** Description:
** Creates a SimObjectType struct for the entity, sets the threatPtr,
** References the target.  Any pre-existing target is dereferenced.
*/
void DigitalBrain::SetThreat(FalconEntity *obj)
{
    short edata[6];
    int randNum, response;

    // don't pre-empt current threat with a new threat until we've been
    // dealing with the threat for a while unless the threat is NULL
    if (obj not_eq NULL and threatTimer > 0.0f)
        return;

    F4Assert( not obj or not obj->IsSim() or not obj->IsHelicopter());

    if (obj and obj->OnGround())//Cobra We want to nail those targeting us
    {
        SetGroundTarget(obj);
        return;
    }

    if (obj and not obj->OnGround())
    {
        // if the threat is the same as our target, we don't
        // need to do anything since we're already dealing
        // with it
        if (targetPtr and targetPtr->BaseData() == obj)
        {
            return;
        }

        // create new target data and reference it
#ifdef DEBUG
        //threatPtr = new SimObjectType( OBJ_TAG, self, obj );
#else
        threatPtr = new SimObjectType(obj);
#endif
        threatTimer = 10.0f;
        SetTarget(threatPtr);

        // Occasionally send a radio call stating defensive or ask for help
        randNum = rand() % 100;

        if (randNum < 5)
        {
            // Yell for help
            edata[0] = ((FlightClass*)self->GetCampaignObject())->callsign_id;
            edata[1] = (((FlightClass*)self->GetCampaignObject())->callsign_num - 1) * 4 + self->GetCampaignObject()->GetComponentIndex(self) + 1;
            AiMakeRadioResponse(self, rcHELPNOW, edata);
        }
        else if (randNum < 40)
        {
            //Inform flight
            if (threatPtr->localData->range > 2.0F * NM_TO_FT)
            {
                if (PlayerOptions.BullseyeOn())
                {
                    response = rcENGDEFENSIVEA;
                }
                else
                {
                    response = rcENGDEFENSIVEB;
                }

                edata[0] = ((FlightClass*)self->GetCampaignObject())->callsign_id;
                edata[1] = (((FlightClass*)self->GetCampaignObject())->callsign_num - 1) * 4 + isWing;
                edata[2] = (short) SimToGrid(threatPtr->BaseData()->YPos());
                edata[3] = (short) SimToGrid(threatPtr->BaseData()->XPos());
                edata[4] = (short) threatPtr->BaseData()->ZPos();
            }
            else
            {
                edata[0] = isWing;
                response = rcENGDEFENSIVEC;
            }

            // Send the message
            AiMakeRadioResponse(self, response, edata);
        }

        threatPtr = NULL;
    }
}

// 2002-03-11 MODIFIED BY S.G. Too many changes to track them all, rewrite it...
#if 1
int CanEngage(AircraftClass *self, int combatClass, SimObjectType* targetPtr, int type)
{
    int hisCombatClass;
    DigitalBrain::ManeuverChoiceTable *theIntercept;
    int retBvr = TRUE;
    int retWvr = TRUE;

    // Check for aircraft, choppers or flights
    if (targetPtr->BaseData()->IsAirplane() or targetPtr->BaseData()->IsFlight() or targetPtr->BaseData()->IsHelicopter())
    {
        // If asked to use the new code, then honor the request
        if (g_bUseNewCanEnage)
        {
            // Find the data table for these two types of flying objects
            // But don't assume you know it, see if id'ed first...
            CampBaseClass *campBaseObj;

            if (targetPtr->BaseData()->IsSim())
                campBaseObj = ((SimBaseClass*)targetPtr->BaseData())->GetCampaignObject();
            else
                campBaseObj = ((CampBaseClass *)targetPtr->BaseData());

            // If it doesn't have a campaign object or it's identified... END OF ADDED SECTION plus the use of campBaseObj below
            if ( not campBaseObj or campBaseObj->GetIdentified(self->GetTeam()))
            {
                // Yes, now you can get its combat class
                hisCombatClass = targetPtr->BaseData()->CombatClass();
            }
            else
            {
                //TJL Combatclass numbers are kinda hosed, assume the worst at all times if you can't determine
                // No :-( Then guestimate it... (from RIK's BVR code)
                hisCombatClass = 4;


                /*if ((targetPtr->BaseData()->GetVt() * FTPSEC_TO_KNOTS > 300.0f or targetPtr->BaseData()->ZPos() < -10000.0f))  {
                 //this might be a combat jet.. asume the worst
                 hisCombatClass = 4;
                }
                else if (targetPtr->BaseData()->GetVt() * FTPSEC_TO_KNOTS > 250.0f) {
                 // this could be a a-a capable thingy, but if it's is it's low level so it's a-a long range shoot capabilitys are not great
                 hisCombatClass = 1;
                }
                else {
                 // this must be something unthreatening...it's below 250 knots but it's still unidentified so...
                 hisCombatClass = 0;
                }*/
            }
        }
        else
            hisCombatClass = targetPtr->BaseData()->CombatClass();

        theIntercept = &(DigitalBrain::maneuverData[combatClass][hisCombatClass]);

        if (type bitand DigitalBrain::WVRManeuver)
        {
            // If no capability, don't go say you can engage
            if (theIntercept->numMerges == 0)
                retWvr = FALSE;
            else if (theIntercept->numMerges == 1)
            {
                // Need to be real close for a hit and run
                if (theIntercept->merge[0] == DigitalBrain::WvrMergeHitAndRun and 
                    targetPtr->localData->ata > 45.0F * DTR)
                {
                    retWvr = FALSE;
                }
                // Can't be behind you for limited
                else if (theIntercept->merge[0] == DigitalBrain::WvrMergeLimited and 
                         targetPtr->localData->ata > 90.0F * DTR)
                {
                    retWvr = FALSE;
                }
            }
        }
        else
            retWvr = FALSE;

        // Check for intercepts if in BVR...
        if (type bitand DigitalBrain::BVRManeuver)
        {
            if (theIntercept->numIntercepts == 0)
                retBvr = FALSE;
        }
        else
            retBvr = FALSE;

        // Fail safe
        if (type == 0)
        {
            retWvr = FALSE;
            retBvr = FALSE;
        }
    }
    // 2002-03-11 ADDED BY S.G. If it's not even an air thingy, why do you say you can engage it???
    else
    {
        retWvr = FALSE;
        retBvr = FALSE;
    }

    return retBvr bitor retWvr;
}

#else

int CanEngage(int combatClass, SimObjectType* targetPtr)
{
    int hisCombatClass;
    DigitalBrain::ManeuverChoiceTable *theIntercept;
    int retval = TRUE; // Assume you can engage

    // Only check for A/C
    // if (targetPtr->BaseData()->IsSim() and targetPtr->BaseData()->IsAirplane())
    if (targetPtr->BaseData()->IsAirplane() or targetPtr->BaseData()->IsFlight() or targetPtr->BaseData()->IsHelicopter()) // 2002-03-05 MODIFIED BY S.G. airplane, choppers and fligth are ok in here (choppers only makes it here if it passed the SensorFusion test first)
    {
        // Find the data table for these two types of A/C
        hisCombatClass = targetPtr->BaseData()->CombatClass(); // 2002-02-26 MODIFIED BY S.G. Removed the AircraftClass cast
        theIntercept = &(DigitalBrain::maneuverData[combatClass][hisCombatClass]);

        if (theIntercept->numMerges == 1)
        {
            // Need to be real close for a hit and run
            if (theIntercept->merge[0] == DigitalBrain::WvrMergeHitAndRun and 
                targetPtr->localData->ata > 45.0F * DTR)
            {
                retval = FALSE;
            }
            // Can't be behind you for limited
            else if (theIntercept->merge[0] == DigitalBrain::WvrMergeLimited and 
                     targetPtr->localData->ata > 90.0F * DTR)
            {
                retval = FALSE;
            }
        }
    }

    return retval;
}
#endif
