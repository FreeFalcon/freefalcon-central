/*
 * Machine Generated source file for message "Radio Chatter Message".
 * NOTE: The functions here must be completed by hand.
 * Generated on 20-November-1997 at 16:32:51
 * Generated from file EVENTS.XLS by MicroProse
 */

#include "MsgInc/RadioChatterMsg.h"
#include "mesg.h"
#include "SimDrive.h"
#include "F4Vu.h"
#include "fsound.h"
#include "FalcSnd/LHSP.h"
#include "FalcSnd/FalcVoice.h"
#include "FalcSnd/VoiceManager.h"
#include "FalcSnd/VoiceMapper.h"
#include "Sim/Include/Digi.h"
#include "Find.h"
#include "CmpClass.h"
#include "tacan.h"
#include "aircrft.h"
#include "falcsess.h"
#include "timerthread.h"
#include "PlayerOp.h"
#include "flight.h"

#include "falclib.h"
#include "falcmesg.h"
#include "falcgame.h"
#include "falcsess.h"
#include "InvalidBufferException.h"


// ======================
// Globals
// ======================

short gDefaultAWACSCallSign = 45;
short gDefaultAWACSFlightNum = 1;

extern int noUIcomms;
extern int g_nChatterInterval; // FRB message interval time

#ifdef _DEBUG
unsigned short TimesCalled[LastComm] = {0};
#endif

// ======================
// Externals
// ======================

extern VoiceFilter *voiceFilter;

// ======================
// Defines
// ======================

float MAX_RADIO_RANGE = 1822800.0F; // Radio range, in feet (300nm)(maximum range at which you hear any calls)
float RADIO_PROX_RANGE = 243050.0F; // Range of proximity filter (40nm)
enum
{
    WARP_NONE,
    WARP_PACKAGE,
    WARP_FLIGHT,
    WARP_PLANE
};

// ======================
// Functions
// ======================

FalconRadioChatterMessage::FalconRadioChatterMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) : FalconEvent(RadioChatterMsg, FalconEvent::SimThread, entityId, target, loopback)
{
    dataBlock.time_to_play = 0;
    memset(dataBlock.edata, 0, sizeof(short)*MAX_EVALS_PER_RADIO_MESSAGE);
    dataBlock.voice_id = 0;
    dataBlock.to = MESSAGE_FOR_TEAM;

    // MonoPrint ("RadioChatter\n");
}

FalconRadioChatterMessage::FalconRadioChatterMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target) : FalconEvent(RadioChatterMsg, FalconEvent::SimThread, senderid, target)
{
    dataBlock.time_to_play = 0;
    memset(dataBlock.edata, 0, sizeof(short)*MAX_EVALS_PER_RADIO_MESSAGE);
    dataBlock.voice_id = 0;
    type;
}

FalconRadioChatterMessage::~FalconRadioChatterMessage(void)
{
}

int FalconRadioChatterMessage::Size() const
{
    // Start by assuming we have to send all the eval data
    int size = FalconEvent::Size() + sizeof(dataBlock) + 1;

    // See how many eval elements we'll be able to strip off
    for (int i = MAX_EVALS_PER_RADIO_MESSAGE - 1; i >= 0; i--)
    {

        if (dataBlock.edata[i] == 0)
        {
            // This one has default data, so we'll be able strip it off
            size -= sizeof(dataBlock.edata[i]);
        }
        else
        {
            // We encountered non-default data, so we have to stop
            break;
        }
    }

    return size;
}

//sfr: changed nEvals to VU_BYTE, as in encode
int FalconRadioChatterMessage::Decode(VU_BYTE **buf, long *rem)
{
    long int init = *rem;
    VU_BYTE nEvals;

    // Handle the base class
    FalconEvent::Decode(buf, rem);

    // Get the invariant data
    memcpychk(&dataBlock, buf, sizeof(dataBlock) - sizeof(dataBlock.edata), rem);

    // Get the number of evals actually sent
    memcpychk(&nEvals, buf, sizeof(VU_BYTE), rem);

    // Read in the evals -- leave the rest default (0)
    for (int i = 0; i < nEvals; i++)
    {
        memcpychk(&dataBlock.edata[i], buf, sizeof(short), rem);
    }

    return init - *rem;
}

int FalconRadioChatterMessage::Encode(VU_BYTE **buf)
{
    int size;
    VU_BYTE nEvals;

    // Handle the base class
    size = FalconEvent::Encode(buf);

    // Send the invariant data
    memcpy(*buf, &dataBlock, sizeof(dataBlock) - sizeof(dataBlock.edata));
    *buf += sizeof(dataBlock) - sizeof(dataBlock.edata);
    size += sizeof(dataBlock) - sizeof(dataBlock.edata);

    // See how many eval elements we'll be able to strip off
    /* for (nEvals=MAX_EVALS_PER_RADIO_MESSAGE; (nEvals>0) and (dataBlock.edata[nEvals-1] == 0); nEvals--) {
     if (dataBlock.edata[nEvals-1] == 0) {
     // This one has default data, so we'll be able strip it off
     } else {
     // We encountered non-default data, so we have to stop
     break;
     }
     }*/

    //sfr: find number of eval elements
    for (nEvals = MAX_EVALS_PER_RADIO_MESSAGE; (nEvals > 0) and (dataBlock.edata[nEvals - 1] == 0); nEvals--) ;

    // Send the number of evals actually sent
    **buf = nEvals;
    *buf += sizeof(VU_BYTE);
    size += sizeof(VU_BYTE) + nEvals * sizeof(dataBlock.edata[0]);

    // Send the non-default evals
    for (int i = 0; i < nEvals; i++)
    {
        *(short*)(*buf) = dataBlock.edata[i];
        *buf += sizeof(dataBlock.edata[i]);
    }

    return size;
}

