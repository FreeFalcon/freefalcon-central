/***************************************************************************\
    PlayerRWR.cpp

    This provides the special abilities required for a player's RWR.  Most
 of the added functionality involves Display and Mode control.
\***************************************************************************/
#include "stdhdr.h"
#include "ClassTbl.h"
#include "fsound.h"
#include "simmover.h"
#include "object.h"
#include "msginc/TrackMsg.h"
#include "Graphics/Include/Display.h"
#include "simdrive.h"
#include "campbase.h"
#include "team.h"
#include "CampList.h"
#include "otwdrive.h"
#include "PlayerRwr.h"
#include "soundfx.h"
#include "radarData.h"
#include "Battalion.h"
/* S.G. SO ARH DON'T GIVE A LAUNCH WARNING */ #include "missile.h"
/* S.G. SO THE PLAYER IS SPOTTED BY GROUND RADAR */ #include "atm.h"
// Controls flashing display elements
static int flash = FALSE;

//MI
#include "flightData.h"
#include "icp.h"
#include "cpmanager.h"
#include "soundfx.h"
#include "aircrft.h"
#include "fack.h"
extern bool g_bRealisticAvionics;
//MI

extern bool g_bIFFRWR; // JB 010727
extern bool g_bRWR; // JB 010802

#include "SimIO.h" // Retro 3Jan2004

// MLR 2003-11-21 Moved volume control for RWR sounds here.
void PlayRWRSoundFX(int SfxID, int Override, float Vol, float PScale)
{
    if (g_bRealisticAvionics  && SimDriver.GetPlayerAircraft())
    {
        if (IO.AnalogIsUsed(AXIS_THREAT_VOLUME) == false) // Retro 3Jan2004
        {
            //make sure we don't hear it
            if (SimDriver.GetPlayerAircraft()->ThreatVolume == 8)
            {
                // can't hear it, don't play it.
                return;
            }
            else
                Vol = -(float)(SimDriver.GetPlayerAircraft()->ThreatVolume * 250);
        }
        else // Retro 3Jan2004
        {
            // Retro 26Jan2004 - the axis is now reversed on default and scales linear to the axis
            // - the user will have to shape it to logarithmic to use the throw efficiently
            Vol = -(float)(/*15000-*/IO.GetAxisValue(AXIS_THREAT_VOLUME)) / 1.5F; // Retro 26Jan2004
        }
    }

    F4SoundFXSetDist(SfxID, Override, Vol, PScale);

}

PlayerRwrClass::PlayerRwrClass(int idx, SimMoverClass* self) : VehRwrClass(idx, self)
{
    flipFactor      = 1;
    mGridVisible    = TRUE;

    priorityMode    = TRUE;
    targetSep       = FALSE;
    showUnknowns    = FALSE;
    showNaval       = FALSE;
    showSearch      = FALSE;
    dropPattern     = FALSE;
    missileActivity = FALSE;
    newGuy          = -1;
    //MI EWS stuff
    InEWSLoop = FALSE;
    LaunchDetected = FALSE;
    ChaffCheck = FALSE;
    FlareCheck = FALSE;
    SaidJammer = FALSE;
    ReleaseManual = FALSE;
}

PlayerRwrClass::~PlayerRwrClass(void)
{
}


