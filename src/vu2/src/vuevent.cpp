#include <stdio.h>
#include <string.h>

//sfr: for checks
#include "InvalidBufferException.h"

#include "vu2.h"
#include "vu_priv.h"
#include "vu_mq.h"

#define VU_ENCODE_8(B,X)  ((*(char *)(*B))++ = *(char *)(X))
#define VU_ENCODE_16(B,X) ((*(short*)(*B))++ = *(short*)(X))
#define VU_ENCODE_32(B,X) ((*(int  *)(*B))++ = *(int  *)(X))

#define VU_DECODE_8(X,B)  (*(char *)(X) = (*(char *)(*B))++)
#define VU_DECODE_16(X,B) (*(short*)(X) = (*(short*)(*B))++)
#define VU_DECODE_32(X,B) (*(int  *)(X) = (*(int  *)(*B))++)

#define VU_PACK_POSITION


//--------------------------------------------------
//sfr: inverted size and data, rem is now long
static VuEntity *VuCreateEntity(ushort type, VU_BYTE* data, long rem)
{
    VuEntity* retval = 0;

    switch (type)
    {
        case VU_SESSION_ENTITY_TYPE:
            retval = new VuSessionEntity(&data, &rem);
            break;

        case VU_GROUP_ENTITY_TYPE:
            retval = new VuGroupEntity(&data, &rem);
            break;

        case VU_GAME_ENTITY_TYPE:
            retval = new VuGameEntity(&data, &rem);
            break;

        case VU_GLOBAL_GROUP_ENTITY_TYPE:
        case VU_PLAYER_POOL_GROUP_ENTITY_TYPE:
            retval = 0;
            break;
    }

    return retval;
}

//--------------------------------------------------
static VuEntity *ResolveWinner(VuEntity* ent1, VuEntity* ent2)
{
    VuEntity* retval = 0;

    if (ent1->EntityType()->createPriority_ >
        ent2->EntityType()->createPriority_)
    {
        retval = ent1;
    }
    else if (ent1->EntityType()->createPriority_ <
             ent2->EntityType()->createPriority_)
    {
        retval = ent2;
    }
    else if (ent1->OwnerId().creator_ == ent1->Id().creator_)
    {
        retval = ent1;
    }
    else if (ent2->OwnerId().creator_ == ent2->Id().creator_)
    {
        retval = ent2;
    }
    else if (ent1->OwnerId().creator_ < ent2->OwnerId().creator_)
    {
        retval = ent1;
    }
    else
    {
        retval = ent2;
    }

    return retval;
}

// sfr: temp test
int nmsgs;

VuMessage::VuMessage(
    VU_MSG_TYPE     type,
    VU_ID           entityId,
    VuTargetEntity* target,
    VU_BOOL         loopback
)
    : refcnt_(0), type_(type), flags_(VU_NORMAL_PRIORITY_MSG_FLAG), entityId_(entityId),
        target_(target), postTime_(0), ent_(0)
{
    // sfr: temp test
    ++nmsgs;

    if (target == vuLocalSessionEntity)
    {
        loopback = TRUE;
    }

    if (target)
    {
        target_.reset(target);
    }
    else if (vuGlobalGroup)
    {
        target_.reset(vuGlobalGroup);
    }
    else
    {
        target_.reset(vuLocalSessionEntity.get());
    }

    if (target_)
    {
        tgtid_ = target_->Id();
    }

    if (loopback)
    {
        flags_ or_eq VU_LOOPBACK_MSG_FLAG;
    }

    // note: msg id is set only for external messages which are sent out
    sender_.num_     = vuLocalSession.num_;
    sender_.creator_ = vuLocalSession.creator_;
}

//sfr: vu change
VuMessage::VuMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target)
    : refcnt_(0), type_(type), flags_(VU_REMOTE_MSG_FLAG), sender_(senderid), tgtid_(target),
      entityId_(0, 0), target_(0), postTime_(0), ent_(0)
{
    // sfr: temp test
    ++nmsgs;
}

VuMessage::~VuMessage()
{
    // sfr: temp test
    --nmsgs;
    SetEntity(0);
}

VU_BOOL VuMessage::IsLocal() const
{
    return (VU_BOOL)(sender_.creator_ == vuLocalSession.creator_ ? TRUE : FALSE);
}

VU_BOOL VuMessage::DoSend()
{
    return TRUE;
}

/*VuEntity *VuMessage::SetEntity(VuEntity* ent)
{
 // sfr: equality test
 if (ent == ent_){ return ent; }

 // reference new entity
 if (ent){
 VuReferenceEntity(ent);
 }
 // deferefence old
 if (ent_){
 VuDeReferenceEntity(ent_);
 }
 // swap pointers
 ent_ = ent;
 return ent_;
}*/

int VuMessage::Ref()
{
    return ++refcnt_;
}

int VuMessage::UnRef()
{
    // NOTE: must assign temp here as memory may be freed prior to return
    int retval = --refcnt_;

    if (refcnt_ <= 0)
        delete this;

    return retval;
}

int VuMessage::Read(VU_BYTE** buf, long *length)
{
    int retval = Decode(buf, length);
    //assert (*length == 0);

    refcnt_ = 0;
    SetEntity(0);
    return retval;
}

int VuMessage::Write(VU_BYTE** buf)
{
    return Encode(buf);
}

int VuMessage::Send()
{
    int retval = -1;

    if (Target() and Target() not_eq vuLocalSessionEntity)
    {
        retval = Target()->SendMessage(this);

        if (retval <= 0)
        {
            flags_ or_eq VU_SEND_FAILED_MSG_FLAG;
        }
    }

    return retval;
}

VU_ERRCODE VuMessage::Dispatch(VU_BOOL autod)
{
    int retval = VU_NO_OP;

    if ( not IsLocal() or (flags_ bitand VU_LOOPBACK_MSG_FLAG))
    {
        if ( not Entity())
        {
            // try to find ent again -- may have been in queue
            VuEntity *ent = vuDatabase->Find(entityId_);

            if (ent)
            {
                Activate(ent);
            }
        }

        retval = Process(autod);
        vuDatabase->Handle(this);
        // mark as sent
        flags_ or_eq VU_PROCESSED_MSG_FLAG;
    }

    return retval;
}

VU_ERRCODE VuMessage::Activate(VuEntity* ent)
{
    SetEntity(ent);
    return VU_SUCCESS;
}


#define VUMESSAGE_LOCALSIZE (sizeof(entityId_.creator_)+sizeof(entityId_.num_))
int VuMessage::LocalSize() const
{
    return (VUMESSAGE_LOCALSIZE);
}


#define VUMESSAGE_SIZE (VUMESSAGE_LOCALSIZE)
int VuMessage::Size() const
{
    return (VUMESSAGE_SIZE);
}

int VuMessage::Decode(VU_BYTE** buf, long *rem)
{
    // sfr: check creator
    memcpychk(&entityId_.creator_, buf, sizeof(entityId_.creator_), rem);
    memcpychk(&entityId_.num_,     buf, sizeof(entityId_.num_), rem);
    return (VUMESSAGE_LOCALSIZE);
}

int VuMessage::Encode(VU_BYTE** buf)
{
    memcpy(*buf, &entityId_.creator_, sizeof(entityId_.creator_));
    *buf += sizeof(entityId_.creator_);
    memcpy(*buf, &entityId_.num_,     sizeof(entityId_.num_));
    *buf += sizeof(entityId_.num_);

    return (VUMESSAGE_LOCALSIZE);
}

//--------------------------------------------------
VuRequestDummyBlockMessage::VuRequestDummyBlockMessage(VU_ADDRESS address, VuTargetEntity *target) :
    VuMessage(VU_REQUEST_DUMMY_BLOCK_MESSAGE, VU_ID(), target, false),
    address_(address)
{
}

VuRequestDummyBlockMessage::VuRequestDummyBlockMessage(VU_ID sender, VU_ID target) :
    VuMessage(VU_REQUEST_DUMMY_BLOCK_MESSAGE, sender, target)
{
}

// serialize functions
int VuRequestDummyBlockMessage::LocalSize() const
{
    return address_.Size();
}


int VuRequestDummyBlockMessage::Size() const
{
    return VuMessage::Size() + LocalSize();
}

int VuRequestDummyBlockMessage::Decode(VU_BYTE **buf, long *length)
{
    VuMessage::Decode(buf, length);
    address_.Decode(buf, length);
    return Size();
}

