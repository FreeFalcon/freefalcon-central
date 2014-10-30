#include "MsgInc/RequestAircraftSlot.h"
#include "MsgInc/SendAircraftSlot.h"
#include "mesg.h"
#include "uicomms.h"
#include "pilot.h"
#include "falclib.h"
#include "falcmesg.h"
#include "falcgame.h"
#include "falcsess.h"
#include "flight.h"
#include "dogfight.h"
#include "InvalidBufferException.h"

extern int RegroupFlight(Flight flight);
extern void UI_Refresh(void);

UI_RequestAircraftSlot::UI_RequestAircraftSlot(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) : FalconEvent(RequestAircraftSlot, FalconEvent::SimThread, entityId, target, loopback)
{
    RequestOutOfBandTransmit();
    RequestReliableTransmit();
    dataBlock.current_pilot_slot = 255;
}

UI_RequestAircraftSlot::UI_RequestAircraftSlot(VU_MSG_TYPE type, VU_ID senderid, VU_ID target) :
    FalconEvent(RequestAircraftSlot, FalconEvent::SimThread, senderid, target)
{
    RequestOutOfBandTransmit();
    RequestReliableTransmit();
}

UI_RequestAircraftSlot::~UI_RequestAircraftSlot()
{
}

int UI_RequestAircraftSlot::Process(uchar autodisp)
{
    Flight flight;
    int retval = FALSE;

    if (autodisp or not FalconLocalGame)
    {
        return FALSE;
    }

    flight = (Flight) Entity();

    // Figure out what we're doing:
    switch (dataBlock.request_type)
    {
        case REQUEST_FLIGHT_DELETE:
            MonoPrint("Request Flight Delete %08x\n", flight);

            if ( not flight)
                return FALSE;

            RegroupFlight(flight);
            vuDatabase->Remove(flight);
            break;

        case REQUEST_TEAM_CHANGE:
            if ( not flight)
                return FALSE;

            retval = ChangeFlightTeam(flight);
            UI_Refresh();
            break;

        case REQUEST_TYPE_CHANGE:
            if ( not flight)
                return FALSE;

            retval = ChangeFlightType(flight);
            UI_Refresh();
            break;

        case REQUEST_CALLSIGN_CHANGE:
            if ( not flight)
                return FALSE;

            retval = ChangeFlightCallsign(flight);
            break;

        case REQUEST_SKILL_CHANGE:
            if ( not flight)
                return FALSE;

            retval = ChangePilotSkill(flight);
            break;

        case REQUEST_SLOT_LEAVE:
            if ( not flight)
                return FALSE;

            retval = EmptyFlightSlot(flight);
            break;

        case REQUEST_UI_UPDATE:
            break;

        case REQUEST_SLOT_JOIN_PLAYER:
        case REQUEST_SLOT_JOIN_AI:
        default:
            retval = AddFlightSlot(flight);
            break;
    }

#if 0

    // sfr: test
    if (FalconLocalGame->IsLocal() and dataBlock.request_type not_eq REQUEST_UI_UPDATE)
    {
        UI_RequestAircraftSlot* msga = new UI_RequestAircraftSlot(FalconNullId, FalconLocalGame);
        msga->dataBlock.request_type = REQUEST_UI_UPDATE;
        // sfr: flag bug, setting all but loopback :/
        //msga->flags_ or_eq compl VU_LOOPBACK_MSG_FLAG;
        msga->flags_ and_eq compl VU_LOOPBACK_MSG_FLAG;
        FalconSendMessage(msga, TRUE);
        //VuTimerEvent *timer = new VuTimerEvent(0, vuxRealTime + 1000, VU_DELAY_TIMER, msga);
        //VuMessageQueue::PostVuMessage(timer);
    }

#endif
    UI_Refresh();
    return retval;
}

int UI_RequestAircraftSlot::Encode(VU_BYTE **buf)
{
    int size;

    size = FalconEvent::Encode(buf);
    memcpy(*buf, &dataBlock, sizeof(dataBlock));
    *buf += sizeof(dataBlock);
    size += sizeof(dataBlock);
    return size;
};

int UI_RequestAircraftSlot::Decode(VU_BYTE **buf, long *rem)
{
    long init = *rem;

    FalconEvent::Decode(buf, rem);
    memcpychk(&dataBlock, buf, sizeof(dataBlock), rem);
    return init - *rem;
};


int UI_RequestAircraftSlot::ChangeFlightTeam(Flight flight)
{
    MonoPrint("Request Team Change %08x To %d\n", flight, dataBlock.team);
    flight->SetOwner(dataBlock.team);
    UnsetCallsignID(flight->callsign_id, flight->callsign_num);
    GetDogfightCallsign(flight);
    SetCallsignID(flight->callsign_id, flight->callsign_num);
    flight->DoFullUpdate();
    return TRUE;
}