SimObjectType* PlayerRwrClass::Exec(SimObjectType* targetList)
{
    SimObjectType* curObj;
    SimBaseClass* curSimObj;
    SimObjectLocalData* localData;
    VuListIterator emitters(EmitterList);
    CampBaseClass *curEmitter, *nextEmitter;
    int pingType;
    int nextPingTime;
    DetectListElement* listElement;

    // Select low or high altitude priority
    AutoSelectAltitudePriority();


    // Call our parent class's Exec to do basic house keeping
    VehRwrClass::Exec(targetList);


    // Check the target list for 'pings'
    curObj = targetList;

    //me123 this is only search spikes
    //while (curObj) // JB 010221 CTD
    //while (curObj && !F4IsBadCodePtr((FARPROC) curObj)) // JB 010221 CTD
    while (curObj && !F4IsBadReadPtr(curObj, sizeof(SimObjectType))) // JB 010318 CTD
    {
        // if ((curObj->BaseData ()) && (curObj->BaseData()->IsSim()) && (curObj->BaseData()->GetRadarType())) // JB 010221 CTD
        //if ((curObj->BaseData()) && !F4IsBadCodePtr((FARPROC) curObj->BaseData()) && (curObj->BaseData()->IsSim()) && (curObj->BaseData()->GetRadarType())) // JB 010221 CTD
        if ((curObj->BaseData()) && !F4IsBadReadPtr(curObj->BaseData(), sizeof(FalconEntity)) && (curObj->BaseData()->IsSim()) && (curObj->BaseData()->GetRadarType())) // JB 010318 CTD
        {
            // Localize the info
            curSimObj = (SimBaseClass*)curObj->BaseData();
            localData = curObj->localData;

            // Is it time to hear this one again?

            if (SimLibElapsedTime > localData->sensorLoopCount[RWR] + 0.95F * curSimObj->RdrCycleTime() * SEC_TO_MSEC)
            {
                // Can we hear it?
                if (!IsFiltered(curObj->BaseData()))
                {
                    if (/*curObj->localData->range <= 5 * NM_TO_FT && curObj->localData->ataFrom < 60.0f * DTR &&//me123 addet range check*/
                        CanSeeObject(curObj) && BeingPainted(curObj) && CanDetectObject(curObj))
                    {
                        if (ObjectDetected(curObj->BaseData(), Track_Ping) && newGuy >= 0)
                        {
                            newGuy = TRUE;
                        }

                        localData->sensorLoopCount[RWR] = SimLibElapsedTime;

                        // Find and update the last hit time for this object.
                        listElement = IsTracked(curObj->BaseData());

                        if (listElement)
                        {
                            listElement->lastHit = SimLibElapsedTime;
                            // JB 010727 RP5 RWR
                            // 2001-02-15 ADDED BY S.G. SO THE NEW cantPlay field IS ZEROED SINCE IT CAN SEE/DETECT IT
                            listElement->cantPlay = 0;
                        }
                    }
                    else
                    {
                        listElement = IsTracked(curObj->BaseData());

                        if (listElement)
                            listElement->cantPlay = 1;
                    }

                    // END OF ADDED SECTION
                    /*
                     if (listElement)
                     listElement->lastHit = SimLibElapsedTime;
                     }
                    */
                }
            }
        }

        curObj = curObj->next;
    }

    if (newGuy < 0)
        newGuy = FALSE;


    // 2001-03-19 ADDED BY S.G. SO THE PLAYER IS DETECTED BY GROUND RADAR AS WELL (INIT CODE)
    Team who;
    int i, enemy, roea[NUM_TEAMS];
    int spotted = 0;
    CampBaseClass *platformCampObj;

    platformCampObj = platform->GetCampaignObject();
    who = platformCampObj->GetTeam();
    enemy = GetEnemyTeam(who);

    if ((Camp_GetCurrentTime() - platformCampObj->GetSpotTime() <= ReconLossTime[platformCampObj->GetMovementType()] / 8 && ((platformCampObj->GetSpotted() >> enemy) & 0x01)))
        spotted = 1;
    else
    {
        // No need to initialize if spotted is at 1 because it won't use the array in that case.
        for (i = 0; i < NUM_TEAMS; i++)
            roea[i] = GetRoE(who, i, ROE_AIR_FIRE);
    }

    // END OF ADDED SECTION

    // Check the emitter list
    curEmitter = (CampBaseClass*)emitters.GetFirst();

    while (curEmitter)
    {
        // FRB - CTD's Here
        if (F4IsBadReadPtr(curEmitter, sizeof(CampBaseClass)))
            continue;

        nextEmitter = static_cast<CampBaseClass*>(emitters.GetNext());

        if (!nextEmitter)
            break;

        if (F4IsBadReadPtr(nextEmitter, sizeof(CampBaseClass)))
            break;

        // Check if aggregated unit can detect
        if (
            !IsFiltered(curEmitter) &&
            // 2001-03-06 MODIFIED BY S.G. SO OBJECTIVES
            // ARE ALWAYS CHECKED, WHETHER OR NOT THEY ARE AGGREGATED...
            //curEmitter->IsAggregate() &&                    // A campaign thing
            //(curEmitter->IsAggregate() || curEmitter->IsObjective()) &&
            // A campaign thing or an objective (sim objectives have no sensor routine
            // so they can never make it to the contact list by themself)
            (
                (curEmitter->IsObjective() && curEmitter->GetElectronicDetectionRange(Air) != 0) ||
                // An objective (sim objectives have no sensor routine
                // so they can never make it to the contact list by themself)
                // that has a working radar or a campaign thing
                curEmitter->IsAggregate()
            ) &&
            curEmitter->CanDetect(platform) &&              // That has us spotted
            curEmitter->GetRadarMode() != FEC_RADAR_OFF &&  // And is emmitting
            CanDetectObject(curEmitter)                     // And there is line of sight
        )
        {
            // What type of hit is this?
            switch (curEmitter->GetRadarMode())
            {
                case FEC_RADAR_SEARCH_1:
                    pingType = Track_Ping;
                    nextPingTime = 40 * SEC_TO_MSEC; //me123 from 120
                    break;

                case FEC_RADAR_SEARCH_2:
                    pingType = Track_Ping;
                    nextPingTime = 25 * SEC_TO_MSEC; //me123 from 60
                    break;

                case FEC_RADAR_SEARCH_3:
                    pingType = Track_Ping;
                    nextPingTime = 10 * SEC_TO_MSEC; //me123 from 120
                    break;

                case FEC_RADAR_SEARCH_100:
                    pingType = Track_Ping;
                    nextPingTime = 3 * SEC_TO_MSEC; //me123 from 1
                    break;

                case FEC_RADAR_AQUIRE:
                    pingType = Track_Ping;
                    nextPingTime = 2 * SEC_TO_MSEC; //me123 from 1
                    break;

                case FEC_RADAR_GUIDE:
                    pingType = Track_Ping;
                    nextPingTime = (int)(0.5f * SEC_TO_MSEC);
                    break;

                default:
                    pingType = Track_Ping;
                    nextPingTime = 6000 * SEC_TO_MSEC; //me123 from 120
                    break;
            }

            // 2002-03-21 REMOVED BY S.G. In accordance with RIK, this code doesn't belong here
            //if(!curEmitter->IsSim() &&
            // curEmitter->GetRadarMode()> FEC_RADAR_SEARCH_1 &&
            // curEmitter->GetRadarMode() != FEC_RADAR_SEARCH_100)
            // ((BattalionClass*)(SimBaseClass*)curEmitter)->SetRadarMode(FEC_RADAR_SEARCH_1);
            // Add it to the list (if the list isn't full)
            //if (ObjectDetected(curEmitter, pingType))
            //{
            // newGuy = TRUE;
            //}
        }

        // 2001-03-19 ADDED BY S.G. SO THE PLAYER IS DETECTED BY GROUND RADAR AS WELL...
        // Don't even start testing if we have been spotted in the last ReconLossTime[GetMovementType()]/8 and we're not on a SEAD missiom
        if (!spotted)
        {
            if (roea[curEmitter->GetTeam()] == ROE_ALLOWED)
            {
                // This Code is OUR "Check to see if we've been detected by OBJECTIVE ground radar"
                // No need to test for unit ground radar since they'll do it themselve...
                // plus we let CanDetect handle stealth flights
                // Stop testing once we got spotted
                if (curEmitter->IsObjective() && curEmitter->CanDetect(platformCampObj) && ((ObjectiveClass *)curEmitter)->IsGCI())
                {
                    if (!platformCampObj->GetSpotted(enemy))
                        RequestIntercept((Flight)platform->GetCampaignObject(), enemy);

                    platformCampObj->SetSpotted(enemy, TheCampaign.CurrentTime, ((ObjectiveClass *)curEmitter)->HasNCTR() != 0); // 2002-02-11 MODIFIED BY S.G. Ground objective radar can identify me
                    spotted = 1;
                }
            }
        }

        // END OF ADDED SECTION

        curEmitter = nextEmitter;//(CampBaseClass*)emitters.GetNext();
    }


    // Now make a pass through the detected list to decide who's important and play audio
    missileActivity = FALSE;

    if (OTWDriver.DisplayInCockpit())
    {
        for (i = 0; i < numContacts; i++)
        {
            if (detectionList[i].missileActivity)
            {
                // 2000-09-03 S.G. SO ARH DON'T GET A LAUNCH WARNING
                // JB 010118 added check of detectionList[i].entity
                if (detectionList[i].entity && (!((MissileClass *)detectionList[i].entity)->IsMissile() || ((MissileClass *)detectionList[i].entity)->GetSeekerType() != SensorClass::Radar))
                {
                    // END OF ADDED SECTION (PLUS INDENTATION OF NEXT LINE)
                    missileActivity = TRUE;
                }
            }

            listElement = &detectionList[i];
            listElement->lethality = GetLethality(listElement->entity);

            // Special handling to implement "Target Separation"
            if (targetSep && i != 0)
            {
                listElement->lethality *= 0.5f;
            }

            // JB 010727 RP5 RWR
            // 2001-02-19 ADDED BY S.G. SO TARGET WITH A LOCK HAVE AN INCREASE IN LETHALITY (THIS ROUTINE IS FOR THE PLAYER ONLY ANYWAY)
            if (listElement->isLocked)
                listElement->lethality += 1.0F;

            // END OF ADDED SECTION

            //ResortList( listElement ); // JB 010718
            DoAudio(listElement);
        }

        SortDetectionList(); // JB 010718
        //DoAudio(); // JB 010718 Alternate DoAudio location
    }

    //MI extracting RWR
    int count;

    for (i = 0, count = 0; i < numContacts && count < 40; i++)
    {
        // let see what should be drawn
        if (!IsFiltered(detectionList[i].entity))
        {
            cockpitFlightData.RWRsymbol[count] = detectionList[i].radarData->RWRsymbol; // Which symbol shows up on the RWR
            cockpitFlightData.bearing[count] = detectionList[i].bearing;
            cockpitFlightData.missileActivity[count] = detectionList[i].missileActivity;  // Is launching on us (active OR beam rider)
            cockpitFlightData.missileLaunch[count] = detectionList[i].missileLaunch;  // Is launching on us (active OR beam rider)
            cockpitFlightData.selected[count] = detectionList[i].selected;  // Is launching on us (active OR beam rider)
            cockpitFlightData.lethality[count] = detectionList[i].lethality;  // Is launching on us (active OR beam rider)
            count++;
        }
    }

    cockpitFlightData.RwrObjectCount = count;

    // Check timeout on target sep
    if (targetSep > 0)
    {
        targetSep -= FloatToInt32(SimLibMajorFrameTime * SEC_TO_MSEC);

        if (targetSep < 0)
            targetSep = 0;
    }

    //MI EWS check, only do this if we have auto chaff/flare
    if (g_bRealisticAvionics)
    {
        if (ReleaseManual)
        {
            if (SimDriver.GetPlayerAircraft())
                SimDriver.GetPlayerAircraft()->ReleaseManualProgram();
        }
        else if (SimDriver.GetPlayerAircraft() && SimDriver.GetPlayerAircraft()->HasPower(AircraftClass::EWSRWRPower))
        {
            if (SimDriver.GetPlayerAircraft()->EWSPGM() != AircraftClass::Off &&
                SimDriver.GetPlayerAircraft()->EWSPGM() != AircraftClass::Stby &&
                SimDriver.GetPlayerAircraft()->EWSPGM() != AircraftClass::Man)
                CheckEWS();
        }
    }

    return NULL;
}


