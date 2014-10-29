/** @file vu_thread.cpp vu thread implementation */

#include "vu_priv.h"
#include "vu_thread.h"
#include "vuevent.h"
#include "vu_mq.h"

extern VuMainThread *vuMainThread;

// filters used

// static namespace
namespace
{
    /** used by filters. */
    int compareEnts(VuEntity *ent1, VuEntity *ent2)
    {
        if ((VU_KEY)ent2->Id() > (VU_KEY)ent1->Id())
        {
            return -1;
        }
        else if ((VU_KEY)ent2->Id() < (VU_KEY)ent1->Id())
        {
            return 1;
        }

        return 0;
    }
}

/** only games are accepted. */
class VuGameFilter : public VuFilter
{
public:
    VuGameFilter() : VuFilter() {}
    virtual ~VuGameFilter() {}
    virtual VU_BOOL Test(VuEntity *ent)
    {
        return ent->IsGame();
    }
    virtual VU_BOOL RemoveTest(VuEntity *ent)
    {
        return ent->IsGame();
    }
    virtual int Compare(VuEntity *ent1, VuEntity *ent2)
    {
        return compareEnts(ent1, ent2);
    }
    virtual VuFilter *Copy()
    {
        return new VuGameFilter(this);
    }
protected:
    VuGameFilter(VuGameFilter* other) : VuFilter(other) {}
};

/** only VuTargetEntity is accepted. */
class VuTargetFilter : public VuFilter
{
public:
    VuTargetFilter() : VuFilter() {}
    virtual ~VuTargetFilter() {}
    virtual VU_BOOL Test(VuEntity* ent)
    {
        return ent->IsTarget();
    }
    virtual VU_BOOL RemoveTest(VuEntity* ent)
    {
        return ent->IsTarget();
    }
    virtual int Compare(VuEntity* ent1, VuEntity* ent2)
    {
        return compareEnts(ent1, ent2);
    }
    virtual VuFilter* Copy()
    {
        return new VuTargetFilter(this);
    }

protected:
    VuTargetFilter(VuTargetFilter* other) : VuFilter(other) {}
};

//=============
// VuBaseThread
//=============

VuBaseThread::VuBaseThread(VuMessageQueue *messageQueue) : messageQueue_(messageQueue)
{
}

VuBaseThread::~VuBaseThread()
{
    if (messageQueue_)
    {
        messageQueue_->DispatchMessages(-1, TRUE);   // flush queue
        delete messageQueue_;
        messageQueue_ = 0;
    }
}

#if CAP_DISPATCH
void VuBaseThread::Update(int mxTime)
{
    DWORD limit = mxTime > 0 ? mxTime : INFINITE;
    DWORD start = GetTickCount();
    bool timeRemaining = true; //na we keep going?
    int nm; // number of messages per dispatch

    do
    {
        // process 10 messages and check time again
        nm = messageQueue_->DispatchMessages(10, FALSE);
        DWORD now = GetTickCount();
        timeRemaining = now - start > limit ? false : true;
    }
    while (nm and timeRemaining);
}
#else
void VuBaseThread::Update()
{
    messageQueue_->DispatchMessages(-1, FALSE); // process all messages from queue
}
#endif

//=========
// VuThread
//=========

VuThread::VuThread(VuMessageFilter* filter, int queueSize) : VuBaseThread(
#if VU_USE_ENUM_FOR_TYPES
        new VuMessageQueue(queueSize, filter)
#else
        (filter) ?
        new VuMessageQueue(queueSize, filter) :
        new VuMessageQueue(queueSize, &VuStandardMsgFilter())
#endif
    )
{
}

VuThread::~VuThread()
{
}

/*
VuThread::VuThread(int queueSize) : VuBaseThread(){
#if VU_USE_ENUM_FOR_TYPES
 messageQueue_ = new VuMessageQueue(queueSize, NULL);
#else
 VuStandardMsgFilter smf;
 messageQueue_ = new VuMessageQueue(queueSize, &smf);
#endif
}
*/