int FalconRadioChatterMessage::Process(uchar autodisp)
{
    if (autodisp)
        return 0;

#define RADIO_TEST 0
#if RADIO_TEST

    // sfr: test hack for testing client radio calls
    if (this->dataBlock.message not_eq rcOUTSIDEAIRSPEED)
    {
        return 0;
    }

#endif

    FalconEntity *from = (FalconEntity*) vuDatabase->Find(dataBlock.from);
    FalconEntity *to = (FalconEntity*) vuDatabase->Find(EntityId());
    FalconEntity *us = (FalconEntity*) FalconLocalSession->GetPlayerEntity();
    CampBaseClass *from_entity = NULL;
    CampBaseClass *from_package = NULL;
    CampBaseClass *to_package = NULL;
    CampBaseClass *to_entity = NULL;
    FlightClass *player_flight = (FlightClass*) FalconLocalSession->GetPlayerFlight();
    PackageClass *player_package = NULL;
    int message = dataBlock.message;
    char channel = 0, playbits = 0;
    VU_ID fromID = dataBlock.from, toID = FalconNullId;


    if (from and (from->IsDead() or (from->IsVehicle() and not ((SimVehicleClass*)from)->HasPilot())))
    {
        return 0;
    }

    // M.N. turn off all players radio chatter if wanted
    if (from and from->IsPlayer() and not PlayerOptions.PlayerRadioVoice)
    {
        return 0;
    }

    if (from and from->IsPlayer())
    {
        FalconSessionEntity *session;
        VuSessionsIterator sit(FalconLocalGame);
        session = (FalconSessionEntity*) sit.GetFirst();

        while (session)
        {
            if (from == session->GetPlayerEntity())
            {
                dataBlock.voice_id = session->GetVoiceID();
                break;
            }

            session = (FalconSessionEntity*) sit.GetNext();
        }
    }

    if (FalconLocalSession->GetFlyState() not_eq FLYSTATE_FLYING and SimDriver.RunningCampaign() and not noUIcomms)
        us = FalconLocalSession->GetPlayerSquadron();

#ifdef _DEBUG
    //count how often each message is called
    TimesCalled[message]++;
#endif
    // KCK: Ignore all this if we're not in the sim or if we've got fucked up data

    //MonoPrint("Processing Chatter Message ID: %d  %d\n", message, us);

    if ((FalconLocalSession->GetFlyState() == FLYSTATE_FLYING) or
        (SimDriver.RunningCampaign() and not noUIcomms) or
        (to and to == us))
    {
        if ( not us)
            return -1;

        //if the sender is more than 300nm away we can't hear it
        if (from and us and (DistSqu(from->XPos(), from->YPos(), us->XPos(), us->YPos()) > MAX_RADIO_RANGE * MAX_RADIO_RANGE))
            return -1;

        if (player_flight)
            player_package = static_cast<PackageClass*>(player_flight->GetUnitParent());

        //until data is correct this only screws things up
        if (voiceFilter and not to and from and us->GetTeam() == from->GetTeam()) // KCK: Added and not to -> basically, if you want it to warp, don't specify a target
        {
            if (DistSqu(from->XPos(), from->YPos(), us->XPos(), us->YPos()) > RADIO_PROX_RANGE * RADIO_PROX_RANGE * 4.0F) //80nm
                return -1;

            switch (voiceFilter->GetWarp(message))
            {
                case WARP_PACKAGE:
                    if (player_package)
                        to = player_package->GetFirstUnitElement();

                    if (to and to->IsFlight())
                    {
                        dataBlock.edata[0] = (static_cast<FlightClass*>(to))->callsign_id;
                        dataBlock.edata[1] = ConvertFlightNumberToCallNumber(((Flight)to)->callsign_num);
                    }

                    break;

                case WARP_FLIGHT:
                    to = (FalconEntity *)player_flight;

                    if (player_flight)
                    {
                        dataBlock.edata[0] = player_flight->callsign_id;
                        dataBlock.edata[1] = ConvertFlightNumberToCallNumber(player_flight->callsign_num);
                    }

                    break;

                case WARP_PLANE:
                    to = (FalconEntity *)us;

                    if (player_flight and us->IsSim())
                    {
                        dataBlock.edata[0] = (short)((SimVehicleClass*)us)->GetCallsignIdx();
                        dataBlock.edata[1] = (short)player_flight->GetPilotCallNumber(((SimVehicleClass*)us)->pilotSlot);
                    }
            }
        }

        if (to and to->IsSim())
            to_entity = (CampBaseClass*)((SimBaseClass*)to)->GetCampaignObject();
        else if (to and to->IsCampaign())
            to_entity = (CampBaseClass*)to;


        if (from and from->IsSim())
            from_entity = (CampBaseClass*)((SimBaseClass*)from)->GetCampaignObject();
        else if (from and from->IsCampaign())
            from_entity = (CampBaseClass*)from;

        if (FalconLocalGame and FalconLocalGame->GetGameType() not_eq game_InstantAction and 
            FalconLocalGame->GetGameType() not_eq game_Dogfight)
        {
            if (from_entity and from_entity->IsFlight())
                from_package = (Package)((Flight)from_entity)->GetUnitParent();

            if (to_entity and to_entity->IsFlight())
                to_package = (Package)((Flight)to_entity)->GetUnitParent();
        }

        //we want to hear everything sent to our flight or us
        if ((to_entity and to_entity == player_flight) or (to == us))
            channel = 1;

        //we want to hear everything said by us
        if (from == us)
            channel = 1;

        //campaign/taceng
        if ((to_entity and to_entity == player_flight) or (from_entity and from_entity == player_flight))
            playbits or_eq TOFROM_FLIGHT;

        //the seemingly weird check for the team is because everyone in instant action is in the same package but on different teams
        if (to and (to_package == player_package) and us and us->GetTeam() == to->GetTeam())
            playbits or_eq TO_PACKAGE;

        if ((to and to_package == player_package and us->GetTeam() == to->GetTeam()) or
            (from and from_package == player_package and us and us->GetTeam() == from->GetTeam()))
            playbits or_eq TOFROM_PACKAGE;

        //campaign/taceng
        if (dataBlock.to == MESSAGE_FOR_TEAM and to and us and us->GetTeam() == to->GetTeam())
            playbits or_eq TO_TEAM;


        if (from and us and (DistSqu(from->XPos(), from->YPos(), us->XPos(), us->YPos()) < RADIO_PROX_RANGE * RADIO_PROX_RANGE)
           and us and ((to and us->GetTeam() == to->GetTeam()) or
                      (from and us->GetTeam() == from->GetTeam())))
            playbits or_eq IN_PROXIMITY;

        if (dataBlock.to == MESSAGE_FOR_WORLD)
            playbits or_eq TO_WORLD;

        int tac_channel;
        TacanList::StationSet set;
        TacanList::Domain domain;
        int range, ttype;
        float ilsfreq;

        if (from_entity and gTacanList->GetChannelFromVUID(from_entity->Id(), &tac_channel, &set, &domain, &range, &ttype, &ilsfreq))
            playbits or_eq TOFROM_TOWER;

        if (to_entity and gTacanList->GetChannelFromVUID(to_entity->Id(), &tac_channel, &set, &domain, &range, &ttype, &ilsfreq))
            playbits or_eq TOFROM_TOWER;
    }

    if (voiceFilter and playbits)
    {
        // KCK: This assumes voicefilter checks the proximity of the event to us and returns
        // true if it's near enough to us to care

        //MI if this is here, we're always in proximity mode
        // JPO - yeah - but if not - it fails to decode BRA/Bullseye data
        if (voiceFilter->GetBullseyeComm(&message, dataBlock.edata))
            playbits or_eq IN_PROXIMITY;


#ifndef ROBIN
        //MonoPrint("Queuing Chatter Message ID: %d\n", message);
#endif

        if (to)
            toID = to->Id();

        // FRB - chatter control
        //dataBlock.time_to_play = g_nChatterInterval * CampaignSeconds;
        // Now play the message (You'll want to pass in the playbits)
        voiceFilter->PlayRadioMessage(dataBlock.voice_id, (short)message, dataBlock.edata, vuxGameTime + dataBlock.time_to_play, playbits, channel, dataBlock.from, EVAL_BY_VALUE, toID);
    }

    return 0;
}