void PlayerRwrClass::AutoSelectAltitudePriority(void)
{
    ShiAssert(platform);

    // Remember:  Z is negative up.
    float agl = OTWDriver.GetGroundLevel(platform->XPos(), platform->YPos()) - platform->ZPos();

    if (agl > 6000.0f)
    {
        lowAltPriority = FALSE;
    }
    else if (agl < 5000.0f)
    {
        lowAltPriority = TRUE;
    }
}


void PlayerRwrClass::Display(VirtualDisplay *activeDisplay)
{
    int i, last, drawn;

    if (!g_bRWR) // JB 010802
        return;

    // Make the sensor/MFD system happy
    display = activeDisplay;

    // Do we need to draw our own reference grid?
    if (mGridVisible)
    {
        DrawGrid();
    }

    // Do we draw flashing things this frame?
    flash = vuxRealTime & 0x200;

    // See if we need to switch between rightsideup and upsidedown presentations
    if (flipFactor > 0)
    {
        // We're currently in normal mode
        if (fabs(platform->Roll()) > 120.0f * DTR)
        {
            // switch to upsidedown mode
            flipFactor = -1;
        }
    }
    else
    {
        // We're currently in upsidedown mode
        if (fabs(platform->Roll()) < 90.0f * DTR)
        {
            // switch to normal mode
            flipFactor = 1;
        }
    }

    // Decide how many contacts to present
    if (priorityMode == TRUE)
    {
        last = min(PriorityContacts, numContacts);
    }
    else
    {
        last = numContacts;
    }

    // Draw them
    for (i = 0, drawn = 0; i < numContacts && drawn < last; i++)
    {
        if (IsFiltered(detectionList[i].entity))
        {
            continue;
        }

        int mode = 0;
        DetectListElement* Element;
        Element = &detectionList[i];

        if (((SimBaseClass*)detectionList[i].entity)->IsSim() && ((BattalionClass*)((SimBaseClass*)detectionList[i].entity)->GetCampaignObject()))
            mode = ((BattalionClass*)((SimBaseClass*)detectionList[i].entity)->GetCampaignObject())->GetRadarMode();
        else mode = ((SimBaseClass*)detectionList[i].entity)->GetRadarMode();

        if (mode != FEC_RADAR_OFF || Element->isLocked)
        {
            drawn++;
            DrawContact(&detectionList[i]);
        }
    }

    display->CenterOriginInViewport();
}