//=============
// VuMainThread
//=============
VuMainThread::VuMainThread(
    int dbSize,
    VuMessageFilter *filter,
    int queueSize,
    VuSessionEntity * (*sessionCtorFunc)(void)
) : VuBaseThread(
#if VU_USE_ENUM_FOR_TYPES
        new VuMainMessageQueue(queueSize, filter)
#else
        filter ?
        new VuMainMessageQueue(queueSize, filter) :
        new VuMainMessageQueue(queueSize, &VuStandardMsgFilter())
#endif
    )
{
    if (vuCollectionManager or vuDatabase)
    {
        VU_PRINT("VU: Warning:  creating second VuMainThread\n");
    }
    else
    {
        Init(dbSize, sessionCtorFunc);
    }
}

// called once during game initialization
void VuMainThread::Init(int dbSize, VuSessionEntity * (*sessionCtorFunc)(void))
{
    // set global, for sneaky internal use...
    vuMainThread = this;

    sendQueue_ = NULL;

    vuCollectionManager = new VuCollectionManager();
    vuDatabase          = new VuDatabase(dbSize);  // create global database

    VuGameFilter gfilter;
#if VU_ALL_FILTERED
    vuGameList = new VuLinkedList(&gfilter);
#else
    vuGameList = new VuOrderedList(&gfilter);
#endif
    vuGameList->Register();

    VuTargetFilter tfilter;
#if VU_ALL_FILTERED
    vuTargetList = new VuLinkedList(&tfilter);
#else
    vuTargetList = new VuFilteredList(&tfilter);
#endif
    vuTargetList->Register();

    // create global group
    vuGlobalGroup = new VuGlobalGroup();
    vuDatabase->/*Quick*/Insert(vuGlobalGroup);
    vuPlayerPoolGroup = 0;

    // create local session
    if (sessionCtorFunc)
    {
        vuLocalSessionEntity.reset(sessionCtorFunc());
    }
    else
    {
        vuLocalSessionEntity.reset(new VuSessionEntity(vuxLocalDomain, "player"));
    }

    vuLocalSession = vuLocalSessionEntity->OwnerId();
    vuLocalSessionEntity->SetSendCreate(VuEntity::VU_SC_SEND_OOB);
    vuDatabase->/*Quick*/Insert(vuLocalSessionEntity.get());
}

VuMainThread::~VuMainThread()
{
    // KCK Added. Probably a good idea
    JoinGame(vuPlayerPoolGroup);

    // must do this prior to destruction of database (below)
    messageQueue_->DispatchMessages(-1, TRUE);   // flush queue
    delete messageQueue_;
    messageQueue_ = 0;
    vuGameList->Unregister();
    delete vuGameList;
    vuGameList = 0;
    vuTargetList->Unregister();
    delete vuTargetList;
    vuTargetList = 0;

    VuReferenceEntity(vuGlobalGroup);
    VuReferenceEntity(vuPlayerPoolGroup);
    vuDatabase->Purge();

    delete vuDatabase;
    vuDatabase = 0;

    VuDeReferenceEntity(vuGlobalGroup);
    VuDeReferenceEntity(vuPlayerPoolGroup);
    vuGlobalGroup     = 0;
    vuPlayerPoolGroup = 0;

    delete vuCollectionManager;
    vuCollectionManager = 0;

    vuLocalSessionEntity.reset();
    vuMainThread = 0;
}



