/***************************************************************************\
    VehRWR.cpp

    This provides the basic functionality of a Radar Warning Receiver.
 This class is used directly by AI aircraft.  The player's RWR
 derives from this class.
\***************************************************************************/
#include "stdhdr.h"
#include "object.h"
#include "simmover.h"
#include "ClassTbl.h"
#include "simdrive.h"
#include "msginc/TrackMsg.h"
#include "airunit.h"
#include "msginc/RadioChatterMsg.h"
#include "aircrft.h"
#include "radar.h"
#include "team.h"
#include "vehrwr.h"
#include "soundfx.h"
#include "fsound.h"
/* S.G. SO ARH DON'T GIVE A LAUNCH WARNING */ #include "missile.h"
// At what threat level should we activate the automatic countermeasures
static const float COUNTERMEASURES_DROP_THRESHOLD = 0.6f;

extern int g_nChatterInterval; // FRB message interval time
extern bool g_bIFFRWR; // JB 010727
//MI
extern bool g_bRealisticAvionics;

// MLR 2003-11-21
void PlayRWRSoundFX(int SfxID, int Override, float Vol, float PScale);


VehRwrClass::VehRwrClass(int type, SimMoverClass* self) : RwrClass(type, self)
{
    int i;

    lowAltPriority = FALSE;
    dropPattern = 1;

    numContacts = 0;

    for (i = 0; i < MaxRWRTracks; i++)
    {
        detectionList[i].entity = NULL;
        detectionList[i].radarData = NULL;
        detectionList[i].bearing = 0.0f;
        detectionList[i].lastPlayed = 0;
        detectionList[i].lastHit = 0;
        detectionList[i].isLocked = 0;
        detectionList[i].isAGLocked = 0;//Cobra TJL
        detectionList[i].missileLaunch = 0;
        detectionList[i].missileActivity = 0;
        detectionList[i].previouslyLocked = 0;
        detectionList[i].newDetection = 0;
        detectionList[i].newDetectionDisplay = 0; // JB 010727
        detectionList[i].selected = 0;
        // JB 010727 RP5 RWR
        // 2001-02-15 ADDED BY S.G. SO THE NEW cantPlay field IS ZEROED AS WELL AND playIt AS WELL
        detectionList[i].cantPlay = 0;
        detectionList[i].playIt = 0;
        // END OF ADDED SECTION
        detectionList[i].lethality = 0.0f;
    }

    // First entry is the default selection
    detectionList[0].selected = 1;
}


VehRwrClass::~VehRwrClass(void)
{
    int i;

    for (i = 0; i < numContacts; i++)
    {
        if (detectionList[i].entity)
        {
            VuDeReferenceEntity(detectionList[i].entity);
            detectionList[i].entity = NULL;
        }
    }
}


void VehRwrClass::SetPower(int flag)
{
    RwrClass::SetPower(flag);

    if (platform->IsAirplane())
    {
        if (flag)
            ((AircraftClass*)platform)->PowerOn(AircraftClass::RwrPower);
        else
            ((AircraftClass*)platform)->PowerOff(AircraftClass::RwrPower);
    }

    // If we've been turned off, drop all tracks
    if ( not IsOn())
    {
        for (int i = 0; i < numContacts; i++)
        {
            DropTrack(i);
        }
    }
}