void PlayerRwrClass::DrawContact(DetectListElement *record)
{
    if (!g_bRWR) // JB 010802
        return;

    RadarDataSet* radarfileData = &radarDatFileTable[record->radarData->RDRDataInd];
    float xPos, yPos, angle, radius;
    mlTrig trig;

    ShiAssert(record->entity);

    if (record->entity && !record->cantPlay)
    {
        angle = record->bearing - platform->Yaw();
        mlSinCos(&trig, angle);

        // JB 010727 RP5 RWR
        // 2001-02-20 MOFIFIED BY S.G. SO WE CONVERT LOCKED TARGET LETHALITY TO SOME VALID NUMBER
        // radius = (1.0F - record->lethality) * 0.95f; // 0.95 keeps things from leaking off the edge of the display
        if (record->lethality > 1.0)
            radius = (2.0F - record->lethality) * 0.95f; // 0.95 keeps things from leaking off the edge of the display
        else
            radius = (1.0F - record->lethality) * 0.95f; // 0.95 keeps things from leaking off the edge of the display

        // END OF MODIFIED SECTION

        xPos  = radius * trig.sin;
        yPos  = radius * trig.cos;

        // Flip this display is we are upside down
        xPos *= flipFactor;

        // Establish the 2D drawing transform
        display->CenterOriginInViewport();
        display->AdjustOriginInViewport(xPos, yPos);

        // Draw the emitter symbol
        // JB 010727 Draw 'F' for friendly aircraft
        if (g_bIFFRWR && (record->radarData->RWRsymbol == RWRSYM_ADVANCED_INTERCEPTOR ||
                          record->radarData->RWRsymbol == RWRSYM_BASIC_INTERCEPTOR) &&
            !GetRoE(record->entity->GetTeam(), platform->GetTeam(), ROE_AIR_ENGAGE))
        {
            DrawEmitterSymbol(RWRSYM_F);
        }

        // 2002-03-01 Modified by MN - if symbol is "S" = Searchradar, and we don't want them, don't draw them ;-)
        // 2002-03-03 don't display anything if "symbol" stays -1 (also no new detection arcs or such...)
        int mode, symbol = -1;

        if (((SimBaseClass*)record->entity)->IsSim() && ((BattalionClass*)((SimBaseClass*)record->entity)->GetCampaignObject()))
            mode = ((BattalionClass*)((SimBaseClass*)record->entity)->GetCampaignObject())->GetRadarMode();
        else
            mode = ((SimBaseClass*)record->entity)->GetRadarMode();

        if (mode == FEC_RADAR_SEARCH_1 && radarfileData->Rwrsymbolsearch1)
        {
            if ((radarfileData->Rwrsymbolsearch1 != RWRSYM_SEARCH) ||
                radarfileData->Rwrsymbolsearch1 == RWRSYM_SEARCH && ShowSearch())
                symbol = radarfileData->Rwrsymbolsearch1;
        }
        else if (mode == FEC_RADAR_SEARCH_2 && radarfileData->Rwrsymbolsearch2)
        {
            if ((radarfileData->Rwrsymbolsearch2 != RWRSYM_SEARCH) ||
                radarfileData->Rwrsymbolsearch2 == RWRSYM_SEARCH && ShowSearch())
                symbol = radarfileData->Rwrsymbolsearch2;
        }
        else if (mode == FEC_RADAR_SEARCH_3 && radarfileData->Rwrsymbolsearch3)
        {
            if ((radarfileData->Rwrsymbolsearch3 != RWRSYM_SEARCH) ||
                radarfileData->Rwrsymbolsearch3 == RWRSYM_SEARCH && ShowSearch())
                symbol = radarfileData->Rwrsymbolsearch3;
        }
        else if (mode == FEC_RADAR_AQUIRE && radarfileData->Rwrsymbolacuire)
            symbol = radarfileData->Rwrsymbolacuire;
        else if (mode == FEC_RADAR_GUIDE && radarfileData->Rwrsymbolguide)
            symbol = radarfileData->Rwrsymbolguide;
        else symbol = record->radarData->RWRsymbol;

        if (symbol == -1)
            return;

        DrawEmitterSymbol(symbol);

        // Mark targets with locks on us
        if (record->missileActivity)
        {
            if (record->missileLaunch)
                DrawStatusSymbol(MissileLaunch);
            else
                DrawStatusSymbol(MissileActivity);
        }

        // Mark the "designated" emitter (the one for which we're playing sound)
        if (record->selected)
            DrawStatusSymbol(Diamond);

        // JB 010727
        if (record->newDetectionDisplay)
            DrawStatusSymbol(NewDetection);

        // RV - ERD - CTD FIX - it seems a missing casting to base object
        //if((record->entity->IsAirplane())&& (symbol >= 65)) // Cobra - Put hat on a/c special symbols
        if ((((SimBaseClass*)record->entity)->IsAirplane()) && (symbol >= 65)) // Cobra - Put hat on a/c special symbols
            DrawStatusSymbol(Hat);
    }
}


void PlayerRwrClass::DrawStatusSymbol(int symbol)
{
    static const float MARK_SIZE = 0.15f;
    //MI
    static const float  HAT_SIZE = 0.06f;

    switch (symbol)
    {
        case Diamond: // For designation
            display->Line(0.0F, MARK_SIZE, MARK_SIZE, 0.0F);
            display->Line(0.0F, MARK_SIZE, -MARK_SIZE, 0.0F);
            display->Line(0.0F, -MARK_SIZE, MARK_SIZE, 0.0F);
            display->Line(0.0F, -MARK_SIZE, -MARK_SIZE, 0.0F);
            break;

            // JB 010727
        case NewDetection:
            display->Arc(0.0, 0.0, MARK_SIZE, float(180.0 * DTR), 0);
            break;

        case MissileActivity: // LAUNCH && !entity->IsMissile()
        {
            if (flash)  break;
        }

        case MissileLaunch: // LAUNCH && entity->IsMissile()
        {
            display->Circle(0.0F, 0.0F, MARK_SIZE);
        }
        break;

        case Hat:
            display->Line(0.0F, HAT_SIZE + 0.04F, HAT_SIZE, 0.04F);
            display->Line(0.0F, HAT_SIZE + 0.04F, -HAT_SIZE, 0.04F);
            break;
    }
}