#if CAP_DISPATCH
void VuMainThread::Update(int mxTime)
#else
void VuMainThread::Update()
#endif
{
#define NEW_MT_UPDATE 1
#if NEW_MT_UPDATE

    VuGameEntity *game = vuLocalSessionEntity->Game();

    // send and get all messages
    vuTransmitTime = vuxRealTime;

    //START_PROFILE("RT_UPDATE_PU");
    if (game not_eq NULL)
    {
        // sfr: send enqueued position updates, before garbage collector to avoid invalid pointers
        // computes sends allowed and iterate over sessions
        unsigned int allowed = VuMaster::SendsPerPlayer();

        if (vuLocalGame not_eq NULL)
        {
            VuSessionsIterator sit(vuLocalGame);

            for (VuSessionEntity *se = sit.GetFirst(); se not_eq NULL; se = sit.GetNext())
            {
                if (se not_eq vuLocalSessionEntity.get())
                {
                    se->SendBestEnqueuedPositionUpdatesAndClear(allowed, vuxGameTime);
                }
            }
        }
    }

    //STOP_PROFILE("RT_UPDATE_PU");

    // clear garbage collector
    //START_PROFILE("RT_GC");
    vuCollectionManager->CreateEntitiesAndRunGc();
    //STOP_PROFILE("RT_GC");

    //REPORT_VALUE("messages", nm);
    //extern int nmsgs;
    //REPORT_VALUE("message diff", nmsgs);
    //extern int nSimBase;
    //REPORT_VALUE("sim base", nSimBase);
    //extern DWORD SimObjects;
    //REPORT_VALUE("sim obj", SimObjects);

    // if no game, do dispatch and quite
    if ( not game)
    {
#if CAP_DISPATCH

        // send 10 queued messages at most
        if (sendQueue_)
        {
            sendQueue_->DispatchMessages(10, FALSE);
        }

        // 10 ms at most
        VuBaseThread::Update(10);
#else

        if (sendQueue_)
        {
            sendQueue_->DispatchMessages(-1, FALSE);
        }

        messageQueue_->DispatchMessages(-1, FALSE);
#endif
        return;
    }

    //START_PROFILE("VUMAINUP_GROUP");
    // Send low priority messages
    //if (vuLowSendQueue){
    // vuLowSendQueue->DispatchMessages(-1, FALSE);
    //}

    // our session is dirty, send update
    if (vuLocalSessionEntity->IsDirty())
    {
        VuFullUpdateEvent *msg = new VuFullUpdateEvent(vuLocalSessionEntity.get(), vuGlobalGroup);
        msg->RequestOutOfBandTransmit();
        msg->RequestReliableTransmit();
        VuMessageQueue::PostVuMessage(msg);
        vuLocalSessionEntity->ClearDirty();
    }

    // send session and game info every interval
    // time variables
    VU_TIME now = vuxRealTime;  ///< time now
    static VU_TIME last_bg = 0; ///< last broadcast time

    if (now - last_bg > 5000)
    {
        last_bg = now;
        vuLocalSessionEntity->SetTransmissionTime(vuxRealTime);
        VuBroadcastGlobalEvent *msg = new VuBroadcastGlobalEvent(vuLocalSessionEntity.get(), vuGlobalGroup);
        msg->RequestOutOfBandTransmit();
        VuMessageQueue::PostVuMessage(msg);
        vuLocalSessionEntity->SetTransmissionTime(vuxRealTime);
        UpdateGroupData(vuGlobalGroup);
        VuListIterator grp_iter(vuGameList);

        for (
            VuGameEntity* game = (VuGameEntity*)grp_iter.GetFirst(), *nextGame;
            game not_eq NULL;
            game = nextGame
        )
        {
            nextGame = (VuGameEntity*)grp_iter.GetNext();

            // removes empty games that are not player pool
            if (game->IsLocal() and (game not_eq vuPlayerPoolGroup) and (game->SessionCount() == 0))
            {
                vuDatabase->Remove(game);
            }

            // broadcast game data if its our game
            if (game->IsLocal() and ((game->LastTransmissionTime() + game->UpdateRate()) < vuxRealTime))
            {
                if (game->IsDirty())
                {
                    // game dirty, update to everyone
                    VuFullUpdateEvent *msg = new VuFullUpdateEvent(game, vuGlobalGroup);
                    msg->RequestReliableTransmit();
                    msg->RequestOutOfBandTransmit();
                    VuMessageQueue::PostVuMessage(msg);
                    game->ClearDirty();
                }
                else
                {
                    // nothing changed, keep game alive
                    VuBroadcastGlobalEvent *msg = new VuBroadcastGlobalEvent(game, vuGlobalGroup);
                    msg->RequestOutOfBandTransmit();
                    VuMessageQueue::PostVuMessage(msg);
                }

                game->SetTransmissionTime(vuxRealTime);
            }

            UpdateGroupData(game);
        }
    }

    //STOP_PROFILE("VUMAINUP_GROUP");

    //START_PROFILE("VUMAINUP_SENDDISPATCH");

    if (sendQueue_)
    {
#if CAP_DISPATCH
        // dispatch 10 queued messages at most before sending remotes
        sendQueue_->DispatchMessages(10, FALSE);
#else
        sendQueue_->DispatchMessages(-1, FALSE);
#endif
    }

    // send to remotes and
    SendQueuedMessages();
    //STOP_PROFILE("VUMAINUP_SENDDISPATCH");
    //START_PROFILE("VUMAINUP_GETDISPATCH");
    // get from remotes
    GetMessages();

#if CAP_DISPATCH
    // 10 ms at most
    VuBaseThread::Update(mxTime);
#else
    messageQueue_->DispatchMessages(-1, FALSE);    // flush queue
#endif

    //STOP_PROFILE("VUMAINUP_GETDISPATCH");

#else
    // clear garbage collector
    vuCollectionManager->CreateEntitiesAndFlushGc();

    // zero transmitted bytes
    //ResetXmit();
    // dispatch messages
    messageQueue_->DispatchMessages(-1, FALSE);

    if (vuNormalSendQueue)
    {
        vuNormalSendQueue->DispatchMessages(-1, FALSE);
    }

    // if no game, nothing else
    VuGameEntity  *game = vuLocalSessionEntity->Game();

    if ( not game)
    {
        return;
    }

    // Send low priority messages
    if (vuLowSendQueue)
    {
        vuLowSendQueue->DispatchMessages(-1, FALSE);
    }

    VuEnterCriticalSection();
    SendQueuedMessages();
    GetMessages();
    VuExitCriticalSection();

    messageQueue_->DispatchMessages(-1, FALSE);    // flush queue
    vuTransmitTime = vuxRealTime;

    // sfr: send enqeued position updates (the ones not sent immediately)
    // compute sends allowed
    // iterate sessions
    unsigned int allowed = VuMaster::SendsPerPlayer(); //TODO

    if (vuLocalGame not_eq NULL)
    {
        VuSessionsIterator sit(vuLocalGame);

        for (VuSessionEntity *se = sit.GetFirst(); se not_eq NULL; se = sit.GetNext())
        {
            if (se not_eq vuLocalSessionEntity.get())
            {
                se->SendBestEnqueuedPositionUpdatesAndClear(allowed, vuxGameTime);
            }
        }
    }

    // our session is dirty, send update
    if (vuLocalSessionEntity->IsDirty())
    {
        VuFullUpdateEvent *msg = new VuFullUpdateEvent(vuLocalSessionEntity.get(), vuGlobalGroup);
        msg->RequestOutOfBandTransmit();
        msg->RequestReliableTransmit();
        VuMessageQueue::PostVuMessage(msg);
        vuLocalSessionEntity->ClearDirty();
    }

    // send session and game info every interval
    // time variables
    VU_TIME now = vuxRealTime;  ///< time now
    static VU_TIME last_bg = 0; ///< last broadcast time

    if (now - last_bg > 5000)
    {
        last_bg = now;
        vuLocalSessionEntity->SetTransmissionTime(vuxRealTime);
        VuBroadcastGlobalEvent *msg = new VuBroadcastGlobalEvent(vuLocalSessionEntity.get(), vuGlobalGroup);
        msg->RequestOutOfBandTransmit();
        VuMessageQueue::PostVuMessage(msg);
        vuLocalSessionEntity->SetTransmissionTime(vuxRealTime);
        UpdateGroupData(vuGlobalGroup);
        VuListIterator grp_iter(vuGameList);

        // sfr: removed loop increment from the for, since remove can kill it
        for (
            VuGameEntity* game = (VuGameEntity*)grp_iter.GetFirst(), *nextGame;
            game not_eq NULL;
            game = nextGame
        )
        {
            nextGame = (VuGameEntity*)grp_iter.GetNext();

            // removes empty games that are not player pool
            if (game->IsLocal() and (game not_eq vuPlayerPoolGroup) and (game->SessionCount() == 0))
            {
                vuDatabase->Remove(game);
            }

            // broadcast game data if its our game
            if (game->IsLocal() and ((game->LastTransmissionTime() + game->UpdateRate()) < vuxRealTime))
            {
                if (game->IsDirty())
                {
                    // game dirty, update to everyone
                    VuFullUpdateEvent *msg = new VuFullUpdateEvent(game, vuGlobalGroup);
                    msg->RequestReliableTransmit();
                    msg->RequestOutOfBandTransmit();
                    VuMessageQueue::PostVuMessage(msg);
                    game->ClearDirty();
                }
                else
                {
                    // nothing changed, keep game alive
                    VuBroadcastGlobalEvent *msg = new VuBroadcastGlobalEvent(game, vuGlobalGroup);
                    msg->RequestOutOfBandTransmit();
                    VuMessageQueue::PostVuMessage(msg);
                }

                game->SetTransmissionTime(vuxRealTime);
            }

            UpdateGroupData(game);
        }
    }

#endif
}

