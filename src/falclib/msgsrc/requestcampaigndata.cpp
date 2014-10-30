#include "MsgInc/RequestCampaignData.h"
#include "MsgInc/SendObjData.h"
#include "MsgInc/SendUnitData.h"
#include "MsgInc/SendVCMsg.h"
#include "MsgInc/SendCampaignMsg.h"
#include "MsgInc/SendPersistantList.h"
#include "mesg.h"
#include "Campaign.h"
#include "dogfight.h"
#include "falclib.h"
#include "falcmesg.h"
#include "falcgame.h"
#include "falcsess.h"
#include "Cmpclass.h"
#include "weather.h"
#include "falcuser.h"
#include "ui95/chandler.h"
#include "team.h"
#include "persist.h"
#include "InvalidBufferException.h"

extern C_Handler *gMainHandler;
extern int F4VuMaxTCPMessageSize;
extern ulong gResendEvalRequestTime;
extern int EncodeObjectiveDeltas(VU_BYTE **stream);
extern int DecodeObjectiveDeltas(VU_BYTE **stream, long *rem);
extern void SendVCData(FalconSessionEntity *);
extern void ResyncTimes();
extern int CheckNumberPlayers(void);
extern void RequestEvalData(void);
extern void SendPrimaryObjectiveList(uchar teammask);

static int MatchPlayStarted(void);

FalconRequestCampaignData *gRequestQueue = NULL;

FalconRequestCampaignData::FalconRequestCampaignData(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) : FalconEvent(RequestCampaignData, FalconEvent::CampaignThread, entityId, target, loopback)
{
    RequestOutOfBandTransmit();
    nextRequest = 0;
    dataBlock.size = 0;
    dataBlock.data = NULL;
}

FalconRequestCampaignData::FalconRequestCampaignData(VU_MSG_TYPE type, VU_ID senderid, VU_ID target) : FalconEvent(RequestCampaignData, FalconEvent::CampaignThread, senderid, target)
{
    nextRequest = 0;
    dataBlock.size = 0;
    dataBlock.data = NULL;
    type;
}

FalconRequestCampaignData::~FalconRequestCampaignData(void)
{
    if (dataBlock.data)
        delete dataBlock.data;

    dataBlock.data = NULL;
}

int FalconRequestCampaignData::Size(void) const
{
    ShiAssert(dataBlock.size >= 0);
    return FalconEvent::Size() + sizeof(VU_ID) + sizeof(ulong) + sizeof(uchar) + dataBlock.size;
}

int FalconRequestCampaignData::Decode(VU_BYTE **buf, long *rem)
{
    long init = *rem;

    FalconEvent::Decode(buf, rem);

    memcpychk(&dataBlock.who, buf, sizeof(VU_ID), rem);
    memcpychk(&dataBlock.dataNeeded, buf, sizeof(ulong), rem);
    memcpychk(&dataBlock.size, buf, sizeof(uchar), rem);
    ShiAssert(dataBlock.size >= 0);

    if (dataBlock.size > 0)
    {
        dataBlock.data = new uchar[dataBlock.size];
        memcpychk(dataBlock.data, buf, dataBlock.size, rem);
    }

    // ShiAssert ( size == Size() );

    return init  - *rem;
}

int FalconRequestCampaignData::Encode(VU_BYTE **buf)
{
    int size = 0;

    size += FalconEvent::Encode(buf);

    ShiAssert(dataBlock.size >= 0);
    memcpy(*buf, &dataBlock.who, sizeof(VU_ID));
    *buf += sizeof(VU_ID);
    size += sizeof(VU_ID);
    memcpy(*buf, &dataBlock.dataNeeded, sizeof(ulong));
    *buf += sizeof(ulong);
    size += sizeof(ulong);
    memcpy(*buf, &dataBlock.size, sizeof(uchar));
    *buf += sizeof(uchar);
    size += sizeof(uchar);

    if (dataBlock.size > 0 and dataBlock.data)
    {
        memcpy(*buf, dataBlock.data, dataBlock.size);
        *buf += dataBlock.size;
        size += dataBlock.size;
    }

    ShiAssert(size == Size());

    return size;
}