int VuRequestDummyBlockMessage::Encode(VU_BYTE **buf)
{
    VuMessage::Encode(buf);
    address_.Encode(buf);
    return Size();
}

VU_ERRCODE VuRequestDummyBlockMessage::Process(VU_BOOL autod)
{
    // send dummy block
    // get any session, we just need the handle send socket
    VuSessionsIterator iter(vuGlobalGroup);
    VuSessionEntity *s;

    for (s = iter.GetFirst(); (s not_eq NULL); s = iter.GetNext())
    {
        if (s->GetCommsHandle() not_eq NULL)
        {
            break;
        }
    }

    VU_ADDRESS sendAddress(address_.ip, address_.recvPort, address_.reliableRecvPort);

    // server sent a private address, use his own
    if (sendAddress.IsPrivate())
    {
        s = (VuSessionEntity*)vuDatabase->Find(sender_);
        sendAddress.ip = s->GetAddress().ip;
    }

    if (s not_eq NULL)
    {
        ComAPISendDummy(s->GetCommsHandle(), sendAddress.ip, sendAddress.recvPort);
        ComAPISendDummy(s->GetCommsHandle(), sendAddress.ip, sendAddress.recvPort + 1);
        ComAPISendDummy(s->GetCommsHandle(), sendAddress.ip, sendAddress.recvPort + 2);
        ComAPISendDummy(s->GetCommsHandle(), sendAddress.ip, sendAddress.recvPort + 3);
    }

    return VU_SUCCESS;
}


VuErrorMessage::VuErrorMessage(int             errorType,
                               VU_ID           srcmsgid,
                               VU_ID           entityId,
                               VuTargetEntity* target)
    : VuMessage(VU_ERROR_MESSAGE, entityId, target, FALSE),
      srcmsgid_(srcmsgid),
      etype_(static_cast<short>(errorType))
{
}

VuErrorMessage::VuErrorMessage(VU_ID     senderid,
                               VU_ID     target)
    : VuMessage(VU_ERROR_MESSAGE, senderid, target),
      etype_(VU_UNKNOWN_ERROR)
{
    srcmsgid_.num_     = 0;
    srcmsgid_.creator_ = (0);
}

VuErrorMessage::~VuErrorMessage()
{
}

#define VUERRORMESSAGE_LOCALSIZE (sizeof(srcmsgid_)+sizeof(etype_))
int VuErrorMessage::LocalSize() const
{
    return (VUERRORMESSAGE_LOCALSIZE);
}

#define VUERRORMESSAGE_SIZE (VUMESSAGE_SIZE+VUERRORMESSAGE_LOCALSIZE)

int VuErrorMessage::Size() const
{
    return (VUERRORMESSAGE_SIZE);
}

int VuErrorMessage::Decode(VU_BYTE** buf, long *rem)
{
    VuMessage::Decode(buf, rem);

    memcpychk(&srcmsgid_, buf, sizeof(srcmsgid_), rem);
    memcpychk(&etype_,    buf, sizeof(etype_), rem);

    return (VUERRORMESSAGE_SIZE);
}

int VuErrorMessage::Encode(VU_BYTE** buf)
{
    VuMessage::Encode(buf);

    memcpy(*buf, &srcmsgid_, sizeof(srcmsgid_));
    *buf += sizeof(srcmsgid_);
    memcpy(*buf, &etype_,    sizeof(etype_));
    *buf += sizeof(etype_);

    return (VUERRORMESSAGE_SIZE);
}

VU_ERRCODE
VuErrorMessage::Process(VU_BOOL)
{
    if (Entity())
    {
        Entity()->Handle(this);
        return VU_SUCCESS;
    }

    return VU_NO_OP;
}

//--------------------------------------------------
VuRequestMessage::VuRequestMessage(VU_MSG_TYPE     type,
                                   VU_ID           entityId,
                                   VuTargetEntity* target)
    : VuMessage(type, entityId, target, FALSE)
{
    // empty
}

VuRequestMessage::VuRequestMessage(VU_MSG_TYPE type,
                                   VU_ID       senderid,
                                   VU_ID       dest)
    : VuMessage(type, senderid, dest)
{
    // empty
}

VuRequestMessage::~VuRequestMessage()
{
    // empty
}

//--------------------------------------------------
VuGetRequest::VuGetRequest(VU_SPECIAL_GET_TYPE sgt,
                           VuSessionEntity*    sess)
    : VuRequestMessage(VU_GET_REQUEST_MESSAGE, vuNullId,
                       (sess ? sess
                        : ((sgt == VU_GET_GLOBAL_ENTS) ? (VuTargetEntity*)vuGlobalGroup
                           : (VuTargetEntity*)vuLocalSessionEntity->Game())))
{
}

VuGetRequest::VuGetRequest(VU_ID           entityId,
                           VuTargetEntity* target)
    : VuRequestMessage(VU_GET_REQUEST_MESSAGE, entityId, target)
{
    // empty
}

VuGetRequest::VuGetRequest(VU_ID     senderid,
                           VU_ID     target)
    : VuRequestMessage(VU_GET_REQUEST_MESSAGE, senderid, target)
{
    // empty
}