VU_ERRCODE VuMainThread::JoinGame(VuGameEntity* game)
{
    VU_ERRCODE retval = vuLocalSessionEntity->JoinGame(game);
    vuLocalSession    = vuLocalSessionEntity->Id();
    return retval;
}

VU_ERRCODE VuMainThread::LeaveGame()
{
    DeinitComms();
    vuLocalSessionEntity->CloseSession();
    return VU_SUCCESS;
}

VU_ERRCODE VuMainThread::InitComms
(
    com_API_handle handle,
    int bufSize,
    int packSize,
    com_API_handle relhandle,
    int relBufSize,
    int relPackSize,
    int resendQueueSize
)
{
    if ((vuGlobalGroup->GetCommsHandle() not_eq NULL) or (vuPlayerPoolGroup not_eq NULL) or (handle == NULL))
    {
        return VU_ERROR;
    }

    if ( not relhandle)
    {
        relhandle   = handle;
        relBufSize  = bufSize;
        relPackSize = packSize;
    }

    // create player pool group(game)
    vuPlayerPoolGroup = new VuPlayerPoolGame(vuxLocalDomain);
    vuPlayerPoolGroup->SetSendCreate(VuEntity::VU_SC_DONT_SEND);
    vuDatabase->Insert(vuPlayerPoolGroup);
    sendQueue_ = new VuPendingSendQueue(resendQueueSize);
    //vuLowSendQueue    = new VuPendingSendQueue(resendQueueSize);

    vuGlobalGroup->SetCommsHandle(handle, bufSize, packSize);
    vuGlobalGroup->SetCommsStatus(VU_CONN_ACTIVE);
    vuGlobalGroup->SetReliableCommsHandle(relhandle, relBufSize, relPackSize);
    vuGlobalGroup->SetReliableCommsStatus(VU_CONN_ACTIVE);
    vuGlobalGroup->SetConnected(TRUE);
    vuLocalSessionEntity->OpenSession();

    return JoinGame(vuPlayerPoolGroup);
}