//SimObjectType* VehRwrClass::Exec (SimObjectType* targetList)
SimObjectType* VehRwrClass::Exec(SimObjectType* targetList)
{
    //Cobra note: This is where the Digi comes to do his RWR
    //If numContacts is 0, nothing happens.
    //Can't find anything yet about track_ping
    //playerRWR sends over here as well, but has more code to detect track_ping


    //Add in track_ping
    SimObjectType* curObj;
    curObj = targetList;
    SimBaseClass* curSimObj;
    DetectListElement* listElement;
    SimObjectLocalData* localData;

    while (curObj and platform not_eq SimDriver.GetPlayerAircraft() and not F4IsBadReadPtr(curObj, sizeof(SimObjectType))) // JB 010318 CTD
    {
        // if ((curObj->BaseData ()) and (curObj->BaseData()->IsSim()) and (curObj->BaseData()->GetRadarType())) // JB 010221 CTD
        //if ((curObj->BaseData()) and not F4IsBadCodePtr((FARPROC) curObj->BaseData()) and (curObj->BaseData()->IsSim()) and (curObj->BaseData()->GetRadarType())) // JB 010221 CTD
        if ((curObj->BaseData()) and not F4IsBadReadPtr(curObj->BaseData(), sizeof(FalconEntity)) and (curObj->BaseData()->IsSim()) and (curObj->BaseData()->GetRadarType())) // JB 010318 CTD
        {
            // Localize the info
            curSimObj = (SimBaseClass*)curObj->BaseData();
            localData = curObj->localData;

            // Is it time to hear this one again?

            if (SimLibElapsedTime > localData->sensorLoopCount[RWR] + 0.95F * curSimObj->RdrCycleTime() * SEC_TO_MSEC)
            {
                if (CanSeeObject(curObj) and BeingPainted(curObj) and CanDetectObject(curObj))
                {
                    if (ObjectDetected(curObj->BaseData(), Track_Ping))
                    {
                        localData->sensorLoopCount[RWR] = SimLibElapsedTime;
                        listElement = IsTracked(curObj->BaseData());
                    }

                    listElement = IsTracked(curObj->BaseData());

                    if (listElement)
                    {
                        listElement->lastHit = SimLibElapsedTime;
                    }
                }
                else
                {
                    listElement = IsTracked(curObj->BaseData());
                }
            }
        }

        curObj = curObj->next;
    }






    //end track_ping


    // Drop tracks if we're turned off
    if ( not IsOn())
    {
        for (int i = numContacts - 1; i >= 0; i--)
        {
            DropTrack(i);
        }
    }

    // Age all RWR Tracks
    for (int i = numContacts - 1; i >= 0; i--)
    {

        // Time out our guidance flags
        // JB 010727 RP5 RWR
        // 2001-02-27 MODIFIED BY S.G. INSTEAD OF USING THE detectionList[i].lastHit + 5000, I'LL USE THE detectionList[i].lastPlayed FIELD INSTEAD. SEE PlayerRWR FOR REASONING.
        //       THE 15000 IS A 'GUESTIMATE' HOOLA WILL TELL ME THE REAL VALUE LATER ON
        // if (SimLibElapsedTime > detectionList[i].lastHit + RadarClass::TrackUpdateTime * 2.5f)

        // 2002-01-27 me123 MP lock message fix - we now only send a message when the state changes
        /* if (detectionList[i].missileActivity and SimLibElapsedTime > detectionList[i].lastPlayed + 15000)
         {
         // detectionList[i].missileLaunch = 0;
         // detectionList[i].missileActivity = 0;
         }

         // Time out our lock flag
         if (SimLibElapsedTime > detectionList[i].lastHit + TRACK_CYCLE)
         {
        // detectionList[i].isLocked = 0; me123 dont' time out anymore...a trackunlock will be gerantied
         }*/

        // Time out our radiate flag
        if (detectionList[i].entity->IsSim())
        {
            // JB 010727 RP5 RWR Changed to 3 seconds S.G. WHY?
            // 2001-02-18 MODIFIED BY S.G. IN 1.08i2, THIS IS 30 SECONDS I'll change it to 6 seconds like the other if it's for the player. Also, why use floats??
            // ((SimBaseClass*)detectionList[i].entity)->RdrCycleTime() * 2.0F * SEC_TO_MSEC)
            if (SimLibElapsedTime > detectionList[i].lastHit + ((SimBaseClass*)detectionList[i].entity)->RdrCycleTime() * SEC_TO_MSEC + 1000 and 
                SimLibElapsedTime > detectionList[i].lastHit + (platform == SimDriver.GetPlayerAircraft() ? 3 * SEC_TO_MSEC : 30 * SEC_TO_MSEC))
            {
                if ( not detectionList[i].isLocked or (SimBaseClass*)detectionList[i].entity->IsDead())
                    DropTrack(i);
            }
        }
        else
        {
            if (SimLibElapsedTime > detectionList[i].lastHit + RADIATE_CYCLE)
            {
                // 2002-03-10 MODIFIED BY S.G. If IsSim returned false, don't cast it to SimBaseClass but CampBaseClass and not required anyway since IsDead is defined in a parent class
                //                             Also, I've seen campaign entries stay here forever with missileActivity set because the missile was launched when aggregated (hence made it here) but once it deaggregated, it stayed here. So to fix this, if it's a DEAGGREGATED campaign object, remove its lock
                //                             I also moved 'DropTrack' on a line by itself. It's much combersome to put breakpoint if the if body is on the same line as the if statement (also done above).
                // if ( not detectionList[i].isLocked or (SimBaseClass*)detectionList[i].entity->IsDead()) DropTrack (i);
                if ( not detectionList[i].isLocked or detectionList[i].entity->IsDead() or not ((CampBaseClass*)detectionList[i].entity)->IsAggregate())
                    DropTrack(i);
            }
        }
    }

    return NULL;
}


