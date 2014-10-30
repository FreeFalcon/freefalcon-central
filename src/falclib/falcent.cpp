#include <string.h>
#include "f4vu.h"
#include "ClassTbl.h"
#include "sim/include/simbase.h"
#include "sim/include/aircrft.h"
#include "campbase.h"
#include "falcmesg.h"
#include "FalcSess.h"
#include "FalcEnt.h"
#include "Find.h"
#include "Campaign.h"
#include "MsgInc/CampDirtyDataMsg.h"
#include "MsgInc/SimDirtyDataMsg.h"
#include "package.h"
#include "gndunit.h"
#include "team.h"
#include "InvalidBufferException.h"

using namespace std;

// ==================================
// FreeFalcon Entity functions
// ==================================

FalconEntity::FalconEntity(ushort type, VU_ID_NUMBER eid) : VuEntity(type, eid)
{
    InitLocalData();
}

FalconEntity::FalconEntity(VU_BYTE** stream, long *rem) : VuEntity(stream, rem)
{
    InitLocalData();
    memcpychk(&falconType, stream, sizeof(falconType), rem);

    if (gCampDataVersion >= 32)
    {
        memcpychk(&falconFlags, stream, sizeof(uchar), rem);
    }
}

FalconEntity::FalconEntity(FILE* filePtr) : VuEntity(filePtr)
{
    InitLocalData();
    fread(&falconType, sizeof(falconType), 1, filePtr);

    if (gCampDataVersion >= 32)
    {
        fread(&falconFlags, sizeof(uchar), 1, filePtr);
    }
}

FalconEntity::~FalconEntity(void)
{
    CleanupLocalData();
}

void FalconEntity::InitData()
{
    InitLocalData();
}

void FalconEntity::InitLocalData()
{
    falconType = 0;
    falconFlags = 0;
    dirty_falcent = 0;
    dirty_classes = 0;
    dirty_score = 0;
    feLocalFlags = 0;
}

void FalconEntity::CleanupData()
{
    CleanupLocalData();
}

void FalconEntity::CleanupLocalData()
{
    // nothing to do here
}


int FalconEntity::Save(VU_BYTE** stream)
{
    int saveSize = VuEntity::Save(stream);

    memcpy(*stream, &falconType, sizeof(falconType));
    *stream += sizeof(falconType);
    saveSize += sizeof(falconType);
    memcpy(*stream, &falconFlags, sizeof(uchar));
    *stream += sizeof(uchar);
    saveSize += sizeof(uchar);
    return (saveSize);
}

int FalconEntity::Save(FILE* filePtr)
{
    int saveSize = VuEntity::Save(filePtr);

    fwrite(&falconType, sizeof(falconType), 1, filePtr);
    saveSize += sizeof(falconType);
    fwrite(&falconFlags, sizeof(uchar), 1, filePtr);
    saveSize += sizeof(uchar);
    return (saveSize);
}

int FalconEntity::SaveSize(void)
{
    return VuEntity::SaveSize() + sizeof(falconType) + sizeof(uchar);
    //   return VuEntity::SaveSize();
}

uchar FalconEntity::GetDomain(void)
{
    return Falcon4ClassTable[Type() - VU_LAST_ENTITY_TYPE].vuClassData.classInfo_[VU_DOMAIN];
}

uchar* FalconEntity::GetDamageModifiers(void)
{
    return DefaultDamageMods;
}

void FalconEntity::GetLocation(GridIndex* x, GridIndex* y) const
{
    ::vector v;

    v.x = XPos();
    v.y = YPos();
    v.z = 0.0F;
    ConvertSimToGrid(&v, x, y);
}

void FalconEntity::SetOwner(FalconSessionEntity* session)
{
    // Set the owner to session
    share_.ownerId_ = session->OwnerId();
}

void FalconEntity::SetOwner(VU_ID sessionId)
{
    // Set the owner to session
    share_.ownerId_ = sessionId;
}

void FalconEntity::DoFullUpdate(void)
{
    VuEvent *event = new VuFullUpdateEvent(this, FalconLocalGame);
    event->RequestReliableTransmit();
    VuMessageQueue::PostVuMessage(event);
}

