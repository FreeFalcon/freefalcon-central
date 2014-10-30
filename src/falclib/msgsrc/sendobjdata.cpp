#include "MsgInc/SendObjData.h"
#include "mesg.h"
#include "F4Comms.h"
#include "falclib.h"
#include "falcmesg.h"
#include "falcgame.h"
#include "falcsess.h"
#include "cmpclass.h"
#include "falcuser.h"
#include "objectiv.h"
#include "ui95/chandler.h"
#include "InvalidBufferException.h"


#define DEBUG_STARTUP 1

extern C_Handler *gMainHandler;
extern void CampaignJoinKeepAlive(void);

// Maximum size data block we can send per message
// Since this needs an instance of the message to really be sized correctly,
// I wait and calculate it the first chance I get.
ulong gObjBlockSize = 0;

FalconSendObjData::FalconSendObjData(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) : FalconEvent(SendObjData, FalconEvent::CampaignThread, entityId, target, loopback)
{
    dataBlock.objData = NULL;
    dataBlock.size = 0;
}

FalconSendObjData::FalconSendObjData(VU_MSG_TYPE type, VU_ID senderid, VU_ID target) : FalconEvent(SendObjData, FalconEvent::CampaignThread, senderid, target)
{
    dataBlock.objData = NULL;
    dataBlock.size = 0;
}

FalconSendObjData::~FalconSendObjData()
{
    delete[] dataBlock.objData;
}

int FalconSendObjData::Size() const
{
    ShiAssert(dataBlock.size >= 0);
    return sizeof(dataBlock) + dataBlock.size + FalconEvent::Size();
}


int FalconSendObjData::Decode(VU_BYTE **buf, long *rem)
{
    long init = *rem;

    memcpychk(&dataBlock, buf, sizeof(dataBlock), rem);
    ShiAssert(dataBlock.size >= 0);
    dataBlock.objData = new VU_BYTE[dataBlock.size];
    memcpychk(dataBlock.objData, buf, dataBlock.size, rem);
    FalconEvent::Decode(buf, rem);


    //ShiAssert ( size == Size() );

    CampaignJoinKeepAlive();

    return init - *rem;
    //size;
}

int FalconSendObjData::Encode(VU_BYTE **buf)
{
    int size = 0;

    ShiAssert(dataBlock.size >= 0);
    memcpy(*buf, &dataBlock, sizeof(dataBlock));
    *buf += sizeof(dataBlock);
    size += sizeof(dataBlock);
    memcpy(*buf, dataBlock.objData, dataBlock.size);
    *buf += dataBlock.size;
    size += dataBlock.size;
    size += FalconEvent::Encode(buf);

    ShiAssert(size == Size());

    return size;
}

int FalconSendObjData::Process(uchar autodisp)
{
    uchar *bufptr;
    FalconSessionEntity *session = (FalconSessionEntity*) vuDatabase->Find(dataBlock.owner);

    if (autodisp or not TheCampaign.IsPreLoaded() or not session)
        return -1;

    if (TheCampaign.Flags bitand CAMP_NEED_OBJ_DELTAS)
    {
        CampaignJoinKeepAlive();

        if (dataBlock.set not_eq session->objDataReceiveSet)
        {
            // New data - toss the old stuff
            delete session->objDataReceiveBuffer;
            session->objDataReceiveBuffer = NULL;
            session->objDataReceiveSet = dataBlock.set;
            memset(session->objDataReceived, 0, FS_MAXBLK / 8);
        }

        if ( not session->objDataReceiveBuffer)
            session->objDataReceiveBuffer = new uchar[dataBlock.totalSize];

        // Find the block size, if we havn't already
        if ( not gObjBlockSize)
            gObjBlockSize = F4VuMaxTCPMessageSize - (Size() - dataBlock.size);

        bufptr = session->objDataReceiveBuffer + dataBlock.block * gObjBlockSize;
        memcpy(bufptr, dataBlock.objData, dataBlock.size);

#ifdef DEBUG_STARTUP
        MonoPrint("Got Obj Block #%d\n", dataBlock.block);
#endif

        // Mark this block as being received.
        session->objDataReceived[dataBlock.block / 8] or_eq (1 << (dataBlock.block % 8));

        // Check if we've gotten all our blocks
        for (int i = 0; i < dataBlock.totalBlocks; i++)
        {
            if ( not (session->objDataReceived[i / 8] bitand (1 << (i % 8))))
                return 0;
        }

        // If we get here, it's because all blocks are read
        TheCampaign.Flags and_eq compl CAMP_NEED_OBJ_DELTAS;
        bufptr = session->objDataReceiveBuffer;

        CampEnterCriticalSection();
        //sfr: added for checks
        long bufSize = gObjBlockSize;
#ifdef MP_DEBUG
        DecodeObjectiveDeltas((VU_BYTE**) &bufptr, &bufSize, session);
#else

        try
        {
            DecodeObjectiveDeltas((VU_BYTE**) &bufptr, &bufSize, session);
        }
        catch (...)
        {
            char err[200];
            sprintf(err, "%s %d: error decoding objective deltas", __FILE__, __LINE__);
            throw InvalidBufferException(err);
        }

#endif
        CampLeaveCriticalSection();

        session->objDataReceiveBuffer = NULL;
        session->objDataReceiveSet = 0;

        // Let the UI know we've received some data
        if (gMainHandler)
            PostMessage(gMainHandler->GetAppWnd(), FM_GOT_CAMPAIGN_DATA, CAMP_NEED_OBJ_DELTAS, 0);
    }
    else
    {
        MonoPrint("**** Re-received OBJ DELTAS\n");
    }

    return 0;
}