int VehRwrClass::ObjectDetected(FalconEntity* theObject, int trackType, int radarMode)  // 2002-02-09 MODIFIED BY S.G. Added the radarMode var
{
    int retval = FALSE;
    float lethality;
    DetectListElement *listElement;

    F4Assert(numContacts == 0 or detectionList[numContacts - 1].entity);

    //Cobra TEST Let me know if AI brain is here.
    if (platform not_eq SimDriver.GetPlayerAircraft())
        int Player = 0;

    int helper = 0;//cobra test
    FalconEntity* whoPingedMe = theObject;//cobra

    // Just return if we're turned off
    if ( not IsOn())
    {
        return retval;
    }

    // See if we've already got this threat in our list
    listElement = IsTracked(theObject);

    // Decide how lethal we think this threat is
    lethality = GetLethality(theObject);

    // JB 010727 RP5 RWR
    // 2001-02-19 ADDED BY S.G. SO TARGET WITH A LOCK HAVE AN INCREASE IN LETHALITY (ONLY FOR THE PLAYER)
    if (platform == SimDriver.GetPlayerAircraft() and trackType == Track_Lock)
        lethality += 1.0F;

    // END OF ADDED SECTION

    if ( not listElement)
    {
        // Don't add a track upon receipt of a "drop" message
        if (trackType not_eq Track_Unlock and trackType not_eq Track_LaunchEnd)
        {

            // Create a new record for this threat
            listElement = AddTrack(theObject, lethality);

            // JB 010727 RP5 RWR
            // 2001-02-19 ADDED BY S.G. SO newGuy ISN'T SET IF THERE IS NO ROOM...
            if (listElement)
                // END OF ADDED SECTION
                retval = TRUE;

            // If this is the new head of the list, it's lethal, and dropPattern, drop countermeasures
            /*if (listElement == &detectionList[0] and AutoDrop() and lethality > COUNTERMEASURES_DROP_THRESHOLD)
            {
             ((AircraftClass*)platform)->DropProgramed();
            }*/ //Cobra Removed because it gets triggered incorrectly and only gets hit once.
            //It's worthless.
        }
    }

    // JB 010718
    /*
     else
     {
     // Resort him into the list
     if (listElement->lethality not_eq lethality)
     {
     listElement->lethality = lethality;
     ResortList (listElement);
     }
     }
    */

    if (listElement)
    {
        // JB 010727 RP5 RWR
        // 2001-02-18 ADDED BY S.G. SO only Track_Ping and Track_lock triggers a lastHit update
        if (trackType == Track_Lock or trackType == Track_Ping)
        {
            // END OF ADDED SECTION
            listElement->lastHit = SimLibElapsedTime;
            listElement->bearing = (float)atan2(theObject->YPos() - platform->YPos(), theObject->XPos() - platform->XPos());

            // 2002-02-09 ADDED BY S.G. If it's a Track_Lock, set the radarMode as well
            if (trackType == Track_Lock)
                listElement->radarMode = radarMode;
        }

        // Update the state of this emitter
        static SIM_ULONG  spikeTimer = SimLibElapsedTime; //Cobra

        switch (trackType)
        {
            case Track_Launch:
                if (platform == SimDriver.GetPlayerAircraft() and listElement->missileActivity == 0)
                {
                    // 2000-09-03 S.G. SO ARH DON'T GET A LAUNCH WARNING
                    // 2000-09-11 S.G. make sure the entity is a missile before testing its sensor type...
                    if ( not listElement->entity->IsMissile() or ((MissileClass *)listElement->entity)->GetSeekerType() not_eq SensorClass::Radar)
                        // END OF ADDED SECTION (PLUS INDENTATION OF NEXT LINE)
                    {
                        //F4SoundFXSetDist( SFX_TWS_LAUNCH, FALSE, 0.0f, 1.0f );
                        PlayRWRSoundFX(SFX_TWS_LAUNCH, FALSE, 0.0f, 1.0f);
                    }
                }

                // JB 010727 RP5 RWR
                // 2001-02-27 ADDED BY S.G. WE SET lastPlayed TO NOW IF THERE IS NO MISSILE ACTIVITY ALREADY SO WE CAN TIME 15 SECONDS. SINCE missileActivity IS SET, DoAudio WILL NOT UPDATE IT
                if ( not listElement->missileActivity)
                    listElement->lastPlayed = SimLibElapsedTime;

                // END OF ADDED SECTION

                listElement->missileActivity = 1;


                if (listElement->entity->IsMissile())
                {
                    if (platform == SimDriver.GetPlayerAircraft() and listElement->missileLaunch == 0)

                        // 2000-09-03 S.G. SO ARH DON'T GET A LAUNCH WARNING
                        if (((MissileClass *)listElement->entity)->GetSeekerType() not_eq SensorClass::Radar)
                        {
                            // END OF ADDED SECTION (PLUS INDENTATION OF NEXT LINE)
                            //F4SoundFXSetDist( SFX_TWS_LOCK, FALSE, 0.0f, 1.0f );
                            PlayRWRSoundFX(SFX_TWS_LOCK, FALSE, 0.0f, 1.0f);

                            listElement->missileLaunch = 1;
                        }
                }

            case Track_Lock:

                if (radarMode == RadarClass::DigiSTT) // 2002-02-10 ADDED BY S.G. Only in STT will the target be flagged as having a lock.
                    listElement->isLocked = 1;
                else
                    listElement->isLocked = 0;

                if (whoPingedMe->OnGround() and SimLibElapsedTime > spikeTimer)//Cobra
                {
                    listElement->isAGLocked = 1;//Cobra
                    SimVehicleClass *spiked = (SimVehicleClass*)platform;
                    float dx = whoPingedMe->XPos() - spiked->XPos();
                    float dy = whoPingedMe->YPos() - spiked->YPos();
                    dx = (float)atan2(dy, dx);
                    int data2 = 0;

                    if (dx < -157.5F * DTR)
                        data2 = 5; // South
                    else if (dx < -112.5F * DTR)
                        data2 = 6; // Southwest
                    else if (dx < -67.5F * DTR)
                        data2 = 7; // West
                    else if (dx < -22.5F * DTR)
                        data2 = 0; // Northwest
                    else if (dx <  22.5F * DTR)
                        data2 = 1; // North
                    else if (dx <  67.5F * DTR)
                        data2 = 2; // Northeast
                    else if (dx <  112.5F * DTR)
                        data2 = 3; // East;
                    else if (dx <  157.5F * DTR)
                        data2 = 4; // Southeast
                    else
                        data2 = 5; // South again

                    FalconRadioChatterMessage *radioMessage = new FalconRadioChatterMessage(theObject->Id(), FalconLocalGame);
                    radioMessage->dataBlock.from = spiked->Id();
                    radioMessage->dataBlock.message = rcSPIKE;
                    radioMessage->dataBlock.to = MESSAGE_FOR_TEAM;
                    radioMessage->dataBlock.voice_id = static_cast<uchar>(((Flight)spiked->GetCampaignObject())->GetPilotVoiceID(spiked->vehicleInUnit));
                    radioMessage->dataBlock.edata[0] = static_cast<short>(spiked->GetCallsignIdx());
                    radioMessage->dataBlock.edata[1] = static_cast<short>(((Flight)spiked->GetCampaignObject())->GetPilotCallNumber(spiked->vehicleInUnit));
                    radioMessage->dataBlock.edata[2] = 1;
                    radioMessage->dataBlock.edata[3] = data2;
                    FalconSendMessage(radioMessage, FALSE);
                    spikeTimer = SimLibElapsedTime + g_nChatterInterval * CampaignSeconds; // FRB - decrease number of reports per minute
                    //spikeTimer = SimLibElapsedTime + 5000;
                }

                break;

            case Track_Unlock:
                listElement->isLocked = 0;

            case Track_LaunchEnd:
                listElement->missileActivity = 0;
                listElement->missileLaunch = 0;

                if (whoPingedMe->OnGround())
                    listElement->isAGLocked = 0;//Cobra

                break;
        }

        // If this is a new lock made by a player, see about sending a "buddy spike" call
        if (trackType == Track_Lock and listElement->previouslyLocked == 0 and radarMode == RadarClass::DigiSTT) // 2002-02-20 MODIFIED BY S.G. Added the radarMode test so only STT will generate a buddy spike.
        {
            if (theObject->IsPlayer() and 
                TeamInfo[platform->GetTeam()]->TStance(theObject->GetTeam()) < Neutral)
            {
                // We're friendly or allied, so...
                ReportBuddySpike(theObject);
            }

            listElement->previouslyLocked = TRUE;
        }
    }

    ShiAssert(numContacts == 0 or detectionList[numContacts - 1].entity);
    return retval;
}