// ==============================
// Conversion functions
// ==============================

short ConvertFlightNumberToCallNumber(int flight_num)
{
    return (short)(flight_num + VF_FLIGHTNUMBER_OFFSET);
}

short ConvertWingNumberToCallNumber(int wing_num)
{
    return (short)(wing_num + 1 + VF_SHORTCALLSIGN_OFFSET);
}

short ConvertToCallNumber(int flight_num, int wing_num)
{
    if (flight_num < 0 and wing_num >= 0)
        return (short)(wing_num + VF_SHORTCALLSIGN_OFFSET);

    if (flight_num >= 0 and wing_num < 0)
        return (short)(flight_num + VF_FLIGHTNUMBER_OFFSET);

    return (short)((flight_num - 1) * 4 + wing_num + 1);
}


// ==============================
// Support functions
// ==============================

FalconRadioChatterMessage* CreateCallFromATC(Objective airbase, AircraftClass* aircraft, short call, VuTargetEntity *target)
{
    FalconRadioChatterMessage *radioMessage;

    short tod;
    int time_in_minutes;
    ShiAssert(airbase);
    ShiAssert(aircraft);

    Flight flight = (Flight)aircraft->GetCampaignObject();

    ShiAssert(flight);

    radioMessage = new FalconRadioChatterMessage(aircraft->Id() , target);
    radioMessage->dataBlock.message = call;
    radioMessage->dataBlock.to = MESSAGE_FOR_FLIGHT;
    radioMessage->dataBlock.from = airbase->Id();
    radioMessage->dataBlock.voice_id = (uchar)airbase->brain->Voice();
    radioMessage->dataBlock.time_to_play = 0;

    switch (call)
    {
        case rcCLEAREDLAND:
        case rcATCLANDSEQUENCE:
            time_in_minutes =  TheCampaign.GetMinutesSinceMidnight();

            if (time_in_minutes < 180)//3am
                tod = 1;
            else if (time_in_minutes < 720)//noon
                tod = 0;
            else if (time_in_minutes < 1020) //5pm
                tod = 2;
            else
                tod = 1;

            //these calls all are preceeded by a greeting
            radioMessage->dataBlock.edata[0] = tod;
            radioMessage->dataBlock.edata[1] = flight->callsign_id;
            radioMessage->dataBlock.edata[2] = ConvertToCallNumber(flight->callsign_num, aircraft->vehicleInUnit);

            if (airbase->brain->Approach() == 32766) // no ATC flag
                radioMessage->dataBlock.edata[3] = 32766;
            else
                radioMessage->dataBlock.edata[3] = airbase->brain->Tower();

            break;

        case rcCONTINUEINBOUND1:
        case rcCONTINUEINBOUND3:
            time_in_minutes =  TheCampaign.GetMinutesSinceMidnight();

            if (time_in_minutes < 180)//3am
                tod = 1;
            else if (time_in_minutes < 720)//noon
                tod = 0;
            else if (time_in_minutes < 1020) //5pm
                tod = 2;
            else
                tod = 1;

            //these calls all are preceeded by a greeting
            radioMessage->dataBlock.edata[0] = tod;
            radioMessage->dataBlock.edata[1] = flight->callsign_id;
            radioMessage->dataBlock.edata[2] = (short)ConvertFlightNumberToCallNumber(flight->callsign_num);
            radioMessage->dataBlock.edata[3] = (short)airbase->brain->Approach();
            break;

        case rcOUTSIDEAIRSPEED: //should be rcOUTSIDEAIRSPACE
            //needs to be converted to use approach instead of a callsign
            radioMessage->dataBlock.edata[0] = flight->callsign_id;
            radioMessage->dataBlock.edata[1] = ConvertFlightNumberToCallNumber(flight->callsign_num);
            radioMessage->dataBlock.edata[2] = -1;
            radioMessage->dataBlock.edata[3] = -1;
            break;

        case rcUSEALTFIELD: //needs to be converted to use approach instead of a callsign
            radioMessage->dataBlock.edata[0] = flight->callsign_id;
            radioMessage->dataBlock.edata[1] = ConvertToCallNumber(flight->callsign_num, aircraft->vehicleInUnit);
            radioMessage->dataBlock.edata[2] = -1;
            radioMessage->dataBlock.edata[3] = -1;
            break;

        case rcCONTINUEINBOUND2:
            //these calls use callsign, callnumber, approach
            radioMessage->dataBlock.edata[0] = flight->callsign_id;
            radioMessage->dataBlock.edata[1] = ConvertFlightNumberToCallNumber(flight->callsign_num);
            radioMessage->dataBlock.edata[2] = (short)airbase->brain->Approach();
            break;

        case rcPOSITIONANDHOLD:
        case rcCLEAREDONRUNWAY:
            //these calls use callsign, callnumber, tower
            radioMessage->dataBlock.edata[0] = flight->callsign_id;
            radioMessage->dataBlock.edata[1] = ConvertToCallNumber(flight->callsign_num, aircraft->vehicleInUnit);

            if (airbase->brain->Approach() == 32766) // no ATC flag
                radioMessage->dataBlock.edata[2] = 32766;
            else
                radioMessage->dataBlock.edata[2] = airbase->brain->Tower();

            // radioMessage->dataBlock.edata[2] = (short)airbase->brain->Tower();
            break;

        case rcHOLDPATTERN:
        case rcATCALTITUDE:
        case rcATCDIVERT:
        case rcATCTRAFFICWARNING:
        case rcATCTRAFFICWARNING2:
        case rcATSCOLDVECTOR:
        case rcCLEAREDEMERGLAND:
        case rcATCSCOLDTRAFFIC:
        case rcATCFOLLOWTRAFIC:
            //these calls use callsign, callnumber, approach
            radioMessage->dataBlock.edata[0] = flight->callsign_id;
            radioMessage->dataBlock.edata[1] = ConvertToCallNumber(flight->callsign_num, aircraft->vehicleInUnit);
            radioMessage->dataBlock.edata[2] = (short)airbase->brain->Approach();
            break;

        case rcCLEAREDDEPARTURE:
        case rcTAXICLEAR:
        case rcEXPEDITEDEPARTURE:
        case rcATCCANCELMISSION:
        case rcTOWERSCOLD1:
        case rcTAXISEQUENCE:
        case rcTOWERSCOLD2:
        case rcTOWERSCOLD3:
            //these calls use callsign, callnumber, tower
            radioMessage->dataBlock.edata[0] = flight->callsign_id;
            radioMessage->dataBlock.edata[1] = ConvertToCallNumber(flight->callsign_num, aircraft->vehicleInUnit);

            // M.N. ID 32766 turns off ATC name
            if (airbase->brain->Approach() == 32766) // no ATC flag
                radioMessage->dataBlock.edata[2] = 32766;
            else
                radioMessage->dataBlock.edata[2] = (short)airbase->brain->Tower();

            // radioMessage->dataBlock.edata[2] = (short)airbase->brain->Tower();
            break;

        case rcDISRUPTINGTRAFFIC:
        case rcGETOFFRUNWAYA:
        case rcGETOFFRUNWAYB:
        case rcHOLDSHORT:
        case rcHURRYUP:
        case rcCLEARTOTAXI:
        case rcATCVECTORS:
        case rcATCVECTORSRW: // JB 010527 (from MN) // new radio chatter
        case rcATCORBIT1:
        case rcATCORBIT2:
        case rcTURNTOFINAL:
        case rcATCSCOLD1:
        case rcATCGOAROUND:
        case rcATCGOAROUND2:
            //these calls use just callsign, callnum
            radioMessage->dataBlock.edata[0] = flight->callsign_id;
            radioMessage->dataBlock.edata[1] = ConvertToCallNumber(flight->callsign_num, aircraft->vehicleInUnit);
            break;

        case rcDEPARTHEADING://Cobra
        case rcRESUMEOWNNAV://Cobra
        case rcLANDINGCHECK://Cobra
            radioMessage->dataBlock.edata[0] = flight->callsign_id;
            radioMessage->dataBlock.edata[1] = ConvertToCallNumber(flight->callsign_num, aircraft->vehicleInUnit);
            radioMessage->dataBlock.edata[2] = airbase->brain->Tower();

            break;

    }

    return radioMessage;
}

