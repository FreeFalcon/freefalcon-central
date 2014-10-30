#include <algorithm>

#include "MsgInc/SendUnitData.h"
#include "mesg.h"
#include "F4Comms.h"
#include "falclib.h"
#include "falcmesg.h"
#include "falcgame.h"
#include "falcsess.h"
#include "InvalidBufferException.h"
#include "Campbase.h"
#include "cmpclass.h"
#include "CampList.h"
#include "falcuser.h"
#include "unit.h"
#include "ui95/chandler.h"

//sfr: added here for checks
using namespace std;

extern C_Handler *gMainHandler;

#define DEBUG_STARTUP 1

extern void CampaignJoinKeepAlive(void);
extern uchar gCampJoinTries;

// Maximum size data block we can send per message
// Since this needs an instance of the message to really be sized correctly,
// I wait and calculate it the first chance I get.
ulong gUnitBlockSize = 0;

FalconSendUnitData::FalconSendUnitData(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) : FalconEvent(SendUnitData, FalconEvent::CampaignThread, entityId, target, loopback)
{
    RequestReliableTransmit();
    dataBlock.unitData = NULL;
    dataBlock.size = 0;
}

FalconSendUnitData::FalconSendUnitData(VU_MSG_TYPE type, VU_ID senderid, VU_ID target) : FalconEvent(SendUnitData, FalconEvent::CampaignThread, senderid, target)
{
    dataBlock.unitData = NULL;
    dataBlock.size = 0;
    type;
}

FalconSendUnitData::~FalconSendUnitData(void)
{
    if (dataBlock.unitData)
        delete[] dataBlock.unitData;

    dataBlock.unitData = NULL;
}

int FalconSendUnitData::Size(void) const
{
    ShiAssert(dataBlock.size >= 0);
    return sizeof(dataBlock) + dataBlock.size + FalconEvent::Size();
}

int FalconSendUnitData::Decode(VU_BYTE **buf, long *rem)
{
    long init = *rem;

    //datablock
    memcpychk(&dataBlock, buf, sizeof(dataBlock), rem);

    if (dataBlock.size < 0)
    {
        char err[50];
        sprintf(err, "%s %d: invalid datablock size", __FILE__, __LINE__);
        throw InvalidBufferException(err);
    }

    dataBlock.unitData = new VU_BYTE[dataBlock.size];
    memcpychk(dataBlock.unitData, buf, dataBlock.size, rem);

    //decode event
    FalconEvent::Decode(buf, rem);

    if ((init - *rem) not_eq Size())
    {
        char err[50];
        sprintf(err, "%s %d: invalid datablock size", __FILE__, __LINE__);
        throw InvalidBufferException(err);
    }

    CampaignJoinKeepAlive();

    uchar *bufptr;
    FalconSessionEntity *session = (FalconSessionEntity*) vuDatabase->Find(dataBlock.owner);

    MonoPrint("RecvUnitData %08x:%08x %d %d %d\n",
              dataBlock.owner,
              dataBlock.set,
              dataBlock.block,
              dataBlock.size);

    if ( not TheCampaign.IsPreLoaded() or not session)
    {
        return (init - *rem);
    }

    if (TheCampaign.Flags bitand CAMP_NEED_UNIT_DATA)
    {
        if (dataBlock.set not_eq session->unitDataReceiveSet)
        {
            MonoPrint("Tossing old Set, starting a new one\n");
            // New data - toss the old stuff
            delete session->unitDataReceiveBuffer;
            session->unitDataReceiveBuffer = NULL;
            session->unitDataReceiveSet = dataBlock.set;
            memset(session->unitDataReceived, 0, FS_MAXBLK / 8);
        }

        if ( not session->unitDataReceiveBuffer)
        {
            session->unitDataReceiveBuffer = new uchar[dataBlock.totalSize];
        }

        long offset;

        if (dataBlock.block + 1 == dataBlock.totalBlocks)
        {
            offset = dataBlock.totalSize - dataBlock.size;
        }
        else
        {
            offset = dataBlock.size * dataBlock.block;
        }

        bufptr = session->unitDataReceiveBuffer + offset;
        memcpy(bufptr, dataBlock.unitData, dataBlock.size);

#ifdef DEBUG_STARTUP
        MonoPrint("Got Unit Block #%d\n", dataBlock.block);
#endif

        gCampJoinTries = 0;

        // Mark this block as being received.
        session->unitDataReceived[dataBlock.block / 8] or_eq (1 << (dataBlock.block % 8));

        // Check if we've gotten all our blocks
        for (int i = 0; i < dataBlock.totalBlocks; i++)
        {
            // if ( not StillNeeded(dataBlock.block, session->unitDataReceived))
            if ( not (session->unitDataReceived[i / 8] bitand (1 << (i % 8))))
            {
                //return size;
                return init - *rem;
            }
        }

        // If we get here, it's because all blocks are read
        TheCampaign.Flags and_eq compl CAMP_NEED_UNIT_DATA;
        bufptr = session->unitDataReceiveBuffer;
        //sfr: hack, i think this is the value
        long lsize = dataBlock.totalSize;

        CampEnterCriticalSection();
        DecodeUnitData((VU_BYTE**) &bufptr, &lsize, session);
        CampLeaveCriticalSection();

        session->unitDataReceiveBuffer = NULL;
        session->unitDataReceiveSet = 0;

        // Let the UI know we've received some data
        if (gMainHandler)
        {
            PostMessage(gMainHandler->GetAppWnd(), FM_GOT_CAMPAIGN_DATA, CAMP_NEED_UNIT_DATA, 0);
        }
    }

    //return size;
    return init - *rem;
}

