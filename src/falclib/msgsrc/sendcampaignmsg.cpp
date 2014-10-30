#include "MsgInc/SendCampaignMsg.h"
#include "mesg.h"
#include "CmpClass.h"
#include "FalcSess.h"
#include "MsgInc/RequestCampaignData.h"
#include "Campaign.h"
#include "Weather.h"
#include "ui/include/falcuser.h"
#include "ui95/chandler.h"
#include "falclib.h"
#include "falcmesg.h"
#include "falcgame.h"
#include "falcsess.h"
#include "InvalidBufferException.h"


extern C_Handler *gMainHandler;
extern int gCurrentDataVersion;
extern int gCampDataVersion;

extern void CampaignJoinKeepAlive(void);

FalconSendCampaign::FalconSendCampaign(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) : FalconEvent(SendCampaignMsg, FalconEvent::CampaignThread, entityId, target, loopback)
{
    uchar *buffer;
    dataBlock.dataSize = (short)TheCampaign.Encode(&buffer);
    dataBlock.campInfo = buffer;
}

FalconSendCampaign::FalconSendCampaign(VU_MSG_TYPE type, VU_ID senderid, VU_ID target) : FalconEvent(SendCampaignMsg, FalconEvent::CampaignThread, senderid, target)
{
    dataBlock.campInfo = NULL;
    dataBlock.dataSize = -1;
}

FalconSendCampaign::~FalconSendCampaign(void)
{
    delete dataBlock.campInfo;
    dataBlock.campInfo = NULL;
}

int FalconSendCampaign::Size() const
{
    ShiAssert(dataBlock.dataSize >= 0);
    return sizeof(dataBlock) + dataBlock.dataSize + FalconEvent::Size();
}

int FalconSendCampaign::Decode(VU_BYTE **buf, long *rem)
{
    long init = *rem;

    ShiAssert(dataBlock.dataSize >= 0);
    memcpychk(&dataBlock, buf, sizeof(dataBlock), rem);
    dataBlock.campInfo = new uchar[dataBlock.dataSize];
    memcpychk(dataBlock.campInfo, buf, dataBlock.dataSize, rem);
    FalconEvent::Decode(buf, rem);

    int size = init - *rem;
    ShiAssert(size == Size());

    CampaignJoinKeepAlive();

    return size;
}

int FalconSendCampaign::Encode(VU_BYTE **buf)
{
    int size = 0;

    memcpy(*buf, &dataBlock, sizeof(dataBlock));
    *buf += sizeof(dataBlock);
    size += sizeof(dataBlock);
    memcpy(*buf, dataBlock.campInfo, dataBlock.dataSize);
    *buf += dataBlock.dataSize;
    size += dataBlock.dataSize;
    ShiAssert(dataBlock.dataSize >= 0);
    size += FalconEvent::Encode(buf);

    ShiAssert(size == Size());

    return size;
};

int FalconSendCampaign::Process(uchar autodisp)
{
    uchar *bufptr = dataBlock.campInfo;
    long bufSize = dataBlock.dataSize;

    if (autodisp)
    {
        return 0;
    }

    if (TheCampaign.Flags bitand CAMP_NEED_PRELOAD)
    {
        CampEnterCriticalSection();
        gCampDataVersion = gCurrentDataVersion;
        TheCampaign.Decode(&bufptr, &bufSize);
        TheCampaign.Flags and_eq compl CAMP_NEED_PRELOAD;
        TheCampaign.Flags or_eq CAMP_PRELOADED;
        CampLeaveCriticalSection();

        if (gMainHandler)
        {
            PostMessage(gMainHandler->GetAppWnd(), FM_GOT_CAMPAIGN_DATA, CAMP_NEED_PRELOAD, 0);
        }

        return 1;
    }
    else
    {
        return 0;
    }
}