VU_ERRCODE VuMainThread::DeinitComms()
{
    if (vuPlayerPoolGroup)
    {
        VuEnterCriticalSection();
        FlushOutboundMessages();

        {
            VuListIterator  iter(vuTargetList);

            for (
                VuTargetEntity* target = (VuTargetEntity*)iter.GetFirst(), *nextTarget;
                target not_eq NULL;
                target = nextTarget
            )
            {
                nextTarget = (VuTargetEntity*)iter.GetNext();

                if (target->IsSession())
                {
                    ((VuSessionEntity*)target)->CloseSession();
                }
            }
        }

        delete sendQueue_;
        //delete vuLowSendQueue;

        sendQueue_ = 0;
        //vuLowSendQueue    = 0;

        vuGlobalGroup->SetConnected(FALSE);
        vuGlobalGroup->SetCommsStatus(VU_CONN_INACTIVE);
        vuGlobalGroup->SetCommsHandle(0, 0, 0);
        vuGlobalGroup->SetReliableCommsStatus(VU_CONN_INACTIVE);
        vuGlobalGroup->SetReliableCommsHandle(0, 0, 0);

        vuDatabase->Remove(vuPlayerPoolGroup);
        vuPlayerPoolGroup = 0;
        VuExitCriticalSection();
    }

    return VU_SUCCESS;
}

