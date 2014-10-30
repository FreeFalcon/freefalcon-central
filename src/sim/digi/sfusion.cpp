#include "stdhdr.h"
#include "classtbl.h"
#include "digi.h"
#include "sensors.h"
#include "simveh.h"
#include "missile.h"
#include "object.h"
#include "sensclas.h"
#include "Entity.h"
#include "team.h"
#include "Aircrft.h"
/* 2001-03-15 S.G. */#include "campbase.h"
/* 2001-03-21 S.G. */#include "flight.h"
/* 2001-03-21 S.G. */#include "atm.h"

#include "RWR.h" // 2002-02-11 S.G.
#include "Radar.h" // 2002-02-11 S.G.
#include "simdrive.h" // 2002-02-17 S.G.

#define MAX_NCTR_RANGE    (60.0F * NM_TO_FT) // 2002-02-12 S.G. See RadarDoppler.h

/* 2001-09-07 S.G. RP5 */ extern bool g_bRP5Comp;
extern int g_nLowestSkillForGCI; // 2002-03-12 S.G. Replaces the hardcoded '3' for skill test
extern bool g_bUseNewCanEnage; // 2002-03-11 S.G.
int GuestimateCombatClass(AircraftClass *self, FalconEntity *baseObj); // 2002-03-11 S.G.

FalconEntity* SpikeCheck(AircraftClass* self, FalconEntity *byHim = NULL, int *data = NULL); // 2002-02-10 S.G.

