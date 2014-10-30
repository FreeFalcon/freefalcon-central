#include "MsgInc/UnitMsg.h"
#include "mesg.h"
#include "unit.h"
#include "find.h"
#include "Squadron.h"
#include "falclib.h"
#include "falcmesg.h"
#include "falcgame.h"
#include "falcsess.h"
#include "classtbl.h"
#include "InvalidBufferException.h"


extern void SetPilotStatus(Squadron sq, int pn);
extern void UnsetPilotStatus(Squadron sq, int pn);

FalconUnitMessage::FalconUnitMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) :
    FalconEvent(UnitMsg, FalconEvent::CampaignThread, entityId, target, loopback)
{

    // Your Code Goes Here
}

FalconUnitMessage::FalconUnitMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target) : FalconEvent(UnitMsg, FalconEvent::CampaignThread, senderid, target)
{
    // Your Code Goes Here
    type;
}

FalconUnitMessage::~FalconUnitMessage(void)
{
    // Your Code Goes Here
}

int FalconUnitMessage::Process(uchar autodisp)
{
    Unit u = FindUnit(EntityId());
    int i;

    if (autodisp or not u)
        return 0;

    switch (dataBlock.message)
    {
        case unitSupply:
            u->SupplyUnit(dataBlock.data1, dataBlock.data2);
            break;

        case unitRequestMet:
            u->HandleRequestReceipt(dataBlock.data1, dataBlock.data2, dataBlock.from);
            break;

        case unitSchedulePilots:
            ShiAssert(dataBlock.data1 < PILOTS_PER_SQUADRON);

            if (u->GetType() == TYPE_SQUADRON and u->GetDomain() == DOMAIN_AIR)
            {
                ((Squadron)u)->SetPilotStatus(dataBlock.data1, (uchar)dataBlock.data2);

                if (dataBlock.data3 >= 0)
                    ((Squadron)u)->GetPilotData(dataBlock.data1)->pilot_id = dataBlock.data3;
            }

            break;

        case unitScheduleAC:
            ShiAssert(dataBlock.data1 < VEHICLE_GROUPS_PER_UNIT);

            if (u->GetType() == TYPE_SQUADRON and u->GetDomain() == DOMAIN_AIR)
                ((Squadron)u)->SetSchedule(dataBlock.data1, dataBlock.data2 bitor dataBlock.data3);

            break;

        case unitSetVehicles:
            if (dataBlock.data1 < VEHICLE_GROUPS_PER_UNIT)
                u->SetNumVehicles(dataBlock.data1, dataBlock.data2);
            else if (dataBlock.data1 == UMSG_FROM_RESERVE)
            {
                // if this is a squadron, we need to increment our losses
                if (u->GetType() == TYPE_SQUADRON and u->GetDomain() == DOMAIN_AIR)
                {
                    // KCK HACK: Fake # of losses to make them look more like what you'd expect
                    // in real life..
                    int losses = rand() % (dataBlock.data2 + 1);

                    if (losses)
                        ((Squadron)u)->SetTotalLosses((uchar)(((Squadron)u)->GetTotalLosses() + losses));

                    // KCK MEGAHACK: Actually make it only lose this amount if it's a player
                    if (u->IsSetFalcFlag(FEC_HASPLAYERS))
                        dataBlock.data2 = (short)losses;
                }

                for (i = VEHICLE_GROUPS_PER_UNIT - 1; i >= 0 and dataBlock.data2; i--)
                {
                    while (u->GetNumVehicles(i) and dataBlock.data2)
                    {
                        u->SetNumVehicles(i, u->GetNumVehicles(i) - 1);
                        dataBlock.data2--;
                    }
                }
            }

            // If this is a flight, set the slot data.
            if (u->GetType() == TYPE_FLIGHT and u->GetDomain() == DOMAIN_AIR)
            {
                for (i = 0; i < PILOTS_PER_FLIGHT; i++)
                {
                    if (((Flight)u)->slots[i] > VEHICLE_GROUPS_PER_UNIT or ((Flight)u)->slots[i] == dataBlock.data1)
                    {
                        ((Flight)u)->slots[i] = (uchar)dataBlock.data1;
                        i = PILOTS_PER_FLIGHT;
                    }
                }
            }

            break;

        case unitActivate:
            u->SetInactive(dataBlock.data1);
            break;

        case unitSupport:
        {
            Unit tar = FindUnit(dataBlock.from);

            if ( not tar)
                return 0;

            u->SetTargeted(1);
            u->SetTarget(tar);
            u->SetSupported(1);
            u->SetEngaged(1);
            u->SetCombat(1);
        }
        break;

        case unitRatings:
            break;

        case unitStatistics:
        {
            // This should only happen for squadrons
            if ( not u->IsSquadron())
                return 0;

            // Get Flight from the from data.
            Flight flight = (Flight) FindUnit(dataBlock.from);

            if (dataBlock.data2 == ASTAT_MISSIONS)
            {
                ((Squadron)u)->ScoreMission(dataBlock.data3);

                if (flight)
                {
                    for (i = 0; i < PILOTS_PER_FLIGHT; i++)
                    {
                        PilotClass *pc = flight->GetPilotData(i);

                        if (pc)
                            pc->missions_flown++;
                    }
                }
            }
            else
                MonoPrint("This is an obsolete use of this function\n");
        }
        break;

        case unitScramble:
        {
            // Converts an alert flight to a scramble flight
            if ( not u->IsFlight())
                return 0;

            if (((Flight)u)->GetUnitMission() not_eq AMIS_ALERT)
                return 0;

            Flight enemy = (Flight) FindUnit(dataBlock.from);
            Squadron squadron = (Squadron)((Flight)u)->GetUnitSquadron();

            if ( not squadron or not enemy)
                return 0;

            MissionRequest mis = new MissionRequestClass;

            mis->targetID = enemy->Id();
            mis->who = squadron->GetTeam();
            mis->vs = enemy->GetTeam();
            mis->tot = Camp_GetCurrentTime();
            enemy->GetLocation(&mis->tx, &mis->ty);
            mis->flags = AMIS_IMMEDIATE;
            mis->target_num = 255;
            mis->tot_type = TYPE_NE;
            mis->mission = AMIS_INTERCEPT;
            mis->context = 0;
            ((Flight)u)->SetUnitMission(mis->mission);
            ((Flight)u)->BuildMission(mis);

            // Broadcast a full update to the game
            VuEvent *event = new VuFullUpdateEvent(u, FalconLocalGame);
            VuMessageQueue::PostVuMessage(event);
        }
        break;

        default:
            break;
    }

    return 0;
}