// JB 010718
void PlayerRwrClass::DoAudio(void)   // 2002-02-20 S.G. Just flagging it as not currently being used so there are no confusion
{
    if (!g_bRWR) // JB 010802
        return;

    // TODO:  This should really be radar cycle time.  Can I get that reliably here?
    //static const int SEARCH_PERIOD = 12 * 1000;
    int i = 0;

    for (i = 0; i < numContacts; i++)
    {
        int SEARCH_PERIOD = (int)(((SimBaseClass*)detectionList[i].entity)->RdrCycleTime() * SEC_TO_MSEC);

        if (SEARCH_PERIOD == 0)
            SEARCH_PERIOD = 6 * SEC_TO_MSEC;



        if (detectionList[i].playIt || (!detectionList[i].cantPlay && (!detectionList[i].entity->IsSim() || !detectionList[i].entity->IsDead())))
        {
            if ((detectionList[i].missileActivity) ||
                (detectionList[i].playIt) ||
                (detectionList[i].newDetection) ||
                (detectionList[i].selected &&
                 ((detectionList[i].isLocked) ||
                  ((SimLibElapsedTime - detectionList[i].lastPlayed) > (unsigned int) SEARCH_PERIOD))))
            {
                ShiAssert(detectionList[i].entity);
                ShiAssert(detectionList[i].radarData->RWRsound > 0);
                //F4SoundFXSetDist( detectionList[i].radarData->RWRsound, FALSE, 0.0f, 1.0f );
                PlayRWRSoundFX(detectionList[i].radarData->RWRsound, FALSE, 0.0f, 1.0f);
                detectionList[i].newDetection = FALSE;
                detectionList[i].playIt = FALSE; // JB 010728

                if (!detectionList[i].missileActivity)
                    detectionList[i].lastPlayed = SimLibElapsedTime;
            }
        }
    }

    if (newGuy)
    {
        //F4SoundFXSetDist( SFX_TWS_SEARCH, FALSE, 0.0f, 1.0f );
        // PlayRWRSoundFX( SFX_TWS_SEARCH, FALSE, 0.0f, 1.0f );
        PlayRWRSoundFX(detectionList[i].radarData->RWRsound, FALSE, 0.0f, 1.0f);
        // ^ JPG 9 Dec 03 - Get rid of that annoying, unrealistic New Guy sound fx and play new guy sound FROM the actual emitter - TODO: play 3 bursts in 1.5 secs like RL
        newGuy = FALSE;
    }
}

