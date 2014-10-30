/*
 * Machine Generated source file for message "Objective Message".
 * NOTE: The functions here must be completed by hand.
 * Generated on 24-October-1996 at 13:37:00
 * Generated from file EVENTS.XLS by KEVINK
 */

#ifdef NDEBUG
#undef CAMPTOOL
#endif

#include "MsgInc/ObjectiveMsg.h"
#include "mesg.h"
#include "Objectiv.h"
#include "Find.h"
#include "Team.h"
#include "uiwin.h"
#include "CmpClass.h"
#include "CampMap.h"
#include "FalcUser.h"
#include "falclib.h"
#include "falcmesg.h"
#include "falcgame.h"
#include "falcsess.h"
#include "dispcfg.h"
#include "InvalidBufferException.h"


#ifdef CAMPTOOL
extern void RedrawCell(MapData md, GridIndex x, GridIndex y);
#endif

FalconObjectiveMessage::FalconObjectiveMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) : FalconEvent(ObjectiveMsg, FalconEvent::CampaignThread, entityId, target, loopback)
{
    // Your Code Goes Here
}

FalconObjectiveMessage::FalconObjectiveMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target) : FalconEvent(ObjectiveMsg, FalconEvent::CampaignThread, senderid, target)
{
    // Your Code Goes Here
    type;
}

FalconObjectiveMessage::~FalconObjectiveMessage(void)
{
    // Your Code Goes Here
}

int FalconObjectiveMessage::Process(uchar autodisp)
{
    Objective o;
    static int  updates = 0;

    if (autodisp)
        return 0;

    o = FindObjective(EntityId());

    if ( not o)
        return -1;

    switch (dataBlock.message)
    {
        case objCaptured:
        {
            Team oldteam = o->GetTeam();

            if (oldteam == GetTeam((Control)dataBlock.data1))
                return 0;

            o->SetOwner((Control)dataBlock.data1);
            o->SetObjectiveSupply(0);
            o->SetObjectiveSupplyLosses(0);
            o->SetAbandoned(0);
#ifdef KEV_DEBUG
            MonoPrint("Objective %d captured by Team %d\n", o->GetCampID(), GetTeam((uchar)(dataBlock.data1)));
#endif
            GridIndex x, y;
            o->GetLocation(&x, &y);
#ifdef CAMPTOOL

            RedrawCell(NULL, x, y);
            RebuildFrontList(FALSE, TRUE);
#endif

            if (o->IsPrimary())
                TheCampaign.lastMajorEvent = TheCampaign.CurrentTime;

            TransferInitiative(oldteam, GetTeam((uchar)dataBlock.data1), 5);
            UpdateCampMap(MAP_OWNERSHIP, TheCampaign.CampMapData, x, y);
            updates++;

            if (updates > 5)
            {
                PostMessage(FalconDisplay.appWin, FM_REFRESH_CAMPMAP, 0, 0);
                updates = 0;
            }
        }
        break;

        case objSetSupply:
            o->SetObjectiveSupply((uchar)dataBlock.data1);
            o->SetObjectiveFuel((uchar)dataBlock.data2);
            o->SetObjectiveSupplyLosses(0);
            break;

        case objSetLosses:
            o->SetObjectiveSupplyLosses((uchar)dataBlock.data1);
            break;

        default:
            break;
    }

    return 1;
}

