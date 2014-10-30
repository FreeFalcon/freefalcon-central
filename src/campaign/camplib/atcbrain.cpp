#include "stdhdr.h"
#include "atcbrain.h"
#include "campbase.h"
#include "MsgInc/ATCCmdMsg.h"
#include "MsgInc/RadioChatterMsg.h"
#include "falcsnd/conv.h"
#include "simveh.h"
#include "falcsess.h"
#include "squadron.h"
#include "flight.h"
#include "rules.h"
#include "ptdata.h"
#include "aircrft.h"
#include "digi.h"
#include "Weather.h"
#include "airframe.h"
#include "ptdata.h"
#include "Graphics/Include/drawobj.h"
#include "classtbl.h"
#include "otwdrive.h"
#include "falcsnd/voicemapper.h"

#if 0
// OW FIXME: this had to be added after installing the DX8 Beta 1 SDK
#define DIRECTINPUT_VERSION 0x0700
#include "dinput.h"
#else
#define USE_DINPUT_8
#ifndef USE_DINPUT_8 // Retro 15Jan2004
#define DIRECTINPUT_VERSION 0x0700
#else
#define DIRECTINPUT_VERSION 0x0800
#endif
#include <dinput.h>
#endif

#include "simdrive.h"
#include "tacan.h"
#include "cmpglobl.h"
#include "team.h"
#include "Graphics/Include/drawbsp.h"


//ATCBrain::atcList = NULL;
#ifdef USE_SH_POOLS
MEM_POOL runwayQueueStruct::pool;
MEM_POOL ATCBrain::pool;
#endif

#ifdef DEBUG
extern DWORD gSimThreadID;
#endif

extern float get_air_speed(float, int);
extern ulong gBumpTime;
extern int gBumpFlag;
extern short NumPtHeaders;
extern short NumPts;

extern int g_nATCTaxiOrderFix;

//constructor
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ATCBrain::ATCBrain(ObjectiveClass* mySelf)
{
    int i, rwindex, count, shortest, ptindex;
    ObjClassDataType *oc;

    self = mySelf;

    oc = mySelf->GetObjectiveClassData();
    numRwys = oc->DataRate;

    //DSP HACK
    if (numRwys > 4)
        numRwys = 1;

    minDeagTime = 0;
    shortest = 100;
    count = 0;

    // RAS - 16Jan04 - Traffic check Variables
    checkTrafficTime = 0;
    lastTrafficCallTime = 0;
    pLastTraffic = NULL;
    trafficCheck = noTraffic;
    trafficRange = 0;
    oldTrafficRange = 0;
    trafficAltitude = 0;
    trafficInSightFlag = FALSE;


#ifdef USE_SH_POOLS
    runwayStats = (runwayStatsStruct *)MemAllocPtr(runwayQueueStruct::pool,  sizeof(runwayStatsStruct) * (numRwys), 0);
    runwayQueue = (runwayQueueStruct **)MemAllocPtr(runwayQueueStruct::pool,  sizeof(runwayQueueStruct *) * (numRwys), 0);
#else
    runwayStats = new runwayStatsStruct[numRwys];
    runwayQueue = new runwayQueueStruct *[numRwys];
#endif

    for (i = 0; i < numRwys; i++)
    {
        runwayStats[i].rwIndexes[0] = 0;
        runwayStats[i].rwIndexes[1] = 0;
        runwayStats[i].nextEmergency = FalconNullId;
        runwayQueue[i] = NULL;
        runwayStats[i].numInQueue = 0;
        runwayStats[i].state = VIS_NORMAL;
    }

    rwindex = oc->PtDataIndex;

    while (rwindex)
    {
        if (GetQueue(rwindex) < numRwys)
        {
            if (PtHeaderDataTable[rwindex].type == RunwayPt)
            {
                if (runwayStats[GetQueue(rwindex)].rwIndexes[0])
                {
                    ShiAssert(rwindex >= 0);
                    runwayStats[GetQueue(rwindex)].rwIndexes[1] = rwindex;
                }
                else
                {
                    ShiAssert(rwindex >= 0);
                    runwayStats[GetQueue(rwindex)].rwIndexes[0] = rwindex;
                }

                count = 0;
                ptindex = PtHeaderDataTable[rwindex].first;
                ptindex = GetNextTaxiPt(ptindex);

                while (ptindex)
                {
                    ptindex = GetNextTaxiPt(ptindex);
                    count++;
                }

                if (count < shortest) // FRB - shortest, longest or mean - could effect time to start taxiing (minDeagTime)????
                    shortest = count;

#if 0 // JPO - I don't see why its rewriting all these. I think this is what screws up Osan.
                float x1, y1, x2, y2, dx, dy, norm;
                ptindex = PtHeaderDataTable[rwindex].first;
                TranslatePointData(self, ptindex, &x1, &y1);
                ptindex = GetNextPt(ptindex);
                TranslatePointData(self, ptindex, &x2, &y2);

                dx = x1 - x2;
                dy = y1 - y2;
                norm = 1.0F / (float)sqrt(dx * dx + dy * dy);
                dx *= norm;
                dy *= norm;

                PtHeaderDataTable[rwindex].cosHeading = dx;
                PtHeaderDataTable[rwindex].sinHeading = dy;
                float hdg = (float)atan2(dy, dx) * RTD / 10.0F;

                if (hdg < 0.0F)
                    hdg += 36.0F;

                int iHdg = FloatToInt32(hdg + 0.5F);

                PtHeaderDataTable[rwindex].data = (short)(iHdg * 10);
                PtHeaderDataTable[rwindex].texIdx = GetTextureIdFromHdg(iHdg, PtHeaderDataTable[rwindex].ltrt);
#endif
            }

            if (PtHeaderDataTable[rwindex].type == RunwayDimPt)
            {
                CalcRunwayDimensions(rwindex);
            }
        }

        rwindex = PtHeaderDataTable[rwindex].nextHeader;
    }

    if (shortest > 1)
        minDeagTime = (shortest - 1) * 15 * CampaignSeconds + 1;  // FRB - checkpoint
    else
        minDeagTime = 0;

    inboundQueue = NULL;

    int tempcall = callsign;

    //this will work for now
    if ( not gTacanList->GetCallsignFromCampID(self->GetCampId(), &tempcall))
    {
        callsign = 0;
#ifdef DAVE_DBG

        if (self->GetType() == TYPE_AIRBASE)
            ShiAssert( not "We have an airbase that is not listed in the tacan data file");

#endif
    }

    else
        callsign = tempcall;

    voice = g_voicemap.PickVoice(VoiceMapper::VOICE_ATC, self->GetCountry()); // (uchar)(12 + rand()%2);
    F4Assert(numRwys > 0);

}

//destructor
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ATCBrain::~ATCBrain(void)
{
    int i;

    for (i = 0; i < numRwys; i++)
    {
        //cleanup runway queue
        while (runwayQueue[i])
        {
            RemoveTraffic(runwayQueue[i]->aircraftID, i);
        }
    }

    while (inboundQueue)
    {
        RemoveInbound(inboundQueue);
    }

#ifdef USE_SH_POOLS
    MemFreePtr(runwayStats);
    MemFreePtr(runwayQueue);
#else
    delete [] runwayStats;
    delete [] runwayQueue;
#endif
    runwayStats = NULL;
    runwayQueue = NULL;
}