void PlayerRwrClass::DoAudio(DetectListElement *record)
{
    if (!g_bRWR) // JB 010802
        return;

    RadarDataSet* radarfileData = &radarDatFileTable[record->radarData->RDRDataInd];

    // TODO:  This should really be radar cycle time.  Can I get that reliably here?
    float SEARCH_PERIOD = 0.0f;

    if (((SimBaseClass*)record->entity)->IsSim())//this is a vhehicle not a battalion
    {
        SEARCH_PERIOD = ((SimBaseClass*)record->entity)->RdrCycleTime() * SEC_TO_MSEC;//me123
    }

    if (!SEARCH_PERIOD) SEARCH_PERIOD = 6 * SEC_TO_MSEC;

    int mode;

    if (((SimBaseClass*)record->entity)->IsSim() && ((BattalionClass*)((SimBaseClass*)record->entity)->GetCampaignObject()))
        mode = ((BattalionClass*)((SimBaseClass*)record->entity)->GetCampaignObject())->GetRadarMode();
    else mode = ((SimBaseClass*)record->entity)->GetRadarMode();

    switch (mode)
    {
        case FEC_RADAR_SEARCH_1:
            SEARCH_PERIOD = (float)radarfileData->Sweeptimesearch1; //me123 from 120
            break;

        case FEC_RADAR_SEARCH_2:
            SEARCH_PERIOD = (float)radarfileData->Sweeptimesearch2; //me123 from 60
            break;

        case FEC_RADAR_SEARCH_3:
            SEARCH_PERIOD = (float)radarfileData->Sweeptimesearch3; //me123 from 120
            break;

        case FEC_RADAR_SEARCH_100:
            if (record->isLocked)
                SEARCH_PERIOD = 0;
            else
                SEARCH_PERIOD = ((SimBaseClass*)record->entity)->RdrCycleTime() * SEC_TO_MSEC;

            break;

        case FEC_RADAR_AQUIRE:
            SEARCH_PERIOD = max(((SimBaseClass*)record->entity)->RdrCycleTime() * SEC_TO_MSEC, radarfileData->Sweeptimeacuire); //me123 from 1
            break;

        case FEC_RADAR_GUIDE:

            SEARCH_PERIOD = ((SimBaseClass*)record->entity)->RdrCycleTime() * SEC_TO_MSEC;
            break;


        default:
            //this is A ACTIVE MISSILE BEING SUPPORTED BY THE LAUNCHER
            break;
    }

    // JB 010727 RP5 RWR
    // 2001-02-15 ADDED BY S.G. SO Dead stuff don't ping us... If where not a sim object or if a sim, if it's not dead, go on...
    if (record->playIt || (!record->cantPlay && (!record->entity->IsSim() || !record->entity->IsDead())))
    {
        // Play all launches and the selected emitter
        if (//(record->missileActivity) ||
            //(record->playIt) || // JB 010727 RP5 RWR 2001-02-17 MODIFIED BY S.G. SO pressing HANDOFF plays the sound
            (record->newDetection) ||
            (record->selected &&
             //((record->isLocked)) ||
             ((float)SimLibElapsedTime - (float)record->lastPlayed > SEARCH_PERIOD)))
        {
            /*int testa = SFX_A50_Radar;//73
            int testb = SFX_BAR_LOCK;//152
            int testc =SFX_A50_AQUIRE_Radar;//196
            int testd =SFX_BAR_LOCK_AQUIRE;//227
            int teste =SFX_A50_search_Radar;//242
            int testf =SFX_BAR_LOCK_search;//273*/
            int sound = 0;

            if (mode == FEC_RADAR_AQUIRE)
            {
                if (radarfileData->Rwrsoundacuire) sound = radarfileData->Rwrsoundacuire;
                else sound = record->radarData->RWRsound;

                //F4SoundFXSetDist( record->radarData->RWRsound, FALSE, 0.0f, 1.0f );// PLAY THE TRACKER
                PlayRWRSoundFX(record->radarData->RWRsound, FALSE, 0.0f, 1.0f);  // PLAY THE TRACKER
            }
            else if (mode == FEC_RADAR_SEARCH_1 || mode == FEC_RADAR_SEARCH_2 || mode == FEC_RADAR_SEARCH_3)
            {
                // 2002-03-01 Modified by MN - if symbol is "S" = Searchradar, and we don't show them, don't play them either
                if (mode == FEC_RADAR_SEARCH_1)
                {
                    if (radarfileData->Rwrsymbolsearch1 == RWRSYM_SEARCH && !ShowSearch())
                        return;

                    if (radarfileData->Rwrsoundsearch1)
                        sound = radarfileData->Rwrsoundsearch1;
                }

                if (mode == FEC_RADAR_SEARCH_2)
                {
                    if (radarfileData->Rwrsymbolsearch2 == RWRSYM_SEARCH && !ShowSearch())
                        return;

                    if (radarfileData->Rwrsoundsearch2)
                        sound = radarfileData->Rwrsoundsearch2;
                }

                if (mode == FEC_RADAR_SEARCH_3)
                {
                    if (radarfileData->Rwrsymbolsearch1 == RWRSYM_SEARCH && !ShowSearch())
                        return;

                    if (radarfileData->Rwrsoundsearch3)
                        sound = radarfileData->Rwrsoundsearch3;
                }

                // no data file...
                assert(sound);

                if (!sound) sound = record->radarData->RWRsound;
            }
            else if (mode == FEC_RADAR_GUIDE)
                if (radarfileData->Rwrsoundguide) sound = radarfileData->Rwrsoundguide;

            if (mode != FEC_RADAR_OFF || record->isLocked)
            {
                if (!sound)  sound = record->radarData->RWRsound;

                ShiAssert(record->entity);
                ShiAssert(record->radarData->RWRsound > 0);

                if (sound)
                    //F4SoundFXSetDist( sound, FALSE, 0.0f, 1.0f );
                    PlayRWRSoundFX(sound, FALSE, 0.0f, 1.0f);

                record->newDetection = FALSE;
                record->playIt = FALSE; // JB 010728

                // JB 010727 RP5 RWR
                // 2001-02-27 ADDED BY S.G. record->lastPlayed AS TWO FUNCTION NOW. ONE FOR THE PING LIKE BEFORE AND A TIMER FOR THE LAUNCH LIGHT WHEN 'record->missileActivity' IS SET
                if (!record->missileActivity)
                    // END OF ADDED SECTION
                    record->lastPlayed = SimLibElapsedTime;
            }
        }
    }

    if (newGuy)
    {
        //F4SoundFXSetDist( SFX_TWS_SEARCH, FALSE, 0.0f, 1.0f );
        //PlayRWRSoundFX( SFX_TWS_SEARCH, FALSE, 0.0f, 1.0f );
        PlayRWRSoundFX(record->radarData->RWRsound, FALSE, 0.0f, 1.0f);
        // ^ JPG 9 Dec 03 - Get rid of that annoying, unrealistic New Guy sound fx and play new guy sound FROM the actual emitter - TODO: play 3 bursts in 1.5 secs like RL
        newGuy = FALSE;
    }
}

void PlayerRwrClass::DrawGrid(void)
{
    static int fpass = TRUE;
    static float tick[40][2];
    int i;

    // The first time through, we compute some stuff
    // (It'd be more efficient to precompute a static array, but its easier to write this way)
    // An intermediate solution would be a simple class with one global instance and a constructor.
    if (fpass)
    {
        float ang;
        mlTrig trig;

        fpass = FALSE;

        for (i = 0; i < 5; i++)
        {
            ang = (i + 1) * 15.0F * DTR;
            mlSinCos(&trig, ang);
            tick[i * 2][0] = 0.95F * trig.cos;
            tick[i * 2][1] = 0.95F * trig.sin;

            if (i != 2)
            {
                tick[i * 2 + 1][0] = 0.80F * trig.cos;
                tick[i * 2 + 1][1] = 0.80F * trig.sin;
            }
            else
            {
                tick[i * 2 + 1][0] = 0.70F * trig.cos;
                tick[i * 2 + 1][1] = 0.70F * trig.sin;
            }

            // Get the other quadrants
            tick [10 + i * 2][0] =  tick[i * 2][0];
            tick [10 + i * 2][1] = -tick[i * 2][1];
            tick [11 + i * 2][0] =  tick[i * 2 + 1][0];
            tick [11 + i * 2][1] = -tick[i * 2 + 1][1];

            tick [20 + i * 2][0] = -tick[i * 2][0];
            tick [20 + i * 2][1] = -tick[i * 2][1];
            tick [21 + i * 2][0] = -tick[i * 2 + 1][0];
            tick [21 + i * 2][1] = -tick[i * 2 + 1][1];

            tick [30 + i * 2][0] = -tick[i * 2][0];
            tick [30 + i * 2][1] =  tick[i * 2][1];
            tick [31 + i * 2][0] = -tick[i * 2 + 1][0];
            tick [31 + i * 2][1] =  tick[i * 2 + 1][1];
        }
    }

    // Now draw the three concentric circles
    display->Circle(0.0F, 0.0F, 0.95F);
    display->Circle(0.0F, 0.0F, 0.55F);
    display->Circle(0.0F, 0.0F, 0.25F);

    // Draw the radial lines
    display->Line(0.0F, 0.95F, 0.0F, 0.2F);
    display->Line(0.0F, -0.95F, 0.0F, -0.2F);
    display->Line(0.95F, 0.0F, 0.2F, 0.0F);
    display->Line(-0.95F, 0.0F, -0.2F, 0.0F);

    // Draw the tick marks
    for (i = 0; i < 20; i++)
    {
        display->Line(tick[i * 2][0], tick[i * 2][1], tick[i * 2 + 1][0], tick[i * 2 + 1][1]);
    }
}