void SendCallFromATC(Objective airbase, AircraftClass* aircraft, short call, VuTargetEntity *target)
{
    FalconRadioChatterMessage *radioMessage = CreateCallFromATC(airbase, aircraft, call, target);
    FalconSendMessage(radioMessage, FALSE);
}


FalconRadioChatterMessage* CreateCallToATC(AircraftClass* aircraft, short call, VuTargetEntity *target)
{
    FalconRadioChatterMessage *radioMessage;

    ShiAssert(aircraft);

    Objective airbase = (ObjectiveClass*)vuDatabase->Find(aircraft->DBrain()->Airbase());
    Flight flight = (Flight)aircraft->GetCampaignObject();

    ShiAssert(flight);

    if (airbase)
    {
        radioMessage = new FalconRadioChatterMessage(airbase->Id() , target);
    }
    else
    {
        radioMessage = new FalconRadioChatterMessage(FalconNullId , target);
    }

    radioMessage->dataBlock.time_to_play = 0;

    switch (call)
    {
        case rcAPPROACH:
        case rcLANDCLEARANCE:
        case rcTAKEOFFCLEARANCE:
        case rcLANDCLEAREMERGENCY:
        case rcONRUNWAY:

        case rcREQUESTTAKEOFFCLEARANCE:

            radioMessage->dataBlock.edata[0] = -1;
            radioMessage->dataBlock.edata[1] = -1;
            radioMessage->dataBlock.voice_id = flight->GetPilotVoiceID(aircraft->vehicleInUnit);
            radioMessage->dataBlock.from = aircraft->Id();
            radioMessage->dataBlock.edata[2] = flight->callsign_id;
            radioMessage->dataBlock.edata[3] = ConvertToCallNumber(flight->callsign_num, aircraft->vehicleInUnit);
            radioMessage->dataBlock.to = MESSAGE_FOR_FLIGHT;
            radioMessage->dataBlock.message = call;
            break;

        case rcREADYFORDERARTURE:

            radioMessage->dataBlock.edata[0] = -1;
            radioMessage->dataBlock.voice_id = flight->GetPilotVoiceID(aircraft->vehicleInUnit);
            radioMessage->dataBlock.from = aircraft->Id();
            radioMessage->dataBlock.edata[1] = flight->callsign_id;
            radioMessage->dataBlock.edata[2] = ConvertToCallNumber(flight->callsign_num, aircraft->vehicleInUnit);
            radioMessage->dataBlock.to = MESSAGE_FOR_FLIGHT;
            radioMessage->dataBlock.message = call;
            break;

        case rcCOPY:  //RAS-21Jan04-For Traffic Call Acknowledgement

            radioMessage->dataBlock.edata[0] = -1;
            radioMessage->dataBlock.voice_id = flight->GetPilotVoiceID(aircraft->vehicleInUnit);
            radioMessage->dataBlock.from = aircraft->Id();
            radioMessage->dataBlock.edata[1] = -1;
            radioMessage->dataBlock.edata[2] = flight->callsign_id;
            radioMessage->dataBlock.edata[3] = ConvertToCallNumber(flight->callsign_num, aircraft->vehicleInUnit);
            radioMessage->dataBlock.to = MESSAGE_FOR_FLIGHT;
            radioMessage->dataBlock.message = call;
            break;

    }

    return radioMessage;
}