int UI_RequestAircraftSlot::ChangeFlightType(Flight flight)
{
    MonoPrint("Request Type Change %08x To %d\n", flight, dataBlock.requested_type);
    flight->SetEntityType(dataBlock.requested_type);
    flight->class_data = (UnitClassDataType*) Falcon4ClassTable[dataBlock.requested_type - VU_LAST_ENTITY_TYPE].dataPtr;
    // Falcon4EntityClassType* classPtr = (Falcon4EntityClassType*)&Falcon4ClassTable[dataBlock.requested_type-VU_LAST_ENTITY_TYPE];
    // flight->SetUnitSType(classPtr->vuClassData.classInfo_[VU_STYPE]);
    // flight->SetUnitSPType(classPtr->vuClassData.classInfo_[VU_SPTYPE]);
    SimDogfight.ApplySettingsToFlight(flight);
    flight->DoFullUpdate();
    UI_Refresh();
    return TRUE;
}

int UI_RequestAircraftSlot::ChangeFlightCallsign(Flight flight)
{
    MonoPrint("Request Callsign Change %08x To %d\n", flight, 0);
    // KCK TODO: Try to meet request
    flight->callsign_id = 0;
    flight->callsign_num = 0;
    return TRUE;
}

int UI_RequestAircraftSlot::ChangePilotSkill(Flight flight)
{
    MonoPrint("Request Skill Change %08x To %d\n", flight, dataBlock.requested_skill);

    if (flight->pilots[dataBlock.requested_slot] not_eq 255)
    {
        flight->pilots[dataBlock.requested_slot] = dataBlock.requested_skill;
        flight->MakeFlightDirty(DIRTY_PILOTS, DDP[152].priority);
        // flight->MakeFlightDirty (DIRTY_PILOTS, SEND_RELIABLE);
    }

    // KCK TODO: Dirty flight, so new skill will be sent.
    return TRUE;
}

