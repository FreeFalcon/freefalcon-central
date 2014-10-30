#include <utility>

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "vu2.h"
#include "vu_priv.h"
#include "vu_thread.h"
#include "FalcSess.h"
//sfr: checks
#include "InvalidBufferException.h"
// sfr: remove this include (create a new VuxMessageType)
#include "mesg.h"

using namespace std;

//#define DEBUG_COMMS 1
//#define DEBUG_SEND 1
#define LAG_COUNT_START 4

extern VuMainThread* vuMainThread;

//sfr: vu change
VU_SESSION_ID vuNullSession(0);
VU_SESSION_ID vuKnownConnectionId(0);

//-----------------------------------------------------------------------------
//  packet bitand message header stuff
//-----------------------------------------------------------------------------
struct VuPacketHeader
{
    VuPacketHeader(VU_ID tid)
        : targetId_(tid) {}
    VuPacketHeader() {}

    VU_ID     targetId_;  // id of target (for destination filtering)
};

struct VuMessageHeader
{
    VuMessageHeader(VU_MSG_TYPE type, ushort length): type_(type), length_(length) {}
    VuMessageHeader() {}
    /** sfr: returns the CAPI type for this header */
    int CAPIType()
    {
        switch (type_)
        {
            case VU_POSITION_UPDATE_EVENT:
                return CAPI_POSUPD_BWTYPE;
                break;

            case CampDirtyDataMsg:
            case SimDirtyDataMsg:
                return CAPI_DIRTY_BWTYPE;
                break;

            case SendCampaignMsg:
            case SendPersistantList:
            case SendObjData:
            case SendUnitData:
            case SendVCMsg:
            case CampDataMsg:

                //return CAPI_JOIN_BW_TYPE;
            default:
                return CAPI_OTHER_BWTYPE;
        }
    }

    // data
    VU_MSG_TYPE type_;
    ushort  length_;
};

static int ReadPacketHeader(VU_BYTE *data, VuPacketHeader* hdr)
{
    if (vuKnownConnectionId not_eq vuNullSession)
    {
        // read nothing  -- fill in from known data
        hdr->targetId_.creator_ = vuKnownConnectionId;
        hdr->targetId_.num_     = VU_SESSION_ENTITY_ID;
        return 0;
    }

    memcpy(&hdr->targetId_.creator_, data, sizeof(hdr->targetId_.creator_));
    data += sizeof(hdr->targetId_.creator_);
    memcpy(&hdr->targetId_.num_, data, sizeof(hdr->targetId_.num_));

    return PACKET_HDR_SIZE;
}

static int WritePacketHeader(VU_BYTE *data, VuPacketHeader* hdr)
{
    if (vuKnownConnectionId not_eq vuNullSession)
    {
        // write nothing -- destination will fill in from known data
        return 0;
    }

    memcpy(data, &hdr->targetId_.creator_, sizeof(hdr->targetId_.creator_));
    data += sizeof(hdr->targetId_.creator_);
    memcpy(data, &hdr->targetId_.num_, sizeof(hdr->targetId_.num_));

    return PACKET_HDR_SIZE;
}

static int ReadMessageHeader(VU_BYTE *data, VuMessageHeader* hdr)
{
    int retsize;
    memcpy(&hdr->type_, data, sizeof(hdr->type_));
    data += sizeof(hdr->type_);

    if (*data bitand 0x80)     // test for high bit set
    {
        hdr->length_  = static_cast<ushort>(((data[0] << 8) bitor data[1]));
        hdr->length_ and_eq 0x7fff; // mask off huffman bit
        retsize       = MAX_MSG_HDR_SIZE;
    }
    else
    {
        hdr->length_ = *data;
        retsize      = MIN_MSG_HDR_SIZE;
    }

    return retsize;
}

static int WriteMessageHeader(VU_BYTE *data, VuMessageHeader* hdr)
{
    int retsize;

    memcpy(data, &hdr->type_, sizeof(hdr->type_));
    data += sizeof(hdr->type_);

    if (hdr->length_ > 0x7f)
    {
        ushort huff = static_cast<ushort>(0x8000 bitor hdr->length_);
        data[0] = static_cast<VU_BYTE>((huff >> 8) bitand 0xff);
        data[1] = static_cast<VU_BYTE>(huff bitand 0xff);
        retsize = MAX_MSG_HDR_SIZE;
    }
    else
    {
        *data   = (VU_BYTE)hdr->length_;
        retsize = MIN_MSG_HDR_SIZE;
    }

    return retsize;
}

//-----------------------------------------------------------------------------
static int MessageReceive(
    VU_ID targetId,
    VU_ADDRESS senderAddress, // sfr added address
    VU_ID senderid,
    VU_MSG_TYPE type,
    VU_BYTE **data,
    int size,
    VU_TIME timestamp
)
{
    VuMessage* event = 0;

#ifdef DEBUG_COMMS
    VU_PRINT("VU: Receive [%d]", type);
#endif

    if (vuLocalSessionEntity->InTarget(targetId))
    {
        switch (type)
        {
            case VU_GET_REQUEST_MESSAGE:
                event = new VuGetRequest(senderid, targetId);
                break;

            case VU_PUSH_REQUEST_MESSAGE:
                event = new VuPushRequest(senderid, targetId);
                break;

            case VU_PULL_REQUEST_MESSAGE:
                event = new VuPullRequest(senderid, targetId);
                break;

            case VU_DELETE_EVENT:
                event = new VuDeleteEvent(senderid, targetId);
                break;
#if 0

            case VU_UNMANAGE_EVENT:
                event = new VuUnmanageEvent(senderid, targetId);
                break;
#endif

            case VU_MANAGE_EVENT:
                event = new VuManageEvent(senderid, targetId);
                break;

            case VU_CREATE_EVENT:
                event = new VuCreateEvent(senderAddress, senderid, targetId);
                break;

            case VU_SESSION_EVENT:
                event = new VuSessionEvent(senderid, targetId);
                break;

            case VU_TRANSFER_EVENT:
                event = new VuTransferEvent(senderid, targetId);
                break;

            case VU_POSITION_UPDATE_EVENT:
                event = new VuPositionUpdateEvent(senderid, targetId);
                break;

            case VU_BROADCAST_GLOBAL_EVENT:
                event = new VuBroadcastGlobalEvent(senderAddress, senderid, targetId);
                break;

            case VU_FULL_UPDATE_EVENT:
                event = new VuFullUpdateEvent(senderAddress, senderid, targetId);
                break;

            case VU_ENTITY_COLLISION_EVENT:
                event = new VuEntityCollisionEvent(senderid, targetId);
                break;

            case VU_GROUND_COLLISION_EVENT:
                event = new VuGroundCollisionEvent(senderid, targetId);
                break;

            case VU_RESERVED_UPDATE_EVENT:
            case VU_UNKNOWN_MESSAGE:
            case VU_TIMER_EVENT:
#if not NO_RELEASE_EVENT
            case VU_RELEASE_EVENT:
                // these are not net events... ignore
                break;
#endif

            case VU_ERROR_MESSAGE:
                event = new VuErrorMessage(senderid, targetId);
                break;

            case VU_REQUEST_DUMMY_BLOCK_MESSAGE:
                event = new VuRequestDummyBlockMessage(senderid, targetId);
                break;
#if defined(VU_SIMPLE_LATENCY)

            case VU_TIMING_MESSAGE:
                event = new VuTimingMessage(senderid, targetId);
                break;
#endif
        }

        if ( not event)
        {
            event = VuxCreateMessage(type, senderid, targetId);
        }

        //sfr: we only read events when they have a size
        //but it would be good to know why message size is 0
        //have to compute message size here
        //for now, comment to force crash and see why we are getting 0 size
        if ((size > 0) and (event not_eq NULL))
        {
            long rem = size;

            if (event->Read(data, &rem))
            {
                event->SetPostTime(timestamp);
                VuMessageQueue::PostVuMessage(event);
            }
        }
    }
    else
    {
        *data += size;
#ifdef DEBUG_COMMS
        VU_PRINT(": dest = 0x%x -- not posted\n", (short)targetId.creator_);
#endif
    }

    return (event ? 1 : 0);
}

static int MessagePoll(VuCommsContext* ctxt)
{
    static int last_full_update = 0;
    int now;
    int             count   = 0;
    int             length  = 0;
    com_API_handle    ch      = ctxt->handle_;
    char*           readBuf = ComAPIRecvBufferGet(ch);
    VU_BYTE*        data;
    VU_BYTE*        end;
    VuPacketHeader  phdr;
    VuMessageHeader mhdr;
    VU_ID           senderid;
    VU_ADDRESS senderAddress;

#pragma warning(disable : 4127)

    while (TRUE)
    {
        if ((length = ComAPIGet(ch)) > 0)
        {
            //sfr: converts /  vu change
            VU_TIME    timestamp = ComAPIGetTimeStamp(ch);
            VuEntity* sender;

            senderAddress.ip = ComAPIQuery(ch, COMAPI_SENDER);
            //senderAddress.sendPort = (short)ComAPIQuery(ch, COMAPI_SENDER_PORT);
            senderid.num_ = VU_SESSION_ENTITY_ID;
            senderid.creator_.value_ = ComAPIQuery(ch, COMAPI_ID);
            // entity who sent message
            sender = vuDatabase->Find(senderid);

            data  = (VU_BYTE*)readBuf;
            end   = data + length;
            data += ReadPacketHeader(data, &phdr);

            while (data < end)
            {
                data += ReadMessageHeader(data, &mhdr);
                count += MessageReceive(
                             phdr.targetId_, senderAddress, senderid, mhdr.type_, &data, mhdr.length_, timestamp
                         );
            }

            readBuf = ComAPIRecvBufferGet(ch);
        }
        else if (length == -1)
        {
            count ++;
            break;
        }
        else if (length == -2)
        {
            // Still not synced up yet - send a full update
            //MonoPrint ("Not Connected - sending full update\n");

            now = GetTickCount();

            if (now - last_full_update > 2000)
            {
                VuFullUpdateEvent *msg = new VuFullUpdateEvent(vuLocalSessionEntity.get(), vuGlobalGroup);
                msg->RequestOutOfBandTransmit();
                VuMessageQueue::PostVuMessage(msg);
                last_full_update = now;
            }

            break;
        }
        else
        {
            break;
        }
    }

#pragma warning(default : 4127)

    return count;
}

//-----------------------------------------------------------------------------
class GlobalGroupFilter : public VuFilter
{
public:
    GlobalGroupFilter();
    virtual ~GlobalGroupFilter();

    virtual VU_BOOL Test(VuEntity *ent);
    virtual VU_BOOL RemoveTest(VuEntity *ent);
    virtual int Compare(VuEntity *ent1, VuEntity *ent2);
    virtual VuFilter *Copy();

protected:
    GlobalGroupFilter(GlobalGroupFilter *other);

    // DATA
protected:
    // none
};

//-----------------------------------------------------------------------------
// VuSessionsIterator
//-----------------------------------------------------------------------------
// use groups collection as mutex, if group is null use local session game
VuSessionsIterator::VuSessionsIterator(VuGroupEntity* group) :
    VuListIterator(
        group ?
        group->sessionCollection_ :
        (
            vuLocalSessionEntity->Game() ?  vuLocalSessionEntity->Game()->sessionCollection_ : 0
        )
    )
{
}

VuSessionsIterator::~VuSessionsIterator()
{
}

VuSessionEntity *VuSessionsIterator::GetFirst()
{
    return static_cast<VuSessionEntity*>(VuListIterator::GetFirst());
}
VuSessionEntity *VuSessionsIterator::GetNext()
{
    return static_cast<VuSessionEntity*>(VuListIterator::GetNext());
}

VuEntity *VuSessionsIterator::CurrEnt()
{
    return curr_->get();
}

VU_ERRCODE VuSessionsIterator::Cleanup()
{
    return VuListIterator::Cleanup();
}

//-----------------------------------------------------------------------------
// VuSessionFilter
//-----------------------------------------------------------------------------

VuSessionFilter::VuSessionFilter(VU_ID groupId): VuFilter(), groupId_(groupId)
{
    // empty
}

VuSessionFilter::VuSessionFilter(VuSessionFilter* other): VuFilter(other), groupId_(other->groupId_)
{
    // empty
}

VuSessionFilter::~VuSessionFilter()
{
    // empty
}

VU_BOOL VuSessionFilter::Test(VuEntity* ent)
{
    return static_cast<VU_BOOL>((ent->IsSession() and ((VuSessionEntity*)ent)->GameId() == groupId_) ? TRUE : FALSE);
}

VU_BOOL VuSessionFilter::RemoveTest(VuEntity* ent)
{
    return ent->IsSession();
}

int VuSessionFilter::Compare(VuEntity* ent1, VuEntity* ent2)
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

VU_BOOL VuSessionFilter::Notice(VuMessage* event)
{
    // danm_TBD: do we need VU_FULL_UPDATE event as well?
    if ((1 << event->Type()) bitand VU_TRANSFER_EVENT)
    {
        return TRUE;
    }

    return FALSE;
}

VuFilter *VuSessionFilter::Copy()
{
    return new VuSessionFilter(this);
}

//-----------------------------------------------------------------------------
// GlobalGroupFilter
//-----------------------------------------------------------------------------

GlobalGroupFilter::GlobalGroupFilter() : VuFilter()
{
    // empty
}

GlobalGroupFilter::GlobalGroupFilter(GlobalGroupFilter* other) : VuFilter(other)
{
    // empty
}

GlobalGroupFilter::~GlobalGroupFilter()
{
    // empty
}

VU_BOOL GlobalGroupFilter::Test(VuEntity* ent)
{
    return ent->IsSession();
}

VU_BOOL GlobalGroupFilter::RemoveTest(VuEntity* ent)
{
    return ent->IsSession();
}

int GlobalGroupFilter::Compare(VuEntity* ent1, VuEntity* ent2)
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

VuFilter *GlobalGroupFilter::Copy()
{
    return new GlobalGroupFilter(this);
}

