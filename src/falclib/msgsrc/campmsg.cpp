/*
 * Machine Generated source file for message "Camp Messages".
 * NOTE: The functions here must be completed by hand.
 * Generated on 05-November-1996 at 17:39:12
 * Generated from file EVENTS.XLS by Leon Rosenshein
 */

#include "msginc/campmsg.h"
#include "mesg.h"
#include "find.h"
#include "campbase.h"
#include "campweap.h"
#include "unit.h"
#include "objectiv.h"
#include "squadron.h"
#include "cmpclass.h"
#include "misseval.h"
#include "update.h"
#include "simdrive.h"
#include "aiinput.h"
#include "msginc/radiochattermsg.h"
#include "classtbl.h"
#include "falcuser.h"
#include "dispcfg.h"
#include "falclib.h"
#include "falcmesg.h"
#include "falcgame.h"
#include "falcsess.h"
#include "invalidbufferexception.h"

void DeaggregateOwnershipCheck(CampEntity the_entity,
                               FalconSessionEntity *session, int deag_request);

FalconCampMessage::FalconCampMessage(VU_ID entityId, VuTargetEntity *target,
                                     VU_BOOL loopback)
    : FalconEvent(CampMsg, FalconEvent::CampaignThread, entityId, target,
                  loopback)
{
    RequestReliableTransmit();
    // Your Code Goes Here
}

FalconCampMessage::FalconCampMessage(VU_MSG_TYPE type, VU_ID senderid,
                                     VU_ID target)
    : FalconEvent(CampMsg, FalconEvent::CampaignThread, senderid, target)
{
    // Your Code Goes Here
    type;
}

FalconCampMessage::~FalconCampMessage(void)
{
    // Your Code Goes Here
}

int FalconCampMessage::Process(uchar autodisp)
{
    CampEntity e;

    if (autodisp)
        return 0;

    e = FindEntity(EntityId());

    if (not e)
        return 0;

    switch (dataBlock.message)
    {
    case campAttackWarning:
        break;

    case campFiredOn:
        break;

    case campSpotted:
        e->SetSpotted((Team)dataBlock.data1, TheCampaign.CurrentTime);
        break;

    case campRepair:
        if (e->IsUnit())
            ((Unit)e)->ChangeVehicles(dataBlock.data1);
        else if (e->IsObjective())
            ((Objective)e)->Repair();

        break;

    default:
        break;
    }

    return 0;
}

// ==================================================
// Local functions
// ==================================================
