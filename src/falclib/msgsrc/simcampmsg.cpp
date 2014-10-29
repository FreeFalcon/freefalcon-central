#include <algorithm>

#include "MsgInc/SimCampMsg.h"
#include "mesg.h"
#include "Campaign.h"
#include "TimerThread.h"//me123
#include "falclib.h"
#include "falcmesg.h"
#include "falcgame.h"
#include "falcsess.h"
#include "Campbase.h"
#include "CampList.h"

//sfr: added here for checks
#include "InvalidBufferException.h"
using namespace std;


FalconSimCampMessage::FalconSimCampMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback)
    : FalconEvent(SimCampMsg, FalconEvent::SimThread, entityId, target, loopback)
{
    dataBlock.size = 0;
    dataBlock.data = NULL;
    RequestReliableTransmit();
}

FalconSimCampMessage::FalconSimCampMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target)
    : FalconEvent(SimCampMsg, FalconEvent::SimThread, senderid, target)
{
    dataBlock.size = 0;
    dataBlock.data = NULL;
    type;
}

FalconSimCampMessage::~FalconSimCampMessage()
{
    if (dataBlock.data)
    {
        delete [] dataBlock.data;
    }

    dataBlock.data = NULL;
    dataBlock.size = 0;
}

int FalconSimCampMessage::Size(void) const
{
    ShiAssert(dataBlock.size >= 0);
    int size = FalconEvent::Size() + sizeof(VU_ID) + sizeof(unsigned int) + sizeof(ushort) + dataBlock.size;
    return size;
}

int FalconSimCampMessage::Decode(VU_BYTE **buf, long *rem)
{
    long init = *rem;

    FalconEvent::Decode(buf, rem);
    memcpychk(&dataBlock.from, buf, sizeof(VU_ID), rem);
    memcpychk(&dataBlock.message, buf, sizeof(unsigned int), rem);
    memcpychk(&dataBlock.size, buf, sizeof(ushort), rem);
    dataBlock.data = new uchar[dataBlock.size];
    memcpychk(dataBlock.data, buf, dataBlock.size, rem);

    int size = init - *rem;
    ShiAssert(size == Size());

    return size;
}

int FalconSimCampMessage::Encode(VU_BYTE **buf)
{
    int size = 0;

    ShiAssert(dataBlock.size >= 0);
    size += FalconEvent::Encode(buf);
    memcpy(*buf, &dataBlock.from, sizeof(VU_ID));
    *buf += sizeof(VU_ID);
    size += sizeof(VU_ID);
    memcpy(*buf, &dataBlock.message, sizeof(unsigned int));
    *buf += sizeof(unsigned int);
    size += sizeof(unsigned int);
    memcpy(*buf, &dataBlock.size, sizeof(ushort));
    *buf += sizeof(ushort);
    size += sizeof(ushort);
    memcpy(*buf, dataBlock.data, dataBlock.size);
    *buf += dataBlock.size;
    size += dataBlock.size;

    ShiAssert(size == Size());

    return size;
}

int FalconSimCampMessage::Process(uchar autodisp)
{

    VuBin<VuEntity> esafe(vuDatabase->Find(EntityId()));
    VuBin<VuEntity> ssafe(vuDatabase->Find(dataBlock.from));
    CampBaseClass *ent = static_cast<CampBaseClass*>(esafe.get());
    FalconSessionEntity *session = static_cast<FalconSessionEntity*>(ssafe.get());

    if (autodisp or not ent or not session or not FalconLocalGame)
    {
        return 0;
    }

    CampEnterCriticalSection();

    switch (dataBlock.message)
    {
        case simcampReaggregate:
            ent->Reaggregate(session);
            break;

        case simcampDeaggregate:
            ent->Deaggregate(session);
            break;

        case simcampChangeOwner:
            ent->RecordCurrentState(session, FALSE);
            ent->SetDeagOwner(session->Id());
            break;

        case simcampRequestDeagData:
            ent->SendDeaggregateData(FalconLocalGame);
            break;

        case simcampReaggregateFromData:
            ent->ReaggregateFromData(dataBlock.data, dataBlock.size);
            break;

        case simcampDeaggregateFromData:
            ent->DeaggregateFromData(dataBlock.data, dataBlock.size);
            break;

        case simcampChangeOwnerFromData:
            break;

        case simcampRequestAllDeagData:
        {
            SetTimeCompression(1);
            // me123 if a client is calling this he's in the pie
            // let's set the compresion to 1 on the host so we don'e fuck up the realtime
            // because the clients stops transmitting
            // timecompresion and we go to 64 again for awhile.
            {
#if USE_VU_COLL_FOR_CAMPAIGN
                VuHashIterator deagIt(deaggregatedEntities);

                for (
                    CampEntity c = static_cast<CampEntity>(deagIt.GetFirst());
                    c not_eq NULL;
                    c = static_cast<CampEntity>(deagIt.GetNext())
                )
                {
                    if (( not c->IsAggregate()) and (c->IsLocal()))
                    {
                        c->SendDeaggregateData(FalconLocalGame);
                    }
                }

#else
                F4ScopeLock l(deaggregatedMap->getMutex());
                CampBaseClass::SendDeagOp op(VuBin<VuTargetEntity>(FalconLocalGame));
                for_each(deaggregatedMap->begin(), deaggregatedMap->end(), op);
#endif
            }
        }
        break;
    }

    CampLeaveCriticalSection();

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