void VehRwrClass::ReportBuddySpike(FalconEntity* theObject)
{
    SimVehicleClass *spiked = (SimVehicleClass*)platform;

    if ( not spiked->IsPlayer())
    {
        FalconRadioChatterMessage *radioMessage = new FalconRadioChatterMessage(theObject->Id(), FalconLocalGame);
        radioMessage->dataBlock.from = spiked->Id();
        radioMessage->dataBlock.message = rcBUDDYSPIKE;
        radioMessage->dataBlock.to = MESSAGE_FOR_TEAM;
        radioMessage->dataBlock.voice_id = static_cast<uchar>(((Flight)spiked->GetCampaignObject())->GetPilotVoiceID(spiked->vehicleInUnit));
        radioMessage->dataBlock.edata[0] = static_cast<short>(spiked->GetCallsignIdx());
        radioMessage->dataBlock.edata[1] = static_cast<short>(((Flight)spiked->GetCampaignObject())->GetPilotCallNumber(spiked->vehicleInUnit));
        FalconSendMessage(radioMessage, FALSE);
    }
}


float VehRwrClass::GetLethality(FalconEntity* theObject)
{
    int alt = (lowAltPriority) ? LOW_ALT_LETHALITY : HIGH_ALT_LETHALITY;
    float lethality = RadarDataTable[theObject->GetRadarType()].Lethality[alt];

    // Scale lethality for normalized range
    float dx = theObject->XPos() - platform->XPos();
    float dy = theObject->YPos() - platform->YPos();
    float nomRange = RadarDataTable[theObject->GetRadarType()].NominalRange;
    float range  = (float)sqrt(dx * dx + dy * dy) / (2.0f * nomRange);

    if (range < 0.8f)
    {
        lethality *= 1 - range;
    }
    else
    {
        lethality *= 0.2f;
    }

    // Here we drop lethality for non-hositile emitters (good for Digis and Easy mode players)
    switch (TeamInfo[platform->GetTeam()]->TStance(theObject->GetTeam()))
    {
        case Hostile:
        case War:
        case Neutral:
            break;

        case Allied:
        case Friendly:
            lethality = 0.0f;
            break;
    }

    if (GetRoE(theObject->GetTeam(), platform->GetTeam(), ROE_AIR_ENGAGE)) // JB 010730
        lethality += .5;
    else if (g_bIFFRWR) // JB 010727
        lethality = 0.0;

    return lethality;
}


