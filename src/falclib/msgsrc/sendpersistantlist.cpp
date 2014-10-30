/*
 * Machine Generated source file for message "Send Persistant List".
 * NOTE: The functions here must be completed by hand.
 * Generated on 29-July-1997 at 17:49:00
 * Generated from file EVENTS.XLS by Kevin Klemmick
 */

#include "MsgInc/SendPersistantList.h"
#include "CmpClass.h"
#include "Persist.h"
#include "mesg.h"
#include "falclib.h"
#include "falcmesg.h"
#include "falcgame.h"
#include "falcsess.h"
#include "falcuser.h"
#include "ui95/chandler.h"
#include "InvalidBufferException.h"

extern C_Handler *gMainHandler;
extern void CampaignJoinKeepAlive(void);

FalconSendPersistantList::FalconSendPersistantList(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) : FalconEvent(SendPersistantList, FalconEvent::CampaignThread, entityId, target, loopback)
{
    dataBlock.data = NULL;
    dataBlock.size = -1;
}

FalconSendPersistantList::FalconSendPersistantList(VU_MSG_TYPE type, VU_ID senderid, VU_ID target) : FalconEvent(SendPersistantList, FalconEvent::CampaignThread, senderid, target)
{
    dataBlock.data = NULL;
    dataBlock.size = -1;
    type;
}

FalconSendPersistantList::~FalconSendPersistantList(void)
{
    if (dataBlock.data)
        delete dataBlock.data;

    dataBlock.data = NULL;
}

int FalconSendPersistantList::Size() const
{
    ShiAssert(dataBlock.size  >= 0);

    return sizeof(short) + dataBlock.size + FalconEvent::Size();
}

//sfr: changed to long *
int FalconSendPersistantList::Decode(VU_BYTE **buf, long *rem)
{
    long int init = *rem;

    memcpychk(&dataBlock.size, buf, sizeof(short), rem);
    ShiAssert(dataBlock.size >= 0);
    dataBlock.data = new VU_BYTE[dataBlock.size];
    memcpychk(dataBlock.data, buf, dataBlock.size, rem);
    FalconEvent::Decode(buf, rem);

    // ShiAssert (size == Size());

    CampaignJoinKeepAlive();

    return init - *rem;
}

int FalconSendPersistantList::Encode(VU_BYTE **buf)
{
    int size = 0;

    ShiAssert(dataBlock.size >= 0);
    memcpy(*buf, &dataBlock.size, sizeof(short));
    *buf += sizeof(short);
    size += sizeof(short);
    memcpy(*buf, dataBlock.data, dataBlock.size);
    *buf += dataBlock.size;
    size += dataBlock.size;
    size += FalconEvent::Encode(buf);

    ShiAssert(size == Size());

    return size;
}

//sfr: added rem
int FalconSendPersistantList::Process(uchar autodisp)
{
    VU_BYTE* buf;
    long rem;

    if (autodisp or not TheCampaign.IsPreLoaded())
        return -1;

    if (TheCampaign.Flags bitand CAMP_NEED_PERSIST)
    {
        CampaignJoinKeepAlive();

        buf = (VU_BYTE*) dataBlock.data;
        rem = dataBlock.size;
        DecodePersistantList(&buf, &rem);
        TheCampaign.Flags and_eq compl CAMP_NEED_PERSIST;

        if (gMainHandler)
            PostMessage(gMainHandler->GetAppWnd(), FM_GOT_CAMPAIGN_DATA, CAMP_NEED_PERSIST, 0);

        TheCampaign.GotJoinData();
    }

    return 0;
}