void DigitalBrain::SensorFusion(void)
{
    SimObjectType* obj = targetList;
    float turnTime = 0.0F, timeToRmax = 0.0F, rmax = 0.0F, tof = 0.0F, totV = 0.0F;
    SimObjectLocalData* localData = NULL;
    int relation = 0, pcId = ID_NONE, canSee = FALSE, i = 0;
    FalconEntity* baseObj = NULL;

    // 2002-04-18 REINSTATED BY S.G. After putting back 'or' instead of ' and ' before "localData->sensorLoopCount[self->sensorArray[i]->Type()] > delayTime" below, this is no longer required
    // 2002-02-17 MODIFIED BY S.G. Sensor routines for AI runs less often than SensorFusion therefore the AI will time out his target after this delayTime as elapsed.
    //                             By using the highest of both, I'm sure this will not happen...
    int delayTime = SimLibElapsedTime - 6 * SEC_TO_MSEC * (SkillLevel() + 1);

    /* int delayTime;
    unsigned int fromSkill = 6 * SEC_TO_MSEC * (SkillLevel() + 1);

     if (fromSkill > self->targetUpdateRate)
     delayTime = SimLibElapsedTime - fromSkill;
     else
     delayTime = SimLibElapsedTime - self->targetUpdateRate;
    */
    /*--------------------*/
    /* do for all objects */
    /*--------------------*/
    while (obj)
    {
        localData = obj->localData;
        baseObj = obj->BaseData();

        //if (F4IsBadCodePtr((FARPROC) baseObj)) // JB 010223 CTD
        if (F4IsBadCodePtr((FARPROC) baseObj) or F4IsBadReadPtr(baseObj, sizeof(FalconEntity))) // JB 010305 CTD
            break; // JB 010223 CTD

        // Check all sensors for contact
        canSee = FALSE;//PUt to true for testing only

        //Cobra Begin rebuilding this function.

        //GCI Code
        CampBaseClass *campBaseObj = (CampBaseClass *)baseObj;

        if (baseObj->IsSim())
            campBaseObj = ((SimBaseClass*)baseObj)->GetCampaignObject();

        // If the object is a weapon, don't do GCI on it
        if (baseObj->IsWeapon())
            campBaseObj = NULL;

        // Only if we have a valid base object...
        // This code is to make sure our GCI targets are prioritized, just like other targets
        if (campBaseObj)
            if (campBaseObj->GetSpotted(self->GetTeam()))
                canSee = TRUE;

        if (localData->sensorState[SensorClass::RWR] >= SensorClass::SensorTrack)
        {
            canSee = TRUE;
            detRWR = 1;
        }
        else
            detRWR = 0;

        if (localData->sensorState[SensorClass::Radar] >= SensorClass::SensorTrack)
        {
            canSee = TRUE;
            detRAD = 1;
        }
        else
            detRAD = 0;

        if (localData->sensorState[SensorClass::Visual] >= SensorClass::SensorTrack)
        {
            canSee = TRUE;
            detVIS = 1;
        }
        else
            detVIS = 0;



        //End




        /* if ( not g_bRP5Comp) {
          // Aces get to use GCI
          // Idiots find out about you inside 1 mile anyway
          if (localData->range > 3.0F * NM_TO_FT and // gci is crap inside 3nm
          (SkillLevel() >= 2 and 
           localData->range < 25.0F * NM_TO_FTor
           SkillLevel() >=3  and 
           localData->range < 35.0F * NM_TO_FTor
           SkillLevel() >=4  and 
           localData->range < 55.0F * NM_TO_FT)
           )//me123 not if no sensor has seen it or localData->range < 1.0F * NM_TO_FT)
          {
          canSee = TRUE;
          }

          // You can always see your designated target
          if (baseObj->Id() == mDesignatedObject and localData->range > 8.0F * NM_TO_FT)
          {
          canSee = TRUE;//me123
          }

          for (i = 0; i<self->numSensors and not canSee; i++)
          {
          if (localData->sensorState[self->sensorArray[i]->Type()] > SensorClass::NoTrack or
          localData->sensorLoopCount[self->sensorArray[i]->Type()] > delayTime)
          {
         canSee = TRUE;
         break;
          }
          }
         }
         else {*/
        // 2001-03-21 REDONE BY S.G. SO SIM AIRPLANE WIL FLAG FLIGHT/AIRPLANE AS DETECTED AND WILL PROVIDE GCI
        /*#if 0
         // Aces get to use GCI
         // Idiots find out about you inside 1 mile anyway
         if (SkillLevel() >= 3 and localData->range < 15.0F * NM_TO_FT or localData->range < 1.0F * NM_TO_FT)
         {
         canSee = TRUE;
         }

         // You can always see your designated target
         if (baseObj->Id() == mDesignatedObject and localData->range > 8.0F * NM_TO_FT)
         {
         canSee = TRUE;
         }

         for (i = 0; i<self->numSensors and not canSee; i++)
         {
         if (localData->sensorState[self->sensorArray[i]->Type()] > SensorClass::NoTrack or
         localData->sensorLoopCount[self->sensorArray[i]->Type()] > delayTime)
         {
         canSee = TRUE;
         break;
         }
         }
        #else */
        // First I'll get the campaign object if it's for a sim since I use it at many places...
        /*CampBaseClass *campBaseObj = (CampBaseClass *)baseObj;

        if (baseObj->IsSim())
         campBaseObj = ((SimBaseClass*)baseObj)->GetCampaignObject();

        // If the object is a weapon, don't do GCI on it
        if (baseObj->IsWeapon())
         campBaseObj = NULL;

        // This is our GCI implementation... Ace and Veteran gets to use GCI.
        // Only if we have a valid base object...
        // This code is to make sure our GCI targets are prioritized, just like other targets
        if (campBaseObj and SkillLevel() >= g_nLowestSkillForGCI and localData->range < 30.0F * NM_TO_FT)
         if (campBaseObj->GetSpotted(self->GetTeam()))
         canSee = TRUE;
        // You can always see your designated target
        if (baseObj->Id() == mDesignatedObject and localData->range > 8.0F * NM_TO_FT)
         canSee = TRUE;*/

        //if (SimDriver.RunningDogfight()) // 2002-02-17 ADDED BY S.G. If in dogfight, don't loose sight of your opponent.
        //canSee = TRUE; //Cobra removed to test

        // Go through all your sensors. If you 'see' the target and are bright enough, flag it as spotted and ask for an intercept if this FLIGHT is spotted for the first time...
        //for (i = 0; i<self->numSensors; i++) {
        //if (localData->sensorState[self->sensorArray[i]->Type()] > SensorClass::NoTrack or localData->sensorLoopCount[self->sensorArray[i]->Type()] > delayTime) { // 2002-04-18 MODIFIED BY S.G. Reverted to and instead of or. *MY* logic was flawed. It gaves a 'delay' (grace period) after the sensor becomes 'NoLock'.
        //if (campBaseObj and /* and SkillLevel() >= g_nLowestSkillForGCI and */ not ((UnitClass *)self->GetCampaignObject())->Broken()) {//Cobra removed GCI test here...not needed
        //if ( not campBaseObj->GetSpotted(self->GetTeam()) and campBaseObj->IsFlight())
        //RequestIntercept((FlightClass *)campBaseObj, self->GetTeam());

        // 2002-02-11 ADDED BY S.G. If the sensor can identify the target, mark it identified as well
        /*int identified = FALSE;

        if (self->sensorArray[i]->Type() == SensorClass::RWR) {
         if (((RwrClass *)self->sensorArray[i])->GetTypeData()->flag bitand RWR_EXACT_TYPE)
         identified = TRUE;
        }
        else if (self->sensorArray[i]->Type() == SensorClass::Radar) {
         if (((RadarClass *)self->sensorArray[i])->GetRadarDatFile() and (((RadarClass *)self->sensorArray[i])->radarData->flag bitand RAD_NCTR) and localData->ataFrom < 45.0f * DTR and localData->range < ((RadarClass *)self->sensorArray[i])->GetRadarDatFile()->MaxNctrRange / (2.0f * (16.0f - (float)SkillLevel()) / 16.0f)) // 2002-03-05 MODIFIED BY S.G. target's aspect and skill used in the equation
         identified = TRUE;
        }
        else
         identified = TRUE;

        campBaseObj->SetSpotted(self->GetTeam(),TheCampaign.CurrentTime, identified);
        }
        //canSee = TRUE;  //Cobra we are removing these to test, this gave everything can see
        //break;
        continue;
        }
        }
        //#endif
        }*/
        /*--------------------------------------------------*/
        /* Sensor id state                                  */
        /* RWR ids coming from RWR_INTERP can be incorrect. */
        /* Visual identification is 100% correct.           */
        /*--------------------------------------------------*/
        if (canSee)
        {

            //Cobra moved spotted stuff here
            CampBaseClass *campBaseObj = (CampBaseClass *)baseObj;

            if (baseObj->IsSim())
            {
                campBaseObj = ((SimBaseClass*)baseObj)->GetCampaignObject();

                if (campBaseObj)
                    campBaseObj->SetSpotted(self->GetTeam(), TheCampaign.CurrentTime, 1);
            }


            if (baseObj->IsMissile())
            {
                pcId = ID_MISSILE;
            }
            else if (baseObj->IsBomb())
            {
                pcId = ID_NEUTRAL;
            }
            else
            {
                if (TeamInfo[self->GetTeam()]) // JB 010617 CTD
                {
                    relation = TeamInfo[self->GetTeam()]->TStance(obj->BaseData()->GetTeam());

                    switch (relation)
                    {
                        case Hostile:
                        case War:
                            pcId = ID_HOSTILE;
                            break;

                        case Allied:
                        case Friendly:
                            pcId = ID_FRIENDLY;
                            break;

                        case Neutral:
                            pcId = ID_NEUTRAL;
                            break;
                    }
                }
            }
        }

        //Cobra Rewrite. Score threats
        if (canSee)
        {
            int hisCombatClass = -1;
            bool isHelo = FALSE;
            float threatRng = 0.0f;
            int totalThreat = 0;

            if (baseObj)
            {
                hisCombatClass = baseObj->CombatClass();

                if (baseObj->IsHelicopter())
                    isHelo = TRUE;
            }

            if (pcId == ID_HOSTILE)//Something we can shoot at
            {
                //Score combatclass

                if (hisCombatClass <= 4 and hisCombatClass >= 2)
                    totalThreat += 50;
                else
                    totalThreat += 30;

                if (localData->ataFrom > 90 * DTR)
                    totalThreat = totalThreat / 2;

                if (localData->range < maxAAWpnRange)
                    totalThreat += 20;

                if (missionType == AMIS_BARCAP or missionType == AMIS_BARCAP2 or missionComplete
                    or (missionClass == AGMission and not IsSetATC(HasAGWeapon)))
                {
                    if (isHelo or hisCombatClass >= 7)
                        totalThreat = 5;
                }
                else if (isHelo or hisCombatClass >= 7)
                    totalThreat = 0;


                //is this our target?
                CampBaseClass *campObj;

                if (baseObj->IsSim())
                    campObj = ((SimBaseClass *)baseObj)->GetCampaignObject();
                else
                    campObj = (CampBaseClass *)baseObj;

                int isMissionTarget = campObj and (((FlightClass *)(self->GetCampaignObject()))-> GetUnitMissionTargetID() == campObj->Id() or
                                                  ((FlightClass *)(self->GetCampaignObject()))->GetAssignedTarget() == campObj->Id());

                if (isMissionTarget)
                    totalThreat += 10;

                localData->threatScore = totalThreat;

            }
            else if (pcId == ID_MISSILE)
            {
                if (obj->BaseData()->GetTeam() == self->GetTeam())
                {
                    localData->threatScore = 0;
                }
                else
                {
                    localData->threatScore = 90;
                }

            }
            else
                localData->threatScore = 0;


        }//end cobra

        /*----------------------------------------------------*/
        /* Threat determination                               */
        /* Assume threat has your own longest range missile.  */
        /* Hypothetical time before we're in the mort locker. */
        /* If its a missile calculate time to impact.         */
        /*---------------------------------------------------*/
        /*
              localData->threatTime = 2.0F * MAX_THREAT_TIME;
              if (canSee)
              {

         if (baseObj->IsMissile())
                 {

                    if (pcId == ID_MISSILE)
                    {

                       if (obj->BaseData()->GetTeam() == self->GetTeam())
                       {
                          localData->threatTime = 2.0F * MAX_THREAT_TIME;
                       }
                       else
                       {
                          if (localData->sensorState[SensorClass::RWR] >= SensorClass::SensorTrack)
                            localData->threatTime = localData->range / AVE_AIM120_VEL;
                          else
         localData->threatTime = localData->range / AVE_AIM9L_VEL;
                       }
                    }
                    else localData->threatTime = MAX_THREAT_TIME;
                 }

                 else if ((baseObj->IsAirplane() or (baseObj->IsFlight() and not baseObj->IsHelicopter())) and pcId not_eq ID_NONE and pcId < ID_NEUTRAL and GuestimateCombatClass(self, baseObj) < MnvrClassA10)
                 {

          //TJL 11/07/03 VO log says there is an radian error in this code
          // I think it is here.  ataFrom is in radians
                    //turnTime = localData->ataFrom / FIVE_G_TURN_RATE;
          turnTime = localData->ataFrom*RTD / FIVE_G_TURN_RATE;// 15.9f degrees per second

          //TJL 11/07/03 Cos takes radians, thus no *DTR
                    //totV = obj->BaseData()->GetVt() + self->GetVt()*(float)cos(localData->ata*DTR);
         totV = obj->BaseData()->GetVt() + self->GetVt()*(float)cos(localData->ata);


         if (SpikeCheck(self) == obj->BaseData())//me123 addet
                    rmax = 2.5f*60762.11F;
         else
         rmax = 60762.11F;

                    if (localData->range > rmax)
                    {
               if ( totV <= 0.0f )
               {
                        timeToRmax = MAX_THREAT_TIME * 2.0f;
               }
               else
               {
                        timeToRmax = (localData->range - rmax) / totV;
                        tof = rmax / AVE_AIM120_VEL;
               }
                    }
                    else
                    {
                       timeToRmax = 0.0F;
                       tof = localData->range / AVE_AIM120_VEL;
                    }

                    localData->threatTime = turnTime + timeToRmax + tof;
                 }
                 else
                 {
                    localData->threatTime = 2.0F * MAX_THREAT_TIME;
                 }
              }

           */

        /*----------------------------------------------------*/
        /* Targetability determination                        */
        /* Use the longest range missile currently on board   */
        /* Hypothetical time before the tgt ac can be morted  */
        /*                                                    */
        /* Aircraft on own team are returned SENSOR_UNK       */
        /*----------------------------------------------------*/
        // 2002-03-05 MODIFIED BY S.G.  CombatClass is defined for FlightClass and AircraftClass now and is virtual in FalconEntity which will return 999
        // This code restrict the calculation of the missile range to either planes, chopper or flights. An aggregated chopper flight will have 'IsFlight' set so check if the 'AirUnitClass::IsHelicopter' function returned TRUE to screen them out from aircraft type test
        // Have to be at war against us
        // Chopper must be our assigned or mission target or we must be on sweep (not a AMIS_SWEEP but still has OnSweep set)
        // Must be worth shooting at, unless it's our assigned or mission target (new addition so AI can go after an AWACS for example if it's their target...
        //    if (canSee and baseObj->IsAirplane() and pcId < ID_NEUTRAL and 
        //       (IsSetATC(OnSweep) or ((AircraftClass*)baseObj)->CombatClass() < MnvrClassA10))
        // 2002-03-11 MODIFIED BY S.G. Don't call CombatClass directly but through GuestimateCombatClass which doesn't assume you have an ID on the target
        // Since I'm going to check for this twice in the next if statement, do it once here but also do the 'canSee' test which is not CPU intensive and will prevent the test from being performed if can't see.

        /*
           CampBaseClass *campObj;
           if (baseObj->IsSim())
           campObj = ((SimBaseClass *)baseObj)->GetCampaignObject();
           else
           campObj = (CampBaseClass *)baseObj;
           int isMissionTarget = canSee and campObj and (((FlightClass *)(self->GetCampaignObject()))->GetUnitMissionTargetID() == campObj->Id() or ((FlightClass *)(self->GetCampaignObject()))->GetAssignedTarget() == campObj->Id());

              if (canSee and 
           (baseObj->IsAirplane() or (baseObj->IsFlight() and not baseObj->IsHelicopter()) or (baseObj->IsHelicopter() and ((missionType not_eq AMIS_SWEEP and IsSetATC(OnSweep)) or isMissionTarget))) and 
           pcId < ID_NEUTRAL and 
           (GuestimateCombatClass(self, baseObj) < MnvrClassA10 or IsSetATC(OnSweep) or isMissionTarget)) // 2002-03-11 Don't assume you know the combat class
        // END OF MODIFIED SECTION 2002-03-05
              {
           // TJL 11/07/03 Cos takes Radians thus no *DTR
                 //totV     = obj->BaseData()->GetVt()*(float)cos(localData->ataFrom*DTR) + self->GetVt();
         totV     = obj->BaseData()->GetVt()*(float)cos(localData->ataFrom) + self->GetVt();

           //TJL 11/07/03 VO log says there is an radian error in this code
          // I think it is here.  ataFrom is in radians
                    //turnTime = localData->ataFrom / FIVE_G_TURN_RATE;
          turnTime = localData->ataFrom*RTD / FIVE_G_TURN_RATE;// 15.9f degrees per second

                 rmax = maxAAWpnRange;//me123 60762.11F;

                 if (localData->range > rmax)
                 {
            if ( totV <= 0.0f )
            {
                       timeToRmax = MAX_TARGET_TIME * 2.0f;
            }
            else
            {
                       timeToRmax = (localData->range - rmax) / totV;
                       tof = rmax / AVE_AIM120_VEL;
            }
                 }
                 else
                 {
                    timeToRmax = 0.0F;
                    tof = localData->range / AVE_AIM120_VEL;
                 }

                 localData->targetTime = turnTime + timeToRmax + tof;
              }
              else
              {
                 localData->targetTime = 2.0F * MAX_TARGET_TIME;
              }
           */

        obj = obj->next;
    }
}