void SendCallToATC(AircraftClass* aircraft, short call, VuTargetEntity *target)
{
    FalconRadioChatterMessage *radioMessage = CreateCallToATC(aircraft, call, target);
    FalconSendMessage(radioMessage, FALSE);
}

FalconRadioChatterMessage* CreateCallToATC(AircraftClass* aircraft, VU_ID airbaseID, short call, VuTargetEntity *target)
{
    FalconRadioChatterMessage *radioMessage;

    ShiAssert(aircraft);

    Flight flight = (Flight)aircraft->GetCampaignObject();

    ShiAssert(flight);

    radioMessage = new FalconRadioChatterMessage(airbaseID , target);

    radioMessage->dataBlock.time_to_play = 0;

    switch (call)
    {
        case rcAPPROACH:
        case rcLANDCLEARANCE:
        case rcTAKEOFFCLEARANCE:
        case rcLANDCLEAREMERGENCY:
        case rcONRUNWAY:
        case rcABORTAPPROACH: // M.N.
        case rcREQUESTTAKEOFFCLEARANCE:

            radioMessage->dataBlock.edata[0] = -1;
            radioMessage->dataBlock.edata[1] = -1;
            radioMessage->dataBlock.voice_id = flight->GetPilotVoiceID(aircraft->vehicleInUnit);
            radioMessage->dataBlock.from = aircraft->Id();
            radioMessage->dataBlock.edata[2] = flight->callsign_id;
            radioMessage->dataBlock.edata[3] = ConvertToCallNumber(flight->callsign_num, aircraft->vehicleInUnit);
            radioMessage->dataBlock.to = MESSAGE_FOR_FLIGHT;
            radioMessage->dataBlock.message = call;
            break;

        case rcREADYFORDERARTURE:

            radioMessage->dataBlock.edata[0] = -1;
            radioMessage->dataBlock.voice_id = flight->GetPilotVoiceID(aircraft->vehicleInUnit);
            radioMessage->dataBlock.from = aircraft->Id();
            radioMessage->dataBlock.edata[1] = flight->callsign_id;
            radioMessage->dataBlock.edata[2] = ConvertToCallNumber(flight->callsign_num, aircraft->vehicleInUnit);
            radioMessage->dataBlock.to = MESSAGE_FOR_FLIGHT;
            radioMessage->dataBlock.message = call;
            break;
    }

    return radioMessage;
}