// =========================================
// Global functions
// =========================================

void SendObjectiveDeltas(FalconSessionEntity *session, VuTargetEntity *target, uchar *blocksNeeded)
{
    int blocks, needed = 0, curBlock = 0, blocksize;
    ulong sizeleft;
    uchar *buffer, *bufptr;
    FalconSendObjData *msg;

    if ( not blocksNeeded)
    {
        int set = rand();

        if ( not set)
            set++;

        // Encode the objective data
        session->objDataSendSize = EncodeObjectiveDeltas((VU_BYTE**)&buffer, NULL);
        session->objDataSendBuffer = buffer;
        session->objDataSendSet = (short)set;
    }

    // Find the block size, if we havn't already
    if ( not gObjBlockSize)
    {
        // This is a temporary message, purely for sizing purposes
        FalconSendObjData msg(session->Id(), target);
        gObjBlockSize = F4VuMaxTCPMessageSize - msg.Size();
    }

    sizeleft = session->objDataSendSize;
    blocks = (session->objDataSendSize + gObjBlockSize - 1) / gObjBlockSize;

#ifdef DEBUG_STARTUP

    if ( not blocksNeeded)
        MonoPrint("Sending objective data (%d of %d blocks): ", needed, blocks);
    else
        MonoPrint("Resending objective data (%d of %d blocks): ", needed, blocks);

#endif

    buffer = bufptr = session->objDataSendBuffer;

    while (sizeleft)
    {
        if (sizeleft < gObjBlockSize)
        {
            // The last block to send
            blocksize = sizeleft;
            sizeleft = 0;
        }
        else
        {
            // Any old block
            blocksize = gObjBlockSize;
            sizeleft -= gObjBlockSize;
        }

        if ( not blocksNeeded or not (blocksNeeded[curBlock / 8] bitand (1 << (curBlock % 8))))
        {
            msg = new FalconSendObjData(session->Id(), target);
            msg->dataBlock.size = (short)blocksize;
            msg->dataBlock.objData = new uchar[blocksize];
            memcpy(msg->dataBlock.objData, bufptr, blocksize);
            bufptr += blocksize;
            msg->dataBlock.owner = FalconLocalSessionId;
            msg->dataBlock.set = session->objDataSendSet;
            msg->dataBlock.totalSize = session->objDataSendSize;
            msg->dataBlock.block = (uchar)curBlock;
            msg->dataBlock.totalBlocks = (uchar)blocks;

#ifdef DEBUG_STARTUP
            MonoPrint("%d:%d ", msg->dataBlock.block, msg->dataBlock.size);
#endif

            FalconSendMessage(msg, TRUE);
            msg = NULL;
        }

        curBlock++;
    }

#ifdef DEBUG_STARTUP
    MonoPrint("\n");
#endif
}