int PlayerRwrClass::LightSearch(void)
{
    // Take the easy way if the search button is pushed
    if (ShowSearch())
    {
        return TRUE;
    }

    // Take the easy way if it isn't time to flash
    if (flash)
    {
        return FALSE;
    }

    // Okay, now we gotta look
    for (int i = 0; i < numContacts; i++)
    {
        if (detectionList[i].radarData->RWRsymbol == RWRSYM_SEARCH)
        {
            // We found one, so light the light
            return TRUE;
        }
    }

    // If we got here, we didn't find any search radars in our hit list
    return FALSE;
}

void PlayerRwrClass::ToggleTargetSep(void)
{
    if (targetSep < FloatToInt32(5.0F * SEC_TO_MSEC))
        targetSep = FloatToInt32(5.0F * SEC_TO_MSEC);
}

int PlayerRwrClass::ManualSelect(void)
{
#if 1
    return !detectionList[0].selected;
#else

    // Lets look through the list
    // TODO:  Set a flag for this and the other lights during EXEC
    for (int i = 0; i < numContacts; i++)
    {
        if (detectionList[i].selected)
        {
            // We found one, so light the light
            return TRUE;
        }
    }

    // If we got here, we didn't find a manually selected emitter
    return FALSE;
#endif
}


int PlayerRwrClass::LightUnknowns(void)
{
    // Take the easy way if the search button is pushed
    if (ShowUnknowns())
    {
        return TRUE;
    }

    // Take the easy way if it isn't time to flash
    if (flash)
    {
        return FALSE;
    }

    // Okay, now we gotta look
    for (int i = 0; i < numContacts; i++)
    {
        if (detectionList[i].radarData->RWRsymbol == RWRSYM_UNKNOWN)
        {
            // We found one, so light the light
            return TRUE;
        }
    }

    // If we got here, we didn't find any unknown radars in our hit list
    return FALSE;
}