int GuestimateCombatClass(AircraftClass *self, FalconEntity *baseObj)
{
    // Fail safe
    if ( not baseObj)
        return 8;

    // If asked to use the old code, then honor the request
    if ( not g_bUseNewCanEnage)
        return baseObj->CombatClass();

    // First I'll get the campaign object if it's for a sim since I use it at many places...
    CampBaseClass *campBaseObj;

    if (baseObj->IsSim())
        campBaseObj = ((SimBaseClass*)baseObj)->GetCampaignObject();
    else
        campBaseObj = ((CampBaseClass *)baseObj);

    // If the object is a weapon, no point
    if (baseObj->IsWeapon())
        return 8;

    // If it doesn't have a campaign object or it's identified...
    if ( not campBaseObj or campBaseObj->GetIdentified(self->GetTeam()))
    {
        // Yes, now you can get its combat class
        return baseObj->CombatClass();
    }
    else
    {
        // No :-( Then guestimate it... (from RIK's BVR code)
        if ((baseObj->GetVt() * FTPSEC_TO_KNOTS > 300.0f or baseObj->ZPos() < -10000.0f))
        {
            //this might be a combat jet.. asume the worst
            return  4;
        }
        else if (baseObj->GetVt() * FTPSEC_TO_KNOTS > 250.0f)
        {
            // this could be a a-a capable thingy, but if it's is it's low level so it's a-a long range shoot capabilitys are not great
            return  1;
        }
        else
        {
            // this must be something unthreatening...it's below 250 knots but it's still unidentified so...
            return  0;
        }
    }
}