//this is executed every 5 seconds by simdriver on all atc's local to the machine
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ATCBrain::Exec(void)
{
    /* 2002-04-08 MN fix for taxiing order:
     we had AI taking off at 09:37, us at 09:36, and AI waiting in front of us. Reversing
     ProcessPlayers and ProcessRunways fixes that. */

    if (g_nATCTaxiOrderFix bitand 0x01)
    {
        ProcessPlayers();
        ProcessRunways();
        ProcessInbound();
    }
    // to experiment...
    else if (g_nATCTaxiOrderFix bitand 0x02)
    {
        ProcessInbound();
        ProcessPlayers();
        ProcessRunways();
    }
    else
    {
        ProcessRunways();
        ProcessPlayers();
        ProcessInbound();
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ATCBrain::ProcessInbound(void)
{
    float dx, dy, distSq;
    runwayQueueStruct *inboundInfo;
    runwayQueueStruct *deleteInfo;
    AircraftClass *inbound;

    if (inboundQueue and F4IsBadReadPtr(inboundQueue, sizeof(runwayQueueStruct))) // JB 010801 CTD
        return;

    inboundInfo = inboundQueue;

    while (inboundInfo)
    {
        deleteInfo = inboundInfo;



        inbound = (AircraftClass *)vuDatabase->Find(inboundInfo->aircraftID);

        if (inbound)
        {
            dx = inbound->XPos() - self->XPos();
            dy = inbound->YPos() - self->YPos();
            distSq = dx * dx + dy * dy; //distSq in feet

            inboundInfo = inboundInfo->next;

            if (distSq < TOWER_RANGE * NM_TO_FT * NM_TO_FT)
            {
                RemoveInbound(deleteInfo);

                //if they are within 15nm we need to start giving them vectors
                if ( not inbound->IsPlayer() and inbound->pctStrength < 0.9F)
                    RequestEmerClearance(inbound);
                else
                    RequestClearance(inbound);
            }
            else if (distSq > ATC_DROP_RANGE * NM_TO_FT * NM_TO_FT)
            {
                //if they are farther than 40nm we don't care about them anymore
                RemoveInbound(deleteInfo);
            }
        }
        else
        {
            //they went away, oh well
            inboundInfo = inboundInfo->next;
            RemoveInbound(deleteInfo);
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ATCBrain::ProcessRunways(void)
{
    int i, rwindex;
    runwayQueueStruct *nextLand[4]; //room for bigger airbases :)

    for (i = 0; i < numRwys; i++)
    {
        runwayStats[i].rnwyInUse = NULL;
        nextLand[i] = NextToLand(i);
    }

    runwayQueueStruct *info;
    FalconSessionEntity *session;
    AircraftClass *player;
    AircraftClass *aircraft;
    int queue = 0;

    VuSessionsIterator sit(FalconLocalGame);
    session = (FalconSessionEntity*) sit.GetFirst();

    while (session)
    {
        player = (AircraftClass*) session->GetPlayerEntity();

        if (player and player->IsAirplane())
        {
            unsigned long deltaTime;

            rwindex = IsOnRunway(player);
            queue = GetQueue(rwindex);

            if (nextLand[queue])
            {
                if (nextLand[queue]->schedTime < SimLibElapsedTime)
                    deltaTime = SimLibElapsedTime - nextLand[queue]->schedTime;
                else
                    deltaTime = nextLand[queue]->schedTime - SimLibElapsedTime;

                if (rwindex and player->af->vt < 30.0F * KNOTS_TO_FTPSEC and (player->DBrain()->ATCStatus() == lCrashed or
                        (player->DBrain()->ATCStatus() > lLanded and deltaTime < LAND_TIME_DELTA / 2) or
                        (player->DBrain()->ATCStatus() == lLanded and deltaTime < LAND_TIME_DELTA / 4 and player->af->vt < 5.0F)))
                {
                    runwayStats[queue].rnwyInUse = player;
                }

                //Cobra If a player is on the runway, we flag him.  This allows the landing check
                //to catch it and abort Digi's on final.
                if (rwindex)
                {
                    runwayStats[queue].rnwyInUse = player;
                }

                //end



            }
        }

        session = (FalconSessionEntity*) sit.GetNext();
    }

    //check runway status
    for (i = 0; i < numRwys; i++)
    {
        info = runwayQueue[i];

        while (nextLand[i] and not runwayStats[i].rnwyInUse and info)
        {
            unsigned long deltaTime;

            if (nextLand[i]->schedTime < SimLibElapsedTime) //If aircraft is scheduled to land
                deltaTime = SimLibElapsedTime - nextLand[i]->schedTime; //how much time do we have
            else
                deltaTime = nextLand[i]->schedTime - SimLibElapsedTime; //land time passed

            //if acft crashed, or schedule to takerwy or takeoff, and aircraft scheduled to land is less than LAND_TIME_DELTA/2 = 30sec away
            //if( info->status == lCrashed or ((info->status == tTakeRunway or info->status == tTakeoff) and deltaTime < LAND_TIME_DELTA/2))
            //cobra
            if (info->status == lCrashed or info->status == tTakeRunway or info->status == tTakeoff)
            {
                aircraft = (AircraftClass*)vuDatabase->Find(info->aircraftID);

                if (aircraft and aircraft->IsAirplane() and aircraft->af->vt < 70.0F * KNOTS_TO_FTPSEC)
                {
                    rwindex = IsOnRunway(aircraft);

                    if (rwindex)
                        runwayStats[i].rnwyInUse = aircraft;
                }
            }

            info = info->next;
        }

        runwayStats[i].state = CheckHeaderStatus(self, runwayStats[i].rwIndexes[0]);

        if (runwayStats[i].state >= VIS_DAMAGED and runwayQueue[i])
        {
            // need to reschedule these planes
            ReschedulePlanes(i);
        }

        if (runwayQueue[i])
        {
            //need to check queues for planes on emergencyhold or emergencystop
            //the emergency is over we can bring 'em in, also remove any planes we
            //can no longer find in the database
            ProcessQueue(i);
        }
    }
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ATCBrain::ProcessQueue(int queue)
{
    AircraftClass *aircraft = NULL;
    runwayQueueStruct *info = NULL;
    runwayQueueStruct *deleteInfo = NULL;
    runwayQueueStruct *nextTakeoff = NULL;
    runwayQueueStruct *nextLand = NULL;
    runwayQueueStruct *temp = NULL;
    AircraftClass *pNTOAircraft = NULL;
    AircraftClass *pNLAircraft = NULL;
    FalconRadioChatterMessage *radioMessage = NULL;
    int waitforlanding = FALSE;
    int accelerateTakeoffs = FALSE;

    deleteInfo = info = runwayQueue[queue];

    nextTakeoff = NextToTakeoff(queue);

    if (nextTakeoff)
        pNTOAircraft = (AircraftClass*)vuDatabase->Find(nextTakeoff->aircraftID);

    nextLand = NextToLand(queue);

    if (nextLand)
        pNLAircraft = (AircraftClass*)vuDatabase->Find(nextLand->aircraftID);

    // RAS - if within 58 seconds of landing time, hold short
    if (nextLand and SimLibElapsedTime + LAND_TIME_DELTA - 2 * CampaignSeconds > nextLand->schedTime)
        waitforlanding = TRUE;

    // RAS - if TO scheduled, and TO time has passed, get them off the ground ASAP
    if (nextTakeoff and nextTakeoff->schedTime < SimLibElapsedTime)
    {
        if ( not pNTOAircraft or (pNTOAircraft->IsAirplane() and pNTOAircraft->af->vt > 80.0F * KNOTS_TO_FTPSEC))
            accelerateTakeoffs = TRUE;
    }

    // RAS - While we have aircraft in the runway queue
    while (info)
    {
        aircraft = (AircraftClass*)vuDatabase->Find(info->aircraftID);

        if (aircraft and aircraft->IsAirplane())
        {
            //total hack to make sure this variable is correct
            aircraft->DBrain()->isWing = aircraft->GetCampaignObject()->GetComponentIndex(aircraft);

            if ( not aircraft->IsPlayer() and SimLibElapsedTime > info->schedTime + FalconLocalGame->rules.AiPullTime)
            {
                // sfr: we had an infinite loop in some situations, since info never got updated
                // maybe this should go just after the while(info)
                info = info->next;
                RegroupAircraft(aircraft);
            }
            else
            {
                switch (info->status)
                {
                    case noATC:
                        info = info->next;

                        if (deleteInfo->schedTime < SimLibElapsedTime)
                            RemoveTraffic(deleteInfo->aircraftID, queue);

                        break;

                    case tWait:
                        if (nextTakeoff->aircraftID == info->aircraftID)
                        {
                            if (info->prev)
                            {
                                // RAS - if we're within 30 sec of T/O, and there is a previous aircraft
                                // and we aren't waiting for a landing, and we're in accelerate TO or
                                // status is lLanded, then enter this section
                                // don't know why it is status lLanded???
                                if (info->schedTime < SimLibElapsedTime + 30 * CampaignSeconds and 
                                    info->prev == runwayQueue[queue] and not waitforlanding and 
                                    (accelerateTakeoffs or runwayQueue[queue]->status ==  lLanded))
                                {
                                    info->status = tTakeRunway;
                                    SendCmdMessage(aircraft, info);
                                    GiveOrderToWingman(aircraft, info->status);

                                    if (aircraft->vehicleInUnit < 2)
                                        GiveOrderToSection(aircraft, tPrepToTakeRunway, 1);
                                }
                                // RAS - if previous aircraft has requested T/O, then enter here
                                else if (info->prev->status < tReqTaxi)
                                {
                                    info->status = tHoldShort;
                                    SendCmdMessage(aircraft, info);

                                    radioMessage = CreateCallFromATC(self, aircraft, rcHOLDSHORT, FalconLocalGame);
                                    radioMessage->dataBlock.edata[3] = 0;

                                    if (info->prev->status < lCrashed)
                                        radioMessage->dataBlock.edata[3] = (short)(1 + (rand() % 4));

                                    FalconSendMessage(radioMessage, FALSE);
                                }
                            }
                            //RAS - if no previous aircraft in the queue
                            //if we're within 30 sec of T/O and we're not waiting for a landing
                            else if (info->schedTime < SimLibElapsedTime + 30 * CampaignSeconds and not waitforlanding)
                            {
                                info->status = tTakeoff;
                                SendCmdMessage(aircraft, info);
                                GiveOrderToWingman(aircraft, info->status);

                                if (aircraft->vehicleInUnit < 2)
                                    GiveOrderToSection(aircraft, tPrepToTakeRunway, 1);
                            }
                            //RAS - we haven't reached T/O time so hold short
                            else
                            {
                                info->status = tHoldShort;
                                SendCmdMessage(aircraft, info);

                                radioMessage = CreateCallFromATC(self, aircraft, rcHOLDSHORT, FalconLocalGame);
                                radioMessage->dataBlock.edata[3] = 0;

                                if (info->prev and info->prev->status < lCrashed)
                                    radioMessage->dataBlock.edata[3] = (short)(1 + (rand() % 4));

                                FalconSendMessage(radioMessage, FALSE);
                            }
                        }

                        info = info->next;

                        if ( not aircraft->IsPlayer() and not aircraft->OnGround())
                        {
                            RemoveTraffic(deleteInfo->aircraftID, queue);
                        }

                        break;

                    case tTaxi:

                        //RAS - we have aircraft in the queue
                        if (nextTakeoff->aircraftID == info->aircraftID)
                        {
                            //RAS - we have someone in the queue in front of us
                            if (info->prev)
                            {
                                // we're within 30 sec of T/O time and someone in the queue in front of us
                                // and we're not waiting on a landing aircraft
                                // and we're in accelerated T/O mode or status queue = lLanded
                                if (info->schedTime < SimLibElapsedTime + 30 * CampaignSeconds and 
                                    info->prev == runwayQueue[queue] and not waitforlanding and 
                                    (accelerateTakeoffs or runwayQueue[queue]->status ==  lLanded))
                                {
                                    //RAS - lead and 2 take the runway
                                    info->status = tTakeRunway;
                                    SendCmdMessage(aircraft, info);
                                    GiveOrderToWingman(aircraft, info->status);

                                    //RAS - 3 and 4 prep to take runway
                                    if (aircraft->vehicleInUnit < 2)
                                        GiveOrderToSection(aircraft, tPrepToTakeRunway, 1);
                                }
                                else if (info->prev->status < tReqTaxi and info->prev->status > noATC)
                                {
                                    info->status = tHoldShort;
                                    SendCmdMessage(aircraft, info);

                                    radioMessage = CreateCallFromATC(self, aircraft, rcHOLDSHORT, FalconLocalGame);
                                    radioMessage->dataBlock.edata[3] = 0;

                                    if (info->prev->status < lCrashed)
                                        radioMessage->dataBlock.edata[3] = (short)(1 + (rand() % 4));

                                    FalconSendMessage(radioMessage, FALSE);
                                }
                            }
                            //RAS - we're first in queue and within 30 sec of T/O and not waiting for ldg
                            else if (info->schedTime < SimLibElapsedTime + 30 * CampaignSeconds and not waitforlanding)
                            {
                                // RAS - 1 and 2 takeoff
                                info->status = tTakeoff;
                                SendCmdMessage(aircraft, info);
                                GiveOrderToWingman(aircraft, info->status);

                                // RAS - 3 and 4 prep to take runway
                                if (aircraft->vehicleInUnit < 2)
                                    GiveOrderToSection(aircraft, tPrepToTakeRunway, 1);
                            }
                            //RAS - not with 30sec of T/O time so hold short
                            else
                            {
                                info->status = tHoldShort;
                                SendCmdMessage(aircraft, info);

                                radioMessage = CreateCallFromATC(self, aircraft, rcHOLDSHORT, FalconLocalGame);
                                radioMessage->dataBlock.edata[3] = 0;

                                if (info->prev and info->prev->status < lCrashed)
                                    radioMessage->dataBlock.edata[3] = (short)(1 + (rand() % 4));

                                FalconSendMessage(radioMessage, FALSE);
                            }
                        }
                        //RAS - need to get here if taxi order is messed up I beleive
                        //RAS - if next T/O acft and it's an airplane, and scheudle T/O time + ATC patience hasn't been reached
                        //and acft is going less than 5 kts (should that be conferted to ft/sec??), and T/O time has passed
                        //and next acft to T/O is not current obj???, and next acft to T/O is not on rwy, then
                        else if (pNTOAircraft and pNTOAircraft->IsAirplane() and nextTakeoff->schedTime + FalconLocalGame->rules.AtcPatience < SimLibElapsedTime  and 
                                 pNTOAircraft->af->vt < 5.0F * KNOTS_TO_FTPSEC and info->schedTime < SimLibElapsedTime  and 
                                 pNTOAircraft->GetCampaignObject() not_eq aircraft->GetCampaignObject() and not IsOnRunway(pNTOAircraft))
                        {
                            //RAS - no aircraft on final and past T/O time + delta(60sec)
                            if ( not nextLand or SimLibElapsedTime + LAND_TIME_DELTA > nextLand->schedTime)
                            {
                                ReorderFlight(queue, (Flight)pNTOAircraft->GetCampaignObject(), tWait);

                                info->status = tTakeoff;
                                SendCmdMessage(aircraft, info);
                                GiveOrderToWingman(aircraft, info->status);

                                if (aircraft->vehicleInUnit < 2)
                                    GiveOrderToSection(aircraft, tPrepToTakeRunway, 1);
                            }
                        }

                        info = info->next;

                        if ( not aircraft->IsPlayer() and not aircraft->OnGround())
                            RemoveTraffic(deleteInfo->aircraftID, queue);

                        break;

                    case tPrepToTakeRunway:
                        if (nextTakeoff->aircraftID == info->aircraftID)
                        {
                            //RAS - no aircraft on final
                            if ( not waitforlanding)
                            {
                                info->status = tTakeoff;
                                SendCmdMessage(aircraft, info);
                                GiveOrderToWingman(aircraft, info->status);

                                if (aircraft->vehicleInUnit < 2)
                                    GiveOrderToSection(aircraft, tPrepToTakeRunway, 1);
                            }
                        }

                        info = info->next;

                        if ( not aircraft->IsPlayer() and not aircraft->OnGround())
                            RemoveTraffic(deleteInfo->aircraftID, queue);

                        break;

                    case tHoldShort:
                        if (nextTakeoff->aircraftID == info->aircraftID)
                        {
                            //RAS - within 30sec of T/O and not waiting for someone to land
                            if (info->schedTime < SimLibElapsedTime + 30 * CampaignSeconds and not waitforlanding)
                            {
                                info->status = tTakeoff;
                                SendCmdMessage(aircraft, info);
                                GiveOrderToWingman(aircraft, info->status);

                                if (aircraft->vehicleInUnit < 2)
                                    GiveOrderToSection(aircraft, tPrepToTakeRunway, 1);
                            }
                        }
                        else if (pNTOAircraft and pNTOAircraft->IsAirplane() and nextTakeoff->schedTime + FalconLocalGame->rules.AtcPatience < SimLibElapsedTime  and 
                                 pNTOAircraft->af->vt < 5.0F * KNOTS_TO_FTPSEC and info->schedTime < SimLibElapsedTime and 
                                 pNTOAircraft->GetCampaignObject() not_eq aircraft->GetCampaignObject() and not IsOnRunway(pNTOAircraft))
                        {
                            if ( not nextLand or SimLibElapsedTime + LAND_TIME_DELTA > nextLand->schedTime)
                            {
                                ReorderFlight(queue, (Flight)pNTOAircraft->GetCampaignObject(), tWait);

                                info->status = tTakeoff;
                                SendCmdMessage(aircraft, info);
                                GiveOrderToWingman(aircraft, info->status);

                                if (aircraft->vehicleInUnit < 2)
                                    GiveOrderToSection(aircraft, tPrepToTakeRunway, 1);
                            }
                        }

                        info = info->next;

                        if ( not aircraft->IsPlayer() and not aircraft->OnGround())
                            RemoveTraffic(deleteInfo->aircraftID, queue);

                        break;

                    case tTakeRunway:

                        //RAS - if not on runway and waiting for someone to land
                        if (waitforlanding and not IsOnRunway(aircraft))
                        {
                            //TODO: RAS - maybe this should be hold short???
                            info->status = tPrepToTakeRunway;
                            SendCmdMessage(aircraft, info);

                            radioMessage = CreateCallFromATC(self, aircraft, rcHOLDSHORT, FalconLocalGame);
                            radioMessage->dataBlock.edata[3] = 0;

                            if (info->prev and info->prev->status < lCrashed)
                                radioMessage->dataBlock.edata[3] = (short)(1 + (rand() % 4));

                            FalconSendMessage(radioMessage, FALSE);
                        }
                        else if (nextTakeoff->aircraftID == info->aircraftID)
                        {
                            //RAS - within 30 seconds of T/O time and not waiting on landing or we're on rwy
                            if (info->schedTime < SimLibElapsedTime + 30 * CampaignSeconds and 
                                ( not waitforlanding or IsOnRunway(aircraft)))
                            {
                                info->status = tTakeoff;
                                SendCmdMessage(aircraft, info);
                                GiveOrderToWingman(aircraft, info->status);

                                if (aircraft->vehicleInUnit < 2)
                                    GiveOrderToSection(aircraft, tPrepToTakeRunway, 1);
                            }
                        }

                        info = info->next;

                        if ( not aircraft->IsPlayer() and not aircraft->OnGround())
                            RemoveTraffic(deleteInfo->aircraftID, queue);

                        break;

                        //TODO: RAS - there only seems to be one case under takeoff which is waiting for aircraft to land
                        // might need to add section if not waiting for aircraft to land?????
                    case tTakeoff:

                        //RAS - waiting for landing and not over rwy and speed less than 45kts
                        if (waitforlanding and not IsOverRunway(aircraft) and aircraft->af->vt < 45.0F * KNOTS_TO_FTPSEC)
                        {
                            //TODO: RAS - waiting for landing so should this be tHoldShort???
                            info->status = tPrepToTakeRunway;
                            SendCmdMessage(aircraft, info);

                            radioMessage = CreateCallFromATC(self, aircraft, rcHOLDSHORT, FalconLocalGame);
                            radioMessage->dataBlock.edata[3] = 0;

                            if (info->prev and info->prev->status < lCrashed)
                                radioMessage->dataBlock.edata[3] = (short)(1 + (rand() % 4));

                            FalconSendMessage(radioMessage, FALSE);
                        }

                        info = info->next;

                        if ( not aircraft->IsPlayer() and not aircraft->OnGround())
                            RemoveTraffic(deleteInfo->aircraftID, queue);

                        break;

                    case tEmerStop:
                        if (runwayStats[GetQueue(info->rwindex)].nextEmergency == FalconNullId)
                        {
                            info->status = tTaxi;
                            SendCmdMessage(aircraft, info);
                        }

                        info = info->next;

                        if ( not aircraft->IsPlayer() and not aircraft->OnGround())
                            RemoveTraffic(deleteInfo->aircraftID, queue);

                        break;

                    case lEmerHold:
                        info = info->next;

                        if (runwayStats[GetQueue(deleteInfo->rwindex)].nextEmergency == FalconNullId)
                        {
                            RemoveTraffic(deleteInfo->aircraftID, queue);
                            RequestClearance(aircraft);
                        }

                        break;

                    case lHolding:
                    case lToBase:
                        if (info->schedTime < SimLibElapsedTime + FINAL_TIME)
                        {
                            info->status = lAborted;
                            SendCmdMessage(aircraft, info);
                        }

                        info = info->next;

                        if ( not aircraft->IsPlayer() and aircraft->OnGround())
                            RemoveTraffic(deleteInfo->aircraftID, queue);

                        break;

                    case lFirstLeg:
                    case lToFinal:
                        if (info->schedTime < SimLibElapsedTime + CampaignMinutes)
                        {
                            info->status = lAborted;
                            SendCmdMessage(aircraft, info);
                        }

                        info = info->next;

                        if ( not aircraft->IsPlayer() and aircraft->OnGround())
                            RemoveTraffic(deleteInfo->aircraftID, queue);

                        break;

                    case lOnFinal:
                    case lClearToLand:
                        temp = info->next;

                        while (temp)
                        {
                            if (temp->status >= lReqClearance and temp->status < lLanded)
                            {
                                break;
                            }

                            temp = temp->next;
                        }

                        if (info->schedTime < SimLibElapsedTime and (temp or runwayStats[queue].nextEmergency not_eq FalconNullId))
                        {
                            if ( not aircraft->DBrain()->IsSetATC(DigitalBrain::ClearToLand) or
                                info->schedTime + CampaignMinutes < SimLibElapsedTime)
                            {
                                //if there is someone who needs to use the runway after us
                                info->status = lAborted;
                                SendCmdMessage(aircraft, info);
                            }
                        }

                        //need to warn off if there is someone in the way
                        CheckFinalApproach(aircraft, info);
                        info = info->next;
                        //if they land the land() function will update their status
                        break;

                    case lTaxiOff:
                    case tFlyOut:
                        info = info->next;
                        RemoveTraffic(deleteInfo->aircraftID, queue);
                        break;

                    case tReqTaxi:
                    case tReqTakeoff:
                    case tTaxiBack:
                        info = info->next;

                        if ( not aircraft->IsPlayer() and not aircraft->OnGround())
                            RemoveTraffic(deleteInfo->aircraftID, queue);

                        break;

                    case lReqClearance:
                        info = info->next;

                        if ( not aircraft->IsPlayer() and aircraft->OnGround())
                            RemoveTraffic(deleteInfo->aircraftID, queue);

                        break;

                    case lReqEmerClearance:
                        info = info->next;

                        if ( not aircraft->IsPlayer() and aircraft->OnGround())
                            RemoveTraffic(deleteInfo->aircraftID, queue);

                        break;

                    case lEmergencyToBase:
                    case lEmergencyToFinal:
                    case lEmergencyOnFinal:
                        info = info->next;

                        if ( not aircraft->IsPlayer() and aircraft->OnGround())
                            RemoveTraffic(deleteInfo->aircraftID, queue);

                        break;

                    case lCrashed:
                        info = info->next;
                        break;

                    case lIngressing:
                    case lTakingPosition:
                    case lAborted:
                        info = info->next;

                        if ( not aircraft->IsPlayer() and aircraft->OnGround())
                            RemoveTraffic(deleteInfo->aircraftID, queue);

                        break;

                    case lLanded:
                        info = info->next;
                        break;

                    default:
                        info = info->next;
                        //we should never get here
                        ShiWarning("We are in an undefined state, we shouldn't be here");
                }
            }

            deleteInfo = info;
        }
        else
        {
            info = info->next;

            if ( not aircraft or ( not aircraft->IsAirplane() and deleteInfo->schedTime + 30 * CampaignSeconds < SimLibElapsedTime))
                RemoveTraffic(deleteInfo->aircraftID, queue);

            deleteInfo = info;
        }
    }
}


// deal with those unruly players (why do we need them anyways? they're always causing problems)
// Check vs all players
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ATCBrain::ProcessPlayers(void)
{
    float dx, dy, cosAngle;
    int queue;
    runwayQueueStruct *playerInfo;
    FalconSessionEntity *session;
    ulong min, max;
    AircraftClass *player;
    FalconRadioChatterMessage *radioMessage;

    VuSessionsIterator sit(FalconLocalGame);
    session = (FalconSessionEntity*) sit.GetFirst();

    while (session)
    {
        player = (AircraftClass*) session->GetPlayerEntity();

        if (player and player->IsAirplane())
        {
            dx = player->XPos() - self->XPos();
            dy = player->YPos() - self->YPos();

            //if they're more than 30nm away, we don't care what they're doing
            if (dx * dx + dy * dy < APPROACH_RANGE * NM_TO_FT * NM_TO_FT)
            {
                playerInfo = InList(player->Id());

                if (playerInfo)
                {
                    //total hack to make sure this variable is correct
                    player->DBrain()->isWing = player->GetCampaignObject()->GetComponentIndex(player);

                    queue = PtHeaderDataTable[playerInfo->rwindex].runwayNum;

                    cosAngle = DetermineAngle(player, playerInfo->rwindex, playerInfo->status);

                    CalculateMinMaxTime(player, playerInfo->rwindex,
                                        playerInfo->status, &min, &max, cosAngle);


                    switch (playerInfo->status)
                    {
                        case lCrashed:
                            //maybe I should do something here?
                            break;

                        case lAborted:
                            CheckLanding(player, playerInfo);
                            break;

                        case lIngressing:

                            //we're more than 30nm
                        case lTakingPosition:

                            //the inbound processor will put the player into a runway queue when
                            //he breaks 15nm.
                        case lEmerHold:
                            CheckLanding(player, playerInfo);
                            break;

                        case lHolding:
                            if (CheckLanding(player, playerInfo))
                                break;

                            if (playerInfo->schedTime < SimLibElapsedTime + max - CampaignSeconds * 5)
                            {
                                if (cosAngle < 0.0F)
                                {
                                    playerInfo->status  = lToBase;
                                    SendCmdMessage(player, playerInfo);
                                }
                                else
                                {
                                    playerInfo->status  = lToFinal;
                                    SendCmdMessage(player, playerInfo);
                                }
                            }

                            break;

                        case lEmergencyToBase:
                        case lEmergencyToFinal:
                            CheckForTraffic(player, playerInfo);      //RAS - 20Jan04 - traffic calls
                            CheckLanding(player, playerInfo);
                            break;

                        case lFirstLeg:
                        case lToBase:
                        case lToFinal:
                            CheckForTraffic(player, playerInfo); //RAS - 20Jan04 -  traffic calls

                            if (CheckLanding(player, playerInfo))
                                break;

                            if (CheckVector(player, playerInfo))
                                SendCmdMessage(player, playerInfo);

                            break;

                        case lEmergencyOnFinal:
                            CheckForTraffic(player, playerInfo); //RAS - 20Jan04 -  traffic calls
                            CheckLanding(player, playerInfo);
                            break;

                        case lOnFinal:
                        case lClearToLand:

                            // CheckForTraffic(player, playerInfo); //RAS - 20Jan04 -  add later
                            if (CheckLanding(player, playerInfo))
                                break;

                            if (CheckVector(player, playerInfo))
                                SendCmdMessage(player, playerInfo);

                            break;

                        case lLanded:
                            if (CheckTakeoff(player, playerInfo))
                                break;

                            if (CheckLanding(player, playerInfo))
                                break;

                            CheckIfBlockingRunway(player, playerInfo);

                            // OW: Jackals "scold-on-bounce" fix
#if 1

                            // JB 000421
                            if (player->GetKias() < 50)
                            {
                                player->DBrain()->SetWaitTimer(SimLibElapsedTime);
                                RemoveTraffic(player->Id(), PtHeaderDataTable[playerInfo->rwindex].runwayNum);
                            }

                            // JB 000421
#endif

                            break;

                        case lTaxiOff:
                            if (CheckTakeoff(player, playerInfo))
                                break;

                            RemoveTraffic(player->Id(), PtHeaderDataTable[playerInfo->rwindex].runwayNum);

                            CheckIfBlockingRunway(player, playerInfo);
                            break;

                        case tEmerStop:
                            CheckTakeoff(player, playerInfo);
                            //check to see if emergency is over, if so continue on
                            break;

                        case tWait:
                        case tPrepToTakeRunway:
                        case tTaxi:
                        case tTaxiBack:
                            if (CheckTakeoff(player, playerInfo))
                            {
                                break;
                            }
                            else if (SimDriver.GetPlayerEntity() == player and gBumpFlag and gBumpTime < SimLibElapsedTime and playerInfo->schedTime + CampaignMinutes < SimLibElapsedTime)
                            {
                                player->DBrain()->SetATCFlag(DigitalBrain::TakeoffAborted);
                                OTWDriver.ExitMenu(DIK_E);
                            }

                            CheckIfBlockingRunway(player, playerInfo);
                            break;

                        case tHoldShort:
                            if (CheckTakeoff(player, playerInfo))
                            {
                                break;
                            }

                            CheckIfBlockingRunway(player, playerInfo);
                            break;

                        case tTakeRunway:
                            CheckTakeoff(player, playerInfo);
                            break;

                        case tTakeoff:
                            if ( not player->DBrain()->IsSetATC(DigitalBrain::MissionCanceled) and CheckTakeoff(player, playerInfo))
                            {
                                break;
                            }
                            else if (SimDriver.GetPlayerEntity() == player and gBumpFlag and gBumpTime < SimLibElapsedTime and playerInfo->schedTime + CampaignMinutes < SimLibElapsedTime)
                            {
                                player->DBrain()->SetATCFlag(DigitalBrain::TakeoffAborted);
                                OTWDriver.ExitMenu(DIK_E);
                            }
                            else if ( not player->af->IsSet(AirframeClass::GearBroken) and playerInfo->lastContacted + 60 * CampaignSeconds < SimLibElapsedTime)
                            {
                                if (SimLibElapsedTime > playerInfo->schedTime + 4 * CampaignMinutes)  // 06FEB04 - FRB - was 2 minutes
                                {
                                    if (rand() % 3)
                                        SendCallFromATC(self, player, rcDISRUPTINGTRAFFIC, FalconLocalGame);
                                    else
                                        SendCallFromATC(self, player, rcHURRYUP, FalconLocalGame);
                                }
                                else if (SimLibElapsedTime > playerInfo->schedTime + 2 * CampaignMinutes)  // 06FEB04 - FRB - was 1 minute
                                {
                                    SendCallFromATC(self, player, rcHURRYUP, FalconLocalGame);
                                }
                                else if (SimLibElapsedTime > playerInfo->schedTime + 60 * CampaignSeconds)  // 06FEB04 - FRB - was 15 seconds
                                {
                                    //yell to hurry up
                                    radioMessage = CreateCallFromATC(self, player, rcEXPEDITEDEPARTURE, FalconLocalGame);

                                    if (runwayQueue[queue]->status == lOnFinal)
                                        radioMessage->dataBlock.edata[3] = 1;
                                    else
                                        radioMessage->dataBlock.edata[3] = 0;

                                    FalconSendMessage(radioMessage, FALSE);
                                }

                                playerInfo->lastContacted = SimLibElapsedTime;
                            }

                            break;

                        case tFlyOut:
                            if (player->AutopilotType() not_eq AircraftClass::CombatAP)
                            {
                                player->DBrain()->SetATCStatus(noATC);
                                //remove from list
                                RemoveTraffic(player->Id(), PtHeaderDataTable[playerInfo->rwindex].runwayNum);
                            }

                            break;

                        default:
                            //we should never get here
                            ShiWarning("We are in an undefined state, we shouldn't be here");
                    }
                }
                else
                {
                    if (GetTTRelations(self->GetTeam(), player->GetTeam()) < Hostile)
                    {
                        //they haven't called us yet
                        if (player->OnGround())
                        {
                            //check to see if at their takeoff waypoint, if so request takeoff
                            if (player->curWaypoint and player->curWaypoint->GetWPAction() == WP_TAKEOFF and not player->DBrain()->isWing and 
                                player->DBrain()->Airbase() == self->Id() and player->DBrain()->IsSetATC(DigitalBrain::RequestTakeoff))
                            {
                                player->DBrain()->ClearATCFlag(DigitalBrain::RequestTakeoff);
                                RequestTakeoff(player);
                            }
                            else
                            {
                                //they may have landed without permission
                                if ( not player->DBrain()->IsSetATC(DigitalBrain::Landed))
                                {
                                    //landed without permission
                                    ObjectiveClass* curObj;
                                    //does someone else already have us? if so let them deal with us
                                    {
                                        // iterator must be destroyed here
                                        VuListIterator findWalker(SimDriver.atcList);
                                        curObj = (ObjectiveClass*)findWalker.GetFirst();

                                        while (curObj)
                                        {
                                            if (
                                                curObj and 
                                                (curObj not_eq self) and 
                                                (curObj->GetType() == TYPE_AIRBASE) and 
                                                curObj->brain->InList(player->Id())
                                            )
                                            {
                                                break;
                                            }

                                            curObj = (ObjectiveClass*)findWalker.GetNext();
                                        }
                                    }

                                    if ( not curObj)
                                    {
                                        // sfr: fixing xy order
                                        GridIndex gx, gy;
                                        //CX = SimToGrid(player->YPos());
                                        //CY = SimToGrid(player->XPos());
                                        ::vector pos = {player->XPos(), player->YPos()};
                                        ConvertSimToGrid(&pos, &gx, &gy);

                                        //we're on our own, so if there is a nearby airbase, he will yell at us
                                        Objective AirBase = FindNearbyAirbase(gx, gy);

                                        if (AirBase and player->pctStrength >= 0.99F)
                                        {
                                            float groundZ = OTWDriver.GetGroundLevel(player->XPos(), player->YPos());
                                            player->FeatureCollision(groundZ);

                                            if (player->onFlatFeature)
                                            {
                                                radioMessage = CreateCallFromATC(AirBase, player, rcTOWERSCOLD3, FalconLocalGame);
                                                radioMessage->dataBlock.edata[3] = 32767;
                                                FalconSendMessage(radioMessage, FALSE);
                                                player->DBrain()->SetWaitTimer(SimLibElapsedTime);
                                            }
                                        }

                                        runwayQueueStruct landInfo;
                                        player->DBrain()->SetATCFlag(DigitalBrain::Landed);
                                        landInfo.rwindex = 0;
                                        landInfo.status = lLanded;
                                        SendCmdMessage(player, &landInfo);
                                    }
                                }

                                CheckIfBlockingRunway(player, playerInfo);
                            }
                        }
                        else
                        {
                            //they may have taken off without permission
                            if (player->DBrain()->IsSetATC(DigitalBrain::Landed))
                            {
                                //took off without permission
                                ObjectiveClass* curObj;
                                {
                                    //does someone else already have us? if so let them deal with us
                                    // destroy iterator here
                                    VuListIterator findWalker(SimDriver.atcList);
                                    curObj = (ObjectiveClass*)findWalker.GetFirst();

                                    while (curObj)
                                    {
                                        if (
                                            curObj and (curObj not_eq self) and 
                                            (curObj->GetType() == TYPE_AIRBASE) and 
                                            (curObj->brain->InList(player->Id()))
                                        )
                                        {
                                            break;
                                        }

                                        curObj = (ObjectiveClass*)findWalker.GetNext();
                                    }
                                }

                                if ( not curObj)
                                {
                                    // sfr: fixing xy order
                                    GridIndex gx, gy;
                                    //gx = SimToGrid(player->YPos());
                                    //gy = SimToGrid(player->XPos());
                                    ::vector pos = {player->XPos(), player->YPos()};
                                    ConvertSimToGrid(&pos, &gx, &gy);
                                    //we're on our own, so if there is a nearby airbase, he will yell at us
                                    Objective AirBase = FindNearbyAirbase(gx, gy);

                                    if (AirBase)
                                    {
                                        float groundZ = OTWDriver.GetGroundLevel(player->XPos(), player->YPos());
                                        player->FeatureCollision(groundZ);

                                        if (player->onFlatFeature)
                                        {
                                            radioMessage = CreateCallFromATC(
                                                               self, player, rcTOWERSCOLD2, FalconLocalGame
                                                           );
                                            radioMessage->dataBlock.edata[3] = 32767;
                                            FalconSendMessage(radioMessage, FALSE);
                                            player->DBrain()->SetWaitTimer(SimLibElapsedTime);
                                        }
                                    }

                                    player->DBrain()->ClearATCFlag(DigitalBrain::Landed);
                                    runwayQueueStruct info ;
                                    info.lastContacted = SimLibElapsedTime;
                                    info.status = tFlyOut;
                                    SendCmdMessage(player, &info);
                                }
                            }
                        }
                    }
                }
            }
        }

        session = (FalconSessionEntity*) sit.GetNext();
    }
}

/*----------------------------------*/
/* Get a landing clearance from ATC */
/*----------------------------------*/
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ATCBrain::RequestClearance(AircraftClass* approaching, int addflight)
{
    int queue, rwindex;
    ulong landTime, max, min;
    float cosAngle;
    runwayQueueStruct *info;
    float finalX, finalY, baseX, baseY, x , y;

    if ( not approaching or approaching->OnGround())
        return;

    // OW - sylvains refuelling fix
#if 1

    // ADDED BY S.G. - FIX FOR THE CTD WHEN TUNED TO THE TANKER TACAN AND ASKING FOR LANDING
    // WHY SELF IS NULL, I DON'T KNOW, BUT IT IS :-(
    if ( not self)
        return;

    // END OF ADDED SECTION
#endif

    if (F4IsBadReadPtr(self, sizeof(ObjectiveClass)) or F4IsBadReadPtr(approaching, sizeof(AircraftClass))) // JB 010326 CTD
        return;

    if (GetTTRelations(self->GetTeam(), approaching->GetTeam()) >= Hostile)
        return;

    /*
    if (underAttack)
    {
       FalconRadioChatterMessage* radioMessage = new FalconRadioChatterMessage( approaching->Id(), FalconLocalSession );
       radioMessage->dataBlock.from = ((ATCBrain*)self)->Self()->Id();
       radioMessage->dataBlock.to = MESSAGE_FOR_FLIGHT;
       radioMessage->dataBlock.voice_id = ((int)self) % 12;
       radioMessage->dataBlock.message = rcBASEUNDERATTACK;
       radioMessage->dataBlock.edata[0] = approaching->GetCallsignIdx();
       radioMessage->dataBlock.edata[1] = ((Flight)approaching->GetCampaignObject())->GetPilotCallNumber(approaching->vehicleInUnit);
       FalconSendMessage(radioMessage, FALSE);
       //PlayRadioMessage(rcBASEUNDERATTACK)
       //approaching is the AC asking for clearance
       //self is ATC
       return;
    }*/

    RemoveFromAllATCs(approaching);

    rwindex = FindBestLandingRunway(approaching, TRUE);
    queue = GetQueue(rwindex);

    if ( not rwindex)
    {
        //all runways destroyed, divert 'em
        FalconATCCmdMessage* ATCCmdMessage = new FalconATCCmdMessage(approaching->Id(), FalconLocalGame);
        ATCCmdMessage->dataBlock.from = self->Id();
        ATCCmdMessage->dataBlock.type = FalconATCCmdMessage::Divert;
        ATCCmdMessage->dataBlock.rwindex = 0;
        FalconSendMessage(ATCCmdMessage, FALSE); // Send it
        return;
    }

    FindFinalPt(approaching, rwindex, &finalX, &finalY);

    if ( not addflight)
    {
        RemoveFromAllATCs(approaching);

        cosAngle = DetermineAngle(approaching, rwindex, lHolding);
        CalculateMinMaxTime(approaching, rwindex, lReqClearance, &min, &max, cosAngle);
        landTime = GetNextAvailRunwayTime(queue, SimLibElapsedTime + min, LAND_TIME_DELTA);

        info = AddTraffic(approaching->Id(), lHolding, rwindex, landTime);

        if ( not info)
            return;

        if (landTime - SimLibElapsedTime < FINAL_TIME)
        {
            info->status = lOnFinal;
        }
        else if (cosAngle < 0.0F)
        {
            FindBasePt(approaching, info->rwindex, finalX, finalY, &baseX, &baseY);
            info->status = FindFirstLegPt(approaching, info->rwindex, info->schedTime, baseX, baseY, TRUE, &x, &y);
        }
        else
        {
            info->status = FindFirstLegPt(approaching, info->rwindex, info->schedTime, finalX, finalY, FALSE, &x, &y);
        }

        SendCmdMessage(approaching, info);
        info->lastContacted = SimLibElapsedTime;
    }
    else
    {
        AircraftClass *aircraft = NULL;
        Flight flight = (Flight)approaching->GetCampaignObject();

        // protect against no components
        if ( not flight->GetComponents())
            return;

        VuListIterator flightIter(flight->GetComponents());
        aircraft = (AircraftClass*) flightIter.GetFirst();

        while (aircraft)
        {
            if ( not aircraft->OnGround())
            {
                RemoveFromAllATCs(aircraft);

                cosAngle = DetermineAngle(aircraft, rwindex, lHolding);
                CalculateMinMaxTime(aircraft, rwindex, lHolding, &min, &max, cosAngle);
                landTime = GetNextAvailRunwayTime(queue, SimLibElapsedTime + min, LAND_TIME_DELTA);

                info = AddTraffic(aircraft->Id(), lHolding, rwindex, landTime);

                if ( not info)
                    return;

                if (landTime - SimLibElapsedTime < FINAL_TIME)
                {
                    info->status = lOnFinal;
                }
                else if (cosAngle < 0.0F)
                {
                    FindBasePt(aircraft, info->rwindex, finalX, finalY, &baseX, &baseY);
                    info->status = FindFirstLegPt(aircraft, info->rwindex, info->schedTime, baseX, baseY, TRUE, &x, &y);
                }
                else
                {
                    info->status = FindFirstLegPt(aircraft, info->rwindex, info->schedTime, finalX, finalY, FALSE, &x, &y);
                }

                SendCmdMessage(aircraft, info);
                info->lastContacted = SimLibElapsedTime;
            }

            aircraft = (AircraftClass*) flightIter.GetNext();
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ATCBrain::RequestEmerClearance(AircraftClass* approaching)
{
    int queue, rwindex;
    runwayQueueStruct *info;
    float cosAngle;
    ulong min, max, landTime;
    AtcStatusEnum status;
    FalconRadioChatterMessage *radioMessage = NULL;

    if ( not approaching or not self or approaching->OnGround())
        return;

    if (GetTTRelations(self->GetTeam(), approaching->GetTeam()) >= Hostile)
        return;

    RemoveFromAllOtherATCs(approaching);

    info = InList(approaching->Id());

    if (info)
    {
        if (info->status >= lEmergencyToBase and info->status <= lEmergencyOnFinal)
        {
            //send confirmation message
            radioMessage = CreateCallFromATC(self, approaching, rcCLEAREDEMERGLAND, FalconLocalGame);
            //M.N. changed to 32767 -> flexibly use randomized value of max available eval indexes
            radioMessage->dataBlock.edata[3] = 32767;

            radioMessage->dataBlock.edata[4] = (short)GetRunwayName(GetOppositeRunway(info->rwindex));
            //M.N. changed to 32767 -> flexibly use randomized value of max available eval indexes
            radioMessage->dataBlock.edata[5] = 32767;
            FalconSendMessage(radioMessage, FALSE);

            //MI inform ground
            if ((rand() % 100) < 75)
            {
                radioMessage = CreateCallFromATC(self, approaching, rcACCIDENT, FalconLocalGame);
                //Get some delay till this get's played. 8 Seconds is our clearance to land,
                //and we want another 3 to 7 seconds delay.
                radioMessage->dataBlock.time_to_play = ((rand() % 5) + 11) * CampaignSeconds;
                FalconSendMessage(radioMessage, TRUE);
            }

            return;
        }
        else
        {
            RemoveTraffic(info->aircraftID, PtHeaderDataTable[info->rwindex].runwayNum);
        }
    }

    rwindex = FindBestLandingRunway(approaching, TRUE);
    queue = GetQueue(rwindex);

    if ( not rwindex)
    {
        //all runways destroyed, divert 'em
        FalconATCCmdMessage* ATCCmdMessage = new FalconATCCmdMessage(approaching->Id(), FalconLocalGame);
        ATCCmdMessage->dataBlock.from = self->Id();
        ATCCmdMessage->dataBlock.type = FalconATCCmdMessage::Divert;
        ATCCmdMessage->dataBlock.rwindex = 0;
        FalconSendMessage(ATCCmdMessage, FALSE); // Send it
        return;
    }

    cosAngle = DetermineAngle(approaching, rwindex, lHolding);
    CalculateMinMaxTime(approaching, rwindex, lHolding, &min, &max, cosAngle);
    info = runwayQueue[queue];

    landTime = SimLibElapsedTime + min;

    //multiple emergencies, you have to wait your turn
    if (runwayStats[queue].nextEmergency not_eq FalconNullId)
    {
        info = InList(runwayStats[queue].nextEmergency);

        while (info)
        {
            if (info->status >= lEmergencyToBase and info->status <= lEmergencyOnFinal and landTime + EMER_SLOT * 2 < info->schedTime)
                break;

            if (info->status >= lEmergencyToBase and info->status <= lEmergencyOnFinal and landTime < info->schedTime + EMER_SLOT * 2)
                landTime = info->schedTime + EMER_SLOT * 2;

            info = info->next;
        }
    }

    if (cosAngle < 0.0F)
        status = lEmergencyToBase;
    else
        status = lEmergencyToFinal;

    info = AddTraffic(approaching->Id(), status, rwindex, landTime);

    if ( not info)
        return;

    SendCmdMessage(approaching, info);

    radioMessage = CreateCallFromATC(self, approaching, rcCLEAREDEMERGLAND, FalconLocalGame);
    //M.N. changed to 32767 -> flexibly use randomized value of max available eval indexes
    radioMessage->dataBlock.edata[3] = 32767;

    radioMessage->dataBlock.edata[4] = (short)GetRunwayName(GetOppositeRunway(info->rwindex));
    radioMessage->dataBlock.edata[5] = 32767;
    FalconSendMessage(radioMessage, FALSE);
    //need to tell all that are in the way to hold, abort, etc...
    //send emergencyhold and emergencystop messages to all planes in list for specified runway
    FindNextEmergency(queue);

    SetEmergency(queue);
}

/*--------------------*/
/* Get a takeoff slot */
/*--------------------*/
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ATCBrain::RequestTakeoff(AircraftClass* departing)
{
    runwayQueueStruct *info = NULL;
    runwayQueueStruct *nextTakeoff = NULL;

    if ( not departing or not self or not departing->OnGround())
        return;

    if (GetTTRelations(self->GetTeam(), departing->GetTeam()) >= Hostile)
        return;

    if (departing->DBrain()->isWing)
    {
        RequestTaxi(departing);
    }
    else
    {
        ulong takeoffTime = 0, time;
        int queue = 0, rwindex = 0;
        AircraftClass *aircraft = NULL;
        FalconRadioChatterMessage *radioMessage = NULL;
        Flight flight = (Flight)departing->GetCampaignObject();

        info = InList(departing->Id());

        if (info)
        {
            if (info->schedTime + 60 * CampaignSeconds > SimLibElapsedTime or //RAS - Change to 60sec prior to takeoff???
                (info->status == tTakeoff))
            {
                int takeoffNum;

                switch (info->status)
                {
                    case tTakeoff:
                        if (info->schedTime + 120 * CampaignSeconds < SimLibElapsedTime)  // 06FEB04 - FRB - was 60 seconds
                        {
                            radioMessage = CreateCallFromATC(self, departing, rcEXPEDITEDEPARTURE, FalconLocalGame);

                            if (runwayQueue[queue]->status == lOnFinal)
                                radioMessage->dataBlock.edata[3] = 1;
                            else
                                radioMessage->dataBlock.edata[3] = 0;

                            FalconSendMessage(radioMessage, FALSE);
                        }
                        else
                        {
                            radioMessage = CreateCallFromATC(self, departing, rcCLEAREDONRUNWAY, FalconLocalGame);
                            radioMessage->dataBlock.edata[3] = (short)GetRunwayName(info->rwindex);
                            FalconSendMessage(radioMessage, FALSE);
                        }

                        break;

                    case tTakeRunway:
                        radioMessage = CreateCallFromATC(self, departing, rcPOSITIONANDHOLD, FalconLocalSession);
                        radioMessage->dataBlock.edata[3] = (short)GetRunwayName(info->rwindex);
                        FalconSendMessage(radioMessage, FALSE);
                        break;

                    case tWait:
                    case tHoldShort:
                    case tPrepToTakeRunway:
                        radioMessage = CreateCallFromATC(self, departing, rcHOLDSHORT, FalconLocalGame);
                        radioMessage->dataBlock.edata[3] = 0;

                        if (info->prev and info->prev->status < lCrashed)
                            radioMessage->dataBlock.edata[3] = (short)(1 + (rand() % 4));

                        FalconSendMessage(radioMessage, FALSE);
                        break;

                    case tTaxi:
                        takeoffNum = GetTakeoffNumber(info);

                        if (takeoffNum)
                        {
                            radioMessage = CreateCallFromATC(self, departing, rcTAXISEQUENCE, FalconLocalGame);
                            radioMessage->dataBlock.edata[3] = (short)(takeoffNum - 1);
                            FalconSendMessage(radioMessage, FALSE);
                        }
                        else
                        {
                            radioMessage = CreateCallFromATC(self, departing, rcCLEARTOTAXI, FalconLocalGame);
                            radioMessage->dataBlock.edata[2] = (short)GetRunwayName(info->rwindex);
                            FalconSendMessage(radioMessage, FALSE);
                        }

                        break;

                    case tEmerStop:
                        break;

                    case tReqTakeoff:
                    case tReqTaxi:
                        break;
                }

                return;
            }
        }

        info = InList(flight->Id());

        if (info)
        {
            rwindex = info->rwindex;
            queue = GetQueue(rwindex);
            takeoffTime = info->schedTime;
        }

        //make sure we remove any placeholders even if we aren't legitimately requesting a takeoff
        RemovePlaceHolders(flight->Id());



        // protect against no components
        if ( not flight->GetComponents())
            return;

        {
            // destroy iterator here
            VuListIterator flightIter(flight->GetComponents());
            aircraft = (AircraftClass*) flightIter.GetFirst();

            while (aircraft)
            {
                RemoveFromAllATCs(aircraft);
                aircraft = (AircraftClass*) flightIter.GetNext();
            }
        }

        if ( not rwindex)
        {
            rwindex = FindBestTakeoffRunway(TRUE);
            queue = GetQueue(rwindex);
        }

        if ( not takeoffTime)
            takeoffTime = FindFlightTakeoffTime(flight, queue);

        VuListIterator flightIter(flight->GetComponents());
        aircraft = (AircraftClass*) flightIter.GetFirst();

        while (aircraft)
        {
            if (aircraft->OnGround())
            {
                if ( not FindBestTakeoffRunway(TRUE))
                {
                    if (aircraft->IsPlayer() or aircraft->vehicleInUnit == 0)
                    {
                        //all runways are currently destroyed
                        radioMessage = CreateCallFromATC(self, departing, rcATCCANCELMISSION, FalconLocalGame);
                        radioMessage->dataBlock.edata[3] = 32767;
                        FalconSendMessage(radioMessage, FALSE);
                    }

                    if (rwindex)
                    {
                        runwayQueueStruct *info = AddTraffic(aircraft->Id(), tTaxiBack, rwindex, SimLibElapsedTime);

                        if (info)
                            SendCmdMessage(aircraft, info);
                    }

                    aircraft = (AircraftClass*) flightIter.GetNext();
                    continue;
                }

                if (UseSectionTakeoff((Flight)aircraft->GetCampaignObject(), rwindex))
                {
                    if (aircraft->vehicleInUnit > 1)
                        // time = takeoffTime + SLOT_TIME * 2;
                        time = takeoffTime + SLOT_TIME;  // 30JAN04 - FRB
                    else
                        time = takeoffTime;
                }
                else
                {
                    time = takeoffTime + SLOT_TIME * aircraft->vehicleInUnit;
                }

                runwayQueueStruct *info = AddTraffic(aircraft->Id(), tTaxi, rwindex, time);

                if ( not info)
                {
                    aircraft = (AircraftClass*) flightIter.GetNext();
                    continue;
                }

                if (IsOnRunway(aircraft))
                {
                    nextTakeoff = NextToTakeoff(GetQueue(rwindex));

                    if (nextTakeoff == info or
                        (UseSectionTakeoff((Flight)aircraft->GetCampaignObject(), rwindex) and aircraft->DBrain()->IsMyWingman(nextTakeoff->aircraftID)))
                        info->status = tTakeoff;
                }
                else if (info->schedTime > SimLibElapsedTime + 30 * CampaignSeconds)
                {
                    nextTakeoff = NextToTakeoff(GetQueue(rwindex));

                    if (nextTakeoff == info)
                        info->status = tHoldShort;
                }

                if (runwayStats[queue].nextEmergency not_eq FalconNullId)
                    info->status = tEmerStop;

                SendCmdMessage(aircraft, info);

                if (aircraft->IsPlayer() or aircraft->vehicleInUnit == 0)
                {
                    if (info->status == tTaxi)
                    {
                        radioMessage = CreateCallFromATC(self, aircraft, rcCLEARTOTAXI, FalconLocalGame);
                        radioMessage->dataBlock.edata[2] = (short)GetRunwayName(rwindex);
                        radioMessage->dataBlock.time_to_play = 2 * CampaignSeconds;
                        FalconSendMessage(radioMessage, FALSE);
                    }
                    else if (info->status == tHoldShort)
                    {
                        //Cobra Holdshort is often first call and doesn't give the runway to taxi to
                        radioMessage = CreateCallFromATC(self, departing, rcCLEARTOTAXI, FalconLocalGame);
                        radioMessage->dataBlock.edata[2] = (short)GetRunwayName(info->rwindex);
                        FalconSendMessage(radioMessage, FALSE);

                        radioMessage = CreateCallFromATC(self, aircraft, rcHOLDSHORT, FalconLocalGame);
                        radioMessage->dataBlock.edata[3] = 0;

                        if (info->prev and info->prev->status < lCrashed)
                            radioMessage->dataBlock.edata[3] = (short)(1 + (rand() % 4));

                        radioMessage->dataBlock.time_to_play = 2 * CampaignSeconds;
                        FalconSendMessage(radioMessage, FALSE);
                    }
                }
            }

            aircraft = (AircraftClass*) flightIter.GetNext();
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ATCBrain::RequestTaxi(AircraftClass* departing)
{
    int queue = 0, rwindex = 0;
    VU_TIME takeoffTime = 0;
    Flight flight;
    runwayQueueStruct *info = NULL;
    runwayQueueStruct *nextTakeoff = NULL;
    FalconRadioChatterMessage *radioMessage = NULL;

    if ( not departing or not self or not departing->OnGround())
        return;

    if (GetTTRelations(self->GetTeam(), departing->GetTeam()) >= Hostile)
        return;

    info = InList(departing->Id());

    if (info)
    {
        if (info->schedTime + 60 * CampaignSeconds > SimLibElapsedTime or
            (info->status == tTakeoff))
        {
            int takeoffNum;

            switch (info->status)
            {
                case tTakeoff:
                    if (info->schedTime + 120 * CampaignSeconds < SimLibElapsedTime)  // 06FEB04 - FRB - was 60 seconds
                    {
                        radioMessage = CreateCallFromATC(self, departing, rcEXPEDITEDEPARTURE, FalconLocalGame);

                        //if(runwayQueue[queue]->status == lOnFinal)
                        if ( not F4IsBadReadPtr(*runwayQueue, sizeof(runwayQueueStruct)) and runwayQueue[queue]->status == lOnFinal) // JB 010326 CTD
                            radioMessage->dataBlock.edata[3] = 1;
                        else
                            radioMessage->dataBlock.edata[3] = 0;

                        FalconSendMessage(radioMessage, FALSE);
                    }
                    else
                    {
                        radioMessage = CreateCallFromATC(self, departing, rcCLEAREDONRUNWAY, FalconLocalGame);
                        radioMessage->dataBlock.edata[3] = (short)GetRunwayName(info->rwindex);
                        FalconSendMessage(radioMessage, FALSE);
                    }

                    break;

                case tTakeRunway:
                    radioMessage = CreateCallFromATC(self, departing, rcPOSITIONANDHOLD, FalconLocalSession);
                    radioMessage->dataBlock.edata[3] = (short)GetRunwayName(info->rwindex);
                    FalconSendMessage(radioMessage, FALSE);
                    break;

                case tWait:
                case tHoldShort:
                case tPrepToTakeRunway:
                    //Cobra Holdshort is often first call and doesn't give the runway to taxi to
                    radioMessage = CreateCallFromATC(self, departing, rcCLEARTOTAXI, FalconLocalGame);
                    radioMessage->dataBlock.edata[2] = (short)GetRunwayName(info->rwindex);
                    FalconSendMessage(radioMessage, FALSE);

                    radioMessage = CreateCallFromATC(self, departing, rcHOLDSHORT, FalconLocalGame);
                    radioMessage->dataBlock.edata[3] = 0;

                    if (info->prev and info->prev->status < lCrashed)
                        radioMessage->dataBlock.edata[3] = (short)(1 + (rand() % 4));

                    FalconSendMessage(radioMessage, FALSE);
                    break;

                case tTaxi:
                    takeoffNum = GetTakeoffNumber(info);

                    if (takeoffNum)
                    {
                        //Cobra Holdshort is often first call and doesn't give the runway to taxi to
                        radioMessage = CreateCallFromATC(self, departing, rcCLEARTOTAXI, FalconLocalGame);
                        radioMessage->dataBlock.edata[2] = (short)GetRunwayName(info->rwindex);
                        FalconSendMessage(radioMessage, FALSE);

                        radioMessage = CreateCallFromATC(self, departing, rcTAXISEQUENCE, FalconLocalGame);
                        radioMessage->dataBlock.edata[3] = (short)(takeoffNum - 1);
                        FalconSendMessage(radioMessage, FALSE);
                    }
                    else
                    {
                        radioMessage = CreateCallFromATC(self, departing, rcCLEARTOTAXI, FalconLocalGame);
                        radioMessage->dataBlock.edata[2] = (short)GetRunwayName(info->rwindex);
                        FalconSendMessage(radioMessage, FALSE);
                    }

                    // sfr: ATC test, changed to TRUE
                    //FalconSendMessage(radioMessage, FALSE);
                    break;

                case tEmerStop:
                    break;

                case tReqTakeoff:
                case tReqTaxi:
                    break;
            }

            return;
        }
    }

    RemoveFromAllATCs(departing);

    flight = (Flight)departing->GetCampaignObject();

    info = InList(flight->Id());

    if (info)
    {
        rwindex = info->rwindex;
        queue = GetQueue(rwindex);
        takeoffTime = info->schedTime;
    }

    //make sure we remove any placeholders even if we aren't legitimately requesting a takeoff
    RemovePlaceHolders(flight->Id());

    if ( not rwindex)
    {
        rwindex = FindBestTakeoffRunway(TRUE);
        queue = GetQueue(rwindex);
    }

    if (rwindex)
    {
        if ( not takeoffTime)
        {
            // takeoffTime = GetNextAvailRunwayTime(queue, flight->GetCurrentUnitWP()->GetWPDepartureTime(), TAKEOFF_TIME_DELTA*2);
            takeoffTime = GetNextAvailRunwayTime(queue, flight->GetCurrentUnitWP()->GetWPDepartureTime(), TAKEOFF_TIME_DELTA);   // 27JAN04 - FRB - Bunch flight TO's closer together
        }
        else
        {
            // takeoffTime = takeoffTime + TAKEOFF_TIME_DELTA * departing->vehicleInUnit;
            takeoffTime = takeoffTime + (TAKEOFF_TIME_DELTA / 2) * departing->vehicleInUnit;  // 27JAN04 - FRB - Bunch flight TO's closer together
        }

        info = AddTraffic(departing->Id(), tTaxi, rwindex, takeoffTime);

        if ( not info)
            return;

        if (IsOnRunway(departing))
        {
            nextTakeoff = NextToTakeoff(GetQueue(rwindex));

            if (nextTakeoff == info or
                (UseSectionTakeoff((Flight)departing->GetCampaignObject(), rwindex) and departing->DBrain()->IsMyWingman(nextTakeoff->aircraftID)))
                info->status = tTakeoff;
        }
        else if (info->schedTime > SimLibElapsedTime + 30 * CampaignSeconds)
        {
            nextTakeoff = NextToTakeoff(GetQueue(rwindex));

            if (nextTakeoff == info)
                info->status = tHoldShort;
        }

        if (runwayStats[queue].nextEmergency not_eq FalconNullId)
            info->status = tEmerStop;

        SendCmdMessage(departing, info);

        if (departing->IsPlayer() or departing->vehicleInUnit == 0)
        {
            if (info->status == tTaxi)
            {
                radioMessage = CreateCallFromATC(self, departing, rcCLEARTOTAXI, FalconLocalGame);
                radioMessage->dataBlock.edata[2] = (short)GetRunwayName(rwindex);
                radioMessage->dataBlock.time_to_play = 2 * CampaignSeconds;
                FalconSendMessage(radioMessage, FALSE);
            }
            else if (info->status == tHoldShort)
            {
                radioMessage = CreateCallFromATC(self, departing, rcHOLDSHORT, FalconLocalGame);
                radioMessage->dataBlock.edata[3] = 0;

                if (info->prev and info->prev->status < lCrashed)
                    radioMessage->dataBlock.edata[3] = (short)(1 + (rand() % 4));

                radioMessage->dataBlock.time_to_play = 2 * CampaignSeconds;
                FalconSendMessage(radioMessage, FALSE);
            }
        }
    }
    else
    {
        //all runways are currently destroyed
        radioMessage = CreateCallFromATC(self, departing, rcATCCANCELMISSION, FalconLocalGame);
        //M.N. changed to 32767 -> flexibly use randomized value of max available eval indexes
        radioMessage->dataBlock.edata[3] = 32767;
        radioMessage->dataBlock.time_to_play = 2 * CampaignSeconds;
        FalconSendMessage(radioMessage, FALSE);
    }

}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ATCBrain::AbortApproach(AircraftClass* approaching)
{
    runwayQueueStruct *info = NULL;
    FalconRadioChatterMessage *radioMessage = NULL;

    info = InList(approaching->Id());

    if (info)
    {
        if (info->rwindex)
            RemoveTraffic(approaching->Id(), PtHeaderDataTable[info->rwindex].runwayNum);
        else
            RemoveInbound(info);
    }

    approaching->DBrain()->SetATCStatus(noATC);
    //Cobra
    radioMessage = CreateCallFromATC(self, approaching, rcRESUMEOWNNAV, FalconLocalSession);
    FalconSendMessage(radioMessage, FALSE);

    /* radioMessage = CreateCallFromATC (self, , rcROGER, FalconLocalGame);
     radioMessage->dataBlock.edata[0] = -1;
     radioMessage->dataBlock.edata[1] = 1; // just "Roger"
     radioMessage->dataBlock.time_to_play= 2*CampaignSeconds;
     FalconSendMessage(radioMessage, FALSE);
    */
}



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ATCBrain::FindNextEmergency(int queue)
{
    runwayQueueStruct *info = runwayQueue[queue];

    runwayStats[queue].nextEmergency = FalconNullId;

    while (info and runwayStats[queue].nextEmergency == FalconNullId)
    {
        if (info->status >= lEmergencyToBase and info->status <= lCrashed)
        {
            runwayStats[queue].nextEmergency = info->aircraftID;
            break;
        }

        info = info->next;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ATCBrain::SetEmergency(int queue)
{
    AircraftClass *aircraft = NULL;
    runwayQueueStruct *info = NULL;
    runwayQueueStruct *list = NULL;
    runwayQueueStruct *listPtr = NULL;
    runwayQueueStruct *emer = NULL;

    emer = runwayQueue[queue];

    while (emer and emer->aircraftID not_eq runwayStats[queue].nextEmergency)
        emer = emer->next;

    // Check for AC in the queue. If its not, don't remove it, just return LRKLUDGE
    if ( not emer)
        return;

    info = emer->prev;

    while (info and info->schedTime + EMER_SLOT > emer->schedTime)
    {
        runwayQueue[queue] = RemoveFromList(runwayQueue[queue], info);
        list = AddToList(list, info);
        info = emer->prev;
    }

    listPtr = emer->next;

    while (listPtr and (listPtr->prev->status < lEmergencyToBase or listPtr->prev->status > lEmergencyOnFinal))
    {
        info = listPtr;
        listPtr = listPtr->next;
        runwayQueue[queue] = RemoveFromList(runwayQueue[queue], info);
        list = AddToList(list, info);
    }

    listPtr = list;

    while (listPtr)
    {
        info = InList(listPtr->aircraftID);

        if (info)
        {
            info = listPtr;
            listPtr = listPtr->next;
            list = RemoveFromList(list, info);
            continue;
        }

        aircraft = (AircraftClass*)vuDatabase->Find(listPtr->aircraftID);

        if (listPtr->status >= tReqTaxi)
            RequestTakeoff(aircraft);
        else if (listPtr->status > noATC)
            RequestClearance(aircraft, FALSE);

        info = listPtr;
        listPtr = listPtr->next;
        list = RemoveFromList(list, info);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ATCBrain::ReschedulePlanes(int queue)
{
    AircraftClass *aircraft = NULL;
    runwayQueueStruct *info = NULL;
    runwayQueueStruct *deleteInfo = NULL;
    FalconRadioChatterMessage *radioMessage = NULL;

    deleteInfo = info = runwayQueue[queue];

    while (info)
    {
        aircraft = (AircraftClass*)vuDatabase->Find(info->aircraftID);

        if (aircraft and info->status not_eq noATC)
        {
            if (info->status > lCrashed)
            {
                info->status = tTaxiBack;
                SendCmdMessage(aircraft, info);
                info = info->next;
                RemoveTraffic(deleteInfo->aircraftID, queue);

                if ( not aircraft->DBrain()->isWing)
                {
                    radioMessage = CreateCallFromATC(self, aircraft, rcATCCANCELMISSION, FalconLocalGame);
                    radioMessage->dataBlock.edata[3] = 32767;
                    FalconSendMessage(radioMessage, FALSE);
                }
            }
            else
            {
                info = info->next;
                RemoveTraffic(deleteInfo->aircraftID, queue);
                RequestClearance(aircraft);
            }
        }
        else
        {
            info = info->next;
            RemoveTraffic(deleteInfo->aircraftID, queue);
        }

        deleteInfo = info;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ATCBrain::RescheduleFlightTakeoff(int queue, Flight flight)
{
    AircraftClass *aircraft = NULL;
    runwayQueueStruct *info = NULL;
    runwayQueueStruct *deleteInfo = NULL;
    int rwindex = 0;

    deleteInfo = info = runwayQueue[queue];

    while (info)
    {
        aircraft = (AircraftClass*)vuDatabase->Find(info->aircraftID);
        info = info->next;

        if (aircraft and aircraft->GetCampaignObject() == flight and aircraft->OnGround())
        {
            rwindex = info->rwindex;
            RemoveTraffic(deleteInfo->aircraftID, queue);
        }

        deleteInfo = info;
    }

    if ( not rwindex)
    {
        rwindex = FindBestTakeoffRunway(TRUE);
        queue = GetQueue(rwindex);
    }

    ulong takeoffTime = FindFlightTakeoffTime(flight, queue);

    VuListIterator flightIter(flight->GetComponents());
    aircraft = (AircraftClass*) flightIter.GetFirst();

    while (aircraft)
    {
        if (aircraft->OnGround())
        {
            if (UseSectionTakeoff((Flight)aircraft->GetCampaignObject(), rwindex))
            {
                if (aircraft->vehicleInUnit == 2)
                    // takeoffTime += SLOT_TIME * 2;
                    takeoffTime += SLOT_TIME;  // 30JAN04 - FRB
            }
            else if (aircraft->vehicleInUnit)
            {
                takeoffTime += SLOT_TIME;
            }

            info = AddTraffic(aircraft->Id(), tTaxi, rwindex, takeoffTime);

            if ( not info)
            {
                aircraft = (AircraftClass*) flightIter.GetNext();
                continue;
            }

            if (runwayStats[queue].nextEmergency not_eq FalconNullId)
                info->status = tEmerStop;

            SendCmdMessage(aircraft, info);
        }

        aircraft = (AircraftClass*) flightIter.GetNext();
    }

}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ATCBrain::ReorderFlight(int queue, Flight flight, AtcStatusEnum status)
{
    AircraftClass *aircraft = NULL;
    runwayQueueStruct *info = NULL;
    FalconRadioChatterMessage *radioMessage = NULL;

    info = runwayQueue[queue];

    while (info)
    {
        aircraft = (AircraftClass*)vuDatabase->Find(info->aircraftID);

        if (aircraft and aircraft->GetCampaignObject() == flight and aircraft->OnGround())
        {
            info->status = status;

            if (status == tHoldShort)
            {
                status = tTaxi;
                radioMessage = CreateCallFromATC(self, aircraft, rcHOLDSHORT, FalconLocalGame);
                radioMessage->dataBlock.edata[3] = 0;
                FalconSendMessage(radioMessage, FALSE);
            }

            SendCmdMessage(aircraft, info);
        }

        info = info->next;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ATCBrain::FindBestTakeoffRunway(int checklist)
{
    int i, j;
    int delta, best = 91, windheading = 0;
    runwayQueueStruct *info = NULL;

    // Find windheading in degrees (add 180 if we want opposite direction)
    if (realWeather)
        windheading = FloatToInt32(((WeatherClass*)realWeather)->windHeading * RTD + 180.5f);

    while (windheading > 360)
        windheading -= 360;

    int rwindex = 0;

    // FRB - hack
    if (numRwys > 4)
        numRwys = 1;

    for (i = 0; i < numRwys; i++)
    {
        //if(runwayStats[i].state < VIS_DAMAGED and runwayStats[i].rwIndexes[1])
        // FRB - CTDs
        if (runwayStats[i].state < VIS_DAMAGED)
        {
            for (j = 0; runwayStats[i].rwIndexes[j] and j < 2; j++)
            {
                delta = abs(PtHeaderDataTable[runwayStats[i].rwIndexes[j]].data - windheading);

                if (delta > 180)
                    delta = 360 - delta;  // Cobra - simplify angle adjustment

                // delta = abs(PtHeaderDataTable[runwayStats[i].rwIndexes[j]].data - 360 + windheading);
                if (delta < best)
                {
                    best = delta;
                    rwindex = runwayStats[i].rwIndexes[j];
                }
            }
        }
    }

    if ( not checklist)
        return rwindex;

    info = runwayQueue[PtHeaderDataTable[rwindex].runwayNum];

    while (info and info->status == noATC)
        info = info->next;

    if (info)
    {
        if (info->status >= tReqTaxi)
            rwindex = info->rwindex;
        else if (GetOppositeRunway(info->rwindex))
            rwindex = GetOppositeRunway(info->rwindex);
    }
    else if (numRwys == 2)
    {
        info = runwayQueue[1 - PtHeaderDataTable[rwindex].runwayNum];

        while (info and info->status == noATC)
            info = info->next;

        if (info)
        {
            delta = abs(PtHeaderDataTable[rwindex].data - PtHeaderDataTable[info->rwindex].data);

            if (delta > 180)
                delta = abs(delta - 360);

            if (delta < 91)
            {
                if (info->status < tReqTaxi and info->status not_eq noATC and GetOppositeRunway(rwindex))
                    rwindex = GetOppositeRunway(rwindex);
            }
            else
            {
                if (info->status >= tReqTaxi and GetOppositeRunway(rwindex))
                    rwindex = GetOppositeRunway(rwindex);
            }
        }
    }

    return rwindex;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ATCBrain::CalculateMinMaxTime(AircraftClass* aircraft, int rwindex, AtcStatusEnum status, CampaignTime *min, CampaignTime *max, float cosAngle)
{
    float finalX, finalY, baseX, baseY, px, py, dist, dx, dy, finalDist;
    float decelDist, decelTime, finAngle, norm;
    float PatternSpd = aircraft->af->MinVcas() * KNOTS_TO_FTPSEC;

    if (aircraft->IsLocal())
    {
        float minSpeed = aircraft->af->CalcDesSpeed(10.0F);

        if (minSpeed > PatternSpd)
            PatternSpd = minSpeed;
    }

    //cosAngle = DetermineAngle(aircraft, rwindex, status);
    decelTime = (float)fabs(aircraft->af->vt - PatternSpd) * 0.2F;
    decelDist = decelTime * (aircraft->af->vt + PatternSpd) * 0.5F;

    FindFinalPt(aircraft, rwindex, &finalX, &finalY);

    finalDist = 0.8F * PatternSpd * 120.0F;

    switch (status)
    {
        case lReqClearance:
        case lReqEmerClearance:
            TranslatePointData(self, GetFirstPt(rwindex), &px, &py);

            //this is declared backwards because the runway heading is opposite the
            //heading we need to have when landing
            dx = aircraft->XPos() - px;
            dy = aircraft->YPos() - py;
            dist = (float)sqrt(dx * dx + dy * dy);

            if (dist < finalDist)
            {
                norm = (float)(1.0F / dist);
                dx *= norm;
                dy *= norm;

                finAngle = dx * PtHeaderDataTable[rwindex].cosHeading +
                            dy * PtHeaderDataTable[rwindex].sinHeading;

                if (finAngle > 0.707F)
                {
                    *max = FloatToInt32(dist / (PatternSpd * 0.65F)) * CampaignSeconds;
                    PatternSpd = PatternSpd * (dist * 0.4F / finalDist + 0.6F);
                    *min = FloatToInt32(dist / (PatternSpd * 0.7F)) * CampaignSeconds;
                    break;
                }
            }

        case noATC:
        case lHolding:
        case lFirstLeg:
        case lAborted:
        case lEmerHold:
        case lIngressing:
        case lTakingPosition:

            if (cosAngle < 0.0F)
            {
                //we need to use a base pt
                FindBasePt(aircraft, rwindex, finalX, finalY, &baseX, &baseY);

                dx = baseX - aircraft->XPos();
                dy = baseY - aircraft->YPos();
                dist = (float)sqrt(dx * dx + dy * dy);

                if (dist > decelDist)
                {
                    *min = FloatToInt32((decelTime + (dist - decelDist) / PatternSpd)) * CampaignSeconds + FINAL_TIME + BASE_TIME;
                    *max = FloatToInt32(1.4142F * (decelTime + (dist - decelDist) / PatternSpd)) * CampaignSeconds + FINAL_TIME + BASE_TIME;
                }
                else
                {
                    *min = FloatToInt32(decelTime) * CampaignSeconds + FINAL_TIME + BASE_TIME;

                    if (dist * 1.4142F > decelDist)
                        *max = FloatToInt32((decelTime + (1.4142F * dist - decelDist) / PatternSpd))  * CampaignSeconds + FINAL_TIME + BASE_TIME;
                    else
                        *max = FloatToInt32(decelTime * CampaignSeconds) + FINAL_TIME + BASE_TIME;
                }
            }
            else
            {
                //we can head directly for our final pt
                dx = finalX - aircraft->XPos();
                dy = finalY - aircraft->YPos();
                dist = (float)sqrt(dx * dx + dy * dy);

                if (dist > decelDist)
                {
                    *min = FloatToInt32((decelTime + (dist - decelDist) / PatternSpd)) * CampaignSeconds + FINAL_TIME;
                    *max = FloatToInt32(1.879385241572F * (decelTime + (dist - decelDist) / PatternSpd)) * CampaignSeconds + FINAL_TIME;
                }
                else
                {
                    *min = FloatToInt32(decelTime) * CampaignSeconds + FINAL_TIME;

                    if (dist *  1.879385241572F > decelDist)
                        *max = FloatToInt32((decelTime + (1.879385241572F * dist - decelDist) / PatternSpd)) * CampaignSeconds + FINAL_TIME;
                    else
                        *max = FloatToInt32(decelTime) * CampaignSeconds + FINAL_TIME;
                }
            }

            break;

        case lEmergencyToBase:
        case lToBase:
            //we need to use a base pt
            FindBasePt(aircraft, rwindex, finalX, finalY, &baseX, &baseY);

            dx = baseX - aircraft->XPos();
            dy = baseY - aircraft->YPos();
            dist = (float)sqrt(dx * dx + dy * dy);

            if (dist > decelDist)
            {
                *min = FloatToInt32((decelTime + (dist - decelDist) / PatternSpd)) * CampaignSeconds + FINAL_TIME + BASE_TIME;
                *max = FloatToInt32(1.4142F * (decelTime + (dist - decelDist) / PatternSpd))  * CampaignSeconds + FINAL_TIME + BASE_TIME;
            }
            else
            {
                *min = FloatToInt32(decelTime) * CampaignSeconds + FINAL_TIME + BASE_TIME;

                if (dist * 1.4142F > decelDist)
                    *max = FloatToInt32((decelTime + (1.4142F * dist - decelDist) / PatternSpd))  * CampaignSeconds + FINAL_TIME + BASE_TIME;
                else
                    *max = FloatToInt32(decelTime) * CampaignSeconds + FINAL_TIME + BASE_TIME;
            }

            break;

        case lEmergencyToFinal:
        case lToFinal:
            //we can head directly for our final pt
            dx = finalX - aircraft->XPos();
            dy = finalY - aircraft->YPos();
            dist = (float)sqrt(dx * dx + dy * dy);

            if (dist > decelDist)
            {
                *min = FloatToInt32((decelTime + (dist - decelDist) / PatternSpd)) * CampaignSeconds + FINAL_TIME;
                *max = FloatToInt32(1.4142F * (decelTime + (dist - decelDist) / PatternSpd)) * CampaignSeconds + FINAL_TIME;
            }
            else
            {
                *min = FloatToInt32(decelTime) * CampaignSeconds + FINAL_TIME;

                if (dist * 1.4142F > decelDist)
                    *max = FloatToInt32((decelTime + (1.4142F * dist - decelDist) / PatternSpd))  * CampaignSeconds + FINAL_TIME;
                else
                    *max = FloatToInt32(decelTime) * CampaignSeconds + FINAL_TIME;
            }

            break;

        case lLanded:
        case lEmergencyOnFinal:
        case lOnFinal:
        case lClearToLand:
            if (cosAngle > 0.707F)
            {
                TranslatePointData(self, GetFirstPt(rwindex), &px, &py);
                dx = px - aircraft->XPos();
                dy = py - aircraft->YPos();
                dist = (float)sqrt(dx * dx + dy * dy);

                *max = FloatToInt32(dist / (PatternSpd * 0.67F)) * CampaignSeconds;

                PatternSpd = PatternSpd * (dist * 0.4F / finalDist + 0.6F);

                *min = FloatToInt32(dist / PatternSpd) * CampaignSeconds;
            }
            else
            {
                *max = 0;
                *min = 180 * CampaignSeconds;
            }

            break;

        case lCrashed:
        case tReqTaxi:
        case tReqTakeoff:
        case tEmerStop:
        case tTaxi:
        case tWait:
        case tHoldShort:
        case tPrepToTakeRunway:
        case tTakeRunway:
        case tTakeoff:
        case tFlyOut:
        case tTaxiBack:
        case lTaxiOff:
            *max = 0;
            *min = 0;
            break;

        default:
            //we should never get here
            ShiWarning("We are in an undefined state, we shouldn't be here");
    }

}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ATCBrain::CalcRunwayDimensions(int index)
{
    int point;
    int queue = PtHeaderDataTable[index].runwayNum;
    float x1, y1, x2, y2, x3, y3;

    point = PtHeaderDataTable[index].first;
    TranslatePointData(self, point, &x1, &y1);

    point = GetNextPtLoop(point);
    TranslatePointData(self, point, &x2, &y2);

    point = GetNextPtLoop(point);
    TranslatePointData(self, point, &x3, &y3);

    runwayStats[queue].halfwidth = (float)sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2)) * 0.5F;
    runwayStats[queue].halfheight = (float)sqrt((x2 - x3) * (x2 - x3) + (y2 - y3) * (y2 - y3)) * 0.5F;
    runwayStats[queue].centerX = (x1 + x3) * 0.5F;
    runwayStats[queue].centerY = (y1 + y3) * 0.5F;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ATCBrain::IsOnRunway(float x, float y)
{
    float dx, dy, relx, rely;
    int i;

    int retval = 0;

    for (i = 0; i < numRwys; i++)
    {
        dx = runwayStats[i].centerX - x;
        dy = runwayStats[i].centerY - y;

        relx = (PtHeaderDataTable[runwayStats[i].rwIndexes[0]].cosHeading * dx + PtHeaderDataTable[runwayStats[i].rwIndexes[0]].sinHeading * dy);
        rely = (-PtHeaderDataTable[runwayStats[i].rwIndexes[0]].sinHeading * dx + PtHeaderDataTable[runwayStats[i].rwIndexes[0]].cosHeading * dy);

        if (fabs(relx) < runwayStats[i].halfheight and fabs(rely) < runwayStats[i].halfwidth)
            retval = runwayStats[i].rwIndexes[0];

    }

    return retval;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ATCBrain::IsOnRunway(AircraftClass* aircraft)
{
    if ( not aircraft->OnGround())
        return FALSE;

    return IsOverRunway(aircraft);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ATCBrain::IsOnRunway(int taxipoint)
{
    float tempX, tempY;
    TranslatePointData(self, taxipoint, &tempX, &tempY);
    return IsOnRunway(tempX, tempY);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ATCBrain::IsOverRunway(AircraftClass* aircraft)
{
    return IsOnRunway(aircraft->XPos(), aircraft->YPos());;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float ATCBrain::DetermineAngle(AircraftClass* aircraft, int rwindex, AtcStatusEnum status)
{
    float px = 0.0F, py = 0.0F, dx = 0.0F, dy = 0.0F, norm = 0.0F, cosAngle = 1.0F;

    switch (status)
    {
        case lReqClearance:
        case lReqEmerClearance:
        case lIngressing:
        case lTakingPosition:

        case lAborted:
        case lEmerHold:
        case lHolding:
        case lFirstLeg:
        case lToBase:
        case lToFinal:
        case lEmergencyToBase:
        case lEmergencyToFinal:

            // RED - CTD FIX
            if (aircraft) FindFinalPt(aircraft, rwindex, &px, &py);

            break;

        case lEmergencyOnFinal:
        case lOnFinal:
        case lClearToLand:
            TranslatePointData(self, GetFirstPt(rwindex), &px, &py);
            px -= 500.0F;
            py -= 500.0F;
            break;

        case noATC:
        case lCrashed:
        case lLanded:
        case lTaxiOff:
        case tReqTaxi:
        case tReqTakeoff:
        case tEmerStop:
        case tTaxi:
        case tWait:
        case tHoldShort:
        case tPrepToTakeRunway:
        case tTakeRunway:
        case tTakeoff:
        case tFlyOut:
        case tTaxiBack:
            TranslatePointData(self, GetNextPt(GetFirstPt(rwindex)), &px, &py);
            break;

        default:
            //we should never get here
            ShiWarning("We are in an undefined state, we shouldn't be here");
            break;
    }

    //this is declared backwards because the runway heading is opposite the
    //heading we need to have when landing
    if (aircraft)
    {
        dx = aircraft->XPos() - px;
        dy = aircraft->YPos() - py;
        norm = (float)(1.0F / sqrt(dx * dx + dy * dy));
        dx *= norm;
        dy *= norm;

        cosAngle = dx * PtHeaderDataTable[rwindex].cosHeading +
                    dy * PtHeaderDataTable[rwindex].sinHeading;
    }

    return cosAngle;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ulong ATCBrain::FindFlightTakeoffTime(FlightClass *flight, int queue)
{
    ulong takeoffTime, delta;
    runwayQueueStruct *cur = runwayQueue[queue];
    runwayQueueStruct *prev = NULL;
    ulong emerDelta = 0;

    //according to Kevin this should never happen
    ShiAssert(flight->GetCurrentUnitWP());

    if ( not flight or not flight->GetCurrentUnitWP())
    {
        if ( not cur)
            return SimLibElapsedTime;

        while (cur->next)
        {
            cur = cur->next;
        }

        return cur->schedTime + LAND_TIME_DELTA;
    }

    takeoffTime = flight->GetCurrentUnitWP()->GetWPDepartureTime();

    //RAS - takeoff time has passed so set it to right now
    if (takeoffTime < SimLibElapsedTime)
        takeoffTime = SimLibElapsedTime;

    if ( not cur)
        return takeoffTime;

    delta = flight->NumberOfComponents() * TAKEOFF_TIME_DELTA + LAND_TIME_DELTA;

    if (cur->aircraftID == runwayStats[queue].nextEmergency or (cur->status >= lEmergencyToBase and cur->status <= lEmergencyOnFinal))
        emerDelta = EMER_SLOT - LAND_TIME_DELTA;
    else
        emerDelta = 0;

    while (cur and (cur->schedTime <= takeoffTime or (takeoffTime < cur->schedTime and takeoffTime + delta + emerDelta > cur->schedTime)))
    {
        if (cur->aircraftID == runwayStats[queue].nextEmergency or (cur->status >= lEmergencyToBase and cur->status <= lEmergencyOnFinal))
            emerDelta = EMER_SLOT - LAND_TIME_DELTA;
        else
            emerDelta = 0;

        if (takeoffTime < cur->schedTime + LAND_TIME_DELTA + emerDelta)
            takeoffTime = cur->schedTime + LAND_TIME_DELTA + emerDelta;

        prev = cur;
        cur = cur->next;
    }

    cur = prev;

    while (cur)
    {
        if (cur->aircraftID == runwayStats[queue].nextEmergency or (cur->status >= lEmergencyToBase and cur->status <= lEmergencyOnFinal))
            emerDelta = EMER_SLOT - LAND_TIME_DELTA;
        else
            emerDelta = 0;

        if (takeoffTime < cur->schedTime + LAND_TIME_DELTA + emerDelta)
            takeoffTime = cur->schedTime + LAND_TIME_DELTA + emerDelta;

        if (cur->next and cur->schedTime + delta + emerDelta <= takeoffTime and takeoffTime + delta + emerDelta <= cur->next->schedTime)
        {
            ShiAssert(takeoffTime >= cur->schedTime + LAND_TIME_DELTA + emerDelta);
            ShiAssert(takeoffTime + delta + emerDelta <= cur->next->schedTime);
            break;
        }

        cur = cur->next;
    }

    return takeoffTime;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ulong ATCBrain::GetNextAvailRunwayTime(int queue, ulong rwTime, ulong delta)
{
    runwayQueueStruct *cur = runwayQueue[queue];
    runwayQueueStruct *prev = NULL;
    ulong tempDelta;

    if ( not cur)
    {
        //flights may ahve been given a time that is already past
        //when they deaggregate, but we don't want to give out ancient
        //takeoff times
        if (rwTime + 30 * CampaignSeconds < SimLibElapsedTime)
            rwTime = SimLibElapsedTime;

        return rwTime;
    }

    if (cur->aircraftID == runwayStats[queue].nextEmergency or (cur->status >= lEmergencyToBase and cur->status <= lEmergencyOnFinal))
        tempDelta = EMER_SLOT;
    else
        tempDelta = delta;

    while (cur and (cur->schedTime < rwTime or (rwTime < cur->schedTime and rwTime + tempDelta > cur->schedTime)))
    {
        if (cur->aircraftID == runwayStats[queue].nextEmergency or (cur->status >= lEmergencyToBase and cur->status <= lEmergencyOnFinal))
            tempDelta = EMER_SLOT;
        else
            tempDelta = delta;

        if (rwTime < cur->schedTime + tempDelta)
            rwTime = cur->schedTime + tempDelta;

        prev = cur;
        cur = cur->next;
    }

    cur = prev;

    while (cur)
    {
        if (cur->aircraftID == runwayStats[queue].nextEmergency or (cur->status >= lEmergencyToBase and cur->status <= lEmergencyOnFinal))
            tempDelta = EMER_SLOT;
        else
            tempDelta = delta;

        if (rwTime < cur->schedTime + tempDelta)
            rwTime = cur->schedTime + tempDelta;

        if (cur->next and cur->schedTime + tempDelta + LAND_TIME_DELTA <= cur->next->schedTime and rwTime + tempDelta < cur->next->schedTime)
        {
            ShiAssert(rwTime >= cur->schedTime + tempDelta);
            ShiAssert(rwTime + tempDelta <= cur->next->schedTime);
            break;
        }

        cur = cur->next;
    }

    return rwTime;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ATCBrain::FindBestLandingRunway(FalconEntity* landing, int checklist)
{

    // sfr: get relations between base and entity
    // only return if they are on the same team
    int rel = GetTTRelations(self->GetTeam(), landing->GetTeam());

    if ((rel not_eq Friendly) and (rel not_eq Allied))
    {
        return 0;
    }

    int i, j, ptindex;
    int delta, best = 91, sbest = 91, windheading, score1, score2;
    int rwindex = 0, queue = 0, rwindex2 = 0, queue2 = 0;
    float dist1, dist2, px, py, dx, dy;
    runwayQueueStruct* info;

    // Find windheading in degrees (add 180 if we want opposite direction)
    // Cobra - windHeading in radians
    windheading = FloatToInt32(((WeatherClass*)realWeather)->windHeading * RTD + 0.5f) % 360;

    for (i = 0; i < numRwys; i++)
    {
        if (runwayStats[i].state < VIS_DAMAGED)
        {
            for (j = 0; runwayStats[i].rwIndexes[j] and j < 2; j++)
            {
                delta = abs(PtHeaderDataTable[runwayStats[i].rwIndexes[j]].data - windheading);

                if (delta > 180)
                {
                    delta = 360 - delta;  // Cobra - simplify angle adjustment
                    // delta = abs(PtHeaderDataTable[runwayStats[i].rwIndexes[j]].data - 360 + windheading);
                }

                if (delta < best)
                {
                    sbest = best;
                    best = delta;
                    rwindex2 = rwindex;
                    rwindex = runwayStats[i].rwIndexes[j];
                    queue2 = queue;
                    queue = i;
                }
                else if (delta < sbest)
                {
                    sbest = delta;
                    rwindex2 = runwayStats[i].rwIndexes[j];
                    queue2 = i;
                }
            }
        }
    }

    //we prefer to use the runway with a shorter queue that is closer to us
    if (numRwys == 2 and rwindex2 and PtHeaderDataTable[rwindex2].runwayNum not_eq PtHeaderDataTable[rwindex].runwayNum)
    {
        //score both choices, while favoring the second choice so we don't have crossing patterns
        score1 = (runwayStats[queue2].numInQueue - runwayStats[queue].numInQueue);

        if (PtHeaderDataTable[rwindex].data == PtHeaderDataTable[rwindex2].data)
        {
            score2 = (runwayStats[queue].numInQueue - runwayStats[queue2].numInQueue);
        }
        else
        {
            score2 = (runwayStats[queue].numInQueue - runwayStats[queue2].numInQueue) + 7;
        }

        if (runwayStats[queue].nextEmergency not_eq FalconNullId)
        {
            score1 -= 6;
        }

        if (runwayStats[queue2].nextEmergency not_eq FalconNullId)
        {
            score2 -= 6;
        }

        //if we choose the closer runway we won't have to worry about our patterns crossing over the runway

        ptindex = GetFirstPt(rwindex);
        TranslatePointData(self, ptindex, &px, &py);

        dx = landing->XPos() - px;
        dy = landing->YPos() - py;

        dist1 = dx * dx + dy * dy;

        ptindex = GetFirstPt(rwindex2);
        TranslatePointData(self, ptindex, &px, &py);

        dx = landing->XPos() - px;
        dy = landing->YPos() - py;

        dist2 = dx * dx + dy * dy;

        if (dist1 > dist2)
        {
            score2 += 1;
        }
        else
        {
            score1 += 1;
        }

        if (score2 > score1)
        {
            rwindex = rwindex2;
            queue = queue2;
        }
    }

    if ( not checklist)
    {
        return rwindex;
    }

    info = runwayQueue[PtHeaderDataTable[rwindex].runwayNum];

    while (info and info->status == noATC)
    {
        info = info->next;
    }

    if (info)
    {
        if (info->status >= tReqTaxi and GetOppositeRunway(info->rwindex))
        {
            rwindex = GetOppositeRunway(info->rwindex);
        }
        else
        {
            rwindex = info->rwindex;
        }
    }
    else if (numRwys == 2)
    {
        info = runwayQueue[1 - PtHeaderDataTable[rwindex].runwayNum];

        if (info)
        {
            delta = abs(PtHeaderDataTable[rwindex].data - PtHeaderDataTable[info->rwindex].data);

            if (delta > 180)
            {
                delta = abs(delta - 360);
            }

            if (delta < 91)
            {
                if (info->status >= tReqTaxi and GetOppositeRunway(rwindex))
                {
                    rwindex = GetOppositeRunway(rwindex);
                }
            }
            else
            {
                if (info->status < tReqTaxi and GetOppositeRunway(rwindex))
                {
                    rwindex = GetOppositeRunway(rwindex);
                }
            }
        }
    }

    return rwindex;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ATCBrain::FindEmergencyLandingRunway(int *queue, int *rwindex, FalconEntity* landing)
{
    int i, j, ptindex;
    int delta, best = 91, sbest = 91, rwindex2, queue2, score1, score2;
    float dist1, dist2, px, py, dx, dy, headingfrom;
    runwayQueueStruct* info;

    headingfrom = (float)atan2(self->XPos() - landing->XPos(), self->YPos() - landing->YPos());

    *queue = 0;
    *rwindex = 0;
    queue2 = 0;
    rwindex2 = 0;

    for (i = 0; i < numRwys; i++)
    {
        if (runwayStats[i].state < VIS_DAMAGED)
        {
            for (j = 0; j < 2 and runwayStats[i].rwIndexes[j]; j++)
            {
                delta = abs(PtHeaderDataTable[runwayStats[i].rwIndexes[j]].data - FloatToInt32(headingfrom * RTD));

                if (delta > 180)
                    delta = abs(PtHeaderDataTable[runwayStats[i].rwIndexes[j]].data - 360 + FloatToInt32(headingfrom * RTD));

                if (delta < best)
                {
                    sbest = best;
                    best = delta;
                    rwindex2 = *rwindex;
                    *rwindex = runwayStats[i].rwIndexes[j];
                    queue2 = *queue;
                    *queue = i;
                }
                else if (delta < sbest)
                {
                    sbest = delta;
                    rwindex2 = runwayStats[i].rwIndexes[j];
                    queue2 = i;
                }
            }
        }
    }

    //we prefer to use the runway with a shorter queue that is closer to us
    if (rwindex2 and PtHeaderDataTable[rwindex2].runwayNum not_eq PtHeaderDataTable[*rwindex].runwayNum)
    {
        //score both choices, while favoring the second choice so we don't have crossing patterns
        score1 = (runwayStats[queue2].numInQueue - runwayStats[*queue].numInQueue);

        if (PtHeaderDataTable[*rwindex].data == PtHeaderDataTable[rwindex2].data)
            score2 = (runwayStats[*queue].numInQueue - runwayStats[queue2].numInQueue);
        else
            score2 = (runwayStats[*queue].numInQueue - runwayStats[queue2].numInQueue) + 7;

        if (runwayStats[*queue].nextEmergency not_eq FalconNullId)
            score1 -= 6;

        if (runwayStats[queue2].nextEmergency not_eq FalconNullId)
            score2 -= 6;

        //if we choose the closer runway we won't have to worry about our patterns crossing over the runway

        ptindex = GetFirstPt(*rwindex);
        TranslatePointData(self, ptindex, &px, &py);

        dx = landing->XPos() - px;
        dy = landing->YPos() - py;

        dist1 = dx * dx + dy * dy;

        ptindex = GetFirstPt(rwindex2);
        TranslatePointData(self, ptindex, &px, &py);

        dx = landing->XPos() - px;
        dy = landing->YPos() - py;

        dist2 = dx * dx + dy * dy;

        if (dist1 > dist2)
            score2 += 1;
        else
            score1 += 1;

        if (score2 > score1)
        {
            *rwindex = rwindex2;
            *queue = queue2;
        }
    }

    info = runwayQueue[PtHeaderDataTable[*rwindex].runwayNum];

    if (info)
    {
        if (info->status >= tReqTaxi)
            *rwindex = GetOppositeRunway(info->rwindex);
        else
            *rwindex = info->rwindex;
    }
    else if (numRwys == 2)
    {
        info = runwayQueue[1 - PtHeaderDataTable[*rwindex].runwayNum];

        if (info)
        {
            delta = abs(PtHeaderDataTable[*rwindex].data - PtHeaderDataTable[info->rwindex].data);

            if (delta > 180)
                delta = abs(delta - 360);

            if (delta < 91)
            {
                if (info->status >= tReqTaxi)
                    *rwindex = GetOppositeRunway(*rwindex);
            }
            else
            {
                if (info->status < tReqTaxi)
                    *rwindex = GetOppositeRunway(*rwindex);
            }
        }
    }

}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
runwayQueueStruct* ATCBrain::NextToTakeoff(int queue)
{
    runwayQueueStruct *temp = runwayQueue[queue];
    runwayQueueStruct *holdshort = NULL;
    runwayQueueStruct *taxi = NULL;
    runwayQueueStruct *wait = NULL;

    while (temp)
    {
        if (temp->status == noATC and not holdshort and not taxi and not wait)
        {
            return temp;
        }
        else if (temp->status >= tPrepToTakeRunway and temp->status <= tTakeoff)
        {
            return temp;
        }
        else if ( not holdshort and temp->status == tHoldShort)
        {
            holdshort = temp;
        }
        else if ( not wait and temp->status == tWait)
        {
            wait = temp;
        }
        else if ( not taxi and temp->status >= tTaxi and temp->status < tHoldShort)
        {
            taxi = temp;
        }

        temp = temp->next;
    }

    if (holdshort)
        return holdshort;
    else if (wait)
        return wait;
    else
        return taxi;
}

runwayQueueStruct* ATCBrain::NextToLand(int queue)
{
    runwayQueueStruct *temp = NULL;
    AircraftClass *aircraft = (AircraftClass*)vuDatabase->Find(runwayStats[queue].nextEmergency);

    if (aircraft)
    {
        temp = InList(aircraft->Id());

        if (temp)
            return temp;
    }

    temp = runwayQueue[queue];

    while (temp)
    {
        if (temp->status >= lReqClearance and temp->status < lLanded)
        {
            return temp;
        }

        temp = temp->next;
    }

    return NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ATCBrain::CheckVector(AircraftClass *aircraft, runwayQueueStruct* info)
{
    float x, y, z, dx, dy, cosAngle, dist;
    float norm, vt, cosHdg, sinHdg, relx, rely;
    float turnDist, speed, deltaTime;
    float baseX, baseY, finalX, finalY;
    ulong turnTime;

    speed = aircraft->af->MinVcas() * KNOTS_TO_FTPSEC;
    aircraft->DBrain()->GetTrackPoint(x, y, z);

    dx = x - aircraft->XPos();
    dy = y - aircraft->YPos();
    dist = (float)sqrt(dx * dx + dy * dy);

    cosHdg = aircraft->platformAngles.cossig;
    sinHdg = aircraft->platformAngles.sinsig;

    relx = (cosHdg * dx + sinHdg * dy);
    rely = (-sinHdg * dx + cosHdg * dy);


    vt = (float)sqrt(aircraft->XDelta() * aircraft->XDelta() + aircraft->YDelta() * aircraft->YDelta());

    //turnDist = aircraft->DBrain()->TurnDistance() + vt * 3.0F;
    turnDist = aircraft->DBrain()->TurnDistance();

    switch (info->status)
    {
        case lFirstLeg:

            //if(dist < turnDist or (cosAngle < -0.5F and dist < turnDist + 1000.0F) or (cosAngle < 0.0F and dist < turnDist + 500.0F) )
            if (relx < turnDist and fabs(rely) < turnDist * 3.0F and info->lastContacted < SimLibElapsedTime)
            {
                FindFinalPt(aircraft, info->rwindex, &x, &y);
                dx = x - aircraft->XPos();
                dy = y - aircraft->YPos();
                dist = (float)sqrt(dx * dx + dy * dy);
                norm = (float)(1.0F / dist);
                dx *= norm;
                dy *= norm;

                cosAngle = dx * aircraft->XDelta() / vt +
                            dy * aircraft->YDelta() / vt;

                if (cosAngle < 0.0F)
                {
                    FindFinalPt(aircraft, info->rwindex, &finalX, &finalY);
                    FindBasePt(aircraft, info->rwindex, finalX, finalY, &baseX, &baseY);
                    aircraft->DBrain()->SetATCStatus(lToBase);
                    aircraft->DBrain()->SetTrackPoint(baseX, baseY, GetAltitude(aircraft, lToBase));
                    aircraft->DBrain()->CalculateNextTurnDistance();
                    info->status = lToBase;
                }
                else
                {
                    FindFinalPt(aircraft, info->rwindex, &finalX, &finalY);
                    aircraft->DBrain()->SetATCStatus(lToFinal);
                    aircraft->DBrain()->SetTrackPoint(finalX, finalY, GetAltitude(aircraft, lToFinal));
                    aircraft->DBrain()->CalculateNextTurnDistance();
                    info->status = lToFinal;
                }

                turnTime = FloatToInt32(turnDist / (12.15854203708F * 3.0F * vt));

                if (turnTime > 30 * CampaignSeconds)
                    turnTime -= 15 * CampaignSeconds;
                else
                    turnTime = 15 * CampaignSeconds;

                info->lastContacted = SimLibElapsedTime + turnTime;
                return TRUE;
            }

            break;


        case lToBase:
            if (relx < turnDist and fabs(rely) < turnDist * 3.0F and info->lastContacted < SimLibElapsedTime)
            {
                FindFinalPt(aircraft, info->rwindex, &finalX, &finalY);
                aircraft->DBrain()->SetATCStatus(lToFinal);
                aircraft->DBrain()->SetTrackPoint(finalX, finalY, GetAltitude(aircraft, lToFinal));
                aircraft->DBrain()->CalculateNextTurnDistance();
                info->status = lToFinal;

                turnTime = FloatToInt32(turnDist / (12.15854203708F * 3.0F * vt));

                if (turnTime > 30 * CampaignSeconds)
                    turnTime -= 15 * CampaignSeconds;
                else
                    turnTime = 15 * CampaignSeconds;

                info->lastContacted = SimLibElapsedTime + turnTime;
                return TRUE;
            }

            if (info->schedTime > SimLibElapsedTime + FINAL_TIME + BASE_TIME)
            {
                deltaTime = (info->schedTime - SimLibElapsedTime - FINAL_TIME - BASE_TIME) / (float)CampaignSeconds;
                speed = dist / deltaTime;
            }

            break;


        case lToFinal:
            cosHdg = PtHeaderDataTable[info->rwindex].cosHeading;
            sinHdg = PtHeaderDataTable[info->rwindex].sinHeading;

            relx = (cosHdg * dx + sinHdg * dy);
            rely = (-sinHdg * dx + cosHdg * dy);

            if (relx > 0.0F)
                rely = max(0.0F, (float)fabs(rely) - relx * 0.3F);

            //Cleared turn to final
            if (relx < 3.0F * NM_TO_FT and relx > -1.0F * NM_TO_FT and fabs(rely) < turnDist and info->lastContacted < SimLibElapsedTime)
            {
                aircraft->DBrain()->SetTaxiPoint(GetFirstPt(info->rwindex));
                aircraft->DBrain()->SetATCStatus(lOnFinal);
                TranslatePointData(self, GetFirstPt(info->rwindex), &x, &y);
                aircraft->DBrain()->SetTrackPoint(x, y, GetAltitude(aircraft, lOnFinal));
                info->status = lOnFinal;

                turnTime = FloatToInt32(turnDist / (12.15854203708F * 3.0F * vt));

                if (turnTime > 30 * CampaignSeconds)
                    turnTime -= 15 * CampaignSeconds;
                else
                    turnTime = 15 * CampaignSeconds;

                info->lastContacted = SimLibElapsedTime + turnTime;
                return TRUE;
            }

            if (info->schedTime > SimLibElapsedTime + FINAL_TIME)
            {
                deltaTime = (info->schedTime - SimLibElapsedTime - FINAL_TIME) / (float)CampaignSeconds;
                speed = dist / deltaTime;
            }

            break;

        case lReqClearance:
        case lReqEmerClearance:
        case lIngressing:
        case lTakingPosition:
        case lAborted:
        case lEmerHold:
        case lHolding:
        case lOnFinal:
        case lEmergencyToBase:
        case lEmergencyToFinal:
        case lClearToLand:
            return FALSE;
            break;

        default:
            //we should never get here
            ShiWarning("We are in an undefined state, we shouldn't be here");
            break;
    }

    norm = (float)(1.0F / dist);
    dx *= norm;
    dy *= norm;

    cosAngle = dx * aircraft->XDelta() / vt +
                dy * aircraft->YDelta() / vt;

    // RAS - 3Oct04 - Verify this information
    // Old information - Broken out to make easier to debug
    /*
    if( (info->lastContacted + 30 * CampaignSeconds < SimLibElapsedTime and dist > 4.0F*turnDist and 
     (cosAngle < 0.965925F or info->status < lLanded and fabs(speed - vt) > 30.0F)) or
     (info->lastContacted + 15 * CampaignSeconds < SimLibElapsedTime and cosAngle < 0.5F and dist > 3.0F*turnDist) or
     (info->lastContacted + 8 * CampaignSeconds < SimLibElapsedTime and cosAngle < -0.866F )or
     info->lastContacted + 2 * CampaignMinutes < SimLibElapsedTime and dist > 4.0F*turnDist)
    {
     //we're not heading for our track point
     MakeVectorCall(aircraft, FalconLocalGame);
     info->lastContacted = SimLibElapsedTime;
    }
    */

    if (info->lastContacted + 30 * CampaignSeconds < SimLibElapsedTime)
    {
        if (dist > 4.0F * turnDist)
            if (cosAngle < 0.965925F)
            {
                //we're not heading for our track point
                MakeVectorCall(aircraft, FalconLocalGame);
                info->lastContacted = SimLibElapsedTime;
                return FALSE;
            }
    }
    //RAS-3Oct04-added airspeed check every 20 seconds instead of every time
    else if (info->lastContacted + 20 * CampaignSeconds < SimLibElapsedTime)
    {
        if (info->status < lLanded and fabs(speed - vt) > 30.0F) //haven't landed and speed is off by > 30kts
        {
            //we're not heading for our track point
            MakeVectorCall(aircraft, FalconLocalGame);
            info->lastContacted = SimLibElapsedTime;
            return FALSE;
        }
    }
    else if (info->lastContacted + 15 * CampaignSeconds < SimLibElapsedTime)
    {
        if (cosAngle < 0.5F)
        {
            if (dist > 3.0F * turnDist)
            {
                //we're not heading for our track point
                MakeVectorCall(aircraft, FalconLocalGame);
                info->lastContacted = SimLibElapsedTime;
                return FALSE;
            }
        }
    }
    else if (info->lastContacted + 8 * CampaignSeconds < SimLibElapsedTime)
    {
        if (cosAngle < -0.866F)
        {
            //we're not heading for our track point
            MakeVectorCall(aircraft, FalconLocalGame);
            info->lastContacted = SimLibElapsedTime;
            return FALSE;
        }
    }
    else if (info->lastContacted + 2 * CampaignMinutes < SimLibElapsedTime)
    {
        if (dist > 4.0F * turnDist)
        {
            //we're not heading for our track point
            MakeVectorCall(aircraft, FalconLocalGame);
            info->lastContacted = SimLibElapsedTime;
            return FALSE;
        }
    }

    // End if statement breakup
    return FALSE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ATCBrain::FindFinalPt(AircraftClass* approaching, int rwindex, float *x, float *y)
{
    float dist;
    float px, py;

    TranslatePointData(self, GetFirstPt(rwindex), &px, &py);

    //assume landing speed is 60% MinVcas use linear decel from minvcas
    dist = 0.8F * approaching->af->MinVcas() * KNOTS_TO_FTPSEC * FINAL_TIME / CampaignSeconds;

    *x = px + dist * PtHeaderDataTable[rwindex].cosHeading;
    *y = py + dist * PtHeaderDataTable[rwindex].sinHeading;

}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
AtcStatusEnum ATCBrain::FindBasePt(AircraftClass* approaching, int rwindex, float finalX, float finalY, float *x, float *y)
{
    float dist, cosAngle, sinAngle, dx, dy, norm;

    dx = dy = 0.0F;

    if (PtHeaderDataTable[rwindex].ltrt > 0)
    {
        //want sin and cos of heading 90 degrees to right of runway heading
        cosAngle = -PtHeaderDataTable[rwindex].sinHeading;
        sinAngle = PtHeaderDataTable[rwindex].cosHeading;
    }
    else
    {
        cosAngle = PtHeaderDataTable[rwindex].sinHeading;
        sinAngle = -PtHeaderDataTable[rwindex].cosHeading;

        if (PtHeaderDataTable[rwindex].ltrt == 0)
        {
            dx = finalX - approaching->XPos();
            dy = finalY - approaching->YPos();
            norm = (float)(1.0F / sqrt(dx * dx + dy * dy));
            dx *= norm;
            dy *= norm;

            if (dx * cosAngle + dy * sinAngle > 0.0F)
            {
                cosAngle = -PtHeaderDataTable[rwindex].sinHeading;
                sinAngle = PtHeaderDataTable[rwindex].cosHeading;
            }
        }
    }

    dist = approaching->af->MinVcas() * KNOTS_TO_FTPSEC * BASE_TIME / CampaignSeconds;

    *x = finalX + dist * cosAngle;
    *y = finalY + dist * sinAngle;
    return lToBase;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
AtcStatusEnum ATCBrain::FindFirstLegPt(AircraftClass* approaching, int rwindex, ulong schedTime, float pointX, float pointY, int usebase, float *x, float *y)
{
    float dist = 0.0F, totalDist = 0.0F, legAngle = 0.0F, legHeading = 0.0F, hdgToPt = 0.0F;
    float dx = 0.0F, dy = 0.0F, PatternSpd = 0.0F, decelTime = 0.0F, avgDecelSpd = 0.0F;
    float deltaTime = 0.0F;
    mlTrig Trig;

    PatternSpd = approaching->af->MinVcas() * KNOTS_TO_FTPSEC;

    dx = pointX - approaching->XPos();
    dy = pointY - approaching->YPos();

    dist = (float)sqrt(dx * dx + dy * dy);

    //assume a 5knot/s deceleration
    decelTime = (float)fabs(approaching->af->vt - PatternSpd) * 0.2F;
    avgDecelSpd = (approaching->af->vt + PatternSpd) * 0.5F;

    deltaTime = (float)(schedTime - FINAL_TIME - BASE_TIME *  usebase - SimLibElapsedTime) / CampaignSeconds;

    if (deltaTime < -10.0F)
    {
        //hmmm, we don't have enough time for the base pt
        if (deltaTime > -BASE_TIME / CampaignSeconds and usebase)
        {
            *x = pointX;
            *y = pointY;
            return lToFinal;
        }
        else
        {
            //can't make a final approach, and arrive in time
            float z;
            FindAbortPt(approaching, x, y, &z);
            return lAborted;
        }

    }

    //cobra this fixes aborts not ever being allowed to come back
    /*if(deltaTime < decelTime)
    {
     *x = approaching->XPos();
     *y = approaching->YPos();
     return lHolding;
    }*/

    totalDist = decelTime * avgDecelSpd + PatternSpd * (deltaTime - decelTime);

    if (totalDist > dist)
        legAngle = (float)acos(dist / totalDist);

    //legAngle = atan2( sqrt(totalDist*totalDist*0.25F - dist*dist*0.25F), dist*0.5F );

    if (legAngle < 10.0F * DTR or totalDist <= dist)
    {
        //if less than 10 degrees, just fly to  the base/final pt
        *x = pointX;
        *y = pointY;

        if (usebase)
            return lToBase;
        else
            return lToFinal;
    }
    else if (legAngle > 70.0F * DTR)
    {
        *x = approaching->XPos();
        *y = approaching->YPos();
        return lHolding;
    }

    hdgToPt = (float)atan2(dy, dx);

    if (PtHeaderDataTable[rwindex].ltrt)
        legHeading = hdgToPt + PtHeaderDataTable[rwindex].ltrt * legAngle;
    else
    {
        float norm = (float)(1.0F / dist);
        dx *= norm;
        dy *= norm;

        if (dx * -PtHeaderDataTable[rwindex].sinHeading + dy * PtHeaderDataTable[rwindex].cosHeading > 0.0F)
        {
            legHeading = hdgToPt - legAngle;
        }
        else
            legHeading = hdgToPt + legAngle;

        //for now always go right then left on the legs
        //if necessary we will calculate the best direction to turn first
        //legHeading = hdgToPt + legAngle;
    }

    mlSinCos(&Trig, legHeading);

    *x = approaching->XPos() + Trig.cos * totalDist * 0.5F;
    *y = approaching->YPos() + Trig.sin * totalDist * 0.5F;

    return lFirstLeg;

}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ATCBrain::FindTakeoffPt(FlightClass* flight, int vehicleInUnit, int rwindex, float *x, float *y)
{
    int point;
    float heading, dir;
    mlTrig Trig;
    Falcon4EntityClassType* classPtr;

    point = GetNextPtLoop(GetFirstPt(rwindex));

    TranslatePointData(self, point, x, y);

    classPtr = (Falcon4EntityClassType*)flight->EntityType();

    if (UseSectionTakeoff(flight, rwindex))
    {
        if (PtHeaderDataTable[rwindex].ltrt)
            dir = PtHeaderDataTable[rwindex].ltrt;
        else
            dir = 1.0F;

        if (vehicleInUnit % 2)
            heading = (PtHeaderDataTable[rwindex].data + dir * 90.0F) * DTR;
        else
            heading = (PtHeaderDataTable[rwindex].data + dir * -90.0F) * DTR;

        mlSinCos(&Trig, heading);
        *x += runwayStats[PtHeaderDataTable[rwindex].runwayNum].halfwidth / 2.0F * Trig.cos;
        *y += runwayStats[PtHeaderDataTable[rwindex].runwayNum].halfwidth / 2.0F * Trig.sin;

        if ( not (vehicleInUnit % 2))
        {
            *x += PtHeaderDataTable[rwindex].cosHeading * 100.0F;
            *y += PtHeaderDataTable[rwindex].sinHeading * 100.0F;
        }
    }


    return point;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ATCBrain::FindRunwayPt(FlightClass* flight, int vehicleInUnit, int rwindex, float *x, float *y)
{
    int point;
    float heading, dir;
    mlTrig Trig;

    point = GetFirstPt(rwindex);
    TranslatePointData(self, point, x, y);

    if (UseSectionTakeoff(flight, rwindex))
    {
        if (PtHeaderDataTable[rwindex].ltrt)
            dir = PtHeaderDataTable[rwindex].ltrt;
        else
            dir = 1.0F;

        if (vehicleInUnit % 2)
            heading = (PtHeaderDataTable[rwindex].data + dir * 90.0F) * DTR;
        else
            heading = (PtHeaderDataTable[rwindex].data + dir * -90.0F) * DTR;

        mlSinCos(&Trig, heading);

        *x += runwayStats[PtHeaderDataTable[rwindex].runwayNum].halfwidth / 2.0F * Trig.cos;
        *y += runwayStats[PtHeaderDataTable[rwindex].runwayNum].halfwidth / 2.0F * Trig.sin;
    }

    return point;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float ATCBrain::GetAltitude(AircraftClass* aircraft, AtcStatusEnum status)
{
    float alt = 0.0F;
    Tpoint pos;
    SimBaseClass* entity = NULL;

    {
        // destroy iterator here
        VuListIterator cit(self->GetComponents());
        entity = (SimBaseClass*)cit.GetFirst();

        while (entity and not entity->drawPointer)
        {
            entity = (SimBaseClass*)cit.GetNext();
        }
    }

    if (entity and entity->drawPointer)
        entity->drawPointer->GetPosition(&pos);
    else
        pos.z = aircraft->af->groundZ;

    switch (status)
    {
        case lReqClearance:
        case lAborted:
        case noATC: // JPO - when landing but outside airspace.
            alt = pos.z - 4000.0F;
            break;

        case lIngressing:
        case lTakingPosition:
        case lEmerHold:
        case lHolding:
        case lFirstLeg:
            alt = pos.z - aircraft->af->MinVcas() * KNOTS_TO_FTPSEC * (FINAL_TIME * 0.8F + BASE_TIME + 60000) / CampaignSeconds *  TAN_THREE_DEG_GLIDE;
            break;

        case lEmergencyToBase:
        case lToBase:
            alt = pos.z - aircraft->af->MinVcas() * KNOTS_TO_FTPSEC * (FINAL_TIME * 0.8F + BASE_TIME) / CampaignSeconds *  TAN_THREE_DEG_GLIDE;
            break;

        case lToFinal:
        case lEmergencyToFinal:
            alt = pos.z - aircraft->af->MinVcas() * KNOTS_TO_FTPSEC * (FINAL_TIME * 0.8F) / CampaignSeconds *  TAN_THREE_DEG_GLIDE;
            break;

        case lOnFinal:
        case lEmergencyOnFinal:
            alt = pos.z;
            break;

        default:
            alt = pos.z - 5.0F;
            break;
    }

    return alt;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ATCBrain::CalculateStandRateTurnToPt(AircraftClass *aircraft, float x, float y, float *finalHdg)
{
    float dx, dy, tx, ty, hdgToPt, deltaHdg, dist, vt, heading;
    int dir;
    mlTrig Trig;

    dx = x - aircraft->XPos();
    dy = y - aircraft->YPos();

    vt = (float)sqrt(aircraft->XDelta() * aircraft->XDelta() + aircraft->YDelta() * aircraft->YDelta());

    hdgToPt = (float)atan2(dy, dx);

    if (hdgToPt < 0.0F)
        hdgToPt += PI * 2.0F;

    heading = aircraft->Yaw();

    if (heading < 0.0F)
        heading += PI * 2.0F;

    deltaHdg = hdgToPt - heading;

    if (deltaHdg > PI)
        deltaHdg -= (2.0F * PI);
    else if (deltaHdg < -PI)
        deltaHdg += (2.0F * PI);

    if (fabs(deltaHdg) < 5.0F * DTR)
        dir = -1; //no turn
    else if (deltaHdg < -PI)
        dir = 1;
    else if (deltaHdg > PI)
        dir = 0;
    else if (deltaHdg < 0.0F)
        dir = 0;//left
    else
        dir = 1;//right

    if (fabs(deltaHdg) <  0.3490658503989F) //20degrees
    {
        *finalHdg = hdgToPt * RTD;
        return dir;
    }

    dist = (float)fabs(deltaHdg) * 12.15854203708F * vt ;

    //mlSinCos(&Trig,hdgToPt + dir * PI * 0.5F);
    mlSinCos(&Trig, aircraft->Yaw());

    tx = aircraft->XPos() + Trig.cos * dist;
    ty = aircraft->YPos() + Trig.sin * dist;

    dx = x - tx;
    dy = y - ty;

    *finalHdg = (float)atan2(dy, dx) * RTD;

    //Hack
    //*finalHdg = hdgToPt * RTD;

    if (*finalHdg < 0.0F)
        *finalHdg += 360.0F;

    return dir;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
runwayQueueStruct* ATCBrain::InList(VU_ID aircraftID)
{
    int i;
    runwayQueueStruct *temp = NULL;

    // OW - sylvains refuelling fix
#if 1

    // ADDED BY S.G. - FIX FOR THE CTD WHEN TUNED TO THE TANKER TACAN AND ASKING FOR LANDING
    // THERE ARE NO RUNWAYS ASSIGNED TO A TANKER
    //if ( not runwayQueue) // JB 010304 CTD
    //if (F4IsBadReadPtr(runwayQueue, sizeof(runwayQueueStruct*)) or not runwayQueue) // JB 010304 CTD
    if (
        F4IsBadReadPtr(this, sizeof(ATCBrain)) or
        F4IsBadReadPtr(runwayQueue, sizeof(runwayQueueStruct*)) or not runwayQueue) // JB 010317 CTD
    {
        return NULL;
    }

    // END OF ADDED SECTION
#endif

    for (i = 0; i < numRwys; i++)
    {
        temp = runwayQueue[i];

        while (temp)
        {
            if (temp->aircraftID == aircraftID)
            {
                return temp;
            }

            temp = temp->next;
        }
    }

    temp = inboundQueue;

    while (temp)
    {
        if (temp->aircraftID == aircraftID)
        {
            return temp;
        }

        temp = temp->next;
    }

    return temp;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
runwayQueueStruct* ATCBrain::AddToList(runwayQueueStruct* list, runwayQueueStruct* info)
{
    runwayQueueStruct* listPtr = list;
    runwayQueueStruct* listPrev = NULL;

    while (listPtr and listPtr->schedTime <= info->schedTime)
    {
        listPrev = listPtr;
        listPtr = listPtr->next;
    }

    info->prev = listPrev;

    if (listPrev)
    {
        info->next = listPrev->next;
        listPrev->next = info;
    }
    else
    {
        info->next = list;
        list = info;
    }

    if (info->next)
    {
        info->next->prev = info;
    }

    return list;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
runwayQueueStruct * ATCBrain::AddTraffic(VU_ID aircraftID, AtcStatusEnum status, int rwindex, long schedTime)
{
    int queue = PtHeaderDataTable[rwindex].runwayNum;

    runwayQueueStruct *newTraffic = NULL;
    int i, point, position;

    newTraffic = InList(aircraftID);
    FalconEntity *entity = (FalconEntity*)vuDatabase->Find(aircraftID);

    if (newTraffic and entity->IsAirplane())
    {
        ShiAssert( not ( not newTraffic->rwindex and runwayQueue[queue] == newTraffic));

        //if we're already in list, pull us and reinsert us
        if (newTraffic->rwindex and runwayQueue[PtHeaderDataTable[newTraffic->rwindex].runwayNum] == newTraffic)
            runwayQueue[PtHeaderDataTable[newTraffic->rwindex].runwayNum] = newTraffic->next;
        else if (inboundQueue == newTraffic)
            inboundQueue = newTraffic->next;

        if (newTraffic->prev)
            newTraffic->prev->next = newTraffic->next;

        if (newTraffic->next)
            newTraffic->next->prev = newTraffic->prev;

        if (newTraffic->rwindex)
            runwayStats[PtHeaderDataTable[newTraffic->rwindex].runwayNum].numInQueue--;
    }
    else
    {
        //not in list create a new link
        newTraffic = new runwayQueueStruct;
        ShiAssert(newTraffic);
    }

    //initialize all the data
    newTraffic->next = NULL;
    newTraffic->prev = NULL;

    newTraffic->aircraftID = aircraftID;
    newTraffic->status = status;
    newTraffic->lastContacted = 0;
    newTraffic->schedTime = schedTime;
    newTraffic->rwindex = rwindex;

    //if we are taking off set up our point appropriately
    if (status > lCrashed)
    {
        point = PtHeaderDataTable[rwindex].first;
        point = GetNextPtLoop(point);
        position = (schedTime - SimLibElapsedTime) / (SLOT_TIME);

        for (i = 0; i < position; i++)
            point = GetNextTaxiPt(point);

    }

    runwayStats[queue].numInQueue++; //increment number in queue

    runwayQueue[queue] = AddToList(runwayQueue[queue], newTraffic);

    return newTraffic;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
runwayQueueStruct* ATCBrain::RemoveFromList(runwayQueueStruct* list, runwayQueueStruct* info)
{
    if (info->prev)
    {
        info->prev->next = info->next;
    }
    else
    {
        ShiAssert(info == list);
        list = info->next;
    }

    if (info->next)
        info->next->prev = info->prev;

    return list;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ATCBrain::RemoveTraffic(VU_ID aircraftID, int queue)
{
    runwayQueueStruct *cur = runwayQueue[queue];

    //ShiAssert( GetCurrentThreadId() == gSimThreadID );

    while (cur)
    {
        if (cur->aircraftID == aircraftID)
        {
            runwayQueue[queue] = RemoveFromList(runwayQueue[queue], cur);

#ifdef TEST_HACK_THAT_LEAKS
            // We'll leave this thing around, but tag it with the "this" of the atc brain that tried to kill it
            cur->deletor = this;
            cur->deleteLine = __LINE__;
#else
            delete cur;
#endif
            runwayStats[queue].numInQueue--;

            break;
        }

        cur = cur->next;
    }

    if (runwayStats[queue].nextEmergency == aircraftID)
    {
        FindNextEmergency(queue);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ATCBrain::AddInbound(AircraftClass *aircraft)
{
    runwayQueueStruct* info;


    if (aircraft->OnGround())
        return;

    info = InList(aircraft->Id());

    if (info)
    {
        if (info->rwindex and runwayQueue[PtHeaderDataTable[info->rwindex].runwayNum] == info)
            runwayQueue[PtHeaderDataTable[info->rwindex].runwayNum] = info->next;
        else if (inboundQueue == info)
            inboundQueue = info->next;

        if (info->prev)
            info->prev->next = info->next;

        if (info->next)
            info->next->prev = info->prev;

        if (info->rwindex)
            runwayStats[PtHeaderDataTable[info->rwindex].runwayNum].numInQueue--;
    }
    else
    {
        info = new runwayQueueStruct;

        if ( not info)
            return;
    }

    if (inboundQueue and F4IsBadReadPtr(inboundQueue, sizeof(runwayQueueStruct))) // JB 010326 CTD
        inboundQueue = NULL; // JB 010326 CTD

    info->aircraftID = aircraft->Id(); //which plane is it
    info->status = lTakingPosition; //at what point in the landing/takeoff process
    info->schedTime = 0; //when scheduled to be on runway
    info->lastContacted = 0; //time last talked to
    info->rwindex = 0; //what runway I'm supposed to use
    info->prev = NULL;
    info->next = inboundQueue;

    if (inboundQueue)
        inboundQueue->prev = info;

    inboundQueue = info;
    SendCmdMessage(aircraft, info);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ATCBrain::AddInboundFlight(FlightClass *flight)
{
    AircraftClass *aircraft;

    // protect against no components
    if ( not flight->GetComponents())
        return;

    VuListIterator flightIter(flight->GetComponents());
    aircraft = (AircraftClass*) flightIter.GetFirst();

    while (aircraft)
    {
        AddInbound(aircraft);
        aircraft = (AircraftClass*) flightIter.GetNext();
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ATCBrain::RemoveInbound(runwayQueueStruct* info)
{
    if (info == inboundQueue)
    {
        inboundQueue = info->next;
    }
    else
    {
        ShiAssert(info->prev);

        if (info->prev) // JB 010109 CTD sanity check
            info->prev->next = info->next;
    }

    if (info->next)
        info->next->prev = info->prev;

#ifdef TEST_HACK_THAT_LEAKS
    // We'll leave this thing arround, but tag it with the "this" of the atc brain that tried to kill it
    info->deletor = this;
    info->deleteLine = __LINE__;
#else
    delete info;
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ATCBrain::GiveOrderToWingman(AircraftClass *us, AtcStatusEnum status)
{
    runwayQueueStruct *wingmanInfo = NULL;
    AircraftClass *wingman = NULL;

    if ( not us)
        return;

    if (us->vehicleInUnit == 1 or us->vehicleInUnit == 3)
        return;

    wingman = (AircraftClass*)us->GetCampaignObject()->GetComponentNumber(WingmanTable[us->vehicleInUnit]);

    if (wingman)
        wingmanInfo = InList(wingman->Id());

    if (UseSectionTakeoff((Flight)us->GetCampaignObject(), us->DBrain()->Runway()))
    {
        if (wingmanInfo and wingmanInfo->status < status)
        {
            wingmanInfo->status = status;
            SendCmdMessage(wingman, wingmanInfo);
        }
    }
    else if (wingmanInfo and (status == tTakeoff or status == tTakeRunway) and wingmanInfo->status < status)
    {
        wingmanInfo->status = tPrepToTakeRunway;
        SendCmdMessage(wingman, wingmanInfo);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ATCBrain::GiveOrderToSection(AircraftClass *us, AtcStatusEnum status, int section)
{
    runwayQueueStruct *info = NULL;
    AircraftClass *aircraft = NULL;
    int i;

    if ( not us)
        return;

    for (i = 0; i < 2; i++)
    {
        aircraft = (AircraftClass*)us->GetCampaignObject()->GetComponentNumber(section * 2 + i);

        if (aircraft)
        {
            info = InList(aircraft->Id());

            if (info and info->status < status)
            {
                info->status = status;
                SendCmdMessage(aircraft, info);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ATCBrain::SendCmdMessage(AircraftClass* aircraft, runwayQueueStruct* info)
{
    if (F4IsBadReadPtr(self, sizeof(ObjectiveClass))) // JB 010408 CTD
        return;

    FalconATCCmdMessage* ATCCmdMessage = new FalconATCCmdMessage(aircraft->Id(), FalconLocalSession/*me123 from localgame*/);
    ATCCmdMessage->dataBlock.from = self->Id();
    //I am sending an actual time instead of a delta, because of the huge time differences between different
    //machines at startup
    ATCCmdMessage->dataBlock.rwtime = info->schedTime;
    ATCCmdMessage->dataBlock.rwindex = (short)info->rwindex;

    switch (info->status)
    {
        case lIngressing:
            ATCCmdMessage->dataBlock.type = FalconATCCmdMessage::TakePosition;
            info->lastContacted = SimLibElapsedTime;
            break;

        case lTakingPosition:
            ATCCmdMessage->dataBlock.type = FalconATCCmdMessage::TakePosition;
            info->lastContacted = SimLibElapsedTime;
            break;

        case lAborted:
            ATCCmdMessage->dataBlock.type = FalconATCCmdMessage::Abort;
            info->lastContacted = SimLibElapsedTime;
            break;

        case lEmerHold:
            ATCCmdMessage->dataBlock.type = FalconATCCmdMessage::EmergencyHold;
            info->lastContacted = SimLibElapsedTime;
            break;

        case lHolding:
            ATCCmdMessage->dataBlock.type = FalconATCCmdMessage::Hold;
            info->lastContacted = SimLibElapsedTime;
            break;

        case lFirstLeg:
            ATCCmdMessage->dataBlock.type = FalconATCCmdMessage::ToFirstLeg;
            break;

        case lToBase:
            ATCCmdMessage->dataBlock.type = FalconATCCmdMessage::ToBase;
            break;

        case lToFinal:
            ATCCmdMessage->dataBlock.type = FalconATCCmdMessage::ToFinal;
            break;

        case lOnFinal:
            ATCCmdMessage->dataBlock.type = FalconATCCmdMessage::OnFinal;
            info->lastContacted = SimLibElapsedTime + 30 * CampaignSeconds;
            break;

        case lClearToLand:
            ATCCmdMessage->dataBlock.type = FalconATCCmdMessage::ClearToLand;
            info->lastContacted = SimLibElapsedTime + 30 * CampaignSeconds;
            break;

        case lLanded:
            ATCCmdMessage->dataBlock.type = FalconATCCmdMessage::Landed;
            info->lastContacted = SimLibElapsedTime + 30 * CampaignSeconds;
            break;

        case lTaxiOff:
            ATCCmdMessage->dataBlock.type = FalconATCCmdMessage::TaxiOff;
            break;

        case lEmergencyToBase:
            ATCCmdMessage->dataBlock.type = FalconATCCmdMessage::EmerToBase;
            info->lastContacted = SimLibElapsedTime;
            break;

        case lEmergencyToFinal:
            ATCCmdMessage->dataBlock.type = FalconATCCmdMessage::EmerToFinal;
            info->lastContacted = SimLibElapsedTime;
            break;

        case lEmergencyOnFinal:
            ATCCmdMessage->dataBlock.type = FalconATCCmdMessage::EmerOnFinal;
            info->lastContacted = SimLibElapsedTime;
            break;

        case tEmerStop:
            ATCCmdMessage->dataBlock.type = FalconATCCmdMessage::EmergencyStop;
            info->lastContacted = SimLibElapsedTime;
            break;

        case tWait:
            ATCCmdMessage->dataBlock.type = FalconATCCmdMessage::Wait;
            info->lastContacted = SimLibElapsedTime;
            break;

        case tTaxi:
            ATCCmdMessage->dataBlock.type = FalconATCCmdMessage::Taxi;
            info->lastContacted = SimLibElapsedTime;
            break;

        case tHoldShort:
            ATCCmdMessage->dataBlock.type = FalconATCCmdMessage::HoldShort;
            info->lastContacted = SimLibElapsedTime;
            break;

        case tPrepToTakeRunway:
            ATCCmdMessage->dataBlock.type = FalconATCCmdMessage::PrepToTakeRunway;
            info->lastContacted = SimLibElapsedTime;
            break;

        case tTakeRunway:
            ATCCmdMessage->dataBlock.type = FalconATCCmdMessage::TakeRunway;
            info->lastContacted = SimLibElapsedTime;
            break;

        case tTakeoff:
            ATCCmdMessage->dataBlock.type = FalconATCCmdMessage::Takeoff;
            info->lastContacted = SimLibElapsedTime;
            break;

        case noATC:
        case tFlyOut:
            ATCCmdMessage->dataBlock.type = FalconATCCmdMessage::Release;
            ATCCmdMessage->dataBlock.rwtime = 0;
            ATCCmdMessage->dataBlock.rwindex = 0;
            break;

        case tTaxiBack:
            ATCCmdMessage->dataBlock.type = FalconATCCmdMessage::TaxiBack;
            info->lastContacted = SimLibElapsedTime;
            break;

        default:
            //we shouldn't get here
            ShiWarning("This ATCCmd message type doesn't exist");
            delete ATCCmdMessage;
            return;

    }

    FalconSendMessage(ATCCmdMessage, TRUE);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ATCBrain::GetRunwayTexture(int component)
{
    int i, index = 0;

    for (i = 0; i < numRwys; i++)
    {
        if (PtHeaderDataTable[runwayStats[i].rwIndexes[0]].features[0] == component)
            index = PtHeaderDataTable[runwayStats[i].rwIndexes[0]].texIdx;

        if (PtHeaderDataTable[runwayStats[i].rwIndexes[1]].features[0] == component)
            index = PtHeaderDataTable[runwayStats[i].rwIndexes[1]].texIdx;
    }

    return index;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
short ATCBrain::GetTextureIdFromHdg(int hdg, int ltrt)
{
    short offset = 0;

    if (ltrt == -1)
        offset = 1;
    else if (ltrt == 1)
        offset = 2;

    switch (hdg)
    {
        case 0:
            return (short)(offset + 37);

        case 1:
            return (short)(offset + 0);

        case 2:
            return (short)(offset + 1);

        case 3:
            return (short)(offset + 4);

        case 4:
            return (short)(offset + 4);

        case 5:
            return (short)(offset + 40);

        case 6:
            return (short)(offset + 5);

        case 7:
            return (short)(offset + 6);

        case 8:
            return (short)(offset + 6);

        case 9:
            return (short)(offset + 7);

        case 10:
            return (short)(offset + 8);

        case 11:
            return (short)(offset + 8);

        case 12:
            return (short)(offset + 9);

        case 13:
            return (short)(offset + 10);

        case 14:
            return (short)(offset + 10);

        case 15:
            return (short)(offset + 16);

        case 16:
            return (short)(offset + 13);

        case 17:
            return (short)(offset + 16);

        case 18:
            return (short)(offset + 17);

        case 19:
            return (short)(offset + 20);

        case 20:
            return (short)(offset + 21);

        case 21:
            return (short)(offset + 24);

        case 22:
            return (short)(offset + 24);

        case 23:
            return (short)(offset + 43);

        case 24:
            return (short)(offset + 25);

        case 25:
            return (short)(offset + 26);

        case 26:
            return (short)(offset + 26);

        case 27:
            return (short)(offset + 27);

        case 28:
            return (short)(offset + 28);

        case 29:
            return (short)(offset + 28);

        case 30:
            return (short)(offset + 29);

        case 31:
            return (short)(offset + 30);

        case 32:
            return (short)(offset + 30);

        case 33:
            return (short)(offset + 36);

        case 34:
            return (short)(offset + 33);

        case 35:
            return (short)(offset + 36);

        case 36:
            return (short)(offset + 37);


    }

    return (short)(offset + 37);
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ATCBrain::GetRunwayName(int rwindex)
{
    // We have some bad data somewhere, just fix that here for now...
    ShiAssert(rwindex >= 0);

    if (rwindex < 0)
        return 0;

    // FRB - CTD's here
    int index = PtHeaderDataTable[rwindex].texIdx;

    // JB 010508 Fix from Schumi
    /*
     //hack for unexpected 23L/R and 05L/R
     if(index >= 43)
     index = 27;
     else if(index >= 40)
     index = 5;
    */

    return index;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ATCBrain::MakeVectorCall(AircraftClass *aircraft, VuTargetEntity *target)
{
    FalconRadioChatterMessage *radioMessage = NULL;
    int rwindex, index;
    float x, y, z, speed, cosAngle;
    AtcStatusEnum status;
    unsigned long rwtime;
    float deltaTime, dx, dy, finalHdg, desAlt;

    rwtime = aircraft->DBrain()->RwTime();
    status = aircraft->DBrain()->ATCStatus();
    rwindex = aircraft->DBrain()->Runway();
    cosAngle = DetermineAngle(aircraft, rwindex, status);
    speed = aircraft->af->MinVcas();



    // JB 010527 (from MN) start
    //radioMessage = CreateCallFromATC (self, aircraft, rcATCVECTORS, target);
    switch (status)
    {
        case lEmergencyToFinal:
        case lToFinal:
            radioMessage = CreateCallFromATC(self, aircraft, rcATCVECTORSRW, target);
            radioMessage->dataBlock.edata[8] = (short)GetRunwayName(GetOppositeRunway(rwindex));//Cobra test
            break;

        default:
            //RAS-6Oct04-Change to from rcATCVECTORS to rcATCVECTORSRW for rwy calls
            radioMessage = CreateCallFromATC(self, aircraft, rcATCVECTORSRW, target);
            radioMessage->dataBlock.edata[8] = (short)GetRunwayName(GetOppositeRunway(rwindex));//Cobra test
    }

    // JB 010527 (from MN) end

    /*
    if(fabs(aircraft->ZPos() - desAlt) < 100.0F)
     index = 2;
    else if(aircraft->ZPos() < desAlt)
     index = 0;
    else
     index = 1;
    */

    //radioMessage->dataBlock.edata[2] = index;
    //radioMessage->dataBlock.edata[3] = -desAlt; //altitude in feet
    radioMessage->dataBlock.edata[2] = -1; //climb or descend
    radioMessage->dataBlock.edata[3] = -1; //altitude in feet

    aircraft->DBrain()->GetTrackPoint(x, y, z);

    switch (status)
    {
        case lFirstLeg:
            if (fabs(aircraft->af->vcas - speed) > 10.0F)
                radioMessage->dataBlock.edata[4] = (short)FloatToInt32(speed); //speed
            else
                radioMessage->dataBlock.edata[4] = -1; //speed

            radioMessage->dataBlock.edata[5] = (short)CalculateStandRateTurnToPt(aircraft, x, y, &finalHdg); //turn direction

            if (radioMessage->dataBlock.edata[5] >= 0)
                radioMessage->dataBlock.edata[6] = (short)FloatToInt32(finalHdg);
            else
                radioMessage->dataBlock.edata[6] = -1;

            radioMessage->dataBlock.edata[7] = -1; //vector type? , not relevant
            FalconSendMessage(radioMessage, FALSE);
            return;
            break;

        case lEmergencyToBase:
        case lToBase:
            if (rwtime > SimLibElapsedTime)
            {
                deltaTime = (rwtime - SimLibElapsedTime - 3 * CampaignMinutes) / (float)CampaignSeconds;
                dx = aircraft->XPos() - x;
                dy = aircraft->YPos() - y;
                speed = (float)sqrt(dx * dx + dy * dy) / deltaTime * FTPSEC_TO_KNOTS;
                speed = get_air_speed(speed, -1 * FloatToInt32(aircraft->af->z));
                speed = max(aircraft->af->MinVcas() * 0.8F, speed);
                speed = min(aircraft->af->MaxVcas() * 0.8F, speed);

            }

            desAlt = GetAltitude(aircraft, lToBase); //RAS-3Oct04-Changed from lToFinal to lToBase

            if (fabs(aircraft->ZPos() - desAlt) < 800.0F) //RAS-23Jan04-changed from 100 to 800
                index = -1; //RAS-3Oct04-Maintain Altitude-changed from 2 to -1
            //no climb/descend/maintain call if within 800
            //feet of alt
            else if (aircraft->ZPos() < desAlt)
                index = 1; // Descend
            else
                index = 0; // Climb


            //RAS-3Oct04- Set index to -1 if don't need to make altitude call
            radioMessage->dataBlock.edata[2] = (short)index;

            if (index == -1) //if on altitude, don't say climb,descend or maintain
                radioMessage->dataBlock.edata[3] = -1;
            else // if on altitude, don't say altitude
                radioMessage->dataBlock.edata[3] = (short)(-1 * FloatToInt32(desAlt)); //altitude in feet

            if (fabs(aircraft->af->vcas - speed) > 10.0F)
                radioMessage->dataBlock.edata[4] = (short)FloatToInt32(speed);  //speed
            else
                radioMessage->dataBlock.edata[4] = -1; //speed

            radioMessage->dataBlock.edata[5] = (short)CalculateStandRateTurnToPt(aircraft, x, y, &finalHdg); //turn direction

            if (radioMessage->dataBlock.edata[5] >= 0)
                radioMessage->dataBlock.edata[6] = (short)FloatToInt32(finalHdg);
            else
                radioMessage->dataBlock.edata[6] = -1;



            //RAS-3Oct04- updated vector type code to be sent every two min
            if (aircraft->DBrain()->lastVectorTypeCall < SimLibElapsedTime)
            {
                aircraft->DBrain()->lastVectorTypeCall = SimLibElapsedTime + 2 * CampaignMinutes;

                if (radioMessage->dataBlock.edata[3] == -1 and radioMessage->dataBlock.edata[4] == -1)
                    radioMessage->dataBlock.edata[7] = -1; //RAS-3Oct04-no vector type if no alt and airspeed call
                else
                {
                    if (PtHeaderDataTable[rwindex].ltrt > 0)
                    {
                        radioMessage->dataBlock.edata[7] = 1; //vector type
                        //RAS-3Oct04-Say rwy name when telling what vectors are for
                        radioMessage->dataBlock.edata[8] = (short)GetRunwayName(GetOppositeRunway(rwindex));
                    }
                    else if (PtHeaderDataTable[rwindex].ltrt < 0)
                    {
                        radioMessage->dataBlock.edata[7] = 2; //vector type
                        //RAS-3Oct04-Say rwy name when telling what vectors are for
                        radioMessage->dataBlock.edata[8] = (short)GetRunwayName(GetOppositeRunway(rwindex));
                    }
                    else
                        radioMessage->dataBlock.edata[7] = -1;
                }
            }
            else
            {
                radioMessage->dataBlock.edata[7] = -1;
                radioMessage->dataBlock.edata[8] = -1;
            }

            //End vector type code

            FalconSendMessage(radioMessage, FALSE);
            return;
            break;

        case lEmergencyToFinal:
        case lToFinal:
            if (rwtime > SimLibElapsedTime + 2 * CampaignMinutes)
            {
                deltaTime = (rwtime - SimLibElapsedTime - 2 * CampaignMinutes) / (float)CampaignSeconds;
                dx = aircraft->XPos() - x;
                dy = aircraft->YPos() - y;
                speed = (float)sqrt(dx * dx + dy * dy) / deltaTime * FTPSEC_TO_KNOTS;
                speed = get_air_speed(speed, -1 * FloatToInt32(aircraft->af->z));
                speed = max(aircraft->af->MinVcas() * 0.8F, speed);
                speed = min(aircraft->af->MaxVcas() * 0.8F, speed);
            }

            desAlt = GetAltitude(aircraft, lToFinal);

            if (fabs(aircraft->ZPos() - desAlt) < 500.0F) //RAS-23Jan04- changed from 100 to 500
                index = -1; //RAS-3Oct04-Maintain Altitude-changed from 2 to -1
            //no climb/descend/maintain call if within 500
            //feet of alt
            else if (aircraft->ZPos() < desAlt)
                index = 1; //Descend
            else
                index = 0; //Climb

            //RAS-3Oct04- Set index to -1 if don't need to make altitude call
            radioMessage->dataBlock.edata[2] = (short)index;

            if (index == -1) //if on altitude, don't say climb,descend or maintain
                radioMessage->dataBlock.edata[3] = -1;
            else // if on altitude, don't say altitude
                radioMessage->dataBlock.edata[3] = (short)(-1 * FloatToInt32(desAlt)); //altitude in feet

            if (fabs(aircraft->af->vcas - speed) > 10.0F)
                radioMessage->dataBlock.edata[4] = (short)FloatToInt32(speed); //speed
            else
                radioMessage->dataBlock.edata[4] = -1; //speed

            radioMessage->dataBlock.edata[5] = (short)CalculateStandRateTurnToPt(aircraft, x, y, &finalHdg); //turn direction

            if (radioMessage->dataBlock.edata[5] >= 0)
                radioMessage->dataBlock.edata[6] = (short)FloatToInt32(finalHdg); //heading
            else
                radioMessage->dataBlock.edata[6] = -1;

            radioMessage->dataBlock.edata[7] = 0; //vector type

            //RAS-3Oct04- updated vector type code to be sent every 30 Sec if getting vectors to final
            if (aircraft->DBrain()->lastVectorTypeCall < SimLibElapsedTime)
            {
                aircraft->DBrain()->lastVectorTypeCall = SimLibElapsedTime + 60 * CampaignSeconds;

                if (radioMessage->dataBlock.edata[3] == -1 and radioMessage->dataBlock.edata[4] == -1)
                {
                    radioMessage->dataBlock.edata[7] = -1; //RAS-3Oct04-no vector type if no alt and airspeed call
                    radioMessage->dataBlock.edata[8] = -1;
                }
                else
                {
                    radioMessage->dataBlock.edata[7] = 0; //vector type
                    //RAS-3Oct04-Say rwy name when telling what vectors are for
                    radioMessage->dataBlock.edata[8] = (short)GetRunwayName(GetOppositeRunway(rwindex)); // JB 010527 (from MN)
                }
            }
            else
            {
                radioMessage->dataBlock.edata[7] = -1; //vector type
                radioMessage->dataBlock.edata[8] = -1; //active rwy
            }

            //End vector type code

            FalconSendMessage(radioMessage, FALSE);
            return;
            break;

        case lEmergencyOnFinal:
        case lOnFinal:
        case lReqClearance:
        case lReqEmerClearance:
        case lIngressing:
        case lTakingPosition:
        case lEmerHold:
        case lHolding:

            break;

        default:
            //we should never get here
            break;
            //ShiAssert( not "We are in an undefined state, we shouldn't be here");
    }

    delete radioMessage;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ATCBrain::GetLandingNumber(runwayQueueStruct* landInfo)
{
    int pos = 0;

    if (landInfo and landInfo->rwindex)
    {
        runwayQueueStruct* info = runwayQueue[PtHeaderDataTable[landInfo->rwindex].runwayNum];

        while (info)
        {
            if (info->aircraftID == landInfo->aircraftID)
                break;

            if (info->status < tReqTaxi and info->status >= lReqClearance)
                pos++;

            info = info->next;
        }
    }

    return pos;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ATCBrain::GetTakeoffNumber(runwayQueueStruct* takeoffInfo)
{
    int pos = 0;

    if (takeoffInfo->status <= lCrashed)
        return -1;

    if (takeoffInfo and takeoffInfo->rwindex)
    {
        runwayQueueStruct* info = runwayQueue[PtHeaderDataTable[takeoffInfo->rwindex].runwayNum];

        while (info)
        {
            if (info->aircraftID == takeoffInfo->aircraftID)
                break;

            if (info->status > lCrashed and info->status < tTakeoff)
                pos++;

            info = info->next;
        }
    }

    return pos;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ATCBrain::GetOppositeRunway(int rwindex)
{
    // 2002-04-08 MN CTD fix
    int runway = 0;

    if (runwayStats[PtHeaderDataTable[rwindex].runwayNum].rwIndexes[0] == rwindex)
        runway = runwayStats[PtHeaderDataTable[rwindex].runwayNum].rwIndexes[1];
    else
        runway = runwayStats[PtHeaderDataTable[rwindex].runwayNum].rwIndexes[0];

    ShiAssert(runway >= 0);

    if (runway < 0)
        runway = 0;

    return runway;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SimBaseClass* CheckPointGlobal(AircraftClass *self, float x, float y)
{
    float tmpX, tmpY;
    SimBaseClass* testObject;
    float myRad, testRad;


    if (self and self->drawPointer)
    {
        myRad = self->drawPointer->Radius();
    }
    else
    {
        myRad = 40.0f;
    }

    VuListIterator unitWalker(SimDriver.objectList);
    testObject = (SimBaseClass*) unitWalker.GetFirst();

    while (testObject)
    {
        // ignore objects under these conditions:
        // Ourself
        // Not on ground
        if ( not testObject->OnGround() or
            testObject == self)
        {
            testObject = (SimBaseClass*) unitWalker.GetNext();
            continue;
        }

        tmpX = testObject->XPos() - x;
        tmpY = testObject->YPos() - y;

        if (testObject->drawPointer)
            testRad = testObject->drawPointer->Radius() + myRad ; // FRB - Increase search dist. due to new parking spot locations ????
        else
            testRad = 40.0f + myRad;

        // if object is within a given range of the point return object
        if (tmpX * tmpX + tmpY * tmpY < testRad * testRad)
        {
            return testObject;
        }

        testObject = (SimBaseClass*) unitWalker.GetNext();
    }

    return NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SimBaseClass* CheckPointGlobal(CampBaseClass *unit, float x, float y)
{
    float tmpX, tmpY;
    SimBaseClass* testObject;
    float myRad, testRad;

    myRad = 40.0f;

    {
        // destroy iterator here
        VuListIterator unitWalker(SimDriver.objectList);
        testObject = (SimBaseClass*) unitWalker.GetFirst();

        while (testObject)
        {
            // ignore objects under these conditions:
            // Ourself
            // Not on ground
            if ( not testObject->OnGround())
            {
                testObject = (SimBaseClass*) unitWalker.GetNext();
                continue;
            }

            tmpX = testObject->XPos() - x;
            tmpY = testObject->YPos() - y;

            if (testObject->drawPointer)
                testRad = testObject->drawPointer->Radius() + myRad ;
            else
                testRad = 40.0f + myRad;

            // if object is within a given range of the point return object
            if (tmpX * tmpX + tmpY * tmpY < testRad * testRad)
            {
                return testObject;
            }

            testObject = (SimBaseClass*) unitWalker.GetNext();
        }
    }

    if (unit)
    {
        VuListIterator cit(unit->GetComponents());
        testObject = (SimBaseClass*)cit.GetFirst();

        while (testObject)
        {
            tmpX = testObject->XPos() - x;
            tmpY = testObject->YPos() - y;

            testRad = 40.0f + myRad; // FRB - Increase search dist. due to new parking spot locations ????

            // if object is within a given range of the point return object
            if (tmpX * tmpX + tmpY * tmpY < testRad * testRad)
            {
                return testObject;
            }

            testObject = (SimBaseClass*)cit.GetNext();
        }
    }

    return NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SimBaseClass* CheckTaxiPointGlobal(AircraftClass *self, float x, float y)
{
    float tmpX, tmpY;
    bool BigBoy = false;
    SimBaseClass* testObject;
    SimBaseClass* closest = NULL;
    float myRad, testRad, testDist, minDist =  1000000.0F; //square of 1000


    // Cobra - "large" aircraft?
    if (self->af->GetParkType() == LargeParkPt)
        BigBoy = true;

    if (self and self->drawPointer)
        myRad = self->drawPointer->Radius();
    else
        myRad = 40.0f;

    // Cobra - Slim down the Big Boys
    if (BigBoy)
        myRad = 10.f;

    VuListIterator unitWalker(SimDriver.objectList);
    testObject = (SimBaseClass*) unitWalker.GetFirst();

    while (testObject)
    {
        // ignore objects under these conditions:
        // Ourself
        // Not on ground
        if ( not testObject->OnGround() or
            testObject == self)
        {
            testObject = (SimBaseClass*) unitWalker.GetNext();
            continue;
        }

        tmpX = testObject->XPos() - x;
        tmpY = testObject->YPos() - y;

        testDist = tmpX * tmpX + tmpY * tmpY;

        // Cobra - Too far away to be in the way
        if (testDist > 300.f * 300.f)
        {
            testObject = (SimBaseClass*) unitWalker.GetNext();
            continue;
        }

        if (testObject->drawPointer)
            testRad = testObject->drawPointer->Radius() + myRad + AVOID_RANGE; // FRB - decreased AVOID_RANGE
        else
            testRad = 40.0f + myRad + AVOID_RANGE;

        // if object is within a given range of the point return object
        if (testDist < testRad * testRad and testObject->GetVt() < self->GetVt() + 20.0F)
        {
            if ( not closest)
            {
                closest = testObject;
                minDist = testDist;
            }
            else if (testDist < minDist)
            {
                // if(testDist <  10000.0F) // 100'
                if (testDist <  2500.0F) // 30JAN04 - FRB - 50'
                    return testObject;

                closest = testObject;
                minDist = testDist;
            }
        }

        testObject = (SimBaseClass*) unitWalker.GetNext();
    }

    return closest;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ATCBrain::NumOperableRunways(void)
{
    int num = 0, i;

    if ( not runwayStats) // JB 010531
        return 0;

    for (i = 0; i < numRwys; i++)
    {
        //cobra
        runwayStats[i].state = CheckHeaderStatus(self, runwayStats[i].rwIndexes[0]);

        //end
        if (runwayStats[i].state < VIS_DAMAGED)
            num++;
    }

    return num;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ATCBrain::CheckLanding(AircraftClass *aircraft, runwayQueueStruct* landInfo)
{
    ShiAssert(landInfo);
    FalconRadioChatterMessage *radioMessage;
    int queue = PtHeaderDataTable[landInfo->rwindex].runwayNum;
    runwayQueueStruct* next = NextToLand(queue);

    //ShiAssert(runwayQueue[queue]);

    if (aircraft->OnGround() and not aircraft->DBrain()->IsSetATC(DigitalBrain::Landed))
    {
        if (landInfo)
        {
            if (landInfo->status not_eq lEmergencyToBase and landInfo->status not_eq lEmergencyToFinal and 
                landInfo->status not_eq lEmergencyOnFinal and next not_eq landInfo and 
                landInfo->status not_eq lLanded) // JB 010713 If we're already landed, then what's the big deal?
            {
                float groundZ = OTWDriver.GetGroundLevel(aircraft->XPos(), aircraft->YPos());
                aircraft->FeatureCollision(groundZ);

                if (aircraft->onFlatFeature)
                {
                    //landed without permission
                    radioMessage = CreateCallFromATC(self, aircraft, rcTOWERSCOLD3, FalconLocalGame);
                    radioMessage->dataBlock.edata[3] = 32767;
                    FalconSendMessage(radioMessage, FALSE);
                    landInfo->lastContacted = SimLibElapsedTime;
                }
            }


            if (landInfo->status not_eq lLanded)
            {
                landInfo->status = lLanded;
                // landInfo->timer = SimLibElapsedTime + 15 * CampaignSeconds;
                SendCmdMessage(aircraft, landInfo);
            }
        }

        aircraft->DBrain()->SetATCFlag(DigitalBrain::Landed);
        return TRUE;
    }

    return FALSE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ATCBrain::CheckTakeoff(AircraftClass *aircraft, runwayQueueStruct* info)
{
    FalconRadioChatterMessage *radioMessage;

    if ( not aircraft->OnGround() and aircraft->DBrain()->IsSetATC(DigitalBrain::Landed))
    {
        if (info)
        {
            // RAS - 29Jan04 - This section changed because when you would do a touch and go, ATC would
            // still think you were on the ground.  I could not see any reason to leave
            // this the way it was.  If you are in this routine, then you are airborne because
            // not aircraft->OnGround() means you are flying.  if you are flying, then you should
            // be removed from the ATC list unless you call inbound again.

            // if(info->status == lLanded)
            // {
            // aircraft->DBrain()->ClearATCFlag(DigitalBrain::Landed);
            // }
            // else
            // {
            info->lastContacted = SimLibElapsedTime;
            info->status = tFlyOut;
            // info->timer = SimLibElapsedTime + 60 * CampaignSeconds;
            SendCmdMessage(aircraft, info);
            RemoveTraffic(info->aircraftID, PtHeaderDataTable[info->rwindex].runwayNum);
            // }
        }

        if ( not aircraft->DBrain()->IsSetATC(DigitalBrain::PermitTakeoff) and aircraft->DBrain()->IsSetATC(DigitalBrain::Landed))
        {
            aircraft->DBrain()->SetATCFlag(DigitalBrain::PermitTakeoff);
            float groundZ = OTWDriver.GetGroundLevel(aircraft->XPos(), aircraft->YPos());
            aircraft->FeatureCollision(groundZ);

            if (aircraft->onFlatFeature)
            {
                //took off without permission
                radioMessage = CreateCallFromATC(self, aircraft, rcTOWERSCOLD2, FalconLocalGame);
                radioMessage->dataBlock.edata[3] = 32767;
                FalconSendMessage(radioMessage, FALSE);
            }
        }

        if (SimDriver.GetPlayerEntity() == aircraft)
            gBumpFlag = FALSE;

        aircraft->DBrain()->ClearATCFlag(DigitalBrain::Landed);
        return TRUE;
    }

    return FALSE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ATCBrain::CheckFinalApproach(AircraftClass *aircraft, runwayQueueStruct *info)
{
    int curTaxiPoint, queue;
    float x, y, dx, dy, dist, cosAngle;

    queue = PtHeaderDataTable[info->rwindex].runwayNum;

    runwayQueueStruct *nextLand = NextToLand(queue);

    //curTaxiPoint = aircraft->DBrain()->GetTaxiPoint();
    curTaxiPoint = GetFirstPt(info->rwindex);

    TranslatePointData(self, curTaxiPoint, &x, &y);
    dx = x - aircraft->XPos();
    dy = y - aircraft->YPos();
    dist = (float)sqrt(dx * dx + dy * dy);

    dx /= dist;
    dy /= dist;

    cosAngle = dx * PtHeaderDataTable[info->rwindex].cosHeading +
                dy * PtHeaderDataTable[info->rwindex].sinHeading;

    if (dist > 500.0F and cosAngle > -0.7071F and cosAngle < 0.939692F)
    {
        info->status = lAborted;
        SendCmdMessage(aircraft, info);
    }
    else if (dist < 0.8F * aircraft->af->MinVcas() * KNOTS_TO_FTPSEC * 60.0F)
    {
        if (runwayStats[queue].rnwyInUse)
        {
            info->status = lAborted;
            SendCmdMessage(aircraft, info);
        }
        else if ( not aircraft->DBrain()->IsSetATC(DigitalBrain::ClearToLand) and nextLand == info)
        {
            aircraft->DBrain()->SetATCFlag(DigitalBrain::ClearToLand);
            info->status = lClearToLand;
            SendCmdMessage(aircraft, info);
            //info->status = lOnFinal;
        }
    }
    else if (info->next and info->next->schedTime + 15 * CampaignSeconds < SimLibElapsedTime and info->next->status == lOnFinal)
    {
        info->status = lAborted;
        SendCmdMessage(aircraft, info);
    }

    //Cobra let's have some fun.  Check for gear down, give warning, then abort them of they still don't have the gear down
    FalconRadioChatterMessage *radioMessage = NULL;

    if (aircraft->IsPlayer() and (dist < 12000.0f and dist > 6000.0f) and aircraft->af->gearPos < 0.5f)
    {
        //rcLANDINGCHECK
        //Cobra
        radioMessage = CreateCallFromATC(self, aircraft, rcLANDINGCHECK, FalconLocalSession);
        FalconSendMessage(radioMessage, FALSE);

    }
    else if (aircraft->IsPlayer() and dist < 6000.0f and aircraft->af->gearPos < 0.5f)
        //rcATCGOAROUND
    {
        info->status = lAborted;
        SendCmdMessage(aircraft, info);
    }

}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ATCBrain::CheckIfBlockingRunway(AircraftClass *aircraft, runwayQueueStruct* info)
{
    if (aircraft->af->IsSet(AirframeClass::GearBroken) or aircraft->af->Fuel() <= 0.0F)
        return FALSE;

    int queue;
    runwayQueueStruct *nextTakeoff = NULL;
    runwayQueueStruct *nextLand = NULL;
    runwayQueueStruct *nextOnRunway = NULL;
    int waitforlanding = FALSE;


    //check to see if on a runway
    int rwindex = IsOnRunway(aircraft);

    if (rwindex)
    {
        queue = GetQueue(rwindex);
        nextTakeoff = NextToTakeoff(queue);
        nextLand = NextToLand(queue);

        if (nextLand and SimLibElapsedTime + LAND_TIME_DELTA - 2 * CampaignSeconds > nextLand->schedTime)
            waitforlanding = TRUE;

        if ( not nextTakeoff or waitforlanding)
            nextOnRunway = nextLand;
        else if ( not nextLand or nextLand->schedTime > nextTakeoff->schedTime)
            nextOnRunway = nextTakeoff;
        else
            nextOnRunway = nextLand;
    }

    if (info and info->status == lLanded)
    {
        // OW: Jackals "scold-on-bounce" fix
#if 0
        if ( not rwindex)
        {
            info->status = lTaxiOff;
#else

        // JB 000421
        if ( not rwindex and aircraft->OnGround())
            // JB 000421
        {
            info->status = lTaxiOff;
#endif

            // info->timer = SimLibElapsedTime + 15 * CampaignSeconds;
        }
        else if (info->lastContacted + 90 * CampaignSeconds < SimLibElapsedTime and // 06FEB04 - FRB - was 45 seconds
                 (SimLibElapsedTime > LAND_TIME_DELTA + info->schedTime or
                  (rwindex not_eq info->rwindex and GetOppositeRunway(rwindex) not_eq info->rwindex)))
        {
            //yell at them to get off runway
            if ( not nextOnRunway)
                SendCallFromATC(self, aircraft, rcTAXICLEAR, FalconLocalGame);
            else if (nextOnRunway == nextTakeoff)
                SendCallFromATC(self, aircraft, rcTAXICLEAR, FalconLocalGame);
            else if (nextOnRunway == nextLand and nextOnRunway not_eq info)
                SendCallFromATC(self, aircraft, rcGETOFFRUNWAYA, FalconLocalGame);

            info->lastContacted = SimLibElapsedTime;
            return TRUE;
        }
    }
    else if (rwindex and not info and aircraft->DBrain()->WaitTime() + 90 * CampaignSeconds < SimLibElapsedTime)  // 06FEB04 - FRB - was 45 seconds
    {
        //yell at them to get off runway
        if ( not nextOnRunway)
            SendCallFromATC(self, aircraft, rcTAXICLEAR, FalconLocalGame);
        else if (nextOnRunway == nextTakeoff)
            SendCallFromATC(self, aircraft, rcTAXICLEAR, FalconLocalGame);
        else if (nextOnRunway == nextLand)
            SendCallFromATC(self, aircraft, rcGETOFFRUNWAYA, FalconLocalGame);

        aircraft->DBrain()->SetWaitTimer(SimLibElapsedTime);
        return TRUE;
    }
    else if (rwindex and info and info->lastContacted + 90 * CampaignSeconds < SimLibElapsedTime)  // 06FEB04 - FRB - was 45 seconds
    {
        //yell at them to get off runway
        if ( not nextOnRunway)
            SendCallFromATC(self, aircraft, rcTAXICLEAR, FalconLocalGame);
        else if (nextOnRunway == nextTakeoff and nextOnRunway not_eq info)
            SendCallFromATC(self, aircraft, rcTAXICLEAR, FalconLocalGame);
        else if (nextOnRunway == nextLand and nextOnRunway not_eq info)
            SendCallFromATC(self, aircraft, rcGETOFFRUNWAYA, FalconLocalGame);

        info->lastContacted = SimLibElapsedTime;
        return TRUE;
    }

    return FALSE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ATCBrain::FindAbortPt(AircraftClass* aircraft, float *x, float *y, float *z)
{
    float abortHeading;
    mlTrig trig;

    abortHeading = aircraft->Yaw() + 90.0F * DTR;

    mlSinCos(&trig, abortHeading);

    *x = self->XPos() + trig.cos * 8.0F * NM_TO_FT;
    *y = self->YPos() + trig.sin * 8.0F * NM_TO_FT;
    *z = GetAltitude(aircraft, lAborted);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ATCBrain::RemoveFromAllOtherATCs(AircraftClass *aircraft)
{
    ObjectiveClass* curObj;
    runwayQueueStruct *info;

    VuListIterator findWalker(SimDriver.atcList);
    curObj = (ObjectiveClass*)findWalker.GetFirst();

    while (curObj)
    {
        if (curObj and curObj not_eq self and curObj->GetType() == TYPE_AIRBASE)
        {
            info = curObj->brain->InList(aircraft->Id());

            if (info)
            {
                if (info->rwindex)
                    curObj->brain->RemoveTraffic(aircraft->Id(), PtHeaderDataTable[info->rwindex].runwayNum);
                else
                    curObj->brain->RemoveInbound(info);

                return;
            }
        }

        curObj = (ObjectiveClass*)findWalker.GetNext();
    }

}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ATCBrain::RemoveFromAllATCs(AircraftClass *aircraft)
{
    ObjectiveClass* curObj;
    runwayQueueStruct *info;

    VuListIterator findWalker(SimDriver.atcList);
    curObj = (ObjectiveClass*)findWalker.GetFirst();

    while (curObj)
    {
        if (curObj and curObj->GetType() == TYPE_AIRBASE)
        {
            info = curObj->brain->InList(aircraft->Id());

            if (info)
            {
                if (info->rwindex)
                    curObj->brain->RemoveTraffic(aircraft->Id(), PtHeaderDataTable[info->rwindex].runwayNum);
                else
                    curObj->brain->RemoveInbound(info);

                return;
            }
        }

        curObj = (ObjectiveClass*)findWalker.GetNext();
    }

}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ATCBrain::UseSectionTakeoff(FlightClass *flight, int rwindex)
{
    //TJL 10/31/03 Changing to 40 from 80
    if (runwayStats[PtHeaderDataTable[rwindex].runwayNum].halfwidth > 40.0F and 
        (flight->GetSType() == STYPE_UNIT_ATTACK or
         flight->GetSType() == STYPE_UNIT_FIGHTER or
         flight->GetSType() == STYPE_UNIT_FIGHTER_BOMBER) and 
        flight->GetTotalVehicles() > 1)
        return TRUE;

    return FALSE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ATCBrain::CheckList(runwayQueueStruct *list)
{
    //it seems to be ok, so I'm going to stop this for now.
#if 0
    runwayQueueStruct *prev = NULL;
    runwayQueueStruct *cirCheck = NULL;
    int count;

#ifdef DEBUG
    /* if(GetCurrentThreadId() not_eq gSimThreadID)
     {
     ShiAssert( not "Tell Dave Power you hit the ATC assert (x4373) Don't ignore this");
     ShiAssert( not "Tell Dave Power you hit the ATC assert (x4373) Don't ignore this");
     //*((unsigned int *) 0x00) = 0; //told you not to ignore it
     }*/

    while (list)
    {
#ifdef TEST_HACK_THAT_LEAKS

        if (list->deleteLine)
#else
        if ((int)list->next == 0xdddddddd)
#endif
        {
            ShiAssert( not "Tell Dave Power you hit the ATC assert (x4373)");
            ShiAssert( not "Tell Dave Power you hit the ATC assert (x4373) Don't ignore this");
            ShiAssert( not "Tell Dave Power you hit the ATC assert (x4373) Don't ignore this");

            //*((unsigned int *) 0x00) = 0; //told you not to ignore it
            if (prev)
                prev->next = NULL;
        }

        count = 1;
        cirCheck = list;

        while (cirCheck and count)
        {
            ShiAssert(cirCheck->next not_eq list);

            if (cirCheck->next == list)
                cirCheck->next = NULL;

            cirCheck = cirCheck->next;
            count = count++ % 100;
        }

        prev = list;
        list = list->next;
    }

#else

    //big time DSP hack
    while (list)
    {
#ifdef TEST_HACK_THAT_LEAKS

        if (list->deleteLine)
#else
        if ((int)list->next == 0xdddddddd)
#endif
        {
            //for now i'll just fix it
            if (prev)
                prev->next = NULL;
        }

        count = 1;
        cirCheck = list;

        while (cirCheck and count)
        {
            //for now i'll just fix it
            if (cirCheck->next == list)
                cirCheck->next = NULL;

            cirCheck = cirCheck->next;
            count = count++ % 100;
        }

        prev = list;
        list = list->next;
    }

#endif
#else
    list;
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ulong ATCBrain::RemovePlaceHolders(VU_ID id)
{
    runwayQueueStruct *info = NULL;
    runwayQueueStruct *deleteInfo = NULL;
    ulong takeoffTime = 0;
    int i;

    for (i = 0; i < numRwys; i++)
    {
        info = runwayQueue[i];

        while (info)
        {
            deleteInfo = info;
            info = info->next;

            if (deleteInfo->aircraftID == id)
            {
                if ( not takeoffTime)
                    takeoffTime = deleteInfo->schedTime;

                RemoveTraffic(id, i);
            }
        }
    }

    return takeoffTime;
}


// RAS - 16Jan04 -Check for Traffic Call
void ATCBrain::CheckForTraffic(AircraftClass *aircraft, runwayQueueStruct *playerInfo)
{
    // check 'checkTrafficTime'; if not time, skip traffic routine:  check every 10 seconds
    if (self->brain->checkTrafficTime < SimLibElapsedTime)
    {
        self->brain->checkTrafficTime = SimLibElapsedTime + 10 * CampaignSeconds;

        //setup variables
        FalconRadioChatterMessage *radioMessage = NULL;
        float altitude = 0.0F; // altitude of traffic
        float oldTrafficRange = 0.0F; // hold range of last traffic call
        float xdiff; // difference in x coord between us and traffic
        float ydiff; // difference in y coord between us and traffic
        float angle; // computed angle in radians to traffic
        int navangle; // computed degrees off nose


        if (self->brain->pLastTraffic not_eq NULL)
            self->brain->oldTrafficRange = self->brain->trafficRange; //store range of last traffic call

        // Check for Traffic
        SimBaseClass *traffic = SimDriver.FindNearestTraffic(aircraft, self, &altitude);

        if (traffic == NULL)
            self->brain->trafficCheck = noTraffic; //if no traffic set status to none

        switch (self->brain->trafficCheck)
        {
            case noTraffic:
                self->brain->trafficInSightFlag = FALSE; // no traffic to see
                break;

            case priorityTraffic: // not used right now, but may use it later
            case newTraffic: // traffic not called out last time
                radioMessage = CreateCallFromATC(self, aircraft, rcATCTRAFFICWARNING, FalconLocalGame);
                self->brain->trafficInSightFlag = FALSE; // not in sight since it's first call
                break;

            case oldTraffic: // Same traffic called last time
                radioMessage = CreateCallFromATC(self, aircraft, rcATCTRAFFICWARNING2, FalconLocalGame);
                break;

            default:
                //we should never get here
                ShiWarning("We are in an undefined traffic call state, we shouldn't be here");
        }

        // Traffic in sight and this traffic has been called out before so return
        if (self->brain->trafficInSightFlag == TRUE and traffic == self->brain->pLastTraffic)
            return;


        // Begin O'Clock code - convert to compass angle
        if (self->brain->trafficCheck not_eq noTraffic)
        {
            xdiff = traffic->XPos() - aircraft->XPos();
            ydiff = traffic->YPos() - aircraft->YPos();

            angle = (float)atan2(ydiff, xdiff);
            angle = angle - aircraft->Yaw();
            navangle =  FloatToInt32(RTD * angle);

            if (navangle < 0)
                navangle = 360 + navangle;


            radioMessage->dataBlock.edata[3] = navangle / 30; // scale compass angle for radio eData

            if (radioMessage->dataBlock.edata[3] >= 12)
                radioMessage->dataBlock.edata[3] = 0;

            //eRangeLast
            radioMessage->dataBlock.edata[4] = SimToGrid(sqrt(self->brain->trafficRange));
            radioMessage->dataBlock.time_to_play = 0;

            FalconSendMessage(radioMessage, FALSE);  //Sends Message to player
            self->brain->lastTrafficCallTime = SimLibElapsedTime;
            playerInfo->lastContacted = SimLibElapsedTime;
        }
    }
}
