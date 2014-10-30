#include "stdhdr.h"
#include "classtbl.h"
#include "digi.h"
#include "object.h"
#include "simbase.h"
#include "Entity.h"
#include "aircrft.h"
#include "airframe.h"
#include "fakerand.h"
#include "wingorder.h"
#include "unit.h"
#include "simdrive.h"
#include "otwdrive.h"
#include "radar.h"//me123
#include "flight.h"//me123
#include "Graphics/Include/tmap.h"

/* S.G. NEED TO KNOW WHICH WEAPON WE FIRED */
#include "Missile.h"
#include "Fcc.h"//me123
#include "sms.h"//cobra
#define MANEUVER_DEBUG // MNLOOK
#ifdef MANEUVER_DEBUG
#include "Graphics/include/drawbsp.h"
extern int g_nShowDebugLabels;
#endif

extern bool g_bUseAggresiveIncompleteA2G; // 2002-03-22 S.G.
extern float g_fHotNoseAngle; // 2002-03-22 S.G.
extern float g_fMaxMARNoIdA; // 2002-03-22 S.G.
extern float g_fMinMARNoId5kA; // 2002-03-22 S.G.
extern float g_fMinMARNoId18kA; // 2002-03-22 S.G.
extern float g_fMinMARNoId28kA; // 2002-03-22 S.G.
extern float g_fMaxMARNoIdB; // 2002-03-22 S.G.
extern float g_fMinMARNoId5kB; // 2002-03-22 S.G.
extern float g_fMinMARNoId18kB; // 2002-03-22 S.G.
extern float g_fMinMARNoId28kB; // 2002-03-22 S.G.
extern float g_fMinMARNoIdC; // 2002-03-22 S.G.


//#define DEBUG_INTERCEPT
int CanEngage(AircraftClass *self, int combatClass, SimObjectType* targetPtr, int type);  // 2002-03-11 MODIFIED BY S.G. Added the 'type' parameter
FalconEntity* SpikeCheck(AircraftClass* self, FalconEntity *byHim = NULL, int *data = NULL); // 2002-02-10 S.G.

/* Check for Entry/Exit condition into WVR */
void DigitalBrain::BvrEngageCheck(void)
{
    Falcon4EntityClassType *classPtr;
    float engageRange;
    radModeSelect = 3;//Default

    /*---------------------*/
    /* return if no target */
    /*---------------------*/
    if (targetPtr == NULL or curMode == RTBMode or /* 2002-04-01 ADDED BY S.G. Player's wing doing a maneuver */ mpActionFlags[AI_EXECUTE_MANEUVER])/*or // No Target
      ( not mpActionFlags[AI_ENGAGE_TARGET] and missionClass not_eq AAMission and not missionComplete) or // Target is not assigned and on AG mission
       curMode == RTBMode)*/
    {
        bvrCurrProfile = Pnone;

        //if ((AircraftClass*)flightLead)
        if ((AircraftClass*)flightLead and bvractionstep not_eq 0) //THW 2003-11-15 Only calc if necessary
        {
            if (((AircraftClass*)flightLead)->DBrain()->bvractionstep == 0 and self->GetCampaignObject()->NumberOfComponents() < 3 or
                ((AircraftClass*)flightLead)->DBrain()->bvractionstep == 0 and (AircraftClass *)self->GetCampaignObject() and 
                (AircraftClass *)self->GetCampaignObject()->GetComponentNumber(2) and 
                ((AircraftClass *)self->GetCampaignObject()->GetComponentNumber(2))->DBrain()->bvractionstep == 0)
            {
                bvractionstep = 0;
            }
        }

        return;
    }

    // RV - Biker - Looks like AceGunsEngage always set...
    //if (maxAAWpnRange <= 1.0F * NM_TO_FT and not IsSetATC(AceGunsEngage))
    if (maxAAWpnRange <= 1.0F * NM_TO_FT and missionClass == AGMission)
    {
        if (targetPtr->localData->range < 2.0F * NM_TO_FT and not self->Sms->DidEmergencyJettison())
        {
            if (self->CombatClass() not_eq MnvrClassBomber)
            {
                if (rand() % 100 < SkillLevel() * 25)
                {
                    self->Sms->EmergencyJettison();
                    SelectGroundWeapon();
                }
                else if (rand() bitand 1)
                {
                    self->Sms->AGJettison();
                    SelectGroundWeapon();
                }
            }
        }

        return;
    }

    if (maxAAWpnRange * 1.3F/*ME123 ADDET 1.3*/ < 45.0F * NM_TO_FT)
        engageRange = 45.0F * NM_TO_FT;
    else
        engageRange = maxAAWpnRange * 1.3F/*ME123 ADDET 1.3*/;

    engageRange = min(engageRange, maxEngageRange);  // DON'T GO FURTHER THEN WHAT THE MISSION ALLOWS US

    // 2002-02-27 ADDED BY S.G. If on a A2G mission, special consideration here...
    if (mpActionFlags[AI_ENGAGE_TARGET] not_eq AI_AIR_TARGET and missionClass not_eq AAMission and not missionComplete)
    {

        // not assigned a target, on a A2G mission that is not over yet...  // 2002-03-04 MODIFIED BY S.G. Use new enum type
        // If we have a ground target selected and doing a ground attack, stick to it unless threatened

        //Cobra to the rescue ;) threatPtr is working as expected
        //We will try and let A/G guys respond in a limited way so as to not blindly ignore
        //obvious threats
        if (groundTargetPtr and agDoctrine not_eq AGD_NONE /* and not threatPtr*/)
        {
            if (targetPtr->localData->range > 8.0f * NM_TO_FT)
            {
                return;
            }
            else if ((targetPtr->localData->range > 6.0F * NM_TO_FT
                     and (fabs(targetPtr->localData->ata) > 110 * DTR)))
            {
                return;
            }

            //we made it here so dump A/G stores
            if (targetPtr->localData->range < 2.0F * NM_TO_FT)
            {
                self->Sms->EmergencyJettison();
                SelectGroundWeapon();
            }
        }



        // 2002-03-04 ADDED BY S.G. Addition to the addition :-) On A2G, don't bother attacking what can't (won't) attack us
        /*    // I think this is not required because SensorFusion will screen them out based on mission type and enemy plane combat type
            if (SkillLevel() > 0) { // Recruit attacks all that moves
            CampBaseClass *campBaseObj;
            if (targetPtr->BaseData()->IsSim())
            campBaseObj = ((SimBaseClass*)targetPtr->BaseData())->GetCampaignObject();
            else
            campBaseObj = ((CampBaseClass *)targetPtr->BaseData());

            // If it has a campaign object and it's identified...
            if (campBaseObj and campBaseObj->GetIdentified(self->GetTeam())) {
            if (targetPtr->BaseData()->CombatClass() >= MnvrClassA10)
            return; // Don't bother attacking non treathning aircrafts...

            }
            }
        */    // END OF ADDED SECTION 2002-03-04

        /*if (SkillLevel() > 1) {
           // For rookies and up, if our target is far,
           if (targetPtr->localData->range > 15.0F * NM_TO_FT) {
            // Ignore if behind us
            if (fabs(targetPtr->localData->ata) > 90.0F)
            return;

            // Or if we have an escort
            if (escortFlightID not_eq FalconNullId) {
            FlightClass *escortFlight = (FlightClass *)vuDatabase->Find(escortFlightID);
            if (escortFlight and not escortFlight->IsDead())
            return; // We have an alive escort so concentrate on the task at hand...
            }
           }
        }
        else {
           // For recruits and cadets, only ignore the target if it's far and behind them
           if (targetPtr->localData->range > 15.0F * NM_TO_FT and fabs(targetPtr->localData->ata) > 90.0F * DTR)
            return;
        }*/

    }

    // END OF ADDED SECTION 2002-02-27

    /*-------*/
    /* entry */
    /*-------*/
    if (curMode not_eq BVREngageMode)
    {
        /*--------------------------------*/
        /* check against threshold values */
        /*--------------------------------*/
        classPtr = (Falcon4EntityClassType*)(targetPtr->BaseData()->EntityType());

        // if its a plane we're in.....
        if ((classPtr->vuClassData.classInfo_[VU_TYPE] == TYPE_AIRPLANE or
             classPtr->vuClassData.classInfo_[VU_TYPE] == TYPE_HELICOPTER) and 
            targetPtr->localData->range < engageRange and self->CombatClass() <= 7
            /*CanEngage(self, self->CombatClass(), targetPtr, BVRManeuver)*/) // 2002-03-11 MODIFIED BY S.G. Added parameter BVRManeuver
        {
            AddMode(BVREngageMode);
        }
    }
    else if (targetPtr->localData->range > 1.2F * engageRange or //1,2 from 1,5
             /* not CanEngage(self, self->CombatClass(), targetPtr, BVRManeuver)*/
             self->CombatClass() > 7) // 2002-03-11 MODIFIED BY S.G. Added parameter BVRManeuver
    {
        ClearTarget();
        ShiAssert(curMode not_eq GunsEngageMode);

        // Clear the current intercept type and timer
        bvrCurrTactic = BvrNoIntercept;
        bvrTacticTimer = SimLibElapsedTime + 1 * CampaignSeconds;
    }
    else
    {
        AddMode(BVREngageMode);
    }
}