int FalconRequestCampaignData::Process(uchar autodisp)
{
    if (autodisp)
        return -1;

    MonoPrint("Process %08x\n", dataBlock.dataNeeded);

    if (dataBlock.dataNeeded bitand CAMP_GAME_FULL)
    {
        MonoPrint("Bang Crash Whollop\n");

        if (gMainHandler)
            PostMessage(gMainHandler->GetAppWnd(), FM_GAME_FULL, 0, 0);
    }

    if (dataBlock.dataNeeded bitand DF_MATCH_IN_PROGRESS)
    {
        MonoPrint("Whollop Crash Bang\n");

        if (gMainHandler)
            PostMessage(gMainHandler->GetAppWnd(), FM_MATCH_IN_PROGRESS, 0, 0);
    }

    if ( not TheCampaign.IsLoaded())
        return -1;

    // KCK TODO: Check if a request from this machine is already on the queue,
    // and toss, if so.

    // Reference the message and put it on our request list.
    Ref();

    nextRequest = gRequestQueue;
    gRequestQueue = this;

    return 1;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// ===============================================================================
// This function is called from the campaign thread, and actually does the sending
// ===============================================================================

void SendRequestedData(void)
{
    int sent = 0;

    CampEnterCriticalSection();

    for (FalconRequestCampaignData *request = gRequestQueue; request not_eq NULL; request = gRequestQueue)
    {
        VU_BYTE *buf;
        uchar *dataptr = request->dataBlock.data;
        FalconSessionEntity *requester = (FalconSessionEntity*)vuDatabase->Find(request->dataBlock.who);

        if ((requester not_eq NULL) and TheCampaign.IsLoaded())
        {
            TheCampaign.SetOnlineStatus(1);

            if (( not (request->dataBlock.dataNeeded bitand CAMP_NEED_PRELOAD)) and (CheckNumberPlayers() < 0))
            {
                FalconRequestCampaignData *msg;
                MonoPrint("Too Many Players");
                msg = new FalconRequestCampaignData(requester->Id(), requester);
                msg->dataBlock.who = vuLocalSessionEntity->Id();
                msg->dataBlock.dataNeeded = CAMP_GAME_FULL;
                FalconSendMessage(msg, TRUE);
            }
            else if (( not (request->dataBlock.dataNeeded bitand CAMP_NEED_PRELOAD)) and (MatchPlayStarted()))
            {
                FalconRequestCampaignData *msg;
                MonoPrint("Send Match Play In Progress");
                msg = new FalconRequestCampaignData(requester->Id(), requester);
                msg->dataBlock.who = vuLocalSessionEntity->Id();
                msg->dataBlock.dataNeeded = DF_MATCH_IN_PROGRESS;
                FalconSendMessage(msg, TRUE);
            }
            else
            {
                MonoPrint("Data Needed %08x\n", request->dataBlock.dataNeeded);

                // Send back what was requested:
                if (request->dataBlock.dataNeeded bitand CAMP_NEED_PRELOAD)
                {
                    MonoPrint("Sending Preload\n");
                    FalconSendCampaign* msg = new FalconSendCampaign(request->dataBlock.who, requester);
                    msg->dataBlock.campTime = Camp_GetCurrentTime();
                    msg->dataBlock.from = vuLocalSessionEntity->Id();
                    msg->RequestOutOfBandTransmit();
                    FalconSendMessage(msg, TRUE);
                }

                if (request->dataBlock.dataNeeded bitand CAMP_NEED_WEATHER)
                {
                    MonoPrint("Sending Weather\n");
                    ((WeatherClass*)realWeather)->SendWeather(requester);
                }

                if (request->dataBlock.dataNeeded bitand CAMP_NEED_PERSIST)
                {
                    MonoPrint("Sending Persist\n");
                    FalconSendPersistantList* msg = new FalconSendPersistantList(request->dataBlock.who, requester);
                    int maxSize = F4VuMaxTCPMessageSize - sizeof(FalconSendPersistantList);
                    msg->dataBlock.size = (short)SizePersistantList(maxSize);
                    msg->dataBlock.data = new VU_BYTE[msg->dataBlock.size];
                    buf = (VU_BYTE*) msg->dataBlock.data;
                    EncodePersistantList(&buf, maxSize);
                    FalconSendMessage(msg, TRUE);
                }

                if (request->dataBlock.dataNeeded bitand CAMP_NEED_OBJ_DELTAS)
                {
                    MonoPrint("Sending Obj Deltas\n");
                    SendObjectiveDeltas(requester, requester, dataptr);

                    if (dataptr)
                    {
                        dataptr += FS_MAXBLK / 8;
                    }
                }
                else if (requester->objDataSendBuffer)
                {
                    MonoPrint("Sending Obj Data\n");
                    delete requester->objDataSendBuffer;
                    requester->objDataSendBuffer = NULL;
                    requester->objDataSendSet = 0;
                }

                if (request->dataBlock.dataNeeded bitand CAMP_NEED_UNIT_DATA)
                {
                    MonoPrint("Sending Unit Data\n");
                    SendCampaignUnitData(requester, requester, dataptr);

                    if (dataptr)
                    {
                        dataptr += FS_MAXBLK / 8;
                    }
                }
                else if (requester->unitDataSendBuffer)
                {
                    MonoPrint("Sending Unit Data\n");
                    delete requester->unitDataSendBuffer;
                    requester->unitDataSendBuffer = NULL;
                    requester->unitDataSendSet = 0;
                }

                if (request->dataBlock.dataNeeded bitand CAMP_NEED_ENTITIES and request->dataBlock.who not_eq vuLocalSession)
                {
                    MonoPrint("Sending Entity Data\n");
                    // KCK: I don't think there's anything we need here -
                    // Non-weapon sim entities are sent with the campaign data.
                    // We could send weapons, I suppose...
                    //
                    /* // We really only want to send owned non campaign entities
                     VuMessage *resp = 0;
                     VuEntity *ent = NULL;
                     VuDatabaseIterator iter;
                     ent = iter.GetFirst();
                     while (ent)
                     {
                     if ( not ent->IsPrivate() and ent->IsLocal() and (ent->EntityType())->classInfo_[VU_CLASS] not_eq CLASS_OBJECTIVE and (ent->EntityType())->classInfo_[VU_CLASS] not_eq CLASS_UNIT and (ent->EntityType())->classInfo_[VU_CLASS] not_eq CLASS_MANAGER)
                     {
                     resp = new VuFullUpdateEvent(ent, requester);
                     resp->RequestReliableTransmit();
                     VuMessageQueue::PostVuMessage(resp);
                     }
                     ent = iter.GetNext();
                     }
                    */
                }

                if (request->dataBlock.dataNeeded bitand CAMP_NEED_TEAM_DATA and request->dataBlock.who not_eq vuLocalSession)
                {
                    MonoPrint("Sending Team Data\n");

                    // We really only want to send manager entities and team entities
                    for (int t = 0; t < NUM_TEAMS; t++)
                    {
                        if (TeamInfo[t] and TeamInfo[t]->IsLocal())
                        {
                            TeamInfo[t]->DoFullUpdate(requester);
                        }
                    }
                }

                if (request->dataBlock.dataNeeded bitand CAMP_NEED_VC and request->dataBlock.who not_eq vuLocalSession)
                {
                    MonoPrint("Sending VC Data\n");
                    SendVCData(requester);
                }

                if (request->dataBlock.dataNeeded bitand CAMP_NEED_PRIORITIES)
                {
                    MonoPrint("Sending Priorities\n");
                    // Send priorities for all teams
                    SendPrimaryObjectiveList(0);
                }
            }
        }

        sent++;
        gRequestQueue = request->nextRequest;
        request->UnRef();
    }

    if (sent)
    {
        // Force time resync, just for good measure.
        ResyncTimes(/*TRUE*/);
    }

    // Resend any pending eval requests
    if (gResendEvalRequestTime > vuxRealTime)
    {
        RequestEvalData();
        gResendEvalRequestTime = 0;
    }

    CampLeaveCriticalSection();
}

// Check if this is a Dogfight game and match play is in progress
int MatchPlayStarted(void)
{
    if (
        (FalconLocalGame->GetGameType() == game_Dogfight) and 
        (SimDogfight.GetGameType() == dog_TeamMatchplay) and 
        SimDogfight.GameStarted()
    )
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}