int FalconSendUnitData::Encode(VU_BYTE **buf)
{
    int size = 0;

    ShiAssert(dataBlock.size >= 0);
    memcpy(*buf, &dataBlock, sizeof(dataBlock));
    *buf += sizeof(dataBlock);
    size += sizeof(dataBlock);
    memcpy(*buf, dataBlock.unitData, dataBlock.size);
    *buf += dataBlock.size;
    size += dataBlock.size;
    size += FalconEvent::Encode(buf);

    ShiAssert(size == Size());

    return size;
}

int FalconSendUnitData::Process(uchar autodisp)
{
    return 0;
    autodisp;
}

// =========================================
// Global functions
// =========================================

void SendCampaignUnitData(FalconSessionEntity *session, VuTargetEntity *target, uchar *blocksNeeded)
{
    int blocks, curBlock = 0, blocksize;
    ulong sizeleft;
    uchar *buffer, *bufptr;
    FalconSendUnitData *msg;
    //CampBaseClass *ent;

    if ( not blocksNeeded)
    {
        int set = rand();

        if ( not set)
            set++;

        if (session->unitDataSendBuffer)
            delete session->unitDataSendBuffer;

        // Encode the unit data
        session->unitDataSendSize = EncodeUnitData((VU_BYTE**)&buffer, FalconLocalSession);
        session->unitDataSendBuffer = buffer;
        session->unitDataSendSet = (short)set;
    }

    // Find the block size, if we havn't already
    if ( not gUnitBlockSize)
    {
        // This is a temporary message, purely for sizing purposes
        FalconSendUnitData tmpmsg(session->Id(), target);
        gUnitBlockSize = F4VuMaxTCPMessageSize - tmpmsg.Size() - 16;
    }

    sizeleft = session->unitDataSendSize;
    blocks = (session->unitDataSendSize + gUnitBlockSize - 1) / gUnitBlockSize;

    buffer = bufptr = session->unitDataSendBuffer;

    while (sizeleft)
    {
        if (sizeleft < gUnitBlockSize)
        {
            blocksize = sizeleft;
            sizeleft = 0;
        }
        else
        {
            blocksize = gUnitBlockSize;
            sizeleft -= gUnitBlockSize;
        }

        msg = new FalconSendUnitData(session->Id(), target);
        msg->dataBlock.size = (short)blocksize;
        msg->dataBlock.unitData = new uchar[blocksize];
        memcpy(msg->dataBlock.unitData, bufptr, blocksize);
        bufptr += blocksize;
        msg->dataBlock.owner = FalconLocalSessionId;
        msg->dataBlock.set = session->unitDataSendSet;
        msg->dataBlock.totalSize = session->unitDataSendSize;
        msg->dataBlock.block = (uchar)curBlock;
        msg->dataBlock.totalBlocks = (uchar)blocks;

        FalconSendMessage(msg, TRUE);
        msg = NULL;

        curBlock++;
    }

    {
#if USE_VU_COLL_FOR_CAMPAIGN
        VuHashIterator deagIt(deaggregatedEntities);

        for (
            CampBaseClass *ent = static_cast<CampBaseClass*>(deagIt.GetFirst());
            ent not_eq NULL;
            ent = static_cast<CampBaseClass*>(deagIt.GetNext())
        )
        {
            ent->SendDeaggregateData(target);
        }

#else
        F4ScopeLock l(deaggregatedMap->getMutex());
        CampBaseClass::SendDeagOp op(VuBin<VuTargetEntity>(FalconLocalGame));
        for_each(deaggregatedMap->begin(), deaggregatedMap->end(), op);
#endif
    }
}

int StillNeeded(int block, uchar gotData[])
{

    ShiAssert(block < FS_MAXBLK);

    if (gotData[block / 8] bitand (1 << (block % 8)))
        return 1;

    return 0;
}


/*
// Split this message into blocks, if necessary
bufptr = buffer;
blocks = (size + gUnitBlockSize - 1) / gUnitBlockSize;

#ifdef DEBUG_STARTUP
MonoPrint("Sending unit data (%d blocks): ",blocks);
#endif

while (sizeleft)
{
if ( not msg)
msg = new FalconSendUnitData(id, target);
if (sizeleft < gUnitBlockSize)
{
msg->dataBlock.size = sizeleft;
msg->dataBlock.unitData = new uchar[sizeleft];
memcpy (msg->dataBlock.unitData, bufptr, sizeleft);
bufptr += sizeleft;
sizeleft = 0;
}
else
{
msg->dataBlock.size = gUnitBlockSize;
msg->dataBlock.unitData = new uchar[gUnitBlockSize];
memcpy (msg->dataBlock.unitData, bufptr, gUnitBlockSize);
bufptr += gUnitBlockSize;
sizeleft -= gUnitBlockSize;
}
msg->dataBlock.owner = FalconLocalSessionId;
msg->dataBlock.set = set;
msg->dataBlock.totalSize = size;
msg->dataBlock.block = curBlock;
msg->dataBlock.totalBlocks = blocks;

#ifdef DEBUG_STARTUP
MonoPrint("%d:%d ",msg->MsgId().id_,msg->dataBlock.size);
#endif

FalconSendMessage(msg,TRUE);
msg = NULL;
curBlock++;
}

#ifdef DEBUG_STARTUP
MonoPrint("\n");
#endif
}
 */