void Init(VuCommsContext* ctxt)
{
    memset(ctxt, 0, sizeof(VuCommsContext));
    ctxt->status_ = VU_CONN_INACTIVE;
}

void Cleanup(VuCommsContext* ctxt)
{
    delete [] ctxt->normalSendPacket_;
    delete [] ctxt->lowSendPacket_;
    delete [] ctxt->recBuffer_;
    Init(ctxt);   // just to be safe
}

//-----------------------------------------------------------------------------
// VuTargetEntity
//-----------------------------------------------------------------------------
VuTargetEntity::VuTargetEntity(ushort typeindex, VU_ID_NUMBER eid): VuEntity(typeindex, eid)
{
    Init(&bestEffortComms_);
    Init(&reliableComms_);
    reliableComms_.reliable_ = TRUE;
    dirty = 0;
}

VuTargetEntity::VuTargetEntity(VU_BYTE** stream, long *rem): VuEntity((ushort)VU_UNKNOWN_ENTITY_TYPE, 0)
{
    Init(&bestEffortComms_);
    Init(&reliableComms_);
    reliableComms_.reliable_ = TRUE;
    memcpychk(&share_.entityType_,  stream, sizeof(share_.entityType_), rem);
    memcpychk(&share_.flags_,       stream, sizeof(share_.flags_), rem);
    memcpychk(&share_.id_.creator_, stream, sizeof(share_.id_.creator_), rem);
    memcpychk(&share_.id_.num_,     stream, sizeof(share_.id_.num_), rem);

    SetEntityType(share_.entityType_);
    dirty = 0;
}

VuTargetEntity::VuTargetEntity(FILE* file): VuEntity((ushort)VU_UNKNOWN_ENTITY_TYPE, 0)
{
    Init(&bestEffortComms_);
    Init(&reliableComms_);
    reliableComms_.reliable_ = TRUE;
    fread(&share_.entityType_,  sizeof(share_.entityType_),  1, file);
    fread(&share_.flags_,       sizeof(share_.flags_),       1, file);
    fread(&share_.id_.creator_, sizeof(share_.id_.creator_), 1, file);
    fread(&share_.id_.num_,     sizeof(share_.id_.num_),     1, file);

    SetEntityType(share_.entityType_);
    dirty = 0;
}

VuTargetEntity::~VuTargetEntity()
{
    VuPendingSendQueue *sq = vuMainThread->SendQueue();

    if (sq)
    {
        sq->RemoveTarget(this);
    }

    //if (vuLowSendQueue){
    // vuLowSendQueue->RemoveTarget(this);
    //}
    CloseComms();
}

int VuTargetEntity::LocalSize()
{
    return sizeof(share_.entityType_)
           + sizeof(share_.flags_)
           + sizeof(share_.id_.creator_)
           + sizeof(share_.id_.num_)
           ;
}

int VuTargetEntity::SaveSize()
{
    return LocalSize();
}

int VuTargetEntity::Save(VU_BYTE** stream)
{
    memcpy(*stream, &share_.entityType_,  sizeof(share_.entityType_));
    *stream += sizeof(share_.entityType_);
    memcpy(*stream, &share_.flags_,       sizeof(share_.flags_));
    *stream += sizeof(share_.flags_);
    memcpy(*stream, &share_.id_.creator_, sizeof(share_.id_.creator_));
    *stream += sizeof(share_.id_.creator_);
    memcpy(*stream, &share_.id_.num_,     sizeof(share_.id_.num_));
    *stream += sizeof(share_.id_.num_);

    return VuTargetEntity::LocalSize();
}

int VuTargetEntity::Save(FILE* file)
{
    int retval = 0;

    if (file)
    {
        retval += fwrite(&share_.entityType_,  sizeof(share_.entityType_),  1, file);
        retval += fwrite(&share_.flags_,       sizeof(share_.flags_),       1, file);
        retval += fwrite(&share_.id_.creator_, sizeof(share_.id_.creator_), 1, file);
        retval += fwrite(&share_.id_.num_,     sizeof(share_.id_.num_),     1, file);
    }

    return retval;
}

VU_BOOL VuTargetEntity::IsTarget()
{
    return TRUE;
}

int VuTargetEntity::BytesPending()
{
    int retval = 0;

    /*
       if (GetCommsStatus() not_eq VU_CONN_INACTIVE and GetCommsHandle())
       {
       retval += bestEffortComms_.normalSendPacketPtr_ - bestEffortComms_.normalSendPacket_;
       retval += bestEffortComms_.lowSendPacketPtr_    - bestEffortComms_.lowSendPacket_;
       }
     */
    if (GetReliableCommsStatus() not_eq VU_CONN_INACTIVE and GetReliableCommsHandle())
    {
        // retval += reliableComms_.normalSendPacketPtr_ - reliableComms_.normalSendPacket_;
        // retval += reliableComms_.lowSendPacketPtr_    - reliableComms_.lowSendPacket_;
        retval += ComAPIQuery(reliableComms_.handle_, COMAPI_BYTES_PENDING);
    }

    return retval;
}

int VuTargetEntity::MaxPacketSize()
{
    if (GetCommsStatus() == VU_CONN_INACTIVE)
    {
        return 0;
    }

    if ( not GetCommsHandle())
    {
        VuTargetEntity *forward = ForwardingTarget();

        if (forward and forward not_eq this)
        {
            return forward->MaxPacketSize();
        }

        return 0;
    }

    // return size of largest message to fit in one packet
    return bestEffortComms_.maxMsgSize_ + MIN_HDR_SIZE;
}

int VuTargetEntity::MaxMessageSize()
{
    if (GetCommsStatus() == VU_CONN_INACTIVE)
    {
        return 0;
    }

    if ( not GetCommsHandle())
    {
        VuTargetEntity *forward = ForwardingTarget();

        if (forward and forward not_eq this)
        {
            return forward->MaxMessageSize();
        }

        return 0;
    }

    return bestEffortComms_.maxMsgSize_;
}

int VuTargetEntity::MaxReliablePacketSize()
{
    if (GetReliableCommsStatus() == VU_CONN_INACTIVE)
    {
        return 0;
    }

    if ( not GetReliableCommsHandle())
    {
        VuTargetEntity *forward = ForwardingTarget();

        if (forward and forward not_eq this)
        {
            return forward->MaxReliablePacketSize();
        }

        return 0;
    }

    // return size of largest message to fit in one packet
    return reliableComms_.maxMsgSize_ + MIN_HDR_SIZE;
}

int VuTargetEntity::MaxReliableMessageSize()
{
    if (GetReliableCommsStatus() == VU_CONN_INACTIVE)
    {
        return 0;
    }

    if ( not GetReliableCommsHandle())
    {
        VuTargetEntity *forward = ForwardingTarget();

        if (forward and forward not_eq this)
        {
            return forward->MaxReliableMessageSize();
        }

        return 0;
    }

    return reliableComms_.maxMsgSize_;
}

void VuTargetEntity::SetCommsHandle(com_API_handle ch, int bufSize, int packSize)
{
    unsigned long chbufsize = 0;

    if (ch)
    {
        chbufsize = ComAPIQuery(ch, COMAPI_ACTUAL_BUFFER_SIZE);
    }

    if (bufSize <= 0)
    {
        bufSize = chbufsize - MIN_HDR_SIZE;
    }

    if (bufSize > 0x7fff)
    {
        bufSize = 0x7fff;   // max size w/ huffman encoding
    }

    delete [] bestEffortComms_.normalSendPacket_;
    delete [] bestEffortComms_.lowSendPacket_;
    delete [] bestEffortComms_.recBuffer_;

    if (ch and bufSize > 0)
    {
        bestEffortComms_.handle_     = ch;
        bestEffortComms_.maxMsgSize_ = bufSize;

        if (packSize)
            bestEffortComms_.maxPackSize_ = packSize;
        else
            bestEffortComms_.maxPackSize_ = bufSize;

        bestEffortComms_.normalSendPacket_ = new VU_BYTE[bufSize + MIN_HDR_SIZE];
        bestEffortComms_.lowSendPacket_    = new VU_BYTE[bufSize + MIN_HDR_SIZE];
        bestEffortComms_.recBuffer_        = new VU_BYTE[bufSize];
    }
    else
    {
        bestEffortComms_.handle_           = 0;
        bestEffortComms_.maxMsgSize_       = 0;
        bestEffortComms_.normalSendPacket_ = 0;
        bestEffortComms_.lowSendPacket_    = 0;
        bestEffortComms_.recBuffer_        = 0;
    }

    bestEffortComms_.normalSendPacketPtr_ = bestEffortComms_.normalSendPacket_;
    bestEffortComms_.lowSendPacketPtr_    = bestEffortComms_.lowSendPacket_;
}