void SendCallToATC(AircraftClass* aircraft, VU_ID airbaseID, short call, VuTargetEntity *target)
{
    FalconRadioChatterMessage *radioMessage = CreateCallToATC(aircraft, airbaseID, call, target);
    FalconSendMessage(radioMessage, FALSE);
}


FalconRadioChatterMessage* CreateCallToAWACS(AircraftClass* aircraft, short call, VuTargetEntity *target)
{
    FalconRadioChatterMessage *radioMessage;
    Flight awacs, flight;

    flight = (Flight)aircraft->GetCampaignObject();
    ShiAssert(flight);

    // AWACS/FAC callsign
    awacs = flight->GetFlightController();

    if (awacs)
    {
        radioMessage = new FalconRadioChatterMessage(awacs->Id() , target);
        radioMessage->dataBlock.edata[0] = awacs->callsign_id;
        radioMessage->dataBlock.edata[1] = ConvertFlightNumberToCallNumber(awacs->callsign_num);
    }
    else
    {
        radioMessage = new FalconRadioChatterMessage(FalconNullId , target);
        radioMessage->dataBlock.edata[0] = gDefaultAWACSCallSign;
        radioMessage->dataBlock.edata[1] = ConvertFlightNumberToCallNumber(gDefaultAWACSFlightNum);
    }

    // Flight data
    radioMessage->dataBlock.voice_id = flight->GetPilotVoiceID(aircraft->vehicleInUnit);
    radioMessage->dataBlock.from = aircraft->Id();
    radioMessage->dataBlock.edata[2] = flight->callsign_id;
    radioMessage->dataBlock.edata[3] = ConvertToCallNumber(flight->callsign_num, aircraft->vehicleInUnit);
    radioMessage->dataBlock.to = MESSAGE_FOR_FLIGHT;
    radioMessage->dataBlock.message = call;
    radioMessage->dataBlock.time_to_play = 0;
    //radioMessage->dataBlock.time_to_play = g_nChatterInterval * CampaignSeconds;

    return radioMessage;
}

void SendCallToAWACS(AircraftClass* aircraft, short call, VuTargetEntity *target)
{
    FalconRadioChatterMessage *radioMessage = CreateCallToAWACS(aircraft, call, target);
    FalconSendMessage(radioMessage, FALSE);
}

FalconRadioChatterMessage* CreateCallToAWACS(Flight flight, short call, VuTargetEntity *target)
{
    FalconRadioChatterMessage *radioMessage;
    Flight awacs;

    ShiAssert(flight);

    // AWACS/FAC callsign
    awacs = flight->GetFlightController();

    if (awacs)
    {
        radioMessage = new FalconRadioChatterMessage(awacs->Id() , target);
        radioMessage->dataBlock.edata[0] = awacs->callsign_id;
        radioMessage->dataBlock.edata[1] = ConvertFlightNumberToCallNumber(awacs->callsign_num);
    }
    else
    {
        radioMessage = new FalconRadioChatterMessage(FalconNullId , target);
        radioMessage->dataBlock.edata[0] = gDefaultAWACSCallSign;
        radioMessage->dataBlock.edata[1] = ConvertFlightNumberToCallNumber(gDefaultAWACSFlightNum);
    }

    // Flight data
    radioMessage->dataBlock.voice_id = flight->GetFlightLeadVoiceID();
    radioMessage->dataBlock.from = flight->Id();
    radioMessage->dataBlock.edata[2] = flight->callsign_id;
    radioMessage->dataBlock.edata[3] = ConvertFlightNumberToCallNumber(flight->callsign_num);
    radioMessage->dataBlock.to = MESSAGE_FOR_FLIGHT;
    radioMessage->dataBlock.message = call;
    radioMessage->dataBlock.time_to_play = 0;

    return radioMessage;
}

void SendCallToAWACS(Flight flight, short call, VuTargetEntity *target)
{
    FalconRadioChatterMessage *radioMessage = CreateCallToAWACS(flight, call, target);
    FalconSendMessage(radioMessage, FALSE);
}


FalconRadioChatterMessage* CreateCallFromAwacs(Flight flight, short call, VuTargetEntity *target)
{
    FalconRadioChatterMessage *radioMessage;
    Flight awacs;

    ShiAssert(flight);

    // AWACS/FAC callsign
    awacs = flight->GetFlightController();

    // Flight data
    radioMessage = new FalconRadioChatterMessage(flight->Id() , target);
    radioMessage->dataBlock.edata[0] = flight->callsign_id;
    radioMessage->dataBlock.edata[1] = ConvertFlightNumberToCallNumber(flight->callsign_num);
    radioMessage->dataBlock.to = MESSAGE_FOR_FLIGHT;


    if (awacs)
    {
        radioMessage->dataBlock.from = awacs->Id();
        radioMessage->dataBlock.voice_id = awacs->GetFlightLeadVoiceID();
        radioMessage->dataBlock.edata[2] = awacs->callsign_id;
        radioMessage->dataBlock.edata[3] = ConvertFlightNumberToCallNumber(awacs->callsign_num);
    }
    else
    {
        radioMessage->dataBlock.from = FalconNullId;
        radioMessage->dataBlock.voice_id = GetDefaultAwacsVoice(); // JPO VOICEFIX
        radioMessage->dataBlock.edata[2] = gDefaultAWACSCallSign;
        radioMessage->dataBlock.edata[3] = ConvertFlightNumberToCallNumber(gDefaultAWACSFlightNum);
    }

    radioMessage->dataBlock.message = call;
    radioMessage->dataBlock.time_to_play = 0;
    //radioMessage->dataBlock.time_to_play = g_nChatterInterval * CampaignSeconds;

    return radioMessage;
}