FalconEntity* VehRwrClass::CurSpike(FalconEntity *byHim, int *data)  // 2002-02-10 MODIFIED BY S.G. Added 'byHim' and 'data' which defaults to NULL if not passed
{
    int curIndex = -1; // 2002-02-11 S.G.
    FalconEntity* curSpike;

    int i = 0;

    for (i = 0; i < numContacts; i++)
    {
        if ( not data)   // 2002-02-11 ADDED BY S.G. If no data passed (ie data is NULL), act like before
        {
            if (detectionList[i].isLocked and detectionList[i].radarMode == RadarClass::DigiSTT) // 2002-02-09 MODIFIED BY S.G. Added the radarMode check
            {
                if ( not byHim or detectionList[i].entity == byHim)   // 2002-02-10 ADDED BY S.G. If we are looking at a specific target, limit your search to it
                {
                    curSpike = detectionList[i].entity;

                    // Only react to enemy activity
                    if ( not curSpike->IsMissile() and TeamInfo[platform->GetTeam()]->TStance(curSpike->GetTeam()) > Neutral)   // 2002-02-09 MODIFIED BY S.G. Added the not IsMissile since AI do not attack missiles anyway...
                    {
                        return curSpike;
                    }
                }
            }
            //Cobra
            else if (detectionList[i].isAGLocked)
            {
                curSpike = detectionList[i].entity;
                return curSpike;
            }
        }
        else   // I passed something in 'data', this means I want to filter out/priorotize targets based on their lock type. The chosen lock type will be passed back in *data
        {
            if (detectionList[i].isLocked)   // If not even locked, don't bother...
            {
                if (((*data bitand RWR_GET_STT) and detectionList[i].radarMode == RadarClass::DigiSTT) or // The lock is of the right type
                    ((*data bitand RWR_GET_SAM) and detectionList[i].radarMode == RadarClass::DigiSAM) or
                    ((*data bitand RWR_GET_TWS) and detectionList[i].radarMode == RadarClass::DigiTWS) or
                    ((*data bitand RWR_GET_RWS) and detectionList[i].radarMode == RadarClass::DigiRWS))
                {
                    if ( not byHim or detectionList[i].entity == byHim)   // 2002-02-10 ADDED BY S.G. If we are looking at a specific target, limit your search to it
                    {
                        curSpike = detectionList[i].entity;

                        // Only react to enemy activity
                        if ( not curSpike->IsMissile() and TeamInfo[platform->GetTeam()]->TStance(curSpike->GetTeam()) > Neutral)
                        {
                            // If we passed a prioritisation mask and not of the right priority, just keep it for later in case we have none of that type
                            if (*data bitand RWR_PRIORITIZE_MASK)
                            {
                                if (((*data bitand RWR_PRIORITIZE_STT) and detectionList[i].radarMode == RadarClass::DigiSTT) or // The lock is of the right priority, return it
                                    ((*data bitand RWR_PRIORITIZE_SAM) and detectionList[i].radarMode == RadarClass::DigiSAM) or
                                    ((*data bitand RWR_PRIORITIZE_TWS) and detectionList[i].radarMode == RadarClass::DigiTWS) or
                                    ((*data bitand RWR_PRIORITIZE_RWS) and detectionList[i].radarMode == RadarClass::DigiRWS))
                                {
                                    *data = detectionList[i].radarMode; // Pass back the radar mode to the calling function
                                    return curSpike;
                                }
                                else if (curIndex == -1) // Keep only the first one (highest one)
                                    curIndex = i; // Although not a priority, it is still a radar mode we are looking for
                            }
                            else
                            {
                                *data = detectionList[i].radarMode; // Pass back the radar mode to the calling function
                                return curSpike;
                            }

                        }
                    }
                }
            }
        }
    }

    // 2002-02-11 ADDED BY S.G. If we reached the end and couldn't find a prioritized one, return at least what we found
    if (curIndex not_eq -1)
    {
        *data = detectionList[i].radarMode; // Pass back the radar mode to the calling function
        return detectionList[i].entity;
    }

    return NULL;
}


FalconEntity* SpikeCheck(AircraftClass* self, FalconEntity *byHim, int *data)  // 2002-02-10 MODIFIED BY S.G. Added 'byHim' and 'data' which defaults to NULL if not passed
{
    VehRwrClass* theRwr = (VehRwrClass*)FindSensor(self, SensorClass::RWR);

    if (theRwr)
    {
        return theRwr->CurSpike(byHim, data);
    }
    else
    {
        return NULL;
    }
}