void VuMainThread::FlushOutboundMessages()
{

    if (sendQueue_)
    {
        sendQueue_->DispatchMessages(-1, TRUE);
    }

    //if (vuLowSendQueue){
    // vuLowSendQueue->DispatchMessages(-1, TRUE);
    //}

    VuTargetEntity* target;
    int cnt = 0;
    int current = 0;
    VuListIterator tli(vuTargetList);
    target = static_cast<VuTargetEntity*>(tli.GetFirst());

    // attempt to send one packet for each comhandle
    while (target and (current = target->FlushOutboundMessageBuffer()) not_eq 0)
    {
        if (current > 0)
        {
            cnt += current;
        }

        target = static_cast<VuTargetEntity*>(tli.GetNext());
    }
}

void VuMainThread::UpdateGroupData(VuGroupEntity* group)
{
    // sfr: placing next inside loop because close can kill it.
    VuSessionsIterator iter(group);

    for (VuSessionEntity *sess = iter.GetFirst(), *next; sess not_eq NULL; sess = next)
    {
        next = iter.GetNext();

        if ((sess not_eq vuLocalSessionEntity) and (sess->GetReliableCommsStatus() == VU_CONN_ERROR))
        {
            // time out this session
            sess->CloseSession();
        }
    }
}

// Get messages from comms, in a round robin manner
// it is assumed that Target::GetMessage will also send
// anything it can in the send queue
int VuMainThread::GetMessages()
{
    int count = 0;

    // Flush all outbound messages into various queues
    VuListIterator  iter(vuTargetList);

    for (
        VuTargetEntity* target = static_cast<VuTargetEntity*>(iter.GetFirst());
        target not_eq NULL;
        target = static_cast<VuTargetEntity*>(iter.GetNext())
    )
    {
        // attempt to send one packet of each type
        target->FlushOutboundMessageBuffer();
        count += target->GetMessages();
    }

    return count;
}

int VuMainThread::SendQueuedMessages()
{

    const unsigned int MAX_TARGETS = 256;
    static int last_time = 0, lru_size[MAX_TARGETS];
    int now;
    unsigned int index;
    VuTargetEntity *target;

    now = vuxRealTime;

    // if interval reached (100ms?), decay the LRU sizes
    if (now - last_time > 100)
    {
        last_time = now;

        for (index = 0; index < MAX_TARGETS; ++index)
        {
            lru_size[index] /= 2;
        }
    }

    // Build array of session targets - for easy indexing later
    // sfr: why only sessions????
    VuTargetEntity *targets[MAX_TARGETS];
    char used[MAX_TARGETS];
    VuListIterator iter(vuTargetList);
    target = (VuTargetEntity*) iter.GetFirst();
    index = 0;

    while (target)
    {
        if (target->IsSession())
        {
            targets[index] = target;
            used[index] = FALSE;
            ++index;
        }

        target = (VuTargetEntity*) iter.GetNext();
    }

    while (index < MAX_TARGETS)
    {
        targets[index] = 0;
        index ++;
    }

    // Now until we run out of stuff to send
    int total = 0;
    bool sent;

    do
    {
        sent = false;

        // clear the used per iteration
        for (index = 0; index < MAX_TARGETS; ++index)
        {
            used[index] = FALSE;
        }

        do
        {
            // find the best - ie. the smallest lru_size
            int best = -1;
            int size = 0x7fffffff;

            for (index = 0; (targets[index]) and (index < MAX_TARGETS); ++index)
            {
                if (( not used[index]) and (lru_size[index] < size))
                {
                    best = index;
                    size = lru_size[index];
                }
            }

            // If we found a best target
            if (best >= 0)
            {
                // attempt to send one packet
                size = targets[best]->SendQueuedMessage();
                // mark it as used
                used[best] = TRUE;
                // add into the lru size
                lru_size[best] += size;

                // remember that we send something
                if (size > 0)
                {
                    sent = true;
                    total += size;
                }
            }
            else
            {
                break;
            }
        }
        while (1);
    }
    while (sent);

    return total;
}