FalconRadioChatterMessage* CreateCallFromAwacsPlane(AircraftClass* plane, short call, VuTargetEntity *target)
{
    FalconRadioChatterMessage *radioMessage;
    Flight awacs, flight;

    flight = (Flight)plane->GetCampaignObject();
    ShiAssert(flight);

    // AWACS/FAC callsign
    awacs = flight->GetFlightController();

    // Flight data
    radioMessage = new FalconRadioChatterMessage(flight->Id() , target);
    radioMessage->dataBlock.to = MESSAGE_FOR_FLIGHT;

    int flightIdx = flight->GetComponentIndex(plane);
    radioMessage->dataBlock.edata[0] = flight->callsign_id;
    radioMessage->dataBlock.edata[1] = (flight->callsign_num - 1) * 4 + flightIdx + 1;

    if (awacs)
    {
        radioMessage->dataBlock.from = awacs->Id();
        radioMessage->dataBlock.voice_id = awacs->GetFlightLeadVoiceID();
        radioMessage->dataBlock.edata[2] = awacs->callsign_id;
        radioMessage->dataBlock.edata[3] = ConvertFlightNumberToCallNumber(awacs->callsign_num);
    }
    else
    {
        radioMessage->dataBlock.from = FalconNullId;
        radioMessage->dataBlock.voice_id = GetDefaultAwacsVoice(); // JPO VOICEFIX
        radioMessage->dataBlock.edata[2] = gDefaultAWACSCallSign;
        radioMessage->dataBlock.edata[3] = ConvertFlightNumberToCallNumber(gDefaultAWACSFlightNum);
    }

    radioMessage->dataBlock.message = call;
    radioMessage->dataBlock.time_to_play = 0;
    //radioMessage->dataBlock.time_to_play = g_nChatterInterval * CampaignSeconds;

    return radioMessage;
}

// This will send a simple call FROM AWACS/FAC (ie: a call with only the callsigns as evals)
void SendCallFromAwacs(Flight flight, short call, VuTargetEntity *target)
{
    FalconRadioChatterMessage *radioMessage = CreateCallFromAwacs(flight, call, target);
    FalconSendMessage(radioMessage, FALSE);
}

FalconRadioChatterMessage* CreateCallToFlight(Flight flight, FalconEntity *from, short call, VuTargetEntity *target)
{
    FalconRadioChatterMessage *radioMessage;

    VU_ID fromID;
    short fromCallsign, fromCallnum;
    uchar fromVoice;

    ShiAssert(flight);

    if ( not from)
    {
        fromID = FalconNullId;
        fromVoice =  GetDefaultAwacsVoice(); // JPO VOICEFIX
        fromCallsign = -1;
        fromCallnum = -1;
    }
    else if (from->IsAirplane())
    {
        Flight fromFlight = ((Flight)((AircraftClass*)from)->GetCampaignObject());
        fromID = from->Id();
        fromVoice =  fromFlight->GetPilotVoiceID(((AircraftClass*)from)->pilotSlot);
        fromCallsign = fromFlight->callsign_id;;
        fromCallnum = ConvertToCallNumber(fromFlight->callsign_num, ((AircraftClass*)from)->vehicleInUnit);
    }
    else if (from->IsFlight())
    {
        fromID = from->Id();
        fromVoice = ((Flight)from)->GetFlightLeadVoiceID();
        fromCallsign = ((Flight)from)->callsign_id;
        fromCallnum = ConvertFlightNumberToCallNumber(((Flight)from)->callsign_num);
    }
    else
    {
        fromID = from->Id();
        fromVoice =  GetDefaultAwacsVoice(); // JPO VOICEFIX
        fromCallsign = -1;
        fromCallnum = -1;
    }

    // Flight data
    radioMessage = new FalconRadioChatterMessage(flight->Id() , target);
    radioMessage->dataBlock.edata[0] = flight->callsign_id;
    radioMessage->dataBlock.edata[1] = ConvertFlightNumberToCallNumber(flight->callsign_num);
    radioMessage->dataBlock.to = MESSAGE_FOR_FLIGHT;
    radioMessage->dataBlock.from = fromID;
    radioMessage->dataBlock.voice_id = fromVoice;
    radioMessage->dataBlock.edata[2] = fromCallsign;
    radioMessage->dataBlock.edata[3] = fromCallnum;
    radioMessage->dataBlock.message = call;
    radioMessage->dataBlock.time_to_play = 0;
    //radioMessage->dataBlock.time_to_play = g_nChatterInterval * CampaignSeconds;

    return radioMessage;
}

// This will send a simple call FROM from (ie: a call with only the callsigns as evals)
void SendCallToFlight(Flight flight, FalconEntity *from, short call, VuTargetEntity *target)
{
    FalconRadioChatterMessage *radioMessage = CreateCallToFlight(flight, from, call, target);
    FalconSendMessage(radioMessage, FALSE);
}

