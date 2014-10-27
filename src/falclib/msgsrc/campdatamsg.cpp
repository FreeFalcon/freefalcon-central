/*
 * Machine Generated source file for message "Camp Data Message".
 * NOTE: The functions here must be completed by hand.
 * Generated on 30-September-1997 at 16:41:13
 * Generated from file EVENTS.XLS by MicroProse
 */

#include "MsgInc/CampDataMsg.h"
#include "battalion.h"
#include "mesg.h"
#include "falclib.h"
#include "falcmesg.h"
#include "falcgame.h"
#include "falcsess.h"
#include "falcuser.h"
#include "ui95/chandler.h"

//sfr: added here for checks
#include "InvalidBufferException.h"

extern C_Handler *gMainHandler;

#ifdef DEBUG
int cdecode_count = 0, cencode_count = 0;
#endif

void DecodePrimaryObjectiveList(uchar *data, FalconEntity *fe);
void tactical_set_orders(Battalion bat, VU_ID obj, GridIndex tx, GridIndex ty);

FalconCampDataMessage::FalconCampDataMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) : FalconEvent(CampDataMsg, FalconEvent::CampaignThread, entityId, target, FALSE)
{
    dataBlock.data = NULL;
    dataBlock.size = 0;
    loopback;
}

FalconCampDataMessage::FalconCampDataMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target) : FalconEvent(CampDataMsg, FalconEvent::CampaignThread, senderid, target)
{
    dataBlock.data = NULL;
    dataBlock.size = 0;
    type;
}

FalconCampDataMessage::~FalconCampDataMessage(void)
{
    if (dataBlock.data)
        delete [] dataBlock.data;

    dataBlock.data = NULL;
    dataBlock.size = 0;
}

int FalconCampDataMessage::Size() const
{
    ShiAssert(dataBlock.size >= 0);
    return FalconEvent::Size() +
           sizeof(unsigned int) +
           sizeof(ushort) +
           dataBlock.size;
}

//sfr: changed to long *
int FalconCampDataMessage::Decode(VU_BYTE **buf, long *rem)
{
    long int init = *rem;

#ifdef DEBUG
    cdecode_count ++;
    //MonoPrint ("CampDataMessageDecode %d\n", cdecode_count);
#endif

    FalconEvent::Decode(buf, rem);
    memcpychk(&dataBlock.type, buf, sizeof(unsigned int), rem);
    memcpychk(&dataBlock.size, buf, sizeof(ushort), rem);
    ShiAssert(dataBlock.size >= 0);
    dataBlock.data = new uchar[dataBlock.size];
    memcpychk(dataBlock.data, buf, dataBlock.size, rem);

    ShiAssert(dataBlock.size == Size());

    return init - *rem;
}

int FalconCampDataMessage::Encode(VU_BYTE **buf)
{
    int size = 0;

#ifdef DEBUG
    cencode_count ++;
    //MonoPrint ("CampDataMessageEncode %d\n",cencode_count);
#endif

    ShiAssert(dataBlock.size >= 0);
    size += FalconEvent::Encode(buf);
    memcpy(*buf, &dataBlock.type, sizeof(unsigned int));
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

int FalconCampDataMessage::Process(uchar autodisp)
{
    CampEntity ent = (CampEntity)vuDatabase->Find(EntityId());
    //sfr: we cant use databloc directly because we update pointer
    VU_BYTE *data = dataBlock.data;

    if (autodisp)
        return 0;

    switch (dataBlock.type)
    {
        case campPriorityData:
            // Set the new priority of this objective/these objectives
            DecodePrimaryObjectiveList(data, ent);
            TheCampaign.Flags and_eq compl CAMP_NEED_PERSIST;

            if (gMainHandler)
                PostMessage(gMainHandler->GetAppWnd(), FM_GOT_CAMPAIGN_DATA, CAMP_NEED_PRIORITIES, 0);

            TheCampaign.GotJoinData();
            break;

        case campOrdersData:
            // Set the new orders
        {
            VU_ID tmpId;
            GridIndex x, y;
            uchar *dataPtr = dataBlock.data;

            ShiAssert(ent->IsBattalion());

            memcpy(&tmpId, dataPtr, sizeof(tmpId));
            dataPtr += sizeof(tmpId);
            memcpy(&x, dataPtr, sizeof(x));
            dataPtr += sizeof(x);
            memcpy(&y, dataPtr, sizeof(y));
            dataPtr += sizeof(y);
            tactical_set_orders((Battalion)ent, tmpId, x, y);
        }
        break;

        default:
            break;
    }

    return 0;
}