void PlayerRwrClass::SelectNextEmitter(void)
{
    int foundSelected = FALSE;
    int drawn;
    int last;
    int i;


    // Decide how many contacts are being drawn
    if (priorityMode)
    {
        last = min(PriorityContacts, numContacts);
    }
    else
    {
        last = numContacts;
    }

    // Loop through the (drawn) contacts
    for (i = 0, drawn = 0; i < numContacts && drawn < last; i++)
    {
        if (IsFiltered(detectionList[i].entity))
        {
            continue;
        }

        // This one will draw
        drawn++;

        // If this is the selected target, clear its flag
        if (detectionList[i].selected)
        {
            detectionList[i].selected = 0;
            foundSelected = TRUE;
            continue;
        }

        // If we've already passed the previously selected target, then make this the new one
        if (foundSelected)
        {
            detectionList[i].selected = 1;
            // JB 010727 RP5 RWR
            // 2001-02-15 ADDED BY S.G. SO THE SOUND PLAYS RIGHT AWAY...
            detectionList[i].playIt = 1;
            // END OF ADDED SECTION
            return;
        }
    }

    // If we get here, we didn't find a "next" selected target, so select the first contact
    detectionList[0].selected = 1;

    // JB 010727 RP5 RWR
    // 2001-02-15 ADDED BY S.G. SO THE SOUND PLAYS RIGHT AWAY...
    detectionList[0].playIt = 1;
    // END OF ADDED SECTION
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int PlayerRwrClass::IsFiltered(FalconEntity *entity)
{
    RadarDataType
    *radarData = NULL;

    if (entity)
    {
        if (GetLethality(entity) <= 0.0f)
        {
            return TRUE;
        }

        radarData = &RadarDataTable[entity->GetRadarType()];

        if (radarData)
        {
            if (!ShowUnknowns() &&
                (radarData->RWRsymbol == RWRSYM_UNKNOWN ||
                 radarData->RWRsymbol == RWRSYM_UNK1 ||
                 radarData->RWRsymbol == RWRSYM_UNK2 ||
                 radarData->RWRsymbol == RWRSYM_UNK3))
            {
                return TRUE;
            }

            if (!ShowSearch() && (radarData->RWRsymbol == RWRSYM_SEARCH))
            {
                return TRUE;
            }

            if (!ShowNaval() && (radarData->RWRsymbol == RWRSYM_NAVAL))
            {
                return TRUE;
            }

            return FALSE;
        }
    }

    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void PlayerRwrClass::CheckEWS(void)
{
    if (!g_bRWR) // JB 010802
        return;

    if (F4IsBadReadPtr(SimDriver.GetPlayerAircraft(), sizeof(AircraftClass))) // JB 010408 CTD
        return;

    //Check for Power and Failure
    if (!SimDriver.GetPlayerAircraft()->HasPower(AircraftClass::UFCPower) ||
        SimDriver.GetPlayerAircraft()->mFaults->GetFault(FaultClass::ufc_fault) ||
        SimDriver.GetPlayerAircraft()->IsExploding())
        return;

    if (LaunchIndication() && !LaunchDetected)
    {
        //F4SoundFXSetDist(SFX_BB_COUNTER, TRUE, 0.0f, 1.0f );
        PlayRWRSoundFX(SFX_BB_COUNTER, TRUE, 0.0f, 1.0f);

        if (SimDriver.GetPlayerAircraft()->EWSPGM() == AircraftClass::Auto)
        {
            InEWSLoop = TRUE;
            SimDriver.GetPlayerAircraft()->DropEWS();
        }

        if (OTWDriver.pCockpitManager->mpIcp->iCHAFF_BQ[SimDriver.GetPlayerAircraft()->EWSProgNum] > 1 ||
            OTWDriver.pCockpitManager->mpIcp->iFLARE_BQ[SimDriver.GetPlayerAircraft()->EWSProgNum] > 1)
        {
            LaunchDetected = TRUE;
        }
    }

    if (!LaunchIndication())
        LaunchDetected = FALSE;

    if (InEWSLoop && !SimDriver.GetPlayerAircraft()->IsExploding())
    {
        if ((unsigned int)SimDriver.GetPlayerAircraft()->ChaffCount >= OTWDriver.pCockpitManager->mpIcp->iCHAFF_BQ[SimDriver.GetPlayerAircraft()->EWSProgNum] &&
            SimDriver.GetPlayerAircraft()->ChaffSalvoCount > 0 && !ChaffCheck)
        {
            //Set our timer
            SimDriver.GetPlayerAircraft()->ChaffBurstInterval = (VU_TIME)(SimLibElapsedTime + (OTWDriver.pCockpitManager->mpIcp->fCHAFF_SI[SimDriver.GetPlayerAircraft()->EWSProgNum] * CampaignSeconds));
            //Reset our count
            SimDriver.GetPlayerAircraft()->ChaffCount = 0;
            //Mark us with one less to go
            SimDriver.GetPlayerAircraft()->ChaffSalvoCount--;
            //We've been here already
            ChaffCheck = TRUE;
        }

        if ((unsigned int)SimDriver.GetPlayerAircraft()->FlareCount >= OTWDriver.pCockpitManager->mpIcp->iFLARE_BQ[SimDriver.GetPlayerAircraft()->EWSProgNum] &&
            SimDriver.GetPlayerAircraft()->FlareSalvoCount > 0 && !FlareCheck)
        {
            //Reset our count
            SimDriver.GetPlayerAircraft()->FlareCount = 0;
            //Set our timer
            SimDriver.GetPlayerAircraft()->FlareBurstInterval = (VU_TIME)(SimLibElapsedTime + (OTWDriver.pCockpitManager->mpIcp->fFLARE_SI[SimDriver.GetPlayerAircraft()->EWSProgNum] * CampaignSeconds));
            //Mark us with one less to go
            SimDriver.GetPlayerAircraft()->FlareSalvoCount--;
            //We've been here already
            FlareCheck = TRUE;
        }

        if (SimLibElapsedTime >= SimDriver.GetPlayerAircraft()->ChaffBurstInterval &&
            ((unsigned int)SimDriver.GetPlayerAircraft()->ChaffCount < OTWDriver.pCockpitManager->mpIcp->iCHAFF_BQ[SimDriver.GetPlayerAircraft()->EWSProgNum]))
        {
            SimDriver.GetPlayerAircraft()->EWSChaffBurst();
            ChaffCheck = FALSE;
        }

        if (SimLibElapsedTime >= SimDriver.GetPlayerAircraft()->FlareBurstInterval &&
            ((unsigned int)SimDriver.GetPlayerAircraft()->FlareCount < OTWDriver.pCockpitManager->mpIcp->iFLARE_BQ[SimDriver.GetPlayerAircraft()->EWSProgNum]))
        {
            SimDriver.GetPlayerAircraft()->EWSFlareBurst();
            FlareCheck = FALSE;
        }

        if (SimDriver.GetPlayerAircraft()->FlareCount == OTWDriver.pCockpitManager->mpIcp->iFLARE_BQ[SimDriver.GetPlayerAircraft()->EWSProgNum] &&
            SimDriver.GetPlayerAircraft()->ChaffCount == OTWDriver.pCockpitManager->mpIcp->iCHAFF_BQ[SimDriver.GetPlayerAircraft()->EWSProgNum] &&
            SimDriver.GetPlayerAircraft()->ChaffSalvoCount <= 0 && SimDriver.GetPlayerAircraft()->FlareSalvoCount <= 0)
        {
            InEWSLoop = FALSE;
        }
    }

    if (OTWDriver.pCockpitManager->mpIcp->EWS_JAMMER_ON)
    {
        if (HasActivity())
        {
            //turn on ECM when getting locked up
            if (SimDriver.GetPlayerAircraft()->HasSPJamming() && !SimDriver.GetPlayerAircraft()->mFaults->GetFault(FaultClass::blkr_fault) &&
                !SimDriver.GetPlayerAircraft()->ManualECM)
            {
                if (SimDriver.GetPlayerAircraft()->HasPower(AircraftClass::EWSJammerPower))
                {
                    if (!SimDriver.GetPlayerAircraft()->IsSetFlag(ECM_ON))
                    {
                        //Only if fully auto
                        if (SimDriver.GetPlayerAircraft()->EWSPGM() == AircraftClass::Auto)
                        {
                            SimDriver.GetPlayerAircraft()->SetFlag(ECM_ON);
                        }

                        else if (SimDriver.GetPlayerAircraft()->EWSPGM() == AircraftClass::Semi) //ask if we want turn it on
                        {
                            if (!SaidJammer)
                            {
                                //F4SoundFXSetDist(SFX_BB_JAMMER, TRUE, 0.0f, 1.0f );
                                PlayRWRSoundFX(SFX_BB_JAMMER, TRUE, 0.0f, 1.0f);
                                SaidJammer = TRUE;
                            }
                        }
                    }
                }
            }
        }
        else
        {
            if (!SimDriver.GetPlayerAircraft()->ManualECM && SimDriver.GetPlayerAircraft()->EWSPGM() == AircraftClass::Auto)
            {
                // Can't turn off ECM w/ ECM pod broken
                if (!SimDriver.GetPlayerAircraft()->mFaults->GetFault(FaultClass::epod_fault))
                    SimDriver.GetPlayerAircraft()->UnSetFlag(ECM_ON);
            }

            if (SaidJammer)
                SaidJammer = FALSE;
        }
    }
}

BOOL PlayerRwrClass::IsOn()
{
    if (!g_bRWR) // JB 010802
        return false;

    return (((AircraftClass*)platform)->HasPower(AircraftClass::RwrPower) && VehRwrClass::IsOn());
}