FalconRadioChatterMessage* CreateCallToPlane(AircraftClass* aircraft, FalconEntity *from, short call, VuTargetEntity *target)
{
    FalconRadioChatterMessage *radioMessage;
    Flight flight;
    VU_ID fromID, toID;
    short fromCallsign, fromCallnum;
    short toCallsign, toCallnum;
    uchar fromVoice;

    if ( not from)
    {
        fromID = FalconNullId;
        fromVoice =  GetDefaultAwacsVoice(); // JPO VOICEFIX
        fromCallsign = -1;
        fromCallnum = -1;
    }
    else if (from->IsAirplane())
    {
        Flight fromFlight = ((Flight)((AircraftClass*)from)->GetCampaignObject());
        fromID = from->Id();
        fromVoice =  fromFlight->GetPilotVoiceID(((AircraftClass*)from)->pilotSlot);
        fromCallsign = fromFlight->callsign_id;;
        fromCallnum = ConvertToCallNumber(fromFlight->callsign_num, ((AircraftClass*)from)->vehicleInUnit);
    }
    else if (from->IsFlight())
    {
        fromID = from->Id();
        fromVoice = ((Flight)from)->GetFlightLeadVoiceID();
        fromCallsign = ((Flight)from)->callsign_id;;
        fromCallnum = ConvertFlightNumberToCallNumber(((Flight)from)->callsign_num);
    }
    else
    {
        fromID = from->Id();
        fromVoice =  GetDefaultAwacsVoice(); // JPO VOICEFIX
        fromCallsign = -1;
        fromCallnum = -1;
    }

    if ( not aircraft)
    {
        toID = FalconNullId;
        toCallsign = -1;
        toCallnum = -1;
    }
    else
    {
        flight = (Flight)aircraft->GetCampaignObject();

        toID = aircraft->Id();
        toCallsign = flight->callsign_id;
        toCallnum = ConvertToCallNumber(flight->callsign_num, aircraft->vehicleInUnit);
    }

    // Flight data
    radioMessage = new FalconRadioChatterMessage(toID , target);
    radioMessage->dataBlock.edata[0] = toCallsign;
    radioMessage->dataBlock.edata[1] = toCallnum;
    radioMessage->dataBlock.to = MESSAGE_FOR_FLIGHT;
    radioMessage->dataBlock.from = fromID;
    radioMessage->dataBlock.voice_id = fromVoice;
    radioMessage->dataBlock.edata[2] = fromCallsign;
    radioMessage->dataBlock.edata[3] = fromCallnum;
    radioMessage->dataBlock.message = call;
    radioMessage->dataBlock.time_to_play = 0;
    //radioMessage->dataBlock.time_to_play = g_nChatterInterval * CampaignSeconds;

    return radioMessage;
}

// This will send a simple call FROM from (ie: a call with only the callsigns as evals)
void SendCallToPlane(AircraftClass* aircraft, FalconEntity *from, short call, VuTargetEntity *target)
{
    FalconRadioChatterMessage *radioMessage = CreateCallToPlane(aircraft, from, call, target);
    FalconSendMessage(radioMessage, FALSE);
}

// This will send a simple call FROM from (ie: a call with only the callsigns as evals)
void SendRogerToPlane(AircraftClass* aircraft, FalconEntity *from, VuTargetEntity *target)
{
    FalconRadioChatterMessage *radioMessage;
    Flight flight;
    VU_ID fromID;
    short fromCallsign, fromCallnum;
    uchar fromVoice;

    ShiAssert(aircraft);
    flight = (Flight)aircraft->GetCampaignObject();

    if ( not from)
    {
        return;
    }
    else if (from->IsAirplane())
    {
        Flight fromFlight = ((Flight)((AircraftClass*)from)->GetCampaignObject());
        fromID = from->Id();
        fromVoice =  fromFlight->GetPilotVoiceID(((AircraftClass*)from)->pilotSlot);
        fromCallsign = fromFlight->callsign_id;;
        fromCallnum = ConvertToCallNumber(fromFlight->callsign_num, ((AircraftClass*)from)->vehicleInUnit);
    }
    else if (from->IsFlight())
    {
        fromID = from->Id();
        fromVoice = ((Flight)from)->GetFlightLeadVoiceID();
        fromCallsign = ((Flight)from)->callsign_id;;
        fromCallnum = ConvertFlightNumberToCallNumber(((Flight)from)->callsign_num);
    }
    else
    {
        fromID = from->Id();
        fromVoice =  GetDefaultAwacsVoice(); // JPO VOICEFIX
        fromCallsign = -1;
        fromCallnum = -1;
    }

    // Flight data
    radioMessage = new FalconRadioChatterMessage(aircraft->Id() , target);
    radioMessage->dataBlock.to = MESSAGE_FOR_FLIGHT;
    radioMessage->dataBlock.from = fromID;
    radioMessage->dataBlock.voice_id = fromVoice;
    radioMessage->dataBlock.edata[0] = -1;
    radioMessage->dataBlock.edata[1] = 1;
    radioMessage->dataBlock.message = rcROGER;

    if (PlayerOptions.PlayerRadioVoice)
        //radioMessage->dataBlock.time_to_play = g_nChatterInterval * CampaignSeconds;
        radioMessage->dataBlock.time_to_play = 2 * CampaignSeconds;
    else
        radioMessage->dataBlock.time_to_play = 500;

    FalconSendMessage(radioMessage, FALSE);
}

static uchar gDefaultAWACSVoice = 255;

uchar GetDefaultAwacsVoice()
{
    if (gDefaultAWACSVoice == 255)
    {
        gDefaultAWACSVoice = g_voicemap.PickVoice(VoiceMapper::VOICE_AWACS, VoiceMapper::VOICE_SIDE_UNK);
    }

    return gDefaultAWACSVoice;
}

void ResetDefaultAwacsVoice()
{
    gDefaultAWACSVoice = g_voicemap.PickVoice(VoiceMapper::VOICE_AWACS, VoiceMapper::VOICE_SIDE_UNK);
}