int FalconEntity::calc_dirty_bucket(int dirty_score)
{
    int ds = dirty_score; //just for debugging

    if (dirty_score == 0)
    {
        return -1;
    }

    // sfr : new
    int bin = 0;

    while ((dirty_score bitand 0x1) == 0)
    {
        ++bin;
        dirty_score >>= 4;
    }

    return bin;

#if 0
    // sfr: old
    else if (dirty_score <= SEND_EVENTUALLY)
    {
        return 1;
    }
    else if (dirty_score <= SEND_SOMETIME)
    {
        return 2;
    }
    else if (dirty_score <= SEND_LATER)
    {
        return 3;
    }
    else if (dirty_score <= SEND_SOON)
    {
        return 4;
    }
    else if (dirty_score <= SEND_NOW)
    {
        return 5;
    }
    else if (dirty_score <= SEND_RELIABLE)
    {
        return 6;
    }
    else if (dirty_score <= SEND_OOB)
    {
        return 7;
    }
    else if (dirty_score > SEND_OOB)
    {
        return 8;
    }
    else
    {
        ShiAssert("This can't happen at all...");
        return 0;
    }

#endif
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FalconEntity::ClearDirty(void)
{
    // dodirtydata removes it from lists nows
    dirty_classes = 0;
    dirty_score = 0;
#if 0
    //sfr: old
    int bin;
    VuEnterCriticalSection();
    bin = calc_dirty_bucket();
    assert((bin >= 0) and (bin < MAX_DIRTY_BUCKETS));
    dirty_classes = 0;
    dirty_score = 0;
    DirtyBucket[bin]->Remove(this);
    VuExitCriticalSection();
#endif
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FalconEntity::MakeDirty(Dirty_Class bits, Dirtyness score)
{
    dirty_classes or_eq bits;

    // sfr: for player entities, always send reliable and immediatelly
    if (IsPlayer())
    {
        score = SEND_RELIABLEANDOOB;
    }

    // send only local units which are active (in DB) and if the unit is more dirty than currently is
    if (
        ( not IsLocal()) or
        (VuState() not_eq VU_MEM_ACTIVE) or
        (score <= dirty_score) or
 not (TheCampaign.Flags bitand CAMP_LOADED)
    )
    {
        return;
    }

    dirty_score = score;
    int bin = calc_dirty_bucket(score);

    if (IsSimBase())
    {
        F4ScopeLock lock(simDirtyMutexes[bin]);
#if USE_VU_COLL_FOR_DIRTY
        simDirtyBuckets[bin]->ForcedInsert(this);
#else
        simDirtyBuckets[bin]->push_back(FalconEntityBin(this));
#endif
    }
    else
    {
        F4ScopeLock lock(campDirtyMutexes[bin]);
#if USE_VU_COLL_FOR_DIRTY
        campDirtyBuckets[bin]->ForcedInsert(this);
#else
        campDirtyBuckets[bin]->push_back(FalconEntityBin(this));
#endif
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int FalconEntity::EncodeDirty(unsigned char **stream)
{
    uchar *start;

    start = *stream;

    // MonoPrint ("SendDirty %08x%08x %08x\n", Id(), dirty_classes);

    *(short*)*stream = (short)dirty_classes;
    *stream += sizeof(short);

    if (dirty_classes bitand DIRTY_FALCON_ENTITY)
    {
        *(uchar *)(*stream) = falconFlags;
        *stream += sizeof(uchar);
    }

    if (dirty_classes bitand DIRTY_CAMPAIGN_BASE)
    {
        ((CampBaseClass*)this)->WriteDirty(stream);
    }

    if (dirty_classes bitand DIRTY_OBJECTIVE)
    {
        ((ObjectiveClass*)this)->WriteDirty(stream);
    }

    if (dirty_classes bitand DIRTY_UNIT)
    {
        ((UnitClass*)this)->WriteDirty(stream);
    }

    if (dirty_classes bitand DIRTY_PACKAGE)
    {
        ((PackageClass*)this)->WriteDirty(stream);
    }

    if (dirty_classes bitand DIRTY_SQUADRON)
    {
        ((SquadronClass*)this)->WriteDirty(stream);
    }

    if (dirty_classes bitand DIRTY_FLIGHT)
    {
        ((FlightClass*)this)->WriteDirty(stream);
    }

    if (dirty_classes bitand DIRTY_GROUND_UNIT)
    {
        ((GroundUnitClass*)this)->WriteDirty(stream);
    }

    if (dirty_classes bitand DIRTY_BATTALION)
    {
        ((BattalionClass*)this)->WriteDirty(stream);
    }

    if (dirty_classes bitand DIRTY_TEAM)
    {
        ((TeamClass*)this)->WriteDirty(stream);
    }

    if (dirty_classes bitand DIRTY_SIM_BASE)
    {
        ((SimBaseClass*)this)->WriteDirty(stream);
    }

    if (dirty_classes bitand DIRTY_AIRCRAFT)
    {
        ((AircraftClass*)this)->WriteDirty(stream);
    }

    return *stream - start;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//sfr: changed this function prototype, see falcent.h
//changed body too
void FalconEntity::DecodeDirty(unsigned char **stream, long *rem)
{

    short bits;

    //read bits and update count
    memcpychk(&bits, stream, sizeof(short), rem);

    if (bits bitand DIRTY_FALCON_ENTITY)
    {
        memcpychk(&falconFlags, stream, sizeof(uchar), rem);
    }

    if (bits bitand DIRTY_CAMPAIGN_BASE)
    {
        ((CampBaseClass*)this)->ReadDirty(stream, rem);
    }

    if (bits bitand DIRTY_OBJECTIVE)
    {
        ((ObjectiveClass*)this)->ReadDirty(stream, rem);
    }

    if (bits bitand DIRTY_UNIT)
    {
        ((UnitClass*)this)->ReadDirty(stream, rem);
    }

    if (bits bitand DIRTY_PACKAGE)
    {
        ((PackageClass*)this)->ReadDirty(stream, rem);
    }

    if (bits bitand DIRTY_SQUADRON)
    {
        ((SquadronClass*)this)->ReadDirty(stream, rem);
    }

    if (bits bitand DIRTY_FLIGHT)
    {
        ((FlightClass*)this)->ReadDirty(stream, rem);
    }

    if (bits bitand DIRTY_GROUND_UNIT)
    {
        ((GroundUnitClass*)this)->ReadDirty(stream, rem);
    }

    if (bits bitand DIRTY_BATTALION)
    {
        ((BattalionClass*)this)->ReadDirty(stream, rem);
    }

    if (bits bitand DIRTY_TEAM)
    {
        ((TeamClass*)this)->ReadDirty(stream, rem);
    }

    if (bits bitand DIRTY_SIM_BASE)
    {
        ((SimBaseClass*)this)->ReadDirty(stream, rem);
    }

    if (bits bitand DIRTY_AIRCRAFT)
    {
        ((AircraftClass*)this)->ReadDirty(stream, rem);
    }
}


//sfr: changed identation and calls to DecodeDirty
void FalconEntity::DoCampaignDirtyData(VU_TIME realTime)
{
    static VU_TIME lastSent = 0;

    if ( not (TheCampaign.Flags bitand CAMP_LOADED) or ((realTime - lastSent) < SIMDIRTYDATA_INTERVAL))
    {
        return;
    }

    lastSent = realTime;

    //a buffer for encoding decoding stuff, usually small, but theres a big one > 512
    unsigned char buffer[1024];
    //pointers to the buffer above
    unsigned char *bufptr;

    // max number of sends to do on this run
    int toSend = 16;

    // run all buckets
    for (int bucket = MAX_DIRTY_BUCKETS - 1; (bucket >= 0); --bucket)
    {
        // send at least one from each bucket
        bool sent = false;
        F4ScopeLock lock(campDirtyMutexes[bucket]);
#if USE_VU_COLL_FOR_DIRTY
        FalconEntity *current;

        while ((current = static_cast<FalconEntity*>(campDirtyBuckets[bucket]->PopHead())) not_eq NULL)
#else
        while ( not campDirtyBuckets[bucket]->empty())
#endif
        {
            //bucket 7 and 8 are OOB(out of band), the others must respect bw
            if ((bucket < 7) and (toSend < 0) and (sent))
            {
                break;
            }

#if not USE_VU_COLL_FOR_DIRTY
            FalconEntityBin current(campDirtyBuckets[bucket]->front());
            campDirtyBuckets[bucket]->pop_front();
#endif

            // can happen if inserted multiple times
            if ((current->dirty_classes == 0) or (current->GetDirty() == 0))
            {
                continue;
            }

            // encode and clear dirty to send message
            bufptr = buffer;
            long bufSize = static_cast<long>(current->EncodeDirty(&bufptr));
            CampDirtyData *campDirtyMsg;
            campDirtyMsg = new CampDirtyData(current->Id(), FalconLocalGame, FALSE);
            campDirtyMsg->dataBlock.size = static_cast<int>(bufSize);
            campDirtyMsg->dataBlock.data = new VU_BYTE[bufSize];
            memcpy(campDirtyMsg->dataBlock.data, buffer, bufSize);

            if (bucket >= 7)
            {
                campDirtyMsg->RequestOutOfBandTransmit();
            }

            FalconSendMessage(campDirtyMsg, bucket >= 6);
            current->ClearDirty();
            --toSend;
            sent = true;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FalconEntity::DoSimDirtyData(VU_TIME realTime)
{
    // only do if initialized
    if ( not (TheCampaign.Flags bitand CAMP_LOADED) or (simDirtyBuckets == NULL))
    {
        return;
    }

    // last time we sent sim dirty data
    static VU_TIME lastSent = 0;

    if ((realTime - lastSent) < SIMDIRTYDATA_INTERVAL)
    {
        return;
    }

    lastSent = realTime;

    //a buffer for encoding decoding stuff (biggest I seen was around 32)
    unsigned char buffer[128];
    //pointer to the buffer above
    unsigned char *bufptr;

    // max number of sends to do
    // but we send at least one on each bucket
    int toSend = 30;

    //now we go through all buckets while can send. Send at least one per bucket
    for (int bucket = MAX_DIRTY_BUCKETS - 1; (bucket >= 0); bucket --)
    {
        bool sent = false;
        F4ScopeLock lock(simDirtyMutexes[bucket]);
#if USE_VU_COLL_FOR_DIRTY
        FalconEntity *current;

        while ((current = static_cast<FalconEntity*>(simDirtyBuckets[bucket]->PopHead())) not_eq NULL)
#else
        while ( not simDirtyBuckets[bucket]->empty())
#endif
        {
            //bucket 7 and 8 are OOB (out of band), others must respect limit
            if ((bucket < 7) and (toSend < 0) and (sent))
            {
                break;
            }

#if not USE_VU_COLL_FOR_DIRTY
            // get the entity in FIFO fashion
            FalconEntityBin current(simDirtyBuckets[bucket]->front());
            simDirtyBuckets[bucket]->pop_front();
#endif

            // discard non dirty entities, can happen if it was inserted more than once
            if ((current->dirty_classes == 0) or (current->GetDirty() == 0))
            {
                continue;
            }

            //point to the buffer
            bufptr = buffer;

            // encode and clear dirtyness to send dirty message
            long bufSize = static_cast<long>(current->EncodeDirty(&bufptr));
            SimDirtyData *simDirtyData;
            simDirtyData = new SimDirtyData(current->Id(), FalconLocalGame, FALSE);
            simDirtyData->dataBlock.size = static_cast<int>(bufSize);
            simDirtyData->dataBlock.data = new VU_BYTE[bufSize];
            memcpy(simDirtyData->dataBlock.data, buffer, bufSize);

            if (bucket >= 7)
            {
                simDirtyData->RequestOutOfBandTransmit();
            }

            //send the message
            FalconSendMessage(simDirtyData, bucket >= 6);
            current->ClearDirty();
            toSend--;
            sent = true;
        }
    }
}

int FalconEntity::GetRadarType(void)
{
    return RDR_NO_RADAR;
}
void FalconEntity::MakeFlagsDirty(void)
{
    //MakeFalconEntityDirty (DIRTY_FALCON_FLAGS, DDP[148].priority);
    MakeFalconEntityDirty(DIRTY_FALCON_FLAGS, SEND_RELIABLEANDOOB);
}

void FalconEntity::MakeFalconEntityDirty(Dirty_Falcon_Entity bits, Dirtyness score)
{
    if (( not IsLocal()) or (VuState() not_eq VU_MEM_ACTIVE))
    {
        return;
    }

    dirty_falcent or_eq bits;

    MakeDirty(DIRTY_FALCON_ENTITY, score);
}

#if NEW_REMOVAL_CALLBACK
VU_ERRCODE FalconEntity::RemovalCallback()
{
    //CleanupData();
    return VU_SUCCESS;
}
#endif


/////////////////
// SPOT ENTITY //
/////////////////
SpotEntity::SpotEntity(ushort type) : FalconEntity(type, GetIdFromNamespace(VolatileNS))
{
    // spotentities are sent oob
    SetSendCreate(VuEntity::VU_SC_SEND_OOB);
    SetYPRDelta(0, 0, 0);
}

SpotEntity::SpotEntity(VU_BYTE ** data, long *rem) : FalconEntity(data, rem)
{
    SetYPRDelta(0, 0, 0);
}