VuGetRequest::~VuGetRequest()
{
    // empty
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VU_ERRCODE VuGetRequest::Process(VU_BOOL autod)
{
    VuTargetEntity* sender = (VuTargetEntity *)vuDatabase->Find(Sender());

    //sfr: took sender out of if and return noop here
    if (IsLocal() or (sender == NULL))
    {
        return VU_NO_OP;
    }

    // sender is a target entity
    if (sender->IsTarget())
    {
        VuMessage *resp = 0;

        if (autod)
        {
            resp = new VuErrorMessage(VU_NOT_AVAILABLE_ERROR, Sender(), EntityId(), sender);
        }
        // get ALL ents
        else if (entityId_ == vuNullId)
        {
            if ((tgtid_ == vuGlobalGroup->Id()) or (tgtid_ == vuLocalSession))
            {
                // get all _global_ ents
                VuDatabaseIterator iter;
                VuEntity* ent = iter.GetFirst();

                while (ent)
                {
                    if ( not ent->IsPrivate() and ent->IsGlobal())
                    {
                        if (ent->Id() not_eq sender->Id())
                        {
                            if (ent->IsLocal())
                            {
                                resp = new VuFullUpdateEvent(ent, sender);
                                resp->RequestOutOfBandTransmit();
                                resp->RequestReliableTransmit();
                                VuMessageQueue::PostVuMessage(resp);
                                //resp->Send();
                            }
                            else
                            {
                                resp = new VuBroadcastGlobalEvent(ent, sender);
                                resp->RequestReliableTransmit();
                                resp->RequestOutOfBandTransmit();
                                VuMessageQueue::PostVuMessage(resp);
                                //resp->Send();
                            }
                        }
                    }

                    ent = iter.GetNext();
                }

                return VU_SUCCESS;
            }
            else if (tgtid_ == vuLocalSessionEntity->GameId())
            {
                // get all _game_ ents
                VuDatabaseIterator iter;
                VuEntity* ent = iter.GetFirst();

                while (ent)
                {
                    if ( not ent->IsPrivate() and ent->IsLocal() and not ent->IsGlobal())
                    {
                        if (ent->Id() not_eq sender->Id())
                        {
                            resp = new VuFullUpdateEvent(ent, sender);
                            resp->RequestReliableTransmit();
                            VuMessageQueue::PostVuMessage(resp);
                        }
                    }

                    ent = iter.GetNext();
                }

                return VU_SUCCESS;
            }
        }
        else if (Entity() and (Entity()->OwnerId() == vuLocalSession))
        {
            resp = new VuFullUpdateEvent(Entity(), sender);
        }
        else if (Destination() == vuLocalSession)
        {
            // we were asked specifically, so send the error response
            resp = new VuErrorMessage(VU_NO_SUCH_ENTITY_ERROR, Sender(), EntityId(), sender);
        }

        //send response
        if (resp)
        {
            resp->RequestReliableTransmit();
            VuMessageQueue::PostVuMessage(resp);
            return VU_SUCCESS;
        }
    }
    // how can we get here?
    // does it mean sender == NULL or sender is not a targetEntity?
    else
    {
        if (entityId_ == vuNullId)
        {
            // get all _global_ ents
            VuDatabaseIterator iter;
            VuEntity* ent = iter.GetFirst();
            VuMessage *resp = 0;

            while (ent)
            {
                if ( not ent->IsPrivate() and ent->IsGlobal())
                {
                    if ((ent->Id() not_eq sender->Id()))
                    {
                        if (ent->IsLocal())
                        {
                            resp = new VuFullUpdateEvent(ent, sender);
                            resp->RequestOutOfBandTransmit();
                            VuMessageQueue::PostVuMessage(resp);
                        }
                        else
                        {
                            resp = new VuBroadcastGlobalEvent(ent, sender);
                            resp->RequestOutOfBandTransmit();
                            VuMessageQueue::PostVuMessage(resp);
                        }
                    }
                }

                ent = iter.GetNext();
            }

            return VU_SUCCESS;
        }
        else if ((Entity()) and (Entity()->IsLocal()))
        {
            VuMessage *resp = 0;
            resp = new VuFullUpdateEvent(Entity(), sender);
            VuMessageQueue::PostVuMessage(resp);
            return VU_SUCCESS;
        }
    }

    return VU_NO_OP;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuPushRequest::VuPushRequest(VU_ID           entityId,
                             VuTargetEntity* target)
    : VuRequestMessage(VU_PUSH_REQUEST_MESSAGE, entityId, target)
{
    // empty
}

VuPushRequest::VuPushRequest(VU_ID     senderid,
                             VU_ID     target)
    : VuRequestMessage(VU_PUSH_REQUEST_MESSAGE, senderid, target)
{
    // empty
}

VuPushRequest::~VuPushRequest()
{
    // empty
}

VU_ERRCODE
VuPushRequest::Process(VU_BOOL)
{
    int retval = VU_NO_OP;

    if ( not IsLocal() and Destination() == vuLocalSession)
    {
        if (Entity())
        {
            retval = Entity()->Handle(this);
        }
        else
        {
            VuTargetEntity* sender = (VuTargetEntity*)vuDatabase->Find(Sender());

            if (sender and sender->IsTarget())
            {
                VuMessage* resp = new VuErrorMessage(VU_NO_SUCH_ENTITY_ERROR, Sender(),
                                                     EntityId(), sender);
                resp->RequestReliableTransmit();
                VuMessageQueue::PostVuMessage(resp);
                retval = VU_SUCCESS;
            }
        }
    }

    return retval;
}

//--------------------------------------------------
VuPullRequest::VuPullRequest(VU_ID           entityId,
                             VuTargetEntity* target)
    : VuRequestMessage(VU_PULL_REQUEST_MESSAGE, entityId, target)
{
    // empty
}

VuPullRequest::VuPullRequest(VU_ID     senderid,
                             VU_ID     target)
    : VuRequestMessage(VU_PUSH_REQUEST_MESSAGE, senderid, target)
{
    // empty
}

VuPullRequest::~VuPullRequest()
{
    // empty
}

VU_ERRCODE
VuPullRequest::Process(VU_BOOL)
{
    int retval = VU_NO_OP;

    if ( not IsLocal() and Destination() == vuLocalSession)
    {
        if (Entity())
        {
            retval = Entity()->Handle(this);
        }
        else
        {
            VuTargetEntity* sender = (VuTargetEntity*)vuDatabase->Find(Sender());

            if (sender and sender->IsTarget())
            {
                VuMessage* resp = new VuErrorMessage(VU_NO_SUCH_ENTITY_ERROR, Sender(),
                                                     EntityId(), sender);
                resp->RequestReliableTransmit();
                VuMessageQueue::PostVuMessage(resp);
                retval = VU_SUCCESS;
            }
        }
    }

    return retval;
}

//--------------------------------------------------

VuEvent::VuEvent(VU_MSG_TYPE type, VU_ID entityId, VuTargetEntity* target, VU_BOOL loopback)
    : VuMessage(type, entityId, target, loopback), updateTime_(vuxGameTime)
{
}

VuEvent::VuEvent(VU_MSG_TYPE type, VU_ID senderid, VU_ID target)
    : VuMessage(type, senderid, target), updateTime_(vuxGameTime)
{
}

VuEvent::~VuEvent()
{
}

int VuEvent::Activate(VuEntity* ent)
{
    SetEntity(ent);

    if (IsLocal() and ent)
    {
        updateTime_ = ent->LastUpdateTime();
    }

    return VU_SUCCESS;
}


#define VUEVENT_LOCALSIZE (sizeof(updateTime_))

int VuEvent::LocalSize() const
{
    return (VUEVENT_LOCALSIZE);
}

#define VUEVENT_SIZE (VUMESSAGE_SIZE+VUEVENT_LOCALSIZE)

int VuEvent::Size() const
{
    return (VUEVENT_SIZE);
}

int VuEvent::Decode(VU_BYTE** buf, long *rem)
{
    VuMessage::Decode(buf, rem);
    memcpychk(&updateTime_, buf, sizeof(updateTime_), rem);
    return (VUEVENT_SIZE);
}

int VuEvent::Encode(VU_BYTE** buf)
{
    VuMessage::Encode(buf);

    memcpy(*buf, &updateTime_, sizeof(updateTime_));
    *buf += sizeof(updateTime_);

    return (VUEVENT_SIZE);
}

//--------------------------------------------------
VuCreateEvent::VuCreateEvent(
    VuEntity*       entity,
    VuTargetEntity* target,
    VU_BOOL         loopback
)
    : VuEvent(VU_CREATE_EVENT, entity->Id(), target, loopback),
      expandedData_(0), vutype_(entity->Type()), size_(0), data_(0)
{
#if defined(VU_USE_CLASS_INFO)
    memcpy(classInfo_, entity->EntityType()->classInfo_, CLASS_NUM_BYTES);
#endif
    /* vutype_ = entity->Type();
     size_   = 0;
     data_   = 0;
     expandedData_ = 0;*/
}

//sfr: converts added address
VuCreateEvent::VuCreateEvent(
    VU_ADDRESS senderAddress, VU_ID     senderid,
    VU_ID     target
)
    : VuEvent(VU_CREATE_EVENT, senderid, target),
      expandedData_(0), vutype_(0), size_(0), data_(0), senderAddress(senderAddress)
{
#if defined(VU_USE_CLASS_INFO)
    memset(classInfo_, '\0', CLASS_NUM_BYTES);
#endif
    /*
    size_   = 0;
    data_   = 0;
    vutype_ = 0;
    expandedData_ = 0;
    this->senderAddress = senderAddress;*/
}

VuCreateEvent::VuCreateEvent(
    VU_MSG_TYPE     type,
    VuEntity*       ent,
    VuTargetEntity* target,
    VU_BOOL         loopback
)
    : VuEvent(type, ent->Id(), target, loopback),
      expandedData_(0), vutype_(ent->Type()), size_(0), data_(0)
{
    SetEntity(ent);
#if defined(VU_USE_CLASS_INFO)
    memcpy(classInfo_, ent->EntityType()->classInfo_, CLASS_NUM_BYTES);
#endif
    /*
    vutype_ = ent->Type();
    size_ = 0;
    data_ = 0;
    expandedData_ = 0;*/
}

VuCreateEvent::VuCreateEvent(
    VU_MSG_TYPE type,
    VU_ADDRESS senderAddress, // sfr: added address
    VU_ID       senderid,
    VU_ID       target
)
    : VuEvent(type, senderid, target),
      expandedData_(0), vutype_(0), size_(0), data_(0), senderAddress(senderAddress)
{
#if defined(VU_USE_CLASS_INFO)
    memset(classInfo_, '\0', CLASS_NUM_BYTES);
#endif
    /*
    size_   = 0;
    data_   = 0;
    vutype_ = 0;
    expandedData_ = 0;

    this->senderAddress = senderAddress;*/
}

VuCreateEvent::~VuCreateEvent()
{
    delete [] data_;
    // sfr: no more antidb
    /*
    if (
     Entity() and // we have an ent ...
     Entity()->VuState() == VU_MEM_ACTIVE and // bitand have not yet removed it...
     Entity()->Id().creator_ == vuLocalSession.creator_ and // bitand it has our session tag...
     vuDatabase->Find(Entity()->Id()) == 0                 // bitand it's not in the DB...
    ) {
     vuAntiDB->Insert(Entity()); // ==> put it in the anti DB
    }
    */
    // sfr: smartpointer
    //VuDeReferenceEntity(expandedData_);
}

int VuCreateEvent::LocalSize() const
{
    ushort size = size_;

    if (Entity())
    {
        size = static_cast<ushort>(Entity()->SaveSize());
    }

    return
#if defined(VU_USE_CLASS_INFO)
        CLASS_NUM_BYTES +
#endif
        sizeof(vutype_) +
        sizeof(size_) +
        size
        ;
}

int VuCreateEvent::Size() const
{
    return VUEVENT_SIZE + VuCreateEvent::LocalSize();
}

int VuCreateEvent::Decode(VU_BYTE** buf, long *rem)
{

    ushort oldsize = size_;
    int retval = VuEvent::Decode(buf, rem);
#if defined(VU_USE_CLASS_INFO)
    memcpychk(classInfo_, buf, CLASS_NUM_BYTES, rem);
#endif
    memcpychk(&vutype_,   buf, sizeof(vutype_), rem);
    memcpychk(&size_,     buf, sizeof(size_), rem);

    if ( not data_ or oldsize not_eq size_)
    {
        delete [] data_;
        data_ = new VU_BYTE[size_];
    }

    memcpychk(data_, buf, size_, rem);

    // note: this MUST be called after capturing size_ (above)
    return (retval + VuCreateEvent::LocalSize());
}

VU_BOOL VuCreateEvent::DoSend()
{
    if (Entity() and Entity()->VuState() == VU_MEM_ACTIVE)
    {
        return TRUE;
    }

    return FALSE;
}

int VuCreateEvent::Encode(VU_BYTE** buf)
{
    ushort oldsize = size_;

    if (Entity())
    {
        size_ = static_cast<ushort>(Entity()->SaveSize());
    }

    if (Entity() and size_)
    {
        // copying ent in save allows multiple send's of same event
        if (size_ > oldsize)
        {
            if (data_)
            {
                delete [] data_;
            }

            data_ = new VU_BYTE[size_];
        }

        VU_BYTE *ptr = data_;
        Entity()->Save(&ptr);
    }

    int retval = VuEvent::Encode(buf);

#if defined(VU_USE_CLASS_INFO)
    memcpy(*buf, classInfo_, CLASS_NUM_BYTES);
    *buf += CLASS_NUM_BYTES;
#endif
    memcpy(*buf, &vutype_,  sizeof(vutype_));
    *buf += sizeof(vutype_);
    memcpy(*buf, &size_,    sizeof(size_));
    *buf += sizeof(size_);
    memcpy(*buf, data_,     size_);
    *buf += size_;
    retval += VuCreateEvent::LocalSize();

    return retval;
}

VU_ERRCODE VuCreateEvent::Activate(VuEntity* ent)
{
    return VuEvent::Activate(ent);
}

VU_ERRCODE VuCreateEvent::Process(VU_BOOL)
{
    if (expandedData_)
    {
        return VU_NO_OP;    // already done...
    }

    if (vutype_ < VU_LAST_ENTITY_TYPE)
    {
        //create a vu entity
        expandedData_.reset(VuCreateEntity(vutype_, data_, size_));
    }
    else
    {
        //create a vu external entity
        expandedData_.reset(VuxCreateEntity(vutype_, size_, data_));
    }

    if (expandedData_.get() == NULL)
    {
        return VU_ERROR;
    }

    if ( not expandedData_->IsLocal())
    {
        expandedData_->SetTransmissionTime(postTime_);
    }

    if (
        Entity() and 
        (Entity()->OwnerId() not_eq expandedData_->OwnerId()) and 
        Entity() not_eq expandedData_
    )
    {
        if (Entity()->IsPrivate())
        {
            //Entity()->ChangeId(expandedData_.get());
            vuDatabase->Remove(Entity());
            Entity()->SetId(VuxGetId()); // will get a new one upon insertion
            vuDatabase->Insert(Entity());
            SetEntity(0);
        }
        else
        {
            VuEntity* winner = ResolveWinner(Entity(), expandedData_.get());

            if (winner == Entity())
            {
                // this will prevent a db insert of expandedData
                SetEntity(expandedData_.get());
            }
            else if (winner == expandedData_)
            {
                Entity()->SetOwnerId(expandedData_->OwnerId());

                if (Entity()->Type() == expandedData_->Type())
                {
                    // if we have the same type, then just transfer to winner
                    VuTargetEntity *dest = 0;

                    if (Entity()->IsGlobal())
                    {
                        dest = vuGlobalGroup;
                    }
                    else
                    {
                        dest = vuLocalSessionEntity->Game();
                    }

                    VuTransferEvent *event = new VuTransferEvent(Entity(), dest);
                    event->Ref();
                    VuMessageQueue::PostVuMessage(event);
                    Entity()->Handle(event);
                    vuDatabase->Handle(event);
                    event->UnRef();
                    SetEntity(expandedData_.get());
                }
                else
                {
                    type_ = VU_CREATE_EVENT;

                    if (Entity()->VuState() == VU_MEM_ACTIVE)
                    {
                        // note: this will cause a memory leak (but is extrememly rare)
                        //   Basically, we have two ents with the same id, and we cannot
                        //   keep track of both, even to know when it is safe to delete
                        //   the abandoned entity -- so we remove it from VU, but don't
                        //   call its destructor... the last thing we do with it is call
                        //   VuxRetireEntity, leaving ultimate cleanup up to the app
                        // sfr: do we ever get here?
                        VuReferenceEntity(Entity());
                        vuDatabase->Remove(Entity());
                        // sfr: no more antidb
                        //vuAntiDB->Remove(Entity());
                    }

                    VuxRetireEntity(Entity());
                    SetEntity(0);
                }
            }
        }
    }

    if (Entity() and (type_ == VU_FULL_UPDATE_EVENT))
    {
        Entity()->Handle((VuFullUpdateEvent *)this);
        return VU_SUCCESS;
    }
    else if ( not Entity())
    {
        // received a session entity
        if (expandedData_->IsSession())
        {
            // this is address we receive. Can be a full valid address or a partial one
            VuSessionEntity *expandedSession = static_cast<VuSessionEntity*>(expandedData_.get());
            VU_ADDRESS entAdd = expandedSession->GetAddress();
            //infer IP
            entAdd.ip = senderAddress.ip;
            // adds entity to address table using the receive ports he told us
            // and IP and send inferred
            //VU_ADDRESSTranslation.insert(expandedData_->Id().creator_);
            //VU_ADDRESSTranslation.setIP(entAdd.ip, expandedData_->Id().creator_);
            //VU_ADDRESSTranslation.setRecvPort(entAdd.recvPort, expandedData_->Id().creator_);
            //VU_ADDRESSTranslation.setReliableRecvPort(entAdd.reliableRecvPort, expandedData_->Id().creator_);
            //VU_ADDRESSTranslation.setSendPort(entAdd.sendPort, expandedData_->Id().creator_);

            // set entity address correctly now
            expandedSession->SetAddress(entAdd);

            // sfr: globalgroup has no owner, and this is first client
            // set its owner
            if (vuGlobalGroup->OwnerId().creator_.value_ == 0)
            {
                vuGlobalGroup->SetOwnerId(expandedData_->OwnerId());
            }
        }

        SetEntity(expandedData_.get());
        // OW: me123 MP Fix
        vuDatabase->/*Silent*/Insert(Entity());  //me123 to silent otherwise this will

        return VU_SUCCESS;
    }

    return VU_NO_OP;
}

//--------------------------------------------------
VuManageEvent::VuManageEvent(VuEntity*       entity,
                             VuTargetEntity* target,
                             VU_BOOL         loopback)
    : VuCreateEvent(VU_MANAGE_EVENT, entity, target, loopback)
{
    // empty
}

VuManageEvent::VuManageEvent(VU_ID     senderid,
                             VU_ID     target)
    : VuCreateEvent(VU_MANAGE_EVENT, senderid, target)
{
    // empty
}

VuManageEvent::~VuManageEvent()
{
    // empty
}

//--------------------------------------------------
VuDeleteEvent::VuDeleteEvent(VuEntity* entity)
    : VuEvent(
        VU_DELETE_EVENT,
        entity->Id(),
        entity->IsGlobal() ?
        static_cast<VuTargetEntity*>(vuGlobalGroup) :
        static_cast<VuTargetEntity*>(vuLocalSessionEntity->Game())
        ,
#if NO_RELEASE_EVENT
        FALSE
#else
        TRUE
#endif
    )
{
    SetEntity(entity);
}

VuDeleteEvent::VuDeleteEvent(VU_ID senderid, VU_ID target)
    : VuEvent(VU_DELETE_EVENT, senderid, target)
{
}

VuDeleteEvent::~VuDeleteEvent()
{
}

VU_ERRCODE VuDeleteEvent::Activate(VuEntity* ent)
{
    if (ent)
    {
        SetEntity(ent);
    }

    if (ent)
    {
        if (ent->VuState() == VU_MEM_ACTIVE)
        {
#if NO_RELEASE_EVENT
            vuDatabase->Remove(ent);
#else
            vuDatabase->DeleteRemove(ent);
#endif
            return VU_SUCCESS;
        }
        else if ( not Entity()->IsLocal())
        {
            // prevent duplicate delete event from remote source
            SetEntity(0);
        }
    }

    return VU_NO_OP;
}

VU_ERRCODE VuDeleteEvent::Process(VU_BOOL)
{
    if (Entity())
    {
        Entity()->Handle(this);
        return VU_SUCCESS;
    }

    return VU_NO_OP;
}

//--------------------------------------------------
#if 0
VuUnmanageEvent::VuUnmanageEvent(
    VuEntity*       entity,
    VuTargetEntity* target,
    VU_TIME         mark,
    VU_BOOL         loopback
)
    : VuEvent(VU_UNMANAGE_EVENT, entity->Id(), target, loopback), mark_(mark)
{
    SetEntity(entity);
    // set kill fuse
    VuReleaseEvent* release = new VuReleaseEvent(entity);
    VuTimerEvent*   timer   = new VuTimerEvent(0, mark, VU_DELETE_TIMER, release);
    VuMessageQueue::PostVuMessage(timer);
}

VuUnmanageEvent::VuUnmanageEvent(VU_ID senderid, VU_ID target)
    : VuEvent(VU_UNMANAGE_EVENT, senderid, target), mark_(0)
{
}

VuUnmanageEvent::~VuUnmanageEvent()
{
}

#define VUUNMANAGEEVENT_LOCALSIZE (sizeof(mark_))
int VuUnmanageEvent::LocalSize() const
{
    return(VUUNMANAGEEVENT_LOCALSIZE);
}

#define VUUNMANAGEEVENT_SIZE (VUEVENT_SIZE+VUUNMANAGEEVENT_LOCALSIZE)
int VuUnmanageEvent::Size() const
{
    return (VUUNMANAGEEVENT_SIZE);
}

int VuUnmanageEvent::Decode(VU_BYTE** buf, long *rem)
{
    VuEvent::Decode(buf, rem);
    memcpychk(&mark_, buf, sizeof(mark_), rem);

    return (VUUNMANAGEEVENT_SIZE);
}

int VuUnmanageEvent::Encode(VU_BYTE** buf)
{
    VuEvent::Encode(buf);

    memcpy(*buf, &mark_, sizeof(mark_));
    *buf += sizeof(mark_);

    return (VUUNMANAGEEVENT_SIZE);
}

VU_ERRCODE VuUnmanageEvent::Process(VU_BOOL)
{
    if (Entity())
    {
        Entity()->Handle(this);
        return VU_SUCCESS;
    }

    return VU_NO_OP;
}
#endif

//--------------------------------------------------
#if not NO_RELEASE_EVENT
VuReleaseEvent::VuReleaseEvent(VuEntity* entity)
    : VuEvent(VU_RELEASE_EVENT, entity->Id(), vuLocalSessionEntity.get(), TRUE)
{
    SetEntity(entity);
}

VuReleaseEvent::~VuReleaseEvent()
{
    // sfr: smartpointer
    //VuDeReferenceEntity(Entity());
}

VU_BOOL VuReleaseEvent::DoSend()
{
    return FALSE;
}

int VuReleaseEvent::Size() const
{
    return 0;
}

int VuReleaseEvent::Decode(VU_BYTE **, long *rem)
{
    // not a net event, so just return
    return 0;
}

int VuReleaseEvent::Encode(VU_BYTE **)
{
    // not a net event, so just return
    return 0;
}

VU_ERRCODE VuReleaseEvent::Activate(VuEntity* ent)
{
    if (ent)
    {
        SetEntity(ent);
    }

    if (Entity() and Entity()->VuState() == VU_MEM_ACTIVE)
    {
        Entity()->share_.ownerId_ = vuNullId;
        vuDatabase->DeleteRemove(Entity());
        return VU_SUCCESS;
    }

    return VU_NO_OP;
}

VU_ERRCODE VuReleaseEvent::Process(VU_BOOL)
{
    if (Entity())
    {
        Entity()->Handle(this);
        return VU_SUCCESS;
    }

    return VU_NO_OP;
}
#endif

//--------------------------------------------------

VuTransferEvent::VuTransferEvent(VuEntity*       entity,
                                 VuTargetEntity* target,
                                 VU_BOOL         loopback)
    : VuEvent(VU_TRANSFER_EVENT, entity->Id(), target, loopback),
      newOwnerId_(entity->OwnerId())
{
    SetEntity(entity);
}

VuTransferEvent::VuTransferEvent(VU_ID senderid, VU_ID target)
    : VuEvent(VU_TRANSFER_EVENT, senderid, target),
      newOwnerId_(vuNullId)
{
    // empty
}

VuTransferEvent::~VuTransferEvent()
{
    // empty
}

#define VUTRANSFEREVENT_LOCALSIZE (sizeof(newOwnerId_))
int VuTransferEvent::LocalSize() const
{
    return(VUTRANSFEREVENT_LOCALSIZE);
}

#define VUTRANSFEREVENT_SIZE (VUEVENT_SIZE+VUTRANSFEREVENT_LOCALSIZE)
int VuTransferEvent::Size() const
{
    return(VUTRANSFEREVENT_SIZE);
}

int VuTransferEvent::Decode(VU_BYTE** buf, long *rem)
{
    VuEvent::Decode(buf, rem);
    memcpychk(&newOwnerId_, buf, sizeof(newOwnerId_), rem);

    return(VUTRANSFEREVENT_SIZE);
}

int
VuTransferEvent::Encode(VU_BYTE** buf)
{
    VuEvent::Encode(buf);

    memcpy(*buf, &newOwnerId_, sizeof(newOwnerId_));
    *buf += sizeof(newOwnerId_);

    return(VUTRANSFEREVENT_SIZE);
}

VU_ERRCODE
VuTransferEvent::Activate(VuEntity* ent)
{
    return VuEvent::Activate(ent);
}

VU_ERRCODE
VuTransferEvent::Process(VU_BOOL)
{
    if (Entity())
    {
        Entity()->Handle(this);
        return VU_SUCCESS;
    }

    return VU_NO_OP;
}

//--------------------------------------------------
// this function is the senders version
VuPositionUpdateEvent::VuPositionUpdateEvent(
    VuEntity*       entity,
    VuTargetEntity* target,
    VU_BOOL         loopback)
    : VuEvent(VU_POSITION_UPDATE_EVENT, entity->Id(), target, loopback)
{
    if (entity)
    {
        SetEntity(entity);
        updateTime_ = entity->LastUpdateTime();

        x_  = entity->XPos();
        y_  = entity->YPos();
        z_  = entity->ZPos();

        dx_ = entity->XDelta();
        dy_ = entity->YDelta();
        dz_ = entity->ZDelta();
#define CHK_MAXSPEED(d)\
 do { if (d > MAX_SPEED){ d = MAX_SPEED; } else if (d < -MAX_SPEED){ d = -MAX_SPEED; } } while(0)
        CHK_MAXSPEED(dx_);
        CHK_MAXSPEED(dy_);
        CHK_MAXSPEED(dz_);
#undef CHKSPEED
        // turn from radians to VU_BYTE
        yaw_    = entity->Yaw();
        pitch_  = entity->Pitch();
        roll_   = entity->Roll();

        //dyaw_   = entity->YawDelta();
        //dpitch_ = entity->PitchDelta();
        //droll_  = entity->RollDelta();
        //VU_PRINT("yaw=%3.3f, pitch=%3.3f, roll=%3.3f, dyaw=%3.3f, dpitch=%3.3f, droll=%3.3f\n", yaw_, pitch_, roll_, dyaw_, dpitch_, droll_);
    }
}

VuPositionUpdateEvent::VuPositionUpdateEvent(VU_ID     senderid,
        VU_ID     target)
    : VuEvent(VU_POSITION_UPDATE_EVENT, senderid, target)
{
    // empty
}

VuPositionUpdateEvent::~VuPositionUpdateEvent()
{
    // empty
}

VU_BOOL VuPositionUpdateEvent::DoSend()
{
    // test is done in vudriver.cpp, prior to generation of event
    return TRUE;
}

int VuPositionUpdateEvent::LocalSize() const
{
    return (
               sizeof(yc_) + sizeof(pc_) + sizeof(rc_) +
               sizeof(x_) + sizeof(y_) + sizeof(z_) +
               sizeof(sdx_) + sizeof(sdy_) + sizeof(sdz_)
           );
}

int VuPositionUpdateEvent::Size() const
{
    return (LocalSize() + VUEVENT_SIZE);
}

int VuPositionUpdateEvent::Decode(VU_BYTE** buf, long *rem)
{
    VuEvent::Decode(buf, rem);

    memcpychk(&x_,  buf, sizeof(x_), rem);
    memcpychk(&y_,  buf, sizeof(y_), rem);
    memcpychk(&z_,  buf, sizeof(z_), rem);
    // receive from wire, convert to dot
    memcpychk(&sdx_, buf, sizeof(sdx_), rem);
    memcpychk(&sdy_, buf, sizeof(sdy_), rem);
    memcpychk(&sdz_, buf, sizeof(sdz_), rem);
    dx_ = sdx_ * SHORT2D;
    dy_ = sdy_ * SHORT2D;
    dz_ = sdz_ * SHORT2D;
    // receive from wire and convert to radians
    memcpychk(&yc_,  buf, sizeof(yc_), rem);
    memcpychk(&pc_,  buf, sizeof(pc_), rem);
    memcpychk(&rc_,  buf, sizeof(rc_), rem);
    yaw_   = yc_ * CHAR2RAD;
    pitch_ = pc_ * CHAR2RAD;
    roll_  = rc_ * CHAR2RAD;

    return LocalSize();
}


int VuPositionUpdateEvent::Encode(VU_BYTE** buf)
{
    VuEvent::Encode(buf);

    memcpy(*buf, &x_,  sizeof(x_));
    *buf += sizeof(x_);
    memcpy(*buf, &y_,  sizeof(y_));
    *buf += sizeof(y_);
    memcpy(*buf, &z_,  sizeof(z_));
    *buf += sizeof(z_);
    // convert to short and send through the wire
    sdx_ = static_cast<short>(dx_ * D2SHORT);
    sdy_ = static_cast<short>(dy_ * D2SHORT);
    sdz_ = static_cast<short>(dz_ * D2SHORT);
    memcpy(*buf, &sdx_, sizeof(sdx_));
    *buf += sizeof(sdx_);
    memcpy(*buf, &sdy_, sizeof(sdy_));
    *buf += sizeof(sdy_);
    memcpy(*buf, &sdz_, sizeof(sdz_));
    *buf += sizeof(sdz_);
    // convert from radians to char and send through the wire
    yc_  = static_cast<char>(yaw_   * RAD2CHAR);
    pc_  = static_cast<char>(pitch_ * RAD2CHAR);
    rc_  = static_cast<char>(roll_  * RAD2CHAR);
    memcpy(*buf, &yc_, sizeof(yc_));
    *buf += sizeof(yc_);
    memcpy(*buf, &pc_, sizeof(pc_));
    *buf += sizeof(pc_);
    memcpy(*buf, &rc_, sizeof(rc_));
    *buf += sizeof(rc_);
    return (LocalSize());
}

VU_ERRCODE VuPositionUpdateEvent::Process(VU_BOOL)
{
    if (Entity())
    {
        Entity()->Handle(this);
        return VU_SUCCESS;
    }

    return VU_NO_OP;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuBroadcastGlobalEvent::VuBroadcastGlobalEvent(VuEntity *entity, VuTargetEntity* target, VU_BOOL loopback) :
    VuEvent(VU_BROADCAST_GLOBAL_EVENT, entity->Id(), target, loopback)
{

#if defined(VU_USE_CLASS_INFO)
    memcpy(classInfo_, entity->EntityType()->classInfo_, CLASS_NUM_BYTES);
#endif

    vutype_ = entity->Type();
    SetEntity(entity);
    updateTime_ = entity->LastUpdateTime();

    // sft address handling
    if (entity->IsSession())
    {
        // sending myself
        if ((entity->Id().creator_ == vuLocalSession.creator_))
        {
            // does not include IP, will be discovered on the other side
            entityAddress = VU_ADDRESS(0, com_API_get_my_receive_port(), com_API_get_my_reliable_receive_port());
        }
        //send another entity
        else
        {
            entityAddress = static_cast<VuSessionEntity*>(entity)->GetAddress();
        }
    }
}

VuBroadcastGlobalEvent::VuBroadcastGlobalEvent(VU_ADDRESS senderAddress, VU_ID senderId, VU_ID target) :
    VuEvent(VU_BROADCAST_GLOBAL_EVENT, senderId, target)
{
#if defined(VU_USE_CLASS_INFO)
    memset(classInfo_, '\0', CLASS_NUM_BYTES);
#endif
    vutype_ = 0;
    this->senderAddress = senderAddress;
}


VuBroadcastGlobalEvent::~VuBroadcastGlobalEvent()
{
}

int VuBroadcastGlobalEvent::Size() const
{
    return VUEVENT_SIZE + entityAddress.Size();
}

int VuBroadcastGlobalEvent::Decode(VU_BYTE** buf, long *rem)
{
    long initRem = *rem;
    VuEvent::Decode(buf, rem);
    entityAddress.Decode(buf, rem);
    return (int)(initRem - *rem);
}

int VuBroadcastGlobalEvent::Encode(VU_BYTE** buf)
{
    int retval = VuEvent::Encode(buf);
    retval += entityAddress.Encode(buf);

    return retval;
}

VU_ERRCODE VuBroadcastGlobalEvent::Activate(VuEntity* ent)
{
    return VuEvent::Activate(ent);
}

VU_ERRCODE VuBroadcastGlobalEvent::Process(VU_BOOL autod)
{
    if (autod)
    {
        return 0;
    }

    // NULL entity means we dont have it in DB yet.
    if (Entity() == NULL)
    {
        //sfr: someone sending himself
        if (entityAddress.ip == 0)
        {
            // infer ip
            entityAddress.ip = senderAddress.ip;
        }
        // someone sending others
        else
        {
            // server is sending an IP
            // if IP is private, entity is on the same network as server
            // if we see server as a private IP,
            // we are on the same network as server,
            // so no translation is necessary
            // otherwise, client is on the same private network as server
            // and we use server IP
            // find server address
            // TODO  check if we can get vuGlobalGroup owner...
            VuSessionEntity *server = static_cast<VuSessionEntity*>(vuDatabase->Find(vuGlobalGroup->OwnerId()));

            if (server not_eq NULL)
            {
                VU_ADDRESS serverAdd = server->GetAddress();

                if ((entityAddress.IsPrivate()) and ( not serverAdd.IsPrivate()))
                {
                    entityAddress.ip = serverAdd.ip;
                }
            }
        }

        // open a dangling session so we can send messages to the entity
        VuxAddDanglingSession(EntityId(), entityAddress);

        // someone broadcasted an event (not a session)
        if (EntityId().num_ not_eq VU_SESSION_ENTITY_ID)
        {
            // Send a get request for the entity if its not a session
            VuGetRequest *msg = new VuGetRequest(EntityId(), vuGlobalGroup);
            msg->RequestOutOfBandTransmit();
            VuMessageQueue::PostVuMessage(msg);
        }
        // someone broadcasted a session
        else
        {
            // broadcasting a session
            // Send full update unreliably
            VuMessage *msg;
            //vuLocalSessionEntity->SetDirty ();
            msg = new VuFullUpdateEvent(vuLocalSessionEntity.get(), vuGlobalGroup);
            msg->RequestOutOfBandTransmit();
            VuMessageQueue::PostVuMessage(msg);
            //msg->Send();

            // request dummy block from everyone to open NATS
            // cause this one is prolly comming in
            msg = new VuRequestDummyBlockMessage(entityAddress, vuGlobalGroup);
            msg->RequestReliableTransmit();
            //msg->Send();
            VuMessageQueue::PostVuMessage(msg);
        }
    }

    return 1;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuFullUpdateEvent::VuFullUpdateEvent(VuEntity*       entity,
                                     VuTargetEntity* target,
                                     VU_BOOL         loopback)
    : VuCreateEvent(VU_FULL_UPDATE_EVENT, entity, target, loopback)
{
    SetEntity(entity);
    updateTime_ = entity->LastUpdateTime();
}

//sfr: added address
VuFullUpdateEvent::VuFullUpdateEvent(VU_ADDRESS senderAdd, VU_ID     senderid,
                                     VU_ID     target)
    : VuCreateEvent(VU_FULL_UPDATE_EVENT, senderAdd, senderid, target)
{
    // empty
}

VuFullUpdateEvent::~VuFullUpdateEvent()
{
    // empty
}

VU_ERRCODE
VuFullUpdateEvent::Activate(VuEntity* ent)
{
    if ( not ent)
    {
        // morph this into a create event
        type_ = VU_CREATE_EVENT;
    }

    return VuEvent::Activate(ent);
}

//--------------------------------------------------
VuEntityCollisionEvent::VuEntityCollisionEvent(VuEntity*       entity,
        VU_ID           otherId,
        VU_DAMAGE       hitLocation,
        int             hitEffect,
        VuTargetEntity* target,
        VU_BOOL         loopback)
    : VuEvent(VU_ENTITY_COLLISION_EVENT, entity->Id(), target, loopback),
      otherId_(otherId),
      hitLocation_(hitLocation),
      hitEffect_(hitEffect)
{
    SetEntity(entity);
}

//sfr: vu change
VuEntityCollisionEvent::VuEntityCollisionEvent(VU_ID     senderid,
        VU_ID     target)
    : VuEvent(VU_ENTITY_COLLISION_EVENT, senderid, target),
      otherId_(0, 0)
{
    // empty
}

VuEntityCollisionEvent::~VuEntityCollisionEvent()
{
    // empty
}

#define VUENTITYCOLLISIONEVENT_LOCALSIZE (sizeof(otherId_)+sizeof(hitLocation_)+sizeof(hitEffect_))
int VuEntityCollisionEvent::LocalSize() const
{
    return (VUENTITYCOLLISIONEVENT_LOCALSIZE);
}

#define VUENTITYCOLLISIONEVENT_SIZE (VUEVENT_SIZE+VUENTITYCOLLISIONEVENT_LOCALSIZE)
int
VuEntityCollisionEvent::Size() const
{
    return (VUENTITYCOLLISIONEVENT_SIZE);
}

int VuEntityCollisionEvent::Decode(VU_BYTE** buf, long *rem)
{
    VuEvent::Decode(buf, rem);

    memcpychk(&otherId_,     buf, sizeof(otherId_), rem);
    memcpychk(&hitLocation_, buf, sizeof(hitLocation_), rem);
    memcpychk(&hitEffect_,   buf, sizeof(hitEffect_), rem);

    return (VUENTITYCOLLISIONEVENT_SIZE);
}

int  VuEntityCollisionEvent::Encode(VU_BYTE** buf)
{
    VuEvent::Encode(buf);

    memcpy(*buf, &otherId_,     sizeof(otherId_));
    *buf += sizeof(otherId_);
    memcpy(*buf, &hitLocation_, sizeof(hitLocation_));
    *buf += sizeof(hitLocation_);
    memcpy(*buf, &hitEffect_,   sizeof(hitEffect_));
    *buf += sizeof(hitEffect_);

    return (VUENTITYCOLLISIONEVENT_SIZE);
}

VU_ERRCODE VuEntityCollisionEvent::Process(VU_BOOL)
{
    if (Entity())
    {
        Entity()->Handle(this);
        return VU_SUCCESS;
    }

    return VU_NO_OP;
}

//--------------------------------------------------
VuGroundCollisionEvent::VuGroundCollisionEvent(VuEntity*       entity,
        VuTargetEntity* target,
        VU_BOOL         loopback)
    : VuEvent(VU_GROUND_COLLISION_EVENT, entity->Id(), target, loopback)
{
    // empty
}

VuGroundCollisionEvent::VuGroundCollisionEvent(VU_ID     senderid,
        VU_ID     target)
    : VuEvent(VU_GROUND_COLLISION_EVENT, senderid, target)
{
    // empty
}

VuGroundCollisionEvent::~VuGroundCollisionEvent()
{
    // empty
}

VU_ERRCODE
VuGroundCollisionEvent::Process(VU_BOOL)
{
    if (Entity())
    {
        Entity()->Handle(this);
        return VU_SUCCESS;
    }

    return VU_NO_OP;
}

//--------------------------------------------------
//sfr: vu change
VuSessionEvent::VuSessionEvent(VuEntity*       ent,
                               ushort          subtype,
                               VuTargetEntity* target,
                               VU_BOOL         loopback)
    : VuEvent(VU_SESSION_EVENT, ent->Id(), target, loopback),
      subtype_(subtype),
      group_(0, 0),
      callsign_(0),
      syncState_(VU_NO_SYNC),
      gameTime_(vuxGameTime)
{
    const char *name = "bad session";

    if (ent->IsSession())
    {
        name   = ((VuSessionEntity*)ent)->Callsign();
        group_ = ((VuSessionEntity*)ent)->GameId();

#if defined(VU_TRACK_LATENCY)
        syncState_ = ((VuSessionEntity*)ent)->TimeSyncState();
#endif

    }
    else if (ent->IsGroup())
    {
        name = ((VuGroupEntity*)ent)->GroupName();
    }

    int len   = strlen(name);
    callsign_ = new char[len + 1];
    strcpy(callsign_, name);

    SetEntity(ent);
    RequestReliableTransmit();
    RequestOutOfBandTransmit();
}

//sfr: vu change
VuSessionEvent::VuSessionEvent(VU_ID     senderid,
                               VU_ID     target)
    : VuEvent(VU_SESSION_EVENT, senderid, target),
      subtype_(VU_SESSION_UNKNOWN_SUBTYPE),
      group_(0, 0),
      callsign_(0),
      syncState_(VU_NO_SYNC),
      gameTime_(vuxGameTime)
{
    //empty
    RequestReliableTransmit();
}

VuSessionEvent::~VuSessionEvent()
{
    delete [] callsign_;
}

int VuSessionEvent::LocalSize() const
{
    return sizeof(sender_) +
           sizeof(entityId_) +
           sizeof(group_) +
           sizeof(subtype_) +
           sizeof(syncState_) +
           sizeof(gameTime_) +
           (callsign_ ? strlen(callsign_) + 1 : 1)
           ;
}

int VuSessionEvent::Size() const
{
    return VuSessionEvent::LocalSize();
}

int VuSessionEvent::Decode(VU_BYTE** buf, long *rem)
{
    VU_BYTE len = 0;

    //sfr: why this function doesnt call other Decodes?

    memcpychk(&sender_,   buf, sizeof(sender_), rem);
    memcpychk(&entityId_, buf, sizeof(entityId_), rem);
    memcpychk(&group_,    buf, sizeof(group_), rem);
    memcpychk(&subtype_,  buf, sizeof(subtype_), rem);
    memcpychk(&len,       buf, sizeof(VU_BYTE), rem);

    if (len)
    {
        delete callsign_;
        callsign_ = new char[len + 1];
        memcpychk(callsign_, buf, len, rem);
        callsign_[len] = '\0';
    }

    memcpychk(&syncState_, buf, sizeof(syncState_), rem);
    memcpychk(&gameTime_,  buf, sizeof(gameTime_), rem);

    return VuSessionEvent::LocalSize();
}

int VuSessionEvent::Encode(VU_BYTE** buf)
{
    VU_BYTE len;

    if (callsign_)
        len = static_cast<VU_BYTE>(strlen(callsign_));
    else
        len = 0;

    memcpy(*buf, &sender_,    sizeof(sender_));
    *buf += sizeof(sender_);
    memcpy(*buf, &entityId_,  sizeof(entityId_));
    *buf += sizeof(entityId_);
    memcpy(*buf, &group_,     sizeof(group_));
    *buf += sizeof(group_);
    memcpy(*buf, &subtype_,   sizeof(subtype_));
    *buf += sizeof(subtype_);
    memcpy(*buf, &len,        sizeof(VU_BYTE));
    *buf += sizeof(VU_BYTE);
    memcpy(*buf, callsign_,   len);
    *buf += len;
    memcpy(*buf, &syncState_, sizeof(syncState_));
    *buf += sizeof(syncState_);
    memcpy(*buf, &gameTime_,  sizeof(gameTime_));
    *buf += sizeof(gameTime_);

    int retval = VuSessionEvent::LocalSize();

    return retval;
}

VU_ERRCODE VuSessionEvent::Process(VU_BOOL)
{
    VuEntity *e = Entity();

    if (e and e->VuState() == VU_MEM_ACTIVE)
    {
        Entity()->Handle(this);
        return VU_SUCCESS;
    }

    return VU_NO_OP;
}

//--------------------------------------------------
VuTimerEvent::VuTimerEvent(
    VuEntity *entity, VU_TIME mark, ushort timertype, VuMessage* event
)
    : VuEvent(
        VU_TIMER_EVENT, (entity ? entity->Id() : vuNullId), vuLocalSessionEntity.get(), TRUE),
    mark_(mark), timertype_(timertype), event_(event), next_(0)
{
    SetEntity(entity);

    if (event_)
    {
        event_->Ref();
    }
}

VuTimerEvent::~VuTimerEvent()
{
    // sfr: FUCK wheres unref?
    if (event_)
    {
        event_->UnRef();
    }
}

VU_BOOL VuTimerEvent::DoSend()
{
    return FALSE;
}

int VuTimerEvent::Size() const
{
    return 0;
}

int VuTimerEvent::Decode(VU_BYTE**, long *)
{
    // not a net event, so just return
    return 0;
}

int VuTimerEvent::Encode(VU_BYTE**)
{
    // not a net event, so just return
    return 0;
}

VU_ERRCODE VuTimerEvent::Process(VU_BOOL)
{
    int retval = VU_NO_OP;

    if (event_)
    {
        // MD -- 20050201: messages that come here from VmMessageQueue::PostVuMessage() really
        // ought to go back there since there is some real processing done there (like the Activate() in
        // particular...if you bypass that Activate() you end up sending a create message to the client
        // systems that has a zero length stream...happens a lot with chaff, flares and ejected pilots
        // it seems -- manifests as all clients CTD at once in busy times like over the FLOT.  Usually
        // blows up in the SimMover::SimMover(VU_BYTE **stream) constructor or somewhere in that chain.
        if (timertype_ == VU_POSTVUMSGBW_TIMER)
        {
            VuMessageQueue::PostVuMessage(event_);
        }
        else if ((event_->Target()) and (event_->Target() not_eq vuLocalSessionEntity))
        {
            //me123 from Target() to event_->Target()
            retval = event_->Send();
        }
        else
        {
            retval = event_->Dispatch(FALSE);
        }

        //VuMessageQueue::PostVuMessage(event_);
        // sfr: removing unref here, since message may not be processed for some reason
        // unref in destructor
        //event_->UnRef();
        retval = VU_SUCCESS;
    }

    if (EntityId() not_eq vuNullId)
    {
        if (Entity())
        {
            Entity()->Handle(this);
            retval++;
        }
    }

    return retval;
}

//--------------------------------------------------
VuShutdownEvent::VuShutdownEvent(VU_BOOL all)
    : VuEvent(VU_SHUTDOWN_EVENT, vuNullId, vuLocalSessionEntity.get(), TRUE),
      shutdownAll_(all), done_(FALSE)
{
    // empty
}

VuShutdownEvent::~VuShutdownEvent()
{
    // sfr: no more antidb
    // vuAntiDB->Purge();
}

VU_BOOL VuShutdownEvent::DoSend()
{
    return FALSE;
}

int VuShutdownEvent::Size() const
{
    return 0;
}

int VuShutdownEvent::Decode(VU_BYTE **, long *)
{
    // not a net event, so just return
    return 0;
}

int VuShutdownEvent::Encode(VU_BYTE **)
{
    // not a net event, so just return
    return 0;
}

VU_ERRCODE VuShutdownEvent::Process(VU_BOOL)
{
    if ( not done_)
    {
        vuCollectionManager->Shutdown(shutdownAll_);
        done_ = TRUE;
        return VU_SUCCESS;
    }

    return VU_NO_OP;
}

#if defined(VU_SIMPLE_LATENCY)
//--------------------------------------------------
VuTimingMessage::VuTimingMessage(VU_ID entityId, VuTargetEntity* target, VU_BOOL)
    : VuMessage(VU_TIMING_MESSAGE, entityId, target, FALSE)
{
    // empty
    RequestOutOfBandTransmit();
}

VuTimingMessage::VuTimingMessage(VU_ID     senderid,
                                 VU_ID     target)
    : VuMessage(VU_TIMING_MESSAGE, senderid, target)
{
}

VuTimingMessage::~VuTimingMessage()
{
    // empty
}

#define VUTIMINGMESSAGE_SIZE (VUMESSAGE_SIZE+sizeof(VU_TIME)+sizeof(VU_TIME)+sizeof(VU_TIME))
int VuTimingMessage::Size() const
{
    return VuMessage::Size() + sizeof(VU_TIME) + sizeof(VU_TIME) + sizeof(VU_TIME);
}

int VuTimingMessage::Decode(VU_BYTE** buf, long *rem)
{
    VU_BYTE *start = *buf;
    VuSessionEntity *session;

    VuMessage::Decode(buf, rem);

    memcpychk(&sessionRealSendTime_, buf, sizeof(VU_TIME), rem);
    memcpychk(&sessionGameSendTime_, buf, sizeof(VU_TIME), rem);
    memcpychk(&remoteGameTime_,      buf, sizeof(VU_TIME), rem);

    session = (VuSessionEntity*) vuDatabase->Find(EntityId());

    if (session)
    {
        float compression = 0.0F;
        int deltal, delta1, delta2;
        // Apply the new latency
        session->SetLatency((vuxRealTime - sessionRealSendTime_) / 2);

        // Determine game delta - KCK: This is overly complex for now, as I wanted to
        // check some assumptions.
        // Determine time compression
        if (vuxRealTime - sessionRealSendTime_)
        {
            compression = static_cast<float>((vuxGameTime - sessionGameSendTime_) / (vuxRealTime - sessionRealSendTime_));
        }

        // Determine time deltas due to latency
        deltal = FTOL(-1.0F * session->Latency() * compression);
        // Add time deltas due to time differences
        delta1 = (vuxGameTime - remoteGameTime_) - deltal;
        delta2 = -1 * ((remoteGameTime_ - sessionGameSendTime_) - deltal);
        // Apply the new time delta - average the two delta we have
        session->SetTimeDelta((delta1 + delta2) / 2);
    }

    return (int)(*buf - start);
}

int VuTimingMessage::Encode(VU_BYTE** buf)
{
    VU_BYTE *start = *buf;

    VuMessage::Encode(buf);

    memcpy(*buf, &sessionRealSendTime_, sizeof(VU_TIME));
    *buf += sizeof(VU_TIME);
    memcpy(*buf, &sessionGameSendTime_, sizeof(VU_TIME));
    *buf += sizeof(VU_TIME);
    memcpy(*buf, &remoteGameTime_,      sizeof(VU_TIME));
    *buf += sizeof(VU_TIME);

    return (int)(*buf - start);
}

VU_ERRCODE VuTimingMessage::Process(VU_BOOL)
{
    return VU_NO_OP;
}

#endif //VU_SIMPLE_LATENCY

//--------------------------------------------------
VuUnknownMessage::VuUnknownMessage()
    : VuMessage(VU_UNKNOWN_MESSAGE, vuNullId, vuLocalSessionEntity.get(), TRUE)
{
    // empty
}

VuUnknownMessage::~VuUnknownMessage()
{
    // empty
}

VU_BOOL
VuUnknownMessage::DoSend()
{
    return FALSE;
}

int VuUnknownMessage::Size() const
{
    return 0;
}

int VuUnknownMessage::Decode(VU_BYTE **, long *)
{
    // not a net event, so just return
    return 0;
}

int
VuUnknownMessage::Encode(VU_BYTE **)
{
    // Not a net event, so just return
    return 0;
}

VU_ERRCODE
VuUnknownMessage::Process(VU_BOOL)
{
    return VU_NO_OP;
}