int UI_RequestAircraftSlot::EmptyFlightSlot(Flight flight)
{
    MonoPrint("Empty Flight Slot %08x From %d To %d\n", flight, dataBlock.current_pilot_slot, dataBlock.requested_slot);

    // This player is currently assigned to an aircraft in this flight - unassign first
    if (dataBlock.requested_slot < 255)
    {
        if (flight->player_slots[dataBlock.requested_slot])
        {
            flight->player_slots[dataBlock.requested_slot] = NO_PILOT;
        }

        flight->MakeFlightDirty(DIRTY_PILOTS, SEND_RELIABLEANDOOB);
    }

    if (FalconLocalGame->GetGameType() == game_Dogfight)
    {
        // Clean out the AI plane as well
        flight->player_slots[dataBlock.requested_slot] = NO_PILOT;
        flight->pilots[dataBlock.requested_slot] = NO_PILOT;
        flight->plane_stats[dataBlock.requested_slot] = AIRCRAFT_NOT_ASSIGNED;
        flight->SetNumVehicles(dataBlock.requested_slot, 0);
        flight->MakeFlightDirty(DIRTY_PILOTS, SEND_RELIABLEANDOOB);

        if (flight->GetTotalVehicles() < 1)
        {
            RegroupFlight(flight);
            vuDatabase->Remove(flight); // Also, remove it from database immediately
        }
    }

    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int UI_RequestAircraftSlot::AddFlightSlot(Flight flight)
{
    FalconSessionEntity *requester = (FalconSessionEntity*)vuDatabase->Find(dataBlock.requesting_session);
    UI_SendAircraftSlot *msg = NULL;
    int i, got_slot = -1, retval = 0;

    if ( not requester)
    {
        return FALSE;
    }

    MonoPrint("\nRequest Slot Join %08x : ", flight);

    if ( not flight)
    {
        if (FalconLocalGame->GetGameType() == game_Dogfight)
        {
            MonoPrint("NT %d T %d : ", dataBlock.requested_type, dataBlock.team);

            // We'll create a new one.
            flight = NewFlight(dataBlock.requested_type, NULL, NULL);
            flight->BuildElements();
            flight->SetRoster(0);
            flight->SetOwner(dataBlock.team);
            flight->SetLocation(0, 0);
            flight->SetUnitDestination(0, 0);
            // flight->SetFinal(1);
            flight->SetUnitMissionID(AMIS_SWEEP);
            flight->SetFalcFlag(FEC_REGENERATING);
            flight->last_player_slot = PILOTS_PER_FLIGHT;
            GetDogfightCallsign(flight);
            SetCallsignID(flight->callsign_id, flight->callsign_num);
            got_slot = 0;

            for (i = 0; i < PILOTS_PER_FLIGHT; i++)
            {
                flight->plane_stats[i] = AIRCRAFT_NOT_ASSIGNED;
                flight->pilots[i] = NO_PILOT;
                flight->player_slots[i] = 255;
                flight->slots[i] = 0;
            }
        }
        else
        {
            MonoPrint("  Failed - NULL Dogfight\n");
            return FALSE;
        }
    }

    if (dataBlock.request_type == REQUEST_SLOT_JOIN_PLAYER)
    {
        MonoPrint("R %08x %d %d : ", requester->GetPlayerFlight(), requester->GetAircraftNum(), requester->GetPilotSlot());
        // Get rid of any previous slot.
        Flight oldflight = requester->GetAssignedPlayerFlight();
        int oldreq = dataBlock.requested_slot;
        int oldslt = dataBlock.current_pilot_slot;
        dataBlock.requested_slot = requester->GetAssignedAircraftNum();
        dataBlock.current_pilot_slot = requester->GetAssignedPilotSlot();

        if (
            (oldflight == flight) and 
            (FalconLocalGame->GetGameType() == game_Dogfight) and 
            (flight->GetTotalVehicles() == 1)
        )
        {
            return TRUE;
        }

        if (oldflight)
        {
            EmptyFlightSlot(oldflight);
        }

        dataBlock.requested_slot = (uchar)oldreq;
        dataBlock.current_pilot_slot = (uchar)oldslt;
    }
    // This player is currently assigned to an aircraft in this flight - unassign first
    else if (dataBlock.current_pilot_slot < 255)
    {
        MonoPrint("  Unassign %d\n", dataBlock.current_pilot_slot);

        for (i = 0; i < PILOTS_PER_FLIGHT; i++)
        {
            if (flight->player_slots[i] == dataBlock.current_pilot_slot)
            {
                flight->player_slots[i] = 255;

                if (FalconLocalGame->GetGameType() == game_Dogfight)
                {
                    flight->pilots[i] = NO_PILOT;
                    flight->plane_stats[i] = AIRCRAFT_NOT_ASSIGNED;
                    //flight->MakeFlightDirty (DIRTY_PILOTS, DDP[155].priority);
                    flight->MakeFlightDirty(DIRTY_PILOTS, SEND_RELIABLEANDOOB);
                }
            }
        }
    }

    // Now try and fill the request
    if (flight)
    {
        // In Dogfight, we look for an empty slot between 0 and 3, and add an aircraft if we find
        // an empty one.
        if (FalconLocalGame->GetGameType() == game_Dogfight)
        {
            for (i = 0; i < PILOTS_PER_FLIGHT and not retval; i++)
            {
                if (flight->plane_stats[i] == AIRCRAFT_NOT_ASSIGNED)
                {
                    flight->plane_stats[i] = AIRCRAFT_AVAILABLE;
                    flight->slots[i] = 0;

                    if (dataBlock.request_type == REQUEST_SLOT_JOIN_PLAYER)
                    {
                        msg = new UI_SendAircraftSlot(flight->Id(), requester);
                        msg->dataBlock.requesting_session = requester->Id();
                        msg->dataBlock.host_id = FalconLocalSession->Id();
                        msg->dataBlock.game_id = FalconLocalGame->Id();
                        msg->dataBlock.team = flight->GetTeam();
                        msg->dataBlock.game_type = (uchar)FalconLocalGame->GetGameType();

                        if (dataBlock.current_pilot_slot == 255)
                        {
                            // Assign a new pilot slot
                            flight->last_player_slot++;
                            dataBlock.current_pilot_slot = flight->last_player_slot;
                        }

                        msg->dataBlock.result = REQUEST_RESULT_SUCCESS;
                        msg->dataBlock.got_pilot_slot = dataBlock.current_pilot_slot;
                        flight->player_slots[i] = dataBlock.current_pilot_slot;
                        flight->pilots[i] = NO_PILOT;
                        msg->dataBlock.got_slot = (uchar)i;
                        requester->SetAssignedPlayerFlight(flight);
                        requester->SetAssignedAircraftNum(msg->dataBlock.got_slot);
                        requester->SetAssignedPilotSlot(msg->dataBlock.got_pilot_slot);
                        flight->MakeFlightDirty(DIRTY_PILOTS, SEND_RELIABLEANDOOB);
                        flight->MakeFlightDirty(DIRTY_PLANE_STATS, SEND_RELIABLEANDOOB);
                    }
                    else
                    {
                        MonoPrint("What the fuck ????");
                        flight->player_slots[i] = 255;
                        flight->pilots[i] = dataBlock.requested_skill;
                        flight->MakeFlightDirty(DIRTY_PILOTS, SEND_RELIABLEANDOOB);
                    }

                    flight->SetNumVehicles(i, 1);
                    retval = TRUE;
                }
            }
        }
        // In Tactical Engagement and Campaign, if there is an AI there, we can take the slot,
        // otherwise, we try the next slot until we run out of aircraft or find an available AI.
        else if (dataBlock.request_type not_eq REQUEST_SLOT_JOIN_AI)
        {
            MonoPrint("Request Slot Join %d\n", dataBlock.requested_slot);

            msg = new UI_SendAircraftSlot(flight->Id(), requester);
            msg->dataBlock.requesting_session = requester->Id();
            msg->dataBlock.host_id = FalconLocalSession->Id();
            msg->dataBlock.game_id = FalconLocalGame->Id();
            msg->dataBlock.team = flight->GetTeam();
            msg->dataBlock.game_type = (uchar)FalconLocalGame->GetGameType();
            msg->dataBlock.result = REQUEST_RESULT_DENIED;
            msg->dataBlock.got_pilot_slot = 255;
            msg->dataBlock.got_slot = 255;

            for (i = 0; i < PILOTS_PER_FLIGHT; i++)
            {
                MonoPrint("  Slot %d Stats %d\n", flight->player_slots[i], flight->plane_stats[i]);
            }

            if (flight->player_slots[dataBlock.requested_slot] == 255 and flight->plane_stats[dataBlock.requested_slot] == AIRCRAFT_AVAILABLE)
            {
                // The requested aircraft slot is empty
                if (dataBlock.current_pilot_slot == 255)
                {
                    // Assign a new pilot slot
                    flight->last_player_slot++;
                    dataBlock.current_pilot_slot = flight->last_player_slot;
                }

                flight->player_slots[dataBlock.requested_slot] = dataBlock.current_pilot_slot;
                msg->dataBlock.result = REQUEST_RESULT_SUCCESS;
                msg->dataBlock.got_pilot_slot = dataBlock.current_pilot_slot;
                msg->dataBlock.got_slot = dataBlock.requested_slot;
                retval = TRUE;
            }
            else
            {
                // Try and find an empty one
                for (i = 0; i < PILOTS_PER_FLIGHT and not retval; i++)
                {
                    if (flight->player_slots[i] == 255 and flight->plane_stats[i] == AIRCRAFT_AVAILABLE)
                    {
                        if (dataBlock.current_pilot_slot == 255)
                        {
                            // Assign a new pilot slot
                            flight->last_player_slot++;
                            dataBlock.current_pilot_slot = flight->last_player_slot;
                        }

                        flight->player_slots[i] = dataBlock.current_pilot_slot;
                        msg->dataBlock.result = REQUEST_RESULT_SUCCESS;
                        msg->dataBlock.got_pilot_slot = dataBlock.current_pilot_slot;
                        msg->dataBlock.got_slot = (uchar)i;
                        retval = TRUE;
                    }
                }
            }

            if (retval)
            {
                MonoPrint("Ok\n");
            }
            else
            {
                MonoPrint("Failed\n");
            }

            requester->SetAssignedPlayerFlight(flight);
            requester->SetAssignedAircraftNum(msg->dataBlock.got_slot);
            requester->SetAssignedPilotSlot(msg->dataBlock.got_pilot_slot);
            //flight->MakeFlightDirty (DIRTY_PILOTS, DDP[159].priority);
            flight->MakeFlightDirty(DIRTY_PILOTS, SEND_RELIABLEANDOOB);

            for (i = 0; i < PILOTS_PER_FLIGHT; i++)
            {
                MonoPrint("  Slot %d Stats %d\n", flight->player_slots[i], flight->plane_stats[i]);
            }
        }

        if (FalconLocalGame->GetGameType() == game_Dogfight)
        {
            SimDogfight.ApplySettingsToFlight(flight);
        }
    }

    flight->SetSendCreate(VuEntity::VU_SC_SEND_OOB);
    vuDatabase->/*Quick*/Insert(flight);
    //flight->MakeFlightDirty (DIRTY_PILOTS, DDP[160].priority);
    flight->MakeFlightDirty(DIRTY_PILOTS, SEND_RELIABLEANDOOB);
    //flight->MakeFlightDirty (DIRTY_PLANE_STATS, DDP[161].priority);
    flight->MakeFlightDirty(DIRTY_PLANE_STATS, SEND_RELIABLEANDOOB);

    if (msg)
    {
        FalconSendMessage(msg, TRUE);
    }

    UI_Refresh();

    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