void VuTargetEntity::SetReliableCommsHandle(com_API_handle ch, int bufSize, int packSize)
{
    unsigned long chbufsize = 0;

    if (ch)
    {
        chbufsize = ComAPIQuery(ch, COMAPI_ACTUAL_BUFFER_SIZE);
    }

    if (bufSize <= 0)
    {
        bufSize = chbufsize - MIN_HDR_SIZE;
    }

    if (bufSize > 0x7fff)
    {
        bufSize = 0x7fff;   // max size w/ huffman encoding
    }

    delete [] reliableComms_.normalSendPacket_;
    delete [] reliableComms_.lowSendPacket_;
    delete [] reliableComms_.recBuffer_;

    if (ch and (bufSize > 0))
    {
        reliableComms_.handle_           = ch;
        reliableComms_.maxMsgSize_       = bufSize;
        reliableComms_.maxPackSize_      = packSize;
        reliableComms_.normalSendPacket_ = new VU_BYTE[bufSize + MIN_HDR_SIZE];
        reliableComms_.lowSendPacket_    = new VU_BYTE[bufSize + MIN_HDR_SIZE];
        reliableComms_.recBuffer_        = new VU_BYTE[bufSize];
    }
    else
    {
        reliableComms_.handle_           = 0;
        reliableComms_.maxMsgSize_       = 0;
        reliableComms_.normalSendPacket_ = 0;
        reliableComms_.lowSendPacket_    = 0;
        reliableComms_.recBuffer_        = 0;
    }

    reliableComms_.normalSendPacketPtr_ = reliableComms_.normalSendPacket_;
    reliableComms_.lowSendPacketPtr_    = reliableComms_.lowSendPacket_;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int VuTargetEntity::SendQueuedMessage(void)
{
    int size;

    if ((reliableComms_.handle_) and (reliableComms_.status_ == VU_CONN_ACTIVE))
    {
        size = ComAPISend(reliableComms_.handle_, 0, CAPI_COMMON_BWTYPE);

        if (size == -2)
        {
            // Still not synced up yet - send a full update
            VuFullUpdateEvent *msg = new VuFullUpdateEvent(vuLocalSessionEntity.get(), vuGlobalGroup);
            msg->RequestOutOfBandTransmit();
            VuMessageQueue::PostVuMessage(msg);
        }
        else if (size > 0)
        {
            return size;
        }
    }

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int VuTargetEntity::GetMessages()
{
    int count = 0;
    int ping = 0;

    // get reliable messages
    if (reliableComms_.handle_ and reliableComms_.status_ == VU_CONN_ACTIVE)
    {
        count += MessagePoll(&reliableComms_);
        ping = ComAPIQuery(reliableComms_.handle_, COMAPI_PING_TIME);

        if (ping == -1)
        {
            reliableComms_.status_ = VU_CONN_ERROR;
            bestEffortComms_.status_ = VU_CONN_ERROR;
            return -1;
        }
        else if (ping == -2)
        {
            reliableComms_.status_ = VU_CONN_ERROR;
            bestEffortComms_.status_ = VU_CONN_ERROR;
            return -2;
        }
        else if (ping > reliableComms_.ping)
        {
            //if we are a host checking a client set that clients bw
            VuSessionEntity* session = (VuSessionEntity*)vuDatabase->Find(this->OwnerId());

            if (vuLocalGame->OwnerId() not_eq vuLocalSessionEntity->Id() or session == vuLocalSessionEntity)
            {
                session = NULL;//we are not host or its us self
            }

            if (
                session and 
                ((FalconSessionEntity*)vuDatabase->Find(this->Id()))->GetFlyState() not_eq FLYSTATE_FLYING
            )
            {
                session = NULL;//session is not in cockpit
            }

            if (session and 
                ((FalconSessionEntity*)vuDatabase->Find(vuLocalSessionEntity->Game()->OwnerId()))->GetFlyState() not_eq FLYSTATE_FLYING
               )
            {
                session = NULL;//host is not in cockpit
            }

            if (
                (static_cast<VU_TIME>(ping) > EntityType()->updateTolerance_)
            )
            {
                reliableComms_.status_ = VU_CONN_ERROR;
                bestEffortComms_.status_ = VU_CONN_ERROR;
                return -1;
            }
        }
        else
        {
            reliableComms_.ping = ping;
        }
    }

    // get unreliable ones
    if (bestEffortComms_.handle_ and bestEffortComms_.status_ == VU_CONN_ACTIVE)
    {
        count += MessagePoll(&bestEffortComms_);
    }

    return count;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void VuTargetEntity::SetDirty(void)
{
    // MonoPrint ("Set Dirty\n");
    dirty = 1;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void VuTargetEntity::ClearDirty(void)
{
    dirty = 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int VuTargetEntity::IsDirty(void)
{
    return dirty;
}

VuTargetEntity* VuTargetEntity::ForwardingTarget(VuMessage *)
{
    return vuGlobalGroup;
}

void VuTargetEntity::CloseComms()
{
    if (bestEffortComms_.handle_)
    {
        ComAPIClose(bestEffortComms_.handle_);
    }

    if (reliableComms_.handle_)
    {
        ComAPIClose(reliableComms_.handle_);
    }

    Cleanup(&bestEffortComms_);
    Cleanup(&reliableComms_);
}

int VuTargetEntity::Flush(VuCommsContext* ctxt)
{
    VuEnterCriticalSection();

    int retval = 0;

    if (ctxt->normalSendPacketPtr_ not_eq ctxt->normalSendPacket_)
    {
        int size = ctxt->normalSendPacketPtr_ - ctxt->normalSendPacket_;

        if (size > 0)
        {
            VU_BYTE * buffer = (VU_BYTE *)ComAPISendBufferGet(ctxt->handle_);
            memcpy(buffer, ctxt->normalSendPacket_, size);
            // sfr: get message type from header
            VU_BYTE *msgPtr = buffer + PACKET_HDR_SIZE;
            VuMessageHeader *msgHdr = (VuMessageHeader*)msgPtr;

            // this is where send occurs
            retval = ComAPISend(ctxt->handle_, size, msgHdr->CAPIType());

            if (retval > 0)
            {
                // message sent successfully
                ctxt->normalSendPacketPtr_ = ctxt->normalSendPacket_;
            }
            else if (retval not_eq COMAPI_WOULDBLOCK)
            {
                // non-recoverable error
                ctxt->normalSendPacketPtr_ = ctxt->normalSendPacket_;
            }
        }
    }

    VuExitCriticalSection();
    return retval;
}

int VuTargetEntity::FlushLow(VuCommsContext* ctxt)
{
    VuEnterCriticalSection();

    int retval = 0;

    if (ctxt->lowSendPacketPtr_ not_eq ctxt->lowSendPacket_)
    {
        int size = ctxt->lowSendPacketPtr_ - ctxt->lowSendPacket_;

        if (size > 0)
        {
            VU_BYTE*  buffer = (VU_BYTE *)ComAPISendBufferGet(ctxt->handle_);
            memcpy(buffer, ctxt->lowSendPacket_, size);
            //sfr msg type
            VU_BYTE *msgPtr = buffer + PACKET_HDR_SIZE;
            VuMessageHeader *msgHdr = (VuMessageHeader*)msgPtr;
            retval = ComAPISend(ctxt->handle_, size, msgHdr->CAPIType());

            if (retval > 0)
            {
                // message sent successfully
                ctxt->lowSendPacketPtr_ = ctxt->lowSendPacket_;
            }
            else if (retval not_eq COMAPI_WOULDBLOCK)
            {
                // non-recoverable error
                ctxt->lowSendPacketPtr_ = ctxt->lowSendPacket_;
            }
        }
    }

    VuExitCriticalSection();
    return retval;
}

// Return values:
//  >  0 --> flush succeeded (all contexts sent data)
//  == 0 --> no data was pending;
//   < 0 --> flush failed (likely send buffer full)
int VuTargetEntity::FlushOutboundMessageBuffer()
{
    VuEnterCriticalSection();

    int retval = -1;

    if (GetReliableCommsStatus() == VU_CONN_ACTIVE and GetReliableCommsHandle())
    {
        retval = Flush(&reliableComms_);
    }

    if (GetCommsStatus() == VU_CONN_ACTIVE and GetCommsHandle())
    {
        int retval2 = Flush(&bestEffortComms_);

        if (retval2 > 0)
        {
            retval += retval2;
        }
    }

    //if (retval > 0) {
    // VuMainThread::ReportXmit(retval);
    //}

    VuExitCriticalSection();
    return retval;
}

int VuTargetEntity::SendOutOfBand(VuCommsContext* ctxt, VuMessage *msg)
{
    int retval = 0;

    // write packet header
    VuPacketHeader phdr(msg->Destination());
    VU_BYTE* start_buffer = (VU_BYTE *)ComAPISendBufferGet(ctxt->handle_);
    VU_BYTE* buffer       = start_buffer;
    buffer += WritePacketHeader(buffer, &phdr);

    // message header
    int size = msg->Size();
    VuMessageHeader mhdr(msg->Type(), static_cast<short>(size));
    buffer += WriteMessageHeader(buffer, &mhdr);

    // message
    msg->Write(&buffer);

    if (size > 0)
    {
        retval = ComAPISendOOB(ctxt->handle_, buffer - start_buffer, mhdr.CAPIType());
    }

    return retval;
}

int VuTargetEntity::SendNormalPriority(VuCommsContext* ctxt, VuMessage *msg)
{
    int retval = -3;
    VuEnterCriticalSection();
    int totsize, size = msg->Size();

    totsize = size + PACKET_HDR_SIZE + MAX_MSG_HDR_SIZE;

    if (totsize > ctxt->maxMsgSize_)
    {
        // write would exceed buffer
        retval = COMAPI_MESSAGE_TOO_BIG;
    }
    else if (
        // data is waiting in queue
        (ctxt->normalSendPacketPtr_ not_eq ctxt->normalSendPacket_) and (
            // write would exceed packet size
            totsize + ctxt->normalSendPacketPtr_ - ctxt->normalSendPacket_ > ctxt->maxPackSize_ or (
                // different origins
                ctxt->normalSendPacketPtr_ not_eq ctxt->normalSendPacket_ and (
                    msg->Sender().creator_ not_eq ctxt->normalPendingSenderId_.creator_ or
                    msg->Destination() not_eq ctxt->normalPendingSendTargetId_
                )
            )
        )
    )
    {
        if (Flush(ctxt) > 0 and ctxt->normalSendPacketPtr_ == ctxt->normalSendPacket_)
        {
            // try resending...
            retval = SendNormalPriority(ctxt, msg);
        }
        else
        {
            // cannot send, return error
            // retval = COMAPI_WOULDBLOCK;
        }
    }
    else
    {
        // write fits within buffer -- add to/begin packet
        if (ctxt->normalSendPacketPtr_ == ctxt->normalSendPacket_)
        {
            // begin a new packet
            VuPacketHeader phdr(msg->Destination());
            ctxt->normalSendPacketPtr_ += WritePacketHeader(ctxt->normalSendPacketPtr_, &phdr);
        }

        VuMessageHeader mhdr(msg->Type(), static_cast<short>(size));
        ctxt->normalSendPacketPtr_           += WriteMessageHeader(ctxt->normalSendPacketPtr_, &mhdr);
        ctxt->normalPendingSenderId_.creator_ = msg->Sender().creator_;
        ctxt->normalPendingSenderId_.num_     = msg->Sender().num_;
        ctxt->normalPendingSendTargetId_      = msg->Destination();
        // return number of bytes written to buffer
        VU_BYTE* ptr    = ctxt->normalSendPacketPtr_;
        retval = msg->Write(&ptr);
        ctxt->normalSendPacketPtr_ = ptr;
        // update msg count
        //*ctxt->sendPacket_ += 1;
    }

    VuExitCriticalSection();
    return retval;
}

int VuTargetEntity::SendLowPriority(VuCommsContext* ctxt, VuMessage *msg)
{
    int retval  = -3;
    int totsize;
    int size;

    VuEnterCriticalSection();

    size    = msg->Size();
    totsize = size + PACKET_HDR_SIZE + MAX_MSG_HDR_SIZE;

    if (totsize > ctxt->maxMsgSize_)   // write would exceed buffer
    {
        assert(0);
        retval = COMAPI_MESSAGE_TOO_BIG;
    }
    else if (
        ctxt->lowSendPacketPtr_ not_eq ctxt->lowSendPacket_ and (
            // data is waiting in queue
            totsize + ctxt->lowSendPacketPtr_ - ctxt->lowSendPacket_ > ctxt->maxPackSize_ or (
                // write would exceed packet size
                ctxt->lowSendPacketPtr_ not_eq ctxt->lowSendPacket_ and (
                    // different origins
                    msg->Sender().creator_ not_eq ctxt->lowPendingSenderId_.creator_ or
                    msg->Destination() not_eq ctxt->lowPendingSendTargetId_
                )
            )
        )
    )
    {
        if (FlushLow(ctxt) > 0 and ctxt->lowSendPacketPtr_ == ctxt->lowSendPacket_)
        {
            // try resending...
            retval = SendLowPriority(ctxt, msg);
        }
        else
        {
            // cannot send, return error
            retval = COMAPI_WOULDBLOCK;
        }
    }
    else
    {
        // write fits within buffer -- add to/begin packet
        if (ctxt->lowSendPacketPtr_ == ctxt->lowSendPacket_)   // begin a new packet
        {
            VuPacketHeader phdr(msg->Destination());
            ctxt->lowSendPacketPtr_ += WritePacketHeader(ctxt->lowSendPacketPtr_, &phdr);
        }

        VuMessageHeader mhdr(msg->Type(), static_cast<short>(size));

        ctxt->lowSendPacketPtr_           += WriteMessageHeader(ctxt->lowSendPacketPtr_, &mhdr);
        ctxt->lowPendingSenderId_.creator_ = msg->Sender().creator_;
        ctxt->lowPendingSenderId_.num_     = msg->Sender().num_;
        ctxt->lowPendingSendTargetId_      = msg->Destination();

        // return number of bytes written to buffer
        VU_BYTE* ptr    = ctxt->lowSendPacketPtr_;
        retval = msg->Write(&ptr);
        ctxt->lowSendPacketPtr_ = ptr;
    }

    VuExitCriticalSection();

    return retval;
}

int VuTargetEntity::SendMessage(VuMessage* msg)
{
    int retval = 0;

    // find the comms handle to send message
    VuCommsContext* ctxt = 0;

    if (msg->Flags() bitand VU_RELIABLE_MSG_FLAG)
    {
        if (GetReliableCommsHandle())
        {
            ctxt = &reliableComms_;
        }
    }
    else
    {
        if (GetCommsHandle())
        {
            ctxt = &bestEffortComms_;
        }
    }

    // target has no handle, try to find a forwaring
    if ( not ctxt)
    {
        VuTargetEntity *forward = ForwardingTarget(msg);

        if (forward and forward not_eq this)
        {
            return forward->SendMessage(msg);
        }

        // cannot send
        return 0;
    }

    // set the message id...
    // note: this counts events which are overwritten by Read()
    // note2: don't reassign id's for resubmitted messages (delay)
    // note3: 5/27/98 -- probably not needed anymore
    if (msg->Flags() bitand VU_OUT_OF_BAND_MSG_FLAG)
    {
        retval = SendOutOfBand(ctxt, msg);
    }
    else if (msg->Flags() bitand VU_NORMAL_PRIORITY_MSG_FLAG)
    {
        retval = SendNormalPriority(ctxt, msg);
    }
    else
    {
        retval = SendLowPriority(ctxt, msg);
    }

    if (
        (msg->Flags() bitand VU_RELIABLE_MSG_FLAG) and 
        (retval == COMAPI_WOULDBLOCK or retval == COMAPI_CONNECTION_PENDING)
    )
    {
        // recoverable error
        VuMessageQueue::RepostMessage(msg, 500); // 500 ms delay
        return 0;
    }

    return retval;
}

//-----------------------------------------------------------------------------
// VuSessionEntity
//-----------------------------------------------------------------------------
// sfr: this is called only for local session. All others are streamized
VuSessionEntity::VuSessionEntity(ushort typeindex, ulong domainMask, char *callsign):
    VuTargetEntity(typeindex, VU_SESSION_ENTITY_ID),
    sessionId_(0), // assigned on session open
    address_(VU_ADDRESS(0, com_API_get_my_receive_port(), com_API_get_my_reliable_receive_port())), //sfr
    domainMask_(domainMask),
    callsign_(0),
    loadMetric_(1),
    gameId_(VU_SESSION_NULL_GROUP), // assigned on session open
    groupCount_(0),
    groupHead_(0),
    // bandwidth_(33600),
#ifdef VU_SIMPLE_LATENCY
    timeDelta_(0),
    latency_(0),
#endif //VU_SIMPLE_LATENCY
#ifdef VU_TRACK_LATENCY
    timeSyncState_(VU_NO_SYNC),
    latency_(0),
    masterTime_(0),
    masterTimePostTime_(0),
    responseTime_(0),
    masterTimeOwner_(0),
    lagTotal_(0),
    lagPackets_(0),
    lagUpdate_(LAG_COUNT_START),
#endif //VU_TRACK_LATENCY
    lastMsgRecvd_(0),
    game_(0),
    action_(VU_NO_GAME_ACTION)
{
    share_.id_.creator_ = sessionId_;
    share_.id_.num_     = VU_SESSION_ENTITY_ID;
    share_.ownerId_     = share_.id_;   // need to reset this
    SetCallsign(callsign);
    SetKeepaliveTime(vuxRealTime);
}

//sfr: vu change
VuSessionEntity::VuSessionEntity(ulong domainMask, const char *callsign):
    VuTargetEntity(VU_SESSION_ENTITY_TYPE, VU_SESSION_ENTITY_ID),
    sessionId_(0), // assigned on session open
    address_(0), //sfr: converts added address
    domainMask_(domainMask),
    callsign_(0),
    loadMetric_(1),
    gameId_(VU_SESSION_NULL_GROUP), // assigned on session open
    groupCount_(0),
    groupHead_(0),
#ifdef VU_SIMPLE_LATENCY
    timeDelta_(0),
    latency_(0),
#endif //VU_SIMPLE_LATENCY
#ifdef VU_TRACK_LATENCY
    timeSyncState_(VU_NO_SYNC),
    latency_(0),
    masterTime_(0),
    masterTimePostTime_(0),
    responseTime_(0),
    masterTimeOwner_(0),
    lagTotal_(0),
    lagPackets_(0),
    lagUpdate_(LAG_COUNT_START),
#endif //VU_TRACK_LATENCY
    lastMsgRecvd_(0),
    game_(0),
    action_(VU_NO_GAME_ACTION)
{
    share_.id_.creator_ = sessionId_;
    share_.id_.num_     = VU_SESSION_ENTITY_ID;
    share_.ownerId_     = share_.id_;   // need to reset this
    SetCallsign(callsign);
    SetKeepaliveTime(vuxRealTime);
}

//sfr: vu change
VuSessionEntity::VuSessionEntity(VU_BYTE** stream, long *rem)
    : VuTargetEntity(stream, rem),
      sessionId_(0),
      address_(0), //sfr: converts added address
      gameId_(0, 0),
      groupCount_(0),
      groupHead_(0),
#ifdef VU_SIMPLE_LATENCY
      timeDelta_(0),
      latency_(0),
#endif //VU_SIMPLE_LATENCY
#ifdef VU_TRACK_LATENCY
      latency_(0),
      masterTime_(0),
      masterTimePostTime_(0),
      responseTime_(0),
      masterTimeOwner_(0),
      lagTotal_(0),
      lagPackets_(0),
      lagUpdate_(LAG_COUNT_START),
#endif //VU_TRACK_LATENCY
      lastMsgRecvd_(0),
      game_(0),
      action_(VU_NO_GAME_ACTION)
{
    VU_BYTE len;

    share_.ownerId_ = Id();

    memcpychk(&lastCollisionCheckTime_,  stream, sizeof(VU_TIME), rem);
    memcpychk(&sessionId_,               stream, sizeof(VU_SESSION_ID), rem);
    //sfr: converts
    address_.Decode(stream, rem);
    memcpychk(&domainMask_,              stream, sizeof(ulong), rem);
    memcpychk(&loadMetric_,              stream, sizeof(VU_BYTE), rem);


    memcpychk(&gameId_,                  stream, sizeof(VU_ID), rem);
    //VU_ID gameId;
    //memcpychk(&gameId,                  stream, sizeof(VU_ID), rem);
    //game_.reset(static_cast<VuGameEntity*>(vuDatabase->Find(gameId)));

    memcpychk(&len,                      stream, sizeof(len), rem);

    VU_ID id;

    for (int i = 0; i < groupCount_; i++)
    {
        memcpychk(&id, stream, sizeof(id), rem);
        AddGroup(id);
    }

    //memcpychk(&bandwidth_,          stream, sizeof(bandwidth_), rem);

#ifdef VU_SIMPLE_LATENCY
    VuSessionEntity *session = (VuSessionEntity*)vuDatabase->Find(Id());
    VU_TIME     rt, gt;
    memcpychk(&rt, stream, sizeof(VU_TIME), rem);
    memcpychk(&gt, stream, sizeof(VU_TIME), rem);

    if (session)
    {
        // Respond to this session with a timing message so it can determine latency
        VuTimingMessage* msg = new VuTimingMessage(vuLocalSession, (VuTargetEntity*)session);
        msg->sessionRealSendTime_ = rt;
        msg->sessionGameSendTime_ = gt;
        msg->remoteGameTime_      = vuxGameTime;
        VuMessageQueue::PostVuMessage(msg);
    }


#endif //VU_SIMPLE_LATENCY
#ifdef VU_TRACK_LATENCY
    memcpychk(&timeSyncState_,      stream, sizeof(timeSyncState_), rem);
    memcpychk(&latency_,            stream, sizeof(latency_), rem);
    memcpychk(&masterTime_,         stream, sizeof(masterTime_), rem);
    memcpychk(&masterTimePostTime_, stream, sizeof(masterTimePostTime_), rem);
    memcpychk(&responseTime_,       stream, sizeof(responseTime_), rem);
    memcpychk(&masterTimeOwner_,    stream, sizeof(masterTimeOwner_), rem);
#endif //VU_TRACK_LATENCY
    memcpychk(&len, stream, sizeof(VU_BYTE), rem);
    //len = (VU_BYTE)**stream;      *stream += sizeof(VU_BYTE);

    callsign_ = new char[len + 1];
    memcpychk(callsign_, stream, len, rem);
    callsign_[len] = '\0';  // null terminate

    // cameras
    unsigned char cameraCount;
    memcpychk(&cameraCount, stream, sizeof(cameraCount), rem);

    for (unsigned char i = 0; i < cameraCount; ++i)
    {
        VU_ID id;
        memcpychk(&id, stream, sizeof(VU_ID), rem);
        VuEntity *e = vuDatabase->Find(id);
        AttachCamera(e, true);
    }
}


//sfr: vu change
VuSessionEntity::VuSessionEntity(FILE* file)
    : VuTargetEntity(file),
      sessionId_(0),
      address_(0), //sfr: converts added address
      gameId_(0, 0),
      groupCount_(0),
      groupHead_(0),
#ifdef VU_TRACK_LATENCY
      latency_(0),
      masterTime_(0),
      masterTimePostTime_(0),
      responseTime_(0),
      masterTimeOwner_(0),
      lagTotal_(0),
      lagPackets_(0),
      lagUpdate_(LAG_COUNT_START),
#endif //VU_TRACK_LATENCY
      lastMsgRecvd_(0),
      game_(0),
      action_(VU_NO_GAME_ACTION)
{
    VU_BYTE len = 0;
    fread(&lastCollisionCheckTime_, sizeof(VU_TIME),       1, file);
    fread(&sessionId_,              sizeof(VU_SESSION_ID), 1, file);
    fread(&domainMask_,             sizeof(ulong),         1, file);
    fread(&loadMetric_,             sizeof(VU_BYTE),       1, file);

    fread(&gameId_,                 sizeof(VU_ID),         1, file);
    //VU_ID gameId;
    //fread(&gameId,                 sizeof(VU_ID),         1, file);
    //game_.reset(static_cast<VuGameEntity*>(vuDatabase->Find(gameId)));

    fread(&len,                     sizeof(len),           1, file);

    VU_ID id;

    for (int i = 0; i < groupCount_; i++)
    {
        fread(&id, sizeof(id), 1, file);
        AddGroup(id);
    }

#ifdef VU_TRACK_LATENCY
    fread(&timeSyncState_,      sizeof(timeSyncState_),      1, file);
    fread(&latency_,            sizeof(latency_),            1, file);
    fread(&masterTime_,         sizeof(masterTime_),         1, file);
    fread(&masterTimePostTime_, sizeof(masterTimePostTime_), 1, file);
    fread(&responseTime_,       sizeof(responseTime_),       1, file);
    fread(&masterTimeOwner_,    sizeof(masterTimeOwner_),    1, file);
#endif

    fread(&len,     sizeof(VU_BYTE), 1, file);
    callsign_ = new char[len + 1];
    fread(callsign_,    len, 1, file);
    callsign_[len] = '\0';  // null terminate

    unsigned char cameraCount;
    fread(&cameraCount, sizeof(cameraCount), 1, file);

    for (unsigned char i = 0; i < cameraCount; ++i)
    {
        VU_ID id;
        fread(&id, sizeof(id), 1, file);
        VuEntity *e = vuDatabase->Find(id);
        AttachCamera(e, true);
    }
}

VuSessionEntity::~VuSessionEntity()
{
    PurgeGroups();
    delete [] callsign_;
}

int VuSessionEntity::LocalSize()
{
    return sizeof(lastCollisionCheckTime_)
           + sizeof(sessionId_)
           + sizeof(address_) //sfr: converts address size
           + sizeof(domainMask_)
           + sizeof(loadMetric_)

           + sizeof(gameId_)
           //+ sizeof(VU_ID)

           + sizeof(groupCount_)
           + (groupCount_ * sizeof(VU_ID))
#ifdef VU_SIMPLE_LATENCY
           + sizeof(VU_TIME)
           + sizeof(VU_TIME)
#endif
#ifdef VU_TRACK_LATENCY
           + sizeof(timeSyncState_)
           + sizeof(latency_)
           + sizeof(masterTime_)
           + sizeof(masterTimePostTime_)
           + sizeof(responseTime_)
           + sizeof(masterTimeOwner_)
#endif
           + strlen(callsign_) + 1
           // camera
           + sizeof(unsigned char)
           + (cameras_.size() * sizeof(VU_ID))
           ;
}

int VuSessionEntity::SaveSize()
{
    return VuTargetEntity::SaveSize() + VuSessionEntity::LocalSize();
}

int VuSessionEntity::Save(VU_BYTE** stream)
{
    int     retval = 0;
    VU_BYTE len    = static_cast<VU_BYTE>(strlen(callsign_));

    SetKeepaliveTime(vuxRealTime);
    retval = VuTargetEntity::Save(stream);

    memcpy(*stream, &lastCollisionCheckTime_, sizeof(VU_TIME));
    *stream += sizeof(VU_TIME);
    memcpy(*stream, &sessionId_,              sizeof(VU_SESSION_ID));
    *stream += sizeof(VU_SESSION_ID);
    address_.Encode(stream);
    memcpy(*stream, &domainMask_,             sizeof(ulong));
    *stream += sizeof(ulong);
    memcpy(*stream, &loadMetric_,             sizeof(VU_BYTE));
    *stream += sizeof(VU_BYTE);

    memcpy(*stream, &gameId_,                 sizeof(VU_ID));
    *stream += sizeof(VU_ID);
    //VU_ID gameId = game_ ? game_->Id() : vuNullId;
    //memcpy(*stream, &gameId,                 sizeof(VU_ID));         *stream += sizeof(VU_ID);

    memcpy(*stream, &groupCount_,             sizeof(groupCount_));
    *stream += sizeof(groupCount_);

    VuGroupNode* gnode = groupHead_;

    while (gnode)
    {
        memcpy(*stream, &gnode->gid_, sizeof(gnode->gid_));
        *stream += sizeof(gnode->gid_);
        gnode = gnode->next_;
    }

#ifdef VU_SIMPLE_LATENCY
    memcpy(*stream, &vuxRealTime, sizeof(VU_TIME));
    *stream += sizeof(VU_TIME);
    memcpy(*stream, &vuxGameTime, sizeof(VU_TIME));
    *stream += sizeof(VU_TIME);
#endif

#ifdef VU_TRACK_LATENCY

    if (TimeSyncState() == VU_MASTER_SYNC)
    {
        masterTime_ = vuxRealTime;
    }

    responseTime_ = vuxRealTime;
    memcpy(*stream, &timeSyncState_,      sizeof(timeSyncState_));
    *stream += sizeof(timeSyncState_);
    memcpy(*stream, &latency_,            sizeof(latency_));
    *stream += sizeof(latency_);
    memcpy(*stream, &masterTime_,         sizeof(masterTime_));
    *stream += sizeof(masterTime_);
    memcpy(*stream, &masterTimePostTime_, sizeof(masterTimePostTime_));
    *stream += sizeof(masterTimePostTime_);
    memcpy(*stream, &responseTime_,       sizeof(responseTime_));
    *stream += sizeof(responseTime_);
    memcpy(*stream, &masterTimeOwner_,    sizeof(masterTimeOwner_));
    *stream += sizeof(masterTimeOwner_);
#endif

    //**stream = len;
    memcpy(*stream, &len, sizeof(VU_BYTE));
    *stream += sizeof(VU_BYTE);
    memcpy(*stream, callsign_, len);
    *stream += len;

    // camera
    unsigned char cameraCount = static_cast<unsigned char>(cameras_.size());
    memcpy(*stream, &cameraCount, sizeof(cameraCount));
    *stream += sizeof(cameraCount);

    for (unsigned char i = 0; i < cameraCount; ++i)
    {
        VU_ID id = cameras_[i]->Id();
        memcpy(*stream, &id, sizeof(id));
        *stream += sizeof(id);
    }

    retval += VuSessionEntity::LocalSize();
    return retval;
}

int VuSessionEntity::Save(FILE* file)
{
    int     retval = 0;
    VU_BYTE len    = 0;

    if (file)
    {
        SetKeepaliveTime(vuxRealTime);

        retval  = VuTargetEntity::Save(file);
        retval += fwrite(&lastCollisionCheckTime_, sizeof(VU_TIME),       1, file);
        retval += fwrite(&sessionId_,              sizeof(VU_SESSION_ID), 1, file);
        retval += fwrite(&domainMask_,             sizeof(ulong),         1, file);
        retval += fwrite(&loadMetric_,             sizeof(VU_BYTE),       1, file);

        retval += fwrite(&gameId_,                 sizeof(VU_ID),         1, file);
        //VU_ID gameId = game_ ? game_->Id() : vuNullId;
        //retval += fwrite(&gameId,                 sizeof(VU_ID),         1, file);

        retval += fwrite(&groupCount_,             sizeof(groupCount_),   1, file);

        VuGroupNode* gnode = groupHead_;

        while (gnode)
        {
            retval += fwrite(&gnode->gid_, sizeof(gnode->gid_), 1, file);
            gnode = gnode->next_;
        }

#ifdef VU_TRACK_LATENCY
        responseTime_ = vuxRealTime;
        retval += fwrite(&timeSyncState_,      sizeof(timeSyncState_),      1, file);
        retval += fwrite(&latency_,            sizeof(latency_),            1, file);
        retval += fwrite(&masterTime_,         sizeof(masterTime_),         1, file);
        retval += fwrite(&masterTimePostTime_, sizeof(masterTimePostTime_), 1, file);
        retval += fwrite(&responseTime_,       sizeof(responseTime_),       1, file);
        retval += fwrite(&masterTimeOwner_,    sizeof(masterTimeOwner_),    1, file);
        len     = strlen(callsign_);
#endif
        retval += fwrite(&len,                 sizeof(VU_BYTE),             1, file);
        retval += fwrite(callsign_,            len,                         1, file);

        // cameras
        unsigned char cameraCount = static_cast<unsigned char>(cameras_.size());
        retval += fwrite(&cameraCount, sizeof(cameraCount), 1, file);

        for (unsigned char i = 0; i < cameraCount; ++i)
        {
            VU_ID id = cameras_[i]->Id();
            retval += fwrite(&id, sizeof(VU_ID), 1, file);
        }
    }

    return retval;
}

VU_BOOL VuSessionEntity::IsSession()
{
    return TRUE;
}

VU_BOOL VuSessionEntity::HasTarget(VU_ID id)
{
    if (id == Id())
    {
        return TRUE;
    }

    return FALSE;
}

VU_BOOL VuSessionEntity::InTarget(VU_ID id)
{
    if (id == Id() or id == GameId() or id == vuGlobalGroup->Id())
    {
        return TRUE;
    }

    VuEnterCriticalSection();
    VuGroupNode* gnode = groupHead_;

    while (gnode)
    {
        if (gnode->gid_ == id)
        {
            VuExitCriticalSection();
            return TRUE;
        }

        gnode = gnode->next_;
    }

    VuExitCriticalSection();
    return FALSE;
}


// begin camera
VuEntity *VuSessionEntity::GetCameraEntity(unsigned char index) const
{
    if (index >= cameras_.size())
    {
        return NULL;
    }

    return cameras_[index].get();
}

int VuSessionEntity::AttachCamera(VuEntity *e, bool silent)
{
    if ((cameras_.size() >= VU_MAX_SESSION_CAMERAS) or (e == NULL))
    {
        return VU_ERROR;
    }

    VuBin<VuEntity> bin(e);
    cameras_.push_back(bin);

    if ( not silent)
    {
        SetDirty();
    }

    return VU_SUCCESS;
}

int VuSessionEntity::RemoveCamera(VuEntity *e, bool silent)
{
    if (e == NULL)
    {
        return VU_ERROR;
    }

    for (VuEntityVectorIterator it = cameras_.begin(); it not_eq cameras_.end(); ++it)
    {
        VuEntity *ce = it->get();

        if (ce->Id() == e->Id())
        {
            cameras_.erase(it);

            if ( not silent)
            {
                SetDirty();
            }

            return VU_SUCCESS;
        }
    }

    return VU_ERROR;
}

void VuSessionEntity::ClearCameras(bool silent)
{
    cameras_.clear();

    if ( not silent)
    {
        SetDirty();
    }
}
// end camera

VU_ERRCODE VuSessionEntity::AddGroup(VU_ID gid)
{
    VU_ERRCODE retval = VU_ERROR;

    if (vuGlobalGroup and gid == vuGlobalGroup->Id())
    {
        return VU_NO_OP;
    }

    VuEnterCriticalSection();
    VuGroupNode* gnode = groupHead_;
    // make certain group isn't already here

    while (gnode and gnode->gid_ not_eq gid)
    {
        gnode = gnode->next_;
    }

    if ( not gnode)
    {
        gnode        = new VuGroupNode;
        gnode->gid_  = gid;
        gnode->next_ = groupHead_;
        groupHead_   = gnode;
        groupCount_++;

        retval = VU_SUCCESS;
    }

    VuExitCriticalSection();
    return retval;
}

VU_ERRCODE VuSessionEntity::RemoveGroup(VU_ID gid)
{
    VU_ERRCODE retval = VU_ERROR;
    VuEnterCriticalSection();

    VuGroupNode* gnode    = groupHead_;
    VuGroupNode* lastnode = 0;

    while (gnode and gnode->gid_ not_eq gid)
    {
        lastnode = gnode;
        gnode    = gnode->next_;
    }

    if (gnode)   // found group
    {
        if (lastnode)
        {
            lastnode->next_ = gnode->next_;
        }
        else   // node was head
        {
            groupHead_ = gnode->next_;
        }

        groupCount_--;
        delete gnode;
        retval = VU_SUCCESS;
    }

    VuExitCriticalSection();
    return retval;
}

VU_ERRCODE VuSessionEntity::PurgeGroups()
{
    VU_ERRCODE retval = VU_NO_OP;
    VuEnterCriticalSection();
    VuGroupNode *gnode;

    while (groupHead_)
    {
        gnode      = groupHead_;
        groupHead_ = groupHead_->next_;
        delete gnode;
        retval     = VU_SUCCESS;
    }

    groupCount_ = 0;
    VuExitCriticalSection();
    return retval;
}

VuTargetEntity * VuSessionEntity::ForwardingTarget(VuMessage *)
{
    VuTargetEntity* retval = Game();

    if ( not retval)
    {
        retval = vuGlobalGroup;
    }

    return retval;
}

void VuSessionEntity::SetCallsign(const char *callsign)
{
    char* oldsign = callsign_;

    SetDirty();

    if (callsign)
    {
        int len = strlen(callsign);

        if (len < MAX_VU_STR_LEN)
        {
            callsign_ = new char[len + 1];
            strcpy(callsign_, callsign);
        }
        else
        {
            callsign_ = new char[MAX_VU_STR_LEN + 1];
            strncpy(callsign_, callsign, MAX_VU_STR_LEN);
        }
    }
    else
    {
        callsign_ = new char[strlen(VU_DEFAULT_PLAYER_NAME) + 1];
        strcpy(callsign_, VU_DEFAULT_PLAYER_NAME);
    }

    if (oldsign)
    {
        delete [] oldsign;

        if (this == vuLocalSessionEntity)
        {
            VuSessionEvent *event =
                new VuSessionEvent(this, VU_SESSION_CHANGE_CALLSIGN, vuGlobalGroup);
            VuMessageQueue::PostVuMessage(event);
        }
    }
}

VU_ERRCODE VuSessionEntity::InsertionCallback()
{
    if (this not_eq vuLocalSessionEntity)
    {
        vuLocalSessionEntity->SetDirty();
        VuxSessionConnect(this);

        if (Game())
        {
            action_ = VU_JOIN_GAME_ACTION;
            Game()->AddSession(this);
        }

        VuEnterCriticalSection();
        VuGroupNode* gnode = groupHead_;

        while (gnode)
        {
            VuGroupEntity* group = (VuGroupEntity*)vuDatabase->Find(gnode->gid_);

            if (group and group->IsGroup())
            {
                group->AddSession(this);
            }

            gnode = gnode->next_;
        }

        VuExitCriticalSection();
        return VU_SUCCESS;
    }

    return VU_NO_OP;
}

//sfr: vu change
// actually I think i didint change this one...
VU_ERRCODE VuSessionEntity::RemovalCallback()
{
    if (this not_eq vuLocalSessionEntity)
    {
        VuEnterCriticalSection();
        VuGroupNode* gnode = groupHead_;

        while (gnode)
        {
            VuGroupEntity* group = (VuGroupEntity*)vuDatabase->Find(gnode->gid_);

            if (group and group->IsGroup())
            {
                VuxGroupRemoveSession(group, this);
            }

            gnode = gnode->next_;
        }

        VuExitCriticalSection();

        if (Game())
        {
            VuxGroupRemoveSession(Game(), this);
        }

        VuxSessionDisconnect(this);
        return VU_SUCCESS;
    }

    return VU_NO_OP;
}

//sfr: vu change
VU_SESSION_ID VuSessionEntity::OpenSession()
{
    if ( not IsLocal() or not vuGlobalGroup or not vuGlobalGroup->Connected())
    {
        return 0;
    }

    com_API_handle ch = vuGlobalGroup->GetCommsHandle();

    if ( not ch)
    {
        ch = vuGlobalGroup->GetReliableCommsHandle();
    }

    if ( not ch)
    {
        return 0;
    }

    // get my ID
    unsigned long id;
    ComAPIHostIDGet(ch, (char*)&id, 1);
    sessionId_ = id;

    if (sessionId_ not_eq vuLocalSession.creator_)
    {
        VuReferenceEntity(this);
        // temporarily make private to prevent sending of bogus delete message
        share_.flags_.breakdown_.private_ = 1;
        vuDatabase->Remove(this);
        share_.flags_.breakdown_.private_ = 0;
        share_.ownerId_.creator_ = sessionId_;

        //  - reset ownerId for all local ent's
        VuDatabaseIterator iter;
        VuEntity *ent = 0;

        for (ent = iter.GetFirst(); (ent not_eq NULL); ent = iter.GetNext())
        {
            if (
                (ent->OwnerId() == vuLocalSession) and 
                (ent not_eq vuPlayerPoolGroup) and 
                (ent not_eq vuGlobalGroup)
            )
            {
                ent->SetOwnerId(OwnerId());
            }
        }

        VuMessageQueue::FlushAllQueues();
        share_.id_.creator_ = sessionId_;
        vuLocalSession = OwnerId();
        //SetVuState(VU_MEM_ACTIVE);
        vuDatabase->/*Quick*/Insert(this);
        VuDeReferenceEntity(this);
    }
    else
    {
        VuMessage *dummyCreateMessage = new VuCreateEvent(this, vuGlobalGroup);
        dummyCreateMessage->RequestReliableTransmit();
        VuMessageQueue::PostVuMessage(dummyCreateMessage);
    }

    //  danm_note: not needed, as vuGlobalGroup membership is implicit
    //  JoinGroup(vuGlobalGroup);
    JoinGame(vuPlayerPoolGroup);
    return sessionId_;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void VuSessionEntity::CloseSession()
{
    VuGroupEntity* game = Game();

    if ( not game)
    {
        // session has no game
        if ( not IsLocal())
        {
            vuDatabase->Remove(this);
        }

        return;
    }

    if (IsLocal())
    {
#ifdef VU_TRACK_LATENCY
        SetTimeSync(VU_NO_SYNC);
#endif
        VuSessionEvent* event = new VuSessionEvent(this, VU_SESSION_CLOSE, vuGlobalGroup);
        VuMessageQueue::PostVuMessage(event);
        VuMessageQueue::FlushAllQueues();

        //VuMainThread::FlushOutboundMessages();
        vuMainThread->FlushOutboundMessages();
        gameId_ = VU_SESSION_NULL_GROUP;
        game_.reset();

        // sfr: session id set to 0, will be reset on open
        sessionId_          = 0;
        share_.id_.creator_ = sessionId_;
        share_.id_.num_     = VU_SESSION_ENTITY_ID;
        share_.ownerId_     = share_.id_;
        vuLocalSession      = OwnerId();
    }
    // session not local
    else
    {
        action_ = VU_LEAVE_GAME_ACTION;
        game->RemoveSession(this);
        game->Distribute(this);

        // game count
        int count = game_->SessionCount();
        // sess is the first session which is not the session being removed
        // it will take control of units owned by session
        VuSessionEntity *sess = 0;

        if (count >= 1)
        {
            VuSessionsIterator iter(Game());
            sess = iter.GetFirst();

            while (sess == this)
            {
                sess = iter.GetNext();
            }
        }

        // iterate over database to remove all entities belonging to the session
        // or transfer ownership to us
        VuDatabaseIterator iter;

        for (
            VuEntity* ent = iter.GetFirst(), *nextEnt;
            ent not_eq NULL;
            ent = nextEnt
        )
        {
            nextEnt = iter.GetNext();

            // checks if entity belongs to session
            if ((ent not_eq this) and (ent->OwnerId() == Id()) and (ent->IsGlobal()))
            {
                if (
                    (sess == NULL) or (ent->IsTransferrable() == 0) or
                    ((ent->IsGame() and ((VuGameEntity *)ent)->SessionCount() == 0))
                )
                {
                    // entity canot be transfered
                    vuDatabase->Remove(ent);
                }
                else
                {
                    // give control to first session in game
                    ent->SetOwnerId(sess->Id());
                }
            }
        }

        // remove session from database
        // but close its comms before it, to avoid getting dangling comm handlers
        // in the release event
        CloseComms();
        vuDatabase->Remove(this);
    }

    LeaveAllGroups();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VU_ERRCODE VuSessionEntity::JoinGroup(VuGroupEntity* newgroup)
{
    VU_ERRCODE retval = VU_NO_OP;

    if ( not newgroup)
    {
        return VU_ERROR;
    }

    retval = AddGroup(newgroup->Id());

    if (retval not_eq VU_ERROR)
    {
        if (IsLocal())
        {
            VuSessionEvent *event = 0;

            if ( not newgroup->IsLocal() and newgroup->SessionCount() == 0 and 
                newgroup not_eq vuGlobalGroup)
            {
                // we need to transfer it here...
                VuTargetEntity* target = (VuTargetEntity*)vuDatabase->Find(newgroup->OwnerId());

                if (target and target->IsTarget())
                {
                    VuMessage* pull = new VuPullRequest(newgroup->Id(), target);
                    // this would likely result in replicated p2p (evil)
                    // pull->RequestReliableTransport();
                    VuMessageQueue::PostVuMessage(pull);
                }
            }

            event = new VuSessionEvent(this, VU_SESSION_JOIN_GROUP, newgroup);
            VuMessageQueue::PostVuMessage(event);
        }

        retval = newgroup->AddSession(this);
    }

    return retval;
}

VU_ERRCODE VuSessionEntity::LeaveGroup(VuGroupEntity* group)
{
    VU_ERRCODE retval = VU_ERROR;

    if (group)
    {
        retval = RemoveGroup(group->Id());

        if (retval == VU_SUCCESS and group->RemoveSession(this) >= 0 and IsLocal())
        {
            VuSessionEvent *event = new VuSessionEvent(this, VU_SESSION_LEAVE_GROUP, vuGlobalGroup);
            VuMessageQueue::PostVuMessage(event);
            retval = VU_SUCCESS;
        }
    }

    return retval;
}

VU_ERRCODE VuSessionEntity::LeaveAllGroups()
{
    VuEnterCriticalSection();
    VuGroupNode* gnode  = groupHead_;
    VuGroupNode* ngnode = (gnode ? gnode->next_ : 0);

    while (gnode)
    {
        VuGroupEntity* gent = (VuGroupEntity*)vuDatabase->Find(gnode->gid_);

        if (gent and gent->IsGroup())
        {
            LeaveGroup(gent);
        }
        else
        {
            RemoveGroup(gnode->gid_);
        }

        gnode = ngnode;

        if (ngnode)
        {
            ngnode = ngnode->next_;
        }
    }

    VuExitCriticalSection();
    return VU_SUCCESS;
}

VU_ERRCODE VuSessionEntity::JoinGame(VuGameEntity* newgame)
{
    VU_ERRCODE retval = VU_NO_OP;
    VuGameEntity *game = Game();

    if (newgame and game == newgame)
    {
        return VU_NO_OP;
    }

    if (newgame and not game)
    {
        action_ = VU_JOIN_GAME_ACTION;
    }
    else if (newgame and game)
    {
        action_ = VU_CHANGE_GAME_ACTION;
    }
    else
    {
        action_ = VU_LEAVE_GAME_ACTION;
    }

#ifdef VU_TRACK_LATENCY
    SetTimeSync(VU_NO_SYNC);
#endif
    VuSessionEvent* event = 0;

    if (game)
    {
        if (game->RemoveSession(this) >= 0)
        {
            LeaveAllGroups();

            if (IsLocal())
            {
                VuMessageQueue::FlushAllQueues();

                if (game->IsLocal())
                {
                    // we need to transfer it away...
                    VuSessionsIterator iter(Game());
                    VuSessionEntity *sess = iter.GetFirst();

                    while (sess == this)
                    {
                        sess = iter.GetNext();
                    }

                    if (sess)
                    {
                        VuMessage *push = new VuPushRequest(game->Id(), sess);
                        VuMessageQueue::PostVuMessage(push);
                    }
                }

                if (newgame)
                {
                    event         = new VuSessionEvent(this, VU_SESSION_CHANGE_GAME, vuGlobalGroup);
                    event->group_ = newgame->Id();
                }
                else
                {
                    event   = new VuSessionEvent(this, VU_SESSION_CLOSE, vuGlobalGroup);
                    gameId_ = VU_SESSION_NULL_GROUP;
                    game_.reset();
                }

                if (this == vuLocalSessionEntity)
                {
                    VuShutdownEvent* se = new VuShutdownEvent(FALSE);
                    // sfr: calling update hre is a fucking hack
                    // this needs to be removed
#define VU_JOIN_DONT_UPDATE 0
#if VU_JOIN_DONT_UPDATE
                    se->Ref();
                    VuMessageQueue::PostVuMessage(se);

                    while ( not se->done_)
                    {
                        ::Sleep(10);
                    }

                    se->UnRef();
#else
                    VuMessageQueue::PostVuMessage(se);
#if CAP_DISPATCH
                    vuMainThread->Update(-1);
#else
                    vuMainThread->Update();
#endif
                    VuMessageQueue::FlushAllQueues();
#endif
                }
            }
        }
        else
        {
            return VU_ERROR;
        }
    }

    if (newgame)
    {
        game_.reset(newgame);
        gameId_ = newgame->Id();

        if (IsLocal())
        {
            if ( not newgame->IsLocal() and newgame->SessionCount() == 0)
            {
                // we need to transfer it here...
                VuTargetEntity* target =
                    (VuTargetEntity*)vuDatabase->Find(newgame->OwnerId());

                if (target and target->IsTarget())
                {
                    VuMessage* pull = new VuPullRequest(newgame->Id(), target);
                    VuMessageQueue::PostVuMessage(pull);
                }
            }

            if ( not game)
            {
                event = new VuSessionEvent(this, VU_SESSION_JOIN_GAME, vuGlobalGroup);
                event->RequestReliableTransmit();
            }
        }

        retval = newgame->AddSession(this);
    }

    if (event)
    {
        VuMessageQueue::PostVuMessage(event);
    }

    if (newgame == 0 and IsLocal() and vuPlayerPoolGroup)
    {
        retval = JoinGame(vuPlayerPoolGroup);
    }

    return retval;
}

VuGameEntity *VuSessionEntity::Game()
{
    if ( not game_)
    {
        VuEntity *g = vuDatabase->Find(gameId_);

        if (g and g->IsGame())
        {
            game_.reset(static_cast<VuGameEntity*>(g));
        }
    }

    return game_.get();
}

#ifdef VU_TRACK_LATENCY
void VuSessionEntity::SetTimeSync(VU_BYTE newstate)
{
    VU_BYTE oldstate = timeSyncState_;
    timeSyncState_   = newstate;

    if (timeSyncState_ == VU_NO_SYNC)
    {
        latency_            = 0;
        masterTime_         = 0;
        masterTimePostTime_ = 0;
        masterTimeOwner_    = 0;
    }

    if (timeSyncState_ not_eq oldstate and timeSyncState_ not_eq VU_NO_SYNC and this == vuLocalSessionEntity)
    {
        VuMessage* msg = 0;

        if (timeSyncState_ == VU_MASTER_SYNC and Game() and Game()->OwnerId() not_eq vuLocalSession)
        {
            masterTimeOwner_ = Id().creator_;
            // transfer game ownership to new master
            VuTargetEntity* target = (VuTargetEntity*)vuDatabase->Find(Game()->OwnerId());

            if (target and target->IsTarget())
            {
                msg = new VuPullRequest(GameId(), target);
                msg->RequestReliableTransmit();
                VuMessageQueue::PostVuMessage(msg);
            }

            // reset statistics
            VuSessionsIterator iter(Game());
            VuSessionEntity*   ent = iter.GetFirst();

            while (ent)
            {
                ent->lagTotal_   = 0;
                ent->lagPackets_ = 0;
                ent->lagUpdate_  = LAG_COUNT_START;
                ent = iter.GetNext();
            }
        }

        msg = new VuSessionEvent(this, VU_SESSION_TIME_SYNC, vuGlobalGroup);
        VuMessageQueue::PostVuMessage(msg);
    }
}

void VuSessionEntity::SetLatency(VU_TIME latency)
{
    if (this == vuLocalSessionEntity and latency not_eq latency_)
    {
        VU_TIME oldlatency = latency_;
        latency_ = latency;
        VuxAdjustLatency(latency_, oldlatency);
    }
}
#endif //VU_TRACK_LATENCY

VU_ERRCODE VuSessionEntity::Handle(VuEvent* event)
{
    int retval = VU_NO_OP;

    switch (event->Type())
    {
        case VU_DELETE_EVENT:
#if not NO_RELEASE_EVENT
        case VU_RELEASE_EVENT:
#endif
            if (Game() and this not_eq vuLocalSessionEntity)
            {
                JoinGame(vuPlayerPoolGroup);
            }

            retval = VU_SUCCESS;
            break;

        default:
            // do nothing
            break;
    }

    return retval;
}

void VuSessionEntity::SetKeepaliveTime(VU_TIME ts)
{
    SetCollisionCheckTime(ts);
}

VU_TIME VuSessionEntity::KeepaliveTime()
{
    return LastCollisionCheckTime();
}

VU_ERRCODE VuSessionEntity::Handle(VuFullUpdateEvent* event)
{

    if (event->EventData())
    {
        SetKeepaliveTime(vuxRealTime);

        if (this not_eq vuLocalSessionEntity)
        {
            VuSessionEntity* sData = static_cast<VuSessionEntity*>(event->EventData());

#ifdef VU_TRACK_LATENCY

            if (vuLocalSessionEntity->TimeSyncState() == VU_MASTER_SYNC)
            {
                // gather statistics
                if (vuLocalSessionEntity->masterTimeOwner_ ==
                    sData->masterTimeOwner_ and 
                    latency_ == sData->Latency())
                {

                    int lag = ((event->PostTime() - sData->masterTime_) -
                               (sData->responseTime_ - sData->masterTimePostTime_));
                    lag = lag / 2;

                    if (lag < 0) lag = 0;

                    lagTotal_ += lag;

                    lagPackets_++;

                    if (lagPackets_ >= lagUpdate_)
                    {
                        VU_TIME newlatency = lagTotal_ / lagPackets_;

                        if (newlatency not_eq latency_)
                        {
                            VuSessionEvent *event =
                                new VuSessionEvent(this, VU_SESSION_LATENCY_NOTICE, Game());
                            event->gameTime_ = newlatency;
                            VuMessageQueue::PostVuMessage(event);
                        }

                        latency_    = newlatency;
                        lagTotal_   = 0;
                        lagPackets_ = 0;
                        lagUpdate_  = lagUpdate_ * 2;
                    }
                }
            }
            else if (sData->TimeSyncState() == VU_MASTER_SYNC and 
                     vuLocalSessionEntity->GameId() == GameId())
            {
                vuLocalSessionEntity->masterTime_         = sData->masterTime_;
                vuLocalSessionEntity->masterTimePostTime_ = event->PostTime();
                vuLocalSessionEntity->masterTimeOwner_    = Id().creator_;
            }

#endif //VU_TRACK_LATENCY

            if (strcmp(callsign_, sData->callsign_))
            {
                SetCallsign(sData->callsign_);
            }

            // copy cameras
            ClearCameras(true);
            unsigned char cams = sData->CameraCount();

            for (unsigned char i = 0; i < cams; ++i)
            {
                AttachCamera(sData->GetCameraEntity(i), true);
            }
        }
    }

    return VuEntity::Handle(event);
}

VU_ERRCODE VuSessionEntity::Handle(VuSessionEvent* event)
{
    int retval = VU_SUCCESS;
    SetKeepaliveTime(vuxRealTime);

    switch (event->subtype_)
    {
        case VU_SESSION_CLOSE:
        {
            CloseSession();
            break;
        }

        case VU_SESSION_JOIN_GAME:
        {
            VuGameEntity* game = (VuGameEntity*)vuDatabase->Find(event->group_);

            if (game and game->IsGame())
            {
                game->Distribute(this);
                JoinGame(game);
            }
            else
            {
                retval = 0;
            }

            break;
        }

        case VU_SESSION_CHANGE_GAME:
        {
            retval = 0;

            if (Game())
            {
                Game()->Distribute(this);
                VuGameEntity* game = (VuGameEntity*)vuDatabase->Find(event->group_);

                if (game)
                {
                    if (game->IsGame())
                    {
                        retval = 1;
                        game->Distribute(this);
                        JoinGame(game);
                    }
                }
                else
                {
                    // MonoPrint ("Session refers to a game that does not exist\n");
                    VuTimerEvent *timer = new VuTimerEvent(0, vuxRealTime + 1000, VU_DELAY_TIMER, event);
                    VuMessageQueue::PostVuMessage(timer);
                }
            }

            break;
        }

        case VU_SESSION_JOIN_GROUP:
        {
            VuGroupEntity* group = (VuGroupEntity*)vuDatabase->Find(event->group_);

            if (group and group->IsGroup())
            {
                JoinGroup(group);
            }
            else
            {
                retval = 0;
            }

            break;
        }

        case VU_SESSION_LEAVE_GROUP:
        {
            VuGroupEntity* group = (VuGroupEntity*)vuDatabase->Find(event->group_);

            if (group and group->IsGroup())
            {
                LeaveGroup(group);
            }
            else
            {
                retval = 0;
            }

            break;
        }

        case VU_SESSION_DISTRIBUTE_ENTITIES:
        {
            VuGameEntity* game = (VuGameEntity*)vuDatabase->Find(event->group_);

            if (game)
            {
                game->Distribute(this);
            }

            break;
        }

        case VU_SESSION_CHANGE_CALLSIGN:
        {
            SetCallsign(event->callsign_);
            break;
        }

#ifdef VU_TRACK_LATENCY

        case VU_SESSION_TIME_SYNC:
        {
            SetTimeSync(event->syncState_);

            if (event->syncState_ == VU_MASTER_SYNC and 
                vuLocalSessionEntity->Game() == Game() and 
                vuLocalSessionEntity->TimeSyncState() not_eq VU_MASTER_SYNC)
            {
                vuLocalSessionEntity->masterTimeOwner_ = Id().creator_;
            }

            break;
        }

        case VU_SESSION_LATENCY_NOTICE:
        {
            SetLatency(event->gameTime_);
            break;
        }

#endif

        default:
        {
            // do nothing
            retval = 0;
            break;
        }
    }

    return retval;
}

// sfr: position update stuff
void VuSessionEntity::EnqueueOobPositionUpdate(VuEntity *entity)
{
    oobPositionUpdateQ_.push_back(entity);
}

void VuSessionEntity::EnqueuePositionUpdate(SM_SCALAR distance, VuEntity *entity)
{
#if SESSION_USES_LIST_FOR_PU
    positionUpdateQ_.push_back(make_pair(distance, entity));
#else
    positionUpdateQ_.insert(make_pair(distance, entity));
#endif
}


void VuSessionEntity::SendBestEnqueuedPositionUpdatesAndClear(unsigned int qty, VU_TIME timestamp)
{
    // send OOB
    for (
        VuOobPositionUpdateQ::iterator it = oobPositionUpdateQ_.begin();
        it not_eq oobPositionUpdateQ_.end();
        ++it
    )
    {
        VuEntity *e = *it;

        // avoid duplicate sends
        if ( not e->EnqueuedForPositionUpdate())
        {
            e->SetEnqueuedForPositionUpdate(true);
            static_cast<VuMaster*>(e->EntityDriver())->GeneratePositionUpdate(false, true, timestamp, this);

            if (qty > 0)
            {
                --qty;
            }
        }
    }

#if SESSION_USES_LIST_FOR_PU
    // sort the updates from lower to higher
    positionUpdateQ_.sort(ScoreEntityPairSort());
#endif

    // respect BW
    for (
        VuPositionUpdateQ::iterator it = positionUpdateQ_.begin();
        it not_eq positionUpdateQ_.end() and (qty-- > 0);
        ++it
    )
    {
        VuEntity *e = (it->second);

        // no duplicates
        if ( not e->EnqueuedForPositionUpdate())
        {
            e->SetEnqueuedForPositionUpdate(false);
            static_cast<VuMaster*>(e->EntityDriver())->GeneratePositionUpdate(false, false, timestamp, this);
        }
    }

    // clear all and unset queued
    while ( not positionUpdateQ_.empty())
    {
        VuEntity *e = positionUpdateQ_.front().second;
        e->SetEnqueuedForPositionUpdate(false);
        positionUpdateQ_.pop_front();
    }

    while ( not oobPositionUpdateQ_.empty())
    {
        VuEntity *e = oobPositionUpdateQ_.front();
        e->SetEnqueuedForPositionUpdate(false);
        oobPositionUpdateQ_.pop_front();
    }
}

VU_ID VuSessionEntity::GameId()
{
    return gameId_;
    //return game_ ? game_->Id() : vuNullId;
}

//-----------------------------------------------------------------------------
// VuGroupEntity
//-----------------------------------------------------------------------------


VuGroupEntity::VuGroupEntity(char* groupname) :
    VuTargetEntity(VU_GROUP_ENTITY_TYPE, VuxGetId()), groupName_(0), sessionMax_(VU_DEFAULT_GROUP_SIZE)
{
    groupName_ = new char[strlen(groupname) + 1];
    strcpy(groupName_, groupname);
    VuSessionFilter filter(Id());
    sessionCollection_ = new VuOrderedList(&filter);
    sessionCollection_->Register();
}


VuGroupEntity::VuGroupEntity(int type, char *gamename, VuFilter* filter)
    : VuTargetEntity(type, VuxGetId()), groupName_(0), sessionMax_(VU_DEFAULT_GROUP_SIZE)
{
    groupName_ = new char[strlen(gamename) + 1];
    strcpy(groupName_, gamename);
    VuSessionFilter sfilter(Id());

    if ( not filter) filter = &sfilter;

    sessionCollection_ = new VuOrderedList(filter);
    sessionCollection_->Register();
}

//sfr: vu change
VuGroupEntity::VuGroupEntity(VU_BYTE** stream, long *rem)
    : VuTargetEntity(stream, rem), selfIndex_(-1)
{
    VU_BYTE len;
    VU_ID sessionid(0, 0);
    VuSessionFilter filter(Id());
    sessionCollection_ = new VuOrderedList(&filter);
    sessionCollection_->Register();

    // VuEntity part
    memcpychk(&share_.ownerId_.creator_, stream, sizeof(share_.ownerId_.creator_), rem);
    memcpychk(&share_.ownerId_.num_, stream, sizeof(share_.ownerId_.num_), rem);
    memcpychk(&share_.assoc_.creator_, stream, sizeof(share_.assoc_.creator_), rem);
    memcpychk(&share_.assoc_.num_, stream, sizeof(share_.assoc_.num_), rem);

    // vuGroupEntity part
    memcpychk(&len, stream, sizeof(VU_BYTE), rem);
    groupName_ = new char[len + 1];
    memcpychk(groupName_, stream, len, rem);
    groupName_[len] = '\0'; // null terminate
    memcpychk(&sessionMax_, stream, sizeof(ushort), rem);
    short count = 0;
    memcpychk(&count, stream, sizeof(short), rem);

    for (int i = 0; i < count; i++)
    {
        memcpychk(&sessionid, stream, sizeof(VU_ID), rem);

        if (sessionid == vuLocalSession)
        {
            selfIndex_ = i;
        }

        AddSession(sessionid);
    }
}

//sfr: vu change
VuGroupEntity::VuGroupEntity(FILE* file) : VuTargetEntity(file)
{
    VU_BYTE len = 0;
    VU_ID sessionid(0, 0);
    VuSessionFilter filter(Id());
    sessionCollection_ = new VuOrderedList(&filter);
    sessionCollection_->Register();

    // VuEntity part
    fread(&share_.ownerId_.creator_, sizeof(share_.ownerId_.creator_), 1, file);
    fread(&share_.ownerId_.num_,     sizeof(share_.ownerId_.num_),     1, file);
    fread(&share_.assoc_.creator_,   sizeof(share_.assoc_.creator_),   1, file);
    fread(&share_.assoc_.num_,       sizeof(share_.assoc_.num_),       1, file);

    // vuGroupEntity part
    fread(&len, sizeof(VU_BYTE), 1, file);
    groupName_ = new char[len + 1];
    fread(groupName_, len, 1, file);
    groupName_[len] = '\0'; // null terminate
    fread(&sessionMax_, sizeof(ushort), 1, file);
    short count = 0;
    fread(&count, sizeof(short), 1, file);

    for (int i = 0; i < count; i++)
    {
        fread(&sessionid, sizeof(VU_ID), 1, file);
        AddSession(sessionid);
    }
}

VuGroupEntity::~VuGroupEntity()
{
    delete [] groupName_;
    // sfr: not necessary (already done on destructor) ?
    sessionCollection_->Purge();
    sessionCollection_->Unregister();
    delete sessionCollection_;
}

int VuGroupEntity::LocalSize()
{
    short count = static_cast<short>(sessionCollection_->Count());

    return
        sizeof(share_.ownerId_.creator_)
        + sizeof(share_.ownerId_.num_)
        + sizeof(share_.assoc_.creator_)
        + sizeof(share_.assoc_.num_)
        + strlen(groupName_) + 1
        + sizeof(sessionMax_)
        + sizeof(count)
        + (sizeof(VU_ID) * count)  // sessions
        ;
}

int VuGroupEntity::SaveSize()
{
    return VuTargetEntity::SaveSize() + VuGroupEntity::LocalSize();
}

int VuGroupEntity::Save(VU_BYTE** stream)
{
    VU_BYTE len;

    int retval = VuTargetEntity::Save(stream);

    // VuEntity part
    memcpy(*stream, &share_.ownerId_.creator_, sizeof(share_.ownerId_.creator_));
    *stream += sizeof(share_.ownerId_.creator_);
    memcpy(*stream, &share_.ownerId_.num_, sizeof(share_.ownerId_.num_));
    *stream += sizeof(share_.ownerId_.num_);
    memcpy(*stream, &share_.assoc_.creator_, sizeof(share_.assoc_.creator_));
    *stream += sizeof(share_.assoc_.creator_);
    memcpy(*stream, &share_.assoc_.num_, sizeof(share_.assoc_.num_));
    *stream += sizeof(share_.assoc_.num_);

    // VuGroupEntity part
    len = static_cast<VU_BYTE>(strlen(groupName_));
    **stream = len;
    *stream += sizeof(VU_BYTE);

    memcpy(*stream, groupName_, len);
    *stream += len;
    memcpy(*stream, &sessionMax_, sizeof(ushort));
    *stream += sizeof(ushort);
    short count = static_cast<short>(sessionCollection_->Count());
    memcpy(*stream, &count, sizeof(short));
    *stream += sizeof(short);
    VuSessionsIterator iter(this);
    VuSessionEntity *ent = iter.GetFirst();

    while (ent)
    {
        VU_ID id = ent->Id();
        memcpy(*stream, &id, sizeof(VU_ID));
        *stream += sizeof(VU_ID);
        ent = iter.GetNext();
    }

    retval += LocalSize();
    return retval;
}

int VuGroupEntity::Save(FILE* file)
{
    int retval = 0;
    VU_BYTE len;

    if (file)
    {
        retval = VuTargetEntity::Save(file);

        // VuEntity part
        retval += fwrite(&share_.ownerId_.creator_, sizeof(share_.ownerId_.creator_), 1, file);
        retval += fwrite(&share_.ownerId_.num_,     sizeof(share_.ownerId_.num_),     1, file);
        retval += fwrite(&share_.assoc_.creator_,   sizeof(share_.assoc_.creator_),   1, file);
        retval += fwrite(&share_.assoc_.num_,       sizeof(share_.assoc_.num_),       1, file);

        len     = static_cast<VU_BYTE>(strlen(groupName_));

        retval += fwrite(&len,         sizeof(VU_BYTE), 1, file);
        retval += fwrite(groupName_,   len,             1, file);
        retval += fwrite(&sessionMax_, sizeof(ushort),  1, file);

        short count = static_cast<short>(sessionCollection_->Count());
        retval += fwrite(&count, sizeof(ushort), 1, file);
        VuSessionsIterator iter(this);
        VuSessionEntity* ent = iter.GetFirst();

        while (ent)
        {
            VU_ID id = ent->Id();
            retval += fwrite(&id, sizeof(VU_ID), 1, file);
            ent = iter.GetNext();
        }
    }

    return retval;
}

VU_BOOL VuGroupEntity::IsGroup()
{
    return TRUE;
}

void VuGroupEntity::SetGroupName(char* groupname)
{
    delete [] groupName_;
    groupName_ = new char[strlen(groupname) + 1];
    strcpy(groupName_, groupname);

    if (IsLocal())
    {
        VuSessionEvent* event =
            new VuSessionEvent(this, VU_SESSION_CHANGE_CALLSIGN, vuGlobalGroup);
        VuMessageQueue::PostVuMessage(event);
    }
}

VU_BOOL VuGroupEntity::HasTarget(VU_ID id)
{
    if (id == Id())
    {
        return TRUE;
    }

    VuSessionEntity* session = (VuSessionEntity*)vuDatabase->Find(id);

    if (session and session->IsSession())
    {
        return SessionInGroup(session);
    }

    return FALSE;
}

VU_BOOL VuGroupEntity::InTarget(VU_ID id)
{
    // supports one level of group nesting: groups are In global group
    if (id == Id() or id == vuGlobalGroup->Id())
    {
        return TRUE;
    }

    return FALSE;
}

VU_BOOL VuGroupEntity::SessionInGroup(VuSessionEntity* session)
{
    VuSessionsIterator iter(this);
    VuEntity* ent = iter.GetFirst();

    while (ent)
    {
        if (ent == session)
        {
            return TRUE;
        }

        ent = iter.GetNext();
    }

    return FALSE;
}

VU_ERRCODE VuGroupEntity::AddSession(VuSessionEntity *session)
{
    short count = static_cast<short>(sessionCollection_->Count());

    if (count >= sessionMax_)
    {
        return VU_ERROR;
    }

    VuxGroupAddSession(this, session);

#if VU_ALL_FILTERED

    // sfr: im not sure this will work, because maybe the check wants to check multiple ids... maybe not
    if ( not sessionCollection_->Find(session))
    {
        return sessionCollection_->Insert(session);
    }

#else

    if ( not sessionCollection_->Find(session->Id()))
    {
        return sessionCollection_->Insert(session);
    }

#endif
    return VU_NO_OP;
}

VU_ERRCODE VuGroupEntity::AddSession(VU_ID sessionId)
{
    VuSessionEntity* session = (VuSessionEntity*)vuDatabase->Find(sessionId);

    if (session and session->IsSession())
    {
        return AddSession(session);
    }

    return VU_ERROR;
}

VU_ERRCODE VuGroupEntity::RemoveSession(VuSessionEntity* session)
{
    if (session not_eq vuLocalSessionEntity)
    {
        VuxGroupRemoveSession(this, session);
    }

    return sessionCollection_->Remove(session);
}

VU_ERRCODE VuGroupEntity::RemoveSession(VU_ID sessionId)
{
    VuSessionEntity* session = (VuSessionEntity*)vuDatabase->Find(sessionId);

    if (session and session->IsSession())
    {
        return RemoveSession(session);
    }

    return VU_ERROR;
}

VU_ERRCODE VuGroupEntity::Handle(VuSessionEvent* event)
{
    int retval = VU_NO_OP;

    switch (event->subtype_)
    {
        case VU_SESSION_CHANGE_CALLSIGN:
            SetGroupName(event->callsign_);
            retval = VU_SUCCESS;
            break;

        case VU_SESSION_DISTRIBUTE_ENTITIES:
            retval = Distribute(0);
            break;

        default:
            break;
    }

    return retval;
}

VU_ERRCODE VuGroupEntity::Handle(VuFullUpdateEvent* event)
{
    // update groups?
    return VuEntity::Handle(event);
}


VU_ERRCODE VuGroupEntity::Distribute(VuSessionEntity *)
{
    // do nothing
    return VU_NO_OP;
}

VU_ERRCODE VuGroupEntity::InsertionCallback()
{
    if (this not_eq vuGlobalGroup)
    {
        VuxGroupConnect(this);
        VuSessionsIterator iter(this);
        VuSessionEntity*   sess = iter.GetFirst();

        while (sess)
        {
            AddSession(sess);
            sess = iter.GetNext();
        }

        return VU_SUCCESS;
    }

    return VU_NO_OP;
}

VU_ERRCODE VuGroupEntity::RemovalCallback()
{
    VuSessionsIterator iter(this);
    VuSessionEntity* sess = iter.GetFirst();

    while (sess)
    {
        RemoveSession(sess);
        sess = iter.GetNext();
    }

    VuxGroupDisconnect(this);
    return VU_SUCCESS;
}

//-----------------------------------------------------------------------------
// VuGlobalGroup
//-----------------------------------------------------------------------------

GlobalGroupFilter globalGrpFilter;

//sfr: vu change
VuGlobalGroup::VuGlobalGroup()
    : VuGroupEntity(VU_GLOBAL_GROUP_ENTITY_TYPE, vuxWorldName, &globalGrpFilter)
{
    // make certain owner is NULL session
    share_.ownerId_     = VU_SESSION_NULL_CONNECTION;
    share_.id_.creator_ = (0);
    share_.id_.num_     = VU_GLOBAL_GROUP_ENTITY_ID;
    sessionMax_         = 1024;
    connected_          = FALSE;
}

VuGlobalGroup::~VuGlobalGroup()
{
    // empty
}

VU_BOOL VuGlobalGroup::HasTarget(VU_ID)
{
    // global group includes everybody
    return TRUE;
}

// virtual function interface -- stubbed out here
int VuGlobalGroup::SaveSize()
{
    return 0;
}

int VuGlobalGroup::Save(VU_BYTE **)
{
    return 0;
}

int VuGlobalGroup::Save(FILE *)
{
    return 0;
}

//-----------------------------------------------------------------------------
// VuGameEntity
//-----------------------------------------------------------------------------

VuGameEntity::VuGameEntity(ulong domainMask, char* gamename):
    VuGroupEntity(VU_GAME_ENTITY_TYPE, VU_GAME_GROUP_NAME),
    domainMask_(domainMask),
    gameName_(0)
{
    // every local game is sent imediately
    SetSendCreate(VuEntity::VU_SC_SEND_OOB);
    gameName_ = new char[strlen(gamename) + 1];
    strcpy(gameName_, gamename);
}

VuGameEntity::VuGameEntity(int type, ulong domainMask, char* gamename, char* groupname):
    VuGroupEntity(type, groupname),
    domainMask_(domainMask),
    gameName_(0)
{
    // every local game is sent imediately
    SetSendCreate(VuEntity::VU_SC_SEND_OOB);
    gameName_ = new char[strlen(gamename) + 1];
    strcpy(gameName_, gamename);
}

VuGameEntity::VuGameEntity(VU_BYTE** stream, long *rem):
    VuGroupEntity(stream, rem)
{
    VU_BYTE len;

    memcpychk(&domainMask_, stream, sizeof(ulong), rem);
    memcpychk(&len, stream, sizeof(VU_BYTE), rem);
    gameName_ = new char[len + 1];
    memcpychk(gameName_, stream, len, rem);
    gameName_[len] = '\0';  // null terminate
#ifdef VU_TRACK_LATENCY
    short count = 0;
    VU_TIME latency;
    memcpychk(&count, stream, sizeof(short), rem);

    for (int i = 0; i < count; i++)
    {
        memcpychk(&latency, stream, sizeof(latency), rem);

        if (selfIndex_ == i and this == vuLocalSessionEntity->Game())
        {
            vuLocalSessionEntity->SetLatency(latency);
        }
    }

#endif //VU_TRACK_LATENCY
}

VuGameEntity::VuGameEntity(FILE* file):
    VuGroupEntity(file)
{
    VU_BYTE len = 0;

    fread(&domainMask_, sizeof(ulong), 1, file);
    fread(&len, sizeof(VU_BYTE), 1, file);
    gameName_ = new char[len + 1];
    fread(gameName_, len, 1, file);
    gameName_[len] = '\0';  // null terminate

#if defined(VU_TRACK_LATENCY)
    short count = 0;
    VU_TIME latency;

    fread(&count, sizeof(count), 1, file);

    for (int i = 0; i < count; i++)
        // just need to read...
        fread(&latency, sizeof(latency), 1, file);

#endif
}

VuGameEntity::~VuGameEntity()
{
    delete [] gameName_;
}

int
VuGameEntity::LocalSize()
{
#ifdef VU_TRACK_LATENCY
    short count = sessionCollection_->Count();
#endif

    return sizeof(domainMask_)
           + strlen(gameName_) + 1
#ifdef VU_TRACK_LATENCY
           + sizeof(count)
           + (sizeof(VU_TIME) * count)  // session latency
#endif //VU_TRACK_LATENCY
           ;
}

int
VuGameEntity::SaveSize()
{
    return VuGroupEntity::SaveSize() + VuGameEntity::LocalSize();
}

int
VuGameEntity::Save(VU_BYTE** stream)
{
    VU_BYTE len;

    int retval = VuGroupEntity::Save(stream);
    memcpy(*stream, &domainMask_, sizeof(ulong));
    *stream += sizeof(ulong);
    len = static_cast<VU_BYTE>(strlen(gameName_));
    **stream = len;
    *stream += sizeof(VU_BYTE);
    memcpy(*stream, gameName_, len);
    *stream += len;

#ifdef VU_TRACK_LATENCY
    short count = sessionCollection_->Count();
    memcpy(*stream, &count, sizeof(short));
    *stream += sizeof(short);
    VuSessionsIterator iter(this);
    VuSessionEntity *ent = iter.GetFirst();

    while (ent)
    {
        VU_TIME latency = ent->Latency();
        memcpy(*stream, &latency, sizeof(latency));
        *stream += sizeof(latency);
        ent = iter.GetNext();
    }

#endif

    retval += LocalSize();
    return retval;
}

int
VuGameEntity::Save(FILE* file)
{
    int retval = 0;
    VU_BYTE len;

    if (file)
    {
        retval  = VuGroupEntity::Save(file);
        retval += fwrite(&domainMask_, sizeof(ulong), 1, file);
        len = static_cast<VU_BYTE>(strlen(gameName_));
        retval += fwrite(&len, sizeof(VU_BYTE), 1, file);
        retval += fwrite(gameName_, len, 1, file);

#ifdef VU_TRACK_LATENCY
        VuEnterCriticalSection();
        short count = sessionCollection_->Count();
        retval += fwrite(&count, sizeof(ushort), 1, file);
        VuSessionsIterator iter(this);
        VuSessionEntity *ent = iter.GetFirst();

        while (ent)
        {
            VU_TIME latency = ent->Latency();
            retval += fwrite(&latency, sizeof(latency), 1, file);
            ent = iter.GetNext();
        }

        VuExitCriticalSection();
#endif
    }

    return retval;
}

VU_BOOL VuGameEntity::IsGame()
{
    return TRUE;
}

void VuGameEntity::SetGameName(char* gamename)
{
    delete [] gameName_;
    gameName_ = new char[strlen(gamename) + 1];
    strcpy(gameName_, gamename);

    if (IsLocal())
    {
        VuSessionEvent* event = new VuSessionEvent(this, VU_SESSION_CHANGE_CALLSIGN, vuGlobalGroup);
        VuMessageQueue::PostVuMessage(event);
    }
}

VU_ERRCODE VuGameEntity::AddSession(VuSessionEntity* session)
{
    short count = static_cast<short>(sessionCollection_->Count());

    if (count >= sessionMax_)
    {
        return VU_ERROR;
    }

    if (session->IsLocal())
    {
        // connect to all sessions in the group
        VuSessionsIterator siter(this);

        for (VuSessionEntity *s = siter.GetFirst(); s; s = siter.GetNext())
        {
            if (s not_eq vuLocalSessionEntity)
            {
                VuxSessionConnect(s);
            }
        }
    }
    else if (vuLocalSessionEntity->Game() == this)
    {
        // connect to particular session
        VuxSessionConnect(session);
    }

    VuxGroupAddSession(this, session);
#if VU_ALL_FILTERED

    // sfr: im not sure this will work, because maybe the check wants to check multiple ids... maybe not
    if ( not sessionCollection_->Find(session))
    {
        return sessionCollection_->Insert(session);
    }

#else

    if ( not sessionCollection_->Find(session->Id()))
    {
        return sessionCollection_->Insert(session);
    }

#endif
    return VU_NO_OP;
}

VU_ERRCODE VuGameEntity::RemoveSession(VuSessionEntity *session)
{
    if (session->IsLocal())
    {
        // disconnect all sessions
        VuSessionsIterator siter(this);

        for (VuSessionEntity *s = siter.GetFirst(); s; s = siter.GetNext())
        {
            VuxSessionDisconnect(s);
        }

        //    VuxGroupDisconnect(this);
    }
    else
    {
        // just disconnect from particular session
        VuxSessionDisconnect(session);
    }

    VuxGroupRemoveSession(this, session);

    return sessionCollection_->Remove(session);
}

VU_ERRCODE VuGameEntity::Handle(VuSessionEvent* event)
{
    int retval = VU_NO_OP;

    switch (event->subtype_)
    {
        case VU_SESSION_CHANGE_CALLSIGN:
            SetGameName(event->callsign_);
            retval = VU_SUCCESS;
            break;

        case VU_SESSION_DISTRIBUTE_ENTITIES:
            retval = Distribute(0);
            break;

        default:
            break;
    }

    return retval;
}

VU_ERRCODE VuGameEntity::Handle(VuFullUpdateEvent* event)
{
    return VuEntity::Handle(event);
}

VU_ERRCODE VuGameEntity::RemovalCallback()
{
    // take care of sessions we still think are in this game...
    VuSessionsIterator iter(this);

    for (
        VuSessionEntity* sess = iter.GetFirst(), *nextSess;
        sess and (sess->Game() == this);
        sess = nextSess
    )
    {
        nextSess = iter.GetNext();
        sess->JoinGame(vuPlayerPoolGroup);
    }

    VuxGroupDisconnect(this);
    return VU_SUCCESS;
}


VU_ERRCODE VuGameEntity::Distribute(VuSessionEntity* sess)
{
    if ( not sess or (Id() == sess->GameId()))
    {
        int i;
        int count[32];
        int totalcount = 0;
        int myseedlower[32];
        int myseedupper[32];

        for (i = 0; i < 32; i++)
        {
            count[i] = 0;
            myseedlower[i] = 0;
            myseedupper[i] = vuLocalSessionEntity->LoadMetric() - 1;
        }

        VuSessionsIterator siter(this);
        VuSessionEntity *cs = siter.GetFirst();

        while (cs)
        {
            if ( not sess or sess->Id() not_eq cs->Id())
            {
                for (i = 0; i < 32; i++)
                {
                    if (1 << i bitand cs->DomainMask())
                    {
                        count[i] += cs->LoadMetric();
                        totalcount++;
                    }
                }
            }

            cs = siter.GetNext();
        }

        if (totalcount == 0)
        {
            // nothing to do  just return...
            return VU_NO_OP;
        }

        cs = siter.GetFirst();

        while (cs and cs not_eq vuLocalSessionEntity)
        {
            if ( not sess or sess->Id() not_eq cs->Id())
            {
                for (i = 0; i < 32; i++)
                {
                    if (1 << i bitand cs->DomainMask())
                    {
                        myseedlower[i] += cs->LoadMetric();
                        myseedupper[i] += cs->LoadMetric();
                    }
                }
            }

            cs = siter.GetNext();
        }

        if ( not cs)
        {
            if ( not sess)
            {
                // note: something is terribly amiss... so bail...
                return VU_ERROR;
            }
            else
            {
                // ... most likely, we did not find any viable sessions
                VuDatabaseIterator dbiter;
                VuEntity *ent = dbiter.GetFirst(), *next;

                while (ent)
                {
                    next = dbiter.GetNext();

                    if ((ent->OwnerId().creator_ == sess->SessionId()) and (sess not_eq ent) and 
                        (sess->VuState() not_eq VU_MEM_ACTIVE or
                         (ent->IsTransferrable() and not ent->IsGlobal())))
                    {
                        vuDatabase->Remove(ent);
                    }

                    ent = next;
                }
            }
        }
        else if (Id() == vuLocalSessionEntity->GameId())
        {
            VuDatabaseIterator dbiter;
            VuEntity *ent = dbiter.GetFirst(), *next;
            VuEntity *test_ent = ent;
            VuEntity *ent2;
            int index;

            while (ent)
            {
                next = dbiter.GetNext();

                if ( not ent->IsTransferrable() and 
                    sess and sess->VuState() not_eq VU_MEM_ACTIVE  and 
                    ent->OwnerId().creator_ == sess->SessionId())
                {
                    vuDatabase->Remove(ent);
                }
                else if (( not sess or (ent->OwnerId().creator_ == sess->SessionId())) and 
                         ((1 << ent->Domain()) bitand vuLocalSessionEntity->DomainMask()))
                {
                    if (ent->Association() not_eq vuNullId and 
                        (ent2 = vuDatabase->Find(ent->Association())) not_eq 0)
                    {
                        test_ent = ent2;
                    }

                    index = test_ent->Id().num_ % count[test_ent->Domain()];

                    if (index >= myseedlower[test_ent->Domain()] and 
                        index <= myseedupper[test_ent->Domain()] and 
                        ent->IsTransferrable() and not ent->IsGlobal())
                    {
                        ent->SetOwnerId(vuLocalSession);
                    }
                }

                ent = next;
                test_ent = ent;
            }
        }
        else
        {
            VuListIterator grpiter(vuGameList);
            VuEntity* ent      = grpiter.GetFirst();
            VuEntity* test_ent = ent;
            VuEntity* ent2;
            int index;

            while (ent)
            {
                if ( not ent->IsTransferrable() and 
                    sess and sess->VuState() not_eq VU_MEM_ACTIVE  and 
                    ent->OwnerId().creator_ == sess->SessionId())
                {
                    vuDatabase->Remove(ent);
                }
                else if (( not sess or (ent->OwnerId().creator_ == sess->SessionId())) and 
                         ((1 << ent->Domain()) bitand vuLocalSessionEntity->DomainMask()))
                {
                    if (ent->Association() not_eq vuNullId and 
                        (ent2 = vuDatabase->Find(ent->Association())) not_eq 0)
                    {
                        test_ent = ent2;
                    }

                    index = test_ent->Id().num_ % count[test_ent->Domain()];

                    if (index >= myseedlower[test_ent->Domain()] and 
                        index <= myseedupper[test_ent->Domain()] and 
                        ent->IsTransferrable() and not ent->IsGlobal() and 
                        ( not sess or sess->VuState() == VU_MEM_ACTIVE))
                    {
                        ent->SetOwnerId(vuLocalSession);
                    }
                }

                ent      = grpiter.GetNext();
                test_ent = ent;
            }
        }
    }

    return VU_SUCCESS;
}

//-----------------------------------------------------------------------------
// VuPlayerPoolGame
//sfr: vu change
//-----------------------------------------------------------------------------
VuPlayerPoolGame::VuPlayerPoolGame(ulong domainMask)
    : VuGameEntity(VU_PLAYER_POOL_GROUP_ENTITY_TYPE, domainMask, VU_PLAYER_POOL_GROUP_NAME, vuxWorldName)
{
    // make certain owner is NULL session
    share_.ownerId_     = VU_SESSION_NULL_CONNECTION;
    share_.id_.creator_ = (0);
    share_.id_.num_     = VU_PLAYER_POOL_ENTITY_ID;
    sessionMax_         = 255;

    // hack, hack, hack up a lung
    sessionCollection_->Unregister();
    delete sessionCollection_;
    VuSessionFilter filter(Id());
    sessionCollection_ = new VuOrderedList(&filter);
    sessionCollection_->Register();
}

VuPlayerPoolGame::~VuPlayerPoolGame()
{
    // empty
}

// virtual function interface -- stubbed out here
int VuPlayerPoolGame::SaveSize()
{
    return 0;
}

int VuPlayerPoolGame::Save(VU_BYTE**)
{
    return 0;
}

int VuPlayerPoolGame::Save(FILE*)
{
    return 0;
}

VU_ERRCODE VuPlayerPoolGame::Distribute(VuSessionEntity *sess)
{
    // just remove all ents managed by session
    if (sess and (Id() == sess->GameId()))
    {
        VuDatabaseIterator dbiter;

        for (
            VuEntity *ent = dbiter.GetFirst(), *next;
            ent not_eq NULL;
            ent = next
        )
        {
            next = dbiter.GetNext();

            if (
                (ent->OwnerId().creator_ == sess->SessionId()) and 
                ( not ent->IsPersistent()) and 
                (sess->VuState() not_eq VU_MEM_ACTIVE or (ent->IsTransferrable() and not ent->IsGlobal()))
            )
            {
                vuDatabase->Remove(ent);
            }
        }
    }

    return VU_SUCCESS;
}