void DigitalBrain::BvrEngage(void)
{

    float tfloor, tceil, gainCtrl;

    // 2002-01-27 MN No need to go through all the stuff if we need to avoid the ground
    if (groundAvoidNeeded)
        return;

    /*-------------------*/
    /* bail if no target */
    /*-------------------*/
    if (targetPtr == NULL)
    {
        return;
    }

    WhoIsSpiked(); //sets the timers and spikestatus

#ifdef MANEUVER_DEBUG
    char tmpchr[40];
#endif

    // do we need to evaluate our position?
    if (bvrTacticTimer < SimLibElapsedTime or targetPtr not_eq lastTarget or
        missilelasttime not_eq missileFiredEntity)
    {
        // run logic for next tactic
        BvrChooseTactic();
    }

    missilelasttime = missileFiredEntity;

    // Execute selected intercept type
    af->SetSimpleMode(SIMPLE_MODE_OFF);  //me123 make sure we are out of simple mode

    switch (bvrCurrTactic)
    {
        case BvrPump:
            DragManeuver();
#ifdef MANEUVER_DEBUG
            sprintf(tmpchr, "%s", "BvrDrag");
#endif

            break;

        case BvrCrank:
            CrankManeuver(0, 0);
#ifdef MANEUVER_DEBUG
            sprintf(tmpchr, "%s", "BvrCrank");
#endif

            break;

        case BvrCrankRight:
            CrankManeuver(offRight, 0);
#ifdef MANEUVER_DEBUG
            sprintf(tmpchr, "%s", "BvrCrank");
#endif

            break;

        case BvrCrankLeft:
            CrankManeuver(offLeft, 0);
#ifdef MANEUVER_DEBUG
            sprintf(tmpchr, "%s", "BvrCrank");
#endif

            break;

        case BvrCrankHi:
            CrankManeuver(0, 1);
#ifdef MANEUVER_DEBUG
            sprintf(tmpchr, "%s", "BvrCrankHi");
#endif

            break;

        case BvrCrankLo:
            CrankManeuver(0, 2);
#ifdef MANEUVER_DEBUG
            sprintf(tmpchr, "%s", "BvrCrankLo");
#endif

            break;

        case BvrCrankRightHi:
            CrankManeuver(offRight, 2);
#ifdef MANEUVER_DEBUG
            sprintf(tmpchr, "%s", "BvrCrankRightHi");
#endif

            break;

        case BvrCrankRightLo:
            CrankManeuver(offRight, 2);
#ifdef MANEUVER_DEBUG
            sprintf(tmpchr, "%s", "BvrCrankRightLo");
#endif

            break;

        case BvrCrankLeftHi:
            CrankManeuver(offLeft, 2);
#ifdef MANEUVER_DEBUG
            sprintf(tmpchr, "%s", "BvrCrankLeftHi");
#endif

            break;

        case BvrCrankLeftLo:
            CrankManeuver(offLeft, 2);
#ifdef MANEUVER_DEBUG
            sprintf(tmpchr, "%s", "BvrCrankLeftLo");
#endif

            break;

        case BvrNotch:
            BeamManeuver(0, 0);
#ifdef MANEUVER_DEBUG
            sprintf(tmpchr, "%s", "BvrBeam");
#endif

            break;

        case BvrNotchRight:
            BeamManeuver(offRight, 0);
#ifdef MANEUVER_DEBUG
            sprintf(tmpchr, "%s", "BvrBeamright");
#endif

            break;

        case BvrNotchRightHigh:
            BeamManeuver(offRight, 1);
#ifdef MANEUVER_DEBUG
            sprintf(tmpchr, "%s", "BvrBeamright");
#endif

            break;

        case BvrNotchLeft:
            BeamManeuver(offLeft, 0);
#ifdef MANEUVER_DEBUG
            sprintf(tmpchr, "%s", "BvrBeamleft");
#endif

            break;

        case BvrNotchLeftHigh:
            BeamManeuver(offLeft, 1);
#ifdef MANEUVER_DEBUG
            sprintf(tmpchr, "%s", "BvrBeamleft");
#endif

            break;

            //////////////////////////////////////////////////
        case BvrSingleSideOffset://me123 baseline intercept
            BaseLineIntercept();//me123
#ifdef MANEUVER_DEBUG
            sprintf(tmpchr, "%s", "BvrBaseLineIntercept");
#endif

            break;

            //////////////////////////////////////////////////
        case BvrGrind://me123 baseline intercept
            BaseLineIntercept();//me123
#ifdef MANEUVER_DEBUG
            sprintf(tmpchr, "%s", "Grind");
#endif

            break;

            //////////////////////////////////////////////////
        case BvrPince:
            BaseLineIntercept();//me123
            //AiExecPince();
#ifdef MANEUVER_DEBUG
            sprintf(tmpchr, "%s", "BvrPince..BaseLineIntercept");
#endif

            break;

            //////////////////////////////////////////////////
        case BvrPursuit:
            if (0)
            {
                BaseLineIntercept();//me123
#ifdef MANEUVER_DEBUG
                sprintf(tmpchr, "%s", "BvrBaselineIntercept");
#endif
            }
            else
            {
#ifdef MANEUVER_DEBUG
                sprintf(tmpchr, "%s", "BvrPursuit");
#endif
                SetTrackPoint(
                    targetPtr->BaseData()->XPos(),
                    targetPtr->BaseData()->YPos(),
                    min(targetPtr->BaseData()->ZPos() - 100.0F, -4000.0f)
                );
                // make sure we don't plant

                OTWDriver.GetAreaFloorAndCeiling(&tfloor, &tceil);

                if (trackZ > tceil - 1500.0f)
                {
                    gainCtrl = 1.0f;
                    trackZ = tceil - 1500.0f;
                }

                StickandThrottle(-1, trackZ);
            }

#ifdef DEBUG_INTERCEPT
            MonoPrint("BvrPursuit");
#endif
            break;

            /////////////////////////////////////////////
        case  BvrFollowWaypoints:
#ifdef MANEUVER_DEBUG
            sprintf(tmpchr, "%s", "BVRfollowwaypoints");
#endif
            FollowWaypoints();
            break;

        case BvrNoIntercept:
#ifdef DEBUG_INTERCEPT
            MonoPrint("BvrNoIntercept");
#endif
#ifdef MANEUVER_DEBUG
            sprintf(tmpchr, "%s", "BvrNoIntercept");
#endif

            if ( not isWing)
            {
#ifdef MANEUVER_DEBUG
                sprintf(tmpchr, "%s", "followwaypoints");
#endif
                FollowWaypoints();
            }
            else
            {
#ifdef MANEUVER_DEBUG
                sprintf(tmpchr, "%s", "AiFlyBvrFOrmation");
#endif
                AiFlyBvrFOrmation();
            }

            break;

        case BvrFlyFormation:
#ifdef DEBUG_INTERCEPT
            MonoPrint("AiFollowLead");
#endif
            {
#ifdef MANEUVER_DEBUG
                sprintf(tmpchr, "%s", "AiFollowLead");
#endif
                AiFlyBvrFOrmation();
            }
            break;
    }

#ifdef MANEUVER_DEBUG // 2002-03-13 ADDED BY S.G. If you ask me, the following is just for debug so enclose it in a ifdef statement...

    if (Isflightlead and flightLead and ((AircraftClass*)flightLead)->DBrain())
    {
        if (((AircraftClass*)flightLead)->DBrain()->bvrCurrProfile == Pnone)
        {
            sprintf(tmpchr, "%s", "none");
        }
        else if (((AircraftClass*)flightLead)->DBrain()->bvrCurrProfile == Plevel1a)
        {
            sprintf(tmpchr, "%s", "lvl1a");
        }
        else if (((AircraftClass*)flightLead)->DBrain()->bvrCurrProfile == Plevel1b)
        {
            sprintf(tmpchr, "%s", "lvl1b");
        }
        else if (((AircraftClass*)flightLead)->DBrain()->bvrCurrProfile == Plevel1c)
        {
            sprintf(tmpchr, "%s", "lvl1c");
        }
        else if (((AircraftClass*)flightLead)->DBrain()->bvrCurrProfile == Plevel2a)
        {
            sprintf(tmpchr, "%s", "lvl2a");
        }
        else if (((AircraftClass*)flightLead)->DBrain()->bvrCurrProfile == Plevel2b)
        {
            sprintf(tmpchr, "%s", "lvl2b");
        }
        else if (((AircraftClass*)flightLead)->DBrain()->bvrCurrProfile == Plevel2c)
        {
            sprintf(tmpchr, "%s", "lvl2c");
        }
        else if (((AircraftClass*)flightLead)->DBrain()->bvrCurrProfile == Plevel3a)
        {
            sprintf(tmpchr, "%s", "lvl3a");
        }
        else if (((AircraftClass*)flightLead)->DBrain()->bvrCurrProfile == Plevel3b)
        {
            sprintf(tmpchr, "%s", "lvl3b");
        }
        else if (((AircraftClass*)flightLead)->DBrain()->bvrCurrProfile == Plevel3c)
        {
            sprintf(tmpchr, "%s", "lvl3c");
        }
        else if (((AircraftClass*)flightLead)->DBrain()->bvrCurrProfile == Pbeamdeploy)
        {
            sprintf(tmpchr, "%s", "beamdeploy");
        }
        else if (((AircraftClass*)flightLead)->DBrain()->bvrCurrProfile == Pbeambeam)
        {
            sprintf(tmpchr, "%s", "beam");
        }
        else if (((AircraftClass*)flightLead)->DBrain()->bvrCurrProfile == Pwall)
        {
            sprintf(tmpchr, "%s", "wall");
        }
        else if (((AircraftClass*)flightLead)->DBrain()->bvrCurrProfile == Pgrinder)
        {
            sprintf(tmpchr, "%s", "chainsaw");
        }
        else if (((AircraftClass*)flightLead)->DBrain()->bvrCurrProfile == Pwideazimuth)
        {
            sprintf(tmpchr, "%s", "wide az");
        }
        else if (((AircraftClass*)flightLead)->DBrain()->bvrCurrProfile == Pshortazimuth)
        {
            sprintf(tmpchr, "%s", "short az");
        }
        else if (((AircraftClass*)flightLead)->DBrain()->bvrCurrProfile == PwideLT)
        {
            sprintf(tmpchr, "%s", "long LT");
        }
        else if (((AircraftClass*)flightLead)->DBrain()->bvrCurrProfile == PShortLT)
        {
            sprintf(tmpchr, "%s", "shortLT");
        }
        else if (((AircraftClass*)flightLead)->DBrain()->bvrCurrProfile == PDefensive)
        {
            sprintf(tmpchr, "%s", "defensive");
        }
        else if (((AircraftClass*)flightLead)->DBrain()->bvrCurrProfile == PSweep)
        {
            sprintf(tmpchr, "%s", "PSweep");
        }
        else assert(0);
    }

#endif
#ifdef MANEUVER_DEBUG

    if (g_nShowDebugLabels bitand 0x08)
    {
        if (g_nShowDebugLabels bitand 0x40)
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
                    strcat(tmpchr, "%s OFF");
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

    ShiAssert(trackZ < 0.0F);
}

void DigitalBrain::BvrChooseTactic(void)
{
#define no 0
#define prepitbull 1
#define pitbull 2
    int targetstrength = 1;
    int ownstrength = 1;
    bool outranged = false;
    bool outnumbered = false;
    bool outstrengthed = false;

    //CalculateMAR();//Cobra bye bye

    if (targetPtr->BaseData()->IsAirplane() or targetPtr->BaseData()->IsFlight() or targetPtr->BaseData()->IsHelicopter())
    {
        bvrCurrTactic = BvrNoIntercept;
    }

    //Cobra big issues here ... VIU isn't changing when flight members die
    //This screws up BVR behavior
    /*
    if (self->vehicleInUnit == 0)Isflightlead = true;
    if (self->vehicleInUnit == 2)IsElementlead = true;*/
    //This should fix it. :)
    if (self->GetCampaignObject()->GetComponentIndex(self) == 0)
        Isflightlead = true;

    if (self->GetCampaignObject()->GetComponentIndex(self) == 2)
        IsElementlead = true;


    if ( not Isflightlead and not IsElementlead)// wingies default to formationflying
    {
        bvrCurrTactic = BvrFlyFormation;
        bvrTacticTimer = SimLibElapsedTime + 5 * CampaignSeconds;
    }

    DoProfile();

    // OVERIDE SELFDEFENCE  AND MISSILE SUPPORT CODE

    if (IsSupportignmissile())
    {
        bvrCurrTactic = BvrCrank;
        bvrTacticTimer = SimLibElapsedTime + 3 * CampaignSeconds;
    }
    else if (spiked and targetData->range > maxAAWpnRange/* and SkillLevel() >2*/)
    {
        bvrCurrTactic = BvrNotch;
        bvrTacticTimer = SimLibElapsedTime + 20 * CampaignSeconds + SkillLevel() * 5 * CampaignSeconds;
        short edata[3];
        int flightIdx = self->GetCampaignObject()->GetComponentIndex(self);
        edata[0] = flightIdx;
        edata[1] = 0;//notching
        AiMakeRadioResponse(self, rcEXECUTERESPONSE, edata);
    }

}





void DigitalBrain::ChoiceProfile(void)
{

    //Cobra here we pick the best profile
    float myMissile = maxAAWpnRange;//my missile range in feet
    int threatScore = 0;
    int targetstrength = 0;
    int myCombatClass = self->CombatClass();

    if (targetPtr)
    {
        CampBaseClass *campBaseObj;

        if (targetPtr->BaseData()->IsSim())
            campBaseObj = ((SimBaseClass*)targetPtr->BaseData())->GetCampaignObject();
        else
            campBaseObj = (CampBaseClass *)targetPtr->BaseData();

        if (campBaseObj)
            targetstrength = campBaseObj->NumberOfComponents();

        int ownstrength = self->GetCampaignObject()->NumberOfComponents();

        //do I have an inferior missile?
        if (myMissile < 10 * NM_TO_FT)
            threatScore += 20;

        if (myCombatClass == 7 and targetPtr->localData->range > myMissile)
            threatScore += 60;

        //who has the numerical advantage?
        int outnumbered = 0;

        if (targetstrength > ownstrength)
            threatScore += 30;

        //who has height advantage?
        if (targetPtr->BaseData()->ZPos() < self->ZPos())
            threatScore += 5;

        //who has speed advantage
        if (((AircraftClass *)targetPtr->BaseData())->GetKias() > self->GetKias())
            threatScore += 5;

        //who has positional advantage
        //Him -> Me ->
        if (targetPtr->localData->ataFrom < 90 * DTR and targetPtr->localData->ata > 90 * DTR)
            threatScore += 20;

        //is he out of my missile range?
        if (targetPtr->localData->range > myMissile)
            threatScore += 10;

        //special cases
        //Me -> Him ->
        if (targetPtr->localData->ataFrom > 90 * DTR and targetPtr->localData->ata < 90 * DTR)
            threatScore = 5; //we go offensive
    }

    missionType = ((UnitClass*)(self->GetCampaignObject()))->GetUnitMission();

    if (missionType > 10 and not g_bUseAggresiveIncompleteA2G and (IsSetATC(HasAGWeapon) or not missionComplete))
    {
        //We are defensive to protect ourselves
        bvrCurrProfile = PDefensive;
    }
    else //A/A Mission or A/A Capable
    {
        //We will chose a profile based on the situation
        //Defensive Positions
        if (threatScore >= 60)
        {
            bvrCurrProfile = PDefensive;
        }
        else if (threatScore < 60 and threatScore >= 50)
        {
            bvrCurrProfile = Plevel3c;
        }
        else if (threatScore < 50 and threatScore >= 30)
        {
            bvrCurrProfile = Plevel2c;
        }
        else if (threatScore < 30 and threatScore >= 20)
        {
            bvrCurrProfile = Plevel3b;
        }
        else if (threatScore < 20 and threatScore >= 10)
        {
            bvrCurrProfile = Pgrinder;
        }
        else
        {
            bvrCurrProfile = Pwall;
        }

    }//end

    //Cobra log of scores and profiles
    //FILE *deb;
    //deb = fopen("c:\\microprose\\falcon4\\bvr.txt", "a+");
    //fprintf(deb, "Bvrscore %2d  Bvrprofile = %3d\n\n", threatScore, bvrCurrProfile);
    //fclose(deb);



    /*
      Pnone,

      AGGRESIVE MISSILE SUPERIORITY
      Plevel1a,
      Plevel2a,
          Plevel3a,

      DEFENSIVE MISSILE SUPERIORITY
      Plevel1b,
          Plevel2b,
          Plevel3b,

      INFERIOR MISSILES
      Plevel1c,
          Plevel2c,
          Plevel3c,
      OTHER
          Pbeamdeploy,
          Pbeambeam,
          Pwall,
          Pgrinder,
          Pwideazimuth,
          Pshortazimuth,
          PwideLT,
          PShortLT,
      PDefensive*/

    /*switch (((UnitClass*)(self->GetCampaignObject()))->GetUnitMission())
    {
    // medium agresiveness...balanced efford to kill and to survive
    // lead trail and grinding technicks when superoir missiles
     case AMIS_BARCAP:
     case AMIS_BARCAP2:
     case AMIS_HAVCAP:
     case AMIS_TARCAP:
     case AMIS_RESCAP:
     case AMIS_AMBUSHCAP:
     {
     if (calcThreatScore == 0)
     {
     if (Isflightlead) AiSendCommand (self, FalconWingmanMsg::WMSpread, AiFlight);
     int choice = rand()% (SkillLevel()+1);
     if (choice == 0) bvrCurrProfile = Pwall;
     else if (choice == 1) bvrCurrProfile = PwideLT;
     else  bvrCurrProfile = Pgrinder;
     }
     else
     {
     if (Isflightlead)
     AiSendCommand (self, FalconWingmanMsg::WMResCell, AiFlight);
     if (calcThreatScore == 1)
     {

     }
     else
     {

     }
     if (choice == 0) bvrCurrProfile = Plevel1c;
     else if (choice == 1) bvrCurrProfile = Plevel2c;
     else if (choice == 2) bvrCurrProfile = Plevel3c;
     else if (choice == 3) bvrCurrProfile = PwideLT;
     else if (choice == 4) bvrCurrProfile = PShortLT;
     else if (choice == 5) bvrCurrProfile = Pwideazimuth;
     else if (choice == 6) bvrCurrProfile = Pshortazimuth;
     else if (choice == 7) bvrCurrProfile = Pwall;
     else if (choice == 8) bvrCurrProfile = Pbeamdeploy;
     else if (choice == 9) bvrCurrProfile = Pbeambeam;
     else bvrCurrProfile = Pbeambeam;
     }
     }
     break;
    // extreemly aggresive missions...kill at all cost and move forward
     case AMIS_ALERT:
     case AMIS_INTERCEPT:
     {
     if ( not outranged)
     {
     if (Isflightlead)AiSendCommand (self, FalconWingmanMsg::WMFluidFour, AiFlight);
     int choice = rand()% (SkillLevel()+1);
     if (choice == 0) bvrCurrProfile = Plevel1a;
     else if (choice == 1) bvrCurrProfile = Plevel2a;
     else bvrCurrProfile = Plevel3a;
     }
     else
     {
     if (Isflightlead) AiSendCommand (self, FalconWingmanMsg::WMResCell, AiFlight);
     int choice = rand()% (SkillLevel()+1);
     if (choice == 0) bvrCurrProfile = Plevel1c;
     else if (choice == 1) bvrCurrProfile = Plevel2c;
     else if (choice == 2) bvrCurrProfile = Pbeamdeploy;
     else if (choice == 3) bvrCurrProfile = Pshortazimuth;
     else if (choice == 4) bvrCurrProfile = PwideLT;
     else bvrCurrProfile = Plevel3c;
     }
     }
     break;
     case AMIS_SWEEP:
     case AMIS_ESCORT: // it's a consideratino to stay on time here to protect the pacage
     {
     if ( not outranged)
     {
     if (Isflightlead)AiSendCommand (self, FalconWingmanMsg::WMFluidFour, AiFlight);
     int choice = rand()% (SkillLevel()+1);
     if (choice == 0) bvrCurrProfile = Plevel2a;
     else if (choice == 1) bvrCurrProfile = PSweep;
     else bvrCurrProfile = PSweep;
     }
     else
     {
     if (Isflightlead) AiSendCommand (self, FalconWingmanMsg::WMResCell, AiFlight);
     int choice = rand()% (SkillLevel()+1);
     choice += rand()%4;
     if (choice == 0) bvrCurrProfile = Plevel1c;
     else if (choice == 1) bvrCurrProfile = Plevel2c;
     else if (choice == 2) bvrCurrProfile = Pbeamdeploy;
     else if (choice == 3) bvrCurrProfile = Pshortazimuth;
     else if (choice == 4) bvrCurrProfile = PShortLT;
     else if (choice == 5) bvrCurrProfile = Pwideazimuth;
     else if (choice == 6) bvrCurrProfile = Plevel3c;
     else if (choice == 7) bvrCurrProfile = Pwall;
     else if (choice == 8) bvrCurrProfile = Pbeamdeploy;
     else if (choice == 9) bvrCurrProfile = Pbeambeam;
     else bvrCurrProfile = Plevel3c;
     }
     }break;

    // thiese are mission with killing a-a dudes as second priority
    // so try to avoid, but be aggresive if the bandits wanna play
     case AMIS_SEADSTRIKE:
     case AMIS_SEADESCORT:
     case AMIS_OCASTRIKE:
     case AMIS_INTSTRIKE:
     case AMIS_STRIKE:
     case AMIS_DEEPSTRIKE:
     case AMIS_CAS:
     case AMIS_ONCALLCAS:
     case AMIS_PRPLANCAS:
     case AMIS_SAD:
     case AMIS_INT:
     case AMIS_BAI:
     case AMIS_ASW:
     case AMIS_ASHIP:
     case AMIS_RECON:
     case AMIS_BDA:
     case AMIS_PATROL:
     case AMIS_RECONPATROL:
     {
     //Cobra we are modifying here to allow A/G flights to attack
     //this works with prior new BVR code for A/G flights
     if ( not IsSetATC(HasAGWeapon) or g_bUseAggresiveIncompleteA2G or ( not g_bUseAggresiveIncompleteA2G and missionComplete))
     {
     if ( not outranged)
     {
     if (Isflightlead) AiSendCommand (self, FalconWingmanMsg::WMBox, AiFlight);
     int choice = rand()%3;
     if (choice == 0) bvrCurrProfile =  Plevel1b;
     else if (choice == 1) bvrCurrProfile = Plevel2b;
     else  bvrCurrProfile = Plevel3b;
     }
     else
     {
     if (Isflightlead) AiSendCommand (self, FalconWingmanMsg::WMResCell, AiFlight);
     int choice = rand()%11;
     if (choice == 0) bvrCurrProfile = Plevel1c;
     else if (choice == 1) bvrCurrProfile = Plevel2c;
     else if (choice == 2) bvrCurrProfile = Pbeamdeploy;
     else if (choice == 3) bvrCurrProfile = Pshortazimuth;
     else if (choice == 4) bvrCurrProfile = PShortLT;
     else if (choice == 5) bvrCurrProfile = Pwideazimuth;
     else if (choice == 6) bvrCurrProfile = Plevel3c;
     else if (choice == 7) bvrCurrProfile = Pwall;
     else if (choice == 8) bvrCurrProfile = Pbeamdeploy;
     else if (choice == 9) bvrCurrProfile = Pbeambeam;
     else bvrCurrProfile = Plevel3c;
     }
     }
     else
     {
     if (Isflightlead)
     {
     AiSendCommand (self, FalconWingmanMsg::WMBox, AiFlight);
     }
     bvrCurrProfile = PDefensive;
     }
     }
     break;
    default:
     {
     bvrCurrProfile = PDefensive;
     break;
     }
    break;
    }*/
    /////////////TEST TEST TEST//////////////////
    // static BVRProfileType test = Plevel1a;
    // bvrCurrProfile = test;
    /////////////TEST TEST TEST//////////////////
}
// a = aggresive missilerange superiority
// b = defensive missilerange superiority
// c = missilerange inferior
// 1 is the least advanced tactic
// 2 is the medium advanced tactic
// 3 is the most advanced

// maneuvers for AGGRESIVE missilerange superiority
void DigitalBrain::level1a(void)
{
    //if (Isflightlead) bvrCurrTactic = BvrNotchRightHigh;
    //else bvrCurrTactic = BvrNotchLeft;

    //return;
    /////////////TEST TEST TEST//////////////////

    if ( not Isflightlead and not IsElementlead) return;

    // well just push forward.
    switch (bvractionstep)
    {

        case 0:
        {
            if (Isflightlead)
            {
                bvrCurrTactic = BvrPursuit ;
                bvrTacticTimer = SimLibElapsedTime + 5 * CampaignSeconds;
            }
        }
        break;
    }
}
void DigitalBrain::level2a(void)
{
    if ( not Isflightlead and not IsElementlead) return;

    switch (bvractionstep)
    {
        case 0:
        {
            if (Isflightlead)
            {
                bvrCurrTactic = BvrSingleSideOffset ;
                bvrTacticTimer = SimLibElapsedTime + 5 * CampaignSeconds;
            }
        }
        break;
    }
}
void DigitalBrain::level3a(void)
{
    if ( not Isflightlead and not IsElementlead) return;

    // allowed to drag once pr missile engagement
    switch (bvractionstep)
    {
        case 0:
        {
            if (Isflightlead)
            {
                bvrCurrTactic = BvrSingleSideOffset ;
                bvrTacticTimer = SimLibElapsedTime + 5 * CampaignSeconds;
            }
        }

        if (IsSupportignmissile()) bvractionstep ++;

        break;

        case 1:
        {
            if (IsSupportignmissile())
            {
                bvrTacticTimer = SimLibElapsedTime + 3 * CampaignSeconds;
                break;
            }

            bvrCurrTactic = BvrPump ;
            bvrTacticTimer = SimLibElapsedTime + 15 * CampaignSeconds;
            bvractionstep = 0;
            break;
        }
        break;
    }
}
// b = DEFENSIVE missilerange superiority
void DigitalBrain::level1b(void)
{
    // idea is to crank away ...if the target lets us offset we continue enroute
    // if the target hotnoses us we engage (continuecrank)

    bvrTacticTimer = SimLibElapsedTime + 8 * CampaignSeconds;

    if ( not Isflightlead) return;

    AircraftClass *elementlead = (AircraftClass *)self->GetCampaignObject()->GetComponentNumber(2);
    AircraftClass *wing1 = (AircraftClass *)self->GetCampaignObject()->GetComponentNumber(1);
    AircraftClass *wing2 = (AircraftClass *)self->GetCampaignObject()->GetComponentNumber(3);

    if (HowManyHotnosed()) bvractionstep = 0;
    else bvractionstep = 1;

    switch (bvractionstep)
    {
        case 0:
        {
            bvrCurrTactic = BvrCrank;
            break;
        }

        case 1:
        {
            bvrCurrTactic = BvrFollowWaypoints;
            break;
        }
        break;
    }
}

void DigitalBrain::level2b(void)
{
    // b = defensive missilerange superiority
    /*idea is :
    the element split up if spiked...
    the spiked element notch, the naked crank
    the offset is in the same direction to "stick together"
      */
    if ( not Isflightlead and not IsElementlead) return;

    AircraftClass *elementlead = NULL;

    if (self->vehicleInUnit == 0)
        elementlead = (AircraftClass *)self->GetCampaignObject()->GetComponentNumber(2);

    if (self->vehicleInUnit == 2)
        elementlead = (AircraftClass *)self->GetCampaignObject()->GetComponentNumber(0);

    if ((bvractionstep not_eq 0 or targetData->range > TGTMAR) and WhoIsSpiked() >= 15 and targetData->range < MAR)
        bvractionstep = 0;
    else bvractionstep = 1;

    if ( not HowManyHotnosed())bvractionstep = 2;


    switch (bvractionstep)
    {
        case 0:
        {
            bvrCurrTactic = BvrPump;
            bvrTacticTimer = SimLibElapsedTime + 50 * CampaignSeconds;
            break;
        }

        case 1:
        {
            if (elementlead and elementlead->DBrain()->offsetdir == offRight)
                bvrCurrTactic = BvrCrankRight;
            else if (elementlead and elementlead->DBrain()->offsetdir == offLeft)
                bvrCurrTactic = BvrCrankLeft;
            else bvrCurrTactic = BvrCrank;

            bvrTacticTimer = SimLibElapsedTime + 40 * CampaignSeconds;
            break;
        }

        case 2:
        {
            if (Isflightlead) bvrCurrTactic = BvrFollowWaypoints;
            else bvrCurrTactic = BvrFlyFormation;

            bvrTacticTimer = SimLibElapsedTime + 8 * CampaignSeconds;
            break;
        }
        break;
    }
}
void DigitalBrain::level3b(void)
{
    if ( not Isflightlead and not IsElementlead) return;

    // b = defensive missilerange superiority
    /*idea is :
    the element split up if spiked...
    the spiked element notch, the naked crank
    the offset is in the same direction to "stick together"
      */
    float crankme = targetPtr->localData->ataFrom * RTD;
    AircraftClass *elementlead = NULL;

    if (self->vehicleInUnit == 0)
        elementlead = (AircraftClass *)self->GetCampaignObject()->GetComponentNumber(2);

    if (self->vehicleInUnit == 2)
        elementlead = (AircraftClass *)self->GetCampaignObject()->GetComponentNumber(0);

    if (bvractionstep not_eq 3)// we are not pumping
    {
        if ((bvractionstep not_eq 0 or targetData->range > maxAAWpnRange) and WhoIsSpiked() >= 15)
            bvractionstep = 0;
        else bvractionstep = 1;

        if ( not HowManyHotnosed())bvractionstep = 2;

        if (IsSupportignmissile()) bvractionstep = 3;
    }


    switch (bvractionstep)
    {
        case 0:
        {
            if (elementlead and elementlead->DBrain()->bvrCurrTactic not_eq BvrNotch and 
                elementlead->DBrain()->bvrCurrTactic not_eq BvrPump)
                if (rand() % 2 == 1)
                    bvrCurrTactic = BvrNotchRightHigh;
                else
                    bvrCurrTactic = BvrNotch;
            else
            {
                int randme = rand() % 3;

                if (elementlead and elementlead->DBrain()->offsetdir == offRight)
                {
                    if (randme == 1 and crankme < 90.0f)
                    {
                        bvrCurrTactic = BvrCrankRightHi;
                    }
                    else if (randme == 2 and crankme < 90.0f)
                    {
                        bvrCurrTactic = BvrCrankRightLo;
                    }
                    else
                        bvrCurrTactic = BvrCrankRight;
                }

                else if (elementlead and elementlead->DBrain()->offsetdir == offLeft)
                {
                    if (randme == 1 and crankme < 90.0f)
                    {
                        bvrCurrTactic = BvrCrankLeftHi;
                    }
                    else if (randme == 2 and crankme < 90.0f)
                    {
                        bvrCurrTactic = BvrCrankLeftLo;
                    }
                    else
                        bvrCurrTactic = BvrCrankLeft;
                }
                else
                {
                    if (randme == 1 and crankme < 90.0f)
                    {
                        bvrCurrTactic = BvrCrankHi;
                    }
                    else if (randme == 2 and crankme < 90.0f)
                    {
                        bvrCurrTactic = BvrCrankLo;
                    }
                    else
                        bvrCurrTactic = BvrCrank;
                }
            }

            bvrTacticTimer = SimLibElapsedTime + 50 * CampaignSeconds;
            break;
        }

        case 1:
        {
            int randme = rand() % 3;

            if (elementlead and elementlead->DBrain()->offsetdir == offRight)
            {
                if (randme == 1 and crankme < 90.0f)
                {
                    bvrCurrTactic = BvrCrankRightHi;
                }
                else if (randme == 2 and crankme < 90.0f)
                {
                    bvrCurrTactic = BvrCrankRightLo;
                }
                else
                    bvrCurrTactic = BvrCrankRight;
            }

            else if (elementlead and elementlead->DBrain()->offsetdir == offLeft)
            {
                if (randme == 1 and crankme < 90.0f)
                {
                    bvrCurrTactic = BvrCrankLeftHi;
                }
                else if (randme == 2 and crankme < 90.0f)
                {
                    bvrCurrTactic = BvrCrankLeftLo;
                }
                else
                    bvrCurrTactic = BvrCrankLeft;
            }
            else
            {
                if (randme == 1 and crankme < 90.0f)
                {
                    bvrCurrTactic = BvrCrankHi;
                }
                else if (randme == 2 and crankme < 90.0f)
                {
                    bvrCurrTactic = BvrCrankLo;
                }
                else
                    bvrCurrTactic = BvrCrank;
            }

            bvrTacticTimer = SimLibElapsedTime + 40 * CampaignSeconds;
            break;
        }

        case 2:
        {
            if (Isflightlead) bvrCurrTactic = BvrFollowWaypoints;
            else bvrCurrTactic = BvrFlyFormation;

            bvrTacticTimer = SimLibElapsedTime + 8 * CampaignSeconds;
            break;
        }

        case 3:// we have launched a weapon
        {
            if (IsSupportignmissile())
            {
                bvrTacticTimer = SimLibElapsedTime + 3 * CampaignSeconds;
                break;
            }

            if (Isflightlead)
            {
                bvrCurrTactic = BvrPump ;
                bvrTacticTimer = SimLibElapsedTime + 15 * CampaignSeconds;
            }
            else if (IsElementlead)
            {
                bvrCurrTactic = BvrPump  ;
                bvrTacticTimer = SimLibElapsedTime + 15 * CampaignSeconds;
            }

            bvractionstep = 1;
        }
        break;
    }
}

// c = missilerange INFERIOR
void DigitalBrain::level1c(void)
{
    if ( not Isflightlead and not IsElementlead) return;

    switch (bvractionstep)
    {
        case 0:
        {
            bvrCurrTactic = BvrPursuit ;
            bvrTacticTimer = SimLibElapsedTime + 5 * CampaignSeconds;
            break;
        }
        break;
    }
}


void DigitalBrain::level2c(void)
{
    /*idea is :
    the flight split up if spiked...
    the spiked element notch, the naked go pure
    if both notch they do it in different direction
    if inside 16nm the wingies are cleared off if untargeted.
      */
    float crankme = targetPtr->localData->ataFrom * RTD;
    AircraftClass *wingman = NULL;

    if (self->vehicleInUnit == 0)
    {
        wingman = (AircraftClass *)self->GetCampaignObject()->GetComponentNumber(1);
    }

    // the elements lead wingie
    if (self->vehicleInUnit == 2)
    {
        wingman = (AircraftClass *)self->GetCampaignObject()->GetComponentNumber(3);
    }

    if (( not Isflightlead and not IsElementlead) and targetData->range > 16 * NM_TO_FT) return;

    if (( not Isflightlead and not IsElementlead) and WhoIsSpiked() < 7 and bvrCurrTactic == BvrFlyFormation) return;

    AircraftClass *elementlead = NULL;

    if (self->vehicleInUnit == 0)
    {
        elementlead = (AircraftClass *)self->GetCampaignObject()->GetComponentNumber(2);
    }

    //we are element lead
    if (self->vehicleInUnit == 2)
    {
        elementlead = (AircraftClass *)self->GetCampaignObject()->GetComponentNumber(0);
    }

    if (wingman)
    {
        if ((bvractionstep not_eq 0  or targetData->range > TGTMAR) and wingman->DBrain()->bvrCurrTactic == BvrFlyFormation and WhoIsSpiked() > 3  or
            (bvractionstep not_eq 0 or targetData->range > TGTMAR) and wingman->DBrain()->bvrCurrTactic not_eq BvrFlyFormation and WhoIsSpiked() > 7)
            bvractionstep = 0;
        else bvractionstep = 1;

    }
    else if ( not wingman)
    {
        if ((bvractionstep not_eq 0 and targetData->range > maxAAWpnRange) and WhoIsSpiked() > 7)
            bvractionstep = 0;
        else bvractionstep = 1;
    }


    switch (bvractionstep)
    {
        case 0:
        {
            if (elementlead and elementlead->DBrain()->offsetdir == offRight)
                bvrCurrTactic = BvrNotchRight;
            else if (elementlead and elementlead->DBrain()->offsetdir == offLeft)
                bvrCurrTactic = BvrNotchLeft;

            break;
            bvrTacticTimer = SimLibElapsedTime + 60 * CampaignSeconds;
        }

        case 1:
        {
            int randme = rand() % 3;

            if (elementlead and elementlead->DBrain()->offsetdir == offLeft)
                bvrCurrTactic = BvrPursuit;
            else if (elementlead and elementlead->DBrain()->offsetdir == offRight)
                bvrCurrTactic = BvrPursuit;
            else
            {
                if (randme == 1 and crankme < 90.0f)
                {
                    bvrCurrTactic = BvrCrankHi;
                }
                else if (randme == 2 and crankme < 90.0f)
                {
                    bvrCurrTactic = BvrCrankLo;
                }
                else
                    bvrCurrTactic = BvrCrank;
            }

            bvrTacticTimer = SimLibElapsedTime + 40 * CampaignSeconds;
            break;
        }
        break;
    }
}

void DigitalBrain::level3c(void)
{
    if ( not Isflightlead and not IsElementlead) return;

    switch (bvractionstep)// use the leads stepper to coordinate
    {
        case 0:
        {
            if (targetData->range > 40 * NM_TO_FT)
            {
                bvrTacticTimer = SimLibElapsedTime + 5 * CampaignSeconds;
                break;
            }

            //ACTION
            if (Isflightlead)
            {
                int randme = rand() % 3;

                if (randme == 1)
                {
                    bvrCurrTactic = BvrCrankLeftHi;
                }
                else if (randme == 2)
                {
                    bvrCurrTactic = BvrCrankLeftLo;
                }
                else
                    bvrCurrTactic = BvrCrankLeft;

                bvrTacticTimer = SimLibElapsedTime + 35 * CampaignSeconds;
            }
            else if (IsElementlead)
            {
                bvrCurrTactic = BvrPump  ;
                bvrTacticTimer = SimLibElapsedTime + 15 * CampaignSeconds;
            }

            bvractionstep ++;
            break;
        }

        case 1:
        {
            //ACTION
            if (Isflightlead)
            {
                bvrCurrTactic = BvrNotchLeft ;
                bvrTacticTimer = SimLibElapsedTime + 50 * CampaignSeconds;
            }
            else if (IsElementlead)
            {
                bvrCurrTactic = BvrCrankRight  ;
                bvrTacticTimer = SimLibElapsedTime + 30 * CampaignSeconds;
            }

            bvractionstep ++;
            break;
        }

        case 2:
        {
            if (WhoIsSpiked() > 3)
            {
                if (Isflightlead)
                {
                    bvrCurrTactic = BvrNotchRight ;
                    bvrTacticTimer = SimLibElapsedTime + 10 * CampaignSeconds;
                }
                else if (IsElementlead)
                {
                    bvrCurrTactic = BvrNotchLeft  ;
                    bvrTacticTimer = SimLibElapsedTime + 10 * CampaignSeconds;
                }
            }
            else
            {
                bvrCurrTactic = BvrSingleSideOffset  ;
                bvrTacticTimer = SimLibElapsedTime + 10 * CampaignSeconds;
            }

            break;
        }
    }
}

void DigitalBrain::beamdeploy(void)
{
    if ( not Isflightlead and not IsElementlead) return;

    if (bvractionstep not_eq 0 and WhoIsSpiked() > 3)
        bvractionstep = 1;

    else bvractionstep = 0;

    switch (bvractionstep)
    {
        case 0:
        {
            /*if (targetData->range > maxAAWpnRange )
            {
               bvrTacticTimer = SimLibElapsedTime + 5 * CampaignSeconds;
               break;
            }*/
            //ACTION
            if (Isflightlead)
            {
                bvrCurrTactic = BvrSingleSideOffset ;
                bvrTacticTimer = SimLibElapsedTime + 5 * CampaignSeconds;
            }
            else if (IsElementlead)
            {
                bvrCurrTactic = BvrSingleSideOffset  ;
                bvrTacticTimer = SimLibElapsedTime + 5 * CampaignSeconds;
            }

            break;
        }

        case 1:
        {
            /* if (targetData->range > maxAAWpnRange )
             {
               bvrTacticTimer = SimLibElapsedTime + 5 * CampaignSeconds;
               break;
             }*/
            //ACTION
            if (Isflightlead)
            {
                bvrCurrTactic = BvrNotchLeft ;
                bvrTacticTimer = SimLibElapsedTime + 120 * CampaignSeconds;
            }
            else if (IsElementlead)
            {
                bvrCurrTactic = BvrNotchRight  ;
                bvrTacticTimer = SimLibElapsedTime + 120 * CampaignSeconds;
            }

            break;
        }
        break;
    }
}

void DigitalBrain::grinder(void)
{
    if ( not Isflightlead and not IsElementlead) return;

    float  ActionRange = 30.0f * NM_TO_FT;

    switch (bvractionstep)
    {
        case 0:
        {
            if (targetData->range > ActionRange)
            {
                bvrTacticTimer = SimLibElapsedTime + 5 * CampaignSeconds;
                break;
            }

            //ACTION
            if (IsElementlead)
            {
                bvrCurrTactic = BvrPursuit ;
                bvrTacticTimer = SimLibElapsedTime + 30 * CampaignSeconds;
            }
            else if (Isflightlead)
            {
                bvrCurrTactic = BvrPump  ;
                bvrTacticTimer = SimLibElapsedTime + 30 * CampaignSeconds;
            }

            bvractionstep ++;
            break;
        }

        case 1:// we shoudl now be setup about 10-15nm trail
        {
            if (Isflightlead)
            {
                bvrCurrTactic = BvrSingleSideOffset ;
                bvrTacticTimer = SimLibElapsedTime + 5 * CampaignSeconds;
            }
            else if (IsElementlead)
            {
                bvrCurrTactic = BvrSingleSideOffset  ;
                bvrTacticTimer = SimLibElapsedTime + 5 * CampaignSeconds;
            }

            if (IsSupportignmissile()) bvractionstep ++;

            break;
        }

        case 2:// we have launched a weapon
        {
            if (IsSupportignmissile())
            {
                bvrTacticTimer = SimLibElapsedTime + 3 * CampaignSeconds;
                break;
            }

            if (Isflightlead)
            {
                bvrCurrTactic = BvrPump ;
                bvrTacticTimer = SimLibElapsedTime + 40 * CampaignSeconds;

                if (targetData->range > 10 * NM_TO_FT or WhoIsSpiked() < 3)
                    bvractionstep = 1;
            }
            else if (IsElementlead)
            {
                bvrCurrTactic = BvrPump  ;
                bvrTacticTimer = SimLibElapsedTime + 40 * CampaignSeconds;

                if (targetData->range > 10 * NM_TO_FT or WhoIsSpiked() < 3)
                    bvractionstep = 1;
            }

            break;
        }

        case 3:
        {
            bvrCurrProfile = Pnone;
            bvractionstep = 0;
            break;
        }
    }
}
void DigitalBrain::wideazimuth(void)
{
    if ( not Isflightlead and not IsElementlead) return;

    switch (bvractionstep)
    {
        case 0:
        {
            if (targetData->range > MAR * 2)
            {
                bvrTacticTimer = SimLibElapsedTime + 5 * CampaignSeconds;
                break;
            }

            //ACTION
            if (Isflightlead)
            {
                bvrCurrTactic = BvrNotchLeft ;
                bvrTacticTimer = SimLibElapsedTime + 55 * CampaignSeconds;
            }
            else if (IsElementlead)
            {
                bvrCurrTactic = BvrNotchRight  ;
                bvrTacticTimer = SimLibElapsedTime + 55 * CampaignSeconds;
            }

            bvractionstep++;
            break;
        }

        case 1:
        {
            if (targetData->range > MAR * 2)
            {
                bvrTacticTimer = SimLibElapsedTime + 5 * CampaignSeconds;
                break;
            }

            //ACTION
            if (Isflightlead)
            {
                bvrCurrTactic = BvrPursuit ;
                bvrTacticTimer = SimLibElapsedTime + 120 * CampaignSeconds;
            }
            else if (IsElementlead)
            {
                bvrCurrTactic = BvrPursuit  ;
                bvrTacticTimer = SimLibElapsedTime + 120 * CampaignSeconds;
            }

            break;
        }
    }
}
void DigitalBrain::shortazimuth(void)
{
    if ( not Isflightlead and not IsElementlead) return;

    switch (bvractionstep)
    {
        case 0:
        {
            if (targetData->range > MAR * 2)
            {
                bvrTacticTimer = SimLibElapsedTime + 5 * CampaignSeconds;
                break;
            }

            //ACTION
            if (Isflightlead)
            {
                bvrCurrTactic = BvrCrankLeft ;
                bvrTacticTimer = SimLibElapsedTime + 55 * CampaignSeconds;
            }
            else if (IsElementlead)
            {
                bvrCurrTactic = BvrCrankRight  ;
                bvrTacticTimer = SimLibElapsedTime + 55 * CampaignSeconds;
            }

            bvractionstep++;
            break;
        }

        case 1:
        {
            if (targetData->range > MAR * 2)
            {
                bvrTacticTimer = SimLibElapsedTime + 5 * CampaignSeconds;
                break;
            }

            //ACTION
            if (Isflightlead)
            {
                bvrCurrTactic = BvrPursuit ;
                bvrTacticTimer = SimLibElapsedTime + 120 * CampaignSeconds;
            }
            else if (IsElementlead)
            {
                bvrCurrTactic = BvrPursuit  ;
                bvrTacticTimer = SimLibElapsedTime + 120 * CampaignSeconds;
            }

            break;
        }
    }
}
void DigitalBrain::wideLT(void)
{
    if ( not Isflightlead and not IsElementlead) return;

    float ActionRange = 30 * NM_TO_FT;

    switch (bvractionstep)
    {
        case 0:
        {
            if (targetData->range > ActionRange)
            {
                bvrTacticTimer = SimLibElapsedTime + 5 * CampaignSeconds;
                break;
            }

            //ACTION
            if (IsElementlead)
            {
                bvrCurrTactic = BvrPursuit ;
                bvrTacticTimer = SimLibElapsedTime + 25 * CampaignSeconds;
            }
            else if (Isflightlead)
            {
                bvrCurrTactic = BvrPump  ;
                bvrTacticTimer = SimLibElapsedTime + 45 * CampaignSeconds;
            }

            bvractionstep ++;
            break;
        }

        case 1:
        {
            if (Isflightlead)
            {
                bvrCurrTactic = BvrPursuit ;
                bvrTacticTimer = SimLibElapsedTime + 60 * CampaignSeconds;
            }
            else if (IsElementlead)
            {
                bvrCurrTactic = BvrPursuit  ;
                bvrTacticTimer = SimLibElapsedTime + 60 * CampaignSeconds;
            }
        }

        bvractionstep ++;
        break;

        case 2:
        {
            bvrCurrProfile = Pnone;
            bvractionstep = 0;
            break;
        }
        break;
    }
}
void DigitalBrain::ShortLT(void)
{
    if ( not Isflightlead and not IsElementlead) return;

    float ActionRange = 30 * NM_TO_FT;

    switch (bvractionstep)
    {
        case 0:
        {
            if (targetData->range > ActionRange)
            {
                bvrTacticTimer = SimLibElapsedTime + 5 * CampaignSeconds;
                break;
            }

            //ACTION
            if (IsElementlead)
            {
                bvrCurrTactic = BvrPursuit ;
                bvrTacticTimer = SimLibElapsedTime + 15 * CampaignSeconds;
            }
            else if (Isflightlead)
            {
                bvrCurrTactic = BvrPump  ;
                bvrTacticTimer = SimLibElapsedTime + 30 * CampaignSeconds;
            }

            bvractionstep ++;
            break;
        }

        case 1:
        {
            if (Isflightlead)
            {
                bvrCurrTactic = BvrPursuit ;
                bvrTacticTimer = SimLibElapsedTime + 60 * CampaignSeconds;
            }
            else if (IsElementlead)
            {
                bvrCurrTactic = BvrPursuit  ;
                bvrTacticTimer = SimLibElapsedTime + 60 * CampaignSeconds;
            }

            break;
        }

        case 2:
        {
            bvrCurrProfile = Pnone;
            bvractionstep = 0;
            break;
        }
    }
}
void DigitalBrain::beambeam(void)
{
    if ( not Isflightlead and not IsElementlead) return;

    float ActionRange = 25 * NM_TO_FT;

    switch (bvractionstep)
    {
        case 0:
        {
            if (targetData->range > ActionRange)
            {
                bvrTacticTimer = SimLibElapsedTime + 5 * CampaignSeconds;
                break;
            }

            //ACTION
            if (Isflightlead)
            {
                bvrCurrTactic = BvrNotchLeft ;
                bvrTacticTimer = SimLibElapsedTime + 120 * CampaignSeconds;
            }
            else if (IsElementlead)
            {
                bvrCurrTactic = BvrNotchRight  ;
                bvrTacticTimer = SimLibElapsedTime + 120 * CampaignSeconds;
            }

            bvractionstep++;
            break;
        }

        case 1:
        {
            if (targetData->range > ActionRange)
            {
                bvrTacticTimer = SimLibElapsedTime + 5 * CampaignSeconds;
                break;
            }

            //ACTION
            if (Isflightlead)
            {
                bvrCurrTactic = BvrPursuit ;
                bvrTacticTimer = SimLibElapsedTime + 60 * CampaignSeconds;
            }
            else if (IsElementlead)
            {
                bvrCurrTactic = BvrPursuit  ;
                bvrTacticTimer = SimLibElapsedTime + 60 * CampaignSeconds;
            }

            bvractionstep++;
            break;
        }

        case 2:
        {
            bvrCurrProfile = Pnone;
            bvractionstep = 0;
            break;
        }
    }
}
void DigitalBrain::wall(void)
{
    if ( not Isflightlead and not IsElementlead) return;

    if (Isflightlead)
    {
        bvrCurrTactic = BvrPursuit;
        bvrTacticTimer = SimLibElapsedTime + 10 * CampaignSeconds;
    }
}
void DigitalBrain::Sweep(void)
{
    // sweep with superior weaopns
    /* the plan is to lean into bandits early
    if we outnumber them then only lean with one element
    when the bandits are cold we fly on course.

      */
    if ( not Isflightlead and not IsElementlead) return;

    AircraftClass *elementlead = NULL;

    if (self->vehicleInUnit == 0)
        elementlead = (AircraftClass *)self->GetCampaignObject()->GetComponentNumber(2);

    if (self->vehicleInUnit == 2)
        elementlead = (AircraftClass *)self->GetCampaignObject()->GetComponentNumber(0);

    bvractionstep = 0;

    if (HowManyTargetet() <= self->GetCampaignObject()->NumberOfComponents() - 1 and HowManyHotnosed()) bvractionstep = 1;
    else if (HowManyTargetet() and HowManyHotnosed()) bvractionstep = 2;

    bvrTacticTimer = SimLibElapsedTime + 15 * CampaignSeconds;

    switch (bvractionstep)
    {
        case 0:
        {
            // fly waypoints
            if (Isflightlead) bvrCurrTactic = BvrFollowWaypoints;
            else bvrCurrTactic = BvrFlyFormation;
        }
        break;

        case 1:// lean element into the threat
        {
            if (Isflightlead) bvrCurrTactic = BvrFollowWaypoints;
            else bvrCurrTactic = BvrPursuit;
        }
        break;

        case 2:// all lean into the threat
        {
            bvrCurrTactic = BvrPursuit;
        }
        break;
    }
}
void DigitalBrain::Defensive(void)
{
    if ( not Isflightlead and not IsElementlead) return;

    if (WhoIsSpiked() or WhoIsHotnosed())
    {
        bvrCurrTactic = BvrFollowWaypoints;
        bvrTacticTimer = SimLibElapsedTime + 10 * CampaignSeconds;

        if (targetData->range > 20 * NM_TO_FT)
        {

            bvrCurrTactic = BvrNotch;
            bvrTacticTimer = SimLibElapsedTime + 15 * CampaignSeconds;
            short edata[3];
            int flightIdx = self->GetCampaignObject()->GetComponentIndex(self);
            edata[0] = flightIdx;
            edata[1] = 0;//notching
            AiMakeRadioResponse(self, rcEXECUTERESPONSE, edata);
        }
        else
        {
            bvrCurrTactic = BvrPump;
            bvrTacticTimer = SimLibElapsedTime + 30 * CampaignSeconds + SkillLevel() * 5 * CampaignSeconds;
            short edata[3];
            int flightIdx = self->GetCampaignObject()->GetComponentIndex(self);
            edata[0] = flightIdx;
            edata[1] = 3;//pumping
            AiMakeRadioResponse(self, rcEXECUTERESPONSE, edata);
        }

        /*if (spiked and targetData->range < maxAAWpnRange)
        {// our lead draged us into a dangeres situation we need to split and notch
        bvrCurrTactic = BvrNotch;
        bvrTacticTimer = SimLibElapsedTime + 15 * CampaignSeconds;
         short edata[3];
         int flightIdx = self->GetCampaignObject()->GetComponentIndex(self);
         edata[0] = flightIdx;
         edata[1] = 0;//notching
         AiMakeRadioResponse( self, rcEXECUTERESPONSE, edata );
        }*/
    }
    else
    {
        bvrCurrTactic = BvrFollowWaypoints;
        bvrTacticTimer = SimLibElapsedTime + 20 * CampaignSeconds;
    }

}
int DigitalBrain::HowManySpiked(void)
{
    int result = 0;

    AircraftClass *wingman = NULL;
    AircraftClass *secondwingman = NULL;
    AircraftClass *elementlead = NULL;

    //the Isflightleads wingie
    if (self->vehicleInUnit == 0)
    {
        wingman = (AircraftClass *)self->GetCampaignObject()->GetComponentNumber(1);
    }

    // the elements lead wingie
    if (self->vehicleInUnit == 2)
        wingman = (AircraftClass *)self->GetCampaignObject()->GetComponentNumber(3);

    //we are flightlead
    if (self->vehicleInUnit == 0)
    {
        elementlead = (AircraftClass *)self->GetCampaignObject()->GetComponentNumber(2);
        secondwingman = (AircraftClass *)self->GetCampaignObject()->GetComponentNumber(3);
    }

    //we are element lead
    if (self->vehicleInUnit == 2)
    {
        elementlead = (AircraftClass *)self->GetCampaignObject()->GetComponentNumber(0);
        secondwingman = (AircraftClass *)self->GetCampaignObject()->GetComponentNumber(1);
    }

    spiked = false;

    if (SpikeCheck(self)) spiked = true;

    if (spiked) // we are spiked by someone the flight is targeted to..
    {
        // first our own spike
        if (SpikeCheck(self) not_eq lastspikeent)
        {
            lastspikeent = SpikeCheck(self) ;
            spiketframetime = SimLibElapsedTime;
        }
        else spikeseconds = (SimLibElapsedTime - spiketframetime) / CampaignSeconds;
    }
    else
    {
        spikeseconds = NULL;
        lastspikeent = NULL;
        spikesecondselement = NULL;
    }

    VU_TIME spikesecondselement = spikeseconds;

    if (wingman and wingman->DBrain()->spikeseconds > spikesecondselement)
        spikesecondselement = wingman->DBrain()->spikeseconds;


    if (spiked) result ++;

    if (wingman and wingman->DBrain()->spiked) result ++;

    if (elementlead and elementlead->DBrain()->spiked) result ++;

    if (secondwingman and secondwingman->DBrain()->spiked) result ++;

    return result;
}
int DigitalBrain::WhoIsSpiked(void)
{
    int result = 0;

    AircraftClass *wingman = NULL;
    AircraftClass *secondwingman = NULL;
    AircraftClass *elementlead = NULL;

    //the Isflightleads wingie
    if (self->vehicleInUnit == 0)
    {
        wingman = (AircraftClass *)self->GetCampaignObject()->GetComponentNumber(1);
    }

    // the elements lead wingie
    if (self->vehicleInUnit == 2)
        wingman = (AircraftClass *)self->GetCampaignObject()->GetComponentNumber(3);

    //we are flightlead
    if (self->vehicleInUnit == 0)
    {
        elementlead = (AircraftClass *)self->GetCampaignObject()->GetComponentNumber(2);
        secondwingman = (AircraftClass *)self->GetCampaignObject()->GetComponentNumber(3);
    }

    //we are element lead
    if (self->vehicleInUnit == 2)
    {
        elementlead = (AircraftClass *)self->GetCampaignObject()->GetComponentNumber(0);
        secondwingman = (AircraftClass *)self->GetCampaignObject()->GetComponentNumber(1);
    }

    spiked = false;

    if (SpikeCheck(self)) spiked = true;

    if (spiked) // we are spiked by someone the flight is targeted to..
    {
        // first our own spike
        if (SpikeCheck(self) not_eq lastspikeent)
        {
            lastspikeent = SpikeCheck(self) ;
            spiketframetime = SimLibElapsedTime;
        }
        else spikeseconds = (SimLibElapsedTime - spiketframetime) / CampaignSeconds;
    }
    else
    {
        spikeseconds = NULL;
        lastspikeent = NULL;
        spikesecondselement = NULL;
    }

    VU_TIME spikesecondselement = spikeseconds;

    if (wingman and wingman->DBrain()->spikeseconds > spikesecondselement)
        spikesecondselement = wingman->DBrain()->spikeseconds;


    if (spiked) result = 8;

    if (wingman and wingman->DBrain()->spiked) result += 4;

    if (elementlead and elementlead->DBrain()->spiked) result += 2;

    if (secondwingman and secondwingman->DBrain()->spiked) result += 1;

    //Cobra Remove the scene of a CTD
    //let's just do the who is hot nosed thing
    if (WhoIsHotnosed() > 3)
    {
        result += 16;
    }

    /*CampBaseClass *campBaseObj;
    if (targetPtr->BaseData()->IsSim())
     campBaseObj = ((SimBaseClass*)targetPtr->BaseData())->GetCampaignObject();
    else
     campBaseObj = ((CampBaseClass *)targetPtr->BaseData());

    // If it doesn't have a campaign object or it's identified... END OF ADDED SECTION plus the use of campBaseObj below
    if ( not campBaseObj or campBaseObj->GetIdentified(self->GetTeam()))
    {
     RadarClass* theRadar = (RadarClass*)FindSensor(((SimMoverClass*)targetPtr->BaseData()), SensorClass::Radar);
     if (theRadar)
     {
     RadarDataSet* radarData = &radarDatFileTable[self->GetRadarType()];
     if (radarData->MaxTwstargets not_eq 0)
     {
     if (WhoIsHotnosed() >3)
     {
     result += 16;
     }
     }
     }
    }*/

    return result;
}

void DigitalBrain::CalculateMAR()
{
    //Cobra... Just say no to crack... Why can't we comment our code?  What the heck is MAR, DOR, TGTMAR etc.
    //So, we just delete you and start over.
    /*
    // find MAR and DOR
     MAR = 5.0f * NM_TO_FT;
     TGTMAR = maxAAWpnRange/2;

     // 2002-03-22 MODIFIED BY S.G. Only use the original code if we have no minTGTMAR
     if ( not self->af or self->af->GetMinTGTMAR() == 0.0) {
     if (self->EntityType()->classInfo_[VU_SPTYPE] == SPTYPE_MIG29or
      self->EntityType()->classInfo_[VU_SPTYPE] == SPTYPE_MIG31)
     {
     if (maxAAWpnRange > 6000.0f) // 2002-03-06 ADDED BY S.G. If you only have guns or nothing, don't be foolish and go on the aggresive.
     TGTMAR = max(TGTMAR,13.0f*NM_TO_FT);
     }

     if (
      self->EntityType()->classInfo_[VU_SPTYPE] == SPTYPE_SU27or
      self->EntityType()->classInfo_[VU_SPTYPE] == SPTYPE_F14Aor
      self->EntityType()->classInfo_[VU_SPTYPE] == SPTYPE_F15Cor
      self->EntityType()->classInfo_[VU_SPTYPE] == SPTYPE_F15Eor
      self->EntityType()->classInfo_[VU_SPTYPE] == SPTYPE_F16Cor
      self->EntityType()->classInfo_[VU_SPTYPE] == SPTYPE_F18Aor
      self->EntityType()->classInfo_[VU_SPTYPE] == SPTYPE_F18Dor
      self->EntityType()->classInfo_[VU_SPTYPE] == SPTYPE_F22)
     {
     if (maxAAWpnRange > 6000.0f) // 2002-03-06 ADDED BY S.G. If you only have guns or nothing, don't be foolish and go on the aggresive
     TGTMAR = max(TGTMAR,22.0f*NM_TO_FT);
     }
     }
     // Otherwise use its assigned minTGTMAR
     else {
     if (maxAAWpnRange > 6000.0f)
     // END OF ADDED SECTION 2002-03-06
     TGTMAR = max(TGTMAR,self->af->GetMinTGTMAR() * NM_TO_FT);
     }
     // END IF MODIFIED SECTION 2002-03-22

     // 2002-02-12 ADDED BY S.G. GetIdentified is a CampBaseClass so we must fetch it
     CampBaseClass *campBaseObj;
     if (targetPtr->BaseData()->IsSim())
     campBaseObj = ((SimBaseClass*)targetPtr->BaseData())->GetCampaignObject();
     else
     campBaseObj = ((CampBaseClass *)targetPtr->BaseData());

     // If it doesn't have a campaign object or it's identified... END OF ADDED SECTION plus the use of campBaseObj below
     if ( not campBaseObj or campBaseObj->GetIdentified(self->GetTeam()))
     {// we have Type ID.
     // 2002-03-22 MODIFIED BY S.G. If our target is an airplane and we have a minMAR, use it instead of the original method
     if ( not targetPtr->BaseData()->IsAirplane() or not ((AircraftClass *)targetPtr->BaseData())->af or ((AircraftClass *)targetPtr->BaseData())->af->GetMaxMARIdedStart() == 0.0f) {
     if (targetPtr->BaseData()->EntityType()->classInfo_[VU_SPTYPE] == SPTYPE_MIG29or
      targetPtr->BaseData()->EntityType()->classInfo_[VU_SPTYPE] == SPTYPE_MIG31)
     {
     MAR  = max(MAR,5.0f*NM_TO_FT) ;
     if (targetPtr->BaseData()->ZPos() <-28000.0f)
     MAR += 8.0f* NM_TO_FT;
     else if (targetPtr->BaseData()->ZPos() <-18000.0f)
     MAR += 5.0f* NM_TO_FT;
     else if (targetPtr->BaseData()->ZPos() <-5000.0f)
     MAR += 3.0f* NM_TO_FT;
     MAR += min (10.0f * NM_TO_FT, spikesecondselement* NM_TO_FT/3);//add 1nm pr 3 spikesec...max 10nm
     }

     if (
      targetPtr->BaseData()->EntityType()->classInfo_[VU_SPTYPE] == SPTYPE_SU27or
      targetPtr->BaseData()->EntityType()->classInfo_[VU_SPTYPE] == SPTYPE_F14Aor
      targetPtr->BaseData()->EntityType()->classInfo_[VU_SPTYPE] == SPTYPE_F15Cor
      targetPtr->BaseData()->EntityType()->classInfo_[VU_SPTYPE] == SPTYPE_F15Eor
      targetPtr->BaseData()->EntityType()->classInfo_[VU_SPTYPE] == SPTYPE_F16Cor
      targetPtr->BaseData()->EntityType()->classInfo_[VU_SPTYPE] == SPTYPE_F18Aor
      targetPtr->BaseData()->EntityType()->classInfo_[VU_SPTYPE] == SPTYPE_F18Dor
      targetPtr->BaseData()->EntityType()->classInfo_[VU_SPTYPE] == SPTYPE_F22)
     {
     MAR  =max(MAR,10.0f*NM_TO_FT);
     if (targetPtr->BaseData()->ZPos() <-28000.0f)
     MAR += 17.0f* NM_TO_FT;
     else if (targetPtr->BaseData()->ZPos() <-18000.0f)
     MAR += 12.0f* NM_TO_FT;
     else if (targetPtr->BaseData()->ZPos() <-5000.0f)
     MAR += 5.0f* NM_TO_FT;
     }
     }
     // We have ACDATA for this target, use it
     else {
     MAR  = max(MAR,((AircraftClass *)targetPtr->BaseData())->af->GetMaxMARIdedStart()*NM_TO_FT) ;

     if (targetPtr->BaseData()->ZPos() <-28000.0f)
     MAR += ((AircraftClass *)targetPtr->BaseData())->af->GetAddMARIded28k()*NM_TO_FT;
     else if (targetPtr->BaseData()->ZPos() <-18000.0f)
     MAR += ((AircraftClass *)targetPtr->BaseData())->af->GetAddMARIded18k()*NM_TO_FT;
     else if (targetPtr->BaseData()->ZPos() <-5000.0f)
     MAR += ((AircraftClass *)targetPtr->BaseData())->af->GetAddMARIded5k()*NM_TO_FT;
     MAR += min (((AircraftClass *)targetPtr->BaseData())->af->GetAddMARIdedSpike()*NM_TO_FT, spikesecondselement*NM_TO_FT/3);//add 1nm pr 3 spikesec...max 10nm
     }
     // END OF MODIFIED SECTION 2002-03-22
     }
     else
     {// we Don't have Type ID, we have SA on their Speed and altitude though since the ent is spotted
     // 2002-03-22 MODIFIED BY S.G. Instead of using the hard coded value, use configurable one
     if (
     (targetPtr->BaseData()->GetVt() * FTPSEC_TO_KNOTS > 300.0f or
     -targetPtr->BaseData()->ZPos() > 10000.0f)

     )
     {//this might be a combat jet.. asume the worst
     MAR  =max(MAR,g_fMaxMARNoIdA*NM_TO_FT); // 10.0f*NM_TO_FT);
     if (targetPtr->BaseData()->ZPos() <-28000.0f)
     MAR +=g_fMinMARNoId28kA*NM_TO_FT; // 17.0f* NM_TO_FT;
     else if (targetPtr->BaseData()->ZPos() <-18000.0f)
     MAR +=g_fMinMARNoId18kA*NM_TO_FT; // 12.0f* NM_TO_FT;
     else if (targetPtr->BaseData()->ZPos() <-5000.0f)
     MAR += g_fMinMARNoId5kA*NM_TO_FT; // 5.0f* NM_TO_FT;
     }
     else if (targetPtr->BaseData()->GetVt() * FTPSEC_TO_KNOTS > 250.0f)
     {// this could be a a-a capable thingy, but if it's is it's low level so it's a-a long range shoot capabilitys are not great
     MAR  = max(MAR,g_fMaxMARNoIdB*NM_TO_FT) ; // 5.0f*NM_TO_FT
     if (targetPtr->BaseData()->ZPos() <-28000.0f)
     MAR +=g_fMinMARNoId28kB*NM_TO_FT; // 8.0f* NM_TO_FT;
     else if (targetPtr->BaseData()->ZPos() <-18000.0f)
     MAR +=g_fMinMARNoId18kB*NM_TO_FT; // 5.0f* NM_TO_FT;
     else if (targetPtr->BaseData()->ZPos() <-5000.0f)
     MAR += g_fMinMARNoId5kB*NM_TO_FT; // 3.0f* NM_TO_FT;
     }
     else
     {// this must be something unthreatening...it's below 250 knots
     MAR  = g_fMinMARNoIdC*NM_TO_FT; // 5.0f * NM_TO_FT;
     }
     }

     DOR = MAR + 10.0f* NM_TO_FT;*/
}

int DigitalBrain::IsSupportignmissile(void)
{
    int result = 0;
    // 0 noone has a missile in the air or its post pitbull
    // 1 self pre pitbull
    // 2 wingie prepitbull
    // 3 both pre pitbull
    AircraftClass *wingman = NULL;
    AircraftClass *elementlead = NULL;

    // find the pointers to wingie, lead and element lead
    //the Isflightleads wingie
    if (self->vehicleInUnit == 0)
        wingman = (AircraftClass *)self->GetCampaignObject()->GetComponentNumber(1);

    // the elements lead wingie
    if (self->vehicleInUnit == 2)
        wingman = (AircraftClass *)self->GetCampaignObject()->GetComponentNumber(3);

    //me123 advanced bvr tactics we have a missile in the air
    // ADDED BY S.G. SO DIGI PILOT SHOOTING SARH WONT GET SCARED RIGHT AWAY
    RadarClass* radar = (RadarClass*)FindSensor(self, SensorClass::Radar);

    if (radar->CurrentTarget() and radar->CurrentTarget()->localData->sensorState[SensorClass::Radar] == SensorClass::NoTrack)
    {
        if (missileFiredEntity)
            VuDeReferenceEntity(missileFiredEntity); // 2002-03-13 ADDED BY S.G. Must dereference it or it will cause memory leak...

        missileFiredEntity = NULL;
    }

    // MODIFIED BY S.G. Can't rely on this... I've seen missile still being guided that told they were not being guided. Use the radar of the missile (can't miss with that)
    if (missileFiredEntity and ((SimWeaponClass *)missileFiredEntity)->sensorArray[0]->Type() == SensorClass::RadarHoming and ((SimWeaponClass *)missileFiredEntity)->GetSPType() not_eq SPTYPE_AIM120)
        result = 1;

    /* if (missileFiredEntity and 
     (((SimWeaponClass *)missileFiredEntity)->sensorArray[0]->Type() == SensorClass::RadarHoming or
     ((SimWeaponClass *)missileFiredEntity)->sensorArray[0]->Type() == SensorClass::Radar ))
     {
     if (self->FCC->lastMissileImpactTime > (((MissileClass *)missileFiredEntity)->GetActiveTime(0.0f, 0.0f, 0.0f, 0.0f, 0.0f)))
     result = 1;
     else if (((SimWeaponClass *)missileFiredEntity)->sensorArray[0]->Type() == SensorClass::Radar)
     result = result;
     }
    */
    // MODIFIED BY S.G. Can't rely on this... I've seen missile still being guided that told they were not being guided. Use the radar of the missile (can't miss with that)
    if (wingman and wingman->DBrain()->missileFiredEntity and ((SimWeaponClass *)wingman->DBrain()->missileFiredEntity)->sensorArray[0]->Type() == SensorClass::RadarHoming)
        result += 2;

    /* if (wingman and 
     wingman->DBrain()->missileFiredEntity and 
     (((SimWeaponClass *)wingman->DBrain()->missileFiredEntity)->sensorArray[0]->Type() == SensorClass::RadarHoming or
     ((SimWeaponClass *)wingman->DBrain()->missileFiredEntity)->sensorArray[0]->Type() == SensorClass::Radar ))
     {
     if (wingman->FCC->lastMissileImpactTime > (((MissileClass *)wingman->DBrain()->missileFiredEntity)->GetActiveTime(0.0f, 0.0f, 0.0f, 0.0f, 0.0f)))
     result += 2;
     else if (((SimWeaponClass *)wingman->DBrain()->missileFiredEntity)->sensorArray[0]->Type() == SensorClass::Radar)
     result = result;
     }
    */
    return result;
}
int DigitalBrain::IsSplitup(void)
{
    int result = 0;
    AircraftClass *otherelementlead = NULL;

    if (self->vehicleInUnit == 0)
    {
        otherelementlead = (AircraftClass *)self->GetCampaignObject()->GetComponentNumber(2);
    }
    else if (self->vehicleInUnit == 2)
    {
        otherelementlead = (AircraftClass *)self->GetCampaignObject()->GetComponentNumber(0);
    }

    if ( not otherelementlead) return result;

    float  rngSq = (self->XPos() - otherelementlead->XPos()) * (self->XPos() - otherelementlead->XPos()) +
                   (self->YPos() - otherelementlead->YPos()) * (self->YPos() - otherelementlead->YPos()) +
                   (self->ZPos() - otherelementlead->ZPos()) * (self->ZPos() - otherelementlead->ZPos());
    result = (int)rngSq;
    return result;
}

int DigitalBrain::HowManyTargetet(void)
{
    int result = 0;
    AircraftClass *elementlead = NULL;
    AircraftClass *Mywing = NULL;
    AircraftClass *Elementwing = NULL;

    if (self->vehicleInUnit == 0)
    {
        elementlead = (AircraftClass *)self->GetCampaignObject()->GetComponentNumber(2);
        Mywing = (AircraftClass *)self->GetCampaignObject()->GetComponentNumber(1);
        Elementwing = (AircraftClass *)self->GetCampaignObject()->GetComponentNumber(3);
    }

    //we are element lead
    if (self->vehicleInUnit == 2)
    {
        elementlead = (AircraftClass *)self->GetCampaignObject()->GetComponentNumber(0);
        Mywing = (AircraftClass *)self->GetCampaignObject()->GetComponentNumber(3);
        Elementwing = (AircraftClass *)self->GetCampaignObject()->GetComponentNumber(1);
    }

    if (self->targetPtr) result++;

    if (Mywing and Mywing->targetPtr)
    {
        if (Mywing->targetPtr not_eq self->targetPtr) result++;
    }

    if (elementlead and elementlead->targetPtr)
    {
        if (elementlead->targetPtr not_eq self->targetPtr)
        {
            if (Mywing and elementlead->targetPtr not_eq Mywing->targetPtr) result++;
            else if ( not Mywing)result++;
        }
    }

    if (Elementwing and Elementwing->targetPtr)
    {
        if (Elementwing->targetPtr not_eq self->targetPtr)// not my target
        {
            if (elementlead and elementlead->targetPtr and Elementwing->targetPtr not_eq elementlead->targetPtr)
            {
                // its not our element leads target
                if (Mywing and Elementwing->targetPtr not_eq Mywing->targetPtr)result++;
                else if ( not Mywing)result++;
            }
            else if ( not elementlead)
            {
                // its not my wings target
                if (Mywing and Elementwing->targetPtr not_eq Mywing->targetPtr)result++;
                else if ( not Mywing)result++;
            }
        }
    }

    return result;
}

int DigitalBrain::HowManyHotnosed(void)
{
    int result = 0;
    AircraftClass *elementlead = NULL;
    AircraftClass *Mywing = NULL;
    AircraftClass *Elementwing = NULL;

    if (self->vehicleInUnit == 0)
    {
        elementlead = (AircraftClass *)self->GetCampaignObject()->GetComponentNumber(2);
        Mywing = (AircraftClass *)self->GetCampaignObject()->GetComponentNumber(1);
        Elementwing = (AircraftClass *)self->GetCampaignObject()->GetComponentNumber(3);
    }

    //we are element lead
    if (self->vehicleInUnit == 2)
    {
        elementlead = (AircraftClass *)self->GetCampaignObject()->GetComponentNumber(0);
        Mywing = (AircraftClass *)self->GetCampaignObject()->GetComponentNumber(3);
        Elementwing = (AircraftClass *)self->GetCampaignObject()->GetComponentNumber(1);
    }

    if (self->targetPtr and self->targetPtr->localData->ataFrom < 50 * DTR) result++;

    if (Mywing and Mywing->targetPtr and Mywing->targetPtr->localData->ataFrom < 50 * DTR) result++;

    if (elementlead and elementlead->targetPtr and elementlead->targetPtr->localData->ataFrom < 50 * DTR) result++;

    if (Elementwing and Elementwing->targetPtr and Elementwing->targetPtr->localData->ataFrom < 50 * DTR)result++;

    return result;
}

int DigitalBrain::WhoIsHotnosed(void)
{
    int result = 0;
    AircraftClass *elementlead = NULL;
    AircraftClass *Mywing = NULL;
    AircraftClass *Elementwing = NULL;

    if (self->vehicleInUnit == 0)
    {
        elementlead = (AircraftClass *)self->GetCampaignObject()->GetComponentNumber(2);
        Mywing = (AircraftClass *)self->GetCampaignObject()->GetComponentNumber(1);
        Elementwing = (AircraftClass *)self->GetCampaignObject()->GetComponentNumber(3);
    }

    //we are element lead
    if (self->vehicleInUnit == 2)
    {
        elementlead = (AircraftClass *)self->GetCampaignObject()->GetComponentNumber(0);
        Mywing = (AircraftClass *)self->GetCampaignObject()->GetComponentNumber(3);
        Elementwing = (AircraftClass *)self->GetCampaignObject()->GetComponentNumber(1);
    }

    if (self->targetPtr and self->targetPtr->localData->ataFrom < g_fHotNoseAngle * DTR) // 2002-03-22 MODIFIED BY S.G. Here and below, replaced 50 by g_fHotNoseAngle so it's configurable
        result += 8;

    if (Mywing and Mywing->targetPtr and Mywing->targetPtr->localData->ataFrom < g_fHotNoseAngle * DTR)
        result += 4;

    if (elementlead and elementlead->targetPtr and elementlead->targetPtr->localData->ataFrom < g_fHotNoseAngle * DTR)
        result += 2;

    if (Elementwing and Elementwing->targetPtr and Elementwing->targetPtr->localData->ataFrom < g_fHotNoseAngle * DTR)
        result += 1;

    return result;
}

void DigitalBrain::DoProfile(void)
{
    if ( not flightLead) return;

    if (((AircraftClass*)flightLead)->DBrain()->bvrCurrProfile == Pnone)
    {
        if (Isflightlead)
        {
            // 2002-03-15 ADDED BY S.G. If the flightLead is a player and NOT in Combat AP, then it CAN'T run ChoiceProfile. Default to Plevel1c which is a pure pursuit for the element
            if (flightLead == self and self->IsPlayer() and self->AutopilotType() not_eq AircraftClass::CombatAP)
                bvrCurrProfile = Plevel1c;
            else
                // END OF ADDED SECTION 2002-03-15
                ChoiceProfile();
        }
    }
    else if (((AircraftClass*)flightLead)->DBrain()->bvrCurrProfile == Plevel1a)
    {
        level1a();
    }
    else if (((AircraftClass*)flightLead)->DBrain()->bvrCurrProfile == Plevel1b)
    {
        level1b();
    }
    else if (((AircraftClass*)flightLead)->DBrain()->bvrCurrProfile == Plevel1c)
    {
        level1c();
    }
    else if (((AircraftClass*)flightLead)->DBrain()->bvrCurrProfile == Plevel2a)
    {
        level2a();
    }
    else if (((AircraftClass*)flightLead)->DBrain()->bvrCurrProfile == Plevel2b)
    {
        level2b();
    }
    else if (((AircraftClass*)flightLead)->DBrain()->bvrCurrProfile == Plevel2c)
    {
        level2c();
    }
    else if (((AircraftClass*)flightLead)->DBrain()->bvrCurrProfile == Plevel3a)
    {
        level3a();
    }
    else if (((AircraftClass*)flightLead)->DBrain()->bvrCurrProfile == Plevel3b)
    {
        level3b();
    }
    else if (((AircraftClass*)flightLead)->DBrain()->bvrCurrProfile == Plevel3c)
    {
        level3c();
    }
    else if (((AircraftClass*)flightLead)->DBrain()->bvrCurrProfile == Pbeamdeploy)
    {
        beamdeploy();
    }
    else if (((AircraftClass*)flightLead)->DBrain()->bvrCurrProfile == Pbeambeam)
    {
        beambeam();
    }
    else if (((AircraftClass*)flightLead)->DBrain()->bvrCurrProfile == Pwall)
    {
        wall();
    }
    else if (((AircraftClass*)flightLead)->DBrain()->bvrCurrProfile == Pgrinder)
    {
        grinder();
    }
    else if (((AircraftClass*)flightLead)->DBrain()->bvrCurrProfile == Pwideazimuth)
    {
        wideazimuth();
    }
    else if (((AircraftClass*)flightLead)->DBrain()->bvrCurrProfile == Pshortazimuth)
    {
        shortazimuth();
    }
    else if (((AircraftClass*)flightLead)->DBrain()->bvrCurrProfile == PwideLT)
    {
        wideLT();
    }
    else if (((AircraftClass*)flightLead)->DBrain()->bvrCurrProfile == PShortLT)
    {
        ShortLT();
    }
    else if (((AircraftClass*)flightLead)->DBrain()->bvrCurrProfile == PDefensive)
    {
        Defensive();
    }
    else if (((AircraftClass*)flightLead)->DBrain()->bvrCurrProfile == PSweep)
    {
        Sweep();
    }
    else assert(0);
}
void DigitalBrain::BaseLineIntercept(void)
{
    //only with radar sa 
    if (fabs(targetData->azFrom) < 30 * DTR or
        fabs(targetData->azFrom) > 40 * DTR and 
        fabs(targetData->azFrom) < 160 * DTR)
    {
        //offset

        float az;
        mlTrig trig;

        if (reactiont > 3.0f) reactiont = 3.0f;

        /*-----------------*/
        /*  heading to target*/
        /*-----------------*/
        az = self->Yaw() + TargetAz(self, targetPtr) ;

        if (az > PI)
            az -= 2 * PI;

        if (az < -PI)
            az += 2 * PI;

        /*--------------------*/
        /* pick a new heading */
        /*--------------------*/
        if (fabs(targetData->azFrom) < 35 * DTR)
        {
            //offset
            if (targetData->azFrom < 0.0f)
            {
                az = az + 45.0f * DTR;
                offsetdir = offRight;
#ifdef DEBUG_INTERCEPT
                MonoPrint("offset +");
#endif
            }
            else
            {
                az = az - 45.0f * DTR;
                offsetdir = offLeft;
#ifdef DEBUG_INTERCEPT
                MonoPrint("offset -");
#endif
            }
        }
        else
        {
            //collision
            if (targetData->azFrom > 0.0f)
            {
                az = az + 45.0f * DTR;
#ifdef DEBUG_INTERCEPT
                MonoPrint("collision +");
#endif
            }

            else
            {
                az = az - 45.0f * DTR;
#ifdef DEBUG_INTERCEPT
                MonoPrint("collision -");
#endif
            }
        }

        /*------------------------------------*/
        /* find a stationary point 10 NM away */
        /* in the nh? direction.              */
        /*------------------------------------*/

        if (az > PI)
            az -= 2 * PI;

        if (az < -PI)
            az += 2 * PI;

        mlSinCos(&trig, az);
        reactiont -= SimLibMajorFrameTime;

        if (reactiont < 0)
        {
            SetTrackPoint(self->XPos() + 10.0F * NM_TO_FT * trig.cos, self->YPos() + 10.0F * NM_TO_FT * trig.sin);
            reactiont = 3.0f;
        }

        StickandThrottle(-1, trackZ);
    }
    else
    {
        // pure
        offsetdir = offForward;
        reactiont -= SimLibMajorFrameTime;

        if (reactiont < 0)
        {
            SetTrackPoint(targetPtr->BaseData()->XPos(), trackY = targetPtr->BaseData()->YPos());
        }

        float wpX, wpY, wpZ = 4000.0f;

        if (self->curWaypoint)self->curWaypoint->GetLocation(&wpX, &wpY, &wpZ);

        trackZ = min(max(targetPtr->BaseData()->ZPos() - 10000.0f, wpZ), targetPtr->BaseData()->ZPos() + 10000.0f);
        StickandThrottle(-1, trackZ);
        reactiont = 3.0f;
#ifdef DEBUG_INTERCEPT
        MonoPrint("PURE");
#endif
    }

    ShiAssert(trackZ < 0.0F);
}
int DigitalBrain::BeamManeuver(int direction, int NotchHI)
{
    int retval = FALSE;
    float nh1, nh2, az;
    mlTrig trig;

    /*-----------------*/
    /* threat heading */
    /*-----------------*/
    az = self->Yaw() + TargetAz(self, targetPtr);

    if (az > PI)
        az -= 2 * PI;

    if (az < -PI)
        az += 2 * PI;

    /*--------------------*/
    /* pick a new heading */
    /*--------------------*/

    nh1 = az + 90.0f * DTR;

    if (nh1 > PI)
        nh1 -= 2 * PI;

    nh2 = az - 90.0f * DTR;

    if (nh2 < -PI)
        nh2 += 2 * PI;

    if (direction == offRight or (TargetAz(self, targetPtr) < 0 and direction not_eq offLeft))
    {
        az = nh1;
        offsetdir = offRight;
    }
    else
    {
        az = nh2;
        offsetdir = offLeft;
    }

    mlSinCos(&trig, az);
    SetTrackPoint(self->XPos() + 4.0F * NM_TO_FT * trig.cos, self->YPos() + 4.0F * NM_TO_FT * trig.sin);

    static VU_TIME heighttimer = 0;

    if (
        heighttimer + 15 * CampaignSeconds < SimLibElapsedTime and 
        (TargetAz(self, targetPtr) > 80.0f * DTR and TargetAz(self, targetPtr) < 100.0f * DTR or
         TargetAz(self, targetPtr) < -80.0f * DTR and TargetAz(self, targetPtr) > -100.0f * DTR)
    )
    {
        heighttimer = SimLibElapsedTime;

        if (
 not direction and 
            bvrTacticTimer > SimLibElapsedTime + 5 * CampaignSeconds - SkillLevel()* CampaignSeconds and 
            SpikeCheck(self) not_eq targetPtr->BaseData()
        )
        {
            bvrTacticTimer = SimLibElapsedTime + 5 * CampaignSeconds - SkillLevel() * CampaignSeconds;
        }
    }

    /*----------------------------------------------------------*/
    /* use AUTO_TRACK to steer toward point, override gs limits */
    /*----------------------------------------------------------*/
    // VP_changes this part should be evaluated. 12.11.2002
    static VU_TIME chaffttimer = 0;

    if (
        SpikeCheck(self) and 
        SpikeCheck(self) == targetPtr->BaseData() and 
        chaffttimer + 1.5 * CampaignSeconds < SimLibElapsedTime
    )
    {
        ((AircraftClass*)self)->dropChaffCmd = TRUE;
        chaffttimer = SimLibElapsedTime;
    }

    if ( not NotchHI)
    {
        trackZ = max(-10000.0f + (float)SkillLevel() * 2000.0f, self->ZPos());
        StickandThrottle(cornerSpeed, trackZ);
    }
    else
    {
        // 2002-03-14 MODIFIED BY S.G. What about plane that can't go that high?
        // StickandThrottle (-3, -50000);
        VehicleClassDataType *vc = GetVehicleClassData(self->Type() - VU_LAST_ENTITY_TYPE);
        float maxAlt = max(vc->HighAlt * -100.0f, -50000.0f);
        StickandThrottle(-3, maxAlt);
    }

    //StickandThrottle (cornerSpeed, trackZ);
    retval = TRUE;
    ShiAssert(trackZ < 0.0F);
    return (retval);
}
void DigitalBrain::CrankManeuver(int direction, int Height)//me123 //Cobra add height
{
    int retval = FALSE;
    float nh1, nh2, az;
    mlTrig trig;

    if (targetPtr->localData->ataFrom > 90.0f * DTR)
    {
        SetTrackPoint(targetPtr->BaseData()->XPos(), targetPtr->BaseData()->YPos(), targetPtr->BaseData()->ZPos());
        float trackSpeed = targetPtr->BaseData()->GetKias() + 100.0f;
        StickandThrottle(trackSpeed, trackZ);
        return;
    }
    else
    {
        az = self->Yaw() + TargetAz(self, targetPtr) ;

        if (az > PI)
        {
            az -= 2 * PI;
        }

        if (az < -PI)
        {
            az += 2 * PI;
        }

        /*--------------------*/
        /* pick a new heading */
        /*--------------------*/
        nh1 = az + 45.0f * DTR;

        if (nh1 > PI)
        {
            nh1 -= 2 * PI;
        }

        nh2 = az - 45.0f * DTR;

        if (nh2 < -PI)
        {
            nh2 += 2 * PI;
        }

        if (direction == offRight or (targetPtr->localData->azFrom < 0.0f and direction not_eq offLeft))
        {
            az = nh1;
            offsetdir = offRight;
        }
        else
        {
            az = nh2;
            offsetdir = offLeft;
        }

        mlSinCos(&trig, az);
        SetTrackPoint(self->XPos() + 10.0F * NM_TO_FT * trig.cos, self->YPos() + 10.0F * NM_TO_FT * trig.sin);
    }

    if ((self->GetVt() < 200.0f * KNOTS_TO_FTPSEC) and Height == 1)
    {
        Height = 2;
    }

    // Cobra - Use local max elevation to try and keep AI from lawndarting
    trackZ = -TheMap.GetMEA(((AircraftClass*) self)->XPos(), ((AircraftClass*) self)->YPos());

    if (Height == 1)//Up
    {
        trackZ = min(trackZ - 30000.0f, self->ZPos());
    }
    else if (Height == 2)//Down
    {
        trackZ -= 2000.0f;
    }
    else
    {
        trackZ = self->ZPos();
    }

    StickandThrottle(cornerSpeed, trackZ);
    ShiAssert(trackZ < 0.0F);
}
void DigitalBrain::DragManeuver(void)
{
    float az;
    mlTrig trig;

    az = targetPtr->BaseData()->Yaw() + targetPtr->localData->azFrom ;

    spikeseconds = NULL;  //resetign the spike timer here, asumign that the possible
    //missile in the air is trashed by the drag

    if (az > PI)
    {
        az -= 2 * PI;
    }

    if (az < -PI)
    {
        az += 2 * PI;
    }

    mlSinCos(&trig, az);
    SetTrackPoint(self->XPos() + 10.0F * NM_TO_FT * trig.cos, self->YPos() + 10.0F * NM_TO_FT * trig.sin);

    // sfr: wth is this??
    //static VU_TIME heighttimer = 0;

    StickandThrottle(-2, trackZ);
}

//priority 1 is speed, 2 is altitude, 0 is none
void DigitalBrain::StickandThrottle(float DesiredSpeed, float DesiredAltitude)
{
    float sensitivityDecent = 20.0f;
    float sensitivityClimp = 800.0f;
    float speeddifference;
    int MaxEnergyMode = 1;

    if (DesiredSpeed == -1)
    {
        DesiredSpeed = ((AircraftClass*)self)->af->GetOptKias(0) * 1.05f; // 2002-03-15 MODIFIED BY S.G. From 1.4 to 1.05. Plane will have trouble getting there and will burn too much fuel trying to
        MaxEnergyMode = 2;
    }

    if (DesiredSpeed == -2)
    {
        DesiredSpeed = ((AircraftClass*)self)->af->MaxVcas();
        MaxEnergyMode = 2;
    }

    if (DesiredSpeed == -3)
    {
        // for notch high maneuver
        // DesiredSpeed = ((AircraftClass*)self)->af->MinVcas();
        DesiredSpeed = ((AircraftClass*)self)->af->GetOptKias(0); // 2002-03-14 MODIFIED BY S.G. If we have to climb, why not find our best climb speed?
        MaxEnergyMode = 10;
        sensitivityClimp *= 10;
    }

    speeddifference = DesiredSpeed - self->GetKias();

    // Cobra - Use local max elevation to try and keep AI from lawndarting
    float trkZ = TheMap.GetMEA(((AircraftClass*) self)->XPos(), ((AircraftClass*) self)->YPos());


    if (speeddifference > 50.0f)
    {
        // we need speed nomatter what sell altitude...speed has priority here
        if (trackZ < -4000.0f - trkZ)
            trackZ = self->ZPos();
    }
    else
    {
        if (-self->ZPos() < -DesiredAltitude)
        {
            trackZ = min(self->ZPos() + (speeddifference * sensitivityClimp) , DesiredAltitude);
            // trackZ = max (trackZ,-45000.0f);
        }
        else
        {
            trackZ = DesiredAltitude;
            trackZ = min(trackZ, -4000.0f);
        }
    }


    AutoTrack(4.0f);
    // MachHold(DesiredSpeed*MaxEnergyMode, self->GetKias(), TRUE);
    MachHold(DesiredSpeed, self->GetKias(), TRUE); // 2002-03-14 S.G. Not sure what RIK was trying to do here but this 'MaxEnergyMode' makes MachHold go into overtime trying to go really really fast :-(
    ShiAssert(trackZ < 0.0F);
}

void DigitalBrain::AiFlyBvrFOrmation(void)
{
    ACFormationData::PositionData *curPosition;
    float rangeFactor;
    float groundZ;
    int vehInFlight;
    int flightIdx;
    bool iamelementlead = false;
    AircraftClass* paircraft;
    // Get wingman slot position relative to the leader
    vehInFlight = ((FlightClass*)self->GetCampaignObject())->GetTotalVehicles();
    flightIdx = ((FlightClass*)self->GetCampaignObject())->GetComponentIndex(self);

    if (flightIdx == AiFirstWing and vehInFlight == 2)
    {
        curPosition = &(acFormationData->twoposData[mFormation]); // The four ship #2 slot position is copied in to the 2 ship formation array.
        paircraft = (AircraftClass*) flightLead;
    }
    else if (flightIdx == AiSecondWing)
    {
        curPosition = &(acFormationData->twoposData[mFormation]);
        paircraft = (AircraftClass*)((FlightClass*)self->GetCampaignObject())->GetComponentEntity(AiElementLead);
    }
    else
    {
        curPosition = &(acFormationData->positionData[mFormation][flightIdx - 1]);
        paircraft = (AircraftClass*) flightLead;
        iamelementlead = true;
    }

    rangeFactor = curPosition->range * (mFormLateralSpaceFactor);

    if ((AircraftClass*)flightLead and ((AircraftClass*)flightLead)->DBrain()->bvrCurrProfile == Pwall) rangeFactor *= 6;

    // Get my leader's position
    ShiAssert(paircraft);

    if (paircraft)
    {
        trackX = paircraft->XPos();
        trackY = paircraft->YPos();
        trackZ = paircraft->ZPos();

        // Calculate position relative to the leader
        trackX += rangeFactor * (float)cos(curPosition->relAz * mFormSide + paircraft->af->sigma);
        trackY += rangeFactor * (float)sin(curPosition->relAz * mFormSide + paircraft->af->sigma);

        if (curPosition->relEl)
        {
            trackZ += rangeFactor * (float)sin(-curPosition->relEl);
        }
        else
        {
            trackZ += (flightIdx * -100.0F);
        }

        AiCheckInPositionCall(trackX, trackY, trackZ);

        // Set track point 1NM ahead of desired location
        trackX += 1.0F * NM_TO_FT * (float)cos((paircraft)->af->sigma);
        trackY += 1.0F * NM_TO_FT * (float)sin((paircraft)->af->sigma);
    }

    // check for terrain following
    groundZ = OTWDriver.GetApproxGroundLevel(self->XPos() + self->XDelta(),
              self->YPos() + self->YDelta());

    if (self->ZPos() - groundZ > -1000.0f)
    {
        groundZ = OTWDriver.GetGroundLevel(self->XPos() + self->XDelta(),
                                           self->YPos() + self->YDelta());

        if (trackZ - groundZ > -800.0f)
        {
            if (self->ZPos() - groundZ > -800.0f)
                trackZ = groundZ - 800.0f - (self->ZPos() - groundZ + 800.0f) * 2.0f;
            else
                trackZ = groundZ - 800.0f;
        }
    }

    float xft;
    float yft;
    float zft;
    float rx;
    float ry;
    float rz;

    ShiAssert(trackZ < 0.0F);
    CalculateRelativePos(&xft, &yft, &zft, &rx, &ry, &rz);
    // SimpleTrack(SimpleTrackDist, 0.0F);
    ShiAssert(flightLead);

    if (flightLead)
        SimpleTrackDistance(flightLead->GetVt(), (float)sqrt(xft * xft + yft * yft));

    AutoTrack(4.0f);
}
void DigitalBrain::chooseRadarMode(void)
{
    //In this function we select the proper radar mode based on radModeSelect
    //Other functions will set radModeSelect and here we set the radar mode.
    int hasTWS = 0;

    // enum DigiRadarMode {DigiSTT, DigiSAM, DigiTWS, DigiRWS, DigiOFF};

    if (SimLibElapsedTime > radarModeTest)
    {
        RadarClass* theRadar = (RadarClass*)FindSensor(self, SensorClass::Radar);

        if (theRadar)
        {
            RadarDataSet* radarData = &radarDatFileTable[self->GetRadarType()];

            if (radarData->MaxTwstargets > 0) // Must be equipped with a radar capable of doing TWS...
                hasTWS = 1;

            switch (radModeSelect)
            {
                default://RWS is the default mode
                    theRadar->digiRadarMode = RadarClass::DigiRWS;
                    break;

                case 0://STT
                    theRadar->digiRadarMode = RadarClass::DigiSTT;
                    break;

                case 1://SAM
                    theRadar->digiRadarMode = RadarClass::DigiSAM;
                    break;

                case 2://TWS
                    if (hasTWS)
                        theRadar->digiRadarMode = RadarClass::DigiTWS;
                    else
                        theRadar->digiRadarMode = RadarClass::DigiRWS;

                    break;

                case 3://RWS
                    theRadar->digiRadarMode = RadarClass::DigiRWS;
                    break;

                case 4://OFF
                    theRadar->digiRadarMode = RadarClass::DigiOFF;
                    break;

            }//end switch

            //Special Cases
            if (curMissile and curMissile->sensorArray and curMissile->sensorArray[0]->Type() == SensorClass::RadarHoming and curMissile->GetSPType() not_eq SPTYPE_AIM120)
                theRadar->digiRadarMode = RadarClass::DigiSTT;

        }

        radarModeTest = SimLibElapsedTime + (4 + (4 - SkillLevel())) * SEC_TO_MSEC;
    }






}//End
