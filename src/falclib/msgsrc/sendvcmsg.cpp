#include "MsgInc/SendVCMsg.h"
#include "mesg.h"
#include "tac_class.h"
#include "te_defs.h"
#include "falclib.h"
#include "falcmesg.h"
#include "falcgame.h"
#include "falcsess.h"
#include "ui95/chandler.h"
#include "Cmpclass.h"
#include "InvalidBufferException.h"


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

struct sent_vc
{
    int team;
    int type;
    VU_ID id;
    int fid;
    int tol;
    int pts;
    int num;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FalconSendVC::FalconSendVC(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) : FalconEvent(SendVCMsg, FalconEvent::CampaignThread, entityId, target, loopback)
{
    dataBlock.data = NULL;
    dataBlock.size = 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FalconSendVC::FalconSendVC(VU_MSG_TYPE type, VU_ID senderid, VU_ID target) : FalconEvent(SendVCMsg, FalconEvent::CampaignThread, senderid, target)
{
    dataBlock.data = NULL;
    dataBlock.size = 0;
    type;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FalconSendVC::~FalconSendVC(void)
{
    if (dataBlock.data)
    {
        delete dataBlock.data;
    }

    dataBlock.data = NULL;
    dataBlock.size = 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int FalconSendVC::Size(void) const
{
    ShiAssert(dataBlock.size >= 0);
    return (FalconEvent::Size() + sizeof(ushort) + dataBlock.size);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int FalconSendVC::Decode(VU_BYTE **buf, long *rem)
{
    long int init  = *rem;

    FalconEvent::Decode(buf, rem);
    memcpychk(&dataBlock.size, buf, sizeof(ushort), rem);
    ShiAssert(dataBlock.size >= 0);

    if (dataBlock.size)
    {
        dataBlock.data = new uchar[dataBlock.size];
        memcpychk(dataBlock.data, buf, dataBlock.size, rem);
    }

    //ShiAssert (size == Size());

    return init - *rem;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int FalconSendVC::Encode(VU_BYTE **buf)
{
    int
    size;

    ShiAssert(dataBlock.size >= 0);
    size = FalconEvent::Encode(buf);
    memcpy(*buf, &dataBlock.size, sizeof(ushort));
    *buf += sizeof(ushort);
    size += sizeof(ushort);

    if (dataBlock.size)
    {
        memcpy(*buf, dataBlock.data, dataBlock.size);
        *buf += dataBlock.size;
        size += dataBlock.size;
    }

    ShiAssert(size == Size());

    return size;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int FalconSendVC::Process(uchar autodisp)
{
    char
    *ptr;

    int
    count;

    victory_condition
    *vc;

    MonoPrint("Got the VC Message %d\n", dataBlock.size);

    if (TheCampaign.Flags bitand CAMP_NEED_VC)
    {
        count = dataBlock.size / sizeof(sent_vc);

        if (current_tactical_mission)
        {
            vc = current_tactical_mission->get_first_unfiltered_victory_condition();

            while (vc)
            {
                delete vc;
                vc = current_tactical_mission->get_first_unfiltered_victory_condition();
            }

            ptr = (char *) dataBlock.data;

            while (count)
            {
                vc = new victory_condition(current_tactical_mission);

                vc->set_team(*(int*)ptr);
                ptr += 4;

                vc->set_type((victory_type) * (int*)ptr);
                ptr += 4;

                vc->set_vu_id(*(VU_ID*)ptr);
                ptr += 8;

                vc->set_sub_objective(*(int*)ptr);
                ptr += 4;

                vc->set_tolerance(*(int*)ptr);
                ptr += 4;

                vc->set_points(*(int*)ptr);
                ptr += 4;

                vc->set_number(*(int*)ptr);
                ptr += 4;

                count --;
            }
        }

        TheCampaign.Flags and_eq compl CAMP_NEED_VC;

        // Let the UI know we've received some data
        if (gMainHandler)
            PostMessage(gMainHandler->GetAppWnd(), FM_GOT_CAMPAIGN_DATA, CAMP_NEED_VC, 0);
    }

    return 0;
    autodisp;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SendVCData(FalconSessionEntity *requester)
{
    victory_condition
    *vc;

    char
    *ptr;

    int
    count;

    FalconSendVC
    *msg;

    if (current_tactical_mission)
    {
        count = 0;

        vc = current_tactical_mission->get_first_unfiltered_victory_condition();

        while (vc)
        {
            count ++;

            vc = current_tactical_mission->get_next_unfiltered_victory_condition();
        }

        msg = new FalconSendVC(requester->Id(), requester);

        msg->dataBlock.size = count * sizeof(sent_vc);
        msg->dataBlock.data = new unsigned char[count * sizeof(sent_vc)];

        vc = current_tactical_mission->get_first_unfiltered_victory_condition();

        ptr = (char *) msg->dataBlock.data;

        while ((count) and (vc))
        {
            *(int*)ptr = vc->get_team();
            ptr += 4;

            *(int*)ptr = (int) vc->get_type();
            ptr += 4;

            *(VU_ID*)ptr = (VU_ID) vc->get_vu_id();
            ptr += 8;

            *(int*)ptr = vc->get_sub_objective();
            ptr += 4;

            *(int*)ptr = vc->get_tolerance();
            ptr += 4;

            *(int*)ptr = vc->get_points();
            ptr += 4;

            *(int*)ptr = vc->get_number();
            ptr += 4;

            vc = current_tactical_mission->get_next_unfiltered_victory_condition();
        }
    }
    else
    {
        msg = new FalconSendVC(requester->Id(), requester);

        msg->dataBlock.size = 0;
        msg->dataBlock.data = 0;
    }

    FalconSendMessage(msg, TRUE);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
